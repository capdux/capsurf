// src/js/shim/parser.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/parser.ts

const SUPPORTED_MIME_TYPES = new Set([
  "text/html",
  "text/xml",
  "application/xml",
  "application/xhtml+xml",
  "image/svg+xml",
]);

const HTML_VOID_TAGS = new Set([
  "area", "base", "br", "col", "embed", "frame", "hr", "img", "input",
  "link", "meta", "param", "source", "track", "wbr",
]);

function camelToDash(str) {
  return String(str).replace(/[A-Z]/g, function (m) {
    return "-" + m.toLowerCase();
  });
}

function dashToCamel(str) {
  return String(str).replace(/-([a-z])/g, function (_, c) {
    return c.toUpperCase();
  });
}

function leadingDoctype(source, html) {
  const m = /^<!doctype(?:\s+([^\s>]+))?([^>]*)>/i.exec(source);
  if (!m) return null;
  const rest = m[2] || "";
  const quoted = function (s) {
    return /^\s*("([^"]*)"|'([^']*)')/.exec(s);
  };
  let publicId = "";
  let systemId = "";
  const kw = /^\s*(public|system)/i.exec(rest);
  if (kw) {
    let after = rest.slice(kw[0].length);
    const first = quoted(after);
    if (first) {
      const value = first[2] !== undefined ? first[2] : (first[3] !== undefined ? first[3] : "");
      after = after.slice(first[0].length);
      if ((kw[1] || "").toLowerCase() === "public") {
        publicId = value;
        const second = quoted(after);
        if (second) {
          systemId = second[2] !== undefined ? second[2] : (second[3] !== undefined ? second[3] : "");
        }
      } else {
        systemId = value;
      }
    }
  }
  const name = m[1] || "";
  return {
    name: html ? name.toLowerCase() : name,
    publicId: publicId,
    systemId: systemId,
  };
}

function unescapeEntities(str) {
  return str.replace(/&([a-zA-Z0-9]+|#[0-9]+|#x[0-9a-fA-F]+);/g, function (match, ent) {
    if (ent === "amp") return "&";
    if (ent === "lt") return "<";
    if (ent === "gt") return ">";
    if (ent === "quot") return '"';
    if (ent === "apos") return "'";
    if (ent.startsWith("#x") || ent.startsWith("#X")) {
      const code = parseInt(ent.slice(2), 16);
      return !isNaN(code) ? String.fromCharCode(code) : match;
    }
    if (ent.startsWith("#")) {
      const code = parseInt(ent.slice(1), 10);
      return !isNaN(code) ? String.fromCharCode(code) : match;
    }
    return match;
  });
}

