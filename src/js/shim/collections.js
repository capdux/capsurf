// src/js/shim/collections.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/collections.ts

const _dtlState = new WeakMap();

function DOMTokenList(getter, setter, supportedSet) {
  const st = { getter: getter, setter: setter, supportedSet: supportedSet || null };
  _dtlState.set(this, st);
  const proxy = new Proxy(this, {
    get: function (target, prop, receiver) {
      if (typeof prop === "string" && /^\d+$/.test(prop)) {
        const idx = Number(prop);
        const tokens = target._getTokens();
        return idx < tokens.length ? tokens[idx] : undefined;
      }
      return Reflect.get(target, prop, receiver);
    },
    has: function (target, prop) {
      if (typeof prop === "string" && /^\d+$/.test(prop)) {
        return Number(prop) < target.length;
      }
      return Reflect.has(target, prop);
    },
  });
  _dtlState.set(proxy, st);
  return proxy;
}

function validateDOMToken(token, methodName) {
  const str = String(token);
  if (str === "") {
    throw new DOMException("Failed to execute '" + methodName + "' on 'DOMTokenList': The token provided must not be empty.", "SyntaxError");
  }
  if (/[\t\n\f\r ]/.test(str)) {
    throw new DOMException("Failed to execute '" + methodName + "' on 'DOMTokenList': The token provided ('" + str + "') contains HTML space characters, which are not valid in tokens.", "InvalidCharacterError");
  }
  return str;
}

DOMTokenList.prototype._getTokens = function () {
  const st = _dtlState.get(this);
  const str = (st ? (st.getter() || "") : "").trim();
  if (!str) return [];
  const raw = str.split(/\s+/);
  const out = [];
  const seen = new Set();
  for (let i = 0; i < raw.length; i++) {
    const tok = raw[i];
    if (tok && !seen.has(tok)) {
      seen.add(tok);
      out.push(tok);
    }
  }
  return out;
};

DOMTokenList.prototype._setTokens = function (tokens) {
  const st = _dtlState.get(this);
  if (st) st.setter(tokens.join(" "));
};

Object.defineProperty(DOMTokenList.prototype, "length", {
  get: function () {
    return this._getTokens().length;
  },
  configurable: true,
});

Object.defineProperty(DOMTokenList.prototype, "value", {
  get: function () {
    const st = _dtlState.get(this);
    return st ? (st.getter() || "") : "";
  },
  set: function (v) {
    const st = _dtlState.get(this);
    if (st) st.setter(String(v == null ? "" : v));
  },
  configurable: true,
});

DOMTokenList.prototype.item = function (index) {
  const tokens = this._getTokens();
  const i = Number(index);
  return (i >= 0 && i < tokens.length) ? tokens[i] : null;
};

DOMTokenList.prototype.contains = function (token) {
  const tokens = this._getTokens();
  return tokens.indexOf(String(token)) !== -1;
};

DOMTokenList.prototype.add = function () {
  const validated = [];
  for (let i = 0; i < arguments.length; i++) {
    validated.push(validateDOMToken(arguments[i], "add"));
  }
  if (validated.length === 0) return;
  const tokens = this._getTokens();
  for (let i = 0; i < validated.length; i++) {
    if (tokens.indexOf(validated[i]) === -1) {
      tokens.push(validated[i]);
    }
  }
  this._setTokens(tokens);
};

DOMTokenList.prototype.remove = function () {
  const validated = [];
  for (let i = 0; i < arguments.length; i++) {
    validated.push(validateDOMToken(arguments[i], "remove"));
  }
  if (validated.length === 0) return;
  const tokens = this._getTokens();
  for (let i = 0; i < validated.length; i++) {
    const idx = tokens.indexOf(validated[i]);
    if (idx !== -1) {
      tokens.splice(idx, 1);
    }
  }
  this._setTokens(tokens);
};

DOMTokenList.prototype.toggle = function (token, force) {
  const str = validateDOMToken(token, "toggle");
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
  const oldStr = validateDOMToken(oldToken, "replace");
  const newStr = validateDOMToken(newToken, "replace");
  const tokens = this._getTokens();
  const idx = tokens.indexOf(oldStr);
  if (idx === -1) return false;
  tokens[idx] = newStr;
  const deduped = [];
  const seen = new Set();
  for (let i = 0; i < tokens.length; i++) {
    if (!seen.has(tokens[i])) {
      seen.add(tokens[i]);
      deduped.push(tokens[i]);
    }
  }
  this._setTokens(deduped);
  return true;
};

