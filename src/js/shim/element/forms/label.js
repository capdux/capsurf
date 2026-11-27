// src/js/shim/element/forms/label.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/label.ts

function isLabelable(el) {
  if (!el || el.nodeType !== 1) return false;
  const tag = (el.tagName || "").toLowerCase();
  if (tag === "input") {
    const type = (el.type || (typeof el.getAttribute === "function" ? el.getAttribute("type") : "") || "text").toLowerCase();
    return type !== "hidden";
  }
  return tag === "button" || tag === "select" || tag === "textarea" ||
         tag === "meter" || tag === "output" || tag === "progress";
}

const _labelsCache = new WeakMap();

function collectLabelsArray(control) {
  if (!isLabelable(control)) return [];
  const root = typeof control.getRootNode === "function" ? control.getRootNode() : null;
  if (!root || (root.nodeType !== 9 && root.nodeType !== 11)) return [];
  const matches = [];
  const seen = new Set();

  // 1. Explicit label with for="control.id"
  const id = control.id || (typeof control.getAttribute === "function" ? control.getAttribute("id") : "");
  if (id) {
    const doc = root;
    const primaryTarget = (doc && typeof doc.getElementById === "function") ? doc.getElementById(id) : control;
    if (doc && typeof doc.querySelectorAll === "function" && (!primaryTarget || primaryTarget === control)) {
      try {
        const all = doc.querySelectorAll('label[for="' + id + '"]');
        for (let i = 0; i < all.length; i++) {
          const l = all[i];
          if (!seen.has(l)) {
            seen.add(l);
            matches.push(l);
          }
        }
      } catch (e) {}
      if (matches.length === 0) {
        try {
          const all = doc.querySelectorAll("label");
          for (let i = 0; i < all.length; i++) {
            const l = all[i];
            const forAttr = typeof l.getAttribute === "function" ? l.getAttribute("for") : l.htmlFor;
            if (forAttr === id && !seen.has(l)) {
              seen.add(l);
              matches.push(l);
            }
          }
        } catch (e) {}
      }
    }
  }

  // 2. Ancestor label(s) (only when label has no `for` attr and this control is its first labelable descendant)
  const ancLabels = [];
  let p = control.parentElement || control.parentNode;
  while (p) {
    if ((p.tagName || "").toLowerCase() === "label") {
      const up = typeof upgradeElement === "function" ? upgradeElement(p) : p;
      const hasFor = typeof up.hasAttribute === "function" ? up.hasAttribute("for") : (up.getAttribute && up.getAttribute("for") !== null);
      if (!hasFor && up.control === control && !seen.has(up)) {
        seen.add(up);
        ancLabels.push(up);
      }
    }
    p = p.parentElement || p.parentNode;
  }
  for (let i = ancLabels.length - 1; i >= 0; i--) {
    matches.push(ancLabels[i]);
  }

  return matches;
}

function getLabelsForControl(control) {
  let cached = _labelsCache.get(control);
  if (cached) return cached;
  const target = new NodeList([]);
  const proxy = new Proxy(target, {
    get: function (t, prop, receiver) {
      if (prop === "length") return collectLabelsArray(control).length;
      if (prop === "item") return function (idx) { const l = collectLabelsArray(control); const i = Number(idx); return i >= 0 && i < l.length ? l[i] : null; };
      if (prop === Symbol.iterator) {
        return function () {
          const l = collectLabelsArray(control);
          let idx = 0;
          return { next: function () { return idx < l.length ? { value: l[idx++], done: false } : { value: undefined, done: true }; } };
        };
      }
      if (typeof prop === "string" && /^\d+$/.test(prop)) {
        const l = collectLabelsArray(control);
        const i = Number(prop);
        return i >= 0 && i < l.length ? l[i] : undefined;
      }
      return Reflect.get(t, prop, receiver);
    },
    has: function (t, prop) {
      if (prop === "length" || prop === "item") return true;
      if (typeof prop === "string" && /^\d+$/.test(prop)) return Number(prop) < collectLabelsArray(control).length;
      return Reflect.has(t, prop);
    },
  });
  _labelsCache.set(control, proxy);
  return proxy;
}

function installLabelElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};
  const HTMLElementProto = (global.HTMLElement && global.HTMLElement.prototype) || ElementProto;

  function HTMLLabelElement() {}
  HTMLLabelElement.prototype = Object.create(HTMLElementProto);
  HTMLLabelElement.prototype.constructor = HTMLLabelElement;
  Object.defineProperty(HTMLLabelElement.prototype, Symbol.toStringTag, {
    value: "HTMLLabelElement",
    configurable: true,
  });

  Object.defineProperty(HTMLLabelElement.prototype, "htmlFor", {
    get: function () { return this.getAttribute("for") || ""; },
    set: function (v) { this.setAttribute("for", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLLabelElement.prototype, "control", {
    get: function () {
      const hasFor = typeof this.hasAttribute === "function" ? this.hasAttribute("for") : (this.getAttribute("for") !== null);
      if (hasFor) {
        const forId = this.getAttribute("for");
        if (!forId) return null;
        const root = typeof this.getRootNode === "function" ? this.getRootNode() : null;
        if (!root || (root.nodeType !== 9 && root.nodeType !== 11)) return null;
        const el = typeof root.getElementById === "function" ? root.getElementById(forId) : null;
        return (el && isLabelable(el)) ? el : null;
      }
      function findFirst(node) {
        if (!node) return null;
        let child = node.firstChild;
        while (child) {
          if (child.nodeType === 1) {
            if (isLabelable(child)) return child;
            const sub = findFirst(child);
            if (sub) return sub;
          }
          child = child.nextSibling;
        }
        return null;
      }
      return findFirst(this);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLLabelElement.prototype, "form", {
    get: function () {
      const c = this.control;
      return c ? c.form : null;
    },
    configurable: true,
  });

  markNative(HTMLLabelElement, "HTMLLabelElement");
  global.HTMLLabelElement = HTMLLabelElement;
  if (typeof globalThis !== "undefined") {
    globalThis.HTMLLabelElement = HTMLLabelElement;
  }

  // Define .labels on ElementProto for labelable elements
  Object.defineProperty(ElementProto, "labels", {
    get: function () {
      const tag = (this.tagName || "").toLowerCase();
      if (tag === "input") {
        const type = (this.type || (typeof this.getAttribute === "function" ? this.getAttribute("type") : "") || "text").toLowerCase();
        if (type === "hidden") return null;
      } else if (tag !== "button" && tag !== "select" && tag !== "textarea" && tag !== "meter" && tag !== "output" && tag !== "progress") {
        return undefined;
      }
      return getLabelsForControl(this);
    },
    configurable: true,
  });
}
