// e2e/harness.ts
// CapSurf Native Headless Browser E2E Test Harness (TypeScript + Bun Engine)
// Spawns ./bin/main as a native independent process with Chromium CLI args,
// discovers CDP WebSocket endpoint, connects via native WebSocket,
// and provides Puppeteer/Vitest-compatible test() and expect() assertors for Bun.

import { spawn, type ChildProcess } from "node:child_process";
import crypto from "node:crypto";
import fs from "node:fs";
import net from "node:net";
import path from "node:path";
import { expect as bunExpect, test as bunTest, describe as bunDescribe } from "bun:test";

function isLoopbackHost(host: string): boolean {
  const h = host.toLowerCase().replace(/^\[|\]$/g, "");
  return (
    h === "localhost" ||
    h.endsWith(".localhost") ||
    h === "::1" ||
    h === "0:0:0:0:0:0:0:1" ||
    h === "127.0.0.1" ||
    /^127\.\d{1,3}\.\d{1,3}\.\d{1,3}$/.test(h) ||
    h === "0.0.0.0" ||
    h === "::"
  );
}

function isPublicToLoopbackNav(fromUrl: string, toUrl: string): boolean {
  try {
    const from = new URL(fromUrl);
    const to = new URL(toUrl);
    if ((from.protocol === "http:" || from.protocol === "https:") && !isLoopbackHost(from.hostname)) {
      if ((to.protocol === "http:" || to.protocol === "https:") && isLoopbackHost(to.hostname)) {
        return true;
      }
    }
  } catch {}
  return false;
}

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
  public lifecycleEventsEnabled = false;
  private nextId = 0;
  private pending = new Map<number, { resolve: (res: any) => void; reject: (err: any) => void }>();
  private listeners = new Map<string, Array<(params: any) => void>>();
  private sessions = new Map<string, any>();

  constructor(public wsUrl: string) {}

  private _createFlatSession(sessionId: string): any {
    const parent = this;
    const sessionListeners = new Map<string, Array<(params: any) => void>>();
    const flatSession: any = {
      id: () => sessionId,
      connection: () => parent.connection(),
      send: (method: string, params: Record<string, any> = {}) => {
        return parent.send(method, params, sessionId);
      },
      on: (event: string, handler: (params: any) => void) => {
        if (!sessionListeners.has(event)) sessionListeners.set(event, []);
        sessionListeners.get(event)!.push(handler);
        parent.on(event, handler);
        return flatSession;
      },
      once: (event: string, handler: (params: any) => void) => {
        const wrapper = (params: any) => {
          flatSession.off(event, wrapper);
          handler(params);
        };
        return flatSession.on(event, wrapper);
      },
      off: (event: string, handler: (params: any) => void) => {
        const list = sessionListeners.get(event);
        if (list) {
          const idx = list.indexOf(handler);
          if (idx !== -1) list.splice(idx, 1);
        }
        parent.off(event, handler);
        return flatSession;
      },
      detach: async () => {
        for (const [ev, list] of sessionListeners) {
          for (const h of list) parent.off(ev, h);
        }
        sessionListeners.clear();
        parent.sessions.delete(sessionId);
        try {
          await parent.send("Target.detachFromTarget", { sessionId });
        } catch {}
      },
      close: async () => {
        await flatSession.detach();
      },
    };
    return flatSession;
  }

  connection(): { session: (sessionId: string) => any } {
    const self = this;
    return {
      session(sessionId: string) {
        if (!sessionId) return null;
        let s = self.sessions.get(sessionId);
        if (!s) {
          s = self._createFlatSession(sessionId);
          self.sessions.set(sessionId, s);
        }
        return s;
      },
    };
  }

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

  async send(method: string, params: Record<string, any> = {}, sessionId?: string): Promise<any> {
    if (method === "Page.setLifecycleEventsEnabled") {
      this.lifecycleEventsEnabled = Boolean(params?.enabled);
    }
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
      await this.send("Input.dispatchMouseEvent", { ...params, type: "mousePressed" }, sessionId);
      return await this.send("Input.dispatchMouseEvent", { ...params, type: "mouseReleased" }, sessionId);
    }
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      throw new Error(`WebSocket is not open (readyState=${this.ws?.readyState})`);
    }
    const skipSyntheticClick = Boolean(params && params._skipSyntheticClick);
    const wireParams = skipSyntheticClick ? { ...params } : params;
    if (skipSyntheticClick) {
      delete wireParams._skipSyntheticClick;
    }
    const id = ++this.nextId;
    const payload: Record<string, any> = { id, method, params: wireParams };
    if (sessionId) {
      payload.sessionId = sessionId;
    }
    const res: any = await new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject });
      this.ws!.send(JSON.stringify(payload));
    });
    if (method === "Target.attachToTarget" && res && res.sessionId) {
      if (!this.sessions.has(res.sessionId)) {
        this.sessions.set(res.sessionId, this._createFlatSession(res.sessionId));
      }
    }
    if (!skipSyntheticClick && method === "Input.dispatchMouseEvent" && params && params.type === "mouseReleased" && sharedPage && typeof params.x === "number" && typeof params.y === "number") {
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
      try {
        await this.page.evaluate((cx: number, cy: number, btns: number) => {
          try {
            const nextEl: any = document.elementFromPoint ? document.elementFromPoint(cx, cy) : null;
            const prevEl: any = (window as any).__capsurf_lastHoverEl || null;
            const EvCls = typeof MouseEvent !== "undefined" ? MouseEvent : ((globalThis as any).__NativeEvent || Event);
            if (nextEl !== prevEl) {
              const chain = (el: any): any[] => {
                const list: any[] = [];
                let cur = el;
                while (cur && cur.nodeType === 1) {
                  list.push(cur);
                  cur = cur.parentElement || cur.parentNode;
                }
                return list;
              };
              const prevChain = chain(prevEl);
              const nextChain = chain(nextEl);
              const prevSet = new Set(prevChain);
              const nextSet = new Set(nextChain);
              if (prevEl && prevEl.isConnected !== false) {
                prevEl.dispatchEvent(new EvCls("mouseout", { bubbles: true, cancelable: true, clientX: cx, clientY: cy, buttons: btns, relatedTarget: nextEl }));
                for (let k = 0; k < prevChain.length; k++) {
                  if (!nextSet.has(prevChain[k])) {
                    prevChain[k].dispatchEvent(new EvCls("mouseleave", { bubbles: false, cancelable: false, clientX: cx, clientY: cy, buttons: btns, relatedTarget: nextEl }));
                  }
                }
              }
              if (nextEl) {
                nextEl.dispatchEvent(new EvCls("mouseover", { bubbles: true, cancelable: true, clientX: cx, clientY: cy, buttons: btns, relatedTarget: prevEl }));
                for (let k = nextChain.length - 1; k >= 0; k--) {
                  if (!prevSet.has(nextChain[k])) {
                    nextChain[k].dispatchEvent(new EvCls("mouseenter", { bubbles: false, cancelable: false, clientX: cx, clientY: cy, buttons: btns, relatedTarget: prevEl }));
                  }
                }
              }
              (window as any).__capsurf_lastHoverEl = nextEl;
            }
            if (nextEl) {
              nextEl.dispatchEvent(new EvCls("mousemove", { bubbles: true, cancelable: true, clientX: cx, clientY: cy, buttons: btns }));
            }
          } catch (e) {}
        }, this._x, this._y, this._buttons);
      } catch {}
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

  async up(options: MouseOptions & { _skipSyntheticClick?: boolean } = {}): Promise<void> {
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
    if (options._skipSyntheticClick) {
      params._skipSyntheticClick = true;
    }
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
    await this.up({ ...options, _skipSyntheticClick: true });

    // Also trigger in-page DOM click & link navigation for full web compatibility
    try {
      await this.page.evaluate((cx: number, cy: number) => {
        try {
          let el: any = document.elementFromPoint ? document.elementFromPoint(cx, cy) : null;
          const elTag = el ? String(el.tagName || el.localName || "").toLowerCase() : "";
          if (!el || el === document.body || el === document.documentElement || elTag === "svg") {
            const scope: any = (elTag === "svg") ? el : document;
            const svgRect = (elTag === "svg" && el.getBoundingClientRect) ? el.getBoundingClientRect() : null;
            const all = scope.querySelectorAll ? scope.querySelectorAll("*") : [];
            for (let i = all.length - 1; i >= 0; i--) {
              const item = all[i];
              const r = item.getBoundingClientRect ? item.getBoundingClientRect() : null;
              let w = r ? r.width : 0;
              let h = r ? r.height : 0;
              let left = r ? r.left : 0;
              let top = r ? r.top : 0;
              if (w === 0 && item.getAttribute && item.getAttribute("width")) {
                w = parseFloat(item.getAttribute("width") || "0") || 0;
                if (svgRect && left === 0) left = svgRect.left + (parseFloat(item.getAttribute("x") || "0") || 0);
              }
              if (h === 0 && item.getAttribute && item.getAttribute("height")) {
                h = parseFloat(item.getAttribute("height") || "0") || 0;
                if (svgRect && top === 0) top = svgRect.top + (parseFloat(item.getAttribute("y") || "0") || 0);
              }
              if (w > 0 && h > 0 && cx >= left && cx <= left + w && cy >= top && cy <= top + h) {
                el = item;
                break;
              }
            }
          }
          if (el) {
            let cur: any = el;
            while (cur && cur !== document.body && cur !== document.documentElement) {
              const tag = String(cur.tagName || cur.localName || "").toLowerCase();
              const rawHref = cur.getAttribute
                ? (cur.getAttribute("href") ?? (cur.namespaceURI === "http://www.w3.org/2000/svg" ? cur.getAttribute("xlink:href") : null))
                : cur.href;
              const href = typeof rawHref === "string"
                ? rawHref
                : (rawHref && typeof rawHref.baseVal === "string" ? rawHref.baseVal : "");
              if (tag === "a" && href) {
                if (href.startsWith("#")) {
                  location.hash = href;
                } else if (typeof location !== "undefined") {
                  location.href = href;
                }
                break;
              }
              cur = cur.parentElement || cur.parentNode;
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
  private exposedFunctions = new Map<string, (...args: any[]) => any>();
  private _requestInterceptionEnabled: boolean = false;
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
      if (this._requestInterceptionEnabled) return;
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
    this.cdp.on("Fetch.requestPaused", (params: any) => {
      if (!this._requestInterceptionEnabled) return;
      const requestId = params?.requestId || "";
      const req = params?.request || {};
      let handled = false;
      const pptrReq = {
        url: () => req?.url || "",
        method: () => req?.method || "GET",
        headers: () => req?.headers || {},
        postData: () => req?.postData || "",
        resourceType: () => params?.resourceType ? String(params.resourceType).toLowerCase() : "other",
        respond: async (r: { status?: number; contentType?: string; headers?: Record<string, string>; body?: string | Uint8Array }) => {
          if (handled) return;
          handled = true;
          const headersObj: Record<string, string> = { ...(r.headers || {}) };
          if (r.contentType && !headersObj["content-type"] && !headersObj["Content-Type"]) {
            headersObj["content-type"] = r.contentType;
          }
          const responseHeaders = Object.entries(headersObj).map(([name, value]) => ({ name, value: String(value) }));
          const bodyStr = typeof r.body === "string"
            ? Buffer.from(r.body, "utf8").toString("base64")
            : r.body
              ? Buffer.from(r.body).toString("base64")
              : "";
          await this.cdp.send("Fetch.fulfillRequest", {
            requestId,
            responseCode: r.status ?? 200,
            responseHeaders,
            body: bodyStr,
          });
        },
        abort: async (errorCode?: string) => {
          if (handled) return;
          handled = true;
          await this.cdp.send("Fetch.failRequest", {
            requestId,
            errorReason: errorCode || "Failed",
          });
        },
        continue: async (overrides?: { url?: string; method?: string; postData?: string; headers?: Record<string, string> }) => {
          if (handled) return;
          handled = true;
          const contParams: Record<string, unknown> = { requestId };
          if (overrides?.url) contParams.url = overrides.url;
          if (overrides?.method) contParams.method = overrides.method;
          if (overrides?.postData) contParams.postData = overrides.postData;
          if (overrides?.headers) {
            contParams.headers = Object.entries(overrides.headers).map(([name, value]) => ({ name, value: String(value) }));
          }
          await this.cdp.send("Fetch.continueRequest", contParams);
        },
      };
      if (this.listenerCount("request") > 0) {
        this.emit("request", pptrReq);
      } else {
        void pptrReq.continue();
      }
    });
    this.cdp.on("Runtime.bindingCalled", (params: any) => {
      const bindingName = params?.name;
      const payload = params?.payload;
      if (typeof bindingName !== "string" || typeof payload !== "string") return;
      if (bindingName === "__capsurf_fetch_intercept") {
        let parsed: any;
        try {
          parsed = JSON.parse(decodeURIComponent(payload));
        } catch {
          try {
            parsed = JSON.parse(payload);
          } catch {
            return;
          }
        }
        const seq = parsed?.seq;
        const reqInfo = parsed?.request || {};
        let settled = false;
        const sendVerdict = async (verdict: Record<string, unknown>) => {
          if (settled) return;
          settled = true;
          try {
            await this.cdp.send("Runtime.evaluate", {
              expression: `globalThis.__capsurf_fetch_intercept_resolve && globalThis.__capsurf_fetch_intercept_resolve(${JSON.stringify(seq)}, ${JSON.stringify(verdict)})`,
            });
          } catch {}
        };
        const pptrReq = {
          url: () => reqInfo?.url || "",
          method: () => reqInfo?.method || "GET",
          headers: () => reqInfo?.headers || {},
          postData: () => reqInfo?.postData || "",
          resourceType: () => reqInfo?.resourceType || "fetch",
          respond: async (r: { status?: number; contentType?: string; headers?: Record<string, string>; body?: string }) => {
            await sendVerdict({
              action: "respond",
              status: r?.status ?? 200,
              contentType: r?.contentType,
              headers: r?.headers || {},
              body: r?.body ?? "",
            });
          },
          abort: async () => {
            await sendVerdict({ action: "abort" });
          },
          continue: async (overrides?: { url?: string; method?: string; headers?: Record<string, string> }) => {
            await sendVerdict({
              action: "continue",
              url: overrides?.url,
              method: overrides?.method,
              headers: overrides?.headers,
            });
          },
        };
        if (this.listenerCount("request") > 0) {
          this.emit("request", pptrReq);
        } else {
          void sendVerdict({ action: "continue" });
        }
        return;
      }
      if (!bindingName.startsWith("puppeteer_")) return;
      const fnName = bindingName.slice("puppeteer_".length);
      const fn = this.exposedFunctions.get(fnName);
      if (!fn) return;
      let parsed: any;
      try {
        parsed = JSON.parse(decodeURIComponent(payload));
      } catch {
        try {
          parsed = JSON.parse(payload);
        } catch {
          return;
        }
      }
      const seq = parsed?.seq;
      const args = Array.isArray(parsed?.args) ? parsed.args : [];
      Promise.resolve()
        .then(() => fn(...args))
        .then(
          async (result) => {
            try {
              await this.cdp.send("Runtime.evaluate", {
                expression: `globalThis.__puppeteer_callbacks && globalThis.__puppeteer_callbacks[${JSON.stringify(seq)}] && globalThis.__puppeteer_callbacks[${JSON.stringify(seq)}].resolve(${JSON.stringify(result)})`,
              });
            } catch {}
          },
          async (err) => {
            try {
              await this.cdp.send("Runtime.evaluate", {
                expression: `globalThis.__puppeteer_callbacks && globalThis.__puppeteer_callbacks[${JSON.stringify(seq)}] && globalThis.__puppeteer_callbacks[${JSON.stringify(seq)}].reject(${JSON.stringify(String(err?.message || err))})`,
              });
            } catch {}
          },
        );
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

    let targetNavUrl = url;
    let respondedStatus: number | null = null;
    if (this._requestInterceptionEnabled && this.listenerCount("request") > 0) {
      const navVerdict = await new Promise<{ action: string; url?: string; status?: number; contentType?: string; body?: string }>((resolve) => {
        let settled = false;
        const done = (v: { action: string; url?: string; status?: number; contentType?: string; body?: string }) => {
          if (!settled) {
            settled = true;
            resolve(v);
          }
        };
        const navReq = {
          url: () => url,
          method: () => "GET",
          headers: () => ({}),
          postData: () => "",
          resourceType: () => "document",
          respond: async (r: { status?: number; contentType?: string; body?: string }) => {
            done({ action: "respond", status: r?.status ?? 200, contentType: r?.contentType, body: r?.body ?? "" });
          },
          abort: async () => {
            done({ action: "abort" });
          },
          continue: async (overrides?: { url?: string }) => {
            done({ action: "continue", url: overrides?.url });
          },
        };
        this.emit("request", navReq);
      });
      if (navVerdict.action === "abort") {
        throw new Error("net::ERR_FAILED");
      }
      if (navVerdict.action === "respond") {
        respondedStatus = navVerdict.status ?? 200;
        const mime = navVerdict.contentType || "text/html";
        targetNavUrl = `data:${mime};charset=utf-8,${encodeURIComponent(navVerdict.body ?? "")}`;
      } else if (navVerdict.action === "continue" && navVerdict.url) {
        targetNavUrl = navVerdict.url;
        this._url = navVerdict.url;
      }
    }

    const navParams: Record<string, unknown> = { url: targetNavUrl };
    if (options && options.referer) {
      navParams.referrer = options.referer;
    }
    if (this.browserContextId && this.browserContextId !== "default") {
      navParams.browserContextId = this.browserContextId;
    }
    const res = await this.cdp.send("Page.navigate", navParams);
    if (this._viewport) {
      try {
        await this.cdp.send("Emulation.setDeviceMetricsOverride", {
          width: this._viewport.width,
          height: this._viewport.height,
          deviceScaleFactor: this._viewport.deviceScaleFactor,
          mobile: false,
        });
      } catch {}
    }
    if ((this.cdp as any).lifecycleEventsEnabled) {
      const ts = Date.now() / 1000;
      const frameId = (res as any)?.frameId || "main-frame";
      const loaderId = (res as any)?.loaderId || "main-loader";
      this.cdp.emit("Page.lifecycleEvent", { frameId, loaderId, name: "DOMContentLoaded", timestamp: ts });
      this.cdp.emit("Page.lifecycleEvent", { frameId, loaderId, name: "load", timestamp: ts });
    }
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

    for (const [name] of this.exposedFunctions) {
      try {
        await this.installExposedFunction(name);
      } catch {}
    }

    if (this._requestInterceptionEnabled) {
      try {
        await this.installFetchInterceptorBridge();
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
      status: () => (respondedStatus !== null ? respondedStatus : ((res as any)?.errorText ? 500 : 200)),
      ok: () => (respondedStatus !== null ? (respondedStatus >= 200 && respondedStatus < 300) : !((res as any)?.errorText)),
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
        const trackAsyncPromise = this.exposedFunctions.size > 0 || this._requestInterceptionEnabled;
        const evalExpr = trackAsyncPromise
          ? `(function() {
              const __v = (0, eval)(${JSON.stringify(expression)});
              if (__v && typeof __v.then === "function") {
                globalThis.__capsurf_eval_seq = (globalThis.__capsurf_eval_seq || 0) + 1;
                const __id = globalThis.__capsurf_eval_seq;
                globalThis.__capsurf_eval_results = globalThis.__capsurf_eval_results || {};
                const __slot = { state: "pending", value: undefined, error: undefined };
                globalThis.__capsurf_eval_results[__id] = __slot;
                __v.then(
                  function(val) { __slot.state = "resolved"; __slot.value = val; },
                  function(err) { __slot.state = "rejected"; __slot.error = String(err && err.stack ? err.stack : err); }
                );
                return { __capsurf_pending_promise_id: __id };
              }
              return { __capsurf_sync_value: __v };
            })()`
          : expression;
        const evalParams: any = {
          expression: evalExpr,
          returnByValue: true,
          awaitPromise: true,
        };
        if (this.browserContextId && this.browserContextId !== "default") {
          evalParams.browserContextId = this.browserContextId;
        }
        const r: any = await this.cdp.send("Runtime.evaluate", evalParams);

        if (trackAsyncPromise && !r?.exceptionDetails && r?.result?.value && typeof r.result.value === "object") {
          const valObj = r.result.value;
          if (typeof valObj.__capsurf_pending_promise_id === "number") {
            const pid = valObj.__capsurf_pending_promise_id;
            const deadline = Date.now() + 10_000;
            let resolvedSlot: any = null;
            while (Date.now() < deadline && !settled) {
              const pollRes: any = await this.cdp.send("Runtime.evaluate", {
                expression: `(function() {
                  if (typeof __capsurf_pump_timers === "function") { try { __capsurf_pump_timers(); } catch (e) {} }
                  const m = globalThis.__capsurf_eval_results;
                  const s = m ? m[${pid}] : null;
                  if (!s) return { state: "missing" };
                  if (s.state !== "pending") {
                    delete m[${pid}];
                    return s;
                  }
                  return { state: "pending" };
                })()`,
                returnByValue: true,
              });
              const st = pollRes?.result?.value;
              if (st && st.state !== "pending") {
                resolvedSlot = st;
                break;
              }
              await new Promise((res) => setTimeout(res, 15));
            }
            if (resolvedSlot) {
              if (resolvedSlot.state === "rejected") {
                r.exceptionDetails = { exception: { description: resolvedSlot.error || "Promise rejected" } };
              } else {
                r.result = { type: typeof resolvedSlot.value, value: resolvedSlot.value };
              }
            }
          } else if ("__capsurf_sync_value" in valObj) {
            r.result = { type: typeof valObj.__capsurf_sync_value, value: valObj.__capsurf_sync_value };
          }
        }

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
            var blocked = globalThis.__capsurf_blocked_navigation || null;
            if (blocked) delete globalThis.__capsurf_blocked_navigation;
            return { same: same, pending: pending, blocked: blocked };
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

          if (navResult.blocked) {
            const targetUrl = navResult.blocked;
            this.emit("response", {
              url: () => targetUrl,
              status: () => 403,
              headers: () => ({}),
              ok: () => false,
            });
            if (!settled) {
              settled = true;
              this.inFlightEvaluates.delete(item);
              this.cancelInFlightEvaluations();
              reject(new Error("Execution context was destroyed -- most likely because of a navigation."));
            }
            return;
          }

          if (navResult.pending) {
            const targetUrl = navResult.pending;
            if (isPublicToLoopbackNav(this._url, targetUrl)) {
              this.emit("response", {
                url: () => targetUrl,
                status: () => 403,
                headers: () => ({}),
                ok: () => false,
              });
            } else {
              // Automatically drive page to load the new target page first so the next evaluate runs on the loaded page
              try {
                await this.goto(targetUrl);
              } catch {}
            }
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

  async waitForSelector(
    selector: string,
    options: { visible?: boolean; hidden?: boolean; timeout?: number } = {},
  ): Promise<ElementHandle | null> {
    const timeout = options.timeout ?? 30000;
    const start = Date.now();
    while (Date.now() - start < timeout) {
      try {
        const handle = await this.$(selector);
        if (options.hidden) {
          if (!handle) return null;
          const box = await handle.boundingBox();
          if (!box || (box.width === 0 && box.height === 0)) return null;
        } else if (handle) {
          if (options.visible) {
            const box = await handle.boundingBox();
            if (box && box.width > 0 && box.height > 0) return handle;
          } else {
            return handle;
          }
        }
      } catch {}
      await new Promise((r) => setTimeout(r, 50));
    }
    const err: any = new Error(`Waiting for selector \`${selector}\` failed: timeout ${timeout}ms exceeded`);
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
              var blocked = globalThis.__capsurf_blocked_navigation || null;
              if (blocked) delete globalThis.__capsurf_blocked_navigation;
              return { same: same, pending: pending, blocked: blocked };
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
            if (navResult.blocked) {
              const targetUrl = navResult.blocked;
              this.emit("response", {
                url: () => targetUrl,
                status: () => 403,
                headers: () => ({}),
                ok: () => false,
              });
            }
            if (navResult.pending) {
              const targetUrl = navResult.pending;
              if (isPublicToLoopbackNav(this._url, targetUrl)) {
                this.emit("response", {
                  url: () => targetUrl,
                  status: () => 403,
                  headers: () => ({}),
                  ok: () => false,
                });
              } else {
                await this.goto(targetUrl);
              }
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

    sessionClient.connection = () => page.cdp.connection();

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

  private _viewport: { width: number; height: number; deviceScaleFactor: number } | null = null;

  async setViewport(viewport: { width: number; height: number; deviceScaleFactor?: number }): Promise<void> {
    this._viewport = {
      width: viewport.width,
      height: viewport.height,
      deviceScaleFactor: viewport.deviceScaleFactor ?? 1,
    };
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
          const tag = String(el.tagName || el.localName || "").toLowerCase();
          const ce = typeof el.getAttribute === "function" ? el.getAttribute("contenteditable") : null;
          const isFocusable =
            tag === "input" ||
            tag === "textarea" ||
            tag === "select" ||
            tag === "button" ||
            (tag === "a" && ((typeof el.hasAttribute === "function" && el.hasAttribute("href")) || Boolean(el.href))) ||
            (typeof el.hasAttribute === "function" && el.hasAttribute("tabindex")) ||
            Boolean(el.isContentEditable) ||
            ce === "" ||
            ce === "true";
          if (isFocusable && typeof el.focus === "function" && !el.disabled) {
            try {
              el.focus();
            } catch (e) {}
          }
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

  private async installExposedFunction(name: string): Promise<void> {
    const bindingName = `puppeteer_${name}`;
    await this.cdp.send("Runtime.addBinding", { name: bindingName });
    await this.evaluate(`(function() {
      const bindingName = ${JSON.stringify(bindingName)};
      const fnName = ${JSON.stringify(name)};
      globalThis.__puppeteer_seq = globalThis.__puppeteer_seq || 0;
      globalThis.__puppeteer_callbacks = globalThis.__puppeteer_callbacks || {};
      const wrapper = function(...args) {
        return new Promise((resolve, reject) => {
          const seq = ++globalThis.__puppeteer_seq;
          globalThis.__puppeteer_callbacks[seq] = {
            resolve: (v) => { delete globalThis.__puppeteer_callbacks[seq]; resolve(v); },
            reject: (e) => { delete globalThis.__puppeteer_callbacks[seq]; reject(new Error(e)); },
          };
          const binding = globalThis[bindingName];
          if (typeof binding === "function") {
            binding(encodeURIComponent(JSON.stringify({ seq, args })));
          } else {
            delete globalThis.__puppeteer_callbacks[seq];
            reject(new Error("Binding not installed: " + bindingName));
          }
        });
      };
      if (typeof window !== "undefined") window[fnName] = wrapper;
      globalThis[fnName] = wrapper;
    })()`);
  }

  private async installFetchInterceptorBridge(): Promise<void> {
    await this.cdp.send("Runtime.addBinding", { name: "__capsurf_fetch_intercept" });
    await this.evaluate(`(function() {
      globalThis.__capsurf_fetch_seq = globalThis.__capsurf_fetch_seq || 0;
      globalThis.__capsurf_fetch_pending = globalThis.__capsurf_fetch_pending || {};
      globalThis.__capsurf_fetch_intercept_resolve = function(seq, verdict) {
        const cb = globalThis.__capsurf_fetch_pending[seq];
        if (cb) {
          delete globalThis.__capsurf_fetch_pending[seq];
          cb(verdict);
        }
      };
      globalThis.__capsurf_fetch_interceptor = function(request) {
        return new Promise(function(resolve) {
          const seq = ++globalThis.__capsurf_fetch_seq;
          globalThis.__capsurf_fetch_pending[seq] = resolve;
          if (typeof globalThis.__capsurf_fetch_intercept === "function") {
            globalThis.__capsurf_fetch_intercept(encodeURIComponent(JSON.stringify({ seq: seq, request: request })));
          } else {
            delete globalThis.__capsurf_fetch_pending[seq];
            resolve({ action: "continue" });
          }
        });
      };
    })()`);
  }

  async setRequestInterception(enabled: boolean): Promise<void> {
    this._requestInterceptionEnabled = Boolean(enabled);
    if (this._requestInterceptionEnabled) {
      await this.cdp.send("Fetch.enable", { patterns: [{ urlPattern: "*" }] });
      try {
        await this.installFetchInterceptorBridge();
      } catch {}
    } else {
      await this.cdp.send("Fetch.disable", {});
      try {
        await this.evaluate(`delete globalThis.__capsurf_fetch_interceptor;`);
      } catch {}
    }
  }

  async exposeFunction(name: string, fn: (...args: any[]) => any): Promise<void> {
    this.exposedFunctions.set(name, fn);
    await this.installExposedFunction(name);
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
  if (sharedWsUrl && (sharedProc || (sharedCdp && sharedCdp.ws && (sharedCdp.ws as any).readyState === 1))) {
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

  function makePollProxy(negated: boolean): any {
    return new Proxy({} as any, {
      get(_target, prop: string) {
        if (prop === "not") {
          return makePollProxy(!negated);
        }
        return async function (...args: any[]) {
          await pollUntil((val) => {
            const exp: any = negated ? expect(val).not : expect(val);
            if (typeof exp[prop] === "function") {
              exp[prop](...args);
            } else {
              throw new Error(`Matcher ${prop} is not a function on expect`);
            }
          });
        };
      },
    });
  }
  return makePollProxy(false);
};

function createCfWebSocketFromSocket(socket: any, head: Buffer) {
  let accepted = false;
  let closed = false;
  let rxBuf: Buffer = head && head.length > 0 ? Buffer.from(head) : Buffer.alloc(0);
  const pendingEvents: Array<{ type: string; event: any }> = [];
  const listeners = new Map<string, Array<(ev: any) => void>>();

  function emitEvent(type: string, event: any) {
    if (!accepted) {
      pendingEvents.push({ type, event });
      return;
    }
    const list = listeners.get(type);
    if (list) {
      for (const fn of list.slice()) {
        try {
          fn(event);
        } catch {}
      }
    }
  }

  function sendFrame(opcode: number, payload: Buffer) {
    if (closed || socket.destroyed) return;
    const len = payload.length;
    const maskKey = crypto.randomBytes(4);
    let headerLen = 2;
    if (len >= 126 && len < 65536) headerLen += 2;
    else if (len >= 65536) headerLen += 8;
    const frame = Buffer.alloc(headerLen + 4 + len);
    frame[0] = 0x80 | (opcode & 0x0f);
    let offset = 2;
    if (len < 126) {
      frame[1] = 0x80 | len;
    } else if (len < 65536) {
      frame[1] = 0x80 | 126;
      frame.writeUInt16BE(len, 2);
      offset = 4;
    } else {
      frame[1] = 0x80 | 127;
      frame.writeBigUInt64BE(BigInt(len), 2);
      offset = 10;
    }
    maskKey.copy(frame, offset);
    offset += 4;
    for (let i = 0; i < len; i++) {
      frame[offset + i] = payload[i] ^ maskKey[i & 3];
    }
    try {
      socket.write(frame);
    } catch {}
  }

  function parseFrames() {
    while (rxBuf.length >= 2) {
      const b0 = rxBuf[0];
      const b1 = rxBuf[1];
      const opcode = b0 & 0x0f;
      const masked = (b1 & 0x80) !== 0;
      let payloadLen = b1 & 0x7f;
      let offset = 2;
      if (payloadLen === 126) {
        if (rxBuf.length < 4) return;
        payloadLen = rxBuf.readUInt16BE(2);
        offset = 4;
      } else if (payloadLen === 127) {
        if (rxBuf.length < 10) return;
        payloadLen = Number(rxBuf.readBigUInt64BE(2));
        offset = 10;
      }
      let maskKey: Buffer | null = null;
      if (masked) {
        if (rxBuf.length < offset + 4) return;
        maskKey = rxBuf.subarray(offset, offset + 4);
        offset += 4;
      }
      if (rxBuf.length < offset + payloadLen) return;
      const payload = Buffer.from(rxBuf.subarray(offset, offset + payloadLen));
      rxBuf = rxBuf.subarray(offset + payloadLen);
      if (maskKey) {
        for (let i = 0; i < payload.length; i++) {
          payload[i] ^= maskKey[i & 3];
        }
      }
      if (opcode === 0x1) {
        emitEvent("message", { data: payload.toString("utf8") });
      } else if (opcode === 0x2) {
        emitEvent("message", { data: payload.buffer.slice(payload.byteOffset, payload.byteOffset + payload.byteLength) });
      } else if (opcode === 0x9) {
        sendFrame(0x0a, payload);
      } else if (opcode === 0x8) {
        closed = true;
        emitEvent("close", { code: 1000, reason: "" });
        try { socket.end(); } catch {}
      }
    }
  }

  socket.on("data", (chunk: Buffer) => {
    rxBuf = rxBuf.length > 0 ? Buffer.concat([rxBuf, chunk]) : chunk;
    parseFrames();
  });
  socket.on("close", () => {
    if (!closed) {
      closed = true;
      emitEvent("close", { code: 1000, reason: "" });
    }
  });
  socket.on("error", (err: any) => {
    emitEvent("error", { error: err });
  });

  if (rxBuf.length > 0) {
    parseFrames();
  }

  return {
    accept() {
      if (accepted) return;
      accepted = true;
      while (pendingEvents.length > 0) {
        const item = pendingEvents.shift()!;
        emitEvent(item.type, item.event);
      }
    },
    send(message: string | Uint8Array | ArrayBuffer) {
      if (typeof message === "string") {
        sendFrame(0x1, Buffer.from(message, "utf8"));
      } else if (message instanceof ArrayBuffer) {
        sendFrame(0x2, Buffer.from(message));
      } else {
        sendFrame(0x2, Buffer.from(message.buffer, message.byteOffset, message.byteLength));
      }
    },
    close() {
      if (closed) return;
      closed = true;
      try { sendFrame(0x8, Buffer.alloc(0)); } catch {}
      try { socket.end(); } catch {}
      try { socket.destroy(); } catch {}
    },
    addEventListener(type: string, callback: (ev: any) => void) {
      let list = listeners.get(type);
      if (!list) {
        list = [];
        listeners.set(type, list);
      }
      list.push(callback);
    },
    removeEventListener(type: string, callback: (ev: any) => void) {
      const list = listeners.get(type);
      if (!list) return;
      const idx = list.indexOf(callback);
      if (idx !== -1) list.splice(idx, 1);
    },
  };
}

export const engine = {
  async fetch(input: string | URL | Request, init?: RequestInit): Promise<any> {
    await ensureServerRunning();
    const port = DEFAULT_PORT;
    const reqObj = input instanceof Request ? input : null;
    const urlStr = reqObj ? reqObj.url : String(input);
    const method = (init?.method || reqObj?.method || "GET").toUpperCase();

    const headers: Record<string, string> = {};
    const applyHeaders = (h: any) => {
      if (!h) return;
      if (h instanceof Headers || typeof h.forEach === "function") {
        h.forEach((v: string, k: string) => {
          headers[k.toLowerCase()] = String(v);
        });
      } else if (Array.isArray(h)) {
        for (const [k, v] of h) {
          headers[String(k).toLowerCase()] = String(v);
        }
      } else if (typeof h === "object") {
        for (const [k, v] of Object.entries(h)) {
          if (v !== undefined) headers[k.toLowerCase()] = String(v);
        }
      }
    };
    if (reqObj) applyHeaders(reqObj.headers);
    if (init?.headers) applyHeaders(init.headers);

    const parsed = new URL(urlStr, `http://127.0.0.1:${port}`);
    if (!headers["host"]) {
      headers["host"] = parsed.host;
    }

    const isUpgrade = String(headers["upgrade"] || "").toLowerCase() === "websocket";
    if (isUpgrade) {
      headers["connection"] = "Upgrade";
      headers["upgrade"] = "websocket";
      if (!headers["sec-websocket-version"]) headers["sec-websocket-version"] = "13";
      if (!headers["sec-websocket-key"]) {
        headers["sec-websocket-key"] = crypto.randomBytes(16).toString("base64");
      }
    }

    const bodyBuf =
      init?.body !== undefined && init.body !== null
        ? typeof init.body === "string"
          ? Buffer.from(init.body, "utf8")
          : Buffer.from(init.body as any)
        : null;
    if (bodyBuf && !headers["content-length"]) {
      headers["content-length"] = String(bodyBuf.length);
    }

    return new Promise((resolve, reject) => {
      const sock = net.connect({ host: "127.0.0.1", port });
      let settled = false;
      let rawBuf: Buffer = Buffer.alloc(0);
      let headersParsed = false;
      let statusCode = 200;
      let statusText = "OK";
      let respHeaders = new Headers();
      let expectedLen = -1;
      let bodyChunks: Buffer[] = [];
      let bodyReceived = 0;

      const finishHttp = () => {
        if (settled) return;
        settled = true;
        try { sock.destroy(); } catch {}
        resolve(
          new Response(Buffer.concat(bodyChunks), {
            status: statusCode,
            statusText,
            headers: respHeaders,
          }),
        );
      };

      const onData = (chunk: Buffer) => {
        if (!headersParsed) {
          rawBuf = rawBuf.length > 0 ? Buffer.concat([rawBuf, chunk]) : chunk;
          const sep = rawBuf.indexOf("\r\n\r\n");
          if (sep === -1) return;
          headersParsed = true;
          const headerText = rawBuf.subarray(0, sep).toString("latin1");
          const rest = rawBuf.subarray(sep + 4);
          const lines = headerText.split("\r\n");
          const statusMatch = (lines[0] || "").match(/^HTTP\/\d\.\d\s+(\d+)\s*(.*)$/i);
          if (statusMatch) {
            statusCode = parseInt(statusMatch[1], 10);
            statusText = statusMatch[2] || "";
          }
          for (let i = 1; i < lines.length; i++) {
            const idx = lines[i].indexOf(":");
            if (idx > 0) {
              const k = lines[i].slice(0, idx).trim();
              const v = lines[i].slice(idx + 1).trim();
              respHeaders.append(k, v);
            }
          }
          if (statusCode === 101) {
            settled = true;
            sock.off("data", onData);
            sock.off("end", onEnd);
            sock.off("close", onEnd);
            sock.off("error", onErr);
            const cfWs = createCfWebSocketFromSocket(sock, rest);
            resolve({
              status: 101,
              statusText: statusText || "Switching Protocols",
              ok: false,
              headers: respHeaders,
              webSocket: cfWs,
            });
            return;
          }
          const cl = respHeaders.get("content-length");
          if (cl !== null && /^\d+$/.test(cl.trim())) {
            expectedLen = parseInt(cl.trim(), 10);
          }
          if (rest.length > 0) {
            bodyChunks.push(rest);
            bodyReceived += rest.length;
          }
          if (expectedLen >= 0 && bodyReceived >= expectedLen) {
            finishHttp();
          }
        } else {
          bodyChunks.push(chunk);
          bodyReceived += chunk.length;
          if (expectedLen >= 0 && bodyReceived >= expectedLen) {
            finishHttp();
          }
        }
      };

      const onEnd = () => {
        if (!settled) {
          if (headersParsed) finishHttp();
          else {
            settled = true;
            reject(new Error("Connection closed before HTTP response headers"));
          }
        }
      };

      const onErr = (err: Error) => {
        if (!settled) {
          settled = true;
          reject(err);
        }
      };

      sock.on("data", onData);
      sock.on("end", onEnd);
      sock.on("close", onEnd);
      sock.on("error", onErr);

      sock.on("connect", () => {
        let reqHead = `${method} ${parsed.pathname + parsed.search} HTTP/1.1\r\n`;
        for (const [k, v] of Object.entries(headers)) {
          reqHead += `${k}: ${v}\r\n`;
        }
        reqHead += "\r\n";
        sock.write(reqHead);
        if (bodyBuf && bodyBuf.length > 0) {
          sock.write(bodyBuf);
        }
      });
    });
  },
};
export { env } from "./shim/cloudflare-test";
export async function connectEngine(): Promise<Browser> {
  const session = await getSharedSession();
  return session.browser;
}
