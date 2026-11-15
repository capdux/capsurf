// src/js/shim/element/html-classes.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/html-classes.ts

function installHtmlClasses(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};

  // 1. HTMLAnchorElement
  function HTMLAnchorElement() {}
  HTMLAnchorElement.prototype = Object.create(ElementProto);
  HTMLAnchorElement.prototype.constructor = HTMLAnchorElement;

  Object.defineProperty(HTMLAnchorElement.prototype, "href", {
    get: function () {
      const raw = this.getAttribute("href") || "";
      if (!raw) return "";
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

  const anchorUrlProps = ["origin", "protocol", "host", "hostname", "port", "pathname", "search", "hash"];
  for (let i = 0; i < anchorUrlProps.length; i++) {
    const prop = anchorUrlProps[i];
    Object.defineProperty(HTMLAnchorElement.prototype, prop, {
      get: function () {
        try {
          const u = new URL(this.href);
          return u[prop];
        } catch (e) {
          return "";
        }
      },
      set: function (v) {
        try {
          const u = new URL(this.href);
          u[prop] = v;
          this.href = u.href;
        } catch (e) {}
      },
      configurable: true,
    });
  }

  Object.defineProperty(HTMLAnchorElement.prototype, "target", {
    get: function () { return this.getAttribute("target") || ""; },
    set: function (v) { this.setAttribute("target", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLAnchorElement.prototype, "rel", {
    get: function () { return this.getAttribute("rel") || ""; },
    set: function (v) { this.setAttribute("rel", String(v)); },
    configurable: true,
  });

  markNative(HTMLAnchorElement, "HTMLAnchorElement");
  global.HTMLAnchorElement = HTMLAnchorElement;

  // 2. HTMLImageElement
  function HTMLImageElement() {}
  HTMLImageElement.prototype = Object.create(ElementProto);
  HTMLImageElement.prototype.constructor = HTMLImageElement;

  Object.defineProperty(HTMLImageElement.prototype, "src", {
    get: function () { return this.getAttribute("src") || ""; },
    set: function (v) { this.setAttribute("src", String(v)); },
    configurable: true,
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
  function HTMLScriptElement() {}
  HTMLScriptElement.prototype = Object.create(ElementProto);
  HTMLScriptElement.prototype.constructor = HTMLScriptElement;

  Object.defineProperty(HTMLScriptElement.prototype, "src", {
    get: function () { return this.getAttribute("src") || ""; },
    set: function (v) { this.setAttribute("src", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLScriptElement.prototype, "type", {
    get: function () { return this.getAttribute("type") || ""; },
    set: function (v) { this.setAttribute("type", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLScriptElement.prototype, "async", {
    get: function () { return this.hasAttribute("async"); },
    set: function (v) { if (v) this.setAttribute("async", ""); else this.removeAttribute("async"); },
    configurable: true,
  });

  Object.defineProperty(HTMLScriptElement.prototype, "defer", {
    get: function () { return this.hasAttribute("defer"); },
    set: function (v) { if (v) this.setAttribute("defer", ""); else this.removeAttribute("defer"); },
    configurable: true,
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
  function HTMLIFrameElement() {}
  HTMLIFrameElement.prototype = Object.create(ElementProto);
  HTMLIFrameElement.prototype.constructor = HTMLIFrameElement;

  Object.defineProperty(HTMLIFrameElement.prototype, "src", {
    get: function () { return this.getAttribute("src") || ""; },
    set: function (v) { this.setAttribute("src", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLIFrameElement.prototype, "contentWindow", {
    get: function () { return null; },
    configurable: true,
  });

  Object.defineProperty(HTMLIFrameElement.prototype, "contentDocument", {
    get: function () { return null; },
    configurable: true,
  });

  markNative(HTMLIFrameElement, "HTMLIFrameElement");
  global.HTMLIFrameElement = HTMLIFrameElement;

  // 6. Generic HTMLElement
  function HTMLElement() {}
  HTMLElement.prototype = Object.create(ElementProto);
  HTMLElement.prototype.constructor = HTMLElement;
  markNative(HTMLElement, "HTMLElement");
  global.HTMLElement = HTMLElement;
}
