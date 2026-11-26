// Migrated from vendor/kitesurf/packages/engine/src/cdp/fetch-interception.ts
// src/cdp/fetch_interception.cp
// Pure CP CDP Fetch Domain Interception & Glob State Machine

pub struct RequestPattern {
    pattern: []const u8 = "*",
    resource_type: []const u8 = "",
    request_stage: []const u8 = "Request",
}

pub enum InterceptActionKind {
    Continue,
    Fulfill,
    Fail,
}

pub struct InterceptAction {
    kind: InterceptActionKind = InterceptActionKind.Continue,
    url: []const u8 = "",
    method: []const u8 = "",
    headers: []const u8 = "",
    postData: []const u8 = "",
    post_data: []const u8 = "",
    responseCode: u32 = 200,
    response_code: u32 = 200,
    responseHeaders: []const u8 = "",
    response_headers: []const u8 = "",
    body: []const u8 = "",
    errorReason: []const u8 = "",
    error_reason: []const u8 = "",

    static fn makeContinue(): InterceptAction {
        return InterceptAction{
            kind: InterceptActionKind.Continue,
        };
    }

    static fn makeContinueWith(url: []const u8, method: []const u8, headers: []const u8, post_data: []const u8): InterceptAction {
        return InterceptAction{
            kind: InterceptActionKind.Continue,
            url: url,
            method: method,
            headers: headers,
            postData: post_data,
            post_data: post_data,
        };
    }

    static fn makeFulfill(code: u32, headers: []const u8, body: []const u8): InterceptAction {
        return InterceptAction{
            kind: InterceptActionKind.Fulfill,
            responseCode: code,
            response_code: code,
            responseHeaders: headers,
            response_headers: headers,
            body: body,
        };
    }

    static fn makeFail(reason: []const u8): InterceptAction {
        return InterceptAction{
            kind: InterceptActionKind.Fail,
            errorReason: reason,
            error_reason: reason,
        };
    }
}

pub struct ParkedRequest {
    requestId: []const u8 = "",
    request_id: []const u8 = "",
    url: []const u8 = "",
    method: []const u8 = "GET",
    headers: []const u8 = "",
    post_data: []const u8 = "",
    postData: []const u8 = "",
    resource_type: []const u8 = "",
    verdict: InterceptAction? = null,
    is_paused: bool = true,

    fn getId(): []const u8 {
        if (_.requestId.len > 0) return _.requestId;
        return _.request_id;
    }

    fn getPostData(): []const u8 {
        if (_.post_data.len > 0) return _.post_data;
        return _.postData;
    }
}

pub const MAX_INTERCEPT_PATTERNS: usize = 64;
pub const MAX_PARKED_REQUESTS: usize = 128;

pub struct FetchInterceptionState {
    enabled: bool = false,
    patterns: [MAX_INTERCEPT_PATTERNS]RequestPattern = [RequestPattern{}; MAX_INTERCEPT_PATTERNS],
    pattern_count: usize = 0,
    parked: [MAX_PARKED_REQUESTS]ParkedRequest = [ParkedRequest{}; MAX_PARKED_REQUESTS],
    parked_count: usize = 0,
    next_id: u64 = 1,

    static fn isLineTerminator(c: u8): bool {
        return c == 0x0a || c == 0x0d;
    }

    /// Linear-time non-backtracking glob matcher.
    /// '*' matches any sequence of bytes excluding line terminators.
    /// '?' matches any single byte excluding line terminators.
    static fn globMatches(pattern: []const u8, url: []const u8): bool {
        let i: usize = 0;
        let j: usize = 0;
        let has_star: bool = false;
        let star_j: usize = 0;
        let mark: usize = 0;

        while (i < url.len) {
            if (j < pattern.len) {
                const pc = pattern[j];
                const uc = url[i];
                if (pc == '*') {
                    has_star = true;
                    star_j = j;
                    j += 1;
                    mark = i;
                    continue;
                }
                const match_char = if (pc == '?') (!FetchInterceptionState.isLineTerminator(uc)) else (pc == uc);
                if (match_char) {
                    i += 1;
                    j += 1;
                    continue;
                }
            }
            if (has_star && mark < url.len && !FetchInterceptionState.isLineTerminator(url[mark])) {
                mark += 1;
                i = mark;
                j = star_j + 1;
                continue;
            }
            return false;
        }

        while (j < pattern.len && pattern[j] == '*') {
            j += 1;
        }
        return j == pattern.len;
    }

