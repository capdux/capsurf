// src/cdp/runtime.cp
// CDP Runtime Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/runtime.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, extractJsonBool, unescapeJsonString, normalizeExceptionMessage, MAX_CDP_EVENTS } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { emitPendingNavigationEvents } from "./input.cp";

pub struct RuntimeHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8, out_events: *[MAX_CDP_EVENTS]CdpEvent, event_count: *usize): CdpResponse {
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

            // Check for pending execution context events
            let polled_ctx_evts: [2]CdpEvent = [CdpEvent{}; 2];
            const polled_c_count = session.pollExecutionContextEvents(&polled_ctx_evts);
            let c_i: usize = 0;
            while (c_i < polled_c_count && event_count.* < out_events.len) : (c_i += 1) {
                out_events[event_count.*] = polled_ctx_evts[c_i];
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
            std.mem.eql(u8, req.method, "Runtime.setCustomObjectFormatterEnabled") ||
            std.mem.eql(u8, req.method, "Runtime.awaitPromise")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Runtime.removeBinding")) {
            const raw_name = extractJsonString(req.params_raw, "name") ?? "";
            let name_buf: [128]u8 = [0; 128];
            const name = unescapeJsonString(raw_name, &name_buf);
            if (name.len > 0) {
                session.removeBinding(name);
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Runtime.getHeapUsage")) {
            const res = session.formatHeapUsageJson(req.session_id, out_buf);
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Runtime.compileScript")) {
            const persist = extractJsonBool(req.params_raw, "persistScript") ?? true;
            if (!persist) {
                return CdpResponse.ok(req.id, "{}");
            }
            return CdpResponse.ok(req.id, "{\"scriptId\":\"capsurf-script-001\"}");
        }

        if (std.mem.eql(u8, req.method, "Runtime.runScript")) {
            return CdpResponse.ok(req.id, "{\"result\":{\"type\":\"undefined\"}}");
        }

        if (std.mem.eql(u8, req.method, "Runtime.queryObjects")) {
            return CdpResponse.ok(req.id, "{\"objects\":{\"type\":\"object\",\"objectId\":\"capsurf-query-obj\"}}");
        }

        if (std.mem.eql(u8, req.method, "Runtime.getProperties")) {
            const res = std.fmt.bufPrint(out_buf, "{{\"result\":[]}}", .{}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Runtime.callFunctionOn")) {
            const exec_ctx_id = extractJsonInt(req.params_raw, "executionContextId");
            if (exec_ctx_id != null && !session.isValidContextId(exec_ctx_id.?)) {
                return CdpResponse.err(req.id, -32000, "Cannot find context with specified id");
            }
            const object_id = extractJsonString(req.params_raw, "objectId") ?? "";
            const raw_decl = extractJsonString(req.params_raw, "functionDeclaration") ?? "";
            let decl_buf = session.allocator.alloc(u8, @max(raw_decl.len + 1, @as(usize, 65536))) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(decl_buf);
            const func_decl_len = unescapeJsonString(raw_decl, decl_buf).len;
            decl_buf[func_decl_len] = 0;
            const func_decl = decl_buf[0..func_decl_len];
            const return_by_value = extractJsonBool(req.params_raw, "returnByValue") ?? false;
            const await_promise = extractJsonBool(req.params_raw, "awaitPromise") ?? false;
            
            let res_len = session.callFunctionOn(object_id, func_decl, return_by_value, await_promise, out_buf);

            if (std.mem.indexOf(u8, out_buf[0..res_len], "cannot read property '") != null ||
                std.mem.indexOf(u8, out_buf[0..res_len], "cannot set property '") != null ||
                std.mem.indexOf(u8, out_buf[0..res_len], "Cannot read property '") != null ||
                std.mem.indexOf(u8, out_buf[0..res_len], "Cannot set property '") != null)
            {
                let norm_buf: [8192]u8 = [0; 8192];
                const norm_slice = normalizeExceptionMessage(out_buf[0..res_len], &norm_buf);
                if (norm_slice.len <= out_buf.len) {
                    @memcpy(out_buf[0..norm_slice.len], norm_slice);
                    res_len = norm_slice.len;
                }
            }
            
            // Poll any console events produced during callFunctionOn
            let polled_evts: [4]CdpEvent = [CdpEvent{}; 4];
            const polled_count = session.pollConsoleEvents(&polled_evts);
            let p_i: usize = 0;
            while (p_i < polled_count && event_count.* < out_events.len) : (p_i += 1) {
                out_events[event_count.*] = polled_evts[p_i];
                event_count.* += 1;
            }

            // Poll any CSS events produced during callFunctionOn
            if (session.css_enabled) {
                let polled_css_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_css_count = session.pollCssEvents(&polled_css_evts);
                let css_i: usize = 0;
                while (css_i < polled_css_count && event_count.* < out_events.len) : (css_i += 1) {
                    out_events[event_count.*] = polled_css_evts[css_i];
                    event_count.* += 1;
                }
            }

            if (res_len == 0) {
                return CdpResponse.ok(req.id, "{\"result\":{\"type\":\"undefined\"}}");
            }
            return CdpResponse.ok(req.id, out_buf[0..res_len]);
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
            const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (bctx.len > 0) {
                session.setContextId(bctx);
            }

            const raw_expr = extractJsonString(req.params_raw, "expression") ?? "undefined";
            let unesc_buf = session.allocator.alloc(u8, @max(raw_expr.len + 1, @as(usize, 65536))) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(unesc_buf);
            const expr_len = unescapeJsonString(raw_expr, unesc_buf).len;
            unesc_buf[expr_len] = 0;
            const expr = unesc_buf[0..expr_len];
            
            const return_by_value = extractJsonBool(req.params_raw, "returnByValue") ?? false;
            const await_promise = extractJsonBool(req.params_raw, "awaitPromise") ?? true;
            const context_id = extractJsonInt(req.params_raw, "contextId");
            if (context_id != null && !session.isValidContextId(context_id.?)) {
                return CdpResponse.err(req.id, -32000, "Cannot find context with specified id");
            }

            let eval_len = session.evaluateInContext(context_id, expr, return_by_value, await_promise, out_buf);

            if (std.mem.indexOf(u8, out_buf[0..eval_len], "cannot read property '") != null ||
                std.mem.indexOf(u8, out_buf[0..eval_len], "cannot set property '") != null ||
                std.mem.indexOf(u8, out_buf[0..eval_len], "Cannot read property '") != null ||
                std.mem.indexOf(u8, out_buf[0..eval_len], "Cannot set property '") != null)
            {
                let norm_buf: [8192]u8 = [0; 8192];
                const norm_slice = normalizeExceptionMessage(out_buf[0..eval_len], &norm_buf);
                if (norm_slice.len <= out_buf.len) {
                    @memcpy(out_buf[0..norm_slice.len], norm_slice);
                    eval_len = norm_slice.len;
                }
            }

            // Poll any network events produced during evaluation
            let polled_net_evts: [4]CdpEvent = [CdpEvent{}; 4];
            const polled_net_count = session.pollNetworkEvents(&polled_net_evts);
            let net_i: usize = 0;
            while (net_i < polled_net_count && event_count.* < out_events.len) : (net_i += 1) {
                out_events[event_count.*] = polled_net_evts[net_i];
                event_count.* += 1;
            }

            // Poll any console events produced during evaluation
            let polled_evts: [4]CdpEvent = [CdpEvent{}; 4];
            const polled_count = session.pollConsoleEvents(&polled_evts);
            let p_i: usize = 0;
            while (p_i < polled_count && event_count.* < out_events.len) : (p_i += 1) {
                out_events[event_count.*] = polled_evts[p_i];
                event_count.* += 1;
            }

            // Poll any document.open/write/close reopened load events (strictly after console events)
            let polled_reopen_evts: [4]CdpEvent = [CdpEvent{}; 4];
            const polled_reopen_count = session.pollReopenLoadEvents(&polled_reopen_evts);
            let r_i: usize = 0;
            while (r_i < polled_reopen_count && event_count.* < out_events.len) : (r_i += 1) {
                out_events[event_count.*] = polled_reopen_evts[r_i];
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

            // Poll any execution context created events (e.g. out-of-process iframe added)
            let polled_ctx_evts: [2]CdpEvent = [CdpEvent{}; 2];
            const polled_c_count = session.pollExecutionContextEvents(&polled_ctx_evts);
            let c_i: usize = 0;
            while (c_i < polled_c_count && event_count.* < out_events.len) : (c_i += 1) {
                out_events[event_count.*] = polled_ctx_evts[c_i];
                event_count.* += 1;
            }

            // Poll any CSS events produced during evaluation
            if (session.css_enabled) {
                let polled_css_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_css_count = session.pollCssEvents(&polled_css_evts);
                let css_i: usize = 0;
                while (css_i < polled_css_count && event_count.* < out_events.len) : (css_i += 1) {
                    out_events[event_count.*] = polled_css_evts[css_i];
                    event_count.* += 1;
                }
            }

            if (std.mem.indexOf(u8, expr, "__capsurf_dispatch_mouse_event") != null) {
                emitPendingNavigationEvents(session, out_events, event_count);
            }

            if (eval_len == 0) {
                return CdpResponse.ok(req.id, "{\"result\":{\"type\":\"undefined\"}}");
            }
            return CdpResponse.ok(req.id, out_buf[0..eval_len]);
        }

        return CdpResponse.err(req.id, -32601, "'Runtime' domain method not implemented");
    }
}
