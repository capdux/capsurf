// src/js/shim/index.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/index.ts
// Entry point for CapSurf Web API Runtime Shim.

function installShim(global) {
  // 1. Expose Event classes
  global.Event = Event;
  global.CustomEvent = CustomEvent;
  global.UIEvent = UIEvent;
  global.MouseEvent = MouseEvent;
  global.KeyboardEvent = KeyboardEvent;
  global.FocusEvent = FocusEvent;
  global.InputEvent = InputEvent;
  global.EventTarget = EventTarget;

  // 2. Expose DOM Collections and Nodes
  global.DOMTokenList = DOMTokenList;
  global.HTMLCollection = HTMLCollection;
  global.NodeList = NodeList;
  global.NamedNodeMap = NamedNodeMap;
  global.Attr = Attr;
  global.Node = Node;
  global.Text = Text;
  global.Comment = Comment;
  global.DocumentFragment = DocumentFragment;

  // 3. Expose Mutation Observer
  global.MutationObserver = MutationObserver;
  global.MutationRecord = MutationRecord;

  // 4. Expose FormData
  global.FormData = FormData;

  // 5. Expose Traversal
  global.NodeFilter = NodeFilter;
  global.TreeWalker = TreeWalker;
  global.NodeIterator = NodeIterator;

  // 6. Expose Range and Selection
  global.Range = Range;
  global.Selection = Selection;

  // 7. Expose DOMParser
  global.DOMParser = DOMParser;

  // 8. Expose Canvas 2D
  global.CanvasRenderingContext2D = CanvasRenderingContext2D;
  global.CanvasGradient = CanvasGradient;
  global.CanvasPattern = CanvasPattern;
  global.Path2D = Path2D;
  global.ImageData = ImageData;
  global.TextMetrics = TextMetrics;

  // 9. Expose Web Animations
  global.Animation = Animation;
  global.KeyframeEffect = KeyframeEffect;
  global.AnimationTimeline = AnimationTimeline;
  global.DocumentTimeline = DocumentTimeline;

  // 10. Install Element & HTML extensions
  if (global.Element) {
    installElementExtensions(global.Element.prototype);
    installHtmlElementExtensions(global.Element.prototype);

    global.Element.prototype.animate = markNative(function (keyframes, options) {
      return animateElement(this, keyframes, options);
    }, "animate");

    global.Element.prototype.getAnimations = markNative(function () {
      return animationsFor(this);
    }, "getAnimations");

    global.Element.prototype.scrollIntoView = markNative(function () {}, "scrollIntoView");

    global.Element.prototype.getContext = markNative(function (type) {
      const t = String(type).toLowerCase();
      if (t === "webgl" || t === "webgl2" || t === "experimental-webgl") {
        return null;
      }
      if (t === "2d") {
        if (!this._ctx2d) {
          this._ctx2d = new CanvasRenderingContext2D(this);
        }
        return this._ctx2d;
      }
      return null;
    }, "getContext");

    global.Element.prototype.toDataURL = markNative(function () {
      return VALID_PNG_BASE64;
    }, "toDataURL");
  }

  // 11. Install specialized elements and forms
  installHtmlClasses(global);
  installInputElement(global);
  installTextAreaElement(global);
  installSelectElement(global);
  installButtonElement(global);
  installFormElement(global);

  // 12. Install window environment
  installWindow(global);

  // 13. Enhance Document prototype and instance
  if (global.Document && global.Document.prototype) {
    const docProto = global.Document.prototype;

    // Inherit from Node
    Object.setPrototypeOf(docProto, Node.prototype);

    if (docProto.createElement) {
      const origCreateElement = docProto.createElement;
      docProto.createElement = function (tagName) {
        const el = origCreateElement.call(this, tagName);
        if (!el) return el;
        const tag = String(tagName).toLowerCase();
        switch (tag) {
          case "input":
            Object.setPrototypeOf(el, global.HTMLInputElement.prototype);
            break;
          case "textarea":
            Object.setPrototypeOf(el, global.HTMLTextAreaElement.prototype);
            break;
          case "select":
            Object.setPrototypeOf(el, global.HTMLSelectElement.prototype);
            break;
          case "option":
            Object.setPrototypeOf(el, global.HTMLOptionElement.prototype);
            break;
          case "button":
            Object.setPrototypeOf(el, global.HTMLButtonElement.prototype);
            break;
          case "form":
            Object.setPrototypeOf(el, global.HTMLFormElement.prototype);
            break;
          case "a":
            Object.setPrototypeOf(el, global.HTMLAnchorElement.prototype);
            break;
          case "img":
            Object.setPrototypeOf(el, global.HTMLImageElement.prototype);
            break;
          case "canvas":
            Object.setPrototypeOf(el, global.HTMLCanvasElement.prototype);
            break;
          case "script":
            Object.setPrototypeOf(el, global.HTMLScriptElement.prototype);
            break;
          case "iframe":
            Object.setPrototypeOf(el, global.HTMLIFrameElement.prototype);
            break;
          default:
            Object.setPrototypeOf(el, global.HTMLElement.prototype);
            break;
        }
        return el;
      };
      markNative(docProto.createElement, "createElement");
    }

    docProto.createRange = markNative(function () {
      return new Range();
    }, "createRange");

    docProto.createTreeWalker = markNative(function (root, whatToShow, filter) {
      return new TreeWalker(root, whatToShow, filter);
    }, "createTreeWalker");

    docProto.createNodeIterator = markNative(function (root, whatToShow, filter) {
      return new NodeIterator(root, whatToShow, filter);
    }, "createNodeIterator");

    docProto.getSelection = markNative(function () {
      if (!this._selection) this._selection = new Selection();
      return this._selection;
    }, "getSelection");

    docProto.getAnimations = markNative(function () {
      return animationsFor(this);
    }, "getAnimations");

    let docTitle = "";
    Object.defineProperty(docProto, "title", {
      get: function () {
        const el = this.querySelector ? this.querySelector("title") : null;
        return el ? (el.textContent || "") : docTitle;
      },
      set: function (v) {
        docTitle = String(v);
        const el = this.querySelector ? this.querySelector("title") : null;
        if (el) el.textContent = docTitle;
      },
      configurable: true,
    });

    Object.defineProperty(docProto, "timeline", {
      get: function () { return globalDocTimeline; },
      configurable: true,
    });

    Object.defineProperty(docProto, "defaultView", {
      get: function () { return global; },
      configurable: true,
    });

    let docCookie = "";
    Object.defineProperty(docProto, "cookie", {
      get: function () { return docCookie; },
      set: function (v) {
        const str = String(v || "").trim();
        if (!str) return;
        const pair = str.split(";")[0].trim();
        const eq = pair.indexOf("=");
        if (eq !== -1) {
          const k = pair.slice(0, eq).trim();
          const val = pair.slice(eq + 1).trim();
          const cookies = docCookie ? docCookie.split("; ").filter(function (c) { return !c.startsWith(k + "="); }) : [];
          cookies.push(k + "=" + val);
          docCookie = cookies.join("; ");
        }
      },
      configurable: true,
    });
  }

  if (global.document) {
    if (!global.document.location) global.document.location = global.location;
    if (!global.document.createRange) global.document.createRange = function () { return new Range(); };
    if (!global.document.getSelection) global.document.getSelection = function () {
      if (!global.document._selection) global.document._selection = new Selection();
      return global.document._selection;
    };
  }

  // 14. Mount window EventTarget methods
  global.addEventListener = EventTarget.prototype.addEventListener.bind(global);
  global.removeEventListener = EventTarget.prototype.removeEventListener.bind(global);
  global.dispatchEvent = EventTarget.prototype.dispatchEvent.bind(global);
  global.getSelection = function () {
    if (global.document && global.document.getSelection) return global.document.getSelection();
    return new Selection();
  };
}

// Auto-install on globalThis/window
installShim(typeof window !== "undefined" ? window : globalThis);
