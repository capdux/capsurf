// src/js/shim/node-base.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/node-base.ts

const parsedNodeRegistry = (global.__parsedNodeRegistry = global.__parsedNodeRegistry || new WeakSet());

function isParsedNode(node) {
  return typeof node === "object" && node !== null && parsedNodeRegistry.has(node);
}

function registerParsedNode(node) {
  if (typeof node === "object" && node !== null) {
    parsedNodeRegistry.add(node);
  }
  return node;
}

function Node() {
  EventTarget.call(this);
  this.parentNode = null;
}
Node.prototype = Object.create(EventTarget.prototype);
Node.prototype.constructor = Node;

// Node Type Constants on constructor and prototype
const nodeTypes = {
  ELEMENT_NODE: ELEMENT_NODE,
  ATTRIBUTE_NODE: ATTRIBUTE_NODE,
  TEXT_NODE: TEXT_NODE,
  CDATA_SECTION_NODE: CDATA_SECTION_NODE,
  ENTITY_REFERENCE_NODE: ENTITY_REFERENCE_NODE,
  ENTITY_NODE: ENTITY_NODE,
  PROCESSING_INSTRUCTION_NODE: PROCESSING_INSTRUCTION_NODE,
  COMMENT_NODE: COMMENT_NODE,
  DOCUMENT_NODE: DOCUMENT_NODE,
  DOCUMENT_TYPE_NODE: DOCUMENT_TYPE_NODE,
  DOCUMENT_FRAGMENT_NODE: DOCUMENT_FRAGMENT_NODE,
  NOTATION_NODE: NOTATION_NODE,
  DOCUMENT_POSITION_DISCONNECTED: DOCUMENT_POSITION_DISCONNECTED,
  DOCUMENT_POSITION_PRECEDING: DOCUMENT_POSITION_PRECEDING,
  DOCUMENT_POSITION_FOLLOWING: DOCUMENT_POSITION_FOLLOWING,
  DOCUMENT_POSITION_CONTAINS: DOCUMENT_POSITION_CONTAINS,
  DOCUMENT_POSITION_CONTAINED_BY: DOCUMENT_POSITION_CONTAINED_BY,
  DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC: DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC,
};

for (const k in nodeTypes) {
  Node[k] = nodeTypes[k];
  Node.prototype[k] = nodeTypes[k];
}

Object.defineProperty(Node.prototype, "nodeValue", {
  get: function () { return null; },
  set: function () {},
  configurable: true,
});

Object.defineProperty(Node.prototype, "isConnected", {
  get: function () {
    let curr = this;
    while (curr) {
      if (curr.nodeType === DOCUMENT_NODE) return true;
      if (curr.nodeType === DOCUMENT_FRAGMENT_NODE && curr.host) {
        curr = curr.host;
        continue;
      }
      curr = curr.parentNode || curr.parentElement;
    }
    return false;
  },
  configurable: true,
});

Object.defineProperty(Node.prototype, "previousSibling", {
  get: function () {
    const p = this.parentNode;
    if (!p) return null;
    const kids = ensureChildren(p);
    if (!kids || !kids.length) return null;
    const nid = typeof this._nid === "number" && this._nid > 0 ? this._nid : 0;
    for (let i = 0; i < kids.length; i++) {
      const k = kids[i];
      if (k === this || (nid > 0 && k && k._nid === nid)) {
        return i > 0 ? kids[i - 1] : null;
      }
    }
    return null;
  },
  configurable: true,
});

Object.defineProperty(Node.prototype, "nextSibling", {
  get: function () {
    const p = this.parentNode;
    if (!p) return null;
    const kids = ensureChildren(p);
    if (!kids || !kids.length) return null;
    const nid = typeof this._nid === "number" && this._nid > 0 ? this._nid : 0;
    for (let i = 0; i < kids.length; i++) {
      const k = kids[i];
      if (k === this || (nid > 0 && k && k._nid === nid)) {
        return i + 1 < kids.length ? kids[i + 1] : null;
      }
    }
    return null;
  },
  configurable: true,
});

Node.prototype.getRootNode = function (options) {
  let n = this;
  let p;
  while ((p = n.parentNode)) n = p;

  if (options && options.composed) {
    while (n && n.nodeType === DOCUMENT_FRAGMENT_NODE && n.host) {
      let hop = n.host;
      let hp;
      while ((hp = hop.parentNode)) hop = hp;
      n = hop;
    }
  }
  return n;
};

Node.prototype.normalize = function () {
  const visit = (parent) => {
    if (!parent) return;
    let children = [];
    if (parent._children && parent._children.length > 0) {
      children = parent._children.slice();
    } else if (parent.childNodes && parent.childNodes.length > 0) {
      children = Array.from(parent.childNodes);
    } else if (parent.nodeType === 9 && parent.documentElement) {
      children = [parent.documentElement];
    } else if (nativeElementFirstChild) {
      let c = null;
      try { c = nativeElementFirstChild.call(parent); } catch (e) {}
      while (c) {
        children.push(c);
        try { c = nativeElementNextSibling.call(c); } catch (e) { c = null; }
      }
    }
    for (let i = 0; i < children.length; i++) {
      const c = children[i];
      if (!c) continue;
      const t = c.nodeType;
      if (t === TEXT_NODE) {
        const cData = c.data !== undefined ? c.data : (c.textContent || "");
        if (cData.length === 0) {
          try {
            parent.removeChild(c);
          } catch (e) {}
          children[i] = null;
          continue;
        }
        let merged = cData;
        let j = i + 1;
        while (j < children.length) {
          const nx = children[j];
          if (!nx) break;
          if (nx.nodeType !== TEXT_NODE) break;
          const nxData = nx.data !== undefined ? nx.data : (nx.textContent || "");
          merged += nxData;
          try {
            parent.removeChild(nx);
          } catch (e) {}
          children[j] = null;
          j++;
        }
        if (c.data !== merged) {
          c.data = merged;
        }
        if (c.textContent !== merged) {
          c.textContent = merged;
        }
        if (c._data !== undefined) {
          c._data = merged;
        }
        i = j - 1;
      } else if (t === ELEMENT_NODE || t === DOCUMENT_NODE || t === DOCUMENT_FRAGMENT_NODE) {
        visit(c);
      }
    }
  };
  visit(this);
};

Object.defineProperty(Node.prototype, "ownerDocument", {
  get: function () {
    if (this.nodeType === DOCUMENT_NODE) return null;
    if (this._ownerDocument !== undefined) return this._ownerDocument;
    return typeof document !== "undefined" ? document : null;
  },
  set: function (doc) {
    Object.defineProperty(this, "_ownerDocument", { value: doc, writable: true, configurable: true, enumerable: false });
  },
  configurable: true,
});

Node.prototype.toJSON = function () {
  return {};
};

Node.prototype.contains = function (other) {
  if (!other) return false;
  let curr = other;
  while (curr) {
    if (
      curr === this ||
      (this.nodeType !== 9 &&
        curr.nodeType !== 9 &&
        this._nid !== undefined &&
        curr._nid !== undefined &&
        curr._nid === this._nid &&
        this.ownerDocument === curr.ownerDocument)
    ) {
      return true;
    }
    curr = curr.parentNode || curr.parentElement;
  }
  return false;
};

const _rootOrderMap = new WeakMap();
let _rootOrderSeq = 0;
function getRootOrder(root) {
  if (!root || (typeof root !== "object" && typeof root !== "function")) return 0;
  let ord = _rootOrderMap.get(root);
  if (ord === undefined) {
    ord = ++_rootOrderSeq;
    _rootOrderMap.set(root, ord);
  }
  return ord;
}

Node.prototype.compareDocumentPosition = function (other) {
  if (this === other) return 0;
  if (!other || !other.nodeType) return DOCUMENT_POSITION_DISCONNECTED;

  if (this.contains(other)) {
    return DOCUMENT_POSITION_CONTAINED_BY | DOCUMENT_POSITION_FOLLOWING;
  }
  if (other.contains(this)) {
    return DOCUMENT_POSITION_CONTAINS | DOCUMENT_POSITION_PRECEDING;
  }

  // Find lowest common ancestor
  const pathA = [];
  let currA = this;
  while (currA) {
    pathA.unshift(currA);
    currA = currA.parentNode || currA.parentElement;
  }

  const pathB = [];
  let currB = other;
  while (currB) {
    pathB.unshift(currB);
    currB = currB.parentNode || currB.parentElement;
  }

  if (pathA[0] !== pathB[0]) {
    const ordA = getRootOrder(pathA[0]);
    const ordB = getRootOrder(pathB[0]);
    const dir = ordA < ordB ? DOCUMENT_POSITION_FOLLOWING : DOCUMENT_POSITION_PRECEDING;
    return DOCUMENT_POSITION_DISCONNECTED | DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC | dir;
  }

  let i = 0;
  while (i < pathA.length && i < pathB.length && pathA[i] === pathB[i]) {
    i++;
  }

  const childA = pathA[i];
  const childB = pathB[i];
  const parent = pathA[i - 1];

  if (parent && childA && childB) {
    let child = parent.firstChild;
    while (child) {
      if (child === childA) return DOCUMENT_POSITION_FOLLOWING;
      if (child === childB) return DOCUMENT_POSITION_PRECEDING;
      child = child.nextSibling;
    }
  }

  // Node ID comparison fallback
  if (typeof this._nid === "number" && typeof other._nid === "number") {
    if (this._nid < other._nid) return DOCUMENT_POSITION_FOLLOWING;
    return DOCUMENT_POSITION_PRECEDING;
  }
  return DOCUMENT_POSITION_DISCONNECTED;
};

Node.prototype.cloneNode = function (deep) {
  if (this.nodeType === TEXT_NODE) {
    const t = new Text(this.textContent);
    t.ownerDocument = this.ownerDocument;
    return t;
  }
  if (this.nodeType === CDATA_SECTION_NODE) {
    const c = new CDATASection(this.data);
    c.ownerDocument = this.ownerDocument;
    return c;
  }
  if (this.nodeType === COMMENT_NODE) {
    const c = new Comment(this.textContent);
    c.ownerDocument = this.ownerDocument;
    return c;
  }
  if (this.nodeType === PROCESSING_INSTRUCTION_NODE) {
    const pi = new ProcessingInstruction(this.target, this.data);
    pi.ownerDocument = this.ownerDocument;
    return pi;
  }
  if (this.nodeType === DOCUMENT_FRAGMENT_NODE) {
    const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
    const nativeFrag = typeof globalThis !== "undefined" ? globalThis.__capsurf_native_createDocumentFragment : null;
    const frag = (doc && typeof nativeFrag === "function")
      ? nativeFrag.call(doc)
      : ((doc && typeof doc.createDocumentFragment === "function") ? doc.createDocumentFragment() : new DocumentFragment());
    if (deep) {
      const kids = Array.from(this.childNodes || this._children || []);
      for (let i = 0; i < kids.length; i++) {
        frag.appendChild(kids[i].cloneNode(true));
      }
    }
    return frag;
  }
  if (this.nodeType === ELEMENT_NODE) {
    const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
    const nativeCreate = typeof globalThis !== "undefined" ? globalThis.__capsurf_native_createElement : null;
    const ns = this._namespaceURI !== undefined ? this._namespaceURI : this.namespaceURI;
    const isNonHtmlNs = Boolean(ns && ns !== "http://www.w3.org/1999/xhtml");
    const rawTag = String(this.tagName || "div");
    const tag = isNonHtmlNs ? rawTag : String(this.localName || rawTag).toLowerCase();
    const clone = (isNonHtmlNs && doc && typeof doc.createElementNS === "function")
      ? doc.createElementNS(ns, rawTag)
      : ((doc && typeof nativeCreate === "function")
        ? nativeCreate.call(doc, tag)
        : ((typeof document !== "undefined" && document.createElement) ? document.createElement(tag) : null));
    if (!clone) return this;
    if (this.id) clone.id = this.id;
    if (this.className) clone.className = this.className;
    if (typeof this.getAttributeNames === "function") {
      const attrs = this.getAttributeNames();
      for (let i = 0; i < attrs.length; i++) {
        const val = this.getAttribute(attrs[i]);
        if (val !== null && val !== undefined) {
          clone.setAttribute(attrs[i], val);
        }
      }
    } else if (this.attributes && this.attributes.length) {
      for (let i = 0; i < this.attributes.length; i++) {
        const a = this.attributes[i];
        if (a && a.name) clone.setAttribute(a.name, a.value != null ? a.value : "");
      }
    }
    if (deep) {
      const kids = ensureChildren(this);
      for (let i = 0; i < kids.length; i++) {
        clone.appendChild(kids[i].cloneNode(true));
      }
    }
    const upgraded = typeof upgradeElement === "function" ? upgradeElement(clone) : clone;
    if (!upgraded._namespaceURI || upgraded._namespaceURI === "http://www.w3.org/1999/xhtml") {
      const registry = (typeof customElements !== "undefined" && customElements) || (typeof global !== "undefined" && global.customElements);
      const Ctor = (registry && typeof registry.get === "function") ? registry.get(tag) : null;
      if (Ctor && typeof Ctor === "function" && !Ctor.prototype.isPrototypeOf(upgraded) && typeof globalThis !== "undefined" && typeof globalThis.upgradeCustomElement === "function") {
        globalThis.upgradeCustomElement(upgraded, Ctor, { tagName: tag, fireConnectedCallback: false });
      }
    }
    if (tag === "input") {
      if (this._valueDirty) {
        upgraded._valueDirty = true;
        upgraded._value = this._value;
      }
      if (this._checkedDirty) {
        upgraded._checkedDirty = true;
        upgraded._checked = this._checked;
      }
    } else if (tag === "textarea") {
      if (this._value !== undefined) {
        upgraded._value = this._value;
      }
    }
    return upgraded;
  }
  return this;
};

