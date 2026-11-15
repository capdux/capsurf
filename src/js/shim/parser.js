// src/js/shim/parser.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/parser.ts

function DOMParser() {}

DOMParser.prototype.parseFromString = function (str, mimeType) {
  const type = String(mimeType || "text/html").toLowerCase();
  const html = String(str || "");

  // Create a synthetic document container
  const doc = {
    nodeType: DOCUMENT_NODE,
    nodeName: "#document",
    contentType: type,
    compatMode: "CSS1Compat",
    characterSet: "UTF-8",
    _html: html,
  };

  if (typeof document !== "undefined" && document.createElement) {
    const root = document.createElement("html");
    const head = document.createElement("head");
    const body = document.createElement("body");
    root.appendChild(head);
    root.appendChild(body);

    doc.documentElement = root;
    doc.head = head;
    doc.body = body;

    // Extract title if present
    const titleMatch = html.match(/<title[^>]*>([\s\S]*?)<\/title>/i);
    if (titleMatch) {
      doc.title = titleMatch[1];
      const titleEl = document.createElement("title");
      titleEl.textContent = titleMatch[1];
      head.appendChild(titleEl);
    } else {
      doc.title = "";
    }

    doc.querySelector = function (sel) { return root.querySelector ? root.querySelector(sel) : null; };
    doc.querySelectorAll = function (sel) { return root.querySelectorAll ? root.querySelectorAll(sel) : new NodeList([]); };
    doc.getElementById = function (id) {
      if (root.querySelectorAll) {
        const matches = root.querySelectorAll("#" + id);
        return matches.length > 0 ? matches[0] : null;
      }
      return null;
    };
  }

  return doc;
};

markNative(DOMParser, "DOMParser");
markNative(DOMParser.prototype.parseFromString, "parseFromString");
