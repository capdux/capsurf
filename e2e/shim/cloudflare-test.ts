// e2e/shim/cloudflare-test.ts
// Native Bun bridge for Kitesurf cloudflare:test imports
import { ensureServerRunning, releaseSharedCdp, DEFAULT_PORT } from "../harness";

class WorkerdWebSocketAdapter {
  constructor(public ws: WebSocket) {}
  accept() {}
  send(data: any) {
    this.ws.send(data);
  }
  close(code?: number, reason?: string) {
    this.ws.close(code, reason);
  }
  addEventListener(event: string, handler: any) {
    this.ws.addEventListener(event, (ev: any) => {
      if (event === "message") {
        handler({ data: ev.data });
      } else if (event === "close") {
        handler({ code: ev.code, reason: ev.reason });
      } else {
        handler(ev);
      }
    });
  }
  removeEventListener(event: string, handler: any) {
    this.ws.removeEventListener(event, handler);
  }
}

function makeEngineFetch(extraHeaders: Record<string, string> = {}) {
  return {
    async fetch(url: string, init: any = {}) {
      await ensureServerRunning();
      const u = new URL(url);
      const rawHeaders = init?.headers;
      const headers = new Headers(rawHeaders instanceof Headers ? rawHeaders : rawHeaders || {});
      const upgrade = headers.get("upgrade");
      if (upgrade && upgrade.toLowerCase() === "websocket") {
        await releaseSharedCdp();
        await new Promise((r) => setTimeout(r, 20));
        const wsUrl = `ws://127.0.0.1:${DEFAULT_PORT}${u.pathname}${u.search}`;
        const ws = new WebSocket(wsUrl);
        await new Promise((resolve, reject) => {
          ws.onopen = () => resolve(true);
          ws.onerror = (err) => reject(err);
        });
        const adapter = new WorkerdWebSocketAdapter(ws);
        const resp: any = new Response(null, { status: 101, statusText: "Switching Protocols" });
        resp.webSocket = adapter;
        return resp;
      }
      if (!headers.has("x-forwarded-host") && u.host) {
        headers.set("X-Forwarded-Host", u.host);
      }
      if (!headers.has("x-forwarded-proto") && u.protocol) {
        headers.set("X-Forwarded-Proto", u.protocol.replace(":", ""));
      }
      for (const [k, v] of Object.entries(extraHeaders)) {
        if (!headers.has(k)) {
          headers.set(k, v);
        }
      }
      const httpUrl = `http://127.0.0.1:${DEFAULT_PORT}${u.pathname}${u.search}`;
      return fetch(httpUrl, { ...init, headers });
    },
  };
}

export const env = {
  ENGINE: makeEngineFetch(),
  ENGINE_NO_INSPECTOR: makeEngineFetch({ "X-Inspector-Base": "none" }),
  ENGINE_SAME_ORIGIN_INSPECTOR: makeEngineFetch({ "X-Inspector-Base": "/inspector/inspector.html" }),
};
