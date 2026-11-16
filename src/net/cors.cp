// Migrated from vendor/kitesurf/packages/engine/src/eval/cors.ts

import { isForbiddenRequestHeaderName, isForbiddenResponseHeader, startsWithIgnoreCase } from "./forbidden_headers.cp";

pub enum CredentialsMode {
    Omit,
    SameOrigin,
    Include,
}

pub fn scriptCredentialsMode(cross_origin: ([]const u8)?): CredentialsMode {
    if (co := cross_origin) {
        if (std.ascii.eqlIgnoreCase(co, "use-credentials")) {
            return CredentialsMode.Include;
        }
    }
    return CredentialsMode.SameOrigin;
}

pub fn isSafelistMethod(method: []const u8): bool {
    return std.ascii.eqlIgnoreCase(method, "GET")
        || std.ascii.eqlIgnoreCase(method, "HEAD")
        || std.ascii.eqlIgnoreCase(method, "POST");
}
pub const isSafelistedMethod = isSafelistMethod;

pub fn stripHttpWhitespace(s: []const u8): []const u8 {
    return std.mem.trim(u8, s, " \t\r\n");
}

pub fn isHttpTokenChar(c: u8): bool {
    if (std.ascii.isAlphanumeric(c)) return true;
    return switch (c) {
        '!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~' => true,
        else => false,
    };
}

pub fn isHttpToken(s: []const u8): bool {
    if (s.len == 0) return false;
    for (c of s) {
        if (!isHttpTokenChar(c)) return false;
    }
    return true;
}

pub const MAX_TOKEN_LIST_SIZE: usize = 32;

pub struct TokenList {
    items: [MAX_TOKEN_LIST_SIZE][]const u8 = [""; MAX_TOKEN_LIST_SIZE],
    len: usize = 0,

    fn slice(): []const []const u8 {
        return _.items[0.._.len];
    }

    fn contains(token: []const u8): bool {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.items[i], token)) return true;
        }
        return false;
    }
}

pub fn parseTokenList(value: []const u8): TokenList? {
    let list = TokenList{};
    let it = std.mem.splitScalar(u8, value, ',');
    while (raw_opt := it.next()) {
        const item = stripHttpWhitespace(raw_opt);
        if (item.len == 0) continue;
        if (!isHttpToken(item)) return null;
        if (list.len < MAX_TOKEN_LIST_SIZE) {
            list.items[list.len] = item;
            list.len += 1;
        }
    }
    return list;
}

pub fn hasCorsUnsafeByte(value: []const u8): bool {
    for (c of value) {
        if (c < 0x20 && c != 0x09) return true;
        if (c == 0x7f) return true;
        switch (c) {
            '"', '(', ')', ':', '<', '>', '?', '@', '[', '\\', ']', '{', '}' => return true,
            else => {},
        }
    }
    return false;
}

fn isSafelistContentType(value: []const u8): bool {
    if (hasCorsUnsafeByte(value)) return false;
    const s = stripHttpWhitespace(value);
    let slash_idx: usize = 0;
    if (idx := std.mem.indexOfScalar(u8, s, '/')) {
        slash_idx = idx;
    } else {
        return false;
    }
    const type_part = s[0..slash_idx];
    if (!isHttpToken(type_part)) return false;
    const rest = s[slash_idx + 1 ..];
    let subtype_raw = rest;
    if (semi_idx := std.mem.indexOfScalar(u8, rest, ';')) {
        subtype_raw = rest[0..semi_idx];
    }
    const subtype_part = std.mem.trim(u8, subtype_raw, " \t\r\n");
    if (!isHttpToken(subtype_part)) return false;

    if (std.ascii.eqlIgnoreCase(type_part, "application") && std.ascii.eqlIgnoreCase(subtype_part, "x-www-form-urlencoded")) {
        return true;
    }
    if (std.ascii.eqlIgnoreCase(type_part, "multipart") && std.ascii.eqlIgnoreCase(subtype_part, "form-data")) {
        return true;
    }
    if (std.ascii.eqlIgnoreCase(type_part, "text") && std.ascii.eqlIgnoreCase(subtype_part, "plain")) {
        return true;
    }
    return false;
}

pub fn isSafelistedRangeHeader(value: []const u8): bool {
    if (!std.mem.startsWith(u8, value, "bytes=")) return false;
    let i: usize = 6;
    const start_begin = i;
    while (i < value.len && value[i] >= '0' && value[i] <= '9') : (i += 1) {}
    const start_digits = value[start_begin..i];
    if (start_digits.len == 0) return false;

    if (i >= value.len || value[i] != '-') return false;
    i += 1;

    const end_begin = i;
    while (i < value.len && value[i] >= '0' && value[i] <= '9') : (i += 1) {}
    const end_digits = value[end_begin..i];

    if (i != value.len) return false;

    const start_num = std.fmt.parseInt(u64, start_digits, 10) catch return false;
    if (end_digits.len > 0) {
        const end_num = std.fmt.parseInt(u64, end_digits, 10) catch return false;
        if (start_num > end_num) return false;
    }
    return true;
}

