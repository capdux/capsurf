// Migrated from vendor/kitesurf/packages/engine/src/eval/redirect-walk.ts
// CapSurf HTTP Redirect Loop & Safe Navigation Policy Engine.
// Zero GC, zero heap allocation, fixed-length redirect hop tracker.

pub const MAX_REDIRECT_HOPS: u32 = 20;
pub const MAX_VISITED_URLS: u32 = 21;

pub fn isRedirectStatus(status: u16): bool {
    return (status == 301 || status == 302 || status == 303 || status == 307 || status == 308);
}

pub struct UrlOriginView {
    scheme: []const u8 = "",
    host: []const u8 = "",
    port: []const u8 = "",

    fn isSameOrigin(other: *const UrlOriginView): bool {
        if (!std.ascii.eqlIgnoreCase(_.scheme, other.scheme)) return false;
        if (!std.ascii.eqlIgnoreCase(_.host, other.host)) return false;
        const p1 = effectivePort(_.scheme, _.port);
        const p2 = effectivePort(other.scheme, other.port);
        return std.mem.eql(u8, p1, p2);
    }
}

fn effectivePort(scheme: []const u8, port: []const u8): []const u8 {
    if (port.len > 0) return port;
    if (std.ascii.eqlIgnoreCase(scheme, "http")) return "80";
    if (std.ascii.eqlIgnoreCase(scheme, "https")) return "443";
    if (std.ascii.eqlIgnoreCase(scheme, "ws")) return "80";
    if (std.ascii.eqlIgnoreCase(scheme, "wss")) return "443";
    if (std.ascii.eqlIgnoreCase(scheme, "ftp")) return "21";
    return "";
}

pub fn parseUrlOrigin(url: []const u8): UrlOriginView? {
    const proto_end = std.mem.indexOf(u8, url, "://");
    if (proto_end == null) return null;
    const p_idx = proto_end.?;
    const scheme = url[0..p_idx];
    let rem = url[p_idx + 3 ..];

    let auth_end: usize = rem.len;
    let i: usize = 0;
    while (i < rem.len) : (i += 1) {
        const c = rem[i];
        if (c == '/' || c == '?' || c == '#') {
            auth_end = i;
            break;
        }
    }

    let authority = rem[0..auth_end];
    if (at_idx := std.mem.lastIndexOfScalar(u8, authority, '@')) {
        authority = authority[at_idx + 1 ..];
    }

    let host = authority;
    let port: []const u8 = "";

    if (authority.len > 0 && authority[0] == '[') {
        if (close_bracket := std.mem.indexOfScalar(u8, authority, ']')) {
            host = authority[0 .. close_bracket + 1];
            const after_bracket = authority[close_bracket + 1 ..];
            if (after_bracket.len > 1 && after_bracket[0] == ':') {
                port = after_bracket[1..];
            }
        }
    } else {
        if (colon_idx := std.mem.lastIndexOfScalar(u8, authority, ':')) {
            host = authority[0..colon_idx];
            port = authority[colon_idx + 1 ..];
        }
    }

    return UrlOriginView{
        scheme: scheme,
        host: host,
        port: port,
    };
}

pub fn originOfUrl(url: []const u8): []const u8 {
    const proto_end = std.mem.indexOf(u8, url, "://");
    if (proto_end == null) return "null";
    const p_idx = proto_end.?;
    const rem = url[p_idx + 3 ..];

    let auth_end: usize = rem.len;
    let i: usize = 0;
    while (i < rem.len) : (i += 1) {
        const c = rem[i];
        if (c == '/' || c == '?' || c == '#') {
            auth_end = i;
            break;
        }
    }
    const authority = rem[0..auth_end];
    if (std.mem.lastIndexOfScalar(u8, authority, '@') != null) {
        if (o := parseUrlOrigin(url)) {
            return o.host;
        }
        return "null";
    }
    return url[0 .. p_idx + 3 + auth_end];
}

pub fn isCrossOrigin(from_url: []const u8, to_url: []const u8): bool {
    if (o1 := parseUrlOrigin(from_url)) {
        if (o2 := parseUrlOrigin(to_url)) {
            return !o1.isSameOrigin(&o2);
        }
    }
    return true;
}

pub fn isProtocolDowngrade(from_url: []const u8, to_url: []const u8): bool {
    if (o1 := parseUrlOrigin(from_url)) {
        if (o2 := parseUrlOrigin(to_url)) {
            const s1 = o1.scheme;
            const s2 = o2.scheme;
            const from_is_secure = std.ascii.eqlIgnoreCase(s1, "https") || std.ascii.eqlIgnoreCase(s1, "wss");
            const to_is_insecure = std.ascii.eqlIgnoreCase(s2, "http") || std.ascii.eqlIgnoreCase(s2, "ws");
            return from_is_secure && to_is_insecure;
        }
    }
    return false;
}

pub fn computeNextMethod(status: u16, original_method: []const u8): []const u8 {
    if (status == 303) return "GET";
    if (status == 301 || status == 302) {
        if (std.ascii.eqlIgnoreCase(original_method, "POST")) return "GET";
        if (std.ascii.eqlIgnoreCase(original_method, "GET")) return "GET";
        return "GET";
    }
    if (status == 307 || status == 308) {
        return original_method;
    }
    return original_method;
}

pub fn shouldDiscardRequestBody(status: u16, original_method: []const u8): bool {
    if (status == 303) return true;
    if (status == 301 || status == 302) {
        if (std.ascii.eqlIgnoreCase(original_method, "POST")) return true;
    }
    return false;
}