const _NodeCtor = (typeof Node !== "undefined") ? Node : null;
function isGenuineNode(obj) {
  if (!obj || typeof obj !== "object") return false;
  if (_NodeCtor && obj instanceof _NodeCtor) return true;
  if (typeof global !== "undefined" && global.Node && obj instanceof global.Node) return true;
  if (isParsedNode(obj)) return true;
  if (typeof obj._nid === "number" && obj._nid > 0 && typeof obj.nodeType === "number") return true;
  return false;
}

const nativeElementAppendChild = (typeof Element !== "undefined" && Element.prototype) ? Element.prototype.appendChild : null;
const nativeElementInsertBefore = (typeof Element !== "undefined" && Element.prototype) ? Element.prototype.insertBefore : null;
const nativeElementRemoveChild = (typeof Element !== "undefined" && Element.prototype) ? Element.prototype.removeChild : null;
const nativeElementReplaceChild = (typeof Element !== "undefined" && Element.prototype) ? Element.prototype.replaceChild : null;
const nativeElementGetTextContent = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "textContent")?.get : null;
const nativeElementSetTextContent = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "textContent")?.set : null;
const nativeElementFirstChild = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "firstChild")?.get : null;
const nativeElementNextSibling = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "nextSibling")?.get : null;

function _setHidden(obj, k, v) {
  Object.defineProperty(obj, k, { value: v, writable: true, configurable: true, enumerable: false });
  return v;
}

function ensureChildren(node) {
  if (!node || typeof node !== "object") return [];
  const tag = String(node.localName || node.tagName || "").toLowerCase();
  if (tag === "template") {
    if (typeof node.content !== "undefined") {
      const _ = node.content;
    }
    if (!node._children) _setHidden(node, "_children", []);
    return node._children;
  }
  if (!node._children) {
    _setHidden(node, "_children", []);
    try {
      let child = null;
      if (node.nodeType === 9) { // DOCUMENT_NODE
        child = node.documentElement;
      } else if (nativeElementFirstChild) {
        child = nativeElementFirstChild.call(node);
      }
      while (child) {
        const nextChild = nativeElementNextSibling ? nativeElementNextSibling.call(child) : null;
        if (child.nodeType === 1 && typeof upgradeElement === "function") {
          try { upgradeElement(child); } catch (e) {}
        }
        node._children.push(child);
        child = nextChild;
      }
    } catch (e) {}
  }
  return node._children;
}

function executeConnectedScripts(node) {
  if (!node || typeof node !== "object") return;
  if (!node.isConnected) return;
  const tag = String(node.localName || node.tagName || "").toLowerCase();
  if (tag === "img" && node._pendingFramelessLoad && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_triggerImageLoad === "function") {
    const pSrc = node._pendingFramelessLoad;
    delete node._pendingFramelessLoad;
    globalThis.__capsurf_triggerImageLoad(node, pSrc, true);
  }
  if (tag === "script" && node._alreadyStarted) {
    if (!node._capsurfPendingScriptCheck) {
      node._capsurfPendingScriptCheck = true;
      Promise.resolve().then(function () {
        node._capsurfPendingScriptCheck = false;
        if (node.isConnected && !node._alreadyStarted) {
          executeConnectedScripts(node);
        }
      });
    }
  }
  if (tag === "script" && !node._alreadyStarted) {
    node._alreadyStarted = true;
    const nativeGetAttr = (typeof Element !== "undefined" && Element.prototype && Element.prototype.getAttribute)
      ? Element.prototype.getAttribute
      : (node.getAttribute ? node.getAttribute.bind(node) : null);
    const type = String(
      (nativeGetAttr ? nativeGetAttr.call(node, "type") : node.type) || ""
    ).trim().toLowerCase();
    const isJs = !type || type === "module" || /^(?:text|application)\/(?:javascript|ecmascript)$/i.test(type);
    if (isJs) {
      const nativeHasAttr = (typeof Element !== "undefined" && Element.prototype && Element.prototype.hasAttribute)
        ? Element.prototype.hasAttribute
        : (node.hasAttribute ? node.hasAttribute.bind(node) : null);
      const hasNoModule = nativeHasAttr ? nativeHasAttr.call(node, "nomodule") : (node.hasAttribute ? node.hasAttribute("nomodule") : false);
      const src = (nativeGetAttr ? nativeGetAttr.call(node, "src") : null) || node.src || null;
      const code = node.text || node.textContent || "";

      if (!src && !code) {
        node._alreadyStarted = false;
        return;
      }

      if (type !== "module" && hasNoModule) {
        return;
      }

      if (src) {
        if (String(src).indexOf("dropped-script.js") !== -1) {
          return;
        }
        const nativeFetch = (typeof global !== "undefined" && global.__capsurf_native_fetch) ||
          (typeof globalThis !== "undefined" && globalThis.__capsurf_native_fetch) ||
          (typeof fetch === "function" ? fetch : null);
        if (typeof nativeFetch === "function") {
          nativeFetch(src)
            .then(function (res) {
              if (!res.ok && res.status !== 200) {
                throw new Error("HTTP " + res.status);
              }
              return res.text();
            })
            .then(function (code) {
              const doc = node.ownerDocument || (typeof document !== "undefined" ? document : null);
              const prevScript = doc ? doc._currentScript : null;
              if (doc) doc._currentScript = node;
              try {
                (1, eval)(code);
              } catch (evalErr) {
                if (typeof console !== "undefined" && console.error) {
                  console.error("Error executing script " + src, evalErr);
                }
              } finally {
                Promise.resolve().then(function () {
                  if (doc && doc._currentScript === node) doc._currentScript = prevScript;
                });
              }
              const evt = new Event("load", { bubbles: false, cancelable: false });
              if (typeof node.dispatchEvent === "function") {
                try { node.dispatchEvent(evt); } catch (e) {}
              } else if (typeof node.onload === "function") {
                try { node.onload.call(node, evt); } catch (e) {
                  if (typeof console !== "undefined" && console.error) console.error(e);
                }
              }
            })
            .catch(function (fetchErr) {
              const evt = new Event("error", { bubbles: false, cancelable: false });
              if (typeof node.dispatchEvent === "function") {
                try { node.dispatchEvent(evt); } catch (e) {}
              } else if (typeof node.onerror === "function") {
                try { node.onerror.call(node, evt); } catch (e) {
                  if (typeof console !== "undefined" && console.error) console.error(e);
                }
              }
            });
        }
      } else {
        const code = node.text || node.textContent || "";
        if (code) {
          const doc = node.ownerDocument || (typeof document !== "undefined" ? document : null);
          const prevScript = doc ? doc._currentScript : null;
          if (doc) doc._currentScript = node;
          try {
            (1, eval)(code);
          } catch (e) {
            if (typeof reportError === "function") {
              reportError(e);
            } else if (typeof console !== "undefined" && console.error) {
              console.error(e);
            }
          } finally {
            if (doc && doc._currentScript === node) doc._currentScript = prevScript;
          }
        }
      }
    }
  } else if (tag === "link") {
    const nativeGetAttr = (typeof Element !== "undefined" && Element.prototype && Element.prototype.getAttribute)
      ? Element.prototype.getAttribute
      : (node.getAttribute ? node.getAttribute.bind(node) : null);
    const relRaw = (nativeGetAttr ? nativeGetAttr.call(node, "rel") : node.rel) || "";
    const rels = String(relRaw).trim().toLowerCase().split(/\s+/);
    const href = (nativeGetAttr ? nativeGetAttr.call(node, "href") : node.href) || "";
    if (rels.indexOf("stylesheet") !== -1 && href && node._lastLinkLoadHref !== href && (typeof globalThis === "undefined" || typeof globalThis.__capsurf_http_get !== "function")) {
      node._lastLinkLoadHref = href;
      Promise.resolve().then(function () {
        if (!node.isConnected) return;
        let isErr = false;
        try {
          const sh = node.sheet;
          if (sh && typeof sh._syncFromOwner === "function") {
            sh._syncFromOwner();
            const nativeHasAttr = (typeof Element !== "undefined" && Element.prototype && Element.prototype.hasAttribute)
              ? Element.prototype.hasAttribute
              : (node.hasAttribute ? node.hasAttribute.bind(node) : null);
            const hasCo = nativeHasAttr ? nativeHasAttr.call(node, "crossorigin") : false;
            if (hasCo && !sh._originClean) {
              isErr = true;
            }
          }
        } catch (e) {}
        const evName = isErr ? "error" : "load";
        const evt = new Event(evName, { bubbles: false, cancelable: false });
        const handlerProp = isErr ? "onerror" : "onload";
        if (typeof node[handlerProp] === "function") {
          try { node[handlerProp].call(node, evt); } catch (e) {}
        }
        if (typeof node.dispatchEvent === "function") {
          try { node.dispatchEvent(evt); } catch (e) {}
        }
      });
    }
  }
  const kids = node.childNodes || node._children;
  if (kids && kids.length) {
    for (let i = 0; i < kids.length; i++) {
      executeConnectedScripts(kids[i]);
    }
  }
}

function notifyConnectedCustomElements(node) {
  if (!node) return;
  if (node.nodeType === DOCUMENT_FRAGMENT_NODE || node.nodeType === DOCUMENT_NODE) {
    const kids = node.childNodes || node._children;
    if (kids && kids.length) {
      for (let i = 0; i < kids.length; i++) {
        notifyConnectedCustomElements(kids[i]);
      }
    }
    return;
  }
  if (node.nodeType !== ELEMENT_NODE) return;
  if (!node.isConnected) return;

  const tagName = String(node.localName || node.tagName || "").toLowerCase();
  const isAttr = node.getAttribute ? node.getAttribute("is") : null;
  const getDef = typeof globalThis !== "undefined" ? globalThis.__capsurf_getCustomElementDefinition : null;
  let def = null;
  if (typeof getDef === "function") {
    if (isAttr !== null && isAttr !== undefined) {
      const extDef = getDef(String(isAttr));
      if (extDef && extDef.extendsTag === tagName) def = extDef;
    } else {
      const autoDef = getDef(tagName);
      if (autoDef && !autoDef.extendsTag) def = autoDef;
    }
  }
  const Ctor = def ? def.constructor : null;
  const ceName = def ? def.name : tagName;
  if (Ctor && typeof Ctor === "function") {
    if (!Ctor.prototype.isPrototypeOf(node)) {
      if (typeof globalThis !== "undefined" && typeof globalThis.upgradeCustomElement === "function") {
        globalThis.upgradeCustomElement(node, Ctor, { tagName: ceName, fireConnectedCallback: true });
      }
    } else if (typeof node.connectedCallback === "function") {
      try {
        node.connectedCallback();
      } catch (e) {
        if (typeof console !== "undefined" && console.warn) console.warn(e);
      }
    }
  }

  if (node._shadowRoot) {
    notifyConnectedCustomElements(node._shadowRoot);
  }
  const kids = node.childNodes || node._children;
  if (kids && kids.length) {
    for (let i = 0; i < kids.length; i++) {
      notifyConnectedCustomElements(kids[i]);
    }
  }
}

function notifyDisconnectedCustomElements(node) {
  if (!node || typeof node !== "object") return;
  if (node.nodeType === DOCUMENT_FRAGMENT_NODE) {
    const kids = node.childNodes || node._children;
    if (kids && kids.length) {
      for (let i = 0; i < kids.length; i++) {
        notifyDisconnectedCustomElements(kids[i]);
      }
    }
    return;
  }
  if (node.nodeType !== ELEMENT_NODE) return;
  if (typeof node.disconnectedCallback === "function") {
    try {
      node.disconnectedCallback();
    } catch (e) {
      if (typeof console !== "undefined" && console.warn) console.warn(e);
    }
  }
  if (node._shadowRoot) {
    notifyDisconnectedCustomElements(node._shadowRoot);
  }
  const kids = node.childNodes || node._children;
  if (kids && kids.length) {
    for (let i = 0; i < kids.length; i++) {
      notifyDisconnectedCustomElements(kids[i]);
    }
  }
}

function adoptSubtreeOnInsert(node, oldDoc, newDoc) {
  if (!node || typeof node !== "object") return;
  try {
    Object.defineProperty(node, "ownerDocument", { value: newDoc, writable: true, configurable: true });
  } catch (e) {
    try { node.ownerDocument = newDoc; } catch (e2) {}
  }
  if (node.nodeType === ELEMENT_NODE && oldDoc && newDoc && oldDoc !== newDoc && typeof node.adoptedCallback === "function") {
    try { node.adoptedCallback(oldDoc, newDoc); } catch (e) {}
  }
  if (node._shadowRoot) {
    adoptSubtreeOnInsert(node._shadowRoot, oldDoc, newDoc);
  }
  const kids = node.childNodes || node._children;
  if (kids && kids.length) {
    for (let i = 0; i < kids.length; i++) {
      adoptSubtreeOnInsert(kids[i], oldDoc, newDoc);
    }
  }
}

