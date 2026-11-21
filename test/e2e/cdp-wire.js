// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
function isCfWebSocket(value) {
    return (typeof value === "object" &&
        value !== null &&
        "accept" in value &&
        typeof value.accept === "function" &&
        "send" in value &&
        typeof value.send === "function" &&
        "close" in value &&
        typeof value.close === "function" &&
        "addEventListener" in value &&
        typeof value.addEventListener === "function");
}
export async function openCdpWireSession(engine) {
    const response = await engine.fetch("https://ks.invalid/devtools/browser", {
        headers: { Upgrade: "websocket" },
    });
    const socket = Reflect.get(response, "webSocket");
    if (!isCfWebSocket(socket)) {
        throw new Error(`no webSocket (status ${response.status})`);
    }
    socket.accept();
    const queue = [];
    const waiters = [];
    socket.addEventListener("message", (event) => {
        const text = typeof event.data === "string" ? event.data : new TextDecoder().decode(event.data);
        const message = JSON.parse(text);
        if (typeof message !== "object" || message === null || !("id" in message))
            return;
        const waiter = waiters.shift();
        if (waiter)
            waiter(message);
        else
            queue.push(message);
    });
    const next = () => queue.length > 0
        ? Promise.resolve(queue.shift())
        : new Promise((resolve) => waiters.push(resolve));
    return { ws: socket, next };
}
