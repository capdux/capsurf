// src/js/shim/xpath.js
// CapSurf XPath 1.0 Specification Engine
// Aligned with vendor/kitesurf/packages/dom-shim/src/xpath.ts

const XPATH_RESULT_TYPES = {
  ANY_TYPE: 0,
  NUMBER_TYPE: 1,
  STRING_TYPE: 2,
  BOOLEAN_TYPE: 3,
  UNORDERED_NODE_ITERATOR_TYPE: 4,
  ORDERED_NODE_ITERATOR_TYPE: 5,
  UNORDERED_NODE_SNAPSHOT_TYPE: 6,
  ORDERED_NODE_SNAPSHOT_TYPE: 7,
  ANY_UNORDERED_NODE_TYPE: 8,
  FIRST_ORDERED_NODE_TYPE: 9,
};

const AXES = new Set([
  "ancestor",
  "ancestor-or-self",
  "attribute",
  "child",
  "descendant",
  "descendant-or-self",
  "following",
  "following-sibling",
  "namespace",
  "parent",
  "preceding",
  "preceding-sibling",
  "self",
]);

const REVERSE_AXES = new Set([
  "ancestor",
  "ancestor-or-self",
  "preceding",
  "preceding-sibling",
]);

const NODE_TYPES = new Set(["comment", "text", "processing-instruction", "node"]);
const OPERATOR_NAMES = new Set(["and", "or", "mod", "div"]);

const EOF_TOKEN = { t: "eof", v: "" };

function getDOMException(message, name) {
  if (typeof DOMException !== "undefined") {
    return new DOMException(message, name);
  }
  if (typeof globalThis !== "undefined" && globalThis.DOMException) {
    return new globalThis.DOMException(message, name);
  }
  const err = new Error(message);
  err.name = name;
  return err;
}

function syntaxError(src) {
  return getDOMException("The expression is not a legal expression.", "SyntaxError");
}

function namespaceError(prefix) {
  return getDOMException(
    "The expression contains an unresolvable namespace prefix.",
    "NamespaceError"
  );
}

function isNameStart(codePoint) {
  return (
    codePoint === 0x5f ||
    (codePoint >= 0x41 && codePoint <= 0x5a) ||
    (codePoint >= 0x61 && codePoint <= 0x7a) ||
    (codePoint >= 0xc0 && codePoint <= 0xd6) ||
    (codePoint >= 0xd8 && codePoint <= 0xf6) ||
    (codePoint >= 0xf8 && codePoint <= 0x2ff) ||
    (codePoint >= 0x370 && codePoint <= 0x37d) ||
    (codePoint >= 0x37f && codePoint <= 0x1fff) ||
    (codePoint >= 0x200c && codePoint <= 0x200d) ||
    (codePoint >= 0x2070 && codePoint <= 0x218f) ||
    (codePoint >= 0x2c00 && codePoint <= 0x2fef) ||
    (codePoint >= 0x3001 && codePoint <= 0xd7ff) ||
    (codePoint >= 0xf900 && codePoint <= 0xfdcf) ||
    (codePoint >= 0xfdf0 && codePoint <= 0xfffd) ||
    (codePoint >= 0x10000 && codePoint <= 0xeffff)
  );
}

function isNameChar(codePoint) {
  return (
    isNameStart(codePoint) ||
    (codePoint >= 0x30 && codePoint <= 0x39) ||
    codePoint === 0x2d ||
    codePoint === 0x2e ||
    codePoint === 0xb7 ||
    (codePoint >= 0x300 && codePoint <= 0x36f) ||
    (codePoint >= 0x203f && codePoint <= 0x2040)
  );
}

function lex(src) {
  const toks = [];
  const n = src.length;
  let i = 0;

  const operatorMayFollow = function () {
    const p = toks[toks.length - 1];
    if (!p) return false;
    if (p.t === "op") return false;
    return !(
      p.t === "punct" &&
      (p.v === "@" || p.v === "::" || p.v === "(" || p.v === "[" || p.v === ",")
    );
  };

  const readNcName = function () {
    const start = i;
    const first = src.codePointAt(i) ?? -1;
    if (!isNameStart(first)) throw syntaxError(src);
    i += first > 0xffff ? 2 : 1;
    while (i < n) {
      const next = src.codePointAt(i) ?? -1;
      if (!isNameChar(next)) break;
      i += next > 0xffff ? 2 : 1;
    }
    return src.slice(start, i);
  };

  const peekNonSpace = function (from) {
    let j = from;
    while (j < n && " \t\r\n".includes(src.charAt(j))) j++;
    return j;
  };

  while (i < n) {
    const c = src.charAt(i);
    if (" \t\r\n".includes(c)) {
      i++;
      continue;
    }
    if (c === '"' || c === "'") {
      const end = src.indexOf(c, i + 1);
      if (end < 0) throw syntaxError(src);
      toks.push({ t: "str", v: src.slice(i + 1, end) });
      i = end + 1;
      continue;
    }
    if (/[0-9]/.test(c) || (c === "." && /[0-9]/.test(src[i + 1] ?? ""))) {
      const m = /^(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)/.exec(src.slice(i));
      if (!m) throw syntaxError(src);
      toks.push({ t: "num", v: m[0] });
      i += m[0].length;
      continue;
    }
    const two = src.slice(i, i + 2);
    if (two === ".." || two === "::") {
      toks.push({ t: "punct", v: two });
      i += 2;
      continue;
    }
    if ("().[]@,".includes(c)) {
      toks.push({ t: "punct", v: c });
      i++;
      continue;
    }
    if (two === "//" || two === "!=" || two === "<=" || two === ">=") {
      toks.push({ t: "op", v: two });
      i += 2;
      continue;
    }
    if ("/|+-=<>".includes(c)) {
      toks.push({ t: "op", v: c });
      i++;
      continue;
    }
    if (c === "*") {
      toks.push(operatorMayFollow() ? { t: "op", v: "*" } : { t: "name", v: "*" });
      i++;
      continue;
    }
    if (c === "$") {
      i++;
      let name = readNcName();
      if (src[i] === ":" && src[i + 1] !== ":") {
        i++;
        name += ":" + readNcName();
      }
      toks.push({ t: "var", v: name });
      continue;
    }
    if (!isNameStart(src.codePointAt(i) ?? -1)) throw syntaxError(src);
    let name = readNcName();
    if (operatorMayFollow()) {
      if (!OPERATOR_NAMES.has(name)) throw syntaxError(src);
      toks.push({ t: "op", v: name });
      continue;
    }
    if (src[i] === ":" && src[i + 1] !== ":") {
      i++;
      if (src[i] === "*") {
        i++;
        toks.push({ t: "name", v: `${name}:*` });
        continue;
      }
      name += ":" + readNcName();
    }
    const next = peekNonSpace(i);
    if (src[next] === "(") {
      toks.push({ t: NODE_TYPES.has(name) ? "type" : "fn", v: name });
    } else if (src.slice(next, next + 2) === "::") {
      if (!AXES.has(name)) throw syntaxError(src);
      toks.push({ t: "axis", v: name });
    } else {
      toks.push({ t: "name", v: name });
    }
  }
  toks.push(EOF_TOKEN);
  return toks;
}

