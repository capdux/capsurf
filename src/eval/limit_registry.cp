// Migrated from vendor/kitesurf/packages/engine/src/eval/limit-registry.ts
// src/eval/limit_registry.cp
// Routes one-shot isolate-limit notices to handlers registered by script name.

pub const MAX_LIMIT_LISTENERS: usize = 64;
pub const MAX_LIMIT_SCRIPT_NAME_LEN: usize = 512;
pub const MAX_LIMIT_OWNER_LEN: usize = 128;
pub const MAX_LIMIT_TARGET_LEN: usize = 128;

pub enum IsolateLimitKind {
    cpu,
    memory,
    wall,
    gone,
    crashed,

    fn asStr(): []const u8 {
        return switch (_) {
            .cpu => "cpu",
            .memory => "memory",
            .wall => "wall",
            .gone => "gone",
            .crashed => "crashed",
        };
    }
}

pub struct IsolateLimitNotice {
    kind: IsolateLimitKind = .wall,
    cap_ms: u64 = 0,
    from_tail: bool = false,
    detail: []const u8 = "",
}

pub type LimitListenerFn = *const fn(notice: *const IsolateLimitNotice, user_data: *!?);
pub type SimpleLimitListenerFn = *const fn(notice: *const IsolateLimitNotice);
pub type IsLiveFn = *const fn(user_data: *!?): bool;

pub struct IsolateLimitListenerOptions {
    owner: []const u8 = "",
    target_id: []const u8 = "",
    is_live_fn: (*const fn(user_data: *!?): bool)? = null,
    user_data: *!? = null,
}

pub struct LimitRegistryEntry {
    script_name: [MAX_LIMIT_SCRIPT_NAME_LEN]u8 = [0; MAX_LIMIT_SCRIPT_NAME_LEN],
    script_name_len: usize = 0,
    owner: [MAX_LIMIT_OWNER_LEN]u8 = [0; MAX_LIMIT_OWNER_LEN],
    owner_len: usize = 0,
    target_id: [MAX_LIMIT_TARGET_LEN]u8 = [0; MAX_LIMIT_TARGET_LEN],
    target_id_len: usize = 0,
    cb: (*const fn(notice: *const IsolateLimitNotice, user_data: *!?))? = null,
    simple_cb: (*const fn(notice: *const IsolateLimitNotice))? = null,
    is_live_fn: (*const fn(user_data: *!?): bool)? = null,
    user_data: *!? = null,
    seq: u64 = 0,
    active: bool = false,
}

fn containsCaseInsensitive(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (haystack.len < needle.len) return false;
    let i: usize = 0;
    const max_start = haystack.len - needle.len;
    while (i <= max_start) : (i += 1) {
        let matched = true;
        let j: usize = 0;
        while (j < needle.len) : (j += 1) {
            if (std.ascii.toLower(haystack[i + j]) != std.ascii.toLower(needle[j])) {
                matched = false;
                break;
            }
        }
        if (matched) return true;
    }
    return false;
}

fn checkIsolateGone(message: []const u8): bool {
    if (containsCaseInsensitive(message, "execution context which hosts this")) return true;
    if (containsCaseInsensitive(message, "Too many dynamic workers")) return true;
    return false;
}

pub fn isIsolateGoneError(message: []const u8): bool {
    return checkIsolateGone(message);
}

pub struct LimitRegistry {
    entries: [MAX_LIMIT_LISTENERS]LimitRegistryEntry = [LimitRegistryEntry{}; MAX_LIMIT_LISTENERS],
    count: usize = 0,
    seq_counter: u64 = 0,

    static fn init(): LimitRegistry {
        return LimitRegistry{};
    }

    static fn isIsolateGoneError(message: []const u8): bool {
        return checkIsolateGone(message);
    }

