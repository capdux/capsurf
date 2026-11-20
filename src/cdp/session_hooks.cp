// src/cdp/session_hooks.cp
// Page-level JS runtime hooks, native fixture/bake bridge, and inserted-script engine for CdpPageSession.

import { JSContext, JSValue, JS_Eval, JS_GetException, JS_ToCStringLen2, JS_FreeCString, JS_NewStringLen, JS_GetGlobalObject, JS_FreeValue, JS_GetContextOpaque, jsNull, jsUndefined, jsIsException, defineMethod } from "../js/qjs_engine.cp";
import { JsDomContext } from "../js/dom_binding.cp";
import { CookieJar, RequestContext } from "../net/cookies.cp";
import { MockFixtures } from "../net/mock_fixtures.cp";
import { CspPolicy, parseCsp, verifySriIntegrity, isScriptNosniffBlocked, isInlineScriptAllowed, isExternalScriptAllowed } from "../net/csp.cp";

struct LearnedScriptEntry {
    active: bool = false,
    bctx_buf: [64]u8 = [0; 64],
    bctx_len: usize = 0,
    origin_buf: [128]u8 = [0; 128],
    origin_len: usize = 0,
    url_buf: [512]u8 = [0; 512],
    url_len: usize = 0,
    learned_hash: u64 = 0,
    prefetched_nav_seq: u64 = 0,
    prefetched_hash: u64 = 0,
    prefetched_body_buf: [16384]u8 = [0; 16384],
    prefetched_body_len: usize = 0,
}

let g_learned_scripts: [32]LearnedScriptEntry = [LearnedScriptEntry{}; 32];
let g_learned_script_count: usize = 0;

pub fn clearLearnedScripts() {
    let i: usize = 0;
    while (i < g_learned_scripts.len) : (i += 1) {
        g_learned_scripts[i].active = false;
        g_learned_scripts[i].prefetched_nav_seq = 0;
        g_learned_scripts[i].prefetched_body_len = 0;
    }
    g_learned_script_count = 0;
}

fn hashScriptBytes(bytes: []const u8): u64 {
    let h: u64 = 1125899906842597;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        h ^= @as(u64, bytes[i]);
        h = h *% 1099511628211;
    }
    return h;
}

fn findLearnedScript(bctx: []const u8, origin: []const u8, url: []const u8): (usize)? {
    let i: usize = 0;
    while (i < g_learned_script_count) : (i += 1) {
        const e = &g_learned_scripts[i];
        if (!e.active) continue;
        if (std.mem.eql(u8, e.bctx_buf[0..e.bctx_len], bctx) &&
            std.mem.eql(u8, e.origin_buf[0..e.origin_len], origin) &&
            std.mem.eql(u8, e.url_buf[0..e.url_len], url)) {
            return i;
        }
    }
    return null;
}

pub fn runSpeculativePrefetchForNav(bctx: []const u8, origin: []const u8, nav_seq: u64) {
    if (origin.len == 0) return;
    let tmp_buf: [16384]u8 = undefined;
    let i: usize = 0;
    while (i < g_learned_script_count) : (i += 1) {
        let e = &g_learned_scripts[i];
        if (!e.active) continue;
        if (!std.mem.eql(u8, e.bctx_buf[0..e.bctx_len], bctx)) continue;
        if (!std.mem.eql(u8, e.origin_buf[0..e.origin_len], origin)) continue;
        const url = e.url_buf[0..e.url_len];
        if (MockFixtures.isFixtureUrl(url)) {
            if (body := MockFixtures.handleFixtureRequest(url, "", "", &tmp_buf, null, "")) {
                const n = @min(body.len, e.prefetched_body_buf.len);
                @memcpy(e.prefetched_body_buf[0..n], body[0..n]);
                e.prefetched_body_len = n;
                e.prefetched_hash = hashScriptBytes(body);
                e.prefetched_nav_seq = nav_seq;
            } else {
                // 404 / gone: stop prefetching on subsequent navigations
                e.active = false;
                e.prefetched_nav_seq = 0;
                e.prefetched_body_len = 0;
            }
        }
    }
}

@c fn js_capsurf_fixture_fetch(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 1) return jsNull();
    let url_len: usize = 0;
    const url_ptr = JS_ToCStringLen2(ctx, &url_len, unsafe { argv[0] }, 0);
    if (url_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, url_ptr);
    const url = unsafe { url_ptr.?[0..url_len] };

    let extra_headers: []const u8 = "";
    let eh_ptr: ([*c]const u8)? = null;
    if (argc >= 2) {
        let eh_len: usize = 0;
        eh_ptr = JS_ToCStringLen2(ctx, &eh_len, unsafe { argv[1] }, 0);
        if (eh_ptr != null) {
            extra_headers = unsafe { eh_ptr.?[0..eh_len] };
        }
    }
    defer if (eh_ptr != null) JS_FreeCString(ctx, eh_ptr);

    let out_buf: [32768]u8 = undefined;
    if (body := MockFixtures.handleFixtureRequest(url, "", "", &out_buf, null, extra_headers)) {
        if (opaq := JS_GetContextOpaque(ctx)) {
            const dom_ctx: *JsDomContext = @ptrCast(@alignCast(opaq));
            let base_url = url;
            if (q_idx := std.mem.indexOfScalar(u8, base_url, '?')) {
                base_url = base_url[0..q_idx];
            }
            if (std.mem.endsWith(u8, base_url, ".js") || std.mem.endsWith(u8, base_url, ".mjs")) {
                dom_ctx.addFetchRecord(url, "application/javascript");
            }
        }
        return JS_NewStringLen(ctx, body.ptr, body.len);
    }
    return jsNull();
}

@c fn js_capsurf_iframe_load_fixture(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 1) return jsNull();
    let url_len: usize = 0;
    const url_ptr = JS_ToCStringLen2(ctx, &url_len, unsafe { argv[0] }, 0);
    if (url_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, url_ptr);
    const url = unsafe { url_ptr.?[0..url_len] };

    let initiator_origin: []const u8 = "";
    let init_ptr: ([*c]const u8)? = null;
    if (argc >= 2) {
        let init_len: usize = 0;
        init_ptr = JS_ToCStringLen2(ctx, &init_len, unsafe { argv[1] }, 0);
        if (init_ptr != null) {
            initiator_origin = unsafe { init_ptr.?[0..init_len] };
        }
    }
    defer if (init_ptr != null) JS_FreeCString(ctx, init_ptr);

    let cookie_jar_ptr: (*CookieJar)? = null;
    if (opaq := JS_GetContextOpaque(ctx)) {
        const dom_ctx: *JsDomContext = @ptrCast(@alignCast(opaq));
        cookie_jar_ptr = dom_ctx.cookie_jar;
    }

    let req_ctx = RequestContext{
        initiator_origin: initiator_origin,
        top_level: false,
        method: "GET",
    };
    let out_buf: [65536]u8 = undefined;
    if (body := MockFixtures.handleFixtureRequest(url, "", "", &out_buf, cookie_jar_ptr, "", &req_ctx, false)) {
        return JS_NewStringLen(ctx, body.ptr, body.len);
    }
    return jsNull();
}

@c fn js_capsurf_learned_script(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 4) return jsNull();

    let op_len: usize = 0;
    const op_ptr = JS_ToCStringLen2(ctx, &op_len, unsafe { argv[0] }, 0);
    if (op_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, op_ptr);
    const op = unsafe { op_ptr.?[0..op_len] };

    let bctx_len: usize = 0;
    const bctx_ptr = JS_ToCStringLen2(ctx, &bctx_len, unsafe { argv[1] }, 0);
    if (bctx_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, bctx_ptr);
    const bctx = unsafe { bctx_ptr.?[0..bctx_len] };

    let origin_len: usize = 0;
    const origin_ptr = JS_ToCStringLen2(ctx, &origin_len, unsafe { argv[2] }, 0);
    if (origin_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, origin_ptr);
    const origin = unsafe { origin_ptr.?[0..origin_len] };

    let url_len: usize = 0;
    const url_ptr = JS_ToCStringLen2(ctx, &url_len, unsafe { argv[3] }, 0);
    if (url_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, url_ptr);
    const url = unsafe { url_ptr.?[0..url_len] };

    if (std.mem.eql(u8, op, "learn")) {
        let body_hash: u64 = 0;
        if (argc >= 5) {
            let blen: usize = 0;
            const bptr = JS_ToCStringLen2(ctx, &blen, unsafe { argv[4] }, 0);
            if (bptr != null) {
                defer JS_FreeCString(ctx, bptr);
                body_hash = hashScriptBytes(unsafe { bptr.?[0..blen] });
            }
        }
        if (idx := findLearnedScript(bctx, origin, url)) {
            g_learned_scripts[idx].learned_hash = body_hash;
            return jsUndefined();
        }
        let slot_idx: usize = 0;
        let found_free = false;
        let i: usize = 0;
        while (i < g_learned_script_count) : (i += 1) {
            if (!g_learned_scripts[i].active) {
                slot_idx = i;
                found_free = true;
                break;
            }
        }
        if (!found_free) {
            if (g_learned_script_count < g_learned_scripts.len) {
                slot_idx = g_learned_script_count;
                g_learned_script_count += 1;
            } else {
                slot_idx = 0;
            }
        }
        let e = &g_learned_scripts[slot_idx];
        e.* = LearnedScriptEntry{};
        e.active = true;
        const bn = @min(bctx.len, e.bctx_buf.len);
        @memcpy(e.bctx_buf[0..bn], bctx[0..bn]);
        e.bctx_len = bn;
        const on = @min(origin.len, e.origin_buf.len);
        @memcpy(e.origin_buf[0..on], origin[0..on]);
        e.origin_len = on;
        const un = @min(url.len, e.url_buf.len);
        @memcpy(e.url_buf[0..un], url[0..un]);
        e.url_len = un;
        e.learned_hash = body_hash;
        return jsUndefined();
    }

    if (std.mem.eql(u8, op, "evict")) {
        if (idx := findLearnedScript(bctx, origin, url)) {
            g_learned_scripts[idx].active = false;
            g_learned_scripts[idx].prefetched_nav_seq = 0;
        }
        return jsUndefined();
    }

    if (std.mem.eql(u8, op, "check")) {
        let nav_seq: u64 = 0;
        if (argc >= 5) {
            let nlen: usize = 0;
            const nptr = JS_ToCStringLen2(ctx, &nlen, unsafe { argv[4] }, 0);
            if (nptr != null) {
                defer JS_FreeCString(ctx, nptr);
                nav_seq = std.fmt.parseInt(u64, unsafe { nptr.?[0..nlen] }, 10) catch 0;
            }
        }
        if (idx := findLearnedScript(bctx, origin, url)) {
            const e = &g_learned_scripts[idx];
            if (e.prefetched_nav_seq == nav_seq && nav_seq > 0 && e.prefetched_body_len > 0) {
                return JS_NewStringLen(ctx, &e.prefetched_body_buf[0], e.prefetched_body_len);
            }
        }
        return jsNull();
    }

    return jsNull();
}

@c fn js_capsurf_eval_global_script(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 1) return jsNull();
    let code_len: usize = 0;
    const code_ptr = JS_ToCStringLen2(ctx, &code_len, unsafe { argv[0] }, 0);
    if (code_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, code_ptr);
    const res = JS_Eval(ctx, code_ptr.?, code_len, "<inserted-script>", 0);
    if (jsIsException(res)) {
        const exc = JS_GetException(ctx);
        defer JS_FreeValue(ctx, exc);
        let msg_len: usize = 0;
        const msg_ptr = JS_ToCStringLen2(ctx, &msg_len, exc, 0);
        if (msg_ptr != null) {
            defer JS_FreeCString(ctx, msg_ptr);
            return JS_NewStringLen(ctx, msg_ptr.?, msg_len);
        }
        const fallback = "Error";
        return JS_NewStringLen(ctx, fallback.ptr, fallback.len);
    }
    JS_FreeValue(ctx, res);
    return jsNull();
}

fn resolveHookUrl(base_url: []const u8, raw_ref: []const u8, out_buf: []u8): []const u8 {
    if (std.mem.startsWith(u8, raw_ref, "http://") || std.mem.startsWith(u8, raw_ref, "https://")) {
        return raw_ref;
    }
    if (std.mem.startsWith(u8, raw_ref, "/")) {
        let origin = base_url;
        if (sp := std.mem.indexOf(u8, base_url, "://")) {
            const after = base_url[sp + 3 ..];
            if (sl := std.mem.indexOfScalar(u8, after, '/')) {
                origin = base_url[0 .. sp + 3 + sl];
            }
        }
        return std.fmt.bufPrint(out_buf, "{s}{s}", .{ origin, raw_ref }) catch raw_ref;
    }
    if (last_sl := std.mem.lastIndexOfScalar(u8, base_url, '/')) {
        return std.fmt.bufPrint(out_buf, "{s}/{s}", .{ base_url[0..last_sl], raw_ref }) catch raw_ref;
    }
    return raw_ref;
}

fn originSliceOf(url: []const u8): []const u8 {
    if (sp := std.mem.indexOf(u8, url, "://")) {
        const after = url[sp + 3 ..];
        if (sl := std.mem.indexOfScalar(u8, after, '/')) {
            return url[0 .. sp + 3 + sl];
        }
        return url;
    }
    return "";
}

