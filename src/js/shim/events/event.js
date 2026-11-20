// src/js/shim/events/event.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/events/event.ts

const _eventState = new WeakMap();
const _trustedFlags = new WeakMap();
function _setEventTrusted(ev, val) {
  if (ev && typeof ev === "object") {
    _trustedFlags.set(ev, Boolean(val));
  }
}
const _customEventState = new WeakMap();
const _uiEventState = new WeakMap();
const _mouseEventState = new WeakMap();
const _keyboardEventState = new WeakMap();
const _focusEventState = new WeakMap();
const _inputEventState = new WeakMap();
const _messageEventState = new WeakMap();
const _submitEventState = new WeakMap();
const _formDataEventState = new WeakMap();
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
  if (!s || !s.path || s.path.length === 0) return [];
  if (typeof global.__capsurf_filterComposedPath === "function") {
    return global.__capsurf_filterComposedPath(s.path, s.currentTarget);
  }
  return s.path.slice();
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
  } else if (type === "keypress" && key === "Enter") {
    charCode = 13;
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
// FormDataEvent
// -------------------------------------------------------------
function FormDataEvent(type, options) {
  if (arguments.length < 2 || !options || typeof options !== "object" || !(options.formData instanceof FormData)) {
    throw new TypeError("Failed to construct 'FormDataEvent': Failed to read the 'formData' property from 'FormDataEventInit': Required member is undefined or not a FormData.");
  }
  Event.call(this, type, options);
  _formDataEventState.set(this, {
    formData: options.formData,
  });
}
FormDataEvent.prototype = Object.create(Event.prototype);
FormDataEvent.prototype.constructor = FormDataEvent;
markNative(FormDataEvent, "FormDataEvent");
Object.defineProperty(FormDataEvent.prototype, Symbol.toStringTag, {
  value: "FormDataEvent",
  configurable: true,
});
Object.defineProperty(FormDataEvent.prototype, "formData", {
  get: function () {
    const s = _formDataEventState.get(this);
    if (!s) throw new TypeError("Illegal invocation");
    return s.formData;
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
  if (s.coalesced.length > 0) return s.coalesced.slice();
  const es = _eventState.get(this);
  const evType = es ? es.type : this.type;
  const isTrusted = Boolean(this.isTrusted);
  if (isTrusted && (evType === "pointermove" || evType === "pointerrawupdate")) {
    return [this];
  }
  return [];
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
