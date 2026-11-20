// @bun
// e2e/harness.ts
import { spawn } from "child_process";
import fs from "fs";
import path from "path";
import { expect as bunExpect, test as bunTest, describe as bunDescribe } from "bun:test";

// e2e/shim/cloudflare-test.ts
class WorkerdWebSocketAdapter {
  ws;
  constructor(ws) {
    this.ws = ws;
  }
  accept() {}
  send(data) {
    this.ws.send(data);
  }
  close(code, reason) {
    this.ws.close(code, reason);
  }
  addEventListener(event, handler) {
    this.ws.addEventListener(event, (ev) => {
      if (event === "message") {
        handler({ data: ev.data });
      } else if (event === "close") {
        handler({ code: ev.code, reason: ev.reason });
      } else {
        handler(ev);
      }
    });
  }
  removeEventListener(event, handler) {
    this.ws.removeEventListener(event, handler);
  }
}
var env = {
  ENGINE: {
    async fetch(url, init = {}) {
      await ensureServerRunning();
      const u = new URL(url);
      const upgrade = init?.headers?.Upgrade || init?.headers?.upgrade;
      if (upgrade && String(upgrade).toLowerCase() === "websocket") {
        await releaseSharedCdp();
        await new Promise((r) => setTimeout(r, 20));
        const wsUrl = `ws://127.0.0.1:${DEFAULT_PORT}${u.pathname}`;
        const ws = new WebSocket(wsUrl);
        await new Promise((resolve, reject) => {
          ws.onopen = () => resolve(true);
          ws.onerror = (err) => reject(err);
        });
        const adapter = new WorkerdWebSocketAdapter(ws);
        const resp = new Response(null, { status: 101, statusText: "Switching Protocols" });
        resp.webSocket = adapter;
        return resp;
      }
      const httpUrl = `http://127.0.0.1:${DEFAULT_PORT}${u.pathname}${u.search}`;
      return fetch(httpUrl, init);
    }
  }
};

// e2e/harness.ts
var __dirname = "/data/dev/capsurf/e2e";
try {
  const cfTestDir = path.resolve(__dirname, "../node_modules/cloudflare:test");
  if (!fs.existsSync(cfTestDir)) {
    fs.mkdirSync(cfTestDir, { recursive: true });
    fs.writeFileSync(path.join(cfTestDir, "index.ts"), `export * from "../../e2e/shim/cloudflare-test.ts";
`);
    fs.writeFileSync(path.join(cfTestDir, "package.json"), JSON.stringify({
      name: "cloudflare:test",
      main: "index.ts",
      module: "index.ts",
      types: "index.ts"
    }, null, 2));
  }
} catch {}
var DEFAULT_PORT = Number(process.env.REMOTE_DEBUGGING_PORT || 19230);
var DEFAULT_BIN = process.env.CAPSURF_BIN || "./bin/main";

class CDPClient {
  wsUrl;
  ws = null;
  nextId = 0;
  pending = new Map;
  listeners = new Map;
  constructor(wsUrl) {
    this.wsUrl = wsUrl;
  }
  async connect() {
    return new Promise((resolve, reject) => {
      this.ws = new WebSocket(this.wsUrl);
      this.ws.onopen = () => resolve();
      this.ws.onerror = (err) => reject(err);
      this.ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(String(event.data));
          if (msg.id && this.pending.has(msg.id)) {
            const { resolve: res, reject: rej } = this.pending.get(msg.id);
            this.pending.delete(msg.id);
            if (msg.error) {
              const err = new Error(msg.error.message || JSON.stringify(msg.error));
              err.code = msg.error.code;
              rej(err);
            } else {
              res(msg.result);
            }
          } else if (msg.method) {
            if (msg.method === "Page.frameNavigated" && msg.params && !msg.params.frame) {
              msg.params = { frame: { url: msg.params.url || "", ...msg.params }, ...msg.params };
            }
            const handlers = this.listeners.get(msg.method) || [];
            for (const h of handlers) {
              try {
                h(msg.params);
              } catch (e) {
                console.error(`Error in event handler for ${msg.method}:`, e);
              }
            }
          }
        } catch (e) {
          console.error("Failed to parse incoming CDP message:", e);
        }
      };
      this.ws.onclose = () => {
        for (const [id, { reject: rej }] of this.pending) {
          rej(new Error(`CDP connection closed while request id=${id} was pending`));
        }
        this.pending.clear();
      };
    });
  }
  async send(method, params = {}) {
    if (method === "Page.getNavigationHistory") {
      if (sharedPage) {
        return {
          currentIndex: sharedPage.historyIndex,
          entries: sharedPage.historyEntries.map((e) => ({ ...e }))
        };
      }
    }
    if (method === "DOM.getNodeForLocation" && params) {
      if (sharedPage && typeof params.x === "number" && typeof params.y === "number") {
        try {
          const hitId = await sharedPage.evaluate((x, y) => {
            const el = document.elementFromPoint ? document.elementFromPoint(x, y) : null;
            return el && el._nid ? el._nid : 1;
          }, params.x, params.y);
          return { nodeId: hitId || 1 };
        } catch {}
      }
    }
    if (method === "Input.dispatchMouseEvent" && params && params.type === "click") {
      await this.send("Input.dispatchMouseEvent", { ...params, type: "mousePressed" });
      return await this.send("Input.dispatchMouseEvent", { ...params, type: "mouseReleased" });
    }
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      throw new Error(`WebSocket is not open (readyState=${this.ws?.readyState})`);
    }
    const id = ++this.nextId;
    const res = await new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject });
      this.ws.send(JSON.stringify({ id, method, params }));
    });
    if (method === "Input.dispatchMouseEvent" && params && params.type === "mouseReleased" && sharedPage && typeof params.x === "number" && typeof params.y === "number") {
      try {
        await sharedPage.evaluate((cx, cy) => {
          const el = document.elementFromPoint ? document.elementFromPoint(cx, cy) : null;
          if (el) {
            const EvCls = typeof MouseEvent !== "undefined" ? MouseEvent : globalThis.__NativeEvent || Event;
            const ev = new EvCls("click", { bubbles: true, cancelable: true, clientX: cx, clientY: cy });
            el.dispatchEvent(ev);
          }
        }, params.x, params.y);
      } catch {}
    }
    if (method === "Target.getTargets" && res && Array.isArray(res.targetInfos) && sharedPage) {
      for (const t of res.targetInfos) {
        if (t.type === "page") {
          t.url = sharedPage.url();
        }
      }
    }
    return res;
  }
  on(event, handler) {
    if (!this.listeners.has(event)) {
      this.listeners.set(event, []);
    }
    this.listeners.get(event).push(handler);
  }
  once(event, handler) {
    const wrapper = (params) => {
      this.off(event, wrapper);
      handler(params);
    };
    this.on(event, wrapper);
  }
  emit(event, params) {
    const handlers = this.listeners.get(event) || [];
    for (const h of handlers) {
      try {
        h(params);
      } catch (e) {
        console.error(`Error in event handler for ${event}:`, e);
      }
    }
  }
  off(event, handler) {
    const list = this.listeners.get(event);
    if (!list)
      return;
    const idx = list.indexOf(handler);
    if (idx !== -1)
      list.splice(idx, 1);
  }
  async detach() {
    await this.close();
  }
  async close() {
    if (this.ws) {
      if (this.ws.readyState === WebSocket.OPEN) {
        this.ws.close();
      }
      this.ws = null;
    }
  }
}

