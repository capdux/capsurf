// Migrated from vendor/kitesurf/packages/engine/src/cdp/outbound-filter-registry.ts
// Stores per-connection URL and resource-type filters used by page egress.

import { LinearRegex, compileLinearRegex } from "./linear_regex.cp";

pub enum OutboundResourceType: u8 {
    Document = 0,
    Stylesheet = 1,
    Image = 2,
    Media = 3,
    Font = 4,
    Script = 5,
    TextTrack = 6,
    XHR = 7,
    Fetch = 8,
    Prefetch = 9,
    EventSource = 10,
    WebSocket = 11,
    Manifest = 12,
    SignedExchange = 13,
    Ping = 14,
    CSPViolationReport = 15,
    Preflight = 16,
    Other = 17,

    fn asStr(): []const u8 {
        return switch (_) {
            .Document => "Document",
            .Stylesheet => "Stylesheet",
            .Image => "Image",
            .Media => "Media",
            .Font => "Font",
            .Script => "Script",
            .TextTrack => "TextTrack",
            .XHR => "XHR",
            .Fetch => "Fetch",
            .Prefetch => "Prefetch",
            .EventSource => "EventSource",
            .WebSocket => "WebSocket",
            .Manifest => "Manifest",
            .SignedExchange => "SignedExchange",
            .Ping => "Ping",
            .CSPViolationReport => "CSPViolationReport",
            .Preflight => "Preflight",
            .Other => "Other",
        };
    }
}

pub const MAX_PATTERN_LEN: usize = 500;
pub const MAX_PATTERNS_PER_SIDE: usize = 32;
pub const MAX_RESOURCE_TYPES: usize = 32;
pub const MAX_RESOURCE_TYPE_LEN: usize = 64;
pub const MAX_RT_STORAGE: usize = MAX_RESOURCE_TYPES * MAX_RESOURCE_TYPE_LEN;
pub const MAX_REGISTRY_ENTRIES: usize = 64;

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn formatUsize(buf: *[20]u8, val: usize): []const u8 {
    if (val == 0) {
        buf[0] = '0';
        return buf[0..1];
    }
    let tmp: [20]u8 = [0; 20];
    let v = val;
    let len: usize = 0;
    while (v > 0) {
        tmp[len] = @as(u8, @intCast(v % 10)) + '0';
        v /= 10;
        len += 1;
    }
    let i: usize = 0;
    while (i < len) : (i += 1) {
        buf[i] = tmp[len - 1 - i];
    }
    return buf[0..len];
}

pub struct OutboundFilters {
    allow_patterns: [MAX_PATTERNS_PER_SIDE]LinearRegex = [LinearRegex{}; MAX_PATTERNS_PER_SIDE],
    allow_pattern_count: usize = 0,
    reject_patterns: [MAX_PATTERNS_PER_SIDE]LinearRegex = [LinearRegex{}; MAX_PATTERNS_PER_SIDE],
    reject_pattern_count: usize = 0,
    allow_resource_types: [MAX_RT_STORAGE]u8 = [0; MAX_RT_STORAGE],
    allow_resource_type_lens: [MAX_RESOURCE_TYPES]usize = [0; MAX_RESOURCE_TYPES],
    allow_resource_type_count: usize = 0,
    reject_resource_types: [MAX_RT_STORAGE]u8 = [0; MAX_RT_STORAGE],
    reject_resource_type_lens: [MAX_RESOURCE_TYPES]usize = [0; MAX_RESOURCE_TYPES],
    reject_resource_type_count: usize = 0,

    fn allowResourceType(idx: usize): []const u8 {
        if (idx >= _.allow_resource_type_count) return "";
        const off = idx * MAX_RESOURCE_TYPE_LEN;
        const len = _.allow_resource_type_lens[idx];
        return _.allow_resource_types[off..off + len];
    }

    fn rejectResourceType(idx: usize): []const u8 {
        if (idx >= _.reject_resource_type_count) return "";
        const off = idx * MAX_RESOURCE_TYPE_LEN;
        const len = _.reject_resource_type_lens[idx];
        return _.reject_resource_types[off..off + len];
    }

