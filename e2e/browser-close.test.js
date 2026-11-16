// e2e/browser-close.test.js
// Migrated from vendor/kitesurf/packages/engine/test/e2e/browser-close.e2e.test.ts
// Verifies that Browser.close replies with {"id": 1, "result": {}}, then CapSurf closes the WebSocket with code 1000 and reason "Browser.close".

const { expect, test } = require("./harness");

test("Browser.close replies, then the engine closes the socket", async ({ cdp }) => {
  const ws = cdp.ws;
  const order = [];

  const closed = new Promise((resolve) => {
    ws.addEventListener("close", (ev) => {
      order.push("close");
      resolve({ code: ev.code, reason: ev.reason });
    });
  });

  ws.addEventListener("message", (ev) => {
    const data = typeof ev.data === "string" ? ev.data : new TextDecoder().decode(ev.data);
    const msg = JSON.parse(data);
    if (msg.id === 1) {
      order.push(`reply:${JSON.stringify(msg.result)}`);
    }
  });

  ws.send(JSON.stringify({ id: 1, method: "Browser.close" }));

  let timer;
  const timeoutPromise = new Promise((_, reject) => {
    timer = setTimeout(() => reject(new Error(`socket still open; saw ${order.join(",")}`)), 5000);
  });

  const { code, reason } = await Promise.race([closed, timeoutPromise]);
  clearTimeout(timer);

  expect(order).toEqual(["reply:{}", "close"]);
  expect(code).toBe(1000);
  expect(reason).toBe("Browser.close");
});
