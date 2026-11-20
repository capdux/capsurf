// e2e/shim/cloudflare-test.ts
// Native Bun bridge for Kitesurf cloudflare:test imports
import { ensureServerRunning, releaseSharedCdp, DEFAULT_PORT } from "../harness";
import { decodePng } from "../png";

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

function makeSceneHostWasmFetch() {
  const retained = new Map<number, { width: number; height: number; data: Uint8Array }>();
  const packResult = (w: number, h: number, rgba: Uint8Array): Uint8Array => {
    const out = new Uint8Array(8 + rgba.length);
    const dv = new DataView(out.buffer);
    dv.setUint32(0, w, true);
    dv.setUint32(4, h, true);
    out.set(rgba, 8);
    return out;
  };
  return {
    async fetch(url: string, init: any = {}) {
      const u = new URL(url);
      const op = Number(u.pathname.slice(1));
      const rawBody = init?.body ? new Uint8Array(init.body) : new Uint8Array(0);
      const view = new DataView(rawBody.buffer, rawBody.byteOffset, rawBody.byteLength);
      if (op === 1) {
        return new Response(new Uint8Array(0));
      }
      if (op === 15) {
        if (rawBody.length < 8) return new Response(new Uint8Array(0));
        const seq = view.getUint32(0, true);
        if (seq === 0xffffffff) {
          retained.clear();
        }
        const decoded = await decodePng(rawBody.subarray(8));
        retained.set(seq, decoded);
        if (retained.size > 16) {
          const oldest = retained.keys().next().value;
          if (oldest !== undefined) retained.delete(oldest);
        }
        return new Response(packResult(decoded.width, decoded.height, decoded.data));
      }
      if (op === 16) {
        if (rawBody.length < 20) return new Response(new Uint8Array(0));
        const seq = view.getUint32(0, true);
        const baseSeq = view.getUint32(4, true);
        const clipX = view.getInt32(8, true);
        const clipY = view.getInt32(12, true);
        const base = retained.get(baseSeq);
        if (!base) {
          return new Response(new Uint8Array(0));
        }
        const patch = await decodePng(rawBody.subarray(20));
        const outData = new Uint8Array(base.data);
        for (let py = 0; py < patch.height; py++) {
          const dy = clipY + py;
          if (dy < 0 || dy >= base.height) continue;
          for (let px = 0; px < patch.width; px++) {
            const dx = clipX + px;
            if (dx < 0 || dx >= base.width) continue;
            const sIdx = (py * patch.width + px) * 4;
            const dIdx = (dy * base.width + dx) * 4;
            outData[dIdx] = patch.data[sIdx]!;
            outData[dIdx + 1] = patch.data[sIdx + 1]!;
            outData[dIdx + 2] = patch.data[sIdx + 2]!;
            outData[dIdx + 3] = patch.data[sIdx + 3]!;
          }
        }
        retained.set(seq, { width: base.width, height: base.height, data: outData });
        if (retained.size > 16) {
          const oldest = retained.keys().next().value;
          if (oldest !== undefined) retained.delete(oldest);
        }
        return new Response(packResult(base.width, base.height, outData));
      }
      return new Response(new Uint8Array(0), { status: 404 });
    },
  };
}

export const env = {
  ENGINE: makeEngineFetch(),
  ENGINE_NO_INSPECTOR: makeEngineFetch({ "X-Inspector-Base": "none" }),
  ENGINE_SAME_ORIGIN_INSPECTOR: makeEngineFetch({ "X-Inspector-Base": "/inspector/inspector.html" }),
  SCENE_HOST_WASM: makeSceneHostWasmFetch(),
};
