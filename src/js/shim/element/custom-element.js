// src/js/shim/element/custom-element.js
// Custom Elements Registry & Lifecycle Management

const customElementRegistry = new Map(); // name -> { constructor, extendsTag, observedAttrs, callback, formAssociated, disabledFeatures }
const customElementByCtor = new WeakMap(); // constructor -> { name, extendsTag }
const customElementObservedAttrs = new WeakMap();
const customElementCallbacks = new WeakMap();
const upgradingElements = new WeakSet();
const queuedAttributeChanges = new WeakMap();
const whenDefinedResolvers = new Map();
const whenDefinedPromises = new Map();
const upgradedCustomNodeIds = new Map();
const elementInternalsMap = new WeakMap();

global.__capsurf_upgradedCustomNodeIds = upgradedCustomNodeIds;
if (typeof globalThis !== "undefined") {
  globalThis.__capsurf_upgradedCustomNodeIds = upgradedCustomNodeIds;
}

const RESERVED_CUSTOM_ELEMENT_NAMES = new Set([
  "annotation-xml",
  "color-profile",
  "font-face",
  "font-face-src",
  "font-face-uri",
  "font-face-format",
  "font-face-name",
  "missing-glyph",
]);

const KNOWN_HTML_TAGS = new Set([
  "a", "abbr", "address", "area", "article", "aside", "audio",
  "b", "base", "bdi", "bdo", "blockquote", "body", "br", "button",
  "canvas", "caption", "cite", "code", "col", "colgroup",
  "data", "datalist", "dd", "del", "details", "dfn", "dialog", "div", "dl", "dt",
  "em", "embed",
  "fieldset", "figcaption", "figure", "footer", "form",
  "h1", "h2", "h3", "h4", "h5", "h6", "head", "header", "hgroup", "hr", "html",
  "i", "iframe", "img", "input", "ins",
  "kbd",
  "label", "legend", "li", "link",
  "main", "map", "mark", "menu", "meta", "meter",
  "nav", "noscript",
  "object", "ol", "optgroup", "option", "output",
  "p", "param", "picture", "pre", "progress",
  "q",
  "rp", "rt", "ruby",
  "s", "samp", "script", "search", "section", "select", "slot", "small", "source", "span", "strong", "style", "sub", "summary", "sup",
  "table", "tbody", "td", "template", "textarea", "tfoot", "th", "thead", "time", "title", "tr", "track",
  "u", "ul",
  "var", "video",
  "wbr",
]);

function isPCENChar(cp) {
  return (
    cp === 0x2d || // '-'
    cp === 0x2e || // '.'
    (cp >= 0x30 && cp <= 0x39) || // '0'-'9'
    cp === 0x5f || // '_'
    (cp >= 0x61 && cp <= 0x7a) || // 'a'-'z'
    cp === 0xb7 ||
    (cp >= 0xc0 && cp <= 0xd6) ||
    (cp >= 0xd8 && cp <= 0xf6) ||
    (cp >= 0xf8 && cp <= 0x37d) ||
    (cp >= 0x37f && cp <= 0x1fff) ||
    (cp >= 0x200c && cp <= 0x200d) ||
    (cp >= 0x203f && cp <= 0x2040) ||
    (cp >= 0x2070 && cp <= 0x218f) ||
    (cp >= 0x2c00 && cp <= 0x2fef) ||
    (cp >= 0x3001 && cp <= 0xd7ff) ||
    (cp >= 0xf900 && cp <= 0xfdcf) ||
    (cp >= 0xfdf0 && cp <= 0xfffd) ||
    (cp >= 0x10000 && cp <= 0xeffff)
  );
}

function isValidCustomElementName(name) {
  if (typeof name !== "string" || name.length === 0) return false;
  const first = name.charCodeAt(0);
  if (first < 0x61 || first > 0x7a) return false; // must start with ASCII lowercase [a-z]
  if (name.indexOf("-") === -1) return false;
  if (RESERVED_CUSTOM_ELEMENT_NAMES.has(name)) return false;
  for (let i = 1; i < name.length; ) {
    const cp = name.codePointAt(i);
    if (!isPCENChar(cp)) return false;
    i += cp > 0xffff ? 2 : 1;
  }
  return true;
}

