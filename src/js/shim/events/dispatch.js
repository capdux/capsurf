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
  const isHarness = Boolean(event._isHarnessEvent);
  event._isTrusted = false;
  return internalDispatch(this, event, isHarness);
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
