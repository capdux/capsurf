// src/js/shim/range.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/range.ts

const _liveRanges = [];
const _rangeState = new WeakMap();

function AbstractRange() {
  throw new TypeError("Illegal constructor");
}
AbstractRange.prototype = {};
Object.defineProperty(AbstractRange.prototype, "constructor", { value: AbstractRange, configurable: true, writable: true });
["startContainer", "startOffset", "endContainer", "endOffset", "collapsed"].forEach(function (prop) {
  Object.defineProperty(AbstractRange.prototype, prop, {
    get: function () {
      const st = _rangeState.get(this);
      if (!st) throw new TypeError("Illegal invocation");
      return st[prop];
    },
    set: function (v) {
      const st = _rangeState.get(this);
      if (st) st[prop] = v;
    },
    configurable: true,
    enumerable: true,
  });
});
markNative(AbstractRange, "AbstractRange");

function StaticRange(init) {
  if (!init || typeof init !== "object") {
    throw new TypeError("Failed to construct 'StaticRange': 1 argument required.");
  }
  const sc = init.startContainer;
  const ec = init.endContainer;
  if (!sc || typeof sc.nodeType !== "number" || !ec || typeof ec.nodeType !== "number") {
    throw new TypeError("Failed to construct 'StaticRange': startContainer and endContainer must be Nodes.");
  }
  if (init.startOffset === undefined || init.endOffset === undefined) {
    throw new TypeError("Failed to construct 'StaticRange': startOffset and endOffset are required.");
  }
  if (sc.nodeType === 10 || sc.nodeType === 2 || ec.nodeType === 10 || ec.nodeType === 2) {
    throw new DOMException("The node provided is of an invalid type.", "InvalidNodeTypeError");
  }
  const so = Number(init.startOffset) >>> 0;
  const eo = Number(init.endOffset) >>> 0;
  _rangeState.set(this, {
    startContainer: sc,
    startOffset: so,
    endContainer: ec,
    endOffset: eo,
    collapsed: sc === ec && so === eo,
  });
}
StaticRange.prototype = Object.create(AbstractRange.prototype);
StaticRange.prototype.constructor = StaticRange;
markNative(StaticRange, "StaticRange");

function Range(ownerDoc) {
  const doc = (ownerDoc && typeof ownerDoc.nodeType === "number")
    ? ownerDoc
    : (typeof document !== "undefined" ? document : null);
  _rangeState.set(this, {
    startContainer: doc,
    startOffset: 0,
    endContainer: doc,
    endOffset: 0,
    collapsed: true,
    commonAncestorContainer: doc,
  });
  _liveRanges.push(this);
}
Range.prototype = Object.create(AbstractRange.prototype);
Range.prototype.constructor = Range;
function isInclusiveAncestorNode(a, b) {
  let cur = b;
  while (cur) {
    if (cur === a) return true;
    cur = cur.parentNode;
  }
  return false;
}

Object.defineProperty(Range.prototype, "commonAncestorContainer", {
  get: function () {
    const st = _rangeState.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    let container = st.startContainer;
    const end = st.endContainer;
    while (container && !isInclusiveAncestorNode(container, end)) {
      container = container.parentNode;
    }
    return container || st.startContainer;
  },
  set: function (v) {
    const st = _rangeState.get(this);
    if (st) st.commonAncestorContainer = v;
  },
  configurable: true,
  enumerable: true,
});

function getChildIndexInParent(node) {
  if (!node || !node.parentNode) return -1;
  const parent = node.parentNode;
  const kids = parent._children || (parent.childNodes ? Array.from(parent.childNodes) : []);
  return kids.indexOf(node);
}

