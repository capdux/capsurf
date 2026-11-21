// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
export function upgradedSocket(resp) {
    return resp.webSocket ?? null;
}
function wrapWebSocket(ws) {
    const transport = {
        send: (m) => ws.send(m),
        close: () => ws.close(),
    };
    ws.addEventListener("message", (ev) => {
        const data = typeof ev.data === "string" ? ev.data : new TextDecoder().decode(ev.data);
        transport.onmessage?.(data);
    });
    ws.addEventListener("close", () => transport.onclose?.());
    return transport;
}
export async function connectEngine(fetcher, headers = {}, query = "") {
    const resp = await fetcher.fetch(`https://ks.invalid/devtools/browser${query}`, {
        headers: { Upgrade: "websocket", ...headers },
    });
    const ws = upgradedSocket(resp);
    if (!ws)
        throw new Error(`no webSocket on /devtools/browser (status ${resp.status})`);
    ws.accept();
    try {
        const puppeteer = await import("@cloudflare/puppeteer");
        return puppeteer.connect({ transport: wrapWebSocket(ws) });
    }
    catch {
        return { ws, transport: wrapWebSocket(ws) };
    }
}
export async function targetIdForSession(session) {
    if (session?._target?._targetId)
        return session._target._targetId;
    if (session?.targetId)
        return session.targetId;
    try {
        const info = await session.send("Target.getTargetInfo");
        if (info?.targetInfo?.targetId)
            return info.targetInfo.targetId;
    }
    catch { }
    return "capsurf-target-001";
}
