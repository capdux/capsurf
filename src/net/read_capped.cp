// Migrated from vendor/kitesurf/packages/engine/src/read-capped.ts
// CapSurf Streaming Byte Budget & In-Flight Resource Protection

pub const MAX_DOCUMENT_BYTES: usize = 16 * 1024 * 1024;
pub const MAX_STYLESHEET_BYTES: usize = 8 * 1024 * 1024;
pub const MAX_CONCURRENT_READ_BYTES: usize = 48 * 1024 * 1024;
pub const MAX_CONNECTION_READ_BYTES: usize = 24 * 1024 * 1024;
pub const MAX_TRACKED_CONNECTIONS: usize = 32;
pub const MAX_CONNECTION_ID_LEN: usize = 64;

pub enum CappedFailure {
    None,
    CapExceeded,
    BudgetExceeded,
    IoError,

    fn asStr(): []const u8 {
        return switch (_) {
            .None => "none",
            .CapExceeded => "net::ERR_FILE_TOO_BIG",
            .BudgetExceeded => "net::ERR_INSUFFICIENT_RESOURCES",
            .IoError => "net::ERR_CONNECTION_CLOSED",
        };
    }
}

pub struct ConnectionBudgetEntry {
    conn_id: [64]u8 = [0; 64],
    conn_id_len: usize = 0,
    in_flight_bytes: usize = 0,
    active: bool = false,

    fn getId(*_): []const u8 {
        return _.conn_id[0.._.conn_id_len];
    }

    fn matches(*_, id: []const u8): bool {
        if (!_.active) return false;
        if (_.conn_id_len != id.len) return false;
        return std.mem.eql(u8, _.conn_id[0.._.conn_id_len], id);
    }

    fn setId(*_, id: []const u8) {
        let copy_len = id.len;
        if (copy_len > 64) {
            copy_len = 64;
        }
        @memcpy(_.conn_id[0..copy_len], id[0..copy_len]);
        _.conn_id_len = copy_len;
    }

    fn reset(*_) {
        _.conn_id_len = 0;
        _.in_flight_bytes = 0;
        _.active = false;
    }
}

pub struct ReadBudgetStats {
    in_flight_bytes: usize = 0,
    peak_in_flight_bytes: usize = 0,
    active_connections: usize = 0,
    max_concurrent_bytes: usize = MAX_CONCURRENT_READ_BYTES,
    max_connection_bytes: usize = MAX_CONNECTION_READ_BYTES,
}

pub struct ReadBudgetTracker {
    entries: [32]ConnectionBudgetEntry = [ConnectionBudgetEntry{}; 32],
    in_flight_bytes: usize = 0,
    peak_in_flight_bytes: usize = 0,
    max_concurrent_bytes: usize = MAX_CONCURRENT_READ_BYTES,
    max_connection_bytes: usize = MAX_CONNECTION_READ_BYTES,
    active_count: usize = 0,

    static fn init(): ReadBudgetTracker {
        return ReadBudgetTracker{};
    }

    static fn initWithLimits(max_concurrent: usize, max_connection: usize): ReadBudgetTracker {
        return ReadBudgetTracker{
            max_concurrent_bytes: max_concurrent,
            max_connection_bytes: max_connection,
        };
    }

    fn inFlightBytes(): usize {
        return _.in_flight_bytes;
    }

    fn peakInFlightBytes(): usize {
        return _.peak_in_flight_bytes;
    }

    fn resetPeak(*_) {
        _.peak_in_flight_bytes = _.in_flight_bytes;
    }

    fn stats(): ReadBudgetStats {
        return ReadBudgetStats{
            in_flight_bytes: _.in_flight_bytes,
            peak_in_flight_bytes: _.peak_in_flight_bytes,
            active_connections: _.active_count,
            max_concurrent_bytes: _.max_concurrent_bytes,
            max_connection_bytes: _.max_connection_bytes,
        };
    }

    fn connectionInFlightBytes(*_, conn_id: []const u8): usize {
        let i: usize = 0;
        while (i < 32) : (i += 1) {
            if (_.entries[i].active && _.entries[i].matches(conn_id)) {
                return _.entries[i].in_flight_bytes;
            }
        }
        return 0;
    }

