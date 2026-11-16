// e2e/attr-type-url.test.js
// Migrated from e2e/attr-type-url.e2e.test.ts
// Verifies attr() with type(<url>) is attr()-tainted and does not produce a valid url at computed-value time.

const { expect, test } = require("./harness");

test("attr() with type(<url>) does not produce a url", async ({ cdpSession: cdp }) => {
  const html = `<style>
    #t {
      --url: attr(data-u type(<url>));
      --url-fallback: attr(data-u type(<url>), none);
      --number: attr(data-n type(<number>));
    }
  </style><div id=t data-u='url("https://example.test/a.png")' data-n="3"></div>`;

  await cdp.send("Page.navigate", {
    url: `data:text/html;charset=utf-8,${encodeURIComponent(html)}`,
  });

  const r = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify((function () {
      const el = document.getElementById("t");
      if (!el) throw new Error("no #t");
      const cs = getComputedStyle(el);
      return {
        url: cs.getPropertyValue("--url"),
        urlFallback: cs.getPropertyValue("--url-fallback"),
        number: cs.getPropertyValue("--number"),
      };
    })())`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const read = JSON.parse(String(r.result?.value));
  expect(read).toEqual({ url: "", urlFallback: "none", number: "3" });
});
