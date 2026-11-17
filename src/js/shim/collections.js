// src/js/shim/collections.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/collections.ts

function DOMTokenList(getter, setter) {
  this._getter = getter;
  this._setter = setter;
}

DOMTokenList.prototype._getTokens = function () {
  const str = (this._getter() || "").trim();
  if (!str) return [];
  return str.split(/\s+/);
};

DOMTokenList.prototype._setTokens = function (tokens) {
  this._setter(tokens.join(" "));
};

Object.defineProperty(DOMTokenList.prototype, "length", {
  get: function () {
    return this._getTokens().length;
  },
  configurable: true,
});

DOMTokenList.prototype.item = function (index) {
  const tokens = this._getTokens();
  const i = Number(index);
  return (i >= 0 && i < tokens.length) ? tokens[i] : null;
};

DOMTokenList.prototype.contains = function (token) {
  if (!token) return false;
  const tokens = this._getTokens();
  return tokens.indexOf(String(token)) !== -1;
};

DOMTokenList.prototype.add = function () {
  const tokens = this._getTokens();
  let changed = false;
  for (let i = 0; i < arguments.length; i++) {
    const token = String(arguments[i]);
    if (!token || /\s/.test(token)) {
      throw new Error("InvalidCharacterError: Invalid token '" + token + "'");
    }
    if (tokens.indexOf(token) === -1) {
      tokens.push(token);
      changed = true;
    }
  }
  if (changed) this._setTokens(tokens);
};

DOMTokenList.prototype.remove = function () {
  let tokens = this._getTokens();
  let changed = false;
  for (let i = 0; i < arguments.length; i++) {
    const token = String(arguments[i]);
    const idx = tokens.indexOf(token);
    if (idx !== -1) {
      tokens.splice(idx, 1);
      changed = true;
    }
  }
  if (changed) this._setTokens(tokens);
};

DOMTokenList.prototype.toggle = function (token, force) {
  const str = String(token);
  if (!str || /\s/.test(str)) {
    throw new Error("InvalidCharacterError: Invalid token '" + str + "'");
  }
  const has = this.contains(str);
  if (force === true) {
    if (!has) this.add(str);
    return true;
  } else if (force === false) {
    if (has) this.remove(str);
    return false;
  } else {
    if (has) {
      this.remove(str);
      return false;
    } else {
      this.add(str);
      return true;
    }
  }
};

DOMTokenList.prototype.replace = function (oldToken, newToken) {
  const tokens = this._getTokens();
  const idx = tokens.indexOf(String(oldToken));
  if (idx === -1) return false;
  tokens[idx] = String(newToken);
  this._setTokens(tokens);
  return true;
};

DOMTokenList.prototype.forEach = function (callback, thisArg) {
  const tokens = this._getTokens();
  for (let i = 0; i < tokens.length; i++) {
    callback.call(thisArg, tokens[i], i, this);
  }
};

DOMTokenList.prototype.values = function () {
  return this._getTokens()[Symbol.iterator]();
};

DOMTokenList.prototype.entries = function () {
  const tokens = this._getTokens();
  const arr = [];
  for (let i = 0; i < tokens.length; i++) arr.push([i, tokens[i]]);
  return arr[Symbol.iterator]();
};

DOMTokenList.prototype.toString = function () {
  return this._getter() || "";
};

DOMTokenList.prototype[Symbol.iterator] = function () {
  return this.values();
};

markNative(DOMTokenList, "DOMTokenList");
markNative(DOMTokenList.prototype.add, "add");
markNative(DOMTokenList.prototype.remove, "remove");
markNative(DOMTokenList.prototype.toggle, "toggle");
markNative(DOMTokenList.prototype.contains, "contains");
markNative(DOMTokenList.prototype.item, "item");

function findNamedItemInList(list, name) {
  const key = String(name);
  const len = list ? (list.length || 0) : 0;
  for (let i = 0; i < len; i++) {
    const el = list[i];
    if (!el) continue;
    if (el.id === key || (el.getAttribute && el.getAttribute("id") === key)) {
      return el;
    }
  }
  for (let i = 0; i < len; i++) {
    const el = list[i];
    if (!el) continue;
    if (el.name === key || (el.getAttribute && el.getAttribute("name") === key)) {
      return el;
    }
  }
  return null;
}