global.__capsurf_rangeOnReplaceData = function (node, offset, count, insertedLen) {
  for (let i = 0; i < _liveRanges.length; i++) {
    const r = _liveRanges[i];
    if (!r) continue;
    let changed = false;
    if (r.startContainer === node) {
      if (r.startOffset > offset && r.startOffset <= offset + count) {
        r.startOffset = offset;
        changed = true;
      } else if (r.startOffset > offset + count) {
        r.startOffset = r.startOffset + insertedLen - count;
        changed = true;
      }
    }
    if (r.endContainer === node) {
      if (r.endOffset > offset && r.endOffset <= offset + count) {
        r.endOffset = offset;
        changed = true;
      } else if (r.endOffset > offset + count) {
        r.endOffset = r.endOffset + insertedLen - count;
        changed = true;
      }
    }
    if (changed) {
      r._updateState();
    }
  }
};

global.__capsurf_rangeOnSplitText = function (oldNode, newNode, offset, parent, nodeIndex) {
  for (let i = 0; i < _liveRanges.length; i++) {
    const r = _liveRanges[i];
    if (!r) continue;
    let changed = false;
    if (r.startContainer === oldNode && r.startOffset > offset) {
      r.startContainer = newNode;
      r.startOffset = r.startOffset - offset;
      changed = true;
    }
    if (r.endContainer === oldNode && r.endOffset > offset) {
      r.endContainer = newNode;
      r.endOffset = r.endOffset - offset;
      changed = true;
    }
    if (parent && nodeIndex >= 0) {
      if (r.startContainer === parent && r.startOffset >= nodeIndex + 1) {
        r.startOffset += 1;
        changed = true;
      }
      if (r.endContainer === parent && r.endOffset >= nodeIndex + 1) {
        r.endOffset += 1;
        changed = true;
      }
    }
    if (changed) {
      r._updateState();
    }
  }
};

global.__capsurf_rangeOnNodeInserted = function (parent, insertIndex, count) {
  const delta = count || 1;
  for (let i = 0; i < _liveRanges.length; i++) {
    const r = _liveRanges[i];
    if (!r) continue;
    let changed = false;
    if (r.startContainer === parent && r.startOffset > insertIndex) {
      r.startOffset += delta;
      changed = true;
    }
    if (r.endContainer === parent && r.endOffset > insertIndex) {
      r.endOffset += delta;
      changed = true;
    }
    if (changed) {
      r._updateState();
    }
  }
};

global.__capsurf_rangeOnNodeRemoved = function (parent, removedNode, removedIndex) {
  for (let i = 0; i < _liveRanges.length; i++) {
    const r = _liveRanges[i];
    if (!r) continue;
    let changed = false;
    if (r.startContainer && (r.startContainer === removedNode || (removedNode.contains && removedNode.contains(r.startContainer)))) {
      r.startContainer = parent;
      r.startOffset = removedIndex;
      changed = true;
    } else if (r.startContainer === parent && r.startOffset > removedIndex) {
      r.startOffset -= 1;
      changed = true;
    }
    if (r.endContainer && (r.endContainer === removedNode || (removedNode.contains && removedNode.contains(r.endContainer)))) {
      r.endContainer = parent;
      r.endOffset = removedIndex;
      changed = true;
    } else if (r.endContainer === parent && r.endOffset > removedIndex) {
      r.endOffset -= 1;
      changed = true;
    }
    if (changed) {
      r._updateState();
    }
  }
};

Range.START_TO_START = 0;
Range.START_TO_END = 1;
Range.END_TO_END = 2;
Range.END_TO_START = 3;

Range.prototype.setStart = function (node, offset) {
  this.startContainer = node;
  this.startOffset = Number(offset || 0);
  this._updateState();
  if (this._selection) {
    selOnRangeMutated(this._selection, this);
  }
};

Range.prototype.setEnd = function (node, offset) {
  this.endContainer = node;
  this.endOffset = Number(offset || 0);
  this._updateState();
  if (this._selection) {
    selOnRangeMutated(this._selection, this);
  }
};

