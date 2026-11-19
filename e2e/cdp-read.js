// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
function asRecord(value, what) {
    if (typeof value !== "object" || value === null) {
        throw new Error(`${what}: expected an object, got ${typeof value}`);
    }
    return value;
}
function numberField(source, key, what) {
    const raw = Reflect.get(source, key);
    if (typeof raw !== "number" || !Number.isFinite(raw)) {
        throw new Error(`${what}.${key}: expected a finite number, got ${String(raw)}`);
    }
    return raw;
}
function booleanField(source, key, what) {
    const raw = Reflect.get(source, key);
    if (typeof raw !== "boolean") {
        throw new Error(`${what}.${key}: expected a boolean, got ${String(raw)}`);
    }
    return raw;
}
export async function readEngineMemory(session) {
    const reply = asRecord(await session.send("Kitesurf.getMemoryStats", {}), "Kitesurf.getMemoryStats");
    const engine = asRecord(Reflect.get(reply, "engine"), "Kitesurf.getMemoryStats.engine");
    const what = "engine";
    return {
        wasmBytes: numberField(engine, "wasmBytes", what),
        wasmInitialBytes: numberField(engine, "wasmInitialBytes", what),
        wasmDeltaBytes: numberField(engine, "wasmDeltaBytes", what),
        retainedBodyBytes: numberField(engine, "retainedBodyBytes", what),
        retainedBodyBytesIsolate: numberField(engine, "retainedBodyBytesIsolate", what),
        retainedRequestCount: numberField(engine, "retainedRequestCount", what),
        retainedImageBytes: numberField(engine, "retainedImageBytes", what),
        retainedImageCount: numberField(engine, "retainedImageCount", what),
        fontAssetCacheBytes: numberField(engine, "fontAssetCacheBytes", what),
        accountedBytes: numberField(engine, "accountedBytes", what),
    };
}
export async function readRendererInfo(session) {
    const reply = asRecord(await session.send("Kitesurf.getRendererInfo"), "Kitesurf.getRendererInfo");
    const what = "rendererInfo";
    return {
        active: booleanField(reply, "active", what),
        framesRendered: numberField(reply, "framesRendered", what),
        relaunchCount: numberField(reply, "relaunchCount", what),
    };
}
export async function readChildFrameCount(session) {
    const reply = asRecord(await session.send("Page.getFrameTree"), "Page.getFrameTree");
    const tree = asRecord(Reflect.get(reply, "frameTree"), "frameTree");
    const children = Reflect.get(tree, "childFrames");
    if (children === undefined)
        return 0;
    if (!Array.isArray(children)) {
        throw new Error("frameTree.childFrames: expected an array");
    }
    return children.length;
}
export async function readFixtureProgress(page, countKey) {
    const raw = asRecord(await page.evaluate(`({done: !!window.__done, count: window.${countKey}|0})`), `window.{__done,${countKey}}`);
    return {
        done: booleanField(raw, "done", "progress"),
        count: numberField(raw, "count", "progress"),
    };
}