    fn deinit(*_) {
        let i: usize = 0;
        while (i < _.allow_pattern_count) : (i += 1) {
            _.allow_patterns[i].deinit();
        }
        _.allow_pattern_count = 0;
        i = 0;
        while (i < _.reject_pattern_count) : (i += 1) {
            _.reject_patterns[i].deinit();
        }
        _.reject_pattern_count = 0;
    }
}

pub struct ShouldBlockResult {
    blocked: bool = false,
    reason_buf: [256]u8 = [0; 256],
    reason_len: usize = 0,

    fn reason(*_): []const u8 {
        return _.reason_buf[0.._.reason_len];
    }

    fn setReason(*_, str: []const u8) {
        const len = if (str.len < 256) str.len else 255;
        @memcpy(_.reason_buf[0..len], str[0..len]);
        _.reason_len = len;
    }

    fn setReason3(*_, a: []const u8, b: []const u8, c: []const u8) {
        let off: usize = 0;
        const a_len = if (a.len <= 256 - off) a.len else (256 - off);
        if (a_len > 0) {
            @memcpy(_.reason_buf[off..off + a_len], a[0..a_len]);
            off += a_len;
        }
        const b_len = if (b.len <= 256 - off) b.len else (256 - off);
        if (b_len > 0) {
            @memcpy(_.reason_buf[off..off + b_len], b[0..b_len]);
            off += b_len;
        }
        const c_len = if (c.len <= 256 - off) c.len else (256 - off);
        if (c_len > 0) {
            @memcpy(_.reason_buf[off..off + c_len], c[0..c_len]);
            off += c_len;
        }
        _.reason_len = off;
    }
}

pub fn shouldBlock(
    filters: *OutboundFilters,
    url: []const u8,
    resource_type: []const u8,
): ShouldBlockResult {
    if (filters.reject_pattern_count > 0) {
        let i: usize = 0;
        while (i < filters.reject_pattern_count) : (i += 1) {
            if (filters.reject_patterns[i].@"test"(url)) {
                let res = ShouldBlockResult{ .blocked = true };
                res.setReason3("rejectRequestPattern matched (", filters.reject_patterns[i].source, ")");
                return res;
            }
        }
    }

    if (filters.reject_resource_type_count > 0) {
        let i: usize = 0;
        while (i < filters.reject_resource_type_count) : (i += 1) {
            const rt = filters.rejectResourceType(i);
            if (eqlIgnoreCase(rt, resource_type)) {
                let res = ShouldBlockResult{ .blocked = true };
                res.setReason3("rejectResourceTypes matched (", resource_type, ")");
                return res;
            }
        }
    }

    if (filters.allow_pattern_count > 0) {
        let ok = false;
        let i: usize = 0;
        while (i < filters.allow_pattern_count) : (i += 1) {
            if (filters.allow_patterns[i].@"test"(url)) {
                ok = true;
                break;
            }
        }
        if (!ok) {
            let res = ShouldBlockResult{ .blocked = true };
            res.setReason("allowRequestPattern did not match");
            return res;
        }
    }

    if (filters.allow_resource_type_count > 0) {
        let ok = false;
        let i: usize = 0;
        while (i < filters.allow_resource_type_count) : (i += 1) {
            const rt = filters.allowResourceType(i);
            if (eqlIgnoreCase(rt, resource_type)) {
                ok = true;
                break;
            }
        }
        if (!ok) {
            let res = ShouldBlockResult{ .blocked = true };
            res.setReason3("allowResourceTypes did not match (", resource_type, ")");
            return res;
        }
    }

    return ShouldBlockResult{ .blocked = false };
}

pub struct CompileFiltersResult {
    ok: bool = false,
    filters: OutboundFilters = OutboundFilters{},
    error_msg_buf: [256]u8 = [0; 256],
    error_msg_len: usize = 0,

    fn error_msg(*_): []const u8 {
        return _.error_msg_buf[0.._.error_msg_len];
    }

    fn @"error"(*_): []const u8 {
        return _.error_msg();
    }