    fn matches(url: []const u8, resource_type: []const u8): bool {
        if (!_.enabled) return false;
        if (std.mem.startsWith(u8, url, "data:") || std.mem.startsWith(u8, url, "blob:")) {
            return false;
        }
        if (_.pattern_count == 0) return false;
        let idx: usize = 0;
        while (idx < _.pattern_count) : (idx += 1) {
            const p = _.patterns[idx];
            if (p.resource_type.len > 0 && resource_type.len > 0) {
                if (!std.mem.eql(u8, p.resource_type, resource_type)) {
                    continue;
                }
            }
            if (FetchInterceptionState.globMatches(p.pattern, url)) {
                return true;
            }
        }
        return false;
    }

    fn addPattern(*_, pattern: []const u8, resource_type: []const u8, request_stage: []const u8) {
        if (_.pattern_count < _.patterns.len) {
            _.patterns[_.pattern_count] = RequestPattern{
                pattern: pattern,
                resource_type: resource_type,
                request_stage: request_stage,
            };
            _.pattern_count += 1;
        }
    }

    fn clearPatterns(*_) {
        _.pattern_count = 0;
    }

    fn findParked(*_, request_id: []const u8): (*ParkedRequest)? {
        let i: usize = 0;
        while (i < _.parked_count) : (i += 1) {
            const p_id = if (_.parked[i].requestId.len > 0) _.parked[i].requestId else _.parked[i].request_id;
            if (std.mem.eql(u8, p_id, request_id)) {
                return &_.parked[i];
            }
        }
        return null;
    }

    fn findParkedByUrl(*_, url: []const u8): (*ParkedRequest)? {
        let i: usize = 0;
        while (i < _.parked_count) : (i += 1) {
            if (std.mem.eql(u8, _.parked[i].url, url)) {
                return &_.parked[i];
            }
        }
        return null;
    }

    fn getParked(*_, request_id: []const u8): (*const ParkedRequest)? {
        let i: usize = 0;
        while (i < _.parked_count) : (i += 1) {
            const p_id = if (_.parked[i].requestId.len > 0) _.parked[i].requestId else _.parked[i].request_id;
            if (std.mem.eql(u8, p_id, request_id)) {
                return &_.parked[i];
            }
        }
        return null;
    }

    fn registerParked(*_, request_id: []const u8, url: []const u8, method: []const u8, headers: []const u8, post_data: []const u8, resource_type: []const u8): (*ParkedRequest)? {
        if (existing := _.findParked(request_id)) {
            existing.url = url;
            existing.method = method;
            existing.headers = headers;
            existing.post_data = post_data;
            existing.postData = post_data;
            existing.resource_type = resource_type;
            existing.verdict = null;
            existing.is_paused = true;
            return existing;
        }

        let idx: usize = 0;
        if (_.parked_count >= _.parked.len) {
            // Evict oldest by shifting left
            let s: usize = 1;
            while (s < _.parked.len) {
                _.parked[s - 1] = _.parked[s];
                s += 1;
            }
            idx = _.parked.len - 1;
        } else {
            idx = _.parked_count;
            _.parked_count += 1;
        }

        _.parked[idx] = ParkedRequest{
            requestId: request_id,
            request_id: request_id,
            url: url,
            method: method,
            headers: headers,
            post_data: post_data,
            postData: post_data,
            resource_type: resource_type,
            verdict: null,
            is_paused: true,
        };
        return &_.parked[idx];
    }

    fn registerParkedRequest(*_, req: ParkedRequest): (*ParkedRequest)? {
        const id = if (req.requestId.len > 0) req.requestId else req.request_id;
        const post = if (req.post_data.len > 0) req.post_data else req.postData;
        return _.registerParked(id, req.url, req.method, req.headers, post, req.resource_type);
    }