    fn isRetired(*_, idx: usize): bool {
        if (!_.entries[idx].active) return true;
        if (fn_ptr := _.entries[idx].is_live_fn) {
            if (!fn_ptr(_.entries[idx].user_data)) {
                return true;
            }
        }
        return false;
    }

    fn makeRoom(*_) {
        let i: usize = 0;
        while (i < MAX_LIMIT_LISTENERS) : (i += 1) {
            if (_.entries[i].active && _.isRetired(i)) {
                _.entries[i].active = false;
                if (_.count > 0) _.count -= 1;
            }
        }
        if (_.count < MAX_LIMIT_LISTENERS) return;

        let oldest_idx: usize = 0;
        let oldest_seq: u64 = std.math.maxInt(u64);
        let found = false;
        let j: usize = 0;
        while (j < MAX_LIMIT_LISTENERS) : (j += 1) {
            if (_.entries[j].active) {
                if (!found || _.entries[j].seq < oldest_seq) {
                    oldest_seq = _.entries[j].seq;
                    oldest_idx = j;
                    found = true;
                }
            }
        }
        if (found) {
            _.entries[oldest_idx].active = false;
            if (_.count > 0) _.count -= 1;
        }
    }

    fn fillSlot(*_, idx: usize, script_name: []const u8, cb: (*const fn(notice: *const IsolateLimitNotice, user_data: *!?))?, simple_cb: (*const fn(notice: *const IsolateLimitNotice))?, options: IsolateLimitListenerOptions) {
        _.seq_counter += 1;
        const s_len = @min(script_name.len, MAX_LIMIT_SCRIPT_NAME_LEN);
        @memcpy(_.entries[idx].script_name[0..s_len], script_name[0..s_len]);
        _.entries[idx].script_name_len = s_len;

        const o_len = @min(options.owner.len, MAX_LIMIT_OWNER_LEN);
        @memcpy(_.entries[idx].owner[0..o_len], options.owner[0..o_len]);
        _.entries[idx].owner_len = o_len;

        const t_len = @min(options.target_id.len, MAX_LIMIT_TARGET_LEN);
        @memcpy(_.entries[idx].target_id[0..t_len], options.target_id[0..t_len]);
        _.entries[idx].target_id_len = t_len;

        _.entries[idx].cb = cb;
        _.entries[idx].simple_cb = simple_cb;
        _.entries[idx].is_live_fn = options.is_live_fn;
        _.entries[idx].user_data = options.user_data;
        _.entries[idx].seq = _.seq_counter;
        _.entries[idx].active = true;
    }

