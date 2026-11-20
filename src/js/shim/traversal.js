// src/js/shim/traversal.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/traversal.ts

const NodeFilter = {
  SHOW_ALL: SHOW_ALL,
  SHOW_ELEMENT: SHOW_ELEMENT,
  SHOW_ATTRIBUTE: SHOW_ATTRIBUTE,
  SHOW_TEXT: SHOW_TEXT,
  SHOW_CDATA_SECTION: SHOW_CDATA_SECTION,
  SHOW_ENTITY_REFERENCE: SHOW_ENTITY_REFERENCE,
  SHOW_ENTITY: SHOW_ENTITY,
  SHOW_PROCESSING_INSTRUCTION: SHOW_PROCESSING_INSTRUCTION,
  SHOW_COMMENT: SHOW_COMMENT,
  SHOW_DOCUMENT: SHOW_DOCUMENT,
  SHOW_DOCUMENT_TYPE: SHOW_DOCUMENT_TYPE,
  SHOW_DOCUMENT_FRAGMENT: SHOW_DOCUMENT_FRAGMENT,
  SHOW_NOTATION: SHOW_NOTATION,
  FILTER_ACCEPT: FILTER_ACCEPT,
  FILTER_REJECT: FILTER_REJECT,
  FILTER_SKIP: FILTER_SKIP,
};

function filterNode(node, whatToShow, filter) {
  const type = node.nodeType;
  const bit = 1 << (type - 1);
  if (!(whatToShow & bit)) {
    return FILTER_SKIP;
  }
  if (!filter) return FILTER_ACCEPT;
  if (typeof filter === "function") {
    return filter(node);
  }
  if (typeof filter.acceptNode === "function") {
    return filter.acceptNode(node);
  }
  return FILTER_ACCEPT;
}

const _twState = new WeakMap();

function TreeWalker(root, whatToShow, filter) {
  if (!root) throw new TypeError("Root node required");
  _twState.set(this, {
    root: root,
    whatToShow: whatToShow !== undefined ? Number(whatToShow) : SHOW_ALL,
    filter: filter || null,
    currentNode: root,
  });
}

["root", "whatToShow", "filter"].forEach(function (prop) {
  Object.defineProperty(TreeWalker.prototype, prop, {
    get: function () {
      const st = _twState.get(this);
      if (!st) throw new TypeError("Illegal invocation");
      return st[prop];
    },
    configurable: true,
    enumerable: true,
  });
});
Object.defineProperty(TreeWalker.prototype, "currentNode", {
  get: function () {
    const st = _twState.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    return st.currentNode;
  },
  set: function (v) {
    const st = _twState.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    st.currentNode = v;
  },
  configurable: true,
  enumerable: true,
});

TreeWalker.prototype.parentNode = function () {
  let node = this.currentNode;
  while (node && node !== this.root) {
    node = node.parentNode || node.parentElement;
    if (node) {
      const res = filterNode(node, this.whatToShow, this.filter);
      if (res === FILTER_ACCEPT) {
        this.currentNode = node;
        return node;
      }
    }
  }
  return null;
};

TreeWalker.prototype.firstChild = function () {
  let node = this.currentNode.firstChild;
  while (node) {
    const res = filterNode(node, this.whatToShow, this.filter);
    if (res === FILTER_ACCEPT) {
      this.currentNode = node;
      return node;
    }
    if (res === FILTER_SKIP && node.firstChild) {
      node = node.firstChild;
      continue;
    }
    node = node.nextSibling;
  }
  return null;
};

TreeWalker.prototype.lastChild = function () {
  let node = this.currentNode.lastChild;
  while (node) {
    const res = filterNode(node, this.whatToShow, this.filter);
    if (res === FILTER_ACCEPT) {
      this.currentNode = node;
      return node;
    }
    if (res === FILTER_SKIP && node.lastChild) {
      node = node.lastChild;
      continue;
    }
    node = node.previousSibling;
  }
  return null;
};

TreeWalker.prototype.previousSibling = function () {
  let node = this.currentNode;
  if (node === this.root) return null;
  while (node) {
    let sibling = node.previousSibling;
    while (sibling) {
      const res = filterNode(sibling, this.whatToShow, this.filter);
      if (res === FILTER_ACCEPT) {
        this.currentNode = sibling;
        return sibling;
      }
      sibling = sibling.lastChild || sibling.previousSibling;
    }
    node = node.parentNode || node.parentElement;
    if (!node || node === this.root) return null;
    if (filterNode(node, this.whatToShow, this.filter) === FILTER_ACCEPT) return null;
  }
  return null;
};

