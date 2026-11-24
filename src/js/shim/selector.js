// src/js/shim/selector.js
// CSS Selector validation and QuerySelector wrappers

const FUNC_SELECTOR_PSEUDOS = new Set([":not", ":is", ":where", ":has", "::slotted"]);
const REQUIRED_FUNC_PSEUDOS = new Set([
  ":not", ":is", ":where", ":has", ":state", ":lang", ":dir",
  ":nth-child", ":nth-last-child", ":nth-of-type", ":nth-last-of-type",
  "::slotted", "::part"
]);
const NON_EMPTY_FUNC_PSEUDOS = new Set([":not", ":has", ":state", "::slotted", "::part", ":lang", ":dir"]);
const KNOWN_PSEUDO_CLASSES = new Set([
  ":hover", ":active", ":focus", ":focus-visible", ":focus-within", ":target",
  ":visited", ":link", ":any-link", ":local-link", ":enabled", ":disabled",
  ":read-only", ":read-write", ":placeholder-shown", ":default", ":checked",
  ":indeterminate", ":valid", ":invalid", ":in-range", ":out-of-range",
  ":required", ":optional", ":user-valid", ":user-invalid", ":root", ":empty",
  ":first-child", ":last-child", ":only-child", ":first-of-type", ":last-of-type",
  ":only-of-type", ":nth-child", ":nth-last-child", ":nth-of-type", ":nth-last-of-type",
  ":not", ":is", ":where", ":has", ":lang", ":dir", ":scope", ":defined", ":state",
  ":host", ":host-context", ":fullscreen", ":modal", ":picture-in-picture",
  ":popover-open", ":autofill", ":playing", ":paused", ":muted", ":seeking",
  ":stalled", ":buffering", ":volume-locked", ":open", ":closed", ":target-within",
  ":past", ":current", ":future", ":blank", ":heading", ":before", ":after",
  ":first-line", ":first-letter"
]);
const KNOWN_PSEUDO_ELEMENTS = new Set([
  "::before", "::after", "::first-line", "::first-letter", "::selection",
  "::marker", "::placeholder", "::backdrop", "::slotted", "::part",
  "::file-selector-button", "::target-text", "::spelling-error", "::grammar-error",
  "::highlight", "::search-text", "::view-transition", "::details-content",
  "::column", "::scroll-marker"
]);

function splitTopLevelSelectorList(str) {
  const out = [];
  let cur = "", depthP = 0, depthB = 0, quote = "";
  for (let i = 0; i < str.length; i++) {
    const ch = str[i];
    if (quote) {
      cur += ch;
      if (ch === "\\" && i + 1 < str.length) { cur += str[++i]; continue; }
      if (ch === quote) quote = "";
      continue;
    }
    if (ch === '"' || ch === "'") { quote = ch; cur += ch; continue; }
    if (ch === "(") depthP++;
    else if (ch === ")") { if (depthP === 0) return null; depthP--; }
    else if (ch === "[") depthB++;
    else if (ch === "]") { if (depthB === 0) return null; depthB--; }
    if (depthP === 0 && depthB === 0 && ch === ",") {
      out.push(cur);
      cur = "";
    } else {
      cur += ch;
    }
  }
  if (quote || depthP !== 0 || depthB !== 0) return null;
  out.push(cur);
  return out;
}

