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

Object.defineProperty(Node.prototype, "isConnected", {
  get: function () {
    let curr = this;
    while (curr) {
      if (curr.nodeType === DOCUMENT_NODE) return true;
      if (curr.host) {
        curr = curr.host;
        continue;
      }
      curr = curr.parentNode || curr.parentElement;
    }
    return false;
  },
  configurable: true,
});

Node.prototype.getRootNode = function (options) {
  let n = this;
  let p;
  while ((p = n.parentNode)) n = p;

  if (options && options.composed) {
    while (n && n.host) {
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
    this._ownerDocument = doc;
  },
  configurable: true,
});

Node.prototype.contains = function (other) {
  if (!other) return false;
  let curr = other;
  while (curr) {
    if (curr === this || (this._nid !== undefined && curr._nid !== undefined && curr._nid === this._nid)) return true;
    curr = curr.parentNode || curr.parentElement;
  }
  return false;
};

Node.prototype.compareDocumentPosition = function (other) {
  if (this === other) return 0;
  if (!other || !other.nodeType) return DOCUMENT_POSITION_DISCONNECTED;

  if (this.contains(other)) {
    return DOCUMENT_POSITION_CONTAINED_BY | DOCUMENT_POSITION_FOLLOWING;
  }
  if (other.contains(this)) {
    return DOCUMENT_POSITION_CONTAINS | DOCUMENT_POSITION_PRECEDING;
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
    return new Text(this.textContent);
  }
  if (this.nodeType === COMMENT_NODE) {
    return new Comment(this.textContent);
  }
  if (this.nodeType === ELEMENT_NODE && typeof document !== "undefined" && document.createElement) {
    const clone = document.createElement(this.tagName.toLowerCase());
    if (this.id) clone.id = this.id;
    if (this.className) clone.className = this.className;
    if (this.getAttributeNames) {
      const attrs = this.getAttributeNames();
      for (let i = 0; i < attrs.length; i++) {
        clone.setAttribute(attrs[i], this.getAttribute(attrs[i]));
      }
    }
    if (deep && this.childNodes) {
      for (let i = 0; i < this.childNodes.length; i++) {
        clone.appendChild(this.childNodes[i].cloneNode(true));
      }
    }
    return clone;
  }
  return this;
};

function isGenuineNode(obj) {
  if (!obj || typeof obj !== "object") return false;
  if (obj instanceof Node) return true;
  if (isParsedNode(obj)) return true;
  return false;
}

const nativeElementAppendChild = (typeof Element !== "undefined" && Element.prototype) ? Element.prototype.appendChild : null;
const nativeElementRemoveChild = (typeof Element !== "undefined" && Element.prototype) ? Element.prototype.removeChild : null;
const nativeElementGetTextContent = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "textContent")?.get : null;
const nativeElementSetTextContent = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "textContent")?.set : null;
const nativeElementFirstChild = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "firstChild")?.get : null;
const nativeElementNextSibling = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "nextSibling")?.get : null;

function ensureChildren(node) {
  if (!node || typeof node !== "object") return [];
  if (!node._children) {
    node._children = [];
    try {
      let child = null;
      if (node.nodeType === 9) { // DOCUMENT_NODE
        child = node.documentElement;
      } else if (nativeElementFirstChild) {
        child = nativeElementFirstChild.call(node);
      }
      while (child) {
        node._children.push(child);
        if (nativeElementNextSibling) {
          child = nativeElementNextSibling.call(child);
        } else {
          break;
        }
      }
    } catch (e) {}
  }
  return node._children;
}