function getCustomElementDefinition(name) {
  return customElementRegistry.get(name) || null;
}

function getCustomIsValue(el) {
  if (el._isValue !== undefined) return el._isValue;
  return el.getAttribute ? el.getAttribute("is") : null;
}

function isElementDefined(el) {
  if (!el || el.nodeType !== 1) return false;
  const ns = el._namespaceURI !== undefined ? el._namespaceURI : el.namespaceURI;
  if (ns && ns !== "http://www.w3.org/1999/xhtml") return true;
  if (el._customElementState === "custom") return true;
  if (el._customElementState === "failed" || el._customElementState === "uncustomized" || el._customElementState === "precustomized") {
    return false;
  }
  const local = String(el.localName || el.tagName || "").toLowerCase();
  const isAttr = getCustomIsValue(el);
  if (isAttr !== null && isAttr !== undefined) {
    const def = customElementRegistry.get(String(isAttr));
    return Boolean(def && def.extendsTag === local && def.constructor.prototype.isPrototypeOf(el));
  }
  if (isValidCustomElementName(local)) {
    const def = customElementRegistry.get(local);
    return Boolean(def && !def.extendsTag && def.constructor.prototype.isPrototypeOf(el));
  }
  return true;
}

if (typeof globalThis !== "undefined") {
  globalThis.__capsurf_isElementDefined = isElementDefined;
  globalThis.__capsurf_isValidCustomElementName = isValidCustomElementName;
}

