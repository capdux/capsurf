// e2e/anchor-invalid-url-protocol.test.js
// Migrated from e2e/anchor-invalid-url-protocol.e2e.test.ts
// Verifies anchor.protocol and area.protocol return ':' when href is invalid,
// and URL decomposition properties handle invalid URLs gracefully per WHATWG spec.

const { expect, test } = require("./harness");

test("anchor and area protocol return ':' when href is invalid", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify((function () {
      const href = "http://example.com:80:";
      const anchor = document.createElement("a");
      const area = document.createElement("area");
      anchor.setAttribute("href", href);
      area.setAttribute("href", href);
      return {
        protocols: [anchor.protocol, area.protocol],
        origins: [anchor.origin, area.origin],
        hosts: [anchor.host, area.host],
        isAnchorInstance: anchor instanceof HTMLAnchorElement,
        isAreaInstance: area instanceof HTMLAreaElement,
      };
    })())`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const res = JSON.parse(String(r.result?.value));
  expect(res.protocols).toEqual([":", ":"]);
  expect(res.origins).toEqual(["null", "null"]);
  expect(res.hosts).toEqual(["", ""]);
  expect(res.isAnchorInstance).toBe(true);
  expect(res.isAreaInstance).toBe(true);
});