function makeHtmlCollection(getList) {
  const target = Object.create(HTMLCollection.prototype);
  const proxy = new Proxy(target, {
    get: function (t, prop, receiver) {
      if (prop === "length") {
        return getList().length;
      }
      if (typeof prop === "string" && /^\d+$/.test(prop)) {
        const i = Number(prop);
        const list = getList();
        if (i >= 0 && i < list.length) {
          return list[i];
        }
        return undefined;
      }
      if (prop === "item") {
        return function (index) {
          const list = getList();
          const i = Number(index);
          return (i >= 0 && i < list.length) ? list[i] : null;
        };
      }
      if (prop === "namedItem") {
        return function (name) {
          return findNamedItemInList(getList(), name);
        };
      }
      if (prop === Symbol.iterator) {
        return function () {
          const list = getList();
          let idx = 0;
          return {
            next: function () {
              if (idx < list.length) {
                return { value: list[idx++], done: false };
              }
              return { value: undefined, done: true };
            }
          };
        };
      }
      if (prop in t) {
        return Reflect.get(t, prop, receiver);
      }
      if (typeof prop === "string") {
        const named = findNamedItemInList(getList(), prop);
        if (named !== null) {
          return named;
        }
      }
      return Reflect.get(t, prop, receiver);
    },
    set: function (t, prop, value, receiver) {
      if (prop === "length") return false;
      if (typeof prop === "string" && /^\d+$/.test(prop)) return false;
      return Reflect.set(t, prop, value, receiver);
    },
    has: function (t, prop) {
      if (prop === "length") return true;
      if (typeof prop === "string" && /^\d+$/.test(prop)) {
        return Number(prop) < getList().length;
      }
      if (prop in t) return true;
      if (typeof prop === "string") {
        return findNamedItemInList(getList(), prop) !== null;
      }
      return false;
    },
    ownKeys: function (t) {
      const list = getList();
      const keys = [];
      for (let i = 0; i < list.length; i++) {
        keys.push(String(i));
      }
      for (let i = 0; i < list.length; i++) {
        const el = list[i];
        if (!el) continue;
        const id = el.id || (el.getAttribute && el.getAttribute("id"));
        if (id && keys.indexOf(id) === -1) keys.push(id);
        const name = el.name || (el.getAttribute && el.getAttribute("name"));
        if (name && keys.indexOf(name) === -1) keys.push(name);
      }
      return keys;
    },
    getOwnPropertyDescriptor: function (t, prop) {
      if (prop === "length") {
        return { value: getList().length, writable: false, enumerable: false, configurable: true };
      }
      if (typeof prop === "string" && /^\d+$/.test(prop)) {
        const i = Number(prop);
        const list = getList();
        if (i >= 0 && i < list.length) {
          return { value: list[i], writable: false, enumerable: true, configurable: true };
        }
      }
      if (typeof prop === "string") {
        const named = findNamedItemInList(getList(), prop);
        if (named !== null) {
          return { value: named, writable: false, enumerable: false, configurable: true };
        }
      }
      return Reflect.getOwnPropertyDescriptor(t, prop);
    }
  });
  return proxy;
}

function getAllDocumentElements(doc) {
  const elements = [];
  function walk(node) {
    if (!node) return;
    if (node._children && node._children.length > 0) {
      for (let i = 0; i < node._children.length; i++) {
        const c = node._children[i];
        if (c && c.nodeType === 1) { // ELEMENT_NODE
          elements.push(c);
          walk(c);
        }
      }
    } else {
      let child = node.firstChild;
      while (child) {
        if (child.nodeType === 1) {
          elements.push(child);
          walk(child);
        }
        child = child.nextSibling;
      }
    }
  }

  const root = doc ? (doc.documentElement || doc.body || doc) : null;
  if (root) {
    if (root.nodeType === 1) {
      elements.push(root);
      walk(root);
    } else {
      walk(root);
    }
  }
  return elements;
}

function HTMLCollection(items) {
  const list = items || [];
  return makeHtmlCollection(function () { return list; });
}
HTMLCollection.prototype.item = function (index) {
  const i = Number(index);
  return (i >= 0 && i < this.length) ? this[i] : null;
};
HTMLCollection.prototype.namedItem = function (name) {
  return findNamedItemInList(this, name);
};
HTMLCollection.prototype[Symbol.iterator] = function () {
  const self = this;
  let idx = 0;
  return {
    next: function () {
      if (idx < self.length) {
        return { value: self[idx++], done: false };
      }
      return { value: undefined, done: true };
    }
  };
};
markNative(HTMLCollection, "HTMLCollection");

function NodeList(items) {
  const list = items || [];
  for (let i = 0; i < list.length; i++) {
    this[i] = list[i];
  }
  this.length = list.length;
}
NodeList.prototype.item = function (index) {
  const i = Number(index);
  return (i >= 0 && i < this.length) ? this[i] : null;
};
NodeList.prototype.forEach = function (callback, thisArg) {
  for (let i = 0; i < this.length; i++) {
    callback.call(thisArg, this[i], i, this);
  }
};
NodeList.prototype[Symbol.iterator] = function () {
  const self = this;
  let idx = 0;
  return {
    next: function () {
      if (idx < self.length) {
        return { value: self[idx++], done: false };
      }
      return { value: undefined, done: true };
    }
  };
};
markNative(NodeList, "NodeList");

function Attr(name, value, ownerElement) {
  this.name = String(name);
  this._value = value == null ? "" : String(value);
  this.specified = true;
  this.ownerElement = ownerElement || null;
}

