// Migrated from vendor/kitesurf/packages/engine/src/eval/forbidden-headers.ts

pub fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return std.ascii.eqlIgnoreCase(s[0..prefix.len], prefix);
}

const FORBIDDEN_REQUEST_HEADER_NAMES: [21][]const u8 = [
    "accept-charset",
    "accept-encoding",
    "access-control-request-headers",
    "access-control-request-method",
    "connection",
    "content-length",
    "cookie",
    "cookie2",
    "date",
    "dnt",
    "expect",
    "host",
    "keep-alive",
    "origin",
    "referer",
    "set-cookie",
    "te",
    "trailer",
    "transfer-encoding",
    "upgrade",
    "via",
];

const METHOD_OVERRIDE_NAMES: [3][]const u8 = [
    "x-http-method",
    "x-http-method-override",
    "x-method-override",
];

const FORBIDDEN_METHODS: [3][]const u8 = [
    "CONNECT",
    "TRACE",
    "TRACK",
];

pub fn isForbiddenRequestHeaderName(name: []const u8): bool {
    if (startsWithIgnoreCase(name, "proxy-") || startsWithIgnoreCase(name, "sec-")) {
        return true;
    }
    let i: usize = 0;
    while (i < FORBIDDEN_REQUEST_HEADER_NAMES.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(name, FORBIDDEN_REQUEST_HEADER_NAMES[i])) return true;
    }
    return false;
}

pub fn isMethodOverrideHeaderName(name: []const u8): bool {
    let i: usize = 0;
    while (i < METHOD_OVERRIDE_NAMES.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(name, METHOD_OVERRIDE_NAMES[i])) return true;
    }
    return false;
}

pub fn isForbiddenMethod(method: []const u8): bool {
    let i: usize = 0;
    while (i < FORBIDDEN_METHODS.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(method, FORBIDDEN_METHODS[i])) return true;
    }
    return false;
}

pub fn isForbiddenResponseHeader(name: []const u8): bool {
    return std.ascii.eqlIgnoreCase(name, "set-cookie") || std.ascii.eqlIgnoreCase(name, "set-cookie2");
}

pub fn isForbiddenRequestHeader(name: []const u8, value: []const u8): bool {
    if (isForbiddenRequestHeaderName(name)) {
        return true;
    }
    if (!isMethodOverrideHeaderName(name)) {
        return false;
    }
    let it = std.mem.splitScalar(u8, value, ',');
    while (raw_opt := it.next()) {
        const item = std.mem.trim(u8, raw_opt, " \t\r\n");
        if (isForbiddenMethod(item)) {
            return true;
        }
    }
    return false;
}
