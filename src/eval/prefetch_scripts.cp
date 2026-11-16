// Migrated from vendor/kitesurf/packages/engine/src/eval/prefetch-scripts.ts
// Fetches external classic scripts before navigation and checks their script policy.

import { CredentialsMode, scriptCredentialsMode } from "../net/cors.cp";
import { isCrossOrigin, originOfUrl } from "../net/redirect_walk.cp";
import { documentBaseUrl, resolveScriptUrl, checkScriptPolicy, GatedScriptDecision } from "./script_gate.cp";
import { HostOutboundConfig, ExternalOutboundRequest, ExternalOutboundResponse, directExternalFetch } from "./external_outbound.cp";
import { getSystemMilliTimestamp } from "./host_clock.cp";

pub const PREFETCH_TIMEOUT_MS: u64 = 10000;
pub const MAX_PREFETCH_SCRIPTS: usize = 32;
pub const MAX_SCRIPT_BLOCKED_MESSAGE_BYTES: usize = 4096;
pub const DEFAULT_PREFETCH_CONCURRENCY: usize = 4;

pub const HEADER_RESOURCE_TYPE: []const u8 = "x-kitesurf-resource-type";
pub const HEADER_INITIATOR: []const u8 = "x-kitesurf-initiator";
pub const HEADER_PAGE_ORIGIN: []const u8 = "x-kitesurf-page-origin";
pub const HEADER_ALLOW_CROSS_ORIGIN: []const u8 = "x-kitesurf-allow-cross-origin";
pub const HEADER_SCRIPT_CORS: []const u8 = "x-kitesurf-script-cors";
pub const HEADER_CREDENTIALS: []const u8 = "x-kitesurf-credentials";
pub const HEADER_CONNECTION_ID: []const u8 = "x-kitesurf-connection-id";
pub const HEADER_FRAME_ID: []const u8 = "x-kitesurf-frame-id";
pub const HEADER_BLOCKED: []const u8 = "x-kitesurf-blocked";

pub enum OriginClassification {
    Synthetic,
    SameOrigin,
    CrossOrigin,
    Invalid,
}

pub fn classifyScriptOrigin(target_url: []const u8, page_url: []const u8): OriginClassification {
    if (page_url.len == 0) return OriginClassification.Invalid;
    if (std.mem.indexOf(u8, page_url, "kitesurf.local") != null) {
        return OriginClassification.Synthetic;
    }
    if (isCrossOrigin(page_url, target_url)) {
        return OriginClassification.CrossOrigin;
    }
    return OriginClassification.SameOrigin;
}

pub fn credentialsModeToString(mode: CredentialsMode): []const u8 {
    return switch (mode) {
        .Omit => "omit",
        .SameOrigin => "same-origin",
        .Include => "include",
    };
}

pub struct ScriptPrefetchEntry {
    src: []const u8 = "",
    resolved_url: []const u8 = "",
    body: []const u8 = "",
    is_module: bool = false,
    status: u32 = 0,
    blocked: bool = false,
    reason: []const u8 = "",

    cross_origin: []const u8 = "",
    nonce: []const u8 = "",
    integrity: []const u8 = "",
    credentials_mode: CredentialsMode = CredentialsMode.SameOrigin,
    url_buf: [1024]u8 = [0; 1024],
}

pub type ScriptFetchCallback = *const fn(
    url: []const u8,
    credentials_mode: CredentialsMode,
    user_data: *!?,
    out_body: *[]const u8,
    out_status: *u32,
): bool;

pub struct PrefetchScriptManager {
    page_url: []const u8 = "",
    base_href: []const u8 = "",
    base_url_buf: [1024]u8 = [0; 1024],
    effective_base: []const u8 = "",
    csp_policy: []const u8 = "",
    connection_id: []const u8 = "",
    frame_id: []const u8 = "",
    timeout_ms: u64 = PREFETCH_TIMEOUT_MS,
    concurrency: usize = DEFAULT_PREFETCH_CONCURRENCY,

