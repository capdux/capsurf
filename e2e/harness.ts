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
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      throw new Error(`WebSocket is not open (readyState=${this.ws?.readyState})`);
    }
    const id = ++this.nextId;
    const res: any = await new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject });
      this.ws!.send(JSON.stringify({ id, method, params }));
    });
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

export class Page {
  private inFlightEvaluates = new Set<{ reject: (err: any) => void }>();
  private _url: string = "about:blank";
  private exposedFunctions = new Map<string, string>();
  public historyEntries: Array<{ id: number; url: string; userTypedURL: string; title: string }> = [
    { id: 1, url: "about:blank", userTypedURL: "about:blank", title: "" }
  ];
  public historyIndex: number = 0;

  constructor(public cdp: CDPClient) {
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
    const res = await this.cdp.send("Page.navigate", navParams);
    try {
      await this.cdp.send("Runtime.evaluate", {
        expression: `(function(){ if (typeof __capsurf_set_document_url === 'function') __capsurf_set_document_url(${JSON.stringify(url)}); })()`,
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

    return res;
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
        const r: any = await this.cdp.send("Runtime.evaluate", {
          expression,
          returnByValue: true,
          awaitPromise: true,
        });

        // Drain same-doc and check cross-doc navigations
        const navCheck: any = await this.cdp.send("Runtime.evaluate", {
          expression: `(function() {
            var same = (globalThis.__capsurf_same_doc_nav && globalThis.__capsurf_same_doc_nav.length > 0)
              ? globalThis.__capsurf_same_doc_nav.splice(0)
              : [];
            var pending = globalThis.__capsurf_pending_navigation || null;
            if (pending) delete globalThis.__capsurf_pending_navigation;
            return { same: same, pending: pending };
          })()`,
          returnByValue: true,
        });

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

  async waitForFunction(fnOrExpr: any, options: { timeout?: number; polling?: number } = {}): Promise<any> {
    const timeout = options.timeout ?? 30000;
    const start = Date.now();
    while (Date.now() - start < timeout) {
      try {
        const val = await this.evaluate(fnOrExpr);
        if (val) return val;
      } catch {}
      await new Promise((r) => setTimeout(r, 50));
    }
    const err: any = new Error(`${timeout}ms exceeded`);
    err.name = "TimeoutError";
    throw err;
  }

  async createCDPSession(): Promise<CDPClient> {
    return this.cdp;
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

  target() {
    return {
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
        }
      }, selector);
    } catch (err: any) {
      if (!String(err?.message || err).includes("Execution context was destroyed")) {
        throw err;
      }
    }
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

  async close(): Promise<void> {}
}

export class ElementHandle {
  constructor(public page: Page, public remoteObject: any, public selector?: string) {}

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

export class Browser {
  constructor(public cdp: CDPClient, public page: Page) {}

  async pages(): Promise<Page[]> {
    return [this.page];
  }

  async newPage(): Promise<Page> {
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
  if (!sharedProc) {
    const port = DEFAULT_PORT;
    const binPath = DEFAULT_BIN;
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
        reject(new Error(`CapSurf process exited prematurely (code=${code}, signal=${signal})`));
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
      if ((name.includes("blank page") || name.includes("fresh page")) && fixtures.page.url() !== "about:blank") {
        await fixtures.page.goto("about:blank");
      }
      const browserFactory = async () => fixtures.browser;
      await fn({ ...fixtures, browserFactory });
    } else {
      await ensureServerRunning();
      await (fn as any)();
    }
  }, timeout);
}

test.concurrent = function (name: string, fn: TestFn, timeout = 120000) {
  test(name, fn, timeout);
};

test.skip = bunTest.skip;
test.only = bunTest.only;
test.todo = bunTest.todo;

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

  return {
    async toBe(expected: any) {
      await pollUntil((val) => expect(val).toBe(expected));
    },
    async toEqual(expected: any) {
      await pollUntil((val) => expect(val).toEqual(expected));
    },
    async toBeTruthy() {
      await pollUntil((val) => expect(val).toBeTruthy());
    },
    async toBeFalsy() {
      await pollUntil((val) => expect(val).toBeFalsy());
    },
  };
};

export const engine = null;
export { env } from "./shim/cloudflare-test";
export async function connectEngine(): Promise<Browser> {
  const session = await getSharedSession();
  return session.browser;
}
