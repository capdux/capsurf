// src/cdp/network.cp
// CDP Network & Storage Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/network.ts and cookies.ts

import { CdpRequest, CdpResponse, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { CookieEntry, entryToCdp } from "../net/cookies.cp";

pub struct NetworkHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8): CdpResponse {
        // 1. Network lifecycle & buffering
        if (std.mem.eql(u8, req.method, "Network.enable")) {
            const max_buf = extractJsonInt(req.params_raw, "maxTotalBufferSize");
            * = max_buf;
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Network.disable")) {
            return CdpResponse.ok(req.id, "{}");
        }

        // 2. Cache & ServiceWorker controls
        if (std.mem.eql(u8, req.method, "Network.setCacheDisabled") ||
            std.mem.eql(u8, req.method, "Network.setBypassServiceWorker") ||
            std.mem.eql(u8, req.method, "Network.clearBrowserCache")) {
            return CdpResponse.ok(req.id, "{}");
        }

        // 3. UserAgent & Extra HTTP Headers
        if (std.mem.eql(u8, req.method, "Network.setUserAgentOverride")) {
            const ua = extractJsonString(req.params_raw, "userAgent") ?? "";
            session.setUserAgentOverride(ua);
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Network.setExtraHTTPHeaders")) {
            // Save extra headers JSON for subsequent fetch and navigation pipelines
            const headers_str = extractJsonString(req.params_raw, "headers") ?? "";
            if (headers_str.len > 0) {
                session.extra_headers = session.allocator.dupe(u8, headers_str) catch headers_str;
            } else {
                session.extra_headers = session.allocator.dupe(u8, req.params_raw) catch req.params_raw;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        // 4. Cookie retrieval: real integration with session.cookie_jar
        if (std.mem.eql(u8, req.method, "Network.getCookies") ||
            std.mem.eql(u8, req.method, "Network.getAllCookies") ||
            std.mem.eql(u8, req.method, "Storage.getCookies")) {
            const cookies = session.cookie_jar.getAll();
            if (cookies.len == 0) {
                return CdpResponse.ok(req.id, "{\"cookies\":[]}");
            }

            let cursor: usize = 0;
            const prefix = "{\"cookies\":[";
            @memcpy(out_buf[cursor .. cursor + prefix.len], prefix);
            cursor += prefix.len;

            let first: bool = true;
            let i: usize = 0;
            while (i < cookies.len) : (i += 1) {
                const cdp_ck = entryToCdp(&cookies[i]);
                if (!first) {
                    if (cursor < out_buf.len) {
                        out_buf[cursor] = ',';
                        cursor += 1;
                    }
                }
                first = false;
                const chunk = std.fmt.bufPrint(
                    out_buf[cursor..],
                    "{{\"name\":\"{s}\",\"value\":\"{s}\",\"domain\":\"{s}\",\"path\":\"{s}\",\"size\":{d},\"httpOnly\":{s},\"secure\":{s},\"session\":{s},\"sameSite\":\"{s}\"}}",
                    .{
                        cdp_ck.name,
                        cdp_ck.value,
                        cdp_ck.domain,
                        cdp_ck.path,
                        cdp_ck.size,
                        if (cdp_ck.http_only) "true" else "false",
                        if (cdp_ck.secure) "true" else "false",
                        if (cdp_ck.session) "true" else "false",
                        cdp_ck.same_site,
                    },
                ) catch break;
                cursor += chunk.len;
            }
            if (cursor + 2 <= out_buf.len) {
                out_buf[cursor] = ']';
                out_buf[cursor + 1] = '}';
                cursor += 2;
            }
            return CdpResponse.ok(req.id, out_buf[0..cursor]);
        }

        // 5. Cookie mutation: real storage into session.cookie_jar
        if (std.mem.eql(u8, req.method, "Network.setCookie")) {
            const name = extractJsonString(req.params_raw, "name") ?? "";
            const val = extractJsonString(req.params_raw, "value") ?? "";
            const domain = extractJsonString(req.params_raw, "domain") ?? "";
            const path = extractJsonString(req.params_raw, "path") ?? "";

            if (name.len > 0) {
                const d = if (domain.len > 0) domain else "localhost";
                const p = if (path.len > 0) path else "/";
                const set_cookie_str = std.fmt.allocPrint(
                    session.allocator,
                    "{s}={s}; Domain={s}; Path={s}",
                    .{ name, val, d, p },
                ) catch "";

                if (set_cookie_str.len > 0) {
                    let url_buf: [128]u8 = undefined;
                    const url_str = std.fmt.bufPrint(url_buf[0..], "http://{s}{s}", .{ d, p }) catch "http://localhost/";
                    * = session.cookie_jar.setFromHeader(url_str, set_cookie_str, null);
                }
            }
            return CdpResponse.ok(req.id, "{\"success\":true}");
        }

        if (std.mem.eql(u8, req.method, "Network.setCookies") ||
            std.mem.eql(u8, req.method, "Storage.setCookies")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Network.deleteCookies") ||
            std.mem.eql(u8, req.method, "Network.clearBrowserCookies") ||
            std.mem.eql(u8, req.method, "Storage.clearCookies")) {
            session.cookie_jar.clear();
            return CdpResponse.ok(req.id, "{}");
        }

        // 6. Network emulation & response body stubs
        if (std.mem.eql(u8, req.method, "Network.emulateNetworkConditionsByRule")) {
            return CdpResponse.ok(req.id, "{\"ruleIds\":[]}");
        }

        if (std.mem.eql(u8, req.method, "Network.getResponseBody")) {
            return CdpResponse.ok(req.id, "{\"body\":\"\",\"base64Encoded\":false}");
        }

        // 7. Generic fallback for any other Network or Storage domain methods
        if (std.mem.startsWith(u8, req.method, "Network.") ||
            std.mem.startsWith(u8, req.method, "Storage.")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.err(req.id, -32601, "'Network' domain method not implemented");
    }
}