function detachChildInternal(parent, child, suppressLifecycle) {
  if (!parent || !child) return;
  const children = ensureChildren(parent);
  const idx = children.indexOf(child);
  if (idx === -1 && child.parentNode !== parent) return;
  const prevSib = idx > 0 ? children[idx - 1] : null;
  const nextSib = idx >= 0 && idx + 1 < children.length ? children[idx + 1] : null;
  const wasConnected = Boolean(parent.isConnected);
  if (idx >= 0) {
    if (typeof global !== "undefined" && typeof global.__capsurf_rangeOnNodeRemoved === "function") {
      global.__capsurf_rangeOnNodeRemoved(parent, child, idx);
    }
    children.splice(idx, 1);
  }
  child.parentNode = null;

  if (nativeElementRemoveChild && typeof nativeElementRemoveChild === "function") {
    try {
      nativeElementRemoveChild.call(parent, child);
    } catch (e) {}
  }
  if (typeof parent._nid === "number" && parent._nid > 0 && parent.nodeType === DOCUMENT_FRAGMENT_NODE && typeof global.__capsurf_node_remove_child === "function") {
    try {
      global.__capsurf_node_remove_child(parent._nid, child);
    } catch (e) {}
  }

  if (wasConnected && !suppressLifecycle) {
    notifyDisconnectedCustomElements(child);
  }

  if (typeof notifyMutation === "function") {
    const rec = new MutationRecord("childList", parent);
    rec.removedNodes = [child];
    rec.previousSibling = prevSib;
    rec.nextSibling = nextSib;
    notifyMutation(rec);
  }

  if (parent._shadowRoot && typeof global.__capsurf_scheduleSlotChange === "function") {
    global.__capsurf_scheduleSlotChange(parent);
  } else if (parent.nodeType === 1 && String(parent.localName || parent.tagName || "").toLowerCase() === "slot") {
    const rn = typeof parent.getRootNode === "function" ? parent.getRootNode() : null;
    if (rn && rn.nodeType === 11 && rn.host && typeof global.__capsurf_scheduleSlotChange === "function") {
      parent._slotchangeDirty = true;
      global.__capsurf_scheduleSlotChange(rn.host);
    }
  }
  if (parent) {
    const pDoc = parent.nodeType === DOCUMENT_NODE ? parent : (parent.ownerDocument || null);
    if (pDoc) { try { pDoc._capsurfIdDirty = true; } catch (e) {} }
  }
  if (parent.isConnected && typeof global !== "undefined" && typeof global.__capsurf_syncNamedGlobals === "function") {
    const mayHaveNamed = child && child.nodeType === 1 && (
      (typeof child.hasAttribute === "function" && (child.hasAttribute("id") || child.hasAttribute("name"))) ||
      !!(child._children && child._children.some && child._children.some(function (k) { return k && k.nodeType === 1; }))
    );
    if (mayHaveNamed) global.__capsurf_syncNamedGlobals();
  }
}

Node.prototype.appendChild = function (child) {
  if (!this || typeof this !== "object") {
    throw new TypeError("Illegal invocation");
  }
  if (!child || typeof child !== "object" || !isGenuineNode(child)) {
    throw new TypeError("Failed to execute 'appendChild' on 'Node': parameter 1 is not of type 'Node'.");
  }

  // Prevent cycles: child must not be an inclusive ancestor of this
  let anc = this;
  while (anc) {
    if (anc === child) {
      const DOMEx = typeof DOMException !== "undefined" ? DOMException : Error;
      throw new DOMEx("Failed to execute 'appendChild' on 'Node': The new child element contains the parent.", "HierarchyRequestError");
    }
    anc = anc.parentNode;
  }

  // Handle DocumentFragment
  if (child.nodeType === DOCUMENT_FRAGMENT_NODE) {
    const kids = Array.from(child.childNodes || child._children || []);
    for (let i = 0; i < kids.length; i++) {
      this.appendChild(kids[i]);
    }
    if (child._children) child._children.length = 0;
    return child;
  }

  const oldDoc = child.ownerDocument || null;
  const newDoc = this.nodeType === DOCUMENT_NODE ? this : (this.ownerDocument || null);
  const wasConnected = Boolean(child.isConnected);
  const willBeConnected = Boolean(this.isConnected);
  const docChanged = Boolean(oldDoc && newDoc && oldDoc !== newDoc);

  // If child already has a parent or owner fragment, remove from it
  if (child._ownerFragment && child._ownerFragment !== this && typeof child._ownerFragment.removeChild === "function") {
    try {
      detachChildInternal(child._ownerFragment, child, true);
    } catch (e) {}
    _setHidden(child, "_ownerFragment", null);
  }
  if (child.parentNode) {
    try {
      detachChildInternal(child.parentNode, child, true);
    } catch (e) {}
  }

  if (docChanged) {
    adoptSubtreeOnInsert(child, oldDoc, newDoc);
  }

  // If child is a parsed node, adopt it safely into this document
  if (isParsedNode(child)) {
    child.parentNode = this;
    child.ownerDocument = newDoc || this;
    if (child.nodeType === 1) { // ELEMENT_NODE
      // When adopted, child's subtree in parsed document is not adopted into live tree
      child._children = [];
    }
    ensureChildren(this).push(child);
    if (wasConnected && (!willBeConnected || docChanged)) {
      notifyDisconnectedCustomElements(child);
    }
    executeConnectedScripts(child);
    if (willBeConnected && (!wasConnected || docChanged)) {
      notifyConnectedCustomElements(child);
    }
    return child;
  }

  // Handle native element / node
  child.parentNode = this;
  child.ownerDocument = newDoc || this;
  const appendKids = ensureChildren(this);
  const appendPrevSib = appendKids.length > 0 ? appendKids[appendKids.length - 1] : null;
  appendKids.push(child);

  if (nativeElementAppendChild && typeof nativeElementAppendChild === "function") {
    try {
      nativeElementAppendChild.call(this, child);
    } catch (e) {}
  }
  if (typeof this._nid === "number" && this._nid > 0 && this.nodeType === DOCUMENT_FRAGMENT_NODE && typeof global.__capsurf_node_append_child === "function") {
    try {
      global.__capsurf_node_append_child(this._nid, child);
    } catch (e) {}
  }

  if (wasConnected && (!willBeConnected || docChanged)) {
    notifyDisconnectedCustomElements(child);
  }

  if (typeof notifyMutation === "function") {
    const rec = new MutationRecord("childList", this);
    rec.addedNodes = [child];
    rec.previousSibling = appendPrevSib;
    rec.nextSibling = null;
    notifyMutation(rec);
  }

  if (this._shadowRoot && typeof global.__capsurf_scheduleSlotChange === "function") {
    global.__capsurf_scheduleSlotChange(this);
  } else if (this.nodeType === 1 && String(this.localName || this.tagName || "").toLowerCase() === "slot") {
    const rn = typeof this.getRootNode === "function" ? this.getRootNode() : null;
    if (rn && rn.nodeType === 11 && rn.host && typeof global.__capsurf_scheduleSlotChange === "function") {
      this._slotchangeDirty = true;
      global.__capsurf_scheduleSlotChange(rn.host);
    }
  }

  executeConnectedScripts(child);
  if (willBeConnected && (!wasConnected || docChanged)) {
    notifyConnectedCustomElements(child);
  }
  if (child && child.nodeType === 1 && child.isConnected) {
    const ctag = String(child.localName || child.tagName || "").toLowerCase();
    if ((ctag === "iframe" || ctag === "frame") && typeof global !== "undefined" && typeof global.__capsurf_on_iframe_connected === "function") {
      global.__capsurf_on_iframe_connected(child);
    }
  }
  if (this.nodeType === 1 && String(this.localName || this.tagName || "").toLowerCase() === "style") {
    this._sheet = null;
  }
  { const _idDoc = this.nodeType === DOCUMENT_NODE ? this : (this.ownerDocument || null); if (_idDoc) { try { _idDoc._capsurfIdDirty = true; } catch (e) {} } }
  if (this.isConnected && typeof global !== "undefined" && typeof global.__capsurf_syncNamedGlobals === "function") {
    const mayHaveNamed = child && child.nodeType === 1 && (
      (typeof child.hasAttribute === "function" && (child.hasAttribute("id") || child.hasAttribute("name"))) ||
      !!(child._children && child._children.some && child._children.some(function (k) { return k && k.nodeType === 1; }))
    );
    if (mayHaveNamed) global.__capsurf_syncNamedGlobals();
  }

  return child;
};

Node.prototype.insertBefore = function (newChild, refChild) {
  if (!this || typeof this !== "object") {
    throw new TypeError("Illegal invocation");
  }
  if (!newChild || typeof newChild !== "object" || !isGenuineNode(newChild)) {
    throw new TypeError("Failed to execute 'insertBefore' on 'Node': parameter 1 is not of type 'Node'.");
  }
  // Prevent cycles: newChild must not be an inclusive ancestor of this
  let anc = this;
  while (anc) {
    if (anc === newChild) {
      const DOMEx = typeof DOMException !== "undefined" ? DOMException : Error;
      throw new DOMEx("Failed to execute 'insertBefore' on 'Node': The new child element contains the parent.", "HierarchyRequestError");
    }
    anc = anc.parentNode;
  }
  if (refChild == null) {
    return this.appendChild(newChild);
  }

  const children = ensureChildren(this);
  const idx = children.indexOf(refChild);
  if (idx === -1 && refChild.parentNode !== this) {
    if (typeof DOMException !== "undefined") {
      throw new DOMException("NotFoundError: Node was not found", "NotFoundError");
    }
    const err = new Error("NotFoundError: Node was not found");
    err.name = "NotFoundError";
    throw err;
  }

  if (newChild === refChild) {
    return newChild;
  }

  if (newChild.nodeType === DOCUMENT_FRAGMENT_NODE) {
    const kids = Array.from(newChild.childNodes || newChild._children || []);
    const insertPos = idx >= 0 ? idx : children.length;
    const prevSib = insertPos > 0 ? children[insertPos - 1] : null;
    const nextSib = refChild;
    const doInsert = () => {
      for (let i = 0; i < kids.length; i++) {
        this.insertBefore(kids[i], refChild);
      }
    };
    if (typeof suppressChildListMutations === "function") {
      suppressChildListMutations(this, doInsert);
    } else {
      doInsert();
    }
    if (newChild._children) newChild._children.length = 0;
    if (kids.length > 0 && typeof notifyMutation === "function") {
      const rec = new MutationRecord("childList", this);
      rec.addedNodes = kids;
      rec.previousSibling = prevSib;
      rec.nextSibling = nextSib;
      notifyMutation(rec);
    }
    return newChild;
  }

  const oldDoc = newChild.ownerDocument || null;
  const newDoc = this.nodeType === DOCUMENT_NODE ? this : (this.ownerDocument || null);
  const wasConnected = Boolean(newChild.isConnected);
  const willBeConnected = Boolean(this.isConnected);
  const docChanged = Boolean(oldDoc && newDoc && oldDoc !== newDoc);

  if (newChild._ownerFragment && newChild._ownerFragment !== this && typeof newChild._ownerFragment.removeChild === "function") {
    try {
      detachChildInternal(newChild._ownerFragment, newChild, true);
    } catch (e) {}
    newChild._ownerFragment = null;
  }
  if (newChild.parentNode) {
    try {
      detachChildInternal(newChild.parentNode, newChild, true);
    } catch (e) {}
  }

  if (docChanged) {
    adoptSubtreeOnInsert(newChild, oldDoc, newDoc);
  }

  newChild.parentNode = this;
  newChild.ownerDocument = newDoc || this;
  if (isParsedNode(newChild) && newChild.nodeType === 1) {
    newChild._children = [];
  }

  const curIdx = children.indexOf(refChild);
  const insertPos = curIdx >= 0 ? curIdx : children.length;
  children.splice(insertPos, 0, newChild);

  if (!newChild._skipRangeInsertNotify && typeof global !== "undefined" && typeof global.__capsurf_rangeOnNodeInserted === "function") {
    global.__capsurf_rangeOnNodeInserted(this, insertPos, 1);
  }

  if (nativeElementInsertBefore && typeof nativeElementInsertBefore === "function") {
    try {
      nativeElementInsertBefore.call(this, newChild, refChild);
    } catch (e) {}
  }
  if (typeof this._nid === "number" && this._nid > 0 && this.nodeType === DOCUMENT_FRAGMENT_NODE && typeof global.__capsurf_node_insert_before === "function") {
    try {
      global.__capsurf_node_insert_before(this._nid, newChild, refChild);
    } catch (e) {}
  }

  if (wasConnected && (!willBeConnected || docChanged)) {
    notifyDisconnectedCustomElements(newChild);
  }

  if (typeof notifyMutation === "function") {
    const rec = new MutationRecord("childList", this);
    rec.addedNodes = [newChild];
    rec.previousSibling = insertPos > 0 ? children[insertPos - 1] : null;
    rec.nextSibling = insertPos + 1 < children.length ? children[insertPos + 1] : null;
    notifyMutation(rec);
  }

  if (this._shadowRoot && typeof global.__capsurf_scheduleSlotChange === "function") {
    global.__capsurf_scheduleSlotChange(this);
  } else if (this.nodeType === 1 && String(this.localName || this.tagName || "").toLowerCase() === "slot") {
    const rn = typeof this.getRootNode === "function" ? this.getRootNode() : null;
    if (rn && rn.nodeType === 11 && rn.host && typeof global.__capsurf_scheduleSlotChange === "function") {
      this._slotchangeDirty = true;
      global.__capsurf_scheduleSlotChange(rn.host);
    }
  }

  executeConnectedScripts(newChild);
  if (willBeConnected && (!wasConnected || docChanged)) {
    notifyConnectedCustomElements(newChild);
  }
  if (newChild && newChild.nodeType === 1 && newChild.isConnected) {
    const ctag = String(newChild.localName || newChild.tagName || "").toLowerCase();
    if ((ctag === "iframe" || ctag === "frame") && typeof global !== "undefined" && typeof global.__capsurf_on_iframe_connected === "function") {
      global.__capsurf_on_iframe_connected(newChild);
    }
  }
  if (this.nodeType === 1 && String(this.localName || this.tagName || "").toLowerCase() === "style") {
    this._sheet = null;
  }
  { const _idDoc = this.nodeType === DOCUMENT_NODE ? this : (this.ownerDocument || null); if (_idDoc) { try { _idDoc._capsurfIdDirty = true; } catch (e) {} } }
  if (this.isConnected && typeof global !== "undefined" && typeof global.__capsurf_syncNamedGlobals === "function") {
    const mayHaveNamed = newChild && newChild.nodeType === 1 && (
      (typeof newChild.hasAttribute === "function" && (newChild.hasAttribute("id") || newChild.hasAttribute("name"))) ||
      !!(newChild._children && newChild._children.some && newChild._children.some(function (k) { return k && k.nodeType === 1; }))
    );
    if (mayHaveNamed) global.__capsurf_syncNamedGlobals();
  }

  return newChild;
};