function resolvePrefix(resolver, prefix) {
  let ns = null;
  try {
    if (typeof resolver === "function") {
      ns = resolver(prefix);
    } else if (resolver !== null && typeof resolver === "object") {
      const lookupNamespaceURI = resolver.lookupNamespaceURI;
      if (typeof lookupNamespaceURI !== "function") {
        throw new TypeError("XPathNSResolver.lookupNamespaceURI is not callable");
      }
      ns = lookupNamespaceURI.call(resolver, prefix);
    }
  } catch (error) {
    throw namespaceError(prefix);
  }
  if (ns === null || ns === undefined) {
    throw namespaceError(prefix);
  }
  if (typeof ns === "symbol") {
    throw namespaceError(prefix);
  }
  try {
    return String(ns);
  } catch (error) {
    throw namespaceError(prefix);
  }
}

class Parser {
  constructor(src, toks, resolver) {
    this.src = src;
    this.toks = toks;
    this.resolver = resolver;
    this.i = 0;
  }

  parse() {
    const e = this.orExpr();
    if (this.peek().t !== "eof") throw syntaxError(this.src);
    return e;
  }

  peek(k = 0) {
    return this.toks[Math.min(this.i + k, this.toks.length - 1)] ?? EOF_TOKEN;
  }

  next() {
    return this.toks[this.i++] ?? EOF_TOKEN;
  }

  is(t, v) {
    const p = this.peek();
    return p.t === t && (v === undefined || p.v === v);
  }

  expect(t, v) {
    if (!this.is(t, v)) throw syntaxError(this.src);
    return this.next();
  }

  binary(sub, ops, make) {
    let l = sub.call(this);
    while (this.peek().t === "op" && ops.includes(this.peek().v)) {
      const op = this.next().v;
      l = make(op, l, sub.call(this));
    }
    return l;
  }

  orExpr() {
    return this.binary(
      this.andExpr,
      ["or"],
      (_op, l, r) => ({ t: "or", l, r })
    );
  }

  andExpr() {
    return this.binary(
      this.equalityExpr,
      ["and"],
      (_op, l, r) => ({ t: "and", l, r })
    );
  }

  equalityExpr() {
    return this.binary(
      this.relationalExpr,
      ["=", "!="],
      (op, l, r) => ({ t: "cmp", op, l, r })
    );
  }

  relationalExpr() {
    return this.binary(
      this.additiveExpr,
      ["<", ">", "<=", ">="],
      (op, l, r) => ({ t: "cmp", op, l, r })
    );
  }

  additiveExpr() {
    return this.binary(
      this.multiplicativeExpr,
      ["+", "-"],
      (op, l, r) => ({ t: "arith", op, l, r })
    );
  }

  multiplicativeExpr() {
    return this.binary(
      this.unaryExpr,
      ["*", "div", "mod"],
      (op, l, r) => ({ t: "arith", op, l, r })
    );
  }

  unaryExpr() {
    if (this.is("op", "-")) {
      this.next();
      return { t: "neg", e: this.unaryExpr() };
    }
    return this.unionExpr();
  }

  unionExpr() {
    return this.binary(
      this.pathExpr,
      ["|"],
      (_op, l, r) => ({ t: "union", l, r })
    );
  }

  pathExpr() {
    const p = this.peek();
    const startsFilter =
      p.t === "var" ||
      p.t === "str" ||
      p.t === "num" ||
      p.t === "fn" ||
      (p.t === "punct" && p.v === "(");
    if (!startsFilter) return this.locationPath();
    const primary = this.primaryExpr();
    const preds = this.predicates();
    let e = preds.length ? { t: "filter", e: primary, preds } : primary;
    if (this.is("op", "/") || this.is("op", "//")) {
      const steps = [];
      this.relativePath(steps, true);
      e = { t: "path", start: e, steps };
    }
    return e;
  }

