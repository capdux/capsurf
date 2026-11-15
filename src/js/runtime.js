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

  EventTarget.prototype.dispatchEvent = function (event) {
    if (!event || typeof event.type !== "string") {
      throw new TypeError("Failed to execute 'dispatchEvent': parameter 1 is not of type 'Event'.");
    }
    event.target = this;
    const eventType = event.type.toLowerCase();

    // 1. Build propagation path from target up to window
    const path = [];
    let curr = this;
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
            if (phase === Event.CAPTURING_PHASE && !item.capture) continue;
            if (phase === Event.BUBBLING_PHASE && item.capture) continue;

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

      // Inline event handler (e.g. element.onclick)
      if (phase === Event.AT_TARGET || phase === Event.BUBBLING_PHASE) {
        const propName = "on" + eventType;
        if (typeof entry[propName] === "function" && !event._immediateStopped) {
          try {
            entry[propName].call(entry, event);
          } catch (e) {
            if (typeof console !== "undefined" && console.error) console.error(e);
          }
        }
      }
    }

    // Phase 1: Capturing (from top down to target's parent)
    for (let i = path.length - 1; i >= 1; i--) {
      if (event._stopped) break;
      invoke(path[i], Event.CAPTURING_PHASE);
    }

    // Phase 2: At Target
    if (!event._stopped) {
      invoke(this, Event.AT_TARGET);
    }

    // Phase 3: Bubbling (from target's parent up to top, if bubbles is true)
    if (event.bubbles && !event._stopped) {
      for (let i = 1; i < path.length; i++) {
        if (event._stopped) break;
        invoke(path[i], Event.BUBBLING_PHASE);
      }
    }

    event.eventPhase = Event.NONE;
    event.currentTarget = null;
    return !event.defaultPrevented;
  };

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

  function notifyMutation(record) {
    for (const obs of activeObservers) {
      for (const sub of obs._targets) {
        if (sub.target === record.target || (sub.options.subtree && sub.target.contains && sub.target.contains(record.target))) {
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
      if (this._targets[i].target === target) {
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

  function HTMLCollection(items) {
    const list = items || [];
    for (let i = 0; i < list.length; i++) {
      this[i] = list[i];
    }
    this.length = list.length;
  }
  HTMLCollection.prototype.item = function (index) {
    const i = Number(index);
    return (i >= 0 && i < this.length) ? this[i] : null;
  };
  HTMLCollection.prototype.namedItem = function (name) {
    for (let i = 0; i < this.length; i++) {
      const el = this[i];
      if (el.id === name || (el.getAttribute && el.getAttribute("name") === name)) {
        return el;
      }
    }
    return null;
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

  // =============================================================
  // Module: node-base.js
  // =============================================================
  // src/js/shim/node-base.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/node-base.ts

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
        curr = curr.parentNode || curr.parentElement;
      }
      return false;
    },
    configurable: true,
  });

  Object.defineProperty(Node.prototype, "ownerDocument", {
    get: function () {
      if (this.nodeType === DOCUMENT_NODE) return null;
      return typeof document !== "undefined" ? document : null;
    },
    configurable: true,
  });

  Node.prototype.contains = function (other) {
    if (!other) return false;
    let curr = other;
    while (curr) {
      if (curr === this) return true;
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

  markNative(Node, "Node");
  markNative(Node.prototype.contains, "contains");
  markNative(Node.prototype.compareDocumentPosition, "compareDocumentPosition");
  markNative(Node.prototype.cloneNode, "cloneNode");

  function Text(data) {
    Node.call(this);
    this.data = String(data !== undefined ? data : "");
    this.nodeType = TEXT_NODE;
    this.nodeName = "#text";
  }
  Text.prototype = Object.create(Node.prototype);
  Text.prototype.constructor = Text;
  Object.defineProperty(Text.prototype, "textContent", {
    get: function () { return this.data; },
    set: function (v) { this.data = String(v); },
    configurable: true,
  });
  Object.defineProperty(Text.prototype, "nodeValue", {
    get: function () { return this.data; },
    set: function (v) { this.data = String(v); },
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
    if (child) {
      this._children.push(child);
      child.parentNode = this;
    }
    return child;
  };
  Object.defineProperty(DocumentFragment.prototype, "childNodes", {
    get: function () {
      return new NodeList(this._children);
    },
    configurable: true,
  });
  markNative(DocumentFragment, "DocumentFragment");

  // =============================================================
  // Module: element/element.js
  // =============================================================
  // src/js/shim/element/element.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/element.ts

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
          const k = p.slice(0, colon).trim().toLowerCase();
          const v = p.slice(colon + 1).trim();
          map.set(k, v);
        }
      }
      return map;
    }

    function serializeStyleMap(map) {
      const arr = [];
      for (const [k, v] of map.entries()) {
        arr.push(k + ": " + v);
      }
      const res = arr.join("; ");
      if (res.length > 0) {
        element.setAttribute("style", res);
      } else {
        element.removeAttribute("style");
      }
    }

    return new Proxy({}, {
      get: function (target, prop) {
        if (typeof prop !== "string") return undefined;
        if (prop === "cssText") return element.getAttribute("style") || "";
        if (prop === "length") return parseStyleMap().size;
        if (prop === "getPropertyValue") {
          return function (p) {
            const map = parseStyleMap();
            return map.get(String(p).toLowerCase()) || "";
          };
        }
        if (prop === "setProperty") {
          return function (p, val) {
            const map = parseStyleMap();
            map.set(String(p).toLowerCase(), String(val));
            serializeStyleMap(map);
          };
        }
        if (prop === "removeProperty") {
          return function (p) {
            const map = parseStyleMap();
            const key = String(p).toLowerCase();
            const old = map.get(key) || "";
            map.delete(key);
            serializeStyleMap(map);
            return old;
          };
        }
        // CamelCase to kebab-case
        const kebab = prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
        const map = parseStyleMap();
        return map.get(kebab) || "";
      },
      set: function (target, prop, value) {
        if (typeof prop !== "string") return true;
        if (prop === "cssText") {
          element.setAttribute("style", String(value));
          return true;
        }
        const kebab = prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
        const map = parseStyleMap();
        if (value === null || value === undefined || value === "") {
          map.delete(kebab);
        } else {
          map.set(kebab, String(value));
        }
        serializeStyleMap(map);
        return true;
      }
    });
  }

  function installElementExtensions(proto) {
    if (!proto) return;

    // Make Element inherit from Node / EventTarget
    Object.setPrototypeOf(proto, Node.prototype);

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

    // removeAttribute
    if (!proto.removeAttribute) {
      proto.removeAttribute = function (name) {
        if (this._attrs) this._attrs.delete(String(name).toLowerCase());
        if (this.setAttribute) this.setAttribute(name, "");
      };
      markNative(proto.removeAttribute, "removeAttribute");
    }

    // getAttributeNames
    if (!proto.getAttributeNames) {
      proto.getAttributeNames = function () {
        const names = [];
        if (this.id) names.push("id");
        if (this.className) names.push("class");
        if (this.hasAttribute && this.hasAttribute("style")) names.push("style");
        if (this._attrs) {
          for (const k of this._attrs.keys()) {
            if (names.indexOf(k) === -1) names.push(k);
          }
        }
        return names;
      };
      markNative(proto.getAttributeNames, "getAttributeNames");
    }

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
        if (this.setAttribute) this.setAttribute("value", String(v));
      },
      configurable: true,
    });

    Object.defineProperty(proto, "checked", {
      get: function () {
        if (this._checked !== undefined) return this._checked;
        return this.hasAttribute("checked");
      },
      set: function (v) {
        this._checked = Boolean(v);
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
        if (this._selected !== undefined) return this._selected;
        return this.hasAttribute("selected");
      },
      set: function (v) {
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
        const opts = [];
        let child = this.firstChild;
        while (child) {
          if ((child.tagName || "").toLowerCase() === "option") {
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
        const opts = this.options;
        for (let i = 0; i < opts.length; i++) {
          if (opts[i].selected) return i;
        }
        return opts.length > 0 && !this.multiple ? 0 : -1;
      },
      set: function (idx) {
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

    Object.defineProperty(proto, "hidden", {
      get: function () {
        return this.hasAttribute("hidden");
      },
      set: function (v) {
        if (v) this.setAttribute("hidden", "");
        else this.removeAttribute("hidden");
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
      };
      markNative(proto.click, "click");
    }

    if (!proto.focus) {
      proto.focus = function () {
        const evt = new FocusEvent("focus", { bubbles: false, cancelable: false });
        this.dispatchEvent(evt);
      };
      markNative(proto.focus, "focus");
    }

    if (!proto.blur) {
      proto.blur = function () {
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

    // 1. HTMLAnchorElement
    function HTMLAnchorElement() {}
    HTMLAnchorElement.prototype = Object.create(ElementProto);
    HTMLAnchorElement.prototype.constructor = HTMLAnchorElement;

    Object.defineProperty(HTMLAnchorElement.prototype, "href", {
      get: function () {
        const raw = this.getAttribute("href") || "";
        if (!raw) return "";
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

    const anchorUrlProps = ["origin", "protocol", "host", "hostname", "port", "pathname", "search", "hash"];
    for (let i = 0; i < anchorUrlProps.length; i++) {
      const prop = anchorUrlProps[i];
      Object.defineProperty(HTMLAnchorElement.prototype, prop, {
        get: function () {
          try {
            const u = new URL(this.href);
            return u[prop];
          } catch (e) {
            return "";
          }
        },
        set: function (v) {
          try {
            const u = new URL(this.href);
            u[prop] = v;
            this.href = u.href;
          } catch (e) {}
        },
        configurable: true,
      });
    }

    Object.defineProperty(HTMLAnchorElement.prototype, "target", {
      get: function () { return this.getAttribute("target") || ""; },
      set: function (v) { this.setAttribute("target", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLAnchorElement.prototype, "rel", {
      get: function () { return this.getAttribute("rel") || ""; },
      set: function (v) { this.setAttribute("rel", String(v)); },
      configurable: true,
    });

    markNative(HTMLAnchorElement, "HTMLAnchorElement");
    global.HTMLAnchorElement = HTMLAnchorElement;

    // 2. HTMLImageElement
    function HTMLImageElement() {}
    HTMLImageElement.prototype = Object.create(ElementProto);
    HTMLImageElement.prototype.constructor = HTMLImageElement;

    Object.defineProperty(HTMLImageElement.prototype, "src", {
      get: function () { return this.getAttribute("src") || ""; },
      set: function (v) { this.setAttribute("src", String(v)); },
      configurable: true,
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
    function HTMLScriptElement() {}
    HTMLScriptElement.prototype = Object.create(ElementProto);
    HTMLScriptElement.prototype.constructor = HTMLScriptElement;

    Object.defineProperty(HTMLScriptElement.prototype, "src", {
      get: function () { return this.getAttribute("src") || ""; },
      set: function (v) { this.setAttribute("src", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLScriptElement.prototype, "type", {
      get: function () { return this.getAttribute("type") || ""; },
      set: function (v) { this.setAttribute("type", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLScriptElement.prototype, "async", {
      get: function () { return this.hasAttribute("async"); },
      set: function (v) { if (v) this.setAttribute("async", ""); else this.removeAttribute("async"); },
      configurable: true,
    });

    Object.defineProperty(HTMLScriptElement.prototype, "defer", {
      get: function () { return this.hasAttribute("defer"); },
      set: function (v) { if (v) this.setAttribute("defer", ""); else this.removeAttribute("defer"); },
      configurable: true,
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
    function HTMLIFrameElement() {}
    HTMLIFrameElement.prototype = Object.create(ElementProto);
    HTMLIFrameElement.prototype.constructor = HTMLIFrameElement;

    Object.defineProperty(HTMLIFrameElement.prototype, "src", {
      get: function () { return this.getAttribute("src") || ""; },
      set: function (v) { this.setAttribute("src", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLIFrameElement.prototype, "contentWindow", {
      get: function () { return null; },
      configurable: true,
    });

    Object.defineProperty(HTMLIFrameElement.prototype, "contentDocument", {
      get: function () { return null; },
      configurable: true,
    });

    markNative(HTMLIFrameElement, "HTMLIFrameElement");
    global.HTMLIFrameElement = HTMLIFrameElement;

    // 6. Generic HTMLElement
    function HTMLElement() {}
    HTMLElement.prototype = Object.create(ElementProto);
    HTMLElement.prototype.constructor = HTMLElement;
    markNative(HTMLElement, "HTMLElement");
    global.HTMLElement = HTMLElement;
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
  // Module: element/forms/input.js
  // =============================================================
  // src/js/shim/element/forms/input.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/input.ts

  function findAssociatedForm(element) {
    const formAttr = element.getAttribute("form");
    if (formAttr && typeof document !== "undefined" && document.getElementById) {
      const f = document.getElementById(formAttr);
      if (f && f.tagName === "FORM") return f;
    }
    let curr = element.parentElement;
    while (curr) {
      if (curr.tagName === "FORM") return curr;
      curr = curr.parentElement;
    }
    return null;
  }

  function installInputElement(global) {
    const ElementProto = global.Element ? global.Element.prototype : {};

    function HTMLInputElement() {
      this._value = "";
      this._checked = false;
      this._customValidityMessage = "";
    }
    HTMLInputElement.prototype = Object.create(ElementProto);
    HTMLInputElement.prototype.constructor = HTMLInputElement;

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
        this._value = String(v);
        if (this.setAttribute) this.setAttribute("value", String(v));
      },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "defaultValue", {
      get: function () { return this.getAttribute("value") || ""; },
      set: function (v) { this.setAttribute("value", String(v)); },
      configurable: true,
    });

    Object.defineProperty(HTMLInputElement.prototype, "checked", {
      get: function () {
        if (this._checked !== undefined) return this._checked;
        return this.hasAttribute("checked");
      },
      set: function (v) {
        this._checked = Boolean(v);
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

    HTMLInputElement.prototype.select = function () {};
    HTMLInputElement.prototype.setSelectionRange = function () {};

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
    }
    HTMLTextAreaElement.prototype = Object.create(ElementProto);
    HTMLTextAreaElement.prototype.constructor = HTMLTextAreaElement;

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
        if (this._value !== undefined) return this._value;
        if (this.hasAttribute && this.hasAttribute("value")) return this.getAttribute("value");
        return this.textContent || "";
      },
      set: function (v) {
        this._value = String(v);
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
        if (this._selected !== undefined) return this._selected;
        return this.hasAttribute("selected");
      },
      set: function (v) {
        this._selected = Boolean(v);
        if (v) {
          if (this.setAttribute) this.setAttribute("selected", "");
        } else {
          if (this.removeAttribute) this.removeAttribute("selected");
        }
      },
      configurable: true,
    });

    Object.defineProperty(HTMLOptionElement.prototype, "defaultSelected", {
      get: function () { return this.hasAttribute("selected"); },
      set: function (v) { if (v) this.setAttribute("selected", ""); else this.removeAttribute("selected"); },
      configurable: true,
    });

    Object.defineProperty(HTMLOptionElement.prototype, "disabled", {
      get: function () { return this.hasAttribute("disabled"); },
      set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
      configurable: true,
    });

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
        const opts = [];
        let child = this.firstChild;
        while (child) {
          if ((child.tagName || "").toLowerCase() === "option") {
            opts.push(child);
          }
          child = child.nextSibling;
        }
        return new HTMLCollection(opts);
      },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "length", {
      get: function () { return this.options.length; },
      configurable: true,
    });

    Object.defineProperty(HTMLSelectElement.prototype, "selectedIndex", {
      get: function () {
        const opts = this.options;
        for (let i = 0; i < opts.length; i++) {
          if (opts[i].selected) return i;
        }
        return opts.length > 0 && !this.multiple ? 0 : -1;
      },
      set: function (idx) {
        const i = Number(idx);
        const opts = this.options;
        for (let j = 0; j < opts.length; j++) {
          opts[j].selected = (j === i);
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
      if (this.appendChild) {
        this.appendChild(element);
      }
    };

    HTMLSelectElement.prototype.remove = function (index) {
      const opts = this.options;
      const i = Number(index);
      if (i >= 0 && i < opts.length && this.removeChild) {
        this.removeChild(opts[i]);
      }
    };

    markNative(HTMLSelectElement, "HTMLSelectElement");
    markNative(HTMLSelectElement.prototype.setCustomValidity, "setCustomValidity");
    markNative(HTMLSelectElement.prototype.checkValidity, "checkValidity");
    markNative(HTMLSelectElement.prototype.add, "add");
    markNative(HTMLSelectElement.prototype.remove, "remove");

    global.HTMLSelectElement = HTMLSelectElement;
  }

  // =============================================================
  // Module: element/forms/button.js
  // =============================================================
  // src/js/shim/element/forms/button.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/button.ts

  function installButtonElement(global) {
    const ElementProto = global.Element ? global.Element.prototype : {};

    function HTMLButtonElement() {
      this._customValidityMessage = "";
    }
    HTMLButtonElement.prototype = Object.create(ElementProto);
    HTMLButtonElement.prototype.constructor = HTMLButtonElement;

    Object.defineProperty(HTMLButtonElement.prototype, "type", {
      get: function () {
        return (this.getAttribute("type") || "submit").toLowerCase();
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

  function installFormElement(global) {
    const ElementProto = global.Element ? global.Element.prototype : {};

    function HTMLFormElement() {}
    HTMLFormElement.prototype = Object.create(ElementProto);
    HTMLFormElement.prototype.constructor = HTMLFormElement;

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
      const evt = new Event("submit", { bubbles: true, cancelable: true });
      this.dispatchEvent(evt);
    };

    HTMLFormElement.prototype.reset = function () {
      const list = collectFormControls(this);
      for (let i = 0; i < list.length; i++) {
        const c = list[i];
        if (c.defaultValue !== undefined) c.value = c.defaultValue;
        if (c.defaultChecked !== undefined) c.checked = c.defaultChecked;
        if (c.defaultSelected !== undefined) c.selected = c.defaultSelected;
      }
      const evt = new Event("reset", { bubbles: true, cancelable: true });
      this.dispatchEvent(evt);
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
    markNative(HTMLFormElement.prototype.reset, "reset");
    markNative(HTMLFormElement.prototype.checkValidity, "checkValidity");
    markNative(HTMLFormElement.prototype.reportValidity, "reportValidity");

    global.HTMLFormElement = HTMLFormElement;
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

  Range.prototype.createContextualFragment = function (fragmentHtml) {
    const frag = new DocumentFragment();
    if (typeof document !== "undefined" && document.createElement) {
      const div = document.createElement("div");
      div.innerHTML = String(fragmentHtml || "");
      if (div.childNodes) {
        for (let i = 0; i < div.childNodes.length; i++) {
          frag.appendChild(div.childNodes[i]);
        }
      }
    }
    return frag;
  };

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

  // =============================================================
  // Module: canvas2d.js
  // =============================================================
  // src/js/shim/canvas2d.js
  // Aligned with vendor/kitesurf/packages/dom-shim/src/canvas2d.ts
  // Cloud vector recording and rendering engine for HTML5 Canvas 2D.
  // Configured by globalThis.__CAP_RENDER_CONFIG.canvasRecording (true: record ops, false: headless fast stub).

  const VALID_PNG_BASE64 = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=";

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

  function CanvasRenderingContext2D(canvas) {
    this.canvas = canvas || null;
    this._state = defaultCanvasState();
    this._stack = [];
    this._ops = []; // Recorded vector operations
  }

  // State stack methods
  CanvasRenderingContext2D.prototype.save = function () {
    this._stack.push(cloneCanvasState(this._state));
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "save" });
    }
  };

  CanvasRenderingContext2D.prototype.restore = function () {
    if (this._stack.length > 0) {
      this._state = this._stack.pop();
      if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
        this._ops.push({ op: "restore" });
      }
    }
  };

  // Transform methods
  CanvasRenderingContext2D.prototype.scale = function (x, y) {
    this._state.matrix[0] *= x;
    this._state.matrix[3] *= y;
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "scale", x: x, y: y });
    }
  };

  CanvasRenderingContext2D.prototype.rotate = function (angle) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "rotate", angle: angle });
    }
  };

  CanvasRenderingContext2D.prototype.translate = function (x, y) {
    this._state.matrix[4] += x;
    this._state.matrix[5] += y;
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "translate", x: x, y: y });
    }
  };

  CanvasRenderingContext2D.prototype.transform = function (a, b, c, d, e, f) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "transform", args: [a, b, c, d, e, f] });
    }
  };

  CanvasRenderingContext2D.prototype.setTransform = function (a, b, c, d, e, f) {
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
    this._state.matrix = [1, 0, 0, 1, 0, 0];
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "resetTransform" });
    }
  };

  CanvasRenderingContext2D.prototype.getTransform = function () {
    const m = this._state.matrix;
    return { a: m[0], b: m[1], c: m[2], d: m[3], e: m[4], f: m[5] };
  };

  // Rectangle drawing
  CanvasRenderingContext2D.prototype.clearRect = function (x, y, w, h) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "clearRect", args: [x, y, w, h] });
    }
  };

  CanvasRenderingContext2D.prototype.fillRect = function (x, y, w, h) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "fillRect", args: [x, y, w, h], fillStyle: this._state.fillStyle });
    }
  };

  CanvasRenderingContext2D.prototype.strokeRect = function (x, y, w, h) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "strokeRect", args: [x, y, w, h], strokeStyle: this._state.strokeStyle, lineWidth: this._state.lineWidth });
    }
  };

  // Path drawing methods
  CanvasRenderingContext2D.prototype.beginPath = function () {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "beginPath" });
    }
  };

  CanvasRenderingContext2D.prototype.closePath = function () {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "closePath" });
    }
  };

  CanvasRenderingContext2D.prototype.moveTo = function (x, y) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "moveTo", x: x, y: y });
    }
  };

  CanvasRenderingContext2D.prototype.lineTo = function (x, y) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "lineTo", x: x, y: y });
    }
  };

  CanvasRenderingContext2D.prototype.quadraticCurveTo = function (cpx, cpy, x, y) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "quadraticCurveTo", args: [cpx, cpy, x, y] });
    }
  };

  CanvasRenderingContext2D.prototype.bezierCurveTo = function (cp1x, cp1y, cp2x, cp2y, x, y) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "bezierCurveTo", args: [cp1x, cp1y, cp2x, cp2y, x, y] });
    }
  };

  CanvasRenderingContext2D.prototype.arcTo = function (x1, y1, x2, y2, radius) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "arcTo", args: [x1, y1, x2, y2, radius] });
    }
  };

  CanvasRenderingContext2D.prototype.arc = function (x, y, radius, startAngle, endAngle, counterclockwise) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "arc", args: [x, y, radius, startAngle, endAngle, Boolean(counterclockwise)] });
    }
  };

  CanvasRenderingContext2D.prototype.ellipse = function (x, y, radiusX, radiusY, rotation, startAngle, endAngle, counterclockwise) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "ellipse", args: [x, y, radiusX, radiusY, rotation, startAngle, endAngle, Boolean(counterclockwise)] });
    }
  };

  CanvasRenderingContext2D.prototype.rect = function (x, y, w, h) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "rect", args: [x, y, w, h] });
    }
  };

  CanvasRenderingContext2D.prototype.fill = function (pathOrRule, rule) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "fill", fillStyle: this._state.fillStyle });
    }
  };

  CanvasRenderingContext2D.prototype.stroke = function (path) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "stroke", strokeStyle: this._state.strokeStyle, lineWidth: this._state.lineWidth });
    }
  };

  CanvasRenderingContext2D.prototype.clip = function (pathOrRule, rule) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "clip" });
    }
  };

  CanvasRenderingContext2D.prototype.isPointInPath = function (x, y) { return true; };
  CanvasRenderingContext2D.prototype.isPointInStroke = function (x, y) { return true; };

  // Text drawing
  CanvasRenderingContext2D.prototype.fillText = function (text, x, y, maxWidth) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "fillText", text: String(text), x: x, y: y, maxWidth: maxWidth, font: this._state.font, fillStyle: this._state.fillStyle });
    }
  };

  CanvasRenderingContext2D.prototype.strokeText = function (text, x, y, maxWidth) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "strokeText", text: String(text), x: x, y: y, maxWidth: maxWidth, font: this._state.font, strokeStyle: this._state.strokeStyle });
    }
  };

  CanvasRenderingContext2D.prototype.measureText = function (text) {
    return new TextMetrics(text, this._state.font);
  };

  // Image drawing
  CanvasRenderingContext2D.prototype.drawImage = function (image) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "drawImage", args: Array.prototype.slice.call(arguments, 1) });
    }
  };

  // Gradients and Patterns
  CanvasRenderingContext2D.prototype.createLinearGradient = function (x0, y0, x1, y1) {
    return new CanvasGradient("linear", x0, y0, 0, x1, y1, 0);
  };

  CanvasRenderingContext2D.prototype.createRadialGradient = function (x0, y0, r0, x1, y1, r1) {
    return new CanvasGradient("radial", x0, y0, r0, x1, y1, r1);
  };

  CanvasRenderingContext2D.prototype.createPattern = function (image, repetition) {
    return new CanvasPattern(image, repetition);
  };

  // Pixel manipulation
  CanvasRenderingContext2D.prototype.createImageData = function (sw, sh) {
    return new ImageData(sw, sh);
  };

  CanvasRenderingContext2D.prototype.getImageData = function (sx, sy, sw, sh) {
    return new ImageData(sw, sh);
  };

  CanvasRenderingContext2D.prototype.putImageData = function (imagedata, dx, dy) {
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "putImageData", dx: dx, dy: dy, width: imagedata.width, height: imagedata.height });
    }
  };

  // Dash styling
  CanvasRenderingContext2D.prototype.setLineDash = function (segments) {
    this._state.lineDash = Array.isArray(segments) ? segments.slice() : [];
  };

  CanvasRenderingContext2D.prototype.getLineDash = function () {
    return this._state.lineDash.slice();
  };

  // Properties with getters and setters
  const stateProps = [
    "strokeStyle", "fillStyle", "lineWidth", "lineCap", "lineJoin", "miterLimit",
    "lineDashOffset", "globalAlpha", "globalCompositeOperation", "shadowColor",
    "shadowBlur", "shadowOffsetX", "shadowOffsetY", "font", "textAlign",
    "textBaseline", "direction", "filter", "imageSmoothingEnabled"
  ];

  for (let i = 0; i < stateProps.length; i++) {
    const p = stateProps[i];
    Object.defineProperty(CanvasRenderingContext2D.prototype, p, {
      get: function () { return this._state[p]; },
      set: function (v) { this._state[p] = v; },
      configurable: true,
    });
  }

  // Vector recording extraction and inspection API for CapSurf
  CanvasRenderingContext2D.prototype.__getRecordingOps = function () {
    return this._ops.slice();
  };

  CanvasRenderingContext2D.prototype.__clearRecordingOps = function () {
    this._ops = [];
  };

  CanvasRenderingContext2D.prototype.__opsCount = function () {
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

    // 2. Navigator: modern Chrome 132 anti-fingerprint shield
    const navigatorObj = {
      userAgent: "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/132.0.0.0 Safari/537.36 CapSurf/1.0.0",
      appVersion: "5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/132.0.0.0 Safari/537.36 CapSurf/1.0.0",
      platform: "Linux x86_64",
      appName: "Netscape",
      appCodeName: "Mozilla",
      product: "Gecko",
      productSub: "20030107",
      vendor: "Google Inc.",
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
    };
    navigatorObj.userAgentData = {
      brands: [
        { brand: "Not A(Brand", version: "8" },
        { brand: "Chromium", version: "132" },
        { brand: "Google Chrome", version: "132" }
      ],
      mobile: false,
      platform: "Linux",
      getHighEntropyValues: markNative(function () {
        return Promise.resolve({
          architecture: "x86",
          bitness: "64",
          model: "",
          platformVersion: "6.5.0",
          uaFullVersion: "132.0.6834.110"
        });
      }, "getHighEntropyValues"),
    };
    global.navigator = navigatorObj;

    // 3. Location & History
    let currentHref = "about:blank";
    const locationObj = {
      get href() { return currentHref; },
      set href(val) { currentHref = String(val); },
      get protocol() { try { return new URL(currentHref).protocol; } catch(e) { return "about:"; } },
      get host() { try { return new URL(currentHref).host; } catch(e) { return ""; } },
      get hostname() { try { return new URL(currentHref).hostname; } catch(e) { return ""; } },
      get port() { try { return new URL(currentHref).port; } catch(e) { return ""; } },
      get pathname() { try { return new URL(currentHref).pathname; } catch(e) { return "blank"; } },
      get search() { try { return new URL(currentHref).search; } catch(e) { return ""; } },
      get hash() { try { return new URL(currentHref).hash; } catch(e) { return ""; } },
      get origin() { try { return new URL(currentHref).origin; } catch(e) { return "null"; } },
      assign: markNative(function (url) { currentHref = String(url); }, "assign"),
      replace: markNative(function (url) { currentHref = String(url); }, "replace"),
      reload: markNative(function () {}, "reload"),
      toString: function () { return currentHref; },
    };
    global.location = locationObj;

    const historyObj = {
      length: 1,
      state: null,
      pushState: markNative(function (st, title, url) {
        this.state = st;
        if (url) locationObj.href = url;
      }, "pushState"),
      replaceState: markNative(function (st, title, url) {
        this.state = st;
        if (url) locationObj.href = url;
      }, "replaceState"),
      back: markNative(function () {}, "back"),
      forward: markNative(function () {}, "forward"),
      go: markNative(function () {}, "go"),
    };
    global.history = historyObj;

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
        return setTimeout(function () {
          try {
            cb(performance.now());
          } catch (e) {
            if (typeof console !== "undefined" && console.error) console.error(e);
          }
        }, 16);
      }, "requestAnimationFrame");
    }

    if (!global.cancelAnimationFrame) {
      global.cancelAnimationFrame = markNative(function (id) {
        clearTimeout(id);
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
    global.EventTarget = EventTarget;

    // 2. Expose DOM Collections and Nodes
    global.DOMTokenList = DOMTokenList;
    global.HTMLCollection = HTMLCollection;
    global.NodeList = NodeList;
    global.Node = Node;
    global.Text = Text;
    global.Comment = Comment;
    global.DocumentFragment = DocumentFragment;

    // 3. Expose Mutation Observer
    global.MutationObserver = MutationObserver;
    global.MutationRecord = MutationRecord;

    // 4. Expose FormData
    global.FormData = FormData;

    // 5. Expose Traversal
    global.NodeFilter = NodeFilter;
    global.TreeWalker = TreeWalker;
    global.NodeIterator = NodeIterator;

    // 6. Expose Range and Selection
    global.Range = Range;
    global.Selection = Selection;

    // 7. Expose DOMParser
    global.DOMParser = DOMParser;

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

      global.Element.prototype.scrollIntoView = markNative(function () {}, "scrollIntoView");

      global.Element.prototype.getContext = markNative(function (type) {
        const t = String(type).toLowerCase();
        if (t === "webgl" || t === "webgl2" || t === "experimental-webgl") {
          return null;
        }
        if (t === "2d") {
          if (!this._ctx2d) {
            this._ctx2d = new CanvasRenderingContext2D(this);
          }
          return this._ctx2d;
        }
        return null;
      }, "getContext");

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

    // 12. Install window environment
    installWindow(global);

    // 13. Enhance Document prototype and instance
    if (global.Document && global.Document.prototype) {
      const docProto = global.Document.prototype;

      // Inherit from Node
      Object.setPrototypeOf(docProto, Node.prototype);

      if (docProto.createElement) {
        const origCreateElement = docProto.createElement;
        docProto.createElement = function (tagName) {
          const el = origCreateElement.call(this, tagName);
          if (!el) return el;
          const tag = String(tagName).toLowerCase();
          switch (tag) {
            case "input":
              Object.setPrototypeOf(el, global.HTMLInputElement.prototype);
              break;
            case "textarea":
              Object.setPrototypeOf(el, global.HTMLTextAreaElement.prototype);
              break;
            case "select":
              Object.setPrototypeOf(el, global.HTMLSelectElement.prototype);
              break;
            case "option":
              Object.setPrototypeOf(el, global.HTMLOptionElement.prototype);
              break;
            case "button":
              Object.setPrototypeOf(el, global.HTMLButtonElement.prototype);
              break;
            case "form":
              Object.setPrototypeOf(el, global.HTMLFormElement.prototype);
              break;
            case "a":
              Object.setPrototypeOf(el, global.HTMLAnchorElement.prototype);
              break;
            case "img":
              Object.setPrototypeOf(el, global.HTMLImageElement.prototype);
              break;
            case "canvas":
              Object.setPrototypeOf(el, global.HTMLCanvasElement.prototype);
              break;
            case "script":
              Object.setPrototypeOf(el, global.HTMLScriptElement.prototype);
              break;
            case "iframe":
              Object.setPrototypeOf(el, global.HTMLIFrameElement.prototype);
              break;
            default:
              Object.setPrototypeOf(el, global.HTMLElement.prototype);
              break;
          }
          return el;
        };
        markNative(docProto.createElement, "createElement");
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

      Object.defineProperty(docProto, "timeline", {
        get: function () { return globalDocTimeline; },
        configurable: true,
      });

      Object.defineProperty(docProto, "defaultView", {
        get: function () { return global; },
        configurable: true,
      });

      let docCookie = "";
      Object.defineProperty(docProto, "cookie", {
        get: function () { return docCookie; },
        set: function (v) {
          const str = String(v || "").trim();
          if (!str) return;
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
    }

    if (global.document) {
      if (!global.document.location) global.document.location = global.location;
      if (!global.document.createRange) global.document.createRange = function () { return new Range(); };
      if (!global.document.getSelection) global.document.getSelection = function () {
        if (!global.document._selection) global.document._selection = new Selection();
        return global.document._selection;
      };
    }

    // 14. Mount window EventTarget methods
    global.addEventListener = EventTarget.prototype.addEventListener.bind(global);
    global.removeEventListener = EventTarget.prototype.removeEventListener.bind(global);
    global.dispatchEvent = EventTarget.prototype.dispatchEvent.bind(global);
    global.getSelection = function () {
      if (global.document && global.document.getSelection) return global.document.getSelection();
      return new Selection();
    };
  }

  // Auto-install on globalThis/window
  installShim(typeof window !== "undefined" ? window : globalThis);

})(typeof window !== "undefined" ? window : globalThis);