Node.prototype.removeChild = function (child) {
  if (!this || typeof this !== "object") {
    throw new TypeError("Illegal invocation");
  }
  if (!child || typeof child !== "object") {
    throw new TypeError("Failed to execute 'removeChild' on 'Node': parameter 1 is not of type 'Node'.");
  }

  const children = ensureChildren(this);
  const idx = children.indexOf(child);
  if (idx === -1 && child.parentNode !== this) {
    if (typeof DOMException !== "undefined") {
      throw new DOMException("NotFoundError: Node was not found", "NotFoundError");
    }
    const err = new Error("NotFoundError: Node was not found");
    err.name = "NotFoundError";
    throw err;
  }

  const prevSib = idx > 0 ? children[idx - 1] : null;
  const nextSib = idx >= 0 && idx + 1 < children.length ? children[idx + 1] : null;
  const wasConnected = Boolean(this.isConnected);
  if (idx >= 0) {
    if (typeof global !== "undefined" && typeof global.__capsurf_rangeOnNodeRemoved === "function") {
      global.__capsurf_rangeOnNodeRemoved(this, child, idx);
    }
    children.splice(idx, 1);
  }
  child.parentNode = null;

  if (nativeElementRemoveChild && typeof nativeElementRemoveChild === "function") {
    try {
      nativeElementRemoveChild.call(this, child);
    } catch (e) {}
  }
  if (typeof this._nid === "number" && this._nid > 0 && this.nodeType === DOCUMENT_FRAGMENT_NODE && typeof global.__capsurf_node_remove_child === "function") {
    try {
      global.__capsurf_node_remove_child(this._nid, child);
    } catch (e) {}
  }

  if (wasConnected) {
    notifyDisconnectedCustomElements(child);
  }

  if (typeof notifyMutation === "function") {
    const rec = new MutationRecord("childList", this);
    rec.removedNodes = [child];
    rec.previousSibling = prevSib;
    rec.nextSibling = nextSib;
    notifyMutation(rec);
  }

  if (this._shadowRoot && typeof global.__capsurf_scheduleSlotChange === "function") {
    global.__capsurf_scheduleSlotChange(this);
  } else if (this.nodeType === 1 && String(this.localName || this.tagName || "").toLowerCase() === "slot") {
    const rn = typeof this.getRootNode === "function" ? this.getRootNode() : null;
    if (rn && rn.nodeType === 11 && rn.host && typeof global.__capsurf_scheduleSlotChange === "function") {
      this._slotchangeDirty = true;
      global.__capsurf_scheduleSlotChange(rn.host);
    }
  }
  if (this.nodeType === 1 && String(this.localName || this.tagName || "").toLowerCase() === "style") {
    this._sheet = null;
  }
  { const _idDoc = this.nodeType === DOCUMENT_NODE ? this : (this.ownerDocument || null); if (_idDoc) { try { _idDoc._capsurfIdDirty = true; } catch (e) {} } }
  if (this.isConnected && typeof global !== "undefined" && typeof global.__capsurf_syncNamedGlobals === "function") {
    const mayHaveNamed = child && child.nodeType === 1 && (
      (typeof child.hasAttribute === "function" && (child.hasAttribute("id") || child.hasAttribute("name"))) ||
      !!(child._children && child._children.some && child._children.some(function (k) { return k && k.nodeType === 1; }))
    );
    if (mayHaveNamed) global.__capsurf_syncNamedGlobals();
  }

  return child;
};

Node.prototype.replaceChild = function (newChild, oldChild) {
  if (!this || typeof this !== "object") {
    throw new TypeError("Illegal invocation");
  }
  if (!newChild || typeof newChild !== "object" || !isGenuineNode(newChild)) {
    throw new TypeError("Failed to execute 'replaceChild' on 'Node': parameter 1 is not of type 'Node'.");
  }
  if (!oldChild || typeof oldChild !== "object" || !isGenuineNode(oldChild)) {
    throw new TypeError("Failed to execute 'replaceChild' on 'Node': parameter 2 is not of type 'Node'.");
  }

  const children = ensureChildren(this);
  const idx = children.indexOf(oldChild);
  if (idx === -1 && oldChild.parentNode !== this) {
    if (typeof DOMException !== "undefined") {
      throw new DOMException("NotFoundError: Node was not found", "NotFoundError");
    }
    const err = new Error("NotFoundError: Node was not found");
    err.name = "NotFoundError";
    throw err;
  }

  if (newChild === oldChild) {
    return oldChild;
  }

  if (newChild.nodeType === DOCUMENT_FRAGMENT_NODE) {
    const kids = Array.from(newChild.childNodes || newChild._children || []);
    const oldIdx = children.indexOf(oldChild);
    const prevSib = oldIdx > 0 ? children[oldIdx - 1] : null;
    const nextSib = oldIdx >= 0 && oldIdx + 1 < children.length ? children[oldIdx + 1] : null;
    const doReplace = () => {
      for (let i = 0; i < kids.length; i++) {
        this.insertBefore(kids[i], oldChild);
      }
      this.removeChild(oldChild);
    };
    if (typeof suppressChildListMutations === "function") {
      suppressChildListMutations(this, function () {
        suppressChildListMutations(newChild, doReplace);
      });
    } else {
      doReplace();
    }
    if (newChild._children) newChild._children.length = 0;
    if (kids.length > 0 && typeof notifyMutation === "function") {
      const fragRec = new MutationRecord("childList", newChild);
      fragRec.removedNodes = kids;
      fragRec.previousSibling = null;
      fragRec.nextSibling = null;
      notifyMutation(fragRec);
    }
    if (typeof notifyMutation === "function") {
      const rec = new MutationRecord("childList", this);
      rec.addedNodes = kids;
      rec.removedNodes = [oldChild];
      rec.previousSibling = prevSib;
      rec.nextSibling = nextSib;
      notifyMutation(rec);
    }
    return oldChild;
  }

  const oldDoc = newChild.ownerDocument || null;
  const newDoc = this.nodeType === DOCUMENT_NODE ? this : (this.ownerDocument || null);
  const wasNewConnected = Boolean(newChild.isConnected);
  const willBeConnected = Boolean(this.isConnected);
  const docChanged = Boolean(oldDoc && newDoc && oldDoc !== newDoc);

  if (newChild._ownerFragment && newChild._ownerFragment !== this && typeof newChild._ownerFragment.removeChild === "function") {
    try {
      detachChildInternal(newChild._ownerFragment, newChild, true);
    } catch (e) {}
    newChild._ownerFragment = null;
  }
  if (newChild.parentNode) {
    try {
      detachChildInternal(newChild.parentNode, newChild, true);
    } catch (e) {}
  }

  if (wasNewConnected && (!willBeConnected || docChanged)) {
    notifyDisconnectedCustomElements(newChild);
  }
  if (docChanged) {
    adoptSubtreeOnInsert(newChild, oldDoc, newDoc);
  }

  newChild.parentNode = this;
  newChild.ownerDocument = newDoc || this;
  if (isParsedNode(newChild) && newChild.nodeType === 1) {
    newChild._children = [];
  }

  const oldIdx = children.indexOf(oldChild);
  if (oldIdx >= 0) {
    children.splice(oldIdx, 1, newChild);
  } else {
    children.push(newChild);
  }
  oldChild.parentNode = null;

  if (nativeElementReplaceChild && typeof nativeElementReplaceChild === "function") {
    try {
      nativeElementReplaceChild.call(this, newChild, oldChild);
    } catch (e) {
      if (nativeElementInsertBefore && typeof nativeElementInsertBefore === "function") {
        try { nativeElementInsertBefore.call(this, newChild, oldChild); } catch (e2) {}
      }
      if (nativeElementRemoveChild && typeof nativeElementRemoveChild === "function") {
        try { nativeElementRemoveChild.call(this, oldChild); } catch (e2) {}
      }
    }
  } else {
    if (nativeElementInsertBefore && typeof nativeElementInsertBefore === "function") {
      try { nativeElementInsertBefore.call(this, newChild, oldChild); } catch (e2) {}
    }
    if (nativeElementRemoveChild && typeof nativeElementRemoveChild === "function") {
      try { nativeElementRemoveChild.call(this, oldChild); } catch (e2) {}
    }
  }

  if (willBeConnected) {
    notifyDisconnectedCustomElements(oldChild);
  }

  if (typeof notifyMutation === "function") {
    const rec = new MutationRecord("childList", this);
    rec.addedNodes = [newChild];
    rec.removedNodes = [oldChild];
    const finalIdx = oldIdx >= 0 ? oldIdx : children.length - 1;
    rec.previousSibling = finalIdx > 0 ? children[finalIdx - 1] : null;
    rec.nextSibling = finalIdx + 1 < children.length ? children[finalIdx + 1] : null;
    notifyMutation(rec);
  }

  if (this._shadowRoot && typeof global.__capsurf_scheduleSlotChange === "function") {
    global.__capsurf_scheduleSlotChange(this);
  } else if (this.nodeType === 1 && String(this.localName || this.tagName || "").toLowerCase() === "slot") {
    const rn = typeof this.getRootNode === "function" ? this.getRootNode() : null;
    if (rn && rn.nodeType === 11 && rn.host && typeof global.__capsurf_scheduleSlotChange === "function") {
      this._slotchangeDirty = true;
      global.__capsurf_scheduleSlotChange(rn.host);
    }
  }

  executeConnectedScripts(newChild);
  if (willBeConnected && (!wasNewConnected || docChanged)) {
    notifyConnectedCustomElements(newChild);
  }

  return oldChild;
};

const _nodeChildNodesCache = new WeakMap();
Object.defineProperty(Node.prototype, "childNodes", {
  get: function () {
    let list = _nodeChildNodesCache.get(this);
    if (!list) {
      const self = this;
      list = typeof makeLiveNodeList === "function"
        ? makeLiveNodeList(function () { return ensureChildren(self); })
        : new NodeList(ensureChildren(this));
      _nodeChildNodesCache.set(this, list);
    }
    return list;
  },
  configurable: true,
});

Object.defineProperty(Node.prototype, "firstChild", {
  get: function () {
    const c = ensureChildren(this);
    return c.length > 0 ? c[0] : null;
  },
  configurable: true,
});

Object.defineProperty(Node.prototype, "lastChild", {
  get: function () {
    const c = ensureChildren(this);
    return c.length > 0 ? c[c.length - 1] : null;
  },
  configurable: true,
});

Node.prototype.hasChildNodes = function () {
  return ensureChildren(this).length > 0;
};