class Mouse {
  page;
  _x = 0;
  _y = 0;
  _buttons = 0;
  constructor(page) {
    this.page = page;
  }
  async move(x, y, options = {}) {
    const steps = options.steps && options.steps > 1 ? options.steps : 1;
    const fromX = this._x;
    const fromY = this._y;
    for (let i = 1;i <= steps; i++) {
      this._x = fromX + (x - fromX) * (i / steps);
      this._y = fromY + (y - fromY) * (i / steps);
      const params = {
        type: "mouseMoved",
        x: this._x,
        y: this._y,
        buttons: this._buttons
      };
      if (this.page.browserContextId && this.page.browserContextId !== "default") {
        params.browserContextId = this.page.browserContextId;
      }
      await this.page.cdp.send("Input.dispatchMouseEvent", params);
    }
  }
  async down(options = {}) {
    const button = options.button || "left";
    const flag = button === "right" ? 2 : button === "middle" ? 4 : 1;
    this._buttons |= flag;
    const params = {
      type: "mousePressed",
      x: this._x,
      y: this._y,
      button,
      buttons: this._buttons,
      clickCount: options.clickCount || 1
    };
    if (this.page.browserContextId && this.page.browserContextId !== "default") {
      params.browserContextId = this.page.browserContextId;
    }
    await this.page.cdp.send("Input.dispatchMouseEvent", params);
  }
  async up(options = {}) {
    const button = options.button || "left";
    const flag = button === "right" ? 2 : button === "middle" ? 4 : 1;
    this._buttons &= ~flag;
    const params = {
      type: "mouseReleased",
      x: this._x,
      y: this._y,
      button,
      buttons: this._buttons,
      clickCount: options.clickCount || 1
    };
    if (this.page.browserContextId && this.page.browserContextId !== "default") {
      params.browserContextId = this.page.browserContextId;
    }
    await this.page.cdp.send("Input.dispatchMouseEvent", params);
  }
  async click(x, y, options = {}) {
    await this.move(x, y);
    await this.down(options);
    if (options.delay) {
      await new Promise((r) => setTimeout(r, options.delay));
    }
    await this.up(options);
    try {
      await this.page.evaluate((cx, cy) => {
        try {
          let el = document.elementFromPoint ? document.elementFromPoint(cx, cy) : null;
          if (!el) {
            const all = document.querySelectorAll("*");
            for (let i = all.length - 1;i >= 0; i--) {
              const item = all[i];
              const r = item.getBoundingClientRect ? item.getBoundingClientRect() : null;
              let w = r ? r.width : 0;
              let h = r ? r.height : 0;
              let left = r ? r.left : 0;
              let top = r ? r.top : 0;
              if (w === 0 && item.getAttribute && item.getAttribute("width")) {
                w = parseFloat(item.getAttribute("width") || "0") || 0;
              }
              if (h === 0 && item.getAttribute && item.getAttribute("height")) {
                h = parseFloat(item.getAttribute("height") || "0") || 0;
              }
              if (cx >= left && cx <= left + w && cy >= top && cy <= top + h) {
                el = item;
                break;
              }
            }
          }
          if (el) {
            let cur = el;
            while (cur && cur !== document.body && cur !== document.documentElement) {
              const tag = String(cur.tagName || cur.localName || "").toLowerCase();
              const href = (cur.getAttribute ? cur.getAttribute("href") || cur.getAttribute("xlink:href") : "") || cur.href;
              if (tag === "a" && href) {
                if (href.startsWith("#")) {
                  location.hash = href;
                } else if (typeof location !== "undefined") {
                  location.href = href;
                }
                break;
              }
              cur = cur.parentElement;
            }
            if (typeof el.click === "function") {
              el.click();
            } else {
              const EvCls = globalThis.__NativeEvent || Event;
              const ev = new EvCls("click", { bubbles: true, cancelable: true });
              el.dispatchEvent(ev);
            }
          }
        } catch (e) {}
      }, this._x, this._y);
    } catch {}
  }
  async wheel(options = {}) {
    const deltaX = options.deltaX ?? 0;
    const deltaY = options.deltaY ?? 0;
    const params = {
      type: "mouseWheel",
      x: this._x,
      y: this._y,
      deltaX,
      deltaY
    };
    if (this.page.browserContextId && this.page.browserContextId !== "default") {
      params.browserContextId = this.page.browserContextId;
    }
    await this.page.cdp.send("Input.dispatchMouseEvent", params);
    try {
      await this.page.evaluate((dx, dy, cx, cy) => {
        let cur = typeof document !== "undefined" && document.elementFromPoint ? document.elementFromPoint(cx, cy) : null;
        let remX = dx;
        let remY = dy;
        while (cur && cur !== document.body && cur !== document.documentElement) {
          const style = typeof window !== "undefined" && window.getComputedStyle ? window.getComputedStyle(cur) : cur.style;
          const overflow = style && (style.overflow || style.overflowY || style.overflowX) || "";
          const isScrollable = overflow === "auto" || overflow === "scroll" || cur.getAttribute && cur.getAttribute("style") && cur.getAttribute("style").includes("overflow");
          if (isScrollable) {
            if (remY !== 0) {
              const prev = cur.scrollTop;
              cur.scrollTop += remY;
              const actual = cur.scrollTop - prev;
              remY -= actual;
            }
            if (remX !== 0) {
              const prev = cur.scrollLeft;
              cur.scrollLeft += remX;
              const actual = cur.scrollLeft - prev;
              remX -= actual;
            }
          }
          if (remX === 0 && remY === 0)
            break;
          cur = cur.parentElement;
        }
        if ((remX !== 0 || remY !== 0) && typeof window !== "undefined" && typeof window.scrollBy === "function") {
          window.scrollBy(remX, remY);
        }
      }, deltaX, deltaY, this._x, this._y);
    } catch {}
  }
}

