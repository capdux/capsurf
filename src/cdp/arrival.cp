// Migrated from vendor/kitesurf/packages/engine/src/cdp/arrival.ts
// src/cdp/arrival.cp
// When the event loop handed the engine a client message: a request that waited before its
// handler ran shows that wait as arrival-to-handler time instead of vanishing between spans.

pub const MAX_ARRIVAL_SLOTS: usize = 512;
pub const MAX_ARRIVAL_REQ_ID_LEN: usize = 128;

pub struct ArrivalSlot {
    req_id: [128]u8 = [0; 128],
    req_id_len: usize = 0,
    timestamp_ms: i64 = 0,
    access_seq: u64 = 0,
    active: bool = false,
}

pub struct ArrivalTracker {
    slots: [512]ArrivalSlot = [ArrivalSlot{}; 512],
    count: usize = 0,
    seq_counter: u64 = 0,

    static fn init(): ArrivalTracker {
        return ArrivalTracker{};
    }

    fn mark(*_, req_id: []const u8, timestamp_ms: i64) {
        if (req_id.len == 0) return;
        _.seq_counter += 1;
        const now_seq = _.seq_counter;

        let i: usize = 0;
        while (i < MAX_ARRIVAL_SLOTS) : (i += 1) {
            if (_.slots[i].active && std.mem.eql(u8, _.slots[i].req_id[0.._.slots[i].req_id_len], req_id)) {
                _.slots[i].timestamp_ms = timestamp_ms;
                _.slots[i].access_seq = now_seq;
                return;
            }
        }

        let j: usize = 0;
        while (j < MAX_ARRIVAL_SLOTS) : (j += 1) {
            if (!_.slots[j].active) {
                const len = @min(req_id.len, MAX_ARRIVAL_REQ_ID_LEN);
                @memcpy(_.slots[j].req_id[0..len], req_id[0..len]);
                _.slots[j].req_id_len = len;
                _.slots[j].timestamp_ms = timestamp_ms;
                _.slots[j].access_seq = now_seq;
                _.slots[j].active = true;
                _.count += 1;
                return;
            }
        }

        let lru_idx: usize = 0;
        let min_seq: u64 = std.math.maxInt(u64);
        let k: usize = 0;
        while (k < MAX_ARRIVAL_SLOTS) : (k += 1) {
            if (_.slots[k].active && _.slots[k].access_seq < min_seq) {
                min_seq = _.slots[k].access_seq;
                lru_idx = k;
            }
        }

        const len = @min(req_id.len, MAX_ARRIVAL_REQ_ID_LEN);
        @memcpy(_.slots[lru_idx].req_id[0..len], req_id[0..len]);
        _.slots[lru_idx].req_id_len = len;
        _.slots[lru_idx].timestamp_ms = timestamp_ms;
        _.slots[lru_idx].access_seq = now_seq;
        _.slots[lru_idx].active = true;
    }

    fn get(*_, req_id: []const u8): i64? {
        if (req_id.len == 0) return null;
        let i: usize = 0;
        while (i < MAX_ARRIVAL_SLOTS) : (i += 1) {
            if (_.slots[i].active && std.mem.eql(u8, _.slots[i].req_id[0.._.slots[i].req_id_len], req_id)) {
                _.seq_counter += 1;
                _.slots[i].access_seq = _.seq_counter;
                return _.slots[i].timestamp_ms;
            }
        }
        return null;
    }

    fn peek(req_id: []const u8): i64? {
        if (req_id.len == 0) return null;
        let i: usize = 0;
        while (i < MAX_ARRIVAL_SLOTS) : (i += 1) {
            if (_.slots[i].active && std.mem.eql(u8, _.slots[i].req_id[0.._.slots[i].req_id_len], req_id)) {
                return _.slots[i].timestamp_ms;
            }
        }
        return null;
    }

    fn remove(*_, req_id: []const u8): bool {
        if (req_id.len == 0) return false;
        let i: usize = 0;
        while (i < MAX_ARRIVAL_SLOTS) : (i += 1) {
            if (_.slots[i].active && std.mem.eql(u8, _.slots[i].req_id[0.._.slots[i].req_id_len], req_id)) {
                _.slots[i].active = false;
                _.slots[i].req_id_len = 0;
                if (_.count > 0) _.count -= 1;
                return true;
            }
        }
        return false;
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_ARRIVAL_SLOTS) : (i += 1) {
            _.slots[i].active = false;
            _.slots[i].req_id_len = 0;
        }
        _.count = 0;
    }

    fn size(): usize {
        return _.count;
    }
}

@tls let g_arrival_tracker: ArrivalTracker = ArrivalTracker.init();

pub fn getGlobalArrivalTracker(): *ArrivalTracker {
    return &g_arrival_tracker;
}

pub fn markArrival(req_id: []const u8, timestamp_ms: i64) {
    g_arrival_tracker.mark(req_id, timestamp_ms);
}

pub fn markArrivalNow(req_id: []const u8) {
    g_arrival_tracker.mark(req_id, std.time.milliTimestamp());
}

pub fn arrivalOf(req_id: []const u8): i64? {
    return g_arrival_tracker.get(req_id);
}

pub fn peekArrival(req_id: []const u8): i64? {
    return g_arrival_tracker.peek(req_id);
}

pub fn removeArrival(req_id: []const u8): bool {
    return g_arrival_tracker.remove(req_id);
}

pub fn clearArrivals() {
    g_arrival_tracker.clear();
}
