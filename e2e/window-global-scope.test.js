// e2e/window-global-scope.test.js
// Migrated from vendor/kitesurf/packages/engine/test/e2e/window-global-scope.e2e.test.ts
// Verifies CapSurf window/global scope baseline types: window, globalThis, document,
// setTimeout, clearTimeout, requestAnimationFrame, cancelAnimationFrame, unforgeables, and bare global lookups.

const { expect, test } = require("./harness");

// Test 1: 验证 window, globalThis, document, setTimeout, clearTimeout, requestAnimationFrame 等基础全局变量类型正确
test("window and global scope baseline types and timers are correct", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify({
      winType: typeof window,
      globalThisType: typeof globalThis,
      docType: typeof document,
      setTimeoutType: typeof setTimeout,
      clearTimeoutType: typeof clearTimeout,
      rafType: typeof requestAnimationFrame,
      cafType: typeof cancelAnimationFrame,
      perfType: typeof performance,
      perfNowType: typeof performance.now,
      screenType: typeof screen,
      locationType: typeof location,
      navigatorType: typeof navigator,
      localStorageType: typeof localStorage,
      sessionStorageType: typeof sessionStorage,
      isWindowGlobal: window === globalThis,
      isWindowDocSame: window.document === document,
      isWindowWindowSame: window.window === window,
      isGlobalThisDocSame: globalThis.document === document,
    })`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const res = JSON.parse(String(r.result?.value));

  expect(res.winType).toBe("object");
  expect(res.globalThisType).toBe("object");
  expect(res.docType).toBe("object");
  expect(res.setTimeoutType).toBe("function");
  expect(res.clearTimeoutType).toBe("function");
  expect(res.rafType).toBe("function");
  expect(res.cafType).toBe("function");
  expect(res.perfType).toBe("object");
  expect(res.perfNowType).toBe("function");
  expect(res.screenType).toBe("object");
  expect(res.locationType).toBe("object");
  expect(res.navigatorType).toBe("object");
  expect(res.localStorageType).toBe("object");
  expect(res.sessionStorageType).toBe("object");
  expect(res.isWindowGlobal).toBe(true);
  expect(res.isWindowDocSame).toBe(true);
  expect(res.isWindowWindowSame).toBe(true);
  expect(res.isGlobalThisDocSame).toBe(true);
});

// Test 2: 验证 DOM, Event 和 Observer 构造器与原型在全局作用域就绪
test("DOM, Event and Observer constructors and prototypes exist on global scope", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify({
      hasEvent: typeof Event === "function",
      hasCustomEvent: typeof CustomEvent === "function",
      hasEventTarget: typeof EventTarget === "function",
      hasNode: typeof Node === "function",
      hasElement: typeof Element === "object" || typeof Element === "function",
      hasDocument: typeof Document === "object" || typeof Document === "function",
      hasElementProto: typeof Element.prototype === "object",
      hasDocumentProto: typeof Document.prototype === "object",
      hasMutationObserver: typeof MutationObserver === "function",
      hasRange: typeof Range === "function",
      hasSelection: typeof Selection === "function",
    })`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const res = JSON.parse(String(r.result?.value));

  expect(res.hasEvent).toBe(true);
  expect(res.hasCustomEvent).toBe(true);
  expect(res.hasEventTarget).toBe(true);
  expect(res.hasNode).toBe(true);
  expect(res.hasElement).toBe(true);
  expect(res.hasDocument).toBe(true);
  expect(res.hasElementProto).toBe(true);
  expect(res.hasDocumentProto).toBe(true);
  expect(res.hasMutationObserver).toBe(true);
  expect(res.hasRange).toBe(true);
  expect(res.hasSelection).toBe(true);
});

// Test 3: 验证 window 属性读写与全局变量作用域互通 (window-global-scope: bare scope access)
test("properties and accessors defined on window are accessible as bare globals", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `(function() {
      window.__r = {};
      var backing = { n: 1 };
      Object.defineProperty(window, "liveHandle", {
        configurable: true,
        get: function () { return backing; },
      });
      window.__r.viaWindow = window.liveHandle.n;
      try { window.__r.bare = liveHandle.n; } catch (e) { window.__r.bare = "throw: " + e.message; }

      backing = { n: 2 };
      try { window.__r.bareAfterSwap = liveHandle.n; } catch (e) { window.__r.bareAfterSwap = "throw: " + e.message; }

      return JSON.stringify(window.__r);
    })()`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const res = JSON.parse(String(r.result?.value));
  expect(res.viaWindow).toBe(1);
  expect(res.bare).toBe(1);
  expect(res.bareAfterSwap).toBe(2);
});

// Test 4: 验证不可伪造属性与全局作用域保护 (window-global-scope: unforgeable window properties)
test("assigning an unforgeable window property preserves window and document references", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `(function() {
      var r = {};
      var origDoc = window.document;
      var origWin = window.window;

      try { window.document = 5; r.sloppyDoc = "no-op"; }
      catch (e) { r.sloppyDoc = "throw: " + e.message; }

      try { window.window = 5; r.sloppyWin = "no-op"; }
      catch (e) { r.sloppyWin = "throw: " + e.message; }

      r.documentIntact = window.document === origDoc && !!(window.document && window.document.getElementById);
      r.windowIntact = window.window === origWin && window.window === window;

      return JSON.stringify(r);
    })()`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const res = JSON.parse(String(r.result?.value));
  expect(res.documentIntact).toBe(true);
  expect(res.windowIntact).toBe(true);
});
