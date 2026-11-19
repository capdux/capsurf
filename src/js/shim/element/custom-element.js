// src/js/shim/element/custom-element.js
// Custom Elements Registry & Lifecycle Management

const customElementRegistry = new Map();
const customElementObservedAttrs = new WeakMap();
const customElementCallbacks = new WeakMap();
const upgradingElements = new WeakSet();
const queuedAttributeChanges = new WeakMap();
const whenDefinedResolvers = new Map();

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
  try {
    new customElementConstructor();
  } catch (err) {
    constructed = false;
    if (typeof console !== "undefined" && console.warn) {
      console.warn("[capsurf] custom-element constructor for " + tagName + " threw:", err);
    }
  } finally {
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
  if (options && options.fireConnectedCallback && typeof element.connectedCallback === "function") {
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
  const ctor = customElementRegistry.get(tagName);
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

function CustomElementRegistry() {}

CustomElementRegistry.prototype.define = function (name, constructor, options) {
  const tagName = String(name).toLowerCase();
  if (customElementRegistry.has(tagName)) {
    throw new DOMException("Custom element '" + tagName + "' has already been defined.", "NotSupportedError");
  }
  if (typeof constructor !== "function") {
    throw new TypeError("Constructor must be a constructor function");
  }

  // Extract observedAttributes
  const observedSet = new Set();
  const observedArr = constructor.observedAttributes;
  if (observedArr && (Array.isArray(observedArr) || typeof observedArr[Symbol.iterator] === "function")) {
    for (const attr of observedArr) {
      observedSet.add(String(attr));
    }
  }
  customElementObservedAttrs.set(constructor, observedSet);

  if (typeof constructor.prototype.attributeChangedCallback === "function") {
    customElementCallbacks.set(constructor, constructor.prototype.attributeChangedCallback);
  }

  customElementRegistry.set(tagName, constructor);

  // Upgrade existing matching elements in the document
  const doc = (typeof document !== "undefined") ? document : (global.document || null);
  if (doc && doc.querySelectorAll) {
    try {
      const existing = doc.querySelectorAll(tagName);
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
};

CustomElementRegistry.prototype.get = function (name) {
  return customElementRegistry.get(String(name).toLowerCase()) || undefined;
};

CustomElementRegistry.prototype.whenDefined = function (name) {
  const tagName = String(name).toLowerCase();
  if (customElementRegistry.has(tagName)) {
    return Promise.resolve(customElementRegistry.get(tagName));
  }
  return new Promise(function (resolve) {
    let list = whenDefinedResolvers.get(tagName);
    if (!list) {
      list = [];
      whenDefinedResolvers.set(tagName, list);
    }
    list.push(resolve);
  });
};

CustomElementRegistry.prototype.upgrade = function (root) {
  if (!root) return;
  for (const [tagName, ctor] of customElementRegistry.entries()) {
    if (root.querySelectorAll) {
      const matches = root.querySelectorAll(tagName);
      for (let i = 0; i < matches.length; i++) {
        upgradeCustomElement(matches[i], ctor, {
          tagName: tagName,
          fireConnectedCallback: Boolean(matches[i].isConnected),
        });
      }
    }
  }
};

markNative(CustomElementRegistry.prototype.define, "define");
markNative(CustomElementRegistry.prototype.get, "get");
markNative(CustomElementRegistry.prototype.whenDefined, "whenDefined");
markNative(CustomElementRegistry.prototype.upgrade, "upgrade");
markNative(CustomElementRegistry, "CustomElementRegistry");

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
}
if (typeof window !== "undefined") {
  defineCustomElementsProp(window);
}