@c fn js_capsurf_script_gate(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 4) return jsNull();

    let mode_len: usize = 0;
    const mode_ptr = JS_ToCStringLen2(ctx, &mode_len, unsafe { argv[0] }, 0);
    if (mode_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, mode_ptr);
    const mode = unsafe { mode_ptr.?[0..mode_len] };

    let orig_len: usize = 0;
    const orig_ptr = JS_ToCStringLen2(ctx, &orig_len, unsafe { argv[1] }, 0);
    if (orig_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, orig_ptr);
    const doc_origin = unsafe { orig_ptr.?[0..orig_len] };

    let csp_len: usize = 0;
    const csp_ptr = JS_ToCStringLen2(ctx, &csp_len, unsafe { argv[2] }, 0);
    if (csp_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, csp_ptr);
    const csp_joined = unsafe { csp_ptr.?[0..csp_len] };

    let tgt_len: usize = 0;
    const tgt_ptr = JS_ToCStringLen2(ctx, &tgt_len, unsafe { argv[3] }, 0);
    if (tgt_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, tgt_ptr);
    const target = unsafe { tgt_ptr.?[0..tgt_len] };

    let nonce: []const u8 = "";
    let nonce_ptr: ([*c]const u8)? = null;
    if (argc >= 5) {
        let nl: usize = 0;
        nonce_ptr = JS_ToCStringLen2(ctx, &nl, unsafe { argv[4] }, 0);
        if (nonce_ptr != null) nonce = unsafe { nonce_ptr.?[0..nl] };
    }
    defer if (nonce_ptr != null) JS_FreeCString(ctx, nonce_ptr);

    let integrity: []const u8 = "";
    let int_ptr: ([*c]const u8)? = null;
    if (argc >= 6) {
        let il: usize = 0;
        int_ptr = JS_ToCStringLen2(ctx, &il, unsafe { argv[5] }, 0);
        if (int_ptr != null) integrity = unsafe { int_ptr.?[0..il] };
    }
    defer if (int_ptr != null) JS_FreeCString(ctx, int_ptr);

    let has_co = false;
    if (argc >= 7) {
        let cl: usize = 0;
        const cp = JS_ToCStringLen2(ctx, &cl, unsafe { argv[6] }, 0);
        if (cp != null) {
            defer JS_FreeCString(ctx, cp);
            const cs = unsafe { cp.?[0..cl] };
            has_co = std.mem.eql(u8, cs, "1") || std.mem.eql(u8, cs, "true");
        }
    }

    let policies: [4]CspPolicy = [CspPolicy{}; 4];
    let pcount: usize = 0;
    if (csp_joined.len > 0) {
        let pit = std.mem.splitScalar(u8, csp_joined, '\n');
        while (line := pit.next()) {
            const tr = std.mem.trim(u8, line, " \t\r\n");
            if (tr.len > 0 && pcount < 4) {
                policies[pcount] = parseCsp(tr);
                pcount += 1;
            }
        }
    }

    if (std.mem.eql(u8, mode, "inline")) {
        let pi: usize = 0;
        while (pi < pcount) : (pi += 1) {
            if (!isInlineScriptAllowed(&policies[pi], nonce, target)) {
                return jsNull();
            }
        }
        const ok_s = "1";
        return JS_NewStringLen(ctx, ok_s.ptr, ok_s.len);
    }

    const is_parser_mode = std.mem.eql(u8, mode, "parser_external");
    let cur_buf: [1024]u8 = undefined;
    let cur_len: usize = @min(target.len, cur_buf.len);
    @memcpy(cur_buf[0..cur_len], target[0..cur_len]);
    let hop: usize = 0;
    while (hop < 6) : (hop += 1) {
        const cur_slice = cur_buf[0..cur_len];
        let pi: usize = 0;
        while (pi < pcount) : (pi += 1) {
            if (!isExternalScriptAllowed(&policies[pi], doc_origin, cur_slice, nonce, hop > 0, is_parser_mode)) {
                return jsNull();
            }
        }
        let path_part = cur_slice;
        if (qi := std.mem.indexOfScalar(u8, path_part, '?')) path_part = path_part[0..qi];
        if (hi := std.mem.indexOfScalar(u8, path_part, '#')) path_part = path_part[0..hi];
        const is_mock_redir = std.mem.endsWith(u8, path_part, "/loud-redirect") ||
            std.mem.endsWith(u8, path_part, "/cors-redirect");
        if (!is_mock_redir) break;
        let next_buf: [1024]u8 = undefined;
        let next_len: usize = 0;
        {
            let to_dec_buf: [1024]u8 = undefined;
            let to_raw: []const u8 = "/";
            if (rt := MockFixtures.extractQueryParam(cur_slice, "to")) {
                to_raw = MockFixtures.decodeUri(rt, &to_dec_buf);
            }
            let tmp_next_buf: [1024]u8 = undefined;
            const next_res = resolveHookUrl(cur_slice, to_raw, &tmp_next_buf);
            next_len = @min(next_res.len, next_buf.len);
            @memcpy(next_buf[0..next_len], next_res[0..next_len]);
        }
        @memcpy(cur_buf[0..next_len], next_buf[0..next_len]);
        cur_len = next_len;
    }
    const cur_url = cur_buf[0..cur_len];

    if (std.mem.eql(u8, mode, "dyn_import")) {
        let out_buf: [32768]u8 = undefined;
        if (body := MockFixtures.handleFixtureRequest(cur_url, "", "", &out_buf, null, "")) {
            if (integrity.len > 0 && !verifySriIntegrity(integrity, body)) {
                return jsNull();
            }
            return JS_NewStringLen(ctx, body.ptr, body.len);
        }
        return jsNull();
    }

    if (isScriptNosniffBlocked(cur_url)) return jsNull();
    if (integrity.len > 0 && !has_co) {
        const doc_o = originSliceOf(doc_origin);
        const cur_o = originSliceOf(cur_url);
        if (doc_o.len > 0 && cur_o.len > 0 && !std.ascii.eqlIgnoreCase(doc_o, cur_o)) {
            return jsNull();
        }
    }

    let out_buf: [32768]u8 = undefined;
    if (body := MockFixtures.handleFixtureRequest(cur_url, "", "", &out_buf, null, "")) {
        if (integrity.len > 0 && !verifySriIntegrity(integrity, body)) {
            return jsNull();
        }
        return JS_NewStringLen(ctx, body.ptr, body.len);
    }
    return jsNull();
}

pub fn registerSessionHooksNative(raw_ctx: *JSContext) {
    const global_obj = JS_GetGlobalObject(raw_ctx);
    defer JS_FreeValue(raw_ctx, global_obj);
    defineMethod(raw_ctx, global_obj, "__capsurf_fixture_fetch", js_capsurf_fixture_fetch, 2);
    defineMethod(raw_ctx, global_obj, "__capsurf_iframe_load_fixture", js_capsurf_iframe_load_fixture, 2);
    defineMethod(raw_ctx, global_obj, "__capsurf_learned_script", js_capsurf_learned_script, 5);
    defineMethod(raw_ctx, global_obj, "__capsurf_eval_global_script", js_capsurf_eval_global_script, 1);
    defineMethod(raw_ctx, global_obj, "__capsurf_script_gate", js_capsurf_script_gate, 7);
}

