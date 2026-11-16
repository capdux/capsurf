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
  let timerIdCounter = 1;
  const activeTimers = new Map();

  if (!global.setTimeout) {
    global.setTimeout = markNative(function (cb, ms) {
      const id = timerIdCounter++;
      const args = Array.prototype.slice.call(arguments, 2);
      activeTimers.set(id, true);
      Promise.resolve().then(function () {
        if (activeTimers.has(id)) {
          activeTimers.delete(id);
          try {
            if (typeof cb === "function") {
              cb.apply(global, args);
            } else if (typeof cb === "string") {
              (new Function(cb))();
            }
          } catch (e) {
            if (typeof console !== "undefined" && console.error) console.error(e);
          }
        }
      });
      return id;
    }, "setTimeout");
  }

  if (!global.clearTimeout) {
    global.clearTimeout = markNative(function (id) {
      activeTimers.delete(Number(id));
    }, "clearTimeout");
  }

  if (!global.setInterval) {
    global.setInterval = markNative(function (cb, ms) {
      const id = timerIdCounter++;
      activeTimers.set(id, true);
      function tick() {
        if (!activeTimers.has(id)) return;
        try {
          if (typeof cb === "function") cb();
        } catch (e) {}
      }
      return id;
    }, "setInterval");
  }

  if (!global.clearInterval) {
    global.clearInterval = markNative(function (id) {
      activeTimers.delete(Number(id));
    }, "clearInterval");
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

  // 10. Unforgeable window properties (WHATWG Window IDL: [Unforgeable, Readonly])
  if (global.document) {
    try {
      Object.defineProperty(global, "document", {
        value: global.document,
        writable: false,
        configurable: false,
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