TreeWalker.prototype.nextSibling = function () {
  let node = this.currentNode;
  if (node === this.root) return null;
  while (node) {
    let sibling = node.nextSibling;
    while (sibling) {
      const res = filterNode(sibling, this.whatToShow, this.filter);
      if (res === FILTER_ACCEPT) {
        this.currentNode = sibling;
        return sibling;
      }
      sibling = sibling.firstChild || sibling.nextSibling;
    }
    node = node.parentNode || node.parentElement;
    if (!node || node === this.root) return null;
    if (filterNode(node, this.whatToShow, this.filter) === FILTER_ACCEPT) return null;
  }
  return null;
};

TreeWalker.prototype.nextNode = function () {
  let node = this.currentNode;
  while (node) {
    let next = node.firstChild;
    while (!next && node && node !== this.root) {
      next = node.nextSibling;
      node = node.parentNode || node.parentElement;
    }
    if (!next) return null;
    node = next;
    const res = filterNode(node, this.whatToShow, this.filter);
    if (res === FILTER_ACCEPT) {
      this.currentNode = node;
      return node;
    }
  }
  return null;
};

TreeWalker.prototype.previousNode = function () {
  let node = this.currentNode;
  if (node === this.root) return null;
  while (node && node !== this.root) {
    let prev = node.previousSibling;
    if (prev) {
      while (prev.lastChild) prev = prev.lastChild;
      node = prev;
    } else {
      node = node.parentNode || node.parentElement;
    }
    if (!node) return null;
    const res = filterNode(node, this.whatToShow, this.filter);
    if (res === FILTER_ACCEPT) {
      this.currentNode = node;
      return node;
    }
  }
  return null;
};

markNative(TreeWalker, "TreeWalker");
markNative(TreeWalker.prototype.parentNode, "parentNode");
markNative(TreeWalker.prototype.firstChild, "firstChild");
markNative(TreeWalker.prototype.lastChild, "lastChild");
markNative(TreeWalker.prototype.previousSibling, "previousSibling");
markNative(TreeWalker.prototype.nextSibling, "nextSibling");
markNative(TreeWalker.prototype.nextNode, "nextNode");
markNative(TreeWalker.prototype.previousNode, "previousNode");

const _niState = new WeakMap();
const _activeNodeIterators = [];

global.__capsurf_onNodeRemovedForIterators = function (parent, removedNode, prevSibling) {
  for (let i = 0; i < _activeNodeIterators.length; i++) {
    const ni = _activeNodeIterators[i];
    const st = _niState.get(ni);
    if (!st) continue;
    const sameRef = st.referenceNode === removedNode ||
      (st.referenceNode && removedNode && st.referenceNode._nid !== undefined && st.referenceNode._nid === removedNode._nid);
    if (sameRef || (removedNode && typeof removedNode.contains === "function" && removedNode.contains(st.referenceNode))) {
      st.referenceNode = prevSibling || parent;
      st.pointerBeforeReferenceNode = false;
      st.walker.currentNode = st.referenceNode;
    }
  }
};

function NodeIterator(root, whatToShow, filter) {
  if (!root) throw new TypeError("Root node required");
  const wts = whatToShow !== undefined ? Number(whatToShow) : SHOW_ALL;
  const flt = filter || null;
  _niState.set(this, {
    root: root,
    whatToShow: wts,
    filter: flt,
    referenceNode: root,
    pointerBeforeReferenceNode: true,
    firstCall: true,
    walker: new TreeWalker(root, wts, flt),
  });
  _activeNodeIterators.push(this);
}

["root", "whatToShow", "filter", "referenceNode", "pointerBeforeReferenceNode"].forEach(function (prop) {
  Object.defineProperty(NodeIterator.prototype, prop, {
    get: function () {
      const st = _niState.get(this);
      if (!st) throw new TypeError("Illegal invocation");
      return st[prop];
    },
    configurable: true,
    enumerable: true,
  });
});

NodeIterator.prototype.nextNode = function () {
  const st = _niState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  if (st.firstCall) {
    st.firstCall = false;
    if (filterNode(st.root, st.whatToShow, st.filter) === FILTER_ACCEPT) {
      st.referenceNode = st.root;
      st.pointerBeforeReferenceNode = false;
      return st.root;
    }
  }
  const node = st.walker.nextNode();
  if (node) {
    st.referenceNode = node;
    st.pointerBeforeReferenceNode = false;
  }
  return node;
};

NodeIterator.prototype.previousNode = function () {
  const st = _niState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  const node = st.walker.previousNode();
  if (node) {
    st.referenceNode = node;
    st.pointerBeforeReferenceNode = true;
  }
  return node;
};

NodeIterator.prototype.detach = function () {};

markNative(NodeIterator, "NodeIterator");
markNative(NodeIterator.prototype.nextNode, "nextNode");
markNative(NodeIterator.prototype.previousNode, "previousNode");
markNative(NodeIterator.prototype.detach, "detach");
