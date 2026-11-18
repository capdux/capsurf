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
