// src/js/shim/element/forms/select.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/select.ts

if (!globalThis.__capsurf_selectState) globalThis.__capsurf_selectState = new Map();
if (!globalThis.__capsurf_optionState) globalThis.__capsurf_optionState = new Map();

function getSelectState(el) {
  const nid = (el && typeof el._nid === "number") ? el._nid : 0;
  let st = globalThis.__capsurf_selectState.get(nid);
  if (!st) {
    st = { selectedIndex: undefined };
    if (nid > 0) globalThis.__capsurf_selectState.set(nid, st);
  }
  return st;
}

function getOptionState(el) {
  const nid = (el && typeof el._nid === "number") ? el._nid : 0;
  let st = globalThis.__capsurf_optionState.get(nid);
  if (!st) {
    st = { selected: undefined, value: undefined };
    if (nid > 0) globalThis.__capsurf_optionState.set(nid, st);
  }
  return st;
}

function isSameElement(a, b) {
  if (a === b) return true;
  if (!a || !b) return false;
  if (typeof a._nid === "number" && typeof b._nid === "number" && a._nid > 0) {
    return a._nid === b._nid;
  }
  return false;
}

function findOwnerSelect(opt) {
  let curr = opt.parentNode;
  while (curr) {
    if ((curr.tagName || "").toLowerCase() === "select") return curr;
    curr = curr.parentNode;
  }
  return null;
}

function getSelectOptions(selectEl) {
  const opts = [];
  const up = typeof upgradeElement === "function" ? upgradeElement : (typeof globalThis !== "undefined" && globalThis.upgradeElement);
  function collect(node) {
    let child = node.firstChild;
    while (child) {
      const tag = (child.tagName || "").toLowerCase();
      if (tag === "option") {
        if (up) up(child);
        opts.push(child);
      } else if (tag === "optgroup") {
        collect(child);
      }
      child = child.nextSibling;
    }
  }
  collect(selectEl);
  return opts;
}

function computeSelectedIndex(selectEl) {
  const selState = getSelectState(selectEl);
  if (selState.selectedIndex !== undefined) {
    return selState.selectedIndex;
  }
  const opts = selectEl.options;
  if (opts.length === 0) return -1;

  if (selectEl.multiple) {
    for (let i = 0; i < opts.length; i++) {
      const ost = getOptionState(opts[i]);
      if (ost.selected === true || (ost.selected === undefined && opts[i].hasAttribute && opts[i].hasAttribute("selected"))) {
        return i;
      }
    }
    return -1;
  }

  // Single select:
  // 1. Check if any option has _selected explicitly set to true
  for (let i = opts.length - 1; i >= 0; i--) {
    const ost = getOptionState(opts[i]);
    if (ost.selected === true) {
      return i;
    }
  }

  // 2. Check if any option has selected attribute (last one wins per WHATWG spec if multiple)
  for (let i = opts.length - 1; i >= 0; i--) {
    const ost = getOptionState(opts[i]);
    if (ost.selected === undefined && opts[i].hasAttribute && opts[i].hasAttribute("selected")) {
      return i;
    }
  }

  // 3. Default selection: first enabled option
  for (let i = 0; i < opts.length; i++) {
    if (!opts[i].disabled) return i;
  }
  // Fallback to first option if all are disabled
  return 0;
}

function installSelectElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};

  // 1. HTMLOptionElement
  function HTMLOptionElement() {
    this._selected = undefined;
  }
  HTMLOptionElement.prototype = Object.create(ElementProto);
  HTMLOptionElement.prototype.constructor = HTMLOptionElement;

  Object.defineProperty(HTMLOptionElement.prototype, "value", {
    get: function () {
      const ost = getOptionState(this);
      if (ost.value !== undefined) return ost.value;
      if (this.hasAttribute && this.hasAttribute("value")) return this.getAttribute("value");
      return this.textContent || "";
    },
    set: function (v) {
      getOptionState(this).value = String(v);
      if (this.setAttribute) this.setAttribute("value", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "text", {
    get: function () { return this.textContent || ""; },
    set: function (v) { this.textContent = String(v); },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "selected", {
    get: function () {
      const ost = getOptionState(this);
      const owner = findOwnerSelect(this);
      if (!owner) {
        if (ost.selected !== undefined) return ost.selected;
        return this.hasAttribute && this.hasAttribute("selected");
      }
      if (owner.multiple) {
        if (ost.selected !== undefined) return ost.selected;
        return this.hasAttribute && this.hasAttribute("selected");
      }
      // Single select:
      const selIdx = computeSelectedIndex(owner);
      if (selIdx === -1) return false;
      return isSameElement(owner.options[selIdx], this);
    },
    set: function (v) {
      const ost = getOptionState(this);
      ost.selected = Boolean(v);
      const owner = findOwnerSelect(this);
      if (owner && !owner.multiple) {
        const selState = getSelectState(owner);
        const opts = owner.options;
        if (v) {
          for (let i = 0; i < opts.length; i++) {
            if (isSameElement(opts[i], this)) {
              selState.selectedIndex = i;
            } else {
              getOptionState(opts[i]).selected = false;
            }
          }
        } else {
          const curIdx = owner.selectedIndex;
          if (curIdx !== -1 && isSameElement(opts[curIdx], this)) {
            selState.selectedIndex = -1;
          }
        }
      }
    },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "defaultSelected", {
    get: function () { return this.hasAttribute && this.hasAttribute("selected"); },
    set: function (v) {
      if (v) {
        this.setAttribute("selected", "");
      } else {
        this.removeAttribute("selected");
      }
    },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "disabled", {
    get: function () { return this.hasAttribute && this.hasAttribute("disabled"); },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  const origOptSetAttr = HTMLOptionElement.prototype.setAttribute;
  HTMLOptionElement.prototype.setAttribute = function (name, value) {
    if (origOptSetAttr) origOptSetAttr.call(this, name, value);
    if (String(name).toLowerCase() === "selected") {
      getOptionState(this).selected = true;
      const owner = findOwnerSelect(this);
      if (owner && !owner.multiple) {
        getSelectState(owner).selectedIndex = undefined;
        const opts = owner.options;
        for (let i = 0; i < opts.length; i++) {
          if (!isSameElement(opts[i], this)) {
            getOptionState(opts[i]).selected = false;
          }
        }
      }
    }
  };

  const origOptRemoveAttr = HTMLOptionElement.prototype.removeAttribute;
  HTMLOptionElement.prototype.removeAttribute = function (name) {
    if (origOptRemoveAttr) origOptRemoveAttr.call(this, name);
    if (String(name).toLowerCase() === "selected") {
      getOptionState(this).selected = false;
      const owner = findOwnerSelect(this);
      if (owner && !owner.multiple) {
        getSelectState(owner).selectedIndex = undefined;
      }
    }
  };

  markNative(HTMLOptionElement, "HTMLOptionElement");
  global.HTMLOptionElement = HTMLOptionElement;
  global.Option = function (text, value, defaultSelected, selected) {
    const opt = typeof document !== "undefined" && document.createElement ? document.createElement("option") : new HTMLOptionElement();
    if (text !== undefined) opt.text = text;
    if (value !== undefined) opt.value = value;
    if (defaultSelected !== undefined) opt.defaultSelected = Boolean(defaultSelected);
    if (selected !== undefined) opt.selected = Boolean(selected);
    return opt;
  };

  // 2. HTMLSelectElement
  function HTMLSelectElement() {
    this._customValidityMessage = "";
  }
  HTMLSelectElement.prototype = Object.create(ElementProto);
  HTMLSelectElement.prototype.constructor = HTMLSelectElement;

  Object.defineProperty(HTMLSelectElement.prototype, "type", {
    get: function () { return this.multiple ? "select-multiple" : "select-one"; },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "options", {
    get: function () {
      return new HTMLCollection(getSelectOptions(this));
    },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "length", {
    get: function () { return this.options.length; },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "selectedIndex", {
    get: function () {
      return computeSelectedIndex(this);
    },
    set: function (idx) {
      const i = Number(idx);
      getSelectState(this).selectedIndex = i;
      const opts = this.options;
      for (let j = 0; j < opts.length; j++) {
        getOptionState(opts[j]).selected = (j === i);
      }
    },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "value", {
    get: function () {
      const idx = this.selectedIndex;
      if (idx >= 0 && idx < this.options.length) {
        return this.options[idx].value;
      }
      return "";
    },
    set: function (v) {
      const str = String(v);
      const opts = this.options;
      for (let i = 0; i < opts.length; i++) {
        if (opts[i].value === str) {
          this.selectedIndex = i;
          return;
        }
      }
      this.selectedIndex = -1;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "multiple", {
    get: function () { return this.hasAttribute("multiple"); },
    set: function (v) { if (v) this.setAttribute("multiple", ""); else this.removeAttribute("multiple"); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "name", {
    get: function () { return this.getAttribute("name") || ""; },
    set: function (v) { this.setAttribute("name", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "disabled", {
    get: function () { return this.hasAttribute("disabled"); },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "required", {
    get: function () { return this.hasAttribute("required"); },
    set: function (v) { if (v) this.setAttribute("required", ""); else this.removeAttribute("required"); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "form", {
    get: function () { return findAssociatedForm(this); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "willValidate", {
    get: function () { return !this.disabled; },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "validity", {
    get: function () { return new ValidityState(this); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "validationMessage", {
    get: function () {
      if (!this.willValidate || this.validity.valid) return "";
      if (this._customValidityMessage) return this._customValidityMessage;
      if (this.validity.valueMissing) return "Please select an item in the list.";
      return "Constraints not satisfied.";
    },
    configurable: true,
  });

  HTMLSelectElement.prototype.setCustomValidity = function (msg) {
    this._customValidityMessage = String(msg || "");
  };

  HTMLSelectElement.prototype.checkValidity = function () {
    if (!this.willValidate) return true;
    if (!this.validity.valid) {
      const evt = new Event("invalid", { bubbles: false, cancelable: true });
      this.dispatchEvent(evt);
      return false;
    }
    return true;
  };

  HTMLSelectElement.prototype.reportValidity = function () {
    return this.checkValidity();
  };

  HTMLSelectElement.prototype.add = function (element, before) {
    getSelectState(this).selectedIndex = undefined;
    if (this.appendChild) {
      this.appendChild(element);
    }
  };

  HTMLSelectElement.prototype.remove = function (index) {
    getSelectState(this).selectedIndex = undefined;
    const opts = this.options;
    const i = Number(index);
    if (i >= 0 && i < opts.length) {
      const node = opts[i];
      if (node.parentNode) {
        node.parentNode.removeChild(node);
      }
    }
  };

  markNative(HTMLSelectElement, "HTMLSelectElement");
  markNative(HTMLSelectElement.prototype.setCustomValidity, "setCustomValidity");
  markNative(HTMLSelectElement.prototype.checkValidity, "checkValidity");
  markNative(HTMLSelectElement.prototype.add, "add");
  markNative(HTMLSelectElement.prototype.remove, "remove");

  global.HTMLSelectElement = HTMLSelectElement;
}
