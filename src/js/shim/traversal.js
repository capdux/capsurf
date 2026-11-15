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

function TreeWalker(root, whatToShow, filter) {
  if (!root) throw new TypeError("Root node required");
  this.root = root;
  this.whatToShow = whatToShow !== undefined ? Number(whatToShow) : SHOW_ALL;
  this.filter = filter || null;
  this.currentNode = root;
}

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

function NodeIterator(root, whatToShow, filter) {
  if (!root) throw new TypeError("Root node required");
  this.root = root;
  this.whatToShow = whatToShow !== undefined ? Number(whatToShow) : SHOW_ALL;
  this.filter = filter || null;
  this.referenceNode = root;
  this.pointerBeforeReferenceNode = true;
  this._walker = new TreeWalker(root, this.whatToShow, this.filter);
}

NodeIterator.prototype.nextNode = function () {
  const node = this._walker.nextNode();
  if (node) {
    this.referenceNode = node;
    this.pointerBeforeReferenceNode = false;
  }
  return node;
};

NodeIterator.prototype.previousNode = function () {
  const node = this._walker.previousNode();
  if (node) {
    this.referenceNode = node;
    this.pointerBeforeReferenceNode = true;
  }
  return node;
};

NodeIterator.prototype.detach = function () {};

markNative(NodeIterator, "NodeIterator");
markNative(NodeIterator.prototype.nextNode, "nextNode");
markNative(NodeIterator.prototype.previousNode, "previousNode");
markNative(NodeIterator.prototype.detach, "detach");
