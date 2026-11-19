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
  if (this._selection) {
    this._selection._onRangeMutated(this);
  }
};

Range.prototype.setEnd = function (node, offset) {
  this.endContainer = node;
  this.endOffset = Number(offset || 0);
  this._updateState();
  if (this._selection) {
    this._selection._onRangeMutated(this);
  }
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
  if (!node) throw new TypeError("Node required");
  this.startContainer = node;
  this.startOffset = 0;
  this.endContainer = node;
  this.endOffset = getNodeLength(node);
  this._updateState();
  if (this._selection) {
    this._selection._onRangeMutated(this);
  }
};

Range.prototype.compareBoundaryPoints = function (how, sourceRange) {
  if (how < 0 || how > 3) {
    throw new DOMException("Invalid compare how type", "NotSupportedError");
  }
  if (!sourceRange) {
    throw new TypeError("sourceRange required");
  }
  let thisNode, thisOffset, sourceNode, sourceOffset;
  if (how === Range.START_TO_START) {
    thisNode = this.startContainer;
    thisOffset = this.startOffset;
    sourceNode = sourceRange.startContainer;
    sourceOffset = sourceRange.startOffset;
  } else if (how === Range.START_TO_END) {
    thisNode = this.endContainer;
    thisOffset = this.endOffset;
    sourceNode = sourceRange.startContainer;
    sourceOffset = sourceRange.startOffset;
  } else if (how === Range.END_TO_END) {
    thisNode = this.endContainer;
    thisOffset = this.endOffset;
    sourceNode = sourceRange.endContainer;
    sourceOffset = sourceRange.endOffset;
  } else if (how === Range.END_TO_START) {
    thisNode = this.startContainer;
    thisOffset = this.startOffset;
    sourceNode = sourceRange.endContainer;
    sourceOffset = sourceRange.endOffset;
  }
  return compareBoundaryPoints(thisNode, thisOffset, sourceNode, sourceOffset);
};