  primaryExpr() {
    const p = this.next();
    if (p.t === "num") return { t: "num", v: Number(p.v) };
    if (p.t === "str") return { t: "str", v: p.v };
    if (p.t === "var") throw syntaxError(this.src);
    if (p.t === "punct" && p.v === "(") {
      const e = this.orExpr();
      this.expect("punct", ")");
      return e;
    }
    if (p.t === "fn") {
      const fn = FUNCTIONS.get(p.v);
      if (!fn) throw syntaxError(this.src);
      this.expect("punct", "(");
      const args = [];
      if (!this.is("punct", ")")) {
        args.push(this.orExpr());
        while (this.is("punct", ",")) {
          this.next();
          args.push(this.orExpr());
        }
      }
      this.expect("punct", ")");
      const [min, max] = fn.arity;
      if (args.length < min || args.length > max) throw syntaxError(this.src);
      return { t: "fn", name: p.v, args };
    }
    throw syntaxError(this.src);
  }

  predicates() {
    const preds = [];
    while (this.is("punct", "[")) {
      this.next();
      preds.push(this.orExpr());
      this.expect("punct", "]");
    }
    return preds;
  }

  locationPath() {
    const steps = [];
    if (this.is("op", "/")) {
      this.next();
      if (this.startsStep()) this.relativePath(steps, false);
      return { t: "path", start: "root", steps };
    }
    if (this.is("op", "//")) {
      this.relativePath(steps, true);
      return { t: "path", start: "root", steps };
    }
    this.relativePath(steps, false);
    return { t: "path", start: "context", steps };
  }

  startsStep() {
    const p = this.peek();
    return (
      p.t === "name" ||
      p.t === "type" ||
      p.t === "axis" ||
      (p.t === "punct" && (p.v === "@" || p.v === "." || p.v === ".."))
    );
  }

  relativePath(steps, leadingSeparator) {
    if (leadingSeparator) this.separator(steps);
    steps.push(this.step());
    while (this.is("op", "/") || this.is("op", "//")) {
      this.separator(steps);
      steps.push(this.step());
    }
  }

  separator(steps) {
    const sep = this.next().v;
    if (sep === "//") steps.push({ axis: "descendant-or-self", test: { k: "node" }, preds: [] });
  }

  step() {
    if (this.is("punct", ".")) {
      this.next();
      return { axis: "self", test: { k: "node" }, preds: [] };
    }
    if (this.is("punct", "..")) {
      this.next();
      return { axis: "parent", test: { k: "node" }, preds: [] };
    }
    let axis = "child";
    if (this.is("axis")) {
      axis = this.next().v;
      this.expect("punct", "::");
    } else if (this.is("punct", "@")) {
      this.next();
      axis = "attribute";
    }
    const test = this.nodeTest();
    return { axis, test, preds: this.predicates() };
  }

  nodeTest() {
    const p = this.next();
    if (p.t === "type") {
      this.expect("punct", "(");
      let target = null;
      if (p.v === "processing-instruction" && this.is("str")) target = this.next().v;
      this.expect("punct", ")");
      if (p.v === "node") return { k: "node" };
      if (p.v === "text") return { k: "text" };
      if (p.v === "comment") return { k: "comment" };
      return { k: "pi", target };
    }
    if (p.t !== "name") throw syntaxError(this.src);
    if (p.v === "*") return { k: "any", ns: null };
    const colon = p.v.indexOf(":");
    if (colon < 0) return { k: "name", ns: null, local: p.v };
    const ns = resolvePrefix(this.resolver, p.v.slice(0, colon));
    const local = p.v.slice(colon + 1);
    return local === "*" ? { k: "any", ns } : { k: "name", ns, local };
  }
}

function compile(src, resolver) {
  return new Parser(src, lex(src), resolver).parse();
}

function isAttr(n) {
  return n?.nodeType === 2 && ("ownerElement" in n || "specified" in n);
}

function parentOf(n) {
  if (isAttr(n)) return n.ownerElement || null;
  const p = n.parentNode || n.parentElement;
  if (!p && n.ownerDocument && n !== n.ownerDocument) {
    if (n === n.ownerDocument.documentElement) return n.ownerDocument;
  }
  return p || null;
}

function inModel(n) {
  const t = n.nodeType;
  return t !== 10 && t !== 12 && t !== 5 && t !== 6;
}

function firstChild(n) {
  let c;
  if (n.nodeType === 9) { // DOCUMENT_NODE
    c = n.firstChild || n.documentElement;
  } else {
    c = isAttr(n) ? null : n.firstChild;
  }
  while (c && !inModel(c)) c = c.nextSibling;
  return c || null;
}

function lastChild(n) {
  let c;
  if (n.nodeType === 9) {
    c = n.lastChild || n.documentElement;
  } else {
    c = isAttr(n) ? null : n.lastChild;
  }
  while (c && !inModel(c)) c = c.previousSibling;
  return c || null;
}

function nextSibling(n) {
  let c = isAttr(n) ? null : n.nextSibling;
  while (c && !inModel(c)) c = c.nextSibling;
  return c || null;
}

function previousSibling(n) {
  let c = isAttr(n) ? null : n.previousSibling;
  while (c && !inModel(c)) c = c.previousSibling;
  return c || null;
}

function descendants(n, out) {
  for (let c = firstChild(n); c; c = nextSibling(c)) {
    out.push(c);
    descendants(c, out);
  }
}

function descendantsReversed(n, out) {
  for (let c = lastChild(n); c; c = previousSibling(c)) {
    descendantsReversed(c, out);
    out.push(c);
  }
}

