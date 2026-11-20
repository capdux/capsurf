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

function inputValueMode(type) {
  if (type === "checkbox" || type === "radio") return "default/on";
  if (type === "file") return "filename";
  if (
    type === "hidden" ||
    type === "submit" ||
    type === "reset" ||
    type === "image" ||
    type === "button"
  ) {
    return "default";
  }
  return "value";
}

function sanitizeForInput(el, raw) {
  const t = normalizeInputType(el.getAttribute ? el.getAttribute("type") : "");
  return sanitizeInputValue(t, raw, {
    multiple: el.hasAttribute ? el.hasAttribute("multiple") : false,
    minAttr: el.getAttribute ? el.getAttribute("min") : null,
    maxAttr: el.getAttribute ? el.getAttribute("max") : null,
    stepAttr: el.getAttribute ? el.getAttribute("step") : null,
  });
}

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
  const HTMLElementProto = (global.HTMLElement && global.HTMLElement.prototype) || ElementProto;

  function HTMLInputElement() {
    this._value = "";
    this._valueDirty = false;
    this._checked = false;
    this._checkedDirty = false;
    this._indeterminate = false;
    this._userEdited = false;
    this._customValidityMessage = "";
    this._selectionStart = 0;
    this._selectionEnd = 0;
    this._selectionDirection = "none";
  }
  HTMLInputElement.prototype = Object.create(HTMLElementProto);
  HTMLInputElement.prototype.constructor = HTMLInputElement;
  Object.defineProperty(HTMLInputElement.prototype, Symbol.toStringTag, {
    value: "HTMLInputElement",
    configurable: true,
  });
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
      const mode = inputValueMode(t);
      const attr = this.getAttribute("value");
      if (mode === "default/on") {
        return attr !== null ? attr : "on";
      }
      if (mode === "default") {
        return attr !== null ? attr : "";
      }
      if (mode === "filename") {
        return this._valueDirty ? (this._value || "") : "";
      }
      if (this._valueDirty) return this._value;
      return sanitizeForInput(this, attr !== null ? attr : "");
    },
    set: function (v) {
      const t = this.type;
      const mode = inputValueMode(t);
      const str = v === null ? "" : String(v);
      if (mode === "filename") {
        if (str !== "") {
          const DOMEx = global.DOMException || Error;
          throw new DOMEx("HTMLInputElement.value: setting non-empty value on file input not allowed", "InvalidStateError");
        }
        this._valueDirty = true;
        this._value = "";
        return;
      }
      if (mode === "default" || mode === "default/on") {
        this.setAttribute("value", str);
        return;
      }
      this._valueDirty = true;
      this._value = sanitizeForInput(this, str);
      this._selectionStart = this._value.length;
      this._selectionEnd = this._value.length;
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

  Object.defineProperty(HTMLInputElement.prototype, "indeterminate", {
    get: function () {
      return Boolean(this._indeterminate);
    },
    set: function (v) {
      this._indeterminate = Boolean(v);
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
      if (!isSteppableInputType(t)) return NaN;
      const parsed = parseInputByType(t, this.value);
      return parsed == null ? NaN : parsed;
    },
    set: function (v) {
      const n = Number(v);
      if (n === Infinity || n === -Infinity) {
        throw new TypeError("valueAsNumber: value is infinite");
      }
      const t = this.type;
      if (!isSteppableInputType(t)) {
        const DOMEx = global.DOMException || Error;
        throw new DOMEx("valueAsNumber: not supported on input[type=" + t + "]", "InvalidStateError");
      }
      if (isNaN(n)) {
        this.value = "";
        return;
      }
      const s = serializeInputByType(t, n);
      this.value = parseInputByType(t, s) == null ? "" : s;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "valueAsDate", {
    get: function () {
      const t = this.type;
      if (!acceptsValueAsDateType(t)) return null;
      return valueAsDateFromString(t, this.value);
    },
    set: function (v) {
      const t = this.type;
      if (!acceptsValueAsDateType(t)) {
        const DOMEx = global.DOMException || Error;
        throw new DOMEx("valueAsDate: not supported on input[type=" + t + "]", "InvalidStateError");
      }
      if (v !== null && !(v instanceof Date)) {
        throw new TypeError("valueAsDate: expected Date or null");
      }
      const s = valueAsDateToString(t, v);
      this.value = s == null ? "" : s;
    },
    configurable: true,
  });

  function applyStep(input, n) {
    const t = input.type;
    const step = input.getAttribute("step");
    const min = input.getAttribute("min");
    const max = input.getAttribute("max");
    const valAttr = input.getAttribute("value");
    const stepResult = computeInputStep(t, input.value, min, max, step, n, valAttr);
    if (stepResult.error) {
      const DOMEx = global.DOMException || Error;
      throw new DOMEx(
        "stepUp/stepDown: not supported on input[type=" + t + "] with step=" + (step || "default"),
        "InvalidStateError"
      );
    }
    if (stepResult.noop || stepResult.value === undefined) return;
    input.value = stepResult.value;
  }

  function stepArg(v) {
    const n = Number(v);
    if (!Number.isFinite(n)) return 0;
    return n | 0;
  }

  HTMLInputElement.prototype.stepUp = function (n) {
    applyStep(this, stepArg(arguments.length ? n : 1));
  };

  HTMLInputElement.prototype.stepDown = function (n) {
    applyStep(this, -stepArg(arguments.length ? n : 1));
  };

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

  Object.defineProperty(HTMLInputElement.prototype, "multiple", {
    get: function () { return this.hasAttribute("multiple"); },
    set: function (v) { if (v) this.setAttribute("multiple", ""); else this.removeAttribute("multiple"); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "autofocus", {
    get: function () { return this.hasAttribute("autofocus"); },
    set: function (v) { if (v) this.setAttribute("autofocus", ""); else this.removeAttribute("autofocus"); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "placeholder", {
    get: function () { return this.getAttribute("placeholder") || ""; },
    set: function (v) { this.setAttribute("placeholder", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "autocomplete", {
    get: function () { return this.getAttribute("autocomplete") || ""; },
    set: function (v) { this.setAttribute("autocomplete", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "pattern", {
    get: function () { return this.getAttribute("pattern") || ""; },
    set: function (v) { this.setAttribute("pattern", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "min", {
    get: function () { return this.getAttribute("min") || ""; },
    set: function (v) { this.setAttribute("min", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "max", {
    get: function () { return this.getAttribute("max") || ""; },
    set: function (v) { this.setAttribute("max", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "step", {
    get: function () { return this.getAttribute("step") || ""; },
    set: function (v) { this.setAttribute("step", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "maxLength", {
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

  Object.defineProperty(HTMLInputElement.prototype, "minLength", {
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

  Object.defineProperty(HTMLInputElement.prototype, "size", {
    get: function () {
      const v = this.getAttribute("size");
      if (v == null) return 20;
      const n = parseInt(v, 10);
      return isNaN(n) || n <= 0 ? 20 : n;
    },
    set: function (v) {
      const n = Number(v) | 0;
      if (n <= 0) {
        const DOMEx = global.DOMException || Error;
        throw new DOMEx("size: value must be > 0", "IndexSizeError");
      }
      this.setAttribute("size", String(n));
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "form", {
    get: function () {
      return findAssociatedForm(this);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "list", {
    get: function () {
      const t = this.type;
      if (
        t === "hidden" || t === "password" || t === "checkbox" || t === "radio" ||
        t === "file" || t === "submit" || t === "image" || t === "reset" || t === "button"
      ) {
        return null;
      }
      const listId = this.getAttribute ? this.getAttribute("list") : null;
      if (!listId) return null;
      const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
      if (!doc || typeof doc.getElementById !== "function") return null;
      const el = doc.getElementById(listId);
      if (!el) return null;
      const tag = String(el.localName || el.tagName || "").toLowerCase();
      if (tag !== "datalist") return null;
      return typeof upgradeElement === "function" ? upgradeElement(el) : el;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "willValidate", {
    get: function () {
      return computeWillValidate(this);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "validity", {
    get: function () {
      return makeValidityState(this);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLInputElement.prototype, "validationMessage", {
    get: function () {
      return computeValidationMessage(this);
    },
    configurable: true,
  });

  HTMLInputElement.prototype.setCustomValidity = function (msg) {
    this._customValidityMessage = String(msg == null ? "" : msg)
      .replace(/\r\n/g, "\n")
      .replace(/\r/g, "\n");
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
  markNative(HTMLInputElement.prototype.stepUp, "stepUp");
  markNative(HTMLInputElement.prototype.stepDown, "stepDown");
  markNative(HTMLInputElement.prototype.setCustomValidity, "setCustomValidity");
  markNative(HTMLInputElement.prototype.checkValidity, "checkValidity");
  markNative(HTMLInputElement.prototype.reportValidity, "reportValidity");

  global.HTMLInputElement = HTMLInputElement;
}
