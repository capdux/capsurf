// Migrated from vendor/kitesurf/packages/engine/src/eval/learned-scripts.ts
// src/eval/learned_scripts.cp
// Records page-inserted script outcomes per browser context and origin for reuse.

pub const MIN_LEARNED_BYTES: usize = 8192;
pub const MAX_URLS_PER_ORIGIN: usize = 24;
pub const MAX_ORIGINS: usize = 32;
pub const MAX_IDLE_NAVIGATES: u32 = 3;
pub const MAX_URL_LEN: usize = 512;
pub const MAX_KEY_LEN: usize = 256;

pub enum InsertedRun {
    boa,
    baked,
    stale,

    fn asStr(): []const u8 {
        return switch (_) {
            .boa => "boa",
            .baked => "baked",
            .stale => "stale",
        };
    }
}

pub struct UrlEntry {
    buf: [MAX_URL_LEN]u8 = [0; MAX_URL_LEN],
    len: usize = 0,
    idle: u32 = 0,

    fn get(*_): []const u8 {
        return _.buf[0.._.len];
    }

    fn set(*_, s: []const u8, idle: u32) {
        const copy_len = @min(s.len, MAX_URL_LEN);
        @memcpy(_.buf[0..copy_len], s[0..copy_len]);
        _.len = copy_len;
        _.idle = idle;
    }

    fn matches(*_, s: []const u8): bool {
        if (_.len != s.len) return false;
        return std.mem.eql(u8, _.buf[0.._.len], s);
    }
}

pub struct UrlKey {
    buf: [MAX_URL_LEN]u8 = [0; MAX_URL_LEN],
    len: usize = 0,

    fn get(*_): []const u8 {
        return _.buf[0.._.len];
    }

    fn set(*_, s: []const u8) {
        const copy_len = @min(s.len, MAX_URL_LEN);
        @memcpy(_.buf[0..copy_len], s[0..copy_len]);
        _.len = copy_len;
    }

    fn matches(*_, s: []const u8): bool {
        if (_.len != s.len) return false;
        return std.mem.eql(u8, _.buf[0.._.len], s);
    }
}

pub struct UrlMap {
    entries: [MAX_URLS_PER_ORIGIN]UrlEntry = [UrlEntry{}; MAX_URLS_PER_ORIGIN],
    count: usize = 0,

    fn has(*_, s: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].matches(s)) return true;
        }
        return false;
    }

    fn getIdle(*_, s: []const u8): u32? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].matches(s)) return _.entries[i].idle;
        }
        return null;
    }

    fn delete(*_, s: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].matches(s)) {
                let j = i;
                while (j + 1 < _.count) : (j += 1) {
                    _.entries[j] = _.entries[j + 1];
                }
                _.count -= 1;
                return true;
            }
        }
        return false;
    }

    fn touch(*_, s: []const u8, idle: u32) {
        * = _.delete(s);
        if (_.count >= MAX_URLS_PER_ORIGIN) {
            _.trimOldest(MAX_URLS_PER_ORIGIN - 1);
        }
        _.entries[_.count].set(s, idle);
        _.count += 1;
    }

    fn trimOldest(*_, max: usize) {
        while (_.count > max && _.count > 0) {
            let j: usize = 0;
            while (j + 1 < _.count) : (j += 1) {
                _.entries[j] = _.entries[j + 1];
            }
            _.count -= 1;
        }
    }

    fn removeAt(*_, idx: usize) {
        if (idx >= _.count) return;
        let j = idx;
        while (j + 1 < _.count) : (j += 1) {
            _.entries[j] = _.entries[j + 1];
        }
        _.count -= 1;
    }
}

pub struct UrlSet {
    entries: [MAX_URLS_PER_ORIGIN]UrlKey = [UrlKey{}; MAX_URLS_PER_ORIGIN],
    count: usize = 0,

    fn has(*_, s: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].matches(s)) return true;
        }
        return false;
    }

    fn delete(*_, s: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].matches(s)) {
                let j = i;
                while (j + 1 < _.count) : (j += 1) {
                    _.entries[j] = _.entries[j + 1];
                }
                _.count -= 1;
                return true;
            }
        }
        return false;
    }

    fn add(*_, s: []const u8) {
        * = _.delete(s);
        if (_.count >= MAX_URLS_PER_ORIGIN) {
            _.trimOldest(MAX_URLS_PER_ORIGIN - 1);
        }
        _.entries[_.count].set(s);
        _.count += 1;
    }

    fn trimOldest(*_, max: usize) {
        while (_.count > max && _.count > 0) {
            let j: usize = 0;
            while (j + 1 < _.count) : (j += 1) {
                _.entries[j] = _.entries[j + 1];
            }
            _.count -= 1;
        }
    }
}

