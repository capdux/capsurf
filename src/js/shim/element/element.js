// src/js/shim/element/element.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/element.ts

const STYLE_DECL_MEMBERS = new Set([
  "cssText", "length", "parentRule", "item", "setProperty",
  "getPropertyValue", "getPropertyPriority", "removeProperty"
]);

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
        const rawKey = p.slice(0, colon).trim();
        const k = rawKey.startsWith("--") ? rawKey : rawKey.toLowerCase();
        let v = p.slice(colon + 1).trim();
        let pri = "";
        if (/!important\s*$/i.test(v)) {
          pri = "important";
          v = v.replace(/\s*!important\s*$/i, "").trim();
        }
        map.set(k, { name: rawKey, value: v, priority: pri });
      }
    }
    return map;
  }

  function serializeStyleMap(map) {
    const arr = [];
    for (const [k, entry] of map.entries()) {
      let s = entry.name + ": " + entry.value;
      if (entry.priority) s += " !" + entry.priority;
      arr.push(s);
    }
    const res = arr.join("; ");
    if (res.length > 0) {
      element.setAttribute("style", res);
    } else {
      element.removeAttribute("style");
    }
  }

  const declTarget = Object.create(
    (typeof global !== "undefined" && global.CSSStyleDeclaration && global.CSSStyleDeclaration.prototype) ||
    (typeof globalThis !== "undefined" && globalThis.CSSStyleDeclaration && globalThis.CSSStyleDeclaration.prototype) ||
    Object.prototype
  );

  function propToKebab(p) {
    if (typeof p !== "string") return "";
    if (p.startsWith("--")) return p;
    if (p === "cssFloat") return "float";
    let body = p;
    let prefix = "";
    if (p.startsWith("webkit")) {
      prefix = "-webkit-";
      body = p.slice(6);
    } else if (p.startsWith("Moz")) {
      prefix = "-moz-";
      body = p.slice(3);
    }
    if (body.length > 0) {
      body = body[0].toLowerCase() + body.slice(1);
    }
    return prefix + body.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
  }

  return new Proxy(declTarget, {
    get: function (target, prop, receiver) {
      if (typeof prop === "symbol") {
        if (prop === Symbol.iterator) {
          return function* () {
            const map = parseStyleMap();
            for (const entry of map.values()) yield entry.name;
          };
        }
        return target[prop];
      }
      if (typeof prop !== "string") return undefined;
      if (prop === "cssText") return element.getAttribute("style") || "";
      if (prop === "length") return parseStyleMap().size;
      if (prop === "parentRule") return null;
      if (prop === "item") {
        return function (idx) {
          const i = Number(idx) >>> 0;
          const map = parseStyleMap();
          let count = 0;
          for (const entry of map.values()) {
            if (count === i) return entry.name;
            count++;
          }
          return "";
        };
      }
      if (prop === "getPropertyValue") {
        return function (p) {
          const map = parseStyleMap();
          const str = String(p);
          const key = str.startsWith("--") ? str : str.toLowerCase();
          const entry = map.get(key);
          if (entry) return entry.value;
          if (key === "css-float" || key === "cssfloat") {
            const e = map.get("float");
            if (e) return e.value;
          } else if (key === "float") {
            const e = map.get("css-float");
            if (e) return e.value;
          } else if (key === "-webkit-transform") {
            const e = map.get("transform");
            if (e) return e.value;
          } else if (key === "transform") {
            const e = map.get("-webkit-transform");
            if (e) return e.value;
          } else if (key === "-webkit-align-content") {
            const e = map.get("align-content");
            if (e) return e.value;
          } else if (key === "align-content") {
            const e = map.get("-webkit-align-content");
            if (e) return e.value;
          }
          return "";
        };
      }
      if (prop === "getPropertyPriority") {
        return function (p) {
          const map = parseStyleMap();
          const str = String(p);
          const key = str.startsWith("--") ? str : str.toLowerCase();
          const entry = map.get(key);
          return entry ? entry.priority : "";
        };
      }
      if (prop === "setProperty") {
        return function (p, val, priority) {
          if (priority !== undefined && priority !== null && priority !== "") {
            if (priority !== "important" && priority !== "!important") return;
          }
          const pPriority = (priority === "important" || priority === "!important") ? "important" : "";
          const map = parseStyleMap();
          let str = String(p);
          if (str.toLowerCase() === "cssfloat" || str.toLowerCase() === "css-float") {
            str = "float";
          }
          const key = str.startsWith("--") ? str : str.toLowerCase();
          map.set(key, { name: str, value: String(val), priority: pPriority });
          if (key === "display") {
            element._displayNone = (String(val).trim().toLowerCase() === "none");
          }
          serializeStyleMap(map);
        };
      }
      if (prop === "removeProperty") {
        return function (p) {
          const map = parseStyleMap();
          let str = String(p);
          if (str.toLowerCase() === "cssfloat" || str.toLowerCase() === "css-float") {
            str = "float";
          }
          const key = str.startsWith("--") ? str : str.toLowerCase();
          const old = map.get(key);
          map.delete(key);
          if (key === "float") map.delete("css-float");
          if (key === "transform") map.delete("-webkit-transform");
          if (key === "-webkit-transform") map.delete("transform");
          if (key === "display") {
            element._displayNone = false;
          }
          serializeStyleMap(map);
          return old ? old.value : "";
        };
      }
      if (prop in target) return target[prop];

      if (prop === "cssFloat" || prop === "float") {
        const map = parseStyleMap();
        const e = map.get("float") || map.get("css-float");
        return e ? e.value : "";
      }
      if (prop === "webkitTransform") {
        const map = parseStyleMap();
        const e = map.get("-webkit-transform") || map.get("transform");
        return e ? e.value : "";
      }
      if (prop === "transform") {
        const map = parseStyleMap();
        const e = map.get("transform") || map.get("-webkit-transform");
        return e ? e.value : "";
      }

      // CamelCase or kebab-case lookup
      const map = parseStyleMap();
      if (map.has(prop)) return map.get(prop).value;
      const kebab = propToKebab(prop);
      if (map.has(kebab)) return map.get(kebab).value;
      return "";
    },
    set: function (target, prop, value) {
      if (typeof prop === "symbol") {
        target[prop] = value;
        return true;
      }
      if (typeof prop !== "string") return true;
      if (prop === "cssText") {
        const valStr = String(value);
        element.setAttribute("style", valStr);
        element._displayNone = /(?:^|;)\s*display\s*:\s*none\b/i.test(valStr);
        return true;
      }
      if (prop === "cssFloat" || prop === "float") {
        const map = parseStyleMap();
        map.delete("css-float");
        if (value === null || value === undefined || value === "") {
          map.delete("float");
        } else {
          map.set("float", { name: "float", value: String(value), priority: "" });
        }
        serializeStyleMap(map);
        return true;
      }
      let key = propToKebab(prop);
      if (key === "css-float") key = "float";
      const map = parseStyleMap();
      const normKey = key.startsWith("--") ? key : key.toLowerCase();
      if (value === null || value === undefined || value === "") {
        map.delete(normKey);
      } else {
        map.set(normKey, { name: key, value: String(value), priority: "" });
      }
      if (normKey === "display") {
        element._displayNone = (value !== null && value !== undefined && String(value).trim().toLowerCase() === "none");
      }
      serializeStyleMap(map);
      return true;
    },
    has: function (target, prop) {
      if (typeof prop === "symbol") return prop === Symbol.iterator || prop in target;
      const pStr = String(prop);
      if (pStr === "cssFloat" || pStr === "float") return true;
      if (STYLE_DECL_MEMBERS.has(pStr)) return true;
      if (prop in target) return true;
      if (pStr.startsWith("--")) return false;
      if (/^(?:webkit|moz|ms|o)-/.test(pStr)) return false;
      if (!pStr.includes("-") && /^(?:moz|ms|o)[A-Z]/.test(pStr)) return false;
      if (typeof global !== "undefined" && typeof global.__isCssPropertySupported === "function") {
        return global.__isCssPropertySupported(pStr);
      }
      if (typeof globalThis !== "undefined" && typeof globalThis.__isCssPropertySupported === "function") {
        return globalThis.__isCssPropertySupported(pStr);
      }
      return false;
    },
    ownKeys: function (target) {
      const map = parseStyleMap();
      const keys = [];
      const seen = new Set();
      for (const [k, entry] of map.entries()) {
        const name = entry && entry.name ? entry.name : k;
        if (!seen.has(name)) {
          seen.add(name);
          keys.push(name);
        }
      }
      const targetKeys = Reflect.ownKeys(target);
      for (let i = 0; i < targetKeys.length; i++) {
        const k = targetKeys[i];
        if (!seen.has(k)) {
          seen.add(k);
          keys.push(k);
        }
      }
      return keys;
    },
    getOwnPropertyDescriptor: function (target, prop) {
      const own = Reflect.getOwnPropertyDescriptor(target, prop);
      if (own) return own;
      if (typeof prop === "string") {
        const map = parseStyleMap();
        if (map.has(prop)) {
          return { value: map.get(prop).value, writable: true, enumerable: true, configurable: true };
        }
        const lower = prop.startsWith("--") ? prop : prop.toLowerCase();
        if (map.has(lower)) {
          return { value: map.get(lower).value, writable: true, enumerable: true, configurable: true };
        }
      }
      return undefined;
    },
    preventExtensions: function () {
      return false;
    }
  });
}

