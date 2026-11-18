// src/js/shim/element/forms/textarea.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/textarea.ts

function installTextAreaElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};

  function HTMLTextAreaElement() {
    this._value = undefined;
    this._customValidityMessage = "";
    this._selectionStart = 0;
    this._selectionEnd = 0;
    this._selectionDirection = "none";
  }
  HTMLTextAreaElement.prototype = Object.create(ElementProto);
  HTMLTextAreaElement.prototype.constructor = HTMLTextAreaElement;
  installSelectionMixin(HTMLTextAreaElement);

  Object.defineProperty(HTMLTextAreaElement.prototype, "type", {
    get: function () { return "textarea"; },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "value", {
    get: function () {
      if (this._value !== undefined) return this._value;
      return this.textContent || "";
    },
    set: function (v) {
      this._value = String(v);
      if (this.setAttribute) this.setAttribute("value", String(v));
      this.textContent = String(v);
      this._selectionStart = this._value.length;
      this._selectionEnd = this._value.length;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "defaultValue", {
    get: function () { return this.textContent || ""; },
    set: function (v) { this.textContent = String(v); },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "rows", {
    get: function () { return Number(this.getAttribute("rows") || 2); },
    set: function (v) { this.setAttribute("rows", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "cols", {
    get: function () { return Number(this.getAttribute("cols") || 20); },
    set: function (v) { this.setAttribute("cols", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "name", {
    get: function () { return this.getAttribute("name") || ""; },
    set: function (v) { this.setAttribute("name", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "disabled", {
    get: function () { return this.hasAttribute("disabled"); },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "required", {
    get: function () { return this.hasAttribute("required"); },
    set: function (v) { if (v) this.setAttribute("required", ""); else this.removeAttribute("required"); },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "readOnly", {
    get: function () { return this.hasAttribute("readonly"); },
    set: function (v) { if (v) this.setAttribute("readonly", ""); else this.removeAttribute("readonly"); },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "placeholder", {
    get: function () { return this.getAttribute("placeholder") || ""; },
    set: function (v) { this.setAttribute("placeholder", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "form", {
    get: function () {
      return findAssociatedForm(this);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "willValidate", {
    get: function () {
      return !this.disabled && !this.readOnly;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "validity", {
    get: function () {
      return new ValidityState(this);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "validationMessage", {
    get: function () {
      if (!this.willValidate || this.validity.valid) return "";
      if (this._customValidityMessage) return this._customValidityMessage;
      if (this.validity.valueMissing) return "Please fill out this field.";
      return "Constraints not satisfied.";
    },
    configurable: true,
  });

  HTMLTextAreaElement.prototype.setCustomValidity = function (msg) {
    this._customValidityMessage = String(msg || "");
  };

  HTMLTextAreaElement.prototype.checkValidity = function () {
    if (!this.willValidate) return true;
    if (!this.validity.valid) {
      const evt = new Event("invalid", { bubbles: false, cancelable: true });
      this.dispatchEvent(evt);
      return false;
    }
    return true;
  };

  HTMLTextAreaElement.prototype.reportValidity = function () {
    return this.checkValidity();
  };

  HTMLTextAreaElement.prototype.select = function () {};
  HTMLTextAreaElement.prototype.setSelectionRange = function () {};

  markNative(HTMLTextAreaElement, "HTMLTextAreaElement");
  markNative(HTMLTextAreaElement.prototype.setCustomValidity, "setCustomValidity");
  markNative(HTMLTextAreaElement.prototype.checkValidity, "checkValidity");
  markNative(HTMLTextAreaElement.prototype.reportValidity, "reportValidity");

  global.HTMLTextAreaElement = HTMLTextAreaElement;
}
