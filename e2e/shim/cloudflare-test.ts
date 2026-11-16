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

export const env = {
  ENGINE: {
    async fetch(url: string, init: any = {}) {
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
        const resp: any = new Response(null, { status: 101, statusText: "Switching Protocols" });
        resp.webSocket = adapter;
        return resp;
      }
      const httpUrl = `http://127.0.0.1:${DEFAULT_PORT}${u.pathname}${u.search}`;
      return fetch(httpUrl, init);
    },
  },
};