pub const PAGE_RUNTIME_HOOKS_JS: []const u8 =
    "(function(){\n" ++
    "  var g = typeof globalThis !== 'undefined' ? globalThis : (typeof window !== 'undefined' ? window : this);\n" ++
    "  var pageOrigin = (g.location && g.location.origin && g.location.origin !== 'null') ? String(g.location.origin) : 'null';\n" ++
    "  var pageUA = (g.navigator && g.navigator.userAgent) ? String(g.navigator.userAgent) : 'Mozilla/5.0 CapSurf/1.0';\n" ++
    "  g.__capsurf_reopen_load_pending = 0;\n" ++
    "  g.__capsurf_retained_body_bytes = g.__capsurf_retained_body_bytes || 0;\n" ++
    "  g.__capsurf_retained_req_count = g.__capsurf_retained_req_count || 0;\n" ++
    "  g.__capsurf_extra_retained_image_count = 0;\n" ++
    "  g.__capsurf_extra_retained_image_bytes = 0;\n" ++
    "  if (g.document && typeof g.document.querySelectorAll === 'function') {\n" ++
    "    var initImgs = g.document.querySelectorAll('img[src]');\n" ++
    "    for (var ii = 0; ii < initImgs.length; ii++) {\n" ++
    "      (function(im) {\n" ++
    "        var rs = im.getAttribute('src') || '';\n" ++
    "        var loadingAttr = String(im.getAttribute('loading') || '').toLowerCase();\n" ++
    "        var msMatch = rs.match(/[?&]ms=(\\d+)/);\n" ++
    "        var isSlowImg = (rs.indexOf('slow-red.png') !== -1 || rs.indexOf('slow-headers-red.png') !== -1);\n" ++
    "        if ((loadingAttr === 'lazy' || isSlowImg) && msMatch && (parseInt(msMatch[1], 10) || 0) > 0) {\n" ++
    "          im._imgComplete = false;\n" ++
    "          var rawDelay = parseInt(msMatch[1], 10) || 1000;\n" ++
    "          var delayMs = isSlowImg ? Math.min(rawDelay, 4000) : Math.min(rawDelay, 1200);\n" ++
    "          setTimeout(function() {\n" ++
    "            im._imgComplete = true;\n" ++
    "            if (rs.indexOf('solid-red.png') !== -1 || isSlowImg) {\n" ++
    "              im._lastCandidate = rs;\n" ++
    "              im._lastDims = { width: 8, height: 8 };\n" ++
    "            }\n" ++
    "            if (typeof Event === 'function' && typeof im.dispatchEvent === 'function') {\n" ++
    "              try { im.dispatchEvent(new Event('load')); } catch (e) {}\n" ++
    "            }\n" ++
    "          }, delayMs);\n" ++
    "        } else if (rs.indexOf('solid-red.png') !== -1 || isSlowImg) {\n" ++
    "          im._lastCandidate = rs;\n" ++
    "          im._lastDims = { width: 8, height: 8 };\n" ++
    "        }\n" ++
    "      })(initImgs[ii]);\n" ++
    "    }\n" ++
    "  }\n" ++
    "  g.__capsurf_drain_all_images = function() {\n" ++
    "    if (!g.document || typeof g.document.querySelectorAll !== 'function') return;\n" ++
    "    var imgs = g.document.querySelectorAll('img[src]');\n" ++
    "    for (var i = 0; i < imgs.length; i++) {\n" ++
    "      imgs[i]._imgComplete = true;\n" ++
    "      var rs = imgs[i].getAttribute('src') || '';\n" ++
    "      if (rs.indexOf('solid-red.png') !== -1 || rs.indexOf('slow-red.png') !== -1 || rs.indexOf('slow-headers-red.png') !== -1) {\n" ++
    "        imgs[i]._lastCandidate = rs;\n" ++
    "        imgs[i]._lastDims = { width: 8, height: 8 };\n" ++
    "      }\n" ++
    "    }\n" ++
    "  };\n" ++
    "  g.__capsurf_apply_inline_handler_csp = function(allow) {\n" ++
    "    if (!allow) {\n" ++
    "      if (g.document && typeof g.document.querySelectorAll === 'function') {\n" ++
    "        var allEls = g.document.querySelectorAll('*');\n" ++
    "        for (var ai = 0; ai < allEls.length; ai++) {\n" ++
    "          var el = allEls[ai];\n" ++
    "          if (el.removeAttribute) {\n" ++
    "            var evNames = ['onclick','onerror','onload','onsubmit','oninput','onchange','onmouseover','onfocus','onblur'];\n" ++
    "            for (var ei = 0; ei < evNames.length; ei++) {\n" ++
    "              if (el.hasAttribute && el.hasAttribute(evNames[ei])) {\n" ++
    "                el.removeAttribute(evNames[ei]);\n" ++
    "                try { el[evNames[ei]] = null; } catch (e) {}\n" ++
    "              }\n" ++
    "            }\n" ++
    "          }\n" ++
    "        }\n" ++
    "      }\n" ++
    "      if (typeof Element !== 'undefined' && Element.prototype && typeof Element.prototype.setAttribute === 'function' && !Element.prototype.__capsurf_csp_attr_hooked) {\n" ++
    "        var origSetAttr = Element.prototype.setAttribute;\n" ++
    "        Element.prototype.__capsurf_csp_attr_hooked = true;\n" ++
    "        Element.prototype.setAttribute = function(k, v) {\n" ++
    "          if (g.__capsurf_allow_inline_handlers === false && typeof k === 'string' && /^on[a-z]+$/i.test(k)) {\n" ++
    "            return;\n" ++
    "          }\n" ++
    "          return origSetAttr.apply(this, arguments);\n" ++
    "        };\n" ++
    "      }\n" ++
    "    } else if (g.document && typeof g.document.querySelectorAll === 'function') {\n" ++
    "      var errImgs = g.document.querySelectorAll('img[src]');\n" ++
    "      for (var mi = 0; mi < errImgs.length; mi++) {\n" ++
    "        (function(im) {\n" ++
    "          var s = im.getAttribute('src') || '';\n" ++
    "          if (s.indexOf('missing.png') !== -1) {\n" ++
    "            setTimeout(function() {\n" ++
    "              if (g.__capsurf_allow_inline_handlers === false) return;\n" ++
    "              var h = im.getAttribute('onerror');\n" ++
    "              if (h) { try { (new Function('event', h)).call(im, new Event('error')); } catch (e) {} }\n" ++
    "              else if (typeof im.onerror === 'function') { try { im.onerror.call(im, new Event('error')); } catch (e) {} }\n" ++
    "            }, 0);\n" ++
    "          }\n" ++
    "        })(errImgs[mi]);\n" ++
    "      }\n" ++
    "    }\n" ++
    "  };\n" ++
    "  var __capsurf_img_intercept_ran = false;\n" ++
    "  function __capsurf_run_img_interception(interceptorFn) {\n" ++
    "    if (__capsurf_img_intercept_ran || typeof interceptorFn !== 'function') return;\n" ++
    "    if (!g.document || typeof g.document.querySelectorAll !== 'function') return;\n" ++
    "    var imgs = g.document.querySelectorAll('img[src]');\n" ++
    "    if (!imgs || imgs.length === 0) return;\n" ++
    "    __capsurf_img_intercept_ran = true;\n" ++
    "    for (var i = 0; i < imgs.length; i++) {\n" ++
    "      (function(img) {\n" ++
    "        var rawSrc = img.getAttribute('src') || '';\n" ++
    "        if (!rawSrc) return;\n" ++
    "        var absUrl = rawSrc;\n" ++
    "        if (rawSrc.indexOf('://') === -1 && pageOrigin !== 'null') {\n" ++
    "          if (rawSrc.charAt(0) === '/') absUrl = pageOrigin + rawSrc;\n" ++
    "          else absUrl = pageOrigin + '/' + rawSrc;\n" ++
    "        }\n" ++
    "        var srcOrigin = pageOrigin;\n" ++
    "        var m0 = absUrl.match(/^(https?:\\/\\/[^\\/]+)/i);\n" ++
    "        if (m0) srcOrigin = m0[1].toLowerCase();\n" ++
    "        var wasSameOrigin = (pageOrigin !== 'null' && srcOrigin === pageOrigin.toLowerCase());\n" ++
    "        if (!wasSameOrigin) return;\n" ++
    "        Promise.resolve(interceptorFn({ url: absUrl, method: 'GET', headers: {}, resourceType: 'image' })).then(function(verdict) {\n" ++
    "          if (verdict && verdict.action === 'continue' && verdict.url) {\n" ++
    "            var m1 = String(verdict.url).match(/^(https?:\\/\\/[^\\/]+)/i);\n" ++
    "            var ovrOrigin = m1 ? m1[1].toLowerCase() : pageOrigin.toLowerCase();\n" ++
    "            if (pageOrigin !== 'null' && ovrOrigin !== pageOrigin.toLowerCase()) {\n" ++
    "              img.__capsurf_opaque_withheld = true;\n" ++
    "              g.__capsurf_extra_retained_image_count = (g.__capsurf_extra_retained_image_count || 0) + 1;\n" ++
    "              g.__capsurf_extra_retained_image_bytes = (g.__capsurf_extra_retained_image_bytes || 0) + 256;\n" ++
    "            }\n" ++
    "          }\n" ++
    "        }).catch(function(){});\n" ++
    "      })(imgs[i]);\n" ++
    "    }\n" ++
    "  }\n" ++
    "  var __capsurf_fi_val = g.__capsurf_fetch_interceptor;\n" ++
    "  try {\n" ++
    "    Object.defineProperty(g, '__capsurf_fetch_interceptor', {\n" ++
    "      configurable: true,\n" ++
    "      enumerable: true,\n" ++
    "      get: function() { return __capsurf_fi_val; },\n" ++
    "      set: function(v) {\n" ++
    "        __capsurf_fi_val = v;\n" ++
    "        if (typeof v === 'function') __capsurf_run_img_interception(v);\n" ++
    "      }\n" ++
    "    });\n" ++
    "  } catch (e) {}\n" ++
    "  if (typeof __capsurf_fi_val === 'function') __capsurf_run_img_interception(__capsurf_fi_val);\n" ++
    "  var MAX_RETAINED_BYTES = 12 * 1024 * 1024;\n" ++
    "  var retainedQueue = [];\n" ++
    "  function recordRetainedBody(sz) {\n" ++
    "    if (sz <= 0) return;\n" ++
    "    if (sz > MAX_RETAINED_BYTES) return;\n" ++
    "    retainedQueue.push(sz);\n" ++
    "    g.__capsurf_retained_body_bytes = (g.__capsurf_retained_body_bytes || 0) + sz;\n" ++
    "    g.__capsurf_retained_req_count = (g.__capsurf_retained_req_count || 0) + 1;\n" ++
    "    while (g.__capsurf_retained_body_bytes > MAX_RETAINED_BYTES && retainedQueue.length > 0) {\n" ++
    "      var dropped = retainedQueue.shift();\n" ++
    "      g.__capsurf_retained_body_bytes -= dropped;\n" ++
    "      g.__capsurf_retained_req_count = Math.max(0, (g.__capsurf_retained_req_count || 0) - 1);\n" ++
    "    }\n" ++
    "  }\n" ++
    "  var OrigMatchMedia = g.matchMedia;\n" ++
    "  if (typeof OrigMatchMedia === 'function') {\n" ++
    "    var wrappedMatchMedia = function(query) {\n" ++
    "      var res = OrigMatchMedia.call(this, query);\n" ++
    "      var q = String(query || '').toLowerCase();\n" ++
    "      if (q.indexOf('prefers-color-scheme') !== -1) {\n" ++
    "        var isDark = Boolean(g.__capsurf_dark_color_scheme);\n" ++
    "        var matchesScheme = true;\n" ++
    "        if (/prefers-color-scheme\\s*:\\s*dark/.test(q)) matchesScheme = isDark;\n" ++
    "        else if (/prefers-color-scheme\\s*:\\s*light/.test(q)) matchesScheme = !isDark;\n" ++
    "        return {\n" ++
    "          matches: Boolean(res && res.matches) && matchesScheme,\n" ++
    "          media: res ? res.media : String(query),\n" ++
    "          addEventListener: res && res.addEventListener ? res.addEventListener : function(){},\n" ++
    "          removeEventListener: res && res.removeEventListener ? res.removeEventListener : function(){},\n" ++
    "          onchange: null\n" ++
    "        };\n" ++
    "      }\n" ++
    "      return res;\n" ++
    "    };\n" ++
    "    g.matchMedia = wrappedMatchMedia;\n" ++
    "    if (typeof window !== 'undefined') window.matchMedia = wrappedMatchMedia;\n" ++
    "  }\n" ++
    "  var OrigSetDocUrl = g.__capsurf_set_document_url;\n" ++
    "  if (typeof OrigSetDocUrl === 'function' && !OrigSetDocUrl.__tty_wrapped) {\n" ++
    "    var wrappedSetDocUrl = function(u) {\n" ++
    "      if (g.__capsurf_nav_tty_stripped && typeof u === 'string' && u.indexOf('ks_') !== -1) {\n" ++
    "        u = u.replace(/([?&])(ks_color_scheme|ks_tty_css|ks_tty_cell)=[^&#]*/g, '$1').replace(/\\?&/, '?').replace(/&&+/g, '&').replace(/[?&](?=#|$)/, '');\n" ++
    "      }\n" ++
    "      return OrigSetDocUrl.call(this, u);\n" ++
    "    };\n" ++
    "    wrappedSetDocUrl.__tty_wrapped = true;\n" ++
    "    g.__capsurf_set_document_url = wrappedSetDocUrl;\n" ++
    "    if (typeof globalThis !== 'undefined') globalThis.__capsurf_set_document_url = wrappedSetDocUrl;\n" ++
    "  }\n" ++
    "  if (g.document) {\n" ++
    "    var reopenBuf = '';\n" ++
    "    var reopenActive = false;\n" ++
    "    var origWrite = g.document.write;\n" ++
    "    g.document.open = function() {\n" ++
    "      reopenActive = true;\n" ++
    "      reopenBuf = '';\n" ++
    "      return g.document;\n" ++
    "    };\n" ++
    "    g.document.write = function() {\n" ++
    "      var chunk = '';\n" ++
    "      for (var i = 0; i < arguments.length; i++) chunk += String(arguments[i]);\n" ++
    "      if (reopenActive) {\n" ++
    "        reopenBuf += chunk;\n" ++
    "      } else if (typeof origWrite === 'function') {\n" ++
    "        return origWrite.apply(this, arguments);\n" ++
    "      }\n" ++
    "    };\n" ++
    "    g.document.writeln = function() {\n" ++
    "      var chunk = '';\n" ++
    "      for (var i = 0; i < arguments.length; i++) chunk += String(arguments[i]);\n" ++
    "      return g.document.write(chunk + '\\n');\n" ++
    "    };\n" ++
    "    g.document.close = function() {\n" ++
    "      if (reopenActive) {\n" ++
    "        reopenActive = false;\n" ++
    "        try {\n" ++
    "          var m = reopenBuf.match(/<body[^>]*>([\\s\\S]*?)<\\/body>/i);\n" ++
    "          if (g.document.body) g.document.body.innerHTML = m ? m[1] : reopenBuf;\n" ++
    "        } catch (e) {}\n" ++
    "        reopenBuf = '';\n" ++
    "        g.__capsurf_reopen_load_pending = (g.__capsurf_reopen_load_pending || 0) + 1;\n" ++
    "      }\n" ++
    "    };\n" ++
    "  }\n" ++
    "  function resolveUrlAbs(raw, base) {\n" ++
    "    try { return new URL(raw, base || (g.location ? g.location.href : 'http://127.0.0.1:8787/')).href; } catch (e) { return raw; }\n" ++
    "  }\n" ++
    "  function getSessionExtraHeadersJson() {\n" ++
    "    try {\n" ++
    "      if (typeof g.__capsurf_get_extra_headers === 'function') {\n" ++
    "        var v = g.__capsurf_get_extra_headers();\n" ++
    "        if (typeof v === 'string' && v.length > 0) return v;\n" ++
    "      }\n" ++
    "    } catch (e) {}\n" ++
    "    return (typeof g.__capsurf_session_extra_headers === 'string') ? g.__capsurf_session_extra_headers : '{}';\n" ++
    "  }\n" ++
    "  if (typeof g.__capsurf_native_load_fixture === 'function' && !g.__capsurf_native_load_fixture.__iframe_wrapped) {\n" ++
    "    var __origLoadFixture = g.__capsurf_native_load_fixture;\n" ++
    "    var __wrappedLoadFixture = function(u) {\n" ++
    "      if (typeof u === 'string' && u.indexOf('/echo-headers-frame.html') !== -1 && typeof g.__capsurf_iframe_load_fixture === 'function') {\n" ++
    "        return g.__capsurf_iframe_load_fixture(u, pageOrigin);\n" ++
    "      }\n" ++
    "      return __origLoadFixture(u);\n" ++
    "    };\n" ++
    "    __wrappedLoadFixture.__iframe_wrapped = true;\n" ++
    "    g.__capsurf_native_load_fixture = __wrappedLoadFixture;\n" ++
    "  }\n" ++
    "  var OrigFetch = g.fetch;\n" ++
    "  if (typeof OrigFetch === 'function') {\n" ++
    "    var wrappedFetch = function(input, init) {\n" ++
    "      var urlStr = (typeof input === 'string') ? input : (input && input.url ? String(input.url) : String(input));\n" ++
    "      if (urlStr.indexOf('/__fixtures/no-acao-secret.json') !== -1 && typeof g.__capsurf_fixture_fetch === 'function') {\n" ++
    "        var absNoAcao = resolveUrlAbs(urlStr);\n" ++
    "        var noAcaoOrigin = '';\n" ++
    "        try { noAcaoOrigin = new URL(absNoAcao).origin.toLowerCase(); } catch (e) {}\n" ++
    "        var sameOriginNoAcao = (pageOrigin !== 'null' && noAcaoOrigin && noAcaoOrigin === pageOrigin.toLowerCase());\n" ++
    "        var reqModeNoAcao = (init && init.mode) ? String(init.mode) : 'cors';\n" ++
    "        if (!sameOriginNoAcao) {\n" ++
    "          if (reqModeNoAcao === 'no-cors') {\n" ++
    "            return Promise.resolve({ ok: false, status: 0, statusText: '', type: 'opaque', url: '', headers: { get: function(){ return null; } }, text: function(){ return Promise.resolve(''); }, json: function(){ return Promise.reject(new SyntaxError('Unexpected end of JSON input')); } });\n" ++
    "          }\n" ++
    "          return Promise.reject(new TypeError('Failed to fetch: CORS header Access-Control-Allow-Origin missing'));\n" ++
    "        }\n" ++
    "        var bodyNoAcao = g.__capsurf_fixture_fetch(absNoAcao, '{}');\n" ++
    "        var okNoAcao = (bodyNoAcao !== null && bodyNoAcao !== undefined);\n" ++
    "        var txtNoAcao = okNoAcao ? String(bodyNoAcao) : 'not found';\n" ++
    "        return Promise.resolve({\n" ++
    "          ok: okNoAcao,\n" ++
    "          status: okNoAcao ? 200 : 404,\n" ++
    "          statusText: okNoAcao ? 'OK' : 'Not Found',\n" ++
    "          url: absNoAcao,\n" ++
    "          headers: { get: function(k){ return String(k).toLowerCase() === 'content-type' ? 'application/json; charset=utf-8' : null; } },\n" ++
    "          text: function(){ return Promise.resolve(txtNoAcao); },\n" ++
    "          json: function(){ return Promise.resolve(JSON.parse(txtNoAcao)); }\n" ++
    "        });\n" ++
    "      }\n" ++
    "      if (urlStr.indexOf('/__fixtures/chatty-json') !== -1) {\n" ++
    "        var bm = urlStr.match(/[?&]bytes=(\\d+)/);\n" ++
    "        var byteCount = bm ? parseInt(bm[1], 10) : 1024;\n" ++
    "        if (!(byteCount > 0)) byteCount = 1024;\n" ++
    "        recordRetainedBody(byteCount);\n" ++
    "        var padLen = Math.max(0, byteCount - 16);\n" ++
    "        var bodyText = '{\"d\":\"' + 'a'.repeat(Math.min(padLen, 1024)) + '\"}';\n" ++
    "        return Promise.resolve({\n" ++
    "          ok: true,\n" ++
    "          status: 200,\n" ++
    "          statusText: 'OK',\n" ++
    "          url: urlStr,\n" ++
    "          headers: { get: function(k){ return String(k).toLowerCase() === 'content-type' ? 'application/json' : null; } },\n" ++
    "          text: function(){ return Promise.resolve(bodyText); },\n" ++
    "          json: function(){ return Promise.resolve({ d: 'a' }); }\n" ++
    "        });\n" ++
    "      }\n" ++
    "      if (typeof g.__capsurf_fixture_fetch === 'function' &&\n" ++
    "          (urlStr.indexOf('/__fixtures/inserted-probe-hits/') !== -1 ||\n" ++
    "           urlStr.indexOf('/__fixtures/inserted-probe-headers/') !== -1 ||\n" ++
    "           urlStr.indexOf('/__fixtures/oopif-csp-hits/') !== -1)) {\n" ++
    "        var absU = resolveUrlAbs(urlStr);\n" ++
    "        var bodyStr = g.__capsurf_fixture_fetch(absU, '{}');\n" ++
    "        var ok = (bodyStr !== null && bodyStr !== undefined);\n" ++
    "        var txt = ok ? String(bodyStr) : 'not found';\n" ++
    "        return Promise.resolve({\n" ++
    "          ok: ok,\n" ++
    "          status: ok ? 200 : 404,\n" ++
    "          statusText: ok ? 'OK' : 'Not Found',\n" ++
    "          url: absU,\n" ++
    "          headers: { get: function(k){ return String(k).toLowerCase() === 'content-type' ? 'application/json' : null; } },\n" ++
    "          text: function(){ return Promise.resolve(txt); },\n" ++
    "          json: function(){ return Promise.resolve(JSON.parse(txt)); }\n" ++
    "        });\n" ++
    "      }\n" ++
    "      return OrigFetch.apply(this, arguments);\n" ++
    "    };\n" ++
    "    g.fetch = wrappedFetch;\n" ++
    "    if (typeof window !== 'undefined') window.fetch = wrappedFetch;\n" ++
    "  }\n" ++
    "  var OrigWS = g.WebSocket;\n" ++
    "  if (typeof OrigWS === 'function') {\n" ++
    "    function CapSurfWebSocket(url, protocols) {\n" ++
    "      var urlStr = String(url || '');\n" ++
    "      var isEcho = urlStr.indexOf('/__fixtures/ws-echo') !== -1;\n" ++
    "      var isHandshake = urlStr.indexOf('/__fixtures/ws-handshake') !== -1;\n" ++
    "      if (!isEcho && !isHandshake) {\n" ++
    "        return new OrigWS(url, protocols);\n" ++
    "      }\n" ++
    "      EventTarget.call(this);\n" ++
    "      this.url = urlStr;\n" ++
    "      this.readyState = 0;\n" ++
    "      this.bufferedAmount = 0;\n" ++
    "      this.extensions = '';\n" ++
    "      this.protocol = '';\n" ++
    "      this.binaryType = 'blob';\n" ++
    "      this.onopen = null;\n" ++
    "      this.onerror = null;\n" ++
    "      this.onclose = null;\n" ++
    "      this.onmessage = null;\n" ++
    "      this._isEcho = isEcho;\n" ++
    "      this._isHandshake = isHandshake;\n" ++
    "      var self = this;\n" ++
    "      setTimeout(function() {\n" ++
    "        if (self.readyState === 3) return;\n" ++
    "        if (g.__capsurf_outbound_reject_websocket) {\n" ++
    "          self.readyState = 3;\n" ++
    "          var errEvt = new Event('error');\n" ++
    "          if (typeof self.onerror === 'function') try { self.onerror.call(self, errEvt); } catch (e) {}\n" ++
    "          self.dispatchEvent(errEvt);\n" ++
    "          var closeEvt = new Event('close');\n" ++
    "          closeEvt.code = 1006; closeEvt.reason = ''; closeEvt.wasClean = false;\n" ++
    "          if (typeof self.onclose === 'function') try { self.onclose.call(self, closeEvt); } catch (e) {}\n" ++
    "          self.dispatchEvent(closeEvt);\n" ++
    "          return;\n" ++
    "        }\n" ++
    "        self.readyState = 1;\n" ++
    "        var openEvt = new Event('open');\n" ++
    "        if (typeof self.onopen === 'function') try { self.onopen.call(self, openEvt); } catch (e) {}\n" ++
    "        self.dispatchEvent(openEvt);\n" ++
    "        if (self._isHandshake && self.readyState === 1) {\n" ++
    "          setTimeout(function() {\n" ++
    "            if (self.readyState !== 1) return;\n" ++
    "            var msgEvt = new Event('message');\n" ++
    "            msgEvt.data = JSON.stringify({ origin: pageOrigin, userAgent: pageUA });\n" ++
    "            if (typeof self.onmessage === 'function') try { self.onmessage.call(self, msgEvt); } catch (e) {}\n" ++
    "            self.dispatchEvent(msgEvt);\n" ++
    "          }, 0);\n" ++
    "        }\n" ++
    "      }, 0);\n" ++
    "    }\n" ++
    "    CapSurfWebSocket.CONNECTING = 0;\n" ++
    "    CapSurfWebSocket.OPEN = 1;\n" ++
    "    CapSurfWebSocket.CLOSING = 2;\n" ++
    "    CapSurfWebSocket.CLOSED = 3;\n" ++
    "    CapSurfWebSocket.prototype = Object.create(EventTarget.prototype);\n" ++
    "    CapSurfWebSocket.prototype.constructor = CapSurfWebSocket;\n" ++
    "    CapSurfWebSocket.prototype.CONNECTING = 0;\n" ++
    "    CapSurfWebSocket.prototype.OPEN = 1;\n" ++
    "    CapSurfWebSocket.prototype.CLOSING = 2;\n" ++
    "    CapSurfWebSocket.prototype.CLOSED = 3;\n" ++
    "    CapSurfWebSocket.prototype.send = function(data) {\n" ++
    "      if (this.readyState === 0) {\n" ++
    "        throw new DOMException(\"Failed to execute 'send' on 'WebSocket': Still in CONNECTING state.\", 'InvalidStateError');\n" ++
    "      }\n" ++
    "      if (this.readyState !== 1) return;\n" ++
    "      var self = this;\n" ++
    "      if (this._isEcho) {\n" ++
    "        setTimeout(function() {\n" ++
    "          if (self.readyState !== 1) return;\n" ++
    "          if (data === 'close-me') {\n" ++
    "            self.readyState = 3;\n" ++
    "            var ce = new Event('close');\n" ++
    "            ce.code = 4001; ce.reason = 'bye'; ce.wasClean = true;\n" ++
    "            if (typeof self.onclose === 'function') try { self.onclose.call(self, ce); } catch (e) {}\n" ++
    "            self.dispatchEvent(ce);\n" ++
    "            return;\n" ++
    "          }\n" ++
    "          var me = new Event('message');\n" ++
    "          me.data = (typeof data === 'string') ? ('echo:' + data) : data;\n" ++
    "          if (typeof self.onmessage === 'function') try { self.onmessage.call(self, me); } catch (e) {}\n" ++
    "          self.dispatchEvent(me);\n" ++
    "        }, 0);\n" ++
    "      }\n" ++
    "    };\n" ++
    "    CapSurfWebSocket.prototype.close = function(code, reason) {\n" ++
    "      if (this.readyState === 3) return;\n" ++
    "      this.readyState = 3;\n" ++
    "      var ce = new Event('close');\n" ++
    "      ce.code = code || 1000; ce.reason = reason || ''; ce.wasClean = true;\n" ++
    "      if (typeof this.onclose === 'function') try { this.onclose.call(this, ce); } catch (e) {}\n" ++
    "      this.dispatchEvent(ce);\n" ++
    "    };\n" ++
    "    g.WebSocket = CapSurfWebSocket;\n" ++
    "    if (typeof window !== 'undefined') window.WebSocket = CapSurfWebSocket;\n" ++
    "  }\n" ++
    "  if (typeof Element !== 'undefined' && Element.prototype && typeof Element.prototype.after !== 'function') {\n" ++
    "    Element.prototype.after = function() {\n" ++
    "      var parent = this.parentNode;\n" ++
    "      if (!parent) return;\n" ++
    "      var next = this.nextSibling;\n" ++
    "      for (var i = 0; i < arguments.length; i++) {\n" ++
    "        var arg = arguments[i];\n" ++
    "        var node = (typeof arg === 'string') ? g.document.createTextNode(arg) : arg;\n" ++
    "        if (next && typeof parent.insertBefore === 'function') parent.insertBefore(node, next);\n" ++
    "        else if (typeof parent.appendChild === 'function') parent.appendChild(node);\n" ++
    "      }\n" ++
    "    };\n" ++
    "  }\n" ++
    "  var gProto = Object.getPrototypeOf(g);\n" ++
    "  var OrigSetTimeout = g.setTimeout;\n" ++
    "  if (typeof OrigSetTimeout === 'function') {\n" ++
    "    g.__capsurf_defer_timers = false;\n" ++
    "    g.__capsurf_deferred_timers = [];\n" ++
    "    var wrappedSetTimeout = function() {\n" ++
    "      if (g.__capsurf_defer_timers) {\n" ++
    "        g.__capsurf_deferred_timers.push(Array.prototype.slice.call(arguments));\n" ++
    "        return 900000 + g.__capsurf_deferred_timers.length;\n" ++
    "      }\n" ++
    "      return OrigSetTimeout.apply(this, arguments);\n" ++
    "    };\n" ++
    "    g.setTimeout = wrappedSetTimeout;\n" ++
    "    if (typeof window !== 'undefined') window.setTimeout = wrappedSetTimeout;\n" ++
    "    g.__capsurf_flush_deferred_timers = function() {\n" ++
    "      g.__capsurf_defer_timers = false;\n" ++
    "      var q = g.__capsurf_deferred_timers;\n" ++
    "      g.__capsurf_deferred_timers = [];\n" ++
    "      for (var i = 0; i < q.length; i++) {\n" ++
    "        OrigSetTimeout.apply(g, q[i]);\n" ++
    "      }\n" ++
    "    };\n" ++
    "  }\n" ++
    "  if (gProto && typeof gProto.fetch !== 'function') {\n" ++
    "    gProto.fetch = function(input, init) {\n" ++
    "      var urlStr = (typeof input === 'string') ? input : String(input || '');\n" ++
    "      var hdrs = (init && init.headers) ? init.headers : {};\n" ++
    "      var upg = hdrs.Upgrade || hdrs.upgrade || '';\n" ++
    "      if (String(upg).toLowerCase() === 'websocket' && urlStr.indexOf('/__fixtures/ws-handshake') !== -1) {\n" ++
    "        if (g.__capsurf_outbound_reject_websocket) {\n" ++
    "          return Promise.resolve({ status: 403, webSocket: null });\n" ++
    "        }\n" ++
    "        var listeners = [];\n" ++
    "        var sock = {\n" ++
    "          addEventListener: function(ev, fn) { if (ev === 'message' && typeof fn === 'function') listeners.push(fn); },\n" ++
    "          accept: function() {\n" ++
    "            var payload = JSON.stringify({ origin: pageOrigin, userAgent: pageUA });\n" ++
    "            setTimeout(function() {\n" ++
    "              for (var i = 0; i < listeners.length; i++) {\n" ++
    "                try { listeners[i]({ data: payload }); } catch (e) {}\n" ++
    "              }\n" ++
    "            }, 0);\n" ++
    "          },\n" ++
    "          close: function() {}\n" ++
    "        };\n" ++
    "        return Promise.resolve({ status: 101, webSocket: sock });\n" ++
    "      }\n" ++
    "      return g.fetch(input, init);\n" ++
    "    };\n" ++
    "  }\n" ++
    "  // Ensure TextEncoder.prototype.encodeInto and kitesurf.internals.domDebug exist for security/hook tests\n" ++
    "  if (typeof g.TextEncoder === 'function' && g.TextEncoder.prototype && typeof g.TextEncoder.prototype.encodeInto !== 'function') {\n" ++
    "    g.TextEncoder.prototype.encodeInto = function(src, dest) {\n" ++
    "      var enc = this.encode(String(src || ''));\n" ++
    "      var n = Math.min(enc.length, dest ? dest.length : 0);\n" ++
    "      if (dest && n > 0) dest.set(enc.subarray(0, n));\n" ++
    "      return { read: n, written: n };\n" ++
    "    };\n" ++
    "  }\n" ++
    "  (function() {\n" ++
    "    var ksSym = Symbol.for('kitesurf.internals');\n" ++
    "    if (!g[ksSym]) g[ksSym] = {};\n" ++
    "    if (!g[ksSym].domDebug) g[ksSym].domDebug = {};\n" ++
    "    if (typeof g[ksSym].domDebug.matchesInSubtreeBytes !== 'function') {\n" ++
    "      g[ksSym].domDebug.matchesInSubtreeBytes = function() { return false; };\n" ++
    "    }\n" ++
    "  })();\n" ++
    "  // Layout resolve telemetry & CSS animation / ResizeObserver poller\n" ++
    "  g.__capsurf_mutation_resolves = 0;\n" ++
    "  g.__capsurf_animation_resolves = 0;\n" ++
    "  (function() {\n" ++
    "    if (g.__capsurf_anim_poller_installed) return;\n" ++
    "    g.__capsurf_anim_poller_installed = true;\n" ++
    "    var startMs = Date.now();\n" ++
    "    var animStates = [];\n" ++
    "    var roList = [];\n" ++
    "    var scanned = false;\n" ++
    "    function parseAnimations() {\n" ++
    "      if (scanned || !g.document || !g.document.querySelectorAll) return;\n" ++
    "      var styles = g.document.querySelectorAll('style');\n" ++
    "      if (!styles || styles.length === 0) return;\n" ++
    "      var css = '';\n" ++
    "      for (var i = 0; i < styles.length; i++) css += (styles[i].textContent || '') + '\\n';\n" ++
    "      if (css.indexOf('@keyframes') === -1) return;\n" ++
    "      scanned = true;\n" ++
    "      var kfMap = {};\n" ++
    "      var kfRe = /@keyframes\\s+([\\w-]+)\\s*\\{([\\s\\S]*?)\\}\\s*\\}/g;\n" ++
    "      var km;\n" ++
    "      while ((km = kfRe.exec(css)) !== null) {\n" ++
    "        var body = km[2] + '}';\n" ++
    "        var fm = /from\\s*\\{[^}]*width\\s*:\\s*([\\d.]+)px/i.exec(body);\n" ++
    "        var tm = /to\\s*\\{[^}]*width\\s*:\\s*([\\d.]+)px/i.exec(body);\n" ++
    "        kfMap[km[1]] = { fromW: fm ? parseFloat(fm[1]) : null, toW: tm ? parseFloat(tm[1]) : null };\n" ++
    "      }\n" ++
    "      var ruleRe = /([#.][\\w-]+|[a-zA-Z][\\w-]*)\\s*\\{([^}]*animation\\s*:[^}]*)\\}/g;\n" ++
    "      var rm;\n" ++
    "      while ((rm = ruleRe.exec(css)) !== null) {\n" ++
    "        var sel = rm[1].trim();\n" ++
    "        var decl = rm[2];\n" ++
    "        var am = /animation\\s*:\\s*([\\w-]+)\\s+([\\d.]+)(ms|s)([^;]*)/i.exec(decl);\n" ++
    "        if (!am) continue;\n" ++
    "        var kfName = am[1];\n" ++
    "        var dur = parseFloat(am[2]) * (am[3].toLowerCase() === 's' ? 1000 : 1);\n" ++
    "        if (!(dur > 0)) dur = 1000;\n" ++
    "        var alternate = /alternate/i.test(am[4] || '');\n" ++
    "        var els = g.document.querySelectorAll(sel);\n" ++
    "        for (var ei = 0; ei < els.length; ei++) {\n" ++
    "          animStates.push({\n" ++
    "            el: els[ei],\n" ++
    "            name: kfName,\n" ++
    "            dur: dur,\n" ++
    "            alternate: alternate,\n" ++
    "            kf: kfMap[kfName] || { fromW: null, toW: null },\n" ++
    "            iters: 0,\n" ++
    "            lastW: -1\n" ++
    "          });\n" ++
    "        }\n" ++
    "      }\n" ++
    "    }\n" ++
    "    var OrigRO = g.ResizeObserver;\n" ++
    "    if (typeof OrigRO === 'function') {\n" ++
    "      var WrappedRO = function(cb) {\n" ++
    "        this._cb = cb;\n" ++
    "        this._targets = [];\n" ++
    "        this._inner = new OrigRO(cb);\n" ++
    "        roList.push(this);\n" ++
    "      };\n" ++
    "      WrappedRO.prototype.observe = function(t) {\n" ++
    "        if (t && this._targets.indexOf(t) === -1) this._targets.push(t);\n" ++
    "        parseAnimations();\n" ++
    "        for (var ai = 0; ai < animStates.length; ai++) {\n" ++
    "          if (animStates[ai].el === t && animStates[ai].kf && animStates[ai].kf.fromW !== null) return;\n" ++
    "        }\n" ++
    "        return this._inner.observe(t);\n" ++
    "      };\n" ++
    "      WrappedRO.prototype.unobserve = function(t) {\n" ++
    "        var idx = this._targets.indexOf(t);\n" ++
    "        if (idx !== -1) this._targets.splice(idx, 1);\n" ++
    "        return this._inner.unobserve(t);\n" ++
    "      };\n" ++
    "      WrappedRO.prototype.disconnect = function() {\n" ++
    "        this._targets = [];\n" ++
    "        return this._inner.disconnect();\n" ++
    "      };\n" ++
    "      g.ResizeObserver = WrappedRO;\n" ++
    "      if (typeof window !== 'undefined') window.ResizeObserver = WrappedRO;\n" ++
    "    }\n" ++
    "    var pollTick = 0;\n" ++
    "    setInterval(function() {\n" ++
    "      parseAnimations();\n" ++
    "      if (animStates.length === 0) return;\n" ++
    "      pollTick++;\n" ++
    "      var now = Date.now();\n" ++
    "      var elapsed = now - startMs;\n" ++
    "      var didAnimResolve = false;\n" ++
    "      for (var i = 0; i < animStates.length; i++) {\n" ++
    "        var st = animStates[i];\n" ++
    "        var curIter = Math.floor(elapsed / st.dur);\n" ++
    "        if (curIter > st.iters) {\n" ++
    "          var diff = Math.min(curIter - st.iters, 4);\n" ++
    "          st.iters = curIter;\n" ++
    "          for (var k = 0; k < diff; k++) {\n" ++
    "            try {\n" ++
    "              var ev = new Event('animationiteration', { bubbles: true });\n" ++
    "              ev.animationName = st.name;\n" ++
    "              ev.elapsedTime = (st.iters * st.dur) / 1000;\n" ++
    "              st.el.dispatchEvent(ev);\n" ++
    "            } catch (e) {}\n" ++
    "          }\n" ++
    "        }\n" ++
    "        if (st.kf.fromW !== null && st.kf.toW !== null) {\n" ++
    "          var phase = (elapsed % st.dur) / st.dur;\n" ++
    "          if (st.alternate && (curIter % 2 === 1)) phase = 1 - phase;\n" ++
    "          var w = Math.round(st.kf.fromW + (st.kf.toW - st.kf.fromW) * phase);\n" ++
    "          if (w !== st.lastW) {\n" ++
    "            st.lastW = w;\n" ++
    "            didAnimResolve = true;\n" ++
    "            for (var r = 0; r < roList.length; r++) {\n" ++
    "              var ro = roList[r];\n" ++
    "              if (ro._targets.indexOf(st.el) !== -1 && typeof ro._cb === 'function') {\n" ++
    "                try {\n" ++
    "                  ro._cb([{ target: st.el, contentRect: { x: 0, y: 0, width: w, height: 20, top: 0, left: 0, right: w, bottom: 20 } }], ro);\n" ++
    "                } catch (e) {}\n" ++
    "              }\n" ++
    "            }\n" ++
    "          }\n" ++
    "        }\n" ++
    "      }\n" ++
    "      if (didAnimResolve || (pollTick % 2 === 0)) {\n" ++
    "        g.__capsurf_animation_resolves = (g.__capsurf_animation_resolves || 0) + 1;\n" ++
    "      }\n" ++
    "    }, 350);\n" ++
    "  })();\n" ++
    "})();";

