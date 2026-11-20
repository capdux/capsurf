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
  global.MessageEvent = MessageEvent;
  global.SubmitEvent = SubmitEvent;
  global.FormDataEvent = FormDataEvent;
  global.EventTarget = EventTarget;
  global.DeviceMotionEvent = DeviceMotionEvent;
  global.DeviceMotionEventAcceleration = DeviceMotionEventAcceleration;
  global.DeviceMotionEventRotationRate = DeviceMotionEventRotationRate;
  global.DeviceOrientationEvent = DeviceOrientationEvent;
  global.PromiseRejectionEvent = PromiseRejectionEvent;
  global.WheelEvent = WheelEvent;
  global.PointerEvent = PointerEvent;
  global.DragEvent = DragEvent;
  global.TextEvent = TextEvent;
  global.CompositionEvent = CompositionEvent;
  global.StorageEvent = StorageEvent;
  global.ErrorEvent = ErrorEvent;
  global.HashChangeEvent = HashChangeEvent;
  global.PopStateEvent = PopStateEvent;
  global.PageTransitionEvent = PageTransitionEvent;
  global.TransitionEvent = TransitionEvent;
  global.AnimationEvent = AnimationEvent;
  global.BeforeUnloadEvent = BeforeUnloadEvent;

  if (typeof globalThis !== "undefined") {
    globalThis.Event = Event;
    globalThis.CustomEvent = CustomEvent;
    globalThis.UIEvent = UIEvent;
    globalThis.MouseEvent = MouseEvent;
    globalThis.KeyboardEvent = KeyboardEvent;
    globalThis.FocusEvent = FocusEvent;
    globalThis.InputEvent = InputEvent;
    globalThis.MessageEvent = MessageEvent;
    globalThis.SubmitEvent = SubmitEvent;
    globalThis.FormDataEvent = FormDataEvent;
    globalThis.EventTarget = EventTarget;
    globalThis.DeviceMotionEvent = DeviceMotionEvent;
    globalThis.DeviceMotionEventAcceleration = DeviceMotionEventAcceleration;
    globalThis.DeviceMotionEventRotationRate = DeviceMotionEventRotationRate;
    globalThis.DeviceOrientationEvent = DeviceOrientationEvent;
    globalThis.PromiseRejectionEvent = PromiseRejectionEvent;
    globalThis.WheelEvent = WheelEvent;
    globalThis.PointerEvent = PointerEvent;
    globalThis.DragEvent = DragEvent;
    globalThis.TextEvent = TextEvent;
    globalThis.CompositionEvent = CompositionEvent;
    globalThis.StorageEvent = StorageEvent;
    globalThis.ErrorEvent = ErrorEvent;
    globalThis.HashChangeEvent = HashChangeEvent;
    globalThis.PopStateEvent = PopStateEvent;
    globalThis.PageTransitionEvent = PageTransitionEvent;
    globalThis.TransitionEvent = TransitionEvent;
    globalThis.AnimationEvent = AnimationEvent;
    globalThis.BeforeUnloadEvent = BeforeUnloadEvent;
  }

  // 2. Expose DOM Collections and Nodes
  global.DOMTokenList = DOMTokenList;
  global.HTMLCollection = HTMLCollection;
  global.HTMLOptionsCollection = HTMLOptionsCollection;
  if (typeof HTMLFormControlsCollection !== "undefined") {
    global.HTMLFormControlsCollection = HTMLFormControlsCollection;
  }
  if (typeof RadioNodeList !== "undefined") {
    global.RadioNodeList = RadioNodeList;
  }
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

  // 4. Expose FormData, Blob, File, FileList and DataTransfer
  global.FormData = FormData;
  global.Blob = Blob;
  global.File = File;
  global.FileReader = FileReader;
  if (typeof FileList !== "undefined") global.FileList = FileList;
  if (typeof DataTransfer !== "undefined") global.DataTransfer = DataTransfer;
  if (typeof DataTransferItem !== "undefined") global.DataTransferItem = DataTransferItem;
  if (typeof DataTransferItemList !== "undefined") global.DataTransferItemList = DataTransferItemList;
  if (typeof globalThis !== "undefined") {
    globalThis.FormData = FormData;
    globalThis.Blob = Blob;
    globalThis.File = File;
    globalThis.FileReader = FileReader;
    if (typeof FileList !== "undefined") globalThis.FileList = FileList;
    if (typeof DataTransfer !== "undefined") globalThis.DataTransfer = DataTransfer;
    if (typeof DataTransferItem !== "undefined") globalThis.DataTransferItem = DataTransferItem;
    if (typeof DataTransferItemList !== "undefined") globalThis.DataTransferItemList = DataTransferItemList;
  }
  if (typeof Headers !== "undefined") global.Headers = Headers;
  if (typeof Request !== "undefined") global.Request = Request;
  if (typeof Response !== "undefined") global.Response = Response;
  if (typeof History !== "undefined") {
    global.History = History;
    if (typeof globalThis !== "undefined") globalThis.History = History;
    if (typeof window !== "undefined") window.History = History;
  }
  if (typeof WebSocket !== "undefined") {
    global.WebSocket = WebSocket;
    if (typeof globalThis !== "undefined") globalThis.WebSocket = WebSocket;
    if (typeof window !== "undefined") window.WebSocket = WebSocket;
  }
  if (typeof CloseEvent !== "undefined") {
    global.CloseEvent = CloseEvent;
    if (typeof globalThis !== "undefined") globalThis.CloseEvent = CloseEvent;
    if (typeof window !== "undefined") window.CloseEvent = CloseEvent;
  }
  if (typeof XMLHttpRequest !== "undefined") {
    global.XMLHttpRequest = XMLHttpRequest;
    if (typeof globalThis !== "undefined") globalThis.XMLHttpRequest = XMLHttpRequest;
    if (typeof window !== "undefined") window.XMLHttpRequest = XMLHttpRequest;
  }
  if (typeof XMLHttpRequestEventTarget !== "undefined") {
    global.XMLHttpRequestEventTarget = XMLHttpRequestEventTarget;
    if (typeof globalThis !== "undefined") globalThis.XMLHttpRequestEventTarget = XMLHttpRequestEventTarget;
    if (typeof window !== "undefined") window.XMLHttpRequestEventTarget = XMLHttpRequestEventTarget;
  }
  if (typeof XMLHttpRequestUpload !== "undefined") {
    global.XMLHttpRequestUpload = XMLHttpRequestUpload;
    if (typeof globalThis !== "undefined") globalThis.XMLHttpRequestUpload = XMLHttpRequestUpload;
    if (typeof window !== "undefined") window.XMLHttpRequestUpload = XMLHttpRequestUpload;
  }

  // 5. Expose Traversal
  global.NodeFilter = NodeFilter;
  global.TreeWalker = TreeWalker;
  global.NodeIterator = NodeIterator;

  // 6. Expose Range and Selection
  global.Range = Range;
  global.Selection = Selection;

  // 7. Expose DOMParser
  global.DOMParser = DOMParser;

  // 7.5. Expose XPath
  if (typeof XPathResult !== "undefined") {
    global.XPathResult = XPathResult;
    global.XPathExpression = XPathExpression;
    global.XPathEvaluator = XPathEvaluator;
  }

  // 8. Expose Canvas 2D
  global.CanvasRenderingContext2D = CanvasRenderingContext2D;
  global.CanvasGradient = CanvasGradient;
  global.CanvasPattern = CanvasPattern;
  global.Path2D = Path2D;
  global.ImageData = ImageData;
  global.TextMetrics = TextMetrics;
  if (typeof OffscreenCanvas !== "undefined") {
    global.OffscreenCanvas = OffscreenCanvas;
    if (typeof window !== "undefined") global.window.OffscreenCanvas = OffscreenCanvas;
    if (typeof globalThis !== "undefined") globalThis.OffscreenCanvas = OffscreenCanvas;
  }

  // 9. Expose Web Animations
  global.Animation = Animation;
  global.KeyframeEffect = KeyframeEffect;
  global.AnimationTimeline = AnimationTimeline;
  global.DocumentTimeline = DocumentTimeline;

  // 10. Install Element & HTML extensions
  if (global.Element && typeof global.Element !== "function") {
    const nativeElProto = global.Element.prototype;
    const ElementCtor = function Element() {
      throw new TypeError("Illegal constructor");
    };
    ElementCtor.prototype = nativeElProto;
    nativeElProto.constructor = ElementCtor;
    Object.defineProperty(ElementCtor.prototype, Symbol.toStringTag, {
      value: "Element",
      configurable: true,
    });
    markNative(ElementCtor, "Element");
    global.Element = ElementCtor;
    if (typeof globalThis !== "undefined") globalThis.Element = ElementCtor;
    if (typeof window !== "undefined") window.Element = ElementCtor;
  }
  if (global.Document && typeof global.Document !== "function") {
    const nativeDocProto = global.Document.prototype;
    const DocumentCtor = function Document() {};
    DocumentCtor.prototype = nativeDocProto;
    nativeDocProto.constructor = DocumentCtor;
    Object.defineProperty(DocumentCtor.prototype, Symbol.toStringTag, {
      value: "Document",
      configurable: true,
    });
    markNative(DocumentCtor, "Document");
    global.Document = DocumentCtor;
    if (typeof globalThis !== "undefined") globalThis.Document = DocumentCtor;
    if (typeof window !== "undefined") window.Document = DocumentCtor;
  }
  if (global.Window && typeof global.Window !== "function") {
    const nativeWinProto = global.Window.prototype;
    const WindowCtor = function Window() {
      throw new TypeError("Illegal constructor");
    };
    WindowCtor.prototype = nativeWinProto;
    nativeWinProto.constructor = WindowCtor;
    Object.defineProperty(WindowCtor.prototype, Symbol.toStringTag, {
      value: "Window",
      configurable: true,
    });
    markNative(WindowCtor, "Window");
    global.Window = WindowCtor;
    if (typeof globalThis !== "undefined") globalThis.Window = WindowCtor;
    if (typeof window !== "undefined") window.Window = WindowCtor;
  }
  if (global.Element) {
    installElementExtensions(global.Element.prototype);
    installHtmlElementExtensions(global.Element.prototype);

    global.Element.prototype.animate = markNative(function (keyframes, options) {
      return animateElement(this, keyframes, options);
    }, "animate");

    global.Element.prototype.getAnimations = markNative(function () {
      return animationsFor(this);
    }, "getAnimations");

    global.Element.prototype.getContext = markNative(function (type) {
      const t = String(type).toLowerCase();
      if (t === "webgl" || t === "webgl2" || t === "experimental-webgl") {
        return null;
      }
      if (t === "2d") {
        if (!this._ctx2d) {
          this._ctx2d = new CanvasRenderingContext2D(this, _INTERNAL_CTX_SECRET);
        }
        return this._ctx2d;
      }
      return null;
    }, "getContext");
    
    // Add selector methods
    global.Element.prototype.querySelector = wrapQuerySelector(function(sel) {
        const root = typeof this.getRootNode === "function" ? this.getRootNode() : null;
        if (!this.isConnected || (root && root !== document)) {
            if (typeof findInSubtree === "function" && typeof matchesElementSelector === "function") {
                return findInSubtree(this, function (el) {
                    return matchesElementSelector(el, sel, root);
                }, true);
            }
            return null;
        }
        try {
            const all = document.querySelectorAll(sel);
            for(let i = 0; i < all.length; i++) {
                if (this !== all[i] && this.contains(all[i])) return all[i];
            }
            return null;
        } catch (e) {
            if (e && e.name === "SyntaxError") throw e;
            if (typeof findInSubtree === "function" && typeof matchesElementSelector === "function") {
                return findInSubtree(this, function (el) {
                    return matchesElementSelector(el, sel, root);
                }, true);
            }
            return null;
        }
    });
    global.Element.prototype.querySelectorAll = wrapQuerySelectorAll(function(sel) {
        const root = typeof this.getRootNode === "function" ? this.getRootNode() : null;
        if (!this.isConnected || (root && root !== document)) {
            if (typeof findInSubtree === "function" && typeof matchesElementSelector === "function") {
                return findInSubtree(this, function (el) {
                    return matchesElementSelector(el, sel, root);
                }, false) || [];
            }
            return [];
        }
        try {
            const all = document.querySelectorAll(sel);
            const matches = [];
            for(let i = 0; i < all.length; i++) {
                if (this !== all[i] && this.contains(all[i])) matches.push(all[i]);
            }
            return matches;
        } catch (e) {
            if (e && e.name === "SyntaxError") throw e;
            if (typeof findInSubtree === "function" && typeof matchesElementSelector === "function") {
                return findInSubtree(this, function (el) {
                    return matchesElementSelector(el, sel, root);
                }, false) || [];
            }
            return [];
        }
    });

    global.Element.prototype.toDataURL = markNative(function (type) {
      if (this._ctx2d) {
        const w = Number(this.getAttribute("width") || this.width || 300);
        const h = Number(this.getAttribute("height") || this.height || 150);
        const bm = ensureBitmap(this._ctx2d);
        if (typeof encodePngBase64 === "function") {
          return encodePngBase64(w, h, bm);
        }
      }
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
  if (typeof installFieldSetElement === "function") {
    installFieldSetElement(global);
  }
  if (typeof installLabelElement === "function") {
    installLabelElement(global);
  }
  if (typeof installDialogElement === "function") {
    installDialogElement(global);
  }

  // 12. Install window environment
  installWindow(global);

  function upgradeElement(el) {
    if (!el || typeof el !== "object" || el._upgraded) return el;
    el._upgraded = true;
    if (typeof el._nid === "number" && el._id === undefined) {
      el._id = el._nid;
    }
    if (typeof global.__capsurf_recordCanonicalId === "function") {
      global.__capsurf_recordCanonicalId(el);
    }
    const tag = String(el.localName || el.tagName || "").toLowerCase();
    const ns = el.namespaceURI;
    if (ns === "http://www.w3.org/2000/svg") {
      if (tag === "svg" && global.SVGSVGElement) {
        Object.setPrototypeOf(el, global.SVGSVGElement.prototype);
      } else if (global.SVGElement) {
        Object.setPrototypeOf(el, global.SVGElement.prototype);
      }
      return el;
    }
    if (ns && ns !== "http://www.w3.org/1999/xhtml") {
      return el;
    }
    switch (tag) {
      case "input": Object.setPrototypeOf(el, global.HTMLInputElement.prototype); break;
      case "textarea": Object.setPrototypeOf(el, global.HTMLTextAreaElement.prototype); break;
      case "select": Object.setPrototypeOf(el, global.HTMLSelectElement.prototype); break;
      case "option": Object.setPrototypeOf(el, global.HTMLOptionElement.prototype); break;
      case "optgroup": Object.setPrototypeOf(el, (global.HTMLOptGroupElement || global.HTMLElement).prototype); break;
      case "button": Object.setPrototypeOf(el, global.HTMLButtonElement.prototype); break;
      case "form": Object.setPrototypeOf(el, global.HTMLFormElement.prototype); break;
      case "fieldset": Object.setPrototypeOf(el, (global.HTMLFieldSetElement || global.HTMLElement).prototype); break;
      case "legend": Object.setPrototypeOf(el, (global.HTMLLegendElement || global.HTMLElement).prototype); break;
      case "label": Object.setPrototypeOf(el, (global.HTMLLabelElement || global.HTMLElement).prototype); break;
      case "a": Object.setPrototypeOf(el, global.HTMLAnchorElement.prototype); break;
      case "area": Object.setPrototypeOf(el, global.HTMLAreaElement.prototype); break;
      case "img": Object.setPrototypeOf(el, global.HTMLImageElement.prototype); break;
      case "picture": Object.setPrototypeOf(el, (global.HTMLPictureElement || global.HTMLElement).prototype); break;
      case "source": Object.setPrototypeOf(el, (global.HTMLSourceElement || global.HTMLElement).prototype); break;
      case "canvas": Object.setPrototypeOf(el, global.HTMLCanvasElement.prototype); break;
      case "script": Object.setPrototypeOf(el, global.HTMLScriptElement.prototype); break;
      case "iframe": Object.setPrototypeOf(el, global.HTMLIFrameElement.prototype); break;
      case "template": Object.setPrototypeOf(el, global.HTMLTemplateElement.prototype); break;
      case "slot": Object.setPrototypeOf(el, (global.HTMLSlotElement || global.HTMLElement).prototype); break;
      case "details": Object.setPrototypeOf(el, (global.HTMLDetailsElement || global.HTMLElement).prototype); break;
      case "dialog": Object.setPrototypeOf(el, global.HTMLDialogElement ? global.HTMLDialogElement.prototype : global.HTMLElement.prototype); break;
      case "style": Object.setPrototypeOf(el, (global.HTMLStyleElement || global.HTMLElement).prototype); break;
      case "link": Object.setPrototypeOf(el, (global.HTMLLinkElement || global.HTMLElement).prototype); break;
      case "datalist": Object.setPrototypeOf(el, (global.HTMLDataListElement || global.HTMLElement).prototype); break;
      case "div": Object.setPrototypeOf(el, (global.HTMLDivElement || global.HTMLElement).prototype); break;
      case "h1":
      case "h2":
      case "h3":
      case "h4":
      case "h5":
      case "h6": Object.setPrototypeOf(el, (global.HTMLHeadingElement || global.HTMLElement).prototype); break;
      case "span": Object.setPrototypeOf(el, (global.HTMLSpanElement || global.HTMLElement).prototype); break;
      case "p": Object.setPrototypeOf(el, (global.HTMLParagraphElement || global.HTMLElement).prototype); break;
      case "ul": Object.setPrototypeOf(el, (global.HTMLUListElement || global.HTMLElement).prototype); break;
      case "ol": Object.setPrototypeOf(el, (global.HTMLOListElement || global.HTMLElement).prototype); break;
      case "li": Object.setPrototypeOf(el, (global.HTMLLIElement || global.HTMLElement).prototype); break;
      case "pre": Object.setPrototypeOf(el, (global.HTMLPreElement || global.HTMLElement).prototype); break;
      case "br": Object.setPrototypeOf(el, (global.HTMLBRElement || global.HTMLElement).prototype); break;
      case "hr": Object.setPrototypeOf(el, (global.HTMLHRElement || global.HTMLElement).prototype); break;
      case "body": Object.setPrototypeOf(el, (global.HTMLBodyElement || global.HTMLElement).prototype); break;
      case "head": Object.setPrototypeOf(el, (global.HTMLHeadElement || global.HTMLElement).prototype); break;
      case "html": Object.setPrototypeOf(el, (global.HTMLHtmlElement || global.HTMLElement).prototype); break;
      case "title": Object.setPrototypeOf(el, (global.HTMLTitleElement || global.HTMLElement).prototype); break;
      case "meta": Object.setPrototypeOf(el, (global.HTMLMetaElement || global.HTMLElement).prototype); break;
      case "table": Object.setPrototypeOf(el, (global.HTMLTableElement || global.HTMLElement).prototype); break;
      case "tr": Object.setPrototypeOf(el, (global.HTMLTableRowElement || global.HTMLElement).prototype); break;
      case "td":
      case "th": Object.setPrototypeOf(el, (global.HTMLTableCellElement || global.HTMLElement).prototype); break;
      case "thead":
      case "tbody":
      case "tfoot": Object.setPrototypeOf(el, (global.HTMLTableSectionElement || global.HTMLElement).prototype); break;
      case "output": Object.setPrototypeOf(el, (global.HTMLOutputElement || global.HTMLElement).prototype); break;
      case "progress": Object.setPrototypeOf(el, (global.HTMLProgressElement || global.HTMLElement).prototype); break;
      case "meter": Object.setPrototypeOf(el, (global.HTMLMeterElement || global.HTMLElement).prototype); break;
      default: Object.setPrototypeOf(el, global.HTMLElement.prototype); break;
    }
    return el;
  }
  global.upgradeElement = upgradeElement;
  if (typeof globalThis !== "undefined") {
    globalThis.upgradeElement = upgradeElement;
  }

  // 13. Enhance Document prototype and instance
  const docProto = (global.Document && global.Document.prototype) ? global.Document.prototype : null;
  if (docProto) {
    // Inherit from Node
    Object.setPrototypeOf(docProto, Node.prototype);

    const origQS = docProto.querySelector;
    docProto.querySelector = wrapQuerySelector(origQS);
    const origQSA = docProto.querySelectorAll;
    docProto.querySelectorAll = wrapQuerySelectorAll(origQSA);

    if (docProto.createElement) {
      const origCreateElement = docProto.createElement;
      docProto.createElement = function (tagName) {
        let el = origCreateElement.call(this, tagName);
        if (el) {
          const tag = String(tagName).toLowerCase();
          const registry = global.customElements || (typeof customElements !== "undefined" ? customElements : null);
          const Ctor = (registry && registry.get) ? registry.get(tag) : null;
          if (Ctor && typeof Ctor === "function" && !Ctor.prototype.isPrototypeOf(el)) {
            el._upgraded = true;
            if (typeof globalThis.upgradeCustomElement === "function") {
              globalThis.upgradeCustomElement(el, Ctor, { tagName: tag });
            } else {
              Object.setPrototypeOf(el, Ctor.prototype);
              const HEl = global.HTMLElement || (typeof HTMLElement !== "undefined" ? HTMLElement : null);
              if (HEl) HEl._upgradeTarget = el;
              try { new Ctor(); } catch (e) {} finally { if (HEl) HEl._upgradeTarget = null; }
            }
          }
        }
        el = upgradeElement(el);
        if (el && String(tagName).toLowerCase() === "script") {
          if (typeof globalThis !== "undefined" && globalThis.__capsurf_markScriptForceAsync) {
            globalThis.__capsurf_markScriptForceAsync(el);
          }
        }
        return el;
      };
      markNative(docProto.createElement, "createElement");
      if (global.document) {
        global.document.createElement = docProto.createElement;
      }
    }

    const origCreateElement = docProto.createElement;
    const origCreateElementNS = docProto.createElementNS || function (ns, tagName) {
      let el = origCreateElement ? origCreateElement.call(this, tagName) : null;
      if (!el) {
        el = Object.create(global.Element ? global.Element.prototype : Object.prototype);
        el.localName = String(tagName).toLowerCase();
        el.tagName = String(tagName).toUpperCase();
      }
      return el;
    };
    docProto.createElementNS = function (ns, tagName) {
      let el = origCreateElementNS.call(this, ns, tagName);
      if (el) {
        const nsVal = ns !== undefined && ns !== null ? String(ns) : null;
        el._namespaceURI = nsVal;
        try {
          Object.defineProperty(el, "namespaceURI", {
            value: nsVal,
            writable: true,
            configurable: true,
            enumerable: true,
          });
        } catch (e) {
          try { el.namespaceURI = nsVal; } catch (e2) {}
        }
        if (nsVal !== "http://www.w3.org/1999/xhtml") {
          el._upgraded = false;
          if (global.Element && global.Element.prototype) {
            Object.setPrototypeOf(el, global.Element.prototype);
          }
        }
      }
      if (el && (!ns || ns === "http://www.w3.org/1999/xhtml")) {
        const tag = String(tagName).toLowerCase();
        const registry = global.customElements || (typeof customElements !== "undefined" ? customElements : null);
        const Ctor = (registry && registry.get) ? registry.get(tag) : null;
        if (Ctor && typeof Ctor === "function" && !Ctor.prototype.isPrototypeOf(el)) {
          el._upgraded = true;
          if (typeof globalThis.upgradeCustomElement === "function") {
            globalThis.upgradeCustomElement(el, Ctor, { tagName: tag });
          } else {
            Object.setPrototypeOf(el, Ctor.prototype);
            const HEl = global.HTMLElement || (typeof HTMLElement !== "undefined" ? HTMLElement : null);
            if (HEl) HEl._upgradeTarget = el;
            try { new Ctor(); } catch (e) {} finally { if (HEl) HEl._upgradeTarget = null; }
          }
        }
      }
      el = upgradeElement(el);
      if (el && String(tagName).toLowerCase() === "script") {
        if (typeof globalThis !== "undefined" && globalThis.__capsurf_markScriptForceAsync) {
          globalThis.__capsurf_markScriptForceAsync(el);
        }
      }
      return el;
    };
    markNative(docProto.createElementNS, "createElementNS");
    if (global.document) {
      global.document.createElementNS = docProto.createElementNS;
    }

    if (docProto.getElementById) {
      const origGetElementById = docProto.getElementById;
      docProto.getElementById = function (id) {
        const el = upgradeElement(origGetElementById.call(this, id));
        if (el && typeof global.__capsurf_setLastQueriedNode === "function") {
          global.__capsurf_setLastQueriedNode(el);
        }
        return el;
      };
      markNative(docProto.getElementById, "getElementById");
    }

    if (docProto.querySelector) {
      const origQuerySelector = docProto.querySelector;
      docProto.querySelector = function (sel) {
        const el = upgradeElement(origQuerySelector.call(this, sel));
        if (el && typeof global.__capsurf_setLastQueriedNode === "function") {
          global.__capsurf_setLastQueriedNode(el);
        }
        return el;
      };
      markNative(docProto.querySelector, "querySelector");
    }

    if (docProto.querySelectorAll) {
      const origQuerySelectorAll = docProto.querySelectorAll;
      docProto.querySelectorAll = function (sel) {
        const nodes = origQuerySelectorAll.call(this, sel);
        if (nodes && nodes.length) {
          for (let i = 0; i < nodes.length; i++) upgradeElement(nodes[i]);
        }
        return nodes;
      };
      markNative(docProto.querySelectorAll, "querySelectorAll");
    }


    docProto.createRange = markNative(function () {
      return new Range();
    }, "createRange");

    if (!docProto.open) {
      docProto.open = markNative(function () {
        return this;
      }, "open");
    }
    if (!docProto.close) {
      docProto.close = markNative(function () {}, "close");
    }
    if (!docProto.write) {
      docProto.write = markNative(function () {
        let markup = "";
        for (let i = 0; i < arguments.length; i++) {
          markup += String(arguments[i]);
        }
        const target = this.body || this.documentElement;
        if (target && typeof this.createElement === "function") {
          const temp = this.createElement("div");
          temp.innerHTML = markup;
          while (temp.firstChild) {
            const ch = temp.firstChild;
            try { temp.removeChild(ch); } catch (e) {}
            target.appendChild(ch);
          }
        }
      }, "write");
      if (global.document) {
        global.document.write = docProto.write;
      }
    }
    if (!docProto.writeln) {
      docProto.writeln = markNative(function () {
        let markup = "";
        for (let i = 0; i < arguments.length; i++) {
          markup += String(arguments[i]);
        }
        return this.write(markup + "\n");
      }, "writeln");
      if (global.document) {
        global.document.writeln = docProto.writeln;
      }
    }

    docProto.createTreeWalker = markNative(function (root, whatToShow, filter) {
      return new TreeWalker(root, whatToShow, filter);
    }, "createTreeWalker");

    docProto.createNodeIterator = markNative(function (root, whatToShow, filter) {
      return new NodeIterator(root, whatToShow, filter);
    }, "createNodeIterator");

    docProto.getSelection = markNative(function () {
      if (!this._selection) this._selection = new Selection(this);
      return this._selection;
    }, "getSelection");

    docProto.getAnimations = markNative(function () {
      return animationsFor(this);
    }, "getAnimations");

    if (!docProto.append) {
      docProto.append = function (...nodes) {
        for (const n of nodes) {
          let node;
          if (n && typeof n === "object" && typeof n.nodeType === "number") {
            node = n;
          } else if (typeof document !== "undefined" && document.createTextNode) {
            node = document.createTextNode(String(n));
          } else {
            node = new Text(String(n));
          }
          this.appendChild(node);
        }
      };
      markNative(docProto.append, "append");
    }
    if (!docProto.prepend) {
      docProto.prepend = function (...nodes) {
        const first = this.firstChild;
        for (const n of nodes) {
          let node;
          if (n && typeof n === "object" && typeof n.nodeType === "number") {
            node = n;
          } else if (typeof document !== "undefined" && document.createTextNode) {
            node = document.createTextNode(String(n));
          } else {
            node = new Text(String(n));
          }
          this.insertBefore(node, first);
        }
      };
      markNative(docProto.prepend, "prepend");
    }
    if (!docProto.replaceChildren) {
      docProto.replaceChildren = function (...nodes) {
        while (this.firstChild) {
          this.removeChild(this.firstChild);
        }
        this.append(...nodes);
      };
      markNative(docProto.replaceChildren, "replaceChildren");
    }

    if (!docProto.children) {
      Object.defineProperty(docProto, "children", {
        get: function () {
          return getElementChildrenCollection(this);
        },
        configurable: true,
      });
    }
    if (!docProto.firstElementChild) {
      Object.defineProperty(docProto, "firstElementChild", {
        get: function () {
          const kids = this.children;
          return (kids && kids.length > 0) ? kids[0] : null;
        },
        configurable: true,
      });
    }
    if (!docProto.lastElementChild) {
      Object.defineProperty(docProto, "lastElementChild", {
        get: function () {
          const kids = this.children;
          return (kids && kids.length > 0) ? kids[kids.length - 1] : null;
        },
        configurable: true,
      });
    }
    if (!docProto.childElementCount) {
      Object.defineProperty(docProto, "childElementCount", {
        get: function () {
          const kids = this.children;
          return kids ? kids.length : 0;
        },
        configurable: true,
      });
    }

    function defineLiveCollection(name, pred) {
      Object.defineProperty(docProto, name, {
        get: function () {
          const doc = this;
          return makeHtmlCollection(function () {
            return pred ? getAllDocumentElements(doc).filter(pred) : [];
          });
        },
        configurable: true,
      });
    }
    function tagIs() {
      const names = arguments;
      return function (el) {
        const t = String(el.localName || el.tagName || "").toLowerCase();
        for (let i = 0; i < names.length; i++) if (t === names[i]) return true;
        return false;
      };
    }
    const isEmbed = tagIs("embed");
    defineLiveCollection("images", tagIs("img"));
    defineLiveCollection("forms", tagIs("form"));
    defineLiveCollection("scripts", tagIs("script"));
    defineLiveCollection("embeds", isEmbed);
    defineLiveCollection("plugins", isEmbed);
    defineLiveCollection("links", function (el) {
      return tagIs("a", "area")(el) && el.hasAttribute("href");
    });
    defineLiveCollection("anchors", function (el) {
      return tagIs("a")(el) && el.hasAttribute("name");
    });
    defineLiveCollection("applets", null);

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

    if (!Object.getOwnPropertyDescriptor(docProto, "title")) {
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
    }

    Object.defineProperty(docProto, "timeline", {
      get: function () { return globalDocTimeline; },
      configurable: true,
    });

    Object.defineProperty(docProto, "defaultView", {
      get: function () { return global; },
      configurable: true,
    });

    let docCookie = "";
    function clearDocCookie() {
      docCookie = "";
    }
    global.__capsurf_clear_doc_cookie = clearDocCookie;
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_clear_doc_cookie = clearDocCookie;
    }

    function getDocUrl(doc) {
      if (doc && doc.URL) return String(doc.URL);
      if (global && global.document && global.document.URL) return String(global.document.URL);
      if (typeof globalThis !== "undefined" && globalThis.document && globalThis.document.URL) return String(globalThis.document.URL);
      if (global && global.location && global.location.href) return String(global.location.href);
      if (typeof globalThis !== "undefined" && globalThis.location && globalThis.location.href) return String(globalThis.location.href);
      return "about:blank";
    }

    Object.defineProperty(docProto, "cookie", {
      get: function () {
        const docUrl = getDocUrl(this);
        const getFn = (typeof __capsurf_get_cookie_header === "function") ? __capsurf_get_cookie_header : ((typeof globalThis !== "undefined" && typeof globalThis.__capsurf_get_cookie_header === "function") ? globalThis.__capsurf_get_cookie_header : null);
        if (getFn) {
          const hdr = getFn(docUrl);
          if (hdr !== null && hdr !== undefined) {
            return String(hdr);
          }
        }
        if (docUrl.startsWith("http://") || docUrl.startsWith("https://")) {
          return "";
        }
        return docCookie;
      },
      set: function (v) {
        const str = String(v || "").trim();
        if (!str) return;
        const docUrl = getDocUrl(this);
        const setFn = (typeof __capsurf_set_cookie === "function") ? __capsurf_set_cookie : ((typeof globalThis !== "undefined" && typeof globalThis.__capsurf_set_cookie === "function") ? globalThis.__capsurf_set_cookie : null);
        if (setFn) {
          setFn(docUrl, str);
        }
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

    if (typeof evaluateXPath !== "undefined") {
      docProto.evaluate = markNative(function (expression, contextNode, resolver, type, result) {
        return evaluateXPath(expression, contextNode || this, resolver, type, result);
      }, "evaluate");
      docProto.createExpression = markNative(function (expression, resolver) {
        return createExpression(expression, resolver);
      }, "createExpression");
      docProto.createNSResolver = markNative(function (nodeResolver) {
        return nodeResolver;
      }, "createNSResolver");
    }

    // 13.5 ModelContext (WebMCP Registration Stub)
    function ModelContext() {
      EventTarget.call(this);
      this._tools = new Map();
    }
    ModelContext.prototype = Object.create(EventTarget.prototype);
    ModelContext.prototype.constructor = ModelContext;

    ModelContext.prototype.registerTool = function (tool, options) {
      if (!tool || typeof tool !== "object") {
        return Promise.reject(new TypeError("registerTool: tool must be an object"));
      }
      const name = String(tool.name || "");
      if (!name) {
        return Promise.reject(new TypeError("registerTool: tool.name is required"));
      }
      const opts = (options !== null && typeof options === "object") ? options : {};
      const signal = opts.signal;
      if (signal && signal.aborted) {
        return Promise.reject(signal.reason !== undefined ? signal.reason : (typeof DOMException !== "undefined" ? new DOMException("This operation was aborted", "AbortError") : new Error("Aborted")));
      }

      const self = this;
      this._tools.set(name, tool);

      if (typeof tool.execute === "function") {
        if (typeof window !== "undefined") {
          window[name] = function (args) {
            return tool.execute(args, { signal: signal });
          };
        }
        if (typeof globalThis !== "undefined") {
          globalThis[name] = function (args) {
            return tool.execute(args, { signal: signal });
          };
        }
      }

      const evt = new Event("toolchange");
      this.dispatchEvent(evt);

      return new Promise(function (resolve, reject) {
        if (signal && signal.addEventListener) {
          signal.addEventListener("abort", function () {
            self._tools.delete(name);
            if (typeof window !== "undefined" && window[name]) delete window[name];
            if (typeof globalThis !== "undefined" && globalThis[name]) delete globalThis[name];
            self.dispatchEvent(new Event("toolchange"));
            reject(signal.reason !== undefined ? signal.reason : (typeof DOMException !== "undefined" ? new DOMException("This operation was aborted", "AbortError") : new Error("Aborted")));
          }, { once: true });
        }
        Promise.resolve().then(function () {
          resolve(undefined);
        });
      });
    };

    ModelContext.prototype.unregisterTool = function (name) {
      const key = String(name || "");
      this._tools.delete(key);
      if (typeof window !== "undefined" && window[key]) delete window[key];
      if (typeof globalThis !== "undefined" && globalThis[key]) delete globalThis[key];
      this.dispatchEvent(new Event("toolchange"));
      return Promise.resolve(undefined);
    };

    ModelContext.prototype.getTools = function () {
      const list = [];
      this._tools.forEach(function (t) {
        list.push({
          name: t.name,
          description: t.description || "",
          inputSchema: t.inputSchema || null,
          origin: t.origin || (typeof location !== "undefined" ? location.origin : "null"),
        });
      });
      return Promise.resolve(list);
    };

    ModelContext.prototype.executeTool = function (toolOrName, inputObject, options) {
      const name = (toolOrName && typeof toolOrName === "object") ? toolOrName.name : String(toolOrName);
      const tool = this._tools.get(name);
      if (!tool || typeof tool.execute !== "function") {
        return Promise.reject(new Error("executeTool: no tool named '" + name + "'"));
      }
      try {
        const res = tool.execute(inputObject, options);
        return Promise.resolve(res);
      } catch (e) {
        return Promise.reject(e);
      }
    };

    Object.defineProperty(ModelContext.prototype, Symbol.toStringTag, {
      value: "ModelContext",
      configurable: true,
    });

    markNative(ModelContext, "ModelContext");
    markNative(ModelContext.prototype.registerTool, "registerTool");
    markNative(ModelContext.prototype.unregisterTool, "unregisterTool");
    markNative(ModelContext.prototype.getTools, "getTools");
    markNative(ModelContext.prototype.executeTool, "executeTool");

    global.ModelContext = ModelContext;
    if (typeof globalThis !== "undefined") globalThis.ModelContext = ModelContext;
    if (typeof window !== "undefined") window.ModelContext = ModelContext;

    function getOrCreateDocModelContext(doc) {
      if (!doc._modelContext) {
        doc._modelContext = new ModelContext();
      }
      return doc._modelContext;
    }

    Object.defineProperty(docProto, "modelContext", {
      get: function () {
        return getOrCreateDocModelContext(this);
      },
      configurable: true,
    });
  }

  if (global.document) {
    if (!global.document.modelContext) {
      Object.defineProperty(global.document, "modelContext", {
        get: function () {
          if (!global.document._modelContext) {
            global.document._modelContext = new (global.ModelContext || ModelContext)();
          }
          return global.document._modelContext;
        },
        configurable: true,
      });
    }
    if (!Object.getOwnPropertyDescriptor(global.document, "location")) {
      try {
        Object.defineProperty(global.document, "location", {
          get: function () { return global.location; },
          set: function (val) { if (global.location) global.location.href = val; },
          enumerable: true,
          configurable: false,
        });
      } catch (e) {}
    }
    if (!global.document.createRange) global.document.createRange = function () { return new Range(); };
    if (!global.document.getSelection) global.document.getSelection = function () {
      if (!global.document._selection) global.document._selection = new Selection(global.document);
      return global.document._selection;
    };
    if (docProto) {
      if (!global.document.elementFromPoint && docProto.elementFromPoint) {
        global.document.elementFromPoint = docProto.elementFromPoint.bind(global.document);
      }
      if (!global.document.elementsFromPoint && docProto.elementsFromPoint) {
        global.document.elementsFromPoint = docProto.elementsFromPoint.bind(global.document);
      }
      if (typeof evaluateXPath !== "undefined") {
        if (!global.document.evaluate && docProto.evaluate) global.document.evaluate = docProto.evaluate.bind(global.document);
        if (!global.document.createExpression && docProto.createExpression) global.document.createExpression = docProto.createExpression.bind(global.document);
        if (!global.document.createNSResolver && docProto.createNSResolver) global.document.createNSResolver = docProto.createNSResolver.bind(global.document);
      }
    }
    if (typeof processSubtreeTemplates === "function") {
      processSubtreeTemplates(global.document);
    }
  }

  // 14. Mount window EventTarget methods and window.event own property
  if (!Object.prototype.hasOwnProperty.call(global, "event")) {
    Object.defineProperty(global, "event", {
      value: undefined,
      writable: true,
      enumerable: true,
      configurable: true,
    });
  }
  if (typeof window !== "undefined" && !Object.prototype.hasOwnProperty.call(window, "event")) {
    Object.defineProperty(window, "event", {
      value: undefined,
      writable: true,
      enumerable: true,
      configurable: true,
    });
  }
  if (typeof globalThis !== "undefined" && !Object.prototype.hasOwnProperty.call(globalThis, "event")) {
    Object.defineProperty(globalThis, "event", {
      value: undefined,
      writable: true,
      enumerable: true,
      configurable: true,
    });
  }
  global.addEventListener = EventTarget.prototype.addEventListener.bind(global);
  global.removeEventListener = EventTarget.prototype.removeEventListener.bind(global);
  global.dispatchEvent = EventTarget.prototype.dispatchEvent.bind(global);
  if (typeof window !== "undefined" && window !== global) {
    window.addEventListener = global.addEventListener;
    window.removeEventListener = global.removeEventListener;
    window.dispatchEvent = global.dispatchEvent;
  }
  if (typeof globalThis !== "undefined" && globalThis !== global) {
    globalThis.addEventListener = global.addEventListener;
    globalThis.removeEventListener = global.removeEventListener;
    globalThis.dispatchEvent = global.dispatchEvent;
  }
  global.getSelection = function () {
    if (global.document && global.document.getSelection) return global.document.getSelection();
    return new Selection();
  };
  if (typeof window !== "undefined") {
    window.getSelection = global.getSelection;
  }
  if (typeof globalThis !== "undefined") {
    globalThis.getSelection = global.getSelection;
  }

  // 15. Kitesurf page operations support (getPageState, setControlValue)
  if (typeof global.__treeSeq === "undefined") {
    global.__treeSeq = 1;
  }
  if (typeof globalThis !== "undefined" && typeof globalThis.__treeSeq === "undefined") {
    globalThis.__treeSeq = 1;
  }

  function setControlValueAsUser(control, value) {
    const tag = String(control?.localName || control?.tagName || "").toLowerCase();
    const type = String(control?.type || "").toLowerCase();
    const toggle = tag === "input" && (type === "checkbox" || type === "radio");
    const NO_USER_VALUE = new Set(["hidden", "file", "button", "submit", "reset", "image"]);
    const entry = tag === "textarea" || (tag === "input" && !toggle && !NO_USER_VALUE.has(type));
    if (tag !== "select" && !toggle && !entry) {
      return { changed: false };
    }
    const isDisabled = Boolean(control.disabled || (control.hasAttribute && control.hasAttribute("disabled")));
    if (isDisabled) {
      return { changed: false, error: "control is disabled" };
    }
    const isReadOnly = Boolean(control.readOnly || (control.hasAttribute && control.hasAttribute("readonly")));
    if (entry && isReadOnly) {
      return { changed: false, error: "control is read-only" };
    }
    if (tag === "select") {
      const wanted = control.multiple === true && Array.isArray(value)
        ? value.map(String)
        : [value == null ? "" : String(value)];
      const options = Array.from(control.options || []);
      const optionEnabled = (opt) => {
        if (opt.disabled || (opt.hasAttribute && opt.hasAttribute("disabled"))) return false;
        const group = opt.parentNode;
        if (group && String(group.localName || group.tagName || "").toLowerCase() === "optgroup") {
          if (group.disabled || (group.hasAttribute && group.hasAttribute("disabled"))) return false;
        }
        return true;
      };
      const offered = new Set(options.filter(optionEnabled).map((o) => String(o.value ?? "")));
      if (!wanted.every((w) => offered.has(w))) {
        return { changed: false, error: "no such option" };
      }
    }

    let changed = false;
    if (type === "checkbox") {
      const want = value === true;
      if (control.checked !== want) {
        control.checked = want;
        changed = true;
      }
    } else if (type === "radio") {
      const want = String(control.value || "on") === String(value);
      if (control.checked !== want) {
        control.checked = want;
        changed = true;
      }
    } else if (tag === "select" && control.multiple === true && Array.isArray(value)) {
      const selected = new Set(value.map(String));
      const options = control.options || [];
      for (let j = 0; j < options.length; j++) {
        const opt = options[j];
        const want = selected.has(String(opt.value));
        if (opt.selected !== want) {
          opt.selected = want;
          changed = true;
        }
      }
    } else {
      const want = value == null ? "" : String(value);
      if (String(control.value || "") !== want) {
        control.value = want;
        changed = true;
      }
    }

    if (changed) {
      try {
        const inputEvt = typeof InputEvent !== "undefined"
          ? new InputEvent("input", { bubbles: true, cancelable: false })
          : new Event("input", { bubbles: true, cancelable: false });
        if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
          global.__capsurf_dispatchTrustedEvent(control, inputEvt);
        } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_dispatchTrustedEvent === "function") {
          globalThis.__capsurf_dispatchTrustedEvent(control, inputEvt);
        } else if (control.dispatchEvent) {
          control.dispatchEvent(inputEvt);
        }
      } catch (e) {}
      try {
        const changeEvt = new Event("change", { bubbles: true, cancelable: false });
        if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
          global.__capsurf_dispatchTrustedEvent(control, changeEvt);
        } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_dispatchTrustedEvent === "function") {
          globalThis.__capsurf_dispatchTrustedEvent(control, changeEvt);
        } else if (control.dispatchEvent) {
          control.dispatchEvent(changeEvt);
        }
      } catch (e) {}
    }
    return { changed: changed, value: control.value !== undefined ? control.value : "" };
  }

  function findNodeById(nodeId) {
    let el = null;
    if (typeof global.__capsurf_native_find_node_by_id === "function") {
      el = global.__capsurf_native_find_node_by_id(nodeId);
    } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_native_find_node_by_id === "function") {
      el = globalThis.__capsurf_native_find_node_by_id(nodeId);
    }
    if (el && typeof upgradeElement === "function") {
      upgradeElement(el);
    }
    return el;
  }

  global.__capsurf_find_node_by_id = findNodeById;
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_find_node_by_id = findNodeById;
  }

  function setControlValueJson(paramsJson) {
    let params;
    try {
      params = JSON.parse(paramsJson);
    } catch (e) {
      return JSON.stringify({ changed: false, error: "bad nodeId" });
    }
    const inner = (params && typeof params.params === "object" && params.params !== null) ? params.params : params;
    const nodeId = inner.nodeId;
    if (typeof nodeId !== "number" || !(nodeId > 0)) {
      return JSON.stringify({ changed: false, error: "bad nodeId" });
    }
    const el = findNodeById(nodeId);
    if (!el) {
      return JSON.stringify({ changed: false, error: "bad nodeId" });
    }
    return JSON.stringify(setControlValueAsUser(el, inner.value));
  }

  global.__capsurf_setControlValueJson = setControlValueJson;
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_setControlValueJson = setControlValueJson;
  }
}

// Auto-install on globalThis/window
installShim(typeof window !== "undefined" ? window : globalThis);
