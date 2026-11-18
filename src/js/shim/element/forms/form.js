// src/js/shim/element/forms/form.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/forms/form.ts

function collectFormControls(form) {
  const controls = [];
  function walk(node) {
    if (!node) return;
    let child = node.firstChild;
    while (child) {
      const tag = (child.tagName || "").toLowerCase();
      if (tag === "input" || tag === "button" || tag === "select" || tag === "textarea") {
        controls.push(child);
      }
      if (tag !== "select") {
        walk(child);
      }
      child = child.nextSibling;
    }
  }
  walk(form);
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
  const ev = new SubmitEvent("submit", {
    bubbles: true,
    cancelable: true,
    submitter: submitter || null,
  });
  form.dispatchEvent(ev);
  if (ev.defaultPrevented) {
    return;
  }
  performFormSubmission(form, submitter);
}

function installFormElement(global) {
  const ElementProto = global.Element ? global.Element.prototype : {};

  function HTMLFormElement() {}
  HTMLFormElement.prototype = Object.create(ElementProto);
  HTMLFormElement.prototype.constructor = HTMLFormElement;

  Object.defineProperty(HTMLFormElement.prototype, "elements", {
    get: function () {
      const list = collectFormControls(this);
      const col = new HTMLCollection(list);
      // Map controls by name and id
      for (let i = 0; i < list.length; i++) {
        const item = list[i];
        if (item.name && !col[item.name]) col[item.name] = item;
        if (item.id && !col[item.id]) col[item.id] = item;
      }
      return col;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLFormElement.prototype, "length", {
    get: function () { return this.elements.length; },
    configurable: true,
  });

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
    performFormSubmission(this, null);
  };

  HTMLFormElement.prototype.requestSubmit = function (submitter) {
    if (submitter !== undefined && submitter !== null) {
      const tag = (submitter.tagName || "").toLowerCase();
      const type = (submitter.type || (typeof submitter.getAttribute === "function" ? submitter.getAttribute("type") : "") || "").toLowerCase();
      const isButton = (tag === "button") || (tag === "input" && (type === "submit" || type === "image"));
      if (!isButton) {
        throw new TypeError("requestSubmit: submitter must be a submit button");
      }
      if (findAssociatedForm(submitter) !== this) {
        throw new DOMException("requestSubmit: submitter is not associated with this form", "NotFoundError");
      }
    }
    submitForm(this, submitter || null);
  };

  HTMLFormElement.prototype.reset = function () {
    const list = collectFormControls(this);
    for (let i = 0; i < list.length; i++) {
      const c = list[i];
      if (c.defaultValue !== undefined) c.value = c.defaultValue;
      if (c.defaultChecked !== undefined) c.checked = c.defaultChecked;
      if (c.defaultSelected !== undefined) c.selected = c.defaultSelected;
      if ((c.tagName || "").toLowerCase() === "select") {
        if (typeof getSelectState === "function") getSelectState(c).selectedIndex = undefined;
        c._selectedIndex = undefined;
        const opts = c.options;
        for (let j = 0; j < opts.length; j++) {
          if (typeof getOptionState === "function") getOptionState(opts[j]).selected = undefined;
          opts[j]._selected = undefined;
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

  if (ElementProto) {
    if (!ElementProto.submit) {
      ElementProto.submit = function () {
        if ((this.tagName || "").toLowerCase() === "form") {
          performFormSubmission(this, null);
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
}
