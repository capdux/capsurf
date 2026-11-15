// src/js/shim/element/forms/button.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/button.ts

function installButtonElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};

  function HTMLButtonElement() {
    this._customValidityMessage = "";
  }
  HTMLButtonElement.prototype = Object.create(ElementProto);
  HTMLButtonElement.prototype.constructor = HTMLButtonElement;

  Object.defineProperty(HTMLButtonElement.prototype, "type", {
    get: function () {
      return (this.getAttribute("type") || "submit").toLowerCase();
    },
    set: function (v) {
      this.setAttribute("type", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(HTMLButtonElement.prototype, "name", {
    get: function () { return this.getAttribute("name") || ""; },
    set: function (v) { this.setAttribute("name", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLButtonElement.prototype, "value", {
    get: function () { return this.getAttribute("value") || ""; },
    set: function (v) { this.setAttribute("value", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLButtonElement.prototype, "disabled", {
    get: function () { return this.hasAttribute("disabled"); },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  Object.defineProperty(HTMLButtonElement.prototype, "form", {
    get: function () { return findAssociatedForm(this); },
    configurable: true,
  });

  Object.defineProperty(HTMLButtonElement.prototype, "willValidate", {
    get: function () { return false; },
    configurable: true,
  });

  HTMLButtonElement.prototype.checkValidity = function () { return true; };
  HTMLButtonElement.prototype.reportValidity = function () { return true; };
  HTMLButtonElement.prototype.setCustomValidity = function (msg) {
    this._customValidityMessage = String(msg || "");
  };

  markNative(HTMLButtonElement, "HTMLButtonElement");
  markNative(HTMLButtonElement.prototype.checkValidity, "checkValidity");
  markNative(HTMLButtonElement.prototype.reportValidity, "reportValidity");

  global.HTMLButtonElement = HTMLButtonElement;
}