Range.prototype.setStartBefore = function (node) {
  if (!node || !node.parentNode) throw new Error("Parent node required");
  const idx = getChildIndexInParent(node);
  this.setStart(node.parentNode, idx >= 0 ? idx : 0);
};

Range.prototype.setStartAfter = function (node) {
  if (!node || !node.parentNode) throw new Error("Parent node required");
  const idx = getChildIndexInParent(node);
  this.setStart(node.parentNode, idx >= 0 ? idx + 1 : 1);
};

Range.prototype.setEndBefore = function (node) {
  if (!node || !node.parentNode) throw new Error("Parent node required");
  const idx = getChildIndexInParent(node);
  this.setEnd(node.parentNode, idx >= 0 ? idx : 0);
};

Range.prototype.setEndAfter = function (node) {
  if (!node || !node.parentNode) throw new Error("Parent node required");
  const idx = getChildIndexInParent(node);
  this.setEnd(node.parentNode, idx >= 0 ? idx + 1 : 1);
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
    selOnRangeMutated(this._selection, this);
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
    const text = this.startContainer.data !== undefined ? String(this.startContainer.data) : String(this.startContainer.textContent || "");
    return text.slice(this.startOffset, this.endOffset);
  }
  if (this.startContainer === this.endContainer && this.startContainer.nodeType === 1) {
    const cs = this.startContainer.childNodes || [];
    let out = "";
    const endIdx = Math.min(this.endOffset, cs.length);
    for (let i = Math.max(0, this.startOffset); i < endIdx; i++) {
      if (cs[i]) out += String(cs[i].nodeType === 3 ? (cs[i].data !== undefined ? cs[i].data : (cs[i].textContent || "")) : (cs[i].textContent || ""));
    }
    return out;
  }
  const root = this.commonAncestorContainer;
  if (!root) return "";
  let out = "";
  const sc = this.startContainer;
  const so = this.startOffset;
  const ec = this.endContainer;
  const eo = this.endOffset;
  function walk(n) {
    if (!n) return;
    if (n.nodeType === 3) {
      const txt = n.data !== undefined ? String(n.data) : String(n.textContent || "");
      const sCmp = compareBoundaryPoints(n, 0, sc, so);
      const eCmp = compareBoundaryPoints(n, txt.length, ec, eo);
      if (n === sc && n === ec) {
        out += txt.slice(so, eo);
      } else if (n === sc) {
        out += txt.slice(so);
      } else if (n === ec) {
        out += txt.slice(0, eo);
      } else if (sCmp >= 0 && eCmp <= 0) {
        out += txt;
      } else if (compareBoundaryPoints(n, txt.length, sc, so) > 0 && compareBoundaryPoints(n, 0, ec, eo) < 0) {
        out += txt;
      }
      return;
    }
    const cs = n.childNodes || [];
    for (let i = 0; i < cs.length; i++) walk(cs[i]);
  }
  walk(root);
  return out;
};

