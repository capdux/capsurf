// e2e/attributes-live-namednodemap.test.js
// Migrated from e2e/attributes-live-namednodemap.e2e.test.ts
// Verifies el.attributes is a live NamedNodeMap across attribute mutations.

const { expect, test } = require("./harness");

// Test 1: 验证 NamedNodeMap 在 setAttribute, removeAttribute, removeAttributeNode 下的实时联动与排空循环
test("el.attributes is a live NamedNodeMap across mutations", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify((function () {
      const el = document.createElement("div");
      el.id = "t";
      el.className = "c";
      el.setAttribute("data-x", "1");
      const out = {};

      // One map object, read once, observed across every later mutation.
      const attrs = el.attributes;
      out.initial = Array.prototype.map.call(attrs, (a) => a.name);
      out.initialLength = attrs.length;

      el.removeAttribute("id");
      out.afterRemoveLength = attrs.length;
      out.afterRemoveFirst = attrs[0] && attrs[0].name;
      out.afterRemoveGetNamed = attrs.getNamedItem("id");
      out.afterRemoveItem = Array.prototype.map.call(attrs, (a) => a.name);

      el.setAttribute("data-y", "z");
      out.afterAddLength = attrs.length;
      out.afterAddGetNamed = attrs.getNamedItem("data-y") && attrs.getNamedItem("data-y").value;
      out.afterAddKeys = Object.keys(attrs).length;

      // React 19's releaseSingletonInstance drain loop, verbatim in shape.
      let iterations = 0;
      out.drainError = null;
      try {
        while (attrs.length) {
          if (++iterations > 20) break;
          el.removeAttributeNode(attrs[0]);
        }
      } catch (e) {
        out.drainError = String(e && e.name);
      }
      out.drainIterations = iterations;
      out.remaining = attrs.length;
      out.elementAttrNames = Array.prototype.map.call(el.attributes, (a) => a.name);

      return out;
    })())`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const res = JSON.parse(String(r.result?.value));

  expect(res.initial).toEqual(["id", "class", "data-x"]);
  expect(res.initialLength).toBe(3);

  expect(res.afterRemoveLength).toBe(2);
  expect(res.afterRemoveFirst).toBe("class");
  expect(res.afterRemoveGetNamed).toBe(null);
  expect(res.afterRemoveItem).toEqual(["class", "data-x"]);

  expect(res.afterAddLength).toBe(3);
  expect(res.afterAddGetNamed).toBe("z");
  expect(res.afterAddKeys).toBe(3);

  expect(res.drainError).toBe(null);
  expect(res.drainIterations).toBe(3);
  expect(res.remaining).toBe(0);
  expect(res.elementAttrNames).toEqual([]);
});

// Test 2: 验证 NamedNodeMap 与 Attr 接口特性（item, getNamedItem, setNamedItem, removeNamedItem, 迭代器）
test("NamedNodeMap and Attr interfaces and methods work as specified", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify((function () {
      const el = document.createElement("span");
      const attrs = el.attributes;

      const hasNamedNodeMap = typeof globalThis.NamedNodeMap === "function";
      const hasAttr = typeof globalThis.Attr === "function";
      const isNamedNodeMapInstance = attrs instanceof NamedNodeMap;

      // item() out-of-bounds returns null
      const itemNull = attrs.item(999);

      // setNamedItem
      const a = new Attr("title", "tooltip", el);
      attrs.setNamedItem(a);
      const afterSetLen = attrs.length;
      const getTitleVal = attrs.getNamedItem("TITLE") ? attrs.getNamedItem("TITLE").value : null;

      // Iteration via for...of
      const iteratedNames = [];
      for (const attr of attrs) {
        iteratedNames.push(attr.name);
      }

      // removeNamedItem
      const removedAttr = attrs.removeNamedItem("title");
      const afterRemoveLen = attrs.length;

      return {
        hasNamedNodeMap,
        hasAttr,
        isNamedNodeMapInstance,
        itemNull,
        afterSetLen,
        getTitleVal,
        iteratedNames,
        removedAttrName: removedAttr ? removedAttr.name : null,
        afterRemoveLen,
      };
    })())`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const res = JSON.parse(String(r.result?.value));

  expect(res.hasNamedNodeMap).toBe(true);
  expect(res.hasAttr).toBe(true);
  expect(res.isNamedNodeMapInstance).toBe(true);
  expect(res.itemNull).toBe(null);
  expect(res.afterSetLen).toBe(1);
  expect(res.getTitleVal).toBe("tooltip");
  expect(res.iteratedNames).toEqual(["title"]);
  expect(res.removedAttrName).toBe("title");
  expect(res.afterRemoveLen).toBe(0);
});
