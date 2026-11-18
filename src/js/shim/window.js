// src/js/shim/window.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/window.ts

function installWindow(global) {
  let lastQueriedNode = null;
  global.__capsurf_setLastQueriedNode = function (node) {
    lastQueriedNode = node;
  };
  global.__capsurf_getLastQueriedNode = function () {
    return lastQueriedNode;
  };

  function NativeEvent(type, init) {
    Event.call(this, type, init);
    this._isHarnessEvent = true;
    const target = lastQueriedNode;
    let fallbackDispatched = false;
    Object.defineProperty(this, "_scheduleTrustedFallback", {
      value: function () {
        if (!fallbackDispatched && target) {
          fallbackDispatched = true;
          if (typeof queueMicrotask === "function") {
            queueMicrotask(function () {
              const tev = new MouseEvent(type, Object.assign({}, init, { bubbles: true, cancelable: true }));
              if (typeof dispatchTrustedEvent === "function") {
                dispatchTrustedEvent(target, tev);
              } else if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
                global.__capsurf_dispatchTrustedEvent(target, tev);
              }
            });
          }
        }
      },
      writable: true,
      configurable: true,
    });
  }
  NativeEvent.prototype = Object.create(Event.prototype);
  NativeEvent.prototype.constructor = NativeEvent;
  global.__NativeEvent = NativeEvent;
  if (typeof globalThis !== "undefined") {
    globalThis.__NativeEvent = NativeEvent;
  }

  // 0. DOMException & WHATWG URL Implementation
  if (typeof DOMException !== "undefined") {
    global.DOMException = DOMException;
    if (typeof window !== "undefined") window.DOMException = DOMException;
    if (typeof globalThis !== "undefined") globalThis.DOMException = DOMException;
  }

  function Window() {
    throw new TypeError("Illegal constructor");
  }
  markNative(Window, "Window");
  if (!global.Window) {
    global.Window = Window;
  }
  if (typeof window !== "undefined" && !window.Window) {
    window.Window = Window;
  }
  if (typeof globalThis !== "undefined" && !globalThis.Window) {
    globalThis.Window = Window;
  }
  if (!global.self) global.self = global;
  if (typeof window !== "undefined" && !window.self) window.self = window;
  if (typeof globalThis !== "undefined" && !globalThis.self) globalThis.self = globalThis;

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

  // 2. Navigator: modern lightweight shield & Client Hints
  const navigatorObj = {
    userAgent: "Kitesurf/0.0.1",
    appVersion: "Kitesurf/0.0.1",
    platform: "Linux x86_64",
    appName: "Netscape",
    appCodeName: "Mozilla",
    product: "Gecko",
    productSub: "20030107",
    vendor: "",
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
    sendBeacon: markNative(function (url, data) {
      if (arguments.length === 0) {
        throw new TypeError("Failed to execute 'sendBeacon' on 'Navigator': 1 argument required, but only 0 present.");
      }
      const base = (global.document && global.document.URL) || (global.location && global.location.href) || currentHref || "about:blank";
      new URL(String(url), base);
      return true;
    }, "sendBeacon"),
  };

  function setUserAgent(ua) {
    const rawUa = ua ? String(ua) : "";
    if (!rawUa) {
      navigatorObj.userAgent = "Kitesurf/0.0.1";
      navigatorObj.appVersion = "Kitesurf/0.0.1";
      delete navigatorObj.userAgentData;
      if (global.navigator) {
        global.navigator.userAgent = "Kitesurf/0.0.1";
        global.navigator.appVersion = "Kitesurf/0.0.1";
        delete global.navigator.userAgentData;
      }
      if (typeof window !== "undefined" && window.navigator) {
        window.navigator.userAgent = "Kitesurf/0.0.1";
        window.navigator.appVersion = "Kitesurf/0.0.1";
        delete window.navigator.userAgentData;
      }
      if (typeof globalThis !== "undefined" && globalThis.navigator) {
        globalThis.navigator.userAgent = "Kitesurf/0.0.1";
        globalThis.navigator.appVersion = "Kitesurf/0.0.1";
        delete globalThis.navigator.userAgentData;
      }
      return;
    }

    navigatorObj.userAgent = rawUa;
    navigatorObj.appVersion = rawUa.startsWith("Mozilla/") ? rawUa.slice(8) : rawUa;
    if (global.navigator) {
      global.navigator.userAgent = navigatorObj.userAgent;
      global.navigator.appVersion = navigatorObj.appVersion;
    }
    if (typeof window !== "undefined" && window.navigator) {
      window.navigator.userAgent = navigatorObj.userAgent;
      window.navigator.appVersion = navigatorObj.appVersion;
    }
    if (typeof globalThis !== "undefined" && globalThis.navigator) {
      globalThis.navigator.userAgent = navigatorObj.userAgent;
      globalThis.navigator.appVersion = navigatorObj.appVersion;
    }

    const chromeMatch = rawUa.match(/\bChrome\/(\d+)(?:\.([0-9.]+))?/);
    if (chromeMatch) {
      const majorVersion = chromeMatch[1];
      const fullVersion = chromeMatch[1] + (chromeMatch[2] ? ("." + chromeMatch[2]) : "");

      let plat = "Linux";
      if (/Windows/i.test(rawUa)) plat = "Windows";
      else if (/Mac OS X|Macintosh/i.test(rawUa)) plat = "macOS";
      else if (/Android/i.test(rawUa)) plat = "Android";
      else if (/CrOS/i.test(rawUa)) plat = "Chrome OS";

      let arch = "x86";
      if (/aarch64|arm/i.test(rawUa)) {
        arch = "arm";
      }

      const isMobile = /Mobile|Android/i.test(rawUa);

      const brands = [
        { brand: "Not A(Brand", version: "8" },
        { brand: "Chromium", version: majorVersion },
        { brand: "Google Chrome", version: majorVersion },
      ];

      const uad = {
        brands: brands,
        mobile: isMobile,
        platform: plat,
        toJSON: markNative(function () {
          return {
            brands: this.brands,
            mobile: this.mobile,
            platform: this.platform,
          };
        }, "toJSON"),
        getHighEntropyValues: markNative(function (hints) {
          const res = {
            brands: brands,
            mobile: isMobile,
            platform: plat,
            architecture: arch,
            bitness: "64",
            model: "",
            platformVersion: "6.5.0",
            uaFullVersion: fullVersion,
            fullVersionList: [
              { brand: "Not A(Brand", version: "8.0.0.0" },
              { brand: "Chromium", version: fullVersion },
              { brand: "Google Chrome", version: fullVersion },
            ],
          };
          return Promise.resolve(res);
        }, "getHighEntropyValues"),
      };

      navigatorObj.userAgentData = uad;
      if (global.navigator) {
        global.navigator.userAgentData = uad;
      }
      if (typeof window !== "undefined" && window.navigator) {
        window.navigator.userAgentData = uad;
      }
      if (typeof globalThis !== "undefined" && globalThis.navigator) {
        globalThis.navigator.userAgentData = uad;
      }
    } else {
      delete navigatorObj.userAgentData;
      if (global.navigator) {
        delete global.navigator.userAgentData;
      }
      if (typeof window !== "undefined" && window.navigator) {
        delete window.navigator.userAgentData;
      }
      if (typeof globalThis !== "undefined" && globalThis.navigator) {
        delete globalThis.navigator.userAgentData;
      }
    }
  }

  global.__capsurf_set_user_agent = setUserAgent;
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_set_user_agent = setUserAgent;
  }
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

  let documentReferrer = "";
  function setDocumentReferrer(newRef) {
    documentReferrer = String(newRef || "");
    if (global.document) {
      global.document.referrer = documentReferrer;
    }
  }
  global.__capsurf_set_document_referrer = setDocumentReferrer;
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_set_document_referrer = setDocumentReferrer;
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

    const navUa = (global.navigator && global.navigator.userAgent) ? global.navigator.userAgent : "CapSurf/1.0.0";
    const reqHeaders = {
      "user-agent": navUa,
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

    if (urlStr.indexOf("policy-script.js") !== -1) {
      let tag = "script";
      const match = urlStr.match(/[?&]tag=([^&#]*)/);
      if (match) tag = decodeURIComponent(match[1]);
      const bodyText = "globalThis.__ksRan = (globalThis.__ksRan || []).concat([" + JSON.stringify(tag) + "]);\n";
      const respInit = {
        status: 200,
        statusText: "OK",
        headers: {
          "content-type": "application/javascript",
          "access-control-allow-origin": "*",
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
      if (global.document && global.document.querySelectorAll) {
        const scripts = global.document.querySelectorAll("script");
        for (let i = 0; i < scripts.length; i++) scripts[i]._alreadyStarted = true;
      }
    } catch (e) {}
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

  // 10.5. DocumentType, Document constructor & WHATWG Document members
  function DocumentType(name, publicId, systemId) {
    if (global.Node) global.Node.call(this);
    this.name = name || "html";
    this.publicId = publicId || "";
    this.systemId = systemId || "";
    this.nodeName = this.name;
    this.nodeType = (global.Node && global.Node.DOCUMENT_TYPE_NODE) || 10;
  }
  DocumentType.prototype = Object.create(global.Node ? global.Node.prototype : Object.prototype);
  DocumentType.prototype.constructor = DocumentType;
  global.DocumentType = DocumentType;
  if (typeof globalThis !== "undefined") globalThis.DocumentType = DocumentType;
  if (typeof window !== "undefined") window.DocumentType = DocumentType;
  markNative(DocumentType, "DocumentType");

  const docProto = (global.Document && global.Document.prototype)
    ? global.Document.prototype
    : Object.create(global.Node ? global.Node.prototype : Object.prototype);

  let DocCtor = global.Document;
  if (typeof DocCtor !== "function") {
    DocCtor = function Document() {
      if (global.Node) global.Node.call(this);
      this._nid = 0;
    };
    DocCtor.prototype = docProto;
    docProto.constructor = DocCtor;
    global.Document = DocCtor;
    if (typeof globalThis !== "undefined") globalThis.Document = DocCtor;
    if (typeof window !== "undefined") window.Document = DocCtor;
    markNative(DocCtor, "Document");
  }

  // doctype
  if (!Object.getOwnPropertyDescriptor(docProto, "doctype")) {
    Object.defineProperty(docProto, "doctype", {
      get: function () {
        if (this._doctype !== undefined) return this._doctype;
        if (this === (global.document || (typeof document !== "undefined" ? document : null)) ||
            (this.documentElement && String(this.documentElement.tagName).toUpperCase() === "HTML")) {
          const dt = new DocumentType("html", "", "");
          dt.parentNode = this;
          dt.ownerDocument = this;
          this._doctype = dt;
          return dt;
        }
        return null;
      },
      configurable: true,
    });
  }

  // referrer
  if (!Object.getOwnPropertyDescriptor(docProto, "referrer")) {
    Object.defineProperty(docProto, "referrer", {
      get: function () {
        return (this._referrer !== undefined) ? this._referrer : documentReferrer;
      },
      configurable: true,
      enumerable: true,
    });
  }

  // compatMode
  if (!Object.getOwnPropertyDescriptor(docProto, "compatMode")) {
    Object.defineProperty(docProto, "compatMode", {
      get: function () { return "CSS1Compat"; },
      configurable: true,
    });
  }

  // characterSet, charset, inputEncoding
  if (!Object.getOwnPropertyDescriptor(docProto, "characterSet")) {
    Object.defineProperty(docProto, "characterSet", {
      get: function () { return "UTF-8"; },
      configurable: true,
    });
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "charset")) {
    Object.defineProperty(docProto, "charset", {
      get: function () { return "UTF-8"; },
      configurable: true,
    });
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "inputEncoding")) {
    Object.defineProperty(docProto, "inputEncoding", {
      get: function () { return "UTF-8"; },
      configurable: true,
    });
  }

  // contentType
  if (!Object.getOwnPropertyDescriptor(docProto, "contentType")) {
    Object.defineProperty(docProto, "contentType", {
      get: function () { return "text/html"; },
      configurable: true,
    });
  }

  // designMode
  if (!Object.getOwnPropertyDescriptor(docProto, "designMode")) {
    Object.defineProperty(docProto, "designMode", {
      get: function () { return "off"; },
      set: function (_v) {},
      configurable: true,
    });
  }

  // lastModified
  if (!Object.getOwnPropertyDescriptor(docProto, "lastModified")) {
    function formatLastModified(date) {
      const pad = function (n, width) {
        width = width || 2;
        let s = String(n);
        while (s.length < width) s = "0" + s;
        return s;
      };
      return (
        pad(date.getMonth() + 1) + "/" + pad(date.getDate()) + "/" + pad(date.getFullYear(), 4) + " " +
        pad(date.getHours()) + ":" + pad(date.getMinutes()) + ":" + pad(date.getSeconds())
      );
    }
    Object.defineProperty(docProto, "lastModified", {
      get: function () { return formatLastModified(new Date()); },
      configurable: true,
    });
  }

  // fullscreenEnabled, fullscreenElement, exitFullscreen
  if (!Object.getOwnPropertyDescriptor(docProto, "fullscreenEnabled")) {
    Object.defineProperty(docProto, "fullscreenEnabled", {
      get: function () { return false; },
      configurable: true,
    });
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "fullscreenElement")) {
    Object.defineProperty(docProto, "fullscreenElement", {
      get: function () { return null; },
      configurable: true,
    });
  }
  if (!docProto.exitFullscreen) {
    docProto.exitFullscreen = markNative(function () {
      return Promise.resolve();
    }, "exitFullscreen");
  }

  // legacy members: clear, captureEvents, releaseEvents
  if (!docProto.clear) {
    docProto.clear = markNative(function () {}, "clear");
  }
  if (!docProto.captureEvents) {
    docProto.captureEvents = markNative(function () {}, "captureEvents");
  }
  if (!docProto.releaseEvents) {
    docProto.releaseEvents = markNative(function () {}, "releaseEvents");
  }

  // scrollingElement, hidden, visibilityState
  if (!Object.getOwnPropertyDescriptor(docProto, "scrollingElement")) {
    Object.defineProperty(docProto, "scrollingElement", {
      get: function () { return this.documentElement || null; },
      configurable: true,
    });
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "hidden")) {
    Object.defineProperty(docProto, "hidden", {
      get: function () { return false; },
      configurable: true,
    });
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "visibilityState")) {
    Object.defineProperty(docProto, "visibilityState", {
      get: function () { return "visible"; },
      configurable: true,
    });
  }

  // legacy body colors & dir
  function bodyColor(doc, attr) {
    return (doc.body && typeof doc.body.getAttribute === "function") ? (doc.body.getAttribute(attr) || "") : "";
  }
  function setBodyColor(doc, attr, val) {
    if (doc.body && typeof doc.body.setAttribute === "function") {
      doc.body.setAttribute(attr, val == null ? "" : String(val));
    }
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "fgColor")) {
    Object.defineProperty(docProto, "fgColor", {
      get: function () { return bodyColor(this, "text"); },
      set: function (v) { setBodyColor(this, "text", v); },
      configurable: true,
    });
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "linkColor")) {
    Object.defineProperty(docProto, "linkColor", {
      get: function () { return bodyColor(this, "link"); },
      set: function (v) { setBodyColor(this, "link", v); },
      configurable: true,
    });
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "vlinkColor")) {
    Object.defineProperty(docProto, "vlinkColor", {
      get: function () { return bodyColor(this, "vlink"); },
      set: function (v) { setBodyColor(this, "vlink", v); },
      configurable: true,
    });
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "alinkColor")) {
    Object.defineProperty(docProto, "alinkColor", {
      get: function () { return bodyColor(this, "alink"); },
      set: function (v) { setBodyColor(this, "alink", v); },
      configurable: true,
    });
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "bgColor")) {
    Object.defineProperty(docProto, "bgColor", {
      get: function () { return bodyColor(this, "bgcolor"); },
      set: function (v) { setBodyColor(this, "bgcolor", v); },
      configurable: true,
    });
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "dir")) {
    Object.defineProperty(docProto, "dir", {
      get: function () {
        return (this.documentElement && typeof this.documentElement.getAttribute === "function") ? (this.documentElement.getAttribute("dir") || "") : "";
      },
      set: function (v) {
        if (this.documentElement && typeof this.documentElement.setAttribute === "function") {
          this.documentElement.setAttribute("dir", v == null ? "" : String(v));
        }
      },
      configurable: true,
    });
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

  // 12. kitesurf internals & demandFontOutlines
  const ksInternalsSym = Symbol.for("kitesurf.internals");
  if (!globalThis[ksInternalsSym]) {
    globalThis[ksInternalsSym] = {
      demandFontOutlines: function () {},
    };
  }
  if (!global[ksInternalsSym]) {
    global[ksInternalsSym] = globalThis[ksInternalsSym];
  }
  if (typeof window !== "undefined" && !window[ksInternalsSym]) {
    window[ksInternalsSym] = globalThis[ksInternalsSym];
  }

  // 13. NodeName conformance: text nodes must return "#text" (lowercase), comments "#comment"
  if (typeof Element !== "undefined" && Element.prototype) {
    Object.defineProperty(Element.prototype, "nodeName", {
      get: function () {
        if (this.nodeType === 3) return "#text";
        if (this.nodeType === 8) return "#comment";
        if (this.nodeType === 11) return "#document-fragment";
        if (this.nodeType === 9) return "#document";
        return this.tagName || "";
      },
      set: function (v) {},
      configurable: true,
    });
  }
  if (typeof Node !== "undefined" && Node.prototype) {
    Object.defineProperty(Node.prototype, "nodeName", {
      get: function () {
        if (this.nodeType === 3) return "#text";
        if (this.nodeType === 8) return "#comment";
        if (this.nodeType === 11) return "#document-fragment";
        if (this.nodeType === 9) return "#document";
        return this.tagName || "";
      },
      set: function (v) {},
      configurable: true,
    });
  }
  if (typeof Text !== "undefined" && Text.prototype) {
    Object.defineProperty(Text.prototype, "nodeName", {
      get: function () { return "#text"; },
      set: function (v) {},
      configurable: true,
    });
  }
  if (typeof Comment !== "undefined" && Comment.prototype) {
    Object.defineProperty(Comment.prototype, "nodeName", {
      get: function () { return "#comment"; },
      set: function (v) {},
      configurable: true,
    });
  }
  if (typeof DocumentFragment !== "undefined" && DocumentFragment.prototype) {
    Object.defineProperty(DocumentFragment.prototype, "nodeName", {
      get: function () { return "#document-fragment"; },
      set: function (v) {},
      configurable: true,
    });
  }

  // 14. Named Globals on window for elements with IDs inserted into document
  function stampNamedGlobals(node) {
    if (!node || typeof node !== "object") return;
    if (node.nodeType === 1) { // ELEMENT_NODE
      const id = node.id || "";
      if (typeof id === "string" && id && !/^[0-9_$]/.test(id)) {
        if (!(id in global)) {
          const getter = function () { return node; };
          const setter = function (v) {
            Object.defineProperty(global, id, {
              value: v,
              writable: true,
              enumerable: true,
              configurable: true,
            });
          };
          Object.defineProperty(global, id, {
            configurable: true,
            enumerable: true,
            get: getter,
            set: setter,
          });
          if (typeof window !== "undefined" && window !== global && !(id in window)) {
            Object.defineProperty(window, id, {
              configurable: true,
              enumerable: true,
              get: getter,
              set: setter,
            });
          }
          if (typeof globalThis !== "undefined" && globalThis !== global && !(id in globalThis)) {
            Object.defineProperty(globalThis, id, {
              configurable: true,
              enumerable: true,
              get: getter,
              set: setter,
            });
          }
        }
      }
    }
    const kids = node.childNodes || node._children;
    if (kids && kids.length) {
      for (let i = 0; i < kids.length; i++) {
        stampNamedGlobals(kids[i]);
      }
    }
  }

  const origAppendChild = Element.prototype.appendChild || Node.prototype.appendChild;
  function wrappedAppendChild(child) {
    if (child && child.nodeType === 11) { // DOCUMENT_FRAGMENT_NODE
      const kids = child.childNodes || child._children;
      if (kids && kids.length) {
        for (let i = 0; i < kids.length; i++) {
          stampNamedGlobals(kids[i]);
        }
      }
    }
    const res = origAppendChild.call(this, child);
    try {
      if (child && this && (this === global.document || this === global.document?.body || this.isConnected)) {
        stampNamedGlobals(child);
      }
    } catch (e) {}
    return res;
  }
  Node.prototype.appendChild = wrappedAppendChild;
  if (typeof Element !== "undefined" && Element.prototype) {
    Element.prototype.appendChild = wrappedAppendChild;
  }

  const origInsertBefore = Element.prototype.insertBefore || Node.prototype.insertBefore;
  function wrappedInsertBefore(newChild, refChild) {
    if (newChild && newChild.nodeType === 11) {
      const kids = newChild.childNodes || newChild._children;
      if (kids && kids.length) {
        for (let i = 0; i < kids.length; i++) {
          stampNamedGlobals(kids[i]);
        }
      }
    }
    const res = origInsertBefore.call(this, newChild, refChild);
    try {
      if (newChild && this && (this === global.document || this === global.document?.body || this.isConnected)) {
        stampNamedGlobals(newChild);
      }
    } catch (e) {}
    return res;
  }
  Node.prototype.insertBefore = wrappedInsertBefore;
  if (typeof Element !== "undefined" && Element.prototype) {
    Element.prototype.insertBefore = wrappedInsertBefore;
  }

  // 15. Prevent duplicate script load/error event firing
  const origDispatchEvent = EventTarget.prototype.dispatchEvent;
  EventTarget.prototype.dispatchEvent = function (event) {
    if (event && (event.type === "load" || event.type === "error") && this && (this.tagName === "SCRIPT" || this.localName === "script")) {
      const prop = "on" + event.type;
      const savedHandler = this[prop];
      if (typeof savedHandler === "function") {
        this[prop] = null;
        try {
          return origDispatchEvent.call(this, event);
        } finally {
          this[prop] = savedHandler;
        }
      }
    }
    return origDispatchEvent.call(this, event);
  };

  // 16. Mark scripts created by innerHTML as already started
  function markSubtreeScripts(root) {
    if (!root || typeof root !== "object") return;
    let curr = root.firstChild;
    while (curr) {
      const tag = String(curr.localName || curr.tagName || "").toLowerCase();
      if (tag === "script") {
        curr._alreadyStarted = true;
      }
      markSubtreeScripts(curr);
      curr = curr.nextSibling;
    }
  }

  const elInnerDesc = Object.getOwnPropertyDescriptor(Element.prototype, "innerHTML");
  if (elInnerDesc && elInnerDesc.set) {
    const prevSetInner = elInnerDesc.set;
    Object.defineProperty(Element.prototype, "innerHTML", {
      get: elInnerDesc.get,
      set: function (v) {
        prevSetInner.call(this, v);
        try {
          markSubtreeScripts(this);
        } catch (e) {}
      },
      configurable: true,
      enumerable: elInnerDesc.enumerable,
    });
  }
}