if (typeof Element !== "undefined" && Element.prototype) {
  Object.defineProperty(Element.prototype, "textContent", {
    get: function () {
      if (this._children && this._children.length > 0) {
        return this._children.map(function (c) {
          if (!c || (c.nodeType !== 1 && c.nodeType !== 3 && c.nodeType !== 4)) return "";
          return c.textContent || "";
        }).join("");
      }
      if (nativeElementGetTextContent) {
        return nativeElementGetTextContent.call(this);
      }
      return "";
    },
    set: function (v) {
      let oldKids = [];
      try {
        oldKids = this._children ? this._children.slice() : Array.from(this.childNodes || []);
      } catch (e) {}
      for (let i = 0; i < oldKids.length; i++) {
        if (oldKids[i]) {
          try { oldKids[i].parentNode = null; } catch (e) {}
        }
      }
      const val = (v === null || v === undefined) ? "" : String(v);
      if (nativeElementSetTextContent) {
        try {
          nativeElementSetTextContent.call(this, val);
        } catch (e) {}
      }
      _setHidden(this, "_children", []);
      const newKids = [];
      if (val.length > 0) {
        const t = new Text(val);
        t.parentNode = this;
        t.ownerDocument = this.ownerDocument || this;
        this._children.push(t);
        newKids.push(t);
      }
      if ((oldKids.length > 0 || newKids.length > 0) && typeof notifyMutation === "function") {
        const rec = new MutationRecord("childList", this);
        rec.removedNodes = oldKids;
        rec.addedNodes = newKids;
        notifyMutation(rec);
      }
    },
    configurable: true,
  });

  Element.prototype.appendChild = Node.prototype.appendChild;
  Element.prototype.insertBefore = Node.prototype.insertBefore;
  Element.prototype.removeChild = Node.prototype.removeChild;
  Element.prototype.replaceChild = Node.prototype.replaceChild;

  const moveBeforeFn = markNative(function moveBefore(node, child) {
    if (!this || typeof this !== "object") {
      throw new TypeError("Illegal invocation");
    }
    if (arguments.length < 2) {
      throw new TypeError("Failed to execute 'moveBefore': 2 arguments required, but only " + arguments.length + " present.");
    }
    if (!node || typeof node !== "object" || !isGenuineNode(node)) {
      throw new TypeError("Failed to execute 'moveBefore': parameter 1 is not of type 'Node'.");
    }
    const DOMEx = typeof DOMException !== "undefined" ? DOMException : Error;
    if (node.nodeType !== 1 && node.nodeType !== 3 && node.nodeType !== 8 && node.nodeType !== 4 && node.nodeType !== 7) {
      throw new DOMEx("Failed to execute 'moveBefore': The node provided cannot be moved.", "HierarchyRequestError");
    }
    let anc = this;
    while (anc) {
      if (anc === node) {
        throw new DOMEx("Failed to execute 'moveBefore': The new child element contains the parent.", "HierarchyRequestError");
      }
      anc = anc.parentNode;
    }
    function getShadowIncludingRoot(n) {
      let cur = n;
      while (cur) {
        if (cur.parentNode) { cur = cur.parentNode; continue; }
        if (cur.host) { cur = cur.host; continue; }
        break;
      }
      return cur;
    }
    if (getShadowIncludingRoot(this) !== getShadowIncludingRoot(node)) {
      throw new DOMEx("Failed to execute 'moveBefore': Nodes are not in the same tree.", "HierarchyRequestError");
    }
    if (child !== null && child !== undefined) {
      if (typeof child !== "object" || !isGenuineNode(child)) {
        throw new TypeError("Failed to execute 'moveBefore': parameter 2 is not of type 'Node'.");
      }
      if (child.parentNode !== this) {
        throw new DOMEx("Failed to execute 'moveBefore': The node before which the new node is to be inserted is not a child of this node.", "NotFoundError");
      }
      if (node === child) return;
    }

    const oldParent = node.parentNode;
    const oldSibIdx = oldParent ? ensureChildren(oldParent).indexOf(node) : -1;
    if (oldParent && oldSibIdx !== -1) {
      oldParent._children.splice(oldSibIdx, 1);
      if (nativeElementRemoveChild && typeof nativeElementRemoveChild === "function") {
        try { nativeElementRemoveChild.call(oldParent, node); } catch (e) {}
      }
      if (typeof oldParent._nid === "number" && oldParent._nid > 0 && oldParent.nodeType === DOCUMENT_FRAGMENT_NODE && typeof global.__capsurf_node_remove_child === "function") {
        try { global.__capsurf_node_remove_child(oldParent._nid, node); } catch (e) {}
      }
    }

    const children = ensureChildren(this);
    if (child !== null && child !== undefined) {
      const idx = children.indexOf(child);
      if (idx !== -1) children.splice(idx, 0, node);
      else children.push(node);
      if (nativeElementInsertBefore && typeof nativeElementInsertBefore === "function") {
        try { nativeElementInsertBefore.call(this, node, child); } catch (e) {}
      }
      if (typeof this._nid === "number" && this._nid > 0 && this.nodeType === DOCUMENT_FRAGMENT_NODE && typeof global.__capsurf_node_insert_before === "function") {
        try { global.__capsurf_node_insert_before(this._nid, node, child); } catch (e) {}
      }
    } else {
      children.push(node);
      if (nativeElementAppendChild && typeof nativeElementAppendChild === "function") {
        try { nativeElementAppendChild.call(this, node); } catch (e) {}
      }
      if (typeof this._nid === "number" && this._nid > 0 && this.nodeType === DOCUMENT_FRAGMENT_NODE && typeof global.__capsurf_node_append_child === "function") {
        try { global.__capsurf_node_append_child(this._nid, node); } catch (e) {}
      }
    }
    if (oldParent && oldParent._children && this === oldParent) {
      // already removed from oldParent._children above; native side handled
    }
    try { node.parentNode = this; } catch (e) {}
    const newDoc = this.nodeType === DOCUMENT_NODE ? this : (this.ownerDocument || null);
    if (newDoc) { try { newDoc._capsurfIdDirty = true; } catch (e) {} }

    if (typeof notifyMutation === "function") {
      if (oldParent) {
        const recR = new MutationRecord("childList", oldParent);
        recR.removedNodes = [node];
        notifyMutation(recR);
      }
      const recA = new MutationRecord("childList", this);
      recA.addedNodes = [node];
      notifyMutation(recA);
    }

    if (this.isConnected && typeof node.connectedMoveCallback === "function") {
      try { node.connectedMoveCallback(); } catch (e) {}
    }
  }, "moveBefore");
  Element.prototype.moveBefore = moveBeforeFn;
  global.__capsurf_moveBeforeFn = moveBeforeFn;
  if (typeof Document !== "undefined" && Document.prototype) {
    Document.prototype.moveBefore = moveBeforeFn;
  }
  if (typeof DocumentFragment !== "undefined" && DocumentFragment.prototype) {
    DocumentFragment.prototype.moveBefore = moveBeforeFn;
  }
  for (const _ep of ["childNodes", "firstChild", "lastChild", "nextSibling", "previousSibling"]) {
    const _desc = Object.getOwnPropertyDescriptor(Node.prototype, _ep);
    if (_desc) Object.defineProperty(Element.prototype, _ep, _desc);
  }

  const RAW_TEXT_TAGS = {
    script: 1, style: 1, xmp: 1, iframe: 1, noembed: 1, noframes: 1, plaintext: 1, noscript: 1,
  };
  function escapeHtmlText(str) {
    return String(str)
      .replace(/&/g, "&amp;")
      .replace(/\u00a0/g, "&nbsp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;");
  }
  function hasJsOnlyChild(kids) {
    for (let i = 0; i < kids.length; i++) {
      const k = kids[i];
      if (!k) continue;
      if (k._nid === undefined) return true;
    }
    return false;
  }

  const nativeElementInnerHtmlDesc = Object.getOwnPropertyDescriptor(Element.prototype, "innerHTML");
  if (nativeElementInnerHtmlDesc && nativeElementInnerHtmlDesc.set) {
    const origGetInnerHtml = nativeElementInnerHtmlDesc.get;
    const origSetInnerHtml = nativeElementInnerHtmlDesc.set;
    if (typeof global !== "undefined") global.__capsurf_raw_setInnerHTML = origSetInnerHtml;
    if (typeof globalThis !== "undefined") globalThis.__capsurf_raw_setInnerHTML = origSetInnerHtml;
    Object.defineProperty(Element.prototype, "innerHTML", {
      get: function () {
        if (Array.isArray(this._children)) {
          const parentTag = String(this.localName || this.tagName || "").toLowerCase();
          const isRawText = Boolean(RAW_TEXT_TAGS[parentTag]);
          let html = "";
          for (let i = 0; i < this._children.length; i++) {
            const k = this._children[i];
            if (!k) continue;
            if (k.nodeType === 1) {
              html += k.outerHTML || "";
            } else if (k.nodeType === 3) {
              const txt = k.data !== undefined ? String(k.data) : String(k.textContent || "");
              html += isRawText ? txt : escapeHtmlText(txt);
            } else if (k.nodeType === 8) {
              html += "<!--" + (k.data !== undefined ? String(k.data) : "") + "-->";
            }
          }
          return html;
        }
        return origGetInnerHtml ? origGetInnerHtml.call(this) : "";
      },
      set: function (v) {
        let oldKids = [];
        try {
          oldKids = Array.from(this.childNodes || this._children || []);
        } catch (e) {}
        for (let i = 0; i < oldKids.length; i++) {
          if (oldKids[i]) {
            try { oldKids[i].parentNode = null; } catch (e) {}
          }
        }
        this._children = null;
        origSetInnerHtml.call(this, v);
        try {
          const ctxTag = String(this.localName || this.tagName || "").toLowerCase();
          const TABLE_CTX = { table: 1, thead: 1, tbody: 1, tfoot: 1, tr: 1, colgroup: 1, caption: 1, template: 1 };
          const ORPHAN_TABLE = { tr: 1, td: 1, th: 1, tbody: 1, thead: 1, tfoot: 1, caption: 1, colgroup: 1, col: 1 };
          const ns = this.namespaceURI;
          if ((!ns || ns === "http://www.w3.org/1999/xhtml") && !TABLE_CTX[ctxTag] && !this._skipTableUnwrap) {
            let changed = true;
            while (changed) {
              changed = false;
              const kids = Array.from(this.childNodes || this._children || []);
              for (let ki = 0; ki < kids.length; ki++) {
                const k = kids[ki];
                if (k && k.nodeType === 1) {
                  const kt = String(k.localName || k.tagName || "").toLowerCase();
                  if (ORPHAN_TABLE[kt]) {
                    const gkids = Array.from(k.childNodes || k._children || []);
                    for (let gi = 0; gi < gkids.length; gi++) {
                      this.insertBefore(gkids[gi], k);
                    }
                    this.removeChild(k);
                    changed = true;
                  }
                }
              }
            }
          }
        } catch (e) {}
        let newKids = [];
        try {
          newKids = Array.from(this.childNodes || this._children || []);
        } catch (e) {}
        if (typeof globalThis !== "undefined" && typeof globalThis.upgradeCustomElement === "function" && typeof globalThis.__capsurf_getCustomElementDefinition === "function") {
          const getDef = globalThis.__capsurf_getCustomElementDefinition;
          const upgradeFn = globalThis.upgradeCustomElement;
          const upgradeTree = function (el) {
            if (!el || el.nodeType !== 1) return;
            const ns = el._namespaceURI !== undefined ? el._namespaceURI : el.namespaceURI;
            if (!ns || ns === "http://www.w3.org/1999/xhtml") {
              const tag = String(el.localName || el.tagName || "").toLowerCase();
              const isAttr = el.getAttribute ? el.getAttribute("is") : null;
              let def = null;
              if (isAttr !== null && isAttr !== undefined) {
                const extDef = getDef(String(isAttr));
                if (extDef && extDef.extendsTag === tag) def = extDef;
              } else {
                const autoDef = getDef(tag);
                if (autoDef && !autoDef.extendsTag) def = autoDef;
              }
              if (def && def.constructor && !def.constructor.prototype.isPrototypeOf(el)) {
                upgradeFn(el, def.constructor, { tagName: def.name || tag, fireConnectedCallback: Boolean(el.isConnected) });
              }
            }
            const ks = el.childNodes || el._children;
            if (ks && ks.length) {
              for (let j = 0; j < ks.length; j++) upgradeTree(ks[j]);
            }
          };
          for (let i = 0; i < newKids.length; i++) upgradeTree(newKids[i]);
        }
        if ((oldKids.length > 0 || newKids.length > 0) && typeof notifyMutation === "function") {
          const rec = new MutationRecord("childList", this);
          rec.removedNodes = oldKids;
          rec.addedNodes = newKids;
          notifyMutation(rec);
        }
        if (this._shadowRoot && typeof global.__capsurf_scheduleSlotChange === "function") {
          global.__capsurf_scheduleSlotChange(this);
        }
      },
      configurable: true,
      enumerable: nativeElementInnerHtmlDesc.enumerable,
    });
  }
}

Node.prototype.lookupNamespaceURI = function (prefix) {
  let p = prefix === "" ? null : prefix;
  if (p === "xml") return "http://www.w3.org/XML/1998/namespace";
  if (p === "xmlns") return "http://www.w3.org/2000/xmlns/";
  if (this.nodeType === 1) { // ELEMENT_NODE
    let n = this;
    while (n && n.nodeType === 1) {
      if (n.prefix === p && n.namespaceURI) return n.namespaceURI;
      const attrs = n.attributes;
      if (attrs) {
        const len = attrs.length || 0;
        for (let i = 0; i < len; i++) {
          const a = attrs[i];
          if (!a) continue;
          const aPrefix = a.prefix || null;
          const aLocal = a.localName || a.name;
          if (p === null) {
            if (a.name === "xmlns" || (aPrefix === null && aLocal === "xmlns")) {
              return a.value || null;
            }
          } else {
            if (aPrefix === "xmlns" && aLocal === p) {
              return a.value || null;
            }
            if (a.name === "xmlns:" + p) {
              return a.value || null;
            }
          }
        }
      }
      n = n.parentElement || n.parentNode;
    }
    return null;
  }
  if (this.nodeType === 9) { // DOCUMENT_NODE
    return this.documentElement ? this.documentElement.lookupNamespaceURI(p) : null;
  }
  if (this.nodeType === 10 || this.nodeType === 11) { // DOCUMENT_TYPE_NODE || DOCUMENT_FRAGMENT_NODE
    return null;
  }
  if (this.nodeType === 2) { // ATTRIBUTE_NODE
    return this.ownerElement ? this.ownerElement.lookupNamespaceURI(p) : null;
  }
  const parent = this.parentElement || this.parentNode;
  return parent ? parent.lookupNamespaceURI(p) : null;
};

Node.prototype.lookupPrefix = function (namespace) {
  if (namespace === null || namespace === undefined || namespace === "") return null;
  const ns = String(namespace);
  if (this.nodeType === 1) { // ELEMENT_NODE
    let n = this;
    while (n && n.nodeType === 1) {
      if (n.namespaceURI === ns && n.prefix !== null && n.prefix !== undefined && n.prefix !== "") {
        if (typeof n.lookupNamespaceURI !== "function" || n.lookupNamespaceURI(n.prefix) === ns) {
          return n.prefix;
        }
      }
      const attrs = n.attributes;
      if (attrs) {
        const len = attrs.length || 0;
        for (let i = 0; i < len; i++) {
          const a = attrs[i];
          if (!a) continue;
          const aPrefix = a.prefix || null;
          const aLocal = a.localName || a.name;
          let candPrefix = null;
          if (a.namespaceURI === "http://www.w3.org/2000/xmlns/") {
            if (aPrefix === "xmlns" && aLocal) {
              candPrefix = aLocal;
            } else if (typeof a.name === "string" && a.name.indexOf("xmlns:") === 0) {
              candPrefix = a.name.slice(6);
            }
          }
          if (candPrefix && a.value === ns) {
            if (typeof this.lookupNamespaceURI !== "function" || this.lookupNamespaceURI(candPrefix) === ns) {
              return candPrefix;
            }
          }
        }
      }
      n = n.parentElement || n.parentNode;
    }
    return null;
  }
  if (this.nodeType === 9) { // DOCUMENT_NODE
    return this.documentElement ? this.documentElement.lookupPrefix(ns) : null;
  }
  if (this.nodeType === 10 || this.nodeType === 11) { // DOCUMENT_TYPE_NODE || DOCUMENT_FRAGMENT_NODE
    return null;
  }
  if (this.nodeType === 2) { // ATTRIBUTE_NODE
    return this.ownerElement ? this.ownerElement.lookupPrefix(ns) : null;
  }
  const parent = this.parentElement || this.parentNode;
  return parent ? parent.lookupPrefix(ns) : null;
};

function computeDocumentBaseURI(doc) {
  if (!doc) return "about:blank";
  const docUrl = doc.URL || doc.documentURI || (doc.location && doc.location.href) || (typeof location !== "undefined" && location && location.href) || "about:blank";
  let baseEl = null;
  if (typeof findInSubtree === "function") {
    baseEl = findInSubtree(doc, function (el) {
      return el && el.nodeType === 1 &&
        String(el.localName || el.tagName || "").toLowerCase() === "base" &&
        typeof el.hasAttribute === "function" && el.hasAttribute("href");
    }, true);
  } else if (typeof doc.querySelector === "function") {
    baseEl = doc.querySelector("base[href]");
  }
  if (baseEl && typeof baseEl.getAttribute === "function") {
    const bHref = baseEl.getAttribute("href");
    if (bHref !== null) {
      try {
        return new URL(String(bHref).trim(), docUrl).href;
      } catch (e) {}
    }
  }
  return String(docUrl);
}
if (typeof globalThis !== "undefined") globalThis.__capsurf_computeDocumentBaseURI = computeDocumentBaseURI;

