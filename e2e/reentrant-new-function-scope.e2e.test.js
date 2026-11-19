// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
import { expect, test } from "./harness/index.cjs";
import { setContent } from "./set-content";
const MARKUP = `<script>
  window.mk = function (params, body) { return new Function(params, body); };
</script>`;
const BODY = "return a + '/' + b + '/' + Array.prototype.slice.call(arguments).join(',');";
test("new Function binds its parameters when compiled from a live boa frame", async ({ page }) => {
    await setContent(page, MARKUP);
    const r = await page.evaluate((body) => {
        const mk = window.mk;
        const out = {};
        out.topLevel = mk("a,b", body)(11, 22);
        function outer(q) {
            const z = q;
            return mk("a,b", body)(11, 22) + "|" + z;
        }
        out.inFunction = outer("Q");
        function lvl1(p1) {
            return (function lvl2(p2) {
                return mk("a,b", body)(11, 22) + "|" + p1 + p2;
            })("2");
        }
        out.nested = lvl1("1");
        return out;
    }, BODY);
    expect(r.topLevel).toBe("11/22/11,22");
    expect(r.inFunction).toBe("11/22/11,22|Q");
    expect(r.nested).toBe("11/22/11,22|12");
});
