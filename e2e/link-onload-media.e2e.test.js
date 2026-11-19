// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
import { expect, test } from "./harness/index.cjs";
test("stylesheet link onload flips media and the sheet applies", async ({ page }) => {
    await page.goto("https://ks.invalid/__fixtures/link-onload-media.html", {
        waitUntil: "load",
    });
    const probe = (await page.evaluate(() => {
        const link = document.querySelector("link[rel~='stylesheet']");
        const cs = getComputedStyle(document.getElementById("hero"));
        return { media: link.media, color: cs.color, fontSize: cs.fontSize };
    }));
    expect(probe.media).toBe("all");
    expect(probe.color).toBe("rgb(184, 63, 69)");
    expect(probe.fontSize).toBe("80px");
}, 120_000);
test("media attr mutation re-scopes an installed sheet both ways", async ({ page }) => {
    await page.goto("https://ks.invalid/__fixtures/link-media.html", {
        waitUntil: "load",
    });
    const probe = (await page.evaluate(() => {
        const el = document.getElementById("masthead");
        const printLink = document.querySelector("link[media='print']");
        const before = getComputedStyle(el).display;
        printLink.media = "screen";
        const after = getComputedStyle(el).display;
        printLink.media = "print";
        const restored = getComputedStyle(el).display;
        return { before, after, restored };
    }));
    expect(probe.before).toBe("block");
    expect(probe.after).toBe("none");
    expect(probe.restored).toBe("block");
}, 120_000);
