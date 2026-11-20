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

// 0. DOMException Implementation (WebIDL binding shape)
const domExceptionState = new WeakMap();

function DOMException(message, name) {
  if (!(this instanceof DOMException)) {
    throw new TypeError("Failed to construct 'DOMException': Please use the 'new' operator.");
  }
  const msg = message === undefined ? "" : String(message);
  const nm = name === undefined ? "Error" : String(name);
  const code = Object.prototype.hasOwnProperty.call(DOM_EXCEPTION_CODES, nm) ? DOM_EXCEPTION_CODES[nm] : 0;
  const err = new Error();
  try { delete err.message; } catch (e) {}
  try { delete err.name; } catch (e) {}
  Object.setPrototypeOf(err, new.target ? new.target.prototype : DOMException.prototype);
  domExceptionState.set(err, { message: msg, name: nm, code: code });
  return err;
}
DOMException.prototype = Object.create(Error.prototype);
Object.defineProperty(DOMException.prototype, "constructor", {
  value: DOMException,
  writable: true,
  enumerable: false,
  configurable: true,
});
Object.defineProperty(DOMException.prototype, "name", {
  get: markNative(function () {
    const st = domExceptionState.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    return st.name;
  }, "get name"),
  enumerable: true,
  configurable: true,
});
Object.defineProperty(DOMException.prototype, "message", {
  get: markNative(function () {
    const st = domExceptionState.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    return st.message;
  }, "get message"),
  enumerable: true,
  configurable: true,
});
Object.defineProperty(DOMException.prototype, "code", {
  get: markNative(function () {
    const st = domExceptionState.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    return st.code;
  }, "get code"),
  enumerable: true,
  configurable: true,
});
Object.defineProperty(DOMException.prototype, Symbol.toStringTag, {
  value: "DOMException",
  writable: false,
  enumerable: false,
  configurable: true,
});

for (let i = 0; i < DOM_EXCEPTION_CONSTANTS.length; i++) {
  const item = DOM_EXCEPTION_CONSTANTS[i];
  Object.defineProperty(DOMException, item[0], { value: item[1], enumerable: true, configurable: false, writable: false });
  Object.defineProperty(DOMException.prototype, item[0], { value: item[1], enumerable: true, configurable: false, writable: false });
}

markNative(DOMException, "DOMException");
if (typeof globalThis !== "undefined") {
  globalThis.DOMException = DOMException;
}

// Global kitesurf & capsurf internals stubs
(function () {
  const ksSym = Symbol.for("kitesurf.internals");
  const csSym = Symbol.for("capsurf.internals");
  const rec = (typeof globalThis !== "undefined" && globalThis[ksSym]) || {
    canvasPushMs: 0,
    demandFontOutlines: function () {},
    fontsInflightCount: function () { return 0; },
    fontsReady: function () { return Promise.resolve(); },
  };
  if (typeof globalThis !== "undefined") {
    if (!globalThis[ksSym]) {
      globalThis[ksSym] = rec;
    } else {
      if (globalThis[ksSym].canvasPushMs === undefined) globalThis[ksSym].canvasPushMs = 0;
      if (typeof globalThis[ksSym].fontsInflightCount !== "function") globalThis[ksSym].fontsInflightCount = function () { return 0; };
      if (typeof globalThis[ksSym].fontsReady !== "function") globalThis[ksSym].fontsReady = function () { return Promise.resolve(); };
      if (typeof globalThis[ksSym].demandFontOutlines !== "function") globalThis[ksSym].demandFontOutlines = function () {};
    }
    if (!globalThis[csSym]) globalThis[csSym] = globalThis[ksSym];
  }
  if (typeof window !== "undefined") {
    if (!window[ksSym]) {
      window[ksSym] = rec;
    } else {
      if (window[ksSym].canvasPushMs === undefined) window[ksSym].canvasPushMs = 0;
      if (typeof window[ksSym].fontsInflightCount !== "function") window[ksSym].fontsInflightCount = function () { return 0; };
      if (typeof window[ksSym].fontsReady !== "function") window[ksSym].fontsReady = function () { return Promise.resolve(); };
      if (typeof window[ksSym].demandFontOutlines !== "function") window[ksSym].demandFontOutlines = function () {};
    }
    if (!window[csSym]) window[csSym] = window[ksSym];
  }
})();

// V8-compatible Error.prototype.stack: lead with "Name: message" (or "Name" when message is empty)
(function () {
  if (typeof Error === "undefined" || !Error.prototype) return;
  const desc = Object.getOwnPropertyDescriptor(Error.prototype, "stack");
  if (!desc || typeof desc.get !== "function" || !desc.configurable) return;
  const origGet = desc.get;
  const origSet = desc.set;
  Object.defineProperty(Error.prototype, "stack", {
    configurable: true,
    enumerable: Boolean(desc.enumerable),
    get: function () {
      const raw = origGet.call(this);
      if (typeof raw !== "string") return raw;
      let header;
      try {
        const name = this.name === undefined ? "Error" : String(this.name);
        const msg = this.message === undefined ? "" : String(this.message);
        header = name === "" ? msg : (msg === "" ? name : name + ": " + msg);
      } catch (_) {
        header = "Error";
      }
      if (!header) return raw;
      if (raw === header || raw.startsWith(header + "\n")) return raw;
      return raw ? header + "\n" + raw : header;
    },
    set: origSet,
  });
})();
