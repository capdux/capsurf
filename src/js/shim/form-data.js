// src/js/shim/form-data.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/form-data.ts

function FormData(form, submitter) {
  this._entries = []; // Array of [name, value]
  if (form && (form.tagName || "").toUpperCase() === "FORM") {
    let submitterEl = null;
    if (submitter && submitter.name && !submitter.disabled) {
      submitterEl = submitter;
    }
    let submitterAppended = false;
    const controls = form.elements || [];
    for (let i = 0; i < controls.length; i++) {
      const c = controls[i];
      if (!c.name || c.disabled) continue;
      const tag = (c.tagName || "").toLowerCase();
      const type = (c.type || "").toLowerCase();

      if (tag === "button" || type === "submit" || type === "reset" || type === "button" || type === "image") {
        if (c === submitterEl) {
          this.append(c.name, c.value !== undefined ? c.value : "");
          submitterAppended = true;
        }
        continue;
      }
      if ((type === "checkbox" || type === "radio") && !c.checked) {
        continue;
      }
      if (tag === "select") {
        const opts = c.options;
        for (let j = 0; j < opts.length; j++) {
          if (opts[j].selected && !opts[j].disabled) {
            this.append(c.name, opts[j].value);
          }
        }
      } else {
        this.append(c.name, c.value);
      }
    }
    if (!submitterAppended && submitterEl) {
      this.append(submitterEl.name, submitterEl.value !== undefined ? submitterEl.value : "");
    }
  }
}

FormData.prototype.append = function (name, value, filename) {
  this._entries.push([String(name), String(value)]);
};

FormData.prototype.delete = function (name) {
  const n = String(name);
  this._entries = this._entries.filter(function (entry) {
    return entry[0] !== n;
  });
};

FormData.prototype.get = function (name) {
  const n = String(name);
  for (let i = 0; i < this._entries.length; i++) {
    if (this._entries[i][0] === n) {
      return this._entries[i][1];
    }
  }
  return null;
};

FormData.prototype.getAll = function (name) {
  const n = String(name);
  const res = [];
  for (let i = 0; i < this._entries.length; i++) {
    if (this._entries[i][0] === n) {
      res.push(this._entries[i][1]);
    }
  }
  return res;
};

FormData.prototype.has = function (name) {
  const n = String(name);
  for (let i = 0; i < this._entries.length; i++) {
    if (this._entries[i][0] === n) return true;
  }
  return false;
};

FormData.prototype.set = function (name, value, filename) {
  const n = String(name);
  const val = String(value);
  let found = false;
  const newEntries = [];
  for (let i = 0; i < this._entries.length; i++) {
    if (this._entries[i][0] === n) {
      if (!found) {
        newEntries.push([n, val]);
        found = true;
      }
    } else {
      newEntries.push(this._entries[i]);
    }
  }
  if (!found) {
    newEntries.push([n, val]);
  }
  this._entries = newEntries;
};

FormData.prototype.entries = function () {
  return this._entries.slice()[Symbol.iterator]();
};

FormData.prototype.keys = function () {
  return this._entries.map(function (e) { return e[0]; })[Symbol.iterator]();
};

FormData.prototype.values = function () {
  return this._entries.map(function (e) { return e[1]; })[Symbol.iterator]();
};

FormData.prototype.forEach = function (callback, thisArg) {
  for (let i = 0; i < this._entries.length; i++) {
    callback.call(thisArg, this._entries[i][1], this._entries[i][0], this);
  }
};

FormData.prototype[Symbol.iterator] = function () {
  return this.entries();
};

markNative(FormData, "FormData");
markNative(FormData.prototype.append, "append");
markNative(FormData.prototype.delete, "delete");
markNative(FormData.prototype.get, "get");
markNative(FormData.prototype.getAll, "getAll");
markNative(FormData.prototype.has, "has");
markNative(FormData.prototype.set, "set");
markNative(FormData.prototype.entries, "entries");

try {
  Object.defineProperty(FormData, "length", { value: 0, configurable: true });
} catch (e) {}
