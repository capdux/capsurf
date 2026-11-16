// Migrated from vendor/kitesurf/packages/script-gate/src/referrer-policy.ts

pub enum ReferrerPolicy {
    NoReferrer,
    NoReferrerWhenDowngrade,
    SameOrigin,
    Origin,
    StrictOrigin,
    OriginWhenCrossOrigin,
    StrictOriginWhenCrossOrigin,
    UnsafeUrl,
}

pub fn parseReferrerPolicy(header: []const u8): ReferrerPolicy {
    let policy: ReferrerPolicy = ReferrerPolicy.StrictOriginWhenCrossOrigin;
    let it = std.mem.splitScalar(u8, header, ',');
    while (token_opt := it.next()) {
        const token = std.mem.trim(u8, token_opt, " \t\r\n");
        if (token.len == 0) continue;
        if (std.ascii.eqlIgnoreCase(token, "no-referrer")) {
            policy = ReferrerPolicy.NoReferrer;
        } else if (std.ascii.eqlIgnoreCase(token, "no-referrer-when-downgrade")) {
            policy = ReferrerPolicy.NoReferrerWhenDowngrade;
        } else if (std.ascii.eqlIgnoreCase(token, "same-origin")) {
            policy = ReferrerPolicy.SameOrigin;
        } else if (std.ascii.eqlIgnoreCase(token, "origin")) {
            policy = ReferrerPolicy.Origin;
        } else if (std.ascii.eqlIgnoreCase(token, "strict-origin")) {
            policy = ReferrerPolicy.StrictOrigin;
        } else if (std.ascii.eqlIgnoreCase(token, "origin-when-cross-origin")) {
            policy = ReferrerPolicy.OriginWhenCrossOrigin;
        } else if (std.ascii.eqlIgnoreCase(token, "strict-origin-when-cross-origin")) {
            policy = ReferrerPolicy.StrictOriginWhenCrossOrigin;
        } else if (std.ascii.eqlIgnoreCase(token, "unsafe-url")) {
            policy = ReferrerPolicy.UnsafeUrl;
        }
    }
    return policy;
}

pub fn isPotentiallyTrustworthy(url: []const u8): bool {
    if (std.mem.eql(u8, url, "about:blank") || std.mem.eql(u8, url, "about:srcdoc")) return true;
    if (std.mem.startsWith(u8, url, "about:blank") && (url.len == 11 || url[11] == '?' || url[11] == '#')) return true;
    if (std.mem.startsWith(u8, url, "about:srcdoc") && (url.len == 12 || url[12] == '?' || url[12] == '#')) return true;

    if (colon_idx := std.mem.indexOfScalar(u8, url, ':')) {
        const scheme = url[0..colon_idx];
        if (std.ascii.eqlIgnoreCase(scheme, "data") || std.ascii.eqlIgnoreCase(scheme, "file")) return true;
        if (std.ascii.eqlIgnoreCase(scheme, "https") || std.ascii.eqlIgnoreCase(scheme, "wss")) return true;
    }

    if (p_end := std.mem.indexOf(u8, url, "://")) {
        let rem = url[p_end + 3 ..];
        let auth_end: usize = rem.len;
        let i: usize = 0;
        while (i < rem.len) : (i += 1) {
            const c = rem[i];
            if (c == '/' || c == '?' || c == '#') {
                auth_end = i;
                break;
            }
        }
        const auth = rem[0..auth_end];
        let host_port = auth;
        if (at_idx := std.mem.lastIndexOfScalar(u8, auth, '@')) {
            host_port = auth[at_idx + 1 ..];
        }

        let host = host_port;
        if (host.len > 0 && host[0] == '[') {
            if (b_end := std.mem.indexOfScalar(u8, host, ']')) {
                host = host[0 .. b_end + 1];
            }
        } else {
            if (col_idx := std.mem.lastIndexOfScalar(u8, host, ':')) {
                host = host[0..col_idx];
            }
        }

        if (std.ascii.eqlIgnoreCase(host, "localhost")) return true;
        if (std.ascii.eqlIgnoreCase(host, "127.0.0.1")) return true;
        if (std.ascii.eqlIgnoreCase(host, "[::1]")) return true;
        if (host.len > 10 && std.ascii.endsWithIgnoreCase(host, ".localhost")) return true;
    }

    return false;
}

