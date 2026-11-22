// src/js/shim/events/dispatch.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/events/dispatch.ts

const nodeListenersMap = new WeakMap();
const hydratedHandlersMap = new WeakMap();

function getListeners(target) {
  if (!target || typeof target !== "object") return null;
  let map = nodeListenersMap.get(target);
  if (!map) {
    map = new Map();
    nodeListenersMap.set(target, map);
  }
  return map;
}

function EventTarget() {
  if (this && this.constructor && this.constructor.name === "CapSurfWebSocket") {
    const ws = this;
    const handlers = { onopen: null, onerror: null, onclose: null, onmessage: null };
    ["onopen", "onerror", "onclose", "onmessage"].forEach(function (prop) {
      Object.defineProperty(ws, prop, {
        configurable: true,
        enumerable: true,
        get: function () {
          const fn = handlers[prop];
          if (typeof fn !== "function") return fn;
          return function (ev) {
            if (ev && typeof ev === "object") {
              ev._handledByOnProp = true;
              if (prop === "onmessage" && ws.binaryType === "arraybuffer" && ev.data && ArrayBuffer.isView(ev.data)) {
                ev.data = ev.data.buffer.slice(ev.data.byteOffset, ev.data.byteOffset + ev.data.byteLength);
              }
            }
            return fn.call(this, ev);
          };
        },
        set: function (v) {
          handlers[prop] = typeof v === "function" ? v : null;
        },
      });
    });
  }
}

const PHASE_NONE = 0;
const PHASE_CAPTURING = 1;
const PHASE_AT_TARGET = 2;
const PHASE_BUBBLING = 3;

EventTarget.prototype.addEventListener = function addEventListener(type, callback) {
  if (!this || (!(this instanceof EventTarget) && this !== global && (typeof window !== "undefined" && this !== window) && (typeof globalThis !== "undefined" && this !== globalThis))) {
    throw new TypeError("Failed to execute 'addEventListener' on 'EventTarget': Illegal invocation");
  }
  const options = arguments[2];
  // Per WHATWG DOM spec, flatten options BEFORE checking if callback is null
  let capture = false;
  let once = false;
  let passive = false;
  let signal = undefined;
  if (typeof options === "boolean") {
    capture = options;
  } else if (options !== null && typeof options === "object") {
    capture = Boolean(options.capture);
    once = Boolean(options.once);
    passive = Boolean(options.passive);
    signal = options.signal;
  }
  if (!type || callback === null || callback === undefined) return;
  if (typeof callback !== "function" && typeof callback !== "object") return;
  if (signal && signal.aborted) return;
  const map = getListeners(this);
  if (!map) return;
  const t = String(type).toLowerCase();
  let list = map.get(t);
  if (!list) {
    list = [];
    map.set(t, list);
  }
  for (let i = 0; i < list.length; i++) {
    if (list[i].callback === callback && list[i].capture === capture) {
      return;
    }
  }
  const entry = { callback: callback, capture: capture, once: once, passive: passive };
  list.push(entry);
  if (signal && typeof signal.addEventListener === "function") {
    const self = this;
    signal.addEventListener("abort", function () {
      self.removeEventListener(type, callback, capture);
    }, { once: true });
  }
};

