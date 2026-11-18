// src/js/shim/selector.js
// CSS Selector validation and QuerySelector wrappers

function validateSelector(selector) {
  if (typeof selector !== "string") {
    selector = String(selector);
  }
  const s = selector.trim();
  if (s.length === 0 || s === "!!" || /^[!@#$%^&;]+$/.test(s) || /^[+>~]$/.test(s)) {
    throw new DOMException("The provided selector is invalid.", "SyntaxError");
  }
  if (s.includes("::")) {
    const pseudoPattern = /::([a-zA-Z-]+)(.*)$/;
    const m = s.match(pseudoPattern);
    if (m) {
      const rest = m[2].trim();
      if (rest.length > 0) {
        throw new DOMException("The provided selector is invalid.", "SyntaxError");
      }
    }
  }
}

function isPurePseudoElementSelector(s) {
  return /::(before|after|selection|marker|placeholder|first-line|first-letter)$/i.test(s) ||
         /:(before|after|first-line|first-letter)$/i.test(s);
}

function wrapQuerySelector(origFn) {
  return function (sel) {
    if (arguments.length === 0) throw new TypeError("querySelector: 1 argument required");
    const s = String(sel).trim();
    validateSelector(s);
    if (isPurePseudoElementSelector(s)) {
      return null;
    }
    if (origFn) {
      const res = origFn.call(this, s);
      return typeof upgradeElement === "function" ? upgradeElement(res) : res;
    }
    return null;
  };
}

function wrapQuerySelectorAll(origFn) {
  return function (sel) {
    if (arguments.length === 0) throw new TypeError("querySelectorAll: 1 argument required");
    const s = String(sel).trim();
    validateSelector(s);
    if (isPurePseudoElementSelector(s)) {
      return new NodeList([]);
    }
    let raw = null;
    if (origFn) {
      raw = origFn.call(this, s);
    }
    const list = [];
    if (raw && raw.length) {
      for (let i = 0; i < raw.length; i++) {
        const el = typeof upgradeElement === "function" ? upgradeElement(raw[i]) : raw[i];
        list.push(el);
      }
    }
    return new NodeList(list);
  };
}

function matchesElementSelector(el, selector) {
  if (!el || el.nodeType !== 1 || !selector) return false;
  const s = selector.trim();
  if (s === "*") return true;

  // If el.matches is available, try it first
  if (typeof el.matches === "function") {
    try {
      return el.matches(s);
    } catch (e) {}
  }

  // Tag only: e.g. "b", "p", "div"
  if (/^[a-zA-Z0-9_-]+$/.test(s)) {
    const elTag = String(el.localName || el.tagName || "").toLowerCase();
    return elTag === s.toLowerCase();
  }

  // ID only: e.g. "#foo"
  if (s.startsWith("#") && !/[\s.>:\[]/.test(s)) {
    const id = s.slice(1);
    return el.id === id || (typeof el.getAttribute === "function" && el.getAttribute("id") === id);
  }

  // Class only: e.g. ".foo"
  if (s.startsWith(".") && !/[\s#>:\[]/.test(s)) {
    const cls = s.slice(1);
    if (el.classList && el.classList.contains(cls)) return true;
    const cName = el.className || (typeof el.getAttribute === "function" && el.getAttribute("class")) || "";
    return cName.split(/\s+/).indexOf(cls) !== -1;
  }

  // Attribute selector: e.g. [attr] or [attr="val"] or [attr=val]
  if (s.startsWith("[") && s.endsWith("]")) {
    const inside = s.slice(1, -1).trim();
    const eqIdx = inside.indexOf("=");
    if (eqIdx === -1) {
      return typeof el.hasAttribute === "function" ? el.hasAttribute(inside) : false;
    }
    const attrName = inside.slice(0, eqIdx).trim();
    let expectedVal = inside.slice(eqIdx + 1).trim();
    if ((expectedVal.startsWith('"') && expectedVal.endsWith('"')) ||
        (expectedVal.startsWith("'") && expectedVal.endsWith("'"))) {
      expectedVal = expectedVal.slice(1, -1);
    }
    const actualVal = typeof el.getAttribute === "function" ? el.getAttribute(attrName) : null;
    return actualVal === expectedVal;
  }

  // Tag + attribute: e.g. "template[shadowrootmode]"
  const bracketIdx = s.indexOf("[");
  if (bracketIdx !== -1 && s.endsWith("]")) {
    const tagPart = s.slice(0, bracketIdx).trim();
    const attrPart = s.slice(bracketIdx);
    return matchesElementSelector(el, tagPart) && matchesElementSelector(el, attrPart);
  }

  // Tag + class: e.g. "p.x"
  const dotIdx = s.indexOf(".");
  if (dotIdx > 0 && !/[\s#>:\[]/.test(s)) {
    const tagPart = s.slice(0, dotIdx);
    const clsPart = s.slice(dotIdx);
    return matchesElementSelector(el, tagPart) && matchesElementSelector(el, clsPart);
  }

  return false;
}

function findInSubtree(root, predicate, single) {
  const result = single ? null : [];
  function walk(node) {
    if (!node) return false;
    const kids = node._children || (node.childNodes ? Array.from(node.childNodes) : []);
    for (let i = 0; i < kids.length; i++) {
      const child = kids[i];
      if (!child) continue;
      if (child.nodeType === 1) { // ELEMENT_NODE
        if (predicate(child)) {
          if (single) return child;
          result.push(child);
        }
      }
      const found = walk(child);
      if (single && found) return found;
    }
    return false;
  }
  const r = walk(root);
  return single ? r : result;
}

if (typeof DocumentFragment !== "undefined" && DocumentFragment.prototype) {
  DocumentFragment.prototype.getElementById = function (id) {
    const targetId = String(id);
    const found = findInSubtree(this, function (el) {
      return el.id === targetId || (typeof el.getAttribute === "function" && el.getAttribute("id") === targetId);
    }, true);
    return found ? (typeof upgradeElement === "function" ? upgradeElement(found) : found) : null;
  };
  markNative(DocumentFragment.prototype.getElementById, "getElementById");

  DocumentFragment.prototype.querySelector = wrapQuerySelector(function (sel) {
    const found = findInSubtree(this, function (el) {
      return matchesElementSelector(el, sel);
    }, true);
    return found ? (typeof upgradeElement === "function" ? upgradeElement(found) : found) : null;
  });
  markNative(DocumentFragment.prototype.querySelector, "querySelector");

  DocumentFragment.prototype.querySelectorAll = wrapQuerySelectorAll(function (sel) {
    return findInSubtree(this, function (el) {
      return matchesElementSelector(el, sel);
    }, false) || [];
  });
  markNative(DocumentFragment.prototype.querySelectorAll, "querySelectorAll");
}