DOMTokenList.prototype.forEach = Array.prototype.forEach;
DOMTokenList.prototype.keys = Array.prototype.keys;
DOMTokenList.prototype.values = Array.prototype.values;
DOMTokenList.prototype.entries = Array.prototype.entries;

DOMTokenList.prototype.toString = function () {
  const st = _dtlState.get(this);
  return st ? (st.getter() || "") : "";
};

DOMTokenList.prototype.supports = function (token) {
  const st = _dtlState.get(this);
  if (!st || !st.supportedSet) {
    throw new TypeError("Failed to execute 'supports' on 'DOMTokenList': DOMTokenList has no supported tokens.");
  }
  return st.supportedSet.has(String(token).toLowerCase());
};

DOMTokenList.prototype[Symbol.iterator] = Array.prototype[Symbol.iterator];
Object.defineProperty(DOMTokenList.prototype, Symbol.toStringTag, {
  value: "DOMTokenList",
  configurable: true,
});

markNative(DOMTokenList, "DOMTokenList");
markNative(DOMTokenList.prototype.add, "add");
markNative(DOMTokenList.prototype.remove, "remove");
markNative(DOMTokenList.prototype.toggle, "toggle");
markNative(DOMTokenList.prototype.contains, "contains");
markNative(DOMTokenList.prototype.item, "item");

function isArrayIndex(prop) {
  if (typeof prop !== "string") return false;
  if (prop === "0") return true;
  if (!/^[1-9]\d*$/.test(prop)) return false;
  const n = Number(prop);
  return n >= 0 && n < 4294967295;
}

function isHtmlNsElement(el) {
  const ns = el && el.namespaceURI;
  return !ns || ns === "http://www.w3.org/1999/xhtml";
}

function findNamedItemInList(list, name) {
  if (name == null || name === "") return null;
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
    if (!el || !isHtmlNsElement(el)) continue;
    if (el.name === key || (el.getAttribute && el.getAttribute("name") === key)) {
      return el;
    }
  }
  return null;
}

