// src/js/shim/element/html-element.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/html-element.ts

function DOMStringMap() {}
if (typeof global !== "undefined") global.DOMStringMap = DOMStringMap;
if (typeof globalThis !== "undefined") globalThis.DOMStringMap = DOMStringMap;
if (typeof window !== "undefined") window.DOMStringMap = DOMStringMap;

function toDatasetAttrName(prop) {
  if (/-[a-z]/.test(prop)) {
    const DOMEx = typeof DOMException === "function" ? DOMException : Error;
    throw new DOMEx("Failed to set the '" + prop + "' property on 'DOMStringMap': '" + prop + "' is not a valid property name.", "SyntaxError");
  }
  const attr = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
  if (/[\s\u0000/>=]/.test(attr)) {
    const DOMEx = typeof DOMException === "function" ? DOMException : Error;
    throw new DOMEx("Failed to set the '" + prop + "' property on 'DOMStringMap': '" + attr + "' is not a valid attribute name.", "InvalidCharacterError");
  }
  return attr;
}

function attrNameToDatasetProp(attrName) {
  if (!attrName || attrName.indexOf("data-") !== 0) return null;
  if (/[A-Z]/.test(attrName)) return null;
  const raw = attrName.slice(5);
  return raw.replace(/-([a-z])/g, function (_, ch) { return ch.toUpperCase(); });
}

function getDatasetKeys(element) {
  const keys = [];
  const attrs = element && element.attributes ? element.attributes : [];
  for (let i = 0; i < attrs.length; i++) {
    const a = attrs[i];
    const p = attrNameToDatasetProp(a && a.name);
    if (p !== null) keys.push(p);
  }
  return keys;
}