    fn resolveParked(*_, request_id: []const u8, action: InterceptAction): bool {
        if (p := _.findParked(request_id)) {
            if (p.verdict != null) return false;
            p.verdict = action;
            p.is_paused = false;
            return true;
        }
        return false;
    }

    fn releaseAll(*_) {
        let i: usize = 0;
        while (i < _.parked_count) : (i += 1) {
            if (_.parked[i].verdict == null) {
                _.parked[i].verdict = InterceptAction{
                    kind: InterceptActionKind.Continue,
                };
            }
            _.parked[i].is_paused = false;
        }
    }

    fn releaseAllParked(*_) {
        _.releaseAll();
    }

    fn clearParked(*_) {
        _.parked_count = 0;
    }

    static fn errorReasonToNet(reason: []const u8): []const u8 {
        if (std.mem.eql(u8, reason, "Aborted")) return "net::ERR_ABORTED";
        if (std.mem.eql(u8, reason, "AccessDenied")) return "net::ERR_ACCESS_DENIED";
        if (std.mem.eql(u8, reason, "AddressUnreachable")) return "net::ERR_ADDRESS_UNREACHABLE";
        if (std.mem.eql(u8, reason, "BlockedByClient")) return "net::ERR_BLOCKED_BY_CLIENT";
        if (std.mem.eql(u8, reason, "BlockedByResponse")) return "net::ERR_BLOCKED_BY_RESPONSE";
        if (std.mem.eql(u8, reason, "ConnectionAborted")) return "net::ERR_CONNECTION_ABORTED";
        if (std.mem.eql(u8, reason, "ConnectionClosed")) return "net::ERR_CONNECTION_CLOSED";
        if (std.mem.eql(u8, reason, "ConnectionFailed")) return "net::ERR_CONNECTION_FAILED";
        if (std.mem.eql(u8, reason, "ConnectionRefused")) return "net::ERR_CONNECTION_REFUSED";
        if (std.mem.eql(u8, reason, "ConnectionReset")) return "net::ERR_CONNECTION_RESET";
        if (std.mem.eql(u8, reason, "InternetDisconnected")) return "net::ERR_INTERNET_DISCONNECTED";
        if (std.mem.eql(u8, reason, "NameNotResolved")) return "net::ERR_NAME_NOT_RESOLVED";
        if (std.mem.eql(u8, reason, "TimedOut")) return "net::ERR_TIMED_OUT";
        return "net::ERR_FAILED";
    }

    static fn formatRequestPausedEvent(buf: []u8, req: *const ParkedRequest): []const u8 {
        const id = if (req.requestId.len > 0) req.requestId else req.request_id;
        const post = if (req.post_data.len > 0) req.post_data else req.postData;
        const priority = if (std.mem.eql(u8, req.resource_type, "Document")) "VeryHigh" else "Medium";
        const hdrs = if (req.headers.len > 0 && req.headers[0] == '{') req.headers else "{}";

        if (post.len > 0) {
            return std.fmt.bufPrint(
                buf,
                "{{\"requestId\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"{s}\",\"headers\":{s},\"postData\":\"{s}\",\"initialPriority\":\"{s}\",\"referrerPolicy\":\"strict-origin-when-cross-origin\"}},\"resourceType\":\"{s}\"}}",
                .{ id, req.url, req.method, hdrs, post, priority, req.resource_type },
            ) catch "";
        } else {
            return std.fmt.bufPrint(
                buf,
                "{{\"requestId\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"{s}\",\"headers\":{s},\"initialPriority\":\"{s}\",\"referrerPolicy\":\"strict-origin-when-cross-origin\"}},\"resourceType\":\"{s}\"}}",
                .{ id, req.url, req.method, hdrs, priority, req.resource_type },
            ) catch "";
        }
    }
}

pub fn globMatches(pattern: []const u8, url: []const u8): bool {
    return FetchInterceptionState.globMatches(pattern, url);
}

pub fn errorReasonToNet(reason: []const u8): []const u8 {
    return FetchInterceptionState.errorReasonToNet(reason);
}

pub fn formatRequestPausedEvent(buf: []u8, req: *const ParkedRequest): []const u8 {
    return FetchInterceptionState.formatRequestPausedEvent(buf, req);
}
