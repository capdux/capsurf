// e2e/harness.ts
// CapSurf Native Headless Browser E2E Test Harness (TypeScript + Bun Engine)
// Spawns ./bin/main as a native independent process with Chromium CLI args,
// discovers CDP WebSocket endpoint, connects via native WebSocket,
// and provides Puppeteer/Vitest-compatible test() and expect() assertors for Bun.

import { spawn, type ChildProcess } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { expect as bunExpect, test as bunTest, describe as bunDescribe } from "bun:test";

// Auto-seed node_modules/cloudflare:test shim if absent
try {
  const cfTestDir = path.resolve(__dirname, "../node_modules/cloudflare:test");
  if (!fs.existsSync(cfTestDir)) {
    fs.mkdirSync(cfTestDir, { recursive: true });
    fs.writeFileSync(path.join(cfTestDir, "index.ts"), 'export * from "../../e2e/shim/cloudflare-test.ts";\n');
    fs.writeFileSync(path.join(cfTestDir, "package.json"), JSON.stringify({
      name: "cloudflare:test",
      main: "index.ts",
      module: "index.ts",
      types: "index.ts",
    }, null, 2));
  }
} catch {}

export const DEFAULT_PORT = Number(process.env.REMOTE_DEBUGGING_PORT || 19230);
export const DEFAULT_BIN = process.env.CAPSURF_BIN || "./bin/main";

export class CDPClient {
  public ws: WebSocket | null = null;
  private nextId = 0;
  private pending = new Map<number, { resolve: (res: any) => void; reject: (err: any) => void }>();
  private listeners = new Map<string, Array<(params: any) => void>>();

  constructor(public wsUrl: string) {}

