// src/js/shim/element/element.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/element.ts

function createStyleProxy(element) {
  function parseStyleMap() {
    const raw = element.getAttribute("style") || "";
    const map = new Map();
    const parts = raw.split(";");
    for (let i = 0; i < parts.length; i++) {
      const p = parts[i].trim();
      if (!p) continue;
      const colon = p.indexOf(":");
      if (colon !== -1) {
        const k = p.slice(0, colon).trim().toLowerCase();
        const v = p.slice(colon + 1).trim();
        map.set(k, v);
      }
    }
    return map;
  }

  function serializeStyleMap(map) {
    const arr = [];
    for (const [k, v] of map.entries()) {
      arr.push(k + ": " + v);
    }
    const res = arr.join("; ");
    if (res.length > 0) {
      element.setAttribute("style", res);
    } else {
      element.removeAttribute("style");
    }
  }

  return new Proxy({}, {
    get: function (target, prop) {
      if (typeof prop !== "string") return undefined;
      if (prop === "cssText") return element.getAttribute("style") || "";
      if (prop === "length") return parseStyleMap().size;
      if (prop === "getPropertyValue") {
        return function (p) {
          const map = parseStyleMap();
          return map.get(String(p).toLowerCase()) || "";
        };
      }
      if (prop === "setProperty") {
        return function (p, val) {
          const map = parseStyleMap();
          map.set(String(p).toLowerCase(), String(val));
          serializeStyleMap(map);
        };
      }
      if (prop === "removeProperty") {
        return function (p) {
          const map = parseStyleMap();
          const key = String(p).toLowerCase();
          const old = map.get(key) || "";
          map.delete(key);
          serializeStyleMap(map);
          return old;
        };
      }
      // CamelCase to kebab-case
      const kebab = prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
      const map = parseStyleMap();
      return map.get(kebab) || "";
    },
    set: function (target, prop, value) {
      if (typeof prop !== "string") return true;
      if (prop === "cssText") {
        element.setAttribute("style", String(value));
        return true;
      }
      const kebab = prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
      const map = parseStyleMap();
      if (value === null || value === undefined || value === "") {
        map.delete(kebab);
      } else {
        map.set(kebab, String(value));
      }
      serializeStyleMap(map);
      return true;
    }
  });
}