function upgradeCustomElement(element, customElementConstructor, options) {
  const tagName = (options && options.tagName) ? options.tagName : String(element.localName || element.tagName || "").toLowerCase();
  if (!customElementConstructor || customElementConstructor.prototype.isPrototypeOf(element)) {
    return;
  }

  // 1. Snapshot initial attributes before calling constructor
  const initialAttributes = [];
  if (element.attributes) {
    for (let i = 0; i < element.attributes.length; i++) {
      const a = element.attributes[i];
      if (a) {
        initialAttributes.push({
          name: a.name || a.localName || "",
          value: a.value || "",
          namespace: a.namespaceURI || null,
        });
      }
    }
  }

  // 2. Set prototype to custom element class
  Object.setPrototypeOf(element, customElementConstructor.prototype);
  element._customElementState = "precustomized";
  const elNid = typeof element._nid === "number" ? element._nid : (typeof element._id === "number" ? element._id : -1);
  if (elNid > 0) {
    upgradedCustomNodeIds.set(elNid, customElementConstructor);
  }

  // 3. Instantiate via constructor
  upgradingElements.add(element);
  const constructionStacks = (typeof global !== "undefined" && global._constructionStacks) ? global._constructionStacks : (typeof globalThis !== "undefined" && globalThis._constructionStacks ? globalThis._constructionStacks : null);
  const upgradeTargets = (typeof global !== "undefined" && global._upgradeTargets) ? global._upgradeTargets : (typeof globalThis !== "undefined" && globalThis._upgradeTargets ? globalThis._upgradeTargets : null);
  let stack = null;
  if (constructionStacks) {
    stack = constructionStacks.get(customElementConstructor);
    if (!stack) {
      stack = [];
      constructionStacks.set(customElementConstructor, stack);
    }
    stack.push(element);
  }
  if (typeof HTMLElement !== "undefined") {
    HTMLElement._upgradeTarget = element;
  }

  let constructed = true;
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_throwOnDynamicMarkupInsertionDepth = (globalThis.__capsurf_throwOnDynamicMarkupInsertionDepth || 0) + 1;
  }
  try {
    new customElementConstructor();
  } catch (err) {
    constructed = false;
    element._customElementState = "failed";
    if (typeof reportError === "function") {
      try {
        reportError(err);
      } catch (_) {}
    } else if (typeof window !== "undefined" && typeof window.onerror === "function") {
      try {
        window.onerror(err && err.message ? err.message : String(err), "", 0, 0, err);
      } catch (_) {}
    } else if (typeof console !== "undefined" && console.warn) {
      console.warn("[capsurf] custom-element constructor for " + tagName + " threw:", err);
    }
  } finally {
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_throwOnDynamicMarkupInsertionDepth = Math.max(0, (globalThis.__capsurf_throwOnDynamicMarkupInsertionDepth || 1) - 1);
    }
    if (stack) {
      const sIdx = stack.indexOf(element);
      if (sIdx !== -1) stack.splice(sIdx, 1);
    }
    if (upgradeTargets) {
      const uIdx = upgradeTargets.indexOf(element);
      if (uIdx !== -1) upgradeTargets.splice(uIdx, 1);
    }
    upgradingElements.delete(element);
  }

  if (!constructed) {
    queuedAttributeChanges.delete(element);
    return;
  }

  element._customElementState = "custom";
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_domMutationEpoch = (globalThis.__capsurf_domMutationEpoch || 0) + 1;
  }

  // 4. Fire attributeChangedCallback for observed attributes
  const observed = customElementObservedAttrs.get(customElementConstructor);
  const cb = (typeof element.attributeChangedCallback === "function")
    ? element.attributeChangedCallback
    : customElementCallbacks.get(customElementConstructor);

  if (observed && cb) {
    // 4.1 First, trigger callbacks for initial observed attributes
    for (let i = 0; i < initialAttributes.length; i++) {
      const attr = initialAttributes[i];
      if (observed.has(attr.name)) {
        try {
          cb.call(element, attr.name, null, attr.value, attr.namespace);
        } catch (e) {
          if (typeof console !== "undefined" && console.warn) console.warn(e);
        }
      }
    }

    // 4.2 Next, trigger queued attribute changes that occurred during constructor
    const queued = queuedAttributeChanges.get(element);
    if (queued && queued.length) {
      for (let i = 0; i < queued.length; i++) {
        const args = queued[i];
        try {
          cb.call(element, args[0], args[1], args[2], args[3]);
        } catch (e) {
          if (typeof console !== "undefined" && console.warn) console.warn(e);
        }
      }
    }
  }
  queuedAttributeChanges.delete(element);

  // 5. Fire connectedCallback if requested and connected
  if (options && options.fireConnectedCallback && element.isConnected && typeof element.connectedCallback === "function") {
    try {
      element.connectedCallback();
    } catch (e) {
      if (typeof console !== "undefined" && console.warn) console.warn(e);
    }
  }
}

function notifyCustomElementAttributeChanged(element, name, oldValue, newValue, namespace) {
  if (!element) return;
  const tagName = String(element.localName || element.tagName || "").toLowerCase();
  const isAttr = element.getAttribute ? element.getAttribute("is") : null;
  let def = customElementRegistry.get(tagName);
  if ((!def || def.extendsTag) && isAttr) {
    const extDef = customElementRegistry.get(String(isAttr));
    if (extDef && extDef.extendsTag === tagName) def = extDef;
  }
  const ctor = def ? def.constructor : null;
  if (!ctor || !ctor.prototype.isPrototypeOf(element)) return;

  const observed = customElementObservedAttrs.get(ctor);
  if (!observed || !observed.has(name)) return;

  const cb = (typeof element.attributeChangedCallback === "function")
    ? element.attributeChangedCallback
    : customElementCallbacks.get(ctor);
  if (!cb) return;

  if (upgradingElements.has(element)) {
    let queued = queuedAttributeChanges.get(element);
    if (!queued) {
      queued = [];
      queuedAttributeChanges.set(element, queued);
    }
    queued.push([name, oldValue, newValue, namespace]);
    return;
  }

  try {
    cb.call(element, name, oldValue, newValue, namespace);
  } catch (e) {
    if (typeof console !== "undefined" && console.warn) console.warn(e);
  }
}

