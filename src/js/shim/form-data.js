// src/js/shim/form-data.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/form-data.ts

function isFormControlActuallyDisabled(el) {
  if (!el || el.nodeType !== 1) return false;
  if (el.disabled || (typeof el.hasAttribute === "function" && el.hasAttribute("disabled"))) {
    return true;
  }
  let cur = el;
  let parent = el.parentElement || el.parentNode;
  while (parent && parent.nodeType === 1) {
    const pTag = (parent.tagName || "").toLowerCase();
    if (pTag === "fieldset" && (parent.disabled || (typeof parent.hasAttribute === "function" && parent.hasAttribute("disabled")))) {
      let firstLegend = null;
      let ch = parent.firstChild;
      while (ch) {
        if (ch.nodeType === 1 && (ch.tagName || "").toLowerCase() === "legend") {
          firstLegend = ch;
          break;
        }
        ch = ch.nextSibling;
      }
      if (!firstLegend || cur !== firstLegend) {
        return true;
      }
    }
    cur = parent;
    parent = parent.parentElement || parent.parentNode;
  }
  return false;
}

function FormData(form, submitter) {
  this._entries = []; // Array of [name, value]
  if (form && (form.tagName || "").toUpperCase() === "FORM") {
    if (form._constructingEntryList) {
      const DOMEx = typeof DOMException !== "undefined" ? DOMException : Error;
      throw new DOMEx("Failed to construct 'FormData': The form is already constructing an entry list.", "InvalidStateError");
    }
    let submitterEl = null;
    if (submitter && submitter.name && !isFormControlActuallyDisabled(submitter)) {
      submitterEl = submitter;
    }
    let submitterAppended = false;
    const controls = (typeof collectFormControls === "function") ? collectFormControls(form) : (form.elements || []);
    for (let i = 0; i < controls.length; i++) {
      const c = controls[i];
      const tag = (c.tagName || "").toLowerCase();
      if (tag === "fieldset") continue;
      if (!c.name || isFormControlActuallyDisabled(c)) continue;
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
      } else if (tag === "input" && type === "file") {
        const files = c.files;
        if (files && files.length > 0) {
          for (let j = 0; j < files.length; j++) {
            this.append(c.name, files[j]);
          }
        } else if (typeof File === "function") {
          this.append(c.name, new File([], "", { type: "application/octet-stream" }));
        }
      } else {
        this.append(c.name, c.value);
      }
    }
    if (!submitterAppended && submitterEl) {
      this.append(submitterEl.name, submitterEl.value !== undefined ? submitterEl.value : "");
    }

    // Dispatch "formdata" event per WHATWG HTML spec
    form._constructingEntryList = true;
    try {
      let evt;
      if (typeof FormDataEvent === "function") {
        evt = new FormDataEvent("formdata", { bubbles: true, cancelable: false, formData: this });
      } else {
        evt = new Event("formdata", { bubbles: true, cancelable: false });
        Object.defineProperty(evt, "formData", { value: this, configurable: true });
      }
      if (typeof form.dispatchEvent === "function") {
        form.dispatchEvent(evt);
      }
    } finally {
      form._constructingEntryList = false;
    }
  }
}

function normalizeFormDataValue(value, filename) {
  if (typeof File === "function" && value instanceof File) {
    if (filename !== undefined) {
      return new File([value], String(filename), { type: value.type, lastModified: value.lastModified });
    }
    return value;
  }
  if (typeof Blob === "function" && value instanceof Blob) {
    const fname = filename !== undefined ? String(filename) : "blob";
    if (typeof File === "function") {
      return new File([value], fname, { type: value.type });
    }
    return value;
  }
  return String(value);
}

FormData.prototype.append = function (name, value, filename) {
  this._entries.push([String(name), normalizeFormDataValue(value, filename)]);
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
  const val = normalizeFormDataValue(value, filename);
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