    fn setError(*_, msg: []const u8) {
        const len = if (msg.len < 256) msg.len else 255;
        @memcpy(_.error_msg_buf[0..len], msg[0..len]);
        _.error_msg_len = len;
        _.ok = false;
    }

    fn setError4(*_, a: []const u8, b: []const u8, c: []const u8, d: []const u8) {
        let off: usize = 0;
        const a_len = if (a.len <= 256 - off) a.len else (256 - off);
        if (a_len > 0) {
            @memcpy(_.error_msg_buf[off..off + a_len], a[0..a_len]);
            off += a_len;
        }
        const b_len = if (b.len <= 256 - off) b.len else (256 - off);
        if (b_len > 0) {
            @memcpy(_.error_msg_buf[off..off + b_len], b[0..b_len]);
            off += b_len;
        }
        const c_len = if (c.len <= 256 - off) c.len else (256 - off);
        if (c_len > 0) {
            @memcpy(_.error_msg_buf[off..off + c_len], c[0..c_len]);
            off += c_len;
        }
        const d_len = if (d.len <= 256 - off) d.len else (256 - off);
        if (d_len > 0) {
            @memcpy(_.error_msg_buf[off..off + d_len], d[0..d_len]);
            off += d_len;
        }
        _.error_msg_len = off;
        _.ok = false;
    }
}

pub fn compileOutboundFilters(
    allow_request_pattern: []const []const u8,
    reject_request_pattern: []const []const u8,
    allow_resource_types: []const []const u8,
    reject_resource_types: []const []const u8,
): CompileFiltersResult {
    let mut_res = CompileFiltersResult{};
    let fmt_buf: [20]u8 = [0; 20];

    if (allow_request_pattern.len > MAX_PATTERNS_PER_SIDE) {
        const num_str = formatUsize(&fmt_buf, allow_request_pattern.len);
        mut_res.setError4("too many allowRequestPattern entries (", num_str, " > 32)", "");
        return mut_res;
    }

    if (reject_request_pattern.len > MAX_PATTERNS_PER_SIDE) {
        const num_str = formatUsize(&fmt_buf, reject_request_pattern.len);
        mut_res.setError4("too many rejectRequestPattern entries (", num_str, " > 32)", "");
        return mut_res;
    }

    let i: usize = 0;
    while (i < allow_request_pattern.len) : (i += 1) {
        if (allow_request_pattern[i].len > MAX_PATTERN_LEN) {
            const num_str = formatUsize(&fmt_buf, allow_request_pattern[i].len);
            mut_res.setError4("allowRequestPattern regex too long (", num_str, " > 500 chars)", "");
            return mut_res;
        }
    }

    i = 0;
    while (i < reject_request_pattern.len) : (i += 1) {
        if (reject_request_pattern[i].len > MAX_PATTERN_LEN) {
            const num_str = formatUsize(&fmt_buf, reject_request_pattern[i].len);
            mut_res.setError4("rejectRequestPattern regex too long (", num_str, " > 500 chars)", "");
            return mut_res;
        }
    }

    let filters = OutboundFilters{};

    i = 0;
    while (i < allow_request_pattern.len) : (i += 1) {
        const comp = compileLinearRegex(allow_request_pattern[i]);
        if (!comp.ok) {
            filters.deinit();
            mut_res.setError4("invalid regex: ", comp.error_msg, "", "");
            return mut_res;
        }
        filters.allow_patterns[filters.allow_pattern_count] = comp.regex;
        filters.allow_pattern_count += 1;
    }

    i = 0;
    while (i < reject_request_pattern.len) : (i += 1) {
        const comp = compileLinearRegex(reject_request_pattern[i]);
        if (!comp.ok) {
            filters.deinit();
            mut_res.setError4("invalid regex: ", comp.error_msg, "", "");
            return mut_res;
        }
        filters.reject_patterns[filters.reject_pattern_count] = comp.regex;
        filters.reject_pattern_count += 1;
    }

    i = 0;
    while (i < allow_resource_types.len && filters.allow_resource_type_count < MAX_RESOURCE_TYPES) : (i += 1) {
        const src_rt = allow_resource_types[i];
        const copy_len = if (src_rt.len < MAX_RESOURCE_TYPE_LEN) src_rt.len else (MAX_RESOURCE_TYPE_LEN - 1);
        const idx = filters.allow_resource_type_count;
        const off = idx * MAX_RESOURCE_TYPE_LEN;
        let c_i: usize = 0;
        while (c_i < copy_len) : (c_i += 1) {
            filters.allow_resource_types[off + c_i] = toLowerAscii(src_rt[c_i]);
        }
        filters.allow_resource_type_lens[idx] = copy_len;
        filters.allow_resource_type_count += 1;
    }

    i = 0;
    while (i < reject_resource_types.len && filters.reject_resource_type_count < MAX_RESOURCE_TYPES) : (i += 1) {
        const src_rt = reject_resource_types[i];
        const copy_len = if (src_rt.len < MAX_RESOURCE_TYPE_LEN) src_rt.len else (MAX_RESOURCE_TYPE_LEN - 1);
        const idx = filters.reject_resource_type_count;
        const off = idx * MAX_RESOURCE_TYPE_LEN;
        let c_i: usize = 0;
        while (c_i < copy_len) : (c_i += 1) {
            filters.reject_resource_types[off + c_i] = toLowerAscii(src_rt[c_i]);
        }
        filters.reject_resource_type_lens[idx] = copy_len;
        filters.reject_resource_type_count += 1;
    }

    mut_res.ok = true;
    mut_res.filters = filters;
    return mut_res;
}

