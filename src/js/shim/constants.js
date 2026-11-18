// src/js/shim/constants.js
// Shared DOM node, namespace, viewport, and traversal constants.
// Aligned with vendor/kitesurf/packages/dom-shim/src/constants.ts

const ELEMENT_NODE = 1;
const ATTRIBUTE_NODE = 2;
const TEXT_NODE = 3;
const CDATA_SECTION_NODE = 4;
const ENTITY_REFERENCE_NODE = 5;
const ENTITY_NODE = 6;
const PROCESSING_INSTRUCTION_NODE = 7;
const COMMENT_NODE = 8;
const DOCUMENT_NODE = 9;
const DOCUMENT_TYPE_NODE = 10;
const DOCUMENT_FRAGMENT_NODE = 11;
const NOTATION_NODE = 12;

const DOCUMENT_POSITION_DISCONNECTED = 0x01;
const DOCUMENT_POSITION_PRECEDING = 0x02;
const DOCUMENT_POSITION_FOLLOWING = 0x04;
const DOCUMENT_POSITION_CONTAINS = 0x08;
const DOCUMENT_POSITION_CONTAINED_BY = 0x10;
const DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC = 0x20;

const HTML_NS = "http://www.w3.org/1999/xhtml";
const SVG_NS = "http://www.w3.org/2000/svg";
const MATHML_NS = "http://www.w3.org/1998/Math/MathML";
const XML_NS = "http://www.w3.org/XML/1998/namespace";
const XMLNS_NS = "http://www.w3.org/2000/xmlns/";

const SHOW_ALL = 0xFFFFFFFF;
const SHOW_ELEMENT = 0x1;
const SHOW_ATTRIBUTE = 0x2;
const SHOW_TEXT = 0x4;
const SHOW_CDATA_SECTION = 0x8;
const SHOW_ENTITY_REFERENCE = 0x10;
const SHOW_ENTITY = 0x20;
const SHOW_PROCESSING_INSTRUCTION = 0x40;
const SHOW_COMMENT = 0x80;
const SHOW_DOCUMENT = 0x100;
const SHOW_DOCUMENT_TYPE = 0x200;
const SHOW_DOCUMENT_FRAGMENT = 0x400;
const SHOW_NOTATION = 0x800;

const FILTER_ACCEPT = 1;
const FILTER_REJECT = 2;
const FILTER_SKIP = 3;

// Helper: Masquerade functions to return `function () { [native code] }`
const nativeToString = "function () { [native code] }";
function markNative(fn, name) {
  try {
    if (name) {
      Object.defineProperty(fn, "name", { value: name, configurable: true });
    }
    Object.defineProperty(fn, "toString", {
      value: function () { return nativeToString; },
      configurable: true,
      writable: true,
    });
  } catch (e) {}
  return fn;
}

// Global render configuration (toggleable at runtime)
if (typeof globalThis.__CAP_RENDER_CONFIG === "undefined") {
  globalThis.__CAP_RENDER_CONFIG = {
    canvasRecording: false, // Default false: high-speed headless Agent mode; true: cloud visual vector recording
    animations: false,      // Default false: static headless mode; true: full WAAPI animation tick & timeline
  };
}

// 0. DOMException Implementation (Available from start)
function DOMException(message, name) {
  const err = new Error(message || "");
  err.name = name || "Error";
  Object.setPrototypeOf(err, DOMException.prototype);
  return err;
}
DOMException.prototype = Object.create(Error.prototype);
DOMException.prototype.constructor = DOMException;
markNative(DOMException, "DOMException");
if (typeof globalThis !== "undefined") {
  globalThis.DOMException = DOMException;
}
