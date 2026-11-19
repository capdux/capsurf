// src/js/shim/util/strings.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/util/strings.ts

function escapeHtml(str) {
  if (typeof str !== "string") str = String(str);
  return str
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;")
    .replace(/'/g, "&#39;");
}

function unescapeHtml(str) {
  if (typeof str !== "string") str = String(str);
  return str
    .replace(/&quot;/g, '"')
    .replace(/&#39;/g, "'")
    .replace(/&lt;/g, "<")
    .replace(/&gt;/g, ">")
    .replace(/&amp;/g, "&");
}

function normalizeAttributeName(name) {
  if (typeof name !== "string") name = String(name);
  return name.trim().toLowerCase();
}

function errorMessage(err) {
  if (err instanceof Error) return normalizeErrorMessage(err.message);
  return normalizeErrorMessage(String(err));
}

function normalizeErrorMessage(s) {
  if (typeof s !== "string") return s;
  return s
    .replace(/cannot read property '([^']+)' of (null|undefined)/g, "cannot read properties of $2 (reading '$1')")
    .replace(/cannot set property '([^']+)' of (null|undefined)/g, "cannot set properties of $2 (setting '$1')")
    .replace(/Cannot read property '([^']+)' of (null|undefined)/g, "Cannot read properties of $2 (reading '$1')")
    .replace(/Cannot set property '([^']+)' of (null|undefined)/g, "Cannot set properties of $2 (setting '$1')");
}

(function () {
  const errorCtors = [
    typeof Error !== "undefined" ? Error : null,
    typeof TypeError !== "undefined" ? TypeError : null,
    typeof RangeError !== "undefined" ? RangeError : null,
    typeof SyntaxError !== "undefined" ? SyntaxError : null,
    typeof ReferenceError !== "undefined" ? ReferenceError : null,
    typeof URIError !== "undefined" ? URIError : null,
    typeof EvalError !== "undefined" ? EvalError : null,
  ];

  for (let i = 0; i < errorCtors.length; i++) {
    const ctor = errorCtors[i];
    if (ctor && ctor.prototype) {
      const origToString = ctor.prototype.toString;
      ctor.prototype.toString = function () {
        if (this && typeof this.message === "string") {
          try {
            this.message = normalizeErrorMessage(this.message);
          } catch (e) {}
        }
        const s = (origToString && typeof origToString === "function") ? origToString.call(this) : (this.name + ": " + this.message);
        return normalizeErrorMessage(s);
      };
    }
  }
})();
