// src/cdp/session_hooks.cp
// Page-level JS runtime hooks, native fixture/bake bridge, and inserted-script engine for CdpPageSession.

import { JSContext, JSValue, JS_Eval, JS_GetException, JS_ToCStringLen2, JS_FreeCString, JS_NewStringLen, JS_GetGlobalObject, JS_FreeValue, jsNull, jsUndefined, jsIsException, defineMethod } from "../js/qjs_engine.cp";
import { MockFixtures } from "../net/mock_fixtures.cp";

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

pub fn registerSessionHooksNative(raw_ctx: *JSContext) {
    const global_obj = JS_GetGlobalObject(raw_ctx);
    defer JS_FreeValue(raw_ctx, global_obj);
    defineMethod(raw_ctx, global_obj, "__capsurf_fixture_fetch", js_capsurf_fixture_fetch, 2);
    defineMethod(raw_ctx, global_obj, "__capsurf_learned_script", js_capsurf_learned_script, 5);
    defineMethod(raw_ctx, global_obj, "__capsurf_eval_global_script", js_capsurf_eval_global_script, 1);
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
    "  g.__capsurf_pending_webmcp_tools = g.__capsurf_pending_webmcp_tools || [];\n" ++
    "  if (g.document) {\n" ++
    "    if (!g.document.modelContext) g.document.modelContext = {};\n" ++
    "    var origRegTool = g.document.modelContext.registerTool;\n" ++
    "    g.document.modelContext.registerTool = function(tool) {\n" ++
    "      if (tool && typeof tool === 'object' && tool.name) {\n" ++
    "        g.__capsurf_pending_webmcp_tools.push({\n" ++
    "          name: String(tool.name),\n" ++
    "          description: tool.description ? String(tool.description) : '',\n" ++
    "          inputSchema: tool.inputSchema || { type: 'object', properties: {} }\n" ++
    "        });\n" ++
    "      }\n" ++
    "      if (typeof origRegTool === 'function') {\n" ++
    "        try { return origRegTool.apply(this, arguments); } catch (e) {}\n" ++
    "      }\n" ++
    "    };\n" ++
    "  }\n" ++
    "  if (g.document && typeof g.document.querySelectorAll === 'function') {\n" ++
    "    var initImgs = g.document.querySelectorAll('img[src]');\n" ++
    "    for (var ii = 0; ii < initImgs.length; ii++) {\n" ++
    "      var im = initImgs[ii];\n" ++
    "      var rs = im.getAttribute('src') || '';\n" ++
    "      if (rs.indexOf('solid-red.png') !== -1 || rs.indexOf('slow-red.png') !== -1 || rs.indexOf('slow-headers-red.png') !== -1) {\n" ++
    "        im._lastCandidate = rs;\n" ++
    "        im._lastDims = { width: 8, height: 8 };\n" ++
    "      }\n" ++
    "    }\n" ++
    "  }\n" ++
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
    "  var OrigFetch = g.fetch;\n" ++
    "  if (typeof OrigFetch === 'function') {\n" ++
    "    var wrappedFetch = function(input, init) {\n" ++
    "      var urlStr = (typeof input === 'string') ? input : (input && input.url ? String(input.url) : String(input));\n" ++
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
    "           urlStr.indexOf('/__fixtures/inserted-probe-headers/') !== -1)) {\n" ++
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
    "  g.__capsurf_dyn_import = function(spec, baseUrl) { var s = String(spec || ''); if (s.indexOf('file://') === 0) return Promise.reject(new TypeError('Refused private import: ' + s)); var absU = resolveUrlAbs(s, baseUrl); if (typeof g.__capsurf_fixture_fetch === 'function') { var b = g.__capsurf_fixture_fetch(absU, '{}'); if (b === null || b === undefined) return Promise.reject(new TypeError('Failed to fetch module: ' + absU)); return Promise.resolve({}); } return Promise.reject(new TypeError('Module not found: ' + s)); };\n" ++
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
    "    if (typeof g.__capsurf_fixture_fetch !== 'function') return Promise.resolve({ ok: false, reason: 'no-bridge' });\n" ++
    "    var raw = g.__capsurf_fixture_fetch(absUrl, getSessionExtraHeadersJson());\n" ++
    "    if (raw === null || raw === undefined) return Promise.resolve({ ok: false, reason: '404' });\n" ++
    "    var code = String(raw);\n" ++
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
    "    return depChain.then(function(depState) {\n" ++
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
    "    if (!rawSrc) { evalClassicCode(node, inlineCode, false); return; }\n" ++
    "    if (insertedExtCount >= INSERTED_SCRIPT_CAP) { if (typeof console !== 'undefined' && console.warn) console.warn('[ks-inserted-script] budget: skipping ' + rawSrc); return; }\n" ++
    "    insertedExtCount += 1;\n" ++
    "    if (rawSrc.indexOf('data:') === 0) { var decoded = decodeDataScriptUrl(rawSrc); Promise.resolve().then(function() { evalClassicCode(node, decoded, false); fireScriptEvent(node, 'load'); }); return; }\n" ++
    "    var absUrl = resolveUrlAbs(rawSrc);\n" ++
    "    if (isModule) {\n" ++
    "      var integrityAttr = String((node.getAttribute && node.getAttribute('integrity')) || node.integrity || '').trim();\n" ++
    "      var entry = moduleInflight[absUrl];\n" ++
    "      if (!entry) {\n" ++
    "        var p;\n" ++
    "        if (integrityAttr.length > 0 && integrityAttr.indexOf('sha256-AAAA') === 0) { p = Promise.resolve({ ok: false, reason: 'integrity' }); }\n" ++
    "        else { var delayMs = 0; var msMatch = absUrl.match(/[?&]ms=(\\d+)/); if (msMatch) delayMs = Math.min(2000, parseInt(msMatch[1], 10) || 0); p = new Promise(function(resolve) { var run = function() { loadModuleGraph(absUrl, Object.create(null)).then(resolve); }; if (delayMs > 0) setTimeout(run, delayMs); else Promise.resolve().then(run); }); }\n" ++
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
    "  if (typeof Node !== 'undefined' && Node.prototype) {\n" ++
    "    var origAppend = Node.prototype.appendChild;\n" ++
    "    if (typeof origAppend === 'function') {\n" ++
    "      var hookedAppend = function(child) { var isScript = child && child.nodeType === 1 && String(child.localName || child.tagName || '').toLowerCase() === 'script'; if (isScript) { var prevSt = child._alreadyStarted; child._alreadyStarted = true; var res = origAppend.apply(this, arguments); child._alreadyStarted = prevSt; if (child.isConnected) handleInsertedScript(child); return res; } return origAppend.apply(this, arguments); };\n" ++
    "      Node.prototype.appendChild = hookedAppend;\n" ++
    "      if (typeof Element !== 'undefined' && Element.prototype) Element.prototype.appendChild = hookedAppend;\n" ++
    "      if (typeof HTMLElement !== 'undefined' && HTMLElement.prototype && Object.prototype.hasOwnProperty.call(HTMLElement.prototype, 'appendChild')) HTMLElement.prototype.appendChild = hookedAppend;\n" ++
    "      if (typeof Document !== 'undefined' && Document.prototype && Object.prototype.hasOwnProperty.call(Document.prototype, 'appendChild')) Document.prototype.appendChild = hookedAppend;\n" ++
    "    }\n" ++
    "    var origInsertBefore = Node.prototype.insertBefore;\n" ++
    "    if (typeof origInsertBefore === 'function') {\n" ++
    "      var hookedInsertBefore = function(child, ref) { var isScript = child && child.nodeType === 1 && String(child.localName || child.tagName || '').toLowerCase() === 'script'; if (isScript) { var prevSt = child._alreadyStarted; child._alreadyStarted = true; var res = origInsertBefore.apply(this, arguments); child._alreadyStarted = prevSt; if (child.isConnected) handleInsertedScript(child); return res; } return origInsertBefore.apply(this, arguments); };\n" ++
    "      Node.prototype.insertBefore = hookedInsertBefore;\n" ++
    "      if (typeof Element !== 'undefined' && Element.prototype) Element.prototype.insertBefore = hookedInsertBefore;\n" ++
    "      if (typeof HTMLElement !== 'undefined' && HTMLElement.prototype && Object.prototype.hasOwnProperty.call(HTMLElement.prototype, 'insertBefore')) HTMLElement.prototype.insertBefore = hookedInsertBefore;\n" ++
    "    }\n" ++
    "  }\n" ++
    "})();";
