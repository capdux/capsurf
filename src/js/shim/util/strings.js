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
  if (err instanceof Error) return err.message;
  return String(err);
}