// CustomStateSet
function CustomStateSet(element) {
  this._element = element;
  this._set = new Set();
}
Object.defineProperty(CustomStateSet.prototype, "size", {
  get: function () { return this._set.size; },
  configurable: true,
});
CustomStateSet.prototype.has = function (state) { return this._set.has(String(state)); };
CustomStateSet.prototype.add = function (state) { this._set.add(String(state)); return this; };
CustomStateSet.prototype.delete = function (state) { return this._set.delete(String(state)); };
CustomStateSet.prototype.clear = function () { this._set.clear(); };
CustomStateSet.prototype.forEach = function (cb, thisArg) { this._set.forEach(cb, thisArg); };
CustomStateSet.prototype.keys = function () { return this._set.keys(); };
CustomStateSet.prototype.values = function () { return this._set.values(); };
CustomStateSet.prototype.entries = function () { return this._set.entries(); };
CustomStateSet.prototype[Symbol.iterator] = function () { return this._set[Symbol.iterator](); };
Object.defineProperty(CustomStateSet.prototype, Symbol.toStringTag, {
  value: "CustomStateSet",
  configurable: true,
});

// ElementInternals
function ElementInternals(element, formAssociated) {
  this._element = element;
  this._formAssociated = Boolean(formAssociated);
  this._states = new CustomStateSet(element);
  element._customStates = this._states;
  this._role = null;
}

Object.defineProperty(ElementInternals.prototype, "shadowRoot", {
  get: function () {
    const sr = this._element ? this._element._shadowRoot : null;
    if (!sr || sr._preDefinition) return null;
    return sr;
  },
  configurable: true,
});

Object.defineProperty(ElementInternals.prototype, "states", {
  get: function () { return this._states; },
  configurable: true,
});

Object.defineProperty(ElementInternals.prototype, "role", {
  get: function () { return this._role; },
  set: function (v) { this._role = v === null || v === undefined ? null : String(v); },
  configurable: true,
});

const ARIA_PROPS = [
  "ariaAtomic", "ariaAutoComplete", "ariaBrailleLabel", "ariaBrailleRoleDescription",
  "ariaBusy", "ariaChecked", "ariaColCount", "ariaColIndex", "ariaColIndexText", "ariaColSpan",
  "ariaCurrent", "ariaDescription", "ariaDisabled", "ariaExpanded", "ariaHasPopup",
  "ariaHidden", "ariaInvalid", "ariaKeyShortcuts", "ariaLabel", "ariaLevel",
  "ariaLive", "ariaModal", "ariaMultiLine", "ariaMultiSelectable", "ariaOrientation",
  "ariaPlaceholder", "ariaPosInSet", "ariaPressed", "ariaReadOnly", "ariaRelevant",
  "ariaRequired", "ariaRoleDescription", "ariaRowCount", "ariaRowIndex", "ariaRowIndexText",
  "ariaRowSpan", "ariaSelected", "ariaSetSize", "ariaSort", "ariaValueMax",
  "ariaValueMin", "ariaValueNow", "ariaValueText"
];
for (let ai = 0; ai < ARIA_PROPS.length; ai++) {
  (function (prop) {
    const slot = "_" + prop;
    Object.defineProperty(ElementInternals.prototype, prop, {
      get: function () { return this[slot] !== undefined ? this[slot] : null; },
      set: function (v) { this[slot] = (v === null || v === undefined) ? null : String(v); },
      configurable: true,
    });
  })(ARIA_PROPS[ai]);
}

function assertFormAssociated(internals, methodName) {
  if (!internals._formAssociated) {
    throw new DOMException(
      "Failed to execute '" + methodName + "' on 'ElementInternals': The target element is not a form-associated custom element.",
      "NotSupportedError"
    );
  }
}

