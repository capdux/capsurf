// src/js/shim/element/forms/input.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/input.ts

const VALID_INPUT_TYPES = new Set([
  "button",
  "checkbox",
  "color",
  "date",
  "datetime-local",
  "email",
  "file",
  "hidden",
  "image",
  "month",
  "number",
  "password",
  "radio",
  "range",
  "reset",
  "search",
  "submit",
  "tel",
  "text",
  "time",
  "url",
  "week",
]);

function normalizeInputType(raw) {
  if (!raw) return "text";
  const lower = String(raw).toLowerCase().trim();
  return VALID_INPUT_TYPES.has(lower) ? lower : "text";
}

const STEPPABLE_TYPES = new Set([
  "number",
  "range",
  "date",
  "month",
  "week",
  "time",
  "datetime-local",
]);

function findAssociatedForm(element) {
  if (!element) return null;
  const formAttr = (typeof element.getAttribute === "function") ? element.getAttribute("form") : null;
  if (formAttr && typeof document !== "undefined" && document.getElementById) {
    const f = document.getElementById(formAttr);
    if (f && (f.tagName || "").toLowerCase() === "form") return f;
  }
  let curr = element.parentElement || element.parentNode;
  while (curr) {
    if ((curr.tagName || "").toLowerCase() === "form") return curr;
    curr = curr.parentElement || curr.parentNode;
  }
  return null;
}

function installInputElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};

  function HTMLInputElement() {
    this._value = "";
    this._valueDirty = false;
    this._checked = false;
    this._checkedDirty = false;
    this._customValidityMessage = "";
    this._selectionStart = 0;
    this._selectionEnd = 0;
    this._selectionDirection = "none";
  }
  HTMLInputElement.prototype = Object.create(ElementProto);
  HTMLInputElement.prototype.constructor = HTMLInputElement;
  installSelectionMixin(HTMLInputElement);

  Object.defineProperty(HTMLInputElement.prototype, "type", {
    get: function () {
      return normalizeInputType(this.getAttribute("type"));
    },
    set: function (v) {
      this.setAttribute("type", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "value", {
    get: function () {
      const t = this.type;
      if (t === "checkbox" || t === "radio") {
        const attr = this.getAttribute("value");
        return attr !== null ? attr : "on";
      }
      if (this._valueDirty) return this._value;
      const attr = this.getAttribute("value");
      return attr !== null ? attr : "";
    },
    set: function (v) {
      const t = this.type;
      const str = v === null ? "" : String(v);
      if (t === "checkbox" || t === "radio") {
        this.setAttribute("value", str);
        return;
      }
      this._valueDirty = true;
      this._value = str;
      this._selectionStart = str.length;
      this._selectionEnd = str.length;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "defaultValue", {
    get: function () {
      return (this.getAttribute && this.getAttribute("value")) || "";
    },
    set: function (v) {
      const str = String(v);
      this.setAttribute("value", str);
    },
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
      const isRadio = this.type === "radio";
      if (isRadio && v && this.name) {
        const form = findAssociatedForm(this);
        const root = form || (this.ownerDocument || (typeof document !== "undefined" ? document : null));
        if (root && root.querySelectorAll) {
          const radios = root.querySelectorAll('input[type="radio"][name="' + this.name + '"]');
          for (let i = 0; i < radios.length; i++) {
            if (radios[i] !== this) {
              radios[i]._checkedDirty = true;
              radios[i]._checked = false;
            }
          }
        }
      }
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "defaultChecked", {
    get: function () {
      return this.hasAttribute("checked");
    },
    set: function (v) {
      if (v) this.setAttribute("checked", "");
      else this.removeAttribute("checked");
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "textLength", {
    get: function () {
      return this.value.length;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "valueAsNumber", {
    get: function () {
      const t = this.type;
      if (!STEPPABLE_TYPES.has(t)) return NaN;
      if (t === "number" || t === "range") {
        const v = parseFloat(this.value);
        return isNaN(v) ? NaN : v;
      }
      if (t === "date") {
        const parts = this.value.split("-");
        if (parts.length === 3) {
          const y = parseInt(parts[0], 10);
          const m = parseInt(parts[1], 10) - 1;
          const d = parseInt(parts[2], 10);
          return Date.UTC(y, m, d);
        }
        return NaN;
      }
      if (t === "month") {
        const parts = this.value.split("-");
        if (parts.length === 2) {
          const y = parseInt(parts[0], 10);
          const m = parseInt(parts[1], 10);
          return (y - 1970) * 12 + (m - 1);
        }
        return NaN;
      }
      if (t === "time") {
        const parts = this.value.split(":");
        if (parts.length >= 2) {
          const h = parseInt(parts[0], 10);
          const m = parseInt(parts[1], 10);
          const s = parts.length > 2 ? parseFloat(parts[2]) : 0;
          return h * 3600000 + m * 60000 + Math.round(s * 1000);
        }
        return NaN;
      }
      if (t === "datetime-local") {
        const m = /^(\d{4,})-(\d{2})-(\d{2})[T ](\d{2}):(\d{2})(?::(\d{2})(?:\.(\d{1,3}))?)?$/.exec(this.value);
        if (m) {
          const y = +m[1];
          const mo = +m[2] - 1;
          const d = +m[3];
          const h = +m[4];
          const mi = +m[5];
          const s = m[6] ? +m[6] : 0;
          const ms = m[7] ? +m[7].padEnd(3, "0") : 0;
          return Date.UTC(y, mo, d, h, mi, s, ms);
        }
        return NaN;
      }
      return NaN;
    },
    set: function (v) {
      const n = Number(v);
      if (n === Infinity || n === -Infinity) {
        throw new TypeError("valueAsNumber: value is infinite");
      }
      const t = this.type;
      if (!STEPPABLE_TYPES.has(t)) {
        const DOMEx = global.DOMException || Error;
        throw new DOMEx("valueAsNumber: not supported on input[type=" + t + "]", "InvalidStateError");
      }
      if (isNaN(n)) {
        this.value = "";
        return;
      }
      this.value = String(n);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "valueAsDate", {
    get: function () {
      const t = this.type;
      if (t !== "date" && t !== "month" && t !== "week" && t !== "time") return null;
      const num = this.valueAsNumber;
      if (isNaN(num)) return null;
      if (t === "month") {
        return new Date(Date.UTC(1970, num, 1));
      }
      return new Date(num);
    },
    set: function (v) {
      const t = this.type;
      if (t !== "date" && t !== "month" && t !== "week" && t !== "time") {
        const DOMEx = global.DOMException || Error;
        throw new DOMEx("valueAsDate: not supported on input[type=" + t + "]", "InvalidStateError");
      }
      if (v === null || v === undefined) {
        this.value = "";
        return;
      }
      if (!(v instanceof Date) || isNaN(v.getTime())) {
        this.value = "";
        return;
      }
      if (t === "date") {
        const y = v.getUTCFullYear();
        const m = String(v.getUTCMonth() + 1).padStart(2, "0");
        const d = String(v.getUTCDate()).padStart(2, "0");
        this.value = y + "-" + m + "-" + d;
      } else {
        this.value = String(v.getTime());
      }
    },
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

  markNative(HTMLInputElement, "HTMLInputElement");
  markNative(HTMLInputElement.prototype.setCustomValidity, "setCustomValidity");
  markNative(HTMLInputElement.prototype.checkValidity, "checkValidity");
  markNative(HTMLInputElement.prototype.reportValidity, "reportValidity");

  global.HTMLInputElement = HTMLInputElement;
}