pub struct OriginScripts {
    key_buf: [MAX_KEY_LEN]u8 = [0; MAX_KEY_LEN],
    key_len: usize = 0,
    urls: UrlMap = UrlMap{},
    refused: UrlSet = UrlSet{},
    queried: UrlSet = UrlSet{},

    fn getKey(*_): []const u8 {
        return _.key_buf[0.._.key_len];
    }

    fn setKey(*_, key: []const u8) {
        const copy_len = @min(key.len, MAX_KEY_LEN);
        @memcpy(_.key_buf[0..copy_len], key[0..copy_len]);
        _.key_len = copy_len;
    }

    fn matchesKey(*_, key: []const u8): bool {
        if (_.key_len != key.len) return false;
        return std.mem.eql(u8, _.key_buf[0.._.key_len], key);
    }

    fn reset(*_) {
        _.key_len = 0;
        _.urls.count = 0;
        _.refused.count = 0;
        _.queried.count = 0;
    }
}

pub fn learnedOriginOf(url: []const u8): ([]const u8)? {
    let scheme_end: usize = 0;
    if (std.mem.startsWith(u8, url, "http://")) {
        scheme_end = 7;
    } else if (std.mem.startsWith(u8, url, "https://")) {
        scheme_end = 8;
    } else {
        return null;
    }

    const rem = url[scheme_end..];
    let auth_end: usize = rem.len;
    let i: usize = 0;
    while (i < rem.len) : (i += 1) {
        const c = rem[i];
        if (c == '/' || c == '?' || c == '#') {
            auth_end = i;
            break;
        }
    }
    if (auth_end == 0) return null;
    const authority = rem[0..auth_end];
    if (std.mem.indexOfScalar(u8, authority, '@') != null) {
        return null;
    }
    return url[0 .. scheme_end + auth_end];
}

pub fn hasQuery(url: []const u8): bool {
    const q_idx_opt = std.mem.indexOfScalar(u8, url, '?');
    if (q_idx_opt == null) return false;
    const q_idx = q_idx_opt.?;

    const hash_idx_opt = std.mem.indexOfScalar(u8, url, '#');
    const end_idx = if (hash_idx_opt != null) hash_idx_opt.? else url.len;

    if (q_idx >= end_idx) return false;
    return (q_idx + 1 < end_idx);
}

pub fn storeKey(buf: []u8, page_url: []const u8, context_id: []const u8): ([]const u8)? {
    if (origin := learnedOriginOf(page_url)) {
        return std.fmt.bufPrint(buf, "{s}|{s}", .{ context_id, origin }) catch null;
    }
    return null;
}

pub struct LearnedScriptsStore {
    origins: [MAX_ORIGINS]OriginScripts = [OriginScripts{}; MAX_ORIGINS],
    origin_count: usize = 0,
    query_result: [MAX_URLS_PER_ORIGIN][]const u8 = [""; MAX_URLS_PER_ORIGIN],

    static fn init(): LearnedScriptsStore {
        return LearnedScriptsStore{};
    }

    fn findIndex(*_, key: []const u8): usize? {
        let i: usize = 0;
        while (i < _.origin_count) : (i += 1) {
            if (_.origins[i].matchesKey(key)) return i;
        }
        return null;
    }

    fn getOrigin(*_, key: []const u8): (*OriginScripts)? {
        if (idx := _.findIndex(key)) {
            return &_.origins[idx];
        }
        return null;
    }

    fn touchOrigin(*_, key: []const u8): (*OriginScripts)? {
        if (idx := _.findIndex(key)) {
            if (idx == _.origin_count - 1) {
                return &_.origins[idx];
            }
            const saved = _.origins[idx];
            let j = idx;
            while (j + 1 < _.origin_count) : (j += 1) {
                _.origins[j] = _.origins[j + 1];
            }
            _.origins[_.origin_count - 1] = saved;
            return &_.origins[_.origin_count - 1];
        }

        if (_.origin_count >= MAX_ORIGINS) {
            _.trimOldestOrigins(MAX_ORIGINS - 1);
        }

        const slot = _.origin_count;
        _.origins[slot].reset();
        _.origins[slot].setKey(key);
        _.origin_count += 1;
        return &_.origins[slot];
    }

    fn trimOldestOrigins(*_, max: usize) {
        while (_.origin_count > max && _.origin_count > 0) {
            let j: usize = 0;
            while (j + 1 < _.origin_count) : (j += 1) {
                _.origins[j] = _.origins[j + 1];
            }
            _.origin_count -= 1;
        }
    }

