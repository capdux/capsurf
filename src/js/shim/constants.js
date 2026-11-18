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

const DOM_EXCEPTION_CODES = {
  IndexSizeError: 1,
  DOMStringSizeError: 2,
  HierarchyRequestError: 3,
  WrongDocumentError: 4,
  InvalidCharacterError: 5,
  NoDataAllowedError: 6,
  NoModificationAllowedError: 7,
  NotFoundError: 8,
  NotSupportedError: 9,
  InUseAttributeError: 10,
  InvalidStateError: 11,
  SyntaxError: 12,
  InvalidModificationError: 13,
  NamespaceError: 14,
  InvalidAccessError: 15,
  ValidationError: 16,
  TypeMismatchError: 17,
  SecurityError: 18,
  NetworkError: 19,
  AbortError: 20,
  URLMismatchError: 21,
  QuotaExceededError: 22,
  TimeoutError: 23,
  InvalidNodeTypeError: 24,
  DataCloneError: 25,
};

const DOM_EXCEPTION_CONSTANTS = [
  ["INDEX_SIZE_ERR", 1],
  ["DOMSTRING_SIZE_ERR", 2],
  ["HIERARCHY_REQUEST_ERR", 3],
  ["WRONG_DOCUMENT_ERR", 4],
  ["INVALID_CHARACTER_ERR", 5],
  ["NO_DATA_ALLOWED_ERR", 6],
  ["NO_MODIFICATION_ALLOWED_ERR", 7],
  ["NOT_FOUND_ERR", 8],
  ["NOT_SUPPORTED_ERR", 9],
  ["INUSE_ATTRIBUTE_ERR", 10],
  ["INVALID_STATE_ERR", 11],
  ["SYNTAX_ERR", 12],
  ["INVALID_MODIFICATION_ERR", 13],
  ["NAMESPACE_ERR", 14],
  ["INVALID_ACCESS_ERR", 15],
  ["VALIDATION_ERR", 16],
  ["TYPE_MISMATCH_ERR", 17],
  ["SECURITY_ERR", 18],
  ["NETWORK_ERR", 19],
  ["ABORT_ERR", 20],
  ["URL_MISMATCH_ERR", 21],
  ["QUOTA_EXCEEDED_ERR", 22],
  ["TIMEOUT_ERR", 23],
  ["INVALID_NODE_TYPE_ERR", 24],
  ["DATA_CLONE_ERR", 25],
];

// 0. DOMException Implementation (Available from start)
function DOMException(message, name) {
  const err = new Error(message || "");
  err.name = name || "Error";
  err.code = DOM_EXCEPTION_CODES[err.name] || 0;
  Object.setPrototypeOf(err, DOMException.prototype);
  return err;
}
DOMException.prototype = Object.create(Error.prototype);
DOMException.prototype.constructor = DOMException;
DOMException.prototype.code = 0;
DOMException.prototype.name = "Error";
DOMException.prototype.message = "";

for (let i = 0; i < DOM_EXCEPTION_CONSTANTS.length; i++) {
  const item = DOM_EXCEPTION_CONSTANTS[i];
  Object.defineProperty(DOMException, item[0], { value: item[1], enumerable: true, configurable: false, writable: false });
  Object.defineProperty(DOMException.prototype, item[0], { value: item[1], enumerable: true, configurable: false, writable: false });
}

markNative(DOMException, "DOMException");
if (typeof globalThis !== "undefined") {
  globalThis.DOMException = DOMException;
}
