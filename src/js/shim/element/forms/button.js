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
  if (lower === "button" || lower === "reset") return lower;
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

  const form = findAssociatedForm(button);
  if (!form) return;

  if (type === "submit" || type === "image") {
    submitForm(form, button);
  } else if (type === "reset") {
    if (typeof form.reset === "function") {
      form.reset();
    }
  }
}

function installButtonElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};

  function HTMLButtonElement() {
    this._customValidityMessage = "";
  }
  HTMLButtonElement.prototype = Object.create(ElementProto);
  HTMLButtonElement.prototype.constructor = HTMLButtonElement;

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
    get: function () { return this.getAttribute("formaction") || ""; },
    set: function (v) { this.setAttribute("formaction", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLButtonElement.prototype, "formMethod", {
    get: function () { return this.getAttribute("formmethod") || ""; },
    set: function (v) { this.setAttribute("formmethod", String(v)); },
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