ElementInternals.prototype.setFormValue = function (value, state) {
  assertFormAssociated(this, "setFormValue");
  this._formValue = value === undefined ? null : value;
  this._formState = state === undefined ? value : state;
};
ElementInternals.prototype.setValidity = function (flags, message, anchor) {
  assertFormAssociated(this, "setValidity");
  this._validityFlags = flags || {};
  this._validationMessage = message ? String(message) : "";
};
ElementInternals.prototype.checkValidity = function () {
  assertFormAssociated(this, "checkValidity");
  return !this._validationMessage;
};
ElementInternals.prototype.reportValidity = function () {
  assertFormAssociated(this, "reportValidity");
  return !this._validationMessage;
};
Object.defineProperty(ElementInternals.prototype, "form", {
  get: function () {
    assertFormAssociated(this, "form");
    let cur = this._element ? this._element.parentNode : null;
    while (cur) {
      if (cur.nodeType === 1 && String(cur.localName || cur.tagName || "").toLowerCase() === "form") return cur;
      cur = cur.parentNode;
    }
    return null;
  },
  configurable: true,
});
Object.defineProperty(ElementInternals.prototype, "willValidate", {
  get: function () { assertFormAssociated(this, "willValidate"); return true; },
  configurable: true,
});
Object.defineProperty(ElementInternals.prototype, "validity", {
  get: function () { assertFormAssociated(this, "validity"); return this._validityFlags || { valid: true }; },
  configurable: true,
});
Object.defineProperty(ElementInternals.prototype, "validationMessage", {
  get: function () { assertFormAssociated(this, "validationMessage"); return this._validationMessage || ""; },
  configurable: true,
});
Object.defineProperty(ElementInternals.prototype, "labels", {
  get: function () { assertFormAssociated(this, "labels"); return []; },
  configurable: true,
});
Object.defineProperty(ElementInternals.prototype, Symbol.toStringTag, {
  value: "ElementInternals",
  configurable: true,
});

function CustomElementRegistry() {}

CustomElementRegistry.prototype.define = function (name, constructor, options) {
  if (arguments.length < 2) {
    throw new TypeError("Failed to execute 'define' on 'CustomElementRegistry': 2 arguments required.");
  }
  const tagName = String(name);
  if (!isValidCustomElementName(tagName)) {
    throw new DOMException("Failed to execute 'define' on 'CustomElementRegistry': '" + tagName + "' is not a valid custom element name.", "SyntaxError");
  }
  if (typeof constructor !== "function") {
    throw new TypeError("Failed to execute 'define' on 'CustomElementRegistry': parameter 2 is not of type 'Function'.");
  }
  if (customElementRegistry.has(tagName)) {
    throw new DOMException("Failed to execute 'define' on 'CustomElementRegistry': the name \"" + tagName + "\" has already been used with this registry", "NotSupportedError");
  }
  if (customElementByCtor.has(constructor)) {
    throw new DOMException("Failed to execute 'define' on 'CustomElementRegistry': this constructor has already been used with this registry", "NotSupportedError");
  }

  let extendsTag = null;
  if (options !== undefined && options !== null) {
    if (typeof options !== "object") {
      throw new TypeError("Failed to execute 'define' on 'CustomElementRegistry': parameter 3 ('options') is not an object.");
    }
    if (options.extends !== undefined && options.extends !== null) {
      const ext = String(options.extends);
      if (ext.indexOf("-") !== -1 || !KNOWN_HTML_TAGS.has(ext.toLowerCase())) {
        throw new DOMException("Failed to execute 'define' on 'CustomElementRegistry': '" + ext + "' is not a valid HTML element name for 'extends'.", "NotSupportedError");
      }
      extendsTag = ext.toLowerCase();
    }
  }

  const disabledFeatures = new Set();
  if (Array.isArray(constructor.disabledFeatures)) {
    for (let i = 0; i < constructor.disabledFeatures.length; i++) {
      disabledFeatures.add(String(constructor.disabledFeatures[i]));
    }
  }
  const formAssociated = Boolean(constructor.formAssociated);

  // Extract observedAttributes
  const observedSet = new Set();
  if (!disabledFeatures.has("attributes")) {
    const observedArr = constructor.observedAttributes;
    if (observedArr && (Array.isArray(observedArr) || typeof observedArr[Symbol.iterator] === "function")) {
      for (const attr of observedArr) {
        observedSet.add(String(attr));
      }
    }
  }
  customElementObservedAttrs.set(constructor, observedSet);

  const proto = constructor.prototype;
  if (!proto || typeof proto !== "object") {
    throw new TypeError("Failed to execute 'define' on 'CustomElementRegistry': constructor prototype is not an object.");
  }
  if (typeof proto.attributeChangedCallback === "function") {
    customElementCallbacks.set(constructor, proto.attributeChangedCallback);
  }

  const def = {
    name: tagName,
    constructor: constructor,
    extendsTag: extendsTag,
    formAssociated: formAssociated,
    disabledFeatures: disabledFeatures,
  };
  customElementRegistry.set(tagName, def);
  customElementByCtor.set(constructor, def);

  // Upgrade existing matching elements in the document
  const doc = (typeof document !== "undefined") ? document : (global.document || null);
  if (doc && doc.querySelectorAll) {
    try {
      const sel = extendsTag ? (extendsTag + '[is="' + tagName + '"]') : tagName;
      const existing = doc.querySelectorAll(sel);
      for (let i = 0; i < existing.length; i++) {
        upgradeCustomElement(existing[i], constructor, {
          tagName: tagName,
          fireConnectedCallback: true,
        });
      }
    } catch (e) {
      if (typeof console !== "undefined" && console.warn) console.warn(e);
    }
  }

  // Resolve whenDefined promises
  const resolvers = whenDefinedResolvers.get(tagName);
  if (resolvers && resolvers.length) {
    for (let i = 0; i < resolvers.length; i++) {
      try { resolvers[i](constructor); } catch (e) {}
    }
    whenDefinedResolvers.delete(tagName);
  }
  whenDefinedPromises.delete(tagName);
};

