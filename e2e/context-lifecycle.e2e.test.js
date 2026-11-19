// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
import { expect, test } from "./harness/index.cjs";
const FIXTURE = "https://ks.invalid/__fixtures/lifecycle.html";
test("default world is id 1 every generation, cleared precedes created", async ({ page, cdpSession: cdp, }) => {
    const log = [];
    cdp.on("Runtime.executionContextCreated", (e) => {
        const c = e.context;
        if (c.auxData?.isDefault)
            log.push({ kind: "created", id: c.id });
    });
    cdp.on("Runtime.executionContextsCleared", () => {
        log.push({ kind: "cleared" });
    });
    await page.goto(FIXTURE, { waitUntil: "domcontentloaded" });
    await page.goto(FIXTURE + "?second", { waitUntil: "domcontentloaded" });
    const created = log.filter((l) => l.kind === "created");
    expect(created.length).toBeGreaterThanOrEqual(2);
    for (const c of created)
        expect(c.id).toBe(1);
    const second = created[created.length - 1];
    const secondIdx = log.lastIndexOf(second);
    const clearedBefore = log.slice(0, secondIdx).some((l) => l.kind === "cleared");
    expect(clearedBefore).toBe(true);
}, 120_000);
test("retired world contextId evaluate fails with the Chrome error", async ({ page, cdpSession: cdp, }) => {
    await page.goto(FIXTURE, { waitUntil: "domcontentloaded" });
    const { frameTree } = await cdp.send("Page.getFrameTree");
    const frameId = frameTree.frame.id;
    const world = (await cdp.send("Page.createIsolatedWorld", {
        frameId,
        worldName: "ks_stale_probe",
    }));
    const staleId = world.executionContextId;
    expect(staleId).toBeGreaterThan(2);
    const ok = (await cdp.send("Runtime.evaluate", {
        expression: "1+1",
        contextId: staleId,
        returnByValue: true,
    }));
    expect(ok.result.value).toBe(2);
    await page.goto(FIXTURE + "?second", { waitUntil: "domcontentloaded" });
    await expect(cdp.send("Runtime.evaluate", { expression: "1+1", contextId: staleId })).rejects.toThrow(/Cannot find context with specified id/);
    await expect(cdp.send("Runtime.callFunctionOn", {
        functionDeclaration: "() => 1",
        executionContextId: staleId,
    })).rejects.toThrow(/Cannot find context with specified id/);
}, 120_000);
test("isolated worlds: per-name ids, dedupe, fresh ids per navigate", async ({ page, cdpSession: cdp, }) => {
    const isoCreated = [];
    cdp.on("Runtime.executionContextCreated", (e) => {
        const c = e.context;
        if (c.auxData?.type === "isolated") {
            isoCreated.push({ id: c.id, name: c.name });
        }
    });
    await page.goto(FIXTURE, { waitUntil: "domcontentloaded" });
    const { frameTree } = await cdp.send("Page.getFrameTree");
    const frameId = frameTree.frame.id;
    const w1 = (await cdp.send("Page.createIsolatedWorld", {
        frameId,
        worldName: "ks_world_a",
    }));
    const w1again = (await cdp.send("Page.createIsolatedWorld", {
        frameId,
        worldName: "ks_world_a",
    }));
    expect(w1again.executionContextId).toBe(w1.executionContextId);
    const w2 = (await cdp.send("Page.createIsolatedWorld", {
        frameId,
        worldName: "ks_world_b",
    }));
    expect(w2.executionContextId).not.toBe(w1.executionContextId);
    await page.goto(FIXTURE + "?second", { waitUntil: "domcontentloaded" });
    const renounced = isoCreated.filter((c) => c.name === "ks_world_a" || c.name === "ks_world_b");
    const latestA = renounced.filter((c) => c.name === "ks_world_a").pop();
    const latestB = renounced.filter((c) => c.name === "ks_world_b").pop();
    expect(latestA.id).toBeGreaterThan(w2.executionContextId);
    expect(latestB.id).toBeGreaterThan(w2.executionContextId);
    expect(latestA.id).not.toBe(latestB.id);
}, 120_000);
