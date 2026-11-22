// src/cdp/runtime.cp
// CDP Runtime Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/runtime.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, extractJsonBool, extractJsonArray, unescapeJsonString, normalizeExceptionMessage, MAX_CDP_EVENTS } from "./types.cp";
import { CdpPageSession, escapeJsonString } from "./session.cp";
import { emitPendingNavigationEvents } from "./input.cp";
import { emitNewChildFrameEvents } from "./page.cp";
import { originOf } from "./frames.cp";
import { JS_Eval, jsIsException } from "../js/qjs_engine.cp";
import { makeCompiledScriptMeta, emitScriptParsedEvent } from "./debugger.cp";

pub struct RuntimeHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8, out_events: *[MAX_CDP_EVENTS]CdpEvent, event_count: *usize): CdpResponse {
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "Runtime.enable")) {
            session.runtime_enabled = true;
            if (req.session_id.len > 0 && !std.mem.eql(u8, req.session_id, "capsurf-session-001")) {
                if (iframe_fid := session.target_registry.findFrameIdBySessionId(req.session_id)) {
                    let ki: usize = 0;
                    while (ki < session.known_child_frame_count) : (ki += 1) {
                        let kf = &session.known_child_frames[ki];
                        if (!kf.active || kf.detached) continue;
                        const k_fid = kf.frame_id_buf[0..kf.frame_id_len];
                        if (std.mem.eql(u8, k_fid, iframe_fid)) {
                            if (kf.exec_context_id <= 0) {
                                kf.exec_context_id = session.next_context_id;
                                session.next_context_id += 1;
                            }
                            const matched_cid = kf.exec_context_id;
                            const k_url = kf.url_buf[0..kf.url_len];
                            let matched_origin: []const u8 = "about:blank";
                            if (kf.is_opaque) {
                                matched_origin = "://";
                            } else if (k_url.len > 0) {
                                matched_origin = originOf(k_url);
                            }
                            let esc_orig_buf: [256]u8 = [0; 256];
                            const esc_orig = escapeJsonString(matched_origin, &esc_orig_buf);
                            const ctx_json = std.fmt.bufPrint(
                                &session.iframe_ctx_event_buf,
                                "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"uniqueId\":\"ctx-{d}\",\"auxData\":{{\"isDefault\":false,\"type\":\"default\",\"frameId\":\"{s}\"}}}}}}",
                                .{ matched_cid, esc_orig, matched_cid, iframe_fid },
                            ) catch "{}";
                            out_events[0] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
                            event_count.* = 1;
                            return CdpResponse.ok(req.id, "{}");
                        }
                    }
                    * = session.ensureIframeContext();
                    const cid = session.iframe_ctx_id;
                    const origin_str = if (session.iframe_origin_len > 0)
                        session.iframe_origin_buf[0..session.iframe_origin_len]
                    else
                        "about:blank";
                    const ctx_json = std.fmt.bufPrint(
                        &session.iframe_ctx_event_buf,
                        "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"uniqueId\":\"ctx-{d}\",\"auxData\":{{\"isDefault\":false,\"type\":\"default\",\"frameId\":\"{s}\"}}}}}}",
                        .{ cid, origin_str, cid, iframe_fid },
                    ) catch "{}";
                    out_events[0] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
                    event_count.* = 1;
                    session.pending_iframe_ctx_event = false;
                    return CdpResponse.ok(req.id, "{}");
                }
            }
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

            // Re-emit existing active child frame execution contexts on Runtime.enable
            let kci: usize = 0;
            while (kci < session.known_child_frame_count && event_count.* < out_events.len) : (kci += 1) {
                const kf = &session.known_child_frames[kci];
                if (!kf.active || kf.detached || kf.exec_context_id <= 0) continue;
                const kf_url = kf.url_buf[0..kf.url_len];
                const kf_orig: []const u8 = if (kf.is_opaque) "://" else originOf(kf_url);
                let esc_orig_buf: [256]u8 = [0; 256];
                const esc_orig = escapeJsonString(kf_orig, &esc_orig_buf);
                const kf_ctx_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"auxData\":{{\"isDefault\":false,\"frameId\":\"{s}\"}}}}}}",
                    .{ kf.exec_context_id, esc_orig, kf.frame_id_buf[0..kf.frame_id_len] },
                ) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", kf_ctx_json);
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
            const raw_expr = extractJsonString(req.params_raw, "expression") ?? "";
            let expr_buf = session.allocator.alloc(u8, @max(raw_expr.len + 1, @as(usize, 65536))) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(expr_buf);
            const expr_len = unescapeJsonString(raw_expr, expr_buf).len;
            expr_buf[expr_len] = 0;
            const expr = expr_buf[0..expr_len];

            const raw_url = extractJsonString(req.params_raw, "sourceURL") ?? "";
            let url_buf: [1024]u8 = [0; 1024];
            const url_slice = unescapeJsonString(raw_url, url_buf[0 .. url_buf.len - 1]);
            url_buf[url_slice.len] = 0;
            const fname: [*c]const u8 = if (url_slice.len > 0) unsafe { @ptrCast(&url_buf) } else "<compileScript>";

            const persist = extractJsonBool(req.params_raw, "persistScript") ?? false;

            const compiled = JS_Eval(session.ctx.raw_ctx, expr_buf.ptr, expr_len, fname, 32);
            if (jsIsException(compiled)) {
                const exc = session.ctx.getException();
                const exc_str = session.ctx.toCString(exc);
                defer std.heap.page_allocator.free(exc_str);
                session.ctx.freeValue(exc);
                let esc_buf: [1024]u8 = [0; 1024];
                const esc_msg = escapeJsonString(exc_str, &esc_buf);
                const err_res = std.fmt.bufPrint(
                    out_buf,
                    "{{\"exceptionDetails\":{{\"text\":\"Uncaught\",\"exception\":{{\"type\":\"object\",\"subtype\":\"error\",\"className\":\"Error\",\"description\":\"{s}\"}}}}}}",
                    .{esc_msg},
                ) catch "{\"exceptionDetails\":{\"text\":\"Uncaught\",\"exception\":{\"type\":\"object\",\"subtype\":\"error\",\"className\":\"Error\",\"description\":\"SyntaxError\"}}}";
                return CdpResponse.ok(req.id, err_res);
            }
            session.ctx.freeValue(compiled);

            if (!persist) {
                return CdpResponse.ok(req.id, "{}");
            }

            let sid_buf: [32]u8 = [0; 32];
            const sid_num = session.debugger_state.next_script_id;
            session.debugger_state.next_script_id += 1;
            const sid = std.fmt.bufPrint(&sid_buf, "cs:{d}", .{sid_num}) catch "cs:1";

            let hash_buf: [8]u8 = [0; 8];
            const meta = makeCompiledScriptMeta(sid, expr, url_slice, false, &hash_buf);
            * = session.debugger_state.addScript(meta);

            if (session.debugger_state.enabled && event_count.* < out_events.len) {
                if (stored := session.debugger_state.getScript(sid)) {
                    out_events[event_count.*] = emitScriptParsedEvent(&session.debugger_state.event_buf, stored, session.frame_id);
                    event_count.* += 1;
                }
            }

            const res = std.fmt.bufPrint(out_buf, "{{\"scriptId\":\"{s}\"}}", .{sid}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Runtime.runScript")) {
            const raw_sid = extractJsonString(req.params_raw, "scriptId") ?? "";
            let sid_buf: [128]u8 = [0; 128];
            const sid = unescapeJsonString(raw_sid, &sid_buf);
            const return_by_value = extractJsonBool(req.params_raw, "returnByValue") ?? false;
            const await_promise = extractJsonBool(req.params_raw, "awaitPromise") ?? true;

            const script_meta = session.debugger_state.getScript(sid);
            if (script_meta == null) {
                return CdpResponse.ok(req.id, "{\"result\":{\"type\":\"undefined\"},\"exceptionDetails\":{\"text\":\"Uncaught\",\"exception\":{\"type\":\"object\",\"subtype\":\"error\",\"className\":\"Error\",\"description\":\"Error: Cannot find script with specified id\"}}}");
            }

            const eval_len = session.evaluateInContext(null, script_meta.?.source, return_by_value, await_promise, out_buf);

            let polled_evts: [4]CdpEvent = [CdpEvent{}; 4];
            const polled_count = session.pollConsoleEvents(&polled_evts);
            let p_i: usize = 0;
            while (p_i < polled_count && event_count.* < out_events.len) : (p_i += 1) {
                out_events[event_count.*] = polled_evts[p_i];
                event_count.* += 1;
            }

            if (eval_len == 0) {
                return CdpResponse.ok(req.id, "{\"result\":{\"type\":\"undefined\"}}");
            }
            return CdpResponse.ok(req.id, out_buf[0..eval_len]);
        }

        if (std.mem.eql(u8, req.method, "Runtime.queryObjects")) {
            return CdpResponse.ok(req.id, "{\"objects\":{\"type\":\"object\",\"objectId\":\"capsurf-query-obj\"}}");
        }

        if (std.mem.eql(u8, req.method, "Runtime.getProperties")) {
            const obj_id = extractJsonString(req.params_raw, "objectId") ?? "";
            const accessor_only = extractJsonBool(req.params_raw, "accessorPropertiesOnly") ?? false;
            if (obj_id.len > 0 && !accessor_only) {
                const id_val = session.ctx.newString(obj_id);
                defer session.ctx.freeValue(id_val);
                session.ctx.setPropertyStr("__capsurf_props_id", id_val);
                const js_val = session.ctx.eval(
                    "(function(){var o=globalThis.__capsurf_get_object?globalThis.__capsurf_get_object(globalThis.__capsurf_props_id):undefined;" ++
                    "if(o===null||(typeof o!=='object'&&typeof o!=='function'))return '{\"result\":[]}';" ++
                    "var ks=Object.getOwnPropertyNames(o);var out=[];" ++
                    "for(var i=0;i<ks.length;i++){var k=ks[i];var d=Object.getOwnPropertyDescriptor(o,k);if(!d)continue;" ++
                    "var e={name:String(k),configurable:!!d.configurable,enumerable:!!d.enumerable,writable:!!d.writable,isOwn:true};" ++
                    "if('value' in d)e.value=globalThis.__capsurf_format_remote_object(d.value,false);out.push(e);}" ++
                    "return JSON.stringify({result:out});})()",
                );
                defer session.ctx.freeValue(js_val);
                if (!jsIsException(js_val)) {
                    const s = session.ctx.toCString(js_val);
                    defer std.heap.page_allocator.free(s);
                    if (s.len > 0 && s[0] == '{') {
                        const res = std.fmt.bufPrint(out_buf, "{s}", .{s}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                        return CdpResponse.ok(req.id, res);
                    }
                } else {
                    const exc = session.ctx.getException();
                    session.ctx.freeValue(exc);
                }
            }
            const res = std.fmt.bufPrint(out_buf, "{{\"result\":[]}}", .{}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Runtime.callFunctionOn")) {
            const exec_ctx_id = extractJsonInt(req.params_raw, "executionContextId");
            if (exec_ctx_id != null && !session.isValidContextId(exec_ctx_id.?)) {
                return CdpResponse.err(req.id, -32000, "Cannot find context with specified id");
            }
            const raw_object_id = extractJsonString(req.params_raw, "objectId");
            const object_id = raw_object_id ?? (if (exec_ctx_id != null) "globalThis" else "");
            const raw_decl = extractJsonString(req.params_raw, "functionDeclaration") ?? "";
            let decl_buf = session.allocator.alloc(u8, @max(raw_decl.len + 1, @as(usize, 65536))) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(decl_buf);
            const func_decl_len = unescapeJsonString(raw_decl, decl_buf).len;
            decl_buf[func_decl_len] = 0;
            const func_decl = decl_buf[0..func_decl_len];
            const return_by_value = extractJsonBool(req.params_raw, "returnByValue") ?? false;
            const await_promise = extractJsonBool(req.params_raw, "awaitPromise") ?? false;
            const args_json = extractJsonArray(req.params_raw, "arguments") ?? "[]";
            
            const in_iso = (exec_ctx_id != null && exec_ctx_id.? > 1) || session.isIsolatedObjectId(object_id);
            const restored_node: i32 = if (in_iso) session.restoreIsolatedNodeIfNeeded() else 0;
            defer if (in_iso) session.clearIsolatedNode(restored_node);
            let res_len = session.callFunctionOn(object_id, func_decl, args_json, return_by_value, await_promise, out_buf);

            if (std.mem.indexOf(u8, out_buf[0..res_len], "cannot read property '") != null ||
                std.mem.indexOf(u8, out_buf[0..res_len], "cannot set property '") != null ||
                std.mem.indexOf(u8, out_buf[0..res_len], "Cannot read property '") != null ||
                std.mem.indexOf(u8, out_buf[0..res_len], "Cannot set property '") != null)
            {
                if (norm_buf := session.allocator.alloc(u8, res_len + 4096) catch null) {
                    defer session.allocator.free(norm_buf);
                    const norm_slice = normalizeExceptionMessage(out_buf[0..res_len], norm_buf);
                    if (norm_slice.len <= out_buf.len) {
                        @memcpy(out_buf[0..norm_slice.len], norm_slice);
                        res_len = norm_slice.len;
                    }
                }
            }
            
            // Poll any console events produced during callFunctionOn
            let polled_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
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

            // Poll any binding events produced during callFunctionOn
            let polled_binding_evts: [4]CdpEvent = [CdpEvent{}; 4];
            const polled_b_count = session.pollBindingEvents(&polled_binding_evts);
            let b_i: usize = 0;
            while (b_i < polled_b_count && event_count.* < out_events.len) : (b_i += 1) {
                out_events[event_count.*] = polled_binding_evts[b_i];
                event_count.* += 1;
            }

            // Poll any same-document navigation events produced during callFunctionOn
            let polled_nav_evts: [4]CdpEvent = [CdpEvent{}; 4];
            const polled_n_count = session.pollNavigatedWithinDocEvents(&polled_nav_evts);
            let n_i: usize = 0;
            while (n_i < polled_n_count && event_count.* < out_events.len) : (n_i += 1) {
                out_events[event_count.*] = polled_nav_evts[n_i];
                event_count.* += 1;
            }

            // Poll any network events produced during callFunctionOn (e.g. fetch() / XHR)
            let polled_net_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            const polled_net_count = session.pollNetworkEvents(&polled_net_evts);
            let net_i: usize = 0;
            while (net_i < polled_net_count && event_count.* < out_events.len) : (net_i += 1) {
                out_events[event_count.*] = polled_net_evts[net_i];
                event_count.* += 1;
            }

            // Poll any execution context created events and child iframe lifecycle events
            let polled_ctx_evts: [2]CdpEvent = [CdpEvent{}; 2];
            const polled_c_count = session.pollExecutionContextEvents(&polled_ctx_evts);
            let c_i: usize = 0;
            while (c_i < polled_c_count && event_count.* < out_events.len) : (c_i += 1) {
                out_events[event_count.*] = polled_ctx_evts[c_i];
                event_count.* += 1;
            }
            emitNewChildFrameEvents(session, out_events, event_count);

            let polled_pop_evts: [8]CdpEvent = [CdpEvent{}; 8];
            const polled_pop_count = session.pollPopupTargetEvents(&polled_pop_evts);
            let pop_i: usize = 0;
            while (pop_i < polled_pop_count && event_count.* < out_events.len) : (pop_i += 1) {
                out_events[event_count.*] = polled_pop_evts[pop_i];
                event_count.* += 1;
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
                if (session.isRetiredIsolatedWorldId(context_id.?)) {
                    return CdpResponse.err(req.id, -32000, "Cannot find context with specified id");
                }
                const err_res = std.fmt.bufPrint(
                    out_buf,
                    "{{\"result\":{{\"type\":\"undefined\"}},\"exceptionDetails\":{{\"text\":\"Uncaught\",\"exception\":{{\"type\":\"object\",\"subtype\":\"error\",\"className\":\"Error\",\"description\":\"Error: unknown or destroyed execution context {d}\"}}}}}}",
                    .{context_id.?},
                ) catch return CdpResponse.err(req.id, -32000, "Cannot find context with specified id");
                return CdpResponse.ok(req.id, err_res);
            }

            const in_iso = context_id != null && context_id.? > 1;
            const restored_node: i32 = if (in_iso) session.restoreIsolatedNodeIfNeeded() else 0;
            defer if (in_iso) session.clearIsolatedNode(restored_node);
            let eval_len = session.evaluateInContext(context_id, expr, return_by_value, await_promise, out_buf);

            if (std.mem.indexOf(u8, out_buf[0..eval_len], "cannot read property '") != null ||
                std.mem.indexOf(u8, out_buf[0..eval_len], "cannot set property '") != null ||
                std.mem.indexOf(u8, out_buf[0..eval_len], "Cannot read property '") != null ||
                std.mem.indexOf(u8, out_buf[0..eval_len], "Cannot set property '") != null)
            {
                if (norm_buf := session.allocator.alloc(u8, eval_len + 4096) catch null) {
                    defer session.allocator.free(norm_buf);
                    const norm_slice = normalizeExceptionMessage(out_buf[0..eval_len], norm_buf);
                    if (norm_slice.len <= out_buf.len) {
                        @memcpy(out_buf[0..norm_slice.len], norm_slice);
                        eval_len = norm_slice.len;
                    }
                }
            }

            // Poll any network events produced during evaluation
            let polled_net_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            const polled_net_count = session.pollNetworkEvents(&polled_net_evts);
            let net_i: usize = 0;
            while (net_i < polled_net_count && event_count.* < out_events.len) : (net_i += 1) {
                out_events[event_count.*] = polled_net_evts[net_i];
                event_count.* += 1;
            }

            // Poll any console events produced during evaluation
            let polled_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            const polled_count = session.pollConsoleEvents(&polled_evts);
            let p_i: usize = 0;
            while (p_i < polled_count && event_count.* < out_events.len) : (p_i += 1) {
                out_events[event_count.*] = polled_evts[p_i];
                event_count.* += 1;
            }

            // Poll any document.open/write/close reopened load events (strictly after console events)
            let polled_reopen_evts: [16]CdpEvent = [CdpEvent{}; 16];
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

            // Emit dynamic child iframe lifecycle events if any new iframes were attached
            emitNewChildFrameEvents(session, out_events, event_count);

            let polled_pop_evts: [8]CdpEvent = [CdpEvent{}; 8];
            const polled_pop_count = session.pollPopupTargetEvents(&polled_pop_evts);
            let pop_i: usize = 0;
            while (pop_i < polled_pop_count && event_count.* < out_events.len) : (pop_i += 1) {
                out_events[event_count.*] = polled_pop_evts[pop_i];
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