pub const MAX_CORS_SAFE_HEADER_VALUE_CHARS: usize = 128;

pub fn isSafelistRequestHeader(name: []const u8, value: []const u8): bool {
    if (value.len > MAX_CORS_SAFE_HEADER_VALUE_CHARS) return false;
    if (std.ascii.eqlIgnoreCase(name, "accept")) {
        return !hasCorsUnsafeByte(value);
    } else if (std.ascii.eqlIgnoreCase(name, "accept-language") || std.ascii.eqlIgnoreCase(name, "content-language")) {
        for (c of value) {
            if (std.ascii.isAlphanumeric(c)) continue;
            switch (c) {
                ' ', '*', ',', '-', '.', ';', '=' => continue,
                else => return false,
            }
        }
        return true;
    } else if (std.ascii.eqlIgnoreCase(name, "content-type")) {
        return isSafelistContentType(value);
    } else if (std.ascii.eqlIgnoreCase(name, "range")) {
        return isSafelistedRangeHeader(value);
    }
    return false;
}
pub const isSafelistedRequestHeader = isSafelistRequestHeader;

pub fn isSafelistedResponseHeader(name: []const u8): bool {
    return std.ascii.eqlIgnoreCase(name, "cache-control")
        || std.ascii.eqlIgnoreCase(name, "content-language")
        || std.ascii.eqlIgnoreCase(name, "content-length")
        || std.ascii.eqlIgnoreCase(name, "content-type")
        || std.ascii.eqlIgnoreCase(name, "expires")
        || std.ascii.eqlIgnoreCase(name, "last-modified")
        || std.ascii.eqlIgnoreCase(name, "pragma");
}

pub fn isCorsUnsafeRequestHeader(name: []const u8, value: []const u8): bool {
    if (startsWithIgnoreCase(name, "x-kitesurf-")) return false;
    if (startsWithIgnoreCase(name, "proxy-") || startsWithIgnoreCase(name, "sec-")) return false;
    if (isForbiddenRequestHeaderName(name)) return false;
    return !isSafelistRequestHeader(name, value);
}

pub fn needsPreflight(method: []const u8, has_unsafe_headers: bool): bool {
    if (!isSafelistMethod(method)) return true;
    return has_unsafe_headers;
}

pub fn checkCorsResponse(
    origin: []const u8,
    allow_origin: []const u8,
    allow_credentials: bool,
): bool {
    const trimmed_origin = stripHttpWhitespace(allow_origin);
    if (trimmed_origin.len == 0) return false;

    // Multiple origins (contains comma) is invalid per spec
    if (std.mem.indexOfScalar(u8, trimmed_origin, ',') != null) return false;

    if (std.mem.eql(u8, trimmed_origin, "*")) {
        if (allow_credentials) return false;
        return true;
    }

    return std.ascii.eqlIgnoreCase(trimmed_origin, origin) || std.mem.eql(u8, trimmed_origin, origin);
}

pub fn checkCorsPreflight(
    origin: []const u8,
    request_method: []const u8,
    request_headers: []const u8,
    allow_origin: []const u8,
    allow_methods: []const u8,
    allow_headers: []const u8,
    allow_credentials: bool,
): bool {
    // 1. Origin verification
    if (!checkCorsResponse(origin, allow_origin, allow_credentials)) {
        return false;
    }

    // 2. Methods verification
    let method_ok = false;
    if (isSafelistMethod(request_method)) {
        method_ok = true;
    } else {
        if (methods_opt := parseTokenList(allow_methods)) {
            const methods = methods_opt;
            if (methods.contains(request_method)) {
                method_ok = true;
            } else if (methods.contains("*") && !allow_credentials) {
                method_ok = true;
            }
        } else {
            return false;
        }
    }
    if (!method_ok) return false;

    // 3. Request headers verification
    if (request_headers.len > 0) {
        if (allowed_hdrs_opt := parseTokenList(allow_headers)) {
            const allowed_hdrs = allowed_hdrs_opt;
            const header_wildcard = allowed_hdrs.contains("*") && !allow_credentials;

            let h_it = std.mem.splitScalar(u8, request_headers, ',');
            while (h_raw := h_it.next()) {
                const h = stripHttpWhitespace(h_raw);
                if (h.len == 0) continue;

                if (allowed_hdrs.contains(h)) continue;
                if (header_wildcard && !std.ascii.eqlIgnoreCase(h, "authorization")) continue;

                return false;
            }
        } else {
            return false;
        }
    }

    return true;
}

