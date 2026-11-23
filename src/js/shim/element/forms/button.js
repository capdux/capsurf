// src/js/shim/element/forms/button.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/button.ts

function getButtonType(button) {
  let raw = "";
  if (button && typeof button.getAttribute === "function") {
    raw = button.getAttribute("type") || "";
  }
  if (!raw && button && button._type) {
    raw = button._type;
  }
  const lower = String(raw).toLowerCase().trim();
  if (lower === "button" || lower === "reset" || lower === "submit") return lower;
  if (button && typeof button.hasAttribute === "function" && (button.hasAttribute("command") || button.hasAttribute("commandfor"))) {
    return "button";
  }
  return "submit";
}

function isSubmitOrResetButton(el) {
  if (!el || el.nodeType !== 1) return false;
  if (el.disabled || (typeof el.hasAttribute === "function" && el.hasAttribute("disabled"))) {
    return false;
  }
  const tag = (el.tagName || "").toLowerCase();
  if (tag === "button") {
    const t = getButtonType(el);
    return t === "submit" || t === "reset";
  }
  if (tag === "input") {
    const t = (el.type || (typeof el.getAttribute === "function" ? el.getAttribute("type") : "") || "").toLowerCase();
    return t === "submit" || t === "reset" || t === "image";
  }
  return false;
}

function handleButtonActivation(button) {
  if (!button) return;
  const tag = (button.tagName || "").toLowerCase();
  let type = "submit";
  if (tag === "button") {
    type = getButtonType(button);
  } else if (tag === "input") {
    type = (button.type || (typeof button.getAttribute === "function" ? button.getAttribute("type") : "") || "submit").toLowerCase();
  }

  const rawForm = findAssociatedForm(button);
  if (!rawForm) return;
  const form = typeof upgradeElement === "function" ? upgradeElement(rawForm) : rawForm;

  if (type === "submit" || type === "image") {
    submitForm(form, button);
  } else if (type === "reset") {
    const resetProto = (typeof HTMLFormElement !== "undefined" && HTMLFormElement.prototype && HTMLFormElement.prototype.reset) || null;
    if (typeof resetProto === "function") {
      resetProto.call(form);
    } else if (typeof form.reset === "function") {
      form.reset();
    }
  }
}

function installButtonElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};
  const HTMLElementProto = (global.HTMLElement && global.HTMLElement.prototype) || ElementProto;

  function HTMLButtonElement() {
    this._customValidityMessage = "";
  }
  HTMLButtonElement.prototype = Object.create(HTMLElementProto);
  HTMLButtonElement.prototype.constructor = HTMLButtonElement;
  Object.defineProperty(HTMLButtonElement.prototype, Symbol.toStringTag, {
    value: "HTMLButtonElement",
    configurable: true,
  });

  Object.defineProperty(HTMLButtonElement.prototype, "type", {
    get: function () {
      return getButtonType(this);
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

  if (ElementProto && !ElementProto.form) {
    Object.defineProperty(ElementProto, "form", {
      get: function () {
        const tag = (this.tagName || "").toLowerCase();
        if (tag === "input" || tag === "button" || tag === "select" || tag === "textarea") {
          return findAssociatedForm(this);
        }
        return null;
      },
      configurable: true,
    });
  }

  Object.defineProperty(HTMLButtonElement.prototype, "formAction", {
    get: function () {
      return typeof global.__capsurf_resolveFormActionUrl === "function"
        ? global.__capsurf_resolveFormActionUrl(this, "formaction")
        : (this.getAttribute("formaction") || "");
    },
    set: function (v) { this.setAttribute("formaction", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLButtonElement.prototype, "formMethod", {
    get: function () { return this.getAttribute("formmethod") || ""; },
    set: function (v) { this.setAttribute("formmethod", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLButtonElement.prototype, "willValidate", {
    get: function () { return computeWillValidate(this); },
    configurable: true,
  });

  Object.defineProperty(HTMLButtonElement.prototype, "validity", {
    get: function () { return makeValidityState(this); },
    configurable: true,
  });

  Object.defineProperty(HTMLButtonElement.prototype, "validationMessage", {
    get: function () { return computeValidationMessage(this); },
    configurable: true,
  });

  HTMLButtonElement.prototype.checkValidity = function () {
    if (!this.willValidate) return true;
    if (!this.validity.valid) {
      const evt = new Event("invalid", { bubbles: false, cancelable: true });
      this.dispatchEvent(evt);
      return false;
    }
    return true;
  };
  HTMLButtonElement.prototype.reportValidity = function () { return this.checkValidity(); };
  HTMLButtonElement.prototype.setCustomValidity = function (msg) {
    this._customValidityMessage = String(msg == null ? "" : msg)
      .replace(/\r\n/g, "\n")
      .replace(/\r/g, "\n");
  };

  const buttonPopoverTargetMap = new WeakMap();
  Object.defineProperty(HTMLButtonElement.prototype, "popoverTargetElement", {
    get: markNative(function () {
      if (!(this instanceof HTMLButtonElement)) {
        throw new TypeError("Illegal invocation");
      }
      return buttonPopoverTargetMap.get(this) || null;
    }, "get popoverTargetElement"),
    set: markNative(function (v) {
      if (!(this instanceof HTMLButtonElement)) {
        throw new TypeError("Illegal invocation");
      }
      if (v === null || v === undefined) {
        buttonPopoverTargetMap.delete(this);
        return;
      }
      if (typeof Element === "function" && !(v instanceof Element)) {
        throw new TypeError("Failed to set the 'popoverTargetElement' property on 'HTMLButtonElement': parameter 1 is not of type 'Element'.");
      }
      buttonPopoverTargetMap.set(this, v);
    }, "set popoverTargetElement"),
    enumerable: true,
    configurable: true,
  });

  markNative(HTMLButtonElement, "HTMLButtonElement");
  markNative(HTMLButtonElement.prototype.checkValidity, "checkValidity");
  markNative(HTMLButtonElement.prototype.reportValidity, "reportValidity");

  global.HTMLButtonElement = HTMLButtonElement;
}
