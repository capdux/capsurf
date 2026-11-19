// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
import { expect, test } from "./harness/index.cjs";
import { setContent } from "./set-content";
test("matches/closest on a dropped node do not throw for a valid selector", async ({ page }) => {
    await setContent(page, '<div id="host"><a id="link" href="/x">x</a></div>');
    const report = await page.evaluate(() => {
        const link = document.getElementById("link");
        const host = document.getElementById("host");
        const before = link.matches("a");
        host.innerHTML = "<span>replaced</span>";
        const out = { before };
        try {
            out.matchesTag = link.matches("actions-caches-filter");
            out.closest = link.closest("div");
        }
        catch (e) {
            out.threw = String(e);
        }
        try {
            link.matches("");
            out.invalidThrew = false;
        }
        catch (e) {
            out.invalidThrew = e.name === "SyntaxError";
        }
        return out;
    });
    expect(report).toEqual({
        before: true,
        matchesTag: false,
        closest: null,
        invalidThrew: true,
    });
});
test("querySelectorAll on a MutationObserver's removed node does not throw", async ({ page }) => {
    await setContent(page, '<div id="host"><a id="link" href="/x"><b>x</b></a></div>');
    const report = await page.evaluate(async () => {
        const host = document.getElementById("host");
        const out = {};
        const done = new Promise((resolve) => {
            new MutationObserver((records, mo) => {
                mo.disconnect();
                for (const r of records) {
                    for (const n of r.removedNodes) {
                        if (n.nodeType !== 1)
                            continue;
                        try {
                            out.count = n.querySelectorAll("*").length;
                            out.selfMatch = n.querySelectorAll("a").length;
                        }
                        catch (e) {
                            out.threw = String(e);
                        }
                        try {
                            n.querySelectorAll("");
                            out.invalidThrew = false;
                        }
                        catch (e) {
                            out.invalidThrew = e.name === "SyntaxError";
                        }
                    }
                }
                resolve();
            }).observe(host, { childList: true });
        });
        host.innerHTML = "<span>replaced</span>";
        await done;
        return out;
    });
    expect(report.threw).toBeUndefined();
    expect(typeof report.count).toBe("number");
    expect(report.invalidThrew).toBe(true);
});