function installElementExtensions(proto) {
  if (!proto) return;

  // Make Element inherit from Node / EventTarget
  Object.setPrototypeOf(proto, Node.prototype);

  if (proto.nodeType === undefined) {
    Object.defineProperty(proto, "nodeType", {
      value: ELEMENT_NODE,
      writable: false,
      configurable: true,
    });
  }

  Object.defineProperty(proto, "nodeName", {
    get: function () {
      return this.tagName || "";
    },
    configurable: true,
  });

  Object.defineProperty(proto, "localName", {
    get: function () {
      return (this.tagName || "").toLowerCase();
    },
    configurable: true,
  });

  // classList getter
  Object.defineProperty(proto, "classList", {
    get: function () {
      if (!this._classList) {
        const self = this;
        this._classList = new DOMTokenList(
          function () { return self.className || ""; },
          function (val) { self.className = val; }
        );
      }
      return this._classList;
    },
    configurable: true,
  });

  // style getter/setter
  Object.defineProperty(proto, "style", {
    get: function () {
      if (!this._styleProxy) {
        this._styleProxy = createStyleProxy(this);
      }
      return this._styleProxy;
    },
    set: function (val) {
      if (typeof val === "string") {
        this.setAttribute("style", val);
      }
    },
    configurable: true,
  });

  // removeAttribute
  if (!proto.removeAttribute) {
    proto.removeAttribute = function (name) {
      if (this._attrs) this._attrs.delete(String(name).toLowerCase());
      if (this.setAttribute) this.setAttribute(name, "");
    };
    markNative(proto.removeAttribute, "removeAttribute");
  }

  // getAttributeNames
  if (!proto.getAttributeNames) {
    proto.getAttributeNames = function () {
      const names = [];
      if (this.id) names.push("id");
      if (this.className) names.push("class");
      if (this.hasAttribute && this.hasAttribute("style")) names.push("style");
      if (this._attrs) {
        for (const k of this._attrs.keys()) {
          if (names.indexOf(k) === -1) names.push(k);
        }
      }
      return names;
    };
    markNative(proto.getAttributeNames, "getAttributeNames");
  }

  // toggleAttribute
  if (!proto.toggleAttribute) {
    proto.toggleAttribute = function (name, force) {
      const has = this.hasAttribute(name);
      if (force === true) {
        if (!has) this.setAttribute(name, "");
        return true;
      } else if (force === false) {
        if (has) this.removeAttribute(name);
        return false;
      } else {
        if (has) {
          this.removeAttribute(name);
          return false;
        } else {
          this.setAttribute(name, "");
          return true;
        }
      }
    };
    markNative(proto.toggleAttribute, "toggleAttribute");
  }

  // matches & closest
  if (!proto.matches) {
    proto.matches = function (selector) {
      if (!selector || typeof selector !== "string") return false;
      const s = selector.trim();
      if (s === "*") return true;
      if (s.startsWith("#") && this.id === s.slice(1)) return true;
      if (s.startsWith(".") && this.classList.contains(s.slice(1))) return true;
      if (s.toLowerCase() === this.localName) return true;
      if (typeof document !== "undefined" && document.querySelectorAll) {
        const matches = document.querySelectorAll(s);
        for (let i = 0; i < matches.length; i++) {
          if (matches[i] === this) return true;
        }
      }
      return false;
    };
    markNative(proto.matches, "matches");
  }

  if (!proto.closest) {
    proto.closest = function (selector) {
      let curr = this;
      while (curr && curr.nodeType === ELEMENT_NODE) {
        if (curr.matches && curr.matches(selector)) return curr;
        curr = curr.parentElement;
      }
      return null;
    };
    markNative(proto.closest, "closest");
  }

  // getClientRects
  if (!proto.getClientRects) {
    proto.getClientRects = function () {
      const rect = this.getBoundingClientRect();
      return [rect];
    };
    markNative(proto.getClientRects, "getClientRects");
  }

  // Universal Form & Control Properties on Element
  Object.defineProperty(proto, "name", {
    get: function () { return this.getAttribute("name") || ""; },
    set: function (v) { this.setAttribute("name", String(v)); },
    configurable: true,
  });

  Object.defineProperty(proto, "type", {
    get: function () {
      const tag = (this.tagName || "").toLowerCase();
      const t = this.getAttribute("type");
      if (t) return t.toLowerCase();
      if (tag === "button") return "submit";
      if (tag === "select") return this.multiple ? "select-multiple" : "select-one";
      if (tag === "textarea") return "textarea";
      if (tag === "input") return "text";
      return "";
    },
    set: function (v) { this.setAttribute("type", String(v)); },
    configurable: true,
  });

  Object.defineProperty(proto, "value", {
    get: function () {
      const tag = (this.tagName || "").toLowerCase();
      if (tag === "select") {
        const idx = this.selectedIndex;
        const opts = this.options;
        if (idx >= 0 && idx < opts.length) {
          return opts[idx].value;
        }
        return "";
      }
      if (this._value !== undefined) return this._value;
      if (tag === "checkbox" || tag === "radio") {
        return this.getAttribute("value") ?? "on";
      }
      if (this.hasAttribute && this.hasAttribute("value")) return this.getAttribute("value");
      if (tag === "textarea" || tag === "option") return this.textContent || "";
      return "";
    },
    set: function (v) {
      const tag = (this.tagName || "").toLowerCase();
      if (tag === "select") {
        const str = String(v);
        const opts = this.options;
        for (let i = 0; i < opts.length; i++) {
          if (opts[i].value === str) {
            this.selectedIndex = i;
            return;
          }
        }
        this.selectedIndex = -1;
        return;
      }
      this._value = String(v);
      if (this.setAttribute) this.setAttribute("value", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(proto, "checked", {
    get: function () {
      if (this._checked !== undefined) return this._checked;
      return this.hasAttribute("checked");
    },
    set: function (v) {
      this._checked = Boolean(v);
      if (v) {
        if (this.setAttribute) this.setAttribute("checked", "");
      } else {
        if (this.removeAttribute) this.removeAttribute("checked");
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "selected", {
    get: function () {
      if (this._selected !== undefined) return this._selected;
      return this.hasAttribute("selected");
    },
    set: function (v) {
      this._selected = Boolean(v);
      if (v) {
        if (this.setAttribute) this.setAttribute("selected", "");
      } else {
        if (this.removeAttribute) this.removeAttribute("selected");
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "disabled", {
    get: function () { return this.hasAttribute("disabled"); },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  Object.defineProperty(proto, "required", {
    get: function () { return this.hasAttribute("required"); },
    set: function (v) { if (v) this.setAttribute("required", ""); else this.removeAttribute("required"); },
    configurable: true,
  });

  Object.defineProperty(proto, "readOnly", {
    get: function () { return this.hasAttribute("readonly"); },
    set: function (v) { if (v) this.setAttribute("readonly", ""); else this.removeAttribute("readonly"); },
    configurable: true,
  });

  Object.defineProperty(proto, "placeholder", {
    get: function () { return this.getAttribute("placeholder") || ""; },
    set: function (v) { this.setAttribute("placeholder", String(v)); },
    configurable: true,
  });

  Object.defineProperty(proto, "options", {
    get: function () {
      const opts = [];
      let child = this.firstChild;
      while (child) {
        if ((child.tagName || "").toLowerCase() === "option") {
          opts.push(child);
        }
        child = child.nextSibling;
      }
      return new HTMLCollection(opts);
    },
    configurable: true,
  });

  Object.defineProperty(proto, "selectedIndex", {
    get: function () {
      const opts = this.options;
      for (let i = 0; i < opts.length; i++) {
        if (opts[i].selected) return i;
      }
      return opts.length > 0 && !this.multiple ? 0 : -1;
    },
    set: function (idx) {
      const i = Number(idx);
      const opts = this.options;
      for (let j = 0; j < opts.length; j++) {
        opts[j].selected = (j === i);
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "validity", {
    get: function () { return new ValidityState(this); },
    configurable: true,
  });

  Object.defineProperty(proto, "willValidate", {
    get: function () {
      if (this.disabled || this.readOnly) return false;
      const tag = (this.tagName || "").toLowerCase();
      if (tag === "button") return false;
      const t = this.type;
      return !(t === "hidden" || t === "button" || t === "reset" || t === "submit");
    },
    configurable: true,
  });

  proto.checkValidity = function () {
    if (!this.willValidate) return true;
    if (this.validity && !this.validity.valid) {
      const evt = new Event("invalid", { bubbles: false, cancelable: true });
      this.dispatchEvent(evt);
      return false;
    }
    return true;
  };
  markNative(proto.checkValidity, "checkValidity");

  proto.reportValidity = function () {
    return this.checkValidity();
  };
  markNative(proto.reportValidity, "reportValidity");

  proto.setCustomValidity = function (msg) {
    this._customValidityMessage = String(msg || "");
  };
  markNative(proto.setCustomValidity, "setCustomValidity");
}
