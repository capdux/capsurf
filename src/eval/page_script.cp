// Migrated from vendor/kitesurf/packages/engine/src/eval/page-script.ts
// Loads each page generation's PageScript isolate and wires its RPC, egress, and tails.

pub const PAGE_SCRIPT_COMPAT_DATE: []const u8 = "2026-09-11";
pub const PAGE_SCRIPT_SUBREQUEST_CAP: u32 = 10000;
pub const PAGE_SCRIPT_CPU_TIME_MS_CAP: u32 = 300000;
pub const PAGE_SCRIPT_FRAME_ID_KEY_PREFIX_CHARS: usize = 8;

pub const PAGE_SCRIPT_COMPAT_FLAGS: [3][]const u8 = [
    "new_module_registry",
    "no_nodejs_compat",
    "no_nodejs_compat_v2",
];

pub struct PageIsolateCaps {
    cpu_ms: u32 = 50,
    subrequests: u32 = 1000,
    memory_mb: u32 = 128,
}

pub struct PageScriptKey {
    origin: []const u8 = "",
    key: []const u8 = "",
    frame_id: []const u8 = "",
    loader_id: []const u8 = "",

    fn getCacheKey(out_buf: []u8): []const u8 {
        return pageScriptCacheKey(_, out_buf);
    }
}

pub fn pageIsolateCaps(cpu_ms: u32, subrequests: u32, memory_mb: u32): PageIsolateCaps {
    return PageIsolateCaps{
        cpu_ms: if (cpu_ms > 0) cpu_ms else 50,
        subrequests: if (subrequests > 0) subrequests else 1000,
        memory_mb: if (memory_mb > 0) memory_mb else 128,
    };
}

pub fn pageScriptCacheKey(k: PageScriptKey, out_buf: []u8): []const u8 {
    const frame_prefix_len: usize = if (k.frame_id.len > PAGE_SCRIPT_FRAME_ID_KEY_PREFIX_CHARS)
        PAGE_SCRIPT_FRAME_ID_KEY_PREFIX_CHARS
    else
        k.frame_id.len;

    if (k.frame_id.len > 0) {
        if (k.origin.len > 0) {
            return std.fmt.bufPrint(
                out_buf,
                "page-{s}-{s}-oopif-{s}",
                .{ k.origin, k.key, k.frame_id[0..frame_prefix_len] },
            ) catch "";
        } else {
            return std.fmt.bufPrint(
                out_buf,
                "page-{s}-oopif-{s}",
                .{ k.key, k.frame_id[0..frame_prefix_len] },
            ) catch "";
        }
    } else {
        if (k.origin.len > 0) {
            return std.fmt.bufPrint(
                out_buf,
                "page-{s}-{s}",
                .{ k.origin, k.key },
            ) catch "";
        } else {
            return std.fmt.bufPrint(
                out_buf,
                "page-{s}",
                .{ k.key },
            ) catch "";
        }
    }
}

pub fn hostnameOf(url: []const u8): []const u8 {
    let rest: []const u8 = "";
    if (p_idx := std.mem.indexOf(u8, url, "://")) {
        rest = url[p_idx + 3 ..];
    } else {
        rest = url;
    }

    let end: usize = 0;
    while (end < rest.len) : (end += 1) {
        const c = rest[end];
        if (c == '/' || c == '?' || c == '#') break;
    }
    let authority = rest[0..end];

    if (at_idx := std.mem.indexOfScalar(u8, authority, '@')) {
        authority = authority[at_idx + 1 ..];
    }

    let hostname = authority;
    if (colon_idx := std.mem.lastIndexOfScalar(u8, authority, ':')) {
        if (!std.mem.endsWith(u8, authority, "]")) {
            hostname = authority[0..colon_idx];
        }
    }

    if (hostname.len > 0 && hostname[hostname.len - 1] == '.') {
        hostname = hostname[0 .. hostname.len - 1];
    }
    return hostname;
}

pub fn originOf(url: []const u8): []const u8 {
    let proto_len: usize = 0;
    if (std.mem.startsWith(u8, url, "http://")) {
        proto_len = 7;
    } else if (std.mem.startsWith(u8, url, "https://")) {
        proto_len = 8;
    } else {
        return "";
    }

    const rest = url[proto_len..];
    let end: usize = 0;
    while (end < rest.len) : (end += 1) {
        const c = rest[end];
        if (c == '/' || c == '?' || c == '#') break;
    }
    const authority = rest[0..end];
    if (authority.len == 0) return "";

    if (std.mem.indexOfScalar(u8, authority, '@') != null) {
        return "";
    }

    return url[0 .. proto_len + end];
}

pub fn formatRemoteObject(val: []const u8, out_buf: []u8): []const u8 {
    if (val.len >= 2 && val[0] == '{' && val[val.len - 1] == '}') {
        if (val.len > out_buf.len) return "";
        @memcpy(out_buf[0..val.len], val);
        return out_buf[0..val.len];
    }
    return std.fmt.bufPrint(out_buf, "{{\"type\":\"string\",\"value\":\"{s}\"}}", .{val}) catch "";
}

pub fn formatConsoleLogEntry(level: []const u8, text: []const u8, out_buf: []u8): []const u8 {
    const lvl = if (level.len == 0) "log" else level;
    return std.fmt.bufPrint(
        out_buf,
        "{{\"type\":\"{s}\",\"args\":[{{\"type\":\"string\",\"value\":\"{s}\"}}],\"executionContextId\":1}}",
        .{ lvl, text },
    ) catch "";
}