    scripts: [MAX_PREFETCH_SCRIPTS]ScriptPrefetchEntry = [ScriptPrefetchEntry{}; MAX_PREFETCH_SCRIPTS],
    script_count: usize = 0,

    outbound_config: HostOutboundConfig? = null,
    custom_fetch: ScriptFetchCallback? = null,
    user_data: *!? = null,

    static fn init(page_url: []const u8, base_href: []const u8): PrefetchScriptManager {
        return PrefetchScriptManager{
            page_url: page_url,
            base_href: base_href,
            effective_base: page_url,
            timeout_ms: PREFETCH_TIMEOUT_MS,
            concurrency: DEFAULT_PREFETCH_CONCURRENCY,
        };
    }

    fn initEffectiveBase(*_) {
        if (_.base_href.len > 0) {
            const b = documentBaseUrl(_.page_url, _.base_href, _.base_url_buf[0..]);
            if (b.len > 0) {
                _.effective_base = b;
                return;
            }
        }
        _.effective_base = _.page_url;
    }

    fn setBaseHref(*_, base_href: []const u8) {
        _.base_href = base_href;
        _.initEffectiveBase();
    }

    fn setCspPolicy(*_, policy: []const u8) {
        _.csp_policy = policy;
    }

    fn setConnectionInfo(*_, connection_id: []const u8, frame_id: []const u8) {
        _.connection_id = connection_id;
        _.frame_id = frame_id;
    }

    fn setTimeoutMs(*_, timeout_ms: u64) {
        _.timeout_ms = if (timeout_ms == 0) PREFETCH_TIMEOUT_MS else timeout_ms;
    }

    fn setConcurrency(*_, concurrency: usize) {
        _.concurrency = if (concurrency == 0) 1 else concurrency;
    }

    fn setOutboundConfig(*_, config: HostOutboundConfig) {
        _.outbound_config = config;
    }

    fn setFetchCallback(*_, cb: ScriptFetchCallback, user_data: *!?) {
        _.custom_fetch = cb;
        _.user_data = user_data;
    }

    fn addScript(
        *_,
        src: []const u8,
        is_module: bool,
        cross_origin: []const u8,
        nonce: []const u8,
        integrity: []const u8,
    ): bool {
        if (_.script_count >= MAX_PREFETCH_SCRIPTS) return false;
        if (src.len == 0) return false;

        if (_.effective_base.len == 0 || (_.base_href.len > 0 && std.mem.eql(u8, _.effective_base, _.page_url))) {
            _.initEffectiveBase();
        }

        const idx = _.script_count;
        _.scripts[idx].src = src;
        _.scripts[idx].is_module = is_module;
        _.scripts[idx].cross_origin = cross_origin;
        _.scripts[idx].nonce = nonce;
        _.scripts[idx].integrity = integrity;
        _.scripts[idx].status = 0;
        _.scripts[idx].blocked = false;
        _.scripts[idx].reason = "";
        _.scripts[idx].body = "";

        const co_opt: ([]const u8)? = if (cross_origin.len > 0) cross_origin else null;
        _.scripts[idx].credentials_mode = scriptCredentialsMode(co_opt);

        const resolved = resolveScriptUrl(src, _.effective_base, _.scripts[idx].url_buf[0..]);
        _.scripts[idx].resolved_url = resolved;

        if (resolved.len == 0) {
            _.scripts[idx].blocked = true;
            _.scripts[idx].reason = "could not resolve src against page URL";
            _.scripts[idx].status = 400;
        }

        _.script_count += 1;
        return true;
    }

    fn collectScript(*_, src: []const u8, is_module: bool): bool {
        return _.addScript(src, is_module, "", "", "");
    }

    fn getScripts(): []const ScriptPrefetchEntry {
        return _.scripts[0.._.script_count];
    }

    fn getScript(index: usize): (*const ScriptPrefetchEntry)? {
        if (index >= _.script_count) return null;
        return &_.scripts[index];
    }

