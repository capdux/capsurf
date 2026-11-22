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
    if ((s.includes(":checked") || s.includes(":scope")) && typeof findInSubtree === "function" && typeof matchesElementSelector === "function") {
      const root = this === document ? (document.documentElement || document) : this;
      const found = findInSubtree(root, function (el) {
        const up = typeof upgradeElement === "function" ? upgradeElement(el) : el;
        return matchesElementSelector(up, s, root, true);
      }, true);
      return found ? (typeof upgradeElement === "function" ? upgradeElement(found) : found) : null;
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
    if ((s.includes(":checked") || s.includes(":scope")) && typeof findInSubtree === "function" && typeof matchesElementSelector === "function") {
      const root = this === document ? (document.documentElement || document) : this;
      const matched = findInSubtree(root, function (el) {
        const up = typeof upgradeElement === "function" ? upgradeElement(el) : el;
        return matchesElementSelector(up, s, root, true);
      }, false);
      const out = [];
      for (let i = 0; i < matched.length; i++) {
        out.push(typeof upgradeElement === "function" ? upgradeElement(matched[i]) : matched[i]);
      }
      return new NodeList(out);
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

function matchesCompoundSelector(el, compound, scopeRoot) {
  if (!el || el.nodeType !== 1 || !compound) return false;
  const s = compound.trim();
  if (!s) return false;
  if (s === "*") return true;

  // Tokenize compound selector into tag, #id, .class, [attr...], :pseudo
  let i = 0;
  const len = s.length;
  while (i < len) {
    const ch = s[i];
    if (ch === "#") {
      i++;
      const start = i;
      while (i < len && !/[.#:\[\s]/.test(s[i])) i++;
      const id = s.slice(start, i);
      if (el.id !== id && !(typeof el.getAttribute === "function" && el.getAttribute("id") === id)) {
        return false;
      }
    } else if (ch === ".") {
      i++;
      const start = i;
      while (i < len && !/[.#:\[\s]/.test(s[i])) i++;
      const cls = s.slice(start, i);
      const hasClass = (el.classList && el.classList.contains(cls)) ||
        (((el.className || (typeof el.getAttribute === "function" && el.getAttribute("class")) || "") + "").split(/\s+/).indexOf(cls) !== -1);
      if (!hasClass) return false;
    } else if (ch === "[") {
      const closeIdx = s.indexOf("]", i + 1);
      if (closeIdx === -1) return false;
      const inside = s.slice(i + 1, closeIdx).trim();
      i = closeIdx + 1;
      const eqIdx = inside.indexOf("=");
      if (eqIdx === -1) {
        if (!(typeof el.hasAttribute === "function" && el.hasAttribute(inside))) return false;
      } else {
        let op = "=";
        let attrName = inside.slice(0, eqIdx).trim();
        const lastChar = attrName[attrName.length - 1];
        if (lastChar === "~" || lastChar === "|" || lastChar === "^" || lastChar === "$" || lastChar === "*") {
          op = lastChar + "=";
          attrName = attrName.slice(0, -1).trim();
        }
        let expectedVal = inside.slice(eqIdx + 1).trim();
        let caseInsensitive = false;
        if (/\s+[iI]$/.test(expectedVal)) {
          caseInsensitive = true;
          expectedVal = expectedVal.slice(0, -1).trim();
        } else if (/\s+[sS]$/.test(expectedVal)) {
          expectedVal = expectedVal.slice(0, -1).trim();
        }
        if ((expectedVal.startsWith('"') && expectedVal.endsWith('"')) ||
            (expectedVal.startsWith("'") && expectedVal.endsWith("'"))) {
          expectedVal = expectedVal.slice(1, -1);
        }
        let actualVal = typeof el.getAttribute === "function" ? el.getAttribute(attrName) : null;
        if (actualVal === null) return false;
        if (caseInsensitive) {
          actualVal = actualVal.toLowerCase();
          expectedVal = expectedVal.toLowerCase();
        }
        if (op === "=" && actualVal !== expectedVal) return false;
        if (op === "~=" && actualVal.split(/\s+/).indexOf(expectedVal) === -1) return false;
        if (op === "|=" && actualVal !== expectedVal && !actualVal.startsWith(expectedVal + "-")) return false;
        if (op === "^=" && (!expectedVal || !actualVal.startsWith(expectedVal))) return false;
        if (op === "$=" && (!expectedVal || !actualVal.endsWith(expectedVal))) return false;
        if (op === "*=" && (!expectedVal || actualVal.indexOf(expectedVal) === -1)) return false;
      }
    } else if (ch === ":") {
      // Pseudo-class
      const start = i;
      i++;
      while (i < len && !/[.#:\[\s]/.test(s[i])) i++;
      if (i < len && s[i] === "(") {
        let depth = 1;
        i++;
        while (i < len && depth > 0) {
          if (s[i] === "(") depth++;
          else if (s[i] === ")") depth--;
          i++;
        }
      }
      const pseudo = s.slice(start, i).toLowerCase();
      if (pseudo === ":checked") { if (!el.checked) return false; }
      else if (pseudo === ":disabled") { if (!el.disabled) return false; }
      else if (pseudo === ":enabled") { if (el.disabled) return false; }
      else if (pseudo === ":scope") {
        const sr = scopeRoot || (typeof document !== "undefined" ? document.documentElement : null);
        if (!sr) return false;
        const same = el === sr || (typeof el._nid === "number" && typeof sr._nid === "number" && el._nid > 0 && el._nid === sr._nid);
        if (!same) return false;
      }
      else if (pseudo === ":first-child") { if (el.previousElementSibling) return false; }
      else if (pseudo === ":last-child") { if (el.nextElementSibling) return false; }
      else if (pseudo === ":empty") { if (el.firstChild) return false; }
      else return false;
    } else if (ch === "*") {
      i++;
    } else {
      const start = i;
      while (i < len && !/[.#:\[\s]/.test(s[i])) i++;
      const tag = s.slice(start, i).toLowerCase();
      const elTag = String(el.localName || el.tagName || "").toLowerCase();
      if (elTag !== tag) return false;
    }
  }
  return true;
}

function splitComplexSelector(sel) {
  // Split into alternating [compound, combinator, compound, ...]
  const parts = [];
  let i = 0;
  const len = sel.length;
  let buf = "";
  let inBracket = 0;
  let inParen = 0;
  let quote = "";
  while (i < len) {
    const ch = sel[i];
    if (quote) {
      buf += ch;
      if (ch === quote) quote = "";
      i++;
      continue;
    }
    if (ch === '"' || ch === "'") {
      quote = ch;
      buf += ch;
      i++;
      continue;
    }
    if (ch === "[") { inBracket++; buf += ch; i++; continue; }
    if (ch === "]") { if (inBracket > 0) inBracket--; buf += ch; i++; continue; }
    if (ch === "(") { inParen++; buf += ch; i++; continue; }
    if (ch === ")") { if (inParen > 0) inParen--; buf += ch; i++; continue; }
    if (inBracket === 0 && inParen === 0 && (ch === ">" || ch === "+" || ch === "~" || /\s/.test(ch))) {
      if (buf.trim()) parts.push(buf.trim());
      buf = "";
      let comb = " ";
      while (i < len && /\s/.test(sel[i])) i++;
      if (i < len && (sel[i] === ">" || sel[i] === "+" || sel[i] === "~")) {
        comb = sel[i];
        i++;
        while (i < len && /\s/.test(sel[i])) i++;
      }
      if (parts.length > 0 && i < len) {
        parts.push(comb);
      }
      continue;
    }
    buf += ch;
    i++;
  }
  if (buf.trim()) parts.push(buf.trim());
  return parts;
}

function matchesComplexSelector(el, sel, scopeRoot) {
  const parts = splitComplexSelector(sel);
  if (parts.length === 0) return false;
  if (!matchesCompoundSelector(el, parts[parts.length - 1], scopeRoot)) return false;
  if (parts.length === 1) return true;

  function matchFrom(node, idx) {
    if (idx < 0) return true;
    const comb = parts[idx];
    const targetCompound = parts[idx - 1];
    if (comb === " ") {
      let anc = node.parentElement || (node.parentNode && node.parentNode.nodeType === 1 ? node.parentNode : null);
      while (anc) {
        if (matchesCompoundSelector(anc, targetCompound, scopeRoot) && matchFrom(anc, idx - 2)) {
          return true;
        }
        if (scopeRoot && (anc === scopeRoot || (typeof anc._nid === "number" && anc._nid > 0 && anc._nid === scopeRoot._nid))) break;
        anc = anc.parentElement || (anc.parentNode && anc.parentNode.nodeType === 1 ? anc.parentNode : null);
      }
      return false;
    }
    if (comb === ">") {
      const p = node.parentElement || (node.parentNode && node.parentNode.nodeType === 1 ? node.parentNode : null);
      if (!p) return false;
      return matchesCompoundSelector(p, targetCompound, scopeRoot) && matchFrom(p, idx - 2);
    }
    if (comb === "+") {
      const prev = node.previousElementSibling;
      if (!prev) return false;
      return matchesCompoundSelector(prev, targetCompound, scopeRoot) && matchFrom(prev, idx - 2);
    }
    if (comb === "~") {
      let prev = node.previousElementSibling;
      while (prev) {
        if (matchesCompoundSelector(prev, targetCompound, scopeRoot) && matchFrom(prev, idx - 2)) {
          return true;
        }
        prev = prev.previousElementSibling;
      }
      return false;
    }
    return false;
  }

  return matchFrom(el, parts.length - 2);
}

function matchesElementSelector(el, selector, scopeRoot, noNativeFallback) {
  if (!el || el.nodeType !== 1 || !selector) return false;
  const s = selector.trim();
  if (s === "*") return true;

  // Handle comma-separated selector lists
  const list = s.split(",");
  for (let i = 0; i < list.length; i++) {
    const item = list[i].trim();
    if (item && matchesComplexSelector(el, item, scopeRoot)) return true;
  }

  if (noNativeFallback) return false;

  // Fallback to el.matches only if element is connected to document (not inside ShadowRoot)
  const root = typeof el.getRootNode === "function" ? el.getRootNode() : null;
  if ((!root || !root.host) && el.isConnected && typeof el.matches === "function") {
    try {
      return el.matches(s);
    } catch (e) {}
  }

  return false;
}

function findInSubtree(root, predicate, single) {
  const result = single ? null : [];
  function walk(node) {
    if (!node) return false;
    let kids = null;
    if (node._children && node._children.length > 0) {
      kids = node._children;
    } else if (node.childNodes && node.childNodes.length > 0) {
      kids = node.childNodes;
    } else if (node.children && node.children.length > 0) {
      kids = node.children;
    } else if (node.firstChild) {
      kids = [];
      for (let c = node.firstChild; c; c = c.nextSibling) kids.push(c);
    }
    if (!kids) return false;
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