Range.prototype.deleteContents = function () {
  if (this.collapsed || !this.startContainer || !this.endContainer) return;
  const sc = this.startContainer;
  const so = this.startOffset;
  const ec = this.endContainer;
  const eo = this.endOffset;
  if (sc === ec) {
    if (sc.nodeType === 3) {
      const txt = String(sc.data !== undefined ? sc.data : (sc.textContent || ""));
      const next = txt.slice(0, so) + txt.slice(eo);
      if (typeof sc.data === "string") sc.data = next;
      else sc.textContent = next;
    } else if (sc.childNodes && sc.removeChild) {
      for (let i = eo - 1; i >= so; i--) {
        const ch = sc.childNodes[i];
        if (ch) sc.removeChild(ch);
      }
    }
    this.setEnd(sc, so);
    return;
  }
  const root = this.commonAncestorContainer;
  if (!root) return;
  const toRemove = [];
  function collect(n) {
    if (!n) return;
    if (n.nodeType === 3) {
      const txt = String(n.data !== undefined ? n.data : (n.textContent || ""));
      if (n === sc) {
        const next = txt.slice(0, so);
        if (typeof n.data === "string") n.data = next;
        else n.textContent = next;
      } else if (n === ec) {
        const next = txt.slice(eo);
        if (typeof n.data === "string") n.data = next;
        else n.textContent = next;
      } else if (compareBoundaryPoints(n, txt.length, sc, so) > 0 && compareBoundaryPoints(n, 0, ec, eo) < 0) {
        toRemove.push(n);
      }
      return;
    }
    const cs = n.childNodes ? Array.prototype.slice.call(n.childNodes) : [];
    for (let i = 0; i < cs.length; i++) collect(cs[i]);
    if (n !== root && n !== sc && n !== ec && !n.contains(sc) && !n.contains(ec)) {
      if (compareBoundaryPoints(n, 0, sc, so) >= 0 && compareBoundaryPoints(n, 0, ec, eo) < 0) {
        toRemove.push(n);
      }
    }
  }
  collect(root);
  for (let i = 0; i < toRemove.length; i++) {
    const node = toRemove[i];
    if (node.parentNode && node.parentNode.removeChild) {
      node.parentNode.removeChild(node);
    }
  }
  this.collapse(true);
};
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
    const rawSet = (typeof globalThis !== "undefined" && globalThis.__capsurf_raw_setInnerHTML) || _nativeSetInnerHtml;
    if (rawSet) {
      rawSet.call(temp, fullHtml);
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
    if (typeof source.removeChild === "function") {
      try { source.removeChild(ch); } catch (e) {}
    }
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
  if (node.nodeType === 3 || node.nodeType === 4 || node.nodeType === 7 || node.nodeType === 8) {
    if (typeof node.data === "string") return node.data.length;
    if (typeof node.textContent === "string") return node.textContent.length;
    return 0;
  }
  if (node.childNodes) return node.childNodes.length;
  if (node._children) return node._children.length;
  return 0;
}

function getRangeRoot(node) {
  let cur = node;
  while (cur && cur.parentNode) {
    cur = cur.parentNode;
  }
  return cur;
}

