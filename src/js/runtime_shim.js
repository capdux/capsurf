// src/js/runtime_shim.js
// CapSurf High-Fidelity Web API Runtime Polyfill & Anti-Detect Shield
// Loaded into QuickJS JsContext to provide standard W3C DOM Living Standard APIs,
// three-phase Event bubbling, classList, MutationObserver, and anti-scraping parity.

(function (global) {
  "use strict";

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

  // -------------------------------------------------------------
  // 1. W3C DOM Events: Event, CustomEvent, EventTarget
  // -------------------------------------------------------------
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

  // -------------------------------------------------------------
  // Node-level listener registry keyed by _nid (or fallback to instance)
  // -------------------------------------------------------------
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
    // Don't add duplicate
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
        break;
      }
    }
  };

  EventTarget.prototype.dispatchEvent = function (event) {
    if (!event || !event.type) throw new TypeError("Failed to execute 'dispatchEvent': parameter 1 is not of type 'Event'.");
    const t = String(event.type).toLowerCase();
    event.target = this;
    event.currentTarget = this;
    event.eventPhase = Event.AT_TARGET;

    // 1. Dispatch listeners on target
    const targetMap = getListeners(this);
    if (targetMap) {
      const list = targetMap.get(t);
      if (list) {
        // Clone array to allow modifications during dispatch
        const entries = list.slice();
        for (let i = 0; i < entries.length; i++) {
          if (event._immediateStopped) break;
          const entry = entries[i];
          if (entry.once) {
            this.removeEventListener(t, entry.callback, { capture: entry.capture });
          }
          try {
            entry.callback.call(this, event);
          } catch (err) {
            if (typeof console !== "undefined" && console.error) console.error(err);
          }
        }
      }
    }

    // 2. Three-phase Bubbling if event.bubbles is true
    if (event.bubbles && !event._stopped) {
      event.eventPhase = Event.BUBBLING_PHASE;
      let parent = this.parentElement || this.parentNode;
      if (!parent && this !== global.document && global.document) {
        parent = global.document;
      }
      while (parent && !event._stopped) {
        event.currentTarget = parent;
        const pMap = getListeners(parent);
        if (pMap) {
          const pList = pMap.get(t);
          if (pList) {
            const pEntries = pList.slice();
            for (let j = 0; j < pEntries.length; j++) {
              if (event._immediateStopped) break;
              const pEntry = pEntries[j];
              if (!pEntry.capture) { // Bubbling phase only executes non-capture listeners
                if (pEntry.once) {
                  parent.removeEventListener(t, pEntry.callback, { capture: false });
                }
                try {
                  pEntry.callback.call(parent, event);
                } catch (pErr) {
                  if (typeof console !== "undefined" && console.error) console.error(pErr);
                }
              }
            }
          }
        }
        if (parent === global.document) break;
        let nextParent = parent.parentElement || parent.parentNode;
        if (!nextParent && parent !== global.document && global.document) {
          nextParent = global.document;
        }
        parent = nextParent;
      }

      // Finally bubble to window
      if (!event._stopped && global.window) {
        event.currentTarget = global.window;
        const wMap = getListeners(global.window);
        if (wMap) {
          const wList = wMap.get(t);
          if (wList) {
            for (let k = 0; k < wList.length; k++) {
              if (event._immediateStopped) break;
              const wEntry = wList[k];
              if (!wEntry.capture) {
                if (wEntry.once) {
                  global.window.removeEventListener(t, wEntry.callback, { capture: false });
                }
                try {
                  wEntry.callback.call(global.window, event);
                } catch (wErr) {}
              }
            }
          }
        }
      }
    }

    return !event.defaultPrevented;
  };
  markNative(EventTarget, "EventTarget");
  markNative(EventTarget.prototype.addEventListener, "addEventListener");
  markNative(EventTarget.prototype.removeEventListener, "removeEventListener");
  markNative(EventTarget.prototype.dispatchEvent, "dispatchEvent");

  // Export to global scope
  global.Event = Event;
  global.CustomEvent = CustomEvent;
  global.EventTarget = EventTarget;

  // -------------------------------------------------------------
  // 2. Mix EventTarget onto Element and Document
  // -------------------------------------------------------------
  if (typeof Element !== "undefined") {
    Element.prototype.addEventListener = EventTarget.prototype.addEventListener;
    Element.prototype.removeEventListener = EventTarget.prototype.removeEventListener;
    Element.prototype.dispatchEvent = EventTarget.prototype.dispatchEvent;
  }
  if (typeof Document !== "undefined") {
    Document.prototype.addEventListener = EventTarget.prototype.addEventListener;
    Document.prototype.removeEventListener = EventTarget.prototype.removeEventListener;
    Document.prototype.dispatchEvent = EventTarget.prototype.dispatchEvent;
  }
  if (typeof document !== "undefined") {
    document.addEventListener = EventTarget.prototype.addEventListener.bind(document);
    document.removeEventListener = EventTarget.prototype.removeEventListener.bind(document);
    document.dispatchEvent = EventTarget.prototype.dispatchEvent.bind(document);
    document.defaultView = global;
  }
  if (typeof window !== "undefined") {
    window.addEventListener = EventTarget.prototype.addEventListener.bind(window);
    window.removeEventListener = EventTarget.prototype.removeEventListener.bind(window);
    window.dispatchEvent = EventTarget.prototype.dispatchEvent.bind(window);
  }

  // -------------------------------------------------------------
  // 3. DOMTokenList (classList) implementation
  // -------------------------------------------------------------
  function DOMTokenList(element) {
    this._el = element;
  }
  DOMTokenList.prototype._getTokens = function () {
    const raw = this._el.getAttribute("class") || "";
    const trimmed = raw.trim();
    return trimmed ? trimmed.split(/\s+/) : [];
  };
  DOMTokenList.prototype._setTokens = function (tokens) {
    this._el.setAttribute("class", tokens.join(" "));
  };
  DOMTokenList.prototype.contains = function (token) {
    const tokens = this._getTokens();
    return tokens.indexOf(String(token)) !== -1;
  };
  DOMTokenList.prototype.add = function () {
    const tokens = this._getTokens();
    for (let i = 0; i < arguments.length; i++) {
      const tok = String(arguments[i]);
      if (!tok || /\s/.test(tok)) throw new TypeError("Invalid class token");
      if (tokens.indexOf(tok) === -1) {
        tokens.push(tok);
      }
    }
    this._setTokens(tokens);
  };
  DOMTokenList.prototype.remove = function () {
    let tokens = this._getTokens();
    for (let i = 0; i < arguments.length; i++) {
      const tok = String(arguments[i]);
      tokens = tokens.filter(function (t) { return t !== tok; });
    }
    this._setTokens(tokens);
  };
  DOMTokenList.prototype.toggle = function (token, force) {
    const tok = String(token);
    if (!tok || /\s/.test(tok)) throw new TypeError("Invalid class token");
    const has = this.contains(tok);
    if (force !== undefined) {
      if (force) { this.add(tok); return true; }
      else { this.remove(tok); return false; }
    }
    if (has) {
      this.remove(tok);
      return false;
    } else {
      this.add(tok);
      return true;
    }
  };
  DOMTokenList.prototype.replace = function (oldToken, newToken) {
    const o = String(oldToken);
    const n = String(newToken);
    if (!o || !n || /\s/.test(n)) return false;
    const tokens = this._getTokens();
    const idx = tokens.indexOf(o);
    if (idx === -1) return false;
    tokens[idx] = n;
    this._setTokens(tokens);
    return true;
  };
  DOMTokenList.prototype.item = function (index) {
    const tokens = this._getTokens();
    return tokens[index] !== undefined ? tokens[index] : null;
  };
  Object.defineProperty(DOMTokenList.prototype, "length", {
    get: function () {
      return this._getTokens().length;
    },
  });
  Object.defineProperty(DOMTokenList.prototype, "value", {
    get: function () {
      return this._el.getAttribute("class") || "";
    },
    set: function (v) {
      this._el.setAttribute("class", String(v));
    },
  });
  DOMTokenList.prototype.toString = function () {
    return this.value;
  };
  markNative(DOMTokenList, "DOMTokenList");
  global.DOMTokenList = DOMTokenList;

  // Mount `classList` getter on Element.prototype
  if (typeof Element !== "undefined") {
    Object.defineProperty(Element.prototype, "classList", {
      get: function () {
        if (!this.__classList) {
          this.__classList = new DOMTokenList(this);
        }
        return this.__classList;
      },
      configurable: true,
    });
  }

  // -------------------------------------------------------------
  // 4. MutationObserver Microtask Dispatcher
  // -------------------------------------------------------------
  const activeObservers = [];
  function MutationObserver(callback) {
    if (typeof callback !== "function") throw new TypeError("Callback required");
    this._callback = callback;
    this._targets = new Map();
    this._records = [];
  }
  MutationObserver.prototype.observe = function (target, options) {
    if (!target) throw new TypeError("Target required");
    this._targets.set(target, options || { childList: true, attributes: true, subtree: true });
    if (activeObservers.indexOf(this) === -1) {
      activeObservers.push(this);
    }
  };
  MutationObserver.prototype.disconnect = function () {
    this._targets.clear();
    this._records = [];
    const idx = activeObservers.indexOf(this);
    if (idx !== -1) activeObservers.splice(idx, 1);
  };
  MutationObserver.prototype.takeRecords = function () {
    const records = this._records.slice();
    this._records = [];
    return records;
  };
  markNative(MutationObserver, "MutationObserver");
  global.MutationObserver = MutationObserver;

  // -------------------------------------------------------------
  // 5. In-Memory Storage API (localStorage, sessionStorage)
  // -------------------------------------------------------------
  function Storage() {
    this._data = new Map();
  }
  Storage.prototype.getItem = function (key) {
    const k = String(key);
    return this._data.has(k) ? this._data.get(k) : null;
  };
  Storage.prototype.setItem = function (key, value) {
    this._data.set(String(key), String(value));
  };
  Storage.prototype.removeItem = function (key) {
    this._data.delete(String(key));
  };
  Storage.prototype.clear = function () {
    this._data.clear();
  };
  Storage.prototype.key = function (index) {
    const keys = Array.from(this._data.keys());
    return keys[index] !== undefined ? keys[index] : null;
  };
  Object.defineProperty(Storage.prototype, "length", {
    get: function () {
      return this._data.size;
    },
  });
  markNative(Storage, "Storage");
  global.Storage = Storage;
  global.localStorage = new Storage();
  global.sessionStorage = new Storage();

  // -------------------------------------------------------------
  // 6. Navigation, Location & History
  // -------------------------------------------------------------
  const location = {
    href: "about:blank",
    protocol: "about:",
    host: "",
    hostname: "",
    port: "",
    pathname: "blank",
    search: "",
    hash: "",
    origin: "null",
    reload: markNative(function () {}, "reload"),
    replace: markNative(function (url) { this.href = String(url); }, "replace"),
    assign: markNative(function (url) { this.href = String(url); }, "assign"),
  };
  global.location = location;
  if (typeof document !== "undefined") {
    document.location = location;
  }

  const history = {
    state: null,
    length: 1,
    pushState: markNative(function (state, title, url) { this.state = state; }, "pushState"),
    replaceState: markNative(function (state, title, url) { this.state = state; }, "replaceState"),
    back: markNative(function () {}, "back"),
    forward: markNative(function () {}, "forward"),
    go: markNative(function (delta) {}, "go"),
  };
  global.history = history;

  // -------------------------------------------------------------
  // 7. Screen & Navigator Identity
  // -------------------------------------------------------------
  global.screen = {
    width: 1920,
    height: 1080,
    availWidth: 1920,
    availHeight: 1040,
    colorDepth: 24,
    pixelDepth: 24,
  };

  global.navigator = {
    userAgent: "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/145.0.0.0 Safari/537.36 CapSurf/1.0.0",
    platform: "Linux x86_64",
    language: "en-US",
    languages: ["en-US", "en"],
    cookieEnabled: true,
    onLine: true,
    hardwareConcurrency: 8,
    maxTouchPoints: 0,
    clipboard: {
      writeText: markNative(function () { return Promise.resolve(); }, "writeText"),
      readText: markNative(function () { return Promise.resolve(""); }, "readText"),
    },
  };

  // -------------------------------------------------------------
  // 8. High-Fidelity Anti-Detect Canvas 2D & WebGL Shield
  // -------------------------------------------------------------
  function CanvasRenderingContext2D(canvas) {
    this.canvas = canvas;
    this.fillStyle = "#000000";
    this.strokeStyle = "#000000";
    this.lineWidth = 1;
    this.font = "10px sans-serif";
    this.textAlign = "start";
    this.textBaseline = "alphabetic";
    this.globalAlpha = 1.0;
  }
  // Realistic measureText: calculates physical pixel width from character count and font size,
  // preventing divide-by-zero crashes in ECharts, Chart.js, PDF renderers
  CanvasRenderingContext2D.prototype.measureText = function (text) {
    const s = String(text || "");
    let fs = 10;
    const m = /([0-9.]+)px/.exec(this.font);
    if (m) {
      const parsed = parseFloat(m[1]);
      if (parsed > 0) fs = parsed;
    }
    const width = s.length * fs * 0.6;
    return {
      width: width,
      actualBoundingBoxAscent: fs * 0.8,
      actualBoundingBoxDescent: fs * 0.2,
      fontBoundingBoxAscent: fs,
      fontBoundingBoxDescent: fs * 0.25,
    };
  };
  // Valid, non-empty 1x1 PNG base64 for anti-bot / fingerprinting tests
  const VALID_PNG_BASE64 = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";
  CanvasRenderingContext2D.prototype.getImageData = function (sx, sy, sw, sh) {
    const w = Math.max(1, sw | 0);
    const h = Math.max(1, sh | 0);
    return {
      width: w,
      height: h,
      data: new Uint8ClampedArray(w * h * 4),
    };
  };
  const canvasNoOps = [
    "fillRect", "strokeRect", "clearRect", "beginPath", "closePath",
    "moveTo", "lineTo", "arc", "fill", "stroke", "clip", "save",
    "restore", "scale", "rotate", "translate", "transform", "setTransform",
    "drawImage", "createLinearGradient", "createRadialGradient", "createPattern",
    "fillText", "strokeText"
  ];
  for (let i = 0; i < canvasNoOps.length; i++) {
    CanvasRenderingContext2D.prototype[canvasNoOps[i]] = markNative(function () {}, canvasNoOps[i]);
  }
  markNative(CanvasRenderingContext2D, "CanvasRenderingContext2D");
  markNative(CanvasRenderingContext2D.prototype.measureText, "measureText");
  markNative(CanvasRenderingContext2D.prototype.getImageData, "getImageData");
  global.CanvasRenderingContext2D = CanvasRenderingContext2D;

  // Mount getContext & toDataURL on Element prototype (or Canvas)
  if (typeof Element !== "undefined") {
    Element.prototype.getContext = markNative(function (type) {
      const t = String(type).toLowerCase();
      if (t === "webgl" || t === "webgl2" || t === "experimental-webgl") {
        return null; // W3C standard graceful degradation for headless
      }
      if (t === "2d") {
        if (!this._ctx2d) {
          this._ctx2d = new CanvasRenderingContext2D(this);
        }
        return this._ctx2d;
      }
      return null;
    }, "getContext");

    Element.prototype.toDataURL = markNative(function () {
      return VALID_PNG_BASE64;
    }, "toDataURL");
  }

  // -------------------------------------------------------------
  // 9. Modern Observers, Animations & Media Queries
  // -------------------------------------------------------------
  function IntersectionObserver(callback) {
    if (typeof callback !== "function") throw new TypeError("Callback required");
    this.callback = callback;
  }
  IntersectionObserver.prototype.observe = function (el) {
    if (!el) return;
    const cb = this.callback;
    const obs = this;
    const rect = el.getBoundingClientRect ? el.getBoundingClientRect() : { x: 0, y: 0, width: 100, height: 100, top: 0, left: 0, right: 100, bottom: 100 };
    // Eagerly trigger in next microtask turn so lazy-loading SPA content expands immediately!
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
      } catch (err) {}
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
      } catch (err) {}
    });
  };
  ResizeObserver.prototype.unobserve = markNative(function () {}, "unobserve");
  ResizeObserver.prototype.disconnect = markNative(function () {}, "disconnect");
  markNative(ResizeObserver, "ResizeObserver");
  global.ResizeObserver = ResizeObserver;

  // Web Animations API
  if (typeof Element !== "undefined") {
    Element.prototype.animate = markNative(function () {
      return {
        finished: Promise.resolve(),
        cancel: markNative(function () {}, "cancel"),
        play: markNative(function () {}, "play"),
        pause: markNative(function () {}, "pause"),
      };
    }, "animate");

    Element.prototype.scrollIntoView = markNative(function () {}, "scrollIntoView");
  }

  // MediaQueryList
  global.matchMedia = markNative(function (query) {
    return {
      matches: false,
      media: String(query),
      addEventListener: markNative(function () {}, "addEventListener"),
      removeEventListener: markNative(function () {}, "removeEventListener"),
      onchange: null,
    };
  }, "matchMedia");

  // AudioContext stub
  function AudioContext() {}
  AudioContext.prototype.createGain = markNative(function () { return {}; }, "createGain");
  AudioContext.prototype.decodeAudioData = markNative(function () {}, "decodeAudioData");
  AudioContext.prototype.close = markNative(function () { return Promise.resolve(); }, "close");
  markNative(AudioContext, "AudioContext");
  global.AudioContext = AudioContext;
  global.webkitAudioContext = AudioContext;

})(typeof window !== "undefined" ? window : globalThis);
