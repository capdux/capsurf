// src/js/shim/element/forms/select.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/select.ts

function installSelectElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};

  // 1. HTMLOptionElement
  function HTMLOptionElement() {
    this._selected = undefined;
  }
  HTMLOptionElement.prototype = Object.create(ElementProto);
  HTMLOptionElement.prototype.constructor = HTMLOptionElement;

  Object.defineProperty(HTMLOptionElement.prototype, "value", {
    get: function () {
      if (this._value !== undefined) return this._value;
      if (this.hasAttribute && this.hasAttribute("value")) return this.getAttribute("value");
      return this.textContent || "";
    },
    set: function (v) {
      this._value = String(v);
      if (this.setAttribute) this.setAttribute("value", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "text", {
    get: function () { return this.textContent || ""; },
    set: function (v) { this.textContent = String(v); },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "selected", {
    get: function () {
      if (this._selected !== undefined) return this._selected;
      return this.hasAttribute("selected");
    },
    set: function (v) {
      this._selected = Boolean(v);
      if (v) {
        if (this.setAttribute) this.setAttribute("selected", "");
      } else {
        if (this.removeAttribute) this.removeAttribute("selected");
      }
    },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "defaultSelected", {
    get: function () { return this.hasAttribute("selected"); },
    set: function (v) { if (v) this.setAttribute("selected", ""); else this.removeAttribute("selected"); },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "disabled", {
    get: function () { return this.hasAttribute("disabled"); },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  markNative(HTMLOptionElement, "HTMLOptionElement");
  global.HTMLOptionElement = HTMLOptionElement;
  global.Option = function (text, value, defaultSelected, selected) {
    const opt = typeof document !== "undefined" && document.createElement ? document.createElement("option") : new HTMLOptionElement();
    if (text !== undefined) opt.text = text;
    if (value !== undefined) opt.value = value;
    if (defaultSelected !== undefined) opt.defaultSelected = Boolean(defaultSelected);
    if (selected !== undefined) opt.selected = Boolean(selected);
    return opt;
  };

  // 2. HTMLSelectElement
  function HTMLSelectElement() {
    this._customValidityMessage = "";
  }
  HTMLSelectElement.prototype = Object.create(ElementProto);
  HTMLSelectElement.prototype.constructor = HTMLSelectElement;

  Object.defineProperty(HTMLSelectElement.prototype, "type", {
    get: function () { return this.multiple ? "select-multiple" : "select-one"; },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "options", {
    get: function () {
      const opts = [];
      let child = this.firstChild;
      while (child) {
        if ((child.tagName || "").toLowerCase() === "option") {
          opts.push(child);
        }
        child = child.nextSibling;
      }
      return new HTMLCollection(opts);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "length", {
    get: function () { return this.options.length; },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "selectedIndex", {
    get: function () {
      const opts = this.options;
      for (let i = 0; i < opts.length; i++) {
        if (opts[i].selected) return i;
      }
      return opts.length > 0 && !this.multiple ? 0 : -1;
    },
    set: function (idx) {
      const i = Number(idx);
      const opts = this.options;
      for (let j = 0; j < opts.length; j++) {
        opts[j].selected = (j === i);
      }
    },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "value", {
    get: function () {
      const idx = this.selectedIndex;
      if (idx >= 0 && idx < this.options.length) {
        return this.options[idx].value;
      }
      return "";
    },
    set: function (v) {
      const str = String(v);
      const opts = this.options;
      for (let i = 0; i < opts.length; i++) {
        if (opts[i].value === str) {
          this.selectedIndex = i;
          return;
        }
      }
      this.selectedIndex = -1;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "multiple", {
    get: function () { return this.hasAttribute("multiple"); },
    set: function (v) { if (v) this.setAttribute("multiple", ""); else this.removeAttribute("multiple"); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "name", {
    get: function () { return this.getAttribute("name") || ""; },
    set: function (v) { this.setAttribute("name", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "disabled", {
    get: function () { return this.hasAttribute("disabled"); },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "required", {
    get: function () { return this.hasAttribute("required"); },
    set: function (v) { if (v) this.setAttribute("required", ""); else this.removeAttribute("required"); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "form", {
    get: function () { return findAssociatedForm(this); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "willValidate", {
    get: function () { return !this.disabled; },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "validity", {
    get: function () { return new ValidityState(this); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "validationMessage", {
    get: function () {
      if (!this.willValidate || this.validity.valid) return "";
      if (this._customValidityMessage) return this._customValidityMessage;
      if (this.validity.valueMissing) return "Please select an item in the list.";
      return "Constraints not satisfied.";
    },
    configurable: true,
  });

  HTMLSelectElement.prototype.setCustomValidity = function (msg) {
    this._customValidityMessage = String(msg || "");
  };

  HTMLSelectElement.prototype.checkValidity = function () {
    if (!this.willValidate) return true;
    if (!this.validity.valid) {
      const evt = new Event("invalid", { bubbles: false, cancelable: true });
      this.dispatchEvent(evt);
      return false;
    }
    return true;
  };

  HTMLSelectElement.prototype.reportValidity = function () {
    return this.checkValidity();
  };

  HTMLSelectElement.prototype.add = function (element, before) {
    if (this.appendChild) {
      this.appendChild(element);
    }
  };

  HTMLSelectElement.prototype.remove = function (index) {
    const opts = this.options;
    const i = Number(index);
    if (i >= 0 && i < opts.length && this.removeChild) {
      this.removeChild(opts[i]);
    }
  };

  markNative(HTMLSelectElement, "HTMLSelectElement");
  markNative(HTMLSelectElement.prototype.setCustomValidity, "setCustomValidity");
  markNative(HTMLSelectElement.prototype.checkValidity, "checkValidity");
  markNative(HTMLSelectElement.prototype.add, "add");
  markNative(HTMLSelectElement.prototype.remove, "remove");

  global.HTMLSelectElement = HTMLSelectElement;
}
