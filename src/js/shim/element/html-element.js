// src/js/shim/element/html-element.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/html-element.ts

function createDatasetProxy(element) {
  return new Proxy({}, {
    get: function (target, prop) {
      if (typeof prop !== "string") return undefined;
      const kebab = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
      return element.getAttribute(kebab) ?? undefined;
    },
    set: function (target, prop, value) {
      if (typeof prop !== "string") return true;
      const kebab = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
      element.setAttribute(kebab, String(value));
      return true;
    },
    deleteProperty: function (target, prop) {
      if (typeof prop !== "string") return true;
      const kebab = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
      element.removeAttribute(kebab);
      return true;
    },
    has: function (target, prop) {
      if (typeof prop !== "string") return false;
      const kebab = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
      return element.hasAttribute(kebab);
    }
  });
}

function installHtmlElementExtensions(proto) {
  if (!proto) return;

  Object.defineProperty(proto, "dataset", {
    get: function () {
      if (!this._datasetProxy) {
        this._datasetProxy = createDatasetProxy(this);
      }
      return this._datasetProxy;
    },
    configurable: true,
  });

  const nativeHasAttribute = proto.hasAttribute || (typeof Element !== "undefined" && Element.prototype && Element.prototype.hasAttribute);
  const nativeSetAttribute = proto.setAttribute || (typeof Element !== "undefined" && Element.prototype && Element.prototype.setAttribute);
  const nativeRemoveAttribute = proto.removeAttribute || (typeof Element !== "undefined" && Element.prototype && Element.prototype.removeAttribute);

  Object.defineProperty(proto, "hidden", {
    get: function () {
      if (nativeHasAttribute) return nativeHasAttribute.call(this, "hidden");
      return this.hasAttribute ? this.hasAttribute("hidden") : false;
    },
    set: function (v) {
      if (v) {
        if (nativeSetAttribute) nativeSetAttribute.call(this, "hidden", "");
        else if (this.setAttribute) this.setAttribute("hidden", "");
      } else {
        if (nativeRemoveAttribute) nativeRemoveAttribute.call(this, "hidden");
        else if (this.removeAttribute) this.removeAttribute("hidden");
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "tabIndex", {
    get: function () {
      const v = this.getAttribute("tabindex");
      return v !== null && v !== "" ? Number(v) : -1;
    },
    set: function (v) {
      this.setAttribute("tabindex", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(proto, "title", {
    get: function () { return this.getAttribute("title") || ""; },
    set: function (v) { this.setAttribute("title", String(v)); },
    configurable: true,
  });

  Object.defineProperty(proto, "lang", {
    get: function () { return this.getAttribute("lang") || ""; },
    set: function (v) { this.setAttribute("lang", String(v)); },
    configurable: true,
  });

  Object.defineProperty(proto, "dir", {
    get: function () { return this.getAttribute("dir") || ""; },
    set: function (v) { this.setAttribute("dir", String(v)); },
    configurable: true,
  });

  Object.defineProperty(proto, "innerText", {
    get: function () { return this.textContent || ""; },
    set: function (v) { this.textContent = String(v); },
    configurable: true,
  });

  if (!proto.click) {
    proto.click = function () {
      const evt = new MouseEvent("click", { bubbles: true, cancelable: true });
      this.dispatchEvent(evt);
    };
    markNative(proto.click, "click");
  }

  if (!proto.focus) {
    proto.focus = function () {
      if (typeof document !== "undefined") {
        document.activeElement = this;
      }
      const evt = new FocusEvent("focus", { bubbles: false, cancelable: false });
      this.dispatchEvent(evt);
    };
    markNative(proto.focus, "focus");
  }

  if (!proto.blur) {
    proto.blur = function () {
      if (typeof document !== "undefined" && document.activeElement === this) {
        document.activeElement = document.body || null;
      }
      const evt = new FocusEvent("blur", { bubbles: false, cancelable: false });
      this.dispatchEvent(evt);
    };
    markNative(proto.blur, "blur");
  }
}