struct ParsedUrlView {
    protocol: []const u8 = "",
    host_and_port: []const u8 = "",
    hostname: []const u8 = "",
    port: []const u8 = "",
    path_and_query: []const u8 = "",
    auth_end_idx: usize = 0,
    has_userinfo: bool = false,
    slash_after_auth: bool = false,
}

fn parseUrlView(url: []const u8): ParsedUrlView? {
    if (url.len == 0) return null;

    let full_no_hash = url;
    if (h_idx := std.mem.indexOfScalar(u8, url, '#')) {
        full_no_hash = url[0..h_idx];
    }

    const proto_end = std.mem.indexOf(u8, full_no_hash, "://");
    if (proto_end == null) {
        if (c_idx := std.mem.indexOfScalar(u8, full_no_hash, ':')) {
            return ParsedUrlView{
                protocol: full_no_hash[0 .. c_idx + 1],
                path_and_query: full_no_hash[c_idx + 1 ..],
            };
        }
        return null;
    }

    const p_idx = proto_end.?;
    const protocol = full_no_hash[0 .. p_idx + 1];
    let rem = full_no_hash[p_idx + 3 ..];

    let auth_end: usize = rem.len;
    let i: usize = 0;
    while (i < rem.len) : (i += 1) {
        const c = rem[i];
        if (c == '/' || c == '?') {
            auth_end = i;
            break;
        }
    }

    const authority = rem[0..auth_end];
    let has_userinfo = false;
    let host_and_port = authority;
    if (at_idx := std.mem.lastIndexOfScalar(u8, authority, '@')) {
        has_userinfo = true;
        host_and_port = authority[at_idx + 1 ..];
    }

    let hostname = host_and_port;
    let port: []const u8 = "";

    if (host_and_port.len > 0 && host_and_port[0] == '[') {
        if (close_bracket := std.mem.indexOfScalar(u8, host_and_port, ']')) {
            hostname = host_and_port[0 .. close_bracket + 1];
            const after_bracket = host_and_port[close_bracket + 1 ..];
            if (after_bracket.len > 1 && after_bracket[0] == ':') {
                port = after_bracket[1..];
            }
        }
    } else {
        if (colon_idx := std.mem.lastIndexOfScalar(u8, host_and_port, ':')) {
            hostname = host_and_port[0..colon_idx];
            port = host_and_port[colon_idx + 1 ..];
        }
    }

    const slash_after_auth = (auth_end < rem.len && rem[auth_end] == '/');

    return ParsedUrlView{
        protocol: protocol,
        host_and_port: host_and_port,
        hostname: hostname,
        port: port,
        path_and_query: rem[auth_end..],
        auth_end_idx: p_idx + 3 + auth_end,
        has_userinfo: has_userinfo,
        slash_after_auth: slash_after_auth,
    };
}

fn getEffectivePort(protocol: []const u8, port: []const u8): []const u8 {
    if (port.len > 0) return port;
    if (std.ascii.eqlIgnoreCase(protocol, "http:") || std.ascii.eqlIgnoreCase(protocol, "http")) return "80";
    if (std.ascii.eqlIgnoreCase(protocol, "https:") || std.ascii.eqlIgnoreCase(protocol, "https")) return "443";
    if (std.ascii.eqlIgnoreCase(protocol, "ws:") || std.ascii.eqlIgnoreCase(protocol, "ws")) return "80";
    if (std.ascii.eqlIgnoreCase(protocol, "wss:") || std.ascii.eqlIgnoreCase(protocol, "wss")) return "443";
    if (std.ascii.eqlIgnoreCase(protocol, "ftp:") || std.ascii.eqlIgnoreCase(protocol, "ftp")) return "21";
    return "";
}