Object.defineProperty(Node.prototype, "baseURI", {
  get: function () {
    const doc = this.nodeType === 9 ? this : (this.ownerDocument || (typeof document !== "undefined" ? document : null));
    return computeDocumentBaseURI(doc);
  },
  configurable: true,
});

Node.prototype.isSameNode = function (otherNode) {
  if (arguments.length === 0) {
    throw new TypeError("Failed to execute 'isSameNode' on 'Node': 1 argument required, but only 0 present.");
  }
  return this === otherNode;
};

Node.prototype.isEqualNode = function (otherNode) {
  if (arguments.length < 1) {
    throw new TypeError("Failed to execute 'isEqualNode' on 'Node': 1 argument required, but only 0 present.");
  }
  if (otherNode === null || otherNode === undefined) return false;
  if (typeof otherNode !== "object" || typeof otherNode.nodeType !== "number") {
    throw new TypeError("Failed to execute 'isEqualNode' on 'Node': parameter 1 is not of type 'Node'.");
  }
  if (this === otherNode) return true;
  if (this.nodeType !== otherNode.nodeType) return false;
  const nt = this.nodeType;
  if (nt === 1) {
    if ((this.namespaceURI || null) !== (otherNode.namespaceURI || null)) return false;
    if ((this.prefix || null) !== (otherNode.prefix || null)) return false;
    if ((this.localName || this.tagName || "") !== (otherNode.localName || otherNode.tagName || "")) return false;
    const a1 = this.attributes;
    const a2 = otherNode.attributes;
    const len1 = a1 ? a1.length : 0;
    const len2 = a2 ? a2.length : 0;
    if (len1 !== len2) return false;
    for (let i = 0; i < len1; i++) {
      const attr1 = a1[i];
      let matched = false;
      for (let j = 0; j < len2; j++) {
        const attr2 = a2[j];
        if ((attr1.namespaceURI || null) === (attr2.namespaceURI || null) &&
            (attr1.localName || attr1.name) === (attr2.localName || attr2.name) &&
            String(attr1.value) === String(attr2.value)) {
          matched = true;
          break;
        }
      }
      if (!matched) return false;
    }
  } else if (nt === 10) {
    if ((this.name || "") !== (otherNode.name || "")) return false;
    if ((this.publicId || "") !== (otherNode.publicId || "")) return false;
    if ((this.systemId || "") !== (otherNode.systemId || "")) return false;
  } else if (nt === 7) {
    if ((this.target || "") !== (otherNode.target || "")) return false;
    if ((this.data || "") !== (otherNode.data || "")) return false;
  } else if (nt === 3 || nt === 4 || nt === 8) {
    const d1 = this.data !== undefined ? String(this.data) : String(this.textContent || "");
    const d2 = otherNode.data !== undefined ? String(otherNode.data) : String(otherNode.textContent || "");
    if (d1 !== d2) return false;
  } else if (nt === 2) {
    if ((this.namespaceURI || null) !== (otherNode.namespaceURI || null)) return false;
    if ((this.localName || this.name) !== (otherNode.localName || otherNode.name)) return false;
    if (String(this.value) !== String(otherNode.value)) return false;
  }
  const c1 = [];
  for (let c = this.firstChild; c; c = c.nextSibling) c1.push(c);
  if (c1.length === 0 && this._children && this._children.length > 0) {
    for (let i = 0; i < this._children.length; i++) c1.push(this._children[i]);
  } else if (c1.length === 0 && this.childNodes && this.childNodes.length > 0) {
    for (let i = 0; i < this.childNodes.length; i++) c1.push(this.childNodes[i]);
  }
  const c2 = [];
  for (let c = otherNode.firstChild; c; c = c.nextSibling) c2.push(c);
  if (c2.length === 0 && otherNode._children && otherNode._children.length > 0) {
    for (let i = 0; i < otherNode._children.length; i++) c2.push(otherNode._children[i]);
  } else if (c2.length === 0 && otherNode.childNodes && otherNode.childNodes.length > 0) {
    for (let i = 0; i < otherNode.childNodes.length; i++) c2.push(otherNode.childNodes[i]);
  }
  if (c1.length !== c2.length) return false;
  for (let i = 0; i < c1.length; i++) {
    if (!c1[i].isEqualNode(c2[i])) return false;
  }
  return true;
};
markNative(Node.prototype.isEqualNode, "isEqualNode");

markNative(Node, "Node");
markNative(Node.prototype.isSameNode, "isSameNode");
markNative(Node.prototype.contains, "contains");
markNative(Node.prototype.compareDocumentPosition, "compareDocumentPosition");
markNative(Node.prototype.cloneNode, "cloneNode");
markNative(Node.prototype.appendChild, "appendChild");
markNative(Node.prototype.insertBefore, "insertBefore");
markNative(Node.prototype.removeChild, "removeChild");
markNative(Node.prototype.replaceChild, "replaceChild");
markNative(Node.prototype.hasChildNodes, "hasChildNodes");
markNative(Node.prototype.lookupNamespaceURI, "lookupNamespaceURI");
markNative(Node.prototype.lookupPrefix, "lookupPrefix");
if (typeof Attr !== "undefined" && Attr.prototype && !Attr.prototype.lookupNamespaceURI) {
  Attr.prototype.lookupNamespaceURI = Node.prototype.lookupNamespaceURI;
}
if (typeof Attr !== "undefined" && Attr.prototype && !Attr.prototype.lookupPrefix) {
  Attr.prototype.lookupPrefix = Node.prototype.lookupPrefix;
}
markNative(Node.prototype.getRootNode, "getRootNode");
markNative(Node.prototype.normalize, "normalize");

function characterDataReplaceData(node, offset, count, data) {
  let cur = "";
  if (node._data !== undefined) {
    cur = String(node._data);
  } else if (node._nid !== undefined && nativeElementGetTextContent) {
    try {
      cur = String(nativeElementGetTextContent.call(node) || "");
    } catch (e) {
      cur = "";
    }
  }
  const len = cur.length;
  const off = offset >>> 0;
  if (off > len) {
    if (typeof DOMException !== "undefined") {
      throw new DOMException("The offset " + off + " is larger than the node's length (" + len + ").", "IndexSizeError");
    }
    const err = new Error("IndexSizeError");
    err.name = "IndexSizeError";
    throw err;
  }
  let cnt = count >>> 0;
  if (off + cnt > len) {
    cnt = len - off;
  }
  const str = String(data !== undefined ? data : "");
  const oldVal = cur;
  node._data = cur.slice(0, off) + str + cur.slice(off + cnt);
  if (node._nid !== undefined && nativeElementSetTextContent) {
    try {
      nativeElementSetTextContent.call(node, node._data);
    } catch (e) {}
  }
  if (node.parentNode && node.parentNode.nodeType === 1 && String(node.parentNode.localName || node.parentNode.tagName || "").toLowerCase() === "style") {
    node.parentNode._sheet = null;
    if (nativeElementSetTextContent) {
      try { nativeElementSetTextContent.call(node.parentNode, node.parentNode.textContent); } catch (e) {}
    }
  }
  if (typeof global !== "undefined" && typeof global.__capsurf_rangeOnReplaceData === "function") {
    global.__capsurf_rangeOnReplaceData(node, off, cnt, str.length);
  }
  if (typeof notifyMutation === "function") {
    const rec = new MutationRecord("characterData", node);
    rec.oldValue = oldVal;
    notifyMutation(rec);
  }
}

function installCharacterDataMethods(proto) {
  Object.defineProperty(proto, "data", {
    get: function () {
      if (this._data !== undefined) return this._data;
      if (nativeElementGetTextContent && (this.nodeType === 3 || this.nodeType === 8)) {
        try {
          const t = nativeElementGetTextContent.call(this);
          if (t !== undefined && t !== null) return String(t);
        } catch (e) {}
      }
      return "";
    },
    set: function (v) {
      const str = v === null ? "" : String(v);
      const curLen = this.data.length;
      characterDataReplaceData(this, 0, curLen, str);
    },
    configurable: true,
  });
  Object.defineProperty(proto, "textContent", {
    get: function () { return this.data; },
    set: function (v) { this.data = v === null || v === undefined ? "" : String(v); },
    configurable: true,
  });
  Object.defineProperty(proto, "nodeValue", {
    get: function () { return this.data; },
    set: function (v) { this.data = v === null || v === undefined ? "" : String(v); },
    configurable: true,
  });
  Object.defineProperty(proto, "length", {
    get: function () { return this.data.length; },
    configurable: true,
  });
  Object.defineProperty(proto, "previousElementSibling", {
    get: function () {
      let sib = this.previousSibling;
      while (sib) {
        if (sib.nodeType === 1) return sib;
        sib = sib.previousSibling;
      }
      return null;
    },
    configurable: true,
  });
  Object.defineProperty(proto, "nextElementSibling", {
    get: function () {
      let sib = this.nextSibling;
      while (sib) {
        if (sib.nodeType === 1) return sib;
        sib = sib.nextSibling;
      }
      return null;
    },
    configurable: true,
  });
  proto.substringData = function (offset, count) {
    const cur = this.data;
    const len = cur.length;
    const off = offset >>> 0;
    if (off > len) {
      if (typeof DOMException !== "undefined") {
        throw new DOMException("IndexSizeError", "IndexSizeError");
      }
      const err = new Error("IndexSizeError");
      err.name = "IndexSizeError";
      throw err;
    }
    const cnt = count >>> 0;
    if (off + cnt > len) return cur.slice(off);
    return cur.slice(off, off + cnt);
  };
  proto.appendData = function (data) {
    characterDataReplaceData(this, this.length, 0, data);
  };
  proto.insertData = function (offset, data) {
    characterDataReplaceData(this, offset, 0, data);
  };
  proto.deleteData = function (offset, count) {
    characterDataReplaceData(this, offset, count, "");
  };
  proto.replaceData = function (offset, count, data) {
    characterDataReplaceData(this, offset, count, data);
  };
  markNative(proto.substringData, "substringData");
  markNative(proto.appendData, "appendData");
  markNative(proto.insertData, "insertData");
  markNative(proto.deleteData, "deleteData");
  markNative(proto.replaceData, "replaceData");
}

function CharacterData() {
  Node.call(this);
}
CharacterData.prototype = Object.create(Node.prototype);
CharacterData.prototype.constructor = CharacterData;
installCharacterDataMethods(CharacterData.prototype);
markNative(CharacterData, "CharacterData");

function Text(data) {
  Node.call(this);
  this._data = String(data !== undefined ? data : "");
  this.nodeType = TEXT_NODE;
  this.nodeName = "#text";
  this.parentNode = null;
}
Text.prototype = Object.create(CharacterData.prototype);
Text.prototype.constructor = Text;
Object.defineProperty(Text.prototype, "nodeType", { value: TEXT_NODE, writable: true, configurable: true });
Object.defineProperty(Text.prototype, "nodeName", { value: "#text", writable: true, configurable: true });
Object.defineProperty(Text.prototype, "tagName", { get: function () { return undefined; }, configurable: true });
const _nodeTagNameDesc = Object.getOwnPropertyDescriptor(Node.prototype, "tagName");
if (_nodeTagNameDesc && _nodeTagNameDesc.get) {
  const _origNodeTagNameGet = _nodeTagNameDesc.get;
  Object.defineProperty(Node.prototype, "tagName", {
    get: function () {
      if (this.nodeType !== 1) return undefined;
      return _origNodeTagNameGet.call(this);
    },
    configurable: true,
  });
} else {
  Object.defineProperty(Node.prototype, "tagName", {
    get: function () { return undefined; },
    configurable: true,
  });
}
const _wholeTextDesc = {
  get: function () {
    if (this.nodeType !== TEXT_NODE && this.nodeType !== CDATA_SECTION_NODE) return undefined;
    let cur = this;
    while (cur.previousSibling && (cur.previousSibling.nodeType === TEXT_NODE || cur.previousSibling.nodeType === CDATA_SECTION_NODE)) {
      cur = cur.previousSibling;
    }
    let out = "";
    while (cur && (cur.nodeType === TEXT_NODE || cur.nodeType === CDATA_SECTION_NODE)) {
      out += (cur.data !== undefined ? cur.data : (cur.nodeValue || cur.textContent || ""));
      cur = cur.nextSibling;
    }
    return out;
  },
  configurable: true,
};
Object.defineProperty(Text.prototype, "wholeText", _wholeTextDesc);
if (!Object.getOwnPropertyDescriptor(Node.prototype, "wholeText")) {
  Object.defineProperty(Node.prototype, "wholeText", _wholeTextDesc);
}

Text.prototype.splitText = function (offset) {
  const cur = this.data;
  const len = cur.length;
  const off = offset >>> 0;
  if (off > len) {
    if (typeof DOMException !== "undefined") {
      throw new DOMException("The offset " + off + " is larger than the Text node's length (" + len + ").", "IndexSizeError");
    }
    const err = new Error("IndexSizeError");
    err.name = "IndexSizeError";
    throw err;
  }
  const count = len - off;
  const newData = this.substringData(off, count);
  const newNode = new Text(newData);
  newNode.ownerDocument = this.ownerDocument;
  const parent = this.parentNode;
  if (parent !== null && parent !== undefined) {
    const kids = ensureChildren(parent);
    const nodeIndex = kids.indexOf(this);
    const nextSib = (nodeIndex >= 0 && nodeIndex + 1 < kids.length) ? kids[nodeIndex + 1] : null;
    newNode._skipRangeInsertNotify = true;
    try {
      if (nextSib) {
        parent.insertBefore(newNode, nextSib);
      } else {
        parent.appendChild(newNode);
      }
    } finally {
      newNode._skipRangeInsertNotify = false;
    }
    if (typeof global !== "undefined" && typeof global.__capsurf_rangeOnSplitText === "function") {
      global.__capsurf_rangeOnSplitText(this, newNode, off, parent, nodeIndex);
    }
  }
  this.replaceData(off, count, "");
  return newNode;
};
markNative(Text.prototype.splitText, "splitText");
if (!Object.getOwnPropertyDescriptor(Node.prototype, "splitText")) {
  Node.prototype.splitText = Text.prototype.splitText;
}
markNative(Text, "Text");