    fn chargeWithLimit(*_, conn_id: []const u8, delta: usize, conn_cap: usize): bool {
        const new_global = _.in_flight_bytes + delta;
        if (new_global > _.max_concurrent_bytes) {
            return false;
        }

        let idx: usize = 0;
        let found = false;
        let i: usize = 0;
        while (i < 32) : (i += 1) {
            if (_.entries[i].active && _.entries[i].matches(conn_id)) {
                idx = i;
                found = true;
                break;
            }
        }

        if (found) {
            const new_conn = _.entries[idx].in_flight_bytes + delta;
            if (new_conn > conn_cap) {
                return false;
            }
            _.entries[idx].in_flight_bytes = new_conn;
            _.in_flight_bytes = new_global;
            if (_.in_flight_bytes > _.peak_in_flight_bytes) {
                _.peak_in_flight_bytes = _.in_flight_bytes;
            }
            return true;
        }

        if (delta > conn_cap) {
            return false;
        }

        let free_idx: usize = 0;
        let found_free = false;
        let k: usize = 0;
        while (k < 32) : (k += 1) {
            if (!_.entries[k].active) {
                free_idx = k;
                found_free = true;
                break;
            }
        }

        if (!found_free) {
            return false;
        }

        _.entries[free_idx].active = true;
        _.entries[free_idx].setId(conn_id);
        _.entries[free_idx].in_flight_bytes = delta;
        _.active_count += 1;
        _.in_flight_bytes = new_global;
        if (_.in_flight_bytes > _.peak_in_flight_bytes) {
            _.peak_in_flight_bytes = _.in_flight_bytes;
        }
        return true;
    }

    fn charge(*_, conn_id: []const u8, delta: usize): bool {
        return _.chargeWithLimit(conn_id, delta, _.max_connection_bytes);
    }

    fn refund(*_, conn_id: []const u8, delta: usize) {
        let i: usize = 0;
        while (i < 32) : (i += 1) {
            if (_.entries[i].active && _.entries[i].matches(conn_id)) {
                const sub = if (delta > _.entries[i].in_flight_bytes) _.entries[i].in_flight_bytes else delta;
                _.entries[i].in_flight_bytes -= sub;
                if (_.entries[i].in_flight_bytes == 0) {
                    _.entries[i].reset();
                    if (_.active_count > 0) {
                        _.active_count -= 1;
                    }
                }
                if (_.in_flight_bytes >= sub) {
                    _.in_flight_bytes -= sub;
                } else {
                    _.in_flight_bytes = 0;
                }
                return;
            }
        }

        const global_sub = if (delta > _.in_flight_bytes) _.in_flight_bytes else delta;
        _.in_flight_bytes -= global_sub;
    }

    fn refundConnection(*_, conn_id: []const u8) {
        let i: usize = 0;
        while (i < 32) : (i += 1) {
            if (_.entries[i].active && _.entries[i].matches(conn_id)) {
                const conn_bytes = _.entries[i].in_flight_bytes;
                _.entries[i].reset();
                if (_.active_count > 0) {
                    _.active_count -= 1;
                }
                if (_.in_flight_bytes >= conn_bytes) {
                    _.in_flight_bytes -= conn_bytes;
                } else {
                    _.in_flight_bytes = 0;
                }
                return;
            }
        }
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < 32) : (i += 1) {
            _.entries[i].reset();
        }
        _.in_flight_bytes = 0;
        _.peak_in_flight_bytes = 0;
        _.active_count = 0;
    }

    fn acquireLease(*_, conn_id: []const u8, connection_cap: usize): ReadLease {
        let lease = ReadLease{
            tracker: _,
            connection_cap: connection_cap,
            charged_bytes: 0,
            released: false,
        };
        lease.setId(conn_id);
        return lease;
    }
}

pub struct ReadLease {
    tracker: *ReadBudgetTracker = undefined,
    conn_id: [64]u8 = [0; 64],
    conn_id_len: usize = 0,
    charged_bytes: usize = 0,
    connection_cap: usize = MAX_CONNECTION_READ_BYTES,
    released: bool = false,