Object.defineProperty(Attr.prototype, "value", {
  get: function () {
    return this._value;
  },
  set: function (v) {
    const next = v == null ? "" : String(v);
    this._value = next;
    if (this.ownerElement && typeof this.ownerElement.setAttribute === "function") {
      this.ownerElement.setAttribute(this.name, next);
    }
  },
  configurable: true,
});

Object.defineProperty(Attr.prototype, "nodeName", {
  get: function () { return this.name; },
  configurable: true,
});

Object.defineProperty(Attr.prototype, "nodeValue", {
  get: function () { return this.value; },
  set: function (v) { this.value = v; },
  configurable: true,
});

Object.defineProperty(Attr.prototype, "textContent", {
  get: function () { return this.value; },
  set: function (v) { this.value = v; },
  configurable: true,
});

Object.defineProperty(Attr.prototype, "localName", {
  get: function () { return this.name.toLowerCase(); },
  configurable: true,
});

Object.defineProperty(Attr.prototype, "nodeType", {
  value: 2, // ATTRIBUTE_NODE
  writable: false,
  configurable: true,
});

markNative(Attr, "Attr");

function NamedNodeMap(ownerElement) {
  Object.defineProperty(this, "_owner", {
    value: ownerElement || null,
    writable: true,
    enumerable: false,
    configurable: true,
  });
  Object.defineProperty(this, "_list", {
    value: [],
    writable: true,
    enumerable: false,
    configurable: true,
  });
  Object.defineProperty(this, "_byName", {
    value: Object.create(null),
    writable: true,
    enumerable: false,
    configurable: true,
  });
  Object.defineProperty(this, "_length", {
    value: 0,
    writable: true,
    enumerable: false,
    configurable: true,
  });
  this._sync();
}

NamedNodeMap.prototype._sync = function () {
  if (!this._owner) return;
  if (typeof this._owner._syncAttrs === "function") {
    this._owner._syncAttrs();
  }
  const attrsMap = this._owner._attrs;
  const currentAttrs = attrsMap ? Array.from(attrsMap.values()) : [];

  for (let i = 0; i < this._length; i++) {
    delete this[i];
  }

  this._list = [];
  this._byName = Object.create(null);

  for (let i = 0; i < currentAttrs.length; i++) {
    const item = currentAttrs[i];
    const attrNode = new Attr(item.name, item.value, this._owner);
    this[i] = attrNode;
    this._list.push(attrNode);
    this._byName[item.name.toLowerCase()] = attrNode;
  }
  this._length = this._list.length;
};

Object.defineProperty(NamedNodeMap.prototype, "length", {
  get: function () {
    this._sync();
    return this._length;
  },
  configurable: true,
  enumerable: false,
});

NamedNodeMap.prototype.item = function (index) {
  this._sync();
  const i = Number(index);
  return (i >= 0 && i < this._length) ? this[i] : null;
};

NamedNodeMap.prototype.getNamedItem = function (name) {
  this._sync();
  if (typeof name !== "string") name = String(name);
  return this._byName[name.toLowerCase()] || null;
};

NamedNodeMap.prototype.setNamedItem = function (attr) {
  if (!attr || !attr.name) throw new TypeError("Attr required");
  const old = this.getNamedItem(attr.name);
  if (this._owner && typeof this._owner.setAttribute === "function") {
    this._owner.setAttribute(attr.name, attr.value);
  }
  this._sync();
  return old;
};

NamedNodeMap.prototype.removeNamedItem = function (name) {
  this._sync();
  const key = String(name).toLowerCase();
  const old = this._byName[key];
  if (!old) {
    const err = new Error("NotFoundError: No attribute named '" + name + "'");
    err.name = "NotFoundError";
    throw err;
  }
  if (this._owner && typeof this._owner.removeAttribute === "function") {
    this._owner.removeAttribute(old.name);
  }
  this._sync();
  return old;
};

NamedNodeMap.prototype[Symbol.iterator] = function () {
  this._sync();
  const list = this._list.slice();
  let idx = 0;
  return {
    next: function () {
      if (idx < list.length) {
        return { value: list[idx++], done: false };
      }
      return { value: undefined, done: true };
    }
  };
};

markNative(NamedNodeMap, "NamedNodeMap");
markNative(NamedNodeMap.prototype.item, "item");
markNative(NamedNodeMap.prototype.getNamedItem, "getNamedItem");
markNative(NamedNodeMap.prototype.setNamedItem, "setNamedItem");
markNative(NamedNodeMap.prototype.removeNamedItem, "removeNamedItem");

if (typeof globalThis !== "undefined") {
  globalThis.NamedNodeMap = NamedNodeMap;
  globalThis.Attr = Attr;
}
if (typeof window !== "undefined") {
  window.NamedNodeMap = NamedNodeMap;
  window.Attr = Attr;
}
