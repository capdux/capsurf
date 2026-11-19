// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
import { env } from "cloudflare:test";
import { upgradedSocket } from "./connect";
import { expect, test } from "./harness/index.cjs";
const FROM = "https://ks.invalid/__fixtures/events.html";
const TO = "https://ks.invalid/__fixtures/keyboard.html";
const PROBE = `JSON.stringify({ doc: typeof document, url: location.href })`;
async function hammerEvaluateDuring(cdp, op) {
    let settled = false;
    const done = op.then(() => (settled = true), () => (settled = true));
    const replies = [];
    const errors = [];
    while (!settled) {
        try {
            const r = (await cdp.send("Runtime.evaluate", {
                expression: PROBE,
                returnByValue: true,
            }));
            if (r.exceptionDetails) {
                errors.push(r.exceptionDetails.exception?.description ?? r.exceptionDetails.text ?? "exception");
            }
            else {
                replies.push(String(r.result?.value));
            }
        }
        catch (e) {
            errors.push(String(e));
        }
    }
    await done;
    return { replies, errors };
}
test("Runtime.evaluate racing a second navigate never sees a document-less isolate", async ({ page, cdpSession: cdp, }) => {
    await page.goto(FROM, { waitUntil: "domcontentloaded" });
    const { replies, errors } = await hammerEvaluateDuring(cdp, page.goto(TO, { waitUntil: "domcontentloaded" }));
    for (const e of errors)
        expect(e).not.toMatch(/is not defined/);
    expect(replies.length).toBeGreaterThan(0);
    for (const r of replies) {
        const parsed = JSON.parse(r);
        expect(parsed.doc).toBe("object");
        expect([FROM, TO]).toContain(parsed.url);
    }
    expect(await page.evaluate(() => location.href)).toBe(TO);
}, 120_000);
test("Runtime.evaluate racing setDocumentContent never sees a document-less isolate", async ({ page, cdpSession: cdp, }) => {
    await page.goto(FROM, { waitUntil: "domcontentloaded" });
    const { replies, errors } = await hammerEvaluateDuring(cdp, page.setContent("<h1 id=swapped>swapped</h1>", {
        waitUntil: "domcontentloaded",
    }));
    for (const e of errors)
        expect(e).not.toMatch(/is not defined/);
    expect(replies.length).toBeGreaterThan(0);
    for (const r of replies) {
        expect(JSON.parse(r).doc).toBe("object");
    }
    expect(await page.$("#swapped")).not.toBeNull();
}, 120_000);
test("createBrowserContext → newPage → evaluate has a document", async ({ browser }) => {
    const context = await browser.createBrowserContext();
    const page = await context.newPage();
    for (let i = 0; i < 5; i++) {
        const r = await page.evaluate("JSON.stringify({ doc: typeof document, body: typeof document.body })");
        expect(JSON.parse(String(r))).toEqual({ doc: "object", body: "object" });
    }
    await context.close();
}, 120_000);
async function rawSession() {
    const resp = await env.ENGINE.fetch("https://ks.invalid/devtools/browser", {
        headers: { Upgrade: "websocket" },
    });
    const ws = upgradedSocket(resp);
    if (!ws)
        throw new Error(`no webSocket (status ${resp.status})`);
    ws.accept();
    const events = [];
    const pending = new Map();
    ws.addEventListener("message", (ev) => {
        const data = typeof ev.data === "string" ? ev.data : new TextDecoder().decode(ev.data);
        const msg = JSON.parse(data);
        if (msg.id !== undefined)
            pending.get(msg.id)?.(msg);
        else
            events.push(msg);
    });
    let nextId = 1;
    const send = (method, params = {}, sessionId) => {
        const id = nextId++;
        const reply = new Promise((r) => pending.set(id, r));
        ws.send(JSON.stringify({ id, method, params, sessionId }));
        return reply;
    };
    const newPage = async () => {
        const created = await send("Target.createTarget", { url: "about:blank" });
        const targetId = created.result.targetId;
        const attached = await send("Target.attachToTarget", { targetId, flatten: true });
        return { targetId, sessionId: attached.result.sessionId };
    };
    return { ws, events, send, newPage };
}
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
test("closeTarget during the navigate window: the waiting eval never answers and leaks no isolate", async () => {
    const { ws, events, send, newPage } = await rawSession();
    try {
        const SLOW = "data:text/html," +
            encodeURIComponent('<script src="https://ks.invalid/__fixtures/slow-script?ms=2000"></script><p>slow</p>');
        for (let i = 0; i < 2; i++) {
            const { targetId, sessionId } = await newPage();
            void send("Page.navigate", { url: SLOW }, sessionId);
            await sleep(300);
            const evalReply = send("Runtime.evaluate", { expression: "location.href", returnByValue: true }, sessionId);
            await sleep(300);
            await send("Target.closeTarget", { targetId });
            const reply = await Promise.race([evalReply, sleep(5000).then(() => null)]);
            if (reply) {
                const value = reply.result?.result?.value;
                expect(value, `eval on a closed target answered ${JSON.stringify(value)}`).toBeUndefined();
            }
        }
        const { sessionId } = await newPage();
        await send("Runtime.enable", {}, sessionId);
        await send("Page.enable", {}, sessionId);
        await send("Page.navigate", { url: "http://localhost:8787/__fixtures/oopif-host.html" }, sessionId);
        const childContext = () => events.some((e) => e.method === "Runtime.executionContextCreated" &&
            e.params?.context?.origin ===
                "http://127.0.0.1:8787");
        await expect.poll(childContext, { timeout: 20_000 }).toBe(true);
    }
    finally {
        ws.close();
    }
}, 120_000);