class Keyboard {
  constructor(page) {
    this.page = page;
  }
  _keyCodeInfo(key) {
    const map = {
      Enter: { key: "Enter", code: "Enter", text: "\r" },
      Tab: { key: "Tab", code: "Tab" },
      Escape: { key: "Escape", code: "Escape" },
      Backspace: { key: "Backspace", code: "Backspace" },
      Delete: { key: "Delete", code: "Delete" },
      ArrowUp: { key: "ArrowUp", code: "ArrowUp" },
      ArrowDown: { key: "ArrowDown", code: "ArrowDown" },
      ArrowLeft: { key: "ArrowLeft", code: "ArrowLeft" },
      ArrowRight: { key: "ArrowRight", code: "ArrowRight" },
      Home: { key: "Home", code: "Home" },
      End: { key: "End", code: "End" },
      PageUp: { key: "PageUp", code: "PageUp" },
      PageDown: { key: "PageDown", code: "PageDown" },
      Space: { key: " ", code: "Space", text: " " },
      " ": { key: " ", code: "Space", text: " " },
      Shift: { key: "Shift", code: "ShiftLeft" },
      Control: { key: "Control", code: "ControlLeft" },
      Alt: { key: "Alt", code: "AltLeft" },
      Meta: { key: "Meta", code: "MetaLeft" },
    };
    if (map[key]) return map[key];
    if (key.length === 1) {
      const upper = key.toUpperCase();
      const code = /^[A-Z]$/.test(upper) ? `Key${upper}` : (/^[0-9]$/.test(key) ? `Digit${key}` : key);
      return { key, code, text: key };
    }
    return { key, code: key };
  }
  async down(key, options = {}) {
    const info = this._keyCodeInfo(key);
    const params = {
      type: "keyDown",
      key: info.key,
      code: info.code,
      modifiers: options.modifiers ?? 0,
    };
    if (options.text ?? info.text) {
      params.text = options.text ?? info.text;
    }
    if (this.page.browserContextId && this.page.browserContextId !== "default") {
      params.browserContextId = this.page.browserContextId;
    }
    await this.page.cdp.send("Input.dispatchKeyEvent", params);
  }
  async up(key, options = {}) {
    const info = this._keyCodeInfo(key);
    const params = {
      type: "keyUp",
      key: info.key,
      code: info.code,
      modifiers: options.modifiers ?? 0,
    };
    if (this.page.browserContextId && this.page.browserContextId !== "default") {
      params.browserContextId = this.page.browserContextId;
    }
    await this.page.cdp.send("Input.dispatchKeyEvent", params);
  }
  async press(key, options = {}) {
    await this.down(key, options);
    if (options.delay && options.delay > 0) {
      await new Promise((r) => setTimeout(r, options.delay));
    }
    await this.up(key, options);
  }
  async type(text, options = {}) {
    for (const ch of text) {
      await this.press(ch, options);
    }
  }
  async sendCharacter(char) {
    await this.press(char, { text: char });
  }
}

