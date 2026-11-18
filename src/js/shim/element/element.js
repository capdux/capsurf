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

  // Scroll and Box Geometry properties
  Object.defineProperty(proto, "scrollTop", {
    get: function () { return this._scrollTop || 0; },
    set: function (v) { this._scrollTop = Math.max(0, Number(v) || 0); },
    configurable: true,
  });

  Object.defineProperty(proto, "scrollLeft", {
    get: function () { return this._scrollLeft || 0; },
    set: function (v) { this._scrollLeft = Math.max(0, Number(v) || 0); },
    configurable: true,
  });

  const origGetBoundingClientRect = proto.getBoundingClientRect;
  proto.getBoundingClientRect = function () {
    const r = origGetBoundingClientRect ? origGetBoundingClientRect.call(this) : { x: 0, y: 0, width: 0, height: 0, left: 0, top: 0, right: 0, bottom: 0 };
    const winX = (typeof window !== "undefined" && (window.scrollX || window.pageXOffset)) || 0;
    const winY = (typeof window !== "undefined" && (window.scrollY || window.pageYOffset)) || 0;
    let scrollOffsetParentX = 0;
    let scrollOffsetParentY = 0;
    let cur = this.parentElement;
    while (cur && cur !== document.body && cur !== document.documentElement) {
      if (cur._scrollTop) scrollOffsetParentY += cur._scrollTop;
      if (cur._scrollLeft) scrollOffsetParentX += cur._scrollLeft;
      cur = cur.parentElement;
    }
    const x = r.x - winX - scrollOffsetParentX;
    const y = r.y - winY - scrollOffsetParentY;
    const w = r.width;
    const h = r.height;
    return {
      x: x,
      y: y,
      left: x,
      top: y,
      width: w,
      height: h,
      right: x + w,
      bottom: y + h,
    };
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
      let maxBottom = this.clientHeight;
      const all = this.children || [];
      for (let i = 0; i < all.length; i++) {
        const cr = all[i].getBoundingClientRect ? all[i].getBoundingClientRect() : null;
        if (cr && cr.bottom > maxBottom) maxBottom = cr.bottom;
      }
      return maxBottom;
    },
    configurable: true,
  });

  Object.defineProperty(proto, "scrollWidth", {
    get: function () {
      let maxRight = this.clientWidth;
      const all = this.children || [];
      for (let i = 0; i < all.length; i++) {
        const cr = all[i].getBoundingClientRect ? all[i].getBoundingClientRect() : null;
        if (cr && cr.right > maxRight) maxRight = cr.right;
      }
      return maxRight;
    },
    configurable: true,
  });

  function scrollElementIntoView(el, centerIfNeeded) {
    if (!el) return;
    const vw = (typeof window !== "undefined" && window.innerWidth) || 1280;
    const vh = (typeof window !== "undefined" && window.innerHeight) || 720;

    const r = el.getBoundingClientRect();
    const inWin = (r.top >= 0 && r.bottom <= vh && r.left >= 0 && r.right <= vw);

    let clipped = false;
    let cur = el.parentElement;
    while (cur && cur !== document.body && cur !== document.documentElement) {
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
      cur = cur.parentElement;
    }

    if (centerIfNeeded && inWin && !clipped) {
      return;
    }

    // Scroll container ancestors
    cur = el.parentElement;
    while (cur && cur !== document.body && cur !== document.documentElement) {
      const style = (typeof window !== "undefined" && window.getComputedStyle) ? window.getComputedStyle(cur) : cur.style;
      const overflow = (style && (style.overflow || style.overflowY || style.overflowX)) || "";
      const isScrollable = (overflow === "auto" || overflow === "scroll" || overflow === "hidden" || (cur.getAttribute && cur.getAttribute("style") && cur.getAttribute("style").includes("overflow")));
      if (isScrollable) {
        const cr = cur.getBoundingClientRect();
        const er = el.getBoundingClientRect();
        const curScrollTop = cur.scrollTop || 0;
        const elMidInContainer = (er.top - cr.top) + curScrollTop + (er.height / 2);
        const targetScrollTop = elMidInContainer - (cur.clientHeight / 2);
        cur.scrollTop = Math.max(0, targetScrollTop);
      }
      cur = cur.parentElement;
    }

    // Scroll window if needed
    const afterR = el.getBoundingClientRect();
    const afterInWin = (afterR.top >= 0 && afterR.bottom <= vh && afterR.left >= 0 && afterR.right <= vw);
    if (!afterInWin || !centerIfNeeded) {
      const curWinY = (typeof window !== "undefined" && (window.scrollY || window.pageYOffset)) || 0;
      const elPageY = afterR.top + curWinY;
      const targetWinY = (elPageY + afterR.height / 2) - (vh / 2);
      if (typeof window !== "undefined" && window.scrollTo) {
        window.scrollTo(window.scrollX || 0, Math.max(0, targetWinY));
      }
    }
  }

  proto.scrollIntoView = function (arg) {
    const center = (typeof arg === "object" && arg !== null && (arg.block === "center" || arg.inline === "center"));
    scrollElementIntoView(this, !center ? false : false);
  };
  markNative(proto.scrollIntoView, "scrollIntoView");

  proto.scrollIntoViewIfNeeded = function (centerIfNeeded) {
    scrollElementIntoView(this, centerIfNeeded !== false);
  };
  markNative(proto.scrollIntoViewIfNeeded, "scrollIntoViewIfNeeded");

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
