// src/cdp/dispatch.cp
// CDP Command Dispatcher for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/dispatch.ts

import { CdpRequest, CdpResponse, CdpEvent } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { DomHandler } from "./dom.cp";
import { PageHandler } from "./page.cp";
import { RuntimeHandler } from "./runtime.cp";
import { TargetHandler } from "./target.cp";
import { InputHandler } from "./input.cp";
import { NetworkHandler } from "./network.cp";
import { EmulationHandler } from "./emulation.cp";
import { FetchHandler } from "./fetch.cp";
import { AccessibilityHandler } from "./accessibility.cp";
import { WebMcpHandler } from "./webmcp.cp";

pub struct CdpDispatcher {
    pub static fn dispatch(
        session: *CdpPageSession,
        req: CdpRequest,
        out_buf: []u8,
        out_events: *[4]CdpEvent,
        event_count: *usize,
    ): CdpResponse {
        event_count.* = 0;

        // 1. Browser domain methods
        if (std.mem.eql(u8, req.method, "Browser.getVersion")) {
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"protocolVersion\":\"1.3\",\"product\":\"CapSurf/1.0.0\",\"revision\":\"cp-native-2026\",\"userAgent\":\"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/145.0.0.0 Safari/537.36 CapSurf/1.0.0 QuickJS-NG\",\"jsVersion\":\"QuickJS-NG\"}}",
                .{},
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        // 2. Route by domain prefix
        if (std.mem.startsWith(u8, req.method, "DOM.")) {
            return DomHandler.handle(session, req, out_buf);
        }

        if (std.mem.startsWith(u8, req.method, "Page.")) {
            let page_evts: [4]CdpEvent = [CdpEvent{}; 4];
            let page_cnt: usize = 0;
            const resp = PageHandler.handle(session, req, out_buf, &page_evts, &page_cnt);
            let i: usize = 0;
            while (i < page_cnt && i < out_events.len) : (i += 1) {
                out_events[i] = page_evts[i];
            }
            event_count.* = @min(page_cnt, out_events.len);
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "Runtime.")) {
            let rt_evts: [1]CdpEvent = [CdpEvent{}; 1];
            let rt_cnt: usize = 0;
            const resp = RuntimeHandler.handle(session, req, out_buf, &rt_evts, &rt_cnt);
            if (rt_cnt > 0 && out_events.len > 0) {
                out_events[0] = rt_evts[0];
                event_count.* = 1;
            }
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "Target.")) {
            let tgt_evts: [1]CdpEvent = [CdpEvent{}; 1];
            let tgt_cnt: usize = 0;
            const resp = TargetHandler.handle(session, req, out_buf, &tgt_evts, &tgt_cnt);
            if (tgt_cnt > 0 && out_events.len > 0) {
                out_events[0] = tgt_evts[0];
                event_count.* = 1;
            }
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "Input.")) {
            return InputHandler.handle(session, req, out_buf);
        }

        if (std.mem.startsWith(u8, req.method, "Accessibility.")) {
            let ax_evts: [1]CdpEvent = [CdpEvent{}; 1];
            let ax_cnt: usize = 0;
            const resp = AccessibilityHandler.handle(session, req, out_buf, &ax_evts, &ax_cnt);
            if (ax_cnt > 0 && out_events.len > 0) {
                out_events[0] = ax_evts[0];
                event_count.* = 1;
            }
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "WebMCP.")) {
            let mcp_evts: [1]CdpEvent = [CdpEvent{}; 1];
            let mcp_cnt: usize = 0;
            const resp = WebMcpHandler.handle(session, req, out_buf, &mcp_evts, &mcp_cnt);
            if (mcp_cnt > 0 && out_events.len > 0) {
                out_events[0] = mcp_evts[0];
                event_count.* = 1;
            }
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "Network.") ||
            std.mem.startsWith(u8, req.method, "Storage.")) {
            return NetworkHandler.handle(session, req, out_buf);
        }

        if (std.mem.startsWith(u8, req.method, "Emulation.")) {
            return EmulationHandler.handle(session, req, out_buf);
        }

        if (std.mem.startsWith(u8, req.method, "Fetch.")) {
            return FetchHandler.handle(session, &session.fetch_state, req, out_buf);
        }

        // 3. Permissive stub domains (Puppeteer / Playwright handshake compatibility)
        if (std.mem.startsWith(u8, req.method, "Security.") ||
            std.mem.startsWith(u8, req.method, "Performance.") ||
            std.mem.startsWith(u8, req.method, "Log.")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.err(req.id, -32601, "Method not found");
    }
}