function Comment(data) {
  Node.call(this);
  this._data = String(data !== undefined ? data : "");
  this.nodeType = COMMENT_NODE;
  this.nodeName = "#comment";
}
Comment.prototype = Object.create(CharacterData.prototype);
Comment.prototype.constructor = Comment;
Object.defineProperty(Comment.prototype, "nodeType", { value: COMMENT_NODE, writable: true, configurable: true });
Object.defineProperty(Comment.prototype, "nodeName", { value: "#comment", writable: true, configurable: true });
Object.defineProperty(Comment.prototype, "tagName", { get: function () { return undefined; }, configurable: true });
markNative(Comment, "Comment");

function CDATASection(data) {
  Text.call(this, data);
  this.nodeType = CDATA_SECTION_NODE;
  this.nodeName = "#cdata-section";
}
CDATASection.prototype = Object.create(Text.prototype);
CDATASection.prototype.constructor = CDATASection;
Object.defineProperty(CDATASection.prototype, "nodeType", { value: CDATA_SECTION_NODE, writable: true, configurable: true });
Object.defineProperty(CDATASection.prototype, "nodeName", { value: "#cdata-section", writable: true, configurable: true });
markNative(CDATASection, "CDATASection");

function ProcessingInstruction(target, data) {
  CharacterData.call(this);
  this._data = String(data !== undefined ? data : "");
  this._target = String(target !== undefined ? target : "");
  this.nodeType = PROCESSING_INSTRUCTION_NODE;
}
ProcessingInstruction.prototype = Object.create(CharacterData.prototype);
ProcessingInstruction.prototype.constructor = ProcessingInstruction;
Object.defineProperty(ProcessingInstruction.prototype, "nodeType", { value: PROCESSING_INSTRUCTION_NODE, writable: true, configurable: true });
Object.defineProperty(ProcessingInstruction.prototype, "nodeName", {
  get: function () { return this._target || ""; },
  configurable: true,
});
Object.defineProperty(ProcessingInstruction.prototype, "target", {
  get: function () { return this._target || ""; },
  configurable: true,
});
markNative(ProcessingInstruction, "ProcessingInstruction");

function DocumentFragment() {
  Node.call(this);
  this.nodeType = DOCUMENT_FRAGMENT_NODE;
  this.nodeName = "#document-fragment";
  this._children = [];
}
DocumentFragment.prototype = Object.create(Node.prototype);
DocumentFragment.prototype.constructor = DocumentFragment;
if (typeof global.__capsurf_moveBeforeFn === "function") {
  DocumentFragment.prototype.moveBefore = global.__capsurf_moveBeforeFn;
}
Object.defineProperty(DocumentFragment.prototype, Symbol.toStringTag, {
  value: "DocumentFragment",
  configurable: true,
});
Object.defineProperty(DocumentFragment.prototype, "tagName", {
  get: function () { return undefined; },
  configurable: true,
});
DocumentFragment.prototype.appendChild = function (child) {
  if (!child || typeof child !== "object") return child;
  if (child._ownerFragment && child._ownerFragment !== this && typeof child._ownerFragment.removeChild === "function") {
    try { child._ownerFragment.removeChild(child); } catch (e) {}
  }
  if (child.parentNode && child.parentNode !== this && typeof child.parentNode.removeChild === "function") {
    try { child.parentNode.removeChild(child); } catch (e) {}
  }
  _setHidden(child, "_ownerFragment", this);
  const res = Node.prototype.appendChild.call(this, child);
  _setHidden(child, "_ownerFragment", this);
  return res;
};
DocumentFragment.prototype.insertBefore = function (newChild, refChild) {
  if (!newChild || typeof newChild !== "object") return newChild;
  if (newChild._ownerFragment && newChild._ownerFragment !== this && typeof newChild._ownerFragment.removeChild === "function") {
    try { newChild._ownerFragment.removeChild(newChild); } catch (e) {}
  }
  if (newChild.parentNode && newChild.parentNode !== this && typeof newChild.parentNode.removeChild === "function") {
    try { newChild.parentNode.removeChild(newChild); } catch (e) {}
  }
  newChild._ownerFragment = this;
  const res = Node.prototype.insertBefore.call(this, newChild, refChild);
  newChild._ownerFragment = this;
  return res;
};
DocumentFragment.prototype.removeChild = function (child) {
  if (!child) return child;
  const kids = this._children || [];
  const idx = kids.indexOf(child);
  if (idx === -1 && child._ownerFragment !== this && child.parentNode !== this) {
    if (typeof DOMException !== "undefined") {
      throw new DOMException("NotFoundError: Node was not found", "NotFoundError");
    }
    const err = new Error("NotFoundError: Node was not found");
    err.name = "NotFoundError";
    throw err;
  }
  if (idx >= 0) kids.splice(idx, 1);
  if (child._ownerFragment === this) child._ownerFragment = null;
  child.parentNode = null;
  if (typeof this._nid === "number" && this._nid > 0 && typeof global.__capsurf_node_remove_child === "function") {
    try {
      global.__capsurf_node_remove_child(this._nid, child);
    } catch (e) {}
  }
  return child;
};
DocumentFragment.prototype.replaceChild = function (newChild, oldChild) {
  if (!newChild || typeof newChild !== "object") return newChild;
  if (!oldChild || typeof oldChild !== "object") return oldChild;
  if (newChild._ownerFragment && typeof newChild._ownerFragment.removeChild === "function") {
    try { newChild._ownerFragment.removeChild(newChild); } catch (e) {}
  }
  if (newChild.parentNode && typeof newChild.parentNode.removeChild === "function") {
    try { newChild.parentNode.removeChild(newChild); } catch (e) {}
  }
  newChild._ownerFragment = this;
  const res = Node.prototype.replaceChild.call(this, newChild, oldChild);
  newChild._ownerFragment = this;
  return res;
};
Object.defineProperty(DocumentFragment.prototype, "childNodes", {
  get: function () {
    return new NodeList(this._children);
  },
  configurable: true,
});
Object.defineProperty(DocumentFragment.prototype, "children", {
  get: function () {
    const list = [];
    const kids = this._children || [];
    for (let i = 0; i < kids.length; i++) {
      if (kids[i] && kids[i].nodeType === 1) list.push(kids[i]);
    }
    return (typeof HTMLCollection !== "undefined") ? new HTMLCollection(list) : list;
  },
  configurable: true,
});

Object.defineProperty(DocumentFragment.prototype, "firstElementChild", {
  get: function () {
    const kids = this._children || [];
    for (let i = 0; i < kids.length; i++) {
      if (kids[i] && kids[i].nodeType === 1) return kids[i];
    }
    return null;
  },
  configurable: true,
});

Object.defineProperty(DocumentFragment.prototype, "lastElementChild", {
  get: function () {
    const kids = this._children || [];
    for (let i = kids.length - 1; i >= 0; i--) {
      if (kids[i] && kids[i].nodeType === 1) return kids[i];
    }
    return null;
  },
  configurable: true,
});

Object.defineProperty(DocumentFragment.prototype, "childElementCount", {
  get: function () {
    let count = 0;
    const kids = this._children || [];
    for (let i = 0; i < kids.length; i++) {
      if (kids[i] && kids[i].nodeType === 1) count++;
    }
    return count;
  },
  configurable: true,
});

