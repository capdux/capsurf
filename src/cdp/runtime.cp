// src/cdp/runtime.cp
// CDP Runtime Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/runtime.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, extractJsonBool } from "./types.cp";
import { CdpPageSession } from "./session.cp";

fn unescapeJsonString(src: []const u8, dst: []u8): []const u8 {
    let out_idx: usize = 0;
    let i: usize = 0;
    while (i < src.len && out_idx < dst.len) {
        if (src[i] == '\\' && i + 1 < src.len) {
            const next = src[i + 1];
            if (next == 'n') {
                dst[out_idx] = '\n';
                out_idx += 1;
                i += 2;
            } else if (next == 'r') {
                dst[out_idx] = '\r';
                out_idx += 1;
                i += 2;
            } else if (next == 't') {
                dst[out_idx] = '\t';
                out_idx += 1;
                i += 2;
            } else if (next == '"') {
                dst[out_idx] = '"';
                out_idx += 1;
                i += 2;
            } else if (next == '\\') {
                dst[out_idx] = '\\';
                out_idx += 1;
                i += 2;
            } else if (next == '/') {
                dst[out_idx] = '/';
                out_idx += 1;
                i += 2;
            } else {
                dst[out_idx] = src[i];
                out_idx += 1;
                i += 1;
            }
        } else {
            dst[out_idx] = src[i];
            out_idx += 1;
            i += 1;
        }
    }
    return dst[0..out_idx];
}

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
            const object_id = extractJsonString(req.params_raw, "objectId") ?? "";
            const func_decl = extractJsonString(req.params_raw, "functionDeclaration") ?? "";
            const return_by_value = extractJsonBool(req.params_raw, "returnByValue") ?? false;
            const await_promise = extractJsonBool(req.params_raw, "awaitPromise") ?? false;
            
            let res_buf: [4096]u8 = [0; 4096];
            const res_len = session.callFunctionOn(object_id, func_decl, return_by_value, await_promise, &res_buf);
            const res_slice = res_buf[0..res_len];
            
            // Poll any console events produced during callFunctionOn
            let polled_evts: [4]CdpEvent = [CdpEvent{}; 4];
            const polled_count = session.pollConsoleEvents(&polled_evts);
            let p_i: usize = 0;
            while (p_i < polled_count && event_count.* < out_events.len) : (p_i += 1) {
                out_events[event_count.*] = polled_evts[p_i];
                event_count.* += 1;
            }
            
            return CdpResponse.ok(req.id, res_slice);
        }

        if (std.mem.eql(u8, req.method, "Runtime.addBinding")) {
            const raw_name = extractJsonString(req.params_raw, "name") ?? "";
            let name_buf: [128]u8 = [0; 128];
            const name = unescapeJsonString(raw_name, &name_buf);
            if (name.len > 0) {
                session.addBinding(name);
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Runtime.evaluate")) {
            const raw_expr = extractJsonString(req.params_raw, "expression") ?? "undefined";
            let unesc_buf: [4096]u8 = [0; 4096];
            const expr = unescapeJsonString(raw_expr, &unesc_buf);
            
            const return_by_value = extractJsonBool(req.params_raw, "returnByValue") ?? false;
            const await_promise = extractJsonBool(req.params_raw, "awaitPromise") ?? true;

            let eval_res_buf: [4096]u8 = [0; 4096];
            const eval_len = session.evaluateWithOptions(expr, return_by_value, await_promise, &eval_res_buf);
            const eval_slice = eval_res_buf[0..eval_len];

            // Poll any console events produced during evaluation
            let polled_evts: [4]CdpEvent = [CdpEvent{}; 4];
            const polled_count = session.pollConsoleEvents(&polled_evts);
            let p_i: usize = 0;
            while (p_i < polled_count && event_count.* < out_events.len) : (p_i += 1) {
                out_events[event_count.*] = polled_evts[p_i];
                event_count.* += 1;
            }

            // Poll any binding events produced during evaluation
            let polled_binding_evts: [4]CdpEvent = [CdpEvent{}; 4];
            const polled_b_count = session.pollBindingEvents(&polled_binding_evts);
            let b_i: usize = 0;
            while (b_i < polled_b_count && event_count.* < out_events.len) : (b_i += 1) {
                out_events[event_count.*] = polled_binding_evts[b_i];
                event_count.* += 1;
            }

            // Poll any same-document navigation events produced during evaluation
            let polled_nav_evts: [4]CdpEvent = [CdpEvent{}; 4];
            const polled_n_count = session.pollNavigatedWithinDocEvents(&polled_nav_evts);
            let n_i: usize = 0;
            while (n_i < polled_n_count && event_count.* < out_events.len) : (n_i += 1) {
                out_events[event_count.*] = polled_nav_evts[n_i];
                event_count.* += 1;
            }

            return CdpResponse.ok(req.id, eval_slice);
        }

        return CdpResponse.err(req.id, -32601, "'Runtime' domain method not implemented");
    }
}