function attributesOf(n) {
  if (!n || n.nodeType !== 1) return []; // ELEMENT_NODE
  const list = n.attributes;
  const out = [];
  if (!list) return out;
  const len = list.length || 0;
  for (let i = 0; i < len; i++) {
    const a = list[i];
    if (
      !a ||
      a.namespaceURI === "http://www.w3.org/2000/xmlns/" ||
      a.name === "xmlns" ||
      String(a.name).startsWith("xmlns:")
    ) {
      continue;
    }
    out.push(a);
  }
  return out;
}

function axisNodes(axis, n) {
  const out = [];
  switch (axis) {
    case "self":
      out.push(n);
      break;
    case "child":
      for (let c = firstChild(n); c; c = nextSibling(c)) out.push(c);
      break;
    case "descendant":
      descendants(n, out);
      break;
    case "descendant-or-self":
      out.push(n);
      descendants(n, out);
      break;
    case "parent": {
      const p = parentOf(n);
      if (p) out.push(p);
      break;
    }
    case "ancestor":
    case "ancestor-or-self":
      if (axis === "ancestor-or-self") out.push(n);
      for (let p = parentOf(n); p; p = parentOf(p)) out.push(p);
      break;
    case "following-sibling":
      for (let s = nextSibling(n); s; s = nextSibling(s)) out.push(s);
      break;
    case "preceding-sibling":
      for (let s = previousSibling(n); s; s = previousSibling(s)) out.push(s);
      break;
    case "following": {
      let a = n;
      if (isAttr(n)) {
        a = n.ownerElement;
        if (!a) break;
        descendants(a, out);
      }
      for (; a; a = parentOf(a)) {
        for (let s = nextSibling(a); s; s = nextSibling(s)) {
          out.push(s);
          descendants(s, out);
        }
      }
      break;
    }
    case "preceding": {
      for (let a = isAttr(n) ? n.ownerElement : n; a; a = parentOf(a)) {
        for (let s = previousSibling(a); s; s = previousSibling(s)) {
          descendantsReversed(s, out);
          out.push(s);
        }
      }
      break;
    }
    case "attribute":
      out.push(...attributesOf(n));
      break;
    case "namespace":
      break;
  }
  return out;
}

function isHtmlDocumentNode(el) {
  if (!el) return false;
  const doc = el.ownerDocument || (el.nodeType === 9 ? el : (typeof document !== "undefined" ? document : null));
  if (!doc) return false;
  if (doc.contentType) return doc.contentType === "text/html";
  return true;
}

function asciiLowercase(s) {
  return String(s || "").toLowerCase();
}

function matches(test, axis, n) {
  const t = n.nodeType;
  switch (test.k) {
    case "node":
      return true;
    case "text":
      return t === 3 || t === 4; // TEXT_NODE || CDATA_SECTION_NODE
    case "comment":
      return t === 8; // COMMENT_NODE
    case "pi":
      return t === 7 && (test.target === null || n.target === test.target); // PROCESSING_INSTRUCTION_NODE
  }
  if (axis === "attribute") {
    if (t !== 2) return false; // ATTRIBUTE_NODE
    const ns = n.namespaceURI || null;
    if (test.k === "any") return test.ns === null || ns === test.ns;
    const owner = n.ownerElement;
    if (
      test.ns === null &&
      ns === null &&
      owner &&
      (owner.namespaceURI === "http://www.w3.org/1999/xhtml" || !owner.namespaceURI) &&
      isHtmlDocumentNode(owner)
    ) {
      return asciiLowercase(n.localName || n.name) === asciiLowercase(test.local);
    }
    return (n.localName || n.name) === test.local && ns === test.ns;
  }
  if (t !== 1) return false; // ELEMENT_NODE
  const ns = n.namespaceURI || null;
  if (test.k === "any") return test.ns === null || ns === test.ns;
  if (isHtmlDocumentNode(n)) {
    if (ns === "http://www.w3.org/1999/xhtml" || ns === null) {
      return (
        asciiLowercase(n.localName || n.tagName) === asciiLowercase(test.local) &&
        (test.ns === null || test.ns === "http://www.w3.org/1999/xhtml")
      );
    }
    return (n.localName || n.tagName) === test.local && test.ns !== null && ns === test.ns;
  }
  return (n.localName || n.tagName) === test.local && ns === test.ns;
}

class Evaluation {
  constructor(root) {
    this.root = root;
    this.order = null;
  }

  index() {
    if (this.order) return this.order;
    const order = new Map();
    let k = 0;
    const visit = (n) => {
      order.set(n, k++);
      if (n.nodeType === 1) { // ELEMENT_NODE
        const attrs = attributesOf(n);
        for (let i = 0; i < attrs.length; i++) order.set(attrs[i], k++);
      }
      for (let c = firstChild(n); c; c = nextSibling(c)) visit(c);
    };
    visit(this.root);
    this.order = order;
    return order;
  }

  sort(nodes) {
    const unique = [...new Set(nodes)];
    if (unique.length < 2) return unique;
    const order = this.index();
    const key = (n) => {
      const parent = parentOf(n);
      return order.get(n) ?? (parent ? order.get(parent) : undefined) ?? Number.MAX_SAFE_INTEGER;
    };
    return unique.sort((a, b) => key(a) - key(b));
  }
}

function rootOf(node) {
  let root = node;
  for (let parent = parentOf(root); parent; parent = parentOf(parent)) {
    root = parent;
  }
  return root;
}