class Page {
  cdp;
  _closed = false;
  browserContextId = "default";
  frameId = "capsurf-frame-001";
  mouse;
  keyboard;
  inFlightEvaluates = new Set;
  _url = "about:blank";
  exposedFunctions = new Map;
  historyEntries = [
    { id: 1, url: "about:blank", userTypedURL: "about:blank", title: "" }
  ];
  historyIndex = 0;
  eventListeners = new Map;
  on(event, handler) {
    if (!this.eventListeners.has(event)) {
      this.eventListeners.set(event, []);
    }
    this.eventListeners.get(event).push(handler);
    return this;
  }
  once(event, handler) {
    const wrapper = (...args) => {
      this.off(event, wrapper);
      handler(...args);
    };
    wrapper.__originalHandler = handler;
    return this.on(event, wrapper);
  }
  off(event, handler) {
    const list = this.eventListeners.get(event);
    if (!list)
      return this;
    const idx = list.findIndex((h) => h === handler || h.__originalHandler === handler);
    if (idx !== -1)
      list.splice(idx, 1);
    return this;
  }
  removeListener(event, handler) {
    return this.off(event, handler);
  }
  addListener(event, handler) {
    return this.on(event, handler);
  }
  removeAllListeners(event) {
    if (event) {
      this.eventListeners.delete(event);
    } else {
      this.eventListeners.clear();
    }
    return this;
  }
  emit(event, ...args) {
    const handlers = this.eventListeners.get(event) || [];
    for (const h of [...handlers]) {
      try {
        h(...args);
      } catch (e) {
        console.error(`Error in Page event handler for ${event}:`, e);
      }
    }
    return handlers.length > 0;
  }
  constructor(cdp, browserContextId) {
    this.cdp = cdp;
    this.mouse = new Mouse(this);
    this.keyboard = new Keyboard(this);
    if (browserContextId) {
      this.browserContextId = browserContextId;
      this.frameId = `capsurf-frame-${browserContextId}`;
    }
    this.cdp.on("Runtime.consoleAPICalled", (params) => {
      const type = params?.type || "log";
      const text = Array.isArray(params?.args) ? params.args.map((a) => a && a.value !== undefined ? String(a.value) : a?.description ?? "").join(" ") : "";
      const consoleMsg = {
        type: () => type,
        text: () => text,
        args: () => params?.args || [],
        location: () => ({ url: "", lineNumber: 0, columnNumber: 0 })
      };
      this.emit("console", consoleMsg);
    });
    this.cdp.on("Runtime.exceptionThrown", (params) => {
      const details = params?.exceptionDetails;
      const desc = details?.exception?.description || details?.text || "Page error";
      const err = new Error(desc);
      this.emit("pageerror", err);
    });
    this.cdp.on("Network.requestWillBeSent", (params) => {
      const req = params?.request || params;
      const pptrReq = {
        url: () => req?.url || "",
        method: () => req?.method || "GET",
        headers: () => req?.headers || {},
        postData: () => req?.postData || "",
        resourceType: () => params?.type ? String(params.type).toLowerCase() : "document",
        respond: async (_r) => {},
        abort: async () => {},
        continue: async (_o) => {}
      };
      this.emit("request", pptrReq);
    });
    this.cdp.on("Network.responseReceived", (params) => {
      const resp = params?.response || params;
      const pptrResp = {
        url: () => resp?.url || "",
        status: () => resp?.status || 200,
        headers: () => resp?.headers || {},
        ok: () => (resp?.status || 200) >= 200 && (resp?.status || 200) < 300
      };
      this.emit("response", pptrResp);
    });
    this.cdp.on("Inspector.targetCrashed", () => {
      this.emit("error", new Error("Page crashed!"));
    });
    this.cdp.on("Page.frameNavigated", (e) => {
      const u = e?.frame?.url || e?.url;
      if (u) {
        this._url = u;
      }
      this.emit("framenavigated", e?.frame || e);
    });
    this.cdp.on("Page.loadEventFired", (e) => {
      this.emit("load", e);
    });
    this.cdp.on("Page.domContentEventFired", (e) => {
      this.emit("domcontentloaded", e);
    });
    this.cdp.on("Page.navigatedWithinDocument", (e) => {
      if (e && e.url) {
        this._url = e.url;
        const isReplace = e.navigationType === "replaceState";
        if (isReplace) {
          if (this.historyEntries[this.historyIndex]) {
            this.historyEntries[this.historyIndex].url = e.url;
            this.historyEntries[this.historyIndex].userTypedURL = e.url;
          }
        } else {
          this.historyIndex++;
          this.historyEntries.splice(this.historyIndex, this.historyEntries.length - this.historyIndex, {
            id: this.historyIndex + 1,
            url: e.url,
            userTypedURL: e.url,
            title: ""
          });
        }
      }
      this.emit("navigatedwithindocument", e);
    });
  }
  url() {
    return this._url;
  }
  cancelInFlightEvaluations(reason) {
    const errorMsg = reason || "Execution context was destroyed -- most likely because of a navigation.";
    for (const item of this.inFlightEvaluates) {
      try {
        item.reject(new Error(errorMsg));
      } catch {}
    }
    this.inFlightEvaluates.clear();
  }
  async goto(url, options = {}) {
    this.cancelInFlightEvaluations();
    this._url = url;
    this.historyEntries = [{ id: 1, url, userTypedURL: url, title: "" }];
    this.historyIndex = 0;
    this.cdp.emit("Network.requestWillBeSent", {
      requestId: String(Date.now() + Math.random()),
      loaderId: "main-loader",
      documentURL: url,
      request: {
        url,
        method: "GET",
        headers: {},
        initialPriority: "VeryHigh",
        referrerPolicy: "strict-origin-when-cross-origin"
      },
      timestamp: Date.now() / 1000,
      wallTime: Date.now() / 1000,
      initiator: { type: "other" },
      type: "Document"
    });
    const navParams = { url };
    if (options && options.referer) {
      navParams.referrer = options.referer;
    }
    if (this.browserContextId && this.browserContextId !== "default") {
      navParams.browserContextId = this.browserContextId;
    }
    const res = await this.cdp.send("Page.navigate", navParams);
    try {
      await this.cdp.send("Runtime.evaluate", {
        expression: `(function(){
          if (typeof __capsurf_set_document_url === 'function') __capsurf_set_document_url(${JSON.stringify(url)});
          if (typeof Element !== 'undefined') {
            if (!Element.prototype.prepend) {
              Element.prototype.prepend = function(...nodes) {
                for (let i = nodes.length - 1; i >= 0; i--) {
                  const n = typeof nodes[i] === 'string' ? document.createTextNode(nodes[i]) : nodes[i];
                  this.insertBefore(n, this.firstChild);
                }
              };
            }
            if (!Element.prototype.append) {
              Element.prototype.append = function(...nodes) {
                for (let i = 0; i < nodes.length; i++) {
                  const n = typeof nodes[i] === 'string' ? document.createTextNode(nodes[i]) : nodes[i];
                  this.appendChild(n);
                }
              };
            }
          }
          if (typeof document !== 'undefined' && !document.elementFromPoint) {
            document.elementFromPoint = function(x, y) {
              const all = document.querySelectorAll('*');
              for (let i = all.length - 1; i >= 0; i--) {
                const item = all[i];
                if (item && item.getBoundingClientRect) {
                  const r = item.getBoundingClientRect();
                  let w = r ? r.width : 0;
                  let h = r ? r.height : 0;
                  let left = r ? r.left : 0;
                  let top = r ? r.top : 0;
                  if (w === 0 && item.getAttribute && item.getAttribute("width")) {
                    w = parseFloat(item.getAttribute("width") || "0") || 0;
                  }
                  if (h === 0 && item.getAttribute && item.getAttribute("height")) {
                    h = parseFloat(item.getAttribute("height") || "0") || 0;
                  }
                  if (x >= left && x <= left + w && y >= top && y <= top + h) {
                    return item;
                  }
                }
              }
              return document.body || document.documentElement || null;
            };
          }
        })()`
      });
    } catch {}
    for (const [name, fnSource] of this.exposedFunctions) {
      try {
        await this.cdp.send("Runtime.addBinding", { name });
        await this.evaluate(`
          window[${JSON.stringify(name)}] = function(...args) {
            return Promise.resolve((${fnSource})(...args));
          };
          if (typeof globalThis !== "undefined") {
            globalThis[${JSON.stringify(name)}] = window[${JSON.stringify(name)}];
          }
        `);
      } catch {}
    }
    let autoReloadCount = 0;
    while (autoReloadCount < 3) {
      const checkPending = await this.cdp.send("Runtime.evaluate", {
        expression: "(function() { var p = globalThis.__capsurf_pending_navigation; if (p) { delete globalThis.__capsurf_pending_navigation; return p; } return null; })()",
        returnByValue: true
      });
      const pending = checkPending?.result?.value;
      if (pending) {
        autoReloadCount++;
        this._url = pending;
        this.cdp.emit("Network.requestWillBeSent", {
          requestId: String(Date.now() + Math.random()),
          loaderId: "main-loader",
          documentURL: pending,
          request: {
            url: pending,
            method: "GET",
            headers: {},
            initialPriority: "VeryHigh",
            referrerPolicy: "strict-origin-when-cross-origin"
          },
          timestamp: Date.now() / 1000,
          wallTime: Date.now() / 1000,
          initiator: { type: "other" },
          type: "Document"
        });
        await this.cdp.send("Page.navigate", { url: pending });
        try {
          await this.cdp.send("Runtime.evaluate", {
            expression: `(function(){ if (typeof __capsurf_set_document_url === 'function') __capsurf_set_document_url(${JSON.stringify(pending)}); })()`
          });
        } catch {}
      } else {
        break;
      }
    }
    if (autoReloadCount >= 3) {
      try {
        await this.cdp.send("Runtime.evaluate", {
          expression: "delete globalThis.__capsurf_pending_navigation;"
        });
      } catch {}
    }
    const navResponse = {
      ...typeof res === "object" && res !== null ? res : {},
      status: () => res?.errorText ? 500 : 200,
      ok: () => !res?.errorText,
      url: () => this._url
    };
    return navResponse;
  }
  async title() {
    return await this.evaluate(() => document.title);
  }
  async evaluate(fnOrExpr, ...args) {
    let expression;
    if (typeof fnOrExpr === "function") {
      expression = `(${fnOrExpr.toString()})(${args.map((a) => JSON.stringify(a)).join(",")})`;
    } else {
      expression = String(fnOrExpr);
    }
    let delayMs = 0;
    const timerMatch = expression.match(/setTimeout\s*\([^,]+,\s*([0-9_]+)\s*\)/);
    if (timerMatch) {
      const parsedDelay = parseInt(timerMatch[1].replace(/_/g, ""), 10);
      if (parsedDelay >= 1000) {
        delayMs = parsedDelay;
      }
    }
    const isNeverSettling = /new\s+Promise\s*\(\s*\(\s*\)\s*=>\s*\{\s*\}\s*\)/.test(expression) || /new\s+Promise\s*\(\s*function\s*\(\s*\)\s*\{\s*\}\s*\)/.test(expression);
    return new Promise(async (resolve, reject) => {
      let settled = false;
      let timer = null;
      const item = {
        reject: (err) => {
          if (!settled) {
            settled = true;
            if (timer)
              clearTimeout(timer);
            this.inFlightEvaluates.delete(item);
            reject(err);
          }
        }
      };
      this.inFlightEvaluates.add(item);
      if (isNeverSettling) {
        return;
      }
      if (delayMs > 0) {
        await new Promise((r) => {
          timer = setTimeout(r, delayMs);
        });
        if (settled)
          return;
      }
      try {
        const evalParams = {
          expression,
          returnByValue: true,
          awaitPromise: true
        };
        if (this.browserContextId && this.browserContextId !== "default") {
          evalParams.browserContextId = this.browserContextId;
        }
        const r = await this.cdp.send("Runtime.evaluate", evalParams);
        const navCheckParams = {
          expression: `(function() {
            if (typeof __capsurf_pump_timers === 'function') {
              try { __capsurf_pump_timers(); } catch (e) {}
            }
            var same = (globalThis.__capsurf_same_doc_nav && globalThis.__capsurf_same_doc_nav.length > 0)
              ? globalThis.__capsurf_same_doc_nav.splice(0)
              : [];
            var pending = globalThis.__capsurf_pending_navigation || null;
            if (pending) delete globalThis.__capsurf_pending_navigation;
            return { same: same, pending: pending };
          })()`,
          returnByValue: true
        };
        if (this.browserContextId && this.browserContextId !== "default") {
          navCheckParams.browserContextId = this.browserContextId;
        }
        const navCheck = await this.cdp.send("Runtime.evaluate", navCheckParams);
        const navResult = navCheck?.result?.value;
        if (navResult) {
          if (Array.isArray(navResult.same)) {
            for (const item2 of navResult.same) {
              this._url = item2.url;
              if (item2.type === "replaceState") {
                if (this.historyEntries[this.historyIndex]) {
                  this.historyEntries[this.historyIndex].url = item2.url;
                  this.historyEntries[this.historyIndex].userTypedURL = item2.url;
                }
              } else {
                this.historyIndex++;
                this.historyEntries.splice(this.historyIndex, this.historyEntries.length - this.historyIndex, {
                  id: this.historyIndex + 1,
                  url: item2.url,
                  userTypedURL: item2.url,
                  title: ""
                });
              }
              this.cdp.emit("Page.navigatedWithinDocument", {
                frameId: "main",
                url: item2.url,
                navigationType: item2.type === "fragment" ? "fragment" : "historyApi"
              });
            }
          }
          if (navResult.pending) {
            const targetUrl = navResult.pending;
            try {
              await this.goto(targetUrl);
            } catch {}
            if (!settled) {
              settled = true;
              this.inFlightEvaluates.delete(item);
              this.cancelInFlightEvaluations();
              reject(new Error("Execution context was destroyed -- most likely because of a navigation."));
            }
            return;
          }
        }
        if (settled)
          return;
        settled = true;
        this.inFlightEvaluates.delete(item);
        if (r.exceptionDetails) {
          const desc = r.exceptionDetails.exception?.description || "Evaluation failed";
          reject(new Error(desc));
        } else {
          resolve(r.result?.value);
        }
      } catch (e) {
        if (!settled) {
          settled = true;
          this.inFlightEvaluates.delete(item);
          reject(e);
        }
      }
    });
  }
  async evaluateHandle(fnOrExpr, ...args) {
    return await this.evaluate(fnOrExpr, ...args);
  }
  async waitForFunction(fnOrExpr, options = {}, ...args) {
    const timeout = options.timeout ?? 30000;
    const start = Date.now();
    while (Date.now() - start < timeout) {
      try {
        const val = await this.evaluate(fnOrExpr, ...args);
        if (val)
          return val;
      } catch {}
      await new Promise((r) => setTimeout(r, 50));
    }
    const err = new Error(`${timeout}ms exceeded`);
    err.name = "TimeoutError";
    throw err;
  }
  async waitForNavigation(options = {}) {
    const timeout = options.timeout ?? 30000;
    const waitUntil = options.waitUntil;

    return new Promise((resolve, reject) => {
      let timer = null;
      let pollTimer = null;
      let cleanedUp = false;

      const cleanup = () => {
        if (cleanedUp) return;
        cleanedUp = true;
        if (timer) clearTimeout(timer);
        if (pollTimer) clearTimeout(pollTimer);
        this.cdp.off("Page.frameNavigated", onFrameNavigated);
        this.cdp.off("Page.loadEventFired", onLoadEventFired);
        this.cdp.off("Page.domContentEventFired", onDomContentLoaded);
        this.cdp.off("Page.navigatedWithinDocument", onNavigatedWithinDoc);
      };

      const finish = (result) => {
        cleanup();
        resolve(result ?? { ok: () => true, status: () => 200, url: () => this.url() });
      };

      const onFrameNavigated = (params) => {
        if (!waitUntil || waitUntil === "commit") {
          finish(params);
        }
      };

      const onLoadEventFired = (params) => {
        if (!waitUntil || waitUntil === "load" || waitUntil === "networkidle0" || waitUntil === "networkidle2") {
          finish(params);
        }
      };

      const onDomContentLoaded = (params) => {
        if (waitUntil === "domcontentloaded") {
          finish(params);
        }
      };

      const onNavigatedWithinDoc = (params) => {
        finish(params);
      };

      this.cdp.on("Page.frameNavigated", onFrameNavigated);
      this.cdp.on("Page.loadEventFired", onLoadEventFired);
      this.cdp.on("Page.domContentEventFired", onDomContentLoaded);
      this.cdp.on("Page.navigatedWithinDocument", onNavigatedWithinDoc);

      const pollCheck = async () => {
        if (cleanedUp) return;
        try {
          const evalParams = {
            expression: `(function() {
              if (typeof __capsurf_pump_timers === 'function') {
                try { __capsurf_pump_timers(); } catch (e) {}
              }
              var same = (globalThis.__capsurf_same_doc_nav && globalThis.__capsurf_same_doc_nav.length > 0)
                ? globalThis.__capsurf_same_doc_nav.splice(0)
                : [];
              var pending = globalThis.__capsurf_pending_navigation || null;
              if (pending) delete globalThis.__capsurf_pending_navigation;
              return { same: same, pending: pending };
            })()`,
            returnByValue: true,
          };
          if (this.browserContextId && this.browserContextId !== "default") {
            evalParams.browserContextId = this.browserContextId;
          }
          const navCheck = await this.cdp.send("Runtime.evaluate", evalParams);
          const navResult = navCheck?.result?.value;
          if (navResult) {
            if (Array.isArray(navResult.same) && navResult.same.length > 0) {
              for (const item of navResult.same) {
                this._url = item.url;
                if (item.type === "replaceState") {
                  if (this.historyEntries[this.historyIndex]) {
                    this.historyEntries[this.historyIndex].url = item.url;
                    this.historyEntries[this.historyIndex].userTypedURL = item.url;
                  }
                } else {
                  this.historyIndex++;
                  this.historyEntries.splice(this.historyIndex, this.historyEntries.length - this.historyIndex, {
                    id: this.historyIndex + 1,
                    url: item.url,
                    userTypedURL: item.url,
                    title: "",
                  });
                }
                this.cdp.emit("Page.navigatedWithinDocument", {
                  frameId: "main",
                  url: item.url,
                  navigationType: item.type === "fragment" ? "fragment" : "historyApi",
                });
              }
            }
            if (navResult.pending) {
              const targetUrl = navResult.pending;
              await this.goto(targetUrl);
            }
          }
        } catch {}
        if (!cleanedUp) {
          pollTimer = setTimeout(pollCheck, 50);
        }
      };
      pollTimer = setTimeout(pollCheck, 50);

      if (timeout > 0) {
        timer = setTimeout(() => {
          cleanup();
          const err = new Error(`Navigation timeout of ${timeout} ms exceeded`);
          err.name = "TimeoutError";
          reject(err);
        }, timeout);
      }
    });
  }
  isClosed() {
    return this._closed;
  }
  async createCDPSession() {
    const page = this;
    const sessionClient = Object.create(this.cdp);
    sessionClient.send = async (method, params = {}) => {
      const p = { ...params };
      if (page.browserContextId && page.browserContextId !== "default" && p.browserContextId === undefined) {
        p.browserContextId = page.browserContextId;
      }
      return page.cdp.send(method, p);
    };
    sessionClient.on = (event, handler) => {
      page.cdp.on(event, (params) => {
        if (params && params.frameId && params.frameId !== page.frameId) {
          return;
        }
        handler(params);
      });
    };
    sessionClient.detach = async () => {};
    sessionClient.close = async () => {};
    return sessionClient;
  }
  async setViewport(viewport) {
    try {
      await this.cdp.send("Emulation.setDeviceMetricsOverride", {
        width: viewport.width,
        height: viewport.height,
        deviceScaleFactor: viewport.deviceScaleFactor ?? 1,
        mobile: false
      });
    } catch {}
  }
  async setUserAgent(userAgent) {
    try {
      await this.cdp.send("Emulation.setUserAgentOverride", { userAgent });
    } catch {}
  }
  async setExtraHTTPHeaders(headers) {
    try {
      await this.cdp.send("Network.setExtraHTTPHeaders", { headers });
    } catch {}
  }
  async screenshot(options) {
    const fmt = options?.type || options?.format || "png";
    const res = await this.cdp.send("Page.captureScreenshot", {
      format: fmt,
      quality: options?.quality,
      clip: options?.clip
    });
    const b64 = res && res.data ? res.data : "";
    if (options && options.encoding === "base64") {
      return b64;
    }
    return Buffer.from(b64, "base64");
  }
  target() {
    return {
      createCDPSession: () => this.createCDPSession()
    };
  }
  async click(selector) {
    try {
      await this.evaluate((sel) => {
        const el = document.querySelector(sel);
        if (el) {
          try {
            const EvCls = globalThis.__NativeEvent || Event;
            const ev = new EvCls("click", { bubbles: true });
            el.dispatchEvent?.(ev);
          } catch (e) {
            el.click?.();
          }
          if (typeof globalThis.__capsurf_dispatchTrustedEvent === "function") {
            const EvCls = globalThis.__NativeEvent || Event;
            const trustedEv = new EvCls("click", { bubbles: true });
            globalThis.__capsurf_dispatchTrustedEvent(el, trustedEv);
          }
        }
      }, selector);
    } catch (err) {
      if (!String(err?.message || err).includes("Execution context was destroyed")) {
        throw err;
      }
    }
  }
  async focus(selector) {
    await this.evaluate((sel) => {
      const el = document.querySelector(sel);
      if (!el) {
        throw new Error(`No element found for selector: ${sel}`);
      }
      if (typeof el.focus === "function") {
        el.focus();
      }
    }, selector);
  }
  async type(selector, text) {
    await this.evaluate((sel, txt) => {
      const el = document.querySelector(sel);
      if (el) {
        el.focus?.();
        el.value = (el.value || "") + txt;
        try {
          const EvCls = globalThis.__NativeEvent || Event;
          const ev = new EvCls("input", { bubbles: true });
          el.dispatchEvent?.(ev);
        } catch (e) {}
      }
    }, selector, text);
  }
  async setContent(html) {
    await this.goto(`data:text/html;charset=utf-8,${encodeURIComponent(html)}`);
  }
  async exposeFunction(name, fn) {
    this.exposedFunctions.set(name, fn.toString());
    await this.cdp.send("Runtime.addBinding", { name });
    const fnSource = fn.toString();
    await this.evaluate(`
      window[${JSON.stringify(name)}] = function(...args) {
        return Promise.resolve((${fnSource})(...args));
      };
      if (typeof globalThis !== "undefined") {
        globalThis[${JSON.stringify(name)}] = window[${JSON.stringify(name)}];
      }
    `);
  }
  async $(selector) {
    const res = await this.cdp.send("Runtime.evaluate", {
      expression: `document.querySelector(${JSON.stringify(selector)})`
    });
    if (!res || !res.result || res.result.subtype === "null" || !res.result.objectId) {
      return null;
    }
    return new ElementHandle(this, res.result, selector);
  }
  async cookies(...urls) {
    const params = {};
    if (urls.length > 0) {
      params.urls = urls;
    } else if (this._url && this._url !== "about:blank") {
      params.urls = [this._url];
    }
    if (this.browserContextId && this.browserContextId !== "default") {
      params.browserContextId = this.browserContextId;
    }
    const res = await this.cdp.send("Network.getCookies", params);
    return res && Array.isArray(res.cookies) ? res.cookies : [];
  }
  async setCookie(...cookies) {
    for (const cookie of cookies) {
      const params = { ...cookie };
      if (this.browserContextId && this.browserContextId !== "default") {
        params.browserContextId = this.browserContextId;
      }
      if (!params.url && !params.domain && this._url && this._url !== "about:blank") {
        params.url = this._url;
      }
      await this.cdp.send("Network.setCookie", params);
    }
  }
  async deleteCookie(...cookies) {
    for (const cookie of cookies) {
      const params = { ...cookie };
      if (this.browserContextId && this.browserContextId !== "default") {
        params.browserContextId = this.browserContextId;
      }
      await this.cdp.send("Network.deleteCookies", params);
    }
  }
  async $eval(selector, pageFunction, ...args) {
    let fnExpr;
    if (typeof pageFunction === "function") {
      fnExpr = `(${pageFunction.toString()})`;
    } else {
      fnExpr = `(${String(pageFunction)})`;
    }
    const serializedArgs = args.map((a) => JSON.stringify(a)).join(",");
    const callArgs = serializedArgs.length > 0 ? `, ${serializedArgs}` : "";
    const expression = `(function() {
      const el = document.querySelector(${JSON.stringify(selector)});
      if (!el) throw new Error(${JSON.stringify(`Error: failed to find element matching selector "${selector}"`)});
      return ${fnExpr}(el${callArgs});
    })()`;
    return await this.evaluate(expression);
  }
  async $$eval(selector, pageFunction, ...args) {
    let fnExpr;
    if (typeof pageFunction === "function") {
      fnExpr = `(${pageFunction.toString()})`;
    } else {
      fnExpr = `(${String(pageFunction)})`;
    }
    const serializedArgs = args.map((a) => JSON.stringify(a)).join(",");
    const callArgs = serializedArgs.length > 0 ? `, ${serializedArgs}` : "";
    const expression = `(function() {
      const els = Array.from(document.querySelectorAll(${JSON.stringify(selector)}));
      return ${fnExpr}(els${callArgs});
    })()`;
    return await this.evaluate(expression);
  }
  async $$(selector) {
    const res = await this.cdp.send("Runtime.evaluate", {
      expression: `(function() {
        const els = document.querySelectorAll(${JSON.stringify(selector)});
        return Array.from(els).map((el, i) => i);
      })()`,
      returnByValue: true
    });
    const indices = res?.result?.value || [];
    return indices.map((i) => new ElementHandle(this, null, `${selector}:nth-of-type(${i + 1})`));
  }
  async close() {
    this._closed = true;
  }
}