function makeHtmlCollection(getList, customTarget) {
  const target = customTarget || Object.create(HTMLCollection.prototype);
  const proxy = new Proxy(target, {
    get: function (t, prop, receiver) {
      if (prop === "length") {
        if (receiver !== proxy) {
          throw new TypeError("Illegal invocation");
        }
        return getList().length;
      }
      if (isArrayIndex(prop)) {
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
          const i = Number(index) >>> 0;
          return i < list.length ? list[i] : null;
        };
      }
      if (prop === "namedItem") {
        return function (name) {
          const key = String(name);
          if (!key) return null;
          const list = getList();
          const matches = [];
          for (let i = 0; i < list.length; i++) {
            const item = list[i];
            if (!item) continue;
            const itemId = item.id || (typeof item.getAttribute === "function" ? item.getAttribute("id") : "");
            const itemName = item.name || (typeof item.getAttribute === "function" ? item.getAttribute("name") : "");
            if (itemId === key || itemName === key) {
              matches.push(item);
            }
          }
          if (matches.length === 0) return null;
          if (matches.length === 1) return matches[0];
          if (t._isFormControlsCollection) {
            return new RadioNodeList(function () {
              const curList = getList();
              const m = [];
              for (let i = 0; i < curList.length; i++) {
                const it = curList[i];
                if (!it) continue;
                const itId = it.id || (typeof it.getAttribute === "function" ? it.getAttribute("id") : "");
                const itName = it.name || (typeof it.getAttribute === "function" ? it.getAttribute("name") : "");
                if (itId === key || itName === key) m.push(it);
              }
              return m;
            });
          }
          return matches[0];
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
        const key = prop;
        const list = getList();
        if (t._isFormControlsCollection) {
          const matches = [];
          for (let i = 0; i < list.length; i++) {
            const item = list[i];
            if (!item) continue;
            const itemId = item.id || (typeof item.getAttribute === "function" ? item.getAttribute("id") : "");
            const itemName = item.name || (typeof item.getAttribute === "function" ? item.getAttribute("name") : "");
            if (itemId === key || itemName === key) {
              matches.push(item);
            }
          }
          if (matches.length === 1) return matches[0];
          if (matches.length > 1) {
            return new RadioNodeList(function () {
              const curList = getList();
              const m = [];
              for (let i = 0; i < curList.length; i++) {
                const it = curList[i];
                if (!it) continue;
                const itId = it.id || (typeof it.getAttribute === "function" ? it.getAttribute("id") : "");
                const itName = it.name || (typeof it.getAttribute === "function" ? it.getAttribute("name") : "");
                if (itId === key || itName === key) m.push(it);
              }
              return m;
            });
          }
        } else {
          const named = findNamedItemInList(list, prop);
          if (named !== null) {
            return named;
          }
        }
      }
      return Reflect.get(t, prop, receiver);
    },
    set: function (t, prop, value, receiver) {
      if (t._isOptionsCollection && t._owner) {
        if (prop === "length") {
          setSelectOptionsLength(t._owner, value);
          return true;
        }
        if (isArrayIndex(prop)) {
          setSelectOptionAt(t._owner, Number(prop), value);
          return true;
        }
      } else {
        if (prop === "length") return false;
        if (isArrayIndex(prop)) return false;
      }
      if (receiver === proxy && typeof prop === "string" && !Object.prototype.hasOwnProperty.call(t, prop)) {
        if (findNamedItemInList(getList(), prop) !== null) {
          return false;
        }
      }
      return Reflect.set(t, prop, value, receiver);
    },
    defineProperty: function (t, prop, desc) {
      if (prop === "length") return false;
      if (!t._isOptionsCollection && isArrayIndex(prop)) return false;
      if (typeof prop === "string" && !Object.prototype.hasOwnProperty.call(t, prop)) {
        if (findNamedItemInList(getList(), prop) !== null) {
          return false;
        }
      }
      return Reflect.defineProperty(t, prop, desc);
    },
    has: function (t, prop) {
      if (prop === "length") return true;
      if (isArrayIndex(prop)) {
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
        if (id && !(id in t) && keys.indexOf(id) === -1) keys.push(id);
        if (isHtmlNsElement(el)) {
          const name = el.name || (el.getAttribute && el.getAttribute("name"));
          if (name && !(name in t) && keys.indexOf(name) === -1) keys.push(name);
        }
      }
      const own = Reflect.ownKeys(t);
      for (let i = 0; i < own.length; i++) {
        if (keys.indexOf(own[i]) === -1) keys.push(own[i]);
      }
      return keys;
    },
    getOwnPropertyDescriptor: function (t, prop) {
      if (isArrayIndex(prop)) {
        const i = Number(prop);
        const list = getList();
        if (i >= 0 && i < list.length) {
          return { value: list[i], writable: !!t._isOptionsCollection, enumerable: true, configurable: true };
        }
        return undefined;
      }
      if (Object.prototype.hasOwnProperty.call(t, prop)) {
        return Reflect.getOwnPropertyDescriptor(t, prop);
      }
      if (typeof prop === "string") {
        const list = getList();
        if (t._isFormControlsCollection) {
          const key = prop;
          const matches = [];
          for (let i = 0; i < list.length; i++) {
            const item = list[i];
            if (!item) continue;
            const itemId = item.id || (typeof item.getAttribute === "function" ? item.getAttribute("id") : "");
            const itemName = isHtmlNsElement(item) ? (item.name || (typeof item.getAttribute === "function" ? item.getAttribute("name") : "")) : "";
            if (itemId === key || itemName === key) {
              matches.push(item);
            }
          }
          if (matches.length === 1) {
            return { value: matches[0], writable: false, enumerable: false, configurable: true };
          }
          if (matches.length > 1) {
            return { value: new RadioNodeList(matches), writable: false, enumerable: false, configurable: true };
          }
        } else {
          const named = findNamedItemInList(list, prop);
          if (named !== null) {
            return { value: named, writable: false, enumerable: false, configurable: true };
          }
        }
      }
      return undefined;
    },
    deleteProperty: function (t, prop) {
      if (isArrayIndex(prop)) {
        return Number(prop) >= getList().length;
      }
      if (Object.prototype.hasOwnProperty.call(t, prop)) {
        return Reflect.deleteProperty(t, prop);
      }
      if (typeof prop === "string") {
        if (findNamedItemInList(getList(), prop) !== null) {
          return false;
        }
      }
      return Reflect.deleteProperty(t, prop);
    }
  });
  return proxy;
}

