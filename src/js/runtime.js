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
    this.cancelable = opts.cancelable !== undefined ? Boolean(opts.cancelable) : (this.type === "click" || this.type === "submit");
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

  function SubmitEvent(type, options) {
    Event.call(this, type, options);
    const opts = options || {};
    this.submitter = opts.submitter !== undefined ? opts.submitter : null;
  }
  SubmitEvent.prototype = Object.create(Event.prototype);
  SubmitEvent.prototype.constructor = SubmitEvent;
  markNative(SubmitEvent, "SubmitEvent");

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
      invoke(this, PHASE_AT_TARGET);
      if (eventType === "click") {
        checkActivation(this);
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

    event.eventPhase = PHASE_NONE;
    event.currentTarget = null;

    // Activation behavior for click events
    if (eventType === "click" && activatedButton && !event.defaultPrevented) {
      if (typeof handleButtonActivation === "function") {
        handleButtonActivation(activatedButton);
      }
    }

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

  function makeHtmlCollection(getList) {
    const target = Object.create(HTMLCollection.prototype);
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
        if (prop === "length") return false;
        if (typeof prop === "string" && /^\d+$/.test(prop)) return false;
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
          return { value: getList().length, writable: false, enumerable: false, configurable: true };
        }
        if (typeof prop === "string" && /^\d+$/.test(prop)) {
          const i = Number(prop);
          const list = getList();
          if (i >= 0 && i < list.length) {
            return { value: list[i], writable: false, enumerable: true, configurable: true };
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
    function walk(node) {
      if (!node) return;
      if (node._children && node._children.length > 0) {
        for (let i = 0; i < node._children.length; i++) {
          const c = node._children[i];
          if (c && c.nodeType === 1) { // ELEMENT_NODE
            elements.push(c);
            walk(c);
          }
        }
      } else {
        let child = node.firstChild;
        while (child) {
          if (child.nodeType === 1) {
            elements.push(child);
            walk(child);
          }
          child = child.nextSibling;
        }
      }
    }

    const root = doc ? (doc.documentElement || doc.body || doc) : null;
    if (root) {
      if (root.nodeType === 1) {
        elements.push(root);
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
  }
  if (typeof window !== "undefined") {
    window.NamedNodeMap = NamedNodeMap;
    window.Attr = Attr;
  }

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
        curr = curr.parentNode || curr.parentElement;
      }
      return false;
    },
    configurable: true,
  });

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

  function ensureChildren(node) {
    if (!node._children) {
      node._children = [];
    }
    return node._children;
  }

  const nativeElementAppendChild = (typeof Element !== "undefined" && Element.prototype) ? Element.prototype.appendChild : null;
  const nativeElementRemoveChild = (typeof Element !== "undefined" && Element.prototype) ? Element.prototype.removeChild : null;
  const nativeElementGetTextContent = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "textContent")?.get : null;
  const nativeElementSetTextContent = (typeof Element !== "undefined" && Element.prototype) ? Object.getOwnPropertyDescriptor(Element.prototype, "textContent")?.set : null;

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

    // If child already has a parent, remove from it
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
        if (this._children) {
          this._children.length = 0;
        }
        if (nativeElementSetTextContent) {
          try {
            nativeElementSetTextContent.call(this, v);
          } catch (e) {}
        }
        const val = String(v);
        if (val.length > 0) {
          const t = new Text(val);
          t.parentNode = this;
          t.ownerDocument = this.ownerDocument || this;
          ensureChildren(this).push(t);
        }
      },
      configurable: true,
    });

    Element.prototype.appendChild = Node.prototype.appendChild;
    Element.prototype.insertBefore = Node.prototype.insertBefore;
    Element.prototype.removeChild = Node.prototype.removeChild;
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
    return Node.prototype.appendChild.call(this, child);
  };
  DocumentFragment.prototype.insertBefore = function (newChild, refChild) {
    return Node.prototype.insertBefore.call(this, newChild, refChild);
  };
  DocumentFragment.prototype.removeChild = function (child) {
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
  Object.defineProperty(DocumentFragment.prototype, "childNodes", {
    get: function () {
      return new NodeList(this._children);
    },
    configurable: true,
  });
  markNative(DocumentFragment, "DocumentFragment");

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
          const rawKey = p.slice(0, colon).trim();
          const k = rawKey.startsWith("--") ? rawKey : rawKey.toLowerCase();
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
            const str = String(p);
            const key = str.startsWith("--") ? str : str.toLowerCase();
            return map.get(key) || "";
          };
        }
        if (prop === "setProperty") {
          return function (p, val) {
            const map = parseStyleMap();
            const str = String(p);
            const key = str.startsWith("--") ? str : str.toLowerCase();
            map.set(key, String(val));
            serializeStyleMap(map);
          };
        }
        if (prop === "removeProperty") {
          return function (p) {
            const map = parseStyleMap();
            const str = String(p);
            const key = str.startsWith("--") ? str : str.toLowerCase();
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
                this._attrs.set(item.name.toLowerCase(), entry);
              }
            }
          }
        }
        if (this.id && !this._attrs.has("id")) {
          const entry = { name: "id", value: this.id, namespaceURI: null, localName: "id", prefix: null };
          this._attrList.push(entry);
          this._attrs.set("id", entry);
        }
        if (this.className && !this._attrs.has("class")) {
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
      const key = String(name);
      const val = String(value);
      if (!this._attrs) this._syncAttrs();
      let existing = null;
      if (this._attrList) {
        for (let i = 0; i < this._attrList.length; i++) {
          if (this._attrList[i].name.toLowerCase() === key.toLowerCase()) {
            existing = this._attrList[i];
            break;
          }
        }
      }
      const oldVal = existing ? existing.value : (this._attrs.get(key.toLowerCase())?.value ?? null);
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
      this._attrs.set(key.toLowerCase(), { name: key, value: val });
      if (key.toLowerCase() === "id") {
        this.id = val;
      } else if (key.toLowerCase() === "class") {
        this.className = val;
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
    };
    markNative(proto.setAttribute, "setAttribute");

    // removeAttribute
    const origRemoveAttribute = proto.removeAttribute;
    proto.removeAttribute = function (name) {
      const key = String(name).toLowerCase();
      if (!this._attrs) this._syncAttrs();
      let oldVal = null;
      if (this._attrList) {
        for (let i = 0; i < this._attrList.length; i++) {
          if (this._attrList[i].name.toLowerCase() === key) {
            oldVal = this._attrList[i].value;
            this._attrList.splice(i, 1);
            break;
          }
        }
      }
      const oldEntry = this._attrs.get(key);
      if (!oldVal && oldEntry) oldVal = oldEntry.value;
      this._attrs.delete(key);
      if (key === "id") {
        this.id = "";
      } else if (key === "class") {
        this.className = "";
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
      const key = String(name).toLowerCase();
      if (!this._attrs) this._syncAttrs();
      if (this._attrList) {
        for (let i = 0; i < this._attrList.length; i++) {
          if (this._attrList[i].name.toLowerCase() === key) {
            return this._attrList[i].value;
          }
        }
      }
      if (this._attrs) {
        const entry = this._attrs.get(key);
        if (entry) return entry.value;
        if (origGetAttribute) {
          const val = origGetAttribute.call(this, name);
          if (val !== null) return val;
        }
        return null;
      }
      if (origGetAttribute) {
        return origGetAttribute.call(this, name);
      }
      return null;
    };
    markNative(proto.getAttribute, "getAttribute");

    // hasAttribute
    const origHasAttribute = proto.hasAttribute;
    proto.hasAttribute = function (name) {
      const key = String(name).toLowerCase();
      if (!this._attrs) this._syncAttrs();
      if (this._attrList) {
        for (let i = 0; i < this._attrList.length; i++) {
          if (this._attrList[i].name.toLowerCase() === key) {
            return true;
          }
        }
      }
      if (this._attrs) {
        if (this._attrs.has(key)) return true;
        if (origHasAttribute) return origHasAttribute.call(this, name);
        return false;
      }
      if (origHasAttribute) {
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
    if (clsDesc && clsDesc.set) {
      const origClsSet = clsDesc.set;
      Object.defineProperty(proto, "className", {
        get: clsDesc.get,
        set: function (v) {
          origClsSet.call(this, v);
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
        if (s === ":checked") return Boolean(this.checked);
        if (s === ":disabled") return Boolean(this.disabled);
        if (s === ":enabled") return !this.disabled;
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
        if (typeof document !== "undefined") {
          document.activeElement = this;
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
        if (this._defaultValue === undefined) {
          this._defaultValue = this.getAttribute("value") || "";
        }
        this._value = String(v);
        if (this.setAttribute) this.setAttribute("value", String(v));
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

  // Expose globals
  global.Blob = Blob;
  global.File = File;
  global.TextDecoder = TextDecoder;
  global.TextEncoder = TextEncoder;

  global.Headers = Headers;
  if (!global.Request) global.Request = Request; else patchExistingBodyBlob(global.Request);
  if (!global.Response) global.Response = Response; else patchExistingBodyBlob(global.Response);

  if (typeof window !== "undefined") {
    window.Blob = Blob;
    window.File = File;
    window.TextDecoder = TextDecoder;
    window.TextEncoder = TextEncoder;
    window.Headers = Headers;
    window.Request = Request;
    window.Response = Response;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.Blob = Blob;
    globalThis.File = File;
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
    const NativeEvent = global.Event || function (type, init) {
      this.type = type;
      this.bubbles = !!init?.bubbles;
      this.cancelable = !!init?.cancelable;
    };
    global.__NativeEvent = NativeEvent;

    // 0. DOMException & WHATWG URL Implementation
    function DOMException(message, name) {
      const err = new Error(message || "");
      err.name = name || "Error";
      err.code = name === "SecurityError" ? 18 : 0;
      Object.setPrototypeOf(err, DOMException.prototype);
      return err;
    }
    DOMException.prototype = Object.create(Error.prototype);
    DOMException.prototype.constructor = DOMException;
    markNative(DOMException, "DOMException");
    global.DOMException = DOMException;
    if (typeof window !== "undefined") window.DOMException = DOMException;
    if (typeof globalThis !== "undefined") globalThis.DOMException = DOMException;

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

      let originVal = (global.location && global.location.origin) ? String(global.location.origin) : "null";
      if (!originVal || originVal === "about:" || originVal === "" || (global.location && global.location.href === "about:blank")) {
        originVal = "null";
      }

      const reqHeaders = {
        "user-agent": "CapSurf/1.0.0",
        "origin": originVal,
        "accept": "*/*",
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

    // 2. Expose DOM Collections and Nodes
    global.DOMTokenList = DOMTokenList;
    global.HTMLCollection = HTMLCollection;
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
            case "area":
              Object.setPrototypeOf(el, global.HTMLAreaElement.prototype);
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
