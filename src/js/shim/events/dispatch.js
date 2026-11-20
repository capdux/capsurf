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

  const prevWindowEvent = typeof window !== "undefined" ? window.event : undefined;
  const eventState = typeof _eventState !== "undefined" ? _eventState.get(event) : null;
  try {
    if (typeof window !== "undefined") {
      try { window.event = event; } catch (e) {}
    }
    event.target = target;
    const rawType = event._type !== undefined ? event._type : event.type;
    if (typeof rawType !== "string") {
      throw new TypeError("Failed to execute 'dispatchEvent': parameter 1 is not of type 'Event'.");
    }
    const eventType = rawType.toLowerCase();
    const isComposed = Boolean(event.composed);
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
        if (typeof window !== "undefined" && !visited.has(window)) {
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

    function invoke(entry, phase) {
      event.target = retargetEventTarget(target, entry);
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
              if (typeof item.callback === "function") {
                item.callback.call(entry, event);
              } else if (item.callback && typeof item.callback === "object") {
                const handleFn = item.callback.handleEvent;
                if (typeof handleFn === "function") {
                  handleFn.call(item.callback, event);
                }
              }
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
        const hasOwnProp = Object.prototype.hasOwnProperty.call(entry, propName);
        if (!entry._hydratedHandlers[propName]) {
          entry._hydratedHandlers[propName] = true;
          if (!hasOwnProp && typeof entry.getAttribute === "function") {
            const attrVal = entry.getAttribute(propName);
            if (typeof attrVal === "string" && attrVal && typeof entry[propName] !== "function") {
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
              if (typeof bodyAttr === "string" && bodyAttr && typeof entry[propName] !== "function") {
                try {
                  entry[propName] = new Function("event", bodyAttr);
                } catch (e) {}
              }
            }
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
    if (eventState) {
      eventState.path = null;
    }
    if (typeof window !== "undefined") {
      try { window.event = prevWindowEvent; } catch (e) {}
    }
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
