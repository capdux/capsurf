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

  // 4. Expose FormData, Blob and File
  global.FormData = FormData;
  global.Blob = Blob;
  global.File = File;
  global.FileReader = FileReader;
  if (typeof globalThis !== "undefined") globalThis.FileReader = FileReader;
  if (typeof Headers !== "undefined") global.Headers = Headers;
  if (typeof Request !== "undefined") global.Request = Request;
  if (typeof Response !== "undefined") global.Response = Response;

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
        const all = document.querySelectorAll(sel);
        for(let i = 0; i < all.length; i++) {
            if (this !== all[i] && this.contains(all[i])) return all[i];
        }
        return null;
    });
    global.Element.prototype.querySelectorAll = wrapQuerySelectorAll(function(sel) {
        const all = document.querySelectorAll(sel);
        const matches = [];
        for(let i = 0; i < all.length; i++) {
            if (this !== all[i] && this.contains(all[i])) matches.push(all[i]);
        }
        return matches;
    });

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
  if (typeof installDialogElement === "function") {
    installDialogElement(global);
  }

  // 12. Install window environment
  installWindow(global);

  function upgradeElement(el) {
    if (!el || typeof el !== "object" || el._upgraded) return el;
    el._upgraded = true;
    if (typeof global.__capsurf_recordCanonicalId === "function") {
      global.__capsurf_recordCanonicalId(el);
    }
    const tag = String(el.localName || el.tagName || "").toLowerCase();
    switch (tag) {
      case "input": Object.setPrototypeOf(el, global.HTMLInputElement.prototype); break;
      case "textarea": Object.setPrototypeOf(el, global.HTMLTextAreaElement.prototype); break;
      case "select": Object.setPrototypeOf(el, global.HTMLSelectElement.prototype); break;
      case "option": Object.setPrototypeOf(el, global.HTMLOptionElement.prototype); break;
      case "button": Object.setPrototypeOf(el, global.HTMLButtonElement.prototype); break;
      case "form": Object.setPrototypeOf(el, global.HTMLFormElement.prototype); break;
      case "a": Object.setPrototypeOf(el, global.HTMLAnchorElement.prototype); break;
      case "area": Object.setPrototypeOf(el, global.HTMLAreaElement.prototype); break;
      case "img": Object.setPrototypeOf(el, global.HTMLImageElement.prototype); break;
      case "canvas": Object.setPrototypeOf(el, global.HTMLCanvasElement.prototype); break;
      case "script": Object.setPrototypeOf(el, global.HTMLScriptElement.prototype); break;
      case "iframe": Object.setPrototypeOf(el, global.HTMLIFrameElement.prototype); break;
      case "template": Object.setPrototypeOf(el, global.HTMLTemplateElement.prototype); break;
      case "dialog": Object.setPrototypeOf(el, global.HTMLDialogElement ? global.HTMLDialogElement.prototype : global.HTMLElement.prototype); break;
      default: Object.setPrototypeOf(el, global.HTMLElement.prototype); break;
    }
    return el;
  }
  global.upgradeElement = upgradeElement;
  if (typeof globalThis !== "undefined") {
    globalThis.upgradeElement = upgradeElement;
  }

  // 13. Enhance Document prototype and instance
  if (global.Document && global.Document.prototype) {
    const docProto = global.Document.prototype;

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
        el.namespaceURI = ns !== undefined && ns !== null ? String(ns) : null;
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
  }

  if (global.document) {
    if (!global.document.location) global.document.location = global.location;
    if (!global.document.createRange) global.document.createRange = function () { return new Range(); };
    if (!global.document.getSelection) global.document.getSelection = function () {
      if (!global.document._selection) global.document._selection = new Selection();
      return global.document._selection;
    };
    if (typeof evaluateXPath !== "undefined") {
      if (!global.document.evaluate) global.document.evaluate = docProto.evaluate.bind(global.document);
      if (!global.document.createExpression) global.document.createExpression = docProto.createExpression.bind(global.document);
      if (!global.document.createNSResolver) global.document.createNSResolver = docProto.createNSResolver.bind(global.document);
    }
    if (typeof processSubtreeTemplates === "function") {
      processSubtreeTemplates(global.document);
    }
  }

  // 14. Mount window EventTarget methods
  global.addEventListener = EventTarget.prototype.addEventListener.bind(global);
  global.removeEventListener = EventTarget.prototype.removeEventListener.bind(global);
  global.dispatchEvent = EventTarget.prototype.dispatchEvent.bind(global);
  global.getSelection = function () {
    if (global.document && global.document.getSelection) return global.document.getSelection();
    return new Selection();
  };

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