function isNodeInDocument(node, doc) {
  if (!node) return false;
  if (!doc) doc = typeof document !== "undefined" ? document : null;
  if (!doc) return true;
  if (node === doc) return true;
  let curr = node;
  while (curr) {
    if (curr === doc) return true;
    curr = curr.parentNode || curr.parentElement || (curr.nodeType === 11 ? curr.host : null);
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

const _selState = new WeakMap();

function selLive(sel) {
  const st = _selState.get(sel);
  if (!st || !st.range) return null;
  const doc = st.doc || (typeof document !== "undefined" ? document : null);
  if (st.range.startContainer && !isNodeInDocument(st.range.startContainer, doc)) {
    if (st.range._selection === sel) st.range._selection = null;
    st.range = null;
    st.direction = "none";
    return null;
  }
  if (st.range.endContainer && !isNodeInDocument(st.range.endContainer, doc)) {
    if (st.range._selection === sel) st.range._selection = null;
    st.range = null;
    st.direction = "none";
    return null;
  }
  return st.range;
}

function selScheduleChange(sel) {
  const st = _selState.get(sel);
  scheduleDocumentSelectionChange(st ? st.doc : null);
}

function selOnRangeMutated(sel, range) {
  const st = _selState.get(sel);
  if (st && range === st.range) {
    selScheduleChange(sel);
  }
}

function Selection(doc) {
  _selState.set(this, {
    doc: doc || (typeof document !== "undefined" ? document : null),
    range: null,
    direction: "none",
  });
}

Object.defineProperty(Selection.prototype, "rangeCount", {
  get: function () {
    return selLive(this) ? 1 : 0;
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "isCollapsed", {
  get: function () {
    const r = selLive(this);
    return r ? r.collapsed : true;
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "type", {
  get: function () {
    const r = selLive(this);
    if (!r) return "None";
    return r.collapsed ? "Caret" : "Range";
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "direction", {
  get: function () {
    if (!selLive(this)) return "none";
    const st = _selState.get(this);
    return (st && st.direction) || "none";
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "anchorNode", {
  get: function () {
    const r = selLive(this);
    if (!r) return null;
    const st = _selState.get(this);
    return st && st.direction === "backward" ? r.endContainer : r.startContainer;
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "anchorOffset", {
  get: function () {
    const r = selLive(this);
    if (!r) return 0;
    const st = _selState.get(this);
    return st && st.direction === "backward" ? r.endOffset : r.startOffset;
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "focusNode", {
  get: function () {
    const r = selLive(this);
    if (!r) return null;
    const st = _selState.get(this);
    return st && st.direction === "backward" ? r.startContainer : r.endContainer;
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Selection.prototype, "focusOffset", {
  get: function () {
    const r = selLive(this);
    if (!r) return 0;
    const st = _selState.get(this);
    return st && st.direction === "backward" ? r.startOffset : r.endOffset;
  },
  configurable: true,
  enumerable: true,
});

Selection.prototype.getRangeAt = function (index) {
  const r = selLive(this);
  if (!r || index >>> 0 !== 0) {
    throw new DOMException("Selection.getRangeAt: index out of range", "IndexSizeError");
  }
  return r;
};

Selection.prototype.addRange = function (range) {
  if (!range) return;
  const st = _selState.get(this);
  if (!st) return;
  const doc = st.doc || (typeof document !== "undefined" ? document : null);
  if (!isNodeInDocument(range.startContainer, doc) || !isNodeInDocument(range.endContainer, doc)) {
    return;
  }
  if (selLive(this)) return;
  if (st.range && st.range._selection === this) {
    st.range._selection = null;
  }
  st.range = range;
  range._selection = this;
  st.direction = range.collapsed ? "none" : "forward";
  selScheduleChange(this);
};

Selection.prototype.removeRange = function (range) {
  const st = _selState.get(this);
  const current = selLive(this);
  if (!st || current !== range) {
    throw new DOMException("removeRange: the given Range is not in the selection", "NotFoundError");
  }
  if (st.range && st.range._selection === this) {
    st.range._selection = null;
  }
  st.range = null;
  st.direction = "none";
  selScheduleChange(this);
};

Selection.prototype.removeAllRanges = function () {
  const st = _selState.get(this);
  if (!st) return;
  const hadRange = st.range !== null;
  if (st.range && st.range._selection === this) {
    st.range._selection = null;
  }
  st.range = null;
  st.direction = "none";
  if (hadRange) selScheduleChange(this);
};

Selection.prototype.empty = function () {
  this.removeAllRanges();
};

Selection.prototype.collapse = function (node, offset) {
  const st = _selState.get(this);
  if (!st) return;
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
  const doc = st.doc || (typeof document !== "undefined" ? document : null);
  if (!isNodeInDocument(node, doc)) {
    return;
  }
  const r = new Range();
  r.setStart(node, o);
  r.setEnd(node, o);
  if (st.range && st.range._selection === this) {
    st.range._selection = null;
  }
  st.range = r;
  r._selection = this;
  st.direction = "none";
  selScheduleChange(this);
};

Selection.prototype.setPosition = function (node, offset) {
  this.collapse(node, offset);
};

Selection.prototype.collapseToStart = function () {
  const r = selLive(this);
  if (!r) throw new DOMException("collapseToStart: there is no selection", "InvalidStateError");
  this.collapse(r.startContainer, r.startOffset);
};

Selection.prototype.collapseToEnd = function () {
  const r = selLive(this);
  if (!r) throw new DOMException("collapseToEnd: there is no selection", "InvalidStateError");
  this.collapse(r.endContainer, r.endOffset);
};

Selection.prototype.extend = function (node, offset) {
  if (!node) throw new TypeError("extend: node required");
  const st = _selState.get(this);
  const r = selLive(this);
  if (!st || !r) throw new DOMException("extend: there is no selection", "InvalidStateError");
  const o = offset === undefined ? 0 : Number(offset);
  if (o < 0) {
    throw new DOMException("extend: offset out of bounds", "IndexSizeError");
  }
  const maxLen = getNodeLength(node);
  if (o > maxLen) {
    throw new DOMException("extend: offset out of bounds", "IndexSizeError");
  }
  const doc = st.doc || (typeof document !== "undefined" ? document : null);
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
  } else {
    isBackward = false;
    startNode = anchorNode;
    startOffset = anchorOffset;
    endNode = node;
    endOffset = o;
  }

  r.setStart(startNode, startOffset);
  r.setEnd(endNode, endOffset);
  st.direction = (startNode === endNode && startOffset === endOffset) ? "none" : (isBackward ? "backward" : "forward");
  selScheduleChange(this);
};

Selection.prototype.setBaseAndExtent = function (anchorNode, anchorOffset, focusNode, focusOffset) {
  if (!anchorNode || !focusNode) throw new TypeError("setBaseAndExtent: nodes required");
  const st = _selState.get(this);
  if (!st) return;
  const ao = anchorOffset === undefined ? 0 : Number(anchorOffset);
  const fo = focusOffset === undefined ? 0 : Number(focusOffset);
  if (ao < 0 || ao > getNodeLength(anchorNode)) {
    throw new DOMException("setBaseAndExtent: anchor offset out of bounds", "IndexSizeError");
  }
  if (fo < 0 || fo > getNodeLength(focusNode)) {
    throw new DOMException("setBaseAndExtent: focus offset out of bounds", "IndexSizeError");
  }
  const doc = st.doc || (typeof document !== "undefined" ? document : null);
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
  if (st.range && st.range._selection === this) {
    st.range._selection = null;
  }
  st.range = r;
  r._selection = this;
  st.direction = (startNode === endNode && startOffset === endOffset) ? "none" : (isBackward ? "backward" : "forward");
  selScheduleChange(this);
};

Selection.prototype.selectAllChildren = function (node) {
  if (!node) throw new TypeError("selectAllChildren: node required");
  const st = _selState.get(this);
  if (!st) return;
  const doc = st.doc || (typeof document !== "undefined" ? document : null);
  if (!isNodeInDocument(node, doc)) return;
  const childCount = node.childNodes ? node.childNodes.length : 0;
  const r = new Range();
  r.setStart(node, 0);
  r.setEnd(node, childCount);
  if (st.range && st.range._selection === this) {
    st.range._selection = null;
  }
  st.range = r;
  r._selection = this;
  st.direction = "none";
  selScheduleChange(this);
};

Selection.prototype.containsNode = function (node, allowPartialContainment) {
  const st = _selState.get(this);
  const r = selLive(this);
  if (!st || !r || !node) return false;
  const doc = st.doc || (typeof document !== "undefined" ? document : null);
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
  const r = selLive(this);
  if (r && !r.collapsed) {
    const isHtmlInert = function (node) {
      let cur = node;
      while (cur) {
        if (cur.nodeType === 1) {
          const ns = cur.namespaceURI;
          if ((!ns || ns === "http://www.w3.org/1999/xhtml") && typeof cur.hasAttribute === "function" && cur.hasAttribute("inert")) {
            return true;
          }
        }
        cur = cur.parentNode;
      }
      return false;
    };
    const root = r.commonAncestorContainer;
    if (root) {
      let out = "";
      const sc = r.startContainer, so = r.startOffset;
      const ec = r.endContainer, eo = r.endOffset;
      const walkNonInert = function (n) {
        if (!n) return;
        if (n.nodeType === 1) {
          const ns = n.namespaceURI;
          if ((!ns || ns === "http://www.w3.org/1999/xhtml") && typeof n.hasAttribute === "function" && n.hasAttribute("inert")) {
            return;
          }
        }
        if (n.nodeType === 3) {
          if (isHtmlInert(n)) return;
          const txt = n.data !== undefined ? String(n.data) : String(n.textContent || "");
          if (n === sc && n === ec) {
            out += txt.slice(so, eo);
          } else if (n === sc) {
            out += txt.slice(so);
          } else if (n === ec) {
            out += txt.slice(0, eo);
          } else if (compareBoundaryPoints(n, txt.length, sc, so) > 0 && compareBoundaryPoints(n, 0, ec, eo) < 0) {
            out += txt;
          }
          return;
        }
        const kids = n.childNodes || n._children || [];
        for (let i = 0; i < kids.length; i++) walkNonInert(kids[i]);
      };
      walkNonInert(root);
      return out;
    }
    return r.toString ? r.toString() : "";
  }
  const st = _selState.get(this);
  const doc = (st && st.doc) || (typeof document !== "undefined" ? document : null);
  const ae = doc ? doc.activeElement : null;
  if (ae && ae.nodeType === 1) {
    const tag = String(ae.tagName || ae.localName || "").toLowerCase();
    if (tag === "input" || tag === "textarea") {
      const s = ae._selectionStart || 0;
      const e = ae._selectionEnd || 0;
      if (e > s) {
        const val = typeof getValueRaw === "function" ? getValueRaw(ae) : (ae._value !== undefined ? ae._value : (ae.value || ""));
        return String(val || "").slice(s, e);
      }
    }
  }
  return "";
};

Range.prototype.getClientRects = function () {
  const wrap = function (arr) {
    if (typeof global !== "undefined" && typeof global.__capsurf_createDOMRectList === "function") {
      return global.__capsurf_createDOMRectList(arr);
    }
    return arr;
  };
  const makeRect = function (x, y, w, h) {
    if (typeof DOMRect === "function") return new DOMRect(x, y, w, h);
    return { x: x, y: y, width: w, height: h, top: y, left: x, bottom: y + h, right: x + w };
  };
  const rects = [];
  const startNode = this.startContainer;
  const endNode = this.endContainer;
  if (!startNode || !endNode) return wrap(rects);

  const doc = (startNode && (startNode.ownerDocument || (startNode.nodeType === 9 ? startNode : null))) || (typeof document !== "undefined" ? document : null);
  const root = doc ? (doc.body || doc.documentElement) : null;
  if (!root) return wrap(rects);

  const range = this;
  if (!this.collapsed) {
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
            rects.push(makeRect(rx, ry, rw, rh));
          }
        }
        return;
      }

      if (node.nodeType === 1) {
        if (node.style && node.style.display === "none") return;
        if (typeof getComputedStyle === "function") {
          try {
            if (getComputedStyle(node).display === "none") return;
          } catch (e) {}
        }
        const tag = String(node.localName || node.tagName || "").toLowerCase();
        if (tag === "details" && !(node.hasAttribute && node.hasAttribute("open"))) {
          const dkids = node.childNodes || node._children || [];
          for (let i = 0; i < dkids.length; i++) {
            const k = dkids[i];
            if (k && k.nodeType === 1 && String(k.localName || k.tagName || "").toLowerCase() === "summary") {
              walk(k);
              break;
            }
          }
          return;
        }
      }

      const kids = node.childNodes || node._children || [];
      for (let i = 0; i < kids.length; i++) {
        walk(kids[i]);
      }
    }

    walk(root);
  }
  if (rects.length === 0 && startNode.nodeType === 1 && typeof startNode.getBoundingClientRect === "function") {
    const r = startNode.getBoundingClientRect();
    if (r && (r.width > 0 || r.height > 0)) {
      rects.push(r instanceof DOMRect ? r : makeRect(r.x, r.y, r.width, r.height));
    }
  }
  return wrap(rects);
};

Range.prototype.getBoundingClientRect = function () {
  const rects = this.getClientRects();
  if (!rects || rects.length === 0) {
    return typeof DOMRect === "function" ? new DOMRect(0, 0, 0, 0) : { x: 0, y: 0, width: 0, height: 0, top: 0, bottom: 0, left: 0, right: 0 };
  }
  let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
  for (let i = 0; i < rects.length; i++) {
    const r = rects[i];
    if (r.left < minX) minX = r.left;
    if (r.top < minY) minY = r.top;
    if (r.right > maxX) maxX = r.right;
    if (r.bottom > maxY) maxY = r.bottom;
  }
  const w = Math.max(0, maxX - minX);
  const h = Math.max(0, maxY - minY);
  if (typeof DOMRect === "function") {
    return new DOMRect(minX, minY, w, h);
  }
  return {
    x: minX,
    y: minY,
    width: w,
    height: h,
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

Range.prototype.comparePoint = function (node, offset) {
  const st = _rangeState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  if (arguments.length < 2 || !node || typeof node.nodeType !== "number") {
    throw new TypeError("Failed to execute 'comparePoint' on 'Range': parameter 1 is not of type 'Node'.");
  }
  const off = Number(offset) >>> 0;
  if (getRangeRoot(node) !== getRangeRoot(st.startContainer)) {
    throw new DOMException("The node is not in the same tree as the range.", "WrongDocumentError");
  }
  if (node.nodeType === 10) {
    throw new DOMException("DocumentType node cannot be used.", "InvalidNodeTypeError");
  }
  if (off > getNodeLength(node)) {
    throw new DOMException("The offset is larger than the node's length.", "IndexSizeError");
  }
  if (compareBoundaryPoints(node, off, st.startContainer, st.startOffset) < 0) return -1;
  if (compareBoundaryPoints(node, off, st.endContainer, st.endOffset) > 0) return 1;
  return 0;
};

Range.prototype.isPointInRange = function (node, offset) {
  const st = _rangeState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  if (arguments.length < 2 || !node || typeof node.nodeType !== "number") {
    throw new TypeError("Failed to execute 'isPointInRange' on 'Range': parameter 1 is not of type 'Node'.");
  }
  const off = Number(offset) >>> 0;
  if (getRangeRoot(node) !== getRangeRoot(st.startContainer)) return false;
  if (node.nodeType === 10) {
    throw new DOMException("DocumentType node cannot be used.", "InvalidNodeTypeError");
  }
  if (off > getNodeLength(node)) {
    throw new DOMException("The offset is larger than the node's length.", "IndexSizeError");
  }
  return compareBoundaryPoints(node, off, st.startContainer, st.startOffset) >= 0 &&
         compareBoundaryPoints(node, off, st.endContainer, st.endOffset) <= 0;
};

Range.prototype.intersectsNode = function (node) {
  const st = _rangeState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  if (arguments.length < 1 || !node || typeof node.nodeType !== "number") {
    throw new TypeError("Failed to execute 'intersectsNode' on 'Range': parameter 1 is not of type 'Node'.");
  }
  if (getRangeRoot(node) !== getRangeRoot(st.startContainer)) return false;
  const parent = node.parentNode;
  if (!parent) return true;
  let offset = 0;
  let s = parent.firstChild;
  while (s && s !== node) {
    offset++;
    s = s.nextSibling;
  }
  return compareBoundaryPoints(parent, offset, st.endContainer, st.endOffset) < 0 &&
         compareBoundaryPoints(parent, offset + 1, st.startContainer, st.startOffset) > 0;
};

markNative(Range.prototype.comparePoint, "comparePoint");
markNative(Range.prototype.isPointInRange, "isPointInRange");
markNative(Range.prototype.intersectsNode, "intersectsNode");
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
