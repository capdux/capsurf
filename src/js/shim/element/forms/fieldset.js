// src/js/shim/element/forms/fieldset.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/fieldset.ts

function formListedDescendants(root) {
  const out = [];
  function walk(node) {
    if (!node) return;
    let child = node.firstChild;
    while (child) {
      if (child.nodeType === 1) {
        if (typeof isFormListed === "function" && isFormListed(child)) {
          out.push(child);
        }
        walk(child);
      }
      child = child.nextSibling;
    }
  }
  walk(root);
  return out;
}

function installFieldSetElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};
  const HTMLElementProto = (global.HTMLElement && global.HTMLElement.prototype) || ElementProto;

  function HTMLFieldSetElement() {}
  HTMLFieldSetElement.prototype = Object.create(HTMLElementProto);
  HTMLFieldSetElement.prototype.constructor = HTMLFieldSetElement;
  Object.defineProperty(HTMLFieldSetElement.prototype, Symbol.toStringTag, {
    value: "HTMLFieldSetElement",
    configurable: true,
  });

  Object.defineProperty(HTMLFieldSetElement.prototype, "type", {
    get: function () { return "fieldset"; },
    configurable: true,
  });

  Object.defineProperty(HTMLFieldSetElement.prototype, "disabled", {
    get: function () { return this.hasAttribute("disabled"); },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  Object.defineProperty(HTMLFieldSetElement.prototype, "name", {
    get: function () { return this.getAttribute("name") || ""; },
    set: function (v) { this.setAttribute("name", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLFieldSetElement.prototype, "form", {
    get: function () {
      if (typeof findAssociatedForm === "function") {
        return findAssociatedForm(this);
      }
      return null;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLFieldSetElement.prototype, "elements", {
    get: function () {
      const self = this;
      return makeHtmlCollection(function () {
        return formListedDescendants(self);
      });
    },
    configurable: true,
  });

  Object.defineProperty(HTMLFieldSetElement.prototype, "validationMessage", {
    get: function () { return ""; },
    configurable: true,
  });

  markNative(HTMLFieldSetElement, "HTMLFieldSetElement");
  global.HTMLFieldSetElement = HTMLFieldSetElement;
  if (typeof globalThis !== "undefined") {
    globalThis.HTMLFieldSetElement = HTMLFieldSetElement;
  }

  function HTMLLegendElement() {}
  HTMLLegendElement.prototype = Object.create(HTMLElementProto);
  HTMLLegendElement.prototype.constructor = HTMLLegendElement;
  Object.defineProperty(HTMLLegendElement.prototype, Symbol.toStringTag, {
    value: "HTMLLegendElement",
    configurable: true,
  });

  Object.defineProperty(HTMLLegendElement.prototype, "form", {
    get: function () {
      let p = this.parentElement || this.parentNode;
      while (p) {
        if ((p.tagName || "").toLowerCase() === "fieldset") {
          return p.form;
        }
        p = p.parentElement || p.parentNode;
      }
      return null;
    },
    configurable: true,
  });

  markNative(HTMLLegendElement, "HTMLLegendElement");
  global.HTMLLegendElement = HTMLLegendElement;
  if (typeof globalThis !== "undefined") {
    globalThis.HTMLLegendElement = HTMLLegendElement;
  }
}
