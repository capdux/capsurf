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
