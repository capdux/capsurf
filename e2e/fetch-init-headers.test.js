// e2e/fetch-init-headers.test.js
// Migrated from e2e/fetch-init-headers.e2e.test.ts
// Verifies Request and Response accept frozen init dictionaries and sequence validation follows Web IDL single-read rules.

const { expect, test } = require("./harness");

test("Request and Response accept frozen init dictionaries", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify((function () {
      const capture = (construct) => {
        try {
          return { value: construct(), error: null };
        } catch (error) {
          return { value: null, error: error && error.name };
        }
      };
      const request = capture(() => {
        const init = Object.freeze({
          headers: Object.freeze({ "x-test": "request" }),
        });
        return new Request("https://example.test/", init).headers.get("x-test");
      });
      const response = capture(() => {
        const init = Object.freeze({
          headers: Object.freeze({ "x-test": "response" }),
        });
        return new Response("body", init).headers.get("x-test");
      });
      return { request, response };
    })())`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const results = JSON.parse(String(r.result?.value));
  expect(results).toEqual({
    request: { value: "request", error: null },
    response: { value: "response", error: null },
  });
});

test("Request header sequences follow single-read Web IDL iteration", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify((function () {
      const capture = (construct) => {
        try {
          return { value: construct(), error: null };
        } catch (error) {
          return { value: null, error: error && error.name };
        }
      };
      let nextGetterReads = 0;
      let step = 0;
      const iterator = Object.defineProperty({}, "next", {
        get() {
          nextGetterReads++;
          if (nextGetterReads > 1) throw new Error("next getter read twice");
          return () => (step++ === 0 ? { done: false, value: ["x-test", "yes"] } : { done: true });
        },
      });
      const headers = {
        [Symbol.iterator]() {
          return iterator;
        },
      };
      const iteratorRequest = capture(() =>
        new Request("https://example.test/", { headers: headers }).headers.get("x-test"),
      );
      const arrayLikePairRequest = capture(() =>
        new Request("https://example.test/", {
          headers: [{ 0: "x-test", 1: "yes", length: 2 }],
        }).headers.get("x-test"),
      );
      const primitivePairRequest = capture(() =>
        new Request("https://example.test/", { headers: ["xy"] }).headers.get("x-test"),
      );
      const primitiveHeaderRequests = [1, true, ""].map((h) =>
        capture(() =>
          new Request("https://example.test/", { headers: h }).headers.get("x-test"),
        ),
      );
      return {
        iteratorRequest,
        nextGetterReads,
        arrayLikePairRequest,
        primitivePairRequest,
        primitiveHeaderRequests,
      };
    })())`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const actual = JSON.parse(String(r.result?.value));
  expect(actual).toEqual({
    iteratorRequest: { value: "yes", error: null },
    nextGetterReads: 1,
    arrayLikePairRequest: { value: null, error: "TypeError" },
    primitivePairRequest: { value: null, error: "TypeError" },
    primitiveHeaderRequests: [
      { value: null, error: "TypeError" },
      { value: null, error: "TypeError" },
      { value: null, error: "TypeError" },
    ],
  });
});
