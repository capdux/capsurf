// src/js/shim/events/event.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/events/event.ts

function Event(type, options) {
  if (!type) throw new TypeError("Event type required");
  const opts = options || {};
  this.type = String(type);
  this.bubbles = Boolean(opts.bubbles);
  this.cancelable = Boolean(opts.cancelable);
  this.composed = Boolean(opts.composed);
  this.target = null;
  this.currentTarget = null;
  this.eventPhase = 0; // 0: NONE, 1: CAPTURING, 2: AT_TARGET, 3: BUBBLING
  this.defaultPrevented = false;
  this.isTrusted = true;
  this.timeStamp = typeof performance !== "undefined" && performance.now ? performance.now() : Date.now();
  this._stopped = false;
  this._immediateStopped = false;
}
Event.NONE = 0;
Event.CAPTURING_PHASE = 1;
Event.AT_TARGET = 2;
Event.BUBBLING_PHASE = 3;

Event.prototype.stopPropagation = function () {
  this._stopped = true;
};
Event.prototype.stopImmediatePropagation = function () {
  this._stopped = true;
  this._immediateStopped = true;
};
Event.prototype.preventDefault = function () {
  if (this.cancelable) {
    this.defaultPrevented = true;
  }
};
markNative(Event, "Event");
markNative(Event.prototype.stopPropagation, "stopPropagation");
markNative(Event.prototype.stopImmediatePropagation, "stopImmediatePropagation");
markNative(Event.prototype.preventDefault, "preventDefault");

function CustomEvent(type, options) {
  Event.call(this, type, options);
  const opts = options || {};
  this.detail = opts.detail !== undefined ? opts.detail : null;
}
CustomEvent.prototype = Object.create(Event.prototype);
CustomEvent.prototype.constructor = CustomEvent;
markNative(CustomEvent, "CustomEvent");

function UIEvent(type, options) {
  Event.call(this, type, options);
  const opts = options || {};
  this.view = opts.view !== undefined ? opts.view : (typeof window !== "undefined" ? window : null);
  this.detail = opts.detail !== undefined ? Number(opts.detail) : 0;
}
UIEvent.prototype = Object.create(Event.prototype);
UIEvent.prototype.constructor = UIEvent;
markNative(UIEvent, "UIEvent");

function MouseEvent(type, options) {
  UIEvent.call(this, type, options);
  const opts = options || {};
  this.screenX = Number(opts.screenX || 0);
  this.screenY = Number(opts.screenY || 0);
  this.clientX = Number(opts.clientX || 0);
  this.clientY = Number(opts.clientY || 0);
  this.pageX = Number(opts.pageX || this.clientX);
  this.pageY = Number(opts.pageY || this.clientY);
  this.offsetX = Number(opts.offsetX || this.clientX);
  this.offsetY = Number(opts.offsetY || this.clientY);
  this.ctrlKey = Boolean(opts.ctrlKey);
  this.shiftKey = Boolean(opts.shiftKey);
  this.altKey = Boolean(opts.altKey);
  this.metaKey = Boolean(opts.metaKey);
  this.button = Number(opts.button || 0);
  this.buttons = Number(opts.buttons || (this.button === 0 ? 1 : 0));
  this.relatedTarget = opts.relatedTarget || null;
}
MouseEvent.prototype = Object.create(UIEvent.prototype);
MouseEvent.prototype.constructor = MouseEvent;
markNative(MouseEvent, "MouseEvent");

function KeyboardEvent(type, options) {
  UIEvent.call(this, type, options);
  const opts = options || {};
  this.key = String(opts.key || "");
  this.code = String(opts.code || "");
  this.location = Number(opts.location || 0);
  this.ctrlKey = Boolean(opts.ctrlKey);
  this.shiftKey = Boolean(opts.shiftKey);
  this.altKey = Boolean(opts.altKey);
  this.metaKey = Boolean(opts.metaKey);
  this.repeat = Boolean(opts.repeat);
  this.isComposing = Boolean(opts.isComposing);
  this.charCode = Number(opts.charCode || (this.key.length === 1 ? this.key.charCodeAt(0) : 0));
  this.keyCode = Number(opts.keyCode || (this.charCode || 0));
  this.which = Number(opts.which || this.keyCode);
}
KeyboardEvent.prototype = Object.create(UIEvent.prototype);
KeyboardEvent.prototype.constructor = KeyboardEvent;
markNative(KeyboardEvent, "KeyboardEvent");

function FocusEvent(type, options) {
  UIEvent.call(this, type, options);
  const opts = options || {};
  this.relatedTarget = opts.relatedTarget || null;
}
FocusEvent.prototype = Object.create(UIEvent.prototype);
FocusEvent.prototype.constructor = FocusEvent;
markNative(FocusEvent, "FocusEvent");

function InputEvent(type, options) {
  UIEvent.call(this, type, options);
  const opts = options || {};
  this.data = opts.data !== undefined ? String(opts.data) : null;
  this.isComposing = Boolean(opts.isComposing);
  this.inputType = String(opts.inputType || "insertText");
  this.dataTransfer = opts.dataTransfer || null;
}
InputEvent.prototype = Object.create(UIEvent.prototype);
InputEvent.prototype.constructor = InputEvent;
markNative(InputEvent, "InputEvent");

function MessageEvent(type, options) {
  Event.call(this, type, options);
  const opts = options || {};
  this.data = opts.data !== undefined ? opts.data : null;
  this.origin = opts.origin !== undefined ? String(opts.origin) : "";
  this.lastEventId = opts.lastEventId !== undefined ? String(opts.lastEventId) : "";
  this.source = opts.source !== undefined ? opts.source : null;
  this.ports = Array.isArray(opts.ports) ? Object.freeze(opts.ports.slice()) : Object.freeze([]);
}
MessageEvent.prototype = Object.create(Event.prototype);
MessageEvent.prototype.constructor = MessageEvent;
markNative(MessageEvent, "MessageEvent");
