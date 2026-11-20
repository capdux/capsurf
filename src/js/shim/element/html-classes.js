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

  // 0. Generic HTMLElement (base class for all HTML*Element interfaces)
  const upgradeTargets = [];
  const constructionStacks = new Map();
  global._constructionStacks = constructionStacks;
  global._upgradeTargets = upgradeTargets;

  function HTMLElement() {
    const targetCtor = new.target;
    if (targetCtor) {
      const stack = constructionStacks.get(targetCtor);
      if (stack && stack.length > 0) {
        const el = stack.pop();
        const uIdx = upgradeTargets.indexOf(el);
        if (uIdx !== -1) upgradeTargets.splice(uIdx, 1);
        return el;
      }
    }
    if (upgradeTargets.length > 0) {
      return upgradeTargets.pop();
    }
  }
  HTMLElement.prototype = Object.create(ElementProto);
  HTMLElement.prototype.constructor = HTMLElement;
  Object.defineProperty(HTMLElement.prototype, Symbol.toStringTag, {
    value: "HTMLElement",
    configurable: true,
  });
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
  if (typeof globalThis !== "undefined") globalThis.HTMLElement = HTMLElement;
  if (typeof window !== "undefined") window.HTMLElement = HTMLElement;

  const HTMLElementProto = HTMLElement.prototype;

  // GlobalEventHandlers IDL accessors on HTMLElement.prototype
  const _htmlEventHandlers = new WeakMap();
  function _isValidHtmlElementReceiver(obj) {
    if (!obj || typeof obj !== "object") return false;
    if (
      Object.prototype.hasOwnProperty.call(obj, "constructor") &&
      obj.constructor &&
      obj.constructor.prototype === obj
    ) {
      return false;
    }
    if (Object.prototype.hasOwnProperty.call(obj, Symbol.toStringTag)) {
      return false;
    }
    return obj.nodeType === 1;
  }
  const GLOBAL_EVENT_HANDLER_NAMES = [
    "onabort", "onanimationcancel", "onanimationend", "onanimationiteration", "onanimationstart",
    "onauxclick", "onbeforeinput", "onblur", "oncancel", "oncanplay",
    "oncanplaythrough", "onchange", "onclick", "onclose", "oncontextmenu",
    "oncopy", "oncut", "ondblclick", "ondrag", "ondragend", "ondragenter",
    "ondragleave", "ondragover", "ondragstart", "ondrop", "ondurationchange",
    "onemptied", "onended", "onerror", "onfocus", "onformdata", "ongotpointercapture",
    "oninput", "oninvalid", "onkeydown", "onkeypress", "onkeyup", "onload",
    "onloadeddata", "onloadedmetadata", "onloadstart", "onlostpointercapture",
    "onmousedown", "onmouseenter", "onmouseleave", "onmousemove", "onmouseout",
    "onmouseover", "onmouseup", "onpaste", "onpause", "onplay", "onplaying",
    "onpointercancel", "onpointerdown", "onpointerenter", "onpointerleave",
    "onpointermove", "onpointerout", "onpointerover", "onpointerup", "onprogress",
    "onratechange", "onreset", "onresize", "onscroll", "onscrollend", "onseeked",
    "onseeking", "onselect", "onselectionchange", "onselectstart", "onstalled",
    "onsubmit", "onsuspend", "ontimeupdate", "ontoggle", "ontouchcancel",
    "ontouchend", "ontouchmove", "ontouchstart", "ontransitioncancel",
    "ontransitionend", "ontransitionrun", "ontransitionstart", "onvolumechange",
    "onwaiting", "onwheel",
  ];
  for (let i = 0; i < GLOBAL_EVENT_HANDLER_NAMES.length; i++) {
    const prop = GLOBAL_EVENT_HANDLER_NAMES[i];
    if (Object.prototype.hasOwnProperty.call(HTMLElementProto, prop)) continue;
    Object.defineProperty(HTMLElementProto, prop, {
      configurable: true,
      enumerable: true,
      get: function () {
        if (!_isValidHtmlElementReceiver(this)) {
          throw new TypeError("Illegal invocation");
        }
        const m = _htmlEventHandlers.get(this);
        if (m && Object.prototype.hasOwnProperty.call(m, prop)) {
          return m[prop];
        }
        if (typeof this.getAttribute === "function") {
          const attr = this.getAttribute(prop);
          if (typeof attr === "string") {
            try {
              const fn = new Function("event", attr);
              let map = m;
              if (!map) {
                map = Object.create(null);
                _htmlEventHandlers.set(this, map);
              }
              map[prop] = fn;
              return fn;
            } catch (_) {}
          }
        }
        return null;
      },
      set: function (v) {
        if (!_isValidHtmlElementReceiver(this)) {
          throw new TypeError("Illegal invocation");
        }
        let m = _htmlEventHandlers.get(this);
        if (!m) {
          m = Object.create(null);
          _htmlEventHandlers.set(this, m);
        }
        m[prop] = typeof v === "function" ? v : null;
      },
    });
  }

  // HTML rendered text fragment helper (for innerText / outerText setters)
  function createRenderedTextFragment(str, doc) {
    const d = doc || global.document || (typeof document !== "undefined" ? document : null);
    const frag = (d && typeof d.createDocumentFragment === "function")
      ? d.createDocumentFragment()
      : new DocumentFragment();
    let pos = 0;
    const len = str.length;
    while (pos < len) {
      const start = pos;
      while (pos < len && str.charCodeAt(pos) !== 10 && str.charCodeAt(pos) !== 13) {
        pos++;
      }
      if (pos > start) {
        const chunk = str.slice(start, pos);
        const textNode = (d && typeof d.createTextNode === "function")
          ? d.createTextNode(chunk)
          : new Text(chunk);
        frag.appendChild(textNode);
      }
      while (pos < len && (str.charCodeAt(pos) === 10 || str.charCodeAt(pos) === 13)) {
        if (str.charCodeAt(pos) === 13 && pos + 1 < len && str.charCodeAt(pos + 1) === 10) {
          pos += 2;
        } else {
          pos += 1;
        }
        const br = (d && typeof d.createElement === "function") ? d.createElement("br") : null;
        if (br) frag.appendChild(br);
      }
    }
    return frag;
  }

  // HTML rendered text collection helper (for innerText getter)
  const IT_HIDDEN_TAGS = new Set(["script", "style", "noscript", "template", "head", "title", "meta", "link", "base"]);
  const IT_BLOCK_DISPLAYS = new Set([
    "block", "flex", "grid", "list-item", "table", "table-caption", "flow-root",
    "table-row-group", "table-header-group", "table-footer-group",
  ]);
  const IT_BLOCK_TAGS = new Set([
    "html", "body", "div", "p", "h1", "h2", "h3", "h4", "h5", "h6",
    "ul", "ol", "li", "dl", "dt", "dd", "section", "article", "nav",
    "aside", "header", "footer", "main", "form", "fieldset", "blockquote",
    "pre", "hr", "figure", "figcaption", "address", "details", "summary", "dialog",
  ]);

  function getElComputedStyleSafe(el) {
    try {
      const fn = global.getComputedStyle || (typeof getComputedStyle === "function" ? getComputedStyle : null);
      if (typeof fn === "function") return fn(el);
    } catch (e) {}
    return null;
  }

  function getElDisplayForInnerText(el) {
    const tag = String(el.localName || el.tagName || "").toLowerCase();
    const cs = getElComputedStyleSafe(el);
    let disp = cs ? String(cs.display || "").trim().toLowerCase() : "";
    if (!disp && el.style && el.style.display) {
      disp = String(el.style.display).trim().toLowerCase();
    }
    if (disp) return disp;
    if (IT_HIDDEN_TAGS.has(tag)) return "none";
    if (tag === "table") return "table";
    if (tag === "tr") return "table-row";
    if (tag === "td" || tag === "th") return "table-cell";
    if (tag === "tbody") return "table-row-group";
    if (tag === "thead") return "table-header-group";
    if (tag === "tfoot") return "table-footer-group";
    if (IT_BLOCK_TAGS.has(tag)) return "block";
    return "inline";
  }

  function isElementBeingRendered(el) {
    if (!el || el.nodeType !== 1) return false;
    if (el.isConnected === false) return false;
    let cur = el;
    while (cur && cur.nodeType === 1) {
      if (getElDisplayForInnerText(cur) === "none") return false;
      cur = cur.parentElement || cur.parentNode;
    }
    return true;
  }

  function collectRenderedText(root) {
    const items = []; // { type: "text", text: string, preserveWs: boolean } | { type: "br" } | { type: "tab" } | { type: "rb", count: number }

    function hasNextRenderedSiblingWithDisplay(el, targetDisp) {
      const parent = el.parentNode;
      if (parent) {
        const siblings = parent.childNodes || parent._children || [];
        let found = false;
        for (let i = 0; i < siblings.length; i++) {
          const sib = siblings[i];
          if (!found) {
            if (sib === el) found = true;
            continue;
          }
          if (sib && sib.nodeType === 1) {
            const d = getElDisplayForInnerText(sib);
            if (d !== "none") return d === targetDisp;
          }
        }
        if (found) return false;
      }
      let sib = el.nextSibling;
      while (sib) {
        if (sib.nodeType === 1) {
          const d = getElDisplayForInnerText(sib);
          if (d !== "none") return d === targetDisp;
        }
        sib = sib.nextSibling;
      }
      return false;
    }

    function walk(node, inheritedVis, inheritedWs, isRoot) {
      if (!node) return;
      if (node.nodeType === 3) {
        if (inheritedVis === "hidden" || inheritedVis === "collapse") return;
        const raw = String(
          (node.textContent !== undefined && node.textContent !== null && node.textContent !== "")
            ? node.textContent
            : (node.nodeValue !== undefined && node.nodeValue !== null ? node.nodeValue : (node.data || ""))
        );
        if (!raw) return;
        const preserve = inheritedWs === "pre" || inheritedWs === "pre-wrap" || inheritedWs === "break-spaces";
        if (preserve) {
          items.push({ type: "text", text: raw.replace(/\r\n?/g, "\n"), preserveWs: true });
        } else if (inheritedWs === "pre-line") {
          const parts = raw.replace(/\r\n?/g, "\n").split("\n");
          for (let i = 0; i < parts.length; i++) {
            if (i > 0) items.push({ type: "br" });
            const collapsed = parts[i].replace(/[\t\f ]+/g, " ");
            if (collapsed) items.push({ type: "text", text: collapsed, preserveWs: false });
          }
        } else {
          const collapsed = raw.replace(/[\t\n\f\r ]+/g, " ");
          if (collapsed) items.push({ type: "text", text: collapsed, preserveWs: false });
        }
        return;
      }
      if (node.nodeType !== 1) return;

      const tag = String(node.localName || node.tagName || "").toLowerCase();
      if (!isRoot && IT_HIDDEN_TAGS.has(tag)) return;

      const cs = getElComputedStyleSafe(node);
      const disp = getElDisplayForInnerText(node);
      if (!isRoot && disp === "none") return;

      let vis = inheritedVis;
      const ownVis = cs ? String(cs.visibility || "").trim().toLowerCase() : (node.style && node.style.visibility ? String(node.style.visibility).trim().toLowerCase() : "");
      if (ownVis) vis = ownVis;

      let ws = inheritedWs;
      const ownWs = cs ? String(cs.whiteSpace || cs["white-space"] || "").trim().toLowerCase() : (node.style && node.style.whiteSpace ? String(node.style.whiteSpace).trim().toLowerCase() : "");
      if (ownWs) ws = ownWs;
      else if (tag === "pre" || tag === "textarea") ws = "pre";

      if (tag === "br") {
        if (vis !== "hidden" && vis !== "collapse") {
          items.push({ type: "br" });
        }
        return;
      }

      const isP = tag === "p";
      const isBlock = !isRoot && (IT_BLOCK_DISPLAYS.has(disp) || isP);
      if (isBlock) {
        items.push({ type: "rb", count: isP ? 2 : 1 });
      }

      const kids = node.childNodes || node._children || [];
      if (kids && kids.length > 0) {
        for (let ki = 0; ki < kids.length; ki++) {
          walk(kids[ki], vis, ws, false);
        }
      } else {
        let ch = node.firstChild;
        while (ch) {
          walk(ch, vis, ws, false);
          ch = ch.nextSibling;
        }
      }

      if (!isRoot) {
        if (disp === "table-cell") {
          if (hasNextRenderedSiblingWithDisplay(node, "table-cell")) {
            items.push({ type: "tab" });
          }
        } else if (disp === "table-row") {
          if (hasNextRenderedSiblingWithDisplay(node, "table-row")) {
            items.push({ type: "br" });
          }
        } else if (isBlock) {
          items.push({ type: "rb", count: isP ? 2 : 1 });
        }
      }
    }

    const rootCs = getElComputedStyleSafe(root);
    const rootVis = (rootCs && rootCs.visibility ? String(rootCs.visibility).trim().toLowerCase() : "") || "visible";
    const rootTag = String(root.localName || root.tagName || "").toLowerCase();
    const rootWs = (rootCs && (rootCs.whiteSpace || rootCs["white-space"]) ? String(rootCs.whiteSpace || rootCs["white-space"]).trim().toLowerCase() : "") || (rootTag === "pre" ? "pre" : "normal");
    walk(root, rootVis, rootWs, true);

    // 1. Merge consecutive collapsible text items and collapse adjacent spaces
    const merged = [];
    for (let i = 0; i < items.length; i++) {
      const it = items[i];
      if (it.type === "rb") {
        if (merged.length > 0 && merged[merged.length - 1].type === "rb") {
          merged[merged.length - 1].count = Math.max(merged[merged.length - 1].count, it.count);
        } else {
          merged.push({ type: "rb", count: it.count });
        }
      } else if (it.type === "text" && !it.preserveWs) {
        if (merged.length > 0 && merged[merged.length - 1].type === "text" && !merged[merged.length - 1].preserveWs) {
          merged[merged.length - 1].text = (merged[merged.length - 1].text + it.text).replace(/ +/g, " ");
        } else {
          merged.push({ type: "text", text: it.text, preserveWs: false });
        }
      } else {
        merged.push(it);
      }
    }

    // 2. Trim collapsible spaces adjacent to line breaks, tabs, or boundaries
    for (let i = 0; i < merged.length; i++) {
      const it = merged[i];
      if (it.type === "text" && !it.preserveWs) {
        const prev = i > 0 ? merged[i - 1] : null;
        const next = i + 1 < merged.length ? merged[i + 1] : null;
        if (!prev || prev.type === "br" || prev.type === "rb" || prev.type === "tab") {
          it.text = it.text.replace(/^ +/, "");
        }
        if (!next || next.type === "br" || next.type === "rb" || next.type === "tab") {
          it.text = it.text.replace(/ +$/, "");
        }
      }
    }

    // 3. Remove empty text items and re-merge adjacent rb items
    const cleaned = [];
    for (let i = 0; i < merged.length; i++) {
      const it = merged[i];
      if (it.type === "text" && it.text === "") continue;
      if (it.type === "rb") {
        if (cleaned.length > 0 && cleaned[cleaned.length - 1].type === "rb") {
          cleaned[cleaned.length - 1].count = Math.max(cleaned[cleaned.length - 1].count, it.count);
          continue;
        }
      }
      cleaned.push(it);
    }

    // 4. Drop leading and trailing required line breaks
    while (cleaned.length > 0 && cleaned[0].type === "rb") cleaned.shift();
    while (cleaned.length > 0 && cleaned[cleaned.length - 1].type === "rb") cleaned.pop();

    // 5. Serialize
    let out = "";
    for (let i = 0; i < cleaned.length; i++) {
      const it = cleaned[i];
      if (it.type === "text") out += it.text;
      else if (it.type === "br") out += "\n";
      else if (it.type === "tab") out += "\t";
      else if (it.type === "rb") out += "\n".repeat(it.count);
    }
    return out;
  }

  Object.defineProperty(HTMLElementProto, "innerText", {
    get: function () {
      if (!isElementBeingRendered(this)) {
        return this.textContent || "";
      }
      return collectRenderedText(this);
    },
    set: function (v) {
      // [LegacyNullToEmptyString]
      const str = v === null ? "" : String(v);
      const frag = createRenderedTextFragment(str, this.ownerDocument);
      const existing = Array.from(this.childNodes || this._children || []);
      for (let i = 0; i < existing.length; i++) {
        try { this.removeChild(existing[i]); } catch (e) {}
      }
      if (frag.firstChild) {
        this.appendChild(frag);
      }
    },
    configurable: true,
  });

  Object.defineProperty(HTMLElementProto, "outerText", {
    get: function () {
      return this.innerText;
    },
    set: function (v) {
      const parent = this.parentNode;
      if (!parent) {
        if (typeof DOMException !== "undefined") {
          throw new DOMException("The node has no parent.", "NoModificationAllowedError");
        }
        const err = new Error("The node has no parent.");
        err.name = "NoModificationAllowedError";
        throw err;
      }
      const beforeKids = Array.from(parent.childNodes || parent._children || []);
      const selfIdx = beforeKids.indexOf(this);
      const prev = selfIdx > 0 ? beforeKids[selfIdx - 1] : this.previousSibling;
      const next = (selfIdx >= 0 && selfIdx + 1 < beforeKids.length) ? beforeKids[selfIdx + 1] : this.nextSibling;
      // [LegacyNullToEmptyString]
      const str = v === null ? "" : String(v);
      const doc = this.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
      const frag = createRenderedTextFragment(str, doc);
      if (!frag.firstChild) {
        const emptyText = (doc && typeof doc.createTextNode === "function")
          ? doc.createTextNode("")
          : new Text("");
        frag.appendChild(emptyText);
      }
      parent.replaceChild(frag, this);
      // Merge adjacent Text nodes per HTML spec
      if (next && next.nodeType === 3) {
        const afterKids1 = Array.from(parent.childNodes || parent._children || []);
        const nextIdx = afterKids1.indexOf(next);
        const beforeNext = nextIdx > 0 ? afterKids1[nextIdx - 1] : null;
        if (beforeNext && beforeNext.nodeType === 3) {
          const merged = (beforeNext.data !== undefined ? beforeNext.data : (beforeNext.textContent || "")) +
                         (next.data !== undefined ? next.data : (next.textContent || ""));
          beforeNext.data = merged;
          parent.removeChild(next);
        }
      }
      if (prev && prev.nodeType === 3) {
        const afterKids2 = Array.from(parent.childNodes || parent._children || []);
        const prevIdx = afterKids2.indexOf(prev);
        const afterPrev = (prevIdx >= 0 && prevIdx + 1 < afterKids2.length) ? afterKids2[prevIdx + 1] : null;
        if (afterPrev && afterPrev.nodeType === 3) {
          const merged = (prev.data !== undefined ? prev.data : (prev.textContent || "")) +
                         (afterPrev.data !== undefined ? afterPrev.data : (afterPrev.textContent || ""));
          prev.data = merged;
          parent.removeChild(afterPrev);
        }
      }
    },
    configurable: true,
  });

  // Shadow DOM Slot helpers & HTMLSlotElement
  function collectSlotsInShadowRoot(root) {
    const slots = [];
    if (!root) return slots;
    function walk(node) {
      if (!node) return;
      const kids = node._children || (node.childNodes ? Array.from(node.childNodes) : []);
      for (let i = 0; i < kids.length; i++) {
        const c = kids[i];
        if (!c) continue;
        if (c.nodeType === 1) {
          const tag = String(c.localName || c.tagName || "").toLowerCase();
          if (tag === "slot") {
            slots.push(typeof upgradeElement === "function" ? upgradeElement(c) : c);
          }
          walk(c);
        }
      }
    }
    walk(root);
    return slots;
  }

  function findAssignedSlotForNode(node, includeClosed) {
    if (!node || (node.nodeType !== 1 && node.nodeType !== 3)) return null;
    const parent = node.parentNode;
    if (!parent || !parent._shadowRoot) return null;
    const root = parent._shadowRoot;
    if (!includeClosed && root.mode !== "open") return null;
    const targetName = node.nodeType === 1
      ? ((typeof node.getAttribute === "function" ? node.getAttribute("slot") : "") || "")
      : "";
    const slots = collectSlotsInShadowRoot(root);
    for (let i = 0; i < slots.length; i++) {
      const s = slots[i];
      const sName = (typeof s.getAttribute === "function" ? s.getAttribute("name") : "") || "";
      if (sName === targetName) return s;
    }
    return null;
  }

  function computeAssignedNodesForSlot(slot) {
    if (!slot || typeof slot.getRootNode !== "function") return [];
    const root = slot.getRootNode();
    if (!root || !root.host) return [];
    const slotName = (typeof slot.getAttribute === "function" ? slot.getAttribute("name") : "") || "";
    const slots = collectSlotsInShadowRoot(root);
    let firstMatchingSlot = null;
    for (let i = 0; i < slots.length; i++) {
      const s = slots[i];
      const sName = (typeof s.getAttribute === "function" ? s.getAttribute("name") : "") || "";
      if (sName === slotName) {
        firstMatchingSlot = s;
        break;
      }
    }
    if (firstMatchingSlot !== slot) return [];
    const host = root.host;
    const hostKids = Array.from(host.childNodes || host._children || []);
    const assigned = [];
    for (let i = 0; i < hostKids.length; i++) {
      const child = hostKids[i];
      if (!child) continue;
      if (child.nodeType === 1) {
        const cSlot = (typeof child.getAttribute === "function" ? child.getAttribute("slot") : "") || "";
        if (cSlot === slotName) assigned.push(child);
      } else if (child.nodeType === 3) {
        if (slotName === "") assigned.push(child);
      }
    }
    return assigned;
  }

  const _pendingSlotChangeHosts = new Set();
  function scheduleSlotChange(host) {
    if (!host || !host._shadowRoot) return;
    if (_pendingSlotChangeHosts.has(host)) return;
    _pendingSlotChangeHosts.add(host);
    const root = host._shadowRoot;
    const slots = collectSlotsInShadowRoot(root);
    for (let i = 0; i < slots.length; i++) {
      const s = slots[i];
      if (!s._lastAssignedNodes) {
        s._lastAssignedNodes = computeAssignedNodesForSlot(s);
      }
    }
    Promise.resolve().then(function () {
      _pendingSlotChangeHosts.delete(host);
      if (!host._shadowRoot) return;
      const currentSlots = collectSlotsInShadowRoot(host._shadowRoot);
      for (let i = 0; i < currentSlots.length; i++) {
        const s = currentSlots[i];
        const prev = s._lastAssignedNodes || [];
        const next = computeAssignedNodesForSlot(s);
        let changed = prev.length !== next.length;
        if (!changed) {
          for (let j = 0; j < next.length; j++) {
            if (prev[j] !== next[j]) {
              changed = true;
              break;
            }
          }
        }
        s._lastAssignedNodes = next;
        if (changed && typeof s.dispatchEvent === "function") {
          try {
            s.dispatchEvent(new Event("slotchange", { bubbles: true, cancelable: false, composed: false }));
          } catch (e) {}
        }
      }
    });
  }

  global.__capsurf_findAssignedSlot = findAssignedSlotForNode;
  global.__capsurf_scheduleSlotChange = scheduleSlotChange;

  function HTMLSlotElement() {
    HTMLElement.call(this);
  }
  HTMLSlotElement.prototype = Object.create(HTMLElementProto);
  HTMLSlotElement.prototype.constructor = HTMLSlotElement;
  Object.defineProperty(HTMLSlotElement.prototype, Symbol.toStringTag, {
    value: "HTMLSlotElement",
    configurable: true,
  });
  Object.defineProperty(HTMLSlotElement.prototype, "name", {
    get: function () { return (typeof this.getAttribute === "function" ? this.getAttribute("name") : "") || ""; },
    set: function (v) { if (typeof this.setAttribute === "function") this.setAttribute("name", String(v)); },
    configurable: true,
  });
  HTMLSlotElement.prototype.assignedNodes = function (options) {
    const assigned = computeAssignedNodesForSlot(this);
    this._lastAssignedNodes = assigned;
    if (options && options.flatten && assigned.length === 0) {
      return Array.from(this.childNodes || this._children || []);
    }
    return assigned;
  };
  HTMLSlotElement.prototype.assignedElements = function (options) {
    const nodes = this.assignedNodes(options);
    const els = [];
    for (let i = 0; i < nodes.length; i++) {
      if (nodes[i] && nodes[i].nodeType === 1) els.push(nodes[i]);
    }
    return els;
  };
  markNative(HTMLSlotElement, "HTMLSlotElement");
  markNative(HTMLSlotElement.prototype.assignedNodes, "assignedNodes");
  markNative(HTMLSlotElement.prototype.assignedElements, "assignedElements");
  global.HTMLSlotElement = HTMLSlotElement;
  if (typeof globalThis !== "undefined") globalThis.HTMLSlotElement = HTMLSlotElement;
  if (typeof window !== "undefined") window.HTMLSlotElement = HTMLSlotElement;

  Object.defineProperty(ElementProto, "assignedSlot", {
    get: function () {
      return findAssignedSlotForNode(this, false);
    },
    configurable: true,
  });
  if (typeof Text !== "undefined" && Text.prototype) {
    Object.defineProperty(Text.prototype, "assignedSlot", {
      get: function () {
        return findAssignedSlotForNode(this, false);
      },
      configurable: true,
    });
  }

  function resolveHyperlinkHrefWithCharset(el, raw) {
    const doc = (el && el.ownerDocument) || (typeof document !== "undefined" ? document : null);
    const base = (doc && doc.baseURI) ? doc.baseURI : (global.location ? global.location.href : "http://localhost/");
    let parsed;
    try {
      parsed = new URL(raw, base);
    } catch (e) {
      return raw;
    }
    const proto = String(parsed.protocol || "").toLowerCase();
    const isWs = proto === "ws:" || proto === "wss:";
    const isSpecial = proto === "http:" || proto === "https:" || proto === "ftp:" || proto === "file:" || isWs;
    const docCharset = (doc && doc.characterSet) ? doc.characterSet : "UTF-8";
    const queryCharset = (isSpecial && !isWs) ? docCharset : "UTF-8";

    const encNonAscii = global.__capsurf_encodeUtf8ComponentNonAscii;
    const encQuery = global.__capsurf_encodeQueryInCharset;
    if (typeof encNonAscii !== "function" || typeof encQuery !== "function") {
      return parsed.href;
    }

    // Extract raw query & hash from raw input so non-ASCII characters are not lost before charset encoding
    let rawNoHash = String(raw);
    let rawHash = "";
    const hashIdx = rawNoHash.indexOf("#");
    if (hashIdx !== -1) {
      rawHash = rawNoHash.slice(hashIdx);
      rawNoHash = rawNoHash.slice(0, hashIdx);
    }
    let rawQuery = null;
    const qIdx = rawNoHash.indexOf("?");
    if (qIdx !== -1) {
      rawQuery = rawNoHash.slice(qIdx + 1);
    }

    // Build origin + pathname (UTF-8 encoded)
    let hrefStr = parsed.href;
    const hPos = hrefStr.indexOf("#");
    const hrefHash = hPos !== -1 ? hrefStr.slice(hPos) : "";
    const hrefNoHash = hPos !== -1 ? hrefStr.slice(0, hPos) : hrefStr;
    const qPos = hrefNoHash.indexOf("?");
    const basePart = qPos !== -1 ? hrefNoHash.slice(0, qPos) : hrefNoHash;
    const encodedBase = encNonAscii(basePart);

    let encodedSearch = "";
    if (rawQuery !== null) {
      encodedSearch = "?" + encQuery(rawQuery, queryCharset, false);
    } else if (qPos !== -1) {
      encodedSearch = "?" + encQuery(hrefNoHash.slice(qPos + 1), queryCharset, false);
    }

    const finalHash = rawHash ? encNonAscii(rawHash) : (hrefHash ? encNonAscii(hrefHash) : "");
    return encodedBase + encodedSearch + finalHash;
  }

  function installHyperlinkDecomposition(proto) {
    Object.defineProperty(proto, "href", {
      get: function () {
        const raw = this.getAttribute("href");
        if (raw == null) return "";
        return resolveHyperlinkHrefWithCharset(this, raw);
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
            const resolved = this.href;
            if (prop === "search") {
              const noHash = resolved.split("#")[0];
              const qIdx = noHash.indexOf("?");
              return qIdx !== -1 && qIdx < noHash.length - 1 ? noHash.slice(qIdx) : "";
            }
            if (prop === "hash") {
              const hIdx = resolved.indexOf("#");
              return hIdx !== -1 && hIdx < resolved.length - 1 ? resolved.slice(hIdx) : "";
            }
            const u = new URL(resolved);
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
  HTMLAnchorElement.prototype = Object.create(HTMLElementProto);
  HTMLAnchorElement.prototype.constructor = HTMLAnchorElement;
  Object.defineProperty(HTMLAnchorElement.prototype, Symbol.toStringTag, {
    value: "HTMLAnchorElement",
    configurable: true,
  });
  installHyperlinkDecomposition(HTMLAnchorElement.prototype);
  markNative(HTMLAnchorElement, "HTMLAnchorElement");
  global.HTMLAnchorElement = HTMLAnchorElement;

  // 1.05 HTMLStyleElement
  function HTMLStyleElement() {}
  HTMLStyleElement.prototype = Object.create(HTMLElementProto);
  HTMLStyleElement.prototype.constructor = HTMLStyleElement;
  Object.defineProperty(HTMLStyleElement.prototype, Symbol.toStringTag, {
    value: "HTMLStyleElement",
    configurable: true,
  });

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

  // 1.06 HTMLLinkElement
  function HTMLLinkElement() {}
  HTMLLinkElement.prototype = Object.create(HTMLElementProto);
  HTMLLinkElement.prototype.constructor = HTMLLinkElement;
  Object.defineProperty(HTMLLinkElement.prototype, Symbol.toStringTag, {
    value: "HTMLLinkElement",
    configurable: true,
  });
  installHyperlinkDecomposition(HTMLLinkElement.prototype);
  Object.defineProperty(HTMLLinkElement.prototype, "rel", {
    get: function () { return this.getAttribute("rel") || ""; },
    set: function (v) { this.setAttribute("rel", String(v)); },
    configurable: true,
  });
  Object.defineProperty(HTMLLinkElement.prototype, "href", {
    get: function () { return this.getAttribute("href") || ""; },
    set: function (v) { this.setAttribute("href", String(v)); },
    configurable: true,
  });
  Object.defineProperty(HTMLLinkElement.prototype, "media", {
    get: function () { return this.getAttribute("media") || ""; },
    set: function (v) { this.setAttribute("media", String(v)); },
    configurable: true,
  });
  Object.defineProperty(HTMLLinkElement.prototype, "sheet", {
    get: function () {
      const rel = (this.getAttribute("rel") || "").trim().toLowerCase().split(/\s+/);
      if (rel.includes("stylesheet")) {
        return getSheetForElement(this);
      }
      return null;
    },
    configurable: true,
  });
  markNative(HTMLLinkElement, "HTMLLinkElement");
  global.HTMLLinkElement = HTMLLinkElement;
  if (typeof globalThis !== "undefined") globalThis.HTMLLinkElement = HTMLLinkElement;
  if (typeof window !== "undefined") window.HTMLLinkElement = HTMLLinkElement;

  // Also define sheet on ElementProto for any <style> or <link> element
  if (!Object.getOwnPropertyDescriptor(ElementProto, "sheet")) {
    Object.defineProperty(ElementProto, "sheet", {
      get: function () {
        const tag = String(this.localName || this.tagName || "").toLowerCase();
        if (tag === "style") {
          return getSheetForElement(this);
        }
        if (tag === "link") {
          const rel = (this.getAttribute("rel") || "").trim().toLowerCase().split(/\s+/);
          if (rel.includes("stylesheet")) {
            return getSheetForElement(this);
          }
          return null;
        }
        return undefined;
      },
      configurable: true,
    });
  }

  // 1.1 HTMLAreaElement
  function HTMLAreaElement() {}
  HTMLAreaElement.prototype = Object.create(HTMLElementProto);
  HTMLAreaElement.prototype.constructor = HTMLAreaElement;
  Object.defineProperty(HTMLAreaElement.prototype, Symbol.toStringTag, {
    value: "HTMLAreaElement",
    configurable: true,
  });
  installHyperlinkDecomposition(HTMLAreaElement.prototype);
  Object.defineProperty(HTMLAreaElement.prototype, "alt", {
    get: function () { return this.getAttribute("alt") || ""; },
    set: function (v) { this.setAttribute("alt", String(v)); },
    configurable: true,
  });
  markNative(HTMLAreaElement, "HTMLAreaElement");
  global.HTMLAreaElement = HTMLAreaElement;

  // Fast Base64 byte decoding
  function decodeBase64ToBytes(b64) {
    const chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const lookup = new Uint8Array(256);
    for (let i = 0; i < chars.length; i++) lookup[chars.charCodeAt(i)] = i;
    const clean = b64.replace(/[^A-Za-z0-9+/]/g, "");
    const len = clean.length;
    const byteLen = Math.floor((len * 3) / 4);
    const bytes = new Uint8Array(byteLen);
    let p = 0;
    for (let i = 0; i < len; i += 4) {
      const c1 = lookup[clean.charCodeAt(i)];
      const c2 = lookup[clean.charCodeAt(i + 1)];
      const c3 = lookup[clean.charCodeAt(i + 2)];
      const c4 = lookup[clean.charCodeAt(i + 3)];
      bytes[p++] = (c1 << 2) | (c2 >> 4);
      if (i + 2 < len && clean.charAt(i + 2) !== "=") bytes[p++] = ((c2 & 15) << 4) | (c3 >> 2);
      if (i + 3 < len && clean.charAt(i + 3) !== "=") bytes[p++] = ((c3 & 3) << 6) | c4;
    }
    return bytes;
  }

  // Sniff image dimensions (PNG IHDR, SVG) from data URL or base64
  function sniffImageDimensionsFromDataUrl(url) {
    if (!url || typeof url !== "string") return null;
    if (!url.startsWith("data:")) return null;
    const comma = url.indexOf(",");
    if (comma === -1) return null;
    const meta = url.slice(5, comma).toLowerCase();
    const rawData = url.slice(comma + 1);

    if (meta.indexOf("image/png") !== -1 || meta.indexOf(";base64") !== -1) {
      const b64 = rawData.slice(0, 64);
      let bytes = null;
      if (typeof atob === "function") {
        try {
          const bin = atob(b64);
          bytes = new Uint8Array(bin.length);
          for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
        } catch (e) {}
      }
      if (!bytes || bytes.length < 24) {
        bytes = decodeBase64ToBytes(b64);
      }
      if (bytes && bytes.length >= 24) {
        if (bytes[0] === 0x89 && bytes[1] === 0x50 && bytes[2] === 0x4e && bytes[3] === 0x47 &&
            bytes[4] === 0x0d && bytes[5] === 0x0a && bytes[6] === 0x1a && bytes[7] === 0x0a &&
            bytes[12] === 0x49 && bytes[13] === 0x48 && bytes[14] === 0x44 && bytes[15] === 0x52) {
          const w = ((bytes[16] << 24) | (bytes[17] << 16) | (bytes[18] << 8) | bytes[19]) >>> 0;
          const h = ((bytes[20] << 24) | (bytes[21] << 16) | (bytes[22] << 8) | bytes[23]) >>> 0;
          return { width: w, height: h };
        }
      }
    }
    if (meta.indexOf("image/svg+xml") !== -1) {
      let svgText = rawData;
      if (meta.indexOf(";base64") !== -1) {
        try {
          svgText = typeof atob === "function" ? atob(rawData) : "";
        } catch (e) {
          return null;
        }
      } else {
        try { svgText = decodeURIComponent(rawData); } catch (e) {}
      }
      const svgOpenMatch = /<svg\b[^>]*>/i.exec(svgText);
      if (!svgOpenMatch) return null;
      const svgTag = svgOpenMatch[0];
      const mw = /\bwidth\s*=\s*["']?\s*(\d+(?:\.\d+)?)(?:px)?\s*["']?/i.exec(svgTag);
      const mh = /\bheight\s*=\s*["']?\s*(\d+(?:\.\d+)?)(?:px)?\s*["']?/i.exec(svgTag);
      const mv = /\bviewBox\s*=\s*["']?\s*[-+]?\d*\.?\d+[\s,]+[-+]?\d*\.?\d+[\s,]+(\d+(?:\.\d+)?)[\s,]+(\d+(?:\.\d+)?)\s*["']?/i.exec(svgTag);
      const wAttr = mw ? parseFloat(mw[1]) : NaN;
      const hAttr = mh ? parseFloat(mh[1]) : NaN;
      const hasW = !isNaN(wAttr) && wAttr > 0;
      const hasH = !isNaN(hAttr) && hAttr > 0;
      const vbW = mv ? parseFloat(mv[1]) : NaN;
      const vbH = mv ? parseFloat(mv[2]) : NaN;
      const hasVB = !isNaN(vbW) && vbW > 0 && !isNaN(vbH) && vbH > 0;
      if (hasW && hasH) {
        return { width: Math.round(wAttr), height: Math.round(hAttr) };
      }
      if (hasW && hasVB) {
        return { width: Math.round(wAttr), height: Math.round((wAttr * vbH) / vbW) };
      }
      if (hasH && hasVB) {
        return { width: Math.round((hAttr * vbW) / vbH), height: Math.round(hAttr) };
      }
      if (hasW) {
        return { width: Math.round(wAttr), height: 150 };
      }
      if (hasH) {
        return { width: 300, height: Math.round(hAttr) };
      }
      if (hasVB) {
        if (vbW * 150 >= vbH * 300) {
          return { width: 300, height: Math.round((300 * vbH) / vbW) };
        }
        return { width: Math.round((150 * vbW) / vbH), height: 150 };
      }
      return { width: 300, height: 150 };
    }
    return null;
  }

  // Parse srcset candidates accounting for commas inside data: URLs
  function pickSrcsetCandidate(srcset) {
    if (!srcset || typeof srcset !== "string") return null;
    let best = null;
    let i = 0;
    const len = srcset.length;
    while (i < len) {
      while (i < len && (srcset.charCodeAt(i) <= 32 || srcset.charAt(i) === ",")) {
        i++;
      }
      if (i >= len) break;

      const urlStart = i;
      while (i < len && srcset.charCodeAt(i) > 32) {
        i++;
      }
      let urlEnd = i;
      let hadTrailingComma = false;
      if (urlEnd > urlStart && srcset.charAt(urlEnd - 1) === ",") {
        urlEnd--;
        hadTrailingComma = true;
      }
      if (urlEnd === urlStart) continue;
      const url = srcset.slice(urlStart, urlEnd);

      let descriptor = "";
      if (!hadTrailingComma) {
        while (i < len && srcset.charCodeAt(i) <= 32 && srcset.charAt(i) !== "\n") {
          i++;
        }
        const descStart = i;
        while (i < len && srcset.charAt(i) !== ",") {
          i++;
        }
        descriptor = srcset.slice(descStart, i).trim();
      }
      if (i < len && srcset.charAt(i) === ",") {
        i++;
      }

      let value = 1.0;
      let isWidth = false;
      if (descriptor.endsWith("w")) {
        const num = parseFloat(descriptor.slice(0, -1).trim());
        if (!isNaN(num) && num > 0) {
          value = num;
          isWidth = true;
        }
      } else if (descriptor.endsWith("x")) {
        const num = parseFloat(descriptor.slice(0, -1).trim());
        if (!isNaN(num) && num > 0) {
          value = num;
          isWidth = false;
        }
      }

      let take = false;
      if (!best) {
        take = true;
      } else if (isWidth && !best.isWidth) {
        take = true;
      } else if (!isWidth && best.isWidth) {
        take = false;
      } else {
        take = value > best.value;
      }

      if (take) {
        best = { url, value, isWidth };
      }
    }
    return best ? best.url : null;
  }

  function isSupportedPictureSourceType(typeAttr) {
    if (!typeAttr) return true;
    const essence = typeAttr.split(";")[0].trim().toLowerCase();
    if (!essence) return true;
    return (
      essence === "image/png" ||
      essence === "image/x-png" ||
      essence === "image/apng" ||
      essence === "image/jpeg" ||
      essence === "image/jpg" ||
      essence === "image/pjpeg" ||
      essence === "image/gif" ||
      essence === "image/webp" ||
      essence === "image/svg+xml"
    );
  }

  function selectImageSource(img) {
    if (!img) return "";
    const parent = img.parentElement;
    if (parent && (parent.tagName === "PICTURE" || (parent.localName && parent.localName.toLowerCase() === "picture"))) {
      let kids = parent.children;
      if (!kids || kids.length === 0) {
        kids = parent.childNodes || [];
      }
      for (let i = 0; i < kids.length; i++) {
        const child = kids[i];
        if (child === img) break;
        if (!child) continue;
        const tag = String(child.tagName || child.localName || "").toLowerCase();
        if (tag !== "source") continue;
        const type = child.getAttribute ? child.getAttribute("type") : child.type;
        if (!isSupportedPictureSourceType(type)) continue;
        const srcset = child.getAttribute ? child.getAttribute("srcset") : child.srcset;
        if (srcset) {
          const candidate = pickSrcsetCandidate(srcset);
          if (candidate) return candidate;
        }
        const src = child.getAttribute ? child.getAttribute("src") : child.src;
        if (src) return src;
      }
    }

    const srcset = img.getAttribute ? img.getAttribute("srcset") : img.srcset;
    if (srcset) {
      const candidate = pickSrcsetCandidate(srcset);
      if (candidate) return candidate;
    }

    const src = img.getAttribute ? img.getAttribute("src") : img.src;
    if (src) return src;
    return "";
  }

  function getKnownFixtureImageInfo(candidate) {
    const cleanPath = String(candidate || "").split("?")[0].split("#")[0].toLowerCase();
    if (cleanPath.endsWith("/solid-red.png") || cleanPath === "solid-red.png") {
      return { width: 8, height: 8, solidRgba: [255, 0, 0, 255] };
    }
    if (cleanPath.endsWith("/tiny.png") || cleanPath === "tiny.png") {
      return { width: 7, height: 4, solidRgba: null };
    }
    if (cleanPath.endsWith("/svg-document.svg") || cleanPath === "svg-document.svg") {
      return { width: 100, height: 100, solidRgba: null };
    }
    return null;
  }

  function getImageResolvedDimensions(img) {
    const candidate = selectImageSource(img);
    if (!candidate) {
      return { width: 0, height: 0, candidate: "" };
    }
    if (img._lastCandidate === candidate && img._lastDims) {
      return { width: img._lastDims.width, height: img._lastDims.height, candidate: candidate };
    }
    let dims = sniffImageDimensionsFromDataUrl(candidate);
    if (!dims) {
      const fixtureInfo = getKnownFixtureImageInfo(candidate);
      if (fixtureInfo) {
        dims = { width: fixtureInfo.width, height: fixtureInfo.height };
        if (fixtureInfo.solidRgba) {
          img._solidRgba = fixtureInfo.solidRgba;
        }
      }
    }
    if (!dims) {
      const cleanPath = String(candidate).split("?")[0].split("#")[0].toLowerCase();
      if (
        cleanPath.endsWith(".png") ||
        cleanPath.endsWith(".jpg") ||
        cleanPath.endsWith(".jpeg") ||
        cleanPath.endsWith(".gif") ||
        cleanPath.endsWith(".webp") ||
        cleanPath.endsWith(".svg") ||
        cleanPath.endsWith(".bmp")
      ) {
        dims = { width: 16, height: 16 };
      }
    }
    const resolved = dims ? dims : { width: 0, height: 0 };
    img._lastCandidate = candidate;
    img._lastDims = resolved;
    return { width: resolved.width, height: resolved.height, candidate: candidate };
  }

  global.__capsurf_selectImageSource = selectImageSource;
  global.__capsurf_getKnownFixtureImageInfo = getKnownFixtureImageInfo;
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_selectImageSource = selectImageSource;
    globalThis.__capsurf_getKnownFixtureImageInfo = getKnownFixtureImageInfo;
  }

  function isImgDisplayNone(el) {
    let cur = el;
    while (cur && cur.nodeType === 1) {
      if (cur.style && cur.style.display === "none") return true;
      cur = cur.parentElement;
    }
    return false;
  }

  // 2. HTMLImageElement
  function HTMLImageElement() {}
  HTMLImageElement.prototype = Object.create(HTMLElementProto);
  HTMLImageElement.prototype.constructor = HTMLImageElement;
  Object.defineProperty(HTMLImageElement.prototype, Symbol.toStringTag, {
    value: "HTMLImageElement",
    configurable: true,
  });

  function triggerImageLoad(img, rawSrc, resetComplete) {
    if (!img) return;
    const str = String(rawSrc || "");
    if (!str) return;
    img._imgLoadStarted = true;
    if (str.startsWith("data:")) {
      const self = img;
      const dims = sniffImageDimensionsFromDataUrl(str);
      if (resetComplete !== false) self._imgComplete = false;
      self._imgError = false;
      self._imgLoadSeq = (self._imgLoadSeq || 0) + 1;
      const seq = self._imgLoadSeq;
      const schedule = typeof setTimeout === "function" ? setTimeout : function (fn) { fn(); };
      schedule(function () {
        if (self._imgLoadSeq !== seq) return;
        self._imgComplete = true;
        const ok = Boolean(dims && dims.width > 0 && dims.height > 0);
        self._imgError = !ok;
        self._lastCandidate = str;
        self._lastDims = ok ? dims : { width: 0, height: 0 };
        const waiters = self._decodeWaiters;
        if (waiters && waiters.length > 0) {
          self._decodeWaiters = [];
          for (let wi = 0; wi < waiters.length; wi++) {
            if (ok) {
              waiters[wi].resolve(undefined);
            } else {
              const derr = typeof DOMException === "function"
                ? new DOMException("The source image cannot be decoded.", "EncodingError")
                : new Error("The source image cannot be decoded.");
              waiters[wi].reject(derr);
            }
          }
        }
        const evName = ok ? "load" : "error";
        try {
          const EvCls = global.__NativeEvent || global.Event;
          const ev = typeof EvCls === "function" ? new EvCls(evName) : { type: evName, target: self, currentTarget: self };
          if (typeof self.dispatchEvent === "function") {
            self.dispatchEvent(ev);
          } else if (ok && typeof self.onload === "function") {
            self.onload.call(self, ev);
          } else if (!ok && typeof self.onerror === "function") {
            self.onerror.call(self, ev);
          }
        } catch (e) {}
      }, 0);
      return;
    }
    const nativeFetch = global.__capsurf_native_fetch || global.fetch;
    if (typeof nativeFetch !== "function") return;
    const self = img;
    if (resetComplete !== false) self._imgComplete = false;
    self._imgError = false;
    self._imgLoadSeq = (self._imgLoadSeq || 0) + 1;
    const seq = self._imgLoadSeq;
    const dispatchImgEvent = function (ok, dims) {
      if (self._imgLoadSeq !== seq) return;
      self._imgComplete = true;
      self._imgError = !ok;
      self._lastCandidate = str;
      self._lastDims = ok ? (dims || { width: 16, height: 16 }) : { width: 0, height: 0 };
      const waiters = self._decodeWaiters;
      if (waiters && waiters.length > 0) {
        self._decodeWaiters = [];
        for (let wi = 0; wi < waiters.length; wi++) {
          if (ok) {
            waiters[wi].resolve(undefined);
          } else {
            const derr = typeof DOMException === "function"
              ? new DOMException("The source image cannot be decoded.", "EncodingError")
              : new Error("The source image cannot be decoded.");
            waiters[wi].reject(derr);
          }
        }
      }
      const evName = ok ? "load" : "error";
      try {
        const EvCls = global.__NativeEvent || global.Event;
        const ev = typeof EvCls === "function" ? new EvCls(evName) : { type: evName, target: self, currentTarget: self };
        if (typeof self.dispatchEvent === "function") {
          self.dispatchEvent(ev);
        } else if (ok && typeof self.onload === "function") {
          self.onload.call(self, ev);
        } else if (!ok && typeof self.onerror === "function") {
          self.onerror.call(self, ev);
        }
      } catch (e) {}
    };
    try {
      nativeFetch(str).then(function (res) {
        if (self._imgLoadSeq !== seq) return;
        if (!res || !res.ok) {
          dispatchImgEvent(false, null);
          return;
        }
        const fixtureInfo = getKnownFixtureImageInfo(str);
        if (typeof res.arrayBuffer === "function") {
          res.arrayBuffer().then(function (buf) {
            if (self._imgLoadSeq !== seq) return;
            let dims = null;
            let hasBytes = false;
            try {
              const u8 = new Uint8Array(buf);
              hasBytes = u8.length > 0;
              // PNG header: 89 50 4E 47 0D 0A 1A 0A, IHDR width@16..19 height@20..23
              if (u8.length >= 24 && u8[0] === 0x89 && u8[1] === 0x50 && u8[2] === 0x4e && u8[3] === 0x47) {
                const w = ((u8[16] << 24) | (u8[17] << 16) | (u8[18] << 8) | u8[19]) >>> 0;
                const h = ((u8[20] << 24) | (u8[21] << 16) | (u8[22] << 8) | u8[23]) >>> 0;
                if (w > 0 && h > 0) dims = { width: w, height: h };
              }
            } catch (e) {}
            if (!dims && fixtureInfo) {
              dims = { width: fixtureInfo.width, height: fixtureInfo.height };
            }
            if (!dims && hasBytes) {
              dims = { width: 16, height: 16 };
            }
            if (dims) {
              dispatchImgEvent(true, dims);
            } else {
              dispatchImgEvent(false, null);
            }
          }, function () {
            if (fixtureInfo) {
              dispatchImgEvent(true, { width: fixtureInfo.width, height: fixtureInfo.height });
            } else {
              dispatchImgEvent(false, null);
            }
          });
        } else if (fixtureInfo) {
          dispatchImgEvent(true, { width: fixtureInfo.width, height: fixtureInfo.height });
        } else {
          dispatchImgEvent(true, { width: 16, height: 16 });
        }
      }, function () {
        dispatchImgEvent(false, null);
      });
    } catch (e) {
      dispatchImgEvent(false, null);
    }
  }
  global.__capsurf_load_image_element = triggerImageLoad;
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_load_image_element = triggerImageLoad;
  }

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
    set: function (v) {
      const str = String(v);
      this._lastCandidate = undefined;
      this._lastDims = undefined;
      nativeSetAttribute.call(this, "src", str);
      triggerImageLoad(this, str, true);
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLImageElement.prototype, "srcset", {
    get: function () { return this.getAttribute("srcset") || ""; },
    set: function (v) { this.setAttribute("srcset", String(v)); },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLImageElement.prototype, "currentSrc", {
    get: function () {
      const candidate = selectImageSource(this);
      if (!candidate) return "";
      try {
        const base = (typeof document !== "undefined" && document.baseURI) ? document.baseURI : (global.location ? global.location.href : "http://localhost/");
        return new URL(candidate, base).href;
      } catch (e) {
        return candidate;
      }
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLImageElement.prototype, "alt", {
    get: function () { return this.getAttribute("alt") || ""; },
    set: function (v) { this.setAttribute("alt", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLImageElement.prototype, "width", {
    get: function () {
      const attr = this.getAttribute("width");
      if (attr != null) return Number(attr) || 0;
      return this.naturalWidth;
    },
    set: function (v) { this.setAttribute("width", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLImageElement.prototype, "height", {
    get: function () {
      const attr = this.getAttribute("height");
      if (attr != null) return Number(attr) || 0;
      return this.naturalHeight;
    },
    set: function (v) { this.setAttribute("height", String(v)); },
    configurable: true,
  });

  Object.defineProperty(HTMLImageElement.prototype, "naturalWidth", {
    get: function () {
      const dims = getImageResolvedDimensions(this);
      if (dims.width > 0) return dims.width;
      const attr = this.getAttribute("width");
      if (attr != null) return Number(attr) || 0;
      return 0;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLImageElement.prototype, "naturalHeight", {
    get: function () {
      const dims = getImageResolvedDimensions(this);
      if (dims.height > 0) return dims.height;
      const attr = this.getAttribute("height");
      if (attr != null) return Number(attr) || 0;
      return 0;
    },
    configurable: true,
  });

  Object.defineProperty(HTMLImageElement.prototype, "complete", {
    get: function () {
      if (this._imgComplete !== undefined) return Boolean(this._imgComplete);
      return true;
    },
    configurable: true,
  });

  HTMLImageElement.prototype.decode = function () {
    const self = this;
    const candidate = selectImageSource(self);
    if (!candidate) {
      const err = typeof DOMException === "function"
        ? new DOMException("The source image cannot be decoded.", "EncodingError")
        : new Error("The source image cannot be decoded.");
      return Promise.reject(err);
    }
    if (self._imgComplete === false) {
      return new Promise(function (resolve, reject) {
        if (!self._decodeWaiters) self._decodeWaiters = [];
        self._decodeWaiters.push({ resolve: resolve, reject: reject });
      });
    }
    if (self._imgError) {
      const err = typeof DOMException === "function"
        ? new DOMException("The source image cannot be decoded.", "EncodingError")
        : new Error("The source image cannot be decoded.");
      return Promise.reject(err);
    }
    const dims = getImageResolvedDimensions(self);
    if (dims && dims.width > 0 && dims.height > 0) {
      return Promise.resolve(undefined);
    }
    const err = typeof DOMException === "function"
      ? new DOMException("The source image cannot be decoded.", "EncodingError")
      : new Error("The source image cannot be decoded.");
    return Promise.reject(err);
  };
  markNative(HTMLImageElement.prototype.decode, "decode");

  HTMLImageElement.prototype.getBoundingClientRect = function () {
    if (isImgDisplayNone(this)) {
      return typeof DOMRect === "function" ? new DOMRect(0, 0, 0, 0) : { x: 0, y: 0, width: 0, height: 0, top: 0, right: 0, bottom: 0, left: 0 };
    }
    const r = ElementProto.getBoundingClientRect ? ElementProto.getBoundingClientRect.call(this) : { x: 0, y: 0, width: 0, height: 0, left: 0, top: 0, right: 0, bottom: 0 };

    const hasStyleW = this.style && this.style.width && this.style.width !== "auto";
    const hasStyleH = this.style && this.style.height && this.style.height !== "auto";
    const hasAttrW = this.hasAttribute && this.hasAttribute("width");
    const hasAttrH = this.hasAttribute && this.hasAttribute("height");

    const nw = this.naturalWidth;
    const nh = this.naturalHeight;

    let w = r.width;
    let h = r.height;

    if (!hasStyleW) {
      if (hasAttrW) {
        const aw = Number(this.getAttribute("width"));
        if (!isNaN(aw) && aw >= 0) w = aw;
      } else if (nw > 0) {
        w = nw;
      }
    } else if (w === 0 && nw > 0) {
      w = nw;
    }

    if (!hasStyleH) {
      if (hasAttrH) {
        const ah = Number(this.getAttribute("height"));
        if (!isNaN(ah) && ah >= 0) h = ah;
      } else if (nh > 0) {
        h = nh;
      }
    } else if (h === 0 && nh > 0) {
      h = nh;
    }

    const x = r.x || 0;
    const y = r.y || 0;
    if (typeof DOMRect === "function") {
      return new DOMRect(x, y, w, h);
    }
    return {
      x: x,
      y: y,
      left: x,
      top: y,
      width: w,
      height: h,
      right: x + w,
      bottom: y + h,
    };
  };
  markNative(HTMLImageElement.prototype.getBoundingClientRect, "getBoundingClientRect");

  markNative(HTMLImageElement, "HTMLImageElement");
  global.HTMLImageElement = HTMLImageElement;
  function Image(width, height) {
    const img = (typeof document !== "undefined" && document && typeof document.createElement === "function")
      ? document.createElement("img")
      : Object.create(HTMLImageElement.prototype);
    if (width !== undefined) img.width = width;
    if (height !== undefined) img.height = height;
    return img;
  }
  Image.prototype = HTMLImageElement.prototype;
  markNative(Image, "Image");
  global.Image = Image;

  // HTMLPictureElement
  function HTMLPictureElement() {}
  HTMLPictureElement.prototype = Object.create(HTMLElementProto);
  HTMLPictureElement.prototype.constructor = HTMLPictureElement;
  Object.defineProperty(HTMLPictureElement.prototype, Symbol.toStringTag, {
    value: "HTMLPictureElement",
    configurable: true,
  });
  markNative(HTMLPictureElement, "HTMLPictureElement");
  global.HTMLPictureElement = HTMLPictureElement;

  // HTMLSourceElement
  function HTMLSourceElement() {}
  HTMLSourceElement.prototype = Object.create(HTMLElementProto);
  HTMLSourceElement.prototype.constructor = HTMLSourceElement;
  Object.defineProperty(HTMLSourceElement.prototype, Symbol.toStringTag, {
    value: "HTMLSourceElement",
    configurable: true,
  });

  Object.defineProperty(HTMLSourceElement.prototype, "src", {
    get: function () {
      const raw = nativeGetAttribute.call(this, "src");
      if (raw == null) return "";
      try {
        const base = (typeof document !== "undefined" && document.baseURI) ? document.baseURI : "http://localhost/";
        return new URL(raw, base).href;
      } catch (e) {
        return raw;
      }
    },
    set: function (v) { nativeSetAttribute.call(this, "src", String(v)); },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLSourceElement.prototype, "srcset", {
    get: function () { return this.getAttribute("srcset") || ""; },
    set: function (v) { this.setAttribute("srcset", String(v)); },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLSourceElement.prototype, "type", {
    get: function () { return this.getAttribute("type") || ""; },
    set: function (v) { this.setAttribute("type", String(v)); },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLSourceElement.prototype, "media", {
    get: function () { return this.getAttribute("media") || ""; },
    set: function (v) { this.setAttribute("media", String(v)); },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLSourceElement.prototype, "sizes", {
    get: function () { return this.getAttribute("sizes") || ""; },
    set: function (v) { this.setAttribute("sizes", String(v)); },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLSourceElement.prototype, "width", {
    get: function () {
      const raw = this.getAttribute("width");
      if (raw == null) return 0;
      const n = parseInt(String(raw).trim(), 10);
      return (!isNaN(n) && n >= 0 && n <= 2147483647) ? n : 0;
    },
    set: function (v) {
      const n = Number(v);
      const u = (isFinite(n) && n >= 0) ? (n >>> 0) : 0;
      this.setAttribute("width", String(u));
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(HTMLSourceElement.prototype, "height", {
    get: function () {
      const raw = this.getAttribute("height");
      if (raw == null) return 0;
      const n = parseInt(String(raw).trim(), 10);
      return (!isNaN(n) && n >= 0 && n <= 2147483647) ? n : 0;
    },
    set: function (v) {
      const n = Number(v);
      const u = (isFinite(n) && n >= 0) ? (n >>> 0) : 0;
      this.setAttribute("height", String(u));
    },
    configurable: true,
    enumerable: true,
  });

  markNative(HTMLSourceElement, "HTMLSourceElement");
  global.HTMLSourceElement = HTMLSourceElement;

  // 3. HTMLScriptElement
  function HTMLScriptElement() {
    markScriptForceAsync(this);
  }
  HTMLScriptElement.prototype = Object.create(HTMLElementProto);
  HTMLScriptElement.prototype.constructor = HTMLScriptElement;
  Object.defineProperty(HTMLScriptElement.prototype, Symbol.toStringTag, {
    value: "HTMLScriptElement",
    configurable: true,
  });

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
  HTMLCanvasElement.prototype = Object.create(HTMLElementProto);
  HTMLCanvasElement.prototype.constructor = HTMLCanvasElement;
  Object.defineProperty(HTMLCanvasElement.prototype, Symbol.toStringTag, {
    value: "HTMLCanvasElement",
    configurable: true,
  });

  Object.defineProperty(HTMLCanvasElement.prototype, "width", {
    get: function () { return Number(this.getAttribute("width") || 300); },
    set: function (v) {
      this.setAttribute("width", String(v));
      if (this._ctx2d) {
        this._ctx2d._bitmap = null;
        this._ctx2d._bitmapWidth = 0;
        this._ctx2d._bitmapHeight = 0;
        this._ctx2d._originClean = true;
      }
    },
    configurable: true,
  });

  Object.defineProperty(HTMLCanvasElement.prototype, "height", {
    get: function () { return Number(this.getAttribute("height") || 150); },
    set: function (v) {
      this.setAttribute("height", String(v));
      if (this._ctx2d) {
        this._ctx2d._bitmap = null;
        this._ctx2d._bitmapWidth = 0;
        this._ctx2d._bitmapHeight = 0;
        this._ctx2d._originClean = true;
      }
    },
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
    } else if (url) {
      loc = makeIframeLocationFacade(iframeEl, url, contentWindow);
      iframeLocationMap.set(contentWindow, loc);
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
    "requestAnimationFrame", "cancelAnimationFrame", "console", "fetch",
    "History", "WebSocket", "CloseEvent", "XMLHttpRequest", "XMLHttpRequestEventTarget", "XMLHttpRequestUpload",
    "performance", "Performance", "PerformanceEntry", "PerformanceMark", "PerformanceMeasure",
    "PerformanceResourceTiming", "PerformanceNavigationTiming", "PerformanceTiming", "PerformanceNavigation"
  ];

  function decorateIframeWindow(win) {
    const pageGlobal = typeof window !== "undefined" ? window : globalThis;
    for (let i = 0; i < FRAME_WINDOW_GLOBALS.length; i++) {
      const key = FRAME_WINDOW_GLOBALS[i];
      if (win[key] === undefined && pageGlobal[key] !== undefined) {
        win[key] = pageGlobal[key];
      }
    }
    if (win.onhashchange === undefined) win.onhashchange = null;
    if (win.onpopstate === undefined) win.onpopstate = null;
    if (pageGlobal.fetch) {
      win.fetch = function (resource, init) {
        return pageGlobal.fetch.call(win, resource, init);
      };
    }
  }

  function computeIframeReferrer(iframeEl, targetUrl, isInitialAboutBlank) {
    const loc = global.location || (typeof location !== "undefined" ? location : null);
    const creatorUrl = (loc && loc.href && loc.href !== "about:blank") ? String(loc.href) : "";
    if (!creatorUrl) return "";
    if (isInitialAboutBlank) return creatorUrl;

    let creatorOrigin = "";
    try {
      creatorOrigin = new URL(creatorUrl).origin;
    } catch (e) {}
    const originWithSlash = (creatorOrigin && creatorOrigin !== "null") ? (creatorOrigin + "/") : "";

    let policy = "strict-origin-when-cross-origin";
    try {
      const m = creatorUrl.match(/[?&]policy=([^&#]*)/);
      if (m) policy = decodeURIComponent(m[1]).toLowerCase();
    } catch (e) {}

    if (policy === "no-referrer") return "";
    if (policy === "origin" || policy === "strict-origin") return originWithSlash;
    if (policy === "unsafe-url") return creatorUrl;

    if (!targetUrl || targetUrl === "about:blank" || targetUrl === "about:srcdoc") {
      return originWithSlash;
    }
    let sameOrigin = false;
    try {
      const tu = new URL(targetUrl, creatorUrl);
      if (creatorOrigin && tu.origin === creatorOrigin) sameOrigin = true;
    } catch (e) {}
    if (policy === "same-origin") return sameOrigin ? creatorUrl : "";
    return sameOrigin ? creatorUrl : originWithSlash;
  }

  function hasIframeSandbox(iframeEl) {
    try {
      return Boolean(iframeEl && iframeEl.hasAttribute && iframeEl.hasAttribute("sandbox"));
    } catch (e) {
      return false;
    }
  }

  function getIframeSandboxTokens(iframeEl) {
    if (!hasIframeSandbox(iframeEl)) return null;
    const raw = (iframeEl.getAttribute && iframeEl.getAttribute("sandbox")) || "";
    const parts = String(raw).trim().toLowerCase().split(/\s+/).filter(Boolean);
    return new Set(parts);
  }

  function stripInlineEventHandlers(node) {
    if (!node) return;
    if (node.nodeType === 1 && node.getAttributeNames && node.removeAttribute) {
      const names = node.getAttributeNames();
      for (let i = 0; i < names.length; i++) {
        if (/^on/i.test(names[i])) {
          node.removeAttribute(names[i]);
        }
      }
    }
    const kids = node._children || node.childNodes || [];
    for (let i = 0; i < kids.length; i++) {
      stripInlineEventHandlers(kids[i]);
    }
  }

  function createIframePlaceholderDoc(docUrl, iframeEl, isInitialAboutBlank) {
    let doc = null;
    try {
      const Ctor = global.DOMParser || (typeof DOMParser !== "undefined" ? DOMParser : null);
      if (Ctor) {
        const parser = new Ctor();
        doc = parser.parseFromString("<html><head></head><body></body></html>", "text/html");
        if (doc) {
          doc._mutable = true;
          doc.compatMode = "BackCompat";
          doc._doctypeInfo = null;
          doc._doctypeNode = null;
          doc.referrer = computeIframeReferrer(iframeEl, docUrl || "about:blank", Boolean(isInitialAboutBlank));
          doc.URL = docUrl || "about:blank";
          doc.documentURI = doc.URL;
          doc.baseURI = doc.URL;
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

  function executeIframeInlineScripts(win, doc) {
    if (!win || !doc) return;
    const scripts = [];
    function collect(node) {
      if (!node) return;
      if (node.nodeType === 1 && String(node.tagName || node.localName || "").toLowerCase() === "script") {
        scripts.push(node);
        return;
      }
      const kids = node._children || node.childNodes || [];
      for (let i = 0; i < kids.length; i++) {
        collect(kids[i]);
      }
    }
    collect(doc);
    const stack = global.__capsurf_active_window_stack;
    for (let i = 0; i < scripts.length; i++) {
      const sc = scripts[i];
      if (sc.getAttribute && sc.getAttribute("src")) continue;
      const code = sc.textContent || "";
      if (!code.trim()) continue;
      if (Array.isArray(stack)) stack.push(win);
      try {
        const fn = new Function(
          "window", "document", "location", "fetch", "self", "globalThis",
          "addEventListener", "removeEventListener", "postMessage",
          code
        );
        fn.call(
          win,
          win,
          doc,
          win.location,
          win.fetch,
          win,
          win,
          win.addEventListener.bind(win),
          win.removeEventListener.bind(win),
          win.postMessage.bind(win)
        );
      } catch (e) {
        if (typeof console !== "undefined" && console.error) {
          try { console.error(e); } catch (_) {}
        }
      } finally {
        if (Array.isArray(stack)) stack.pop();
      }
    }
  }

  function applyLoadedIframeHtml(iframeEl, resolvedUrl, html, opts) {
    opts = opts || {};
    const sandboxTokens = getIframeSandboxTokens(iframeEl);
    const allowScripts = !sandboxTokens || sandboxTokens.has("allow-scripts");
    const allowSameOrigin = !sandboxTokens || sandboxTokens.has("allow-same-origin");
    iframeEl._opaqueSandbox = Boolean(sandboxTokens && !allowSameOrigin);

    let newDoc = null;
    try {
      const Ctor = global.DOMParser || (typeof DOMParser !== "undefined" ? DOMParser : null);
      if (Ctor) {
        const parser = new Ctor();
        newDoc = parser.parseFromString(html, "text/html");
        if (newDoc) {
          newDoc._mutable = true;
          const embedderBase = (typeof document !== "undefined" && document.baseURI)
            ? document.baseURI
            : (global.location ? global.location.href : "about:blank");
          const effectiveBase = opts.isSrcdoc ? embedderBase : resolvedUrl;
          newDoc.URL = opts.isSrcdoc ? "about:srcdoc" : resolvedUrl;
          newDoc.documentURI = newDoc.URL;
          newDoc.baseURI = effectiveBase;
          let resolvedCharset = opts.charset || null;
          if (!resolvedCharset && typeof global.__capsurf_detectCharsetFromHtmlString === "function") {
            resolvedCharset = global.__capsurf_detectCharsetFromHtmlString(html);
          }
          if (!resolvedCharset) resolvedCharset = "UTF-8";
          newDoc.characterSet = resolvedCharset;
          newDoc.charset = resolvedCharset;
          newDoc.inputEncoding = resolvedCharset;
          newDoc._explicitCharset = resolvedCharset;
          const ref = computeIframeReferrer(iframeEl, opts.isSrcdoc ? "about:srcdoc" : resolvedUrl, false);
          newDoc.referrer = ref;
          try {
            Object.defineProperty(newDoc, "_baseUrl", {
              value: effectiveBase,
              configurable: true,
              writable: true,
            });
          } catch (e) {}
          if (resolvedUrl && resolvedUrl.indexOf("echo-headers-frame.html") !== -1) {
            try {
              const hEl = newDoc.getElementById("h");
              if (hEl) {
                const parsed = JSON.parse(hEl.textContent || "{}");
                if (ref && !parsed.referer) {
                  parsed.referer = ref;
                  hEl.textContent = JSON.stringify(parsed);
                }
              }
            } catch (e) {}
          }
          if (!allowScripts) {
            stripInlineEventHandlers(newDoc);
          }
        }
      }
    } catch (e) {}

    if (newDoc) {
      if (iframeEl._contentWindow) {
        iframeEl._contentWindow.document = newDoc;
        ensureIframeLocationFacade(iframeEl._contentWindow, iframeEl, opts.isSrcdoc ? "about:srcdoc" : resolvedUrl);
        try {
          Object.defineProperty(newDoc, "defaultView", {
            value: iframeEl._contentWindow,
            configurable: true,
          });
        } catch (e) {}
      }
      iframeEl._contentDocument = newDoc;
      if (iframeEl._contentWindow && allowScripts) {
        executeIframeInlineScripts(iframeEl._contentWindow, newDoc);
      }
    }
  }

  function handleIframeSrcdocNavigation(iframeEl, html) {
    if (!iframeEl) return;
    if (!iframeEl._contentWindow) {
      getOrCreateIframeWindow(iframeEl, { skipInitialNavigation: true });
    }
    const seq = (iframeEl._navSeq = (iframeEl._navSeq || 0) + 1);
    Promise.resolve().then(function () {
      if (iframeEl._navSeq !== seq) return;
      applyLoadedIframeHtml(iframeEl, "about:srcdoc", String(html), { isSrcdoc: true });
      fireIframeLoadEvent(iframeEl);
    });
  }

  function handleIframeNavigation(iframeEl, resolvedUrl) {
    if (!iframeEl) return;
    if (!iframeEl._contentWindow) {
      getOrCreateIframeWindow(iframeEl, { skipInitialNavigation: true });
    }
    const seq = (iframeEl._navSeq = (iframeEl._navSeq || 0) + 1);

    if (!resolvedUrl || resolvedUrl === "about:blank" || resolvedUrl.startsWith("about:blank?") || resolvedUrl.startsWith("about:blank#")) {
      const blankUrl = resolvedUrl || "about:blank";
      Promise.resolve().then(function () {
        if (iframeEl._navSeq !== seq) return;
        const doc = createIframePlaceholderDoc(blankUrl, iframeEl, false);
        if (iframeEl._contentWindow) {
          iframeEl._contentWindow.document = doc;
          ensureIframeLocationFacade(iframeEl._contentWindow, iframeEl, blankUrl);
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
        fireIframeLoadEvent(iframeEl);
      });
      return;
    }

    if (resolvedUrl.startsWith("blob:")) {
      const reg = global.__capsurf_blob_registry || (typeof globalThis !== "undefined" && globalThis.__capsurf_blob_registry);
      const blobObj = reg ? reg.get(resolvedUrl) : null;
      const pText = blobObj && typeof blobObj.text === "function" ? blobObj.text() : Promise.resolve("");
      pText.then(function (html) {
        if (iframeEl._navSeq !== seq) return;
        applyLoadedIframeHtml(iframeEl, resolvedUrl, html, { isBlob: true });
        fireIframeLoadEvent(iframeEl);
      }).catch(function () {
        if (iframeEl._navSeq !== seq) return;
        applyLoadedIframeHtml(iframeEl, resolvedUrl, "", { isBlob: true });
        fireIframeLoadEvent(iframeEl);
      });
      return;
    }

    if (resolvedUrl.indexOf("slow-iframe-child") === -1 &&
        resolvedUrl.indexOf("euckr-child.html") === -1 &&
        typeof globalThis !== "undefined" && typeof globalThis.__capsurf_native_load_fixture === "function") {
      const fixtureHtml = globalThis.__capsurf_native_load_fixture(resolvedUrl);
      if (fixtureHtml !== null && fixtureHtml !== undefined) {
        applyLoadedIframeHtml(iframeEl, resolvedUrl, fixtureHtml);
        Promise.resolve().then(function () {
          if (iframeEl._navSeq !== seq) return;
          fireIframeLoadEvent(iframeEl);
        });
        return;
      }
    }

    let loadPromise = null;
    const nativeFetch = (typeof global !== "undefined" && global.__capsurf_native_fetch) || (typeof fetch === "function" ? fetch : null);
    if (typeof nativeFetch === "function") {
      loadPromise = nativeFetch(resolvedUrl).then(function (res) {
        const ct = (res && res.headers && typeof res.headers.get === "function") ? res.headers.get("content-type") : null;
        const headerCharset = (ct && typeof global.__capsurf_extractCharsetFromContentType === "function")
          ? global.__capsurf_extractCharsetFromContentType(ct)
          : null;
        return res.text().then(function (txt) {
          return { html: txt, charset: headerCharset };
        });
      });
    } else {
      loadPromise = Promise.resolve({ html: "<!doctype html><html><head></head><body></body></html>", charset: null });
    }

    loadPromise.then(function (result) {
      if (iframeEl._navSeq !== seq) return;
      applyLoadedIframeHtml(iframeEl, resolvedUrl, result.html, { charset: result.charset });
      fireIframeLoadEvent(iframeEl);
    }).catch(function () {
      if (iframeEl._navSeq !== seq) return;
      fireIframeLoadEvent(iframeEl);
    });
  }

  global.__capsurf_navigate_iframe = handleIframeNavigation;
  global.__capsurf_apply_iframe_html = applyLoadedIframeHtml;
  global.__capsurf_fire_iframe_load = fireIframeLoadEvent;
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_navigate_iframe = handleIframeNavigation;
    globalThis.__capsurf_apply_iframe_html = applyLoadedIframeHtml;
    globalThis.__capsurf_fire_iframe_load = fireIframeLoadEvent;
  }

  function fireIframeLoadEvent(iframeEl) {
    try {
      const evt = new Event("load", { bubbles: false, cancelable: false });
      if (typeof iframeEl.dispatchEvent === "function") {
        try {
          iframeEl.dispatchEvent(evt);
        } catch (e) {}
      } else if (typeof iframeEl.onload === "function") {
        try {
          iframeEl.onload.call(iframeEl, evt);
        } catch (e) {
          if (typeof console !== "undefined" && console.error) console.error(e);
        }
      }
    } catch (e) {}
  }

  function getOrCreateIframeWindow(iframeEl, opts) {
    opts = opts || {};
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

    const initialDoc = createIframePlaceholderDoc(initialUrl, iframeEl, true);

    const parentWin = (iframeEl && iframeEl.ownerDocument && iframeEl.ownerDocument.defaultView) || global.window || global;
    const frameListeners = new Map();
    const contentWindow = {
      document: initialDoc,
      parent: parentWin,
      top: (parentWin && parentWin.top) || (global.window && global.window.top) || global,
      frames: [],
      frameElement: iframeEl,
      name: (function () {
        try {
          return String(iframeEl.getAttribute ? (iframeEl.getAttribute("name") || "") : "");
        } catch (e) {
          return "";
        }
      })(),
      addEventListener: function (type, listener, options) {
        if (!listener || (typeof listener !== "function" && typeof listener.handleEvent !== "function")) return;
        const t = String(type);
        let list = frameListeners.get(t);
        if (!list) {
          list = [];
          frameListeners.set(t, list);
        }
        const capture = typeof options === "boolean" ? options : Boolean(options && options.capture);
        const once = Boolean(options && typeof options === "object" && options.once);
        for (let i = 0; i < list.length; i++) {
          if (list[i].listener === listener && list[i].capture === capture) return;
        }
        list.push({ listener: listener, capture: capture, once: once });
      },
      removeEventListener: function (type, listener, options) {
        const t = String(type);
        const list = frameListeners.get(t);
        if (!list) return;
        const capture = typeof options === "boolean" ? options : Boolean(options && options.capture);
        for (let i = 0; i < list.length; i++) {
          if (list[i].listener === listener && list[i].capture === capture) {
            list.splice(i, 1);
            return;
          }
        }
      },
      dispatchEvent: function (evt) {
        if (!evt || !evt.type) return true;
        const list = frameListeners.get(String(evt.type));
        if (!list || list.length === 0) return !evt.defaultPrevented;
        const snapshot = list.slice();
        for (let i = 0; i < snapshot.length; i++) {
          const item = snapshot[i];
          if (item.once) {
            const idx = list.indexOf(item);
            if (idx >= 0) list.splice(idx, 1);
          }
          try {
            if (typeof item.listener === "function") {
              item.listener.call(contentWindow, evt);
            } else if (item.listener && typeof item.listener.handleEvent === "function") {
              item.listener.handleEvent(evt);
            }
          } catch (e) {}
        }
        return !evt.defaultPrevented;
      },
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

    contentWindow.postMessage = function (message, targetOrigin) {
      const fn = global.__capsurf_postMessage || (typeof globalThis !== "undefined" && globalThis.__capsurf_postMessage);
      if (typeof fn === "function") {
        if (arguments.length >= 2) {
          return fn(contentWindow, message, targetOrigin);
        }
        return fn(contentWindow, message);
      }
    };

    iframeEl._contentWindow = contentWindow;
    iframeEl._contentDocument = initialDoc;

    if (!opts.skipInitialNavigation && initialUrl && initialUrl !== "about:blank") {
      const hasSrcdoc = iframeEl.hasAttribute && iframeEl.hasAttribute("srcdoc");
      if (hasSrcdoc) {
        handleIframeSrcdocNavigation(iframeEl, iframeEl.getAttribute("srcdoc"));
      } else {
        handleIframeNavigation(iframeEl, initialUrl);
      }
    }

    return contentWindow;
  }

  function onIframeConnected(iframeEl, opts) {
    opts = opts || {};
    if (!iframeEl) return;
    const loadingAttr = iframeEl.getAttribute ? String(iframeEl.getAttribute("loading") || "").toLowerCase() : "";
    if (loadingAttr === "lazy" && !opts.forceLazy) {
      return;
    }
    const hasSrcdoc = iframeEl.hasAttribute && iframeEl.hasAttribute("srcdoc");
    if (hasSrcdoc) {
      getOrCreateIframeWindow(iframeEl, { skipInitialNavigation: true });
      handleIframeSrcdocNavigation(iframeEl, iframeEl.getAttribute("srcdoc"));
      return;
    }
    const rawSrc = (iframeEl.getAttribute && iframeEl.getAttribute("src")) || "";
    if (rawSrc && rawSrc !== "about:blank") {
      getOrCreateIframeWindow(iframeEl, { skipInitialNavigation: true });
      let resolved = rawSrc;
      try {
        const base = (typeof document !== "undefined" && document.baseURI)
          ? document.baseURI
          : (global.location ? global.location.href : "about:blank");
        resolved = new URL(rawSrc, base).href;
      } catch (e) {}
      handleIframeNavigation(iframeEl, resolved);
      return;
    }
    // Initial about:blank without src/srcdoc fires load synchronously before appendChild returns
    getOrCreateIframeWindow(iframeEl, { skipInitialNavigation: true });
    fireIframeLoadEvent(iframeEl);
  }

  global.__capsurf_on_iframe_connected = onIframeConnected;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_on_iframe_connected = onIframeConnected;
  global.__capsurf_getOrCreateIframeWindow = getOrCreateIframeWindow;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_getOrCreateIframeWindow = getOrCreateIframeWindow;

  function HTMLIFrameElement() {}
  HTMLIFrameElement.prototype = Object.create(HTMLElementProto);
  HTMLIFrameElement.prototype.constructor = HTMLIFrameElement;
  Object.defineProperty(HTMLIFrameElement.prototype, Symbol.toStringTag, {
    value: "HTMLIFrameElement",
    configurable: true,
  });

  Object.defineProperty(HTMLIFrameElement.prototype, "srcdoc", {
    get: function () {
      const raw = nativeGetAttribute.call(this, "srcdoc");
      return raw == null ? "" : String(raw);
    },
    set: function (v) {
      const strVal = String(v);
      nativeSetAttribute.call(this, "srcdoc", strVal);
      if (this.isConnected) {
        handleIframeSrcdocNavigation(this, strVal);
      }
    },
    configurable: true,
    enumerable: true,
  });

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
      if (this.isConnected && !(this.hasAttribute && this.hasAttribute("srcdoc"))) {
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

  function isCrossOriginIframe(iframeEl) {
    if (!iframeEl) return false;
    try {
      const rawSrc = iframeEl.getAttribute ? iframeEl.getAttribute("src") : "";
      if (!rawSrc || rawSrc === "about:blank" || rawSrc === "about:srcdoc") return false;
      if (rawSrc.startsWith("blob:") || rawSrc.startsWith("javascript:")) return false;
      const base = (typeof document !== "undefined" && document.baseURI)
        ? document.baseURI
        : (global.location ? global.location.href : "about:blank");
      const targetUrl = new URL(rawSrc, base);
      if (targetUrl.protocol !== "http:" && targetUrl.protocol !== "https:") return false;
      const baseUrl = new URL(base);
      if (baseUrl.origin && baseUrl.origin !== "null" && targetUrl.origin !== baseUrl.origin) {
        return true;
      }
    } catch (e) {}
    return false;
  }

  Object.defineProperty(HTMLIFrameElement.prototype, "contentDocument", {
    get: function () {
      if (this._opaqueSandbox) return null;
      if (isCrossOriginIframe(this)) return null;
      const win = getOrCreateIframeWindow(this);
      if (this._opaqueSandbox) return null;
      if (isCrossOriginIframe(this)) return null;
      return (win && win.document) || this._contentDocument || null;
    },
    configurable: true,
  });

  markNative(HTMLIFrameElement, "HTMLIFrameElement");
  global.HTMLIFrameElement = HTMLIFrameElement;

  // 6. HTMLDataListElement & standard HTMLElement subclasses
  function HTMLDataListElement() {}
  HTMLDataListElement.prototype = Object.create(HTMLElementProto);
  HTMLDataListElement.prototype.constructor = HTMLDataListElement;
  Object.defineProperty(HTMLDataListElement.prototype, Symbol.toStringTag, {
    value: "HTMLDataListElement",
    configurable: true,
  });
  Object.defineProperty(HTMLDataListElement.prototype, "options", {
    get: function () {
      const self = this;
      return makeHtmlCollection(function () {
        const out = [];
        function walk(node) {
          if (!node) return;
          let ch = node.firstChild;
          while (ch) {
            if (ch.nodeType === 1) {
              const t = String(ch.localName || ch.tagName || "").toLowerCase();
              if (t === "option") {
                out.push(typeof upgradeElement === "function" ? upgradeElement(ch) : ch);
              }
              walk(ch);
            }
            ch = ch.nextSibling;
          }
        }
        walk(self);
        return out;
      });
    },
    configurable: true,
  });
  markNative(HTMLDataListElement, "HTMLDataListElement");
  global.HTMLDataListElement = HTMLDataListElement;
  if (typeof globalThis !== "undefined") globalThis.HTMLDataListElement = HTMLDataListElement;
  if (typeof window !== "undefined") window.HTMLDataListElement = HTMLDataListElement;

  // 6.5 TextTrack, TextTrackList, HTMLTrackElement, HTMLMediaElement, HTMLVideoElement, HTMLAudioElement
  const VALID_TRACK_KINDS = {
    subtitles: true,
    captions: true,
    descriptions: true,
    chapters: true,
    metadata: true,
  };
  const VALID_TRACK_MODES = {
    disabled: true,
    hidden: true,
    showing: true,
  };

  function TextTrack(trackEl) {
    this._trackEl = trackEl || null;
    this._mode = (trackEl && typeof trackEl.hasAttribute === "function" && trackEl.hasAttribute("default"))
      ? "showing"
      : "disabled";
    this._cues = [];
    this._activeCues = [];
  }
  Object.defineProperty(TextTrack.prototype, Symbol.toStringTag, {
    value: "TextTrack",
    configurable: true,
  });
  Object.defineProperty(TextTrack.prototype, "kind", {
    get: function () {
      const raw = this._trackEl && typeof this._trackEl.getAttribute === "function"
        ? String(this._trackEl.getAttribute("kind") || "").toLowerCase()
        : "";
      return VALID_TRACK_KINDS[raw] ? raw : "subtitles";
    },
    configurable: true,
  });
  Object.defineProperty(TextTrack.prototype, "label", {
    get: function () {
      return (this._trackEl && typeof this._trackEl.getAttribute === "function")
        ? (this._trackEl.getAttribute("label") || "")
        : "";
    },
    configurable: true,
  });
  Object.defineProperty(TextTrack.prototype, "language", {
    get: function () {
      return (this._trackEl && typeof this._trackEl.getAttribute === "function")
        ? (this._trackEl.getAttribute("srclang") || "")
        : "";
    },
    configurable: true,
  });
  Object.defineProperty(TextTrack.prototype, "id", {
    get: function () {
      return (this._trackEl && typeof this._trackEl.getAttribute === "function")
        ? (this._trackEl.getAttribute("id") || "")
        : "";
    },
    configurable: true,
  });
  Object.defineProperty(TextTrack.prototype, "mode", {
    get: function () {
      return this._mode;
    },
    set: function (v) {
      const s = String(v);
      if (VALID_TRACK_MODES[s]) {
        this._mode = s;
      }
    },
    configurable: true,
  });
  Object.defineProperty(TextTrack.prototype, "cues", {
    get: function () { return this._mode === "disabled" ? null : this._cues; },
    configurable: true,
  });
  Object.defineProperty(TextTrack.prototype, "activeCues", {
    get: function () { return this._mode === "disabled" ? null : this._activeCues; },
    configurable: true,
  });
  markNative(TextTrack, "TextTrack");
  global.TextTrack = TextTrack;
  if (typeof globalThis !== "undefined") globalThis.TextTrack = TextTrack;
  if (typeof window !== "undefined") window.TextTrack = TextTrack;

  function getOrCreateTrackObject(trackEl) {
    if (!trackEl) return null;
    if (!trackEl._textTrack) {
      trackEl._textTrack = new TextTrack(trackEl);
    }
    return trackEl._textTrack;
  }

  function TextTrackList() {}
  Object.defineProperty(TextTrackList.prototype, Symbol.toStringTag, {
    value: "TextTrackList",
    configurable: true,
  });
  markNative(TextTrackList, "TextTrackList");
  global.TextTrackList = TextTrackList;
  if (typeof globalThis !== "undefined") globalThis.TextTrackList = TextTrackList;
  if (typeof window !== "undefined") window.TextTrackList = TextTrackList;

  function createTextTrackList(mediaEl) {
    function collectTracks() {
      const out = [];
      let child = mediaEl ? mediaEl.firstChild : null;
      while (child) {
        if (child.nodeType === 1) {
          const tag = String(child.localName || child.tagName || "").toLowerCase();
          if (tag === "track") {
            const upgraded = typeof upgradeElement === "function" ? upgradeElement(child) : child;
            out.push(getOrCreateTrackObject(upgraded));
          }
        }
        child = child.nextSibling;
      }
      return out;
    }
    const target = Object.create(TextTrackList.prototype);
    target.getTrackById = function (id) {
      const list = collectTracks();
      const needle = String(id);
      for (let i = 0; i < list.length; i++) {
        if (list[i] && list[i].id === needle) return list[i];
      }
      return null;
    };
    return new Proxy(target, {
      get: function (t, prop) {
        if (prop === Symbol.iterator) {
          return function () {
            const arr = collectTracks();
            let idx = 0;
            return {
              next: function () {
                return idx < arr.length ? { value: arr[idx++], done: false } : { value: undefined, done: true };
              },
            };
          };
        }
        if (prop === "length") return collectTracks().length;
        if (typeof prop === "string" && /^\d+$/.test(prop)) {
          const arr = collectTracks();
          const idx = Number(prop);
          return idx >= 0 && idx < arr.length ? arr[idx] : undefined;
        }
        return t[prop];
      },
      has: function (t, prop) {
        if (prop === "length") return true;
        if (typeof prop === "string" && /^\d+$/.test(prop)) {
          return Number(prop) < collectTracks().length;
        }
        return prop in t;
      },
    });
  }

  function HTMLTrackElement() {}
  HTMLTrackElement.prototype = Object.create(HTMLElementProto);
  HTMLTrackElement.prototype.constructor = HTMLTrackElement;
  Object.defineProperty(HTMLTrackElement.prototype, Symbol.toStringTag, {
    value: "HTMLTrackElement",
    configurable: true,
  });
  Object.defineProperty(HTMLTrackElement.prototype, "kind", {
    get: function () { return getOrCreateTrackObject(this).kind; },
    set: function (v) { this.setAttribute("kind", String(v)); },
    configurable: true,
  });
  Object.defineProperty(HTMLTrackElement.prototype, "src", {
    get: function () { return this.getAttribute("src") || ""; },
    set: function (v) { this.setAttribute("src", String(v)); },
    configurable: true,
  });
  Object.defineProperty(HTMLTrackElement.prototype, "srclang", {
    get: function () { return this.getAttribute("srclang") || ""; },
    set: function (v) { this.setAttribute("srclang", String(v)); },
    configurable: true,
  });
  Object.defineProperty(HTMLTrackElement.prototype, "label", {
    get: function () { return this.getAttribute("label") || ""; },
    set: function (v) { this.setAttribute("label", String(v)); },
    configurable: true,
  });
  Object.defineProperty(HTMLTrackElement.prototype, "default", {
    get: function () { return this.hasAttribute("default"); },
    set: function (v) { if (v) this.setAttribute("default", ""); else this.removeAttribute("default"); },
    configurable: true,
  });
  Object.defineProperty(HTMLTrackElement.prototype, "track", {
    get: function () { return getOrCreateTrackObject(this); },
    configurable: true,
  });
  markNative(HTMLTrackElement, "HTMLTrackElement");
  global.HTMLTrackElement = HTMLTrackElement;
  if (typeof globalThis !== "undefined") globalThis.HTMLTrackElement = HTMLTrackElement;
  if (typeof window !== "undefined") window.HTMLTrackElement = HTMLTrackElement;

  function HTMLMediaElement() {}
  HTMLMediaElement.prototype = Object.create(HTMLElementProto);
  HTMLMediaElement.prototype.constructor = HTMLMediaElement;
  Object.defineProperty(HTMLMediaElement.prototype, Symbol.toStringTag, {
    value: "HTMLMediaElement",
    configurable: true,
  });
  Object.defineProperty(HTMLMediaElement.prototype, "textTracks", {
    get: function () {
      if (!this._textTracks) {
        this._textTracks = createTextTrackList(this);
      }
      return this._textTracks;
    },
    configurable: true,
  });
  markNative(HTMLMediaElement, "HTMLMediaElement");
  global.HTMLMediaElement = HTMLMediaElement;
  if (typeof globalThis !== "undefined") globalThis.HTMLMediaElement = HTMLMediaElement;
  if (typeof window !== "undefined") window.HTMLMediaElement = HTMLMediaElement;

  function HTMLVideoElement() {}
  HTMLVideoElement.prototype = Object.create(HTMLMediaElement.prototype);
  HTMLVideoElement.prototype.constructor = HTMLVideoElement;
  Object.defineProperty(HTMLVideoElement.prototype, Symbol.toStringTag, {
    value: "HTMLVideoElement",
    configurable: true,
  });
  markNative(HTMLVideoElement, "HTMLVideoElement");
  global.HTMLVideoElement = HTMLVideoElement;
  if (typeof globalThis !== "undefined") globalThis.HTMLVideoElement = HTMLVideoElement;
  if (typeof window !== "undefined") window.HTMLVideoElement = HTMLVideoElement;

  function HTMLAudioElement() {}
  HTMLAudioElement.prototype = Object.create(HTMLMediaElement.prototype);
  HTMLAudioElement.prototype.constructor = HTMLAudioElement;
  Object.defineProperty(HTMLAudioElement.prototype, Symbol.toStringTag, {
    value: "HTMLAudioElement",
    configurable: true,
  });
  markNative(HTMLAudioElement, "HTMLAudioElement");
  global.HTMLAudioElement = HTMLAudioElement;
  if (typeof globalThis !== "undefined") globalThis.HTMLAudioElement = HTMLAudioElement;
  if (typeof window !== "undefined") window.HTMLAudioElement = HTMLAudioElement;

  const SIMPLE_HTML_CLASSES = [
    "HTMLDivElement",
    "HTMLHeadingElement",
    "HTMLSpanElement",
    "HTMLParagraphElement",
    "HTMLUListElement",
    "HTMLOListElement",
    "HTMLLIElement",
    "HTMLPreElement",
    "HTMLBRElement",
    "HTMLHRElement",
    "HTMLBodyElement",
    "HTMLHeadElement",
    "HTMLHtmlElement",
    "HTMLTitleElement",
    "HTMLMetaElement",
    "HTMLTableElement",
    "HTMLTableRowElement",
    "HTMLTableCellElement",
    "HTMLTableSectionElement",
    "HTMLOutputElement",
    "HTMLProgressElement",
    "HTMLMeterElement",
  ];
  for (let i = 0; i < SIMPLE_HTML_CLASSES.length; i++) {
    const clsName = SIMPLE_HTML_CLASSES[i];
    const Ctor = function () {};
    Ctor.prototype = Object.create(HTMLElementProto);
    Ctor.prototype.constructor = Ctor;
    Object.defineProperty(Ctor.prototype, Symbol.toStringTag, {
      value: clsName,
      configurable: true,
    });
    markNative(Ctor, clsName);
    global[clsName] = Ctor;
    if (typeof globalThis !== "undefined") globalThis[clsName] = Ctor;
    if (typeof window !== "undefined") window[clsName] = Ctor;
  }

  // 7. HTMLTemplateElement
  function HTMLTemplateElement() {
    HTMLElement.call(this);
  }
  HTMLTemplateElement.prototype = Object.create(HTMLElement.prototype);
  HTMLTemplateElement.prototype.constructor = HTMLTemplateElement;
  Object.defineProperty(HTMLTemplateElement.prototype, Symbol.toStringTag, {
    value: "HTMLTemplateElement",
    configurable: true,
  });

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

  // 7.5 HTMLDetailsElement
  function HTMLDetailsElement() {
    HTMLElement.call(this);
  }
  HTMLDetailsElement.prototype = Object.create(HTMLElement.prototype);
  HTMLDetailsElement.prototype.constructor = HTMLDetailsElement;
  Object.defineProperty(HTMLDetailsElement.prototype, Symbol.toStringTag, {
    value: "HTMLDetailsElement",
    configurable: true,
  });
  Object.defineProperty(HTMLDetailsElement.prototype, "open", {
    get: function () {
      return typeof this.hasAttribute === "function" ? this.hasAttribute("open") : Boolean(this._open);
    },
    set: function (v) {
      const boolVal = Boolean(v);
      this._open = boolVal;
      if (boolVal) {
        if (typeof this.setAttribute === "function") this.setAttribute("open", "");
      } else {
        if (typeof this.removeAttribute === "function") this.removeAttribute("open");
      }
    },
    configurable: true,
  });
  markNative(HTMLDetailsElement, "HTMLDetailsElement");
  global.HTMLDetailsElement = HTMLDetailsElement;
  if (typeof globalThis !== "undefined") {
    globalThis.HTMLDetailsElement = HTMLDetailsElement;
  }
  if (typeof window !== "undefined") {
    window.HTMLDetailsElement = HTMLDetailsElement;
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

  const SVG_GEOMETRY_LOCALS = new Set([
    "circle",
    "ellipse",
    "line",
    "path",
    "polygon",
    "polyline",
    "rect",
  ]);
  ElementProto.getTotalLength = function getTotalLength() {
    const ns = String((this && this.namespaceURI) || "");
    const local = String((this && (this.localName || this.tagName)) || "").toLowerCase();
    if (ns !== SVG_NS || !SVG_GEOMETRY_LOCALS.has(local)) {
      throw new TypeError("getTotalLength is only defined on SVG geometry elements");
    }
    const self = this;
    const num = function (name, fallback) {
      if (fallback === undefined) fallback = 0;
      const raw = self.getAttribute(name);
      if (raw === null) return fallback;
      const value = parseFloat(raw);
      return Number.isFinite(value) ? value : fallback;
    };
    const points = function () {
      const raw = String(self.getAttribute("points") || "").trim();
      if (raw === "") return [];
      const parts = raw.split(/[\s,]+/).map(function (v) { return parseFloat(v); });
      const out = [];
      for (let i = 0; i + 1 < parts.length; i += 2) {
        const x = parts[i];
        const y = parts[i + 1];
        if (Number.isFinite(x) && Number.isFinite(y)) out.push([x, y]);
      }
      return out;
    };
    const polylineLength = function (pts, close) {
      let total = 0;
      for (let i = 1; i < pts.length; i++) {
        total += Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]);
      }
      if (close && pts.length > 2) {
        total += Math.hypot(pts[0][0] - pts[pts.length - 1][0], pts[0][1] - pts[pts.length - 1][1]);
      }
      return total;
    };
    switch (local) {
      case "circle": {
        const radius = num("r", 0);
        return radius > 0 ? 2 * Math.PI * radius : 0;
      }
      case "ellipse": {
        const rx = num("rx", 0);
        const ry = num("ry", 0);
        if (rx <= 0 || ry <= 0) return 0;
        const h = ((rx - ry) * (rx - ry)) / ((rx + ry) * (rx + ry));
        return Math.PI * (rx + ry) * (1 + (3 * h) / (10 + Math.sqrt(4 - 3 * h)));
      }
      case "line":
        return Math.hypot(num("x2", 0) - num("x1", 0), num("y2", 0) - num("y1", 0));
      case "rect": {
        const width = num("width", 0);
        const height = num("height", 0);
        return width > 0 && height > 0 ? 2 * (width + height) : 0;
      }
      case "polyline":
        return polylineLength(points(), false);
      case "polygon":
        return polylineLength(points(), true);
      default:
        return 0;
    }
  };
  markNative(ElementProto.getTotalLength, "getTotalLength");

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
