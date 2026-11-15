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
