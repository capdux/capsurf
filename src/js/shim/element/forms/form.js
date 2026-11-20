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

  const formId = form.id || (typeof form.getAttribute === "function" ? form.getAttribute("id") : null);
  if (formId) {
    const doc = form.ownerDocument || (typeof document !== "undefined" ? document : null);
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
            }
          }
        }
      } catch (e) {}
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

  if (method === "get") {
    const controls = collectFormControls(form);
    const queryParts = [];
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

      let val = c.value !== undefined ? String(c.value) : "";
      if ((type === "checkbox" || type === "radio") && !val && (typeof c.hasAttribute === "function" && !c.hasAttribute("value"))) {
        val = "on";
      }
      queryParts.push(encodeURIComponent(name) + "=" + encodeURIComponent(val));
    }

    const queryString = queryParts.join("&");
    const baseUrl = targetUrl.split("?")[0].split("#")[0];
    const finalUrl = queryString ? (baseUrl + "?" + queryString) : baseUrl;
    navigateTo(finalUrl);
  } else if (method === "post") {
    navigateTo(targetUrl.split("#")[0]);
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

  Object.defineProperty(HTMLFormElement.prototype, "elements", {
    get: function () {
      if (!this._elementsCollection) {
        const self = this;
        const target = Object.create((global.HTMLFormControlsCollection || HTMLCollection).prototype);
        target._isFormControlsCollection = true;
        this._elementsCollection = makeHtmlCollection(function () {
          return collectFormControls(self);
        }, target);
      }
      return this._elementsCollection;
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
    if (!(this instanceof HTMLFormElement)) {
      throw new TypeError("Failed to execute 'reset' on 'HTMLFormElement': Illegal invocation");
    }

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
    const evt = new Event("reset", { bubbles: true, cancelable: true });
    this.dispatchEvent(evt);
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
}