EventTarget.prototype.removeEventListener = function removeEventListener(type, callback) {
  if (!this || (!(this instanceof EventTarget) && this !== global && (typeof window !== "undefined" && this !== window) && (typeof globalThis !== "undefined" && this !== globalThis))) {
    throw new TypeError("Failed to execute 'removeEventListener' on 'EventTarget': Illegal invocation");
  }
  const options = arguments[2];
  // Per WHATWG DOM spec, flatten options BEFORE checking if callback is null
  const capture = typeof options === "boolean" ? options : Boolean(options && options.capture);
  if (!type || callback === null || callback === undefined) return;
  if (typeof callback !== "function" && typeof callback !== "object") return;
  const map = getListeners(this);
  if (!map) return;
  const t = String(type).toLowerCase();
  const list = map.get(t);
  if (!list) return;
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

function getNodeRoot(node) {
  if (!node || typeof node !== "object") return null;
  if (typeof node.getRootNode === "function") {
    return node.getRootNode();
  }
  let cur = node;
  while (cur && cur.parentNode) cur = cur.parentNode;
  return cur;
}

function isShadowRootNode(node) {
  return Boolean(node && node.nodeType === 11 && node.host);
}

function isShadowIncludingInclusiveAncestor(ancestor, node) {
  if (!ancestor || !node) return false;
  if (ancestor === node) return true;
  let cur = node;
  const seen = new Set();
  while (cur && !seen.has(cur)) {
    seen.add(cur);
    if (cur === ancestor) return true;
    if (cur.parentNode) {
      cur = cur.parentNode;
    } else if (cur.host) {
      cur = cur.host;
    } else {
      break;
    }
  }
  return false;
}

function retargetEventTarget(target, currentTarget) {
  let a = target;
  const seen = new Set();
  while (a && !seen.has(a)) {
    seen.add(a);
    const root = getNodeRoot(a);
    if (isShadowRootNode(root) && !isShadowIncludingInclusiveAncestor(root, currentTarget)) {
      a = root.host;
    } else {
      break;
    }
  }
  return a;
}

function filterComposedPath(fullPath, currentTarget) {
  if (!fullPath || fullPath.length === 0) return [];
  if (!currentTarget) return [];
  const result = [];
  for (let i = 0; i < fullPath.length; i++) {
    const node = fullPath[i];
    // Hide node if any closed shadow root that is a shadow-including inclusive ancestor of node
    // is NOT a shadow-including inclusive ancestor of currentTarget.
    let hidden = false;
    let cur = node;
    const seen = new Set();
    while (cur && !seen.has(cur)) {
      seen.add(cur);
      const r = getNodeRoot(cur);
      if (isShadowRootNode(r)) {
        if (r.mode === "closed" && !isShadowIncludingInclusiveAncestor(r, currentTarget)) {
          hidden = true;
          break;
        }
        cur = r.host;
      } else {
        break;
      }
    }
    if (!hidden) {
      result.push(node);
    }
  }
  return result;
}
global.__capsurf_filterComposedPath = filterComposedPath;

const _eventsInFlight = new WeakSet();

function internalDispatch(target, event, trusted) {
  if (!event || typeof event !== "object") {
    throw new TypeError("Failed to execute 'dispatchEvent': parameter 1 is not of type 'Event'.");
  }
  const eventState = typeof _eventState !== "undefined" ? _eventState.get(event) : null;
  if (_eventsInFlight.has(event)) {
    throw new DOMException("The event is already being dispatched.", "InvalidStateError");
  }
  _eventsInFlight.add(event);
  if (eventState) {
    eventState.isDispatching = true;
    eventState.dispatched = true;
  }

  const prevWindowEvent = typeof window !== "undefined" ? window.event : undefined;
  try {
    if (target && typeof target === "object") {
      const targetId = target._id;
      if (trusted) {
        const canonical = canonicalNodeIds.get(target);
        if (canonical !== undefined && targetId !== undefined && targetId !== canonical) {
          throw new TypeError("Invalid trusted event target");
        }
      }
    }

    // Freeze dispatch metadata from internal state before touching page-owned accessors
    const rawType = eventState ? eventState.type : (event._type !== undefined ? event._type : event.type);
    if (typeof rawType !== "string") {
      throw new TypeError("Failed to execute 'dispatchEvent': parameter 1 is not of type 'Event'.");
    }
    const eventType = rawType.toLowerCase();
    const isBubbles = eventState ? Boolean(eventState.bubbles) : Boolean(event.bubbles);
    const isComposed = eventState ? Boolean(eventState.composed) : Boolean(event.composed);

    if (typeof _setEventTrusted === "function") {
      _setEventTrusted(event, Boolean(trusted));
    } else {
      event._isTrusted = Boolean(trusted);
    }

    // Access relatedTarget while event is locked so re-entrant getters throw InvalidStateError
    // and thrown getters cleanly unlock in finally
    const origRelated = event.relatedTarget;
    void origRelated;

    if (typeof window !== "undefined") {
      try { window.event = event; } catch (e) {}
    }
    if (eventState) {
      eventState.target = target;
    } else {
      event.target = target;
    }
    const targetRoot = getNodeRoot(target);

    // 1. Build propagation path from target up to window (supporting ShadowRoot & <slot>)
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
        if (typeof window !== "undefined" && !visited.has(window) && eventType !== "load") {
          path.push(window);
        }
        break;
      }

      // If curr is a slottable child of a shadow host, route through its assigned <slot>
      if ((curr.nodeType === 1 || curr.nodeType === 3) && curr.parentNode && curr.parentNode._shadowRoot) {
        if (typeof global.__capsurf_findAssignedSlot === "function") {
          const assignedSlot = global.__capsurf_findAssignedSlot(curr, true);
          if (assignedSlot && !visited.has(assignedSlot)) {
            curr = assignedSlot;
            continue;
          }
        }
      }

      // If curr is a ShadowRoot, cross to host only if event.composed or curr !== targetRoot
      if (isShadowRootNode(curr)) {
        if (isComposed || curr !== targetRoot) {
          curr = curr.host;
          continue;
        } else {
          break;
        }
      }

      let parent = null;
      try {
        parent = curr.parentNode || curr.parentElement;
      } catch (e) {}

      if (!parent) {
        if (typeof document !== "undefined" && curr !== document && !isShadowRootNode(curr) && curr.isConnected && !visited.has(document)) {
          parent = document;
        }
      }
      curr = parent;
    }

    if (eventState) {
      eventState.path = path;
    }

    function isStopped() {
      return eventState ? eventState.stopPropagation : Boolean(event._stopped);
    }
    function isImmediateStopped() {
      return eventState ? eventState.stopImmediatePropagation : Boolean(event._immediateStopped);
    }

    let _inErrorReportingMode = false;
    function reportListenerError(e) {
      if (typeof console !== "undefined" && console.error) console.error(e);
      if (_inErrorReportingMode) return;
      _inErrorReportingMode = true;
      try {
        const win = typeof window !== "undefined" ? window : global;
        const msg = (e && e.message) ? String(e.message) : String(e);
        if (win && typeof win.onerror === "function") {
          try {
            win.onerror.call(win, msg, "", 0, 0, e);
          } catch (e2) {}
        }
      } finally {
        _inErrorReportingMode = false;
      }
    }

    function invoke(entry, phase) {
      const retargeted = retargetEventTarget(target, entry);
      if (eventState) {
        eventState.target = retargeted;
        eventState.currentTarget = entry;
        eventState.eventPhase = phase;
      } else {
        event.target = retargeted;
        event.currentTarget = entry;
        event.eventPhase = phase;
      }

      const map = getListeners(entry);
      if (map) {
        const list = map.get(eventType);
        if (list && list.length > 0) {
          const copy = list.slice();
          for (let i = 0; i < copy.length; i++) {
            if (isImmediateStopped()) return;
            const item = copy[i];
            if (phase === PHASE_CAPTURING && !item.capture) continue;
            if (phase === PHASE_BUBBLING && item.capture) continue;

            if (item.once) {
              entry.removeEventListener(eventType, item.callback, item.capture);
            }
            try {
              if (typeof item.callback === "function") {
                item.callback.call(entry, event);
              } else if (item.callback && typeof item.callback === "object") {
                const handleFn = item.callback.handleEvent;
                if (typeof handleFn === "function") {
                  handleFn.call(item.callback, event);
                }
              }
            } catch (e) {
              reportListenerError(e);
            }
          }
        }
      }

      // Inline event handler (e.g. element.onclick, element.onsubmit)
      if ((phase === PHASE_AT_TARGET || phase === PHASE_BUBBLING) && !event._handledByOnProp) {
        const propName = "on" + eventType;
        let hydrated = hydratedHandlersMap.get(entry);
        if (!hydrated) {
          hydrated = Object.create(null);
          hydratedHandlersMap.set(entry, hydrated);
        }
        const hasOwnProp = Object.prototype.hasOwnProperty.call(entry, propName);
        if (!hydrated[propName]) {
          hydrated[propName] = true;
          if (!hasOwnProp && typeof entry.getAttribute === "function") {
            const attrVal = entry.getAttribute(propName);
            if (typeof attrVal === "string" && attrVal && entry[propName] == null) {
              try {
                entry[propName] = new Function("event", attrVal);
              } catch (e) {}
            }
          } else if (
            !hasOwnProp &&
            (entry === global || (typeof window !== "undefined" && entry === window)) &&
            typeof document !== "undefined" &&
            document &&
            document.body &&
            typeof document.body.getAttribute === "function"
          ) {
            const WINDOW_BODY_HANDLERS = {
              onafterprint: 1, onbeforeprint: 1, onbeforeunload: 1, onblur: 1,
              onerror: 1, onfocus: 1, onhashchange: 1, onlanguagechange: 1,
              onload: 1, onmessage: 1, onmessageerror: 1, onoffline: 1,
              ononline: 1, onpagehide: 1, onpageshow: 1, onpopstate: 1,
              onrejectionhandled: 1, onresize: 1, onscroll: 1, onstorage: 1,
              onunhandledrejection: 1, onunload: 1,
            };
            if (WINDOW_BODY_HANDLERS[propName] && !Object.prototype.hasOwnProperty.call(document.body, propName)) {
              const bodyAttr = document.body.getAttribute(propName);
              if (typeof bodyAttr === "string" && bodyAttr && entry[propName] === undefined) {
                try {
                  entry[propName] = new Function("event", bodyAttr);
                } catch (e) {}
              }
            }
          }
        }
        const fn = entry[propName];
        if (typeof fn === "function" && !isImmediateStopped()) {
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
      if (isStopped()) break;
      invoke(path[i], PHASE_CAPTURING);
    }

    // Phase 2: At Target
    if (!isStopped()) {
      invoke(target, PHASE_AT_TARGET);
      if (eventType === "click") {
        checkActivation(target);
      }
    }

    // Phase 3: Bubbling (from target's parent up to top, if bubbles is true)
    if (isBubbles && !isStopped()) {
      for (let i = 1; i < path.length; i++) {
        if (isStopped()) break;
        invoke(path[i], PHASE_BUBBLING);
        if (eventType === "click") {
          checkActivation(path[i]);
        }
      }
    }

    // Activation behavior for click events
    const wasCanceled = eventState ? eventState.canceled : Boolean(event._canceled);
    if (eventType === "click" && !wasCanceled) {
      if (activatedButton && typeof handleButtonActivation === "function") {
        handleButtonActivation(activatedButton);
      } else if (!event._ksSelectHandled && typeof global.__capsurf_toggleSelectDropdown === "function") {
        let selCur = target;
        while (selCur && selCur.nodeType === 1) {
          const selTag = String(selCur.localName || selCur.tagName || "").toLowerCase();
          if (selTag === "select") {
            event._ksSelectHandled = true;
            global.__capsurf_toggleSelectDropdown(selCur);
            break;
          }
          selCur = selCur.parentNode;
        }
      }
    }
  } finally {
    _eventsInFlight.delete(event);
    if (eventState) {
      eventState.isDispatching = false;
      eventState.eventPhase = PHASE_NONE;
      eventState.currentTarget = null;
      eventState.stopPropagation = false;
      eventState.stopImmediatePropagation = false;
      eventState.path = null;
    } else {
      event._isDispatching = false;
      event.eventPhase = PHASE_NONE;
      event.currentTarget = null;
    }
    if (typeof window !== "undefined") {
      try { window.event = prevWindowEvent; } catch (e) {}
    }
  }

  return eventState ? !eventState.canceled : !event._canceled;
}

EventTarget.prototype.dispatchEvent = function (event) {
  if (!event || typeof event !== "object") {
    throw new TypeError("Failed to execute 'dispatchEvent': parameter 1 is not of type 'Event'.");
  }
  if (_eventsInFlight.has(event)) {
    throw new DOMException("The event is already being dispatched.", "InvalidStateError");
  }
  return internalDispatch(this, event, false);
};

function dispatchTrustedEvent(target, event) {
  if (!target || !event) return false;
  const st = typeof _eventState !== "undefined" ? _eventState.get(event) : null;
  if (!st && typeof event._type !== "string" && typeof event.type !== "string") {
    const fallbackCtor = typeof MouseEvent === "function" ? MouseEvent : (typeof Event === "function" ? Event : null);
    if (fallbackCtor) {
      event = new fallbackCtor("click", { bubbles: true, cancelable: true, composed: true });
    }
  }
  return internalDispatch(target, event, true);
}
global.__capsurf_dispatchTrustedEvent = dispatchTrustedEvent;
if (typeof globalThis !== "undefined") {
  globalThis.__capsurf_dispatchTrustedEvent = dispatchTrustedEvent;
}

markNative(EventTarget, "EventTarget");
markNative(EventTarget.prototype.addEventListener, "addEventListener");
markNative(EventTarget.prototype.removeEventListener, "removeEventListener");
markNative(EventTarget.prototype.dispatchEvent, "dispatchEvent");
