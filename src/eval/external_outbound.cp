// Migrated from vendor/kitesurf/packages/engine/src/eval/external-outbound.ts
// Selects the host egress hook or default fetch for forwarded requests.

pub struct ExternalOutboundRequest {
    url: []const u8 = "",
    method: []const u8 = "GET",
    headers: []const u8 = "",
    body: []const u8 = "",
    redirect: []const u8 = "follow",
}

pub struct ExternalOutboundResponse {
    status: u32 = 200,
    status_text: []const u8 = "OK",
    headers: []const u8 = "",
    body: []const u8 = "",
    ok: bool = true,
}

pub type ExternalFetchFn = *const fn(req: *const ExternalOutboundRequest, user_data: *!?): ExternalOutboundResponse;

pub fn defaultExternalFetch(req: *const ExternalOutboundRequest): ExternalOutboundResponse {
    * = req;
    return ExternalOutboundResponse{
        status: 200,
        status_text: "OK",
        headers: "",
        body: "",
        ok: true,
    };
}

pub struct ExternalOutbound {
    fetch_fn: ExternalFetchFn? = null,
    user_data: *!? = null,

    fn fetch(*_, req: *const ExternalOutboundRequest): ExternalOutboundResponse {
        if (f := _.fetch_fn) {
            return f(req, _.user_data);
        }
        return defaultExternalFetch(req);
    }
}

pub struct HostOutboundConfig {
    custom_fetch: ExternalFetchFn? = null,
    user_data: *!? = null,
    allow_redirects: bool = true,
    max_redirects: u32 = 20,

    static fn init(): HostOutboundConfig {
        return HostOutboundConfig{};
    }

    static fn withFetch(fetch_fn: ExternalFetchFn, user_data: *!?): HostOutboundConfig {
        return HostOutboundConfig{
            custom_fetch: fetch_fn,
            user_data: user_data,
            allow_redirects: true,
            max_redirects: 20,
        };
    }

    fn isConfigured(): bool {
        return _.custom_fetch != null;
    }

    fn setCustomFetch(*_, fetch_fn: ExternalFetchFn?, user_data: *!?) {
        _.custom_fetch = fetch_fn;
        _.user_data = user_data;
    }

    fn setRedirectPolicy(*_, allow: bool, max_hops: u32) {
        _.allow_redirects = allow;
        _.max_redirects = max_hops;
    }

    fn shouldFollowRedirect(status: u32, hop_count: u32): bool {
        if (!_.allow_redirects) return false;
        if (hop_count >= _.max_redirects) return false;
        return (status == 301 || status == 302 || status == 303 || status == 307 || status == 308);
    }

    fn fetch(*_, req: *const ExternalOutboundRequest): ExternalOutboundResponse {
        if (f := _.custom_fetch) {
            return f(req, _.user_data);
        }
        return defaultExternalFetch(req);
    }
}

pub fn directExternalFetch(
    config: *const HostOutboundConfig?,
    req: *const ExternalOutboundRequest,
): ExternalOutboundResponse {
    if (config != null) {
        const cfg = config.?;
        if (f := cfg.custom_fetch) {
            return f(req, cfg.user_data);
        }
    }
    return defaultExternalFetch(req);
}
