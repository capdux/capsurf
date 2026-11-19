// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
import { expect, test } from "./harness/index.cjs";
const BIG_BYTES = 900_000;
const SMALL_BYTES = 4_096;
const url = (bytes, seed) => `https://ks.invalid/__fixtures/chatty-json?bytes=${bytes}&seed=${seed}`;
test("an oversize text body streams through whole; a small one is still captured", async ({ page, cdpSession, }) => {
    const seen = new Map();
    cdpSession.on("Network.responseReceived", (p) => {
        const ev = p;
        if (ev.requestId && ev.response?.url)
            seen.set(ev.response.url, ev.requestId);
    });
    await cdpSession.send("Network.enable");
    await page.goto("https://ks.invalid/__fixtures/lifecycle.html", {
        waitUntil: "load",
        timeout: 60000,
    });
    const fetchBody = async (target) => (await page.evaluate(async (u) => {
        const r = await fetch(u);
        const text = await r.text();
        return {
            status: r.status,
            length: text.length,
            head: text.slice(0, 12),
            tail: text.slice(-2),
            parsedSeed: JSON.parse(text).seed,
        };
    }, target));
    const big = await fetchBody(url(BIG_BYTES, 7));
    expect(big.status).toBe(200);
    expect(big.length, "the page must get every byte, not the 256 KB we were willing to retain").toBe(BIG_BYTES);
    expect(big.head).toBe('{"seed":7,"f');
    expect(big.tail).toBe('"}');
    expect(big.parsedSeed).toBe(7);
    const small = await fetchBody(url(SMALL_BYTES, 8));
    expect(small.length).toBe(SMALL_BYTES);
    expect(small.parsedSeed).toBe(8);
    const bodyOf = async (target) => {
        const requestId = seen.get(target);
        expect(requestId, `Network.responseReceived seen for ${target}`).toBeTruthy();
        const reply = (await cdpSession.send("Network.getResponseBody", {
            requestId: requestId,
        }));
        return reply.body ?? "";
    };
    expect(await bodyOf(url(SMALL_BYTES, 8)), "a body under the cap stays readable from the Network panel").toHaveLength(SMALL_BYTES);
    expect(await bodyOf(url(BIG_BYTES, 7)), "a body past the cap must not be retained").toBe("");
});
