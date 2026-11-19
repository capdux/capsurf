// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
import { expect, test } from "./harness/index.cjs";
const SCRIPT = "https://ks-other.invalid/__fixtures/corp-script.js";
async function insert(page, query, crossOrigin) {
    const body = `<script>
    var s = document.createElement("script");
    ${crossOrigin ? 's.crossOrigin = "anonymous";' : ""}
    s.src = ${JSON.stringify(SCRIPT + query)};
    s.onload = function () { window.__outcome = "load:" + (window.__ks_corp_ran || 0); };
    s.onerror = function () { window.__outcome = "error:" + (window.__ks_corp_ran || 0); };
    document.head.appendChild(s);
    window.addEventListener("load", function () {
      setTimeout(function () {
        if (!window.__outcome) window.__outcome = "none:" + (window.__ks_corp_ran || 0);
      }, 1000);
    });
  </script>`;
    const u = new URL("https://ks.invalid/__fixtures/policy-doc.html");
    u.searchParams.set("body", body);
    await page.goto(u.toString(), { waitUntil: "load" });
    await page.waitForFunction(() => typeof globalThis.__outcome === "string", {
        timeout: 20_000,
    });
    return (await page.evaluate(() => globalThis.__outcome));
}
test("a cross-origin script with no CORS headers runs", async ({ page }) => {
    expect(await insert(page, "", false)).toBe("load:1");
}, 60_000);
test("CORP same-origin still blocks it", async ({ page }) => {
    expect(await insert(page, "?policy=same-origin", false)).toMatch(/^(none|error):0$/);
}, 60_000);
test("with crossorigin it needs Access-Control-Allow-Origin", async ({ page }) => {
    expect(await insert(page, "", true)).toMatch(/^(none|error):0$/);
}, 60_000);