    fn getScriptMut(*_, index: usize): (*ScriptPrefetchEntry)? {
        if (index >= _.script_count) return null;
        return &_.scripts[index];
    }

    fn getCount(): usize {
        return _.script_count;
    }

    fn isTimedOut(start_time: f64): bool {
        const now = getSystemMilliTimestamp();
        const elapsed = now - start_time;
        return elapsed >= @as(f64, @floatFromInt(_.timeout_ms));
    }

    fn prefetchSingle(*_, index: usize, start_time: f64): bool {
        if (index >= _.script_count) return false;

        if (_.isTimedOut(start_time)) {
            _.scripts[index].blocked = true;
            _.scripts[index].reason = "timeout";
            _.scripts[index].status = 408;
            return false;
        }

        if (_.scripts[index].blocked) {
            return false;
        }

        if (_.scripts[index].is_module) {
            return false;
        }

        const resolved = _.scripts[index].resolved_url;
        if (resolved.len == 0) {
            _.scripts[index].blocked = true;
            _.scripts[index].reason = "could not resolve src against page URL";
            _.scripts[index].status = 400;
            return false;
        }

        if (_.csp_policy.len > 0) {
            const decision = checkScriptPolicy(
                resolved,
                _.scripts[index].nonce,
                _.scripts[index].integrity,
                _.csp_policy,
            );
            if (!decision.allowed) {
                _.scripts[index].blocked = true;
                _.scripts[index].reason = if (decision.reason.len > 0) decision.reason else "csp";
                _.scripts[index].status = 403;
                return false;
            }
        }

        if (f := _.custom_fetch) {
            let body_out: []const u8 = "";
            let status_out: u32 = 0;
            const ok = f(
                resolved,
                _.scripts[index].credentials_mode,
                _.user_data,
                &body_out,
                &status_out,
            );
            _.scripts[index].body = body_out;
            _.scripts[index].status = status_out;
            if (!ok) {
                if (_.scripts[index].status == 0) {
                    _.scripts[index].status = 500;
                }
                _.scripts[index].reason = "fetch failed";
                return false;
            }
            return true;
        }

        if (_.outbound_config != null) {
            const req = ExternalOutboundRequest{
                url: resolved,
                method: "GET",
                redirect: "follow",
            };
            const resp = directExternalFetch(&_.outbound_config, &req);
            _.scripts[index].status = resp.status;
            _.scripts[index].body = resp.body;

            if (resp.status == 403 && (std.mem.indexOf(u8, resp.headers, "x-kitesurf-blocked") != null || std.mem.indexOf(u8, resp.body, "renderer egress denied") != null)) {
                _.scripts[index].blocked = true;
                _.scripts[index].reason = "renderer-no-egress";
                return false;
            }

            if (!resp.ok || resp.status >= 400) {
                _.scripts[index].reason = "fetch returned error status";
                return false;
            }

            return true;
        }

        _.scripts[index].status = 200;
        return true;
    }

    fn prefetchAll(*_): usize {
        if (_.script_count == 0) return 0;

        const start_time = getSystemMilliTimestamp();
        let success_count: usize = 0;
        let next: usize = 0;
        const eff_concurrency = if (_.concurrency == 0) 1 else @min(_.concurrency, _.script_count);

        while (!_.isTimedOut(start_time) && next < _.script_count) {
            let lane: usize = 0;
            while (lane < eff_concurrency && !_.isTimedOut(start_time) && next < _.script_count) : (lane += 1) {
                const idx = next;
                next += 1;
                if (_.prefetchSingle(idx, start_time)) {
                    success_count += 1;
                }
            }
        }

        while (next < _.script_count) : (next += 1) {
            if (!_.scripts[next].blocked && _.scripts[next].status == 0) {
                _.scripts[next].blocked = true;
                _.scripts[next].reason = "timeout";
                _.scripts[next].status = 408;
            }
        }

        return success_count;
    }
}