function stringValue(n) {
  switch (n.nodeType) {
    case 1: // ELEMENT_NODE
    case 11: // DOCUMENT_FRAGMENT_NODE
      return String(n.textContent ?? "");
    case 9: { // DOCUMENT_NODE
      let s = "";
      for (let c = firstChild(n); c; c = nextSibling(c)) {
        if (c.nodeType === 1 || c.nodeType === 3 || c.nodeType === 4) {
          s += c.textContent ?? "";
        }
      }
      return s;
    }
    case 2: // ATTRIBUTE_NODE
      return String(n.value ?? "");
    default:
      return String(n.nodeValue ?? n.data ?? n.textContent ?? "");
  }
}

function isNodeSet(v) {
  return Array.isArray(v);
}

function toBoolean(v) {
  if (isNodeSet(v)) return v.length > 0;
  if (typeof v === "number") return v !== 0 && !Number.isNaN(v);
  if (typeof v === "string") return v.length > 0;
  return Boolean(v);
}

function stringToNumber(s) {
  return /^[\t\n\r ]*-?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)[\t\n\r ]*$/.test(s)
    ? Number(s.trim())
    : NaN;
}

function toNumber(v) {
  if (isNodeSet(v)) return stringToNumber(toStringValue(v));
  if (typeof v === "string") return stringToNumber(v);
  if (typeof v === "boolean") return v ? 1 : 0;
  return Number(v);
}

function numberToString(x) {
  if (Number.isNaN(x)) return "NaN";
  if (x === Infinity) return "Infinity";
  if (x === -Infinity) return "-Infinity";
  if (x === 0) return "0";
  const s = String(x);
  if (!/e/i.test(s)) return s;
  const [mant, expStr] = s.toLowerCase().split("e");
  const neg = mant.startsWith("-");
  const digits = mant.replace(/^-/, "").replace(".", "");
  const intLen = (mant.replace(/^-/, "").split(".")[0] || "").length;
  const exp = Number(expStr) + intLen;
  let out;
  if (exp <= 0) out = "0." + "0".repeat(-exp) + digits;
  else if (exp >= digits.length) out = digits + "0".repeat(exp - digits.length);
  else out = digits.slice(0, exp) + "." + digits.slice(exp);
  return (neg ? "-" : "") + out;
}

function toStringValue(v) {
  if (isNodeSet(v)) return v.length ? stringValue(v[0]) : "";
  if (typeof v === "number") return numberToString(v);
  if (typeof v === "boolean") return v ? "true" : "false";
  return String(v);
}

function compareAtoms(op, a, b) {
  if (op === "=" || op === "!=") {
    let eq;
    if (typeof a === "boolean" || typeof b === "boolean") eq = toBoolean(a) === toBoolean(b);
    else if (typeof a === "number" || typeof b === "number") eq = toNumber(a) === toNumber(b);
    else eq = toStringValue(a) === toStringValue(b);
    return op === "=" ? eq : !eq;
  }
  const x = toNumber(a);
  const y = toNumber(b);
  if (op === "<") return x < y;
  if (op === "<=") return x <= y;
  if (op === ">") return x > y;
  return x >= y;
}

function compare(op, a, b) {
  if (isNodeSet(a) && isNodeSet(b)) {
    const bs = b.map(stringValue);
    return a.some((n) => {
      const s = stringValue(n);
      return bs.some((t) => compareAtoms(op, s, t));
    });
  }
  if (isNodeSet(a) || isNodeSet(b)) {
    const set = isNodeSet(a) ? a : b;
    const other = isNodeSet(a) ? b : a;
    if (typeof other === "boolean") {
      return isNodeSet(a)
        ? compareAtoms(op, toBoolean(set), other)
        : compareAtoms(op, other, toBoolean(set));
    }
    return set.some((n) => {
      const s = typeof other === "number" ? stringToNumber(stringValue(n)) : stringValue(n);
      return isNodeSet(a) ? compareAtoms(op, s, other) : compareAtoms(op, other, s);
    });
  }
  return compareAtoms(op, a, b);
}

function nodeSetOf(v) {
  if (!isNodeSet(v)) throw new TypeError("The expression does not evaluate to a node-set.");
  return v;
}

function xpathRound(x) {
  if (!Number.isFinite(x)) return x;
  if (x < 0 && x >= -0.5) return -0;
  return Math.floor(x + 0.5);
}

function localNameOf(n) {
  const t = n.nodeType;
  if (t === 1 || t === 2) return String(n.localName ?? "");
  if (t === 7) return String(n.target ?? "");
  return "";
}

function qualifiedNameOf(n) {
  const t = n.nodeType;
  if (t === 1 || t === 2) {
    return n.prefix ? `${n.prefix}:${n.localName}` : String(n.localName ?? "");
  }
  if (t === 7) return String(n.target ?? "");
  return "";
}

function firstOf(c, args) {
  if (!args.length) return c.node;
  const set = c.ev.sort(nodeSetOf(evaluate(arg(args, 0), c)));
  return set[0] ?? null;
}

function idLookup(c, ids) {
  const root = c.ev.root;
  const found = [];
  const wanted = new Set(ids.filter(Boolean));
  if (!wanted.size) return found;
  const walk = (n) => {
    if (n.nodeType === 1) {
      const id = typeof n.getAttribute === "function" ? n.getAttribute("id") : n.id;
      if (id !== null && id !== undefined && wanted.has(id)) {
        found.push(n);
        wanted.delete(id);
      }
    }
    for (let k = firstChild(n); k && wanted.size; k = nextSibling(k)) walk(k);
  };
  walk(root);
  return c.ev.sort(found);
}