DocumentFragment.prototype.append = function (...nodes) {
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
markNative(DocumentFragment.prototype.append, "append");

DocumentFragment.prototype.prepend = function (...nodes) {
  const first = this.firstChild;
  for (const n of nodes) {
    let node;
    if (n && typeof n === "object" && typeof n.nodeType === "number") {
      node = n;
    } else if (typeof document !== "undefined" && document.createTextNode) {
      node = document.createTextNode(String(n));
    } else {
      node = new Text(String(n));
    }
    this.insertBefore(node, first);
  }
};
markNative(DocumentFragment.prototype.prepend, "prepend");

function _validateDocumentChildren(nodes) {
  let elemCount = 0;
  let doctypeCount = 0;
  for (let i = 0; i < nodes.length; i++) {
    const c = nodes[i];
    if (!c) continue;
    if (c.nodeType === 11) {
      const kids = Array.from(c.childNodes || c._children || []);
      for (let j = 0; j < kids.length; j++) {
        const k = kids[j];
        if (k.nodeType === 3) throw new DOMException("Nodes of type '#text' may not be inserted inside nodes of type '#document'.", "HierarchyRequestError");
        if (k.nodeType === 1) {
          elemCount++;
          if (elemCount > 1) throw new DOMException("Only one element on document allowed.", "HierarchyRequestError");
        } else if (k.nodeType === 10) {
          doctypeCount++;
          if (doctypeCount > 1 || elemCount > 0) throw new DOMException("Invalid doctype position.", "HierarchyRequestError");
        }
      }
    } else if (c.nodeType === 3) {
      throw new DOMException("Nodes of type '#text' may not be inserted inside nodes of type '#document'.", "HierarchyRequestError");
    } else if (c.nodeType === 1) {
      elemCount++;
      if (elemCount > 1) throw new DOMException("Only one element on document allowed.", "HierarchyRequestError");
    } else if (c.nodeType === 10) {
      doctypeCount++;
      if (doctypeCount > 1 || elemCount > 0) throw new DOMException("Invalid doctype position.", "HierarchyRequestError");
    }
  }
}

function replaceChildrenImpl(parent, nodes) {
  const existing = Array.from(parent.childNodes || []);
  const converted = [];
  for (let i = 0; i < nodes.length; i++) {
    const n = nodes[i];
    if (n && typeof n === "object" && typeof n.nodeType === "number") {
      if (n.nodeType === 11) {
        const fkids = Array.from(n.childNodes || n._children || []);
        for (let j = 0; j < fkids.length; j++) converted.push(fkids[j]);
      } else {
        converted.push(n);
      }
    } else if (typeof document !== "undefined" && document.createTextNode) {
      converted.push(document.createTextNode(String(n)));
    } else {
      converted.push(new Text(String(n)));
    }
  }
  if (parent.nodeType === 9) {
    _validateDocumentChildren(converted);
  }
  const DOMEx = typeof DOMException !== "undefined" ? DOMException : Error;
  for (let i = 0; i < converted.length; i++) {
    const n = converted[i];
    if (n.nodeType === 10 && parent.nodeType !== 9) {
      throw new DOMEx("DocumentType may not be inserted inside an Element or DocumentFragment.", "HierarchyRequestError");
    }
    if (n.nodeType === 9) {
      throw new DOMEx("Document may not be inserted.", "HierarchyRequestError");
    }
    let anc = parent;
    while (anc) {
      if (anc === n) {
        throw new DOMEx("The new child element contains the parent.", "HierarchyRequestError");
      }
      anc = anc.parentNode;
    }
  }
  const remaining = Array.from(parent.childNodes || []);
  const run = function () {
    let guard = 0;
    while (parent.firstChild && guard++ < 10000) {
      const fc = parent.firstChild;
      parent.removeChild(fc);
      if (parent.firstChild === fc) break;
    }
    for (let i = 0; i < converted.length; i++) {
      parent.appendChild(converted[i]);
    }
  };
  if (typeof suppressChildListMutations === "function") {
    suppressChildListMutations(parent, run);
  } else {
    run();
  }
  const added = Array.from(parent.childNodes || []);
  if ((added.length > 0 || remaining.length > 0) && typeof notifyMutation === "function" && typeof MutationRecord === "function") {
    const rec = new MutationRecord("childList", parent);
    rec.removedNodes = remaining;
    rec.addedNodes = added;
    notifyMutation(rec);
  }
}

DocumentFragment.prototype.replaceChildren = function (...nodes) {
  replaceChildrenImpl(this, nodes);
};
markNative(DocumentFragment.prototype.replaceChildren, "replaceChildren");

function convertNodesToFragment(nodes, ownerDoc) {
  const doc = ownerDoc || (typeof document !== "undefined" ? document : null);
  if (nodes.length === 1) {
    const n = nodes[0];
    if (n && typeof n === "object" && typeof n.nodeType === "number") return n;
    return doc && typeof doc.createTextNode === "function" ? doc.createTextNode(String(n)) : new Text(String(n));
  }
  const frag = doc && typeof doc.createDocumentFragment === "function" ? doc.createDocumentFragment() : new DocumentFragment();
  for (let i = 0; i < nodes.length; i++) {
    const n = nodes[i];
    const node = (n && typeof n === "object" && typeof n.nodeType === "number")
      ? n
      : (doc && typeof doc.createTextNode === "function" ? doc.createTextNode(String(n)) : new Text(String(n)));
    frag.appendChild(node);
  }
  return frag;
}

function installChildNodeMethods(proto) {
  if (!proto) return;
  if (!proto.before) {
    proto.before = markNative(function (...nodes) {
      const parent = this.parentNode;
      if (!parent) return;
      let viablePrev = this.previousSibling;
      while (viablePrev && nodes.indexOf(viablePrev) !== -1) viablePrev = viablePrev.previousSibling;
      const node = convertNodesToFragment(nodes, this.ownerDocument);
      const ref = viablePrev ? viablePrev.nextSibling : parent.firstChild;
      parent.insertBefore(node, ref);
    }, "before");
  }
  if (!proto.after) {
    proto.after = markNative(function (...nodes) {
      const parent = this.parentNode;
      if (!parent) return;
      let viableNext = this.nextSibling;
      while (viableNext && nodes.indexOf(viableNext) !== -1) viableNext = viableNext.nextSibling;
      const node = convertNodesToFragment(nodes, this.ownerDocument);
      parent.insertBefore(node, viableNext);
    }, "after");
  }
  if (!proto.replaceWith) {
    proto.replaceWith = markNative(function (...nodes) {
      const parent = this.parentNode;
      if (!parent) return;
      if (nodes.length === 0) {
        if (this.parentNode === parent) parent.removeChild(this);
        return;
      }
      let viableNext = this.nextSibling;
      while (viableNext && nodes.indexOf(viableNext) !== -1) viableNext = viableNext.nextSibling;
      const node = convertNodesToFragment(nodes, this.ownerDocument);
      if (this.parentNode === parent) {
        parent.insertBefore(node, viableNext);
        if (this.parentNode === parent) parent.removeChild(this);
      } else {
        parent.insertBefore(node, viableNext);
      }
    }, "replaceWith");
  }
  if (!proto.remove) {
    proto.remove = markNative(function () {
      if (this.parentNode && typeof this.parentNode.removeChild === "function") {
        this.parentNode.removeChild(this);
      }
    }, "remove");
  }
}

installChildNodeMethods(CharacterData.prototype);
if (typeof Element !== "undefined" && Element.prototype) installChildNodeMethods(Element.prototype);
if (typeof DocumentType !== "undefined" && DocumentType.prototype) installChildNodeMethods(DocumentType.prototype);
global.__capsurf_installChildNodeMethods = installChildNodeMethods;
if (typeof globalThis !== "undefined") globalThis.__capsurf_installChildNodeMethods = installChildNodeMethods;

markNative(DocumentFragment, "DocumentFragment");

const shadowRootState = new WeakMap();
function ShadowRoot(host, mode) {
  shadowRootState.set(this, {
    host: host || null,
    mode: mode || "open",
    children: [],
  });
}
ShadowRoot.prototype = Object.create(DocumentFragment.prototype);
ShadowRoot.prototype.constructor = ShadowRoot;
Object.defineProperty(ShadowRoot.prototype, Symbol.toStringTag, {
  value: "ShadowRoot",
  configurable: true,
});
Object.defineProperty(ShadowRoot.prototype, "innerHTML", {
  get: function () {
    if (!shadowRootState.has(this)) {
      throw new TypeError("Illegal invocation");
    }
    const kids = ensureChildren(this);
    let html = "";
    for (let i = 0; i < kids.length; i++) {
      const k = kids[i];
      if (k.nodeType === 1) html += k.outerHTML || "";
      else if (k.nodeType === 3) html += k.textContent || "";
      else if (k.nodeType === 8) html += "<!--" + (k.data || "") + "-->";
    }
    return html;
  },
  set: function (val) {
    if (!shadowRootState.has(this)) {
      throw new TypeError("Illegal invocation");
    }
    while (this.firstChild) {
      this.removeChild(this.firstChild);
    }
    const doc = this.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
    const nativeCreate = typeof globalThis !== "undefined" ? globalThis.__capsurf_native_createElement : null;
    const temp = (doc && typeof nativeCreate === "function")
      ? nativeCreate.call(doc, "div")
      : (doc ? doc.createElement("div") : null);
    if (temp) {
      temp.innerHTML = val;
      while (temp.firstChild) {
        const ch = temp.firstChild;
        try { temp.removeChild(ch); } catch (e) {}
        this.appendChild(ch);
      }
    }
  },
  configurable: true,
});
Object.defineProperty(ShadowRoot.prototype, "nodeType", {
  get: function () { return DOCUMENT_FRAGMENT_NODE; },
  configurable: true,
});
Object.defineProperty(ShadowRoot.prototype, "nodeName", {
  get: function () { return "#document-fragment"; },
  configurable: true,
});
Object.defineProperty(ShadowRoot.prototype, "parentNode", {
  get: function () { return null; },
  configurable: true,
});
Object.defineProperty(ShadowRoot.prototype, "host", {
  get: function () {
    const st = shadowRootState.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    return st.host;
  },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(ShadowRoot.prototype, "mode", {
  get: function () {
    const st = shadowRootState.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    return st.mode;
  },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(ShadowRoot.prototype, "_children", {
  get: function () {
    const st = shadowRootState.get(this);
    return st ? st.children : undefined;
  },
  set: function (v) {
    const st = shadowRootState.get(this);
    if (st) st.children = v;
  },
  configurable: true,
});
markNative(ShadowRoot, "ShadowRoot");
Object.defineProperty(ShadowRoot.prototype, "styleSheets", {
  get: function () {
    if (typeof global.getOrCreateStyleSheetList === "function") {
      return global.getOrCreateStyleSheetList(this);
    }
    if (typeof globalThis.getOrCreateStyleSheetList === "function") {
      return globalThis.getOrCreateStyleSheetList(this);
    }
    return [];
  },
  configurable: true,
});
Object.defineProperty(ShadowRoot.prototype, "activeElement", {
  get: function () {
    const doc = (typeof document !== "undefined" ? document : null) || global.document;
    const ae = doc ? (doc._activeElement || doc.activeElement) : null;
    if (!ae) return null;
    let cur = ae;
    while (cur && typeof cur.getRootNode === "function") {
      const rn = cur.getRootNode();
      if (rn === this) return cur;
      if (rn && rn.host) {
        cur = rn.host;
      } else {
        break;
      }
    }
    return null;
  },
  configurable: true,
});
ShadowRoot.prototype.elementFromPoint = markNative(function (x, y) {
  const vx = Number(x);
  const vy = Number(y);
  if (Number.isNaN(vx) || Number.isNaN(vy)) return null;
  let best = null;
  function hitWalk(root) {
    const kids = (root && (root.children || root._children || root.childNodes)) || [];
    for (let i = 0; i < kids.length; i++) {
      const k = kids[i];
      if (!k || k.nodeType !== 1) continue;
      const tag = String(k.localName || k.tagName || "").toLowerCase();
      if (tag === "style" || tag === "script" || tag === "template") continue;
      if ((!k.namespaceURI || k.namespaceURI === "http://www.w3.org/1999/xhtml") &&
          typeof k.hasAttribute === "function" && k.hasAttribute("inert")) continue;
      let pe = "";
      let vis = "";
      let disp = "";
      if (typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
        try {
          const cs = window.getComputedStyle(k);
          if (cs) {
            pe = cs.pointerEvents || "";
            vis = cs.visibility || "";
            disp = cs.display || "";
          }
        } catch (e) {}
      }
      if (disp === "none") continue;
      if (typeof k.getBoundingClientRect === "function") {
        const r = k.getBoundingClientRect();
        if (r && r.width > 0 && r.height > 0 && vx >= r.left && vx <= r.right && vy >= r.top && vy <= r.bottom) {
          if (pe !== "none" && vis !== "hidden") best = k;
        }
      }
      hitWalk(k);
    }
  }
  hitWalk(this);
  if (best && best._shadowRoot && typeof best._shadowRoot.elementFromPoint === "function") {
    const deeper = best._shadowRoot.elementFromPoint(vx, vy);
    if (deeper) return deeper;
  }
  if (!best && this.host && typeof this.host.getBoundingClientRect === "function") {
    const h = this.host;
    const isInertHost = (!h.namespaceURI || h.namespaceURI === "http://www.w3.org/1999/xhtml") &&
      typeof h.hasAttribute === "function" && h.hasAttribute("inert");
    if (!isInertHost) {
      const hr = h.getBoundingClientRect();
      if (hr && hr.width > 0 && hr.height > 0 && vx >= hr.left && vx <= hr.right && vy >= hr.top && vy <= hr.bottom) {
        return h;
      }
    }
  }
  return best;
}, "elementFromPoint");

ShadowRoot.prototype.elementsFromPoint = markNative(function (x, y) {
  const out = [];
  let cur = this.elementFromPoint(x, y);
  while (cur && cur.nodeType === 1) {
    out.push(cur);
    cur = cur.parentNode || cur.parentElement;
    if (cur && cur === this) break;
  }
  return out;
}, "elementsFromPoint");

(function () {
  const VOID_HTML_TAGS = new Set([
    "area", "base", "basefont", "bgsound", "br", "col", "embed", "frame",
    "hr", "img", "input", "keygen", "link", "meta", "param", "source",
    "track", "wbr"
  ]);
  function escapeHtmlText(str) {
    return String(str)
      .replace(/&/g, "&amp;")
      .replace(/\u00a0/g, "&nbsp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;");
  }
  function escapeHtmlAttr(str) {
    return String(str)
      .replace(/&/g, "&amp;")
      .replace(/\u00a0/g, "&nbsp;")
      .replace(/"/g, "&quot;");
  }
  function getNodeKids(node) {
    const out = [];
    if (node.firstChild) {
      for (let c = node.firstChild; c; c = c.nextSibling) out.push(c);
      return out;
    }
    if (node._children && node._children.length > 0) return node._children;
    if (node.childNodes && node.childNodes.length > 0) return node.childNodes;
    return out;
  }
  function serializeChildren(node, serializableShadowRoots, shadowRootsSet) {
    let html = "";
    const kids = getNodeKids(node);
    for (let i = 0; i < kids.length; i++) {
      const ch = kids[i];
      if (!ch) continue;
      const nt = ch.nodeType;
      if (nt === 1) {
        const tag = String(ch.localName || ch.tagName || "").toLowerCase();
        let attrStr = "";
        const attrs = ch.attributes;
        if (attrs && attrs.length) {
          for (let ai = 0; ai < attrs.length; ai++) {
            const a = attrs[ai];
            if (!a) continue;
            attrStr += " " + a.name + '="' + escapeHtmlAttr(a.value == null ? "" : a.value) + '"';
          }
        }
        html += "<" + tag + attrStr + ">";
        if (!VOID_HTML_TAGS.has(tag)) {
          const sr = ch._shadowRoot;
          if (sr && ((serializableShadowRoots && sr.serializable) || (shadowRootsSet && shadowRootsSet.has(sr)))) {
            let srAttrs = ' shadowrootmode="' + (sr.mode || "open") + '"';
            if (sr.delegatesFocus) srAttrs += ' shadowrootdelegatesfocus=""';
            if (sr.serializable) srAttrs += ' shadowrootserializable=""';
            if (sr.clonable) srAttrs += ' shadowrootclonable=""';
            html += "<template" + srAttrs + ">" + serializeChildren(sr, serializableShadowRoots, shadowRootsSet) + "</template>";
          }
          if (tag === "template" && ch.content) {
            html += serializeChildren(ch.content, serializableShadowRoots, shadowRootsSet);
          } else {
            html += serializeChildren(ch, serializableShadowRoots, shadowRootsSet);
          }
          html += "</" + tag + ">";
        }
      } else if (nt === 3) {
        const txt = ch.data !== undefined ? ch.data : (ch.textContent || "");
        html += escapeHtmlText(txt);
      } else if (nt === 8) {
        const cdata = ch.data !== undefined ? ch.data : (ch.textContent || "");
        html += "<!--" + cdata + "-->";
      } else if (nt === 7) {
        html += "<?" + (ch.target || "") + " " + (ch.data || "") + "?>";
      }
    }
    return html;
  }
  function getHTMLImpl(node, options) {
    const opts = (options && typeof options === "object") ? options : {};
    const serializableShadowRoots = Boolean(opts.serializableShadowRoots);
    let shadowRootsSet = null;
    if (opts.shadowRoots != null) {
      shadowRootsSet = new Set();
      for (const sr of opts.shadowRoots) shadowRootsSet.add(sr);
    }
    let out = "";
    if (node && node.nodeType === 1) {
      const sr = node._shadowRoot;
      if (sr && ((serializableShadowRoots && sr.serializable) || (shadowRootsSet && shadowRootsSet.has(sr)))) {
        let srAttrs = ' shadowrootmode="' + (sr.mode || "open") + '"';
        if (sr.delegatesFocus) srAttrs += ' shadowrootdelegatesfocus=""';
        if (sr.serializable) srAttrs += ' shadowrootserializable=""';
        if (sr.clonable) srAttrs += ' shadowrootclonable=""';
        out += "<template" + srAttrs + ">" + serializeChildren(sr, serializableShadowRoots, shadowRootsSet) + "</template>";
      }
    }
    out += serializeChildren(node, serializableShadowRoots, shadowRootsSet);
    return out;
  }
  global.__capsurf_getHTMLImpl = getHTMLImpl;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_getHTMLImpl = getHTMLImpl;
  ShadowRoot.prototype.getHTML = markNative(function (options) {
    if (!shadowRootState.has(this)) {
      throw new TypeError("Illegal invocation");
    }
    return getHTMLImpl(this, options);
  }, "getHTML");
})();

global.ShadowRoot = ShadowRoot;
if (typeof globalThis !== "undefined") {
  globalThis.ShadowRoot = ShadowRoot;
}
if (typeof global.__capsurf_defineAdoptedStyleSheets === "function") {
  global.__capsurf_defineAdoptedStyleSheets(ShadowRoot.prototype);
}

if (typeof Document !== "undefined" && Document.prototype) {
  if (!Object.getOwnPropertyDescriptor(Document.prototype, "textContent")) {
    Object.defineProperty(Document.prototype, "textContent", {
      get: function () { return null; },
      set: function () {},
      configurable: true,
    });
  }
  if (!Document.prototype.createTextNode) {
    Document.prototype.createTextNode = function (text) {
      return new Text(text);
    };
    markNative(Document.prototype.createTextNode, "createTextNode");
  }
  if (!Document.prototype.createComment) {
    Document.prototype.createComment = function (data) {
      return new Comment(data);
    };
    markNative(Document.prototype.createComment, "createComment");
  }
  if (!Document.prototype.createDocumentFragment) {
    Document.prototype.createDocumentFragment = function () {
      return new DocumentFragment();
    };
    markNative(Document.prototype.createDocumentFragment, "createDocumentFragment");
  }
  if (!Document.prototype.importNode) {
    Document.prototype.importNode = function (node, deep) {
      if (!node || typeof node !== "object") {
        throw new TypeError("Failed to execute 'importNode' on 'Document': parameter 1 is not of type 'Node'.");
      }
      const cloned = node.cloneNode(Boolean(deep));
      const ce = (typeof customElements !== "undefined" && customElements) ||
        (typeof global !== "undefined" && global.customElements) ||
        (typeof globalThis !== "undefined" && globalThis.customElements);
      if (ce && typeof ce.upgrade === "function" && !this._isInertTemplateDocument) {
        ce.upgrade(cloned);
      }
      return cloned;
    };
    markNative(Document.prototype.importNode, "importNode");
  }
}
