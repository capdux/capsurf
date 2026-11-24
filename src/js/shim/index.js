// src/js/shim/index.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/index.ts
// Entry point for CapSurf Web API Runtime Shim.

function _makeTagCollection(root, qualifiedName) {
  const q = String(qualifiedName);
  const isWildcard = (q === "*");
  const qLower = q.toLowerCase();
  const getList = function () {
    if (typeof findInSubtree !== "function") return [];
    const isHtmlDoc = !root.ownerDocument ? (root.contentType !== "application/xml" && root.contentType !== "text/xml") : (root.ownerDocument.contentType !== "application/xml" && root.ownerDocument.contentType !== "text/xml");
    const list = findInSubtree(root, function (el) {
      if (isWildcard) return true;
      const tag = el.tagName || el.nodeName || "";
      return isHtmlDoc ? (tag.toLowerCase() === qLower) : (tag === q);
    }, false) || [];
    if (typeof upgradeElement === "function") {
      for (let i = 0; i < list.length; i++) list[i] = upgradeElement(list[i]);
    }
    return list;
  };
  return (typeof makeHtmlCollection === "function") ? makeHtmlCollection(getList) : getList();
}

function _makeTagNSCollection(root, namespace, localName) {
  const ns = (namespace === null || namespace === undefined || namespace === "") ? null : String(namespace);
  const ln = String(localName);
  const getList = function () {
    if (typeof findInSubtree !== "function") return [];
    const list = findInSubtree(root, function (el) {
      const elNs = el.namespaceURI !== undefined ? el.namespaceURI : "http://www.w3.org/1999/xhtml";
      const elLn = el.localName || (el.tagName || el.nodeName || "").toLowerCase();
      const nsMatch = (ns === "*") || (elNs === ns);
      const lnMatch = (ln === "*") || (elLn === ln);
      return nsMatch && lnMatch;
    }, false) || [];
    if (typeof upgradeElement === "function") {
      for (let i = 0; i < list.length; i++) list[i] = upgradeElement(list[i]);
    }
    return list;
  };
  return (typeof makeHtmlCollection === "function") ? makeHtmlCollection(getList) : getList();
}

function _makeClassCollection(root, classNames) {
  const tokens = String(classNames).split(/[ \t\r\n\f]+/).filter(Boolean);
  const getList = function () {
    if (tokens.length === 0 || typeof findInSubtree !== "function") return [];
    const list = findInSubtree(root, function (el) {
      const cls = (typeof el.getAttribute === "function" ? el.getAttribute("class") : el.className) || "";
      if (!cls) return false;
      const elTokens = String(cls).split(/[ \t\r\n\f]+/);
      for (let i = 0; i < tokens.length; i++) {
        if (elTokens.indexOf(tokens[i]) === -1) return false;
      }
      return true;
    }, false) || [];
    if (typeof upgradeElement === "function") {
      for (let i = 0; i < list.length; i++) list[i] = upgradeElement(list[i]);
    }
    return list;
  };
  return (typeof makeHtmlCollection === "function") ? makeHtmlCollection(getList) : getList();
}

