// src/js/shim/fonts.js
// CSS Font Loading Module Level 3 implementation (FontFace, FontFaceSet, document.fonts)
// Aligned with W3C CSS Font Loading Module Level 3 & vendor/kitesurf/packages/dom-shim/src/document.ts

(function () {
  "use strict";

  const ksSym = Symbol.for("kitesurf.internals");

  function getInternals() {
    return (typeof globalThis !== "undefined" && globalThis[ksSym]) ||
           (typeof global !== "undefined" && global[ksSym]) ||
           (typeof window !== "undefined" && window[ksSym]) || null;
  }

  // 1. FontFace class
  function FontFace(family, source, descriptors) {
    descriptors = descriptors || {};
    this.family = String(family || "").replace(/^["']|["']$/g, "").trim();
    this.source = source;
    this.style = String(descriptors.style || "normal");
    this.weight = String(descriptors.weight || "normal");
    this.stretch = String(descriptors.stretch || "normal");
    this.unicodeRange = String(descriptors.unicodeRange || "U+0-10ffff");
    this.variant = String(descriptors.variant || "normal");
    this.featureSettings = String(descriptors.featureSettings || "normal");
    this.variationSettings = String(descriptors.variationSettings || "normal");
    this.display = String(descriptors.display || "auto");
    this.ascentOverride = String(descriptors.ascentOverride || "normal");
    this.descentOverride = String(descriptors.descentOverride || "normal");
    this.lineGapOverride = String(descriptors.lineGapOverride || "normal");
    this.status = "loaded";
    this.loaded = Promise.resolve(this);
  }

  FontFace.prototype.load = function () {
    this.status = "loaded";
    return Promise.resolve(this);
  };

  markNative(FontFace, "FontFace");
  markNative(FontFace.prototype.load, "load");

  // 2. FontFaceSet class (extends EventTarget)
  const EventTargetCtor = global.EventTarget || (typeof EventTarget !== "undefined" ? EventTarget : Object);

  function FontFaceSet() {
    if (EventTargetCtor !== Object) {
      EventTargetCtor.call(this);
    }
    this._set = new Set();
    this.onloading = null;
    this.onloadingdone = null;
    this.onloadingerror = null;
  }

  FontFaceSet.prototype = Object.create(EventTargetCtor.prototype || Object.prototype);
  FontFaceSet.prototype.constructor = FontFaceSet;

  Object.defineProperty(FontFaceSet.prototype, "status", {
    get: function () {
      const g = getInternals();
      if (g && typeof g.fontsInflightCount === "function") {
        try {
          const n = g.fontsInflightCount();
          return (n | 0) > 0 ? "loading" : "loaded";
        } catch (e) {
          return "loaded";
        }
      }
      return "loaded";
    },
    configurable: true,
    enumerable: true,
  });

  Object.defineProperty(FontFaceSet.prototype, "ready", {
    get: function () {
      const g = getInternals();
      const hook = g && typeof g.fontsReady === "function" ? g.fontsReady : null;
      let p = null;
      if (hook) {
        try {
          p = hook();
        } catch (e) {
          p = null;
        }
      }
      const self = this;
      return (p || Promise.resolve()).then(function () {
        return self;
      });
    },
    configurable: true,
    enumerable: true,
  });

  FontFaceSet.prototype.add = function (font) {
    this._set.add(font);
    return this;
  };

  FontFaceSet.prototype.delete = function (font) {
    return this._set.delete(font);
  };

  FontFaceSet.prototype.clear = function () {
    this._set.clear();
  };

  FontFaceSet.prototype.has = function (font) {
    return this._set.has(font);
  };

  Object.defineProperty(FontFaceSet.prototype, "size", {
    get: function () {
      return this._set.size;
    },
    configurable: true,
    enumerable: true,
  });

  FontFaceSet.prototype.forEach = function (callback, thisArg) {
    this._set.forEach(callback, thisArg);
  };

  FontFaceSet.prototype.entries = function () {
    return this._set.entries();
  };

  FontFaceSet.prototype.keys = function () {
    return this._set.keys();
  };

  FontFaceSet.prototype.values = function () {
    return this._set.values();
  };

  FontFaceSet.prototype[Symbol.iterator] = function () {
    return this._set[Symbol.iterator]();
  };

  FontFaceSet.prototype.check = function (font, text) {
    return true;
  };

  FontFaceSet.prototype.load = function (font, text) {
    const self = this;
    return this.ready.then(function () {
      const GENERIC = new Set(["serif", "sans-serif", "monospace", "cursive", "fantasy", "system-ui", "ui-serif", "ui-sans-serif", "ui-monospace", "ui-rounded", "emoji", "math", "fangsong"]);
      let str = String(font || "").trim();
      const sizeMatch = /\d+(?:\.\d+)?(?:px|pt|em|rem|ex|ch|vw|vh|vmin|vmax|cm|mm|in|pc|%)(?:\s*\/\s*[\d.]+(?:px|pt|em|rem|%)?)?\s+(.+)$/i.exec(str);
      const famStr = sizeMatch ? sizeMatch[1] : str;
      const rawFams = famStr.split(",");
      const out = [];
      const seen = new Set();
      if (!self._standInFaces) self._standInFaces = new Map();
      for (let i = 0; i < rawFams.length; i++) {
        const fam = rawFams[i].trim().replace(/^["']|["']$/g, "");
        if (!fam) continue;
        const lower = fam.toLowerCase();
        if (GENERIC.has(lower) || seen.has(lower)) continue;
        seen.add(lower);
        let matched = null;
        self._set.forEach(function (f) {
          if (!matched && f && String(f.family || "").toLowerCase() === lower) {
            matched = f;
          }
        });
        if (!matched) {
          matched = self._standInFaces.get(lower);
          if (!matched) {
            matched = new FontFace(fam, "");
            self._standInFaces.set(lower, matched);
          }
        }
        out.push(matched);
      }
      return out;
    });
  };

  markNative(FontFaceSet, "FontFaceSet");
  markNative(FontFaceSet.prototype.add, "add");
  markNative(FontFaceSet.prototype.delete, "delete");
  markNative(FontFaceSet.prototype.clear, "clear");
  markNative(FontFaceSet.prototype.has, "has");
  markNative(FontFaceSet.prototype.forEach, "forEach");
  markNative(FontFaceSet.prototype.check, "check");
  markNative(FontFaceSet.prototype.load, "load");

  // Expose FontFace & FontFaceSet constructors
  global.FontFace = FontFace;
  global.FontFaceSet = FontFaceSet;
  if (typeof globalThis !== "undefined") {
    globalThis.FontFace = FontFace;
    globalThis.FontFaceSet = FontFaceSet;
  }
  if (typeof window !== "undefined") {
    window.FontFace = FontFace;
    window.FontFaceSet = FontFaceSet;
  }

  // Helper to parse @font-face rules from document styles
  function syncFontsFromDocumentStyles(fontFaceSet) {
    try {
      const doc = (typeof document !== "undefined") ? document : (global.document || null);
      if (!doc) return;
      const styleEls = (doc.getElementsByTagName && doc.getElementsByTagName("style")) ||
                       (doc.querySelectorAll && doc.querySelectorAll("style")) || [];
      for (let i = 0; i < styleEls.length; i++) {
        const text = styleEls[i].textContent || "";
        if (text.indexOf("@font-face") !== -1) {
          parseAndAddFontFaces(text, fontFaceSet);
        }
      }
    } catch (e) {}
  }

  function parseAndAddFontFaces(css, fontFaceSet) {
    const regex = /@font-face\s*\{([^}]+)\}/gi;
    let m;
    while ((m = regex.exec(css)) !== null) {
      const block = m[1];
      const famMatch = /font-family\s*:\s*([^;]+)/i.exec(block);
      const srcMatch = /src\s*:\s*([^;]+)/i.exec(block);
      if (famMatch && srcMatch) {
        const fam = famMatch[1].trim().replace(/^["']|["']$/g, "");
        const src = srcMatch[1].trim();
        let exists = false;
        fontFaceSet.forEach(function (f) {
          if (f.family === fam) exists = true;
        });
        if (!exists) {
          fontFaceSet.add(new FontFace(fam, src));
        }
      }
    }
  }

  // 3. Mount document.fonts getter
  if (typeof Document !== "undefined" && Document.prototype) {
    Object.defineProperty(Document.prototype, "fonts", {
      get: function () {
        if (!this.__fontFaceSet) {
          this.__fontFaceSet = new FontFaceSet();
          syncFontsFromDocumentStyles(this.__fontFaceSet);
        }
        return this.__fontFaceSet;
      },
      configurable: true,
      enumerable: true,
    });
  }

  const doc = (typeof document !== "undefined") ? document : (global.document || (typeof window !== "undefined" ? window.document : null));
  if (doc) {
    try {
      Object.defineProperty(doc, "fonts", {
        get: function () {
          if (!this.__fontFaceSet) {
            this.__fontFaceSet = new FontFaceSet();
            syncFontsFromDocumentStyles(this.__fontFaceSet);
          }
          return this.__fontFaceSet;
        },
        configurable: true,
        enumerable: true,
      });
    } catch (e) {
      if (!doc.fonts) {
        doc.fonts = new FontFaceSet();
        syncFontsFromDocumentStyles(doc.fonts);
      }
    }
  }

  // 4. Ensure Symbol.for("kitesurf.internals") defaults are present
  (function ensureInternals() {
    const rec = getInternals() || {};
    if (typeof rec.fontsInflightCount !== "function") {
      rec.fontsInflightCount = function () { return 0; };
    }
    if (typeof rec.fontsReady !== "function") {
      rec.fontsReady = function () { return Promise.resolve(); };
    }
    if (typeof rec.demandFontOutlines !== "function") {
      rec.demandFontOutlines = function () {};
    }
    if (typeof globalThis !== "undefined" && !globalThis[ksSym]) {
      globalThis[ksSym] = rec;
    }
    if (typeof global !== "undefined" && !global[ksSym]) {
      global[ksSym] = rec;
    }
    if (typeof window !== "undefined" && !window[ksSym]) {
      window[ksSym] = rec;
    }
  })();
})();
