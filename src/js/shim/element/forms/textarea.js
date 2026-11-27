// src/js/shim/element/forms/textarea.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/textarea.ts

function installTextAreaElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};
  const HTMLElementProto = (global.HTMLElement && global.HTMLElement.prototype) || ElementProto;

  function HTMLTextAreaElement() {
    this._value = undefined;
    this._customValidityMessage = "";
    this._selectionStart = 0;
    this._selectionEnd = 0;
    this._selectionDirection = "none";
  }
  HTMLTextAreaElement.prototype = Object.create(HTMLElementProto);
  HTMLTextAreaElement.prototype.constructor = HTMLTextAreaElement;
  Object.defineProperty(HTMLTextAreaElement.prototype, Symbol.toStringTag, {
    value: "HTMLTextAreaElement",
    configurable: true,
  });
  installSelectionMixin(HTMLTextAreaElement);

  Object.defineProperty(HTMLTextAreaElement.prototype, "type", {
    get: function () { return "textarea"; },
    configurable: true,
  });

  function getTextAreaDefaultValue(el) {
    let out = "";
    const kids = el.childNodes;
    if (kids && kids.length) {
      for (let i = 0; i < kids.length; i++) {
        const k = kids[i];
        if (k && (k.nodeType === 3 || k.nodeType === 4)) {
          out += k.data !== undefined ? String(k.data) : String(k.textContent || "");
        }
      }
      return out;
    }
    return el.textContent || "";
  }

  Object.defineProperty(HTMLTextAreaElement.prototype, "value", {
    get: function () {
      if (this._value !== undefined) return this._value;
      return getTextAreaDefaultValue(this).replace(/\r\n?/g, "\n");
    },
    set: function (v) {
      const cur = this._value !== undefined ? this._value : getTextAreaDefaultValue(this).replace(/\r\n?/g, "\n");
      const next = String(v).replace(/\r\n?/g, "\n");
      this._value = next;
      if (next !== cur) {
        this._selectionStart = next.length;
        this._selectionEnd = next.length;
        this._selectionDirection = "none";
      }
    },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "defaultValue", {
    get: function () { return getTextAreaDefaultValue(this); },
    set: function (v) { this.textContent = String(v); },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "textLength", {
    get: function () {
      return this.value.length;
    },
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

  Object.defineProperty(HTMLTextAreaElement.prototype, "maxLength", {
    get: function () {
      const v = this.getAttribute("maxlength");
      if (v == null) return -1;
      const n = parseInt(v, 10);
      return isNaN(n) || n < 0 ? -1 : n;
    },
    set: function (v) {
      const n = Number(v) | 0;
      if (n < 0) {
        const DOMEx = global.DOMException || Error;
        throw new DOMEx("maxLength: negative value", "IndexSizeError");
      }
      this.setAttribute("maxlength", String(n));
    },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "minLength", {
    get: function () {
      const v = this.getAttribute("minlength");
      if (v == null) return -1;
      const n = parseInt(v, 10);
      return isNaN(n) || n < 0 ? -1 : n;
    },
    set: function (v) {
      const n = Number(v) | 0;
      if (n < 0) {
        const DOMEx = global.DOMException || Error;
        throw new DOMEx("minLength: negative value", "IndexSizeError");
      }
      this.setAttribute("minlength", String(n));
    },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "willValidate", {
    get: function () {
      return computeWillValidate(this);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "validity", {
    get: function () {
      return makeValidityState(this);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLTextAreaElement.prototype, "validationMessage", {
    get: function () {
      return computeValidationMessage(this);
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
      if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_dispatchTrustedEvent === "function") {
        globalThis.__capsurf_dispatchTrustedEvent(this, evt);
      } else {
        this.dispatchEvent(evt);
      }
      return false;
    }
    return true;
  };

  HTMLTextAreaElement.prototype.reportValidity = function () {
    return this.checkValidity();
  };

  markNative(HTMLTextAreaElement, "HTMLTextAreaElement");
  markNative(HTMLTextAreaElement.prototype.setCustomValidity, "setCustomValidity");
  markNative(HTMLTextAreaElement.prototype.checkValidity, "checkValidity");
  markNative(HTMLTextAreaElement.prototype.reportValidity, "reportValidity");

  global.HTMLTextAreaElement = HTMLTextAreaElement;
}