CustomElementRegistry.prototype.get = function (name) {
  const def = customElementRegistry.get(String(name));
  return def ? def.constructor : undefined;
};

CustomElementRegistry.prototype.getName = function (constructor) {
  const def = customElementByCtor.get(constructor);
  return def ? def.name : null;
};

CustomElementRegistry.prototype.whenDefined = function (name) {
  const tagName = String(name);
  if (!isValidCustomElementName(tagName)) {
    return Promise.reject(new DOMException("Failed to execute 'whenDefined' on 'CustomElementRegistry': '" + tagName + "' is not a valid custom element name.", "SyntaxError"));
  }
  if (customElementRegistry.has(tagName)) {
    return Promise.resolve(customElementRegistry.get(tagName).constructor);
  }
  if (whenDefinedPromises.has(tagName)) {
    return whenDefinedPromises.get(tagName);
  }
  const p = new Promise(function (resolve) {
    let list = whenDefinedResolvers.get(tagName);
    if (!list) {
      list = [];
      whenDefinedResolvers.set(tagName, list);
    }
    list.push(resolve);
  });
  whenDefinedPromises.set(tagName, p);
  return p;
};

CustomElementRegistry.prototype.upgrade = function (root) {
  if (!root || typeof root !== "object") return;
  function visit(node) {
    if (!node || typeof node !== "object") return;
    if (node.nodeType === 1) {
      const ns = node._namespaceURI !== undefined ? node._namespaceURI : node.namespaceURI;
      if (!ns || ns === "http://www.w3.org/1999/xhtml") {
        const local = String(node.localName || node.tagName || "").toLowerCase();
        const isAttr = getCustomIsValue(node);
        let def = customElementRegistry.get(local);
        if (def && def.extendsTag) def = null;
        if (!def && isAttr) {
          const extDef = customElementRegistry.get(String(isAttr));
          if (extDef && extDef.extendsTag === local) def = extDef;
        }
        if (def) {
          upgradeCustomElement(node, def.constructor, {
            tagName: def.name,
            fireConnectedCallback: Boolean(node.isConnected),
          });
        }
      }
    }
    const kids = node.childNodes || node._children;
    if (kids && kids.length) {
      for (let i = 0; i < kids.length; i++) {
        visit(kids[i]);
      }
    }
  }
  visit(root);
};