Range.prototype.toString = function () {
  if (this.collapsed) return "";
  if (this.startContainer === this.endContainer && (this.startContainer.nodeType === 3 || this.startContainer.nodeType === 8)) {
    const text = this.startContainer.data || this.startContainer.textContent || "";
    return text.slice(this.startOffset, this.endOffset);
  }
  return "";
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

const _nativeSetInnerHtml = (typeof Element !== "undefined" && Object.getOwnPropertyDescriptor(Element.prototype, "innerHTML"))?.set;
const _nativeCreateElement = (typeof document !== "undefined" && document.createElement) ? document.createElement.bind(document) : null;

Range.prototype.createContextualFragment = function (fragmentHtml) {
  const node = this.startContainer;
  const doc = (node && (node.ownerDocument || (node.nodeType === 9 ? node : null))) || (typeof document !== "undefined" ? document : null);
  if (!doc) {
    throw new TypeError("createContextualFragment: start container has no document");
  }

  // 1. Determine context element
  let element = null;
  if (node && node.nodeType === 1) { // ELEMENT_NODE
    element = node;
  } else if (node && (node.nodeType === 3 || node.nodeType === 8)) { // TEXT_NODE or COMMENT_NODE
    element = node.parentElement || node.parentNode;
    if (element && element.nodeType !== 1) element = null;
  }

  if (!element || String(element.localName || element.tagName || "").toLowerCase() === "html") {
    element = doc.body || (doc.documentElement ? doc.documentElement : null);
  }

  const tag = element ? String(element.localName || element.tagName || "").toLowerCase() : "body";
  const ns = (element && element._namespaceURI) ? element._namespaceURI : "http://www.w3.org/1999/xhtml";

  // 2. Format markup (null becomes "null")
  const markup = fragmentHtml === null ? "null" : String(fragmentHtml);

  // 3. Demand SVG font outlines if SVG text is present
  let inSvg = (tag === "svg" || ns === "http://www.w3.org/2000/svg");
  if (!inSvg && element) {
    let p = element.parentElement || element.parentNode;
    while (p) {
      if (String(p.localName || p.tagName || "").toLowerCase() === "svg") {
        inSvg = true;
        break;
      }
      p = p.parentElement || p.parentNode;
    }
  }

  if (
    markup.length !== 0 &&
    /<text[\s/>]/i.test(markup) &&
    (/<svg[\s/>]/i.test(markup) || inSvg)
  ) {
    try {
      const ksInternals = globalThis[Symbol.for("kitesurf.internals")];
      if (ksInternals && typeof ksInternals.demandFontOutlines === "function") {
        ksInternals.demandFontOutlines();
      }
    } catch (e) {}
  }

  // 4. Wrap markup according to context
  let prefix = "";
  let suffix = "";
  let depth = 0;
  let cleanMarkup = markup;
  const isTableContext = (
    tag === "tbody" || tag === "thead" || tag === "tfoot" ||
    tag === "tr" || tag === "td" || tag === "th" ||
    tag === "col" || tag === "colgroup" || tag === "caption" ||
    tag === "table" || tag === "template"
  );
  if (!isTableContext && /^\s*<tr[\s>]/i.test(markup)) {
    cleanMarkup = markup.replace(/<\/?(?:table|tbody|thead|tfoot|tr|td|th|colgroup|col|caption)[^>]*>/gi, "");
  } else if (tag === "tbody" || tag === "thead" || tag === "tfoot") {
    prefix = "<table><" + tag + ">";
    suffix = "</" + tag + "></table>";
    depth = 2;
  } else if (tag === "tr") {
    prefix = "<table><tbody><tr>";
    suffix = "</tr></tbody></table>";
    depth = 3;
  } else if (tag === "td" || tag === "th") {
    prefix = "<table><tbody><tr><" + tag + ">";
    suffix = "</" + tag + "></tr></tbody></table>";
    depth = 4;
  } else if (tag === "col") {
    prefix = "<table><colgroup>";
    suffix = "</colgroup></table>";
    depth = 2;
  } else if (tag === "colgroup" || tag === "caption") {
    prefix = "<table>";
    suffix = "</table>";
    depth = 1;
  } else if (tag === "template") {
    prefix = "<template>";
    suffix = "</template>";
    depth = 1;
  } else if (tag === "select" || tag === "optgroup") {
    prefix = "<select>";
    suffix = "</select>";
    depth = 1;
  } else if (tag === "svg") {
    prefix = "<svg>";
    suffix = "</svg>";
    depth = 1;
  }

  const fullHtml = prefix + cleanMarkup + suffix;

  // 5. Parse in a temporary container using native innerHTML setter
  const mkEl = _nativeCreateElement || (typeof document !== "undefined" && document.createElement ? document.createElement.bind(document) : null);
  const temp = mkEl ? mkEl("div") : (doc.createElement ? doc.createElement("div") : null);

  if (temp) {
    if (_nativeSetInnerHtml) {
      _nativeSetInnerHtml.call(temp, fullHtml);
    } else {
      temp.innerHTML = fullHtml;
    }
  }

  // 6. Navigate to target at the specified depth
  let target = temp;
  for (let d = 0; d < depth && target; d++) {
    target = target.firstElementChild || target.firstChild;
  }
  if (!target) target = temp;

  let source = target;
  if (target && String(target.localName || target.tagName || "").toLowerCase() === "template") {
    if (target.content && target.content.firstChild) {
      source = target.content;
    }
  }

  // 7. Move nodes to a new DocumentFragment
  const frag = new DocumentFragment();
  frag.ownerDocument = doc;

  while (source && source.firstChild) {
    const ch = source.firstChild;
    frag.appendChild(typeof upgradeElement === "function" ? upgradeElement(ch) : ch);
  }

  // 8. Mark scripts within fragment as executable
  function markExecutable(n) {
    if (!n || typeof n !== "object") return;
    const t = String(n.localName || n.tagName || "").toLowerCase();
    if (t === "script") {
      n._alreadyStarted = false;
    }
    const kids = n.childNodes || n._children;
    if (kids && kids.length) {
      for (let i = 0; i < kids.length; i++) {
        markExecutable(kids[i]);
      }
    }
  }
  markExecutable(frag);

  return frag;
};

if (typeof DocumentFragment !== "undefined" && DocumentFragment.prototype) {
  if (!Object.getOwnPropertyDescriptor(DocumentFragment.prototype, "textContent")) {
    Object.defineProperty(DocumentFragment.prototype, "textContent", {
      get: function () {
        let text = "";
        const kids = this.childNodes || this._children || [];
        for (let i = 0; i < kids.length; i++) {
          const k = kids[i];
          if (k.nodeType === 3) text += k.data || k.textContent || "";
          else if (k.nodeType === 1) text += k.textContent || "";
        }
        return text;
      },
      set: function (val) {
        while (this.firstChild) {
          this.removeChild(this.firstChild);
        }
        if (val) {
          this.appendChild(new Text(val));
        }
      },
      configurable: true,
    });
  }
}

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

function getNodeLength(node) {
  if (!node) return 0;
  if (node.nodeType === 3 || node.nodeType === 8) { // TEXT_NODE or COMMENT_NODE
    if (typeof node.data === "string") return node.data.length;
    if (typeof node.textContent === "string") return node.textContent.length;
    return 0;
  }
  if (node.childNodes) return node.childNodes.length;
  if (node._children) return node._children.length;
  return 0;
}

function isNodeInDocument(node, doc) {
  if (!node) return false;
  if (!doc) doc = typeof document !== "undefined" ? document : null;
  if (!doc) return true;
  if (node === doc) return true;
  let curr = node;
  while (curr) {
    if (curr === doc) return true;
    curr = curr.parentNode || curr.parentElement;
  }
  return false;
}

function compareBoundaryPoints(nodeA, offsetA, nodeB, offsetB) {
  if (nodeA === nodeB) {
    if (offsetA === offsetB) return 0;
    return offsetA < offsetB ? -1 : 1;
  }
  if (nodeA.contains && nodeA.contains(nodeB)) {
    let child = nodeB;
    while (child && (child.parentNode || child.parentElement) !== nodeA) {
      child = child.parentNode || child.parentElement;
    }
    if (child) {
      let idx = 0;
      let sibling = nodeA.firstChild;
      while (sibling && sibling !== child) {
        idx++;
        sibling = sibling.nextSibling;
      }
      if (idx < offsetA) return 1;
      return -1;
    }
  }
  if (nodeB.contains && nodeB.contains(nodeA)) {
    let child = nodeA;
    while (child && (child.parentNode || child.parentElement) !== nodeB) {
      child = child.parentNode || child.parentElement;
    }
    if (child) {
      let idx = 0;
      let sibling = nodeB.firstChild;
      while (sibling && sibling !== child) {
        idx++;
        sibling = sibling.nextSibling;
      }
      if (offsetB <= idx) return 1;
      return -1;
    }
  }
  if (typeof nodeA.compareDocumentPosition === "function") {
    const pos = nodeA.compareDocumentPosition(nodeB);
    if (pos & 4) { // DOCUMENT_POSITION_FOLLOWING: nodeB follows nodeA
      return -1;
    }
    if (pos & 2) { // DOCUMENT_POSITION_PRECEDING: nodeB precedes nodeA
      return 1;
    }
  }
  return 0;
}

let _docSelectionChangeScheduled = false;

function scheduleDocumentSelectionChange(doc) {
  if (_docSelectionChangeScheduled) return;
  _docSelectionChangeScheduled = true;
  setTimeout(function () {
    _docSelectionChangeScheduled = false;
    const targetDoc = doc || (typeof document !== "undefined" ? document : null);
    if (targetDoc && typeof targetDoc.dispatchEvent === "function") {
      try {
        targetDoc.dispatchEvent(new Event("selectionchange"));
      } catch (e) {}
    }
  }, 0);
}

function Selection(doc) {
  this._doc = doc || (typeof document !== "undefined" ? document : null);
  this._range = null;
  this._direction = "none";
}

Selection.prototype._live = function () {
  if (!this._range) return null;
  const doc = this._doc || (typeof document !== "undefined" ? document : null);
  if (this._range.startContainer && !isNodeInDocument(this._range.startContainer, doc)) {
    if (this._range._selection === this) this._range._selection = null;
    this._range = null;
    this._direction = "none";
    return null;
  }
  if (this._range.endContainer && !isNodeInDocument(this._range.endContainer, doc)) {
    if (this._range._selection === this) this._range._selection = null;
    this._range = null;
    this._direction = "none";
    return null;
  }
  return this._range;
};

Selection.prototype._scheduleChange = function () {
  scheduleDocumentSelectionChange(this._doc);
};

Selection.prototype._onRangeMutated = function (range) {
  if (range === this._range) {
    this._scheduleChange();
  }
};

Object.defineProperty(Selection.prototype, "rangeCount", {
  get: function () {
    return this._live() ? 1 : 0;
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "isCollapsed", {
  get: function () {
    const r = this._live();
    return r ? r.collapsed : true;
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "type", {
  get: function () {
    const r = this._live();
    if (!r) return "None";
    return r.collapsed ? "Caret" : "Range";
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "direction", {
  get: function () {
    if (!this._live()) return "none";
    return this._direction || "none";
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "anchorNode", {
  get: function () {
    const r = this._live();
    if (!r) return null;
    return this._direction === "backward" ? r.endContainer : r.startContainer;
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "anchorOffset", {
  get: function () {
    const r = this._live();
    if (!r) return 0;
    return this._direction === "backward" ? r.endOffset : r.startOffset;
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "focusNode", {
  get: function () {
    const r = this._live();
    if (!r) return null;
    return this._direction === "backward" ? r.startContainer : r.endContainer;
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "focusOffset", {
  get: function () {
    const r = this._live();
    if (!r) return 0;
    return this._direction === "backward" ? r.startOffset : r.endOffset;
  },
  configurable: true,
  enumerable: true,
});

Selection.prototype.getRangeAt = function (index) {
  const r = this._live();
  if (!r || index >>> 0 !== 0) {
    throw new DOMException("Selection.getRangeAt: index out of range", "IndexSizeError");
  }
  return r;
};

Selection.prototype.addRange = function (range) {
  if (!range) return;
  const doc = this._doc || (typeof document !== "undefined" ? document : null);
  if (!isNodeInDocument(range.startContainer, doc) || !isNodeInDocument(range.endContainer, doc)) {
    return;
  }
  if (this._live()) return;
  if (this._range && this._range._selection === this) {
    this._range._selection = null;
  }
  this._range = range;
  range._selection = this;
  this._direction = range.collapsed ? "none" : "forward";
  this._scheduleChange();
};

Selection.prototype.removeRange = function (range) {
  const current = this._live();
  if (current !== range) {
    throw new DOMException("removeRange: the given Range is not in the selection", "NotFoundError");
  }
  if (this._range && this._range._selection === this) {
    this._range._selection = null;
  }
  this._range = null;
  this._direction = "none";
  this._scheduleChange();
};

Selection.prototype.removeAllRanges = function () {
  const hadRange = this._range !== null;
  if (this._range && this._range._selection === this) {
    this._range._selection = null;
  }
  this._range = null;
  this._direction = "none";
  if (hadRange) this._scheduleChange();
};

Selection.prototype.empty = function () {
  this.removeAllRanges();
};

Selection.prototype.collapse = function (node, offset) {
  if (node === null || node === undefined) {
    this.removeAllRanges();
    return;
  }
  const o = offset === undefined ? 0 : Number(offset);
  if (o < 0) {
    throw new DOMException("collapse: offset out of bounds", "IndexSizeError");
  }
  const maxLen = getNodeLength(node);
  if (o > maxLen) {
    throw new DOMException("collapse: offset out of bounds", "IndexSizeError");
  }
  const doc = this._doc || (typeof document !== "undefined" ? document : null);
  if (!isNodeInDocument(node, doc)) {
    // detached node: abort silently
    return;
  }
  const r = new Range();
  r.setStart(node, o);
  r.setEnd(node, o);
  if (this._range && this._range._selection === this) {
    this._range._selection = null;
  }
  this._range = r;
  r._selection = this;
  this._direction = "none";
  this._scheduleChange();
};

Selection.prototype.setPosition = function (node, offset) {
  this.collapse(node, offset);
};

Selection.prototype.collapseToStart = function () {
  const r = this._live();
  if (!r) throw new DOMException("collapseToStart: there is no selection", "InvalidStateError");
  this.collapse(r.startContainer, r.startOffset);
};

Selection.prototype.collapseToEnd = function () {
  const r = this._live();
  if (!r) throw new DOMException("collapseToEnd: there is no selection", "InvalidStateError");
  this.collapse(r.endContainer, r.endOffset);
};

Selection.prototype.extend = function (node, offset) {
  if (!node) throw new TypeError("extend: node required");
  const r = this._live();
  if (!r) throw new DOMException("extend: there is no selection", "InvalidStateError");
  const o = offset === undefined ? 0 : Number(offset);
  if (o < 0) {
    throw new DOMException("extend: offset out of bounds", "IndexSizeError");
  }
  const maxLen = getNodeLength(node);
  if (o > maxLen) {
    throw new DOMException("extend: offset out of bounds", "IndexSizeError");
  }
  const doc = this._doc || (typeof document !== "undefined" ? document : null);
  if (!isNodeInDocument(node, doc)) {
    return;
  }

  const anchorNode = this.anchorNode;
  const anchorOffset = this.anchorOffset;
  if (!anchorNode) return;

  const cmp = compareBoundaryPoints(anchorNode, anchorOffset, node, o);
  let isBackward = false;
  let startNode, startOffset, endNode, endOffset;
  if (cmp > 0) {
    isBackward = true;
    startNode = node;
    startOffset = o;
    endNode = anchorNode;
    endOffset = anchorOffset;
  } else if (cmp < 0) {
    isBackward = false;
    startNode = anchorNode;
    startOffset = anchorOffset;
    endNode = node;
    endOffset = o;
  } else {
    isBackward = false;
    startNode = anchorNode;
    startOffset = anchorOffset;
    endNode = node;
    endOffset = o;
  }

  r.setStart(startNode, startOffset);
  r.setEnd(endNode, endOffset);
  this._direction = (startNode === endNode && startOffset === endOffset) ? "none" : (isBackward ? "backward" : "forward");
  this._scheduleChange();
};

Selection.prototype.setBaseAndExtent = function (anchorNode, anchorOffset, focusNode, focusOffset) {
  if (!anchorNode || !focusNode) throw new TypeError("setBaseAndExtent: nodes required");
  const ao = anchorOffset === undefined ? 0 : Number(anchorOffset);
  const fo = focusOffset === undefined ? 0 : Number(focusOffset);
  if (ao < 0 || ao > getNodeLength(anchorNode)) {
    throw new DOMException("setBaseAndExtent: anchor offset out of bounds", "IndexSizeError");
  }
  if (fo < 0 || fo > getNodeLength(focusNode)) {
    throw new DOMException("setBaseAndExtent: focus offset out of bounds", "IndexSizeError");
  }
  const doc = this._doc || (typeof document !== "undefined" ? document : null);
  if (!isNodeInDocument(anchorNode, doc) || !isNodeInDocument(focusNode, doc)) {
    return;
  }
  const cmp = compareBoundaryPoints(anchorNode, ao, focusNode, fo);
  let isBackward = false;
  let startNode, startOffset, endNode, endOffset;
  if (cmp > 0) {
    isBackward = true;
    startNode = focusNode;
    startOffset = fo;
    endNode = anchorNode;
    endOffset = ao;
  } else {
    isBackward = false;
    startNode = anchorNode;
    startOffset = ao;
    endNode = focusNode;
    endOffset = fo;
  }
  const r = new Range();
  r.setStart(startNode, startOffset);
  r.setEnd(endNode, endOffset);
  if (this._range && this._range._selection === this) {
    this._range._selection = null;
  }
  this._range = r;
  r._selection = this;
  this._direction = (startNode === endNode && startOffset === endOffset) ? "none" : (isBackward ? "backward" : "forward");
  this._scheduleChange();
};

Selection.prototype.selectAllChildren = function (node) {
  if (!node) throw new TypeError("selectAllChildren: node required");
  const doc = this._doc || (typeof document !== "undefined" ? document : null);
  if (!isNodeInDocument(node, doc)) return;
  const childCount = node.childNodes ? node.childNodes.length : 0;
  const r = new Range();
  r.setStart(node, 0);
  r.setEnd(node, childCount);
  if (this._range && this._range._selection === this) {
    this._range._selection = null;
  }
  this._range = r;
  r._selection = this;
  this._direction = "none";
  this._scheduleChange();
};

Selection.prototype.containsNode = function (node, allowPartialContainment) {
  const r = this._live();
  if (!r || !node) return false;
  const doc = this._doc || (typeof document !== "undefined" ? document : null);
  if (!isNodeInDocument(node, doc)) return false;
  const len = getNodeLength(node);
  const nodeStartCmp = compareBoundaryPoints(node, 0, r.endContainer, r.endOffset);
  const nodeEndCmp = compareBoundaryPoints(node, len, r.startContainer, r.startOffset);
  if (allowPartialContainment) {
    return nodeStartCmp <= 0 && nodeEndCmp >= 0;
  }
  return compareBoundaryPoints(node, 0, r.startContainer, r.startOffset) >= 0 &&
         compareBoundaryPoints(node, len, r.endContainer, r.endOffset) <= 0;
};

Selection.prototype.toString = function () {
  const r = this._live();
  if (!r) return "";
  return r.toString ? r.toString() : "";
};

Range.prototype.getClientRects = function () {
  const rects = [];
  if (this.collapsed) return rects;
  const startNode = this.startContainer;
  const endNode = this.endContainer;
  if (!startNode || !endNode) return rects;

  const doc = (startNode && (startNode.ownerDocument || (startNode.nodeType === 9 ? startNode : null))) || (typeof document !== "undefined" ? document : null);
  const root = doc ? (doc.body || doc.documentElement) : null;
  if (!root) return rects;

  const range = this;
  function walk(node) {
    if (!node) return;

    if (node.nodeType === 3) { // TEXT_NODE
      const text = node.data || node.textContent || "";
      const len = text.length;
      if (len === 0) return;

      if (compareBoundaryPoints(node, 0, range.endContainer, range.endOffset) >= 0) {
        return;
      }
      if (compareBoundaryPoints(node, len, range.startContainer, range.startOffset) <= 0) {
        return;
      }

      let s = 0;
      let e = len;
      if (node === range.startContainer) {
        s = Math.max(0, Math.min(len, range.startOffset));
      }
      if (node === range.endContainer) {
        e = Math.max(0, Math.min(len, range.endOffset));
      }
      if (s < e) {
        let nodeRect = null;
        if (typeof node.getBoundingClientRect === "function") {
          nodeRect = node.getBoundingClientRect();
        }
        if (!nodeRect || (!nodeRect.width && !nodeRect.height)) {
          if (node.parentNode && typeof node.parentNode.getBoundingClientRect === "function") {
            nodeRect = node.parentNode.getBoundingClientRect();
          }
        }
        if (nodeRect) {
          const rx = (nodeRect.left !== undefined ? nodeRect.left : (nodeRect.x || 0)) + 2 + s * 8;
          const ry = (nodeRect.top !== undefined ? nodeRect.top : (nodeRect.y || 0)) + 2;
          const rw = (e - s) * 8;
          const rh = (nodeRect.height && nodeRect.height >= 10) ? nodeRect.height : 12;
          rects.push({
            x: rx,
            y: ry,
            width: rw,
            height: rh,
            top: ry,
            left: rx,
            bottom: ry + rh,
            right: rx + rw,
          });
        }
      }
      return;
    }

    const kids = node.childNodes || node._children || [];
    for (let i = 0; i < kids.length; i++) {
      walk(kids[i]);
    }
  }

  walk(root);
  return rects;
};

Range.prototype.getBoundingClientRect = function () {
  const rects = this.getClientRects();
  if (!rects || rects.length === 0) {
    return { x: 0, y: 0, width: 0, height: 0, top: 0, bottom: 0, left: 0, right: 0 };
  }
  let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
  for (let i = 0; i < rects.length; i++) {
    const r = rects[i];
    if (r.left < minX) minX = r.left;
    if (r.top < minY) minY = r.top;
    if (r.right > maxX) maxX = r.right;
    if (r.bottom > maxY) maxY = r.bottom;
  }
  return {
    x: minX,
    y: minY,
    width: Math.max(0, maxX - minX),
    height: Math.max(0, maxY - minY),
    top: minY,
    bottom: maxY,
    left: minX,
    right: maxX,
  };
};

function __capsurf_get_selection_rects() {
  const sel = (typeof window !== "undefined" && window.getSelection) ? window.getSelection() : (typeof document !== "undefined" && document.getSelection ? document.getSelection() : null);
  if (!sel || sel.rangeCount === 0 || sel.isCollapsed) return "";
  let range = null;
  try {
    range = sel.getRangeAt(0);
  } catch (e) {
    return "";
  }
  if (!range || range.collapsed) return "";
  const rects = range.getClientRects();
  if (!rects || rects.length === 0) return "";
  const parts = [];
  for (let i = 0; i < rects.length; i++) {
    const r = rects[i];
    parts.push(r.x + ":" + r.y + ":" + r.width + ":" + r.height);
  }
  return parts.join(";");
}

if (typeof globalThis !== "undefined") {
  globalThis.__capsurf_get_selection_rects = __capsurf_get_selection_rects;
}
if (typeof global !== "undefined") {
  global.__capsurf_get_selection_rects = __capsurf_get_selection_rects;
}

markNative(Range.prototype.getClientRects, "getClientRects");
markNative(Range.prototype.getBoundingClientRect, "getBoundingClientRect");

markNative(Selection, "Selection");
markNative(Selection.prototype.getRangeAt, "getRangeAt");
markNative(Selection.prototype.addRange, "addRange");
markNative(Selection.prototype.removeRange, "removeRange");
markNative(Selection.prototype.removeAllRanges, "removeAllRanges");
markNative(Selection.prototype.empty, "empty");
markNative(Selection.prototype.collapse, "collapse");
markNative(Selection.prototype.setPosition, "setPosition");
markNative(Selection.prototype.collapseToStart, "collapseToStart");
markNative(Selection.prototype.collapseToEnd, "collapseToEnd");
markNative(Selection.prototype.extend, "extend");
markNative(Selection.prototype.setBaseAndExtent, "setBaseAndExtent");
markNative(Selection.prototype.selectAllChildren, "selectAllChildren");
markNative(Selection.prototype.containsNode, "containsNode");
markNative(Selection.prototype.toString, "toString");
