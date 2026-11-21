// src/js/shim/element/forms/form.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/form.ts

function isFormListedTag(tag) {
  return tag === "button" || tag === "fieldset" || tag === "input" ||
         tag === "object" || tag === "output" || tag === "select" || tag === "textarea";
}

function isFormListed(el) {
  if (!el || el.nodeType !== 1) return false;
  const tag = (el.tagName || "").toLowerCase();
  return isFormListedTag(tag);
}

function collectFormControls(form) {
  const controls = [];
  const seen = new Set();

  function walk(node) {
    if (!node) return;
    let child = node.firstChild;
    while (child) {
      if (child.nodeType === 1) {
        if (isFormListed(child)) {
          if (typeof findAssociatedForm === "function" && findAssociatedForm(child) === form) {
            controls.push(child);
            seen.add(child);
          }
        }
        walk(child);
      }
      child = child.nextSibling;
    }
  }
  walk(form);

  const formId = (typeof form.getAttribute === "function" ? form.getAttribute("id") : null);
  if (formId) {
    const doc = form.ownerDocument || (typeof document !== "undefined" ? document : null);
    let foundExternals = false;
    if (doc && typeof doc.querySelectorAll === "function") {
      try {
        const externals = doc.querySelectorAll('[form="' + formId + '"]');
        for (let i = 0; i < externals.length; i++) {
          const ext = externals[i];
          if (!seen.has(ext) && isFormListed(ext)) {
            const owner = typeof findAssociatedForm === "function" ? findAssociatedForm(ext) : null;
            if (owner === form) {
              controls.push(ext);
              seen.add(ext);
              foundExternals = true;
            }
          }
        }
      } catch (e) {}
    }
    if (!foundExternals) {
      let root = form;
      while (root.parentNode) root = root.parentNode;
      function walkExt(n) {
        if (!n) return;
        if (n.nodeType === 1 && !seen.has(n) && isFormListed(n)) {
          const fAttr = typeof n.getAttribute === "function" ? n.getAttribute("form") : null;
          if (fAttr === formId) {
            const owner = typeof findAssociatedForm === "function" ? findAssociatedForm(n) : null;
            if (owner === form) {
              controls.push(n);
              seen.add(n);
            }
          }
        }
        let c = n.firstChild;
        while (c) {
          walkExt(c);
          c = c.nextSibling;
        }
      }
      walkExt(root);
    }
  }

  return controls;
}

function navigateTo(url) {
  const loc = (typeof window !== "undefined" && window.location) ||
              (typeof globalThis !== "undefined" && globalThis.location) ||
              (typeof location !== "undefined" && location);
  if (loc) {
    loc.href = url;
  }
}