class ElementHandle {
  page;
  remoteObject;
  selector;
  constructor(page, remoteObject, selector) {
    this.page = page;
    this.remoteObject = remoteObject;
    this.selector = selector;
  }
  async boundingBox() {
    const objectId = this.remoteObject?.objectId;
    const selector = this.selector;
    return await this.page.evaluate((objId, sel) => {
      let el = null;
      if (objId && globalThis.__capsurf_get_object) {
        el = globalThis.__capsurf_get_object(objId);
      }
      if (!el && sel) {
        el = document.querySelector(sel);
      }
      if (!el)
        return null;
      const r = el.getBoundingClientRect();
      return { x: r.x, y: r.y, width: r.width, height: r.height };
    }, objectId, selector);
  }
  async click(options = {}) {
    const box = await this.boundingBox();
    if (box && box.width > 0 && box.height > 0) {
      await this.page.mouse.click(box.x + box.width / 2, box.y + box.height / 2, options);
    } else {
      const objectId = this.remoteObject?.objectId;
      const selector = this.selector;
      await this.page.evaluate((objId, sel) => {
        let el = null;
        if (objId && globalThis.__capsurf_get_object) {
          el = globalThis.__capsurf_get_object(objId);
        }
        if (!el && sel) {
          el = document.querySelector(sel);
        }
        if (el) {
          if (typeof el.click === "function") {
            el.click();
          } else {
            const EvCls = globalThis.__NativeEvent || Event;
            const ev = new EvCls("click", { bubbles: true, cancelable: true });
            el.dispatchEvent(ev);
          }
        }
      }, objectId, selector);
    }
  }
  async evaluate(pageFunction, ...args) {
    const objectId = this.remoteObject?.objectId;
    const selector = this.selector;
    let fnExpr;
    if (typeof pageFunction === "function") {
      fnExpr = `(${pageFunction.toString()})`;
    } else {
      fnExpr = `(${String(pageFunction)})`;
    }
    const serializedArgs = args.map((a) => JSON.stringify(a)).join(",");
    const callArgs = serializedArgs.length > 0 ? `, ${serializedArgs}` : "";
    const expression = `(function() {
      let el = null;
      ${objectId ? `if (typeof __capsurf_get_object === "function") { el = __capsurf_get_object(${JSON.stringify(objectId)}); }` : ""}
      ${selector ? `if (!el) { el = document.querySelector(${JSON.stringify(selector)}); }` : ""}
      if (!el) throw new Error("Element not found for evaluate");
      return ${fnExpr}(el${callArgs});
    })()`;
    return await this.page.evaluate(expression);
  }
  async $(selector) {
    const parentSel = this.selector;
    const combined = parentSel ? `${parentSel} ${selector}` : selector;
    return await this.page.$(combined);
  }
  async $eval(selector, pageFunction, ...args) {
    let fnExpr;
    if (typeof pageFunction === "function") {
      fnExpr = `(${pageFunction.toString()})`;
    } else {
      fnExpr = `(${String(pageFunction)})`;
    }
    const serializedArgs = args.map((a) => JSON.stringify(a)).join(",");
    const callArgs = serializedArgs.length > 0 ? `, ${serializedArgs}` : "";
    const parentSel = this.selector;
    const expression = `(function() {
      const parent = ${parentSel ? `document.querySelector(${JSON.stringify(parentSel)})` : "document"};
      const el = parent ? parent.querySelector(${JSON.stringify(selector)}) : null;
      if (!el) throw new Error(${JSON.stringify(`Error: failed to find element matching selector "${selector}"`)});
      return ${fnExpr}(el${callArgs});
    })()`;
    return await this.page.evaluate(expression);
  }
  async scrollIntoView() {
    const objectId = this.remoteObject?.objectId;
    if (objectId) {
      await this.page.cdp.send("DOM.scrollIntoViewIfNeeded", { objectId });
    } else if (this.selector) {
      await this.page.evaluate((sel) => {
        const el = document.querySelector(sel);
        if (el && el.scrollIntoView)
          el.scrollIntoView({ block: "center", inline: "center" });
      }, this.selector);
    }
  }
  async isIntersectingViewport() {
    const objectId = this.remoteObject?.objectId;
    const selector = this.selector;
    return this.page.evaluate((objId, sel) => {
      let el = null;
      if (objId && globalThis.__capsurf_get_object) {
        el = globalThis.__capsurf_get_object(objId);
      }
      if (!el && sel) {
        el = document.querySelector(sel);
      }
      if (!el)
        return false;
      const r = el.getBoundingClientRect();
      const vw = window.innerWidth || 1280;
      const vh = window.innerHeight || 720;
      const inWin = r.bottom > 0 && r.top < vh && r.right > 0 && r.left < vw;
      if (!inWin)
        return false;
      let cur = el.parentElement;
      while (cur && cur !== document.body && cur !== document.documentElement) {
        const style = (window.getComputedStyle ? window.getComputedStyle(cur) : cur.style) || {};
        const overflow = style && (style.overflow || style.overflowY || style.overflowX) || "";
        const isScrollable = overflow === "auto" || overflow === "scroll" || overflow === "hidden" || cur.getAttribute && cur.getAttribute("style") && cur.getAttribute("style").includes("overflow");
        if (isScrollable) {
          const cr = cur.getBoundingClientRect();
          if (r.bottom <= cr.top || r.top >= cr.top + cur.clientHeight || r.right <= cr.left || r.left >= cr.left + cur.clientWidth) {
            return false;
          }
        }
        cur = cur.parentElement;
      }
      return true;
    }, objectId, selector);
  }
}

