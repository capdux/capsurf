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
    set: function (v) { nativeSetAttribute.call(this, "src", String(v)); },
    configurable: true,
    enumerable: true,
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
