// src/cdp/runtime.cp
// CDP Runtime Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/runtime.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession } from "./session.cp";

pub struct RuntimeHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8, out_events: *[4]CdpEvent, event_count: *usize): CdpResponse {
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "Runtime.enable")) {
            session.runtime_enabled = true;
            let origin = session.url;
            if (origin.len == 0) origin = "about:blank";

            const ctx_json = session.formatExecutionContextEvent(origin);

            out_events[0] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
            event_count.* = 1;

            // Check for pending console events
            let polled_evts: [3]CdpEvent = [CdpEvent{}; 3];
            const polled_count = session.pollConsoleEvents(&polled_evts);
            let p_i: usize = 0;
            while (p_i < polled_count && event_count.* < out_events.len) : (p_i += 1) {
                out_events[event_count.*] = polled_evts[p_i];
                event_count.* += 1;
            }

            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Runtime.disable")) {
            session.runtime_enabled = false;
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Runtime.releaseObject") ||
            std.mem.eql(u8, req.method, "Runtime.releaseObjectGroup") ||
            std.mem.eql(u8, req.method, "Runtime.discardConsoleEntries") ||
            std.mem.eql(u8, req.method, "Runtime.runIfWaitingForDebugger") ||
            std.mem.eql(u8, req.method, "Runtime.setAsyncCallStackDepth") ||
            std.mem.eql(u8, req.method, "Runtime.setCustomObjectFormatterEnabled")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Runtime.getProperties")) {
            const res = std.fmt.bufPrint(out_buf, "{{\"result\":[]}}", .{}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Runtime.callFunctionOn")) {
            const res = std.fmt.bufPrint(out_buf, "{{\"result\":{{\"type\":\"undefined\"}}}}", .{}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Runtime.evaluate")) {
            const expr = extractJsonString(req.params_raw, "expression") ?? "undefined";
            let eval_res_buf: [2048]u8 = [0; 2048];
            const eval_len = session.evaluate(expr, &eval_res_buf);
            const eval_slice = eval_res_buf[0..eval_len];

            // Poll any console events produced during evaluation
            let polled_evts: [4]CdpEvent = [CdpEvent{}; 4];
            const polled_count = session.pollConsoleEvents(&polled_evts);
            let p_i: usize = 0;
            while (p_i < polled_count && event_count.* < out_events.len) : (p_i += 1) {
                out_events[event_count.*] = polled_evts[p_i];
                event_count.* += 1;
            }

            const res = std.fmt.bufPrint(
                out_buf,
                "{s}",
                .{eval_slice},
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        return CdpResponse.err(req.id, -32601, "'Runtime' domain method not implemented");
    }
}