markNative(CustomElementRegistry.prototype.define, "define");
markNative(CustomElementRegistry.prototype.get, "get");
markNative(CustomElementRegistry.prototype.getName, "getName");
markNative(CustomElementRegistry.prototype.whenDefined, "whenDefined");
markNative(CustomElementRegistry.prototype.upgrade, "upgrade");
markNative(CustomElementRegistry, "CustomElementRegistry");

const attachInternalsFn = markNative(function () {
    if (!this || this.nodeType !== 1) {
      throw new TypeError("Illegal invocation");
    }
    const def = customElementByCtor.get(this.constructor);
    if (!def) {
      throw new DOMException("Failed to execute 'attachInternals' on 'HTMLElement': The target element is not a custom element.", "NotSupportedError");
    }
    if (def.extendsTag) {
      throw new DOMException("Failed to execute 'attachInternals' on 'HTMLElement': Cannot attach internals to customized built-in elements.", "NotSupportedError");
    }
    if (def.disabledFeatures && def.disabledFeatures.has("internals")) {
      throw new DOMException("Failed to execute 'attachInternals' on 'HTMLElement': ElementInternals are disabled for this custom element.", "NotSupportedError");
    }
    if (elementInternalsMap.has(this)) {
      throw new DOMException("Failed to execute 'attachInternals' on 'HTMLElement': ElementInternals for the specified element was already attached.", "NotSupportedError");
    }
    const internals = new ElementInternals(this, def.formAssociated);
    elementInternalsMap.set(this, internals);
    return internals;
  }, "attachInternals");
global.__capsurf_attachInternalsFn = attachInternalsFn;
if (typeof globalThis !== "undefined") globalThis.__capsurf_attachInternalsFn = attachInternalsFn;
if (typeof HTMLElement !== "undefined" && HTMLElement.prototype && !HTMLElement.prototype.attachInternals) {
  HTMLElement.prototype.attachInternals = attachInternalsFn;
}

const customElementsInstance = new CustomElementRegistry();
global.CustomElementRegistry = CustomElementRegistry;
let currentCustomElements = customElementsInstance;

function defineCustomElementsProp(obj) {
  if (!obj) return;
  try {
    Object.defineProperty(obj, "CustomElementRegistry", {
      value: CustomElementRegistry,
      writable: true,
      configurable: true,
      enumerable: true,
    });
    Object.defineProperty(obj, "ElementInternals", {
      value: ElementInternals,
      writable: true,
      configurable: true,
      enumerable: true,
    });
    Object.defineProperty(obj, "CustomStateSet", {
      value: CustomStateSet,
      writable: true,
      configurable: true,
      enumerable: true,
    });
    Object.defineProperty(obj, "customElements", {
      get: function () { return currentCustomElements; },
      set: function (val) {
        if (val && val instanceof CustomElementRegistry) {
          currentCustomElements = val;
        }
      },
      configurable: true,
      enumerable: true,
    });
  } catch (e) {}
}

defineCustomElementsProp(global);
if (typeof globalThis !== "undefined") {
  defineCustomElementsProp(globalThis);
  globalThis.notifyCustomElementAttributeChanged = notifyCustomElementAttributeChanged;
  globalThis.upgradeCustomElement = upgradeCustomElement;
  globalThis.__capsurf_getCustomElementDefinition = getCustomElementDefinition;
  globalThis.__capsurf_getCustomElementByCtor = function (ctor) {
    return customElementByCtor.get(ctor) || null;
  };
}
if (typeof window !== "undefined") {
  defineCustomElementsProp(window);
}
