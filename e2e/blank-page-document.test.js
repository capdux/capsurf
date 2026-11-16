// e2e/blank-page-document.test.js
// Migrated from vendor/kitesurf/packages/engine/test/e2e/blank-page-document.e2e.test.ts
// Verifies CapSurf blank page document initialization and CDP protocol communication without explicit navigation.

const { expect, test } = require("./harness");

// Test 1: 新建页面具有 document，无需显式导航（通过 Runtime.evaluate 验证 window、document、document.body 类型）
test("fresh page has a document without navigating", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify({
      win: typeof window,
      doc: typeof document,
      body: typeof document.body,
      title: typeof document.title,
    })`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  expect(JSON.parse(String(r.result?.value))).toEqual({
    win: "object",
    doc: "object",
    body: "object",
    title: "string",
  });
});

// Test 2: 验证页面基础属性与 CDP 通信正常
test("page basic properties and CDP communication work normally", async ({ cdpSession: cdp }) => {
  // 1. Verify CDP Browser domain handshake
  const version = await cdp.send("Browser.getVersion");
  expect(version.product).toContain("CapSurf");
  expect(version.protocolVersion).toBe("1.3");

  // 2. Verify Page & Runtime domains enable cleanly
  const pageEnable = await cdp.send("Page.enable");
  expect(pageEnable).toEqual({});

  const runtimeEnable = await cdp.send("Runtime.enable");
  expect(runtimeEnable).toEqual({});

  // 3. Verify DOM document tree access
  const dom = await cdp.send("DOM.getDocument");
  expect(dom.root).toBeDefined();
  expect(dom.root.nodeType).toBe(9); // DOCUMENT_NODE
  expect(dom.root.nodeName).toBe("#document");

  // 4. Verify basic DOM properties on blank page
  const props = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify({
      nodeType: document.nodeType,
      title: document.title,
      hasCreateElement: typeof document.createElement === "function",
      hasQuerySelector: typeof document.querySelector === "function",
      htmlTag: document.documentElement ? document.documentElement.tagName : null,
      bodyTag: document.body ? document.body.tagName : null,
      isWindowSameAsGlobal: window === globalThis,
    })`,
    returnByValue: true,
  });

  expect(props.exceptionDetails).toBeFalsy();
  const parsed = JSON.parse(String(props.result?.value));
  expect(parsed.nodeType).toBe(9);
  expect(parsed.title).toBe("Blank Page");
  expect(parsed.hasCreateElement).toBe(true);
  expect(parsed.hasQuerySelector).toBe(true);
  expect(parsed.htmlTag).toBe("HTML");
  expect(parsed.bodyTag).toBe("BODY");
  expect(parsed.isWindowSameAsGlobal).toBe(true);
});
