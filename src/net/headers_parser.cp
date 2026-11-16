// src/net/headers_parser.cp
// HeadersInit specification validation and Origin computation per WHATWG Fetch & HTML specs.

import { isForbiddenRequestHeaderName, isForbiddenResponseHeader } from "./forbidden_headers.cp";

pub struct HeaderEntry {
    name: []const u8 = "",
    value: []const u8 = "",
}

pub fn isHttpTokenChar(c: u8): bool {
    if (std.ascii.isAlphanumeric(c)) return true;
    return switch (c) {
        '!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~' => true,
        else => false,
    };
}

pub fn isValidHeaderName(name: []const u8): bool {
    if (name.len == 0) return false;
    for (c of name) {
        if (!isHttpTokenChar(c)) return false;
    }
    return true;
}

pub fn isValidHeaderValue(value: []const u8): bool {
    for (c of value) {
        if (c == 0x00 || c == 0x0A || c == 0x0D) return false;
    }
    return true;
}

pub fn stripHttpWhitespace(s: []const u8): []const u8 {
    return std.mem.trim(u8, s, " \t\r\n");
}

pub struct HeadersValidator {
    static fn isValidName(name: []const u8): bool {
        return isValidHeaderName(name);
    }

    static fn isValidValue(value: []const u8): bool {
        return isValidHeaderValue(value);
    }

    static fn validateHeader(name: []const u8, value: []const u8): bool {
        return isValidHeaderName(name) && isValidHeaderValue(value);
    }

    static fn isForbiddenRequest(name: []const u8): bool {
        return isForbiddenRequestHeaderName(name);
    }

    static fn isForbiddenResponse(name: []const u8): bool {
        return isForbiddenResponseHeader(name);
    }
}

pub struct HeadersParser {
    static fn parseRaw(raw: []const u8, out_entries: []HeaderEntry): usize {
        let count: usize = 0;
        let line_it = std.mem.splitSequence(u8, raw, "\r\n");
        while (line := line_it.next()) {
            if (line.len == 0) continue;
            if (colon := std.mem.indexOfScalar(u8, line, ':')) {
                const name = std.mem.trim(u8, line[0..colon], " \t");
                const val = std.mem.trim(u8, line[colon + 1 ..], " \t");
                if (count < out_entries.len) {
                    out_entries[count] = HeaderEntry{
                        name: name,
                        value: val,
                    };
                    count += 1;
                }
            }
        }
        return count;
    }
}

pub struct FetchOriginPolicy {
    /// Computes the origin of a URL per WHATWG Fetch and HTML specification:
    /// - If page_url is "about:blank" or starts with "data:" or is empty, returns "null";
    /// - Otherwise parses scheme, host and port (e.g. "https://example.com:8080").
    static fn computeOrigin(page_url: []const u8): []const u8 {
        if (page_url.len == 0) return "null";
        if (std.mem.eql(u8, page_url, "about:blank")) return "null";
        if (std.mem.startsWith(u8, page_url, "about:blank?") || std.mem.startsWith(u8, page_url, "about:blank#")) return "null";
        if (std.mem.eql(u8, page_url, "about:srcdoc")) return "null";
        if (std.mem.startsWith(u8, page_url, "data:")) return "null";
        if (std.mem.startsWith(u8, page_url, "javascript:")) return "null";

        const sep = "://";
        const sep_opt = std.mem.indexOf(u8, page_url, sep);
        if (sep_opt == null) return "null";
        const proto_end = sep_opt.?;
        if (proto_end == 0) return "null";

        const scheme = page_url[0..proto_end];
        const is_http = std.ascii.eqlIgnoreCase(scheme, "http");
        const is_https = std.ascii.eqlIgnoreCase(scheme, "https");
        const is_ws = std.ascii.eqlIgnoreCase(scheme, "ws");
        const is_wss = std.ascii.eqlIgnoreCase(scheme, "wss");
        if (!is_http && !is_https && !is_ws && !is_wss) {
            return "null";
        }

        const after_sep = page_url[proto_end + sep.len ..];
        let auth_end: usize = 0;
        while (auth_end < after_sep.len) : (auth_end += 1) {
            const c = after_sep[auth_end];
            if (c == '/' || c == '?' || c == '#') break;
        }

        if (auth_end == 0) return "null";
        const authority = after_sep[0..auth_end];

        // IPv6 host literal: [2001:db8::1] or [2001:db8::1]:8080
        if (authority[0] == '[') {
            if (rb := std.mem.indexOfScalar(u8, authority, ']')) {
                if (rb + 1 < authority.len && authority[rb + 1] == ':') {
                    const port_str = authority[rb + 2 ..];
                    const port_num = std.fmt.parseInt(u16, port_str, 10) catch 0;
                    if (((is_http || is_ws) && port_num == 80) || ((is_https || is_wss) && port_num == 443)) {
                        return page_url[0 .. proto_end + sep.len + rb + 1];
                    }
                }
            }
            return page_url[0 .. proto_end + sep.len + auth_end];
        }

        // Host name with optional port: example.com:8080
        if (colon := std.mem.lastIndexOfScalar(u8, authority, ':')) {
            const port_str = authority[colon + 1 ..];
            const port_num = std.fmt.parseInt(u16, port_str, 10) catch 0;
            if (((is_http || is_ws) && port_num == 80) || ((is_https || is_wss) && port_num == 443)) {
                return page_url[0 .. proto_end + sep.len + colon];
            }
        }

        return page_url[0 .. proto_end + sep.len + auth_end];
    }

    static fn isSameOrigin(url_a: []const u8, url_b: []const u8): bool {
        const orig_a = computeOrigin(url_a);
        const orig_b = computeOrigin(url_b);
        if (std.mem.eql(u8, orig_a, "null") || std.mem.eql(u8, orig_b, "null")) {
            return false;
        }
        return std.mem.eql(u8, orig_a, orig_b);
    }

    static fn isOpaqueOrigin(origin: []const u8): bool {
        return std.mem.eql(u8, origin, "null") || origin.len == 0;
    }
}
