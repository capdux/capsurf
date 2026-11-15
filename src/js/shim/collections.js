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

function HTMLCollection(items) {
  const list = items || [];
  for (let i = 0; i < list.length; i++) {
    this[i] = list[i];
  }
  this.length = list.length;
}
HTMLCollection.prototype.item = function (index) {
  const i = Number(index);
  return (i >= 0 && i < this.length) ? this[i] : null;
};
HTMLCollection.prototype.namedItem = function (name) {
  for (let i = 0; i < this.length; i++) {
    const el = this[i];
    if (el.id === name || (el.getAttribute && el.getAttribute("name") === name)) {
      return el;
    }
  }
  return null;
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