function canonicalizeComplexSelector(raw, inHas, inFunc) {
  const s = raw.trim();
  if (!s) return null;
  let i = 0;
  let out = "";
  let prevWasCombinator = false;
  let atStart = true;
  let seenTerminalPseudoEl = false;
  let lastPseudoEl = "";

  while (i < s.length) {
    // Whitespace -> descendant combinator (unless adjacent to > + ~ or start)
    if (/\s/.test(s[i])) {
      while (i < s.length && /\s/.test(s[i])) i++;
      if (i < s.length && s[i] !== ">" && s[i] !== "+" && s[i] !== "~" && !prevWasCombinator && !atStart) {
        if (seenTerminalPseudoEl && s[i] !== ":") return null;
        out += " ";
        prevWasCombinator = true;
      }
      continue;
    }
    // Combinator > + ~
    if (s[i] === ">" || s[i] === "+" || s[i] === "~") {
      if (seenTerminalPseudoEl && s[i] !== ":") return null;
      const comb = s[i++];
      while (i < s.length && /\s/.test(s[i])) i++;
      if (i >= s.length || s[i] === ">" || s[i] === "+" || s[i] === "~") return null;
      if (atStart) {
        if (!inHas) return null;
        out += comb + " ";
      } else {
        out = out.trimEnd() + " " + comb + " ";
      }
      prevWasCombinator = true;
      atStart = false;
      continue;
    }
    if (seenTerminalPseudoEl && s[i] !== ":") return null;
    const atSimpleStart = (atStart || prevWasCombinator);
    atStart = false;
    prevWasCombinator = false;
    if (atSimpleStart && /[0-9]/.test(s[i])) return null;

    // Attribute selector [...]
    if (s[i] === "[") {
      let j = i + 1, q = "";
      while (j < s.length) {
        if (q) {
          if (s[j] === "\\" && j + 1 < s.length) { j += 2; continue; }
          if (s[j] === q) q = "";
        } else {
          if (s[j] === '"' || s[j] === "'") q = s[j];
          else if (s[j] === "]") break;
        }
        j++;
      }
      if (j >= s.length || s[j] !== "]") return null;
      const inner = s.slice(i + 1, j).trim();
      if (!inner) return null;
      const am = /^([a-zA-Z_*|-][a-zA-Z0-9_*|-]*)\s*(?:([~|^$*]?=)\s*([\s\S]*?))?$/.exec(inner);
      if (!am) return null;
      let attrName = am[1];
      if (attrName.startsWith("|")) attrName = attrName.slice(1);
      if (!attrName) return null;
      if (am[2]) {
        let attrVal = am[3].trim();
        let mod = "";
        const modMatch = /^(.*)\s+([iIsS])$/.exec(attrVal);
        if (modMatch && (attrVal.startsWith('"') || attrVal.startsWith("'"))) {
          attrVal = modMatch[1].trim();
          mod = " " + modMatch[2].toLowerCase();
        }
        if ((attrVal.startsWith('"') && attrVal.endsWith('"')) || (attrVal.startsWith("'") && attrVal.endsWith("'"))) {
          const innerV = attrVal.slice(1, -1).replace(/\\'/g, "'").replace(/(?<!\\)"/g, '\\"');
          attrVal = '"' + innerV + '"';
        } else {
          if (!attrVal || /\s/.test(attrVal)) return null;
          attrVal = '"' + attrVal.replace(/"/g, '\\"') + '"';
        }
        out += "[" + attrName + am[2] + attrVal + mod + "]";
      } else {
        out += "[" + attrName + "]";
      }
      i = j + 1;
      continue;
    }

    // Pseudo-element ::name or pseudo-class :name
    if (s[i] === ":") {
      const isDouble = (i + 1 < s.length && s[i + 1] === ":");
      let j = i + (isDouble ? 2 : 1);
      if (j >= s.length || !/[a-zA-Z_-]/.test(s[j])) return null;
      while (j < s.length && /[a-zA-Z0-9_-]/.test(s[j])) j++;
      const pName = s.slice(i, j).toLowerCase();
      if (seenTerminalPseudoEl) {
        const allowChain = (isDouble && (
          (pName === "::column" && (lastPseudoEl === "::before" || lastPseudoEl === "::after")) ||
          (pName === "::scroll-marker" && lastPseudoEl === "::column")
        )) || (!isDouble && lastPseudoEl === "::file-selector-button" && (
          pName === ":hover" || pName === ":focus" || pName === ":active" || pName === ":focus-visible" || pName === ":focus-within" ||
          pName === ":is" || pName === ":where"
        ));
        if (!allowChain) return null;
      }
      if (isDouble) {
        if (inFunc || !KNOWN_PSEUDO_ELEMENTS.has(pName)) return null;
      } else {
        if (!KNOWN_PSEUDO_CLASSES.has(pName)) return null;
      }
      if (j < s.length && s[j] === "(") {
        let k = j + 1, dp = 1, q = "";
        while (k < s.length && dp > 0) {
          if (q) {
            if (s[k] === "\\" && k + 1 < s.length) { k += 2; continue; }
            if (s[k] === q) q = "";
          } else {
            if (s[k] === '"' || s[k] === "'") q = s[k];
            else if (s[k] === "(") dp++;
            else if (s[k] === ")") dp--;
          }
          k++;
        }
        if (dp !== 0) return null;
        const inner = s.slice(j + 1, k - 1).trim();
        if (NON_EMPTY_FUNC_PSEUDOS.has(pName) && !inner) return null;
        if (pName === ":has" && inHas) return null;
        if (pName === "::slotted") {
          const sub = canonicalizeComplexSelector(inner, false, true);
          if (sub === null || /[\s>+~]/.test(sub.replace(/\[[^\]]*\]/g, "").replace(/\([^)]*\)/g, ""))) return null;
          out += "::slotted(" + sub + ")";
          seenTerminalPseudoEl = true;
        } else if (pName === ":host" || pName === ":host-context") {
          const sub = canonicalizeComplexSelector(inner, false, true);
          if (sub === null || /[\s>+~]/.test(sub.replace(/\[[^\]]*\]/g, ""))) return null;
          out += pName + "(" + sub + ")";
        } else if (pName === ":is" || pName === ":where") {
          if (!inner) {
            out += pName + "()";
          } else {
            const items = splitTopLevelSelectorList(inner);
            if (!items) return null;
            const kept = [];
            for (let ii = 0; ii < items.length; ii++) {
              const c = canonicalizeComplexSelector(items[ii], inHas, true);
              if (c !== null) kept.push(c);
            }
            out += pName + "(" + kept.join(", ") + ")";
          }
        } else if (FUNC_SELECTOR_PSEUDOS.has(pName)) {
          if (!inner) {
            out += pName + "()";
          } else {
            const sub = canonicalizeSelectorList(inner, pName === ":has", true);
            if (sub === null) return null;
            out += pName + "(" + sub + ")";
          }
        } else if (pName === ":state") {
          if (!/^--?[a-zA-Z0-9_-]+$|^[a-zA-Z_-][a-zA-Z0-9_-]*$/.test(inner)) return null;
          out += ":state(" + inner + ")";
        } else {
          out += pName + "(" + inner + ")";
          if (isDouble && pName !== "::part") { seenTerminalPseudoEl = true; lastPseudoEl = pName; }
        }
        i = k;
        continue;
      } else {
        if (REQUIRED_FUNC_PSEUDOS.has(pName)) return null;
        out += pName;
        if (isDouble && pName !== "::part") { seenTerminalPseudoEl = true; lastPseudoEl = pName; }
        i = j;
        continue;
      }
    }

    // Universal *|* or *
    if (s.slice(i, i + 3) === "*|*") {
      const nextCh = s[i + 3] || "";
      if (nextCh === ":" || nextCh === "." || nextCh === "#" || nextCh === "[") {
        // *|*:has(*) -> :has(*)
      } else {
        out += "*";
      }
      i += 3;
      continue;
    }
    if (s[i] === "*") {
      if (s[i + 1] === ":" && s[i + 2] === ":") {
        i++;
        continue;
      }
      out += s[i++];
      continue;
    }

    // Invalid stray characters
    if (/[!@$%^&;{}()\]/]/.test(s[i])) return null;
    out += s[i++];
  }

  return prevWasCombinator ? null : out.trim();
}

