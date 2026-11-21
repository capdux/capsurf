// e2e/shim/cloudflare-test.ts
// Native Bun bridge for Kitesurf cloudflare:test imports
import { ensureServerRunning, releaseSharedCdp, DEFAULT_PORT } from "../harness";
class WorkerdWebSocketAdapter {
    ws;
    constructor(ws) {
        this.ws = ws;
    }
    accept() { }
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
            }
            else if (event === "close") {
                handler({ code: ev.code, reason: ev.reason });
            }
            else {
                handler(ev);
            }
        });
    }
    removeEventListener(event, handler) {
        this.ws.removeEventListener(event, handler);
    }
}
export const env = {
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
        },
    },
};