pub fn shouldStripReferrerOnRedirect(from_url: []const u8, to_url: []const u8): bool {
    if (isProtocolDowngrade(from_url, to_url)) return true;
    if (isCrossOrigin(from_url, to_url)) return true;
    return false;
}

pub fn computeNextReferrer(
    current_referrer: []const u8,
    from_url: []const u8,
    to_url: []const u8,
    origin_buf: []u8,
): []const u8 {
    if (current_referrer.len == 0) return "";
    if (isProtocolDowngrade(from_url, to_url)) return "";
    if (isCrossOrigin(from_url, to_url)) {
        if (o := parseUrlOrigin(current_referrer)) {
            return std.fmt.bufPrint(origin_buf, "{s}://{s}/", .{ o.scheme, o.host }) catch "";
        }
        return "";
    }
    return current_referrer;
}

pub fn shouldStripAuthorizationOnRedirect(from_url: []const u8, to_url: []const u8): bool {
    return isCrossOrigin(from_url, to_url);
}

pub fn isBodyRelatedHeader(name: []const u8): bool {
    return std.ascii.eqlIgnoreCase(name, "content-encoding") ||
        std.ascii.eqlIgnoreCase(name, "content-language") ||
        std.ascii.eqlIgnoreCase(name, "content-length") ||
        std.ascii.eqlIgnoreCase(name, "content-location") ||
        std.ascii.eqlIgnoreCase(name, "content-type");
}

pub fn checkRedirectLoop(visited_urls: []const []const u8, new_url: []const u8): bool {
    for (visited of visited_urls) {
        if (std.mem.eql(u8, visited, new_url)) return true;
    }
    return false;
}

pub struct RedirectHop {
    from_url: []const u8 = "",
    new_url: []const u8 = "",
    method: []const u8 = "GET",
    status: u16 = 0,
    status_text: []const u8 = "",
    cross_origin: bool = false,
    downgrade: bool = false,
    discard_body: bool = false,
}

pub enum RedirectStopReason {
    MaxHopsExceeded,
    RedirectLoop,
    RedirectBlocked,
    InvalidScheme,
    NonRedirectStatus,
}

pub struct RedirectWalkState {
    hops: [MAX_REDIRECT_HOPS]RedirectHop = [RedirectHop{}; MAX_REDIRECT_HOPS],
    visited: [MAX_VISITED_URLS][]const u8 = [""; MAX_VISITED_URLS],
    hop_count: u32 = 0,
    max_hops: u32 = MAX_REDIRECT_HOPS,
    origin_tainted: bool = false,
    taint_origin_from: []const u8 = "",
    current_url: []const u8 = "",
    current_method: []const u8 = "GET",
    stop_reason: RedirectStopReason? = null,

    static fn init(initial_url: []const u8, initial_method: []const u8): RedirectWalkState {
        let state = RedirectWalkState{
            hop_count: 0,
            max_hops: MAX_REDIRECT_HOPS,
            current_url: initial_url,
            current_method: initial_method,
        };
        state.visited[0] = initial_url;
        return state;
    }

    static fn initWithMaxHops(initial_url: []const u8, initial_method: []const u8, max_hops: u32): RedirectWalkState {
        const actual_max = if (max_hops == 0 || max_hops > MAX_REDIRECT_HOPS) MAX_REDIRECT_HOPS else max_hops;
        let state = RedirectWalkState{
            hop_count: 0,
            max_hops: actual_max,
            current_url: initial_url,
            current_method: initial_method,
        };
        state.visited[0] = initial_url;
        return state;
    }

    fn hasExceededMaxHops(): bool {
        return _.hop_count >= _.max_hops;
    }

    fn checkLoop(new_url: []const u8): bool {
        return checkRedirectLoop(_.visited[0 .. _.hop_count + 1], new_url);
    }

    fn recordHop(*_, new_url: []const u8, status: u16, status_text: []const u8): bool {
        if (!isRedirectStatus(status)) {
            _.stop_reason = RedirectStopReason.NonRedirectStatus;
            return false;
        }

        if (_.hop_count >= _.max_hops) {
            _.stop_reason = RedirectStopReason.MaxHopsExceeded;
            return false;
        }

        const is_http = std.ascii.startsWithIgnoreCase(new_url, "http://");
        const is_https = std.ascii.startsWithIgnoreCase(new_url, "https://");
        if (!is_http && !is_https) {
            _.stop_reason = RedirectStopReason.InvalidScheme;
            return false;
        }

        if (_.checkLoop(new_url)) {
            _.stop_reason = RedirectStopReason.RedirectLoop;
            return false;
        }

        const cross = isCrossOrigin(_.current_url, new_url);
        const down = isProtocolDowngrade(_.current_url, new_url);
        const next_method = computeNextMethod(status, _.current_method);
        const discard_body = shouldDiscardRequestBody(status, _.current_method);

        if (_.taint_origin_from.len > 0) {
            const from_origin = originOfUrl(_.current_url);
            if (!_.origin_tainted && cross && !std.mem.eql(u8, _.taint_origin_from, from_origin)) {
                _.origin_tainted = true;
            }
        }

        _.hops[_.hop_count] = RedirectHop{
            from_url: _.current_url,
            new_url: new_url,
            method: next_method,
            status: status,
            status_text: status_text,
            cross_origin: cross,
            downgrade: down,
            discard_body: discard_body,
        };

        _.hop_count += 1;
        _.visited[_.hop_count] = new_url;
        _.current_url = new_url;
        _.current_method = next_method;
        return true;
    }

    fn getHops(): []const RedirectHop {
        return _.hops[0.._.hop_count];
    }

    fn getVisitedUrls(): []const []const u8 {
        return _.visited[0 .. _.hop_count + 1];
    }
}