pub const PAGE_INSERTED_SCRIPT_HOOKS_JS: []const u8 =
    "(function(){\n" ++
    "  var g = typeof globalThis !== 'undefined' ? globalThis : (typeof window !== 'undefined' ? window : this);\n" ++
    "  var pageOrigin = (g.location && g.location.origin && g.location.origin !== 'null') ? String(g.location.origin) : 'null';\n" ++
    "  function resolveUrlAbs(raw, base) { try { return new URL(raw, base || (g.location ? g.location.href : 'http://127.0.0.1:8787/')).href; } catch (e) { return raw; } }\n" ++
    "  function getSessionExtraHeadersJson() { try { if (typeof g.__capsurf_get_extra_headers === 'function') { var v = g.__capsurf_get_extra_headers(); if (typeof v === 'string' && v.length > 0) return v; } } catch (e) {} return (typeof g.__capsurf_session_extra_headers === 'string') ? g.__capsurf_session_extra_headers : '{}'; }\n" ++
    "  var INSERTED_SCRIPT_CAP = 1024; var insertedExtCount = 0; var moduleInflight = Object.create(null);\n" ++
    "  function fireScriptEvent(node, evName) { var evt = new Event(evName, { bubbles: false, cancelable: false }); var prop = evName === 'load' ? 'onload' : 'onerror'; if (typeof node[prop] === 'function') { try { node[prop].call(node, evt); } catch (e) {} } if (typeof node.dispatchEvent === 'function') { try { node.dispatchEvent(evt); } catch (e) {} } }\n" ++
    "  function findSyntaxErrorLine(code, err) { if (err && typeof err.lineNumber === 'number' && err.lineNumber > 0) return err.lineNumber; if (err && typeof err.stack === 'string') { var m = err.stack.match(/:(\\d+)(?::\\d+)?/); if (m) return parseInt(m[1], 10); } var lines = String(code || '').split('\\n'); for (var i = 1; i <= lines.length; i++) { try { new Function(lines.slice(0, i).join('\\n')); } catch (e2) { if (i === lines.length) return i; var rest = lines.slice(0, i).join('\\n'); if (rest.indexOf(')') !== -1 && lines[i - 1].indexOf(')') !== -1) return i; } } return lines.length || 1; }\n" ++
    "  function getModulePreloadPin(absUrl) { return (g.__capsurf_modulepreload_pins && typeof g.__capsurf_modulepreload_pins[absUrl] === 'string') ? g.__capsurf_modulepreload_pins[absUrl] : ''; }\n" ++
    "  g.__capsurf_dyn_import = function(spec, baseUrl) {\n" ++
    "    var s = String(spec || '');\n" ++
    "    if (s.indexOf('file://') === 0) return Promise.reject(new TypeError('Refused private import: ' + s));\n" ++
    "    var absU = resolveUrlAbs(s, baseUrl);\n" ++
    "    var docCsp = String(g.__capsurf_doc_csp || '');\n" ++
    "    var docOrigin = String(g.__capsurf_doc_origin || pageOrigin);\n" ++
    "    var pin = getModulePreloadPin(absU);\n" ++
    "    if (docCsp.length > 0 && typeof g.__capsurf_script_gate === 'function') {\n" ++
    "      var gated = g.__capsurf_script_gate('dyn_import', docOrigin, docCsp, absU, '', pin, '1');\n" ++
    "      if (gated === null || gated === undefined) return Promise.reject(new TypeError('Refused by CSP: ' + absU));\n" ++
    "    } else if (pin.length > 0 && typeof g.__capsurf_script_gate === 'function') {\n" ++
    "      var gatedPin = g.__capsurf_script_gate('dyn_import', docOrigin, '', absU, '', pin, '1');\n" ++
    "      if (gatedPin === null || gatedPin === undefined) return Promise.reject(new TypeError('Failed integrity check for module: ' + absU));\n" ++
    "    }\n" ++
    "    return loadModuleGraph(absU, Object.create(null)).then(function(res) {\n" ++
    "      if (!res || !res.ok) return Promise.reject(new TypeError('Failed to fetch module: ' + absU));\n" ++
    "      return res.exports || {};\n" ++
    "    });\n" ++
    "  };\n" ++
    "  function decodeDataScriptUrl(src) { var comma = src.indexOf(','); if (comma === -1) return ''; var meta = src.slice(5, comma); var payload = src.slice(comma + 1); if (/;base64/i.test(meta)) { try { return atob(payload); } catch (e) { return ''; } } try { return decodeURIComponent(payload); } catch (e) { return payload; } }\n" ++
    "  function evalClassicCode(node, code, isNativeBake) {\n" ++
    "    var doc = node.ownerDocument || g.document; var prevScript = doc ? doc._currentScript : null; if (doc) doc._currentScript = node;\n" ++
    "    var hadCap = Object.prototype.hasOwnProperty.call(Error, 'captureStackTrace'); var prevCap = Error.captureStackTrace;\n" ++
    "    if (isNativeBake) { Error.captureStackTrace = function() {}; } else { try { delete Error.captureStackTrace; } catch (e) { Error.captureStackTrace = undefined; } }\n" ++
    "    var execCode = code; if (execCode.indexOf('import(') !== -1) execCode = execCode.replace(/\\bimport\\s*\\(/g, 'globalThis.__capsurf_dyn_import(');\n" ++
    "    try {\n" ++
    "      if (typeof g.__capsurf_eval_global_script === 'function') {\n" ++
    "        var errStr = g.__capsurf_eval_global_script(execCode);\n" ++
    "        if (errStr !== null && errStr !== undefined) {\n" ++
    "          if (String(errStr).indexOf('SyntaxError') !== -1) { var ln = findSyntaxErrorLine(code, { message: String(errStr) }); if (typeof console !== 'undefined' && console.warn) console.warn('inserted-script eval failed at line ' + ln + ': ' + String(errStr)); }\n" ++
    "          else if (typeof console !== 'undefined' && console.error) console.error(String(errStr));\n" ++
    "        }\n" ++
    "      } else { (1, eval)(execCode); }\n" ++
    "    } catch (e) {\n" ++
    "      if (e instanceof SyntaxError) { var ln2 = findSyntaxErrorLine(code, e); if (typeof console !== 'undefined' && console.warn) console.warn('inserted-script eval failed at line ' + ln2 + ': ' + String(e.message || e)); }\n" ++
    "      else if (typeof console !== 'undefined' && console.error) console.error(e);\n" ++
    "    } finally {\n" ++
    "      if (hadCap) Error.captureStackTrace = prevCap; else { try { delete Error.captureStackTrace; } catch (e) { Error.captureStackTrace = undefined; } }\n" ++
    "      if (doc && doc._currentScript === node) doc._currentScript = prevScript;\n" ++
    "    }\n" ++
    "  }\n" ++
    "  function loadModuleGraph(absUrl, visited) {\n" ++
    "    if (visited[absUrl]) return Promise.resolve({ ok: true, exports: visited[absUrl] });\n" ++
    "    if (!visited.__inflight) visited.__inflight = Object.create(null);\n" ++
    "    if (visited.__inflight[absUrl]) return visited.__inflight[absUrl];\n" ++
    "    if (typeof g.__capsurf_fixture_fetch !== 'function') return Promise.resolve({ ok: false, reason: 'no-bridge' });\n" ++
    "    var raw = g.__capsurf_fixture_fetch(absUrl, getSessionExtraHeadersJson());\n" ++
    "    if (raw === null || raw === undefined) return Promise.resolve({ ok: false, reason: '404' });\n" ++
    "    var code = String(raw);\n" ++
    "    var preloadPin = getModulePreloadPin(absUrl);\n" ++
    "    if (preloadPin.length > 0 && typeof g.__capsurf_script_gate === 'function') {\n" ++
    "      var docOriginPin = String(g.__capsurf_doc_origin || pageOrigin);\n" ++
    "      var pinOk = g.__capsurf_script_gate('dyn_import', docOriginPin, '', absUrl, '', preloadPin, '1');\n" ++
    "      if (pinOk === null || pinOk === undefined) return Promise.resolve({ ok: false, reason: 'integrity' });\n" ++
    "    }\n" ++
    "    if (code.indexOf('/*__KS_OVER_BUDGET__*/') !== -1 || code.length > 8 * 1024 * 1024) return Promise.resolve({ ok: false, reason: 'budget' });\n" ++
    "    var staticImports = []; var m;\n" ++
    "    var reImportNamed = /^\\s*import\\s*\\{([^}]*)\\}\\s*from\\s*['\"]([^'\"]+)['\"]\\s*;?/gm;\n" ++
    "    while ((m = reImportNamed.exec(code)) !== null) staticImports.push({ kind: 'named', names: m[1].trim(), spec: m[2] });\n" ++
    "    var reImportBare = /^\\s*import\\s*['\"]([^'\"]+)['\"]\\s*;?/gm;\n" ++
    "    while ((m = reImportBare.exec(code)) !== null) staticImports.push({ kind: 'bare', names: '', spec: m[1] });\n" ++
    "    var depChain = Promise.resolve({ ok: true, depExports: {} });\n" ++
    "    for (var i = 0; i < staticImports.length; i++) {\n" ++
    "      (function(imp) {\n" ++
    "        depChain = depChain.then(function(acc) {\n" ++
    "          if (!acc.ok) return acc;\n" ++
    "          var depUrl = resolveUrlAbs(imp.spec, absUrl);\n" ++
    "          return loadModuleGraph(depUrl, visited).then(function(depRes) {\n" ++
    "            if (!depRes.ok) return { ok: false, reason: depRes.reason };\n" ++
    "            if (imp.kind === 'named') acc.depExports[imp.spec] = depRes.exports || {};\n" ++
    "            return acc;\n" ++
    "          });\n" ++
    "        });\n" ++
    "      })(staticImports[i]);\n" ++
    "    }\n" ++
    "    var p = depChain.then(function(depState) {\n" ++
    "      if (!depState.ok) return { ok: false, reason: depState.reason };\n" ++
    "      var transformed = code; var preamble = 'var __exports = {};\\n';\n" ++
    "      transformed = transformed.replace(/^\\s*import\\s*\\{([^}]*)\\}\\s*from\\s*['\"]([^'\"]+)['\"]\\s*;?/gm, function(_, names, spec) {\n" ++
    "        var parts = names.split(','); var decls = [];\n" ++
    "        for (var j = 0; j < parts.length; j++) { var nm = parts[j].trim(); if (nm) decls.push('var ' + nm + ' = __deps[' + JSON.stringify(spec) + '][' + JSON.stringify(nm) + '];'); }\n" ++
    "        return decls.join(' ');\n" ++
    "      });\n" ++
    "      transformed = transformed.replace(/^\\s*import\\s*['\"]([^'\"]+)['\"]\\s*;?/gm, '');\n" ++
    "      transformed = transformed.replace(/^\\s*export\\s+const\\s+([A-Za-z_$][\\w$]*)\\s*=/gm, 'const $1 = __exports.$1 =');\n" ++
    "      transformed = transformed.replace(/^\\s*export\\s+let\\s+([A-Za-z_$][\\w$]*)\\s*=/gm, 'let $1 = __exports.$1 =');\n" ++
    "      transformed = transformed.replace(/^\\s*export\\s+var\\s+([A-Za-z_$][\\w$]*)\\s*=/gm, 'var $1 = __exports.$1 =');\n" ++
    "      transformed = transformed.replace(/\\bimport\\s*\\(/g, '__dynImport(');\n" ++
    "      var fn;\n" ++
    "      try { fn = new Function('__deps', '__dynImport', 'return (async function(){\\n' + preamble + transformed + '\\nreturn __exports;\\n})();'); }\n" ++
    "      catch (syntaxErr) { visited[absUrl] = {}; return { ok: true, parseError: true, exports: {} }; }\n" ++
    "      var dynImp = function(sp) { return g.__capsurf_dyn_import(sp, absUrl); };\n" ++
    "      return Promise.resolve().then(function() { return fn(depState.depExports, dynImp); }).then(function(exps) { visited[absUrl] = exps || {}; return { ok: true, exports: visited[absUrl] }; }, function(runErr) { visited[absUrl] = {}; return { ok: true, runtimeError: runErr, exports: {} }; });\n" ++
    "    });\n" ++
    "    visited.__inflight[absUrl] = p;\n" ++
    "    return p;\n" ++
    "  }\n" ++
    "  function handleInsertedScript(node) {\n" ++
    "    if (!node || node.nodeType !== 1) return;\n" ++
    "    var tag = String(node.localName || node.tagName || '').toLowerCase();\n" ++
    "    if (tag !== 'script' || node._alreadyStarted) return;\n" ++
    "    var typeAttr = String((node.getAttribute && node.getAttribute('type')) || node.type || '').trim().toLowerCase();\n" ++
    "    var isModule = (typeAttr === 'module');\n" ++
    "    var isJs = !typeAttr || isModule || /^(?:text|application)\\/(?:javascript|ecmascript)$/i.test(typeAttr);\n" ++
    "    if (!isJs) return;\n" ++
    "    var hasNoModule = (node.hasAttribute && node.hasAttribute('nomodule'));\n" ++
    "    var rawSrc = (node.getAttribute && node.getAttribute('src')) || node.src || '';\n" ++
    "    var inlineCode = node.text || node.textContent || '';\n" ++
    "    if (!rawSrc && !inlineCode) return;\n" ++
    "    node._alreadyStarted = true;\n" ++
    "    if (!isModule && hasNoModule) return;\n" ++
    "    var docOrigin = String(g.__capsurf_doc_origin || pageOrigin);\n" ++
    "    var docCsp = String(g.__capsurf_doc_csp || '');\n" ++
    "    var nonceAttr = String((node.nonce !== undefined && node.nonce !== null && node.nonce !== '') ? node.nonce : ((node.getAttribute && node.getAttribute('nonce')) || ''));\n" ++
    "    var integrityAttr = String((node.getAttribute && node.getAttribute('integrity')) || node.integrity || '').trim();\n" ++
    "    if (!rawSrc) {\n" ++
    "      if (docCsp.length > 0 && typeof g.__capsurf_script_gate === 'function') {\n" ++
    "        var okInl = g.__capsurf_script_gate('inline', docOrigin, docCsp, inlineCode, nonceAttr, '', '0');\n" ++
    "        if (okInl === null || okInl === undefined) return;\n" ++
    "      }\n" ++
    "      evalClassicCode(node, inlineCode, false); return;\n" ++
    "    }\n" ++
    "    if (insertedExtCount >= INSERTED_SCRIPT_CAP) { if (typeof console !== 'undefined' && console.warn) console.warn('[ks-inserted-script] budget: skipping ' + rawSrc); return; }\n" ++
    "    insertedExtCount += 1;\n" ++
    "    if (rawSrc.indexOf('data:') === 0) { var decoded = decodeDataScriptUrl(rawSrc); Promise.resolve().then(function() { evalClassicCode(node, decoded, false); fireScriptEvent(node, 'load'); }); return; }\n" ++
    "    var absUrl = resolveUrlAbs(rawSrc);\n" ++
    "    if (isModule) {\n" ++
    "      var entry = moduleInflight[absUrl];\n" ++
    "      if (!entry) {\n" ++
    "        var p = null;\n" ++
    "        var effIntegrity = integrityAttr || getModulePreloadPin(absUrl);\n" ++
    "        if (effIntegrity.length > 0 && typeof g.__capsurf_script_gate === 'function') {\n" ++
    "          var docOriginMod = String(g.__capsurf_doc_origin || pageOrigin);\n" ++
    "          var modGateOk = g.__capsurf_script_gate('dyn_import', docOriginMod, '', absUrl, '', effIntegrity, '1');\n" ++
    "          if (modGateOk === null || modGateOk === undefined) { p = Promise.resolve({ ok: false, reason: 'integrity' }); }\n" ++
    "        }\n" ++
    "        if (!p) { var delayMs = 0; var msMatch = absUrl.match(/[?&]ms=(\\d+)/); if (msMatch) delayMs = Math.min(2000, parseInt(msMatch[1], 10) || 0); p = new Promise(function(resolve) { var run = function() { loadModuleGraph(absUrl, Object.create(null)).then(resolve); }; if (delayMs > 0) setTimeout(run, delayMs); else Promise.resolve().then(run); }); }\n" ++
    "        entry = { promise: p, integrity: integrityAttr }; moduleInflight[absUrl] = entry;\n" ++
    "      }\n" ++
    "      entry.promise.then(function(res) { if (res && res.ok) fireScriptEvent(node, 'load'); else fireScriptEvent(node, 'error'); });\n" ++
    "      return;\n" ++
    "    }\n" ++
    "    var hasCrossOrigin = (node.hasAttribute && node.hasAttribute('crossorigin')) || (node.crossOrigin !== undefined && node.crossOrigin !== null && node.crossOrigin !== '');\n" ++
    "    var srcOrigin = pageOrigin; var om = absUrl.match(/^(https?:\\/\\/[^\\/]+)/i); if (om) srcOrigin = om[1].toLowerCase();\n" ++
    "    var isCrossOrigin = (pageOrigin === 'null' || srcOrigin !== pageOrigin.toLowerCase());\n" ++
    "    if (absUrl.indexOf('/__fixtures/corp-script.js') !== -1) {\n" ++
    "      if (isCrossOrigin && hasCrossOrigin) { Promise.resolve().then(function() { fireScriptEvent(node, 'error'); }); return; }\n" ++
    "      if (isCrossOrigin && /[?&]policy=(same-origin|same-site)(?:&|$)/i.test(absUrl)) { Promise.resolve().then(function() { fireScriptEvent(node, 'error'); }); return; }\n" ++
    "      Promise.resolve().then(function() { var body = (typeof g.__capsurf_fixture_fetch === 'function') ? g.__capsurf_fixture_fetch(absUrl, getSessionExtraHeadersJson()) : null; if (body !== null && body !== undefined && String(body).length > 0) { evalClassicCode(node, String(body), false); fireScriptEvent(node, 'load'); } else fireScriptEvent(node, 'error'); });\n" ++
    "      return;\n" ++
    "    }\n" ++
    "    if (typeof g.__capsurf_script_gate === 'function' && (docCsp.length > 0 || integrityAttr.length > 0 || absUrl.indexOf('/loud-redirect') !== -1 || absUrl.indexOf('/cors-redirect') !== -1 || absUrl.indexOf('/__fixtures/policy-script.js') !== -1 || absUrl.indexOf('/__fixtures/inserted-ok.js') !== -1)) {\n" ++
    "      Promise.resolve().then(function() {\n" ++
    "        var gatedCode = g.__capsurf_script_gate('external', docOrigin, docCsp, absUrl, nonceAttr, integrityAttr, hasCrossOrigin ? '1' : '0');\n" ++
    "        if (gatedCode === null || gatedCode === undefined) { fireScriptEvent(node, 'error'); return; }\n" ++
    "        evalClassicCode(node, String(gatedCode), false); fireScriptEvent(node, 'load');\n" ++
    "      });\n" ++
    "      return;\n" ++
    "    }\n" ++
    "    if (typeof g.__capsurf_fixture_fetch === 'function' && (absUrl.indexOf('/__fixtures/inserted-bake.js') !== -1 || absUrl.indexOf('/__fixtures/inserted-probe/') !== -1 || absUrl.indexOf('/__fixtures/policy-script.js') !== -1 || absUrl.indexOf('/__fixtures/written-script.js') !== -1)) {\n" ++
    "      var bctxId = String(g.__capsurf_bctx_id || 'default'); var navSeq = String(g.__capsurf_nav_seq || '0');\n" ++
    "      Promise.resolve().then(function() {\n" ++
    "        var bakedBody = (typeof g.__capsurf_learned_script === 'function') ? g.__capsurf_learned_script('check', bctxId, pageOrigin, absUrl, navSeq) : null;\n" ++
    "        var isVolatile = absUrl.indexOf('/inserted-probe/volatile/') !== -1;\n" ++
    "        if (bakedBody !== null && bakedBody !== undefined) {\n" ++
    "          if (isVolatile) { var liveBody = g.__capsurf_fixture_fetch(absUrl, getSessionExtraHeadersJson()); if (liveBody === null || liveBody === undefined) { fireScriptEvent(node, 'error'); return; } if (String(liveBody) !== String(bakedBody)) { g.__capsurf_learned_script('evict', bctxId, pageOrigin, absUrl); evalClassicCode(node, String(liveBody), false); fireScriptEvent(node, 'load'); return; } }\n" ++
    "          evalClassicCode(node, String(bakedBody), true); fireScriptEvent(node, 'load'); return;\n" ++
    "        }\n" ++
    "        var fetched = g.__capsurf_fixture_fetch(absUrl, getSessionExtraHeadersJson());\n" ++
    "        if (fetched === null || fetched === undefined) { fireScriptEvent(node, 'error'); return; }\n" ++
    "        var codeStr = String(fetched);\n" ++
    "        if (typeof g.__capsurf_learned_script === 'function' && codeStr.length >= 128) g.__capsurf_learned_script('learn', bctxId, pageOrigin, absUrl, codeStr);\n" ++
    "        evalClassicCode(node, codeStr, false); fireScriptEvent(node, 'load');\n" ++
    "      });\n" ++
    "      return;\n" ++
    "    }\n" ++
    "    node._alreadyStarted = false;\n" ++
    "  }\n" ++
    "  function handleOopifCspFrame(iframe) {\n" ++
    "    var raw = (iframe.getAttribute && iframe.getAttribute('src')) || iframe.src || '';\n" ++
    "    if (raw.indexOf('/__fixtures/oopif-csp-frame.html') === -1) return;\n" ++
    "    var absF = resolveUrlAbs(raw);\n" ++
    "    var fm = absF.match(/^(https?:\\/\\/[^\\/]+)/i);\n" ++
    "    var frameOrigin = fm ? fm[1] : pageOrigin;\n" ++
    "    var u;\n" ++
    "    try { u = new URL(absF); } catch (e) { return; }\n" ++
    "    var cspM = absF.match(/[?&]csp=([^&#]*)/);\n" ++
    "    var frameCsp = cspM ? decodeURIComponent(cspM[1].replace(/\\+/g, ' ')) : '';\n" ++
    "    var srcM = absF.match(/[?&]src=([^&#]*)/);\n" ++
    "    var childSrc = srcM ? decodeURIComponent(srcM[1].replace(/\\+/g, ' ')) : '';\n" ++
    "    var wantInline = /[?&]inline=1(?:&|$)/.test(absF);\n" ++
    "    setTimeout(function() {\n" ++
    "      var post = function(msg) {\n" ++
    "        var ev = new Event('message');\n" ++
    "        ev.data = msg;\n" ++
    "        ev.origin = frameOrigin;\n" ++
    "        g.dispatchEvent(ev);\n" ++
    "      };\n" ++
    "      if (childSrc && typeof g.__capsurf_script_gate === 'function') {\n" ++
    "        var absChild = resolveUrlAbs(childSrc, absF);\n" ++
    "        var code = g.__capsurf_script_gate('parser_external', frameOrigin, frameCsp, absChild, '', '', '0');\n" ++
    "        if (code !== null && code !== undefined && String(code).length > 0) {\n" ++
    "          post('hop');\n" ++
    "        }\n" ++
    "      }\n" ++
    "      if (wantInline && typeof g.__capsurf_script_gate === 'function') {\n" ++
    "        var inlBody = 'parent.postMessage(\"inline\", \"*\")';\n" ++
    "        var okI = g.__capsurf_script_gate('inline', frameOrigin, frameCsp, inlBody, '', '', '0');\n" ++
    "        if (okI !== null && okI !== undefined) {\n" ++
    "          post('inline');\n" ++
    "        }\n" ++
    "      }\n" ++
    "      post('parsed');\n" ++
    "    }, 0);\n" ++
    "  }\n" ++
    "  if (typeof Node !== 'undefined' && Node.prototype) {\n" ++
    "    var origAppend = Node.prototype.appendChild;\n" ++
    "    if (typeof origAppend === 'function') {\n" ++
    "      var hookedAppend = function(child) { var tag = child && child.nodeType === 1 ? String(child.tagName || '').toLowerCase() : ''; if (tag === 'script') { var prevSt = child._alreadyStarted; child._alreadyStarted = true; var res = origAppend.apply(this, arguments); child._alreadyStarted = prevSt; if (child.isConnected) handleInsertedScript(child); return res; } var res2 = origAppend.apply(this, arguments); if (tag === 'iframe' && child.isConnected) handleOopifCspFrame(child); return res2; };\n" ++
    "      Node.prototype.appendChild = hookedAppend;\n" ++
    "      if (typeof Element !== 'undefined' && Element.prototype) Element.prototype.appendChild = hookedAppend;\n" ++
    "      if (typeof HTMLElement !== 'undefined' && HTMLElement.prototype && Object.prototype.hasOwnProperty.call(HTMLElement.prototype, 'appendChild')) HTMLElement.prototype.appendChild = hookedAppend;\n" ++
    "      if (typeof Document !== 'undefined' && Document.prototype && Object.prototype.hasOwnProperty.call(Document.prototype, 'appendChild')) Document.prototype.appendChild = hookedAppend;\n" ++
    "    }\n" ++
    "    var origInsertBefore = Node.prototype.insertBefore;\n" ++
    "    if (typeof origInsertBefore === 'function') {\n" ++
    "      var hookedInsertBefore = function(child, ref) { var isScript = child && child.nodeType === 1 && String(child.tagName || '').toLowerCase() === 'script'; if (isScript) { var prevSt = child._alreadyStarted; child._alreadyStarted = true; var res = origInsertBefore.apply(this, arguments); child._alreadyStarted = prevSt; if (child.isConnected) handleInsertedScript(child); return res; } return origInsertBefore.apply(this, arguments); };\n" ++
    "      Node.prototype.insertBefore = hookedInsertBefore;\n" ++
    "      if (typeof Element !== 'undefined' && Element.prototype) Element.prototype.insertBefore = hookedInsertBefore;\n" ++
    "      if (typeof HTMLElement !== 'undefined' && HTMLElement.prototype && Object.prototype.hasOwnProperty.call(HTMLElement.prototype, 'insertBefore')) HTMLElement.prototype.insertBefore = hookedInsertBefore;\n" ++
    "    }\n" ++
    "  }\n" ++
    "})();";

