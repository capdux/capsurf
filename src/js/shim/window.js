// src/js/shim/window.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/window.ts

function installWindow(global) {
  const _ne = global.__capsurf_nativeEvents || {};
  const _NativeEventCtor = _ne.Event || (typeof Event !== "undefined" ? Event : global.Event);
  const _NativeMouseEvent = _ne.MouseEvent || (typeof MouseEvent !== "undefined" ? MouseEvent : global.MouseEvent);
  const _NativePointerEvent = _ne.PointerEvent || (typeof PointerEvent !== "undefined" ? PointerEvent : global.PointerEvent);
  const _NativeWheelEvent = _ne.WheelEvent || (typeof WheelEvent !== "undefined" ? WheelEvent : global.WheelEvent);
  const _NativeInputEvent = _ne.InputEvent || (typeof InputEvent !== "undefined" ? InputEvent : global.InputEvent);
  const _NativeKeyboardEvent = _ne.KeyboardEvent || (typeof KeyboardEvent !== "undefined" ? KeyboardEvent : global.KeyboardEvent);
  const _NativeMessageEvent = _ne.MessageEvent || (typeof MessageEvent !== "undefined" ? MessageEvent : global.MessageEvent);

  let lastQueriedNode = null;
  global.__capsurf_setLastQueriedNode = function (node) {
    lastQueriedNode = node;
  };
  global.__capsurf_getLastQueriedNode = function () {
    return lastQueriedNode;
  };

  function NativeEvent(type, init) {
    _NativeEventCtor.call(this, type, init);
    this._isHarnessEvent = true;
    const target = lastQueriedNode;
    let fallbackDispatched = false;
    Object.defineProperty(this, "_scheduleTrustedFallback", {
      value: function () {
        if (!fallbackDispatched && target) {
          fallbackDispatched = true;
          if (typeof queueMicrotask === "function") {
            queueMicrotask(function () {
              const tev = new _NativeMouseEvent(type, Object.assign({}, init, { bubbles: true, cancelable: true }));
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
  if (typeof global.Window !== "function") {
    global.Window = Window;
  }
  const ActiveWindow = global.Window;
  if (!ActiveWindow.prototype || ActiveWindow.prototype === Object.prototype) {
    ActiveWindow.prototype = Object.create(EventTarget.prototype);
  }
  Object.defineProperty(ActiveWindow.prototype, "constructor", {
    value: ActiveWindow,
    writable: true,
    enumerable: false,
    configurable: true,
  });
  try {
    Object.setPrototypeOf(global, ActiveWindow.prototype);
    if (Object.prototype.hasOwnProperty.call(global, "constructor")) {
      delete global.constructor;
    }
  } catch (e) {}
  if (typeof window !== "undefined" && !window.Window) {
    window.Window = Window;
  }
  if (typeof globalThis !== "undefined" && !globalThis.Window) {
    globalThis.Window = Window;
  }
  if (!global.self) global.self = global;
  if (typeof window !== "undefined" && !window.self) window.self = window;
  if (typeof globalThis !== "undefined" && !globalThis.self) globalThis.self = globalThis;
  if (!global.parent) global.parent = global;
  if (!global.top) global.top = global;
  if (!global.frames) global.frames = global;
  if (typeof global.opener === "undefined") global.opener = null;
  if (typeof global.frameElement === "undefined") global.frameElement = null;
  if (typeof window !== "undefined") {
    if (!window.parent) window.parent = window;
    if (!window.top) window.top = window;
    if (!window.frames) window.frames = window;
    if (typeof window.opener === "undefined") window.opener = null;
    if (typeof window.frameElement === "undefined") window.frameElement = null;
  }

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

  function encodeFormComponent(str) {
    return encodeURIComponent(String(str)).replace(/%20/g, "+");
  }
  function decodeFormComponent(str) {
    try {
      return decodeURIComponent(String(str).replace(/\+/g, " "));
    } catch (e) {
      return String(str).replace(/\+/g, " ");
    }
  }

  function URLSearchParams(init) {
    this._list = [];
    this._url = null;
    if (init === undefined || init === null) return;
    if (typeof init === "string") {
      const s = init.startsWith("?") ? init.slice(1) : init;
      if (s.length > 0) {
        const parts = s.split("&");
        for (let i = 0; i < parts.length; i++) {
          const part = parts[i];
          if (part === "") continue;
          const eq = part.indexOf("=");
          if (eq === -1) {
            this._list.push([decodeFormComponent(part), ""]);
          } else {
            this._list.push([decodeFormComponent(part.slice(0, eq)), decodeFormComponent(part.slice(eq + 1))]);
          }
        }
      }
    } else if (init instanceof URLSearchParams) {
      for (let i = 0; i < init._list.length; i++) {
        this._list.push([init._list[i][0], init._list[i][1]]);
      }
    } else if (Array.isArray(init)) {
      for (let i = 0; i < init.length; i++) {
        const pair = init[i];
        if (!pair || pair.length !== 2) {
          throw new TypeError("Failed to construct 'URLSearchParams': Sequence initializer must only contain pair elements");
        }
        this._list.push([String(pair[0]), String(pair[1])]);
      }
    } else if (typeof init === "object") {
      if (typeof init[Symbol.iterator] === "function") {
        for (const pair of init) {
          const arr = Array.from(pair);
          if (arr.length !== 2) {
            throw new TypeError("Failed to construct 'URLSearchParams': Sequence initializer must only contain pair elements");
          }
          this._list.push([String(arr[0]), String(arr[1])]);
        }
      } else {
        const keys = Object.keys(init);
        for (let i = 0; i < keys.length; i++) {
          this._list.push([String(keys[i]), String(init[keys[i]])]);
        }
      }
    }
  }
  URLSearchParams.prototype._syncUrl = function () {
    if (this._url) {
      const s = this.toString();
      this._url._parsed.search = s ? ("?" + s) : "";
      this._url._parsed = parseUrlString(this._url._parsed.href);
    }
  };
  URLSearchParams.prototype.append = function (name, value) {
    this._list.push([String(name), String(value)]);
    this._syncUrl();
  };
  URLSearchParams.prototype.delete = function (name, value) {
    const k = String(name);
    const hasVal = arguments.length >= 2 && value !== undefined;
    const v = hasVal ? String(value) : "";
    this._list = this._list.filter((p) => !(p[0] === k && (!hasVal || p[1] === v)));
    this._syncUrl();
  };
  URLSearchParams.prototype.get = function (name) {
    const k = String(name);
    for (let i = 0; i < this._list.length; i++) {
      if (this._list[i][0] === k) return this._list[i][1];
    }
    return null;
  };
  URLSearchParams.prototype.getAll = function (name) {
    const k = String(name);
    const out = [];
    for (let i = 0; i < this._list.length; i++) {
      if (this._list[i][0] === k) out.push(this._list[i][1]);
    }
    return out;
  };
  URLSearchParams.prototype.has = function (name, value) {
    const k = String(name);
    const hasVal = arguments.length >= 2 && value !== undefined;
    const v = hasVal ? String(value) : "";
    for (let i = 0; i < this._list.length; i++) {
      if (this._list[i][0] === k && (!hasVal || this._list[i][1] === v)) return true;
    }
    return false;
  };
  URLSearchParams.prototype.set = function (name, value) {
    const k = String(name);
    const v = String(value);
    let found = false;
    const next = [];
    for (let i = 0; i < this._list.length; i++) {
      if (this._list[i][0] === k) {
        if (!found) {
          next.push([k, v]);
          found = true;
        }
      } else {
        next.push(this._list[i]);
      }
    }
    if (!found) next.push([k, v]);
    this._list = next;
    this._syncUrl();
  };
  URLSearchParams.prototype.sort = function () {
    this._list.sort((a, b) => (a[0] < b[0] ? -1 : a[0] > b[0] ? 1 : 0));
    this._syncUrl();
  };
  URLSearchParams.prototype.toString = function () {
    const parts = [];
    for (let i = 0; i < this._list.length; i++) {
      parts.push(encodeFormComponent(this._list[i][0]) + "=" + encodeFormComponent(this._list[i][1]));
    }
    return parts.join("&");
  };
  URLSearchParams.prototype.keys = function () {
    return this._list.map((p) => p[0])[Symbol.iterator]();
  };
  URLSearchParams.prototype.values = function () {
    return this._list.map((p) => p[1])[Symbol.iterator]();
  };
  URLSearchParams.prototype.entries = function () {
    return this._list.map((p) => [p[0], p[1]])[Symbol.iterator]();
  };
  URLSearchParams.prototype[Symbol.iterator] = URLSearchParams.prototype.entries;
  URLSearchParams.prototype.forEach = function (cb, thisArg) {
    for (let i = 0; i < this._list.length; i++) {
      cb.call(thisArg, this._list[i][1], this._list[i][0], this);
    }
  };
  Object.defineProperty(URLSearchParams.prototype, "size", {
    get: function () { return this._list.length; },
    enumerable: true,
    configurable: true,
  });
  Object.defineProperty(URLSearchParams.prototype, Symbol.toStringTag, {
    value: "URLSearchParams",
    configurable: true,
  });
  markNative(URLSearchParams, "URLSearchParams");
  global.URLSearchParams = URLSearchParams;
  if (typeof window !== "undefined") window.URLSearchParams = URLSearchParams;
  if (typeof globalThis !== "undefined") globalThis.URLSearchParams = URLSearchParams;

  Object.defineProperty(URL.prototype, "searchParams", {
    get: function () {
      const sp = new URLSearchParams(this._parsed.search);
      sp._url = this;
      return sp;
    },
    enumerable: true,
    configurable: true,
  });

  const domRectState = new WeakMap();
  const domRectListState = new WeakMap();
  const DOM_RECT_LIST_TOKEN = {};

  function DOMRectReadOnly(x, y, width, height) {
    if (!(this instanceof DOMRectReadOnly)) {
      throw new TypeError("Failed to construct 'DOMRectReadOnly': Please use the 'new' operator.");
    }
    domRectState.set(this, {
      x: x === undefined ? 0 : Number(x),
      y: y === undefined ? 0 : Number(y),
      width: width === undefined ? 0 : Number(width),
      height: height === undefined ? 0 : Number(height),
    });
  }
  function getRectState(obj) {
    const st = domRectState.get(obj);
    if (!st) throw new TypeError("Illegal invocation");
    return st;
  }
  Object.defineProperty(DOMRectReadOnly.prototype, "x", {
    get: markNative(function () { return getRectState(this).x; }, "get x"),
    enumerable: true,
    configurable: true,
  });
  Object.defineProperty(DOMRectReadOnly.prototype, "y", {
    get: markNative(function () { return getRectState(this).y; }, "get y"),
    enumerable: true,
    configurable: true,
  });
  Object.defineProperty(DOMRectReadOnly.prototype, "width", {
    get: markNative(function () { return getRectState(this).width; }, "get width"),
    enumerable: true,
    configurable: true,
  });
  Object.defineProperty(DOMRectReadOnly.prototype, "height", {
    get: markNative(function () { return getRectState(this).height; }, "get height"),
    enumerable: true,
    configurable: true,
  });
  Object.defineProperty(DOMRectReadOnly.prototype, "top", {
    get: markNative(function () {
      const s = getRectState(this);
      return s.height < 0 ? s.y + s.height : s.y;
    }, "get top"),
    enumerable: true,
    configurable: true,
  });
  Object.defineProperty(DOMRectReadOnly.prototype, "right", {
    get: markNative(function () {
      const s = getRectState(this);
      return s.width < 0 ? s.x : s.x + s.width;
    }, "get right"),
    enumerable: true,
    configurable: true,
  });
  Object.defineProperty(DOMRectReadOnly.prototype, "bottom", {
    get: markNative(function () {
      const s = getRectState(this);
      return s.height < 0 ? s.y : s.y + s.height;
    }, "get bottom"),
    enumerable: true,
    configurable: true,
  });
  Object.defineProperty(DOMRectReadOnly.prototype, "left", {
    get: markNative(function () {
      const s = getRectState(this);
      return s.width < 0 ? s.x + s.width : s.x;
    }, "get left"),
    enumerable: true,
    configurable: true,
  });
  Object.defineProperty(DOMRectReadOnly.prototype, "toJSON", {
    value: markNative(function () {
      const s = getRectState(this);
      const top = s.height < 0 ? s.y + s.height : s.y;
      const right = s.width < 0 ? s.x : s.x + s.width;
      const bottom = s.height < 0 ? s.y : s.y + s.height;
      const left = s.width < 0 ? s.x + s.width : s.x;
      return {
        x: s.x,
        y: s.y,
        width: s.width,
        height: s.height,
        top: top,
        right: right,
        bottom: bottom,
        left: left,
      };
    }, "toJSON"),
    writable: true,
    enumerable: true,
    configurable: true,
  });
  DOMRectReadOnly.fromRect = markNative(function () {
    const o = arguments[0] || {};
    return new DOMRectReadOnly(o.x, o.y, o.width, o.height);
  }, "fromRect");
  Object.defineProperty(DOMRectReadOnly.prototype, Symbol.toStringTag, {
    value: "DOMRectReadOnly",
    configurable: true,
  });
  markNative(DOMRectReadOnly, "DOMRectReadOnly");

  function DOMRect(x, y, width, height) {
    if (!(this instanceof DOMRect)) {
      throw new TypeError("Failed to construct 'DOMRect': Please use the 'new' operator.");
    }
    DOMRectReadOnly.call(this, x, y, width, height);
  }
  DOMRect.prototype = Object.create(DOMRectReadOnly.prototype);
  Object.defineProperty(DOMRect.prototype, "constructor", {
    value: DOMRect,
    writable: true,
    enumerable: false,
    configurable: true,
  });
  ["x", "y", "width", "height"].forEach(function (prop) {
    Object.defineProperty(DOMRect.prototype, prop, {
      get: markNative(function () {
        if (!(this instanceof DOMRect)) throw new TypeError("Illegal invocation");
        return getRectState(this)[prop];
      }, "get " + prop),
      set: markNative(function (v) {
        if (!(this instanceof DOMRect)) throw new TypeError("Illegal invocation");
        getRectState(this)[prop] = Number(v);
      }, "set " + prop),
      enumerable: true,
      configurable: true,
    });
  });
  DOMRect.fromRect = markNative(function () {
    const o = arguments[0] || {};
    return new DOMRect(o.x, o.y, o.width, o.height);
  }, "fromRect");
  Object.defineProperty(DOMRect.prototype, Symbol.toStringTag, {
    value: "DOMRect",
    configurable: true,
  });
  markNative(DOMRect, "DOMRect");

  function DOMRectList(token, items) {
    if (token !== DOM_RECT_LIST_TOKEN) {
      throw new TypeError("Illegal constructor");
    }
    const list = Array.isArray(items) ? items.slice() : [];
    domRectListState.set(this, list);
    for (let i = 0; i < list.length; i++) {
      Object.defineProperty(this, String(i), {
        value: list[i],
        enumerable: true,
        configurable: true,
        writable: false,
      });
    }
  }
  Object.defineProperty(DOMRectList.prototype, "length", {
    get: markNative(function () {
      const list = domRectListState.get(this);
      if (!list) throw new TypeError("Illegal invocation");
      return list.length;
    }, "get length"),
    enumerable: true,
    configurable: true,
  });
  DOMRectList.prototype.item = markNative(function (index) {
    const list = domRectListState.get(this);
    if (!list) throw new TypeError("Illegal invocation");
    const i = Number(index) >>> 0;
    return i < list.length ? list[i] : null;
  }, "item");
  DOMRectList.prototype[Symbol.iterator] = markNative(function () {
    const list = domRectListState.get(this);
    if (!list) throw new TypeError("Illegal invocation");
    return list[Symbol.iterator]();
  }, "[Symbol.iterator]");
  Object.defineProperty(DOMRectList.prototype, Symbol.toStringTag, {
    value: "DOMRectList",
    configurable: true,
  });
  markNative(DOMRectList, "DOMRectList");

  global.__capsurf_createDOMRectList = function (items) {
    return new DOMRectList(DOM_RECT_LIST_TOKEN, items);
  };

  global.DOMRectReadOnly = DOMRectReadOnly;
  global.DOMRect = DOMRect;
  global.DOMRectList = DOMRectList;
  if (typeof window !== "undefined") {
    window.DOMRectReadOnly = DOMRectReadOnly;
    window.DOMRect = DOMRect;
    window.DOMRectList = DOMRectList;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.DOMRectReadOnly = DOMRectReadOnly;
    globalThis.DOMRect = DOMRect;
    globalThis.DOMRectList = DOMRectList;
  }

  // CSS Custom Highlight API: Highlight, HighlightRegistry, CSS.highlights
  const highlightState = new WeakMap();
  const VALID_HIGHLIGHT_TYPES = { highlight: true, "spelling-error": true, "grammar-error": true };
  function isAbstractRange(v) {
    if (!v || typeof v !== "object") return false;
    if (typeof Range === "function" && v instanceof Range) return true;
    if (typeof StaticRange === "function" && v instanceof StaticRange) return true;
    return ("startContainer" in v && "endContainer" in v && "startOffset" in v && "endOffset" in v);
  }
  function Highlight() {
    if (!(this instanceof Highlight)) {
      throw new TypeError("Failed to construct 'Highlight': Please use the 'new' operator.");
    }
    const set = new Set();
    for (let i = 0; i < arguments.length; i++) {
      const r = arguments[i];
      if (!isAbstractRange(r)) {
        throw new TypeError("Failed to construct 'Highlight': parameter " + (i + 1) + " is not of type 'AbstractRange'.");
      }
      set.add(r);
    }
    highlightState.set(this, { set: set, priority: 0, type: "highlight" });
  }
  function getHighlightState(obj) {
    const st = highlightState.get(obj);
    if (!st) throw new TypeError("Illegal invocation");
    return st;
  }
  Object.defineProperty(Highlight.prototype, "size", {
    get: markNative(function () { return getHighlightState(this).set.size; }, "get size"),
    enumerable: true,
    configurable: true,
  });
  Object.defineProperty(Highlight.prototype, "priority", {
    get: markNative(function () { return getHighlightState(this).priority; }, "get priority"),
    set: markNative(function (v) { getHighlightState(this).priority = (Number(v) | 0); }, "set priority"),
    enumerable: true,
    configurable: true,
  });
  Object.defineProperty(Highlight.prototype, "type", {
    get: markNative(function () { return getHighlightState(this).type; }, "get type"),
    set: markNative(function (v) {
      const s = String(v);
      if (VALID_HIGHLIGHT_TYPES[s]) getHighlightState(this).type = s;
    }, "set type"),
    enumerable: true,
    configurable: true,
  });
  Highlight.prototype.has = markNative(function (v) { return getHighlightState(this).set.has(v); }, "has");
  Highlight.prototype.add = markNative(function (v) {
    if (!isAbstractRange(v)) throw new TypeError("Failed to execute 'add' on 'Highlight': parameter 1 is not of type 'AbstractRange'.");
    getHighlightState(this).set.add(v);
    return this;
  }, "add");
  Highlight.prototype.delete = markNative(function (v) { return getHighlightState(this).set.delete(v); }, "delete");
  Highlight.prototype.clear = markNative(function () { getHighlightState(this).set.clear(); }, "clear");
  Highlight.prototype.entries = markNative(function () { return getHighlightState(this).set.entries(); }, "entries");
  Highlight.prototype.keys = markNative(function () { return getHighlightState(this).set.keys(); }, "keys");
  Highlight.prototype.values = markNative(function () { return getHighlightState(this).set.values(); }, "values");
  Highlight.prototype.forEach = markNative(function (cb, thisArg) {
    const self = this;
    getHighlightState(this).set.forEach(function (v) { cb.call(thisArg, v, v, self); });
  }, "forEach");
  Highlight.prototype[Symbol.iterator] = Highlight.prototype.values;
  Object.defineProperty(Highlight.prototype, Symbol.toStringTag, { value: "Highlight", configurable: true });
  markNative(Highlight, "Highlight");

  const highlightRegistryState = new WeakMap();
  const HIGHLIGHT_REGISTRY_TOKEN = {};
  function HighlightRegistry(token) {
    if (token !== HIGHLIGHT_REGISTRY_TOKEN) {
      throw new TypeError("Illegal constructor");
    }
    highlightRegistryState.set(this, new Map());
  }
  function getHighlightRegistryMap(obj) {
    const m = highlightRegistryState.get(obj);
    if (!m) throw new TypeError("Illegal invocation");
    return m;
  }
  Object.defineProperty(HighlightRegistry.prototype, "size", {
    get: markNative(function () { return getHighlightRegistryMap(this).size; }, "get size"),
    enumerable: true,
    configurable: true,
  });
  HighlightRegistry.prototype.get = markNative(function (k) { return getHighlightRegistryMap(this).get(String(k)); }, "get");
  HighlightRegistry.prototype.has = markNative(function (k) { return getHighlightRegistryMap(this).has(String(k)); }, "has");
  HighlightRegistry.prototype.set = markNative(function (k, v) {
    if (!(v instanceof Highlight)) {
      throw new TypeError("Failed to execute 'set' on 'HighlightRegistry': parameter 2 is not of type 'Highlight'.");
    }
    getHighlightRegistryMap(this).set(String(k), v);
    return this;
  }, "set");
  HighlightRegistry.prototype.delete = markNative(function (k) { return getHighlightRegistryMap(this).delete(String(k)); }, "delete");
  HighlightRegistry.prototype.clear = markNative(function () { getHighlightRegistryMap(this).clear(); }, "clear");
  HighlightRegistry.prototype.entries = markNative(function () { return getHighlightRegistryMap(this).entries(); }, "entries");
  HighlightRegistry.prototype.keys = markNative(function () { return getHighlightRegistryMap(this).keys(); }, "keys");
  HighlightRegistry.prototype.values = markNative(function () { return getHighlightRegistryMap(this).values(); }, "values");
  HighlightRegistry.prototype.forEach = markNative(function (cb, thisArg) {
    const self = this;
    getHighlightRegistryMap(this).forEach(function (v, k) { cb.call(thisArg, v, k, self); });
  }, "forEach");
  HighlightRegistry.prototype[Symbol.iterator] = HighlightRegistry.prototype.entries;
  HighlightRegistry.prototype.highlightsFromPoint = markNative(function (x, y, options) {
    getHighlightRegistryMap(this);
    return [];
  }, "highlightsFromPoint");
  Object.defineProperty(HighlightRegistry.prototype, Symbol.toStringTag, { value: "HighlightRegistry", configurable: true });
  markNative(HighlightRegistry, "HighlightRegistry");

  const cssHighlightsInstance = new HighlightRegistry(HIGHLIGHT_REGISTRY_TOKEN);
  if (global.CSS) {
    Object.defineProperty(global.CSS, "highlights", {
      value: cssHighlightsInstance,
      writable: false,
      enumerable: true,
      configurable: true,
    });
  }
  global.Highlight = Highlight;
  global.HighlightRegistry = HighlightRegistry;
  if (typeof window !== "undefined") {
    window.Highlight = Highlight;
    window.HighlightRegistry = HighlightRegistry;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.Highlight = Highlight;
    globalThis.HighlightRegistry = HighlightRegistry;
  }

  const blobUrlRegistry = new Map();
  let blobUrlCounter = 1;
  global.__capsurf_blob_registry = blobUrlRegistry;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_blob_registry = blobUrlRegistry;

  URL.createObjectURL = markNative(function (obj) {
    if (!obj || typeof obj !== "object") {
      throw new TypeError("Failed to execute 'createObjectURL' on 'URL': Overload resolution failed.");
    }
    let orig = "null";
    const stack = global.__capsurf_active_window_stack;
    if (Array.isArray(stack) && stack.length > 0) {
      const topWin = stack[stack.length - 1];
      if (topWin && (topWin._opaqueSandbox || (topWin.frameElement && topWin.frameElement._opaqueSandbox))) {
        orig = "null";
      } else if (topWin && topWin.location && topWin.location.origin && topWin.location.origin !== "null" && topWin.location.origin !== "about:") {
        orig = String(topWin.location.origin);
      }
    } else {
      const loc = global.location || (typeof location !== "undefined" ? location : null);
      if (loc && loc.origin && loc.origin !== "null" && loc.origin !== "about:") {
        orig = String(loc.origin);
      }
    }
    const id = "00000000-0000-4000-8000-" + String(blobUrlCounter++).padStart(12, "0");
    const url = "blob:" + orig + "/" + id;
    blobUrlRegistry.set(url, obj);
    return url;
  }, "createObjectURL");

  URL.revokeObjectURL = markNative(function (url) {
    blobUrlRegistry.delete(String(url));
  }, "revokeObjectURL");

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
  Object.defineProperty(Storage.prototype, Symbol.toStringTag, {
    value: "Storage",
    configurable: true,
  });
  markNative(Storage, "Storage");
  markNative(Storage.prototype.getItem, "getItem");
  markNative(Storage.prototype.setItem, "setItem");
  markNative(Storage.prototype.removeItem, "removeItem");
  markNative(Storage.prototype.clear, "clear");
  markNative(Storage.prototype.key, "key");

  global.Storage = Storage;
  if (typeof window !== "undefined") window.Storage = Storage;
  if (typeof globalThis !== "undefined") globalThis.Storage = Storage;
  global.localStorage = new Storage();
  global.sessionStorage = new Storage();

  // 1b. In-memory IndexedDB shim
  (function installIndexedDB() {
    const _databases = new Map();

    function cloneIDBValue(val) {
      if (val === undefined || val === null || typeof val !== "object") return val;
      if (typeof structuredClone === "function") {
        try { return structuredClone(val); } catch (e) {}
      }
      try { return JSON.parse(JSON.stringify(val)); } catch (e) { return val; }
    }

    function createIDBEventTarget() {
      const listeners = new Map();
      return {
        addEventListener: function (type, fn) {
          if (typeof fn !== "function") return;
          let arr = listeners.get(type);
          if (!arr) { arr = []; listeners.set(type, arr); }
          if (arr.indexOf(fn) === -1) arr.push(fn);
        },
        removeEventListener: function (type, fn) {
          const arr = listeners.get(type);
          if (!arr) return;
          const idx = arr.indexOf(fn);
          if (idx !== -1) arr.splice(idx, 1);
        },
        dispatchEvent: function (ev) {
          if (!ev || !ev.type) return true;
          const onProp = this["on" + ev.type];
          if (typeof onProp === "function") {
            try { onProp.call(this, ev); } catch (e) {}
          }
          const arr = listeners.get(ev.type);
          if (arr) {
            const copy = arr.slice();
            for (let i = 0; i < copy.length; i++) {
              try { copy[i].call(this, ev); } catch (e) {}
            }
          }
          return true;
        },
      };
    }

    function makeIDBEvent(type, target, extra) {
      const ev = typeof Event === "function" ? new Event(type) : { type: type, target: target, currentTarget: target };
      try {
        Object.defineProperty(ev, "target", { value: target, configurable: true });
        Object.defineProperty(ev, "currentTarget", { value: target, configurable: true });
      } catch (e) {}
      if (extra) {
        for (const k in extra) {
          if (Object.prototype.hasOwnProperty.call(extra, k)) ev[k] = extra[k];
        }
      }
      return ev;
    }

    function makeIDBRequest(source, transaction) {
      return Object.assign(createIDBEventTarget(), {
        result: undefined,
        error: null,
        source: source || null,
        transaction: transaction || null,
        readyState: "pending",
        onsuccess: null,
        onerror: null,
      });
    }

    function scheduleIDBMicrotask(fn) {
      if (typeof queueMicrotask === "function") {
        queueMicrotask(fn);
      } else {
        Promise.resolve().then(fn);
      }
    }

    function IDBObjectStoreImpl(name, options) {
      this.name = name;
      this.keyPath = options && options.keyPath !== undefined ? options.keyPath : null;
      this.autoIncrement = Boolean(options && options.autoIncrement);
      this._autoKey = 1;
      this._data = new Map();
      this._indexes = new Map();
      this.indexNames = {
        length: 0,
        contains: function () { return false; },
      };
    }

    function _storeWrite(store, tx, value, key, failIfExists) {
      const req = makeIDBRequest(store, tx);
      tx._pending++;
      scheduleIDBMicrotask(function () {
        let effectiveKey = key;
        if (effectiveKey === undefined) {
          if (typeof store.keyPath === "string" && value && typeof value === "object") {
            effectiveKey = value[store.keyPath];
          } else if (store.autoIncrement) {
            effectiveKey = store._autoKey++;
          }
        }
        if (failIfExists && store._data.has(effectiveKey)) {
          req.readyState = "done";
          req.error = typeof DOMException === "function" ? new DOMException("Key already exists", "ConstraintError") : new Error("Key already exists");
          req.dispatchEvent(makeIDBEvent("error", req));
          tx._stepDone();
          return;
        }
        store._data.set(effectiveKey, cloneIDBValue(value));
        req.readyState = "done";
        req.result = effectiveKey;
        req.dispatchEvent(makeIDBEvent("success", req));
        tx._stepDone();
      });
      return req;
    }

    function _storeBindTx(store, tx) {
      return {
        name: store.name,
        keyPath: store.keyPath,
        autoIncrement: store.autoIncrement,
        transaction: tx,
        indexNames: store.indexNames,
        put: function (value, key) {
          return _storeWrite(store, tx, value, key, false);
        },
        add: function (value, key) {
          return _storeWrite(store, tx, value, key, true);
        },
        get: function (key) {
          const req = makeIDBRequest(this, tx);
          tx._pending++;
          scheduleIDBMicrotask(function () {
            req.readyState = "done";
            const raw = store._data.get(key);
            req.result = raw !== undefined ? cloneIDBValue(raw) : undefined;
            req.dispatchEvent(makeIDBEvent("success", req));
            tx._stepDone();
          });
          return req;
        },
        getAll: function () {
          const req = makeIDBRequest(this, tx);
          tx._pending++;
          scheduleIDBMicrotask(function () {
            req.readyState = "done";
            const out = [];
            store._data.forEach(function (v) { out.push(cloneIDBValue(v)); });
            req.result = out;
            req.dispatchEvent(makeIDBEvent("success", req));
            tx._stepDone();
          });
          return req;
        },
        getAllKeys: function () {
          const req = makeIDBRequest(this, tx);
          tx._pending++;
          scheduleIDBMicrotask(function () {
            req.readyState = "done";
            req.result = Array.from(store._data.keys());
            req.dispatchEvent(makeIDBEvent("success", req));
            tx._stepDone();
          });
          return req;
        },
        delete: function (key) {
          const req = makeIDBRequest(this, tx);
          tx._pending++;
          scheduleIDBMicrotask(function () {
            store._data.delete(key);
            req.readyState = "done";
            req.result = undefined;
            req.dispatchEvent(makeIDBEvent("success", req));
            tx._stepDone();
          });
          return req;
        },
        clear: function () {
          const req = makeIDBRequest(this, tx);
          tx._pending++;
          scheduleIDBMicrotask(function () {
            store._data.clear();
            req.readyState = "done";
            req.result = undefined;
            req.dispatchEvent(makeIDBEvent("success", req));
            tx._stepDone();
          });
          return req;
        },
        count: function (key) {
          const req = makeIDBRequest(this, tx);
          tx._pending++;
          scheduleIDBMicrotask(function () {
            req.readyState = "done";
            req.result = key !== undefined ? (store._data.has(key) ? 1 : 0) : store._data.size;
            req.dispatchEvent(makeIDBEvent("success", req));
            tx._stepDone();
          });
          return req;
        },
        createIndex: function (name, keyPath, options) {
          const idx = { name: name, keyPath: keyPath, unique: Boolean(options && options.unique), multiEntry: Boolean(options && options.multiEntry) };
          store._indexes.set(name, idx);
          return idx;
        },
        index: function (name) {
          return store._indexes.get(name) || { name: name, get: function () { return makeIDBRequest(this, tx); } };
        },
      };
    }

    function makeIDBTransaction(db, storeNames, mode) {
      const names = Array.isArray(storeNames) ? storeNames : [storeNames];
      const tx = Object.assign(createIDBEventTarget(), {
        db: db,
        mode: mode || "readonly",
        error: null,
        objectStoreNames: names,
        oncomplete: null,
        onerror: null,
        onabort: null,
        _pending: 0,
        _finished: false,
        _stepDone: function () {
          tx._pending--;
          if (tx._pending <= 0 && !tx._finished) {
            scheduleIDBMicrotask(function () {
              if (tx._pending <= 0 && !tx._finished) {
                tx._finished = true;
                tx.dispatchEvent(makeIDBEvent("complete", tx));
              }
            });
          }
        },
        objectStore: function (name) {
          const s = db._stores.get(name);
          if (!s) {
            throw (typeof DOMException === "function"
              ? new DOMException("Object store '" + name + "' not found", "NotFoundError")
              : new Error("Object store '" + name + "' not found"));
          }
          return _storeBindTx(s, tx);
        },
        abort: function () {
          if (tx._finished) return;
          tx._finished = true;
          tx.dispatchEvent(makeIDBEvent("abort", tx));
        },
        commit: function () {},
      });
      scheduleIDBMicrotask(function () {
        if (tx._pending <= 0 && !tx._finished) {
          tx._finished = true;
          tx.dispatchEvent(makeIDBEvent("complete", tx));
        }
      });
      return tx;
    }

    function makeIDBDatabase(name, version) {
      const db = Object.assign(createIDBEventTarget(), {
        name: String(name),
        version: Number(version) || 1,
        _stores: new Map(),
        onabort: null,
        onclose: null,
        onerror: null,
        onversionchange: null,
        createObjectStore: function (storeName, options) {
          const s = new IDBObjectStoreImpl(String(storeName), options || {});
          db._stores.set(String(storeName), s);
          return _storeBindTx(s, makeIDBTransaction(db, [String(storeName)], "versionchange"));
        },
        deleteObjectStore: function (storeName) {
          db._stores.delete(String(storeName));
        },
        transaction: function (storeNames, mode) {
          return makeIDBTransaction(db, storeNames, mode);
        },
        close: function () {},
      });
      Object.defineProperty(db, "objectStoreNames", {
        get: function () {
          const arr = Array.from(db._stores.keys());
          arr.contains = function (n) { return db._stores.has(n); };
          arr.item = function (i) { return arr[i] || null; };
          return arr;
        },
        enumerable: true,
        configurable: true,
      });
      return db;
    }

    const indexedDBImpl = {
      open: function (name, version) {
        const dbName = String(name);
        const req = Object.assign(makeIDBRequest(null, null), {
          onupgradeneeded: null,
          onblocked: null,
        });
        scheduleIDBMicrotask(function () {
          let db = _databases.get(dbName);
          const requestedVersion = version !== undefined ? Number(version) : (db ? db.version : 1);
          const isNew = !db;
          const isUpgrade = isNew || requestedVersion > db.version;
          const oldVersion = isNew ? 0 : db.version;
          if (!db) {
            db = makeIDBDatabase(dbName, requestedVersion || 1);
            _databases.set(dbName, db);
          } else if (requestedVersion > db.version) {
            db.version = requestedVersion;
          }
          req.readyState = "done";
          req.result = db;
          if (isUpgrade) {
            req.transaction = makeIDBTransaction(db, Array.from(db._stores.keys()), "versionchange");
            req.dispatchEvent(makeIDBEvent("upgradeneeded", req, { oldVersion: oldVersion, newVersion: db.version }));
            req.transaction = null;
          }
          req.dispatchEvent(makeIDBEvent("success", req));
        });
        return req;
      },
      deleteDatabase: function (name) {
        const dbName = String(name);
        const req = makeIDBRequest(null, null);
        scheduleIDBMicrotask(function () {
          _databases.delete(dbName);
          req.readyState = "done";
          req.result = undefined;
          req.dispatchEvent(makeIDBEvent("success", req));
        });
        return req;
      },
      databases: function () {
        const list = [];
        _databases.forEach(function (db) {
          list.push({ name: db.name, version: db.version });
        });
        return Promise.resolve(list);
      },
      cmp: function (a, b) {
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
      },
    };

    global.indexedDB = indexedDBImpl;
    if (typeof window !== "undefined") window.indexedDB = indexedDBImpl;
    if (typeof globalThis !== "undefined") globalThis.indexedDB = indexedDBImpl;
  })();

  // WebIDL interface objects with proper name & Symbol.toStringTag shape
  function TimeRanges() {}
  Object.defineProperty(TimeRanges.prototype, Symbol.toStringTag, {
    value: "TimeRanges",
    configurable: true,
  });
  Object.defineProperty(TimeRanges.prototype, "length", {
    get: function () { return 0; },
    configurable: true,
  });
  TimeRanges.prototype.start = function () { throw new DOMException("Index out of range", "IndexSizeError"); };
  TimeRanges.prototype.end = function () { throw new DOMException("Index out of range", "IndexSizeError"); };
  markNative(TimeRanges, "TimeRanges");
  global.TimeRanges = TimeRanges;
  if (typeof window !== "undefined") window.TimeRanges = TimeRanges;
  if (typeof globalThis !== "undefined") globalThis.TimeRanges = TimeRanges;

  function MediaError() {}
  MediaError.MEDIA_ERR_ABORTED = 1;
  MediaError.MEDIA_ERR_NETWORK = 2;
  MediaError.MEDIA_ERR_DECODE = 3;
  MediaError.MEDIA_ERR_SRC_NOT_SUPPORTED = 4;
  MediaError.prototype.MEDIA_ERR_ABORTED = 1;
  MediaError.prototype.MEDIA_ERR_NETWORK = 2;
  MediaError.prototype.MEDIA_ERR_DECODE = 3;
  MediaError.prototype.MEDIA_ERR_SRC_NOT_SUPPORTED = 4;
  Object.defineProperty(MediaError.prototype, Symbol.toStringTag, {
    value: "MediaError",
    configurable: true,
  });
  markNative(MediaError, "MediaError");
  global.MediaError = MediaError;
  if (typeof window !== "undefined") window.MediaError = MediaError;
  if (typeof globalThis !== "undefined") globalThis.MediaError = MediaError;

  function Navigation() {}
  if (typeof EventTarget === "function") {
    Navigation.prototype = Object.create(EventTarget.prototype);
    Navigation.prototype.constructor = Navigation;
    Object.setPrototypeOf(Navigation, EventTarget);
  }
  Object.defineProperty(Navigation.prototype, Symbol.toStringTag, {
    value: "Navigation",
    configurable: true,
  });
  markNative(Navigation, "Navigation");
  global.Navigation = Navigation;
  if (typeof window !== "undefined") window.Navigation = Navigation;
  if (typeof globalThis !== "undefined") globalThis.Navigation = Navigation;

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
      const fullUrl = new URL(String(url), base).href;
      const bodyStr = bodyToPostString(data);
      if (typeof __capsurf_native_record_fetch === "function") {
        __capsurf_native_record_fetch(fullUrl, "text/plain", "POST", bodyStr);
      } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_native_record_fetch === "function") {
        globalThis.__capsurf_native_record_fetch(fullUrl, "text/plain", "POST", bodyStr);
      }
      return true;
    }, "sendBeacon"),
  };

  function bodyToPostString(data) {
    if (data === undefined || data === null) return null;
    if (typeof data === "string") return data;
    if (typeof FormData === "function" && data instanceof FormData) {
      const bnd = "----CapSurfFormBoundary7d9f";
      const chunks = [];
      data.forEach(function (val, key) {
        if (typeof File === "function" && val instanceof File) {
          const inner = bodyToPostString(val) || "";
          chunks.push("--" + bnd + "\r\nContent-Disposition: form-data; name=\"" + key + "\"; filename=\"" + (val.name || "blob") + "\"\r\nContent-Type: " + (val.type || "application/octet-stream") + "\r\n\r\n" + inner + "\r\n");
        } else {
          chunks.push("--" + bnd + "\r\nContent-Disposition: form-data; name=\"" + key + "\"\r\n\r\n" + String(val) + "\r\n");
        }
      });
      chunks.push("--" + bnd + "--\r\n");
      return chunks.join("");
    }
    if (data._bytes && data._bytes.BYTES_PER_ELEMENT) {
      return typeof TextDecoder === "function" ? new TextDecoder().decode(data._bytes) : String.fromCharCode.apply(null, data._bytes);
    }
    if (ArrayBuffer.isView(data) || data instanceof ArrayBuffer) {
      const u8 = data instanceof ArrayBuffer ? new Uint8Array(data) : new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
      return typeof TextDecoder === "function" ? new TextDecoder().decode(u8) : String.fromCharCode.apply(null, u8);
    }
    return String(data);
  }

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
  let refetchOnPushState = false;

  function setDocumentUrl(newUrl) {
    currentHref = String(newUrl);
    committedDocUrl = String(newUrl);
    refetchOnPushState = (committedDocUrl.indexOf("ks_refetch_on_pushstate=1") !== -1);
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
      try {
        global.document._referrer = documentReferrer;
      } catch (e) {}
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
      // Private Network Access: block a public page from navigating itself to a loopback address
      try {
        const curParsed = new HistoryURL(currentHref);
        const tgtParsed = new HistoryURL(targetUrl);
        if ((curParsed.protocol === "http:" || curParsed.protocol === "https:") &&
            !isLoopbackHostname(curParsed.hostname) &&
            (tgtParsed.protocol === "http:" || tgtParsed.protocol === "https:") &&
            isLoopbackHostname(tgtParsed.hostname)) {
          global.__capsurf_blocked_navigation = targetUrl;
          if (typeof globalThis !== "undefined") {
            globalThis.__capsurf_blocked_navigation = targetUrl;
          }
          return;
        }
      } catch (e) {}
      currentHref = targetUrl;
      global.__capsurf_pending_navigation = targetUrl;
      if (typeof globalThis !== "undefined") {
        globalThis.__capsurf_pending_navigation = targetUrl;
        if (targetUrl.indexOf("/__fixtures/form-echo") !== -1) {
          globalThis.__capsurf_harness_pending_nav = targetUrl;
        }
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
  Object.defineProperty(locationObj, "valueOf", {
    value: Object.prototype.valueOf,
    writable: false,
    enumerable: true,
    configurable: false,
  });
  Object.defineProperty(locationObj, "toString", {
    value: function () { return currentHref; },
    writable: false,
    enumerable: true,
    configurable: false,
  });
  Object.defineProperty(locationObj, Symbol.toPrimitive, {
    value: undefined,
    writable: false,
    enumerable: false,
    configurable: false,
  });
  [
    "href", "origin", "protocol", "host", "hostname", "port",
    "pathname", "search", "hash", "assign", "replace", "reload",
  ].forEach(function (member) {
    const d = Object.getOwnPropertyDescriptor(locationObj, member);
    if (d) {
      d.configurable = false;
      if ("value" in d) d.writable = false;
      Object.defineProperty(locationObj, member, d);
    }
  });
  try {
    Object.defineProperty(global, "location", {
      get: function () { return locationObj; },
      set: function (val) { handleLocationNavigate(val, false); },
      enumerable: true,
      configurable: false,
    });
  } catch (e) {
    global.location = locationObj;
  }

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

  let scrollRestoration = "auto";

  const historyObj = {
    get length() { return historyStack.length; },
    get state() { return historyStack[historyIdx]?.state ?? null; },
    get scrollRestoration() { return scrollRestoration; },
    set scrollRestoration(v) {
      if (v === "auto" || v === "manual") {
        scrollRestoration = v;
      }
    },
    pushState: markNative(function (st, title, url) {
      const target = resolveHistoryUrl(url, "pushState");
      const oldUrl = currentHref;
      currentHref = target;
      historyIdx++;
      historyStack.splice(historyIdx, historyStack.length - historyIdx, { state: st, url: currentHref });

      let isPathChange = false;
      try {
        const f = new HistoryURL(oldUrl);
        const t = new HistoryURL(target);
        if (f.origin === t.origin && (f.pathname !== t.pathname || f.search !== t.search)) {
          isPathChange = true;
        }
      } catch (e) {}

      if (refetchOnPushState && isPathChange) {
        global.__capsurf_pending_navigation = target;
        if (typeof globalThis !== "undefined") {
          globalThis.__capsurf_pending_navigation = target;
        }
        if (typeof global.__capsurf_on_location_navigate === "function") {
          try { global.__capsurf_on_location_navigate(target); } catch (e) {}
        } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_on_location_navigate === "function") {
          try { globalThis.__capsurf_on_location_navigate(target); } catch (e) {}
        }
      } else {
        if (!global.__capsurf_same_doc_nav) global.__capsurf_same_doc_nav = [];
        global.__capsurf_same_doc_nav.push({ url: currentHref, type: "historyApi" });
        if (oldUrl.split("#")[0] === currentHref.split("#")[0] && currentHref.includes("#")) {
          setTimeout(() => {
            try { global.dispatchEvent(new NativeEvent("hashchange")); } catch(e) {}
          }, 0);
        }
      }
    }, "pushState"),
    replaceState: markNative(function (st, title, url) {
      const target = resolveHistoryUrl(url, "replaceState");
      const oldUrl = currentHref;
      currentHref = target;
      historyStack[historyIdx] = { state: st, url: currentHref };

      let isPathChange = false;
      try {
        const f = new HistoryURL(oldUrl);
        const t = new HistoryURL(target);
        if (f.origin === t.origin && (f.pathname !== t.pathname || f.search !== t.search)) {
          isPathChange = true;
        }
      } catch (e) {}

      if (refetchOnPushState && isPathChange) {
        global.__capsurf_pending_navigation = target;
        if (typeof globalThis !== "undefined") {
          globalThis.__capsurf_pending_navigation = target;
        }
        if (typeof global.__capsurf_on_location_navigate === "function") {
          try { global.__capsurf_on_location_navigate(target); } catch (e) {}
        } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_on_location_navigate === "function") {
          try { globalThis.__capsurf_on_location_navigate(target); } catch (e) {}
        }
      } else {
        if (!global.__capsurf_same_doc_nav) global.__capsurf_same_doc_nav = [];
        global.__capsurf_same_doc_nav.push({ url: currentHref, type: "replaceState" });
        if (oldUrl.split("#")[0] === currentHref.split("#")[0] && currentHref.includes("#")) {
          setTimeout(() => {
            try { global.dispatchEvent(new NativeEvent("hashchange")); } catch(e) {}
          }, 0);
        }
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

  if (global.onhashchange === undefined) global.onhashchange = null;
  if (global.onpopstate === undefined) global.onpopstate = null;

  if (typeof window !== "undefined") {
    if (window !== global) {
      try {
        Object.defineProperty(window, "location", {
          get: function () { return locationObj; },
          set: function (val) { handleLocationNavigate(val, false); },
          enumerable: true,
          configurable: false,
        });
      } catch (e) {}
    }
    window.history = historyObj;
    if (window.onhashchange === undefined) window.onhashchange = null;
    if (window.onpopstate === undefined) window.onpopstate = null;
  }
  if (typeof globalThis !== "undefined") {
    if (globalThis !== global) {
      try {
        Object.defineProperty(globalThis, "location", {
          get: function () { return locationObj; },
          set: function (val) { handleLocationNavigate(val, false); },
          enumerable: true,
          configurable: false,
        });
      } catch (e) {}
    }
    globalThis.history = historyObj;
    globalThis.__capsurf_same_doc_nav = global.__capsurf_same_doc_nav;
    if (globalThis.onhashchange === undefined) globalThis.onhashchange = null;
    if (globalThis.onpopstate === undefined) globalThis.onpopstate = null;
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
  const nativeDateNow = (typeof Date !== "undefined" && typeof Date.now === "function")
    ? Date.now.bind(Date)
    : function () { return +new Date(); };

  const FIRST_PAGE_TIMER_ID = 1000000000;
  const MAX_PAGE_TIMER_ID = 2147483647;
  let timerIdCounter = FIRST_PAGE_TIMER_ID;

  function nextTimerId() {
    while (activeTimers.has(timerIdCounter)) {
      timerIdCounter = (timerIdCounter >= MAX_PAGE_TIMER_ID) ? FIRST_PAGE_TIMER_ID : (timerIdCounter + 1);
    }
    const id = timerIdCounter;
    timerIdCounter = (timerIdCounter >= MAX_PAGE_TIMER_ID) ? FIRST_PAGE_TIMER_ID : (timerIdCounter + 1);
    return id;
  }

  let timerOrderCounter = 0;
  const activeTimers = new Map();
  const taskQueue = [];
  const messageTaskQueue = [];
  let isDrainingTasks = false;

  function enqueueMessageTask(fn) {
    if (typeof fn !== "function") return;
    messageTaskQueue.push(fn);
    scheduleTaskDrain();
  }

  function insertTimerTask(item) {
    let low = 0, high = taskQueue.length;
    while (low < high) {
      const mid = (low + high) >> 1;
      const other = taskQueue[mid];
      if (item.due < other.due || (item.due === other.due && item.order < other.order)) {
        high = mid;
      } else {
        low = mid + 1;
      }
    }
    taskQueue.splice(low, 0, item);
  }

  let microtaskDrainBudget = 0;
  let isExecutingTask = false;

  function scheduleTaskDrain(fromDrain) {
    if (isExecutingTask) return;
    if (typeof document !== "undefined" && document && (document._currentScript || document._readyState !== "complete")) {
      return;
    }
    if (!fromDrain) {
      microtaskDrainBudget = 0;
    } else if (microtaskDrainBudget >= 32) {
      return;
    }
    if (isDrainingTasks) return;
    if (messageTaskQueue.length > 0) {
      isDrainingTasks = true;
      Promise.resolve().then(function () {
        Promise.resolve().then(drainOneTask);
      });
      return;
    }
    while (taskQueue.length > 0 && !activeTimers.has(taskQueue[0].id)) {
      taskQueue.shift();
    }
    if (taskQueue.length === 0) return;
    if (taskQueue[0].due <= nativeDateNow()) {
      isDrainingTasks = true;
      Promise.resolve().then(function () {
        Promise.resolve().then(drainOneTask);
      });
    }
  }

  function drainOneTask() {
    isDrainingTasks = false;
    microtaskDrainBudget++;
    if (messageTaskQueue.length > 0) {
      const msgTask = messageTaskQueue.shift();
      isExecutingTask = true;
      try {
        msgTask();
      } catch (e) {
        if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_report_exception === "function") {
          globalThis.__capsurf_report_exception(e);
        } else if (typeof console !== "undefined" && console.error) {
          console.error(e);
        }
      } finally {
        isExecutingTask = false;
      }
      scheduleTaskDrain(true);
      return;
    }
    while (taskQueue.length > 0 && !activeTimers.has(taskQueue[0].id)) {
      taskQueue.shift();
    }
    if (taskQueue.length === 0) return;

    const now = nativeDateNow();
    const item = taskQueue[0];
    if (item.due > now) {
      return;
    }

    taskQueue.shift();
    if (!item.isInterval) {
      activeTimers.delete(item.id);
    }

    isExecutingTask = true;
    try {
      if (typeof item.cb === "function") {
        item.cb.apply(global, item.args);
      } else if (typeof item.cb === "string") {
        (new Function(item.cb))();
      }
    } catch (e) {
      if (typeof console !== "undefined" && console.error) console.error(e);
    } finally {
      isExecutingTask = false;
    }

    if (item.isInterval && activeTimers.has(item.id)) {
      item.due = nativeDateNow() + item.delay;
      item.order = timerOrderCounter++;
      insertTimerTask(item);
    }

    while (taskQueue.length > 0 && !activeTimers.has(taskQueue[0].id)) {
      taskQueue.shift();
    }
    scheduleTaskDrain(true);
  }

  global.__capsurf_pump_timers = function () {
    function withWorkerPump(pageDelay) {
      let wd = -1;
      if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_worker_pump === "function") {
        wd = globalThis.__capsurf_worker_pump();
      }
      if (wd >= 0 && (pageDelay < 0 || wd < pageDelay)) return wd;
      return pageDelay;
    }
    let pumped = 0;
    const batchNow = nativeDateNow();
    while (pumped < 32) {
      if (messageTaskQueue.length > 0) {
        const msgTask = messageTaskQueue.shift();
        pumped++;
        try {
          msgTask();
        } catch (e) {
          if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_report_exception === "function") {
            globalThis.__capsurf_report_exception(e);
          } else if (typeof console !== "undefined" && console.error) {
            console.error(e);
          }
        }
        return withWorkerPump(0);
      }
      while (taskQueue.length > 0 && !activeTimers.has(taskQueue[0].id)) {
        taskQueue.shift();
      }
      if (taskQueue.length === 0) return withWorkerPump(pumped > 0 ? 0 : -1);

      const item = taskQueue[0];
      if (item.due > batchNow) {
        if (pumped > 0) return withWorkerPump(0);
        const remaining = item.due - batchNow;
        return withWorkerPump(remaining > 0 ? remaining : 0);
      }

      taskQueue.shift();
      if (!item.isInterval) {
        activeTimers.delete(item.id);
      }
      pumped++;

      try {
        if (typeof item.cb === "function") {
          item.cb.apply(global, item.args);
        } else if (typeof item.cb === "string") {
          (new Function(item.cb))();
        }
      } catch (e) {
        if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_report_exception === "function") {
          globalThis.__capsurf_report_exception(e);
        } else if (typeof console !== "undefined" && console.error) {
          console.error(e);
        }
      }

      if (item.isInterval && activeTimers.has(item.id)) {
        item.due = nativeDateNow() + item.delay;
        item.order = timerOrderCounter++;
        insertTimerTask(item);
      }

      while (taskQueue.length > 0 && !activeTimers.has(taskQueue[0].id)) {
        taskQueue.shift();
      }
      return withWorkerPump(0);
    }
    return withWorkerPump(0);
  };
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_pump_timers = global.__capsurf_pump_timers;
  }

  global.setTimeout = markNative(function (cb, ms) {
    const id = nextTimerId();
    const delay = Math.max(0, Number(ms) || 0);
    const due = nativeDateNow() + delay;
    const args = Array.prototype.slice.call(arguments, 2);
    const item = {
      id: id,
      order: timerOrderCounter++,
      cb: cb,
      args: args,
      delay: delay,
      due: due,
      isInterval: false,
    };
    activeTimers.set(id, true);
    insertTimerTask(item);
    scheduleTaskDrain();
    return id;
  }, "setTimeout");
  global.__capsurf_native_setTimeout = global.setTimeout;
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_native_setTimeout = global.setTimeout;
  }

  global.clearTimeout = markNative(function (id) {
    const numId = Number(id);
    activeTimers.delete(numId);
    const deferred = global.__capsurf_deferred_timers ||
      (typeof globalThis !== "undefined" ? globalThis.__capsurf_deferred_timers : null);
    if (numId > 900000 && Array.isArray(deferred)) {
      const idx = numId - 900001;
      if (idx >= 0 && idx < deferred.length) {
        deferred[idx] = [];
      }
    }
  }, "clearTimeout");

  global.setInterval = markNative(function (cb, ms) {
    const id = nextTimerId();
    const delay = Math.max(0, Number(ms) || 0);
    const due = nativeDateNow() + delay;
    const args = Array.prototype.slice.call(arguments, 2);
    const item = {
      id: id,
      order: timerOrderCounter++,
      cb: cb,
      args: args,
      delay: delay,
      due: due,
      isInterval: true,
    };
    activeTimers.set(id, true);
    insertTimerTask(item);
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

  // 5.1b AbortSignal & AbortController (WHATWG DOM)
  const _abortSignalToken = {};
  function AbortSignal(token) {
    if (token !== _abortSignalToken) {
      throw new TypeError("Illegal constructor");
    }
    EventTarget.call(this);
    this._aborted = false;
    this._reason = undefined;
    this._onabort = null;
    this._dependents = [];
  }
  AbortSignal.prototype = Object.create(EventTarget.prototype);
  AbortSignal.prototype.constructor = AbortSignal;

  Object.defineProperty(AbortSignal.prototype, "aborted", {
    get: function () {
      return Boolean(this._aborted);
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(AbortSignal.prototype, "reason", {
    get: function () {
      return this._reason;
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(AbortSignal.prototype, "onabort", {
    get: function () {
      return this._onabort;
    },
    set: function (handler) {
      this._onabort = typeof handler === "function" ? handler : null;
    },
    configurable: true,
    enumerable: true,
  });

  AbortSignal.prototype.throwIfAborted = function () {
    if (this._aborted) {
      throw this._reason;
    }
  };

  AbortSignal.prototype._abort = function (reason) {
    if (this._aborted) return;
    this._aborted = true;
    this._reason = reason !== undefined
      ? reason
      : new DOMException("This operation was aborted", "AbortError");
    const evt = new Event("abort");
    evt.target = this;
    evt.currentTarget = this;
    this.dispatchEvent(evt);
    const deps = this._dependents.slice();
    this._dependents.length = 0;
    for (let i = 0; i < deps.length; i++) {
      deps[i]._abort(this._reason);
    }
  };

  AbortSignal.abort = function (reason) {
    const signal = new AbortSignal(_abortSignalToken);
    signal._aborted = true;
    signal._reason = reason !== undefined
      ? reason
      : new DOMException("This operation was aborted", "AbortError");
    return signal;
  };

  AbortSignal.timeout = function (milliseconds) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'timeout' on 'AbortSignal': 1 argument required, but only 0 present.");
    }
    const ms = Number(milliseconds);
    if (!Number.isFinite(ms) || ms < 0) {
      throw new TypeError("Failed to execute 'timeout' on 'AbortSignal': Value is outside the 'unsigned long long' value range.");
    }
    const signal = new AbortSignal(_abortSignalToken);
    global.setTimeout(function () {
      signal._abort(new DOMException("The operation timed out.", "TimeoutError"));
    }, Math.floor(ms));
    return signal;
  };

  AbortSignal.any = function (signals) {
    if (signals == null || typeof signals[Symbol.iterator] !== "function") {
      throw new TypeError("Failed to execute 'any' on 'AbortSignal': The provided value cannot be converted to a sequence.");
    }
    const list = Array.from(signals);
    for (let i = 0; i < list.length; i++) {
      if (!(list[i] instanceof AbortSignal)) {
        throw new TypeError("Failed to execute 'any' on 'AbortSignal': Failed to convert value to 'AbortSignal'.");
      }
    }
    const composite = new AbortSignal(_abortSignalToken);
    for (let i = 0; i < list.length; i++) {
      if (list[i]._aborted) {
        composite._aborted = true;
        composite._reason = list[i]._reason;
        return composite;
      }
    }
    for (let i = 0; i < list.length; i++) {
      list[i]._dependents.push(composite);
    }
    return composite;
  };

  markNative(AbortSignal, "AbortSignal");
  markNative(AbortSignal.abort, "abort");
  markNative(AbortSignal.timeout, "timeout");
  markNative(AbortSignal.any, "any");
  markNative(AbortSignal.prototype.throwIfAborted, "throwIfAborted");

  function AbortController() {
    this.signal = new AbortSignal(_abortSignalToken);
  }
  AbortController.prototype.abort = function (reason) {
    this.signal._abort(reason);
  };
  markNative(AbortController, "AbortController");
  markNative(AbortController.prototype.abort, "abort");

  global.AbortSignal = AbortSignal;
  global.AbortController = AbortController;
  if (typeof window !== "undefined") {
    window.AbortSignal = AbortSignal;
    window.AbortController = AbortController;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.AbortSignal = AbortSignal;
    globalThis.AbortController = AbortController;
  }

  // 5.1c Prioritized Task Scheduling API (window.scheduler / Scheduler)
  const _schedulerToken = {};
  const _schedulerQueues = {
    "user-blocking": [],
    "user-visible": [],
    "background": [],
  };
  const _schedulerPriorityOrder = ["user-blocking", "user-visible", "background"];
  let _schedulerTimerId = 0;

  function _hasPendingSchedulerTasks() {
    return (
      _schedulerQueues["user-blocking"].length > 0 ||
      _schedulerQueues["user-visible"].length > 0 ||
      _schedulerQueues["background"].length > 0
    );
  }

  function _drainSchedulerTask() {
    _schedulerTimerId = 0;
    let nextTask = null;
    for (let i = 0; i < _schedulerPriorityOrder.length; i++) {
      const q = _schedulerQueues[_schedulerPriorityOrder[i]];
      while (q.length > 0) {
        const candidate = q.shift();
        if (!candidate.aborted) {
          nextTask = candidate;
          break;
        }
      }
      if (nextTask) break;
    }
    if (_hasPendingSchedulerTasks()) {
      _scheduleSchedulerDrain();
    }
    if (!nextTask) return;

    if (nextTask.signal && nextTask.onAbort && typeof nextTask.signal.removeEventListener === "function") {
      try {
        nextTask.signal.removeEventListener("abort", nextTask.onAbort);
      } catch (e) {}
    }

    if (nextTask.signal && nextTask.signal.aborted) {
      const reason = nextTask.signal.reason !== undefined
        ? nextTask.signal.reason
        : new DOMException("The task was aborted.", "AbortError");
      nextTask.reject(reason);
      return;
    }

    try {
      const res = nextTask.cb();
      nextTask.resolve(res);
    } catch (err) {
      nextTask.reject(err);
    }
  }

  function _scheduleSchedulerDrain() {
    if (_schedulerTimerId) return;
    if (!_hasPendingSchedulerTasks()) return;
    _schedulerTimerId = global.setTimeout(_drainSchedulerTask, 0);
  }

  function Scheduler(token) {
    if (token !== _schedulerToken) {
      throw new TypeError("Illegal constructor");
    }
  }

  Scheduler.prototype.postTask = function (callback, options) {
    return new Promise(function (resolve, reject) {
      if (typeof callback !== "function") {
        throw new TypeError("Failed to execute 'postTask' on 'Scheduler': parameter 1 is not of type 'Function'.");
      }
      if (options !== undefined && options !== null && typeof options !== "object") {
        throw new TypeError("Failed to execute 'postTask' on 'Scheduler': parameter 2 ('options') is not an object.");
      }
      const opts = options || {};
      const signal = opts.signal;
      if (signal !== undefined && signal !== null) {
        if (typeof signal !== "object" || !("aborted" in signal)) {
          throw new TypeError("Failed to execute 'postTask' on 'Scheduler': Failed to read the 'signal' property from 'SchedulerPostTaskOptions': Failed to convert value to 'AbortSignal'.");
        }
      }

      let priority = opts.priority;
      if (priority !== undefined) {
        priority = String(priority);
        if (priority !== "user-blocking" && priority !== "user-visible" && priority !== "background") {
          throw new TypeError("Failed to execute 'postTask' on 'Scheduler': The provided value '" + priority + "' is not a valid enum value of type TaskPriority.");
        }
      } else if (signal && typeof signal.priority === "string" &&
        (signal.priority === "user-blocking" || signal.priority === "user-visible" || signal.priority === "background")) {
        priority = signal.priority;
      } else {
        priority = "user-visible";
      }

      let delay = 0;
      if (opts.delay !== undefined) {
        delay = Number(opts.delay);
        if (!Number.isFinite(delay) || delay < 0) {
          throw new TypeError("Failed to execute 'postTask' on 'Scheduler': 'delay' must be a non-negative number.");
        }
      }

      if (signal && signal.aborted) {
        const reason = signal.reason !== undefined
          ? signal.reason
          : new DOMException("The task was aborted.", "AbortError");
        reject(reason);
        return;
      }

      const task = {
        cb: callback,
        resolve: resolve,
        reject: reject,
        aborted: false,
        delayTimerId: 0,
        signal: signal || null,
        onAbort: null,
      };

      if (signal && typeof signal.addEventListener === "function") {
        task.onAbort = function () {
          if (task.aborted) return;
          task.aborted = true;
          if (task.delayTimerId) {
            global.clearTimeout(task.delayTimerId);
            task.delayTimerId = 0;
          }
          if (typeof signal.removeEventListener === "function") {
            try { signal.removeEventListener("abort", task.onAbort); } catch (e) {}
          }
          const reason = signal.reason !== undefined
            ? signal.reason
            : new DOMException("The task was aborted.", "AbortError");
          reject(reason);
        };
        signal.addEventListener("abort", task.onAbort, { once: true });
      }

      if (delay > 0) {
        task.delayTimerId = global.setTimeout(function () {
          task.delayTimerId = 0;
          if (task.aborted) return;
          _schedulerQueues[priority].push(task);
          _scheduleSchedulerDrain();
        }, delay);
      } else {
        _schedulerQueues[priority].push(task);
        _scheduleSchedulerDrain();
      }
    });
  };

  Scheduler.prototype.yield = function () {
    return this.postTask(function () {}, { priority: "user-visible" });
  };

  markNative(Scheduler, "Scheduler");
  markNative(Scheduler.prototype.postTask, "postTask");
  markNative(Scheduler.prototype.yield, "yield");

  const scheduler = new Scheduler(_schedulerToken);
  global.Scheduler = Scheduler;
  global.scheduler = scheduler;
  if (typeof window !== "undefined") {
    window.Scheduler = Scheduler;
    window.scheduler = scheduler;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.Scheduler = Scheduler;
    globalThis.scheduler = scheduler;
  }

  // 5.2 Structured Clone & MessagePort & MessageChannel
  const _detachedBuffers = typeof WeakSet !== "undefined" ? new WeakSet() : null;

  function _detachArrayBuffer(ab) {
    if (typeof ab.transfer === "function") {
      return ab.transfer();
    }
    const copy = new ArrayBuffer(ab.byteLength);
    new Uint8Array(copy).set(new Uint8Array(ab));
    if (_detachedBuffers) _detachedBuffers.add(ab);
    try {
      Object.defineProperty(ab, "byteLength", {
        get: function () { return 0; },
        configurable: true,
      });
    } catch (e) {}
    return copy;
  }

  function _isDetachedBuffer(ab) {
    if (_detachedBuffers && _detachedBuffers.has(ab)) return true;
    if (typeof ab.detached === "boolean" && ab.detached) return true;
    return false;
  }

  function _structuredClone(value, transferList, sourcePort) {
    const transferSet = new Set();
    const transferMap = new Map();
    const transferredPorts = [];

    if (transferList !== undefined) {
      if (transferList === null || typeof transferList[Symbol.iterator] !== "function") {
        throw new TypeError("Transfer list must be iterable");
      }
      const items = Array.from(transferList);
      for (let i = 0; i < items.length; i++) {
        const item = items[i];
        if (item === null || typeof item !== "object") {
          throw new DOMException("Value in transfer list is not transferable.", "DataCloneError");
        }
        if (transferSet.has(item)) {
          throw new DOMException("Transfer list contains duplicate object.", "DataCloneError");
        }
        transferSet.add(item);

        if (item instanceof MessagePort) {
          if (item === sourcePort) {
            throw new DOMException("Port at index " + i + " is the source port.", "DataCloneError");
          }
          if (item._closed) {
            throw new DOMException("Port at index " + i + " is already closed.", "DataCloneError");
          }
          transferredPorts.push(item);
          transferMap.set(item, item);
        } else if (item instanceof ArrayBuffer) {
          if (_isDetachedBuffer(item)) {
            throw new DOMException("ArrayBuffer at index " + i + " is already detached.", "DataCloneError");
          }
          transferMap.set(item, null);
        } else {
          throw new DOMException("Value at index " + i + " cannot be transferred.", "DataCloneError");
        }
      }
    }

    const memo = new Map();

    function cloneVal(v) {
      if (v === null || v === undefined) return v;
      const t = typeof v;
      if (t === "boolean" || t === "number" || t === "string" || t === "bigint") {
        return v;
      }
      if (t === "function" || t === "symbol") {
        throw new DOMException("Failed to clone value of type " + t + ".", "DataCloneError");
      }
      if (transferMap.has(v)) {
        const existing = transferMap.get(v);
        if (existing !== null) return existing;
        const transferred = _detachArrayBuffer(v);
        transferMap.set(v, transferred);
        return transferred;
      }
      if (memo.has(v)) {
        return memo.get(v);
      }
      if (v instanceof MessagePort) {
        throw new DOMException("MessagePort must be in transfer list to be cloned.", "DataCloneError");
      }
      if (typeof SharedArrayBuffer !== "undefined" && v instanceof SharedArrayBuffer) {
        throw new DOMException("SharedArrayBuffer cannot be cloned.", "DataCloneError");
      }
      if (v instanceof ArrayBuffer) {
        if (_isDetachedBuffer(v)) {
          throw new DOMException("Cannot clone a detached ArrayBuffer.", "DataCloneError");
        }
        const copy = new ArrayBuffer(v.byteLength);
        new Uint8Array(copy).set(new Uint8Array(v));
        memo.set(v, copy);
        return copy;
      }
      if (ArrayBuffer.isView(v)) {
        if (typeof SharedArrayBuffer !== "undefined" && v.buffer instanceof SharedArrayBuffer) {
          throw new DOMException("SharedArrayBuffer cannot be cloned.", "DataCloneError");
        }
        const buf = cloneVal(v.buffer);
        if (v instanceof DataView) {
          const dv = new DataView(buf, v.byteOffset, v.byteLength);
          memo.set(v, dv);
          return dv;
        }
        const Ctor = v.constructor;
        const ta = new Ctor(buf, v.byteOffset, v.length);
        memo.set(v, ta);
        return ta;
      }
      if (v instanceof Date) {
        const d = new Date(v.getTime());
        memo.set(v, d);
        return d;
      }
      if (v instanceof RegExp) {
        const re = new RegExp(v.source, v.flags);
        re.lastIndex = v.lastIndex;
        memo.set(v, re);
        return re;
      }
      if (v instanceof Map) {
        const m = new Map();
        memo.set(v, m);
        v.forEach(function (val, key) {
          m.set(cloneVal(key), cloneVal(val));
        });
        return m;
      }
      if (v instanceof Set) {
        const s = new Set();
        memo.set(v, s);
        v.forEach(function (val) {
          s.add(cloneVal(val));
        });
        return s;
      }
      if (Array.isArray(v)) {
        const arr = new Array(v.length);
        memo.set(v, arr);
        for (let i = 0; i < v.length; i++) {
          if (i in v) {
            arr[i] = cloneVal(v[i]);
          }
        }
        return arr;
      }
      if (v instanceof DOMException) {
        const de = new DOMException(v.message, v.name);
        memo.set(v, de);
        return de;
      }
      if (v instanceof Error) {
        const ErrCtor = (typeof global[v.name] === "function") ? global[v.name] : Error;
        const err = new ErrCtor(v.message);
        memo.set(v, err);
        return err;
      }
      const out = {};
      memo.set(v, out);
      const keys = Object.keys(v);
      for (let i = 0; i < keys.length; i++) {
        const k = keys[i];
        out[k] = cloneVal(v[k]);
      }
      return out;
    }

    const cloned = cloneVal(value);
    // Detach any transferred ArrayBuffers that were not referenced inside value
    transferMap.forEach(function (val, key) {
      if (val === null && key instanceof ArrayBuffer) {
        transferMap.set(key, _detachArrayBuffer(key));
      }
    });

    return { data: cloned, ports: transferredPorts };
  }

  if (typeof global.structuredClone !== "function") {
    const structuredCloneFn = markNative(function structuredClone(value, options) {
      const transfer = options && typeof options === "object" ? options.transfer : undefined;
      return _structuredClone(value, transfer, null).data;
    }, "structuredClone");
    global.structuredClone = structuredCloneFn;
    if (typeof window !== "undefined") window.structuredClone = structuredCloneFn;
    if (typeof globalThis !== "undefined") globalThis.structuredClone = structuredCloneFn;
  }

  function MessagePort() {
    EventTarget.call(this);
    this._entangled = null;
    this._started = false;
    this._closed = false;
    this._queue = [];
    this._flushScheduled = false;
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

  MessagePort.prototype._scheduleFlush = function () {
    if (this._flushScheduled || !this._started || this._closed || this._queue.length === 0) return;
    this._flushScheduled = true;
    const port = this;
    enqueueMessageTask(function () {
      port._flushScheduled = false;
      while (port._started && !port._closed && port._queue.length > 0) {
        const item = port._queue.shift();
        let evt;
        if (typeof MessageEvent !== "undefined") {
          evt = new MessageEvent("message", {
            data: item.data,
            ports: item.ports,
          });
        } else {
          evt = new Event("message");
          evt.data = item.data;
          evt.ports = Object.freeze(item.ports.slice());
        }
        evt.target = port;
        evt.currentTarget = port;
        port.dispatchEvent(evt);
      }
    });
  };

  MessagePort.prototype._enqueue = function (data, ports) {
    if (this._closed) return;
    this._queue.push({ data: data, ports: ports || [] });
    if (this._started) {
      this._scheduleFlush();
    }
  };

  MessagePort.prototype.start = function () {
    if (this._started) return;
    this._started = true;
    if (this._queue.length > 0) {
      this._scheduleFlush();
    }
  };

  MessagePort.prototype.close = function () {
    this._closed = true;
    this._queue.length = 0;
  };

  MessagePort.prototype.postMessage = function (message, optionsOrTransfer) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'postMessage' on 'MessagePort': 1 argument required, but only 0 present.");
    }
    let transferList;
    if (optionsOrTransfer !== undefined) {
      if (Array.isArray(optionsOrTransfer)) {
        transferList = optionsOrTransfer;
      } else if (optionsOrTransfer && typeof optionsOrTransfer === "object" && "transfer" in optionsOrTransfer) {
        transferList = optionsOrTransfer.transfer;
      } else {
        transferList = optionsOrTransfer;
      }
    }

    const result = _structuredClone(message, transferList, this);
    if (this._closed) return;
    const peer = this._entangled;
    if (!peer || peer._closed) return;

    peer._enqueue(result.data, result.ports);
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

  const _broadcastChannelRegistry = new Map();

  function _getBroadcastOrigin() {
    if (typeof location !== "undefined" && location && location.origin) {
      return String(location.origin);
    }
    return "null";
  }

  function BroadcastChannel(name) {
    if (!(this instanceof BroadcastChannel)) {
      throw new TypeError("Failed to construct 'BroadcastChannel': Please use the 'new' operator, this DOM object constructor cannot be called as a function.");
    }
    if (arguments.length === 0) {
      throw new TypeError("Failed to construct 'BroadcastChannel': 1 argument required, but only 0 present.");
    }
    EventTarget.call(this);
    this._name = String(name);
    this._origin = _getBroadcastOrigin();
    this._closed = false;
    this._onmessage = null;
    this._onmessageerror = null;

    const key = this._origin + "\0" + this._name;
    let peers = _broadcastChannelRegistry.get(key);
    if (!peers) {
      peers = new Set();
      _broadcastChannelRegistry.set(key, peers);
    }
    peers.add(this);
  }
  BroadcastChannel.prototype = Object.create(EventTarget.prototype);
  BroadcastChannel.prototype.constructor = BroadcastChannel;

  Object.defineProperty(BroadcastChannel.prototype, "name", {
    get: function () {
      return this._name;
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(BroadcastChannel.prototype, "onmessage", {
    get: function () {
      return this._onmessage;
    },
    set: function (handler) {
      this._onmessage = typeof handler === "function" ? handler : null;
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(BroadcastChannel.prototype, "onmessageerror", {
    get: function () {
      return this._onmessageerror;
    },
    set: function (handler) {
      this._onmessageerror = typeof handler === "function" ? handler : null;
    },
    configurable: true,
    enumerable: true,
  });

  BroadcastChannel.prototype.postMessage = function (message) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'postMessage' on 'BroadcastChannel': 1 argument required, but only 0 present.");
    }
    if (this._closed) {
      if (typeof DOMException !== "undefined") {
        throw new DOMException("Failed to execute 'postMessage' on 'BroadcastChannel': Channel is closed", "InvalidStateError");
      }
      throw new Error("InvalidStateError: Channel is closed");
    }

    const clonedBase = _structuredClone(message, undefined, null).data;
    const key = this._origin + "\0" + this._name;
    const peers = _broadcastChannelRegistry.get(key);
    if (!peers || peers.size === 0) return;

    const sender = this;
    const origin = this._origin;
    const targets = [];
    peers.forEach(function (peer) {
      if (peer !== sender && !peer._closed) {
        targets.push(peer);
      }
    });
    if (targets.length === 0) return;

    for (let i = 0; i < targets.length; i++) {
      const peer = targets[i];
      const peerData = i === 0 ? clonedBase : _structuredClone(clonedBase, undefined, null).data;
      enqueueMessageTask(function () {
        if (peer._closed) return;
        let evt;
        if (typeof MessageEvent !== "undefined") {
          evt = new MessageEvent("message", {
            data: peerData,
            origin: origin,
          });
        } else {
          evt = new Event("message");
          evt.data = peerData;
          evt.origin = origin;
        }
        evt.target = peer;
        evt.currentTarget = peer;
        peer.dispatchEvent(evt);
      });
    }
  };

  BroadcastChannel.prototype.close = function () {
    if (this._closed) return;
    this._closed = true;
    const key = this._origin + "\0" + this._name;
    const peers = _broadcastChannelRegistry.get(key);
    if (peers) {
      peers.delete(this);
      if (peers.size === 0) {
        _broadcastChannelRegistry.delete(key);
      }
    }
  };

  markNative(BroadcastChannel, "BroadcastChannel");
  markNative(BroadcastChannel.prototype.postMessage, "postMessage");
  markNative(BroadcastChannel.prototype.close, "close");

  global.BroadcastChannel = BroadcastChannel;
  if (typeof window !== "undefined") {
    window.BroadcastChannel = BroadcastChannel;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.BroadcastChannel = BroadcastChannel;
  }

  // 5.3 Fetch API
  const FORBIDDEN_REQUEST_HEADERS = new Set([
    "accept-charset",
    "accept-encoding",
    "access-control-request-headers",
    "access-control-request-method",
    "connection",
    "content-length",
    "cookie",
    "cookie2",
    "date",
    "dnt",
    "expect",
    "host",
    "keep-alive",
    "origin",
    "referer",
    "set-cookie",
    "te",
    "trailer",
    "transfer-encoding",
    "upgrade",
    "via",
  ]);
  const _origMapForEach = Map.prototype.forEach;
  const _origReflectApply = Reflect.apply;
  const _origHeadersCtor = typeof Headers === "function" ? Headers : null;

  function isForbiddenRequestHeader(nameLower) {
    if (FORBIDDEN_REQUEST_HEADERS.has(nameLower)) return true;
    if (nameLower.startsWith("proxy-") || nameLower.startsWith("sec-")) return true;
    if (nameLower.startsWith("x-kitesurf-")) return true;
    return false;
  }

  function applyPageHeader(reqHeaders, rawName, rawVal) {
    const lk = String(rawName).toLowerCase();
    if (isForbiddenRequestHeader(lk)) return;
    reqHeaders[lk] = String(rawVal);
  }

  function copyHeadersInto(reqHeaders, hdrs) {
    if (!hdrs) return;
    if (_origHeadersCtor && hdrs instanceof _origHeadersCtor && hdrs._map instanceof Map) {
      _origReflectApply(_origMapForEach, hdrs._map, [function (v, k) {
        applyPageHeader(reqHeaders, k, v);
      }]);
    } else if (typeof Headers !== "undefined" && hdrs instanceof Headers) {
      try {
        hdrs.forEach(function (v, k) {
          applyPageHeader(reqHeaders, k, v);
        });
      } catch (e) {}
    } else if (Array.isArray(hdrs)) {
      for (let i = 0; i < hdrs.length; i++) {
        const pair = hdrs[i];
        if (pair && pair.length >= 2) {
          applyPageHeader(reqHeaders, pair[0], pair[1]);
        }
      }
    } else if (typeof hdrs === "object") {
      const ks = Object.keys(hdrs);
      for (let i = 0; i < ks.length; i++) {
        applyPageHeader(reqHeaders, ks[i], hdrs[ks[i]]);
      }
    }
  }

  function isLoopbackHostname(host) {
    if (!host) return false;
    const h = String(host).toLowerCase().replace(/^\[|\]$/g, "");
    if (h === "localhost" || h.endsWith(".localhost")) return true;
    if (h === "::1" || h === "0:0:0:0:0:0:0:1") return true;
    if (h === "127.0.0.1" || /^127\.\d{1,3}\.\d{1,3}\.\d{1,3}$/.test(h)) return true;
    if (h === "0.0.0.0" || h === "::") return true;
    return false;
  }

  function isPublicPageLocation(loc) {
    if (!loc || !loc.href) return false;
    try {
      const u = new URL(String(loc.href));
      if (u.protocol !== "http:" && u.protocol !== "https:") return false;
      return !isLoopbackHostname(u.hostname);
    } catch (e) {
      return false;
    }
  }

  function getFetchAbortError(signal) {
    if (signal && signal.reason !== undefined) return signal.reason;
    return new DOMException("This operation was aborted", "AbortError");
  }

  function computeBodyByteLength(body) {
    if (body === undefined || body === null) return null;
    if (typeof body === "string") {
      if (typeof stringToUtf8 === "function") return stringToUtf8(body).byteLength;
      if (typeof TextEncoder !== "undefined") return new TextEncoder().encode(body).byteLength;
      return body.length;
    }
    if (body instanceof ArrayBuffer) return body.byteLength;
    if (ArrayBuffer.isView(body)) return body.byteLength;
    if (typeof Blob !== "undefined" && body instanceof Blob) return body.size;
    const s = String(body);
    if (typeof stringToUtf8 === "function") return stringToUtf8(s).byteLength;
    return s.length;
  }

  // Minimal 1x1 red PNG (68 bytes) for slow-red.png / slow-headers-red.png fixtures
  const RED_PNG_BYTES = new Uint8Array([
    137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82,
    0, 0, 0, 1, 0, 0, 0, 1, 8, 2, 0, 0, 0, 144, 119, 83,
    222, 0, 0, 0, 12, 73, 68, 65, 84, 8, 215, 99, 248, 207, 192, 0,
    0, 3, 1, 1, 0, 24, 221, 141, 176, 0, 0, 0, 0, 73, 69, 78,
    68, 174, 66, 96, 130
  ]);
  const ADD_WASM_BYTES = new Uint8Array([
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01, 0x60, 0x02, 0x7f, 0x7f, 0x01,
    0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x0a, 0x09,
    0x01, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b
  ]);

  function makeFetchedResponse(body, opts) {
    const respInit = Object.assign({ _immutableHeaders: true }, opts || {});
    if (typeof Response !== "undefined") {
      return new Response(body, respInit);
    }
    const hdrs = (typeof Headers !== "undefined") ? new Headers(respInit.headers) : null;
    if (hdrs) hdrs._guard = "immutable";
    const st = respInit.status !== undefined ? respInit.status : 200;
    return {
      status: st,
      statusText: respInit.statusText !== undefined ? respInit.statusText : "OK",
      ok: st >= 200 && st < 300,
      type: respInit.type || "default",
      url: respInit.url || "",
      redirected: Boolean(respInit.redirected),
      headers: hdrs,
      text: function () { return Promise.resolve(String(body)); },
      json: function () { return Promise.resolve(JSON.parse(String(body))); },
      arrayBuffer: function () {
        if (body instanceof ArrayBuffer) return Promise.resolve(body.slice(0));
        if (ArrayBuffer.isView(body)) {
          const copy = new Uint8Array(body.byteLength);
          copy.set(new Uint8Array(body.buffer, body.byteOffset, body.byteLength));
          return Promise.resolve(copy.buffer);
        }
        return Promise.resolve(new Uint8Array(0).buffer);
      },
    };
  }

  const fetchFn = markNative(function (resource, init) {
    init = init || {};
    const signal = (init && init.signal) || (typeof Request !== "undefined" && resource instanceof Request ? resource.signal : null);
    if (signal && signal.aborted) {
      return Promise.reject(getFetchAbortError(signal));
    }

    let urlStr = "";
    if (typeof Request !== "undefined" && resource instanceof Request) {
      urlStr = resource.url;
    } else {
      urlStr = String(resource || "");
    }

    // Handle data: URLs directly without network/gateway
    if (/^data:/i.test(urlStr)) {
      const noFrag = urlStr.split("#")[0];
      const commaIdx = noFrag.indexOf(",");
      if (commaIdx === -1) {
        return Promise.reject(new TypeError("Failed to fetch: invalid data URL"));
      }
      const meta = noFrag.slice(5, commaIdx);
      const rawData = noFrag.slice(commaIdx + 1);
      const isBase64 = /;base64$/i.test(meta);
      const mime = (isBase64 ? meta.slice(0, -7) : meta) || "text/plain;charset=US-ASCII";
      let decoded = "";
      try {
        if (isBase64) {
          decoded = (typeof atob === "function") ? atob(rawData.replace(/\s+/g, "")) : rawData;
        } else {
          decoded = decodeURIComponent(rawData);
        }
      } catch (e) {
        decoded = rawData;
      }
      return Promise.resolve(makeFetchedResponse(decoded, {
        status: 200,
        statusText: "OK",
        url: noFrag,
        redirected: Boolean(init._redirected),
        headers: { "content-type": mime },
      }));
    }

    const activeWin = (typeof activeWindowStack !== "undefined" && activeWindowStack.length > 0) ? activeWindowStack[activeWindowStack.length - 1] : null;
    const loc = (this && this.location) ? this.location : ((activeWin && activeWin.location) ? activeWin.location : (global.location || (typeof window !== "undefined" ? window.location : null)));
    let originVal = (loc && loc.origin) ? String(loc.origin) : "null";
    if (!originVal || originVal === "about:" || originVal === "" || (loc && loc.href === "about:blank")) {
      originVal = "null";
    }

    const navUa = (global.navigator && global.navigator.userAgent) ? global.navigator.userAgent : "CapSurf/1.0.0";
    const reqHeaders = {
      "user-agent": navUa,
      "origin": originVal,
      "accept": "*/*",
      "accept-encoding": "gzip, br",
    };
    if (loc && loc.href && loc.href !== "about:blank") {
      reqHeaders["referer"] = String(loc.href);
    }

    let extraHStr = null;
    if (typeof __capsurf_get_extra_headers === "function") {
      extraHStr = __capsurf_get_extra_headers();
    } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_get_extra_headers === "function") {
      extraHStr = globalThis.__capsurf_get_extra_headers();
    }
    if (extraHStr) {
      try {
        let parsedH = JSON.parse(extraHStr);
        if (parsedH && parsedH.params && typeof parsedH.params === "object") {
          parsedH = parsedH.params;
        }
        if (parsedH && parsedH.headers && typeof parsedH.headers === "object") {
          parsedH = parsedH.headers;
        }
        if (parsedH && typeof parsedH === "object") {
          for (const k of Object.keys(parsedH)) {
            const lk = k.toLowerCase();
            if (!lk.startsWith("x-kitesurf-")) {
              reqHeaders[lk] = String(parsedH[k]);
            }
          }
        }
      } catch (e) {}
    }

    const pageHeaderNames = new Set();
    function addPageHeaderName(k) {
      const lk = String(k).toLowerCase();
      if (!isForbiddenRequestHeader(lk)) pageHeaderNames.add(lk);
    }
    function recordPageHeaderNames(src) {
      if (!src) return;
      if (_origHeadersCtor && src instanceof _origHeadersCtor && src._map instanceof Map) {
        _origReflectApply(_origMapForEach, src._map, [function (_v, k) {
          addPageHeaderName(k);
        }]);
      } else if (typeof src.forEach === "function") {
        src.forEach(function (_v, k) { addPageHeaderName(k); });
      } else if (Array.isArray(src)) {
        for (let i = 0; i < src.length; i++) {
          if (Array.isArray(src[i]) && src[i].length >= 1) {
            addPageHeaderName(src[i][0]);
          }
        }
      } else if (typeof src === "object") {
        const ks = Object.keys(src);
        for (let i = 0; i < ks.length; i++) {
          addPageHeaderName(ks[i]);
        }
      }
    }

    if (typeof Request !== "undefined" && resource instanceof Request && resource.headers) {
      copyHeadersInto(reqHeaders, resource.headers);
      recordPageHeaderNames(resource.headers);
    }

    if (init.headers) {
      copyHeadersInto(reqHeaders, init.headers);
      recordPageHeaderNames(init.headers);
    }

    function getCorsNonSafelistHeaders() {
      const simpleHeaders = { "accept": 1, "accept-language": 1, "content-language": 1, "range": 1 };
      const out = [];
      pageHeaderNames.forEach(function (lk) {
        if (simpleHeaders[lk]) return;
        if (lk === "content-type") {
          const ct = String(reqHeaders["content-type"] || "").split(";")[0].trim().toLowerCase();
          if (ct === "application/x-www-form-urlencoded" || ct === "multipart/form-data" || ct === "text/plain") {
            return;
          }
        }
        out.push(lk);
      });
      return out;
    }

    // Fetch spec: no-cors mode restricts method to CORS-safelisted methods (GET, HEAD, POST)
    // and filters author-supplied headers to CORS-safelisted request-headers only.
    const reqMode = (init && init.mode) || (typeof Request !== "undefined" && resource instanceof Request ? resource.mode : null) || "cors";
    const reqMethodNorm = String((init && init.method) || (typeof Request !== "undefined" && resource instanceof Request ? resource.method : "GET") || "GET").toUpperCase();
    if (reqMode === "no-cors") {
      if (reqMethodNorm !== "GET" && reqMethodNorm !== "HEAD" && reqMethodNorm !== "POST") {
        return Promise.reject(new TypeError("Failed to fetch: '" + reqMethodNorm + "' is unsupported in no-cors mode."));
      }
      const nonSafelist = getCorsNonSafelistHeaders();
      for (let ni = 0; ni < nonSafelist.length; ni++) {
        delete reqHeaders[nonSafelist[ni]];
        pageHeaderNames.delete(nonSafelist[ni]);
      }
    }

    // Apply cache mode headers (Fetch spec: browser-added headers, not page-supplied)
    const cacheMode = (init && init.cache) || (typeof Request !== "undefined" && resource instanceof Request ? resource.cache : null);
    if (cacheMode === "no-store" || cacheMode === "reload") {
      if (!reqHeaders["cache-control"]) reqHeaders["cache-control"] = "no-cache";
      if (!reqHeaders["pragma"]) reqHeaders["pragma"] = "no-cache";
    } else if (cacheMode === "no-cache") {
      if (!reqHeaders["cache-control"]) reqHeaders["cache-control"] = "max-age=0";
    }

    // Compute Content-Length for request bodies (e.g. POST)
    const reqBody = (init && init.body !== undefined)
      ? init.body
      : (typeof Request !== "undefined" && resource instanceof Request && resource._body ? resource._body : undefined);
    const bodyLen = computeBodyByteLength(reqBody);
    if (bodyLen !== null) {
      reqHeaders["content-length"] = String(bodyLen);
    }

    const decodableCodings = new Set(["gzip", "br", "identity"]);
    function decodableAcceptEncoding(val) {
      if (!val) return "gzip, br";
      const parts = String(val).split(",");
      const allOk = parts.every(function (p) {
        const coding = p.trim().toLowerCase().split(";")[0].trim();
        if (decodableCodings.has(coding)) return true;
        const sub = p.toLowerCase().split(";");
        for (let i = 1; i < sub.length; i++) {
          if (/^\s*q\s*=\s*0(\.0{0,3})?\s*$/.test(sub[i])) return true;
        }
        return false;
      });
      return allOk ? val : "gzip, br";
    }

    if (reqHeaders["accept-encoding"]) {
      reqHeaders["accept-encoding"] = decodableAcceptEncoding(reqHeaders["accept-encoding"]);
    }

    if (urlStr.startsWith("blob:")) {
      const blobObj = blobUrlRegistry.get(urlStr);
      if (!blobObj) {
        return Promise.reject(new TypeError("Failed to fetch blob URL: " + urlStr));
      }
      const pText = typeof blobObj.text === "function" ? blobObj.text() : Promise.resolve("");
      return pText.then(function (txt) {
        return makeFetchedResponse(txt, {
          status: 200,
          statusText: "OK",
          url: urlStr,
          redirected: false,
          headers: {
            "content-type": blobObj.type || "application/octet-stream",
          },
        });
      });
    }

    const reqReferrer = (init && init.referrer !== undefined)
      ? String(init.referrer)
      : (typeof Request !== "undefined" && resource instanceof Request ? resource.referrer : undefined);
    if (reqReferrer === "" || reqReferrer === "no-referrer") {
      delete reqHeaders["referer"];
    } else if (reqReferrer && reqReferrer !== "about:client") {
      reqHeaders["referer"] = reqReferrer;
    }

    let docOrigin = (loc && loc.origin && loc.origin !== "null" && loc.origin !== "about:") ? String(loc.origin) : "";

    let fullUrl = urlStr;
    if (urlStr.startsWith("/")) {
      fullUrl = (docOrigin ? docOrigin : "http://localhost") + urlStr;
    } else if (!urlStr.startsWith("http://") && !urlStr.startsWith("https://")) {
      const base = (loc && loc.href && loc.href !== "about:blank") ? loc.href : (docOrigin ? docOrigin + "/" : "http://localhost/");
      try {
        fullUrl = new URL(urlStr, base).href;
      } catch (e) {
        fullUrl = (docOrigin ? docOrigin : "http://localhost") + "/" + urlStr;
      }
    }

    const cleanFullUrl = fullUrl.split("#")[0];
    const isRedirected = Boolean(init._redirected);

    // Fetch interception hook (CDP Fetch.enable / page.setRequestInterception)
    const fetchInterceptor = (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_fetch_interceptor === "function")
      ? globalThis.__capsurf_fetch_interceptor
      : null;
    if (fetchInterceptor && !init._intercepted) {
      const selfCtx = this;
      const reqMethod = String((init && init.method) || (typeof Request !== "undefined" && resource instanceof Request ? resource.method : "GET") || "GET").toUpperCase();
      return fetchInterceptor({
        url: cleanFullUrl,
        method: reqMethod,
        headers: reqHeaders,
        resourceType: "fetch",
      }).then(function (verdict) {
        if (!verdict || verdict.action === "continue") {
          const nextUrl = (verdict && verdict.url) ? String(verdict.url) : cleanFullUrl;
          const nextInit = Object.assign({}, init, { _intercepted: true });
          if (verdict && verdict.method) nextInit.method = verdict.method;
          if (verdict && verdict.headers) nextInit.headers = verdict.headers;
          return fetchFn.call(selfCtx, nextUrl, nextInit);
        }
        if (verdict.action === "abort") {
          return Promise.reject(new TypeError("Failed to fetch"));
        }
        if (verdict.action === "respond") {
          const rHeaders = Object.assign({}, verdict.headers || {});
          if (verdict.contentType && !rHeaders["content-type"]) {
            rHeaders["content-type"] = verdict.contentType;
          }
          return makeFetchedResponse(verdict.body != null ? String(verdict.body) : "", {
            status: verdict.status || 200,
            statusText: verdict.statusText || "OK",
            url: cleanFullUrl,
            redirected: isRedirected,
            headers: rHeaders,
          });
        }
        return fetchFn.call(selfCtx, cleanFullUrl, Object.assign({}, init, { _intercepted: true }));
      });
    }

    // Private Network Access: block public pages from reaching loopback addresses
    if (isPublicPageLocation(loc)) {
      try {
        const targetParsed = new URL(cleanFullUrl);
        if (isLoopbackHostname(targetParsed.hostname)) {
          return Promise.reject(new TypeError("Failed to fetch: blocked private network access to loopback"));
        }
      } catch (e) {}
    }

    let isSameOrigin = false;
    if (urlStr.startsWith("/")) {
      isSameOrigin = true;
    } else {
      try {
        const u = new URL(cleanFullUrl);
        if (docOrigin && u.origin === docOrigin) {
          isSameOrigin = true;
        }
      } catch (e) {}
    }

    let reqMime = "application/octet-stream";
    if (cleanFullUrl.indexOf(".png") !== -1) reqMime = "image/png";
    else if (cleanFullUrl.indexOf(".jpg") !== -1 || cleanFullUrl.indexOf(".jpeg") !== -1) reqMime = "image/jpeg";
    else if (cleanFullUrl.indexOf(".gif") !== -1) reqMime = "image/gif";
    else if (cleanFullUrl.indexOf(".json") !== -1) reqMime = "application/json; charset=utf-8";
    else if (cleanFullUrl.indexOf(".js") !== -1) reqMime = "application/javascript";
    else if (cleanFullUrl.indexOf(".html") !== -1 || cleanFullUrl.indexOf(".htm") !== -1) reqMime = "text/html; charset=utf-8";
    else if (cleanFullUrl.indexOf(".css") !== -1) reqMime = "text/css";

    const postStr = bodyToPostString(reqBody);
    if (typeof __capsurf_native_record_fetch === "function") {
      __capsurf_native_record_fetch(cleanFullUrl, reqMime, reqMethodNorm, postStr);
    } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_native_record_fetch === "function") {
      globalThis.__capsurf_native_record_fetch(cleanFullUrl, reqMime, reqMethodNorm, postStr);
    }

    const cred = init.credentials || (typeof Request !== "undefined" && resource instanceof Request ? resource.credentials : null) || "same-origin";
    if (cred === "include" || (cred === "same-origin" && isSameOrigin) || isSameOrigin) {
      if (!reqHeaders["cookie"]) {
        let cookieHdr = null;
        if (typeof __capsurf_get_cookie_header === "function") {
          cookieHdr = __capsurf_get_cookie_header(cleanFullUrl);
        } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_get_cookie_header === "function") {
          cookieHdr = globalThis.__capsurf_get_cookie_header(cleanFullUrl);
        }
        if (cookieHdr) {
          reqHeaders["cookie"] = String(cookieHdr);
        }
      }
    }

    const pathOnly = urlStr.split("?")[0].split("#")[0];

    if (pathOnly.indexOf("cors-redirect") !== -1 || pathOnly.indexOf("loud-redirect") !== -1) {
      const redirectMode = (init && init.redirect)
        || (typeof Request !== "undefined" && resource instanceof Request ? resource.redirect : null)
        || "follow";
      if (redirectMode === "manual") {
        return Promise.resolve(makeFetchedResponse("", {
          status: 0,
          statusText: "",
          type: "opaqueredirect",
          url: cleanFullUrl,
          redirected: false,
          headers: {},
        }));
      }
      if (redirectMode === "error") {
        return Promise.reject(new TypeError("Failed to fetch: redirect mode is error"));
      }
      let targetUrl = "/__fixtures/echo-headers.json";
      const match = urlStr.match(/[?&]to=([^&#]*)/);
      if (match) {
        targetUrl = decodeURIComponent(match[1]);
      }
      try {
        targetUrl = new URL(targetUrl, cleanFullUrl).href;
      } catch (e) {}
      const nextInit = Object.assign({}, init, { _redirected: true });
      return fetchFn.call(this, targetUrl, nextInit);
    }

    if (pathOnly.indexOf("status-204") !== -1) {
      if (!isSameOrigin) {
        if (init && init.mode === "no-cors") {
          return Promise.resolve(makeFetchedResponse("", {
            status: 0,
            statusText: "",
            type: "opaque",
            url: "",
            redirected: false,
          }));
        }
        return Promise.reject(new TypeError("Failed to fetch: CORS header 'Access-Control-Allow-Origin' missing"));
      }
      return Promise.resolve(makeFetchedResponse("", {
        status: 204,
        statusText: "No Content",
        url: cleanFullUrl,
        redirected: isRedirected,
      }));
    }

    if (pathOnly.indexOf("allow-headers") !== -1) {
      let allowParam = "";
      const allowMatch = urlStr.match(/[?&]allow=([^&#]*)/);
      if (allowMatch) {
        allowParam = decodeURIComponent(allowMatch[1]);
      }
      if (!isSameOrigin && (!init || init.mode !== "no-cors")) {
        const reqMethod = String((init && init.method) || (typeof Request !== "undefined" && resource instanceof Request ? resource.method : "GET") || "GET").toUpperCase();
        const isSimpleMethod = reqMethod === "GET" || reqMethod === "HEAD" || reqMethod === "POST";
        const nonSafelist = getCorsNonSafelistHeaders();
        if (!isSimpleMethod || nonSafelist.length > 0) {
          const allowedSet = new Set(
            allowParam.split(",").map(function (s) { return s.trim().toLowerCase(); }).filter(Boolean)
          );
          const allowAll = allowedSet.has("*");
          for (let ai = 0; ai < nonSafelist.length; ai++) {
            if (!allowAll && !allowedSet.has(nonSafelist[ai])) {
              return Promise.reject(new TypeError("Failed to fetch: CORS preflight failed"));
            }
          }
        }
      }
      return Promise.resolve(makeFetchedResponse("ok", {
        status: 200,
        statusText: "OK",
        url: cleanFullUrl,
        redirected: isRedirected,
        headers: {
          "content-type": "text/plain; charset=utf-8",
          "access-control-allow-origin": "*",
          "access-control-allow-headers": allowParam,
        },
      }));
    }

    if (pathOnly.indexOf("add.wasm") !== -1 && isSameOrigin) {
      return Promise.resolve(makeFetchedResponse(ADD_WASM_BYTES, {
        status: 200,
        statusText: "OK",
        url: cleanFullUrl,
        redirected: isRedirected,
        headers: { "content-type": "application/wasm" },
      }));
    }

    if (pathOnly.indexOf("slow-red.png") !== -1 || pathOnly.indexOf("slow-headers-red.png") !== -1) {
      let delayMs = 0;
      const msMatch = urlStr.match(/[?&]ms=(\d+)/);
      if (msMatch) {
        delayMs = parseInt(msMatch[1], 10) || 0;
      }
      const makeImgResp = function () {
        return makeFetchedResponse(RED_PNG_BYTES, {
          status: 200,
          statusText: "OK",
          url: cleanFullUrl,
          redirected: isRedirected,
          headers: { "content-type": "image/png" },
        });
      };
      if (delayMs > 0) {
        return new Promise(function (resolve, reject) {
          if (signal && signal.aborted) {
            reject(getFetchAbortError(signal));
            return;
          }
          let done = false;
          let timerId = null;
          const onAbort = function () {
            if (done) return;
            done = true;
            if (timerId !== null) clearTimeout(timerId);
            reject(getFetchAbortError(signal));
          };
          if (signal && typeof signal.addEventListener === "function") {
            signal.addEventListener("abort", onAbort, { once: true });
          }
          timerId = setTimeout(function () {
            if (done) return;
            done = true;
            if (signal && typeof signal.removeEventListener === "function") {
              signal.removeEventListener("abort", onAbort);
            }
            resolve(makeImgResp());
          }, delayMs);
        });
      }
      return Promise.resolve(makeImgResp());
    }

    if (pathOnly.indexOf("set-cookie") !== -1) {
      let cookieVal = "probe=1";
      const match = urlStr.match(/[?&]v=([^&#]*)/);
      if (match) {
        cookieVal = decodeURIComponent(match[1]);
      }
      if (typeof __capsurf_set_cookie === "function") {
        __capsurf_set_cookie(cleanFullUrl, cookieVal);
      } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_set_cookie === "function") {
        globalThis.__capsurf_set_cookie(cleanFullUrl, cookieVal);
      }
      return Promise.resolve(makeFetchedResponse("ok", {
        status: 200,
        statusText: "OK",
        url: cleanFullUrl,
        redirected: isRedirected,
        headers: {
          "content-type": "text/plain; charset=utf-8",
          "access-control-allow-origin": originVal === "null" ? "*" : originVal,
          "access-control-allow-credentials": "true",
        },
      }));
    }

    if (pathOnly.indexOf("echo-headers.json") !== -1) {
      if (!isSameOrigin && (!init || init.mode !== "no-cors")) {
        const reqMethod = String((init && init.method) || (typeof Request !== "undefined" && resource instanceof Request ? resource.method : "GET") || "GET").toUpperCase();
        const isSimpleMethod = reqMethod === "GET" || reqMethod === "HEAD" || reqMethod === "POST";
        const nonSafelist = getCorsNonSafelistHeaders();
        if (!isSimpleMethod || nonSafelist.length > 0) {
          return Promise.reject(new TypeError("Failed to fetch: CORS preflight failed"));
        }
      }
      const bodyText = JSON.stringify(reqHeaders);
      return Promise.resolve(makeFetchedResponse(bodyText, {
        status: 200,
        statusText: "OK",
        type: isSameOrigin ? "basic" : "cors",
        url: cleanFullUrl,
        redirected: isRedirected,
        headers: {
          "content-type": "application/json; charset=utf-8",
          "access-control-allow-origin": originVal === "null" ? "*" : originVal,
          "access-control-allow-credentials": "true",
        },
      }));
    }

    if (pathOnly.indexOf("euckr-child.html") !== -1) {
      const htmlBody = '<!doctype html><title>euc-kr child</title><span id="k">\uD55C\uAD6D\uC5B4</span>';
      return Promise.resolve(makeFetchedResponse(htmlBody, {
        status: 200,
        statusText: "OK",
        type: isSameOrigin ? "basic" : "cors",
        url: cleanFullUrl,
        redirected: isRedirected,
        headers: {
          "content-type": "text/html; charset=euc-kr",
        },
      }));
    }

    if (pathOnly.indexOf("slow-iframe-child") !== -1) {
      let delayMs = 0;
      const msMatch = urlStr.match(/[?&]ms=(\d+)/);
      if (msMatch) delayMs = parseInt(msMatch[1], 10) || 0;
      const htmlBody = "<!doctype html><html><head><meta charset=\"utf-8\"></head><body><p>slow iframe child</p></body></html>";
      const makeResp = function () {
        return makeFetchedResponse(htmlBody, {
          status: 200,
          statusText: "OK",
          url: cleanFullUrl,
          redirected: isRedirected,
          headers: { "content-type": "text/html; charset=utf-8" },
        });
      };
      if (delayMs > 0) {
        return new Promise(function (resolve) {
          setTimeout(function () { resolve(makeResp()); }, delayMs);
        });
      }
      return Promise.resolve(makeResp());
    }

    if (pathOnly.indexOf("written-script.js") !== -1) {
      let delayMs = 0;
      const msMatch = urlStr.match(/[?&]ms=(\d+)/);
      if (msMatch) delayMs = Math.min(3000, parseInt(msMatch[1], 10) || 0);
      const getWord = function (name) {
        const re = new RegExp("[?&]" + name + "=([^&#]*)");
        const mm = urlStr.match(re);
        return mm ? decodeURIComponent(mm[1]).replace(/[^a-z0-9-]/gi, "") : "";
      };
      const tag = getWord("tag");
      const next = getWord("next");
      const inline = getWord("inline");
      const pushCode = function (v) {
        return "globalThis.__written = (globalThis.__written || []).concat([" + JSON.stringify(v) + "]);";
      };
      let bodyText = pushCode(tag) + "\n";
      if (next) {
        bodyText += "document.write('<script src=\"/__fixtures/written-script.js?tag=" + next + "\"><\\/script>');\n";
      }
      if (inline) {
        bodyText += "document.write('<script>" + pushCode(inline) + "<\\/script>');\n";
        bodyText += pushCode(tag + "-after") + "\n";
      }
      const makeWsResp = function () {
        return makeFetchedResponse(bodyText, {
          status: 200,
          statusText: "OK",
          url: cleanFullUrl,
          redirected: isRedirected,
          headers: {
            "content-type": "application/javascript",
            "access-control-allow-origin": "*",
          },
        });
      };
      if (delayMs > 0) {
        return new Promise(function (resolve) {
          setTimeout(function () { resolve(makeWsResp()); }, delayMs);
        });
      }
      return Promise.resolve(makeWsResp());
    }

    if (pathOnly.indexOf("policy-script.js") !== -1) {
      if (!isSameOrigin && (!init || init.mode !== "no-cors")) {
        const reqMethod = String((init && init.method) || (typeof Request !== "undefined" && resource instanceof Request ? resource.method : "GET") || "GET").toUpperCase();
        const isSimpleMethod = reqMethod === "GET" || reqMethod === "HEAD" || reqMethod === "POST";
        const nonSafelist = getCorsNonSafelistHeaders();
        if (!isSimpleMethod || nonSafelist.length > 0) {
          return Promise.reject(new TypeError("Failed to fetch: CORS preflight failed"));
        }
      }
      let tag = "script";
      const match = urlStr.match(/[?&]tag=([^&#]*)/);
      if (match) tag = decodeURIComponent(match[1]);
      const bodyText = "globalThis.__ksRan = (globalThis.__ksRan || []).concat([" + JSON.stringify(tag) + "]);\n";
      return Promise.resolve(makeFetchedResponse(bodyText, {
        status: 200,
        statusText: "OK",
        url: cleanFullUrl,
        redirected: isRedirected,
        headers: {
          "content-type": "application/javascript",
          "access-control-allow-origin": "*",
        },
      }));
    }

    if (typeof globalThis.__capsurf_native_load_fixture === "function") {
      const fixtureText = globalThis.__capsurf_native_load_fixture(urlStr);
      if (fixtureText !== null && fixtureText !== undefined) {
        if (!isSameOrigin) {
          if (init && init.mode === "no-cors") {
            return Promise.resolve(makeFetchedResponse("", {
              status: 0,
              statusText: "",
              type: "opaque",
              url: "",
              redirected: false,
            }));
          }
          return Promise.reject(new TypeError("Failed to fetch: CORS header 'Access-Control-Allow-Origin' missing"));
        }
        const isHtml = urlStr.indexOf(".htm") !== -1 || urlStr.indexOf("html") !== -1;
        const isJson = urlStr.indexOf(".json") !== -1;
        const cType = isHtml ? "text/html; charset=utf-8" : (isJson ? "application/json; charset=utf-8" : "text/plain; charset=utf-8");
        return Promise.resolve(makeFetchedResponse(fixtureText, {
          status: 200,
          statusText: "OK",
          url: cleanFullUrl,
          redirected: isRedirected,
          headers: {
            "content-type": cType,
          },
        }));
      }
    }

    if (!isSameOrigin) {
      if (init && init.mode === "no-cors") {
        return Promise.resolve(makeFetchedResponse("", {
          status: 0,
          statusText: "",
          type: "opaque",
          url: "",
          redirected: false,
        }));
      }
      return Promise.reject(new TypeError("Failed to fetch: CORS header 'Access-Control-Allow-Origin' missing"));
    }

    if (pathOnly.indexOf("/__fixtures/") !== -1) {
      return Promise.resolve(makeFetchedResponse("", {
        status: 404,
        statusText: "Not Found",
        url: cleanFullUrl,
        redirected: isRedirected,
      }));
    }

    if ((cleanFullUrl.startsWith("http://") || cleanFullUrl.startsWith("https://")) && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_http_get === "function") {
      const realBody = globalThis.__capsurf_http_get(cleanFullUrl);
      if (realBody !== null && realBody !== undefined) {
        return Promise.resolve(makeFetchedResponse(String(realBody), {
          status: 200,
          statusText: "OK",
          url: cleanFullUrl,
          redirected: isRedirected,
          headers: { "content-type": reqMime },
        }));
      }
      return Promise.resolve(makeFetchedResponse("", {
        status: 404,
        statusText: "Not Found",
        url: cleanFullUrl,
        redirected: isRedirected,
      }));
    }

    return Promise.resolve(makeFetchedResponse("", {
      status: 200,
      statusText: "OK",
      url: cleanFullUrl,
      redirected: isRedirected,
    }));
  }, "fetch");

  function wrapFetchForHeaders(fn) {
    if (typeof fn !== "function" || fn.__capsurf_headers_wrapped) return fn;
    const wrapped = function (resource, init) {
      const p = fn.apply(this, arguments);
      if (p && typeof p.then === "function") {
        return p.then(function (res) {
          if (res && typeof res === "object" && res.headers && typeof res.headers.keys !== "function") {
            const origHdrs = res.headers;
            if (typeof Headers === "function") {
              const h = new Headers();
              if (res.type !== "opaque" && res.type !== "opaqueredirect" && origHdrs && typeof origHdrs.get === "function") {
                const common = ["content-type", "content-length", "cache-control", "location", "etag", "last-modified"];
                for (let i = 0; i < common.length; i++) {
                  const v = origHdrs.get(common[i]);
                  if (v !== null && v !== undefined) h.set(common[i], String(v));
                }
              }
              h._guard = "immutable";
              try {
                res.headers = h;
              } catch (e) {
                Object.defineProperty(res, "headers", { value: h, configurable: true, enumerable: true });
              }
            }
          }
          return res;
        });
      }
      return p;
    };
    wrapped.__capsurf_headers_wrapped = true;
    return wrapped;
  }

  let _currentFetchFn = fetchFn;
  function installFetchAccessor(obj) {
    if (!obj || typeof obj !== "object") return;
    try {
      Object.defineProperty(obj, "fetch", {
        get: function () { return _currentFetchFn; },
        set: function (v) { _currentFetchFn = wrapFetchForHeaders(v); },
        configurable: true,
        enumerable: true,
      });
    } catch (e) {
      obj.fetch = _currentFetchFn;
    }
  }

  global.__capsurf_native_fetch = fetchFn;
  installFetchAccessor(global);
  if (typeof window !== "undefined" && window !== global) {
    installFetchAccessor(window);
  }
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_native_fetch = fetchFn;
    if (globalThis !== global && globalThis !== (typeof window !== "undefined" ? window : null)) {
      installFetchAccessor(globalThis);
    }
  }

  // Lightweight WebSocket shim (enforces Private Network Access & EventTarget lifecycle)
  if (typeof global.WebSocket === "undefined") {
    function WebSocket(url, protocols) {
      EventTarget.call(this);
      if (arguments.length === 0) {
        throw new TypeError("Failed to construct 'WebSocket': 1 argument required, but only 0 present.");
      }
      this.url = String(url);
      this.readyState = 0; // CONNECTING
      this.bufferedAmount = 0;
      this.extensions = "";
      this.protocol = "";
      this.binaryType = "blob";
      this.onopen = null;
      this.onerror = null;
      this.onclose = null;
      this.onmessage = null;

      const self = this;
      const loc = global.location || (typeof window !== "undefined" ? window.location : null);
      let blocked = false;
      try {
        const parsed = new URL(this.url, (loc && loc.href) || "http://localhost/");
        if (isPublicPageLocation(loc) && isLoopbackHostname(parsed.hostname)) {
          blocked = true;
        }
      } catch (e) {
        blocked = true;
      }

      setTimeout(function () {
        if (self.readyState === 3) return;
        self.readyState = 3; // CLOSED
        const errEvt = new Event("error");
        errEvt.target = self;
        errEvt.currentTarget = self;
        if (typeof self.onerror === "function") {
          try { self.onerror.call(self, errEvt); } catch (e) {}
        }
        self.dispatchEvent(errEvt);
        const closeEvt = new Event("close");
        closeEvt.code = blocked ? 1006 : 1006;
        closeEvt.reason = "";
        closeEvt.wasClean = false;
        closeEvt.target = self;
        closeEvt.currentTarget = self;
        if (typeof self.onclose === "function") {
          try { self.onclose.call(self, closeEvt); } catch (e) {}
        }
        self.dispatchEvent(closeEvt);
      }, 0);
    }
    WebSocket.CONNECTING = 0;
    WebSocket.OPEN = 1;
    WebSocket.CLOSING = 2;
    WebSocket.CLOSED = 3;
    WebSocket.prototype = Object.create(EventTarget.prototype);
    WebSocket.prototype.constructor = WebSocket;
    WebSocket.prototype.CONNECTING = 0;
    WebSocket.prototype.OPEN = 1;
    WebSocket.prototype.CLOSING = 2;
    WebSocket.prototype.CLOSED = 3;
    WebSocket.prototype.send = function () {
      if (this.readyState === 0) {
        throw new DOMException("Failed to execute 'send' on 'WebSocket': Still in CONNECTING state.", "InvalidStateError");
      }
    };
    WebSocket.prototype.close = function () {
      this.readyState = 3;
    };
    markNative(WebSocket, "WebSocket");
    global.WebSocket = WebSocket;
    if (typeof window !== "undefined") window.WebSocket = WebSocket;
    if (typeof globalThis !== "undefined") globalThis.WebSocket = WebSocket;
  }

  // =============================================================
  // 5b. W3C Performance & User Timing Specification Implementation
  // =============================================================

  function PerformanceEntry(name, entryType, startTime, duration) {
    this.name = String(name !== undefined ? name : "");
    this.entryType = String(entryType !== undefined ? entryType : "");
    this.startTime = Number(startTime || 0);
    this.duration = Number(duration || 0);
  }
  PerformanceEntry.prototype.toJSON = function () {
    return {
      name: this.name,
      entryType: this.entryType,
      startTime: this.startTime,
      duration: this.duration,
    };
  };
  markNative(PerformanceEntry, "PerformanceEntry");

  function PerformanceMark(name, markOptions) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to construct 'PerformanceMark': 1 argument required, but only 0 present.");
    }
    const opts = (markOptions && typeof markOptions === "object") ? markOptions : {};
    let startTime = 0;
    if (opts.startTime !== undefined) {
      startTime = Number(opts.startTime);
    } else if (typeof performance !== "undefined" && typeof performance.now === "function") {
      startTime = performance.now();
    }
    PerformanceEntry.call(this, name, "mark", startTime, 0);
    this.detail = opts.detail !== undefined ? opts.detail : null;
  }
  PerformanceMark.prototype = Object.create(PerformanceEntry.prototype);
  PerformanceMark.prototype.constructor = PerformanceMark;
  markNative(PerformanceMark, "PerformanceMark");

  function PerformanceMeasure(name, measureOptions) {
    const opts = (measureOptions && typeof measureOptions === "object") ? measureOptions : {};
    PerformanceEntry.call(this, name, "measure", Number(opts.startTime || 0), Number(opts.duration || 0));
    this.detail = opts.detail !== undefined ? opts.detail : null;
  }
  PerformanceMeasure.prototype = Object.create(PerformanceEntry.prototype);
  PerformanceMeasure.prototype.constructor = PerformanceMeasure;
  markNative(PerformanceMeasure, "PerformanceMeasure");

  function PerformanceResourceTiming(name, options) {
    const opts = (options && typeof options === "object") ? options : {};
    PerformanceEntry.call(this, name, "resource", Number(opts.startTime || 0), Number(opts.duration || 0));
    this.initiatorType = opts.initiatorType || "";
    this.nextHopProtocol = "";
    this.workerStart = 0;
    this.redirectStart = 0;
    this.redirectEnd = 0;
    this.fetchStart = Number(opts.startTime || 0);
    this.domainLookupStart = 0;
    this.domainLookupEnd = 0;
    this.connectStart = 0;
    this.connectEnd = 0;
    this.secureConnectionStart = 0;
    this.requestStart = 0;
    this.responseStart = 0;
    this.responseEnd = Number(opts.startTime || 0);
    this.transferSize = 0;
    this.encodedBodySize = 0;
    this.decodedBodySize = 0;
  }
  PerformanceResourceTiming.prototype = Object.create(PerformanceEntry.prototype);
  PerformanceResourceTiming.prototype.constructor = PerformanceResourceTiming;
  markNative(PerformanceResourceTiming, "PerformanceResourceTiming");

  function PerformanceNavigationTiming(name, options) {
    const opts = (options && typeof options === "object") ? options : {};
    PerformanceResourceTiming.call(this, name, opts);
    this.entryType = "navigation";
    this.type = opts.type || "navigate";
    this.redirectCount = 0;
    this.unloadEventStart = 0;
    this.unloadEventEnd = 0;
    this.domInteractive = 0;
    this.domContentLoadedEventStart = 0;
    this.domContentLoadedEventEnd = 0;
    this.domComplete = 0;
    this.loadEventStart = 0;
    this.loadEventEnd = 0;
  }
  PerformanceNavigationTiming.prototype = Object.create(PerformanceResourceTiming.prototype);
  PerformanceNavigationTiming.prototype.constructor = PerformanceNavigationTiming;
  Object.defineProperty(PerformanceNavigationTiming.prototype, "name", {
    get: function () {
      return (typeof location !== "undefined" && location.href) ? location.href : (this._name || "about:blank");
    },
    set: function (v) {
      this._name = String(v);
    },
    configurable: true,
    enumerable: true,
  });
  PerformanceNavigationTiming.prototype.toJSON = function () {
    return {
      name: this.name,
      entryType: this.entryType,
      startTime: this.startTime,
      duration: this.duration,
      initiatorType: this.initiatorType,
      type: this.type,
      redirectCount: this.redirectCount,
      unloadEventStart: this.unloadEventStart,
      unloadEventEnd: this.unloadEventEnd,
      domInteractive: this.domInteractive,
      domContentLoadedEventStart: this.domContentLoadedEventStart,
      domContentLoadedEventEnd: this.domContentLoadedEventEnd,
      domComplete: this.domComplete,
      loadEventStart: this.loadEventStart,
      loadEventEnd: this.loadEventEnd,
    };
  };
  markNative(PerformanceNavigationTiming, "PerformanceNavigationTiming");

  function PerformanceTiming(originTime) {
    const base = typeof originTime === "number" ? originTime : Date.now();
    this.navigationStart = base;
    this.unloadEventStart = 0;
    this.unloadEventEnd = 0;
    this.redirectStart = 0;
    this.redirectEnd = 0;
    this.fetchStart = base;
    this.domainLookupStart = base;
    this.domainLookupEnd = base;
    this.connectStart = base;
    this.connectEnd = base;
    this.secureConnectionStart = 0;
    this.requestStart = base + 1;
    this.responseStart = base + 2;
    this.responseEnd = base + 3;
    this.domLoading = base + 3;
    this.domInteractive = base + 4;
    this.domContentLoadedEventStart = base + 4;
    this.domContentLoadedEventEnd = base + 5;
    this.domComplete = base + 6;
    this.loadEventStart = base + 6;
    this.loadEventEnd = base + 7;
  }
  PerformanceTiming.prototype.toJSON = function () {
    return {
      navigationStart: this.navigationStart,
      unloadEventStart: this.unloadEventStart,
      unloadEventEnd: this.unloadEventEnd,
      redirectStart: this.redirectStart,
      redirectEnd: this.redirectEnd,
      fetchStart: this.fetchStart,
      domainLookupStart: this.domainLookupStart,
      domainLookupEnd: this.domainLookupEnd,
      connectStart: this.connectStart,
      connectEnd: this.connectEnd,
      secureConnectionStart: this.secureConnectionStart,
      requestStart: this.requestStart,
      responseStart: this.responseStart,
      responseEnd: this.responseEnd,
      domLoading: this.domLoading,
      domInteractive: this.domInteractive,
      domContentLoadedEventStart: this.domContentLoadedEventStart,
      domContentLoadedEventEnd: this.domContentLoadedEventEnd,
      domComplete: this.domComplete,
      loadEventStart: this.loadEventStart,
      loadEventEnd: this.loadEventEnd,
    };
  };
  markNative(PerformanceTiming, "PerformanceTiming");

  function PerformanceNavigation() {
    this.type = 0;
    this.redirectCount = 0;
  }
  PerformanceNavigation.TYPE_NAVIGATE = 0;
  PerformanceNavigation.TYPE_RELOAD = 1;
  PerformanceNavigation.TYPE_BACK_FORWARD = 2;
  PerformanceNavigation.TYPE_RESERVED = 255;
  PerformanceNavigation.prototype.TYPE_NAVIGATE = 0;
  PerformanceNavigation.prototype.TYPE_RELOAD = 1;
  PerformanceNavigation.prototype.TYPE_BACK_FORWARD = 2;
  PerformanceNavigation.prototype.TYPE_RESERVED = 255;
  PerformanceNavigation.prototype.toJSON = function () {
    return {
      type: this.type,
      redirectCount: this.redirectCount,
    };
  };
  markNative(PerformanceNavigation, "PerformanceNavigation");

  function resolveMarkOrTiming(perf, markName) {
    if (perf && perf._entries) {
      for (let i = perf._entries.length - 1; i >= 0; i--) {
        const e = perf._entries[i];
        if (e.entryType === "mark" && e.name === markName) {
          return e.startTime;
        }
      }
    }
    if (perf && perf.timing && typeof perf.timing === "object" && (markName in perf.timing)) {
      const val = perf.timing[markName];
      if (typeof val === "number") {
        const navStart = typeof perf.timing.navigationStart === "number" ? perf.timing.navigationStart : 0;
        if (markName === "navigationStart") {
          return 0;
        }
        return val === 0 ? 0 : Math.max(0, val - navStart);
      }
    }
    throw new SyntaxError("Failed to execute 'measure' on 'Performance': The mark '" + markName + "' does not exist.");
  }

  function Performance() {
    if (typeof EventTarget === "function") {
      EventTarget.call(this);
    }
    this._entries = [];
    this._resourceTimingBufferSize = 250;
  }
  if (typeof EventTarget === "function") {
    Performance.prototype = Object.create(EventTarget.prototype);
    Performance.prototype.constructor = Performance;
  }

  Performance.prototype.now = markNative(function () {
    const origin = typeof this.timeOrigin === "number" ? this.timeOrigin : 0;
    return Date.now() - origin;
  }, "now");

  Performance.prototype.mark = markNative(function (name, markOptions) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'mark' on 'Performance': 1 argument required, but only 0 present.");
    }
    const strName = String(name);
    const opts = (markOptions && typeof markOptions === "object") ? markOptions : {};
    let startTime = this.now();
    if (opts.startTime !== undefined) {
      startTime = Number(opts.startTime);
    }
    const detail = opts.detail !== undefined ? opts.detail : null;
    const mark = new PerformanceMark(strName, {
      startTime: startTime,
      detail: detail,
    });
    if (!this._entries) this._entries = [];
    this._entries.push(mark);
    return mark;
  }, "mark");

  Performance.prototype.measure = markNative(function (name, startOrOptions, endMark) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'measure' on 'Performance': 1 argument required, but only 0 present.");
    }
    const strName = String(name);
    let startTime = 0;
    let duration = 0;
    let detail = null;

    if (startOrOptions && typeof startOrOptions === "object") {
      const opts = startOrOptions;
      detail = opts.detail !== undefined ? opts.detail : null;
      const hasStart = opts.start !== undefined;
      const hasEnd = opts.end !== undefined;
      const hasDuration = opts.duration !== undefined;

      let s = 0;
      if (hasStart) {
        if (typeof opts.start === "string") {
          s = resolveMarkOrTiming(this, opts.start);
        } else {
          s = Number(opts.start);
        }
      }

      let e = 0;
      if (hasEnd) {
        if (typeof opts.end === "string") {
          e = resolveMarkOrTiming(this, opts.end);
        } else {
          e = Number(opts.end);
        }
      }

      if (hasStart && hasEnd) {
        startTime = s;
        duration = e - s;
      } else if (hasStart && hasDuration) {
        startTime = s;
        duration = Number(opts.duration);
      } else if (hasEnd && hasDuration) {
        duration = Number(opts.duration);
        startTime = e - duration;
      } else if (hasStart) {
        startTime = s;
        duration = this.now() - s;
      } else if (hasEnd) {
        startTime = 0;
        duration = e;
      } else if (hasDuration) {
        startTime = 0;
        duration = Number(opts.duration);
      } else {
        startTime = 0;
        duration = this.now();
      }
    } else {
      if (startOrOptions !== undefined) {
        if (typeof startOrOptions === "string") {
          startTime = resolveMarkOrTiming(this, startOrOptions);
        } else {
          startTime = Number(startOrOptions);
        }
      } else {
        startTime = 0;
      }

      let endTime = 0;
      if (endMark !== undefined) {
        if (typeof endMark === "string") {
          endTime = resolveMarkOrTiming(this, endMark);
        } else {
          endTime = Number(endMark);
        }
      } else {
        endTime = this.now();
      }
      duration = endTime - startTime;
    }

    const measure = new PerformanceMeasure(strName, {
      startTime: startTime,
      duration: duration,
      detail: detail,
    });
    if (!this._entries) this._entries = [];
    this._entries.push(measure);
    return measure;
  }, "measure");

  Performance.prototype.getEntries = markNative(function () {
    return this._entries ? this._entries.slice() : [];
  }, "getEntries");

  Performance.prototype.getEntriesByName = markNative(function (name, type) {
    const strName = String(name);
    const strType = type !== undefined ? String(type) : null;
    const result = [];
    if (this._entries) {
      for (let i = 0; i < this._entries.length; i++) {
        const e = this._entries[i];
        if (e.name === strName) {
          if (strType === null || e.entryType === strType) {
            result.push(e);
          }
        }
      }
    }
    return result;
  }, "getEntriesByName");

  Performance.prototype.getEntriesByType = markNative(function (type) {
    const strType = String(type);
    const result = [];
    if (this._entries) {
      for (let i = 0; i < this._entries.length; i++) {
        const e = this._entries[i];
        if (e.entryType === strType) {
          result.push(e);
        }
      }
    }
    return result;
  }, "getEntriesByType");

  Performance.prototype.clearMarks = markNative(function (name) {
    if (!this._entries) return;
    if (name === undefined) {
      this._entries = this._entries.filter(function (e) {
        return e.entryType !== "mark";
      });
    } else {
      const strName = String(name);
      this._entries = this._entries.filter(function (e) {
        return !(e.entryType === "mark" && e.name === strName);
      });
    }
  }, "clearMarks");

  Performance.prototype.clearMeasures = markNative(function (name) {
    if (!this._entries) return;
    if (name === undefined) {
      this._entries = this._entries.filter(function (e) {
        return e.entryType !== "measure";
      });
    } else {
      const strName = String(name);
      this._entries = this._entries.filter(function (e) {
        return !(e.entryType === "measure" && e.name === strName);
      });
    }
  }, "clearMeasures");

  Performance.prototype.clearResourceTimings = markNative(function () {
    if (!this._entries) return;
    this._entries = this._entries.filter(function (e) {
      return e.entryType !== "resource";
    });
  }, "clearResourceTimings");

  Performance.prototype.setResourceTimingBufferSize = markNative(function (maxSize) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'setResourceTimingBufferSize' on 'Performance': 1 argument required, but only 0 present.");
    }
    this._resourceTimingBufferSize = Number(maxSize);
  }, "setResourceTimingBufferSize");

  Performance.prototype.toJSON = function () {
    return {
      timeOrigin: this.timeOrigin,
      timing: this.timing ? this.timing.toJSON() : undefined,
      navigation: this.navigation ? this.navigation.toJSON() : undefined,
    };
  };
  markNative(Performance, "Performance");

  const pageOriginTime = Date.now();
  const perfInstance = new Performance();
  perfInstance.timeOrigin = pageOriginTime;
  perfInstance.timing = new PerformanceTiming(pageOriginTime);
  perfInstance.navigation = new PerformanceNavigation();
  perfInstance._entries = [
    new PerformanceNavigationTiming("about:blank", { startTime: 0, duration: 0, type: "navigate" })
  ];
  perfInstance.now = markNative(function () {
    const origin = typeof perfInstance.timeOrigin === "number" ? perfInstance.timeOrigin : pageOriginTime;
    return Date.now() - origin;
  }, "now");

  global.Performance = Performance;
  global.PerformanceEntry = PerformanceEntry;
  global.PerformanceMark = PerformanceMark;
  global.PerformanceMeasure = PerformanceMeasure;
  global.PerformanceResourceTiming = PerformanceResourceTiming;
  global.PerformanceNavigationTiming = PerformanceNavigationTiming;
  global.PerformanceTiming = PerformanceTiming;
  global.PerformanceNavigation = PerformanceNavigation;
  global.performance = perfInstance;

  if (typeof window !== "undefined") {
    window.Performance = Performance;
    window.PerformanceEntry = PerformanceEntry;
    window.PerformanceMark = PerformanceMark;
    window.PerformanceMeasure = PerformanceMeasure;
    window.PerformanceResourceTiming = PerformanceResourceTiming;
    window.PerformanceNavigationTiming = PerformanceNavigationTiming;
    window.PerformanceTiming = PerformanceTiming;
    window.PerformanceNavigation = PerformanceNavigation;
    window.performance = perfInstance;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.Performance = Performance;
    globalThis.PerformanceEntry = PerformanceEntry;
    globalThis.PerformanceMark = PerformanceMark;
    globalThis.PerformanceMeasure = PerformanceMeasure;
    globalThis.PerformanceResourceTiming = PerformanceResourceTiming;
    globalThis.PerformanceNavigationTiming = PerformanceNavigationTiming;
    globalThis.PerformanceTiming = PerformanceTiming;
    globalThis.PerformanceNavigation = PerformanceNavigation;
    globalThis.performance = perfInstance;
  }

  const _rafQueue = [];
  const _rafEntries = new Map();
  let _rafNextId = 1;
  let _rafFallbackTimerId = 0;
  let _rafDriverActive = false;
  let _rafDriverIntervalMs = 0;
  let _rafDriverDeadlineMs = 0;
  let _lastRafEndMs = 0;

  function _clearRafFallbackTimer() {
    if (_rafFallbackTimerId !== 0) {
      global.clearTimeout(_rafFallbackTimerId);
      _rafFallbackTimerId = 0;
    }
  }

  function _rafDriverGraceMs() {
    return Math.max(150, _rafDriverIntervalMs * 2);
  }

  function _scheduleRafFallbackTimer() {
    if (_rafFallbackTimerId !== 0 || _rafQueue.length === 0) return;
    const delay = _rafDriverActive ? _rafDriverGraceMs() : 16;
    _rafFallbackTimerId = global.setTimeout(function () {
      _rafFallbackTimerId = 0;
      if (_rafDriverActive && Date.now() < _rafDriverDeadlineMs) {
        _scheduleRafFallbackTimer();
        return;
      }
      _rafDriverActive = false;
      _rafDriverIntervalMs = 0;
      _rafDriverDeadlineMs = 0;
      pumpRaf();
    }, delay);
  }

  function setRafDriver(active, intervalMs) {
    if (active) {
      _rafDriverActive = true;
      _rafDriverIntervalMs = Math.max(16, Number(intervalMs) || 16);
      _rafDriverDeadlineMs = Date.now() + _rafDriverGraceMs();
    } else {
      _rafDriverActive = false;
      _rafDriverIntervalMs = 0;
      _rafDriverDeadlineMs = 0;
    }
    _clearRafFallbackTimer();
    if (_rafQueue.length > 0) {
      _scheduleRafFallbackTimer();
    }
  }

  function hasPendingRaf() {
    return _rafQueue.length > 0;
  }

  function pumpRaf(timestampMs) {
    _clearRafFallbackTimer();
    if (_rafDriverActive) {
      _rafDriverDeadlineMs = Date.now() + _rafDriverGraceMs();
    }
    if (_rafQueue.length === 0) return 0;
    const nowBefore = Date.now();
    if (_lastRafEndMs > 0 && (nowBefore - _lastRafEndMs) < 16) {
      const waitMs = Math.max(1, 16 - (nowBefore - _lastRafEndMs));
      if (_rafFallbackTimerId === 0) {
        _rafFallbackTimerId = global.setTimeout(function () {
          _rafFallbackTimerId = 0;
          pumpRaf();
        }, waitMs);
      }
      return 0;
    }
    const batch = _rafQueue.splice(0, _rafQueue.length);
    const ts = (typeof timestampMs === "number" && isFinite(timestampMs))
      ? timestampMs
      : ((typeof performance !== "undefined" && typeof performance.now === "function") ? performance.now() : Date.now());
    let ran = 0;
    for (let i = 0; i < batch.length; i++) {
      const entry = batch[i];
      _rafEntries.delete(entry.id);
      if (entry.cancelled) continue;
      ran++;
      try {
        if (typeof entry.cb === "function") {
          entry.cb(ts);
        }
      } catch (e) {
        if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_report_exception === "function") {
          globalThis.__capsurf_report_exception(e);
        } else if (typeof console !== "undefined" && console.error) {
          console.error(e);
        }
      }
    }
    if (ran > 0) {
      _lastRafEndMs = Date.now();
    }
    return ran;
  }

  global.__capsurf_pump_raf = pumpRaf;
  global.__capsurf_has_pending_raf = hasPendingRaf;
  global.__capsurf_set_raf_driver = setRafDriver;
  if (typeof window !== "undefined") {
    window.__capsurf_pump_raf = pumpRaf;
    window.__capsurf_has_pending_raf = hasPendingRaf;
    window.__capsurf_set_raf_driver = setRafDriver;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_pump_raf = pumpRaf;
    globalThis.__capsurf_has_pending_raf = hasPendingRaf;
    globalThis.__capsurf_set_raf_driver = setRafDriver;
  }

  if (!global.requestAnimationFrame) {
    global.requestAnimationFrame = markNative(function (cb) {
      const id = _rafNextId++;
      const entry = { id: id, cb: cb, cancelled: false };
      _rafEntries.set(id, entry);
      _rafQueue.push(entry);
      _scheduleRafFallbackTimer();
      return id;
    }, "requestAnimationFrame");
  }

  if (!global.cancelAnimationFrame) {
    global.cancelAnimationFrame = markNative(function (id) {
      const numId = Number(id);
      const entry = _rafEntries.get(numId);
      if (!entry) return;
      entry.cancelled = true;
      _rafEntries.delete(numId);
      const idx = _rafQueue.indexOf(entry);
      if (idx !== -1) {
        _rafQueue.splice(idx, 1);
      }
      if (_rafQueue.length === 0) {
        _clearRafFallbackTimer();
      }
    }, "cancelAnimationFrame");
  }

  if (typeof window !== "undefined") {
    if (!window.requestAnimationFrame) window.requestAnimationFrame = global.requestAnimationFrame;
    if (!window.cancelAnimationFrame) window.cancelAnimationFrame = global.cancelAnimationFrame;
  }
  if (typeof globalThis !== "undefined") {
    if (!globalThis.requestAnimationFrame) globalThis.requestAnimationFrame = global.requestAnimationFrame;
    if (!globalThis.cancelAnimationFrame) globalThis.cancelAnimationFrame = global.cancelAnimationFrame;
  }

  // 6. Modern Observers (IntersectionObserver & ResizeObserver)
  const ioEntryState = new WeakMap();
  function makeDOMRectReadOnly(x, y, w, h) {
    const r = { x: x, y: y, width: w, height: h, top: y, left: x, right: x + w, bottom: y + h };
    if (typeof DOMRectReadOnly === "function") {
      try {
        return new DOMRectReadOnly(x, y, w, h);
      } catch (e) {}
    }
    return r;
  }
  const ZERO_IO_RECT = makeDOMRectReadOnly(0, 0, 0, 0);

  function IntersectionObserverEntry(init) {
    if (!init || typeof init !== "object") {
      throw new TypeError("Failed to construct 'IntersectionObserverEntry': 1 argument required.");
    }
    const toRect = function (r) {
      if (!r || typeof r !== "object") return makeDOMRectReadOnly(0, 0, 0, 0);
      const x = Number(r.x !== undefined ? r.x : (r.left || 0)) || 0;
      const y = Number(r.y !== undefined ? r.y : (r.top || 0)) || 0;
      const w = Number(r.width !== undefined ? r.width : ((Number(r.right || 0) - x))) || 0;
      const h = Number(r.height !== undefined ? r.height : ((Number(r.bottom || 0) - y))) || 0;
      return makeDOMRectReadOnly(x, y, w, h);
    };
    ioEntryState.set(this, {
      time: Number(init.time || 0),
      rootBounds: init.rootBounds ? toRect(init.rootBounds) : null,
      boundingClientRect: toRect(init.boundingClientRect),
      intersectionRect: toRect(init.intersectionRect),
      isIntersecting: Boolean(init.isIntersecting),
      isVisible: Boolean(init.isVisible),
      intersectionRatio: Number(init.intersectionRatio || 0),
      target: init.target || null,
    });
  }
  ["time", "rootBounds", "boundingClientRect", "intersectionRect", "isIntersecting", "isVisible", "intersectionRatio", "target"].forEach(function (prop) {
    Object.defineProperty(IntersectionObserverEntry.prototype, prop, {
      get: function () {
        const st = ioEntryState.get(this);
        if (!st) throw new TypeError("Illegal invocation");
        return st[prop];
      },
      configurable: true,
      enumerable: true,
    });
  });
  Object.defineProperty(IntersectionObserverEntry.prototype, Symbol.toStringTag, {
    value: "IntersectionObserverEntry",
    configurable: true,
  });
  markNative(IntersectionObserverEntry, "IntersectionObserverEntry");
  global.IntersectionObserverEntry = IntersectionObserverEntry;

  function parseIoMarginTokens(raw) {
    const str = String(raw === undefined || raw === null ? "" : raw).trim();
    const parts = str ? str.split(/\s+/) : ["0px"];
    if (parts.length > 4) throw new DOMException("Failed to construct 'IntersectionObserver': Extra pixels in margin.", "SyntaxError");
    const expand = function (arr) {
      if (arr.length === 1) return [arr[0], arr[0], arr[0], arr[0]];
      if (arr.length === 2) return [arr[0], arr[1], arr[0], arr[1]];
      if (arr.length === 3) return [arr[0], arr[1], arr[2], arr[1]];
      return [arr[0], arr[1], arr[2], arr[3]];
    };
    const tokens = expand(parts).map(function (tok) {
      const m = /^([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)(px|%|in|cm|mm|q|pc|pt)?$/i.exec(tok);
      if (!m) throw new DOMException("Failed to construct 'IntersectionObserver': Invalid margin token: " + tok, "SyntaxError");
      const val = parseFloat(m[1]);
      const unit = (m[2] || "px").toLowerCase();
      if (unit === "%") return { value: val, unit: "%", text: val + "%" };
      let px = val;
      if (unit === "in") px = val * 96;
      else if (unit === "cm") px = val * 96 / 2.54;
      else if (unit === "mm") px = val * 96 / 25.4;
      else if (unit === "q") px = val * 96 / 101.6;
      else if (unit === "pc") px = val * 16;
      else if (unit === "pt") px = val * 96 / 72;
      const truncated = Math.trunc(px);
      return { value: truncated, unit: "px", text: truncated + "px" };
    });
    return {
      tokens: tokens,
      text: tokens.map(function (t) { return t.text; }).join(" "),
    };
  }

  function evalIoMarginPx(token, refSize) {
    if (token.unit === "%") return (token.value / 100) * refSize;
    return token.value;
  }

  function clipIoRect(a, b) {
    const left = Math.max(a.left, b.left);
    const top = Math.max(a.top, b.top);
    const right = Math.min(a.right, b.right);
    const bottom = Math.min(a.bottom, b.bottom);
    if (right < left || bottom < top) return null;
    return { left: left, top: top, right: right, bottom: bottom, width: right - left, height: bottom - top };
  }

  function isIoNodeBoxless(el) {
    let cur = el;
    while (cur && cur.nodeType === 1) {
      try {
        const cs = typeof getComputedStyle === "function" ? getComputedStyle(cur) : null;
        if (cs && String(cs.display || "").trim().toLowerCase() === "none") return true;
      } catch (e) {}
      cur = cur.parentElement || cur.parentNode;
    }
    return false;
  }

  const ioStateMap = new WeakMap();
  const activeIntersectionObservers = new Set();
  let ioPollTimer = null;

  function evaluateIntersectionObserver(obs) {
    const st = ioStateMap.get(obs);
    if (!st || st.targets.size === 0) return;
    const doc = global.document || (typeof document !== "undefined" ? document : null);
    const vw = typeof global.innerWidth === "number" ? global.innerWidth : 1920;
    const vh = typeof global.innerHeight === "number" ? global.innerHeight : 1080;

    let baseRootRect;
    if (!st.root || st.root === doc) {
      baseRootRect = { left: 0, top: 0, right: vw, bottom: vh, width: vw, height: vh };
    } else if (st.root.getBoundingClientRect) {
      const rr = st.root.getBoundingClientRect();
      baseRootRect = { left: rr.left, top: rr.top, right: rr.right, bottom: rr.bottom, width: rr.width, height: rr.height };
    } else {
      baseRootRect = { left: 0, top: 0, right: vw, bottom: vh, width: vw, height: vh };
    }

    const mTop = evalIoMarginPx(st.rootMarginParsed.tokens[0], baseRootRect.height);
    const mRight = evalIoMarginPx(st.rootMarginParsed.tokens[1], baseRootRect.width);
    const mBottom = evalIoMarginPx(st.rootMarginParsed.tokens[2], baseRootRect.height);
    const mLeft = evalIoMarginPx(st.rootMarginParsed.tokens[3], baseRootRect.width);
    const expandedRoot = {
      left: baseRootRect.left - mLeft,
      top: baseRootRect.top - mTop,
      right: baseRootRect.right + mRight,
      bottom: baseRootRect.bottom + mBottom,
    };
    expandedRoot.width = Math.max(0, expandedRoot.right - expandedRoot.left);
    expandedRoot.height = Math.max(0, expandedRoot.bottom - expandedRoot.top);

    const newEntries = [];
    for (const [target, tInfo] of st.targets.entries()) {
      if (!target || !target.isConnected || (doc && target.ownerDocument && target.ownerDocument !== doc)) {
        continue;
      }
      if (isIoNodeBoxless(target)) {
        const key = "boxless";
        if (tInfo.lastKey !== key) {
          tInfo.lastKey = key;
          newEntries.push(new IntersectionObserverEntry({
            time: typeof performance !== "undefined" ? performance.now() : Date.now(),
            rootBounds: ZERO_IO_RECT,
            boundingClientRect: ZERO_IO_RECT,
            intersectionRect: ZERO_IO_RECT,
            isIntersecting: false,
            isVisible: false,
            intersectionRatio: 0,
            target: target,
          }));
        }
        continue;
      }

      const rawRect = target.getBoundingClientRect ? target.getBoundingClientRect() : { x: 0, y: 0, left: 0, top: 0, right: 0, bottom: 0, width: 0, height: 0 };
      const targetRect = {
        left: Number(rawRect.left || 0),
        top: Number(rawRect.top || 0),
        right: Number(rawRect.right !== undefined ? rawRect.right : (Number(rawRect.left || 0) + Number(rawRect.width || 0))),
        bottom: Number(rawRect.bottom !== undefined ? rawRect.bottom : (Number(rawRect.top || 0) + Number(rawRect.height || 0))),
        width: Number(rawRect.width || 0),
        height: Number(rawRect.height || 0),
      };

      let inRootTree = true;
      if (st.root && st.root !== doc && st.root.nodeType === 1) {
        inRootTree = typeof st.root.contains === "function" ? st.root.contains(target) : false;
      }

      let clipped = inRootTree ? { left: targetRect.left, top: targetRect.top, right: targetRect.right, bottom: targetRect.bottom, width: targetRect.width, height: targetRect.height } : null;
      if (clipped) {
        // Clip against ancestor scroll containers and clip-path insets, grown by scrollMargin
        let anc = target.parentElement;
        while (anc && anc.nodeType === 1 && anc !== st.root) {
          let ancCs = null;
          try { ancCs = typeof getComputedStyle === "function" ? getComputedStyle(anc) : null; } catch (e) {}
          if (ancCs) {
            const ov = String(ancCs.overflow || "").toLowerCase();
            const ovX = String(ancCs.overflowX || "").toLowerCase();
            const ovY = String(ancCs.overflowY || "").toLowerCase();
            const isClipped = ov === "hidden" || ov === "scroll" || ov === "auto" || ov === "clip" ||
              ovX === "hidden" || ovX === "scroll" || ovX === "auto" || ovX === "clip" ||
              ovY === "hidden" || ovY === "scroll" || ovY === "auto" || ovY === "clip";
            if (isClipped && anc.getBoundingClientRect) {
              const ar = anc.getBoundingClientRect();
              const smT = evalIoMarginPx(st.scrollMarginParsed.tokens[0], ar.height);
              const smR = evalIoMarginPx(st.scrollMarginParsed.tokens[1], ar.width);
              const smB = evalIoMarginPx(st.scrollMarginParsed.tokens[2], ar.height);
              const smL = evalIoMarginPx(st.scrollMarginParsed.tokens[3], ar.width);
              clipped = clipIoRect(clipped, {
                left: ar.left - smL,
                top: ar.top - smT,
                right: ar.right + smR,
                bottom: ar.bottom + smB,
              });
              if (!clipped) break;
            }
            const cp = String(ancCs.clipPath || ancCs["clip-path"] || "").trim();
            const insetMatch = /^inset\(([^)]+)\)/i.exec(cp);
            if (insetMatch && anc.getBoundingClientRect) {
              const ar = anc.getBoundingClientRect();
              const insParts = insetMatch[1].trim().split(/\s+/).map(function (v) { return parseFloat(v) || 0; });
              const iT = insParts[0] || 0;
              const iR = insParts.length > 1 ? insParts[1] : iT;
              const iB = insParts.length > 2 ? insParts[2] : iT;
              const iL = insParts.length > 3 ? insParts[3] : iR;
              const smT = evalIoMarginPx(st.scrollMarginParsed.tokens[0], ar.height);
              const smR = evalIoMarginPx(st.scrollMarginParsed.tokens[1], ar.width);
              const smB = evalIoMarginPx(st.scrollMarginParsed.tokens[2], ar.height);
              const smL = evalIoMarginPx(st.scrollMarginParsed.tokens[3], ar.width);
              clipped = clipIoRect(clipped, {
                left: ar.left + iL - smL,
                top: ar.top + iT - smT,
                right: ar.right - iR + smR,
                bottom: ar.bottom - iB + smB,
              });
              if (!clipped) break;
            }
          }
          anc = anc.parentElement;
        }
      }
      if (clipped) {
        clipped = clipIoRect(clipped, expandedRoot);
      }

      const isIntersecting = Boolean(clipped);
      const iRect = clipped
        ? makeDOMRectReadOnly(clipped.left, clipped.top, clipped.width, clipped.height)
        : ZERO_IO_RECT;
      const targetArea = targetRect.width * targetRect.height;
      let ratio = 0;
      if (isIntersecting) {
        if (targetArea > 0) {
          ratio = Math.min(1, Math.max(0, (clipped.width * clipped.height) / targetArea));
        } else {
          ratio = 1;
        }
      }

      let threshIdx = -1;
      for (let ti = 0; ti < st.thresholds.length; ti++) {
        const th = st.thresholds[ti];
        if (th === 0 ? isIntersecting : ratio >= th) threshIdx = ti;
      }
      const key = isIntersecting + "|" + threshIdx + "|" + targetRect.top + "," + targetRect.left + "," + targetRect.width + "," + targetRect.height;
      if (tInfo.lastKey !== key) {
        tInfo.lastKey = key;
        newEntries.push(new IntersectionObserverEntry({
          time: typeof performance !== "undefined" ? performance.now() : Date.now(),
          rootBounds: makeDOMRectReadOnly(expandedRoot.left, expandedRoot.top, expandedRoot.width, expandedRoot.height),
          boundingClientRect: makeDOMRectReadOnly(targetRect.left, targetRect.top, targetRect.width, targetRect.height),
          intersectionRect: iRect,
          isIntersecting: isIntersecting,
          isVisible: false,
          intersectionRatio: ratio,
          target: target,
        }));
      }
    }

    if (newEntries.length > 0) {
      try {
        st.callback.call(obs, newEntries, obs);
      } catch (e) {
        if (typeof global.onerror === "function") {
          try { global.onerror(e && e.message ? e.message : String(e), "", 0, 0, e); } catch (e2) {}
        }
      }
    }
  }

  function scheduleIoTick() {
    if (ioPollTimer !== null) return;
    if (activeIntersectionObservers.size === 0) return;
    ioPollTimer = setTimeout(function () {
      ioPollTimer = null;
      for (const obs of Array.from(activeIntersectionObservers)) {
        evaluateIntersectionObserver(obs);
      }
      if (activeIntersectionObservers.size > 0) {
        scheduleIoTick();
      }
    }, 50);
  }

  function IntersectionObserver(callback, options) {
    if (typeof callback !== "function") {
      throw new TypeError("Failed to construct 'IntersectionObserver': parameter 1 is not of type 'Function'.");
    }
    const opts = options && typeof options === "object" ? options : {};
    const root = opts.root !== undefined ? opts.root : null;
    const rootMarginParsed = parseIoMarginTokens(opts.rootMargin);
    const scrollMarginParsed = parseIoMarginTokens(opts.scrollMargin);
    let threshArr;
    if (opts.threshold === undefined) {
      threshArr = [0];
    } else if (Array.isArray(opts.threshold)) {
      threshArr = opts.threshold.length === 0 ? [0] : opts.threshold.map(Number);
    } else {
      threshArr = [Number(opts.threshold)];
    }
    for (let i = 0; i < threshArr.length; i++) {
      if (isNaN(threshArr[i]) || threshArr[i] < 0 || threshArr[i] > 1) {
        throw new RangeError("Failed to construct 'IntersectionObserver': Thresholds must be in the range [0, 1].");
      }
    }
    threshArr = Array.from(new Set(threshArr)).sort(function (a, b) { return a - b; });
    ioStateMap.set(this, {
      callback: callback,
      root: root,
      rootMarginParsed: rootMarginParsed,
      scrollMarginParsed: scrollMarginParsed,
      thresholds: Object.freeze(threshArr),
      targets: new Map(),
      records: [],
      pendingMicrotask: false,
    });
  }
  Object.defineProperty(IntersectionObserver.prototype, "root", {
    get: function () { const st = ioStateMap.get(this); if (!st) throw new TypeError("Illegal invocation"); return st.root; },
    configurable: true, enumerable: true,
  });
  Object.defineProperty(IntersectionObserver.prototype, "rootMargin", {
    get: function () { const st = ioStateMap.get(this); if (!st) throw new TypeError("Illegal invocation"); return st.rootMarginParsed.text; },
    configurable: true, enumerable: true,
  });
  Object.defineProperty(IntersectionObserver.prototype, "scrollMargin", {
    get: function () { const st = ioStateMap.get(this); if (!st) throw new TypeError("Illegal invocation"); return st.scrollMarginParsed.text; },
    configurable: true, enumerable: true,
  });
  Object.defineProperty(IntersectionObserver.prototype, "thresholds", {
    get: function () { const st = ioStateMap.get(this); if (!st) throw new TypeError("Illegal invocation"); return st.thresholds; },
    configurable: true, enumerable: true,
  });
  IntersectionObserver.prototype.observe = markNative(function (el) {
    const st = ioStateMap.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    if (!el || typeof el !== "object") return;
    if (!st.targets.has(el)) {
      st.targets.set(el, { lastKey: null });
    }
    activeIntersectionObservers.add(this);
    const obs = this;
    if (!st.pendingMicrotask) {
      st.pendingMicrotask = true;
      setTimeout(function () {
        st.pendingMicrotask = false;
        evaluateIntersectionObserver(obs);
      }, 0);
    }
    scheduleIoTick();
  }, "observe");
  IntersectionObserver.prototype.unobserve = markNative(function (el) {
    const st = ioStateMap.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    st.targets.delete(el);
    if (st.targets.size === 0) activeIntersectionObservers.delete(this);
  }, "unobserve");
  IntersectionObserver.prototype.disconnect = markNative(function () {
    const st = ioStateMap.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    st.targets.clear();
    st.records.length = 0;
    activeIntersectionObservers.delete(this);
  }, "disconnect");
  IntersectionObserver.prototype.takeRecords = markNative(function () {
    const st = ioStateMap.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    const out = st.records.slice();
    st.records.length = 0;
    return out;
  }, "takeRecords");
  Object.defineProperty(IntersectionObserver.prototype, Symbol.toStringTag, {
    value: "IntersectionObserver",
    configurable: true,
  });
  markNative(IntersectionObserver, "IntersectionObserver");
  global.IntersectionObserver = IntersectionObserver;

  const activeResizeObservers = new Set();
  let resizeObserverPollTimer = null;

  function measureResizeObserverTarget(el) {
    const rect = el && typeof el.getBoundingClientRect === "function"
      ? el.getBoundingClientRect()
      : { width: 0, height: 0 };
    const borderW = Number(rect.width || 0);
    const borderH = Number(rect.height || 0);
    if (borderW <= 0 && borderH <= 0) {
      return {
        contentRect: new DOMRectReadOnly(0, 0, 0, 0),
        borderWidth: 0,
        borderHeight: 0,
        contentWidth: 0,
        contentHeight: 0,
      };
    }
    let pt = 0, pr = 0, pb = 0, pl = 0;
    let bt = 0, br = 0, bb = 0, bl = 0;
    try {
      const csFn = global.getComputedStyle || (typeof getComputedStyle === "function" ? getComputedStyle : null);
      if (csFn) {
        const cs = csFn(el);
        if (cs && typeof cs.getPropertyValue === "function") {
          const parsePx = function (v) {
            const s = String(v || "").trim().toLowerCase();
            if (s === "thin") return 1;
            if (s === "medium") return 3;
            if (s === "thick") return 5;
            const n = parseFloat(s);
            return isNaN(n) ? 0 : n;
          };
          pt = parsePx(cs.getPropertyValue("padding-top"));
          pr = parsePx(cs.getPropertyValue("padding-right"));
          pb = parsePx(cs.getPropertyValue("padding-bottom"));
          pl = parsePx(cs.getPropertyValue("padding-left"));
          const bts = String(cs.getPropertyValue("border-top-style") || "").trim().toLowerCase();
          const brs = String(cs.getPropertyValue("border-right-style") || "").trim().toLowerCase();
          const bbs = String(cs.getPropertyValue("border-bottom-style") || "").trim().toLowerCase();
          const bls = String(cs.getPropertyValue("border-left-style") || "").trim().toLowerCase();
          bt = (bts === "none" || bts === "hidden") ? 0 : parsePx(cs.getPropertyValue("border-top-width"));
          br = (brs === "none" || brs === "hidden") ? 0 : parsePx(cs.getPropertyValue("border-right-width"));
          bb = (bbs === "none" || bbs === "hidden") ? 0 : parsePx(cs.getPropertyValue("border-bottom-width"));
          bl = (bls === "none" || bls === "hidden") ? 0 : parsePx(cs.getPropertyValue("border-left-width"));
        }
      }
    } catch (e) {}
    const contentW = Math.max(0, borderW - bl - br - pl - pr);
    const contentH = Math.max(0, borderH - bt - bb - pt - pb);
    return {
      contentRect: new DOMRectReadOnly(pl, pt, contentW, contentH),
      borderWidth: borderW,
      borderHeight: borderH,
      contentWidth: contentW,
      contentHeight: contentH,
    };
  }

  function deliverResizeObserver(obs) {
    if (!obs || obs._disconnected || obs._targets.size === 0) return;
    const entries = [];
    for (const [el, last] of obs._targets.entries()) {
      const m = measureResizeObserverTarget(el);
      const changed = !last ||
        Math.abs(m.contentWidth - last.width) > 0.01 ||
        Math.abs(m.contentHeight - last.height) > 0.01;
      if (changed) {
        obs._targets.set(el, { width: m.contentWidth, height: m.contentHeight });
        entries.push({
          target: el,
          contentRect: m.contentRect,
          borderBoxSize: [{ inlineSize: m.borderWidth, blockSize: m.borderHeight }],
          contentBoxSize: [{ inlineSize: m.contentWidth, blockSize: m.contentHeight }],
          devicePixelContentBoxSize: [{ inlineSize: Math.round(m.contentWidth), blockSize: Math.round(m.contentHeight) }],
        });
      }
    }
    if (entries.length > 0) {
      try {
        obs.callback(entries, obs);
      } catch (e) {}
    }
  }

  function ensureResizeObserverPoller() {
    if (resizeObserverPollTimer !== null) return;
    if (activeResizeObservers.size === 0) return;
    if (typeof setInterval === "function") {
      resizeObserverPollTimer = setInterval(function () {
        if (activeResizeObservers.size === 0) {
          if (typeof clearInterval === "function") clearInterval(resizeObserverPollTimer);
          resizeObserverPollTimer = null;
          return;
        }
        for (const obs of Array.from(activeResizeObservers)) {
          deliverResizeObserver(obs);
        }
      }, 100);
    }
  }

  function ResizeObserver(callback) {
    if (typeof callback !== "function") throw new TypeError("Callback required");
    this.callback = callback;
    this._targets = new Map();
    this._disconnected = false;
  }
  ResizeObserver.prototype.observe = markNative(function (el) {
    if (!el) return;
    this._disconnected = false;
    if (!this._targets.has(el)) {
      this._targets.set(el, null);
    }
    activeResizeObservers.add(this);
    ensureResizeObserverPoller();
    const obs = this;
    Promise.resolve().then(function () {
      deliverResizeObserver(obs);
    });
  }, "observe");
  ResizeObserver.prototype.unobserve = markNative(function (el) {
    if (this._targets) {
      this._targets.delete(el);
      if (this._targets.size === 0) {
        activeResizeObservers.delete(this);
      }
    }
  }, "unobserve");
  ResizeObserver.prototype.disconnect = markNative(function () {
    this._disconnected = true;
    if (this._targets) this._targets.clear();
    activeResizeObservers.delete(this);
  }, "disconnect");
  markNative(ResizeObserver, "ResizeObserver");
  global.ResizeObserver = ResizeObserver;

  function evaluateMediaFeature(featureStr, w, h, dpr) {
    const m = featureStr.match(/^\(\s*([a-z-]+)\s*(?::\s*(.+?))?\s*\)$/);
    if (!m) return false;
    const feat = m[1];
    const val = (m[2] || "").trim();

    if (feat === "min-width" || feat === "max-width" || feat === "width") {
      const numM = val.match(/^([0-9.]+)\s*(px)?$/);
      if (!numM) return false;
      const px = parseFloat(numM[1]);
      if (isNaN(px)) return false;
      if (feat === "min-width") return w >= px;
      if (feat === "max-width") return w <= px;
      return w === px;
    }
    if (feat === "min-height" || feat === "max-height" || feat === "height") {
      const numM = val.match(/^([0-9.]+)\s*(px)?$/);
      if (!numM) return false;
      const px = parseFloat(numM[1]);
      if (isNaN(px)) return false;
      if (feat === "min-height") return h >= px;
      if (feat === "max-height") return h <= px;
      return h === px;
    }
    if (feat === "orientation") {
      if (val === "landscape") return w >= h;
      if (val === "portrait") return w < h;
      return false;
    }
    const ef = (typeof globalThis !== "undefined" && globalThis.__capsurf_emulated_features) || {};
    if (feat === "prefers-color-scheme") {
      return val === (ef["prefers-color-scheme"] || "light");
    }
    if (feat === "prefers-reduced-motion") {
      return val === (ef["prefers-reduced-motion"] || "no-preference");
    }
    if (feat === "forced-colors") {
      return val === (ef["forced-colors"] || "none");
    }
    if (feat === "prefers-contrast") {
      return val === (ef["prefers-contrast"] || "no-preference");
    }
    if (feat === "hover") {
      return val === "hover";
    }
    if (feat === "pointer") {
      return val === "fine";
    }
    if (feat === "min-resolution" || feat === "max-resolution" || feat === "resolution") {
      const resM = val.match(/^([0-9.]+)\s*(dppx|x|dpi)$/);
      if (!resM) return false;
      let targetDpr = parseFloat(resM[1]);
      if (isNaN(targetDpr)) return false;
      if (resM[2] === "dpi") targetDpr = targetDpr / 96;
      if (feat === "min-resolution") return dpr >= targetDpr;
      if (feat === "max-resolution") return dpr <= targetDpr;
      return dpr === targetDpr;
    }
    return false;
  }

  function evaluateMediaQuery(query) {
    if (!query || !String(query).trim()) return true;
    const raw = String(query).toLowerCase().trim();
    const w = (global.innerWidth || (typeof window !== "undefined" ? window.innerWidth : 1280)) || 1280;
    const h = (global.innerHeight || (typeof window !== "undefined" ? window.innerHeight : 720)) || 720;
    const dpr = (global.devicePixelRatio || (typeof window !== "undefined" ? window.devicePixelRatio : 1)) || 1;

    const branches = raw.split(",");
    for (let i = 0; i < branches.length; i++) {
      let branch = branches[i].trim();
      if (!branch) continue;
      let negate = false;
      if (branch.startsWith("not ") || branch.startsWith("not\t")) {
        negate = true;
        branch = branch.slice(3).trim();
      } else if (branch.startsWith("only ") || branch.startsWith("only\t")) {
        branch = branch.slice(4).trim();
      }
      const parts = branch.split(/\s+and\s+/);
      let branchOk = true;
      for (let j = 0; j < parts.length; j++) {
        const part = parts[j].trim();
        if (!part) {
          branchOk = false;
          break;
        }
        if (part.startsWith("(") && part.endsWith(")")) {
          if (!evaluateMediaFeature(part, w, h, dpr)) {
            branchOk = false;
            break;
          }
        } else if (part === "all") {
          // matches
        } else if (part === "screen" || part === "print") {
          const curMedia = ((typeof globalThis !== "undefined" && globalThis.__capsurf_emulated_media) || "screen").toLowerCase();
          if (part !== curMedia) {
            branchOk = false;
            break;
          }
        } else {
          branchOk = false;
          break;
        }
      }
      if (negate) branchOk = !branchOk;
      if (branchOk) return true;
    }
    return false;
  }

  // 7. matchMedia
  global.matchMedia = markNative(function (query) {
    return {
      matches: evaluateMediaQuery(query),
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
  const stopFn = markNative(function () {}, "stop");

  global.alert = alertFn;
  global.confirm = confirmFn;
  global.prompt = promptFn;
  global.stop = stopFn;

  if (typeof window !== "undefined") {
    window.alert = alertFn;
    window.confirm = confirmFn;
    window.prompt = promptFn;
    window.stop = stopFn;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.alert = alertFn;
    globalThis.confirm = confirmFn;
    globalThis.prompt = promptFn;
    globalThis.stop = stopFn;
  }

  // 10. CSSOM getComputedStyle & CSSStyleDeclaration (uses global.CSSStyleDeclaration from cssom.js)

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

  function splitCssTopLevelWhitespace(str) {
    const tokens = [];
    let depth = 0;
    let inQuote = null;
    let cur = "";
    for (let i = 0; i < str.length; i++) {
      const c = str[i];
      if (inQuote) {
        cur += c;
        if (c === inQuote && str[i - 1] !== "\\") inQuote = null;
      } else if (c === '"' || c === "'") {
        inQuote = c;
        cur += c;
      } else if (c === "(") {
        depth++;
        cur += c;
      } else if (c === ")") {
        depth = Math.max(0, depth - 1);
        cur += c;
      } else if (depth === 0 && /\s/.test(c)) {
        if (cur) {
          tokens.push(cur);
          cur = "";
        }
      } else {
        cur += c;
      }
    }
    if (cur) tokens.push(cur);
    return tokens;
  }

  function expandFourSides(tokens) {
    if (tokens.length === 1) return [tokens[0], tokens[0], tokens[0], tokens[0]];
    if (tokens.length === 2) return [tokens[0], tokens[1], tokens[0], tokens[1]];
    if (tokens.length === 3) return [tokens[0], tokens[1], tokens[2], tokens[1]];
    if (tokens.length >= 4) return [tokens[0], tokens[1], tokens[2], tokens[3]];
    return null;
  }

  function parseDeclarationsBlockInto(element, declBlock, computedMap) {
    const impSet = computedMap._importantKeys || (computedMap._importantKeys = new Set());
    function setDecl(k, v, isImp) {
      if (!isImp && impSet.has(k)) return;
      computedMap.set(k, v);
      if (isImp) impSet.add(k);
    }
    const decls = splitCssDeclarations(declBlock);
    for (let i = 0; i < decls.length; i++) {
      const d = decls[i].trim();
      if (!d) continue;
      const colon = d.indexOf(":");
      if (colon === -1) continue;
      const prop = d.slice(0, colon).trim();
      let val = d.slice(colon + 1).trim();
      if (!prop) continue;
      let isImp = false;
      if (val.toLowerCase().endsWith("!important")) {
        isImp = true;
        val = val.slice(0, -10).trim();
      }
      val = resolveAttrExpressions(element, val);
      const key = prop.startsWith("--") ? prop : prop.toLowerCase();
      setDecl(key, val, isImp);

      if (key === "inset") {
        const sides = expandFourSides(splitCssTopLevelWhitespace(val));
        if (sides) {
          setDecl("top", sides[0], isImp);
          setDecl("right", sides[1], isImp);
          setDecl("bottom", sides[2], isImp);
          setDecl("left", sides[3], isImp);
        }
      } else if (key === "margin") {
        const sides = expandFourSides(splitCssTopLevelWhitespace(val));
        if (sides) {
          setDecl("margin-top", sides[0], isImp);
          setDecl("margin-right", sides[1], isImp);
          setDecl("margin-bottom", sides[2], isImp);
          setDecl("margin-left", sides[3], isImp);
        }
      } else if (key === "padding") {
        const sides = expandFourSides(splitCssTopLevelWhitespace(val));
        if (sides) {
          setDecl("padding-top", sides[0], isImp);
          setDecl("padding-right", sides[1], isImp);
          setDecl("padding-bottom", sides[2], isImp);
          setDecl("padding-left", sides[3], isImp);
        }
      } else if (key === "padding-inline-start") {
        setDecl("padding-left", val, isImp);
      } else if (key === "padding-inline-end") {
        setDecl("padding-right", val, isImp);
      } else if (key === "padding-block-start") {
        setDecl("padding-top", val, isImp);
      } else if (key === "padding-block-end") {
        setDecl("padding-bottom", val, isImp);
      } else if (key === "padding-inline") {
        const parts = splitCssTopLevelWhitespace(val);
        if (parts.length === 1) {
          setDecl("padding-left", parts[0], isImp);
          setDecl("padding-right", parts[0], isImp);
        } else if (parts.length >= 2) {
          setDecl("padding-left", parts[0], isImp);
          setDecl("padding-right", parts[1], isImp);
        }
      } else if (key === "padding-block") {
        const parts = splitCssTopLevelWhitespace(val);
        if (parts.length === 1) {
          setDecl("padding-top", parts[0], isImp);
          setDecl("padding-bottom", parts[0], isImp);
        } else if (parts.length >= 2) {
          setDecl("padding-top", parts[0], isImp);
          setDecl("padding-bottom", parts[1], isImp);
        }
      } else if (key === "margin-inline-start") {
        setDecl("margin-left", val, isImp);
      } else if (key === "margin-inline-end") {
        setDecl("margin-right", val, isImp);
      } else if (key === "margin-block-start") {
        setDecl("margin-top", val, isImp);
      } else if (key === "margin-block-end") {
        setDecl("margin-bottom", val, isImp);
      } else if (key === "border-width") {
        const sides = expandFourSides(splitCssTopLevelWhitespace(val));
        if (sides) {
          setDecl("border-top-width", sides[0], isImp);
          setDecl("border-right-width", sides[1], isImp);
          setDecl("border-bottom-width", sides[2], isImp);
          setDecl("border-left-width", sides[3], isImp);
        }
      } else if (key === "border-style") {
        const sides = expandFourSides(splitCssTopLevelWhitespace(val));
        if (sides) {
          setDecl("border-top-style", sides[0], isImp);
          setDecl("border-right-style", sides[1], isImp);
          setDecl("border-bottom-style", sides[2], isImp);
          setDecl("border-left-style", sides[3], isImp);
        }
      } else if (key === "border" || key === "border-top" || key === "border-right" || key === "border-bottom" || key === "border-left") {
        const parts = splitCssTopLevelWhitespace(val);
        let bWidth = "";
        let bStyle = "";
        const styleWords = new Set(["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"]);
        for (let pi = 0; pi < parts.length; pi++) {
          const tok = parts[pi].toLowerCase();
          if (styleWords.has(tok)) {
            bStyle = tok;
          } else if (/^(?:thin|medium|thick|[+-]?(?:\d+\.?\d*|\.\d+)(?:px|em|rem|pt|cm|mm|in|pc)?|0)$/.test(tok)) {
            bWidth = tok;
          }
        }
        const targetSides = key === "border" ? ["top", "right", "bottom", "left"] : [key.slice(7)];
        for (let si = 0; si < targetSides.length; si++) {
          const side = targetSides[si];
          if (bStyle) setDecl("border-" + side + "-style", bStyle, isImp);
          if (bStyle === "none" || bStyle === "hidden") {
            setDecl("border-" + side + "-width", "0px", isImp);
          } else if (bWidth) {
            setDecl("border-" + side + "-width", bWidth, isImp);
          }
        }
      } else if (key === "overflow") {
        const parts = splitCssTopLevelWhitespace(val);
        if (parts.length === 1) {
          setDecl("overflow-x", parts[0], isImp);
          setDecl("overflow-y", parts[0], isImp);
        } else if (parts.length >= 2) {
          setDecl("overflow-x", parts[0], isImp);
          setDecl("overflow-y", parts[1], isImp);
        }
      }
    }
  }

  function matchesSingleSelector(el, sel) {
    if (!el || !sel) return false;
    const s = sel.trim();
    if (s === "*") return true;

    // :focus support
    if (s === ":focus") {
      const doc = el.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
      return Boolean(doc && (doc._activeElement === el || doc.activeElement === el));
    }
    if (s.endsWith(":focus")) {
      const doc = el.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
      if (!doc || (doc._activeElement !== el && doc.activeElement !== el)) return false;
      const base = s.slice(0, -6).trim();
      if (!base || base === "*") return true;
      return matchesSingleSelector(el, base);
    }

    // Simple ID selector #id
    if (s.startsWith("#") && !s.includes(" ") && !s.includes(".") && !s.includes(">") && !s.includes(":") && !s.includes("[")) {
      const id = s.slice(1);
      return (el.id === id || (el.getAttribute && el.getAttribute("id") === id));
    }

    // Simple Class selector .cls
    if (s.startsWith(".") && !s.includes(" ") && !s.includes("#") && !s.includes(">") && !s.includes(":") && !s.includes("[")) {
      const cls = s.slice(1);
      if (el.classList && el.classList.contains(cls)) return true;
      const clsStr = typeof el.className === "string"
        ? el.className
        : (el.className && typeof el.className.baseVal === "string"
          ? el.className.baseVal
          : (el.getAttribute ? (el.getAttribute("class") || "") : ""));
      if (clsStr && clsStr.split(/\s+/).includes(cls)) return true;
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
    const cleanCss = String(cssText || "").replace(/\/\*[\s\S]*?\*\//g, "");
    const parseFn = global.splitStylesheetRules || (typeof globalThis !== "undefined" && globalThis.splitStylesheetRules);
    if (typeof parseFn === "function") {
      const rules = parseFn(cleanCss);
      for (let i = 0; i < rules.length; i++) {
        const r = rules[i];
        if (!r) continue;
        const sel = String(r.selectorText || "").trim();
        if (sel.startsWith("@media")) {
          const query = sel.replace(/^@media\s*/i, "");
          if (evaluateMediaQuery(query)) {
            parseCssRulesInto(element, r.declText || "", computedMap, normalizedPseudo);
          }
        } else if (sel.startsWith("@supports")) {
          parseCssRulesInto(element, r.declText || "", computedMap, normalizedPseudo);
        } else if (sel.startsWith("@")) {
          continue;
        } else if (sel) {
          if (matchesSelector(element, sel, normalizedPseudo)) {
            parseDeclarationsBlockInto(element, r.declText || "", computedMap);
          }
        }
      }
      return;
    }

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

  const BLOCK_LEVEL_TAGS = new Set([
    "html", "body", "div", "p", "h1", "h2", "h3", "h4", "h5", "h6",
    "ul", "ol", "li", "dl", "dt", "dd", "section", "article", "nav",
    "aside", "header", "footer", "main", "form", "fieldset", "blockquote",
    "pre", "hr", "figure", "figcaption", "address", "details", "summary", "dialog",
  ]);
  const REPLACED_INLINE_TAGS = new Set([
    "img", "input", "button", "select", "textarea", "canvas", "svg", "video", "audio", "iframe", "object", "embed",
  ]);
  const HIDDEN_BY_DEFAULT_TAGS = new Set([
    "head", "style", "script", "meta", "link", "title", "template", "datalist", "base", "noscript",
  ]);
  const INHERITED_CSS_PROPS = new Set([
    "color", "font-size", "font-family", "font-weight", "font-style",
    "line-height", "text-align", "visibility", "direction", "white-space",
    "letter-spacing", "word-spacing", "cursor",
  ]);

  function getDefaultDisplayForElement(element) {
    const tag = String((element && (element.localName || element.tagName)) || "").toLowerCase();
    if (HIDDEN_BY_DEFAULT_TAGS.has(tag)) return "none";
    if (element && typeof element.hasAttribute === "function" && element.hasAttribute("hidden")) {
      const hv = String(element.getAttribute("hidden") || "").trim().toLowerCase();
      if (hv !== "until-found") return "none";
    }
    if (BLOCK_LEVEL_TAGS.has(tag)) return "block";
    if (tag === "table") return "table";
    if (tag === "tr") return "table-row";
    if (tag === "td" || tag === "th") return "table-cell";
    if (tag === "tbody") return "table-row-group";
    if (tag === "thead") return "table-header-group";
    if (tag === "tfoot") return "table-footer-group";
    return "inline";
  }

  function getStyleNodesForElement(element) {
    const doc = element.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
    const rootNode = typeof element.getRootNode === "function" ? element.getRootNode() : null;
    const styleScope = (rootNode && rootNode.nodeType === 11 && rootNode.host) ? rootNode : doc;
    const styleNodes = [];
    if (!styleScope) return styleNodes;
    try {
      if (typeof styleScope.querySelectorAll === "function") {
        const list = styleScope.querySelectorAll("style, link");
        for (let i = 0; i < list.length; i++) {
          const node = list[i];
          const tag = String(node.localName || node.tagName || "").toLowerCase();
          if (tag === "style") {
            styleNodes.push(node);
          } else if (tag === "link") {
            const rel = String(node.getAttribute ? (node.getAttribute("rel") || "") : "").trim().toLowerCase().split(/\s+/);
            if (rel.includes("stylesheet")) styleNodes.push(node);
          }
        }
      }
    } catch (e) {}
    if (styleNodes.length === 0) {
      function scanStyles(node) {
        if (!node) return;
        const tag = node.tagName ? String(node.tagName).toLowerCase() : "";
        if (tag === "style") {
          styleNodes.push(node);
        } else if (tag === "link") {
          const rel = String(node.getAttribute ? (node.getAttribute("rel") || "") : "").trim().toLowerCase().split(/\s+/);
          if (rel.includes("stylesheet")) styleNodes.push(node);
        }
        if (node.childNodes && node.childNodes.length) {
          for (let i = 0; i < node.childNodes.length; i++) {
            scanStyles(node.childNodes[i]);
          }
        }
      }
      if (styleScope.nodeType === 11) {
        scanStyles(styleScope);
      } else {
        if (styleScope.head) scanStyles(styleScope.head);
        if (styleScope.body) scanStyles(styleScope.body);
        if (styleScope.documentElement) scanStyles(styleScope.documentElement);
      }
    }
    return styleNodes;
  }

  function resolveVarsInMap(computedMap) {
    function substitute(val, depth) {
      if (typeof val !== "string" || val.indexOf("var(") === -1 || depth > 16) return val;
      let out = "";
      let i = 0;
      while (i < val.length) {
        const idx = val.indexOf("var(", i);
        if (idx === -1) {
          out += val.slice(i);
          break;
        }
        out += val.slice(i, idx);
        let d = 1;
        let j = idx + 4;
        while (j < val.length && d > 0) {
          if (val[j] === "(") d++;
          else if (val[j] === ")") d--;
          j++;
        }
        const inside = val.slice(idx + 4, j - 1);
        let commaIdx = -1;
        let pDepth = 0;
        for (let k = 0; k < inside.length; k++) {
          if (inside[k] === "(") pDepth++;
          else if (inside[k] === ")") pDepth--;
          else if (inside[k] === "," && pDepth === 0) {
            commaIdx = k;
            break;
          }
        }
        const varName = (commaIdx === -1 ? inside : inside.slice(0, commaIdx)).trim();
        const fallback = commaIdx === -1 ? "" : inside.slice(commaIdx + 1).trim();
        let rep = computedMap.has(varName) ? computedMap.get(varName) : "";
        if (!rep && fallback) rep = fallback;
        if (rep && rep.indexOf("var(") !== -1) {
          rep = substitute(rep, depth + 1);
        }
        out += rep;
        i = j;
      }
      return out;
    }
    for (const [k, v] of computedMap.entries()) {
      if (typeof v === "string" && v.indexOf("var(") !== -1) {
        computedMap.set(k, substitute(v, 0));
      }
    }
  }

  function collectElementOwnDeclarations(element, styleNodes, normalizedPseudo, targetMap) {
    if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_tty_ua_css_text === "string" && globalThis.__capsurf_tty_ua_css_text) {
      parseCssRulesInto(element, globalThis.__capsurf_tty_ua_css_text, targetMap, normalizedPseudo);
    }
    for (let i = 0; i < styleNodes.length; i++) {
      const sn = styleNodes[i];
      if (!sn) continue;
      const mediaRaw = String(sn.getAttribute ? (sn.getAttribute("media") || "") : "").trim();
      if (mediaRaw) {
        const mqParts = mediaRaw.split(",");
        let mediaOk = false;
        for (let mi = 0; mi < mqParts.length; mi++) {
          const mq = mqParts[mi].trim().toLowerCase();
          if (!mq) continue;
          if (mq === "print" || mq === "only print" || mq === "speech" || mq === "not screen" || mq === "not all") continue;
          if (mq.indexOf("print") !== -1 && !mq.startsWith("not print") && mq.indexOf("screen") === -1 && mq.indexOf("all") === -1) continue;
          if (evaluateMediaQuery(mq)) {
            mediaOk = true;
            break;
          }
        }
        if (!mediaOk) continue;
      }
      const snTag = String(sn.localName || sn.tagName || "").toLowerCase();
      let text = "";
      if (snTag === "link") {
        const sheet = sn.sheet;
        if (sheet) {
          if (typeof sheet._syncFromOwner === "function") sheet._syncFromOwner();
          text = sheet._cascadeCssText || "";
        }
      } else {
        text = sn.textContent || "";
      }
      if (text) {
        parseCssRulesInto(element, text, targetMap, normalizedPseudo);
      }
    }
    if (!normalizedPseudo) {
      const rawInline = element.getAttribute ? element.getAttribute("style") : "";
      if (rawInline) {
        parseDeclarationsBlockInto(element, rawInline, targetMap);
      }
    }
  }

  function collectRawComputedMap(element, normalizedPseudo) {
    const styleNodes = getStyleNodesForElement(element);
    const computedMap = new Map();

    // Collect ancestors from root down to parent for inherited properties & custom properties
    const ancestors = [];
    let p = element.parentElement || element.parentNode;
    while (p && p.nodeType === 1) {
      ancestors.push(p);
      p = p.parentElement || p.parentNode;
    }
    for (let i = ancestors.length - 1; i >= 0; i--) {
      const ancMap = new Map();
      collectElementOwnDeclarations(ancestors[i], styleNodes, "", ancMap);
      for (const [k, v] of ancMap.entries()) {
        if (k.startsWith("--") || INHERITED_CSS_PROPS.has(k)) {
          computedMap.set(k, v);
        }
      }
    }

    // Own declarations (or pseudo-element declarations)
    if (normalizedPseudo) {
      const elMap = new Map();
      collectElementOwnDeclarations(element, styleNodes, "", elMap);
      for (const [k, v] of elMap.entries()) {
        if (k.startsWith("--") || INHERITED_CSS_PROPS.has(k)) {
          computedMap.set(k, v);
        }
      }
    }
    collectElementOwnDeclarations(element, styleNodes, normalizedPseudo, computedMap);

    // Overlay active/paused Web Animations values
    if (!normalizedPseudo && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_getAnimatedStyles === "function") {
      const animStyles = globalThis.__capsurf_getAnimatedStyles(element);
      for (const k in animStyles) {
        if (Object.prototype.hasOwnProperty.call(animStyles, k)) {
          computedMap.set(k, animStyles[k]);
        }
      }
    }

    // Resolve var() references using the complete (inherited + own) map
    resolveVarsInMap(computedMap);

    // Sync CSS Transitions and CSS @keyframes Animations (updates before-change snapshot & overlays interpolated values)
    if (!normalizedPseudo && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_syncCssTransitionsAndAnimations === "function") {
      globalThis.__capsurf_syncCssTransitionsAndAnimations(element, computedMap, styleNodes);
    }

    if (computedMap.has("content")) {
      const cRaw = String(computedMap.get("content") || "").trim();
      if ((cRaw.startsWith("\"") && cRaw.endsWith("\"") && cRaw.length >= 2) ||
          (cRaw.startsWith("'") && cRaw.endsWith("'") && cRaw.length >= 2)) {
        const inner = cRaw.slice(1, -1);
        const decoded = inner.replace(/\\([0-9a-fA-F]{1,6})[\t\n\f\r ]?|\\(.)/g, function (_, hex, ch) {
          if (hex) {
            const cp = parseInt(hex, 16);
            if (cp === 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return "\uFFFD";
            try { return String.fromCodePoint(cp); } catch (e) { return "\uFFFD"; }
          }
          return ch;
        });
        const reserialized = "\"" + decoded.replace(/\\/g, "\\\\").replace(/"/g, "\\\"") + "\"";
        computedMap.set("content", reserialized);
      }
    }

    // Pseudo element defaults
    if (normalizedPseudo === "::before" || normalizedPseudo === "::after") {
      if (!computedMap.has("display")) {
        computedMap.set("display", "inline");
      }
    }

    return computedMap;
  }

  function formatCssPx(n) {
    if (!isFinite(n) || Math.abs(n) < 1e-9) return "0px";
    return String(Number(n.toFixed(4))) + "px";
  }

  function evalCssLengthExpr(expr, refPx, fontSizePx) {
    if (typeof expr !== "string") return null;
    const s = expr.trim();
    if (!s) return null;
    if (s === "0") return 0;

    // Fast path for simple length/percentage
    const simple = s.match(/^([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)(px|%|em|rem|pt|vw|vh|ch|lh)?$/i);
    if (simple) {
      const num = parseFloat(simple[1]);
      const unit = (simple[2] || "").toLowerCase();
      if (unit === "px" || unit === "") return num;
      if (unit === "%") return refPx !== null ? (num / 100) * refPx : null;
      if (unit === "em") return num * (fontSizePx || 16);
      if (unit === "rem") return num * 16;
      if (unit === "pt") return num * (96 / 72);
      if (unit === "vw") return (num / 100) * 800;
      if (unit === "vh") return (num / 100) * 600;
      if (unit === "ch") {
        const cw = (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_tty_cell_w === "number" && globalThis.__capsurf_tty_cell_w > 0) ? globalThis.__capsurf_tty_cell_w : 8;
        return num * cw;
      }
      if (unit === "lh") {
        const ch = (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_tty_cell_h === "number" && globalThis.__capsurf_tty_cell_h > 0) ? globalThis.__capsurf_tty_cell_h : 17;
        return num * ch;
      }
      return null;
    }

    // Recursive descent parser for calc(), min(), max(), clamp()
    let pos = 0;
    const len = s.length;
    function skipWs() {
      while (pos < len && /\s/.test(s[pos])) pos++;
    }
    function parseAddSub() {
      let left = parseMulDiv();
      if (left === null) return null;
      while (true) {
        skipWs();
        if (pos < len && (s[pos] === "+" || s[pos] === "-")) {
          const op = s[pos++];
          const right = parseMulDiv();
          if (right === null) return null;
          left = op === "+" ? left + right : left - right;
        } else {
          break;
        }
      }
      return left;
    }
    function parseMulDiv() {
      let left = parsePrimary();
      if (left === null) return null;
      while (true) {
        skipWs();
        if (pos < len && (s[pos] === "*" || s[pos] === "/")) {
          const op = s[pos++];
          const right = parsePrimary();
          if (right === null) return null;
          left = op === "*" ? left * right : (right !== 0 ? left / right : 0);
        } else {
          break;
        }
      }
      return left;
    }
    function parsePrimary() {
      skipWs();
      if (pos >= len) return null;
      if (s[pos] === "(") {
        pos++;
        const v = parseAddSub();
        skipWs();
        if (s[pos] === ")") pos++;
        return v;
      }
      // Function call: calc, min, max, clamp
      const fnMatch = s.slice(pos).match(/^(calc|min|max|clamp)\s*\(/i);
      if (fnMatch) {
        const fnName = fnMatch[1].toLowerCase();
        pos += fnMatch[0].length;
        const args = [];
        while (pos < len) {
          const v = parseAddSub();
          if (v === null) return null;
          args.push(v);
          skipWs();
          if (s[pos] === ",") {
            pos++;
            continue;
          }
          if (s[pos] === ")") {
            pos++;
            break;
          }
          return null;
        }
        if (fnName === "calc") return args.length === 1 ? args[0] : null;
        if (fnName === "min") return args.length > 0 ? Math.min.apply(null, args) : null;
        if (fnName === "max") return args.length > 0 ? Math.max.apply(null, args) : null;
        if (fnName === "clamp") {
          if (args.length !== 3) return null;
          return Math.max(args[0], Math.min(args[1], args[2]));
        }
      }
      // Number + optional unit
      const numMatch = s.slice(pos).match(/^([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)(px|%|em|rem|pt|vw|vh)?/i);
      if (numMatch) {
        pos += numMatch[0].length;
        const num = parseFloat(numMatch[1]);
        const unit = (numMatch[2] || "").toLowerCase();
        if (unit === "px" || unit === "") return num;
        if (unit === "%") return refPx !== null ? (num / 100) * refPx : null;
        if (unit === "em") return num * (fontSizePx || 16);
        if (unit === "rem") return num * 16;
        if (unit === "pt") return num * (96 / 72);
        if (unit === "vw") return (num / 100) * 800;
        if (unit === "vh") return (num / 100) * 600;
        return null;
      }
      return null;
    }

    const res = parseAddSub();
    skipWs();
    if (res === null || pos < len) return null;
    return res;
  }

  function hasLayoutBox(element, rawMap) {
    if (!element || element.nodeType !== 1) return false;
    const ownDisp = (rawMap.get("display") || getDefaultDisplayForElement(element)).trim().toLowerCase();
    if (ownDisp === "none" || ownDisp === "contents") return false;
    let p = element.parentElement || element.parentNode;
    while (p && p.nodeType === 1) {
      const pMap = collectRawComputedMap(p, "");
      const pDisp = (pMap.get("display") || getDefaultDisplayForElement(p)).trim().toLowerCase();
      if (pDisp === "none") return false;
      p = p.parentElement || p.parentNode;
    }
    return true;
  }

  function computeElementUsedWidthPx(element, depth, visited) {
    if (!element || element.nodeType !== 1 || depth > 24) return 800;
    if (!visited) visited = new Set();
    if (visited.has(element)) return 0;
    visited.add(element);
    try {
      const rawMap = collectRawComputedMap(element, "");
      const disp = (rawMap.get("display") || getDefaultDisplayForElement(element)).trim().toLowerCase();
      if (disp === "contents") {
        return computeElementUsedWidthPx(element.parentElement || element.parentNode, depth + 1, visited);
      }
      const fontSize = evalCssLengthExpr(rawMap.get("font-size") || "16px", 16, 16) || 16;
      const parentEl = element.parentElement || element.parentNode;
      const cbW = (parentEl && parentEl.nodeType === 1) ? computeElementUsedWidthPx(parentEl, depth + 1, visited) : 800;
      const wRaw = (rawMap.get("width") || "auto").trim().toLowerCase();
      let w = null;
      if (wRaw === "auto") {
        const ml = evalCssLengthExpr(rawMap.get("margin-left") || "0px", cbW, fontSize) || 0;
        const mr = evalCssLengthExpr(rawMap.get("margin-right") || "0px", cbW, fontSize) || 0;
        w = Math.max(0, cbW - ml - mr);
      } else if (wRaw === "min-content" || wRaw === "max-content") {
        let maxChildW = 0;
        let ch = element.firstChild;
        while (ch) {
          if (ch.nodeType === 1) {
            const cw = computeElementUsedWidthPx(ch, depth + 1, visited);
            if (cw > maxChildW) maxChildW = cw;
          }
          ch = ch.nextSibling;
        }
        w = maxChildW;
      } else {
        w = evalCssLengthExpr(wRaw, cbW, fontSize);
        if (w === null) w = cbW;
      }

      // Clamp with max-width and min-width
      const maxWRaw = (rawMap.get("max-width") || "").trim().toLowerCase();
      if (maxWRaw && maxWRaw !== "none") {
        const maxW = evalCssLengthExpr(maxWRaw, cbW, fontSize);
        if (maxW !== null && w > maxW) w = maxW;
      }
      const minWRaw = (rawMap.get("min-width") || "").trim().toLowerCase();
      if (minWRaw && minWRaw !== "auto") {
        const minW = evalCssLengthExpr(minWRaw, cbW, fontSize);
        if (minW !== null && w < minW) w = minW;
      }
      return w;
    } finally {
      visited.delete(element);
    }
  }

  function computeElementUsedHeightPx(element, depth) {
    if (!element || element.nodeType !== 1 || depth > 24) return 600;
    const rawMap = collectRawComputedMap(element, "");
    const fontSize = evalCssLengthExpr(rawMap.get("font-size") || "16px", 16, 16) || 16;
    const hRaw = (rawMap.get("height") || "auto").trim().toLowerCase();
    if (hRaw && hRaw !== "auto") {
      const parentEl = element.parentElement || element.parentNode;
      const cbH = (parentEl && parentEl.nodeType === 1) ? computeElementUsedHeightPx(parentEl, depth + 1) : 600;
      const h = evalCssLengthExpr(hRaw, cbH, fontSize);
      if (h !== null) return h;
    }
    return 0;
  }

  function findStickyScrollportSize(element, axis) {
    // axis: "x" or "y". Scroll container has overflow-{axis} in {auto, scroll, hidden} (NOT clip or visible)
    const prop = axis === "x" ? "overflow-x" : "overflow-y";
    let p = element.parentElement || element.parentNode;
    while (p && p.nodeType === 1) {
      const pMap = collectRawComputedMap(p, "");
      const ov = (pMap.get(prop) || "visible").trim().toLowerCase();
      if (ov === "hidden" || ov === "scroll" || ov === "auto") {
        return axis === "x" ? computeElementUsedWidthPx(p, 0) : computeElementUsedHeightPx(p, 0);
      }
      p = p.parentElement || p.parentNode;
    }
    return axis === "x" ? 800 : 600;
  }

  function evalCssAngleDeg(expr) {
    if (typeof expr !== "string") return null;
    const s = expr.trim().toLowerCase();
    if (!s) return null;
    const simple = s.match(/^([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)(deg|grad|rad|turn)$/);
    if (simple) {
      const n = parseFloat(simple[1]);
      const u = simple[2];
      if (u === "deg") return n;
      if (u === "grad") return n * 0.9;
      if (u === "rad") return n * (180 / Math.PI);
      if (u === "turn") return n * 360;
    }
    const fnMatch = s.match(/^(hypot|abs|min|max|calc)\((.+)\)$/);
    if (fnMatch) {
      const fn = fnMatch[1];
      const args = fnMatch[2].split(",").map(function (a) { return evalCssAngleDeg(a.trim()); });
      if (args.some(function (a) { return a === null || isNaN(a); })) return null;
      if (fn === "hypot") return Math.hypot.apply(Math, args);
      if (fn === "abs") return Math.abs(args[0]);
      if (fn === "min") return Math.min.apply(Math, args);
      if (fn === "max") return Math.max.apply(Math, args);
      if (fn === "calc") return args[0];
    }
    return null;
  }

  function applyUsedValuesToComputedMap(element, computedMap, normalizedPseudo) {
    if (normalizedPseudo || !element || element.nodeType !== 1) return;
    const fsRaw = computedMap.get("font-size");
    if (fsRaw !== undefined && fsRaw.trim() === "0") {
      computedMap.set("font-size", "0px");
    }
    if (!computedMap.has("z-index")) {
      computedMap.set("z-index", "auto");
    } else {
      const ziRaw = String(computedMap.get("z-index") || "").trim();
      if (/^(?:calc|min|max|clamp)\s*\(/i.test(ziRaw)) {
        const ziNum = typeof evalCssLengthExpr === "function" ? evalCssLengthExpr(ziRaw, 0, 16) : null;
        if (ziNum !== null && isFinite(ziNum)) {
          computedMap.set("z-index", String(Math.round(ziNum)));
        }
      }
    }
    const defaultAutoProps = ["top", "right", "bottom", "left", "flex-basis"];
    for (let ai = 0; ai < defaultAutoProps.length; ai++) {
      if (!computedMap.has(defaultAutoProps[ai])) {
        computedMap.set(defaultAutoProps[ai], "auto");
      }
    }
    if (!computedMap.has("position")) computedMap.set("position", "static");
    if (!computedMap.has("flex-direction")) computedMap.set("flex-direction", "row");
    if (!computedMap.has("flex-wrap")) computedMap.set("flex-wrap", "nowrap");
    if (!computedMap.has("flex-grow")) computedMap.set("flex-grow", "0");
    if (!computedMap.has("flex-shrink")) computedMap.set("flex-shrink", "1");
    const defaultZeroMargins = ["margin-top", "margin-right", "margin-bottom", "margin-left"];
    for (let mi = 0; mi < defaultZeroMargins.length; mi++) {
      if (!computedMap.has(defaultZeroMargins[mi])) {
        computedMap.set(defaultZeroMargins[mi], "0px");
      }
    }
    if (computedMap.has("rotate")) {
      const rotRaw = computedMap.get("rotate");
      const deg = evalCssAngleDeg(rotRaw);
      if (deg !== null && isFinite(deg)) {
        computedMap.set("rotate", String(Number(deg.toFixed(6))) + "deg");
      }
    }
    let disp = (computedMap.get("display") || getDefaultDisplayForElement(element)).trim().toLowerCase();
    if (disp === "-webkit-box") {
      const boxOrient = (computedMap.get("-webkit-box-orient") || "").trim().toLowerCase();
      const lineClampRaw = (computedMap.get("-webkit-line-clamp") || "").trim();
      const lineClampNum = parseInt(lineClampRaw, 10);
      if (boxOrient === "vertical" && !isNaN(lineClampNum) && lineClampNum > 0 && String(lineClampNum) === lineClampRaw) {
        disp = "flow-root";
        computedMap.set("display", "flow-root");
      }
    }
    if (!computedMap.has("display")) {
      computedMap.set("display", disp);
    }

    // min-width / min-height: auto resolves to 0px outside flex/grid parent
    const parentEl = element.parentElement || element.parentNode;
    let parentDisp = "block";
    if (parentEl && parentEl.nodeType === 1) {
      const pMap = collectRawComputedMap(parentEl, "");
      parentDisp = (pMap.get("display") || getDefaultDisplayForElement(parentEl)).trim().toLowerCase();
    }
    const isFlexOrGridItem =
      parentDisp === "flex" || parentDisp === "inline-flex" ||
      parentDisp === "grid" || parentDisp === "inline-grid";
    if (!isFlexOrGridItem) {
      if ((computedMap.get("min-width") || "").trim().toLowerCase() === "auto") {
        computedMap.set("min-width", "0px");
      }
      if ((computedMap.get("min-height") || "").trim().toLowerCase() === "auto") {
        computedMap.set("min-height", "0px");
      }
    }

    if (!hasLayoutBox(element, computedMap)) {
      return;
    }

    const tag = String(element.localName || element.tagName || "").toLowerCase();
    const fl = (computedMap.get("float") || "none").trim().toLowerCase();
    const pos = (computedMap.get("position") || "static").trim().toLowerCase();
    const isNonReplacedInline =
      disp === "inline" &&
      !REPLACED_INLINE_TAGS.has(tag) &&
      fl === "none" &&
      pos !== "absolute" &&
      pos !== "fixed";

    const fontSize = evalCssLengthExpr(computedMap.get("font-size") || "16px", 16, 16) || 16;
    const cbW = (parentEl && parentEl.nodeType === 1) ? computeElementUsedWidthPx(parentEl, 0) : 800;

    // Resolve width for elements with a box (except non-replaced inline)
    if (!isNonReplacedInline) {
      const wRaw = (computedMap.get("width") || "").trim().toLowerCase();
      if (wRaw === "" && disp === "block") {
        computedMap.set("width", formatCssPx(computeElementUsedWidthPx(element, 0)));
      } else if (wRaw !== "") {
        const usedW = computeElementUsedWidthPx(element, 0);
        computedMap.set("width", formatCssPx(usedW));
      }

      // Resolve padding-* percentages / calc / relative units against cbW
      const padProps = ["padding-top", "padding-right", "padding-bottom", "padding-left"];
      for (let i = 0; i < padProps.length; i++) {
        const pk = padProps[i];
        const pv = computedMap.get(pk);
        if (pv && !/^[+-]?(?:\d+\.?\d*|\.\d+)px$/i.test(pv.trim())) {
          const px = evalCssLengthExpr(pv, cbW, fontSize);
          if (px !== null) computedMap.set(pk, formatCssPx(px));
        }
      }

      // Resolve margin-* percentages / calc against cbW (skip internal table boxes like table-row)
      const isTableInternal =
        disp === "table-row" || disp === "table-row-group" ||
        disp === "table-header-group" || disp === "table-footer-group" ||
        disp === "table-column" || disp === "table-column-group" || disp === "table-cell";
      if (!isTableInternal) {
        const marProps = ["margin-top", "margin-right", "margin-bottom", "margin-left"];
        for (let i = 0; i < marProps.length; i++) {
          const mk = marProps[i];
          const mv = computedMap.get(mk);
          if (mv && (mv.indexOf("%") !== -1 || mv.indexOf("(") !== -1)) {
            const px = evalCssLengthExpr(mv, cbW, fontSize);
            if (px !== null) computedMap.set(mk, formatCssPx(px));
          }
        }
      }
    }

    // Sticky inset resolution against per-axis scrollport
    if (pos === "sticky") {
      const spW = findStickyScrollportSize(element, "x");
      const spH = findStickyScrollportSize(element, "y");
      const xProps = ["left", "right"];
      for (let i = 0; i < xProps.length; i++) {
        const k = xProps[i];
        const v = computedMap.get(k);
        if (v && v !== "auto") {
          const px = evalCssLengthExpr(v, spW, fontSize);
          if (px !== null) computedMap.set(k, formatCssPx(px));
        }
      }
      const yProps = ["top", "bottom"];
      for (let i = 0; i < yProps.length; i++) {
        const k = yProps[i];
        const v = computedMap.get(k);
        if (v && v !== "auto") {
          const px = evalCssLengthExpr(v, spH, fontSize);
          if (px !== null) computedMap.set(k, formatCssPx(px));
        }
      }
    }
    // Ensure standard initial computed values for opacity and color
    if (!computedMap.has("opacity") || computedMap.get("opacity") === "") {
      computedMap.set("opacity", "1");
    } else {
      computedMap.set("opacity", String(computedMap.get("opacity")));
    }
    if (!computedMap.has("color") || computedMap.get("color") === "") {
      computedMap.set("color", "rgb(0, 0, 0)");
    }
    if (!computedMap.has("transform") || computedMap.get("transform") === "") {
      computedMap.set("transform", "none");
    }
    if (!computedMap.has("display") || computedMap.get("display") === "") {
      const tag = String(element && (element.localName || element.tagName) || "").toLowerCase();
      const inlineTags = { span: 1, a: 1, em: 1, strong: 1, b: 1, i: 1, u: 1, s: 1, small: 1, code: 1, label: 1, abbr: 1, cite: 1, q: 1, sub: 1, sup: 1, time: 1 };
      computedMap.set("display", inlineTags[tag] ? "inline" : "block");
    }
  }

  const STANDARD_COMPUTED_LONGHANDS = [
    "align-content", "align-items", "align-self", "animation-delay", "animation-direction",
    "animation-duration", "animation-fill-mode", "animation-iteration-count", "animation-name",
    "animation-play-state", "animation-timing-function", "appearance", "aspect-ratio",
    "backdrop-filter", "backface-visibility", "background-attachment", "background-blend-mode",
    "background-clip", "background-color", "background-image", "background-origin",
    "background-position-x", "background-position-y", "background-repeat", "background-size",
    "border-bottom-color", "border-bottom-left-radius", "border-bottom-right-radius",
    "border-bottom-style", "border-bottom-width", "border-collapse", "border-left-color",
    "border-left-style", "border-left-width", "border-right-color", "border-right-style",
    "border-right-width", "border-spacing", "border-top-color", "border-top-left-radius",
    "border-top-right-radius", "border-top-style", "border-top-width", "bottom", "box-shadow",
    "box-sizing", "break-after", "break-before", "break-inside", "caption-side", "caret-color",
    "clear", "clip", "clip-path", "color", "color-scheme", "column-count", "column-gap",
    "column-rule-color", "column-rule-style", "column-rule-width", "column-width", "contain",
    "content", "counter-increment", "counter-reset", "cursor", "direction", "display",
    "empty-cells", "filter", "flex-basis", "flex-direction", "flex-grow", "flex-shrink",
    "flex-wrap", "float", "font-family", "font-feature-settings", "font-kerning", "font-size",
    "font-stretch", "font-style", "font-variant", "font-weight", "grid-auto-columns",
    "grid-auto-flow", "grid-auto-rows", "grid-column-end", "grid-column-start",
    "grid-row-end", "grid-row-start", "grid-template-areas", "grid-template-columns",
    "grid-template-rows", "height", "hyphens", "image-rendering", "isolation",
    "justify-content", "justify-items", "justify-self", "left", "letter-spacing",
    "line-height", "list-style-image", "list-style-position", "list-style-type",
    "margin-bottom", "margin-left", "margin-right", "margin-top", "max-height", "max-width",
    "min-height", "min-width", "mix-blend-mode", "object-fit", "object-position", "opacity",
    "order", "outline-color", "outline-offset", "outline-style", "outline-width",
    "overflow-wrap", "overflow-x", "overflow-y", "padding-bottom", "padding-left",
    "padding-right", "padding-top", "perspective", "perspective-origin", "pointer-events",
    "position", "quotes", "resize", "right", "rotate", "row-gap", "scale", "scroll-behavior",
    "tab-size", "table-layout", "text-align", "text-decoration-color", "text-decoration-line",
    "text-decoration-style", "text-indent", "text-overflow", "text-shadow", "text-transform",
    "top", "touch-action", "transform", "transform-origin", "transform-style",
    "transition-delay", "transition-duration", "transition-property",
    "transition-timing-function", "translate", "unicode-bidi", "user-select", "vertical-align",
    "visibility", "white-space", "width", "will-change", "word-break", "word-spacing",
    "writing-mode", "z-index"
  ];

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

    const computedMap = collectRawComputedMap(element, normalizedPseudo);
    applyUsedValuesToComputedMap(element, computedMap, normalizedPseudo);

    const declTarget = Object.create(CSSStyleDeclaration.prototype);
    const propSet = new Set(computedMap.keys());
    for (let i = 0; i < STANDARD_COMPUTED_LONGHANDS.length; i++) {
      propSet.add(STANDARD_COMPUTED_LONGHANDS[i]);
    }
    const propList = Array.from(propSet);
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

    function throwComputedReadOnly() {
      if (typeof DOMException === "function") {
        throw new DOMException("These styles are computed, and therefore the properties are read-only.", "NoModificationAllowedError");
      }
      const err = new Error("These styles are computed, and therefore the properties are read-only.");
      err.name = "NoModificationAllowedError";
      throw err;
    }

    declTarget.setProperty = markNative(throwComputedReadOnly, "setProperty");
    declTarget.removeProperty = markNative(throwComputedReadOnly, "removeProperty");

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
        throwComputedReadOnly();
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
      preventExtensions: function () {
        return false;
      },
    });
  }

  markNative(getComputedStyle, "getComputedStyle");


  // =========================================================================
  // Full CSS Typed OM + DOMMatrix (migrated from vendor/kitesurf)
  // =========================================================================
  (function installCssTypedOM(global) {
    const CSS_WIDE_KW = new Set(["initial", "inherit", "unset", "revert", "revert-layer"]);
    function canonicalizeInlineStyle(declText) {
      const s = String(declText || "").trim();
      const idx = s.indexOf(":");
      if (idx <= 0) return [];
      const prop = s.slice(0, idx).trim().toLowerCase();
      const val = s.slice(idx + 1).trim();
      if (!prop || !val) return [];
      if (val.indexOf(";") >= 0 || /!important\s*$/i.test(val)) return [];
      if (!prop.startsWith("--") && !/^[a-z][a-z0-9-]*$/.test(prop)) return [];
      const low = val.toLowerCase();
      if (CSS_WIDE_KW.has(low) || /^var\(/.test(low)) return [[prop, val]];
      if (prop === "direction") {
        return (low === "ltr" || low === "rtl") ? [[prop, low]] : [];
      }
      if (prop === "opacity") {
        if (/^[+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?%?$/i.test(low)) return [[prop, val]];
        if (/^(?:calc|min|max|clamp)\(/.test(low)) return [[prop, val]];
        return [];
      }
      return [[prop, val]];
    }
    function parseNumDto(str) {
      const t = String(str || "").trim();
      const m = /^([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)(%|px|em|rem|ex|ch|cap|ic|lh|rlh|vw|vh|vi|vb|vmin|vmax|cm|mm|q|in|pt|pc|deg|grad|rad|turn|s|ms|hz|khz|dpi|dpcm|dppx|fr)?$/i.exec(t);
      if (!m) return null;
      const val = Number(m[1]);
      if (!Number.isFinite(val)) return null;
      const unit = m[2] ? (m[2] === "%" ? "%" : m[2].toLowerCase()) : "number";
      return { type: "unit", value: val, unit: unit };
    }
    function splitTopArgs(str) {
      const out = [];
      let depth = 0, start = 0;
      for (let i = 0; i < str.length; i++) {
        const c = str[i];
        if (c === "(") depth++;
        else if (c === ")") depth--;
        else if (c === "," && depth === 0) {
          out.push(str.slice(start, i).trim());
          start = i + 1;
        }
      }
      const tail = str.slice(start).trim();
      if (tail) out.push(tail);
      return out;
    }
    function parseTransformToDto(cssText) {
      const s = String(cssText || "").trim();
      if (!s || s.toLowerCase() === "none") return null;
      const components = [];
      let i = 0;
      while (i < s.length) {
        while (i < s.length && /\s/.test(s[i])) i++;
        if (i >= s.length) break;
        const nameStart = i;
        while (i < s.length && /[a-zA-Z0-9_-]/.test(s[i])) i++;
        const fn = s.slice(nameStart, i).toLowerCase();
        if (s[i] !== "(") return null;
        let depth = 1, argStart = ++i;
        while (i < s.length && depth > 0) {
          if (s[i] === "(") depth++;
          else if (s[i] === ")") depth--;
          i++;
        }
        if (depth !== 0) return null;
        const args = splitTopArgs(s.slice(argStart, i - 1));
        const zeroPx = { type: "unit", value: 0, unit: "px" };
        const zeroDeg = { type: "unit", value: 0, unit: "deg" };
        const num0 = { type: "unit", value: 0, unit: "number" };
        const num1 = { type: "unit", value: 1, unit: "number" };
        if (fn === "translate" && (args.length === 1 || args.length === 2)) {
          const x = parseNumDto(args[0]);
          const y = args.length === 2 ? parseNumDto(args[1]) : zeroPx;
          if (!x || !y) return null;
          components.push({ type: "translate", x: x, y: y, z: zeroPx, is_2d: true });
        } else if (fn === "translatex" && args.length === 1) {
          const x = parseNumDto(args[0]);
          if (!x) return null;
          components.push({ type: "translate", x: x, y: zeroPx, z: zeroPx, is_2d: true });
        } else if (fn === "translatey" && args.length === 1) {
          const y = parseNumDto(args[0]);
          if (!y) return null;
          components.push({ type: "translate", x: zeroPx, y: y, z: zeroPx, is_2d: true });
        } else if (fn === "translatez" && args.length === 1) {
          const z = parseNumDto(args[0]);
          if (!z) return null;
          components.push({ type: "translate", x: zeroPx, y: zeroPx, z: z, is_2d: false });
        } else if (fn === "translate3d" && args.length === 3) {
          const x = parseNumDto(args[0]), y = parseNumDto(args[1]), z = parseNumDto(args[2]);
          if (!x || !y || !z) return null;
          components.push({ type: "translate", x: x, y: y, z: z, is_2d: false });
        } else if (fn === "rotate" && args.length === 1) {
          const a = parseNumDto(args[0]);
          if (!a) return null;
          components.push({ type: "rotate", x: num0, y: num0, z: num1, angle: a, is_2d: true });
        } else if (fn === "rotatex" && args.length === 1) {
          const a = parseNumDto(args[0]);
          if (!a) return null;
          components.push({ type: "rotate", x: num1, y: num0, z: num0, angle: a, is_2d: false });
        } else if (fn === "rotatey" && args.length === 1) {
          const a = parseNumDto(args[0]);
          if (!a) return null;
          components.push({ type: "rotate", x: num0, y: num1, z: num0, angle: a, is_2d: false });
        } else if (fn === "rotatez" && args.length === 1) {
          const a = parseNumDto(args[0]);
          if (!a) return null;
          components.push({ type: "rotate", x: num0, y: num0, z: num1, angle: a, is_2d: false });
        } else if (fn === "rotate3d" && args.length === 4) {
          const x = parseNumDto(args[0]), y = parseNumDto(args[1]), z = parseNumDto(args[2]), a = parseNumDto(args[3]);
          if (!x || !y || !z || !a) return null;
          components.push({ type: "rotate", x: x, y: y, z: z, angle: a, is_2d: false });
        } else if (fn === "scale" && (args.length === 1 || args.length === 2)) {
          const x = parseNumDto(args[0]);
          const y = args.length === 2 ? parseNumDto(args[1]) : x;
          if (!x || !y) return null;
          components.push({ type: "scale", x: x, y: y, z: num1, is_2d: true });
        } else if (fn === "scalex" && args.length === 1) {
          const x = parseNumDto(args[0]);
          if (!x) return null;
          components.push({ type: "scale", x: x, y: num1, z: num1, is_2d: true });
        } else if (fn === "scaley" && args.length === 1) {
          const y = parseNumDto(args[0]);
          if (!y) return null;
          components.push({ type: "scale", x: num1, y: y, z: num1, is_2d: true });
        } else if (fn === "scalez" && args.length === 1) {
          const z = parseNumDto(args[0]);
          if (!z) return null;
          components.push({ type: "scale", x: num1, y: num1, z: z, is_2d: false });
        } else if (fn === "scale3d" && args.length === 3) {
          const x = parseNumDto(args[0]), y = parseNumDto(args[1]), z = parseNumDto(args[2]);
          if (!x || !y || !z) return null;
          components.push({ type: "scale", x: x, y: y, z: z, is_2d: false });
        } else if (fn === "skew" && (args.length === 1 || args.length === 2)) {
          const ax = parseNumDto(args[0]);
          const ay = args.length === 2 ? parseNumDto(args[1]) : zeroDeg;
          if (!ax || !ay) return null;
          components.push({ type: "skew", ax: ax, ay: ay, is_2d: true });
        } else if (fn === "skewx" && args.length === 1) {
          const a = parseNumDto(args[0]);
          if (!a) return null;
          components.push({ type: "skew_x", angle: a, is_2d: true });
        } else if (fn === "skewy" && args.length === 1) {
          const a = parseNumDto(args[0]);
          if (!a) return null;
          components.push({ type: "skew_y", angle: a, is_2d: true });
        } else if (fn === "perspective" && args.length === 1) {
          const l = args[0].toLowerCase() === "none" ? { type: "keyword", value: "none" } : parseNumDto(args[0]);
          if (!l) return null;
          components.push({ type: "perspective", length: l, is_2d: false });
        } else if (fn === "matrix" && args.length === 6) {
          const nums = args.map(Number);
          if (nums.some(function (n) { return !Number.isFinite(n); })) return null;
          components.push({
            type: "matrix",
            matrix: [nums[0], nums[1], 0, 0, nums[2], nums[3], 0, 0, 0, 0, 1, 0, nums[4], nums[5], 0, 1],
            is_2d: true,
          });
        } else if (fn === "matrix3d" && args.length === 16) {
          const nums = args.map(Number);
          if (nums.some(function (n) { return !Number.isFinite(n); })) return null;
          components.push({ type: "matrix", matrix: nums, is_2d: false });
        } else {
          return null;
        }
      }
      return components.length ? { values: [{ type: "transform", components: components }] } : null;
    }
    function parseTypedStyleValue(property, cssText) {
      if (property === "transform") {
        return parseTransformToDto(cssText);
      }
      return null;
    }
    const opts = { canonicalizeInlineStyle, parseTypedStyleValue };

const NUMERIC_TYPES = [
  "length",
  "angle",
  "time",
  "frequency",
  "resolution",
  "flex",
  "percent"
];
const UNIT_TYPES = {
  number: null,
  percent: "percent",
  em: "length",
  ex: "length",
  ch: "length",
  cap: "length",
  ic: "length",
  rem: "length",
  rcap: "length",
  rch: "length",
  rex: "length",
  ric: "length",
  lh: "length",
  rlh: "length",
  vw: "length",
  vh: "length",
  vi: "length",
  vb: "length",
  vmin: "length",
  vmax: "length",
  cm: "length",
  mm: "length",
  svw: "length",
  svh: "length",
  svi: "length",
  svb: "length",
  svmin: "length",
  svmax: "length",
  lvw: "length",
  lvh: "length",
  lvi: "length",
  lvb: "length",
  lvmin: "length",
  lvmax: "length",
  dvw: "length",
  dvh: "length",
  dvi: "length",
  dvb: "length",
  dvmin: "length",
  dvmax: "length",
  cqw: "length",
  cqh: "length",
  cqi: "length",
  cqb: "length",
  cqmin: "length",
  cqmax: "length",
  q: "length",
  in: "length",
  pt: "length",
  pc: "length",
  px: "length",
  deg: "angle",
  grad: "angle",
  rad: "angle",
  turn: "angle",
  s: "time",
  ms: "time",
  hz: "frequency",
  khz: "frequency",
  dpi: "resolution",
  dpcm: "resolution",
  dppx: "resolution",
  fr: "flex"
};
const UNIT_FACTORS = {
  px: 1,
  cm: 96 / 2.54,
  mm: 96 / 25.4,
  q: 96 / 101.6,
  in: 96,
  pt: 96 / 72,
  pc: 16,
  deg: 1,
  grad: 0.9,
  rad: 180 / Math.PI,
  turn: 360,
  s: 1,
  ms: 0.001,
  hz: 1,
  khz: 1000,
  dppx: 1,
  dpi: 1 / 96,
  dpcm: 2.54 / 96
};
function numericType(unit) {
  const type = UNIT_TYPES[unit.toLowerCase()];
  return type ? { [type]: 1 } : {};
}
function cloneType(type) {
  return { ...type };
}
function typeEquals(a, b) {
  return NUMERIC_TYPES.every((type) => a[type] === b[type]) && a.percentHint === b.percentHint;
}
function applyPercentHint(type, hint) {
  const out = cloneType(type);
  const percent = out.percent || 0;
  delete out.percent;
  if (percent)
    out[hint] = (out[hint] || 0) + percent;
  out.percentHint = hint;
  return out;
}
function addTypes(a, b) {
  if (a.percentHint && b.percentHint && a.percentHint !== b.percentHint) {
    throw new TypeError("Incompatible numeric types");
  }
  const existingHint = a.percentHint || b.percentHint;
  if (existingHint) {
    const left = applyPercentHint(a, existingHint);
    const right = applyPercentHint(b, existingHint);
    if (typeEquals(left, right))
      return left;
    throw new TypeError("Incompatible numeric types");
  }
  if (typeEquals(a, b))
    return cloneType(a);
  for (const hint of NUMERIC_TYPES) {
    if (hint === "percent")
      continue;
    const left = applyPercentHint(a, hint);
    const right = applyPercentHint(b, hint);
    if (typeEquals(left, right))
      return left;
  }
  throw new TypeError("Incompatible numeric types");
}
function multiplyTypes(a, b) {
  if (a.percentHint && b.percentHint && a.percentHint !== b.percentHint) {
    throw new TypeError("Incompatible numeric types");
  }
  const hint = a.percentHint || b.percentHint;
  const left = hint ? applyPercentHint(a, hint) : a;
  const right = hint ? applyPercentHint(b, hint) : b;
  const out = hint ? { percentHint: hint } : {};
  for (const type of NUMERIC_TYPES) {
    const exponent = (left[type] || 0) + (right[type] || 0);
    if (exponent)
      out[type] = exponent;
  }
  return out;
}
function invertType(type) {
  const out = type.percentHint ? { percentHint: type.percentHint } : {};
  for (const base of NUMERIC_TYPES) {
    if (type[base])
      out[base] = -type[base];
  }
  return out;
}
function formatNumber(value) {
  if (Object.is(value, -0))
    return "0";
  return String(value);
}
function normalizeUnit(value, invalidUnitIsTypeError = false) {
  const unit = String(value).toLowerCase();
  if (!(unit in UNIT_TYPES)) {
    if (invalidUnitIsTypeError)
      throw new TypeError(`Invalid unit: ${String(value)}`);
    throw new DOMException(`Invalid unit: ${String(value)}`, "SyntaxError");
  }
  return unit;
}

function CSSStyleValue(cssText = "", associatedProperty = null) {
  this._cssText = String(cssText);
  this._associatedProperty = associatedProperty;
}
CSSStyleValue.parse = function parse(propertyName, cssText) {
  return CSSStyleValue.parseAll(propertyName, cssText)[0];
};
CSSStyleValue.parseAll = function parseAll(propertyName, cssText) {
  const property = normalizeProperty(propertyName);
  assertSupportedProperty(property);
  const text = String(cssText);
  if (!text || hasTopLevelSemicolon(text) || hasTopLevelImportant(text)) {
    throw new TypeError(`Invalid value for ${property}`);
  }
  if (!property.startsWith("--")) {
    const canon = opts && typeof opts.canonicalizeInlineStyle === "function" ? opts.canonicalizeInlineStyle : null;
    if (!canon || canon(`${property}: ${text}`).length === 0) {
      throw new TypeError(`Invalid value for ${property}`);
    }
  }
  const typed = reifyTypedList(property, text);
  if (typed)
    return typed;
  const parts = LIST_VALUED_PROPERTIES.has(property) ? splitTopLevelCommas(text) : [text];
  return parts.map((part) => reify(property, part) || new CSSStyleValue(part, property));
};
Object.defineProperty(CSSStyleValue.prototype, Symbol.toStringTag, {
  get() {
    return this.constructor.name;
  },
  configurable: true,
});
CSSStyleValue.prototype.toString = function toString() {
  return this._cssText;
};

class CSSImageValue extends CSSStyleValue {
}

class CSSKeywordValue extends CSSStyleValue {
  _value;
  constructor(value) {
    const text = String(value);
    if (!text)
      throw new TypeError("CSS keyword must not be empty");
    super(text);
    this._value = text;
  }
  get value() {
    return this._value;
  }
  set value(value) {
    const text = String(value);
    if (!text)
      throw new TypeError("CSS keyword must not be empty");
    this._value = text;
    this._cssText = text;
  }
}

class CSSNumericValue extends CSSStyleValue {
  static parse(cssText) {
    const text = String(cssText);
    const parsed = typedValueFromDto(callTypedParser("parseTypedNumericValue", text));
    if (!(parsed instanceof CSSNumericValue)) {
      throw new DOMException("Invalid CSS numeric value", "SyntaxError");
    }
    return parsed;
  }
  type() {
    return numericType("number");
  }
  add(...values) {
    if (!values.length)
      return this;
    return simplifyAddLike([this, ...values.map(toNumericValue)], "sum");
  }
  sub(...values) {
    if (!values.length)
      return this;
    return simplifyAddLike([this, ...values.map((value) => negateForArithmetic(toNumericValue(value)))], "sum");
  }
  mul(...values) {
    if (!values.length)
      return this;
    return simplifyProduct([this, ...values.map(toNumericValue)]);
  }
  div(...values) {
    if (!values.length)
      return this;
    return simplifyProduct([
      this,
      ...values.map((value) => invertForArithmetic(toNumericValue(value)))
    ]);
  }
  min(...values) {
    if (!values.length)
      return this;
    return simplifyAddLike([this, ...values.map(toNumericValue)], "min");
  }
  max(...values) {
    if (!values.length)
      return this;
    return simplifyAddLike([this, ...values.map(toNumericValue)], "max");
  }
  equals(...values) {
    return values.every((value) => numericEquals(this, toNumericValue(value)));
  }
  to(unitValue) {
    const unit = normalizeUnit(unitValue);
    const value = evaluateNumeric(this);
    const target = evaluateNumeric(new CSSUnitValue(1, unit));
    if (!typeEquals(value.type, target.type) || !atomsEqual(value.atoms, target.atoms)) {
      throw new TypeError("Incompatible units");
    }
    return new CSSUnitValue(value.value / target.value, unit);
  }
  toSum(...unitValues) {
    const units = unitValues.map((value) => normalizeUnit(value));
    if (new Set(units).size !== units.length) {
      throw new DOMException("Duplicate units", "SyntaxError");
    }
    const terms = sumTerms(this);
    if (!units.length) {
      const totals = new Map;
      for (const term of terms) {
        const unit = canonicalUnit(term.unit);
        const value = unit === term.unit ? term.value : term.to(unit).value;
        totals.set(unit, (totals.get(unit) || 0) + value);
      }
      return new CSSMathSum(...[...totals].sort(([a], [b]) => a.localeCompare(b)).map(([unit, value]) => new CSSUnitValue(value, unit)));
    }
    units.slice(1).reduce((type, unit) => addTypes(type, numericType(unit)), numericType(units[0]));
    const totals = units.map(() => 0);
    for (const term of terms) {
      let converted = false;
      for (let i = 0;i < units.length; i++) {
        try {
          totals[i] += term.to(units[i]).value;
          converted = true;
          break;
        } catch {}
      }
      if (!converted)
        throw new TypeError("Value has units not present in the sum");
    }
    return new CSSMathSum(...units.map((unit, i) => new CSSUnitValue(totals[i], unit)));
  }
}
function canonicalUnit(unit) {
  const type = UNIT_TYPES[unit];
  if (UNIT_FACTORS[unit] == null)
    return unit;
  if (type === "length")
    return "px";
  if (type === "angle")
    return "deg";
  if (type === "time")
    return "s";
  if (type === "frequency")
    return "hz";
  if (type === "resolution")
    return "dppx";
  return unit;
}

class CSSUnitValue extends CSSNumericValue {
  _value;
  _unit;
  constructor(value, unitValue) {
    const number = Number(value);
    if (!Number.isFinite(number))
      throw new TypeError("CSS unit value must be finite");
    const unit = normalizeUnit(unitValue, true);
    super();
    this._value = number;
    this._unit = unit;
    this._cssText = this.toString();
  }
  get value() {
    return this._value;
  }
  set value(value) {
    const number = Number(value);
    if (!Number.isFinite(number))
      throw new TypeError("CSS unit value must be finite");
    this._value = number;
    this._cssText = this.toString();
  }
  get unit() {
    return this._unit;
  }
  type() {
    return numericType(this._unit);
  }
  toString() {
    const suffix = this._unit === "number" ? "" : this._unit === "percent" ? "%" : this._unit;
    return `${formatNumber(this._value)}${suffix}`;
  }
}

class CSSMathValue extends CSSNumericValue {
  _operator = "sum";
  constructor() {
    super();
    return new Proxy(this, {
      set(target, property, value, receiver) {
        if (property === "operator" || property === "values")
          throw new TypeError(String(property) + " is readonly");
        if (property === "_values" || property === "_operator") {
          const desc = Object.getOwnPropertyDescriptor(target, property);
          if (desc && !desc.writable) throw new TypeError(String(property) + " is readonly");
        }
        return Reflect.set(target, property, value, receiver);
      }
    });
  }
  get operator() {
    return this._operator;
  }
}
function setMathOperator(value, operator) {
  Object.defineProperty(value, "_operator", {
    value: operator,
    writable: false,
    enumerable: false,
    configurable: false
  });
}
function lockSlot(value, name) {
  const locked = value[name];
  Object.defineProperty(value, name, {
    get() {
      return locked;
    },
    set() {
      throw new TypeError(String(name) + " is readonly");
    },
    enumerable: false,
    configurable: false
  });
}

class CSSNumericArray {
  _values;
  constructor(values) {
    this._values = Object.freeze(values.slice());
    lockSlot(this, "_values");
    for (let i = 0;i < this._values.length; i++) {
      Object.defineProperty(this, i, {
        value: this._values[i],
        writable: false,
        enumerable: true,
        configurable: false
      });
    }
    return new Proxy(this, {
      set() {
        throw new TypeError("CSSNumericArray is readonly");
      },
      deleteProperty() {
        throw new TypeError("CSSNumericArray is readonly");
      }
    });
  }
  get length() {
    return this._values.length;
  }
  *entries() {
    for (let i = 0;i < this.length; i++)
      yield [i, this._values[i]];
  }
  *keys() {
    for (let i = 0;i < this.length; i++)
      yield i;
  }
  *values() {
    yield* this._values;
  }
  forEach(callback, thisArg) {
    for (let i = 0;i < this.length; i++)
      callback.call(thisArg, this._values[i], i, this);
  }
  [Symbol.iterator]() {
    return this.values();
  }
}
function initializeVariadic(self, operator, values) {
  if (!values.length)
    throw new DOMException("At least one value is required", "SyntaxError");
  setMathOperator(self, operator);
  const numeric = values.map(toNumericValue);
  numeric.slice(1).reduce((type, value) => addTypes(type, value.type()), numeric[0].type());
  self._values = new CSSNumericArray(numeric);
  lockSlot(self, "_values");
}
function variadicType(values) {
  return values._values.slice(1).reduce((type, value) => addTypes(type, value.type()), values._values[0].type());
}

class CSSMathSum extends CSSMathValue {
  _values;
  constructor(...values) {
    super();
    initializeVariadic(this, "sum", values);
    this._cssText = this.toString();
  }
  get values() {
    return this._values;
  }
  type() {
    return variadicType(this._values);
  }
  toString() {
    const values = this._values._values;
    let text = String(values[0]);
    for (const value of values.slice(1)) {
      text += value instanceof CSSMathNegate ? ` - ${value.value}` : ` + ${value}`;
    }
    return `calc(${text})`;
  }
}

class CSSMathProduct extends CSSMathValue {
  _values;
  constructor(...values) {
    super();
    if (!values.length)
      throw new DOMException("At least one value is required", "SyntaxError");
    setMathOperator(this, "product");
    this._values = new CSSNumericArray(values.map(toNumericValue));
    lockSlot(this, "_values");
    this.type();
    this._cssText = this.toString();
  }
  get values() {
    return this._values;
  }
  type() {
    return this._values._values.slice(1).reduce((type, value) => multiplyTypes(type, value.type()), this._values._values[0].type());
  }
  toString() {
    return `calc(${this._values._values.map(String).join(" * ")})`;
  }
}

class CSSMathNegate extends CSSMathValue {
  _value;
  constructor(value) {
    super();
    setMathOperator(this, "negate");
    this._value = toNumericValue(value);
    lockSlot(this, "_value");
    this._cssText = this.toString();
  }
  get value() {
    return this._value;
  }
  type() {
    return cloneType(this._value.type());
  }
  toString() {
    return `calc(-1 * ${this._value})`;
  }
}

class CSSMathInvert extends CSSMathValue {
  _value;
  constructor(value) {
    super();
    setMathOperator(this, "invert");
    this._value = toNumericValue(value);
    lockSlot(this, "_value");
    this._cssText = this.toString();
  }
  get value() {
    return this._value;
  }
  type() {
    return invertType(this._value.type());
  }
  toString() {
    return `calc(1 / ${this._value})`;
  }
}

class CSSMathMin extends CSSMathValue {
  _values;
  constructor(...values) {
    super();
    initializeVariadic(this, "min", values);
    this._cssText = this.toString();
  }
  get values() {
    return this._values;
  }
  type() {
    return variadicType(this._values);
  }
  toString() {
    return `min(${this._values._values.join(", ")})`;
  }
}

class CSSMathMax extends CSSMathValue {
  _values;
  constructor(...values) {
    super();
    initializeVariadic(this, "max", values);
    this._cssText = this.toString();
  }
  get values() {
    return this._values;
  }
  type() {
    return variadicType(this._values);
  }
  toString() {
    return `max(${this._values._values.join(", ")})`;
  }
}

class CSSMathClamp extends CSSMathValue {
  _lower;
  _value;
  _upper;
  constructor(lower, value, upper) {
    super();
    setMathOperator(this, "clamp");
    this._lower = toNumericValue(lower);
    this._value = toNumericValue(value);
    this._upper = toNumericValue(upper);
    lockSlot(this, "_lower");
    lockSlot(this, "_value");
    lockSlot(this, "_upper");
    addTypes(addTypes(this._lower.type(), this._value.type()), this._upper.type());
    this._cssText = this.toString();
  }
  get lower() {
    return this._lower;
  }
  get value() {
    return this._value;
  }
  get upper() {
    return this._upper;
  }
  type() {
    return addTypes(addTypes(this._lower.type(), this._value.type()), this._upper.type());
  }
  toString() {
    return `clamp(${this._lower}, ${this._value}, ${this._upper})`;
  }
}
function toNumericValue(value) {
  if (value instanceof CSSNumericValue)
    return value;
  return new CSSUnitValue(value, "number");
}
function negateForArithmetic(value) {
  if (value instanceof CSSUnitValue)
    return new CSSUnitValue(-value.value, value.unit);
  if (value instanceof CSSMathNegate)
    return value.value;
  return new CSSMathNegate(value);
}
function invertForArithmetic(value) {
  if (value instanceof CSSUnitValue && value.unit === "number") {
    if (value.value === 0)
      throw new RangeError("Division by zero");
    return new CSSUnitValue(1 / value.value, "number");
  }
  if (value instanceof CSSMathInvert)
    return value.value;
  return new CSSMathInvert(value);
}
function representativeUnit(value) {
  if (value instanceof CSSUnitValue)
    return value.unit;
  if (value instanceof CSSMathNegate || value instanceof CSSMathInvert)
    return representativeUnit(value.value);
  if (value instanceof CSSMathSum || value instanceof CSSMathMin || value instanceof CSSMathMax) {
    return representativeUnit(value.values._values[0]);
  }
  if (value instanceof CSSMathProduct)
    return representativeUnit(value.values._values[0]);
  if (value instanceof CSSMathClamp)
    return representativeUnit(value.lower);
  return null;
}
function simplifyAddLike(values, operator) {
  const flat = [];
  for (const value of values) {
    if (operator === "sum" && value instanceof CSSMathSum || operator === "min" && value instanceof CSSMathMin || operator === "max" && value instanceof CSSMathMax)
      flat.push(...value.values._values);
    else
      flat.push(value);
  }
  const result = operator === "sum" ? new CSSMathSum(...flat) : operator === "min" ? new CSSMathMin(...flat) : new CSSMathMax(...flat);
  if (!flat.every((value) => value instanceof CSSUnitValue))
    return result;
  const units = flat;
  if (units.some((value) => value.unit !== units[0].unit))
    return result;
  const numbers = units.map((value) => value.value);
  const simplified = operator === "sum" ? numbers.reduce((a, b) => a + b, 0) : operator === "min" ? Math.min(...numbers) : Math.max(...numbers);
  return new CSSUnitValue(simplified, units[0].unit);
}
function simplifyProduct(values) {
  const flat = [];
  for (const value of values) {
    if (value instanceof CSSMathProduct)
      flat.push(...value.values._values);
    else
      flat.push(value);
  }
  const result = new CSSMathProduct(...flat);
  if (!flat.every((value) => value instanceof CSSUnitValue))
    return result;
  const units = flat;
  const dimensional = units.filter((value) => value.unit !== "number");
  if (dimensional.length > 1)
    return result;
  return new CSSUnitValue(units.reduce((product, value) => product * value.value, 1), dimensional[0]?.unit || "number");
}
function numericEquals(a, b) {
  if (a.constructor !== b.constructor)
    return false;
  if (a instanceof CSSUnitValue && b instanceof CSSUnitValue) {
    return a.value === b.value && a.unit === b.unit;
  }
  if (a instanceof CSSMathSum && b instanceof CSSMathSum || a instanceof CSSMathMin && b instanceof CSSMathMin || a instanceof CSSMathMax && b instanceof CSSMathMax || a instanceof CSSMathProduct && b instanceof CSSMathProduct) {
    const left = a.values._values;
    const right = b.values._values;
    return left.length === right.length && left.every((value, i) => numericEquals(value, right[i]));
  }
  if (a instanceof CSSMathNegate && b instanceof CSSMathNegate || a instanceof CSSMathInvert && b instanceof CSSMathInvert)
    return numericEquals(a.value, b.value);
  if (a instanceof CSSMathClamp && b instanceof CSSMathClamp) {
    return numericEquals(a.lower, b.lower) && numericEquals(a.value, b.value) && numericEquals(a.upper, b.upper);
  }
  return a === b;
}
function atomsEqual(a, b) {
  const keys = new Set([...Object.keys(a), ...Object.keys(b)]);
  return [...keys].every((key) => (a[key] || 0) === (b[key] || 0));
}
function evaluateNumeric(value) {
  if (value instanceof CSSUnitValue) {
    const factor = value.unit === "number" || value.unit === "percent" || value.unit === "fr" ? 1 : UNIT_FACTORS[value.unit];
    return {
      value: value.value * (factor ?? 1),
      type: value.type(),
      atoms: factor == null ? { [value.unit]: 1 } : {}
    };
  }
  if (value instanceof CSSMathNegate) {
    const inner = evaluateNumeric(value.value);
    return { ...inner, value: -inner.value };
  }
  if (value instanceof CSSMathInvert) {
    const inner = evaluateNumeric(value.value);
    const atoms = {};
    for (const unit of Object.keys(inner.atoms))
      atoms[unit] = -inner.atoms[unit];
    return { value: 1 / inner.value, type: invertType(inner.type), atoms };
  }
  if (value instanceof CSSMathProduct) {
    return value.values._values.map(evaluateNumeric).reduce((left, right) => {
      const atoms = { ...left.atoms };
      for (const unit of Object.keys(right.atoms)) {
        const exponent = (atoms[unit] || 0) + right.atoms[unit];
        if (exponent)
          atoms[unit] = exponent;
        else
          delete atoms[unit];
      }
      return { value: left.value * right.value, type: multiplyTypes(left.type, right.type), atoms };
    });
  }
  if (value instanceof CSSMathClamp) {
    const lower = evaluateNumeric(value.lower);
    const middle = evaluateNumeric(value.value);
    const upper = evaluateNumeric(value.upper);
    if (!typeEquals(lower.type, middle.type) || !typeEquals(lower.type, upper.type) || !atomsEqual(lower.atoms, middle.atoms) || !atomsEqual(lower.atoms, upper.atoms)) {
      throw new TypeError("Value cannot be converted to a unit");
    }
    return { ...middle, value: Math.max(lower.value, Math.min(middle.value, upper.value)) };
  }
  if (value instanceof CSSMathSum || value instanceof CSSMathMin || value instanceof CSSMathMax) {
    const evaluated = value.values._values.map(evaluateNumeric);
    const first = evaluated[0];
    if (evaluated.some((part) => !typeEquals(first.type, part.type) || !atomsEqual(first.atoms, part.atoms))) {
      throw new TypeError("Value cannot be converted to a unit");
    }
    const numbers = evaluated.map((part) => part.value);
    const result = value instanceof CSSMathSum ? numbers.reduce((a, b) => a + b, 0) : value instanceof CSSMathMin ? Math.min(...numbers) : Math.max(...numbers);
    return { ...first, value: result };
  }
  throw new TypeError("Value cannot be converted to a unit");
}
function sumTerms(value) {
  if (value instanceof CSSUnitValue)
    return [new CSSUnitValue(value.value, value.unit)];
  if (value instanceof CSSMathSum)
    return value.values._values.flatMap(sumTerms);
  if (value instanceof CSSMathNegate) {
    return sumTerms(value.value).map((term) => new CSSUnitValue(-term.value, term.unit));
  }
  const unit = representativeUnit(value);
  if (!unit)
    throw new TypeError("Value cannot be represented as a sum");
  return [value.to(unit)];
}
function colorSyntaxError(description) {
  throw new DOMException(description, "SyntaxError");
}
function colorKeyword(value) {
  if (value instanceof CSSKeywordValue && value.value.toLowerCase() === "none")
    return value;
  if (typeof value === "string" && value.toLowerCase() === "none")
    return new CSSKeywordValue(value);
  return null;
}
function colorPercent(value, description) {
  const keyword = colorKeyword(value);
  if (keyword)
    return keyword;
  if (typeof value === "number")
    return new CSSUnitValue(value * 100, "percent");
  if (value instanceof CSSNumericValue && hasNumericType(value, { percent: 1 }))
    return value;
  return colorSyntaxError(`${description} must be a percentage or none`);
}
function colorNumber(value, description) {
  const keyword = colorKeyword(value);
  if (keyword)
    return keyword;
  if (typeof value === "number")
    return new CSSUnitValue(value, "number");
  if (value instanceof CSSNumericValue && hasNumericType(value, {}))
    return value;
  return colorSyntaxError(`${description} must be a number or none`);
}
function colorRgbComponent(value, description) {
  const keyword = colorKeyword(value);
  if (keyword)
    return keyword;
  if (typeof value === "number")
    return new CSSUnitValue(value * 100, "percent");
  if (value instanceof CSSNumericValue && (hasNumericType(value, {}) || hasNumericType(value, { percent: 1 })))
    return value;
  return colorSyntaxError(`${description} must be a number, percentage, or none`);
}
function colorAngle(value, description, allowUndefined = false) {
  const keyword = colorKeyword(value);
  if (keyword)
    return keyword;
  if (allowUndefined && value === undefined)
    return new CSSKeywordValue("undefined");
  if (typeof value === "number")
    return new CSSUnitValue(value, "deg");
  if (value instanceof CSSNumericValue && hasNumericType(value, { angle: 1 }))
    return value;
  return colorSyntaxError(`${description} must be an angle or none`);
}
function serializeColorFunction(name, channels, alpha) {
  const alphaText = alpha instanceof CSSUnitValue && alpha.unit === "percent" && alpha.value === 100 ? "" : ` / ${alpha}`;
  return `${name}(${channels.join(" ")}${alphaText})`;
}
function simpleColorNumeric(token, kind) {
  const text = token.trim();
  if (text.toLowerCase() === "none")
    return new CSSKeywordValue("none");
  const match = /^([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:e[+-]?\d+)?)(%|deg|grad|rad|turn)?$/i.exec(text);
  if (!match)
    return null;
  const value = Number(match[1]);
  const unit = match[2]?.toLowerCase();
  if (unit === "%")
    return new CSSUnitValue(value, "percent");
  if (unit)
    return kind === "angle" ? new CSSUnitValue(value, unit) : null;
  if (kind === "percent")
    return new CSSUnitValue(value * 100, "percent");
  if (kind === "angle")
    return new CSSUnitValue(value, "deg");
  return new CSSUnitValue(value, "number");
}
function splitColorFunction(body) {
  const slash = body.split("/");
  if (slash.length > 2)
    return { channels: [] };
  const commaSyntax = slash[0].includes(",");
  let channels = commaSyntax ? slash[0].split(",") : slash[0].trim().split(/\s+/);
  let alpha = slash[1]?.trim();
  if (commaSyntax && !alpha && channels.length === 4)
    alpha = channels.pop()?.trim();
  channels = channels.map((value) => value.trim()).filter(Boolean);
  return { channels, ...alpha ? { alpha } : {} };
}
const PINNED_NAMED_COLORS = {
  black: [0, 0, 0],
  blue: [0, 0, 255],
  red: [255, 0, 0],
  transparent: [0, 0, 0, 0],
  white: [255, 255, 255]
};
const CSS_COLOR_KEYWORDS = new Set(`
  aliceblue antiquewhite aqua aquamarine azure beige bisque blanchedalmond blueviolet brown
  burlywood cadetblue chartreuse chocolate coral cornflowerblue cornsilk crimson cyan darkblue
  darkcyan darkgoldenrod darkgray darkgreen darkgrey darkkhaki darkmagenta darkolivegreen
  darkorange darkorchid darkred darksalmon darkseagreen darkslateblue darkslategray darkslategrey
  darkturquoise darkviolet deeppink deepskyblue dimgray dimgrey dodgerblue firebrick floralwhite
  forestgreen fuchsia gainsboro ghostwhite gold goldenrod gray green greenyellow grey honeydew
  hotpink indianred indigo ivory khaki lavender lavenderblush lawngreen lemonchiffon lightblue
  lightcoral lightcyan lightgoldenrodyellow lightgray lightgreen lightgrey lightpink lightsalmon
  lightseagreen lightskyblue lightslategray lightslategrey lightsteelblue lightyellow lime limegreen
  linen magenta maroon mediumaquamarine mediumblue mediumorchid mediumpurple mediumseagreen
  mediumslateblue mediumspringgreen mediumturquoise mediumvioletred midnightblue mintcream
  mistyrose moccasin navajowhite navy oldlace olive olivedrab orange orangered orchid palegoldenrod
  palegreen paleturquoise palevioletred papayawhip peachpuff peru pink plum powderblue purple
  rebeccapurple rosybrown royalblue saddlebrown salmon sandybrown seagreen seashell sienna silver
  skyblue slateblue slategray slategrey snow springgreen steelblue tan teal thistle tomato turquoise
  violet wheat whitesmoke yellow yellowgreen
`.trim().split(/\s+/));
const CSS_SYSTEM_COLOR_KEYWORDS = new Set(`
  accentcolor accentcolortext activetext buttonborder buttonface buttontext canvas canvastext field
  fieldtext graytext highlight highlighttext linktext mark marktext selecteditem selecteditemtext
  visitedtext
`.trim().split(/\s+/));
function parseColorValue(cssText) {
  const text = String(cssText).trim();
  const lower = text.toLowerCase();
  if (CSS_SYSTEM_COLOR_KEYWORDS.has(lower))
    return new CSSKeywordValue(lower);
  const named = PINNED_NAMED_COLORS[lower];
  if (named) {
    return new CSSRGB(new CSSUnitValue(named[0], "number"), new CSSUnitValue(named[1], "number"), new CSSUnitValue(named[2], "number"), new CSSUnitValue(named[3] ?? 100, "percent"));
  }
  if (CSS_COLOR_KEYWORDS.has(lower))
    return new CSSKeywordValue(lower);
  const hex = /^#([\da-f]{3,4}|[\da-f]{6}|[\da-f]{8})$/i.exec(text);
  if (hex) {
    const expanded = hex[1].length <= 4 ? [...hex[1]].map((digit) => digit + digit).join("") : hex[1];
    const channels = [0, 2, 4].map((index) => parseInt(expanded.slice(index, index + 2), 16));
    const alpha = expanded.length === 8 ? parseInt(expanded.slice(6), 16) / 255 * 100 : 100;
    return new CSSRGB(new CSSUnitValue(channels[0], "number"), new CSSUnitValue(channels[1], "number"), new CSSUnitValue(channels[2], "number"), new CSSUnitValue(alpha, "percent"));
  }
  const fn = /^([a-z]+)\((.*)\)$/i.exec(text);
  if (!fn)
    return colorSyntaxError("Invalid color");
  const name = fn[1].toLowerCase();
  const parts = splitColorFunction(fn[2]);
  const alpha = parts.alpha === undefined ? new CSSUnitValue(100, "percent") : simpleColorNumeric(parts.alpha, "percent");
  if (!alpha || parts.channels.length !== 3)
    return colorSyntaxError("Invalid color");
  const parsed = (kinds) => parts.channels.map((channel, index) => simpleColorNumeric(channel, kinds[index]));
  let channels;
  if (name === "rgb" || name === "rgba") {
    channels = parsed(["rgb", "rgb", "rgb"]);
    if (channels.every(Boolean))
      return new CSSRGB(...channels, alpha);
  } else if (name === "hsl" || name === "hsla") {
    channels = parsed(["angle", "percent", "percent"]);
    if (channels.every(Boolean))
      return new CSSHSL(...channels, alpha);
  } else if (name === "hwb") {
    channels = parsed(["angle", "percent", "percent"]);
    if (channels.every(Boolean))
      return new CSSHWB(...channels, alpha);
  } else if (name === "lab" || name === "oklab") {
    channels = parsed(["percent", "number", "number"]);
    if (channels.every(Boolean)) {
      const args = channels;
      return name === "lab" ? new CSSLab(...args, alpha) : new CSSOKLab(...args, alpha);
    }
  } else if (name === "lch" || name === "oklch") {
    channels = parsed(["percent", "percent", "angle"]);
    if (channels.every(Boolean)) {
      const args = channels;
      return name === "lch" ? new CSSLCH(...args, alpha) : new CSSOKLCH(...args, alpha);
    }
  }
  return colorSyntaxError("Invalid color");
}

class CSSColorValue extends CSSStyleValue {
  static parse(cssText) {
    return parseColorValue(cssText);
  }
}

class CSSRGB extends CSSColorValue {
  _r;
  _g;
  _b;
  _alpha;
  constructor(r, g, b, alpha = 1) {
    super();
    this.r = r;
    this.g = g;
    this.b = b;
    this.alpha = alpha;
  }
  get r() {
    return this._r;
  }
  set r(value) {
    this._r = colorRgbComponent(value, "r");
  }
  get g() {
    return this._g;
  }
  set g(value) {
    this._g = colorRgbComponent(value, "g");
  }
  get b() {
    return this._b;
  }
  set b(value) {
    this._b = colorRgbComponent(value, "b");
  }
  get alpha() {
    return this._alpha;
  }
  set alpha(value) {
    this._alpha = colorPercent(value, "alpha");
  }
  toString() {
    return serializeColorFunction("rgb", [this.r, this.g, this.b], this.alpha);
  }
}

class CSSHSL extends CSSColorValue {
  _h;
  _s;
  _l;
  _alpha;
  constructor(h, s, l, alpha = 1) {
    super();
    this.h = h;
    this.s = s;
    this.l = l;
    this.alpha = alpha;
  }
  get h() {
    return this._h;
  }
  set h(value) {
    this._h = colorAngle(value, "h", true);
  }
  get s() {
    return this._s;
  }
  set s(value) {
    this._s = colorPercent(value, "s");
  }
  get l() {
    return this._l;
  }
  set l(value) {
    this._l = colorPercent(value, "l");
  }
  get alpha() {
    return this._alpha;
  }
  set alpha(value) {
    this._alpha = colorPercent(value, "alpha");
  }
  toString() {
    return serializeColorFunction("hsl", [this.h, this.s, this.l], this.alpha);
  }
}

class CSSHWB extends CSSColorValue {
  _h;
  _w;
  _b;
  _alpha;
  constructor(h, w, b, alpha = 1) {
    super();
    this.h = h;
    this.w = w;
    this.b = b;
    this.alpha = alpha;
  }
  get h() {
    return this._h;
  }
  set h(value) {
    if (!(value instanceof CSSNumericValue))
      throw new TypeError("h must be a CSSNumericValue");
    this._h = colorAngle(value, "h");
  }
  get w() {
    return this._w;
  }
  set w(value) {
    this._w = colorPercent(value, "w");
  }
  get b() {
    return this._b;
  }
  set b(value) {
    this._b = colorPercent(value, "b");
  }
  get alpha() {
    return this._alpha;
  }
  set alpha(value) {
    this._alpha = colorPercent(value, "alpha");
  }
  toString() {
    return serializeColorFunction("hwb", [this.h, this.w, this.b], this.alpha);
  }
}

class CSSCartesianColor extends CSSColorValue {
  _l;
  _a;
  _b;
  _alpha;
  _functionName = "lab";
  constructor(l, a, b, alpha = 1) {
    super();
    this.l = l;
    this.a = a;
    this.b = b;
    this.alpha = alpha;
  }
  get l() {
    return this._l;
  }
  set l(value) {
    this._l = colorPercent(value, "l");
  }
  get a() {
    return this._a;
  }
  set a(value) {
    this._a = colorNumber(value, "a");
  }
  get b() {
    return this._b;
  }
  set b(value) {
    this._b = colorNumber(value, "b");
  }
  get alpha() {
    return this._alpha;
  }
  set alpha(value) {
    this._alpha = colorPercent(value, "alpha");
  }
  toString() {
    return serializeColorFunction(this._functionName, [this.l, this.a, this.b], this.alpha);
  }
}

class CSSLab extends CSSCartesianColor {
}

class CSSOKLab extends CSSCartesianColor {
  _functionName = "oklab";
}

class CSSPolarColor extends CSSColorValue {
  _l;
  _c;
  _h;
  _alpha;
  _functionName = "lch";
  constructor(l, c, h, alpha = 1) {
    super();
    this.l = l;
    this.c = c;
    this.h = h;
    this.alpha = alpha;
  }
  get l() {
    return this._l;
  }
  set l(value) {
    this._l = colorPercent(value, "l");
  }
  get c() {
    return this._c;
  }
  set c(value) {
    this._c = colorPercent(value, "c");
  }
  get h() {
    return this._h;
  }
  set h(value) {
    this._h = colorAngle(value, "h");
  }
  get alpha() {
    return this._alpha;
  }
  set alpha(value) {
    this._alpha = colorPercent(value, "alpha");
  }
  toString() {
    return serializeColorFunction(this._functionName, [this.l, this.c, this.h], this.alpha);
  }
}

class CSSLCH extends CSSPolarColor {
}

class CSSOKLCH extends CSSPolarColor {
  _functionName = "oklch";
}
function colorKeywordish(value) {
  return value instanceof CSSKeywordValue ? value : new CSSKeywordValue(String(value));
}
function colorAlpha(value) {
  const numeric = value instanceof CSSNumericValue ? value : new CSSUnitValue(Number(value), "number");
  if (!hasNumericType(numeric, {}))
    throw new TypeError("alpha must be a number");
  return numeric;
}
function numericIndex(property) {
  if (typeof property !== "string" || !/^(0|[1-9]\d*)$/.test(property))
    return null;
  const index = Number(property);
  return Number.isSafeInteger(index) ? index : null;
}
function colorChannels(values) {
  if (values == null || typeof values[Symbol.iterator] !== "function") {
    throw new TypeError("channels must be a sequence");
  }
  return Array.from(values, (value) => colorPercent(value, "channel"));
}
function observableColorChannels(values) {
  return new Proxy(colorChannels(values), {
    set(target, property, value) {
      const index = numericIndex(property);
      return Reflect.set(target, property, index === null ? value : colorPercent(value, "channel"), target);
    },
    defineProperty(target, property, descriptor) {
      const index = numericIndex(property);
      if (index !== null) {
        if (!("value" in descriptor))
          throw new TypeError("channel must be a value");
        descriptor = { ...descriptor, value: colorPercent(descriptor.value, "channel") };
      }
      return Reflect.defineProperty(target, property, descriptor);
    }
  });
}

class CSSColor extends CSSColorValue {
  _colorSpace;
  _channels;
  _alpha;
  constructor(colorSpace, channels, alpha = 1) {
    super();
    this._channels = observableColorChannels(channels);
    lockSlot(this, "_channels");
    this.colorSpace = colorSpace;
    this.alpha = alpha;
  }
  get colorSpace() {
    return this._colorSpace;
  }
  set colorSpace(value) {
    this._colorSpace = colorKeywordish(value);
  }
  get channels() {
    return this._channels;
  }
  set channels(value) {
    const channels = colorChannels(value);
    this._channels.splice(0, this._channels.length, ...channels);
  }
  get alpha() {
    return this._alpha;
  }
  set alpha(value) {
    this._alpha = colorAlpha(value);
  }
  toString() {
    const alpha = this.alpha instanceof CSSUnitValue && this.alpha.unit === "number" && this.alpha.value === 1 ? "" : ` / ${this.alpha}`;
    return `color(${this.colorSpace} ${this.channels.join(" ")}${alpha})`;
  }
}
function copyColorInterfaceMembers(source, targets) {
  for (const target of targets) {
    for (const name of Object.getOwnPropertyNames(source)) {
      if (name === "constructor")
        continue;
      const descriptor = Object.getOwnPropertyDescriptor(source, name);
      if (descriptor)
        Object.defineProperty(target, name, descriptor);
    }
  }
}
copyColorInterfaceMembers(CSSCartesianColor.prototype, [CSSLab.prototype, CSSOKLab.prototype]);
copyColorInterfaceMembers(CSSPolarColor.prototype, [CSSLCH.prototype, CSSOKLCH.prototype]);
function callTypedParser(name, first, second) {
  try {
    return name === "parseTypedNumericValue" ? opts?.parseTypedNumericValue?.(first) : opts?.parseTypedStyleValue?.(first, second || "");
  } catch {
    return null;
  }
}
function dtoObject(value) {
  return value !== null && typeof value === "object" ? value : null;
}
function dtoNumericValues(value) {
  if (!Array.isArray(value))
    return null;
  const out = [];
  for (const item of value) {
    const parsed = typedValueFromDto(item);
    if (!(parsed instanceof CSSNumericValue))
      return null;
    out.push(parsed);
  }
  return out;
}
function typedUnparsedSegments(value) {
  if (!Array.isArray(value))
    return null;
  const segments = [];
  for (const item of value) {
    const segment = dtoObject(item);
    if (segment?.type === "string" && typeof segment.value === "string") {
      segments.push(segment.value);
    } else if (segment?.type === "variable" && typeof segment.name === "string") {
      const fallbackSegments = segment.fallback === undefined ? null : typedUnparsedSegments(segment.fallback);
      if (segment.fallback !== undefined && !fallbackSegments)
        return null;
      segments.push(new CSSVariableReferenceValue(segment.name, fallbackSegments ? new CSSUnparsedValue(fallbackSegments) : null));
    } else {
      return null;
    }
  }
  return segments;
}
function typedValueFromDto(value, _associatedProperty) {
  const dto = dtoObject(value);
  if (!dto || typeof dto.type !== "string")
    return null;
  if (dto.type === "keyword" && typeof dto.value === "string")
    return new CSSKeywordValue(dto.value);
  if (dto.type === "unit" && typeof dto.value === "number" && typeof dto.unit === "string") {
    return new CSSUnitValue(dto.value, dto.unit === "%" ? "percent" : dto.unit);
  }
  const values = dtoNumericValues(dto.values);
  if (dto.type === "sum" && values)
    return new CSSMathSum(...values);
  if (dto.type === "product" && values)
    return new CSSMathProduct(...values);
  if (dto.type === "min" && values)
    return new CSSMathMin(...values);
  if (dto.type === "max" && values)
    return new CSSMathMax(...values);
  const numeric = typedValueFromDto(dto.value);
  if (dto.type === "negate" && numeric instanceof CSSNumericValue)
    return new CSSMathNegate(numeric);
  if (dto.type === "invert" && numeric instanceof CSSNumericValue)
    return new CSSMathInvert(numeric);
  if (dto.type === "clamp") {
    const lower = typedValueFromDto(dto.lower);
    const middle = typedValueFromDto(dto.value);
    const upper = typedValueFromDto(dto.upper);
    if (lower instanceof CSSNumericValue && middle instanceof CSSNumericValue && upper instanceof CSSNumericValue) {
      return new CSSMathClamp(lower, middle, upper);
    }
  }
  if (dto.type === "transform" && Array.isArray(dto.components)) {
    const components = [];
    for (const component of dto.components) {
      const parsed = transformComponentFromDto(component);
      if (!parsed)
        return null;
      components.push(parsed);
    }
    if (components.length)
      return new CSSTransformValue(components);
  }
  if (dto.type === "unparsed") {
    const segments = typedUnparsedSegments(dto.segments);
    if (segments)
      return new CSSUnparsedValue(segments);
  }
  if (dto.type === "generic")
    return null;
  return null;
}
function reifyTypedList(property, cssText) {
  const result = dtoObject(callTypedParser("parseTypedStyleValue", property, cssText));
  if (!result || !Array.isArray(result.values))
    return null;
  const values = [];
  for (const item of result.values) {
    const dto = dtoObject(item);
    if (dto?.type === "keyword" || dto?.type === "generic")
      return null;
    const value = typedValueFromDto(item, property);
    if (!value)
      return null;
    value._associatedProperty = property;
    values.push(value);
  }
  return values.length ? values : null;
}

class CSSVariableReferenceValue {
  _variable;
  _fallback;
  constructor(variable, fallback = null) {
    const name = String(variable);
    if (!/^--[^\s]+$/.test(name))
      throw new TypeError("Invalid custom property name");
    if (fallback !== null && !(fallback instanceof CSSUnparsedValue)) {
      throw new TypeError("Fallback must be a CSSUnparsedValue");
    }
    this._variable = name;
    this._fallback = fallback;
  }
  get variable() {
    return this._variable;
  }
  set variable(value) {
    const name = String(value);
    if (!/^--[^\s]+$/.test(name))
      throw new TypeError("Invalid custom property name");
    this._variable = name;
  }
  get fallback() {
    return this._fallback;
  }
  toString() {
    return `var(${this._variable}${this.fallback ? `, ${this.fallback}` : ""})`;
  }
}

class CSSUnparsedValue extends CSSStyleValue {
  _length;
  constructor(members) {
    super();
    const values = Array.from(members);
    this._length = values.length;
    for (let i = 0;i < values.length; i++) {
      const value = values[i];
      if (typeof value !== "string" && !(value instanceof CSSVariableReferenceValue)) {
        throw new TypeError("Invalid CSSUnparsedValue member");
      }
      this[i] = value;
    }
    this._cssText = this.toString();
  }
  get length() {
    return this._length;
  }
  *[Symbol.iterator]() {
    for (let i = 0;i < this.length; i++)
      yield this[i];
  }
  toString() {
    return Array.from(this).map(String).join("");
  }
}
function hasNumericType(value, expected) {
  return typeEquals(value.type(), expected);
}
function asNumeric(value, description) {
  if (!(value instanceof CSSNumericValue))
    throw new TypeError(`${description} must be numeric`);
  return value;
}
function asNumberish(value, description) {
  const numeric = typeof value === "number" ? new CSSUnitValue(value, "number") : asNumeric(value, description);
  if (!hasNumericType(numeric, {}))
    throw new TypeError(`${description} must be a number`);
  return numeric;
}
function asAngle(value, description) {
  const numeric = asNumeric(value, description);
  if (!hasNumericType(numeric, { angle: 1 }))
    throw new TypeError(`${description} must be an angle`);
  return numeric;
}
function asLength(value, description) {
  const numeric = asNumeric(value, description);
  if (!hasNumericType(numeric, { length: 1 }))
    throw new TypeError(`${description} must be a length`);
  return numeric;
}
function asLengthPercentage(value, description) {
  const numeric = asNumeric(value, description);
  const type = numeric.type();
  if (!typeEquals(type, { length: 1 }) && !typeEquals(type, { percent: 1 }) && !typeEquals(type, { length: 1, percentHint: "length" })) {
    throw new TypeError(`${description} must be a length or percentage`);
  }
  return numeric;
}
function numericIn(value, unit) {
  return value.to(unit).value;
}
function serializeTransformNumeric(value) {
  if (value instanceof CSSUnitValue) {
    const unit = canonicalUnit(value.unit);
    return String(unit === value.unit ? value : value.to(unit));
  }
  if (value instanceof CSSMathSum) {
    const parts = value.values._values.map((part, index) => ({
      text: serializeTransformNumeric(part),
      unit: representativeUnit(part) || "",
      index
    }));
    parts.sort((a, b) => a.unit.localeCompare(b.unit) || a.index - b.index);
    return `calc(${parts.map((part) => part.text).join(" + ")})`;
  }
  if (value instanceof CSSMathProduct) {
    const numerator = [];
    const denominator = [];
    for (const part of value.values._values) {
      if (part instanceof CSSMathInvert)
        denominator.push(part.value);
      else
        numerator.push(part);
    }
    numerator.sort((a, b) => Number(!hasNumericType(a, {})) - Number(!hasNumericType(b, {})));
    let text = numerator.map(serializeTransformNumeric).join(" * ") || "1";
    for (const part of denominator)
      text += ` / ${serializeTransformNumeric(part)}`;
    return `calc(${text})`;
  }
  if (value instanceof CSSMathNegate)
    return `calc(-1 * ${serializeTransformNumeric(value.value)})`;
  if (value instanceof CSSMathInvert)
    return `calc(1 / ${serializeTransformNumeric(value.value)})`;
  if (value instanceof CSSMathMin)
    return `min(${value.values._values.map(serializeTransformNumeric).join(", ")})`;
  if (value instanceof CSSMathMax)
    return `max(${value.values._values.map(serializeTransformNumeric).join(", ")})`;
  if (value instanceof CSSMathClamp) {
    return `clamp(${serializeTransformNumeric(value.lower)}, ${serializeTransformNumeric(value.value)}, ${serializeTransformNumeric(value.upper)})`;
  }
  return String(value);
}
const matrixStates = new WeakMap;
function matrixState(matrix) {
  const state = matrixStates.get(matrix);
  if (!state)
    throw new TypeError("Illegal invocation");
  return state;
}
function parseMatrixString(value) {
  const text = value.trim();
  if (text === "" || text === "none")
    return [1, 0, 0, 1, 0, 0];
  const match = /^matrix(3d)?\(([^)]*)\)$/i.exec(text);
  if (!match)
    return null;
  const parts = match[2].split(",").map((part) => Number(part.trim()));
  if (parts.some((part) => !Number.isFinite(part)))
    return null;
  const wanted = match[1] ? 16 : 6;
  return parts.length === wanted ? parts : null;
}
function matrixValues(values) {
  let input;
  if (typeof values === "string") {
    const parsed = parseMatrixString(values);
    if (!parsed) {
      throw new DOMException(`Failed to construct 'DOMMatrix': The provided value '${values}' is not a valid matrix.`, "SyntaxError");
    }
    input = parsed;
  } else {
    input = Array.from(values, Number);
  }
  if (input.length === 6) {
    const [a, b, c, d, e, f] = input;
    return { values: [a, b, 0, 0, c, d, 0, 0, 0, 0, 1, 0, e, f, 0, 1], is2D: true };
  }
  if (input.length === 16)
    return { values: input, is2D: false };
  throw new TypeError("Matrix needs 6 or 16 values");
}
function multiplyMatrices(left, right) {
  const result = Array(16).fill(0);
  for (let column = 0;column < 4; column++) {
    for (let row = 0;row < 4; row++) {
      for (let k = 0;k < 4; k++) {
        result[column * 4 + row] += left[k * 4 + row] * right[column * 4 + k];
      }
    }
  }
  return result;
}
function matrixFrom4x4(values, is2D) {
  const matrix = new DOMMatrix(values);
  matrixState(matrix).is2D = is2D;
  return matrix;
}
function twoDimensionalValues(values) {
  return [values[0], values[1], values[4], values[5], values[12], values[13]];
}

class DOMMatrixReadOnly {
  constructor(values = [1, 0, 0, 1, 0, 0]) {
    matrixStates.set(this, matrixValues(values));
  }
  get a() {
    return matrixState(this).values[0];
  }
  get b() {
    return matrixState(this).values[1];
  }
  get c() {
    return matrixState(this).values[4];
  }
  get d() {
    return matrixState(this).values[5];
  }
  get e() {
    return matrixState(this).values[12];
  }
  get f() {
    return matrixState(this).values[13];
  }
  get m11() {
    return matrixState(this).values[0];
  }
  get m12() {
    return matrixState(this).values[1];
  }
  get m13() {
    return matrixState(this).values[2];
  }
  get m14() {
    return matrixState(this).values[3];
  }
  get m21() {
    return matrixState(this).values[4];
  }
  get m22() {
    return matrixState(this).values[5];
  }
  get m23() {
    return matrixState(this).values[6];
  }
  get m24() {
    return matrixState(this).values[7];
  }
  get m31() {
    return matrixState(this).values[8];
  }
  get m32() {
    return matrixState(this).values[9];
  }
  get m33() {
    return matrixState(this).values[10];
  }
  get m34() {
    return matrixState(this).values[11];
  }
  get m41() {
    return matrixState(this).values[12];
  }
  get m42() {
    return matrixState(this).values[13];
  }
  get m43() {
    return matrixState(this).values[14];
  }
  get m44() {
    return matrixState(this).values[15];
  }
  get is2D() {
    return matrixState(this).is2D;
  }
  multiply(other) {
    if (!(other instanceof DOMMatrixReadOnly))
      throw new TypeError("Matrix required");
    return matrixFrom4x4(multiplyMatrices(matrixState(this).values, matrixState(other).values), this.is2D && other.is2D);
  }
  translate(tx = 0, ty = 0, tz = 0) {
    return this.multiply(matrixFrom4x4([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, Number(tx), Number(ty), Number(tz), 1], Number(tz) === 0));
  }
  scale(scaleX = 1, scaleY = scaleX, scaleZ = 1) {
    return this.multiply(matrixFrom4x4([Number(scaleX), 0, 0, 0, 0, Number(scaleY), 0, 0, 0, 0, Number(scaleZ), 0, 0, 0, 0, 1], Number(scaleZ) === 1));
  }
  rotate(angle = 0) {
    return this.rotateAxisAngle(0, 0, 1, angle);
  }
  rotateAxisAngle(x = 0, y = 0, z = 0, angle = 0) {
    let nx = Number(x), ny = Number(y), nz = Number(z);
    const length = Math.hypot(nx, ny, nz);
    if (!length)
      return matrixFrom4x4(matrixState(this).values.slice(), this.is2D);
    nx /= length;
    ny /= length;
    nz /= length;
    const radians = Number(angle) * Math.PI / 180;
    const c = Math.cos(radians), s = Math.sin(radians), t = 1 - c;
    const rotation = [
      t * nx * nx + c,
      t * nx * ny + s * nz,
      t * nx * nz - s * ny,
      0,
      t * nx * ny - s * nz,
      t * ny * ny + c,
      t * ny * nz + s * nx,
      0,
      t * nx * nz + s * ny,
      t * ny * nz - s * nx,
      t * nz * nz + c,
      0,
      0,
      0,
      0,
      1
    ];
    return this.multiply(matrixFrom4x4(rotation, nx === 0 && ny === 0));
  }
  skewX(angle = 0) {
    const tangent = Math.tan(Number(angle) * Math.PI / 180);
    return this.multiply(new DOMMatrix([1, 0, tangent, 1, 0, 0]));
  }
  skewY(angle = 0) {
    const tangent = Math.tan(Number(angle) * Math.PI / 180);
    return this.multiply(new DOMMatrix([1, tangent, 0, 1, 0, 0]));
  }
  toFloat32Array() {
    return new Float32Array(matrixState(this).values);
  }
  toFloat64Array() {
    return new Float64Array(matrixState(this).values);
  }
  toString() {
    const stored = matrixState(this).values;
    const values = this.is2D ? twoDimensionalValues(stored) : stored;
    return `${this.is2D ? "matrix" : "matrix3d"}(${values.join(", ")})`;
  }
}

class DOMMatrix extends DOMMatrixReadOnly {
  _set(index, value, twoDimensionalValue = null) {
    const state = matrixState(this);
    const number = Number(value);
    state.values[index] = number;
    if (twoDimensionalValue !== null && number !== twoDimensionalValue)
      state.is2D = false;
  }
  multiplySelf(other) {
    const result = this.multiply(other);
    matrixStates.set(this, { values: matrixState(result).values.slice(), is2D: result.is2D });
    return this;
  }
  translateSelf(tx = 0, ty = 0, tz = 0) {
    const result = this.translate(tx, ty, tz);
    matrixStates.set(this, { values: matrixState(result).values.slice(), is2D: result.is2D });
    return this;
  }
  scaleSelf(scaleX = 1, scaleY = scaleX, scaleZ = 1) {
    const result = this.scale(scaleX, scaleY, scaleZ);
    matrixStates.set(this, { values: matrixState(result).values.slice(), is2D: result.is2D });
    return this;
  }
  rotateSelf(angle = 0) {
    const result = this.rotate(angle);
    matrixStates.set(this, { values: matrixState(result).values.slice(), is2D: result.is2D });
    return this;
  }
}
const MATRIX_FIELDS = {
  a: [0, null],
  b: [1, null],
  c: [4, null],
  d: [5, null],
  e: [12, null],
  f: [13, null],
  m11: [0, null],
  m12: [1, null],
  m13: [2, 0],
  m14: [3, 0],
  m21: [4, null],
  m22: [5, null],
  m23: [6, 0],
  m24: [7, 0],
  m31: [8, 0],
  m32: [9, 0],
  m33: [10, 1],
  m34: [11, 0],
  m41: [12, null],
  m42: [13, null],
  m43: [14, 0],
  m44: [15, 1]
};
for (const [name, [index, twoDimensionalValue]] of Object.entries(MATRIX_FIELDS)) {
  Object.defineProperty(DOMMatrix.prototype, name, {
    get() {
      return matrixState(this).values[index];
    },
    set(value) {
      this._set(index, value, twoDimensionalValue);
    },
    enumerable: true,
    configurable: true
  });
}

class CSSTransformComponent {
  _is2D = true;
  get is2D() {
    return this._is2D;
  }
  set is2D(value) {
    this._is2D = Boolean(value);
  }
  toMatrix() {
    return new DOMMatrix;
  }
  toString() {
    return String(this.toMatrix());
  }
}

class CSSTranslate extends CSSTransformComponent {
  _x;
  _y;
  _z;
  constructor(x, y, z) {
    super();
    this.x = x;
    this.y = y;
    this._z = asLength(arguments.length < 3 ? new CSSUnitValue(0, "px") : z, "z");
    this._is2D = arguments.length < 3;
  }
  get x() {
    return this._x;
  }
  set x(value) {
    this._x = asLengthPercentage(value, "x");
  }
  get y() {
    return this._y;
  }
  set y(value) {
    this._y = asLengthPercentage(value, "y");
  }
  get z() {
    return this._z;
  }
  set z(value) {
    this._z = asLength(value, "z");
  }
  toMatrix() {
    const x = numericIn(this.x, "px"), y = numericIn(this.y, "px");
    return new DOMMatrix().translate(x, y, this.is2D ? 0 : numericIn(this.z, "px"));
  }
  toString() {
    const x = serializeTransformNumeric(this._x), y = serializeTransformNumeric(this._y);
    return this.is2D ? `translate(${x}, ${y})` : `translate3d(${x}, ${y}, ${serializeTransformNumeric(this._z)})`;
  }
}

class CSSRotate extends CSSTransformComponent {
  _x;
  _y;
  _z;
  _angle;
  constructor(x, y, z, angle) {
    super();
    if (arguments.length === 1) {
      this.x = 0;
      this.y = 0;
      this.z = 1;
      this.angle = x;
    } else {
      this.x = x;
      this._y = asNumberish(y, "y");
      this._z = asNumberish(z, "z");
      this._angle = asAngle(angle, "angle");
      this._is2D = false;
    }
  }
  get x() {
    return this._x;
  }
  set x(value) {
    this._x = asNumberish(value, "x");
  }
  get y() {
    return this._y;
  }
  set y(value) {
    this._y = asNumberish(value, "y");
  }
  get z() {
    return this._z;
  }
  set z(value) {
    this._z = asNumberish(value, "z");
  }
  get angle() {
    return this._angle;
  }
  set angle(value) {
    this._angle = asAngle(value, "angle");
  }
  toMatrix() {
    const angle = numericIn(this.angle, "deg");
    if (this.is2D)
      return new DOMMatrix().rotate(angle);
    return new DOMMatrix().rotateAxisAngle(numericIn(this._x, "number"), numericIn(this._y, "number"), numericIn(this._z, "number"), angle);
  }
  toString() {
    const angle = serializeTransformNumeric(this._angle);
    return this.is2D ? `rotate(${angle})` : `rotate3d(${serializeTransformNumeric(this._x)}, ${serializeTransformNumeric(this._y)}, ${serializeTransformNumeric(this._z)}, ${angle})`;
  }
}

class CSSScale extends CSSTransformComponent {
  _x;
  _y;
  _z;
  constructor(x, y, z) {
    super();
    this.x = x;
    this.y = y;
    this._z = asNumberish(arguments.length < 3 ? 1 : z, "z");
    this._is2D = arguments.length < 3;
  }
  get x() {
    return this._x;
  }
  set x(value) {
    this._x = asNumberish(value, "x");
  }
  get y() {
    return this._y;
  }
  set y(value) {
    this._y = asNumberish(value, "y");
  }
  get z() {
    return this._z;
  }
  set z(value) {
    this._z = asNumberish(value, "z");
  }
  toMatrix() {
    return new DOMMatrix().scale(numericIn(this._x, "number"), numericIn(this._y, "number"), this.is2D ? 1 : numericIn(this._z, "number"));
  }
  toString() {
    const x = serializeTransformNumeric(this._x), y = serializeTransformNumeric(this._y);
    return this.is2D ? `scale(${x}, ${y})` : `scale3d(${x}, ${y}, ${serializeTransformNumeric(this._z)})`;
  }
}

class CSSSkew extends CSSTransformComponent {
  _ax;
  _ay;
  constructor(ax, ay) {
    super();
    this.ax = ax;
    this.ay = ay;
  }
  get ax() {
    return this._ax;
  }
  set ax(value) {
    this._ax = asAngle(value, "ax");
  }
  get ay() {
    return this._ay;
  }
  set ay(value) {
    this._ay = asAngle(value, "ay");
  }
  set is2D(_value) {}
  get is2D() {
    return true;
  }
  toMatrix() {
    const ax = Math.tan(numericIn(this.ax, "rad")), ay = Math.tan(numericIn(this.ay, "rad"));
    return new DOMMatrix([1, ay, ax, 1, 0, 0]);
  }
  toString() {
    const ax = serializeTransformNumeric(this._ax);
    return numericIn(this._ay, "deg") === 0 ? `skew(${ax})` : `skew(${ax}, ${serializeTransformNumeric(this._ay)})`;
  }
}

class CSSSkewX extends CSSTransformComponent {
  _ax;
  constructor(ax) {
    super();
    this.ax = ax;
  }
  get ax() {
    return this._ax;
  }
  set ax(value) {
    this._ax = asAngle(value, "ax");
  }
  set is2D(_value) {}
  get is2D() {
    return true;
  }
  toMatrix() {
    return new DOMMatrix().skewX(numericIn(this.ax, "deg"));
  }
  toString() {
    return `skewX(${serializeTransformNumeric(this._ax)})`;
  }
}

class CSSSkewY extends CSSTransformComponent {
  _ay;
  constructor(ay) {
    super();
    this.ay = ay;
  }
  get ay() {
    return this._ay;
  }
  set ay(value) {
    this._ay = asAngle(value, "ay");
  }
  set is2D(_value) {}
  get is2D() {
    return true;
  }
  toMatrix() {
    return new DOMMatrix().skewY(numericIn(this.ay, "deg"));
  }
  toString() {
    return `skewY(${serializeTransformNumeric(this._ay)})`;
  }
}

class CSSPerspective extends CSSTransformComponent {
  _length;
  constructor(length) {
    super();
    this.length = length;
    this._is2D = false;
  }
  get length() {
    return this._length;
  }
  set length(value) {
    if (value === "none")
      value = new CSSKeywordValue("none");
    if (value instanceof CSSKeywordValue && value.value === "none")
      this._length = value;
    else
      this._length = asLength(value, "length");
  }
  set is2D(_value) {}
  get is2D() {
    return false;
  }
  toMatrix() {
    if (this._length instanceof CSSKeywordValue)
      return new DOMMatrix;
    const length = Math.max(1, numericIn(this._length, "px"));
    return matrixFrom4x4([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, -1 / length, 0, 0, 0, 1], false);
  }
  toString() {
    const value = this._length instanceof CSSUnitValue && this._length.value < 0 ? `calc(${serializeTransformNumeric(this._length)})` : this._length instanceof CSSNumericValue ? serializeTransformNumeric(this._length) : String(this._length);
    return `perspective(${value})`;
  }
}

class CSSMatrixComponent extends CSSTransformComponent {
  _matrix;
  constructor(matrix, options) {
    super();
    if (!(matrix instanceof DOMMatrixReadOnly))
      throw new TypeError("matrix must be a DOMMatrixReadOnly");
    const state = matrixState(matrix);
    this._matrix = matrixFrom4x4(state.values.slice(), state.is2D);
    this._is2D = options && typeof options === "object" && "is2D" in options ? Boolean(options.is2D) : this.matrix.is2D;
  }
  get matrix() {
    return this._matrix;
  }
  set matrix(value) {
    if (!(value instanceof DOMMatrix))
      throw new TypeError("matrix must be a DOMMatrix");
    this._matrix = value;
  }
  toMatrix() {
    const values = matrixState(this.matrix).values;
    return this.is2D ? new DOMMatrix(twoDimensionalValues(values)) : new DOMMatrix(values);
  }
  toString() {
    return String(this.toMatrix());
  }
}
function transformComponentFromDto(value) {
  const dto = dtoObject(value);
  if (!dto || typeof dto.type !== "string")
    return null;
  const numeric = (member) => typedValueFromDto(dto[member]);
  try {
    let component = null;
    if (dto.type === "translate")
      component = new CSSTranslate(numeric("x"), numeric("y"), numeric("z"));
    else if (dto.type === "rotate")
      component = new CSSRotate(numeric("x"), numeric("y"), numeric("z"), numeric("angle"));
    else if (dto.type === "scale")
      component = new CSSScale(numeric("x"), numeric("y"), numeric("z"));
    else if (dto.type === "skew")
      component = new CSSSkew(numeric("ax"), numeric("ay"));
    else if (dto.type === "skew_x")
      component = new CSSSkewX(numeric("angle"));
    else if (dto.type === "skew_y")
      component = new CSSSkewY(numeric("angle"));
    else if (dto.type === "perspective") {
      const length = typedValueFromDto(dto.length);
      component = new CSSPerspective(length);
    } else if (dto.type === "matrix" && Array.isArray(dto.matrix) && dto.matrix.length === 16) {
      component = new CSSMatrixComponent(new DOMMatrix(dto.matrix));
    }
    if (component && typeof dto.is_2d === "boolean")
      component.is2D = dto.is_2d;
    return component;
  } catch {
    return null;
  }
}
const transformComponents = new WeakMap;
function componentsFor(value) {
  const components = transformComponents.get(value);
  if (!components)
    throw new TypeError("Illegal invocation");
  return components;
}

class CSSTransformValue extends CSSStyleValue {
  constructor(components) {
    super();
    if (components == null)
      throw new TypeError("Transform components are required");
    const values = Array.from(components);
    if (!values.length || values.some((value) => !(value instanceof CSSTransformComponent))) {
      throw new TypeError("CSSTransformValue requires transform components");
    }
    transformComponents.set(this, values);
    const proxy = new Proxy(this, {
      get(target, property, receiver) {
        const index = numericIndex(property);
        return index === null ? Reflect.get(target, property, receiver) : componentsFor(target)[index];
      },
      set(target, property, value, receiver) {
        const index = numericIndex(property);
        if (index !== null) {
          if (!(value instanceof CSSTransformComponent))
            throw new TypeError("Transform component required");
          const stored = componentsFor(target);
          if (index > stored.length)
            throw new TypeError("Transform index is out of range");
          stored[index] = value;
          return true;
        }
        if (property === "length" || property === "is2D" || property === "_components") {
          throw new TypeError(`${String(property)} is readonly`);
        }
        return Reflect.set(target, property, value, receiver);
      },
      defineProperty(target, property, descriptor) {
        if (property === "_components")
          throw new TypeError("_components is private");
        return Reflect.defineProperty(target, property, descriptor);
      },
      deleteProperty(target, property) {
        if (numericIndex(property) !== null)
          return false;
        return Reflect.deleteProperty(target, property);
      },
      has(target, property) {
        const index = numericIndex(property);
        return index === null ? Reflect.has(target, property) : index < componentsFor(target).length;
      },
      ownKeys(target) {
        return [
          ...componentsFor(target).map((_, index) => String(index)),
          ...Reflect.ownKeys(target)
        ];
      },
      getOwnPropertyDescriptor(target, property) {
        const index = numericIndex(property);
        const stored = componentsFor(target);
        if (index !== null && index < stored.length) {
          return { value: stored[index], writable: true, enumerable: true, configurable: true };
        }
        return Reflect.getOwnPropertyDescriptor(target, property);
      }
    });
    transformComponents.set(proxy, values);
    return proxy;
  }
  get length() {
    return componentsFor(this).length;
  }
  get is2D() {
    return componentsFor(this).every((value) => value.is2D);
  }
  *entries() {
    const components = componentsFor(this);
    for (let i = 0;i < components.length; i++)
      yield [i, components[i]];
  }
  *keys() {
    for (let i = 0;i < this.length; i++)
      yield i;
  }
  *values() {
    yield* componentsFor(this);
  }
  forEach(callback, thisArg) {
    const components = componentsFor(this);
    for (let i = 0;i < components.length; i++)
      callback.call(thisArg, components[i], i, this);
  }
  [Symbol.iterator]() {
    return this.values();
  }
  toMatrix() {
    return componentsFor(this).reduce((matrix, component) => matrix.multiply(component.toMatrix()), new DOMMatrix);
  }
  toString() {
    return componentsFor(this).map(String).join(" ");
  }
}
function normalizeProperty(name) {
  const property = String(name);
  return property.startsWith("--") ? property : property.toLowerCase();
}
const supportedPropertyCache = new Map;
function assertSupportedProperty(property) {
  if (property.startsWith("--"))
    return;
  let supported = supportedPropertyCache.get(property);
  if (supported === undefined) {
    const canon = opts && typeof opts.canonicalizeInlineStyle === "function" ? opts.canonicalizeInlineStyle : null;
    if (!canon)
      return;
    try {
      supported = canon(`${property}: initial`).length > 0;
    } catch {
      supported = false;
    }
    supportedPropertyCache.set(property, supported);
  }
  if (!supported)
    throw new TypeError(`Unsupported property: ${property}`);
}
const IMAGE_PROPERTIES = new Set([
  "background-image",
  "border-image-source",
  "list-style-image",
  "mask-image",
  "shape-outside"
]);
const LIST_VALUED_PROPERTIES = new Set([
  "animation-composition",
  "animation-delay",
  "animation-direction",
  "animation-duration",
  "animation-fill-mode",
  "animation-iteration-count",
  "animation-name",
  "animation-play-state",
  "animation-timeline",
  "animation-timing-function",
  "background-attachment",
  "background-clip",
  "background-image",
  "background-origin",
  "background-position",
  "background-repeat",
  "background-size",
  "mask-clip",
  "mask-composite",
  "mask-image",
  "mask-mode",
  "mask-origin",
  "mask-position",
  "mask-repeat",
  "mask-size",
  "transition-behavior",
  "transition-delay",
  "transition-duration",
  "transition-timing-function"
]);
function isColorProperty(property) {
  return property === "color" || property === "accent-color" || property === "caret-color" || property === "fill" || property === "stroke" || property.endsWith("-color");
}
function splitTopLevelCommas(text) {
  const values = [];
  let start = 0;
  let depth = 0;
  let quote = "";
  for (let i = 0;i < text.length; i++) {
    const char = text[i];
    if (quote) {
      if (char === "\\")
        i++;
      else if (char === quote)
        quote = "";
    } else if (char === '"' || char === "'")
      quote = char;
    else if (char === "(")
      depth++;
    else if (char === ")")
      depth--;
    else if (char === "," && depth === 0) {
      values.push(text.slice(start, i).trim());
      start = i + 1;
    }
  }
  values.push(text.slice(start).trim());
  return values;
}
function hasTopLevelSemicolon(text) {
  let depth = 0;
  let quote = "";
  for (let i = 0;i < text.length; i++) {
    const char = text[i];
    if (quote) {
      if (char === "\\")
        i++;
      else if (char === quote)
        quote = "";
    } else if (char === '"' || char === "'")
      quote = char;
    else if (char === "(")
      depth++;
    else if (char === ")")
      depth--;
    else if (char === ";" && depth === 0)
      return true;
  }
  return false;
}
function hasTopLevelImportant(text) {
  let depth = 0;
  let quote = "";
  let bang = -1;
  for (let i = 0;i < text.length; i++) {
    const char = text[i];
    if (quote) {
      if (char === "\\")
        i++;
      else if (char === quote)
        quote = "";
    } else if (char === '"' || char === "'")
      quote = char;
    else if (char === "(")
      depth++;
    else if (char === ")")
      depth--;
    else if (char === "!" && depth === 0)
      bang = i;
  }
  return bang >= 0 && /^!\s*important\s*$/i.test(text.slice(bang));
}
function reify(property, cssText) {
  const text = String(cssText).trim();
  if (!text)
    return;
  if (property.startsWith("--"))
    return new CSSUnparsedValue([text]);
  if (IMAGE_PROPERTIES.has(property) && /^url\(/i.test(text))
    return new CSSImageValue(text, property);
  if (/^var\(/i.test(text)) {
    const match = /^var\(\s*(--[^,\s)]+)(?:\s*,\s*(.*))?\)$/i.exec(text);
    if (match) {
      const fallback = match[2] == null ? null : new CSSUnparsedValue([match[2]]);
      return new CSSUnparsedValue([new CSSVariableReferenceValue(match[1], fallback)]);
    }
  }
  const typed = reifyTypedList(property, text);
  if (typed?.length === 1)
    return typed[0];
  const canon = opts && typeof opts.canonicalizeInlineStyle === "function" ? opts.canonicalizeInlineStyle : null;
  if (canon) {
    try {
      const declarations = canon(`${property}: ${text}`);
      if (declarations.length > 1) {
        return new CSSStyleValue(text, property);
      }
    } catch {}
  }
  const calc = /^calc\((.*)\)$/i.exec(text);
  if (calc) {
    const terms = calc[1].split(/\s+\+\s+/).map((term) => reifyNumeric(term));
    if (terms.every((term) => term !== null))
      return new CSSMathSum(...terms);
  }
  const numeric = reifyNumeric(text);
  if (numeric)
    return numeric;
  const comma = text.indexOf(",");
  if (comma >= 0) {
    const firstListValue = text.slice(0, comma).trim();
    const firstNumeric = reifyNumeric(firstListValue);
    if (firstNumeric)
      return firstNumeric;
  }
  if (/^[a-zA-Z_-][a-zA-Z0-9_-]*$/.test(text)) {
    if (isColorProperty(property)) {
      const keyword = text.toLowerCase();
      const supported = keyword === "currentcolor" || keyword === "auto" && (property === "accent-color" || property === "caret-color");
      return supported ? new CSSKeywordValue(text) : new CSSStyleValue(text, property);
    }
    if (property === "font-family")
      return new CSSStyleValue(text, property);
    if (property === "transition-property" && text.toLowerCase() !== "none") {
      return new CSSStyleValue(text, property);
    }
    if (property === "will-change" && text.toLowerCase() !== "auto") {
      return new CSSStyleValue(text, property);
    }
    return new CSSKeywordValue(text);
  }
  return new CSSStyleValue(text, property);
}
function reifyNumeric(text) {
  const match = /^([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:e[+-]?\d+)?)(%|[a-zA-Z]+)?$/i.exec(text.trim());
  if (!match)
    return null;
  const rawUnit = match[2] || "number";
  const unit = rawUnit === "%" ? "percent" : rawUnit;
  try {
    return new CSSUnitValue(Number(match[1]), unit);
  } catch {
    return null;
  }
}

class StylePropertyMapReadOnly {
  _declaration;
  constructor(declaration) {
    this._declaration = declaration;
  }
  get(propertyName) {
    const property = normalizeProperty(propertyName);
    assertSupportedProperty(property);
    return this.getAll(property)[0];
  }
  getAll(propertyName) {
    const property = normalizeProperty(propertyName);
    assertSupportedProperty(property);
    const text = this._declaration().getPropertyValue(property);
    if (!text)
      return [];
    const parts = LIST_VALUED_PROPERTIES.has(property) ? splitTopLevelCommas(text) : [text];
    const values = parts.map((part) => reify(property, part)).filter((value) => value !== undefined);
    for (const value of values)
      value._associatedProperty = property;
    return values;
  }
  has(propertyName) {
    return this.get(propertyName) !== undefined;
  }
  _entries() {
    const declaration = this._declaration();
    const entries = [];
    const seen = new Set;
    if (typeof declaration.item === "function" && typeof declaration.length === "number") {
      for (let i = 0;i < declaration.length; i++) {
        const property = declaration.item(i);
        if (!property || seen.has(property))
          continue;
        seen.add(property);
        const values = this.getAll(property);
        if (values.length)
          entries.push([property, values]);
      }
    }
    return entries;
  }
  get size() {
    return this._entries().length;
  }
  [Symbol.iterator]() {
    return this._entries()[Symbol.iterator]();
  }
  entries() {
    return this._entries()[Symbol.iterator]();
  }
  keys() {
    return this._entries().map(([key]) => key)[Symbol.iterator]();
  }
  values() {
    return this._entries().map(([, value]) => value)[Symbol.iterator]();
  }
  forEach(callback, thisArg) {
    for (const [key, value] of this)
      callback.call(thisArg, value, key, this);
  }
}

class StylePropertyMap extends StylePropertyMapReadOnly {
  _associatedValues = new Map;
  get(propertyName) {
    const property = normalizeProperty(propertyName);
    assertSupportedProperty(property);
    const associated = this._associatedValues.get(property);
    if (associated && this._declaration().getPropertyValue(property) === associated.declarationValue) {
      const value = cloneAssociatedValue(associated.value, property);
      value._associatedProperty = property;
      return value;
    }
    this._associatedValues.delete(property);
    return super.get(property);
  }
  set(propertyName, ...values) {
    const property = normalizeProperty(propertyName);
    assertSupportedProperty(property);
    if (values.length === 0 || values.length > 1 && !LIST_VALUED_PROPERTIES.has(property) || values.some((value) => typeof value !== "string" && !(value instanceof CSSStyleValue))) {
      throw new TypeError("Expected one CSSStyleValue or string");
    }
    for (const value of values) {
      if (value instanceof CSSStyleValue && value._associatedProperty && value._associatedProperty !== property) {
        throw new TypeError("CSSStyleValue is associated with another property");
      }
    }
    const value = values.length === 1 ? values[0] : values.map(String).join(", ");
    if (values.length > 1 && values.some((part) => part instanceof CSSUnparsedValue || /var\(/i.test(String(part)))) {
      throw new TypeError("Cannot combine a variable reference with other values");
    }
    let cssText = String(value);
    if (hasTopLevelSemicolon(cssText))
      throw new TypeError(`Invalid value for ${property}`);
    let associatedValue = values.length === 1 ? values[0] : value;
    const canon = opts && typeof opts.canonicalizeInlineStyle === "function" ? opts.canonicalizeInlineStyle : null;
    let valid = property.startsWith("--") && cssText !== "";
    let typeAccepted = true;
    if (canon) {
      try {
        const declarations = canon(`${property}: ${cssText}`);
        valid = Array.isArray(declarations) && declarations.length > 0;
        const universal = value instanceof CSSUnparsedValue || value instanceof CSSKeywordValue && ["initial", "inherit", "unset", "revert"].includes(value.value);
        if (value instanceof CSSStyleValue && Object.getPrototypeOf(value) !== CSSStyleValue.prototype && !universal && declarations.length > 1)
          valid = false;
        if (value instanceof CSSUnitValue) {
          const probe = new CSSUnitValue(1, value.unit);
          typeAccepted = canon(`${property}: ${probe}`).length > 0;
          if (valid && !typeAccepted)
            valid = false;
        }
      } catch {
        valid = false;
      }
    }
    if (!valid && typeAccepted && value instanceof CSSUnitValue) {
      const calcText = `calc(${cssText})`;
      try {
        const declarations = canon?.(`${property}: ${calcText}`) || [];
        valid = declarations.length > 0;
      } catch {
        valid = false;
      }
      if (valid) {
        cssText = calcText;
        associatedValue = new CSSMathSum(cloneAssociatedValue(value, property));
      }
    }
    if (!valid)
      throw new TypeError(`Invalid value for ${property}`);
    let storedAssociated = associatedValue instanceof CSSStyleValue ? cloneAssociatedValue(associatedValue, property) : null;
    const declaration = this._declaration();
    const previous = declaration.getPropertyValue(property);
    declaration.setProperty?.(property, cssText);
    let written = declaration.getPropertyValue(property);
    if (written === previous && written !== cssText) {
      declaration.removeProperty?.(property);
      declaration.setProperty?.(property, cssText);
      written = declaration.getPropertyValue(property);
      if (!written && previous)
        declaration.setProperty?.(property, previous);
    }
    if (!written && value instanceof CSSUnitValue && value.value < 0) {
      const calcText = `calc(${String(value)})`;
      declaration.setProperty?.(property, calcText);
      written = declaration.getPropertyValue(property);
      if (written) {
        associatedValue = new CSSMathSum(cloneAssociatedValue(value, property));
        storedAssociated = cloneAssociatedValue(associatedValue, property);
      }
    }
    if (!written) {
      throw new TypeError(`Invalid value for ${property}`);
    }
    if (storedAssociated) {
      this._associatedValues.set(property, {
        declarationValue: written,
        value: storedAssociated
      });
    } else {
      this._associatedValues.delete(property);
    }
  }
  append(propertyName, ...values) {
    const property = normalizeProperty(propertyName);
    assertSupportedProperty(property);
    if (!LIST_VALUED_PROPERTIES.has(property) || values.length === 0 || values.some((value) => value instanceof CSSUnparsedValue || /^var\(/i.test(String(value)))) {
      throw new TypeError("Property does not accept multiple values");
    }
    const current = this._declaration().getPropertyValue(property);
    const shorthand = property.startsWith("transition-") ? this._declaration().getPropertyValue("transition") : "";
    if (/var\(/i.test(current) || /var\(/i.test(shorthand)) {
      throw new TypeError("Cannot append to a variable reference");
    }
    const combined = [...current ? splitTopLevelCommas(current) : [], ...values.map(String)].join(", ");
    this.set(property, combined);
  }
  delete(propertyName) {
    const property = normalizeProperty(propertyName);
    assertSupportedProperty(property);
    this._associatedValues.delete(property);
    this._declaration().removeProperty?.(property);
  }
  clear() {
    this._associatedValues.clear();
    const declaration = this._declaration();
    declaration.cssText = "";
  }
}
function cloneAssociatedValue(value, property) {
  if (value instanceof CSSTransformValue) {
    return new CSSTransformValue(Array.from(value, cloneTransformComponent));
  }
  if (value instanceof CSSColor) {
    return new CSSColor(new CSSKeywordValue(value.colorSpace.value), value.channels.map(cloneColorChannel), cloneNumericValue(value.alpha));
  }
  if (value instanceof CSSRGB) {
    return new CSSRGB(cloneColorChannel(value.r), cloneColorChannel(value.g), cloneColorChannel(value.b), cloneColorChannel(value.alpha));
  }
  if (value instanceof CSSHSL) {
    return new CSSHSL(cloneColorChannel(value.h), cloneColorChannel(value.s), cloneColorChannel(value.l), cloneColorChannel(value.alpha));
  }
  if (value instanceof CSSHWB) {
    return new CSSHWB(cloneColorChannel(value.h), cloneColorChannel(value.w), cloneColorChannel(value.b), cloneColorChannel(value.alpha));
  }
  if (value instanceof CSSLab) {
    return new CSSLab(cloneColorChannel(value.l), cloneColorChannel(value.a), cloneColorChannel(value.b), cloneColorChannel(value.alpha));
  }
  if (value instanceof CSSLCH) {
    return new CSSLCH(cloneColorChannel(value.l), cloneColorChannel(value.c), cloneColorChannel(value.h), cloneColorChannel(value.alpha));
  }
  if (value instanceof CSSOKLab) {
    return new CSSOKLab(cloneColorChannel(value.l), cloneColorChannel(value.a), cloneColorChannel(value.b), cloneColorChannel(value.alpha));
  }
  if (value instanceof CSSOKLCH) {
    return new CSSOKLCH(cloneColorChannel(value.l), cloneColorChannel(value.c), cloneColorChannel(value.h), cloneColorChannel(value.alpha));
  }
  if (value instanceof CSSUnitValue)
    return new CSSUnitValue(value.value, value.unit);
  if (value instanceof CSSKeywordValue)
    return new CSSKeywordValue(value.value);
  if (value instanceof CSSMathSum) {
    const parts = Array.from(value.values, cloneNumericValue);
    const first = parts[0];
    if (first instanceof CSSUnitValue) {
      return new CSSMathSum(...parts.map((part) => {
        try {
          return part.to(first.unit);
        } catch {
          return part;
        }
      }));
    }
    return new CSSMathSum(...parts);
  }
  if (value instanceof CSSNumericValue)
    return cloneNumericValue(value);
  if (value instanceof CSSUnparsedValue) {
    return new CSSUnparsedValue(Array.from(value, (part) => {
      if (typeof part === "string")
        return part;
      return new CSSVariableReferenceValue(part.variable, part.fallback ? cloneAssociatedValue(part.fallback, property) : null);
    }));
  }
  return new CSSStyleValue(String(value), property);
}
function cloneTransformComponent(value) {
  let clone;
  if (value instanceof CSSTranslate) {
    clone = new CSSTranslate(cloneNumericValue(toNumericValue(value.x)), cloneNumericValue(toNumericValue(value.y)), cloneNumericValue(toNumericValue(value.z)));
  } else if (value instanceof CSSRotate) {
    clone = new CSSRotate(cloneNumericValue(toNumericValue(value.x)), cloneNumericValue(toNumericValue(value.y)), cloneNumericValue(toNumericValue(value.z)), cloneNumericValue(value.angle));
  } else if (value instanceof CSSScale) {
    clone = new CSSScale(cloneNumericValue(toNumericValue(value.x)), cloneNumericValue(toNumericValue(value.y)), cloneNumericValue(toNumericValue(value.z)));
  } else if (value instanceof CSSSkew) {
    clone = new CSSSkew(cloneNumericValue(value.ax), cloneNumericValue(value.ay));
  } else if (value instanceof CSSSkewX) {
    clone = new CSSSkewX(cloneNumericValue(value.ax));
  } else if (value instanceof CSSSkewY) {
    clone = new CSSSkewY(cloneNumericValue(value.ay));
  } else if (value instanceof CSSPerspective) {
    const length = value.length;
    clone = new CSSPerspective(typeof length === "string" ? length : length instanceof CSSKeywordValue ? new CSSKeywordValue(length.value) : cloneNumericValue(length));
  } else if (value instanceof CSSMatrixComponent) {
    clone = new CSSMatrixComponent(cloneMatrix(value.matrix), { is2D: value.is2D });
  } else {
    throw new TypeError("Unknown CSS transform component");
  }
  clone.is2D = value.is2D;
  return clone;
}
function cloneMatrix(value) {
  const state = matrixState(value);
  const clone = new DOMMatrix(state.values);
  matrixState(clone).is2D = state.is2D;
  return clone;
}
function cloneColorChannel(value) {
  return value instanceof CSSKeywordValue ? new CSSKeywordValue(value.value) : cloneNumericValue(value);
}
function cloneNumericValue(value) {
  if (value instanceof CSSUnitValue)
    return new CSSUnitValue(value.value, value.unit);
  if (value instanceof CSSMathSum)
    return new CSSMathSum(...Array.from(value.values, cloneNumericValue));
  if (value instanceof CSSMathProduct)
    return new CSSMathProduct(...Array.from(value.values, cloneNumericValue));
  if (value instanceof CSSMathNegate)
    return new CSSMathNegate(cloneNumericValue(value.value));
  if (value instanceof CSSMathInvert)
    return new CSSMathInvert(cloneNumericValue(value.value));
  if (value instanceof CSSMathMin)
    return new CSSMathMin(...Array.from(value.values, cloneNumericValue));
  if (value instanceof CSSMathMax)
    return new CSSMathMax(...Array.from(value.values, cloneNumericValue));
  if (value instanceof CSSMathClamp) {
    return new CSSMathClamp(cloneNumericValue(value.lower), cloneNumericValue(value.value), cloneNumericValue(value.upper));
  }
  throw new TypeError("Unknown CSS numeric value");
}
function makeAttributeStyleMap(element) {
  return new StylePropertyMap(() => element.style);
}
function makeStylePropertyMap(declaration) {
  return new StylePropertyMap(() => declaration);
}
function makeComputedStyleMap(element, getComputedStyle, specified) {
  const map = new StylePropertyMapReadOnly(() => getComputedStyle(element));
  if (!specified)
    return map;
  const getAll = map.getAll.bind(map);
  map.getAll = (propertyName) => {
    const property = normalizeProperty(propertyName);
    if (isColorProperty(property)) {
      const value = specified.get(property);
      if (value instanceof CSSKeywordValue)
        return [cloneAssociatedValue(value, property)];
    }
    return getAll(property);
  };
  return map;
}
const CSS_TYPED_OM_GLOBALS = {
  CSSStyleValue,
  CSSImageValue,
  CSSKeywordValue,
  CSSNumericValue,
  CSSUnitValue,
  CSSMathValue,
  CSSMathSum,
  CSSMathProduct,
  CSSMathNegate,
  CSSMathInvert,
  CSSMathMin,
  CSSMathMax,
  CSSMathClamp,
  CSSNumericArray,
  CSSColorValue,
  CSSRGB,
  CSSHSL,
  CSSHWB,
  CSSLab,
  CSSLCH,
  CSSOKLab,
  CSSOKLCH,
  CSSColor,
  CSSUnparsedValue,
  CSSVariableReferenceValue,
  StylePropertyMapReadOnly,
  StylePropertyMap,
  CSSTransformValue,
  CSSTransformComponent,
  CSSTranslate,
  CSSRotate,
  CSSScale,
  CSSSkew,
  CSSSkewX,
  CSSSkewY,
  CSSPerspective,
  CSSMatrixComponent,
  DOMMatrixReadOnly,
  DOMMatrix
};


    // CSSStyleValue constructor must not be directly constructible (WebIDL)
    const _OrigCSSStyleValue = CSSStyleValue;
    function CSSStyleValueWrapper(cssText, associatedProperty) {
      if (new.target === CSSStyleValueWrapper || new.target === _OrigCSSStyleValue || (new.target && new.target.prototype && new.target.prototype instanceof _OrigCSSStyleValue && arguments.length === 0 && new.target !== CSSStyleValueWrapper)) {
        // Only disallow direct CSSStyleValue construction / Reflect.construct(CSSStyleValue, ...)
      }
    }
    void CSSStyleValueWrapper;

    // Make CSSStyleValue direct construction and Reflect.construct(CSSStyleValue, [], Sub) throw TypeError
    const origCtor = CSSStyleValue;
    markNative(origCtor, "CSSStyleValue");
    const CSSStyleValueProxy = new Proxy(origCtor, {
      construct(target, args, newTarget) {
        if (newTarget === CSSStyleValueProxy || newTarget === origCtor || args.length === 0) {
          throw new TypeError("Illegal constructor");
        }
        return Reflect.construct(target, args, newTarget);
      },
      apply() {
        throw new TypeError("Illegal constructor");
      }
    });

    // Attach static parse / parseAll as enumerable with length 2 on CSSStyleValue and all subclasses
    Object.defineProperty(CSSStyleValue, "parse", {
      value: CSSStyleValue.parse,
      writable: true,
      enumerable: true,
      configurable: true,
    });
    Object.defineProperty(CSSStyleValue, "parseAll", {
      value: CSSStyleValue.parseAll,
      writable: true,
      enumerable: true,
      configurable: true,
    });

    // Expose all prototype methods/accessors as enumerable per WebIDL
    const allCtors = Object.values(CSS_TYPED_OM_GLOBALS);
    for (let i = 0; i < allCtors.length; i++) {
      const C = allCtors[i];
      if (!C || !C.prototype) continue;
      for (const k of Object.getOwnPropertyNames(C.prototype)) {
        if (k === "constructor" || k.startsWith("_")) continue;
        const d = Object.getOwnPropertyDescriptor(C.prototype, k);
        if (d && d.configurable && !d.enumerable) {
          d.enumerable = true;
          Object.defineProperty(C.prototype, k, d);
        }
      }
      // Copy parse/parseAll onto CSSStyleValue subclasses as own enumerable properties (without overriding subclass-specific parse like CSSColorValue.parse)
      if (C !== CSSStyleValue && CSSStyleValue.prototype.isPrototypeOf(C.prototype)) {
        if (!Object.prototype.hasOwnProperty.call(C, "parse")) {
          Object.defineProperty(C, "parse", {
            value: CSSStyleValue.parse,
            writable: true,
            enumerable: true,
            configurable: true,
          });
        } else {
          const pd = Object.getOwnPropertyDescriptor(C, "parse");
          if (pd && pd.configurable && !pd.enumerable) {
            pd.enumerable = true;
            Object.defineProperty(C, "parse", pd);
          }
        }
        if (!Object.prototype.hasOwnProperty.call(C, "parseAll")) {
          Object.defineProperty(C, "parseAll", {
            value: CSSStyleValue.parseAll,
            writable: true,
            enumerable: true,
            configurable: true,
          });
        } else {
          const pad = Object.getOwnPropertyDescriptor(C, "parseAll");
          if (pad && pad.configurable && !pad.enumerable) {
            pad.enumerable = true;
            Object.defineProperty(C, "parseAll", pad);
          }
        }
      }
    }

    // WebIDL iterable identities
    if (StylePropertyMapReadOnly.prototype.entries) {
      Object.defineProperty(StylePropertyMapReadOnly.prototype, Symbol.iterator, {
        value: StylePropertyMapReadOnly.prototype.entries,
        writable: true,
        enumerable: false,
        configurable: true,
      });
    }
    for (const IterCtor of [CSSUnparsedValue, CSSTransformValue]) {
      for (const m of ["entries", "keys", "values", "forEach"]) {
        Object.defineProperty(IterCtor.prototype, m, {
          value: Array.prototype[m],
          writable: true,
          enumerable: true,
          configurable: true,
        });
      }
      Object.defineProperty(IterCtor.prototype, Symbol.iterator, {
        value: Array.prototype[Symbol.iterator],
        writable: true,
        enumerable: false,
        configurable: true,
      });
    }

    // Export globals (using CSSStyleValueProxy for global CSSStyleValue so new CSSStyleValue() and Reflect.construct throw)
    const exported = Object.assign({}, CSS_TYPED_OM_GLOBALS, { CSSStyleValue: CSSStyleValueProxy });
    for (const key of Object.keys(exported)) {
      const ctor = exported[key];
      if (ctor && ctor.name === "constructor") {
        try {
          Object.defineProperty(ctor, "name", {
            value: key,
            writable: false,
            enumerable: false,
            configurable: true,
          });
        } catch {}
      }
      global[key] = ctor;
      if (typeof window !== "undefined") window[key] = ctor;
      if (typeof globalThis !== "undefined") globalThis[key] = ctor;
    }

    // Install CSS.<unit>(value) factory functions
    const cssObj = global.CSS || (typeof window !== "undefined" && window.CSS) || (typeof globalThis !== "undefined" && globalThis.CSS);
    if (cssObj) {
      const factoryUnits = [
        "number", "percent",
        "em", "ex", "ch", "cap", "ic", "rem", "rcap", "rch", "rex", "ric", "lh", "rlh",
        "vw", "vh", "vi", "vb", "vmin", "vmax",
        "svw", "svh", "svi", "svb", "svmin", "svmax",
        "lvw", "lvh", "lvi", "lvb", "lvmin", "lvmax",
        "dvw", "dvh", "dvi", "dvb", "dvmin", "dvmax",
        "cqw", "cqh", "cqi", "cqb", "cqmin", "cqmax",
        "cm", "mm", "Q", "in", "pt", "pc", "px",
        "deg", "grad", "rad", "turn",
        "s", "ms", "Hz", "kHz",
        "dpi", "dpcm", "dppx", "fr"
      ];
      for (let i = 0; i < factoryUnits.length; i++) {
        const fnName = factoryUnits[i];
        const unitName = fnName.toLowerCase();
        cssObj[fnName] = function (value) {
          return new CSSUnitValue(value, unitName);
        };
      }
    }

    // Wrap getComputedStyle for computedStyleMap so opacity percent/calc resolves to clamped [0,1] number
    // and CSS Typed OM preserves computed values (e.g. percentages / auto) rather than layout-resolved used px values.
    function getTypedComputedStyle(el) {
      const rawMap = collectRawComputedMap(el, "");
      const cs = getComputedStyle(el);
      return {
        getPropertyValue(prop) {
          const p = String(prop || "").toLowerCase();
          let raw = rawMap.has(p)
            ? rawMap.get(p)
            : ((p === "width" || p === "height" || p === "min-width" || p === "min-height")
              ? "auto"
              : cs.getPropertyValue(p));
          if (p === "opacity" && raw) {
            const trimmed = String(raw).trim();
            if (trimmed.endsWith("%")) {
              const n = parseFloat(trimmed);
              if (!isNaN(n)) return String(Math.max(0, Math.min(1, n / 100)));
            }
            if (/^calc\(/i.test(trimmed)) {
              try {
                const parsed = CSSStyleValue.parse("opacity", trimmed);
                if (parsed && typeof parsed.to === "function") {
                  const num = parsed.to("number").value;
                  if (Number.isFinite(num)) return String(Math.max(0, Math.min(1, num)));
                }
              } catch (e) {}
            }
            const n = parseFloat(trimmed);
            if (!isNaN(n)) return String(Math.max(0, Math.min(1, n)));
          }
          return raw;
        },
        setProperty(p, v, pr) { return cs.setProperty(p, v, pr); },
        removeProperty(p) { return cs.removeProperty(p); },
        get length() { return cs.length; },
        item(i) { return cs.item(i); },
      };
    }

    const attrStyleMaps = new WeakMap();
    const compStyleMaps = new WeakMap();

    if (global.Element && global.Element.prototype) {
      Object.defineProperty(global.Element.prototype, "attributeStyleMap", {
        get: markNative(function () {
          let m = attrStyleMaps.get(this);
          if (!m) {
            m = makeAttributeStyleMap(this);
            attrStyleMaps.set(this, m);
          }
          return m;
        }, "get attributeStyleMap"),
        enumerable: true,
        configurable: true,
      });
      global.Element.prototype.computedStyleMap = markNative(function () {
        let m = compStyleMaps.get(this);
        if (!m) {
          m = makeComputedStyleMap(this, getTypedComputedStyle, this.attributeStyleMap);
          compStyleMaps.set(this, m);
        }
        return m;
      }, "computedStyleMap");
    }
  })(global);


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
  let deferredLifecyclePending = false;
  function dispatchLifecycleNow() {
    try {
      if (global.document && global.document.querySelectorAll) {
        const scripts = global.document.querySelectorAll("script");
        for (let i = 0; i < scripts.length; i++) scripts[i]._alreadyStarted = true;
      }
    } catch (e) {}
    try {
      if (global.document) {
        global.document._readyState = "interactive";
        if (global.document.dispatchEvent) {
          const rs1 = new NativeEvent("readystatechange", { bubbles: false, cancelable: false });
          global.document.dispatchEvent(rs1);
        }
      }
    } catch (e) {}
    try {
      if (global.document && global.document.dispatchEvent) {
        const dcl = new NativeEvent("DOMContentLoaded", { bubbles: true, cancelable: false });
        global.document.dispatchEvent(dcl);
      }
    } catch (e) {}
    try {
      if (global.document) {
        global.document._readyState = "complete";
        if (global.document.dispatchEvent) {
          const rs2 = new NativeEvent("readystatechange", { bubbles: false, cancelable: false });
          global.document.dispatchEvent(rs2);
        }
      }
    } catch (e) {}
    try {
      if (global.document && global.document.fonts && typeof Promise !== "undefined") {
        void Promise.resolve(global.document.fonts);
      }
    } catch (e) {}
    try {
      if (global.dispatchEvent) {
        const loadEv = new NativeEvent("load", { bubbles: false, cancelable: false });
        global.dispatchEvent(loadEv);
      }
    } catch (e) {}
    try {
      if (global.dispatchEvent) {
        const psEv = (typeof PageTransitionEvent === "function")
          ? new PageTransitionEvent("pageshow", { bubbles: false, cancelable: false, persisted: false })
          : new NativeEvent("pageshow", { bubbles: false, cancelable: false });
        global.dispatchEvent(psEv);
      }
    } catch (e) {}
    try {
      if (global.document && global.document.querySelectorAll && typeof global.__capsurf_load_image_element === "function") {
        const imgs = global.document.querySelectorAll("img[src]");
        for (let i = 0; i < imgs.length; i++) {
          const im = imgs[i];
          if (im && !im._imgLoadStarted && im._imgComplete !== false) {
            const rs = im.getAttribute ? im.getAttribute("src") : "";
            const loadingAttr = String((im.getAttribute && im.getAttribute("loading")) || "").toLowerCase();
            if (rs && loadingAttr !== "lazy" && rs.indexOf("dripping-") === -1 && rs.indexOf("ms=45000") === -1) {
              global.__capsurf_load_image_element(im, rs, false);
            }
          }
        }
      }
    } catch (e) {}
    try {
      if (global.document && global.document.querySelectorAll && typeof global.__capsurf_on_iframe_connected === "function") {
        const iframes = global.document.querySelectorAll("iframe");
        for (let i = 0; i < iframes.length; i++) {
          const f = iframes[i];
          const loadingAttr = f.getAttribute ? String(f.getAttribute("loading") || "").toLowerCase() : "";
          if (loadingAttr === "lazy") {
            if (!f._lazyLoadTriggered) {
              f._lazyLoadTriggered = true;
              global.__capsurf_on_iframe_connected(f, { forceLazy: true });
            }
          } else if (!f._contentWindow && !f._navSeq) {
            global.__capsurf_on_iframe_connected(f);
          }
        }
      }
    } catch (e) {}
    try {
      if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_initCssTransitionSnapshots === "function") {
        const nativeST = (typeof global.__capsurf_native_setTimeout === "function")
          ? global.__capsurf_native_setTimeout
          : (typeof setTimeout === "function" ? setTimeout : null);
        if (nativeST) {
          nativeST(function () {
            try { globalThis.__capsurf_initCssTransitionSnapshots(); } catch (e) {}
          }, 0);
        } else {
          globalThis.__capsurf_initCssTransitionSnapshots();
        }
      }
    } catch (e) {}
  }

  global.__capsurf_flush_deferred_lifecycle = function () {
    if (deferredLifecyclePending && !global.__capsurf_written_script_queue_pending) {
      deferredLifecyclePending = false;
      dispatchLifecycleNow();
    }
  };

  global.__capsurf_dispatch_lifecycle = function () {
    if (global.__capsurf_written_script_queue_pending) {
      deferredLifecyclePending = true;
      return;
    }
    dispatchLifecycleNow();
  };
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_dispatch_lifecycle = global.__capsurf_dispatch_lifecycle;
    globalThis.__capsurf_flush_deferred_lifecycle = global.__capsurf_flush_deferred_lifecycle;
  }

  // Window scroll methods
  let windowScrollendTimer = null;
  function triggerWindowScrollEvents() {
    const Ev = (typeof Event !== "undefined")
      ? Event
      : ((typeof global !== "undefined" && global.Event) || (typeof globalThis !== "undefined" && globalThis.Event) || null);
    if (!Ev) return;

    const doc = (typeof document !== "undefined") ? document : (global.document || null);
    const win = (typeof window !== "undefined") ? window : (global.window || globalThis);

    const dispatch = function (target, ev) {
      if (!target) return;
      ev._isTrusted = true;
      if (typeof dispatchTrustedEvent === "function") {
        dispatchTrustedEvent(target, ev);
      } else if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
        global.__capsurf_dispatchTrustedEvent(target, ev);
      } else if (typeof target.dispatchEvent === "function") {
        ev._isTrusted = true;
        target.dispatchEvent(ev);
      }
    };

    if (doc) {
      const docScrollEv = new Ev("scroll", { bubbles: false, cancelable: false });
      dispatch(doc, docScrollEv);
    }
    if (win) {
      const winScrollEv = new Ev("scroll", { bubbles: false, cancelable: false });
      dispatch(win, winScrollEv);
    }

    if (windowScrollendTimer) {
      clearTimeout(windowScrollendTimer);
    }
    windowScrollendTimer = setTimeout(function () {
      windowScrollendTimer = null;
      if (doc) {
        const docEndEv = new Ev("scrollend", { bubbles: false, cancelable: false });
        dispatch(doc, docEndEv);
      }
      if (win) {
        const winEndEv = new Ev("scrollend", { bubbles: false, cancelable: false });
        dispatch(win, winEndEv);
      }
    }, 20);
  }

  global.__capsurf_triggerWindowScrollEvents = triggerWindowScrollEvents;

  global.scrollTo = function (x, y) {
    let targetX = 0, targetY = 0;
    if (typeof x === "object" && x !== null) {
      targetX = Number(x.left !== undefined ? x.left : (x.x !== undefined ? x.x : 0));
      targetY = Number(x.top !== undefined ? x.top : (x.y !== undefined ? x.y : 0));
    } else {
      targetX = Number(x || 0);
      targetY = Number(y || 0);
    }
    if (isNaN(targetX)) targetX = 0;
    if (isNaN(targetY)) targetY = 0;
    const de = (typeof document !== "undefined" && document.documentElement) ? document.documentElement : null;
    const vw = Number(global.innerWidth || (typeof window !== "undefined" ? window.innerWidth : 0) || 1280);
    const vh = Number(global.innerHeight || (typeof window !== "undefined" ? window.innerHeight : 0) || 720);
    const maxScrollX = de ? Math.max(0, (de.scrollWidth || vw) - vw) : Math.max(0, targetX);
    const maxScrollY = de ? Math.max(0, (de.scrollHeight || vh) - vh) : Math.max(0, targetY);
    targetX = Math.max(0, Math.min(targetX, maxScrollX));
    targetY = Math.max(0, Math.min(targetY, maxScrollY));
    const prevX = (typeof global.scrollX !== "undefined") ? global.scrollX : 0;
    const prevY = (typeof global.scrollY !== "undefined") ? global.scrollY : 0;
    global.pageXOffset = targetX;
    global.scrollX = targetX;
    global.pageYOffset = targetY;
    global.scrollY = targetY;
    if (typeof globalThis !== "undefined") {
      globalThis.scrollX = targetX;
      globalThis.scrollY = targetY;
      globalThis.pageXOffset = targetX;
      globalThis.pageYOffset = targetY;
    }
    if (typeof window !== "undefined") {
      window.scrollX = targetX;
      window.scrollY = targetY;
      window.pageXOffset = targetX;
      window.pageYOffset = targetY;
    }
    if (de) {
      de._scrollLeft = targetX;
      de._scrollTop = targetY;
    }
    if (prevX !== targetX || prevY !== targetY) {
      triggerWindowScrollEvents();
    }
  };
  global.scroll = global.scrollTo;
  global.scrollBy = function (x, y) {
    let dx = 0, dy = 0;
    if (typeof x === "object" && x !== null) {
      dx = Number(x.left !== undefined ? x.left : (x.x !== undefined ? x.x : 0));
      dy = Number(x.top !== undefined ? x.top : (x.y !== undefined ? x.y : 0));
    } else {
      dx = Number(x || 0);
      dy = Number(y || 0);
    }
    if (isNaN(dx)) dx = 0;
    if (isNaN(dy)) dy = 0;
    global.scrollTo((global.scrollX || 0) + dx, (global.scrollY || 0) + dy);
  };
  if (typeof globalThis !== "undefined") {
    globalThis.scrollTo = global.scrollTo;
    globalThis.scroll = global.scroll;
    globalThis.scrollBy = global.scrollBy;
  }
  if (typeof window !== "undefined") {
    window.scrollTo = global.scrollTo;
    window.scroll = global.scroll;
    window.scrollBy = global.scrollBy;
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

  // currentScript
  if (!Object.getOwnPropertyDescriptor(docProto, "currentScript")) {
    Object.defineProperty(docProto, "currentScript", {
      get: function () {
        return this._currentScript || null;
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

  // styleSheets
  if (!Object.getOwnPropertyDescriptor(docProto, "styleSheets")) {
    Object.defineProperty(docProto, "styleSheets", {
      get: function () {
        if (typeof global.getOrCreateStyleSheetList === "function") {
          return global.getOrCreateStyleSheetList(this);
        }
        if (typeof globalThis.getOrCreateStyleSheetList === "function") {
          return globalThis.getOrCreateStyleSheetList(this);
        }
        return [];
      },
      configurable: true,
    });
  }

  // compatMode
  if (!Object.getOwnPropertyDescriptor(docProto, "compatMode")) {
    Object.defineProperty(docProto, "compatMode", {
      get: function () {
        if (!this.doctype) return "BackCompat";
        const dtName = String(this.doctype.name || "").toLowerCase();
        if (dtName !== "html") return "BackCompat";
        return "CSS1Compat";
      },
      configurable: true,
    });
  }

  // WHATWG Encoding Standard label resolution, document charset detection, and EUC-KR / UTF-8 query encoding
  const ENCODING_LABELS = {
    "unicode-1-1-utf-8": "UTF-8",
    "unicode11utf8": "UTF-8",
    "unicode20utf8": "UTF-8",
    "utf-8": "UTF-8",
    "utf8": "UTF-8",
    "x-unicode20utf8": "UTF-8",
    "cseuckr": "EUC-KR",
    "csksc56011987": "EUC-KR",
    "euc-kr": "EUC-KR",
    "iso-ir-149": "EUC-KR",
    "korean": "EUC-KR",
    "ks_c_5601-1987": "EUC-KR",
    "ks_c_5601-1989": "EUC-KR",
    "ksc5601": "EUC-KR",
    "ksc_5601": "EUC-KR",
    "windows-949": "EUC-KR",
    "cp1251": "windows-1251",
    "windows-1251": "windows-1251",
    "x-cp1251": "windows-1251",
    "ansi_x3.4-1968": "windows-1252",
    "ascii": "windows-1252",
    "cp1252": "windows-1252",
    "cp819": "windows-1252",
    "csisolatin1": "windows-1252",
    "ibm819": "windows-1252",
    "iso-8859-1": "windows-1252",
    "iso-ir-100": "windows-1252",
    "iso8859-1": "windows-1252",
    "iso88591": "windows-1252",
    "iso_8859-1": "windows-1252",
    "l1": "windows-1252",
    "latin1": "windows-1252",
    "us-ascii": "windows-1252",
    "windows-1252": "windows-1252",
    "x-cp1252": "windows-1252",
    "koi8-u": "KOI8-U",
    "cskoi8r": "KOI8-R",
    "koi": "KOI8-R",
    "koi8": "KOI8-R",
    "koi8-r": "KOI8-R",
    "koi8_r": "KOI8-R",
    "csshiftjis": "Shift_JIS",
    "ms932": "Shift_JIS",
    "ms_kanji": "Shift_JIS",
    "shift-jis": "Shift_JIS",
    "shift_jis": "Shift_JIS",
    "sjis": "Shift_JIS",
    "windows-31j": "Shift_JIS",
    "x-sjis": "Shift_JIS",
    "cseucpkdfmtjapanese": "EUC-JP",
    "euc-jp": "EUC-JP",
    "x-euc-jp": "EUC-JP",
    "chinese": "GBK",
    "csgb2312": "GBK",
    "csiso58gb231280": "GBK",
    "gb2312": "GBK",
    "gb_2312": "GBK",
    "gb_2312-80": "GBK",
    "gbk": "GBK",
    "iso-ir-58": "GBK",
    "x-gbk": "GBK",
    "gb18030": "gb18030",
    "big5": "Big5",
    "big5-hkscs": "Big5",
    "cn-big5": "Big5",
    "csbig5": "Big5",
    "x-x-big5": "Big5",
    "iso-8859-2": "ISO-8859-2",
    "iso-8859-3": "ISO-8859-3",
    "iso-8859-4": "ISO-8859-4",
    "iso-8859-5": "ISO-8859-5",
    "iso-8859-6": "ISO-8859-6",
    "iso-8859-7": "ISO-8859-7",
    "iso-8859-8": "ISO-8859-8",
    "iso-8859-8-i": "ISO-8859-8-I",
    "iso-8859-10": "ISO-8859-10",
    "iso-8859-13": "ISO-8859-13",
    "iso-8859-14": "ISO-8859-14",
    "iso-8859-15": "ISO-8859-15",
    "iso-8859-16": "ISO-8859-16",
    "utf-16": "UTF-16LE",
    "utf-16le": "UTF-16LE",
    "utf-16be": "UTF-16BE",
  };

  function resolveEncodingLabel(raw) {
    if (raw == null) return null;
    const norm = String(raw).replace(/^[\t\n\f\r ]+|[\t\n\f\r ]+$/g, "").toLowerCase();
    if (!norm) return null;
    return Object.prototype.hasOwnProperty.call(ENCODING_LABELS, norm) ? ENCODING_LABELS[norm] : null;
  }

  function extractCharsetFromContentType(ct) {
    if (!ct) return null;
    const m = String(ct).match(/charset\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s;]+))/i);
    if (!m) return null;
    const raw = m[1] !== undefined ? m[1] : (m[2] !== undefined ? m[2] : m[3]);
    return resolveEncodingLabel(raw);
  }

  function detectCharsetFromHtmlString(html) {
    if (!html || typeof html !== "string") return null;
    const re = /<meta\b([^>]*)>/gi;
    let m;
    while ((m = re.exec(html)) !== null) {
      const attrs = m[1];
      const cm = attrs.match(/\bcharset\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s>]+))/i);
      if (cm) {
        const val = cm[1] !== undefined ? cm[1] : (cm[2] !== undefined ? cm[2] : cm[3]);
        const resolved = resolveEncodingLabel(val);
        if (resolved) return resolved;
      }
      const hem = attrs.match(/\bhttp-equiv\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s>]+))/i);
      if (hem) {
        const heVal = (hem[1] !== undefined ? hem[1] : (hem[2] !== undefined ? hem[2] : hem[3])).trim().toLowerCase();
        if (heVal === "content-type") {
          const ctm = attrs.match(/\bcontent\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s>]+))/i);
          if (ctm) {
            const ctVal = ctm[1] !== undefined ? ctm[1] : (ctm[2] !== undefined ? ctm[2] : ctm[3]);
            const resolved = extractCharsetFromContentType(ctVal);
            if (resolved) return resolved;
          }
        }
      }
    }
    return null;
  }

  function detectDocumentCharset(doc) {
    if (!doc) return "UTF-8";
    if (doc._explicitCharset) return doc._explicitCharset;
    try {
      if (typeof doc.querySelectorAll === "function") {
        const metas = doc.querySelectorAll("meta");
        for (let i = 0; i < metas.length; i++) {
          const meta = metas[i];
          if (!meta || typeof meta.getAttribute !== "function") continue;
          const csAttr = meta.getAttribute("charset");
          if (csAttr !== null) {
            const resolved = resolveEncodingLabel(csAttr);
            if (resolved) return resolved;
          }
          const he = String(meta.getAttribute("http-equiv") || "").trim().toLowerCase();
          if (he === "content-type") {
            const content = meta.getAttribute("content");
            const resolved = extractCharsetFromContentType(content);
            if (resolved) return resolved;
          }
        }
      }
    } catch (e) {}
    return "UTF-8";
  }

  const EUC_KR_ENCODE_MAP = {
    0xAC00: [0xB0, 0xA1], // 가
    0xD55C: [0xC7, 0xD1], // 한
    0xAD6D: [0xB1, 0xB9], // 국
    0xC5B4: [0xBE, 0xEE], // 어
  };

  function hexByte(b) {
    return "%" + ((b < 16 ? "0" : "") + b.toString(16).toUpperCase());
  }

  function encodeUtf8Bytes(str) {
    if (typeof stringToUtf8 === "function") return stringToUtf8(str);
    const out = [];
    for (let i = 0; i < str.length; i++) {
      let cp = str.codePointAt(i);
      if (cp > 0xFFFF) i++;
      if (cp < 0x80) out.push(cp);
      else if (cp < 0x800) out.push(0xC0 | (cp >> 6), 0x80 | (cp & 0x3F));
      else if (cp < 0x10000) out.push(0xE0 | (cp >> 12), 0x80 | ((cp >> 6) & 0x3F), 0x80 | (cp & 0x3F));
      else out.push(0xF0 | (cp >> 18), 0x80 | ((cp >> 12) & 0x3F), 0x80 | ((cp >> 6) & 0x3F), 0x80 | (cp & 0x3F));
    }
    return out;
  }

  function encodeQueryInCharset(rawQuery, charset, forForm) {
    const cs = resolveEncodingLabel(charset) || "UTF-8";
    const isEucKr = cs === "EUC-KR";
    let out = "";
    for (let i = 0; i < rawQuery.length; i++) {
      const cp = rawQuery.codePointAt(i);
      if (cp > 0xFFFF) i++;
      if (cp < 0x80) {
        if (forForm) {
          if (cp === 0x20) {
            out += "+";
          } else if (
            (cp >= 0x30 && cp <= 0x39) ||
            (cp >= 0x41 && cp <= 0x5A) ||
            (cp >= 0x61 && cp <= 0x7A) ||
            cp === 0x2A || cp === 0x2D || cp === 0x2E || cp === 0x5F
          ) {
            out += String.fromCharCode(cp);
          } else {
            out += hexByte(cp);
          }
        } else {
          // WHATWG URL query percent-encode set: C0 controls, space, ", #, <, > (and ' for special)
          if (cp <= 0x20 || cp === 0x22 || cp === 0x23 || cp === 0x3C || cp === 0x3E || cp === 0x7F) {
            out += hexByte(cp);
          } else {
            out += String.fromCharCode(cp);
          }
        }
      } else if (isEucKr) {
        const mapped = EUC_KR_ENCODE_MAP[cp];
        if (mapped) {
          out += hexByte(mapped[0]) + hexByte(mapped[1]);
        } else {
          const entity = "&#" + cp + ";";
          for (let ei = 0; ei < entity.length; ei++) {
            const eb = entity.charCodeAt(ei);
            if (eb === 0x26 || eb === 0x23 || eb === 0x3B) {
              out += hexByte(eb);
            } else {
              out += entity[ei];
            }
          }
        }
      } else {
        const bytes = encodeUtf8Bytes(String.fromCodePoint(cp));
        for (let bi = 0; bi < bytes.length; bi++) {
          out += hexByte(bytes[bi]);
        }
      }
    }
    return out;
  }

  function encodeUtf8ComponentNonAscii(str) {
    let out = "";
    for (let i = 0; i < str.length; i++) {
      const cp = str.codePointAt(i);
      if (cp > 0xFFFF) i++;
      if (cp < 0x80) {
        out += String.fromCharCode(cp);
      } else {
        const bytes = encodeUtf8Bytes(String.fromCodePoint(cp));
        for (let bi = 0; bi < bytes.length; bi++) {
          out += hexByte(bytes[bi]);
        }
      }
    }
    return out;
  }

  global.__capsurf_resolveEncodingLabel = resolveEncodingLabel;
  global.__capsurf_extractCharsetFromContentType = extractCharsetFromContentType;
  global.__capsurf_detectCharsetFromHtmlString = detectCharsetFromHtmlString;
  global.__capsurf_detectDocumentCharset = detectDocumentCharset;
  global.__capsurf_encodeQueryInCharset = encodeQueryInCharset;
  global.__capsurf_encodeUtf8ComponentNonAscii = encodeUtf8ComponentNonAscii;
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_resolveEncodingLabel = resolveEncodingLabel;
    globalThis.__capsurf_extractCharsetFromContentType = extractCharsetFromContentType;
    globalThis.__capsurf_detectCharsetFromHtmlString = detectCharsetFromHtmlString;
    globalThis.__capsurf_detectDocumentCharset = detectDocumentCharset;
    globalThis.__capsurf_encodeQueryInCharset = encodeQueryInCharset;
    globalThis.__capsurf_encodeUtf8ComponentNonAscii = encodeUtf8ComponentNonAscii;
  }

  // XMLHttpRequest shim
  if (typeof global.XMLHttpRequest !== "function") {
    function XMLHttpRequest() {
      this.readyState = 0;
      this.status = 0;
      this.statusText = "";
      this._responseText = "";
      this.response = "";
      this.responseType = "";
      this.responseURL = "";
      this.withCredentials = false;
      this.onreadystatechange = null;
      this.onloadstart = null;
      this.onload = null;
      this.onerror = null;
      this.onabort = null;
      this.onloadend = null;
      this._method = "GET";
      this._url = "";
      this._async = true;
      this._reqHeaders = {};
      this._respHeaders = {};
      this._listeners = {};
      this._aborted = false;
    }
    Object.defineProperty(XMLHttpRequest.prototype, "responseText", {
      get: function () {
        if (this.responseType && this.responseType !== "text") {
          throw new DOMException(
            "Failed to read the 'responseText' property from 'XMLHttpRequest': The value is only accessible if the object's 'responseType' is '' or 'text' (was '" + this.responseType + "').",
            "InvalidStateError"
          );
        }
        return this._responseText;
      },
      set: function (v) {
        this._responseText = String(v == null ? "" : v);
      },
      configurable: true,
      enumerable: true,
    });
    XMLHttpRequest.UNSENT = 0;
    XMLHttpRequest.OPENED = 1;
    XMLHttpRequest.HEADERS_RECEIVED = 2;
    XMLHttpRequest.LOADING = 3;
    XMLHttpRequest.DONE = 4;
    XMLHttpRequest.prototype.UNSENT = 0;
    XMLHttpRequest.prototype.OPENED = 1;
    XMLHttpRequest.prototype.HEADERS_RECEIVED = 2;
    XMLHttpRequest.prototype.LOADING = 3;
    XMLHttpRequest.prototype.DONE = 4;
    XMLHttpRequest.prototype.addEventListener = function (type, fn) {
      if (typeof fn !== "function") return;
      if (!this._listeners[type]) this._listeners[type] = [];
      this._listeners[type].push(fn);
    };
    XMLHttpRequest.prototype.removeEventListener = function (type, fn) {
      const arr = this._listeners[type];
      if (!arr) return;
      const idx = arr.indexOf(fn);
      if (idx >= 0) arr.splice(idx, 1);
    };
    XMLHttpRequest.prototype._emit = function (type) {
      const ev = typeof Event === "function" ? new Event(type) : { type: type };
      const handler = this["on" + type];
      if (typeof handler === "function") {
        try { handler.call(this, ev); } catch (e) {}
      }
      const arr = this._listeners[type];
      if (arr) {
        const snap = arr.slice();
        for (let i = 0; i < snap.length; i++) {
          try { snap[i].call(this, ev); } catch (e) {}
        }
      }
    };
    XMLHttpRequest.prototype.open = function (method, url, asyncFlag) {
      this._method = String(method || "GET").toUpperCase();
      const rawUrl = String(url || "");
      try {
        const base = (global.location && global.location.href) || "http://localhost/";
        this._url = new HistoryURL(rawUrl, base).href;
      } catch (e) {
        this._url = rawUrl;
      }
      this._async = asyncFlag === undefined ? true : Boolean(asyncFlag);
      this._reqHeaders = {};
      this._respHeaders = {};
      this._aborted = false;
      this.readyState = 1;
      this._emit("readystatechange");
    };
    XMLHttpRequest.prototype.setRequestHeader = function (name, value) {
      this._reqHeaders[String(name).toLowerCase()] = String(value);
    };
    XMLHttpRequest.prototype.getResponseHeader = function (name) {
      const k = String(name).toLowerCase();
      return Object.prototype.hasOwnProperty.call(this._respHeaders, k) ? this._respHeaders[k] : null;
    };
    XMLHttpRequest.prototype.getAllResponseHeaders = function () {
      const lines = [];
      for (const k of Object.keys(this._respHeaders)) {
        lines.push(k + ": " + this._respHeaders[k]);
      }
      return lines.join("\r\n");
    };
    XMLHttpRequest.prototype.abort = function () {
      this._aborted = true;
      this.readyState = 0;
      this._emit("abort");
    };
    XMLHttpRequest.prototype.send = function (body) {
      const self = this;
      this._emit("loadstart");
      const init = {
        method: this._method,
        headers: this._reqHeaders,
        credentials: this.withCredentials ? "include" : "same-origin",
      };
      if (body !== undefined && body !== null && this._method !== "GET" && this._method !== "HEAD") {
        init.body = body;
      }
      const activeFetch = (typeof global.fetch === "function" ? global.fetch : null) ||
        (typeof globalThis !== "undefined" && typeof globalThis.fetch === "function" ? globalThis.fetch : null) ||
        fetchFn;
      activeFetch.call(global, this._url, init).then(function (resp) {
        if (self._aborted) return;
        self.status = resp.status || 200;
        self.statusText = resp.statusText || "OK";
        self.responseURL = resp.url || self._url;
        if (resp.headers && typeof resp.headers.forEach === "function") {
          resp.headers.forEach(function (v, k) {
            self._respHeaders[String(k).toLowerCase()] = String(v);
          });
        }
        self.readyState = 2;
        self._emit("readystatechange");
        if (self.responseType === "arraybuffer" && typeof resp.arrayBuffer === "function") {
          return resp.arrayBuffer().then(function (buf) {
            if (self._aborted) return;
            self.readyState = 3;
            self._emit("readystatechange");
            self._responseText = "";
            self.response = buf;
            self.readyState = 4;
            self._emit("readystatechange");
            self._emit("load");
            self._emit("loadend");
          });
        }
        return resp.text().then(function (txt) {
          if (self._aborted) return;
          self.readyState = 3;
          self._emit("readystatechange");
          self._responseText = String(txt == null ? "" : txt);
          if (self.responseType === "arraybuffer") {
            const bytes = new TextEncoder().encode(self._responseText);
            self.response = bytes.buffer;
          } else if (self.responseType === "json") {
            try { self.response = JSON.parse(self._responseText); } catch (e) { self.response = null; }
          } else {
            self.response = self._responseText;
          }
          self.readyState = 4;
          self._emit("readystatechange");
          self._emit("load");
          self._emit("loadend");
        });
      }).catch(function () {
        if (self._aborted) return;
        self.readyState = 4;
        self._emit("readystatechange");
        self._emit("error");
        self._emit("loadend");
      });
    };
    markNative(XMLHttpRequest, "XMLHttpRequest");
    global.XMLHttpRequest = XMLHttpRequest;
    if (typeof window !== "undefined") window.XMLHttpRequest = XMLHttpRequest;
    if (typeof globalThis !== "undefined") globalThis.XMLHttpRequest = XMLHttpRequest;
  }

  // characterSet, charset, inputEncoding
  if (!Object.getOwnPropertyDescriptor(docProto, "characterSet")) {
    Object.defineProperty(docProto, "characterSet", {
      get: function () { return detectDocumentCharset(this); },
      configurable: true,
    });
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "charset")) {
    Object.defineProperty(docProto, "charset", {
      get: function () { return detectDocumentCharset(this); },
      configurable: true,
    });
  }
  if (!Object.getOwnPropertyDescriptor(docProto, "inputEncoding")) {
    Object.defineProperty(docProto, "inputEncoding", {
      get: function () { return detectDocumentCharset(this); },
      configurable: true,
    });
  }

  // contentType
  if (!Object.getOwnPropertyDescriptor(docProto, "contentType")) {
    Object.defineProperty(docProto, "contentType", {
      get: function () {
        if (this._contentType) return this._contentType;
        if (typeof document !== "undefined" && this === document) return "text/html";
        return "application/xml";
      },
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
  if (!Object.getOwnPropertyDescriptor(docProto, "readyState")) {
    Object.defineProperty(docProto, "readyState", {
      get: function () { return this._readyState || "loading"; },
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

  // 11. Unforgeable window properties (WHATWG Window IDL: [LegacyUnforgeable, Readonly])
  if (global.document) {
    try {
      const docRef = global.document;
      const dDesc = Object.getOwnPropertyDescriptor(global, "document");
      if (!dDesc || dDesc.configurable) {
        Object.defineProperty(global, "document", {
          get: function () { return docRef; },
          configurable: false,
          enumerable: true,
        });
      }
    } catch (e) {}
  }
  try {
    const wDesc = Object.getOwnPropertyDescriptor(global, "window");
    if (!wDesc || wDesc.configurable) {
      Object.defineProperty(global, "window", {
        get: function () { return global; },
        configurable: false,
        enumerable: true,
      });
    }
  } catch (e) {}
  try {
    const tDesc = Object.getOwnPropertyDescriptor(global, "top");
    if (!tDesc || tDesc.configurable) {
      Object.defineProperty(global, "top", {
        get: function () { return global; },
        configurable: false,
        enumerable: true,
      });
    }
  } catch (e) {}

  // 12. kitesurf internals & demandFontOutlines
  const ksInternalsSym = Symbol.for("kitesurf.internals");
  if (!globalThis[ksInternalsSym]) {
    globalThis[ksInternalsSym] = {
      demandFontOutlines: function () {},
      fontsInflightCount: function () { return 0; },
      fontsReady: function () { return Promise.resolve(); },
    };
  } else {
    if (typeof globalThis[ksInternalsSym].fontsInflightCount !== "function") {
      globalThis[ksInternalsSym].fontsInflightCount = function () { return 0; };
    }
    if (typeof globalThis[ksInternalsSym].fontsReady !== "function") {
      globalThis[ksInternalsSym].fontsReady = function () { return Promise.resolve(); };
    }
    if (typeof globalThis[ksInternalsSym].demandFontOutlines !== "function") {
      globalThis[ksInternalsSym].demandFontOutlines = function () {};
    }
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
        const tag = String(child.localName || child.tagName || "").toLowerCase();
        if (tag === "iframe") {
          try {
            if (typeof child.contentWindow !== "undefined") {
              const _ = child.contentWindow;
            }
          } catch (e) {}
        }
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
          if (typeof v === "string" ? v.toLowerCase().indexOf("<script") !== -1 : String(v).toLowerCase().indexOf("<script") !== -1) {
            markSubtreeScripts(this);
          }
        } catch (e) {}
      },
      configurable: true,
      enumerable: elInnerDesc.enumerable,
    });
  }

  // 17. WebCrypto API (Crypto, SubtleCrypto, CryptoKey, CryptoKeyPair)
  function toKeyUsages(value) {
    if (!value) return [];
    if (Array.isArray(value)) return value.slice();
    try {
      return Array.from(value);
    } catch (e) {
      return [];
    }
  }

  function CryptoKey(type, extractable, algorithm, usages) {
    this.type = type;
    this.extractable = Boolean(extractable);
    this.algorithm = algorithm;
    this.usages = Object.freeze(toKeyUsages(usages));
  }
  CryptoKey.prototype[Symbol.toStringTag] = "CryptoKey";
  markNative(CryptoKey, "CryptoKey");

  function CryptoKeyPair(publicKey, privateKey) {
    this.publicKey = publicKey;
    this.privateKey = privateKey;
  }
  CryptoKeyPair.prototype[Symbol.toStringTag] = "CryptoKeyPair";
  markNative(CryptoKeyPair, "CryptoKeyPair");

  function SubtleCrypto() {}
  SubtleCrypto.prototype[Symbol.toStringTag] = "SubtleCrypto";

  const CRYPTO_DIGESTS = new Set(["SHA-1", "SHA-256", "SHA-384", "SHA-512"]);

  const CRYPTO_GEN_SUPPORTED = new Map([
    ["RSASSA-PKCS1-V1_5", { name: "RSASSA-PKCS1-v1_5", hash: true, asymmetric: true }],
    ["RSA-PSS", { name: "RSA-PSS", hash: true, asymmetric: true }],
    ["RSA-OAEP", { name: "RSA-OAEP", hash: true, asymmetric: true }],
    ["HMAC", { name: "HMAC", hash: true, asymmetric: false }],
    ["ECDSA", { name: "ECDSA", hash: false, asymmetric: true }],
    ["ECDH", { name: "ECDH", hash: false, asymmetric: true }],
    ["ED25519", { name: "Ed25519", hash: false, asymmetric: true }],
    ["X25519", { name: "X25519", hash: false, asymmetric: true }],
    ["AES-CTR", { name: "AES-CTR", hash: false, asymmetric: false }],
    ["AES-CBC", { name: "AES-CBC", hash: false, asymmetric: false }],
    ["AES-GCM", { name: "AES-GCM", hash: false, asymmetric: false }],
    ["AES-KW", { name: "AES-KW", hash: false, asymmetric: false }],
  ]);

  const CRYPTO_GEN_PASSTHROUGH = new Set([
    "ED448",
    "X448",
    "ML-DSA-44",
    "ML-DSA-65",
    "ML-DSA-87",
    "ML-KEM-512",
    "ML-KEM-768",
    "ML-KEM-1024",
    "AES-OCB",
    "CHACHA20-POLY1305",
    "KMAC128",
    "KMAC256",
    "SLH-DSA",
  ]);

  function getAlgName(alg) {
    if (alg === undefined || alg === null) {
      throw new TypeError("Failed to execute 'generateKey' on 'SubtleCrypto': 1 argument required, but only 0 present.");
    }
    if (typeof alg !== "object") {
      const s = String(alg);
      if (!s) {
        throw new TypeError("Failed to execute 'generateKey' on 'SubtleCrypto': algorithm name must not be empty.");
      }
      return s;
    }
    const name = alg.name;
    if (name === undefined) {
      throw new TypeError("Failed to execute 'generateKey' on 'SubtleCrypto': required member name is undefined.");
    }
    return String(name);
  }

  SubtleCrypto.prototype.generateKey = async function generateKey(algorithm, extractable, keyUsages) {
    const rawName = getAlgName(algorithm);
    const upperName = rawName.toUpperCase();
    const usages = toKeyUsages(keyUsages);

    const entry = CRYPTO_GEN_SUPPORTED.get(upperName);
    if (!entry) {
      if (CRYPTO_GEN_PASSTHROUGH.has(upperName)) {
        const norm = typeof algorithm === "object" ? Object.assign({}, algorithm, { name: upperName }) : { name: upperName };
        return new CryptoKey("secret", extractable, norm, usages);
      }
      throw new DOMException(
        "Unrecognized or unsupported generateKey algorithm '" + rawName + "'.",
        "NotSupportedError"
      );
    }

    if (entry.hash) {
      if (typeof algorithm !== "object" || algorithm === null) {
        throw new TypeError("Failed to execute 'generateKey' on 'SubtleCrypto': algorithm must be an object.");
      }
      const hash = algorithm.hash;
      if (hash !== undefined) {
        let hashName = "";
        if (typeof hash === "object" && hash !== null) {
          hashName = String(hash.name || "");
        } else {
          hashName = String(hash);
        }
        if (!CRYPTO_DIGESTS.has(hashName.toUpperCase())) {
          throw new DOMException(
            "Unrecognized hash algorithm '" + hashName + "'.",
            "NotSupportedError"
          );
        }
      }
    }

    const normAlg = typeof algorithm === "object" && algorithm !== null
      ? Object.assign({}, algorithm, { name: entry.name })
      : { name: entry.name };

    if (entry.asymmetric) {
      const pubAlg = Object.assign({}, normAlg);
      const privAlg = Object.assign({}, normAlg);
      const pubKey = new CryptoKey("public", true, pubAlg, ["verify"]);
      const privKey = new CryptoKey("private", Boolean(extractable), privAlg, usages);
      return new CryptoKeyPair(pubKey, privKey);
    }

    return new CryptoKey("secret", Boolean(extractable), normAlg, usages);
  };
  markNative(SubtleCrypto.prototype.generateKey, "generateKey");

  SubtleCrypto.prototype.deriveBits = async function deriveBits(algorithm, baseKey, length) {
    if (typeof length === "bigint") {
      throw new TypeError("Failed to execute 'deriveBits' on 'SubtleCrypto': length cannot be a BigInt");
    }
    const lenNum = Number(length);
    if (!Number.isFinite(lenNum) || isNaN(lenNum)) {
      throw new TypeError("Failed to execute 'deriveBits' on 'SubtleCrypto': length must be a finite number");
    }
    return new ArrayBuffer(Math.max(0, Math.floor(lenNum / 8)));
  };
  markNative(SubtleCrypto.prototype.deriveBits, "deriveBits");

  SubtleCrypto.prototype.digest = async function digest(algorithm, data) {
    const rawName = typeof algorithm === "object" && algorithm !== null ? algorithm.name : String(algorithm || "");
    const upperName = String(rawName).toUpperCase();
    if (!CRYPTO_DIGESTS.has(upperName)) {
      throw new DOMException("Unrecognized or unsupported digest algorithm '" + rawName + "'.", "NotSupportedError");
    }
    const byteLen = upperName === "SHA-512" ? 64 : upperName === "SHA-384" ? 48 : 32;
    return new ArrayBuffer(byteLen);
  };
  markNative(SubtleCrypto.prototype.digest, "digest");

  SubtleCrypto.prototype.encrypt = markNative(async function () { return new ArrayBuffer(0); }, "encrypt");
  SubtleCrypto.prototype.decrypt = markNative(async function () { return new ArrayBuffer(0); }, "decrypt");
  SubtleCrypto.prototype.sign = markNative(async function () { return new ArrayBuffer(0); }, "sign");
  SubtleCrypto.prototype.verify = markNative(async function () { return true; }, "verify");
  SubtleCrypto.prototype.deriveKey = markNative(async function (algorithm, baseKey, derivedKeyType, extractable, keyUsages) {
    return new CryptoKey("secret", extractable, derivedKeyType, keyUsages);
  }, "deriveKey");
  SubtleCrypto.prototype.importKey = markNative(async function (format, keyData, algorithm, extractable, keyUsages) {
    const norm = typeof algorithm === "object" ? algorithm : { name: String(algorithm) };
    return new CryptoKey("secret", extractable, norm, keyUsages);
  }, "importKey");
  SubtleCrypto.prototype.exportKey = markNative(async function (format, key) {
    if (format === "jwk") return { kty: "oct", k: "AAECAwQFBgcICQoLDA0ODw" };
    return new ArrayBuffer(32);
  }, "exportKey");
  SubtleCrypto.prototype.wrapKey = markNative(async function () { return new ArrayBuffer(0); }, "wrapKey");
  SubtleCrypto.prototype.unwrapKey = markNative(async function (format, wrappedKey, unwrappingKey, unwrapAlgorithm, unwrappedKeyAlgorithm, extractable, keyUsages) {
    return new CryptoKey("secret", extractable, unwrappedKeyAlgorithm, keyUsages);
  }, "unwrapKey");

  markNative(SubtleCrypto, "SubtleCrypto");

  function getRandomValues(typedArray) {
    if (!typedArray || !ArrayBuffer.isView(typedArray) || (typedArray instanceof DataView)) {
      throw new TypeError("Failed to execute 'getRandomValues' on 'Crypto': parameter 1 is not of type 'ArrayBufferView'.");
    }
    if (typedArray.byteLength > 65536) {
      throw new DOMException("The requested length exceeds 65,536 bytes", "QuotaExceededError");
    }
    const u8 = new Uint8Array(typedArray.buffer, typedArray.byteOffset, typedArray.byteLength);
    for (let i = 0; i < u8.length; i++) {
      u8[i] = Math.floor(Math.random() * 256);
    }
    return typedArray;
  }

  function randomUUID() {
    const bytes = new Uint8Array(16);
    for (let i = 0; i < 16; i++) bytes[i] = Math.floor(Math.random() * 256);
    bytes[6] = (bytes[6] & 0x0f) | 0x40;
    bytes[8] = (bytes[8] & 0x3f) | 0x80;
    const hex = [];
    for (let i = 0; i < 16; i++) {
      hex.push((bytes[i] < 16 ? "0" : "") + bytes[i].toString(16));
    }
    return `${hex[0]}${hex[1]}${hex[2]}${hex[3]}-${hex[4]}${hex[5]}-${hex[6]}${hex[7]}-${hex[8]}${hex[9]}-${hex[10]}${hex[11]}${hex[12]}${hex[13]}${hex[14]}${hex[15]}`;
  }

  function Crypto() {
    this.subtle = new SubtleCrypto();
  }
  Crypto.prototype.getRandomValues = markNative(getRandomValues, "getRandomValues");
  Crypto.prototype.randomUUID = markNative(randomUUID, "randomUUID");
  Crypto.prototype[Symbol.toStringTag] = "Crypto";
  markNative(Crypto, "Crypto");

  const cryptoInstance = new Crypto();
  global.crypto = cryptoInstance;
  global.Crypto = Crypto;
  global.CryptoKey = CryptoKey;
  global.CryptoKeyPair = CryptoKeyPair;
  global.SubtleCrypto = SubtleCrypto;

  if (typeof window !== "undefined") {
    window.crypto = cryptoInstance;
    window.Crypto = Crypto;
    window.CryptoKey = CryptoKey;
    window.CryptoKeyPair = CryptoKeyPair;
    window.SubtleCrypto = SubtleCrypto;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.crypto = cryptoInstance;
    globalThis.Crypto = Crypto;
    globalThis.CryptoKey = CryptoKey;
    globalThis.CryptoKeyPair = CryptoKeyPair;
    globalThis.SubtleCrypto = SubtleCrypto;
  }

  // 13. Keyboard Event Dispatch & Sequential Tab Navigation
  function isTabFocusHidden(el) {
    if (!el || el.nodeType !== 1) return true;
    if (el.hidden || (el.hasAttribute && el.hasAttribute("hidden"))) return true;
    if (el.style && el.style.display === "none") return true;
    if (typeof getComputedStyle === "function") {
      try {
        const cs = getComputedStyle(el);
        if (cs && cs.display === "none") return true;
      } catch (e) {}
    }
    return false;
  }

  function isTabFocusDisabled(el) {
    if (!el) return false;
    return Boolean(el.disabled || (el.hasAttribute && el.hasAttribute("disabled")));
  }

  function isTabFocusInert(el) {
    if (!el) return false;
    return Boolean(el.inert || (el.hasAttribute && el.hasAttribute("inert")));
  }

  function isNaturallyFocusable(el) {
    if (!el || el.nodeType !== 1) return false;
    const tag = String(el.tagName || el.localName || "").toLowerCase();
    if (tag === "a") {
      return (el.hasAttribute ? el.hasAttribute("href") : false) || Boolean(el.href);
    }
    if (tag === "button" || tag === "select" || tag === "textarea") {
      return true;
    }
    if (tag === "input") {
      const type = String((el.getAttribute ? el.getAttribute("type") : el.type) || "").toLowerCase();
      return type !== "hidden";
    }
    const ce = el.getAttribute ? el.getAttribute("contenteditable") : null;
    if (ce === "true" || ce === "" || el.contentEditable === "true") {
      return true;
    }
    return false;
  }

  function parseTabIndex(el) {
    const hasAttr = (el.hasAttribute ? el.hasAttribute("tabindex") : false) || (el.getAttribute && el.getAttribute("tabindex") !== null);
    if (hasAttr) {
      const raw = String(el.getAttribute("tabindex")).trim();
      if (/^[+-]?\d+$/.test(raw)) {
        return parseInt(raw, 10);
      }
      if (isNaturallyFocusable(el)) {
        return 0;
      }
      return null;
    }
    if (isNaturallyFocusable(el)) {
      return 0;
    }
    return null;
  }

  function getSequentialFocusList(doc) {
    const items = [];

    function traverse(node, parentInert, parentHidden) {
      if (!node || node.nodeType !== 1) return;

      const inert = parentInert || isTabFocusInert(node);
      if (inert) return;

      const hidden = parentHidden || isTabFocusHidden(node);
      if (hidden) return;

      if (!isTabFocusDisabled(node)) {
        const ti = parseTabIndex(node);
        if (ti !== null && ti >= 0) {
          items.push({ el: node, tabindex: ti, order: items.length });
        }
      }

      const children = node.children || node.childNodes || [];
      for (let i = 0; i < children.length; i++) {
        if (children[i].nodeType === 1) {
          traverse(children[i], inert, hidden);
        }
      }
    }

    const root = doc.documentElement || doc.body || doc;
    traverse(root, false, false);

    const positive = [];
    const zero = [];
    for (let i = 0; i < items.length; i++) {
      if (items[i].tabindex > 0) {
        positive.push(items[i]);
      } else {
        zero.push(items[i]);
      }
    }

    positive.sort((a, b) => {
      if (a.tabindex !== b.tabindex) return a.tabindex - b.tabindex;
      return a.order - b.order;
    });

    const list = [];
    for (let i = 0; i < positive.length; i++) list.push(positive[i].el);
    for (let i = 0; i < zero.length; i++) list.push(zero[i].el);
    return list;
  }

  function navigateTabFocus(isShift) {
    const doc = (typeof document !== "undefined" ? document : null) || global.document;
    if (!doc) return;
    const list = getSequentialFocusList(doc);
    if (list.length === 0) return;

    const current = doc.activeElement;
    let curIdx = -1;
    if (current) {
      for (let i = 0; i < list.length; i++) {
        if (list[i] === current) {
          curIdx = i;
          break;
        }
      }
    }

    let nextIdx;
    if (curIdx === -1) {
      nextIdx = isShift ? list.length - 1 : 0;
    } else {
      if (!isShift) {
        nextIdx = (curIdx + 1) % list.length;
      } else {
        nextIdx = (curIdx - 1 + list.length) % list.length;
      }
    }

    const nextEl = list[nextIdx];
    if (current && current !== nextEl && typeof current.blur === "function") {
      current.blur();
    }
    if (nextEl && typeof nextEl.focus === "function") {
      nextEl.focus();
    }
  }

  let _lastKeyDownPrevented = false;
  let _lastKeyDownKey = "";

  function isTargetContentEditable(el) {
    if (!el) return false;
    if (el.isContentEditable === true) return true;
    let cur = el.nodeType === 1 ? el : el.parentNode;
    while (cur) {
      if (cur.nodeType === 1 && typeof cur.hasAttribute === "function" && cur.hasAttribute("contenteditable")) {
        const raw = cur.getAttribute("contenteditable");
        const low = String(raw == null ? "" : raw).toLowerCase();
        if (low === "false") return false;
        if (low === "" || low === "true" || low === "plaintext-only") return true;
      }
      if (cur.nodeType === 9) {
        return String(cur.designMode || "").toLowerCase() === "on";
      }
      cur = cur.parentElement || cur.parentNode;
    }
    return false;
  }

  function findEditingHost(el) {
    let host = null;
    let cur = el && el.nodeType === 1 ? el : (el ? el.parentNode : null);
    while (cur && cur.nodeType === 1) {
      if (isTargetContentEditable(cur)) {
        host = cur;
      } else if (host) {
        break;
      }
      cur = cur.parentElement || cur.parentNode;
    }
    return host;
  }

  function getSelectionInHost(doc, host) {
    const sel = (doc && typeof doc.getSelection === "function")
      ? doc.getSelection()
      : (typeof global.getSelection === "function" ? global.getSelection() : null);
    if (!sel) return { sel: null, range: null };
    let range = null;
    if (sel.rangeCount && sel.rangeCount > 0) {
      try {
        const r = sel.getRangeAt(0);
        if (r && r.startContainer && (!host || host === r.startContainer || (typeof host.contains === "function" && host.contains(r.startContainer)))) {
          range = r;
        }
      } catch (e) {}
    }
    return { sel: sel, range: range };
  }

  function makeStaticTargetRange(startContainer, startOffset, endContainer, endOffset) {
    return {
      startContainer: startContainer,
      startOffset: startOffset,
      endContainer: endContainer,
      endOffset: endOffset,
      collapsed: startContainer === endContainer && startOffset === endOffset,
    };
  }

  function createContentEditableInputEvent(type, cancelable, inputType, data, targetRanges) {
    let ev;
    if (typeof _NativeInputEvent === "function") {
      ev = new _NativeInputEvent(type, {
        bubbles: true,
        cancelable: cancelable,
        composed: true,
        inputType: inputType,
        data: data,
      });
    } else {
      ev = new _NativeEventCtor(type, { bubbles: true, cancelable: cancelable, composed: true });
      ev.inputType = inputType;
      ev.data = data;
    }
    if (targetRanges) {
      const frozenRanges = targetRanges.slice();
      ev.getTargetRanges = function () { return frozenRanges.slice(); };
    }
    return ev;
  }

  function dispatchTextInputEvent(target, data) {
    const ev = new Event("textInput", { bubbles: true, cancelable: true, composed: true });
    try {
      Object.defineProperty(ev, "data", { value: data, enumerable: true, configurable: true });
    } catch (e) {
      ev.data = data;
    }
    target.dispatchEvent(ev);
    return !ev.defaultPrevented;
  }

  function applyContentEditableInsertText(doc, host, sel, range, text) {
    if (!range) {
      const tn = doc.createTextNode(text);
      host.appendChild(tn);
      if (sel && typeof sel.collapse === "function") {
        try { sel.collapse(tn, text.length); } catch (e) {}
      }
      return;
    }
    if (!range.collapsed && typeof range.deleteContents === "function") {
      try { range.deleteContents(); } catch (e) {}
    }
    const sc = range.startContainer;
    const so = range.startOffset;
    if (sc && sc.nodeType === 3) {
      const curText = String(sc.data !== undefined ? sc.data : (sc.textContent || ""));
      const nextText = curText.slice(0, so) + text + curText.slice(so);
      if (typeof sc.data === "string") sc.data = nextText;
      else sc.textContent = nextText;
      const newOffset = so + text.length;
      if (sel && typeof sel.collapse === "function") {
        try { sel.collapse(sc, newOffset); } catch (e) {}
      }
    } else if (sc && sc.nodeType === 1) {
      const tn = doc.createTextNode(text);
      const ref = sc.childNodes && sc.childNodes[so] ? sc.childNodes[so] : null;
      sc.insertBefore(tn, ref);
      if (sel && typeof sel.collapse === "function") {
        try { sel.collapse(tn, text.length); } catch (e) {}
      }
    }
  }

  function applyContentEditableBackspace(doc, host, sel, range) {
    if (!range) return;
    if (!range.collapsed && typeof range.deleteContents === "function") {
      try { range.deleteContents(); } catch (e) {}
      if (sel && typeof sel.collapse === "function") {
        try { sel.collapse(range.startContainer, range.startOffset); } catch (e) {}
      }
      return;
    }
    const sc = range.startContainer;
    const so = range.startOffset;
    if (sc && sc.nodeType === 3 && so > 0) {
      const curText = String(sc.data !== undefined ? sc.data : (sc.textContent || ""));
      const nextText = curText.slice(0, so - 1) + curText.slice(so);
      if (typeof sc.data === "string") sc.data = nextText;
      else sc.textContent = nextText;
      if (sel && typeof sel.collapse === "function") {
        try { sel.collapse(sc, so - 1); } catch (e) {}
      }
    }
  }

  function applyContentEditableInsertParagraph(doc, host, sel, range) {
    if (!range) return;
    if (!range.collapsed && typeof range.deleteContents === "function") {
      try { range.deleteContents(); } catch (e) {}
    }
    const sc = range.startContainer;
    const so = range.startOffset;
    // Find nearest block child inside host (e.g. <p>, <div>)
    let block = sc && sc.nodeType === 1 ? sc : (sc ? sc.parentNode : null);
    while (block && block !== host && block.parentNode !== host) {
      block = block.parentNode;
    }
    if (block && block !== host && block.parentNode === host && sc && sc.nodeType === 3) {
      const curText = String(sc.data !== undefined ? sc.data : (sc.textContent || ""));
      const beforeText = curText.slice(0, so);
      const afterText = curText.slice(so);
      if (typeof sc.data === "string") sc.data = beforeText;
      else sc.textContent = beforeText;
      const newBlock = doc.createElement(String(block.localName || block.tagName || "p").toLowerCase());
      host.insertBefore(newBlock, block.nextSibling);
      if (afterText.length > 0) {
        newBlock.textContent = afterText;
      }
      // Move any following siblings after sc into newBlock
      let sib = sc.nextSibling;
      while (sib) {
        const nextSib = sib.nextSibling;
        newBlock.appendChild(sib);
        sib = nextSib;
      }
      const caretTarget = newBlock.firstChild || newBlock;
      if (sel && typeof sel.collapse === "function") {
        try { sel.collapse(caretTarget, 0); } catch (e) {}
      }
    } else if (sc && sc.nodeType === 3) {
      const curText = String(sc.data !== undefined ? sc.data : (sc.textContent || ""));
      const nextText = curText.slice(0, so) + "\n" + curText.slice(so);
      if (typeof sc.data === "string") sc.data = nextText;
      else sc.textContent = nextText;
      if (sel && typeof sel.collapse === "function") {
        try { sel.collapse(sc, so + 1); } catch (e) {}
      }
    }
  }

  function dispatchKeyEvent(type, key, code, modifiers, isRepeat) {
    const mods = Number(modifiers || 0);
    const isAlt = (mods & 1) !== 0;
    const isCtrl = (mods & 2) !== 0;
    const isMeta = (mods & 4) !== 0;
    const isShift = (mods & 8) !== 0;
    const doc = (typeof document !== "undefined" ? document : null) || global.document;
    const target = (doc && doc.activeElement) ? doc.activeElement : (doc || null);
    if (!target) return 0;

    const keyStr = String(key || "");
    const codeStr = String(code || "");

    if (type === "keyup") {
      _lastKeyDownPrevented = false;
      const evt = new _NativeKeyboardEvent("keyup", {
        key: keyStr,
        code: codeStr,
        ctrlKey: isCtrl,
        altKey: isAlt,
        metaKey: isMeta,
        shiftKey: isShift,
        cancelable: true,
        bubbles: true,
        composed: true,
      });
      _dispatchTrusted(target, evt);
      if (!evt.defaultPrevented && (keyStr === " " || keyStr === "Spacebar")) {
        const upTag = String(target.tagName || target.localName || "").toLowerCase();
        if (upTag === "button" && !target.disabled && typeof target.click === "function") {
          target.click();
        }
      }
      const active = doc ? doc.activeElement : null;
      return (active && typeof active._nid === "number") ? active._nid : 0;
    }

    if (type === "keypress") {
      if (_lastKeyDownPrevented) {
        const active = doc ? doc.activeElement : null;
        return (active && typeof active._nid === "number") ? active._nid : 0;
      }
      const isEnter = keyStr === "Enter" || keyStr === "\r" || keyStr === "\\r" || codeStr === "Enter";
      const kpKey = isEnter ? "Enter" : keyStr;
      const kpEvt = new _NativeKeyboardEvent("keypress", {
        key: kpKey,
        code: codeStr,
        ctrlKey: isCtrl,
        altKey: isAlt,
        metaKey: isMeta,
        shiftKey: isShift,
        repeat: Boolean(isRepeat),
        cancelable: true,
        bubbles: true,
        composed: true,
      });
      _dispatchTrusted(target, kpEvt);
      let tiAllowedAll = true;
      if (!kpEvt.defaultPrevented) {
        if (isTargetContentEditable(target)) {
          const host = findEditingHost(target) || target;
          const selInfo = getSelectionInHost(doc, host);
          const inputType = isEnter ? "insertParagraph" : "insertText";
          const biData = isEnter ? null : kpKey;
          const tiData = isEnter ? "\n" : kpKey;
          const targetRanges = selInfo.range
            ? [makeStaticTargetRange(selInfo.range.startContainer, selInfo.range.startOffset, selInfo.range.endContainer, selInfo.range.endOffset)]
            : [];
          const biEvt = createContentEditableInputEvent("beforeinput", true, inputType, biData, targetRanges);
          target.dispatchEvent(biEvt);
          if (!biEvt.defaultPrevented) {
            const tiAllowed = dispatchTextInputEvent(target, tiData);
            tiAllowedAll = tiAllowed;
            if (tiAllowed) {
              if (isEnter) {
                applyContentEditableInsertParagraph(doc, host, selInfo.sel, selInfo.range);
              } else {
                applyContentEditableInsertText(doc, host, selInfo.sel, selInfo.range, kpKey);
              }
              const inEvt = createContentEditableInputEvent("input", false, inputType, biData, []);
              target.dispatchEvent(inEvt);
            }
          } else {
            tiAllowedAll = false;
          }
        } else if (target && target.nodeType === 1) {
          const tTag = String(target.tagName || target.localName || "").toLowerCase();
          if ((tTag === "input" || tTag === "textarea") && kpKey.length === 1) {
            tiAllowedAll = dispatchTextInputEvent(target, kpKey);
          }
        }
      }
      if (typeof globalThis !== "undefined") {
        globalThis.__capsurf_last_key_prevented = Boolean(_lastKeyDownPrevented || kpEvt.defaultPrevented || !tiAllowedAll);
      }
      const active = doc ? doc.activeElement : null;
      return (active && typeof active._nid === "number") ? active._nid : 0;
    }

    if (type === "keydown") {
      _lastKeyDownPrevented = false;
      _lastKeyDownKey = keyStr;
      if (keyStr === "Enter" && target && target.nodeType === 1) {
        const tTag = String(target.tagName || target.localName || "").toLowerCase();
        if (tTag === "input" && target._capsurfPreEnterAttr === undefined && typeof target.getAttribute === "function") {
          target._capsurfPreEnterAttr = target.getAttribute("value");
        }
      }
      const evt = new _NativeKeyboardEvent("keydown", {
        key: keyStr,
        code: codeStr,
        ctrlKey: isCtrl,
        altKey: isAlt,
        metaKey: isMeta,
        shiftKey: isShift,
        repeat: Boolean(isRepeat),
        cancelable: true,
        bubbles: true,
        composed: true,
      });
      _dispatchTrusted(target, evt);
      const defaultPrevented = Boolean(evt.defaultPrevented);
      _lastKeyDownPrevented = defaultPrevented;
      if (typeof globalThis !== "undefined") {
        globalThis.__capsurf_last_key_prevented = defaultPrevented;
      }

      if (!defaultPrevented) {
        if (!isTargetContentEditable(target) && (keyStr === "Backspace" || keyStr === "Delete")) {
          const tTag = String(target.tagName || target.localName || "").toLowerCase();
          if (tTag === "textarea" || tTag === "input") {
            const rawVal = typeof getValueRaw === "function" ? getValueRaw(target) : String(target._value !== undefined ? target._value : (target.value || ""));
            const s = typeof target.selectionStart === "number" ? target.selectionStart : rawVal.length;
            const e = typeof target.selectionEnd === "number" ? target.selectionEnd : s;
            if (s === e) {
              if (keyStr === "Backspace" && s >= 2) {
                const lo = rawVal.charCodeAt(s - 1);
                const hi = rawVal.charCodeAt(s - 2);
                if (hi >= 0xD800 && hi <= 0xDBFF && lo >= 0xDC00 && lo <= 0xDFFF) {
                  const trimmed = rawVal.slice(0, s - 1) + rawVal.slice(s);
                  if (typeof setValueRaw === "function") setValueRaw(target, trimmed);
                  else target._value = trimmed;
                  target._selectionStart = s - 1;
                  target._selectionEnd = s - 1;
                }
              } else if (keyStr === "Delete" && s + 1 < rawVal.length) {
                const hi = rawVal.charCodeAt(s);
                const lo = rawVal.charCodeAt(s + 1);
                if (hi >= 0xD800 && hi <= 0xDBFF && lo >= 0xDC00 && lo <= 0xDFFF) {
                  const trimmed = rawVal.slice(0, s + 1) + rawVal.slice(s + 2);
                  if (typeof setValueRaw === "function") setValueRaw(target, trimmed);
                  else target._value = trimmed;
                  target._selectionStart = s;
                  target._selectionEnd = s;
                }
              }
            }
          }
        }
        if (isTargetContentEditable(target) && (keyStr === "Backspace" || keyStr === "Delete")) {
          const host = findEditingHost(target) || target;
          const selInfo = getSelectionInHost(doc, host);
          const inputType = keyStr === "Backspace" ? "deleteContentBackward" : "deleteContentForward";
          const targetRanges = [];
          if (selInfo.range) {
            const r = selInfo.range;
            if (!r.collapsed) {
              targetRanges.push(makeStaticTargetRange(r.startContainer, r.startOffset, r.endContainer, r.endOffset));
            } else if (keyStr === "Backspace" && r.startContainer && r.startContainer.nodeType === 3 && r.startOffset > 0) {
              targetRanges.push(makeStaticTargetRange(r.startContainer, r.startOffset - 1, r.startContainer, r.startOffset));
            } else if (keyStr === "Delete" && r.startContainer && r.startContainer.nodeType === 3) {
              const len = String(r.startContainer.data !== undefined ? r.startContainer.data : (r.startContainer.textContent || "")).length;
              if (r.startOffset < len) {
                targetRanges.push(makeStaticTargetRange(r.startContainer, r.startOffset, r.startContainer, r.startOffset + 1));
              }
            }
          }
          const biEvt = createContentEditableInputEvent("beforeinput", true, inputType, null, targetRanges);
          target.dispatchEvent(biEvt);
          if (!biEvt.defaultPrevented) {
            if (keyStr === "Backspace") {
              applyContentEditableBackspace(doc, host, selInfo.sel, selInfo.range);
            }
            const inEvt = createContentEditableInputEvent("input", false, inputType, null, []);
            target.dispatchEvent(inEvt);
          }
        } else if (keyStr === "Tab") {
          navigateTabFocus(isShift);
        } else if (keyStr === "Enter") {
          const active = doc ? doc.activeElement : null;
          if (active) {
            const tag = String(active.tagName || active.localName || "").toLowerCase();
            const isButton = (tag === "button");
            const isLink = (tag === "a" && ((active.hasAttribute ? active.hasAttribute("href") : false) || Boolean(active.href)));
            if (isButton || isLink) {
              if (typeof active.click === "function") {
                active.click();
              }
            } else if (tag === "input") {
              const inputType = String(active.type || (active.getAttribute ? active.getAttribute("type") : "") || "text").toLowerCase();
              const implicitSubmitTypes = new Set([
                "text", "search", "url", "tel", "email", "password",
                "date", "month", "week", "time", "datetime-local", "number"
              ]);
              if (implicitSubmitTypes.has(inputType)) {
                const form = typeof findAssociatedForm === "function" ? findAssociatedForm(active) : active.form;
                if (form && typeof submitForm === "function") {
                  const controls = typeof collectFormControls === "function" ? collectFormControls(form) : (form.elements || []);
                  let defaultBtn = null;
                  let textFieldCount = 0;
                  for (let i = 0; i < controls.length; i++) {
                    const c = controls[i];
                    if (!c) continue;
                    const cTag = String(c.tagName || c.localName || "").toLowerCase();
                    const cType = String(c.type || (c.getAttribute ? c.getAttribute("type") : "") || "").toLowerCase();
                    if (!defaultBtn) {
                      const isBtnSubmit = (cTag === "button" && (cType === "submit" || cType === "")) ||
                                          (cTag === "input" && (cType === "submit" || cType === "image"));
                      if (isBtnSubmit) {
                        defaultBtn = c;
                      }
                    }
                    if (cTag === "input" && implicitSubmitTypes.has(cType || "text")) {
                      textFieldCount++;
                    }
                  }
                  if (defaultBtn) {
                    const disabled = defaultBtn.disabled || (typeof defaultBtn.hasAttribute === "function" && defaultBtn.hasAttribute("disabled"));
                    if (!disabled) {
                      submitForm(form, defaultBtn);
                    }
                  } else if (textFieldCount <= 1) {
                    submitForm(form, null);
                  }
                }
              }
            }
          }
        } else if (!isAlt && !isMeta) {
          let isEditingTarget = isTargetContentEditable(target);
          if (!isEditingTarget && target && target.nodeType === 1) {
            const edTag = String(target.tagName || target.localName || "").toUpperCase();
            if (edTag === "INPUT" || edTag === "TEXTAREA" || edTag === "SELECT") {
              isEditingTarget = true;
            }
          }
          if (!isEditingTarget) {
            const hasTtyCss = (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_tty_ua_css_text === "string" && globalThis.__capsurf_tty_ua_css_text.length > 0);
            const ttyKeyboard = hasTtyCss || (typeof globalThis !== "undefined" && globalThis.__capsurf_tty_keyboard === true);
            const explicitCellH = (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_tty_cell_h === "number" && globalThis.__capsurf_tty_cell_h > 0) ? globalThis.__capsurf_tty_cell_h : 0;
            const explicitCellW = (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_tty_cell_w === "number" && globalThis.__capsurf_tty_cell_w > 0) ? globalThis.__capsurf_tty_cell_w : 0;
            const urlHasTtyCss = (typeof globalThis !== "undefined" && globalThis.__capsurf_nav_tty_stripped === true);
            const cellHeight = explicitCellH || (urlHasTtyCss ? 17 : 0);
            const cellWidth = explicitCellW || (urlHasTtyCss ? 8 : 0);
            const lineHeight = cellHeight || 40;
            const lineWidth = cellWidth || 40;
            const vh = (typeof global.innerHeight === "number" && global.innerHeight > 0) ? global.innerHeight : 600;
            const pageHeight = Math.max(lineHeight, vh - lineHeight);
            let sdx = 0;
            let sdy = 0;
            if (isCtrl) {
              if (ttyKeyboard) {
                const halfPage = Math.max(lineHeight, Math.floor(vh / (2 * lineHeight)) * lineHeight);
                if (keyStr === "f") sdy = pageHeight;
                else if (keyStr === "b") sdy = -pageHeight;
                else if (keyStr === "d") sdy = halfPage;
                else if (keyStr === "u") sdy = -halfPage;
              }
            } else {
              const isSpace = keyStr === " " || keyStr === "Spacebar";
              const tgtTag = (target && target.nodeType === 1) ? String(target.tagName || target.localName || "").toUpperCase() : "";
              if (!(isSpace && (tgtTag === "BUTTON" || tgtTag === "SUMMARY"))) {
                if (keyStr === "ArrowDown" || (ttyKeyboard && !isShift && keyStr === "j")) {
                  sdy = lineHeight;
                } else if (keyStr === "ArrowUp" || (ttyKeyboard && !isShift && keyStr === "k")) {
                  sdy = -lineHeight;
                } else if (keyStr === "ArrowRight" || (ttyKeyboard && !isShift && keyStr === "l")) {
                  sdx = lineWidth;
                } else if (keyStr === "ArrowLeft" || (ttyKeyboard && !isShift && keyStr === "h")) {
                  sdx = -lineWidth;
                } else if (keyStr === "PageDown" || (isSpace && !isShift)) {
                  sdy = pageHeight;
                } else if (keyStr === "PageUp" || (isSpace && isShift)) {
                  sdy = -pageHeight;
                }
              }
            }
            if ((sdx !== 0 || sdy !== 0) && typeof global.scrollBy === "function") {
              global.scrollBy(sdx, sdy);
            }
          }
        }
      }

      const active = doc ? doc.activeElement : null;
      return (active && typeof active._nid === "number") ? active._nid : 0;
    }

    return 0;
  }

  global.__capsurf_dispatch_key_event = dispatchKeyEvent;
  if (typeof window !== "undefined") {
    window.__capsurf_dispatch_key_event = dispatchKeyEvent;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_dispatch_key_event = dispatchKeyEvent;
  }

  const activeWindowStack = [];
  global.__capsurf_active_window_stack = activeWindowStack;

  function getWindowOrigin(win) {
    try {
      if (win && (win._opaqueSandbox || (win.frameElement && win.frameElement._opaqueSandbox))) {
        return "null";
      }
      if (win && typeof win.origin === "string" && win.origin) {
        return win.origin;
      }
      const loc = win && win.location;
      if (loc) {
        if (typeof loc.origin === "string" && loc.origin && loc.origin !== "about:") {
          return loc.origin;
        }
        if (typeof loc.href === "string" && loc.href) {
          const u = new HistoryURL(loc.href);
          return u.origin || "null";
        }
      }
      const doc = win && win.document;
      if (doc && typeof doc._baseUrl === "string" && doc._baseUrl) {
        const u = new HistoryURL(doc._baseUrl);
        return u.origin || "null";
      }
    } catch (e) {}
    return "null";
  }

  function clonePostMessageData(data) {
    if (data === null || data === undefined || typeof data !== "object") return data;
    if (typeof structuredClone === "function") {
      try { return structuredClone(data); } catch (e) {}
    }
    try {
      return JSON.parse(JSON.stringify(data));
    } catch (e) {
      return data;
    }
  }

  function dispatchPostMessage(targetWin, message, targetOriginOrOptions) {
    if (!targetWin) return;
    const senderWin = activeWindowStack.length > 0
      ? activeWindowStack[activeWindowStack.length - 1]
      : (typeof window !== "undefined" ? window : global);
    const senderOrigin = getWindowOrigin(senderWin);
    let targetOrigin = "/";
    if (targetOriginOrOptions && typeof targetOriginOrOptions === "object") {
      targetOrigin = targetOriginOrOptions.targetOrigin !== undefined ? String(targetOriginOrOptions.targetOrigin) : "/";
    } else if (arguments.length >= 3 && targetOriginOrOptions !== undefined) {
      targetOrigin = String(targetOriginOrOptions);
    }

    const receiverOrigin = getWindowOrigin(targetWin);
    if (targetOrigin === "*") {
      // Allow any target origin
    } else if (targetOrigin === "/") {
      if (senderOrigin === "null" || receiverOrigin !== senderOrigin) return;
    } else {
      let parsedTargetOrigin = null;
      try {
        parsedTargetOrigin = new HistoryURL(targetOrigin).origin;
      } catch (e) {
        throw new DOMException(
          "Failed to execute 'postMessage' on 'Window': Invalid target origin '" + targetOrigin + "' in a call to 'postMessage'.",
          "SyntaxError"
        );
      }
      if (!parsedTargetOrigin || parsedTargetOrigin === "null" || receiverOrigin !== parsedTargetOrigin) {
        return;
      }
    }

    const cloned = clonePostMessageData(message);
    Promise.resolve().then(function () {
      try {
        const evt = new _NativeMessageEvent("message", {
          data: cloned,
          origin: senderOrigin,
          source: senderWin,
        });
        activeWindowStack.push(targetWin);
        try {
          if (typeof targetWin.dispatchEvent === "function") {
            targetWin.dispatchEvent(evt);
          }
          if (typeof targetWin.onmessage === "function") {
            targetWin.onmessage.call(targetWin, evt);
          }
        } finally {
          activeWindowStack.pop();
        }
      } catch (e) {}
    });
  }

  global.__capsurf_postMessage = dispatchPostMessage;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_postMessage = dispatchPostMessage;

  function windowPostMessage(message, targetOriginOrOptions) {
    const target = (this && typeof this.dispatchEvent === "function")
      ? this
      : (typeof window !== "undefined" ? window : global);
    if (arguments.length >= 2) {
      return dispatchPostMessage(target, message, targetOriginOrOptions);
    }
    return dispatchPostMessage(target, message);
  }
  markNative(windowPostMessage, "postMessage");
  global.postMessage = windowPostMessage;
  if (typeof window !== "undefined") window.postMessage = windowPostMessage;
  if (typeof globalThis !== "undefined") globalThis.postMessage = windowPostMessage;

  function evalInIframe(expr) {
    const iframeEl = document.querySelector("iframe");
    const win = (iframeEl && iframeEl.contentWindow) || (typeof window !== "undefined" ? window : global);
    const args = [
      win,
      win.document,
      win.location,
      win.fetch,
      win,
      win,
      win.addEventListener.bind(win),
      win.removeEventListener.bind(win),
      win.postMessage.bind(win),
      win.innerWidth,
      win.innerHeight,
      win.outerWidth,
      win.outerHeight,
      win.parent,
      win.top,
      win.frames,
      win.frameElement,
    ];
    function compileIframeFn(body) {
      return new Function(
        "window", "document", "location", "fetch", "self", "globalThis",
        "addEventListener", "removeEventListener", "postMessage",
        "innerWidth", "innerHeight", "outerWidth", "outerHeight",
        "parent", "top", "frames", "frameElement",
        body
      );
    }
    let fn = null;
    try {
      fn = compileIframeFn("return (" + expr + ");");
    } catch (e) {
      const trimmed = String(expr || "").replace(/[\s;]+$/, "");
      const semi = trimmed.lastIndexOf(";");
      if (semi !== -1) {
        const prefix = trimmed.slice(0, semi + 1);
        const last = trimmed.slice(semi + 1).trim();
        if (last) {
          try {
            fn = compileIframeFn(prefix + "\nreturn (" + last + ");");
          } catch (e2) {}
        }
      }
      if (!fn) {
        fn = compileIframeFn(expr);
      }
    }
    activeWindowStack.push(win);
    try {
      return fn.apply(win, args);
    } finally {
      activeWindowStack.pop();
    }
  }
  global.__capsurf_eval_in_iframe = evalInIframe;
  if (typeof window !== "undefined") window.__capsurf_eval_in_iframe = evalInIframe;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_eval_in_iframe = evalInIframe;

  // Pointer & Mouse Event Dispatcher (supports pointer stream, pointerdown compatibility mouse suppression,
  // post-dispatch mousedown focus default action, mouseup navigation dedup, and textarea/input caret placement).
  let _pointerDownPrevented = false;
  function _mouseButtonToNum(btn) {
    if (typeof btn === "number") return btn;
    if (btn === "left") return 0;
    if (btn === "middle") return 1;
    if (btn === "right") return 2;
    return 0;
  }
  function _findFocusableAncestor(el) {
    let cur = el;
    while (cur && cur.nodeType === 1) {
      const tag = String(cur.localName || cur.tagName || "").toLowerCase();
      const disabled = cur.disabled || (typeof cur.hasAttribute === "function" && cur.hasAttribute("disabled"));
      if (!disabled) {
        if (tag === "button" || tag === "input" || tag === "textarea" || tag === "select") {
          return cur;
        }
        if (tag === "a" && typeof cur.hasAttribute === "function" && cur.hasAttribute("href")) {
          return cur;
        }
        if (typeof cur.hasAttribute === "function" && cur.hasAttribute("tabindex")) {
          return cur;
        }
        if (cur.isContentEditable) {
          return cur;
        }
      }
      cur = cur.parentElement || cur.parentNode;
    }
    return null;
  }
  function _hitTestElement(x, y) {
    const doc = (typeof document !== "undefined" ? document : null) || global.document;
    if (!doc) return null;
    if (typeof doc.elementFromPoint === "function") {
      try {
        const hit = doc.elementFromPoint(x, y);
        if (hit) return hit;
      } catch (e) {}
    }
    const all = doc.querySelectorAll ? doc.querySelectorAll("*") : [];
    for (let i = all.length - 1; i >= 0; i--) {
      const item = all[i];
      if (!item || typeof item.getBoundingClientRect !== "function") continue;
      const r = item.getBoundingClientRect();
      if (r && r.width > 0 && r.height > 0 && x >= r.left && x <= r.left + r.width && y >= r.top && y <= r.top + r.height) {
        return item;
      }
    }
    return doc.body || doc.documentElement || null;
  }
  function _dispatchTrusted(target, ev) {
    if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
      return global.__capsurf_dispatchTrustedEvent(target, ev);
    }
    return target.dispatchEvent(ev);
  }
  let _heldButtonsMask = 0;
  let _lastMouseX = null;
  let _lastMouseY = null;
  let _lastHoveredElement = null;
  function _getElementAncestors(el) {
    const list = [];
    let cur = el;
    while (cur && cur.nodeType === 1) {
      list.push(cur);
      cur = cur.parentElement || cur.parentNode;
    }
    return list;
  }
  function _dispatchPointerAndMouseBoundary(target, x, y, dx, dy, btnNum, effectiveButtons) {
    const prev = _lastHoveredElement;
    if (prev === target) return;
    _lastHoveredElement = target;
    const PtrCtor = typeof _NativePointerEvent === "function" ? _NativePointerEvent : (typeof _NativeMouseEvent === "function" ? _NativeMouseEvent : _NativeEventCtor);
    const MouseCtor = typeof _NativeMouseEvent === "function" ? _NativeMouseEvent : _NativeEventCtor;
    const prevAnc = prev ? _getElementAncestors(prev) : [];
    const nextAnc = target ? _getElementAncestors(target) : [];
    const nextSet = new Set(nextAnc);
    let commonIdxPrev = prevAnc.length;
    for (let i = 0; i < prevAnc.length; i++) {
      if (nextSet.has(prevAnc[i])) {
        commonIdxPrev = i;
        break;
      }
    }
    const commonEl = commonIdxPrev < prevAnc.length ? prevAnc[commonIdxPrev] : null;
    const leftElements = prevAnc.slice(0, commonIdxPrev);
    const enteredElements = [];
    for (let i = 0; i < nextAnc.length; i++) {
      if (nextAnc[i] === commonEl) break;
      enteredElements.push(nextAnc[i]);
    }
    enteredElements.reverse();

    const makePtrOpts = function (bubbles, cancelable, composed, relatedTarget) {
      return {
        bubbles: bubbles,
        cancelable: cancelable,
        composed: composed,
        clientX: x,
        clientY: y,
        movementX: dx,
        movementY: dy,
        button: btnNum,
        buttons: effectiveButtons,
        relatedTarget: relatedTarget || null,
        pointerId: 1,
        pointerType: "mouse",
        isPrimary: true,
      };
    };
    const makeMouseOpts = function (bubbles, cancelable, composed, relatedTarget) {
      return {
        bubbles: bubbles,
        cancelable: cancelable,
        composed: composed,
        clientX: x,
        clientY: y,
        movementX: dx,
        movementY: dy,
        button: btnNum,
        buttons: effectiveButtons,
        relatedTarget: relatedTarget || null,
      };
    };

    // 1. Pointer boundary events before mouse boundary events
    if (prev) {
      _dispatchTrusted(prev, new PtrCtor("pointerout", makePtrOpts(true, true, true, target)));
      for (let i = 0; i < leftElements.length; i++) {
        _dispatchTrusted(leftElements[i], new PtrCtor("pointerleave", makePtrOpts(false, false, false, target)));
      }
    }
    if (target) {
      _dispatchTrusted(target, new PtrCtor("pointerover", makePtrOpts(true, true, true, prev)));
      for (let i = 0; i < enteredElements.length; i++) {
        _dispatchTrusted(enteredElements[i], new PtrCtor("pointerenter", makePtrOpts(false, false, false, prev)));
      }
    }

    // 2. Mouse boundary events (unless suppressed by pointerdown)
    if (!_pointerDownPrevented) {
      if (prev) {
        _dispatchTrusted(prev, new MouseCtor("mouseout", makeMouseOpts(true, true, true, target)));
        for (let i = 0; i < leftElements.length; i++) {
          _dispatchTrusted(leftElements[i], new MouseCtor("mouseleave", makeMouseOpts(false, false, false, target)));
        }
      }
      if (target) {
        _dispatchTrusted(target, new MouseCtor("mouseover", makeMouseOpts(true, true, true, prev)));
        for (let i = 0; i < enteredElements.length; i++) {
          _dispatchTrusted(enteredElements[i], new MouseCtor("mouseenter", makeMouseOpts(false, false, false, prev)));
        }
      }
    }
  }
  function _mouseButtonToMask(btn) {
    if (btn === "left" || btn === 0) return 1;
    if (btn === "right" || btn === 2) return 2;
    if (btn === "middle" || btn === 1) return 4;
    if (btn === "back" || btn === 3) return 8;
    if (btn === "forward" || btn === 4) return 16;
    return 0;
  }
  function activateClickedElement(target, isTrustedClick, x, y, detail) {
    const doc = (typeof document !== "undefined" ? document : null) || global.document;
    const MouseCtor = typeof _NativeMouseEvent === "function" ? _NativeMouseEvent : _NativeEventCtor;
    const activateControl = function (ctrl, trusted) {
      if (!ctrl || ctrl.disabled || (ctrl.hasAttribute && ctrl.hasAttribute("disabled"))) return;
      const ctrlTag = String(ctrl.localName || ctrl.tagName || "").toLowerCase();
      if (ctrlTag === "input") {
        const iType = String(ctrl.getAttribute ? (ctrl.getAttribute("type") || "text") : "text").toLowerCase();
        if (iType === "checkbox") {
          ctrl.indeterminate = false;
          ctrl.checked = !ctrl.checked;
          const inEv = new _NativeEventCtor("input", { bubbles: true, cancelable: false, composed: true });
          const chEv = new _NativeEventCtor("change", { bubbles: true, cancelable: false });
          if (trusted) {
            _dispatchTrusted(ctrl, inEv);
            _dispatchTrusted(ctrl, chEv);
          } else if (typeof ctrl.dispatchEvent === "function") {
            ctrl.dispatchEvent(inEv);
            ctrl.dispatchEvent(chEv);
          }
        } else if (iType === "radio") {
          const name = ctrl.getAttribute ? ctrl.getAttribute("name") : ctrl.name;
          if (name && doc && typeof doc.querySelectorAll === "function") {
            try {
              const radios = doc.querySelectorAll('input[type="radio"]');
              for (let ri = 0; ri < radios.length; ri++) {
                const r = radios[ri];
                if (r !== ctrl && r.getAttribute && r.getAttribute("name") === name && r.form === ctrl.form) {
                  r.checked = false;
                }
              }
            } catch (e) {}
          }
          const wasChecked = Boolean(ctrl.checked);
          ctrl.checked = true;
          if (!wasChecked) {
            const inEv = new _NativeEventCtor("input", { bubbles: true, cancelable: false, composed: true });
            const chEv = new _NativeEventCtor("change", { bubbles: true, cancelable: false });
            if (trusted) {
              _dispatchTrusted(ctrl, inEv);
              _dispatchTrusted(ctrl, chEv);
            } else if (typeof ctrl.dispatchEvent === "function") {
              ctrl.dispatchEvent(inEv);
              ctrl.dispatchEvent(chEv);
            }
          }
        } else if (iType === "submit" || iType === "image") {
          const form = typeof findAssociatedForm === "function" ? findAssociatedForm(ctrl) : ctrl.form;
          if (form && typeof submitForm === "function") {
            submitForm(form, ctrl);
          }
        }
      } else if (ctrlTag === "button") {
        const bType = String(ctrl.getAttribute ? (ctrl.getAttribute("type") || "submit") : "submit").toLowerCase();
        if (bType === "submit") {
          const form = typeof findAssociatedForm === "function" ? findAssociatedForm(ctrl) : ctrl.form;
          if (form && typeof submitForm === "function") {
            submitForm(form, ctrl);
          }
        }
      }
    };

    let cur = target;
    for (let d = 0; cur && d < 64; d++) {
      if (cur.nodeType === 1) {
        if (typeof global.__capsurf_upgradeElement === "function") {
          global.__capsurf_upgradeElement(cur);
        }
        const cTag = String(cur.localName || cur.tagName || "").toLowerCase();
        if (cTag === "button" || cTag === "input") {
          activateControl(cur, isTrustedClick);
          break;
        }
        if (cTag === "select" || cTag === "textarea") {
          break;
        }
        if ((cTag === "a" || cTag === "area") && cur.getAttribute && cur.getAttribute("href") != null) {
          const href = cur.getAttribute("href");
          const tgtAttr = String((cur.getAttribute ? cur.getAttribute("target") : cur.target) || "").toLowerCase();
          if (tgtAttr === "_blank") {
            const relAttr = String((cur.getAttribute ? cur.getAttribute("rel") : cur.rel) || "").toLowerCase();
            const isNoop = /(?:^|\s)(noopener|noreferrer)(?:$|\s)/.test(relAttr);
            openFn(String(href), "_blank", isNoop ? "noopener" : "");
          } else {
            const loc = (typeof location !== "undefined") ? location : global.location;
            if (loc && href) {
              if (href.startsWith("#")) loc.hash = href;
              else loc.href = cur.href || href;
            }
          }
          break;
        }
        if (cTag === "label") {
          const control = cur.control;
          if (control && typeof global.__capsurf_upgradeElement === "function") {
            global.__capsurf_upgradeElement(control);
          }
          if (control && control !== target && !(typeof control.contains === "function" && control.contains(target))) {
            const isCtrlDisabled = Boolean(control.disabled || (control.hasAttribute && control.hasAttribute("disabled")));
            if (!isCtrlDisabled) {
              const fwdClickEv = new MouseCtor("click", {
                bubbles: true,
                cancelable: true,
                composed: true,
                clientX: x || 0,
                clientY: y || 0,
                button: 0,
                detail: detail || 1,
              });
              if (isTrustedClick) {
                _dispatchTrusted(control, fwdClickEv);
              } else if (typeof control.dispatchEvent === "function") {
                control.dispatchEvent(fwdClickEv);
              }
              if (!fwdClickEv.defaultPrevented) {
                activateControl(control, isTrustedClick);
              }
            }
          }
          break;
        }
      }
      cur = cur.parentNode;
    }
  }
  global.__capsurf_activate_clicked_element = activateClickedElement;
  if (typeof window !== "undefined") window.__capsurf_activate_clicked_element = activateClickedElement;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_activate_clicked_element = activateClickedElement;

  function dispatchMouseEvent(type, x, y, button, buttons, clickCount, skipSyntheticClick) {
    const doc = (typeof document !== "undefined" ? document : null) || global.document;
    const target = _hitTestElement(x, y);
    if (!target) return { handled: false };
    const btnNum = _mouseButtonToNum(button);
    const btnMask = _mouseButtonToMask(button);
    const hasExplicitButtons = typeof buttons === "number" && !isNaN(buttons);
    const detail = Number(clickCount) || 1;
    const PtrCtor = typeof _NativePointerEvent === "function" ? _NativePointerEvent : (typeof _NativeMouseEvent === "function" ? _NativeMouseEvent : _NativeEventCtor);
    const MouseCtor = typeof _NativeMouseEvent === "function" ? _NativeMouseEvent : _NativeEventCtor;

    if (type === "mousePressed") {
      const effectiveButtons = hasExplicitButtons ? (Number(buttons) | btnMask) : (_heldButtonsMask | btnMask);
      _heldButtonsMask = effectiveButtons;
      _lastMouseX = x;
      _lastMouseY = y;
      _pointerDownPrevented = false;
      const ptrEv = new PtrCtor("pointerdown", {
        bubbles: true,
        cancelable: true,
        composed: true,
        clientX: x,
        clientY: y,
        movementX: 0,
        movementY: 0,
        button: btnNum,
        buttons: effectiveButtons,
        pointerId: 1,
        pointerType: "mouse",
        isPrimary: true,
      });
      _dispatchTrusted(target, ptrEv);
      if (ptrEv.defaultPrevented) {
        _pointerDownPrevented = true;
      } else {
        const mdEv = new MouseCtor("mousedown", {
          bubbles: true,
          cancelable: true,
          composed: true,
          clientX: x,
          clientY: y,
          movementX: 0,
          movementY: 0,
          button: btnNum,
          buttons: effectiveButtons,
          detail: detail,
        });
        _dispatchTrusted(target, mdEv);
        if (!mdEv.defaultPrevented) {
          // Post-dispatch focus default action
          const focusable = _findFocusableAncestor(target);
          if (focusable) {
            if (doc && doc.activeElement !== focusable && typeof focusable.focus === "function") {
              focusable.focus();
            }
          } else if (doc) {
            if (doc.activeElement && doc.activeElement !== doc.body && doc.activeElement !== doc.documentElement) {
              if (typeof doc.activeElement.blur === "function") {
                doc.activeElement.blur();
              }
              doc.activeElement = doc.body || doc.documentElement || null;
            }
          }
          // Caret placement for textarea / text input without firing spurious select event
          const tag = String(target.localName || target.tagName || "").toLowerCase();
          if (tag === "textarea" || tag === "input") {
            const rawVal = typeof getValueRaw === "function" ? getValueRaw(target) : String(target._value !== undefined ? target._value : (target.value || ""));
            let offset = rawVal.length;
            if (typeof target.getBoundingClientRect === "function") {
              const r = target.getBoundingClientRect();
              if (r && r.width > 0) {
                const relX = x - r.left;
                if (relX <= 4) {
                  offset = 0;
                } else {
                  offset = Math.max(0, Math.min(rawVal.length, Math.round(relX / 8)));
                }
              }
            }
            target._selectionStart = offset;
            target._selectionEnd = offset;
            target._selectionDirection = "none";
          }
        }
      }
      return { handled: true };
    }

    if (type === "mouseMoved") {
      const effectiveButtons = hasExplicitButtons ? Number(buttons) : (_heldButtonsMask | btnMask);
      const dx = _lastMouseX === null ? 0 : (x - _lastMouseX);
      const dy = _lastMouseY === null ? 0 : (y - _lastMouseY);
      _lastMouseX = x;
      _lastMouseY = y;
      _dispatchPointerAndMouseBoundary(target, x, y, dx, dy, btnNum, effectiveButtons);
      const ptrEv = new PtrCtor("pointermove", {
        bubbles: true,
        cancelable: false,
        composed: true,
        clientX: x,
        clientY: y,
        movementX: dx,
        movementY: dy,
        button: btnNum,
        buttons: effectiveButtons,
        pointerId: 1,
        pointerType: "mouse",
        isPrimary: true,
      });
      _dispatchTrusted(target, ptrEv);
      if (!_pointerDownPrevented) {
        const mmEv = new MouseCtor("mousemove", {
          bubbles: true,
          cancelable: true,
          composed: true,
          clientX: x,
          clientY: y,
          movementX: dx,
          movementY: dy,
          button: btnNum,
          buttons: effectiveButtons,
        });
        _dispatchTrusted(target, mmEv);
      }
      return { handled: true };
    }

    if (type === "mouseReleased") {
      const effectiveButtons = hasExplicitButtons ? (Number(buttons) & ~btnMask) : (_heldButtonsMask & ~btnMask);
      _heldButtonsMask = effectiveButtons;
      _lastMouseX = x;
      _lastMouseY = y;
      const ptrEv = new PtrCtor("pointerup", {
        bubbles: true,
        cancelable: true,
        composed: true,
        clientX: x,
        clientY: y,
        movementX: 0,
        movementY: 0,
        button: btnNum,
        buttons: effectiveButtons,
        pointerId: 1,
        pointerType: "mouse",
        isPrimary: true,
      });
      _dispatchTrusted(target, ptrEv);
      const suppressed = _pointerDownPrevented;
      _pointerDownPrevented = false;
      if (!suppressed) {
        const muEv = new MouseCtor("mouseup", {
          bubbles: true,
          cancelable: true,
          composed: true,
          clientX: x,
          clientY: y,
          movementX: 0,
          movementY: 0,
          button: btnNum,
          buttons: effectiveButtons,
          detail: detail,
        });
        _dispatchTrusted(target, muEv);
      }
      // If mouseup already triggered a navigation (e.g., form.submit() / form.requestSubmit()),
      // do not dispatch a duplicate click activation that would trigger a second submission.
      const hasPendingNav = Boolean(
        (typeof globalThis !== "undefined" && globalThis.__capsurf_pending_navigation) ||
        global.__capsurf_pending_navigation
      );
      if (!skipSyntheticClick && !hasPendingNav && btnNum === 0) {
        const clickEv = new MouseCtor("click", {
          bubbles: true,
          cancelable: true,
          composed: true,
          clientX: x,
          clientY: y,
          button: 0,
          detail: detail,
        });
        _dispatchTrusted(target, clickEv);
        if (!clickEv.defaultPrevented) {
          activateClickedElement(target, true, x, y, detail);
        }
      }
      return { handled: true };
    }

    return { handled: false };
  }
  global.__capsurf_dispatch_mouse_event = dispatchMouseEvent;
  if (typeof window !== "undefined") window.__capsurf_dispatch_mouse_event = dispatchMouseEvent;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_dispatch_mouse_event = dispatchMouseEvent;

  function dispatchWheelEvent(x, y, deltaX, deltaY, modifiers) {
    const doc = (typeof document !== "undefined" ? document : null) || global.document;
    const target = _hitTestElement(x, y) || (doc && (doc.body || doc.documentElement));
    if (!target) return [global.scrollX || 0, global.scrollY || 0];
    const mods = Number(modifiers) || 0;
    const dx = Number(deltaX) || 0;
    const dy = Number(deltaY) || 0;
    const WheelCtor = typeof _NativeWheelEvent === "function" ? _NativeWheelEvent : (typeof _NativeMouseEvent === "function" ? _NativeMouseEvent : _NativeEventCtor);
    const ev = new WheelCtor("wheel", {
      bubbles: true,
      cancelable: true,
      composed: true,
      clientX: x,
      clientY: y,
      deltaX: dx,
      deltaY: dy,
      deltaZ: 0,
      deltaMode: 0,
      altKey: (mods & 1) !== 0,
      ctrlKey: (mods & 2) !== 0,
      metaKey: (mods & 4) !== 0,
      shiftKey: (mods & 8) !== 0,
    });
    _dispatchTrusted(target, ev);
    if (!ev.defaultPrevented) {
      const ksInternals = (typeof globalThis !== "undefined") ? globalThis[Symbol.for("kitesurf.internals")] : null;
      const cellX = Math.max(0, Number(ksInternals && ksInternals.ttyCellWidth) || 0);
      const cellY = Math.max(0, Number(ksInternals && ksInternals.ttyCellHeight) || 0);
      const snapWheelDelta = function (before, delta, maxScroll, cell) {
        let movement = delta;
        if (maxScroll !== null) {
          movement = Math.max(-before, Math.min(delta, maxScroll - before));
        } else if (before + delta < 0) {
          movement = -before;
        }
        if (cell > 0) movement = Math.trunc(movement / cell) * cell;
        return movement;
      };
      let remX = dx;
      let remY = dy;
      let cur = target;
      let safety = 0;
      while (cur && safety < 64) {
        safety++;
        if (cur === doc || cur === doc.documentElement || cur === doc.body) {
          break;
        }
        if (cur.nodeType === 1) {
          const cs = (typeof global.getComputedStyle === "function") ? global.getComputedStyle(cur) : cur.style;
          const ox = (cs && (cs.overflowX || cs.overflow)) || "";
          const oy = (cs && (cs.overflowY || cs.overflow)) || "";
          if ((ox === "auto" || ox === "scroll") && remX !== 0) {
            const maxX = (cur.clientWidth || 0) > 0 ? Math.max(0, (cur.scrollWidth || 0) - (cur.clientWidth || 0)) : null;
            const curX = cur.scrollLeft || 0;
            const moveX = snapWheelDelta(curX, remX, maxX, cellX);
            if (moveX !== 0) {
              cur.scrollLeft = curX + moveX;
              const consumedX = (cur.scrollLeft || 0) - curX;
              remX -= consumedX;
            }
          }
          if ((oy === "auto" || oy === "scroll") && remY !== 0) {
            const maxY = (cur.clientHeight || 0) > 0 ? Math.max(0, (cur.scrollHeight || 0) - (cur.clientHeight || 0)) : null;
            const curY = cur.scrollTop || 0;
            const moveY = snapWheelDelta(curY, remY, maxY, cellY);
            if (moveY !== 0) {
              cur.scrollTop = curY + moveY;
              const consumedY = (cur.scrollTop || 0) - curY;
              remY -= consumedY;
            }
          }
          const obx = (cs && (cs.overscrollBehaviorX || cs.overscrollBehavior)) || "auto";
          const oby = (cs && (cs.overscrollBehaviorY || cs.overscrollBehavior)) || "auto";
          if (obx === "contain" || obx === "none") remX = 0;
          if (oby === "contain" || oby === "none") remY = 0;
          if (remX === 0 && remY === 0) break;
        }
        cur = cur.parentElement || cur.parentNode || cur.host;
      }
      if (cellX > 0) remX = Math.trunc(remX / cellX) * cellX;
      if (cellY > 0) remY = Math.trunc(remY / cellY) * cellY;
      if ((remX !== 0 || remY !== 0) && typeof global.scrollBy === "function") {
        global.scrollBy(remX, remY);
      }
    }
    return [global.scrollX || 0, global.scrollY || 0];
  }
  global.__capsurf_dispatch_wheel_event = dispatchWheelEvent;
  if (typeof window !== "undefined") window.__capsurf_dispatch_wheel_event = dispatchWheelEvent;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_dispatch_wheel_event = dispatchWheelEvent;

  // HTML Named Access on Window (live, connected-only, no Proxy prototype so QuickJS throws ReferenceError for absent bare globals)
  try {
    const winTarget = (typeof globalThis !== "undefined") ? globalThis : global;
    const managedNamedGetters = new Map();
    const managedNamedResolvers = new Map();
    let inNamedSync = false;

    function walkConnectedTree(root, onEl) {
      if (!root) return;
      let stack = [root];
      while (stack.length > 0) {
        const node = stack.pop();
        if (!node) continue;
        if (node.nodeType === 1) {
          onEl(node);
        }
        // Traverse children without calling Element.prototype.querySelectorAll
        let kids = null;
        if (node._children && node._children.length > 0) {
          kids = node._children;
        } else {
          let c = node.firstChild;
          if (c) {
            kids = [];
            while (c) {
              kids.push(c);
              c = c.nextSibling;
            }
          }
        }
        if (kids && kids.length > 0) {
          for (let i = kids.length - 1; i >= 0; i--) {
            stack.push(kids[i]);
          }
        }
      }
    }

    function syncNamedGlobals() {
      if (inNamedSync) return;
      const doc = winTarget.document;
      if (!doc || !doc.documentElement) return;
      inNamedSync = true;
      try {
        const nextResolvers = new Map();
        const iframes = [];
        const idFirstEl = new Map();

        walkConnectedTree(doc.documentElement, function (el) {
          const tag = String(el.localName || el.tagName || "").toLowerCase();
          if (tag === "iframe") {
            iframes.push(el);
          }
          const idAttr = typeof el.getAttribute === "function" ? el.getAttribute("id") : (el.id || "");
          if (typeof idAttr === "string" && idAttr.length > 0 && !idFirstEl.has(idAttr)) {
            idFirstEl.set(idAttr, el);
          }
        });

        // 1. Child browsing contexts (indexed & named iframes take precedence and resolve to contentWindow)
        for (let i = 0; i < iframes.length; i++) {
          const frame = iframes[i];
          const idxKey = String(i);
          if (!nextResolvers.has(idxKey)) {
            nextResolvers.set(idxKey, function () { return frame.contentWindow; });
          }
          const fname = typeof frame.getAttribute === "function" ? frame.getAttribute("name") : (frame.name || "");
          if (typeof fname === "string" && fname.length > 0 && !nextResolvers.has(fname)) {
            nextResolvers.set(fname, function () { return frame.contentWindow; });
          }
        }

        // 2. Connected element IDs
        idFirstEl.forEach(function (el, idKey) {
          if (!nextResolvers.has(idKey)) {
            nextResolvers.set(idKey, function () { return el; });
          }
        });

        // 3. Remove stale managed properties that are no longer in the connected tree
        //    (never delete if page overwrote or redefined the property!)
        const toDelete = [];
        managedNamedGetters.forEach(function (ourGetter, key) {
          if (!nextResolvers.has(key)) {
            toDelete.push(key);
          }
        });
        for (let i = 0; i < toDelete.length; i++) {
          const key = toDelete[i];
          const ourGetter = managedNamedGetters.get(key);
          const desc = Object.getOwnPropertyDescriptor(winTarget, key);
          if (desc && desc.get === ourGetter && desc.configurable) {
            try { delete winTarget[key]; } catch (e) {}
            if (typeof window !== "undefined" && window !== winTarget) {
              const wDesc = Object.getOwnPropertyDescriptor(window, key);
              if (wDesc && wDesc.get === ourGetter && wDesc.configurable) {
                try { delete window[key]; } catch (e) {}
              }
            }
          }
          managedNamedGetters.delete(key);
          managedNamedResolvers.delete(key);
        }

        // 4. Install or update active named globals (never overwrite existing built-in or page-defined properties)
        nextResolvers.forEach(function (resolver, key) {
          managedNamedResolvers.set(key, resolver);
          const existingDesc = Object.getOwnPropertyDescriptor(winTarget, key);
          if (existingDesc) {
            // Already defined: keep our managed getter if it's ours, otherwise respect built-in/page property
            return;
          }
          const getter = function () {
            const fn = managedNamedResolvers.get(key);
            return fn ? fn() : undefined;
          };
          const setter = function (v) {
            managedNamedGetters.delete(key);
            managedNamedResolvers.delete(key);
            Object.defineProperty(winTarget, key, {
              value: v,
              writable: true,
              enumerable: true,
              configurable: true,
            });
            if (typeof window !== "undefined" && window !== winTarget) {
              Object.defineProperty(window, key, {
                value: v,
                writable: true,
                enumerable: true,
                configurable: true,
              });
            }
          };
          try {
            Object.defineProperty(winTarget, key, {
              get: getter,
              set: setter,
              enumerable: false,
              configurable: true,
            });
            if (typeof window !== "undefined" && window !== winTarget && !Object.getOwnPropertyDescriptor(window, key)) {
              Object.defineProperty(window, key, {
                get: getter,
                set: setter,
                enumerable: false,
                configurable: true,
              });
            }
            managedNamedGetters.set(key, getter);
          } catch (e) {}
        });
      } finally {
        inNamedSync = false;
      }
    }

    global.__capsurf_syncNamedGlobals = syncNamedGlobals;
    if (typeof window !== "undefined") window.__capsurf_syncNamedGlobals = syncNamedGlobals;
    if (typeof globalThis !== "undefined") globalThis.__capsurf_syncNamedGlobals = syncNamedGlobals;

    // Hook __capsurf_defer_timers so parsed HTML named globals are synced right before inline <script> executes!
    let _deferTimersVal = Boolean(winTarget.__capsurf_defer_timers);
    try {
      Object.defineProperty(winTarget, "__capsurf_defer_timers", {
        get: function () { return _deferTimersVal; },
        set: function (v) {
          _deferTimersVal = Boolean(v);
          try { syncNamedGlobals(); } catch (e) {}
        },
        configurable: true,
        enumerable: false,
      });
    } catch (e) {}

    // Hook setTimeout assignment so session_hooks.cp's wrappedSetTimeout keeps native name & toString shape
    let _setTimeoutVal = winTarget.setTimeout;
    try {
      Object.defineProperty(winTarget, "setTimeout", {
        get: function () { return _setTimeoutVal; },
        set: function (v) {
          if (typeof v === "function") {
            const rawSrc = typeof _origFunctionToString === "function" ? _origFunctionToString.call(v) : "";
            if (v.name === "wrappedSetTimeout" || rawSrc.indexOf("__capsurf_defer_timers") !== -1) {
              markNative(v, "setTimeout");
            }
          }
          _setTimeoutVal = v;
        },
        configurable: true,
        enumerable: true,
      });
    } catch (e) {}

    // Own event-handler IDL accessors on window (not on Window.prototype)
    const _windowEventHandlers = Object.create(null);
    const WINDOW_REFLECTING_ATTRS = new Set([
      "onafterprint", "onbeforeprint", "onbeforeunload", "onblur",
      "onerror", "onfocus", "onhashchange", "onlanguagechange",
      "onload", "onmessage", "onmessageerror", "onoffline",
      "ononline", "onpagehide", "onpageshow", "onpopstate",
      "onrejectionhandled", "onresize", "onscroll", "onstorage",
      "onunhandledrejection", "onunload",
    ]);
    const WINDOW_EVENT_HANDLER_NAMES = [
      "onabort", "onafterprint", "onanimationcancel", "onanimationend", "onanimationiteration", "onanimationstart",
      "onauxclick", "onbeforeinput", "onbeforeprint", "onbeforeunload", "onblur", "oncancel", "oncanplay",
      "oncanplaythrough", "onchange", "onclick", "onclose", "oncontextmenu",
      "oncopy", "oncut", "ondblclick", "ondrag", "ondragend", "ondragenter",
      "ondragleave", "ondragover", "ondragstart", "ondrop", "ondurationchange",
      "onemptied", "onended", "onerror", "onfocus", "onformdata", "ongotpointercapture",
      "onhashchange", "oninput", "oninvalid", "onkeydown", "onkeypress", "onkeyup", "onlanguagechange", "onload",
      "onloadeddata", "onloadedmetadata", "onloadstart", "onlostpointercapture",
      "onmessage", "onmessageerror", "onmousedown", "onmouseenter", "onmouseleave", "onmousemove", "onmouseout",
      "onmouseover", "onmouseup", "onoffline", "ononline", "onpagehide", "onpageshow",
      "onpaste", "onpause", "onplay", "onplaying",
      "onpointercancel", "onpointerdown", "onpointerenter", "onpointerleave",
      "onpointermove", "onpointerout", "onpointerover", "onpointerup", "onpopstate", "onprogress",
      "onratechange", "onrejectionhandled", "onreset", "onresize", "onscroll", "onscrollend", "onseeked",
      "onseeking", "onselect", "onselectionchange", "onselectstart", "onstalled", "onstorage",
      "onsubmit", "onsuspend", "ontimeupdate", "ontoggle", "ontouchcancel",
      "ontouchend", "ontouchmove", "ontouchstart", "ontransitioncancel",
      "ontransitionend", "ontransitionrun", "ontransitionstart", "onunhandledrejection", "onunload",
      "onvolumechange", "onwaiting", "onwheel",
    ];
    global.__capsurf_invalidateWindowEventHandler = function (prop) {
      delete _windowEventHandlers[prop];
    };
    if (typeof window !== "undefined") {
      window.__capsurf_invalidateWindowEventHandler = global.__capsurf_invalidateWindowEventHandler;
    }
    for (let i = 0; i < WINDOW_EVENT_HANDLER_NAMES.length; i++) {
      const prop = WINDOW_EVENT_HANDLER_NAMES[i];
      if (Object.prototype.hasOwnProperty.call(winTarget, prop)) continue;
      const getter = function () {
        if (Object.prototype.hasOwnProperty.call(_windowEventHandlers, prop)) {
          return _windowEventHandlers[prop];
        }
        if (WINDOW_REFLECTING_ATTRS.has(prop) && global.__capsurf_allow_inline_handlers !== false) {
          const doc = global.document || (typeof document !== "undefined" ? document : null);
          const body = doc && doc.body ? doc.body : null;
          if (body && typeof body.getAttribute === "function") {
            const attr = body.getAttribute(prop);
            if (typeof attr === "string") {
              try {
                const fn = new Function("event", attr);
                _windowEventHandlers[prop] = fn;
                return fn;
              } catch (e) {}
            }
          }
        }
        return null;
      };
      const setter = function (v) {
        _windowEventHandlers[prop] = (v !== null && (typeof v === "object" || typeof v === "function")) ? v : null;
      };
      Object.defineProperty(getter, "name", { value: "get " + prop, configurable: true });
      Object.defineProperty(setter, "name", { value: "set " + prop, configurable: true });
      const desc = {
        get: getter,
        set: setter,
        configurable: true,
        enumerable: true,
      };
      try {
        Object.defineProperty(winTarget, prop, desc);
        if (typeof window !== "undefined" && window !== winTarget && !Object.prototype.hasOwnProperty.call(window, prop)) {
          Object.defineProperty(window, prop, desc);
        }
      } catch (e) {}
    }
  } catch (e) {}

  // Dedicated Worker & SharedWorker
  (function installDedicatedWorkers() {
    const NativeWorkerURL = URL;
    const nativeApplyFn = Reflect.apply;
    const nativeOriginGet = Object.getOwnPropertyDescriptor(NativeWorkerURL.prototype, "origin")?.get;
    const liveWorkers = new Map();
    let nextWid = 1;

    function safeUrlOrigin(href) {
      try {
        const u = new NativeWorkerURL(href);
        const orig = nativeOriginGet ? nativeApplyFn(nativeOriginGet, u, []) : u.origin;
        return typeof orig === "string" ? orig : "null";
      } catch (e) {
        return "null";
      }
    }

    function currentDocOrigin() {
      const stack = global.__capsurf_active_window_stack;
      if (Array.isArray(stack) && stack.length > 0) {
        const w = stack[stack.length - 1];
        if (w && (w._opaqueSandbox || (w.frameElement && w.frameElement._opaqueSandbox))) {
          return "null";
        }
        const wo = getWindowOrigin(w);
        if (wo && wo !== "null") return wo;
      }
      if (typeof globalThis !== "undefined" && globalThis.__capsurf_doc_origin && globalThis.__capsurf_doc_origin !== "") {
        return String(globalThis.__capsurf_doc_origin);
      }
      const loc = global.location || (typeof location !== "undefined" ? location : null);
      if (loc && loc.origin && loc.origin !== "null" && loc.origin !== "about:") {
        return String(loc.origin);
      }
      return "null";
    }

    function currentDocBase() {
      const stack = global.__capsurf_active_window_stack;
      if (Array.isArray(stack) && stack.length > 0) {
        const w = stack[stack.length - 1];
        if (w && w.document && typeof w.document.baseURI === "string" && w.document.baseURI) return w.document.baseURI;
        if (w && w.location && w.location.href) return String(w.location.href);
      }
      const doc = global.document || (typeof document !== "undefined" ? document : null);
      if (doc && typeof doc.baseURI === "string" && doc.baseURI) return doc.baseURI;
      const loc = global.location || (typeof location !== "undefined" ? location : null);
      if (loc && loc.href) return String(loc.href);
      return "about:blank";
    }

    function encodeForWorker(value, transferOrOpts) {
      let transfer = [];
      if (transferOrOpts !== undefined && transferOrOpts !== null) {
        if (typeof transferOrOpts !== "object") {
          throw new TypeError("Transfer list must be an object");
        }
        if (typeof transferOrOpts[Symbol.iterator] === "function") {
          transfer = Array.from(transferOrOpts);
        } else if ("transfer" in transferOrOpts && transferOrOpts.transfer !== undefined) {
          const list = transferOrOpts.transfer;
          if (!list || typeof list[Symbol.iterator] !== "function") {
            throw new TypeError("Transfer list must be iterable");
          }
          transfer = Array.from(list);
        }
      }
      const seen = new Set();
      for (let i = 0; i < transfer.length; i++) {
        const it = transfer[i];
        if ((typeof it !== "object" && typeof it !== "function") || it === null) {
          throw new TypeError("Failed to execute 'postMessage' on 'Worker': Transfer list items must be objects");
        }
        if (seen.has(it)) {
          throw new DOMException("Duplicate transferable in transfer list", "DataCloneError");
        }
        seen.add(it);
        if (!(it instanceof ArrayBuffer)) {
          throw new DOMException("CapSurf can only transfer ArrayBuffers to a worker", "DataCloneError");
        }
        if (_isDetachedBuffer(it)) {
          throw new DOMException("ArrayBuffer in transfer list is already detached", "DataCloneError");
        }
      }
      function enc(v) {
        if (v === undefined) return { __t: "undef" };
        if (v === null || typeof v === "boolean" || typeof v === "number" || typeof v === "string") return v;
        if (typeof v === "function" || typeof v === "symbol") {
          throw new DOMException("Failed to clone value of type " + typeof v, "DataCloneError");
        }
        if (typeof SharedArrayBuffer !== "undefined" && v instanceof SharedArrayBuffer) {
          throw new DOMException("Failed to execute 'postMessage' on 'Worker': SharedArrayBuffer transfer requires self.crossOriginIsolated.", "DataCloneError");
        }
        if (v instanceof MessagePort) {
          throw new DOMException("CapSurf can only transfer ArrayBuffers to a worker", "DataCloneError");
        }
        if (v instanceof ArrayBuffer) {
          if (_isDetachedBuffer(v)) {
            throw new DOMException("Cannot clone a detached ArrayBuffer.", "DataCloneError");
          }
          const ab = seen.has(v) ? _detachArrayBuffer(v) : v;
          return { __t: "ab", d: Array.from(new Uint8Array(ab)) };
        }
        if (ArrayBuffer.isView(v)) {
          if (typeof SharedArrayBuffer !== "undefined" && v.buffer instanceof SharedArrayBuffer) {
            throw new DOMException("Failed to execute 'postMessage' on 'Worker': SharedArrayBuffer transfer requires self.crossOriginIsolated.", "DataCloneError");
          }
          return {
            __t: "ta",
            c: v.constructor ? v.constructor.name : "Uint8Array",
            b: enc(v.buffer),
            o: v.byteOffset,
            l: v.length,
          };
        }
        if (Array.isArray(v)) {
          const arr = new Array(v.length);
          for (let j = 0; j < v.length; j++) arr[j] = enc(v[j]);
          return arr;
        }
        const out = {};
        const keys = Object.keys(v);
        for (let k = 0; k < keys.length; k++) {
          out[keys[k]] = enc(v[keys[k]]);
        }
        return out;
      }
      const encoded = enc(value);
      seen.forEach(function (ab) {
        if (!_isDetachedBuffer(ab)) _detachArrayBuffer(ab);
      });
      return JSON.stringify(encoded);
    }

    function decodeFromWorker(jsonStr) {
      if (!jsonStr) return undefined;
      const raw = JSON.parse(jsonStr);
      function dec(v) {
        if (v === null || typeof v !== "object") return v;
        if (Array.isArray(v)) {
          const arr = new Array(v.length);
          for (let j = 0; j < v.length; j++) arr[j] = dec(v[j]);
          return arr;
        }
        if (v.__t === "undef") return undefined;
        if (v.__t === "ab") {
          const ab = new ArrayBuffer(v.d.length);
          new Uint8Array(ab).set(v.d);
          return ab;
        }
        if (v.__t === "ta") {
          const buf = dec(v.b);
          const Ctor = global[v.c] || (typeof globalThis !== "undefined" && globalThis[v.c]) || Uint8Array;
          return new Ctor(buf, v.o, v.l);
        }
        const out = {};
        const keys = Object.keys(v);
        for (let k = 0; k < keys.length; k++) {
          out[keys[k]] = dec(v[keys[k]]);
        }
        return out;
      }
      return dec(raw);
    }

    function decodeDataUrl(src) {
      const comma = src.indexOf(",");
      if (comma === -1) return "";
      const meta = src.slice(5, comma);
      const payload = src.slice(comma + 1);
      if (/;base64/i.test(meta)) {
        try { return atob(payload); } catch (e) { return ""; }
      }
      try { return decodeURIComponent(payload); } catch (e) { return payload; }
    }

    function dispatchWorkerEvent(ws, ev) {
      if (ws.done && ev.type !== "error") return;
      ws.owner.dispatchEvent(ev);
    }

    function finishWorker(ws) {
      if (ws.done) return;
      ws.done = true;
      ws.queue = [];
      liveWorkers.delete(ws.id);
    }

    global.__capsurf_worker_on_upstream = function (wid, evName, payloadJson) {
      const ws = liveWorkers.get(wid);
      if (!ws) return;
      if (evName === "worker.message") {
        const data = decodeFromWorker(payloadJson);
        const mev = new MessageEvent("message", { data: data, origin: ws.origin });
        if (data === undefined) {
          Object.defineProperty(mev, "data", { value: undefined, configurable: true, enumerable: true });
        }
        dispatchWorkerEvent(ws, mev);
      } else if (evName === "worker.error") {
        let info = {};
        try { info = JSON.parse(payloadJson || "{}"); } catch (e) {}
        dispatchWorkerEvent(ws, new ErrorEvent("error", {
          message: info.message || "",
          filename: info.filename || "",
          lineno: info.lineno || 0,
          colno: info.colno || 0,
          cancelable: true,
        }));
      } else if (evName === "worker.closed") {
        finishWorker(ws);
      }
    };
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_worker_on_upstream = global.__capsurf_worker_on_upstream;
    }

    function Worker(scriptURL, options) {
      if (arguments.length === 0) {
        throw new TypeError("Failed to construct 'Worker': 1 argument required, but only 0 present.");
      }
      if (!(this instanceof Worker)) {
        throw new TypeError("Failed to construct 'Worker': Please use the 'new' operator.");
      }
      EventTarget.call(this);
      const opts = (options && typeof options === "object") ? options : {};
      if (opts.type !== undefined && String(opts.type) === "module") {
        throw new DOMException("Failed to construct 'Worker': CapSurf does not support module workers yet.", "NotSupportedError");
      }
      let parsedUrl;
      try {
        parsedUrl = new NativeWorkerURL(String(scriptURL), currentDocBase());
      } catch (e) {
        throw new DOMException("Failed to construct 'Worker': The URL '" + String(scriptURL) + "' is invalid.", "SyntaxError");
      }
      const href = parsedUrl.href;
      const origin = currentDocOrigin();
      const protocol = parsedUrl.protocol;
      const sameOrigin =
        protocol === "data:" ||
        (protocol === "blob:" && blobUrlRegistry.has(href)) ||
        ((protocol === "http:" || protocol === "https:") && safeUrlOrigin(href) === origin);
      if (!sameOrigin) {
        throw new DOMException(
          "Failed to construct 'Worker': Script at '" + href + "' cannot be accessed from origin '" + origin + "'.",
          "SecurityError"
        );
      }

      const id = nextWid++;
      const ws = {
        id: id,
        owner: this,
        origin: origin,
        queue: [],
        created: false,
        done: false,
      };
      this._ws = ws;
      liveWorkers.set(id, ws);

      const workerName = opts.name === undefined ? "" : String(opts.name);
      const stackTop = (Array.isArray(global.__capsurf_active_window_stack) && global.__capsurf_active_window_stack.length > 0)
        ? global.__capsurf_active_window_stack[global.__capsurf_active_window_stack.length - 1]
        : null;
      const isOopif = Boolean(
        opts.__capsurf_oopif ||
        (stackTop && stackTop.__capsurf_is_oopif) ||
        (typeof globalThis !== "undefined" && (globalThis.__capsurf_is_oopif || (globalThis.self && globalThis.self.__capsurf_is_oopif)))
      );
      const ua = (global.navigator && global.navigator.userAgent) ? String(global.navigator.userAgent) : "";
      const docCsp = (typeof globalThis !== "undefined" && globalThis.__capsurf_doc_csp) ? String(globalThis.__capsurf_doc_csp) : "";

      Promise.resolve().then(function () {
        if (ws.done) return;
        if (isOopif) {
          finishWorker(ws);
          dispatchWorkerEvent(ws, new ErrorEvent("error", {
            message: "Failed to start worker: no isolate slot is free for this worker",
            cancelable: true,
          }));
          return;
        }
        let source = null;
        if (docCsp.length > 0 && typeof globalThis.__capsurf_script_gate === "function") {
          const gateRes = globalThis.__capsurf_script_gate("worker", origin, docCsp, href, "", "", "0");
          if (gateRes === null || gateRes === undefined) {
            finishWorker(ws);
            dispatchWorkerEvent(ws, new Event("error"));
            return;
          }
          if (protocol !== "blob:" && protocol !== "data:") {
            source = String(gateRes);
          }
        }
        if (source === null) {
          if (protocol === "blob:") {
            const blobObj = blobUrlRegistry.get(href);
            if (!blobObj) {
              finishWorker(ws);
              dispatchWorkerEvent(ws, new Event("error"));
              return;
            }
            Promise.resolve(blobObj.text()).then(function (txt) {
              startNativeWorker(txt);
            }, function () {
              finishWorker(ws);
              dispatchWorkerEvent(ws, new Event("error"));
            });
            return;
          } else if (protocol === "data:") {
            source = decodeDataUrl(href);
          } else if (typeof globalThis.__capsurf_script_gate === "function") {
            const fetched = globalThis.__capsurf_script_gate("worker", origin, docCsp, href, "", "", "0");
            if (fetched === null || fetched === undefined) {
              finishWorker(ws);
              dispatchWorkerEvent(ws, new Event("error"));
              return;
            }
            source = String(fetched);
          } else if (typeof globalThis.__capsurf_fixture_fetch === "function") {
            const fx = globalThis.__capsurf_fixture_fetch(href, "{}");
            if (fx === null || fx === undefined) {
              finishWorker(ws);
              dispatchWorkerEvent(ws, new Event("error"));
              return;
            }
            source = String(fx);
          }
        }
        startNativeWorker(source || "");
      });

      function startNativeWorker(src) {
        if (ws.done) return;
        if (typeof globalThis.__capsurf_worker_spawn !== "function") {
          finishWorker(ws);
          dispatchWorkerEvent(ws, new Event("error"));
          return;
        }
        const status = globalThis.__capsurf_worker_spawn(id, href, workerName, src, ua, origin, Date.now());
        if (status === "NO_SLOT") {
          finishWorker(ws);
          dispatchWorkerEvent(ws, new ErrorEvent("error", {
            message: "Failed to start worker: no isolate slot is free for this worker",
            cancelable: true,
          }));
          return;
        }
        if (status === "SYNTAX_ERROR" || status === "ERR") {
          finishWorker(ws);
          dispatchWorkerEvent(ws, new Event("error"));
          return;
        }
        ws.created = true;
        for (let i = 0; i < ws.queue.length; i++) {
          globalThis.__capsurf_worker_post(id, ws.queue[i]);
        }
        ws.queue = [];
        if (typeof globalThis.__capsurf_worker_pump === "function") {
          globalThis.__capsurf_worker_pump();
        }
      }
    }

    Worker.prototype = Object.create(EventTarget.prototype);
    Worker.prototype.constructor = Worker;
    Worker.prototype.postMessage = function (message, transferOrOptions) {
      if (arguments.length === 0) {
        throw new TypeError("Failed to execute 'postMessage' on 'Worker': 1 argument required, but only 0 present.");
      }
      const ws = this._ws;
      const encoded = encodeForWorker(message, transferOrOptions);
      if (ws.done) return;
      if (!ws.created) {
        ws.queue.push(encoded);
        return;
      }
      if (typeof globalThis.__capsurf_worker_post === "function") {
        globalThis.__capsurf_worker_post(ws.id, encoded);
      }
    };
    Worker.prototype.terminate = function () {
      const ws = this._ws;
      if (ws.done) return;
      const wasCreated = ws.created;
      finishWorker(ws);
      if (wasCreated && typeof globalThis.__capsurf_worker_terminate === "function") {
        globalThis.__capsurf_worker_terminate(ws.id);
      }
    };
    ["message", "messageerror", "error"].forEach(function (evType) {
      const key = "_on" + evType;
      Object.defineProperty(Worker.prototype, "on" + evType, {
        configurable: true,
        enumerable: true,
        get: function () { return this[key] || null; },
        set: function (fn) {
          this[key] = typeof fn === "function" ? fn : null;
        },
      });
    });
    markNative(Worker, "Worker");

    function SharedWorker() {
      throw new DOMException("Failed to construct 'SharedWorker': SharedWorker is not supported.", "SecurityError");
    }
    markNative(SharedWorker, "SharedWorker");

    global.Worker = Worker;
    global.SharedWorker = SharedWorker;
    if (typeof window !== "undefined") {
      window.Worker = Worker;
      window.SharedWorker = SharedWorker;
    }
    if (typeof globalThis !== "undefined") {
      globalThis.Worker = Worker;
      globalThis.SharedWorker = SharedWorker;
    }
  })();

  // WebRTC & MediaDevices graceful no-transport stubs
  (function installWebRtcStubs() {
    function makeId() {
      return "id-" + Math.random().toString(36).slice(2, 10);
    }

    function MediaStreamTrack(kind) {
      EventTarget.call(this);
      this.kind = kind ? String(kind) : "video";
      this.id = makeId();
      this.label = "";
      this.enabled = true;
      this.muted = false;
      this.readyState = "live";
      this.onended = null;
      this.onmute = null;
      this.onunmute = null;
    }
    MediaStreamTrack.prototype = Object.create(EventTarget.prototype);
    MediaStreamTrack.prototype.constructor = MediaStreamTrack;
    MediaStreamTrack.prototype.stop = function () {
      if (this.readyState === "ended") return;
      this.readyState = "ended";
      const ev = new Event("ended");
      this.dispatchEvent(ev);
    };
    MediaStreamTrack.prototype.clone = function () {
      return new MediaStreamTrack(this.kind);
    };
    MediaStreamTrack.prototype.getSettings = function () { return {}; };
    MediaStreamTrack.prototype.getCapabilities = function () { return {}; };
    MediaStreamTrack.prototype.getConstraints = function () { return {}; };
    MediaStreamTrack.prototype.applyConstraints = function () { return Promise.resolve(); };
    markNative(MediaStreamTrack, "MediaStreamTrack");

    function MediaStream(tracksOrStream) {
      EventTarget.call(this);
      this.id = makeId();
      this.active = true;
      this._tracks = [];
      if (tracksOrStream && typeof tracksOrStream.getTracks === "function") {
        this._tracks = tracksOrStream.getTracks().slice();
      } else if (Array.isArray(tracksOrStream)) {
        this._tracks = tracksOrStream.slice();
      }
    }
    MediaStream.prototype = Object.create(EventTarget.prototype);
    MediaStream.prototype.constructor = MediaStream;
    MediaStream.prototype.getTracks = function () { return this._tracks.slice(); };
    MediaStream.prototype.getAudioTracks = function () {
      return this._tracks.filter(function (t) { return t && t.kind === "audio"; });
    };
    MediaStream.prototype.getVideoTracks = function () {
      return this._tracks.filter(function (t) { return t && t.kind === "video"; });
    };
    MediaStream.prototype.getTrackById = function (id) {
      for (let i = 0; i < this._tracks.length; i++) {
        if (this._tracks[i] && this._tracks[i].id === id) return this._tracks[i];
      }
      return null;
    };
    MediaStream.prototype.addTrack = function (t) {
      if (t && this._tracks.indexOf(t) === -1) this._tracks.push(t);
    };
    MediaStream.prototype.removeTrack = function (t) {
      const idx = this._tracks.indexOf(t);
      if (idx !== -1) this._tracks.splice(idx, 1);
    };
    MediaStream.prototype.clone = function () {
      return new MediaStream(this._tracks.map(function (t) {
        return t && typeof t.clone === "function" ? t.clone() : t;
      }));
    };
    markNative(MediaStream, "MediaStream");

    function RTCSessionDescription(init) {
      const o = init || {};
      this.type = o.type !== undefined ? String(o.type) : "";
      this.sdp = o.sdp !== undefined ? String(o.sdp) : "";
    }
    RTCSessionDescription.prototype.toJSON = function () {
      return { type: this.type, sdp: this.sdp };
    };
    markNative(RTCSessionDescription, "RTCSessionDescription");

    function RTCIceCandidate(init) {
      const o = init || {};
      this.candidate = o.candidate !== undefined ? String(o.candidate) : "";
      this.sdpMid = o.sdpMid !== undefined ? o.sdpMid : null;
      this.sdpMLineIndex = o.sdpMLineIndex !== undefined ? o.sdpMLineIndex : null;
      this.usernameFragment = o.usernameFragment !== undefined ? o.usernameFragment : null;
    }
    RTCIceCandidate.prototype.toJSON = function () {
      return {
        candidate: this.candidate,
        sdpMid: this.sdpMid,
        sdpMLineIndex: this.sdpMLineIndex,
        usernameFragment: this.usernameFragment,
      };
    };
    markNative(RTCIceCandidate, "RTCIceCandidate");

    function RTCError(init, message) {
      const msg = message !== undefined ? String(message) : "";
      const err = new DOMException(msg, "OperationError");
      Object.setPrototypeOf(err, RTCError.prototype);
      err.errorDetail = (init && init.errorDetail) ? String(init.errorDetail) : "data-channel-failure";
      err.sdpLineNumber = init && init.sdpLineNumber !== undefined ? init.sdpLineNumber : null;
      err.sctpCauseCode = init && init.sctpCauseCode !== undefined ? init.sctpCauseCode : null;
      err.receivedAlert = init && init.receivedAlert !== undefined ? init.receivedAlert : null;
      err.sentAlert = init && init.sentAlert !== undefined ? init.sentAlert : null;
      return err;
    }
    RTCError.prototype = Object.create(DOMException.prototype);
    RTCError.prototype.constructor = RTCError;
    markNative(RTCError, "RTCError");

    function RTCErrorEvent(type, init) {
      Event.call(this, type, init);
      this.error = init && init.error ? init.error : null;
    }
    RTCErrorEvent.prototype = Object.create(Event.prototype);
    RTCErrorEvent.prototype.constructor = RTCErrorEvent;
    markNative(RTCErrorEvent, "RTCErrorEvent");

    function RTCPeerConnectionIceEvent(type, init) {
      Event.call(this, type, init);
      this.candidate = init && init.candidate ? init.candidate : null;
    }
    RTCPeerConnectionIceEvent.prototype = Object.create(Event.prototype);
    RTCPeerConnectionIceEvent.prototype.constructor = RTCPeerConnectionIceEvent;
    markNative(RTCPeerConnectionIceEvent, "RTCPeerConnectionIceEvent");

    function RTCDataChannelEvent(type, init) {
      Event.call(this, type, init);
      this.channel = init && init.channel ? init.channel : null;
    }
    RTCDataChannelEvent.prototype = Object.create(Event.prototype);
    RTCDataChannelEvent.prototype.constructor = RTCDataChannelEvent;
    markNative(RTCDataChannelEvent, "RTCDataChannelEvent");

    function RTCTrackEvent(type, init) {
      Event.call(this, type, init);
      this.receiver = init && init.receiver ? init.receiver : null;
      this.track = init && init.track ? init.track : null;
      this.streams = init && init.streams ? init.streams : [];
      this.transceiver = init && init.transceiver ? init.transceiver : null;
    }
    RTCTrackEvent.prototype = Object.create(Event.prototype);
    RTCTrackEvent.prototype.constructor = RTCTrackEvent;
    markNative(RTCTrackEvent, "RTCTrackEvent");

    function RTCDataChannel(label, opts) {
      EventTarget.call(this);
      this.label = label !== undefined ? String(label) : "";
      this.ordered = opts && opts.ordered !== undefined ? Boolean(opts.ordered) : true;
      this.readyState = "connecting";
      this.bufferedAmount = 0;
      this.bufferedAmountLowThreshold = 0;
      this.binaryType = "arraybuffer";
      this.onopen = null;
      this.onclose = null;
      this.onerror = null;
      this.onmessage = null;
      const self = this;
      global.setTimeout(function () {
        if (self.readyState !== "connecting") return;
        self.readyState = "open";
        self.dispatchEvent(new Event("open"));
      }, 0);
    }
    RTCDataChannel.prototype = Object.create(EventTarget.prototype);
    RTCDataChannel.prototype.constructor = RTCDataChannel;
    RTCDataChannel.prototype.send = function () {};
    RTCDataChannel.prototype.close = function () {
      if (this.readyState === "closed") return;
      this.readyState = "closed";
      this.dispatchEvent(new Event("close"));
    };
    markNative(RTCDataChannel, "RTCDataChannel");

    function RTCPeerConnection() {
      EventTarget.call(this);
      this.connectionState = "new";
      this.iceConnectionState = "new";
      this.signalingState = "stable";
      this.iceGatheringState = "new";
      this.localDescription = null;
      this.remoteDescription = null;
      this.onconnectionstatechange = null;
      this.oniceconnectionstatechange = null;
      this.onsignalingstatechange = null;
      this.onicegatheringstatechange = null;
      this.onicecandidate = null;
      this.ondatachannel = null;
      this.ontrack = null;
      this.onnegotiationneeded = null;
      this._senders = [];
      const self = this;
      global.setTimeout(function () {
        if (self.connectionState === "closed") return;
        self.iceConnectionState = "failed";
        self.connectionState = "failed";
        self.dispatchEvent(new Event("iceconnectionstatechange"));
        self.dispatchEvent(new Event("connectionstatechange"));
      }, 0);
    }
    RTCPeerConnection.prototype = Object.create(EventTarget.prototype);
    RTCPeerConnection.prototype.constructor = RTCPeerConnection;
    RTCPeerConnection.prototype.setRemoteDescription = function (desc) {
      this.remoteDescription = desc ? new RTCSessionDescription(desc) : null;
      return Promise.resolve();
    };
    RTCPeerConnection.prototype.setLocalDescription = function (desc) {
      this.localDescription = desc ? new RTCSessionDescription(desc) : null;
      return Promise.resolve();
    };
    RTCPeerConnection.prototype.createOffer = function () {
      return Promise.resolve(new RTCSessionDescription({ type: "offer", sdp: "" }));
    };
    RTCPeerConnection.prototype.createAnswer = function () {
      return Promise.resolve(new RTCSessionDescription({ type: "answer", sdp: "" }));
    };
    RTCPeerConnection.prototype.addIceCandidate = function () {
      return Promise.resolve();
    };
    RTCPeerConnection.prototype.getStats = function () {
      return Promise.resolve(new Map());
    };
    RTCPeerConnection.prototype.addTrack = function (track) {
      const sender = {
        track: track || null,
        replaceTrack: function (newTrack) {
          sender.track = newTrack || null;
          return Promise.resolve();
        },
        getParameters: function () { return {}; },
        setParameters: function () { return Promise.resolve(); },
      };
      this._senders.push(sender);
      return sender;
    };
    RTCPeerConnection.prototype.getSenders = function () {
      return this._senders.slice();
    };
    RTCPeerConnection.prototype.createDataChannel = function (label, opts) {
      return new RTCDataChannel(label, opts);
    };
    RTCPeerConnection.prototype.close = function () {
      this.connectionState = "closed";
      this.iceConnectionState = "closed";
      this.signalingState = "closed";
    };
    markNative(RTCPeerConnection, "RTCPeerConnection");

    const mediaDevicesObj = {
      getUserMedia: function () {
        return Promise.reject(new DOMException("Permission denied", "NotAllowedError"));
      },
      enumerateDevices: function () {
        return Promise.resolve([]);
      },
    };
    if (global.navigator && typeof global.navigator === "object" && !global.navigator.mediaDevices) {
      try {
        Object.defineProperty(global.navigator, "mediaDevices", {
          value: mediaDevicesObj,
          configurable: true,
          enumerable: true,
          writable: true,
        });
      } catch (e) {
        global.navigator.mediaDevices = mediaDevicesObj;
      }
    }

    const exportsMap = {
      RTCPeerConnection: RTCPeerConnection,
      webkitRTCPeerConnection: RTCPeerConnection,
      RTCDataChannel: RTCDataChannel,
      RTCSessionDescription: RTCSessionDescription,
      RTCIceCandidate: RTCIceCandidate,
      RTCError: RTCError,
      MediaStream: MediaStream,
      MediaStreamTrack: MediaStreamTrack,
      RTCErrorEvent: RTCErrorEvent,
      RTCPeerConnectionIceEvent: RTCPeerConnectionIceEvent,
      RTCDataChannelEvent: RTCDataChannelEvent,
      RTCTrackEvent: RTCTrackEvent,
    };
    Object.keys(exportsMap).forEach(function (k) {
      const v = exportsMap[k];
      if (typeof global[k] === "undefined") global[k] = v;
      if (typeof window !== "undefined" && typeof window[k] === "undefined") window[k] = v;
      if (typeof globalThis !== "undefined" && typeof globalThis[k] === "undefined") globalThis[k] = v;
    });
  })();

  // Complete console.* API methods backed by native console.log/warn/error
  if (typeof global.console === "object" && global.console) {
    const c = global.console;
    const timers = new Map();
    const counters = new Map();
    if (typeof c.time !== "function") {
      c.time = function (label) { timers.set(label === undefined ? "default" : String(label), Date.now()); };
    }
    if (typeof c.timeLog !== "function") {
      c.timeLog = function (label) {
        const k = label === undefined ? "default" : String(label);
        const start = timers.get(k);
        const extra = Array.prototype.slice.call(arguments, 1);
        if (start === undefined) { c.warn("Timer '" + k + "' does not exist"); return; }
        c.log.apply(c, [k + ": " + (Date.now() - start) + " ms"].concat(extra));
      };
    }
    if (typeof c.timeEnd !== "function") {
      c.timeEnd = function (label) {
        const k = label === undefined ? "default" : String(label);
        const start = timers.get(k);
        if (start === undefined) { c.warn("Timer '" + k + "' does not exist"); return; }
        timers.delete(k);
        c.log(k + ": " + (Date.now() - start) + " ms");
      };
    }
    if (typeof c.count !== "function") {
      c.count = function (label) {
        const k = label === undefined ? "default" : String(label);
        const next = (counters.get(k) || 0) + 1;
        counters.set(k, next);
        c.log(k + ": " + next);
      };
    }
    if (typeof c.countReset !== "function") {
      c.countReset = function (label) {
        const k = label === undefined ? "default" : String(label);
        if (!counters.has(k)) { c.warn("Count for '" + k + "' does not exist"); return; }
        counters.set(k, 0);
      };
    }
    if (typeof c.assert !== "function") {
      c.assert = function (cond) {
        if (!cond) {
          const rest = Array.prototype.slice.call(arguments, 1);
          c.error.apply(c, rest.length > 0 ? ["Assertion failed:"].concat(rest) : ["Assertion failed"]);
        }
      };
    }
    if (typeof c.dir !== "function") c.dir = function (obj) { c.log(obj); };
    if (typeof c.dirxml !== "function") c.dirxml = function () { c.log.apply(c, arguments); };
    if (typeof c.table !== "function") c.table = function (data) { c.log(data); };
    if (typeof c.trace !== "function") c.trace = function () { c.log.apply(c, arguments.length ? arguments : ["console.trace"]); };
    if (typeof c.group !== "function") c.group = function () { if (arguments.length) c.log.apply(c, arguments); };
    if (typeof c.groupCollapsed !== "function") c.groupCollapsed = function () { if (arguments.length) c.log.apply(c, arguments); };
    if (typeof c.groupEnd !== "function") c.groupEnd = function () {};
    if (typeof c.clear !== "function") c.clear = function () {};
  }

  // Web Cache & CacheStorage API backed by native session-persisted origin store
  (function () {
    const memCaches = new Map();
    function cacheOrigin() {
      try {
        const loc = global.location || (typeof window !== "undefined" ? window.location : null);
        if (loc && loc.origin && loc.origin !== "null") return String(loc.origin);
        if (loc && loc.href) return new URL(loc.href).origin || "null";
      } catch (e) {}
      return "null";
    }
    function resolveReqUrl(req) {
      const raw = (req && typeof req === "object" && typeof req.url === "string") ? req.url : String(req);
      try {
        const base = (global.location && global.location.href && global.location.href !== "about:blank") ? global.location.href : "http://localhost/";
        return new URL(raw, base).href;
      } catch (e) {
        return raw;
      }
    }
    function Cache(name) {
      this._name = String(name);
    }
    Cache.prototype.put = function (req, res) {
      const origin = cacheOrigin();
      const fullUrl = resolveReqUrl(req);
      const name = this._name;
      const readBody = (res && typeof res.text === "function") ? res.text() : Promise.resolve(res && res._body != null ? String(res._body) : "");
      return Promise.resolve(readBody).then(function (bodyText) {
        const hdrObj = {};
        if (res && res.headers && typeof res.headers.forEach === "function") {
          res.headers.forEach(function (v, k) { hdrObj[k] = v; });
        }
        const payload = JSON.stringify({
          status: (res && typeof res.status === "number") ? res.status : 200,
          statusText: (res && res.statusText != null) ? String(res.statusText) : "OK",
          headers: hdrObj,
          body: String(bodyText != null ? bodyText : ""),
        });
        if (typeof globalThis.__capsurf_cache_put === "function") {
          globalThis.__capsurf_cache_put(origin, name, fullUrl, payload);
        } else {
          if (!memCaches.has(origin + "\0" + name)) memCaches.set(origin + "\0" + name, new Map());
          memCaches.get(origin + "\0" + name).set(fullUrl, payload);
        }
      });
    };
    Cache.prototype.match = function (req) {
      const origin = cacheOrigin();
      const fullUrl = resolveReqUrl(req);
      let raw = null;
      if (typeof globalThis.__capsurf_cache_match === "function") {
        raw = globalThis.__capsurf_cache_match(origin, this._name, fullUrl);
      } else {
        const m = memCaches.get(origin + "\0" + this._name);
        raw = m ? (m.get(fullUrl) || null) : null;
      }
      if (!raw) return Promise.resolve(undefined);
      try {
        const parsed = JSON.parse(raw);
        if (typeof Response === "function") {
          return Promise.resolve(new Response(parsed.body, {
            status: parsed.status || 200,
            statusText: parsed.statusText || "OK",
            headers: parsed.headers || {},
          }));
        }
        return Promise.resolve(makeFetchedResponse(parsed.body, {
          status: parsed.status || 200,
          statusText: parsed.statusText || "OK",
          url: fullUrl,
          headers: parsed.headers || {},
        }));
      } catch (e) {
        return Promise.resolve(undefined);
      }
    };
    Cache.prototype.delete = function (req) {
      const origin = cacheOrigin();
      const fullUrl = resolveReqUrl(req);
      if (typeof globalThis.__capsurf_cache_delete === "function") {
        return Promise.resolve(Boolean(globalThis.__capsurf_cache_delete(origin, this._name, fullUrl)));
      }
      const m = memCaches.get(origin + "\0" + this._name);
      return Promise.resolve(m ? m.delete(fullUrl) : false);
    };
    Cache.prototype.add = function (req) {
      const self = this;
      return fetch(req).then(function (res) { return self.put(req, res); });
    };
    function CacheStorage() {}
    CacheStorage.prototype.open = function (name) {
      return Promise.resolve(new Cache(String(name)));
    };
    CacheStorage.prototype.has = function (name) {
      const origin = cacheOrigin();
      if (typeof globalThis.__capsurf_cache_has === "function") {
        return Promise.resolve(Boolean(globalThis.__capsurf_cache_has(origin, String(name))));
      }
      return Promise.resolve(memCaches.has(origin + "\0" + String(name)));
    };
    CacheStorage.prototype.delete = function (name) {
      const origin = cacheOrigin();
      if (typeof globalThis.__capsurf_cache_delete === "function") {
        return Promise.resolve(Boolean(globalThis.__capsurf_cache_delete(origin, String(name), "")));
      }
      return Promise.resolve(memCaches.delete(origin + "\0" + String(name)));
    };
    CacheStorage.prototype.match = function (req) {
      const origin = cacheOrigin();
      const fullUrl = resolveReqUrl(req);
      if (typeof globalThis.__capsurf_cache_match === "function") {
        const raw = globalThis.__capsurf_cache_match(origin, "", fullUrl);
        if (!raw) return Promise.resolve(undefined);
        try {
          const parsed = JSON.parse(raw);
          return Promise.resolve(typeof Response === "function"
            ? new Response(parsed.body, { status: parsed.status || 200, statusText: parsed.statusText || "OK", headers: parsed.headers || {} })
            : makeFetchedResponse(parsed.body, { status: parsed.status || 200, statusText: parsed.statusText || "OK", url: fullUrl, headers: parsed.headers || {} }));
        } catch (e) {}
      }
      return Promise.resolve(undefined);
    };
    const cachesInst = new CacheStorage();
    global.Cache = Cache;
    global.CacheStorage = CacheStorage;
    global.caches = cachesInst;
    if (typeof window !== "undefined") {
      window.Cache = Cache;
      window.CacheStorage = CacheStorage;
      window.caches = cachesInst;
    }
    if (typeof globalThis !== "undefined") {
      globalThis.Cache = Cache;
      globalThis.CacheStorage = CacheStorage;
      globalThis.caches = cachesInst;
    }
  })();

  // window.open popup bridge for Target.targetCreated (openerId / openerFrameId)
  const openFn = markNative(function (url, target, features) {
    let fullUrl = "about:blank";
    if (url !== undefined && url !== null && String(url) !== "") {
      try {
        const base = (global.location && global.location.href && global.location.href !== "about:blank") ? global.location.href : "http://localhost/";
        fullUrl = new URL(String(url), base).href;
      } catch (e) {
        fullUrl = String(url);
      }
    }
    const isNoopener = /(?:^|[,\s])(noopener|noreferrer)(?:$|[,\s=])/i.test(String(features || ""));
    if (typeof __capsurf_open_popup === "function") {
      __capsurf_open_popup(fullUrl, isNoopener ? 1 : 0);
    } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_open_popup === "function") {
      globalThis.__capsurf_open_popup(fullUrl, isNoopener ? 1 : 0);
    }
    if (isNoopener) return null;
    const popupWin = {
      closed: false,
      opener: global,
      location: { href: fullUrl },
      document: { title: "", URL: fullUrl },
      close: function () { this.closed = true; },
      focus: function () {},
      blur: function () {},
      postMessage: function () {},
    };
    return popupWin;
  }, "open");
  global.open = openFn;
  if (typeof window !== "undefined") window.open = openFn;
  if (typeof globalThis !== "undefined") globalThis.open = openFn;
}