pub enum CorsError {
    MissingOrigin,
    MultipleOrigins,
    WildcardWithCredentials,
    OriginMismatch,
    CredentialsNotAllowed,
}

pub fn corsCheck(
    allow_origin: ([]const u8)?,
    allow_credentials: ([]const u8)?,
    request_origin: []const u8,
    credentials_include: bool,
): CorsError? {
    if (allow_origin == null) return CorsError.MissingOrigin;
    const trimmed = stripHttpWhitespace(allow_origin.?);
    if (trimmed.len == 0) return CorsError.MissingOrigin;

    if (std.mem.eql(u8, trimmed, "*")) {
        if (credentials_include) return CorsError.WildcardWithCredentials;
        return null;
    }

    if (!std.mem.eql(u8, trimmed, request_origin)) {
        if (std.mem.indexOfScalar(u8, trimmed, ',') != null) {
            return CorsError.MultipleOrigins;
        }
        return CorsError.OriginMismatch;
    }

    if (credentials_include) {
        if (allow_credentials == null || !std.mem.eql(u8, stripHttpWhitespace(allow_credentials.?), "true")) {
            return CorsError.CredentialsNotAllowed;
        }
    }
    return null;
}

pub const MAX_PREFLIGHT_CACHE_ENTRIES: usize = 64;

pub struct PreflightEntry {
    partition: [64]u8 = [0; 64],
    partition_len: usize = 0,
    origin: [128]u8 = [0; 128],
    origin_len: usize = 0,
    url: [256]u8 = [0; 256],
    url_len: usize = 0,
    credentials_include: bool = false,
    method_wildcard: bool = false,
    header_wildcard: bool = false,
    methods_buf: [128]u8 = [0; 128],
    methods_len: usize = 0,
    headers_buf: [512]u8 = [0; 512],
    headers_len: usize = 0,
    expires_at: i64 = 0,
    valid: bool = false,

    fn satisfies(
        method: []const u8,
        request_headers: []const u8,
        credentials_include: bool,
        now_ms: i64,
    ): bool {
        if (!_.valid) return false;
        if (_.expires_at <= now_ms) return false;

        let method_ok = false;
        if (isSafelistMethod(method)) {
            method_ok = true;
        } else if (_.method_wildcard && !credentials_include) {
            method_ok = true;
        } else {
            const methods_str = _.methods_buf[0.._.methods_len];
            let m_it = std.mem.splitScalar(u8, methods_str, ',');
            while (m_raw := m_it.next()) {
                const m = stripHttpWhitespace(m_raw);
                if (std.ascii.eqlIgnoreCase(m, method)) {
                    method_ok = true;
                    break;
                }
            }
        }
        if (!method_ok) return false;

        if (request_headers.len > 0) {
            let h_it = std.mem.splitScalar(u8, request_headers, ',');
            while (h_raw := h_it.next()) {
                const h = stripHttpWhitespace(h_raw);
                if (h.len == 0) continue;

                if (_.header_wildcard && !credentials_include && !std.ascii.eqlIgnoreCase(h, "authorization")) {
                    continue;
                }

                let found = false;
                const hdrs_str = _.headers_buf[0.._.headers_len];
                let allowed_it = std.mem.splitScalar(u8, hdrs_str, ',');
                while (a_raw := allowed_it.next()) {
                    const a = stripHttpWhitespace(a_raw);
                    if (std.ascii.eqlIgnoreCase(a, h)) {
                        found = true;
                        break;
                    }
                }
                if (!found) return false;
            }
        }

        return true;
    }
}

pub struct PreflightCache {
    entries: [MAX_PREFLIGHT_CACHE_ENTRIES]PreflightEntry = [PreflightEntry{}; MAX_PREFLIGHT_CACHE_ENTRIES],
    count: usize = 0,
    next_evict: usize = 0,

    fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_PREFLIGHT_CACHE_ENTRIES) : (i += 1) {
            _.entries[i].valid = false;
        }
        _.count = 0;
        _.next_evict = 0;
    }

    fn lookup(
        partition: []const u8,
        origin: []const u8,
        url: []const u8,
        method: []const u8,
        request_headers: []const u8,
        credentials_include: bool,
        now_ms: i64,
    ): bool {
        let i: usize = 0;
        while (i < MAX_PREFLIGHT_CACHE_ENTRIES) : (i += 1) {
            if (!_.entries[i].valid) continue;
            if (_.entries[i].credentials_include != credentials_include) continue;
            if (!std.mem.eql(u8, _.entries[i].partition[0.._.entries[i].partition_len], partition)) continue;
            if (!std.mem.eql(u8, _.entries[i].origin[0.._.entries[i].origin_len], origin)) continue;
            if (!std.mem.eql(u8, _.entries[i].url[0.._.entries[i].url_len], url)) continue;
            return _.entries[i].satisfies(method, request_headers, credentials_include, now_ms);
        }
        return false;
    }

    fn store(
        *_,
        partition: []const u8,
        origin: []const u8,
        url: []const u8,
        credentials_include: bool,
        allow_methods: []const u8,
        allow_headers: []const u8,
        max_age_seconds: u32,
        now_ms: i64,
    ) {
        if (partition.len == 0) return;
        if (max_age_seconds == 0) return;
        const clamped_seconds = @min(max_age_seconds, 600);
        const ttl_ms = @as(i64, clamped_seconds) * 1000;
        const expires_at = now_ms + ttl_ms;

        let slot_idx: usize = MAX_PREFLIGHT_CACHE_ENTRIES;
        let i: usize = 0;
        while (i < MAX_PREFLIGHT_CACHE_ENTRIES) : (i += 1) {
            if (_.entries[i].valid
                && _.entries[i].credentials_include == credentials_include
                && std.mem.eql(u8, _.entries[i].partition[0.._.entries[i].partition_len], partition)
                && std.mem.eql(u8, _.entries[i].origin[0.._.entries[i].origin_len], origin)
                && std.mem.eql(u8, _.entries[i].url[0.._.entries[i].url_len], url))
            {
                slot_idx = i;
                break;
            }
        }

        if (slot_idx == MAX_PREFLIGHT_CACHE_ENTRIES) {
            let j: usize = 0;
            while (j < MAX_PREFLIGHT_CACHE_ENTRIES) : (j += 1) {
                if (!_.entries[j].valid || _.entries[j].expires_at <= now_ms) {
                    slot_idx = j;
                    break;
                }
            }
        }

        if (slot_idx == MAX_PREFLIGHT_CACHE_ENTRIES) {
            slot_idx = _.next_evict;
            _.next_evict = (_.next_evict + 1) % MAX_PREFLIGHT_CACHE_ENTRIES;
        }

        let e = &_.entries[slot_idx];
        e.valid = false;

        const part_len = @min(partition.len, 64);
        @memcpy(e.partition[0..part_len], partition[0..part_len]);
        e.partition_len = part_len;

        const orig_len = @min(origin.len, 128);
        @memcpy(e.origin[0..orig_len], origin[0..orig_len]);
        e.origin_len = orig_len;

        const u_len = @min(url.len, 256);
        @memcpy(e.url[0..u_len], url[0..u_len]);
        e.url_len = u_len;

        e.credentials_include = credentials_include;
        e.expires_at = expires_at;

        const m_len = @min(allow_methods.len, 128);
        @memcpy(e.methods_buf[0..m_len], allow_methods[0..m_len]);
        e.methods_len = m_len;

        e.method_wildcard = false;
        let m_it = std.mem.splitScalar(u8, allow_methods, ',');
        while (m_raw := m_it.next()) {
            if (std.mem.eql(u8, stripHttpWhitespace(m_raw), "*")) {
                e.method_wildcard = true;
                break;
            }
        }

        const h_len = @min(allow_headers.len, 512);
        @memcpy(e.headers_buf[0..h_len], allow_headers[0..h_len]);
        e.headers_len = h_len;

        e.header_wildcard = false;
        let h_it = std.mem.splitScalar(u8, allow_headers, ',');
        while (h_raw := h_it.next()) {
            if (std.mem.eql(u8, stripHttpWhitespace(h_raw), "*")) {
                e.header_wildcard = true;
                break;
            }
        }

        e.valid = true;
    }
}

pub threadlocal let g_preflight_cache: PreflightCache = PreflightCache{};

pub fn preflightCacheLookup(
    partition: []const u8,
    origin: []const u8,
    url: []const u8,
    method: []const u8,
    request_headers: []const u8,
    credentials_include: bool,
    now_ms: i64,
): bool {
    return g_preflight_cache.lookup(partition, origin, url, method, request_headers, credentials_include, now_ms);
}

pub fn preflightCacheStore(
    partition: []const u8,
    origin: []const u8,
    url: []const u8,
    credentials_include: bool,
    allow_methods: []const u8,
    allow_headers: []const u8,
    max_age_seconds: u32,
    now_ms: i64,
) {
    g_preflight_cache.store(partition, origin, url, credentials_include, allow_methods, allow_headers, max_age_seconds, now_ms);
}

pub fn clearPreflightCache() {
    g_preflight_cache.clear();
}