function arg(args, i) {
  const e = args[i];
  if (!e) throw new Error(`xpath: missing argument ${i}`);
  return e;
}

const str = (c, args, i) => toStringValue(evaluate(arg(args, i), c));
const num = (c, args, i) => toNumber(evaluate(arg(args, i), c));

const FUNCTIONS = new Map(
  Object.entries({
    last: { arity: [0, 0], run: (c) => c.size },
    position: { arity: [0, 0], run: (c) => c.pos },
    count: { arity: [1, 1], run: (c, a) => nodeSetOf(evaluate(arg(a, 0), c)).length },
    id: {
      arity: [1, 1],
      run: (c, a) => {
        const v = evaluate(arg(a, 0), c);
        const tokens = isNodeSet(v)
          ? v.flatMap((n) => stringValue(n).split(/[\t\n\r ]+/))
          : toStringValue(v).split(/[\t\n\r ]+/);
        return idLookup(c, tokens);
      },
    },
    "local-name": {
      arity: [0, 1],
      run: (c, a) => {
        const n = firstOf(c, a);
        return n ? localNameOf(n) : "";
      },
    },
    "namespace-uri": {
      arity: [0, 1],
      run: (c, a) => {
        const n = firstOf(c, a);
        return n && (n.nodeType === 1 || n.nodeType === 2)
          ? String(n.namespaceURI ?? "")
          : "";
      },
    },
    name: {
      arity: [0, 1],
      run: (c, a) => {
        const n = firstOf(c, a);
        return n ? qualifiedNameOf(n) : "";
      },
    },
    string: { arity: [0, 1], run: (c, a) => (a.length ? str(c, a, 0) : stringValue(c.node)) },
    concat: { arity: [2, Infinity], run: (c, a) => a.map((_, i) => str(c, a, i)).join("") },
    "starts-with": { arity: [2, 2], run: (c, a) => str(c, a, 0).startsWith(str(c, a, 1)) },
    contains: { arity: [2, 2], run: (c, a) => str(c, a, 0).includes(str(c, a, 1)) },
    "substring-before": {
      arity: [2, 2],
      run: (c, a) => {
        const s = str(c, a, 0);
        const i = s.indexOf(str(c, a, 1));
        return i < 0 ? "" : s.slice(0, i);
      },
    },
    "substring-after": {
      arity: [2, 2],
      run: (c, a) => {
        const s = str(c, a, 0);
        const t = str(c, a, 1);
        const i = s.indexOf(t);
        return i < 0 ? "" : s.slice(i + t.length);
      },
    },
    substring: {
      arity: [2, 3],
      run: (c, a) => {
        const chars = Array.from(str(c, a, 0));
        const start = xpathRound(num(c, a, 1));
        const end = a.length > 2 ? start + xpathRound(num(c, a, 2)) : Infinity;
        return chars.filter((_, i) => i + 1 >= start && i + 1 < end).join("");
      },
    },
    "string-length": {
      arity: [0, 1],
      run: (c, a) => Array.from(a.length ? str(c, a, 0) : stringValue(c.node)).length,
    },
    "normalize-space": {
      arity: [0, 1],
      run: (c, a) =>
        (a.length ? str(c, a, 0) : stringValue(c.node))
          .replace(/[\t\n\r ]+/g, " ")
          .replace(/^ | $/g, ""),
    },
    translate: {
      arity: [3, 3],
      run: (c, a) => {
        const from = Array.from(str(c, a, 1));
        const to = Array.from(str(c, a, 2));
        return Array.from(str(c, a, 0))
          .map((ch) => {
            const i = from.indexOf(ch);
            return i < 0 ? ch : (to[i] ?? "");
          })
          .join("");
      },
    },
    boolean: { arity: [1, 1], run: (c, a) => toBoolean(evaluate(arg(a, 0), c)) },
    not: { arity: [1, 1], run: (c, a) => !toBoolean(evaluate(arg(a, 0), c)) },
    true: { arity: [0, 0], run: () => true },
    false: { arity: [0, 0], run: () => false },
    lang: {
      arity: [1, 1],
      run: (c, a) => {
        const want = str(c, a, 0).toLowerCase();
        for (let n = isAttr(c.node) ? c.node.ownerElement : c.node; n; n = parentOf(n)) {
          if (n.nodeType !== 1) continue;
          const lang = typeof n.getAttributeNS === "function"
            ? (n.getAttributeNS("http://www.w3.org/XML/1998/namespace", "lang") ?? n.getAttribute?.("xml:lang") ?? n.getAttribute?.("lang"))
            : (typeof n.getAttribute === "function" ? n.getAttribute("lang") : null);
          if (lang === null || lang === undefined) continue;
          const l = String(lang).toLowerCase();
          return l === want || l.startsWith(want + "-");
        }
        return false;
      },
    },
    number: {
      arity: [0, 1],
      run: (c, a) => (a.length ? num(c, a, 0) : stringToNumber(stringValue(c.node))),
    },
    sum: {
      arity: [1, 1],
      run: (c, a) =>
        nodeSetOf(evaluate(arg(a, 0), c)).reduce((s, n) => s + stringToNumber(stringValue(n)), 0),
    },
    floor: { arity: [1, 1], run: (c, a) => Math.floor(num(c, a, 0)) },
    ceiling: { arity: [1, 1], run: (c, a) => Math.ceil(num(c, a, 0)) },
    round: { arity: [1, 1], run: (c, a) => xpathRound(num(c, a, 0)) },
  })
);

