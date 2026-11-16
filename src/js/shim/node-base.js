// src/js/shim/node-base.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/node-base.ts

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
      curr = curr.parentNode || curr.parentElement;
    }
    return false;
  },
  configurable: true,
});

Object.defineProperty(Node.prototype, "ownerDocument", {
  get: function () {
    if (this.nodeType === DOCUMENT_NODE) return null;
    return typeof document !== "undefined" ? document : null;
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

markNative(Node, "Node");
markNative(Node.prototype.contains, "contains");
markNative(Node.prototype.compareDocumentPosition, "compareDocumentPosition");
markNative(Node.prototype.cloneNode, "cloneNode");

function Text(data) {
  Node.call(this);
  this.data = String(data !== undefined ? data : "");
  this.nodeType = TEXT_NODE;
  this.nodeName = "#text";
}
Text.prototype = Object.create(Node.prototype);
Text.prototype.constructor = Text;
Object.defineProperty(Text.prototype, "textContent", {
  get: function () { return this.data; },
  set: function (v) { this.data = String(v); },
  configurable: true,
});
Object.defineProperty(Text.prototype, "nodeValue", {
  get: function () { return this.data; },
  set: function (v) { this.data = String(v); },
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
  if (child) {
    this._children.push(child);
    child.parentNode = this;
  }
  return child;
};
Object.defineProperty(DocumentFragment.prototype, "childNodes", {
  get: function () {
    return new NodeList(this._children);
  },
  configurable: true,
});
markNative(DocumentFragment, "DocumentFragment");
