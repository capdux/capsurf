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
        const rawKey = p.slice(0, colon).trim();
        const k = rawKey.startsWith("--") ? rawKey : rawKey.toLowerCase();
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
          const str = String(p);
          const key = str.startsWith("--") ? str : str.toLowerCase();
          return map.get(key) || "";
        };
      }
      if (prop === "setProperty") {
        return function (p, val) {
          const map = parseStyleMap();
          const str = String(p);
          const key = str.startsWith("--") ? str : str.toLowerCase();
          map.set(key, String(val));
          serializeStyleMap(map);
        };
      }
      if (prop === "removeProperty") {
        return function (p) {
          const map = parseStyleMap();
          const str = String(p);
          const key = str.startsWith("--") ? str : str.toLowerCase();
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

  // Internal attribute synchronization
  proto._syncAttrs = function () {
    if (!this._attrs) {
      this._attrs = new Map();
      this._attrList = [];
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
              this._attrs.set(item.name.toLowerCase(), entry);
            }
          }
        }
      }
      if (this.id && !this._attrs.has("id")) {
        const entry = { name: "id", value: this.id, namespaceURI: null, localName: "id", prefix: null };
        this._attrList.push(entry);
        this._attrs.set("id", entry);
      }
      if (this.className && !this._attrs.has("class")) {
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
    const key = String(name);
    const val = String(value);
    if (!this._attrs) this._syncAttrs();
    let existing = null;
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        if (this._attrList[i].name.toLowerCase() === key.toLowerCase()) {
          existing = this._attrList[i];
          break;
        }
      }
    }
    const oldVal = existing ? existing.value : (this._attrs.get(key.toLowerCase())?.value ?? null);
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
    this._attrs.set(key.toLowerCase(), { name: key, value: val });
    if (key.toLowerCase() === "id") {
      this.id = val;
    } else if (key.toLowerCase() === "class") {
      this.className = val;
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
  };
  markNative(proto.setAttribute, "setAttribute");

  // removeAttribute
  const origRemoveAttribute = proto.removeAttribute;
  proto.removeAttribute = function (name) {
    const key = String(name).toLowerCase();
    if (!this._attrs) this._syncAttrs();
    let oldVal = null;
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        if (this._attrList[i].name.toLowerCase() === key) {
          oldVal = this._attrList[i].value;
          this._attrList.splice(i, 1);
          break;
        }
      }
    }
    const oldEntry = this._attrs.get(key);
    if (!oldVal && oldEntry) oldVal = oldEntry.value;
    this._attrs.delete(key);
    if (key === "id") {
      this.id = "";
    } else if (key === "class") {
      this.className = "";
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
    const key = String(name).toLowerCase();
    if (!this._attrs) this._syncAttrs();
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        if (this._attrList[i].name.toLowerCase() === key) {
          return this._attrList[i].value;
        }
      }
    }
    if (this._attrs) {
      const entry = this._attrs.get(key);
      if (entry) return entry.value;
      if (origGetAttribute) {
        const val = origGetAttribute.call(this, name);
        if (val !== null) return val;
      }
      return null;
    }
    if (origGetAttribute) {
      return origGetAttribute.call(this, name);
    }
    return null;
  };
  markNative(proto.getAttribute, "getAttribute");

  // hasAttribute
  const origHasAttribute = proto.hasAttribute;
  proto.hasAttribute = function (name) {
    const key = String(name).toLowerCase();
    if (!this._attrs) this._syncAttrs();
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        if (this._attrList[i].name.toLowerCase() === key) {
          return true;
        }
      }
    }
    if (this._attrs) {
      if (this._attrs.has(key)) return true;
      if (origHasAttribute) return origHasAttribute.call(this, name);
      return false;
    }
    if (origHasAttribute) {
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
  if (clsDesc && clsDesc.set) {
    const origClsSet = clsDesc.set;
    Object.defineProperty(proto, "className", {
      get: clsDesc.get,
      set: function (v) {
        origClsSet.call(this, v);
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
      if (s === ":checked") return Boolean(this.checked);
      if (s === ":disabled") return Boolean(this.disabled);
      if (s === ":enabled") return !this.disabled;
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
      if (this._checkedDirty) return this._checked;
      return this.hasAttribute("checked");
    },
    set: function (v) {
      this._checkedDirty = true;
      this._checked = Boolean(v);
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
