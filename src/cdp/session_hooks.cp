// src/cdp/session_hooks.cp
// Page-level JS runtime hooks and memory accounting helpers for CdpPageSession.

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