function filterByPredicates(nodes, preds, ev) {
  let cur = nodes;
  for (const pred of preds) {
    const size = cur.length;
    cur = cur.filter((node, i) => {
      const v = evaluate(pred, { node, pos: i + 1, size, ev });
      return typeof v === "number" ? v === i + 1 : toBoolean(v);
    });
  }
  return cur;
}

function applyStep(step, contexts, ev) {
  const out = [];
  for (const n of contexts) {
    const picked = axisNodes(step.axis, n).filter((m) => matches(step.test, step.axis, m));
    out.push(...filterByPredicates(picked, step.preds, ev));
  }
  if (contexts.length === 1) return REVERSE_AXES.has(step.axis) ? out.reverse() : out;
  return ev.sort(out);
}

function evaluate(e, c) {
  switch (e.t) {
    case "num":
      return e.v;
    case "str":
      return e.v;
    case "fn": {
      const fn = FUNCTIONS.get(e.name);
      if (!fn) throw syntaxError(e.name);
      return fn.run(c, e.args);
    }
    case "or":
      return toBoolean(evaluate(e.l, c)) || toBoolean(evaluate(e.r, c));
    case "and":
      return toBoolean(evaluate(e.l, c)) && toBoolean(evaluate(e.r, c));
    case "cmp":
      return compare(e.op, evaluate(e.l, c), evaluate(e.r, c));
    case "arith": {
      const x = toNumber(evaluate(e.l, c));
      const y = toNumber(evaluate(e.r, c));
      if (e.op === "+") return x + y;
      if (e.op === "-") return x - y;
      if (e.op === "*") return x * y;
      if (e.op === "div") return x / y;
      return x % y;
    }
    case "neg":
      return -toNumber(evaluate(e.e, c));
    case "union":
      return c.ev.sort([...nodeSetOf(evaluate(e.l, c)), ...nodeSetOf(evaluate(e.r, c))]);
    case "filter":
      return filterByPredicates(c.ev.sort(nodeSetOf(evaluate(e.e, c))), e.preds, c.ev);
    case "path": {
      let nodes;
      if (e.start === "root") nodes = [c.ev.root];
      else if (e.start === "context") nodes = [c.node];
      else nodes = c.ev.sort(nodeSetOf(evaluate(e.start, c)));
      for (const step of e.steps) nodes = applyStep(step, nodes, c.ev);
      return nodes;
    }
  }
}

function isNode(v) {
  return !!v && typeof v === "object" && typeof v.nodeType === "number";
}

const INTERNAL_CONSTRUCT = Symbol("kitesurfInternalConstruct");
const xpathResultInstances = new WeakSet();

function need(ok, what) {
  if (!ok) throw new TypeError(`The result type is not ${what}.`);
}

function XPathResult(token, type = 0, value = false) {
  if (token !== INTERNAL_CONSTRUCT) throw new TypeError("Illegal constructor");
  this._type = type;
  this._value = value;
  this._seq = (typeof globalThis !== "undefined" && globalThis.__treeSeq) || 0;
  this._cursor = 0;
  xpathResultInstances.add(this);
}

XPathResult.prototype._refill = function (type, value) {
  this._type = type;
  this._value = value;
  this._cursor = 0;
  this._seq = (typeof globalThis !== "undefined" && globalThis.__treeSeq) || 0;
};

Object.defineProperty(XPathResult.prototype, "resultType", {
  get: function () {
    return this._type;
  },
  configurable: true,
});

Object.defineProperty(XPathResult.prototype, "numberValue", {
  get: function () {
    need(this._type === XPATH_RESULT_TYPES.NUMBER_TYPE, "a number");
    return this._value;
  },
  configurable: true,
});

Object.defineProperty(XPathResult.prototype, "stringValue", {
  get: function () {
    need(this._type === XPATH_RESULT_TYPES.STRING_TYPE, "a string");
    return this._value;
  },
  configurable: true,
});

Object.defineProperty(XPathResult.prototype, "booleanValue", {
  get: function () {
    need(this._type === XPATH_RESULT_TYPES.BOOLEAN_TYPE, "a boolean");
    return this._value;
  },
  configurable: true,
});

Object.defineProperty(XPathResult.prototype, "singleNodeValue", {
  get: function () {
    need(
      this._type === XPATH_RESULT_TYPES.ANY_UNORDERED_NODE_TYPE ||
        this._type === XPATH_RESULT_TYPES.FIRST_ORDERED_NODE_TYPE,
      "a single node"
    );
    return this._value[0] ?? null;
  },
  configurable: true,
});

Object.defineProperty(XPathResult.prototype, "invalidIteratorState", {
  get: function () {
    return (
      (this._type === XPATH_RESULT_TYPES.UNORDERED_NODE_ITERATOR_TYPE ||
        this._type === XPATH_RESULT_TYPES.ORDERED_NODE_ITERATOR_TYPE) &&
      ((typeof globalThis !== "undefined" && globalThis.__treeSeq) || 0) !== this._seq
    );
  },
  configurable: true,
});

Object.defineProperty(XPathResult.prototype, "snapshotLength", {
  get: function () {
    need(
      this._type === XPATH_RESULT_TYPES.UNORDERED_NODE_SNAPSHOT_TYPE ||
        this._type === XPATH_RESULT_TYPES.ORDERED_NODE_SNAPSHOT_TYPE,
      "a snapshot"
    );
    return this._value.length;
  },
  configurable: true,
});

