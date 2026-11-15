// src/js/shim/util/validation.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/util/validation.ts

const NAME_START_CHAR = /^[a-zA-Z_:\u00C0-\u00D6\u00D8-\u00F6\u00F8-\u02FF\u0370-\u037D\u037F-\u1FFF\u200C-\u200D\u2070-\u218F\u2C00-\u2FEF\u3001-\uD7FF\uF900-\uFDCF\uFDF0-\uFFFD]/;
const NAME_CHAR = /^[a-zA-Z0-9._:\u00B7\u00C0-\u00D6\u00D8-\u00F6\u00F8-\u02FF\u0370-\u037D\u037F-\u1FFF\u200C-\u200D\u2070-\u218F\u2C00-\u2FEF\u3001-\uD7FF\uF900-\uFDCF\uFDF0-\uFFFD-]/;

function isXmlName(name) {
  if (typeof name !== "string" || name.length === 0) return false;
  if (!NAME_START_CHAR.test(name[0])) return false;
  for (let i = 1; i < name.length; i++) {
    if (!NAME_CHAR.test(name[i])) return false;
  }
  return true;
}

function validateName(name) {
  if (!isXmlName(name)) {
    throw new Error("InvalidCharacterError: '" + name + "' is not a valid XML name.");
  }
}

function validateAttributeName(name) {
  if (typeof name !== "string" || name.length === 0) {
    throw new Error("InvalidCharacterError: Empty attribute name.");
  }
  if (!isXmlName(name)) {
    throw new Error("InvalidCharacterError: '" + name + "' is not a valid attribute name.");
  }
}