function installShim(global) {
  try {
    Object.defineProperty(global, "__capsurf_nativeEvents", {
      value: {
        Event: Event,
        MouseEvent: MouseEvent,
        PointerEvent: PointerEvent,
        WheelEvent: WheelEvent,
        InputEvent: InputEvent,
        KeyboardEvent: KeyboardEvent,
        MessageEvent: MessageEvent,
        HashChangeEvent: HashChangeEvent,
        PopStateEvent: PopStateEvent,
      },
      configurable: true,
      writable: true,
    });
  } catch (e) {}
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
  try { Object.defineProperty(global, "__capsurf_orig_Node", { value: Node, enumerable: false, writable: false, configurable: false }); } catch (e) {}
  global.CharacterData = CharacterData;
  global.Text = Text;
  global.Comment = Comment;
  global.CDATASection = CDATASection;
  global.ProcessingInstruction = ProcessingInstruction;
  global.DocumentFragment = DocumentFragment;

  // 3. Expose Mutation Observer
  global.MutationObserver = MutationObserver;
  try { Object.defineProperty(global, "__capsurf_orig_MutationObserver", { value: MutationObserver, enumerable: false, writable: false, configurable: false }); } catch (e) {}
  global.MutationRecord = MutationRecord;

  if (typeof global.Proxy === "function") {
    const _OrigProxy = global.Proxy;
    const _proxyTargets = new WeakMap();
    const _WrappedProxy = function (target, handler) {
      const p = new _OrigProxy(target, handler);
      try { _proxyTargets.set(p, target); } catch (e) {}
      return p;
    };
    _WrappedProxy.revocable = function (target, handler) {
      const res = _OrigProxy.revocable(target, handler);
      try { if (res && res.proxy) _proxyTargets.set(res.proxy, target); } catch (e) {}
      return res;
    };
    global.Proxy = _WrappedProxy;
    global.__capsurf_proxy_targets = _proxyTargets;
  }

  global.__capsurf_fmt_hook = function (obj, val) {
    if (global.__capsurf_proxy_targets && global.__capsurf_proxy_targets.has(val)) {
      obj.subtype = "proxy";
      obj.className = "Proxy";
      const tgt = global.__capsurf_proxy_targets.get(val);
      const tgtName = (tgt && tgt.constructor && tgt.constructor.name) ? tgt.constructor.name : "Object";
      obj.description = "Proxy(" + tgtName + ")";
      delete obj.preview;
      return obj;
    }
    if (obj.subtype === "array" && Array.isArray(val)) {
      obj.description = "Array(" + val.length + ")";
      if (val.length === 0) return obj;
    }
    if (!obj.subtype || obj.subtype === "array") {
      try {
        const keys = Object.keys(val);
        const props = [];
        const limit = Math.min(keys.length, 5);
        for (let pi = 0; pi < limit; pi++) {
          const pk = keys[pi];
          const pv = val[pk];
          const pt = typeof pv;
          if (pv === null) props.push({ name: pk, type: "object", subtype: "null", value: "null" });
          else if (pt === "undefined" || pt === "boolean" || pt === "number" || pt === "string") props.push({ name: pk, type: pt, value: String(pv) });
          else props.push({ name: pk, type: pt, value: pt === "function" ? "ƒ" : "Object" });
        }
        obj.preview = { type: "object", description: obj.description, overflow: keys.length > 5, properties: props };
        if (obj.subtype === "array") obj.preview.subtype = "array";
      } catch (e) {}
    }
    return obj;
  };

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
  global.AbstractRange = AbstractRange;
  global.StaticRange = StaticRange;
  global.Range = Range;
  global.__capsurf_upgradeElement = upgradeElement;
  global.Selection = Selection;

  // 7. Expose DOMParser & XMLSerializer
  global.DOMParser = DOMParser;
  global.XMLSerializer = XMLSerializer;

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

    function copyCloneFormState(src, dst, deep) {
      if (!src || !dst) return dst;
      const tag = String(src.localName || src.tagName || "").toLowerCase();
      if (tag === "input") {
        if (src._valueDirty) {
          dst._valueDirty = true;
          dst._value = src._value;
        }
        if (src._checkedDirty) {
          dst._checkedDirty = true;
          dst._checked = src._checked;
        }
      } else if (tag === "textarea") {
        if (src._value !== undefined) {
          dst._value = src._value;
        }
      }
      if (deep) {
        const sk = src.children || src._children || [];
        const dk = dst.children || dst._children || [];
        const len = sk.length < dk.length ? sk.length : dk.length;
        for (let i = 0; i < len; i++) {
          copyCloneFormState(sk[i], upgradeElement(dk[i]), true);
        }
      }
      return dst;
    }

    if (typeof global.Element.prototype.cloneNode === "function") {
      const origElCloneNode = global.Element.prototype.cloneNode;
      global.Element.prototype.cloneNode = markNative(function (deep) {
        const cloned = origElCloneNode.call(this, Boolean(deep));
        if (!cloned) return cloned;
        return copyCloneFormState(this, upgradeElement(cloned), Boolean(deep));
      }, "cloneNode");
    }

    const outerHtmlDesc = Object.getOwnPropertyDescriptor(global.Element.prototype, "outerHTML");
    if (outerHtmlDesc && typeof outerHtmlDesc.get === "function" && !outerHtmlDesc.set) {
      const origOuterGet = outerHtmlDesc.get;
      Object.defineProperty(global.Element.prototype, "outerHTML", {
        get: function () {
          const raw = origOuterGet.call(this);
          if (this._children && this._children.length > 0 && typeof raw === "string") {
            const gt = raw.indexOf(">");
            const closeIdx = raw.lastIndexOf("</");
            if (gt !== -1 && closeIdx > gt) {
              return raw.slice(0, gt + 1) + (this.innerHTML || "") + raw.slice(closeIdx);
            }
          }
          return raw;
        },
        set: function (val) {
          const parent = this.parentNode;
          if (!parent) return;
          const doc = this.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
          const nativeCreate = global.__capsurf_native_createElement;
          const temp = (doc && typeof nativeCreate === "function")
            ? nativeCreate.call(doc, "div")
            : (doc ? doc.createElement("div") : null);
          if (!temp) return;
          temp.innerHTML = String(val == null ? "" : val);
          while (temp.firstChild) {
            const ch = temp.firstChild;
            try { temp.removeChild(ch); } catch (e) {}
            parent.insertBefore(ch, this);
          }
          parent.removeChild(this);
        },
        configurable: true,
        enumerable: true,
      });
    }

    global.Element.prototype.animate = markNative(function (keyframes, options) {
      return animateElement(this, keyframes, options);
    }, "animate");

    global.Element.prototype.getAnimations = markNative(function () {
      return animationsFor(this);
    }, "getAnimations");

    global.Element.prototype.getContext = markNative(function (type) {
      if (typeof upgradeElement === "function") {
        upgradeElement(this);
      }
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

    global.Element.prototype.getElementsByTagName = markNative(function(qualifiedName) {
        if (arguments.length === 0) throw new TypeError("getElementsByTagName: 1 argument required");
        return _makeTagCollection(this, qualifiedName);
    }, "getElementsByTagName");

    global.Element.prototype.getElementsByTagNameNS = markNative(function(namespace, localName) {
        if (arguments.length < 2) throw new TypeError("getElementsByTagNameNS: 2 arguments required");
        return _makeTagNSCollection(this, namespace, localName);
    }, "getElementsByTagNameNS");

    global.Element.prototype.getElementsByClassName = markNative(function(classNames) {
        if (arguments.length === 0) throw new TypeError("getElementsByClassName: 1 argument required");
        return _makeClassCollection(this, classNames);
    }, "getElementsByClassName");

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

  const upgradedElementsSet = new WeakSet();
  const elementNidMap = new WeakMap();
  if (global.Node && global.Node.prototype && !Object.getOwnPropertyDescriptor(global.Node.prototype, "_nid")) {
    Object.defineProperty(global.Node.prototype, "_nid", {
      get: function () { return elementNidMap.get(this); },
      set: function (v) { elementNidMap.set(this, v); },
      configurable: true,
    });
  }
  function hideInternalNid(el) {
    if (!el || typeof el !== "object") return;
    if (Object.prototype.hasOwnProperty.call(el, "_nid")) {
      const n = el._nid;
      if (typeof n === "number") {
        elementNidMap.set(el, n);
        if (el._id === undefined) el._id = n;
      }
      try { delete el._nid; } catch (e) {}
    }
  }

  function upgradeElement(el) {
    if (!el || typeof el !== "object") return el;
    if (typeof isParsedNode === "function" && isParsedNode(el)) return el;
    hideInternalNid(el);
    if (el.nodeType === 3) {
      if (global.Text && global.Text.prototype && !global.Text.prototype.isPrototypeOf(el)) {
        if (el._data === undefined) {
          try {
            const t = el.textContent;
            if (typeof t === "string") el._data = t;
          } catch (e) {}
        }
        Object.setPrototypeOf(el, global.Text.prototype);
      }
      return el;
    }
    if (el.nodeType === 8) {
      if (global.Comment && global.Comment.prototype && !global.Comment.prototype.isPrototypeOf(el)) {
        if (el._data === undefined) {
          try {
            const t = el.textContent;
            if (typeof t === "string") el._data = t;
          } catch (e) {}
        }
        Object.setPrototypeOf(el, global.Comment.prototype);
      }
      return el;
    }
    if (el.nodeType !== undefined && el.nodeType !== 1) {
      return el;
    }
    const nidForCe = typeof el._nid === "number" ? el._nid : (typeof el._id === "number" ? el._id : -1);
    if (nidForCe > 0 && global.__capsurf_upgradedCustomNodeIds && global.__capsurf_upgradedCustomNodeIds.has(nidForCe)) {
      const CeCtor = global.__capsurf_upgradedCustomNodeIds.get(nidForCe);
      if (CeCtor && CeCtor.prototype && !CeCtor.prototype.isPrototypeOf(el)) {
        Object.setPrototypeOf(el, CeCtor.prototype);
      }
      upgradedElementsSet.add(el);
      return el;
    }
    if (upgradedElementsSet.has(el)) return el;
    upgradedElementsSet.add(el);
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
      case "form": Object.setPrototypeOf(el, global.__capsurf_formNamedPropsProxy || global.HTMLFormElement.prototype); break;
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
      case "frame":
      case "iframe": Object.setPrototypeOf(el, global.HTMLIFrameElement.prototype); break;
      case "template": Object.setPrototypeOf(el, global.HTMLTemplateElement.prototype); break;
      case "slot": Object.setPrototypeOf(el, (global.HTMLSlotElement || global.HTMLElement).prototype); break;
      case "details": Object.setPrototypeOf(el, (global.HTMLDetailsElement || global.HTMLElement).prototype); break;
      case "dialog": Object.setPrototypeOf(el, global.HTMLDialogElement ? global.HTMLDialogElement.prototype : global.HTMLElement.prototype); break;
      case "style": Object.setPrototypeOf(el, (global.HTMLStyleElement || global.HTMLElement).prototype); break;
      case "link": Object.setPrototypeOf(el, (global.HTMLLinkElement || global.HTMLElement).prototype); break;
      case "base": Object.setPrototypeOf(el, (global.HTMLBaseElement || global.HTMLElement).prototype); break;
      case "video": Object.setPrototypeOf(el, (global.HTMLVideoElement || global.HTMLMediaElement || global.HTMLElement).prototype); break;
      case "audio": Object.setPrototypeOf(el, (global.HTMLAudioElement || global.HTMLMediaElement || global.HTMLElement).prototype); break;
      case "track": Object.setPrototypeOf(el, (global.HTMLTrackElement || global.HTMLElement).prototype); break;
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

    if (typeof docProto.getElementsByTagName !== "function") {
      docProto.getElementsByTagName = markNative(function (qualifiedName) {
        if (arguments.length === 0) throw new TypeError("getElementsByTagName: 1 argument required");
        return _makeTagCollection(this, qualifiedName);
      }, "getElementsByTagName");
    }
    if (typeof docProto.getElementsByTagNameNS !== "function") {
      docProto.getElementsByTagNameNS = markNative(function (namespace, localName) {
        if (arguments.length < 2) throw new TypeError("getElementsByTagNameNS: 2 arguments required");
        return _makeTagNSCollection(this, namespace, localName);
      }, "getElementsByTagNameNS");
    }
    if (typeof docProto.getElementsByClassName !== "function") {
      docProto.getElementsByClassName = markNative(function (classNames) {
        if (arguments.length === 0) throw new TypeError("getElementsByClassName: 1 argument required");
        return _makeClassCollection(this, classNames);
      }, "getElementsByClassName");
    }

    // Upgrade document.body / document.head / document.documentElement so HTMLBodyElement/HTMLElement
    // prototype getters (e.g. innerText) are always available even before querySelector is called.
    const docElemProps = ["body", "head", "documentElement"];
    for (let dpIdx = 0; dpIdx < docElemProps.length; dpIdx++) {
      const propName = docElemProps[dpIdx];
      const ownDesc = Object.getOwnPropertyDescriptor(docProto, propName);
      if (ownDesc && typeof ownDesc.get === "function") {
        const origGet = ownDesc.get;
        const origSet = ownDesc.set;
        Object.defineProperty(docProto, propName, {
          get: function () {
            return upgradeElement(origGet.call(this));
          },
          set: origSet,
          configurable: true,
          enumerable: ownDesc.enumerable !== false,
        });
      }
    }

    // Include document.doctype in document.firstChild and document.childNodes per DOM spec
    const origDocFirstChildDesc = Object.getOwnPropertyDescriptor(docProto, "firstChild")
      || Object.getOwnPropertyDescriptor(Node.prototype, "firstChild");
    const origDocFirstChildGet = origDocFirstChildDesc && origDocFirstChildDesc.get;
    Object.defineProperty(docProto, "firstChild", {
      get: function () {
        const orig = origDocFirstChildGet ? origDocFirstChildGet.call(this) : null;
        const dt = this.doctype;
        if (dt && (!orig || orig.nodeType !== 10)) {
          return dt;
        }
        return orig;
      },
      configurable: true,
      enumerable: true,
    });

    const origDocChildNodesDesc = Object.getOwnPropertyDescriptor(docProto, "childNodes")
      || Object.getOwnPropertyDescriptor(Node.prototype, "childNodes");
    const origDocChildNodesGet = origDocChildNodesDesc && origDocChildNodesDesc.get;
    Object.defineProperty(docProto, "childNodes", {
      get: function () {
        const orig = origDocChildNodesGet ? origDocChildNodesGet.call(this) : null;
        const arr = orig ? Array.from(orig) : [];
        const dt = this.doctype;
        if (dt && (arr.length === 0 || arr[0].nodeType !== 10)) {
          arr.unshift(dt);
        }
        return new NodeList(arr);
      },
      configurable: true,
      enumerable: true,
    });

    if (docProto.createElement) {
      const origCreateElement = docProto.createElement;
      docProto.createElement = function (tagName) {
        let el = origCreateElement.call(this, tagName);
        if (el) {
          const tag = String(tagName).toLowerCase();
          const registry = global.customElements || (typeof customElements !== "undefined" ? customElements : null);
          const Ctor = (registry && registry.get) ? registry.get(tag) : null;
          if (Ctor && typeof Ctor === "function" && !Ctor.prototype.isPrototypeOf(el)) {
            upgradedElementsSet.add(el);
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
      global.__capsurf_native_createElement = docProto.createElement;
      if (typeof docProto.createDocumentFragment === "function") {
        global.__capsurf_native_createDocumentFragment = docProto.createDocumentFragment;
      }
      if (typeof docProto.createTextNode === "function") {
        global.__capsurf_native_createTextNode = docProto.createTextNode;
      }
      if (typeof docProto.createComment === "function") {
        global.__capsurf_native_createComment = docProto.createComment;
      }
      if (typeof docProto.createEvent !== "function") {
        docProto.createEvent = markNative(function (iface) {
          if (arguments.length < 1) {
            throw new TypeError("Failed to execute 'createEvent' on 'Document': 1 argument required, but only 0 present.");
          }
          const key = String(iface).toLowerCase();
          const EventCtor = global.Event;
          if (key === "beforeunloadevent") {
            const C = global.BeforeUnloadEvent || EventCtor;
            return new C("");
          }
          if (key === "compositionevent" || key === "textevent") {
            const C = global.CompositionEvent || global.UIEvent || EventCtor;
            return new C("");
          }
          if (key === "customevent") {
            const C = global.CustomEvent || EventCtor;
            return new C("");
          }
          if (key === "devicemotionevent" || key === "deviceorientationevent" || key === "event" || key === "events" || key === "htmlevents" || key === "svgevents") {
            return new EventCtor("");
          }
          if (key === "dragevent") {
            const C = global.DragEvent || global.MouseEvent || EventCtor;
            return new C("");
          }
          if (key === "focusevent") {
            const C = global.FocusEvent || global.UIEvent || EventCtor;
            return new C("");
          }
          if (key === "hashchangeevent") {
            const C = global.HashChangeEvent || EventCtor;
            return new C("");
          }
          if (key === "keyboardevent" || key === "keyevents") {
            const C = global.KeyboardEvent || global.UIEvent || EventCtor;
            return new C("");
          }
          if (key === "messageevent") {
            const C = global.MessageEvent || EventCtor;
            return new C("");
          }
          if (key === "mouseevent" || key === "mouseevents") {
            const C = global.MouseEvent || global.UIEvent || EventCtor;
            return new C("");
          }
          if (key === "storageevent") {
            const C = global.StorageEvent || EventCtor;
            return new C("");
          }
          if (key === "touchevent" || key === "uievent" || key === "uievents") {
            const C = global.UIEvent || EventCtor;
            return new C("");
          }
          throw new DOMException("The operation is not supported.", "NotSupportedError");
        }, "createEvent");
      }
      if (typeof docProto.createCDATASection !== "function") {
        docProto.createCDATASection = markNative(function (data) {
          if (arguments.length < 1) {
            throw new TypeError("Failed to execute 'createCDATASection' on 'Document': 1 argument required, but only 0 present.");
          }
          const s = String(data);
          if (s.includes("]]>")) {
            throw new DOMException("String contains an invalid character", "InvalidCharacterError");
          }
          const isHtml = this.contentType === "text/html" || (!this.contentType && this === global.document);
          if (isHtml) {
            throw new DOMException("This operation is not supported for HTML documents.", "NotSupportedError");
          }
          const node = new CDATASection(s);
          node.ownerDocument = this;
          return node;
        }, "createCDATASection");
      }
      if (typeof docProto.createProcessingInstruction !== "function") {
        docProto.createProcessingInstruction = markNative(function (target, data) {
          if (arguments.length < 2) {
            throw new TypeError("Failed to execute 'createProcessingInstruction' on 'Document': 2 arguments required.");
          }
          const t = String(target);
          const d = String(data);
          if (!/^[A-Za-z_\xC0-\uFFFF][A-Za-z0-9._:-\xC0-\uFFFF]*$/.test(t) || d.includes("?>")) {
            throw new DOMException("String contains an invalid character", "InvalidCharacterError");
          }
          const node = new ProcessingInstruction(t, d);
          node.ownerDocument = this;
          return node;
        }, "createProcessingInstruction");
      }
      if (global.document) {
        global.document.createElement = docProto.createElement;
        if (typeof global.document.createEvent !== "function") {
          global.document.createEvent = docProto.createEvent;
        }
        if (typeof global.document.createCDATASection !== "function") {
          global.document.createCDATASection = docProto.createCDATASection;
        }
        if (typeof global.document.createProcessingInstruction !== "function") {
          global.document.createProcessingInstruction = docProto.createProcessingInstruction;
        }
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
        const nsVal = (ns !== undefined && ns !== null && String(ns) !== "") ? String(ns) : null;
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
          upgradedElementsSet.delete(el);
          const nonHtmlNid = typeof el._nid === "number" ? el._nid : (typeof el._id === "number" ? el._id : -1);
          if (nonHtmlNid > 0 && global.__capsurf_upgradedCustomNodeIds) {
            global.__capsurf_upgradedCustomNodeIds.delete(nonHtmlNid);
          }
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
          upgradedElementsSet.add(el);
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
    global.__capsurf_native_createElementNS = docProto.createElementNS;
    if (global.document) {
      global.document.createElementNS = docProto.createElementNS;
    }

    if (typeof docProto.adoptNode !== "function") {
      docProto.adoptNode = markNative(function (node) {
        if (!node || typeof node !== "object") return node;
        if (node.parentNode && typeof node.parentNode.removeChild === "function") {
          try { node.parentNode.removeChild(node); } catch (e) {}
        }
        try {
          Object.defineProperty(node, "ownerDocument", { value: this, writable: true, configurable: true });
        } catch (e) {
          try { node.ownerDocument = this; } catch (e2) {}
        }
        return node;
      }, "adoptNode");
      if (global.document && !global.document.adoptNode) {
        global.document.adoptNode = docProto.adoptNode;
      }
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

    const checkDirSelector = (selStr) => {
      const str = String(selStr || "");
      const re = /:dir\(([^)]*)\)/gi;
      let m;
      while ((m = re.exec(str)) !== null) {
        if (!/^\s*[a-zA-Z_-][a-zA-Z0-9_-]*\s*$/.test(m[1])) {
          throw new DOMException("The provided selector is invalid.", "SyntaxError");
        }
      }
    };
    if (docProto.querySelector) {
      const origQuerySelector = docProto.querySelector;
      docProto.querySelector = function (sel) {
        checkDirSelector(sel);
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
        checkDirSelector(sel);
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
      const writtenQueue = [];
      let writtenRunning = false;
      let writtenInWriter = false;

      function flushWrittenQueue() {
        if (writtenRunning) return;
        if (writtenQueue.length === 0) {
          global.__capsurf_written_script_queue_pending = false;
          if (typeof global.__capsurf_flush_deferred_lifecycle === "function") {
            global.__capsurf_flush_deferred_lifecycle();
          }
          return;
        }
        const head = writtenQueue[0];
        if (head.kind === "external") {
          if (!head.ready) {
            return;
          }
          writtenQueue.shift();
          writtenRunning = true;
          const prevInWriter = writtenInWriter;
          writtenInWriter = true;
          const savedQueue = writtenQueue.splice(0, writtenQueue.length);
          try {
            if (head.code) {
              (1, eval)(head.code);
            }
          } catch (e) {
            if (typeof console !== "undefined" && console.error) console.error(e);
          } finally {
            for (let si = 0; si < savedQueue.length; si++) {
              writtenQueue.push(savedQueue[si]);
            }
            writtenInWriter = prevInWriter;
            writtenRunning = false;
          }
          flushWrittenQueue();
          return;
        }
        if (head.kind === "inline") {
          writtenQueue.shift();
          writtenRunning = true;
          const prevInWriter = writtenInWriter;
          writtenInWriter = true;
          const savedQueue = writtenQueue.splice(0, writtenQueue.length);
          try {
            if (head.code) {
              (1, eval)(head.code);
            }
          } catch (e) {
            if (typeof console !== "undefined" && console.error) console.error(e);
          } finally {
            for (let si = 0; si < savedQueue.length; si++) {
              writtenQueue.push(savedQueue[si]);
            }
            writtenInWriter = prevInWriter;
            writtenRunning = false;
          }
          flushWrittenQueue();
        }
      }

      function enqueueWrittenScripts(markup, doc) {
        const scriptRe = /<script\b([^>]*)>([\s\S]*?)<\/script>/gi;
        const items = [];
        let m;
        while ((m = scriptRe.exec(markup)) !== null) {
          const attrs = m[1] || "";
          const body = m[2] || "";
          const srcMatch = /\bsrc\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s>]+))/i.exec(attrs);
          const src = srcMatch ? (srcMatch[1] !== undefined ? srcMatch[1] : (srcMatch[2] !== undefined ? srcMatch[2] : srcMatch[3])) : null;
          if (src) {
            items.push({ kind: "external", src: src, ready: false, code: "" });
          } else {
            items.push({ kind: "inline", code: body });
          }
        }
        if (items.length === 0) return false;

        for (let i = 0; i < items.length; i++) {
          const item = items[i];
          if (item.kind === "inline" && writtenQueue.length === 0) {
            try {
              if (item.code) (1, eval)(item.code);
            } catch (e) {
              if (typeof console !== "undefined" && console.error) console.error(e);
            }
          } else {
            writtenQueue.push(item);
            global.__capsurf_written_script_queue_pending = true;
            const nativeFetch = (typeof global !== "undefined" && global.__capsurf_native_fetch) ||
              (typeof globalThis !== "undefined" && globalThis.__capsurf_native_fetch) ||
              (typeof fetch === "function" ? fetch : null);
            if (item.kind === "external" && typeof nativeFetch === "function") {
              (function (entry) {
                nativeFetch(entry.src)
                  .then(function (res) { return res.text(); })
                  .then(function (txt) {
                    entry.code = txt;
                    entry.ready = true;
                    flushWrittenQueue();
                  })
                  .catch(function () {
                    entry.code = "";
                    entry.ready = true;
                    flushWrittenQueue();
                  });
              })(item);
            }
          }
        }
        if (!writtenInWriter) {
          flushWrittenQueue();
        }
        return true;
      }

      docProto.write = markNative(function () {
        let markup = "";
        for (let i = 0; i < arguments.length; i++) {
          markup += String(arguments[i]);
        }
        const hadScripts = enqueueWrittenScripts(markup, this);
        const nonScriptMarkup = hadScripts ? markup.replace(/<script\b[^>]*>[\s\S]*?<\/script>/gi, "") : markup;
        if (nonScriptMarkup) {
          const target = this.body || this.documentElement;
          if (target && typeof this.createElement === "function") {
            const temp = this.createElement("div");
            temp.innerHTML = nonScriptMarkup;
            while (temp.firstChild) {
              const ch = temp.firstChild;
              try { temp.removeChild(ch); } catch (e) {}
              target.appendChild(ch);
            }
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

    docProto.createAttribute = markNative(function (localName) {
      const name = String(localName);
      const isHtmlDoc = !this.contentType || String(this.contentType).toLowerCase() === "text/html";
      const finalName = isHtmlDoc ? name.toLowerCase() : name;
      const attr = new Attr(finalName, "", null, null, finalName, null);
      attr.ownerDocument = this;
      return attr;
    }, "createAttribute");

    docProto.createAttributeNS = markNative(function (namespace, qualifiedName) {
      const qn = String(qualifiedName);
      const ns = namespace ? String(namespace) : null;
      const colon = qn.indexOf(":");
      const prefix = colon !== -1 ? qn.slice(0, colon) : null;
      const localName = colon !== -1 ? qn.slice(colon + 1) : qn;
      const attr = new Attr(qn, "", null, ns, localName, prefix);
      attr.ownerDocument = this;
      return attr;
    }, "createAttributeNS");

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

    docProto.hasFocus = markNative(function () {
      return true;
    }, "hasFocus");
    if (global.document && typeof global.document.hasFocus !== "function") {
      global.document.hasFocus = docProto.hasFocus;
    }

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
        if (typeof replaceChildrenImpl === "function") {
          replaceChildrenImpl(this, nodes);
          return;
        }
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

    Object.defineProperty(docProto, "activeElement", {
      get: function () {
        const ae = this._activeElement;
        if (ae && ae.isConnected !== false) {
          let cur = ae;
          while (cur && typeof cur.getRootNode === "function") {
            const rn = cur.getRootNode();
            if (rn && rn !== this && rn.host) {
              cur = rn.host;
            } else {
              break;
            }
          }
          return cur;
        }
        return this.body || this.documentElement || null;
      },
      set: function (v) {
        this._activeElement = v || null;
      },
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
    const docLocDesc = Object.getOwnPropertyDescriptor(global.document, "location");
    if (!docLocDesc || docLocDesc.configurable) {
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
      if (!docProto.elementsFromPoint) {
        docProto.elementsFromPoint = markNative(function (x, y) {
          const out = [];
          const top = typeof this.elementFromPoint === "function" ? this.elementFromPoint(x, y) : null;
          let cur = top;
          while (cur && cur.nodeType === 1) {
            out.push(cur);
            cur = cur.parentElement;
          }
          return out;
        }, "elementsFromPoint");
      }
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
        const InputCtor = (global.__capsurf_nativeEvents && global.__capsurf_nativeEvents.InputEvent) || (typeof InputEvent !== "undefined" ? InputEvent : null);
        const inputEvt = InputCtor
          ? new InputCtor("input", { bubbles: true, cancelable: false })
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

  // Ensure HTML*Element constructor inheritance chain (WebIDL interface object prototype)
  if (typeof global.HTMLElement === "function") {
    if (typeof global.Element === "function") {
      try { Object.setPrototypeOf(global.HTMLElement, global.Element); } catch (e) {}
    }
    const keys = Object.keys(global);
    for (let i = 0; i < keys.length; i++) {
      const k = keys[i];
      if (k !== "HTMLElement" && /^HTML.+Element$/.test(k)) {
        const Ctor = global[k];
        if (typeof Ctor === "function" && Ctor.prototype && global.HTMLElement.prototype.isPrototypeOf(Ctor.prototype)) {
          try { Object.setPrototypeOf(Ctor, global.HTMLElement); } catch (e) {}
        }
      }
    }
  }

  // Expose HTMLDocument legacy alias per WHATWG HTML spec
  if (typeof global.Document === "function" && !global.HTMLDocument) {
    global.HTMLDocument = global.Document;
    if (typeof globalThis !== "undefined") globalThis.HTMLDocument = global.Document;
    if (typeof window !== "undefined") window.HTMLDocument = global.Document;
  }

  // W3C Editing API: document.execCommand / queryCommand*
  const editingStateMap = new WeakMap();
  function getEditingState(doc) {
    let st = editingStateMap.get(doc);
    if (!st) {
      st = { styleWithCSS: false, defaultParagraphSeparator: "div" };
      editingStateMap.set(doc, st);
    }
    return st;
  }

  const SUPPORTED_EDITING_COMMANDS = new Set([
    "bold", "italic", "underline", "strikethrough",
    "subscript", "superscript",
    "stylewithcss", "defaultparagraphseparator",
    "inserttext", "inserthtml", "delete", "forwarddelete",
    "selectall", "removeformat", "copy",
  ]);

  function isNodeEditable(node) {
    let cur = node && node.nodeType === 1 ? node : (node ? node.parentNode : null);
    while (cur) {
      if (cur.nodeType === 1 && typeof cur.getAttribute === "function") {
        const ce = cur.getAttribute("contenteditable");
        if (ce !== null) {
          const v = String(ce).toLowerCase();
          if (v === "false") return false;
          return true;
        }
      }
      if (cur.nodeType === 9 && String(cur.designMode || "").toLowerCase() === "on") {
        return true;
      }
      cur = cur.parentNode;
    }
    return false;
  }

  function getActiveEditingRange(doc) {
    const sel = (doc && typeof doc.getSelection === "function")
      ? doc.getSelection()
      : (typeof global.getSelection === "function" ? global.getSelection() : null);
    if (!sel || !sel.rangeCount || sel.rangeCount <= 0) return null;
    try {
      return sel.getRangeAt(0);
    } catch (e) {
      return null;
    }
  }

  function isNodeStyledForCommand(node, cmd) {
    let cur = node && node.nodeType === 1 ? node : (node ? node.parentNode : null);
    while (cur && cur.nodeType === 1) {
      const tag = String(cur.localName || cur.tagName || "").toLowerCase();
      if (cmd === "bold") {
        if (tag === "b" || tag === "strong") return true;
        if (cur.style && cur.style.fontWeight) {
          const fw = String(cur.style.fontWeight).toLowerCase();
          if (fw === "bold" || Number(fw) >= 600) return true;
        }
      } else if (cmd === "italic") {
        if (tag === "i" || tag === "em") return true;
        if (cur.style && String(cur.style.fontStyle || "").toLowerCase() === "italic") return true;
      } else if (cmd === "underline") {
        if (tag === "u") return true;
        if (cur.style && String(cur.style.textDecoration || "").toLowerCase().includes("underline")) return true;
      } else if (cmd === "strikethrough") {
        if (tag === "s" || tag === "strike" || tag === "del") return true;
      }
      cur = cur.parentNode;
    }
    return false;
  }

  function collectRangeTextNodes(range) {
    const out = [];
    if (!range) return out;
    const sc = range.startContainer;
    const ec = range.endContainer;
    const so = range.startOffset;
    const eo = range.endOffset;
    if (sc === ec && sc && sc.nodeType === 3) {
      if (eo > so) out.push(sc);
      return out;
    }
    const ancestor = range.commonAncestorContainer || (sc ? sc.parentNode : null);
    if (!ancestor) return out;
    let started = false;
    let done = false;
    function walk(node) {
      if (!node || done) return;
      if (node === sc && node.nodeType === 3) {
        started = true;
        if ((node.data || node.textContent || "").length > so) out.push(node);
        if (node === ec) { done = true; return; }
        return;
      }
      if (node === ec && node.nodeType === 3) {
        if (eo > 0) out.push(node);
        done = true;
        return;
      }
      if (node.nodeType === 3) {
        if (started && (node.data || node.textContent || "").length > 0) out.push(node);
        return;
      }
      const kids = node.childNodes && node.childNodes.length > 0
        ? Array.from(node.childNodes)
        : (node._children && node._children.length > 0 ? node._children.slice() : []);
      if (kids.length === 0) {
        let ch = node.firstChild;
        while (ch) { kids.push(ch); ch = ch.nextSibling; }
      }
      if (kids.length === 0 && node.nodeType === 1 && started && (node.textContent || "").length > 0) {
        out.push(node);
      }
      for (let i = 0; i < kids.length; i++) {
        if (node === sc && i === so) started = true;
        if (node === ec && i === eo) { done = true; return; }
        walk(kids[i]);
        if (done) return;
      }
      if (node === sc && so >= kids.length) started = true;
      if (node === ec && eo >= kids.length) done = true;
    }
    walk(ancestor);
    return out;
  }

  function installEditingCommands(target) {
    if (!target) return;
    target.queryCommandSupported = markNative(function (command) {
      const cmd = String(command || "").toLowerCase();
      return SUPPORTED_EDITING_COMMANDS.has(cmd);
    }, "queryCommandSupported");

    target.queryCommandEnabled = markNative(function (command) {
      const cmd = String(command || "").toLowerCase();
      if (!SUPPORTED_EDITING_COMMANDS.has(cmd)) return false;
      if (cmd === "stylewithcss" || cmd === "defaultparagraphseparator") return true;
      const range = getActiveEditingRange(this);
      if (!range) return false;
      return isNodeEditable(range.startContainer) && isNodeEditable(range.endContainer);
    }, "queryCommandEnabled");

    target.queryCommandState = markNative(function (command) {
      const cmd = String(command || "").toLowerCase();
      const st = getEditingState(this);
      if (cmd === "stylewithcss") return st.styleWithCSS;
      if (cmd === "bold" || cmd === "italic" || cmd === "underline" || cmd === "strikethrough") {
        const range = getActiveEditingRange(this);
        if (!range) return false;
        const texts = collectRangeTextNodes(range);
        if (texts.length === 0) {
          return isNodeStyledForCommand(range.startContainer, cmd);
        }
        for (let i = 0; i < texts.length; i++) {
          if (!isNodeStyledForCommand(texts[i], cmd)) return false;
        }
        return true;
      }
      return false;
    }, "queryCommandState");

    target.queryCommandIndeterm = markNative(function (command) {
      const cmd = String(command || "").toLowerCase();
      if (cmd === "bold" || cmd === "italic" || cmd === "underline" || cmd === "strikethrough") {
        const range = getActiveEditingRange(this);
        if (!range) return false;
        const texts = collectRangeTextNodes(range);
        if (texts.length < 2) return false;
        let hasTrue = false;
        let hasFalse = false;
        for (let i = 0; i < texts.length; i++) {
          if (isNodeStyledForCommand(texts[i], cmd)) hasTrue = true;
          else hasFalse = true;
          if (hasTrue && hasFalse) return true;
        }
      }
      return false;
    }, "queryCommandIndeterm");

    target.queryCommandValue = markNative(function (command) {
      const cmd = String(command || "").toLowerCase();
      const st = getEditingState(this);
      if (cmd === "defaultparagraphseparator") return st.defaultParagraphSeparator;
      if (cmd === "stylewithcss") return "";
      return "";
    }, "queryCommandValue");

    target.execCommand = markNative(function (command, showUI, value) {
      const cmd = String(command || "").toLowerCase();
      const st = getEditingState(this);
      if (!SUPPORTED_EDITING_COMMANDS.has(cmd)) return false;

      if (cmd === "stylewithcss") {
        const v = value !== undefined && value !== null ? String(value) : "";
        st.styleWithCSS = v.toLowerCase() !== "false";
        return true;
      }
      if (cmd === "defaultparagraphseparator") {
        const v = value !== undefined && value !== null ? String(value).toLowerCase() : "";
        if (v === "p" || v === "div") {
          st.defaultParagraphSeparator = v;
          return true;
        }
        return false;
      }
      if (cmd === "selectall") {
        const sel = typeof this.getSelection === "function" ? this.getSelection() : (typeof global.getSelection === "function" ? global.getSelection() : null);
        const root = (this.activeElement && isNodeEditable(this.activeElement)) ? this.activeElement : (this.body || this.documentElement);
        if (sel && root && typeof this.createRange === "function") {
          const r = this.createRange();
          r.selectNodeContents(root);
          sel.removeAllRanges();
          sel.addRange(r);
          return true;
        }
        return false;
      }
      if (cmd === "copy") {
        const sel = typeof this.getSelection === "function" ? this.getSelection() : (typeof global.getSelection === "function" ? global.getSelection() : null);
        globalThis.__capsurf_clipboard_text = sel ? String(sel.toString() || "") : "";
        return true;
      }

      const range = getActiveEditingRange(this);
      if (!range || !isNodeEditable(range.startContainer) || !isNodeEditable(range.endContainer)) {
        return false;
      }

      const tagMap = { bold: "b", italic: "i", underline: "u", strikethrough: "strike", subscript: "sub", superscript: "sup" };
      if (tagMap[cmd]) {
        const doc = this && this.createElement ? this : global.document;
        if (range.startContainer === range.endContainer && range.startContainer.nodeType === 3) {
          const textNode = range.startContainer;
          const fullText = textNode.data !== undefined ? String(textNode.data) : String(textNode.textContent || "");
          const so = Math.max(0, Math.min(range.startOffset, fullText.length));
          const eo = Math.max(so, Math.min(range.endOffset, fullText.length));
          if (eo > so && textNode.parentNode) {
            const before = fullText.slice(0, so);
            const selected = fullText.slice(so, eo);
            const after = fullText.slice(eo);
            const parent = textNode.parentNode;
            const wrapper = doc.createElement(tagMap[cmd]);
            wrapper.textContent = selected;
            if (before) {
              parent.insertBefore(doc.createTextNode(before), textNode);
            }
            parent.insertBefore(wrapper, textNode);
            if (after) {
              parent.insertBefore(doc.createTextNode(after), textNode);
            }
            parent.removeChild(textNode);
            const innerNode = wrapper.firstChild || wrapper;
            range.setStart(innerNode, 0);
            range.setEnd(innerNode, innerNode.nodeType === 3 ? selected.length : (wrapper.childNodes ? wrapper.childNodes.length : 1));
            return true;
          }
        }
        const texts = collectRangeTextNodes(range);
        for (let i = 0; i < texts.length; i++) {
          const t = texts[i];
          if (!isNodeStyledForCommand(t, cmd) && t.parentNode) {
            const wrapper = doc.createElement(tagMap[cmd]);
            t.parentNode.insertBefore(wrapper, t);
            wrapper.appendChild(t);
          }
        }
        return true;
      }
      return false;
    }, "execCommand");
  }

  if (docProto) {
    installEditingCommands(docProto);
  }
  if (global.document && typeof global.document.execCommand !== "function") {
    installEditingCommands(global.document);
  }

  // Wrap Document.prototype.implementation getter so createHTMLDocument returns a detached document
  // whose elements properly report ownerDocument === doc and resolve getElementById within doc.
  if (docProto) {
    const implDesc = Object.getOwnPropertyDescriptor(docProto, "implementation");
    if (implDesc && typeof implDesc.get === "function") {
      const origImplGet = implDesc.get;
      const implCache = new WeakMap();
      Object.defineProperty(docProto, "implementation", {
        get: function () {
          let cached = implCache.get(this);
          if (cached) return cached;
          const impl = origImplGet.call(this);
          const implProto = Object.getPrototypeOf(impl);
          if (impl && typeof impl.createHTMLDocument === "function" && implProto && !implProto.__capsurfWrapped) {
            implProto.__capsurfWrapped = true;
            const origCreateHTMLDoc = impl.createHTMLDocument;
            try { delete impl.createHTMLDocument; } catch (e) {}
            implProto.createHTMLDocument = markNative(function (title) {
              const doc = origCreateHTMLDoc.apply(this, arguments);
              if (doc && typeof doc === "object") {
                try {
                  Object.defineProperty(doc, "location", { value: null, configurable: true });
                } catch (e) {
                  try { doc.location = null; } catch (e2) {}
                }
                const setDocOwner = function (node) {
                  if (!node || typeof node !== "object") return;
                  try {
                    Object.defineProperty(node, "ownerDocument", { value: doc, writable: true, configurable: true });
                  } catch (e) {
                    try { node.ownerDocument = doc; } catch (e2) {}
                  }
                };
                const setParent = function (node, parent) {
                  if (!node || typeof node !== "object" || !parent) return;
                  try {
                    Object.defineProperty(node, "parentNode", { value: parent, writable: true, configurable: true });
                  } catch (e) {
                    try { node.parentNode = parent; } catch (e2) {}
                  }
                };
                const cachedBody = doc.body ? upgradeElement(doc.body) : null;
                const cachedHead = doc.head ? upgradeElement(doc.head) : null;
                const cachedDocEl = doc.documentElement ? upgradeElement(doc.documentElement) : null;
                if (cachedDocEl) {
                  setDocOwner(cachedDocEl);
                  setParent(cachedDocEl, doc);
                  Object.defineProperty(doc, "documentElement", { value: cachedDocEl, configurable: true });
                }
                if (cachedHead) {
                  setDocOwner(cachedHead);
                  setParent(cachedHead, cachedDocEl || doc);
                  Object.defineProperty(doc, "head", { value: cachedHead, configurable: true });
                }
                if (cachedBody) {
                  setDocOwner(cachedBody);
                  setParent(cachedBody, cachedDocEl || doc);
                  Object.defineProperty(doc, "body", {
                    get: function () {
                      const de = this.documentElement;
                      if (de && de.children) {
                        for (let i = 0; i < de.children.length; i++) {
                          const t = (de.children[i].tagName || "").toUpperCase();
                          if (t === "BODY" || t === "FRAMESET") return upgradeElement(de.children[i]);
                        }
                      }
                      return cachedBody;
                    },
                    configurable: true
                  });
                }
                const origDocCreateElement = doc.createElement;
                if (typeof origDocCreateElement === "function") {
                  doc.createElement = function (tagName) {
                    const el = origDocCreateElement.call(this, tagName);
                    if (el) {
                      const upgraded = upgradeElement(el);
                      setDocOwner(upgraded);
                      return upgraded;
                    }
                    return el;
                  };
                }
                doc.getElementById = function (id) {
                  const targetId = String(id);
                  let found = null;
                  function walk(n) {
                    if (!n || found) return;
                    if (n.nodeType === 1 && typeof n.getAttribute === "function" && n.getAttribute("id") === targetId) {
                      found = upgradeElement(n);
                      return;
                    }
                    let c = n.firstChild;
                    while (c && !found) {
                      walk(c);
                      c = c.nextSibling;
                    }
                  }
                  walk(cachedDocEl || cachedBody || doc);
                  return found;
                };
              }
              try {
                Object.defineProperty(doc, "readyState", { value: "complete", writable: true, configurable: true });
              } catch (e) {}
              return doc;
            }, "createHTMLDocument");
          }
          if (impl && typeof impl.createDocument !== "function" && implProto) {
            const createHtmlDocFn = implProto.createHTMLDocument || impl.createHTMLDocument;
            implProto.createDocument = markNative(function (namespaceURI, qualifiedNameStr, _doctype) {
              const doc = typeof createHtmlDocFn === "function" ? createHtmlDocFn.call(this, "") : Object.create(docProto);
              if (doc && typeof doc === "object") {
                let rootEl = null;
                if (qualifiedNameStr) {
                  const ns = namespaceURI !== undefined && namespaceURI !== null ? String(namespaceURI) : null;
                  const nativeNS = global.__capsurf_native_createElementNS || doc.createElementNS;
                  if (typeof nativeNS === "function") {
                    rootEl = nativeNS.call(doc, ns, String(qualifiedNameStr));
                  }
                  if (rootEl) {
                    try {
                      Object.defineProperty(rootEl, "ownerDocument", { value: doc, writable: true, configurable: true });
                    } catch (e) {}
                  }
                }
                try {
                  Object.defineProperty(doc, "documentElement", { value: rootEl, writable: true, configurable: true });
                } catch (e) {}
              }
              return doc;
            }, "createDocument");
          }
          if (impl && typeof impl.createDocumentType !== "function" && implProto) {
            const ownerDoc = this;
            implProto.createDocumentType = markNative(function (qualifiedNameStr, publicId, systemId) {
              const dt = Object.create(global.DocumentType ? global.DocumentType.prototype : (global.Node ? global.Node.prototype : Object.prototype));
              dt.nodeType = 10;
              dt.nodeName = String(qualifiedNameStr || "");
              dt.name = String(qualifiedNameStr || "");
              dt.publicId = String(publicId || "");
              dt.systemId = String(systemId || "");
              dt.ownerDocument = ownerDoc;
              dt.parentNode = null;
              return dt;
            }, "createDocumentType");
          }
          implCache.set(this, impl);
          return impl;
        },
        configurable: true,
      });
    }
  }
  // Mark all built-in / shim functions and prototype accessors as native so Function.prototype.toString
  // returns Chrome-compatible `function <name>() { [native code] }`.
  (function markAllShimBuiltinsNative() {
    const visitedObjs = new WeakSet();
    function markObjMembers(obj) {
      if (!obj || (typeof obj !== "object" && typeof obj !== "function")) return;
      if (visitedObjs.has(obj)) return;
      visitedObjs.add(obj);
      let keys;
      try {
        keys = Object.getOwnPropertyNames(obj);
      } catch (e) {
        return;
      }
      for (let i = 0; i < keys.length; i++) {
        const k = keys[i];
        if (k === "constructor" || k === "prototype" || k === "__proto__" || k.startsWith("__capsurf_")) continue;
        let desc;
        try {
          desc = Object.getOwnPropertyDescriptor(obj, k);
        } catch (e) {
          continue;
        }
        if (!desc) continue;
        if (typeof desc.value === "function") {
          if (!_nativeFunctionNames.has(desc.value)) {
            markNative(desc.value, k);
          }
          if (desc.value.prototype && typeof desc.value.prototype === "object") {
            markObjMembers(desc.value.prototype);
          }
        }
        if (typeof desc.get === "function" && !_nativeFunctionNames.has(desc.get)) {
          markNative(desc.get, "get " + k);
        }
        if (typeof desc.set === "function" && !_nativeFunctionNames.has(desc.set)) {
          markNative(desc.set, "set " + k);
        }
      }
    }
    markObjMembers(global);
    if (typeof globalThis !== "undefined" && globalThis !== global) markObjMembers(globalThis);
    if (typeof window !== "undefined" && window !== global) markObjMembers(window);
    if (global.document) {
      markObjMembers(global.document);
      const docProto = Object.getPrototypeOf(global.document);
      if (docProto) markObjMembers(docProto);
    }
    if (typeof Node !== "undefined" && Node.prototype) markObjMembers(Node.prototype);
    if (typeof Element !== "undefined" && Element.prototype) markObjMembers(Element.prototype);
    if (typeof HTMLElement !== "undefined" && HTMLElement.prototype) markObjMembers(HTMLElement.prototype);
    if (typeof EventTarget !== "undefined" && EventTarget.prototype) markObjMembers(EventTarget.prototype);
  })();
  try {
    const ksSym = Symbol.for("kitesurf.internals");
    const rec = (typeof globalThis !== "undefined" && globalThis[ksSym]) ||
      (global && global[ksSym]) || {
        canvasPushMs: 0,
        demandFontOutlines: function () {},
        fontsInflightCount: function () { return 0; },
        fontsReady: function () { return Promise.resolve(); },
      };
    if (typeof rec.pumpDueTimers !== "function") {
      rec.pumpDueTimers = (typeof global !== "undefined" && typeof global.__capsurf_pump_timers === "function")
        ? global.__capsurf_pump_timers
        : ((typeof globalThis !== "undefined" && typeof globalThis.__capsurf_pump_timers === "function")
          ? globalThis.__capsurf_pump_timers
          : function () { return -1; });
    }
    if (typeof globalThis !== "undefined") {
      try {
        const desc = Object.getOwnPropertyDescriptor(globalThis, ksSym);
        if (!desc || desc.configurable) {
          Object.defineProperty(globalThis, ksSym, {
            value: rec,
            enumerable: false,
            writable: false,
            configurable: false,
          });
        }
      } catch (e) {}
      if (!Object.getOwnPropertyDescriptor(globalThis, "__ks_import")) {
        Object.defineProperty(globalThis, "__ks_import", {
          value: function __ks_import(specifier) {
            return Promise.reject(new TypeError("Dynamic import not supported: " + specifier));
          },
          enumerable: false,
          writable: false,
          configurable: false,
        });
      }
      if (!Object.getOwnPropertyDescriptor(globalThis, "__ks_resolve_import_map")) {
        Object.defineProperty(globalThis, "__ks_resolve_import_map", {
          value: function __ks_resolve_import_map(specifier) {
            return specifier;
          },
          enumerable: false,
          writable: false,
          configurable: false,
        });
      }
    }
  } catch (e) {}
  try {
    if (Object.prototype.hasOwnProperty.call(Object.prototype, "fetch")) {
      delete Object.prototype.fetch;
    }
    Object.defineProperty(Object.prototype, "fetch", {
      get: function () { return undefined; },
      set: function (v) {
        if (this !== Object.prototype) {
          Object.defineProperty(this, "fetch", { value: v, writable: true, enumerable: true, configurable: true });
        }
      },
      enumerable: false,
      configurable: true,
    });
  } catch (e) {}
}

// Auto-install on globalThis/window
installShim(typeof window !== "undefined" ? window : globalThis);