function performFormSubmission(form, submitter) {
  if (!form) return;

  // Resolve method
  let method = (form.getAttribute("method") || "get").toLowerCase().trim();
  if (submitter && typeof submitter.getAttribute === "function") {
    const formMethod = submitter.getAttribute("formmethod");
    if (formMethod) {
      method = formMethod.toLowerCase().trim();
    }
  }
  if (method !== "post") {
    method = "get";
  }

  // Resolve action URL
  let action = form.getAttribute("action") || "";
  if (submitter && typeof submitter.getAttribute === "function") {
    const formAction = submitter.getAttribute("formaction");
    if (formAction) {
      action = formAction;
    }
  }

  const base = (typeof window !== "undefined" && window.location && window.location.href) ||
               (typeof location !== "undefined" && location.href) || "about:blank";
  let targetUrl = action;
  if (!targetUrl) {
    targetUrl = base;
  } else {
    try {
      targetUrl = new URL(targetUrl, base).toString();
    } catch (e) {
      // Keep targetUrl as-is
    }
  }

  // Resolve enctype
  let enctype = (form.getAttribute("enctype") || "").toLowerCase().trim();
  if (submitter && typeof submitter.getAttribute === "function") {
    const formEnctype = submitter.getAttribute("formenctype");
    if (formEnctype) {
      enctype = formEnctype.toLowerCase().trim();
    }
  }
  if (enctype !== "multipart/form-data" && enctype !== "text/plain") {
    enctype = "application/x-www-form-urlencoded";
  }

  // Pick submission encoding from accept-charset or document.characterSet
  const doc = form.ownerDocument || (typeof document !== "undefined" ? document : null);
  let submitCharset = (doc && doc.characterSet) ? doc.characterSet : "UTF-8";
  const acceptCharsetAttr = form.getAttribute("accept-charset");
  if (acceptCharsetAttr && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_resolveEncodingLabel === "function") {
    const tokens = String(acceptCharsetAttr).split(/[\s,]+/);
    for (let ti = 0; ti < tokens.length; ti++) {
      const resolved = globalThis.__capsurf_resolveEncodingLabel(tokens[ti]);
      if (resolved) {
        submitCharset = resolved;
        break;
      }
    }
  }

  const controls = collectFormControls(form);
  const entries = [];
  for (let i = 0; i < controls.length; i++) {
    const c = controls[i];
    if (c.disabled || (typeof c.hasAttribute === "function" && c.hasAttribute("disabled"))) {
      continue;
    }
    const name = c.name || (typeof c.getAttribute === "function" ? c.getAttribute("name") : "");
    if (!name) continue;

    const tag = (c.tagName || "").toLowerCase();
    const type = (c.type || (typeof c.getAttribute === "function" ? c.getAttribute("type") : "") || "").toLowerCase();

    // For buttons and submit inputs: only include if it is the submitter
    if (tag === "button" || type === "submit" || type === "image") {
      if (c !== submitter) continue;
    }

    // Skip non-submitting input types
    if (type === "button" || type === "reset") {
      continue;
    }

    // Checkable inputs: only include if checked
    if (type === "checkbox" || type === "radio") {
      if (!c.checked) continue;
    }

    if (tag === "input" && type === "file") {
      const files = c.files;
      if (files && files.length > 0) {
        for (let fi = 0; fi < files.length; fi++) {
          const f = files[fi];
          if (f) entries.push({ name: name, file: f, value: f.name || "" });
        }
      } else {
        entries.push({ name: name, file: null, value: "" });
      }
      continue;
    }

    let val = c.value !== undefined ? String(c.value) : "";
    if (tag === "input" && type === "hidden" && name === "_charset_") {
      val = submitCharset;
    }
    if ((type === "checkbox" || type === "radio") && !val && (typeof c.hasAttribute === "function" && !c.hasAttribute("value"))) {
      val = "on";
    }
    entries.push({ name: name, file: null, value: val });
  }

  function decodeFileBytesSync(fileObj) {
    if (!fileObj) return "";
    const bytes = fileObj._bytes;
    if (bytes && typeof bytes.length === "number") {
      if (typeof utf8ToString === "function") return utf8ToString(bytes);
      let s = "";
      for (let bi = 0; bi < bytes.length; bi++) s += String.fromCharCode(bytes[bi]);
      return s;
    }
    return "";
  }

  const encFormFn = (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_encodeQueryInCharset === "function")
    ? globalThis.__capsurf_encodeQueryInCharset
    : null;
  const queryParts = [];
  for (let ei = 0; ei < entries.length; ei++) {
    const kEnc = encFormFn ? encFormFn(entries[ei].name, submitCharset, true) : encodeURIComponent(entries[ei].name);
    const vEnc = encFormFn ? encFormFn(entries[ei].value, submitCharset, true) : encodeURIComponent(entries[ei].value);
    queryParts.push(kEnc + "=" + vEnc);
  }
  const encodedParams = queryParts.join("&");

  // Check if form targets a named iframe in the document
  let formTarget = (form.getAttribute("target") || "").trim();
  if (submitter && typeof submitter.getAttribute === "function") {
    const ft = submitter.getAttribute("formtarget");
    if (ft) formTarget = ft.trim();
  }
  let targetIframe = null;
  if (formTarget && formTarget !== "_self" && formTarget !== "_top" && formTarget !== "_parent" && formTarget !== "_blank") {
    if (doc && typeof doc.querySelectorAll === "function") {
      const frames = doc.querySelectorAll("iframe");
      for (let fi = 0; fi < frames.length; fi++) {
        const fn = frames[fi] && typeof frames[fi].getAttribute === "function" ? frames[fi].getAttribute("name") : "";
        if (fn === formTarget) {
          targetIframe = frames[fi];
          break;
        }
      }
    }
  }

  if (method === "get") {
    const baseUrl = targetUrl.split("?")[0].split("#")[0];
    const finalUrl = encodedParams ? (baseUrl + "?" + encodedParams) : baseUrl;
    if (targetIframe && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_navigate_iframe === "function") {
      globalThis.__capsurf_navigate_iframe(targetIframe, finalUrl);
      return;
    }
    if (typeof globalThis !== "undefined") {
      delete globalThis.__capsurf_pending_navigation_post;
    }
    navigateTo(finalUrl);
  } else if (method === "post") {
    const postUrl = targetUrl.split("#")[0];
    let bodyStr = encodedParams;
    let contentType = "application/x-www-form-urlencoded";
    if (enctype === "text/plain") {
      const lines = [];
      for (let ei = 0; ei < entries.length; ei++) {
        lines.push(entries[ei].name + "=" + entries[ei].value + "\n");
      }
      bodyStr = lines.join("");
      contentType = "text/plain";
    } else if (enctype === "multipart/form-data") {
      const boundary = "----CapSurfFormBoundary7MA4YWxkTrZu0gW";
      const parts = [];
      for (let ei = 0; ei < entries.length; ei++) {
        const ent = entries[ei];
        if (ent.file) {
          const fname = String(ent.file.name || "").replace(/"/g, "%22");
          const ftype = ent.file.type || "application/octet-stream";
          const fbody = decodeFileBytesSync(ent.file);
          parts.push(
            "--" + boundary + "\r\n" +
            'Content-Disposition: form-data; name="' + ent.name.replace(/"/g, "%22") + '"; filename="' + fname + '"\r\n' +
            "Content-Type: " + ftype + "\r\n\r\n" +
            fbody + "\r\n"
          );
        } else {
          parts.push(
            "--" + boundary + "\r\n" +
            'Content-Disposition: form-data; name="' + ent.name.replace(/"/g, "%22") + '"\r\n\r\n' +
            ent.value + "\r\n"
          );
        }
      }
      parts.push("--" + boundary + "--\r\n");
      bodyStr = parts.join("");
      contentType = "multipart/form-data; boundary=" + boundary;
    }
    if (targetIframe && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_apply_iframe_html === "function") {
      const escHtml = function (s) {
        return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
      };
      let reqQuery = "";
      try {
        const qIdx = postUrl.indexOf("?");
        reqQuery = qIdx !== -1 ? postUrl.slice(qIdx) : "";
      } catch (e) {}
      const echoHtml = '<!doctype html><html><body>' +
        '<div id="method">POST</div>' +
        '<div id="ctype">' + escHtml(contentType) + '</div>' +
        '<div id="body">' + escHtml(bodyStr) + '</div>' +
        '<div id="query">' + escHtml(reqQuery) + '</div>' +
        '</body></html>';
      const frame = targetIframe;
      Promise.resolve().then(function () {
        if (!frame._contentWindow) {
          void frame.contentWindow;
        }
        globalThis.__capsurf_apply_iframe_html(frame, postUrl, echoHtml, { charset: "UTF-8" });
        if (typeof globalThis.__capsurf_fire_iframe_load === "function") {
          globalThis.__capsurf_fire_iframe_load(frame);
        }
      });
      return;
    }
    if (typeof globalThis !== "undefined") {
      globalThis.__capsurf_pending_navigation_post = {
        url: postUrl,
        method: "POST",
        body: bodyStr,
        contentType: contentType,
      };
    }
    navigateTo(postUrl);
  }
}

function submitForm(form, submitter) {
  if (!form) return;
  let ev;
  try {
    ev = new SubmitEvent("submit", {
      bubbles: true,
      cancelable: true,
      submitter: submitter || null,
    });
  } catch (e) {
    ev = new Event("submit", { bubbles: true, cancelable: true });
    Object.defineProperty(ev, "submitter", {
      value: submitter || null,
      writable: true,
      configurable: true,
    });
  }
  form.dispatchEvent(ev);
  if (ev.defaultPrevented) {
    return;
  }
  performFormSubmission(form, submitter);
}

function installFormElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};
  const HTMLElementProto = (global.HTMLElement && global.HTMLElement.prototype) || ElementProto;

  function HTMLFormElement() {}
  HTMLFormElement.prototype = Object.create(HTMLElementProto);
  HTMLFormElement.prototype.constructor = HTMLFormElement;
  Object.defineProperty(HTMLFormElement.prototype, Symbol.toStringTag, {
    value: "HTMLFormElement",
    configurable: true,
  });

  const formElementsCollectionMap = new WeakMap();

  Object.defineProperty(HTMLFormElement.prototype, "elements", {
    get: function () {
      let col = formElementsCollectionMap.get(this);
      if (!col) {
        const self = this;
        const target = Object.create((global.HTMLFormControlsCollection || HTMLCollection).prototype);
        target._isFormControlsCollection = true;
        col = makeHtmlCollection(function () {
          return collectFormControls(self);
        }, target);
        formElementsCollectionMap.set(this, col);
      }
      return col;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "length", {
    get: function () {
      const els = this.elements;
      let count = 0;
      for (let i = 0; i < els.length; i++) {
        const e = els[i];
        if (e) {
          const tag = (e.tagName || "").toLowerCase();
          const type = (e.type || (typeof e.getAttribute === "function" ? e.getAttribute("type") : "") || "").toLowerCase();
          if (tag === "input" && type === "image") continue;
        }
        count++;
      }
      return count;
    },
    configurable: true,
  });

  HTMLFormElement.prototype.namedItem = function (name) {
    if (!name) return null;
    return this.elements.namedItem(name);
  };

  HTMLFormElement.prototype.item = function (index) {
    return this.elements.item(index);
  };

  Object.defineProperty(HTMLFormElement.prototype, "name", {
    get: function () { return this.getAttribute("name") || ""; },
    set: function (v) { this.setAttribute("name", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "method", {
    get: function () { return (this.getAttribute("method") || "get").toLowerCase(); },
    set: function (v) { this.setAttribute("method", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "action", {
    get: function () { return this.getAttribute("action") || ""; },
    set: function (v) { this.setAttribute("action", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "enctype", {
    get: function () {
      const raw = (this.getAttribute("enctype") || "").toLowerCase();
      if (raw === "multipart/form-data" || raw === "text/plain") return raw;
      return "application/x-www-form-urlencoded";
    },
    set: function (v) { this.setAttribute("enctype", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "encoding", {
    get: function () { return this.enctype; },
    set: function (v) { this.enctype = v; },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "target", {
    get: function () { return this.getAttribute("target") || ""; },
    set: function (v) { this.setAttribute("target", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "acceptCharset", {
    get: function () { return this.getAttribute("accept-charset") || ""; },
    set: function (v) { this.setAttribute("accept-charset", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "noValidate", {
    get: function () { return this.hasAttribute("novalidate"); },
    set: function (v) { if (v) this.setAttribute("novalidate", ""); else this.removeAttribute("novalidate"); },
    configurable: true,
  });

  HTMLFormElement.prototype.submit = function () {
    submitForm(this, null);
  };

  HTMLFormElement.prototype.requestSubmit = function (submitter) {
    if (submitter !== undefined && submitter !== null) {
      const tag = (submitter.tagName || "").toLowerCase();
      const type = (submitter.type || (typeof submitter.getAttribute === "function" ? submitter.getAttribute("type") : "") || "").toLowerCase();
      const isButton = (tag === "button" && (type === "submit" || !type || type === "")) ||
                       (tag === "input" && (type === "submit" || type === "image"));
      if (!isButton) {
        throw new TypeError("requestSubmit: submitter must be a submit button");
      }
      if (findAssociatedForm(submitter) !== this) {
        const DOMEx = global.DOMException || Error;
        throw new DOMEx("requestSubmit: submitter is not associated with this form", "NotFoundError");
      }
    }
    submitForm(this, submitter || null);
  };

  HTMLFormElement.prototype.reset = function () {
    if (!(this instanceof HTMLFormElement) || (this.namespaceURI && this.namespaceURI !== HTML_NS)) {
      throw new TypeError("Failed to execute 'reset' on 'HTMLFormElement': Illegal invocation");
    }

    const evt = new Event("reset", { bubbles: true, cancelable: true });
    let allowed = true;
    if (typeof dispatchTrustedEvent === "function") {
      allowed = dispatchTrustedEvent(this, evt);
    } else if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
      allowed = global.__capsurf_dispatchTrustedEvent(this, evt);
    } else {
      allowed = this.dispatchEvent(evt);
    }
    if (!allowed) return;

    const list = collectFormControls(this);
    for (let i = 0; i < list.length; i++) {
      const c = list[i];
      const tag = (c.tagName || "").toLowerCase();
      if (tag === "input" || tag === "textarea") {
        if (c.defaultValue !== undefined) {
          c.value = c.defaultValue;
        } else if (typeof c.getAttribute === "function") {
          const attrVal = c.getAttribute("value");
          c.value = attrVal !== null ? attrVal : "";
        }
        c._valueDirty = false;
        if (c.defaultChecked !== undefined) {
          c.checked = c.defaultChecked;
        } else if (typeof c.hasAttribute === "function") {
          c.checked = c.hasAttribute("checked");
        }
        c._checkedDirty = false;
      } else if (tag === "select") {
        if (typeof getSelectState === "function") getSelectState(c).selectedIndex = undefined;
        c._selectedIndex = undefined;
        const opts = c.options;
        if (opts) {
          for (let j = 0; j < opts.length; j++) {
            if (typeof getOptionState === "function") getOptionState(opts[j]).selected = undefined;
            opts[j]._selected = undefined;
            opts[j].selected = typeof opts[j].hasAttribute === "function" ? opts[j].hasAttribute("selected") : false;
          }
        }
      }
    }
  };

  HTMLFormElement.prototype.checkValidity = function () {
    const list = collectFormControls(this);
    let allValid = true;
    for (let i = 0; i < list.length; i++) {
      if (list[i].checkValidity && !list[i].checkValidity()) {
        allValid = false;
      }
    }
    return allValid;
  };

  HTMLFormElement.prototype.reportValidity = function () {
    return this.checkValidity();
  };

  markNative(HTMLFormElement, "HTMLFormElement");
  markNative(HTMLFormElement.prototype.submit, "submit");
  markNative(HTMLFormElement.prototype.requestSubmit, "requestSubmit");
  markNative(HTMLFormElement.prototype.reset, "reset");
  markNative(HTMLFormElement.prototype.checkValidity, "checkValidity");
  markNative(HTMLFormElement.prototype.reportValidity, "reportValidity");
  markNative(HTMLFormElement.prototype.namedItem, "namedItem");
  markNative(HTMLFormElement.prototype.item, "item");

  if (ElementProto) {
    if (!ElementProto.submit) {
      ElementProto.submit = function () {
        if ((this.tagName || "").toLowerCase() === "form") {
          submitForm(this, null);
        }
      };
      markNative(ElementProto.submit, "submit");
    }
    if (!ElementProto.reset) {
      ElementProto.reset = function () {
        if ((this.tagName || "").toLowerCase() === "form") {
          HTMLFormElement.prototype.reset.call(this);
        }
      };
      markNative(ElementProto.reset, "reset");
    }
    if (!ElementProto.requestSubmit) {
      ElementProto.requestSubmit = function (submitter) {
        if ((this.tagName || "").toLowerCase() === "form") {
          HTMLFormElement.prototype.requestSubmit.call(this, submitter);
        }
      };
      markNative(ElementProto.requestSubmit, "requestSubmit");
    }
  }

  global.HTMLFormElement = HTMLFormElement;
  if (typeof globalThis !== "undefined") {
    globalThis.HTMLFormElement = HTMLFormElement;
  }

  // [LegacyOverrideBuiltIns] named & indexed property proxy on HTMLFormElement instances.
  // Tree/EventTarget/Node/Element members stay the form's own so engine walks never break.
  const FORM_OWN_KEEP = new Set([
    "parentNode", "parentElement", "firstChild", "lastChild", "childNodes", "children",
    "nextSibling", "previousSibling", "nextElementSibling", "previousElementSibling",
    "ownerDocument", "nodeType", "nodeName", "tagName", "localName", "namespaceURI",
    "prefix", "isConnected", "getRootNode", "contains", "hasChildNodes",
    "appendChild", "insertBefore", "removeChild", "replaceChild", "cloneNode",
    "compareDocumentPosition", "normalize", "isEqualNode", "isSameNode",
    "getAttribute", "setAttribute", "removeAttribute", "hasAttribute",
    "getAttributeNS", "setAttributeNS", "removeAttributeNS", "hasAttributeNS",
    "getAttributeNames", "attributes",
    "addEventListener", "removeEventListener", "dispatchEvent",
    "querySelector", "querySelectorAll", "getElementsByTagName", "getElementsByClassName",
    "matches", "closest", "remove", "before", "after", "replaceWith", "append", "prepend",
    "focus", "blur", "click", "getBoundingClientRect", "getClientRects",
    "style", "classList", "className", "innerHTML", "outerHTML", "textContent", "innerText",
    "constructor", "toString", "valueOf", "hasOwnProperty", "isPrototypeOf",
    "propertyIsEnumerable", "toLocaleString", "__proto__", "shadowRoot", "attachShadow",
    "dataset", "hidden", "tabIndex", "title", "lang", "dir", "slot",
  ]);
  const RX_ARRAY_INDEX = /^(0|[1-9]\d*)$/;
  let _inFormNamedLookup = false;

  function lookupFormNamedProperty(form, prop) {
    if (_inFormNamedLookup) return undefined;
    _inFormNamedLookup = true;
    try {
      const controls = collectFormControls(form);
      const matches = [];
      for (let i = 0; i < controls.length; i++) {
        const c = controls[i];
        if (!c || c.nodeType !== 1) continue;
        const n = typeof c.getAttribute === "function" ? c.getAttribute("name") : null;
        const id = typeof c.getAttribute === "function" ? c.getAttribute("id") : null;
        if (n === prop || id === prop) {
          matches.push(typeof upgradeElement === "function" ? upgradeElement(c) : c);
        }
      }
      if (matches.length === 0) return undefined;
      if (matches.length === 1) return matches[0];
      return makeHtmlCollection(function () { return matches; });
    } finally {
      _inFormNamedLookup = false;
    }
  }

  const formNamedPropsProxy = new Proxy(HTMLFormElement.prototype, {
    get: function (target, prop, receiver) {
      if (
        !receiver ||
        receiver === target ||
        receiver === formNamedPropsProxy ||
        typeof prop !== "string" ||
        prop.charCodeAt(0) === 95 || // '_'
        FORM_OWN_KEEP.has(prop)
      ) {
        return Reflect.get(target, prop, receiver);
      }
      if (RX_ARRAY_INDEX.test(prop)) {
        const idx = Number(prop);
        const controls = collectFormControls(receiver);
        if (idx >= 0 && idx < controls.length) {
          const item = controls[idx];
          return typeof upgradeElement === "function" ? upgradeElement(item) : item;
        }
        return undefined;
      }
      const named = lookupFormNamedProperty(receiver, prop);
      if (named !== undefined) return named;
      return Reflect.get(target, prop, receiver);
    },
    has: function (target, prop) {
      return Reflect.has(target, prop);
    },
  });

  Object.defineProperty(HTMLFormElement, Symbol.hasInstance, {
    value: function (inst) {
      if (!inst || (typeof inst !== "object" && typeof inst !== "function")) return false;
      let p = Object.getPrototypeOf(inst);
      while (p) {
        if (p === HTMLFormElement.prototype || p === formNamedPropsProxy) return true;
        p = Object.getPrototypeOf(p);
      }
      return false;
    },
    configurable: true,
  });

  global.__capsurf_formNamedPropsProxy = formNamedPropsProxy;
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_formNamedPropsProxy = formNamedPropsProxy;
  }
}
