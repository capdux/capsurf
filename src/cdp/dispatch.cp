// src/cdp/dispatch.cp
// CDP Command Dispatcher for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/dispatch.ts

import { CdpRequest, CdpResponse, CdpEvent, MAX_CDP_EVENTS } from "./types.cp";
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
import { LogHandler } from "./log.cp";
import { DebuggerHandler } from "./debugger.cp";
import { CssHandler } from "./css.cp";
import { BrowserHandler } from "./browser.cp";

pub struct CdpDispatcher {
    pub static fn dispatch(
        session: *CdpPageSession,
        req: CdpRequest,
        out_buf: []u8,
        out_events: *[MAX_CDP_EVENTS]CdpEvent,
        event_count: *usize,
    ): CdpResponse {
        event_count.* = 0;

        // 0. Handle malformed envelope or parse errors
        if (req.hasEnvelopeError()) {
            if (req.raw_id.len > 0) {
                return CdpResponse.errWithRawId(req.raw_id, req.envelope_error_code, req.envelope_error_message);
            }
            return CdpResponse.err(req.id, req.envelope_error_code, req.envelope_error_message);
        }

        if (req.method.len == 0) {
            if (req.raw_id.len > 0) {
                return CdpResponse.errWithRawId(req.raw_id, -32600, "Invalid Request");
            }
            return CdpResponse.err(req.id, -32600, "Invalid Request");
        }

        // 1. Browser domain methods
        if (std.mem.startsWith(u8, req.method, "Browser.")) {
            return BrowserHandler.handle(session, req, out_buf);
        }

        // 2. Route by domain prefix
        if (std.mem.startsWith(u8, req.method, "DOM.")) {
            let dom_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            let dom_cnt: usize = 0;
            const resp = DomHandler.handle(session, req, out_buf, &dom_evts, &dom_cnt);
            let i: usize = 0;
            while (i < dom_cnt && i < out_events.len) : (i += 1) {
                out_events[i] = dom_evts[i];
            }
            event_count.* = @min(dom_cnt, out_events.len);
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "Page.")) {
            let page_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
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
            let rt_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            let rt_cnt: usize = 0;
            const resp = RuntimeHandler.handle(session, req, out_buf, &rt_evts, &rt_cnt);
            let i: usize = 0;
            while (i < rt_cnt && i < out_events.len) : (i += 1) {
                out_events[i] = rt_evts[i];
            }
            event_count.* = @min(rt_cnt, out_events.len);
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "Target.")) {
            let tgt_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            let tgt_cnt: usize = 0;
            const resp = TargetHandler.handle(session, req, out_buf, &tgt_evts, &tgt_cnt);
            let i: usize = 0;
            while (i < tgt_cnt && i < out_events.len) : (i += 1) {
                out_events[i] = tgt_evts[i];
            }
            event_count.* = @min(tgt_cnt, out_events.len);
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "Input.")) {
            let in_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            let in_cnt: usize = 0;
            const resp = InputHandler.handle(session, req, out_buf, &in_evts, &in_cnt);
            let i: usize = 0;
            while (i < in_cnt && i < out_events.len) : (i += 1) {
                out_events[i] = in_evts[i];
            }
            event_count.* = @min(in_cnt, out_events.len);
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "Accessibility.")) {
            let ax_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            let ax_cnt: usize = 0;
            const resp = AccessibilityHandler.handle(session, req, out_buf, &ax_evts, &ax_cnt);
            let i: usize = 0;
            while (i < ax_cnt && i < out_events.len) : (i += 1) {
                out_events[i] = ax_evts[i];
            }
            event_count.* = @min(ax_cnt, out_events.len);
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "WebMCP.")) {
            let mcp_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            let mcp_cnt: usize = 0;
            const resp = WebMcpHandler.handle(session, req, out_buf, &mcp_evts, &mcp_cnt);
            let mi: usize = 0;
            while (mi < mcp_cnt && mi < out_events.len) : (mi += 1) {
                out_events[mi] = mcp_evts[mi];
            }
            event_count.* = @min(mcp_cnt, out_events.len);
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

        if (std.mem.startsWith(u8, req.method, "Log.")) {
            let log_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            let log_cnt: usize = 0;
            const resp = LogHandler.handle(session, req, out_buf, &log_evts, &log_cnt);
            let i: usize = 0;
            while (i < log_cnt && i < out_events.len) : (i += 1) {
                out_events[i] = log_evts[i];
            }
            event_count.* = @min(log_cnt, out_events.len);
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "Debugger.")) {
            let dbg_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            let dbg_cnt: usize = 0;
            const resp = DebuggerHandler.handle(session, &session.debugger_state, req, out_buf, &dbg_evts, &dbg_cnt);
            let i: usize = 0;
            while (i < dbg_cnt && i < out_events.len) : (i += 1) {
                out_events[i] = dbg_evts[i];
            }
            event_count.* = @min(dbg_cnt, out_events.len);
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "CSS.")) {
            let css_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            let css_cnt: usize = 0;
            const resp = CssHandler.handle(session, req, out_buf, &css_evts, &css_cnt);
            let i: usize = 0;
            while (i < css_cnt && i < out_events.len) : (i += 1) {
                out_events[i] = css_evts[i];
            }
            event_count.* = @min(css_cnt, out_events.len);
            return resp;
        }

        if (std.mem.startsWith(u8, req.method, "Kitesurf.") ||
            std.mem.startsWith(u8, req.method, "CapSurf.")) {
            if (res := session.facade.dispatchCommand(req.method, req.params_raw, session, out_buf)) {
                return CdpResponse.ok(req.id, res);
            }
            return CdpResponse.ok(req.id, "{}");
        }

        // 3. Permissive stub domains (Puppeteer / Playwright / DevTools probe compatibility)
        if (std.mem.startsWith(u8, req.method, "Security.") ||
            std.mem.startsWith(u8, req.method, "Performance.") ||
            std.mem.startsWith(u8, req.method, "PerformanceTimeline.") ||
            std.mem.startsWith(u8, req.method, "Animation.") ||
            std.mem.startsWith(u8, req.method, "Audits.") ||
            std.mem.startsWith(u8, req.method, "Autofill.") ||
            std.mem.startsWith(u8, req.method, "BackgroundService.") ||
            std.mem.startsWith(u8, req.method, "Cast.") ||
            std.mem.startsWith(u8, req.method, "Console.") ||
            std.mem.startsWith(u8, req.method, "Database.") ||
            std.mem.startsWith(u8, req.method, "DeviceOrientation.") ||
            std.mem.startsWith(u8, req.method, "DOMDebugger.") ||
            std.mem.startsWith(u8, req.method, "DOMSnapshot.") ||
            std.mem.startsWith(u8, req.method, "DOMStorage.") ||
            std.mem.startsWith(u8, req.method, "FedCm.") ||
            std.mem.startsWith(u8, req.method, "Headless.") ||
            std.mem.startsWith(u8, req.method, "HeadlessExperimental.") ||
            std.mem.startsWith(u8, req.method, "HeapProfiler.") ||
            std.mem.startsWith(u8, req.method, "IndexedDB.") ||
            std.mem.startsWith(u8, req.method, "Inspector.") ||
            std.mem.startsWith(u8, req.method, "IO.") ||
            std.mem.startsWith(u8, req.method, "LayerTree.") ||
            std.mem.startsWith(u8, req.method, "Media.") ||
            std.mem.startsWith(u8, req.method, "Memory.") ||
            std.mem.startsWith(u8, req.method, "Overlay.") ||
            std.mem.startsWith(u8, req.method, "Page-CSP.") ||
            std.mem.startsWith(u8, req.method, "Preload.") ||
            std.mem.startsWith(u8, req.method, "Profiler.") ||
            std.mem.startsWith(u8, req.method, "Schema.") ||
            std.mem.startsWith(u8, req.method, "ServiceWorker.") ||
            std.mem.startsWith(u8, req.method, "SystemInfo.") ||
            std.mem.startsWith(u8, req.method, "Tracing.") ||
            std.mem.startsWith(u8, req.method, "WebAudio.") ||
            std.mem.startsWith(u8, req.method, "WebAuthn.")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.err(req.id, -32601, "Method not found");
    }
}