fn isSameOrigin(source: *const ParsedUrlView, target: *const ParsedUrlView): bool {
    if (std.mem.eql(u8, target.protocol, "about:") ||
        std.mem.eql(u8, target.protocol, "data:") ||
        std.mem.eql(u8, target.protocol, "blob:") ||
        std.mem.eql(u8, target.protocol, "file:"))
    {
        return false;
    }
    if (!std.ascii.eqlIgnoreCase(source.protocol, target.protocol)) return false;
    if (!std.ascii.eqlIgnoreCase(source.hostname, target.hostname)) return false;
    const s_port = getEffectivePort(source.protocol, source.port);
    const t_port = getEffectivePort(target.protocol, target.port);
    return std.mem.eql(u8, s_port, t_port);
}

@tls let referrer_buf: [2048]u8 = [0; 2048];

pub fn determineReferrer(source_url: []const u8, target_url: []const u8, policy: ReferrerPolicy): []const u8 {
    const source_opt = parseUrlView(source_url);
    if (source_opt == null) return "";
    const source = source_opt.?;

    const target_opt = parseUrlView(target_url);
    if (target_opt == null) return "";
    const target = target_opt.?;

    const is_http = std.ascii.eqlIgnoreCase(source.protocol, "http:");
    const is_https = std.ascii.eqlIgnoreCase(source.protocol, "https:");
    if (!is_http && !is_https) return "";

    const same_origin = isSameOrigin(&source, &target);
    const downgrade = is_https && !isPotentiallyTrustworthy(target_url);

    let full: []const u8 = "";
    let origin_only: []const u8 = "";

    if (source.has_userinfo) {
        let pos: usize = 0;
        @memcpy(referrer_buf[pos .. pos + source.protocol.len], source.protocol);
        pos += source.protocol.len;
        @memcpy(referrer_buf[pos .. pos + 2], "//");
        pos += 2;
        @memcpy(referrer_buf[pos .. pos + source.host_and_port.len], source.host_and_port);
        pos += source.host_and_port.len;

        referrer_buf[pos] = '/';
        const origin_len = pos + 1;

        if (source.path_and_query.len > 0) {
            if (source.path_and_query[0] == '/') {
                @memcpy(referrer_buf[pos .. pos + source.path_and_query.len], source.path_and_query);
                pos += source.path_and_query.len;
            } else {
                pos += 1;
                @memcpy(referrer_buf[pos .. pos + source.path_and_query.len], source.path_and_query);
                pos += source.path_and_query.len;
            }
        }

        full = referrer_buf[0..pos];
        origin_only = referrer_buf[0..origin_len];
    } else {
        if (h_idx := std.mem.indexOfScalar(u8, source_url, '#')) {
            full = source_url[0..h_idx];
        } else {
            full = source_url;
        }

        if (source.slash_after_auth) {
            origin_only = source_url[0 .. source.auth_end_idx + 1];
        } else {
            let pos: usize = 0;
            const prefix = source_url[0..source.auth_end_idx];
            @memcpy(referrer_buf[pos .. pos + prefix.len], prefix);
            pos += prefix.len;
            referrer_buf[pos] = '/';
            pos += 1;
            origin_only = referrer_buf[0..pos];
        }
    }

    switch (policy) {
        .NoReferrer => return "",
        .NoReferrerWhenDowngrade => {
            if (downgrade) return "";
            return full;
        },
        .SameOrigin => {
            if (same_origin) return full;
            return "";
        },
        .Origin => return origin_only,
        .StrictOrigin => {
            if (downgrade) return "";
            return origin_only;
        },
        .OriginWhenCrossOrigin => {
            if (same_origin) return full;
            return origin_only;
        },
        .UnsafeUrl => return full,
        .StrictOriginWhenCrossOrigin => {
            if (same_origin) return full;
            if (downgrade) return "";
            return origin_only;
        },
    }
}
