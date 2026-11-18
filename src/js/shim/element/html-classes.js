// src/js/shim/element/html-classes.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/html-classes.ts

function installHtmlClasses(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};
  const nativeGetAttribute = ElementProto.getAttribute || function (name) { return this.getAttribute(name); };
  const nativeSetAttribute = ElementProto.setAttribute || function (name, val) { this.setAttribute(name, val); };
  const nativeRemoveAttribute = ElementProto.removeAttribute || function (name) { this.removeAttribute(name); };
  const nativeHasAttribute = ElementProto.hasAttribute || function (name) { return this.hasAttribute(name); };

  const forceAsyncScripts = new WeakSet();
  function markScriptForceAsync(el) {
    if (el && typeof el === "object") forceAsyncScripts.add(el);
  }
  function clearScriptForceAsync(el) {
    if (el && typeof el === "object") forceAsyncScripts.delete(el);
  }
  function isScriptForceAsync(el) {
    return (el && typeof el === "object") ? forceAsyncScripts.has(el) : false;
  }
  global.__capsurf_markScriptForceAsync = markScriptForceAsync;
  global.__capsurf_clearScriptForceAsync = clearScriptForceAsync;
  global.__capsurf_isScriptForceAsync = isScriptForceAsync;

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

  // 1.05 HTMLStyleElement
  function HTMLStyleElement() {}
  HTMLStyleElement.prototype = Object.create(ElementProto);
  HTMLStyleElement.prototype.constructor = HTMLStyleElement;

  function getSheetForElement(el) {
    if (!el._sheet) {
      if (typeof globalThis.getOrCreateStyleSheet === "function") {
        el._sheet = globalThis.getOrCreateStyleSheet(el);
      } else if (typeof global.getOrCreateStyleSheet === "function") {
        el._sheet = global.getOrCreateStyleSheet(el);
      } else if (typeof global.CSSStyleSheet === "function") {
        el._sheet = new global.CSSStyleSheet(el);
      } else if (typeof globalThis.CSSStyleSheet === "function") {
        el._sheet = new globalThis.CSSStyleSheet(el);
      }
    }
    return el._sheet;
  }

  Object.defineProperty(HTMLStyleElement.prototype, "sheet", {
    get: function () {
      return getSheetForElement(this);
    },
    configurable: true,
  });
  markNative(HTMLStyleElement, "HTMLStyleElement");
  global.HTMLStyleElement = HTMLStyleElement;
  if (typeof globalThis !== "undefined") globalThis.HTMLStyleElement = HTMLStyleElement;
  if (typeof window !== "undefined") window.HTMLStyleElement = HTMLStyleElement;

  // Also define sheet on ElementProto for any <style> element
  if (!Object.getOwnPropertyDescriptor(ElementProto, "sheet")) {
    Object.defineProperty(ElementProto, "sheet", {
      get: function () {
        const tag = String(this.localName || this.tagName || "").toLowerCase();
        if (tag === "style") {
          return getSheetForElement(this);
        }
        return undefined;
      },
      configurable: true,
    });
  }

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
    get: function () {
      const raw = nativeGetAttribute.call(this, "src");
      if (raw == null) return "";
      try {
        const base = (typeof document !== "undefined" && document.baseURI) ? document.baseURI : (global.location ? global.location.href : "http://localhost/");
        return new URL(raw, base).href;
      } catch (e) {
        return raw;
      }
    },
    set: function (v) { nativeSetAttribute.call(this, "src", String(v)); },
    configurable: true,
    enumerable: true,
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
  function HTMLScriptElement() {
    markScriptForceAsync(this);
  }
  HTMLScriptElement.prototype = Object.create(ElementProto);
  HTMLScriptElement.prototype.constructor = HTMLScriptElement;

  Object.defineProperty(HTMLScriptElement.prototype, "text", {
    get: function () {
      return this.textContent || "";
    },
    set: function (v) {
      this.textContent = String(v);
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLScriptElement.prototype, "src", {
    get: function () {
      const raw = nativeGetAttribute.call(this, "src");
      if (raw == null) return "";
      try {
        const base = (typeof document !== "undefined" && document.baseURI) ? document.baseURI : (global.location ? global.location.href : "http://localhost/");
        return new URL(raw, base).href;
      } catch (e) {
        return raw;
      }
    },
    set: function (v) { nativeSetAttribute.call(this, "src", String(v)); },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLScriptElement.prototype, "type", {
    get: function () {
      const v = nativeGetAttribute.call(this, "type");
      return v == null ? "" : v;
    },
    set: function (v) { nativeSetAttribute.call(this, "type", String(v)); },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLScriptElement.prototype, "async", {
    get: function () {
      return isScriptForceAsync(this) || nativeHasAttribute.call(this, "async");
    },
    set: function (v) {
      clearScriptForceAsync(this);
      if (v) nativeSetAttribute.call(this, "async", "");
      else nativeRemoveAttribute.call(this, "async");
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLScriptElement.prototype, "defer", {
    get: function () { return nativeHasAttribute.call(this, "defer"); },
    set: function (v) { if (v) nativeSetAttribute.call(this, "defer", ""); else nativeRemoveAttribute.call(this, "defer"); },
    configurable: true,
    enumerable: true,
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
  function locationInputString(value) {
    if (typeof value === "symbol") {
      throw new TypeError("Cannot convert a Symbol value to a string");
    }
    return String(value);
  }

  const FACADE_LOCATION_PARTS = [
    "protocol",
    "host",
    "hostname",
    "port",
    "pathname",
    "search",
    "hash",
  ];

  function makeIframeLocationFacade(iframeEl, initialUrl, contentWindow) {
    let current = initialUrl || "about:blank";

    function resolveAndNavigate(v) {
      const inputStr = locationInputString(v);
      let resolved = "";
      try {
        const base = (typeof document !== "undefined" && document.baseURI)
          ? document.baseURI
          : (global.location ? global.location.href : "about:blank");
        resolved = new URL(inputStr, base).toString();
      } catch (e) {
        const FrameDOMException = (contentWindow && contentWindow.DOMException) ||
          (typeof global !== "undefined" && global.DOMException) ||
          (typeof globalThis !== "undefined" && globalThis.DOMException) ||
          DOMException;
        const error = new FrameDOMException("Invalid URL", "SyntaxError");
        if (typeof FrameDOMException === "function") {
          try {
            Object.defineProperty(error, "constructor", { value: FrameDOMException });
          } catch (_) {}
        }
        throw error;
      }
      current = resolved;
      try {
        handleIframeNavigation(iframeEl, resolved);
      } catch (e) {}
    }

    const loc = {
      get href() {
        return current;
      },
      set href(v) {
        resolveAndNavigate(v);
      },
      get origin() {
        try {
          return new URL(current).origin;
        } catch (e) {
          return "null";
        }
      },
      toString: function () {
        return current;
      },
      valueOf: function () {
        return current;
      },
      assign: function (v) {
        resolveAndNavigate(v);
      },
      replace: function (v) {
        resolveAndNavigate(v);
      },
      reload: function () {
        resolveAndNavigate(current);
      },
    };

    for (let i = 0; i < FACADE_LOCATION_PARTS.length; i++) {
      (function (part) {
        Object.defineProperty(loc, part, {
          enumerable: true,
          configurable: true,
          get: function () {
            try {
              return new URL(current)[part];
            } catch (e) {
              return "";
            }
          },
          set: function (v) {
            try {
              const next = new URL(current);
              next[part] = String(v);
              loc.href = next.toString();
            } catch (e) {}
          },
        });
      })(FACADE_LOCATION_PARTS[i]);
    }

    loc[Symbol.toPrimitive] = function () {
      return current;
    };

    return loc;
  }

  const iframeLocationMap = new WeakMap();

  function ensureIframeLocationFacade(contentWindow, iframeEl, url) {
    let loc = iframeLocationMap.get(contentWindow);
    if (!loc) {
      loc = makeIframeLocationFacade(iframeEl, url, contentWindow);
      iframeLocationMap.set(contentWindow, loc);
      Object.defineProperty(contentWindow, "location", {
        enumerable: true,
        configurable: false,
        get: function () {
          return iframeLocationMap.get(contentWindow);
        },
        set: function (value) {
          const cur = iframeLocationMap.get(contentWindow);
          if (cur) {
            cur.href = locationInputString(value);
          }
        },
      });
    }
    return loc;
  }

  const FRAME_WINDOW_GLOBALS = [
    "DOMException", "Error", "TypeError", "RangeError", "SyntaxError",
    "ReferenceError", "URIError", "EvalError",
    "Array", "Object", "String", "Number", "Boolean", "Symbol",
    "Promise", "Function", "Date", "RegExp", "Map", "Set", "WeakMap", "WeakSet",
    "JSON", "Math", "Node", "Element", "Document", "DocumentFragment",
    "Event", "MessageEvent", "CustomEvent", "NodeList", "HTMLCollection",
    "DOMParser", "Range", "StaticRange", "URL", "URLSearchParams",
    "Blob", "FormData", "Headers", "Request", "Response",
    "AbortController", "AbortSignal", "HTMLElement", "HTMLIFrameElement",
    "setTimeout", "clearTimeout", "setInterval", "clearInterval",
    "requestAnimationFrame", "cancelAnimationFrame", "console", "fetch"
  ];

  function decorateIframeWindow(win) {
    const pageGlobal = typeof window !== "undefined" ? window : globalThis;
    for (let i = 0; i < FRAME_WINDOW_GLOBALS.length; i++) {
      const key = FRAME_WINDOW_GLOBALS[i];
      if (win[key] === undefined && pageGlobal[key] !== undefined) {
        win[key] = pageGlobal[key];
      }
    }
    if (pageGlobal.fetch) {
      win.fetch = function (resource, init) {
        return pageGlobal.fetch.call(win, resource, init);
      };
    }
  }

  function createIframePlaceholderDoc(docUrl) {
    let doc = null;
    try {
      const Ctor = global.DOMParser || (typeof DOMParser !== "undefined" ? DOMParser : null);
      if (Ctor) {
        const parser = new Ctor();
        doc = parser.parseFromString("<html><head></head><body></body></html>", "text/html");
        if (doc) {
          doc._mutable = true;
          try {
            Object.defineProperty(doc, "_baseUrl", {
              value: docUrl || "about:blank",
              configurable: true,
              writable: true,
            });
          } catch (e) {}
        }
      }
    } catch (e) {}
    return doc;
  }

  function handleIframeNavigation(iframeEl, resolvedUrl) {
    if (!iframeEl) return;

    if (resolvedUrl === "about:blank" || !resolvedUrl) {
      const doc = createIframePlaceholderDoc("about:blank");
      if (iframeEl._contentWindow) {
        iframeEl._contentWindow.document = doc;
        if (doc) {
          try {
            Object.defineProperty(doc, "defaultView", {
              value: iframeEl._contentWindow,
              configurable: true,
            });
          } catch (e) {}
        }
      }
      iframeEl._contentDocument = doc;
      Promise.resolve().then(function () {
        fireIframeLoadEvent(iframeEl);
      });
      return;
    }

    let loadPromise = null;
    if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_native_load_fixture === "function") {
      const fixtureHtml = globalThis.__capsurf_native_load_fixture(resolvedUrl);
      if (fixtureHtml !== null && fixtureHtml !== undefined) {
        loadPromise = Promise.resolve(fixtureHtml);
      }
    }

    if (!loadPromise) {
      if (typeof fetch === "function") {
        loadPromise = fetch(resolvedUrl).then(function (res) {
          return res.text();
        });
      } else {
        loadPromise = Promise.resolve("<!doctype html><html><head></head><body></body></html>");
      }
    }

    loadPromise.then(function (html) {
      let newDoc = null;
      try {
        const Ctor = global.DOMParser || (typeof DOMParser !== "undefined" ? DOMParser : null);
        if (Ctor) {
          const parser = new Ctor();
          newDoc = parser.parseFromString(html, "text/html");
          if (newDoc) {
            newDoc._mutable = true;
            try {
              Object.defineProperty(newDoc, "_baseUrl", {
                value: resolvedUrl,
                configurable: true,
                writable: true,
              });
            } catch (e) {}
          }
        }
      } catch (e) {}

      if (newDoc) {
        if (iframeEl._contentWindow) {
          iframeEl._contentWindow.document = newDoc;
          try {
            Object.defineProperty(newDoc, "defaultView", {
              value: iframeEl._contentWindow,
              configurable: true,
            });
          } catch (e) {}
        }
        iframeEl._contentDocument = newDoc;
      }

      fireIframeLoadEvent(iframeEl);
    }).catch(function () {
      fireIframeLoadEvent(iframeEl);
    });
  }

  function fireIframeLoadEvent(iframeEl) {
    try {
      const evt = new Event("load", { bubbles: false, cancelable: false });
      if (typeof iframeEl.onload === "function") {
        try {
          iframeEl.onload.call(iframeEl, evt);
        } catch (e) {
          if (typeof console !== "undefined" && console.error) console.error(e);
        }
      }
      if (typeof iframeEl.dispatchEvent === "function") {
        try {
          iframeEl.dispatchEvent(evt);
        } catch (e) {}
      }
    } catch (e) {}
  }

  function getOrCreateIframeWindow(iframeEl) {
    if (iframeEl._contentWindow) return iframeEl._contentWindow;

    let initialUrl = "";
    try {
      initialUrl = (iframeEl.getAttribute && iframeEl.getAttribute("src")) || "";
    } catch (e) {}
    if (!initialUrl || initialUrl === "about:blank") {
      initialUrl = "about:blank";
    } else {
      try {
        const base = (typeof document !== "undefined" && document.baseURI)
          ? document.baseURI
          : (global.location ? global.location.href : "about:blank");
        initialUrl = new URL(initialUrl, base).href;
      } catch (e) {
        initialUrl = "about:blank";
      }
    }

    const initialDoc = createIframePlaceholderDoc(initialUrl);

    const contentWindow = {
      document: initialDoc,
      parent: global.window || global,
      top: (global.window && global.window.top) || global,
      frames: [],
      frameElement: iframeEl,
      name: (function () {
        try {
          return String(iframeEl.getAttribute ? (iframeEl.getAttribute("name") || "") : "");
        } catch (e) {
          return "";
        }
      })(),
      addEventListener: function () {},
      removeEventListener: function () {},
      dispatchEvent: function () { return true; },
    };

    contentWindow.self = contentWindow;
    contentWindow.window = contentWindow;

    decorateIframeWindow(contentWindow);
    ensureIframeLocationFacade(contentWindow, iframeEl, initialUrl);

    if (initialDoc) {
      try {
        Object.defineProperty(initialDoc, "defaultView", {
          value: contentWindow,
          configurable: true,
        });
      } catch (e) {}
    }

    contentWindow.postMessage = function () {};

    iframeEl._contentWindow = contentWindow;
    iframeEl._contentDocument = initialDoc;

    if (initialUrl && initialUrl !== "about:blank") {
      handleIframeNavigation(iframeEl, initialUrl);
    }

    return contentWindow;
  }

  function HTMLIFrameElement() {}
  HTMLIFrameElement.prototype = Object.create(ElementProto);
  HTMLIFrameElement.prototype.constructor = HTMLIFrameElement;

  Object.defineProperty(HTMLIFrameElement.prototype, "src", {
    get: function () {
      const raw = nativeGetAttribute.call(this, "src");
      if (raw == null) return "";
      try {
        const base = (typeof document !== "undefined" && document.baseURI) ? document.baseURI : (global.location ? global.location.href : "http://localhost/");
        return new URL(raw, base).href;
      } catch (e) {
        return raw;
      }
    },
    set: function (v) {
      const strVal = String(v);
      nativeSetAttribute.call(this, "src", strVal);
      if (this._contentWindow) {
        const base = (typeof document !== "undefined" && document.baseURI) ? document.baseURI : (global.location ? global.location.href : "about:blank");
        let resolved = "about:blank";
        try {
          resolved = new URL(strVal, base).href;
        } catch (e) {
          resolved = strVal;
        }
        handleIframeNavigation(this, resolved);
      }
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLIFrameElement.prototype, "contentWindow", {
    get: function () {
      return getOrCreateIframeWindow(this);
    },
    configurable: true,
  });

  Object.defineProperty(HTMLIFrameElement.prototype, "contentDocument", {
    get: function () {
      const win = getOrCreateIframeWindow(this);
      return (win && win.document) || this._contentDocument || null;
    },
    configurable: true,
  });

  markNative(HTMLIFrameElement, "HTMLIFrameElement");
  global.HTMLIFrameElement = HTMLIFrameElement;

  // 6. Generic HTMLElement
  const upgradeTargets = [];
  function HTMLElement() {
    if (upgradeTargets.length > 0) {
      return upgradeTargets.pop();
    }
  }
  HTMLElement.prototype = Object.create(ElementProto);
  HTMLElement.prototype.constructor = HTMLElement;
  Object.defineProperty(HTMLElement, "_upgradeTarget", {
    get: function () {
      return upgradeTargets.length > 0 ? upgradeTargets[upgradeTargets.length - 1] : null;
    },
    set: function (val) {
      if (val === null || val === undefined) {
        if (upgradeTargets.length > 0) upgradeTargets.pop();
      } else {
        upgradeTargets.push(val);
      }
    },
    configurable: true,
  });
  markNative(HTMLElement, "HTMLElement");
  global.HTMLElement = HTMLElement;

  // 7. HTMLTemplateElement
  function HTMLTemplateElement() {
    HTMLElement.call(this);
  }
  HTMLTemplateElement.prototype = Object.create(HTMLElement.prototype);
  HTMLTemplateElement.prototype.constructor = HTMLTemplateElement;

  Object.defineProperty(HTMLTemplateElement.prototype, "content", {
    get: function () {
      if (!this._templateContent) {
        this._templateContent = initTemplateContent(this);
      }
      return this._templateContent;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLTemplateElement.prototype, "innerHTML", {
    get: function () {
      return this.content ? this.content.innerHTML || "" : "";
    },
    set: function (val) {
      const content = this.content;
      if (content) {
        while (content.firstChild) {
          content.removeChild(content.firstChild);
        }
        const doc = this.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
        const temp = doc ? doc.createElement("div") : null;
        if (temp) {
          temp.innerHTML = val;
          while (temp.firstChild) {
            content.appendChild(temp.firstChild);
          }
        }
      }
    },
    configurable: true,
  });

  markNative(HTMLTemplateElement, "HTMLTemplateElement");
  global.HTMLTemplateElement = HTMLTemplateElement;
  if (typeof globalThis !== "undefined") {
    globalThis.HTMLTemplateElement = HTMLTemplateElement;
  }

  // 8. SVG Interfaces & AnimatedString
  function SVGAnimatedString() {
    throw new TypeError("Illegal constructor");
  }
  Object.defineProperty(SVGAnimatedString.prototype, "baseVal", {
    get: function () {
      if (this._element && typeof this._element.getAttribute === "function") {
        return this._element.getAttribute("class") || "";
      }
      return this._baseVal || "";
    },
    set: function (v) {
      const str = String(v);
      if (this._element && typeof this._element.setAttribute === "function") {
        this._element.setAttribute("class", str);
      } else {
        this._baseVal = str;
      }
    },
    configurable: true,
    enumerable: true,
  });
  Object.defineProperty(SVGAnimatedString.prototype, "animVal", {
    get: function () {
      return this.baseVal;
    },
    configurable: true,
    enumerable: true,
  });
  SVGAnimatedString.prototype.toString = function () {
    return this.baseVal;
  };
  markNative(SVGAnimatedString, "SVGAnimatedString");
  global.SVGAnimatedString = SVGAnimatedString;
  if (typeof globalThis !== "undefined") {
    globalThis.SVGAnimatedString = SVGAnimatedString;
  }

  function SVGElement() {}
  SVGElement.prototype = Object.create(ElementProto);
  SVGElement.prototype.constructor = SVGElement;
  Object.defineProperty(SVGElement.prototype, "className", {
    get: function () {
      if (!this._svgClassName) {
        const animStr = Object.create(SVGAnimatedString.prototype);
        animStr._element = this;
        this._svgClassName = animStr;
      }
      return this._svgClassName;
    },
    set: function (val) {
      if (typeof val === "string") {
        this.setAttribute("class", val);
      }
    },
    configurable: true,
    enumerable: true,
  });
  markNative(SVGElement, "SVGElement");
  global.SVGElement = SVGElement;
  if (typeof globalThis !== "undefined") {
    globalThis.SVGElement = SVGElement;
  }

  function SVGSVGElement() {
    SVGElement.call(this);
  }
  SVGSVGElement.prototype = Object.create(SVGElement.prototype);
  SVGSVGElement.prototype.constructor = SVGSVGElement;
  markNative(SVGSVGElement, "SVGSVGElement");
  global.SVGSVGElement = SVGSVGElement;
  if (typeof globalThis !== "undefined") {
    globalThis.SVGSVGElement = SVGSVGElement;
  }

  const SVG_VIEWBOX_LOCALS = new Set(["svg", "symbol", "marker", "pattern", "view"]);
  Object.defineProperty(ElementProto, "viewBox", {
    configurable: true,
    get: function () {
      const tag = String(this.localName || this.tagName || "").toLowerCase();
      if (!SVG_VIEWBOX_LOCALS.has(tag)) {
        return undefined;
      }
      const el = this;
      const parse = function () {
        const raw = String(el.getAttribute("viewBox") ?? "");
        const n = raw
          .split(/[\s,]+/)
          .filter(Boolean)
          .map(Number);
        return n.length === 4 && n.every(Number.isFinite) ? n : [0, 0, 0, 0];
      };
      const write = function (i, v) {
        const n = parse();
        n[i] = Number(v) || 0;
        el.setAttribute("viewBox", n.join(" "));
      };
      const mkRect = function () {
        return {
          get x() { return parse()[0]; },
          set x(v) { write(0, v); },
          get y() { return parse()[1]; },
          set y(v) { write(1, v); },
          get width() { return parse()[2]; },
          set width(v) { write(2, v); },
          get height() { return parse()[3]; },
          set height(v) { write(3, v); },
        };
      };
      return { baseVal: mkRect(), animVal: mkRect() };
    },
  });
}

function initTemplateContent(template) {
  if (template._templateContent) return template._templateContent;
  const doc = template.ownerDocument || (typeof document !== "undefined" ? document : null);
  const frag = (doc && typeof doc.createDocumentFragment === "function")
    ? doc.createDocumentFragment()
    : new DocumentFragment();
  template._templateContent = frag;

  // Move existing JS _children if any
  if (template._children && template._children.length > 0) {
    const kids = template._children.slice();
    template._children.length = 0;
    for (let i = 0; i < kids.length; i++) {
      if (kids[i]) frag.appendChild(typeof upgradeElement === "function" ? upgradeElement(kids[i]) : kids[i]);
    }
  }

  // Move native CP children if any
  if (typeof nativeElementFirstChild === "function") {
    let c = null;
    try { c = nativeElementFirstChild.call(template); } catch (e) {}
    const nativeKids = [];
    while (c) {
      nativeKids.push(c);
      let next = null;
      if (typeof nativeElementNextSibling === "function") {
        try { next = nativeElementNextSibling.call(c); } catch (e) {}
      }
      c = next;
    }
    for (let i = 0; i < nativeKids.length; i++) {
      const k = nativeKids[i];
      try {
        if (typeof nativeElementRemoveChild === "function") {
          nativeElementRemoveChild.call(template, k);
        }
      } catch (e) {}
      const upgraded = typeof upgradeElement === "function" ? upgradeElement(k) : k;
      upgraded._ownerFragment = frag;
      frag.appendChild(upgraded);
    }
  }

  template._children = [];
  return frag;
}

function processSubtreeTemplates(root) {
  if (!root) return;
  let tpls = [];
  try {
    if (typeof root.querySelectorAll === "function") {
      const q = root.querySelectorAll("template");
      if (q && q.length) {
        for (let i = 0; i < q.length; i++) tpls.push(q[i]);
      }
    }
  } catch (e) {}

  const rootTag = String(root.localName || root.tagName || "").toLowerCase();
  if (rootTag === "template") {
    tpls.unshift(root);
  }

  for (let i = 0; i < tpls.length; i++) {
    const tpl = typeof upgradeElement === "function" ? upgradeElement(tpls[i]) : tpls[i];
    if (!tpl) continue;
    const content = tpl.content;

    // Check declarative shadow DOM
    const shadowMode = tpl.getAttribute && tpl.getAttribute("shadowrootmode");
    if (shadowMode) {
      const mode = String(shadowMode).toLowerCase();
      if (mode === "open" || mode === "closed") {
        const host = tpl.parentNode || tpl.parentElement;
        if (host && host.nodeType === 1) { // ELEMENT_NODE
          let shadow = null;
          try {
            shadow = host.attachShadow({
              mode: mode,
              delegatesFocus: Boolean(tpl.hasAttribute && tpl.hasAttribute("shadowrootdelegatesfocus")),
            });
          } catch (e) {
            shadow = host.shadowRoot || host._shadowRoot;
          }
          if (shadow && content) {
            while (content.firstChild) {
              const ch = content.firstChild;
              try { content.removeChild(ch); } catch (e) {}
              shadow.appendChild(ch);
            }
          }
          try {
            if (typeof tpl.remove === "function") {
              tpl.remove();
            } else if (host.removeChild) {
              host.removeChild(tpl);
            }
          } catch (e) {}
        }
      }
    }
  }
}
