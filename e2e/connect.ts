// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0

// Adapt to WebSocket with a plain CDP handshake
import type { Browser } from "./harness";

export type WorkerdWebSocket = any;

export function upgradedSocket(resp: Response): any {
  return (resp as Response & { webSocket?: any }).webSocket ?? null;
}

function wrapWebSocket(ws: any) {
  const transport: {
    send(m: string): void;
    close(): void;
    onmessage?: (m: string) => void;
    onclose?: () => void;
  } = {
    send: (m) => ws.send(m),
    close: () => ws.close(),
  };
  ws.addEventListener("message", (ev: { data: string | ArrayBuffer }) => {
    const data = typeof ev.data === "string" ? ev.data : new TextDecoder().decode(ev.data);
    transport.onmessage?.(data);
  });
  ws.addEventListener("close", () => transport.onclose?.());
  return transport;
}

export async function connectEngine(
  fetcher: { fetch: typeof fetch },
  headers: Record<string, string> = {},
  query = "",
): Promise<Browser> {
  const resp = await fetcher.fetch(`https://ks.invalid/devtools/browser${query}`, {
    headers: { Upgrade: "websocket", ...headers },
  });
  const ws = upgradedSocket(resp);
  if (!ws) throw new Error(`no webSocket on /devtools/browser (status ${resp.status})`);
  ws.accept();
  try {
    const puppeteer: any = await import("@cloudflare/puppeteer");
    return puppeteer.connect({ transport: wrapWebSocket(ws) as never });
  } catch {
    return { ws, transport: wrapWebSocket(ws) } as any;
  }
}


export async function targetIdForSession(session: any): Promise<any> {
  if (session?._target?._targetId) return session._target._targetId;
  if (session?.targetId) return session.targetId;
  try {
    const info = await session.send("Target.getTargetInfo");
    if (info?.targetInfo?.targetId) return info.targetInfo.targetId;
  } catch {}
  return "capsurf-target-001";
}