Node.prototype.appendChild = function (child) {
  if (!this || typeof this !== "object") {
    throw new TypeError("Illegal invocation");
  }
  if (!child || typeof child !== "object" || !isGenuineNode(child)) {
    throw new TypeError("Failed to execute 'appendChild' on 'Node': parameter 1 is not of type 'Node'.");
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

  // If child already has a parent, remove from it
  if (child.parentNode && typeof child.parentNode.removeChild === "function") {
    try {
      child.parentNode.removeChild(child);
    } catch (e) {}
  }

  // If child is a parsed node, adopt it safely into this document
  if (isParsedNode(child)) {
    child.parentNode = this;
    child.ownerDocument = this.ownerDocument || this;
    if (child.nodeType === 1) { // ELEMENT_NODE
      // When adopted, child's subtree in parsed document is not adopted into live tree
      child._children = [];
    }
    ensureChildren(this).push(child);
    return child;
  }

  // Handle native element / node
  child.parentNode = this;
  child.ownerDocument = this.ownerDocument || this;
  ensureChildren(this).push(child);

  if (nativeElementAppendChild && typeof nativeElementAppendChild === "function") {
    try {
      nativeElementAppendChild.call(this, child);
    } catch (e) {}
  }

  if (typeof notifyMutation === "function") {
    const rec = new MutationRecord("childList", this);
    rec.addedNodes = [child];
    notifyMutation(rec);
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

  if (newChild.nodeType === DOCUMENT_FRAGMENT_NODE) {
    const kids = Array.from(newChild.childNodes || newChild._children || []);
    for (let i = 0; i < kids.length; i++) {
      this.insertBefore(kids[i], refChild);
    }
    if (newChild._children) newChild._children.length = 0;
    return newChild;
  }

  if (newChild.parentNode && typeof newChild.parentNode.removeChild === "function") {
    try {
      newChild.parentNode.removeChild(newChild);
    } catch (e) {}
  }

  newChild.parentNode = this;
  newChild.ownerDocument = this.ownerDocument || this;
  if (isParsedNode(newChild) && newChild.nodeType === 1) {
    newChild._children = [];
  }

  const curIdx = children.indexOf(refChild);
  children.splice(curIdx >= 0 ? curIdx : children.length, 0, newChild);

  if (typeof notifyMutation === "function") {
    const rec = new MutationRecord("childList", this);
    rec.addedNodes = [newChild];
    notifyMutation(rec);
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

  if (idx >= 0) {
    children.splice(idx, 1);
  }
  child.parentNode = null;

  if (nativeElementRemoveChild && typeof nativeElementRemoveChild === "function") {
    try {
      nativeElementRemoveChild.call(this, child);
    } catch (e) {}
  }

  if (typeof notifyMutation === "function") {
    const rec = new MutationRecord("childList", this);
    rec.removedNodes = [child];
    notifyMutation(rec);
  }

  return child;
};

Object.defineProperty(Node.prototype, "childNodes", {
  get: function () {
    return new NodeList(ensureChildren(this));
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
          return c.textContent || "";
        }).join("");
      }
      if (nativeElementGetTextContent) {
        return nativeElementGetTextContent.call(this);
      }
      return "";
    },
    set: function (v) {
      if (nativeElementSetTextContent) {
        try {
          nativeElementSetTextContent.call(this, v);
        } catch (e) {}
      }
      this._children = [];
      const val = String(v);
      if (val.length > 0) {
        const t = new Text(val);
        t.parentNode = this;
        t.ownerDocument = this.ownerDocument || this;
        this._children.push(t);
      }
    },
    configurable: true,
  });

  Element.prototype.appendChild = Node.prototype.appendChild;
  Element.prototype.insertBefore = Node.prototype.insertBefore;
  Element.prototype.removeChild = Node.prototype.removeChild;

  const nativeElementInnerHtmlDesc = Object.getOwnPropertyDescriptor(Element.prototype, "innerHTML");
  if (nativeElementInnerHtmlDesc && nativeElementInnerHtmlDesc.set) {
    const origSetInnerHtml = nativeElementInnerHtmlDesc.set;
    Object.defineProperty(Element.prototype, "innerHTML", {
      get: nativeElementInnerHtmlDesc.get,
      set: function (v) {
        if (this._children) {
          this._children.length = 0;
        }
        origSetInnerHtml.call(this, v);
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
  if (this.nodeType === 2) { // ATTRIBUTE_NODE
    return this.ownerElement ? this.ownerElement.lookupNamespaceURI(p) : null;
  }
  const parent = this.parentElement || this.parentNode;
  return parent ? parent.lookupNamespaceURI(p) : null;
};

markNative(Node, "Node");
markNative(Node.prototype.contains, "contains");
markNative(Node.prototype.compareDocumentPosition, "compareDocumentPosition");
markNative(Node.prototype.cloneNode, "cloneNode");
markNative(Node.prototype.appendChild, "appendChild");
markNative(Node.prototype.insertBefore, "insertBefore");
markNative(Node.prototype.removeChild, "removeChild");
markNative(Node.prototype.hasChildNodes, "hasChildNodes");
markNative(Node.prototype.lookupNamespaceURI, "lookupNamespaceURI");
markNative(Node.prototype.getRootNode, "getRootNode");
markNative(Node.prototype.normalize, "normalize");

function Text(data) {
  Node.call(this);
  this._data = String(data !== undefined ? data : "");
  this.nodeType = TEXT_NODE;
  this.nodeName = "#text";
}
Text.prototype = Object.create(Node.prototype);
Text.prototype.constructor = Text;
Object.defineProperty(Text.prototype, "data", {
  get: function () { return this._data !== undefined ? this._data : ""; },
  set: function (v) {
    this._data = String(v);
    if (typeof notifyMutation === "function") {
      notifyMutation(new MutationRecord("characterData", this));
    }
  },
  configurable: true,
});
Object.defineProperty(Text.prototype, "textContent", {
  get: function () { return this.data; },
  set: function (v) { this.data = v; },
  configurable: true,
});
Object.defineProperty(Text.prototype, "nodeValue", {
  get: function () { return this.data; },
  set: function (v) { this.data = v; },
  configurable: true,
});
Object.defineProperty(Text.prototype, "length", {
  get: function () { return this.data.length; },
  configurable: true,
});
markNative(Text, "Text");

function Comment(data) {
  Node.call(this);
  this.data = String(data !== undefined ? data : "");
  this.nodeType = COMMENT_NODE;
  this.nodeName = "#comment";
}
Comment.prototype = Object.create(Node.prototype);
Comment.prototype.constructor = Comment;
Object.defineProperty(Comment.prototype, "textContent", {
  get: function () { return this.data; },
  set: function (v) { this.data = String(v); },
  configurable: true,
});
Object.defineProperty(Comment.prototype, "nodeValue", {
  get: function () { return this.data; },
  set: function (v) { this.data = String(v); },
  configurable: true,
});
Object.defineProperty(Comment.prototype, "length", {
  get: function () { return this.data.length; },
  configurable: true,
});
markNative(Comment, "Comment");

function DocumentFragment() {
  Node.call(this);
  this.nodeType = DOCUMENT_FRAGMENT_NODE;
  this.nodeName = "#document-fragment";
  this._children = [];
}
DocumentFragment.prototype = Object.create(Node.prototype);
DocumentFragment.prototype.constructor = DocumentFragment;
DocumentFragment.prototype.appendChild = function (child) {
  return Node.prototype.appendChild.call(this, child);
};
DocumentFragment.prototype.insertBefore = function (newChild, refChild) {
  return Node.prototype.insertBefore.call(this, newChild, refChild);
};
DocumentFragment.prototype.removeChild = function (child) {
  if (!child || child.parentNode !== this || this._children.indexOf(child) === -1) {
    if (typeof DOMException !== "undefined") {
      throw new DOMException("NotFoundError: Node was not found", "NotFoundError");
    }
    const err = new Error("NotFoundError: Node was not found");
    err.name = "NotFoundError";
    throw err;
  }
  const idx = this._children.indexOf(child);
  if (idx >= 0) this._children.splice(idx, 1);
  child.parentNode = null;
  return child;
};
Object.defineProperty(DocumentFragment.prototype, "childNodes", {
  get: function () {
    return new NodeList(this._children);
  },
  configurable: true,
});
markNative(DocumentFragment, "DocumentFragment");

if (typeof Document !== "undefined" && Document.prototype) {
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
}
