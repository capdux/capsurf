// src/js/shim/element/forms/input.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/input.ts

function findAssociatedForm(element) {
  const formAttr = element.getAttribute("form");
  if (formAttr && typeof document !== "undefined" && document.getElementById) {
    const f = document.getElementById(formAttr);
    if (f && f.tagName === "FORM") return f;
  }
  let curr = element.parentElement;
  while (curr) {
    if (curr.tagName === "FORM") return curr;
    curr = curr.parentElement;
  }
  return null;
}

function installInputElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};

  function HTMLInputElement() {
    this._value = "";
    this._checked = false;
    this._customValidityMessage = "";
  }
  HTMLInputElement.prototype = Object.create(ElementProto);
  HTMLInputElement.prototype.constructor = HTMLInputElement;

  Object.defineProperty(HTMLInputElement.prototype, "type", {
    get: function () {
      return (this.getAttribute("type") || "text").toLowerCase();
    },
    set: function (v) {
      this.setAttribute("type", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "value", {
    get: function () {
      if (this.type === "checkbox" || this.type === "radio") {
        return this.getAttribute("value") ?? "on";
      }
      if (this._value !== undefined) return this._value;
      return this.getAttribute("value") || "";
    },
    set: function (v) {
      this._value = String(v);
      if (this.setAttribute) this.setAttribute("value", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "defaultValue", {
    get: function () { return this.getAttribute("value") || ""; },
    set: function (v) { this.setAttribute("value", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "checked", {
    get: function () {
      if (this._checkedDirty) return this._checked;
      return this.hasAttribute("checked");
    },
    set: function (v) {
      this._checkedDirty = true;
      this._checked = Boolean(v);
      const isRadio = (this.getAttribute("type") || "").toLowerCase() === "radio";
      if (isRadio && v && this.name) {
        const form = findAssociatedForm(this);
        const root = form || (this.ownerDocument || (typeof document !== "undefined" ? document : null));
        if (root && root.querySelectorAll) {
          const radios = root.querySelectorAll('input[type="radio"][name="' + this.name + '"]');
          for (let i = 0; i < radios.length; i++) {
            if (radios[i] !== this) {
              radios[i]._checkedDirty = true;
              radios[i]._checked = false;
              if (radios[i].removeAttribute) radios[i].removeAttribute("checked");
            }
          }
        }
      }
      if (v) {
        if (this.setAttribute) this.setAttribute("checked", "");
      } else {
        if (this.removeAttribute) this.removeAttribute("checked");
      }
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "defaultChecked", {
    get: function () { return this.hasAttribute("checked"); },
    set: function (v) { if (v) this.setAttribute("checked", ""); else this.removeAttribute("checked"); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "name", {
    get: function () { return this.getAttribute("name") || ""; },
    set: function (v) { this.setAttribute("name", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "disabled", {
    get: function () { return this.hasAttribute("disabled"); },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "required", {
    get: function () { return this.hasAttribute("required"); },
    set: function (v) { if (v) this.setAttribute("required", ""); else this.removeAttribute("required"); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "readOnly", {
    get: function () { return this.hasAttribute("readonly"); },
    set: function (v) { if (v) this.setAttribute("readonly", ""); else this.removeAttribute("readonly"); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "placeholder", {
    get: function () { return this.getAttribute("placeholder") || ""; },
    set: function (v) { this.setAttribute("placeholder", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "form", {
    get: function () {
      return findAssociatedForm(this);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "willValidate", {
    get: function () {
      if (this.disabled || this.readOnly) return false;
      const t = this.type;
      return !(t === "hidden" || t === "button" || t === "reset");
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "validity", {
    get: function () {
      return new ValidityState(this);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "validationMessage", {
    get: function () {
      if (!this.willValidate || this.validity.valid) return "";
      if (this._customValidityMessage) return this._customValidityMessage;
      if (this.validity.valueMissing) return "Please fill out this field.";
      if (this.validity.typeMismatch) return "Please enter a valid value.";
      return "Constraints not satisfied.";
    },
    configurable: true,
  });

  HTMLInputElement.prototype.setCustomValidity = function (msg) {
    this._customValidityMessage = String(msg || "");
  };

  HTMLInputElement.prototype.checkValidity = function () {
    if (!this.willValidate) return true;
    if (!this.validity.valid) {
      const evt = new Event("invalid", { bubbles: false, cancelable: true });
      this.dispatchEvent(evt);
      return false;
    }
    return true;
  };

  HTMLInputElement.prototype.reportValidity = function () {
    return this.checkValidity();
  };

  HTMLInputElement.prototype.select = function () {};
  HTMLInputElement.prototype.setSelectionRange = function () {};

  markNative(HTMLInputElement, "HTMLInputElement");
  markNative(HTMLInputElement.prototype.setCustomValidity, "setCustomValidity");
  markNative(HTMLInputElement.prototype.checkValidity, "checkValidity");
  markNative(HTMLInputElement.prototype.reportValidity, "reportValidity");

  global.HTMLInputElement = HTMLInputElement;
}