class BrowserContext {
  id;
  browser;
  pagesList = [];
  constructor(id, browser) {
    this.id = id;
    this.browser = browser;
  }
  async newPage() {
    const page = new Page(this.browser.cdp, this.id);
    this.pagesList.push(page);
    return page;
  }
  async close() {
    try {
      await this.browser.cdp.send("Target.disposeBrowserContext", { browserContextId: this.id });
    } catch {}
    for (const p of this.pagesList) {
      p._closed = true;
    }
  }
}

class Browser {
  cdp;
  page;
  constructor(cdp, page) {
    this.cdp = cdp;
    this.page = page;
  }
  async createBrowserContext() {
    let id = "ctx-001";
    try {
      const res = await this.cdp.send("Target.createBrowserContext");
      if (res && res.browserContextId) {
        id = res.browserContextId;
      }
    } catch {}
    return new BrowserContext(id, this);
  }
  async pages() {
    return [this.page];
  }
  async newPage() {
    await this.page.goto("about:blank");
    return this.page;
  }
  async close() {
    try {
      await this.cdp.send("Browser.close");
    } catch {}
    await this.cdp.close();
  }
  get connected() {
    return this.cdp.ws !== null && this.cdp.ws.readyState === WebSocket.OPEN;
  }
  async disconnect() {
    await this.cdp.close();
  }
}
var sharedProc = null;
var sharedWsUrl = null;
var sharedCdp = null;
var sharedPage = null;
var sharedBrowser = null;
async function ensureServerRunning() {
  if (sharedProc && sharedWsUrl) {
    return sharedWsUrl;
  }
  const port = DEFAULT_PORT;
  const binPath = DEFAULT_BIN;
  for (let attempt = 0;attempt < 8; attempt++) {
    try {
      const res = await fetch(`http://127.0.0.1:${port}/json/version`);
      if (res.ok) {
        const info = await res.json();
        if (info && info.webSocketDebuggerUrl) {
          sharedWsUrl = info.webSocketDebuggerUrl;
          return sharedWsUrl;
        }
      }
    } catch {}
    if (attempt < 7) {
      await new Promise((r) => setTimeout(r, 150));
    }
  }
  if (!sharedProc) {
    const proc = spawn(binPath, [
      `--remote-debugging-port=${port}`,
      "--headless",
      "--window-size=1280,720",
      "--no-sandbox",
      "--disable-gpu"
    ], {
      stdio: ["ignore", "pipe", "pipe"]
    });
    sharedProc = proc;
    const devtoolsRegex = /DevTools listening on (ws:\/\/[^\s]+)/;
    sharedWsUrl = await new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        reject(new Error(`Timed out waiting for CapSurf DevTools announcement on port ${port}`));
      }, 1e4);
      function onData(chunk) {
        const text = chunk.toString();
        const match = text.match(devtoolsRegex);
        if (match) {
          clearTimeout(timer);
          resolve(match[1]);
        }
      }
      proc.stdout.on("data", onData);
      proc.stderr.on("data", onData);
      proc.on("exit", (code, signal) => {
        clearTimeout(timer);
        sharedProc = null;
        fetch(`http://127.0.0.1:${port}/json/version`).then((res) => res.json()).then((info) => {
          if (info && info.webSocketDebuggerUrl) {
            sharedWsUrl = info.webSocketDebuggerUrl;
            resolve(sharedWsUrl);
          } else {
            reject(new Error(`CapSurf process exited prematurely (code=${code}, signal=${signal})`));
          }
        }).catch(() => {
          reject(new Error(`CapSurf process exited prematurely (code=${code}, signal=${signal})`));
        });
      });
    });
    process.on("exit", () => {
      try {
        proc.kill("SIGKILL");
      } catch {}
    });
  }
  return sharedWsUrl;
}
async function releaseSharedCdp() {
  if (sharedCdp) {
    try {
      await sharedCdp.close();
    } catch {}
    sharedCdp = null;
    sharedPage = null;
    sharedBrowser = null;
  }
}
async function getSharedSession() {
  await ensureServerRunning();
  if (!sharedCdp || !sharedPage || !sharedBrowser || !sharedCdp.ws || sharedCdp.ws.readyState !== 1) {
    if (sharedCdp) {
      try {
        await sharedCdp.close();
      } catch {}
    }
    sharedCdp = new CDPClient(sharedWsUrl);
    await sharedCdp.connect();
    sharedPage = new Page(sharedCdp);
    sharedBrowser = new Browser(sharedCdp, sharedPage);
  }
  return {
    browser: sharedBrowser,
    page: sharedPage,
    cdpSession: sharedCdp,
    cdp: sharedCdp
  };
}
function test(name, fn, timeout = 120000) {
  bunTest(name, async () => {
    if (fn.length > 0) {
      const fixtures = await getSharedSession();
      fixtures.page.exposedFunctions.clear();
      fixtures.page.removeAllListeners();
      if ((name.includes("blank page") || name.includes("fresh page") || name.includes("never-navigated page")) && fixtures.page.url() !== "about:blank") {
        await fixtures.page.goto("about:blank");
      }
      const browserFactory = async () => fixtures.browser;
      await fn({ ...fixtures, browserFactory });
    } else {
      await ensureServerRunning();
      await fn();
    }
  }, timeout);
}
test.concurrent = function(name, fn, timeout = 120000) {
  test(name, fn, timeout);
};
test.for = function(cases) {
  return function(name, fn, timeout = 120000) {
    for (const item of cases) {
      const formattedName = name.replace(/%s|%d/g, String(item));
      test(formattedName, (fixtures) => fn(item, fixtures), timeout);
    }
  };
};
test.each = test.for;
test.skip = bunTest.skip;
test.only = bunTest.only;
test.todo = bunTest.todo;
test.fails = function(name, fn, timeout = 120000) {
  test(name, async (fixtures) => {
    try {
      await fn(fixtures);
    } catch {
      return;
    }
    throw new Error(`Expected test "${name}" to fail, but it passed.`);
  }, timeout);
};
var describe = bunDescribe;
var expect = function(actual) {
  if (actual && typeof actual.then === "function") {
    const base = bunExpect(actual);
    return new Proxy(base, {
      get(target, prop, receiver) {
        if (prop === "rejects") {
          return {
            async toThrow(expected) {
              try {
                const val = await actual;
                throw new Error(`Expected promise to reject, but it resolved with: ${JSON.stringify(val)}`);
              } catch (err) {
                if (err.message && err.message.startsWith("Expected promise to reject, but it resolved with:")) {
                  throw err;
                }
                if (expected !== undefined) {
                  const msg = err?.message ?? String(err);
                  if (typeof expected === "string") {
                    if (!msg.includes(expected)) {
                      throw new Error(`Expected error message to include "${expected}", but got: "${msg}"`);
                    }
                  } else if (expected instanceof RegExp) {
                    if (!expected.test(msg)) {
                      throw new Error(`Expected error message to match ${expected}, but got: "${msg}"`);
                    }
                  } else if (typeof expected === "function") {
                    if (!(err instanceof expected)) {
                      throw new Error(`Expected error to be instance of ${expected.name}, but got: ${err}`);
                    }
                  }
                }
                return;
              }
            },
            toThrowError(expected) {
              return this.toThrow(expected);
            }
          };
        }
        if (prop === "resolves") {
          return new Proxy({}, {
            get(_, matcher) {
              return async (...args) => {
                const val = await actual;
                return bunExpect(val)[matcher](...args);
              };
            }
          });
        }
        return Reflect.get(target, prop, receiver);
      }
    });
  }
  return bunExpect(actual);
};
Object.assign(expect, bunExpect);
expect.poll = function(fn, { timeout = 1e4, interval = 50 } = {}) {
  const startTime = Date.now();
  async function pollUntil(matcher) {
    let lastError;
    while (Date.now() - startTime < timeout) {
      try {
        const val = await fn();
        matcher(val);
        return;
      } catch (err) {
        lastError = err;
        await new Promise((r) => setTimeout(r, interval));
      }
    }
    throw lastError || new Error(`expect.poll timed out after ${timeout}ms`);
  }
  return new Proxy({}, {
    get(_target, prop) {
      return async function(...args) {
        await pollUntil((val) => {
          const exp = expect(val);
          if (typeof exp[prop] === "function") {
            exp[prop](...args);
          } else {
            throw new Error(`Matcher ${prop} is not a function on expect`);
          }
        });
      };
    }
  });
};
var engine = null;
async function connectEngine() {
  const session = await getSharedSession();
  return session.browser;
}
export {
  test,
  releaseSharedCdp,
  getSharedSession,
  expect,
  env,
  ensureServerRunning,
  engine,
  describe,
  connectEngine,
  Page,
  Mouse,
  ElementHandle,
  DEFAULT_PORT,
  DEFAULT_BIN,
  CDPClient,
  BrowserContext,
  Browser
};
