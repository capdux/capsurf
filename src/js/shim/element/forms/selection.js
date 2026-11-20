// src/js/shim/element/forms/selection.js

function supportsSelection(control) {
  if (!control) return false;
  const tag = (control.tagName || "").toLowerCase();
  if (tag === "textarea") return true;
  if (tag === "input") {
    const type = (control.getAttribute("type") || "text").toLowerCase();
    return ["text", "search", "url", "tel", "password"].includes(type);
  }
  return false;
}

function getValueRaw(el) {
  if (el._value !== undefined) return el._value;
  let proto = Object.getPrototypeOf(el);
  while (proto) {
    const desc = Object.getOwnPropertyDescriptor(proto, "value");
    if (desc && typeof desc.get === "function") {
      const v = desc.get.call(el);
      return typeof v === "string" ? v : String(v ?? "");
    }
    proto = Object.getPrototypeOf(proto);
  }
  return el.getAttribute ? (el.getAttribute("value") || "") : "";
}

function setValueRaw(el, newValue) {
  const str = String(newValue);
  const tag = (el && (el.tagName || el.localName) || "").toLowerCase();
  if (tag === "input") {
    const rawType = typeof el.getAttribute === "function" ? el.getAttribute("type") : "";
    const type = typeof normalizeInputType === "function" ? normalizeInputType(rawType) : (rawType || "text").toLowerCase();
    const sanitized = typeof sanitizeInputValue === "function"
      ? sanitizeInputValue(type, str, {
          multiple: typeof el.hasAttribute === "function" && el.hasAttribute("multiple"),
          minAttr: typeof el.getAttribute === "function" ? el.getAttribute("min") : null,
          maxAttr: typeof el.getAttribute === "function" ? el.getAttribute("max") : null,
          stepAttr: typeof el.getAttribute === "function" ? el.getAttribute("step") : null,
        })
      : str;
    el._valueDirty = true;
    el._value = sanitized;
    return;
  }
  el._value = str;
}

function notifyControlValueMutation(control, cursor) {
  if (typeof control.setSelectionRange === "function") {
    control.setSelectionRange(cursor, cursor);
  }
  try {
    const inputEvt = typeof InputEvent !== "undefined"
      ? new InputEvent("input", { bubbles: true, cancelable: false })
      : new Event("input", { bubbles: true });
    control.dispatchEvent(inputEvt);
  } catch (e) {}
}

function installSelectionMixin(Cls) {
  Object.defineProperty(Cls.prototype, "selectionStart", {
    get: function () {
      if (!supportsSelection(this)) return null;
      return this._selectionStart || 0;
    },
    set: function (v) {
      if (!supportsSelection(this)) throw new DOMException("InvalidStateError");
      const n = Math.max(0, Number(v) >>> 0);
      this.setSelectionRange(n, Math.max(n, this._selectionEnd || 0), this._selectionDirection);
    },
    configurable: true,
  });

  Object.defineProperty(Cls.prototype, "selectionEnd", {
    get: function () {
      if (!supportsSelection(this)) return null;
      return this._selectionEnd || 0;
    },
    set: function (v) {
      if (!supportsSelection(this)) throw new DOMException("InvalidStateError");
      const n = Math.max(0, Number(v) >>> 0);
      this.setSelectionRange(this._selectionStart || 0, n, this._selectionDirection);
    },
    configurable: true,
  });

  Cls.prototype.setSelectionRange = function (start, end, direction) {
    if (!supportsSelection(this)) throw new DOMException("InvalidStateError");
    const len = (this.value || "").length;
    let b = Math.min(Math.max(0, Number(end) || 0), len);
    let a = Math.min(Math.max(0, Number(start) || 0), b);
    this._selectionStart = a;
    this._selectionEnd = b;
    this._selectionDirection = (direction === "forward" || direction === "backward") ? direction : "none";

    if (!this._selectionChangeScheduled) {
      this._selectionChangeScheduled = true;
      const self = this;
      setTimeout(function () {
        self._selectionChangeScheduled = false;
        try {
          self.dispatchEvent(new Event("selectionchange"));
        } catch (e) {}
      }, 0);
    }
  };
}

if (typeof globalThis !== "undefined") {
  globalThis.getValueRaw = getValueRaw;
  globalThis.setValueRaw = setValueRaw;
  globalThis.notifyControlValueMutation = notifyControlValueMutation;
}
if (typeof global !== "undefined") {
  global.getValueRaw = getValueRaw;
  global.setValueRaw = setValueRaw;
  global.notifyControlValueMutation = notifyControlValueMutation;
}
