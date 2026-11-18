// src/js/runtime.js
// CapSurf High-Performance Native Web API Runtime
// Auto-bundled from src/js/shim/ (25 modules) - DO NOT EDIT BY HAND

(function (global) {
  "use strict";

  // =============================================================
  // Module: constants.js
  // =============================================================
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

  // =============================================================
  // Module: util/strings.js
  // =============================================================
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

  // =============================================================
  // Module: util/validation.js
  // =============================================================
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

  // =============================================================
  // Module: events/event.js
  // =============================================================
  // src/js/shim/events/event.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/events/event.ts

  const _eventState = new WeakMap();
  const _trustedFlags = new WeakMap();
  const _customEventState = new WeakMap();
  const _uiEventState = new WeakMap();
  const _mouseEventState = new WeakMap();
  const _keyboardEventState = new WeakMap();
  const _focusEventState = new WeakMap();
  const _inputEventState = new WeakMap();
  const _messageEventState = new WeakMap();
  const _submitEventState = new WeakMap();
  const _storageEventState = new WeakMap();
  const _compositionEventState = new WeakMap();
  const _textEventState = new WeakMap();
  const _wheelEventState = new WeakMap();
  const _pointerEventState = new WeakMap();
  const _dragEventState = new WeakMap();
  const _errorEventState = new WeakMap();
  const _promiseRejectionEventState = new WeakMap();
  const _deviceMotionEventState = new WeakMap();
  const _accelerationState = new WeakMap();
  const _rotationRateState = new WeakMap();
  const _deviceOrientationEventState = new WeakMap();
  const _pageTransitionEventState = new WeakMap();
  const _hashChangeEventState = new WeakMap();
  const _popStateEventState = new WeakMap();
  const _transitionEventState = new WeakMap();
  const _animationEventState = new WeakMap();
  const _beforeUnloadEventState = new WeakMap();

  const _internalConstruct = Symbol("internalConstruct");

  function nullableDouble(value, member) {
    if (value === undefined || value === null) return null;
    const n = Number(value);
    if (!Number.isFinite(n)) throw new TypeError(member + " is not a finite number.");
    return n;
  }

  // -------------------------------------------------------------
  // Base Event
  // -------------------------------------------------------------
  function Event(type, options) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to construct 'Event': 1 argument required, but only 0 present.");
    }
    const typeStr = String(type);
    const opts = (options !== null && typeof options === "object") ? options : {};
    const s = {
      type: typeStr,
      bubbles: Boolean(opts.bubbles),
      cancelable: opts.cancelable !== undefined ? Boolean(opts.cancelable) : (typeStr === "click" || typeStr === "submit"),
      composed: Boolean(opts.composed),
      timeStamp: typeof performance !== "undefined" && performance.now ? performance.now() : Date.now(),
      target: null,
      currentTarget: null,
      relatedTarget: null,
      eventPhase: 0,
      stopPropagation: false,
      stopImmediatePropagation: false,
      canceled: false,
      isDispatching: false,
      dispatched: false,
      initialized: true,
      path: null,
    };
    _eventState.set(this, s);
    _trustedFlags.set(this, false);

    Object.defineProperty(this, "isTrusted", {
      get: function () {
        return Boolean(_trustedFlags.get(this));
      },
      set: undefined,
      enumerable: true,
      configurable: false,
    });
  }

  Event.NONE = 0;
  Event.CAPTURING_PHASE = 1;
  Event.AT_TARGET = 2;
  Event.BUBBLING_PHASE = 3;

  Event.prototype.NONE = 0;
  Event.prototype.CAPTURING_PHASE = 1;
  Event.prototype.AT_TARGET = 2;
  Event.prototype.BUBBLING_PHASE = 3;

  Object.defineProperty(Event.prototype, "type", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.type : "";
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "bubbles", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.bubbles : false;
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "cancelable", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.cancelable : false;
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "composed", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.composed : false;
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "timeStamp", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.timeStamp : 0;
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "target", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.target : null;
    },
    set: function (v) {
      const s = _eventState.get(this);
      if (s) s.target = v;
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "srcElement", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.target : null;
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "currentTarget", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.currentTarget : null;
    },
    set: function (v) {
      const s = _eventState.get(this);
      if (s) s.currentTarget = v;
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "eventPhase", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.eventPhase : 0;
    },
    set: function (v) {
      const s = _eventState.get(this);
      if (s) s.eventPhase = Number(v);
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "defaultPrevented", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.canceled : false;
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "returnValue", {
    get: function () {
      const s = _eventState.get(this);
      return s ? !s.canceled : true;
    },
    set: function (v) {
      if (v === false) this.preventDefault();
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "cancelBubble", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.stopPropagation : false;
    },
    set: function (v) {
      const s = _eventState.get(this);
      if (s && v) s.stopPropagation = true;
    },
    enumerable: false,
    configurable: true,
  });

  // Non-enumerable internal helpers for dispatch.js
  Object.defineProperty(Event.prototype, "_stopped", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.stopPropagation : false;
    },
    set: function (v) {
      const s = _eventState.get(this);
      if (s) s.stopPropagation = Boolean(v);
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "_immediateStopped", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.stopImmediatePropagation : false;
    },
    set: function (v) {
      const s = _eventState.get(this);
      if (s) s.stopImmediatePropagation = Boolean(v);
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "_canceled", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.canceled : false;
    },
    set: function (v) {
      const s = _eventState.get(this);
      if (s) s.canceled = Boolean(v);
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "_isDispatching", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.isDispatching : false;
    },
    set: function (v) {
      const s = _eventState.get(this);
      if (s) {
        s.isDispatching = Boolean(v);
        if (v) s.dispatched = true;
      }
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "_isTrusted", {
    get: function () {
      return Boolean(_trustedFlags.get(this));
    },
    set: function (v) {
      _trustedFlags.set(this, Boolean(v));
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "_type", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.type : "";
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "_path", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.path : null;
    },
    set: function (v) {
      const s = _eventState.get(this);
      if (s) s.path = v;
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(Event.prototype, "_dispatched", {
    get: function () {
      const s = _eventState.get(this);
      return s ? s.dispatched : false;
    },
    set: function (v) {
      const s = _eventState.get(this);
      if (s) s.dispatched = Boolean(v);
    },
    enumerable: false,
    configurable: true,
  });

  Event.prototype.stopPropagation = function () {
    const s = _eventState.get(this);
    if (s) s.stopPropagation = true;
  };

  Event.prototype.stopImmediatePropagation = function () {
    const s = _eventState.get(this);
    if (s) {
      s.stopPropagation = true;
      s.stopImmediatePropagation = true;
    }
  };

  Event.prototype.preventDefault = function () {
    const s = _eventState.get(this);
    if (s && s.cancelable) {
      s.canceled = true;
    }
  };

  Event.prototype.composedPath = function () {
    const s = _eventState.get(this);
    return (s && s.path) ? s.path.slice() : [];
  };

  Event.prototype.initEvent = function (type, bubbles, cancelable) {
    if (arguments.length < 1) {
      throw new TypeError("Event.initEvent: type argument is required");
    }
    const s = _eventState.get(this);
    if (!s || s.dispatched) return;
    s.type = String(type);
    s.bubbles = Boolean(bubbles);
    s.cancelable = Boolean(cancelable);
    s.canceled = false;
    s.stopPropagation = false;
    s.stopImmediatePropagation = false;
    s.target = null;
    s.initialized = true;
  };

  markNative(Event, "Event");
  markNative(Event.prototype.stopPropagation, "stopPropagation");
  markNative(Event.prototype.stopImmediatePropagation, "stopImmediatePropagation");
  markNative(Event.prototype.preventDefault, "preventDefault");
  markNative(Event.prototype.composedPath, "composedPath");
  markNative(Event.prototype.initEvent, "initEvent");

  // -------------------------------------------------------------
  // CustomEvent
  // -------------------------------------------------------------
  function CustomEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _customEventState.set(this, {
      detail: opts.detail !== undefined ? opts.detail : null,
    });
  }
  CustomEvent.prototype = Object.create(Event.prototype);
  CustomEvent.prototype.constructor = CustomEvent;
  markNative(CustomEvent, "CustomEvent");

  Object.defineProperty(CustomEvent.prototype, "detail", {
    get: function () {
      const s = _customEventState.get(this);
      if (!s) throw new TypeError("Illegal invocation");
      return s.detail;
    },
    enumerable: false,
    configurable: true,
  });

  CustomEvent.prototype.initCustomEvent = function (type, bubbles, cancelable, detail) {
    if (arguments.length < 1) {
      throw new TypeError("CustomEvent.initCustomEvent: type argument is required");
    }
    const es = _eventState.get(this);
    if (!es || es.dispatched) return;
    this.initEvent(type, bubbles, cancelable);
    const s = _customEventState.get(this);
    if (s) {
      s.detail = detail !== undefined ? detail : null;
    }
  };
  markNative(CustomEvent.prototype.initCustomEvent, "initCustomEvent");

  // -------------------------------------------------------------
  // UIEvent
  // -------------------------------------------------------------
  function UIEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    if (opts.view != null && typeof opts.view !== "object") {
      throw new TypeError("UIEvent: view must be a Window or null");
    }
    _uiEventState.set(this, {
      view: opts.view !== undefined ? opts.view : (typeof window !== "undefined" ? window : null),
      detail: typeof opts.detail === "number" ? opts.detail : 0,
      which: typeof opts.which === "number" ? opts.which : 0,
    });
  }
  UIEvent.prototype = Object.create(Event.prototype);
  UIEvent.prototype.constructor = UIEvent;
  markNative(UIEvent, "UIEvent");

  ["view", "detail", "which"].forEach(function (prop) {
    Object.defineProperty(UIEvent.prototype, prop, {
      get: function () {
        const s = _uiEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  UIEvent.prototype.initUIEvent = function (type, bubbles, cancelable, view, detail) {
    const es = _eventState.get(this);
    if (!es || es.dispatched) return;
    this.initEvent(type, bubbles, cancelable);
    const s = _uiEventState.get(this);
    if (s) {
      s.view = view ?? null;
      s.detail = typeof detail === "number" ? detail : 0;
    }
  };
  markNative(UIEvent.prototype.initUIEvent, "initUIEvent");

  // -------------------------------------------------------------
  // MouseEvent
  // -------------------------------------------------------------
  function MouseEvent(type, options) {
    UIEvent.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    const clientX = typeof opts.clientX === "number" ? opts.clientX : 0;
    const clientY = typeof opts.clientY === "number" ? opts.clientY : 0;
    const relatedTarget = opts.relatedTarget == null ? null : opts.relatedTarget;
    const s = {
      screenX: typeof opts.screenX === "number" ? opts.screenX : 0,
      screenY: typeof opts.screenY === "number" ? opts.screenY : 0,
      clientX: clientX,
      clientY: clientY,
      pageX: typeof opts.pageX === "number" ? opts.pageX : clientX,
      pageY: typeof opts.pageY === "number" ? opts.pageY : clientY,
      offsetX: typeof opts.offsetX === "number" ? opts.offsetX : clientX,
      offsetY: typeof opts.offsetY === "number" ? opts.offsetY : clientY,
      movementX: typeof opts.movementX === "number" ? opts.movementX : 0,
      movementY: typeof opts.movementY === "number" ? opts.movementY : 0,
      button: typeof opts.button === "number" ? opts.button : 0,
      buttons: typeof opts.buttons === "number" ? opts.buttons : (typeof opts.button === "number" && opts.button === 0 ? 1 : 0),
      ctrlKey: Boolean(opts.ctrlKey),
      shiftKey: Boolean(opts.shiftKey),
      altKey: Boolean(opts.altKey),
      metaKey: Boolean(opts.metaKey),
      relatedTarget: relatedTarget,
    };
    _mouseEventState.set(this, s);
    const es = _eventState.get(this);
    if (es) es.relatedTarget = relatedTarget;
  }
  MouseEvent.prototype = Object.create(UIEvent.prototype);
  MouseEvent.prototype.constructor = MouseEvent;
  markNative(MouseEvent, "MouseEvent");

  ["screenX", "screenY", "clientX", "clientY", "pageX", "pageY", "offsetX", "offsetY", "movementX", "movementY", "button", "buttons", "ctrlKey", "shiftKey", "altKey", "metaKey", "relatedTarget"].forEach(function (prop) {
    Object.defineProperty(MouseEvent.prototype, prop, {
      get: function () {
        const s = _mouseEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  Object.defineProperty(MouseEvent.prototype, "x", {
    get: function () {
      const s = _mouseEventState.get(this);
      if (!s) throw new TypeError("Illegal invocation");
      return s.clientX;
    },
    enumerable: false,
    configurable: true,
  });

  Object.defineProperty(MouseEvent.prototype, "y", {
    get: function () {
      const s = _mouseEventState.get(this);
      if (!s) throw new TypeError("Illegal invocation");
      return s.clientY;
    },
    enumerable: false,
    configurable: true,
  });

  MouseEvent.prototype.getModifierState = function (name) {
    const s = _mouseEventState.get(this);
    if (!s) throw new TypeError("Illegal invocation");
    switch (name) {
      case "Alt": return s.altKey;
      case "Control": return s.ctrlKey;
      case "Shift": return s.shiftKey;
      case "Meta": return s.metaKey;
      default: return false;
    }
  };
  markNative(MouseEvent.prototype.getModifierState, "getModifierState");

  MouseEvent.prototype.initMouseEvent = function (
    type, bubbles, cancelable, view, detail,
    screenX, screenY, clientX, clientY,
    ctrlKey, altKey, shiftKey, metaKey,
    button, relatedTarget
  ) {
    const es = _eventState.get(this);
    if (!es || es.dispatched) return;
    this.initUIEvent(type, bubbles, cancelable, view, detail);
    const s = _mouseEventState.get(this);
    if (s) {
      s.screenX = typeof screenX === "number" ? screenX : 0;
      s.screenY = typeof screenY === "number" ? screenY : 0;
      s.clientX = typeof clientX === "number" ? clientX : 0;
      s.clientY = typeof clientY === "number" ? clientY : 0;
      s.pageX = s.clientX;
      s.pageY = s.clientY;
      s.ctrlKey = Boolean(ctrlKey);
      s.altKey = Boolean(altKey);
      s.shiftKey = Boolean(shiftKey);
      s.metaKey = Boolean(metaKey);
      s.button = typeof button === "number" ? button : 0;
      s.relatedTarget = relatedTarget ?? null;
    }
    es.relatedTarget = relatedTarget ?? null;
  };
  markNative(MouseEvent.prototype.initMouseEvent, "initMouseEvent");

  // -------------------------------------------------------------
  // KeyboardEvent
  // -------------------------------------------------------------
  function KeyboardEvent(type, options) {
    UIEvent.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    const key = typeof opts.key === "string" ? opts.key : "";
    let keyCode = 0;
    if (typeof opts.keyCode === "number") {
      keyCode = opts.keyCode;
    } else if (key && key.length === 1) {
      keyCode = key.toUpperCase().charCodeAt(0);
    }
    let charCode = 0;
    if (typeof opts.charCode === "number") {
      charCode = opts.charCode;
    } else if (type === "keypress" && key && key.length === 1) {
      charCode = key.charCodeAt(0);
    }
    const which = typeof opts.which === "number" ? opts.which : keyCode;
    const s = {
      key: key,
      code: typeof opts.code === "string" ? opts.code : "",
      location: typeof opts.location === "number" ? opts.location : 0,
      repeat: Boolean(opts.repeat),
      isComposing: Boolean(opts.isComposing),
      keyCode: keyCode,
      charCode: charCode,
      which: which,
      ctrlKey: Boolean(opts.ctrlKey),
      shiftKey: Boolean(opts.shiftKey),
      altKey: Boolean(opts.altKey),
      metaKey: Boolean(opts.metaKey),
    };
    _keyboardEventState.set(this, s);
    const uis = _uiEventState.get(this);
    if (uis) uis.which = which;
  }
  KeyboardEvent.prototype = Object.create(UIEvent.prototype);
  KeyboardEvent.prototype.constructor = KeyboardEvent;
  markNative(KeyboardEvent, "KeyboardEvent");

  ["key", "code", "location", "repeat", "isComposing", "keyCode", "charCode", "which", "ctrlKey", "shiftKey", "altKey", "metaKey"].forEach(function (prop) {
    Object.defineProperty(KeyboardEvent.prototype, prop, {
      get: function () {
        const s = _keyboardEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  KeyboardEvent.prototype.getModifierState = function (name) {
    const s = _keyboardEventState.get(this);
    if (!s) throw new TypeError("Illegal invocation");
    switch (name) {
      case "Alt": return s.altKey;
      case "Control": return s.ctrlKey;
      case "Shift": return s.shiftKey;
      case "Meta": return s.metaKey;
      default: return false;
    }
  };
  markNative(KeyboardEvent.prototype.getModifierState, "getModifierState");

  KeyboardEvent.prototype.initKeyboardEvent = function (
    type, bubbles, cancelable, view,
    key, location, ctrlKey, altKey, shiftKey, metaKey
  ) {
    const es = _eventState.get(this);
    if (!es || es.dispatched) return;
    this.initUIEvent(type, bubbles, cancelable, view, 0);
    const s = _keyboardEventState.get(this);
    if (s) {
      s.key = key !== undefined ? String(key) : "";
      s.location = location !== undefined ? (Number(location) >>> 0) : 0;
      s.ctrlKey = Boolean(ctrlKey);
      s.altKey = Boolean(altKey);
      s.shiftKey = Boolean(shiftKey);
      s.metaKey = Boolean(metaKey);
    }
  };
  markNative(KeyboardEvent.prototype.initKeyboardEvent, "initKeyboardEvent");

  // -------------------------------------------------------------
  // FocusEvent
  // -------------------------------------------------------------
  function FocusEvent(type, options) {
    UIEvent.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    const relatedTarget = opts.relatedTarget == null ? null : opts.relatedTarget;
    _focusEventState.set(this, {
      relatedTarget: relatedTarget,
    });
    const es = _eventState.get(this);
    if (es) es.relatedTarget = relatedTarget;
  }
  FocusEvent.prototype = Object.create(UIEvent.prototype);
  FocusEvent.prototype.constructor = FocusEvent;
  markNative(FocusEvent, "FocusEvent");

  Object.defineProperty(FocusEvent.prototype, "relatedTarget", {
    get: function () {
      const s = _focusEventState.get(this);
      if (!s) throw new TypeError("Illegal invocation");
      return s.relatedTarget;
    },
    enumerable: false,
    configurable: true,
  });

  // -------------------------------------------------------------
  // InputEvent
  // -------------------------------------------------------------
  function InputEvent(type, options) {
    UIEvent.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _inputEventState.set(this, {
      data: opts.data !== undefined ? (opts.data === null ? null : String(opts.data)) : null,
      isComposing: Boolean(opts.isComposing),
      inputType: typeof opts.inputType === "string" ? opts.inputType : "",
      dataTransfer: opts.dataTransfer || null,
      targetRanges: Array.isArray(opts.targetRanges) ? opts.targetRanges.slice() : [],
    });
  }
  InputEvent.prototype = Object.create(UIEvent.prototype);
  InputEvent.prototype.constructor = InputEvent;
  markNative(InputEvent, "InputEvent");

  ["data", "isComposing", "inputType", "dataTransfer"].forEach(function (prop) {
    Object.defineProperty(InputEvent.prototype, prop, {
      get: function () {
        const s = _inputEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  InputEvent.prototype.getTargetRanges = function () {
    const s = _inputEventState.get(this);
    if (!s) throw new TypeError("Illegal invocation");
    return s.targetRanges.slice();
  };
  markNative(InputEvent.prototype.getTargetRanges, "getTargetRanges");

  // -------------------------------------------------------------
  // MessageEvent
  // -------------------------------------------------------------
  function MessageEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _messageEventState.set(this, {
      data: opts.data !== undefined ? opts.data : null,
      origin: opts.origin !== undefined ? String(opts.origin) : "",
      lastEventId: opts.lastEventId !== undefined ? String(opts.lastEventId) : "",
      source: opts.source !== undefined ? opts.source : null,
      ports: Array.isArray(opts.ports) ? Object.freeze(opts.ports.slice()) : Object.freeze([]),
    });
  }
  MessageEvent.prototype = Object.create(Event.prototype);
  MessageEvent.prototype.constructor = MessageEvent;
  markNative(MessageEvent, "MessageEvent");

  ["data", "origin", "lastEventId", "source", "ports"].forEach(function (prop) {
    Object.defineProperty(MessageEvent.prototype, prop, {
      get: function () {
        const s = _messageEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  MessageEvent.prototype.initMessageEvent = function (
    type, bubbles, cancelable, data, origin, lastEventId, source, ports
  ) {
    const es = _eventState.get(this);
    if (!es || es.dispatched) return;
    this.initEvent(type, bubbles, cancelable);
    const s = _messageEventState.get(this);
    if (s) {
      s.data = data === undefined ? null : data;
      s.origin = origin !== undefined ? String(origin) : "";
      s.lastEventId = lastEventId !== undefined ? String(lastEventId) : "";
      s.source = source ?? null;
      s.ports = Array.isArray(ports) ? Object.freeze(ports.slice()) : Object.freeze([]);
    }
  };
  markNative(MessageEvent.prototype.initMessageEvent, "initMessageEvent");

  // -------------------------------------------------------------
  // SubmitEvent
  // -------------------------------------------------------------
  function SubmitEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _submitEventState.set(this, {
      submitter: opts.submitter !== undefined ? opts.submitter : null,
    });
  }
  SubmitEvent.prototype = Object.create(Event.prototype);
  SubmitEvent.prototype.constructor = SubmitEvent;
  markNative(SubmitEvent, "SubmitEvent");

  Object.defineProperty(SubmitEvent.prototype, "submitter", {
    get: function () {
      const s = _submitEventState.get(this);
      if (!s) throw new TypeError("Illegal invocation");
      return s.submitter;
    },
    enumerable: false,
    configurable: true,
  });

  // -------------------------------------------------------------
  // StorageEvent
  // -------------------------------------------------------------
  function StorageEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _storageEventState.set(this, {
      key: opts.key !== undefined ? (opts.key === null ? null : String(opts.key)) : null,
      oldValue: opts.oldValue !== undefined ? (opts.oldValue === null ? null : String(opts.oldValue)) : null,
      newValue: opts.newValue !== undefined ? (opts.newValue === null ? null : String(opts.newValue)) : null,
      url: opts.url !== undefined ? String(opts.url) : "",
      storageArea: opts.storageArea !== undefined ? opts.storageArea : null,
    });
  }
  StorageEvent.prototype = Object.create(Event.prototype);
  StorageEvent.prototype.constructor = StorageEvent;
  markNative(StorageEvent, "StorageEvent");

  ["key", "oldValue", "newValue", "url", "storageArea"].forEach(function (prop) {
    Object.defineProperty(StorageEvent.prototype, prop, {
      get: function () {
        const s = _storageEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  StorageEvent.prototype.initStorageEvent = function (
    type, bubbles, cancelable, key, oldValue, newValue, url, storageArea
  ) {
    const es = _eventState.get(this);
    if (!es || es.dispatched) return;
    this.initEvent(type, bubbles, cancelable);
    const s = _storageEventState.get(this);
    if (s) {
      s.key = (key === undefined || key === null) ? null : String(key);
      s.oldValue = (oldValue === undefined || oldValue === null) ? null : String(oldValue);
      s.newValue = (newValue === undefined || newValue === null) ? null : String(newValue);
      s.url = url !== undefined ? String(url) : "";
      s.storageArea = storageArea ?? null;
    }
  };
  markNative(StorageEvent.prototype.initStorageEvent, "initStorageEvent");

  // -------------------------------------------------------------
  // CompositionEvent
  // -------------------------------------------------------------
  function CompositionEvent(type, options) {
    UIEvent.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _compositionEventState.set(this, {
      data: typeof opts.data === "string" ? opts.data : "",
    });
  }
  CompositionEvent.prototype = Object.create(UIEvent.prototype);
  CompositionEvent.prototype.constructor = CompositionEvent;
  markNative(CompositionEvent, "CompositionEvent");

  Object.defineProperty(CompositionEvent.prototype, "data", {
    get: function () {
      const s = _compositionEventState.get(this);
      if (!s) throw new TypeError("Illegal invocation");
      return s.data;
    },
    enumerable: false,
    configurable: true,
  });

  CompositionEvent.prototype.initCompositionEvent = function (type, bubbles, cancelable, view, data) {
    const es = _eventState.get(this);
    if (!es || es.dispatched) return;
    this.initUIEvent(type, bubbles, cancelable, view, 0);
    const s = _compositionEventState.get(this);
    if (s) {
      s.data = data !== undefined ? String(data) : "";
    }
  };
  markNative(CompositionEvent.prototype.initCompositionEvent, "initCompositionEvent");

  // -------------------------------------------------------------
  // TextEvent
  // -------------------------------------------------------------
  function TextEvent(type, options) {
    UIEvent.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _textEventState.set(this, {
      data: opts.data !== undefined ? String(opts.data) : "",
    });
  }
  TextEvent.prototype = Object.create(UIEvent.prototype);
  TextEvent.prototype.constructor = TextEvent;
  markNative(TextEvent, "TextEvent");

  Object.defineProperty(TextEvent.prototype, "data", {
    get: function () {
      const s = _textEventState.get(this);
      if (!s) throw new TypeError("Illegal invocation");
      return s.data;
    },
    enumerable: false,
    configurable: true,
  });

  TextEvent.prototype.initTextEvent = function (type, bubbles, cancelable, view, data) {
    const es = _eventState.get(this);
    if (!es || es.dispatched) return;
    this.initUIEvent(type, bubbles, cancelable, view, 0);
    const s = _textEventState.get(this);
    if (s) {
      s.data = data === undefined ? "undefined" : String(data);
    }
  };
  markNative(TextEvent.prototype.initTextEvent, "initTextEvent");

  // -------------------------------------------------------------
  // WheelEvent
  // -------------------------------------------------------------
  function WheelEvent(type, options) {
    MouseEvent.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _wheelEventState.set(this, {
      deltaX: typeof opts.deltaX === "number" ? opts.deltaX : 0,
      deltaY: typeof opts.deltaY === "number" ? opts.deltaY : 0,
      deltaZ: typeof opts.deltaZ === "number" ? opts.deltaZ : 0,
      deltaMode: typeof opts.deltaMode === "number" ? opts.deltaMode : 0,
      momentum: Boolean(opts.momentum),
    });
  }
  WheelEvent.prototype = Object.create(MouseEvent.prototype);
  WheelEvent.prototype.constructor = WheelEvent;
  WheelEvent.DOM_DELTA_PIXEL = 0;
  WheelEvent.DOM_DELTA_LINE = 1;
  WheelEvent.DOM_DELTA_PAGE = 2;
  WheelEvent.prototype.DOM_DELTA_PIXEL = 0;
  WheelEvent.prototype.DOM_DELTA_LINE = 1;
  WheelEvent.prototype.DOM_DELTA_PAGE = 2;
  markNative(WheelEvent, "WheelEvent");

  ["deltaX", "deltaY", "deltaZ", "deltaMode", "momentum"].forEach(function (prop) {
    Object.defineProperty(WheelEvent.prototype, prop, {
      get: function () {
        const s = _wheelEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  // -------------------------------------------------------------
  // PointerEvent
  // -------------------------------------------------------------
  function PointerEvent(type, options) {
    MouseEvent.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _pointerEventState.set(this, {
      pointerId: typeof opts.pointerId === "number" ? opts.pointerId : 0,
      width: typeof opts.width === "number" ? opts.width : 1,
      height: typeof opts.height === "number" ? opts.height : 1,
      pressure: typeof opts.pressure === "number" ? opts.pressure : 0,
      tangentialPressure: typeof opts.tangentialPressure === "number" ? opts.tangentialPressure : 0,
      tiltX: typeof opts.tiltX === "number" ? opts.tiltX : 0,
      tiltY: typeof opts.tiltY === "number" ? opts.tiltY : 0,
      twist: typeof opts.twist === "number" ? opts.twist : 0,
      altitudeAngle: typeof opts.altitudeAngle === "number" ? opts.altitudeAngle : Math.PI / 2,
      azimuthAngle: typeof opts.azimuthAngle === "number" ? opts.azimuthAngle : 0,
      pointerType: typeof opts.pointerType === "string" ? opts.pointerType : "",
      isPrimary: Boolean(opts.isPrimary),
      persistentDeviceId: typeof opts.persistentDeviceId === "number" ? opts.persistentDeviceId : 0,
      coalesced: Array.isArray(opts.coalescedEvents) ? opts.coalescedEvents.slice() : [],
      predicted: Array.isArray(opts.predictedEvents) ? opts.predictedEvents.slice() : [],
    });
  }
  PointerEvent.prototype = Object.create(MouseEvent.prototype);
  PointerEvent.prototype.constructor = PointerEvent;
  markNative(PointerEvent, "PointerEvent");

  ["pointerId", "width", "height", "pressure", "tangentialPressure", "tiltX", "tiltY", "twist", "altitudeAngle", "azimuthAngle", "pointerType", "isPrimary", "persistentDeviceId"].forEach(function (prop) {
    Object.defineProperty(PointerEvent.prototype, prop, {
      get: function () {
        const s = _pointerEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  PointerEvent.prototype.getCoalescedEvents = function () {
    const s = _pointerEventState.get(this);
    if (!s) throw new TypeError("Illegal invocation");
    return s.coalesced.slice();
  };
  markNative(PointerEvent.prototype.getCoalescedEvents, "getCoalescedEvents");

  PointerEvent.prototype.getPredictedEvents = function () {
    const s = _pointerEventState.get(this);
    if (!s) throw new TypeError("Illegal invocation");
    return s.predicted.slice();
  };
  markNative(PointerEvent.prototype.getPredictedEvents, "getPredictedEvents");

  // -------------------------------------------------------------
  // DragEvent
  // -------------------------------------------------------------
  function DragEvent(type, options) {
    MouseEvent.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _dragEventState.set(this, {
      dataTransfer: opts.dataTransfer || null,
    });
  }
  DragEvent.prototype = Object.create(MouseEvent.prototype);
  DragEvent.prototype.constructor = DragEvent;
  markNative(DragEvent, "DragEvent");

  Object.defineProperty(DragEvent.prototype, "dataTransfer", {
    get: function () {
      const s = _dragEventState.get(this);
      if (!s) throw new TypeError("Illegal invocation");
      return s.dataTransfer;
    },
    enumerable: false,
    configurable: true,
  });

  // -------------------------------------------------------------
  // ErrorEvent
  // -------------------------------------------------------------
  function ErrorEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _errorEventState.set(this, {
      message: typeof opts.message === "string" ? opts.message : "",
      filename: typeof opts.filename === "string" ? opts.filename : "",
      lineno: (opts.lineno | 0) || 0,
      colno: (opts.colno | 0) || 0,
      error: opts.error !== undefined ? opts.error : null,
    });
  }
  ErrorEvent.prototype = Object.create(Event.prototype);
  ErrorEvent.prototype.constructor = ErrorEvent;
  markNative(ErrorEvent, "ErrorEvent");

  ["message", "filename", "lineno", "colno", "error"].forEach(function (prop) {
    Object.defineProperty(ErrorEvent.prototype, prop, {
      get: function () {
        const s = _errorEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  // -------------------------------------------------------------
  // PromiseRejectionEvent
  // -------------------------------------------------------------
  function PromiseRejectionEvent(type, options) {
    const promise = (options == null) ? undefined : options.promise;
    if (promise === null || (typeof promise !== "object" && typeof promise !== "function")) {
      throw new TypeError("Failed to construct 'PromiseRejectionEvent': the init dictionary needs a promise object.");
    }
    Event.call(this, type, options);
    _promiseRejectionEventState.set(this, {
      promise: promise,
      reason: options.reason,
    });
  }
  PromiseRejectionEvent.prototype = Object.create(Event.prototype);
  PromiseRejectionEvent.prototype.constructor = PromiseRejectionEvent;
  markNative(PromiseRejectionEvent, "PromiseRejectionEvent");

  ["promise", "reason"].forEach(function (prop) {
    Object.defineProperty(PromiseRejectionEvent.prototype, prop, {
      get: function () {
        const s = _promiseRejectionEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  // -------------------------------------------------------------
  // DeviceMotionEventAcceleration & DeviceMotionEventRotationRate
  // -------------------------------------------------------------
  function DeviceMotionEventAcceleration(token, init) {
    if (token !== _internalConstruct) throw new TypeError("Illegal constructor");
    const i = init || {};
    _accelerationState.set(this, {
      x: nullableDouble(i.x, "x"),
      y: nullableDouble(i.y, "y"),
      z: nullableDouble(i.z, "z"),
    });
  }
  DeviceMotionEventAcceleration.prototype = Object.create(Object.prototype);
  DeviceMotionEventAcceleration.prototype.constructor = DeviceMotionEventAcceleration;
  markNative(DeviceMotionEventAcceleration, "DeviceMotionEventAcceleration");

  ["x", "y", "z"].forEach(function (prop) {
    Object.defineProperty(DeviceMotionEventAcceleration.prototype, prop, {
      get: function () {
        const s = _accelerationState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  function DeviceMotionEventRotationRate(token, init) {
    if (token !== _internalConstruct) throw new TypeError("Illegal constructor");
    const i = init || {};
    _rotationRateState.set(this, {
      alpha: nullableDouble(i.alpha, "alpha"),
      beta: nullableDouble(i.beta, "beta"),
      gamma: nullableDouble(i.gamma, "gamma"),
    });
  }
  DeviceMotionEventRotationRate.prototype = Object.create(Object.prototype);
  DeviceMotionEventRotationRate.prototype.constructor = DeviceMotionEventRotationRate;
  markNative(DeviceMotionEventRotationRate, "DeviceMotionEventRotationRate");

  ["alpha", "beta", "gamma"].forEach(function (prop) {
    Object.defineProperty(DeviceMotionEventRotationRate.prototype, prop, {
      get: function () {
        const s = _rotationRateState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  // -------------------------------------------------------------
  // DeviceMotionEvent
  // -------------------------------------------------------------
  function DeviceMotionEvent(type, options) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to construct 'DeviceMotionEvent': 1 argument required, but only 0 present.");
    }
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    let acceleration = null;
    if (opts.acceleration != null) {
      acceleration = new DeviceMotionEventAcceleration(_internalConstruct, opts.acceleration);
    }
    let accelerationIncludingGravity = null;
    if (opts.accelerationIncludingGravity != null) {
      accelerationIncludingGravity = new DeviceMotionEventAcceleration(_internalConstruct, opts.accelerationIncludingGravity);
    }
    let rotationRate = null;
    if (opts.rotationRate != null) {
      rotationRate = new DeviceMotionEventRotationRate(_internalConstruct, opts.rotationRate);
    }
    _deviceMotionEventState.set(this, {
      acceleration: acceleration,
      accelerationIncludingGravity: accelerationIncludingGravity,
      rotationRate: rotationRate,
      interval: typeof opts.interval === "number" ? opts.interval : 0,
    });
  }
  DeviceMotionEvent.prototype = Object.create(Event.prototype);
  DeviceMotionEvent.prototype.constructor = DeviceMotionEvent;
  markNative(DeviceMotionEvent, "DeviceMotionEvent");

  ["acceleration", "accelerationIncludingGravity", "rotationRate", "interval"].forEach(function (prop) {
    Object.defineProperty(DeviceMotionEvent.prototype, prop, {
      get: function () {
        const s = _deviceMotionEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  // -------------------------------------------------------------
  // DeviceOrientationEvent
  // -------------------------------------------------------------
  function DeviceOrientationEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _deviceOrientationEventState.set(this, {
      alpha: opts.alpha != null ? Number(opts.alpha) : null,
      beta: opts.beta != null ? Number(opts.beta) : null,
      gamma: opts.gamma != null ? Number(opts.gamma) : null,
      absolute: Boolean(opts.absolute),
    });
  }
  DeviceOrientationEvent.prototype = Object.create(Event.prototype);
  DeviceOrientationEvent.prototype.constructor = DeviceOrientationEvent;
  markNative(DeviceOrientationEvent, "DeviceOrientationEvent");

  ["alpha", "beta", "gamma", "absolute"].forEach(function (prop) {
    Object.defineProperty(DeviceOrientationEvent.prototype, prop, {
      get: function () {
        const s = _deviceOrientationEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  // -------------------------------------------------------------
  // Other Lifecycle / Transition Events
  // -------------------------------------------------------------
  function HashChangeEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _hashChangeEventState.set(this, {
      oldURL: typeof opts.oldURL === "string" ? opts.oldURL : "",
      newURL: typeof opts.newURL === "string" ? opts.newURL : "",
    });
  }
  HashChangeEvent.prototype = Object.create(Event.prototype);
  HashChangeEvent.prototype.constructor = HashChangeEvent;
  markNative(HashChangeEvent, "HashChangeEvent");
  ["oldURL", "newURL"].forEach(function (prop) {
    Object.defineProperty(HashChangeEvent.prototype, prop, {
      get: function () {
        const s = _hashChangeEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  function PopStateEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _popStateEventState.set(this, {
      state: opts.state !== undefined ? opts.state : null,
      hasUAVisualTransition: Boolean(opts.hasUAVisualTransition),
    });
  }
  PopStateEvent.prototype = Object.create(Event.prototype);
  PopStateEvent.prototype.constructor = PopStateEvent;
  markNative(PopStateEvent, "PopStateEvent");
  ["state", "hasUAVisualTransition"].forEach(function (prop) {
    Object.defineProperty(PopStateEvent.prototype, prop, {
      get: function () {
        const s = _popStateEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  function PageTransitionEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _pageTransitionEventState.set(this, {
      persisted: Boolean(opts.persisted),
    });
  }
  PageTransitionEvent.prototype = Object.create(Event.prototype);
  PageTransitionEvent.prototype.constructor = PageTransitionEvent;
  markNative(PageTransitionEvent, "PageTransitionEvent");
  Object.defineProperty(PageTransitionEvent.prototype, "persisted", {
    get: function () {
      const s = _pageTransitionEventState.get(this);
      if (!s) throw new TypeError("Illegal invocation");
      return s.persisted;
    },
    enumerable: false,
    configurable: true,
  });

  function TransitionEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _transitionEventState.set(this, {
      propertyName: opts.propertyName != null ? String(opts.propertyName) : "",
      elapsedTime: typeof opts.elapsedTime === "number" ? opts.elapsedTime : 0,
      pseudoElement: opts.pseudoElement != null ? String(opts.pseudoElement) : "",
    });
  }
  TransitionEvent.prototype = Object.create(Event.prototype);
  TransitionEvent.prototype.constructor = TransitionEvent;
  markNative(TransitionEvent, "TransitionEvent");
  ["propertyName", "elapsedTime", "pseudoElement"].forEach(function (prop) {
    Object.defineProperty(TransitionEvent.prototype, prop, {
      get: function () {
        const s = _transitionEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  function AnimationEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _animationEventState.set(this, {
      animationName: opts.animationName != null ? String(opts.animationName) : "",
      elapsedTime: typeof opts.elapsedTime === "number" ? opts.elapsedTime : 0,
      pseudoElement: opts.pseudoElement != null ? String(opts.pseudoElement) : "",
    });
  }
  AnimationEvent.prototype = Object.create(Event.prototype);
  AnimationEvent.prototype.constructor = AnimationEvent;
  markNative(AnimationEvent, "AnimationEvent");
  ["animationName", "elapsedTime", "pseudoElement"].forEach(function (prop) {
    Object.defineProperty(AnimationEvent.prototype, prop, {
      get: function () {
        const s = _animationEventState.get(this);
        if (!s) throw new TypeError("Illegal invocation");
        return s[prop];
      },
      enumerable: false,
      configurable: true,
    });
  });

  function BeforeUnloadEvent(type, options) {
    Event.call(this, type, options);
    const opts = (options !== null && typeof options === "object") ? options : {};
    _beforeUnloadEventState.set(this, {
      returnValue: typeof opts.returnValue === "string" ? opts.returnValue : "",
    });
  }
  BeforeUnloadEvent.prototype = Object.create(Event.prototype);
  BeforeUnloadEvent.prototype.constructor = BeforeUnloadEvent;
  markNative(BeforeUnloadEvent, "BeforeUnloadEvent");
  Object.defineProperty(BeforeUnloadEvent.prototype, "returnValue", {
    get: function () {
      const s = _beforeUnloadEventState.get(this);
      if (!s) throw new TypeError("Illegal invocation");
      return s.returnValue;
    },
    set: function (v) {
      const s = _beforeUnloadEventState.get(this);
      if (s) s.returnValue = String(v ?? "");
    },
    enumerable: false,
    configurable: true,
  });

  // =============================================================
  // Module: events/dispatch.js
  // =============================================================
  // src/js/shim/events/dispatch.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/events/dispatch.ts

  const nodeListenersMap = new Map();

  function getListeners(target) {
    if (!target) return null;
    const key = (target._nid !== undefined) ? target._nid : target;
    let map = nodeListenersMap.get(key);
    if (!map) {
      map = new Map();
      nodeListenersMap.set(key, map);
    }
    return map;
  }

  function EventTarget() {}

  const PHASE_NONE = 0;
  const PHASE_CAPTURING = 1;
  const PHASE_AT_TARGET = 2;
  const PHASE_BUBBLING = 3;

  EventTarget.prototype.addEventListener = function (type, callback, options) {
    if (!type || typeof callback !== "function") return;
    const map = getListeners(this);
    if (!map) return;
    const t = String(type).toLowerCase();
    let list = map.get(t);
    if (!list) {
      list = [];
      map.set(t, list);
    }
    const capture = typeof options === "boolean" ? options : Boolean(options && options.capture);
    const once = Boolean(options && options.once);
    const passive = Boolean(options && options.passive);
    for (let i = 0; i < list.length; i++) {
      if (list[i].callback === callback && list[i].capture === capture) {
        return;
      }
    }
    list.push({ callback: callback, capture: capture, once: once, passive: passive });
  };

  EventTarget.prototype.removeEventListener = function (type, callback, options) {
    if (!type || typeof callback !== "function") return;
    const map = getListeners(this);
    if (!map) return;
    const t = String(type).toLowerCase();
    const list = map.get(t);
    if (!list) return;
    const capture = typeof options === "boolean" ? options : Boolean(options && options.capture);
    for (let i = 0; i < list.length; i++) {
      if (list[i].callback === callback && list[i].capture === capture) {
        list.splice(i, 1);
        return;
      }
    }
  };

  const canonicalNodeIds = new WeakMap();

  function recordCanonicalId(el) {
    if (!el || typeof el !== "object") return;
    if (typeof el._id === "number" && !canonicalNodeIds.has(el)) {
      canonicalNodeIds.set(el, el._id);
    }
    if (typeof el._nid === "number" && !canonicalNodeIds.has(el)) {
      canonicalNodeIds.set(el, el._nid);
    }
  }

  function getCanonicalId(el) {
    return canonicalNodeIds.get(el);
  }

  global.__capsurf_recordCanonicalId = recordCanonicalId;
  global.__capsurf_getCanonicalId = getCanonicalId;

  function internalDispatch(target, event, trusted) {
    if (!event || typeof event !== "object") {
      throw new TypeError("Failed to execute 'dispatchEvent': parameter 1 is not of type 'Event'.");
    }
    if (event._isDispatching) {
      throw new DOMException("The event is already being dispatched.", "InvalidStateError");
    }
    if (trusted) {
      event._isTrusted = true;
    }
    event._isDispatching = true;

    try {
      let targetId;
      if (target) {
        targetId = target._id;
      }
      let origRelated = event.relatedTarget;
    } catch (e) {
      event._isDispatching = false;
      throw e;
    }

    try {
      event.target = target;
      const rawType = event._type !== undefined ? event._type : event.type;
      if (typeof rawType !== "string") {
        throw new TypeError("Failed to execute 'dispatchEvent': parameter 1 is not of type 'Event'.");
      }
      const eventType = rawType.toLowerCase();

      // 1. Build propagation path from target up to window
      const path = [];
      let curr = target;
      const visited = new Set();
      while (curr) {
        if (visited.has(curr)) break;
        visited.add(curr);
        path.push(curr);

        if (curr === (typeof window !== "undefined" ? window : globalThis)) {
          break;
        }

        if (typeof document !== "undefined" && curr === document) {
          if (typeof window !== "undefined" && !visited.has(window)) {
            path.push(window);
          }
          break;
        }

        let parent = null;
        try {
          parent = curr.parentElement || curr.parentNode;
        } catch (e) {}

        if (!parent) {
          if (typeof document !== "undefined" && curr !== document && !visited.has(document)) {
            parent = document;
          }
        }
        curr = parent;
      }

      function invoke(entry, phase) {
        event.currentTarget = entry;
        event.eventPhase = phase;

        const map = getListeners(entry);
        if (map) {
          const list = map.get(eventType);
          if (list && list.length > 0) {
            const copy = list.slice();
            for (let i = 0; i < copy.length; i++) {
              if (event._immediateStopped) return;
              const item = copy[i];
              if (phase === PHASE_CAPTURING && !item.capture) continue;
              if (phase === PHASE_BUBBLING && item.capture) continue;

              if (item.once) {
                entry.removeEventListener(eventType, item.callback, item.capture);
              }
              try {
                item.callback.call(entry, event);
              } catch (e) {
                if (typeof console !== "undefined" && console.error) console.error(e);
              }
            }
          }
        }

        // Inline event handler (e.g. element.onclick, element.onsubmit)
        if (phase === PHASE_AT_TARGET || phase === PHASE_BUBBLING) {
          const propName = "on" + eventType;
          if (!entry._hydratedHandlers) entry._hydratedHandlers = {};
          if (!entry._hydratedHandlers[propName] && typeof entry.getAttribute === "function") {
            entry._hydratedHandlers[propName] = true;
            const attrVal = entry.getAttribute(propName);
            if (typeof attrVal === "string" && attrVal && typeof entry[propName] !== "function") {
              try {
                entry[propName] = new Function("event", attrVal);
              } catch (e) {}
            }
          }
          const fn = entry[propName];
          if (typeof fn === "function" && !event._immediateStopped) {
            try {
              const ret = fn.call(entry, event);
              if (ret === false) {
                event.preventDefault();
              }
            } catch (e) {
              if (typeof console !== "undefined" && console.error) console.error(e);
            }
          }
        }
      }

      let activatedButton = null;

      function checkActivation(entry) {
        if (!activatedButton && typeof isSubmitOrResetButton === "function" && isSubmitOrResetButton(entry)) {
          activatedButton = entry;
        }
      }

      // Phase 1: Capturing (from top down to target's parent)
      for (let i = path.length - 1; i >= 1; i--) {
        if (event._stopped) break;
        invoke(path[i], PHASE_CAPTURING);
      }

      // Phase 2: At Target
      if (!event._stopped) {
        invoke(target, PHASE_AT_TARGET);
        if (eventType === "click") {
          checkActivation(target);
        }
      }

      // Phase 3: Bubbling (from target's parent up to top, if bubbles is true)
      if (event.bubbles && !event._stopped) {
        for (let i = 1; i < path.length; i++) {
          if (event._stopped) break;
          invoke(path[i], PHASE_BUBBLING);
          if (eventType === "click") {
            checkActivation(path[i]);
          }
        }
      }

      // Activation behavior for click events
      if (eventType === "click" && activatedButton && !event._canceled) {
        if (typeof handleButtonActivation === "function") {
          handleButtonActivation(activatedButton);
        }
      }
    } finally {
      event._isDispatching = false;
      event.eventPhase = PHASE_NONE;
      event.currentTarget = null;
    }

    return !event._canceled;
  }

  EventTarget.prototype.dispatchEvent = function (event) {
    if (!event || typeof event !== "object") {
      throw new TypeError("Failed to execute 'dispatchEvent': parameter 1 is not of type 'Event'.");
    }
    if (event._isDispatching) {
      throw new DOMException("The event is already being dispatched.", "InvalidStateError");
    }
    event._isTrusted = false;
    return internalDispatch(this, event, false);
  };

  function dispatchTrustedEvent(target, event) {
    if (!target || !event) return false;
    return internalDispatch(target, event, true);
  }
  global.__capsurf_dispatchTrustedEvent = dispatchTrustedEvent;

  markNative(EventTarget, "EventTarget");
  markNative(EventTarget.prototype.addEventListener, "addEventListener");
  markNative(EventTarget.prototype.removeEventListener, "removeEventListener");
  markNative(EventTarget.prototype.dispatchEvent, "dispatchEvent");

  // =============================================================
  // Module: mutations/observer.js
  // =============================================================
  // src/js/shim/mutations/observer.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/mutations/observer.ts

  function MutationRecord(type, target) {
    this.type = type; // "childList", "attributes", "characterData"
    this.target = target;
    this.addedNodes = [];
    this.removedNodes = [];
    this.previousSibling = null;
    this.nextSibling = null;
    this.attributeName = null;
    this.attributeNamespace = null;
    this.oldValue = null;
  }
  markNative(MutationRecord, "MutationRecord");

  const activeObservers = new Set();
  let mutationScheduled = false;

  function scheduleMutationDelivery() {
    if (mutationScheduled) return;
    mutationScheduled = true;
    Promise.resolve().then(function () {
      mutationScheduled = false;
      for (const obs of activeObservers) {
        if (obs._records.length > 0) {
          const batch = obs._records.splice(0, obs._records.length);
          try {
            obs.callback.call(obs, batch, obs);
          } catch (e) {
            if (typeof console !== "undefined" && console.error) console.error(e);
          }
        }
      }
    });
  }

  function isSameTarget(a, b) {
    if (a === b) return true;
    if (a && b && a._nid !== undefined && b._nid !== undefined && a._nid === b._nid) return true;
    return false;
  }

  function notifyMutation(record) {
    if (typeof globalThis !== "undefined") {
      globalThis.__treeSeq = (globalThis.__treeSeq || 0) + 1;
    }
    for (const obs of activeObservers) {
      for (const sub of obs._targets) {
        if (isSameTarget(sub.target, record.target) || (sub.options.subtree && sub.target.contains && sub.target.contains(record.target))) {
          if (record.type === "attributes" && sub.options.attributes) {
            if (!sub.options.attributeFilter || sub.options.attributeFilter.indexOf(record.attributeName) !== -1) {
              obs._records.push(record);
              scheduleMutationDelivery();
              break;
            }
          } else if (record.type === "childList" && sub.options.childList) {
            obs._records.push(record);
            scheduleMutationDelivery();
            break;
          } else if (record.type === "characterData" && sub.options.characterData) {
            obs._records.push(record);
            scheduleMutationDelivery();
            break;
          }
        }
      }
    }
  }

  function MutationObserver(callback) {
    if (typeof callback !== "function") throw new TypeError("Callback required");
    this.callback = callback;
    this._records = [];
    this._targets = [];
  }

  MutationObserver.prototype.observe = function (target, options) {
    if (!target) throw new TypeError("Target required");
    const opts = options || {};
    // Update existing observation of this target if present
    for (let i = 0; i < this._targets.length; i++) {
      if (isSameTarget(this._targets[i].target, target)) {
        this._targets[i].options = opts;
        activeObservers.add(this);
        return;
      }
    }
    this._targets.push({ target: target, options: opts });
    activeObservers.add(this);
  };

  MutationObserver.prototype.disconnect = function () {
    this._records = [];
    this._targets = [];
    activeObservers.delete(this);
  };

  MutationObserver.prototype.takeRecords = function () {
    const records = this._records.slice();
    this._records = [];
    return records;
  };

  markNative(MutationObserver, "MutationObserver");
  markNative(MutationObserver.prototype.observe, "observe");
  markNative(MutationObserver.prototype.disconnect, "disconnect");
  markNative(MutationObserver.prototype.takeRecords, "takeRecords");

  // =============================================================
  // Module: collections.js
  // =============================================================
  // src/js/shim/collections.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/collections.ts

  function DOMTokenList(getter, setter) {
    this._getter = getter;
    this._setter = setter;
  }

  DOMTokenList.prototype._getTokens = function () {
    const str = (this._getter() || "").trim();
    if (!str) return [];
    return str.split(/\s+/);
  };

  DOMTokenList.prototype._setTokens = function (tokens) {
    this._setter(tokens.join(" "));
  };

  Object.defineProperty(DOMTokenList.prototype, "length", {
    get: function () {
      return this._getTokens().length;
    },
    configurable: true,
  });

  DOMTokenList.prototype.item = function (index) {
    const tokens = this._getTokens();
    const i = Number(index);
    return (i >= 0 && i < tokens.length) ? tokens[i] : null;
  };

  DOMTokenList.prototype.contains = function (token) {
    if (!token) return false;
    const tokens = this._getTokens();
    return tokens.indexOf(String(token)) !== -1;
  };

  DOMTokenList.prototype.add = function () {
    const tokens = this._getTokens();
    let changed = false;
    for (let i = 0; i < arguments.length; i++) {
      const token = String(arguments[i]);
      if (!token || /\s/.test(token)) {
        throw new Error("InvalidCharacterError: Invalid token '" + token + "'");
      }
      if (tokens.indexOf(token) === -1) {
        tokens.push(token);
        changed = true;
      }
    }
    if (changed) this._setTokens(tokens);
  };

  DOMTokenList.prototype.remove = function () {
    let tokens = this._getTokens();
    let changed = false;
    for (let i = 0; i < arguments.length; i++) {
      const token = String(arguments[i]);
      const idx = tokens.indexOf(token);
      if (idx !== -1) {
        tokens.splice(idx, 1);
        changed = true;
      }
    }
    if (changed) this._setTokens(tokens);
  };

  DOMTokenList.prototype.toggle = function (token, force) {
    const str = String(token);
    if (!str || /\s/.test(str)) {
      throw new Error("InvalidCharacterError: Invalid token '" + str + "'");
    }
    const has = this.contains(str);
    if (force === true) {
      if (!has) this.add(str);
      return true;
    } else if (force === false) {
      if (has) this.remove(str);
      return false;
    } else {
      if (has) {
        this.remove(str);
        return false;
      } else {
        this.add(str);
        return true;
      }
    }
  };

  DOMTokenList.prototype.replace = function (oldToken, newToken) {
    const tokens = this._getTokens();
    const idx = tokens.indexOf(String(oldToken));
    if (idx === -1) return false;
    tokens[idx] = String(newToken);
    this._setTokens(tokens);
    return true;
  };

  DOMTokenList.prototype.forEach = function (callback, thisArg) {
    const tokens = this._getTokens();
    for (let i = 0; i < tokens.length; i++) {
      callback.call(thisArg, tokens[i], i, this);
    }
  };

  DOMTokenList.prototype.values = function () {
    return this._getTokens()[Symbol.iterator]();
  };

  DOMTokenList.prototype.entries = function () {
    const tokens = this._getTokens();
    const arr = [];
    for (let i = 0; i < tokens.length; i++) arr.push([i, tokens[i]]);
    return arr[Symbol.iterator]();
  };

  DOMTokenList.prototype.toString = function () {
    return this._getter() || "";
  };

  DOMTokenList.prototype[Symbol.iterator] = function () {
    return this.values();
  };

  markNative(DOMTokenList, "DOMTokenList");
  markNative(DOMTokenList.prototype.add, "add");
  markNative(DOMTokenList.prototype.remove, "remove");
  markNative(DOMTokenList.prototype.toggle, "toggle");
  markNative(DOMTokenList.prototype.contains, "contains");
  markNative(DOMTokenList.prototype.item, "item");

  function findNamedItemInList(list, name) {
    if (name == null || name === "") return null;
    const key = String(name);
    const len = list ? (list.length || 0) : 0;
    for (let i = 0; i < len; i++) {
      const el = list[i];
      if (!el) continue;
      if (el.id === key || (el.getAttribute && el.getAttribute("id") === key)) {
        return el;
      }
    }
    for (let i = 0; i < len; i++) {
      const el = list[i];
      if (!el) continue;
      if (el.name === key || (el.getAttribute && el.getAttribute("name") === key)) {
        return el;
      }
    }
    return null;
  }

  function makeHtmlCollection(getList, customTarget) {
    const target = customTarget || Object.create(HTMLCollection.prototype);
    const proxy = new Proxy(target, {
      get: function (t, prop, receiver) {
        if (prop === "length") {
          return getList().length;
        }
        if (typeof prop === "string" && /^\d+$/.test(prop)) {
          const i = Number(prop);
          const list = getList();
          if (i >= 0 && i < list.length) {
            return list[i];
          }
          return undefined;
        }
        if (prop === "item") {
          return function (index) {
            const list = getList();
            const i = Number(index);
            return (i >= 0 && i < list.length) ? list[i] : null;
          };
        }
        if (prop === "namedItem") {
          return function (name) {
            return findNamedItemInList(getList(), name);
          };
        }
        if (prop === Symbol.iterator) {
          return function () {
            const list = getList();
            let idx = 0;
            return {
              next: function () {
                if (idx < list.length) {
                  return { value: list[idx++], done: false };
                }
                return { value: undefined, done: true };
              }
            };
          };
        }
        if (prop in t) {
          return Reflect.get(t, prop, receiver);
        }
        if (typeof prop === "string") {
          const named = findNamedItemInList(getList(), prop);
          if (named !== null) {
            return named;
          }
        }
        return Reflect.get(t, prop, receiver);
      },
      set: function (t, prop, value, receiver) {
        if (t._isOptionsCollection && t._owner) {
          if (prop === "length") {
            setSelectOptionsLength(t._owner, value);
            return true;
          }
          if (typeof prop === "string" && /^\d+$/.test(prop)) {
            setSelectOptionAt(t._owner, Number(prop), value);
            return true;
          }
        } else {
          if (prop === "length") return false;
          if (typeof prop === "string" && /^\d+$/.test(prop)) return false;
        }
        return Reflect.set(t, prop, value, receiver);
      },
      has: function (t, prop) {
        if (prop === "length") return true;
        if (typeof prop === "string" && /^\d+$/.test(prop)) {
          return Number(prop) < getList().length;
        }
        if (prop in t) return true;
        if (typeof prop === "string") {
          return findNamedItemInList(getList(), prop) !== null;
        }
        return false;
      },
      ownKeys: function (t) {
        const list = getList();
        const keys = [];
        for (let i = 0; i < list.length; i++) {
          keys.push(String(i));
        }
        for (let i = 0; i < list.length; i++) {
          const el = list[i];
          if (!el) continue;
          const id = el.id || (el.getAttribute && el.getAttribute("id"));
          if (id && keys.indexOf(id) === -1) keys.push(id);
          const name = el.name || (el.getAttribute && el.getAttribute("name"));
          if (name && keys.indexOf(name) === -1) keys.push(name);
        }
        return keys;
      },
      getOwnPropertyDescriptor: function (t, prop) {
        if (prop === "length") {
          return { value: getList().length, writable: !!t._isOptionsCollection, enumerable: false, configurable: true };
        }
        if (typeof prop === "string" && /^\d+$/.test(prop)) {
          const i = Number(prop);
          const list = getList();
          if (i >= 0 && i < list.length) {
            return { value: list[i], writable: !!t._isOptionsCollection, enumerable: true, configurable: true };
          }
        }
        if (typeof prop === "string") {
          const named = findNamedItemInList(getList(), prop);
          if (named !== null) {
            return { value: named, writable: false, enumerable: false, configurable: true };
          }
        }
        return Reflect.getOwnPropertyDescriptor(t, prop);
      }
    });
    return proxy;
  }

  function getAllDocumentElements(doc) {
    const elements = [];
    const seen = (typeof Set !== "undefined") ? new Set() : null;

    function addEl(el) {
      if (!el || el.nodeType !== 1) return;
      if (seen) {
        if (seen.has(el)) return;
        seen.add(el);
      } else {
        if (elements.indexOf(el) !== -1) return;
      }
      elements.push(el);
    }

    if (doc && typeof doc.querySelectorAll === "function") {
      try {
        const q = doc.querySelectorAll("*");
        if (q && typeof q.length === "number") {
          for (let i = 0; i < q.length; i++) {
            addEl(q[i]);
          }
        }
      } catch (e) {}
    }

    function walk(node) {
      if (!node) return;
      if (node._children && node._children.length > 0) {
        for (let i = 0; i < node._children.length; i++) {
          const c = node._children[i];
          if (c && c.nodeType === 1) { // ELEMENT_NODE
            addEl(c);
            walk(c);
          }
        }
      } else {
        let child = node.firstChild;
        while (child) {
          if (child.nodeType === 1) {
            addEl(child);
            walk(child);
          }
          child = child.nextSibling;
        }
      }
    }

    const root = doc ? (doc.documentElement || doc.body || doc) : null;
    if (root) {
      if (root.nodeType === 1) {
        addEl(root);
        walk(root);
      } else {
        walk(root);
      }
    }
    return elements;
  }

  function HTMLCollection(items) {
    const list = items || [];
    return makeHtmlCollection(function () { return list; });
  }
  HTMLCollection.prototype.item = function (index) {
    const i = Number(index);
    return (i >= 0 && i < this.length) ? this[i] : null;
  };
  HTMLCollection.prototype.namedItem = function (name) {
    return findNamedItemInList(this, name);
  };
  HTMLCollection.prototype[Symbol.iterator] = function () {
    const self = this;
    let idx = 0;
    return {
      next: function () {
        if (idx < self.length) {
          return { value: self[idx++], done: false };
        }
        return { value: undefined, done: true };
      }
    };
  };
  markNative(HTMLCollection, "HTMLCollection");

  function HTMLOptionsCollection() {
    throw new TypeError("Illegal constructor");
  }
  HTMLOptionsCollection.prototype = Object.create(HTMLCollection.prototype);
  HTMLOptionsCollection.prototype.constructor = HTMLOptionsCollection;
  Object.defineProperty(HTMLOptionsCollection.prototype, Symbol.toStringTag, {
    value: "HTMLOptionsCollection",
    configurable: true,
  });
  markNative(HTMLOptionsCollection, "HTMLOptionsCollection");

  Object.defineProperty(HTMLOptionsCollection.prototype, "length", {
    get: function () {
      const select = this._owner;
      if (!select) return 0;
      return getSelectOptions(select).length;
    },
    set: function (v) {
      const select = this._owner;
      if (!select) return;
      setSelectOptionsLength(select, v);
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLOptionsCollection.prototype, "selectedIndex", {
    get: function () {
      const select = this._owner;
      return select ? select.selectedIndex : -1;
    },
    set: function (v) {
      const select = this._owner;
      if (select) select.selectedIndex = v;
    },
    configurable: true,
    enumerable: true,
  });

  HTMLOptionsCollection.prototype.add = function (element, before = null) {
    const select = this._owner;
    if (!select) {
      throw new TypeError("HTMLOptionsCollection.add: missing owner select");
    }
    const tag = element && typeof element.tagName === "string" ? element.tagName.toLowerCase() : "";
    if (tag !== "option" && tag !== "optgroup") {
      throw new TypeError("HTMLOptionsCollection.add: argument must be HTMLOptionElement or HTMLOptGroupElement");
    }
    if (before === element) return;
    let refNode = null;
    if (before == null) {
      refNode = null;
    } else if (typeof before === "number") {
      const idx = Number(before) | 0;
      const list = getSelectOptions(select);
      if (idx >= 0 && idx < list.length) {
        refNode = list[idx];
      }
    } else if (typeof before === "object") {
      refNode = before;
    }
    if (refNode) {
      let p = refNode.parentNode;
      let isDesc = false;
      while (p) {
        if (p === select) {
          isDesc = true;
          break;
        }
        p = p.parentNode;
      }
      if (!isDesc) {
        const DEx = typeof DOMException !== "undefined" ? DOMException : Error;
        throw new DEx("HTMLOptionsCollection.add: before is not a descendant of the select", "NotFoundError");
      }
      if (refNode.parentNode && typeof refNode.parentNode.insertBefore === "function") {
        refNode.parentNode.insertBefore(element, refNode);
      } else {
        select.appendChild(element);
      }
    } else {
      select.appendChild(element);
    }
  };
  markNative(HTMLOptionsCollection.prototype.add, "add");

  HTMLOptionsCollection.prototype.remove = function (index) {
    const select = this._owner;
    if (!select) return;
    const i = Number(index) | 0;
    const list = getSelectOptions(select);
    if (i >= 0 && i < list.length) {
      const o = list[i];
      if (o && o.parentNode && typeof o.parentNode.removeChild === "function") {
        o.parentNode.removeChild(o);
      }
    }
  };
  markNative(HTMLOptionsCollection.prototype.remove, "remove");

  function getSelectOptions(selectEl) {
    const opts = [];
    const up = typeof upgradeElement === "function" ? upgradeElement : (typeof globalThis !== "undefined" && globalThis.upgradeElement);
    function collect(node) {
      if (!node) return;
      if (node.firstChild !== undefined) {
        let child = node.firstChild;
        while (child) {
          const tag = (child.tagName || "").toLowerCase();
          if (tag === "option") {
            if (up) up(child);
            opts.push(child);
          } else if (tag === "optgroup") {
            collect(child);
          }
          child = child.nextSibling;
        }
        return;
      }
      if (node._children) {
        for (let i = 0; i < node._children.length; i++) {
          const c = node._children[i];
          if (!c) continue;
          const tag = (c.tagName || "").toLowerCase();
          if (tag === "option") {
            if (up) up(c);
            opts.push(c);
          } else if (tag === "optgroup") {
            collect(c);
          }
        }
      }
    }
    collect(selectEl);
    return opts;
  }

  function setSelectOptionsLength(select, v) {
    const target = Math.max(0, Math.min(100000, Math.floor(Number(v) || 0)));
    const list = getSelectOptions(select);
    const cur = list.length;
    if (target < cur) {
      for (let i = cur - 1; i >= target; i--) {
        const o = list[i];
        if (o && o.parentNode && typeof o.parentNode.removeChild === "function") {
          o.parentNode.removeChild(o);
        }
      }
    } else if (target > cur) {
      const doc = select.ownerDocument || (typeof document !== "undefined" ? document : null);
      for (let i = cur; i < target; i++) {
        const opt = doc && typeof doc.createElement === "function" ? doc.createElement("option") : (typeof HTMLOptionElement === "function" ? new HTMLOptionElement() : null);
        if (opt) select.appendChild(opt);
      }
    }
  }

  function setSelectOptionAt(select, idx, value) {
    const list = getSelectOptions(select);
    const doc = select.ownerDocument || (typeof document !== "undefined" ? document : null);
    if (value === null || value === undefined) {
      if (idx >= 0 && idx < list.length) {
        const o = list[idx];
        if (o && o.parentNode && typeof o.parentNode.removeChild === "function") {
          o.parentNode.removeChild(o);
        }
      }
      return;
    }
    const tag = value && typeof value.tagName === "string" ? value.tagName.toLowerCase() : "";
    if (tag !== "option") {
      throw new TypeError("HTMLOptionsCollection: indexed value must be an HTMLOptionElement");
    }
    if (idx < list.length) {
      const o = list[idx];
      if (o && o.parentNode && typeof o.parentNode.replaceChild === "function") {
        o.parentNode.replaceChild(value, o);
      }
    } else {
      for (let i = list.length; i < idx; i++) {
        const opt = doc && typeof doc.createElement === "function" ? doc.createElement("option") : (typeof HTMLOptionElement === "function" ? new HTMLOptionElement() : null);
        if (opt) select.appendChild(opt);
      }
      select.appendChild(value);
    }
  }

  function makeHtmlOptionsCollection(select) {
    const getList = function () {
      return getSelectOptions(select);
    };
    const target = Object.create(HTMLOptionsCollection.prototype);
    Object.defineProperty(target, "_owner", {
      value: select,
      writable: true,
      configurable: true,
      enumerable: false,
    });
    Object.defineProperty(target, "_isOptionsCollection", {
      value: true,
      writable: true,
      configurable: true,
      enumerable: false,
    });
    return makeHtmlCollection(getList, target);
  }

  const selectOptionsCache = new WeakMap();

  function getSelectOptionsCollection(select) {
    if (!select || typeof select !== "object") return null;
    let coll = selectOptionsCache.get(select);
    if (!coll) {
      coll = makeHtmlOptionsCollection(select);
      selectOptionsCache.set(select, coll);
    }
    return coll;
  }

  const elementChildrenCache = new WeakMap();

  function getElementChildren(parent) {
    const kids = [];
    const up = typeof upgradeElement === "function" ? upgradeElement : (typeof globalThis !== "undefined" && globalThis.upgradeElement);
    if (parent && parent.firstChild !== undefined) {
      let child = parent.firstChild;
      while (child) {
        if (child.nodeType === 1) { // ELEMENT_NODE
          if (up) up(child);
          kids.push(child);
        }
        child = child.nextSibling;
      }
      return kids;
    }
    if (parent && parent._children) {
      for (let i = 0; i < parent._children.length; i++) {
        const c = parent._children[i];
        if (c && c.nodeType === 1) {
          if (up) up(c);
          kids.push(c);
        }
      }
    }
    return kids;
  }

  function getElementChildrenCollection(element) {
    if (!element || typeof element !== "object") return null;
    let coll = elementChildrenCache.get(element);
    if (!coll) {
      coll = makeHtmlCollection(function () {
        return getElementChildren(element);
      });
      elementChildrenCache.set(element, coll);
    }
    return coll;
  }

  function NodeList(items) {
    const list = items || [];
    for (let i = 0; i < list.length; i++) {
      this[i] = list[i];
    }
    this.length = list.length;
  }
  NodeList.prototype.item = function (index) {
    const i = Number(index);
    return (i >= 0 && i < this.length) ? this[i] : null;
  };
  NodeList.prototype.forEach = function (callback, thisArg) {
    for (let i = 0; i < this.length; i++) {
      callback.call(thisArg, this[i], i, this);
    }
  };
  NodeList.prototype[Symbol.iterator] = function () {
    const self = this;
    let idx = 0;
    return {
      next: function () {
        if (idx < self.length) {
          return { value: self[idx++], done: false };
        }
        return { value: undefined, done: true };
      }
    };
  };
  markNative(NodeList, "NodeList");

  function Attr(name, value, ownerElement, namespaceURI, localName, prefix) {
    this.name = String(name);
    this._value = value == null ? "" : String(value);
    this.specified = true;
    this.ownerElement = ownerElement || null;
    this._namespaceURI = namespaceURI !== undefined ? namespaceURI : null;
    const colon = this.name.indexOf(":");
    this._prefix = prefix !== undefined ? prefix : (colon !== -1 ? this.name.slice(0, colon) : null);
    this._localName = localName !== undefined ? localName : (colon !== -1 ? this.name.slice(colon + 1) : this.name);
  }

  Object.defineProperty(Attr.prototype, "value", {
    get: function () {
      return this._value;
    },
    set: function (v) {
      const next = v == null ? "" : String(v);
      this._value = next;
      if (this.ownerElement && typeof this.ownerElement.setAttribute === "function") {
        this.ownerElement.setAttribute(this.name, next);
      }
    },
    configurable: true,
  });

  Object.defineProperty(Attr.prototype, "nodeName", {
    get: function () { return this.name; },
    configurable: true,
  });

  Object.defineProperty(Attr.prototype, "nodeValue", {
    get: function () { return this.value; },
    set: function (v) { this.value = v; },
    configurable: true,
  });

  Object.defineProperty(Attr.prototype, "textContent", {
    get: function () { return this.value; },
    set: function (v) { this.value = v; },
    configurable: true,
  });

  Object.defineProperty(Attr.prototype, "localName", {
    get: function () {
      if (this._localName !== undefined) return this._localName;
      const colon = this.name.indexOf(":");
      return colon !== -1 ? this.name.slice(colon + 1) : this.name.toLowerCase();
    },
    configurable: true,
  });

  Object.defineProperty(Attr.prototype, "prefix", {
    get: function () {
      if (this._prefix !== undefined) return this._prefix;
      const colon = this.name.indexOf(":");
      return colon !== -1 ? this.name.slice(0, colon) : null;
    },
    configurable: true,
  });

  Object.defineProperty(Attr.prototype, "namespaceURI", {
    get: function () {
      return this._namespaceURI !== undefined ? this._namespaceURI : null;
    },
    configurable: true,
  });

  Object.defineProperty(Attr.prototype, "nodeType", {
    value: 2, // ATTRIBUTE_NODE
    writable: false,
    configurable: true,
  });

  markNative(Attr, "Attr");

  function NamedNodeMap(ownerElement) {
    Object.defineProperty(this, "_owner", {
      value: ownerElement || null,
      writable: true,
      enumerable: false,
      configurable: true,
    });
    Object.defineProperty(this, "_list", {
      value: [],
      writable: true,
      enumerable: false,
      configurable: true,
    });
    Object.defineProperty(this, "_byName", {
      value: Object.create(null),
      writable: true,
      enumerable: false,
      configurable: true,
    });
    Object.defineProperty(this, "_length", {
      value: 0,
      writable: true,
      enumerable: false,
      configurable: true,
    });
    this._sync();
  }

  NamedNodeMap.prototype._sync = function () {
    if (!this._owner) return;
    if (typeof this._owner._syncAttrs === "function") {
      this._owner._syncAttrs();
    }
    const currentAttrs = this._owner._attrList || (this._owner._attrs ? Array.from(this._owner._attrs.values()) : []);

    for (let i = 0; i < this._length; i++) {
      delete this[i];
    }

    this._list = [];
    this._byName = Object.create(null);

    for (let i = 0; i < currentAttrs.length; i++) {
      const item = currentAttrs[i];
      const colon = item.name.indexOf(":");
      const pfx = item.prefix !== undefined ? item.prefix : (colon !== -1 ? item.name.slice(0, colon) : null);
      const loc = item.localName !== undefined ? item.localName : (colon !== -1 ? item.name.slice(colon + 1) : item.name);
      const ns = item.namespaceURI !== undefined ? item.namespaceURI : null;
      const attrNode = new Attr(item.name, item.value, this._owner, ns, loc, pfx);
      this[i] = attrNode;
      this._list.push(attrNode);
      this._byName[item.name.toLowerCase()] = attrNode;
    }
    this._length = this._list.length;
  };

  Object.defineProperty(NamedNodeMap.prototype, "length", {
    get: function () {
      this._sync();
      return this._length;
    },
    configurable: true,
    enumerable: false,
  });

  NamedNodeMap.prototype.item = function (index) {
    this._sync();
    const i = Number(index);
    return (i >= 0 && i < this._length) ? this[i] : null;
  };

  NamedNodeMap.prototype.getNamedItem = function (name) {
    this._sync();
    if (typeof name !== "string") name = String(name);
    return this._byName[name.toLowerCase()] || null;
  };

  NamedNodeMap.prototype.setNamedItem = function (attr) {
    if (!attr || !attr.name) throw new TypeError("Attr required");
    const old = this.getNamedItem(attr.name);
    if (this._owner && typeof this._owner.setAttribute === "function") {
      this._owner.setAttribute(attr.name, attr.value);
    }
    this._sync();
    return old;
  };

  NamedNodeMap.prototype.removeNamedItem = function (name) {
    this._sync();
    const key = String(name).toLowerCase();
    const old = this._byName[key];
    if (!old) {
      const err = new Error("NotFoundError: No attribute named '" + name + "'");
      err.name = "NotFoundError";
      throw err;
    }
    if (this._owner && typeof this._owner.removeAttribute === "function") {
      this._owner.removeAttribute(old.name);
    }
    this._sync();
    return old;
  };

  NamedNodeMap.prototype.getNamedItemNS = function (namespace, localName) {
    this._sync();
    const ns = (namespace === undefined || namespace === null || namespace === "") ? null : String(namespace);
    const loc = String(localName).toLowerCase();
    for (let i = 0; i < this._length; i++) {
      const a = this._list[i];
      const aNs = a.namespaceURI || null;
      const aLoc = (a.localName || a.name).toLowerCase();
      if (aNs === ns && aLoc === loc) return a;
    }
    return null;
  };

  NamedNodeMap.prototype.setNamedItemNS = function (attr) {
    if (!attr || !attr.name) throw new TypeError("Attr required");
    const old = this.getNamedItemNS(attr.namespaceURI, attr.localName || attr.name);
    if (this._owner && typeof this._owner.setAttributeNS === "function") {
      this._owner.setAttributeNS(attr.namespaceURI, attr.name, attr.value);
    }
    this._sync();
    return old;
  };

  NamedNodeMap.prototype.removeNamedItemNS = function (namespace, localName) {
    this._sync();
    const old = this.getNamedItemNS(namespace, localName);
    if (!old) {
      const err = new Error("NotFoundError: No attribute matching namespace and localName");
      err.name = "NotFoundError";
      throw err;
    }
    if (this._owner && typeof this._owner.removeAttributeNS === "function") {
      this._owner.removeAttributeNS(namespace, localName);
    }
    this._sync();
    return old;
  };

  NamedNodeMap.prototype[Symbol.iterator] = function () {
    this._sync();
    const list = this._list.slice();
    let idx = 0;
    return {
      next: function () {
        if (idx < list.length) {
          return { value: list[idx++], done: false };
        }
        return { value: undefined, done: true };
      }
    };
  };

  markNative(NamedNodeMap, "NamedNodeMap");
  markNative(NamedNodeMap.prototype.item, "item");
  markNative(NamedNodeMap.prototype.getNamedItem, "getNamedItem");
  markNative(NamedNodeMap.prototype.getNamedItemNS, "getNamedItemNS");
  markNative(NamedNodeMap.prototype.setNamedItem, "setNamedItem");
  markNative(NamedNodeMap.prototype.setNamedItemNS, "setNamedItemNS");
  markNative(NamedNodeMap.prototype.removeNamedItem, "removeNamedItem");
  markNative(NamedNodeMap.prototype.removeNamedItemNS, "removeNamedItemNS");

  if (typeof globalThis !== "undefined") {
    globalThis.NamedNodeMap = NamedNodeMap;
    globalThis.Attr = Attr;
    globalThis.HTMLCollection = HTMLCollection;
    globalThis.HTMLOptionsCollection = HTMLOptionsCollection;
  }
  if (typeof window !== "undefined") {
    window.NamedNodeMap = NamedNodeMap;
    window.Attr = Attr;
    window.HTMLCollection = HTMLCollection;
    window.HTMLOptionsCollection = HTMLOptionsCollection;
  }

  // =============================================================
  // Module: cssom.js
  // =============================================================
  // src/js/shim/cssom.js
  // CSSOM implementation: CSSRule hierarchy, CSSStyleSheet, CSS.supports, CSSStyleDeclaration

  (function (global) {
    "use strict";

    // =========================================================================
    // 1. CSS Properties & Values Validation
    // =========================================================================

    const STANDARD_CSS_PROPERTIES = new Set([
      "accent-color", "align-content", "align-items", "align-self", "all",
      "animation", "animation-composition", "animation-delay", "animation-direction",
      "animation-duration", "animation-fill-mode", "animation-iteration-count",
      "animation-name", "animation-play-state", "animation-timeline", "animation-timing-function",
      "appearance", "aspect-ratio", "backdrop-filter", "backface-visibility",
      "background", "background-attachment", "background-blend-mode", "background-clip",
      "background-color", "background-image", "background-origin", "background-position",
      "background-position-x", "background-position-y", "background-repeat", "background-size",
      "base-palette", "block-size",
      "border", "border-block", "border-block-color", "border-block-end", "border-block-end-color",
      "border-block-end-style", "border-block-end-width", "border-block-start", "border-block-start-color",
      "border-block-start-style", "border-block-start-width", "border-block-style", "border-block-width",
      "border-bottom", "border-bottom-color", "border-bottom-left-radius", "border-bottom-right-radius",
      "border-bottom-style", "border-bottom-width", "border-collapse", "border-color",
      "border-end-end-radius", "border-end-start-radius", "border-image", "border-image-outset",
      "border-image-repeat", "border-image-slice", "border-image-source", "border-image-width",
      "border-inline", "border-inline-color", "border-inline-end", "border-inline-end-color",
      "border-inline-end-style", "border-inline-end-width", "border-inline-start",
      "border-inline-start-color", "border-inline-start-style", "border-inline-start-width",
      "border-inline-style", "border-inline-width", "border-left", "border-left-color",
      "border-left-style", "border-left-width", "border-radius", "border-right",
      "border-right-color", "border-right-style", "border-right-width", "border-spacing",
      "border-start-end-radius", "border-start-start-radius", "border-style", "border-top",
      "border-top-color", "border-top-left-radius", "border-top-right-radius", "border-top-style",
      "border-top-width", "border-width", "bottom", "box-decoration-break", "box-shadow",
      "box-sizing", "break-after", "break-before", "break-inside", "caption-side", "caret-color",
      "clear", "clip", "clip-path", "color", "color-scheme", "column-count", "column-fill",
      "column-gap", "column-rule", "column-rule-color", "column-rule-style", "column-rule-width",
      "column-span", "column-width", "columns", "contain", "contain-intrinsic-block-size",
      "contain-intrinsic-height", "contain-intrinsic-inline-size", "contain-intrinsic-size",
      "contain-intrinsic-width", "container", "container-name", "container-type", "content",
      "content-visibility", "counter-increment", "counter-reset", "counter-set", "cursor",
      "direction", "display", "empty-cells", "filter", "flex", "flex-basis", "flex-direction",
      "flex-flow", "flex-grow", "flex-shrink", "flex-wrap", "float", "font", "font-family",
      "font-feature-settings", "font-kerning", "font-optical-sizing", "font-palette",
      "font-size", "font-size-adjust", "font-stretch", "font-style", "font-synthesis",
      "font-variant", "font-variant-alternates", "font-variant-caps", "font-variant-east-asian",
      "font-variant-ligatures", "font-variant-numeric", "font-variant-position",
      "font-variation-settings", "font-weight", "font-display", "forced-color-adjust", "gap",
      "grid", "grid-area", "grid-auto-columns", "grid-auto-flow", "grid-auto-rows",
      "grid-column", "grid-column-end", "grid-column-gap", "grid-column-start", "grid-gap",
      "grid-row", "grid-row-end", "grid-row-gap", "grid-row-start", "grid-template",
      "grid-template-areas", "grid-template-columns", "grid-template-rows", "height",
      "hyphenate-character", "hyphens", "image-orientation", "image-rendering",
      "inline-size", "inset", "inset-block", "inset-block-end", "inset-block-start",
      "inset-inline", "inset-inline-end", "inset-inline-start", "isolation", "justify-content",
      "justify-items", "justify-self", "left", "letter-spacing", "line-break", "line-height",
      "list-style", "list-style-image", "list-style-position", "list-style-type", "margin",
      "margin-block", "margin-block-end", "margin-block-start", "margin-bottom",
      "margin-inline", "margin-inline-end", "margin-inline-start", "margin-left",
      "margin-right", "margin-top", "mask", "mask-clip", "mask-composite", "mask-image",
      "mask-mode", "mask-origin", "mask-position", "mask-repeat", "mask-size", "mask-type",
      "max-block-size", "max-height", "max-inline-size", "max-width", "min-block-size",
      "min-height", "min-inline-size", "min-width", "mix-blend-mode", "object-fit",
      "object-position", "offset", "offset-anchor", "offset-distance", "offset-path",
      "offset-position", "offset-rotate", "opacity", "order", "orphans", "outline",
      "outline-color", "outline-offset", "outline-style", "outline-width", "overflow",
      "overflow-anchor", "overflow-block", "overflow-clip-margin", "overflow-inline",
      "overflow-wrap", "overflow-x", "overflow-y", "overscroll-behavior",
      "overscroll-behavior-block", "overscroll-behavior-inline", "overscroll-behavior-x",
      "overscroll-behavior-y", "override-colors", "padding", "padding-block",
      "padding-block-end", "padding-block-start", "padding-bottom", "padding-inline",
      "padding-inline-end", "padding-inline-start", "padding-left", "padding-right",
      "padding-top", "page", "page-break-after", "page-break-before", "page-break-inside",
      "paint-order", "perspective", "perspective-origin", "place-content", "place-items",
      "place-self", "pointer-events", "position", "quotes", "resize", "right", "rotate",
      "row-gap", "scale", "scroll-behavior", "scroll-margin", "scroll-margin-block",
      "scroll-margin-block-end", "scroll-margin-block-start", "scroll-margin-bottom",
      "scroll-margin-inline", "scroll-margin-inline-end", "scroll-margin-inline-start",
      "scroll-margin-left", "scroll-margin-right", "scroll-margin-top", "scroll-padding",
      "scroll-padding-block", "scroll-padding-block-end", "scroll-padding-block-start",
      "scroll-padding-bottom", "scroll-padding-inline", "scroll-padding-inline-end",
      "scroll-padding-inline-start", "scroll-padding-left", "scroll-padding-right",
      "scroll-padding-top", "scroll-snap-align", "scroll-snap-stop", "scroll-snap-type",
      "scrollbar-color", "scrollbar-gutter", "scrollbar-width", "shape-image-threshold",
      "shape-margin", "shape-outside", "size", "src", "tab-size", "table-layout",
      "text-align", "text-align-last", "text-combine-upright", "text-decoration",
      "text-decoration-color", "text-decoration-line", "text-decoration-skip-ink",
      "text-decoration-style", "text-decoration-thickness", "text-emphasis",
      "text-emphasis-color", "text-emphasis-position", "text-emphasis-style", "text-indent",
      "text-justify", "text-orientation", "text-overflow", "text-rendering", "text-shadow",
      "text-transform", "text-underline-offset", "text-underline-position", "top",
      "touch-action", "transform", "transform-box", "transform-origin", "transform-style",
      "transition", "transition-delay", "transition-duration", "transition-property",
      "transition-timing-function", "translate", "unicode-bidi", "unicode-range",
      "user-select", "vertical-align", "visibility", "white-space", "widows", "width",
      "will-change", "word-break", "word-spacing", "word-wrap", "writing-mode", "z-index",
      // Vendor prefixed
      "-webkit-transform", "-webkit-transform-origin", "-webkit-transition", "-webkit-animation",
      "-webkit-user-select", "-webkit-box-sizing", "-webkit-appearance", "-webkit-line-clamp",
      "-webkit-box-orient", "-webkit-box-direction", "-webkit-background-clip",
      "-moz-animation", "-moz-transform", "-moz-user-select", "-moz-box-sizing", "-moz-appearance"
    ]);

    const SHORTHAND_LONGHANDS = {
      margin: ["margin-top", "margin-right", "margin-bottom", "margin-left"],
      padding: ["padding-top", "padding-right", "padding-bottom", "padding-left"],
      "border-width": ["border-top-width", "border-right-width", "border-bottom-width", "border-left-width"],
      "border-style": ["border-top-style", "border-right-style", "border-bottom-style", "border-left-style"],
      "border-color": ["border-top-color", "border-right-color", "border-bottom-color", "border-left-color"],
      "border-radius": ["border-top-left-radius", "border-top-right-radius", "border-bottom-right-radius", "border-bottom-left-radius"],
      overflow: ["overflow-x", "overflow-y"]
    };

    function cssPropCamelToKebab(s) {
      const str = String(s);
      if (!str || str.startsWith("--")) return str;
      if (str.startsWith("-")) return str.toLowerCase();
      let body = str;
      let prefix = "";
      if (str.startsWith("webkit")) {
        prefix = "-webkit-";
        body = str.slice(6);
      } else if (str.startsWith("Moz")) {
        prefix = "-moz-";
        body = str.slice(3);
      }
      if (body.length > 0) {
        body = body[0].toLowerCase() + body.slice(1);
      }
      return prefix + body.replace(/[A-Z]/g, (c) => "-" + c.toLowerCase());
    }

    function isCssPropertySupported(prop) {
      if (typeof prop !== "string") return false;
      const p = prop.trim();
      if (!p) return false;
      if (p.startsWith("--")) return true;
      if (STANDARD_CSS_PROPERTIES.has(p)) return true;
      const kebab = cssPropCamelToKebab(p);
      return STANDARD_CSS_PROPERTIES.has(kebab);
    }
    global.__isCssPropertySupported = isCssPropertySupported;
    if (typeof globalThis !== "undefined") globalThis.__isCssPropertySupported = isCssPropertySupported;

    function isValidColor(val) {
      const v = val.trim().toLowerCase();
      if (!v) return false;
      if (v === "bogus") return false;
      if (v === "currentcolor" || v === "transparent") return true;
      if (/^#[0-9a-f]{3,8}$/i.test(v)) return true;
      if (/^(?:rgb|rgba|hsl|hsla|hwb|lab|lch|oklab|oklch|color)\(/i.test(v)) return true;
      const NAMED_COLORS = new Set([
        "black", "silver", "gray", "white", "maroon", "red", "purple", "fuchsia",
        "green", "lime", "olive", "yellow", "navy", "blue", "teal", "aqua",
        "orange", "aliceblue", "antiquewhite", "aquamarine", "azure", "beige",
        "bisque", "blanchedalmond", "blueviolet", "brown", "burlywood", "cadetblue",
        "chartreuse", "chocolate", "coral", "cornflowerblue", "cornsilk", "crimson",
        "cyan", "darkblue", "darkcyan", "darkgoldenrod", "darkgray", "darkgreen",
        "darkgrey", "darkkhaki", "darkmagenta", "darkolivegreen", "darkorange",
        "darkorchid", "darkred", "darksalmon", "darkseagreen", "darkslateblue",
        "darkslategray", "darkslategrey", "darkturquoise", "darkviolet", "deeppink",
        "deepskyblue", "dimgray", "dimgrey", "dodgerblue", "firebrick", "floralwhite",
        "forestgreen", "gainsboro", "ghostwhite", "gold", "goldenrod", "greenyellow",
        "grey", "honeydew", "hotpink", "indianred", "indigo", "ivory", "khaki",
        "lavender", "lavenderblush", "lawngreen", "lemonchiffon", "lightblue",
        "lightcoral", "lightcyan", "lightgoldenrodyellow", "lightgray", "lightgreen",
        "lightgrey", "lightpink", "lightsalmon", "lightseagreen", "lightskyblue",
        "lightslategray", "lightslategrey", "lightsteelblue", "lightyellow", "limegreen",
        "linen", "magenta", "mediumaquamarine", "mediumblue", "mediumorchid",
        "mediumpurple", "mediumseagreen", "mediumslateblue", "mediumspringgreen",
        "mediumturquoise", "mediumvioletred", "midnightblue", "mintcream", "mistyrose",
        "moccasin", "navajowhite", "oldlace", "olivedrab", "orangered", "orchid",
        "palegoldenrod", "palegreen", "paleturquoise", "palevioletred", "papayawhip",
        "peachpuff", "peru", "pink", "plum", "powderblue", "rosybrown", "royalblue",
        "saddlebrown", "salmon", "sandybrown", "seagreen", "seashell", "sienna",
        "skyblue", "slateblue", "slategray", "slategrey", "snow", "springgreen",
        "steelblue", "tan", "thistle", "tomato", "turquoise", "violet", "wheat",
        "whitesmoke", "yellowgreen"
      ]);
      return NAMED_COLORS.has(v);
    }

    function isValidCssValue(prop, value) {
      if (typeof value !== "string") value = String(value);
      const trimmed = value.trim();
      if (!trimmed) return false;
      if (prop.startsWith("--")) return true;

      const vLower = trimmed.toLowerCase();
      if (vLower === "inherit" || vLower === "initial" || vLower === "unset" || vLower === "revert" || vLower === "revert-layer") {
        return true;
      }
      if (vLower === "bogus") return false;

      if (prop === "display") {
        const validDisplays = new Set([
          "none", "inline", "block", "inline-block", "flex", "inline-flex",
          "grid", "inline-grid", "flow-root", "contents", "table", "inline-table",
          "table-row", "table-row-group", "table-column", "table-column-group",
          "table-cell", "table-caption", "list-item"
        ]);
        return validDisplays.has(vLower);
      }

      if (prop === "color" || prop === "background-color" || prop.endsWith("-color")) {
        return isValidColor(trimmed);
      }

      if (prop === "font-display") {
        return ["auto", "block", "swap", "fallback", "optional"].includes(vLower);
      }

      if (/^(?:url|var|calc|env|attr|min|max|clamp)\(/i.test(trimmed)) {
        return true;
      }

      return true;
    }

    function declSupported(name, value) {
      if (!name || typeof name !== "string") return false;
      if (name.startsWith("--")) return typeof value === "string" && value.trim() !== "";
      const kebab = cssPropCamelToKebab(name);
      if (!isCssPropertySupported(kebab)) return false;
      return isValidCssValue(kebab, value);
    }

    function selectorSupported(selector) {
      if (typeof selector !== "string") return false;
      const s = selector.trim();
      if (!s || s === "!!" || /^[!@#$%^&;]+$/.test(s) || /^[+>~]$/.test(s)) return false;
      if (s.includes("::")) {
        const m = s.match(/::([a-zA-Z-]+)(.*)$/);
        if (m && m[2].trim().length > 0) return false;
      }
      return true;
    }

    function matchParen(text, openParenIndex) {
      let depth = 0;
      let quote = null;
      for (let index = openParenIndex; index < text.length; index++) {
        const character = text[index];
        if (quote) {
          if (character === "\\") index++;
          else if (character === quote) quote = null;
          continue;
        }
        if (character === '"' || character === "'") quote = character;
        else if (character === "(") depth++;
        else if (character === ")") {
          depth--;
          if (depth === 0) return index + 1;
        }
      }
      return -1;
    }

    function evalCondition(condition) {
      let cursor = 0;
      const skipWhitespace = () => {
        while (cursor < condition.length && /\s/.test(condition[cursor])) cursor++;
      };

      const parseInParens = () => {
        skipWhitespace();
        const remainingCondition = condition.slice(cursor);
        const functionMatch = /^([a-zA-Z][a-zA-Z0-9-]*)\(/.exec(remainingCondition);
        if (functionMatch) {
          const openParenIndex = cursor + functionMatch[1].length;
          const endIndex = matchParen(condition, openParenIndex);
          if (endIndex < 0) return null;
          const innerCondition = condition.slice(openParenIndex + 1, endIndex - 1);
          cursor = endIndex;
          if (functionMatch[1].toLowerCase() === "selector") {
            return selectorSupported(innerCondition);
          }
          return "unknown";
        }
        if (condition[cursor] !== "(") return null;
        const endIndex = matchParen(condition, cursor);
        if (endIndex < 0) return null;
        const innerCondition = condition.slice(cursor + 1, endIndex - 1);
        cursor = endIndex;
        const nested = evalCondition(innerCondition);
        if (nested !== null) return nested;
        const colon = innerCondition.indexOf(":");
        if (colon > 0) {
          const name = innerCondition.slice(0, colon).trim();
          if (/^-{0,2}[a-zA-Z_][a-zA-Z0-9_-]*$/.test(name)) {
            return declSupported(name, innerCondition.slice(colon + 1).trim());
          }
        }
        return "unknown";
      };

      skipWhitespace();
      if (/^not(?![a-zA-Z0-9_-])/i.test(condition.slice(cursor))) {
        cursor += 3;
        const conditionValue = parseInParens();
        if (conditionValue === null) return null;
        skipWhitespace();
        if (cursor < condition.length) return null;
        return conditionValue === "unknown" ? "unknown" : !conditionValue;
      }

      let result = parseInParens();
      if (result === null) return null;
      skipWhitespace();
      let combinator = null;
      while (cursor < condition.length) {
        const keywordMatch = /^(and|or)(?![a-zA-Z0-9_-])/i.exec(condition.slice(cursor));
        if (!keywordMatch) return null;
        const keyword = keywordMatch[1].toLowerCase();
        if (combinator && keyword !== combinator) return null;
        combinator = keyword;
        cursor += keyword.length;
        const conditionValue = parseInParens();
        if (conditionValue === null) return null;
        if (keyword === "and") {
          if (result === false || conditionValue === false) result = false;
          else if (result === "unknown" || conditionValue === "unknown") result = "unknown";
        } else {
          if (result === true || conditionValue === true) result = true;
          else if (result === "unknown" || conditionValue === "unknown") result = "unknown";
        }
        skipWhitespace();
      }
      return result;
    }

    function serializeCssValue(val) {
      return String(val).replace(/url\(\s*(?!"|')([^)]+?)\s*\)/g, 'url("$1")');
    }

    function splitCssDeclarations(block) {
      const parts = [];
      let depth = 0;
      let inQuote = null;
      let start = 0;
      for (let i = 0; i < block.length; i++) {
        const c = block[i];
        if (inQuote) {
          if (c === inQuote && block[i - 1] !== "\\") {
            inQuote = null;
          }
        } else if (c === '"' || c === "'") {
          inQuote = c;
        } else if (c === "(") {
          depth++;
        } else if (c === ")") {
          depth = Math.max(0, depth - 1);
        } else if (c === ";" && depth === 0) {
          parts.push(block.slice(start, i));
          start = i + 1;
        }
      }
      if (start < block.length) {
        parts.push(block.slice(start));
      }
      return parts;
    }

    // =========================================================================
    // 2. CSS Namespace & CSS.supports
    // =========================================================================

    const CSS = {
      escape(ident) {
        return String(ident).replace(/[^a-zA-Z0-9_-]/g, (c) => "\\" + c);
      },
      supports(propertyOrCondition, value) {
        if (typeof propertyOrCondition === "string" && typeof value === "string") {
          return declSupported(propertyOrCondition, value);
        }
        if (typeof propertyOrCondition === "string") {
          const conditionResult = evalCondition(propertyOrCondition);
          if (conditionResult !== null) return conditionResult === true;
          const colon = propertyOrCondition.indexOf(":");
          if (colon > 0) {
            return declSupported(
              propertyOrCondition.slice(0, colon).trim(),
              propertyOrCondition.slice(colon + 1).trim()
            );
          }
        }
        return false;
      }
    };

    const CSS_UNITS = [
      "number", "percent", "px", "cm", "mm", "Q", "in", "pt", "pc",
      "em", "ex", "ch", "cap", "ic", "rem", "lh", "rlh", "rcap", "rch",
      "rex", "ric", "vw", "vh", "vi", "vb", "vmin", "vmax", "svw", "svh",
      "svi", "svb", "svmin", "svmax", "lvw", "lvh", "lvi", "lvb", "lvmin",
      "lvmax", "dvw", "dvh", "dvi", "dvb", "dvmin", "dvmax", "cqw", "cqh",
      "cqi", "cqb", "cqmin", "cqmax", "deg", "grad", "rad", "turn", "s",
      "ms", "Hz", "kHz", "dpi", "dpcm", "dppx", "fr"
    ];
    for (let i = 0; i < CSS_UNITS.length; i++) {
      const unit = CSS_UNITS[i];
      CSS[unit] = function (val) {
        return { value: Number(val), unit: unit };
      };
    }

    global.CSS = CSS;
    if (typeof globalThis !== "undefined") globalThis.CSS = CSS;
    if (typeof window !== "undefined") window.CSS = CSS;

    // =========================================================================
    // 3. MediaList
    // =========================================================================

    function MediaList(initialText, onMutate) {
      this._media = [];
      this._onMutate = onMutate || null;
      if (initialText) this._setText(initialText);
    }
    MediaList.prototype._setText = function (text) {
      const old = this._media.length;
      this._media = String(text == null ? "" : text)
        .split(",")
        .map((m) => m.trim())
        .filter(Boolean);
      for (let i = this._media.length; i < old; i++) delete this[i];
      for (let i = 0; i < this._media.length; i++) this[i] = this._media[i];
    };
    MediaList.prototype._notify = function () {
      if (this._onMutate) this._onMutate();
    };
    Object.defineProperty(MediaList.prototype, "length", {
      get: function () { return this._media.length; },
      configurable: true,
    });
    Object.defineProperty(MediaList.prototype, "mediaText", {
      get: function () { return this._media.join(", "); },
      set: function (v) { this._setText(v); this._notify(); },
      configurable: true,
    });
    MediaList.prototype.item = function (index) {
      const m = this._media[Number(index) | 0];
      return m != null ? m : null;
    };
    MediaList.prototype.appendMedium = function (medium) {
      const m = String(medium).trim();
      if (m && !this._media.includes(m)) {
        this._media.push(m);
        this[this._media.length - 1] = m;
        this._notify();
      }
    };
    MediaList.prototype.deleteMedium = function (medium) {
      const idx = this._media.indexOf(String(medium).trim());
      if (idx >= 0) {
        const old = this._media.length;
        this._media.splice(idx, 1);
        for (let i = this._media.length; i < old; i++) delete this[i];
        for (let i = 0; i < this._media.length; i++) this[i] = this._media[i];
        this._notify();
      }
    };
    MediaList.prototype.toString = function () { return this.mediaText; };
    MediaList.prototype[Symbol.iterator] = Array.prototype[Symbol.iterator];
    markNative(MediaList, "MediaList");
    global.MediaList = MediaList;

    // =========================================================================
    // 4. CSSStyleDeclaration
    // =========================================================================

    function CSSStyleDeclaration() {}
    CSSStyleDeclaration.prototype.item = function (index) { return ""; };
    CSSStyleDeclaration.prototype.getPropertyValue = function (property) { return ""; };
    CSSStyleDeclaration.prototype.getPropertyPriority = function (property) { return ""; };
    CSSStyleDeclaration.prototype.setProperty = function (property, value, priority) {};
    CSSStyleDeclaration.prototype.removeProperty = function (property) { return ""; };
    Object.defineProperty(CSSStyleDeclaration.prototype, "cssText", {
      get: function () { return this._cssText || ""; },
      set: function (v) { this._cssText = String(v == null ? "" : v); },
      configurable: true,
    });
    Object.defineProperty(CSSStyleDeclaration.prototype, "length", {
      get: function () { return this._length != null ? this._length : 0; },
      set: function (v) { this._length = Number(v) >>> 0; },
      configurable: true,
    });
    Object.defineProperty(CSSStyleDeclaration.prototype, "parentRule", {
      get: function () { return this._parentRule || null; },
      set: function (v) { this._parentRule = v; },
      configurable: true,
    });
    CSSStyleDeclaration.prototype[Symbol.iterator] = Array.prototype[Symbol.iterator];
    markNative(CSSStyleDeclaration, "CSSStyleDeclaration");

    let _CSSStyleDeclaration = CSSStyleDeclaration;

    Object.defineProperty(CSSStyleDeclaration, Symbol.hasInstance, {
      value: function (inst) {
        if (!inst || typeof inst !== "object") return false;
        return Object.prototype.isPrototypeOf.call(CSSStyleDeclaration.prototype, inst) ||
               (inst._isCSSStyleDeclaration === true);
      },
      configurable: true,
    });

    function installCSSStyleDeclarationProperty(targetObj) {
      if (!targetObj) return;
      Object.defineProperty(targetObj, "CSSStyleDeclaration", {
        get: function () { return _CSSStyleDeclaration; },
        set: function (newClass) {
          if (newClass && newClass.prototype && newClass !== _CSSStyleDeclaration) {
            Object.setPrototypeOf(newClass.prototype, _CSSStyleDeclaration.prototype);
          }
        },
        configurable: true,
        enumerable: true,
      });
    }

    installCSSStyleDeclarationProperty(global);
    if (typeof globalThis !== "undefined") installCSSStyleDeclarationProperty(globalThis);
    if (typeof window !== "undefined") installCSSStyleDeclarationProperty(window);

    function makeRuleStyleDeclaration(initialDeclText, context, baseUrl, onMutate) {
      if (!context) context = "style";
      const data = new Map(); // key -> { name, value, priority }
      let authoredOrder = []; // array of keys

      function parseInitial(text) {
        if (!text) return;
        const decls = splitCssDeclarations(text);
        for (let i = 0; i < decls.length; i++) {
          const d = decls[i].trim();
          if (!d) continue;
          const colon = d.indexOf(":");
          if (colon <= 0) continue;
          const rawName = d.slice(0, colon).trim();
          let val = d.slice(colon + 1).trim();
          let priority = "";
          if (/!important\s*$/i.test(val)) {
            priority = "important";
            val = val.replace(/\s*!important\s*$/i, "").trim();
          }
          if (context === "keyframe" && priority === "important") continue;
          if (context === "font-face" && rawName === "font-display") {
            if (!["auto", "block", "swap", "fallback", "optional"].includes(val.toLowerCase())) continue;
          }
          if (context === "page" && rawName.toLowerCase() === "size") {
            val = val.toLowerCase();
          }
          if (context === "margin" && rawName.toLowerCase() === "size") {
            continue;
          }
          const key = rawName.startsWith("--") ? rawName : rawName.toLowerCase();
          if (!data.has(key)) {
            authoredOrder.push(key);
          }
          data.set(key, { name: rawName, value: val, priority: priority });
        }
      }

      parseInitial(initialDeclText);

      function notify() {
        if (onMutate) onMutate();
      }

      const decl = Object.create(CSSStyleDeclaration.prototype);
      decl._isCSSStyleDeclaration = true;

      Object.defineProperty(decl, "length", {
        get: function () { return data.size; },
        configurable: true,
      });

      Object.defineProperty(decl, "parentRule", {
        get: function () { return decl._parentRule || null; },
        configurable: true,
      });

      decl.item = function (index) {
        const idx = Number(index) >>> 0;
        let keys = authoredOrder;
        if (context === "font-face") {
          keys = Array.from(data.keys()).sort();
        }
        const k = keys[idx];
        return k != null ? (data.get(k)?.name || k) : "";
      };

      decl.getPropertyValue = function (prop) {
        const p = String(prop).trim();
        const key = p.startsWith("--") ? p : p.toLowerCase();
        if (context === "margin" && key === "size") return "";
        if (data.has(key)) {
          return serializeCssValue(data.get(key).value);
        }
        if (key === "margin-top" || key === "margin-right" || key === "margin-bottom" || key === "margin-left") {
          if (data.has("margin")) {
            const parts = data.get("margin").value.trim().split(/\s+/);
            if (parts.length === 1) return serializeCssValue(parts[0]);
            if (parts.length === 2) {
              if (key === "margin-top" || key === "margin-bottom") return serializeCssValue(parts[0]);
              return serializeCssValue(parts[1]);
            }
            if (parts.length === 3) {
              if (key === "margin-top") return serializeCssValue(parts[0]);
              if (key === "margin-bottom") return serializeCssValue(parts[2]);
              return serializeCssValue(parts[1]);
            }
            if (parts.length === 4) {
              if (key === "margin-top") return serializeCssValue(parts[0]);
              if (key === "margin-right") return serializeCssValue(parts[1]);
              if (key === "margin-bottom") return serializeCssValue(parts[2]);
              if (key === "margin-left") return serializeCssValue(parts[3]);
            }
          }
        }
        if (key === "padding-top" || key === "padding-right" || key === "padding-bottom" || key === "padding-left") {
          if (data.has("padding")) {
            const parts = data.get("padding").value.trim().split(/\s+/);
            if (parts.length === 1) return serializeCssValue(parts[0]);
            if (parts.length === 2) {
              if (key === "padding-top" || key === "padding-bottom") return serializeCssValue(parts[0]);
              return serializeCssValue(parts[1]);
            }
            if (parts.length === 3) {
              if (key === "padding-top") return serializeCssValue(parts[0]);
              if (key === "padding-bottom") return serializeCssValue(parts[2]);
              return serializeCssValue(parts[1]);
            }
            if (parts.length === 4) {
              if (key === "padding-top") return serializeCssValue(parts[0]);
              if (key === "padding-right") return serializeCssValue(parts[1]);
              if (key === "padding-bottom") return serializeCssValue(parts[2]);
              if (key === "padding-left") return serializeCssValue(parts[3]);
            }
          }
        }
        if (SHORTHAND_LONGHANDS[key]) {
          const longhands = SHORTHAND_LONGHANDS[key];
          let pri = null;
          let vals = [];
          for (let i = 0; i < longhands.length; i++) {
            const lhKey = longhands[i];
            if (!data.has(lhKey)) return "";
            const entry = data.get(lhKey);
            if (pri === null) pri = entry.priority;
            else if (pri !== entry.priority) return "";
            vals.push(serializeCssValue(entry.value));
          }
          if (vals.length === 4) {
            if (vals[0] === vals[1] && vals[1] === vals[2] && vals[2] === vals[3]) return vals[0];
            if (vals[0] === vals[2] && vals[1] === vals[3]) return vals[0] + " " + vals[1];
            if (vals[1] === vals[3]) return vals[0] + " " + vals[1] + " " + vals[2];
            return vals.join(" ");
          }
        }
        return "";
      };

      decl.getPropertyPriority = function (prop) {
        const p = String(prop).trim();
        const key = p.startsWith("--") ? p : p.toLowerCase();
        const entry = data.get(key);
        return entry ? entry.priority : "";
      };

      decl.setProperty = function (prop, val, priority) {
        const p = String(prop).trim();
        if (!p) return;
        if (priority !== undefined && priority !== null && priority !== "") {
          if (priority !== "important" && priority !== "!important") {
            return;
          }
        }
        if (context === "keyframe" && (priority === "important" || priority === "!important")) {
          return;
        }
        const pPriority = (priority === "important" || priority === "!important") ? "important" : "";
        let v = val == null ? "" : String(val).trim();
        if (v === "") {
          decl.removeProperty(p);
          return;
        }
        const key = p.startsWith("--") ? p : p.toLowerCase();
        if (!p.startsWith("--")) {
          if (!isValidCssValue(key, v)) return;
        }
        if (context === "page" && key === "size") {
          v = v.toLowerCase();
        }
        if (context === "margin" && key === "size") {
          return;
        }

        if (SHORTHAND_LONGHANDS[key]) {
          const longhands = SHORTHAND_LONGHANDS[key];
          for (let i = 0; i < longhands.length; i++) {
            const lhKey = longhands[i];
            data.set(lhKey, { name: lhKey, value: v, priority: pPriority });
            if (!authoredOrder.includes(lhKey)) authoredOrder.push(lhKey);
          }
        }

        if (!data.has(key)) {
          authoredOrder.push(key);
        }
        data.set(key, { name: p, value: v, priority: pPriority });
        notify();
      };

      decl.removeProperty = function (prop) {
        const p = String(prop).trim();
        const key = p.startsWith("--") ? p : p.toLowerCase();
        let prev = "";
        if (data.has(key)) {
          prev = data.get(key).value;
          data.delete(key);
          authoredOrder = authoredOrder.filter((k) => k !== key);
        }
        if (SHORTHAND_LONGHANDS[key]) {
          const longhands = SHORTHAND_LONGHANDS[key];
          for (let i = 0; i < longhands.length; i++) {
            const lhKey = longhands[i];
            if (data.has(lhKey)) {
              if (!prev) prev = data.get(lhKey).value;
              data.delete(lhKey);
              authoredOrder = authoredOrder.filter((k) => k !== lhKey);
            }
          }
        }
        notify();
        return prev;
      };

      decl[Symbol.iterator] = function* () {
        let keys = authoredOrder;
        if (context === "font-face") {
          keys = Array.from(data.keys()).sort();
        }
        for (let i = 0; i < keys.length; i++) {
          const k = keys[i];
          yield data.get(k)?.name || k;
        }
      };

      function serializeDecls() {
        const parts = [];
        let keys = authoredOrder;
        if (context === "font-face") {
          keys = Array.from(data.keys()).sort();
        }
        for (let i = 0; i < keys.length; i++) {
          const k = keys[i];
          const entry = data.get(k);
          if (!entry) continue;
          let v = serializeCssValue(entry.value);
          let name = entry.name;
          if (!name.startsWith("--")) name = name.toLowerCase();
          let s = name + ": " + v;
          if (entry.priority) s += " !" + entry.priority;
          parts.push(s);
        }
        if (context === "font-face") {
          return parts.length ? parts.join("; ") + ";" : "";
        }
        return parts.join("; ");
      }

      Object.defineProperty(decl, "cssText", {
        get: function () { return serializeDecls(); },
        set: function (v) {
          data.clear();
          authoredOrder = [];
          parseInitial(v);
          notify();
        },
        configurable: true,
      });

      return new Proxy(decl, {
        get: function (target, prop, receiver) {
          if (typeof prop === "symbol") return target[prop];
          if (typeof prop === "string" && prop.startsWith("_")) return target[prop];
          if (prop in target) return target[prop];
          if (typeof prop === "string") {
            const kebab = cssPropCamelToKebab(prop);
            let val = decl.getPropertyValue(kebab);
            if (!val && prop.startsWith("--")) val = decl.getPropertyValue(prop);
            if (context === "page" && kebab === "size" && val) return val.toLowerCase();
            return val;
          }
          return undefined;
        },
        set: function (target, prop, value) {
          if (typeof prop === "symbol") { target[prop] = value; return true; }
          if (typeof prop === "string" && prop.startsWith("_")) { target[prop] = value; return true; }
          if (prop === "cssText") { target.cssText = value; return true; }
          const kebab = cssPropCamelToKebab(prop);
          decl.setProperty(kebab, value);
          return true;
        },
        has: function (target, prop) {
          if (typeof prop === "symbol") return prop in target;
          if (typeof prop === "string" && prop.startsWith("_")) return prop in target;
          if (prop in target) return true;
          const pStr = String(prop);
          if (pStr.startsWith("--")) return false;
          if (/^(?:webkit|moz|ms|o)-/.test(pStr)) return false;
          if (!pStr.includes("-") && /^(?:moz|ms|o)[A-Z]/.test(pStr)) return false;
          return isCssPropertySupported(pStr);
        },
        ownKeys: function () {
          let keys = authoredOrder;
          if (context === "font-face") keys = Array.from(data.keys()).sort();
          return keys.slice();
        },
        getOwnPropertyDescriptor: function (target, prop) {
          if (typeof prop === "string" && data.has(prop)) {
            return { value: data.get(prop).value, writable: true, enumerable: true, configurable: true };
          }
          return Reflect.getOwnPropertyDescriptor(target, prop);
        }
      });
    }

    // =========================================================================
    // 5. CSSRule and Subclasses
    // =========================================================================

    const RULE_CONSTANTS = {
      STYLE_RULE: 1,
      CHARSET_RULE: 2,
      IMPORT_RULE: 3,
      MEDIA_RULE: 4,
      FONT_FACE_RULE: 5,
      PAGE_RULE: 6,
      KEYFRAMES_RULE: 7,
      KEYFRAME_RULE: 8,
      MARGIN_RULE: 9,
      NAMESPACE_RULE: 10,
      COUNTER_STYLE_RULE: 11,
      SUPPORTS_RULE: 12,
      FONT_FEATURE_VALUES_RULE: 14,
    };

    function CSSRule() {
      this.parentRule = null;
      this.parentStyleSheet = null;
    }
    for (const [k, v] of Object.entries(RULE_CONSTANTS)) {
      Object.defineProperty(CSSRule, k, { value: v, writable: false, enumerable: true, configurable: false });
      Object.defineProperty(CSSRule.prototype, k, { value: v, writable: false, enumerable: true, configurable: false });
    }
    Object.defineProperty(CSSRule.prototype, "type", {
      get: function () { return this._type || 0; },
      configurable: true,
    });
    Object.defineProperty(CSSRule.prototype, "cssText", {
      get: function () { return this._serialize ? this._serialize() : ""; },
      set: function () {},
      configurable: true,
    });
    markNative(CSSRule, "CSSRule");
    global.CSSRule = CSSRule;

    function makeRuleList(arr) {
      Object.defineProperty(arr, "item", {
        value: function (index) {
          const i = Number(index) >>> 0;
          return arr[i] || null;
        },
        configurable: true,
        enumerable: false,
        writable: true,
      });
      return arr;
    }

    // 5.1 CSSGroupingRule
    function CSSGroupingRule() {
      CSSRule.call(this);
      this.cssRules = makeRuleList([]);
    }
    CSSGroupingRule.prototype = Object.create(CSSRule.prototype);
    CSSGroupingRule.prototype.constructor = CSSGroupingRule;
    CSSGroupingRule.prototype.insertRule = function (ruleText, index) {
      if (arguments.length === 0) {
        throw new TypeError("Failed to execute 'insertRule': 1 argument required, but only 0 present.");
      }
      const idx = index === undefined ? 0 : Number(index) >>> 0;
      if (idx > this.cssRules.length) {
        throw new DOMException("Index out of bounds", "IndexSizeError");
      }
      const parsed = parseSingleRuleOrThrow(ruleText);
      if (parsed.kind === "import" || parsed.kind === "namespace") {
        throw new DOMException("Rule not allowed inside a grouping rule", "HierarchyRequestError");
      }
      const rule = constructRule(parsed, this.parentStyleSheet, this, () => {
        if (this._onMutate) this._onMutate();
      });
      this.cssRules.splice(idx, 0, rule);
      if (this._onMutate) this._onMutate();
      return idx;
    };
    CSSGroupingRule.prototype.deleteRule = function (index) {
      if (arguments.length === 0) {
        throw new TypeError("Failed to execute 'deleteRule': 1 argument required, but only 0 present.");
      }
      const idx = Number(index) >>> 0;
      if (idx >= this.cssRules.length) {
        throw new DOMException("Index out of bounds", "IndexSizeError");
      }
      this.cssRules.splice(idx, 1);
      if (this._onMutate) this._onMutate();
    };
    markNative(CSSGroupingRule, "CSSGroupingRule");
    global.CSSGroupingRule = CSSGroupingRule;

    // 5.2 CSSConditionRule
    function CSSConditionRule() {
      CSSGroupingRule.call(this);
    }
    CSSConditionRule.prototype = Object.create(CSSGroupingRule.prototype);
    CSSConditionRule.prototype.constructor = CSSConditionRule;
    Object.defineProperty(CSSConditionRule.prototype, "conditionText", {
      get: function () { return this._conditionText || ""; },
      configurable: true,
    });
    markNative(CSSConditionRule, "CSSConditionRule");
    global.CSSConditionRule = CSSConditionRule;

    // 5.3 CSSMediaRule
    function CSSMediaRule() {
      CSSConditionRule.call(this);
      this._type = 4;
    }
    CSSMediaRule.prototype = Object.create(CSSConditionRule.prototype);
    CSSMediaRule.prototype.constructor = CSSMediaRule;
    Object.defineProperty(CSSMediaRule.prototype, "conditionText", {
      get: function () { return this.media ? this.media.mediaText : (this._conditionText || ""); },
      configurable: true,
    });
    markNative(CSSMediaRule, "CSSMediaRule");
    global.CSSMediaRule = CSSMediaRule;

    // 5.4 CSSSupportsRule
    function CSSSupportsRule() {
      CSSConditionRule.call(this);
      this._type = 12;
    }
    CSSSupportsRule.prototype = Object.create(CSSConditionRule.prototype);
    CSSSupportsRule.prototype.constructor = CSSSupportsRule;
    markNative(CSSSupportsRule, "CSSSupportsRule");
    global.CSSSupportsRule = CSSSupportsRule;

    // 5.5 CSSStyleRule
    function CSSStyleRule() {
      CSSGroupingRule.call(this);
      this._type = 1;
    }
    CSSStyleRule.prototype = Object.create(CSSGroupingRule.prototype);
    CSSStyleRule.prototype.constructor = CSSStyleRule;
    markNative(CSSStyleRule, "CSSStyleRule");
    global.CSSStyleRule = CSSStyleRule;

    // 5.6 CSSImportRule
    function CSSImportRule() {
      CSSRule.call(this);
      this._type = 3;
      this.href = "";
      this.layerName = null;
      this.supportsText = null;
      this.media = new MediaList("");
      this.styleSheet = null;
    }
    CSSImportRule.prototype = Object.create(CSSRule.prototype);
    CSSImportRule.prototype.constructor = CSSImportRule;
    markNative(CSSImportRule, "CSSImportRule");
    global.CSSImportRule = CSSImportRule;

    // 5.7 CSSFontPaletteValuesRule
    function CSSFontPaletteValuesRule() {
      CSSRule.call(this);
      this._type = 0;
      this.name = "";
      this.fontFamily = "";
      this.basePalette = "";
      this.overrideColors = "";
    }
    CSSFontPaletteValuesRule.prototype = Object.create(CSSRule.prototype);
    CSSFontPaletteValuesRule.prototype.constructor = CSSFontPaletteValuesRule;
    markNative(CSSFontPaletteValuesRule, "CSSFontPaletteValuesRule");
    global.CSSFontPaletteValuesRule = CSSFontPaletteValuesRule;

    const MARGIN_BOX_NAMES = new Set([
      "top-left-corner", "top-left", "top-center", "top-right", "top-right-corner",
      "bottom-left-corner", "bottom-left", "bottom-center", "bottom-right", "bottom-right-corner",
      "left-top", "left-middle", "left-bottom", "right-top", "right-middle", "right-bottom"
    ]);

    function atKeywordNameOf(text) {
      const s = String(text || "").trim();
      if (s[0] !== "@") return null;
      let name = "";
      for (let i = 1; i < s.length;) {
        const ch = s[i];
        if (/[a-zA-Z0-9_-]/.test(ch)) {
          name += ch;
          i++;
          continue;
        }
        if (ch !== "\\" || i + 1 >= s.length || /[\n\r\f]/.test(s[i + 1])) return null;
        i++;
        const hex = s.slice(i).match(/^[0-9a-fA-F]{1,6}/)?.[0];
        if (hex) {
          const cp = Number.parseInt(hex, 16);
          name += (cp === 0 || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
            ? "\ufffd"
            : String.fromCodePoint(cp);
          i += hex.length;
          if (/[\t\n\f\r ]/.test(s[i] || "")) {
            i += s[i] === "\r" && s[i + 1] === "\n" ? 2 : 1;
          }
        } else {
          name += s[i++];
        }
      }
      return name ? name.toLowerCase() : null;
    }

    function canonicalizePageSelectorList(str) {
      if (str == null) return null;
      const s = String(str).trim();
      if (s === "") return "";

      const parts = [];
      let cur = "";
      for (let i = 0; i < s.length; i++) {
        const ch = s[i];
        if (ch === "\\") {
          cur += ch;
          if (i + 1 < s.length) cur += s[++i];
          continue;
        }
        if (ch === ",") {
          if (!cur.trim()) return null;
          parts.push(cur.trim());
          cur = "";
          continue;
        }
        cur += ch;
      }
      if (!cur.trim()) return null;
      parts.push(cur.trim());

      const VALID_PSEUDO_PAGES = new Set([":left", ":right", ":first", ":blank"]);
      const canonParts = [];

      for (const part of parts) {
        if (!part) return null;
        let i = 0;
        let ident = "";
        if (part[0] !== ":") {
          while (i < part.length && part[i] !== ":") {
            if (part[i] === "\\") {
              ident += part[i];
              if (i + 1 < part.length) {
                ident += part[++i];
                i++;
              } else {
                return null;
              }
              continue;
            }
            if (/\s/.test(part[i])) break;
            ident += part[i++];
          }
          if (ident.startsWith("--")) return null;
          while (i < part.length && /\s/.test(part[i])) i++;
        }

        let pseudos = "";
        while (i < part.length) {
          if (part[i] !== ":") return null;
          let pseudo = ":";
          i++;
          while (i < part.length && /[a-zA-Z0-9_-]/.test(part[i])) {
            pseudo += part[i++];
          }
          const pseudoLower = pseudo.toLowerCase();
          if (!VALID_PSEUDO_PAGES.has(pseudoLower)) return null;
          pseudos += pseudoLower;
          while (i < part.length && /\s/.test(part[i])) i++;
        }

        if (!ident && !pseudos) return null;
        canonParts.push(ident + pseudos);
      }

      return canonParts.join(", ");
    }

    // 5.8 CSSPageRule
    function CSSPageRule() {
      CSSGroupingRule.call(this);
      this._type = 6;
      this._selectorText = "";
      this._style = null;
    }
    CSSPageRule.prototype = Object.create(CSSGroupingRule.prototype);
    CSSPageRule.prototype.constructor = CSSPageRule;
    Object.defineProperty(CSSPageRule.prototype, "selectorText", {
      get: function () { return this._selectorText || ""; },
      set: function (v) {
        const canon = canonicalizePageSelectorList(v);
        if (canon === null) return;
        this._selectorText = canon;
        if (this._onMutate) this._onMutate();
      },
      configurable: true,
    });
    Object.defineProperty(CSSPageRule.prototype, "style", {
      get: function () { return this._style; },
      set: function (v) {
        if (this._style) this._style.cssText = String(v == null ? "" : v);
      },
      configurable: true,
    });
    CSSPageRule.prototype.insertRule = function (ruleText, index) {
      if (arguments.length === 0) {
        throw new TypeError("Failed to execute 'insertRule': 1 argument required, but only 0 present.");
      }
      const idx = index === undefined ? 0 : Number(index) >>> 0;
      if (idx > this.cssRules.length) {
        throw new DOMException("Index out of bounds", "IndexSizeError");
      }
      const split = splitStylesheetRules(ruleText);
      if (split.unbalanced || split.length !== 1 || split.trailingGarbage) {
        throw new DOMException("Failed to parse CSS rule: " + ruleText, "SyntaxError");
      }
      const parsed = split[0];
      const marginName = atKeywordNameOf(parsed.selectorText);
      if (!marginName || !MARGIN_BOX_NAMES.has(marginName)) {
        throw new DOMException("Only margin at-rules may be inserted into a @page rule", "SyntaxError");
      }
      parsed.kind = "margin";
      parsed.marginName = marginName;
      const rule = constructRule(parsed, this.parentStyleSheet, this, () => {
        if (this._onMutate) this._onMutate();
      });
      this.cssRules.splice(idx, 0, rule);
      if (this._onMutate) this._onMutate();
      return idx;
    };
    CSSPageRule.prototype.deleteRule = function (index) {
      if (arguments.length === 0) {
        throw new TypeError("Failed to execute 'deleteRule': 1 argument required, but only 0 present.");
      }
      const idx = Number(index) >>> 0;
      if (idx >= this.cssRules.length) {
        throw new DOMException("Index out of bounds", "IndexSizeError");
      }
      this.cssRules.splice(idx, 1);
      if (this._onMutate) this._onMutate();
    };
    markNative(CSSPageRule, "CSSPageRule");
    global.CSSPageRule = CSSPageRule;
    if (typeof globalThis !== "undefined") globalThis.CSSPageRule = CSSPageRule;
    if (typeof window !== "undefined") window.CSSPageRule = CSSPageRule;

    // 5.9 CSSMarginRule
    function CSSMarginRule() {
      CSSRule.call(this);
      this._type = 9;
      this._name = "";
      this._style = null;
    }
    CSSMarginRule.prototype = Object.create(CSSRule.prototype);
    CSSMarginRule.prototype.constructor = CSSMarginRule;
    Object.defineProperty(CSSMarginRule.prototype, "name", {
      get: function () { return this._name || ""; },
      configurable: true,
    });
    Object.defineProperty(CSSMarginRule.prototype, "style", {
      get: function () { return this._style; },
      set: function (v) {
        if (this._style) this._style.cssText = String(v == null ? "" : v);
      },
      configurable: true,
    });
    markNative(CSSMarginRule, "CSSMarginRule");
    global.CSSMarginRule = CSSMarginRule;
    if (typeof globalThis !== "undefined") globalThis.CSSMarginRule = CSSMarginRule;
    if (typeof window !== "undefined") window.CSSMarginRule = CSSMarginRule;

    // 5.10 CSSFontFaceRule
    function CSSFontFaceRule() {
      CSSRule.call(this);
      this._type = 5;
    }
    CSSFontFaceRule.prototype = Object.create(CSSRule.prototype);
    CSSFontFaceRule.prototype.constructor = CSSFontFaceRule;
    markNative(CSSFontFaceRule, "CSSFontFaceRule");
    global.CSSFontFaceRule = CSSFontFaceRule;

    // 5.11 CSSKeyframesRule & CSSKeyframeRule
    function CSSKeyframesRule() {
      CSSRule.call(this);
      this._type = 7;
      this.name = "";
      this.cssRules = makeRuleList([]);
    }
    CSSKeyframesRule.prototype = Object.create(CSSRule.prototype);
    CSSKeyframesRule.prototype.constructor = CSSKeyframesRule;
    Object.defineProperty(CSSKeyframesRule.prototype, "length", {
      get: function () { return this.cssRules.length; },
      configurable: true,
    });
    markNative(CSSKeyframesRule, "CSSKeyframesRule");
    global.CSSKeyframesRule = CSSKeyframesRule;

    function CSSKeyframeRule() {
      CSSRule.call(this);
      this._type = 8;
      this.keyText = "";
    }
    CSSKeyframeRule.prototype = Object.create(CSSRule.prototype);
    CSSKeyframeRule.prototype.constructor = CSSKeyframeRule;
    markNative(CSSKeyframeRule, "CSSKeyframeRule");
    global.CSSKeyframeRule = CSSKeyframeRule;

    // 5.12 CSSNamespaceRule
    function CSSNamespaceRule() {
      CSSRule.call(this);
      this._type = 10;
      this.prefix = "";
      this.namespaceURI = "";
    }
    CSSNamespaceRule.prototype = Object.create(CSSRule.prototype);
    CSSNamespaceRule.prototype.constructor = CSSNamespaceRule;
    markNative(CSSNamespaceRule, "CSSNamespaceRule");
    global.CSSNamespaceRule = CSSNamespaceRule;

    // 5.13 CSSCounterStyleRule
    const COUNTER_STYLE_IDL_TO_DESC = {
      system: "system",
      symbols: "symbols",
      additiveSymbols: "additive-symbols",
      negative: "negative",
      prefix: "prefix",
      suffix: "suffix",
      range: "range",
      pad: "pad",
      speakAs: "speak-as",
      fallback: "fallback",
    };
    const COUNTER_STYLE_RESERVED_NAMES = new Set([
      "none", "decimal", "initial", "inherit", "unset", "revert", "revert-layer", "default"
    ]);
    const COUNTER_STYLE_SYSTEM_FAMILIES = new Set([
      "cyclic", "numeric", "alphabetic", "symbolic", "additive", "fixed", "extends"
    ]);

    function canonicalizePad(val) {
      const s = String(val == null ? "" : val).trim();
      if (!s) return "";
      const m1 = s.match(/^(\d+)\s+(.+)$/);
      if (m1) return m1[1] + " " + m1[2];
      const m2 = s.match(/^(.+?)\s+(\d+)$/);
      if (m2) return m2[2] + " " + m2[1];
      return s;
    }

    function canonicalizeAdditiveSymbols(val) {
      const s = String(val == null ? "" : val).trim();
      if (!s) return "";
      const pairs = s.split(",").map((p) => p.trim()).filter(Boolean);
      return pairs.map((p) => {
        const m1 = p.match(/^(\d+)\s+(.+)$/);
        if (m1) return m1[1] + " " + m1[2];
        const m2 = p.match(/^(.+?)\s+(\d+)$/);
        if (m2) return m2[2] + " " + m2[1];
        return p;
      }).join(", ");
    }

    function parseCounterStyleRule(cssText) {
      const text = String(cssText == null ? "" : cssText).trim();
      const brace = text.indexOf("{");
      const lastBrace = text.lastIndexOf("}");
      if (brace === -1 || lastBrace === -1 || lastBrace <= brace) return null;
      const prelude = text.slice(0, brace).trim();
      if (!/^@counter-style\b/i.test(prelude)) return null;

      const nameRaw = prelude.replace(/^@counter-style/i, "").trim();
      if (!nameRaw) return null;
      if (nameRaw.endsWith("\\") || nameRaw.includes("\\ ") || /\s/.test(nameRaw)) return null;
      if (COUNTER_STYLE_RESERVED_NAMES.has(nameRaw.toLowerCase())) return null;

      const body = text.slice(brace + 1, lastBrace).trim();
      if (body.includes("\\;") || /\\(?:\s*)$/.test(body)) return null;

      const descriptors = new Map();
      const decls = splitCssDeclarations(body);
      for (let i = 0; i < decls.length; i++) {
        const d = decls[i].trim();
        if (!d) continue;
        const colon = d.indexOf(":");
        if (colon <= 0) continue;
        const rawName = d.slice(0, colon).trim().toLowerCase();
        let val = d.slice(colon + 1).trim();

        if (rawName === "system") {
          const fam = val.split(/\s+/)[0]?.toLowerCase();
          if (!COUNTER_STYLE_SYSTEM_FAMILIES.has(fam)) {
            continue;
          }
        } else if (rawName === "pad") {
          val = canonicalizePad(val);
        } else if (rawName === "additive-symbols") {
          val = canonicalizeAdditiveSymbols(val);
        } else if (rawName === "suffix") {
          if (!val) continue;
        } else if (!Object.values(COUNTER_STYLE_IDL_TO_DESC).includes(rawName)) {
          continue;
        }

        descriptors.set(rawName, val);
      }

      return { name: nameRaw, descriptors };
    }

    function CSSCounterStyleRule() {
      CSSRule.call(this);
      this._type = 11;
      this._name = "";
      this._descriptors = new Map();
    }
    CSSCounterStyleRule.prototype = Object.create(CSSRule.prototype);
    CSSCounterStyleRule.prototype.constructor = CSSCounterStyleRule;

    Object.defineProperty(CSSCounterStyleRule.prototype, "name", {
      get: function () { return this._name || ""; },
      set: function (v) {
        const s = String(v);
        if (s === "\\") {
          this._name = "\ufffd";
          if (this._onMutate) this._onMutate();
          return;
        }
        const trimmed = s.trim();
        if (!trimmed || /\s/.test(trimmed)) return;
        if (COUNTER_STYLE_RESERVED_NAMES.has(trimmed.toLowerCase())) return;
        this._name = trimmed;
        if (this._onMutate) this._onMutate();
      },
      configurable: true,
    });

    Object.defineProperty(CSSCounterStyleRule.prototype, "system", {
      get: function () { return this._descriptors.get("system") || ""; },
      set: function (v) {
        const val = String(v).trim();
        if (!val) return;
        const newFamily = val.split(/\s+/)[0]?.toLowerCase();
        if (!COUNTER_STYLE_SYSTEM_FAMILIES.has(newFamily)) return;
        const oldVal = this._descriptors.get("system") || "";
        const oldFamily = oldVal ? oldVal.split(/\s+/)[0]?.toLowerCase() : "";
        if (oldFamily && oldFamily !== newFamily) return;
        this._descriptors.set("system", val);
        if (this._onMutate) this._onMutate();
      },
      configurable: true,
    });

    Object.defineProperty(CSSCounterStyleRule.prototype, "symbols", {
      get: function () { return this._descriptors.get("symbols") || ""; },
      set: function (v) {
        this._descriptors.set("symbols", String(v).trim());
        if (this._onMutate) this._onMutate();
      },
      configurable: true,
    });

    Object.defineProperty(CSSCounterStyleRule.prototype, "additiveSymbols", {
      get: function () { return this._descriptors.get("additive-symbols") || ""; },
      set: function (v) {
        this._descriptors.set("additive-symbols", canonicalizeAdditiveSymbols(String(v)));
        if (this._onMutate) this._onMutate();
      },
      configurable: true,
    });

    Object.defineProperty(CSSCounterStyleRule.prototype, "negative", {
      get: function () { return this._descriptors.get("negative") || ""; },
      set: function (v) {
        this._descriptors.set("negative", String(v).trim());
        if (this._onMutate) this._onMutate();
      },
      configurable: true,
    });

    Object.defineProperty(CSSCounterStyleRule.prototype, "prefix", {
      get: function () { return this._descriptors.get("prefix") || ""; },
      set: function (v) {
        this._descriptors.set("prefix", String(v).trim());
        if (this._onMutate) this._onMutate();
      },
      configurable: true,
    });

    Object.defineProperty(CSSCounterStyleRule.prototype, "suffix", {
      get: function () { return this._descriptors.get("suffix") || ""; },
      set: function (v) {
        if (v === "") return;
        const s = String(v);
        if (s === "\\") {
          this._descriptors.set("suffix", '"\\ufffd"');
          if (this._onMutate) this._onMutate();
          return;
        }
        this._descriptors.set("suffix", s.trim());
        if (this._onMutate) this._onMutate();
      },
      configurable: true,
    });

    Object.defineProperty(CSSCounterStyleRule.prototype, "range", {
      get: function () { return this._descriptors.get("range") || ""; },
      set: function (v) {
        this._descriptors.set("range", String(v).trim());
        if (this._onMutate) this._onMutate();
      },
      configurable: true,
    });

    Object.defineProperty(CSSCounterStyleRule.prototype, "pad", {
      get: function () { return this._descriptors.get("pad") || ""; },
      set: function (v) {
        this._descriptors.set("pad", canonicalizePad(String(v)));
        if (this._onMutate) this._onMutate();
      },
      configurable: true,
    });

    Object.defineProperty(CSSCounterStyleRule.prototype, "speakAs", {
      get: function () { return this._descriptors.get("speak-as") || ""; },
      set: function (v) {
        this._descriptors.set("speak-as", String(v).trim());
        if (this._onMutate) this._onMutate();
      },
      configurable: true,
    });

    Object.defineProperty(CSSCounterStyleRule.prototype, "fallback", {
      get: function () { return this._descriptors.get("fallback") || ""; },
      set: function (v) {
        this._descriptors.set("fallback", String(v).trim());
        if (this._onMutate) this._onMutate();
      },
      configurable: true,
    });

    markNative(CSSCounterStyleRule, "CSSCounterStyleRule");
    global.CSSCounterStyleRule = CSSCounterStyleRule;
    if (typeof globalThis !== "undefined") globalThis.CSSCounterStyleRule = CSSCounterStyleRule;
    if (typeof window !== "undefined") window.CSSCounterStyleRule = CSSCounterStyleRule;

    // 5.14 CSSFontFeatureValuesRule
    function CSSFontFeatureValuesRule() {
      CSSRule.call(this);
      this._type = 14;
      this.fontFamily = "";
    }
    CSSFontFeatureValuesRule.prototype = Object.create(CSSRule.prototype);
    CSSFontFeatureValuesRule.prototype.constructor = CSSFontFeatureValuesRule;
    markNative(CSSFontFeatureValuesRule, "CSSFontFeatureValuesRule");
    global.CSSFontFeatureValuesRule = CSSFontFeatureValuesRule;

    // =========================================================================
    // 6. Stylesheet Rule Parsing & Splitting
    // =========================================================================

    function splitStylesheetRules(src) {
      const out = [];
      out.trailingGarbage = "";
      out.unbalanced = false;
      const text = String(src == null ? "" : src).replace(/\/\*[\s\S]*?\*\//g, "");
      const n = text.length;
      let i = 0;
      let chunkStart = 0;
      let depth = 0;
      let parenDepth = 0;
      let braceOpenAt = -1;
      let inStr = "";

      while (i < n) {
        const ch = text[i];
        if (inStr) {
          if (ch === "\\") { i += 2; continue; }
          if (ch === inStr) inStr = "";
          i++;
          continue;
        }
        if (ch === "\\") { i += 2; continue; }
        if (ch === '"' || ch === "'") { inStr = ch; i++; continue; }
        if (ch === "(") { parenDepth++; i++; continue; }
        if (ch === ")" && parenDepth > 0) { parenDepth--; i++; continue; }
        if (ch === "{") {
          if (depth === 0) braceOpenAt = i;
          depth++;
          i++;
          continue;
        }
        if (ch === "}") {
          depth--;
          if (depth === 0 && braceOpenAt >= 0) {
            const selectorText = text.slice(chunkStart, braceOpenAt).trim();
            const declText = text.slice(braceOpenAt + 1, i).trim();
            const cssText = selectorText
              ? selectorText + " { " + declText + " }"
              : text.slice(chunkStart, i + 1).trim();
            out.push({ cssText, selectorText, declText });
            chunkStart = i + 1;
            braceOpenAt = -1;
          }
          i++;
          continue;
        }
        if (depth === 0 && parenDepth === 0 && ch === ";") {
          const raw = text.slice(chunkStart, i + 1).trim();
          if (raw) out.push({ cssText: raw, selectorText: "", declText: "" });
          chunkStart = i + 1;
        }
        i++;
      }

      const tail = text.slice(chunkStart).trim();
      out.trailingGarbage = tail.length > 0 ? tail : "";
      out.unbalanced = depth !== 0 || parenDepth !== 0 || inStr !== "";
      return out;
    }

    function classifyRuleKind(parsed) {
      const sel = String(parsed.selectorText || "").trim();
      const raw = String(parsed.cssText || "").trim();
      if (sel.startsWith("@")) {
        const marginMatch = sel.match(/^@([a-zA-Z-]+)/);
        if (marginMatch && MARGIN_BOX_NAMES.has(marginMatch[1].toLowerCase())) {
          return "margin";
        }
      }
      const at = (sel || raw).match(/^@([a-zA-Z-]+)/);
      if (!at) return sel ? "style" : "invalid";
      const name = at[1].toLowerCase();
      switch (name) {
        case "media": return "media";
        case "supports": return "supports";
        case "layer": return sel ? "layer-block" : "layer-statement";
        case "import": return "import";
        case "namespace": return "namespace";
        case "font-face": return "font-face";
        case "font-palette-values": return "font-palette-values";
        case "font-feature-values": return "font-feature-values";
        case "keyframes":
        case "-webkit-keyframes": return "keyframes";
        case "page": return "page";
        case "charset": return "charset";
        case "counter-style": return "counter-style";
        default: return "unknown";
      }
    }

    function validateSelectorOrThrow(sel) {
      if (typeof sel !== "string") sel = String(sel);
      const s = sel.trim();
      if (!s || s === "!!" || /^[!@#$%^&;]+$/.test(s) || /^[+>~]$/.test(s)) {
        throw new DOMException("The provided selector is invalid.", "SyntaxError");
      }
      if (s.includes("::")) {
        const pseudoPattern = /::([a-zA-Z-]+)(.*)$/;
        const m = s.match(pseudoPattern);
        if (m && m[2].trim().length > 0) {
          throw new DOMException("The provided selector is invalid.", "SyntaxError");
        }
      }
    }

    function parseSingleRuleOrThrow(text) {
      const t = String(text == null ? "" : text);
      const split = splitStylesheetRules(t);
      if (split.unbalanced || split.length !== 1 || split.trailingGarbage) {
        throw new DOMException("Failed to parse CSS rule: " + t, "SyntaxError");
      }
      const parsed = split[0];
      const kind = classifyRuleKind(parsed);
      parsed.kind = kind;
      if (kind === "charset" || kind === "invalid" || kind === "margin") {
        throw new DOMException("Failed to parse CSS rule: invalid rule", "SyntaxError");
      }
      if (kind === "page") {
        const canonSel = canonicalizePageSelectorList(parsed.selectorText.replace(/^@page/i, ""));
        if (canonSel === null) {
          throw new DOMException("Failed to parse CSS rule: invalid page selector", "SyntaxError");
        }
      }
      if (kind === "counter-style") {
        const cs = parseCounterStyleRule(parsed.cssText);
        if (!cs) {
          throw new DOMException("Failed to parse CSS rule: invalid counter-style", "SyntaxError");
        }
      }
      if (kind === "style") {
        validateSelectorOrThrow(parsed.selectorText);
      }
      return parsed;
    }

    function constructRule(parsed, parentSheet, parentRule, onMutate) {
      const kind = parsed.kind || classifyRuleKind(parsed);
      const sel = String(parsed.selectorText || "").trim();
      const raw = String(parsed.cssText || "").trim();

      if (kind === "style") {
        const rule = new CSSStyleRule();
        rule.parentRule = parentRule || null;
        rule.parentStyleSheet = parentSheet || null;
        rule._onMutate = onMutate || null;
        rule.selectorText = sel;
        const style = makeRuleStyleDeclaration(parsed.declText || "", "style", null, onMutate);
        style._parentRule = rule;
        rule.style = style;
        rule._serialize = function () {
          const d = style.cssText;
          return rule.selectorText + (d ? " { " + d + " }" : " { }");
        };
        return rule;
      }

      if (kind === "media" || kind === "supports") {
        const rule = kind === "media" ? new CSSMediaRule() : new CSSSupportsRule();
        rule.parentRule = parentRule || null;
        rule.parentStyleSheet = parentSheet || null;
        rule._onMutate = onMutate || null;
        const cond = sel.replace(/^@(media|supports)/i, "").trim();
        rule._conditionText = cond;
        if (kind === "media") {
          rule.media = new MediaList(cond, onMutate);
        }
        const children = splitStylesheetRules(parsed.declText || "");
        for (let i = 0; i < children.length; i++) {
          const c = constructRule(children[i], parentSheet, rule, onMutate);
          if (c) rule.cssRules.push(c);
        }
        rule._serialize = function () {
          const c = rule.media ? rule.media.mediaText : rule.conditionText;
          const inner = rule.cssRules.map((r) => r.cssText).join("\n  ");
          return "@" + kind + (c ? " " + c : "") + (inner ? " {\n  " + inner + "\n}" : " {\n}");
        };
        return rule;
      }

      if (kind === "import") {
        const rule = new CSSImportRule();
        rule.parentRule = parentRule || null;
        rule.parentStyleSheet = parentSheet || null;
        rule._onMutate = onMutate || null;
        const m = raw.match(/^@import\s+(?:url\(\s*(?:"([^"]*)"|'([^']*)'|([^)"']*))\s*\)|"([^"]*)"|'([^']*)')\s*(layer(?:\(([^)]*)\))?)?\s*(?:supports\(([\s\S]*?)\))?\s*([^;]*);?\s*$/i);
        if (m) {
          rule.href = m[1] ?? m[2] ?? (m[3] !== undefined ? m[3].trim() : undefined) ?? m[4] ?? m[5] ?? "";
          rule.layerName = m[6] ? (m[7] !== undefined ? m[7].trim() : "") : null;
          rule.supportsText = m[8] !== undefined ? m[8].trim() : null;
          rule.media = new MediaList((m[9] || "").trim(), onMutate);
        }
        rule._serialize = function () {
          let out = '@import url("' + rule.href + '")';
          if (rule.layerName !== null) {
            out += rule.layerName === "" ? " layer" : " layer(" + rule.layerName + ")";
          }
          if (rule.supportsText !== null) {
            out += " supports(" + rule.supportsText + ")";
          }
          const mt = rule.media.mediaText;
          if (mt) out += " " + mt;
          return out + ";";
        };
        return rule;
      }

      if (kind === "font-face") {
        const rule = new CSSFontFaceRule();
        rule.parentRule = parentRule || null;
        rule.parentStyleSheet = parentSheet || null;
        rule._onMutate = onMutate || null;
        const style = makeRuleStyleDeclaration(parsed.declText || "", "font-face", null, onMutate);
        style._parentRule = rule;
        rule.style = style;
        rule._serialize = function () {
          const d = style.cssText;
          return "@font-face" + (d ? " { " + d + " }" : " { }");
        };
        return rule;
      }

      if (kind === "font-palette-values") {
        const rule = new CSSFontPaletteValuesRule();
        rule.parentRule = parentRule || null;
        rule.parentStyleSheet = parentSheet || null;
        rule._onMutate = onMutate || null;
        rule.name = sel.replace(/^@font-palette-values/i, "").trim();
        const style = makeRuleStyleDeclaration(parsed.declText || "", "style", null, onMutate);
        rule.fontFamily = style.getPropertyValue("font-family") || "";
        rule.basePalette = style.getPropertyValue("base-palette") || "";
        rule.overrideColors = style.getPropertyValue("override-colors") || "";
        rule._serialize = function () {
          const d = style.cssText;
          return "@font-palette-values " + rule.name + (d ? " { " + d + " }" : " { }");
        };
        return rule;
      }

      if (kind === "page") {
        const canonSel = canonicalizePageSelectorList(sel.replace(/^@page/i, ""));
        if (canonSel === null) return null;

        const rule = new CSSPageRule();
        rule.parentRule = parentRule || null;
        rule.parentStyleSheet = parentSheet || null;
        rule._onMutate = onMutate || null;
        rule._selectorText = canonSel;

        const declParts = [];
        const childRules = makeRuleList([]);
        const pageBody = splitStylesheetRules(parsed.declText || "");
        for (let i = 0; i < pageBody.length; i++) {
          const part = pageBody[i];
          const mName = atKeywordNameOf(part.selectorText);
          if (mName && MARGIN_BOX_NAMES.has(mName)) {
            part.kind = "margin";
            part.marginName = mName;
            const mRule = constructRule(part, parentSheet, rule, onMutate);
            if (mRule) childRules.push(mRule);
          } else if (!part.selectorText && !/^@/.test(String(part.cssText || "").trim())) {
            declParts.push(String(part.cssText));
          }
        }
        if (pageBody.trailingGarbage && !/^@/.test(pageBody.trailingGarbage)) {
          declParts.push(pageBody.trailingGarbage);
        }

        const style = makeRuleStyleDeclaration(declParts.join("; "), "page", null, onMutate);
        style._parentRule = rule;
        rule._style = style;
        rule.cssRules = childRules;

        rule._serialize = function () {
          const parts = [];
          let d = style.cssText;
          if (d) {
            if (!d.endsWith(";")) d += ";";
            parts.push(d);
          }
          for (let i = 0; i < rule.cssRules.length; i++) {
            parts.push(rule.cssRules[i].cssText);
          }
          const body = parts.join(" ");
          return "@page" + (rule.selectorText ? " " + rule.selectorText : "") + (body ? " { " + body + " }" : " { }");
        };
        return rule;
      }

      if (kind === "margin") {
        const rule = new CSSMarginRule();
        rule.parentRule = parentRule || null;
        rule.parentStyleSheet = parentSheet || null;
        rule._onMutate = onMutate || null;
        rule._name = parsed.marginName || atKeywordNameOf(sel) || "";
        const style = makeRuleStyleDeclaration(parsed.declText || "", "margin", null, onMutate);
        style._parentRule = rule;
        rule._style = style;
        rule._serialize = function () {
          let d = style.cssText;
          if (d && !d.endsWith(";")) d += ";";
          return "@" + rule.name + (d ? " { " + d + " }" : " { }");
        };
        return rule;
      }

      if (kind === "counter-style") {
        const csData = parseCounterStyleRule(parsed.cssText);
        if (!csData) return null;

        const rule = new CSSCounterStyleRule();
        rule.parentRule = parentRule || null;
        rule.parentStyleSheet = parentSheet || null;
        rule._onMutate = onMutate || null;
        rule._name = csData.name;
        for (const [k, v] of csData.descriptors) {
          rule._descriptors.set(k, v);
        }
        rule._serialize = function () {
          const parts = [];
          for (const [k, v] of rule._descriptors) {
            parts.push(k + ": " + v + ";");
          }
          return "@counter-style " + rule.name + (parts.length ? " { " + parts.join(" ") + " }" : " { }");
        };
        return rule;
      }

      if (kind === "keyframes") {
        const rule = new CSSKeyframesRule();
        rule.parentRule = parentRule || null;
        rule.parentStyleSheet = parentSheet || null;
        rule._onMutate = onMutate || null;
        rule.name = sel.replace(/^@(-webkit-)?keyframes/i, "").trim();

        const children = splitStylesheetRules(parsed.declText || "");
        for (let i = 0; i < children.length; i++) {
          const p = children[i];
          if (!p.selectorText) continue;
          const kf = new CSSKeyframeRule();
          kf.parentRule = rule;
          kf.parentStyleSheet = parentSheet || null;
          kf.keyText = p.selectorText.trim();
          const style = makeRuleStyleDeclaration(p.declText || "", "keyframe", null, onMutate);
          style._parentRule = kf;
          kf.style = style;
          kf._serialize = function () {
            const d = style.cssText;
            return kf.keyText + (d ? " { " + d + " }" : " { }");
          };
          rule.cssRules.push(kf);
        }
        rule._serialize = function () {
          const inner = rule.cssRules.map((r) => r.cssText).join("\n  ");
          return "@keyframes " + rule.name + (inner ? " {\n  " + inner + "\n}" : " {\n}");
        };
        return rule;
      }

      // Default CSSRule
      const rule = new CSSRule();
      rule.parentRule = parentRule || null;
      rule.parentStyleSheet = parentSheet || null;
      rule._type = 0;
      rule._serialize = function () {
        if (sel) {
          const d = String(parsed.declText || "").replace(/\s+/g, " ").trim();
          return sel + (d ? " { " + d + " }" : " { }");
        }
        return raw.replace(/\s+/g, " ").trim();
      };
      return rule;
    }

    // =========================================================================
    // 7. CSSStyleSheet
    // =========================================================================

    function CSSStyleSheet(ownerNode) {
      this.ownerNode = ownerNode || null;
      this.ownerRule = null;
      this.parentStyleSheet = null;
      this.href = null;
      this.title = null;
      this.media = new MediaList("");
      this.disabled = false;
      this._rules = [];
      this._lastParsedText = null;

      const self = this;
      const rulesList = makeRuleList(this._rules);

      Object.defineProperty(this, "cssRules", {
        get: function () {
          self._syncFromOwner();
          return rulesList;
        },
        configurable: true,
      });

      Object.defineProperty(this, "rules", {
        get: function () { return this.cssRules; },
        configurable: true,
      });
    }

    CSSStyleSheet.prototype._syncFromOwner = function () {
      if (!this.ownerNode) return;
      const cur = String(this.ownerNode.textContent || "");
      if (cur !== this._lastParsedText) {
        this._reparseText(cur);
      }
    };

    CSSStyleSheet.prototype._reparseText = function (text) {
      this._rules.length = 0;
      const chunks = splitStylesheetRules(text);
      for (let i = 0; i < chunks.length; i++) {
        const rule = constructRule(chunks[i], this, null, () => this._remirror());
        if (rule) this._rules.push(rule);
      }
      this._lastParsedText = text;
    };

    CSSStyleSheet.prototype._remirror = function () {
      if (!this.ownerNode) return;
      const text = this._rules.map((r) => r.cssText).join("\n");
      this._lastParsedText = text;
      this.ownerNode.textContent = text;
    };

    CSSStyleSheet.prototype.insertRule = function (ruleText, index) {
      if (arguments.length === 0) {
        throw new TypeError("Failed to execute 'insertRule': 1 argument required, but only 0 present.");
      }
      this._syncFromOwner();
      const idx = index === undefined ? 0 : Number(index) >>> 0;
      if (idx > this._rules.length) {
        throw new DOMException("Index out of bounds", "IndexSizeError");
      }
      const parsed = parseSingleRuleOrThrow(ruleText);
      const rule = constructRule(parsed, this, null, () => this._remirror());
      if (!rule) throw new DOMException("Failed to parse CSS rule", "SyntaxError");
      this._rules.splice(idx, 0, rule);
      this._remirror();
      return idx;
    };

    CSSStyleSheet.prototype.deleteRule = function (index) {
      if (arguments.length === 0) {
        throw new TypeError("Failed to execute 'deleteRule': 1 argument required, but only 0 present.");
      }
      this._syncFromOwner();
      const idx = Number(index) >>> 0;
      if (idx >= this._rules.length) {
        throw new DOMException("Index out of bounds", "IndexSizeError");
      }
      this._rules.splice(idx, 1);
      this._remirror();
    };

    CSSStyleSheet.prototype.replaceSync = function (text) {
      this._reparseText(String(text == null ? "" : text));
      this._remirror();
    };

    CSSStyleSheet.prototype.replace = function (text) {
      try {
        this.replaceSync(text);
      } catch (e) {
        return Promise.reject(e);
      }
      return Promise.resolve(this);
    };

    markNative(CSSStyleSheet, "CSSStyleSheet");
    global.CSSStyleSheet = CSSStyleSheet;
    if (typeof globalThis !== "undefined") globalThis.CSSStyleSheet = CSSStyleSheet;
    if (typeof window !== "undefined") window.CSSStyleSheet = CSSStyleSheet;

    function getOrCreateStyleSheet(ownerNode) {
      if (!ownerNode) return new CSSStyleSheet(null);
      if (!ownerNode._sheet) {
        ownerNode._sheet = new CSSStyleSheet(ownerNode);
      }
      return ownerNode._sheet;
    }
    global.getOrCreateStyleSheet = getOrCreateStyleSheet;
    if (typeof globalThis !== "undefined") globalThis.getOrCreateStyleSheet = getOrCreateStyleSheet;

    // =========================================================================
    // 8. Global getComputedStyle wrapper for var() resolution & padding/margin expansion
    // =========================================================================

    function resolveCssVars(el, val) {
      if (!val || typeof val !== "string" || !val.includes("var(")) return val;
      let res = val;
      let match;
      const varRegex = /var\(\s*(--[a-zA-Z0-9_-]+)(?:\s*,\s*([^)]+))?\s*\)/g;
      while ((match = varRegex.exec(val)) !== null) {
        const varName = match[1];
        const fallback = match[2] || "";
        let found = "";

        // Check element inline style
        if (el && el.style && typeof el.style.getPropertyValue === "function") {
          found = el.style.getPropertyValue(varName);
        }

        // Check ancestors up to documentElement
        if (!found && el) {
          let curr = el.parentElement || el.parentNode;
          while (curr && curr.nodeType === 1) {
            if (curr.style && typeof curr.style.getPropertyValue === "function") {
              found = curr.style.getPropertyValue(varName);
              if (found) break;
            }
            curr = curr.parentElement || curr.parentNode;
          }
        }

        // Check document style elements for :root / html declarations
        if (!found) {
          const doc = el?.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
          if (doc) {
            const styles = doc.querySelectorAll ? doc.querySelectorAll("style") : [];
            for (let i = 0; i < styles.length; i++) {
              const txt = styles[i].textContent || "";
              const rootMatch = txt.match(/(?::root|html)\s*\{([^}]+)\}/i);
              if (rootMatch) {
                const decls = rootMatch[1].split(";");
                for (let j = 0; j < decls.length; j++) {
                  const colon = decls[j].indexOf(":");
                  if (colon > 0) {
                    const k = decls[j].slice(0, colon).trim();
                    if (k === varName) {
                      found = decls[j].slice(colon + 1).trim();
                      break;
                    }
                  }
                }
              }
              if (found) break;
            }
          }
        }

        const replacement = found || fallback;
        res = res.replace(match[0], replacement);
      }
      return res;
    }

    let _capturedGetComputedStyle = null;

    function wrapComputedStyleDecl(decl, element) {
      if (!decl || typeof decl !== "object") return decl;
      return new Proxy(decl, {
        get: function (target, prop, receiver) {
          if (typeof prop === "symbol") return target[prop];
          if (typeof prop === "string") {
            const kebab = cssPropCamelToKebab(prop);

            // If color is unstyled, default to rgb(0, 0, 0)
            if (kebab === "color") {
              const val = target.getPropertyValue ? target.getPropertyValue("color") : target[prop];
              return val || "rgb(0, 0, 0)";
            }

            // Shorthand padding -> paddingTop / paddingBottom / etc.
            if (kebab.startsWith("padding-")) {
              let val = target.getPropertyValue ? target.getPropertyValue(kebab) : target[prop];
              if (!val) {
                const shorthand = target.getPropertyValue ? target.getPropertyValue("padding") : target.padding;
                if (shorthand) val = resolveCssVars(element, shorthand);
              } else {
                val = resolveCssVars(element, val);
              }
              return val || "";
            }

            // Shorthand margin -> marginTop / marginBottom / etc.
            if (kebab.startsWith("margin-")) {
              let val = target.getPropertyValue ? target.getPropertyValue(kebab) : target[prop];
              if (!val) {
                const shorthand = target.getPropertyValue ? target.getPropertyValue("margin") : target.margin;
                if (shorthand) val = resolveCssVars(element, shorthand);
              } else {
                val = resolveCssVars(element, val);
              }
              return val || "";
            }

            let val = target[prop];
            if (typeof val === "string") val = resolveCssVars(element, val);
            return val;
          }
          return target[prop];
        }
      });
    }

    function enhancedGetComputedStyle(element, pseudoElt) {
      if (!_capturedGetComputedStyle) {
        throw new TypeError("getComputedStyle is not initialized");
      }
      const raw = _capturedGetComputedStyle.call(this, element, pseudoElt);
      return wrapComputedStyleDecl(raw, element);
    }
    markNative(enhancedGetComputedStyle, "getComputedStyle");

    function installGetComputedStyle(targetObj) {
      if (!targetObj) return;
      Object.defineProperty(targetObj, "getComputedStyle", {
        get: function () {
          return _capturedGetComputedStyle ? enhancedGetComputedStyle : undefined;
        },
        set: function (fn) {
          _capturedGetComputedStyle = fn;
        },
        configurable: true,
        enumerable: true,
      });
    }

    installGetComputedStyle(global);
    if (typeof globalThis !== "undefined") installGetComputedStyle(globalThis);
    if (typeof window !== "undefined") installGetComputedStyle(window);

  })(typeof window !== "undefined" ? window : globalThis);

  // =============================================================
  // Module: node-base.js
  // =============================================================
  // src/js/shim/node-base.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/node-base.ts

  const parsedNodeRegistry = (global.__parsedNodeRegistry = global.__parsedNodeRegistry || new WeakSet());

  function isParsedNode(node) {
    return typeof node === "object" && node !== null && parsedNodeRegistry.has(node);
  }

  function registerParsedNode(node) {
    if (typeof node === "object" && node !== null) {
      parsedNodeRegistry.add(node);
    }
    return node;
  }

  function Node() {
    EventTarget.call(this);
  }
  Node.prototype = Object.create(EventTarget.prototype);
  Node.prototype.constructor = Node;

  // Node Type Constants on constructor and prototype
  const nodeTypes = {
    ELEMENT_NODE: ELEMENT_NODE,
    ATTRIBUTE_NODE: ATTRIBUTE_NODE,
    TEXT_NODE: TEXT_NODE,
    CDATA_SECTION_NODE: CDATA_SECTION_NODE,
    ENTITY_REFERENCE_NODE: ENTITY_REFERENCE_NODE,
    ENTITY_NODE: ENTITY_NODE,
    PROCESSING_INSTRUCTION_NODE: PROCESSING_INSTRUCTION_NODE,
    COMMENT_NODE: COMMENT_NODE,
    DOCUMENT_NODE: DOCUMENT_NODE,
    DOCUMENT_TYPE_NODE: DOCUMENT_TYPE_NODE,
    DOCUMENT_FRAGMENT_NODE: DOCUMENT_FRAGMENT_NODE,
    NOTATION_NODE: NOTATION_NODE,
    DOCUMENT_POSITION_DISCONNECTED: DOCUMENT_POSITION_DISCONNECTED,
    DOCUMENT_POSITION_PRECEDING: DOCUMENT_POSITION_PRECEDING,
    DOCUMENT_POSITION_FOLLOWING: DOCUMENT_POSITION_FOLLOWING,
    DOCUMENT_POSITION_CONTAINS: DOCUMENT_POSITION_CONTAINS,
    DOCUMENT_POSITION_CONTAINED_BY: DOCUMENT_POSITION_CONTAINED_BY,
    DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC: DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC,
  };

  for (const k in nodeTypes) {
    Node[k] = nodeTypes[k];
    Node.prototype[k] = nodeTypes[k];
  }

  Object.defineProperty(Node.prototype, "isConnected", {
    get: function () {
      let curr = this;
      while (curr) {
        if (curr.nodeType === DOCUMENT_NODE) return true;
        if (curr.host) {
          curr = curr.host;
          continue;
        }
        curr = curr.parentNode || curr.parentElement;
      }
      return false;
    },
    configurable: true,
  });

  Node.prototype.getRootNode = function (options) {
    let n = this;
    let p;
    while ((p = n.parentNode)) n = p;

    if (options && options.composed) {
      while (n && n.host) {
        let hop = n.host;
        let hp;
        while ((hp = hop.parentNode)) hop = hp;
        n = hop;
      }
    }
    return n;
  };

  Node.prototype.normalize = function () {
    const visit = (parent) => {
      if (!parent) return;
      let children = [];
      if (parent._children && parent._children.length > 0) {
        children = parent._children.slice();
      } else if (parent.childNodes && parent.childNodes.length > 0) {
        children = Array.from(parent.childNodes);
      } else if (parent.nodeType === 9 && parent.documentElement) {
        children = [parent.documentElement];
      } else if (nativeElementFirstChild) {
        let c = null;
        try { c = nativeElementFirstChild.call(parent); } catch (e) {}
        while (c) {
          children.push(c);
          try { c = nativeElementNextSibling.call(c); } catch (e) { c = null; }
        }
      }
      for (let i = 0; i < children.length; i++) {
        const c = children[i];
        if (!c) continue;
        const t = c.nodeType;
        if (t === TEXT_NODE) {
          const cData = c.data !== undefined ? c.data : (c.textContent || "");
          if (cData.length === 0) {
            try {
              parent.removeChild(c);
            } catch (e) {}
            children[i] = null;
            continue;
          }
          let merged = cData;
          let j = i + 1;
          while (j < children.length) {
            const nx = children[j];
            if (!nx) break;
            if (nx.nodeType !== TEXT_NODE) break;
            const nxData = nx.data !== undefined ? nx.data : (nx.textContent || "");
            merged += nxData;
            try {
              parent.removeChild(nx);
            } catch (e) {}
            children[j] = null;
            j++;
          }
          if (c.data !== merged) {
            c.data = merged;
          }
          if (c.textContent !== merged) {
            c.textContent = merged;
          }
          if (c._data !== undefined) {
            c._data = merged;
          }
          i = j - 1;
        } else if (t === ELEMENT_NODE || t === DOCUMENT_NODE || t === DOCUMENT_FRAGMENT_NODE) {
          visit(c);
        }
      }
    };
    visit(this);
  };

  Object.defineProperty(Node.prototype, "ownerDocument", {
    get: function () {
      if (this.nodeType === DOCUMENT_NODE) return null;
      if (this._ownerDocument !== undefined) return this._ownerDocument;
      return typeof document !== "undefined" ? document : null;
    },
    set: function (doc) {
      this._ownerDocument = doc;
    },
    configurable: true,
  });

  Node.prototype.contains = function (other) {
    if (!other) return false;
    let curr = other;
    while (curr) {
      if (curr === this || (this._nid !== undefined && curr._nid !== undefined && curr._nid === this._nid)) return true;
      curr = curr.parentNode || curr.parentElement;
    }
    return false;
  };

  Node.prototype.compareDocumentPosition = function (other) {
    if (this === other) return 0;
    if (!other || !other.nodeType) return DOCUMENT_POSITION_DISCONNECTED;

    if (this.contains(other)) {
      return DOCUMENT_POSITION_CONTAINED_BY | DOCUMENT_POSITION_FOLLOWING;
    }
    if (other.contains(this)) {
      return DOCUMENT_POSITION_CONTAINS | DOCUMENT_POSITION_PRECEDING;
    }

    // Node ID comparison fallback
    if (typeof this._nid === "number" && typeof other._nid === "number") {
      if (this._nid < other._nid) return DOCUMENT_POSITION_FOLLOWING;
      return DOCUMENT_POSITION_PRECEDING;
    }
    return DOCUMENT_POSITION_DISCONNECTED;
  };

  Node.prototype.cloneNode = function (deep) {
    if (this.nodeType === TEXT_NODE) {
      return new Text(this.textContent);
    }
    if (this.nodeType === COMMENT_NODE) {
      return new Comment(this.textContent);
    }
    if (this.nodeType === ELEMENT_NODE && typeof document !== "undefined" && document.createElement) {
      const clone = document.createElement(this.tagName.toLowerCase());
      if (this.id) clone.id = this.id;
      if (this.className) clone.className = this.className;
      if (this.getAttributeNames) {
        const attrs = this.getAttributeNames();
        for (let i = 0; i < attrs.length; i++) {
          clone.setAttribute(attrs[i], this.getAttribute(attrs[i]));
        }
      }
      if (deep && this.childNodes) {
        for (let i = 0; i < this.childNodes.length; i++) {
          clone.appendChild(this.childNodes[i].cloneNode(true));
        }
      }
      return clone;
    }
    return this;
  };

  function isGenuineNode(obj) {
    if (!obj || typeof obj !== "object") return false;
    if (obj instanceof Node) return true;
    if (isParsedNode(obj)) return true;
    return false;
  }

  const nativeElementAppendChild = (typeof Element !== "undefined" && Element.prototype) ? Element.prototype.appendChild : null;
  const nativeElementRemoveChild = (typeof Element !== "undefined" && Element.prototype) ? Element.prototype.removeChild : null;
  const nativeElementGetTextContent = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "textContent")?.get : null;
  const nativeElementSetTextContent = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "textContent")?.set : null;
  const nativeElementFirstChild = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "firstChild")?.get : null;
  const nativeElementNextSibling = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "nextSibling")?.get : null;

  function ensureChildren(node) {
    if (!node || typeof node !== "object") return [];
    const tag = String(node.localName || node.tagName || "").toLowerCase();
    if (tag === "template") {
      if (typeof node.content !== "undefined") {
        const _ = node.content;
      }
      if (!node._children) node._children = [];
      return node._children;
    }
    if (!node._children) {
      node._children = [];
      try {
        let child = null;
        if (node.nodeType === 9) { // DOCUMENT_NODE
          child = node.documentElement;
        } else if (nativeElementFirstChild) {
          child = nativeElementFirstChild.call(node);
        }
        while (child) {
          node._children.push(child);
          if (nativeElementNextSibling) {
            child = nativeElementNextSibling.call(child);
          } else {
            break;
          }
        }
      } catch (e) {}
    }
    return node._children;
  }

  function executeConnectedScripts(node) {
    if (!node || typeof node !== "object") return;
    if (!node.isConnected) return;
    const tag = String(node.localName || node.tagName || "").toLowerCase();
    if (tag === "script" && !node._alreadyStarted) {
      node._alreadyStarted = true;
      const nativeGetAttr = (typeof Element !== "undefined" && Element.prototype && Element.prototype.getAttribute)
        ? Element.prototype.getAttribute
        : (node.getAttribute ? node.getAttribute.bind(node) : null);
      const type = String(
        (nativeGetAttr ? nativeGetAttr.call(node, "type") : node.type) || ""
      ).trim().toLowerCase();
      const isJs = !type || type === "module" || /^(?:text|application)\/(?:javascript|ecmascript)$/i.test(type);
      if (isJs) {
        const nativeHasAttr = (typeof Element !== "undefined" && Element.prototype && Element.prototype.hasAttribute)
          ? Element.prototype.hasAttribute
          : (node.hasAttribute ? node.hasAttribute.bind(node) : null);
        const hasNoModule = nativeHasAttr ? nativeHasAttr.call(node, "nomodule") : (node.hasAttribute ? node.hasAttribute("nomodule") : false);
        const src = (nativeGetAttr ? nativeGetAttr.call(node, "src") : null) || node.src || null;
        const code = node.text || node.textContent || "";

        if (!src && !code) {
          node._alreadyStarted = false;
          return;
        }

        if (type !== "module" && hasNoModule) {
          return;
        }

        if (src) {
          if (typeof fetch === "function") {
            fetch(src)
              .then(function (res) {
                if (!res.ok && res.status !== 200) {
                  throw new Error("HTTP " + res.status);
                }
                return res.text();
              })
              .then(function (code) {
                try {
                  (1, eval)(code);
                } catch (evalErr) {
                  if (typeof console !== "undefined" && console.error) {
                    console.error("Error executing script " + src, evalErr);
                  }
                }
                const evt = new Event("load", { bubbles: false, cancelable: false });
                if (typeof node.onload === "function") {
                  try { node.onload.call(node, evt); } catch (e) {
                    if (typeof console !== "undefined" && console.error) console.error(e);
                  }
                }
                if (typeof node.dispatchEvent === "function") {
                  try { node.dispatchEvent(evt); } catch (e) {}
                }
              })
              .catch(function (fetchErr) {
                const evt = new Event("error", { bubbles: false, cancelable: false });
                if (typeof node.onerror === "function") {
                  try { node.onerror.call(node, evt); } catch (e) {
                    if (typeof console !== "undefined" && console.error) console.error(e);
                  }
                }
                if (typeof node.dispatchEvent === "function") {
                  try { node.dispatchEvent(evt); } catch (e) {}
                }
              });
          }
        } else {
          const code = node.text || node.textContent || "";
          if (code) {
            try {
              (1, eval)(code);
            } catch (e) {
              if (typeof reportError === "function") {
                reportError(e);
              } else if (typeof console !== "undefined" && console.error) {
                console.error(e);
              }
            }
          }
        }
      }
    }
    const kids = node.childNodes || node._children;
    if (kids && kids.length) {
      for (let i = 0; i < kids.length; i++) {
        executeConnectedScripts(kids[i]);
      }
    }
  }

  Node.prototype.appendChild = function (child) {
    if (!this || typeof this !== "object") {
      throw new TypeError("Illegal invocation");
    }
    if (!child || typeof child !== "object" || !isGenuineNode(child)) {
      throw new TypeError("Failed to execute 'appendChild' on 'Node': parameter 1 is not of type 'Node'.");
    }

    // Handle DocumentFragment
    if (child.nodeType === DOCUMENT_FRAGMENT_NODE) {
      const kids = Array.from(child.childNodes || child._children || []);
      for (let i = 0; i < kids.length; i++) {
        this.appendChild(kids[i]);
      }
      if (child._children) child._children.length = 0;
      return child;
    }

    // If child already has a parent or owner fragment, remove from it
    if (child._ownerFragment && typeof child._ownerFragment.removeChild === "function") {
      try {
        child._ownerFragment.removeChild(child);
      } catch (e) {}
      child._ownerFragment = null;
    }
    if (child.parentNode && typeof child.parentNode.removeChild === "function") {
      try {
        child.parentNode.removeChild(child);
      } catch (e) {}
    }

    // If child is a parsed node, adopt it safely into this document
    if (isParsedNode(child)) {
      child.parentNode = this;
      child.ownerDocument = this.ownerDocument || this;
      if (child.nodeType === 1) { // ELEMENT_NODE
        // When adopted, child's subtree in parsed document is not adopted into live tree
        child._children = [];
      }
      ensureChildren(this).push(child);
      executeConnectedScripts(child);
      return child;
    }

    // Handle native element / node
    child.parentNode = this;
    child.ownerDocument = this.ownerDocument || this;
    ensureChildren(this).push(child);

    if (nativeElementAppendChild && typeof nativeElementAppendChild === "function") {
      try {
        nativeElementAppendChild.call(this, child);
      } catch (e) {}
    }

    if (typeof notifyMutation === "function") {
      const rec = new MutationRecord("childList", this);
      rec.addedNodes = [child];
      notifyMutation(rec);
    }

    executeConnectedScripts(child);

    return child;
  };

  Node.prototype.insertBefore = function (newChild, refChild) {
    if (!this || typeof this !== "object") {
      throw new TypeError("Illegal invocation");
    }
    if (!newChild || typeof newChild !== "object" || !isGenuineNode(newChild)) {
      throw new TypeError("Failed to execute 'insertBefore' on 'Node': parameter 1 is not of type 'Node'.");
    }
    if (refChild == null) {
      return this.appendChild(newChild);
    }

    const children = ensureChildren(this);
    const idx = children.indexOf(refChild);
    if (idx === -1 && refChild.parentNode !== this) {
      if (typeof DOMException !== "undefined") {
        throw new DOMException("NotFoundError: Node was not found", "NotFoundError");
      }
      const err = new Error("NotFoundError: Node was not found");
      err.name = "NotFoundError";
      throw err;
    }

    if (newChild.nodeType === DOCUMENT_FRAGMENT_NODE) {
      const kids = Array.from(newChild.childNodes || newChild._children || []);
      for (let i = 0; i < kids.length; i++) {
        this.insertBefore(kids[i], refChild);
      }
      if (newChild._children) newChild._children.length = 0;
      return newChild;
    }

    if (newChild._ownerFragment && typeof newChild._ownerFragment.removeChild === "function") {
      try {
        newChild._ownerFragment.removeChild(newChild);
      } catch (e) {}
      newChild._ownerFragment = null;
    }
    if (newChild.parentNode && typeof newChild.parentNode.removeChild === "function") {
      try {
        newChild.parentNode.removeChild(newChild);
      } catch (e) {}
    }

    newChild.parentNode = this;
    newChild.ownerDocument = this.ownerDocument || this;
    if (isParsedNode(newChild) && newChild.nodeType === 1) {
      newChild._children = [];
    }

    const curIdx = children.indexOf(refChild);
    children.splice(curIdx >= 0 ? curIdx : children.length, 0, newChild);

    if (typeof notifyMutation === "function") {
      const rec = new MutationRecord("childList", this);
      rec.addedNodes = [newChild];
      notifyMutation(rec);
    }

    executeConnectedScripts(newChild);

    return newChild;
  };

  Node.prototype.removeChild = function (child) {
    if (!this || typeof this !== "object") {
      throw new TypeError("Illegal invocation");
    }
    if (!child || typeof child !== "object") {
      throw new TypeError("Failed to execute 'removeChild' on 'Node': parameter 1 is not of type 'Node'.");
    }

    const children = ensureChildren(this);
    const idx = children.indexOf(child);
    if (idx === -1 && child.parentNode !== this) {
      if (typeof DOMException !== "undefined") {
        throw new DOMException("NotFoundError: Node was not found", "NotFoundError");
      }
      const err = new Error("NotFoundError: Node was not found");
      err.name = "NotFoundError";
      throw err;
    }

    if (idx >= 0) {
      children.splice(idx, 1);
    }
    child.parentNode = null;

    if (nativeElementRemoveChild && typeof nativeElementRemoveChild === "function") {
      try {
        nativeElementRemoveChild.call(this, child);
      } catch (e) {}
    }

    if (typeof notifyMutation === "function") {
      const rec = new MutationRecord("childList", this);
      rec.removedNodes = [child];
      notifyMutation(rec);
    }

    return child;
  };

  Object.defineProperty(Node.prototype, "childNodes", {
    get: function () {
      return new NodeList(ensureChildren(this));
    },
    configurable: true,
  });

  Object.defineProperty(Node.prototype, "firstChild", {
    get: function () {
      const c = ensureChildren(this);
      return c.length > 0 ? c[0] : null;
    },
    configurable: true,
  });

  Object.defineProperty(Node.prototype, "lastChild", {
    get: function () {
      const c = ensureChildren(this);
      return c.length > 0 ? c[c.length - 1] : null;
    },
    configurable: true,
  });

  Node.prototype.hasChildNodes = function () {
    return ensureChildren(this).length > 0;
  };

  if (typeof Element !== "undefined" && Element.prototype) {
    Object.defineProperty(Element.prototype, "textContent", {
      get: function () {
        if (this._children && this._children.length > 0) {
          return this._children.map(function (c) {
            return c.textContent || "";
          }).join("");
        }
        if (nativeElementGetTextContent) {
          return nativeElementGetTextContent.call(this);
        }
        return "";
      },
      set: function (v) {
        if (nativeElementSetTextContent) {
          try {
            nativeElementSetTextContent.call(this, v);
          } catch (e) {}
        }
        this._children = [];
        const val = String(v);
        if (val.length > 0) {
          const t = new Text(val);
          t.parentNode = this;
          t.ownerDocument = this.ownerDocument || this;
          this._children.push(t);
        }
      },
      configurable: true,
    });

    Element.prototype.appendChild = Node.prototype.appendChild;
    Element.prototype.insertBefore = Node.prototype.insertBefore;
    Element.prototype.removeChild = Node.prototype.removeChild;

    const nativeElementInnerHtmlDesc = Object.getOwnPropertyDescriptor(Element.prototype, "innerHTML");
    if (nativeElementInnerHtmlDesc && nativeElementInnerHtmlDesc.set) {
      const origSetInnerHtml = nativeElementInnerHtmlDesc.set;
      Object.defineProperty(Element.prototype, "innerHTML", {
        get: nativeElementInnerHtmlDesc.get,
        set: function (v) {
          if (this._children) {
            this._children.length = 0;
          }
          origSetInnerHtml.call(this, v);
          if (typeof processSubtreeTemplates === "function") {
            processSubtreeTemplates(this);
          }
        },
        configurable: true,
        enumerable: nativeElementInnerHtmlDesc.enumerable,
      });
    }
  }

  Node.prototype.lookupNamespaceURI = function (prefix) {
    let p = prefix === "" ? null : prefix;
    if (p === "xml") return "http://www.w3.org/XML/1998/namespace";
    if (p === "xmlns") return "http://www.w3.org/2000/xmlns/";
    if (this.nodeType === 1) { // ELEMENT_NODE
      let n = this;
      while (n && n.nodeType === 1) {
        if (n.prefix === p && n.namespaceURI) return n.namespaceURI;
        const attrs = n.attributes;
        if (attrs) {
          const len = attrs.length || 0;
          for (let i = 0; i < len; i++) {
            const a = attrs[i];
            if (!a) continue;
            const aPrefix = a.prefix || null;
            const aLocal = a.localName || a.name;
            if (p === null) {
              if (a.name === "xmlns" || (aPrefix === null && aLocal === "xmlns")) {
                return a.value || null;
              }
            } else {
              if (aPrefix === "xmlns" && aLocal === p) {
                return a.value || null;
              }
              if (a.name === "xmlns:" + p) {
                return a.value || null;
              }
            }
          }
        }
        n = n.parentElement || n.parentNode;
      }
      return null;
    }
    if (this.nodeType === 9) { // DOCUMENT_NODE
      return this.documentElement ? this.documentElement.lookupNamespaceURI(p) : null;
    }
    if (this.nodeType === 2) { // ATTRIBUTE_NODE
      return this.ownerElement ? this.ownerElement.lookupNamespaceURI(p) : null;
    }
    const parent = this.parentElement || this.parentNode;
    return parent ? parent.lookupNamespaceURI(p) : null;
  };

  markNative(Node, "Node");
  markNative(Node.prototype.contains, "contains");
  markNative(Node.prototype.compareDocumentPosition, "compareDocumentPosition");
  markNative(Node.prototype.cloneNode, "cloneNode");
  markNative(Node.prototype.appendChild, "appendChild");
  markNative(Node.prototype.insertBefore, "insertBefore");
  markNative(Node.prototype.removeChild, "removeChild");
  markNative(Node.prototype.hasChildNodes, "hasChildNodes");
  markNative(Node.prototype.lookupNamespaceURI, "lookupNamespaceURI");
  markNative(Node.prototype.getRootNode, "getRootNode");
  markNative(Node.prototype.normalize, "normalize");

  function Text(data) {
    Node.call(this);
    this._data = String(data !== undefined ? data : "");
    this.nodeType = TEXT_NODE;
    this.nodeName = "#text";
  }
  Text.prototype = Object.create(Node.prototype);
  Text.prototype.constructor = Text;
  Object.defineProperty(Text.prototype, "data", {
    get: function () { return this._data !== undefined ? this._data : ""; },
    set: function (v) {
      this._data = String(v);
      if (typeof notifyMutation === "function") {
        notifyMutation(new MutationRecord("characterData", this));
      }
    },
    configurable: true,
  });
  Object.defineProperty(Text.prototype, "textContent", {
    get: function () { return this.data; },
    set: function (v) { this.data = v; },
    configurable: true,
  });
  Object.defineProperty(Text.prototype, "nodeValue", {
    get: function () { return this.data; },
    set: function (v) { this.data = v; },
    configurable: true,
  });
  Object.defineProperty(Text.prototype, "length", {
    get: function () { return this.data.length; },
    configurable: true,
  });
  markNative(Text, "Text");

  function Comment(data) {
    Node.call(this);
    this.data = String(data !== undefined ? data : "");
    this.nodeType = COMMENT_NODE;
    this.nodeName = "#comment";
  }
  Comment.prototype = Object.create(Node.prototype);
  Comment.prototype.constructor = Comment;
  Object.defineProperty(Comment.prototype, "textContent", {
    get: function () { return this.data; },
    set: function (v) { this.data = String(v); },
    configurable: true,
  });
  Object.defineProperty(Comment.prototype, "nodeValue", {
    get: function () { return this.data; },
    set: function (v) { this.data = String(v); },
    configurable: true,
  });
  Object.defineProperty(Comment.prototype, "length", {
    get: function () { return this.data.length; },
    configurable: true,
  });
  markNative(Comment, "Comment");

  function DocumentFragment() {
    Node.call(this);
    this.nodeType = DOCUMENT_FRAGMENT_NODE;
    this.nodeName = "#document-fragment";
    this._children = [];
  }
  DocumentFragment.prototype = Object.create(Node.prototype);
  DocumentFragment.prototype.constructor = DocumentFragment;
  DocumentFragment.prototype.appendChild = function (child) {
    if (!child || typeof child !== "object") return child;
    if (child._ownerFragment && typeof child._ownerFragment.removeChild === "function") {
      try { child._ownerFragment.removeChild(child); } catch (e) {}
    }
    if (child.parentNode && typeof child.parentNode.removeChild === "function") {
      try { child.parentNode.removeChild(child); } catch (e) {}
    }
    child._ownerFragment = this;
    const res = Node.prototype.appendChild.call(this, child);
    child._ownerFragment = this;
    return res;
  };
  DocumentFragment.prototype.insertBefore = function (newChild, refChild) {
    if (!newChild || typeof newChild !== "object") return newChild;
    if (newChild._ownerFragment && typeof newChild._ownerFragment.removeChild === "function") {
      try { newChild._ownerFragment.removeChild(newChild); } catch (e) {}
    }
    if (newChild.parentNode && typeof newChild.parentNode.removeChild === "function") {
      try { newChild.parentNode.removeChild(newChild); } catch (e) {}
    }
    newChild._ownerFragment = this;
    const res = Node.prototype.insertBefore.call(this, newChild, refChild);
    newChild._ownerFragment = this;
    return res;
  };
  DocumentFragment.prototype.removeChild = function (child) {
    if (!child) return child;
    const kids = this._children || [];
    const idx = kids.indexOf(child);
    if (idx === -1 && child._ownerFragment !== this && child.parentNode !== this) {
      if (typeof DOMException !== "undefined") {
        throw new DOMException("NotFoundError: Node was not found", "NotFoundError");
      }
      const err = new Error("NotFoundError: Node was not found");
      err.name = "NotFoundError";
      throw err;
    }
    if (idx >= 0) kids.splice(idx, 1);
    if (child._ownerFragment === this) child._ownerFragment = null;
    child.parentNode = null;
    return child;
  };
  Object.defineProperty(DocumentFragment.prototype, "childNodes", {
    get: function () {
      return new NodeList(this._children);
    },
    configurable: true,
  });
  markNative(DocumentFragment, "DocumentFragment");

  Object.defineProperty(DocumentFragment.prototype, "innerHTML", {
    get: function () {
      const kids = ensureChildren(this);
      let html = "";
      for (let i = 0; i < kids.length; i++) {
        const k = kids[i];
        if (k.nodeType === 1) html += k.outerHTML || "";
        else if (k.nodeType === 3) html += k.textContent || "";
        else if (k.nodeType === 8) html += "<!--" + (k.data || "") + "-->";
      }
      return html;
    },
    set: function (val) {
      while (this.firstChild) {
        this.removeChild(this.firstChild);
      }
      const doc = this.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
      const temp = doc ? doc.createElement("div") : null;
      if (temp) {
        temp.innerHTML = val;
        while (temp.firstChild) {
          const ch = temp.firstChild;
          try { temp.removeChild(ch); } catch (e) {}
          this.appendChild(ch);
        }
      }
    },
    configurable: true,
  });

  function ShadowRoot(host, mode) {
    DocumentFragment.call(this);
    this.nodeType = DOCUMENT_FRAGMENT_NODE;
    this.nodeName = "#document-fragment";
    this.host = host || null;
    this.mode = mode || "open";
    this._children = [];
  }
  ShadowRoot.prototype = Object.create(DocumentFragment.prototype);
  ShadowRoot.prototype.constructor = ShadowRoot;
  markNative(ShadowRoot, "ShadowRoot");
  global.ShadowRoot = ShadowRoot;
  if (typeof globalThis !== "undefined") {
    globalThis.ShadowRoot = ShadowRoot;
  }

  if (typeof Document !== "undefined" && Document.prototype) {
    if (!Document.prototype.createTextNode) {
      Document.prototype.createTextNode = function (text) {
        return new Text(text);
      };
      markNative(Document.prototype.createTextNode, "createTextNode");
    }
    if (!Document.prototype.createComment) {
      Document.prototype.createComment = function (data) {
        return new Comment(data);
      };
      markNative(Document.prototype.createComment, "createComment");
    }
    if (!Document.prototype.createDocumentFragment) {
      Document.prototype.createDocumentFragment = function () {
        return new DocumentFragment();
      };
      markNative(Document.prototype.createDocumentFragment, "createDocumentFragment");
    }
  }

  // =============================================================
  // Module: selector.js
  // =============================================================
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

  function matchesElementSelector(el, selector) {
    if (!el || el.nodeType !== 1 || !selector) return false;
    const s = selector.trim();
    if (s === "*") return true;

    // If el.matches is available, try it first
    if (typeof el.matches === "function") {
      try {
        return el.matches(s);
      } catch (e) {}
    }

    // Tag only: e.g. "b", "p", "div"
    if (/^[a-zA-Z0-9_-]+$/.test(s)) {
      const elTag = String(el.localName || el.tagName || "").toLowerCase();
      return elTag === s.toLowerCase();
    }

    // ID only: e.g. "#foo"
    if (s.startsWith("#") && !/[\s.>:\[]/.test(s)) {
      const id = s.slice(1);
      return el.id === id || (typeof el.getAttribute === "function" && el.getAttribute("id") === id);
    }

    // Class only: e.g. ".foo"
    if (s.startsWith(".") && !/[\s#>:\[]/.test(s)) {
      const cls = s.slice(1);
      if (el.classList && el.classList.contains(cls)) return true;
      const cName = el.className || (typeof el.getAttribute === "function" && el.getAttribute("class")) || "";
      return cName.split(/\s+/).indexOf(cls) !== -1;
    }

    // Attribute selector: e.g. [attr] or [attr="val"] or [attr=val]
    if (s.startsWith("[") && s.endsWith("]")) {
      const inside = s.slice(1, -1).trim();
      const eqIdx = inside.indexOf("=");
      if (eqIdx === -1) {
        return typeof el.hasAttribute === "function" ? el.hasAttribute(inside) : false;
      }
      const attrName = inside.slice(0, eqIdx).trim();
      let expectedVal = inside.slice(eqIdx + 1).trim();
      if ((expectedVal.startsWith('"') && expectedVal.endsWith('"')) ||
          (expectedVal.startsWith("'") && expectedVal.endsWith("'"))) {
        expectedVal = expectedVal.slice(1, -1);
      }
      const actualVal = typeof el.getAttribute === "function" ? el.getAttribute(attrName) : null;
      return actualVal === expectedVal;
    }

    // Tag + attribute: e.g. "template[shadowrootmode]"
    const bracketIdx = s.indexOf("[");
    if (bracketIdx !== -1 && s.endsWith("]")) {
      const tagPart = s.slice(0, bracketIdx).trim();
      const attrPart = s.slice(bracketIdx);
      return matchesElementSelector(el, tagPart) && matchesElementSelector(el, attrPart);
    }

    // Tag + class: e.g. "p.x"
    const dotIdx = s.indexOf(".");
    if (dotIdx > 0 && !/[\s#>:\[]/.test(s)) {
      const tagPart = s.slice(0, dotIdx);
      const clsPart = s.slice(dotIdx);
      return matchesElementSelector(el, tagPart) && matchesElementSelector(el, clsPart);
    }

    return false;
  }

  function findInSubtree(root, predicate, single) {
    const result = single ? null : [];
    function walk(node) {
      if (!node) return false;
      const kids = node._children || (node.childNodes ? Array.from(node.childNodes) : []);
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

  // =============================================================
  // Module: element/element.js
  // =============================================================
  // src/js/shim/element/element.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/element.ts

  const STYLE_DECL_MEMBERS = new Set([
    "cssText", "length", "parentRule", "item", "setProperty",
    "getPropertyValue", "getPropertyPriority", "removeProperty"
  ]);

  function createStyleProxy(element) {
    function parseStyleMap() {
      const raw = element.getAttribute("style") || "";
      const map = new Map();
      const parts = raw.split(";");
      for (let i = 0; i < parts.length; i++) {
        const p = parts[i].trim();
        if (!p) continue;
        const colon = p.indexOf(":");
        if (colon !== -1) {
          const rawKey = p.slice(0, colon).trim();
          const k = rawKey.startsWith("--") ? rawKey : rawKey.toLowerCase();
          let v = p.slice(colon + 1).trim();
          let pri = "";
          if (/!important\s*$/i.test(v)) {
            pri = "important";
            v = v.replace(/\s*!important\s*$/i, "").trim();
          }
          map.set(k, { name: rawKey, value: v, priority: pri });
        }
      }
      return map;
    }

    function serializeStyleMap(map) {
      const arr = [];
      for (const [k, entry] of map.entries()) {
        let s = entry.name + ": " + entry.value;
        if (entry.priority) s += " !" + entry.priority;
        arr.push(s);
      }
      const res = arr.join("; ");
      if (res.length > 0) {
        element.setAttribute("style", res);
      } else {
        element.removeAttribute("style");
      }
    }

    const declTarget = Object.create(
      (typeof global !== "undefined" && global.CSSStyleDeclaration && global.CSSStyleDeclaration.prototype) ||
      (typeof globalThis !== "undefined" && globalThis.CSSStyleDeclaration && globalThis.CSSStyleDeclaration.prototype) ||
      Object.prototype
    );

    return new Proxy(declTarget, {
      get: function (target, prop, receiver) {
        if (typeof prop === "symbol") {
          if (prop === Symbol.iterator) {
            return function* () {
              const map = parseStyleMap();
              for (const entry of map.values()) yield entry.name;
            };
          }
          return target[prop];
        }
        if (typeof prop !== "string") return undefined;
        if (prop === "cssText") return element.getAttribute("style") || "";
        if (prop === "length") return parseStyleMap().size;
        if (prop === "parentRule") return null;
        if (prop === "item") {
          return function (idx) {
            const i = Number(idx) >>> 0;
            const map = parseStyleMap();
            let count = 0;
            for (const entry of map.values()) {
              if (count === i) return entry.name;
              count++;
            }
            return "";
          };
        }
        if (prop === "getPropertyValue") {
          return function (p) {
            const map = parseStyleMap();
            const str = String(p);
            const key = str.startsWith("--") ? str : str.toLowerCase();
            const entry = map.get(key);
            return entry ? entry.value : "";
          };
        }
        if (prop === "getPropertyPriority") {
          return function (p) {
            const map = parseStyleMap();
            const str = String(p);
            const key = str.startsWith("--") ? str : str.toLowerCase();
            const entry = map.get(key);
            return entry ? entry.priority : "";
          };
        }
        if (prop === "setProperty") {
          return function (p, val, priority) {
            if (priority !== undefined && priority !== null && priority !== "") {
              if (priority !== "important" && priority !== "!important") return;
            }
            const pPriority = (priority === "important" || priority === "!important") ? "important" : "";
            const map = parseStyleMap();
            const str = String(p);
            const key = str.startsWith("--") ? str : str.toLowerCase();
            map.set(key, { name: str, value: String(val), priority: pPriority });
            serializeStyleMap(map);
          };
        }
        if (prop === "removeProperty") {
          return function (p) {
            const map = parseStyleMap();
            const str = String(p);
            const key = str.startsWith("--") ? str : str.toLowerCase();
            const old = map.get(key);
            map.delete(key);
            serializeStyleMap(map);
            return old ? old.value : "";
          };
        }
        if (prop in target) return target[prop];

        // CamelCase or kebab-case lookup
        const map = parseStyleMap();
        if (map.has(prop)) return map.get(prop).value;
        const kebab = prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
        if (map.has(kebab)) return map.get(kebab).value;
        let vendorKebab = kebab;
        if (prop.startsWith("webkit")) vendorKebab = "-webkit-" + prop.slice(6).replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
        else if (prop.startsWith("Moz")) vendorKebab = "-moz-" + prop.slice(3).replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
        if (map.has(vendorKebab)) return map.get(vendorKebab).value;
        return "";
      },
      set: function (target, prop, value) {
        if (typeof prop === "symbol") {
          target[prop] = value;
          return true;
        }
        if (typeof prop !== "string") return true;
        if (prop === "cssText") {
          element.setAttribute("style", String(value));
          return true;
        }
        let key = prop;
        if (!prop.startsWith("--")) {
          if (prop.startsWith("webkit")) key = "-webkit-" + prop.slice(6).replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
          else if (prop.startsWith("Moz")) key = "-moz-" + prop.slice(3).replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
          else key = prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
        }
        const map = parseStyleMap();
        if (value === null || value === undefined || value === "") {
          map.delete(key.startsWith("--") ? key : key.toLowerCase());
        } else {
          map.set(key.startsWith("--") ? key : key.toLowerCase(), { name: key, value: String(value), priority: "" });
        }
        serializeStyleMap(map);
        return true;
      },
      has: function (target, prop) {
        if (typeof prop === "symbol") return prop === Symbol.iterator || prop in target;
        const pStr = String(prop);
        if (STYLE_DECL_MEMBERS.has(pStr)) return true;
        if (prop in target) return true;
        if (pStr.startsWith("--")) return false;
        if (/^(?:webkit|moz|ms|o)-/.test(pStr)) return false;
        if (!pStr.includes("-") && /^(?:moz|ms|o)[A-Z]/.test(pStr)) return false;
        if (typeof global !== "undefined" && typeof global.__isCssPropertySupported === "function") {
          return global.__isCssPropertySupported(pStr);
        }
        if (typeof globalThis !== "undefined" && typeof globalThis.__isCssPropertySupported === "function") {
          return globalThis.__isCssPropertySupported(pStr);
        }
        return false;
      }
    });
  }

  function isHtmlElement(el) {
    if (!el) return true;
    if (el._namespaceURI !== undefined) {
      return el._namespaceURI === null || el._namespaceURI === "http://www.w3.org/1999/xhtml";
    }
    let curr = el;
    while (curr) {
      if (curr._namespaceURI !== undefined) {
        return curr._namespaceURI === null || curr._namespaceURI === "http://www.w3.org/1999/xhtml";
      }
      const tag = String(curr.localName || curr.tagName || "").toLowerCase();
      if (tag === "svg" || tag === "math") return false;
      curr = curr.parentElement || curr.parentNode;
    }
    return true;
  }

  function installElementExtensions(proto) {
    if (!proto) return;

    // Make Element inherit from Node / EventTarget
    Object.setPrototypeOf(proto, Node.prototype);
    proto.appendChild = Node.prototype.appendChild;
    proto.insertBefore = Node.prototype.insertBefore;
    proto.removeChild = Node.prototype.removeChild;

    if (proto.nodeType === undefined) {
      Object.defineProperty(proto, "nodeType", {
        value: ELEMENT_NODE,
        writable: false,
        configurable: true,
      });
    }

    Object.defineProperty(proto, "nodeName", {
      get: function () {
        return this.tagName || "";
      },
      configurable: true,
    });

    Object.defineProperty(proto, "localName", {
      get: function () {
        return (this.tagName || "").toLowerCase();
      },
      configurable: true,
    });

    Object.defineProperty(proto, "prefix", {
      get: function () {
        if (this._prefix !== undefined) return this._prefix;
        const tag = this.tagName || "";
        const colon = tag.indexOf(":");
        return colon !== -1 ? tag.slice(0, colon) : null;
      },
      configurable: true,
    });

    Object.defineProperty(proto, "namespaceURI", {
      get: function () {
        if (this._namespaceURI !== undefined) return this._namespaceURI;
        if (this.getAttribute) {
          const xmlns = this.getAttribute("xmlns");
          if (xmlns !== null && xmlns !== undefined && xmlns !== "") return xmlns;
        }
        let curr = this;
        while (curr) {
          const tag = String(curr.localName || curr.tagName || "").toLowerCase();
          if (tag === "svg") return "http://www.w3.org/2000/svg";
          if (tag === "math") return "http://www.w3.org/1998/Math/MathML";
          curr = curr.parentElement || curr.parentNode;
        }
        const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
        if (!doc || !doc.contentType || doc.contentType === "text/html") {
          return "http://www.w3.org/1999/xhtml";
        }
        return null;
      },
      set: function (v) {
        this._namespaceURI = v;
      },
      configurable: true,
    });

    Object.defineProperty(proto, "data", {
      get: function () { return this.textContent || ""; },
      set: function (v) {
        this.textContent = String(v);
        if (typeof notifyMutation === "function") {
          notifyMutation(new MutationRecord("characterData", this));
        }
      },
      configurable: true,
    });

    if (!proto.lookupNamespaceURI) {
      proto.lookupNamespaceURI = Node.prototype.lookupNamespaceURI;
    }

    if (!proto.append) {
      proto.append = function (...nodes) {
        for (const n of nodes) {
          let node;
          if (n && typeof n === "object" && typeof n.nodeType === "number") {
            node = n;
          } else if (typeof document !== "undefined" && document.createTextNode) {
            node = document.createTextNode(String(n));
          } else {
            node = new Text(String(n));
          }
          this.appendChild(node);
        }
      };
      markNative(proto.append, "append");
    }

    if (!proto.remove) {
      proto.remove = function () {
        const parent = this.parentElement || this.parentNode;
        if (parent && parent.removeChild) {
          parent.removeChild(this);
        }
      };
      markNative(proto.remove, "remove");
    }

    // Children and Element Traversal ([SameObject] semantics)
    Object.defineProperty(proto, "children", {
      get: function () {
        return getElementChildrenCollection(this);
      },
      configurable: true,
    });

    Object.defineProperty(proto, "firstElementChild", {
      get: function () {
        const kids = this.children;
        return (kids && kids.length > 0) ? kids[0] : null;
      },
      configurable: true,
    });

    Object.defineProperty(proto, "lastElementChild", {
      get: function () {
        const kids = this.children;
        return (kids && kids.length > 0) ? kids[kids.length - 1] : null;
      },
      configurable: true,
    });

    Object.defineProperty(proto, "childElementCount", {
      get: function () {
        const kids = this.children;
        return kids ? kids.length : 0;
      },
      configurable: true,
    });

    Object.defineProperty(proto, "previousElementSibling", {
      get: function () {
        let sib = this.previousSibling;
        while (sib) {
          if (sib.nodeType === 1) return sib;
          sib = sib.previousSibling;
        }
        return null;
      },
      configurable: true,
    });

    Object.defineProperty(proto, "nextElementSibling", {
      get: function () {
        let sib = this.nextSibling;
        while (sib) {
          if (sib.nodeType === 1) return sib;
          sib = sib.nextSibling;
        }
        return null;
      },
      configurable: true,
    });

    // classList getter
    Object.defineProperty(proto, "classList", {
      get: function () {
        if (!this._classList) {
          const self = this;
          this._classList = new DOMTokenList(
            function () { return self.className || ""; },
            function (val) { self.className = val; }
          );
        }
        return this._classList;
      },
      configurable: true,
    });

    // style getter/setter
    Object.defineProperty(proto, "style", {
      get: function () {
        if (!this._styleProxy) {
          this._styleProxy = createStyleProxy(this);
        }
        return this._styleProxy;
      },
      set: function (val) {
        if (typeof val === "string") {
          this.setAttribute("style", val);
        }
      },
      configurable: true,
    });

    // Internal attribute synchronization
    proto._syncAttrs = function () {
      if (!this._attrs) {
        this._attrs = new Map();
        this._attrList = [];
        const isHtml = isHtmlElement(this);
        if (typeof this._getRawAttrs === "function") {
          const raw = this._getRawAttrs();
          if (raw && Array.isArray(raw)) {
            for (let i = 0; i < raw.length; i++) {
              const item = raw[i];
              if (item && item.name) {
                const colon = item.name.indexOf(":");
                const prefix = colon !== -1 ? item.name.slice(0, colon) : null;
                const localName = colon !== -1 ? item.name.slice(colon + 1) : item.name;
                let ns = null;
                if (prefix === "xml") ns = "http://www.w3.org/XML/1998/namespace";
                else if (prefix === "xmlns" || item.name === "xmlns") ns = "http://www.w3.org/2000/xmlns/";
                else if (prefix === "xlink") ns = "http://www.w3.org/1999/xlink";
                const entry = {
                  name: item.name,
                  value: item.value || "",
                  namespaceURI: ns,
                  localName: localName,
                  prefix: prefix,
                };
                this._attrList.push(entry);
                const mapKey = isHtml ? item.name.toLowerCase() : item.name;
                this._attrs.set(mapKey, entry);
              }
            }
          }
        }
        if (this.id && !this._attrs.has("id")) {
          const entry = { name: "id", value: this.id, namespaceURI: null, localName: "id", prefix: null };
          this._attrList.push(entry);
          this._attrs.set("id", entry);
        }
        if (this.className && typeof this.className === "string" && !this._attrs.has("class")) {
          const entry = { name: "class", value: this.className, namespaceURI: null, localName: "class", prefix: null };
          this._attrList.push(entry);
          this._attrs.set("class", entry);
        }
        if (this.hasAttribute && this.hasAttribute("style") && !this._attrs.has("style")) {
          const entry = { name: "style", value: this.getAttribute("style") || "", namespaceURI: null, localName: "style", prefix: null };
          this._attrList.push(entry);
          this._attrs.set("style", entry);
        }
      }
    };

    // attributes getter returning live NamedNodeMap
    Object.defineProperty(proto, "attributes", {
      get: function () {
        if (!this._attributesMap) {
          this._attributesMap = new NamedNodeMap(this);
        } else {
          this._attributesMap._sync();
        }
        return this._attributesMap;
      },
      configurable: true,
    });

    // setAttribute
    const origSetAttribute = proto.setAttribute;
    proto.setAttribute = function (name, value) {
      const isHtml = isHtmlElement(this);
      const key = isHtml ? String(name).toLowerCase() : String(name);
      const val = String(value);
      if (!this._attrs) this._syncAttrs();
      let existing = null;
      if (this._attrList) {
        for (let i = 0; i < this._attrList.length; i++) {
          const match = isHtml
            ? this._attrList[i].name.toLowerCase() === key
            : this._attrList[i].name === key;
          if (match) {
            existing = this._attrList[i];
            break;
          }
        }
      }
      const oldVal = existing ? existing.value : (this._attrs.get(key)?.value ?? null);
      if (existing) {
        existing.value = val;
      } else {
        const colon = key.indexOf(":");
        const prefix = colon !== -1 ? key.slice(0, colon) : null;
        const localName = colon !== -1 ? key.slice(colon + 1) : key;
        let ns = null;
        if (prefix === "xml") ns = "http://www.w3.org/XML/1998/namespace";
        else if (prefix === "xmlns" || key === "xmlns") ns = "http://www.w3.org/2000/xmlns/";
        else if (prefix === "xlink") ns = "http://www.w3.org/1999/xlink";
        const entry = {
          name: key,
          value: val,
          namespaceURI: ns,
          localName: localName,
          prefix: prefix,
        };
        if (!this._attrList) this._attrList = [];
        this._attrList.push(entry);
      }
      this._attrs.set(key, { name: key, value: val });
      if (key.toLowerCase() === "async") {
        if (typeof globalThis !== "undefined" && globalThis.__capsurf_clearScriptForceAsync) {
          globalThis.__capsurf_clearScriptForceAsync(this);
        }
      }
      if (key.toLowerCase() === "id") {
        this.id = val;
      } else if (key.toLowerCase() === "class" && typeof this.className === "string") {
        this.className = val;
      } else if (key.toLowerCase() === "selected" && (this.tagName || "").toLowerCase() === "option") {
        this._selected = true;
        let curr = this.parentElement || this.parentNode;
        while (curr) {
          if ((curr.tagName || "").toLowerCase() === "select") {
            if (!curr.multiple) {
              curr._selectedIndex = undefined;
              const opts = curr.options;
              for (let i = 0; i < opts.length; i++) {
                if (opts[i] !== this && (!this._nid || opts[i]._nid !== this._nid)) {
                  opts[i]._selected = false;
                }
              }
            }
            break;
          }
          curr = curr.parentElement || curr.parentNode;
        }
      }
      if (origSetAttribute) {
        origSetAttribute.call(this, key, val);
      }
      if (this._attributesMap) {
        this._attributesMap._sync();
      }
      if (typeof notifyMutation === "function") {
        const rec = new MutationRecord("attributes", this);
        rec.attributeName = key;
        rec.oldValue = oldVal;
        notifyMutation(rec);
      }
      if (typeof notifyCustomElementAttributeChanged === "function") {
        notifyCustomElementAttributeChanged(this, key, oldVal, val, null);
      }
    };
    markNative(proto.setAttribute, "setAttribute");

    // removeAttribute
    const origRemoveAttribute = proto.removeAttribute;
    proto.removeAttribute = function (name) {
      const isHtml = isHtmlElement(this);
      const key = isHtml ? String(name).toLowerCase() : String(name);
      if (!this._attrs) this._syncAttrs();
      let oldVal = null;
      if (this._attrList) {
        for (let i = 0; i < this._attrList.length; i++) {
          const match = isHtml
            ? this._attrList[i].name.toLowerCase() === key
            : this._attrList[i].name === key;
          if (match) {
            oldVal = this._attrList[i].value;
            this._attrList.splice(i, 1);
            break;
          }
        }
      }
      const oldEntry = this._attrs.get(key);
      if (!oldVal && oldEntry) oldVal = oldEntry.value;
      this._attrs.delete(key);
      if (key.toLowerCase() === "async") {
        if (typeof globalThis !== "undefined" && globalThis.__capsurf_clearScriptForceAsync) {
          globalThis.__capsurf_clearScriptForceAsync(this);
        }
      }
      if (key.toLowerCase() === "id") {
        this.id = "";
      } else if (key.toLowerCase() === "class" && typeof this.className === "string") {
        this.className = "";
      } else if (key.toLowerCase() === "selected" && (this.tagName || "").toLowerCase() === "option") {
        this._selected = false;
        let curr = this.parentElement || this.parentNode;
        while (curr) {
          if ((curr.tagName || "").toLowerCase() === "select") {
            if (!curr.multiple) {
              curr._selectedIndex = undefined;
            }
            break;
          }
          curr = curr.parentElement || curr.parentNode;
        }
      }

      if (origRemoveAttribute) {
        origRemoveAttribute.call(this, name);
      }
      if (this._attributesMap) {
        this._attributesMap._sync();
      }
      if (typeof notifyMutation === "function") {
        const rec = new MutationRecord("attributes", this);
        rec.attributeName = name;
        rec.oldValue = oldVal;
        notifyMutation(rec);
      }
      if (typeof notifyCustomElementAttributeChanged === "function") {
        notifyCustomElementAttributeChanged(this, key, oldVal, null, null);
      }
    };
    markNative(proto.removeAttribute, "removeAttribute");

    // removeAttributeNode
    proto.removeAttributeNode = function (attr) {
      if (!attr || !attr.name) return null;
      if (attr.ownerElement && attr.ownerElement !== this) {
        const err = new Error("NotFoundError: Attr does not belong to this element");
        err.name = "NotFoundError";
        throw err;
      }
      this.removeAttribute(attr.name);
      attr.ownerElement = null;
      return attr;
    };
    markNative(proto.removeAttributeNode, "removeAttributeNode");

    // getAttributeNode & setAttributeNode
    proto.getAttributeNode = function (name) {
      if (!this._attributesMap) this._attributesMap = new NamedNodeMap(this);
      return this._attributesMap.getNamedItem(name);
    };
    markNative(proto.getAttributeNode, "getAttributeNode");

    proto.getAttributeNodeNS = function (namespace, localName) {
      if (!this._attributesMap) this._attributesMap = new NamedNodeMap(this);
      return this._attributesMap.getNamedItemNS(namespace, localName);
    };
    markNative(proto.getAttributeNodeNS, "getAttributeNodeNS");

    proto.setAttributeNode = function (attr) {
      if (!attr || !attr.name) throw new TypeError("Attr required");
      if (!this._attributesMap) this._attributesMap = new NamedNodeMap(this);
      return this._attributesMap.setNamedItem(attr);
    };
    markNative(proto.setAttributeNode, "setAttributeNode");

    // getAttribute
    const origGetAttribute = proto.getAttribute;
    proto.getAttribute = function (name) {
      const isHtml = isHtmlElement(this);
      const key = isHtml ? String(name).toLowerCase() : String(name);
      if (!this._attrs) this._syncAttrs();
      if (this._attrList) {
        for (let i = 0; i < this._attrList.length; i++) {
          const match = isHtml
            ? this._attrList[i].name.toLowerCase() === key
            : this._attrList[i].name === key;
          if (match) {
            return this._attrList[i].value;
          }
        }
      }
      if (this._attrs) {
        const entry = this._attrs.get(key);
        if (entry) return entry.value;
        if (isHtml && origGetAttribute) {
          const val = origGetAttribute.call(this, name);
          if (val !== null) return val;
        }
        return null;
      }
      if (isHtml && origGetAttribute) {
        return origGetAttribute.call(this, name);
      }
      return null;
    };
    markNative(proto.getAttribute, "getAttribute");

    // hasAttribute
    const origHasAttribute = proto.hasAttribute;
    proto.hasAttribute = function (name) {
      const isHtml = isHtmlElement(this);
      const key = isHtml ? String(name).toLowerCase() : String(name);
      if (!this._attrs) this._syncAttrs();
      if (this._attrList) {
        for (let i = 0; i < this._attrList.length; i++) {
          const match = isHtml
            ? this._attrList[i].name.toLowerCase() === key
            : this._attrList[i].name === key;
          if (match) {
            return true;
          }
        }
      }
      if (this._attrs) {
        if (this._attrs.has(key)) return true;
        if (isHtml && origHasAttribute) return origHasAttribute.call(this, name);
        return false;
      }
      if (isHtml && origHasAttribute) {
        return origHasAttribute.call(this, name);
      }
      return false;
    };
    markNative(proto.hasAttribute, "hasAttribute");

    // getAttributeNames
    proto.getAttributeNames = function () {
      if (!this._attrs) this._syncAttrs();
      const names = [];
      if (this._attrList) {
        for (let i = 0; i < this._attrList.length; i++) {
          names.push(this._attrList[i].name);
        }
      } else {
        for (const entry of this._attrs.values()) {
          names.push(entry.name);
        }
      }
      return names;
    };
    markNative(proto.getAttributeNames, "getAttributeNames");

    // getAttributeNS
    proto.getAttributeNS = function (namespace, localName) {
      if (typeof localName !== "string") return null;
      if (!this._attrs) this._syncAttrs();
      const ns = (namespace === undefined || namespace === null || namespace === "") ? null : String(namespace);
      const loc = String(localName).toLowerCase();
      if (this._attrList) {
        for (let i = 0; i < this._attrList.length; i++) {
          const item = this._attrList[i];
          const itemNs = item.namespaceURI || null;
          const itemLoc = (item.localName || item.name).toLowerCase();
          if (itemNs === ns && itemLoc === loc) {
            return item.value;
          }
        }
        if (ns === null) {
          for (let i = 0; i < this._attrList.length; i++) {
            const item = this._attrList[i];
            if ((item.namespaceURI === null || item.namespaceURI === undefined) &&
                item.name.toLowerCase() === loc) {
              return item.value;
            }
          }
        }
        return null;
      }
      return null;
    };
    markNative(proto.getAttributeNS, "getAttributeNS");

    // hasAttributeNS
    proto.hasAttributeNS = function (namespace, localName) {
      return this.getAttributeNS(namespace, localName) !== null;
    };
    markNative(proto.hasAttributeNS, "hasAttributeNS");

    // setAttributeNS
    proto.setAttributeNS = function (namespace, qualifiedName, value) {
      const qname = String(qualifiedName);
      const val = String(value);
      const ns = (namespace === undefined || namespace === null || namespace === "") ? null : String(namespace);
      const colon = qname.indexOf(":");
      const prefix = colon !== -1 ? qname.slice(0, colon) : null;
      const localName = colon !== -1 ? qname.slice(colon + 1) : qname;

      if (!this._attrs) this._syncAttrs();

      let existing = null;
      if (this._attrList) {
        for (let i = 0; i < this._attrList.length; i++) {
          const item = this._attrList[i];
          const itemNs = item.namespaceURI || null;
          const itemLoc = (item.localName || item.name).toLowerCase();
          if (itemNs === ns && itemLoc === localName.toLowerCase()) {
            existing = item;
            break;
          }
        }
      }

      const oldVal = existing ? existing.value : null;

      if (existing) {
        existing.value = val;
        existing.name = qname;
        existing.prefix = prefix;
      } else {
        const newEntry = {
          name: qname,
          value: val,
          namespaceURI: ns,
          localName: localName,
          prefix: prefix,
        };
        if (!this._attrList) this._attrList = [];
        this._attrList.push(newEntry);
      }

      this._attrs.set(qname.toLowerCase(), {
        name: qname,
        value: val,
        namespaceURI: ns,
        localName: localName,
        prefix: prefix,
      });

      if (qname.toLowerCase() === "async" || (localName && String(localName).toLowerCase() === "async")) {
        if (typeof globalThis !== "undefined" && globalThis.__capsurf_clearScriptForceAsync) {
          globalThis.__capsurf_clearScriptForceAsync(this);
        }
      }

      if (qname.toLowerCase() === "id" && ns === null) {
        this.id = val;
      } else if (qname.toLowerCase() === "class" && ns === null) {
        this.className = val;
      }

      if (origSetAttribute) {
        origSetAttribute.call(this, qname, val);
      }
      if (this._attributesMap) {
        this._attributesMap._sync();
      }
      if (typeof notifyMutation === "function") {
        const rec = new MutationRecord("attributes", this);
        rec.attributeName = qname;
        rec.attributeNamespace = ns;
        rec.oldValue = oldVal;
        notifyMutation(rec);
      }
      if (typeof notifyCustomElementAttributeChanged === "function") {
        notifyCustomElementAttributeChanged(this, localName, oldVal, val, ns);
      }
    };
    markNative(proto.setAttributeNS, "setAttributeNS");

    // removeAttributeNS
    proto.removeAttributeNS = function (namespace, localName) {
      if (typeof localName !== "string" || localName.length === 0) return;
      if (!this._attrs) this._syncAttrs();
      const ns = (namespace === undefined || namespace === null || namespace === "") ? null : String(namespace);
      const loc = String(localName).toLowerCase();

      let removed = null;
      if (this._attrList) {
        for (let i = 0; i < this._attrList.length; i++) {
          const item = this._attrList[i];
          const itemNs = item.namespaceURI || null;
          const itemLoc = (item.localName || item.name).toLowerCase();
          if (itemNs === ns && itemLoc === loc) {
            removed = item;
            this._attrList.splice(i, 1);
            break;
          }
        }
        if (!removed && ns === null) {
          for (let i = 0; i < this._attrList.length; i++) {
            const item = this._attrList[i];
            if ((item.namespaceURI === null || item.namespaceURI === undefined) &&
                item.name.toLowerCase() === loc) {
              removed = item;
              this._attrList.splice(i, 1);
              break;
            }
          }
        }
      }

      const oldVal = removed ? removed.value : null;
      const removedName = removed ? removed.name : localName;

      this._attrs.delete(removedName.toLowerCase());
      this._attrs.delete(localName.toLowerCase());

      if (removedName.toLowerCase() === "id" && ns === null) {
        this.id = "";
      } else if (removedName.toLowerCase() === "class" && ns === null) {
        this.className = "";
      }

      if (loc === "async") {
        if (typeof globalThis !== "undefined" && globalThis.__capsurf_clearScriptForceAsync) {
          globalThis.__capsurf_clearScriptForceAsync(this);
        }
      }

      if (origRemoveAttribute) {
        origRemoveAttribute.call(this, removedName);
      }
      if (this._attributesMap) {
        this._attributesMap._sync();
      }
      if (typeof notifyMutation === "function") {
        const rec = new MutationRecord("attributes", this);
        rec.attributeName = removedName;
        rec.attributeNamespace = ns;
        rec.oldValue = oldVal;
        notifyMutation(rec);
      }
      if (typeof notifyCustomElementAttributeChanged === "function") {
        notifyCustomElementAttributeChanged(this, loc, oldVal, null, ns);
      }
    };
    markNative(proto.removeAttributeNS, "removeAttributeNS");

    // Wrap id & className accessors if present
    const idDesc = Object.getOwnPropertyDescriptor(proto, "id");
    if (idDesc && idDesc.set) {
      const origIdSet = idDesc.set;
      Object.defineProperty(proto, "id", {
        get: idDesc.get,
        set: function (v) {
          origIdSet.call(this, v);
          const str = String(v);
          if (!this._attrs) this._syncAttrs();
          if (str) {
            const entry = { name: "id", value: str, namespaceURI: null, localName: "id", prefix: null };
            this._attrs.set("id", entry);
            if (this._attrList) {
              let found = false;
              for (let i = 0; i < this._attrList.length; i++) {
                if (this._attrList[i].name.toLowerCase() === "id") {
                  this._attrList[i].value = str;
                  found = true;
                  break;
                }
              }
              if (!found) this._attrList.push(entry);
            }
          } else {
            this._attrs.delete("id");
            if (this._attrList) {
              for (let i = 0; i < this._attrList.length; i++) {
                if (this._attrList[i].name.toLowerCase() === "id") {
                  this._attrList.splice(i, 1);
                  break;
                }
              }
            }
          }
          if (this._attributesMap) this._attributesMap._sync();
        },
        configurable: true,
      });
    }
    const clsDesc = Object.getOwnPropertyDescriptor(proto, "className");
    const origClsGet = clsDesc ? clsDesc.get : null;
    const origClsSet = clsDesc ? clsDesc.set : null;
    Object.defineProperty(proto, "className", {
      get: function () {
        if (!isHtmlElement(this)) {
          if (!this._svgClassName) {
            const animStr = Object.create(SVGAnimatedString.prototype);
            animStr._element = this;
            this._svgClassName = animStr;
          }
          return this._svgClassName;
        }
        if (origClsGet) return origClsGet.call(this);
        return this.getAttribute("class") || "";
      },
      set: function (v) {
        if (!isHtmlElement(this) && typeof v === "object" && v !== null && v.baseVal !== undefined) {
          v = v.baseVal;
        }
        if (origClsSet) {
          origClsSet.call(this, v);
        }
        const str = String(v);
        if (!this._attrs) this._syncAttrs();
        if (str) {
          const entry = { name: "class", value: str, namespaceURI: null, localName: "class", prefix: null };
          this._attrs.set("class", entry);
          if (this._attrList) {
            let found = false;
            for (let i = 0; i < this._attrList.length; i++) {
              if (this._attrList[i].name.toLowerCase() === "class") {
                this._attrList[i].value = str;
                found = true;
                break;
              }
            }
            if (!found) this._attrList.push(entry);
          }
        } else {
          this._attrs.delete("class");
          if (this._attrList) {
            for (let i = 0; i < this._attrList.length; i++) {
              if (this._attrList[i].name.toLowerCase() === "class") {
                this._attrList.splice(i, 1);
                break;
              }
            }
          }
        }
        if (this._attributesMap) this._attributesMap._sync();
      },
      configurable: true,
    });

    // toggleAttribute
    if (!proto.toggleAttribute) {
      proto.toggleAttribute = function (name, force) {
        const has = this.hasAttribute(name);
        if (force === true) {
          if (!has) this.setAttribute(name, "");
          return true;
        } else if (force === false) {
          if (has) this.removeAttribute(name);
          return false;
        } else {
          if (has) {
            this.removeAttribute(name);
            return false;
          } else {
            this.setAttribute(name, "");
            return true;
          }
        }
      };
      markNative(proto.toggleAttribute, "toggleAttribute");
    }

    // matches & closest
    if (!proto.matches) {
      proto.matches = function (selector) {
        if (!selector || typeof selector !== "string") return false;
        const s = selector.trim();
        if (s === ":checked") return Boolean(this.checked);
        if (s === ":disabled") return Boolean(this.disabled);
        if (s === ":enabled") return !this.disabled;
        if (s === ":focus") return Boolean(typeof document !== "undefined" && document.activeElement === this);
        if (s.endsWith(":focus")) {
          const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
          if (!doc || doc.activeElement !== this) return false;
          const base = s.slice(0, -6).trim();
          if (!base || base === "*") return true;
          return this.matches(base);
        }
        if (s === "*") return true;
        if (s.startsWith("#") && this.id === s.slice(1)) return true;
        if (s.startsWith(".") && this.classList.contains(s.slice(1))) return true;
        if (s.toLowerCase() === this.localName) return true;
        if (typeof document !== "undefined" && document.querySelectorAll) {
          const matches = document.querySelectorAll(s);
          for (let i = 0; i < matches.length; i++) {
            if (matches[i] === this) return true;
          }
        }
        return false;
      };
      markNative(proto.matches, "matches");
    }

    if (!proto.closest) {
      proto.closest = function (selector) {
        let curr = this;
        while (curr && curr.nodeType === ELEMENT_NODE) {
          if (curr.matches && curr.matches(selector)) return curr;
          curr = curr.parentElement;
        }
        return null;
      };
      markNative(proto.closest, "closest");
    }

    // getClientRects
    if (!proto.getClientRects) {
      proto.getClientRects = function () {
        const rect = this.getBoundingClientRect();
        return [rect];
      };
      markNative(proto.getClientRects, "getClientRects");
    }

    // Scroll and Box Geometry properties
    Object.defineProperty(proto, "scrollTop", {
      get: function () { return this._scrollTop || 0; },
      set: function (v) { this._scrollTop = Math.max(0, Number(v) || 0); },
      configurable: true,
    });

    Object.defineProperty(proto, "scrollLeft", {
      get: function () { return this._scrollLeft || 0; },
      set: function (v) { this._scrollLeft = Math.max(0, Number(v) || 0); },
      configurable: true,
    });

    const origGetBoundingClientRect = proto.getBoundingClientRect;
    proto.getBoundingClientRect = function () {
      const r = origGetBoundingClientRect ? origGetBoundingClientRect.call(this) : { x: 0, y: 0, width: 0, height: 0, left: 0, top: 0, right: 0, bottom: 0 };
      const winX = (typeof window !== "undefined" && (window.scrollX || window.pageXOffset)) || 0;
      const winY = (typeof window !== "undefined" && (window.scrollY || window.pageYOffset)) || 0;
      let scrollOffsetParentX = 0;
      let scrollOffsetParentY = 0;
      let cur = this.parentElement;
      while (cur && cur !== document.body && cur !== document.documentElement) {
        if (cur._scrollTop) scrollOffsetParentY += cur._scrollTop;
        if (cur._scrollLeft) scrollOffsetParentX += cur._scrollLeft;
        cur = cur.parentElement;
      }
      const x = r.x - winX - scrollOffsetParentX;
      const y = r.y - winY - scrollOffsetParentY;
      const w = r.width;
      const h = r.height;
      return {
        x: x,
        y: y,
        left: x,
        top: y,
        width: w,
        height: h,
        right: x + w,
        bottom: y + h,
      };
    };
    markNative(proto.getBoundingClientRect, "getBoundingClientRect");

    Object.defineProperty(proto, "clientHeight", {
      get: function () {
        if (this === document.documentElement || this === document.body) {
          return (typeof window !== "undefined" && window.innerHeight) || 720;
        }
        const r = origGetBoundingClientRect ? origGetBoundingClientRect.call(this) : this.getBoundingClientRect();
        return Math.round(r.height);
      },
      configurable: true,
    });

    Object.defineProperty(proto, "clientWidth", {
      get: function () {
        if (this === document.documentElement || this === document.body) {
          return (typeof window !== "undefined" && window.innerWidth) || 1280;
        }
        const r = origGetBoundingClientRect ? origGetBoundingClientRect.call(this) : this.getBoundingClientRect();
        return Math.round(r.width);
      },
      configurable: true,
    });

    Object.defineProperty(proto, "scrollHeight", {
      get: function () {
        let maxBottom = this.clientHeight;
        const all = this.children || [];
        for (let i = 0; i < all.length; i++) {
          const cr = all[i].getBoundingClientRect ? all[i].getBoundingClientRect() : null;
          if (cr && cr.bottom > maxBottom) maxBottom = cr.bottom;
        }
        return maxBottom;
      },
      configurable: true,
    });

    Object.defineProperty(proto, "scrollWidth", {
      get: function () {
        let maxRight = this.clientWidth;
        const all = this.children || [];
        for (let i = 0; i < all.length; i++) {
          const cr = all[i].getBoundingClientRect ? all[i].getBoundingClientRect() : null;
          if (cr && cr.right > maxRight) maxRight = cr.right;
        }
        return maxRight;
      },
      configurable: true,
    });

    function scrollElementIntoView(el, centerIfNeeded) {
      if (!el) return;
      const vw = (typeof window !== "undefined" && window.innerWidth) || 1280;
      const vh = (typeof window !== "undefined" && window.innerHeight) || 720;

      const r = el.getBoundingClientRect();
      const inWin = (r.top >= 0 && r.bottom <= vh && r.left >= 0 && r.right <= vw);

      let clipped = false;
      let cur = el.parentElement;
      while (cur && cur !== document.body && cur !== document.documentElement) {
        const style = (typeof window !== "undefined" && window.getComputedStyle) ? window.getComputedStyle(cur) : cur.style;
        const overflow = (style && (style.overflow || style.overflowY || style.overflowX)) || "";
        const isScrollable = (overflow === "auto" || overflow === "scroll" || overflow === "hidden" || (cur.getAttribute && cur.getAttribute("style") && cur.getAttribute("style").includes("overflow")));
        if (isScrollable) {
          const cr = cur.getBoundingClientRect();
          if (r.top < cr.top || r.bottom > cr.top + cur.clientHeight || r.left < cr.left || r.right > cr.left + cur.clientWidth) {
            clipped = true;
            break;
          }
        }
        cur = cur.parentElement;
      }

      if (centerIfNeeded && inWin && !clipped) {
        return;
      }

      // Scroll container ancestors
      cur = el.parentElement;
      while (cur && cur !== document.body && cur !== document.documentElement) {
        const style = (typeof window !== "undefined" && window.getComputedStyle) ? window.getComputedStyle(cur) : cur.style;
        const overflow = (style && (style.overflow || style.overflowY || style.overflowX)) || "";
        const isScrollable = (overflow === "auto" || overflow === "scroll" || overflow === "hidden" || (cur.getAttribute && cur.getAttribute("style") && cur.getAttribute("style").includes("overflow")));
        if (isScrollable) {
          const cr = cur.getBoundingClientRect();
          const er = el.getBoundingClientRect();
          const curScrollTop = cur.scrollTop || 0;
          const elMidInContainer = (er.top - cr.top) + curScrollTop + (er.height / 2);
          const targetScrollTop = elMidInContainer - (cur.clientHeight / 2);
          cur.scrollTop = Math.max(0, targetScrollTop);
        }
        cur = cur.parentElement;
      }

      // Scroll window if needed
      const afterR = el.getBoundingClientRect();
      const afterInWin = (afterR.top >= 0 && afterR.bottom <= vh && afterR.left >= 0 && afterR.right <= vw);
      if (!afterInWin || !centerIfNeeded) {
        const curWinY = (typeof window !== "undefined" && (window.scrollY || window.pageYOffset)) || 0;
        const elPageY = afterR.top + curWinY;
        const targetWinY = (elPageY + afterR.height / 2) - (vh / 2);
        if (typeof window !== "undefined" && window.scrollTo) {
          window.scrollTo(window.scrollX || 0, Math.max(0, targetWinY));
        }
      }
    }

    proto.scrollIntoView = function (arg) {
      const center = (typeof arg === "object" && arg !== null && (arg.block === "center" || arg.inline === "center"));
      scrollElementIntoView(this, !center ? false : false);
    };
    markNative(proto.scrollIntoView, "scrollIntoView");

    proto.scrollIntoViewIfNeeded = function (centerIfNeeded) {
      scrollElementIntoView(this, centerIfNeeded !== false);
    };
    markNative(proto.scrollIntoViewIfNeeded, "scrollIntoViewIfNeeded");

    proto.checkVisibility = function (options) {
      if (!this.isConnected) return false;

      function getStyle(el) {
        if (typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
          try {
            const cs = window.getComputedStyle(el);
            if (cs) return cs;
          } catch (e) {}
        }
        if (typeof getComputedStyle === "function") {
          try {
            const cs = getComputedStyle(el);
            if (cs) return cs;
          } catch (e) {}
        }
        if (typeof globalThis !== "undefined" && typeof globalThis.getComputedStyle === "function") {
          try {
            const cs = globalThis.getComputedStyle(el);
            if (cs) return cs;
          } catch (e) {}
        }
        return el.style || null;
      }

      function getProp(el, name) {
        if (!el) return "";
        const s = getStyle(el);
        let v = "";
        if (s) {
          if (typeof s.getPropertyValue === "function") {
            const val = s.getPropertyValue(name);
            if (val !== undefined && val !== null && val !== "") v = String(val).trim();
          }
          if (!v) {
            const camel = name.replace(/-([a-z])/g, function (_, c) { return c.toUpperCase(); });
            if (s[camel] !== undefined && s[camel] !== null && s[camel] !== "") {
              v = String(s[camel]).trim();
            } else if (s[name] !== undefined && s[name] !== null && s[name] !== "") {
              v = String(s[name]).trim();
            }
          }
        }
        if (!v && el.style) {
          if (typeof el.style.getPropertyValue === "function") {
            const val = el.style.getPropertyValue(name);
            if (val !== undefined && val !== null && val !== "") v = String(val).trim();
          }
          if (!v) {
            const camel = name.replace(/-([a-z])/g, function (_, c) { return c.toUpperCase(); });
            if (el.style[camel] !== undefined && el.style[camel] !== null && el.style[camel] !== "") {
              v = String(el.style[camel]).trim();
            }
          }
        }
        return v;
      }

      function isZero(val) {
        if (val === "" || val === null || val === undefined) return false;
        const num = Number(val);
        return !isNaN(num) && num === 0;
      }

      const selfDisplay = getProp(this, "display").toLowerCase();
      if (selfDisplay === "none" || selfDisplay === "contents") return false;

      const checkOpacity = Boolean(options && options.checkOpacity === true);
      const checkVisibilityCSS = Boolean(options && options.checkVisibilityCSS === true);

      if (checkOpacity) {
        const selfOpacity = getProp(this, "opacity");
        if (isZero(selfOpacity)) return false;
      }

      if (checkVisibilityCSS) {
        const selfVis = getProp(this, "visibility").toLowerCase();
        if (selfVis === "hidden" || selfVis === "collapse") return false;
      }

      let cur = this.parentElement || this.parentNode;
      while (cur) {
        if (cur.nodeType === 1) { // ELEMENT_NODE
          const d = getProp(cur, "display").toLowerCase();
          if (d === "none") return false;

          if (checkOpacity) {
            const op = getProp(cur, "opacity");
            if (isZero(op)) return false;
          }
        } else if (cur.nodeType === 11 && cur.host) { // ShadowRoot
          cur = cur.host;
          continue;
        }
        cur = cur.parentElement || cur.parentNode;
      }

      return true;
    };
    markNative(proto.checkVisibility, "checkVisibility");

    proto.attachShadow = function (init) {
      if (!init || typeof init !== "object" || !init.mode) {
        throw new TypeError("Failed to execute 'attachShadow' on 'Element': 1 argument required, but only 0 present.");
      }
      const mode = String(init.mode);
      if (mode !== "open" && mode !== "closed") {
        throw new TypeError("Failed to execute 'attachShadow' on 'Element': The provided value '" + mode + "' is not a valid enum value of type ShadowRootMode.");
      }
      if (this._shadowRoot) {
        if (typeof DOMException !== "undefined") {
          throw new DOMException("Shadow root cannot be created on a host which already hosts a shadow tree", "InvalidStateError");
        }
        const err = new Error("Shadow root cannot be created on a host which already hosts a shadow tree");
        err.name = "InvalidStateError";
        throw err;
      }
      const ShadowRootCtor = typeof ShadowRoot !== "undefined" ? ShadowRoot : (global.ShadowRoot || null);
      const shadow = ShadowRootCtor ? new ShadowRootCtor(this, mode) : new DocumentFragment();
      shadow.host = this;
      shadow.mode = mode;
      this._shadowRoot = shadow;
      return shadow;
    };
    markNative(proto.attachShadow, "attachShadow");

    Object.defineProperty(proto, "shadowRoot", {
      get: function () {
        if (this._shadowRoot && this._shadowRoot.mode === "open") {
          return this._shadowRoot;
        }
        return null;
      },
      configurable: true,
    });

    // Universal Form & Control Properties on Element
    Object.defineProperty(proto, "name", {
      get: function () { return this.getAttribute("name") || ""; },
      set: function (v) { this.setAttribute("name", String(v)); },
      configurable: true,
    });

    Object.defineProperty(proto, "type", {
      get: function () {
        const tag = (this.tagName || "").toLowerCase();
        const t = this.getAttribute("type");
        if (t) return t.toLowerCase();
        if (tag === "button") return "submit";
        if (tag === "select") return this.multiple ? "select-multiple" : "select-one";
        if (tag === "textarea") return "textarea";
        if (tag === "input") return "text";
        return "";
      },
      set: function (v) { this.setAttribute("type", String(v)); },
      configurable: true,
    });

    Object.defineProperty(proto, "value", {
      get: function () {
        const tag = (this.tagName || "").toLowerCase();
        if (tag === "select") {
          const idx = this.selectedIndex;
          const opts = this.options;
          if (idx >= 0 && idx < opts.length) {
            return opts[idx].value;
          }
          return "";
        }
        if (this._value !== undefined) return this._value;
        if (tag === "checkbox" || tag === "radio") {
          return this.getAttribute("value") ?? "on";
        }
        if (this.hasAttribute && this.hasAttribute("value")) return this.getAttribute("value");
        if (tag === "textarea" || tag === "option") return this.textContent || "";
        return "";
      },
      set: function (v) {
        const tag = (this.tagName || "").toLowerCase();
        if (tag === "select") {
          const str = String(v);
          const opts = this.options;
          for (let i = 0; i < opts.length; i++) {
            if (opts[i].value === str) {
              this.selectedIndex = i;
              return;
            }
          }
          this.selectedIndex = -1;
          return;
        }
        this._value = String(v);
        if (this.setAttribute) {
          this.setAttribute("value", String(v));
        }
      },
      configurable: true,
    });

    Object.defineProperty(proto, "defaultValue", {
      get: function () {
        return this.getAttribute("value") || "";
      },
      set: function (v) {
        this.setAttribute("value", String(v));
      },
      configurable: true,
    });

    Object.defineProperty(proto, "checked", {
      get: function () {
        if (this._checkedDirty) return this._checked;
        return this.hasAttribute("checked");
      },
      set: function (v) {
        this._checkedDirty = true;
        this._checked = Boolean(v);
        const isRadio = (this.getAttribute && (this.getAttribute("type") || "").toLowerCase() === "radio");
        if (isRadio && v && this.name) {
          const root = (this.ownerDocument || (typeof document !== "undefined" ? document : null));
          if (root && root.querySelectorAll) {
            const radios = root.querySelectorAll('input[type="radio"][name="' + this.name + '"]');
            for (let i = 0; i < radios.length; i++) {
              if (radios[i] !== this) {
                radios[i]._checkedDirty = true;
                radios[i]._checked = false;
                if (radios[i].removeAttribute) radios[i].removeAttribute("checked");
              }
            }
          }
        }
        if (v) {
          if (this.setAttribute) this.setAttribute("checked", "");
        } else {
          if (this.removeAttribute) this.removeAttribute("checked");
        }
      },
      configurable: true,
    });

    Object.defineProperty(proto, "selected", {
      get: function () {
        if ((this.tagName || "").toLowerCase() === "option" && typeof global !== "undefined" && global.HTMLOptionElement) {
          const desc = Object.getOwnPropertyDescriptor(global.HTMLOptionElement.prototype, "selected");
          if (desc && desc.get) return desc.get.call(this);
        }
        if (this._selected !== undefined) return this._selected;
        return this.hasAttribute("selected");
      },
      set: function (v) {
        if ((this.tagName || "").toLowerCase() === "option" && typeof global !== "undefined" && global.HTMLOptionElement) {
          const desc = Object.getOwnPropertyDescriptor(global.HTMLOptionElement.prototype, "selected");
          if (desc && desc.set) return desc.set.call(this, v);
        }
        this._selected = Boolean(v);
        if (v) {
          if (this.setAttribute) this.setAttribute("selected", "");
        } else {
          if (this.removeAttribute) this.removeAttribute("selected");
        }
      },
      configurable: true,
    });

    Object.defineProperty(proto, "disabled", {
      get: function () { return this.hasAttribute("disabled"); },
      set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
      configurable: true,
    });

    Object.defineProperty(proto, "required", {
      get: function () { return this.hasAttribute("required"); },
      set: function (v) { if (v) this.setAttribute("required", ""); else this.removeAttribute("required"); },
      configurable: true,
    });

    Object.defineProperty(proto, "readOnly", {
      get: function () { return this.hasAttribute("readonly"); },
      set: function (v) { if (v) this.setAttribute("readonly", ""); else this.removeAttribute("readonly"); },
      configurable: true,
    });

    Object.defineProperty(proto, "placeholder", {
      get: function () { return this.getAttribute("placeholder") || ""; },
      set: function (v) { this.setAttribute("placeholder", String(v)); },
      configurable: true,
    });

    Object.defineProperty(proto, "options", {
      get: function () {
        if ((this.tagName || "").toLowerCase() === "select") {
          if (typeof getSelectOptionsCollection === "function") {
            return getSelectOptionsCollection(this);
          }
        }
        const opts = [];
        const up = typeof upgradeElement === "function" ? upgradeElement : (typeof globalThis !== "undefined" && globalThis.upgradeElement);
        let child = this.firstChild;
        while (child) {
          if ((child.tagName || "").toLowerCase() === "option") {
            if (up) up(child);
            opts.push(child);
          }
          child = child.nextSibling;
        }
        return new HTMLCollection(opts);
      },
      configurable: true,
    });

    Object.defineProperty(proto, "selectedIndex", {
      get: function () {
        if ((this.tagName || "").toLowerCase() === "select" && typeof global !== "undefined" && global.HTMLSelectElement) {
          const desc = Object.getOwnPropertyDescriptor(global.HTMLSelectElement.prototype, "selectedIndex");
          if (desc && desc.get) return desc.get.call(this);
        }
        const opts = this.options;
        for (let i = 0; i < opts.length; i++) {
          if (opts[i].selected) return i;
        }
        return opts.length > 0 && !this.multiple ? 0 : -1;
      },
      set: function (idx) {
        if ((this.tagName || "").toLowerCase() === "select" && typeof global !== "undefined" && global.HTMLSelectElement) {
          const desc = Object.getOwnPropertyDescriptor(global.HTMLSelectElement.prototype, "selectedIndex");
          if (desc && desc.set) return desc.set.call(this, idx);
        }
        const i = Number(idx);
        const opts = this.options;
        for (let j = 0; j < opts.length; j++) {
          opts[j].selected = (j === i);
        }
      },
      configurable: true,
    });

    Object.defineProperty(proto, "validity", {
      get: function () { return new ValidityState(this); },
      configurable: true,
    });

    Object.defineProperty(proto, "willValidate", {
      get: function () {
        if (this.disabled || this.readOnly) return false;
        const tag = (this.tagName || "").toLowerCase();
        if (tag === "button") return false;
        const t = this.type;
        return !(t === "hidden" || t === "button" || t === "reset" || t === "submit");
      },
      configurable: true,
    });

    proto.checkValidity = function () {
      if (!this.willValidate) return true;
      if (this.validity && !this.validity.valid) {
        const evt = new Event("invalid", { bubbles: false, cancelable: true });
        this.dispatchEvent(evt);
        return false;
      }
      return true;
    };
    markNative(proto.checkValidity, "checkValidity");

    proto.reportValidity = function () {
      return this.checkValidity();
    };
    markNative(proto.reportValidity, "reportValidity");

    proto.setCustomValidity = function (msg) {
      this._customValidityMessage = String(msg || "");
    };
    markNative(proto.setCustomValidity, "setCustomValidity");
  }

  // =============================================================
  // Module: element/html-element.js
  // =============================================================
  // src/js/shim/element/html-element.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/html-element.ts

  function createDatasetProxy(element) {
    return new Proxy({}, {
      get: function (target, prop) {
        if (typeof prop !== "string") return undefined;
        const kebab = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
        return element.getAttribute(kebab) ?? undefined;
      },
      set: function (target, prop, value) {
        if (typeof prop !== "string") return true;
        const kebab = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
        element.setAttribute(kebab, String(value));
        return true;
      },
      deleteProperty: function (target, prop) {
        if (typeof prop !== "string") return true;
        const kebab = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
        element.removeAttribute(kebab);
        return true;
      },
      has: function (target, prop) {
        if (typeof prop !== "string") return false;
        const kebab = "data-" + prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
        return element.hasAttribute(kebab);
      }
    });
  }

  function installHtmlElementExtensions(proto) {
    if (!proto) return;

    Object.defineProperty(proto, "dataset", {
      get: function () {
        if (!this._datasetProxy) {
          this._datasetProxy = createDatasetProxy(this);
        }
        return this._datasetProxy;
      },
      configurable: true,
    });

    const nativeHasAttribute = proto.hasAttribute || (typeof Element !== "undefined" && Element.prototype && Element.prototype.hasAttribute);
    const nativeSetAttribute = proto.setAttribute || (typeof Element !== "undefined" && Element.prototype && Element.prototype.setAttribute);
    const nativeRemoveAttribute = proto.removeAttribute || (typeof Element !== "undefined" && Element.prototype && Element.prototype.removeAttribute);

    Object.defineProperty(proto, "hidden", {
      get: function () {
        if (nativeHasAttribute) return nativeHasAttribute.call(this, "hidden");
        return this.hasAttribute ? this.hasAttribute("hidden") : false;
      },
      set: function (v) {
        if (v) {
          if (nativeSetAttribute) nativeSetAttribute.call(this, "hidden", "");
          else if (this.setAttribute) this.setAttribute("hidden", "");
        } else {
          if (nativeRemoveAttribute) nativeRemoveAttribute.call(this, "hidden");
          else if (this.removeAttribute) this.removeAttribute("hidden");
        }
      },
      configurable: true,
    });

    Object.defineProperty(proto, "tabIndex", {
      get: function () {
        const v = this.getAttribute("tabindex");
        return v !== null && v !== "" ? Number(v) : -1;
      },
      set: function (v) {
        this.setAttribute("tabindex", String(v));
      },
      configurable: true,
    });

    Object.defineProperty(proto, "title", {
      get: function () { return this.getAttribute("title") || ""; },
      set: function (v) { this.setAttribute("title", String(v)); },
      configurable: true,
    });

    Object.defineProperty(proto, "lang", {
      get: function () { return this.getAttribute("lang") || ""; },
      set: function (v) { this.setAttribute("lang", String(v)); },
      configurable: true,
    });

    Object.defineProperty(proto, "dir", {
      get: function () { return this.getAttribute("dir") || ""; },
      set: function (v) { this.setAttribute("dir", String(v)); },
      configurable: true,
    });

    Object.defineProperty(proto, "innerText", {
      get: function () { return this.textContent || ""; },
      set: function (v) { this.textContent = String(v); },
      configurable: true,
    });

    if (!proto.click) {
      proto.click = function () {
        const evt = new MouseEvent("click", { bubbles: true, cancelable: true });
        this.dispatchEvent(evt);
        if (!evt.defaultPrevented) {
          const tag = String(this.tagName || this.localName || "").toLowerCase();
          const hasHref = (this.hasAttribute ? this.hasAttribute("href") : false) || Boolean(this.href);
          if (tag === "a" && hasHref) {
            const href = (this.getAttribute ? this.getAttribute("href") : this.href) || "";
            if (href) {
              const loc = (typeof location !== "undefined") ? location : ((typeof window !== "undefined") ? window.location : null);
              if (loc) {
                if (href.startsWith("#")) {
                  loc.hash = href;
                } else {
                  loc.href = href;
                }
              }
            }
          }
        }
      };
      markNative(proto.click, "click");
    }

    if (!proto.focus) {
      proto.focus = function () {
        if (typeof document !== "undefined") {
          if (document.activeElement && document.activeElement !== this && typeof document.activeElement.blur === "function") {
            document.activeElement.blur();
          }
          document.activeElement = this;
        }
        if (typeof this.scrollIntoViewIfNeeded === "function") {
          this.scrollIntoViewIfNeeded();
        } else if (typeof this.scrollIntoView === "function") {
          this.scrollIntoView();
        }
        const evt = new FocusEvent("focus", { bubbles: false, cancelable: false });
        this.dispatchEvent(evt);
      };
      markNative(proto.focus, "focus");
    }

    if (!proto.blur) {
      proto.blur = function () {
        if (typeof document !== "undefined" && document.activeElement === this) {
          document.activeElement = document.body || null;
        }
        const evt = new FocusEvent("blur", { bubbles: false, cancelable: false });
        this.dispatchEvent(evt);
      };
      markNative(proto.blur, "blur");
    }
  }

  // =============================================================
  // Module: element/html-classes.js
  // =============================================================
  // src/js/shim/element/html-classes.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/html-classes.ts

  function installHtmlClasses(global) {
    const ElementProto = global.Element ? global.Element.prototype : {};
    const nativeGetAttribute = ElementProto.getAttribute || function (name) { return this.getAttribute(name); };
    const nativeSetAttribute = ElementProto.setAttribute || function (name, val) { this.setAttribute(name, val); };
    const nativeRemoveAttribute = ElementProto.removeAttribute || function (name) { this.removeAttribute(name); };
    const nativeHasAttribute = ElementProto.hasAttribute || function (name) { return this.hasAttribute(name); };

    const forceAsyncScripts = new WeakSet();
    function markScriptForceAsync(el) {
      if (el && typeof el === "object") forceAsyncScripts.add(el);
    }
    function clearScriptForceAsync(el) {
      if (el && typeof el === "object") forceAsyncScripts.delete(el);
    }
    function isScriptForceAsync(el) {
      return (el && typeof el === "object") ? forceAsyncScripts.has(el) : false;
    }
    global.__capsurf_markScriptForceAsync = markScriptForceAsync;
    global.__capsurf_clearScriptForceAsync = clearScriptForceAsync;
    global.__capsurf_isScriptForceAsync = isScriptForceAsync;

    function installHyperlinkDecomposition(proto) {
      Object.defineProperty(proto, "href", {
        get: function () {
          const raw = this.getAttribute("href");
          if (raw == null) return "";
          try {
            const base = typeof document !== "undefined" && document.baseURI ? document.baseURI : "http://localhost/";
            return new URL(raw, base).href;
          } catch (e) {
            return raw;
          }
        },
        set: function (v) { this.setAttribute("href", String(v)); },
        configurable: true,
      });

      const urlProps = ["origin", "protocol", "host", "hostname", "port", "pathname", "search", "hash"];
      for (let i = 0; i < urlProps.length; i++) {
        const prop = urlProps[i];
        Object.defineProperty(proto, prop, {
          get: function () {
            const raw = this.getAttribute("href");
            if (raw == null) {
              return prop === "protocol" ? ":" : "";
            }
            try {
              const u = new URL(this.href);
              return u[prop];
            } catch (e) {
              if (prop === "protocol") return ":";
              if (prop === "origin") return "null";
              return "";
            }
          },
          set: function (v) {
            const raw = this.getAttribute("href");
            if (raw == null) return;
            try {
              const u = new URL(this.href);
              u[prop] = v;
              this.href = u.href;
            } catch (e) {}
          },
          configurable: true,
        });
      }

      Object.defineProperty(proto, "target", {
        get: function () { return this.getAttribute("target") || ""; },
        set: function (v) { this.setAttribute("target", String(v)); },
        configurable: true,
      });

      Object.defineProperty(proto, "rel", {
        get: function () { return this.getAttribute("rel") || ""; },
        set: function (v) { this.setAttribute("rel", String(v)); },
        configurable: true,
      });
    }

    // 1. HTMLAnchorElement
    function HTMLAnchorElement() {}
    HTMLAnchorElement.prototype = Object.create(ElementProto);
    HTMLAnchorElement.prototype.constructor = HTMLAnchorElement;
    installHyperlinkDecomposition(HTMLAnchorElement.prototype);
    markNative(HTMLAnchorElement, "HTMLAnchorElement");
    global.HTMLAnchorElement = HTMLAnchorElement;

    // 1.05 HTMLStyleElement
    function HTMLStyleElement() {}
    HTMLStyleElement.prototype = Object.create(ElementProto);
    HTMLStyleElement.prototype.constructor = HTMLStyleElement;

    function getSheetForElement(el) {
      if (!el._sheet) {
        if (typeof globalThis.getOrCreateStyleSheet === "function") {
          el._sheet = globalThis.getOrCreateStyleSheet(el);
        } else if (typeof global.getOrCreateStyleSheet === "function") {
          el._sheet = global.getOrCreateStyleSheet(el);
        } else if (typeof global.CSSStyleSheet === "function") {
          el._sheet = new global.CSSStyleSheet(el);
        } else if (typeof globalThis.CSSStyleSheet === "function") {
          el._sheet = new globalThis.CSSStyleSheet(el);
        }
      }
      return el._sheet;
    }

    Object.defineProperty(HTMLStyleElement.prototype, "sheet", {
      get: function () {
        return getSheetForElement(this);
      },
      configurable: true,
    });
    markNative(HTMLStyleElement, "HTMLStyleElement");
    global.HTMLStyleElement = HTMLStyleElement;
    if (typeof globalThis !== "undefined") globalThis.HTMLStyleElement = HTMLStyleElement;
    if (typeof window !== "undefined") window.HTMLStyleElement = HTMLStyleElement;

    // Also define sheet on ElementProto for any <style> element
    if (!Object.getOwnPropertyDescriptor(ElementProto, "sheet")) {
      Object.defineProperty(ElementProto, "sheet", {
        get: function () {
          const tag = String(this.localName || this.tagName || "").toLowerCase();
          if (tag === "style") {
            return getSheetForElement(this);
          }
          return undefined;
        },
        configurable: true,
      });
    }

    // 1.1 HTMLAreaElement
    function HTMLAreaElement() {}
    HTMLAreaElement.prototype = Object.create(ElementProto);
    HTMLAreaElement.prototype.constructor = HTMLAreaElement;
    installHyperlinkDecomposition(HTMLAreaElement.prototype);
    Object.defineProperty(HTMLAreaElement.prototype, "alt", {
      get: function () { return this.getAttribute("alt") || ""; },
      set: function (v) { this.setAttribute("alt", String(v)); },
      configurable: true,
    });
    markNative(HTMLAreaElement, "HTMLAreaElement");
    global.HTMLAreaElement = HTMLAreaElement;

    // 2. HTMLImageElement
    function HTMLImageElement() {}
    HTMLImageElement.prototype = Object.create(ElementProto);
    HTMLImageElement.prototype.constructor = HTMLImageElement;

    Object.defineProperty(HTMLImageElement.prototype, "src", {
      get: function () {
        const raw = nativeGetAttribute.call(this, "src");
        if (raw == null) return "";
        try {
          const base = (typeof document !== "undefined" && document.baseURI) ? document.baseURI : (global.location ? global.location.href : "http://localhost/");
          return new URL(raw, base).href;
        } catch (e) {
          return raw;
        }
      },
      set: function (v) { nativeSetAttribute.call(this, "src", String(v)); },
      configurable: true,
      enumerable: true,
    });

    Object.defineProperty(HTMLImageElement.prototype, "alt", {
      get: function () { return this.getAttribute("alt") || ""; },
      set: function (v) { this.setAttribute("alt", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLImageElement.prototype, "width", {
      get: function () { return Number(this.getAttribute("width") || 0); },
      set: function (v) { this.setAttribute("width", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLImageElement.prototype, "height", {
      get: function () { return Number(this.getAttribute("height") || 0); },
      set: function (v) { this.setAttribute("height", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLImageElement.prototype, "naturalWidth", {
      get: function () { return this.width || 100; },
      configurable: true,
    });

    Object.defineProperty(HTMLImageElement.prototype, "naturalHeight", {
      get: function () { return this.height || 100; },
      configurable: true,
    });

    Object.defineProperty(HTMLImageElement.prototype, "complete", {
      get: function () { return true; },
      configurable: true,
    });

    markNative(HTMLImageElement, "HTMLImageElement");
    global.HTMLImageElement = HTMLImageElement;
    global.Image = HTMLImageElement;

    // 3. HTMLScriptElement
    function HTMLScriptElement() {
      markScriptForceAsync(this);
    }
    HTMLScriptElement.prototype = Object.create(ElementProto);
    HTMLScriptElement.prototype.constructor = HTMLScriptElement;

    Object.defineProperty(HTMLScriptElement.prototype, "text", {
      get: function () {
        return this.textContent || "";
      },
      set: function (v) {
        this.textContent = String(v);
      },
      configurable: true,
      enumerable: true,
    });

    Object.defineProperty(HTMLScriptElement.prototype, "src", {
      get: function () {
        const raw = nativeGetAttribute.call(this, "src");
        if (raw == null) return "";
        try {
          const base = (typeof document !== "undefined" && document.baseURI) ? document.baseURI : (global.location ? global.location.href : "http://localhost/");
          return new URL(raw, base).href;
        } catch (e) {
          return raw;
        }
      },
      set: function (v) { nativeSetAttribute.call(this, "src", String(v)); },
      configurable: true,
      enumerable: true,
    });

    Object.defineProperty(HTMLScriptElement.prototype, "type", {
      get: function () {
        const v = nativeGetAttribute.call(this, "type");
        return v == null ? "" : v;
      },
      set: function (v) { nativeSetAttribute.call(this, "type", String(v)); },
      configurable: true,
      enumerable: true,
    });

    Object.defineProperty(HTMLScriptElement.prototype, "async", {
      get: function () {
        return isScriptForceAsync(this) || nativeHasAttribute.call(this, "async");
      },
      set: function (v) {
        clearScriptForceAsync(this);
        if (v) nativeSetAttribute.call(this, "async", "");
        else nativeRemoveAttribute.call(this, "async");
      },
      configurable: true,
      enumerable: true,
    });

    Object.defineProperty(HTMLScriptElement.prototype, "defer", {
      get: function () { return nativeHasAttribute.call(this, "defer"); },
      set: function (v) { if (v) nativeSetAttribute.call(this, "defer", ""); else nativeRemoveAttribute.call(this, "defer"); },
      configurable: true,
      enumerable: true,
    });

    markNative(HTMLScriptElement, "HTMLScriptElement");
    global.HTMLScriptElement = HTMLScriptElement;

    // 4. HTMLCanvasElement
    function HTMLCanvasElement() {}
    HTMLCanvasElement.prototype = Object.create(ElementProto);
    HTMLCanvasElement.prototype.constructor = HTMLCanvasElement;

    Object.defineProperty(HTMLCanvasElement.prototype, "width", {
      get: function () { return Number(this.getAttribute("width") || 300); },
      set: function (v) { this.setAttribute("width", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLCanvasElement.prototype, "height", {
      get: function () { return Number(this.getAttribute("height") || 150); },
      set: function (v) { this.setAttribute("height", String(v)); },
      configurable: true,
    });

    markNative(HTMLCanvasElement, "HTMLCanvasElement");
    global.HTMLCanvasElement = HTMLCanvasElement;

    // 5. HTMLIFrameElement
    function locationInputString(value) {
      if (typeof value === "symbol") {
        throw new TypeError("Cannot convert a Symbol value to a string");
      }
      return String(value);
    }

    const FACADE_LOCATION_PARTS = [
      "protocol",
      "host",
      "hostname",
      "port",
      "pathname",
      "search",
      "hash",
    ];

    function makeIframeLocationFacade(iframeEl, initialUrl, contentWindow) {
      let current = initialUrl || "about:blank";

      function resolveAndNavigate(v) {
        const inputStr = locationInputString(v);
        let resolved = "";
        try {
          const base = (typeof document !== "undefined" && document.baseURI)
            ? document.baseURI
            : (global.location ? global.location.href : "about:blank");
          resolved = new URL(inputStr, base).toString();
        } catch (e) {
          const FrameDOMException = (contentWindow && contentWindow.DOMException) ||
            (typeof global !== "undefined" && global.DOMException) ||
            (typeof globalThis !== "undefined" && globalThis.DOMException) ||
            DOMException;
          const error = new FrameDOMException("Invalid URL", "SyntaxError");
          if (typeof FrameDOMException === "function") {
            try {
              Object.defineProperty(error, "constructor", { value: FrameDOMException });
            } catch (_) {}
          }
          throw error;
        }
        current = resolved;
        try {
          handleIframeNavigation(iframeEl, resolved);
        } catch (e) {}
      }

      const loc = {
        get href() {
          return current;
        },
        set href(v) {
          resolveAndNavigate(v);
        },
        get origin() {
          try {
            return new URL(current).origin;
          } catch (e) {
            return "null";
          }
        },
        toString: function () {
          return current;
        },
        valueOf: function () {
          return current;
        },
        assign: function (v) {
          resolveAndNavigate(v);
        },
        replace: function (v) {
          resolveAndNavigate(v);
        },
        reload: function () {
          resolveAndNavigate(current);
        },
      };

      for (let i = 0; i < FACADE_LOCATION_PARTS.length; i++) {
        (function (part) {
          Object.defineProperty(loc, part, {
            enumerable: true,
            configurable: true,
            get: function () {
              try {
                return new URL(current)[part];
              } catch (e) {
                return "";
              }
            },
            set: function (v) {
              try {
                const next = new URL(current);
                next[part] = String(v);
                loc.href = next.toString();
              } catch (e) {}
            },
          });
        })(FACADE_LOCATION_PARTS[i]);
      }

      loc[Symbol.toPrimitive] = function () {
        return current;
      };

      return loc;
    }

    const iframeLocationMap = new WeakMap();

    function ensureIframeLocationFacade(contentWindow, iframeEl, url) {
      let loc = iframeLocationMap.get(contentWindow);
      if (!loc) {
        loc = makeIframeLocationFacade(iframeEl, url, contentWindow);
        iframeLocationMap.set(contentWindow, loc);
        Object.defineProperty(contentWindow, "location", {
          enumerable: true,
          configurable: false,
          get: function () {
            return iframeLocationMap.get(contentWindow);
          },
          set: function (value) {
            const cur = iframeLocationMap.get(contentWindow);
            if (cur) {
              cur.href = locationInputString(value);
            }
          },
        });
      }
      return loc;
    }

    const FRAME_WINDOW_GLOBALS = [
      "DOMException", "Error", "TypeError", "RangeError", "SyntaxError",
      "ReferenceError", "URIError", "EvalError",
      "Array", "Object", "String", "Number", "Boolean", "Symbol",
      "Promise", "Function", "Date", "RegExp", "Map", "Set", "WeakMap", "WeakSet",
      "JSON", "Math", "Node", "Element", "Document", "DocumentFragment",
      "Event", "MessageEvent", "CustomEvent", "NodeList", "HTMLCollection",
      "DOMParser", "Range", "StaticRange", "URL", "URLSearchParams",
      "Blob", "FormData", "Headers", "Request", "Response",
      "AbortController", "AbortSignal", "HTMLElement", "HTMLIFrameElement",
      "setTimeout", "clearTimeout", "setInterval", "clearInterval",
      "requestAnimationFrame", "cancelAnimationFrame", "console", "fetch"
    ];

    function decorateIframeWindow(win) {
      const pageGlobal = typeof window !== "undefined" ? window : globalThis;
      for (let i = 0; i < FRAME_WINDOW_GLOBALS.length; i++) {
        const key = FRAME_WINDOW_GLOBALS[i];
        if (win[key] === undefined && pageGlobal[key] !== undefined) {
          win[key] = pageGlobal[key];
        }
      }
      if (pageGlobal.fetch) {
        win.fetch = function (resource, init) {
          return pageGlobal.fetch.call(win, resource, init);
        };
      }
    }

    function createIframePlaceholderDoc(docUrl) {
      let doc = null;
      try {
        const Ctor = global.DOMParser || (typeof DOMParser !== "undefined" ? DOMParser : null);
        if (Ctor) {
          const parser = new Ctor();
          doc = parser.parseFromString("<html><head></head><body></body></html>", "text/html");
          if (doc) {
            doc._mutable = true;
            try {
              Object.defineProperty(doc, "_baseUrl", {
                value: docUrl || "about:blank",
                configurable: true,
                writable: true,
              });
            } catch (e) {}
          }
        }
      } catch (e) {}
      return doc;
    }

    function handleIframeNavigation(iframeEl, resolvedUrl) {
      if (!iframeEl) return;

      if (resolvedUrl === "about:blank" || !resolvedUrl) {
        const doc = createIframePlaceholderDoc("about:blank");
        if (iframeEl._contentWindow) {
          iframeEl._contentWindow.document = doc;
          if (doc) {
            try {
              Object.defineProperty(doc, "defaultView", {
                value: iframeEl._contentWindow,
                configurable: true,
              });
            } catch (e) {}
          }
        }
        iframeEl._contentDocument = doc;
        Promise.resolve().then(function () {
          fireIframeLoadEvent(iframeEl);
        });
        return;
      }

      let loadPromise = null;
      if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_native_load_fixture === "function") {
        const fixtureHtml = globalThis.__capsurf_native_load_fixture(resolvedUrl);
        if (fixtureHtml !== null && fixtureHtml !== undefined) {
          loadPromise = Promise.resolve(fixtureHtml);
        }
      }

      if (!loadPromise) {
        if (typeof fetch === "function") {
          loadPromise = fetch(resolvedUrl).then(function (res) {
            return res.text();
          });
        } else {
          loadPromise = Promise.resolve("<!doctype html><html><head></head><body></body></html>");
        }
      }

      loadPromise.then(function (html) {
        let newDoc = null;
        try {
          const Ctor = global.DOMParser || (typeof DOMParser !== "undefined" ? DOMParser : null);
          if (Ctor) {
            const parser = new Ctor();
            newDoc = parser.parseFromString(html, "text/html");
            if (newDoc) {
              newDoc._mutable = true;
              try {
                Object.defineProperty(newDoc, "_baseUrl", {
                  value: resolvedUrl,
                  configurable: true,
                  writable: true,
                });
              } catch (e) {}
            }
          }
        } catch (e) {}

        if (newDoc) {
          if (iframeEl._contentWindow) {
            iframeEl._contentWindow.document = newDoc;
            try {
              Object.defineProperty(newDoc, "defaultView", {
                value: iframeEl._contentWindow,
                configurable: true,
              });
            } catch (e) {}
          }
          iframeEl._contentDocument = newDoc;
        }

        fireIframeLoadEvent(iframeEl);
      }).catch(function () {
        fireIframeLoadEvent(iframeEl);
      });
    }

    function fireIframeLoadEvent(iframeEl) {
      try {
        const evt = new Event("load", { bubbles: false, cancelable: false });
        if (typeof iframeEl.onload === "function") {
          try {
            iframeEl.onload.call(iframeEl, evt);
          } catch (e) {
            if (typeof console !== "undefined" && console.error) console.error(e);
          }
        }
        if (typeof iframeEl.dispatchEvent === "function") {
          try {
            iframeEl.dispatchEvent(evt);
          } catch (e) {}
        }
      } catch (e) {}
    }

    function getOrCreateIframeWindow(iframeEl) {
      if (iframeEl._contentWindow) return iframeEl._contentWindow;

      let initialUrl = "";
      try {
        initialUrl = (iframeEl.getAttribute && iframeEl.getAttribute("src")) || "";
      } catch (e) {}
      if (!initialUrl || initialUrl === "about:blank") {
        initialUrl = "about:blank";
      } else {
        try {
          const base = (typeof document !== "undefined" && document.baseURI)
            ? document.baseURI
            : (global.location ? global.location.href : "about:blank");
          initialUrl = new URL(initialUrl, base).href;
        } catch (e) {
          initialUrl = "about:blank";
        }
      }

      const initialDoc = createIframePlaceholderDoc(initialUrl);

      const contentWindow = {
        document: initialDoc,
        parent: global.window || global,
        top: (global.window && global.window.top) || global,
        frames: [],
        frameElement: iframeEl,
        name: (function () {
          try {
            return String(iframeEl.getAttribute ? (iframeEl.getAttribute("name") || "") : "");
          } catch (e) {
            return "";
          }
        })(),
        addEventListener: function () {},
        removeEventListener: function () {},
        dispatchEvent: function () { return true; },
      };

      contentWindow.self = contentWindow;
      contentWindow.window = contentWindow;

      decorateIframeWindow(contentWindow);
      ensureIframeLocationFacade(contentWindow, iframeEl, initialUrl);

      if (initialDoc) {
        try {
          Object.defineProperty(initialDoc, "defaultView", {
            value: contentWindow,
            configurable: true,
          });
        } catch (e) {}
      }

      contentWindow.postMessage = function () {};

      iframeEl._contentWindow = contentWindow;
      iframeEl._contentDocument = initialDoc;

      if (initialUrl && initialUrl !== "about:blank") {
        handleIframeNavigation(iframeEl, initialUrl);
      }

      return contentWindow;
    }

    function HTMLIFrameElement() {}
    HTMLIFrameElement.prototype = Object.create(ElementProto);
    HTMLIFrameElement.prototype.constructor = HTMLIFrameElement;

    Object.defineProperty(HTMLIFrameElement.prototype, "src", {
      get: function () {
        const raw = nativeGetAttribute.call(this, "src");
        if (raw == null) return "";
        try {
          const base = (typeof document !== "undefined" && document.baseURI) ? document.baseURI : (global.location ? global.location.href : "http://localhost/");
          return new URL(raw, base).href;
        } catch (e) {
          return raw;
        }
      },
      set: function (v) {
        const strVal = String(v);
        nativeSetAttribute.call(this, "src", strVal);
        if (this._contentWindow) {
          const base = (typeof document !== "undefined" && document.baseURI) ? document.baseURI : (global.location ? global.location.href : "about:blank");
          let resolved = "about:blank";
          try {
            resolved = new URL(strVal, base).href;
          } catch (e) {
            resolved = strVal;
          }
          handleIframeNavigation(this, resolved);
        }
      },
      configurable: true,
      enumerable: true,
    });

    Object.defineProperty(HTMLIFrameElement.prototype, "contentWindow", {
      get: function () {
        return getOrCreateIframeWindow(this);
      },
      configurable: true,
    });

    Object.defineProperty(HTMLIFrameElement.prototype, "contentDocument", {
      get: function () {
        const win = getOrCreateIframeWindow(this);
        return (win && win.document) || this._contentDocument || null;
      },
      configurable: true,
    });

    markNative(HTMLIFrameElement, "HTMLIFrameElement");
    global.HTMLIFrameElement = HTMLIFrameElement;

    // 6. Generic HTMLElement
    const upgradeTargets = [];
    function HTMLElement() {
      if (upgradeTargets.length > 0) {
        return upgradeTargets.pop();
      }
    }
    HTMLElement.prototype = Object.create(ElementProto);
    HTMLElement.prototype.constructor = HTMLElement;
    Object.defineProperty(HTMLElement, "_upgradeTarget", {
      get: function () {
        return upgradeTargets.length > 0 ? upgradeTargets[upgradeTargets.length - 1] : null;
      },
      set: function (val) {
        if (val === null || val === undefined) {
          if (upgradeTargets.length > 0) upgradeTargets.pop();
        } else {
          upgradeTargets.push(val);
        }
      },
      configurable: true,
    });
    markNative(HTMLElement, "HTMLElement");
    global.HTMLElement = HTMLElement;

    // 7. HTMLTemplateElement
    function HTMLTemplateElement() {
      HTMLElement.call(this);
    }
    HTMLTemplateElement.prototype = Object.create(HTMLElement.prototype);
    HTMLTemplateElement.prototype.constructor = HTMLTemplateElement;

    Object.defineProperty(HTMLTemplateElement.prototype, "content", {
      get: function () {
        if (!this._templateContent) {
          this._templateContent = initTemplateContent(this);
        }
        return this._templateContent;
      },
      configurable: true,
    });

    Object.defineProperty(HTMLTemplateElement.prototype, "innerHTML", {
      get: function () {
        return this.content ? this.content.innerHTML || "" : "";
      },
      set: function (val) {
        const content = this.content;
        if (content) {
          while (content.firstChild) {
            content.removeChild(content.firstChild);
          }
          const doc = this.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
          const temp = doc ? doc.createElement("div") : null;
          if (temp) {
            temp.innerHTML = val;
            while (temp.firstChild) {
              content.appendChild(temp.firstChild);
            }
          }
        }
      },
      configurable: true,
    });

    markNative(HTMLTemplateElement, "HTMLTemplateElement");
    global.HTMLTemplateElement = HTMLTemplateElement;
    if (typeof globalThis !== "undefined") {
      globalThis.HTMLTemplateElement = HTMLTemplateElement;
    }

    // 8. SVG Interfaces & AnimatedString
    function SVGAnimatedString() {
      throw new TypeError("Illegal constructor");
    }
    Object.defineProperty(SVGAnimatedString.prototype, "baseVal", {
      get: function () {
        if (this._element && typeof this._element.getAttribute === "function") {
          return this._element.getAttribute("class") || "";
        }
        return this._baseVal || "";
      },
      set: function (v) {
        const str = String(v);
        if (this._element && typeof this._element.setAttribute === "function") {
          this._element.setAttribute("class", str);
        } else {
          this._baseVal = str;
        }
      },
      configurable: true,
      enumerable: true,
    });
    Object.defineProperty(SVGAnimatedString.prototype, "animVal", {
      get: function () {
        return this.baseVal;
      },
      configurable: true,
      enumerable: true,
    });
    SVGAnimatedString.prototype.toString = function () {
      return this.baseVal;
    };
    markNative(SVGAnimatedString, "SVGAnimatedString");
    global.SVGAnimatedString = SVGAnimatedString;
    if (typeof globalThis !== "undefined") {
      globalThis.SVGAnimatedString = SVGAnimatedString;
    }

    function SVGElement() {}
    SVGElement.prototype = Object.create(ElementProto);
    SVGElement.prototype.constructor = SVGElement;
    Object.defineProperty(SVGElement.prototype, "className", {
      get: function () {
        if (!this._svgClassName) {
          const animStr = Object.create(SVGAnimatedString.prototype);
          animStr._element = this;
          this._svgClassName = animStr;
        }
        return this._svgClassName;
      },
      set: function (val) {
        if (typeof val === "string") {
          this.setAttribute("class", val);
        }
      },
      configurable: true,
      enumerable: true,
    });
    markNative(SVGElement, "SVGElement");
    global.SVGElement = SVGElement;
    if (typeof globalThis !== "undefined") {
      globalThis.SVGElement = SVGElement;
    }

    function SVGSVGElement() {
      SVGElement.call(this);
    }
    SVGSVGElement.prototype = Object.create(SVGElement.prototype);
    SVGSVGElement.prototype.constructor = SVGSVGElement;
    markNative(SVGSVGElement, "SVGSVGElement");
    global.SVGSVGElement = SVGSVGElement;
    if (typeof globalThis !== "undefined") {
      globalThis.SVGSVGElement = SVGSVGElement;
    }

    const SVG_VIEWBOX_LOCALS = new Set(["svg", "symbol", "marker", "pattern", "view"]);
    Object.defineProperty(ElementProto, "viewBox", {
      configurable: true,
      get: function () {
        const tag = String(this.localName || this.tagName || "").toLowerCase();
        if (!SVG_VIEWBOX_LOCALS.has(tag)) {
          return undefined;
        }
        const el = this;
        const parse = function () {
          const raw = String(el.getAttribute("viewBox") ?? "");
          const n = raw
            .split(/[\s,]+/)
            .filter(Boolean)
            .map(Number);
          return n.length === 4 && n.every(Number.isFinite) ? n : [0, 0, 0, 0];
        };
        const write = function (i, v) {
          const n = parse();
          n[i] = Number(v) || 0;
          el.setAttribute("viewBox", n.join(" "));
        };
        const mkRect = function () {
          return {
            get x() { return parse()[0]; },
            set x(v) { write(0, v); },
            get y() { return parse()[1]; },
            set y(v) { write(1, v); },
            get width() { return parse()[2]; },
            set width(v) { write(2, v); },
            get height() { return parse()[3]; },
            set height(v) { write(3, v); },
          };
        };
        return { baseVal: mkRect(), animVal: mkRect() };
      },
    });
  }

  function initTemplateContent(template) {
    if (template._templateContent) return template._templateContent;
    const doc = template.ownerDocument || (typeof document !== "undefined" ? document : null);
    const frag = (doc && typeof doc.createDocumentFragment === "function")
      ? doc.createDocumentFragment()
      : new DocumentFragment();
    template._templateContent = frag;

    // Move existing JS _children if any
    if (template._children && template._children.length > 0) {
      const kids = template._children.slice();
      template._children.length = 0;
      for (let i = 0; i < kids.length; i++) {
        if (kids[i]) frag.appendChild(typeof upgradeElement === "function" ? upgradeElement(kids[i]) : kids[i]);
      }
    }

    // Move native CP children if any
    if (typeof nativeElementFirstChild === "function") {
      let c = null;
      try { c = nativeElementFirstChild.call(template); } catch (e) {}
      const nativeKids = [];
      while (c) {
        nativeKids.push(c);
        let next = null;
        if (typeof nativeElementNextSibling === "function") {
          try { next = nativeElementNextSibling.call(c); } catch (e) {}
        }
        c = next;
      }
      for (let i = 0; i < nativeKids.length; i++) {
        const k = nativeKids[i];
        try {
          if (typeof nativeElementRemoveChild === "function") {
            nativeElementRemoveChild.call(template, k);
          }
        } catch (e) {}
        const upgraded = typeof upgradeElement === "function" ? upgradeElement(k) : k;
        upgraded._ownerFragment = frag;
        frag.appendChild(upgraded);
      }
    }

    template._children = [];
    return frag;
  }

  function processSubtreeTemplates(root) {
    if (!root) return;
    let tpls = [];
    try {
      if (typeof root.querySelectorAll === "function") {
        const q = root.querySelectorAll("template");
        if (q && q.length) {
          for (let i = 0; i < q.length; i++) tpls.push(q[i]);
        }
      }
    } catch (e) {}

    const rootTag = String(root.localName || root.tagName || "").toLowerCase();
    if (rootTag === "template") {
      tpls.unshift(root);
    }

    for (let i = 0; i < tpls.length; i++) {
      const tpl = typeof upgradeElement === "function" ? upgradeElement(tpls[i]) : tpls[i];
      if (!tpl) continue;
      const content = tpl.content;

      // Check declarative shadow DOM
      const shadowMode = tpl.getAttribute && tpl.getAttribute("shadowrootmode");
      if (shadowMode) {
        const mode = String(shadowMode).toLowerCase();
        if (mode === "open" || mode === "closed") {
          const host = tpl.parentNode || tpl.parentElement;
          if (host && host.nodeType === 1) { // ELEMENT_NODE
            let shadow = null;
            try {
              shadow = host.attachShadow({
                mode: mode,
                delegatesFocus: Boolean(tpl.hasAttribute && tpl.hasAttribute("shadowrootdelegatesfocus")),
              });
            } catch (e) {
              shadow = host.shadowRoot || host._shadowRoot;
            }
            if (shadow && content) {
              while (content.firstChild) {
                const ch = content.firstChild;
                try { content.removeChild(ch); } catch (e) {}
                shadow.appendChild(ch);
              }
            }
            try {
              if (typeof tpl.remove === "function") {
                tpl.remove();
              } else if (host.removeChild) {
                host.removeChild(tpl);
              }
            } catch (e) {}
          }
        }
      }
    }
  }

  // =============================================================
  // Module: element/dialog.js
  // =============================================================
  // src/js/shim/element/dialog.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/dialog.ts

  function installDialogElement(global) {
    const ElementProto = global.Element ? global.Element.prototype : {};
    const HTMLElementProto = (global.HTMLElement && global.HTMLElement.prototype) || ElementProto;

    function HTMLDialogElement() {
      if (global.HTMLElement) {
        const el = global.HTMLElement.call(this);
        if (el) return el;
      }
    }
    HTMLDialogElement.prototype = Object.create(HTMLElementProto);
    HTMLDialogElement.prototype.constructor = HTMLDialogElement;

    const returnValues = new WeakMap();
    const modalDialogs = new WeakMap();

    const dialogShow = function () {
      if (this.open) {
        if (modalDialogs.get(this) === true) {
          throw new DOMException("The dialog is already open.", "InvalidStateError");
        }
        return;
      }
      modalDialogs.set(this, false);
      this.setAttribute("open", "");
    };

    const dialogShowModal = function () {
      if (this.open) {
        if (modalDialogs.get(this) === true) return;
        throw new DOMException("The dialog is already open.", "InvalidStateError");
      }
      modalDialogs.set(this, true);
      this.setAttribute("open", "");
    };

    const dialogClose = function (returnValue) {
      if (!this.open) return;
      if (returnValue !== undefined) {
        this.returnValue = String(returnValue);
      }
      this.removeAttribute("open");
      modalDialogs.delete(this);
      const dialog = this;
      setTimeout(function () {
        const ev = new Event("close", { bubbles: false, cancelable: false });
        dialog.dispatchEvent(ev);
      }, 0);
    };

    const dialogRequestClose = function (returnValue) {
      if (!this.open) return;
      const cancel = new Event("cancel", { bubbles: false, cancelable: true });
      if (!this.dispatchEvent(cancel)) return;
      dialogClose.call(this, returnValue);
    };

    Object.defineProperties(HTMLDialogElement.prototype, {
      open: {
        configurable: true,
        enumerable: true,
        get: function () {
          return this.hasAttribute ? this.hasAttribute("open") : false;
        },
        set: function (val) {
          if (val) {
            this.setAttribute("open", "");
          } else {
            this.removeAttribute("open");
            modalDialogs.delete(this);
          }
        },
      },
      returnValue: {
        configurable: true,
        enumerable: true,
        get: function () {
          return returnValues.get(this) ?? "";
        },
        set: function (value) {
          returnValues.set(this, String(value));
        },
      },
      show: { configurable: true, writable: true, value: dialogShow },
      showModal: { configurable: true, writable: true, value: dialogShowModal },
      close: { configurable: true, writable: true, value: dialogClose },
      requestClose: { configurable: true, writable: true, value: dialogRequestClose },
    });

    markNative(HTMLDialogElement, "HTMLDialogElement");
    markNative(dialogShow, "show");
    markNative(dialogShowModal, "showModal");
    markNative(dialogClose, "close");
    markNative(dialogRequestClose, "requestClose");

    global.HTMLDialogElement = HTMLDialogElement;
    if (typeof globalThis !== "undefined") {
      globalThis.HTMLDialogElement = HTMLDialogElement;
    }
  }

  // =============================================================
  // Module: element/forms/validity.js
  // =============================================================
  // src/js/shim/element/forms/validity.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/validity.ts

  function ValidityState(control) {
    this._control = control;
  }

  Object.defineProperty(ValidityState.prototype, "valueMissing", {
    get: function () {
      const c = this._control;
      if (!c || !c.required) return false;
      if (c.type === "checkbox" || c.type === "radio") {
        return !c.checked;
      }
      return !c.value || c.value.length === 0;
    },
    configurable: true,
  });

  Object.defineProperty(ValidityState.prototype, "customError", {
    get: function () {
      return Boolean(this._control && this._control._customValidityMessage);
    },
    configurable: true,
  });

  Object.defineProperty(ValidityState.prototype, "typeMismatch", {
    get: function () {
      const c = this._control;
      if (!c || !c.value) return false;
      if (c.type === "email") {
        return !/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(c.value);
      }
      if (c.type === "url") {
        try {
          new URL(c.value);
          return false;
        } catch (e) {
          return true;
        }
      }
      return false;
    },
    configurable: true,
  });

  Object.defineProperty(ValidityState.prototype, "patternMismatch", {
    get: function () {
      const c = this._control;
      if (!c || !c.value || !c.pattern) return false;
      try {
        const reg = new RegExp("^(?:" + c.pattern + ")$");
        return !reg.test(c.value);
      } catch (e) {
        return false;
      }
    },
    configurable: true,
  });

  Object.defineProperty(ValidityState.prototype, "tooLong", {
    get: function () {
      const c = this._control;
      if (!c || !c.value || c.maxLength === undefined || c.maxLength < 0) return false;
      return c.value.length > c.maxLength;
    },
    configurable: true,
  });

  Object.defineProperty(ValidityState.prototype, "tooShort", {
    get: function () {
      const c = this._control;
      if (!c || !c.value || c.minLength === undefined || c.minLength <= 0) return false;
      return c.value.length < c.minLength;
    },
    configurable: true,
  });

  Object.defineProperty(ValidityState.prototype, "rangeUnderflow", {
    get: function () {
      const c = this._control;
      if (!c || c.min === undefined || c.min === "" || !c.value) return false;
      const num = Number(c.value);
      const min = Number(c.min);
      return !isNaN(num) && !isNaN(min) && num < min;
    },
    configurable: true,
  });

  Object.defineProperty(ValidityState.prototype, "rangeOverflow", {
    get: function () {
      const c = this._control;
      if (!c || c.max === undefined || c.max === "" || !c.value) return false;
      const num = Number(c.value);
      const max = Number(c.max);
      return !isNaN(num) && !isNaN(max) && num > max;
    },
    configurable: true,
  });

  Object.defineProperty(ValidityState.prototype, "stepMismatch", {
    get: function () { return false; },
    configurable: true,
  });

  Object.defineProperty(ValidityState.prototype, "badInput", {
    get: function () {
      const c = this._control;
      if (!c || !c.value) return false;
      if (c.type === "number") return isNaN(Number(c.value));
      return false;
    },
    configurable: true,
  });

  Object.defineProperty(ValidityState.prototype, "valid", {
    get: function () {
      return !this.valueMissing &&
        !this.customError &&
        !this.typeMismatch &&
        !this.patternMismatch &&
        !this.tooLong &&
        !this.tooShort &&
        !this.rangeUnderflow &&
        !this.rangeOverflow &&
        !this.stepMismatch &&
        !this.badInput;
    },
    configurable: true,
  });

  markNative(ValidityState, "ValidityState");

  // =============================================================
  // Module: element/forms/selection.js
  // =============================================================
  // src/js/shim/element/forms/selection.js

  function supportsSelection(control) {
    if (!control) return false;
    const tag = (control.tagName || "").toLowerCase();
    if (tag === "textarea") return true;
    if (tag === "input") {
      const type = (control.getAttribute("type") || "text").toLowerCase();
      return ["text", "search", "url", "tel", "password"].includes(type);
    }
    return false;
  }

  function getValueRaw(el) {
    if (el._value !== undefined) return el._value;
    let proto = Object.getPrototypeOf(el);
    while (proto) {
      const desc = Object.getOwnPropertyDescriptor(proto, "value");
      if (desc && typeof desc.get === "function") {
        const v = desc.get.call(el);
        return typeof v === "string" ? v : String(v ?? "");
      }
      proto = Object.getPrototypeOf(proto);
    }
    return el.getAttribute ? (el.getAttribute("value") || "") : "";
  }

  function setValueRaw(el, newValue) {
    el._value = String(newValue);
    if (el.setAttribute) el.setAttribute("value", String(newValue));
  }

  function notifyControlValueMutation(control, cursor) {
    if (typeof control.setSelectionRange === "function") {
      control.setSelectionRange(cursor, cursor);
    }
    try {
      const inputEvt = typeof InputEvent !== "undefined"
        ? new InputEvent("input", { bubbles: true, cancelable: false })
        : new Event("input", { bubbles: true });
      control.dispatchEvent(inputEvt);
    } catch (e) {}
  }

  function installSelectionMixin(Cls) {
    Object.defineProperty(Cls.prototype, "selectionStart", {
      get: function () {
        if (!supportsSelection(this)) return null;
        return this._selectionStart || 0;
      },
      set: function (v) {
        if (!supportsSelection(this)) throw new DOMException("InvalidStateError");
        const n = Math.max(0, Number(v) >>> 0);
        this.setSelectionRange(n, Math.max(n, this._selectionEnd || 0), this._selectionDirection);
      },
      configurable: true,
    });

    Object.defineProperty(Cls.prototype, "selectionEnd", {
      get: function () {
        if (!supportsSelection(this)) return null;
        return this._selectionEnd || 0;
      },
      set: function (v) {
        if (!supportsSelection(this)) throw new DOMException("InvalidStateError");
        const n = Math.max(0, Number(v) >>> 0);
        this.setSelectionRange(this._selectionStart || 0, n, this._selectionDirection);
      },
      configurable: true,
    });

    Cls.prototype.setSelectionRange = function (start, end, direction) {
      if (!supportsSelection(this)) throw new DOMException("InvalidStateError");
      const len = (this.value || "").length;
      let b = Math.min(Math.max(0, Number(end) || 0), len);
      let a = Math.min(Math.max(0, Number(start) || 0), b);
      this._selectionStart = a;
      this._selectionEnd = b;
      this._selectionDirection = (direction === "forward" || direction === "backward") ? direction : "none";
    };
  }

  if (typeof globalThis !== "undefined") {
    globalThis.getValueRaw = getValueRaw;
    globalThis.setValueRaw = setValueRaw;
    globalThis.notifyControlValueMutation = notifyControlValueMutation;
  }
  if (typeof global !== "undefined") {
    global.getValueRaw = getValueRaw;
    global.setValueRaw = setValueRaw;
    global.notifyControlValueMutation = notifyControlValueMutation;
  }

  // =============================================================
  // Module: element/forms/input.js
  // =============================================================
  // src/js/shim/element/forms/input.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/input.ts

  function findAssociatedForm(element) {
    if (!element) return null;
    const formAttr = (typeof element.getAttribute === "function") ? element.getAttribute("form") : null;
    if (formAttr && typeof document !== "undefined" && document.getElementById) {
      const f = document.getElementById(formAttr);
      if (f && (f.tagName || "").toLowerCase() === "form") return f;
    }
    let curr = element.parentElement || element.parentNode;
    while (curr) {
      if ((curr.tagName || "").toLowerCase() === "form") return curr;
      curr = curr.parentElement || curr.parentNode;
    }
    return null;
  }

  function installInputElement(global) {
    const ElementProto = global.Element ? global.Element.prototype : {};

    function HTMLInputElement() {
      this._value = "";
      this._checked = false;
      this._customValidityMessage = "";
      this._selectionStart = 0;
      this._selectionEnd = 0;
      this._selectionDirection = "none";
    }
    HTMLInputElement.prototype = Object.create(ElementProto);
    HTMLInputElement.prototype.constructor = HTMLInputElement;
    installSelectionMixin(HTMLInputElement);

    Object.defineProperty(HTMLInputElement.prototype, "type", {
      get: function () {
        return (this.getAttribute("type") || "text").toLowerCase();
      },
      set: function (v) {
        this.setAttribute("type", String(v));
      },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "value", {
      get: function () {
        if (this.type === "checkbox" || this.type === "radio") {
          return this.getAttribute("value") ?? "on";
        }
        if (this._value !== undefined) return this._value;
        return this.getAttribute("value") || "";
      },
      set: function (v) {
        if (this._defaultValue === undefined) {
          this._defaultValue = this.getAttribute("value") || "";
        }
        this._value = String(v);
        if (this.setAttribute) this.setAttribute("value", String(v));
        this._selectionStart = this._value.length;
        this._selectionEnd = this._value.length;
      },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "defaultValue", {
      get: function () {
        return this._defaultValue !== undefined ? this._defaultValue : (this.getAttribute("value") || "");
      },
      set: function (v) {
        this._defaultValue = String(v);
        this.setAttribute("value", String(v));
      },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "checked", {
      get: function () {
        if (this._checkedDirty) return this._checked;
        return this.hasAttribute("checked");
      },
      set: function (v) {
        this._checkedDirty = true;
        this._checked = Boolean(v);
        const isRadio = (this.getAttribute("type") || "").toLowerCase() === "radio";
        if (isRadio && v && this.name) {
          const form = findAssociatedForm(this);
          const root = form || (this.ownerDocument || (typeof document !== "undefined" ? document : null));
          if (root && root.querySelectorAll) {
            const radios = root.querySelectorAll('input[type="radio"][name="' + this.name + '"]');
            for (let i = 0; i < radios.length; i++) {
              if (radios[i] !== this) {
                radios[i]._checkedDirty = true;
                radios[i]._checked = false;
                if (radios[i].removeAttribute) radios[i].removeAttribute("checked");
              }
            }
          }
        }
        if (v) {
          if (this.setAttribute) this.setAttribute("checked", "");
        } else {
          if (this.removeAttribute) this.removeAttribute("checked");
        }
      },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "defaultChecked", {
      get: function () { return this.hasAttribute("checked"); },
      set: function (v) { if (v) this.setAttribute("checked", ""); else this.removeAttribute("checked"); },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "name", {
      get: function () { return this.getAttribute("name") || ""; },
      set: function (v) { this.setAttribute("name", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "disabled", {
      get: function () { return this.hasAttribute("disabled"); },
      set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "required", {
      get: function () { return this.hasAttribute("required"); },
      set: function (v) { if (v) this.setAttribute("required", ""); else this.removeAttribute("required"); },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "readOnly", {
      get: function () { return this.hasAttribute("readonly"); },
      set: function (v) { if (v) this.setAttribute("readonly", ""); else this.removeAttribute("readonly"); },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "placeholder", {
      get: function () { return this.getAttribute("placeholder") || ""; },
      set: function (v) { this.setAttribute("placeholder", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "form", {
      get: function () {
        return findAssociatedForm(this);
      },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "willValidate", {
      get: function () {
        if (this.disabled || this.readOnly) return false;
        const t = this.type;
        return !(t === "hidden" || t === "button" || t === "reset");
      },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "validity", {
      get: function () {
        return new ValidityState(this);
      },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "validationMessage", {
      get: function () {
        if (!this.willValidate || this.validity.valid) return "";
        if (this._customValidityMessage) return this._customValidityMessage;
        if (this.validity.valueMissing) return "Please fill out this field.";
        if (this.validity.typeMismatch) return "Please enter a valid value.";
        return "Constraints not satisfied.";
      },
      configurable: true,
    });

    HTMLInputElement.prototype.setCustomValidity = function (msg) {
      this._customValidityMessage = String(msg || "");
    };

    HTMLInputElement.prototype.checkValidity = function () {
      if (!this.willValidate) return true;
      if (!this.validity.valid) {
        const evt = new Event("invalid", { bubbles: false, cancelable: true });
        this.dispatchEvent(evt);
        return false;
      }
      return true;
    };

    HTMLInputElement.prototype.reportValidity = function () {
      return this.checkValidity();
    };

    markNative(HTMLInputElement, "HTMLInputElement");
    markNative(HTMLInputElement.prototype.setCustomValidity, "setCustomValidity");
    markNative(HTMLInputElement.prototype.checkValidity, "checkValidity");
    markNative(HTMLInputElement.prototype.reportValidity, "reportValidity");

    global.HTMLInputElement = HTMLInputElement;
  }

  // =============================================================
  // Module: element/forms/textarea.js
  // =============================================================
  // src/js/shim/element/forms/textarea.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/textarea.ts

  function installTextAreaElement(global) {
    const ElementProto = global.Element ? global.Element.prototype : {};

    function HTMLTextAreaElement() {
      this._value = undefined;
      this._customValidityMessage = "";
      this._selectionStart = 0;
      this._selectionEnd = 0;
      this._selectionDirection = "none";
    }
    HTMLTextAreaElement.prototype = Object.create(ElementProto);
    HTMLTextAreaElement.prototype.constructor = HTMLTextAreaElement;
    installSelectionMixin(HTMLTextAreaElement);

    Object.defineProperty(HTMLTextAreaElement.prototype, "type", {
      get: function () { return "textarea"; },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "value", {
      get: function () {
        if (this._value !== undefined) return this._value;
        return this.textContent || "";
      },
      set: function (v) {
        this._value = String(v);
        if (this.setAttribute) this.setAttribute("value", String(v));
        this.textContent = String(v);
        this._selectionStart = this._value.length;
        this._selectionEnd = this._value.length;
      },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "defaultValue", {
      get: function () { return this.textContent || ""; },
      set: function (v) { this.textContent = String(v); },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "rows", {
      get: function () { return Number(this.getAttribute("rows") || 2); },
      set: function (v) { this.setAttribute("rows", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "cols", {
      get: function () { return Number(this.getAttribute("cols") || 20); },
      set: function (v) { this.setAttribute("cols", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "name", {
      get: function () { return this.getAttribute("name") || ""; },
      set: function (v) { this.setAttribute("name", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "disabled", {
      get: function () { return this.hasAttribute("disabled"); },
      set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "required", {
      get: function () { return this.hasAttribute("required"); },
      set: function (v) { if (v) this.setAttribute("required", ""); else this.removeAttribute("required"); },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "readOnly", {
      get: function () { return this.hasAttribute("readonly"); },
      set: function (v) { if (v) this.setAttribute("readonly", ""); else this.removeAttribute("readonly"); },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "placeholder", {
      get: function () { return this.getAttribute("placeholder") || ""; },
      set: function (v) { this.setAttribute("placeholder", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "form", {
      get: function () {
        return findAssociatedForm(this);
      },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "willValidate", {
      get: function () {
        return !this.disabled && !this.readOnly;
      },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "validity", {
      get: function () {
        return new ValidityState(this);
      },
      configurable: true,
    });

    Object.defineProperty(HTMLTextAreaElement.prototype, "validationMessage", {
      get: function () {
        if (!this.willValidate || this.validity.valid) return "";
        if (this._customValidityMessage) return this._customValidityMessage;
        if (this.validity.valueMissing) return "Please fill out this field.";
        return "Constraints not satisfied.";
      },
      configurable: true,
    });

    HTMLTextAreaElement.prototype.setCustomValidity = function (msg) {
      this._customValidityMessage = String(msg || "");
    };

    HTMLTextAreaElement.prototype.checkValidity = function () {
      if (!this.willValidate) return true;
      if (!this.validity.valid) {
        const evt = new Event("invalid", { bubbles: false, cancelable: true });
        this.dispatchEvent(evt);
        return false;
      }
      return true;
    };

    HTMLTextAreaElement.prototype.reportValidity = function () {
      return this.checkValidity();
    };

    HTMLTextAreaElement.prototype.select = function () {};
    HTMLTextAreaElement.prototype.setSelectionRange = function () {};

    markNative(HTMLTextAreaElement, "HTMLTextAreaElement");
    markNative(HTMLTextAreaElement.prototype.setCustomValidity, "setCustomValidity");
    markNative(HTMLTextAreaElement.prototype.checkValidity, "checkValidity");
    markNative(HTMLTextAreaElement.prototype.reportValidity, "reportValidity");

    global.HTMLTextAreaElement = HTMLTextAreaElement;
  }

  // =============================================================
  // Module: element/forms/select.js
  // =============================================================
  // src/js/shim/element/forms/select.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/select.ts

  if (!globalThis.__capsurf_selectState) globalThis.__capsurf_selectState = new Map();
  if (!globalThis.__capsurf_optionState) globalThis.__capsurf_optionState = new Map();

  function getSelectState(el) {
    const nid = (el && typeof el._nid === "number") ? el._nid : 0;
    let st = globalThis.__capsurf_selectState.get(nid);
    if (!st) {
      st = { selectedIndex: undefined };
      if (nid > 0) globalThis.__capsurf_selectState.set(nid, st);
    }
    return st;
  }

  function getOptionState(el) {
    const nid = (el && typeof el._nid === "number") ? el._nid : 0;
    let st = globalThis.__capsurf_optionState.get(nid);
    if (!st) {
      st = { selected: undefined, value: undefined };
      if (nid > 0) globalThis.__capsurf_optionState.set(nid, st);
    }
    return st;
  }

  function isSameElement(a, b) {
    if (a === b) return true;
    if (!a || !b) return false;
    if (typeof a._nid === "number" && typeof b._nid === "number" && a._nid > 0) {
      return a._nid === b._nid;
    }
    return false;
  }

  function findOwnerSelect(opt) {
    let curr = opt.parentNode;
    while (curr) {
      if ((curr.tagName || "").toLowerCase() === "select") return curr;
      curr = curr.parentNode;
    }
    return null;
  }



  function computeSelectedIndex(selectEl) {
    const selState = getSelectState(selectEl);
    if (selState.selectedIndex !== undefined) {
      return selState.selectedIndex;
    }
    const opts = selectEl.options;
    if (opts.length === 0) return -1;

    if (selectEl.multiple) {
      for (let i = 0; i < opts.length; i++) {
        const ost = getOptionState(opts[i]);
        if (ost.selected === true || (ost.selected === undefined && opts[i].hasAttribute && opts[i].hasAttribute("selected"))) {
          return i;
        }
      }
      return -1;
    }

    // Single select:
    // 1. Check if any option has _selected explicitly set to true
    for (let i = opts.length - 1; i >= 0; i--) {
      const ost = getOptionState(opts[i]);
      if (ost.selected === true) {
        return i;
      }
    }

    // 2. Check if any option has selected attribute (last one wins per WHATWG spec if multiple)
    for (let i = opts.length - 1; i >= 0; i--) {
      const ost = getOptionState(opts[i]);
      if (ost.selected === undefined && opts[i].hasAttribute && opts[i].hasAttribute("selected")) {
        return i;
      }
    }

    // 3. Default selection: first enabled option
    for (let i = 0; i < opts.length; i++) {
      if (!opts[i].disabled) return i;
    }
    // Fallback to first option if all are disabled
    return 0;
  }

  function installSelectElement(global) {
    const ElementProto = global.Element ? global.Element.prototype : {};

    // 1. HTMLOptionElement
    function HTMLOptionElement() {
      this._selected = undefined;
    }
    HTMLOptionElement.prototype = Object.create(ElementProto);
    HTMLOptionElement.prototype.constructor = HTMLOptionElement;

    Object.defineProperty(HTMLOptionElement.prototype, "value", {
      get: function () {
        const ost = getOptionState(this);
        if (ost.value !== undefined) return ost.value;
        if (this.hasAttribute && this.hasAttribute("value")) return this.getAttribute("value");
        return this.textContent || "";
      },
      set: function (v) {
        getOptionState(this).value = String(v);
        if (this.setAttribute) this.setAttribute("value", String(v));
      },
      configurable: true,
    });

    Object.defineProperty(HTMLOptionElement.prototype, "text", {
      get: function () { return this.textContent || ""; },
      set: function (v) { this.textContent = String(v); },
      configurable: true,
    });

    Object.defineProperty(HTMLOptionElement.prototype, "selected", {
      get: function () {
        const ost = getOptionState(this);
        const owner = findOwnerSelect(this);
        if (!owner) {
          if (ost.selected !== undefined) return ost.selected;
          return this.hasAttribute && this.hasAttribute("selected");
        }
        if (owner.multiple) {
          if (ost.selected !== undefined) return ost.selected;
          return this.hasAttribute && this.hasAttribute("selected");
        }
        // Single select:
        const selIdx = computeSelectedIndex(owner);
        if (selIdx === -1) return false;
        return isSameElement(owner.options[selIdx], this);
      },
      set: function (v) {
        const ost = getOptionState(this);
        ost.selected = Boolean(v);
        const owner = findOwnerSelect(this);
        if (owner && !owner.multiple) {
          const selState = getSelectState(owner);
          const opts = owner.options;
          if (v) {
            for (let i = 0; i < opts.length; i++) {
              if (isSameElement(opts[i], this)) {
                selState.selectedIndex = i;
              } else {
                getOptionState(opts[i]).selected = false;
              }
            }
          } else {
            const curIdx = owner.selectedIndex;
            if (curIdx !== -1 && isSameElement(opts[curIdx], this)) {
              selState.selectedIndex = -1;
            }
          }
        }
      },
      configurable: true,
    });

    Object.defineProperty(HTMLOptionElement.prototype, "defaultSelected", {
      get: function () { return this.hasAttribute && this.hasAttribute("selected"); },
      set: function (v) {
        if (v) {
          this.setAttribute("selected", "");
        } else {
          this.removeAttribute("selected");
        }
      },
      configurable: true,
    });

    Object.defineProperty(HTMLOptionElement.prototype, "disabled", {
      get: function () { return this.hasAttribute && this.hasAttribute("disabled"); },
      set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
      configurable: true,
    });

    const origOptSetAttr = HTMLOptionElement.prototype.setAttribute;
    HTMLOptionElement.prototype.setAttribute = function (name, value) {
      if (origOptSetAttr) origOptSetAttr.call(this, name, value);
      if (String(name).toLowerCase() === "selected") {
        getOptionState(this).selected = true;
        const owner = findOwnerSelect(this);
        if (owner && !owner.multiple) {
          getSelectState(owner).selectedIndex = undefined;
          const opts = owner.options;
          for (let i = 0; i < opts.length; i++) {
            if (!isSameElement(opts[i], this)) {
              getOptionState(opts[i]).selected = false;
            }
          }
        }
      }
    };

    const origOptRemoveAttr = HTMLOptionElement.prototype.removeAttribute;
    HTMLOptionElement.prototype.removeAttribute = function (name) {
      if (origOptRemoveAttr) origOptRemoveAttr.call(this, name);
      if (String(name).toLowerCase() === "selected") {
        getOptionState(this).selected = false;
        const owner = findOwnerSelect(this);
        if (owner && !owner.multiple) {
          getSelectState(owner).selectedIndex = undefined;
        }
      }
    };

    markNative(HTMLOptionElement, "HTMLOptionElement");
    global.HTMLOptionElement = HTMLOptionElement;
    global.Option = function (text, value, defaultSelected, selected) {
      const opt = typeof document !== "undefined" && document.createElement ? document.createElement("option") : new HTMLOptionElement();
      if (text !== undefined) opt.text = text;
      if (value !== undefined) opt.value = value;
      if (defaultSelected !== undefined) opt.defaultSelected = Boolean(defaultSelected);
      if (selected !== undefined) opt.selected = Boolean(selected);
      return opt;
    };

    // 2. HTMLSelectElement
    function HTMLSelectElement() {
      this._customValidityMessage = "";
    }
    HTMLSelectElement.prototype = Object.create(ElementProto);
    HTMLSelectElement.prototype.constructor = HTMLSelectElement;

    Object.defineProperty(HTMLSelectElement.prototype, "type", {
      get: function () { return this.multiple ? "select-multiple" : "select-one"; },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "options", {
      get: function () {
        return getSelectOptionsCollection(this);
      },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "length", {
      get: function () { return this.options.length; },
      set: function (v) { this.options.length = v; },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "selectedIndex", {
      get: function () {
        return computeSelectedIndex(this);
      },
      set: function (idx) {
        const i = Number(idx);
        getSelectState(this).selectedIndex = i;
        const opts = this.options;
        for (let j = 0; j < opts.length; j++) {
          getOptionState(opts[j]).selected = (j === i);
        }
      },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "value", {
      get: function () {
        const idx = this.selectedIndex;
        if (idx >= 0 && idx < this.options.length) {
          return this.options[idx].value;
        }
        return "";
      },
      set: function (v) {
        const str = String(v);
        const opts = this.options;
        for (let i = 0; i < opts.length; i++) {
          if (opts[i].value === str) {
            this.selectedIndex = i;
            return;
          }
        }
        this.selectedIndex = -1;
      },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "multiple", {
      get: function () { return this.hasAttribute("multiple"); },
      set: function (v) { if (v) this.setAttribute("multiple", ""); else this.removeAttribute("multiple"); },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "name", {
      get: function () { return this.getAttribute("name") || ""; },
      set: function (v) { this.setAttribute("name", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "disabled", {
      get: function () { return this.hasAttribute("disabled"); },
      set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "required", {
      get: function () { return this.hasAttribute("required"); },
      set: function (v) { if (v) this.setAttribute("required", ""); else this.removeAttribute("required"); },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "form", {
      get: function () { return findAssociatedForm(this); },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "willValidate", {
      get: function () { return !this.disabled; },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "validity", {
      get: function () { return new ValidityState(this); },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "validationMessage", {
      get: function () {
        if (!this.willValidate || this.validity.valid) return "";
        if (this._customValidityMessage) return this._customValidityMessage;
        if (this.validity.valueMissing) return "Please select an item in the list.";
        return "Constraints not satisfied.";
      },
      configurable: true,
    });

    HTMLSelectElement.prototype.setCustomValidity = function (msg) {
      this._customValidityMessage = String(msg || "");
    };

    HTMLSelectElement.prototype.checkValidity = function () {
      if (!this.willValidate) return true;
      if (!this.validity.valid) {
        const evt = new Event("invalid", { bubbles: false, cancelable: true });
        this.dispatchEvent(evt);
        return false;
      }
      return true;
    };

    HTMLSelectElement.prototype.reportValidity = function () {
      return this.checkValidity();
    };

    HTMLSelectElement.prototype.add = function (element, before) {
      getSelectState(this).selectedIndex = undefined;
      this.options.add(element, before);
    };

    HTMLSelectElement.prototype.remove = function (index) {
      getSelectState(this).selectedIndex = undefined;
      if (index === undefined) {
        const p = this.parentElement || this.parentNode;
        if (p && p.removeChild) p.removeChild(this);
        return;
      }
      this.options.remove(index);
    };

    HTMLSelectElement.prototype.item = function (index) {
      return this.options.item(index);
    };

    HTMLSelectElement.prototype.namedItem = function (name) {
      return this.options.namedItem(name);
    };

    markNative(HTMLSelectElement, "HTMLSelectElement");
    markNative(HTMLSelectElement.prototype.setCustomValidity, "setCustomValidity");
    markNative(HTMLSelectElement.prototype.checkValidity, "checkValidity");
    markNative(HTMLSelectElement.prototype.add, "add");
    markNative(HTMLSelectElement.prototype.remove, "remove");
    markNative(HTMLSelectElement.prototype.item, "item");
    markNative(HTMLSelectElement.prototype.namedItem, "namedItem");

    global.HTMLSelectElement = HTMLSelectElement;
  }

  // =============================================================
  // Module: element/forms/button.js
  // =============================================================
  // src/js/shim/element/forms/button.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/button.ts

  function getButtonType(button) {
    let raw = "";
    if (button && typeof button.getAttribute === "function") {
      raw = button.getAttribute("type") || "";
    }
    if (!raw && button && button._type) {
      raw = button._type;
    }
    const lower = String(raw).toLowerCase().trim();
    if (lower === "button" || lower === "reset") return lower;
    return "submit";
  }

  function isSubmitOrResetButton(el) {
    if (!el || el.nodeType !== 1) return false;
    if (el.disabled || (typeof el.hasAttribute === "function" && el.hasAttribute("disabled"))) {
      return false;
    }
    const tag = (el.tagName || "").toLowerCase();
    if (tag === "button") {
      const t = getButtonType(el);
      return t === "submit" || t === "reset";
    }
    if (tag === "input") {
      const t = (el.type || (typeof el.getAttribute === "function" ? el.getAttribute("type") : "") || "").toLowerCase();
      return t === "submit" || t === "reset" || t === "image";
    }
    return false;
  }

  function handleButtonActivation(button) {
    if (!button) return;
    const tag = (button.tagName || "").toLowerCase();
    let type = "submit";
    if (tag === "button") {
      type = getButtonType(button);
    } else if (tag === "input") {
      type = (button.type || (typeof button.getAttribute === "function" ? button.getAttribute("type") : "") || "submit").toLowerCase();
    }

    const form = findAssociatedForm(button);
    if (!form) return;

    if (type === "submit" || type === "image") {
      submitForm(form, button);
    } else if (type === "reset") {
      if (typeof form.reset === "function") {
        form.reset();
      }
    }
  }

  function installButtonElement(global) {
    const ElementProto = global.Element ? global.Element.prototype : {};

    function HTMLButtonElement() {
      this._customValidityMessage = "";
    }
    HTMLButtonElement.prototype = Object.create(ElementProto);
    HTMLButtonElement.prototype.constructor = HTMLButtonElement;

    Object.defineProperty(HTMLButtonElement.prototype, "type", {
      get: function () {
        return getButtonType(this);
      },
      set: function (v) {
        this.setAttribute("type", String(v));
      },
      configurable: true,
    });

    Object.defineProperty(HTMLButtonElement.prototype, "name", {
      get: function () { return this.getAttribute("name") || ""; },
      set: function (v) { this.setAttribute("name", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLButtonElement.prototype, "value", {
      get: function () { return this.getAttribute("value") || ""; },
      set: function (v) { this.setAttribute("value", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLButtonElement.prototype, "disabled", {
      get: function () { return this.hasAttribute("disabled"); },
      set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
      configurable: true,
    });

    Object.defineProperty(HTMLButtonElement.prototype, "form", {
      get: function () { return findAssociatedForm(this); },
      configurable: true,
    });

    if (ElementProto && !ElementProto.form) {
      Object.defineProperty(ElementProto, "form", {
        get: function () {
          const tag = (this.tagName || "").toLowerCase();
          if (tag === "input" || tag === "button" || tag === "select" || tag === "textarea") {
            return findAssociatedForm(this);
          }
          return null;
        },
        configurable: true,
      });
    }

    Object.defineProperty(HTMLButtonElement.prototype, "formAction", {
      get: function () { return this.getAttribute("formaction") || ""; },
      set: function (v) { this.setAttribute("formaction", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLButtonElement.prototype, "formMethod", {
      get: function () { return this.getAttribute("formmethod") || ""; },
      set: function (v) { this.setAttribute("formmethod", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLButtonElement.prototype, "willValidate", {
      get: function () { return false; },
      configurable: true,
    });

    HTMLButtonElement.prototype.checkValidity = function () { return true; };
    HTMLButtonElement.prototype.reportValidity = function () { return true; };
    HTMLButtonElement.prototype.setCustomValidity = function (msg) {
      this._customValidityMessage = String(msg || "");
    };

    markNative(HTMLButtonElement, "HTMLButtonElement");
    markNative(HTMLButtonElement.prototype.checkValidity, "checkValidity");
    markNative(HTMLButtonElement.prototype.reportValidity, "reportValidity");

    global.HTMLButtonElement = HTMLButtonElement;
  }

  // =============================================================
  // Module: element/forms/form.js
  // =============================================================
  // src/js/shim/element/forms/form.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/form.ts

  function collectFormControls(form) {
    const controls = [];
    function walk(node) {
      if (!node) return;
      let child = node.firstChild;
      while (child) {
        const tag = (child.tagName || "").toLowerCase();
        if (tag === "input" || tag === "button" || tag === "select" || tag === "textarea") {
          controls.push(child);
        }
        if (tag !== "select") {
          walk(child);
        }
        child = child.nextSibling;
      }
    }
    walk(form);
    return controls;
  }

  function navigateTo(url) {
    const loc = (typeof window !== "undefined" && window.location) ||
                (typeof globalThis !== "undefined" && globalThis.location) ||
                (typeof location !== "undefined" && location);
    if (loc) {
      loc.href = url;
    }
  }

  function performFormSubmission(form, submitter) {
    if (!form) return;

    // Resolve method
    let method = (form.getAttribute("method") || "get").toLowerCase().trim();
    if (submitter && typeof submitter.getAttribute === "function") {
      const formMethod = submitter.getAttribute("formmethod");
      if (formMethod) {
        method = formMethod.toLowerCase().trim();
      }
    }
    if (method !== "post") {
      method = "get";
    }

    // Resolve action URL
    let action = form.getAttribute("action") || "";
    if (submitter && typeof submitter.getAttribute === "function") {
      const formAction = submitter.getAttribute("formaction");
      if (formAction) {
        action = formAction;
      }
    }

    const base = (typeof window !== "undefined" && window.location && window.location.href) ||
                 (typeof location !== "undefined" && location.href) || "about:blank";
    let targetUrl = action;
    if (!targetUrl) {
      targetUrl = base;
    } else {
      try {
        targetUrl = new URL(targetUrl, base).toString();
      } catch (e) {
        // Keep targetUrl as-is
      }
    }

    if (method === "get") {
      const controls = collectFormControls(form);
      const queryParts = [];
      for (let i = 0; i < controls.length; i++) {
        const c = controls[i];
        if (c.disabled || (typeof c.hasAttribute === "function" && c.hasAttribute("disabled"))) {
          continue;
        }
        const name = c.name || (typeof c.getAttribute === "function" ? c.getAttribute("name") : "");
        if (!name) continue;

        const tag = (c.tagName || "").toLowerCase();
        const type = (c.type || (typeof c.getAttribute === "function" ? c.getAttribute("type") : "") || "").toLowerCase();

        // For buttons and submit inputs: only include if it is the submitter
        if (tag === "button" || type === "submit" || type === "image") {
          if (c !== submitter) continue;
        }

        // Skip non-submitting input types
        if (type === "button" || type === "reset") {
          continue;
        }

        // Checkable inputs: only include if checked
        if (type === "checkbox" || type === "radio") {
          if (!c.checked) continue;
        }

        let val = c.value !== undefined ? String(c.value) : "";
        if ((type === "checkbox" || type === "radio") && !val && (typeof c.hasAttribute === "function" && !c.hasAttribute("value"))) {
          val = "on";
        }
        queryParts.push(encodeURIComponent(name) + "=" + encodeURIComponent(val));
      }

      const queryString = queryParts.join("&");
      const baseUrl = targetUrl.split("?")[0].split("#")[0];
      const finalUrl = queryString ? (baseUrl + "?" + queryString) : baseUrl;
      navigateTo(finalUrl);
    } else if (method === "post") {
      navigateTo(targetUrl.split("#")[0]);
    }
  }

  function submitForm(form, submitter) {
    if (!form) return;
    const ev = new SubmitEvent("submit", {
      bubbles: true,
      cancelable: true,
      submitter: submitter || null,
    });
    form.dispatchEvent(ev);
    if (ev.defaultPrevented) {
      return;
    }
    performFormSubmission(form, submitter);
  }

  function installFormElement(global) {
    const ElementProto = global.Element ? global.Element.prototype : {};

    function HTMLFormElement() {}
    HTMLFormElement.prototype = Object.create(ElementProto);
    HTMLFormElement.prototype.constructor = HTMLFormElement;

    Object.defineProperty(HTMLFormElement.prototype, "elements", {
      get: function () {
        const list = collectFormControls(this);
        const col = new HTMLCollection(list);
        // Map controls by name and id
        for (let i = 0; i < list.length; i++) {
          const item = list[i];
          if (item.name && !col[item.name]) col[item.name] = item;
          if (item.id && !col[item.id]) col[item.id] = item;
        }
        return col;
      },
      configurable: true,
    });

    Object.defineProperty(HTMLFormElement.prototype, "length", {
      get: function () { return this.elements.length; },
      configurable: true,
    });

    Object.defineProperty(HTMLFormElement.prototype, "name", {
      get: function () { return this.getAttribute("name") || ""; },
      set: function (v) { this.setAttribute("name", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLFormElement.prototype, "method", {
      get: function () { return (this.getAttribute("method") || "get").toLowerCase(); },
      set: function (v) { this.setAttribute("method", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLFormElement.prototype, "action", {
      get: function () { return this.getAttribute("action") || ""; },
      set: function (v) { this.setAttribute("action", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLFormElement.prototype, "target", {
      get: function () { return this.getAttribute("target") || ""; },
      set: function (v) { this.setAttribute("target", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLFormElement.prototype, "noValidate", {
      get: function () { return this.hasAttribute("novalidate"); },
      set: function (v) { if (v) this.setAttribute("novalidate", ""); else this.removeAttribute("novalidate"); },
      configurable: true,
    });

    HTMLFormElement.prototype.submit = function () {
      performFormSubmission(this, null);
    };

    HTMLFormElement.prototype.requestSubmit = function (submitter) {
      if (submitter !== undefined && submitter !== null) {
        const tag = (submitter.tagName || "").toLowerCase();
        const type = (submitter.type || (typeof submitter.getAttribute === "function" ? submitter.getAttribute("type") : "") || "").toLowerCase();
        const isButton = (tag === "button") || (tag === "input" && (type === "submit" || type === "image"));
        if (!isButton) {
          throw new TypeError("requestSubmit: submitter must be a submit button");
        }
        if (findAssociatedForm(submitter) !== this) {
          throw new DOMException("requestSubmit: submitter is not associated with this form", "NotFoundError");
        }
      }
      submitForm(this, submitter || null);
    };

    HTMLFormElement.prototype.reset = function () {
      if (!(this instanceof HTMLFormElement)) {
        throw new TypeError("Failed to execute 'reset' on 'HTMLFormElement': Illegal invocation");
      }
      if (this.namespaceURI && this.namespaceURI !== "http://www.w3.org/1999/xhtml") {
        throw new TypeError("Failed to execute 'reset' on 'HTMLFormElement': Illegal invocation");
      }
      if (Object.prototype.hasOwnProperty.call(this, "_id")) {
        const canonical = typeof global.__capsurf_getCanonicalId === "function" ? global.__capsurf_getCanonicalId(this) : undefined;
        if (canonical !== undefined && this._id !== canonical) {
          throw new TypeError("Failed to execute 'reset' on 'HTMLFormElement': Invalid trusted event target");
        }
      }

      const list = collectFormControls(this);
      for (let i = 0; i < list.length; i++) {
        const c = list[i];
        if (c.defaultValue !== undefined) c.value = c.defaultValue;
        if (c.defaultChecked !== undefined) c.checked = c.defaultChecked;
        if (c.defaultSelected !== undefined) c.selected = c.defaultSelected;
        if ((c.tagName || "").toLowerCase() === "select") {
          if (typeof getSelectState === "function") getSelectState(c).selectedIndex = undefined;
          c._selectedIndex = undefined;
          const opts = c.options;
          for (let j = 0; j < opts.length; j++) {
            if (typeof getOptionState === "function") getOptionState(opts[j]).selected = undefined;
            opts[j]._selected = undefined;
          }
        }
      }
      const evt = new Event("reset", { bubbles: true, cancelable: true });
      evt._isTrusted = true;
      if (typeof dispatchTrustedEvent === "function") {
        dispatchTrustedEvent(this, evt);
      } else if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
        global.__capsurf_dispatchTrustedEvent(this, evt);
      } else {
        this.dispatchEvent(evt);
      }
    };

    HTMLFormElement.prototype.checkValidity = function () {
      const list = collectFormControls(this);
      let allValid = true;
      for (let i = 0; i < list.length; i++) {
        if (list[i].checkValidity && !list[i].checkValidity()) {
          allValid = false;
        }
      }
      return allValid;
    };

    HTMLFormElement.prototype.reportValidity = function () {
      return this.checkValidity();
    };

    markNative(HTMLFormElement, "HTMLFormElement");
    markNative(HTMLFormElement.prototype.submit, "submit");
    markNative(HTMLFormElement.prototype.requestSubmit, "requestSubmit");
    markNative(HTMLFormElement.prototype.reset, "reset");
    markNative(HTMLFormElement.prototype.checkValidity, "checkValidity");
    markNative(HTMLFormElement.prototype.reportValidity, "reportValidity");

    if (ElementProto) {
      if (!ElementProto.submit) {
        ElementProto.submit = function () {
          if ((this.tagName || "").toLowerCase() === "form") {
            performFormSubmission(this, null);
          }
        };
        markNative(ElementProto.submit, "submit");
      }
      if (!ElementProto.reset) {
        ElementProto.reset = function () {
          if ((this.tagName || "").toLowerCase() === "form") {
            HTMLFormElement.prototype.reset.call(this);
          }
        };
        markNative(ElementProto.reset, "reset");
      }
      if (!ElementProto.requestSubmit) {
        ElementProto.requestSubmit = function (submitter) {
          if ((this.tagName || "").toLowerCase() === "form") {
            HTMLFormElement.prototype.requestSubmit.call(this, submitter);
          }
        };
        markNative(ElementProto.requestSubmit, "requestSubmit");
      }
    }

    global.HTMLFormElement = HTMLFormElement;
  }

  // =============================================================
  // Module: file.js
  // =============================================================
  // src/js/shim/file.js
  // Aligned with WHATWG File API specification
  // Provides Blob and File implementations, UTF-8 conversion, MIME normalization,
  // and Content-Type preservation for Request/Response .blob()

  function normalizeBlobType(type) {
    if (type === undefined || type === null) return "";
    const str = String(type);
    for (let i = 0; i < str.length; i++) {
      const code = str.charCodeAt(i);
      if (code < 0x20 || code > 0x7e) {
        return "";
      }
    }
    return str.toLowerCase();
  }

  function stringToUtf8(str) {
    const bytes = [];
    for (let i = 0; i < str.length; i++) {
      let codePoint = str.charCodeAt(i);
      if (codePoint >= 0xd800 && codePoint <= 0xdbff && i + 1 < str.length) {
        const next = str.charCodeAt(i + 1);
        if (next >= 0xdc00 && next <= 0xdfff) {
          codePoint = ((codePoint - 0xd800) << 10) + (next - 0xdc00) + 0x10000;
          i++;
        }
      }
      if (codePoint < 0x80) {
        bytes.push(codePoint);
      } else if (codePoint < 0x800) {
        bytes.push(0xc0 | (codePoint >> 6));
        bytes.push(0x80 | (codePoint & 0x3f));
      } else if (codePoint < 0x10000) {
        bytes.push(0xe0 | (codePoint >> 12));
        bytes.push(0x80 | ((codePoint >> 6) & 0x3f));
        bytes.push(0x80 | (codePoint & 0x3f));
      } else {
        bytes.push(0xf0 | (codePoint >> 18));
        bytes.push(0x80 | ((codePoint >> 12) & 0x3f));
        bytes.push(0x80 | ((codePoint >> 6) & 0x3f));
        bytes.push(0x80 | (codePoint & 0x3f));
      }
    }
    return new Uint8Array(bytes);
  }

  function utf8ToString(bytes) {
    let str = "";
    let i = 0;
    while (i < bytes.length) {
      const b1 = bytes[i++];
      if (b1 < 0x80) {
        str += String.fromCharCode(b1);
      } else if ((b1 >> 5) === 0x06) {
        const b2 = bytes[i++];
        str += String.fromCharCode(((b1 & 0x1f) << 6) | (b2 & 0x3f));
      } else if ((b1 >> 4) === 0x0e) {
        const b2 = bytes[i++];
        const b3 = bytes[i++];
        str += String.fromCharCode(((b1 & 0x0f) << 12) | ((b2 & 0x3f) << 6) | (b3 & 0x3f));
      } else if ((b1 >> 3) === 0x1e) {
        const b2 = bytes[i++];
        const b3 = bytes[i++];
        const b4 = bytes[i++];
        let cp = ((b1 & 0x07) << 18) | ((b2 & 0x3f) << 12) | ((b3 & 0x3f) << 6) | (b4 & 0x3f);
        cp -= 0x10000;
        str += String.fromCharCode((cp >> 10) + 0xd800, (cp & 0x3ff) + 0xdc00);
      } else {
        str += String.fromCharCode(b1);
      }
    }
    return str;
  }

  function processBlobParts(blobParts) {
    if (blobParts === undefined || blobParts === null) {
      return new Uint8Array(0);
    }
    let list = [];
    if (Array.isArray(blobParts)) {
      list = blobParts;
    } else if (typeof blobParts[Symbol.iterator] === "function") {
      for (const item of blobParts) {
        list.push(item);
      }
    } else if (typeof blobParts.length === "number") {
      for (let i = 0; i < blobParts.length; i++) {
        list.push(blobParts[i]);
      }
    } else {
      throw new TypeError("Failed to construct 'Blob': The provided value cannot be converted to a sequence.");
    }

    const chunks = [];
    let totalLength = 0;

    for (let i = 0; i < list.length; i++) {
      const part = list[i];
      let u8;
      if (typeof part === "string") {
        u8 = stringToUtf8(part);
      } else if (part instanceof ArrayBuffer) {
        u8 = new Uint8Array(part.slice(0));
      } else if (ArrayBuffer.isView(part)) {
        u8 = new Uint8Array(part.buffer.slice(part.byteOffset, part.byteOffset + part.byteLength));
      } else if (part && part._bytes instanceof Uint8Array) {
        u8 = part._bytes;
      } else {
        u8 = stringToUtf8(String(part));
      }
      chunks.push(u8);
      totalLength += u8.length;
    }

    const result = new Uint8Array(totalLength);
    let offset = 0;
    for (let i = 0; i < chunks.length; i++) {
      result.set(chunks[i], offset);
      offset += chunks[i].length;
    }
    return result;
  }

  function Blob(blobParts, options) {
    if (!(this instanceof Blob)) {
      throw new TypeError("Failed to construct 'Blob': Please use the 'new' operator.");
    }
    if (options !== undefined && options !== null && typeof options !== "object" && typeof options !== "function") {
      throw new TypeError("Failed to construct 'Blob': parameter 2 is not an object.");
    }
    options = options || {};

    if (options._rawType != null) {
      this._type = String(options._rawType);
    } else {
      this._type = normalizeBlobType(options.type);
    }

    this._bytes = processBlobParts(blobParts);
  }

  Object.defineProperty(Blob.prototype, "size", {
    get: function () { return this._bytes.length; },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(Blob.prototype, "type", {
    get: function () { return this._type; },
    configurable: true,
    enumerable: true,
  });

  Blob.prototype.slice = function (start, end, contentType) {
    const size = this._bytes.length;
    let relativeStart = start !== undefined ? Number(start) : 0;
    let relativeEnd = end !== undefined ? Number(end) : size;

    if (Number.isNaN(relativeStart)) relativeStart = 0;
    if (Number.isNaN(relativeEnd)) relativeEnd = 0;

    let spanStart;
    if (relativeStart < 0) {
      spanStart = Math.max(size + relativeStart, 0);
    } else {
      spanStart = Math.min(relativeStart, size);
    }

    let spanEnd;
    if (relativeEnd < 0) {
      spanEnd = Math.max(size + relativeEnd, 0);
    } else {
      spanEnd = Math.min(relativeEnd, size);
    }

    const span = Math.max(spanEnd - spanStart, 0);
    const slicedBytes = this._bytes.subarray(spanStart, spanStart + span);

    const newBlob = new Blob([], {
      type: contentType !== undefined ? String(contentType) : "",
    });
    newBlob._bytes = new Uint8Array(slicedBytes);
    return newBlob;
  };

  Blob.prototype.text = function () {
    try {
      return Promise.resolve(utf8ToString(this._bytes));
    } catch (e) {
      return Promise.reject(e);
    }
  };

  Blob.prototype.arrayBuffer = function () {
    try {
      const buffer = this._bytes.buffer.slice(
        this._bytes.byteOffset,
        this._bytes.byteOffset + this._bytes.byteLength
      );
      return Promise.resolve(buffer);
    } catch (e) {
      return Promise.reject(e);
    }
  };

  Blob.prototype.stream = function () {
    const bytes = this._bytes;
    if (typeof ReadableStream !== "undefined") {
      return new ReadableStream({
        start(controller) {
          controller.enqueue(bytes);
          controller.close();
        }
      });
    }
    throw new Error("ReadableStream not implemented");
  };

  Object.defineProperty(Blob.prototype, Symbol.toStringTag, {
    value: "Blob",
    configurable: true,
  });

  markNative(Blob, "Blob");
  markNative(Blob.prototype.slice, "slice");
  markNative(Blob.prototype.text, "text");
  markNative(Blob.prototype.arrayBuffer, "arrayBuffer");
  markNative(Blob.prototype.stream, "stream");

  function File(fileBits, fileName, options) {
    if (!(this instanceof File)) {
      throw new TypeError("Failed to construct 'File': Please use the 'new' operator.");
    }
    if (arguments.length < 2) {
      throw new TypeError("Failed to construct 'File': 2 arguments required, but only " + arguments.length + " present.");
    }
    if (options !== undefined && options !== null && typeof options !== "object" && typeof options !== "function") {
      throw new TypeError("Failed to construct 'File': parameter 3 is not an object.");
    }
    options = options || {};
    Blob.call(this, fileBits, options);

    this._name = String(fileName);
    this._lastModified = options.lastModified != null ? Number(options.lastModified) : Date.now();
  }

  File.prototype = Object.create(Blob.prototype);
  File.prototype.constructor = File;

  Object.defineProperty(File.prototype, "name", {
    get: function () { return this._name; },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(File.prototype, "lastModified", {
    get: function () { return this._lastModified; },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(File.prototype, "webkitRelativePath", {
    get: function () { return ""; },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(File.prototype, Symbol.toStringTag, {
    value: "File",
    configurable: true,
  });

  markNative(File, "File");

  // WHATWG MIME serialization
  function parseAndSerializeMimeType(contentType) {
    if (!contentType) return "";
    const parts = String(contentType).split(";");
    const mime = parts[0].trim().toLowerCase();
    if (!mime || mime.indexOf("/") === -1) return "";
    const params = [];
    for (let i = 1; i < parts.length; i++) {
      const part = parts[i].trim();
      if (!part) continue;
      const eq = part.indexOf("=");
      if (eq !== -1) {
        const key = part.slice(0, eq).trim().toLowerCase();
        const val = part.slice(eq + 1).trim();
        params.push(key + "=" + val);
      } else {
        params.push(part.toLowerCase());
      }
    }
    return params.length > 0 ? mime + ";" + params.join(";") : mime;
  }

  function extractMimeFromHeaders(headers) {
    if (!headers) return "";
    let ct = null;
    if (typeof headers.get === "function") {
      ct = headers.get("content-type");
    } else if (typeof headers === "object") {
      for (const k of Object.keys(headers)) {
        if (k.toLowerCase() === "content-type") {
          ct = headers[k];
          break;
        }
      }
    }
    return ct ? parseAndSerializeMimeType(ct) : "";
  }

  // Request & Response support for .blob() Content-Type passthrough
  function Headers(init) {
    this._map = new Map();
    if (init === undefined) {
      return;
    }
    if (typeof init !== "object" || init === null) {
      throw new TypeError(
        "Failed to construct 'Headers': The provided value is not of type '(sequence<sequence<ByteString>> or record<ByteString, ByteString>)'"
      );
    }

    if (init instanceof Headers) {
      init._map.forEach(function (v, k) {
        this._map.set(k, v);
      }, this);
      return;
    }

    // If init is iterable and not a string: follow Web IDL single-read sequence conversion
    if (typeof init[Symbol.iterator] === "function") {
      const it = init[Symbol.iterator]();
      if (!it || typeof it !== "object") {
        throw new TypeError("Failed to construct 'Headers': The provided value is not iterable");
      }
      const nextMethod = it.next;
      if (typeof nextMethod !== "function") {
        throw new TypeError("Failed to construct 'Headers': The provided value is not iterable");
      }
      while (true) {
        const step = nextMethod.call(it);
        if (step.done) break;
        const item = step.value;
        let pair = null;
        if (Array.isArray(item)) {
          pair = item;
        } else if (item && typeof item === "object" && typeof item[Symbol.iterator] === "function") {
          const subIt = item[Symbol.iterator]();
          if (!subIt || typeof subIt !== "object") {
            throw new TypeError("Failed to construct 'Headers': Value in sequence is not iterable");
          }
          const subNext = subIt.next;
          if (typeof subNext !== "function") {
            throw new TypeError("Failed to construct 'Headers': Value in sequence is not iterable");
          }
          const items = [];
          while (true) {
            const subStep = subNext.call(subIt);
            if (subStep.done) break;
            items.push(subStep.value);
            if (items.length > 2) break;
          }
          pair = items;
        } else {
          throw new TypeError("Failed to construct 'Headers': Value in sequence is not a sequence");
        }

        if (pair.length !== 2) {
          throw new TypeError("Failed to construct 'Headers': Each header entry must be a 2-element sequence");
        }
        this.append(pair[0], pair[1]);
      }
      return;
    }

    // Plain record / dictionary: iterate own enumerable properties
    const keys = Object.keys(init);
    for (let i = 0; i < keys.length; i++) {
      const k = keys[i];
      this.set(k, init[k]);
    }
  }

  Headers.prototype.append = function (k, v) {
    const key = String(k).toLowerCase();
    const val = String(v);
    const existing = this._map.get(key);
    this._map.set(key, existing ? existing + ", " + val : val);
  };

  Headers.prototype.set = function (k, v) {
    this._map.set(String(k).toLowerCase(), String(v));
  };

  Headers.prototype.get = function (k) {
    const key = String(k).toLowerCase();
    return this._map.has(key) ? this._map.get(key) : null;
  };

  Headers.prototype.has = function (k) {
    return this._map.has(String(k).toLowerCase());
  };

  Headers.prototype.delete = function (k) {
    this._map.delete(String(k).toLowerCase());
  };

  Headers.prototype.forEach = function (cb, thisArg) {
    this._map.forEach(cb, thisArg);
  };

  Headers.prototype.entries = function () {
    return this._map.entries();
  };

  Headers.prototype.keys = function () {
    return this._map.keys();
  };

  Headers.prototype.values = function () {
    return this._map.values();
  };

  Headers.prototype[Symbol.iterator] = function () {
    return this._map.entries();
  };

  markNative(Headers, "Headers");
  markNative(Headers.prototype.append, "append");
  markNative(Headers.prototype.set, "set");
  markNative(Headers.prototype.get, "get");
  markNative(Headers.prototype.has, "has");
  markNative(Headers.prototype.delete, "delete");
  markNative(Headers.prototype.forEach, "forEach");
  markNative(Headers.prototype.entries, "entries");
  markNative(Headers.prototype.keys, "keys");
  markNative(Headers.prototype.values, "values");

  function Request(input, init) {
    init = init || {};
    this.url = String(input);
    this.method = (init.method || "GET").toUpperCase();
    this.headers = new Headers(init.headers);
    this._rawMimeType = extractMimeFromHeaders(this.headers);
    this._body = init.body !== undefined ? init.body : "";
  }

  Request.prototype.blob = function () {
    const mime = this._rawMimeType || "";
    const parts = this._body != null ? [this._body] : [];
    const blob = new Blob(parts, { _rawType: mime, type: mime });
    const promise = Promise.resolve(blob);
    promise._blob = blob;
    Object.defineProperty(promise, "type", {
      get: function () { return blob.type; },
      configurable: true,
    });
    return promise;
  };

  Request.prototype.text = function () {
    return Promise.resolve(String(this._body));
  };

  Request.prototype.json = function () {
    return Promise.resolve(JSON.parse(String(this._body)));
  };

  Request.prototype.arrayBuffer = function () {
    return Promise.resolve(stringToUtf8(String(this._body)).buffer);
  };

  markNative(Request, "Request");
  markNative(Request.prototype.blob, "blob");
  markNative(Request.prototype.text, "text");
  markNative(Request.prototype.json, "json");
  markNative(Request.prototype.arrayBuffer, "arrayBuffer");

  function Response(body, init) {
    init = init || {};
    this.status = init.status !== undefined ? Number(init.status) : 200;
    this.statusText = init.statusText !== undefined ? String(init.statusText) : "OK";
    this.headers = new Headers(init.headers);
    this._rawMimeType = extractMimeFromHeaders(this.headers);
    this._body = body !== undefined ? body : "";
  }

  Response.prototype.blob = function () {
    const mime = this._rawMimeType || "";
    const parts = this._body != null ? [this._body] : [];
    const blob = new Blob(parts, { _rawType: mime, type: mime });
    const promise = Promise.resolve(blob);
    promise._blob = blob;
    Object.defineProperty(promise, "type", {
      get: function () { return blob.type; },
      configurable: true,
    });
    return promise;
  };

  Response.prototype.text = function () {
    return Promise.resolve(String(this._body));
  };

  Response.prototype.json = function () {
    return Promise.resolve(JSON.parse(String(this._body)));
  };

  Response.prototype.arrayBuffer = function () {
    return Promise.resolve(stringToUtf8(String(this._body)).buffer);
  };

  markNative(Response, "Response");
  markNative(Response.prototype.blob, "blob");
  markNative(Response.prototype.text, "text");
  markNative(Response.prototype.json, "json");
  markNative(Response.prototype.arrayBuffer, "arrayBuffer");

  // If Request or Response were already defined by environment, enhance their .blob()
  function patchExistingBodyBlob(Ctor) {
    if (Ctor && Ctor.prototype && typeof Ctor.prototype.blob === "function") {
      const origBlob = Ctor.prototype.blob;
      Ctor.prototype.blob = function () {
        const self = this;
        return origBlob.call(self).then(function (b) {
          if (self._rawMimeType != null) {
            b._type = self._rawMimeType;
          } else {
            try {
              const ct = self.headers && self.headers.get ? self.headers.get("content-type") : null;
              if (ct) {
                const mime = parseAndSerializeMimeType(ct);
                if (mime) b._type = mime;
              }
            } catch (e) {}
          }
          return b;
        });
      };
    }
  }

  // ... (existing code, added at the end)

  function TextDecoder(label, options) {
    let enc = (label || 'utf-8').trim().toLowerCase();
    if (enc === 'utf8') enc = 'utf-8';
    this.label = enc;
    this.encoding = enc;
    this.fatal = options ? !!options.fatal : false;
    this.ignoreBOM = options ? !!options.ignoreBOM : false;
  }

  TextDecoder.prototype.decode = function(input, options) {
    if (input === undefined || input === null) {
      return "";
    }
    let label = (this.label || 'utf-8').trim().toLowerCase();
    if (label === 'utf8') label = 'utf-8';

    let bytes;
    if (input instanceof ArrayBuffer) {
      bytes = new Uint8Array(input);
    } else if (ArrayBuffer.isView(input)) {
      bytes = new Uint8Array(input.buffer, input.byteOffset, input.byteLength);
    } else {
      throw new TypeError("The provided value is not of type '(ArrayBufferView or ArrayBuffer)'");
    }

    if (label === 'utf-8') {
      return utf8ToString(bytes);
    } else if (label === 'latin1' || label === 'iso-8859-1' || label === 'windows-1252') {
      let str = "";
      for (let i = 0; i < bytes.length; i++) {
        str += String.fromCharCode(bytes[i]);
      }
      return str;
    }

    throw new Error('Encoding not supported: ' + this.label);
  };

  markNative(TextDecoder, "TextDecoder");
  markNative(TextDecoder.prototype.decode, "decode");

  function TextEncoder() {
    this.encoding = "utf-8";
  }
  TextEncoder.prototype.encode = function (input) {
    return stringToUtf8(input === undefined ? "" : String(input));
  };
  markNative(TextEncoder, "TextEncoder");
  markNative(TextEncoder.prototype.encode, "encode");

  // -------------------------------------------------------------
  // FileReader
  // -------------------------------------------------------------
  const _b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  function base64Encode(bytes) {
    let result = "";
    let i = 0;
    const len = bytes.length;
    for (; i + 2 < len; i += 3) {
      result += _b64chars[bytes[i] >> 2];
      result += _b64chars[((bytes[i] & 3) << 4) | (bytes[i + 1] >> 4)];
      result += _b64chars[((bytes[i + 1] & 15) << 2) | (bytes[i + 2] >> 6)];
      result += _b64chars[bytes[i + 2] & 63];
    }
    if (i < len) {
      result += _b64chars[bytes[i] >> 2];
      if (i + 1 < len) {
        result += _b64chars[((bytes[i] & 3) << 4) | (bytes[i + 1] >> 4)];
        result += _b64chars[(bytes[i + 1] & 15) << 2];
        result += "=";
      } else {
        result += _b64chars[(bytes[i] & 3) << 4];
        result += "==";
      }
    }
    return result;
  }

  function _safeQueueMicrotask(fn) {
    if (typeof queueMicrotask === "function") {
      queueMicrotask(fn);
    } else {
      Promise.resolve().then(fn);
    }
  }

  function FileReader() {
    if (!(this instanceof FileReader)) {
      throw new TypeError("Failed to construct 'FileReader': Please use the 'new' operator.");
    }
    EventTarget.call(this);
    this._gen = 0;
    this.readyState = 0;
    this.result = null;
    this.error = null;
    this.onloadstart = null;
    this.onprogress = null;
    this.onload = null;
    this.onabort = null;
    this.onerror = null;
    this.onloadend = null;
  }

  FileReader.EMPTY = 0;
  FileReader.LOADING = 1;
  FileReader.DONE = 2;

  FileReader.prototype = Object.create(EventTarget.prototype);
  FileReader.prototype.constructor = FileReader;

  FileReader.prototype.EMPTY = 0;
  FileReader.prototype.LOADING = 1;
  FileReader.prototype.DONE = 2;

  FileReader.prototype._fire = function (type, detail) {
    let event;
    try {
      event = new Event(type, { bubbles: false, cancelable: false });
      if (detail) {
        for (const k in detail) {
          if (Object.prototype.hasOwnProperty.call(detail, k)) {
            event[k] = detail[k];
          }
        }
      }
    } catch (e) {
      event = { type: type, bubbles: false, cancelable: false };
      if (detail) {
        for (const k in detail) {
          if (Object.prototype.hasOwnProperty.call(detail, k)) {
            event[k] = detail[k];
          }
        }
      }
    }
    const handler = this["on" + type];
    if (typeof handler === "function") {
      try {
        handler.call(this, event);
      } catch (e) {
        if (typeof console !== "undefined" && console.error) console.error(e);
      }
    }
    try {
      this.dispatchEvent(event);
    } catch (e) {}
  };

  FileReader.prototype._read = function (blob, readType, encoding) {
    const readGeneration = ++this._gen;
    const later = (callback) => {
      setTimeout(() => {
        setTimeout(() => {
          if (this._gen !== readGeneration) return;
          callback();
        }, 0);
      }, 0);
    };
    const fireLoadendLater = () => later(() => this._fire("loadend"));

    if (blob == null || typeof blob.arrayBuffer !== "function") {
      let error;
      const DOMEx = (typeof window !== "undefined" && window.DOMException) ||
                    (typeof globalThis !== "undefined" && globalThis.DOMException);
      if (typeof DOMEx === "function") {
        error = new DOMEx("Argument is not a Blob", "TypeError");
      } else {
        error = new TypeError("FileReader: argument is not a Blob");
      }
      this.error = error;
      this.readyState = 2; // DONE
      _safeQueueMicrotask(() => {
        if (this._gen !== readGeneration) return;
        this._fire("error");
        fireLoadendLater();
      });
      return;
    }

    this.readyState = 1; // LOADING
    this.result = null;
    this.error = null;

    _safeQueueMicrotask(() => {
      if (this._gen !== readGeneration) return;
      this._fire("loadstart");
    });

    Promise.resolve()
      .then(() => blob.arrayBuffer())
      .then((buffer) => {
        if (this._gen !== readGeneration) return;
        const bytes = new Uint8Array(buffer);
        let result;
        if (readType === "arrayBuffer") {
          result = buffer;
        } else if (readType === "text") {
          try {
            const decoder = new TextDecoder(encoding || "utf-8");
            result = decoder.decode(bytes);
          } catch (e) {
            result = "";
          }
        } else if (readType === "binaryString") {
          let binaryString = "";
          for (let i = 0; i < bytes.length; i++) {
            binaryString += String.fromCharCode(bytes[i]);
          }
          result = binaryString;
        } else {
          let binaryString = "";
          for (let i = 0; i < bytes.length; i++) {
            binaryString += String.fromCharCode(bytes[i]);
          }
          const base64 = (typeof btoa === "function") ? btoa(binaryString) : base64Encode(bytes);
          const mediaType = (blob.type && String(blob.type)) || "application/octet-stream";
          result = "data:" + mediaType + ";base64," + base64;
        }

        this._fire("progress", {
          lengthComputable: true,
          loaded: bytes.length,
          total: bytes.length,
        });

        later(() => {
          this.result = result;
          this.readyState = 2; // DONE
          this._fire("load");
          fireLoadendLater();
        });
      })
      .catch((error) => {
        if (this._gen !== readGeneration) return;
        this.error = error;
        this.readyState = 2; // DONE
        this._fire("error");
        fireLoadendLater();
      });
  };

  FileReader.prototype.readAsText = function (blob, encoding) {
    this._read(blob, "text", encoding);
  };
  FileReader.prototype.readAsArrayBuffer = function (blob) {
    this._read(blob, "arrayBuffer");
  };
  FileReader.prototype.readAsDataURL = function (blob) {
    this._read(blob, "dataURL");
  };
  FileReader.prototype.readAsBinaryString = function (blob) {
    this._read(blob, "binaryString");
  };

  FileReader.prototype.abort = function () {
    if (this.readyState === 0) {
      this.result = null;
      return;
    }
    if (this.readyState === 2) return;
    this._gen++;
    this.readyState = 2;
    this.result = null;
    this._fire("abort");
    this._fire("loadend");
  };

  markNative(FileReader, "FileReader");
  markNative(FileReader.prototype.readAsText, "readAsText");
  markNative(FileReader.prototype.readAsArrayBuffer, "readAsArrayBuffer");
  markNative(FileReader.prototype.readAsDataURL, "readAsDataURL");
  markNative(FileReader.prototype.readAsBinaryString, "readAsBinaryString");
  markNative(FileReader.prototype.abort, "abort");

  // Expose globals
  global.Blob = Blob;
  global.File = File;
  global.FileReader = FileReader;
  global.TextDecoder = TextDecoder;
  global.TextEncoder = TextEncoder;

  global.Headers = Headers;
  if (!global.Request) global.Request = Request; else patchExistingBodyBlob(global.Request);
  if (!global.Response) global.Response = Response; else patchExistingBodyBlob(global.Response);

  if (typeof window !== "undefined") {
    window.Blob = Blob;
    window.File = File;
    window.FileReader = FileReader;
    window.TextDecoder = TextDecoder;
    window.TextEncoder = TextEncoder;
    window.Headers = Headers;
    window.Request = Request;
    window.Response = Response;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.Blob = Blob;
    globalThis.File = File;
    globalThis.FileReader = FileReader;
    globalThis.TextDecoder = TextDecoder;
    globalThis.TextEncoder = TextEncoder;
    globalThis.Headers = Headers;
    globalThis.Request = Request;
    globalThis.Response = Response;
  }


  // =============================================================
  // Module: form-data.js
  // =============================================================
  // src/js/shim/form-data.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/form-data.ts

  function FormData(form) {
    this._entries = []; // Array of [name, value]
    if (form && (form.tagName || "").toUpperCase() === "FORM") {
      const controls = form.elements;
      for (let i = 0; i < controls.length; i++) {
        const c = controls[i];
        if (!c.name || c.disabled) continue;
        const tag = (c.tagName || "").toLowerCase();
        const type = (c.type || "").toLowerCase();

        if (type === "submit" || type === "reset" || type === "button" || type === "image") {
          continue;
        }
        if ((type === "checkbox" || type === "radio") && !c.checked) {
          continue;
        }
        if (tag === "select") {
          const opts = c.options;
          for (let j = 0; j < opts.length; j++) {
            if (opts[j].selected && !opts[j].disabled) {
              this.append(c.name, opts[j].value);
            }
          }
        } else {
          this.append(c.name, c.value);
        }
      }
    }
  }

  FormData.prototype.append = function (name, value, filename) {
    this._entries.push([String(name), String(value)]);
  };

  FormData.prototype.delete = function (name) {
    const n = String(name);
    this._entries = this._entries.filter(function (entry) {
      return entry[0] !== n;
    });
  };

  FormData.prototype.get = function (name) {
    const n = String(name);
    for (let i = 0; i < this._entries.length; i++) {
      if (this._entries[i][0] === n) {
        return this._entries[i][1];
      }
    }
    return null;
  };

  FormData.prototype.getAll = function (name) {
    const n = String(name);
    const res = [];
    for (let i = 0; i < this._entries.length; i++) {
      if (this._entries[i][0] === n) {
        res.push(this._entries[i][1]);
      }
    }
    return res;
  };

  FormData.prototype.has = function (name) {
    const n = String(name);
    for (let i = 0; i < this._entries.length; i++) {
      if (this._entries[i][0] === n) return true;
    }
    return false;
  };

  FormData.prototype.set = function (name, value, filename) {
    const n = String(name);
    const val = String(value);
    let found = false;
    const newEntries = [];
    for (let i = 0; i < this._entries.length; i++) {
      if (this._entries[i][0] === n) {
        if (!found) {
          newEntries.push([n, val]);
          found = true;
        }
      } else {
        newEntries.push(this._entries[i]);
      }
    }
    if (!found) {
      newEntries.push([n, val]);
    }
    this._entries = newEntries;
  };

  FormData.prototype.entries = function () {
    return this._entries.slice()[Symbol.iterator]();
  };

  FormData.prototype.keys = function () {
    return this._entries.map(function (e) { return e[0]; })[Symbol.iterator]();
  };

  FormData.prototype.values = function () {
    return this._entries.map(function (e) { return e[1]; })[Symbol.iterator]();
  };

  FormData.prototype.forEach = function (callback, thisArg) {
    for (let i = 0; i < this._entries.length; i++) {
      callback.call(thisArg, this._entries[i][1], this._entries[i][0], this);
    }
  };

  FormData.prototype[Symbol.iterator] = function () {
    return this.entries();
  };

  markNative(FormData, "FormData");
  markNative(FormData.prototype.append, "append");
  markNative(FormData.prototype.delete, "delete");
  markNative(FormData.prototype.get, "get");
  markNative(FormData.prototype.getAll, "getAll");
  markNative(FormData.prototype.has, "has");
  markNative(FormData.prototype.set, "set");
  markNative(FormData.prototype.entries, "entries");

  // =============================================================
  // Module: traversal.js
  // =============================================================
  // src/js/shim/traversal.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/traversal.ts

  const NodeFilter = {
    SHOW_ALL: SHOW_ALL,
    SHOW_ELEMENT: SHOW_ELEMENT,
    SHOW_ATTRIBUTE: SHOW_ATTRIBUTE,
    SHOW_TEXT: SHOW_TEXT,
    SHOW_CDATA_SECTION: SHOW_CDATA_SECTION,
    SHOW_ENTITY_REFERENCE: SHOW_ENTITY_REFERENCE,
    SHOW_ENTITY: SHOW_ENTITY,
    SHOW_PROCESSING_INSTRUCTION: SHOW_PROCESSING_INSTRUCTION,
    SHOW_COMMENT: SHOW_COMMENT,
    SHOW_DOCUMENT: SHOW_DOCUMENT,
    SHOW_DOCUMENT_TYPE: SHOW_DOCUMENT_TYPE,
    SHOW_DOCUMENT_FRAGMENT: SHOW_DOCUMENT_FRAGMENT,
    SHOW_NOTATION: SHOW_NOTATION,
    FILTER_ACCEPT: FILTER_ACCEPT,
    FILTER_REJECT: FILTER_REJECT,
    FILTER_SKIP: FILTER_SKIP,
  };

  function filterNode(node, whatToShow, filter) {
    const type = node.nodeType;
    const bit = 1 << (type - 1);
    if (!(whatToShow & bit)) {
      return FILTER_SKIP;
    }
    if (!filter) return FILTER_ACCEPT;
    if (typeof filter === "function") {
      return filter(node);
    }
    if (typeof filter.acceptNode === "function") {
      return filter.acceptNode(node);
    }
    return FILTER_ACCEPT;
  }

  function TreeWalker(root, whatToShow, filter) {
    if (!root) throw new TypeError("Root node required");
    this.root = root;
    this.whatToShow = whatToShow !== undefined ? Number(whatToShow) : SHOW_ALL;
    this.filter = filter || null;
    this.currentNode = root;
  }

  TreeWalker.prototype.parentNode = function () {
    let node = this.currentNode;
    while (node && node !== this.root) {
      node = node.parentNode || node.parentElement;
      if (node) {
        const res = filterNode(node, this.whatToShow, this.filter);
        if (res === FILTER_ACCEPT) {
          this.currentNode = node;
          return node;
        }
      }
    }
    return null;
  };

  TreeWalker.prototype.firstChild = function () {
    let node = this.currentNode.firstChild;
    while (node) {
      const res = filterNode(node, this.whatToShow, this.filter);
      if (res === FILTER_ACCEPT) {
        this.currentNode = node;
        return node;
      }
      if (res === FILTER_SKIP && node.firstChild) {
        node = node.firstChild;
        continue;
      }
      node = node.nextSibling;
    }
    return null;
  };

  TreeWalker.prototype.lastChild = function () {
    let node = this.currentNode.lastChild;
    while (node) {
      const res = filterNode(node, this.whatToShow, this.filter);
      if (res === FILTER_ACCEPT) {
        this.currentNode = node;
        return node;
      }
      if (res === FILTER_SKIP && node.lastChild) {
        node = node.lastChild;
        continue;
      }
      node = node.previousSibling;
    }
    return null;
  };

  TreeWalker.prototype.previousSibling = function () {
    let node = this.currentNode;
    if (node === this.root) return null;
    while (node) {
      let sibling = node.previousSibling;
      while (sibling) {
        const res = filterNode(sibling, this.whatToShow, this.filter);
        if (res === FILTER_ACCEPT) {
          this.currentNode = sibling;
          return sibling;
        }
        sibling = sibling.lastChild || sibling.previousSibling;
      }
      node = node.parentNode || node.parentElement;
      if (!node || node === this.root) return null;
      if (filterNode(node, this.whatToShow, this.filter) === FILTER_ACCEPT) return null;
    }
    return null;
  };

  TreeWalker.prototype.nextSibling = function () {
    let node = this.currentNode;
    if (node === this.root) return null;
    while (node) {
      let sibling = node.nextSibling;
      while (sibling) {
        const res = filterNode(sibling, this.whatToShow, this.filter);
        if (res === FILTER_ACCEPT) {
          this.currentNode = sibling;
          return sibling;
        }
        sibling = sibling.firstChild || sibling.nextSibling;
      }
      node = node.parentNode || node.parentElement;
      if (!node || node === this.root) return null;
      if (filterNode(node, this.whatToShow, this.filter) === FILTER_ACCEPT) return null;
    }
    return null;
  };

  TreeWalker.prototype.nextNode = function () {
    let node = this.currentNode;
    while (node) {
      let next = node.firstChild;
      while (!next && node && node !== this.root) {
        next = node.nextSibling;
        node = node.parentNode || node.parentElement;
      }
      if (!next) return null;
      node = next;
      const res = filterNode(node, this.whatToShow, this.filter);
      if (res === FILTER_ACCEPT) {
        this.currentNode = node;
        return node;
      }
    }
    return null;
  };

  TreeWalker.prototype.previousNode = function () {
    let node = this.currentNode;
    if (node === this.root) return null;
    while (node && node !== this.root) {
      let prev = node.previousSibling;
      if (prev) {
        while (prev.lastChild) prev = prev.lastChild;
        node = prev;
      } else {
        node = node.parentNode || node.parentElement;
      }
      if (!node) return null;
      const res = filterNode(node, this.whatToShow, this.filter);
      if (res === FILTER_ACCEPT) {
        this.currentNode = node;
        return node;
      }
    }
    return null;
  };

  markNative(TreeWalker, "TreeWalker");
  markNative(TreeWalker.prototype.parentNode, "parentNode");
  markNative(TreeWalker.prototype.firstChild, "firstChild");
  markNative(TreeWalker.prototype.lastChild, "lastChild");
  markNative(TreeWalker.prototype.previousSibling, "previousSibling");
  markNative(TreeWalker.prototype.nextSibling, "nextSibling");
  markNative(TreeWalker.prototype.nextNode, "nextNode");
  markNative(TreeWalker.prototype.previousNode, "previousNode");

  function NodeIterator(root, whatToShow, filter) {
    if (!root) throw new TypeError("Root node required");
    this.root = root;
    this.whatToShow = whatToShow !== undefined ? Number(whatToShow) : SHOW_ALL;
    this.filter = filter || null;
    this.referenceNode = root;
    this.pointerBeforeReferenceNode = true;
    this._walker = new TreeWalker(root, this.whatToShow, this.filter);
  }

  NodeIterator.prototype.nextNode = function () {
    const node = this._walker.nextNode();
    if (node) {
      this.referenceNode = node;
      this.pointerBeforeReferenceNode = false;
    }
    return node;
  };

  NodeIterator.prototype.previousNode = function () {
    const node = this._walker.previousNode();
    if (node) {
      this.referenceNode = node;
      this.pointerBeforeReferenceNode = true;
    }
    return node;
  };

  NodeIterator.prototype.detach = function () {};

  markNative(NodeIterator, "NodeIterator");
  markNative(NodeIterator.prototype.nextNode, "nextNode");
  markNative(NodeIterator.prototype.previousNode, "previousNode");
  markNative(NodeIterator.prototype.detach, "detach");

  // =============================================================
  // Module: range.js
  // =============================================================
  // src/js/shim/range.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/range.ts

  function Range() {
    this.startContainer = typeof document !== "undefined" ? document : null;
    this.startOffset = 0;
    this.endContainer = this.startContainer;
    this.endOffset = 0;
    this.collapsed = true;
    this.commonAncestorContainer = this.startContainer;
  }

  Range.START_TO_START = 0;
  Range.START_TO_END = 1;
  Range.END_TO_END = 2;
  Range.END_TO_START = 3;

  Range.prototype.setStart = function (node, offset) {
    this.startContainer = node;
    this.startOffset = Number(offset || 0);
    this._updateState();
  };

  Range.prototype.setEnd = function (node, offset) {
    this.endContainer = node;
    this.endOffset = Number(offset || 0);
    this._updateState();
  };

  Range.prototype.setStartBefore = function (node) {
    if (!node || !node.parentNode) throw new Error("Parent node required");
    this.setStart(node.parentNode, 0);
  };

  Range.prototype.setStartAfter = function (node) {
    if (!node || !node.parentNode) throw new Error("Parent node required");
    this.setStart(node.parentNode, 1);
  };

  Range.prototype.setEndBefore = function (node) {
    if (!node || !node.parentNode) throw new Error("Parent node required");
    this.setEnd(node.parentNode, 0);
  };

  Range.prototype.setEndAfter = function (node) {
    if (!node || !node.parentNode) throw new Error("Parent node required");
    this.setEnd(node.parentNode, 1);
  };

  Range.prototype.collapse = function (toStart) {
    if (toStart) {
      this.endContainer = this.startContainer;
      this.endOffset = this.startOffset;
    } else {
      this.startContainer = this.endContainer;
      this.startOffset = this.endOffset;
    }
    this.collapsed = true;
  };

  Range.prototype.selectNode = function (node) {
    if (!node || !node.parentNode) throw new Error("Parent node required");
    this.setStartBefore(node);
    this.setEndAfter(node);
  };

  Range.prototype.selectNodeContents = function (node) {
    if (!node) throw new Error("Node required");
    this.startContainer = node;
    this.startOffset = 0;
    this.endContainer = node;
    this.endOffset = node.textContent ? node.textContent.length : 0;
    this._updateState();
  };

  Range.prototype.compareBoundaryPoints = function (how, sourceRange) {
    return 0;
  };

  Range.prototype.deleteContents = function () {};
  Range.prototype.extractContents = function () {
    return new DocumentFragment();
  };
  Range.prototype.cloneContents = function () {
    return new DocumentFragment();
  };
  Range.prototype.insertNode = function (node) {
    if (this.startContainer && this.startContainer.appendChild) {
      this.startContainer.appendChild(node);
    }
  };
  Range.prototype.surroundContents = function (newParent) {
    if (newParent && this.startContainer && this.startContainer.appendChild) {
      this.startContainer.appendChild(newParent);
    }
  };

  Range.prototype.cloneRange = function () {
    const r = new Range();
    r.startContainer = this.startContainer;
    r.startOffset = this.startOffset;
    r.endContainer = this.endContainer;
    r.endOffset = this.endOffset;
    r.collapsed = this.collapsed;
    r.commonAncestorContainer = this.commonAncestorContainer;
    return r;
  };

  Range.prototype.detach = function () {};

  const _nativeSetInnerHtml = (typeof Element !== "undefined" && Object.getOwnPropertyDescriptor(Element.prototype, "innerHTML"))?.set;
  const _nativeCreateElement = (typeof document !== "undefined" && document.createElement) ? document.createElement.bind(document) : null;

  Range.prototype.createContextualFragment = function (fragmentHtml) {
    const node = this.startContainer;
    const doc = (node && (node.ownerDocument || (node.nodeType === 9 ? node : null))) || (typeof document !== "undefined" ? document : null);
    if (!doc) {
      throw new TypeError("createContextualFragment: start container has no document");
    }

    // 1. Determine context element
    let element = null;
    if (node && node.nodeType === 1) { // ELEMENT_NODE
      element = node;
    } else if (node && (node.nodeType === 3 || node.nodeType === 8)) { // TEXT_NODE or COMMENT_NODE
      element = node.parentElement || node.parentNode;
      if (element && element.nodeType !== 1) element = null;
    }

    if (!element || String(element.localName || element.tagName || "").toLowerCase() === "html") {
      element = doc.body || (doc.documentElement ? doc.documentElement : null);
    }

    const tag = element ? String(element.localName || element.tagName || "").toLowerCase() : "body";
    const ns = (element && element._namespaceURI) ? element._namespaceURI : "http://www.w3.org/1999/xhtml";

    // 2. Format markup (null becomes "null")
    const markup = fragmentHtml === null ? "null" : String(fragmentHtml);

    // 3. Demand SVG font outlines if SVG text is present
    let inSvg = (tag === "svg" || ns === "http://www.w3.org/2000/svg");
    if (!inSvg && element) {
      let p = element.parentElement || element.parentNode;
      while (p) {
        if (String(p.localName || p.tagName || "").toLowerCase() === "svg") {
          inSvg = true;
          break;
        }
        p = p.parentElement || p.parentNode;
      }
    }

    if (
      markup.length !== 0 &&
      /<text[\s/>]/i.test(markup) &&
      (/<svg[\s/>]/i.test(markup) || inSvg)
    ) {
      try {
        const ksInternals = globalThis[Symbol.for("kitesurf.internals")];
        if (ksInternals && typeof ksInternals.demandFontOutlines === "function") {
          ksInternals.demandFontOutlines();
        }
      } catch (e) {}
    }

    // 4. Wrap markup according to context
    let prefix = "";
    let suffix = "";
    let depth = 0;
    let cleanMarkup = markup;
    const isTableContext = (
      tag === "tbody" || tag === "thead" || tag === "tfoot" ||
      tag === "tr" || tag === "td" || tag === "th" ||
      tag === "col" || tag === "colgroup" || tag === "caption" ||
      tag === "table" || tag === "template"
    );
    if (!isTableContext && /^\s*<tr[\s>]/i.test(markup)) {
      cleanMarkup = markup.replace(/<\/?(?:table|tbody|thead|tfoot|tr|td|th|colgroup|col|caption)[^>]*>/gi, "");
    } else if (tag === "tbody" || tag === "thead" || tag === "tfoot") {
      prefix = "<table><" + tag + ">";
      suffix = "</" + tag + "></table>";
      depth = 2;
    } else if (tag === "tr") {
      prefix = "<table><tbody><tr>";
      suffix = "</tr></tbody></table>";
      depth = 3;
    } else if (tag === "td" || tag === "th") {
      prefix = "<table><tbody><tr><" + tag + ">";
      suffix = "</" + tag + "></tr></tbody></table>";
      depth = 4;
    } else if (tag === "col") {
      prefix = "<table><colgroup>";
      suffix = "</colgroup></table>";
      depth = 2;
    } else if (tag === "colgroup" || tag === "caption") {
      prefix = "<table>";
      suffix = "</table>";
      depth = 1;
    } else if (tag === "template") {
      prefix = "<template>";
      suffix = "</template>";
      depth = 1;
    } else if (tag === "select" || tag === "optgroup") {
      prefix = "<select>";
      suffix = "</select>";
      depth = 1;
    } else if (tag === "svg") {
      prefix = "<svg>";
      suffix = "</svg>";
      depth = 1;
    }

    const fullHtml = prefix + cleanMarkup + suffix;

    // 5. Parse in a temporary container using native innerHTML setter
    const mkEl = _nativeCreateElement || (typeof document !== "undefined" && document.createElement ? document.createElement.bind(document) : null);
    const temp = mkEl ? mkEl("div") : (doc.createElement ? doc.createElement("div") : null);

    if (temp) {
      if (_nativeSetInnerHtml) {
        _nativeSetInnerHtml.call(temp, fullHtml);
      } else {
        temp.innerHTML = fullHtml;
      }
    }

    // 6. Navigate to target at the specified depth
    let target = temp;
    for (let d = 0; d < depth && target; d++) {
      target = target.firstElementChild || target.firstChild;
    }
    if (!target) target = temp;

    let source = target;
    if (target && String(target.localName || target.tagName || "").toLowerCase() === "template") {
      if (target.content && target.content.firstChild) {
        source = target.content;
      }
    }

    // 7. Move nodes to a new DocumentFragment
    const frag = new DocumentFragment();
    frag.ownerDocument = doc;

    while (source && source.firstChild) {
      const ch = source.firstChild;
      frag.appendChild(typeof upgradeElement === "function" ? upgradeElement(ch) : ch);
    }

    // 8. Mark scripts within fragment as executable
    function markExecutable(n) {
      if (!n || typeof n !== "object") return;
      const t = String(n.localName || n.tagName || "").toLowerCase();
      if (t === "script") {
        n._alreadyStarted = false;
      }
      const kids = n.childNodes || n._children;
      if (kids && kids.length) {
        for (let i = 0; i < kids.length; i++) {
          markExecutable(kids[i]);
        }
      }
    }
    markExecutable(frag);

    return frag;
  };

  if (typeof DocumentFragment !== "undefined" && DocumentFragment.prototype) {
    if (!Object.getOwnPropertyDescriptor(DocumentFragment.prototype, "textContent")) {
      Object.defineProperty(DocumentFragment.prototype, "textContent", {
        get: function () {
          let text = "";
          const kids = this.childNodes || this._children || [];
          for (let i = 0; i < kids.length; i++) {
            const k = kids[i];
            if (k.nodeType === 3) text += k.data || k.textContent || "";
            else if (k.nodeType === 1) text += k.textContent || "";
          }
          return text;
        },
        set: function (val) {
          while (this.firstChild) {
            this.removeChild(this.firstChild);
          }
          if (val) {
            this.appendChild(new Text(val));
          }
        },
        configurable: true,
      });
    }
  }

  Range.prototype._updateState = function () {
    this.collapsed = (this.startContainer === this.endContainer && this.startOffset === this.endOffset);
    this.commonAncestorContainer = this.startContainer;
  };

  markNative(Range, "Range");
  markNative(Range.prototype.setStart, "setStart");
  markNative(Range.prototype.setEnd, "setEnd");
  markNative(Range.prototype.collapse, "collapse");
  markNative(Range.prototype.selectNode, "selectNode");
  markNative(Range.prototype.selectNodeContents, "selectNodeContents");
  markNative(Range.prototype.cloneRange, "cloneRange");
  markNative(Range.prototype.createContextualFragment, "createContextualFragment");

  function Selection() {
    this._range = null;
  }

  Object.defineProperty(Selection.prototype, "anchorNode", {
    get: function () { return this._range ? this._range.startContainer : null; },
    configurable: true,
  });

  Object.defineProperty(Selection.prototype, "anchorOffset", {
    get: function () { return this._range ? this._range.startOffset : 0; },
    configurable: true,
  });

  Object.defineProperty(Selection.prototype, "focusNode", {
    get: function () { return this._range ? this._range.endContainer : null; },
    configurable: true,
  });

  Object.defineProperty(Selection.prototype, "focusOffset", {
    get: function () { return this._range ? this._range.endOffset : 0; },
    configurable: true,
  });

  Object.defineProperty(Selection.prototype, "isCollapsed", {
    get: function () { return this._range ? this._range.collapsed : true; },
    configurable: true,
  });

  Object.defineProperty(Selection.prototype, "rangeCount", {
    get: function () { return this._range ? 1 : 0; },
    configurable: true,
  });

  Selection.prototype.getRangeAt = function (index) {
    if (index === 0 && this._range) return this._range;
    throw new Error("IndexSizeError: Invalid index " + index);
  };

  Selection.prototype.addRange = function (range) {
    this._range = range;
  };

  Selection.prototype.removeRange = function (range) {
    if (this._range === range) this._range = null;
  };

  Selection.prototype.removeAllRanges = function () {
    this._range = null;
  };

  Selection.prototype.empty = function () {
    this._range = null;
  };

  Selection.prototype.collapse = function (node, offset) {
    if (!this._range) this._range = new Range();
    this._range.setStart(node, offset);
    this._range.collapse(true);
  };

  Selection.prototype.setPosition = function (node, offset) {
    this.collapse(node, offset);
  };

  Selection.prototype.selectAllChildren = function (node) {
    if (!this._range) this._range = new Range();
    this._range.selectNodeContents(node);
  };

  Selection.prototype.toString = function () {
    return "";
  };

  markNative(Selection, "Selection");
  markNative(Selection.prototype.getRangeAt, "getRangeAt");
  markNative(Selection.prototype.addRange, "addRange");
  markNative(Selection.prototype.removeAllRanges, "removeAllRanges");

  // =============================================================
  // Module: parser.js
  // =============================================================
  // src/js/shim/parser.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/parser.ts

  const SUPPORTED_MIME_TYPES = new Set([
    "text/html",
    "text/xml",
    "application/xml",
    "application/xhtml+xml",
    "image/svg+xml",
  ]);

  const HTML_VOID_TAGS = new Set([
    "area", "base", "br", "col", "embed", "hr", "img", "input",
    "link", "meta", "param", "source", "track", "wbr",
  ]);

  function camelToDash(str) {
    return String(str).replace(/[A-Z]/g, function (m) {
      return "-" + m.toLowerCase();
    });
  }

  function dashToCamel(str) {
    return String(str).replace(/-([a-z])/g, function (_, c) {
      return c.toUpperCase();
    });
  }

  function leadingDoctype(source, html) {
    const m = /^<!doctype(?:\s+([^\s>]+))?([^>]*)>/i.exec(source);
    if (!m) return null;
    const rest = m[2] || "";
    const quoted = function (s) {
      return /^\s*("([^"]*)"|'([^']*)')/.exec(s);
    };
    let publicId = "";
    let systemId = "";
    const kw = /^\s*(public|system)/i.exec(rest);
    if (kw) {
      let after = rest.slice(kw[0].length);
      const first = quoted(after);
      if (first) {
        const value = first[2] !== undefined ? first[2] : (first[3] !== undefined ? first[3] : "");
        after = after.slice(first[0].length);
        if ((kw[1] || "").toLowerCase() === "public") {
          publicId = value;
          const second = quoted(after);
          if (second) {
            systemId = second[2] !== undefined ? second[2] : (second[3] !== undefined ? second[3] : "");
          }
        } else {
          systemId = value;
        }
      }
    }
    const name = m[1] || "";
    return {
      name: html ? name.toLowerCase() : name,
      publicId: publicId,
      systemId: systemId,
    };
  }

  function unescapeEntities(str) {
    return str.replace(/&([a-zA-Z0-9]+|#[0-9]+|#x[0-9a-fA-F]+);/g, function (match, ent) {
      if (ent === "amp") return "&";
      if (ent === "lt") return "<";
      if (ent === "gt") return ">";
      if (ent === "quot") return '"';
      if (ent === "apos") return "'";
      if (ent.startsWith("#x") || ent.startsWith("#X")) {
        const code = parseInt(ent.slice(2), 16);
        return !isNaN(code) ? String.fromCharCode(code) : match;
      }
      if (ent.startsWith("#")) {
        const code = parseInt(ent.slice(1), 10);
        return !isNaN(code) ? String.fromCharCode(code) : match;
      }
      return match;
    });
  }

  function matchesSimpleSelector(el, s) {
    if (!el || el.nodeType !== 1) return false;
    s = s.trim();
    if (s === "*") return true;

    let rest = s;
    const tagMatch = rest.match(/^[a-zA-Z0-9_-]+/);
    if (tagMatch) {
      const tag = tagMatch[0].toLowerCase();
      rest = rest.slice(tag.length);
      const elTag = (el.localName || el.tagName || "").toLowerCase();
      if (elTag !== tag) return false;
    }

    while (rest.length > 0) {
      if (rest.startsWith("#")) {
        const idMatch = rest.match(/^#([a-zA-Z0-9_-]+)/);
        if (!idMatch) return false;
        const id = idMatch[1];
        if (el.getAttribute("id") !== id && el.id !== id) return false;
        rest = rest.slice(idMatch[0].length);
      } else if (rest.startsWith(".")) {
        const clsMatch = rest.match(/^\.([a-zA-Z0-9_-]+)/);
        if (!clsMatch) return false;
        const cls = clsMatch[1];
        const clsAttr = el.getAttribute("class") || el.className || "";
        if (!clsAttr.split(/\s+/).includes(cls)) return false;
        rest = rest.slice(clsMatch[0].length);
      } else if (rest.startsWith("[")) {
        const attrMatch = rest.match(/^\[([a-zA-Z0-9_:-]+)(?:([*^$]?=)(?:"([^"]*)"|'([^']*)'|([^\]]+)))?\]/);
        if (!attrMatch) return false;
        const attrName = attrMatch[1];
        const op = attrMatch[2];
        const attrVal = attrMatch[3] !== undefined ? attrMatch[3] : (attrMatch[4] !== undefined ? attrMatch[4] : attrMatch[5]);
        if (!el.hasAttribute(attrName)) return false;
        if (op) {
          const actual = el.getAttribute(attrName) || "";
          if (op === "=" && actual !== attrVal) return false;
          if (op === "*=" && !actual.includes(attrVal)) return false;
          if (op === "^=" && !actual.startsWith(attrVal)) return false;
          if (op === "$=" && !actual.endsWith(attrVal)) return false;
        }
        rest = rest.slice(attrMatch[0].length);
      } else {
        break;
      }
    }

    return true;
  }

  function matchesSelector(el, sel) {
    if (!el || el.nodeType !== 1) return false;
    const s = sel.trim();
    if (s === "*") return true;

    if (s.includes(",")) {
      const parts = s.split(",");
      for (let i = 0; i < parts.length; i++) {
        if (matchesSelector(el, parts[i])) return true;
      }
      return false;
    }

    const parts = s.split(/\s+/);
    if (parts.length > 1) {
      if (!matchesSimpleSelector(el, parts[parts.length - 1])) return false;
      let curr = el.parentNode;
      let pIdx = parts.length - 2;
      while (curr && pIdx >= 0) {
        if (curr.nodeType === 1 && matchesSimpleSelector(curr, parts[pIdx])) {
          pIdx--;
        }
        curr = curr.parentNode;
      }
      return pIdx < 0;
    }

    return matchesSimpleSelector(el, s);
  }

  function ParsedText(data, ownerDoc) {
    this.nodeType = TEXT_NODE;
    this.nodeName = "#text";
    this.data = String(data !== undefined ? data : "");
    this.ownerDocument = ownerDoc || null;
    this.parentNode = null;
    registerParsedNode(this);
  }
  Object.defineProperty(ParsedText.prototype, "textContent", {
    get: function () { return this.data; },
    set: function (v) { this.data = String(v); },
    configurable: true,
  });
  Object.defineProperty(ParsedText.prototype, "nodeValue", {
    get: function () { return this.data; },
    set: function (v) { this.data = String(v); },
    configurable: true,
  });
  Object.defineProperty(ParsedText.prototype, "length", {
    get: function () { return this.data.length; },
    configurable: true,
  });
  Object.defineProperty(ParsedText.prototype, "parentElement", {
    get: function () { return (this.parentNode && this.parentNode.nodeType === ELEMENT_NODE) ? this.parentNode : null; },
    configurable: true,
  });

  function ParsedComment(data, ownerDoc) {
    this.nodeType = COMMENT_NODE;
    this.nodeName = "#comment";
    this.data = String(data !== undefined ? data : "");
    this.ownerDocument = ownerDoc || null;
    this.parentNode = null;
    registerParsedNode(this);
  }
  Object.defineProperty(ParsedComment.prototype, "textContent", {
    get: function () { return this.data; },
    set: function (v) { this.data = String(v); },
    configurable: true,
  });
  Object.defineProperty(ParsedComment.prototype, "nodeValue", {
    get: function () { return this.data; },
    set: function (v) { this.data = String(v); },
    configurable: true,
  });
  Object.defineProperty(ParsedComment.prototype, "length", {
    get: function () { return this.data.length; },
    configurable: true,
  });
  Object.defineProperty(ParsedComment.prototype, "parentElement", {
    get: function () { return (this.parentNode && this.parentNode.nodeType === ELEMENT_NODE) ? this.parentNode : null; },
    configurable: true,
  });

  function ParsedFragment(ownerDoc) {
    this.nodeType = DOCUMENT_FRAGMENT_NODE;
    this.nodeName = "#document-fragment";
    this.ownerDocument = ownerDoc || null;
    this.parentNode = null;
    this._children = [];
    registerParsedNode(this);
  }
  ParsedFragment.prototype.appendChild = function (child) {
    if (!child || typeof child !== "object") {
      throw new TypeError("Failed to execute 'appendChild' on 'Node': parameter 1 is not of type 'Node'.");
    }
    if (child.nodeType === DOCUMENT_FRAGMENT_NODE) {
      const kids = Array.from(child.childNodes || child._children || []);
      for (let i = 0; i < kids.length; i++) {
        this.appendChild(kids[i]);
      }
      if (child._children) child._children.length = 0;
      return child;
    }
    if (child.parentNode && typeof child.parentNode.removeChild === "function") {
      try { child.parentNode.removeChild(child); } catch (e) {}
    }
    child.parentNode = this;
    child.ownerDocument = this.ownerDocument || this;
    this._children.push(child);
    return child;
  };
  ParsedFragment.prototype.removeChild = function (child) {
    if (!child || child.parentNode !== this || this._children.indexOf(child) === -1) {
      if (typeof DOMException !== "undefined") {
        throw new DOMException("NotFoundError: Node was not found", "NotFoundError");
      }
      const err = new Error("NotFoundError: Node was not found");
      err.name = "NotFoundError";
      throw err;
    }
    const idx = this._children.indexOf(child);
    if (idx >= 0) this._children.splice(idx, 1);
    child.parentNode = null;
    return child;
  };
  Object.defineProperty(ParsedFragment.prototype, "childNodes", {
    get: function () {
      return new NodeList(this._children);
    },
    configurable: true,
  });
  Object.defineProperty(ParsedFragment.prototype, "children", {
    get: function () {
      return this._children.filter(function (n) { return n.nodeType === ELEMENT_NODE; });
    },
    configurable: true,
  });
  Object.defineProperty(ParsedFragment.prototype, "firstChild", {
    get: function () {
      return this._children.length > 0 ? this._children[0] : null;
    },
    configurable: true,
  });
  Object.defineProperty(ParsedFragment.prototype, "lastChild", {
    get: function () {
      return this._children.length > 0 ? this._children[this._children.length - 1] : null;
    },
    configurable: true,
  });

  function ParsedElement(tagName, ownerDoc) {
    this.nodeType = ELEMENT_NODE;
    const isHtml = !ownerDoc || ownerDoc.contentType === "text/html";
    this.localName = String(tagName).toLowerCase();
    this.tagName = isHtml ? String(tagName).toUpperCase() : this.localName;
    this.nodeName = this.tagName;
    this.ownerDocument = ownerDoc || null;
    this.parentNode = null;
    this._attrs = new Map();
    this._attrCases = new Map();
    this._children = [];
    registerParsedNode(this);
  }

  ParsedElement.prototype.getAttribute = function (name) {
    const k = String(name).toLowerCase();
    return this._attrs.has(k) ? this._attrs.get(k) : null;
  };

  ParsedElement.prototype.setAttribute = function (name, value) {
    const k = String(name).toLowerCase();
    const val = String(value);
    this._attrs.set(k, val);
    if (!this._attrCases.has(k)) {
      this._attrCases.set(k, String(name));
    }
  };

  ParsedElement.prototype.hasAttribute = function (name) {
    const k = String(name).toLowerCase();
    return this._attrs.has(k);
  };

  ParsedElement.prototype.removeAttribute = function (name) {
    const k = String(name).toLowerCase();
    this._attrs.delete(k);
    this._attrCases.delete(k);
  };

  ParsedElement.prototype.getAttributeNames = function () {
    return Array.from(this._attrCases.values());
  };

  Object.defineProperty(ParsedElement.prototype, "id", {
    get: function () { return this.getAttribute("id") || ""; },
    set: function (v) { this.setAttribute("id", v); },
    configurable: true,
  });

  Object.defineProperty(ParsedElement.prototype, "className", {
    get: function () { return this.getAttribute("class") || ""; },
    set: function (v) { this.setAttribute("class", v); },
    configurable: true,
  });

  Object.defineProperty(ParsedElement.prototype, "classList", {
    get: function () {
      const cls = this.className;
      const tokens = cls.split(/\s+/).filter(Boolean);
      const self = this;
      tokens.contains = function (t) { return tokens.indexOf(t) >= 0; };
      tokens.add = function (t) {
        if (!tokens.contains(t)) {
          tokens.push(t);
          self.className = tokens.join(" ");
        }
      };
      tokens.remove = function (t) {
        const i = tokens.indexOf(t);
        if (i >= 0) {
          tokens.splice(i, 1);
          self.className = tokens.join(" ");
        }
      };
      return tokens;
    },
    configurable: true,
  });

  Object.defineProperty(ParsedElement.prototype, "dataset", {
    get: function () {
      const el = this;
      return new Proxy(Object.create(null), {
        get: function (target, prop) {
          if (typeof prop !== "string") return undefined;
          const attrName = "data-" + camelToDash(prop);
          return el.hasAttribute(attrName) ? el.getAttribute(attrName) : undefined;
        },
        set: function (target, prop, value) {
          if (typeof prop !== "string") return true;
          const attrName = "data-" + camelToDash(prop);
          el.setAttribute(attrName, String(value));
          return true;
        },
        has: function (target, prop) {
          if (typeof prop !== "string") return false;
          const attrName = "data-" + camelToDash(prop);
          return el.hasAttribute(attrName);
        },
        deleteProperty: function (target, prop) {
          if (typeof prop !== "string") return true;
          const attrName = "data-" + camelToDash(prop);
          el.removeAttribute(attrName);
          return true;
        },
        ownKeys: function () {
          const keys = [];
          const names = el.getAttributeNames ? el.getAttributeNames() : [];
          for (let i = 0; i < names.length; i++) {
            if (names[i].startsWith("data-")) {
              keys.push(dashToCamel(names[i].slice(5)));
            }
          }
          return keys;
        },
        getOwnPropertyDescriptor: function (target, prop) {
          if (typeof prop === "string") {
            const attrName = "data-" + camelToDash(prop);
            if (el.hasAttribute(attrName)) {
              return {
                value: el.getAttribute(attrName),
                writable: true,
                enumerable: true,
                configurable: true,
              };
            }
          }
          return undefined;
        },
      });
    },
    configurable: true,
  });

  function collectParsedText(node) {
    if (node.nodeType === TEXT_NODE) return node.data || "";
    if (node.nodeType === ELEMENT_NODE || node.nodeType === DOCUMENT_FRAGMENT_NODE || node.nodeType === DOCUMENT_NODE) {
      let res = "";
      const kids = node._children || [];
      for (let i = 0; i < kids.length; i++) {
        res += collectParsedText(kids[i]);
      }
      return res;
    }
    return "";
  }

  Object.defineProperty(ParsedElement.prototype, "textContent", {
    get: function () {
      return collectParsedText(this);
    },
    set: function (v) {
      this._children = [];
      const val = String(v);
      if (val.length > 0) {
        const t = this.ownerDocument ? this.ownerDocument.createTextNode(val) : new ParsedText(val, null);
        t.parentNode = this;
        this._children.push(t);
      }
    },
    configurable: true,
  });

  Object.defineProperty(ParsedElement.prototype, "childNodes", {
    get: function () {
      return new NodeList(this._children);
    },
    configurable: true,
  });

  Object.defineProperty(ParsedElement.prototype, "children", {
    get: function () {
      return this._children.filter(function (n) { return n.nodeType === ELEMENT_NODE; });
    },
    configurable: true,
  });

  Object.defineProperty(ParsedElement.prototype, "firstChild", {
    get: function () {
      return this._children.length > 0 ? this._children[0] : null;
    },
    configurable: true,
  });

  Object.defineProperty(ParsedElement.prototype, "lastChild", {
    get: function () {
      return this._children.length > 0 ? this._children[this._children.length - 1] : null;
    },
    configurable: true,
  });

  Object.defineProperty(ParsedElement.prototype, "previousSibling", {
    get: function () {
      if (!this.parentNode || !this.parentNode._children) return null;
      const idx = this.parentNode._children.indexOf(this);
      return idx > 0 ? this.parentNode._children[idx - 1] : null;
    },
    configurable: true,
  });

  Object.defineProperty(ParsedElement.prototype, "nextSibling", {
    get: function () {
      if (!this.parentNode || !this.parentNode._children) return null;
      const idx = this.parentNode._children.indexOf(this);
      return (idx >= 0 && idx < this.parentNode._children.length - 1) ? this.parentNode._children[idx + 1] : null;
    },
    configurable: true,
  });

  Object.defineProperty(ParsedElement.prototype, "parentElement", {
    get: function () {
      return (this.parentNode && this.parentNode.nodeType === ELEMENT_NODE) ? this.parentNode : null;
    },
    configurable: true,
  });

  ParsedElement.prototype.appendChild = function (child) {
    if (!child || typeof child !== "object") {
      throw new TypeError("Failed to execute 'appendChild' on 'Node': parameter 1 is not of type 'Node'.");
    }
    if (child.nodeType === DOCUMENT_FRAGMENT_NODE) {
      const kids = Array.from(child.childNodes || child._children || []);
      for (let i = 0; i < kids.length; i++) {
        this.appendChild(kids[i]);
      }
      if (child._children) child._children.length = 0;
      return child;
    }
    if (child.parentNode && typeof child.parentNode.removeChild === "function") {
      try { child.parentNode.removeChild(child); } catch (e) {}
    }
    child.parentNode = this;
    child.ownerDocument = this.ownerDocument || this;
    this._children.push(child);
    return child;
  };

  ParsedElement.prototype.removeChild = function (child) {
    if (!child || child.parentNode !== this || this._children.indexOf(child) === -1) {
      if (typeof DOMException !== "undefined") {
        throw new DOMException("NotFoundError: Node was not found", "NotFoundError");
      }
      const err = new Error("NotFoundError: Node was not found");
      err.name = "NotFoundError";
      throw err;
    }
    const idx = this._children.indexOf(child);
    if (idx >= 0) this._children.splice(idx, 1);
    child.parentNode = null;
    return child;
  };

  ParsedElement.prototype.insertBefore = function (newChild, refChild) {
    if (!newChild || typeof newChild !== "object") {
      throw new TypeError("Failed to execute 'insertBefore' on 'Node': parameter 1 is not of type 'Node'.");
    }
    if (refChild == null) {
      return this.appendChild(newChild);
    }
    const idx = this._children.indexOf(refChild);
    if (idx === -1) {
      if (typeof DOMException !== "undefined") {
        throw new DOMException("NotFoundError: Node was not found", "NotFoundError");
      }
      const err = new Error("NotFoundError: Node was not found");
      err.name = "NotFoundError";
      throw err;
    }
    if (newChild.nodeType === DOCUMENT_FRAGMENT_NODE) {
      const kids = Array.from(newChild.childNodes || newChild._children || []);
      for (let i = 0; i < kids.length; i++) {
        this.insertBefore(kids[i], refChild);
      }
      if (newChild._children) newChild._children.length = 0;
      return newChild;
    }
    if (newChild.parentNode && typeof newChild.parentNode.removeChild === "function") {
      try { newChild.parentNode.removeChild(newChild); } catch (e) {}
    }
    newChild.parentNode = this;
    newChild.ownerDocument = this.ownerDocument || this;
    const curIdx = this._children.indexOf(refChild);
    this._children.splice(curIdx >= 0 ? curIdx : this._children.length, 0, newChild);
    return newChild;
  };

  ParsedElement.prototype.querySelector = function (sel) {
    if (arguments.length === 0) throw new TypeError("querySelector: 1 argument required");
    function walk(node) {
      const kids = node._children || [];
      for (let i = 0; i < kids.length; i++) {
        const k = kids[i];
        if (k.nodeType === ELEMENT_NODE) {
          if (matchesSelector(k, sel)) return k;
          const found = walk(k);
          if (found) return found;
        }
      }
      return null;
    }
    return walk(this);
  };

  ParsedElement.prototype.querySelectorAll = function (sel) {
    if (arguments.length === 0) throw new TypeError("querySelectorAll: 1 argument required");
    const results = [];
    function walk(node) {
      const kids = node._children || [];
      for (let i = 0; i < kids.length; i++) {
        const k = kids[i];
        if (k.nodeType === ELEMENT_NODE) {
          if (matchesSelector(k, sel)) results.push(k);
          walk(k);
        }
      }
    }
    walk(this);
    return new NodeList(results);
  };

  ParsedElement.prototype.getElementById = function (id) {
    return this.querySelector("#" + id);
  };

  ParsedElement.prototype.getElementsByTagName = function (tag) {
    return this.querySelectorAll(tag);
  };

  ParsedElement.prototype.getElementsByClassName = function (cls) {
    return this.querySelectorAll("." + cls);
  };

  ParsedElement.prototype.lookupNamespaceURI = function (prefix) {
    const p = prefix === "" || prefix === undefined ? null : String(prefix);
    let n = this;
    while (n && n.nodeType === ELEMENT_NODE) {
      if (p === null) {
        if (n.hasAttribute("xmlns")) return n.getAttribute("xmlns") || null;
      } else {
        if (n.hasAttribute("xmlns:" + p)) return n.getAttribute("xmlns:" + p) || null;
        const attrs = n.attributes;
        if (attrs) {
          for (let i = 0; i < attrs.length; i++) {
            const a = attrs[i];
            if (a && a.prefix === "xmlns" && a.localName === p) {
              return a.value || null;
            }
          }
        }
      }
      n = n.parentElement || n.parentNode;
    }
    return null;
  };

  function ParsedDocument(contentType, compatMode) {
    this.nodeType = DOCUMENT_NODE;
    this.nodeName = "#document";
    this.contentType = contentType || "text/html";
    this.compatMode = compatMode || "CSS1Compat";
    this.characterSet = "UTF-8";
    this.charset = "UTF-8";
    this.referrer = "";
    this.URL = "";
    this.documentURI = "";
    this.baseURI = "";
    this.defaultView = null;
    this.ownerDocument = null;
    this.parentNode = null;
    this._children = [];
    this.documentElement = null;
    this.head = null;
    this.body = null;
    this._title = "";
    this._doctypeInfo = null;
    this._doctypeNode = null;
    registerParsedNode(this);
  }

  Object.defineProperty(ParsedDocument.prototype, "title", {
    get: function () {
      const titleEl = this.querySelector ? this.querySelector("title") : null;
      return titleEl ? (titleEl.textContent || "") : this._title;
    },
    set: function (v) {
      this._title = String(v);
      const titleEl = this.querySelector ? this.querySelector("title") : null;
      if (titleEl) {
        titleEl.textContent = this._title;
      }
    },
    configurable: true,
  });

  Object.defineProperty(ParsedDocument.prototype, "doctype", {
    get: function () {
      if (!this._doctypeInfo) return null;
      if (!this._doctypeNode) {
        this._doctypeNode = {
          nodeType: DOCUMENT_TYPE_NODE,
          nodeName: this._doctypeInfo.name,
          name: this._doctypeInfo.name,
          publicId: this._doctypeInfo.publicId,
          systemId: this._doctypeInfo.systemId,
          ownerDocument: this,
          parentNode: this,
        };
      }
      return this._doctypeNode;
    },
    configurable: true,
  });

  Object.defineProperty(ParsedDocument.prototype, "childNodes", {
    get: function () {
      return new NodeList(this._children);
    },
    configurable: true,
  });

  Object.defineProperty(ParsedDocument.prototype, "children", {
    get: function () {
      return this._children.filter(function (n) { return n.nodeType === ELEMENT_NODE; });
    },
    configurable: true,
  });

  Object.defineProperty(ParsedDocument.prototype, "firstChild", {
    get: function () {
      return this._children.length > 0 ? this._children[0] : null;
    },
    configurable: true,
  });

  Object.defineProperty(ParsedDocument.prototype, "lastChild", {
    get: function () {
      return this._children.length > 0 ? this._children[this._children.length - 1] : null;
    },
    configurable: true,
  });

  ParsedDocument.prototype.createElement = function (tagName) {
    return new ParsedElement(tagName, this);
  };

  ParsedDocument.prototype.createTextNode = function (text) {
    return new ParsedText(text, this);
  };

  ParsedDocument.prototype.createComment = function (data) {
    return new ParsedComment(data, this);
  };

  ParsedDocument.prototype.createDocumentFragment = function () {
    return new ParsedFragment(this);
  };

  ParsedDocument.prototype.querySelector = function (sel) {
    if (arguments.length === 0) throw new TypeError("querySelector: 1 argument required");
    for (let i = 0; i < this._children.length; i++) {
      const root = this._children[i];
      if (root.nodeType === ELEMENT_NODE) {
        if (matchesSelector(root, sel)) return root;
        const found = root.querySelector(sel);
        if (found) return found;
      }
    }
    return null;
  };

  ParsedDocument.prototype.querySelectorAll = function (sel) {
    if (arguments.length === 0) throw new TypeError("querySelectorAll: 1 argument required");
    const list = [];
    for (let i = 0; i < this._children.length; i++) {
      const root = this._children[i];
      if (root.nodeType === ELEMENT_NODE) {
        if (matchesSelector(root, sel)) list.push(root);
        const sub = root.querySelectorAll(sel);
        for (let j = 0; j < sub.length; j++) list.push(sub[j]);
      }
    }
    return new NodeList(list);
  };

  ParsedDocument.prototype.getElementById = function (id) {
    return this.querySelector("#" + id);
  };

  ParsedDocument.prototype.getElementsByTagName = function (tag) {
    return this.querySelectorAll(tag);
  };

  ParsedDocument.prototype.getElementsByClassName = function (cls) {
    return this.querySelectorAll("." + cls);
  };

  ParsedDocument.prototype.createNSResolver = function (nodeResolver) {
    return nodeResolver;
  };

  ParsedDocument.prototype.evaluate = function (expression, contextNode, resolver, type, result) {
    if (typeof evaluateXPath !== "undefined") {
      return evaluateXPath(expression, contextNode || this, resolver, type, result);
    }
    throw new TypeError("evaluateXPath is not available");
  };

  ParsedDocument.prototype.createExpression = function (expression, resolver) {
    if (typeof createExpression !== "undefined") {
      return createExpression(expression, resolver);
    }
    throw new TypeError("createExpression is not available");
  };

  ParsedDocument.prototype.lookupNamespaceURI = function (prefix) {
    return this.documentElement ? this.documentElement.lookupNamespaceURI(prefix) : null;
  };

  function parseDocument(str, mime) {
    const type = String(mime || "").toLowerCase();
    if (!SUPPORTED_MIME_TYPES.has(type)) {
      throw new TypeError(
        "Failed to execute 'parseFromString' on 'DOMParser': The provided value '" +
          mime +
          "' is not a valid enum value of type SupportedType."
      );
    }

    let input = typeof str === "string" ? str : String(str ?? "");
    const trimmed = input.replace(/^\uFEFF/, "").trim();
    const isHtml = type === "text/html";
    const compatMode = isHtml ? (/^<!doctype\s+html\s*>/i.test(trimmed) ? "CSS1Compat" : "BackCompat") : "CSS1Compat";
    const doc = new ParsedDocument(type, compatMode);
    doc._doctypeInfo = leadingDoctype(trimmed, isHtml);

    if (isHtml) {
      if (input.startsWith("<?")) {
        const end = input.indexOf(">");
        const rawPI = input.slice(1, end < 0 ? undefined : end);
        const cNode = doc.createComment(rawPI);
        cNode.parentNode = doc;
        doc._children.push(cNode);
        input = end < 0 ? "" : input.slice(end + 1);
      }

      const root = doc.createElement("html");
      const head = doc.createElement("head");
      const body = doc.createElement("body");
      root.appendChild(head);
      root.appendChild(body);
      root.parentNode = doc;
      doc._children.push(root);
      doc.documentElement = root;
      doc.head = head;
      doc.body = body;

      const tokenRegex = /<!--([\s\S]*?)-->|<!\[CDATA\[([\s\S]*?)\]\]>|<!DOCTYPE[^>]*>|<\?([\s\S]*?)\?>|<\/([a-zA-Z0-9_:-]+)\s*>|<([a-zA-Z0-9_:-]+)((?:\s+[^=>/\s]+(?:\s*=\s*(?:"[^"]*"|'[^']*'|[^>\s]+))?)*)\s*(\/?)>/g;
      let stack = [body];
      let lastIndex = 0;
      let match;

      while ((match = tokenRegex.exec(input)) !== null) {
        if (match.index > lastIndex) {
          const text = unescapeEntities(input.slice(lastIndex, match.index));
          if (text.length > 0) {
            const tNode = doc.createTextNode(text);
            stack[stack.length - 1].appendChild(tNode);
          }
        }
        lastIndex = tokenRegex.lastIndex;

        if (match[1] !== undefined) {
          // Comment
          const cNode = doc.createComment(match[1]);
          stack[stack.length - 1].appendChild(cNode);
        } else if (match[2] !== undefined) {
          // CDATA
          const tNode = doc.createTextNode(match[2]);
          stack[stack.length - 1].appendChild(tNode);
        } else if (match[3] !== undefined) {
          // PI
          const cNode = doc.createComment("?" + match[3]);
          stack[stack.length - 1].appendChild(cNode);
        } else if (match[4] !== undefined) {
          // End tag
          const endTag = match[4].toLowerCase();
          for (let i = stack.length - 1; i > 0; i--) {
            if (stack[i].localName === endTag) {
              stack.length = i;
              break;
            }
          }
        } else if (match[5] !== undefined) {
          // Start tag
          const tagName = match[5];
          const lowerTag = tagName.toLowerCase();
          const attrStr = match[6] || "";
          const isSelfClosing = Boolean(match[7]);

          if (lowerTag === "html") {
            applyAttributes(root, attrStr);
          } else if (lowerTag === "head") {
            applyAttributes(head, attrStr);
            stack = [head];
          } else if (lowerTag === "body") {
            applyAttributes(body, attrStr);
            stack = [body];
          } else {
            const el = doc.createElement(tagName);
            applyAttributes(el, attrStr);

            if (lowerTag === "title" || lowerTag === "meta" || lowerTag === "link" || lowerTag === "style") {
              head.appendChild(el);
              if (lowerTag === "title" && !isSelfClosing) {
                stack.push(el);
              }
            } else {
              stack[stack.length - 1].appendChild(el);
              if (!isSelfClosing && !HTML_VOID_TAGS.has(lowerTag)) {
                stack.push(el);
              }
            }
          }
        }
      }

      if (lastIndex < input.length) {
        const text = unescapeEntities(input.slice(lastIndex));
        if (text.length > 0) {
          const tNode = doc.createTextNode(text);
          stack[stack.length - 1].appendChild(tNode);
        }
      }

      const titleEl = head.querySelector("title");
      if (titleEl) {
        doc._title = titleEl.textContent;
      }
    } else {
      // XML / SVG mode
      const tokenRegex = /<!--([\s\S]*?)-->|<!\[CDATA\[([\s\S]*?)\]\]>|<!DOCTYPE[^>]*>|<\?([\s\S]*?)\?>|<\/([a-zA-Z0-9_:-]+)\s*>|<([a-zA-Z0-9_:-]+)((?:\s+[^=>/\s]+(?:\s*=\s*(?:"[^"]*"|'[^']*'|[^>\s]+))?)*)\s*(\/?)>/g;
      let stack = [doc];
      let lastIndex = 0;
      let match;

      while ((match = tokenRegex.exec(input)) !== null) {
        if (match.index > lastIndex) {
          const text = unescapeEntities(input.slice(lastIndex, match.index));
          if (text.length > 0 && stack.length > 1) {
            const tNode = doc.createTextNode(text);
            stack[stack.length - 1].appendChild(tNode);
          }
        }
        lastIndex = tokenRegex.lastIndex;

        if (match[1] !== undefined) {
          const cNode = doc.createComment(match[1]);
          if (stack.length === 1) {
            cNode.parentNode = doc;
            doc._children.push(cNode);
          } else {
            stack[stack.length - 1].appendChild(cNode);
          }
        } else if (match[2] !== undefined) {
          if (stack.length > 1) {
            const tNode = doc.createTextNode(match[2]);
            stack[stack.length - 1].appendChild(tNode);
          }
        } else if (match[3] !== undefined) {
          const cNode = doc.createComment("?" + match[3]);
          if (stack.length === 1) {
            cNode.parentNode = doc;
            doc._children.push(cNode);
          } else {
            stack[stack.length - 1].appendChild(cNode);
          }
        } else if (match[4] !== undefined) {
          const endTag = match[4].toLowerCase();
          for (let i = stack.length - 1; i > 0; i--) {
            if (stack[i].localName === endTag) {
              stack.length = i;
              break;
            }
          }
        } else if (match[5] !== undefined) {
          const tagName = match[5];
          const attrStr = match[6] || "";
          const isSelfClosing = Boolean(match[7]);

          const el = doc.createElement(tagName);
          applyAttributes(el, attrStr);

          if (stack.length === 1) {
            el.parentNode = doc;
            doc._children.push(el);
            doc.documentElement = el;
            if (!isSelfClosing) {
              stack.push(el);
            }
          } else {
            stack[stack.length - 1].appendChild(el);
            if (!isSelfClosing) {
              stack.push(el);
            }
          }
        }
      }

      if (lastIndex < input.length) {
        const text = unescapeEntities(input.slice(lastIndex));
        if (text.length > 0 && stack.length > 1) {
          const tNode = doc.createTextNode(text);
          stack[stack.length - 1].appendChild(tNode);
        }
      }
    }

    return doc;
  }

  function applyAttributes(el, attrStr) {
    if (!attrStr) return;
    const attrRegex = /([a-zA-Z0-9_:-]+)(?:\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s>]+)))?/g;
    let m;
    while ((m = attrRegex.exec(attrStr)) !== null) {
      const name = m[1];
      const val = m[2] !== undefined ? m[2] : (m[3] !== undefined ? m[3] : (m[4] !== undefined ? m[4] : ""));
      el.setAttribute(name, unescapeEntities(val));
    }
  }

  function DOMParser() {}

  DOMParser.prototype.parseFromString = function (str, mimeType) {
    return parseDocument(str, mimeType);
  };

  markNative(DOMParser, "DOMParser");
  markNative(DOMParser.prototype.parseFromString, "parseFromString");

  if (typeof globalThis !== "undefined") {
    globalThis.DOMParser = DOMParser;
  }
  if (typeof window !== "undefined") {
    window.DOMParser = DOMParser;
  }

  // =============================================================
  // Module: xpath.js
  // =============================================================
  // src/js/shim/xpath.js
  // CapSurf XPath 1.0 Specification Engine
  // Aligned with vendor/kitesurf/packages/dom-shim/src/xpath.ts

  const XPATH_RESULT_TYPES = {
    ANY_TYPE: 0,
    NUMBER_TYPE: 1,
    STRING_TYPE: 2,
    BOOLEAN_TYPE: 3,
    UNORDERED_NODE_ITERATOR_TYPE: 4,
    ORDERED_NODE_ITERATOR_TYPE: 5,
    UNORDERED_NODE_SNAPSHOT_TYPE: 6,
    ORDERED_NODE_SNAPSHOT_TYPE: 7,
    ANY_UNORDERED_NODE_TYPE: 8,
    FIRST_ORDERED_NODE_TYPE: 9,
  };

  const AXES = new Set([
    "ancestor",
    "ancestor-or-self",
    "attribute",
    "child",
    "descendant",
    "descendant-or-self",
    "following",
    "following-sibling",
    "namespace",
    "parent",
    "preceding",
    "preceding-sibling",
    "self",
  ]);

  const REVERSE_AXES = new Set([
    "ancestor",
    "ancestor-or-self",
    "preceding",
    "preceding-sibling",
  ]);

  const NODE_TYPES = new Set(["comment", "text", "processing-instruction", "node"]);
  const OPERATOR_NAMES = new Set(["and", "or", "mod", "div"]);

  const EOF_TOKEN = { t: "eof", v: "" };

  function getDOMException(message, name) {
    if (typeof DOMException !== "undefined") {
      return new DOMException(message, name);
    }
    if (typeof globalThis !== "undefined" && globalThis.DOMException) {
      return new globalThis.DOMException(message, name);
    }
    const err = new Error(message);
    err.name = name;
    return err;
  }

  function syntaxError(src) {
    return getDOMException("The expression is not a legal expression.", "SyntaxError");
  }

  function namespaceError(prefix) {
    return getDOMException(
      "The expression contains an unresolvable namespace prefix.",
      "NamespaceError"
    );
  }

  function isNameStart(codePoint) {
    return (
      codePoint === 0x5f ||
      (codePoint >= 0x41 && codePoint <= 0x5a) ||
      (codePoint >= 0x61 && codePoint <= 0x7a) ||
      (codePoint >= 0xc0 && codePoint <= 0xd6) ||
      (codePoint >= 0xd8 && codePoint <= 0xf6) ||
      (codePoint >= 0xf8 && codePoint <= 0x2ff) ||
      (codePoint >= 0x370 && codePoint <= 0x37d) ||
      (codePoint >= 0x37f && codePoint <= 0x1fff) ||
      (codePoint >= 0x200c && codePoint <= 0x200d) ||
      (codePoint >= 0x2070 && codePoint <= 0x218f) ||
      (codePoint >= 0x2c00 && codePoint <= 0x2fef) ||
      (codePoint >= 0x3001 && codePoint <= 0xd7ff) ||
      (codePoint >= 0xf900 && codePoint <= 0xfdcf) ||
      (codePoint >= 0xfdf0 && codePoint <= 0xfffd) ||
      (codePoint >= 0x10000 && codePoint <= 0xeffff)
    );
  }

  function isNameChar(codePoint) {
    return (
      isNameStart(codePoint) ||
      (codePoint >= 0x30 && codePoint <= 0x39) ||
      codePoint === 0x2d ||
      codePoint === 0x2e ||
      codePoint === 0xb7 ||
      (codePoint >= 0x300 && codePoint <= 0x36f) ||
      (codePoint >= 0x203f && codePoint <= 0x2040)
    );
  }

  function lex(src) {
    const toks = [];
    const n = src.length;
    let i = 0;

    const operatorMayFollow = function () {
      const p = toks[toks.length - 1];
      if (!p) return false;
      if (p.t === "op") return false;
      return !(
        p.t === "punct" &&
        (p.v === "@" || p.v === "::" || p.v === "(" || p.v === "[" || p.v === ",")
      );
    };

    const readNcName = function () {
      const start = i;
      const first = src.codePointAt(i) ?? -1;
      if (!isNameStart(first)) throw syntaxError(src);
      i += first > 0xffff ? 2 : 1;
      while (i < n) {
        const next = src.codePointAt(i) ?? -1;
        if (!isNameChar(next)) break;
        i += next > 0xffff ? 2 : 1;
      }
      return src.slice(start, i);
    };

    const peekNonSpace = function (from) {
      let j = from;
      while (j < n && " \t\r\n".includes(src.charAt(j))) j++;
      return j;
    };

    while (i < n) {
      const c = src.charAt(i);
      if (" \t\r\n".includes(c)) {
        i++;
        continue;
      }
      if (c === '"' || c === "'") {
        const end = src.indexOf(c, i + 1);
        if (end < 0) throw syntaxError(src);
        toks.push({ t: "str", v: src.slice(i + 1, end) });
        i = end + 1;
        continue;
      }
      if (/[0-9]/.test(c) || (c === "." && /[0-9]/.test(src[i + 1] ?? ""))) {
        const m = /^(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)/.exec(src.slice(i));
        if (!m) throw syntaxError(src);
        toks.push({ t: "num", v: m[0] });
        i += m[0].length;
        continue;
      }
      const two = src.slice(i, i + 2);
      if (two === ".." || two === "::") {
        toks.push({ t: "punct", v: two });
        i += 2;
        continue;
      }
      if ("().[]@,".includes(c)) {
        toks.push({ t: "punct", v: c });
        i++;
        continue;
      }
      if (two === "//" || two === "!=" || two === "<=" || two === ">=") {
        toks.push({ t: "op", v: two });
        i += 2;
        continue;
      }
      if ("/|+-=<>".includes(c)) {
        toks.push({ t: "op", v: c });
        i++;
        continue;
      }
      if (c === "*") {
        toks.push(operatorMayFollow() ? { t: "op", v: "*" } : { t: "name", v: "*" });
        i++;
        continue;
      }
      if (c === "$") {
        i++;
        let name = readNcName();
        if (src[i] === ":" && src[i + 1] !== ":") {
          i++;
          name += ":" + readNcName();
        }
        toks.push({ t: "var", v: name });
        continue;
      }
      if (!isNameStart(src.codePointAt(i) ?? -1)) throw syntaxError(src);
      let name = readNcName();
      if (operatorMayFollow()) {
        if (!OPERATOR_NAMES.has(name)) throw syntaxError(src);
        toks.push({ t: "op", v: name });
        continue;
      }
      if (src[i] === ":" && src[i + 1] !== ":") {
        i++;
        if (src[i] === "*") {
          i++;
          toks.push({ t: "name", v: `${name}:*` });
          continue;
        }
        name += ":" + readNcName();
      }
      const next = peekNonSpace(i);
      if (src[next] === "(") {
        toks.push({ t: NODE_TYPES.has(name) ? "type" : "fn", v: name });
      } else if (src.slice(next, next + 2) === "::") {
        if (!AXES.has(name)) throw syntaxError(src);
        toks.push({ t: "axis", v: name });
      } else {
        toks.push({ t: "name", v: name });
      }
    }
    toks.push(EOF_TOKEN);
    return toks;
  }

  function resolvePrefix(resolver, prefix) {
    let ns = null;
    try {
      if (typeof resolver === "function") {
        ns = resolver(prefix);
      } else if (resolver !== null && typeof resolver === "object") {
        const lookupNamespaceURI = resolver.lookupNamespaceURI;
        if (typeof lookupNamespaceURI !== "function") {
          throw new TypeError("XPathNSResolver.lookupNamespaceURI is not callable");
        }
        ns = lookupNamespaceURI.call(resolver, prefix);
      }
    } catch (error) {
      throw namespaceError(prefix);
    }
    if (ns === null || ns === undefined) {
      throw namespaceError(prefix);
    }
    if (typeof ns === "symbol") {
      throw namespaceError(prefix);
    }
    try {
      return String(ns);
    } catch (error) {
      throw namespaceError(prefix);
    }
  }

  class Parser {
    constructor(src, toks, resolver) {
      this.src = src;
      this.toks = toks;
      this.resolver = resolver;
      this.i = 0;
    }

    parse() {
      const e = this.orExpr();
      if (this.peek().t !== "eof") throw syntaxError(this.src);
      return e;
    }

    peek(k = 0) {
      return this.toks[Math.min(this.i + k, this.toks.length - 1)] ?? EOF_TOKEN;
    }

    next() {
      return this.toks[this.i++] ?? EOF_TOKEN;
    }

    is(t, v) {
      const p = this.peek();
      return p.t === t && (v === undefined || p.v === v);
    }

    expect(t, v) {
      if (!this.is(t, v)) throw syntaxError(this.src);
      return this.next();
    }

    binary(sub, ops, make) {
      let l = sub.call(this);
      while (this.peek().t === "op" && ops.includes(this.peek().v)) {
        const op = this.next().v;
        l = make(op, l, sub.call(this));
      }
      return l;
    }

    orExpr() {
      return this.binary(
        this.andExpr,
        ["or"],
        (_op, l, r) => ({ t: "or", l, r })
      );
    }

    andExpr() {
      return this.binary(
        this.equalityExpr,
        ["and"],
        (_op, l, r) => ({ t: "and", l, r })
      );
    }

    equalityExpr() {
      return this.binary(
        this.relationalExpr,
        ["=", "!="],
        (op, l, r) => ({ t: "cmp", op, l, r })
      );
    }

    relationalExpr() {
      return this.binary(
        this.additiveExpr,
        ["<", ">", "<=", ">="],
        (op, l, r) => ({ t: "cmp", op, l, r })
      );
    }

    additiveExpr() {
      return this.binary(
        this.multiplicativeExpr,
        ["+", "-"],
        (op, l, r) => ({ t: "arith", op, l, r })
      );
    }

    multiplicativeExpr() {
      return this.binary(
        this.unaryExpr,
        ["*", "div", "mod"],
        (op, l, r) => ({ t: "arith", op, l, r })
      );
    }

    unaryExpr() {
      if (this.is("op", "-")) {
        this.next();
        return { t: "neg", e: this.unaryExpr() };
      }
      return this.unionExpr();
    }

    unionExpr() {
      return this.binary(
        this.pathExpr,
        ["|"],
        (_op, l, r) => ({ t: "union", l, r })
      );
    }

    pathExpr() {
      const p = this.peek();
      const startsFilter =
        p.t === "var" ||
        p.t === "str" ||
        p.t === "num" ||
        p.t === "fn" ||
        (p.t === "punct" && p.v === "(");
      if (!startsFilter) return this.locationPath();
      const primary = this.primaryExpr();
      const preds = this.predicates();
      let e = preds.length ? { t: "filter", e: primary, preds } : primary;
      if (this.is("op", "/") || this.is("op", "//")) {
        const steps = [];
        this.relativePath(steps, true);
        e = { t: "path", start: e, steps };
      }
      return e;
    }

    primaryExpr() {
      const p = this.next();
      if (p.t === "num") return { t: "num", v: Number(p.v) };
      if (p.t === "str") return { t: "str", v: p.v };
      if (p.t === "var") throw syntaxError(this.src);
      if (p.t === "punct" && p.v === "(") {
        const e = this.orExpr();
        this.expect("punct", ")");
        return e;
      }
      if (p.t === "fn") {
        const fn = FUNCTIONS.get(p.v);
        if (!fn) throw syntaxError(this.src);
        this.expect("punct", "(");
        const args = [];
        if (!this.is("punct", ")")) {
          args.push(this.orExpr());
          while (this.is("punct", ",")) {
            this.next();
            args.push(this.orExpr());
          }
        }
        this.expect("punct", ")");
        const [min, max] = fn.arity;
        if (args.length < min || args.length > max) throw syntaxError(this.src);
        return { t: "fn", name: p.v, args };
      }
      throw syntaxError(this.src);
    }

    predicates() {
      const preds = [];
      while (this.is("punct", "[")) {
        this.next();
        preds.push(this.orExpr());
        this.expect("punct", "]");
      }
      return preds;
    }

    locationPath() {
      const steps = [];
      if (this.is("op", "/")) {
        this.next();
        if (this.startsStep()) this.relativePath(steps, false);
        return { t: "path", start: "root", steps };
      }
      if (this.is("op", "//")) {
        this.relativePath(steps, true);
        return { t: "path", start: "root", steps };
      }
      this.relativePath(steps, false);
      return { t: "path", start: "context", steps };
    }

    startsStep() {
      const p = this.peek();
      return (
        p.t === "name" ||
        p.t === "type" ||
        p.t === "axis" ||
        (p.t === "punct" && (p.v === "@" || p.v === "." || p.v === ".."))
      );
    }

    relativePath(steps, leadingSeparator) {
      if (leadingSeparator) this.separator(steps);
      steps.push(this.step());
      while (this.is("op", "/") || this.is("op", "//")) {
        this.separator(steps);
        steps.push(this.step());
      }
    }

    separator(steps) {
      const sep = this.next().v;
      if (sep === "//") steps.push({ axis: "descendant-or-self", test: { k: "node" }, preds: [] });
    }

    step() {
      if (this.is("punct", ".")) {
        this.next();
        return { axis: "self", test: { k: "node" }, preds: [] };
      }
      if (this.is("punct", "..")) {
        this.next();
        return { axis: "parent", test: { k: "node" }, preds: [] };
      }
      let axis = "child";
      if (this.is("axis")) {
        axis = this.next().v;
        this.expect("punct", "::");
      } else if (this.is("punct", "@")) {
        this.next();
        axis = "attribute";
      }
      const test = this.nodeTest();
      return { axis, test, preds: this.predicates() };
    }

    nodeTest() {
      const p = this.next();
      if (p.t === "type") {
        this.expect("punct", "(");
        let target = null;
        if (p.v === "processing-instruction" && this.is("str")) target = this.next().v;
        this.expect("punct", ")");
        if (p.v === "node") return { k: "node" };
        if (p.v === "text") return { k: "text" };
        if (p.v === "comment") return { k: "comment" };
        return { k: "pi", target };
      }
      if (p.t !== "name") throw syntaxError(this.src);
      if (p.v === "*") return { k: "any", ns: null };
      const colon = p.v.indexOf(":");
      if (colon < 0) return { k: "name", ns: null, local: p.v };
      const ns = resolvePrefix(this.resolver, p.v.slice(0, colon));
      const local = p.v.slice(colon + 1);
      return local === "*" ? { k: "any", ns } : { k: "name", ns, local };
    }
  }

  function compile(src, resolver) {
    return new Parser(src, lex(src), resolver).parse();
  }

  function isAttr(n) {
    return n?.nodeType === 2 && ("ownerElement" in n || "specified" in n);
  }

  function parentOf(n) {
    if (isAttr(n)) return n.ownerElement || null;
    const p = n.parentNode || n.parentElement;
    if (!p && n.ownerDocument && n !== n.ownerDocument) {
      if (n === n.ownerDocument.documentElement) return n.ownerDocument;
    }
    return p || null;
  }

  function inModel(n) {
    const t = n.nodeType;
    return t !== 10 && t !== 12 && t !== 5 && t !== 6;
  }

  function firstChild(n) {
    let c;
    if (n.nodeType === 9) { // DOCUMENT_NODE
      c = n.firstChild || n.documentElement;
    } else {
      c = isAttr(n) ? null : n.firstChild;
    }
    while (c && !inModel(c)) c = c.nextSibling;
    return c || null;
  }

  function lastChild(n) {
    let c;
    if (n.nodeType === 9) {
      c = n.lastChild || n.documentElement;
    } else {
      c = isAttr(n) ? null : n.lastChild;
    }
    while (c && !inModel(c)) c = c.previousSibling;
    return c || null;
  }

  function nextSibling(n) {
    let c = isAttr(n) ? null : n.nextSibling;
    while (c && !inModel(c)) c = c.nextSibling;
    return c || null;
  }

  function previousSibling(n) {
    let c = isAttr(n) ? null : n.previousSibling;
    while (c && !inModel(c)) c = c.previousSibling;
    return c || null;
  }

  function descendants(n, out) {
    for (let c = firstChild(n); c; c = nextSibling(c)) {
      out.push(c);
      descendants(c, out);
    }
  }

  function descendantsReversed(n, out) {
    for (let c = lastChild(n); c; c = previousSibling(c)) {
      descendantsReversed(c, out);
      out.push(c);
    }
  }

  function attributesOf(n) {
    if (!n || n.nodeType !== 1) return []; // ELEMENT_NODE
    const list = n.attributes;
    const out = [];
    if (!list) return out;
    const len = list.length || 0;
    for (let i = 0; i < len; i++) {
      const a = list[i];
      if (
        !a ||
        a.namespaceURI === "http://www.w3.org/2000/xmlns/" ||
        a.name === "xmlns" ||
        String(a.name).startsWith("xmlns:")
      ) {
        continue;
      }
      out.push(a);
    }
    return out;
  }

  function axisNodes(axis, n) {
    const out = [];
    switch (axis) {
      case "self":
        out.push(n);
        break;
      case "child":
        for (let c = firstChild(n); c; c = nextSibling(c)) out.push(c);
        break;
      case "descendant":
        descendants(n, out);
        break;
      case "descendant-or-self":
        out.push(n);
        descendants(n, out);
        break;
      case "parent": {
        const p = parentOf(n);
        if (p) out.push(p);
        break;
      }
      case "ancestor":
      case "ancestor-or-self":
        if (axis === "ancestor-or-self") out.push(n);
        for (let p = parentOf(n); p; p = parentOf(p)) out.push(p);
        break;
      case "following-sibling":
        for (let s = nextSibling(n); s; s = nextSibling(s)) out.push(s);
        break;
      case "preceding-sibling":
        for (let s = previousSibling(n); s; s = previousSibling(s)) out.push(s);
        break;
      case "following": {
        let a = n;
        if (isAttr(n)) {
          a = n.ownerElement;
          if (!a) break;
          descendants(a, out);
        }
        for (; a; a = parentOf(a)) {
          for (let s = nextSibling(a); s; s = nextSibling(s)) {
            out.push(s);
            descendants(s, out);
          }
        }
        break;
      }
      case "preceding": {
        for (let a = isAttr(n) ? n.ownerElement : n; a; a = parentOf(a)) {
          for (let s = previousSibling(a); s; s = previousSibling(s)) {
            descendantsReversed(s, out);
            out.push(s);
          }
        }
        break;
      }
      case "attribute":
        out.push(...attributesOf(n));
        break;
      case "namespace":
        break;
    }
    return out;
  }

  function isHtmlDocumentNode(el) {
    if (!el) return false;
    const doc = el.ownerDocument || (el.nodeType === 9 ? el : (typeof document !== "undefined" ? document : null));
    if (!doc) return false;
    if (doc.contentType) return doc.contentType === "text/html";
    return true;
  }

  function asciiLowercase(s) {
    return String(s || "").toLowerCase();
  }

  function matches(test, axis, n) {
    const t = n.nodeType;
    switch (test.k) {
      case "node":
        return true;
      case "text":
        return t === 3 || t === 4; // TEXT_NODE || CDATA_SECTION_NODE
      case "comment":
        return t === 8; // COMMENT_NODE
      case "pi":
        return t === 7 && (test.target === null || n.target === test.target); // PROCESSING_INSTRUCTION_NODE
    }
    if (axis === "attribute") {
      if (t !== 2) return false; // ATTRIBUTE_NODE
      const ns = n.namespaceURI || null;
      if (test.k === "any") return test.ns === null || ns === test.ns;
      const owner = n.ownerElement;
      if (
        test.ns === null &&
        ns === null &&
        owner &&
        (owner.namespaceURI === "http://www.w3.org/1999/xhtml" || !owner.namespaceURI) &&
        isHtmlDocumentNode(owner)
      ) {
        return asciiLowercase(n.localName || n.name) === asciiLowercase(test.local);
      }
      return (n.localName || n.name) === test.local && ns === test.ns;
    }
    if (t !== 1) return false; // ELEMENT_NODE
    const ns = n.namespaceURI || null;
    if (test.k === "any") return test.ns === null || ns === test.ns;
    if (isHtmlDocumentNode(n)) {
      if (ns === "http://www.w3.org/1999/xhtml" || ns === null) {
        return (
          asciiLowercase(n.localName || n.tagName) === asciiLowercase(test.local) &&
          (test.ns === null || test.ns === "http://www.w3.org/1999/xhtml")
        );
      }
      return (n.localName || n.tagName) === test.local && test.ns !== null && ns === test.ns;
    }
    return (n.localName || n.tagName) === test.local && ns === test.ns;
  }

  class Evaluation {
    constructor(root) {
      this.root = root;
      this.order = null;
    }

    index() {
      if (this.order) return this.order;
      const order = new Map();
      let k = 0;
      const visit = (n) => {
        order.set(n, k++);
        if (n.nodeType === 1) { // ELEMENT_NODE
          const attrs = attributesOf(n);
          for (let i = 0; i < attrs.length; i++) order.set(attrs[i], k++);
        }
        for (let c = firstChild(n); c; c = nextSibling(c)) visit(c);
      };
      visit(this.root);
      this.order = order;
      return order;
    }

    sort(nodes) {
      const unique = [...new Set(nodes)];
      if (unique.length < 2) return unique;
      const order = this.index();
      const key = (n) => {
        const parent = parentOf(n);
        return order.get(n) ?? (parent ? order.get(parent) : undefined) ?? Number.MAX_SAFE_INTEGER;
      };
      return unique.sort((a, b) => key(a) - key(b));
    }
  }

  function rootOf(node) {
    let root = node;
    for (let parent = parentOf(root); parent; parent = parentOf(parent)) {
      root = parent;
    }
    return root;
  }

  function stringValue(n) {
    switch (n.nodeType) {
      case 1: // ELEMENT_NODE
      case 11: // DOCUMENT_FRAGMENT_NODE
        return String(n.textContent ?? "");
      case 9: { // DOCUMENT_NODE
        let s = "";
        for (let c = firstChild(n); c; c = nextSibling(c)) {
          if (c.nodeType === 1 || c.nodeType === 3 || c.nodeType === 4) {
            s += c.textContent ?? "";
          }
        }
        return s;
      }
      case 2: // ATTRIBUTE_NODE
        return String(n.value ?? "");
      default:
        return String(n.nodeValue ?? n.data ?? n.textContent ?? "");
    }
  }

  function isNodeSet(v) {
    return Array.isArray(v);
  }

  function toBoolean(v) {
    if (isNodeSet(v)) return v.length > 0;
    if (typeof v === "number") return v !== 0 && !Number.isNaN(v);
    if (typeof v === "string") return v.length > 0;
    return Boolean(v);
  }

  function stringToNumber(s) {
    return /^[\t\n\r ]*-?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)[\t\n\r ]*$/.test(s)
      ? Number(s.trim())
      : NaN;
  }

  function toNumber(v) {
    if (isNodeSet(v)) return stringToNumber(toStringValue(v));
    if (typeof v === "string") return stringToNumber(v);
    if (typeof v === "boolean") return v ? 1 : 0;
    return Number(v);
  }

  function numberToString(x) {
    if (Number.isNaN(x)) return "NaN";
    if (x === Infinity) return "Infinity";
    if (x === -Infinity) return "-Infinity";
    if (x === 0) return "0";
    const s = String(x);
    if (!/e/i.test(s)) return s;
    const [mant, expStr] = s.toLowerCase().split("e");
    const neg = mant.startsWith("-");
    const digits = mant.replace(/^-/, "").replace(".", "");
    const intLen = (mant.replace(/^-/, "").split(".")[0] || "").length;
    const exp = Number(expStr) + intLen;
    let out;
    if (exp <= 0) out = "0." + "0".repeat(-exp) + digits;
    else if (exp >= digits.length) out = digits + "0".repeat(exp - digits.length);
    else out = digits.slice(0, exp) + "." + digits.slice(exp);
    return (neg ? "-" : "") + out;
  }

  function toStringValue(v) {
    if (isNodeSet(v)) return v.length ? stringValue(v[0]) : "";
    if (typeof v === "number") return numberToString(v);
    if (typeof v === "boolean") return v ? "true" : "false";
    return String(v);
  }

  function compareAtoms(op, a, b) {
    if (op === "=" || op === "!=") {
      let eq;
      if (typeof a === "boolean" || typeof b === "boolean") eq = toBoolean(a) === toBoolean(b);
      else if (typeof a === "number" || typeof b === "number") eq = toNumber(a) === toNumber(b);
      else eq = toStringValue(a) === toStringValue(b);
      return op === "=" ? eq : !eq;
    }
    const x = toNumber(a);
    const y = toNumber(b);
    if (op === "<") return x < y;
    if (op === "<=") return x <= y;
    if (op === ">") return x > y;
    return x >= y;
  }

  function compare(op, a, b) {
    if (isNodeSet(a) && isNodeSet(b)) {
      const bs = b.map(stringValue);
      return a.some((n) => {
        const s = stringValue(n);
        return bs.some((t) => compareAtoms(op, s, t));
      });
    }
    if (isNodeSet(a) || isNodeSet(b)) {
      const set = isNodeSet(a) ? a : b;
      const other = isNodeSet(a) ? b : a;
      if (typeof other === "boolean") {
        return isNodeSet(a)
          ? compareAtoms(op, toBoolean(set), other)
          : compareAtoms(op, other, toBoolean(set));
      }
      return set.some((n) => {
        const s = typeof other === "number" ? stringToNumber(stringValue(n)) : stringValue(n);
        return isNodeSet(a) ? compareAtoms(op, s, other) : compareAtoms(op, other, s);
      });
    }
    return compareAtoms(op, a, b);
  }

  function nodeSetOf(v) {
    if (!isNodeSet(v)) throw new TypeError("The expression does not evaluate to a node-set.");
    return v;
  }

  function xpathRound(x) {
    if (!Number.isFinite(x)) return x;
    if (x < 0 && x >= -0.5) return -0;
    return Math.floor(x + 0.5);
  }

  function localNameOf(n) {
    const t = n.nodeType;
    if (t === 1 || t === 2) return String(n.localName ?? "");
    if (t === 7) return String(n.target ?? "");
    return "";
  }

  function qualifiedNameOf(n) {
    const t = n.nodeType;
    if (t === 1 || t === 2) {
      return n.prefix ? `${n.prefix}:${n.localName}` : String(n.localName ?? "");
    }
    if (t === 7) return String(n.target ?? "");
    return "";
  }

  function firstOf(c, args) {
    if (!args.length) return c.node;
    const set = c.ev.sort(nodeSetOf(evaluate(arg(args, 0), c)));
    return set[0] ?? null;
  }

  function idLookup(c, ids) {
    const root = c.ev.root;
    const found = [];
    const wanted = new Set(ids.filter(Boolean));
    if (!wanted.size) return found;
    const walk = (n) => {
      if (n.nodeType === 1) {
        const id = typeof n.getAttribute === "function" ? n.getAttribute("id") : n.id;
        if (id !== null && id !== undefined && wanted.has(id)) {
          found.push(n);
          wanted.delete(id);
        }
      }
      for (let k = firstChild(n); k && wanted.size; k = nextSibling(k)) walk(k);
    };
    walk(root);
    return c.ev.sort(found);
  }

  function arg(args, i) {
    const e = args[i];
    if (!e) throw new Error(`xpath: missing argument ${i}`);
    return e;
  }

  const str = (c, args, i) => toStringValue(evaluate(arg(args, i), c));
  const num = (c, args, i) => toNumber(evaluate(arg(args, i), c));

  const FUNCTIONS = new Map(
    Object.entries({
      last: { arity: [0, 0], run: (c) => c.size },
      position: { arity: [0, 0], run: (c) => c.pos },
      count: { arity: [1, 1], run: (c, a) => nodeSetOf(evaluate(arg(a, 0), c)).length },
      id: {
        arity: [1, 1],
        run: (c, a) => {
          const v = evaluate(arg(a, 0), c);
          const tokens = isNodeSet(v)
            ? v.flatMap((n) => stringValue(n).split(/[\t\n\r ]+/))
            : toStringValue(v).split(/[\t\n\r ]+/);
          return idLookup(c, tokens);
        },
      },
      "local-name": {
        arity: [0, 1],
        run: (c, a) => {
          const n = firstOf(c, a);
          return n ? localNameOf(n) : "";
        },
      },
      "namespace-uri": {
        arity: [0, 1],
        run: (c, a) => {
          const n = firstOf(c, a);
          return n && (n.nodeType === 1 || n.nodeType === 2)
            ? String(n.namespaceURI ?? "")
            : "";
        },
      },
      name: {
        arity: [0, 1],
        run: (c, a) => {
          const n = firstOf(c, a);
          return n ? qualifiedNameOf(n) : "";
        },
      },
      string: { arity: [0, 1], run: (c, a) => (a.length ? str(c, a, 0) : stringValue(c.node)) },
      concat: { arity: [2, Infinity], run: (c, a) => a.map((_, i) => str(c, a, i)).join("") },
      "starts-with": { arity: [2, 2], run: (c, a) => str(c, a, 0).startsWith(str(c, a, 1)) },
      contains: { arity: [2, 2], run: (c, a) => str(c, a, 0).includes(str(c, a, 1)) },
      "substring-before": {
        arity: [2, 2],
        run: (c, a) => {
          const s = str(c, a, 0);
          const i = s.indexOf(str(c, a, 1));
          return i < 0 ? "" : s.slice(0, i);
        },
      },
      "substring-after": {
        arity: [2, 2],
        run: (c, a) => {
          const s = str(c, a, 0);
          const t = str(c, a, 1);
          const i = s.indexOf(t);
          return i < 0 ? "" : s.slice(i + t.length);
        },
      },
      substring: {
        arity: [2, 3],
        run: (c, a) => {
          const chars = Array.from(str(c, a, 0));
          const start = xpathRound(num(c, a, 1));
          const end = a.length > 2 ? start + xpathRound(num(c, a, 2)) : Infinity;
          return chars.filter((_, i) => i + 1 >= start && i + 1 < end).join("");
        },
      },
      "string-length": {
        arity: [0, 1],
        run: (c, a) => Array.from(a.length ? str(c, a, 0) : stringValue(c.node)).length,
      },
      "normalize-space": {
        arity: [0, 1],
        run: (c, a) =>
          (a.length ? str(c, a, 0) : stringValue(c.node))
            .replace(/[\t\n\r ]+/g, " ")
            .replace(/^ | $/g, ""),
      },
      translate: {
        arity: [3, 3],
        run: (c, a) => {
          const from = Array.from(str(c, a, 1));
          const to = Array.from(str(c, a, 2));
          return Array.from(str(c, a, 0))
            .map((ch) => {
              const i = from.indexOf(ch);
              return i < 0 ? ch : (to[i] ?? "");
            })
            .join("");
        },
      },
      boolean: { arity: [1, 1], run: (c, a) => toBoolean(evaluate(arg(a, 0), c)) },
      not: { arity: [1, 1], run: (c, a) => !toBoolean(evaluate(arg(a, 0), c)) },
      true: { arity: [0, 0], run: () => true },
      false: { arity: [0, 0], run: () => false },
      lang: {
        arity: [1, 1],
        run: (c, a) => {
          const want = str(c, a, 0).toLowerCase();
          for (let n = isAttr(c.node) ? c.node.ownerElement : c.node; n; n = parentOf(n)) {
            if (n.nodeType !== 1) continue;
            const lang = typeof n.getAttributeNS === "function"
              ? (n.getAttributeNS("http://www.w3.org/XML/1998/namespace", "lang") ?? n.getAttribute?.("xml:lang") ?? n.getAttribute?.("lang"))
              : (typeof n.getAttribute === "function" ? n.getAttribute("lang") : null);
            if (lang === null || lang === undefined) continue;
            const l = String(lang).toLowerCase();
            return l === want || l.startsWith(want + "-");
          }
          return false;
        },
      },
      number: {
        arity: [0, 1],
        run: (c, a) => (a.length ? num(c, a, 0) : stringToNumber(stringValue(c.node))),
      },
      sum: {
        arity: [1, 1],
        run: (c, a) =>
          nodeSetOf(evaluate(arg(a, 0), c)).reduce((s, n) => s + stringToNumber(stringValue(n)), 0),
      },
      floor: { arity: [1, 1], run: (c, a) => Math.floor(num(c, a, 0)) },
      ceiling: { arity: [1, 1], run: (c, a) => Math.ceil(num(c, a, 0)) },
      round: { arity: [1, 1], run: (c, a) => xpathRound(num(c, a, 0)) },
    })
  );

  function filterByPredicates(nodes, preds, ev) {
    let cur = nodes;
    for (const pred of preds) {
      const size = cur.length;
      cur = cur.filter((node, i) => {
        const v = evaluate(pred, { node, pos: i + 1, size, ev });
        return typeof v === "number" ? v === i + 1 : toBoolean(v);
      });
    }
    return cur;
  }

  function applyStep(step, contexts, ev) {
    const out = [];
    for (const n of contexts) {
      const picked = axisNodes(step.axis, n).filter((m) => matches(step.test, step.axis, m));
      out.push(...filterByPredicates(picked, step.preds, ev));
    }
    if (contexts.length === 1) return REVERSE_AXES.has(step.axis) ? out.reverse() : out;
    return ev.sort(out);
  }

  function evaluate(e, c) {
    switch (e.t) {
      case "num":
        return e.v;
      case "str":
        return e.v;
      case "fn": {
        const fn = FUNCTIONS.get(e.name);
        if (!fn) throw syntaxError(e.name);
        return fn.run(c, e.args);
      }
      case "or":
        return toBoolean(evaluate(e.l, c)) || toBoolean(evaluate(e.r, c));
      case "and":
        return toBoolean(evaluate(e.l, c)) && toBoolean(evaluate(e.r, c));
      case "cmp":
        return compare(e.op, evaluate(e.l, c), evaluate(e.r, c));
      case "arith": {
        const x = toNumber(evaluate(e.l, c));
        const y = toNumber(evaluate(e.r, c));
        if (e.op === "+") return x + y;
        if (e.op === "-") return x - y;
        if (e.op === "*") return x * y;
        if (e.op === "div") return x / y;
        return x % y;
      }
      case "neg":
        return -toNumber(evaluate(e.e, c));
      case "union":
        return c.ev.sort([...nodeSetOf(evaluate(e.l, c)), ...nodeSetOf(evaluate(e.r, c))]);
      case "filter":
        return filterByPredicates(c.ev.sort(nodeSetOf(evaluate(e.e, c))), e.preds, c.ev);
      case "path": {
        let nodes;
        if (e.start === "root") nodes = [c.ev.root];
        else if (e.start === "context") nodes = [c.node];
        else nodes = c.ev.sort(nodeSetOf(evaluate(e.start, c)));
        for (const step of e.steps) nodes = applyStep(step, nodes, c.ev);
        return nodes;
      }
    }
  }

  function isNode(v) {
    return !!v && typeof v === "object" && typeof v.nodeType === "number";
  }

  const INTERNAL_CONSTRUCT = Symbol("kitesurfInternalConstruct");
  const xpathResultInstances = new WeakSet();

  function need(ok, what) {
    if (!ok) throw new TypeError(`The result type is not ${what}.`);
  }

  function XPathResult(token, type = 0, value = false) {
    if (token !== INTERNAL_CONSTRUCT) throw new TypeError("Illegal constructor");
    this._type = type;
    this._value = value;
    this._seq = (typeof globalThis !== "undefined" && globalThis.__treeSeq) || 0;
    this._cursor = 0;
    xpathResultInstances.add(this);
  }

  XPathResult.prototype._refill = function (type, value) {
    this._type = type;
    this._value = value;
    this._cursor = 0;
    this._seq = (typeof globalThis !== "undefined" && globalThis.__treeSeq) || 0;
  };

  Object.defineProperty(XPathResult.prototype, "resultType", {
    get: function () {
      return this._type;
    },
    configurable: true,
  });

  Object.defineProperty(XPathResult.prototype, "numberValue", {
    get: function () {
      need(this._type === XPATH_RESULT_TYPES.NUMBER_TYPE, "a number");
      return this._value;
    },
    configurable: true,
  });

  Object.defineProperty(XPathResult.prototype, "stringValue", {
    get: function () {
      need(this._type === XPATH_RESULT_TYPES.STRING_TYPE, "a string");
      return this._value;
    },
    configurable: true,
  });

  Object.defineProperty(XPathResult.prototype, "booleanValue", {
    get: function () {
      need(this._type === XPATH_RESULT_TYPES.BOOLEAN_TYPE, "a boolean");
      return this._value;
    },
    configurable: true,
  });

  Object.defineProperty(XPathResult.prototype, "singleNodeValue", {
    get: function () {
      need(
        this._type === XPATH_RESULT_TYPES.ANY_UNORDERED_NODE_TYPE ||
          this._type === XPATH_RESULT_TYPES.FIRST_ORDERED_NODE_TYPE,
        "a single node"
      );
      return this._value[0] ?? null;
    },
    configurable: true,
  });

  Object.defineProperty(XPathResult.prototype, "invalidIteratorState", {
    get: function () {
      return (
        (this._type === XPATH_RESULT_TYPES.UNORDERED_NODE_ITERATOR_TYPE ||
          this._type === XPATH_RESULT_TYPES.ORDERED_NODE_ITERATOR_TYPE) &&
        ((typeof globalThis !== "undefined" && globalThis.__treeSeq) || 0) !== this._seq
      );
    },
    configurable: true,
  });

  Object.defineProperty(XPathResult.prototype, "snapshotLength", {
    get: function () {
      need(
        this._type === XPATH_RESULT_TYPES.UNORDERED_NODE_SNAPSHOT_TYPE ||
          this._type === XPATH_RESULT_TYPES.ORDERED_NODE_SNAPSHOT_TYPE,
        "a snapshot"
      );
      return this._value.length;
    },
    configurable: true,
  });

  XPathResult.prototype.iterateNext = function () {
    need(
      this._type === XPATH_RESULT_TYPES.UNORDERED_NODE_ITERATOR_TYPE ||
        this._type === XPATH_RESULT_TYPES.ORDERED_NODE_ITERATOR_TYPE,
      "an iterator"
    );
    if (this.invalidIteratorState) {
      throw getDOMException(
        "The document has mutated since the result was returned.",
        "InvalidStateError"
      );
    }
    return this._value[this._cursor++] ?? null;
  };

  XPathResult.prototype.snapshotItem = function (index) {
    need(
      this._type === XPATH_RESULT_TYPES.UNORDERED_NODE_SNAPSHOT_TYPE ||
        this._type === XPATH_RESULT_TYPES.ORDERED_NODE_SNAPSHOT_TYPE,
      "a snapshot"
    );
    return this._value[Number(index)] ?? null;
  };

  for (const [k, v] of Object.entries(XPATH_RESULT_TYPES)) {
    Object.defineProperty(XPathResult, k, { value: v, writable: false, enumerable: true, configurable: false });
    Object.defineProperty(XPathResult.prototype, k, { value: v, writable: false, enumerable: true, configurable: false });
  }

  function toXPathResultType(value) {
    if (typeof value === "bigint") {
      throw new TypeError("Cannot convert a BigInt value to a number");
    }
    const number = Number(value);
    if (!Number.isFinite(number) || number === 0) return 0;
    const modulo = Math.trunc(number) % 0x10000;
    return (modulo + 0x10000) % 0x10000;
  }

  function makeResult(value, type, reuse = null) {
    let t = toXPathResultType(type);
    if (t === XPATH_RESULT_TYPES.ANY_TYPE) {
      t = isNodeSet(value)
        ? XPATH_RESULT_TYPES.UNORDERED_NODE_ITERATOR_TYPE
        : typeof value === "number"
          ? XPATH_RESULT_TYPES.NUMBER_TYPE
          : typeof value === "string"
            ? XPATH_RESULT_TYPES.STRING_TYPE
            : XPATH_RESULT_TYPES.BOOLEAN_TYPE;
    }
    let resultValue;
    if (t === XPATH_RESULT_TYPES.NUMBER_TYPE) resultValue = toNumber(value);
    else if (t === XPATH_RESULT_TYPES.STRING_TYPE) resultValue = toStringValue(value);
    else if (t === XPATH_RESULT_TYPES.BOOLEAN_TYPE) resultValue = toBoolean(value);
    else {
      if (t < 0 || t > XPATH_RESULT_TYPES.FIRST_ORDERED_NODE_TYPE) {
        throw getDOMException("The result type is not supported.", "NotSupportedError");
      }
      if (!isNodeSet(value)) {
        throw new TypeError(
          "The result is not a node set, and therefore cannot be converted to the desired type."
        );
      }
      resultValue = value;
    }
    if (reuse) {
      reuse._refill(t, resultValue);
      return reuse;
    }
    return new XPathResult(INTERNAL_CONSTRUCT, t, resultValue);
  }

  function isXPathResult(value) {
    return typeof value === "object" && value !== null && xpathResultInstances.has(value);
  }

  function reusableResult(value) {
    if (value === null || value === undefined) return null;
    if (!isXPathResult(value)) throw new TypeError("The result is not an XPathResult.");
    return value;
  }

  function XPathExpression(token, expr) {
    if (token !== INTERNAL_CONSTRUCT || !expr) throw new TypeError("Illegal constructor");
    this._expr = expr;
  }

  XPathExpression.prototype.evaluate = function (contextNode, type = 0, result = null) {
    if (!isNode(contextNode)) {
      throw new TypeError(
        "Failed to execute 'evaluate' on 'XPathExpression': parameter 1 is not of type 'Node'."
      );
    }
    const reuse = reusableResult(result);
    const ev = new Evaluation(rootOf(contextNode));
    const value = evaluate(this._expr, { node: contextNode, pos: 1, size: 1, ev });
    return makeResult(isNodeSet(value) ? ev.sort(value) : value, type, reuse);
  };

  function createExpression(expression, resolver = null) {
    return new XPathExpression(INTERNAL_CONSTRUCT, compile(String(expression), resolver));
  }

  function evaluateXPath(
    expression,
    contextNode,
    resolver = null,
    type = 0,
    result = null
  ) {
    reusableResult(result);
    return createExpression(expression, resolver).evaluate(contextNode, type, result);
  }

  function XPathEvaluator() {}

  XPathEvaluator.prototype.createExpression = function (expression, resolver = null) {
    return createExpression(expression, resolver);
  };

  XPathEvaluator.prototype.createNSResolver = function (nodeResolver) {
    return nodeResolver;
  };

  XPathEvaluator.prototype.evaluate = function (
    expression,
    contextNode,
    resolver = null,
    type = 0,
    result = null
  ) {
    return evaluateXPath(expression, contextNode, resolver, type, result);
  };

  if (typeof markNative === "function") {
    markNative(XPathResult, "XPathResult");
    markNative(XPathExpression, "XPathExpression");
    markNative(XPathEvaluator, "XPathEvaluator");
    markNative(XPathEvaluator.prototype.createExpression, "createExpression");
    markNative(XPathEvaluator.prototype.createNSResolver, "createNSResolver");
    markNative(XPathEvaluator.prototype.evaluate, "evaluate");
    markNative(XPathExpression.prototype.evaluate, "evaluate");
    markNative(XPathResult.prototype.iterateNext, "iterateNext");
    markNative(XPathResult.prototype.snapshotItem, "snapshotItem");
    markNative(createExpression, "createExpression");
    markNative(evaluateXPath, "evaluateXPath");
  }

  // =============================================================
  // Module: canvas2d.js
  // =============================================================
  // src/js/shim/canvas2d.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/canvas2d.ts
  // Cloud vector recording and rendering engine for HTML5 Canvas 2D.
  // Configured by globalThis.__CAP_RENDER_CONFIG.canvasRecording (true: record ops, false: headless fast stub).

  const VALID_PNG_BASE64 = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=";

  const _INTERNAL_CTX_SECRET = Symbol("CanvasRenderingContext2D_internal");
  const _ctxInstances = new WeakSet();

  function checkCtx(self) {
    if (!self || typeof self !== "object" || !_ctxInstances.has(self)) {
      throw new TypeError("Illegal invocation");
    }
  }

  const NAMED_COLORS = {
    transparent: [0, 0, 0, 0],
    black: [0, 0, 0, 1],
    white: [255, 255, 255, 1],
    red: [255, 0, 0, 1],
    green: [0, 128, 0, 1],
    blue: [0, 0, 255, 1],
    yellow: [255, 255, 0, 1],
    cyan: [0, 255, 255, 1],
    aqua: [0, 255, 255, 1],
    magenta: [255, 0, 255, 1],
    fuchsia: [255, 0, 255, 1],
    gray: [128, 128, 128, 1],
    grey: [128, 128, 128, 1],
    lightgray: [211, 211, 211, 1],
    lightgrey: [211, 211, 211, 1],
    darkgray: [169, 169, 169, 1],
    darkgrey: [169, 169, 169, 1],
    orange: [255, 165, 0, 1],
    pink: [255, 192, 203, 1],
    purple: [128, 0, 128, 1],
    brown: [165, 42, 42, 1],
    lime: [0, 255, 0, 1],
    maroon: [128, 0, 0, 1],
    navy: [0, 0, 128, 1],
    olive: [128, 128, 0, 1],
    silver: [192, 192, 192, 1],
    teal: [0, 128, 128, 1],
    coral: [255, 127, 80, 1],
    gold: [255, 215, 0, 1],
    indigo: [75, 0, 130, 1],
    violet: [238, 130, 238, 1],
  };

  function parseColor(str) {
    if (typeof str !== "string") return null;
    const s = str.trim().toLowerCase();
    if (!s) return null;

    if (Object.prototype.hasOwnProperty.call(NAMED_COLORS, s)) {
      return NAMED_COLORS[s].slice();
    }

    if (s.charCodeAt(0) === 35) { // '#'
      const hex = s.slice(1);
      if (hex.length === 3) {
        const r = parseInt(hex[0] + hex[0], 16);
        const g = parseInt(hex[1] + hex[1], 16);
        const b = parseInt(hex[2] + hex[2], 16);
        if (!Number.isNaN(r) && !Number.isNaN(g) && !Number.isNaN(b)) {
          return [r, g, b, 1.0];
        }
      } else if (hex.length === 4) {
        const r = parseInt(hex[0] + hex[0], 16);
        const g = parseInt(hex[1] + hex[1], 16);
        const b = parseInt(hex[2] + hex[2], 16);
        const a = parseInt(hex[3] + hex[3], 16) / 255.0;
        if (!Number.isNaN(r) && !Number.isNaN(g) && !Number.isNaN(b) && !Number.isNaN(a)) {
          return [r, g, b, a];
        }
      } else if (hex.length === 6) {
        const r = parseInt(hex.slice(0, 2), 16);
        const g = parseInt(hex.slice(2, 4), 16);
        const b = parseInt(hex.slice(4, 6), 16);
        if (!Number.isNaN(r) && !Number.isNaN(g) && !Number.isNaN(b)) {
          return [r, g, b, 1.0];
        }
      } else if (hex.length === 8) {
        const r = parseInt(hex.slice(0, 2), 16);
        const g = parseInt(hex.slice(2, 4), 16);
        const b = parseInt(hex.slice(4, 6), 16);
        const a = parseInt(hex.slice(6, 8), 16) / 255.0;
        if (!Number.isNaN(r) && !Number.isNaN(g) && !Number.isNaN(b) && !Number.isNaN(a)) {
          return [r, g, b, a];
        }
      }
      return null;
    }

    if (s.startsWith("rgb(") || s.startsWith("rgba(")) {
      const start = s.indexOf("(") + 1;
      const end = s.indexOf(")");
      if (end > start) {
        const parts = s.slice(start, end).split(/[, /]+/).map(function (p) { return p.trim(); }).filter(Boolean);
        if (parts.length >= 3) {
          const parseComponent = function (p) {
            if (p.endsWith("%")) return Math.round((parseFloat(p) / 100.0) * 255.0);
            return Math.round(parseFloat(p));
          };
          const r = Math.min(255, Math.max(0, parseComponent(parts[0])));
          const g = Math.min(255, Math.max(0, parseComponent(parts[1])));
          const b = Math.min(255, Math.max(0, parseComponent(parts[2])));
          let a = 1.0;
          if (parts.length >= 4) {
            const ap = parts[3];
            if (ap.endsWith("%")) a = parseFloat(ap) / 100.0;
            else a = parseFloat(ap);
            a = Math.min(1.0, Math.max(0.0, a));
          }
          if (!Number.isNaN(r) && !Number.isNaN(g) && !Number.isNaN(b) && !Number.isNaN(a)) {
            return [r, g, b, a];
          }
        }
      }
      return null;
    }

    return null;
  }

  function formatColor(rgba) {
    const r = Math.min(255, Math.max(0, Math.round(rgba[0])));
    const g = Math.min(255, Math.max(0, Math.round(rgba[1])));
    const b = Math.min(255, Math.max(0, Math.round(rgba[2])));
    const a = rgba[3];
    if (a >= 1.0) {
      const hex = ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
      return "#" + hex;
    }
    const aFormatted = Number(a.toFixed(3)).toString();
    return "rgba(" + r + ", " + g + ", " + b + ", " + aFormatted + ")";
  }

  function getCanvasWidth(canvas) {
    if (!canvas) return 300;
    const w = parseInt(canvas.width, 10);
    return Number.isFinite(w) && w > 0 ? w : 300;
  }

  function getCanvasHeight(canvas) {
    if (!canvas) return 150;
    const h = parseInt(canvas.height, 10);
    return Number.isFinite(h) && h > 0 ? h : 150;
  }

  function ensureBitmap(ctx) {
    const w = getCanvasWidth(ctx._canvas);
    const h = getCanvasHeight(ctx._canvas);
    if (!ctx._bitmap || ctx._bitmapWidth !== w || ctx._bitmapHeight !== h) {
      ctx._bitmapWidth = w;
      ctx._bitmapHeight = h;
      ctx._bitmap = new Uint8ClampedArray(w * h * 4);
    }
    return ctx._bitmap;
  }

  function CanvasGradient(type, x0, y0, r0, x1, y1, r1) {
    this._type = type; // "linear" or "radial"
    this._stops = [];
    this._coords = [x0, y0, r0, x1, y1, r1];
  }
  CanvasGradient.prototype.addColorStop = function (offset, color) {
    this._stops.push({ offset: Number(offset), color: String(color) });
  };
  markNative(CanvasGradient, "CanvasGradient");
  markNative(CanvasGradient.prototype.addColorStop, "addColorStop");

  function CanvasPattern(image, repetition) {
    this._image = image;
    this._repetition = String(repetition || "repeat");
    this._matrix = [1, 0, 0, 1, 0, 0];
  }
  CanvasPattern.prototype.setTransform = function (matrix) {
    if (matrix && matrix.a !== undefined) {
      this._matrix = [matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f];
    }
  };
  markNative(CanvasPattern, "CanvasPattern");
  markNative(CanvasPattern.prototype.setTransform, "setTransform");

  function Path2D(path) {
    this._ops = [];
    if (path instanceof Path2D) {
      this._ops = path._ops.slice();
    }
  }
  const pathMethods = ["moveTo", "lineTo", "quadraticCurveTo", "bezierCurveTo", "arcTo", "arc", "ellipse", "rect", "closePath"];
  for (let i = 0; i < pathMethods.length; i++) {
    const m = pathMethods[i];
    Path2D.prototype[m] = function () {
      this._ops.push({ op: m, args: Array.prototype.slice.call(arguments) });
    };
  }
  Path2D.prototype.addPath = function (path, transform) {
    if (path instanceof Path2D) {
      this._ops.push({ op: "addPath", pathOps: path._ops.slice(), transform: transform });
    }
  };
  markNative(Path2D, "Path2D");

  function TextMetrics(text, font) {
    const str = String(text || "");
    const fontSizeMatch = (font || "").match(/(\d+)px/);
    const fontSize = fontSizeMatch ? parseFloat(fontSizeMatch[1]) : 10;
    const width = str.length * fontSize * 0.6;
    this.width = width;
    this.actualBoundingBoxLeft = 0.0;
    this.actualBoundingBoxRight = width;
    this.actualBoundingBoxAscent = fontSize * 0.8;
    this.actualBoundingBoxDescent = fontSize * 0.2;
    this.fontBoundingBoxAscent = fontSize;
    this.fontBoundingBoxDescent = fontSize * 0.25;
    this.alphabeticBaseline = 0.0;
  }
  markNative(TextMetrics, "TextMetrics");

  function ImageData(w, h, data) {
    if (typeof w === "object" && w instanceof Uint8ClampedArray) {
      this.data = w;
      this.width = Number(h || 1);
      this.height = Math.max(1, (this.data.length / (4 * this.width)) | 0);
    } else {
      this.width = Math.max(1, Number(w || 1) | 0);
      this.height = Math.max(1, Number(h || 1) | 0);
      this.data = data instanceof Uint8ClampedArray ? data : new Uint8ClampedArray(this.width * this.height * 4);
    }
    this.colorSpace = "srgb";
  }
  markNative(ImageData, "ImageData");

  function defaultCanvasState() {
    return {
      strokeStyle: "#000000",
      fillStyle: "#000000",
      lineWidth: 1.0,
      lineCap: "butt",
      lineJoin: "miter",
      miterLimit: 10.0,
      lineDash: [],
      lineDashOffset: 0.0,
      globalAlpha: 1.0,
      globalCompositeOperation: "source-over",
      shadowColor: "rgba(0, 0, 0, 0)",
      shadowBlur: 0,
      shadowOffsetX: 0,
      shadowOffsetY: 0,
      font: "10px sans-serif",
      textAlign: "start",
      textBaseline: "alphabetic",
      direction: "inherit",
      filter: "none",
      imageSmoothingEnabled: true,
      matrix: [1, 0, 0, 1, 0, 0],
    };
  }

  function cloneCanvasState(s) {
    return {
      strokeStyle: s.strokeStyle,
      fillStyle: s.fillStyle,
      lineWidth: s.lineWidth,
      lineCap: s.lineCap,
      lineJoin: s.lineJoin,
      miterLimit: s.miterLimit,
      lineDash: s.lineDash.slice(),
      lineDashOffset: s.lineDashOffset,
      globalAlpha: s.globalAlpha,
      globalCompositeOperation: s.globalCompositeOperation,
      shadowColor: s.shadowColor,
      shadowBlur: s.shadowBlur,
      shadowOffsetX: s.shadowOffsetX,
      shadowOffsetY: s.shadowOffsetY,
      font: s.font,
      textAlign: s.textAlign,
      textBaseline: s.textBaseline,
      direction: s.direction,
      filter: s.filter,
      imageSmoothingEnabled: s.imageSmoothingEnabled,
      matrix: s.matrix.slice(),
    };
  }

  function CanvasRenderingContext2D(canvas, secret) {
    if (secret !== _INTERNAL_CTX_SECRET) {
      throw new TypeError("Illegal constructor");
    }
    _ctxInstances.add(this);
    this._canvas = canvas || null;
    this._state = defaultCanvasState();
    this._stack = [];
    this._ops = []; // Recorded vector operations
    this._bitmap = null;
    this._bitmapWidth = 0;
    this._bitmapHeight = 0;
  }

  // State stack methods
  CanvasRenderingContext2D.prototype.save = function () {
    checkCtx(this);
    this._stack.push(cloneCanvasState(this._state));
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "save" });
    }
  };

  CanvasRenderingContext2D.prototype.restore = function () {
    checkCtx(this);
    if (this._stack.length > 0) {
      this._state = this._stack.pop();
      if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
        this._ops.push({ op: "restore" });
      }
    }
  };

  // Transform methods
  CanvasRenderingContext2D.prototype.scale = function (x, y) {
    checkCtx(this);
    this._state.matrix[0] *= x;
    this._state.matrix[3] *= y;
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "scale", x: x, y: y });
    }
  };

  CanvasRenderingContext2D.prototype.rotate = function (angle) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "rotate", angle: angle });
    }
  };

  CanvasRenderingContext2D.prototype.translate = function (x, y) {
    checkCtx(this);
    this._state.matrix[4] += x;
    this._state.matrix[5] += y;
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "translate", x: x, y: y });
    }
  };

  CanvasRenderingContext2D.prototype.transform = function (a, b, c, d, e, f) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "transform", args: [a, b, c, d, e, f] });
    }
  };

  CanvasRenderingContext2D.prototype.setTransform = function (a, b, c, d, e, f) {
    checkCtx(this);
    if (typeof a === "object" && a !== null) {
      this._state.matrix = [a.a || 1, a.b || 0, a.c || 0, a.d || 1, a.e || 0, a.f || 0];
    } else {
      this._state.matrix = [a || 1, b || 0, c || 0, d || 1, e || 0, f || 0];
    }
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "setTransform", matrix: this._state.matrix.slice() });
    }
  };

  CanvasRenderingContext2D.prototype.resetTransform = function () {
    checkCtx(this);
    this._state.matrix = [1, 0, 0, 1, 0, 0];
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "resetTransform" });
    }
  };

  CanvasRenderingContext2D.prototype.getTransform = function () {
    checkCtx(this);
    const m = this._state.matrix;
    return { a: m[0], b: m[1], c: m[2], d: m[3], e: m[4], f: m[5] };
  };

  // Rectangle drawing
  CanvasRenderingContext2D.prototype.clearRect = function (x, y, w, h) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "clearRect", args: [x, y, w, h] });
    }

    const numX = Number(x);
    const numY = Number(y);
    let numW = Number(w);
    let numH = Number(h);
    if (!Number.isFinite(numX) || !Number.isFinite(numY) || !Number.isFinite(numW) || !Number.isFinite(numH)) return;
    if (numW === 0 || numH === 0) return;

    const bitmap = ensureBitmap(this);
    const bw = this._bitmapWidth;
    const bh = this._bitmapHeight;

    let startX = numX;
    let startY = numY;
    if (numW < 0) {
      startX += numW;
      numW = -numW;
    }
    if (numH < 0) {
      startY += numH;
      numH = -numH;
    }

    const x0 = Math.max(0, Math.floor(startX));
    const y0 = Math.max(0, Math.floor(startY));
    const x1 = Math.min(bw, Math.ceil(startX + numW));
    const y1 = Math.min(bh, Math.ceil(startY + numH));

    for (let py = y0; py < y1; py++) {
      let idx = (py * bw + x0) * 4;
      for (let px = x0; px < x1; px++) {
        bitmap[idx] = 0;
        bitmap[idx + 1] = 0;
        bitmap[idx + 2] = 0;
        bitmap[idx + 3] = 0;
        idx += 4;
      }
    }
  };

  CanvasRenderingContext2D.prototype.fillRect = function (x, y, w, h) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "fillRect", args: [x, y, w, h], fillStyle: this._state.fillStyle });
    }

    const numX = Number(x);
    const numY = Number(y);
    let numW = Number(w);
    let numH = Number(h);
    if (!Number.isFinite(numX) || !Number.isFinite(numY) || !Number.isFinite(numW) || !Number.isFinite(numH)) return;
    if (numW === 0 || numH === 0) return;

    const color = parseColor(this._state.fillStyle);
    if (!color) return;

    const bitmap = ensureBitmap(this);
    const bw = this._bitmapWidth;
    const bh = this._bitmapHeight;

    let startX = numX;
    let startY = numY;
    if (numW < 0) {
      startX += numW;
      numW = -numW;
    }
    if (numH < 0) {
      startY += numH;
      numH = -numH;
    }

    const x0 = Math.max(0, Math.floor(startX));
    const y0 = Math.max(0, Math.floor(startY));
    const x1 = Math.min(bw, Math.ceil(startX + numW));
    const y1 = Math.min(bh, Math.ceil(startY + numH));

    const alpha = color[3] * this._state.globalAlpha;
    if (alpha <= 0) return;

    const rByte = Math.min(255, Math.max(0, Math.round(color[0])));
    const gByte = Math.min(255, Math.max(0, Math.round(color[1])));
    const bByte = Math.min(255, Math.max(0, Math.round(color[2])));
    const aByte = Math.min(255, Math.max(0, Math.round(alpha * 255)));

    if (alpha >= 1.0) {
      for (let py = y0; py < y1; py++) {
        let idx = (py * bw + x0) * 4;
        for (let px = x0; px < x1; px++) {
          bitmap[idx] = rByte;
          bitmap[idx + 1] = gByte;
          bitmap[idx + 2] = bByte;
          bitmap[idx + 3] = 255;
          idx += 4;
        }
      }
    } else {
      const invA = 1.0 - alpha;
      for (let py = y0; py < y1; py++) {
        let idx = (py * bw + x0) * 4;
        for (let px = x0; px < x1; px++) {
          const dstR = bitmap[idx];
          const dstG = bitmap[idx + 1];
          const dstB = bitmap[idx + 2];
          const dstA = bitmap[idx + 3] / 255.0;

          const outA = alpha + dstA * invA;
          if (outA > 0) {
            bitmap[idx] = Math.round((color[0] * alpha + dstR * dstA * invA) / outA);
            bitmap[idx + 1] = Math.round((color[1] * alpha + dstG * dstA * invA) / outA);
            bitmap[idx + 2] = Math.round((color[2] * alpha + dstB * dstA * invA) / outA);
            bitmap[idx + 3] = Math.round(outA * 255);
          }
          idx += 4;
        }
      }
    }
  };

  CanvasRenderingContext2D.prototype.strokeRect = function (x, y, w, h) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "strokeRect", args: [x, y, w, h], strokeStyle: this._state.strokeStyle, lineWidth: this._state.lineWidth });
    }
  };

  // Path drawing methods
  CanvasRenderingContext2D.prototype.beginPath = function () {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "beginPath" });
    }
  };

  CanvasRenderingContext2D.prototype.closePath = function () {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "closePath" });
    }
  };

  CanvasRenderingContext2D.prototype.moveTo = function (x, y) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "moveTo", x: x, y: y });
    }
  };

  CanvasRenderingContext2D.prototype.lineTo = function (x, y) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "lineTo", x: x, y: y });
    }
  };

  CanvasRenderingContext2D.prototype.quadraticCurveTo = function (cpx, cpy, x, y) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "quadraticCurveTo", args: [cpx, cpy, x, y] });
    }
  };

  CanvasRenderingContext2D.prototype.bezierCurveTo = function (cp1x, cp1y, cp2x, cp2y, x, y) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "bezierCurveTo", args: [cp1x, cp1y, cp2x, cp2y, x, y] });
    }
  };

  CanvasRenderingContext2D.prototype.arcTo = function (x1, y1, x2, y2, radius) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "arcTo", args: [x1, y1, x2, y2, radius] });
    }
  };

  CanvasRenderingContext2D.prototype.arc = function (x, y, radius, startAngle, endAngle, counterclockwise) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "arc", args: [x, y, radius, startAngle, endAngle, Boolean(counterclockwise)] });
    }
  };

  CanvasRenderingContext2D.prototype.ellipse = function (x, y, radiusX, radiusY, rotation, startAngle, endAngle, counterclockwise) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "ellipse", args: [x, y, radiusX, radiusY, rotation, startAngle, endAngle, Boolean(counterclockwise)] });
    }
  };

  CanvasRenderingContext2D.prototype.rect = function (x, y, w, h) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "rect", args: [x, y, w, h] });
    }
  };

  CanvasRenderingContext2D.prototype.fill = function (pathOrRule, rule) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "fill", fillStyle: this._state.fillStyle });
    }
  };

  CanvasRenderingContext2D.prototype.stroke = function (path) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "stroke", strokeStyle: this._state.strokeStyle, lineWidth: this._state.lineWidth });
    }
  };

  CanvasRenderingContext2D.prototype.clip = function (pathOrRule, rule) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "clip" });
    }
  };

  CanvasRenderingContext2D.prototype.isPointInPath = function (x, y) { checkCtx(this); return true; };
  CanvasRenderingContext2D.prototype.isPointInStroke = function (x, y) { checkCtx(this); return true; };

  // Text drawing
  CanvasRenderingContext2D.prototype.fillText = function (text, x, y, maxWidth) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "fillText", text: String(text), x: x, y: y, maxWidth: maxWidth, font: this._state.font, fillStyle: this._state.fillStyle });
    }
  };

  CanvasRenderingContext2D.prototype.strokeText = function (text, x, y, maxWidth) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "strokeText", text: String(text), x: x, y: y, maxWidth: maxWidth, font: this._state.font, strokeStyle: this._state.strokeStyle });
    }
  };

  CanvasRenderingContext2D.prototype.measureText = function (text) {
    checkCtx(this);
    return new TextMetrics(text, this._state.font);
  };

  // Image drawing
  CanvasRenderingContext2D.prototype.drawImage = function (image) {
    checkCtx(this);
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "drawImage", args: Array.prototype.slice.call(arguments, 1) });
    }
  };

  // Gradients and Patterns
  CanvasRenderingContext2D.prototype.createLinearGradient = function (x0, y0, x1, y1) {
    checkCtx(this);
    return new CanvasGradient("linear", x0, y0, 0, x1, y1, 0);
  };

  CanvasRenderingContext2D.prototype.createRadialGradient = function (x0, y0, r0, x1, y1, r1) {
    checkCtx(this);
    return new CanvasGradient("radial", x0, y0, r0, x1, y1, r1);
  };

  CanvasRenderingContext2D.prototype.createPattern = function (image, repetition) {
    checkCtx(this);
    return new CanvasPattern(image, repetition);
  };

  // Pixel manipulation
  CanvasRenderingContext2D.prototype.createImageData = function (sw, sh) {
    checkCtx(this);
    if (typeof sw === "object" && sw !== null) {
      const w = (sw.width | 0);
      const h = (sw.height | 0);
      return new ImageData(Math.abs(w) || 1, Math.abs(h) || 1);
    }
    const w = (sw | 0);
    const h = (sh | 0);
    return new ImageData(Math.abs(w) || 1, Math.abs(h) || 1);
  };

  CanvasRenderingContext2D.prototype.getImageData = function (sx, sy, sw, sh) {
    checkCtx(this);
    let intSx = (sx | 0);
    let intSy = (sy | 0);
    let intSw = (sw | 0);
    let intSh = (sh | 0);

    if (intSw === 0 || intSh === 0) {
      const err = new Error("IndexSizeError");
      err.name = "IndexSizeError";
      throw err;
    }

    if (intSw < 0) {
      intSx += intSw;
      intSw = -intSw;
    }
    if (intSh < 0) {
      intSy += intSh;
      intSh = -intSh;
    }

    const imgData = new ImageData(intSw, intSh);
    const outData = imgData.data;

    const bitmap = ensureBitmap(this);
    const bw = this._bitmapWidth;
    const bh = this._bitmapHeight;

    for (let y = 0; y < intSh; y++) {
      const srcY = intSy + y;
      for (let x = 0; x < intSw; x++) {
        const srcX = intSx + x;
        const dstIdx = (y * intSw + x) * 4;
        if (srcX >= 0 && srcX < bw && srcY >= 0 && srcY < bh) {
          const srcIdx = (srcY * bw + srcX) * 4;
          outData[dstIdx] = bitmap[srcIdx];
          outData[dstIdx + 1] = bitmap[srcIdx + 1];
          outData[dstIdx + 2] = bitmap[srcIdx + 2];
          outData[dstIdx + 3] = bitmap[srcIdx + 3];
        } else {
          outData[dstIdx] = 0;
          outData[dstIdx + 1] = 0;
          outData[dstIdx + 2] = 0;
          outData[dstIdx + 3] = 0;
        }
      }
    }

    return imgData;
  };

  CanvasRenderingContext2D.prototype.putImageData = function (imagedata, dx, dy) {
    checkCtx(this);
    if (!imagedata || !imagedata.data) {
      throw new TypeError("Failed to execute 'putImageData' on 'CanvasRenderingContext2D': parameter 1 is not of type 'ImageData'.");
    }
    const intDx = (dx | 0);
    const intDy = (dy | 0);
    const intW = (imagedata.width | 0);
    const intH = (imagedata.height | 0);
    const inData = imagedata.data;

    const bitmap = ensureBitmap(this);
    const bw = this._bitmapWidth;
    const bh = this._bitmapHeight;

    for (let y = 0; y < intH; y++) {
      const dstY = intDy + y;
      if (dstY < 0 || dstY >= bh) continue;
      for (let x = 0; x < intW; x++) {
        const dstX = intDx + x;
        if (dstX < 0 || dstX >= bw) continue;
        const srcIdx = (y * intW + x) * 4;
        const dstIdx = (dstY * bw + dstX) * 4;
        bitmap[dstIdx] = inData[srcIdx];
        bitmap[dstIdx + 1] = inData[srcIdx + 1];
        bitmap[dstIdx + 2] = inData[srcIdx + 2];
        bitmap[dstIdx + 3] = inData[srcIdx + 3];
      }
    }

    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "putImageData", dx: intDx, dy: intDy, width: intW, height: intH });
    }
  };

  // Dash styling
  CanvasRenderingContext2D.prototype.setLineDash = function (segments) {
    checkCtx(this);
    this._state.lineDash = Array.isArray(segments) ? segments.slice() : [];
  };

  CanvasRenderingContext2D.prototype.getLineDash = function () {
    checkCtx(this);
    return this._state.lineDash.slice();
  };

  CanvasRenderingContext2D.prototype._setPaintStyle = function (which, v) {
    if (typeof v === "string") {
      const parsed = parseColor(v);
      if (!parsed) return;
      const formatted = formatColor(parsed);
      this._state[which] = formatted;
      if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
        this._ops.push({ op: which, color: formatted });
      }
    } else if (v instanceof CanvasGradient || v instanceof CanvasPattern) {
      this._state[which] = v;
    }
  };

  // Properties with getters and setters
  const stateProps = [
    "lineWidth", "lineCap", "lineJoin", "miterLimit",
    "lineDashOffset", "globalAlpha", "globalCompositeOperation", "shadowColor",
    "shadowBlur", "shadowOffsetX", "shadowOffsetY", "font", "textAlign",
    "textBaseline", "direction", "filter", "imageSmoothingEnabled"
  ];

  for (let i = 0; i < stateProps.length; i++) {
    const p = stateProps[i];
    Object.defineProperty(CanvasRenderingContext2D.prototype, p, {
      get: function () {
        checkCtx(this);
        return this._state[p];
      },
      set: function (v) {
        checkCtx(this);
        this._state[p] = v;
      },
      configurable: true,
      enumerable: true,
    });
  }

  Object.defineProperty(CanvasRenderingContext2D.prototype, "fillStyle", {
    get: function () {
      checkCtx(this);
      return this._state.fillStyle;
    },
    set: function (v) {
      checkCtx(this);
      this._setPaintStyle("fillStyle", v);
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(CanvasRenderingContext2D.prototype, "strokeStyle", {
    get: function () {
      checkCtx(this);
      return this._state.strokeStyle;
    },
    set: function (v) {
      checkCtx(this);
      this._setPaintStyle("strokeStyle", v);
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(CanvasRenderingContext2D.prototype, "canvas", {
    get: function () {
      checkCtx(this);
      return this._canvas;
    },
    configurable: true,
    enumerable: true,
  });

  // Vector recording extraction and inspection API for CapSurf
  CanvasRenderingContext2D.prototype.__getRecordingOps = function () {
    checkCtx(this);
    return this._ops.slice();
  };

  CanvasRenderingContext2D.prototype.__clearRecordingOps = function () {
    checkCtx(this);
    this._ops = [];
  };

  CanvasRenderingContext2D.prototype.__opsCount = function () {
    checkCtx(this);
    return this._ops.length;
  };

  markNative(CanvasRenderingContext2D, "CanvasRenderingContext2D");
  markNative(CanvasRenderingContext2D.prototype.save, "save");
  markNative(CanvasRenderingContext2D.prototype.restore, "restore");
  markNative(CanvasRenderingContext2D.prototype.fillRect, "fillRect");
  markNative(CanvasRenderingContext2D.prototype.strokeRect, "strokeRect");
  markNative(CanvasRenderingContext2D.prototype.clearRect, "clearRect");
  markNative(CanvasRenderingContext2D.prototype.measureText, "measureText");
  markNative(CanvasRenderingContext2D.prototype.getImageData, "getImageData");

  // =============================================================
  // Module: animations.js
  // =============================================================
  // src/js/shim/animations.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/animations.ts
  // Web Animations API (web-animations-1) implementation.
  // Configured by globalThis.__CAP_RENDER_CONFIG.animations (true: full playback timeline, false: static headless immediate finish).

  function AnimationTimeline() {}
  AnimationTimeline.prototype.currentTime = 0;
  markNative(AnimationTimeline, "AnimationTimeline");

  function DocumentTimeline(options) {
    AnimationTimeline.call(this);
    this.originTime = Number(options && options.originTime || 0);
  }
  DocumentTimeline.prototype = Object.create(AnimationTimeline.prototype);
  DocumentTimeline.prototype.constructor = DocumentTimeline;
  Object.defineProperty(DocumentTimeline.prototype, "currentTime", {
    get: function () {
      return (typeof performance !== "undefined" && performance.now) ? performance.now() : Date.now();
    },
    configurable: true,
  });
  markNative(DocumentTimeline, "DocumentTimeline");

  const globalDocTimeline = new DocumentTimeline();

  function AnimationEffect() {}
  markNative(AnimationEffect, "AnimationEffect");

  function KeyframeEffect(target, keyframes, options) {
    AnimationEffect.call(this);
    this.target = target || null;
    this.keyframes = Array.isArray(keyframes) ? keyframes : [];
    const opts = typeof options === "number" ? { duration: options } : (options || {});
    this.delay = Number(opts.delay || 0);
    this.endDelay = Number(opts.endDelay || 0);
    this.fill = String(opts.fill || "auto");
    this.iterationStart = Number(opts.iterationStart || 0);
    this.iterations = Number(opts.iterations || 1);
    this.duration = opts.duration !== undefined ? (opts.duration === "auto" ? "auto" : Number(opts.duration)) : 0;
    this.direction = String(opts.direction || "normal");
    this.easing = String(opts.easing || "linear");
  }
  KeyframeEffect.prototype = Object.create(AnimationEffect.prototype);
  KeyframeEffect.prototype.constructor = KeyframeEffect;

  KeyframeEffect.prototype.getComputedTiming = function () {
    return {
      delay: this.delay,
      endDelay: this.endDelay,
      fill: this.fill,
      iterationStart: this.iterationStart,
      iterations: this.iterations,
      duration: this.duration,
      direction: this.direction,
      easing: this.easing,
      endTime: this.delay + (typeof this.duration === "number" ? this.duration * this.iterations : 0) + this.endDelay,
      activeDuration: typeof this.duration === "number" ? this.duration * this.iterations : 0,
      localTime: 0,
      progress: 1.0,
    };
  };

  KeyframeEffect.prototype.getKeyframes = function () {
    return this.keyframes.slice();
  };

  markNative(KeyframeEffect, "KeyframeEffect");
  markNative(KeyframeEffect.prototype.getComputedTiming, "getComputedTiming");
  markNative(KeyframeEffect.prototype.getKeyframes, "getKeyframes");

  const registeredAnimations = new Set();

  function Animation(effect, timeline) {
    EventTarget.call(this);
    this.id = "";
    this.effect = effect || null;
    this.timeline = timeline || globalDocTimeline;
    this.playbackRate = 1.0;
    this.startTime = null;
    this.currentTime = 0;
    this.pending = false;
    this.onfinish = null;
    this.oncancel = null;
    this.onremove = null;

    const isFullAnimation = Boolean(globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.animations);

    if (!isFullAnimation) {
      // Static headless mode: instant finish, zero background cpu burn
      this.playState = "finished";
      const dur = (effect && typeof effect.duration === "number") ? effect.duration : 0;
      this.currentTime = dur;
      this.ready = Promise.resolve(this);
      this.finished = Promise.resolve(this);
      // Apply end keyframes directly to target style
      if (effect && effect.target && effect.keyframes && effect.keyframes.length > 0) {
        const endFrame = effect.keyframes[effect.keyframes.length - 1];
        for (const k in endFrame) {
          if (k !== "offset" && k !== "easing" && k !== "composite") {
            try { effect.target.style[k] = endFrame[k]; } catch (e) {}
          }
        }
      }
    } else {
      // Full interactive animation mode
      this.playState = "idle";
      const self = this;
      this._readyResolve = null;
      this._finishedResolve = null;
      this.ready = new Promise(function (res) { self._readyResolve = res; });
      this.finished = new Promise(function (res) { self._finishedResolve = res; });
      registeredAnimations.add(this);
    }
  }
  Animation.prototype = Object.create(EventTarget.prototype);
  Animation.prototype.constructor = Animation;

  Animation.prototype.play = function () {
    if (!globalThis.__CAP_RENDER_CONFIG || !globalThis.__CAP_RENDER_CONFIG.animations) {
      this.playState = "finished";
      return;
    }
    this.playState = "running";
    this.startTime = this.timeline.currentTime;
    if (this._readyResolve) {
      this._readyResolve(this);
      this._readyResolve = null;
    }
    const self = this;
    const dur = (this.effect && typeof this.effect.duration === "number") ? this.effect.duration : 0;
    setTimeout(function () {
      if (self.playState === "running") {
        self.finish();
      }
    }, Math.max(0, dur));
  };

  Animation.prototype.pause = function () {
    if (this.playState === "running") {
      this.playState = "paused";
    }
  };

  Animation.prototype.finish = function () {
    this.playState = "finished";
    const dur = (this.effect && typeof this.effect.duration === "number") ? this.effect.duration : 0;
    this.currentTime = dur;

    // Apply end keyframes to target
    if (this.effect && this.effect.target && this.effect.keyframes && this.effect.keyframes.length > 0) {
      const endFrame = this.effect.keyframes[this.effect.keyframes.length - 1];
      for (const k in endFrame) {
        if (k !== "offset" && k !== "easing" && k !== "composite") {
          try { this.effect.target.style[k] = endFrame[k]; } catch (e) {}
        }
      }
    }

    if (this._finishedResolve) {
      this._finishedResolve(this);
      this._finishedResolve = null;
    }

    const evt = new Event("finish");
    this.dispatchEvent(evt);
    if (typeof this.onfinish === "function") {
      try { this.onfinish.call(this, evt); } catch (e) {}
    }
  };

  Animation.prototype.cancel = function () {
    this.playState = "idle";
    this.currentTime = null;
    this.startTime = null;
    const evt = new Event("cancel");
    this.dispatchEvent(evt);
    if (typeof this.oncancel === "function") {
      try { this.oncancel.call(this, evt); } catch (e) {}
    }
  };

  Animation.prototype.reverse = function () {
    this.playbackRate = -this.playbackRate;
    this.play();
  };

  markNative(Animation, "Animation");
  markNative(Animation.prototype.play, "play");
  markNative(Animation.prototype.pause, "pause");
  markNative(Animation.prototype.finish, "finish");
  markNative(Animation.prototype.cancel, "cancel");

  function animateElement(element, keyframes, options) {
    const effect = new KeyframeEffect(element, keyframes, options);
    const anim = new Animation(effect, globalDocTimeline);
    anim.play();
    return anim;
  }

  function animationsFor(root) {
    const res = [];
    for (const anim of registeredAnimations) {
      if (anim.effect && anim.effect.target) {
        if (!root || root === anim.effect.target || (root.contains && root.contains(anim.effect.target))) {
          res.push(anim);
        }
      }
    }
    return res;
  }

  // =============================================================
  // Module: window.js
  // =============================================================
  // src/js/shim/window.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/window.ts

  function installWindow(global) {
    let lastQueriedNode = null;
    global.__capsurf_setLastQueriedNode = function (node) {
      lastQueriedNode = node;
    };
    global.__capsurf_getLastQueriedNode = function () {
      return lastQueriedNode;
    };

    function NativeEvent(type, init) {
      Event.call(this, type, init);
      this._isHarnessEvent = true;
      const target = lastQueriedNode;
      let fallbackDispatched = false;
      Object.defineProperty(this, "_scheduleTrustedFallback", {
        value: function () {
          if (!fallbackDispatched && target) {
            fallbackDispatched = true;
            if (typeof queueMicrotask === "function") {
              queueMicrotask(function () {
                const tev = new MouseEvent(type, Object.assign({}, init, { bubbles: true, cancelable: true }));
                if (typeof dispatchTrustedEvent === "function") {
                  dispatchTrustedEvent(target, tev);
                } else if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
                  global.__capsurf_dispatchTrustedEvent(target, tev);
                }
              });
            }
          }
        },
        writable: true,
        configurable: true,
      });
    }
    NativeEvent.prototype = Object.create(Event.prototype);
    NativeEvent.prototype.constructor = NativeEvent;
    global.__NativeEvent = NativeEvent;
    if (typeof globalThis !== "undefined") {
      globalThis.__NativeEvent = NativeEvent;
    }

    // 0. DOMException & WHATWG URL Implementation
    if (typeof DOMException !== "undefined") {
      global.DOMException = DOMException;
      if (typeof window !== "undefined") window.DOMException = DOMException;
      if (typeof globalThis !== "undefined") globalThis.DOMException = DOMException;
    }

    function Window() {
      throw new TypeError("Illegal constructor");
    }
    markNative(Window, "Window");
    if (!global.Window) {
      global.Window = Window;
    }
    if (typeof window !== "undefined" && !window.Window) {
      window.Window = Window;
    }
    if (typeof globalThis !== "undefined" && !globalThis.Window) {
      globalThis.Window = Window;
    }
    if (!global.self) global.self = global;
    if (typeof window !== "undefined" && !window.self) window.self = window;
    if (typeof globalThis !== "undefined" && !globalThis.self) globalThis.self = globalThis;

    function resolveRelativePath(p) {
      const segments = p.split("/");
      const resolved = [];
      for (let i = 0; i < segments.length; i++) {
        const s = segments[i];
        if (s === "." || (s === "" && i > 0 && i < segments.length - 1)) {
          continue;
        } else if (s === "..") {
          if (resolved.length > 1) {
            resolved.pop();
          }
        } else {
          resolved.push(s);
        }
      }
      let res = resolved.join("/");
      if (!res.startsWith("/")) res = "/" + res;
      return res;
    }

    function parseHostPort(authHost) {
      let hostname = "";
      let port = "";
      if (authHost.startsWith("[")) {
        const closeBracket = authHost.indexOf("]");
        if (closeBracket !== -1) {
          hostname = authHost.slice(0, closeBracket + 1);
          const after = authHost.slice(closeBracket + 1);
          if (after.startsWith(":")) {
            port = after.slice(1);
            if (port === "" || !/^[0-9]+$/.test(port)) {
              throw new TypeError("Invalid URL port: " + port);
            }
          } else if (after.length > 0) {
            throw new TypeError("Invalid URL IPv6: " + authHost);
          } else {
            port = "";
          }
        } else {
          throw new TypeError("Unclosed IPv6 bracket in URL: " + authHost);
        }
      } else {
        const colons = authHost.split(":");
        if (colons.length > 2) {
          throw new TypeError("Invalid URL host/port: " + authHost);
        }
        if (colons.length === 2) {
          hostname = colons[0];
          port = colons[1];
          if (port === "" || !/^[0-9]+$/.test(port)) {
            throw new TypeError("Invalid URL port: " + port);
          }
        } else {
          hostname = authHost;
          port = "";
        }
      }
      const host = port ? hostname + ":" + port : hostname;
      return { hostname, port, host };
    }

    function parseUrlString(input, base) {
      let inStr = String(input).trim();
      let baseObj = null;
      if (base !== undefined && base !== null) {
        if (base instanceof URL) {
          baseObj = base._parsed;
        } else if (typeof base === "object" && base._parsed) {
          baseObj = base._parsed;
        } else {
          baseObj = parseUrlString(String(base));
        }
      }

      const schemeMatch = inStr.match(/^([a-zA-Z][a-zA-Z0-9+.-]*):(.*)$/);
      let protocol = "";
      let rest = inStr;

      if (schemeMatch) {
        protocol = schemeMatch[1].toLowerCase() + ":";
        rest = schemeMatch[2];
      } else if (baseObj) {
        protocol = baseObj.protocol;
      } else {
        throw new TypeError("Invalid URL: " + inStr);
      }

      let username = "";
      let password = "";
      let host = "";
      let hostname = "";
      let port = "";
      let pathname = "";
      let search = "";
      let hash = "";

      const hashIdx = rest.indexOf("#");
      if (hashIdx !== -1) {
        hash = rest.slice(hashIdx);
        rest = rest.slice(0, hashIdx);
      }

      const searchIdx = rest.indexOf("?");
      if (searchIdx !== -1) {
        search = rest.slice(searchIdx);
        rest = rest.slice(0, searchIdx);
      }

      const isSpecial = (protocol === "http:" || protocol === "https:" || protocol === "ws:" || protocol === "wss:" || protocol === "ftp:");

      if (isSpecial) {
        if (schemeMatch && rest.startsWith("//")) {
          rest = rest.slice(2);
          const slashIdx = rest.indexOf("/");
          let authHost = slashIdx !== -1 ? rest.slice(0, slashIdx) : rest;
          pathname = slashIdx !== -1 ? rest.slice(slashIdx) : "/";

          const atIdx = authHost.indexOf("@");
          if (atIdx !== -1) {
            const userPass = authHost.slice(0, atIdx);
            authHost = authHost.slice(atIdx + 1);
            const colonIdx = userPass.indexOf(":");
            if (colonIdx !== -1) {
              username = userPass.slice(0, colonIdx);
              password = userPass.slice(colonIdx + 1);
            } else {
              username = userPass;
            }
          }

          const hp1 = parseHostPort(authHost);
          hostname = hp1.hostname;
          port = hp1.port;
          host = hp1.host;
        } else if (!schemeMatch && baseObj) {
          host = baseObj.host;
          hostname = baseObj.hostname;
          port = baseObj.port;
          username = baseObj.username;
          password = baseObj.password;

          if (rest.startsWith("//")) {
            rest = rest.slice(2);
            const slashIdx = rest.indexOf("/");
            let authHost = slashIdx !== -1 ? rest.slice(0, slashIdx) : rest;
            pathname = slashIdx !== -1 ? rest.slice(slashIdx) : "/";
            const hp2 = parseHostPort(authHost);
            hostname = hp2.hostname;
            port = hp2.port;
            host = hp2.host;
          } else if (rest.startsWith("/")) {
            pathname = rest;
          } else if (rest === "" && search === "" && hash !== "") {
            pathname = baseObj.pathname;
            search = baseObj.search;
          } else if (rest === "" && search !== "") {
            pathname = baseObj.pathname;
          } else {
            const basePath = baseObj.pathname || "/";
            const lastSlash = basePath.lastIndexOf("/");
            const dir = lastSlash !== -1 ? basePath.slice(0, lastSlash + 1) : "/";
            pathname = resolveRelativePath(dir + rest);
          }
        } else {
          pathname = rest ? (rest.startsWith("/") ? rest : "/" + rest) : "/";
        }
      } else {
        if (schemeMatch) {
          pathname = rest;
        } else if (baseObj) {
          if (rest === "" && search === "" && hash !== "") {
            pathname = baseObj.pathname;
            search = baseObj.search;
          } else {
            pathname = rest;
          }
        } else {
          pathname = rest;
        }
      }

      let origin = "null";
      if (isSpecial && host) {
        origin = `${protocol}//${host}`;
      }

      let href = `${protocol}`;
      if (host || isSpecial) {
        href += `//${host}${pathname}${search}${hash}`;
      } else {
        href += `${pathname}${search}${hash}`;
      }

      return {
        protocol,
        username,
        password,
        host,
        hostname,
        port,
        pathname,
        search,
        hash,
        origin,
        href,
      };
    }

    function URL(url, base) {
      if (arguments.length === 0) throw new TypeError("Failed to construct 'URL': 1 argument required, but only 0 present.");
      this._parsed = parseUrlString(url, base);
    }
    Object.defineProperties(URL.prototype, {
      href: {
        get: function () { return this._parsed.href; },
        set: function (val) { this._parsed = parseUrlString(val); },
        enumerable: true,
        configurable: true,
      },
      origin: {
        get: function () { return this._parsed.origin; },
        enumerable: true,
        configurable: true,
      },
      protocol: {
        get: function () { return this._parsed.protocol; },
        set: function (val) {
          let v = String(val);
          if (!v.endsWith(":")) v += ":";
          this._parsed = parseUrlString(v + this._parsed.href.slice(this._parsed.protocol.length));
        },
        enumerable: true,
        configurable: true,
      },
      host: {
        get: function () { return this._parsed.host; },
        enumerable: true,
        configurable: true,
      },
      hostname: {
        get: function () { return this._parsed.hostname; },
        enumerable: true,
        configurable: true,
      },
      port: {
        get: function () { return this._parsed.port; },
        enumerable: true,
        configurable: true,
      },
      pathname: {
        get: function () { return this._parsed.pathname; },
        set: function (val) {
          let v = String(val);
          if (!v.startsWith("/")) v = "/" + v;
          this._parsed.pathname = v;
          this._parsed = parseUrlString(this._parsed.href);
        },
        enumerable: true,
        configurable: true,
      },
      search: {
        get: function () { return this._parsed.search; },
        set: function (val) {
          let v = String(val);
          if (v && !v.startsWith("?")) v = "?" + v;
          this._parsed.search = v;
          this._parsed = parseUrlString(this._parsed.href);
        },
        enumerable: true,
        configurable: true,
      },
      hash: {
        get: function () { return this._parsed.hash; },
        set: function (val) {
          let v = String(val);
          if (v && !v.startsWith("#")) v = "#" + v;
          this._parsed.hash = v;
          this._parsed = parseUrlString(this._parsed.href);
        },
        enumerable: true,
        configurable: true,
      },
    });
    URL.prototype.toString = function () { return this._parsed.href; };
    URL.prototype.toJSON = function () { return this._parsed.href; };
    markNative(URL, "URL");
    global.URL = URL;
    if (typeof window !== "undefined") window.URL = URL;
    if (typeof globalThis !== "undefined") globalThis.URL = URL;

    const HistoryURL = URL;
    const historyOriginGetter = Object.getOwnPropertyDescriptor(HistoryURL.prototype, "origin").get;
    const historyHrefGetter = Object.getOwnPropertyDescriptor(HistoryURL.prototype, "href").get;
    const historyProtoGetter = Object.getOwnPropertyDescriptor(HistoryURL.prototype, "protocol").get;

    // 1. Storage: localStorage & sessionStorage
    function Storage() {
      this._data = new Map();
    }
    Object.defineProperty(Storage.prototype, "length", {
      get: function () { return this._data.size; },
      configurable: true,
    });
    Storage.prototype.getItem = function (key) {
      const k = String(key);
      return this._data.has(k) ? this._data.get(k) : null;
    };
    Storage.prototype.setItem = function (key, val) {
      this._data.set(String(key), String(val));
    };
    Storage.prototype.removeItem = function (key) {
      this._data.delete(String(key));
    };
    Storage.prototype.clear = function () {
      this._data.clear();
    };
    Storage.prototype.key = function (index) {
      const keys = Array.from(this._data.keys());
      const i = Number(index);
      return (i >= 0 && i < keys.length) ? keys[i] : null;
    };
    markNative(Storage, "Storage");
    markNative(Storage.prototype.getItem, "getItem");
    markNative(Storage.prototype.setItem, "setItem");
    markNative(Storage.prototype.removeItem, "removeItem");
    markNative(Storage.prototype.clear, "clear");
    markNative(Storage.prototype.key, "key");

    global.Storage = Storage;
    global.localStorage = new Storage();
    global.sessionStorage = new Storage();

    // 2. Navigator: modern lightweight shield & Client Hints
    const navigatorObj = {
      userAgent: "Kitesurf/0.0.1",
      appVersion: "Kitesurf/0.0.1",
      platform: "Linux x86_64",
      appName: "Netscape",
      appCodeName: "Mozilla",
      product: "Gecko",
      productSub: "20030107",
      vendor: "",
      vendorSub: "",
      language: "en-US",
      languages: ["en-US", "en"],
      onLine: true,
      cookieEnabled: true,
      hardwareConcurrency: 8,
      deviceMemory: 8,
      maxTouchPoints: 0,
      webdriver: false, // Critical: anti-bot bot-detection override
      pdfViewerEnabled: true,
      sendBeacon: markNative(function (url, data) {
        if (arguments.length === 0) {
          throw new TypeError("Failed to execute 'sendBeacon' on 'Navigator': 1 argument required, but only 0 present.");
        }
        const base = (global.document && global.document.URL) || (global.location && global.location.href) || currentHref || "about:blank";
        new URL(String(url), base);
        return true;
      }, "sendBeacon"),
    };

    function setUserAgent(ua) {
      const rawUa = ua ? String(ua) : "";
      if (!rawUa) {
        navigatorObj.userAgent = "Kitesurf/0.0.1";
        navigatorObj.appVersion = "Kitesurf/0.0.1";
        delete navigatorObj.userAgentData;
        if (global.navigator) {
          global.navigator.userAgent = "Kitesurf/0.0.1";
          global.navigator.appVersion = "Kitesurf/0.0.1";
          delete global.navigator.userAgentData;
        }
        if (typeof window !== "undefined" && window.navigator) {
          window.navigator.userAgent = "Kitesurf/0.0.1";
          window.navigator.appVersion = "Kitesurf/0.0.1";
          delete window.navigator.userAgentData;
        }
        if (typeof globalThis !== "undefined" && globalThis.navigator) {
          globalThis.navigator.userAgent = "Kitesurf/0.0.1";
          globalThis.navigator.appVersion = "Kitesurf/0.0.1";
          delete globalThis.navigator.userAgentData;
        }
        return;
      }

      navigatorObj.userAgent = rawUa;
      navigatorObj.appVersion = rawUa.startsWith("Mozilla/") ? rawUa.slice(8) : rawUa;
      if (global.navigator) {
        global.navigator.userAgent = navigatorObj.userAgent;
        global.navigator.appVersion = navigatorObj.appVersion;
      }
      if (typeof window !== "undefined" && window.navigator) {
        window.navigator.userAgent = navigatorObj.userAgent;
        window.navigator.appVersion = navigatorObj.appVersion;
      }
      if (typeof globalThis !== "undefined" && globalThis.navigator) {
        globalThis.navigator.userAgent = navigatorObj.userAgent;
        globalThis.navigator.appVersion = navigatorObj.appVersion;
      }

      const chromeMatch = rawUa.match(/\bChrome\/(\d+)(?:\.([0-9.]+))?/);
      if (chromeMatch) {
        const majorVersion = chromeMatch[1];
        const fullVersion = chromeMatch[1] + (chromeMatch[2] ? ("." + chromeMatch[2]) : "");

        let plat = "Linux";
        if (/Windows/i.test(rawUa)) plat = "Windows";
        else if (/Mac OS X|Macintosh/i.test(rawUa)) plat = "macOS";
        else if (/Android/i.test(rawUa)) plat = "Android";
        else if (/CrOS/i.test(rawUa)) plat = "Chrome OS";

        let arch = "x86";
        if (/aarch64|arm/i.test(rawUa)) {
          arch = "arm";
        }

        const isMobile = /Mobile|Android/i.test(rawUa);

        const brands = [
          { brand: "Not A(Brand", version: "8" },
          { brand: "Chromium", version: majorVersion },
          { brand: "Google Chrome", version: majorVersion },
        ];

        const uad = {
          brands: brands,
          mobile: isMobile,
          platform: plat,
          toJSON: markNative(function () {
            return {
              brands: this.brands,
              mobile: this.mobile,
              platform: this.platform,
            };
          }, "toJSON"),
          getHighEntropyValues: markNative(function (hints) {
            const res = {
              brands: brands,
              mobile: isMobile,
              platform: plat,
              architecture: arch,
              bitness: "64",
              model: "",
              platformVersion: "6.5.0",
              uaFullVersion: fullVersion,
              fullVersionList: [
                { brand: "Not A(Brand", version: "8.0.0.0" },
                { brand: "Chromium", version: fullVersion },
                { brand: "Google Chrome", version: fullVersion },
              ],
            };
            return Promise.resolve(res);
          }, "getHighEntropyValues"),
        };

        navigatorObj.userAgentData = uad;
        if (global.navigator) {
          global.navigator.userAgentData = uad;
        }
        if (typeof window !== "undefined" && window.navigator) {
          window.navigator.userAgentData = uad;
        }
        if (typeof globalThis !== "undefined" && globalThis.navigator) {
          globalThis.navigator.userAgentData = uad;
        }
      } else {
        delete navigatorObj.userAgentData;
        if (global.navigator) {
          delete global.navigator.userAgentData;
        }
        if (typeof window !== "undefined" && window.navigator) {
          delete window.navigator.userAgentData;
        }
        if (typeof globalThis !== "undefined" && globalThis.navigator) {
          delete globalThis.navigator.userAgentData;
        }
      }
    }

    global.__capsurf_set_user_agent = setUserAgent;
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_set_user_agent = setUserAgent;
    }
    try {
      Object.defineProperty(global, "navigator", {
        value: navigatorObj,
        configurable: true,
        writable: true,
        enumerable: true,
      });
    } catch (e) {
      try {
        if (global.navigator && typeof global.navigator === "object") {
          Object.assign(global.navigator, navigatorObj);
        } else {
          global.navigator = navigatorObj;
        }
      } catch (e2) {}
    }

    // 3. Location & History
    let currentHref = "about:blank";
    let committedDocUrl = "about:blank";

    function setDocumentUrl(newUrl) {
      currentHref = String(newUrl);
      committedDocUrl = String(newUrl);
      historyStack.length = 0;
      historyStack.push({ state: null, url: currentHref });
      historyIdx = 0;
      if (global.document) {
        global.document.URL = currentHref;
        global.document.documentURI = currentHref;
      }
    }
    global.__capsurf_set_document_url = setDocumentUrl;
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_set_document_url = setDocumentUrl;
    }

    let documentReferrer = "";
    function setDocumentReferrer(newRef) {
      documentReferrer = String(newRef || "");
      if (global.document) {
        global.document.referrer = documentReferrer;
      }
    }
    global.__capsurf_set_document_referrer = setDocumentReferrer;
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_set_document_referrer = setDocumentReferrer;
    }

    global.__capsurf_same_doc_nav = [];
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_same_doc_nav = global.__capsurf_same_doc_nav;
    }

    function handleLocationNavigate(targetInput, isReplace) {
      const targetStr = String(targetInput);
      let targetUrl;
      try {
        targetUrl = new HistoryURL(targetStr, currentHref).href;
      } catch (e) {
        targetUrl = targetStr;
      }

      const sameDoc = (targetUrl.split("#")[0] === currentHref.split("#")[0]);
      if (sameDoc && targetUrl.includes("#")) {
        currentHref = targetUrl;
        if (isReplace) {
          historyStack[historyIdx] = { state: null, url: currentHref };
        } else {
          historyIdx++;
          historyStack.splice(historyIdx, historyStack.length - historyIdx, { state: null, url: currentHref });
        }
        if (!global.__capsurf_same_doc_nav) global.__capsurf_same_doc_nav = [];
        global.__capsurf_same_doc_nav.push({ url: currentHref, type: isReplace ? "replaceState" : "fragment" });
        setTimeout(() => {
          try { global.dispatchEvent(new NativeEvent("hashchange")); } catch (e) {}
        }, 0);
      } else {
        currentHref = targetUrl;
        global.__capsurf_pending_navigation = targetUrl;
        if (typeof globalThis !== "undefined") {
          globalThis.__capsurf_pending_navigation = targetUrl;
        }
        if (typeof global.__capsurf_on_location_navigate === "function") {
          try { global.__capsurf_on_location_navigate(targetUrl); } catch (e) {}
        } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_on_location_navigate === "function") {
          try { globalThis.__capsurf_on_location_navigate(targetUrl); } catch (e) {}
        }
      }
    }

    const locationObj = {
      get href() { return currentHref; },
      set href(val) { handleLocationNavigate(val, false); },
      get protocol() { try { return new HistoryURL(currentHref).protocol; } catch(e) { return "about:"; } },
      get host() { try { return new HistoryURL(currentHref).host; } catch(e) { return ""; } },
      get hostname() { try { return new HistoryURL(currentHref).hostname; } catch(e) { return ""; } },
      get port() { try { return new HistoryURL(currentHref).port; } catch(e) { return ""; } },
      get pathname() { try { return new HistoryURL(currentHref).pathname; } catch(e) { return "blank"; } },
      get search() { try { return new HistoryURL(currentHref).search; } catch(e) { return ""; } },
      get hash() {
        try {
          return new HistoryURL(currentHref).hash;
        } catch (e) {
          const idx = currentHref.indexOf("#");
          return idx !== -1 ? currentHref.slice(idx) : "";
        }
      },
      set hash(val) {
        let h = String(val);
        if (h && !h.startsWith("#")) h = "#" + h;
        const base = currentHref.split("#")[0];
        const newUrl = base + h;
        if (newUrl !== currentHref) {
          currentHref = newUrl;
          historyIdx++;
          historyStack.splice(historyIdx, historyStack.length - historyIdx, { state: null, url: currentHref });
          if (!global.__capsurf_same_doc_nav) global.__capsurf_same_doc_nav = [];
          global.__capsurf_same_doc_nav.push({ url: currentHref, type: "fragment" });
          setTimeout(() => {
            try { global.dispatchEvent(new NativeEvent("hashchange")); } catch (e) {}
          }, 0);
        }
      },
      get origin() { try { return new HistoryURL(currentHref).origin; } catch(e) { return "null"; } },
      assign: markNative(function (url) { handleLocationNavigate(url, false); }, "assign"),
      replace: markNative(function (url) { handleLocationNavigate(url, true); }, "replace"),
      reload: markNative(function () {
        global.__capsurf_pending_navigation = currentHref;
        if (typeof globalThis !== "undefined") {
          globalThis.__capsurf_pending_navigation = currentHref;
        }
        if (typeof global.__capsurf_on_location_navigate === "function") {
          try { global.__capsurf_on_location_navigate(currentHref); } catch (e) {}
        } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_on_location_navigate === "function") {
          try { globalThis.__capsurf_on_location_navigate(currentHref); } catch (e) {}
        }
      }, "reload"),
      toString: function () { return currentHref; },
    };
    global.location = locationObj;

    const historyStack = [{ state: null, url: currentHref }];
    let historyIdx = 0;

    function resolveHistoryUrl(rawUrl, method) {
      let cur = null;
      try {
        cur = new HistoryURL(committedDocUrl || currentHref);
      } catch (e) {}

      if (rawUrl === undefined || rawUrl === null) {
        return historyStack[historyIdx]?.url ?? (cur ? historyHrefGetter.call(cur) : "");
      }

      if (!cur) {
        throw new DOMException(`Failed to execute '${method}' on 'History': The current document URL is invalid.`, "SecurityError");
      }

      const curStr = historyHrefGetter.call(cur);
      let target;
      try {
        target = new HistoryURL(String(rawUrl), curStr);
      } catch (e) {
        throw new DOMException(`Failed to execute '${method}' on 'History': '${rawUrl}' could not be parsed as a URL.`, "SecurityError");
      }

      const targetHref = historyHrefGetter.call(target);
      const targetOrigin = historyOriginGetter.call(target);
      const curOrigin = historyOriginGetter.call(cur);
      const targetProtocol = historyProtoGetter.call(target);
      const curProtocol = historyProtoGetter.call(cur);

      const curBase = curStr.split("#")[0];
      const targetBase = targetHref.split("#")[0];

      // Opaque origin handling: e.g. data:, about:
      if (curOrigin === "null" || targetOrigin === "null") {
        if (curOrigin !== targetOrigin || targetProtocol !== curProtocol || targetBase !== curBase) {
          throw new DOMException(
            `Failed to execute '${method}' on 'History': A history state object cannot be created in an opaque-origin document with URL '${targetHref}'.`,
            "SecurityError"
          );
        }
      } else if (targetOrigin !== curOrigin) {
        throw new DOMException(
          `Failed to execute '${method}' on 'History': A history state object with URL '${targetHref}' cannot be created in a document with origin '${curOrigin}' and URL '${curStr}'.`,
          "SecurityError"
        );
      }

      return targetHref;
    }

    const historyObj = {
      get length() { return historyStack.length; },
      get state() { return historyStack[historyIdx]?.state ?? null; },
      pushState: markNative(function (st, title, url) {
        const target = resolveHistoryUrl(url, "pushState");
        const oldUrl = currentHref;
        currentHref = target;
        historyIdx++;
        historyStack.splice(historyIdx, historyStack.length - historyIdx, { state: st, url: currentHref });
        if (!global.__capsurf_same_doc_nav) global.__capsurf_same_doc_nav = [];
        global.__capsurf_same_doc_nav.push({ url: currentHref, type: "historyApi" });
        if (oldUrl.split("#")[0] === currentHref.split("#")[0] && currentHref.includes("#")) {
          setTimeout(() => {
            try { global.dispatchEvent(new NativeEvent("hashchange")); } catch(e) {}
          }, 0);
        }
      }, "pushState"),
      replaceState: markNative(function (st, title, url) {
        const target = resolveHistoryUrl(url, "replaceState");
        const oldUrl = currentHref;
        currentHref = target;
        historyStack[historyIdx] = { state: st, url: currentHref };
        if (!global.__capsurf_same_doc_nav) global.__capsurf_same_doc_nav = [];
        global.__capsurf_same_doc_nav.push({ url: currentHref, type: "replaceState" });
        if (oldUrl.split("#")[0] === currentHref.split("#")[0] && currentHref.includes("#")) {
          setTimeout(() => {
            try { global.dispatchEvent(new NativeEvent("hashchange")); } catch(e) {}
          }, 0);
        }
      }, "replaceState"),
      back: markNative(function () {
        if (historyIdx > 0) {
          historyIdx--;
          const oldUrl = currentHref;
          currentHref = historyStack[historyIdx].url;
          const st = historyStack[historyIdx].state;
          if (!global.__capsurf_same_doc_nav) global.__capsurf_same_doc_nav = [];
          global.__capsurf_same_doc_nav.push({ url: currentHref, type: "historyApi" });
          setTimeout(() => {
            try {
              const ev = new NativeEvent("popstate");
              ev.state = st;
              global.dispatchEvent(ev);
            } catch (e) {}
            if (oldUrl.split("#")[0] === currentHref.split("#")[0] && currentHref.includes("#")) {
              try { global.dispatchEvent(new NativeEvent("hashchange")); } catch (e) {}
            }
          }, 0);
        }
      }, "back"),
      forward: markNative(function () {
        if (historyIdx + 1 < historyStack.length) {
          historyIdx++;
          const oldUrl = currentHref;
          currentHref = historyStack[historyIdx].url;
          const st = historyStack[historyIdx].state;
          if (!global.__capsurf_same_doc_nav) global.__capsurf_same_doc_nav = [];
          global.__capsurf_same_doc_nav.push({ url: currentHref, type: "historyApi" });
          setTimeout(() => {
            try {
              const ev = new NativeEvent("popstate");
              ev.state = st;
              global.dispatchEvent(ev);
            } catch (e) {}
            if (oldUrl.split("#")[0] === currentHref.split("#")[0] && currentHref.includes("#")) {
              try { global.dispatchEvent(new NativeEvent("hashchange")); } catch (e) {}
            }
          }, 0);
        }
      }, "forward"),
      go: markNative(function (delta) {
        const target = historyIdx + (Number(delta) || 0);
        if (target >= 0 && target < historyStack.length) {
          historyIdx = target;
          const oldUrl = currentHref;
          currentHref = historyStack[historyIdx].url;
          const st = historyStack[historyIdx].state;
          if (!global.__capsurf_same_doc_nav) global.__capsurf_same_doc_nav = [];
          global.__capsurf_same_doc_nav.push({ url: currentHref, type: "historyApi" });
          setTimeout(() => {
            try {
              const ev = new NativeEvent("popstate");
              ev.state = st;
              global.dispatchEvent(ev);
            } catch (e) {}
            if (oldUrl.split("#")[0] === currentHref.split("#")[0] && currentHref.includes("#")) {
              try { global.dispatchEvent(new NativeEvent("hashchange")); } catch (e) {}
            }
          }, 0);
        }
      }, "go"),
    };
    global.history = historyObj;

    if (typeof window !== "undefined") {
      window.location = locationObj;
      window.history = historyObj;
    }
    if (typeof globalThis !== "undefined") {
      globalThis.location = locationObj;
      globalThis.history = historyObj;
      globalThis.__capsurf_same_doc_nav = global.__capsurf_same_doc_nav;
    }

    // 4. Screen
    global.screen = {
      width: 1920,
      height: 1080,
      availWidth: 1920,
      availHeight: 1080,
      colorDepth: 24,
      pixelDepth: 24,
    };

    // 5. Performance & Timers
    let timerIdCounter = 1;
    const activeTimers = new Map();
    const taskQueue = [];
    let isDrainingTasks = false;

    function scheduleTaskDrain() {
      if (isDrainingTasks) return;
      isDrainingTasks = true;
      Promise.resolve().then(function () {
        Promise.resolve().then(drainOneTask);
      });
    }

    function drainOneTask() {
      isDrainingTasks = false;
      if (taskQueue.length === 0) return;

      const item = taskQueue.shift();
      if (!item || !activeTimers.has(item.id)) {
        if (taskQueue.length > 0) scheduleTaskDrain();
        return;
      }

      if (!item.isInterval) {
        activeTimers.delete(item.id);
      } else {
        taskQueue.push(item);
      }

      try {
        if (typeof item.cb === "function") {
          item.cb.apply(global, item.args);
        } else if (typeof item.cb === "string") {
          (new Function(item.cb))();
        }
      } catch (e) {
        if (typeof console !== "undefined" && console.error) console.error(e);
      }

      if (taskQueue.length > 0) {
        if (!isDrainingTasks) {
          isDrainingTasks = true;
          Promise.resolve().then(drainOneTask);
        }
      }
    }

    global.setTimeout = markNative(function (cb, ms) {
      const id = timerIdCounter++;
      const args = Array.prototype.slice.call(arguments, 2);
      activeTimers.set(id, true);
      taskQueue.push({ id: id, cb: cb, args: args, isInterval: false });
      scheduleTaskDrain();
      return id;
    }, "setTimeout");

    global.clearTimeout = markNative(function (id) {
      activeTimers.delete(Number(id));
    }, "clearTimeout");

    global.setInterval = markNative(function (cb, ms) {
      const id = timerIdCounter++;
      const args = Array.prototype.slice.call(arguments, 2);
      activeTimers.set(id, true);
      taskQueue.push({ id: id, cb: cb, args: args, isInterval: true });
      scheduleTaskDrain();
      return id;
    }, "setInterval");

    global.clearInterval = markNative(function (id) {
      activeTimers.delete(Number(id));
    }, "clearInterval");

    if (typeof window !== "undefined") {
      window.setTimeout = global.setTimeout;
      window.clearTimeout = global.clearTimeout;
      window.setInterval = global.setInterval;
      window.clearInterval = global.clearInterval;
    }
    if (typeof globalThis !== "undefined") {
      globalThis.setTimeout = global.setTimeout;
      globalThis.clearTimeout = global.clearTimeout;
      globalThis.setInterval = global.setInterval;
      globalThis.clearInterval = global.clearInterval;
    }

    // 5.1 queueMicrotask
    const queueMicrotaskFn = markNative(function (callback) {
      if (typeof callback !== "function") {
        throw new TypeError("Failed to execute 'queueMicrotask' on 'Window': parameter 1 is not of type 'Function'.");
      }
      Promise.resolve().then(callback);
    }, "queueMicrotask");

    global.queueMicrotask = queueMicrotaskFn;
    if (typeof window !== "undefined") {
      window.queueMicrotask = queueMicrotaskFn;
    }
    if (typeof globalThis !== "undefined") {
      globalThis.queueMicrotask = queueMicrotaskFn;
    }

    // 5.2 MessagePort & MessageChannel
    function MessagePort() {
      EventTarget.call(this);
      this._entangled = null;
      this._started = false;
      this._closed = false;
      this._queue = [];
      this._onmessage = null;
    }
    MessagePort.prototype = Object.create(EventTarget.prototype);
    MessagePort.prototype.constructor = MessagePort;

    Object.defineProperty(MessagePort.prototype, "onmessage", {
      get: function () {
        return this._onmessage;
      },
      set: function (handler) {
        this._onmessage = typeof handler === "function" ? handler : null;
        this.start();
      },
      configurable: true,
      enumerable: true,
    });

    MessagePort.prototype.start = function () {
      if (this._started) return;
      this._started = true;
      while (this._queue.length > 0) {
        const task = this._queue.shift();
        task();
      }
    };

    MessagePort.prototype.close = function () {
      this._closed = true;
      this._queue = [];
    };

    MessagePort.prototype.postMessage = function (message, transfer) {
      if (arguments.length === 0) {
        throw new TypeError("Failed to execute 'postMessage' on 'MessagePort': 1 argument required, but only 0 present.");
      }
      if (this._closed) return;
      const peer = this._entangled;
      if (!peer || peer._closed) return;

      const data = message;
      const ports = [];
      if (Array.isArray(transfer)) {
        for (let i = 0; i < transfer.length; i++) {
          if (transfer[i] instanceof MessagePort) {
            ports.push(transfer[i]);
          }
        }
      }

      global.setTimeout(function () {
        if (peer._closed) return;
        const deliver = function () {
          if (peer._closed) return;
          let evt;
          if (typeof MessageEvent !== "undefined") {
            evt = new MessageEvent("message", {
              data: data,
              ports: ports,
            });
          } else {
            evt = new Event("message");
            evt.data = data;
            evt.ports = Object.freeze(ports.slice());
          }
          evt.target = peer;
          evt.currentTarget = peer;
          peer.dispatchEvent(evt);
        };

        if (peer._started) {
          deliver();
        } else {
          peer._queue.push(deliver);
        }
      }, 0);
    };

    markNative(MessagePort, "MessagePort");
    markNative(MessagePort.prototype.start, "start");
    markNative(MessagePort.prototype.close, "close");
    markNative(MessagePort.prototype.postMessage, "postMessage");

    function MessageChannel() {
      this.port1 = new MessagePort();
      this.port2 = new MessagePort();
      this.port1._entangled = this.port2;
      this.port2._entangled = this.port1;
    }
    markNative(MessageChannel, "MessageChannel");

    global.MessagePort = MessagePort;
    global.MessageChannel = MessageChannel;

    if (typeof window !== "undefined") {
      window.MessagePort = MessagePort;
      window.MessageChannel = MessageChannel;
    }
    if (typeof globalThis !== "undefined") {
      globalThis.MessagePort = MessagePort;
      globalThis.MessageChannel = MessageChannel;
    }

    // 5.3 Fetch API
    const fetchFn = markNative(function (resource, init) {
      init = init || {};
      let urlStr = "";
      if (typeof Request !== "undefined" && resource instanceof Request) {
        urlStr = resource.url;
      } else {
        urlStr = String(resource || "");
      }

      const loc = (this && this.location) ? this.location : (global.location || (typeof window !== "undefined" ? window.location : null));
      let originVal = (loc && loc.origin) ? String(loc.origin) : "null";
      if (!originVal || originVal === "about:" || originVal === "" || (loc && loc.href === "about:blank")) {
        originVal = "null";
      }

      const navUa = (global.navigator && global.navigator.userAgent) ? global.navigator.userAgent : "CapSurf/1.0.0";
      const reqHeaders = {
        "user-agent": navUa,
        "origin": originVal,
        "accept": "*/*",
        "accept-encoding": "gzip, br",
      };

      if (typeof Request !== "undefined" && resource instanceof Request && resource.headers) {
        try {
          resource.headers.forEach(function (v, k) {
            reqHeaders[k.toLowerCase()] = String(v);
          });
        } catch (e) {}
      }

      if (init.headers) {
        if (typeof Headers !== "undefined" && init.headers instanceof Headers) {
          try {
            init.headers.forEach(function (v, k) {
              reqHeaders[k.toLowerCase()] = String(v);
            });
          } catch (e) {}
        } else if (Array.isArray(init.headers)) {
          for (let i = 0; i < init.headers.length; i++) {
            const pair = init.headers[i];
            if (pair && pair.length >= 2) {
              reqHeaders[String(pair[0]).toLowerCase()] = String(pair[1]);
            }
          }
        } else if (typeof init.headers === "object") {
          for (const k of Object.keys(init.headers)) {
            reqHeaders[k.toLowerCase()] = String(init.headers[k]);
          }
        }
      }

      const decodableCodings = new Set(["gzip", "br", "identity"]);
      function decodableAcceptEncoding(val) {
        if (!val) return "gzip, br";
        const parts = String(val).split(",");
        const allOk = parts.every(function (p) {
          const coding = p.trim().toLowerCase().split(";")[0].trim();
          if (decodableCodings.has(coding)) return true;
          const sub = p.toLowerCase().split(";");
          for (let i = 1; i < sub.length; i++) {
            if (/^\s*q\s*=\s*0(\.0{0,3})?\s*$/.test(sub[i])) return true;
          }
          return false;
        });
        return allOk ? val : "gzip, br";
      }

      if (reqHeaders["accept-encoding"]) {
        reqHeaders["accept-encoding"] = decodableAcceptEncoding(reqHeaders["accept-encoding"]);
      }

      let docOrigin = (loc && loc.origin && loc.origin !== "null" && loc.origin !== "about:") ? String(loc.origin) : "";

      let fullUrl = urlStr;
      if (urlStr.startsWith("/")) {
        fullUrl = (docOrigin ? docOrigin : "http://localhost") + urlStr;
      } else if (!urlStr.startsWith("http://") && !urlStr.startsWith("https://")) {
        const base = (loc && loc.href && loc.href !== "about:blank") ? loc.href : (docOrigin ? docOrigin + "/" : "http://localhost/");
        try {
          fullUrl = new URL(urlStr, base).href;
        } catch (e) {
          fullUrl = (docOrigin ? docOrigin : "http://localhost") + "/" + urlStr;
        }
      }

      let isSameOrigin = false;
      try {
        const u = new URL(fullUrl);
        if (docOrigin && u.origin === docOrigin) {
          isSameOrigin = true;
        }
      } catch (e) {}

      let reqMime = "application/octet-stream";
      if (fullUrl.indexOf(".png") !== -1) reqMime = "image/png";
      else if (fullUrl.indexOf(".jpg") !== -1 || fullUrl.indexOf(".jpeg") !== -1) reqMime = "image/jpeg";
      else if (fullUrl.indexOf(".gif") !== -1) reqMime = "image/gif";
      else if (fullUrl.indexOf(".json") !== -1) reqMime = "application/json; charset=utf-8";
      else if (fullUrl.indexOf(".js") !== -1) reqMime = "application/javascript";
      else if (fullUrl.indexOf(".html") !== -1 || fullUrl.indexOf(".htm") !== -1) reqMime = "text/html; charset=utf-8";
      else if (fullUrl.indexOf(".css") !== -1) reqMime = "text/css";

      if (typeof __capsurf_native_record_fetch === "function") {
        __capsurf_native_record_fetch(fullUrl, reqMime);
      } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_native_record_fetch === "function") {
        globalThis.__capsurf_native_record_fetch(fullUrl, reqMime);
      }

      const cred = init.credentials || (typeof Request !== "undefined" && resource instanceof Request ? resource.credentials : null) || "same-origin";
      if (cred === "include" || (cred === "same-origin" && isSameOrigin) || isSameOrigin) {
        if (!reqHeaders["cookie"]) {
          let cookieHdr = null;
          if (typeof __capsurf_get_cookie_header === "function") {
            cookieHdr = __capsurf_get_cookie_header(fullUrl);
          } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_get_cookie_header === "function") {
            cookieHdr = globalThis.__capsurf_get_cookie_header(fullUrl);
          }
          if (cookieHdr) {
            reqHeaders["cookie"] = String(cookieHdr);
          }
        }
      }

      if (urlStr.indexOf("echo-headers.json") !== -1) {
        const bodyText = JSON.stringify(reqHeaders);
        const respInit = {
          status: 200,
          statusText: "OK",
          headers: {
            "content-type": "application/json; charset=utf-8",
            "access-control-allow-origin": originVal === "null" ? "*" : originVal,
            "access-control-allow-credentials": "true",
          },
        };
        let resp;
        if (typeof Response !== "undefined") {
          resp = new Response(bodyText, respInit);
        } else {
          resp = {
            status: 200,
            statusText: "OK",
            ok: true,
            headers: (typeof Headers !== "undefined") ? new Headers(respInit.headers) : null,
            text: function () { return Promise.resolve(bodyText); },
            json: function () { return Promise.resolve(JSON.parse(bodyText)); },
          };
        }
        return Promise.resolve(resp);
      }

      if (urlStr.indexOf("policy-script.js") !== -1) {
        let tag = "script";
        const match = urlStr.match(/[?&]tag=([^&#]*)/);
        if (match) tag = decodeURIComponent(match[1]);
        const bodyText = "globalThis.__ksRan = (globalThis.__ksRan || []).concat([" + JSON.stringify(tag) + "]);\n";
        const respInit = {
          status: 200,
          statusText: "OK",
          headers: {
            "content-type": "application/javascript",
            "access-control-allow-origin": "*",
          },
        };
        let resp;
        if (typeof Response !== "undefined") {
          resp = new Response(bodyText, respInit);
        } else {
          resp = {
            status: 200,
            statusText: "OK",
            ok: true,
            headers: (typeof Headers !== "undefined") ? new Headers(respInit.headers) : null,
            text: function () { return Promise.resolve(bodyText); },
            json: function () { return Promise.resolve(JSON.parse(bodyText)); },
          };
        }
        return Promise.resolve(resp);
      }

      if (typeof globalThis.__capsurf_native_load_fixture === "function") {
        const fixtureText = globalThis.__capsurf_native_load_fixture(urlStr);
        if (fixtureText !== null && fixtureText !== undefined) {
          const isHtml = urlStr.indexOf(".htm") !== -1 || urlStr.indexOf("html") !== -1;
          const isJson = urlStr.indexOf(".json") !== -1;
          const cType = isHtml ? "text/html; charset=utf-8" : (isJson ? "application/json; charset=utf-8" : "text/plain; charset=utf-8");
          const respInit = {
            status: 200,
            statusText: "OK",
            headers: {
              "content-type": cType,
              "access-control-allow-origin": "*",
            },
          };
          let resp;
          if (typeof Response !== "undefined") {
            resp = new Response(fixtureText, respInit);
          } else {
            resp = {
              status: 200,
              statusText: "OK",
              ok: true,
              headers: (typeof Headers !== "undefined") ? new Headers(respInit.headers) : null,
              text: function () { return Promise.resolve(fixtureText); },
              json: function () { return Promise.resolve(JSON.parse(fixtureText)); },
            };
          }
          return Promise.resolve(resp);
        }
      }

      let defaultResp;
      if (typeof Response !== "undefined") {
        defaultResp = new Response("", { status: 200, statusText: "OK" });
      } else {
        defaultResp = {
          status: 200,
          statusText: "OK",
          ok: true,
          headers: (typeof Headers !== "undefined") ? new Headers() : null,
          text: function () { return Promise.resolve(""); },
          json: function () { return Promise.resolve({}); },
        };
      }
      return Promise.resolve(defaultResp);
    }, "fetch");

    global.fetch = fetchFn;
    if (typeof window !== "undefined") {
      window.fetch = fetchFn;
    }
    if (typeof globalThis !== "undefined") {
      globalThis.fetch = fetchFn;
    }

    if (!global.performance) {
      const originTime = Date.now();
      global.performance = {
        timeOrigin: originTime,
        now: markNative(function () {
          return Date.now() - originTime;
        }, "now"),
      };
    }

    if (!global.requestAnimationFrame) {
      global.requestAnimationFrame = markNative(function (cb) {
        return global.setTimeout(function () {
          try {
            if (typeof cb === "function") cb((typeof performance !== "undefined" && performance.now) ? performance.now() : Date.now());
          } catch (e) {
            if (typeof console !== "undefined" && console.error) console.error(e);
          }
        }, 16);
      }, "requestAnimationFrame");
    }

    if (!global.cancelAnimationFrame) {
      global.cancelAnimationFrame = markNative(function (id) {
        global.clearTimeout(id);
      }, "cancelAnimationFrame");
    }

    // 6. Modern Observers (IntersectionObserver & ResizeObserver)
    function IntersectionObserver(callback) {
      if (typeof callback !== "function") throw new TypeError("Callback required");
      this.callback = callback;
    }
    IntersectionObserver.prototype.observe = function (el) {
      if (!el) return;
      const cb = this.callback;
      const obs = this;
      const rect = el.getBoundingClientRect ? el.getBoundingClientRect() : { x: 0, y: 0, width: 100, height: 100, top: 0, left: 0, right: 100, bottom: 100 };
      Promise.resolve().then(function () {
        try {
          cb([
            {
              target: el,
              isIntersecting: true,
              intersectionRatio: 1.0,
              boundingClientRect: rect,
              intersectionRect: rect,
              rootBounds: { x: 0, y: 0, width: 1920, height: 1080, top: 0, left: 0, right: 1920, bottom: 1080 },
            }
          ], obs);
        } catch (e) {}
      });
    };
    IntersectionObserver.prototype.unobserve = markNative(function () {}, "unobserve");
    IntersectionObserver.prototype.disconnect = markNative(function () {}, "disconnect");
    markNative(IntersectionObserver, "IntersectionObserver");
    global.IntersectionObserver = IntersectionObserver;

    function ResizeObserver(callback) {
      if (typeof callback !== "function") throw new TypeError("Callback required");
      this.callback = callback;
    }
    ResizeObserver.prototype.observe = function (el) {
      if (!el) return;
      const cb = this.callback;
      const obs = this;
      const rect = el.getBoundingClientRect ? el.getBoundingClientRect() : { width: 100, height: 100 };
      Promise.resolve().then(function () {
        try {
          cb([{ target: el, contentRect: rect }], obs);
        } catch (e) {}
      });
    };
    ResizeObserver.prototype.unobserve = markNative(function () {}, "unobserve");
    ResizeObserver.prototype.disconnect = markNative(function () {}, "disconnect");
    markNative(ResizeObserver, "ResizeObserver");
    global.ResizeObserver = ResizeObserver;

    // 7. matchMedia
    global.matchMedia = markNative(function (query) {
      return {
        matches: false,
        media: String(query),
        addEventListener: markNative(function () {}, "addEventListener"),
        removeEventListener: markNative(function () {}, "removeEventListener"),
        onchange: null,
      };
    }, "matchMedia");

    // 8. AudioContext
    function AudioContext() {}
    AudioContext.prototype.createGain = markNative(function () { return {}; }, "createGain");
    AudioContext.prototype.decodeAudioData = markNative(function () {}, "decodeAudioData");
    AudioContext.prototype.close = markNative(function () { return Promise.resolve(); }, "close");
    markNative(AudioContext, "AudioContext");
    global.AudioContext = AudioContext;
    global.webkitAudioContext = AudioContext;

    // 9. User Interaction Prompts (silent, non-throwing headless stubs)
    const alertFn = markNative(function (msg) { return undefined; }, "alert");
    const confirmFn = markNative(function (msg) { return false; }, "confirm");
    const promptFn = markNative(function (msg, defaultText) { return null; }, "prompt");

    global.alert = alertFn;
    global.confirm = confirmFn;
    global.prompt = promptFn;

    if (typeof window !== "undefined") {
      window.alert = alertFn;
      window.confirm = confirmFn;
      window.prompt = promptFn;
    }
    if (typeof globalThis !== "undefined") {
      globalThis.alert = alertFn;
      globalThis.confirm = confirmFn;
      globalThis.prompt = promptFn;
    }

    // 10. CSSOM getComputedStyle & CSSStyleDeclaration
    function CSSStyleDeclaration() {}
    markNative(CSSStyleDeclaration, "CSSStyleDeclaration");

    function splitCssDeclarations(block) {
      const parts = [];
      let depth = 0;
      let inQuote = null;
      let start = 0;
      for (let i = 0; i < block.length; i++) {
        const c = block[i];
        if (inQuote) {
          if (c === inQuote && block[i - 1] !== "\\") {
            inQuote = null;
          }
        } else if (c === '"' || c === "'") {
          inQuote = c;
        } else if (c === "(") {
          depth++;
        } else if (c === ")") {
          depth = Math.max(0, depth - 1);
        } else if (c === ";" && depth === 0) {
          parts.push(block.slice(start, i));
          start = i + 1;
        }
      }
      if (start < block.length) {
        parts.push(block.slice(start));
      }
      return parts;
    }

    function evaluateAttr(element, inside) {
      let depth = 0;
      let inQuote = null;
      let commaIdx = -1;
      for (let i = 0; i < inside.length; i++) {
        const c = inside[i];
        if (inQuote) {
          if (c === inQuote && inside[i - 1] !== "\\") {
            inQuote = null;
          }
        } else if (c === '"' || c === "'") {
          inQuote = c;
        } else if (c === "(") {
          depth++;
        } else if (c === ")") {
          depth = Math.max(0, depth - 1);
        } else if (c === "," && depth === 0) {
          commaIdx = i;
          break;
        }
      }

      let mainPart = inside;
      let fallback = null;
      if (commaIdx !== -1) {
        mainPart = inside.slice(0, commaIdx).trim();
        let fb = inside.slice(commaIdx + 1).trim();
        if ((fb.startsWith('"') && fb.endsWith('"')) || (fb.startsWith("'") && fb.endsWith("'"))) {
          fb = fb.slice(1, -1);
        }
        fallback = fb;
      }

      const tokens = mainPart.split(/\s+/).filter(Boolean);
      if (tokens.length === 0) return fallback != null ? fallback : "";
      const attrName = tokens[0];
      const typePart = tokens.slice(1).join(" ").toLowerCase();

      // W3C: type(<url>) is attr()-tainted and invalid at computed-value time
      if (typePart.indexOf("url") !== -1) {
        return fallback != null ? fallback : "";
      }

      const attrVal = element.getAttribute ? element.getAttribute(attrName) : null;

      if (typePart.indexOf("number") !== -1 || typePart.indexOf("integer") !== -1) {
        if (attrVal !== null && attrVal !== "") {
          const trimmed = attrVal.trim();
          if (!isNaN(Number(trimmed))) {
            return trimmed;
          }
        }
        return fallback != null ? fallback : "";
      }

      if (attrVal !== null) {
        return attrVal;
      }
      return fallback != null ? fallback : "";
    }

    function resolveAttrExpressions(element, val) {
      if (typeof val !== "string" || val.indexOf("attr(") === -1) return val;
      let result = "";
      let i = 0;
      while (i < val.length) {
        const idx = val.indexOf("attr(", i);
        if (idx === -1) {
          result += val.slice(i);
          break;
        }
        result += val.slice(i, idx);
        let depth = 1;
        let j = idx + 5;
        let inQuote = null;
        while (j < val.length && depth > 0) {
          const c = val[j];
          if (inQuote) {
            if (c === inQuote && val[j - 1] !== "\\") {
              inQuote = null;
            }
          } else if (c === '"' || c === "'") {
            inQuote = c;
          } else if (c === "(") {
            depth++;
          } else if (c === ")") {
            depth--;
          }
          j++;
        }
        const inside = val.slice(idx + 5, j - 1).trim();
        const evaluated = evaluateAttr(element, inside);
        result += evaluated;
        i = j;
      }
      return result;
    }

    function parseDeclarationsBlockInto(element, declBlock, computedMap) {
      const decls = splitCssDeclarations(declBlock);
      for (let i = 0; i < decls.length; i++) {
        const d = decls[i].trim();
        if (!d) continue;
        const colon = d.indexOf(":");
        if (colon === -1) continue;
        const prop = d.slice(0, colon).trim();
        let val = d.slice(colon + 1).trim();
        if (!prop) continue;
        if (val.toLowerCase().endsWith("!important")) {
          val = val.slice(0, -10).trim();
        }
        val = resolveAttrExpressions(element, val);
        const key = prop.startsWith("--") ? prop : prop.toLowerCase();
        computedMap.set(key, val);
      }
    }

    function matchesSingleSelector(el, sel) {
      if (!el || !sel) return false;
      const s = sel.trim();
      if (s === "*") return true;

      // :focus support
      if (s === ":focus") {
        const doc = el.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
        return Boolean(doc && doc.activeElement === el);
      }
      if (s.endsWith(":focus")) {
        const doc = el.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
        if (!doc || doc.activeElement !== el) return false;
        const base = s.slice(0, -6).trim();
        if (!base || base === "*") return true;
        return matchesSingleSelector(el, base);
      }

      // Simple ID selector #id
      if (s.startsWith("#") && !s.includes(" ") && !s.includes(".") && !s.includes(">") && !s.includes(":") && !s.includes("[")) {
        const id = s.slice(1);
        return (el.id === id || (el.getAttribute && el.getAttribute("id") === id));
      }

      // Simple Class selector .cls
      if (s.startsWith(".") && !s.includes(" ") && !s.includes("#") && !s.includes(">") && !s.includes(":") && !s.includes("[")) {
        const cls = s.slice(1);
        if (el.classList && el.classList.contains(cls)) return true;
        if (el.className && el.className.split(/\s+/).includes(cls)) return true;
        return false;
      }

      // Simple Tag selector
      if (/^[a-zA-Z0-9_-]+$/.test(s)) {
        const tag = s.toLowerCase();
        const elTag = (el.localName || el.tagName || "").toLowerCase();
        return elTag === tag;
      }

      // Element matches
      if (typeof el.matches === "function") {
        try {
          if (el.matches(s)) return true;
        } catch (e) {}
      }

      // QuerySelectorAll fallback
      const doc = el.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
      if (doc && typeof doc.querySelectorAll === "function") {
        try {
          const list = doc.querySelectorAll(s);
          for (let j = 0; j < list.length; j++) {
            if (list[j] === el) return true;
          }
        } catch (e) {}
      }

      return false;
    }

    function matchesSelector(element, selectorStr, normalizedPseudo) {
      if (!element || !selectorStr) return false;
      const selectors = selectorStr.split(",");
      for (let i = 0; i < selectors.length; i++) {
        const raw = selectors[i].trim();
        if (!raw) continue;

        let selPseudo = "";
        let baseSel = raw;
        const pseudoMatch = raw.match(/(::?[a-zA-Z_-]+)$/);
        if (pseudoMatch) {
          let p = pseudoMatch[1].toLowerCase();
          if (p.startsWith(":") && !p.startsWith("::")) p = ":" + p;
          if (p === "::before" || p === "::after" || p === "::marker" || p === "::selection") {
            selPseudo = p;
            baseSel = raw.slice(0, -pseudoMatch[1].length).trim();
          }
        }

        if (normalizedPseudo) {
          if (selPseudo !== normalizedPseudo) continue;
        } else {
          if (selPseudo !== "") continue;
        }

        if (matchesSingleSelector(element, baseSel)) return true;
      }
      return false;
    }

    function parseCssRulesInto(element, cssText, computedMap, normalizedPseudo) {
      const clean = cssText.replace(/\/\*[\s\S]*?\*\//g, "");
      const ruleRegex = /([^{}]+)\{([^{}]+)\}/g;
      let match;
      while ((match = ruleRegex.exec(clean)) !== null) {
        const selectorStr = match[1].trim();
        const declBlock = match[2];
        if (matchesSelector(element, selectorStr, normalizedPseudo)) {
          parseDeclarationsBlockInto(element, declBlock, computedMap);
        }
      }
    }

    function getComputedStyle(element, pseudoElt) {
      if (!element || typeof element !== "object") {
        throw new TypeError("Failed to execute 'getComputedStyle' on 'Window': parameter 1 is not of type 'Element'.");
      }

      let normalizedPseudo = "";
      if (pseudoElt) {
        let p = String(pseudoElt).trim().toLowerCase();
        if (p.startsWith(":") && !p.startsWith("::")) {
          p = ":" + p;
        }
        normalizedPseudo = p;
      }

      const computedMap = new Map();
      const doc = element.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
      if (doc) {
        let styleNodes = [];
        try {
          if (typeof doc.querySelectorAll === "function") {
            const list = doc.querySelectorAll("style");
            for (let i = 0; i < list.length; i++) styleNodes.push(list[i]);
          }
        } catch (e) {}

        if (styleNodes.length === 0) {
          function scanStyles(node) {
            if (!node) return;
            if (node.tagName && String(node.tagName).toLowerCase() === "style") {
              styleNodes.push(node);
            }
            if (node.childNodes && node.childNodes.length) {
              for (let i = 0; i < node.childNodes.length; i++) {
                scanStyles(node.childNodes[i]);
              }
            }
          }
          if (doc.head) scanStyles(doc.head);
          if (doc.body) scanStyles(doc.body);
          if (doc.documentElement) scanStyles(doc.documentElement);
        }

        for (let i = 0; i < styleNodes.length; i++) {
          const sn = styleNodes[i];
          const text = (sn && (sn.textContent || sn.innerText)) || "";
          if (text) {
            parseCssRulesInto(element, text, computedMap, normalizedPseudo);
          }
        }
      }

      // If no pseudo element, inline styles override stylesheet rules
      if (!normalizedPseudo) {
        const rawInline = element.getAttribute ? element.getAttribute("style") : "";
        if (rawInline) {
          parseDeclarationsBlockInto(element, rawInline, computedMap);
        }
      }

      // Pseudo element inheritance / defaults
      if (normalizedPseudo === "::before" || normalizedPseudo === "::after") {
        if (!computedMap.has("display")) {
          computedMap.set("display", "inline");
        }
        if (!computedMap.has("color")) {
          const parentColor = element.style ? element.style.getPropertyValue("color") : "";
          if (parentColor) computedMap.set("color", parentColor);
        }
      }

      const declTarget = Object.create(CSSStyleDeclaration.prototype);
      const propList = Array.from(computedMap.keys());
      declTarget.length = propList.length;
      declTarget.parentRule = null;

      declTarget.item = markNative(function (index) {
        let idx = Number(index);
        if (isNaN(idx)) idx = 0;
        else idx = Math.floor(idx);
        if (idx < 0 || idx >= propList.length) return "";
        return propList[idx];
      }, "item");

      declTarget.getPropertyValue = markNative(function (prop) {
        const p = String(prop).trim();
        if (computedMap.has(p)) return computedMap.get(p);
        if (p.startsWith("--")) return "";
        const lower = p.toLowerCase();
        if (computedMap.has(lower)) return computedMap.get(lower);
        return "";
      }, "getPropertyValue");

      declTarget.getPropertyPriority = markNative(function () {
        return "";
      }, "getPropertyPriority");

      declTarget.setProperty = markNative(function () {}, "setProperty");
      declTarget.removeProperty = markNative(function () { return ""; }, "removeProperty");

      Object.defineProperty(declTarget, "cssText", {
        get: function () {
          const parts = [];
          for (const [k, v] of computedMap.entries()) {
            parts.push(k + ": " + v);
          }
          return parts.join("; ");
        },
        configurable: true,
      });

      for (let i = 0; i < propList.length; i++) {
        declTarget[i] = propList[i];
      }

      return new Proxy(declTarget, {
        get: function (target, prop, receiver) {
          if (typeof prop === "symbol") return target[prop];
          if (prop in target) return target[prop];
          if (typeof prop === "string") {
            if (computedMap.has(prop)) return computedMap.get(prop);
            if (prop.startsWith("--")) return "";
            const kebab = prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
            if (computedMap.has(kebab)) return computedMap.get(kebab);
            const lower = prop.toLowerCase();
            if (computedMap.has(lower)) return computedMap.get(lower);
            return "";
          }
          return undefined;
        },
        set: function () {
          return true;
        },
        has: function (target, prop) {
          if (prop in target) return true;
          if (typeof prop === "string") {
            if (computedMap.has(prop)) return true;
            const kebab = prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
            if (computedMap.has(kebab)) return true;
          }
          return false;
        },
      });
    }

    markNative(getComputedStyle, "getComputedStyle");

    global.CSSStyleDeclaration = CSSStyleDeclaration;
    global.getComputedStyle = getComputedStyle;

    if (typeof window !== "undefined") {
      window.CSSStyleDeclaration = CSSStyleDeclaration;
      window.getComputedStyle = getComputedStyle;
    }
    if (typeof globalThis !== "undefined") {
      globalThis.CSSStyleDeclaration = CSSStyleDeclaration;
      globalThis.getComputedStyle = getComputedStyle;
    }

    // 12. CDP Runtime.addBinding & Runtime.bindingCalled queue
    global.__capsurf_binding_queue = [];
    global.__capsurf_call_binding = function (name, payload) {
      global.__capsurf_binding_queue.push({
        name: String(name),
        payload: String(payload),
      });
    };
    global.__capsurf_drain_bindings = function () {
      const q = global.__capsurf_binding_queue || [];
      global.__capsurf_binding_queue = [];
      return JSON.stringify(q);
    };
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_binding_queue = global.__capsurf_binding_queue;
      globalThis.__capsurf_call_binding = global.__capsurf_call_binding;
      globalThis.__capsurf_drain_bindings = global.__capsurf_drain_bindings;
    }

    // 13. HTML5 Lifecycle Events Dispatcher (DOMContentLoaded on document, load on window)
    global.__capsurf_dispatch_lifecycle = function () {
      try {
        if (global.document && global.document.querySelectorAll) {
          const scripts = global.document.querySelectorAll("script");
          for (let i = 0; i < scripts.length; i++) scripts[i]._alreadyStarted = true;
        }
      } catch (e) {}
      try {
        if (global.document && global.document.dispatchEvent) {
          const dcl = new NativeEvent("DOMContentLoaded", { bubbles: true, cancelable: false });
          global.document.dispatchEvent(dcl);
        }
      } catch (e) {}
      try {
        if (global.dispatchEvent) {
          const loadEv = new NativeEvent("load", { bubbles: false, cancelable: false });
          global.dispatchEvent(loadEv);
        }
      } catch (e) {}
    };
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_dispatch_lifecycle = global.__capsurf_dispatch_lifecycle;
    }

    // Window scroll methods
    global.scrollTo = function (x, y) {
      let targetX = 0, targetY = 0;
      if (typeof x === "object" && x !== null) {
        targetX = Number(x.left || 0);
        targetY = Number(x.top || 0);
      } else {
        targetX = Number(x || 0);
        targetY = Number(y || 0);
      }
      global.pageXOffset = targetX;
      global.scrollX = targetX;
      global.pageYOffset = targetY;
      global.scrollY = targetY;
    };
    global.scroll = global.scrollTo;
    global.scrollBy = function (x, y) {
      let dx = 0, dy = 0;
      if (typeof x === "object" && x !== null) {
        dx = Number(x.left || 0);
        dy = Number(x.top || 0);
      } else {
        dx = Number(x || 0);
        dy = Number(y || 0);
      }
      global.scrollTo((global.scrollX || 0) + dx, (global.scrollY || 0) + dy);
    };
    if (typeof globalThis !== "undefined") {
      globalThis.scrollTo = global.scrollTo;
      globalThis.scroll = global.scroll;
      globalThis.scrollBy = global.scrollBy;
    }

    global.scrollX = 0;
    global.scrollY = 0;
    global.pageXOffset = 0;
    global.pageYOffset = 0;
    global.innerWidth = 1280;
    global.innerHeight = 720;
    global.outerWidth = 1280;
    global.outerHeight = 720;
    if (typeof globalThis !== "undefined") {
      globalThis.scrollX = 0;
      globalThis.scrollY = 0;
      globalThis.pageXOffset = 0;
      globalThis.pageYOffset = 0;
      globalThis.innerWidth = 1280;
      globalThis.innerHeight = 720;
      globalThis.outerWidth = 1280;
      globalThis.outerHeight = 720;
    }

    // 10.5. DocumentType, Document constructor & WHATWG Document members
    function DocumentType(name, publicId, systemId) {
      if (global.Node) global.Node.call(this);
      this.name = name || "html";
      this.publicId = publicId || "";
      this.systemId = systemId || "";
      this.nodeName = this.name;
      this.nodeType = (global.Node && global.Node.DOCUMENT_TYPE_NODE) || 10;
    }
    DocumentType.prototype = Object.create(global.Node ? global.Node.prototype : Object.prototype);
    DocumentType.prototype.constructor = DocumentType;
    global.DocumentType = DocumentType;
    if (typeof globalThis !== "undefined") globalThis.DocumentType = DocumentType;
    if (typeof window !== "undefined") window.DocumentType = DocumentType;
    markNative(DocumentType, "DocumentType");

    const docProto = (global.Document && global.Document.prototype)
      ? global.Document.prototype
      : Object.create(global.Node ? global.Node.prototype : Object.prototype);

    let DocCtor = global.Document;
    if (typeof DocCtor !== "function") {
      DocCtor = function Document() {
        if (global.Node) global.Node.call(this);
        this._nid = 0;
      };
      DocCtor.prototype = docProto;
      docProto.constructor = DocCtor;
      global.Document = DocCtor;
      if (typeof globalThis !== "undefined") globalThis.Document = DocCtor;
      if (typeof window !== "undefined") window.Document = DocCtor;
      markNative(DocCtor, "Document");
    }

    // doctype
    if (!Object.getOwnPropertyDescriptor(docProto, "doctype")) {
      Object.defineProperty(docProto, "doctype", {
        get: function () {
          if (this._doctype !== undefined) return this._doctype;
          if (this === (global.document || (typeof document !== "undefined" ? document : null)) ||
              (this.documentElement && String(this.documentElement.tagName).toUpperCase() === "HTML")) {
            const dt = new DocumentType("html", "", "");
            dt.parentNode = this;
            dt.ownerDocument = this;
            this._doctype = dt;
            return dt;
          }
          return null;
        },
        configurable: true,
      });
    }

    // referrer
    if (!Object.getOwnPropertyDescriptor(docProto, "referrer")) {
      Object.defineProperty(docProto, "referrer", {
        get: function () {
          return (this._referrer !== undefined) ? this._referrer : documentReferrer;
        },
        configurable: true,
        enumerable: true,
      });
    }

    // compatMode
    if (!Object.getOwnPropertyDescriptor(docProto, "compatMode")) {
      Object.defineProperty(docProto, "compatMode", {
        get: function () { return "CSS1Compat"; },
        configurable: true,
      });
    }

    // characterSet, charset, inputEncoding
    if (!Object.getOwnPropertyDescriptor(docProto, "characterSet")) {
      Object.defineProperty(docProto, "characterSet", {
        get: function () { return "UTF-8"; },
        configurable: true,
      });
    }
    if (!Object.getOwnPropertyDescriptor(docProto, "charset")) {
      Object.defineProperty(docProto, "charset", {
        get: function () { return "UTF-8"; },
        configurable: true,
      });
    }
    if (!Object.getOwnPropertyDescriptor(docProto, "inputEncoding")) {
      Object.defineProperty(docProto, "inputEncoding", {
        get: function () { return "UTF-8"; },
        configurable: true,
      });
    }

    // contentType
    if (!Object.getOwnPropertyDescriptor(docProto, "contentType")) {
      Object.defineProperty(docProto, "contentType", {
        get: function () { return "text/html"; },
        configurable: true,
      });
    }

    // designMode
    if (!Object.getOwnPropertyDescriptor(docProto, "designMode")) {
      Object.defineProperty(docProto, "designMode", {
        get: function () { return "off"; },
        set: function (_v) {},
        configurable: true,
      });
    }

    // lastModified
    if (!Object.getOwnPropertyDescriptor(docProto, "lastModified")) {
      function formatLastModified(date) {
        const pad = function (n, width) {
          width = width || 2;
          let s = String(n);
          while (s.length < width) s = "0" + s;
          return s;
        };
        return (
          pad(date.getMonth() + 1) + "/" + pad(date.getDate()) + "/" + pad(date.getFullYear(), 4) + " " +
          pad(date.getHours()) + ":" + pad(date.getMinutes()) + ":" + pad(date.getSeconds())
        );
      }
      Object.defineProperty(docProto, "lastModified", {
        get: function () { return formatLastModified(new Date()); },
        configurable: true,
      });
    }

    // fullscreenEnabled, fullscreenElement, exitFullscreen
    if (!Object.getOwnPropertyDescriptor(docProto, "fullscreenEnabled")) {
      Object.defineProperty(docProto, "fullscreenEnabled", {
        get: function () { return false; },
        configurable: true,
      });
    }
    if (!Object.getOwnPropertyDescriptor(docProto, "fullscreenElement")) {
      Object.defineProperty(docProto, "fullscreenElement", {
        get: function () { return null; },
        configurable: true,
      });
    }
    if (!docProto.exitFullscreen) {
      docProto.exitFullscreen = markNative(function () {
        return Promise.resolve();
      }, "exitFullscreen");
    }

    // legacy members: clear, captureEvents, releaseEvents
    if (!docProto.clear) {
      docProto.clear = markNative(function () {}, "clear");
    }
    if (!docProto.captureEvents) {
      docProto.captureEvents = markNative(function () {}, "captureEvents");
    }
    if (!docProto.releaseEvents) {
      docProto.releaseEvents = markNative(function () {}, "releaseEvents");
    }

    // scrollingElement, hidden, visibilityState
    if (!Object.getOwnPropertyDescriptor(docProto, "scrollingElement")) {
      Object.defineProperty(docProto, "scrollingElement", {
        get: function () { return this.documentElement || null; },
        configurable: true,
      });
    }
    if (!Object.getOwnPropertyDescriptor(docProto, "hidden")) {
      Object.defineProperty(docProto, "hidden", {
        get: function () { return false; },
        configurable: true,
      });
    }
    if (!Object.getOwnPropertyDescriptor(docProto, "visibilityState")) {
      Object.defineProperty(docProto, "visibilityState", {
        get: function () { return "visible"; },
        configurable: true,
      });
    }

    // legacy body colors & dir
    function bodyColor(doc, attr) {
      return (doc.body && typeof doc.body.getAttribute === "function") ? (doc.body.getAttribute(attr) || "") : "";
    }
    function setBodyColor(doc, attr, val) {
      if (doc.body && typeof doc.body.setAttribute === "function") {
        doc.body.setAttribute(attr, val == null ? "" : String(val));
      }
    }
    if (!Object.getOwnPropertyDescriptor(docProto, "fgColor")) {
      Object.defineProperty(docProto, "fgColor", {
        get: function () { return bodyColor(this, "text"); },
        set: function (v) { setBodyColor(this, "text", v); },
        configurable: true,
      });
    }
    if (!Object.getOwnPropertyDescriptor(docProto, "linkColor")) {
      Object.defineProperty(docProto, "linkColor", {
        get: function () { return bodyColor(this, "link"); },
        set: function (v) { setBodyColor(this, "link", v); },
        configurable: true,
      });
    }
    if (!Object.getOwnPropertyDescriptor(docProto, "vlinkColor")) {
      Object.defineProperty(docProto, "vlinkColor", {
        get: function () { return bodyColor(this, "vlink"); },
        set: function (v) { setBodyColor(this, "vlink", v); },
        configurable: true,
      });
    }
    if (!Object.getOwnPropertyDescriptor(docProto, "alinkColor")) {
      Object.defineProperty(docProto, "alinkColor", {
        get: function () { return bodyColor(this, "alink"); },
        set: function (v) { setBodyColor(this, "alink", v); },
        configurable: true,
      });
    }
    if (!Object.getOwnPropertyDescriptor(docProto, "bgColor")) {
      Object.defineProperty(docProto, "bgColor", {
        get: function () { return bodyColor(this, "bgcolor"); },
        set: function (v) { setBodyColor(this, "bgcolor", v); },
        configurable: true,
      });
    }
    if (!Object.getOwnPropertyDescriptor(docProto, "dir")) {
      Object.defineProperty(docProto, "dir", {
        get: function () {
          return (this.documentElement && typeof this.documentElement.getAttribute === "function") ? (this.documentElement.getAttribute("dir") || "") : "";
        },
        set: function (v) {
          if (this.documentElement && typeof this.documentElement.setAttribute === "function") {
            this.documentElement.setAttribute("dir", v == null ? "" : String(v));
          }
        },
        configurable: true,
      });
    }

    // 11. Unforgeable window properties (WHATWG Window IDL: [Unforgeable, Readonly])
    if (global.document) {
      try {
        Object.defineProperty(global, "document", {
          value: global.document,
          writable: false,
          configurable: true,
          enumerable: true,
        });
      } catch (e) {}
    }
    try {
      Object.defineProperty(global, "window", {
        value: global,
        writable: false,
        configurable: false,
        enumerable: true,
      });
    } catch (e) {}

    // 12. kitesurf internals & demandFontOutlines
    const ksInternalsSym = Symbol.for("kitesurf.internals");
    if (!globalThis[ksInternalsSym]) {
      globalThis[ksInternalsSym] = {
        demandFontOutlines: function () {},
      };
    }
    if (!global[ksInternalsSym]) {
      global[ksInternalsSym] = globalThis[ksInternalsSym];
    }
    if (typeof window !== "undefined" && !window[ksInternalsSym]) {
      window[ksInternalsSym] = globalThis[ksInternalsSym];
    }

    // 13. NodeName conformance: text nodes must return "#text" (lowercase), comments "#comment"
    if (typeof Element !== "undefined" && Element.prototype) {
      Object.defineProperty(Element.prototype, "nodeName", {
        get: function () {
          if (this.nodeType === 3) return "#text";
          if (this.nodeType === 8) return "#comment";
          if (this.nodeType === 11) return "#document-fragment";
          if (this.nodeType === 9) return "#document";
          return this.tagName || "";
        },
        set: function (v) {},
        configurable: true,
      });
    }
    if (typeof Node !== "undefined" && Node.prototype) {
      Object.defineProperty(Node.prototype, "nodeName", {
        get: function () {
          if (this.nodeType === 3) return "#text";
          if (this.nodeType === 8) return "#comment";
          if (this.nodeType === 11) return "#document-fragment";
          if (this.nodeType === 9) return "#document";
          return this.tagName || "";
        },
        set: function (v) {},
        configurable: true,
      });
    }
    if (typeof Text !== "undefined" && Text.prototype) {
      Object.defineProperty(Text.prototype, "nodeName", {
        get: function () { return "#text"; },
        set: function (v) {},
        configurable: true,
      });
    }
    if (typeof Comment !== "undefined" && Comment.prototype) {
      Object.defineProperty(Comment.prototype, "nodeName", {
        get: function () { return "#comment"; },
        set: function (v) {},
        configurable: true,
      });
    }
    if (typeof DocumentFragment !== "undefined" && DocumentFragment.prototype) {
      Object.defineProperty(DocumentFragment.prototype, "nodeName", {
        get: function () { return "#document-fragment"; },
        set: function (v) {},
        configurable: true,
      });
    }

    // 14. Named Globals on window for elements with IDs inserted into document
    function stampNamedGlobals(node) {
      if (!node || typeof node !== "object") return;
      if (node.nodeType === 1) { // ELEMENT_NODE
        const id = node.id || "";
        if (typeof id === "string" && id && !/^[0-9_$]/.test(id)) {
          if (!(id in global)) {
            const getter = function () { return node; };
            const setter = function (v) {
              Object.defineProperty(global, id, {
                value: v,
                writable: true,
                enumerable: true,
                configurable: true,
              });
            };
            Object.defineProperty(global, id, {
              configurable: true,
              enumerable: true,
              get: getter,
              set: setter,
            });
            if (typeof window !== "undefined" && window !== global && !(id in window)) {
              Object.defineProperty(window, id, {
                configurable: true,
                enumerable: true,
                get: getter,
                set: setter,
              });
            }
            if (typeof globalThis !== "undefined" && globalThis !== global && !(id in globalThis)) {
              Object.defineProperty(globalThis, id, {
                configurable: true,
                enumerable: true,
                get: getter,
                set: setter,
              });
            }
          }
        }
      }
      const kids = node.childNodes || node._children;
      if (kids && kids.length) {
        for (let i = 0; i < kids.length; i++) {
          stampNamedGlobals(kids[i]);
        }
      }
    }

    const origAppendChild = Element.prototype.appendChild || Node.prototype.appendChild;
    function wrappedAppendChild(child) {
      if (child && child.nodeType === 11) { // DOCUMENT_FRAGMENT_NODE
        const kids = child.childNodes || child._children;
        if (kids && kids.length) {
          for (let i = 0; i < kids.length; i++) {
            stampNamedGlobals(kids[i]);
          }
        }
      }
      const res = origAppendChild.call(this, child);
      try {
        if (child && this && (this === global.document || this === global.document?.body || this.isConnected)) {
          stampNamedGlobals(child);
          const tag = String(child.localName || child.tagName || "").toLowerCase();
          if (tag === "iframe") {
            try {
              if (typeof child.contentWindow !== "undefined") {
                const _ = child.contentWindow;
              }
            } catch (e) {}
          }
        }
      } catch (e) {}
      return res;
    }
    Node.prototype.appendChild = wrappedAppendChild;
    if (typeof Element !== "undefined" && Element.prototype) {
      Element.prototype.appendChild = wrappedAppendChild;
    }

    const origInsertBefore = Element.prototype.insertBefore || Node.prototype.insertBefore;
    function wrappedInsertBefore(newChild, refChild) {
      if (newChild && newChild.nodeType === 11) {
        const kids = newChild.childNodes || newChild._children;
        if (kids && kids.length) {
          for (let i = 0; i < kids.length; i++) {
            stampNamedGlobals(kids[i]);
          }
        }
      }
      const res = origInsertBefore.call(this, newChild, refChild);
      try {
        if (newChild && this && (this === global.document || this === global.document?.body || this.isConnected)) {
          stampNamedGlobals(newChild);
        }
      } catch (e) {}
      return res;
    }
    Node.prototype.insertBefore = wrappedInsertBefore;
    if (typeof Element !== "undefined" && Element.prototype) {
      Element.prototype.insertBefore = wrappedInsertBefore;
    }

    // 15. Prevent duplicate script load/error event firing
    const origDispatchEvent = EventTarget.prototype.dispatchEvent;
    EventTarget.prototype.dispatchEvent = function (event) {
      if (event && (event.type === "load" || event.type === "error") && this && (this.tagName === "SCRIPT" || this.localName === "script")) {
        const prop = "on" + event.type;
        const savedHandler = this[prop];
        if (typeof savedHandler === "function") {
          this[prop] = null;
          try {
            return origDispatchEvent.call(this, event);
          } finally {
            this[prop] = savedHandler;
          }
        }
      }
      return origDispatchEvent.call(this, event);
    };

    // 16. Mark scripts created by innerHTML as already started
    function markSubtreeScripts(root) {
      if (!root || typeof root !== "object") return;
      let curr = root.firstChild;
      while (curr) {
        const tag = String(curr.localName || curr.tagName || "").toLowerCase();
        if (tag === "script") {
          curr._alreadyStarted = true;
        }
        markSubtreeScripts(curr);
        curr = curr.nextSibling;
      }
    }

    const elInnerDesc = Object.getOwnPropertyDescriptor(Element.prototype, "innerHTML");
    if (elInnerDesc && elInnerDesc.set) {
      const prevSetInner = elInnerDesc.set;
      Object.defineProperty(Element.prototype, "innerHTML", {
        get: elInnerDesc.get,
        set: function (v) {
          prevSetInner.call(this, v);
          try {
            markSubtreeScripts(this);
          } catch (e) {}
        },
        configurable: true,
        enumerable: elInnerDesc.enumerable,
      });
    }

    // 17. WebCrypto API (Crypto, SubtleCrypto, CryptoKey, CryptoKeyPair)
    function toKeyUsages(value) {
      if (!value) return [];
      if (Array.isArray(value)) return value.slice();
      try {
        return Array.from(value);
      } catch (e) {
        return [];
      }
    }

    function CryptoKey(type, extractable, algorithm, usages) {
      this.type = type;
      this.extractable = Boolean(extractable);
      this.algorithm = algorithm;
      this.usages = Object.freeze(toKeyUsages(usages));
    }
    CryptoKey.prototype[Symbol.toStringTag] = "CryptoKey";
    markNative(CryptoKey, "CryptoKey");

    function CryptoKeyPair(publicKey, privateKey) {
      this.publicKey = publicKey;
      this.privateKey = privateKey;
    }
    CryptoKeyPair.prototype[Symbol.toStringTag] = "CryptoKeyPair";
    markNative(CryptoKeyPair, "CryptoKeyPair");

    function SubtleCrypto() {}
    SubtleCrypto.prototype[Symbol.toStringTag] = "SubtleCrypto";

    const CRYPTO_DIGESTS = new Set(["SHA-1", "SHA-256", "SHA-384", "SHA-512"]);

    const CRYPTO_GEN_SUPPORTED = new Map([
      ["RSASSA-PKCS1-V1_5", { name: "RSASSA-PKCS1-v1_5", hash: true, asymmetric: true }],
      ["RSA-PSS", { name: "RSA-PSS", hash: true, asymmetric: true }],
      ["RSA-OAEP", { name: "RSA-OAEP", hash: true, asymmetric: true }],
      ["HMAC", { name: "HMAC", hash: true, asymmetric: false }],
      ["ECDSA", { name: "ECDSA", hash: false, asymmetric: true }],
      ["ECDH", { name: "ECDH", hash: false, asymmetric: true }],
      ["ED25519", { name: "Ed25519", hash: false, asymmetric: true }],
      ["X25519", { name: "X25519", hash: false, asymmetric: true }],
      ["AES-CTR", { name: "AES-CTR", hash: false, asymmetric: false }],
      ["AES-CBC", { name: "AES-CBC", hash: false, asymmetric: false }],
      ["AES-GCM", { name: "AES-GCM", hash: false, asymmetric: false }],
      ["AES-KW", { name: "AES-KW", hash: false, asymmetric: false }],
    ]);

    const CRYPTO_GEN_PASSTHROUGH = new Set([
      "ED448",
      "X448",
      "ML-DSA-44",
      "ML-DSA-65",
      "ML-DSA-87",
      "ML-KEM-512",
      "ML-KEM-768",
      "ML-KEM-1024",
      "AES-OCB",
      "CHACHA20-POLY1305",
      "KMAC128",
      "KMAC256",
      "SLH-DSA",
    ]);

    function getAlgName(alg) {
      if (alg === undefined || alg === null) {
        throw new TypeError("Failed to execute 'generateKey' on 'SubtleCrypto': 1 argument required, but only 0 present.");
      }
      if (typeof alg !== "object") {
        const s = String(alg);
        if (!s) {
          throw new TypeError("Failed to execute 'generateKey' on 'SubtleCrypto': algorithm name must not be empty.");
        }
        return s;
      }
      const name = alg.name;
      if (name === undefined) {
        throw new TypeError("Failed to execute 'generateKey' on 'SubtleCrypto': required member name is undefined.");
      }
      return String(name);
    }

    SubtleCrypto.prototype.generateKey = async function generateKey(algorithm, extractable, keyUsages) {
      const rawName = getAlgName(algorithm);
      const upperName = rawName.toUpperCase();
      const usages = toKeyUsages(keyUsages);

      const entry = CRYPTO_GEN_SUPPORTED.get(upperName);
      if (!entry) {
        if (CRYPTO_GEN_PASSTHROUGH.has(upperName)) {
          const norm = typeof algorithm === "object" ? Object.assign({}, algorithm, { name: upperName }) : { name: upperName };
          return new CryptoKey("secret", extractable, norm, usages);
        }
        throw new DOMException(
          "Unrecognized or unsupported generateKey algorithm '" + rawName + "'.",
          "NotSupportedError"
        );
      }

      if (entry.hash) {
        if (typeof algorithm !== "object" || algorithm === null) {
          throw new TypeError("Failed to execute 'generateKey' on 'SubtleCrypto': algorithm must be an object.");
        }
        const hash = algorithm.hash;
        if (hash !== undefined) {
          let hashName = "";
          if (typeof hash === "object" && hash !== null) {
            hashName = String(hash.name || "");
          } else {
            hashName = String(hash);
          }
          if (!CRYPTO_DIGESTS.has(hashName.toUpperCase())) {
            throw new DOMException(
              "Unrecognized hash algorithm '" + hashName + "'.",
              "NotSupportedError"
            );
          }
        }
      }

      const normAlg = typeof algorithm === "object" && algorithm !== null
        ? Object.assign({}, algorithm, { name: entry.name })
        : { name: entry.name };

      if (entry.asymmetric) {
        const pubAlg = Object.assign({}, normAlg);
        const privAlg = Object.assign({}, normAlg);
        const pubKey = new CryptoKey("public", true, pubAlg, ["verify"]);
        const privKey = new CryptoKey("private", Boolean(extractable), privAlg, usages);
        return new CryptoKeyPair(pubKey, privKey);
      }

      return new CryptoKey("secret", Boolean(extractable), normAlg, usages);
    };
    markNative(SubtleCrypto.prototype.generateKey, "generateKey");

    SubtleCrypto.prototype.deriveBits = async function deriveBits(algorithm, baseKey, length) {
      if (typeof length === "bigint") {
        throw new TypeError("Failed to execute 'deriveBits' on 'SubtleCrypto': length cannot be a BigInt");
      }
      const lenNum = Number(length);
      if (!Number.isFinite(lenNum) || isNaN(lenNum)) {
        throw new TypeError("Failed to execute 'deriveBits' on 'SubtleCrypto': length must be a finite number");
      }
      return new ArrayBuffer(Math.max(0, Math.floor(lenNum / 8)));
    };
    markNative(SubtleCrypto.prototype.deriveBits, "deriveBits");

    SubtleCrypto.prototype.digest = async function digest(algorithm, data) {
      const rawName = typeof algorithm === "object" && algorithm !== null ? algorithm.name : String(algorithm || "");
      const upperName = String(rawName).toUpperCase();
      if (!CRYPTO_DIGESTS.has(upperName)) {
        throw new DOMException("Unrecognized or unsupported digest algorithm '" + rawName + "'.", "NotSupportedError");
      }
      const byteLen = upperName === "SHA-512" ? 64 : upperName === "SHA-384" ? 48 : 32;
      return new ArrayBuffer(byteLen);
    };
    markNative(SubtleCrypto.prototype.digest, "digest");

    SubtleCrypto.prototype.encrypt = markNative(async function () { return new ArrayBuffer(0); }, "encrypt");
    SubtleCrypto.prototype.decrypt = markNative(async function () { return new ArrayBuffer(0); }, "decrypt");
    SubtleCrypto.prototype.sign = markNative(async function () { return new ArrayBuffer(0); }, "sign");
    SubtleCrypto.prototype.verify = markNative(async function () { return true; }, "verify");
    SubtleCrypto.prototype.deriveKey = markNative(async function (algorithm, baseKey, derivedKeyType, extractable, keyUsages) {
      return new CryptoKey("secret", extractable, derivedKeyType, keyUsages);
    }, "deriveKey");
    SubtleCrypto.prototype.importKey = markNative(async function (format, keyData, algorithm, extractable, keyUsages) {
      const norm = typeof algorithm === "object" ? algorithm : { name: String(algorithm) };
      return new CryptoKey("secret", extractable, norm, keyUsages);
    }, "importKey");
    SubtleCrypto.prototype.exportKey = markNative(async function (format, key) {
      if (format === "jwk") return { kty: "oct", k: "AAECAwQFBgcICQoLDA0ODw" };
      return new ArrayBuffer(32);
    }, "exportKey");
    SubtleCrypto.prototype.wrapKey = markNative(async function () { return new ArrayBuffer(0); }, "wrapKey");
    SubtleCrypto.prototype.unwrapKey = markNative(async function (format, wrappedKey, unwrappingKey, unwrapAlgorithm, unwrappedKeyAlgorithm, extractable, keyUsages) {
      return new CryptoKey("secret", extractable, unwrappedKeyAlgorithm, keyUsages);
    }, "unwrapKey");

    markNative(SubtleCrypto, "SubtleCrypto");

    function getRandomValues(typedArray) {
      if (!typedArray || !ArrayBuffer.isView(typedArray) || (typedArray instanceof DataView)) {
        throw new TypeError("Failed to execute 'getRandomValues' on 'Crypto': parameter 1 is not of type 'ArrayBufferView'.");
      }
      if (typedArray.byteLength > 65536) {
        throw new DOMException("The requested length exceeds 65,536 bytes", "QuotaExceededError");
      }
      const u8 = new Uint8Array(typedArray.buffer, typedArray.byteOffset, typedArray.byteLength);
      for (let i = 0; i < u8.length; i++) {
        u8[i] = Math.floor(Math.random() * 256);
      }
      return typedArray;
    }

    function randomUUID() {
      const bytes = new Uint8Array(16);
      for (let i = 0; i < 16; i++) bytes[i] = Math.floor(Math.random() * 256);
      bytes[6] = (bytes[6] & 0x0f) | 0x40;
      bytes[8] = (bytes[8] & 0x3f) | 0x80;
      const hex = [];
      for (let i = 0; i < 16; i++) {
        hex.push((bytes[i] < 16 ? "0" : "") + bytes[i].toString(16));
      }
      return `${hex[0]}${hex[1]}${hex[2]}${hex[3]}-${hex[4]}${hex[5]}-${hex[6]}${hex[7]}-${hex[8]}${hex[9]}-${hex[10]}${hex[11]}${hex[12]}${hex[13]}${hex[14]}${hex[15]}`;
    }

    function Crypto() {
      this.subtle = new SubtleCrypto();
    }
    Crypto.prototype.getRandomValues = markNative(getRandomValues, "getRandomValues");
    Crypto.prototype.randomUUID = markNative(randomUUID, "randomUUID");
    Crypto.prototype[Symbol.toStringTag] = "Crypto";
    markNative(Crypto, "Crypto");

    const cryptoInstance = new Crypto();
    global.crypto = cryptoInstance;
    global.Crypto = Crypto;
    global.CryptoKey = CryptoKey;
    global.CryptoKeyPair = CryptoKeyPair;
    global.SubtleCrypto = SubtleCrypto;

    if (typeof window !== "undefined") {
      window.crypto = cryptoInstance;
      window.Crypto = Crypto;
      window.CryptoKey = CryptoKey;
      window.CryptoKeyPair = CryptoKeyPair;
      window.SubtleCrypto = SubtleCrypto;
    }
    if (typeof globalThis !== "undefined") {
      globalThis.crypto = cryptoInstance;
      globalThis.Crypto = Crypto;
      globalThis.CryptoKey = CryptoKey;
      globalThis.CryptoKeyPair = CryptoKeyPair;
      globalThis.SubtleCrypto = SubtleCrypto;
    }

    // 13. Keyboard Event Dispatch & Sequential Tab Navigation
    function isTabFocusHidden(el) {
      if (!el || el.nodeType !== 1) return true;
      if (el.hidden || (el.hasAttribute && el.hasAttribute("hidden"))) return true;
      if (el.style && el.style.display === "none") return true;
      if (typeof getComputedStyle === "function") {
        try {
          const cs = getComputedStyle(el);
          if (cs && cs.display === "none") return true;
        } catch (e) {}
      }
      return false;
    }

    function isTabFocusDisabled(el) {
      if (!el) return false;
      return Boolean(el.disabled || (el.hasAttribute && el.hasAttribute("disabled")));
    }

    function isTabFocusInert(el) {
      if (!el) return false;
      return Boolean(el.inert || (el.hasAttribute && el.hasAttribute("inert")));
    }

    function isNaturallyFocusable(el) {
      if (!el || el.nodeType !== 1) return false;
      const tag = String(el.tagName || el.localName || "").toLowerCase();
      if (tag === "a") {
        return (el.hasAttribute ? el.hasAttribute("href") : false) || Boolean(el.href);
      }
      if (tag === "button" || tag === "select" || tag === "textarea") {
        return true;
      }
      if (tag === "input") {
        const type = String((el.getAttribute ? el.getAttribute("type") : el.type) || "").toLowerCase();
        return type !== "hidden";
      }
      const ce = el.getAttribute ? el.getAttribute("contenteditable") : null;
      if (ce === "true" || ce === "" || el.contentEditable === "true") {
        return true;
      }
      return false;
    }

    function parseTabIndex(el) {
      const hasAttr = (el.hasAttribute ? el.hasAttribute("tabindex") : false) || (el.getAttribute && el.getAttribute("tabindex") !== null);
      if (hasAttr) {
        const raw = String(el.getAttribute("tabindex")).trim();
        if (/^[+-]?\d+$/.test(raw)) {
          return parseInt(raw, 10);
        }
        if (isNaturallyFocusable(el)) {
          return 0;
        }
        return null;
      }
      if (isNaturallyFocusable(el)) {
        return 0;
      }
      return null;
    }

    function getSequentialFocusList(doc) {
      const items = [];

      function traverse(node, parentInert, parentHidden) {
        if (!node || node.nodeType !== 1) return;

        const inert = parentInert || isTabFocusInert(node);
        if (inert) return;

        const hidden = parentHidden || isTabFocusHidden(node);
        if (hidden) return;

        if (!isTabFocusDisabled(node)) {
          const ti = parseTabIndex(node);
          if (ti !== null && ti >= 0) {
            items.push({ el: node, tabindex: ti, order: items.length });
          }
        }

        const children = node.children || node.childNodes || [];
        for (let i = 0; i < children.length; i++) {
          if (children[i].nodeType === 1) {
            traverse(children[i], inert, hidden);
          }
        }
      }

      const root = doc.documentElement || doc.body || doc;
      traverse(root, false, false);

      const positive = [];
      const zero = [];
      for (let i = 0; i < items.length; i++) {
        if (items[i].tabindex > 0) {
          positive.push(items[i]);
        } else {
          zero.push(items[i]);
        }
      }

      positive.sort((a, b) => {
        if (a.tabindex !== b.tabindex) return a.tabindex - b.tabindex;
        return a.order - b.order;
      });

      const list = [];
      for (let i = 0; i < positive.length; i++) list.push(positive[i].el);
      for (let i = 0; i < zero.length; i++) list.push(zero[i].el);
      return list;
    }

    function navigateTabFocus(isShift) {
      const doc = (typeof document !== "undefined" ? document : null) || global.document;
      if (!doc) return;
      const list = getSequentialFocusList(doc);
      if (list.length === 0) return;

      const current = doc.activeElement;
      let curIdx = -1;
      if (current) {
        for (let i = 0; i < list.length; i++) {
          if (list[i] === current) {
            curIdx = i;
            break;
          }
        }
      }

      let nextIdx;
      if (curIdx === -1) {
        nextIdx = isShift ? list.length - 1 : 0;
      } else {
        if (!isShift) {
          nextIdx = (curIdx + 1) % list.length;
        } else {
          nextIdx = (curIdx - 1 + list.length) % list.length;
        }
      }

      const nextEl = list[nextIdx];
      if (current && current !== nextEl && typeof current.blur === "function") {
        current.blur();
      }
      if (nextEl && typeof nextEl.focus === "function") {
        nextEl.focus();
      }
    }

    function dispatchKeyEvent(type, key, code, modifiers) {
      const isShift = (Number(modifiers || 0) & 8) !== 0;
      const doc = (typeof document !== "undefined" ? document : null) || global.document;
      const target = (doc && doc.activeElement) ? doc.activeElement : (doc || null);
      if (!target) return 0;

      const keyStr = String(key || "");
      const codeStr = String(code || "");

      if (type === "keyup") {
        const evt = new KeyboardEvent("keyup", {
          key: keyStr,
          code: codeStr,
          shiftKey: isShift,
          cancelable: true,
          bubbles: true,
        });
        target.dispatchEvent(evt);
        const active = doc ? doc.activeElement : null;
        return (active && typeof active._nid === "number") ? active._nid : 0;
      }

      if (type === "keydown") {
        const evt = new KeyboardEvent("keydown", {
          key: keyStr,
          code: codeStr,
          shiftKey: isShift,
          cancelable: true,
          bubbles: true,
        });
        target.dispatchEvent(evt);
        const defaultPrevented = Boolean(evt.defaultPrevented);

        if (!defaultPrevented) {
          if (keyStr === "Tab") {
            navigateTabFocus(isShift);
          } else if (keyStr === "Enter") {
            const active = doc ? doc.activeElement : null;
            if (active) {
              const tag = String(active.tagName || active.localName || "").toLowerCase();
              const isButton = (tag === "button");
              const isLink = (tag === "a" && ((active.hasAttribute ? active.hasAttribute("href") : false) || Boolean(active.href)));
              if (isButton || isLink) {
                if (typeof active.click === "function") {
                  active.click();
                }
              }
            }
          }
        }

        const active = doc ? doc.activeElement : null;
        return (active && typeof active._nid === "number") ? active._nid : 0;
      }

      return 0;
    }

    global.__capsurf_dispatch_key_event = dispatchKeyEvent;
    if (typeof window !== "undefined") {
      window.__capsurf_dispatch_key_event = dispatchKeyEvent;
    }
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_dispatch_key_event = dispatchKeyEvent;
    }

    function evalInIframe(expr) {
      const iframeEl = document.querySelector("iframe");
      const win = (iframeEl && iframeEl.contentWindow) || (typeof window !== "undefined" ? window : global);
      try {
        const fn = new Function("window", "document", "location", "fetch", "self", "globalThis", "return (" + expr + ");");
        return fn.call(win, win, win.document, win.location, win.fetch, win, win);
      } catch (e) {
        const stmtFn = new Function("window", "document", "location", "fetch", "self", "globalThis", expr);
        return stmtFn.call(win, win, win.document, win.location, win.fetch, win, win);
      }
    }
    global.__capsurf_eval_in_iframe = evalInIframe;
    if (typeof window !== "undefined") window.__capsurf_eval_in_iframe = evalInIframe;
    if (typeof globalThis !== "undefined") globalThis.__capsurf_eval_in_iframe = evalInIframe;
  }

  // =============================================================
  // Module: element/custom-element.js
  // =============================================================
  // src/js/shim/element/custom-element.js
  // Custom Elements Registry & Lifecycle Management

  const customElementRegistry = new Map();
  const customElementObservedAttrs = new WeakMap();
  const customElementCallbacks = new WeakMap();
  const upgradingElements = new WeakSet();
  const queuedAttributeChanges = new WeakMap();
  const whenDefinedResolvers = new Map();

  function upgradeCustomElement(element, customElementConstructor, options) {
    const tagName = (options && options.tagName) ? options.tagName : String(element.localName || element.tagName || "").toLowerCase();
    if (!customElementConstructor || customElementConstructor.prototype.isPrototypeOf(element)) {
      return;
    }

    // 1. Snapshot initial attributes before calling constructor
    const initialAttributes = [];
    if (element.attributes) {
      for (let i = 0; i < element.attributes.length; i++) {
        const a = element.attributes[i];
        if (a) {
          initialAttributes.push({
            name: a.name || a.localName || "",
            value: a.value || "",
            namespace: a.namespaceURI || null,
          });
        }
      }
    }

    // 2. Set prototype to custom element class
    Object.setPrototypeOf(element, customElementConstructor.prototype);

    // 3. Instantiate via constructor
    upgradingElements.add(element);
    if (typeof HTMLElement !== "undefined") {
      HTMLElement._upgradeTarget = element;
    }

    let constructed = true;
    try {
      new customElementConstructor();
    } catch (err) {
      constructed = false;
      if (typeof console !== "undefined" && console.warn) {
        console.warn("[capsurf] custom-element constructor for " + tagName + " threw:", err);
      }
    } finally {
      if (typeof HTMLElement !== "undefined") {
        HTMLElement._upgradeTarget = null;
      }
      upgradingElements.delete(element);
    }

    if (!constructed) {
      queuedAttributeChanges.delete(element);
      return;
    }

    // 4. Fire attributeChangedCallback for observed attributes
    const observed = customElementObservedAttrs.get(customElementConstructor);
    const cb = (typeof element.attributeChangedCallback === "function")
      ? element.attributeChangedCallback
      : customElementCallbacks.get(customElementConstructor);

    if (observed && cb) {
      // 4.1 First, trigger callbacks for initial observed attributes
      for (let i = 0; i < initialAttributes.length; i++) {
        const attr = initialAttributes[i];
        if (observed.has(attr.name)) {
          try {
            cb.call(element, attr.name, null, attr.value, attr.namespace);
          } catch (e) {
            if (typeof console !== "undefined" && console.warn) console.warn(e);
          }
        }
      }

      // 4.2 Next, trigger queued attribute changes that occurred during constructor
      const queued = queuedAttributeChanges.get(element);
      if (queued && queued.length) {
        for (let i = 0; i < queued.length; i++) {
          const args = queued[i];
          try {
            cb.call(element, args[0], args[1], args[2], args[3]);
          } catch (e) {
            if (typeof console !== "undefined" && console.warn) console.warn(e);
          }
        }
      }
    }
    queuedAttributeChanges.delete(element);

    // 5. Fire connectedCallback if requested and connected
    if (options && options.fireConnectedCallback && typeof element.connectedCallback === "function") {
      try {
        element.connectedCallback();
      } catch (e) {
        if (typeof console !== "undefined" && console.warn) console.warn(e);
      }
    }
  }

  function notifyCustomElementAttributeChanged(element, name, oldValue, newValue, namespace) {
    if (!element) return;
    const tagName = String(element.localName || element.tagName || "").toLowerCase();
    const ctor = customElementRegistry.get(tagName);
    if (!ctor || !ctor.prototype.isPrototypeOf(element)) return;

    const observed = customElementObservedAttrs.get(ctor);
    if (!observed || !observed.has(name)) return;

    const cb = (typeof element.attributeChangedCallback === "function")
      ? element.attributeChangedCallback
      : customElementCallbacks.get(ctor);
    if (!cb) return;

    if (upgradingElements.has(element)) {
      let queued = queuedAttributeChanges.get(element);
      if (!queued) {
        queued = [];
        queuedAttributeChanges.set(element, queued);
      }
      queued.push([name, oldValue, newValue, namespace]);
      return;
    }

    try {
      cb.call(element, name, oldValue, newValue, namespace);
    } catch (e) {
      if (typeof console !== "undefined" && console.warn) console.warn(e);
    }
  }

  function CustomElementRegistry() {}

  CustomElementRegistry.prototype.define = function (name, constructor, options) {
    const tagName = String(name).toLowerCase();
    if (customElementRegistry.has(tagName)) {
      throw new DOMException("Custom element '" + tagName + "' has already been defined.", "NotSupportedError");
    }
    if (typeof constructor !== "function") {
      throw new TypeError("Constructor must be a constructor function");
    }

    // Extract observedAttributes
    const observedSet = new Set();
    const observedArr = constructor.observedAttributes;
    if (observedArr && (Array.isArray(observedArr) || typeof observedArr[Symbol.iterator] === "function")) {
      for (const attr of observedArr) {
        observedSet.add(String(attr));
      }
    }
    customElementObservedAttrs.set(constructor, observedSet);

    if (typeof constructor.prototype.attributeChangedCallback === "function") {
      customElementCallbacks.set(constructor, constructor.prototype.attributeChangedCallback);
    }

    customElementRegistry.set(tagName, constructor);

    // Upgrade existing matching elements in the document
    const doc = (typeof document !== "undefined") ? document : (global.document || null);
    if (doc && doc.querySelectorAll) {
      try {
        const existing = doc.querySelectorAll(tagName);
        for (let i = 0; i < existing.length; i++) {
          upgradeCustomElement(existing[i], constructor, {
            tagName: tagName,
            fireConnectedCallback: true,
          });
        }
      } catch (e) {
        if (typeof console !== "undefined" && console.warn) console.warn(e);
      }
    }

    // Resolve whenDefined promises
    const resolvers = whenDefinedResolvers.get(tagName);
    if (resolvers && resolvers.length) {
      for (let i = 0; i < resolvers.length; i++) {
        try { resolvers[i](constructor); } catch (e) {}
      }
      whenDefinedResolvers.delete(tagName);
    }
  };

  CustomElementRegistry.prototype.get = function (name) {
    return customElementRegistry.get(String(name).toLowerCase()) || undefined;
  };

  CustomElementRegistry.prototype.whenDefined = function (name) {
    const tagName = String(name).toLowerCase();
    if (customElementRegistry.has(tagName)) {
      return Promise.resolve(customElementRegistry.get(tagName));
    }
    return new Promise(function (resolve) {
      let list = whenDefinedResolvers.get(tagName);
      if (!list) {
        list = [];
        whenDefinedResolvers.set(tagName, list);
      }
      list.push(resolve);
    });
  };

  CustomElementRegistry.prototype.upgrade = function (root) {
    if (!root) return;
    for (const [tagName, ctor] of customElementRegistry.entries()) {
      if (root.querySelectorAll) {
        const matches = root.querySelectorAll(tagName);
        for (let i = 0; i < matches.length; i++) {
          upgradeCustomElement(matches[i], ctor, {
            tagName: tagName,
            fireConnectedCallback: Boolean(matches[i].isConnected),
          });
        }
      }
    }
  };

  markNative(CustomElementRegistry.prototype.define, "define");
  markNative(CustomElementRegistry.prototype.get, "get");
  markNative(CustomElementRegistry.prototype.whenDefined, "whenDefined");
  markNative(CustomElementRegistry.prototype.upgrade, "upgrade");
  markNative(CustomElementRegistry, "CustomElementRegistry");

  const customElementsInstance = new CustomElementRegistry();
  global.CustomElementRegistry = CustomElementRegistry;
  let currentCustomElements = customElementsInstance;

  function defineCustomElementsProp(obj) {
    if (!obj) return;
    try {
      Object.defineProperty(obj, "CustomElementRegistry", {
        value: CustomElementRegistry,
        writable: true,
        configurable: true,
        enumerable: true,
      });
      Object.defineProperty(obj, "customElements", {
        get: function () { return currentCustomElements; },
        set: function (val) {
          if (val && val instanceof CustomElementRegistry) {
            currentCustomElements = val;
          }
        },
        configurable: true,
        enumerable: true,
      });
    } catch (e) {}
  }

  defineCustomElementsProp(global);
  if (typeof globalThis !== "undefined") {
    defineCustomElementsProp(globalThis);
    globalThis.notifyCustomElementAttributeChanged = notifyCustomElementAttributeChanged;
    globalThis.upgradeCustomElement = upgradeCustomElement;
  }
  if (typeof window !== "undefined") {
    defineCustomElementsProp(window);
  }

  // =============================================================
  // Module: index.js
  // =============================================================
  // src/js/shim/index.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/index.ts
  // Entry point for CapSurf Web API Runtime Shim.

  function installShim(global) {
    // 1. Expose Event classes
    global.Event = Event;
    global.CustomEvent = CustomEvent;
    global.UIEvent = UIEvent;
    global.MouseEvent = MouseEvent;
    global.KeyboardEvent = KeyboardEvent;
    global.FocusEvent = FocusEvent;
    global.InputEvent = InputEvent;
    global.MessageEvent = MessageEvent;
    global.SubmitEvent = SubmitEvent;
    global.EventTarget = EventTarget;
    global.DeviceMotionEvent = DeviceMotionEvent;
    global.DeviceMotionEventAcceleration = DeviceMotionEventAcceleration;
    global.DeviceMotionEventRotationRate = DeviceMotionEventRotationRate;
    global.DeviceOrientationEvent = DeviceOrientationEvent;
    global.PromiseRejectionEvent = PromiseRejectionEvent;
    global.WheelEvent = WheelEvent;
    global.PointerEvent = PointerEvent;
    global.DragEvent = DragEvent;
    global.TextEvent = TextEvent;
    global.CompositionEvent = CompositionEvent;
    global.StorageEvent = StorageEvent;
    global.ErrorEvent = ErrorEvent;
    global.HashChangeEvent = HashChangeEvent;
    global.PopStateEvent = PopStateEvent;
    global.PageTransitionEvent = PageTransitionEvent;
    global.TransitionEvent = TransitionEvent;
    global.AnimationEvent = AnimationEvent;
    global.BeforeUnloadEvent = BeforeUnloadEvent;

    if (typeof globalThis !== "undefined") {
      globalThis.Event = Event;
      globalThis.CustomEvent = CustomEvent;
      globalThis.UIEvent = UIEvent;
      globalThis.MouseEvent = MouseEvent;
      globalThis.KeyboardEvent = KeyboardEvent;
      globalThis.FocusEvent = FocusEvent;
      globalThis.InputEvent = InputEvent;
      globalThis.MessageEvent = MessageEvent;
      globalThis.SubmitEvent = SubmitEvent;
      globalThis.EventTarget = EventTarget;
      globalThis.DeviceMotionEvent = DeviceMotionEvent;
      globalThis.DeviceMotionEventAcceleration = DeviceMotionEventAcceleration;
      globalThis.DeviceMotionEventRotationRate = DeviceMotionEventRotationRate;
      globalThis.DeviceOrientationEvent = DeviceOrientationEvent;
      globalThis.PromiseRejectionEvent = PromiseRejectionEvent;
      globalThis.WheelEvent = WheelEvent;
      globalThis.PointerEvent = PointerEvent;
      globalThis.DragEvent = DragEvent;
      globalThis.TextEvent = TextEvent;
      globalThis.CompositionEvent = CompositionEvent;
      globalThis.StorageEvent = StorageEvent;
      globalThis.ErrorEvent = ErrorEvent;
      globalThis.HashChangeEvent = HashChangeEvent;
      globalThis.PopStateEvent = PopStateEvent;
      globalThis.PageTransitionEvent = PageTransitionEvent;
      globalThis.TransitionEvent = TransitionEvent;
      globalThis.AnimationEvent = AnimationEvent;
      globalThis.BeforeUnloadEvent = BeforeUnloadEvent;
    }

    // 2. Expose DOM Collections and Nodes
    global.DOMTokenList = DOMTokenList;
    global.HTMLCollection = HTMLCollection;
    global.HTMLOptionsCollection = HTMLOptionsCollection;
    global.NodeList = NodeList;
    global.NamedNodeMap = NamedNodeMap;
    global.Attr = Attr;
    global.Node = Node;
    global.Text = Text;
    global.Comment = Comment;
    global.DocumentFragment = DocumentFragment;

    // 3. Expose Mutation Observer
    global.MutationObserver = MutationObserver;
    global.MutationRecord = MutationRecord;

    // 4. Expose FormData, Blob and File
    global.FormData = FormData;
    global.Blob = Blob;
    global.File = File;
    global.FileReader = FileReader;
    if (typeof globalThis !== "undefined") globalThis.FileReader = FileReader;
    if (typeof Headers !== "undefined") global.Headers = Headers;
    if (typeof Request !== "undefined") global.Request = Request;
    if (typeof Response !== "undefined") global.Response = Response;

    // 5. Expose Traversal
    global.NodeFilter = NodeFilter;
    global.TreeWalker = TreeWalker;
    global.NodeIterator = NodeIterator;

    // 6. Expose Range and Selection
    global.Range = Range;
    global.Selection = Selection;

    // 7. Expose DOMParser
    global.DOMParser = DOMParser;

    // 7.5. Expose XPath
    if (typeof XPathResult !== "undefined") {
      global.XPathResult = XPathResult;
      global.XPathExpression = XPathExpression;
      global.XPathEvaluator = XPathEvaluator;
    }

    // 8. Expose Canvas 2D
    global.CanvasRenderingContext2D = CanvasRenderingContext2D;
    global.CanvasGradient = CanvasGradient;
    global.CanvasPattern = CanvasPattern;
    global.Path2D = Path2D;
    global.ImageData = ImageData;
    global.TextMetrics = TextMetrics;

    // 9. Expose Web Animations
    global.Animation = Animation;
    global.KeyframeEffect = KeyframeEffect;
    global.AnimationTimeline = AnimationTimeline;
    global.DocumentTimeline = DocumentTimeline;

    // 10. Install Element & HTML extensions
    if (global.Element) {
      installElementExtensions(global.Element.prototype);
      installHtmlElementExtensions(global.Element.prototype);

      global.Element.prototype.animate = markNative(function (keyframes, options) {
        return animateElement(this, keyframes, options);
      }, "animate");

      global.Element.prototype.getAnimations = markNative(function () {
        return animationsFor(this);
      }, "getAnimations");

      global.Element.prototype.getContext = markNative(function (type) {
        const t = String(type).toLowerCase();
        if (t === "webgl" || t === "webgl2" || t === "experimental-webgl") {
          return null;
        }
        if (t === "2d") {
          if (!this._ctx2d) {
            this._ctx2d = new CanvasRenderingContext2D(this, _INTERNAL_CTX_SECRET);
          }
          return this._ctx2d;
        }
        return null;
      }, "getContext");

      // Add selector methods
      global.Element.prototype.querySelector = wrapQuerySelector(function(sel) {
          const all = document.querySelectorAll(sel);
          for(let i = 0; i < all.length; i++) {
              if (this !== all[i] && this.contains(all[i])) return all[i];
          }
          return null;
      });
      global.Element.prototype.querySelectorAll = wrapQuerySelectorAll(function(sel) {
          const all = document.querySelectorAll(sel);
          const matches = [];
          for(let i = 0; i < all.length; i++) {
              if (this !== all[i] && this.contains(all[i])) matches.push(all[i]);
          }
          return matches;
      });

      global.Element.prototype.toDataURL = markNative(function () {
        return VALID_PNG_BASE64;
      }, "toDataURL");
    }

    // 11. Install specialized elements and forms
    installHtmlClasses(global);
    installInputElement(global);
    installTextAreaElement(global);
    installSelectElement(global);
    installButtonElement(global);
    installFormElement(global);
    if (typeof installDialogElement === "function") {
      installDialogElement(global);
    }

    // 12. Install window environment
    installWindow(global);

    function upgradeElement(el) {
      if (!el || typeof el !== "object" || el._upgraded) return el;
      el._upgraded = true;
      if (typeof global.__capsurf_recordCanonicalId === "function") {
        global.__capsurf_recordCanonicalId(el);
      }
      const tag = String(el.localName || el.tagName || "").toLowerCase();
      switch (tag) {
        case "input": Object.setPrototypeOf(el, global.HTMLInputElement.prototype); break;
        case "textarea": Object.setPrototypeOf(el, global.HTMLTextAreaElement.prototype); break;
        case "select": Object.setPrototypeOf(el, global.HTMLSelectElement.prototype); break;
        case "option": Object.setPrototypeOf(el, global.HTMLOptionElement.prototype); break;
        case "button": Object.setPrototypeOf(el, global.HTMLButtonElement.prototype); break;
        case "form": Object.setPrototypeOf(el, global.HTMLFormElement.prototype); break;
        case "a": Object.setPrototypeOf(el, global.HTMLAnchorElement.prototype); break;
        case "area": Object.setPrototypeOf(el, global.HTMLAreaElement.prototype); break;
        case "img": Object.setPrototypeOf(el, global.HTMLImageElement.prototype); break;
        case "canvas": Object.setPrototypeOf(el, global.HTMLCanvasElement.prototype); break;
        case "script": Object.setPrototypeOf(el, global.HTMLScriptElement.prototype); break;
        case "iframe": Object.setPrototypeOf(el, global.HTMLIFrameElement.prototype); break;
        case "template": Object.setPrototypeOf(el, global.HTMLTemplateElement.prototype); break;
        case "dialog": Object.setPrototypeOf(el, global.HTMLDialogElement ? global.HTMLDialogElement.prototype : global.HTMLElement.prototype); break;
        default: Object.setPrototypeOf(el, global.HTMLElement.prototype); break;
      }
      return el;
    }
    global.upgradeElement = upgradeElement;
    if (typeof globalThis !== "undefined") {
      globalThis.upgradeElement = upgradeElement;
    }

    // 13. Enhance Document prototype and instance
    if (global.Document && global.Document.prototype) {
      const docProto = global.Document.prototype;

      // Inherit from Node
      Object.setPrototypeOf(docProto, Node.prototype);

      const origQS = docProto.querySelector;
      docProto.querySelector = wrapQuerySelector(origQS);
      const origQSA = docProto.querySelectorAll;
      docProto.querySelectorAll = wrapQuerySelectorAll(origQSA);

      if (docProto.createElement) {
        const origCreateElement = docProto.createElement;
        docProto.createElement = function (tagName) {
          let el = origCreateElement.call(this, tagName);
          if (el) {
            const tag = String(tagName).toLowerCase();
            const registry = global.customElements || (typeof customElements !== "undefined" ? customElements : null);
            const Ctor = (registry && registry.get) ? registry.get(tag) : null;
            if (Ctor && typeof Ctor === "function" && !Ctor.prototype.isPrototypeOf(el)) {
              el._upgraded = true;
              if (typeof globalThis.upgradeCustomElement === "function") {
                globalThis.upgradeCustomElement(el, Ctor, { tagName: tag });
              } else {
                Object.setPrototypeOf(el, Ctor.prototype);
                const HEl = global.HTMLElement || (typeof HTMLElement !== "undefined" ? HTMLElement : null);
                if (HEl) HEl._upgradeTarget = el;
                try { new Ctor(); } catch (e) {} finally { if (HEl) HEl._upgradeTarget = null; }
              }
            }
          }
          el = upgradeElement(el);
          if (el && String(tagName).toLowerCase() === "script") {
            if (typeof globalThis !== "undefined" && globalThis.__capsurf_markScriptForceAsync) {
              globalThis.__capsurf_markScriptForceAsync(el);
            }
          }
          return el;
        };
        markNative(docProto.createElement, "createElement");
        if (global.document) {
          global.document.createElement = docProto.createElement;
        }
      }

      const origCreateElement = docProto.createElement;
      const origCreateElementNS = docProto.createElementNS || function (ns, tagName) {
        let el = origCreateElement ? origCreateElement.call(this, tagName) : null;
        if (!el) {
          el = Object.create(global.Element ? global.Element.prototype : Object.prototype);
          el.localName = String(tagName).toLowerCase();
          el.tagName = String(tagName).toUpperCase();
        }
        return el;
      };
      docProto.createElementNS = function (ns, tagName) {
        let el = origCreateElementNS.call(this, ns, tagName);
        if (el) {
          el.namespaceURI = ns !== undefined && ns !== null ? String(ns) : null;
        }
        if (el && (!ns || ns === "http://www.w3.org/1999/xhtml")) {
          const tag = String(tagName).toLowerCase();
          const registry = global.customElements || (typeof customElements !== "undefined" ? customElements : null);
          const Ctor = (registry && registry.get) ? registry.get(tag) : null;
          if (Ctor && typeof Ctor === "function" && !Ctor.prototype.isPrototypeOf(el)) {
            el._upgraded = true;
            if (typeof globalThis.upgradeCustomElement === "function") {
              globalThis.upgradeCustomElement(el, Ctor, { tagName: tag });
            } else {
              Object.setPrototypeOf(el, Ctor.prototype);
              const HEl = global.HTMLElement || (typeof HTMLElement !== "undefined" ? HTMLElement : null);
              if (HEl) HEl._upgradeTarget = el;
              try { new Ctor(); } catch (e) {} finally { if (HEl) HEl._upgradeTarget = null; }
            }
          }
        }
        el = upgradeElement(el);
        if (el && String(tagName).toLowerCase() === "script") {
          if (typeof globalThis !== "undefined" && globalThis.__capsurf_markScriptForceAsync) {
            globalThis.__capsurf_markScriptForceAsync(el);
          }
        }
        return el;
      };
      markNative(docProto.createElementNS, "createElementNS");
      if (global.document) {
        global.document.createElementNS = docProto.createElementNS;
      }

      if (docProto.getElementById) {
        const origGetElementById = docProto.getElementById;
        docProto.getElementById = function (id) {
          const el = upgradeElement(origGetElementById.call(this, id));
          if (el && typeof global.__capsurf_setLastQueriedNode === "function") {
            global.__capsurf_setLastQueriedNode(el);
          }
          return el;
        };
        markNative(docProto.getElementById, "getElementById");
      }

      if (docProto.querySelector) {
        const origQuerySelector = docProto.querySelector;
        docProto.querySelector = function (sel) {
          const el = upgradeElement(origQuerySelector.call(this, sel));
          if (el && typeof global.__capsurf_setLastQueriedNode === "function") {
            global.__capsurf_setLastQueriedNode(el);
          }
          return el;
        };
        markNative(docProto.querySelector, "querySelector");
      }

      if (docProto.querySelectorAll) {
        const origQuerySelectorAll = docProto.querySelectorAll;
        docProto.querySelectorAll = function (sel) {
          const nodes = origQuerySelectorAll.call(this, sel);
          if (nodes && nodes.length) {
            for (let i = 0; i < nodes.length; i++) upgradeElement(nodes[i]);
          }
          return nodes;
        };
        markNative(docProto.querySelectorAll, "querySelectorAll");
      }


      docProto.createRange = markNative(function () {
        return new Range();
      }, "createRange");

      docProto.createTreeWalker = markNative(function (root, whatToShow, filter) {
        return new TreeWalker(root, whatToShow, filter);
      }, "createTreeWalker");

      docProto.createNodeIterator = markNative(function (root, whatToShow, filter) {
        return new NodeIterator(root, whatToShow, filter);
      }, "createNodeIterator");

      docProto.getSelection = markNative(function () {
        if (!this._selection) this._selection = new Selection();
        return this._selection;
      }, "getSelection");

      docProto.getAnimations = markNative(function () {
        return animationsFor(this);
      }, "getAnimations");

      if (!docProto.children) {
        Object.defineProperty(docProto, "children", {
          get: function () {
            return getElementChildrenCollection(this);
          },
          configurable: true,
        });
      }
      if (!docProto.firstElementChild) {
        Object.defineProperty(docProto, "firstElementChild", {
          get: function () {
            const kids = this.children;
            return (kids && kids.length > 0) ? kids[0] : null;
          },
          configurable: true,
        });
      }
      if (!docProto.lastElementChild) {
        Object.defineProperty(docProto, "lastElementChild", {
          get: function () {
            const kids = this.children;
            return (kids && kids.length > 0) ? kids[kids.length - 1] : null;
          },
          configurable: true,
        });
      }
      if (!docProto.childElementCount) {
        Object.defineProperty(docProto, "childElementCount", {
          get: function () {
            const kids = this.children;
            return kids ? kids.length : 0;
          },
          configurable: true,
        });
      }

      function defineLiveCollection(name, pred) {
        Object.defineProperty(docProto, name, {
          get: function () {
            const doc = this;
            return makeHtmlCollection(function () {
              return pred ? getAllDocumentElements(doc).filter(pred) : [];
            });
          },
          configurable: true,
        });
      }
      function tagIs() {
        const names = arguments;
        return function (el) {
          const t = String(el.localName || el.tagName || "").toLowerCase();
          for (let i = 0; i < names.length; i++) if (t === names[i]) return true;
          return false;
        };
      }
      const isEmbed = tagIs("embed");
      defineLiveCollection("images", tagIs("img"));
      defineLiveCollection("forms", tagIs("form"));
      defineLiveCollection("scripts", tagIs("script"));
      defineLiveCollection("embeds", isEmbed);
      defineLiveCollection("plugins", isEmbed);
      defineLiveCollection("links", function (el) {
        return tagIs("a", "area")(el) && el.hasAttribute("href");
      });
      defineLiveCollection("anchors", function (el) {
        return tagIs("a")(el) && el.hasAttribute("name");
      });
      defineLiveCollection("applets", null);

      if (!Object.getOwnPropertyDescriptor(docProto, "title")) {
        let docTitle = "";
        Object.defineProperty(docProto, "title", {
          get: function () {
            const el = this.querySelector ? this.querySelector("title") : null;
            return el ? (el.textContent || "") : docTitle;
          },
          set: function (v) {
            docTitle = String(v);
            const el = this.querySelector ? this.querySelector("title") : null;
            if (el) el.textContent = docTitle;
          },
          configurable: true,
        });
      }

      Object.defineProperty(docProto, "timeline", {
        get: function () { return globalDocTimeline; },
        configurable: true,
      });

      Object.defineProperty(docProto, "defaultView", {
        get: function () { return global; },
        configurable: true,
      });

      let docCookie = "";
      function clearDocCookie() {
        docCookie = "";
      }
      global.__capsurf_clear_doc_cookie = clearDocCookie;
      if (typeof globalThis !== "undefined") {
        globalThis.__capsurf_clear_doc_cookie = clearDocCookie;
      }

      function getDocUrl(doc) {
        if (doc && doc.URL) return String(doc.URL);
        if (global && global.document && global.document.URL) return String(global.document.URL);
        if (typeof globalThis !== "undefined" && globalThis.document && globalThis.document.URL) return String(globalThis.document.URL);
        if (global && global.location && global.location.href) return String(global.location.href);
        if (typeof globalThis !== "undefined" && globalThis.location && globalThis.location.href) return String(globalThis.location.href);
        return "about:blank";
      }

      Object.defineProperty(docProto, "cookie", {
        get: function () {
          const docUrl = getDocUrl(this);
          const getFn = (typeof __capsurf_get_cookie_header === "function") ? __capsurf_get_cookie_header : ((typeof globalThis !== "undefined" && typeof globalThis.__capsurf_get_cookie_header === "function") ? globalThis.__capsurf_get_cookie_header : null);
          if (getFn) {
            const hdr = getFn(docUrl);
            if (hdr !== null && hdr !== undefined) {
              return String(hdr);
            }
          }
          if (docUrl.startsWith("http://") || docUrl.startsWith("https://")) {
            return "";
          }
          return docCookie;
        },
        set: function (v) {
          const str = String(v || "").trim();
          if (!str) return;
          const docUrl = getDocUrl(this);
          const setFn = (typeof __capsurf_set_cookie === "function") ? __capsurf_set_cookie : ((typeof globalThis !== "undefined" && typeof globalThis.__capsurf_set_cookie === "function") ? globalThis.__capsurf_set_cookie : null);
          if (setFn) {
            setFn(docUrl, str);
          }
          const pair = str.split(";")[0].trim();
          const eq = pair.indexOf("=");
          if (eq !== -1) {
            const k = pair.slice(0, eq).trim();
            const val = pair.slice(eq + 1).trim();
            const cookies = docCookie ? docCookie.split("; ").filter(function (c) { return !c.startsWith(k + "="); }) : [];
            cookies.push(k + "=" + val);
            docCookie = cookies.join("; ");
          }
        },
        configurable: true,
      });

      if (typeof evaluateXPath !== "undefined") {
        docProto.evaluate = markNative(function (expression, contextNode, resolver, type, result) {
          return evaluateXPath(expression, contextNode || this, resolver, type, result);
        }, "evaluate");
        docProto.createExpression = markNative(function (expression, resolver) {
          return createExpression(expression, resolver);
        }, "createExpression");
        docProto.createNSResolver = markNative(function (nodeResolver) {
          return nodeResolver;
        }, "createNSResolver");
      }
    }

    if (global.document) {
      if (!global.document.location) global.document.location = global.location;
      if (!global.document.createRange) global.document.createRange = function () { return new Range(); };
      if (!global.document.getSelection) global.document.getSelection = function () {
        if (!global.document._selection) global.document._selection = new Selection();
        return global.document._selection;
      };
      if (typeof evaluateXPath !== "undefined") {
        if (!global.document.evaluate) global.document.evaluate = docProto.evaluate.bind(global.document);
        if (!global.document.createExpression) global.document.createExpression = docProto.createExpression.bind(global.document);
        if (!global.document.createNSResolver) global.document.createNSResolver = docProto.createNSResolver.bind(global.document);
      }
      if (typeof processSubtreeTemplates === "function") {
        processSubtreeTemplates(global.document);
      }
    }

    // 14. Mount window EventTarget methods
    global.addEventListener = EventTarget.prototype.addEventListener.bind(global);
    global.removeEventListener = EventTarget.prototype.removeEventListener.bind(global);
    global.dispatchEvent = EventTarget.prototype.dispatchEvent.bind(global);
    global.getSelection = function () {
      if (global.document && global.document.getSelection) return global.document.getSelection();
      return new Selection();
    };

    // 15. Kitesurf page operations support (getPageState, setControlValue)
    if (typeof global.__treeSeq === "undefined") {
      global.__treeSeq = 1;
    }
    if (typeof globalThis !== "undefined" && typeof globalThis.__treeSeq === "undefined") {
      globalThis.__treeSeq = 1;
    }

    function setControlValueAsUser(control, value) {
      const tag = String(control?.localName || control?.tagName || "").toLowerCase();
      const type = String(control?.type || "").toLowerCase();
      const toggle = tag === "input" && (type === "checkbox" || type === "radio");
      const NO_USER_VALUE = new Set(["hidden", "file", "button", "submit", "reset", "image"]);
      const entry = tag === "textarea" || (tag === "input" && !toggle && !NO_USER_VALUE.has(type));
      if (tag !== "select" && !toggle && !entry) {
        return { changed: false };
      }
      const isDisabled = Boolean(control.disabled || (control.hasAttribute && control.hasAttribute("disabled")));
      if (isDisabled) {
        return { changed: false, error: "control is disabled" };
      }
      const isReadOnly = Boolean(control.readOnly || (control.hasAttribute && control.hasAttribute("readonly")));
      if (entry && isReadOnly) {
        return { changed: false, error: "control is read-only" };
      }
      if (tag === "select") {
        const wanted = control.multiple === true && Array.isArray(value)
          ? value.map(String)
          : [value == null ? "" : String(value)];
        const options = Array.from(control.options || []);
        const optionEnabled = (opt) => {
          if (opt.disabled || (opt.hasAttribute && opt.hasAttribute("disabled"))) return false;
          const group = opt.parentNode;
          if (group && String(group.localName || group.tagName || "").toLowerCase() === "optgroup") {
            if (group.disabled || (group.hasAttribute && group.hasAttribute("disabled"))) return false;
          }
          return true;
        };
        const offered = new Set(options.filter(optionEnabled).map((o) => String(o.value ?? "")));
        if (!wanted.every((w) => offered.has(w))) {
          return { changed: false, error: "no such option" };
        }
      }

      let changed = false;
      if (type === "checkbox") {
        const want = value === true;
        if (control.checked !== want) {
          control.checked = want;
          changed = true;
        }
      } else if (type === "radio") {
        const want = String(control.value || "on") === String(value);
        if (control.checked !== want) {
          control.checked = want;
          changed = true;
        }
      } else if (tag === "select" && control.multiple === true && Array.isArray(value)) {
        const selected = new Set(value.map(String));
        const options = control.options || [];
        for (let j = 0; j < options.length; j++) {
          const opt = options[j];
          const want = selected.has(String(opt.value));
          if (opt.selected !== want) {
            opt.selected = want;
            changed = true;
          }
        }
      } else {
        const want = value == null ? "" : String(value);
        if (String(control.value || "") !== want) {
          control.value = want;
          changed = true;
        }
      }

      if (changed) {
        try {
          const inputEvt = typeof InputEvent !== "undefined"
            ? new InputEvent("input", { bubbles: true, cancelable: false })
            : new Event("input", { bubbles: true, cancelable: false });
          if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
            global.__capsurf_dispatchTrustedEvent(control, inputEvt);
          } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_dispatchTrustedEvent === "function") {
            globalThis.__capsurf_dispatchTrustedEvent(control, inputEvt);
          } else if (control.dispatchEvent) {
            control.dispatchEvent(inputEvt);
          }
        } catch (e) {}
        try {
          const changeEvt = new Event("change", { bubbles: true, cancelable: false });
          if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
            global.__capsurf_dispatchTrustedEvent(control, changeEvt);
          } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_dispatchTrustedEvent === "function") {
            globalThis.__capsurf_dispatchTrustedEvent(control, changeEvt);
          } else if (control.dispatchEvent) {
            control.dispatchEvent(changeEvt);
          }
        } catch (e) {}
      }
      return { changed: changed, value: control.value !== undefined ? control.value : "" };
    }

    function findNodeById(nodeId) {
      let el = null;
      if (typeof global.__capsurf_native_find_node_by_id === "function") {
        el = global.__capsurf_native_find_node_by_id(nodeId);
      } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_native_find_node_by_id === "function") {
        el = globalThis.__capsurf_native_find_node_by_id(nodeId);
      }
      if (el && typeof upgradeElement === "function") {
        upgradeElement(el);
      }
      return el;
    }

    global.__capsurf_find_node_by_id = findNodeById;
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_find_node_by_id = findNodeById;
    }

    function setControlValueJson(paramsJson) {
      let params;
      try {
        params = JSON.parse(paramsJson);
      } catch (e) {
        return JSON.stringify({ changed: false, error: "bad nodeId" });
      }
      const inner = (params && typeof params.params === "object" && params.params !== null) ? params.params : params;
      const nodeId = inner.nodeId;
      if (typeof nodeId !== "number" || !(nodeId > 0)) {
        return JSON.stringify({ changed: false, error: "bad nodeId" });
      }
      const el = findNodeById(nodeId);
      if (!el) {
        return JSON.stringify({ changed: false, error: "bad nodeId" });
      }
      return JSON.stringify(setControlValueAsUser(el, inner.value));
    }

    global.__capsurf_setControlValueJson = setControlValueJson;
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_setControlValueJson = setControlValueJson;
    }
  }

  // Auto-install on globalThis/window
  installShim(typeof window !== "undefined" ? window : globalThis);

})(typeof window !== "undefined" ? window : globalThis);