    fn registerListener(*_, script_name: []const u8, cb: LimitListenerFn, options: IsolateLimitListenerOptions) {
        let i: usize = 0;
        while (i < MAX_LIMIT_LISTENERS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].script_name[0.._.entries[i].script_name_len], script_name)) {
                _.fillSlot(i, script_name, cb, null, options);
                return;
            }
        }

        if (_.count >= MAX_LIMIT_LISTENERS) {
            _.makeRoom();
        }

        i = 0;
        while (i < MAX_LIMIT_LISTENERS) : (i += 1) {
            if (!_.entries[i].active) {
                _.fillSlot(i, script_name, cb, null, options);
                _.count += 1;
                return;
            }
        }
    }

    fn registerSimpleListener(*_, script_name: []const u8, cb: SimpleLimitListenerFn, options: IsolateLimitListenerOptions) {
        let i: usize = 0;
        while (i < MAX_LIMIT_LISTENERS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].script_name[0.._.entries[i].script_name_len], script_name)) {
                _.fillSlot(i, script_name, null, cb, options);
                return;
            }
        }

        if (_.count >= MAX_LIMIT_LISTENERS) {
            _.makeRoom();
        }

        i = 0;
        while (i < MAX_LIMIT_LISTENERS) : (i += 1) {
            if (!_.entries[i].active) {
                _.fillSlot(i, script_name, null, cb, options);
                _.count += 1;
                return;
            }
        }
    }

    fn unregisterListener(*_, script_name: []const u8): bool {
        let i: usize = 0;
        while (i < MAX_LIMIT_LISTENERS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].script_name[0.._.entries[i].script_name_len], script_name)) {
                _.entries[i].active = false;
                _.entries[i].script_name_len = 0;
                if (_.count > 0) _.count -= 1;
                return true;
            }
        }
        return false;
    }

    fn unregisterForOwner(*_, owner: []const u8): usize {
        let removed: usize = 0;
        let i: usize = 0;
        while (i < MAX_LIMIT_LISTENERS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].owner[0.._.entries[i].owner_len], owner)) {
                _.entries[i].active = false;
                _.entries[i].script_name_len = 0;
                if (_.count > 0) _.count -= 1;
                removed += 1;
            }
        }
        return removed;
    }

    fn unregisterForTarget(*_, owner: []const u8, target_id: []const u8): usize {
        let removed: usize = 0;
        let i: usize = 0;
        while (i < MAX_LIMIT_LISTENERS) : (i += 1) {
            if (_.entries[i].active &&
                std.mem.eql(u8, _.entries[i].owner[0.._.entries[i].owner_len], owner) &&
                std.mem.eql(u8, _.entries[i].target_id[0.._.entries[i].target_id_len], target_id)) {
                _.entries[i].active = false;
                _.entries[i].script_name_len = 0;
                if (_.count > 0) _.count -= 1;
                removed += 1;
            }
        }
        return removed;
    }

    fn notifyLimit(*_, script_name: []const u8, notice: IsolateLimitNotice): bool {
        let i: usize = 0;
        while (i < MAX_LIMIT_LISTENERS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].script_name[0.._.entries[i].script_name_len], script_name)) {
                const cb = _.entries[i].cb;
                const simple_cb = _.entries[i].simple_cb;
                const udata = _.entries[i].user_data;
                _.entries[i].active = false;
                _.entries[i].script_name_len = 0;
                if (_.count > 0) _.count -= 1;

                if (cb_fn := cb) {
                    cb_fn(&notice, udata);
                } else if (scb_fn := simple_cb) {
                    scb_fn(&notice);
                }
                return true;
            }
        }
        return false;
    }

    fn hasListener(*_, script_name: []const u8): bool {
        let i: usize = 0;
        while (i < MAX_LIMIT_LISTENERS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].script_name[0.._.entries[i].script_name_len], script_name)) {
                return true;
            }
        }
        return false;
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_LIMIT_LISTENERS) : (i += 1) {
            _.entries[i].active = false;
            _.entries[i].script_name_len = 0;
        }
        _.count = 0;
    }

    fn size(): usize {
        return _.count;
    }
}

@tls let g_limit_registry: LimitRegistry = LimitRegistry.init();

pub fn getGlobalLimitRegistry(): *LimitRegistry {
    return &g_limit_registry;
}

pub fn registerIsolateLimitListener(script_name: []const u8, cb: LimitListenerFn, options: IsolateLimitListenerOptions) {
    g_limit_registry.registerListener(script_name, cb, options);
}

pub fn registerSimpleIsolateLimitListener(script_name: []const u8, cb: SimpleLimitListenerFn, options: IsolateLimitListenerOptions) {
    g_limit_registry.registerSimpleListener(script_name, cb, options);
}

pub fn unregisterIsolateLimitListener(script_name: []const u8) {
    * = g_limit_registry.unregisterListener(script_name);
}

pub fn unregisterIsolateLimitListenersFor(owner: []const u8) {
    * = g_limit_registry.unregisterForOwner(owner);
}

pub fn unregisterIsolateLimitListenersForTarget(owner: []const u8, target_id: []const u8) {
    * = g_limit_registry.unregisterForTarget(owner, target_id);
}

pub fn notifyIsolateLimit(script_name: []const u8, notice: IsolateLimitNotice) {
    * = g_limit_registry.notifyLimit(script_name, notice);
}