function createDatasetProxy(element) {
  const target = Object.create(DOMStringMap.prototype);
  return new Proxy(target, {
    get: function (_t, prop, receiver) {
      if (typeof prop !== "string") return Reflect.get(_t, prop, receiver);
      if (prop in DOMStringMap.prototype && !element.hasAttribute("data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); }))) {
        return Reflect.get(_t, prop, receiver);
      }
      if (/-[a-z]/.test(prop)) return undefined;
      const kebab = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
      const v = element.getAttribute(kebab);
      return v === null ? undefined : v;
    },
    set: function (_t, prop, value) {
      if (typeof prop !== "string") return true;
      const kebab = toDatasetAttrName(prop);
      element.setAttribute(kebab, String(value));
      return true;
    },
    deleteProperty: function (_t, prop) {
      if (typeof prop !== "string") return true;
      if (/-[a-z]/.test(prop)) return true;
      const kebab = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
      element.removeAttribute(kebab);
      return true;
    },
    has: function (_t, prop) {
      if (typeof prop !== "string") return false;
      if (/-[a-z]/.test(prop)) return false;
      const kebab = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
      return element.hasAttribute(kebab) || (prop in DOMStringMap.prototype);
    },
    ownKeys: function () {
      return getDatasetKeys(element);
    },
    getOwnPropertyDescriptor: function (_t, prop) {
      if (typeof prop !== "string" || /-[a-z]/.test(prop)) return undefined;
      const kebab = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
      if (!element.hasAttribute(kebab)) return undefined;
      return {
        value: element.getAttribute(kebab),
        writable: true,
        enumerable: true,
        configurable: true,
      };
    }
  });
}

function installHtmlElementExtensions(proto) {
  if (!proto) return;

  Object.defineProperty(proto, "dataset", {
    get: function () {
      const ns = this.namespaceURI;
      if (ns !== undefined && ns !== "http://www.w3.org/1999/xhtml" && ns !== "http://www.w3.org/2000/svg" && ns !== "http://www.w3.org/1998/Math/MathML") {
        return undefined;
      }
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

  Object.defineProperty(proto, "contentEditable", {
    get: function () {
      if (typeof this.hasAttribute === "function" && this.hasAttribute("contenteditable")) {
        const raw = this.getAttribute("contenteditable");
        const low = String(raw == null ? "" : raw).toLowerCase();
        if (low === "" || low === "true") return "true";
        if (low === "false") return "false";
        if (low === "plaintext-only") return "plaintext-only";
      }
      return "inherit";
    },
    set: function (v) {
      const low = String(v).toLowerCase();
      if (low === "inherit") {
        if (typeof this.removeAttribute === "function") this.removeAttribute("contenteditable");
      } else if (low === "true" || low === "false" || low === "plaintext-only") {
        if (typeof this.setAttribute === "function") this.setAttribute("contenteditable", low);
      } else {
        throw new DOMException(
          "Failed to set the 'contentEditable' property on 'HTMLElement': The value provided ('" + v + "') is not one of 'true', 'false', 'plaintext-only', or 'inherit'.",
          "SyntaxError"
        );
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "isContentEditable", {
    get: function () {
      let cur = this;
      while (cur) {
        if (cur.nodeType === 1 && typeof cur.hasAttribute === "function" && cur.hasAttribute("contenteditable")) {
          const raw = cur.getAttribute("contenteditable");
          const low = String(raw == null ? "" : raw).toLowerCase();
          if (low === "false") return false;
          if (low === "" || low === "true" || low === "plaintext-only") return true;
        }
        if (cur.nodeType === 9) {
          return String(cur.designMode || "").toLowerCase() === "on";
        }
        cur = cur.parentElement || cur.parentNode;
      }
      const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
      if (doc && String(doc.designMode || "").toLowerCase() === "on") return true;
      return false;
    },
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
      const MouseCtor = (global.__capsurf_nativeEvents && global.__capsurf_nativeEvents.MouseEvent) || MouseEvent;
      const dispatchTrusted = typeof dispatchTrustedEvent === "function"
        ? dispatchTrustedEvent
        : function (t, e) { return t.dispatchEvent(e); };
      const selfTag = String(this.tagName || this.localName || "").toLowerCase();
      const selfType = selfTag === "input" ? String(this.getAttribute ? (this.getAttribute("type") || "text") : "text").toLowerCase() : "";
      const isMutableInput = selfTag === "input" && !this.disabled && !(this.hasAttribute && this.hasAttribute("disabled"));
      let preSaved = null;
      if (isMutableInput && selfType === "checkbox") {
        preSaved = { type: "checkbox", checked: Boolean(this.checked), indeterminate: Boolean(this.indeterminate) };
        this.indeterminate = false;
        this.checked = !preSaved.checked;
      } else if (isMutableInput && selfType === "radio") {
        preSaved = { type: "radio", checked: Boolean(this.checked) };
        this.checked = true;
      }
      const evt = new MouseCtor("click", { bubbles: true, cancelable: true, composed: true });
      this.dispatchEvent(evt);
      if (evt.defaultPrevented) {
        if (preSaved) {
          this.checked = preSaved.checked;
          if (preSaved.type === "checkbox") this.indeterminate = preSaved.indeterminate;
        }
        return;
      }
      if (preSaved) {
        if (preSaved.type === "checkbox" || (preSaved.type === "radio" && !preSaved.checked)) {
          dispatchTrusted(this, new Event("input", { bubbles: true, cancelable: false, composed: true }));
          dispatchTrusted(this, new Event("change", { bubbles: true, cancelable: false }));
        }
        return;
      }
      {
        // Walk up ancestors to find an <a>, <label>, or interactive control
        let anchor = null;
        let labelEl = null;
        let cur = this;
        for (let depth = 0; cur && depth < 64; depth++) {
          if (cur.nodeType === 1) {
            const curTag = String(cur.tagName || cur.localName || "").toLowerCase();
            if (curTag === "input") {
              const iType = String(cur.getAttribute ? (cur.getAttribute("type") || "text") : "text").toLowerCase();
              if (!cur.disabled && !(cur.hasAttribute && cur.hasAttribute("disabled"))) {
                if (iType === "checkbox") {
                  cur.indeterminate = false;
                  cur.checked = !cur.checked;
                  dispatchTrusted(cur, new Event("input", { bubbles: true, cancelable: false, composed: true }));
                  dispatchTrusted(cur, new Event("change", { bubbles: true, cancelable: false }));
                } else if (iType === "radio") {
                  const wasChecked = Boolean(cur.checked);
                  cur.checked = true;
                  if (!wasChecked) {
                    dispatchTrusted(cur, new Event("input", { bubbles: true, cancelable: false, composed: true }));
                    dispatchTrusted(cur, new Event("change", { bubbles: true, cancelable: false }));
                  }
                }
              }
              break;
            }
            if (
              curTag === "button" || curTag === "select" || curTag === "textarea" ||
              curTag === "details" || curTag === "embed" || curTag === "iframe" ||
              ((curTag === "audio" || curTag === "video") && cur.hasAttribute && cur.hasAttribute("controls")) ||
              ((curTag === "img" || curTag === "object") && cur.hasAttribute && cur.hasAttribute("usemap"))
            ) {
              break;
            }
            if (curTag === "a") {
              const curHref = cur.getAttribute
                ? (cur.getAttribute("href") ?? (cur.namespaceURI === SVG_NS ? cur.getAttribute("xlink:href") : null))
                : null;
              if (curHref !== null && curHref !== "") {
                anchor = cur;
                break;
              }
            }
            if (curTag === "label") {
              labelEl = cur;
              break;
            }
          }
          cur = cur.parentNode;
        }
        if (labelEl) {
          const ctrl = labelEl.control;
          if (ctrl && ctrl !== this && !(typeof ctrl.contains === "function" && ctrl.contains(this))) {
            const isCtrlDisabled = Boolean(ctrl.disabled || (ctrl.hasAttribute && ctrl.hasAttribute("disabled")));
            if (!isCtrlDisabled && typeof ctrl.dispatchEvent === "function") {
              const fwdEvt = new MouseCtor("click", { bubbles: true, cancelable: true, composed: true });
              ctrl.dispatchEvent(fwdEvt);
              if (!fwdEvt.defaultPrevented) {
                const ctrlTag = String(ctrl.localName || ctrl.tagName || "").toLowerCase();
                if (ctrlTag === "input") {
                  const iType = String(ctrl.getAttribute ? (ctrl.getAttribute("type") || "text") : "text").toLowerCase();
                  if (iType === "checkbox") {
                    ctrl.indeterminate = false;
                    ctrl.checked = !ctrl.checked;
                    ctrl.dispatchEvent(new Event("input", { bubbles: true, cancelable: false, composed: true }));
                    ctrl.dispatchEvent(new Event("change", { bubbles: true, cancelable: false }));
                  } else if (iType === "radio") {
                    const wasChecked = Boolean(ctrl.checked);
                    ctrl.checked = true;
                    if (!wasChecked) {
                      ctrl.dispatchEvent(new Event("input", { bubbles: true, cancelable: false, composed: true }));
                      ctrl.dispatchEvent(new Event("change", { bubbles: true, cancelable: false }));
                    }
                  }
                }
              }
            }
          }
        } else if (anchor) {
          const rawHref = anchor.getAttribute
            ? (anchor.getAttribute("href") ?? (anchor.namespaceURI === SVG_NS ? anchor.getAttribute("xlink:href") : null))
            : anchor.href;
          const href = typeof rawHref === "string" ? rawHref : (rawHref && typeof rawHref.baseVal === "string" ? rawHref.baseVal : "");
          if (href) {
            const tgtAttr = String((anchor.getAttribute ? anchor.getAttribute("target") : anchor.target) || "").toLowerCase();
            if (tgtAttr === "_blank" && typeof globalThis.open === "function") {
              const relAttr = String((anchor.getAttribute ? anchor.getAttribute("rel") : anchor.rel) || "").toLowerCase();
              const isNoop = /(?:^|\s)(noopener|noreferrer)(?:$|\s)/.test(relAttr);
              globalThis.open(href, "_blank", isNoop ? "noopener" : "");
            } else {
              const loc = (typeof location !== "undefined") ? location : ((typeof window !== "undefined") ? window.location : null);
              if (loc) {
                if (href.startsWith("#")) {
                  loc.hash = href;
                } else {
                  loc.href = href;
                }
              }
            }
          }
        } else if (selfTag === "summary") {
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

  function isFocusableElement(el) {
    if (!el || el.nodeType !== 1) return false;
    if (el.disabled) return false;
    const tag = String(el.tagName || el.localName || "").toLowerCase();
    if (tag === "button" || tag === "input" || tag === "select" || tag === "textarea" || tag === "summary" || tag === "iframe" || tag === "body") {
      return true;
    }
    if ((tag === "a" || tag === "area") && typeof el.hasAttribute === "function" && el.hasAttribute("href")) {
      return true;
    }
    if (typeof el.hasAttribute === "function" && el.hasAttribute("tabindex")) {
      return true;
    }
    if (el.isContentEditable) return true;
    if (typeof el.getAttribute === "function") {
      const ce = el.getAttribute("contenteditable");
      if (ce !== null && String(ce).toLowerCase() !== "false") return true;
    }
    return false;
  }

  if (!proto.focus) {
    proto.focus = function (options) {
      if (!isFocusableElement(this)) return;
      if (typeof document !== "undefined") {
        if (document.activeElement === this) return;
        if (document.activeElement && document.activeElement !== this && typeof document.activeElement.blur === "function") {
          document.activeElement.blur();
        }
        document.activeElement = this;
      }
      if (!(options && options.preventScroll)) {
        if (typeof this.scrollIntoViewIfNeeded === "function") {
          this.scrollIntoViewIfNeeded(false);
        } else if (typeof this.scrollIntoView === "function") {
          this.scrollIntoView({ block: "nearest", inline: "nearest" });
        }
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
