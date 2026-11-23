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

function getLabelsForControl(control) {
  const matches = [];
  const seen = new Set();

  // 1. Explicit label with for="control.id"
  const id = control.id || (typeof control.getAttribute === "function" ? control.getAttribute("id") : "");
  if (id) {
    const doc = control.ownerDocument || (typeof document !== "undefined" ? document : null);
    if (doc && typeof doc.querySelectorAll === "function") {
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
  let p = control.parentElement || control.parentNode;
  while (p) {
    if ((p.tagName || "").toLowerCase() === "label") {
      const up = typeof upgradeElement === "function" ? upgradeElement(p) : p;
      const hasFor = typeof up.hasAttribute === "function" ? up.hasAttribute("for") : (up.getAttribute && up.getAttribute("for") !== null);
      if (!hasFor && up.control === control && !seen.has(up)) {
        seen.add(up);
        matches.push(up);
      }
    }
    p = p.parentElement || p.parentNode;
  }

  return new NodeList(matches);
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
        const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
        const el = doc && typeof doc.getElementById === "function" ? doc.getElementById(forId) : null;
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
        return null;
      }
      return getLabelsForControl(this);
    },
    configurable: true,
  });
}