function isHtmlElement(el) {
  if (!el) return true;
  if (el._namespaceURI !== undefined) {
    return el._namespaceURI === null || el._namespaceURI === "http://www.w3.org/1999/xhtml";
  }
  let curr = el;
  while (curr) {
    if (curr._namespaceURI !== undefined) {
      return curr._namespaceURI === null || curr._namespaceURI === "http://www.w3.org/1999/xhtml";
    }
    const tag = String(curr.localName || curr.tagName || "").toLowerCase();
    if (tag === "svg" || tag === "math") return false;
    curr = curr.parentElement || curr.parentNode;
  }
  return true;
}

function installElementExtensions(proto) {
  if (!proto) return;

  // Make Element inherit from Node / EventTarget
  Object.setPrototypeOf(proto, Node.prototype);
  proto.appendChild = Node.prototype.appendChild;
  proto.insertBefore = Node.prototype.insertBefore;
  proto.removeChild = Node.prototype.removeChild;

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

  Object.defineProperty(proto, "prefix", {
    get: function () {
      if (this._prefix !== undefined) return this._prefix;
      const tag = this.tagName || "";
      const colon = tag.indexOf(":");
      return colon !== -1 ? tag.slice(0, colon) : null;
    },
    configurable: true,
  });

  Object.defineProperty(proto, "namespaceURI", {
    get: function () {
      if (this._namespaceURI !== undefined) return this._namespaceURI;
      if (this.getAttribute) {
        const xmlns = this.getAttribute("xmlns");
        if (xmlns !== null && xmlns !== undefined && xmlns !== "") return xmlns;
      }
      let curr = this;
      while (curr) {
        const tag = String(curr.localName || curr.tagName || "").toLowerCase();
        if (tag === "svg") return "http://www.w3.org/2000/svg";
        if (tag === "math") return "http://www.w3.org/1998/Math/MathML";
        curr = curr.parentElement || curr.parentNode;
      }
      const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
      if (!doc || !doc.contentType || doc.contentType === "text/html") {
        return "http://www.w3.org/1999/xhtml";
      }
      return null;
    },
    set: function (v) {
      this._namespaceURI = v;
    },
    configurable: true,
  });

  Object.defineProperty(proto, "data", {
    get: function () { return this.textContent || ""; },
    set: function (v) {
      this.textContent = String(v);
      if (typeof notifyMutation === "function") {
        notifyMutation(new MutationRecord("characterData", this));
      }
    },
    configurable: true,
  });

  if (!proto.lookupNamespaceURI) {
    proto.lookupNamespaceURI = Node.prototype.lookupNamespaceURI;
  }

  if (!proto.append) {
    proto.append = function (...nodes) {
      for (const n of nodes) {
        let node;
        if (n && typeof n === "object" && typeof n.nodeType === "number") {
          node = n;
        } else if (typeof document !== "undefined" && document.createTextNode) {
          node = document.createTextNode(String(n));
        } else {
          node = new Text(String(n));
        }
        this.appendChild(node);
      }
    };
    markNative(proto.append, "append");
  }

  if (!proto.remove) {
    proto.remove = function () {
      const parent = this.parentElement || this.parentNode;
      if (parent && parent.removeChild) {
        parent.removeChild(this);
      }
    };
    markNative(proto.remove, "remove");
  }

  // Children and Element Traversal ([SameObject] semantics)
  Object.defineProperty(proto, "children", {
    get: function () {
      return getElementChildrenCollection(this);
    },
    configurable: true,
  });

  Object.defineProperty(proto, "firstElementChild", {
    get: function () {
      const kids = this.children;
      return (kids && kids.length > 0) ? kids[0] : null;
    },
    configurable: true,
  });

  Object.defineProperty(proto, "lastElementChild", {
    get: function () {
      const kids = this.children;
      return (kids && kids.length > 0) ? kids[kids.length - 1] : null;
    },
    configurable: true,
  });

  Object.defineProperty(proto, "childElementCount", {
    get: function () {
      const kids = this.children;
      return kids ? kids.length : 0;
    },
    configurable: true,
  });

  Object.defineProperty(proto, "previousElementSibling", {
    get: function () {
      let sib = this.previousSibling;
      while (sib) {
        if (sib.nodeType === 1) return sib;
        sib = sib.previousSibling;
      }
      return null;
    },
    configurable: true,
  });

  Object.defineProperty(proto, "nextElementSibling", {
    get: function () {
      let sib = this.nextSibling;
      while (sib) {
        if (sib.nodeType === 1) return sib;
        sib = sib.nextSibling;
      }
      return null;
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

  // Internal attribute synchronization
  proto._syncAttrs = function () {
    if (!this._attrs) {
      this._attrs = new Map();
      this._attrList = [];
      const isHtml = isHtmlElement(this);
      if (typeof this._getRawAttrs === "function") {
        const raw = this._getRawAttrs();
        if (raw && Array.isArray(raw)) {
          for (let i = 0; i < raw.length; i++) {
            const item = raw[i];
            if (item && item.name) {
              const colon = item.name.indexOf(":");
              const prefix = colon !== -1 ? item.name.slice(0, colon) : null;
              const localName = colon !== -1 ? item.name.slice(colon + 1) : item.name;
              let ns = null;
              if (prefix === "xml") ns = "http://www.w3.org/XML/1998/namespace";
              else if (prefix === "xmlns" || item.name === "xmlns") ns = "http://www.w3.org/2000/xmlns/";
              else if (prefix === "xlink") ns = "http://www.w3.org/1999/xlink";
              const entry = {
                name: item.name,
                value: item.value || "",
                namespaceURI: ns,
                localName: localName,
                prefix: prefix,
              };
              this._attrList.push(entry);
              const mapKey = isHtml ? item.name.toLowerCase() : item.name;
              this._attrs.set(mapKey, entry);
            }
          }
        }
      }
      if (this.id && !this._attrs.has("id")) {
        const entry = { name: "id", value: this.id, namespaceURI: null, localName: "id", prefix: null };
        this._attrList.push(entry);
        this._attrs.set("id", entry);
      }
      if (this.className && typeof this.className === "string" && !this._attrs.has("class")) {
        const entry = { name: "class", value: this.className, namespaceURI: null, localName: "class", prefix: null };
        this._attrList.push(entry);
        this._attrs.set("class", entry);
      }
      if (this.hasAttribute && this.hasAttribute("style") && !this._attrs.has("style")) {
        const entry = { name: "style", value: this.getAttribute("style") || "", namespaceURI: null, localName: "style", prefix: null };
        this._attrList.push(entry);
        this._attrs.set("style", entry);
      }
    }
  };

  // attributes getter returning live NamedNodeMap
  Object.defineProperty(proto, "attributes", {
    get: function () {
      if (!this._attributesMap) {
        this._attributesMap = new NamedNodeMap(this);
      } else {
        this._attributesMap._sync();
      }
      return this._attributesMap;
    },
    configurable: true,
  });

  // setAttribute
  const origSetAttribute = proto.setAttribute;
  proto.setAttribute = function (name, value) {
    const isHtml = isHtmlElement(this);
    const key = isHtml ? String(name).toLowerCase() : String(name);
    const val = String(value);
    if (!this._attrs) this._syncAttrs();
    let existing = null;
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const match = isHtml
          ? this._attrList[i].name.toLowerCase() === key
          : this._attrList[i].name === key;
        if (match) {
          existing = this._attrList[i];
          break;
        }
      }
    }
    const oldVal = existing ? existing.value : (this._attrs.get(key)?.value ?? null);
    if (existing) {
      existing.value = val;
    } else {
      const colon = key.indexOf(":");
      const prefix = colon !== -1 ? key.slice(0, colon) : null;
      const localName = colon !== -1 ? key.slice(colon + 1) : key;
      let ns = null;
      if (prefix === "xml") ns = "http://www.w3.org/XML/1998/namespace";
      else if (prefix === "xmlns" || key === "xmlns") ns = "http://www.w3.org/2000/xmlns/";
      else if (prefix === "xlink") ns = "http://www.w3.org/1999/xlink";
      const entry = {
        name: key,
        value: val,
        namespaceURI: ns,
        localName: localName,
        prefix: prefix,
      };
      if (!this._attrList) this._attrList = [];
      this._attrList.push(entry);
    }
    this._attrs.set(key, { name: key, value: val });
    if (key.toLowerCase() === "async") {
      if (typeof globalThis !== "undefined" && globalThis.__capsurf_clearScriptForceAsync) {
        globalThis.__capsurf_clearScriptForceAsync(this);
      }
    }
    if (key.toLowerCase() === "id") {
      this.id = val;
      if (typeof val === "string" && val.length > 0 && /^[a-zA-Z_$][a-zA-Z0-9_$]*$/.test(val)) {
        try {
          const idVal = val;
          const desc = typeof globalThis !== "undefined" ? Object.getOwnPropertyDescriptor(globalThis, idVal) : null;
          if (!desc || desc.configurable) {
            Object.defineProperty(globalThis, idVal, {
              get: function () { return typeof document !== "undefined" && document.getElementById ? document.getElementById(idVal) : null; },
              set: function (v) {
                Object.defineProperty(globalThis, idVal, {
                  value: v,
                  writable: true,
                  enumerable: true,
                  configurable: true,
                });
              },
              configurable: true,
              enumerable: true,
            });
            if (typeof window !== "undefined" && window !== globalThis) {
              Object.defineProperty(window, idVal, {
                get: function () { return typeof document !== "undefined" && document.getElementById ? document.getElementById(idVal) : null; },
                set: function (v) {
                  Object.defineProperty(window, idVal, {
                    value: v,
                    writable: true,
                    enumerable: true,
                    configurable: true,
                  });
                },
                configurable: true,
                enumerable: true,
              });
            }
          }
        } catch (e) {}
      }
    } else if (key.toLowerCase() === "style") {
      this._displayNone = /(?:^|;)\s*display\s*:\s*none\b/i.test(val);
    } else if (key.toLowerCase() === "class" && typeof this.className === "string") {
      this.className = val;
    } else if (key.toLowerCase() === "selected" && (this.tagName || "").toLowerCase() === "option") {
      this._selected = true;
      let curr = this.parentElement || this.parentNode;
      while (curr) {
        if ((curr.tagName || "").toLowerCase() === "select") {
          if (!curr.multiple) {
            curr._selectedIndex = undefined;
            const opts = curr.options;
            for (let i = 0; i < opts.length; i++) {
              if (opts[i] !== this && (!this._nid || opts[i]._nid !== this._nid)) {
                opts[i]._selected = false;
              }
            }
          }
          break;
        }
        curr = curr.parentElement || curr.parentNode;
      }
    }
    if (origSetAttribute) {
      origSetAttribute.call(this, key, val);
    }
    if (this._attributesMap) {
      this._attributesMap._sync();
    }
    if (typeof notifyMutation === "function") {
      const rec = new MutationRecord("attributes", this);
      rec.attributeName = key;
      rec.oldValue = oldVal;
      notifyMutation(rec);
    }
    if (typeof notifyCustomElementAttributeChanged === "function") {
      notifyCustomElementAttributeChanged(this, key, oldVal, val, null);
    }
  };
  markNative(proto.setAttribute, "setAttribute");

  // removeAttribute
  const origRemoveAttribute = proto.removeAttribute;
  proto.removeAttribute = function (name) {
    const isHtml = isHtmlElement(this);
    const key = isHtml ? String(name).toLowerCase() : String(name);
    if (!this._attrs) this._syncAttrs();
    let oldVal = null;
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const match = isHtml
          ? this._attrList[i].name.toLowerCase() === key
          : this._attrList[i].name === key;
        if (match) {
          oldVal = this._attrList[i].value;
          this._attrList.splice(i, 1);
          break;
        }
      }
    }
    const oldEntry = this._attrs.get(key);
    if (!oldVal && oldEntry) oldVal = oldEntry.value;
    this._attrs.delete(key);
    if (key.toLowerCase() === "async") {
      if (typeof globalThis !== "undefined" && globalThis.__capsurf_clearScriptForceAsync) {
        globalThis.__capsurf_clearScriptForceAsync(this);
      }
    }
    if (key.toLowerCase() === "id") {
      this.id = "";
    } else if (key.toLowerCase() === "style") {
      this._displayNone = false;
    } else if (key.toLowerCase() === "class" && typeof this.className === "string") {
      this.className = "";
    } else if (key.toLowerCase() === "selected" && (this.tagName || "").toLowerCase() === "option") {
      this._selected = false;
      let curr = this.parentElement || this.parentNode;
      while (curr) {
        if ((curr.tagName || "").toLowerCase() === "select") {
          if (!curr.multiple) {
            curr._selectedIndex = undefined;
          }
          break;
        }
        curr = curr.parentElement || curr.parentNode;
      }
    }

    if (origRemoveAttribute) {
      origRemoveAttribute.call(this, name);
    }
    if (this._attributesMap) {
      this._attributesMap._sync();
    }
    if (typeof notifyMutation === "function") {
      const rec = new MutationRecord("attributes", this);
      rec.attributeName = name;
      rec.oldValue = oldVal;
      notifyMutation(rec);
    }
    if (typeof notifyCustomElementAttributeChanged === "function") {
      notifyCustomElementAttributeChanged(this, key, oldVal, null, null);
    }
  };
  markNative(proto.removeAttribute, "removeAttribute");

  // removeAttributeNode
  proto.removeAttributeNode = function (attr) {
    if (!attr || !attr.name) return null;
    if (attr.ownerElement && attr.ownerElement !== this) {
      const err = new Error("NotFoundError: Attr does not belong to this element");
      err.name = "NotFoundError";
      throw err;
    }
    this.removeAttribute(attr.name);
    attr.ownerElement = null;
    return attr;
  };
  markNative(proto.removeAttributeNode, "removeAttributeNode");

  // getAttributeNode & setAttributeNode
  proto.getAttributeNode = function (name) {
    if (!this._attributesMap) this._attributesMap = new NamedNodeMap(this);
    return this._attributesMap.getNamedItem(name);
  };
  markNative(proto.getAttributeNode, "getAttributeNode");

  proto.getAttributeNodeNS = function (namespace, localName) {
    if (!this._attributesMap) this._attributesMap = new NamedNodeMap(this);
    return this._attributesMap.getNamedItemNS(namespace, localName);
  };
  markNative(proto.getAttributeNodeNS, "getAttributeNodeNS");

  proto.setAttributeNode = function (attr) {
    if (!attr || !attr.name) throw new TypeError("Attr required");
    if (!this._attributesMap) this._attributesMap = new NamedNodeMap(this);
    return this._attributesMap.setNamedItem(attr);
  };
  markNative(proto.setAttributeNode, "setAttributeNode");

  // getAttribute
  const origGetAttribute = proto.getAttribute;
  proto.getAttribute = function (name) {
    const isHtml = isHtmlElement(this);
    const key = isHtml ? String(name).toLowerCase() : String(name);
    if (!this._attrs) this._syncAttrs();
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const match = isHtml
          ? this._attrList[i].name.toLowerCase() === key
          : this._attrList[i].name === key;
        if (match) {
          return this._attrList[i].value;
        }
      }
    }
    if (this._attrs) {
      const entry = this._attrs.get(key);
      if (entry) return entry.value;
      if (isHtml && origGetAttribute) {
        const val = origGetAttribute.call(this, name);
        if (val !== null) return val;
      }
      return null;
    }
    if (isHtml && origGetAttribute) {
      return origGetAttribute.call(this, name);
    }
    return null;
  };
  markNative(proto.getAttribute, "getAttribute");

  // hasAttribute
  const origHasAttribute = proto.hasAttribute;
  proto.hasAttribute = function (name) {
    const isHtml = isHtmlElement(this);
    const key = isHtml ? String(name).toLowerCase() : String(name);
    if (!this._attrs) this._syncAttrs();
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const match = isHtml
          ? this._attrList[i].name.toLowerCase() === key
          : this._attrList[i].name === key;
        if (match) {
          return true;
        }
      }
    }
    if (this._attrs) {
      if (this._attrs.has(key)) return true;
      if (isHtml && origHasAttribute) return origHasAttribute.call(this, name);
      return false;
    }
    if (isHtml && origHasAttribute) {
      return origHasAttribute.call(this, name);
    }
    return false;
  };
  markNative(proto.hasAttribute, "hasAttribute");

  // getAttributeNames
  proto.getAttributeNames = function () {
    if (!this._attrs) this._syncAttrs();
    const names = [];
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        names.push(this._attrList[i].name);
      }
    } else {
      for (const entry of this._attrs.values()) {
        names.push(entry.name);
      }
    }
    return names;
  };
  markNative(proto.getAttributeNames, "getAttributeNames");

  // getAttributeNS
  proto.getAttributeNS = function (namespace, localName) {
    if (typeof localName !== "string") return null;
    if (!this._attrs) this._syncAttrs();
    const ns = (namespace === undefined || namespace === null || namespace === "") ? null : String(namespace);
    const loc = String(localName).toLowerCase();
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const item = this._attrList[i];
        const itemNs = item.namespaceURI || null;
        const itemLoc = (item.localName || item.name).toLowerCase();
        if (itemNs === ns && itemLoc === loc) {
          return item.value;
        }
      }
      if (ns === null) {
        for (let i = 0; i < this._attrList.length; i++) {
          const item = this._attrList[i];
          if ((item.namespaceURI === null || item.namespaceURI === undefined) &&
              item.name.toLowerCase() === loc) {
            return item.value;
          }
        }
      }
      return null;
    }
    return null;
  };
  markNative(proto.getAttributeNS, "getAttributeNS");

  // hasAttributeNS
  proto.hasAttributeNS = function (namespace, localName) {
    return this.getAttributeNS(namespace, localName) !== null;
  };
  markNative(proto.hasAttributeNS, "hasAttributeNS");

  // setAttributeNS
  proto.setAttributeNS = function (namespace, qualifiedName, value) {
    const qname = String(qualifiedName);
    const val = String(value);
    const ns = (namespace === undefined || namespace === null || namespace === "") ? null : String(namespace);
    const colon = qname.indexOf(":");
    const prefix = colon !== -1 ? qname.slice(0, colon) : null;
    const localName = colon !== -1 ? qname.slice(colon + 1) : qname;

    if (!this._attrs) this._syncAttrs();

    let existing = null;
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const item = this._attrList[i];
        const itemNs = item.namespaceURI || null;
        const itemLoc = (item.localName || item.name).toLowerCase();
        if (itemNs === ns && itemLoc === localName.toLowerCase()) {
          existing = item;
          break;
        }
      }
    }

    const oldVal = existing ? existing.value : null;

    if (existing) {
      existing.value = val;
      existing.name = qname;
      existing.prefix = prefix;
    } else {
      const newEntry = {
        name: qname,
        value: val,
        namespaceURI: ns,
        localName: localName,
        prefix: prefix,
      };
      if (!this._attrList) this._attrList = [];
      this._attrList.push(newEntry);
    }

    this._attrs.set(qname.toLowerCase(), {
      name: qname,
      value: val,
      namespaceURI: ns,
      localName: localName,
      prefix: prefix,
    });

    if (qname.toLowerCase() === "async" || (localName && String(localName).toLowerCase() === "async")) {
      if (typeof globalThis !== "undefined" && globalThis.__capsurf_clearScriptForceAsync) {
        globalThis.__capsurf_clearScriptForceAsync(this);
      }
    }

    if (qname.toLowerCase() === "id" && ns === null) {
      this.id = val;
    } else if (qname.toLowerCase() === "class" && ns === null) {
      this.className = val;
    }

    if (origSetAttribute) {
      origSetAttribute.call(this, qname, val);
    }
    if (this._attributesMap) {
      this._attributesMap._sync();
    }
    if (typeof notifyMutation === "function") {
      const rec = new MutationRecord("attributes", this);
      rec.attributeName = qname;
      rec.attributeNamespace = ns;
      rec.oldValue = oldVal;
      notifyMutation(rec);
    }
    if (typeof notifyCustomElementAttributeChanged === "function") {
      notifyCustomElementAttributeChanged(this, localName, oldVal, val, ns);
    }
  };
  markNative(proto.setAttributeNS, "setAttributeNS");

  // removeAttributeNS
  proto.removeAttributeNS = function (namespace, localName) {
    if (typeof localName !== "string" || localName.length === 0) return;
    if (!this._attrs) this._syncAttrs();
    const ns = (namespace === undefined || namespace === null || namespace === "") ? null : String(namespace);
    const loc = String(localName).toLowerCase();

    let removed = null;
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const item = this._attrList[i];
        const itemNs = item.namespaceURI || null;
        const itemLoc = (item.localName || item.name).toLowerCase();
        if (itemNs === ns && itemLoc === loc) {
          removed = item;
          this._attrList.splice(i, 1);
          break;
        }
      }
      if (!removed && ns === null) {
        for (let i = 0; i < this._attrList.length; i++) {
          const item = this._attrList[i];
          if ((item.namespaceURI === null || item.namespaceURI === undefined) &&
              item.name.toLowerCase() === loc) {
            removed = item;
            this._attrList.splice(i, 1);
            break;
          }
        }
      }
    }

    const oldVal = removed ? removed.value : null;
    const removedName = removed ? removed.name : localName;

    this._attrs.delete(removedName.toLowerCase());
    this._attrs.delete(localName.toLowerCase());

    if (removedName.toLowerCase() === "id" && ns === null) {
      this.id = "";
    } else if (removedName.toLowerCase() === "class" && ns === null) {
      this.className = "";
    }

    if (loc === "async") {
      if (typeof globalThis !== "undefined" && globalThis.__capsurf_clearScriptForceAsync) {
        globalThis.__capsurf_clearScriptForceAsync(this);
      }
    }

    if (origRemoveAttribute) {
      origRemoveAttribute.call(this, removedName);
    }
    if (this._attributesMap) {
      this._attributesMap._sync();
    }
    if (typeof notifyMutation === "function") {
      const rec = new MutationRecord("attributes", this);
      rec.attributeName = removedName;
      rec.attributeNamespace = ns;
      rec.oldValue = oldVal;
      notifyMutation(rec);
    }
    if (typeof notifyCustomElementAttributeChanged === "function") {
      notifyCustomElementAttributeChanged(this, loc, oldVal, null, ns);
    }
  };
  markNative(proto.removeAttributeNS, "removeAttributeNS");

  // Wrap id & className accessors if present
  const idDesc = Object.getOwnPropertyDescriptor(proto, "id");
  if (idDesc && idDesc.set) {
    const origIdSet = idDesc.set;
    Object.defineProperty(proto, "id", {
      get: idDesc.get,
      set: function (v) {
        origIdSet.call(this, v);
        const str = String(v);
        if (!this._attrs) this._syncAttrs();
        if (str) {
          const entry = { name: "id", value: str, namespaceURI: null, localName: "id", prefix: null };
          this._attrs.set("id", entry);
          if (this._attrList) {
            let found = false;
            for (let i = 0; i < this._attrList.length; i++) {
              if (this._attrList[i].name.toLowerCase() === "id") {
                this._attrList[i].value = str;
                found = true;
                break;
              }
            }
            if (!found) this._attrList.push(entry);
          }
        } else {
          this._attrs.delete("id");
          if (this._attrList) {
            for (let i = 0; i < this._attrList.length; i++) {
              if (this._attrList[i].name.toLowerCase() === "id") {
                this._attrList.splice(i, 1);
                break;
              }
            }
          }
        }
        if (this._attributesMap) this._attributesMap._sync();
      },
      configurable: true,
    });
  }
  const clsDesc = Object.getOwnPropertyDescriptor(proto, "className");
  const origClsGet = clsDesc ? clsDesc.get : null;
  const origClsSet = clsDesc ? clsDesc.set : null;
  Object.defineProperty(proto, "className", {
    get: function () {
      if (!isHtmlElement(this)) {
        if (!this._svgClassName) {
          const animStr = Object.create(SVGAnimatedString.prototype);
          animStr._element = this;
          this._svgClassName = animStr;
        }
        return this._svgClassName;
      }
      if (origClsGet) return origClsGet.call(this);
      return this.getAttribute("class") || "";
    },
    set: function (v) {
      if (!isHtmlElement(this) && typeof v === "object" && v !== null && v.baseVal !== undefined) {
        v = v.baseVal;
      }
      if (origClsSet) {
        origClsSet.call(this, v);
      }
      const str = String(v);
      if (!this._attrs) this._syncAttrs();
      if (str) {
        const entry = { name: "class", value: str, namespaceURI: null, localName: "class", prefix: null };
        this._attrs.set("class", entry);
        if (this._attrList) {
          let found = false;
          for (let i = 0; i < this._attrList.length; i++) {
            if (this._attrList[i].name.toLowerCase() === "class") {
              this._attrList[i].value = str;
              found = true;
              break;
            }
          }
          if (!found) this._attrList.push(entry);
        }
      } else {
        this._attrs.delete("class");
        if (this._attrList) {
          for (let i = 0; i < this._attrList.length; i++) {
            if (this._attrList[i].name.toLowerCase() === "class") {
              this._attrList.splice(i, 1);
              break;
            }
          }
        }
      }
      if (this._attributesMap) this._attributesMap._sync();
    },
    configurable: true,
  });

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

  // Directionality helpers (WHATWG HTML §3.2.6.4 + Shadow DOM + Selectors 4 :dir())
  function isStrongRtlCodePoint(cp) {
    return (
      (cp >= 0x0590 && cp <= 0x05FF) || // Hebrew
      (cp >= 0x0600 && cp <= 0x06FF) || // Arabic
      (cp >= 0x0700 && cp <= 0x074F) || // Syriac
      (cp >= 0x0750 && cp <= 0x077F) || // Arabic Supplement
      (cp >= 0x0780 && cp <= 0x07BF) || // Thaana
      (cp >= 0x07C0 && cp <= 0x07FF) || // NKo
      (cp >= 0x0800 && cp <= 0x085F) || // Samaritan / Mandaic
      (cp >= 0x08A0 && cp <= 0x08FF) || // Arabic Extended-A
      (cp >= 0xFB1D && cp <= 0xFDFF) || // Hebrew/Arabic Presentation Forms-A
      (cp >= 0xFE70 && cp <= 0xFEFF) || // Arabic Presentation Forms-B
      (cp >= 0x10800 && cp <= 0x10FFF) || // Ancient RTL scripts
      (cp >= 0x1E800 && cp <= 0x1EFFF) // Adlam / Arabic Math
    );
  }

  function isStrongLtrCodePoint(cp) {
    return (
      (cp >= 0x0041 && cp <= 0x005A) || // A-Z
      (cp >= 0x0061 && cp <= 0x007A) || // a-z
      (cp >= 0x00C0 && cp <= 0x00D6) ||
      (cp >= 0x00D8 && cp <= 0x00F6) ||
      (cp >= 0x00F8 && cp <= 0x02B8) || // Latin Extended
      (cp >= 0x0370 && cp <= 0x058F) || // Greek, Cyrillic, Armenian
      (cp >= 0x0900 && cp <= 0x1FFF) || // Indic, SE Asian, etc.
      (cp >= 0x2C00 && cp <= 0x2DFF) ||
      (cp >= 0x3040 && cp <= 0xA4CF) || // CJK, Hiragana, Katakana
      (cp >= 0xAC00 && cp <= 0xD7AF) // Hangul
    );
  }

  function firstStrongDirectionInText(str) {
    if (!str || typeof str !== "string") return null;
    for (let i = 0; i < str.length; i++) {
      const cp = str.codePointAt(i);
      if (cp > 0xFFFF) i++;
      if (isStrongRtlCodePoint(cp)) return "rtl";
      if (isStrongLtrCodePoint(cp)) return "ltr";
    }
    return null;
  }

  function getElementDirAttr(el) {
    if (!el || el.nodeType !== 1 || typeof el.getAttribute !== "function") return "";
    const raw = el.getAttribute("dir");
    if (!raw) return "";
    const v = String(raw).trim().toLowerCase();
    if (v === "ltr" || v === "rtl" || v === "auto") return v;
    return "";
  }

  function getLightChildren(node) {
    if (!node) return [];
    if (typeof ensureChildren === "function") return ensureChildren(node);
    if (node._children) return node._children;
    if (node.childNodes) return Array.from(node.childNodes);
    return [];
  }

  function findSlotForLightChild(host, child) {
    if (!host || !host._shadowRoot) return null;
    const slotName = (child && child.nodeType === 1 && typeof child.getAttribute === "function")
      ? (child.getAttribute("slot") || "")
      : "";
    function walk(root) {
      const kids = getLightChildren(root);
      for (let i = 0; i < kids.length; i++) {
        const k = kids[i];
        if (!k || k.nodeType !== 1) continue;
        const t = String(k.localName || k.tagName || "").toLowerCase();
        if (t === "slot") {
          const name = (typeof k.getAttribute === "function" ? k.getAttribute("name") : "") || "";
          if (name === slotName) return k;
        }
        const nested = walk(k);
        if (nested) return nested;
      }
      return null;
    }
    return walk(host._shadowRoot);
  }

  function getSlotAssignedNodes(slotEl) {
    let cur = slotEl;
    let shadowRoot = null;
    while (cur) {
      if (cur.nodeType === 11 && cur.host) {
        shadowRoot = cur;
        break;
      }
      cur = cur.parentNode || cur.parentElement;
    }
    if (!shadowRoot || !shadowRoot.host) return [];
    const host = shadowRoot.host;
    const targetName = (typeof slotEl.getAttribute === "function" ? slotEl.getAttribute("name") : "") || "";
    const hostKids = getLightChildren(host);
    const assigned = [];
    for (let i = 0; i < hostKids.length; i++) {
      const c = hostKids[i];
      if (!c) continue;
      if (c.nodeType === 1) {
        const cSlot = (typeof c.getAttribute === "function" ? c.getAttribute("slot") : "") || "";
        if (cSlot === targetName) assigned.push(c);
      } else if (c.nodeType === 3) {
        if (targetName === "") assigned.push(c);
      }
    }
    return assigned;
  }

  function scanSubtreeForFirstStrong(node, isRoot, followSlotAssigned) {
    if (!node) return null;
    if (node.nodeType === 3) {
      const txt = node.data !== undefined ? node.data : (node.textContent || "");
      return firstStrongDirectionInText(txt);
    }
    if (node.nodeType !== 1 && node.nodeType !== 11) return null;
    if (!isRoot && node.nodeType === 1) {
      const tag = String(node.localName || node.tagName || "").toLowerCase();
      if (tag === "bdi" || tag === "script" || tag === "style" || tag === "textarea") {
        return null;
      }
      if (getElementDirAttr(node) !== "") {
        return null;
      }
    }
    if (node.nodeType === 1) {
      const tag = String(node.localName || node.tagName || "").toLowerCase();
      if (tag === "slot" && followSlotAssigned) {
        const assigned = getSlotAssignedNodes(node);
        if (assigned.length > 0) {
          for (let i = 0; i < assigned.length; i++) {
            const d = scanSubtreeForFirstStrong(assigned[i], false, followSlotAssigned);
            if (d) return d;
          }
          return null;
        }
      }
    }
    // IMPORTANT: Never descend into node._shadowRoot (light tree only for dir=auto)
    const kids = getLightChildren(node);
    for (let i = 0; i < kids.length; i++) {
      const d = scanSubtreeForFirstStrong(kids[i], false, followSlotAssigned);
      if (d) return d;
    }
    return null;
  }

  function getNodeDirectionality(el) {
    let cur = el;
    while (cur) {
      if (cur.nodeType === 1) {
        const dirAttr = getElementDirAttr(cur);
        const tag = String(cur.localName || cur.tagName || "").toLowerCase();
        if (dirAttr === "ltr" || dirAttr === "rtl") return dirAttr;
        if (dirAttr === "auto" || (tag === "bdi" && dirAttr === "")) {
          const found = scanSubtreeForFirstStrong(cur, true, tag === "slot");
          return found || "ltr";
        }
      }
      if (cur.parentNode && cur.parentNode.nodeType === 11 && cur.parentNode.host) {
        cur = cur.parentNode.host;
        continue;
      }
      cur = cur.parentElement || cur.parentNode;
    }
    return "ltr";
  }

  function getFlatTreeParent(el) {
    if (!el) return null;
    const p = el.parentNode || el.parentElement;
    if (!p) return null;
    if (p.nodeType === 1 && p._shadowRoot) {
      const slot = findSlotForLightChild(p, el);
      if (slot) return slot;
    }
    if (p.nodeType === 11 && p.host) {
      return p.host;
    }
    return p.nodeType === 1 ? p : null;
  }

  function getComputedDirection(el) {
    let cur = el;
    while (cur && cur.nodeType === 1) {
      // 1. Explicit inline style direction
      const inlineDir = (cur.style && typeof cur.style.getPropertyValue === "function")
        ? String(cur.style.getPropertyValue("direction") || "").trim().toLowerCase()
        : "";
      if (inlineDir === "ltr" || inlineDir === "rtl") return inlineDir;

      // 2. HTML dir attribute or <bdi>
      const dirAttr = getElementDirAttr(cur);
      const tag = String(cur.localName || cur.tagName || "").toLowerCase();
      if (dirAttr === "ltr" || dirAttr === "rtl") return dirAttr;
      if (dirAttr === "auto" || (tag === "bdi" && dirAttr === "")) {
        const found = scanSubtreeForFirstStrong(cur, true, tag === "slot");
        if (found) return found;
        if (tag !== "slot") return "ltr";
      }
      cur = getFlatTreeParent(cur);
    }
    return "ltr";
  }

  global.__capsurf_getNodeDirectionality = getNodeDirectionality;
  global.__capsurf_getComputedDirection = getComputedDirection;

  // matches & closest
  const origProtoMatches = typeof proto.matches === "function" ? proto.matches : null;
  proto.matches = function (selector) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'matches' on 'Element': 1 argument required, but only 0 present.");
    }
    if (typeof validateSelector === "function") {
      validateSelector(selector);
    }
    const s = String(selector).trim();
    const dirMatch = s.match(/^:dir\(\s*([a-zA-Z0-9_-]+)\s*\)$/i);
    if (dirMatch) {
      const targetDir = dirMatch[1].toLowerCase();
      if (targetDir !== "ltr" && targetDir !== "rtl") {
        return false;
      }
      return getNodeDirectionality(this) === targetDir;
    }
    if (origProtoMatches) {
      try {
        return origProtoMatches.call(this, selector);
      } catch (e) {
        if (e && e.name === "SyntaxError") throw e;
      }
    }
    if (s === ":checked") return Boolean(this.checked);
    if (s === ":disabled") return Boolean(this.disabled);
    if (s === ":enabled") return !this.disabled;
    if (s === ":focus") return Boolean(typeof document !== "undefined" && document.activeElement === this);
    if (s.endsWith(":focus")) {
      const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
      if (!doc || doc.activeElement !== this) return false;
      const base = s.slice(0, -6).trim();
      if (!base || base === "*") return true;
      return this.matches(base);
    }
    if (s === "*") return true;
    if (s.startsWith("#") && this.id === s.slice(1)) return true;
    if (s.startsWith(".") && this.classList.contains(s.slice(1))) return true;
    if (s.toLowerCase() === this.localName) return true;
    if (typeof document !== "undefined" && document.querySelectorAll) {
      try {
        const matches = document.querySelectorAll(s);
        for (let i = 0; i < matches.length; i++) {
          if (matches[i] === this) return true;
        }
      } catch (e) {
        if (e && e.name === "SyntaxError") throw e;
      }
    }
    return false;
  };
  markNative(proto.matches, "matches");

  if (!proto.closest) {
    proto.closest = function (selector) {
      if (arguments.length === 0) {
        throw new TypeError("Failed to execute 'closest' on 'Element': 1 argument required, but only 0 present.");
      }
      if (typeof validateSelector === "function") {
        validateSelector(selector);
      }
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

  // Scroll and Box Geometry properties
  function triggerScrollEvents(el) {
    if (!el) return;
    const Ev = (typeof Event !== "undefined")
      ? Event
      : ((typeof global !== "undefined" && global.Event) || (typeof globalThis !== "undefined" && globalThis.Event) || null);
    if (!Ev) return;

    const scrollEv = new Ev("scroll", { bubbles: false, cancelable: false });
    scrollEv._isTrusted = true;
    if (typeof dispatchTrustedEvent === "function") {
      dispatchTrustedEvent(el, scrollEv);
    } else if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
      global.__capsurf_dispatchTrustedEvent(el, scrollEv);
    } else if (typeof el.dispatchEvent === "function") {
      scrollEv._isTrusted = true;
      el.dispatchEvent(scrollEv);
    }

    if (el._scrollendTimer) {
      clearTimeout(el._scrollendTimer);
    }
    el._scrollendTimer = setTimeout(function () {
      el._scrollendTimer = null;
      const endEv = new Ev("scrollend", { bubbles: false, cancelable: false });
      endEv._isTrusted = true;
      if (typeof dispatchTrustedEvent === "function") {
        dispatchTrustedEvent(el, endEv);
      } else if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
        global.__capsurf_dispatchTrustedEvent(el, endEv);
      } else if (typeof el.dispatchEvent === "function") {
        endEv._isTrusted = true;
        el.dispatchEvent(endEv);
      }
    }, 20);
  }

  Object.defineProperty(proto, "scrollTop", {
    get: function () { return this._scrollTop || 0; },
    set: function (v) {
      const num = Number(v) || 0;
      let target = Math.max(0, num);
      const sh = this.scrollHeight;
      const ch = this.clientHeight;
      if (sh > ch && ch > 0) {
        target = Math.min(target, sh - ch);
      }
      const prev = this._scrollTop || 0;
      this._scrollTop = target;
      if (prev !== target) {
        triggerScrollEvents(this);
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "scrollLeft", {
    get: function () { return this._scrollLeft || 0; },
    set: function (v) {
      const num = Number(v) || 0;
      let target = Math.max(0, num);
      const sw = this.scrollWidth;
      const cw = this.clientWidth;
      if (sw > cw && cw > 0) {
        target = Math.min(target, sw - cw);
      }
      const prev = this._scrollLeft || 0;
      this._scrollLeft = target;
      if (prev !== target) {
        triggerScrollEvents(this);
      }
    },
    configurable: true,
  });

  function getScrollParent(node) {
    if (!node) return null;
    if (node.parentElement) return node.parentElement;
    if (node.parentNode) {
      if (node.parentNode.nodeType === 11 && node.parentNode.host) { // ShadowRoot
        return node.parentNode.host;
      }
      if (node.parentNode.nodeType === 1) {
        return node.parentNode;
      }
      if (node.parentNode.host) {
        return node.parentNode.host;
      }
    }
    if (node.host) return node.host;
    return null;
  }

  function isElementDisplayNone(el) {
    if (!el || el.nodeType !== 1) return false;
    let cur = el;
    while (cur && cur.nodeType === 1) {
      if (cur._displayNone === true) return true;
      if (cur.style) {
        const d = (typeof cur.style.getPropertyValue === "function")
          ? cur.style.getPropertyValue("display")
          : cur.style.display;
        if (d === "none") return true;
      }
      const rawStyle = (typeof cur.getAttribute === "function") ? cur.getAttribute("style") : "";
      if (rawStyle && /(?:^|;)\s*display\s*:\s*none\b/i.test(rawStyle)) {
        return true;
      }
      if (cur._computedStyle && cur._computedStyle.display === "none") {
        return true;
      }
      if (typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
        try {
          const cs = window.getComputedStyle(cur);
          if (cs) {
            const cd = (typeof cs.getPropertyValue === "function") ? cs.getPropertyValue("display") : cs.display;
            if (cd === "none") return true;
          }
        } catch (e) {}
      }
      cur = getScrollParent(cur);
    }
    return false;
  }

  function makeDomRect(x, y, width, height) {
    if (typeof DOMRect === "function") {
      return new DOMRect(x, y, width, height);
    }
    const nx = x || 0, ny = y || 0, nw = width || 0, nh = height || 0;
    const obj = {
      x: nx,
      y: ny,
      width: nw,
      height: nh,
      top: ny,
      right: nx + nw,
      bottom: ny + nh,
      left: nx,
    };
    obj.toJSON = function () {
      return { x: this.x, y: this.y, width: this.width, height: this.height, top: this.top, right: this.right, bottom: this.bottom, left: this.left };
    };
    return obj;
  }

  const origGetBoundingClientRect = proto.getBoundingClientRect;
  proto.getBoundingClientRect = function () {
    if (isElementDisplayNone(this)) {
      return makeDomRect(0, 0, 0, 0);
    }
    const r = origGetBoundingClientRect ? origGetBoundingClientRect.call(this) : { x: 0, y: 0, width: 0, height: 0, left: 0, top: 0, right: 0, bottom: 0 };
    if (this._shadowRoot) {
      if (this._inShadowRect) {
        const hw = r.width || 800;
        const hx = r.x || 0;
        const hy = r.y || 0;
        return makeDomRect(hx, hy, hw, 0);
      }
      let explicitH = 0;
      if (this.style && this.style.height) explicitH = parseFloat(this.style.height) || 0;
      if (!explicitH && typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
        try {
          const hcs = window.getComputedStyle(this);
          if (hcs && hcs.height && hcs.height !== "auto") explicitH = parseFloat(hcs.height) || 0;
        } catch (e) {}
      }
      if (!explicitH) {
        this._inShadowRect = true;
        let shadowH = 0;
        try {
          const sKids = Array.from(this._shadowRoot.childNodes || this._shadowRoot._children || []);
          for (let i = 0; i < sKids.length; i++) {
            const sk = sKids[i];
            if (sk && sk.nodeType === 1) {
              const stag = String(sk.localName || sk.tagName || "").toLowerCase();
              if (stag === "style" || stag === "script") continue;
              const sr = typeof sk.getBoundingClientRect === "function" ? sk.getBoundingClientRect() : null;
              if (sr && sr.height > 0) {
                shadowH += sr.height;
              } else if (sk.textContent && sk.textContent.trim().length > 0) {
                shadowH += 16;
              }
            }
          }
        } finally {
          this._inShadowRect = false;
        }
        const hostW = r.width || 800;
        const hostX = r.x || 0;
        const hostY = r.y || 0;
        return makeDomRect(hostX, hostY, hostW, shadowH);
      }
    }
    if (r.width === 0 && r.height === 0 && r.x === 0 && r.y === 0) {
      const tag = String(this.tagName || this.localName || "").toUpperCase();
      if (tag === "IMG") {
        const nw = Number(this.naturalWidth || 0);
        const nh = Number(this.naturalHeight || 0);
        if (nw > 0 || nh > 0) {
          return makeDomRect(0, 0, nw, nh);
        }
      }
      let childW = 0, childH = 0;
      if (this.style) {
        if (this.style.width) childW = parseFloat(this.style.width) || 0;
        if (this.style.height) childH = parseFloat(this.style.height) || 0;
      }
      if ((!childW || !childH) && typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
        try {
          const cs = window.getComputedStyle(this);
          if (cs) {
            if (!childW && cs.width && cs.width !== "auto") childW = parseFloat(cs.width) || 0;
            if (!childH && cs.height && cs.height !== "auto") childH = parseFloat(cs.height) || 0;
          }
        } catch (e) {}
      }
      const parent = getScrollParent(this);
      if (parent && parent.nodeType === 1 && (childW > 0 || childH > 0)) {
        const pr = parent.getBoundingClientRect();
        if (!childW) childW = pr.width;
        let offsetTop = 0;
        const siblings = (this.parentNode && (this.parentNode.children || this.parentNode._children || this.parentNode.childNodes)) ||
                         (parent.children || parent._children || []);
        for (let i = 0; i < siblings.length; i++) {
          const sib = siblings[i];
          if (sib === this) break;
          if (sib && sib.nodeType === 1) {
            let sh = (sib.getBoundingClientRect ? sib.getBoundingClientRect().height : 0) || 0;
            if (!sh && sib.style && sib.style.height) sh = parseFloat(sib.style.height) || 0;
            offsetTop += sh;
          }
        }
        const relX = pr.left;
        const relY = pr.top + offsetTop - (parent._scrollTop || 0);
        return makeDomRect(relX, relY, childW, childH);
      }
      return makeDomRect(0, 0, 0, 0);
    }
    const winX = (typeof window !== "undefined" && (window.scrollX || window.pageXOffset)) || 0;
    const winY = (typeof window !== "undefined" && (window.scrollY || window.pageYOffset)) || 0;
    let scrollOffsetParentX = 0;
    let scrollOffsetParentY = 0;
    let cur = getScrollParent(this);
    while (cur && cur !== document.body && cur !== document.documentElement && cur !== document) {
      if (cur._scrollTop) scrollOffsetParentY += cur._scrollTop;
      if (cur._scrollLeft) scrollOffsetParentX += cur._scrollLeft;
      cur = getScrollParent(cur);
    }
    const x = r.x - winX - scrollOffsetParentX;
    const y = r.y - winY - scrollOffsetParentY;
    const w = r.width;
    const h = r.height;
    return makeDomRect(x, y, w, h);
  };
  markNative(proto.getBoundingClientRect, "getBoundingClientRect");

  Object.defineProperty(proto, "clientHeight", {
    get: function () {
      if (this === document.documentElement || this === document.body) {
        return (typeof window !== "undefined" && window.innerHeight) || 720;
      }
      const r = origGetBoundingClientRect ? origGetBoundingClientRect.call(this) : this.getBoundingClientRect();
      return Math.round(r.height);
    },
    configurable: true,
  });

  Object.defineProperty(proto, "clientWidth", {
    get: function () {
      if (this === document.documentElement || this === document.body) {
        return (typeof window !== "undefined" && window.innerWidth) || 1280;
      }
      const r = origGetBoundingClientRect ? origGetBoundingClientRect.call(this) : this.getBoundingClientRect();
      return Math.round(r.width);
    },
    configurable: true,
  });

  Object.defineProperty(proto, "scrollHeight", {
    get: function () {
      if (this === document.documentElement || this === document.body) {
        let maxH = (typeof window !== "undefined" && window.innerHeight) || 720;
        if (this.style && this.style.height) {
          const ph = parseFloat(this.style.height);
          if (!isNaN(ph) && ph > maxH) maxH = ph;
        }
        const winY = (typeof window !== "undefined" && (window.scrollY || window.pageYOffset)) || 0;
        const all = this.children || [];
        for (let i = 0; i < all.length; i++) {
          const child = all[i];
          const cr = child.getBoundingClientRect ? child.getBoundingClientRect() : null;
          if (cr && cr.bottom + winY > maxH) maxH = cr.bottom + winY;
        }
        return Math.round(maxH);
      }

      let maxBottom = this.clientHeight;
      const parentR = this.getBoundingClientRect();
      const shadow = this._shadowRoot || this.shadowRoot;
      const shadowKids = shadow ? (shadow.children || shadow._children || shadow.childNodes || []) : [];
      const all = (shadowKids.length > 0) ? shadowKids : (this.children || []);
      let accumY = 0;
      for (let i = 0; i < all.length; i++) {
        const child = all[i];
        if (!child || child.nodeType !== 1) continue;
        const cr = child.getBoundingClientRect ? child.getBoundingClientRect() : null;
        let childH = (cr && cr.height) || 0;
        if (!childH && child.style && child.style.height) {
          childH = parseFloat(child.style.height) || 0;
        }
        let relY = (parentR && cr && cr.height > 0) ? (cr.y - parentR.y + (this._scrollTop || 0)) : accumY;
        accumY += childH;
        const b = Math.max(relY + childH, childH);
        if (b > maxBottom) maxBottom = b;
      }
      return Math.round(maxBottom);
    },
    configurable: true,
  });

  Object.defineProperty(proto, "scrollWidth", {
    get: function () {
      if (this === document.documentElement || this === document.body) {
        let maxW = (typeof window !== "undefined" && window.innerWidth) || 1280;
        if (this.style && this.style.width) {
          const pw = parseFloat(this.style.width);
          if (!isNaN(pw) && pw > maxW) maxW = pw;
        }
        const winX = (typeof window !== "undefined" && (window.scrollX || window.pageXOffset)) || 0;
        const all = this.children || [];
        for (let i = 0; i < all.length; i++) {
          const child = all[i];
          const cr = child.getBoundingClientRect ? child.getBoundingClientRect() : null;
          if (cr && cr.right + winX > maxW) maxW = cr.right + winX;
        }
        return Math.round(maxW);
      }

      let maxRight = this.clientWidth;
      const parentR = this.getBoundingClientRect();
      const shadow = this._shadowRoot || this.shadowRoot;
      const shadowKids = shadow ? (shadow.children || shadow._children || shadow.childNodes || []) : [];
      const all = (shadowKids.length > 0) ? shadowKids : (this.children || []);
      let accumX = 0;
      for (let i = 0; i < all.length; i++) {
        const child = all[i];
        if (!child || child.nodeType !== 1) continue;
        const cr = child.getBoundingClientRect ? child.getBoundingClientRect() : null;
        let childW = (cr && cr.width) || 0;
        if (!childW && child.style && child.style.width) {
          childW = parseFloat(child.style.width) || 0;
        }
        let relX = (parentR && cr && cr.width > 0) ? (cr.x - parentR.x + (this._scrollLeft || 0)) : accumX;
        accumX += childW;
        const r = Math.max(relX + childW, childW);
        if (r > maxRight) maxRight = r;
      }
      return Math.round(maxRight);
    },
    configurable: true,
  });

  function computeAxis(elPos, elSize, viewSize, curScroll, mode) {
    switch (mode) {
      case "start":
        return elPos;
      case "end":
        return elPos + elSize - viewSize;
      case "center":
        return elPos + elSize / 2 - viewSize / 2;
      case "nearest": {
        if (elPos >= curScroll && elPos + elSize <= curScroll + viewSize) {
          return curScroll;
        }
        if (elPos < curScroll) return elPos;
        return elPos + elSize - viewSize;
      }
      default:
        return elPos;
    }
  }

  function scrollElementIntoView(el, block, inline, onlyIfNeeded) {
    if (!el) return;
    const vw = (typeof window !== "undefined" && window.innerWidth) || 1280;
    const vh = (typeof window !== "undefined" && window.innerHeight) || 720;

    const r = el.getBoundingClientRect();
    const inWin = (r.top >= 0 && r.bottom <= vh && r.left >= 0 && r.right <= vw);

    let clipped = false;
    let cur = getScrollParent(el);
    while (cur && cur !== document.body && cur !== document.documentElement && cur !== document) {
      if (cur.nodeType === 1) {
        const style = (typeof window !== "undefined" && window.getComputedStyle) ? window.getComputedStyle(cur) : cur.style;
        const overflow = (style && (style.overflow || style.overflowY || style.overflowX)) || "";
        const isScrollable = (overflow === "auto" || overflow === "scroll" || overflow === "hidden" || (cur.getAttribute && cur.getAttribute("style") && cur.getAttribute("style").includes("overflow")));
        if (isScrollable) {
          const cr = cur.getBoundingClientRect();
          if (r.top < cr.top || r.bottom > cr.top + cur.clientHeight || r.left < cr.left || r.right > cr.left + cur.clientWidth) {
            clipped = true;
            break;
          }
        }
      }
      cur = getScrollParent(cur);
    }

    if (onlyIfNeeded && inWin && !clipped) {
      return;
    }

    // 1. Scroll container ancestors
    cur = getScrollParent(el);
    let safety = 0;
    while (cur && cur !== document.body && cur !== document.documentElement && cur !== document && safety < 50) {
      safety++;
      if (cur.nodeType === 1) {
        const style = (typeof window !== "undefined" && window.getComputedStyle) ? window.getComputedStyle(cur) : cur.style;
        const ox = (style && (style.overflowX || style.overflow)) || "";
        const oy = (style && (style.overflowY || style.overflow)) || "";
        const styleAttr = (cur.getAttribute && cur.getAttribute("style")) || "";
        const isScrollable = ox === "auto" || ox === "scroll" || ox === "hidden" ||
                             oy === "auto" || oy === "scroll" || oy === "hidden" ||
                             styleAttr.includes("overflow");
        const sh = cur.scrollHeight || 0;
        const ch = cur.clientHeight || 0;
        const sw = cur.scrollWidth || 0;
        const cw = cur.clientWidth || 0;
        const overflowsY = isScrollable || (sh > ch && ch > 0);
        const overflowsX = isScrollable || (sw > cw && cw > 0);

        if (overflowsY || overflowsX) {
          const cr = cur.getBoundingClientRect();
          const er = el.getBoundingClientRect();
          const curScrollTop = cur.scrollTop || 0;
          const curScrollLeft = cur.scrollLeft || 0;
          const elInContentY = (er.top - cr.top) + curScrollTop;
          const elInContentX = (er.left - cr.left) + curScrollLeft;

          if (overflowsY) {
            const rawY = computeAxis(elInContentY, er.height, ch, curScrollTop, block);
            const maxY = Math.max(0, sh - ch);
            const targetY = Math.max(0, Math.min(rawY, maxY > 0 ? maxY : rawY));
            if (targetY !== curScrollTop) {
              cur.scrollTop = targetY;
            }
          }

          if (overflowsX) {
            const rawX = computeAxis(elInContentX, er.width, cw, curScrollLeft, inline);
            const maxX = Math.max(0, sw - cw);
            const targetX = Math.max(0, Math.min(rawX, maxX > 0 ? maxX : rawX));
            if (targetX !== curScrollLeft) {
              cur.scrollLeft = targetX;
            }
          }
        }
      }
      cur = getScrollParent(cur);
    }

    // 2. Scroll window if needed
    const afterR = el.getBoundingClientRect();
    const curWinX = (typeof window !== "undefined" && (window.scrollX || window.pageXOffset)) || 0;
    const curWinY = (typeof window !== "undefined" && (window.scrollY || window.pageYOffset)) || 0;
    const targetDocY = afterR.top + curWinY;
    const targetDocX = afterR.left + curWinX;

    const newWinY = computeAxis(targetDocY, afterR.height, vh, curWinY, block);
    const newWinX = computeAxis(targetDocX, afterR.width, vw, curWinX, inline);

    if (typeof window !== "undefined" && window.scrollTo) {
      window.scrollTo(Math.max(0, Math.round(newWinX)), Math.max(0, Math.round(newWinY)));
    }
  }

  proto.scrollIntoView = function (arg) {
    let block = "start";
    let inline = "nearest";
    if (arg === false) {
      block = "end";
      inline = "nearest";
    } else if (arg === true || arg == null) {
      block = "start";
      inline = "nearest";
    } else if (typeof arg === "object") {
      block = String(arg.block || "start");
      inline = String(arg.inline || "nearest");
    }
    scrollElementIntoView(this, block, inline, false);
  };
  markNative(proto.scrollIntoView, "scrollIntoView");

  proto.scrollIntoViewIfNeeded = function (centerIfNeeded) {
    const center = centerIfNeeded !== false;
    scrollElementIntoView(this, center ? "center" : "nearest", center ? "center" : "nearest", true);
  };
  markNative(proto.scrollIntoViewIfNeeded, "scrollIntoViewIfNeeded");

  proto.checkVisibility = function (options) {
    if (!this.isConnected) return false;

    function getStyle(el) {
      if (typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
        try {
          const cs = window.getComputedStyle(el);
          if (cs) return cs;
        } catch (e) {}
      }
      if (typeof getComputedStyle === "function") {
        try {
          const cs = getComputedStyle(el);
          if (cs) return cs;
        } catch (e) {}
      }
      if (typeof globalThis !== "undefined" && typeof globalThis.getComputedStyle === "function") {
        try {
          const cs = globalThis.getComputedStyle(el);
          if (cs) return cs;
        } catch (e) {}
      }
      return el.style || null;
    }

    function getProp(el, name) {
      if (!el) return "";
      const s = getStyle(el);
      let v = "";
      if (s) {
        if (typeof s.getPropertyValue === "function") {
          const val = s.getPropertyValue(name);
          if (val !== undefined && val !== null && val !== "") v = String(val).trim();
        }
        if (!v) {
          const camel = name.replace(/-([a-z])/g, function (_, c) { return c.toUpperCase(); });
          if (s[camel] !== undefined && s[camel] !== null && s[camel] !== "") {
            v = String(s[camel]).trim();
          } else if (s[name] !== undefined && s[name] !== null && s[name] !== "") {
            v = String(s[name]).trim();
          }
        }
      }
      if (!v && el.style) {
        if (typeof el.style.getPropertyValue === "function") {
          const val = el.style.getPropertyValue(name);
          if (val !== undefined && val !== null && val !== "") v = String(val).trim();
        }
        if (!v) {
          const camel = name.replace(/-([a-z])/g, function (_, c) { return c.toUpperCase(); });
          if (el.style[camel] !== undefined && el.style[camel] !== null && el.style[camel] !== "") {
            v = String(el.style[camel]).trim();
          }
        }
      }
      return v;
    }

    function isZero(val) {
      if (val === "" || val === null || val === undefined) return false;
      const num = Number(val);
      return !isNaN(num) && num === 0;
    }

    const selfDisplay = getProp(this, "display").toLowerCase();
    if (selfDisplay === "none" || selfDisplay === "contents") return false;

    const checkOpacity = Boolean(options && options.checkOpacity === true);
    const checkVisibilityCSS = Boolean(options && options.checkVisibilityCSS === true);

    if (checkOpacity) {
      const selfOpacity = getProp(this, "opacity");
      if (isZero(selfOpacity)) return false;
    }

    if (checkVisibilityCSS) {
      const selfVis = getProp(this, "visibility").toLowerCase();
      if (selfVis === "hidden" || selfVis === "collapse") return false;
    }

    let cur = this.parentElement || this.parentNode;
    while (cur) {
      if (cur.nodeType === 1) { // ELEMENT_NODE
        const d = getProp(cur, "display").toLowerCase();
        if (d === "none") return false;

        if (checkOpacity) {
          const op = getProp(cur, "opacity");
          if (isZero(op)) return false;
        }
      } else if (cur.nodeType === 11 && cur.host) { // ShadowRoot
        cur = cur.host;
        continue;
      }
      cur = cur.parentElement || cur.parentNode;
    }

    return true;
  };
  markNative(proto.checkVisibility, "checkVisibility");

  proto.attachShadow = function (init) {
    if (!init || typeof init !== "object" || !init.mode) {
      throw new TypeError("Failed to execute 'attachShadow' on 'Element': 1 argument required, but only 0 present.");
    }
    const mode = String(init.mode);
    if (mode !== "open" && mode !== "closed") {
      throw new TypeError("Failed to execute 'attachShadow' on 'Element': The provided value '" + mode + "' is not a valid enum value of type ShadowRootMode.");
    }
    if (this._shadowRoot) {
      if (typeof DOMException !== "undefined") {
        throw new DOMException("Shadow root cannot be created on a host which already hosts a shadow tree", "NotSupportedError");
      }
      const err = new Error("Shadow root cannot be created on a host which already hosts a shadow tree");
      err.name = "NotSupportedError";
      throw err;
    }
    const ShadowRootCtor = typeof ShadowRoot !== "undefined" ? ShadowRoot : (global.ShadowRoot || null);
    const shadow = ShadowRootCtor ? new ShadowRootCtor(this, mode) : new DocumentFragment();
    shadow.host = this;
    shadow.mode = mode;
    this._shadowRoot = shadow;
    return shadow;
  };
  markNative(proto.attachShadow, "attachShadow");

  Object.defineProperty(proto, "shadowRoot", {
    get: function () {
      if (this._shadowRoot && this._shadowRoot.mode === "open") {
        return this._shadowRoot;
      }
      return null;
    },
    configurable: true,
  });

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
      if (this.setAttribute) {
        this.setAttribute("value", String(v));
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "defaultValue", {
    get: function () {
      return this.getAttribute("value") || "";
    },
    set: function (v) {
      this.setAttribute("value", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(proto, "checked", {
    get: function () {
      if (this._checkedDirty) return this._checked;
      return this.hasAttribute("checked");
    },
    set: function (v) {
      this._checkedDirty = true;
      this._checked = Boolean(v);
      const isRadio = (this.getAttribute && (this.getAttribute("type") || "").toLowerCase() === "radio");
      if (isRadio && v && this.name) {
        const root = (this.ownerDocument || (typeof document !== "undefined" ? document : null));
        if (root && root.querySelectorAll) {
          const radios = root.querySelectorAll('input[type="radio"][name="' + this.name + '"]');
          for (let i = 0; i < radios.length; i++) {
            if (radios[i] !== this) {
              radios[i]._checkedDirty = true;
              radios[i]._checked = false;
              if (radios[i].removeAttribute) radios[i].removeAttribute("checked");
            }
          }
        }
      }
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
      if ((this.tagName || "").toLowerCase() === "option" && typeof global !== "undefined" && global.HTMLOptionElement) {
        const desc = Object.getOwnPropertyDescriptor(global.HTMLOptionElement.prototype, "selected");
        if (desc && desc.get) return desc.get.call(this);
      }
      if (this._selected !== undefined) return this._selected;
      return this.hasAttribute("selected");
    },
    set: function (v) {
      if ((this.tagName || "").toLowerCase() === "option" && typeof global !== "undefined" && global.HTMLOptionElement) {
        const desc = Object.getOwnPropertyDescriptor(global.HTMLOptionElement.prototype, "selected");
        if (desc && desc.set) return desc.set.call(this, v);
      }
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
      if ((this.tagName || "").toLowerCase() === "select") {
        if (typeof getSelectOptionsCollection === "function") {
          return getSelectOptionsCollection(this);
        }
      }
      const opts = [];
      const up = typeof upgradeElement === "function" ? upgradeElement : (typeof globalThis !== "undefined" && globalThis.upgradeElement);
      let child = this.firstChild;
      while (child) {
        if ((child.tagName || "").toLowerCase() === "option") {
          if (up) up(child);
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
      if ((this.tagName || "").toLowerCase() === "select" && typeof global !== "undefined" && global.HTMLSelectElement) {
        const desc = Object.getOwnPropertyDescriptor(global.HTMLSelectElement.prototype, "selectedIndex");
        if (desc && desc.get) return desc.get.call(this);
      }
      const opts = this.options;
      for (let i = 0; i < opts.length; i++) {
        if (opts[i].selected) return i;
      }
      return opts.length > 0 && !this.multiple ? 0 : -1;
    },
    set: function (idx) {
      if ((this.tagName || "").toLowerCase() === "select" && typeof global !== "undefined" && global.HTMLSelectElement) {
        const desc = Object.getOwnPropertyDescriptor(global.HTMLSelectElement.prototype, "selectedIndex");
        if (desc && desc.set) return desc.set.call(this, idx);
      }
      const i = Number(idx);
      const opts = this.options;
      for (let j = 0; j < opts.length; j++) {
        opts[j].selected = (j === i);
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "validity", {
    get: function () { return makeValidityState(this); },
    configurable: true,
  });

  Object.defineProperty(proto, "willValidate", {
    get: function () {
      return computeWillValidate(this);
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
