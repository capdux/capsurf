// src/js/shim/range.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/range.ts

function Range() {
  this.startContainer = typeof document !== "undefined" ? document : null;
  this.startOffset = 0;
  this.endContainer = this.startContainer;
  this.endOffset = 0;
  this.collapsed = true;
  this.commonAncestorContainer = this.startContainer;
}

Range.START_TO_START = 0;
Range.START_TO_END = 1;
Range.END_TO_END = 2;
Range.END_TO_START = 3;

Range.prototype.setStart = function (node, offset) {
  this.startContainer = node;
  this.startOffset = Number(offset || 0);
  this._updateState();
};

Range.prototype.setEnd = function (node, offset) {
  this.endContainer = node;
  this.endOffset = Number(offset || 0);
  this._updateState();
};

Range.prototype.setStartBefore = function (node) {
  if (!node || !node.parentNode) throw new Error("Parent node required");
  this.setStart(node.parentNode, 0);
};

Range.prototype.setStartAfter = function (node) {
  if (!node || !node.parentNode) throw new Error("Parent node required");
  this.setStart(node.parentNode, 1);
};

Range.prototype.setEndBefore = function (node) {
  if (!node || !node.parentNode) throw new Error("Parent node required");
  this.setEnd(node.parentNode, 0);
};

Range.prototype.setEndAfter = function (node) {
  if (!node || !node.parentNode) throw new Error("Parent node required");
  this.setEnd(node.parentNode, 1);
};

Range.prototype.collapse = function (toStart) {
  if (toStart) {
    this.endContainer = this.startContainer;
    this.endOffset = this.startOffset;
  } else {
    this.startContainer = this.endContainer;
    this.startOffset = this.endOffset;
  }
  this.collapsed = true;
};

Range.prototype.selectNode = function (node) {
  if (!node || !node.parentNode) throw new Error("Parent node required");
  this.setStartBefore(node);
  this.setEndAfter(node);
};

Range.prototype.selectNodeContents = function (node) {
  if (!node) throw new Error("Node required");
  this.startContainer = node;
  this.startOffset = 0;
  this.endContainer = node;
  this.endOffset = node.textContent ? node.textContent.length : 0;
  this._updateState();
};

Range.prototype.compareBoundaryPoints = function (how, sourceRange) {
  return 0;
};

Range.prototype.deleteContents = function () {};
Range.prototype.extractContents = function () {
  return new DocumentFragment();
};
Range.prototype.cloneContents = function () {
  return new DocumentFragment();
};
Range.prototype.insertNode = function (node) {
  if (this.startContainer && this.startContainer.appendChild) {
    this.startContainer.appendChild(node);
  }
};
Range.prototype.surroundContents = function (newParent) {
  if (newParent && this.startContainer && this.startContainer.appendChild) {
    this.startContainer.appendChild(newParent);
  }
};

Range.prototype.cloneRange = function () {
  const r = new Range();
  r.startContainer = this.startContainer;
  r.startOffset = this.startOffset;
  r.endContainer = this.endContainer;
  r.endOffset = this.endOffset;
  r.collapsed = this.collapsed;
  r.commonAncestorContainer = this.commonAncestorContainer;
  return r;
};

Range.prototype.detach = function () {};

Range.prototype.createContextualFragment = function (fragmentHtml) {
  const frag = new DocumentFragment();
  if (typeof document !== "undefined" && document.createElement) {
    const div = document.createElement("div");
    div.innerHTML = String(fragmentHtml || "");
    if (div.childNodes) {
      for (let i = 0; i < div.childNodes.length; i++) {
        frag.appendChild(div.childNodes[i]);
      }
    }
  }
  return frag;
};

Range.prototype._updateState = function () {
  this.collapsed = (this.startContainer === this.endContainer && this.startOffset === this.endOffset);
  this.commonAncestorContainer = this.startContainer;
};

markNative(Range, "Range");
markNative(Range.prototype.setStart, "setStart");
markNative(Range.prototype.setEnd, "setEnd");
markNative(Range.prototype.collapse, "collapse");
markNative(Range.prototype.selectNode, "selectNode");
markNative(Range.prototype.selectNodeContents, "selectNodeContents");
markNative(Range.prototype.cloneRange, "cloneRange");
markNative(Range.prototype.createContextualFragment, "createContextualFragment");

function Selection() {
  this._range = null;
}

Object.defineProperty(Selection.prototype, "anchorNode", {
  get: function () { return this._range ? this._range.startContainer : null; },
  configurable: true,
});

Object.defineProperty(Selection.prototype, "anchorOffset", {
  get: function () { return this._range ? this._range.startOffset : 0; },
  configurable: true,
});

Object.defineProperty(Selection.prototype, "focusNode", {
  get: function () { return this._range ? this._range.endContainer : null; },
  configurable: true,
});

Object.defineProperty(Selection.prototype, "focusOffset", {
  get: function () { return this._range ? this._range.endOffset : 0; },
  configurable: true,
});

Object.defineProperty(Selection.prototype, "isCollapsed", {
  get: function () { return this._range ? this._range.collapsed : true; },
  configurable: true,
});

Object.defineProperty(Selection.prototype, "rangeCount", {
  get: function () { return this._range ? 1 : 0; },
  configurable: true,
});

Selection.prototype.getRangeAt = function (index) {
  if (index === 0 && this._range) return this._range;
  throw new Error("IndexSizeError: Invalid index " + index);
};

Selection.prototype.addRange = function (range) {
  this._range = range;
};

Selection.prototype.removeRange = function (range) {
  if (this._range === range) this._range = null;
};

Selection.prototype.removeAllRanges = function () {
  this._range = null;
};

Selection.prototype.empty = function () {
  this._range = null;
};

Selection.prototype.collapse = function (node, offset) {
  if (!this._range) this._range = new Range();
  this._range.setStart(node, offset);
  this._range.collapse(true);
};

Selection.prototype.setPosition = function (node, offset) {
  this.collapse(node, offset);
};

Selection.prototype.selectAllChildren = function (node) {
  if (!this._range) this._range = new Range();
  this._range.selectNodeContents(node);
};

Selection.prototype.toString = function () {
  return "";
};

markNative(Selection, "Selection");
markNative(Selection.prototype.getRangeAt, "getRangeAt");
markNative(Selection.prototype.addRange, "addRange");
markNative(Selection.prototype.removeAllRanges, "removeAllRanges");
