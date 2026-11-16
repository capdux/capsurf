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
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      throw new Error(`WebSocket is not open (readyState=${this.ws?.readyState})`);
    }
    const id = ++this.nextId;
    return new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject });
      this.ws!.send(JSON.stringify({ id, method, params }));
    });
  }

  on(event: string, handler: (params: any) => void) {
    if (!this.listeners.has(event)) {
      this.listeners.set(event, []);
    }
    this.listeners.get(event)!.push(handler);
  }

  off(event: string, handler: (params: any) => void) {
    const list = this.listeners.get(event);
    if (!list) return;
    const idx = list.indexOf(handler);
    if (idx !== -1) list.splice(idx, 1);
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
  constructor(public cdp: CDPClient) {}

  async goto(url: string, options: { waitUntil?: string; timeout?: number } = {}) {
    return await this.cdp.send("Page.navigate", { url });
  }

  async evaluate(fnOrExpr: any, ...args: any[]): Promise<any> {
    let expression: string;
    if (typeof fnOrExpr === "function") {
      expression = `(${fnOrExpr.toString()})(${args.map((a) => JSON.stringify(a)).join(",")})`;
    } else {
      expression = String(fnOrExpr);
    }
    const r: any = await this.cdp.send("Runtime.evaluate", {
      expression,
      returnByValue: true,
      awaitPromise: true,
    });
    if (r.exceptionDetails) {
      const desc = r.exceptionDetails.exception?.description || "Evaluation failed";
      throw new Error(desc);
    }
    return r.result?.value;
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

  async click(selector: string): Promise<void> {
    await this.evaluate((sel: string) => {
      const el = document.querySelector(sel);
      if (el) (el as any).click?.();
    }, selector);
  }

  async setContent(html: string): Promise<void> {
    await this.goto(`data:text/html;charset=utf-8,${encodeURIComponent(html)}`);
  }

  async exposeFunction(name: string, fn: (...args: any[]) => any): Promise<void> {
    // expose function binding stub
    await this.evaluate(`window[${JSON.stringify(name)}] = function(...args) { return Promise.resolve(); }`);
  }

  async close(): Promise<void> {}
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
  if (!sharedCdp || !sharedPage || !sharedBrowser) {
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

export const expect: any = bunExpect;
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