pub struct RegistryEntry {
    connection_id_buf: [128]u8 = [0; 128],
    connection_id_len: usize = 0,
    filters: OutboundFilters = OutboundFilters{},
    active: bool = false,

    fn connectionId(*_): []const u8 {
        return _.connection_id_buf[0.._.connection_id_len];
    }
}

pub struct OutboundFilterRegistry {
    entries: [MAX_REGISTRY_ENTRIES]RegistryEntry = [RegistryEntry{}; MAX_REGISTRY_ENTRIES],
    count: usize = 0,

    static fn init(): OutboundFilterRegistry {
        return OutboundFilterRegistry{};
    }

    fn register(*_, connection_id: []const u8, filters: OutboundFilters) {
        let i: usize = 0;
        while (i < MAX_REGISTRY_ENTRIES) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].connectionId(), connection_id)) {
                _.entries[i].filters.deinit();
                _.entries[i].filters = filters;
                return;
            }
        }

        i = 0;
        while (i < MAX_REGISTRY_ENTRIES) : (i += 1) {
            if (!_.entries[i].active) {
                const id_len = if (connection_id.len < 128) connection_id.len else 127;
                @memcpy(_.entries[i].connection_id_buf[0..id_len], connection_id[0..id_len]);
                _.entries[i].connection_id_len = id_len;
                _.entries[i].filters = filters;
                _.entries[i].active = true;
                _.count += 1;
                return;
            }
        }
    }

    fn unregister(*_, connection_id: []const u8) {
        let i: usize = 0;
        while (i < MAX_REGISTRY_ENTRIES) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].connectionId(), connection_id)) {
                _.entries[i].filters.deinit();
                _.entries[i].active = false;
                _.entries[i].connection_id_len = 0;
                if (_.count > 0) _.count -= 1;
                return;
            }
        }
    }

    fn get(*_, connection_id: []const u8): (*OutboundFilters)? {
        let i: usize = 0;
        while (i < MAX_REGISTRY_ENTRIES) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].connectionId(), connection_id)) {
                return &_.entries[i].filters;
            }
        }
        return null;
    }
}

pub threadlocal let g_registry: OutboundFilterRegistry = OutboundFilterRegistry{};

pub fn registerOutboundFilters(connection_id: []const u8, filters: OutboundFilters) {
    g_registry.register(connection_id, filters);
}

pub fn unregisterOutboundFilters(connection_id: []const u8) {
    g_registry.unregister(connection_id);
}

pub fn getOutboundFilters(connection_id: []const u8): (*OutboundFilters)? {
    return g_registry.get(connection_id);
}