    fn recordInsertedScript(*_, page_url: []const u8, script_url: []const u8, bytes: usize, run: InsertedRun, context_id: []const u8) {
        let key_buf: [256]u8 = [0; 256];
        const key_opt = storeKey(&key_buf, page_url, context_id);
        if (key_opt == null) return;
        const key = key_opt.?;

        if (learnedOriginOf(script_url) == null) return;

        let entry = _.getOrigin(key);
        if (run == InsertedRun.stale) {
            if (e := entry) {
                * = e.urls.delete(script_url);
                * = e.refused.delete(script_url);
                e.refused.add(script_url);
                e.refused.trimOldest(MAX_URLS_PER_ORIGIN);
            }
            return;
        }

        if (run == InsertedRun.baked) {
            if (e := entry) {
                if (!e.urls.has(script_url)) return;
            } else {
                return;
            }
        } else {
            if (bytes < MIN_LEARNED_BYTES) return;
            if (e := entry) {
                if (e.refused.has(script_url)) return;
            }
        }

        const touched_entry = _.touchOrigin(key);
        if (touched_entry == null) return;
        let valid_entry = touched_entry.?;
        _.trimOldestOrigins(MAX_ORIGINS);

        if (run == InsertedRun.boa && hasQuery(script_url) && !valid_entry.urls.has(script_url)) {
            const was_deleted = valid_entry.queried.delete(script_url);
            if (!was_deleted) {
                valid_entry.queried.add(script_url);
                valid_entry.queried.trimOldest(MAX_URLS_PER_ORIGIN);
                return;
            }
        }

        valid_entry.urls.touch(script_url, 0);
        valid_entry.urls.trimOldest(MAX_URLS_PER_ORIGIN);
    }

    fn forgetLearnedScript(*_, page_url: []const u8, script_url: []const u8, context_id: []const u8) {
        let key_buf: [256]u8 = [0; 256];
        const key_opt = storeKey(&key_buf, page_url, context_id);
        if (key_opt == null) return;
        const key = key_opt.?;

        if (entry := _.getOrigin(key)) {
            * = entry.urls.delete(script_url);
        }
    }

    fn clearLearnedScripts(*_, context_id: []const u8) {
        let prefix_buf: [256]u8 = [0; 256];
        const prefix = std.fmt.bufPrint(&prefix_buf, "{s}|", .{context_id}) catch return;
        let i: usize = 0;
        while (i < _.origin_count) {
            const k = _.origins[i].getKey();
            if (std.mem.startsWith(u8, k, prefix)) {
                let j = i;
                while (j + 1 < _.origin_count) : (j += 1) {
                    _.origins[j] = _.origins[j + 1];
                }
                _.origin_count -= 1;
            } else {
                i += 1;
            }
        }
    }

    fn learnedScriptsFor(*_, page_url: []const u8, context_id: []const u8): [][]const u8 {
        let key_buf: [256]u8 = [0; 256];
        const key_opt = storeKey(&key_buf, page_url, context_id);
        if (key_opt == null) return [];
        const key = key_opt.?;

        if (_.getOrigin(key) == null) {
            return [];
        }

        const touched = _.touchOrigin(key);
        if (touched == null) return [];
        let entry = touched.?;

        let out_count: usize = 0;
        let i: usize = 0;
        while (i < entry.urls.count) {
            if (entry.urls.entries[i].idle >= MAX_IDLE_NAVIGATES) {
                entry.urls.removeAt(i);
            } else {
                entry.urls.entries[i].idle += 1;
                if (out_count < MAX_URLS_PER_ORIGIN) {
                    _.query_result[out_count] = entry.urls.entries[i].get();
                    out_count += 1;
                }
                i += 1;
            }
        }
        return _.query_result[0..out_count];
    }
}

pub fn recordInsertedScript(store: *LearnedScriptsStore, page_url: []const u8, script_url: []const u8, bytes: usize, run: InsertedRun, context_id: []const u8) {
    store.recordInsertedScript(page_url, script_url, bytes, run, context_id);
}

pub fn forgetLearnedScript(store: *LearnedScriptsStore, page_url: []const u8, script_url: []const u8, context_id: []const u8) {
    store.forgetLearnedScript(page_url, script_url, context_id);
}

pub fn clearLearnedScripts(store: *LearnedScriptsStore, context_id: []const u8) {
    store.clearLearnedScripts(context_id);
}

pub fn learnedScriptsFor(store: *LearnedScriptsStore, page_url: []const u8, context_id: []const u8): [][]const u8 {
    return store.learnedScriptsFor(page_url, context_id);
}