XPathResult.prototype.iterateNext = function () {
  need(
    this._type === XPATH_RESULT_TYPES.UNORDERED_NODE_ITERATOR_TYPE ||
      this._type === XPATH_RESULT_TYPES.ORDERED_NODE_ITERATOR_TYPE,
    "an iterator"
  );
  if (this.invalidIteratorState) {
    throw getDOMException(
      "The document has mutated since the result was returned.",
      "InvalidStateError"
    );
  }
  return this._value[this._cursor++] ?? null;
};

XPathResult.prototype.snapshotItem = function (index) {
  need(
    this._type === XPATH_RESULT_TYPES.UNORDERED_NODE_SNAPSHOT_TYPE ||
      this._type === XPATH_RESULT_TYPES.ORDERED_NODE_SNAPSHOT_TYPE,
    "a snapshot"
  );
  return this._value[Number(index)] ?? null;
};

for (const [k, v] of Object.entries(XPATH_RESULT_TYPES)) {
  Object.defineProperty(XPathResult, k, { value: v, writable: false, enumerable: true, configurable: false });
  Object.defineProperty(XPathResult.prototype, k, { value: v, writable: false, enumerable: true, configurable: false });
}

function toXPathResultType(value) {
  if (typeof value === "bigint") {
    throw new TypeError("Cannot convert a BigInt value to a number");
  }
  const number = Number(value);
  if (!Number.isFinite(number) || number === 0) return 0;
  const modulo = Math.trunc(number) % 0x10000;
  return (modulo + 0x10000) % 0x10000;
}

function makeResult(value, type, reuse = null) {
  let t = toXPathResultType(type);
  if (t === XPATH_RESULT_TYPES.ANY_TYPE) {
    t = isNodeSet(value)
      ? XPATH_RESULT_TYPES.UNORDERED_NODE_ITERATOR_TYPE
      : typeof value === "number"
        ? XPATH_RESULT_TYPES.NUMBER_TYPE
        : typeof value === "string"
          ? XPATH_RESULT_TYPES.STRING_TYPE
          : XPATH_RESULT_TYPES.BOOLEAN_TYPE;
  }
  let resultValue;
  if (t === XPATH_RESULT_TYPES.NUMBER_TYPE) resultValue = toNumber(value);
  else if (t === XPATH_RESULT_TYPES.STRING_TYPE) resultValue = toStringValue(value);
  else if (t === XPATH_RESULT_TYPES.BOOLEAN_TYPE) resultValue = toBoolean(value);
  else {
    if (t < 0 || t > XPATH_RESULT_TYPES.FIRST_ORDERED_NODE_TYPE) {
      throw getDOMException("The result type is not supported.", "NotSupportedError");
    }
    if (!isNodeSet(value)) {
      throw new TypeError(
        "The result is not a node set, and therefore cannot be converted to the desired type."
      );
    }
    resultValue = value;
  }
  if (reuse) {
    reuse._refill(t, resultValue);
    return reuse;
  }
  return new XPathResult(INTERNAL_CONSTRUCT, t, resultValue);
}

function isXPathResult(value) {
  return typeof value === "object" && value !== null && xpathResultInstances.has(value);
}

function reusableResult(value) {
  if (value === null || value === undefined) return null;
  if (!isXPathResult(value)) throw new TypeError("The result is not an XPathResult.");
  return value;
}

function XPathExpression(token, expr) {
  if (token !== INTERNAL_CONSTRUCT || !expr) throw new TypeError("Illegal constructor");
  this._expr = expr;
}

XPathExpression.prototype.evaluate = function (contextNode, type = 0, result = null) {
  if (!isNode(contextNode)) {
    throw new TypeError(
      "Failed to execute 'evaluate' on 'XPathExpression': parameter 1 is not of type 'Node'."
    );
  }
  const reuse = reusableResult(result);
  const ev = new Evaluation(rootOf(contextNode));
  const value = evaluate(this._expr, { node: contextNode, pos: 1, size: 1, ev });
  return makeResult(isNodeSet(value) ? ev.sort(value) : value, type, reuse);
};

function createExpression(expression, resolver = null) {
  return new XPathExpression(INTERNAL_CONSTRUCT, compile(String(expression), resolver));
}

function evaluateXPath(
  expression,
  contextNode,
  resolver = null,
  type = 0,
  result = null
) {
  reusableResult(result);
  return createExpression(expression, resolver).evaluate(contextNode, type, result);
}

function XPathEvaluator() {}

XPathEvaluator.prototype.createExpression = function (expression, resolver = null) {
  return createExpression(expression, resolver);
};

XPathEvaluator.prototype.createNSResolver = function (nodeResolver) {
  return nodeResolver;
};

XPathEvaluator.prototype.evaluate = function (
  expression,
  contextNode,
  resolver = null,
  type = 0,
  result = null
) {
  return evaluateXPath(expression, contextNode, resolver, type, result);
};

if (typeof markNative === "function") {
  markNative(XPathResult, "XPathResult");
  markNative(XPathExpression, "XPathExpression");
  markNative(XPathEvaluator, "XPathEvaluator");
  markNative(XPathEvaluator.prototype.createExpression, "createExpression");
  markNative(XPathEvaluator.prototype.createNSResolver, "createNSResolver");
  markNative(XPathEvaluator.prototype.evaluate, "evaluate");
  markNative(XPathExpression.prototype.evaluate, "evaluate");
  markNative(XPathResult.prototype.iterateNext, "iterateNext");
  markNative(XPathResult.prototype.snapshotItem, "snapshotItem");
  markNative(createExpression, "createExpression");
  markNative(evaluateXPath, "evaluateXPath");
}
