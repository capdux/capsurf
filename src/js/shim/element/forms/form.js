// src/js/shim/element/forms/form.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/form.ts

function installFormElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};

  function HTMLFormElement() {}
  HTMLFormElement.prototype = Object.create(ElementProto);
  HTMLFormElement.prototype.constructor = HTMLFormElement;

  function collectFormControls(form) {
    const controls = [];
    function walk(node) {
      if (!node) return;
      let child = node.firstChild;
      while (child) {
        const tag = (child.tagName || "").toLowerCase();
        if (tag === "input" || tag === "button" || tag === "select" || tag === "textarea") {
          controls.push(child);
        }
        if (tag !== "select") {
          walk(child);
        }
        child = child.nextSibling;
      }
    }
    walk(form);
    return controls;
  }

  Object.defineProperty(HTMLFormElement.prototype, "elements", {
    get: function () {
      const list = collectFormControls(this);
      const col = new HTMLCollection(list);
      // Map controls by name and id
      for (let i = 0; i < list.length; i++) {
        const item = list[i];
        if (item.name && !col[item.name]) col[item.name] = item;
        if (item.id && !col[item.id]) col[item.id] = item;
      }
      return col;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "length", {
    get: function () { return this.elements.length; },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "name", {
    get: function () { return this.getAttribute("name") || ""; },
    set: function (v) { this.setAttribute("name", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "method", {
    get: function () { return (this.getAttribute("method") || "get").toLowerCase(); },
    set: function (v) { this.setAttribute("method", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "action", {
    get: function () { return this.getAttribute("action") || ""; },
    set: function (v) { this.setAttribute("action", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "target", {
    get: function () { return this.getAttribute("target") || ""; },
    set: function (v) { this.setAttribute("target", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "noValidate", {
    get: function () { return this.hasAttribute("novalidate"); },
    set: function (v) { if (v) this.setAttribute("novalidate", ""); else this.removeAttribute("novalidate"); },
    configurable: true,
  });

  HTMLFormElement.prototype.submit = function () {
    const evt = new Event("submit", { bubbles: true, cancelable: true });
    this.dispatchEvent(evt);
  };

  HTMLFormElement.prototype.reset = function () {
    const list = collectFormControls(this);
    for (let i = 0; i < list.length; i++) {
      const c = list[i];
      if (c.defaultValue !== undefined) c.value = c.defaultValue;
      if (c.defaultChecked !== undefined) c.checked = c.defaultChecked;
      if (c.defaultSelected !== undefined) c.selected = c.defaultSelected;
    }
    const evt = new Event("reset", { bubbles: true, cancelable: true });
    this.dispatchEvent(evt);
  };

  HTMLFormElement.prototype.checkValidity = function () {
    const list = collectFormControls(this);
    let allValid = true;
    for (let i = 0; i < list.length; i++) {
      if (list[i].checkValidity && !list[i].checkValidity()) {
        allValid = false;
      }
    }
    return allValid;
  };

  HTMLFormElement.prototype.reportValidity = function () {
    return this.checkValidity();
  };

  markNative(HTMLFormElement, "HTMLFormElement");
  markNative(HTMLFormElement.prototype.submit, "submit");
  markNative(HTMLFormElement.prototype.reset, "reset");
  markNative(HTMLFormElement.prototype.checkValidity, "checkValidity");
  markNative(HTMLFormElement.prototype.reportValidity, "reportValidity");

  global.HTMLFormElement = HTMLFormElement;
}
