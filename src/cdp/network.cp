// src/cdp/network.cp
// CDP Network & Storage Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/network.ts

import { CdpRequest, CdpResponse, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession } from "./session.cp";

pub struct NetworkHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8): CdpResponse {
        * = session;
        * = out_buf;

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
            const ua = extractJsonString(req.params_raw, "userAgent");
            * = ua;
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Network.setExtraHTTPHeaders")) {
            return CdpResponse.ok(req.id, "{}");
        }

        // 4. Cookie retrieval
        if (std.mem.eql(u8, req.method, "Network.getCookies") ||
            std.mem.eql(u8, req.method, "Network.getAllCookies") ||
            std.mem.eql(u8, req.method, "Storage.getCookies")) {
            return CdpResponse.ok(req.id, "{\"cookies\":[]}");
        }

        // 5. Cookie mutation
        if (std.mem.eql(u8, req.method, "Network.setCookie")) {
            return CdpResponse.ok(req.id, "{\"success\":true}");
        }

        if (std.mem.eql(u8, req.method, "Network.setCookies") ||
            std.mem.eql(u8, req.method, "Storage.setCookies")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Network.deleteCookies") ||
            std.mem.eql(u8, req.method, "Network.clearBrowserCookies") ||
            std.mem.eql(u8, req.method, "Storage.clearCookies")) {
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
