// src/js/shim/window.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/window.ts

function installWindow(global) {
  const NativeEvent = global.Event || function (type, init) {
    this.type = type;
    this.bubbles = !!init?.bubbles;
    this.cancelable = !!init?.cancelable;
  };
  global.__NativeEvent = NativeEvent;

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

  const historyStack = [{ state: null, url: currentHref }];
  let historyIdx = 0;

  const historyObj = {
    get length() { return historyStack.length; },
    get state() { return historyStack[historyIdx]?.state ?? null; },
    pushState: markNative(function (st, title, url) {
      if (url) {
        try {
          const oldUrl = currentHref;
          const newUrl = new URL(url, currentHref).href;
          currentHref = newUrl;
          if (oldUrl.split("#")[0] === newUrl.split("#")[0] && newUrl.includes("#")) {
            setTimeout(() => {
              try { global.dispatchEvent(new NativeEvent("hashchange")); } catch(e) {}
            }, 0);
          }
        } catch (e) {
          currentHref = String(url);
        }
      }
      historyIdx++;
      historyStack.splice(historyIdx, historyStack.length - historyIdx, { state: st, url: currentHref });
    }, "pushState"),
    replaceState: markNative(function (st, title, url) {
      if (url) {
        try { currentHref = new URL(url, currentHref).href; } catch (e) { currentHref = String(url); }
      }
      historyStack[historyIdx] = { state: st, url: currentHref };
    }, "replaceState"),
    back: markNative(function () {
      if (historyIdx > 0) {
        historyIdx--;
        currentHref = historyStack[historyIdx].url;
        setTimeout(() => {
          try {
            global.dispatchEvent(new NativeEvent("popstate"));
          } catch (e) {}
        }, 0);
      }
    }, "back"),
    forward: markNative(function () {
      if (historyIdx + 1 < historyStack.length) {
        historyIdx++;
        currentHref = historyStack[historyIdx].url;
        setTimeout(() => {
          try {
            global.dispatchEvent(new NativeEvent("popstate"));
          } catch (e) {}
        }, 0);
      }
    }, "forward"),
    go: markNative(function (delta) {
      const target = historyIdx + (Number(delta) || 0);
      if (target >= 0 && target < historyStack.length) {
        historyIdx = target;
        currentHref = historyStack[historyIdx].url;
        setTimeout(() => {
          try {
            global.dispatchEvent(new NativeEvent("popstate"));
          } catch (e) {}
        }, 0);
      }
    }, "go"),
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