function getAllDocumentElements(doc) {
  const elements = [];
  const seen = (typeof Set !== "undefined") ? new Set() : null;

  function addEl(el) {
    if (!el || el.nodeType !== 1) return;
    if (seen) {
      if (seen.has(el)) return;
      seen.add(el);
    } else {
      if (elements.indexOf(el) !== -1) return;
    }
    elements.push(el);
  }

  if (doc && typeof doc.querySelectorAll === "function") {
    try {
      const q = doc.querySelectorAll("*");
      if (q && typeof q.length === "number") {
        for (let i = 0; i < q.length; i++) {
          addEl(q[i]);
        }
      }
    } catch (e) {}
  }

  function walk(node) {
    if (!node) return;
    if (node._children && node._children.length > 0) {
      for (let i = 0; i < node._children.length; i++) {
        const c = node._children[i];
        if (c && c.nodeType === 1) { // ELEMENT_NODE
          addEl(c);
          walk(c);
        }
      }
    } else {
      let child = node.firstChild;
      while (child) {
        if (child.nodeType === 1) {
          addEl(child);
          walk(child);
        }
        child = child.nextSibling;
      }
    }
  }

  const root = doc ? (doc.documentElement || doc.body || doc) : null;
  if (root) {
    if (root.nodeType === 1) {
      addEl(root);
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
  const i = Number(index) >>> 0;
  return i < this.length ? this[i] : null;
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

function HTMLFormControlsCollection() {
  throw new TypeError("Illegal constructor");
}
HTMLFormControlsCollection.prototype = Object.create(HTMLCollection.prototype);
HTMLFormControlsCollection.prototype.constructor = HTMLFormControlsCollection;
Object.defineProperty(HTMLFormControlsCollection.prototype, Symbol.toStringTag, {
  value: "HTMLFormControlsCollection",
  configurable: true,
});
HTMLFormControlsCollection.prototype.namedItem = function (name) {
  const key = String(name);
  if (!key) return null;
  const matches = [];
  for (let i = 0; i < this.length; i++) {
    const item = this[i];
    if (!item) continue;
    const itemId = item.id || (typeof item.getAttribute === "function" ? item.getAttribute("id") : "");
    const itemName = item.name || (typeof item.getAttribute === "function" ? item.getAttribute("name") : "");
    if (itemId === key || itemName === key) {
      matches.push(item);
    }
  }
  if (matches.length === 0) return null;
  if (matches.length === 1) return matches[0];
  return new RadioNodeList(matches);
};
markNative(HTMLFormControlsCollection, "HTMLFormControlsCollection");
markNative(HTMLFormControlsCollection.prototype.namedItem, "namedItem");

function isRadio(node) {
  return node && (node.tagName || "").toLowerCase() === "input" && (node.type || "").toLowerCase() === "radio";
}

function RadioNodeList(itemsOrGetter) {
  if (typeof itemsOrGetter === "function") {
    const getItems = itemsOrGetter;
    return new Proxy(this, {
      get: function (target, prop, receiver) {
        if (prop === "length") return getItems().length;
        if (prop === "item") return function (idx) { const l = getItems(); const i = Number(idx); return i >= 0 && i < l.length ? l[i] : null; };
        if (prop === Symbol.iterator) {
          return function () {
            const l = getItems();
            let idx = 0;
            return { next: function () { return idx < l.length ? { value: l[idx++], done: false } : { value: undefined, done: true }; } };
          };
        }
        if (typeof prop === "string" && /^\d+$/.test(prop)) {
          const l = getItems();
          const i = Number(prop);
          return i >= 0 && i < l.length ? l[i] : undefined;
        }
        return Reflect.get(target, prop, receiver);
      },
      has: function (target, prop) {
        if (prop === "length" || prop === "item") return true;
        if (typeof prop === "string" && /^\d+$/.test(prop)) return Number(prop) < getItems().length;
        return Reflect.has(target, prop);
      },
      ownKeys: function (target) {
        const l = getItems();
        const keys = [];
        for (let i = 0; i < l.length; i++) keys.push(String(i));
        return keys;
      },
      getOwnPropertyDescriptor: function (target, prop) {
        if (typeof prop === "string" && /^\d+$/.test(prop)) {
          const l = getItems();
          const i = Number(prop);
          if (i >= 0 && i < l.length) return { value: l[i], writable: false, enumerable: true, configurable: true };
        }
        return Reflect.getOwnPropertyDescriptor(target, prop);
      },
    });
  }
  const list = itemsOrGetter || [];
  for (let i = 0; i < list.length; i++) {
    Object.defineProperty(this, i, {
      value: list[i],
      writable: false,
      enumerable: true,
      configurable: true,
    });
  }
  Object.defineProperty(this, "length", {
    value: list.length,
    writable: false,
    enumerable: false,
    configurable: true,
  });
}
RadioNodeList.prototype = Object.create(NodeList.prototype);
RadioNodeList.prototype.constructor = RadioNodeList;
Object.defineProperty(RadioNodeList.prototype, Symbol.toStringTag, {
  value: "RadioNodeList",
  configurable: true,
});
Object.defineProperty(RadioNodeList.prototype, "value", {
  get: function () {
    for (let i = 0; i < this.length; i++) {
      const node = this[i];
      if (isRadio(node) && node.checked) return node.value || "on";
    }
    return "";
  },
  set: function (v) {
    const val = String(v);
    for (let i = 0; i < this.length; i++) {
      const node = this[i];
      if (isRadio(node) && (node.value || "on") === val) {
        node.checked = true;
        return;
      }
    }
  },
  configurable: true,
});
markNative(RadioNodeList, "RadioNodeList");

function HTMLOptionsCollection() {
  throw new TypeError("Illegal constructor");
}
HTMLOptionsCollection.prototype = Object.create(HTMLCollection.prototype);
HTMLOptionsCollection.prototype.constructor = HTMLOptionsCollection;
Object.defineProperty(HTMLOptionsCollection.prototype, Symbol.toStringTag, {
  value: "HTMLOptionsCollection",
  configurable: true,
});
markNative(HTMLOptionsCollection, "HTMLOptionsCollection");

Object.defineProperty(HTMLOptionsCollection.prototype, "length", {
  get: function () {
    const select = this._owner;
    if (!select) return 0;
    return getSelectOptions(select).length;
  },
  set: function (v) {
    const select = this._owner;
    if (!select) return;
    setSelectOptionsLength(select, v);
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(HTMLOptionsCollection.prototype, "selectedIndex", {
  get: function () {
    const select = this._owner;
    return select ? select.selectedIndex : -1;
  },
  set: function (v) {
    const select = this._owner;
    if (select) select.selectedIndex = v;
  },
  configurable: true,
  enumerable: true,
});

HTMLOptionsCollection.prototype.add = function (element, before = null) {
  const select = this._owner;
  if (!select) {
    throw new TypeError("HTMLOptionsCollection.add: missing owner select");
  }
  const tag = element && typeof element.tagName === "string" ? element.tagName.toLowerCase() : "";
  if (tag !== "option" && tag !== "optgroup") {
    throw new TypeError("HTMLOptionsCollection.add: argument must be HTMLOptionElement or HTMLOptGroupElement");
  }
  if (before === element) return;
  let refNode = null;
  if (before == null) {
    refNode = null;
  } else if (typeof before === "number") {
    const idx = Number(before) | 0;
    const list = getSelectOptions(select);
    if (idx >= 0 && idx < list.length) {
      refNode = list[idx];
    }
  } else if (typeof before === "object") {
    refNode = before;
  }
  if (refNode) {
    let p = refNode.parentNode;
    let isDesc = false;
    while (p) {
      if (p === select) {
        isDesc = true;
        break;
      }
      p = p.parentNode;
    }
    if (!isDesc) {
      const DEx = typeof DOMException !== "undefined" ? DOMException : Error;
      throw new DEx("HTMLOptionsCollection.add: before is not a descendant of the select", "NotFoundError");
    }
    if (refNode.parentNode && typeof refNode.parentNode.insertBefore === "function") {
      refNode.parentNode.insertBefore(element, refNode);
    } else {
      select.appendChild(element);
    }
  } else {
    select.appendChild(element);
  }
};
markNative(HTMLOptionsCollection.prototype.add, "add");

HTMLOptionsCollection.prototype.remove = function (index) {
  const select = this._owner;
  if (!select) return;
  const i = Number(index) | 0;
  const list = getSelectOptions(select);
  if (i >= 0 && i < list.length) {
    const o = list[i];
    if (o && o.parentNode && typeof o.parentNode.removeChild === "function") {
      o.parentNode.removeChild(o);
    }
  }
};
markNative(HTMLOptionsCollection.prototype.remove, "remove");

function getSelectOptions(selectEl) {
  const opts = [];
  const seen = new Set();
  const up = typeof upgradeElement === "function" ? upgradeElement : (typeof globalThis !== "undefined" && globalThis.upgradeElement);
  function collect(node) {
    if (!node) return;
    if (node.firstChild !== undefined) {
      let child = node.firstChild;
      while (child && !seen.has(child)) {
        seen.add(child);
        const tag = (child.tagName || "").toLowerCase();
        if (tag === "option") {
          if (up) up(child);
          opts.push(child);
        } else if (tag === "optgroup") {
          collect(child);
        }
        child = child.nextSibling;
      }
      return;
    }
    if (node._children) {
      for (let i = 0; i < node._children.length; i++) {
        const c = node._children[i];
        if (!c) continue;
        const tag = (c.tagName || "").toLowerCase();
        if (tag === "option") {
          if (up) up(c);
          opts.push(c);
        } else if (tag === "optgroup") {
          collect(c);
        }
      }
    }
  }
  collect(selectEl);
  return opts;
}

function setSelectOptionsLength(select, v) {
  const target = Math.max(0, Math.min(100000, Math.floor(Number(v) || 0)));
  const list = getSelectOptions(select);
  const cur = list.length;
  if (target < cur) {
    for (let i = cur - 1; i >= target; i--) {
      const o = list[i];
      if (o && o.parentNode && typeof o.parentNode.removeChild === "function") {
        o.parentNode.removeChild(o);
      }
    }
  } else if (target > cur) {
    const doc = select.ownerDocument || (typeof document !== "undefined" ? document : null);
    for (let i = cur; i < target; i++) {
      const opt = doc && typeof doc.createElement === "function" ? doc.createElement("option") : (typeof HTMLOptionElement === "function" ? new HTMLOptionElement() : null);
      if (opt) select.appendChild(opt);
    }
  }
}

function setSelectOptionAt(select, idx, value) {
  const list = getSelectOptions(select);
  const doc = select.ownerDocument || (typeof document !== "undefined" ? document : null);
  if (value === null || value === undefined) {
    if (idx >= 0 && idx < list.length) {
      const o = list[idx];
      if (o && o.parentNode && typeof o.parentNode.removeChild === "function") {
        o.parentNode.removeChild(o);
      }
    }
    return;
  }
  const tag = value && typeof value.tagName === "string" ? value.tagName.toLowerCase() : "";
  if (tag !== "option") {
    throw new TypeError("HTMLOptionsCollection: indexed value must be an HTMLOptionElement");
  }
  if (idx < list.length) {
    const o = list[idx];
    if (o && o.parentNode && typeof o.parentNode.replaceChild === "function") {
      o.parentNode.replaceChild(value, o);
    }
  } else {
    for (let i = list.length; i < idx; i++) {
      const opt = doc && typeof doc.createElement === "function" ? doc.createElement("option") : (typeof HTMLOptionElement === "function" ? new HTMLOptionElement() : null);
      if (opt) select.appendChild(opt);
    }
    select.appendChild(value);
  }
}

function makeHtmlOptionsCollection(select) {
  const getList = function () {
    return getSelectOptions(select);
  };
  const target = Object.create(HTMLOptionsCollection.prototype);
  Object.defineProperty(target, "_owner", {
    value: select,
    writable: true,
    configurable: true,
    enumerable: false,
  });
  Object.defineProperty(target, "_isOptionsCollection", {
    value: true,
    writable: true,
    configurable: true,
    enumerable: false,
  });
  return makeHtmlCollection(getList, target);
}

const selectOptionsCache = new WeakMap();

function getSelectOptionsCollection(select) {
  if (!select || typeof select !== "object") return null;
  let coll = selectOptionsCache.get(select);
  if (!coll) {
    coll = makeHtmlOptionsCollection(select);
    selectOptionsCache.set(select, coll);
  }
  return coll;
}

const elementChildrenCache = new WeakMap();

function getElementChildren(parent) {
  const kids = [];
  const up = typeof upgradeElement === "function" ? upgradeElement : (typeof globalThis !== "undefined" && globalThis.upgradeElement);
  if (parent && parent.nodeType === 9) {
    const de = parent.documentElement;
    if (de && de.nodeType === 1) {
      if (up) up(de);
      kids.push(de);
    }
    return kids;
  }
  if (parent && parent.firstChild !== undefined) {
    let child = parent.firstChild;
    while (child) {
      if (child.nodeType === 1) { // ELEMENT_NODE
        if (up) up(child);
        kids.push(child);
      }
      child = child.nextSibling;
    }
    return kids;
  }
  if (parent && parent._children) {
    for (let i = 0; i < parent._children.length; i++) {
      const c = parent._children[i];
      if (c && c.nodeType === 1) {
        if (up) up(c);
        kids.push(c);
      }
    }
  }
  return kids;
}

function getElementChildrenCollection(element) {
  if (!element || typeof element !== "object") return null;
  let coll = elementChildrenCache.get(element);
  if (!coll) {
    coll = makeHtmlCollection(function () {
      return getElementChildren(element);
    });
    elementChildrenCache.set(element, coll);
  }
  return coll;
}

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
    },
    [Symbol.iterator]: function () { return this; }
  };
};
NodeList.prototype.values = NodeList.prototype[Symbol.iterator];
NodeList.prototype.keys = function () {
  const self = this;
  let idx = 0;
  return {
    next: function () {
      if (idx < self.length) {
        return { value: idx++, done: false };
      }
      return { value: undefined, done: true };
    },
    [Symbol.iterator]: function () { return this; }
  };
};
NodeList.prototype.entries = function () {
  const self = this;
  let idx = 0;
  return {
    next: function () {
      if (idx < self.length) {
        const i = idx++;
        return { value: [i, self[i]], done: false };
      }
      return { value: undefined, done: true };
    },
    [Symbol.iterator]: function () { return this; }
  };
};
markNative(NodeList.prototype.item, "item");
markNative(NodeList.prototype.forEach, "forEach");
markNative(NodeList.prototype.keys, "keys");
markNative(NodeList.prototype.values, "values");
markNative(NodeList.prototype.entries, "entries");
markNative(NodeList, "NodeList");

function Attr(name, value, ownerElement, namespaceURI, localName, prefix) {
  this.name = String(name);
  this._value = value == null ? "" : String(value);
  this.specified = true;
  this.ownerElement = ownerElement || null;
  this._namespaceURI = namespaceURI !== undefined ? namespaceURI : null;
  const colon = this.name.indexOf(":");
  this._prefix = prefix !== undefined ? prefix : (colon !== -1 ? this.name.slice(0, colon) : null);
  this._localName = localName !== undefined ? localName : (colon !== -1 ? this.name.slice(colon + 1) : this.name);
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
  get: function () {
    if (this._localName !== undefined) return this._localName;
    const colon = this.name.indexOf(":");
    return colon !== -1 ? this.name.slice(colon + 1) : this.name.toLowerCase();
  },
  configurable: true,
});

Object.defineProperty(Attr.prototype, "prefix", {
  get: function () {
    if (this._prefix !== undefined) return this._prefix;
    const colon = this.name.indexOf(":");
    return colon !== -1 ? this.name.slice(0, colon) : null;
  },
  configurable: true,
});

Object.defineProperty(Attr.prototype, "namespaceURI", {
  get: function () {
    return this._namespaceURI !== undefined ? this._namespaceURI : null;
  },
  configurable: true,
});

Object.defineProperty(Attr.prototype, "nodeType", {
  value: 2, // ATTRIBUTE_NODE
  writable: false,
  configurable: true,
});

Attr.prototype.isSameNode = function (otherNode) {
  return this === otherNode;
};
markNative(Attr.prototype.isSameNode, "isSameNode");

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
  return new Proxy(this, {
    get: function (t, prop, receiver) {
      if (prop === "length") {
        t._sync();
        return t._list.length;
      }
      if (isArrayIndex(prop)) {
        t._sync();
        const i = Number(prop);
        return i < t._list.length ? t._list[i] : undefined;
      }
      if (typeof prop === "string" && prop !== "_owner" && prop !== "_list" && prop !== "_byName" && prop !== "_length" && !(prop in t)) {
        t._sync();
        const attr = t._byName[prop.toLowerCase()];
        if (attr) return attr;
      }
      return Reflect.get(t, prop, receiver);
    },
    has: function (t, prop) {
      if (isArrayIndex(prop)) {
        t._sync();
        return Number(prop) < t._list.length;
      }
      if (typeof prop === "string" && prop !== "_owner" && prop !== "_list" && prop !== "_byName" && prop !== "_length") {
        if (prop in t) return true;
        t._sync();
        return !!t._byName[prop.toLowerCase()];
      }
      return prop in t;
    },
    ownKeys: function (t) {
      t._sync();
      const keys = [];
      for (let i = 0; i < t._list.length; i++) {
        keys.push(String(i));
      }
      for (let i = 0; i < t._list.length; i++) {
        const n = t._list[i] && t._list[i].name;
        if (n && keys.indexOf(n) === -1) keys.push(n);
      }
      const own = Reflect.ownKeys(t);
      for (let i = 0; i < own.length; i++) {
        const k = own[i];
        if (k === "_owner" || k === "_list" || k === "_byName" || k === "_length") continue;
        if (keys.indexOf(k) === -1) keys.push(k);
      }
      return keys;
    },
    getOwnPropertyDescriptor: function (t, prop) {
      t._sync();
      if (isArrayIndex(prop)) {
        const i = Number(prop);
        if (i < t._list.length) {
          return { value: t._list[i], writable: false, enumerable: true, configurable: true };
        }
        return undefined;
      }
      if (typeof prop === "string" && prop !== "_owner" && prop !== "_list" && prop !== "_byName" && prop !== "_length") {
        const attr = t._byName[prop.toLowerCase()];
        if (attr) {
          return { value: attr, writable: false, enumerable: false, configurable: true };
        }
      }
      return Reflect.getOwnPropertyDescriptor(t, prop);
    }
  });
}

NamedNodeMap.prototype._sync = function () {
  if (!this._owner) return;
  if (typeof this._owner._syncAttrs === "function") {
    this._owner._syncAttrs();
  }
  const currentAttrs = this._owner._attrList || (this._owner._attrs ? Array.from(this._owner._attrs.values()) : []);

  const prevByName = this._byName;
  this._list = [];
  this._byName = Object.create(null);

  for (let i = 0; i < currentAttrs.length; i++) {
    const item = currentAttrs[i];
    const colon = item.name.indexOf(":");
    const pfx = item.prefix !== undefined ? item.prefix : (colon !== -1 ? item.name.slice(0, colon) : null);
    const loc = item.localName !== undefined ? item.localName : (colon !== -1 ? item.name.slice(colon + 1) : item.name);
    const ns = item.namespaceURI !== undefined ? item.namespaceURI : null;
    const key = item.name.toLowerCase();
    let attrNode = prevByName && prevByName[key];
    if (attrNode) {
      attrNode.name = String(item.name);
      attrNode._value = item.value == null ? "" : String(item.value);
      attrNode.ownerElement = this._owner;
      attrNode._namespaceURI = ns;
      attrNode._localName = loc;
      attrNode._prefix = pfx;
    } else {
      attrNode = new Attr(item.name, item.value, this._owner, ns, loc, pfx);
    }
    this._list.push(attrNode);
    this._byName[key] = attrNode;
  }
  if (prevByName) {
    for (const k in prevByName) {
      if (!this._byName[k]) {
        prevByName[k].ownerElement = null;
      }
    }
  }
  this._length = this._list.length;
};

Object.defineProperty(NamedNodeMap.prototype, "length", {
  get: function () {
    this._sync();
    return this._list.length;
  },
  configurable: true,
  enumerable: false,
});

NamedNodeMap.prototype.item = function (index) {
  this._sync();
  const i = Number(index) >>> 0;
  return i < this._list.length ? this[i] : null;
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

NamedNodeMap.prototype.getNamedItemNS = function (namespace, localName) {
  this._sync();
  const ns = (namespace === undefined || namespace === null || namespace === "") ? null : String(namespace);
  const loc = String(localName).toLowerCase();
  for (let i = 0; i < this._length; i++) {
    const a = this._list[i];
    const aNs = a.namespaceURI || null;
    const aLoc = (a.localName || a.name).toLowerCase();
    if (aNs === ns && aLoc === loc) return a;
  }
  return null;
};

NamedNodeMap.prototype.setNamedItemNS = function (attr) {
  if (!attr || !attr.name) throw new TypeError("Attr required");
  const old = this.getNamedItemNS(attr.namespaceURI, attr.localName || attr.name);
  if (this._owner && typeof this._owner.setAttributeNS === "function") {
    this._owner.setAttributeNS(attr.namespaceURI, attr.name, attr.value);
  }
  this._sync();
  return old;
};

NamedNodeMap.prototype.removeNamedItemNS = function (namespace, localName) {
  this._sync();
  const old = this.getNamedItemNS(namespace, localName);
  if (!old) {
    const err = new Error("NotFoundError: No attribute matching namespace and localName");
    err.name = "NotFoundError";
    throw err;
  }
  if (this._owner && typeof this._owner.removeAttributeNS === "function") {
    this._owner.removeAttributeNS(namespace, localName);
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
markNative(NamedNodeMap.prototype.getNamedItemNS, "getNamedItemNS");
markNative(NamedNodeMap.prototype.setNamedItem, "setNamedItem");
markNative(NamedNodeMap.prototype.setNamedItemNS, "setNamedItemNS");
markNative(NamedNodeMap.prototype.removeNamedItem, "removeNamedItem");
markNative(NamedNodeMap.prototype.removeNamedItemNS, "removeNamedItemNS");

if (typeof globalThis !== "undefined") {
  globalThis.NamedNodeMap = NamedNodeMap;
  globalThis.Attr = Attr;
  globalThis.HTMLCollection = HTMLCollection;
  globalThis.HTMLOptionsCollection = HTMLOptionsCollection;
  globalThis.HTMLFormControlsCollection = HTMLFormControlsCollection;
  globalThis.RadioNodeList = RadioNodeList;
}
if (typeof window !== "undefined") {
  window.NamedNodeMap = NamedNodeMap;
  window.Attr = Attr;
  window.HTMLCollection = HTMLCollection;
  window.HTMLOptionsCollection = HTMLOptionsCollection;
  window.HTMLFormControlsCollection = HTMLFormControlsCollection;
  window.RadioNodeList = RadioNodeList;
}
