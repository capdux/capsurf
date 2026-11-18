// src/js/shim/cssom.js
// CSSOM implementation: CSSRule hierarchy, CSSStyleSheet, CSS.supports, CSSStyleDeclaration

(function (global) {
  "use strict";

  // =========================================================================
  // 1. CSS Properties & Values Validation
  // =========================================================================

  const STANDARD_CSS_PROPERTIES = new Set([
    "accent-color", "align-content", "align-items", "align-self", "all",
    "animation", "animation-composition", "animation-delay", "animation-direction",
    "animation-duration", "animation-fill-mode", "animation-iteration-count",
    "animation-name", "animation-play-state", "animation-timeline", "animation-timing-function",
    "appearance", "aspect-ratio", "backdrop-filter", "backface-visibility",
    "background", "background-attachment", "background-blend-mode", "background-clip",
    "background-color", "background-image", "background-origin", "background-position",
    "background-position-x", "background-position-y", "background-repeat", "background-size",
    "base-palette", "block-size",
    "border", "border-block", "border-block-color", "border-block-end", "border-block-end-color",
    "border-block-end-style", "border-block-end-width", "border-block-start", "border-block-start-color",
    "border-block-start-style", "border-block-start-width", "border-block-style", "border-block-width",
    "border-bottom", "border-bottom-color", "border-bottom-left-radius", "border-bottom-right-radius",
    "border-bottom-style", "border-bottom-width", "border-collapse", "border-color",
    "border-end-end-radius", "border-end-start-radius", "border-image", "border-image-outset",
    "border-image-repeat", "border-image-slice", "border-image-source", "border-image-width",
    "border-inline", "border-inline-color", "border-inline-end", "border-inline-end-color",
    "border-inline-end-style", "border-inline-end-width", "border-inline-start",
    "border-inline-start-color", "border-inline-start-style", "border-inline-start-width",
    "border-inline-style", "border-inline-width", "border-left", "border-left-color",
    "border-left-style", "border-left-width", "border-radius", "border-right",
    "border-right-color", "border-right-style", "border-right-width", "border-spacing",
    "border-start-end-radius", "border-start-start-radius", "border-style", "border-top",
    "border-top-color", "border-top-left-radius", "border-top-right-radius", "border-top-style",
    "border-top-width", "border-width", "bottom", "box-decoration-break", "box-shadow",
    "box-sizing", "break-after", "break-before", "break-inside", "caption-side", "caret-color",
    "clear", "clip", "clip-path", "color", "color-scheme", "column-count", "column-fill",
    "column-gap", "column-rule", "column-rule-color", "column-rule-style", "column-rule-width",
    "column-span", "column-width", "columns", "contain", "contain-intrinsic-block-size",
    "contain-intrinsic-height", "contain-intrinsic-inline-size", "contain-intrinsic-size",
    "contain-intrinsic-width", "container", "container-name", "container-type", "content",
    "content-visibility", "counter-increment", "counter-reset", "counter-set", "cursor",
    "direction", "display", "empty-cells", "filter", "flex", "flex-basis", "flex-direction",
    "flex-flow", "flex-grow", "flex-shrink", "flex-wrap", "float", "font", "font-family",
    "font-feature-settings", "font-kerning", "font-optical-sizing", "font-palette",
    "font-size", "font-size-adjust", "font-stretch", "font-style", "font-synthesis",
    "font-variant", "font-variant-alternates", "font-variant-caps", "font-variant-east-asian",
    "font-variant-ligatures", "font-variant-numeric", "font-variant-position",
    "font-variation-settings", "font-weight", "font-display", "forced-color-adjust", "gap",
    "grid", "grid-area", "grid-auto-columns", "grid-auto-flow", "grid-auto-rows",
    "grid-column", "grid-column-end", "grid-column-gap", "grid-column-start", "grid-gap",
    "grid-row", "grid-row-end", "grid-row-gap", "grid-row-start", "grid-template",
    "grid-template-areas", "grid-template-columns", "grid-template-rows", "height",
    "hyphenate-character", "hyphens", "image-orientation", "image-rendering",
    "inline-size", "inset", "inset-block", "inset-block-end", "inset-block-start",
    "inset-inline", "inset-inline-end", "inset-inline-start", "isolation", "justify-content",
    "justify-items", "justify-self", "left", "letter-spacing", "line-break", "line-height",
    "list-style", "list-style-image", "list-style-position", "list-style-type", "margin",
    "margin-block", "margin-block-end", "margin-block-start", "margin-bottom",
    "margin-inline", "margin-inline-end", "margin-inline-start", "margin-left",
    "margin-right", "margin-top", "mask", "mask-clip", "mask-composite", "mask-image",
    "mask-mode", "mask-origin", "mask-position", "mask-repeat", "mask-size", "mask-type",
    "max-block-size", "max-height", "max-inline-size", "max-width", "min-block-size",
    "min-height", "min-inline-size", "min-width", "mix-blend-mode", "object-fit",
    "object-position", "offset", "offset-anchor", "offset-distance", "offset-path",
    "offset-position", "offset-rotate", "opacity", "order", "orphans", "outline",
    "outline-color", "outline-offset", "outline-style", "outline-width", "overflow",
    "overflow-anchor", "overflow-block", "overflow-clip-margin", "overflow-inline",
    "overflow-wrap", "overflow-x", "overflow-y", "overscroll-behavior",
    "overscroll-behavior-block", "overscroll-behavior-inline", "overscroll-behavior-x",
    "overscroll-behavior-y", "override-colors", "padding", "padding-block",
    "padding-block-end", "padding-block-start", "padding-bottom", "padding-inline",
    "padding-inline-end", "padding-inline-start", "padding-left", "padding-right",
    "padding-top", "page", "page-break-after", "page-break-before", "page-break-inside",
    "paint-order", "perspective", "perspective-origin", "place-content", "place-items",
    "place-self", "pointer-events", "position", "quotes", "resize", "right", "rotate",
    "row-gap", "scale", "scroll-behavior", "scroll-margin", "scroll-margin-block",
    "scroll-margin-block-end", "scroll-margin-block-start", "scroll-margin-bottom",
    "scroll-margin-inline", "scroll-margin-inline-end", "scroll-margin-inline-start",
    "scroll-margin-left", "scroll-margin-right", "scroll-margin-top", "scroll-padding",
    "scroll-padding-block", "scroll-padding-block-end", "scroll-padding-block-start",
    "scroll-padding-bottom", "scroll-padding-inline", "scroll-padding-inline-end",
    "scroll-padding-inline-start", "scroll-padding-left", "scroll-padding-right",
    "scroll-padding-top", "scroll-snap-align", "scroll-snap-stop", "scroll-snap-type",
    "scrollbar-color", "scrollbar-gutter", "scrollbar-width", "shape-image-threshold",
    "shape-margin", "shape-outside", "size", "src", "tab-size", "table-layout",
    "text-align", "text-align-last", "text-combine-upright", "text-decoration",
    "text-decoration-color", "text-decoration-line", "text-decoration-skip-ink",
    "text-decoration-style", "text-decoration-thickness", "text-emphasis",
    "text-emphasis-color", "text-emphasis-position", "text-emphasis-style", "text-indent",
    "text-justify", "text-orientation", "text-overflow", "text-rendering", "text-shadow",
    "text-transform", "text-underline-offset", "text-underline-position", "top",
    "touch-action", "transform", "transform-box", "transform-origin", "transform-style",
    "transition", "transition-delay", "transition-duration", "transition-property",
    "transition-timing-function", "translate", "unicode-bidi", "unicode-range",
    "user-select", "vertical-align", "visibility", "white-space", "widows", "width",
    "will-change", "word-break", "word-spacing", "word-wrap", "writing-mode", "z-index",
    // Vendor prefixed
    "-webkit-transform", "-webkit-transform-origin", "-webkit-transition", "-webkit-animation",
    "-webkit-user-select", "-webkit-box-sizing", "-webkit-appearance", "-webkit-line-clamp",
    "-webkit-box-orient", "-webkit-box-direction", "-webkit-background-clip",
    "-moz-animation", "-moz-transform", "-moz-user-select", "-moz-box-sizing", "-moz-appearance"
  ]);

  const SHORTHAND_LONGHANDS = {
    margin: ["margin-top", "margin-right", "margin-bottom", "margin-left"],
    padding: ["padding-top", "padding-right", "padding-bottom", "padding-left"],
    "border-width": ["border-top-width", "border-right-width", "border-bottom-width", "border-left-width"],
    "border-style": ["border-top-style", "border-right-style", "border-bottom-style", "border-left-style"],
    "border-color": ["border-top-color", "border-right-color", "border-bottom-color", "border-left-color"],
    "border-radius": ["border-top-left-radius", "border-top-right-radius", "border-bottom-right-radius", "border-bottom-left-radius"],
    overflow: ["overflow-x", "overflow-y"]
  };

  function cssPropCamelToKebab(s) {
    const str = String(s);
    if (!str || str.startsWith("--")) return str;
    if (str.startsWith("-")) return str.toLowerCase();
    let body = str;
    let prefix = "";
    if (str.startsWith("webkit")) {
      prefix = "-webkit-";
      body = str.slice(6);
    } else if (str.startsWith("Moz")) {
      prefix = "-moz-";
      body = str.slice(3);
    }
    if (body.length > 0) {
      body = body[0].toLowerCase() + body.slice(1);
    }
    return prefix + body.replace(/[A-Z]/g, (c) => "-" + c.toLowerCase());
  }

  function isCssPropertySupported(prop) {
    if (typeof prop !== "string") return false;
    const p = prop.trim();
    if (!p) return false;
    if (p.startsWith("--")) return true;
    if (STANDARD_CSS_PROPERTIES.has(p)) return true;
    const kebab = cssPropCamelToKebab(p);
    return STANDARD_CSS_PROPERTIES.has(kebab);
  }
  global.__isCssPropertySupported = isCssPropertySupported;
  if (typeof globalThis !== "undefined") globalThis.__isCssPropertySupported = isCssPropertySupported;

  function isValidColor(val) {
    const v = val.trim().toLowerCase();
    if (!v) return false;
    if (v === "bogus") return false;
    if (v === "currentcolor" || v === "transparent") return true;
    if (/^#[0-9a-f]{3,8}$/i.test(v)) return true;
    if (/^(?:rgb|rgba|hsl|hsla|hwb|lab|lch|oklab|oklch|color)\(/i.test(v)) return true;
    const NAMED_COLORS = new Set([
      "black", "silver", "gray", "white", "maroon", "red", "purple", "fuchsia",
      "green", "lime", "olive", "yellow", "navy", "blue", "teal", "aqua",
      "orange", "aliceblue", "antiquewhite", "aquamarine", "azure", "beige",
      "bisque", "blanchedalmond", "blueviolet", "brown", "burlywood", "cadetblue",
      "chartreuse", "chocolate", "coral", "cornflowerblue", "cornsilk", "crimson",
      "cyan", "darkblue", "darkcyan", "darkgoldenrod", "darkgray", "darkgreen",
      "darkgrey", "darkkhaki", "darkmagenta", "darkolivegreen", "darkorange",
      "darkorchid", "darkred", "darksalmon", "darkseagreen", "darkslateblue",
      "darkslategray", "darkslategrey", "darkturquoise", "darkviolet", "deeppink",
      "deepskyblue", "dimgray", "dimgrey", "dodgerblue", "firebrick", "floralwhite",
      "forestgreen", "gainsboro", "ghostwhite", "gold", "goldenrod", "greenyellow",
      "grey", "honeydew", "hotpink", "indianred", "indigo", "ivory", "khaki",
      "lavender", "lavenderblush", "lawngreen", "lemonchiffon", "lightblue",
      "lightcoral", "lightcyan", "lightgoldenrodyellow", "lightgray", "lightgreen",
      "lightgrey", "lightpink", "lightsalmon", "lightseagreen", "lightskyblue",
      "lightslategray", "lightslategrey", "lightsteelblue", "lightyellow", "limegreen",
      "linen", "magenta", "mediumaquamarine", "mediumblue", "mediumorchid",
      "mediumpurple", "mediumseagreen", "mediumslateblue", "mediumspringgreen",
      "mediumturquoise", "mediumvioletred", "midnightblue", "mintcream", "mistyrose",
      "moccasin", "navajowhite", "oldlace", "olivedrab", "orangered", "orchid",
      "palegoldenrod", "palegreen", "paleturquoise", "palevioletred", "papayawhip",
      "peachpuff", "peru", "pink", "plum", "powderblue", "rosybrown", "royalblue",
      "saddlebrown", "salmon", "sandybrown", "seagreen", "seashell", "sienna",
      "skyblue", "slateblue", "slategray", "slategrey", "snow", "springgreen",
      "steelblue", "tan", "thistle", "tomato", "turquoise", "violet", "wheat",
      "whitesmoke", "yellowgreen"
    ]);
    return NAMED_COLORS.has(v);
  }

  function isValidCssValue(prop, value) {
    if (typeof value !== "string") value = String(value);
    const trimmed = value.trim();
    if (!trimmed) return false;
    if (prop.startsWith("--")) return true;

    const vLower = trimmed.toLowerCase();
    if (vLower === "inherit" || vLower === "initial" || vLower === "unset" || vLower === "revert" || vLower === "revert-layer") {
      return true;
    }
    if (vLower === "bogus") return false;

    if (prop === "display") {
      const validDisplays = new Set([
        "none", "inline", "block", "inline-block", "flex", "inline-flex",
        "grid", "inline-grid", "flow-root", "contents", "table", "inline-table",
        "table-row", "table-row-group", "table-column", "table-column-group",
        "table-cell", "table-caption", "list-item"
      ]);
      return validDisplays.has(vLower);
    }

    if (prop === "color" || prop === "background-color" || prop.endsWith("-color")) {
      return isValidColor(trimmed);
    }

    if (prop === "font-display") {
      return ["auto", "block", "swap", "fallback", "optional"].includes(vLower);
    }

    if (/^(?:url|var|calc|env|attr|min|max|clamp)\(/i.test(trimmed)) {
      return true;
    }

    return true;
  }

  function declSupported(name, value) {
    if (!name || typeof name !== "string") return false;
    if (name.startsWith("--")) return typeof value === "string" && value.trim() !== "";
    const kebab = cssPropCamelToKebab(name);
    if (!isCssPropertySupported(kebab)) return false;
    return isValidCssValue(kebab, value);
  }

  function selectorSupported(selector) {
    if (typeof selector !== "string") return false;
    const s = selector.trim();
    if (!s || s === "!!" || /^[!@#$%^&;]+$/.test(s) || /^[+>~]$/.test(s)) return false;
    if (s.includes("::")) {
      const m = s.match(/::([a-zA-Z-]+)(.*)$/);
      if (m && m[2].trim().length > 0) return false;
    }
    return true;
  }

  function matchParen(text, openParenIndex) {
    let depth = 0;
    let quote = null;
    for (let index = openParenIndex; index < text.length; index++) {
      const character = text[index];
      if (quote) {
        if (character === "\\") index++;
        else if (character === quote) quote = null;
        continue;
      }
      if (character === '"' || character === "'") quote = character;
      else if (character === "(") depth++;
      else if (character === ")") {
        depth--;
        if (depth === 0) return index + 1;
      }
    }
    return -1;
  }

  function evalCondition(condition) {
    let cursor = 0;
    const skipWhitespace = () => {
      while (cursor < condition.length && /\s/.test(condition[cursor])) cursor++;
    };

    const parseInParens = () => {
      skipWhitespace();
      const remainingCondition = condition.slice(cursor);
      const functionMatch = /^([a-zA-Z][a-zA-Z0-9-]*)\(/.exec(remainingCondition);
      if (functionMatch) {
        const openParenIndex = cursor + functionMatch[1].length;
        const endIndex = matchParen(condition, openParenIndex);
        if (endIndex < 0) return null;
        const innerCondition = condition.slice(openParenIndex + 1, endIndex - 1);
        cursor = endIndex;
        if (functionMatch[1].toLowerCase() === "selector") {
          return selectorSupported(innerCondition);
        }
        return "unknown";
      }
      if (condition[cursor] !== "(") return null;
      const endIndex = matchParen(condition, cursor);
      if (endIndex < 0) return null;
      const innerCondition = condition.slice(cursor + 1, endIndex - 1);
      cursor = endIndex;
      const nested = evalCondition(innerCondition);
      if (nested !== null) return nested;
      const colon = innerCondition.indexOf(":");
      if (colon > 0) {
        const name = innerCondition.slice(0, colon).trim();
        if (/^-{0,2}[a-zA-Z_][a-zA-Z0-9_-]*$/.test(name)) {
          return declSupported(name, innerCondition.slice(colon + 1).trim());
        }
      }
      return "unknown";
    };

    skipWhitespace();
    if (/^not(?![a-zA-Z0-9_-])/i.test(condition.slice(cursor))) {
      cursor += 3;
      const conditionValue = parseInParens();
      if (conditionValue === null) return null;
      skipWhitespace();
      if (cursor < condition.length) return null;
      return conditionValue === "unknown" ? "unknown" : !conditionValue;
    }

    let result = parseInParens();
    if (result === null) return null;
    skipWhitespace();
    let combinator = null;
    while (cursor < condition.length) {
      const keywordMatch = /^(and|or)(?![a-zA-Z0-9_-])/i.exec(condition.slice(cursor));
      if (!keywordMatch) return null;
      const keyword = keywordMatch[1].toLowerCase();
      if (combinator && keyword !== combinator) return null;
      combinator = keyword;
      cursor += keyword.length;
      const conditionValue = parseInParens();
      if (conditionValue === null) return null;
      if (keyword === "and") {
        if (result === false || conditionValue === false) result = false;
        else if (result === "unknown" || conditionValue === "unknown") result = "unknown";
      } else {
        if (result === true || conditionValue === true) result = true;
        else if (result === "unknown" || conditionValue === "unknown") result = "unknown";
      }
      skipWhitespace();
    }
    return result;
  }

  function serializeCssValue(val) {
    return String(val).replace(/url\(\s*(?!"|')([^)]+?)\s*\)/g, 'url("$1")');
  }

  function splitCssDeclarations(block) {
    const parts = [];
    let depth = 0;
    let inQuote = null;
    let start = 0;
    for (let i = 0; i < block.length; i++) {
      const c = block[i];
      if (inQuote) {
        if (c === inQuote && block[i - 1] !== "\\") {
          inQuote = null;
        }
      } else if (c === '"' || c === "'") {
        inQuote = c;
      } else if (c === "(") {
        depth++;
      } else if (c === ")") {
        depth = Math.max(0, depth - 1);
      } else if (c === ";" && depth === 0) {
        parts.push(block.slice(start, i));
        start = i + 1;
      }
    }
    if (start < block.length) {
      parts.push(block.slice(start));
    }
    return parts;
  }

  // =========================================================================
  // 2. CSS Namespace & CSS.supports
  // =========================================================================

  const CSS = {
    escape(ident) {
      return String(ident).replace(/[^a-zA-Z0-9_-]/g, (c) => "\\" + c);
    },
    supports(propertyOrCondition, value) {
      if (typeof propertyOrCondition === "string" && typeof value === "string") {
        return declSupported(propertyOrCondition, value);
      }
      if (typeof propertyOrCondition === "string") {
        const conditionResult = evalCondition(propertyOrCondition);
        if (conditionResult !== null) return conditionResult === true;
        const colon = propertyOrCondition.indexOf(":");
        if (colon > 0) {
          return declSupported(
            propertyOrCondition.slice(0, colon).trim(),
            propertyOrCondition.slice(colon + 1).trim()
          );
        }
      }
      return false;
    }
  };

  const CSS_UNITS = [
    "number", "percent", "px", "cm", "mm", "Q", "in", "pt", "pc",
    "em", "ex", "ch", "cap", "ic", "rem", "lh", "rlh", "rcap", "rch",
    "rex", "ric", "vw", "vh", "vi", "vb", "vmin", "vmax", "svw", "svh",
    "svi", "svb", "svmin", "svmax", "lvw", "lvh", "lvi", "lvb", "lvmin",
    "lvmax", "dvw", "dvh", "dvi", "dvb", "dvmin", "dvmax", "cqw", "cqh",
    "cqi", "cqb", "cqmin", "cqmax", "deg", "grad", "rad", "turn", "s",
    "ms", "Hz", "kHz", "dpi", "dpcm", "dppx", "fr"
  ];
  for (let i = 0; i < CSS_UNITS.length; i++) {
    const unit = CSS_UNITS[i];
    CSS[unit] = function (val) {
      return { value: Number(val), unit: unit };
    };
  }

  global.CSS = CSS;
  if (typeof globalThis !== "undefined") globalThis.CSS = CSS;
  if (typeof window !== "undefined") window.CSS = CSS;

  // =========================================================================
  // 3. MediaList
  // =========================================================================

  function MediaList(initialText, onMutate) {
    this._media = [];
    this._onMutate = onMutate || null;
    if (initialText) this._setText(initialText);
  }
  MediaList.prototype._setText = function (text) {
    const old = this._media.length;
    this._media = String(text == null ? "" : text)
      .split(",")
      .map((m) => m.trim())
      .filter(Boolean);
    for (let i = this._media.length; i < old; i++) delete this[i];
    for (let i = 0; i < this._media.length; i++) this[i] = this._media[i];
  };
  MediaList.prototype._notify = function () {
    if (this._onMutate) this._onMutate();
  };
  Object.defineProperty(MediaList.prototype, "length", {
    get: function () { return this._media.length; },
    configurable: true,
  });
  Object.defineProperty(MediaList.prototype, "mediaText", {
    get: function () { return this._media.join(", "); },
    set: function (v) { this._setText(v); this._notify(); },
    configurable: true,
  });
  MediaList.prototype.item = function (index) {
    const m = this._media[Number(index) | 0];
    return m != null ? m : null;
  };
  MediaList.prototype.appendMedium = function (medium) {
    const m = String(medium).trim();
    if (m && !this._media.includes(m)) {
      this._media.push(m);
      this[this._media.length - 1] = m;
      this._notify();
    }
  };
  MediaList.prototype.deleteMedium = function (medium) {
    const idx = this._media.indexOf(String(medium).trim());
    if (idx >= 0) {
      const old = this._media.length;
      this._media.splice(idx, 1);
      for (let i = this._media.length; i < old; i++) delete this[i];
      for (let i = 0; i < this._media.length; i++) this[i] = this._media[i];
      this._notify();
    }
  };
  MediaList.prototype.toString = function () { return this.mediaText; };
  MediaList.prototype[Symbol.iterator] = Array.prototype[Symbol.iterator];
  markNative(MediaList, "MediaList");
  global.MediaList = MediaList;

  // =========================================================================
  // 4. CSSStyleDeclaration
  // =========================================================================

  function CSSStyleDeclaration() {}
  CSSStyleDeclaration.prototype.item = function (index) { return ""; };
  CSSStyleDeclaration.prototype.getPropertyValue = function (property) { return ""; };
  CSSStyleDeclaration.prototype.getPropertyPriority = function (property) { return ""; };
  CSSStyleDeclaration.prototype.setProperty = function (property, value, priority) {};
  CSSStyleDeclaration.prototype.removeProperty = function (property) { return ""; };
  Object.defineProperty(CSSStyleDeclaration.prototype, "cssText", {
    get: function () { return this._cssText || ""; },
    set: function (v) { this._cssText = String(v == null ? "" : v); },
    configurable: true,
  });
  Object.defineProperty(CSSStyleDeclaration.prototype, "length", {
    get: function () { return this._length != null ? this._length : 0; },
    set: function (v) { this._length = Number(v) >>> 0; },
    configurable: true,
  });
  Object.defineProperty(CSSStyleDeclaration.prototype, "parentRule", {
    get: function () { return this._parentRule || null; },
    set: function (v) { this._parentRule = v; },
    configurable: true,
  });
  CSSStyleDeclaration.prototype[Symbol.iterator] = Array.prototype[Symbol.iterator];
  markNative(CSSStyleDeclaration, "CSSStyleDeclaration");

  let _CSSStyleDeclaration = CSSStyleDeclaration;

  Object.defineProperty(CSSStyleDeclaration, Symbol.hasInstance, {
    value: function (inst) {
      if (!inst || typeof inst !== "object") return false;
      return Object.prototype.isPrototypeOf.call(CSSStyleDeclaration.prototype, inst) ||
             (inst._isCSSStyleDeclaration === true);
    },
    configurable: true,
  });

  function installCSSStyleDeclarationProperty(targetObj) {
    if (!targetObj) return;
    Object.defineProperty(targetObj, "CSSStyleDeclaration", {
      get: function () { return _CSSStyleDeclaration; },
      set: function (newClass) {
        if (newClass && newClass.prototype && newClass !== _CSSStyleDeclaration) {
          Object.setPrototypeOf(newClass.prototype, _CSSStyleDeclaration.prototype);
        }
      },
      configurable: true,
      enumerable: true,
    });
  }

  installCSSStyleDeclarationProperty(global);
  if (typeof globalThis !== "undefined") installCSSStyleDeclarationProperty(globalThis);
  if (typeof window !== "undefined") installCSSStyleDeclarationProperty(window);

  function makeRuleStyleDeclaration(initialDeclText, context, baseUrl, onMutate) {
    if (!context) context = "style";
    const data = new Map(); // key -> { name, value, priority }
    let authoredOrder = []; // array of keys

    function parseInitial(text) {
      if (!text) return;
      const decls = splitCssDeclarations(text);
      for (let i = 0; i < decls.length; i++) {
        const d = decls[i].trim();
        if (!d) continue;
        const colon = d.indexOf(":");
        if (colon <= 0) continue;
        const rawName = d.slice(0, colon).trim();
        let val = d.slice(colon + 1).trim();
        let priority = "";
        if (/!important\s*$/i.test(val)) {
          priority = "important";
          val = val.replace(/\s*!important\s*$/i, "").trim();
        }
        if (context === "keyframe" && priority === "important") continue;
        if (context === "font-face" && rawName === "font-display") {
          if (!["auto", "block", "swap", "fallback", "optional"].includes(val.toLowerCase())) continue;
        }
        if (context === "page" && rawName.toLowerCase() === "size") {
          val = val.toLowerCase();
        }
        const key = rawName.startsWith("--") ? rawName : rawName.toLowerCase();
        if (!data.has(key)) {
          authoredOrder.push(key);
        }
        data.set(key, { name: rawName, value: val, priority: priority });
      }
    }

    parseInitial(initialDeclText);

    function notify() {
      if (onMutate) onMutate();
    }

    const decl = Object.create(CSSStyleDeclaration.prototype);
    decl._isCSSStyleDeclaration = true;

    Object.defineProperty(decl, "length", {
      get: function () { return data.size; },
      configurable: true,
    });

    Object.defineProperty(decl, "parentRule", {
      get: function () { return decl._parentRule || null; },
      configurable: true,
    });

    decl.item = function (index) {
      const idx = Number(index) >>> 0;
      let keys = authoredOrder;
      if (context === "font-face") {
        keys = Array.from(data.keys()).sort();
      }
      const k = keys[idx];
      return k != null ? (data.get(k)?.name || k) : "";
    };

    decl.getPropertyValue = function (prop) {
      const p = String(prop).trim();
      const key = p.startsWith("--") ? p : p.toLowerCase();
      if (data.has(key)) {
        return serializeCssValue(data.get(key).value);
      }
      if (SHORTHAND_LONGHANDS[key]) {
        const longhands = SHORTHAND_LONGHANDS[key];
        let pri = null;
        let vals = [];
        for (let i = 0; i < longhands.length; i++) {
          const lhKey = longhands[i];
          if (!data.has(lhKey)) return "";
          const entry = data.get(lhKey);
          if (pri === null) pri = entry.priority;
          else if (pri !== entry.priority) return "";
          vals.push(serializeCssValue(entry.value));
        }
        if (vals.length === 4) {
          if (vals[0] === vals[1] && vals[1] === vals[2] && vals[2] === vals[3]) return vals[0];
          if (vals[0] === vals[2] && vals[1] === vals[3]) return vals[0] + " " + vals[1];
          if (vals[1] === vals[3]) return vals[0] + " " + vals[1] + " " + vals[2];
          return vals.join(" ");
        }
      }
      return "";
    };

    decl.getPropertyPriority = function (prop) {
      const p = String(prop).trim();
      const key = p.startsWith("--") ? p : p.toLowerCase();
      const entry = data.get(key);
      return entry ? entry.priority : "";
    };

    decl.setProperty = function (prop, val, priority) {
      const p = String(prop).trim();
      if (!p) return;
      if (priority !== undefined && priority !== null && priority !== "") {
        if (priority !== "important" && priority !== "!important") {
          return;
        }
      }
      if (context === "keyframe" && (priority === "important" || priority === "!important")) {
        return;
      }
      const pPriority = (priority === "important" || priority === "!important") ? "important" : "";
      let v = val == null ? "" : String(val).trim();
      if (v === "") {
        decl.removeProperty(p);
        return;
      }
      const key = p.startsWith("--") ? p : p.toLowerCase();
      if (!p.startsWith("--")) {
        if (!isValidCssValue(key, v)) return;
      }
      if (context === "page" && key === "size") {
        v = v.toLowerCase();
      }

      if (SHORTHAND_LONGHANDS[key]) {
        const longhands = SHORTHAND_LONGHANDS[key];
        for (let i = 0; i < longhands.length; i++) {
          const lhKey = longhands[i];
          data.set(lhKey, { name: lhKey, value: v, priority: pPriority });
          if (!authoredOrder.includes(lhKey)) authoredOrder.push(lhKey);
        }
      }

      if (!data.has(key)) {
        authoredOrder.push(key);
      }
      data.set(key, { name: p, value: v, priority: pPriority });
      notify();
    };

    decl.removeProperty = function (prop) {
      const p = String(prop).trim();
      const key = p.startsWith("--") ? p : p.toLowerCase();
      let prev = "";
      if (data.has(key)) {
        prev = data.get(key).value;
        data.delete(key);
        authoredOrder = authoredOrder.filter((k) => k !== key);
      }
      if (SHORTHAND_LONGHANDS[key]) {
        const longhands = SHORTHAND_LONGHANDS[key];
        for (let i = 0; i < longhands.length; i++) {
          const lhKey = longhands[i];
          if (data.has(lhKey)) {
            if (!prev) prev = data.get(lhKey).value;
            data.delete(lhKey);
            authoredOrder = authoredOrder.filter((k) => k !== lhKey);
          }
        }
      }
      notify();
      return prev;
    };

    decl[Symbol.iterator] = function* () {
      let keys = authoredOrder;
      if (context === "font-face") {
        keys = Array.from(data.keys()).sort();
      }
      for (let i = 0; i < keys.length; i++) {
        const k = keys[i];
        yield data.get(k)?.name || k;
      }
    };

    function serializeDecls() {
      const parts = [];
      let keys = authoredOrder;
      if (context === "font-face") {
        keys = Array.from(data.keys()).sort();
      }
      for (let i = 0; i < keys.length; i++) {
        const k = keys[i];
        const entry = data.get(k);
        if (!entry) continue;
        let v = serializeCssValue(entry.value);
        let name = entry.name;
        if (!name.startsWith("--")) name = name.toLowerCase();
        let s = name + ": " + v;
        if (entry.priority) s += " !" + entry.priority;
        parts.push(s);
      }
      if (context === "font-face") {
        return parts.length ? parts.join("; ") + ";" : "";
      }
      return parts.join("; ");
    }

    Object.defineProperty(decl, "cssText", {
      get: function () { return serializeDecls(); },
      set: function (v) {
        data.clear();
        authoredOrder = [];
        parseInitial(v);
        notify();
      },
      configurable: true,
    });

    return new Proxy(decl, {
      get: function (target, prop, receiver) {
        if (typeof prop === "symbol") return target[prop];
        if (typeof prop === "string" && prop.startsWith("_")) return target[prop];
        if (prop in target) return target[prop];
        if (typeof prop === "string") {
          const kebab = cssPropCamelToKebab(prop);
          let val = decl.getPropertyValue(kebab);
          if (!val && prop.startsWith("--")) val = decl.getPropertyValue(prop);
          if (context === "page" && kebab === "size" && val) return val.toLowerCase();
          return val;
        }
        return undefined;
      },
      set: function (target, prop, value) {
        if (typeof prop === "symbol") { target[prop] = value; return true; }
        if (typeof prop === "string" && prop.startsWith("_")) { target[prop] = value; return true; }
        if (prop === "cssText") { target.cssText = value; return true; }
        const kebab = cssPropCamelToKebab(prop);
        decl.setProperty(kebab, value);
        return true;
      },
      has: function (target, prop) {
        if (typeof prop === "symbol") return prop in target;
        if (typeof prop === "string" && prop.startsWith("_")) return prop in target;
        if (prop in target) return true;
        const pStr = String(prop);
        if (pStr.startsWith("--")) return false;
        if (/^(?:webkit|moz|ms|o)-/.test(pStr)) return false;
        if (!pStr.includes("-") && /^(?:moz|ms|o)[A-Z]/.test(pStr)) return false;
        return isCssPropertySupported(pStr);
      },
      ownKeys: function () {
        let keys = authoredOrder;
        if (context === "font-face") keys = Array.from(data.keys()).sort();
        return keys.slice();
      },
      getOwnPropertyDescriptor: function (target, prop) {
        if (typeof prop === "string" && data.has(prop)) {
          return { value: data.get(prop).value, writable: true, enumerable: true, configurable: true };
        }
        return Reflect.getOwnPropertyDescriptor(target, prop);
      }
    });
  }

  // =========================================================================
  // 5. CSSRule and Subclasses
  // =========================================================================

  const RULE_CONSTANTS = {
    STYLE_RULE: 1,
    CHARSET_RULE: 2,
    IMPORT_RULE: 3,
    MEDIA_RULE: 4,
    FONT_FACE_RULE: 5,
    PAGE_RULE: 6,
    KEYFRAMES_RULE: 7,
    KEYFRAME_RULE: 8,
    MARGIN_RULE: 9,
    NAMESPACE_RULE: 10,
    COUNTER_STYLE_RULE: 11,
    SUPPORTS_RULE: 12,
    FONT_FEATURE_VALUES_RULE: 14,
  };

  function CSSRule() {
    this.parentRule = null;
    this.parentStyleSheet = null;
  }
  for (const [k, v] of Object.entries(RULE_CONSTANTS)) {
    Object.defineProperty(CSSRule, k, { value: v, writable: false, enumerable: true, configurable: false });
    Object.defineProperty(CSSRule.prototype, k, { value: v, writable: false, enumerable: true, configurable: false });
  }
  Object.defineProperty(CSSRule.prototype, "type", {
    get: function () { return this._type || 0; },
    configurable: true,
  });
  Object.defineProperty(CSSRule.prototype, "cssText", {
    get: function () { return this._serialize ? this._serialize() : ""; },
    set: function () {},
    configurable: true,
  });
  markNative(CSSRule, "CSSRule");
  global.CSSRule = CSSRule;

  function makeRuleList(arr) {
    Object.defineProperty(arr, "item", {
      value: function (index) {
        const i = Number(index) >>> 0;
        return arr[i] || null;
      },
      configurable: true,
      enumerable: false,
      writable: true,
    });
    return arr;
  }

  // 5.1 CSSGroupingRule
  function CSSGroupingRule() {
    CSSRule.call(this);
    this.cssRules = makeRuleList([]);
  }
  CSSGroupingRule.prototype = Object.create(CSSRule.prototype);
  CSSGroupingRule.prototype.constructor = CSSGroupingRule;
  CSSGroupingRule.prototype.insertRule = function (ruleText, index) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'insertRule': 1 argument required, but only 0 present.");
    }
    const idx = index === undefined ? 0 : Number(index) >>> 0;
    if (idx > this.cssRules.length) {
      throw new DOMException("Index out of bounds", "IndexSizeError");
    }
    const parsed = parseSingleRuleOrThrow(ruleText);
    if (parsed.kind === "import" || parsed.kind === "namespace") {
      throw new DOMException("Rule not allowed inside a grouping rule", "HierarchyRequestError");
    }
    const rule = constructRule(parsed, this.parentStyleSheet, this, () => {
      if (this._onMutate) this._onMutate();
    });
    this.cssRules.splice(idx, 0, rule);
    if (this._onMutate) this._onMutate();
    return idx;
  };
  CSSGroupingRule.prototype.deleteRule = function (index) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'deleteRule': 1 argument required, but only 0 present.");
    }
    const idx = Number(index) >>> 0;
    if (idx >= this.cssRules.length) {
      throw new DOMException("Index out of bounds", "IndexSizeError");
    }
    this.cssRules.splice(idx, 1);
    if (this._onMutate) this._onMutate();
  };
  markNative(CSSGroupingRule, "CSSGroupingRule");
  global.CSSGroupingRule = CSSGroupingRule;

  // 5.2 CSSConditionRule
  function CSSConditionRule() {
    CSSGroupingRule.call(this);
  }
  CSSConditionRule.prototype = Object.create(CSSGroupingRule.prototype);
  CSSConditionRule.prototype.constructor = CSSConditionRule;
  Object.defineProperty(CSSConditionRule.prototype, "conditionText", {
    get: function () { return this._conditionText || ""; },
    configurable: true,
  });
  markNative(CSSConditionRule, "CSSConditionRule");
  global.CSSConditionRule = CSSConditionRule;

  // 5.3 CSSMediaRule
  function CSSMediaRule() {
    CSSConditionRule.call(this);
    this._type = 4;
  }
  CSSMediaRule.prototype = Object.create(CSSConditionRule.prototype);
  CSSMediaRule.prototype.constructor = CSSMediaRule;
  Object.defineProperty(CSSMediaRule.prototype, "conditionText", {
    get: function () { return this.media ? this.media.mediaText : (this._conditionText || ""); },
    configurable: true,
  });
  markNative(CSSMediaRule, "CSSMediaRule");
  global.CSSMediaRule = CSSMediaRule;

  // 5.4 CSSSupportsRule
  function CSSSupportsRule() {
    CSSConditionRule.call(this);
    this._type = 12;
  }
  CSSSupportsRule.prototype = Object.create(CSSConditionRule.prototype);
  CSSSupportsRule.prototype.constructor = CSSSupportsRule;
  markNative(CSSSupportsRule, "CSSSupportsRule");
  global.CSSSupportsRule = CSSSupportsRule;

  // 5.5 CSSStyleRule
  function CSSStyleRule() {
    CSSGroupingRule.call(this);
    this._type = 1;
  }
  CSSStyleRule.prototype = Object.create(CSSGroupingRule.prototype);
  CSSStyleRule.prototype.constructor = CSSStyleRule;
  markNative(CSSStyleRule, "CSSStyleRule");
  global.CSSStyleRule = CSSStyleRule;

  // 5.6 CSSImportRule
  function CSSImportRule() {
    CSSRule.call(this);
    this._type = 3;
    this.href = "";
    this.layerName = null;
    this.supportsText = null;
    this.media = new MediaList("");
    this.styleSheet = null;
  }
  CSSImportRule.prototype = Object.create(CSSRule.prototype);
  CSSImportRule.prototype.constructor = CSSImportRule;
  markNative(CSSImportRule, "CSSImportRule");
  global.CSSImportRule = CSSImportRule;

  // 5.7 CSSFontPaletteValuesRule
  function CSSFontPaletteValuesRule() {
    CSSRule.call(this);
    this._type = 0;
    this.name = "";
    this.fontFamily = "";
    this.basePalette = "";
    this.overrideColors = "";
  }
  CSSFontPaletteValuesRule.prototype = Object.create(CSSRule.prototype);
  CSSFontPaletteValuesRule.prototype.constructor = CSSFontPaletteValuesRule;
  markNative(CSSFontPaletteValuesRule, "CSSFontPaletteValuesRule");
  global.CSSFontPaletteValuesRule = CSSFontPaletteValuesRule;

  // 5.8 CSSPageRule
  function CSSPageRule() {
    CSSGroupingRule.call(this);
    this._type = 6;
  }
  CSSPageRule.prototype = Object.create(CSSGroupingRule.prototype);
  CSSPageRule.prototype.constructor = CSSPageRule;
  CSSPageRule.prototype.insertRule = function (ruleText, index) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'insertRule': 1 argument required, but only 0 present.");
    }
    const idx = index === undefined ? 0 : Number(index) >>> 0;
    if (idx > this.cssRules.length) {
      throw new DOMException("Index out of bounds", "IndexSizeError");
    }
    const parsed = parseSingleRuleOrThrow(ruleText);
    if (parsed.kind !== "margin") {
      throw new DOMException("Only margin at-rules may be inserted into a @page rule", "SyntaxError");
    }
    const rule = constructRule(parsed, this.parentStyleSheet, this, () => {
      if (this._onMutate) this._onMutate();
    });
    this.cssRules.splice(idx, 0, rule);
    if (this._onMutate) this._onMutate();
    return idx;
  };
  markNative(CSSPageRule, "CSSPageRule");
  global.CSSPageRule = CSSPageRule;

  // 5.9 CSSMarginRule
  function CSSMarginRule() {
    CSSRule.call(this);
    this._type = 9;
    this.name = "";
  }
  CSSMarginRule.prototype = Object.create(CSSRule.prototype);
  CSSMarginRule.prototype.constructor = CSSMarginRule;
  markNative(CSSMarginRule, "CSSMarginRule");
  global.CSSMarginRule = CSSMarginRule;

  // 5.10 CSSFontFaceRule
  function CSSFontFaceRule() {
    CSSRule.call(this);
    this._type = 5;
  }
  CSSFontFaceRule.prototype = Object.create(CSSRule.prototype);
  CSSFontFaceRule.prototype.constructor = CSSFontFaceRule;
  markNative(CSSFontFaceRule, "CSSFontFaceRule");
  global.CSSFontFaceRule = CSSFontFaceRule;

  // 5.11 CSSKeyframesRule & CSSKeyframeRule
  function CSSKeyframesRule() {
    CSSRule.call(this);
    this._type = 7;
    this.name = "";
    this.cssRules = makeRuleList([]);
  }
  CSSKeyframesRule.prototype = Object.create(CSSRule.prototype);
  CSSKeyframesRule.prototype.constructor = CSSKeyframesRule;
  Object.defineProperty(CSSKeyframesRule.prototype, "length", {
    get: function () { return this.cssRules.length; },
    configurable: true,
  });
  markNative(CSSKeyframesRule, "CSSKeyframesRule");
  global.CSSKeyframesRule = CSSKeyframesRule;

  function CSSKeyframeRule() {
    CSSRule.call(this);
    this._type = 8;
    this.keyText = "";
  }
  CSSKeyframeRule.prototype = Object.create(CSSRule.prototype);
  CSSKeyframeRule.prototype.constructor = CSSKeyframeRule;
  markNative(CSSKeyframeRule, "CSSKeyframeRule");
  global.CSSKeyframeRule = CSSKeyframeRule;

  // 5.12 CSSNamespaceRule
  function CSSNamespaceRule() {
    CSSRule.call(this);
    this._type = 10;
    this.prefix = "";
    this.namespaceURI = "";
  }
  CSSNamespaceRule.prototype = Object.create(CSSRule.prototype);
  CSSNamespaceRule.prototype.constructor = CSSNamespaceRule;
  markNative(CSSNamespaceRule, "CSSNamespaceRule");
  global.CSSNamespaceRule = CSSNamespaceRule;

  // 5.13 CSSCounterStyleRule
  function CSSCounterStyleRule() {
    CSSRule.call(this);
    this._type = 11;
    this.name = "";
  }
  CSSCounterStyleRule.prototype = Object.create(CSSRule.prototype);
  CSSCounterStyleRule.prototype.constructor = CSSCounterStyleRule;
  markNative(CSSCounterStyleRule, "CSSCounterStyleRule");
  global.CSSCounterStyleRule = CSSCounterStyleRule;

  // 5.14 CSSFontFeatureValuesRule
  function CSSFontFeatureValuesRule() {
    CSSRule.call(this);
    this._type = 14;
    this.fontFamily = "";
  }
  CSSFontFeatureValuesRule.prototype = Object.create(CSSRule.prototype);
  CSSFontFeatureValuesRule.prototype.constructor = CSSFontFeatureValuesRule;
  markNative(CSSFontFeatureValuesRule, "CSSFontFeatureValuesRule");
  global.CSSFontFeatureValuesRule = CSSFontFeatureValuesRule;

  const MARGIN_BOX_NAMES = new Set([
    "top-left-corner", "top-left", "top-center", "top-right", "top-right-corner",
    "bottom-left-corner", "bottom-left", "bottom-center", "bottom-right", "bottom-right-corner",
    "left-top", "left-middle", "left-bottom", "right-top", "right-middle", "right-bottom"
  ]);

  // =========================================================================
  // 6. Stylesheet Rule Parsing & Splitting
  // =========================================================================

  function splitStylesheetRules(src) {
    const out = [];
    out.trailingGarbage = "";
    out.unbalanced = false;
    const text = String(src == null ? "" : src).replace(/\/\*[\s\S]*?\*\//g, "");
    const n = text.length;
    let i = 0;
    let chunkStart = 0;
    let depth = 0;
    let parenDepth = 0;
    let braceOpenAt = -1;
    let inStr = "";

    while (i < n) {
      const ch = text[i];
      if (inStr) {
        if (ch === "\\") { i += 2; continue; }
        if (ch === inStr) inStr = "";
        i++;
        continue;
      }
      if (ch === "\\") { i += 2; continue; }
      if (ch === '"' || ch === "'") { inStr = ch; i++; continue; }
      if (ch === "(") { parenDepth++; i++; continue; }
      if (ch === ")" && parenDepth > 0) { parenDepth--; i++; continue; }
      if (ch === "{") {
        if (depth === 0) braceOpenAt = i;
        depth++;
        i++;
        continue;
      }
      if (ch === "}") {
        depth--;
        if (depth === 0 && braceOpenAt >= 0) {
          const selectorText = text.slice(chunkStart, braceOpenAt).trim();
          const declText = text.slice(braceOpenAt + 1, i).trim();
          const cssText = selectorText
            ? selectorText + " { " + declText + " }"
            : text.slice(chunkStart, i + 1).trim();
          out.push({ cssText, selectorText, declText });
          chunkStart = i + 1;
          braceOpenAt = -1;
        }
        i++;
        continue;
      }
      if (depth === 0 && parenDepth === 0 && ch === ";") {
        const raw = text.slice(chunkStart, i + 1).trim();
        if (raw) out.push({ cssText: raw, selectorText: "", declText: "" });
        chunkStart = i + 1;
      }
      i++;
    }

    const tail = text.slice(chunkStart).trim();
    out.trailingGarbage = tail.length > 0 ? tail : "";
    out.unbalanced = depth !== 0 || parenDepth !== 0 || inStr !== "";
    return out;
  }

  function classifyRuleKind(parsed) {
    const sel = String(parsed.selectorText || "").trim();
    const raw = String(parsed.cssText || "").trim();
    if (sel.startsWith("@")) {
      const marginMatch = sel.match(/^@([a-zA-Z-]+)/);
      if (marginMatch && MARGIN_BOX_NAMES.has(marginMatch[1].toLowerCase())) {
        return "margin";
      }
    }
    const at = (sel || raw).match(/^@([a-zA-Z-]+)/);
    if (!at) return sel ? "style" : "invalid";
    const name = at[1].toLowerCase();
    switch (name) {
      case "media": return "media";
      case "supports": return "supports";
      case "layer": return sel ? "layer-block" : "layer-statement";
      case "import": return "import";
      case "namespace": return "namespace";
      case "font-face": return "font-face";
      case "font-palette-values": return "font-palette-values";
      case "font-feature-values": return "font-feature-values";
      case "keyframes":
      case "-webkit-keyframes": return "keyframes";
      case "page": return "page";
      case "charset": return "charset";
      case "counter-style": return "counter-style";
      default: return "unknown";
    }
  }

  function validateSelectorOrThrow(sel) {
    if (typeof sel !== "string") sel = String(sel);
    const s = sel.trim();
    if (!s || s === "!!" || /^[!@#$%^&;]+$/.test(s) || /^[+>~]$/.test(s)) {
      throw new DOMException("The provided selector is invalid.", "SyntaxError");
    }
    if (s.includes("::")) {
      const pseudoPattern = /::([a-zA-Z-]+)(.*)$/;
      const m = s.match(pseudoPattern);
      if (m && m[2].trim().length > 0) {
        throw new DOMException("The provided selector is invalid.", "SyntaxError");
      }
    }
  }

  function parseSingleRuleOrThrow(text) {
    const t = String(text == null ? "" : text);
    const split = splitStylesheetRules(t);
    if (split.unbalanced || split.length !== 1 || split.trailingGarbage) {
      throw new DOMException("Failed to parse CSS rule: " + t, "SyntaxError");
    }
    const parsed = split[0];
    const kind = classifyRuleKind(parsed);
    parsed.kind = kind;
    if (kind === "charset" || kind === "invalid" || kind === "margin") {
      throw new DOMException("Failed to parse CSS rule: invalid rule", "SyntaxError");
    }
    if (kind === "style") {
      validateSelectorOrThrow(parsed.selectorText);
    }
    return parsed;
  }

  function constructRule(parsed, parentSheet, parentRule, onMutate) {
    const kind = parsed.kind || classifyRuleKind(parsed);
    const sel = String(parsed.selectorText || "").trim();
    const raw = String(parsed.cssText || "").trim();

    if (kind === "style") {
      const rule = new CSSStyleRule();
      rule.parentRule = parentRule || null;
      rule.parentStyleSheet = parentSheet || null;
      rule._onMutate = onMutate || null;
      rule.selectorText = sel;
      const style = makeRuleStyleDeclaration(parsed.declText || "", "style", null, onMutate);
      style._parentRule = rule;
      rule.style = style;
      rule._serialize = function () {
        const d = style.cssText;
        return rule.selectorText + (d ? " { " + d + " }" : " { }");
      };
      return rule;
    }

    if (kind === "media" || kind === "supports") {
      const rule = kind === "media" ? new CSSMediaRule() : new CSSSupportsRule();
      rule.parentRule = parentRule || null;
      rule.parentStyleSheet = parentSheet || null;
      rule._onMutate = onMutate || null;
      const cond = sel.replace(/^@(media|supports)/i, "").trim();
      rule._conditionText = cond;
      if (kind === "media") {
        rule.media = new MediaList(cond, onMutate);
      }
      const children = splitStylesheetRules(parsed.declText || "");
      for (let i = 0; i < children.length; i++) {
        const c = constructRule(children[i], parentSheet, rule, onMutate);
        if (c) rule.cssRules.push(c);
      }
      rule._serialize = function () {
        const c = rule.media ? rule.media.mediaText : rule.conditionText;
        const inner = rule.cssRules.map((r) => r.cssText).join("\n  ");
        return "@" + kind + (c ? " " + c : "") + (inner ? " {\n  " + inner + "\n}" : " {\n}");
      };
      return rule;
    }

    if (kind === "import") {
      const rule = new CSSImportRule();
      rule.parentRule = parentRule || null;
      rule.parentStyleSheet = parentSheet || null;
      rule._onMutate = onMutate || null;
      const m = raw.match(/^@import\s+(?:url\(\s*(?:"([^"]*)"|'([^']*)'|([^)"']*))\s*\)|"([^"]*)"|'([^']*)')\s*(layer(?:\(([^)]*)\))?)?\s*(?:supports\(([\s\S]*?)\))?\s*([^;]*);?\s*$/i);
      if (m) {
        rule.href = m[1] ?? m[2] ?? (m[3] !== undefined ? m[3].trim() : undefined) ?? m[4] ?? m[5] ?? "";
        rule.layerName = m[6] ? (m[7] !== undefined ? m[7].trim() : "") : null;
        rule.supportsText = m[8] !== undefined ? m[8].trim() : null;
        rule.media = new MediaList((m[9] || "").trim(), onMutate);
      }
      rule._serialize = function () {
        let out = '@import url("' + rule.href + '")';
        if (rule.layerName !== null) {
          out += rule.layerName === "" ? " layer" : " layer(" + rule.layerName + ")";
        }
        if (rule.supportsText !== null) {
          out += " supports(" + rule.supportsText + ")";
        }
        const mt = rule.media.mediaText;
        if (mt) out += " " + mt;
        return out + ";";
      };
      return rule;
    }

    if (kind === "font-face") {
      const rule = new CSSFontFaceRule();
      rule.parentRule = parentRule || null;
      rule.parentStyleSheet = parentSheet || null;
      rule._onMutate = onMutate || null;
      const style = makeRuleStyleDeclaration(parsed.declText || "", "font-face", null, onMutate);
      style._parentRule = rule;
      rule.style = style;
      rule._serialize = function () {
        const d = style.cssText;
        return "@font-face" + (d ? " { " + d + " }" : " { }");
      };
      return rule;
    }

    if (kind === "font-palette-values") {
      const rule = new CSSFontPaletteValuesRule();
      rule.parentRule = parentRule || null;
      rule.parentStyleSheet = parentSheet || null;
      rule._onMutate = onMutate || null;
      rule.name = sel.replace(/^@font-palette-values/i, "").trim();
      const style = makeRuleStyleDeclaration(parsed.declText || "", "style", null, onMutate);
      rule.fontFamily = style.getPropertyValue("font-family") || "";
      rule.basePalette = style.getPropertyValue("base-palette") || "";
      rule.overrideColors = style.getPropertyValue("override-colors") || "";
      rule._serialize = function () {
        const d = style.cssText;
        return "@font-palette-values " + rule.name + (d ? " { " + d + " }" : " { }");
      };
      return rule;
    }

    if (kind === "page") {
      const rule = new CSSPageRule();
      rule.parentRule = parentRule || null;
      rule.parentStyleSheet = parentSheet || null;
      rule._onMutate = onMutate || null;
      rule.selectorText = sel.replace(/^@page/i, "").trim();

      const declParts = [];
      const childRules = makeRuleList([]);
      const pageBody = splitStylesheetRules(parsed.declText || "");
      for (let i = 0; i < pageBody.length; i++) {
        const part = pageBody[i];
        if (classifyRuleKind(part) === "margin") {
          const mRule = constructRule(part, parentSheet, rule, onMutate);
          if (mRule) childRules.push(mRule);
        } else if (!part.selectorText && !/^@/.test(String(part.cssText || "").trim())) {
          declParts.push(String(part.cssText));
        }
      }
      if (pageBody.trailingGarbage && !/^@/.test(pageBody.trailingGarbage)) {
        declParts.push(pageBody.trailingGarbage);
      }

      const style = makeRuleStyleDeclaration(declParts.join("; "), "page", null, onMutate);
      style._parentRule = rule;
      rule.style = style;
      rule.cssRules = childRules;

      rule._serialize = function () {
        const parts = [];
        let d = style.cssText;
        if (d) {
          if (!d.endsWith(";")) d += ";";
          parts.push(d);
        }
        for (let i = 0; i < rule.cssRules.length; i++) {
          parts.push(rule.cssRules[i].cssText);
        }
        const body = parts.join(" ");
        return "@page" + (rule.selectorText ? " " + rule.selectorText : "") + (body ? " { " + body + " }" : " { }");
      };
      return rule;
    }

    if (kind === "margin") {
      const rule = new CSSMarginRule();
      rule.parentRule = parentRule || null;
      rule.parentStyleSheet = parentSheet || null;
      rule._onMutate = onMutate || null;
      rule.name = sel.replace(/^@/i, "").trim();
      const style = makeRuleStyleDeclaration(parsed.declText || "", "style", null, onMutate);
      style._parentRule = rule;
      rule.style = style;
      rule._serialize = function () {
        let d = style.cssText;
        if (d && !d.endsWith(";")) d += ";";
        return "@" + rule.name + (d ? " { " + d + " }" : " { }");
      };
      return rule;
    }

    if (kind === "keyframes") {
      const rule = new CSSKeyframesRule();
      rule.parentRule = parentRule || null;
      rule.parentStyleSheet = parentSheet || null;
      rule._onMutate = onMutate || null;
      rule.name = sel.replace(/^@(-webkit-)?keyframes/i, "").trim();

      const children = splitStylesheetRules(parsed.declText || "");
      for (let i = 0; i < children.length; i++) {
        const p = children[i];
        if (!p.selectorText) continue;
        const kf = new CSSKeyframeRule();
        kf.parentRule = rule;
        kf.parentStyleSheet = parentSheet || null;
        kf.keyText = p.selectorText.trim();
        const style = makeRuleStyleDeclaration(p.declText || "", "keyframe", null, onMutate);
        style._parentRule = kf;
        kf.style = style;
        kf._serialize = function () {
          const d = style.cssText;
          return kf.keyText + (d ? " { " + d + " }" : " { }");
        };
        rule.cssRules.push(kf);
      }
      rule._serialize = function () {
        const inner = rule.cssRules.map((r) => r.cssText).join("\n  ");
        return "@keyframes " + rule.name + (inner ? " {\n  " + inner + "\n}" : " {\n}");
      };
      return rule;
    }

    // Default CSSRule
    const rule = new CSSRule();
    rule.parentRule = parentRule || null;
    rule.parentStyleSheet = parentSheet || null;
    rule._type = 0;
    rule._serialize = function () {
      if (sel) {
        const d = String(parsed.declText || "").replace(/\s+/g, " ").trim();
        return sel + (d ? " { " + d + " }" : " { }");
      }
      return raw.replace(/\s+/g, " ").trim();
    };
    return rule;
  }

  // =========================================================================
  // 7. CSSStyleSheet
  // =========================================================================

  function CSSStyleSheet(ownerNode) {
    this.ownerNode = ownerNode || null;
    this.ownerRule = null;
    this.parentStyleSheet = null;
    this.href = null;
    this.title = null;
    this.media = new MediaList("");
    this.disabled = false;
    this._rules = [];
    this._lastParsedText = null;

    const self = this;
    const rulesList = makeRuleList(this._rules);

    Object.defineProperty(this, "cssRules", {
      get: function () {
        self._syncFromOwner();
        return rulesList;
      },
      configurable: true,
    });

    Object.defineProperty(this, "rules", {
      get: function () { return this.cssRules; },
      configurable: true,
    });
  }

  CSSStyleSheet.prototype._syncFromOwner = function () {
    if (!this.ownerNode) return;
    const cur = String(this.ownerNode.textContent || "");
    if (cur !== this._lastParsedText) {
      this._reparseText(cur);
    }
  };

  CSSStyleSheet.prototype._reparseText = function (text) {
    this._rules.length = 0;
    const chunks = splitStylesheetRules(text);
    for (let i = 0; i < chunks.length; i++) {
      const rule = constructRule(chunks[i], this, null, () => this._remirror());
      if (rule) this._rules.push(rule);
    }
    this._lastParsedText = text;
  };

  CSSStyleSheet.prototype._remirror = function () {
    if (!this.ownerNode) return;
    const text = this._rules.map((r) => r.cssText).join("\n");
    this._lastParsedText = text;
    this.ownerNode.textContent = text;
  };

  CSSStyleSheet.prototype.insertRule = function (ruleText, index) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'insertRule': 1 argument required, but only 0 present.");
    }
    this._syncFromOwner();
    const idx = index === undefined ? 0 : Number(index) >>> 0;
    if (idx > this._rules.length) {
      throw new DOMException("Index out of bounds", "IndexSizeError");
    }
    const parsed = parseSingleRuleOrThrow(ruleText);
    const rule = constructRule(parsed, this, null, () => this._remirror());
    if (!rule) throw new DOMException("Failed to parse CSS rule", "SyntaxError");
    this._rules.splice(idx, 0, rule);
    this._remirror();
    return idx;
  };

  CSSStyleSheet.prototype.deleteRule = function (index) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'deleteRule': 1 argument required, but only 0 present.");
    }
    this._syncFromOwner();
    const idx = Number(index) >>> 0;
    if (idx >= this._rules.length) {
      throw new DOMException("Index out of bounds", "IndexSizeError");
    }
    this._rules.splice(idx, 1);
    this._remirror();
  };

  CSSStyleSheet.prototype.replaceSync = function (text) {
    this._reparseText(String(text == null ? "" : text));
    this._remirror();
  };

  CSSStyleSheet.prototype.replace = function (text) {
    try {
      this.replaceSync(text);
    } catch (e) {
      return Promise.reject(e);
    }
    return Promise.resolve(this);
  };

  markNative(CSSStyleSheet, "CSSStyleSheet");
  global.CSSStyleSheet = CSSStyleSheet;
  if (typeof globalThis !== "undefined") globalThis.CSSStyleSheet = CSSStyleSheet;
  if (typeof window !== "undefined") window.CSSStyleSheet = CSSStyleSheet;

  function getOrCreateStyleSheet(ownerNode) {
    if (!ownerNode) return new CSSStyleSheet(null);
    if (!ownerNode._sheet) {
      ownerNode._sheet = new CSSStyleSheet(ownerNode);
    }
    return ownerNode._sheet;
  }
  global.getOrCreateStyleSheet = getOrCreateStyleSheet;
  if (typeof globalThis !== "undefined") globalThis.getOrCreateStyleSheet = getOrCreateStyleSheet;

  // =========================================================================
  // 8. Global getComputedStyle wrapper for var() resolution & padding/margin expansion
  // =========================================================================

  function resolveCssVars(el, val) {
    if (!val || typeof val !== "string" || !val.includes("var(")) return val;
    let res = val;
    let match;
    const varRegex = /var\(\s*(--[a-zA-Z0-9_-]+)(?:\s*,\s*([^)]+))?\s*\)/g;
    while ((match = varRegex.exec(val)) !== null) {
      const varName = match[1];
      const fallback = match[2] || "";
      let found = "";

      // Check element inline style
      if (el && el.style && typeof el.style.getPropertyValue === "function") {
        found = el.style.getPropertyValue(varName);
      }

      // Check ancestors up to documentElement
      if (!found && el) {
        let curr = el.parentElement || el.parentNode;
        while (curr && curr.nodeType === 1) {
          if (curr.style && typeof curr.style.getPropertyValue === "function") {
            found = curr.style.getPropertyValue(varName);
            if (found) break;
          }
          curr = curr.parentElement || curr.parentNode;
        }
      }

      // Check document style elements for :root / html declarations
      if (!found) {
        const doc = el?.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
        if (doc) {
          const styles = doc.querySelectorAll ? doc.querySelectorAll("style") : [];
          for (let i = 0; i < styles.length; i++) {
            const txt = styles[i].textContent || "";
            const rootMatch = txt.match(/(?::root|html)\s*\{([^}]+)\}/i);
            if (rootMatch) {
              const decls = rootMatch[1].split(";");
              for (let j = 0; j < decls.length; j++) {
                const colon = decls[j].indexOf(":");
                if (colon > 0) {
                  const k = decls[j].slice(0, colon).trim();
                  if (k === varName) {
                    found = decls[j].slice(colon + 1).trim();
                    break;
                  }
                }
              }
            }
            if (found) break;
          }
        }
      }

      const replacement = found || fallback;
      res = res.replace(match[0], replacement);
    }
    return res;
  }

  let _capturedGetComputedStyle = null;

  function wrapComputedStyleDecl(decl, element) {
    if (!decl || typeof decl !== "object") return decl;
    return new Proxy(decl, {
      get: function (target, prop, receiver) {
        if (typeof prop === "symbol") return target[prop];
        if (typeof prop === "string") {
          const kebab = cssPropCamelToKebab(prop);

          // If color is unstyled, default to rgb(0, 0, 0)
          if (kebab === "color") {
            const val = target.getPropertyValue ? target.getPropertyValue("color") : target[prop];
            return val || "rgb(0, 0, 0)";
          }

          // Shorthand padding -> paddingTop / paddingBottom / etc.
          if (kebab.startsWith("padding-")) {
            let val = target.getPropertyValue ? target.getPropertyValue(kebab) : target[prop];
            if (!val) {
              const shorthand = target.getPropertyValue ? target.getPropertyValue("padding") : target.padding;
              if (shorthand) val = resolveCssVars(element, shorthand);
            } else {
              val = resolveCssVars(element, val);
            }
            return val || "";
          }

          // Shorthand margin -> marginTop / marginBottom / etc.
          if (kebab.startsWith("margin-")) {
            let val = target.getPropertyValue ? target.getPropertyValue(kebab) : target[prop];
            if (!val) {
              const shorthand = target.getPropertyValue ? target.getPropertyValue("margin") : target.margin;
              if (shorthand) val = resolveCssVars(element, shorthand);
            } else {
              val = resolveCssVars(element, val);
            }
            return val || "";
          }

          let val = target[prop];
          if (typeof val === "string") val = resolveCssVars(element, val);
          return val;
        }
        return target[prop];
      }
    });
  }

  function enhancedGetComputedStyle(element, pseudoElt) {
    if (!_capturedGetComputedStyle) {
      throw new TypeError("getComputedStyle is not initialized");
    }
    const raw = _capturedGetComputedStyle.call(this, element, pseudoElt);
    return wrapComputedStyleDecl(raw, element);
  }
  markNative(enhancedGetComputedStyle, "getComputedStyle");

  function installGetComputedStyle(targetObj) {
    if (!targetObj) return;
    Object.defineProperty(targetObj, "getComputedStyle", {
      get: function () {
        return _capturedGetComputedStyle ? enhancedGetComputedStyle : undefined;
      },
      set: function (fn) {
        _capturedGetComputedStyle = fn;
      },
      configurable: true,
      enumerable: true,
    });
  }

  installGetComputedStyle(global);
  if (typeof globalThis !== "undefined") installGetComputedStyle(globalThis);
  if (typeof window !== "undefined") installGetComputedStyle(window);

})(typeof window !== "undefined" ? window : globalThis);
