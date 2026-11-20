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

  Object.defineProperty(proto, "writingSuggestions", {
    get: function () {
      let cur = this;
      while (cur && cur.nodeType === 1) {
        if (typeof cur.hasAttribute === "function" && cur.hasAttribute("writingsuggestions")) {
          const raw = cur.getAttribute("writingsuggestions");
          return String(raw == null ? "" : raw).toLowerCase() === "false" ? "false" : "true";
        }
        cur = cur.parentElement || cur.parentNode;
      }
      return "true";
    },
    set: function (v) {
      if (typeof this.setAttribute === "function") {
        this.setAttribute("writingsuggestions", String(v));
      }
    },
    configurable: true,
  });

  if (!proto.click) {
    proto.click = function () {
      const evt = new MouseEvent("click", { bubbles: true, cancelable: true, composed: true });
      this.dispatchEvent(evt);
      if (!evt.defaultPrevented) {
        const tag = String(this.tagName || this.localName || "").toLowerCase();
        // Walk up ancestors to find an <a> (HTML or SVG with href / xlink:href)
        let anchor = null;
        let cur = this;
        for (let depth = 0; cur && depth < 32; depth++) {
          const curTag = String(cur.tagName || cur.localName || "").toLowerCase();
          if (curTag === "a") {
            const curHref = cur.getAttribute
              ? (cur.getAttribute("href") ?? (cur.namespaceURI === SVG_NS ? cur.getAttribute("xlink:href") : null))
              : null;
            if (curHref !== null && curHref !== "") {
              anchor = cur;
              break;
            }
          }
          cur = cur.parentElement || cur.parentNode;
        }
        if (anchor) {
          const rawHref = anchor.getAttribute
            ? (anchor.getAttribute("href") ?? (anchor.namespaceURI === SVG_NS ? anchor.getAttribute("xlink:href") : null))
            : anchor.href;
          const href = typeof rawHref === "string" ? rawHref : (rawHref && typeof rawHref.baseVal === "string" ? rawHref.baseVal : "");
          if (href) {
            const loc = (typeof location !== "undefined") ? location : ((typeof window !== "undefined") ? window.location : null);
            if (loc) {
              if (href.startsWith("#")) {
                loc.hash = href;
              } else {
                loc.href = href;
              }
            }
          }
        } else if (tag === "summary") {
          const parent = this.parentElement || this.parentNode;
          if (parent && String(parent.localName || parent.tagName || "").toLowerCase() === "details") {
            const kids = parent._children || (parent.childNodes ? Array.from(parent.childNodes) : []);
            let firstSummary = null;
            for (let i = 0; i < kids.length; i++) {
              const k = kids[i];
              if (k && k.nodeType === 1 && String(k.localName || k.tagName || "").toLowerCase() === "summary") {
                firstSummary = k;
                break;
              }
            }
            if (firstSummary === this) {
              parent.open = !parent.open;
              if (typeof parent.dispatchEvent === "function") {
                try {
                  parent.dispatchEvent(new Event("toggle", { bubbles: false, cancelable: false }));
                } catch (e) {}
              }
            }
          }
        }
      }
    };
    markNative(proto.click, "click");
  }

  if (!proto.focus) {
    proto.focus = function () {
      if (typeof document !== "undefined") {
        if (document.activeElement && document.activeElement !== this && typeof document.activeElement.blur === "function") {
          document.activeElement.blur();
        }
        document.activeElement = this;
      }
      if (typeof this.scrollIntoViewIfNeeded === "function") {
        this.scrollIntoViewIfNeeded();
      } else if (typeof this.scrollIntoView === "function") {
        this.scrollIntoView();
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
