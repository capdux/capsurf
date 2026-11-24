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
  if (filter === null || filter === undefined) return FILTER_ACCEPT;
  let res;
  if (typeof filter === "function") {
    res = filter(node);
  } else {
    const acceptNode = filter.acceptNode;
    if (typeof acceptNode !== "function") {
      throw new TypeError("Failed to execute 'acceptNode' on 'NodeFilter': The provided value is not a function.");
    }
    res = acceptNode.call(filter, node);
  }
  return (Number(res) >>> 0) & 0xFFFF;
}

const _twState = new WeakMap();

function TreeWalker(root, whatToShow, filter) {
  if (!root || typeof root.nodeType !== "number") throw new TypeError("Root node required");
  _twState.set(this, {
    root: root,
    whatToShow: whatToShow !== undefined ? (Number(whatToShow) >>> 0) : SHOW_ALL,
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
    if (!v || typeof v.nodeType !== "number") throw new TypeError("Failed to set 'currentNode': value is not of type 'Node'.");
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

function traverseChildren(walker, isFirst) {
  let node = isFirst ? walker.currentNode.firstChild : walker.currentNode.lastChild;
  while (node) {
    const result = filterNode(node, walker.whatToShow, walker.filter);
    if (result === FILTER_ACCEPT) {
      walker.currentNode = node;
      return node;
    }
    if (result === FILTER_SKIP) {
      const child = isFirst ? node.firstChild : node.lastChild;
      if (child) {
        node = child;
        continue;
      }
    }
    while (node) {
      const sibling = isFirst ? node.nextSibling : node.previousSibling;
      if (sibling) {
        node = sibling;
        break;
      }
      const parent = node.parentNode || node.parentElement;
      if (!parent || parent === walker.root || parent === walker.currentNode) return null;
      node = parent;
    }
  }
  return null;
}

function traverseSiblings(walker, isNext) {
  let node = walker.currentNode;
  if (node === walker.root) return null;
  while (true) {
    let sibling = isNext ? node.nextSibling : node.previousSibling;
    while (sibling) {
      node = sibling;
      const result = filterNode(node, walker.whatToShow, walker.filter);
      if (result === FILTER_ACCEPT) {
        walker.currentNode = node;
        return node;
      }
      sibling = isNext ? node.firstChild : node.lastChild;
      if (result === FILTER_REJECT || !sibling) {
        sibling = isNext ? node.nextSibling : node.previousSibling;
      }
    }
    node = node.parentNode || node.parentElement;
    if (!node || node === walker.root) return null;
    if (filterNode(node, walker.whatToShow, walker.filter) === FILTER_ACCEPT) return null;
  }
}

TreeWalker.prototype.firstChild = function () {
  return traverseChildren(this, true);
};

TreeWalker.prototype.lastChild = function () {
  return traverseChildren(this, false);
};

TreeWalker.prototype.previousSibling = function () {
  return traverseSiblings(this, false);
};

TreeWalker.prototype.nextSibling = function () {
  return traverseSiblings(this, true);
};

TreeWalker.prototype.nextNode = function () {
  let node = this.currentNode;
  let result = FILTER_ACCEPT;
  while (true) {
    while (result !== FILTER_REJECT && node.firstChild) {
      node = node.firstChild;
      result = filterNode(node, this.whatToShow, this.filter);
      if (result === FILTER_ACCEPT) {
        this.currentNode = node;
        return node;
      }
    }
    let following = null;
    while (node) {
      if (node === this.root) return null;
      if (node.nextSibling) {
        following = node.nextSibling;
        break;
      }
      node = node.parentNode || node.parentElement;
    }
    if (!following) return null;
    node = following;
    result = filterNode(node, this.whatToShow, this.filter);
    if (result === FILTER_ACCEPT) {
      this.currentNode = node;
      return node;
    }
  }
};

TreeWalker.prototype.previousNode = function () {
  let node = this.currentNode;
  while (node !== this.root) {
    let sibling = node.previousSibling;
    while (sibling) {
      node = sibling;
      let result = filterNode(node, this.whatToShow, this.filter);
      while (result !== FILTER_REJECT && node.lastChild) {
        node = node.lastChild;
        result = filterNode(node, this.whatToShow, this.filter);
      }
      if (result === FILTER_ACCEPT) {
        this.currentNode = node;
        return node;
      }
      sibling = node.previousSibling;
    }
    if (node === this.root) return null;
    const parent = node.parentNode || node.parentElement;
    if (!parent) return null;
    node = parent;
    if (filterNode(node, this.whatToShow, this.filter) === FILTER_ACCEPT) {
      this.currentNode = node;
      return node;
    }
  }
  return null;
};

markNative(TreeWalker, "TreeWalker");
Object.defineProperty(TreeWalker.prototype, Symbol.toStringTag, {
  value: "TreeWalker",
  configurable: true,
});
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
Object.defineProperty(NodeIterator.prototype, Symbol.toStringTag, {
  value: "NodeIterator",
  configurable: true,
});
markNative(NodeIterator.prototype.nextNode, "nextNode");
markNative(NodeIterator.prototype.previousNode, "previousNode");
markNative(NodeIterator.prototype.detach, "detach");