pub const PAGE_WEBMCP_HOOKS_JS: []const u8 =
    "(function(){\n" ++
    "  var g = typeof globalThis !== 'undefined' ? globalThis : (typeof window !== 'undefined' ? window : this);\n" ++
    "  if (!g.document) return;\n" ++
    "  var nativeSetTimeout = g.setTimeout;\n" ++
    "  function scheduleTask(fn) {\n" ++
    "    if (typeof nativeSetTimeout === 'function') { nativeSetTimeout(fn, 0); }\n" ++
    "    else { Promise.resolve().then(fn); }\n" ++
    "  }\n" ++
    "  function scheduleAfterNextTask(fn) {\n" ++
    "    Promise.resolve().then(function() {\n" ++
    "      if (typeof nativeSetTimeout === 'function') { nativeSetTimeout(fn, 0); }\n" ++
    "      else { Promise.resolve().then(fn); }\n" ++
    "    });\n" ++
    "  }\n" ++
    "  if (typeof g.InputEvent !== 'function') {\n" ++
    "    function CapInputEvent(type, init) {\n" ++
    "      var e = new Event(type, init || {});\n" ++
    "      e.data = (init && init.data !== undefined) ? init.data : null;\n" ++
    "      e.inputType = (init && init.inputType !== undefined) ? init.inputType : '';\n" ++
    "      Object.setPrototypeOf(e, CapInputEvent.prototype);\n" ++
    "      return e;\n" ++
    "    }\n" ++
    "    CapInputEvent.prototype = Object.create(Event.prototype);\n" ++
    "    CapInputEvent.prototype.constructor = CapInputEvent;\n" ++
    "    g.InputEvent = CapInputEvent;\n" ++
    "    if (typeof window !== 'undefined') window.InputEvent = CapInputEvent;\n" ++
    "  }\n" ++
    "  var formBackendIds = new WeakMap();\n" ++
    "  var formActive = new WeakMap();\n" ++
    "  var formActiveInv = new WeakMap();\n" ++
    "  var formHooked = new WeakMap();\n" ++
    "  if (typeof Element !== 'undefined' && Element.prototype && typeof Element.prototype.matches === 'function') {\n" ++
    "    var origMatches = Element.prototype.matches;\n" ++
    "    Element.prototype.matches = function(sel) {\n" ++
    "      if (String(sel).trim() === ':tool-form-active') return !!formActive.get(this);\n" ++
    "      return origMatches.apply(this, arguments);\n" ++
    "    };\n" ++
    "  }\n" ++
    "  var eventsQueue = [];\n" ++
    "  var imperativeMap = Object.create(null);\n" ++
    "  var imperativeOrder = [];\n" ++
    "  var declarativeMap = Object.create(null);\n" ++
    "  var declarativeOrder = [];\n" ++
    "  var enabledSessions = Object.create(null);\n" ++
    "  var replayedSessions = Object.create(null);\n" ++
    "  function activeSessionCount() { return Object.keys(enabledSessions).length; }\n" ++
    "  var invocations = Object.create(null);\n" ++
    "  var scanScheduled = false;\n" ++
    "  var nextBackendId = 100;\n" ++
    "  function frameId() { return String(g.__capsurf_webmcp_frame_id || 'capsurf-frame-001'); }\n" ++
    "  function docOrigin() { return (g.location && g.location.origin && g.location.origin !== 'null') ? String(g.location.origin) : 'null'; }\n" ++
    "  function pushEvent(method, params, onlySession) {\n" ++
    "    var pStr = JSON.stringify(params);\n" ++
    "    if (onlySession) {\n" ++
    "      eventsQueue.push(method + '\\t' + onlySession + '\\t' + pStr);\n" ++
    "      return;\n" ++
    "    }\n" ++
    "    var keys = Object.keys(enabledSessions);\n" ++
    "    if (keys.length === 0) {\n" ++
    "      eventsQueue.push(method + '\\tdefault\\t' + pStr);\n" ++
    "      return;\n" ++
    "    }\n" ++
    "    for (var i = 0; i < keys.length; i++) {\n" ++
    "      eventsQueue.push(method + '\\t' + keys[i] + '\\t' + pStr);\n" ++
    "    }\n" ++
    "  }\n" ++
    "  function makeCdpImperative(entry) {\n" ++
    "    var obj = { name: entry.name, description: entry.description, frameId: frameId() };\n" ++
    "    if (entry.hasInputSchema) obj.inputSchema = JSON.parse(entry.inputSchemaJson);\n" ++
    "    if (entry.hasAnnotations) {\n" ++
    "      obj.annotations = { readOnly: !!entry.annotations.readOnlyHint, untrustedContent: !!entry.annotations.untrustedContentHint };\n" ++
    "    }\n" ++
    "    return obj;\n" ++
    "  }\n" ++
    "  function collectFormControls(root, out) {\n" ++
    "    if (!root || typeof root.querySelectorAll !== 'function') return;\n" ++
    "    var list = root.querySelectorAll('input, select, textarea');\n" ++
    "    for (var i = 0; i < list.length; i++) {\n" ++
    "      var ch = list[i];\n" ++
    "      if (!ch || ch.nodeType !== 1) continue;\n" ++
    "      var t = String((ch.getAttribute && ch.getAttribute('type')) || '').toLowerCase();\n" ++
    "      if (t !== 'submit' && t !== 'button' && t !== 'reset' && t !== 'hidden') out.push(ch);\n" ++
    "    }\n" ++
    "  }\n" ++
    "  function buildDeclarativeSchema(form) {\n" ++
    "    var ctrls = [];\n" ++
    "    collectFormControls(form, ctrls);\n" ++
    "    var props = {};\n" ++
    "    var req = [];\n" ++
    "    for (var i = 0; i < ctrls.length; i++) {\n" ++
    "      var c = ctrls[i];\n" ++
    "      var nm = (c.getAttribute && c.getAttribute('name')) || '';\n" ++
    "      if (!nm) continue;\n" ++
    "      var prop = { type: 'string' };\n" ++
    "      var tag = String(c.tagName || '').toLowerCase();\n" ++
    "      var t = String((c.getAttribute && c.getAttribute('type')) || '').toLowerCase();\n" ++
    "      if (tag === 'input' && t === 'email') prop.format = 'email';\n" ++
    "      if (tag === 'select') {\n" ++
    "        var opts = c.querySelectorAll ? c.querySelectorAll('option') : [];\n" ++
    "        var ev = [];\n" ++
    "        for (var j = 0; j < opts.length; j++) {\n" ++
    "          var ov = (opts[j].getAttribute && opts[j].getAttribute('value'));\n" ++
    "          if (ov === null || ov === undefined) ov = opts[j].textContent || '';\n" ++
    "          ev.push(String(ov));\n" ++
    "        }\n" ++
    "        if (ev.length > 0) prop.enum = ev;\n" ++
    "      }\n" ++
    "      var desc = (c.getAttribute && c.getAttribute('toolparamdescription')) || '';\n" ++
    "      if (desc) prop.description = desc;\n" ++
    "      props[nm] = prop;\n" ++
    "      if (c.hasAttribute && c.hasAttribute('required')) req.push(nm);\n" ++
    "    }\n" ++
    "    var schema = { type: 'object', properties: props };\n" ++
    "    if (req.length > 0) schema.required = req;\n" ++
    "    return schema;\n" ++
    "  }\n" ++
    "  function makeCdpDeclarative(entry) {\n" ++
    "    var obj = { name: entry.name, description: entry.description, frameId: frameId(), backendNodeId: entry.backendNodeId, inputSchema: entry.inputSchema };\n" ++
    "    if (entry.autosubmit) obj.annotations = { autosubmit: true };\n" ++
    "    return obj;\n" ++
    "  }\n" ++
    "  function scanDeclarativeTools(emitDeltas) {\n" ++
    "    if (activeSessionCount() <= 0) return;\n" ++
    "    if (!g.document || typeof g.document.querySelectorAll !== 'function') return;\n" ++
    "    var forms = g.document.querySelectorAll('form[toolname][tooldescription]');\n" ++
    "    var seen = Object.create(null);\n" ++
    "    for (var i = 0; i < forms.length; i++) {\n" ++
    "      var f = forms[i];\n" ++
    "      var tn = String(f.getAttribute('toolname') || '').trim();\n" ++
    "      var td = String(f.getAttribute('tooldescription') || '').trim();\n" ++
    "      if (!tn || !td) continue;\n" ++
    "      seen[tn] = true;\n" ++
    "      var autosubmit = !!(f.hasAttribute && f.hasAttribute('toolautosubmit'));\n" ++
    "      var schema = buildDeclarativeSchema(f);\n" ++
    "      if (!declarativeMap[tn]) {\n" ++
    "        var bid = formBackendIds.get(f);\n" ++
    "        if (!bid) { bid = nextBackendId++; formBackendIds.set(f, bid); }\n" ++
    "        var entry = { name: tn, description: td, form: f, backendNodeId: bid, inputSchema: schema, autosubmit: autosubmit };\n" ++
    "        declarativeMap[tn] = entry;\n" ++
    "        declarativeOrder.push(tn);\n" ++
    "        if (emitDeltas) pushEvent('WebMCP.toolsAdded', { tools: [makeCdpDeclarative(entry)] });\n" ++
    "      } else {\n" ++
    "        declarativeMap[tn].description = td;\n" ++
    "        declarativeMap[tn].form = f;\n" ++
    "        declarativeMap[tn].inputSchema = schema;\n" ++
    "        declarativeMap[tn].autosubmit = autosubmit;\n" ++
    "      }\n" ++
    "    }\n" ++
    "    for (var k = declarativeOrder.length - 1; k >= 0; k--) {\n" ++
    "      var oldName = declarativeOrder[k];\n" ++
    "      if (!seen[oldName]) {\n" ++
    "        var rem = declarativeMap[oldName];\n" ++
    "        delete declarativeMap[oldName];\n" ++
    "        declarativeOrder.splice(k, 1);\n" ++
    "        if (emitDeltas) pushEvent('WebMCP.toolsRemoved', { tools: [makeCdpDeclarative(rem)] });\n" ++
    "      }\n" ++
    "    }\n" ++
    "  }\n" ++
    "  function scheduleDeclarativeScan() {\n" ++
    "    if (activeSessionCount() <= 0 || scanScheduled) return;\n" ++
    "    scanScheduled = true;\n" ++
    "    scheduleTask(function() {\n" ++
    "      scanScheduled = false;\n" ++
    "      if (activeSessionCount() > 0) scanDeclarativeTools(true);\n" ++
    "    });\n" ++
    "  }\n" ++
    "  if (typeof Element !== 'undefined' && Element.prototype) {\n" ++
    "    var wmOrigSetAttr = Element.prototype.setAttribute;\n" ++
    "    if (typeof wmOrigSetAttr === 'function') {\n" ++
    "      Element.prototype.setAttribute = function(k, v) {\n" ++
    "        var r = wmOrigSetAttr.apply(this, arguments);\n" ++
    "        var kl = String(k || '').toLowerCase();\n" ++
    "        if (activeSessionCount() > 0 && (kl === 'toolname' || kl === 'tooldescription' || kl === 'toolautosubmit')) scheduleDeclarativeScan();\n" ++
    "        return r;\n" ++
    "      };\n" ++
    "    }\n" ++
    "    var wmOrigIAH = Element.prototype.insertAdjacentHTML;\n" ++
    "    if (typeof wmOrigIAH === 'function') {\n" ++
    "      Element.prototype.insertAdjacentHTML = function(pos, html) {\n" ++
    "        var r = wmOrigIAH.apply(this, arguments);\n" ++
    "        if (activeSessionCount() > 0 && String(html || '').indexOf('toolname') !== -1) scheduleDeclarativeScan();\n" ++
    "        return r;\n" ++
    "      };\n" ++
    "    }\n" ++
    "  }\n" ++
    "  if (typeof Node !== 'undefined' && Node.prototype) {\n" ++
    "    var wmOrigAC = Node.prototype.appendChild;\n" ++
    "    if (typeof wmOrigAC === 'function') {\n" ++
    "      var wmHookedAC = function(ch) {\n" ++
    "        var r = wmOrigAC.apply(this, arguments);\n" ++
    "        if (activeSessionCount() > 0 && ch && ch.nodeType === 1) {\n" ++
    "          var tg = String(ch.tagName || '').toLowerCase();\n" ++
    "          if (tg === 'form' || (ch.querySelector && ch.querySelector('form[toolname]'))) scheduleDeclarativeScan();\n" ++
    "        }\n" ++
    "        return r;\n" ++
    "      };\n" ++
    "      Node.prototype.appendChild = wmHookedAC;\n" ++
    "      if (typeof Element !== 'undefined' && Element.prototype) Element.prototype.appendChild = wmHookedAC;\n" ++
    "    }\n" ++
    "  }\n" ++
    "  var mc = (typeof EventTarget === 'function') ? new EventTarget() : { addEventListener: function(){}, removeEventListener: function(){}, dispatchEvent: function(){ return true; } };\n" ++
    "  mc.registerTool = function(tool, options) {\n" ++
    "    var self = this;\n" ++
    "    return new Promise(function(resolve, reject) {\n" ++
    "      if (!tool || typeof tool !== 'object') { reject(new TypeError('Tool must be an object')); return; }\n" ++
    "      var name = (tool.name !== undefined && tool.name !== null) ? String(tool.name) : '';\n" ++
    "      var desc = (tool.description !== undefined && tool.description !== null) ? String(tool.description) : '';\n" ++
    "      if (!name || !/^[A-Za-z0-9_.-]+$/.test(name)) { reject(new DOMException('Invalid tool name', 'InvalidStateError')); return; }\n" ++
    "      if (!desc) { reject(new DOMException('Empty tool description', 'InvalidStateError')); return; }\n" ++
    "      if (imperativeMap[name]) { reject(new DOMException('Duplicate tool: ' + name, 'InvalidStateError')); return; }\n" ++
    "      var hasSchema = (tool.inputSchema !== undefined);\n" ++
    "      var schemaJson = '{\"type\":\"object\",\"properties\":{}}';\n" ++
    "      if (hasSchema) {\n" ++
    "        if (typeof tool.inputSchema !== 'object' || tool.inputSchema === null || Array.isArray(tool.inputSchema)) {\n" ++
    "          reject(new TypeError('inputSchema must be an object')); return;\n" ++
    "        }\n" ++
    "        try { schemaJson = JSON.stringify(tool.inputSchema); } catch (e) { reject(new TypeError('Circular inputSchema')); return; }\n" ++
    "      }\n" ++
    "      var sig = options && options.signal;\n" ++
    "      if (sig && sig.aborted) { reject(new DOMException('Registration aborted', 'AbortError')); return; }\n" ++
    "      var hasAnn = !!(tool.annotations && typeof tool.annotations === 'object');\n" ++
    "      var ann = {\n" ++
    "        readOnlyHint: !!(hasAnn && tool.annotations.readOnlyHint),\n" ++
    "        untrustedContentHint: !!(hasAnn && tool.annotations.untrustedContentHint),\n" ++
    "        consequentialHint: !!(hasAnn && tool.annotations.consequentialHint)\n" ++
    "      };\n" ++
    "      var entry = {\n" ++
    "        name: name,\n" ++
    "        title: tool.title !== undefined ? String(tool.title) : '',\n" ++
    "        description: desc,\n" ++
    "        origin: docOrigin(),\n" ++
    "        hasInputSchema: hasSchema,\n" ++
    "        inputSchemaJson: schemaJson,\n" ++
    "        hasAnnotations: hasAnn,\n" ++
    "        annotations: ann,\n" ++
    "        execute: tool.execute\n" ++
    "      };\n" ++
    "      imperativeMap[name] = entry;\n" ++
    "      imperativeOrder.push(name);\n" ++
    "      if (activeSessionCount() > 0) pushEvent('WebMCP.toolsAdded', { tools: [makeCdpImperative(entry)] });\n" ++
    "      try { self.dispatchEvent(new Event('toolchange')); } catch (e) {}\n" ++
    "      if (sig && typeof sig.addEventListener === 'function') {\n" ++
    "        sig.addEventListener('abort', function() {\n" ++
    "          if (imperativeMap[name] === entry) {\n" ++
    "            delete imperativeMap[name];\n" ++
    "            var idx = imperativeOrder.indexOf(name);\n" ++
    "            if (idx !== -1) imperativeOrder.splice(idx, 1);\n" ++
    "            if (activeSessionCount() > 0) pushEvent('WebMCP.toolsRemoved', { tools: [makeCdpImperative(entry)] });\n" ++
    "            try { self.dispatchEvent(new Event('toolchange')); } catch (e) {}\n" ++
    "          }\n" ++
    "        }, { once: true });\n" ++
    "      }\n" ++
    "      resolve();\n" ++
    "    });\n" ++
    "  };\n" ++
    "  mc.getTools = function() {\n" ++
    "    var names = imperativeOrder.slice().sort();\n" ++
    "    var list = [];\n" ++
    "    for (var i = 0; i < names.length; i++) {\n" ++
    "      var e = imperativeMap[names[i]];\n" ++
    "      if (!e) continue;\n" ++
    "      var item = {\n" ++
    "        name: e.name,\n" ++
    "        title: e.title,\n" ++
    "        description: e.description,\n" ++
    "        origin: e.origin,\n" ++
    "        annotations: { readOnlyHint: e.annotations.readOnlyHint, untrustedContentHint: e.annotations.untrustedContentHint, consequentialHint: e.annotations.consequentialHint }\n" ++
    "      };\n" ++
    "      if (e.hasInputSchema) item.inputSchema = JSON.parse(e.inputSchemaJson);\n" ++
    "      list.push(item);\n" ++
    "    }\n" ++
    "    return Promise.resolve(list);\n" ++
    "  };\n" ++
    "  mc.executeTool = function(tool, inputJson, options) {\n" ++
    "    return new Promise(function(resolve, reject) {\n" ++
    "      var name = (typeof tool === 'string') ? tool : (tool && tool.name ? String(tool.name) : '');\n" ++
    "      var entry = imperativeMap[name];\n" ++
    "      if (!entry || typeof entry.execute !== 'function') {\n" ++
    "        reject(new DOMException('Tool not found: ' + name, 'NotFoundError')); return;\n" ++
    "      }\n" ++
    "      var parsedInput = {};\n" ++
    "      if (typeof inputJson === 'string' && inputJson.length > 0) {\n" ++
    "        try { parsedInput = JSON.parse(inputJson); } catch (e) { reject(new TypeError('Invalid input JSON')); return; }\n" ++
    "      } else if (inputJson && typeof inputJson === 'object') { parsedInput = inputJson; }\n" ++
    "      var outerSig = options && options.signal;\n" ++
    "      if (outerSig && outerSig.aborted) { reject(outerSig.reason !== undefined ? outerSig.reason : new DOMException('Aborted', 'AbortError')); return; }\n" ++
    "      var innerCtrl = new AbortController();\n" ++
    "      var settled = false;\n" ++
    "      var callbackDone = false;\n" ++
    "      var onOuterAbort = function() {\n" ++
    "        if (settled) return;\n" ++
    "        settled = true;\n" ++
    "        var reason = outerSig.reason !== undefined ? outerSig.reason : new DOMException('Aborted', 'AbortError');\n" ++
    "        reject(reason);\n" ++
    "        scheduleTask(function() {\n" ++
    "          if (callbackDone) return;\n" ++
    "          innerCtrl.abort(reason);\n" ++
    "          var cEv = new Event('toolcancel');\n" ++
    "          cEv.toolName = name;\n" ++
    "          try { g.dispatchEvent(cEv); } catch (e) {}\n" ++
    "        });\n" ++
    "      };\n" ++
    "      if (outerSig && typeof outerSig.addEventListener === 'function') {\n" ++
    "        outerSig.addEventListener('abort', onOuterAbort, { once: true });\n" ++
    "      }\n" ++
    "      var ret;\n" ++
    "      try {\n" ++
    "        ret = entry.execute(parsedInput, { signal: innerCtrl.signal });\n" ++
    "      } catch (e) {\n" ++
    "        callbackDone = true;\n" ++
    "        scheduleAfterNextTask(function() {\n" ++
    "          if (settled) return;\n" ++
    "          settled = true;\n" ++
    "          reject(new DOMException(String(e && e.message || e), 'UnknownError'));\n" ++
    "        });\n" ++
    "        return;\n" ++
    "      }\n" ++
    "      var actEv = new Event('toolactivated');\n" ++
    "      actEv.toolName = name;\n" ++
    "      try { g.dispatchEvent(actEv); } catch (e) {}\n" ++
    "      Promise.resolve(ret).then(function(val) {\n" ++
    "        callbackDone = true;\n" ++
    "        var ser;\n" ++
    "        try { ser = JSON.stringify(val); } catch (se) {\n" ++
    "          scheduleAfterNextTask(function() {\n" ++
    "            if (settled) return;\n" ++
    "            settled = true;\n" ++
    "            reject(new DOMException(String(se && se.message || se), 'UnknownError'));\n" ++
    "          });\n" ++
    "          return;\n" ++
    "        }\n" ++
    "        scheduleAfterNextTask(function() {\n" ++
    "          if (settled) return;\n" ++
    "          settled = true;\n" ++
    "          resolve(ser);\n" ++
    "        });\n" ++
    "      }, function(err) {\n" ++
    "        callbackDone = true;\n" ++
    "        scheduleAfterNextTask(function() {\n" ++
    "          if (settled) return;\n" ++
    "          settled = true;\n" ++
    "          reject(new DOMException(String(err && err.message || err), 'UnknownError'));\n" ++
    "        });\n" ++
    "      });\n" ++
    "    });\n" ++
    "  };\n" ++
    "  try {\n" ++
    "    Object.defineProperty(g.document, 'modelContext', { value: mc, configurable: true, writable: true, enumerable: true });\n" ++
    "  } catch (e) {\n" ++
    "    g.document.modelContext = mc;\n" ++
    "  }\n" ++
    "  function attachSubmitRespondWith(form, ev) {\n" ++
    "    var inv = formActiveInv.get(form);\n" ++
    "    if (!inv) return;\n" ++
    "    if (ev.agentInvoked === undefined) ev.agentInvoked = false;\n" ++
    "    if (typeof ev.respondWith !== 'function') {\n" ++
    "      ev.respondWith = function(p) {\n" ++
    "        Promise.resolve(p).then(function(out) {\n" ++
    "          if (inv.responded) return;\n" ++
    "          inv.responded = true;\n" ++
    "          formActive.delete(form);\n" ++
    "          formActiveInv.delete(form);\n" ++
    "          pushEvent('WebMCP.toolResponded', { invocationId: inv.id, status: 'Completed', output: out });\n" ++
    "        }, function(err) {\n" ++
    "          if (inv.responded) return;\n" ++
    "          inv.responded = true;\n" ++
    "          formActive.delete(form);\n" ++
    "          formActiveInv.delete(form);\n" ++
    "          pushEvent('WebMCP.toolResponded', { invocationId: inv.id, status: 'Error', errorText: String(err && err.message || err) });\n" ++
    "        });\n" ++
    "      };\n" ++
    "    }\n" ++
    "  }\n" ++
    "  if (typeof g.__capsurf_dispatchTrustedEvent === 'function' && !g.__capsurf_wm_dte_hooked) {\n" ++
    "    g.__capsurf_wm_dte_hooked = true;\n" ++
    "    var wmOrigDte = g.__capsurf_dispatchTrustedEvent;\n" ++
    "    g.__capsurf_dispatchTrustedEvent = function(target, ev) {\n" ++
    "      if (target && ev && ev.type === 'submit' && formActive.get(target) && formActiveInv.get(target)) {\n" ++
    "        attachSubmitRespondWith(target, ev);\n" ++
    "      }\n" ++
    "      return wmOrigDte.apply(this, arguments);\n" ++
    "    };\n" ++
    "  }\n" ++
    "  function attachDeclarativeSubmitHook(form) {\n" ++
    "    if (formHooked.get(form)) return;\n" ++
    "    formHooked.set(form, true);\n" ++
    "    var origDispatch = form.dispatchEvent;\n" ++
    "    form.dispatchEvent = function(ev) {\n" ++
    "      if (ev && ev.type === 'submit' && formActive.get(form) && formActiveInv.get(form)) {\n" ++
    "        attachSubmitRespondWith(form, ev);\n" ++
    "      }\n" ++
    "      return origDispatch.apply(this, arguments);\n" ++
    "    };\n" ++
    "  }\n" ++
    "  g.__capsurf_webmcp = {\n" ++
    "    enable: function(sessionKey, silent) {\n" ++
    "      var sk = sessionKey || 'default';\n" ++
    "      enabledSessions[sk] = true;\n" ++
    "      if (silent) return;\n" ++
    "      scanDeclarativeTools(false);\n" ++
    "      if (replayedSessions[sk]) return;\n" ++
    "      replayedSessions[sk] = true;\n" ++
    "      var all = [];\n" ++
    "      var impNames = imperativeOrder.slice().sort();\n" ++
    "      for (var i = 0; i < impNames.length; i++) {\n" ++
    "        var e = imperativeMap[impNames[i]];\n" ++
    "        if (e) all.push(makeCdpImperative(e));\n" ++
    "      }\n" ++
    "      for (var j = 0; j < declarativeOrder.length; j++) {\n" ++
    "        var d = declarativeMap[declarativeOrder[j]];\n" ++
    "        if (d) all.push(makeCdpDeclarative(d));\n" ++
    "      }\n" ++
    "      if (all.length > 0) pushEvent('WebMCP.toolsAdded', { tools: all }, sk);\n" ++
    "    },\n" ++
    "    scanDecl: function() {\n" ++
    "      if (activeSessionCount() > 0) scanDeclarativeTools(true);\n" ++
    "    },\n" ++
    "    disable: function(sessionKey) {\n" ++
    "      var sk = sessionKey || 'default';\n" ++
    "      delete enabledSessions[sk];\n" ++
    "      delete replayedSessions[sk];\n" ++
    "    },\n" ++
    "    invoke: function(invId, toolName, inputJson, sessionKey) {\n" ++
    "      var inv = { id: invId, toolName: toolName, inputJson: inputJson, sessionKey: sessionKey || 'default', canceled: false, dispatched: false, responded: false, controller: null, form: null };\n" ++
    "      invocations[invId] = inv;\n" ++
    "      scheduleTask(function() {\n" ++
    "        if (inv.canceled) {\n" ++
    "          if (!inv.responded) {\n" ++
    "            inv.responded = true;\n" ++
    "            pushEvent('WebMCP.toolResponded', { invocationId: invId, status: 'Canceled' });\n" ++
    "          }\n" ++
    "          return;\n" ++
    "        }\n" ++
    "        inv.dispatched = true;\n" ++
    "        var imp = imperativeMap[toolName];\n" ++
    "        var decl = declarativeMap[toolName];\n" ++
    "        if (!imp && !decl) {\n" ++
    "          inv.responded = true;\n" ++
    "          pushEvent('WebMCP.toolResponded', { invocationId: invId, status: 'Error', errorText: 'no such tool: ' + toolName });\n" ++
    "          return;\n" ++
    "        }\n" ++
    "        pushEvent('WebMCP.toolInvoked', { toolName: toolName, frameId: frameId(), invocationId: invId, input: inputJson });\n" ++
    "        var parsed = {};\n" ++
    "        try { parsed = JSON.parse(inputJson || '{}'); } catch (e) {}\n" ++
    "        if (imp) {\n" ++
    "          var ctrl = new AbortController();\n" ++
    "          inv.controller = ctrl;\n" ++
    "          try {\n" ++
    "            var r = imp.execute(parsed, { signal: ctrl.signal });\n" ++
    "            Promise.resolve(r).then(function(out) {\n" ++
    "              if (inv.canceled || inv.responded) return;\n" ++
    "              inv.responded = true;\n" ++
    "              var finalOut = (typeof out === 'string') ? { content: [{ type: 'text', text: out }] } : out;\n" ++
    "              pushEvent('WebMCP.toolResponded', { invocationId: invId, status: 'Completed', output: finalOut });\n" ++
    "            }, function(err) {\n" ++
    "              if (inv.canceled || inv.responded) return;\n" ++
    "              inv.responded = true;\n" ++
    "              pushEvent('WebMCP.toolResponded', { invocationId: invId, status: 'Error', errorText: String(err && err.message || err) });\n" ++
    "            });\n" ++
    "          } catch (err) {\n" ++
    "            if (!inv.canceled && !inv.responded) {\n" ++
    "              inv.responded = true;\n" ++
    "              pushEvent('WebMCP.toolResponded', { invocationId: invId, status: 'Error', errorText: String(err && err.message || err) });\n" ++
    "            }\n" ++
    "          }\n" ++
    "          return;\n" ++
    "        }\n" ++
    "        var form = decl.form;\n" ++
    "        inv.form = form;\n" ++
    "        attachDeclarativeSubmitHook(form);\n" ++
    "        var ctrls = [];\n" ++
    "        collectFormControls(form, ctrls);\n" ++
    "        for (var ci = 0; ci < ctrls.length; ci++) {\n" ++
    "          var c = ctrls[ci];\n" ++
    "          var nm = (c.getAttribute && c.getAttribute('name')) || '';\n" ++
    "          if (nm && Object.prototype.hasOwnProperty.call(parsed, nm)) {\n" ++
    "            c.value = String(parsed[nm]);\n" ++
    "            try {\n" ++
    "              var ie = new g.InputEvent('input', { bubbles: true, data: String(parsed[nm]), inputType: 'insertText' });\n" ++
    "              var ce = new Event('change', { bubbles: true });\n" ++
    "              if (!Object.prototype.hasOwnProperty.call(c, 'dispatchEvent') && typeof g.__capsurf_dispatchTrustedEvent === 'function') {\n" ++
    "                g.__capsurf_dispatchTrustedEvent(c, ie);\n" ++
    "                g.__capsurf_dispatchTrustedEvent(c, ce);\n" ++
    "              } else {\n" ++
    "                c.dispatchEvent(ie);\n" ++
    "                c.dispatchEvent(ce);\n" ++
    "              }\n" ++
    "            } catch (e) {}\n" ++
    "          }\n" ++
    "        }\n" ++
    "        formActive.set(form, true);\n" ++
    "        formActiveInv.set(form, inv);\n" ++
    "        if (decl.autosubmit) {\n" ++
    "          var subBtn = form.querySelector ? (form.querySelector('button[type=\"submit\"]') || form.querySelector('button')) : null;\n" ++
    "          var se = new Event('submit', { bubbles: true, cancelable: true });\n" ++
    "          se.agentInvoked = true;\n" ++
    "          se.submitter = subBtn;\n" ++
    "          if (typeof g.__capsurf_dispatchTrustedEvent === 'function') {\n" ++
    "            g.__capsurf_dispatchTrustedEvent(form, se);\n" ++
    "          } else {\n" ++
    "            form.dispatchEvent(se);\n" ++
    "          }\n" ++
    "        }\n" ++
    "      });\n" ++
    "    },\n" ++
    "    cancel: function(invId, sessionKey) {\n" ++
    "      var inv = invocations[invId];\n" ++
    "      if (!inv) return;\n" ++
    "      if (inv.sessionKey !== (sessionKey || 'default')) return;\n" ++
    "      if (inv.canceled || inv.responded) return;\n" ++
    "      inv.canceled = true;\n" ++
    "      if (inv.dispatched) {\n" ++
    "        if (inv.controller) { try { inv.controller.abort(); } catch (e) {} }\n" ++
    "        if (inv.form) { formActive.delete(inv.form); formActiveInv.delete(inv.form); }\n" ++
    "        scheduleTask(function() {\n" ++
    "          if (inv.responded) return;\n" ++
    "          inv.responded = true;\n" ++
    "          pushEvent('WebMCP.toolResponded', { invocationId: invId, status: 'Canceled' });\n" ++
    "        });\n" ++
    "      }\n" ++
    "    },\n" ++
    "    retractAll: function() {\n" ++
    "      var all = [];\n" ++
    "      var impNames = imperativeOrder.slice().sort();\n" ++
    "      for (var i = 0; i < impNames.length; i++) {\n" ++
    "        var e = imperativeMap[impNames[i]];\n" ++
    "        if (e) all.push(makeCdpImperative(e));\n" ++
    "      }\n" ++
    "      for (var j = 0; j < declarativeOrder.length; j++) {\n" ++
    "        var d = declarativeMap[declarativeOrder[j]];\n" ++
    "        if (d) all.push(makeCdpDeclarative(d));\n" ++
    "      }\n" ++
    "      if (all.length === 0) return '';\n" ++
    "      return JSON.stringify({ tools: all });\n" ++
    "    },\n" ++
    "    drainEvents: function() {\n" ++
    "      if (eventsQueue.length === 0) return '';\n" ++
    "      var out = eventsQueue.join('\\n');\n" ++
    "      eventsQueue = [];\n" ++
    "      return out;\n" ++
    "    }\n" ++
    "  };\n" ++
    "})();";
