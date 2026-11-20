// src/js/shim/element/forms/select.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/select.ts

if (!globalThis.__capsurf_selectState) globalThis.__capsurf_selectState = new Map();
if (!globalThis.__capsurf_optionState) globalThis.__capsurf_optionState = new Map();

function getSelectState(el) {
  if (!el || typeof el !== "object") return { selectedIndex: undefined };
  if (el._selState) return el._selState;
  const nid = typeof el._nid === "number" ? el._nid : 0;
  let st = null;
  if (nid > 0) {
    st = globalThis.__capsurf_selectState.get(nid);
  }
  if (!st) {
    st = { selectedIndex: undefined };
    if (nid > 0) globalThis.__capsurf_selectState.set(nid, st);
  }
  el._selState = st;
  return st;
}

function getOptionState(el) {
  if (!el || typeof el !== "object") return { selected: undefined, value: undefined };
  if (el._optState) return el._optState;
  const nid = typeof el._nid === "number" ? el._nid : 0;
  let st = null;
  if (nid > 0) {
    st = globalThis.__capsurf_optionState.get(nid);
  }
  if (!st) {
    st = { selected: undefined, value: undefined };
    if (nid > 0) globalThis.__capsurf_optionState.set(nid, st);
  }
  el._optState = st;
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
    if ((curr.tagName || "").toLowerCase() === "select") {
      if (typeof upgradeElement === "function") upgradeElement(curr);
      else if (typeof globalThis !== "undefined" && typeof globalThis.upgradeElement === "function") globalThis.upgradeElement(curr);
      return curr;
    }
    curr = curr.parentNode;
  }
  return null;
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
  const HTMLElementProto = (global.HTMLElement && global.HTMLElement.prototype) || ElementProto;

  // 1. HTMLOptionElement
  function HTMLOptionElement() {
    this._selected = undefined;
  }
  HTMLOptionElement.prototype = Object.create(HTMLElementProto);
  HTMLOptionElement.prototype.constructor = HTMLOptionElement;
  Object.defineProperty(HTMLOptionElement.prototype, Symbol.toStringTag, {
    value: "HTMLOptionElement",
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "value", {
    get: function () {
      const ost = getOptionState(this);
      if (ost.value !== undefined) return ost.value;
      if (this.hasAttribute && this.hasAttribute("value")) return this.getAttribute("value") || "";
      return this.text;
    },
    set: function (v) {
      getOptionState(this).value = String(v);
      if (this.setAttribute) this.setAttribute("value", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "text", {
    get: function () {
      const tc = (this.textContent || "").replace(/[\t\n\f\r ]+/g, " ").trim();
      return tc;
    },
    set: function (v) {
      this.textContent = String(v);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "label", {
    get: function () {
      if (this.hasAttribute && this.hasAttribute("label")) return this.getAttribute("label") || "";
      return this.text;
    },
    set: function (v) {
      if (this.setAttribute) this.setAttribute("label", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "selected", {
    get: function () {
      const ost = getOptionState(this);
      const owner = findOwnerSelect(this);
      if (!owner) {
        if (ost.selected !== undefined) return ost.selected;
        return Boolean(this.hasAttribute && this.hasAttribute("selected"));
      }
      if (owner.multiple) {
        if (ost.selected !== undefined) return ost.selected;
        return Boolean(this.hasAttribute && this.hasAttribute("selected"));
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
    get: function () { return Boolean(this.hasAttribute && this.hasAttribute("selected")); },
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
    get: function () {
      if (this.hasAttribute && this.hasAttribute("disabled")) return true;
      let p = this.parentNode;
      while (p) {
        const tag = String(p.tagName || "").toLowerCase();
        if (tag === "optgroup") {
          return p.disabled || Boolean(p.hasAttribute && p.hasAttribute("disabled"));
        }
        if (tag === "select") break;
        p = p.parentNode;
      }
      return false;
    },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "index", {
    get: function () {
      const owner = findOwnerSelect(this);
      if (!owner) return 0;
      const opts = owner.options;
      for (let i = 0; i < opts.length; i++) {
        if (isSameElement(opts[i], this)) return i;
      }
      return 0;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLOptionElement.prototype, "form", {
    get: function () {
      const owner = findOwnerSelect(this);
      return owner ? owner.form : (typeof findAssociatedForm === "function" ? findAssociatedForm(this) : null);
    },
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
  if (typeof globalThis !== "undefined") {
    globalThis.HTMLOptionElement = HTMLOptionElement;
  }
  if (typeof window !== "undefined") {
    window.HTMLOptionElement = HTMLOptionElement;
  }

  global.Option = function (text, value, defaultSelected, selected) {
    const opt = typeof document !== "undefined" && document.createElement ? document.createElement("option") : new HTMLOptionElement();
    if (text !== undefined) opt.text = text;
    if (value !== undefined) opt.value = value;
    if (defaultSelected !== undefined) opt.defaultSelected = Boolean(defaultSelected);
    if (selected !== undefined) opt.selected = Boolean(selected);
    return opt;
  };
  if (typeof globalThis !== "undefined") {
    globalThis.Option = global.Option;
  }
  if (typeof window !== "undefined") {
    window.Option = global.Option;
  }

  // 2. HTMLOptGroupElement
  function HTMLOptGroupElement() {}
  HTMLOptGroupElement.prototype = Object.create(HTMLElementProto);
  HTMLOptGroupElement.prototype.constructor = HTMLOptGroupElement;
  Object.defineProperty(HTMLOptGroupElement.prototype, Symbol.toStringTag, {
    value: "HTMLOptGroupElement",
    configurable: true,
  });

  Object.defineProperty(HTMLOptGroupElement.prototype, "disabled", {
    get: function () { return Boolean(this.hasAttribute && this.hasAttribute("disabled")); },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  Object.defineProperty(HTMLOptGroupElement.prototype, "label", {
    get: function () { return (this.getAttribute && this.getAttribute("label")) || ""; },
    set: function (v) { if (this.setAttribute) this.setAttribute("label", String(v)); },
    configurable: true,
  });

  markNative(HTMLOptGroupElement, "HTMLOptGroupElement");
  global.HTMLOptGroupElement = HTMLOptGroupElement;
  if (typeof globalThis !== "undefined") {
    globalThis.HTMLOptGroupElement = HTMLOptGroupElement;
  }
  if (typeof window !== "undefined") {
    window.HTMLOptGroupElement = HTMLOptGroupElement;
  }

  // 3. HTMLSelectElement
  function HTMLSelectElement() {
    this._customValidityMessage = "";
  }
  HTMLSelectElement.prototype = Object.create(HTMLElementProto);
  HTMLSelectElement.prototype.constructor = HTMLSelectElement;
  Object.defineProperty(HTMLSelectElement.prototype, Symbol.toStringTag, {
    value: "HTMLSelectElement",
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "type", {
    get: function () { return this.multiple ? "select-multiple" : "select-one"; },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "options", {
    get: function () {
      return getSelectOptionsCollection(this);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "length", {
    get: function () { return this.options.length; },
    set: function (v) { this.options.length = v; },
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

  Object.defineProperty(HTMLSelectElement.prototype, "selectedOptions", {
    get: function () {
      const self = this;
      return makeHtmlCollection(function () {
        const list = [];
        const opts = self.options;
        for (let i = 0; i < opts.length; i++) {
          if (opts[i].selected) {
            list.push(opts[i]);
          }
        }
        return list;
      });
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
    get: function () { return Boolean(this.hasAttribute && this.hasAttribute("multiple")); },
    set: function (v) { if (v) this.setAttribute("multiple", ""); else this.removeAttribute("multiple"); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "name", {
    get: function () { return (this.getAttribute && this.getAttribute("name")) || ""; },
    set: function (v) { if (this.setAttribute) this.setAttribute("name", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "disabled", {
    get: function () { return Boolean(this.hasAttribute && this.hasAttribute("disabled")); },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "required", {
    get: function () { return Boolean(this.hasAttribute && this.hasAttribute("required")); },
    set: function (v) { if (v) this.setAttribute("required", ""); else this.removeAttribute("required"); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "form", {
    get: function () { return typeof findAssociatedForm === "function" ? findAssociatedForm(this) : null; },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "willValidate", {
    get: function () { return computeWillValidate(this); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "validity", {
    get: function () { return makeValidityState(this); },
    configurable: true,
  });

  Object.defineProperty(HTMLSelectElement.prototype, "validationMessage", {
    get: function () {
      return computeValidationMessage(this);
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
    this.options.add(element, before);
  };

  HTMLSelectElement.prototype.remove = function (index) {
    getSelectState(this).selectedIndex = undefined;
    if (index === undefined) {
      const p = this.parentElement || this.parentNode;
      if (p && p.removeChild) p.removeChild(this);
      return;
    }
    this.options.remove(index);
  };

  HTMLSelectElement.prototype.item = function (index) {
    return this.options.item(index);
  };

  HTMLSelectElement.prototype.namedItem = function (name) {
    return this.options.namedItem(name);
  };

  // Expose the displayed single-select label as a trailing Text node on childNodes
  // (used by forms-select-cdp-demo to inspect the rendered select label).
  let baseChildNodesDesc = null;
  let curProto = HTMLElementProto;
  while (curProto && !baseChildNodesDesc) {
    baseChildNodesDesc = Object.getOwnPropertyDescriptor(curProto, "childNodes");
    curProto = Object.getPrototypeOf(curProto);
  }
  if (!baseChildNodesDesc && typeof global.Node === "function" && global.Node.prototype) {
    baseChildNodesDesc = Object.getOwnPropertyDescriptor(global.Node.prototype, "childNodes");
  }
  Object.defineProperty(HTMLSelectElement.prototype, "childNodes", {
    get: function () {
      const base = baseChildNodesDesc && typeof baseChildNodesDesc.get === "function"
        ? baseChildNodesDesc.get.call(this)
        : null;
      const arr = [];
      if (base && typeof base.length === "number") {
        for (let i = 0; i < base.length; i++) arr.push(base[i]);
      }
      const opts = this.options;
      if (!this.multiple && opts && opts.length > 0) {
        const idx = this.selectedIndex;
        const opt = (idx >= 0 && idx < opts.length) ? opts[idx] : opts[0];
        const label = opt ? String(opt.text !== undefined ? opt.text : (opt.textContent || "")) : "";
        if (label) {
          let textNode = null;
          if (typeof global.document === "object" && global.document && typeof global.document.createTextNode === "function") {
            try { textNode = global.document.createTextNode(label); } catch (e) {}
          }
          if (!textNode) {
            textNode = { nodeType: 3, nodeName: "#text", nodeValue: label, data: label, textContent: label };
          }
          arr.push(textNode);
        }
      }
      if (typeof global.NodeList === "function") {
        try { return new global.NodeList(arr); } catch (e) {}
      }
      return arr;
    },
    configurable: true,
  });

  markNative(HTMLSelectElement, "HTMLSelectElement");
  markNative(HTMLSelectElement.prototype.setCustomValidity, "setCustomValidity");
  markNative(HTMLSelectElement.prototype.checkValidity, "checkValidity");
  markNative(HTMLSelectElement.prototype.add, "add");
  markNative(HTMLSelectElement.prototype.remove, "remove");
  markNative(HTMLSelectElement.prototype.item, "item");
  markNative(HTMLSelectElement.prototype.namedItem, "namedItem");

  global.HTMLSelectElement = HTMLSelectElement;
  if (typeof globalThis !== "undefined") {
    globalThis.HTMLSelectElement = HTMLSelectElement;
  }
  if (typeof window !== "undefined") {
    window.HTMLSelectElement = HTMLSelectElement;
  }
}