    fn getId(*_): []const u8 {
        return _.conn_id[0.._.conn_id_len];
    }

    fn setId(*_, id: []const u8) {
        let copy_len = id.len;
        if (copy_len > 64) {
            copy_len = 64;
        }
        @memcpy(_.conn_id[0..copy_len], id[0..copy_len]);
        _.conn_id_len = copy_len;
    }

    fn charge(*_, delta: usize): bool {
        if (_.released) return false;
        if (!_.tracker.chargeWithLimit(_.getId(), delta, _.connection_cap)) {
            return false;
        }
        _.charged_bytes += delta;
        return true;
    }

    fn release(*_) {
        if (_.released) return;
        _.released = true;
        if (_.charged_bytes > 0) {
            _.tracker.refund(_.getId(), _.charged_bytes);
            _.charged_bytes = 0;
        }
    }
}

pub struct CappedBuffer {
    data: []u8 = [],
    len: usize = 0,
    allocator: (std.mem.Allocator)? = null,

    static fn fromSlice(storage: []u8): CappedBuffer {
        return CappedBuffer{
            data: storage,
            len: 0,
            allocator: null,
        };
    }

    static fn init(allocator: std.mem.Allocator): CappedBuffer {
        return CappedBuffer{
            data: [],
            len: 0,
            allocator: allocator,
        };
    }

    fn deinit(*_) {
        if (alloc := _.allocator) {
            if (_.data.len > 0) {
                alloc.free(_.data);
                _.data = [];
            }
        }
        _.len = 0;
    }

    fn getBytes(): []const u8 {
        return _.data[0.._.len];
    }

    fn capacity(): usize {
        return _.data.len;
    }

    fn clear(*_) {
        _.len = 0;
    }

    fn appendChunk(*_, chunk: []const u8, cap_limit: usize): CappedFailure {
        if (chunk.len == 0) return .None;
        if (_.len + chunk.len > cap_limit) {
            return .CapExceeded;
        }
        if (_.len + chunk.len > _.data.len) {
            if (alloc := _.allocator) {
                let new_cap = if (_.data.len == 0) 1024 else _.data.len * 2;
                while (new_cap < _.len + chunk.len && new_cap < cap_limit) {
                    new_cap *= 2;
                }
                if (new_cap > cap_limit) {
                    new_cap = cap_limit;
                }
                if (_.len + chunk.len > new_cap) {
                    return .CapExceeded;
                }
                const new_mem = alloc.alloc(u8, new_cap) catch {
                    return .IoError;
                };
                if (_.len > 0) {
                    @memcpy(new_mem[0.._.len], _.data[0.._.len]);
                }
                if (_.data.len > 0) {
                    alloc.free(_.data);
                }
                _.data = new_mem;
            } else {
                return .CapExceeded;
            }
        }
        @memcpy(_.data[_.len .. _.len + chunk.len], chunk);
        _.len += chunk.len;
        return .None;
    }
}

pub fn declaredOverCap(content_length_str: []const u8, cap: usize): bool {
    const trimmed = std.mem.trim(u8, content_length_str, " \t\r\n");
    if (trimmed.len == 0) return false;
    const n = std.fmt.parseInt(usize, trimmed, 10) catch return false;
    return n > cap;
}

pub fn cappedErrorText(failure: CappedFailure): []const u8 {
    return failure.asStr();
}

pub threadlocal let g_budget_tracker: ReadBudgetTracker = ReadBudgetTracker.init();

pub fn inFlightReadBytes(): usize {
    return g_budget_tracker.inFlightBytes();
}

pub fn peakInFlightReadBytes(): usize {
    return g_budget_tracker.peakInFlightBytes();
}

pub fn resetPeakInFlightReadBytes() {
    g_budget_tracker.resetPeak();
}

pub fn charge(conn_id: []const u8, delta: usize): bool {
    return g_budget_tracker.charge(conn_id, delta);
}

pub fn refund(conn_id: []const u8, delta: usize) {
    g_budget_tracker.refund(conn_id, delta);
}

pub fn trackerStats(): ReadBudgetStats {
    return g_budget_tracker.stats();
}