function canonicalizeSelectorList(selector, inHas, inFunc) {
  if (typeof selector !== "string") selector = String(selector);
  const parts = splitTopLevelSelectorList(selector.trim());
  if (!parts || parts.length === 0) return null;
  const out = [];
  for (let i = 0; i < parts.length; i++) {
    const c = canonicalizeComplexSelector(parts[i], !!inHas, !!inFunc);
    if (c === null) return null;
    out.push(c);
  }
  return out.join(", ");
}

if (typeof globalThis !== "undefined") {
  globalThis.__capsurf_canonicalizeSelector = canonicalizeSelectorList;
}

function validateSelector(selector) {
  const res = canonicalizeSelectorList(selector, false);
  if (res === null) {
    throw new DOMException("The provided selector is invalid.", "SyntaxError");
  }
  return res;
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
      const doc = el.ownerDocument || (typeof document !== "undefined" ? document : null);
      const quirks = doc && doc.compatMode === "BackCompat";
      const actualId = el.id || (typeof el.getAttribute === "function" ? el.getAttribute("id") : "") || "";
      if (quirks ? (actualId.toLowerCase() !== id.toLowerCase()) : (actualId !== id)) {
        return false;
      }
    } else if (ch === ".") {
      i++;
      const start = i;
      while (i < len && !/[.#:\[\s]/.test(s[i])) i++;
      const cls = s.slice(start, i);
      const doc = el.ownerDocument || (typeof document !== "undefined" ? document : null);
      const quirks = doc && doc.compatMode === "BackCompat";
      const clsStr = ((el.className || (typeof el.getAttribute === "function" && el.getAttribute("class")) || "") + "");
      let hasClass = !quirks && Boolean(el.classList && el.classList.contains(cls));
      if (!hasClass && clsStr) {
        const parts = clsStr.split(/\s+/);
        const target = quirks ? cls.toLowerCase() : cls;
        for (let k = 0; k < parts.length; k++) {
          if ((quirks ? parts[k].toLowerCase() : parts[k]) === target) {
            hasClass = true;
            break;
          }
        }
      }
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
      while (i < len && !/[.#:\[\s(]/.test(s[i])) i++;
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
      if (pseudo === ":checked") { if (!el.checked && !(String(el.tagName || "").toUpperCase() === "OPTION" && el.selected)) return false; }
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
      else if (pseudo === ":only-child") { if (el.previousElementSibling || el.nextElementSibling) return false; }
      else if (pseudo === ":first-of-type") { if (!matchesNthChild(el, "1", false, true)) return false; }
      else if (pseudo === ":last-of-type") { if (!matchesNthChild(el, "1", true, true)) return false; }
      else if (pseudo === ":only-of-type") { if (!matchesNthChild(el, "1", false, true) || !matchesNthChild(el, "1", true, true)) return false; }
      else if (pseudo === ":empty") { if (el.firstChild) return false; }
      else if (pseudo.startsWith(":nth-child(") && pseudo.endsWith(")")) {
        if (!matchesNthChild(el, pseudo.slice(11, -1), false, false)) return false;
      }
      else if (pseudo.startsWith(":nth-last-child(") && pseudo.endsWith(")")) {
        if (!matchesNthChild(el, pseudo.slice(16, -1), true, false)) return false;
      }
      else if (pseudo.startsWith(":nth-of-type(") && pseudo.endsWith(")")) {
        if (!matchesNthChild(el, pseudo.slice(13, -1), false, true)) return false;
      }
      else if (pseudo.startsWith(":nth-last-of-type(") && pseudo.endsWith(")")) {
        if (!matchesNthChild(el, pseudo.slice(18, -1), true, true)) return false;
      }
      else if (pseudo.startsWith(":is(") && pseudo.endsWith(")")) {
        if (!matchesElementSelector(el, s.slice(start + 4, i - 1), scopeRoot, true)) return false;
      }
      else if (pseudo.startsWith(":where(") && pseudo.endsWith(")")) {
        if (!matchesElementSelector(el, s.slice(start + 7, i - 1), scopeRoot, true)) return false;
      }
      else if (pseudo.startsWith(":matches(") && pseudo.endsWith(")")) {
        if (!matchesElementSelector(el, s.slice(start + 9, i - 1), scopeRoot, true)) return false;
      }
      else if (pseudo.startsWith(":not(") && pseudo.endsWith(")")) {
        if (matchesElementSelector(el, s.slice(start + 5, i - 1), scopeRoot, true)) return false;
      }
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

function matchesNthChild(el, exprRaw, fromLast, ofType) {
  const p = el.parentNode;
  if (!p) return false;
  const kids = p.children || p._children || p.childNodes || [];
  const elTag = ofType ? String(el.localName || el.tagName || "").toLowerCase() : "";
  let idx = 0;
  let total = 0;
  for (let k = 0; k < kids.length; k++) {
    const ch = kids[k];
    if (ch && ch.nodeType === 1) {
      if (ofType && String(ch.localName || ch.tagName || "").toLowerCase() !== elTag) continue;
      total++;
      if (ch === el || (typeof el._nid === "number" && el._nid > 0 && ch._nid === el._nid)) {
        idx = total;
      }
    }
  }
  if (idx === 0) return false;
  const targetIdx = fromLast ? (total - idx + 1) : idx;
  const expr = String(exprRaw || "").trim().toLowerCase().replace(/\s+/g, "");
  if (expr === "odd") return targetIdx % 2 === 1;
  if (expr === "even") return targetIdx % 2 === 0;
  if (/^[+-]?\d+$/.test(expr)) return targetIdx === parseInt(expr, 10);
  const m = expr.match(/^([+-]?\d*)n([+-]\d+)?$/);
  if (!m) return false;
  const aStr = m[1];
  const a = (aStr === "" || aStr === "+") ? 1 : (aStr === "-" ? -1 : parseInt(aStr, 10));
  const b = m[2] ? parseInt(m[2], 10) : 0;
  if (a === 0) return targetIdx === b;
  const diff = targetIdx - b;
  if (a > 0) return diff >= 0 && diff % a === 0;
  return diff <= 0 && diff % a === 0;
}

function splitSelectorList(sel) {
  const list = [];
  let buf = "";
  let inBracket = 0;
  let inParen = 0;
  let quote = "";
  for (let i = 0; i < sel.length; i++) {
    const ch = sel[i];
    if (quote) {
      buf += ch;
      if (ch === quote) quote = "";
      continue;
    }
    if (ch === '"' || ch === "'") { quote = ch; buf += ch; continue; }
    if (ch === "[") { inBracket++; buf += ch; continue; }
    if (ch === "]") { if (inBracket > 0) inBracket--; buf += ch; continue; }
    if (ch === "(") { inParen++; buf += ch; continue; }
    if (ch === ")") { if (inParen > 0) inParen--; buf += ch; continue; }
    if (ch === "," && inBracket === 0 && inParen === 0) {
      if (buf.trim()) list.push(buf.trim());
      buf = "";
      continue;
    }
    buf += ch;
  }
  if (buf.trim()) list.push(buf.trim());
  return list;
}

function matchesElementSelector(el, selector, scopeRoot, noNativeFallback) {
  if (!el || el.nodeType !== 1 || !selector) return false;
  const s = selector.trim();
  if (s === "*") return true;

  // Handle comma-separated selector lists (respecting parens/brackets)
  const list = splitSelectorList(s);
  for (let i = 0; i < list.length; i++) {
    const item = list[i];
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
    if (!targetId) return null;
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