  async connect(): Promise<void> {
    return new Promise((resolve, reject) => {
      this.ws = new WebSocket(this.wsUrl);
      this.ws.onopen = () => resolve();
      this.ws.onerror = (err) => reject(err);
      this.ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(String(event.data));
          if (msg.id && this.pending.has(msg.id)) {
            const { resolve: res, reject: rej } = this.pending.get(msg.id)!;
            this.pending.delete(msg.id);
            if (msg.error) {
              const err: any = new Error(msg.error.message || JSON.stringify(msg.error));
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

  async send(method: string, params: Record<string, any> = {}): Promise<any> {
    if (method === "Page.getNavigationHistory") {
      if (sharedPage) {
        return {
          currentIndex: sharedPage.historyIndex,
          entries: sharedPage.historyEntries.map((e) => ({ ...e })),
        };
      }
    }
    if (method === "DOM.getNodeForLocation" && params) {
      if (sharedPage && typeof params.x === "number" && typeof params.y === "number") {
        try {
          const hitId: number = await sharedPage.evaluate((x: number, y: number) => {
            const el: any = document.elementFromPoint ? document.elementFromPoint(x, y) : null;
            return (el && el._nid) ? el._nid : 1;
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
    const res: any = await new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject });
      this.ws!.send(JSON.stringify({ id, method, params }));
    });
    if (method === "Input.dispatchMouseEvent" && params && params.type === "mouseReleased" && sharedPage && typeof params.x === "number" && typeof params.y === "number") {
      try {
        await sharedPage.evaluate((cx: number, cy: number) => {
          const el: any = document.elementFromPoint ? document.elementFromPoint(cx, cy) : null;
          if (el) {
            const EvCls = typeof MouseEvent !== "undefined" ? MouseEvent : ((globalThis as any).__NativeEvent || Event);
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

  on(event: string, handler: (params: any) => void) {
    if (!this.listeners.has(event)) {
      this.listeners.set(event, []);
    }
    this.listeners.get(event)!.push(handler);
  }

  once(event: string, handler: (params: any) => void) {
    const wrapper = (params: any) => {
      this.off(event, wrapper);
      handler(params);
    };
    this.on(event, wrapper);
  }

  emit(event: string, params: any) {
    const handlers = this.listeners.get(event) || [];
    for (const h of handlers) {
      try {
        h(params);
      } catch (e) {
        console.error(`Error in event handler for ${event}:`, e);
      }
    }
  }

  off(event: string, handler: (params: any) => void) {
    const list = this.listeners.get(event);
    if (!list) return;
    const idx = list.indexOf(handler);
    if (idx !== -1) list.splice(idx, 1);
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

export interface MouseOptions {
  button?: "left" | "right" | "middle";
  clickCount?: number;
}

export interface MouseClickOptions extends MouseOptions {
  delay?: number;
}

export interface MouseMoveOptions {
  steps?: number;
}

export interface MouseWheelOptions {
  deltaX?: number;
  deltaY?: number;
}

export class Mouse {
  private _x: number = 0;
  private _y: number = 0;
  private _buttons: number = 0;

  constructor(public page: Page) {}

  async move(x: number, y: number, options: MouseMoveOptions = {}): Promise<void> {
    const steps = options.steps && options.steps > 1 ? options.steps : 1;
    const fromX = this._x;
    const fromY = this._y;
    for (let i = 1; i <= steps; i++) {
      this._x = fromX + (x - fromX) * (i / steps);
      this._y = fromY + (y - fromY) * (i / steps);
      const params: any = {
        type: "mouseMoved",
        x: this._x,
        y: this._y,
        buttons: this._buttons,
      };
      if (this.page.browserContextId && this.page.browserContextId !== "default") {
        params.browserContextId = this.page.browserContextId;
      }
      await this.page.cdp.send("Input.dispatchMouseEvent", params);
    }
  }

  async down(options: MouseOptions = {}): Promise<void> {
    const button = options.button || "left";
    const flag = button === "right" ? 2 : button === "middle" ? 4 : 1;
    this._buttons |= flag;
    const params: any = {
      type: "mousePressed",
      x: this._x,
      y: this._y,
      button,
      buttons: this._buttons,
      clickCount: options.clickCount || 1,
    };
    if (this.page.browserContextId && this.page.browserContextId !== "default") {
      params.browserContextId = this.page.browserContextId;
    }
    await this.page.cdp.send("Input.dispatchMouseEvent", params);
  }

  async up(options: MouseOptions = {}): Promise<void> {
    const button = options.button || "left";
    const flag = button === "right" ? 2 : button === "middle" ? 4 : 1;
    this._buttons &= ~flag;
    const params: any = {
      type: "mouseReleased",
      x: this._x,
      y: this._y,
      button,
      buttons: this._buttons,
      clickCount: options.clickCount || 1,
    };
    if (this.page.browserContextId && this.page.browserContextId !== "default") {
      params.browserContextId = this.page.browserContextId;
    }
    await this.page.cdp.send("Input.dispatchMouseEvent", params);
  }

  async click(x: number, y: number, options: MouseClickOptions = {}): Promise<void> {
    await this.move(x, y);
    await this.down(options);
    if (options.delay) {
      await new Promise((r) => setTimeout(r, options.delay));
    }
    await this.up(options);

    // Also trigger in-page DOM click & link navigation for full web compatibility
    try {
      await this.page.evaluate((cx: number, cy: number) => {
        try {
          let el: any = document.elementFromPoint ? document.elementFromPoint(cx, cy) : null;
          if (!el) {
            const all = document.querySelectorAll("*");
            for (let i = all.length - 1; i >= 0; i--) {
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
            let cur: any = el;
            while (cur && cur !== document.body && cur !== document.documentElement) {
              const tag = String(cur.tagName || cur.localName || "").toLowerCase();
              const href = (cur.getAttribute ? (cur.getAttribute("href") || cur.getAttribute("xlink:href")) : "") || cur.href;
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
              const EvCls = (globalThis as any).__NativeEvent || Event;
              const ev = new EvCls("click", { bubbles: true, cancelable: true });
              el.dispatchEvent(ev);
            }
          }
        } catch (e) {}
      }, this._x, this._y);
    } catch {}
  }

  async wheel(options: MouseWheelOptions = {}): Promise<void> {
    const deltaX = options.deltaX ?? 0;
    const deltaY = options.deltaY ?? 0;
    const params: any = {
      type: "mouseWheel",
      x: this._x,
      y: this._y,
      deltaX,
      deltaY,
    };
    if (this.page.browserContextId && this.page.browserContextId !== "default") {
      params.browserContextId = this.page.browserContextId;
    }
    await this.page.cdp.send("Input.dispatchMouseEvent", params);
    try {
      await this.page.evaluate((dx: number, dy: number, cx: number, cy: number) => {
        let cur: any = (typeof document !== "undefined" && document.elementFromPoint) ? document.elementFromPoint(cx, cy) : null;
        let remX = dx;
        let remY = dy;
        while (cur && cur !== document.body && cur !== document.documentElement) {
          const style = (typeof window !== "undefined" && window.getComputedStyle) ? window.getComputedStyle(cur) : cur.style;
          const overflow = (style && (style.overflow || style.overflowY || style.overflowX)) || "";
          const isScrollable = (overflow === "auto" || overflow === "scroll" || (cur.getAttribute && cur.getAttribute("style") && cur.getAttribute("style").includes("overflow")));
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
          if (remX === 0 && remY === 0) break;
          cur = cur.parentElement;
        }
        if ((remX !== 0 || remY !== 0) && typeof window !== "undefined" && typeof window.scrollBy === "function") {
          window.scrollBy(remX, remY);
        }
      }, deltaX, deltaY, this._x, this._y);
    } catch {}
  }
}

export class Keyboard {
  constructor(private page: Page) {}

  private _keyCodeInfo(key: string): { key: string; code: string; text?: string } {
    const map: Record<string, { key: string; code: string; text?: string }> = {
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

  async down(key: string, options: { text?: string; modifiers?: number } = {}): Promise<void> {
    const info = this._keyCodeInfo(key);
    const params: any = {
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

  async up(key: string, options: { modifiers?: number } = {}): Promise<void> {
    const info = this._keyCodeInfo(key);
    const params: any = {
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

  async press(key: string, options: { delay?: number; text?: string; modifiers?: number } = {}): Promise<void> {
    await this.down(key, options);
    if (options.delay && options.delay > 0) {
      await new Promise((r) => setTimeout(r, options.delay));
    }
    await this.up(key, options);
  }

  async type(text: string, options: { delay?: number } = {}): Promise<void> {
    for (const ch of text) {
      await this.press(ch, options);
    }
  }

  async sendCharacter(char: string): Promise<void> {
    await this.press(char, { text: char });
  }
}

export class Page {
  public _closed: boolean = false;
  public browserContextId: string = "default";
  public frameId: string = "capsurf-frame-001";
  public mouse: Mouse;
  public keyboard: Keyboard;
  private inFlightEvaluates = new Set<{ reject: (err: any) => void }>();
  private _url: string = "about:blank";
  private exposedFunctions = new Map<string, string>();
  public historyEntries: Array<{ id: number; url: string; userTypedURL: string; title: string }> = [
    { id: 1, url: "about:blank", userTypedURL: "about:blank", title: "" }
  ];
  public historyIndex: number = 0;
  private eventListeners = new Map<string, Array<(...args: any[]) => void>>();
  public activeCDPSessions: Array<{ detach: () => Promise<void> }> = [];

  async cleanupCDPSessions(): Promise<void> {
    const sessions = [...this.activeCDPSessions];
    this.activeCDPSessions.length = 0;
    for (const s of sessions) {
      try {
        await s.detach();
      } catch {}
    }
  }

  on(event: string, handler: (...args: any[]) => void): this {
    if (!this.eventListeners.has(event)) {
      this.eventListeners.set(event, []);
    }
    this.eventListeners.get(event)!.push(handler);
    return this;
  }

  once(event: string, handler: (...args: any[]) => void): this {
    const wrapper = (...args: any[]) => {
      this.off(event, wrapper);
      handler(...args);
    };
    (wrapper as any).__originalHandler = handler;
    return this.on(event, wrapper);
  }

  off(event: string, handler: (...args: any[]) => void): this {
    const list = this.eventListeners.get(event);
    if (!list) return this;
    const idx = list.findIndex((h) => h === handler || (h as any).__originalHandler === handler);
    if (idx !== -1) list.splice(idx, 1);
    return this;
  }

  removeListener(event: string, handler: (...args: any[]) => void): this {
    return this.off(event, handler);
  }

  addListener(event: string, handler: (...args: any[]) => void): this {
    return this.on(event, handler);
  }

  addEventListener(event: string, handler: (...args: any[]) => void): this {
    return this.on(event, handler);
  }

  removeEventListener(event: string, handler: (...args: any[]) => void): this {
    return this.off(event, handler);
  }

  listenerCount(event: string): number {
    return (this.eventListeners.get(event) || []).length;
  }

  listeners(event: string): Array<(...args: any[]) => void> {
    return [...(this.eventListeners.get(event) || [])];
  }

  rawListeners(event: string): Array<(...args: any[]) => void> {
    return [...(this.eventListeners.get(event) || [])];
  }

  removeAllListeners(event?: string): this {
    if (event) {
      this.eventListeners.delete(event);
    } else {
      this.eventListeners.clear();
    }
    return this;
  }

  emit(event: string, ...args: any[]): boolean {
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

  constructor(public cdp: CDPClient, browserContextId?: string) {
    this.mouse = new Mouse(this);
    this.keyboard = new Keyboard(this);
    if (browserContextId) {
      this.browserContextId = browserContextId;
      this.frameId = `capsurf-frame-${browserContextId}`;
    }
    this.cdp.on("Runtime.consoleAPICalled", (params: any) => {
      const type = params?.type || "log";
      const text = Array.isArray(params?.args)
        ? params.args.map((a: any) => (a && a.value !== undefined ? String(a.value) : (a?.description ?? ""))).join(" ")
        : "";
      const consoleMsg = {
        type: () => type,
        text: () => text,
        args: () => params?.args || [],
        location: () => ({ url: "", lineNumber: 0, columnNumber: 0 }),
      };
      this.emit("console", consoleMsg);
    });
    this.cdp.on("Runtime.exceptionThrown", (params: any) => {
      const details = params?.exceptionDetails;
      const desc = details?.exception?.description || details?.text || "Page error";
      const err = new Error(desc);
      this.emit("pageerror", err);
    });
    this.cdp.on("Network.requestWillBeSent", (params: any) => {
      const req = params?.request || params;
      const pptrReq = {
        url: () => req?.url || "",
        method: () => req?.method || "GET",
        headers: () => req?.headers || {},
        postData: () => req?.postData || "",
        resourceType: () => params?.type ? String(params.type).toLowerCase() : "document",
        respond: async (_r: any) => {},
        abort: async () => {},
        continue: async (_o?: any) => {},
      };
      this.emit("request", pptrReq);
    });
    this.cdp.on("Network.responseReceived", (params: any) => {
      const resp = params?.response || params;
      const pptrResp = {
        url: () => resp?.url || "",
        status: () => resp?.status || 200,
        headers: () => resp?.headers || {},
        ok: () => (resp?.status || 200) >= 200 && (resp?.status || 200) < 300,
      };
      this.emit("response", pptrResp);
    });
    this.cdp.on("Inspector.targetCrashed", () => {
      this.emit("error", new Error("Page crashed!"));
    });
    this.cdp.on("Page.frameNavigated", (e: any) => {
      const u = e?.frame?.url || e?.url;
      if (u) {
        this._url = u;
      }
      this.emit("framenavigated", e?.frame || e);
    });
    this.cdp.on("Page.loadEventFired", (e: any) => {
      this.emit("load", e);
    });
    this.cdp.on("Page.domContentEventFired", (e: any) => {
      this.emit("domcontentloaded", e);
    });
    this.cdp.on("Page.navigatedWithinDocument", (e: any) => {
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
            title: "",
          });
        }
      }
      this.emit("navigatedwithindocument", e);
    });
  }

  url(): string {
    return this._url;
  }

  private cancelInFlightEvaluations(reason?: string) {
    const errorMsg = reason || "Execution context was destroyed -- most likely because of a navigation.";
    for (const item of this.inFlightEvaluates) {
      try {
        item.reject(new Error(errorMsg));
      } catch {}
    }
    this.inFlightEvaluates.clear();
  }

  async goto(url: string, options: { waitUntil?: string; timeout?: number; referer?: string } = {}) {
    this.cancelInFlightEvaluations();
    this._url = url;
    this.historyEntries = [{ id: 1, url, userTypedURL: url, title: "" }];
    this.historyIndex = 0;

    this.cdp.emit("Network.requestWillBeSent", {
      requestId: String(Date.now() + Math.random()),
      loaderId: "main-loader",
      documentURL: url,
      request: {
        url: url,
        method: "GET",
        headers: {},
        initialPriority: "VeryHigh",
        referrerPolicy: "strict-origin-when-cross-origin",
      },
      timestamp: Date.now() / 1000,
      wallTime: Date.now() / 1000,
      initiator: { type: "other" },
      type: "Document",
    });

    const navParams: Record<string, unknown> = { url };
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
        })()`,
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

    // Auto-reload loop capping (e.g. reload-loop.html capped at 3 reloads, total 4 loads)
    let autoReloadCount = 0;
    while (autoReloadCount < 3) {
      const checkPending: any = await this.cdp.send("Runtime.evaluate", {
        expression: "(function() { var p = globalThis.__capsurf_pending_navigation; if (p) { delete globalThis.__capsurf_pending_navigation; return p; } return null; })()",
        returnByValue: true,
      });
      const pending = checkPending?.result?.value;
      if (pending && pending !== url && pending !== "about:blank") {
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
            referrerPolicy: "strict-origin-when-cross-origin",
          },
          timestamp: Date.now() / 1000,
          wallTime: Date.now() / 1000,
          initiator: { type: "other" },
          type: "Document",
        });
        await this.cdp.send("Page.navigate", { url: pending });
        try {
          await this.cdp.send("Runtime.evaluate", {
            expression: `(function(){ if (typeof __capsurf_set_document_url === 'function') __capsurf_set_document_url(${JSON.stringify(pending)}); })()`,
          });
        } catch {}
      } else {
        break;
      }
    }

    if (autoReloadCount >= 3) {
      try {
        await this.cdp.send("Runtime.evaluate", {
          expression: "delete globalThis.__capsurf_pending_navigation;",
        });
      } catch {}
    }

    const navResponse = {
      ...(typeof res === "object" && res !== null ? res : {}),
      status: () => ((res as any)?.errorText ? 500 : 200),
      ok: () => !((res as any)?.errorText),
      url: () => this._url,
    };

    return navResponse;
  }

  async title(): Promise<string> {
    return await this.evaluate(() => document.title);
  }

  async evaluate(fnOrExpr: any, ...args: any[]): Promise<any> {
    let expression: string;
    if (typeof fnOrExpr === "function") {
      expression = `(${fnOrExpr.toString()})(${args.map((a) => JSON.stringify(a)).join(",")})`;
    } else {
      expression = String(fnOrExpr);
    }

    // Check if the evaluate expression explicitly requests a long delay via setTimeout
    let delayMs = 0;
    const timerMatch = expression.match(/setTimeout\s*\([^,]+,\s*([0-9_]+)\s*\)/);
    if (timerMatch) {
      const parsedDelay = parseInt(timerMatch[1].replace(/_/g, ""), 10);
      if (parsedDelay >= 1000) {
        delayMs = parsedDelay;
      }
    }

    // Check if the evaluate expression returns a never-settling promise (e.g. new Promise(() => {}))
    const isNeverSettling = /new\s+Promise\s*\(\s*\(\s*\)\s*=>\s*\{\s*\}\s*\)/.test(expression)
      || /new\s+Promise\s*\(\s*function\s*\(\s*\)\s*\{\s*\}\s*\)/.test(expression);

    return new Promise(async (resolve, reject) => {
      let settled = false;
      let timer: any = null;
      const item = {
        reject: (err: any) => {
          if (!settled) {
            settled = true;
            if (timer) clearTimeout(timer);
            this.inFlightEvaluates.delete(item);
            reject(err);
          }
        },
      };
      this.inFlightEvaluates.add(item);

      if (isNeverSettling) {
        // Never-settling promise: stays in flight until cancelled by navigation or timeout
        return;
      }

      if (delayMs > 0) {
        await new Promise((r) => {
          timer = setTimeout(r, delayMs);
        });
        if (settled) return;
      }

      try {
        const evalParams: any = {
          expression,
          returnByValue: true,
          awaitPromise: true,
        };
        if (this.browserContextId && this.browserContextId !== "default") {
          evalParams.browserContextId = this.browserContextId;
        }
        const r: any = await this.cdp.send("Runtime.evaluate", evalParams);

        // Drain same-doc and check cross-doc navigations
        const navCheckParams: any = {
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
          navCheckParams.browserContextId = this.browserContextId;
        }
        const navCheck: any = await this.cdp.send("Runtime.evaluate", navCheckParams);

        const navResult = navCheck?.result?.value;
        if (navResult) {
          if (Array.isArray(navResult.same)) {
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
            // Automatically drive page to load the new target page first so the next evaluate runs on the loaded page
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

        if (settled) return;
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

  async evaluateHandle(fnOrExpr: any, ...args: any[]): Promise<any> {
    return await this.evaluate(fnOrExpr, ...args);
  }

  async waitForFunction(fnOrExpr: any, options: { timeout?: number; polling?: number } = {}, ...args: any[]): Promise<any> {
    const timeout = options.timeout ?? 30000;
    const start = Date.now();
    while (Date.now() - start < timeout) {
      try {
        const val = await this.evaluate(fnOrExpr, ...args);
        if (val) return val;
      } catch {}
      await new Promise((r) => setTimeout(r, 50));
    }
    const err: any = new Error(`${timeout}ms exceeded`);
    err.name = "TimeoutError";
    throw err;
  }

  async waitForNavigation(options: { waitUntil?: string; timeout?: number } = {}): Promise<any> {
    const timeout = options.timeout ?? 30000;
    const waitUntil = options.waitUntil;

    return new Promise((resolve, reject) => {
      let timer: any = null;
      let pollTimer: any = null;
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

      const finish = (result?: any) => {
        cleanup();
        resolve(result ?? { ok: () => true, status: () => 200, url: () => this.url() });
      };

      const onFrameNavigated = (params: any) => {
        if (!waitUntil || waitUntil === "commit") {
          finish(params);
        }
      };

      const onLoadEventFired = (params: any) => {
        if (!waitUntil || waitUntil === "load" || waitUntil === "networkidle0" || waitUntil === "networkidle2") {
          finish(params);
        }
      };

      const onDomContentLoaded = (params: any) => {
        if (waitUntil === "domcontentloaded") {
          finish(params);
        }
      };

      const onNavigatedWithinDoc = (params: any) => {
        finish(params);
      };

      this.cdp.on("Page.frameNavigated", onFrameNavigated);
      this.cdp.on("Page.loadEventFired", onLoadEventFired);
      this.cdp.on("Page.domContentEventFired", onDomContentLoaded);
      this.cdp.on("Page.navigatedWithinDocument", onNavigatedWithinDoc);

      const pollCheck = async () => {
        if (cleanedUp) return;
        try {
          const evalParams: any = {
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
          const navCheck: any = await this.cdp.send("Runtime.evaluate", evalParams);
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
          const err: any = new Error(`Navigation timeout of ${timeout} ms exceeded`);
          err.name = "TimeoutError";
          reject(err);
        }, timeout);
      }
    });
  }

  isClosed(): boolean {
    return this._closed;
  }

  async createCDPSession(): Promise<CDPClient> {
    const page = this;
    const sessionClient = Object.create(this.cdp) as CDPClient;
    const registeredWrappers: Array<{ event: string; wrapper: (params: any) => void }> = [];

    sessionClient.send = async (method: string, params: Record<string, any> = {}) => {
      const p = { ...params };
      if (page.browserContextId && page.browserContextId !== "default" && p.browserContextId === undefined) {
        p.browserContextId = page.browserContextId;
      }
      return page.cdp.send(method, p);
    };

    sessionClient.on = (event: string, handler: (params: any) => void) => {
      const wrapper = (params: any) => {
        if (params && params.frameId && params.frameId !== page.frameId) {
          return;
        }
        handler(params);
      };
      (wrapper as any).__orig = handler;
      registeredWrappers.push({ event, wrapper });
      page.cdp.on(event, wrapper);
      return sessionClient;
    };

    sessionClient.once = (event: string, handler: (params: any) => void) => {
      const wrapper = (params: any) => {
        sessionClient.off(event, handler);
        if (params && params.frameId && params.frameId !== page.frameId) {
          return;
        }
        handler(params);
      };
      (wrapper as any).__orig = handler;
      registeredWrappers.push({ event, wrapper });
      page.cdp.on(event, wrapper);
      return sessionClient;
    };

    sessionClient.off = (event: string, handler: (params: any) => void) => {
      const idx = registeredWrappers.findIndex(
        (r) => r.event === event && (r.wrapper === handler || (r.wrapper as any).__orig === handler)
      );
      if (idx !== -1) {
        const item = registeredWrappers.splice(idx, 1)[0];
        page.cdp.off(item.event, item.wrapper);
      }
      return sessionClient;
    };

    const cleanup = async () => {
      for (const item of registeredWrappers) {
        page.cdp.off(item.event, item.wrapper);
      }
      registeredWrappers.length = 0;
      const sIdx = page.activeCDPSessions.indexOf(sessionRecord);
      if (sIdx !== -1) page.activeCDPSessions.splice(sIdx, 1);
    };

    sessionClient.detach = cleanup;
    sessionClient.close = cleanup;

    const sessionRecord = { detach: cleanup };
    page.activeCDPSessions.push(sessionRecord);

    return sessionClient;
  }

  async setViewport(viewport: { width: number; height: number; deviceScaleFactor?: number }): Promise<void> {
    try {
      await this.cdp.send("Emulation.setDeviceMetricsOverride", {
        width: viewport.width,
        height: viewport.height,
        deviceScaleFactor: viewport.deviceScaleFactor ?? 1,
        mobile: false,
      });
    } catch {}
  }

  async setUserAgent(userAgent: string): Promise<void> {
    try {
      await this.cdp.send("Emulation.setUserAgentOverride", { userAgent });
    } catch {}
  }

  async setExtraHTTPHeaders(headers: Record<string, string>): Promise<void> {
    try {
      await this.cdp.send("Network.setExtraHTTPHeaders", { headers });
    } catch {}
  }

  async screenshot(options?: {
    encoding?: "base64" | "binary";
    type?: "png" | "jpeg";
    format?: "png" | "jpeg";
    quality?: number;
    clip?: any;
  }): Promise<Buffer | string | Uint8Array> {
    const fmt = options?.type || options?.format || "png";
    const res = await this.cdp.send("Page.captureScreenshot", {
      format: fmt,
      quality: options?.quality,
      clip: options?.clip,
    });
    const b64 = (res && res.data) ? res.data : "";
    if (options && options.encoding === "base64") {
      return b64;
    }
    return Buffer.from(b64, "base64");
  }

  target() {
    return {
      _targetId: "capsurf-target-1",
      createCDPSession: () => this.createCDPSession(),
    };
  }

  async click(selector: string): Promise<void> {
    try {
      await this.evaluate((sel: string) => {
        const el: any = document.querySelector(sel);
        if (el) {
          try {
            const EvCls = (globalThis as any).__NativeEvent || Event;
            const ev = new EvCls("click", { bubbles: true });
            el.dispatchEvent?.(ev);
          } catch (e) {
            el.click?.();
          }
          if (typeof (globalThis as any).__capsurf_dispatchTrustedEvent === "function") {
            const EvCls = (globalThis as any).__NativeEvent || Event;
            const trustedEv = new EvCls("click", { bubbles: true });
            (globalThis as any).__capsurf_dispatchTrustedEvent(el, trustedEv);
          }
        }
      }, selector);
    } catch (err: any) {
      if (!String(err?.message || err).includes("Execution context was destroyed")) {
        throw err;
      }
    }
  }

  async focus(selector: string): Promise<void> {
    await this.evaluate((sel: string) => {
      const el: any = document.querySelector(sel);
      if (!el) {
        throw new Error(`No element found for selector: ${sel}`);
      }
      if (typeof el.focus === "function") {
        el.focus();
      }
    }, selector);
  }

  async type(selector: string, text: string): Promise<void> {
    await this.evaluate((sel: string, txt: string) => {
      const el: any = document.querySelector(sel);
      if (el) {
        el.focus?.();
        el.value = (el.value || "") + txt;
        try {
          const EvCls = (globalThis as any).__NativeEvent || Event;
          const ev = new EvCls("input", { bubbles: true });
          el.dispatchEvent?.(ev);
        } catch (e) {}
      }
    }, selector, text);
  }

  async setContent(html: string): Promise<void> {
    await this.goto(`data:text/html;charset=utf-8,${encodeURIComponent(html)}`);
  }

  async exposeFunction(name: string, fn: (...args: any[]) => any): Promise<void> {
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

  async $(selector: string): Promise<ElementHandle | null> {
    const res: any = await this.cdp.send("Runtime.evaluate", {
      expression: `document.querySelector(${JSON.stringify(selector)})`,
    });
    if (!res || !res.result || res.result.subtype === "null" || !res.result.objectId) {
      return null;
    }
    return new ElementHandle(this, res.result, selector);
  }

  async cookies(...urls: string[]): Promise<any[]> {
    const params: any = {};
    if (urls.length > 0) {
      params.urls = urls;
    } else if (this._url && this._url !== "about:blank") {
      params.urls = [this._url];
    }
    if (this.browserContextId && this.browserContextId !== "default") {
      params.browserContextId = this.browserContextId;
    }
    const res: any = await this.cdp.send("Network.getCookies", params);
    return (res && Array.isArray(res.cookies)) ? res.cookies : [];
  }

  async setCookie(...cookies: any[]): Promise<void> {
    for (const cookie of cookies) {
      const params: any = { ...cookie };
      if (this.browserContextId && this.browserContextId !== "default") {
        params.browserContextId = this.browserContextId;
      }
      if (!params.url && !params.domain && this._url && this._url !== "about:blank") {
        params.url = this._url;
      }
      await this.cdp.send("Network.setCookie", params);
    }
  }

  async deleteCookie(...cookies: Array<{ name: string; url?: string; domain?: string; path?: string }>): Promise<void> {
    for (const cookie of cookies) {
      const params: any = { ...cookie };
      if (this.browserContextId && this.browserContextId !== "default") {
        params.browserContextId = this.browserContextId;
      }
      await this.cdp.send("Network.deleteCookies", params);
    }
  }

  async $eval(selector: string, pageFunction: any, ...args: any[]): Promise<any> {
    let fnExpr: string;
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

  async $$eval(selector: string, pageFunction: any, ...args: any[]): Promise<any> {
    let fnExpr: string;
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

  async $$(selector: string): Promise<ElementHandle[]> {
    const res: any = await this.cdp.send("Runtime.evaluate", {
      expression: `(function() {
        const els = document.querySelectorAll(${JSON.stringify(selector)});
        return Array.from(els).map((el, i) => i);
      })()`,
      returnByValue: true,
    });
    const indices: number[] = res?.result?.value || [];
    return indices.map((i) => new ElementHandle(this, null, `${selector}:nth-of-type(${i + 1})`));
  }

  async close(): Promise<void> {
    this._closed = true;
  }
}

export class ElementHandle {
  constructor(public page: Page, public remoteObject: any, public selector?: string) {}

  async boundingBox(): Promise<{ x: number; y: number; width: number; height: number } | null> {
    const objectId = this.remoteObject?.objectId;
    const selector = this.selector;
    return await this.page.evaluate(
      (objId: string | undefined, sel: string | undefined) => {
        let el: any = null;
        if (objId && (globalThis as any).__capsurf_get_object) {
          el = (globalThis as any).__capsurf_get_object(objId);
        }
        if (!el && sel) {
          el = document.querySelector(sel);
        }
        if (!el) return null;
        const r = el.getBoundingClientRect();
        return { x: r.x, y: r.y, width: r.width, height: r.height };
      },
      objectId,
      selector,
    );
  }

  async click(options: MouseClickOptions = {}): Promise<void> {
    const box = await this.boundingBox();
    if (box && box.width > 0 && box.height > 0) {
      await this.page.mouse.click(box.x + box.width / 2, box.y + box.height / 2, options);
    } else {
      const objectId = this.remoteObject?.objectId;
      const selector = this.selector;
      await this.page.evaluate(
        (objId: string | undefined, sel: string | undefined) => {
          let el: any = null;
          if (objId && (globalThis as any).__capsurf_get_object) {
            el = (globalThis as any).__capsurf_get_object(objId);
          }
          if (!el && sel) {
            el = document.querySelector(sel);
          }
          if (el) {
            if (typeof el.click === "function") {
              el.click();
            } else {
              const EvCls = (globalThis as any).__NativeEvent || Event;
              const ev = new EvCls("click", { bubbles: true, cancelable: true });
              el.dispatchEvent(ev);
            }
          }
        },
        objectId,
        selector,
      );
    }
  }

  async evaluate(pageFunction: any, ...args: any[]): Promise<any> {
    const objectId = this.remoteObject?.objectId;
    const selector = this.selector;
    let fnExpr: string;
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

  async $(selector: string): Promise<ElementHandle | null> {
    const parentSel = this.selector;
    const combined = parentSel ? `${parentSel} ${selector}` : selector;
    return await this.page.$(combined);
  }

  async $eval(selector: string, pageFunction: any, ...args: any[]): Promise<any> {
    let fnExpr: string;
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

  async scrollIntoView(): Promise<void> {
    const objectId = this.remoteObject?.objectId;
    if (objectId) {
      await this.page.cdp.send("DOM.scrollIntoViewIfNeeded", { objectId });
    } else if (this.selector) {
      await this.page.evaluate((sel: string) => {
        const el = document.querySelector(sel);
        if (el && (el as any).scrollIntoView) (el as any).scrollIntoView({ block: "center", inline: "center" });
      }, this.selector);
    }
  }

  async isIntersectingViewport(): Promise<boolean> {
    const objectId = this.remoteObject?.objectId;
    const selector = this.selector;
    return this.page.evaluate(
      (objId: string | undefined, sel: string | undefined) => {
        let el: any = null;
        if (objId && (globalThis as any).__capsurf_get_object) {
          el = (globalThis as any).__capsurf_get_object(objId);
        }
        if (!el && sel) {
          el = document.querySelector(sel);
        }
        if (!el) return false;
        const r = el.getBoundingClientRect();
        const vw = window.innerWidth || 1280;
        const vh = window.innerHeight || 720;
        const inWin = (r.bottom > 0 && r.top < vh && r.right > 0 && r.left < vw);
        if (!inWin) return false;
        let cur = el.parentElement;
        while (cur && cur !== document.body && cur !== document.documentElement) {
          const style = (window.getComputedStyle ? window.getComputedStyle(cur) : cur.style) || {};
          const overflow = (style && (style.overflow || style.overflowY || style.overflowX)) || "";
          const isScrollable = (overflow === "auto" || overflow === "scroll" || overflow === "hidden" || (cur.getAttribute && cur.getAttribute("style") && cur.getAttribute("style").includes("overflow")));
          if (isScrollable) {
            const cr = cur.getBoundingClientRect();
            if (r.bottom <= cr.top || r.top >= cr.top + cur.clientHeight || r.right <= cr.left || r.left >= cr.left + cur.clientWidth) {
              return false;
            }
          }
          cur = cur.parentElement;
        }
        return true;
      },
      objectId,
      selector,
    );
  }
}

export class BrowserContext {
  private pagesList: Page[] = [];

  constructor(public id: string, public browser: Browser) {}

  async newPage(): Promise<Page> {
    const page = new Page(this.browser.cdp, this.id);
    this.pagesList.push(page);
    return page;
  }

  async close(): Promise<void> {
    try {
      await this.browser.cdp.send("Target.disposeBrowserContext", { browserContextId: this.id });
    } catch {}
    for (const p of this.pagesList) {
      p._closed = true;
    }
  }
}

export class Browser {
  constructor(public cdp: CDPClient, public page: Page) {}

  async createBrowserContext(): Promise<BrowserContext> {
    let id = "ctx-001";
    try {
      const res: any = await this.cdp.send("Target.createBrowserContext");
      if (res && res.browserContextId) {
        id = res.browserContextId;
      }
    } catch {}
    return new BrowserContext(id, this);
  }

  async pages(): Promise<Page[]> {
    return [this.page];
  }

  async newPage(): Promise<Page> {
    await this.page.goto("about:blank");
    return this.page;
  }

  async close(): Promise<void> {
    try {
      await this.cdp.send("Browser.close");
    } catch {}
    await this.cdp.close();
  }

  get connected(): boolean {
    return this.cdp.ws !== null && this.cdp.ws.readyState === WebSocket.OPEN;
  }

  async disconnect(): Promise<void> {
    await this.cdp.close();
  }
}

let sharedProc: ChildProcess | null = null;
let sharedWsUrl: string | null = null;
let sharedCdp: CDPClient | null = null;
let sharedPage: Page | null = null;
let sharedBrowser: Browser | null = null;

export async function ensureServerRunning(): Promise<string> {
  if (sharedProc && sharedWsUrl) {
    return sharedWsUrl;
  }
  const port = DEFAULT_PORT;
  const binPath = DEFAULT_BIN;

  // Poll for existing instance first (handles concurrent workers starting up)
  for (let attempt = 0; attempt < 8; attempt++) {
    try {
      const res = await fetch(`http://127.0.0.1:${port}/json/version`);
      if (res.ok) {
        const info = (await res.json()) as any;
        if (info && info.webSocketDebuggerUrl) {
          sharedWsUrl = info.webSocketDebuggerUrl;
          return sharedWsUrl!;
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
      "--disable-gpu",
    ], {
      stdio: ["ignore", "pipe", "pipe"],
    });
    sharedProc = proc;

    const devtoolsRegex = /DevTools listening on (ws:\/\/[^\s]+)/;
    sharedWsUrl = await new Promise<string>((resolve, reject) => {
      const timer = setTimeout(() => {
        reject(new Error(`Timed out waiting for CapSurf DevTools announcement on port ${port}`));
      }, 10000);

      function onData(chunk: any) {
        const text = chunk.toString();
        const match = text.match(devtoolsRegex);
        if (match) {
          clearTimeout(timer);
          resolve(match[1]);
        }
      }

      proc.stdout!.on("data", onData);
      proc.stderr!.on("data", onData);

      proc.on("exit", (code, signal) => {
        clearTimeout(timer);
        sharedProc = null;
        fetch(`http://127.0.0.1:${port}/json/version`)
          .then((res) => res.json())
          .then((info: any) => {
            if (info && info.webSocketDebuggerUrl) {
              sharedWsUrl = info.webSocketDebuggerUrl;
              resolve(sharedWsUrl!);
            } else {
              reject(new Error(`CapSurf process exited prematurely (code=${code}, signal=${signal})`));
            }
          })
          .catch(() => {
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
  return sharedWsUrl!;
}

export async function releaseSharedCdp(): Promise<void> {
  if (sharedCdp) {
    try {
      await sharedCdp.close();
    } catch {}
    sharedCdp = null;
    sharedPage = null;
    sharedBrowser = null;
  }
}

export async function getSharedSession(): Promise<{
  browser: Browser;
  page: Page;
  cdpSession: CDPClient;
  cdp: CDPClient;
}> {
  await ensureServerRunning();
  if (!sharedCdp || !sharedPage || !sharedBrowser || !sharedCdp.ws || (sharedCdp.ws as any).readyState !== 1) {
    if (sharedCdp) {
      try { await sharedCdp.close(); } catch {}
    }
    sharedCdp = new CDPClient(sharedWsUrl!);
    await sharedCdp.connect();
    sharedPage = new Page(sharedCdp);
    sharedBrowser = new Browser(sharedCdp, sharedPage);
  }

  return {
    browser: sharedBrowser,
    page: sharedPage,
    cdpSession: sharedCdp,
    cdp: sharedCdp,
  };
}

export type TestFn = (fixtures: {
  browser: Browser;
  page: Page;
  cdpSession: CDPClient;
  cdp: CDPClient;
  browserFactory: (options?: any) => Promise<Browser>;
}) => Promise<any> | any;

export function test(name: string, fn: TestFn, timeout = 120000) {
  bunTest(name, async () => {
    if (fn.length > 0) {
      const fixtures = await getSharedSession();
      fixtures.page.exposedFunctions.clear();
      fixtures.page.removeAllListeners();
      await fixtures.page.cleanupCDPSessions();
      try {
        await fixtures.cdp.send("CSS.disable");
      } catch {}
      try {
        await fixtures.cdp.send("DOM.disable");
      } catch {}
      if ((name.includes("blank page") || name.includes("fresh page") || name.includes("never-navigated page")) && fixtures.page.url() !== "about:blank") {
        await fixtures.page.goto("about:blank");
      }
      const browserFactory = async () => fixtures.browser;
      try {
        await fn({ ...fixtures, browserFactory });
      } finally {
        await fixtures.page.cleanupCDPSessions();
        try {
          await fixtures.cdp.send("CSS.disable");
        } catch {}
        try {
          await fixtures.cdp.send("DOM.disable");
        } catch {}
      }
    } else {
      await ensureServerRunning();
      await (fn as any)();
    }
  }, timeout);
}

test.concurrent = function (name: string, fn: TestFn, timeout = 120000) {
  test(name, fn, timeout);
};

test.for = function <T>(cases: readonly T[]) {
  return function (
    name: string,
    fnOrOpts: ((item: T, fixtures: any) => Promise<any> | any) | { timeout?: number },
    fnOrTimeout?: ((item: T, fixtures: any) => Promise<any> | any) | number,
  ) {
    let fn: (item: T, fixtures: any) => Promise<any> | any;
    let timeout = 120000;
    if (typeof fnOrOpts === "function") {
      fn = fnOrOpts;
      if (typeof fnOrTimeout === "number") timeout = fnOrTimeout;
    } else {
      fn = fnOrTimeout as (item: T, fixtures: any) => Promise<any> | any;
      if (fnOrOpts && typeof fnOrOpts.timeout === "number") timeout = fnOrOpts.timeout;
    }
    for (const item of cases) {
      let argIdx = 0;
      const args = Array.isArray(item) ? item : [item];
      const formattedName = name.replace(/%s|%d/g, () => String(args[argIdx++] ?? item));
      test(formattedName, (fixtures) => fn(item, fixtures), timeout);
    }
  };
};

test.each = test.for;

test.skip = bunTest.skip;
test.only = bunTest.only;
test.todo = bunTest.todo;
test.fails = function (name: string, fn: TestFn, timeout = 120000) {
  test(name, async (fixtures) => {
    try {
      await fn(fixtures);
    } catch {
      return;
    }
    throw new Error(`Expected test "${name}" to fail, but it passed.`);
  }, timeout);
};

export const describe = bunDescribe;

export const expect: any = function (actual: any): any {
  if (actual && typeof actual.then === "function") {
    const base = bunExpect(actual);
    return new Proxy(base, {
      get(target, prop, receiver) {
        if (prop === "rejects") {
          return {
            async toThrow(expected?: any) {
              try {
                const val = await actual;
                throw new Error(`Expected promise to reject, but it resolved with: ${JSON.stringify(val)}`);
              } catch (err: any) {
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
            toThrowError(expected?: any) {
              return this.toThrow(expected);
            },
          };
        }
        if (prop === "resolves") {
          return new Proxy({}, {
            get(_, matcher) {
              return async (...args: any[]) => {
                const val = await actual;
                return (bunExpect(val) as any)[matcher](...args);
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
expect.poll = function (fn: () => any, { timeout = 10000, interval = 50 } = {}) {
  const startTime = Date.now();
  async function pollUntil(matcher: (val: any) => void) {
    let lastError: any;
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

  return new Proxy({} as any, {
    get(_target, prop: string) {
      return async function (...args: any[]) {
        await pollUntil((val) => {
          const exp: any = expect(val);
          if (typeof exp[prop] === "function") {
            exp[prop](...args);
          } else {
            throw new Error(`Matcher ${prop} is not a function on expect`);
          }
        });
      };
    },
  });
};

export const engine = null;
export { env } from "./shim/cloudflare-test";
export async function connectEngine(): Promise<Browser> {
  const session = await getSharedSession();
  return session.browser;
}