function matchesSimpleSelector(el, s) {
  if (!el || el.nodeType !== 1) return false;
  s = s.trim();
  if (s === "*") return true;

  let rest = s;
  const tagMatch = rest.match(/^[a-zA-Z0-9_-]+/);
  if (tagMatch) {
    const tag = tagMatch[0].toLowerCase();
    rest = rest.slice(tag.length);
    const elTag = (el.localName || el.tagName || "").toLowerCase();
    if (elTag !== tag) return false;
  }

  while (rest.length > 0) {
    if (rest.startsWith("#")) {
      const idMatch = rest.match(/^#([a-zA-Z0-9_-]+)/);
      if (!idMatch) return false;
      const id = idMatch[1];
      if (el.getAttribute("id") !== id && el.id !== id) return false;
      rest = rest.slice(idMatch[0].length);
    } else if (rest.startsWith(".")) {
      const clsMatch = rest.match(/^\.([a-zA-Z0-9_-]+)/);
      if (!clsMatch) return false;
      const cls = clsMatch[1];
      const clsAttr = el.getAttribute("class") || el.className || "";
      if (!clsAttr.split(/\s+/).includes(cls)) return false;
      rest = rest.slice(clsMatch[0].length);
    } else if (rest.startsWith("[")) {
      const attrMatch = rest.match(/^\[([a-zA-Z0-9_:-]+)(?:([*^$]?=)(?:"([^"]*)"|'([^']*)'|([^\]]+)))?\]/);
      if (!attrMatch) return false;
      const attrName = attrMatch[1];
      const op = attrMatch[2];
      const attrVal = attrMatch[3] !== undefined ? attrMatch[3] : (attrMatch[4] !== undefined ? attrMatch[4] : attrMatch[5]);
      if (!el.hasAttribute(attrName)) return false;
      if (op) {
        const actual = el.getAttribute(attrName) || "";
        if (op === "=" && actual !== attrVal) return false;
        if (op === "*=" && !actual.includes(attrVal)) return false;
        if (op === "^=" && !actual.startsWith(attrVal)) return false;
        if (op === "$=" && !actual.endsWith(attrVal)) return false;
      }
      rest = rest.slice(attrMatch[0].length);
    } else {
      break;
    }
  }

  return true;
}

function matchesSelector(el, sel) {
  if (!el || el.nodeType !== 1) return false;
  const s = sel.trim();
  if (s === "*") return true;

  if (s.includes(",")) {
    const parts = s.split(",");
    for (let i = 0; i < parts.length; i++) {
      if (matchesSelector(el, parts[i])) return true;
    }
    return false;
  }

  const parts = s.split(/\s+/);
  if (parts.length > 1) {
    if (!matchesSimpleSelector(el, parts[parts.length - 1])) return false;
    let curr = el.parentNode;
    let pIdx = parts.length - 2;
    while (curr && pIdx >= 0) {
      if (curr.nodeType === 1 && matchesSimpleSelector(curr, parts[pIdx])) {
        pIdx--;
      }
      curr = curr.parentNode;
    }
    return pIdx < 0;
  }

  return matchesSimpleSelector(el, s);
}

function ParsedText(data, ownerDoc) {
  this.nodeType = TEXT_NODE;
  this.nodeName = "#text";
  this.data = String(data !== undefined ? data : "");
  this.ownerDocument = ownerDoc || null;
  this.parentNode = null;
  registerParsedNode(this);
}
Object.defineProperty(ParsedText.prototype, "textContent", {
  get: function () { return this.data; },
  set: function (v) { this.data = String(v); },
  configurable: true,
});
Object.defineProperty(ParsedText.prototype, "nodeValue", {
  get: function () { return this.data; },
  set: function (v) { this.data = String(v); },
  configurable: true,
});
Object.defineProperty(ParsedText.prototype, "length", {
  get: function () { return this.data.length; },
  configurable: true,
});
Object.defineProperty(ParsedText.prototype, "parentElement", {
  get: function () { return (this.parentNode && this.parentNode.nodeType === ELEMENT_NODE) ? this.parentNode : null; },
  configurable: true,
});

function ParsedComment(data, ownerDoc) {
  this.nodeType = COMMENT_NODE;
  this.nodeName = "#comment";
  this.data = String(data !== undefined ? data : "");
  this.ownerDocument = ownerDoc || null;
  this.parentNode = null;
  registerParsedNode(this);
}
Object.defineProperty(ParsedComment.prototype, "textContent", {
  get: function () { return this.data; },
  set: function (v) { this.data = String(v); },
  configurable: true,
});
Object.defineProperty(ParsedComment.prototype, "nodeValue", {
  get: function () { return this.data; },
  set: function (v) { this.data = String(v); },
  configurable: true,
});
Object.defineProperty(ParsedComment.prototype, "length", {
  get: function () { return this.data.length; },
  configurable: true,
});
Object.defineProperty(ParsedComment.prototype, "parentElement", {
  get: function () { return (this.parentNode && this.parentNode.nodeType === ELEMENT_NODE) ? this.parentNode : null; },
  configurable: true,
});

function ParsedFragment(ownerDoc) {
  this.nodeType = DOCUMENT_FRAGMENT_NODE;
  this.nodeName = "#document-fragment";
  this.ownerDocument = ownerDoc || null;
  this.parentNode = null;
  this._children = [];
  registerParsedNode(this);
}
ParsedFragment.prototype.appendChild = function (child) {
  if (!child || typeof child !== "object") {
    throw new TypeError("Failed to execute 'appendChild' on 'Node': parameter 1 is not of type 'Node'.");
  }
  if (child.nodeType === DOCUMENT_FRAGMENT_NODE) {
    const kids = Array.from(child.childNodes || child._children || []);
    for (let i = 0; i < kids.length; i++) {
      this.appendChild(kids[i]);
    }
    if (child._children) child._children.length = 0;
    return child;
  }
  if (child.parentNode && typeof child.parentNode.removeChild === "function") {
    try { child.parentNode.removeChild(child); } catch (e) {}
  }
  child.parentNode = this;
  child.ownerDocument = this.ownerDocument || this;
  this._children.push(child);
  return child;
};
ParsedFragment.prototype.removeChild = function (child) {
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
ParsedFragment.prototype.replaceChild = function (newChild, oldChild) {
  this.insertBefore(newChild, oldChild);
  return this.removeChild(oldChild);
};
Object.defineProperty(ParsedFragment.prototype, "childNodes", {
  get: function () {
    return new NodeList(this._children);
  },
  configurable: true,
});
Object.defineProperty(ParsedFragment.prototype, "children", {
  get: function () {
    return this._children.filter(function (n) { return n.nodeType === ELEMENT_NODE; });
  },
  configurable: true,
});
Object.defineProperty(ParsedFragment.prototype, "firstChild", {
  get: function () {
    return this._children.length > 0 ? this._children[0] : null;
  },
  configurable: true,
});
Object.defineProperty(ParsedFragment.prototype, "lastChild", {
  get: function () {
    return this._children.length > 0 ? this._children[this._children.length - 1] : null;
  },
  configurable: true,
});

function ParsedElement(tagName, ownerDoc) {
  this.nodeType = ELEMENT_NODE;
  const isHtml = !ownerDoc || ownerDoc.contentType === "text/html";
  this.localName = String(tagName).toLowerCase();
  this.tagName = isHtml ? String(tagName).toUpperCase() : this.localName;
  this.nodeName = this.tagName;
  this.ownerDocument = ownerDoc || null;
  this.parentNode = null;
  this._attrs = new Map();
  this._attrCases = new Map();
  this._children = [];
  registerParsedNode(this);
}

ParsedElement.prototype.getAttribute = function (name) {
  const k = String(name).toLowerCase();
  return this._attrs.has(k) ? this._attrs.get(k) : null;
};

ParsedElement.prototype.setAttribute = function (name, value) {
  const k = String(name).toLowerCase();
  const val = String(value);
  this._attrs.set(k, val);
  if (!this._attrCases.has(k)) {
    this._attrCases.set(k, String(name));
  }
};

ParsedElement.prototype.hasAttribute = function (name) {
  const k = String(name).toLowerCase();
  return this._attrs.has(k);
};

ParsedElement.prototype.removeAttribute = function (name) {
  const k = String(name).toLowerCase();
  this._attrs.delete(k);
  this._attrCases.delete(k);
};

ParsedElement.prototype.getAttributeNames = function () {
  return Array.from(this._attrCases.values());
};

Object.defineProperty(ParsedElement.prototype, "attributes", {
  get: function () {
    const items = [];
    const byName = new Map();
    const map = {};
    if (this._attrs instanceof Map) {
      this._attrs.forEach((val, key) => {
        const name = (this._attrCases && this._attrCases.get(key)) || key;
        const attr = {
          name: name,
          value: val,
          nodeName: name,
          nodeValue: val,
          specified: true,
        };
        map[items.length] = attr;
        items.push(attr);
        byName.set(String(key).toLowerCase(), attr);
      });
    }
    Object.defineProperties(map, {
      length: { value: items.length, enumerable: false, configurable: true },
      item: {
        value: function (i) {
          const idx = Number(i);
          return Number.isInteger(idx) && idx >= 0 && idx < items.length ? items[idx] : null;
        },
        enumerable: false,
        configurable: true,
      },
      getNamedItem: {
        value: function (name) {
          return byName.get(String(name).toLowerCase()) || null;
        },
        enumerable: false,
        configurable: true,
      },
      [Symbol.iterator]: {
        value: function () {
          return items[Symbol.iterator]();
        },
        enumerable: false,
        configurable: true,
      },
    });
    return map;
  },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "nodeValue", {
  get: function () { return null; },
  set: function () {},
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "id", {
  get: function () { return this.getAttribute("id") || ""; },
  set: function (v) { this.setAttribute("id", v); },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "className", {
  get: function () { return this.getAttribute("class") || ""; },
  set: function (v) { this.setAttribute("class", v); },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "classList", {
  get: function () {
    const cls = this.className;
    const tokens = cls.split(/\s+/).filter(Boolean);
    const self = this;
    tokens.contains = function (t) { return tokens.indexOf(t) >= 0; };
    tokens.add = function (t) {
      if (!tokens.contains(t)) {
        tokens.push(t);
        self.className = tokens.join(" ");
      }
    };
    tokens.remove = function (t) {
      const i = tokens.indexOf(t);
      if (i >= 0) {
        tokens.splice(i, 1);
        self.className = tokens.join(" ");
      }
    };
    return tokens;
  },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "dataset", {
  get: function () {
    const el = this;
    return new Proxy(Object.create(null), {
      get: function (target, prop) {
        if (typeof prop !== "string") return undefined;
        const attrName = "data-" + camelToDash(prop);
        return el.hasAttribute(attrName) ? el.getAttribute(attrName) : undefined;
      },
      set: function (target, prop, value) {
        if (typeof prop !== "string") return true;
        const attrName = "data-" + camelToDash(prop);
        el.setAttribute(attrName, String(value));
        return true;
      },
      has: function (target, prop) {
        if (typeof prop !== "string") return false;
        const attrName = "data-" + camelToDash(prop);
        return el.hasAttribute(attrName);
      },
      deleteProperty: function (target, prop) {
        if (typeof prop !== "string") return true;
        const attrName = "data-" + camelToDash(prop);
        el.removeAttribute(attrName);
        return true;
      },
      ownKeys: function () {
        const keys = [];
        const names = el.getAttributeNames ? el.getAttributeNames() : [];
        for (let i = 0; i < names.length; i++) {
          if (names[i].startsWith("data-")) {
            keys.push(dashToCamel(names[i].slice(5)));
          }
        }
        return keys;
      },
      getOwnPropertyDescriptor: function (target, prop) {
        if (typeof prop === "string") {
          const attrName = "data-" + camelToDash(prop);
          if (el.hasAttribute(attrName)) {
            return {
              value: el.getAttribute(attrName),
              writable: true,
              enumerable: true,
              configurable: true,
            };
          }
        }
        return undefined;
      },
    });
  },
  configurable: true,
});

function collectParsedText(node) {
  if (node.nodeType === TEXT_NODE) return node.data || "";
  if (node.nodeType === ELEMENT_NODE || node.nodeType === DOCUMENT_FRAGMENT_NODE || node.nodeType === DOCUMENT_NODE) {
    let res = "";
    const kids = node._children || [];
    for (let i = 0; i < kids.length; i++) {
      res += collectParsedText(kids[i]);
    }
    return res;
  }
  return "";
}

Object.defineProperty(ParsedElement.prototype, "textContent", {
  get: function () {
    return collectParsedText(this);
  },
  set: function (v) {
    this._children = [];
    const val = String(v);
    if (val.length > 0) {
      const t = this.ownerDocument ? this.ownerDocument.createTextNode(val) : new ParsedText(val, null);
      t.parentNode = this;
      this._children.push(t);
    }
  },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "childNodes", {
  get: function () {
    return new NodeList(this._children);
  },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "children", {
  get: function () {
    return this._children.filter(function (n) { return n.nodeType === ELEMENT_NODE; });
  },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "firstChild", {
  get: function () {
    return this._children.length > 0 ? this._children[0] : null;
  },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "lastChild", {
  get: function () {
    return this._children.length > 0 ? this._children[this._children.length - 1] : null;
  },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "previousSibling", {
  get: function () {
    if (!this.parentNode || !this.parentNode._children) return null;
    const idx = this.parentNode._children.indexOf(this);
    return idx > 0 ? this.parentNode._children[idx - 1] : null;
  },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "nextSibling", {
  get: function () {
    if (!this.parentNode || !this.parentNode._children) return null;
    const idx = this.parentNode._children.indexOf(this);
    return (idx >= 0 && idx < this.parentNode._children.length - 1) ? this.parentNode._children[idx + 1] : null;
  },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "parentElement", {
  get: function () {
    return (this.parentNode && this.parentNode.nodeType === ELEMENT_NODE) ? this.parentNode : null;
  },
  configurable: true,
});

ParsedElement.prototype.appendChild = function (child) {
  if (!child || typeof child !== "object") {
    throw new TypeError("Failed to execute 'appendChild' on 'Node': parameter 1 is not of type 'Node'.");
  }
  if (child.nodeType === DOCUMENT_FRAGMENT_NODE) {
    const kids = Array.from(child.childNodes || child._children || []);
    for (let i = 0; i < kids.length; i++) {
      this.appendChild(kids[i]);
    }
    if (child._children) child._children.length = 0;
    return child;
  }
  if (child.parentNode && typeof child.parentNode.removeChild === "function") {
    try { child.parentNode.removeChild(child); } catch (e) {}
  }
  child.parentNode = this;
  child.ownerDocument = this.ownerDocument || this;
  this._children.push(child);
  return child;
};

ParsedElement.prototype.removeChild = function (child) {
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
ParsedElement.prototype.remove = function () {
  if (this.parentNode && typeof this.parentNode.removeChild === "function") {
    this.parentNode.removeChild(this);
  }
};
ParsedElement.prototype.replaceChild = function (newChild, oldChild) {
  this.insertBefore(newChild, oldChild);
  return this.removeChild(oldChild);
};

ParsedElement.prototype.insertBefore = function (newChild, refChild) {
  if (!newChild || typeof newChild !== "object") {
    throw new TypeError("Failed to execute 'insertBefore' on 'Node': parameter 1 is not of type 'Node'.");
  }
  if (refChild == null) {
    return this.appendChild(newChild);
  }
  const idx = this._children.indexOf(refChild);
  if (idx === -1) {
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
    try { newChild.parentNode.removeChild(newChild); } catch (e) {}
  }
  newChild.parentNode = this;
  newChild.ownerDocument = this.ownerDocument || this;
  const curIdx = this._children.indexOf(refChild);
  this._children.splice(curIdx >= 0 ? curIdx : this._children.length, 0, newChild);
  return newChild;
};

ParsedElement.prototype.querySelector = function (sel) {
  if (arguments.length === 0) throw new TypeError("querySelector: 1 argument required");
  function walk(node) {
    const kids = node._children || [];
    for (let i = 0; i < kids.length; i++) {
      const k = kids[i];
      if (k.nodeType === ELEMENT_NODE) {
        if (matchesSelector(k, sel)) return k;
        const found = walk(k);
        if (found) return found;
      }
    }
    return null;
  }
  return walk(this);
};

ParsedElement.prototype.querySelectorAll = function (sel) {
  if (arguments.length === 0) throw new TypeError("querySelectorAll: 1 argument required");
  const results = [];
  function walk(node) {
    const kids = node._children || [];
    for (let i = 0; i < kids.length; i++) {
      const k = kids[i];
      if (k.nodeType === ELEMENT_NODE) {
        if (matchesSelector(k, sel)) results.push(k);
        walk(k);
      }
    }
  }
  walk(this);
  return new NodeList(results);
};

ParsedElement.prototype.getElementById = function (id) {
  return this.querySelector("#" + id);
};

ParsedElement.prototype.getElementsByTagName = function (tag) {
  return this.querySelectorAll(tag);
};

ParsedElement.prototype.getElementsByClassName = function (cls) {
  return this.querySelectorAll("." + cls);
};

Object.defineProperty(ParsedElement.prototype, "outerHTML", {
  get: function () {
    return serializeXmlNode(this, true);
  },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "innerHTML", {
  get: function () {
    const kids = this._children || [];
    let out = "";
    for (let i = 0; i < kids.length; i++) {
      out += serializeXmlNode(kids[i]);
    }
    return out;
  },
  set: function (val) {
    this._children = [];
    const str = String(val);
    if (!str) return;
    const parsed = parseDocument(str, "text/html");
    const rawNodes = (parsed.body && parsed.body._children && parsed.body._children.length > 0)
      ? parsed.body._children
      : ((parsed.head && parsed.head._children && parsed.head._children.length > 0)
          ? parsed.head._children
          : (parsed._children || []));
    const nodes = rawNodes.slice();
    const targetDoc = this.ownerDocument || null;
    function setOwnerDoc(node, doc) {
      if (!node || typeof node !== "object") return;
      node.ownerDocument = doc;
      const cs = node._children;
      if (cs && cs.length) {
        for (let i = 0; i < cs.length; i++) setOwnerDoc(cs[i], doc);
      }
    }
    for (let i = 0; i < nodes.length; i++) {
      const n = nodes[i];
      this.appendChild(n);
      if (targetDoc) setOwnerDoc(n, targetDoc);
    }
  },
  configurable: true,
});

ParsedElement.prototype.lookupNamespaceURI = function (prefix) {
  const p = prefix === "" || prefix === undefined ? null : String(prefix);
  let n = this;
  while (n && n.nodeType === ELEMENT_NODE) {
    if (p === null) {
      if (n.hasAttribute("xmlns")) return n.getAttribute("xmlns") || null;
    } else {
      if (n.hasAttribute("xmlns:" + p)) return n.getAttribute("xmlns:" + p) || null;
      const attrs = n.attributes;
      if (attrs) {
        for (let i = 0; i < attrs.length; i++) {
          const a = attrs[i];
          if (a && a.prefix === "xmlns" && a.localName === p) {
            return a.value || null;
          }
        }
      }
    }
    n = n.parentElement || n.parentNode;
  }
  return null;
};

ParsedElement.prototype.addEventListener = function (type, listener, options) {
  if (typeof EventTarget === "function" && EventTarget.prototype.addEventListener) {
    return EventTarget.prototype.addEventListener.call(this, type, listener, options);
  }
};
ParsedElement.prototype.removeEventListener = function (type, listener, options) {
  if (typeof EventTarget === "function" && EventTarget.prototype.removeEventListener) {
    return EventTarget.prototype.removeEventListener.call(this, type, listener, options);
  }
};
ParsedElement.prototype.dispatchEvent = function (event) {
  if (typeof EventTarget === "function" && EventTarget.prototype.dispatchEvent) {
    return EventTarget.prototype.dispatchEvent.call(this, event);
  }
  return true;
};

Object.defineProperty(ParsedElement.prototype, "isConnected", {
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

ParsedElement.prototype.getBoundingClientRect = function () {
  if (typeof Element !== "undefined" && Element.prototype && typeof Element.prototype.getBoundingClientRect === "function") {
    return Element.prototype.getBoundingClientRect.call(this);
  }
  return { x: 0, y: 0, left: 0, top: 0, right: 0, bottom: 0, width: 0, height: 0 };
};

ParsedElement.prototype.getClientRects = function () {
  if (typeof Element !== "undefined" && Element.prototype && typeof Element.prototype.getClientRects === "function") {
    return Element.prototype.getClientRects.call(this);
  }
  return [this.getBoundingClientRect()];
};

ParsedElement.prototype.checkVisibility = function (options) {
  if (typeof Element !== "undefined" && Element.prototype && typeof Element.prototype.checkVisibility === "function") {
    return Element.prototype.checkVisibility.call(this, options);
  }
  return Boolean(this.isConnected);
};

ParsedElement.prototype.matches = function (sel) {
  return matchesSelector(this, sel);
};

ParsedElement.prototype.closest = function (sel) {
  let curr = this;
  while (curr && curr.nodeType === ELEMENT_NODE) {
    if (matchesSelector(curr, sel)) return curr;
    curr = curr.parentElement || curr.parentNode;
  }
  return null;
};

Object.defineProperty(ParsedElement.prototype, "type", {
  get: function () {
    if (this.localName === "input") {
      if (typeof HTMLInputElement !== "undefined" && HTMLInputElement.prototype) {
        const d = Object.getOwnPropertyDescriptor(HTMLInputElement.prototype, "type");
        if (d && d.get) return d.get.call(this);
      }
      return String(this.getAttribute("type") || "text").toLowerCase();
    }
    if (this.localName === "button") return String(this.getAttribute("type") || "submit").toLowerCase();
    if (this.localName === "select") return this.hasAttribute("multiple") ? "select-multiple" : "select-one";
    if (this.localName === "textarea") return "textarea";
    return this.getAttribute("type");
  },
  set: function (v) {
    this.setAttribute("type", String(v));
  },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "disabled", {
  get: function () { return this.hasAttribute("disabled"); },
  set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "multiple", {
  get: function () { return this.hasAttribute("multiple"); },
  set: function (v) { if (v) this.setAttribute("multiple", ""); else this.removeAttribute("multiple"); },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "checked", {
  get: function () { return this._checked !== undefined ? Boolean(this._checked) : this.hasAttribute("checked"); },
  set: function (v) { this._checked = Boolean(v); },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "value", {
  get: function () { return this._value !== undefined ? this._value : (this.getAttribute("value") || ""); },
  set: function (v) { this._value = String(v); },
  configurable: true,
});

Object.defineProperty(ParsedElement.prototype, "files", {
  get: function () { return this.localName === "input" ? (this._files || []) : undefined; },
  set: function (v) { this._files = v; },
  configurable: true,
});

ParsedElement.prototype.click = function () {
  if (typeof HTMLElement !== "undefined" && HTMLElement.prototype && typeof HTMLElement.prototype.click === "function") {
    return HTMLElement.prototype.click.call(this);
  }
};

ParsedElement.prototype.focus = function () {
  if (typeof HTMLElement !== "undefined" && HTMLElement.prototype && typeof HTMLElement.prototype.focus === "function") {
    return HTMLElement.prototype.focus.call(this);
  }
};

ParsedElement.prototype.blur = function () {
  if (typeof HTMLElement !== "undefined" && HTMLElement.prototype && typeof HTMLElement.prototype.blur === "function") {
    return HTMLElement.prototype.blur.call(this);
  }
};

ParsedElement.prototype.getRootNode = function () {
  let cur = this;
  while (cur && cur.parentNode) cur = cur.parentNode;
  return cur;
};

ParsedElement.prototype.contains = function (other) {
  let cur = other;
  while (cur) {
    if (cur === this) return true;
    cur = cur.parentNode;
  }
  return false;
};

ParsedElement.prototype.scrollIntoView = function () {};
ParsedElement.prototype.scrollIntoViewIfNeeded = function () {};

function ParsedDocument(contentType, compatMode) {
  this.nodeType = DOCUMENT_NODE;
  this.nodeName = "#document";
  this.contentType = contentType || "text/html";
  this.compatMode = compatMode || "CSS1Compat";
  this.characterSet = "UTF-8";
  this.charset = "UTF-8";
  this.inputEncoding = "UTF-8";
  this.referrer = "";
  this.URL = "";
  this.documentURI = "";
  this.baseURI = "";
  this.readyState = "complete";
  this.defaultView = null;
  this.location = null;
  this.ownerDocument = null;
  this.parentNode = null;
  this._children = [];
  this.documentElement = null;
  this.head = null;
  this.body = null;
  this._title = "";
  this._doctypeInfo = null;
  this._doctypeNode = null;
  registerParsedNode(this);
}

Object.defineProperty(ParsedDocument.prototype, "isConnected", {
  get: function () {
    return true;
  },
  configurable: true,
});

Object.defineProperty(ParsedDocument.prototype, "title", {
  get: function () {
    const titleEl = this.querySelector ? this.querySelector("title") : null;
    return titleEl ? (titleEl.textContent || "") : this._title;
  },
  set: function (v) {
    this._title = String(v);
    const titleEl = this.querySelector ? this.querySelector("title") : null;
    if (titleEl) {
      titleEl.textContent = this._title;
    }
  },
  configurable: true,
});

Object.defineProperty(ParsedDocument.prototype, "styleSheets", {
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

Object.defineProperty(ParsedDocument.prototype, "doctype", {
  get: function () {
    if (!this._doctypeInfo) return null;
    if (!this._doctypeNode) {
      this._doctypeNode = {
        nodeType: DOCUMENT_TYPE_NODE,
        nodeName: this._doctypeInfo.name,
        name: this._doctypeInfo.name,
        publicId: this._doctypeInfo.publicId,
        systemId: this._doctypeInfo.systemId,
        ownerDocument: this,
        parentNode: this,
      };
    }
    return this._doctypeNode;
  },
  configurable: true,
});

Object.defineProperty(ParsedDocument.prototype, "childNodes", {
  get: function () {
    return new NodeList(this._children);
  },
  configurable: true,
});

Object.defineProperty(ParsedDocument.prototype, "children", {
  get: function () {
    return this._children.filter(function (n) { return n.nodeType === ELEMENT_NODE; });
  },
  configurable: true,
});

Object.defineProperty(ParsedDocument.prototype, "firstChild", {
  get: function () {
    return this._children.length > 0 ? this._children[0] : null;
  },
  configurable: true,
});

Object.defineProperty(ParsedDocument.prototype, "lastChild", {
  get: function () {
    return this._children.length > 0 ? this._children[this._children.length - 1] : null;
  },
  configurable: true,
});

ParsedDocument.prototype.elementFromPoint = function (x, y) {
  if (typeof ShadowRoot !== "undefined" && ShadowRoot.prototype && typeof ShadowRoot.prototype.elementFromPoint === "function") {
    return ShadowRoot.prototype.elementFromPoint.call(this, x, y);
  }
  return this.body || this.documentElement || null;
};

ParsedDocument.prototype.elementsFromPoint = function (x, y) {
  const out = [];
  let cur = this.elementFromPoint(x, y);
  while (cur && cur.nodeType === 1) {
    out.push(cur);
    cur = cur.parentElement || cur.parentNode;
  }
  return out;
};

ParsedDocument.prototype.createElement = function (tagName) {
  const el = new ParsedElement(tagName, this);
  const lowerTag = String(tagName).toLowerCase();
  if (lowerTag === "iframe" || lowerTag === "frame") {
    Object.defineProperty(el, "contentWindow", {
      get: function () {
        if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_getOrCreateIframeWindow === "function") {
          return globalThis.__capsurf_getOrCreateIframeWindow(this);
        }
        return null;
      },
      configurable: true,
    });
    Object.defineProperty(el, "contentDocument", {
      get: function () {
        const win = this.contentWindow;
        return (win && win.document) || this._contentDocument || null;
      },
      configurable: true,
    });
  }
  return el;
};

function normalizeParsedChildren(node) {
  if (!node || !Array.isArray(node._children)) return;
  const kids = node._children;
  let i = 0;
  while (i < kids.length) {
    const ch = kids[i];
    if (ch && ch.nodeType === TEXT_NODE) {
      if (ch.data === "") {
        ch.parentNode = null;
        kids.splice(i, 1);
        continue;
      }
      while (i + 1 < kids.length && kids[i + 1] && kids[i + 1].nodeType === TEXT_NODE) {
        const next = kids[i + 1];
        ch.data += next.data;
        next.parentNode = null;
        kids.splice(i + 1, 1);
      }
      i++;
    } else {
      if (ch && Array.isArray(ch._children)) {
        normalizeParsedChildren(ch);
      }
      i++;
    }
  }
}

ParsedElement.prototype.normalize = function () {
  normalizeParsedChildren(this);
};

ParsedDocument.prototype.normalize = function () {
  normalizeParsedChildren(this);
};

ParsedDocument.prototype.createTextNode = function (text) {
  return new ParsedText(text, this);
};

ParsedDocument.prototype.createComment = function (data) {
  return new ParsedComment(data, this);
};

ParsedDocument.prototype.createCDATASection = function (data) {
  if (arguments.length === 0) throw new TypeError("Failed to execute 'createCDATASection': 1 argument required.");
  const s = String(data);
  if (s.indexOf("]]>") !== -1) {
    throw new DOMException("String contains ']]>'", "InvalidCharacterError");
  }
  if (this.contentType === "text/html") {
    throw new DOMException("Operation is not supported for HTML documents.", "NotSupportedError");
  }
  const node = new ParsedText(s, this);
  node.nodeType = 4;
  node.nodeName = "#cdata-section";
  return node;
};

ParsedDocument.prototype.createDocumentFragment = function () {
  return new ParsedFragment(this);
};

ParsedDocument.prototype.querySelector = function (sel) {
  if (arguments.length === 0) throw new TypeError("querySelector: 1 argument required");
  for (let i = 0; i < this._children.length; i++) {
    const root = this._children[i];
    if (root.nodeType === ELEMENT_NODE) {
      if (matchesSelector(root, sel)) return root;
      const found = root.querySelector(sel);
      if (found) return found;
    }
  }
  return null;
};

ParsedDocument.prototype.querySelectorAll = function (sel) {
  if (arguments.length === 0) throw new TypeError("querySelectorAll: 1 argument required");
  const list = [];
  for (let i = 0; i < this._children.length; i++) {
    const root = this._children[i];
    if (root.nodeType === ELEMENT_NODE) {
      if (matchesSelector(root, sel)) list.push(root);
      const sub = root.querySelectorAll(sel);
      for (let j = 0; j < sub.length; j++) list.push(sub[j]);
    }
  }
  return new NodeList(list);
};

ParsedDocument.prototype.getElementById = function (id) {
  return this.querySelector("#" + id);
};

ParsedDocument.prototype.getElementsByTagName = function (tag) {
  return this.querySelectorAll(tag);
};

ParsedDocument.prototype.getElementsByClassName = function (cls) {
  return this.querySelectorAll("." + cls);
};

ParsedDocument.prototype.createNSResolver = function (nodeResolver) {
  return nodeResolver;
};

ParsedDocument.prototype.evaluate = function (expression, contextNode, resolver, type, result) {
  if (typeof evaluateXPath !== "undefined") {
    return evaluateXPath(expression, contextNode || this, resolver, type, result);
  }
  throw new TypeError("evaluateXPath is not available");
};

ParsedDocument.prototype.createExpression = function (expression, resolver) {
  if (typeof createExpression !== "undefined") {
    return createExpression(expression, resolver);
  }
  throw new TypeError("createExpression is not available");
};

ParsedDocument.prototype.lookupNamespaceURI = function (prefix) {
  return this.documentElement ? this.documentElement.lookupNamespaceURI(prefix) : null;
};

ParsedDocument.prototype.addEventListener = function (type, listener, options) {
  if (typeof EventTarget === "function" && EventTarget.prototype.addEventListener) {
    return EventTarget.prototype.addEventListener.call(this, type, listener, options);
  }
};
ParsedDocument.prototype.removeEventListener = function (type, listener, options) {
  if (typeof EventTarget === "function" && EventTarget.prototype.removeEventListener) {
    return EventTarget.prototype.removeEventListener.call(this, type, listener, options);
  }
};
ParsedDocument.prototype.dispatchEvent = function (event) {
  if (typeof EventTarget === "function" && EventTarget.prototype.dispatchEvent) {
    return EventTarget.prototype.dispatchEvent.call(this, event);
  }
  return true;
};

function parseDocument(str, mime) {
  const type = String(mime || "").toLowerCase();
  if (!SUPPORTED_MIME_TYPES.has(type)) {
    throw new TypeError(
      "Failed to execute 'parseFromString' on 'DOMParser': The provided value '" +
        mime +
        "' is not a valid enum value of type SupportedType."
    );
  }

  let input = typeof str === "string" ? str : String(str ?? "");
  const trimmed = input.replace(/^\uFEFF/, "").trim();
  const isHtml = type === "text/html";
  const compatMode = isHtml ? (/^<!doctype\s+html\s*>/i.test(trimmed) ? "CSS1Compat" : "BackCompat") : "CSS1Compat";
  const doc = new ParsedDocument(type, compatMode);
  doc._doctypeInfo = leadingDoctype(trimmed, isHtml);

  if (isHtml) {
    if (input.startsWith("<?")) {
      const end = input.indexOf(">");
      const rawPI = input.slice(1, end < 0 ? undefined : end);
      const cNode = doc.createComment(rawPI);
      cNode.parentNode = doc;
      doc._children.push(cNode);
      input = end < 0 ? "" : input.slice(end + 1);
    }

    const root = doc.createElement("html");
    const head = doc.createElement("head");
    const body = doc.createElement("body");
    root.appendChild(head);
    root.appendChild(body);
    root.parentNode = doc;
    doc._children.push(root);
    doc.documentElement = root;
    doc.head = head;
    doc.body = body;

    const tokenRegex = /<!--([\s\S]*?)-->|<!\[CDATA\[([\s\S]*?)\]\]>|<!doctype[^>]*>|<\?([\s\S]*?)\?>|<\/([a-zA-Z0-9_:-]+)\s*>|<([a-zA-Z0-9_:-]+)((?:\s+[^=>/\s]+(?:\s*=\s*(?:"[^"]*"|'[^']*'|[^>\s]+))?)*)\s*(\/?)>/gi;
    let stack = [body];
    let lastIndex = 0;
    let match;

    while ((match = tokenRegex.exec(input)) !== null) {
      if (match.index > lastIndex) {
        const text = unescapeEntities(input.slice(lastIndex, match.index));
        if (text.length > 0) {
          const tNode = doc.createTextNode(text);
          stack[stack.length - 1].appendChild(tNode);
        }
      }
      lastIndex = tokenRegex.lastIndex;

      if (match[1] !== undefined) {
        // Comment
        const cNode = doc.createComment(match[1]);
        stack[stack.length - 1].appendChild(cNode);
      } else if (match[2] !== undefined) {
        // CDATA
        const tNode = doc.createTextNode(match[2]);
        stack[stack.length - 1].appendChild(tNode);
      } else if (match[3] !== undefined) {
        // PI
        const cNode = doc.createComment("?" + match[3]);
        stack[stack.length - 1].appendChild(cNode);
      } else if (match[4] !== undefined) {
        // End tag
        const endTag = match[4].toLowerCase();
        for (let i = stack.length - 1; i > 0; i--) {
          if (stack[i].localName === endTag) {
            stack.length = i;
            break;
          }
        }
      } else if (match[5] !== undefined) {
        // Start tag
        const tagName = match[5];
        const lowerTag = tagName.toLowerCase();
        const attrStr = match[6] || "";
        const isSelfClosing = Boolean(match[7]);

        if (lowerTag === "html") {
          applyAttributes(root, attrStr);
        } else if (lowerTag === "head") {
          applyAttributes(head, attrStr);
          stack = [head];
        } else if (lowerTag === "body") {
          applyAttributes(body, attrStr);
          stack = [body];
        } else if (lowerTag === "frameset" && stack.length === 1 && stack[0] === body && body._children.length === 0) {
          const el = doc.createElement(tagName);
          applyAttributes(el, attrStr);
          root.removeChild(body);
          root.appendChild(el);
          doc.body = el;
          if (!isSelfClosing) {
            stack = [el];
          }
        } else {
          const el = doc.createElement(tagName);
          applyAttributes(el, attrStr);

          if (lowerTag === "title" || lowerTag === "meta" || lowerTag === "link" || lowerTag === "style") {
            head.appendChild(el);
            if ((lowerTag === "title" || lowerTag === "style") && !isSelfClosing) {
              stack.push(el);
            }
          } else {
            stack[stack.length - 1].appendChild(el);
            if (!isSelfClosing && !HTML_VOID_TAGS.has(lowerTag)) {
              stack.push(el);
            }
          }
        }
      }
    }

    if (lastIndex < input.length) {
      const text = unescapeEntities(input.slice(lastIndex));
      if (text.length > 0) {
        const tNode = doc.createTextNode(text);
        stack[stack.length - 1].appendChild(tNode);
      }
    }

    const titleEl = head.querySelector("title");
    if (titleEl) {
      doc._title = titleEl.textContent;
    }
  } else {
    // XML / SVG mode
    const tokenRegex = /<!--([\s\S]*?)-->|<!\[CDATA\[([\s\S]*?)\]\]>|<!doctype[^>]*>|<\?([\s\S]*?)\?>|<\/([a-zA-Z0-9_:-]+)\s*>|<([a-zA-Z0-9_:-]+)((?:\s+[^=>/\s]+(?:\s*=\s*(?:"[^"]*"|'[^']*'|[^>\s]+))?)*)\s*(\/?)>/gi;
    let stack = [doc];
    let lastIndex = 0;
    let match;

    while ((match = tokenRegex.exec(input)) !== null) {
      if (match.index > lastIndex) {
        const text = unescapeEntities(input.slice(lastIndex, match.index));
        if (text.length > 0 && stack.length > 1) {
          const tNode = doc.createTextNode(text);
          stack[stack.length - 1].appendChild(tNode);
        }
      }
      lastIndex = tokenRegex.lastIndex;

      if (match[1] !== undefined) {
        const cNode = doc.createComment(match[1]);
        if (stack.length === 1) {
          cNode.parentNode = doc;
          doc._children.push(cNode);
        } else {
          stack[stack.length - 1].appendChild(cNode);
        }
      } else if (match[2] !== undefined) {
        if (stack.length > 1) {
          const tNode = doc.createTextNode(match[2]);
          stack[stack.length - 1].appendChild(tNode);
        }
      } else if (match[3] !== undefined) {
        const cNode = doc.createComment("?" + match[3]);
        if (stack.length === 1) {
          cNode.parentNode = doc;
          doc._children.push(cNode);
        } else {
          stack[stack.length - 1].appendChild(cNode);
        }
      } else if (match[4] !== undefined) {
        const endTag = match[4].toLowerCase();
        for (let i = stack.length - 1; i > 0; i--) {
          if (stack[i].localName === endTag) {
            stack.length = i;
            break;
          }
        }
      } else if (match[5] !== undefined) {
        const tagName = match[5];
        const attrStr = match[6] || "";
        const isSelfClosing = Boolean(match[7]);

        const el = doc.createElement(tagName);
        applyAttributes(el, attrStr);

        if (stack.length === 1) {
          el.parentNode = doc;
          doc._children.push(el);
          doc.documentElement = el;
          if (!isSelfClosing) {
            stack.push(el);
          }
        } else {
          stack[stack.length - 1].appendChild(el);
          if (!isSelfClosing) {
            stack.push(el);
          }
        }
      }
    }

    if (lastIndex < input.length) {
      const text = unescapeEntities(input.slice(lastIndex));
      if (text.length > 0 && stack.length > 1) {
        const tNode = doc.createTextNode(text);
        stack[stack.length - 1].appendChild(tNode);
      }
    }
  }

  return doc;
}

function applyAttributes(el, attrStr) {
  if (!attrStr) return;
  const attrRegex = /([a-zA-Z0-9_:-]+)(?:\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s>]+)))?/g;
  let m;
  while ((m = attrRegex.exec(attrStr)) !== null) {
    const name = m[1];
    const val = m[2] !== undefined ? m[2] : (m[3] !== undefined ? m[3] : (m[4] !== undefined ? m[4] : ""));
    el.setAttribute(name, unescapeEntities(val));
  }
}

function DOMParser() {}

DOMParser.prototype.parseFromString = function (str, mimeType) {
  return parseDocument(str, mimeType);
};

markNative(DOMParser, "DOMParser");
markNative(DOMParser.prototype.parseFromString, "parseFromString");

function XMLSerializer() {}

function serializeXmlDoctype(dt) {
  if (!dt || !dt.name) return "";
  let out = "<!DOCTYPE " + dt.name;
  if (dt.publicId) {
    out += ' PUBLIC "' + dt.publicId + '"';
    if (dt.systemId !== undefined && dt.systemId !== null) {
      out += ' "' + dt.systemId + '"';
    }
  } else if (dt.systemId) {
    out += ' SYSTEM "' + dt.systemId + '"';
  }
  return out + ">";
}

function serializeXmlNode(node) {
  if (!node || typeof node !== "object") {
    throw new TypeError("Failed to execute 'serializeToString' on 'XMLSerializer': parameter 1 is not of type 'Node'.");
  }
  const nt = node.nodeType;
  if (nt === 9) {
    const cs = node.childNodes || [];
    const hasOverrideDt = Object.prototype.hasOwnProperty.call(node, "_reopenDoctype");
    let out = "";
    let sawDoctype = false;
    for (let i = 0; i < cs.length; i++) {
      if (cs[i] && cs[i].nodeType === 10) {
        if (hasOverrideDt) continue;
        sawDoctype = true;
      }
      out += serializeXmlNode(cs[i]);
    }
    if (hasOverrideDt) {
      if (node._reopenDoctype) {
        out = serializeXmlDoctype(node._reopenDoctype) + out;
      }
    } else if (!sawDoctype && node.doctype) {
      out = serializeXmlDoctype(node.doctype) + out;
    }
    if (!out && node.documentElement) {
      out = serializeXmlNode(node.documentElement);
    }
    return out;
  }
  if (nt === 11) {
    const cs = node.childNodes || [];
    let out = "";
    for (let i = 0; i < cs.length; i++) out += serializeXmlNode(cs[i]);
    return out;
  }
  if (nt === 10) {
    const ownerDoc = node.ownerDocument || (typeof document !== "undefined" ? document : null);
    if (ownerDoc && Object.prototype.hasOwnProperty.call(ownerDoc, "_reopenDoctype")) {
      return ownerDoc._reopenDoctype ? serializeXmlDoctype(ownerDoc._reopenDoctype) : "";
    }
    return serializeXmlDoctype(node);
  }
  if (nt === 3) {
    return String(node.data !== undefined ? node.data : (node.textContent || ""))
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;");
  }
  if (nt === 8) return "<!--" + String(node.data !== undefined ? node.data : (node.textContent || "")) + "-->";
  if (nt === 7) return "<?" + String(node.target || "") + " " + String(node.data || "") + "?>";
  if (nt === 1 && (node instanceof ParsedElement)) {
    const tag = node.localName || node.tagName || "div";
    let attrsStr = "";
    if (node._attrs instanceof Map) {
      node._attrs.forEach((val, key) => {
        const origName = (node._attrCases && node._attrCases.get(key)) || key;
        const v = String(val || "").replace(/&/g, "&amp;").replace(/"/g, "&quot;").replace(/</g, "&lt;");
        attrsStr += " " + origName + '="' + v + '"';
      });
    } else if (node.attributes) {
      const attrs = node.attributes;
      for (let i = 0; i < attrs.length; i++) {
        const a = attrs[i];
        if (!a) continue;
        const v = String(a.value || "").replace(/&/g, "&amp;").replace(/"/g, "&quot;").replace(/</g, "&lt;");
        attrsStr += " " + a.name + '="' + v + '"';
      }
    }
    const cs = node.childNodes || [];
    let inner = "";
    for (let i = 0; i < cs.length; i++) inner += serializeXmlNode(cs[i]);
    return "<" + tag + attrsStr + ">" + inner + "</" + tag + ">";
  }
  if (typeof node.outerHTML === "string") return node.outerHTML;
  return "";
}

XMLSerializer.prototype.serializeToString = function (root) {
  return serializeXmlNode(root);
};

markNative(XMLSerializer, "XMLSerializer");
markNative(XMLSerializer.prototype.serializeToString, "serializeToString");

if (typeof globalThis !== "undefined") {
  globalThis.DOMParser = DOMParser;
  globalThis.XMLSerializer = XMLSerializer;
}
if (typeof window !== "undefined") {
  window.DOMParser = DOMParser;
  window.XMLSerializer = XMLSerializer;
}
