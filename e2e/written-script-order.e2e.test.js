// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
import { expect, test } from "./harness/index.cjs";
const ORIGIN = "http://127.0.0.1:8787";
const written = (query) => `<script src="/__fixtures/written-script.js?${query}"><\\/script>`;
// The page records the order at DCL. Reading it later would pass even if the
// scripts ran after DCL.
async function orderAtDcl(page, writes) {
    const u = new URL(`${ORIGIN}/__fixtures/policy-doc.html`);
    u.searchParams.set("body", `<script>
  globalThis.__written = [];
  document.addEventListener("DOMContentLoaded", function () {
    globalThis.__atDcl = globalThis.__written.join(",");
  });
  document.write('${writes}');
</script>`);
    await page.goto(u.toString(), { waitUntil: "domcontentloaded", timeout: 30_000 });
    const read = () => page.evaluate(() => {
        const atDcl = Reflect.get(globalThis, "__atDcl");
        const all = Reflect.get(globalThis, "__written");
        return {
            atDcl: typeof atDcl === "string" ? atDcl : "",
            all: Array.isArray(all) ? all.join(",") : "",
        };
    });
    await expect.poll(async () => (await read()).atDcl, { timeout: 30_000 }).not.toBe("");
    return read();
}
test("a slow written script still runs before the one written after it", async ({ page }) => {
    const order = await orderAtDcl(page, written("tag=a&ms=800") + written("tag=b"));
    expect(order).toEqual({ atDcl: "a,b", all: "a,b" });
});
test("an inline written script waits for the external one written before it", async ({ page }) => {
    const inline = `<script>globalThis.__written.push("inline")<\\/script>`;
    const order = await orderAtDcl(page, written("tag=a&ms=500") + inline);
    expect(order).toEqual({ atDcl: "a,inline", all: "a,inline" });
});
test("an inline script that a written script writes runs inside its document.write", async ({ page, }) => {
    const order = await orderAtDcl(page, written("tag=a&inline=i"));
    expect(order).toEqual({ atDcl: "a,i,a-after", all: "a,i,a-after" });
});
test("an inline script waits for an external script its writer wrote first", async ({ page }) => {
    const order = await orderAtDcl(page, written("tag=a&next=b&inline=i"));
    expect(order).toEqual({ atDcl: "a,a-after,b,i", all: "a,a-after,b,i" });
});
test("a script a written script writes runs before the writer's later siblings", async ({ page, }) => {
    const order = await orderAtDcl(page, written("tag=a&next=b") + written("tag=c"));
    expect(order).toEqual({ atDcl: "a,b,c", all: "a,b,c" });
});
test("an inline script a written script writes does not wait for its siblings", async ({ page, }) => {
    const order = await orderAtDcl(page, written("tag=a&inline=i") + written("tag=c"));
    expect(order).toEqual({ atDcl: "a,i,a-after,c", all: "a,i,a-after,c" });
});
test("scripts a written script writes run before DOMContentLoaded", async ({ page }) => {
    const order = await orderAtDcl(page, written("tag=a&next=b"));
    expect(order).toEqual({ atDcl: "a,b", all: "a,b" });
});
