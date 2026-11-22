// src/js/shim/element/forms/selection.js

function supportsSelection(control) {
  if (!control) return false;
  const tag = (control.tagName || "").toLowerCase();
  if (tag === "textarea") return true;
  if (tag === "input") {
    const rawType = control.getAttribute("type");
    const type = typeof normalizeInputType === "function" ? normalizeInputType(rawType) : (rawType || "text").toLowerCase();
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
  if (tag !== "input" && tag !== "textarea") {
    return;
  }
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
  const tag = (control && (control.tagName || control.localName) || "").toLowerCase();
  if (tag !== "input" && tag !== "textarea") {
    return;
  }
  const len = String(getValueRaw(control) || "").length;
  const clamped = Math.max(0, Math.min(Number(cursor) || 0, len));
  control._selectionStart = clamped;
  control._selectionEnd = clamped;
  control._selectionDirection = "none";
  try {
    const _ne = (typeof globalThis !== "undefined" && globalThis.__capsurf_nativeEvents) || null;
    const InputCtor = (_ne && _ne.InputEvent) || (typeof InputEvent !== "undefined" ? InputEvent : null);
    const EventCtor = (_ne && _ne.Event) || Event;
    const inputEvt = InputCtor
      ? new InputCtor("input", { bubbles: true, cancelable: false })
      : new EventCtor("input", { bubbles: true });
    control.dispatchEvent(inputEvt);
  } catch (e) {}
}

function installSelectionMixin(Cls) {
  function scheduleSelectEvents(el, rangeChanged) {
    if (!el._selectionChangeScheduled) {
      el._selectionChangeScheduled = true;
      setTimeout(function () {
        el._selectionChangeScheduled = false;
        try {
          el.dispatchEvent(new Event("selectionchange"));
        } catch (e) {}
      }, 0);
    }
    if (rangeChanged && !el._selectEventScheduled) {
      el._selectEventScheduled = true;
      setTimeout(function () {
        el._selectEventScheduled = false;
        try {
          el.dispatchEvent(new Event("select", { bubbles: true, cancelable: false }));
        } catch (e) {}
      }, 0);
    }
  }

  Object.defineProperty(Cls.prototype, "selectionStart", {
    get: function () {
      if (!supportsSelection(this)) return null;
      return this._selectionStart || 0;
    },
    set: function (v) {
      if (!supportsSelection(this)) throw new DOMException("InvalidStateError");
      const n = Number(v) >>> 0;
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
      const n = Number(v) >>> 0;
      this.setSelectionRange(this._selectionStart || 0, n, this._selectionDirection);
    },
    configurable: true,
  });

  Object.defineProperty(Cls.prototype, "selectionDirection", {
    get: function () {
      if (!supportsSelection(this)) return null;
      return this._selectionDirection || "none";
    },
    set: function (v) {
      if (!supportsSelection(this)) throw new DOMException("InvalidStateError");
      this.setSelectionRange(this._selectionStart || 0, this._selectionEnd || 0, String(v));
    },
    configurable: true,
  });

  Cls.prototype.setSelectionRange = function (start, end, direction) {
    if (!supportsSelection(this)) throw new DOMException("InvalidStateError");
    const len = String(getValueRaw(this) || "").length;
    const rawStart = start === null || start === undefined ? 0 : (Number(start) >>> 0);
    const rawEnd = end === null || end === undefined ? 0 : (Number(end) >>> 0);
    const b = Math.min(rawEnd, len);
    const a = Math.min(rawStart, b);
    const dir = (direction === "forward" || direction === "backward") ? direction : "none";
    const prevStart = this._selectionStart || 0;
    const prevEnd = this._selectionEnd || 0;
    const rangeChanged = (prevStart !== a || prevEnd !== b);

    this._selectionStart = a;
    this._selectionEnd = b;
    this._selectionDirection = dir;

    scheduleSelectEvents(this, rangeChanged);
  };

  Cls.prototype.select = function () {
    if (!supportsSelection(this)) return;
    if (typeof this.focus === "function" && typeof document !== "undefined" && document.activeElement !== this) {
      this.focus();
    }
    const len = String(getValueRaw(this) || "").length;
    this.setSelectionRange(0, len, "none");
  };

  Cls.prototype.setRangeText = function (replacement, start, end, selectionMode) {
    if (!supportsSelection(this)) throw new DOMException("InvalidStateError");
    const repl = String(replacement);
    const val = this.value || "";
    const len = val.length;
    const oldStart = this._selectionStart || 0;
    const oldEnd = this._selectionEnd || 0;
    const oldDir = this._selectionDirection || "none";

    let s;
    let e;
    if (arguments.length < 2 || (start === undefined && end === undefined)) {
      s = oldStart;
      e = oldEnd;
    } else {
      s = Number(start) >>> 0;
      e = Number(end) >>> 0;
      if (s > e) {
        throw new DOMException("Failed to execute 'setRangeText': The provided start value is greater than the end value.", "IndexSizeError");
      }
      s = Math.min(s, len);
      e = Math.min(e, len);
    }

    const mode = selectionMode === undefined ? "preserve" : String(selectionMode);
    if (mode !== "select" && mode !== "start" && mode !== "end" && mode !== "preserve") {
      throw new TypeError("Failed to execute 'setRangeText': Invalid selection mode '" + mode + "'.");
    }

    const newVal = val.slice(0, s) + repl + val.slice(e);
    this.value = newVal;

    let newStart = oldStart;
    let newEnd = oldEnd;
    if (mode === "select") {
      newStart = s;
      newEnd = s + repl.length;
    } else if (mode === "start") {
      newStart = s;
      newEnd = s;
    } else if (mode === "end") {
      newStart = s + repl.length;
      newEnd = s + repl.length;
    } else {
      const delta = repl.length - (e - s);
      if (oldStart > e) newStart = oldStart + delta;
      else if (oldStart > s) newStart = s;
      if (oldEnd > e) newEnd = oldEnd + delta;
      else if (oldEnd > s) newEnd = s + repl.length;
      newStart = Math.max(0, Math.min(newStart, newVal.length));
      newEnd = Math.max(newStart, Math.min(newEnd, newVal.length));
    }

    const rangeChanged = (oldStart !== newStart || oldEnd !== newEnd);
    this._selectionStart = newStart;
    this._selectionEnd = newEnd;
    this._selectionDirection = oldDir;
    scheduleSelectEvents(this, rangeChanged);
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
