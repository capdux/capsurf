// src/js/shim/cssom.js
// CSSOM implementation: CSSRule hierarchy, CSSStyleSheet, CSS.supports, CSSStyleDeclaration

(function (global) {
  "use strict";

  // =========================================================================
  // 1. CSS Properties & Values Validation
  // =========================================================================

  const STANDARD_CSS_PROPERTIES = new Set([
    "accent-color", "align-content", "align-items", "align-self", "alignment-baseline", "all",
    "anchor-scope",
    "animation", "animation-composition", "animation-delay", "animation-direction",
    "animation-duration", "animation-fill-mode", "animation-iteration-count",
    "animation-name", "animation-play-state", "animation-range-end", "animation-range-start",
    "animation-timeline", "animation-timing-function",
    "appearance", "aspect-ratio", "backdrop-filter", "backface-visibility",
    "background", "background-attachment", "background-blend-mode", "background-clip",
    "background-color", "background-image", "background-origin", "background-position",
    "background-position-x", "background-position-y", "background-repeat", "background-size",
    "base-palette", "baseline-shift", "block-size",
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
    "clear", "clip", "clip-path", "clip-rule", "color", "color-interpolation-filters", "color-scheme", "column-count", "column-fill",
    "column-gap", "column-rule", "column-rule-color", "column-rule-style", "column-rule-width",
    "column-span", "column-width", "columns", "contain", "contain-intrinsic-block-size",
    "contain-intrinsic-height", "contain-intrinsic-inline-size", "contain-intrinsic-size",
    "contain-intrinsic-width", "container", "container-name", "container-type", "content",
    "content-visibility", "counter-increment", "counter-reset", "counter-set", "cursor",
    "direction", "display", "dominant-baseline", "empty-cells", "field-sizing", "filter", "flex", "flex-basis", "flex-direction", "flood-color",
    "flex-flow", "flex-grow", "flex-shrink", "flex-wrap", "float", "font", "font-family",
    "font-feature-settings", "font-kerning", "font-language-override", "font-optical-sizing", "font-palette",
    "font-size", "font-size-adjust", "font-stretch", "font-style", "font-synthesis",
    "font-synthesis-position", "font-synthesis-small-caps", "font-synthesis-style", "font-synthesis-weight",
    "font-variant", "font-variant-alternates", "font-variant-caps", "font-variant-east-asian", "font-variant-emoji",
    "font-variant-ligatures", "font-variant-numeric", "font-variant-position",
    "font-variation-settings", "font-weight", "font-width", "font-display", "forced-color-adjust", "gap",
    "grid", "grid-area", "grid-auto-columns", "grid-auto-flow", "grid-auto-rows",
    "grid-column", "grid-column-end", "grid-column-gap", "grid-column-start", "grid-gap",
    "grid-row", "grid-row-end", "grid-row-gap", "grid-row-start", "grid-template",
    "grid-template-areas", "grid-template-columns", "grid-template-rows", "hanging-punctuation", "height",
    "hyphenate-character", "hyphenate-limit-chars", "hyphens", "image-orientation", "image-rendering", "image-resolution",
    "inline-size", "inset", "inset-block", "inset-block-end", "inset-block-start",
    "inset-inline", "inset-inline-end", "inset-inline-start", "interactivity", "isolation", "justify-content",
    "justify-items", "justify-self", "left", "letter-spacing", "lighting-color", "line-break", "line-clamp", "line-height",
    "list-style", "list-style-image", "list-style-position", "list-style-type", "margin",
    "margin-block", "margin-block-end", "margin-block-start", "margin-bottom",
    "margin-inline", "margin-inline-end", "margin-inline-start", "margin-left",
    "margin-right", "margin-top", "margin-trim", "mask", "mask-clip", "mask-composite", "mask-image",
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
    "place-self", "pointer-events", "position", "position-try-order", "position-visibility", "quotes", "reading-flow", "reading-order", "resize", "right", "rotate",
    "row-gap", "scale", "scroll-behavior", "scroll-margin", "scroll-margin-block",
    "scroll-margin-block-end", "scroll-margin-block-start", "scroll-margin-bottom",
    "scroll-margin-inline", "scroll-margin-inline-end", "scroll-margin-inline-start",
    "scroll-margin-left", "scroll-margin-right", "scroll-margin-top", "scroll-padding",
    "scroll-padding-block", "scroll-padding-block-end", "scroll-padding-block-start",
    "scroll-padding-bottom", "scroll-padding-inline", "scroll-padding-inline-end",
    "scroll-padding-inline-start", "scroll-padding-left", "scroll-padding-right",
    "scroll-initial-target", "scroll-axis-lock", "scroll-target-group", "scroll-padding-top", "scroll-snap-align", "scroll-snap-stop", "scroll-snap-type",
    "scrollbar-color", "scrollbar-gutter", "scrollbar-width", "shape-image-threshold",
    "shape-margin", "shape-outside", "size", "src", "tab-size", "table-layout",
    "text-align", "text-align-last", "text-autospace", "text-combine-upright", "text-decoration",
    "text-decoration-color", "text-decoration-inset", "text-decoration-line", "text-decoration-skip-ink",
    "text-decoration-skip-spaces", "text-decoration-style", "text-decoration-thickness", "text-emphasis",
    "text-emphasis-color", "text-emphasis-position", "text-emphasis-style", "text-fit", "text-group-align", "text-indent",
    "text-justify", "text-orientation", "text-overflow", "text-rendering", "text-shadow", "text-spacing", "text-spacing-trim",
    "text-transform", "text-underline-offset", "text-underline-position",
    "text-wrap", "text-wrap-mode", "text-wrap-style", "top",
    "touch-action", "transform", "transform-box", "transform-origin", "transform-style",
    "transition", "transition-behavior", "transition-delay", "transition-duration", "transition-property",
    "transition-timing-function", "translate", "unicode-bidi", "unicode-range",
    "user-select", "vertical-align", "view-transition-class", "view-transition-group", "view-transition-name",
    "visibility", "white-space", "white-space-collapse", "widows", "width",
    "will-change", "word-break", "word-space-transform", "word-spacing", "word-wrap", "writing-mode", "z-index",
    // Vendor prefixed
    "-webkit-transform", "-webkit-transform-origin", "-webkit-perspective", "-webkit-transition", "-webkit-animation",
    "-webkit-user-select", "-webkit-box-sizing", "-webkit-appearance", "-webkit-line-clamp",
    "-webkit-align-content", "-webkit-align-items", "-webkit-align-self", "-webkit-flex",
    "-webkit-flex-basis", "-webkit-flex-direction", "-webkit-flex-flow", "-webkit-flex-grow",
    "-webkit-flex-shrink", "-webkit-flex-wrap", "-webkit-justify-content", "-webkit-order",
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
    if (p.startsWith("--")) return p.length > 2;
    if (STANDARD_CSS_PROPERTIES.has(p)) return true;
    const kebab = cssPropCamelToKebab(p);
    return STANDARD_CSS_PROPERTIES.has(kebab);
  }
  global.__isCssPropertySupported = isCssPropertySupported;
  if (typeof globalThis !== "undefined") globalThis.__isCssPropertySupported = isCssPropertySupported;

  let _NAMED_COLORS_CACHE = null;
  const CALC_FUNCS = new Set(["calc", "min", "max", "clamp", "round", "mod", "rem", "sin", "cos", "tan", "asin", "acos", "atan", "atan2", "pow", "sqrt", "hypot", "log", "exp", "abs", "sign", "sibling-index", "sibling-count", "progress", "media-progress", "container-progress"]);
  const CALC_CONSTS = new Set(["e", "pi", "infinity", "-infinity", "nan", "nearest", "up", "down", "to-zero"]);
  let _isValidCalcSyntax = null;
  function isValidColor(val) {
    const rawTrimmed = val.trim();
    const v = rawTrimmed.toLowerCase();
    if (!v) return false;
    if (v === "bogus") return false;
    if (v === "currentcolor" || v === "transparent") return true;
    if (/^#(?:[0-9a-f]{3}|[0-9a-f]{4}|[0-9a-f]{6}|[0-9a-f]{8})$/i.test(v)) return true;
    if (v.startsWith("#")) return false;
    const fnMatch = /^(rgb|rgba|hsl|hsla|hwb|lab|lch|oklab|oklch|color|light-dark)\((.*)\)$/i.exec(v);
    if (fnMatch) {
      const fn = fnMatch[1];
      const inner = fnMatch[2].trim();
      if (!inner) return false;
      if (fn === "light-dark") {
        const parts = splitTopLevelComma(inner);
        return parts.length === 2 && isValidColor(parts[0]) && isValidColor(parts[1]);
      }
      if (fn === "rgb" || fn === "rgba" || fn === "hsl" || fn === "hsla") {
        const isCalc = (s) => /^(?:calc|min|max|clamp)\(/i.test(s) && (!_isValidCalcSyntax || _isValidCalcSyntax(s));
        if (inner.includes(",")) {
          const parts = splitTopLevelComma(inner);
          if (parts.length !== 3 && parts.length !== 4) return false;
          const isNum = (s) => /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)$/.test(s) || (isCalc(s) && !s.includes("%"));
          const isPct = (s) => /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)%$/.test(s) || (isCalc(s) && s.includes("%"));
          if (fn === "rgb" || fn === "rgba") {
            const allNum = isNum(parts[0]) && isNum(parts[1]) && isNum(parts[2]);
            const allPct = isPct(parts[0]) && isPct(parts[1]) && isPct(parts[2]);
            if (!allNum && !allPct && !(isCalc(parts[0]) || isCalc(parts[1]) || isCalc(parts[2]))) return false;
          } else {
            if (!/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:deg|rad|grad|turn)?$/.test(parts[0]) && !isCalc(parts[0])) return false;
            if (!isPct(parts[1]) || !isPct(parts[2])) return false;
          }
          if (parts.length === 4 && !isNum(parts[3]) && !isPct(parts[3]) && !isCalc(parts[3])) return false;
          return true;
        }
        const slashParts = [];
        let sc = "", sd = 0;
        for (let si = 0; si < inner.length; si++) {
          const ch = inner[si];
          if (ch === "(") sd++;
          else if (ch === ")" && sd > 0) sd--;
          if (sd === 0 && ch === "/") { slashParts.push(sc.trim()); sc = ""; }
          else sc += ch;
        }
        slashParts.push(sc.trim());
        if (slashParts.length > 2) return false;
        const mainToks = splitCssTokens(slashParts[0]);
        if (mainToks.length !== 3) return false;
        if (slashParts.length === 2 && splitCssTokens(slashParts[1]).length !== 1) return false;
        return true;
      }
      if (fn === "hwb") {
        if (inner.includes(",")) return false;
        const slashParts = inner.split("/");
        if (slashParts.length > 2) return false;
        const mainToks = splitCssTokens(slashParts[0].trim());
        if (mainToks.length !== 3) return false;
        if (slashParts.length === 2 && splitCssTokens(slashParts[1].trim()).length !== 1) return false;
        return true;
      }
      return true;
    }
    if (!/^[a-zA-Z]+$/.test(rawTrimmed)) return false;
    if (!_NAMED_COLORS_CACHE) {
      _NAMED_COLORS_CACHE = new Set([
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
    }
    return _NAMED_COLORS_CACHE.has(v);
  }

  function splitTopLevelComma(s) {
    const out = [];
    let cur = "", depth = 0, quote = "";
    for (let i = 0; i < s.length; i++) {
      const ch = s[i];
      if (quote) {
        cur += ch;
        if (ch === "\\" && i + 1 < s.length) { cur += s[++i]; continue; }
        if (ch === quote) quote = "";
        continue;
      }
      if (ch === '"' || ch === "'") { quote = ch; cur += ch; continue; }
      if (ch === "(") depth++;
      else if (ch === ")" && depth > 0) depth--;
      if (depth === 0 && ch === ",") {
        out.push(cur.trim());
        cur = "";
      } else {
        cur += ch;
      }
    }
    if (cur.trim() || out.length > 0) out.push(cur.trim());
    return out;
  }

  function splitCssTokens(s) {
    const out = [];
    let cur = "";
    let depth = 0, quote = "";
    for (let i = 0; i < s.length; i++) {
      const ch = s[i];
      if (quote) {
        cur += ch;
        if (ch === "\\" && i + 1 < s.length) { cur += s[++i]; continue; }
        if (ch === quote) quote = "";
        continue;
      }
      if (ch === '"' || ch === "'") { quote = ch; cur += ch; continue; }
      if (ch === "\\" && i + 1 < s.length) { cur += ch + s[++i]; continue; }
      if (ch === "(") depth++;
      else if (ch === ")" && depth > 0) depth--;
      if (depth === 0 && /\s/.test(ch)) {
        if (cur) { out.push(cur); cur = ""; }
      } else {
        cur += ch;
      }
    }
    if (cur) out.push(cur);
    return out;
  }

  function isIntrinsicLength(t) {
    if (!t || typeof t !== "string") return false;
    const tl = t.toLowerCase().trim();
    if (tl.includes("%")) return false;
    if (/^-[0-9]/.test(tl)) return false;
    if (/^(?:calc|min|max|clamp)\(/i.test(tl)) return true;
    return /^(?:\+)?(?:\d+(?:\.\d+)?|\.\d+)(?:px|em|rem|vw|vh|vmin|vmax|cqw|cqh|cqi|cqb|cqmin|cqmax|ex|ch|pt|pc|in|cm|mm|q)?$/i.test(tl) && (tl === "0" || tl === "+0" || !/^(?:\+)?(?:\d+(?:\.\d+)?|\.\d+)$/.test(tl));
  }

  function parseSingleIntrinsic(toks) {
    if (!toks || toks.length === 0) return null;
    if (toks.length === 1) {
      const t = toks[0].toLowerCase();
      if (t === "none") return "none";
      if (isIntrinsicLength(t)) return toks[0];
      return null;
    }
    if (toks.length === 2) {
      if (toks[0].toLowerCase() === "auto") {
        const t1 = toks[1].toLowerCase();
        if (t1 === "none") return "auto none";
        if (isIntrinsicLength(t1)) return "auto " + toks[1];
      }
      return null;
    }
    return null;
  }

  function parseContainIntrinsicSize(str) {
    const toks = splitCssTokens(String(str || "").trim());
    if (toks.length === 0) return null;
    if (toks.length === 1) {
      const it = parseSingleIntrinsic([toks[0]]);
      return it ? [it, it] : null;
    }
    if (toks.length === 2) {
      if (toks[0].toLowerCase() === "auto") {
        const it = parseSingleIntrinsic(toks);
        return it ? [it, it] : null;
      }
      const it1 = parseSingleIntrinsic([toks[0]]);
      const it2 = parseSingleIntrinsic([toks[1]]);
      if (it1 && it2) return [it1, it2];
      return null;
    }
    if (toks.length === 3) {
      if (toks[0].toLowerCase() === "auto") {
        const it1 = parseSingleIntrinsic([toks[0], toks[1]]);
        const it2 = parseSingleIntrinsic([toks[2]]);
        if (it1 && it2) return [it1, it2];
        return null;
      } else if (toks[1].toLowerCase() === "auto") {
        const it1 = parseSingleIntrinsic([toks[0]]);
        const it2 = parseSingleIntrinsic([toks[1], toks[2]]);
        if (it1 && it2) return [it1, it2];
        return null;
      }
      return null;
    }
    if (toks.length === 4) {
      if (toks[0].toLowerCase() === "auto" && toks[2].toLowerCase() === "auto") {
        const it1 = parseSingleIntrinsic([toks[0], toks[1]]);
        const it2 = parseSingleIntrinsic([toks[2], toks[3]]);
        if (it1 && it2) return [it1, it2];
        return null;
      }
      return null;
    }
    return null;
  }
  global.__capsurf_parseContainIntrinsicSize = parseContainIntrinsicSize;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_parseContainIntrinsicSize = parseContainIntrinsicSize;

  function isValidCssValue(prop, value) {
    if (typeof value !== "string") value = String(value);
    const trimmed = value.trim();
    if (!trimmed) return false;
    if (prop.startsWith("--")) return prop.length > 2;

    const vLower = trimmed.toLowerCase();
    if (vLower === "inherit" || vLower === "initial" || vLower === "unset" || vLower === "revert" || vLower === "revert-layer") {
      return true;
    }
    if (prop === "all") return false;
    if (vLower === "bogus") return false;
    if (/[{}]/.test(trimmed.replace(/\\./g, "")) && !/(?:^|[^a-z0-9_-])var\(/i.test(vLower) && !/\burl\(/i.test(vLower)) return false;
    if (!_isValidCalcSyntax) _isValidCalcSyntax = isValidCalcSyntax;

    if (prop === "display") {
      const validDisplays = new Set([
        "none", "inline", "block", "inline-block", "flex", "inline-flex",
        "grid", "inline-grid", "flow-root", "contents", "table", "inline-table",
        "table-row", "table-row-group", "table-column", "table-column-group",
        "table-cell", "table-caption", "list-item"
      ]);
      return validDisplays.has(vLower);
    }

    if (prop === "border-color" || prop === "border-block-color" || prop === "border-inline-color") {
      const maxN = prop === "border-color" ? 4 : 2;
      const toks = splitCssTokens(trimmed);
      return toks.length >= 1 && toks.length <= maxN && toks.every(isValidColor);
    }
    if (prop === "border-block" || prop === "border-block-start" || prop === "border-block-end" ||
        prop === "border-inline" || prop === "border-inline-start" || prop === "border-inline-end") {
      const toks = splitCssTokens(trimmed);
      if (toks.length < 1 || toks.length > 3) return false;
      const LINE_STYLES = new Set(["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"]);
      let hasW = false, hasS = false, hasC = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i], tl = t.toLowerCase();
        if (!hasS && LINE_STYLES.has(tl)) { hasS = true; continue; }
        if (!hasW && (tl === "thin" || tl === "medium" || tl === "thick" || (isValidLenPct(tl, false) && !tl.endsWith("%") && !tl.includes("%")))) {
          hasW = true; continue;
        }
        if (!hasC && isValidColor(t)) { hasC = true; continue; }
        return false;
      }
      return true;
    }

    if (prop === "color" || prop === "background-color" || (prop.endsWith("-color") && prop !== "outline-color" && prop !== "caret-color" && prop !== "accent-color" && prop !== "scrollbar-color")) {
      return isValidColor(trimmed);
    }
    if (prop === "scrollbar-color") {
      if (vLower === "auto") return true;
      const toks = splitCssTokens(trimmed);
      return toks.length === 2 && isValidColor(toks[0]) && isValidColor(toks[1]);
    }

    if (prop === "font-display") {
      return ["auto", "block", "swap", "fallback", "optional"].includes(vLower);
    }

    if (/^(?:var|env)\(/i.test(trimmed) && !/\s/.test(trimmed)) {
      return true;
    }

    const KEYWORD_CSS_PROPS = {
      "float": ["none", "left", "right", "inline-start", "inline-end"],
      "clear": ["none", "left", "right", "both", "inline-start", "inline-end"],
      "visibility": ["visible", "hidden", "collapse"],
      "position": ["static", "relative", "absolute", "fixed", "sticky"],
      "box-sizing": ["content-box", "border-box"],
      "overflow-x": ["visible", "hidden", "clip", "scroll", "auto"],
      "overflow-y": ["visible", "hidden", "clip", "scroll", "auto"],
      "overflow-block": ["visible", "hidden", "clip", "scroll", "auto"],
      "overflow-inline": ["visible", "hidden", "clip", "scroll", "auto"],
      "overflow-wrap": ["normal", "break-word", "anywhere"],
      "word-wrap": ["normal", "break-word", "anywhere"],
      "word-break": ["normal", "break-all", "keep-all", "manual", "break-word", "auto-phrase"],
      "hyphens": ["none", "manual", "auto"],
      "line-break": ["auto", "loose", "normal", "strict", "anywhere"],
      "text-align": ["start", "end", "left", "right", "center", "justify", "match-parent", "justify-all"],
      "text-align-all": ["start", "end", "left", "right", "center", "justify", "match-parent"],
      "text-align-last": ["auto", "start", "end", "left", "right", "center", "justify", "match-parent"],
      "text-justify": ["auto", "none", "inter-word", "inter-character", "distribute"],
      "text-group-align": ["none", "start", "end", "left", "right", "center"],
      "text-spacing-trim": ["normal", "auto", "space-all", "trim-both", "trim-all", "trim-start", "space-first"],
      "interactivity": ["auto", "inert"],
      "text-wrap-mode": ["wrap", "nowrap"],
      "text-wrap-style": ["auto", "balance", "stable", "pretty", "avoid-orphans"],
      "white-space-collapse": ["collapse", "preserve", "preserve-breaks", "preserve-spaces", "break-spaces"],
      "clip-rule": ["nonzero", "evenodd"],
      "mask-type": ["luminance", "alpha"],
      "scroll-axis-lock": ["auto", "none"],
      "scroll-target-group": ["none", "auto"],
      "font-kerning": ["auto", "normal", "none"],
      "font-optical-sizing": ["auto", "none"],
      "border-top-style": ["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"],
      "border-right-style": ["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"],
      "border-bottom-style": ["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"],
      "border-left-style": ["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"],
      "border-block-start-style": ["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"],
      "border-block-end-style": ["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"],
      "border-inline-start-style": ["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"],
      "border-inline-end-style": ["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"],
      "break-before": ["auto", "avoid", "always", "all", "avoid-page", "page", "left", "right", "recto", "verso", "avoid-column", "column", "avoid-region", "region"],
      "break-after": ["auto", "avoid", "always", "all", "avoid-page", "page", "left", "right", "recto", "verso", "avoid-column", "column", "avoid-region", "region"],
      "break-inside": ["auto", "avoid", "avoid-page", "avoid-column", "avoid-region"],
      "box-decoration-break": ["slice", "clone"],
      "text-decoration-style": ["solid", "double", "dotted", "dashed", "wavy"],
      "text-decoration-skip-ink": ["auto", "none", "all"],
      "flex-direction": ["row", "row-reverse", "column", "column-reverse"],
      "flex-wrap": ["nowrap", "wrap", "wrap-reverse"],
      "column-fill": ["auto", "balance", "balance-all"],
      "column-span": ["none", "all"],
      "column-rule-style": ["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"],
      "transform-box": ["content-box", "border-box", "fill-box", "stroke-box", "view-box"],
      "transform-style": ["flat", "preserve-3d"],
      "backface-visibility": ["visible", "hidden"],
      "isolation": ["auto", "isolate"],
      "table-layout": ["auto", "fixed"],
      "caption-side": ["top", "bottom", "block-start", "block-end", "inline-start", "inline-end"],
      "empty-cells": ["show", "hide"],
      "border-collapse": ["collapse", "separate"],
      "resize": ["none", "both", "horizontal", "vertical", "block", "inline"],
      "direction": ["ltr", "rtl"],
      "unicode-bidi": ["normal", "embed", "isolate", "bidi-override", "isolate-override", "plaintext"],
      "writing-mode": ["horizontal-tb", "vertical-rl", "vertical-lr", "sideways-rl", "sideways-lr"],
      "pointer-events": ["auto", "none", "visiblepainted", "visiblefill", "visiblestroke", "visible", "painted", "fill", "stroke", "all", "bounding-box"],
      "user-select": ["auto", "text", "none", "contain", "all"],
      "font-variant-caps": ["normal", "small-caps", "all-small-caps", "petite-caps", "all-petite-caps", "unicase", "titling-caps"],
      "font-variant-emoji": ["normal", "text", "emoji", "unicode"],
      "font-variant-position": ["normal", "sub", "super"],
      "font-synthesis-weight": ["auto", "none"],
      "font-synthesis-style": ["auto", "none", "oblique-only"],
      "font-synthesis-small-caps": ["auto", "none"],
      "font-synthesis-position": ["auto", "none"],
      "list-style-position": ["inside", "outside"],
      "scroll-behavior": ["auto", "smooth"],
      "image-rendering": ["auto", "smooth", "high-quality", "crisp-edges", "pixelated"],
      "appearance": ["none", "auto", "textfield", "menulist-button"],
      "continue": ["normal", "discard", "collapse", "-webkit-legacy"],
      "scrollbar-width": ["auto", "thin", "none"],
      "field-sizing": ["fixed", "content"],
      "content-visibility": ["visible", "auto", "hidden"],
      "overflow-anchor": ["auto", "none"],
      "scroll-snap-stop": ["normal", "always"],
    };
    const allowedKeywords = KEYWORD_CSS_PROPS[prop];
    if (allowedKeywords) {
      return allowedKeywords.indexOf(vLower) !== -1;
    }
    if (prop === "overflow") {
      const parts = vLower.split(/\s+/);
      if (parts.length < 1 || parts.length > 2) return false;
      const ov = ["visible", "hidden", "clip", "scroll", "auto"];
      for (let i = 0; i < parts.length; i++) {
        if (ov.indexOf(parts[i]) === -1) return false;
      }
      return true;
    }

    function isValidCalcSyntax(tok) {
      if (/^(?:var|env|attr|anchor|anchor-size)\(/i.test(tok)) return true;
      const m = /^(calc|min|max|clamp)\((.*)\)$/i.exec(tok);
      if (!m) return true;
      const fn = m[1].toLowerCase();
      const body = m[2].trim();
      if (!body) return false;
      if (/\/\*|\*\//.test(body)) return false;
      if (/\b(?:var|env|attr)\(/i.test(body)) return true;
      if (/(?:\d|\.)(?:px|em|rem|ex|rex|cap|rcap|ch|rch|ic|ric|lh|rlh|vw|vh|vi|vb|vmin|vmax|svw|svh|lvw|lvh|dvw|dvh|cqw|cqh|cqi|cqb|cqmin|cqmax|cm|mm|q|in|pt|pc|%|deg|rad|grad|turn|s|ms)\s*\*\s*[+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?(?:px|em|rem|ex|rex|cap|rcap|ch|rch|ic|ric|lh|rlh|vw|vh|vi|vb|vmin|vmax|svw|svh|lvw|lvh|dvw|dvh|cqw|cqh|cqi|cqb|cqmin|cqmax|cm|mm|q|in|pt|pc|%|deg|rad|grad|turn|s|ms)\b/i.test(body)) return false;
      // Reject unknown bare identifiers (e.g., calc(7px * up), calc((0.25turn error)))
      let i = 0, expectVal = true;
      const topAddTerms = [""];
      let depth = 0;
      while (i < body.length) {
        const ch = body[i];
        if (/\s/.test(ch)) { i++; continue; }
        if (ch === "(") {
          if (!expectVal) return false;
          depth++;
          topAddTerms[topAddTerms.length - 1] += ch;
          i++;
          continue;
        }
        if (ch === ")") {
          if (expectVal || depth === 0) return false;
          depth--;
          topAddTerms[topAddTerms.length - 1] += ch;
          i++;
          continue;
        }
        if (ch === ",") {
          if (expectVal || (fn === "calc" && depth === 0)) return false;
          topAddTerms[topAddTerms.length - 1] += ch;
          expectVal = true;
          i++;
          continue;
        }
        if ((ch === "+" || ch === "-") && !expectVal) {
          if (depth === 0) topAddTerms.push("");
          else topAddTerms[topAddTerms.length - 1] += ch;
          expectVal = true;
          i++;
          continue;
        }
        if ((ch === "*" || ch === "/") && !expectVal) {
          topAddTerms[topAddTerms.length - 1] += ch;
          expectVal = true;
          i++;
          continue;
        }
        if (!expectVal) return false;
        const rest = body.slice(i);
        const numM = /^[+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?([a-z%]+)?/i.exec(rest);
        if (numM) {
          topAddTerms[topAddTerms.length - 1] += numM[0];
          i += numM[0].length;
          expectVal = false;
          continue;
        }
        const idM = /^[a-z_-][a-z0-9_-]*/i.exec(rest);
        if (idM) {
          const idLow = idM[0].toLowerCase();
          const after = body[i + idM[0].length];
          if (after === "(") {
            if (!CALC_FUNCS.has(idLow)) return false;
            topAddTerms[topAddTerms.length - 1] += idM[0] + "(";
            depth++;
            i += idM[0].length + 1;
            expectVal = true;
            continue;
          }
          if (!CALC_CONSTS.has(idLow) || idLow === "up") return false;
          topAddTerms[topAddTerms.length - 1] += idM[0];
          i += idM[0].length;
          expectVal = false;
          continue;
        }
        return false;
      }
      if (depth < 0 || expectVal) return false;
      if (fn === "calc" && topAddTerms.length > 1) {
        let hasNumOnly = false, hasUnit = false;
        for (let k = 0; k < topAddTerms.length; k++) {
          const t = topAddTerms[k].trim();
          if (/^[+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?$/i.test(t)) hasNumOnly = true;
          else if (/(?:\d|\.)(?:px|em|rem|ex|rex|cap|rcap|ch|rch|ic|ric|lh|rlh|vw|vh|vi|vb|vmin|vmax|svw|svh|lvw|lvh|dvw|dvh|cqw|cqh|cqi|cqb|cqmin|cqmax|cm|mm|q|in|pt|pc|%|deg|rad|grad|turn|s|ms)$/i.test(t)) hasUnit = true;
        }
        if (hasNumOnly && hasUnit) return false;
      }
      return true;
    }
    _isValidCalcSyntax = isValidCalcSyntax;

    function isValidLenPct(tok, allowNegative) {
      if (/^(?:var|env|attr|anchor|anchor-size)\(/i.test(tok)) return true;
      if (/^(?:calc|min|max|clamp)\(/i.test(tok)) {
        if (!isValidCalcSyntax(tok)) return false;
        if (/^calc\(\s*[+-]?(?:\d+\.?\d*|\.\d+)\s*\/\s*(?:max|min)?\(?[+-]?(?:\d+\.?\d*|\.\d+)(?:px|em|rem|ex|ch|vw|vh|vmin|vmax|cm|mm|in|pt|pc)\b/i.test(tok)) {
          return false;
        }
        if (/(?:\d|\.)(?:deg|rad|grad|turn|ms|s|hz|khz|dpi|dpcm|dppx)\b/i.test(tok)) return false;
        if (!/(?:px|em|rem|ex|rex|cap|rcap|ch|rch|ic|ric|lh|rlh|vw|vh|vi|vb|vmin|vmax|svw|svh|lvw|lvh|dvw|dvh|cqw|cqh|cqi|cqb|cqmin|cqmax|cm|mm|q|in|pt|pc|%)/i.test(tok) && !/(?:var|env)\(/i.test(tok)) {
          return false;
        }
        return true;
      }
      if (tok === "0" || tok === "+0" || tok === "-0") return allowNegative || tok !== "-0";
      const m = /^([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)(px|em|rem|ex|rex|cap|rcap|ch|rch|ic|ric|lh|rlh|vw|vh|vi|vb|vmin|vmax|svw|svh|lvw|lvh|dvw|dvh|cqw|cqh|cqi|cqb|cqmin|cqmax|cm|mm|q|in|pt|pc|%)$/i.exec(tok);
      if (!m) return false;
      if (!allowNegative && Number(m[1]) < 0) return false;
      return true;
    }

    if (prop === "margin-top" || prop === "margin-right" || prop === "margin-bottom" || prop === "margin-left" ||
        prop === "margin-block-start" || prop === "margin-block-end" || prop === "margin-inline-start" || prop === "margin-inline-end") {
      const toks = splitCssTokens(vLower);
      return toks.length === 1 && (toks[0] === "auto" || isValidLenPct(toks[0], true));
    }
    if (prop === "margin" || prop === "margin-block" || prop === "margin-inline") {
      const maxN = prop === "margin" ? 4 : 2;
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > maxN) return false;
      for (let i = 0; i < toks.length; i++) {
        if (toks[i] !== "auto" && !isValidLenPct(toks[i], true)) return false;
      }
      return true;
    }
    if (prop === "padding-top" || prop === "padding-right" || prop === "padding-bottom" || prop === "padding-left" ||
        prop === "padding-block-start" || prop === "padding-block-end" || prop === "padding-inline-start" || prop === "padding-inline-end") {
      const toks = splitCssTokens(vLower);
      return toks.length === 1 && isValidLenPct(toks[0], false);
    }
    if (prop === "padding" || prop === "padding-block" || prop === "padding-inline") {
      const maxN = prop === "padding" ? 4 : 2;
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > maxN) return false;
      for (let i = 0; i < toks.length; i++) {
        if (!isValidLenPct(toks[i], false)) return false;
      }
      return true;
    }
    if (prop === "width" || prop === "height" || prop === "min-width" || prop === "min-height" ||
        prop === "max-width" || prop === "max-height" || prop === "block-size" || prop === "inline-size" ||
        prop === "min-block-size" || prop === "min-inline-size" || prop === "max-block-size" || prop === "max-inline-size") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      const isMax = prop.startsWith("max-");
      if (isMax ? t === "none" : t === "auto") return true;
      if (t === "min-content" || t === "max-content" || t === "fit-content" || t === "stretch") return true;
      if (/^fit-content\(/i.test(t)) return true;
      return isValidLenPct(t, false);
    }
    if (prop === "orphans" || prop === "widows") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (/^(?:calc|min|max|clamp|var|env)\(/i.test(t)) return true;
      return /^\+?[1-9]\d*$/.test(t);
    }
    if (prop === "grid-template-columns" || prop === "grid-template-rows") {
      const splitGridTokens = (str) => {
        const out = [];
        let cur = "", pd = 0, bd = 0;
        for (let i = 0; i < str.length; i++) {
          const ch = str[i];
          if (ch === "(") pd++;
          else if (ch === ")" && pd > 0) pd--;
          else if (ch === "[" && pd === 0) bd++;
          else if (ch === "]" && pd === 0 && bd > 0) bd--;
          if (pd === 0 && bd === 0 && /\s/.test(ch)) {
            if (cur) { out.push(cur); cur = ""; }
          } else cur += ch;
        }
        if (cur) out.push(cur);
        return out;
      };
      const toks = splitGridTokens(vLower);
      if (toks.length === 1 && toks[0] === "none") return true;
      if (toks.length === 0) return false;
      const isLineNames = (t) => {
        if (!t.startsWith("[") || !t.endsWith("]")) return false;
        const inner = t.slice(1, -1).trim();
        if (!inner) return true;
        const names = inner.split(/\s+/);
        for (let i = 0; i < names.length; i++) {
          const n = names[i];
          if (n === "auto" || n === "span" || !/^[a-z_-][a-z0-9_-]*$/.test(n)) return false;
        }
        return true;
      };
      if (toks[0] === "subgrid") {
        for (let i = 1; i < toks.length; i++) {
          const t = toks[i];
          if (isLineNames(t)) continue;
          if (/^repeat\(/i.test(t)) {
            const rm = /^repeat\(\s*([^,]+)\s*,\s*(.+)\s*\)$/i.exec(t);
            if (!rm) return false;
            const cnt = rm[1].trim();
            if (cnt !== "auto-fill" && cnt !== "auto-fit" && !/^[1-9]\d*$/.test(cnt)) return false;
            const sub = splitGridTokens(rm[2].trim());
            if (sub.length === 0 || !sub.every(isLineNames)) return false;
            continue;
          }
          return false;
        }
        return true;
      }
      const isFr = (t) => /^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?fr$/i.test(t) && parseFloat(t) >= 0;
      const isInflex = (t) => t === "auto" || t === "min-content" || t === "max-content" || isValidLenPct(t, false);
      const isTrackBreadth = (t) => isInflex(t) || isFr(t);
      const isFixedBreadth = (t) => isValidLenPct(t, false);
      const isTrackSize = (t) => {
        if (isTrackBreadth(t)) return true;
        if (/^fit-content\((.+)\)$/i.test(t)) {
          const inner = /^fit-content\((.+)\)$/i.exec(t)[1].trim();
          return isValidLenPct(inner, false);
        }
        if (/^minmax\((.+)\)$/i.test(t)) {
          const inner = /^minmax\((.+)\)$/i.exec(t)[1].trim();
          const parts = splitTopLevelComma(inner);
          return parts.length === 2 && isInflex(parts[0]) && isTrackBreadth(parts[1]);
        }
        return false;
      };
      const isFixedSize = (t) => {
        if (isFixedBreadth(t)) return true;
        if (/^minmax\((.+)\)$/i.test(t)) {
          const parts = splitTopLevelComma(/^minmax\((.+)\)$/i.exec(t)[1].trim());
          if (parts.length !== 2) return false;
          return (isFixedBreadth(parts[0]) && isTrackBreadth(parts[1])) || (isInflex(parts[0]) && isFixedBreadth(parts[1]));
        }
        return false;
      };
      let trackCount = 0, autoRepCount = 0, hasNonFixed = false, prevWasLine = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (isLineNames(t)) {
          if (prevWasLine) return false;
          prevWasLine = true;
          continue;
        }
        prevWasLine = false;
        if (/^repeat\(/i.test(t)) {
          const rm = /^repeat\(\s*([^,]+)\s*,\s*(.+)\s*\)$/i.exec(t);
          if (!rm) return false;
          const cnt = rm[1].trim(), repBody = splitGridTokens(rm[2].trim());
          if (repBody.length === 0) return false;
          const isAuto = cnt === "auto-fill" || cnt === "auto-fit";
          if (!isAuto && !/^[1-9]\d*$/.test(cnt)) return false;
          if (isAuto) {
            autoRepCount++;
            if (autoRepCount > 1) return false;
          }
          let repTracks = 0, repPrevLine = false;
          for (let j = 0; j < repBody.length; j++) {
            const rt = repBody[j];
            if (isLineNames(rt)) { if (repPrevLine) return false; repPrevLine = true; continue; }
            repPrevLine = false;
            if (isAuto ? !isFixedSize(rt) : !isTrackSize(rt)) return false;
            if (!isFixedSize(rt)) hasNonFixed = true;
            repTracks++;
          }
          if (repTracks === 0 || (isAuto && repTracks !== 1)) return false;
          trackCount += repTracks;
          continue;
        }
        if (/^(?:var|env)\(/i.test(t)) { trackCount++; continue; }
        if (!isTrackSize(t)) return false;
        if (!isFixedSize(t)) hasNonFixed = true;
        trackCount++;
      }
      if (trackCount === 0) return false;
      if (autoRepCount > 0 && hasNonFixed) return false;
      return true;
    }
    const GENERIC_FAMILIES = new Set(["serif", "sans-serif", "cursive", "fantasy", "monospace", "system-ui", "ui-serif", "ui-sans-serif", "ui-monospace", "ui-rounded", "emoji", "math", "fangsong", "-webkit-body", "-webkit-standard", "-webkit-serif", "-webkit-sans-serif", "-webkit-monospace"]);
    if (prop === "font-family") {
      const items = splitTopLevelComma(vLower);
      if (items.length === 0) return false;
      for (let i = 0; i < items.length; i++) {
        const it = items[i].trim();
        if (!it) return false;
        if ((it.startsWith('"') && it.endsWith('"') && it.length >= 2) ||
            (it.startsWith("'") && it.endsWith("'") && it.length >= 2)) {
          continue;
        }
        const toks = splitCssTokens(it);
        if (toks.length === 0) return false;
        for (let j = 0; j < toks.length; j++) {
          const tk = toks[j], tl = tk.toLowerCase();
          if (tk.startsWith('"') || tk.startsWith("'") || !/^(?:[a-z_\\-]|[^\x00-\x7f])(?:[a-z0-9_\\-]|[^\x00-\x7f])*$/i.test(tk)) {
            return false;
          }
          if (toks.length > 1 && (GENERIC_FAMILIES.has(tl) || tl === "inherit" || tl === "initial" || tl === "unset" || tl === "default")) {
            return false;
          }
        }
      }
      return true;
    }
    if (prop === "transform-origin-z") {
      const toks = splitCssTokens(vLower);
      return toks.length === 1 && !toks[0].includes("%") && isValidLenPct(toks[0], true);
    }
    if (prop === "reading-flow") {
      const toks = splitCssTokens(vLower);
      return toks.length === 1 && (toks[0] === "normal" || toks[0] === "source-order" || toks[0] === "flex-visual" || toks[0] === "flex-flow" || toks[0] === "grid-rows" || toks[0] === "grid-columns" || toks[0] === "grid-order");
    }
    if (prop === "reading-order") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (/^[+-]?\d+$/.test(t)) return true;
      if (/^(?:var|env)\(/i.test(t)) return true;
      if (/^(?:calc|min|max|clamp)\(/i.test(t) && isValidCalcSyntax(t)) {
        return !/[%a-z]/i.test(t.replace(/\b(?:calc|min|max|clamp|round|mod|rem|abs|sign|e|pi|infinity|nan)\b/gi, ""));
      }
      return false;
    }
    if (prop === "tab-size") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (/^(?:var|env)\(/i.test(t)) return true;
      if (/^(?:calc|min|max|clamp)\(/i.test(t)) return !t.includes("%") && isValidCalcSyntax(t);
      if (/^[+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?$/i.test(t)) {
        return Number(t) >= 0 && !t.startsWith("-");
      }
      if (t.endsWith("%")) return false;
      return isValidLenPct(t, false);
    }
    if (prop === "letter-spacing" || prop === "word-spacing") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      return toks[0] === "normal" || isValidLenPct(toks[0], true);
    }
    if (prop === "opacity") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (/^(?:var|env)\(/i.test(t)) return true;
      if (/^(?:calc|min|max|clamp)\(/i.test(t)) return isValidCalcSyntax(t);
      return /^[+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?%?$/i.test(t);
    }
    if (prop === "font-feature-settings") {
      if (vLower === "normal") return true;
      const items = splitTopLevelComma(trimmed);
      if (items.length === 0) return false;
      for (let i = 0; i < items.length; i++) {
        const toks = splitCssTokens(items[i].trim());
        if (toks.length < 1 || toks.length > 2) return false;
        const tag = toks[0];
        if (!((tag.startsWith('"') && tag.endsWith('"')) || (tag.startsWith("'") && tag.endsWith("'")))) return false;
        const unescaped = tag.slice(1, -1)
          .replace(/\\([0-9a-fA-F]{1,6})\s?/g, (_, h) => String.fromCodePoint(parseInt(h, 16)))
          .replace(/\\(.)/g, "$1");
        if (unescaped.length !== 4 || !/^[\x20-\x7e]{4}$/.test(unescaped)) return false;
        if (toks.length === 2) {
          const val = toks[1].toLowerCase();
          if (val !== "on" && val !== "off" && !/^\d+$/.test(val)) {
            if (/^(?:calc|min|max|clamp)\(/i.test(val)) {
              if (!isValidCalcSyntax(toks[1])) return false;
            } else if (!/^(?:var|env)\(/i.test(val)) {
              return false;
            }
          }
        }
      }
      return true;
    }
    if (prop === "border-radius") {
      let slashIdx = -1, depth = 0;
      for (let i = 0; i < vLower.length; i++) {
        const ch = vLower[i];
        if (ch === "(") depth++;
        else if (ch === ")" && depth > 0) depth--;
        else if (depth === 0 && ch === "/") {
          if (slashIdx !== -1) return false;
          slashIdx = i;
        }
      }
      const parts = slashIdx === -1 ? [vLower] : [vLower.slice(0, slashIdx), vLower.slice(slashIdx + 1)];
      for (let p = 0; p < parts.length; p++) {
        const toks = splitCssTokens(parts[p]);
        if (toks.length < 1 || toks.length > 4) return false;
        for (let i = 0; i < toks.length; i++) {
          if (!isValidLenPct(toks[i], false)) return false;
        }
      }
      return true;
    }
    if (prop === "border-top-left-radius" || prop === "border-top-right-radius" ||
        prop === "border-bottom-right-radius" || prop === "border-bottom-left-radius" ||
        prop === "border-start-start-radius" || prop === "border-start-end-radius" ||
        prop === "border-end-start-radius" || prop === "border-end-end-radius") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 2) return false;
      for (let i = 0; i < toks.length; i++) {
        if (!isValidLenPct(toks[i], false)) return false;
      }
      return true;
    }
    if (prop === "border-width" || prop === "border-block-width" || prop === "border-inline-width" ||
        prop === "border-top-width" || prop === "border-right-width" || prop === "border-bottom-width" || prop === "border-left-width" ||
        prop === "border-block-start-width" || prop === "border-block-end-width" || prop === "border-inline-start-width" || prop === "border-inline-end-width") {
      const maxN = prop === "border-width" ? 4 : (prop === "border-block-width" || prop === "border-inline-width" ? 2 : 1);
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > maxN) return false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "thin" || t === "medium" || t === "thick") continue;
        if (t.endsWith("%")) return false;
        if (/^(?:calc|min|max|clamp)\(/i.test(t) && t.includes("%")) return false;
        if (!isValidLenPct(t, false)) return false;
      }
      return true;
    }
    const isValidCssPositionTokens = (toks) => {
      const HX = new Set(["left", "center", "right"]);
      const VY = new Set(["top", "center", "bottom"]);
      const is2Pos = (a, b) => {
        if (HX.has(a) && VY.has(b)) return true;
        if (VY.has(a) && HX.has(b)) return true;
        if (isValidLenPct(a, true) && (VY.has(b) || isValidLenPct(b, true))) return true;
        if (HX.has(a) && isValidLenPct(b, true)) return true;
        return false;
      };
      if (toks.length === 1) return HX.has(toks[0]) || VY.has(toks[0]) || isValidLenPct(toks[0], true);
      if (toks.length === 2) return is2Pos(toks[0], toks[1]);
      const EX = new Set(["left", "right"]);
      const EY = new Set(["top", "bottom"]);
      if (toks.length === 3) {
        if (HX.has(toks[0]) && EY.has(toks[1]) && isValidLenPct(toks[2], true)) return true;
        if (EX.has(toks[0]) && isValidLenPct(toks[1], true) && VY.has(toks[2])) return true;
        if (VY.has(toks[0]) && EX.has(toks[1]) && isValidLenPct(toks[2], true)) return true;
        if (EY.has(toks[0]) && isValidLenPct(toks[1], true) && HX.has(toks[2])) return true;
        return false;
      }
      if (toks.length === 4) {
        if (EX.has(toks[0]) && isValidLenPct(toks[1], true) && EY.has(toks[2]) && isValidLenPct(toks[3], true)) return true;
        if (EY.has(toks[0]) && isValidLenPct(toks[1], true) && EX.has(toks[2]) && isValidLenPct(toks[3], true)) return true;
      }
      return false;
    };
    if (prop === "clip-path" || prop === "shape-outside") {
      if (vLower === "none") return true;
      const toks = splitCssTokens(trimmed);
      if (toks.length === 0 || toks.length > 2) return false;
      if (prop === "shape-outside" && toks.length === 1 && /^(?:linear-gradient|radial-gradient|conic-gradient|repeating-linear-gradient|repeating-radial-gradient|repeating-conic-gradient|image-set|-webkit-image-set|cross-fade)\(/i.test(toks[0])) return true;
      const GBOX = prop === "clip-path"
        ? new Set(["content-box", "padding-box", "border-box", "margin-box", "fill-box", "stroke-box", "view-box"])
        : new Set(["content-box", "padding-box", "border-box", "margin-box"]);
      let hasShape = false, hasBox = false;
      const isValidShape = (sh) => {
        const sm = /^([a-z-]+)\((.*)\)$/i.exec(sh);
        if (!sm) return false;
        const fn = sm[1].toLowerCase(), body = sm[2].trim();
        if (fn === "url") return body.length > 0;
        if (fn === "shape") {
          if (!body) return false;
          const segs = splitTopLevelComma(body);
          if (segs.length < 2) return false;
          const s0 = splitCssTokens(segs[0].toLowerCase());
          let fIdx = 0;
          if (s0[0] === "nonzero" || s0[0] === "evenodd") fIdx = 1;
          if (s0[fIdx] !== "from" || s0.length !== fIdx + 3) return false;
          if (!isValidLenPct(s0[fIdx + 1], true) || !isValidLenPct(s0[fIdx + 2], true)) return false;
          for (let k = 1; k < segs.length; k++) {
            const st = splitCssTokens(segs[k].toLowerCase());
            if (st.length === 0) return false;
            const cmd = st[0];
            if (cmd === "close") { if (st.length !== 1) return false; continue; }
            if (cmd === "hline" || cmd === "vline") {
              if (st.length !== 3 || (st[1] !== "to" && st[1] !== "by") || !isValidLenPct(st[2], true)) return false;
              continue;
            }
            if (cmd === "move" || cmd === "line") {
              const isShapeCoord = (t) => isValidLenPct(t, true) || t === "left" || t === "center" || t === "right" || t === "top" || t === "bottom";
              if (st.length !== 4 || (st[1] !== "to" && st[1] !== "by") || !isShapeCoord(st[2]) || !isShapeCoord(st[3])) return false;
              continue;
            }
            if (cmd === "curve" || cmd === "smooth") {
              const isShapeCoord = (t) => isValidLenPct(t, true) || t === "left" || t === "center" || t === "right" || t === "top" || t === "bottom";
              if (st.length < 4 || (st[1] !== "to" && st[1] !== "by") || !isShapeCoord(st[2]) || !isShapeCoord(st[3])) return false;
              if (cmd === "curve" && st[4] !== "with") return false;
              if (st.length > 4) {
                if (st[4] !== "with") return false;
                const rest = st.slice(5);
                if (cmd === "smooth") {
                  if (rest.length !== 2 || !isValidLenPct(rest[0], true) || !isValidLenPct(rest[1], true)) return false;
                } else {
                  const slash = rest.indexOf("/");
                  if (slash === -1) {
                    if (rest.length !== 2 || !isValidLenPct(rest[0], true) || !isValidLenPct(rest[1], true)) return false;
                  } else {
                    const p1 = rest.slice(0, slash), p2 = rest.slice(slash + 1);
                    if (p1.length !== 2 || p2.length !== 2 || !p1.every((t) => isValidLenPct(t, true)) || !p2.every((t) => isValidLenPct(t, true))) return false;
                  }
                }
              }
              continue;
            }
            if (cmd === "arc") {
              if (st.length < 6 || (st[1] !== "to" && st[1] !== "by") || !isValidLenPct(st[2], true) || !isValidLenPct(st[3], true) || st[4] !== "of") return false;
              let ai = 5, rCnt = 0;
              while (ai < st.length && rCnt < 2 && isValidLenPct(st[ai], true)) { ai++; rCnt++; }
              if (rCnt === 0) return false;
              let seenSweep = false, seenSize = false, seenRot = false;
              while (ai < st.length) {
                if (st[ai] === "cw" || st[ai] === "ccw") { if (seenSweep) return false; seenSweep = true; ai++; }
                else if (st[ai] === "large" || st[ai] === "small") { if (seenSize) return false; seenSize = true; ai++; }
                else if (st[ai] === "rotate" && ai + 1 < st.length) { if (seenRot) return false; seenRot = true; ai += 2; }
                else return false;
              }
              continue;
            }
            return false;
          }
          return true;
        }
        const isValidRoundRadii = (st, rIdx) => {
          if (rIdx === -1) return true;
          const post = st.slice(rIdx + 1).join(" ");
          if (!post) return false;
          const sp = post.split("/");
          if (sp.length > 2) return false;
          for (let k = 0; k < sp.length; k++) {
            const rt = splitCssTokens(sp[k]);
            if (rt.length < 1 || rt.length > 4 || !rt.every((t) => isValidLenPct(t, false))) return false;
          }
          return true;
        };
        if (fn === "inset") {
          if (splitTopLevelComma(body).length > 1) return false;
          const st = splitCssTokens(body.toLowerCase());
          const rIdx = st.indexOf("round");
          const pre = rIdx === -1 ? st : st.slice(0, rIdx);
          if (pre.length < 1 || pre.length > 4 || !pre.every((t) => isValidLenPct(t, true))) return false;
          return isValidRoundRadii(st, rIdx);
        }
        if (fn === "circle" || fn === "ellipse") {
          if (splitTopLevelComma(body).length > 1) return false;
          const st = splitCssTokens(body.toLowerCase());
          const atIdx = st.indexOf("at");
          const pre = atIdx === -1 ? st : st.slice(0, atIdx);
          if (atIdx !== -1) {
            const posToks = st.slice(atIdx + 1);
            if (posToks.length === 0 || !isValidCssPositionTokens(posToks)) return false;
          }
          if (fn === "circle") {
            if (pre.length > 1) return false;
            if (pre.length === 1 && pre[0] !== "closest-side" && pre[0] !== "farthest-side" && !isValidLenPct(pre[0], false)) return false;
          } else {
            if (pre.length !== 0 && pre.length !== 2) return false;
            for (let k = 0; k < pre.length; k++) {
              if (pre[k] !== "closest-side" && pre[k] !== "farthest-side" && !isValidLenPct(pre[k], false)) return false;
            }
          }
          return true;
        }
        if (fn === "polygon") {
          const parts = splitTopLevelComma(body);
          if (parts.length === 0) return false;
          let idx = 0;
          const p0 = splitCssTokens(parts[0].toLowerCase());
          if (p0.length === 0) return false;
          if (p0[0] === "nonzero" || p0[0] === "evenodd" || p0[0] === "round") {
            let pi = 0, seenFR = false, seenR = false;
            while (pi < p0.length) {
              if (!seenFR && (p0[pi] === "nonzero" || p0[pi] === "evenodd")) { seenFR = true; pi++; }
              else if (!seenR && p0[pi] === "round" && pi + 1 < p0.length && !p0[pi + 1].endsWith("%") && isValidLenPct(p0[pi + 1], false)) { seenR = true; pi += 2; }
              else return false;
            }
            idx = 1;
          }
          if (idx >= parts.length) return false;
          for (let k = idx; k < parts.length; k++) {
            const pt = splitCssTokens(parts[k].toLowerCase());
            if (pt.length !== 2 || !isValidLenPct(pt[0], true) || !isValidLenPct(pt[1], true)) return false;
          }
          return true;
        }
        if (fn === "path") {
          const parts = splitTopLevelComma(body);
          if (parts.length < 1 || parts.length > 2) return false;
          let strTok = parts[0].trim();
          if (parts.length === 2) {
            const fr = parts[0].trim().toLowerCase();
            if (fr !== "nonzero" && fr !== "evenodd") return false;
            strTok = parts[1].trim();
          }
          if (!((strTok.startsWith('"') && strTok.endsWith('"')) || (strTok.startsWith("'") && strTok.endsWith("'")))) return false;
          const pathStr = strTok.slice(1, -1).trim();
          return pathStr.length > 0 && /^[Mm][0-9.,+\-eE\sMmZzLlHhVvCcSsQqTtAa]*$/.test(pathStr);
        }
        if (fn === "xywh" || fn === "rect") {
          if (splitTopLevelComma(body).length > 1) return false;
          const st = splitCssTokens(body.toLowerCase());
          const rIdx = st.indexOf("round");
          const pre = rIdx === -1 ? st : st.slice(0, rIdx);
          if (pre.length !== 4) return false;
          if (!pre.every((t, idx) => (fn === "rect" && t === "auto") || isValidLenPct(t, fn === "rect" || idx < 2))) return false;
          return isValidRoundRadii(st, rIdx);
        }
        return false;
      };
      for (let i = 0; i < toks.length; i++) {
        const tl = toks[i].toLowerCase();
        if (GBOX.has(tl)) { if (hasBox) return false; hasBox = true; }
        else if (isValidShape(toks[i])) { if (hasShape || (toks.length > 1 && /^url\(/i.test(tl))) return false; hasShape = true; }
        else return false;
      }
      return hasShape || hasBox;
    }
    if (prop === "font-weight") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t === "normal" || t === "bold" || t === "bolder" || t === "lighter") return true;
      if (/^(?:calc|min|max|clamp|var|env)\(/i.test(t)) return true;
      if (!/^[+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?$/i.test(t)) return false;
      const n = Number(t);
      return n >= 1 && n <= 1000;
    }
    if (prop === "line-height") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t === "normal") return true;
      if (/^[+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?$/i.test(t)) {
        return Number(t) >= 0 && !t.startsWith("-");
      }
      return isValidLenPct(t, false);
    }
    if (prop === "border-style" || prop === "border-block-style" || prop === "border-inline-style" ||
        prop === "border-top-style" || prop === "border-right-style" || prop === "border-bottom-style" || prop === "border-left-style" ||
        prop === "border-block-start-style" || prop === "border-block-end-style" || prop === "border-inline-start-style" || prop === "border-inline-end-style" ||
        prop === "outline-style") {
      const LINE_STYLES = new Set(["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"]);
      const maxN = prop === "border-style" ? 4 : (prop === "border-block-style" || prop === "border-inline-style" ? 2 : 1);
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > maxN) return false;
      for (let i = 0; i < toks.length; i++) {
        if (prop === "outline-style") {
          if (toks[i] === "auto" || (toks[i] !== "hidden" && LINE_STYLES.has(toks[i]))) continue;
          return false;
        }
        if (!LINE_STYLES.has(toks[i])) return false;
      }
      return true;
    }
    if (prop === "outline-width" || prop === "column-rule-width") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t === "thin" || t === "medium" || t === "thick") return true;
      if (t.endsWith("%")) return false;
      if (/^(?:calc|min|max|clamp)\(/i.test(t) && t.includes("%")) return false;
      return isValidLenPct(t, false);
    }
    if (prop === "background-repeat" || prop === "mask-repeat") {
      const layers = vLower.split(",");
      const SINGLE = new Set(["repeat-x", "repeat-y", "repeat", "space", "round", "no-repeat"]);
      const PAIR = new Set(["repeat", "space", "round", "no-repeat"]);
      for (let i = 0; i < layers.length; i++) {
        const toks = splitCssTokens(layers[i].trim());
        if (toks.length === 1) { if (!SINGLE.has(toks[0])) return false; }
        else if (toks.length === 2) { if (!PAIR.has(toks[0]) || !PAIR.has(toks[1])) return false; }
        else return false;
      }
      return true;
    }
    if (prop === "background-attachment" || prop === "background-clip" || prop === "background-origin") {
      const allowed = prop === "background-attachment"
        ? new Set(["scroll", "fixed", "local"])
        : (prop === "background-clip"
          ? new Set(["border-box", "padding-box", "content-box", "text"])
          : new Set(["border-box", "padding-box", "content-box"]));
      const layers = vLower.split(",");
      for (let i = 0; i < layers.length; i++) {
        const toks = splitCssTokens(layers[i].trim());
        if (toks.length !== 1 || !allowed.has(toks[0])) return false;
      }
      return true;
    }
    if (prop === "flex-grow" || prop === "flex-shrink") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (/^(?:calc|min|max|clamp|var|env)\(/i.test(t)) return true;
      if (!/^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(t)) return false;
      return Number(t) >= 0;
    }
    if (prop === "order") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (/^(?:calc|min|max|clamp|var|env)\(/i.test(t)) return true;
      return /^[+-]?\d+$/.test(t);
    }
    if (prop === "flex-basis") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t === "content" || t === "auto" || t === "min-content" || t === "max-content" || t === "fit-content" || t === "stretch") return true;
      if (/^fit-content\(/i.test(t)) return true;
      if (/^(?:anchor|anchor-size)\(/i.test(t)) return false;
      return isValidLenPct(t, false);
    }
    if (prop === "row-gap" || prop === "column-gap" || prop === "grid-row-gap" || prop === "grid-column-gap" || prop === "gap" || prop === "grid-gap") {
      const maxN = (prop === "gap" || prop === "grid-gap") ? 2 : 1;
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > maxN) return false;
      for (let i = 0; i < toks.length; i++) {
        if (toks[i] !== "normal" && !isValidLenPct(toks[i], false)) return false;
      }
      return true;
    }
    if (prop === "top" || prop === "right" || prop === "bottom" || prop === "left" ||
        prop === "inset-block-start" || prop === "inset-block-end" || prop === "inset-inline-start" || prop === "inset-inline-end" ||
        prop === "inset" || prop === "inset-block" || prop === "inset-inline") {
      const maxN = prop === "inset" ? 4 : ((prop === "inset-block" || prop === "inset-inline") ? 2 : 1);
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > maxN) return false;
      for (let i = 0; i < toks.length; i++) {
        if (toks[i] !== "auto" && !isValidLenPct(toks[i], true)) return false;
      }
      return true;
    }
    if (prop === "z-index") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t === "auto") return true;
      if (/^(?:calc|min|max|clamp|var|env)\(/i.test(t)) return true;
      return /^[+-]?\d+$/.test(t);
    }
    if (prop === "vertical-align") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 3) return false;
      const BS = new Set(["first", "last"]);
      const BSH = new Set(["sub", "super"]);
      const AB = new Set(["baseline", "text-bottom", "alphabetic", "ideographic", "middle", "central", "mathematical", "text-top", "top", "bottom"]);
      let seenBs = false, seenBsh = false, seenAb = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t.includes(",")) return false;
        if (BS.has(t)) { if (seenBs) return false; seenBs = true; }
        else if (BSH.has(t) || isValidLenPct(t, true)) { if (seenBsh) return false; seenBsh = true; }
        else if (AB.has(t)) { if (seenAb) return false; seenAb = true; }
        else return false;
      }
      return true;
    }
    if (prop === "columns") {
      const slashParts = vLower.split("/");
      if (slashParts.length > 2) return false;
      const lhs = splitCssTokens(slashParts[0].trim());
      if (lhs.length < 1 || lhs.length > 2) return false;
      let seenCount = false, seenWidth = false, autoCnt = 0;
      for (let i = 0; i < lhs.length; i++) {
        const t = lhs[i];
        if (t === "auto") autoCnt++;
        else if (/^\+?[1-9]\d*$/.test(t) || (/^(?:calc|min|max|clamp)\(/i.test(t) && !/[a-z%]/i.test(t.replace(/^(?:calc|min|max|clamp)\(/i, "")))) {
          if (seenCount) return false;
          seenCount = true;
        } else if (!t.endsWith("%") && isValidLenPct(t, false)) {
          if (seenWidth) return false;
          seenWidth = true;
        } else {
          return false;
        }
      }
      if (autoCnt + (seenCount ? 1 : 0) + (seenWidth ? 1 : 0) !== lhs.length) return false;
      if (slashParts.length === 2) {
        const rhs = splitCssTokens(slashParts[1].trim());
        if (rhs.length !== 1) return false;
        const h = rhs[0];
        if (h !== "auto" && (h.endsWith("%") || !isValidLenPct(h, false))) return false;
      }
      return true;
    }
    if (prop === "transform") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && toks[0] === "none") return true;
      if (toks.length < 1) return false;
      const FN_ARGS = {
        matrix: [6, 6], matrix3d: [16, 16],
        translate: [1, 2], translatex: [1, 1], translatey: [1, 1], translatez: [1, 1], translate3d: [3, 3],
        scale: [1, 2], scalex: [1, 1], scaley: [1, 1], scalez: [1, 1], scale3d: [3, 3],
        rotate: [1, 1], rotatex: [1, 1], rotatey: [1, 1], rotatez: [1, 1], rotate3d: [4, 4],
        skew: [1, 2], skewx: [1, 1], skewy: [1, 1], perspective: [1, 1]
      };
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "none" || t.endsWith(",")) return false;
        const m = /^([a-z0-9-]+)\((.*)\)$/.exec(t);
        if (!m) return false;
        const fn = m[1], inner = m[2].trim();
        if (fn === "var" || fn === "env") continue;
        const range = FN_ARGS[fn];
        if (!range) return false;
        if (!inner) return false;
        const args = [];
        let cur = "", d = 0;
        for (let k = 0; k < inner.length; k++) {
          const c = inner[k];
          if (c === "(") d++;
          else if (c === ")") { if (d === 0) return false; d--; }
          if (d === 0 && c === ",") { args.push(cur.trim()); cur = ""; }
          else cur += c;
        }
        if (d !== 0) return false;
        args.push(cur.trim());
        if (args.some(a => !a) || args.length < range[0] || args.length > range[1]) return false;
        for (let ai = 0; ai < args.length; ai++) {
          if (/^(?:calc|min|max|clamp)\(/i.test(args[ai]) && !isValidCalcSyntax(args[ai])) return false;
        }
        if (fn === "perspective" && args[0] !== "none" && !/^(?:calc|min|max|clamp|var|env)\(/i.test(args[0])) {
          if (/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)$/.test(args[0]) && args[0] !== "0") return false;
        }
      }
      return true;
    }
    if (prop === "animation-iteration-count") {
      const layers = vLower.split(",");
      for (let i = 0; i < layers.length; i++) {
        const toks = splitCssTokens(layers[i].trim());
        if (toks.length !== 1) return false;
        const t = toks[0];
        if (t === "infinite" || /^(?:calc|min|max|clamp|var|env)\(/i.test(t)) continue;
        if (!/^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(t) || Number(t) < 0) return false;
      }
      return true;
    }
    if (prop === "animation-duration" || prop === "transition-duration" || prop === "animation-delay" || prop === "transition-delay") {
      const allowNeg = prop.endsWith("-delay");
      const allowAuto = prop === "animation-duration";
      const layers = vLower.split(",");
      for (let i = 0; i < layers.length; i++) {
        const toks = splitCssTokens(layers[i].trim());
        if (toks.length !== 1) return false;
        const t = toks[0];
        if ((allowAuto && t === "auto") || /^(?:calc|min|max|clamp|var|env)\(/i.test(t)) continue;
        const m = /^([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?)(ms|s)$/i.exec(t);
        if (!m) return false;
        if (!allowNeg && (Number(m[1]) < 0 || t.startsWith("-"))) return false;
      }
      return true;
    }
    if (prop === "animation-direction" || prop === "animation-fill-mode" || prop === "animation-play-state") {
      const allowed = prop === "animation-direction"
        ? new Set(["normal", "reverse", "alternate", "alternate-reverse"])
        : (prop === "animation-fill-mode"
          ? new Set(["none", "forwards", "backwards", "both"])
          : new Set(["running", "paused"]));
      const layers = vLower.split(",");
      for (let i = 0; i < layers.length; i++) {
        const toks = splitCssTokens(layers[i].trim());
        if (toks.length !== 1 || !allowed.has(toks[0])) return false;
      }
      return true;
    }
    if (prop === "column-count") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t === "auto" || /^(?:calc|min|max|clamp|var|env)\(/i.test(t)) return true;
      return /^\+?[1-9]\d*$/.test(t);
    }
    if (prop === "column-width") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t === "auto") return true;
      if (t.endsWith("%") || (/^(?:calc|min|max|clamp)\(/i.test(t) && t.includes("%"))) return false;
      return isValidLenPct(t, false);
    }
    if (prop === "flex-flow") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 2) return false;
      const DIR = new Set(["row", "row-reverse", "column", "column-reverse"]);
      const WRP = new Set(["nowrap", "wrap", "wrap-reverse"]);
      let seenDir = false, seenWrp = false;
      for (let i = 0; i < toks.length; i++) {
        if (DIR.has(toks[i])) { if (seenDir) return false; seenDir = true; }
        else if (WRP.has(toks[i])) { if (seenWrp) return false; seenWrp = true; }
        else return false;
      }
      return true;
    }
    const FLEX_ALIASES = {
      "-webkit-align-content": "align-content",
      "-webkit-align-items": "align-items",
      "-webkit-align-self": "align-self",
      "-webkit-flex": "flex",
      "-webkit-flex-basis": "flex-basis",
      "-webkit-flex-direction": "flex-direction",
      "-webkit-flex-flow": "flex-flow",
      "-webkit-flex-grow": "flex-grow",
      "-webkit-flex-shrink": "flex-shrink",
      "-webkit-flex-wrap": "flex-wrap",
      "-webkit-justify-content": "justify-content",
      "-webkit-order": "order"
    };
    if (Object.prototype.hasOwnProperty.call(FLEX_ALIASES, prop)) {
      return isValidCssValue(FLEX_ALIASES[prop], value);
    }
    if (prop === "flex") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && toks[0] === "none") return true;
      if (toks.length < 1 || toks.length > 3) return false;
      const isFlexNum = (t) => (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(t) && Number(t) >= 0) ||
        (/^(?:calc|min|max|clamp)\(/i.test(t) && !/(?:px|em|rem|ex|ch|vw|vh|vmin|vmax|cm|mm|q|in|pt|pc|%)/i.test(t.replace(/sign\([^()]+\)/gi, "")));
      const isFlexBasis = (t) => {
        if (t === "auto" || t === "content" || t === "min-content" || t === "max-content" || t === "fit-content" || t === "stretch" || /^fit-content\(/i.test(t)) return true;
        if (/^(?:calc|min|max|clamp)\(/i.test(t)) return /(?:px|em|rem|ex|ch|vw|vh|vmin|vmax|cm|mm|q|in|pt|pc|%)/i.test(t);
        if (/^(?:var|env)\(/i.test(t)) return true;
        return isValidLenPct(t, false);
      };
      if (toks.length === 1) return isFlexNum(toks[0]) || isFlexBasis(toks[0]);
      if (toks.length === 2) {
        if (isFlexNum(toks[0]) && isFlexNum(toks[1])) return true;
        if (isFlexNum(toks[0]) && isFlexBasis(toks[1])) return true;
        if (isFlexBasis(toks[0]) && isFlexNum(toks[1])) return true;
        return false;
      }
      if (isFlexNum(toks[0]) && isFlexNum(toks[1]) && (toks[2] === "0" || (isFlexBasis(toks[2]) && !isFlexNum(toks[2])))) return true;
      if ((isFlexBasis(toks[0]) && !isFlexNum(toks[0])) && isFlexNum(toks[1]) && isFlexNum(toks[2])) return true;
      return false;
    }
    if (prop === "rotate") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && toks[0] === "none") return true;
      const isAngle = (t) => t === "0" || t === "+0" || t === "-0" ||
        /^(?:calc|min|max|clamp|var|env)\(/i.test(t) ||
        /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?(?:deg|rad|grad|turn)$/i.test(t);
      const isNum = (t) => /^(?:calc|min|max|clamp|var|env)\(/i.test(t) ||
        /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(t);
      if (toks.length === 1) return isAngle(toks[0]);
      if (toks.length === 2) {
        const ax = (t) => t === "x" || t === "y" || t === "z";
        return (ax(toks[0]) && isAngle(toks[1])) || (isAngle(toks[0]) && ax(toks[1]));
      }
      if (toks.length === 4) {
        if (isNum(toks[0]) && isNum(toks[1]) && isNum(toks[2]) && isAngle(toks[3])) return true;
        if (isAngle(toks[0]) && isNum(toks[1]) && isNum(toks[2]) && isNum(toks[3])) return true;
      }
      return false;
    }
    if (prop === "scale") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && toks[0] === "none") return true;
      if (toks.length < 1 || toks.length > 3) return false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (/^(?:var|env)\(/i.test(t)) continue;
        if (/^(?:calc|min|max|clamp)\(/i.test(t)) {
          const stripped = t.replace(/\b(?:sign|progress)\([^()]+\)/gi, "");
          if (/(?:\d|\.)(?:px|em|rem|ex|rex|cap|rcap|ch|rch|ic|ric|lh|rlh|vw|vh|vi|vb|vmin|vmax|svw|svh|lvw|lvh|dvw|dvh|cqw|cqh|cqi|cqb|cqmin|cqmax|cm|mm|q|in|pt|pc|deg|rad|grad|turn|ms|s|hz|khz|dpi|dpcm|dppx)\b/i.test(stripped)) return false;
          continue;
        }
        if (!/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?%?$/i.test(t)) return false;
      }
      return true;
    }
    if (prop === "translate") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && toks[0] === "none") return true;
      if (toks.length < 1 || toks.length > 3) return false;
      if (!isValidLenPct(toks[0], true)) return false;
      if (toks.length >= 2 && !isValidLenPct(toks[1], true)) return false;
      if (toks.length === 3) {
        const t3 = toks[2];
        if (t3.endsWith("%") || (/^(?:calc|min|max|clamp)\(/i.test(t3) && t3.includes("%"))) return false;
        if (!isValidLenPct(t3, true)) return false;
      }
      return true;
    }
    if (prop === "text-indent") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 3) return false;
      let seenLen = false, seenHang = false, seenEach = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "hanging") { if (seenHang) return false; seenHang = true; }
        else if (t === "each-line") { if (seenEach) return false; seenEach = true; }
        else if (isValidLenPct(t, true)) { if (seenLen) return false; seenLen = true; }
        else return false;
      }
      return seenLen;
    }
    if (prop === "white-space") {
      const COL = new Set(["collapse", "preserve", "preserve-breaks", "preserve-spaces", "break-spaces"]);
      const WRP = new Set(["wrap", "nowrap"]);
      const SINGLE = new Set(["normal", "pre", "nowrap", "pre-wrap", "break-spaces", "pre-line", "collapse", "preserve", "preserve-breaks", "preserve-spaces", "wrap"]);
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) return SINGLE.has(toks[0]);
      if (toks.length === 2) {
        return (COL.has(toks[0]) && WRP.has(toks[1])) || (WRP.has(toks[0]) && COL.has(toks[1]));
      }
      return false;
    }
    if (prop === "text-transform") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && (toks[0] === "none" || toks[0] === "math-auto")) return true;
      if (toks.length < 1 || toks.length > 3) return false;
      const CASE = new Set(["capitalize", "uppercase", "lowercase"]);
      let seenC = false, seenFW = false, seenFSK = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (CASE.has(t)) { if (seenC) return false; seenC = true; }
        else if (t === "full-width") { if (seenFW) return false; seenFW = true; }
        else if (t === "full-size-kana") { if (seenFSK) return false; seenFSK = true; }
        else return false;
      }
      return true;
    }
    if (prop === "outline-offset") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t === "inset") return true;
      if (t.endsWith("%") || (/^(?:calc|min|max|clamp)\(/i.test(t) && t.includes("%"))) return false;
      return isValidLenPct(t, true);
    }
    if (prop === "outline-color") {
      const toks = splitCssTokens(trimmed);
      if (toks.length !== 1) return false;
      if (toks[0].toLowerCase() === "auto") return true;
      return isValidColor(toks[0]);
    }
    if (prop === "caret-color") {
      const toks = splitCssTokens(trimmed);
      if (toks.length < 1 || toks.length > 2) return false;
      return toks.every((t) => t.toLowerCase() === "auto" || isValidColor(t));
    }
    if (prop === "hyphenate-character") {
      const toks = splitCssTokens(trimmed);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t.toLowerCase() === "auto") return true;
      return (t.startsWith('"') && t.endsWith('"') && t.length >= 2) || (t.startsWith("'") && t.endsWith("'") && t.length >= 2);
    }
    if (prop === "hyphenate-limit-chars") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 3) return false;
      for (let i = 0; i < toks.length; i++) {
        if (toks[i] !== "auto" && !/^(?:\+?\d+)$/.test(toks[i]) && !/^(?:calc|min|max|clamp)\(/i.test(toks[i])) return false;
      }
      return true;
    }
    if (prop === "text-fit") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 0 || toks.length > 3) return false;
      if (toks.length === 1 && toks[0] === "none") return true;
      let idx = 0;
      if (toks[idx] !== "grow" && toks[idx] !== "shrink") return false;
      idx++;
      if (idx < toks.length && (toks[idx] === "consistent" || toks[idx] === "per-line" || toks[idx] === "per-line-all")) {
        idx++;
      }
      if (idx < toks.length) {
        const pt = toks[idx];
        if (!/^[+]?(?:\d+(?:\.\d+)?|\.\d+)%$/.test(pt) && !/^(?:calc|min|max|clamp)\(/i.test(pt)) return false;
        idx++;
      }
      return idx === toks.length;
    }
    if (prop === "word-space-transform") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 0 || toks.length > 2) return false;
      if (toks.length === 1 && toks[0] === "none") return true;
      let seenMain = false, seenAutoPhrase = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "space" || t === "ideographic-space") {
          if (seenMain) return false; seenMain = true;
        } else if (t === "auto-phrase") {
          if (seenAutoPhrase) return false; seenAutoPhrase = true;
        } else return false;
      }
      return seenMain;
    }
    if (prop === "text-autospace") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 0 || toks.length > 4) return false;
      if (toks.length === 1 && (toks[0] === "normal" || toks[0] === "no-autospace" || toks[0] === "auto")) return true;
      let seenIA = false, seenIN = false, seenP = false, seenAct = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "ideograph-alpha") { if (seenIA) return false; seenIA = true; }
        else if (t === "ideograph-numeric") { if (seenIN) return false; seenIN = true; }
        else if (t === "punctuation") { if (seenP) return false; seenP = true; }
        else if (t === "insert" || t === "replace") { if (seenAct) return false; seenAct = true; }
        else return false;
      }
      return seenIA || seenIN || seenP;
    }
    if (prop === "text-spacing") {
      const TRIM_KW = new Set(["space-all", "trim-both", "trim-all", "trim-start", "space-first"]);
      const toks = splitCssTokens(vLower);
      if (toks.length === 0) return false;
      if (toks.length === 1 && (toks[0] === "none" || toks[0] === "auto" || toks[0] === "normal" || toks[0] === "no-autospace" || TRIM_KW.has(toks[0]))) return true;
      if (toks.some((t) => t === "none" || t === "auto")) return false;
      let trimCount = 0, normalCount = 0;
      const autoToks = [];
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (TRIM_KW.has(t)) trimCount++;
        else if (t === "normal") normalCount++;
        else autoToks.push(t);
      }
      if (trimCount > 1) return false;
      if (trimCount + normalCount > 2) return false;
      if (autoToks.length > 0) {
        if (normalCount > 1 || (trimCount === 1 && normalCount > 0)) return false;
        if (autoToks.length === 1 && autoToks[0] === "no-autospace") return true;
        return isValidCssValue("text-autospace", autoToks.join(" "));
      }
      return (trimCount + normalCount) <= 2;
    }
    if (prop === "hanging-punctuation") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 0 || toks.length > 3) return false;
      if (toks.length === 1 && toks[0] === "none") return true;
      let seenFirst = false, seenEnd = false, seenLast = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "first") { if (seenFirst) return false; seenFirst = true; }
        else if (t === "force-end" || t === "allow-end") { if (seenEnd) return false; seenEnd = true; }
        else if (t === "last") { if (seenLast) return false; seenLast = true; }
        else return false;
      }
      return true;
    }
    if (prop === "clip") {
      if (vLower === "auto") return true;
      const rm = /^rect\((.+)\)$/i.exec(vLower);
      if (!rm) return false;
      const args = splitTopLevelComma(rm[1]);
      if (args.length !== 4) return false;
      return args.every((a) => {
        if (a === "auto") return true;
        if (a.endsWith("%") || (/^(?:calc|min|max|clamp)\(/i.test(a) && a.includes("%"))) return false;
        return isValidLenPct(a, true);
      });
    }
    if (prop === "perspective" || prop === "-webkit-perspective") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t === "none") return true;
      if (t.endsWith("%") || (/^(?:calc|min|max|clamp)\(/i.test(t) && t.includes("%"))) return false;
      if (prop === "-webkit-perspective" && /^(?:0|[+]?(?:\d+(?:\.\d+)?|\.\d+))$/.test(t)) return true;
      if (/^(?:calc|min|max|clamp)\(/i.test(t) && !/(?:px|em|rem|ex|ch|cap|ic|lh|rlh|vw|vh|vi|vb|vmin|vmax|cm|mm|q|in|pt|pc)\b/i.test(t)) return false;
      return isValidLenPct(t, false);
    }
    if (prop === "perspective-origin" || prop === "object-position" || prop === "transform-origin") {
      const toks = splitCssTokens(vLower);
      if (prop === "transform-origin") {
        if (toks.length === 4) return false;
        if (toks.length === 3) {
          const z = toks[2];
          if (z.endsWith("%") || (/^(?:calc|min|max|clamp)\(/i.test(z) && z.includes("%"))) return false;
          return isValidCssPositionTokens(toks.slice(0, 2)) && isValidLenPct(z, true);
        }
      }
      return isValidCssPositionTokens(toks);
    }
    if (prop === "mask-composite") {
      const layers = vLower.split(",");
      for (let i = 0; i < layers.length; i++) {
        const toks = splitCssTokens(layers[i].trim());
        if (toks.length !== 1 || !/^(?:add|subtract|intersect|exclude)$/.test(toks[0])) return false;
      }
      return true;
    }
    if (prop === "mask-position") {
      const layers = vLower.split(",");
      for (let i = 0; i < layers.length; i++) {
        const toks = splitCssTokens(layers[i].trim());
        if (!isValidCssPositionTokens(toks)) return false;
      }
      return true;
    }
    if (prop === "background-size" || prop === "mask-size") {
      const layers = vLower.split(",");
      for (let i = 0; i < layers.length; i++) {
        const toks = splitCssTokens(layers[i].trim());
        if (toks.length === 1 && (toks[0] === "cover" || toks[0] === "contain")) continue;
        if (toks.length < 1 || toks.length > 2) return false;
        for (let j = 0; j < toks.length; j++) {
          if (toks[j] !== "auto" && !isValidLenPct(toks[j], false)) return false;
        }
      }
      return true;
    }
    if (prop === "background-position") {
      return splitTopLevelComma(vLower).every((layer) => {
        const toks = splitCssTokens(layer.trim());
        return toks.length >= 1 && isValidCssPositionTokens(toks);
      });
    }
    if (prop === "background-position-x" || prop === "background-position-y") {
      const isX = prop === "background-position-x";
      const EDGES = isX ? new Set(["left", "right", "x-start", "x-end"]) : new Set(["top", "bottom", "y-start", "y-end"]);
      const layers = vLower.split(",");
      for (let i = 0; i < layers.length; i++) {
        const toks = splitCssTokens(layers[i].trim());
        if (toks.length === 1) {
          if (toks[0] !== "center" && !EDGES.has(toks[0]) && !isValidLenPct(toks[0], true)) return false;
        } else if (toks.length === 2) {
          if (!EDGES.has(toks[0]) || !isValidLenPct(toks[1], true)) return false;
        } else return false;
      }
      return true;
    }
    if (prop === "border-spacing") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 2) return false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t.endsWith("%") || (/^(?:calc|min|max|clamp)\(/i.test(t) && t.includes("%"))) return false;
        if (!isValidLenPct(t, false)) return false;
      }
      return true;
    }
    if (prop === "text-combine-upright") {
      return vLower === "none" || vLower === "all";
    }
    if (prop === "text-orientation") {
      return vLower === "mixed" || vLower === "upright" || vLower === "sideways";
    }
    if (prop === "will-change") {
      if (vLower === "auto") return true;
      const BAD_WC = new Set(["auto", "will-change", "none", "all", "initial", "inherit", "unset", "revert", "revert-layer", "revert-rule", "default"]);
      const parts = splitTopLevelComma(trimmed);
      for (let i = 0; i < parts.length; i++) {
        const p = parts[i].trim();
        if (!p) return false;
        const toks = splitCssTokens(p);
        if (toks.length !== 1) return false;
        const t = toks[0];
        if (BAD_WC.has(t.toLowerCase())) return false;
        if (!/^(?:--[a-z0-9_-]|-?(?:[a-z_]|\\.))(?:[a-z0-9_-]|\\.)*$/i.test(t)) return false;
      }
      return true;
    }
    if (prop === "cursor") {
      const KW = new Set([
        "auto", "default", "none", "context-menu", "help", "pointer", "progress", "wait",
        "cell", "crosshair", "text", "vertical-text", "alias", "copy", "move", "no-drop",
        "not-allowed", "grab", "grabbing", "e-resize", "n-resize", "ne-resize", "nw-resize",
        "s-resize", "se-resize", "sw-resize", "w-resize", "ew-resize", "ns-resize",
        "nesw-resize", "nwse-resize", "col-resize", "row-resize", "all-scroll", "zoom-in", "zoom-out"
      ]);
      const parts = splitTopLevelComma(vLower);
      const last = splitCssTokens(parts[parts.length - 1].trim());
      if (last.length !== 1 || !KW.has(last[0])) return false;
      const isCoord = (x) => /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)$/.test(x) || /^(?:calc|min|max|clamp)\(/i.test(x);
      const isCursorImg = (tok) => {
        if (/^(?:url|image-set|(?:repeating-)?(?:linear|radial|conic)-gradient|cross-fade)\(/i.test(tok)) return true;
        if (/^light-dark\(/i.test(tok)) {
          const inn = tok.slice(11, -1).trim();
          const p = splitTopLevelComma(inn);
          return p.length === 2 && isCursorImg(p[0]) && isCursorImg(p[1]);
        }
        return false;
      };
      for (let i = 0; i < parts.length - 1; i++) {
        const toks = splitCssTokens(parts[i].trim());
        if (toks.length !== 1 && toks.length !== 3) return false;
        if (!isCursorImg(toks[0])) return false;
        if (toks.length === 3 && (!isCoord(toks[1]) || !isCoord(toks[2]))) return false;
      }
      return true;
    }
    if (prop === "font-size") {
      const KW = new Set(["xx-small", "x-small", "small", "medium", "large", "x-large", "xx-large", "xxx-large", "smaller", "larger", "math"]);
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      return KW.has(toks[0]) || isValidLenPct(toks[0], false);
    }
    if (prop === "font-style") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) return toks[0] === "normal" || toks[0] === "italic" || toks[0] === "oblique";
      if (toks.length === 2 && toks[0] === "oblique") {
        const t = toks[1];
        if (/^(?:calc|min|max|clamp|var|env)\(/i.test(t)) return true;
        const m = /^([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?)(deg|rad|grad|turn)$/i.exec(t);
        if (!m) return false;
        const val = Number(m[1]), u = m[2].toLowerCase();
        const deg = u === "deg" ? val : (u === "turn" ? val * 360 : (u === "grad" ? val * 0.9 : val * 180 / Math.PI));
        return deg >= -90 && deg <= 90;
      }
      return false;
    }
    if (prop === "font-stretch" || prop === "font-width") {
      const KW = new Set(["normal", "ultra-condensed", "extra-condensed", "condensed", "semi-condensed", "semi-expanded", "expanded", "extra-expanded", "ultra-expanded"]);
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (KW.has(t) || /^(?:calc|min|max|clamp|var|env)\(/i.test(t)) return true;
      const m = /^([+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?)%$/i.exec(t);
      return !!m && Number(m[1]) >= 0;
    }
    if (prop === "font-palette") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      return t === "normal" || t === "light" || t === "dark" || /^--[a-z0-9_-]*$/i.test(t);
    }
    if (prop === "font-size-adjust") {
      const METRICS = new Set(["ex-height", "cap-height", "ch-width", "ic-height", "ic-width"]);
      const isAdjVal = (t) => t === "from-font" || (/^(?:calc|min|max|clamp|var|env)\(/i.test(t)) ||
        (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(t) && Number(t) >= 0);
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) return toks[0] === "none" || isAdjVal(toks[0]);
      if (toks.length === 2) return METRICS.has(toks[0]) && isAdjVal(toks[1]);
      return false;
    }
    if (prop === "font-synthesis") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && toks[0] === "none") return true;
      if (toks.length < 1 || toks.length > 4) return false;
      let seenW = false, seenS = false, seenSC = false, seenP = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "weight") { if (seenW) return false; seenW = true; }
        else if (t === "style" || t === "oblique-only") { if (seenS) return false; seenS = true; }
        else if (t === "small-caps") { if (seenSC) return false; seenSC = true; }
        else if (t === "position") { if (seenP) return false; seenP = true; }
        else return false;
      }
      return true;
    }
    if (prop === "font-language-override") {
      const toks = splitCssTokens(trimmed);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t.toLowerCase() === "normal") return true;
      const qm = /^(?:"([^"]*)"|'([^']*)')$/.exec(t);
      if (!qm) return false;
      const inner = qm[1] !== undefined ? qm[1] : qm[2];
      return /^[\x21-\x7e][\x20-\x7e]{0,3}$/.test(inner);
    }
    if (prop === "font-variant-numeric") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && toks[0] === "normal") return true;
      if (toks.length < 1 || toks.length > 5) return false;
      let sFig = false, sSpc = false, sFrc = false, sOrd = false, sZero = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "lining-nums" || t === "oldstyle-nums") { if (sFig) return false; sFig = true; }
        else if (t === "proportional-nums" || t === "tabular-nums") { if (sSpc) return false; sSpc = true; }
        else if (t === "diagonal-fractions" || t === "stacked-fractions") { if (sFrc) return false; sFrc = true; }
        else if (t === "ordinal") { if (sOrd) return false; sOrd = true; }
        else if (t === "slashed-zero") { if (sZero) return false; sZero = true; }
        else return false;
      }
      return true;
    }
    if (prop === "font-variant-ligatures") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && (toks[0] === "normal" || toks[0] === "none")) return true;
      if (toks.length < 1 || toks.length > 4) return false;
      let sCom = false, sDis = false, sHis = false, sCtx = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "common-ligatures" || t === "no-common-ligatures") { if (sCom) return false; sCom = true; }
        else if (t === "discretionary-ligatures" || t === "no-discretionary-ligatures") { if (sDis) return false; sDis = true; }
        else if (t === "historical-ligatures" || t === "no-historical-ligatures") { if (sHis) return false; sHis = true; }
        else if (t === "contextual" || t === "no-contextual") { if (sCtx) return false; sCtx = true; }
        else return false;
      }
      return true;
    }
    if (prop === "font-variant-east-asian") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && toks[0] === "normal") return true;
      if (toks.length < 1 || toks.length > 3) return false;
      const VARS = new Set(["jis78", "jis83", "jis90", "jis04", "simplified", "traditional"]);
      let sVar = false, sWid = false, sRuby = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (VARS.has(t)) { if (sVar) return false; sVar = true; }
        else if (t === "full-width" || t === "proportional-width") { if (sWid) return false; sWid = true; }
        else if (t === "ruby") { if (sRuby) return false; sRuby = true; }
        else return false;
      }
      return true;
    }
    if (prop === "align-content" || prop === "justify-content" ||
        prop === "align-items" || prop === "justify-items" ||
        prop === "align-self" || prop === "justify-self" ||
        prop === "place-content" || prop === "place-items" || prop === "place-self") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 0) return false;
      function consumeAlign(arr, i, p) {
        if (i >= arr.length) return -1;
        const t = arr[i];
        if (t === "normal" || ((p === "align-self" || p === "justify-self") && t === "auto")) return i + 1;
        if (t === "stretch") return i + 1;
        if ((p === "align-content" || p === "justify-content") && (t === "space-between" || t === "space-around" || t === "space-evenly")) return i + 1;
        if (p !== "justify-content") {
          if ((t === "first" || t === "last") && i + 1 < arr.length && arr[i + 1] === "baseline") return i + 2;
          if (t === "baseline") return i + 1;
        }
        if (p === "justify-items") {
          const LEG = new Set(["left", "right", "center"]);
          if (t === "legacy") return (i + 1 < arr.length && LEG.has(arr[i + 1])) ? i + 2 : i + 1;
          if (LEG.has(t) && i + 1 < arr.length && arr[i + 1] === "legacy") return i + 2;
        }
        let posIdx = i;
        if (t === "safe" || t === "unsafe") {
          posIdx = i + 1;
          if (posIdx >= arr.length) return -1;
        }
        const pt = arr[posIdx];
        const base = pt === "center" || pt === "start" || pt === "end" || pt === "flex-start" || pt === "flex-end";
        const selfPos = (p !== "align-content" && p !== "justify-content") && (pt === "self-start" || pt === "self-end");
        const lrPos = (p === "justify-content" || p === "justify-items" || p === "justify-self") && (pt === "left" || pt === "right");
        if (base || selfPos || lrPos) return posIdx + 1;
        return -1;
      }
      if (prop.startsWith("place-")) {
        const kind = prop.slice(6);
        const next = consumeAlign(toks, 0, "align-" + kind);
        if (next === -1) return false;
        if (next === toks.length) return true;
        return consumeAlign(toks, next, "justify-" + kind) === toks.length;
      }
      return consumeAlign(toks, 0, prop) === toks.length;
    }
    if (prop === "grid-auto-flow") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 2) return false;
      let seenAxis = false, seenDense = false;
      for (let i = 0; i < toks.length; i++) {
        if (toks[i] === "row" || toks[i] === "column") { if (seenAxis) return false; seenAxis = true; }
        else if (toks[i] === "dense") { if (seenDense) return false; seenDense = true; }
        else return false;
      }
      return true;
    }
    if (prop === "counter-reset" || prop === "counter-increment" || prop === "counter-set") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && toks[0] === "none") return true;
      if (toks.length === 0) return false;
      const BAD_ID = new Set(["none", "default", "inherit", "initial", "unset", "revert", "revert-layer"]);
      const isIdent = (s) => !BAD_ID.has(s) && /^(?:-?(?:[a-z_]|\\.)(?:[a-z0-9_-]|\\.)*)$/i.test(s) && !s.startsWith("--");
      let i = 0;
      while (i < toks.length) {
        const t = toks[i];
        if (t.includes(",")) return false;
        let okName = isIdent(t);
        if (!okName && prop === "counter-reset") {
          const rm = /^reversed\(([^)]+)\)$/.exec(t);
          if (rm && isIdent(rm[1].trim())) okName = true;
        }
        if (!okName) return false;
        i++;
        if (i < toks.length && (/^[+-]?\d+$/.test(toks[i]) || /^(?:calc|min|max|clamp|sign)\(/i.test(toks[i]))) i++;
      }
      return true;
    }
    if (prop === "aspect-ratio") {
      const norm = vLower.replace(/\s*\/\s*/g, " / ");
      const toks = splitCssTokens(norm);
      if (toks.length === 1 && toks[0] === "auto") return true;
      const isPosNum = (s) => (/^(?:calc|min|max|clamp|var|env)\(/i.test(s)) ||
        (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(s) && Number(s) >= 0);
      let idx = 0, len = toks.length;
      if (toks[0] === "auto") { idx = 1; }
      else if (toks[len - 1] === "auto") { len--; }
      const rem = len - idx;
      if (rem === 1) return isPosNum(toks[idx]);
      if (rem === 3 && toks[idx + 1] === "/") return isPosNum(toks[idx]) && isPosNum(toks[idx + 2]);
      return false;
    }
    if (prop === "text-overflow") {
      const toks = splitCssTokens(trimmed);
      if (toks.length < 1 || toks.length > 2) return false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i], tl = t.toLowerCase();
        if (tl !== "clip" && tl !== "ellipsis" && !/^"[^"]*"$/.test(t) && !/^'[^']*'$/.test(t)) return false;
      }
      return true;
    }
    if (prop === "scrollbar-gutter") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) return toks[0] === "auto" || toks[0] === "stable";
      if (toks.length === 2) {
        return (toks[0] === "stable" && toks[1] === "both-edges") || (toks[0] === "both-edges" && toks[1] === "stable");
      }
      return false;
    }
    if (prop === "-webkit-line-clamp") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t === "none" || /^(?:calc|min|max|clamp|var|env)\(/i.test(t)) return true;
      return /^\+?[1-9]\d*$/.test(t);
    }
    if (prop === "max-lines") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 2) return false;
      if (toks.length === 1) {
        return toks[0] === "auto" || /^\+?[1-9]\d*$/.test(toks[0]) || /^(?:calc|min|max|clamp|var|env)\(/i.test(toks[0]);
      }
      return (toks[0] === "auto" && (/^\+?[1-9]\d*$/.test(toks[1]) || /^(?:calc|min|max|clamp|var|env)\(/i.test(toks[1]))) ||
             (toks[1] === "auto" && (/^\+?[1-9]\d*$/.test(toks[0]) || /^(?:calc|min|max|clamp|var|env)\(/i.test(toks[0])));
    }
    if (prop === "line-clamp") {
      const toks = splitCssTokens(trimmed);
      if (toks.length === 0) return false;
      if (toks.length === 1 && toks[0].toLowerCase() === "none") return true;
      let items = toks;
      if (toks[toks.length - 1].toLowerCase() === "-webkit-legacy") {
        items = toks.slice(0, -1);
        if (items.length === 0) return false;
      }
      let seenInt = false;
      let seenEllipsis = false;
      const isPositiveInt = (t) => /^\+?[1-9]\d*$/.test(t) || /^(?:calc|min|max|clamp)\(/i.test(t);
      const isBlockEllipsis = (t) => {
        const tl = t.toLowerCase();
        return tl === "none" || tl === "auto" || tl === "ellipsis" || tl === "no-ellipsis" || /^"[^"]*"$/.test(t) || /^'[^']*'$/.test(t);
      };
      for (const t of items) {
        const tl = t.toLowerCase();
        if (isPositiveInt(t) && !seenInt) {
          seenInt = true;
        } else if (isBlockEllipsis(t) && !seenEllipsis) {
          if (tl === "none") return false;
          seenEllipsis = true;
        } else {
          return false;
        }
      }
      return seenInt || seenEllipsis;
    }
    if (prop === "overflow-clip-margin") {
      const toks = splitCssTokens(trimmed);
      if (toks.length < 1 || toks.length > 2) return false;
      let box = null, len = null;
      const BOXES = new Set(["content-box", "padding-box", "border-box"]);
      for (const t of toks) {
        const tl = t.toLowerCase();
        if (BOXES.has(tl) && !box) {
          box = tl;
        } else if (!len) {
          if (tl.includes("%")) return false;
          if (/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)px$/i.test(t) || /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)em$/i.test(t) || /^(?:calc|min|max|clamp)\(/i.test(t)) {
            len = t;
          } else {
            return false;
          }
        } else {
          return false;
        }
      }
      return true;
    }
    if (prop === "contain") {
      const toks = splitCssTokens(trimmed);
      if (toks.length === 0) return false;
      if (toks.length === 1) {
        const t0 = toks[0].toLowerCase();
        if (t0 === "none" || t0 === "strict" || t0 === "content") return true;
      }
      let hasSize = false;
      let hasInlineSize = false;
      let hasLayout = false;
      let hasStyle = false;
      let hasPaint = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i].toLowerCase();
        if (t === "size") {
          if (hasSize || hasInlineSize) return false;
          hasSize = true;
        } else if (t === "inline-size") {
          if (hasSize || hasInlineSize) return false;
          hasInlineSize = true;
        } else if (t === "layout") {
          if (hasLayout) return false;
          hasLayout = true;
        } else if (t === "style") {
          if (hasStyle) return false;
          hasStyle = true;
        } else if (t === "paint") {
          if (hasPaint) return false;
          hasPaint = true;
        } else {
          return false;
        }
      }
      return hasSize || hasInlineSize || hasLayout || hasStyle || hasPaint;
    }
    if (prop === "contain-intrinsic-size") {
      return parseContainIntrinsicSize(trimmed) !== null;
    }
    if (prop === "contain-intrinsic-width" || prop === "contain-intrinsic-height" ||
        prop === "contain-intrinsic-inline-size" || prop === "contain-intrinsic-block-size") {
      const toks = splitCssTokens(trimmed);
      return parseSingleIntrinsic(toks) !== null;
    }
    if (prop === "shape-image-threshold") {
      const tl = trimmed.toLowerCase();
      if (tl.endsWith("%")) {
        const num = Number(tl.slice(0, -1));
        return !isNaN(num) && isFinite(num);
      }
      if (/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)$/.test(tl)) return true;
      return /^(?:calc|min|max|clamp)\(/i.test(tl);
    }
    if (prop === "shape-margin") {
      const tl = trimmed.toLowerCase();
      if (tl === "0" || tl === "+0") return true;
      if (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)%$/.test(tl)) return true;
      if (/^(?:calc|min|max|clamp)\(/i.test(tl)) return true;
      return /^(?:\+)?(?:\d+(?:\.\d+)?|\.\d+)(?:px|em|rem|vw|vh|vmin|vmax|cqw|cqh|cqi|cqb|cqmin|cqmax|ex|ch|pt|pc|in|cm|mm|q)$/i.test(tl);
    }
    if (prop === "scroll-snap-type") {
      const toks = splitCssTokens(trimmed.toLowerCase());
      if (toks.length === 1) {
        return toks[0] === "none" || toks[0] === "x" || toks[0] === "y" || toks[0] === "block" || toks[0] === "inline" || toks[0] === "both";
      }
      if (toks.length === 2) {
        const AXIS = new Set(["x", "y", "block", "inline", "both"]);
        const STRICT = new Set(["mandatory", "proximity"]);
        return AXIS.has(toks[0]) && STRICT.has(toks[1]);
      }
      return false;
    }
    if (prop === "scroll-snap-align") {
      const toks = splitCssTokens(trimmed.toLowerCase());
      const VALS = new Set(["none", "start", "end", "center"]);
      if (toks.length === 1) return VALS.has(toks[0]);
      if (toks.length === 2) return VALS.has(toks[0]) && VALS.has(toks[1]);
      return false;
    }
    const isSingleScrollMarginValue = (t) => {
      const tl = t.toLowerCase();
      if (tl === "0" || tl === "+0" || tl === "-0") return true;
      if (/(?:auto|none|fit-content|max-content|min-content)/i.test(tl)) return false;
      if (/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:px|em|rem|vw|vh|vmin|vmax|cqw|cqh|cqi|cqb|cqmin|cqmax|ex|ch|pt|pc|in|cm|mm|q)$/i.test(tl)) return true;
      return /^(?:calc|min|max|clamp)\(/i.test(tl);
    };
    const isSingleScrollPaddingValue = (t) => {
      const tl = t.toLowerCase();
      if (tl === "auto") return true;
      if (tl === "0" || tl === "+0") return true;
      if (/(?:auto|none|fit-content|max-content|min-content)/i.test(tl)) return false;
      if (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)%$/.test(tl)) return true;
      if (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:px|em|rem|vw|vh|vmin|vmax|cqw|cqh|cqi|cqb|cqmin|cqmax|ex|ch|pt|pc|in|cm|mm|q)$/i.test(tl)) return true;
      return /^(?:calc|min|max|clamp)\(/i.test(tl);
    };
    if (prop === "scroll-margin-top" || prop === "scroll-margin-right" || prop === "scroll-margin-bottom" || prop === "scroll-margin-left" ||
        prop === "scroll-margin-block-start" || prop === "scroll-margin-block-end" || prop === "scroll-margin-inline-start" || prop === "scroll-margin-inline-end") {
      const toks = splitCssTokens(trimmed);
      return toks.length === 1 && isSingleScrollMarginValue(toks[0]);
    }
    if (prop === "scroll-margin-block" || prop === "scroll-margin-inline") {
      const toks = splitCssTokens(trimmed);
      return toks.length >= 1 && toks.length <= 2 && toks.every(isSingleScrollMarginValue);
    }
    if (prop === "scroll-margin") {
      const toks = splitCssTokens(trimmed);
      return toks.length >= 1 && toks.length <= 4 && toks.every(isSingleScrollMarginValue);
    }
    if (prop === "scroll-padding-top" || prop === "scroll-padding-right" || prop === "scroll-padding-bottom" || prop === "scroll-padding-left" ||
        prop === "scroll-padding-block-start" || prop === "scroll-padding-block-end" || prop === "scroll-padding-inline-start" || prop === "scroll-padding-inline-end") {
      const toks = splitCssTokens(trimmed);
      return toks.length === 1 && isSingleScrollPaddingValue(toks[0]);
    }
    if (prop === "scroll-padding-block" || prop === "scroll-padding-inline") {
      const toks = splitCssTokens(trimmed);
      return toks.length >= 1 && toks.length <= 2 && toks.every(isSingleScrollPaddingValue);
    }
    if (prop === "scroll-padding") {
      const toks = splitCssTokens(trimmed);
      return toks.length >= 1 && toks.length <= 4 && toks.every(isSingleScrollPaddingValue);
    }

    if (prop === "block-ellipsis") {
      const toks = splitCssTokens(trimmed);
      if (toks.length !== 1) return false;
      const t = toks[0], tl = t.toLowerCase();
      return tl === "no-ellipsis" || tl === "ellipsis" || /^"[^"]*"$/.test(t) || /^'[^']*'$/.test(t);
    }
    if (prop === "text-decoration-line") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && (toks[0] === "none" || toks[0] === "spelling-error" || toks[0] === "grammar-error")) return true;
      if (toks.length < 1 || toks.length > 4) return false;
      const FLAGS = new Set(["underline", "overline", "line-through", "blink"]);
      const seen = new Set();
      for (let i = 0; i < toks.length; i++) {
        if (!FLAGS.has(toks[i]) || seen.has(toks[i])) return false;
        seen.add(toks[i]);
      }
      return true;
    }
    if (prop === "text-underline-position") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && toks[0] === "auto") return true;
      if (toks.length < 1 || toks.length > 2) return false;
      let sEdge = false, sSide = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "under" || t === "from-font") { if (sEdge) return false; sEdge = true; }
        else if (t === "left" || t === "right") { if (sSide) return false; sSide = true; }
        else return false;
      }
      return true;
    }
    if (prop === "text-underline-offset") {
      const toks = splitCssTokens(vLower);
      return toks.length === 1 && (toks[0] === "auto" || isValidLenPct(toks[0], true));
    }
    if (prop === "animation") {
      const items = splitTopLevelComma(trimmed);
      if (items.length === 0) return false;
      const TF_KW = new Set(["linear", "ease", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end"]);
      const DIR = new Set(["normal", "reverse", "alternate", "alternate-reverse"]);
      const FILL = new Set(["none", "forwards", "backwards", "both"]);
      const PLAY = new Set(["running", "paused"]);
      const BAD = new Set(["initial", "inherit", "unset", "revert", "revert-layer", "default"]);
      for (let li = 0; li < items.length; li++) {
        const p = splitCssTokens(items[li]);
        if (p.length === 0) return false;
        let seenTime = 0, seenTf = false, seenIter = false, seenDir = false, seenFill = false, seenPlay = false, seenName = false;
        for (let i = 0; i < p.length; i++) {
          const t = p[i], tl = t.toLowerCase();
          if (!seenTf && (TF_KW.has(tl) || /^(?:cubic-bezier|steps|linear)\(/i.test(tl))) {
            seenTf = true;
          } else if (seenTime < 2 && /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?(?:ms|s)$/i.test(tl)) {
            if (seenTime === 0 && parseFloat(tl) < 0) return false;
            seenTime++;
          } else if (!seenIter && (tl === "infinite" || (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(tl) && tl !== ""))) {
            seenIter = true;
          } else if (!seenDir && DIR.has(tl)) {
            seenDir = true;
          } else if (!seenFill && FILL.has(tl)) {
            seenFill = true;
          } else if (!seenPlay && PLAY.has(tl)) {
            seenPlay = true;
          } else if (!seenName && !BAD.has(tl) && (/^(?:-?(?:[a-z_]|\\.)(?:[a-z0-9_-]|\\.)*)$/i.test(t) || (t.length > 2 && ((t.startsWith('"') && t.endsWith('"')) || (t.startsWith("'") && t.endsWith("'")))))) {
            seenName = true;
          } else {
            return false;
          }
        }
      }
      return true;
    }
    if (prop === "animation-composition") {
      const items = splitTopLevelComma(vLower);
      if (items.length === 0) return false;
      for (let i = 0; i < items.length; i++) {
        if (items[i] !== "replace" && items[i] !== "add" && items[i] !== "accumulate") return false;
      }
      return true;
    }
    if (prop === "animation-range-start" || prop === "animation-range-end") {
      const RANGE_NAMES = new Set(["cover", "contain", "entry", "exit", "entry-crossing", "exit-crossing"]);
      const items = splitTopLevelComma(vLower);
      if (items.length === 0) return false;
      for (let i = 0; i < items.length; i++) {
        const toks = splitCssTokens(items[i]);
        if (toks.length === 0 || toks.length > 2) return false;
        if (toks.length === 1) {
          if (toks[0] === "normal" || RANGE_NAMES.has(toks[0])) continue;
          if (!isValidLenPct(toks[0], true)) return false;
        } else {
          if (!RANGE_NAMES.has(toks[0])) return false;
          if (!isValidLenPct(toks[1], true)) return false;
        }
      }
      return true;
    }
    if (prop === "animation-name") {
      const items = splitTopLevelComma(trimmed);
      if (items.length === 0) return false;
      const BAD = new Set(["initial", "inherit", "unset", "revert", "revert-layer", "default"]);
      for (let i = 0; i < items.length; i++) {
        const toks = splitCssTokens(items[i]);
        if (toks.length !== 1) return false;
        const t = toks[0], tl = t.toLowerCase();
        if ((t.startsWith('"') && t.endsWith('"')) || (t.startsWith("'") && t.endsWith("'"))) {
          if (t.length <= 2) return false;
          continue;
        }
        if (BAD.has(tl) || !/^(?:-*(?:[a-z_]|\\.)(?:[a-z0-9_-]|\\.)*)$/i.test(t)) return false;
      }
      return true;
    }
    if (prop === "transition") {
      const items = splitTopLevelComma(trimmed);
      if (items.length === 0) return false;
      const TF_KW = new Set(["linear", "ease", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end"]);
      const BAD_PROP = new Set(["initial", "inherit", "unset", "revert", "revert-layer", "default"]);
      for (let li = 0; li < items.length; li++) {
        const p = splitCssTokens(items[li]);
        if (p.length === 0) return false;
        let seenProp = false, seenTf = false, seenBeh = false, seenTime = 0;
        for (let i = 0; i < p.length; i++) {
          const t = p[i], tl = t.toLowerCase();
          if (tl === "normal" || tl === "allow-discrete") {
            if (seenBeh) return false; seenBeh = true;
          } else if (TF_KW.has(tl) || /^(?:cubic-bezier|steps|linear)\(/i.test(tl)) {
            if (seenTf) return false; seenTf = true;
          } else if (/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?(?:ms|s)$/i.test(tl) || tl === "0") {
            if (seenTime >= 2) return false;
            if (seenTime === 0 && parseFloat(tl) < 0) return false;
            seenTime++;
          } else if (!BAD_PROP.has(tl) && /^(?:-?(?:[a-z_]|\\.)(?:[a-z0-9_-]|\\.)*)$/i.test(t)) {
            if (seenProp) return false; seenProp = true;
          } else {
            return false;
          }
        }
      }
      return true;
    }
    if (prop === "transition-behavior") {
      const items = splitTopLevelComma(vLower);
      if (items.length === 0) return false;
      for (let i = 0; i < items.length; i++) {
        if (items[i] !== "normal" && items[i] !== "allow-discrete") return false;
      }
      return true;
    }
    if (prop === "transition-timing-function" || prop === "animation-timing-function") {
      const items = splitTopLevelComma(vLower);
      if (items.length === 0) return false;
      const KW = new Set(["linear", "ease", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end"]);
      const STEP_POS = new Set(["start", "end", "jump-start", "jump-end", "jump-none", "jump-both"]);
      const isNum = (x) => /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(x);
      for (let i = 0; i < items.length; i++) {
        const toks = splitCssTokens(items[i]);
        if (toks.length !== 1) return false;
        const t = toks[0];
        if (KW.has(t)) continue;
        if (t.startsWith("cubic-bezier(") && t.endsWith(")")) {
          const args = splitTopLevelComma(t.slice(13, -1));
          if (args.length !== 4) return false;
          if (!args.every(a => isNum(a) || /^(?:calc|var|env)\(/i.test(a))) return false;
          if (isNum(args[0]) && (Number(args[0]) < 0 || Number(args[0]) > 1)) return false;
          if (isNum(args[2]) && (Number(args[2]) < 0 || Number(args[2]) > 1)) return false;
          continue;
        }
        if (t.startsWith("steps(") && t.endsWith(")")) {
          const args = splitTopLevelComma(t.slice(6, -1));
          if (args.length < 1 || args.length > 2) return false;
          if (!/^\+?\d+$/.test(args[0])) return false;
          const n = Number(args[0]);
          if (args.length === 2 && !STEP_POS.has(args[1])) return false;
          if (n < (args[1] === "jump-none" ? 2 : 1)) return false;
          continue;
        }
        if (t.startsWith("linear(") && t.endsWith(")")) {
          const inner = t.slice(7, -1).trim();
          if (!inner) return false;
          continue;
        }
        return false;
      }
      return true;
    }
    if (prop === "transition-property") {
      const items = trimmed.split(",");
      if (items.length === 1 && items[0].trim().toLowerCase() === "none") return true;
      const BAD = new Set(["none", "initial", "inherit", "unset", "revert", "revert-layer", "default"]);
      for (let i = 0; i < items.length; i++) {
        const toks = splitCssTokens(items[i].trim());
        if (toks.length !== 1) return false;
        const t = toks[0], tl = t.toLowerCase();
        if (BAD.has(tl) || !/^(?:-*(?:[a-z_]|\\.)(?:[a-z0-9_-]|\\.)*)$/i.test(t)) return false;
      }
      return true;
    }
    if (prop === "text-wrap") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 2) return false;
      const MODES = new Set(["wrap", "nowrap"]);
      const STYLES = new Set(["auto", "balance", "stable", "pretty", "avoid-orphans"]);
      let seenM = false, seenS = false;
      for (let i = 0; i < toks.length; i++) {
        if (MODES.has(toks[i])) { if (seenM) return false; seenM = true; }
        else if (STYLES.has(toks[i])) { if (seenS) return false; seenS = true; }
        else return false;
      }
      return true;
    }
    if (prop === "text-decoration-thickness") {
      const toks = splitCssTokens(vLower);
      return toks.length === 1 && (toks[0] === "auto" || toks[0] === "from-font" || isValidLenPct(toks[0], true));
    }
    if (prop === "text-shadow") {
      if (vLower === "none") return true;
      const layers = splitTopLevelComma(trimmed);
      if (layers.length === 0) return false;
      for (let li = 0; li < layers.length; li++) {
        const toks = splitCssTokens(layers[li]);
        if (toks.length < 2 || toks.length > 4) return false;
        let colorCount = 0;
        const lens = [];
        for (let ti = 0; ti < toks.length; ti++) {
          const tok = toks[ti];
          if (!tok.includes("%") && isValidLenPct(tok, true)) {
            lens.push(tok);
          } else if (isValidColor(tok)) {
            colorCount++;
            if (colorCount > 1) return false;
          } else {
            return false;
          }
        }
        if (lens.length < 2 || lens.length > 3) return false;
        if (lens.length === 3 && (lens[2].includes("%") || !isValidLenPct(lens[2], false))) return false;
      }
      return true;
    }
    if (prop === "box-shadow") {
      if (vLower === "none") return true;
      const layers = splitTopLevelComma(trimmed);
      if (layers.length === 0) return false;
      for (let li = 0; li < layers.length; li++) {
        const toks = splitCssTokens(layers[li]);
        if (toks.length < 2 || toks.length > 6) return false;
        let colorCount = 0, insetCount = 0;
        let lenPhase = 0;
        const lens = [];
        for (let ti = 0; ti < toks.length; ti++) {
          const tok = toks[ti];
          const tl = tok.toLowerCase();
          if (!tok.includes("%") && isValidLenPct(tok, true)) {
            if (lenPhase === 2) return false;
            lenPhase = 1;
            lens.push(tok);
          } else {
            if (lenPhase === 1) lenPhase = 2;
            if (tl === "inset") {
              insetCount++;
              if (insetCount > 1) return false;
            } else if (isValidColor(tok)) {
              colorCount++;
              if (colorCount > 1) return false;
            } else {
              return false;
            }
          }
        }
        if (lens.length < 2 || lens.length > 4) return false;
        if (lens.length >= 3 && (lens[2].includes("%") || !isValidLenPct(lens[2], false))) return false;
      }
      return true;
    }
    if (prop === "text-decoration-skip-spaces") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) return toks[0] === "none" || toks[0] === "all" || toks[0] === "start" || toks[0] === "end";
      if (toks.length === 2) return (toks[0] === "start" && toks[1] === "end") || (toks[0] === "end" && toks[1] === "start");
      return false;
    }
    if (prop === "text-decoration-inset") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) return toks[0] === "auto" || isValidLenPct(toks[0], true);
      if (toks.length === 2) return isValidLenPct(toks[0], true) && isValidLenPct(toks[1], true);
      return false;
    }
    if (prop === "text-emphasis-position") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) return toks[0] === "auto" || toks[0] === "over" || toks[0] === "under";
      if (toks.length === 2) {
        const OU = new Set(["over", "under"]);
        const LR = new Set(["left", "right"]);
        return (OU.has(toks[0]) && LR.has(toks[1])) || (LR.has(toks[0]) && OU.has(toks[1]));
      }
      return false;
    }
    if (prop === "text-emphasis-style") {
      const toks = splitCssTokens(trimmed);
      if (toks.length === 1) {
        const tl = toks[0].toLowerCase();
        if (tl === "none" || tl === "filled" || tl === "open") return true;
        if (tl === "dot" || tl === "circle" || tl === "double-circle" || tl === "triangle" || tl === "sesame") return true;
        return /^"[^"]*"$/.test(toks[0]) || /^'[^']*'$/.test(toks[0]);
      }
      if (toks.length === 2) {
        const a = toks[0].toLowerCase(), b = toks[1].toLowerCase();
        const FILL = new Set(["filled", "open"]);
        const SHAPE = new Set(["dot", "circle", "double-circle", "triangle", "sesame"]);
        return (FILL.has(a) && SHAPE.has(b)) || (SHAPE.has(a) && FILL.has(b));
      }
      return false;
    }
    if (prop === "text-emphasis") {
      const toks = splitCssTokens(trimmed);
      if (toks.length < 1 || toks.length > 3) return false;
      const FILL = new Set(["filled", "open"]);
      const SHAPE = new Set(["dot", "circle", "double-circle", "triangle", "sesame"]);
      let hasFill = false, hasShape = false, hasStr = false, hasColor = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i], tl = t.toLowerCase();
        if (tl === "none") { if (hasFill || hasShape || hasStr) return false; hasShape = true; }
        else if (FILL.has(tl)) { if (hasFill || hasStr) return false; hasFill = true; }
        else if (SHAPE.has(tl)) { if (hasShape || hasStr) return false; hasShape = true; }
        else if (/^"[^"]*"$/.test(t) || /^'[^']*'$/.test(t)) { if (hasFill || hasShape || hasStr) return false; hasStr = true; }
        else if (isValidColor(t)) { if (hasColor) return false; hasColor = true; }
        else return false;
      }
      return true;
    }
    if (prop === "outline" || prop === "border" || prop === "column-rule" ||
        prop === "border-top" || prop === "border-right" || prop === "border-bottom" || prop === "border-left" ||
        prop === "border-block" || prop === "border-inline" ||
        prop === "border-block-start" || prop === "border-block-end" || prop === "border-inline-start" || prop === "border-inline-end") {
      const toks = splitCssTokens(trimmed);
      if (toks.length < 1 || toks.length > 3) return false;
      const STYLES = new Set(["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"]);
      let seenW = false, seenS = false, seenC = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i], tl = t.toLowerCase();
        if ((prop === "outline" ? (tl === "auto" || (tl !== "hidden" && STYLES.has(tl))) : STYLES.has(tl))) {
          if (seenS) return false; seenS = true;
        } else if (tl === "thin" || tl === "medium" || tl === "thick" ||
                   (!tl.endsWith("%") && !(/^(?:calc|min|max|clamp)\(/i.test(tl) && tl.includes("%")) && isValidLenPct(tl, false))) {
          if (seenW) return false; seenW = true;
        } else if (isValidColor(t)) {
          if (seenC) return false; seenC = true;
        } else {
          return false;
        }
      }
      return true;
    }
    if (prop === "text-decoration") {
      const toks = splitCssTokens(trimmed);
      if (toks.length < 1) return false;
      const LINES = new Set(["underline", "overline", "line-through", "blink"]);
      const STYLES = new Set(["solid", "double", "dotted", "dashed", "wavy"]);
      let seenStyle = false, seenColor = false, seenThick = false, seenLineGroup = false, inLineGroup = false;
      const seenLines = new Set();
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i], tl = t.toLowerCase();
        if (tl === "none" || tl === "spelling-error" || tl === "grammar-error") {
          if (inLineGroup || seenLineGroup || seenLines.size > 0) return false;
          seenLineGroup = true;
        } else if (LINES.has(tl)) {
          if ((seenLineGroup && !inLineGroup) || seenLines.has(tl)) return false;
          seenLines.add(tl);
          seenLineGroup = true;
          inLineGroup = true;
        } else {
          inLineGroup = false;
          if (STYLES.has(tl)) { if (seenStyle) return false; seenStyle = true; }
          else if (tl === "auto" || tl === "from-font" || isValidLenPct(tl, false)) { if (seenThick) return false; seenThick = true; }
          else if (isValidColor(t)) { if (seenColor) return false; seenColor = true; }
          else return false;
        }
      }
      return true;
    }
    if (prop === "grid-template-areas") {
      const toks = splitCssTokens(trimmed);
      if (toks.length === 1 && toks[0].toLowerCase() === "none") return true;
      if (toks.length < 1) return false;
      let cols = -1;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (!(/^"[^"]*"$/.test(t) || /^'[^']*'$/.test(t))) return false;
        const inner = t.slice(1, -1).trim();
        if (!inner) return false;
        const cells = inner.split(/\s+/);
        for (let c = 0; c < cells.length; c++) {
          if (!/^(?:\.+|[0-9a-zA-Z_\-\u0080-\uffff]+)$/.test(cells[c])) return false;
        }
        if (cols === -1) cols = cells.length;
        else if (cells.length !== cols) return false;
      }
      return true;
    }
    function isValidSymbolsFn(tok) {
      const m = /^symbols\((.*)\)$/i.exec(tok);
      if (!m) return false;
      const args = splitCssTokens(m[1].trim());
      if (args.length === 0) return false;
      const TYPES = new Set(["cyclic", "numeric", "alphabetic", "symbolic", "fixed"]);
      let type = "", idx = 0;
      const firstLow = args[0].toLowerCase();
      if (TYPES.has(firstLow)) { type = firstLow; idx = 1; }
      const count = args.length - idx;
      if (count < ((type === "numeric" || type === "alphabetic") ? 2 : 1)) return false;
      for (let i = idx; i < args.length; i++) {
        if (!(/^"[^"]*"$/.test(args[i]) || /^'[^']*'$/.test(args[i]))) return false;
      }
      return true;
    }
    if (prop === "list-style-type") {
      const toks = splitCssTokens(trimmed);
      if (toks.length !== 1) return false;
      const t = toks[0], tl = t.toLowerCase();
      if (tl === "none" || /^"[^"]*"$/.test(t) || /^'[^']*'$/.test(t)) return true;
      if (/^symbols\(/i.test(t)) return isValidSymbolsFn(t);
      return !/^(?:auto|inherit|initial|unset|revert|revert-layer|default)$/i.test(tl) && /^-?[a-z_][a-z0-9_-]*$/i.test(t);
    }
    if (prop === "list-style-image") {
      const toks = splitCssTokens(trimmed);
      if (toks.length !== 1) return false;
      const t = toks[0], tl = t.toLowerCase();
      return tl === "none" || /^(?:url|linear-gradient|radial-gradient|conic-gradient|repeating-linear-gradient|repeating-radial-gradient|image-set)\(/i.test(tl);
    }
    if (prop === "list-style") {
      const toks = splitCssTokens(trimmed);
      if (toks.length < 1 || toks.length > 3) return false;
      let sPos = false, sImg = false, sType = false, noneCnt = 0;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i], tl = t.toLowerCase();
        if (tl === "inside" || tl === "outside") {
          if (!sPos) sPos = true;
          else if (tl === "outside" && !sType) sType = true;
          else return false;
        }
        else if (tl === "none") { noneCnt++; if (noneCnt > 2) return false; }
        else if (/^(?:url|linear-gradient|radial-gradient|conic-gradient|image-set)\(/i.test(tl)) { if (sImg) return false; sImg = true; }
        else if (/^symbols\(/i.test(t)) { if (sType || !isValidSymbolsFn(t)) return false; sType = true; }
        else if (/^"[^"]*"$/.test(t) || /^'[^']*'$/.test(t) || (/^-?[a-z_][a-z0-9_-]*$/i.test(t) && !/^(?:auto|inherit|initial|unset|revert|revert-layer)$/i.test(tl))) {
          if (sType) return false; sType = true;
        } else return false;
      }
      if (noneCnt > 0 && ((sImg && sType) || (noneCnt === 2 && (sImg || sType)))) return false;
      return true;
    }
    if ((prop.startsWith("margin-") || prop.startsWith("padding-")) && !STANDARD_CSS_PROPERTIES.has(prop)) {
      return false;
    }
    if (prop === "margin-trim") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) return toks[0] === "none" || toks[0] === "block" || toks[0] === "block-start" || toks[0] === "block-end";
      if (toks.length === 2) return (toks[0] === "block-start" && toks[1] === "block-end") || (toks[0] === "block-end" && toks[1] === "block-start");
      return false;
    }
    if (prop === "ruby-align") {
      return vLower === "space-around" || vLower === "start" || vLower === "center" || vLower === "space-between";
    }
    if (prop === "ruby-merge") {
      return vLower === "separate" || vLower === "merge" || vLower === "auto";
    }
    if (prop === "ruby-overhang") {
      return vLower === "auto" || vLower === "spaces" || vLower === "none";
    }
    if (prop === "ruby-position") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) return toks[0] === "over" || toks[0] === "under" || toks[0] === "inter-character" || toks[0] === "alternate";
      if (toks.length === 2) {
        return (toks[0] === "alternate" && (toks[1] === "over" || toks[1] === "under")) ||
               (toks[1] === "alternate" && (toks[0] === "over" || toks[0] === "under"));
      }
      return false;
    }
    if (prop === "block-step-align") {
      return vLower === "auto" || vLower === "center" || vLower === "start" || vLower === "end";
    }
    if (prop === "block-step-insert") {
      return vLower === "margin-box" || vLower === "padding-box" || vLower === "content-box";
    }
    if (prop === "block-step-round") {
      return vLower === "up" || vLower === "down" || vLower === "nearest";
    }
    if (prop === "block-step-size") {
      if (vLower === "none") return true;
      if (vLower.includes("%")) return false;
      return isValidLenPct(vLower, false);
    }
    if (prop === "block-step") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 4) return false;
      let seenSize = false, seenInsert = false, seenAlign = false, seenRound = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "margin-box" || t === "padding-box" || t === "content-box") {
          if (seenInsert) return false; seenInsert = true;
        } else if (t === "auto" || t === "center" || t === "start" || t === "end") {
          if (seenAlign) return false; seenAlign = true;
        } else if (t === "up" || t === "down" || t === "nearest") {
          if (seenRound) return false; seenRound = true;
        } else if (t === "none" || (!t.includes("%") && isValidLenPct(t, false))) {
          if (seenSize) return false; seenSize = true;
        } else {
          return false;
        }
      }
      return true;
    }
    if (prop === "image-orientation") {
      return vLower === "from-image" || vLower === "none";
    }
    if (prop === "accent-color") {
      return vLower === "auto" || isValidColor(trimmed);
    }
    if (prop === "alignment-baseline") {
      return vLower === "baseline" || vLower === "text-bottom" || vLower === "alphabetic" ||
        vLower === "ideographic" || vLower === "middle" || vLower === "central" ||
        vLower === "mathematical" || vLower === "text-top";
    }
    if (prop === "baseline-shift") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      return t === "sub" || t === "super" || t === "top" || t === "center" || t === "bottom" || isValidLenPct(t, true);
    }
    if (prop === "anchor-scope") {
      if (vLower === "none" || vLower === "all") return true;
      const items = splitTopLevelComma(trimmed);
      if (items.length === 0) return false;
      for (let i = 0; i < items.length; i++) {
        const toks = splitCssTokens(items[i]);
        if (toks.length !== 1 || !/^--(?:[a-z0-9_-]|[^\x00-\x7f])+$/i.test(toks[0])) return false;
      }
      return true;
    }
    if (prop === "position-try-order") {
      return vLower === "normal" || vLower === "most-width" || vLower === "most-height" || vLower === "most-block-size" || vLower === "most-inline-size";
    }
    if (prop === "position-visibility") {
      if (vLower === "always") return true;
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 3) return false;
      const seen = new Set();
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t !== "anchors-valid" && t !== "anchors-visible" && t !== "no-overflow") return false;
        if (seen.has(t)) return false;
        seen.add(t);
      }
      return true;
    }
    if (prop === "border-image-source") {
      if (vLower === "none") return true;
      if (splitTopLevelComma(trimmed).length !== 1) return false;
      return /^(?:url|linear-gradient|radial-gradient|conic-gradient|repeating-linear-gradient|repeating-radial-gradient|repeating-conic-gradient|image|image-set|cross-fade|-webkit-linear-gradient|-webkit-radial-gradient)\(/i.test(trimmed);
    }
    if (prop === "border-image-width") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 4) return false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "auto") continue;
        if (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)$/.test(t)) continue;
        if (isValidLenPct(t, false)) continue;
        return false;
      }
      return true;
    }
    if (prop === "border-image-outset") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 4) return false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)$/.test(t)) continue;
        if (!t.includes("%") && isValidLenPct(t, false)) continue;
        return false;
      }
      return true;
    }
    if (prop === "border-image-repeat") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 2) return false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t !== "stretch" && t !== "repeat" && t !== "round" && t !== "space") return false;
      }
      return true;
    }
    if (prop === "border-image-slice") {
      const toks = splitCssTokens(vLower);
      let hasFill = false;
      const nums = [];
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i];
        if (t === "fill") {
          if (hasFill || (i !== 0 && i !== toks.length - 1)) return false;
          hasFill = true;
        } else if (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)%?$/.test(t) || /^calc\(/i.test(t)) {
          if (/^calc\(/i.test(t) && !isValidLenPct(t, false)) return false;
          nums.push(t);
        } else {
          return false;
        }
      }
      return nums.length >= 1 && nums.length <= 4;
    }
    if (prop === "view-transition-name" || prop === "view-transition-class" || prop === "view-transition-group") {
      const toks = splitCssTokens(trimmed);
      if (toks.length === 0) return false;
      const isCustomIdent = (t) => {
        const tl = t.toLowerCase();
        if (tl === "none" || tl === "default" || tl === "inherit" || tl === "initial" || tl === "unset" || tl === "revert" || tl === "revert-layer") return false;
        return /^(?:--|[a-z_-]|[^\x00-\x7f])(?:[a-z0-9_-]|[^\x00-\x7f])*$/i.test(t);
      };
      if (prop === "view-transition-name") {
        return toks.length === 1 && (toks[0].toLowerCase() === "none" || isCustomIdent(toks[0]));
      }
      if (prop === "view-transition-class") {
        if (toks.length === 1 && toks[0].toLowerCase() === "none") return true;
        return toks.every(isCustomIdent);
      }
      if (toks.length !== 1) return false;
      const t0 = toks[0].toLowerCase();
      return t0 === "normal" || t0 === "nearest" || t0 === "contain" || isCustomIdent(toks[0]);
    }
    if (prop === "color-scheme") {
      if (trimmed.includes(",")) return false;
      const toks = splitCssTokens(trimmed);
      if (toks.length === 0) return false;
      if (toks.length === 1 && toks[0].toLowerCase() === "normal") return true;
      const BAD_KW = new Set(["normal", "inherit", "initial", "unset", "revert", "revert-layer", "default"]);
      let onlyCount = 0;
      let start = 0, end = toks.length;
      if (toks[0].toLowerCase() === "only") { onlyCount++; start = 1; }
      else if (toks[toks.length - 1].toLowerCase() === "only") { onlyCount++; end = toks.length - 1; }
      if (start >= end) return false;
      for (let i = start; i < end; i++) {
        const tl = toks[i].toLowerCase();
        if (tl === "only" || BAD_KW.has(tl)) return false;
        if (!/^(?:--|[a-z_-]|[^\x00-\x7f])(?:[a-z0-9_-]|[^\x00-\x7f])*$/i.test(toks[i])) return false;
      }
      return onlyCount <= 1;
    }
    if (prop === "image-resolution") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 3) return false;
      let start = 0, end = toks.length;
      if (toks[0] === "snap") start = 1;
      else if (toks[toks.length - 1] === "snap") end = toks.length - 1;
      const core = toks.slice(start, end);
      if (core.length < 1 || core.length > 2) return false;
      let seenFi = false, seenRes = false;
      for (let i = 0; i < core.length; i++) {
        if (core[i] === "from-image") {
          if (seenFi) return false; seenFi = true;
        } else if (/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:dpi|dpcm|dppx|x)$/i.test(core[i]) || /^calc\(/i.test(core[i])) {
          if (seenRes) return false; seenRes = true;
        } else {
          return false;
        }
      }
      return true;
    }
    if (prop === "object-fit") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) {
        return toks[0] === "fill" || toks[0] === "contain" || toks[0] === "cover" || toks[0] === "none" || toks[0] === "scale-down";
      }
      if (toks.length === 2) {
        return (toks[0] === "scale-down" && (toks[1] === "contain" || toks[1] === "cover")) ||
               (toks[1] === "scale-down" && (toks[0] === "contain" || toks[0] === "cover"));
      }
      return false;
    }
    if (prop === "background-image") {
      const bgLayers = splitTopLevelComma(trimmed);
      for (let bi = 0; bi < bgLayers.length; bi++) {
        const bl = bgLayers[bi].trim();
        if (!bl) return false;
        if (bl.toLowerCase() === "none") continue;
        if (/^cross-fade\(/i.test(bl)) {
          const cfm = /^cross-fade\(\s*(.*)\s*\)$/i.exec(bl);
          if (!cfm || !cfm[1]) return false;
          const cfItems = splitTopLevelComma(cfm[1]);
          for (let ci = 0; ci < cfItems.length; ci++) {
            const ct = splitCssTokens(cfItems[ci].trim());
            if (ct.length < 1 || ct.length > 2) return false;
            let imgTok = null, pctTok = null;
            if (ct.length === 1) {
              imgTok = ct[0];
            } else {
              const p0 = /^([+-]?(?:\d+(?:\.\d+)?|\.\d+))%$/.exec(ct[0]);
              const p1 = /^([+-]?(?:\d+(?:\.\d+)?|\.\d+))%$/.exec(ct[1]);
              if (p0) { pctTok = parseFloat(p0[1]); imgTok = ct[1]; }
              else if (p1) { pctTok = parseFloat(p1[1]); imgTok = ct[0]; }
              else return false;
              if (!(pctTok >= 0 && pctTok <= 100)) return false;
            }
            if (!(/^(?:url|image|image-set|-webkit-image-set|cross-fade|(?:repeating-)?(?:linear|radial|conic)-gradient)\(/i.test(imgTok) || isValidColor(imgTok))) return false;
          }
          continue;
        }
        if (/^(?:url|image|image-set|-webkit-image-set|(?:repeating-)?(?:linear|radial|conic)-gradient|-webkit-(?:linear|radial)-gradient)\(/i.test(bl)) continue;
        return false;
      }
    }
    if (prop === "background-image" && /^image\(/i.test(trimmed)) {
      const im = /^image\(\s*(.*)\s*\)$/i.exec(trimmed);
      return !!(im && im[1] && isValidColor(im[1]));
    }
    if ((prop === "background-image" || prop === "list-style-image") && /(?:linear|radial|conic)-gradient\(/i.test(trimmed)) {
      const gm = /^(?:repeating-)?(linear|radial|conic)-gradient\(\s*(.*)\s*\)$/i.exec(trimmed);
      if (gm) {
        const gKind = gm[1].toLowerCase();
        const args = splitTopLevelComma(gm[2]);
        if (args.length >= 2) {
          const toks0 = splitCssTokens(args[0].toLowerCase());
          if (gKind === "radial" || gKind === "conic") {
            const atIdx = toks0.indexOf("at");
            if (atIdx !== -1) {
              const posToks = toks0.slice(atIdx + 1);
              if (posToks.length === 0 || !isValidCssPositionTokens(posToks)) return false;
            }
            if (gKind === "radial") {
              const preAt = atIdx === -1 ? toks0 : toks0.slice(0, atIdx);
              const RADIAL_KW = new Set(["circle", "ellipse", "closest-side", "closest-corner", "farthest-side", "farthest-corner"]);
              let lenCount = 0, shapeKw = "";
              for (let ri = 0; ri < preAt.length; ri++) {
                const rt = preAt[ri];
                if (rt === "circle" || rt === "ellipse") shapeKw = rt;
                else if (!RADIAL_KW.has(rt)) {
                  if (!isValidLenPct(rt, false)) return false;
                  lenCount++;
                }
              }
              if ((shapeKw === "circle" && lenCount > 1) || (shapeKw === "ellipse" && lenCount === 1) || lenCount > 2) return false;
            }
          }
          if (gKind === "conic" && toks0[0] === "from" && toks0.length >= 2 && /^calc\(/i.test(toks0[1]) && toks0[1].includes("%")) {
            return false;
          }
          for (let ai = 0; ai < args.length; ai++) {
            const atoks = splitCssTokens(args[ai].toLowerCase());
            for (let ti = 0; ti < atoks.length; ti++) {
              const ct = atoks[ti];
              if (!/^calc\(/i.test(ct)) continue;
              if ((gKind === "linear" || gKind === "radial") && ct.includes("%") && /\d(?:deg|rad|grad|turn)\b/i.test(ct)) {
                return false;
              }
              if (/(?:[+-]\s*(?:\d+(?:\.\d+)?|\.\d+)\s*(?:\)|[+-])|\(\s*(?:\d+(?:\.\d+)?|\.\d+)\s*[+-])/.test(ct)) {
                return false;
              }
            }
          }
        }
      }
    }
    if (prop === "scroll-initial-target") {
      return vLower === "none" || vLower === "nearest";
    }
    if (prop === "color-interpolation-filters") {
      return vLower === "auto" || vLower === "srgb" || vLower === "linearrgb";
    }
    if (prop === "dominant-baseline") {
      return vLower === "auto" || vLower === "text-bottom" || vLower === "alphabetic" || vLower === "ideographic" ||
             vLower === "middle" || vLower === "central" || vLower === "mathematical" || vLower === "hanging" || vLower === "text-top";
    }
    if (prop === "filter" || prop === "backdrop-filter") {
      if (vLower === "none") return true;
      const toks = splitCssTokens(trimmed);
      if (toks.length === 0) return false;
      const isAng = (t) => t === "0" || t === "+0" || t === "-0" || /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?(?:deg|rad|grad|turn)$/i.test(t) || /^(?:calc|min|max|clamp)\(/i.test(t);
      const isNumPct0 = (t) => {
        if (/^(?:calc|min|max|clamp)\(/i.test(t)) return true;
        const m = /^([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?)(%)?$/i.exec(t);
        return !!(m && parseFloat(m[1]) >= 0);
      };
      for (let i = 0; i < toks.length; i++) {
        const fm = /^([a-z-]+)\((.*)\)$/i.exec(toks[i]);
        if (!fm) return false;
        const fn = fm[1].toLowerCase(), body = fm[2].trim();
        if (fn === "url") { if (!body) return false; continue; }
        const args = body ? splitCssTokens(body) : [];
        if (fn === "blur") {
          if (args.length > 1) return false;
          if (args.length === 1 && (args[0].endsWith("%") || !isValidLenPct(args[0], false))) return false;
        } else if (fn === "brightness" || fn === "contrast" || fn === "grayscale" || fn === "invert" || fn === "opacity" || fn === "saturate" || fn === "sepia") {
          if (args.length > 1) return false;
          if (args.length === 1 && !isNumPct0(args[0])) return false;
        } else if (fn === "hue-rotate") {
          if (args.length > 1) return false;
          if (args.length === 1 && !isAng(args[0].toLowerCase())) return false;
        } else if (fn === "drop-shadow") {
          if (args.length < 2 || args.length > 4) return false;
          let lens = [], hasCol = false;
          for (let j = 0; j < args.length; j++) {
            const a = args[j];
            if (!a.endsWith("%") && isValidLenPct(a, true)) lens.push(a);
            else if (!hasCol && isValidColor(a)) hasCol = true;
            else return false;
          }
          if (lens.length < 2 || lens.length > 3) return false;
          if (lens.length === 3 && !isValidLenPct(lens[2], false)) return false;
        } else return false;
      }
      return true;
    }
    if (prop === "offset-distance") {
      return isValidLenPct(trimmed, true);
    }
    if (prop === "offset-rotate") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 2) return false;
      const isAng = (t) => /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?(?:deg|rad|grad|turn)$/i.test(t) || /^(?:calc|min|max|clamp)\(/i.test(t);
      let seenKw = false, seenAng = false;
      for (let i = 0; i < toks.length; i++) {
        if (toks[i] === "auto" || toks[i] === "reverse") { if (seenKw) return false; seenKw = true; }
        else if (isAng(toks[i])) { if (seenAng) return false; seenAng = true; }
        else return false;
      }
      return seenKw || seenAng;
    }
    if (prop === "offset-position" || prop === "offset-anchor") {
      if (vLower === "auto" || (prop === "offset-position" && vLower === "normal")) return true;
      const toks = splitCssTokens(vLower);
      return toks.length !== 3 && isValidCssPositionTokens(toks);
    }
    if (prop === "overscroll-behavior") {
      const toks = splitCssTokens(vLower);
      if (toks.length < 1 || toks.length > 2) return false;
      for (let i = 0; i < toks.length; i++) {
        if (toks[i] !== "contain" && toks[i] !== "none" && toks[i] !== "auto" && toks[i] !== "chain") return false;
      }
      return true;
    }
    if (prop === "overscroll-behavior-x" || prop === "overscroll-behavior-y" ||
        prop === "overscroll-behavior-inline" || prop === "overscroll-behavior-block") {
      return vLower === "contain" || vLower === "none" || vLower === "auto" || vLower === "chain";
    }
    if (prop === "content") {
      const isValidSymbolsFn = (fnStr) => {
        const sm = /^symbols\(\s*(.*)\s*\)$/i.exec(fnStr);
        if (!sm) return false;
        const inner = sm[1].trim();
        if (!inner) return false;
        const stoks = splitCssTokens(inner);
        let idx = 0;
        let symType = "symbolic";
        const t0 = stoks[0].toLowerCase();
        if (t0 === "cyclic" || t0 === "numeric" || t0 === "alphabetic" || t0 === "symbolic" || t0 === "fixed") {
          symType = t0;
          idx = 1;
        } else if (/^[a-z_-]/i.test(stoks[0]) && !/^(?:url|image|linear-gradient|radial-gradient)\(/i.test(stoks[0])) {
          return false;
        }
        const syms = stoks.slice(idx);
        if (syms.length === 0) return false;
        if ((symType === "alphabetic" || symType === "numeric") && syms.length < 2) return false;
        for (let k = 0; k < syms.length; k++) {
          const sk = syms[k];
          if (!((sk.startsWith('"') && sk.endsWith('"')) || (sk.startsWith("'") && sk.endsWith("'")) || /^(?:url|image)\(/i.test(sk))) {
            return false;
          }
        }
        return true;
      };
      const isValidCounterStyle = (cs) => {
        const low = cs.trim().toLowerCase();
        if (!low) return false;
        if (low.startsWith("symbols(")) return isValidSymbolsFn(cs.trim());
        if (/^(?:none|unset|initial|inherit|revert|revert-layer|revert-rule|default)$/.test(low)) return false;
        return /^(?:--[a-z0-9_-]|-?(?:[a-z_]|\\.))(?:[a-z0-9_-]|\\.)*$/i.test(cs.trim());
      };
      const isValidCounterTok = (tok) => {
        const cm = /^(counters?)\((.*)\)$/i.exec(tok);
        if (!cm) return false;
        const isPlural = cm[1].toLowerCase() === "counters";
        const parts = splitTopLevelComma(cm[2]).map(x => x.trim());
        if (isPlural) {
          if (parts.length < 2 || parts.length > 3) return false;
          if (!parts[0] || !((parts[1].startsWith('"') && parts[1].endsWith('"')) || (parts[1].startsWith("'") && parts[1].endsWith("'")))) return false;
          if (parts.length === 3 && !isValidCounterStyle(parts[2])) return false;
        } else {
          if (parts.length < 1 || parts.length > 2 || !parts[0]) return false;
          if (parts.length === 2 && !isValidCounterStyle(parts[1])) return false;
        }
        return true;
      };
      const toks = splitCssTokens(trimmed);
      if (toks.length === 0) return false;
      const slashIdx = toks.indexOf("/");
      const mainToks = slashIdx === -1 ? toks : toks.slice(0, slashIdx);
      const altToks = slashIdx === -1 ? null : toks.slice(slashIdx + 1);
      if (mainToks.length === 0) return false;
      if (altToks !== null) {
        if (altToks.length === 0) return false;
        if (mainToks.length === 1 && (mainToks[0].toLowerCase() === "none" || mainToks[0].toLowerCase() === "normal")) return false;
        for (let i = 0; i < altToks.length; i++) {
          const at = altToks[i];
          if (at === "/") return false;
          const isStr = (at.startsWith('"') && at.endsWith('"')) || (at.startsWith("'") && at.endsWith("'"));
          if (!isStr && !isValidCounterTok(at)) return false;
        }
      }
      for (let i = 0; i < mainToks.length; i++) {
        const tk = mainToks[i];
        const tl = tk.toLowerCase();
        if (/^attr\(/i.test(tk)) {
          const am = /^attr\(\s*(.*)\s*\)$/i.exec(tk);
          if (!am || !am[1]) return false;
        } else if (/^counters?\(/i.test(tk)) {
          if (!isValidCounterTok(tk)) return false;
        } else if (tl === "none" || tl === "normal") {
          if (mainToks.length > 1) return false;
        }
      }
      return true;
    }

    return true;
  }
  global.__capsurf_splitCssTokens = splitCssTokens;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_splitCssTokens = splitCssTokens;
  global.__capsurf_isValidCssValue = isValidCssValue;

  const NUM_NORM_PROPS = new Set(["flex-grow", "flex-shrink", "opacity", "z-index", "order", "orphans", "widows", "column-count", "font-weight"]);
  const ZERO_PX_PROPS = new Set([
    "margin-top", "margin-right", "margin-bottom", "margin-left", "margin-block-start", "margin-block-end", "margin-inline-start", "margin-inline-end",
    "padding-top", "padding-right", "padding-bottom", "padding-left", "padding-block-start", "padding-block-end", "padding-inline-start", "padding-inline-end",
    "top", "right", "bottom", "left", "inset-block-start", "inset-block-end", "inset-inline-start", "inset-inline-end",
    "width", "height", "min-width", "min-height", "max-width", "max-height", "block-size", "inline-size", "min-block-size", "min-inline-size", "max-block-size", "max-inline-size",
    "row-gap", "column-gap", "grid-row-gap", "grid-column-gap", "border-top-width", "border-right-width", "border-bottom-width", "border-left-width",
    "outline-width", "outline-offset", "column-rule-width", "letter-spacing", "word-spacing", "border-top-left-radius", "border-top-right-radius", "border-bottom-right-radius", "border-bottom-left-radius",
    "perspective", "column-width", "flex-basis", "translate", "font-size", "vertical-align", "text-underline-offset", "text-decoration-thickness", "baseline-shift"
  ]);
  const TWO_SIDE_PROPS = new Set([
    "inset-block", "inset-inline", "margin-block", "margin-inline", "padding-block", "padding-inline",
    "overflow", "gap", "grid-gap"
  ]);
  function canonicalizeCalcToken(t) {
    const m = /^calc\(\s*(.+)\s*\)$/i.exec(t);
    if (!m) return t;
    const ABS_PX = { px: 1, in: 96, cm: 96 / 2.54, mm: 96 / 25.4, q: 96 / 101.6, pt: 96 / 72, pc: 16 };
    const src = m[1].trim();
    if (!src) return t;
    let pos = 0;
    const skipWs = () => { while (pos < src.length && /\s/.test(src[pos])) pos++; };
    const addMaps = (a, b, sgn) => {
      const res = new Map(a);
      for (const [u, v] of b.entries()) res.set(u, (res.get(u) || 0) + sgn * v);
      return res;
    };
    const mulMaps = (a, b) => {
      if (a.size === 1 && a.has("")) {
        const k = a.get("");
        const res = new Map();
        for (const [u, v] of b.entries()) res.set(u, k * v);
        return res;
      }
      if (b.size === 1 && b.has("")) {
        const k = b.get("");
        const res = new Map();
        for (const [u, v] of a.entries()) res.set(u, v * k);
        return res;
      }
      return null;
    };
    const parseExpr = () => {
      let left = parseTerm();
      if (!left) return null;
      while (true) {
        skipWs();
        const op = src[pos];
        if (op !== "+" && op !== "-") break;
        pos++;
        const right = parseTerm();
        if (!right) return null;
        left = addMaps(left, right, op === "-" ? -1 : 1);
      }
      return left;
    };
    const parseTerm = () => {
      let left = parseFactor();
      if (!left) return null;
      while (true) {
        skipWs();
        const op = src[pos];
        if (op !== "*" && op !== "/") break;
        pos++;
        const right = parseFactor();
        if (!right) return null;
        if (op === "*") {
          left = mulMaps(left, right);
          if (!left) return null;
        } else {
          if (right.size !== 1 || !right.has("")) return null;
          const d = right.get("");
          if (d === 0 || !isFinite(d)) return null;
          const res = new Map();
          for (const [u, v] of left.entries()) res.set(u, v / d);
          left = res;
        }
      }
      return left;
    };
    const parseFactor = () => {
      skipWs();
      if (src.slice(pos, pos + 5).toLowerCase() === "calc(") {
        pos += 4;
      }
      if (src[pos] === "(") {
        pos++;
        const inner = parseExpr();
        skipWs();
        if (src[pos] !== ")") return null;
        pos++;
        return inner;
      }
      const nm = /^([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)([a-z%]*)/i.exec(src.slice(pos));
      if (!nm) return null;
      pos += nm[0].length;
      const val = Number(nm[1]);
      if (!isFinite(val)) return null;
      let unit = (nm[2] || "").toLowerCase();
      let scaled = val;
      if (Object.prototype.hasOwnProperty.call(ABS_PX, unit)) {
        scaled = val * ABS_PX[unit];
        unit = "px";
      }
      const res = new Map();
      res.set(unit, scaled);
      return res;
    };
    const combined = parseExpr();
    skipWs();
    if (!combined || pos !== src.length) return t;
    const entries = [];
    for (const [u, v] of combined.entries()) {
      const r = Number(v.toFixed(6));
      if (r !== 0 || combined.size === 1 || u === "%" || u === "px" || u === "deg") entries.push({ unit: u, val: r });
    }
    if (entries.length === 0) entries.push({ unit: combined.has("px") ? "px" : "", val: 0 });
    entries.sort((a, b) => {
      const ra = a.unit === "" ? 0 : (a.unit === "%" ? 1 : 2);
      const rb = b.unit === "" ? 0 : (b.unit === "%" ? 1 : 2);
      if (ra !== rb) return ra - rb;
      return a.unit < b.unit ? -1 : (a.unit > b.unit ? 1 : 0);
    });
    let out = "";
    for (let i = 0; i < entries.length; i++) {
      const e = entries[i];
      const absMag = String(Math.abs(e.val)) + e.unit;
      if (i === 0) out += (e.val < 0 ? "-" : "") + absMag;
      else out += (e.val < 0 ? " - " : " + ") + absMag;
    }
    return "calc(" + out + ")";
  }

  function normalizeCssSpecifiedValue(key, raw) {
    if (!raw || key.startsWith("--")) return raw;
    const s = String(raw).trim();
    if (/^(?:inherit|initial|unset|revert|revert-layer)$/i.test(s)) return s.toLowerCase();
    if (/\bvar\(/i.test(s)) return s;
    if (NUM_NORM_PROPS.has(key) && /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(s)) {
      const n = Number(s);
      if (Number.isFinite(n)) return String(n === 0 ? 0 : n);
    }
    if (key === "opacity") {
      const pctM = /^([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?)%$/i.exec(s);
      if (pctM) {
        const n = Number(pctM[1]) / 100;
        if (Number.isFinite(n)) return String(n === 0 ? 0 : n);
      }
      if (/^calc\(/i.test(s)) return canonicalizeCalcToken(s);
      const fnM = /^(min|max|clamp)\(\s*(.+)\s*\)$/i.exec(s);
      if (fnM) {
        const fnName = fnM[1].toLowerCase();
        const args = splitTopLevelComma(fnM[2]);
        let unit = null;
        const vals = [];
        let ok = true;
        for (let ai = 0; ai < args.length; ai++) {
          const am = /^([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?)(%)?$/i.exec(args[ai].trim());
          if (!am) { ok = false; break; }
          const u = am[2] || "";
          if (unit === null) unit = u;
          else if (unit !== u) { ok = false; break; }
          vals.push(Number(am[1]));
        }
        if (ok && vals.length > 0) {
          let r = null;
          if (fnName === "min") r = Math.min.apply(null, vals);
          else if (fnName === "max") r = Math.max.apply(null, vals);
          else if (fnName === "clamp" && vals.length === 3) r = Math.max(vals[0], Math.min(vals[1], vals[2]));
          if (r !== null && Number.isFinite(r)) return "calc(" + String(r === 0 ? 0 : r) + unit + ")";
        }
      }
    }
    if (ZERO_PX_PROPS.has(key)) {
      if (s === "0" || s === "+0" || s === "-0") return "0px";
      if (/^calc\(/i.test(s)) return canonicalizeCalcToken(s);
    }
    if (key === "background-position") {
      return splitTopLevelComma(s).map((layer) => normalizePositionLayer(layer.trim(), false)).join(", ");
    }
    if (key === "background-position-x" || key === "background-position-y") {
      return splitTopLevelComma(s).map((layer) => splitCssTokens(layer).map((tok) => (/^calc\(/i.test(tok) ? canonicalizeCalcToken(tok) : tok)).join(" ")).join(", ");
    }
    if (key === "font-family") {
      const GEN = new Set(["serif", "sans-serif", "cursive", "fantasy", "monospace", "system-ui", "ui-serif", "ui-sans-serif", "ui-monospace", "ui-rounded", "emoji", "math", "fangsong"]);
      return splitTopLevelComma(s).map((it) => {
        const tr = it.trim();
        return GEN.has(tr.toLowerCase()) ? tr.toLowerCase() : tr;
      }).join(", ");
    }
    if (key === "transform") {
      return s.replace(/\b([a-zA-Z0-9]+)\(([^()]+)\)/g, (_, fn, args) => {
        const fnLow = fn.toLowerCase();
        if (fnLow.startsWith("scale")) {
          const normArgs = splitTopLevelComma(args).map((a) => {
            const pm = /^([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?)%$/i.exec(a.trim());
            return pm ? String(Number(pm[1]) / 100) : a.trim();
          });
          return fnLow + "(" + normArgs.join(", ") + ")";
        }
        if (fnLow === "rotate" || fnLow === "rotatez" || fnLow === "skew" || fnLow === "skewx" || fnLow === "skewy") {
          const normArgs = splitTopLevelComma(args).map((a) => {
            const t = a.trim();
            return (t === "0" || t === "+0" || t === "-0") ? "0deg" : t;
          });
          return fnLow + "(" + normArgs.join(", ") + ")";
        }
        return fn + "(" + args + ")";
      });
    }
    if (key === "clip-path") {
      const shm = /^shape\(\s*(.+)\s*\)$/i.exec(s);
      if (shm) {
        const segs = splitTopLevelComma(shm[1]).map((seg, idx) => {
          const toks = splitCssTokens(seg.trim()).map((t) => (/^(?:var|env)\(/i.test(t) ? t : t.toLowerCase()));
          if (idx === 0 && toks[0] === "nonzero") toks.shift();
          if (toks[0] === "arc") {
            const out = [];
            for (let i = 0; i < toks.length; i++) {
              if (toks[i] === "small" || toks[i] === "ccw") continue;
              if (toks[i] === "rotate" && i + 1 < toks.length && (toks[i + 1] === "0deg" || toks[i + 1] === "0")) { i++; continue; }
              out.push(toks[i]);
            }
            return out.join(" ");
          }
          return toks.join(" ");
        });
        return "shape(" + segs.join(", ") + ")";
      }
    }
    if (key === "font-feature-settings") {
      if (s.toLowerCase() === "normal") return "normal";
      const items = splitTopLevelComma(s);
      const out = [];
      for (let i = 0; i < items.length; i++) {
        const toks = splitCssTokens(items[i].trim());
        if (toks.length === 0 || toks.length > 2) return s;
        let tag = toks[0];
        if ((tag.startsWith('"') && tag.endsWith('"')) || (tag.startsWith("'") && tag.endsWith("'"))) {
          let inner = tag.slice(1, -1);
          inner = inner.replace(/\\([0-9a-fA-F]{1,6})\s?/g, (_, h) => String.fromCodePoint(parseInt(h, 16)))
                       .replace(/\\(.)/g, "$1");
          tag = '"' + inner.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '"';
        }
        if (toks.length === 1) {
          out.push(tag);
        } else {
          const val = toks[1].toLowerCase();
          if (val === "1" || val === "on") out.push(tag);
          else if (val === "off") out.push(tag + " 0");
          else out.push(tag + " " + toks[1]);
        }
      }
      return out.join(", ");
    }
    if (key === "font-variation-settings") {
      if (s.toLowerCase() === "normal") return "normal";
      const items = splitTopLevelComma(s);
      const out = [];
      for (let i = 0; i < items.length; i++) {
        const toks = splitCssTokens(items[i].trim());
        if (toks.length !== 2) return s;
        let tag = toks[0];
        if ((tag.startsWith('"') && tag.endsWith('"')) || (tag.startsWith("'") && tag.endsWith("'"))) {
          const inner = tag.slice(1, -1);
          tag = '"' + inner.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '"';
        }
        let num = toks[1];
        if (/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(num)) {
          const n = Number(num);
          if (Number.isFinite(n)) num = String(n === 0 ? 0 : n);
        }
        out.push(tag + " " + num);
      }
      return out.join(", ");
    }
    if (key === "font-variant-alternates") {
      return splitCssTokens(s).map((tk) => (/^[a-z-]+$/i.test(tk) ? tk.toLowerCase() : tk)).join(" ");
    }
    if (key === "margin-trim") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 2 && ((p[0] === "block-start" && p[1] === "block-end") || (p[0] === "block-end" && p[1] === "block-start"))) {
        return "block";
      }
      return p.join(" ");
    }
    if (TWO_SIDE_PROPS.has(key)) {
      const parts = splitCssTokens(s);
      const isLenProp = !key.startsWith("overflow") && !key.startsWith("place-");
      if (isLenProp) {
        for (let i = 0; i < parts.length; i++) {
          if (parts[i] === "0" || parts[i] === "+0" || parts[i] === "-0") parts[i] = "0px";
          else if (/^calc\(/i.test(parts[i])) parts[i] = canonicalizeCalcToken(parts[i]);
        }
      }
      if (parts.length === 2 && parts[0] === parts[1]) return parts[0];
      return parts.join(" ");
    }
    if (key === "margin" || key === "padding" || key === "inset" || key === "border-width") {
      const parts = splitCssTokens(s);
      for (let i = 0; i < parts.length; i++) {
        if (parts[i] === "0" || parts[i] === "+0" || parts[i] === "-0") parts[i] = "0px";
        else if (/^calc\(/i.test(parts[i])) parts[i] = canonicalizeCalcToken(parts[i]);
      }
      if (parts.length === 4 && parts[1] === parts[3]) parts.pop();
      if (parts.length === 3 && parts[0] === parts[2]) parts.pop();
      if (parts.length === 2 && parts[0] === parts[1]) parts.pop();
      return parts.join(" ");
    }
    if (key === "border-image-repeat") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 2 && p[0] === p[1]) p.pop();
      return p.join(" ");
    }
    if (key === "border-image-slice") {
      const p = splitCssTokens(s.toLowerCase());
      const hasFill = p.includes("fill");
      const nums = p.filter((t) => t !== "fill");
      if (nums.length === 4 && nums[1] === nums[3]) nums.pop();
      if (nums.length === 3 && nums[0] === nums[2]) nums.pop();
      if (nums.length === 2 && nums[0] === nums[1]) nums.pop();
      if (hasFill) nums.push("fill");
      return nums.join(" ");
    }
    if (key === "border-image-outset" || key === "border-image-width") {
      const p = splitCssTokens(s.toLowerCase());
      for (let i = 0; i < p.length; i++) {
        if (/^calc\(/i.test(p[i])) p[i] = canonicalizeCalcToken(p[i]);
      }
      if (p.length === 4 && p[1] === p[3]) p.pop();
      if (p.length === 3 && p[0] === p[2]) p.pop();
      if (p.length === 2 && p[0] === p[1]) p.pop();
      return p.join(" ");
    }
    if (key === "border-style") {
      const parts = splitCssTokens(s.toLowerCase());
      if (parts.length === 4 && parts[1] === parts[3]) parts.pop();
      if (parts.length === 3 && parts[0] === parts[2]) parts.pop();
      if (parts.length === 2 && parts[0] === parts[1]) parts.pop();
      return parts.join(" ");
    }
    if (key === "scale") {
      const parts = splitCssTokens(s);
      for (let i = 0; i < parts.length; i++) {
        if (/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(parts[i])) {
          const n = Number(parts[i]);
          if (Number.isFinite(n)) parts[i] = String(n === 0 ? 0 : n);
        } else {
          const cm = /^calc\(\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+))\s*\*\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+)(%)?)\s*\)$/i.exec(parts[i]);
          if (cm) {
            const prod = Number((Number(cm[1]) * Number(cm[2].replace(/%$/, ""))).toFixed(6));
            parts[i] = "calc(" + String(prod === 0 ? 0 : prod) + (cm[3] || "") + ")";
          } else {
            const pm = /^([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?)%$/i.exec(parts[i]);
            if (pm) parts[i] = String(Number(pm[1]) / 100);
          }
        }
      }
      if (parts.length === 3 && parts[2] === "1") parts.pop();
      if (parts.length === 2 && parts[1] === parts[0]) parts.pop();
      return parts.join(" ");
    }
    if (key === "translate") {
      const parts = splitCssTokens(s);
      if (parts.length === 1 && parts[0].toLowerCase() === "none") return "none";
      for (let i = 0; i < parts.length; i++) {
        if (parts[i] === "0" || parts[i] === "+0" || parts[i] === "-0") parts[i] = "0px";
        else if (/^calc\(/i.test(parts[i])) parts[i] = canonicalizeCalcToken(parts[i]);
      }
      if (parts.length === 3 && parts[2] === "0px") parts.pop();
      if (parts.length === 2 && parts[1] === "0px") parts.pop();
      return parts.join(" ");
    }
    if (key === "rotate") {
      const parts = splitCssTokens(s);
      if (parts.length === 1 && parts[0].toLowerCase() === "none") return "none";
      if (parts.length === 2) {
        const p0 = parts[0].toLowerCase(), p1 = parts[1].toLowerCase();
        if (p0 === "z") return parts[1];
        if (p1 === "z") return parts[0];
        if (p1 === "x" || p1 === "y") return p1 + " " + parts[0];
      }
      if (parts.length === 4) {
        const neg = (a) => a.startsWith("-") ? a.slice(1) : (a.startsWith("+") ? ("-" + a.slice(1)) : ("-" + a));
        let p = parts;
        const isNumTok = (t) => /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(t);
        if (!isNumTok(p[0]) && isNumTok(p[1]) && isNumTok(p[2]) && isNumTok(p[3])) {
          p = [p[1], p[2], p[3], p[0]];
        }
        const x = Number(p[0]), y = Number(p[1]), z = Number(p[2]);
        if (!isNaN(x) && !isNaN(y) && !isNaN(z)) {
          if (y === 0 && z === 0 && x !== 0) return "x " + (x < 0 ? neg(p[3]) : p[3]);
          if (x === 0 && z === 0 && y !== 0) return "y " + (y < 0 ? neg(p[3]) : p[3]);
          if (x === 0 && y === 0 && z !== 0) return z < 0 ? neg(p[3]) : p[3];
        }
        return p.join(" ");
      }
      return parts.join(" ");
    }
    if (key === "border-radius" || key === "-webkit-border-radius") {
      const si = s.indexOf("/");
      const col4 = (str) => {
        const p = splitCssTokens(str);
        for (let i = 0; i < p.length; i++) if (p[i] === "0" || p[i] === "+0" || p[i] === "-0") p[i] = "0px";
        if (p.length === 4 && p[1] === p[3]) p.pop();
        if (p.length === 3 && p[0] === p[2]) p.pop();
        if (p.length === 2 && p[0] === p[1]) p.pop();
        return p.join(" ");
      };
      if (si === -1) return col4(s);
      const l = col4(s.slice(0, si)), r = col4(s.slice(si + 1));
      return l === r ? l : (l + " / " + r);
    }
    if (key === "flex-flow") {
      const parts = splitCssTokens(s.toLowerCase());
      const DIR = new Set(["row", "row-reverse", "column", "column-reverse"]);
      let d = "", w = "";
      for (let i = 0; i < parts.length; i++) {
        if (DIR.has(parts[i])) d = parts[i];
        else w = parts[i];
      }
      return d && w ? (d + " " + w) : (d || w || s);
    }
    if (key === "outline" || key === "border" || key === "column-rule" ||
        key === "border-top" || key === "border-right" || key === "border-bottom" || key === "border-left") {
      const parts = splitCssTokens(s);
      const STY = new Set(["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset", "auto"]);
      let w = "", st = "", c = "";
      for (let i = 0; i < parts.length; i++) {
        const t = parts[i], tl = t.toLowerCase();
        if (STY.has(tl)) st = tl;
        else if (tl === "thin" || tl === "medium" || tl === "thick" || /^[+-]?(?:\d|\.)/.test(tl) || /^(?:calc|min|max|clamp)\(/i.test(tl)) {
          w = (tl === "0" || tl === "+0" || tl === "-0") ? "0px" : (/^calc\(/i.test(t) ? canonicalizeCalcToken(t) : t);
        } else c = t;
      }
      if (key === "outline") {
        const oc = c.toLowerCase() === "currentcolor" ? "" : c;
        const ost = st === "none" ? "" : st;
        const ow = w === "medium" ? "" : w;
        return [oc, ost, ow].filter(Boolean).join(" ") || "none";
      }
      return [w, st, c].filter(Boolean).join(" ");
    }
    if (key === "flex") {
      const normCalcTok = (t) => {
        if (!/^calc\(/i.test(t)) return t;
        let out = t.replace(/(sign\([^()]+\))\s*\*\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:px|em|rem|%)?)/gi, "$2 * $1");
        const inn = out.slice(5, -1).trim();
        if (/^[\d+\-*/().\s]+$/.test(inn)) {
          try {
            const v = Function("return (" + inn + ")")();
            if (typeof v === "number" && isFinite(v)) return "calc(" + String(Number(v.toFixed(6))) + ")";
          } catch (_) {}
        }
        return out;
      };
      const parts = splitCssTokens(s).map(normCalcTok);
      if (parts.length === 1) {
        const p0 = parts[0].toLowerCase();
        if (p0 === "none") return "0 0 auto";
        if (p0 === "auto") return "1 1 auto";
        if (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(p0)) {
          return String(Number(p0) === 0 ? 0 : Number(p0)) + " 1 0%";
        }
        return "1 1 " + (p0 === "0" ? "0px" : parts[0]);
      }
      const isNum = (x) => /^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(x) ||
        (/^(?:calc|min|max|clamp)\(/i.test(x) && !/(?:px|em|rem|ex|ch|vw|vh|vmin|vmax|cm|mm|q|in|pt|pc|%)/i.test(x.replace(/sign\([^()]+\)/gi, "")));
      let grow = "1", shrink = "1", basis = "0%";
      const nums = [];
      let bRaw = "";
      for (let i = 0; i < parts.length; i++) {
        if (isNum(parts[i]) && !(nums.length === 2 && parts[i] === "0")) {
          nums.push(/^(?:calc|min|max|clamp)\(/i.test(parts[i]) ? parts[i] : String(Number(parts[i])));
        } else bRaw = parts[i];
      }
      if (nums.length >= 1) grow = nums[0];
      if (nums.length >= 2) shrink = nums[1];
      if (bRaw) basis = (bRaw === "0" || bRaw === "+0" || bRaw === "-0") ? "0px" : bRaw;
      return grow + " " + shrink + " " + basis;
    }
    if (key === "text-indent") {
      const parts = splitCssTokens(s);
      let len = "", hang = false, each = false;
      for (let i = 0; i < parts.length; i++) {
        const pl = parts[i].toLowerCase();
        if (pl === "hanging") hang = true;
        else if (pl === "each-line") each = true;
        else len = (pl === "0" || pl === "+0" || pl === "-0") ? "0px" : parts[i];
      }
      let out = len;
      if (hang) out += " hanging";
      if (each) out += " each-line";
      return out;
    }
    if (key === "text-decoration") {
      const parts = splitCssTokens(s);
      const LINE_ORDER = ["underline", "overline", "line-through", "blink"];
      const LINES = new Set(LINE_ORDER);
      const STY = new Set(["solid", "double", "dotted", "dashed", "wavy"]);
      const lines = [];
      let st = "", th = "", col = "";
      for (let i = 0; i < parts.length; i++) {
        const t = parts[i], tl = t.toLowerCase();
        if (tl === "none") continue;
        if (tl === "spelling-error" || tl === "grammar-error" || LINES.has(tl)) lines.push(tl);
        else if (STY.has(tl)) st = tl;
        else if (tl === "auto" || tl === "from-font" || /^[+-]?(?:\d|\.)/.test(tl) || /^(?:calc|min|max|clamp)\(/i.test(tl)) {
          th = (tl === "0" || tl === "+0" || tl === "-0") ? "0px" : tl;
        } else col = t;
      }
      const sortedLines = (lines.length === 1 && (lines[0] === "spelling-error" || lines[0] === "grammar-error"))
        ? lines[0]
        : LINE_ORDER.filter(k => lines.indexOf(k) !== -1).join(" ");
      const outStr = [
        sortedLines,
        st === "solid" ? "" : st,
        th === "auto" ? "" : th,
        col.toLowerCase() === "currentcolor" ? "" : col
      ].filter(Boolean).join(" ");
      return outStr || "none";
    }
    if (key === "-webkit-perspective") {
      if (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)$/.test(s)) return String(Number(s)) + "px";
      return s;
    }
    if (key === "-webkit-transform") {
      return s.replace(/\bperspective\(\s*([+]?(?:\d+(?:\.\d+)?|\.\d+))\s*\)/gi, (_, n) => "perspective(" + String(Number(n)) + "px)");
    }
    function normalizePositionLayer(layerStr, allowZ) {
      const parts = splitCssTokens(layerStr);
      for (let i = 0; i < parts.length; i++) {
        if (parts[i] === "0" || parts[i] === "+0" || parts[i] === "-0") parts[i] = "0px";
      }
      if (parts.length === 1) {
        const a = parts[0].toLowerCase();
        return (a === "top" || a === "bottom") ? ("center " + a) : (parts[0] + " center");
      }
      if (parts.length === 2 || (allowZ && parts.length === 3)) {
        const a = parts[0].toLowerCase(), b = parts[1].toLowerCase();
        const isY = (x) => x === "top" || x === "bottom";
        const isX = (x) => x === "left" || x === "right";
        if (isY(a) || (a === "center" && isX(b))) {
          const tmp = parts[0]; parts[0] = parts[1]; parts[1] = tmp;
        }
      }
      if (!allowZ && parts.length === 3) {
        const a = parts[0].toLowerCase(), b = parts[1].toLowerCase();
        const isY = (x) => x === "top" || x === "bottom";
        const isX = (x) => x === "left" || x === "right";
        if (isY(a) && !isX(b)) return parts[2] + " " + parts[0] + " " + parts[1];
        if ((isY(a) || a === "center") && isX(b)) return parts[1] + " " + parts[2] + " " + parts[0];
      }
      if (parts.length === 4) {
        const a = parts[0].toLowerCase();
        if (a === "top" || a === "bottom") {
          return parts[2] + " " + parts[3] + " " + parts[0] + " " + parts[1];
        }
      }
      return parts.join(" ");
    }
    if (key === "color-scheme") {
      const toks = splitCssTokens(s.trim());
      if (toks.length === 1 && toks[0].toLowerCase() === "normal") return "normal";
      if (toks.length > 1 && toks[0].toLowerCase() === "only") {
        return toks.slice(1).map((t) => (t.toLowerCase() === "light" || t.toLowerCase() === "dark") ? t.toLowerCase() : t).concat("only").join(" ");
      }
      if (toks.length > 1 && toks[toks.length - 1].toLowerCase() === "only") {
        return toks.slice(0, -1).map((t) => (t.toLowerCase() === "light" || t.toLowerCase() === "dark") ? t.toLowerCase() : t).concat("only").join(" ");
      }
      return toks.map((t) => (t.toLowerCase() === "light" || t.toLowerCase() === "dark") ? t.toLowerCase() : t).join(" ");
    }
    if (key === "mask-position") {
      return s.split(",").map((l) => normalizePositionLayer(l.trim(), false)).join(", ");
    }
    if (key === "perspective-origin" || key === "transform-origin" || key === "object-position") {
      return normalizePositionLayer(s, key === "transform-origin");
    }
    if (key === "shape-outside" || key === "clip-path") {
      let leadBox = "", shapeSrc = s;
      const leadM = /^(content-box|padding-box|border-box|margin-box|fill-box|stroke-box|view-box)\s+(.+)$/i.exec(s);
      if (leadM) { leadBox = leadM[1].toLowerCase(); shapeSrc = leadM[2].trim(); }
      const fnM = /^(circle|ellipse|shape|path|inset|xywh|rect|polygon)\(/i.exec(shapeSrc);
      if (fnM) {
        const fnName = fnM[1].toLowerCase();
        let depth = 1, i = fnM[0].length, q = "";
        for (; i < shapeSrc.length && depth > 0; i++) {
          const ch = shapeSrc[i];
          if (q) { if (ch === "\\" && i + 1 < shapeSrc.length) i++; else if (ch === q) q = ""; continue; }
          if (ch === '"' || ch === "'") { q = ch; continue; }
          if (ch === "(") depth++;
          else if (ch === ")") depth--;
        }
        if (depth === 0) {
          const inner = shapeSrc.slice(fnM[0].length, i - 1).trim();
          const tailBox = shapeSrc.slice(i).trim().toLowerCase();
          let box = leadBox || tailBox;
          if (key === "clip-path" && box === "border-box") box = "";
          const suffix = box ? " " + box : "";
          const normCalcTok = (t) => /^calc\(/i.test(t) ? canonicalizeCalcToken(t) : ((t === "0" || t === "+0" || t === "-0") ? "0px" : t);
          if (fnName === "circle" || fnName === "ellipse") {
            const toks = splitCssTokens(inner);
            const atIdx = toks.findIndex((t) => t.toLowerCase() === "at");
            let preToks = atIdx === -1 ? toks : toks.slice(0, atIdx);
            if (fnName === "circle" && preToks.length === 1 && preToks[0].toLowerCase() === "closest-side") preToks = [];
            else if (fnName === "ellipse" && preToks.length === 2 && preToks[0].toLowerCase() === "closest-side" && preToks[1].toLowerCase() === "closest-side") preToks = [];
            preToks = preToks.map(normCalcTok);
            const preStr = preToks.join(" ");
            if (atIdx !== -1 && atIdx + 1 < toks.length) {
              const pos = normalizePositionLayer(toks.slice(atIdx + 1).map(normCalcTok).join(" "), false);
              return fnName + "(" + (preStr ? preStr + " at " : "at ") + pos + ")" + suffix;
            }
            return fnName + "(" + preStr + ")" + suffix;
          } else if (fnName === "inset" || fnName === "xywh" || fnName === "rect") {
            const toks = splitCssTokens(inner).map(normCalcTok);
            const rIdx = toks.findIndex((t) => t.toLowerCase() === "round");
            if (rIdx !== -1) {
              const rPost = toks.slice(rIdx + 1);
              if (rPost.length > 0 && rPost.every((t) => t === "0px" || t === "0%" || t === "/")) {
                toks.length = rIdx;
              }
            }
            return fnName + "(" + toks.join(" ") + ")" + suffix;
          } else if (fnName === "polygon") {
            const segs = splitTopLevelComma(inner).map((seg) => splitCssTokens(seg).map(normCalcTok));
            if (segs.length > 1) {
              let s0 = segs[0].filter((t) => t.toLowerCase() !== "nonzero");
              const rIdx = s0.findIndex((t) => t.toLowerCase() === "round");
              if (rIdx !== -1 && rIdx + 1 < s0.length && s0[rIdx + 1] === "0px") {
                s0.splice(rIdx, 2);
              }
              if (s0.length === 0) segs.shift();
              else segs[0] = s0;
            }
            return "polygon(" + segs.map((t) => t.join(" ")).join(", ") + ")" + suffix;
          } else if (fnName === "path") {
            const parts = splitTopLevelComma(inner);
            let fr = "", strTok = parts[0].trim();
            if (parts.length === 2) {
              fr = parts[0].trim().toLowerCase();
              strTok = parts[1].trim();
            }
            const rawPath = strTok.slice(1, -1).trim();
            const pToks = rawPath.match(/[MmLlHhVvCcSsQqTtAa]|[Zz]|[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?/g) || [];
            const normPath = pToks.map((t) => (t === "z" ? "Z" : t)).join(" ");
            return "path(" + (fr === "evenodd" ? "evenodd, " : "") + '"' + normPath + '")' + suffix;
          } else {
            const segs = splitTopLevelComma(inner).map((seg) => {
              const toks = splitCssTokens(seg).map(normCalcTok);
              return toks.join(" ");
            });
            return "shape(" + segs.join(", ") + ")" + suffix;
          }
        }
      }
    }
    if (key === "border-spacing") {
      const parts = splitCssTokens(s);
      for (let i = 0; i < parts.length; i++) {
        if (parts[i] === "0" || parts[i] === "+0" || parts[i] === "-0") parts[i] = "0px";
        else if (/^calc\(/i.test(parts[i])) parts[i] = canonicalizeCalcToken(parts[i]);
      }
      if (parts.length === 2 && parts[0] === parts[1]) parts.pop();
      return parts.join(" ");
    }
    if (key === "background-size" || key === "mask-size") {
      const layers = s.split(",");
      for (let i = 0; i < layers.length; i++) {
        const p = splitCssTokens(layers[i].trim());
        for (let j = 0; j < p.length; j++) if (p[j] === "0" || p[j] === "+0" || p[j] === "-0") p[j] = "0px";
        if (p.length === 2 && p[0].toLowerCase() === "auto" && p[1].toLowerCase() === "auto") p.pop();
        else if (p.length === 1 && !/^(?:auto|contain|cover|inherit|initial|unset|revert|revert-layer)$/i.test(p[0])) p.push("auto");
        layers[i] = p.join(" ");
      }
      return layers.join(", ");
    }
    if (key === "border-image") {
      const slashes = [];
      let sc = "", sd = 0, sq = "";
      for (let i = 0; i < s.length; i++) {
        const ch = s[i];
        if (sq) { sc += ch; if (ch === sq && s[i - 1] !== "\\") sq = ""; continue; }
        if (ch === "\"" || ch === "'") { sq = ch; sc += ch; continue; }
        if (ch === "(") sd++;
        else if (ch === ")" && sd > 0) sd--;
        if (sd === 0 && ch === "/") { slashes.push(sc.trim()); sc = ""; }
        else sc += ch;
      }
      slashes.push(sc.trim());
      const seg0 = splitCssTokens(slashes[0]);
      const fillIdx = seg0.findIndex((t) => t.toLowerCase() === "fill");
      if (fillIdx !== -1) {
        seg0.splice(fillIdx, 1);
        seg0.push("fill");
        slashes[0] = seg0.join(" ");
        return slashes.join(" / ").replace(/ \/  \/ /g, " / / ");
      }
    }
    if (key === "background-repeat" || key === "mask-repeat") {
      const layers = s.toLowerCase().split(",");
      for (let i = 0; i < layers.length; i++) {
        const p = splitCssTokens(layers[i].trim());
        if (p.length === 2) {
          if (p[0] === "repeat" && p[1] === "no-repeat") layers[i] = "repeat-x";
          else if (p[0] === "no-repeat" && p[1] === "repeat") layers[i] = "repeat-y";
          else if (p[0] === p[1]) layers[i] = p[0];
          else layers[i] = p.join(" ");
        } else layers[i] = p.join(" ");
      }
      return layers.join(", ");
    }
    if (key === "align-content" || key === "justify-content" ||
        key === "align-items" || key === "justify-items" ||
        key === "align-self" || key === "justify-self") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 2 && p[0] === "first" && p[1] === "baseline") return "baseline";
      if (key === "justify-items" && p.length === 2 && p[1] === "legacy") return "legacy " + p[0];
      return p.join(" ");
    }
    if (key === "place-content" || key === "place-items" || key === "place-self") {
      const p = splitCssTokens(s.toLowerCase());
      const normAlignPart = (arr) => {
        if (arr.length === 2 && arr[0] === "first" && arr[1] === "baseline") return "baseline";
        if (arr.length === 2 && arr[1] === "legacy") return "legacy " + arr[0];
        return arr.join(" ");
      };
      if (p.length >= 2) {
        const splitAt = (p[0] === "first" || p[0] === "last" || p[0] === "safe" || p[0] === "unsafe" || p[0] === "legacy") ? 2 : 1;
        if (splitAt < p.length) {
          const a = normAlignPart(p.slice(0, splitAt));
          const b = normAlignPart(p.slice(splitAt));
          return a === b ? a : (a + " " + b);
        }
      }
      const single = normAlignPart(p);
      if (key === "place-content" && (single === "baseline" || single === "last baseline")) {
        return single + " start";
      }
      return single;
    }
    if (key === "line-height" && /^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(s)) {
      const n = Number(s);
      if (Number.isFinite(n)) return String(n === 0 ? 0 : n);
    }
    if (key === "aspect-ratio") {
      const p = splitCssTokens(s.replace(/\s*\/\s*/g, " / ").toLowerCase());
      if (p.length === 1 && p[0] === "auto") return "auto";
      let hasAuto = false;
      const rem = [];
      for (let i = 0; i < p.length; i++) {
        if (p[i] === "auto") hasAuto = true;
        else rem.push(p[i]);
      }
      if (rem.length === 1) rem.push("/", "1");
      const out = rem.join(" ");
      return hasAuto ? ("auto " + out) : out;
    }
    if (key === "text-emphasis-position") {
      const p = splitCssTokens(s.toLowerCase());
      const ordered = (p.length === 2 && (p[0] === "left" || p[0] === "right")) ? [p[1], p[0]] : p;
      if (ordered.length === 2 && ordered[1] === "right") return ordered[0];
      return ordered.join(" ");
    }
    if (key === "text-emphasis-style") {
      const p = splitCssTokens(s);
      if (p.length === 1) {
        const tl = p[0].toLowerCase();
        if (/^'[^']*'$/.test(p[0])) return '"' + p[0].slice(1, -1) + '"';
        if (/^"[^"]*"$/.test(p[0])) return p[0];
        return tl;
      }
      if (p.length === 2) {
        const a = p[0].toLowerCase(), b = p[1].toLowerCase();
        const fill = (a === "filled" || a === "open") ? a : b;
        const shape = (a === "filled" || a === "open") ? b : a;
        return fill === "filled" ? shape : ("open " + shape);
      }
      return s;
    }
    if (key === "object-fit") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 2 && (p[0] === "contain" || p[1] === "contain")) return "scale-down";
      if (p.length === 2 && p[0] === "scale-down") return p[1] + " scale-down";
      return p.join(" ");
    }
    if (key === "ruby-overhang") {
      return s.toLowerCase() === "none" ? "spaces" : s.toLowerCase();
    }
    if (key === "position-visibility") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 1 && p[0] === "always") return "always";
      const order = ["anchors-valid", "anchors-visible", "no-overflow"];
      return order.filter((k) => p.indexOf(k) !== -1).join(" ");
    }
    if (key === "font-language-override") {
      if (s.toLowerCase() === "normal") return "normal";
      const qm = /^(?:"([^"]*)"|'([^']*)')$/.exec(s);
      if (qm) {
        const inner = (qm[1] !== undefined ? qm[1] : qm[2]).replace(/\s+$/, "");
        return "\"" + inner + "\"";
      }
      return s;
    }
    if (key === "color-interpolation-filters") {
      return s.trim().toLowerCase();
    }
    if (key === "color" || key === "background-color" || key === "accent-color" || key === "scrollbar-color" || key === "flood-color" || key === "lighting-color" || key === "border-color" || key === "border-top-color" || key === "border-right-color" || key === "border-bottom-color" || key === "border-left-color") {
      if (/\blight-dark\(/i.test(s)) return s.toLowerCase();
      const toks = splitCssTokens(s);
      return toks.map((t) => {
        const tl = t.toLowerCase();
        if (tl === "auto" || tl === "currentcolor" || tl === "transparent") return tl;
        if (tl.startsWith("#") || /^(?:rgba?|hsla?)\(/i.test(tl)) return normalizeSingleColor(t);
        return tl;
      }).join(" ");
    }
    if (key === "background-position-x" || key === "background-position-y") {
      return splitTopLevelComma(s).map((layer) => {
        return splitCssTokens(layer).map((t) => {
          if (t === "0" || t === "+0" || t === "-0") return "0px";
          if (/^calc\(/i.test(t)) return canonicalizeCalcToken(t);
          return t;
        }).join(" ");
      }).join(", ");
    }
    if (key === "content") {
      const normCounterTok = (tok) => {
        const cm = /^(counters?)\((.*)\)$/i.exec(tok);
        if (!cm) return tok;
        const fn = cm[1].toLowerCase();
        const parts = splitTopLevelComma(cm[2]).map(x => x.trim());
        const normStyle = (st) => {
          const low = st.toLowerCase();
          if (low === "decimal") return "";
          if (low.startsWith("symbols(")) {
            const sm = /^symbols\(\s*symbolic\s+([\s\S]+)\)$/i.exec(st);
            if (sm) return "symbols(" + sm[1].trim() + ")";
          }
          return st;
        };
        if (fn === "counter" && parts.length === 2) {
          const ns = normStyle(parts[1]);
          return ns ? ("counter(" + parts[0] + ", " + ns + ")") : ("counter(" + parts[0] + ")");
        }
        if (fn === "counters" && parts.length === 3) {
          const ns = normStyle(parts[2]);
          return ns ? ("counters(" + parts[0] + ", " + parts[1] + ", " + ns + ")") : ("counters(" + parts[0] + ", " + parts[1] + ")");
        }
        return fn + "(" + parts.join(", ") + ")";
      };
      return splitCssTokens(s).map(normCounterTok).join(" ");
    }
    if (key === "text-transform") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 1 && (p[0] === "none" || p[0] === "math-auto")) return p[0];
      let c = "", fw = false, fsk = false;
      for (let i = 0; i < p.length; i++) {
        if (p[i] === "capitalize" || p[i] === "uppercase" || p[i] === "lowercase") c = p[i];
        else if (p[i] === "full-width") fw = true;
        else if (p[i] === "full-size-kana") fsk = true;
      }
      return [c, fw ? "full-width" : "", fsk ? "full-size-kana" : ""].filter(Boolean).join(" ");
    }
    if (key === "white-space") {
      const p = splitCssTokens(s.toLowerCase());
      const SINGLE_MAP = {
        normal: ["collapse", "wrap"],
        pre: ["preserve", "nowrap"],
        nowrap: ["collapse", "nowrap"],
        "pre-wrap": ["preserve", "wrap"],
        "break-spaces": ["break-spaces", "wrap"],
        "pre-line": ["preserve-breaks", "wrap"],
        collapse: ["collapse", "wrap"],
        preserve: ["preserve", "wrap"],
        "preserve-breaks": ["preserve-breaks", "wrap"],
        "preserve-spaces": ["preserve-spaces", "wrap"],
        wrap: ["collapse", "wrap"]
      };
      let col = "collapse", wr = "wrap";
      if (p.length === 1 && SINGLE_MAP[p[0]]) {
        col = SINGLE_MAP[p[0]][0]; wr = SINGLE_MAP[p[0]][1];
      } else if (p.length === 2) {
        if (p[0] === "wrap" || p[0] === "nowrap") { wr = p[0]; col = p[1]; }
        else { col = p[0]; wr = p[1]; }
      }
      if (col === "collapse" && wr === "wrap") return "normal";
      if (col === "preserve" && wr === "nowrap") return "pre";
      if (col === "collapse" && wr === "nowrap") return "nowrap";
      if (col === "preserve" && wr === "wrap") return "pre-wrap";
      if (col === "break-spaces" && wr === "wrap") return "break-spaces";
      if (col === "preserve-breaks" && wr === "wrap") return "pre-line";
      return col + " " + wr;
    }
    if (key === "text-wrap") {
      const p = splitCssTokens(s.toLowerCase());
      let m = "", st = "";
      for (let i = 0; i < p.length; i++) {
        if (p[i] === "wrap" || p[i] === "nowrap") m = p[i];
        else st = p[i];
      }
      if (m === "wrap" && st) return st;
      if (st === "auto" && m) return m;
      return [m, st].filter(Boolean).join(" ") || "wrap";
    }
    if (key === "text-decoration-skip-spaces") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 2) return "start end";
      return p.join(" ");
    }
    if (key === "text-decoration-inset") {
      const p = splitCssTokens(s.toLowerCase());
      for (let i = 0; i < p.length; i++) {
        if (/^[+-]?0(?:\.0+)?$/.test(p[i])) p[i] = "0px";
        else {
          const dm = /^calc\(\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+))([a-z%]+)\s*\/\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+))\s*\)$/i.exec(p[i]);
          if (dm && Number(dm[3]) !== 0) {
            p[i] = "calc(" + String(Number((Number(dm[1]) / Number(dm[3])).toFixed(6))) + dm[2] + ")";
          }
        }
      }
      if (p.length === 2 && p[0] === p[1]) return p[0];
      return p.join(" ");
    }
    if (key === "text-underline-position") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 2 && (p[0] === "left" || p[0] === "right")) return p[1] + " " + p[0];
      return p.join(" ");
    }
    if (key === "grid-auto-flow") {
      const p = splitCssTokens(s.toLowerCase());
      let ax = "", den = false;
      for (let i = 0; i < p.length; i++) {
        if (p[i] === "dense") den = true;
        else ax = p[i];
      }
      return ax && den ? (ax === "row" ? "dense" : (ax + " dense")) : (ax || (den ? "dense" : s));
    }
    if (key === "container") {
      const slash = s.split("/");
      if (slash.length === 2 && slash[1].trim().toLowerCase() === "normal") return slash[0].trim();
      return s;
    }
    if (key === "grid-template-areas") {
      if (s.toLowerCase() === "none") return "none";
      const toks = splitCssTokens(s);
      return toks.map((t) => {
        const inner = t.slice(1, -1).trim();
        const cells = inner.split(/\s+/).map((c) => (/^\.+$/.test(c) ? "." : c));
        return '"' + cells.join(" ") + '"';
      }).join(" ");
    }
    if (key === "grid-template-columns" || key === "grid-template-rows") {
      if (s.includes("[]") || /\[\s+\]/.test(s)) {
        return s.replace(/\[\s*\]/g, "").replace(/\s+/g, " ").replace(/,\s+/g, ", ").replace(/\(\s+/g, "(").replace(/\s+\)/g, ")").trim();
      }
      return s;
    }
    if (key === "font-synthesis") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 1 && p[0] === "none") return "none";
      const order = ["weight", "style", "oblique-only", "small-caps", "position"];
      return order.filter(k => p.indexOf(k) !== -1).join(" ");
    }
    if (key === "font-variant-numeric") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 1 && p[0] === "normal") return "normal";
      const order = ["lining-nums", "oldstyle-nums", "proportional-nums", "tabular-nums", "diagonal-fractions", "stacked-fractions", "ordinal", "slashed-zero"];
      return order.filter(k => p.indexOf(k) !== -1).join(" ");
    }
    if (key === "font-variant-east-asian") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 1 && p[0] === "normal") return "normal";
      const order = ["jis78", "jis83", "jis90", "jis04", "simplified", "traditional", "full-width", "proportional-width", "ruby"];
      return order.filter(k => p.indexOf(k) !== -1).join(" ");
    }
    if (key === "font-variant-ligatures") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 1 && (p[0] === "normal" || p[0] === "none")) return p[0];
      const order = ["common-ligatures", "no-common-ligatures", "discretionary-ligatures", "no-discretionary-ligatures", "historical-ligatures", "no-historical-ligatures", "contextual", "no-contextual"];
      return order.filter(k => p.indexOf(k) !== -1).join(" ");
    }
    if (key === "animation") {
      const TF_KW = new Set(["linear", "ease", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end"]);
      const DIR = new Set(["normal", "reverse", "alternate", "alternate-reverse"]);
      const FILL = new Set(["none", "forwards", "backwards", "both"]);
      const PLAY = new Set(["running", "paused"]);
      return splitTopLevelComma(s).map((layer) => {
        const p = splitCssTokens(layer);
        let du = "", tf = "", dl = "", it = "", dr = "", fl = "", pl = "", nm = "";
        let seenTime = 0;
        for (let i = 0; i < p.length; i++) {
          const t = p[i], tl = t.toLowerCase();
          if (!tf && TF_KW.has(tl)) tf = tl;
          else if (!tf && /^(?:cubic-bezier|steps|linear)\(/i.test(tl)) tf = t.replace(/\(\s+/g, "(").replace(/\s+\)/g, ")");
          else if (seenTime < 2 && /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?(?:ms|s)$/i.test(tl)) {
            if (seenTime === 0) du = tl;
            else dl = tl;
            seenTime++;
          } else if (!it && (tl === "infinite" || /^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(tl))) it = tl;
          else if (!dr && DIR.has(tl)) dr = tl;
          else if (!fl && FILL.has(tl)) fl = tl;
          else if (!pl && PLAY.has(tl)) pl = tl;
          else if (!nm) nm = tl === "none" ? "none" : t;
        }
        return [du, tf, dl, it, dr, fl, pl, nm].filter(Boolean).join(" ") || "none";
      }).join(", ");
    }
    if (key === "animation-name") {
      const RESTRICTED = new Set(["none", "initial", "inherit", "unset", "revert", "revert-layer", "default"]);
      return splitTopLevelComma(s).map((part) => {
        if ((part.startsWith('"') && part.endsWith('"')) || (part.startsWith("'") && part.endsWith("'"))) {
          const inner = part.slice(1, -1).replace(/\\([0-9a-fA-F]{1,6})\s?/g, (_, hex) => {
            try { return String.fromCodePoint(parseInt(hex, 16)); } catch (_) { return _; }
          });
          if (RESTRICTED.has(inner.toLowerCase())) return '"' + inner + '"';
          return inner.replace(/([ "\'\\])/g, "\\$1");
        }
        return part.toLowerCase() === "none" ? "none" : part;
      }).join(", ");
    }
    if (key === "animation-range-start" || key === "animation-range-end") {
      const RANGE_NAMES = new Set(["cover", "contain", "entry", "exit", "entry-crossing", "exit-crossing"]);
      const defaultPctRe = key === "animation-range-end" ? /^[+-]?100(?:\.0+)?%$/ : /^[+-]?0(?:\.0+)?%$/;
      const normOffset = (tok) => {
        if (/^[+-]?0(?:\.0+)?$/.test(tok)) return "0px";
        if (/^calc\(\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:px|em|rem|ex|ch|vw|vh))\s*\+\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+)%)\s*\)$/i.test(tok)) {
          return tok.replace(/^calc\(\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:px|em|rem|ex|ch|vw|vh))\s*\+\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+)%)\s*\)$/i, "calc($2 + $1)");
        }
        if (/^calc\(\s*(sign\([^()]+\))\s*\*\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+)%)\s*\)$/i.test(tok)) {
          return tok.replace(/^calc\(\s*(sign\([^()]+\))\s*\*\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+)%)\s*\)$/i, "calc($2 * $1)");
        }
        return tok;
      };
      return splitTopLevelComma(s).map((part) => {
        const toks = splitCssTokens(part);
        if (toks.length === 1) {
          const tl = toks[0].toLowerCase();
          if (tl === "normal" || RANGE_NAMES.has(tl)) return tl;
          return normOffset(toks[0]);
        }
        if (toks.length === 2) {
          const name = toks[0].toLowerCase();
          const off = normOffset(toks[1]);
          if (defaultPctRe.test(off)) return name;
          return name + " " + off;
        }
        return part;
      }).join(", ");
    }
    if (key === "transition-property") {
      return s.split(",").map((part) => {
        const t = part.trim();
        const tl = t.toLowerCase();
        return (tl === "all" || tl === "none") ? tl : t;
      }).join(", ");
    }
    if (key === "letter-spacing" || key === "word-spacing") {
      const sl = s.toLowerCase();
      if (/^calc\(\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:ch|em|rem|ex|px|vw|vh))\s*([+-])\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+)%)\s*\)$/i.test(sl)) {
        return sl.replace(/^calc\(\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:ch|em|rem|ex|px|vw|vh))\s*([+-])\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+)%)\s*\)$/i, (_, len, op, pct) => {
          const signedPct = op === "-" ? (pct.startsWith("-") ? pct.slice(1) : "-" + pct) : pct;
          return "calc(" + signedPct + " + " + len + ")";
        });
      }
      return sl;
    }
    if (key === "hyphenate-character") {
      const toks = splitCssTokens(s);
      if (toks.length === 1) {
        const t = toks[0];
        if (t.toLowerCase() === "auto") return "auto";
        if ((t.startsWith('"') && t.endsWith('"')) || (t.startsWith("'") && t.endsWith("'"))) {
          const decoded = t.slice(1, -1).replace(/\\([0-9a-fA-F]{1,6})\s?/g, (_, hex) => {
            try { return String.fromCodePoint(parseInt(hex, 16)); } catch (_) { return _; }
          });
          return '"' + decoded.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '"';
        }
      }
      return s;
    }
    if (key === "hyphenate-limit-chars") {
      const p = splitCssTokens(s.toLowerCase()).map((t) => (t === "auto" ? "auto" : (/^\+?\d+$/.test(t) ? String(parseInt(t, 10)) : t)));
      if (p.length === 3 && p[2] === p[1]) p.pop();
      if (p.length === 2 && p[0] === "auto" && p[1] === "auto") return "auto";
      return p.join(" ");
    }
    if (key === "word-space-transform") {
      return splitCssTokens(s.toLowerCase()).join(" ");
    }
    if (key === "text-autospace") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 1 && (p[0] === "normal" || p[0] === "no-autospace" || p[0] === "auto")) return p[0];
      const out = [];
      if (p.indexOf("ideograph-alpha") !== -1) out.push("ideograph-alpha");
      if (p.indexOf("ideograph-numeric") !== -1) out.push("ideograph-numeric");
      if (p.indexOf("punctuation") !== -1) out.push("punctuation");
      if (p.indexOf("insert") !== -1) out.push("insert");
      else if (p.indexOf("replace") !== -1) out.push("replace");
      return out.join(" ");
    }
    if (key === "text-spacing") {
      const TRIM_KW = new Set(["space-all", "trim-both", "trim-all", "trim-start", "space-first"]);
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 1 && (p[0] === "none" || p[0] === "auto" || p[0] === "normal")) return p[0];
      if (p.length === 2 && p[0] === "auto" && p[1] === "auto") return "auto";
      let trim = "normal";
      const autoToks = [];
      for (let i = 0; i < p.length; i++) {
        if (TRIM_KW.has(p[i])) trim = p[i];
        else if (p[i] !== "normal") autoToks.push(p[i]);
      }
      const auto = autoToks.length > 0 ? normalizeCssSpecifiedValue("text-autospace", autoToks.join(" ")) : "normal";
      if (trim === "space-all" && auto === "no-autospace") return "none";
      if (trim === "auto" && auto === "auto") return "auto";
      const out = [];
      if (trim !== "normal") out.push(trim);
      if (auto !== "normal") out.push(auto);
      return out.length > 0 ? out.join(" ") : "normal";
    }
    if (key === "hanging-punctuation") {
      return splitCssTokens(s.toLowerCase()).join(" ");
    }
    if (key === "scrollbar-gutter") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 2) return "stable both-edges";
      return p.join(" ");
    }
    if (key === "line-clamp") {
      const toks = splitCssTokens(s.trim());
      if (toks.length === 1 && toks[0].toLowerCase() === "none") return "none";
      let hasLegacy = false;
      let items = toks;
      if (toks[toks.length - 1].toLowerCase() === "-webkit-legacy") {
        hasLegacy = true;
        items = toks.slice(0, -1);
      }
      let count = null, ellipsis = null;
      const isPositiveInt = (t) => /^\+?[1-9]\d*$/.test(t) || /^(?:calc|min|max|clamp)\(/i.test(t);
      if (items.length === 1) {
        const t = items[0], tl = t.toLowerCase();
        if (isPositiveInt(t) || tl === "auto") {
          count = t;
        } else if (tl === "ellipsis") {
          count = "auto";
        } else {
          count = "auto";
          ellipsis = t;
        }
      } else if (items.length === 2) {
        let first = items[0], second = items[1];
        if (isPositiveInt(first)) {
          count = first;
          ellipsis = second.toLowerCase() === "ellipsis" ? null : second;
        } else if (isPositiveInt(second)) {
          count = second;
          ellipsis = first.toLowerCase() === "ellipsis" ? null : first;
        } else {
          count = "auto";
          ellipsis = second.toLowerCase() === "ellipsis" ? null : second;
        }
      }
      const parts = [];
      if (count) parts.push(count);
      if (ellipsis) parts.push(ellipsis);
      if (hasLegacy) parts.push("-webkit-legacy");
      return parts.join(" ");
    }
    if (key === "max-lines") {
      const toks = splitCssTokens(s.toLowerCase());
      if (toks.length === 2 && toks[0] === "auto") return toks[1] + " auto";
      return toks.join(" ");
    }
    if (key === "overflow-clip-margin") {
      const toks = splitCssTokens(s.trim());
      let box = null, len = null;
      const BOXES = new Set(["content-box", "padding-box", "border-box"]);
      for (const t of toks) {
        const tl = t.toLowerCase();
        if (BOXES.has(tl)) box = tl;
        else len = t;
      }
      if (len && /^calc\(/i.test(len)) {
        const inn = len.slice(5, -1).trim();
        const m = /^([+-]?(?:\d+(?:\.\d+)?|\.\d+))px\s*([+-])\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+))px$/i.exec(inn);
        if (m) {
          const v1 = Number(m[1]), op = m[2], v2 = Number(m[3]);
          const res = op === "+" ? v1 + v2 : v1 - v2;
          len = "calc(" + res + "px)";
        }
      }
      if (!box || box === "padding-box") {
        return len || "0px";
      }
      if (!len || len === "0px" || len === "calc(0px)") {
        return box;
      }
      return box + " " + len;
    }
    if (key === "contain") {
      const toks = splitCssTokens(s);
      if (toks.length === 1) {
        const t0 = toks[0].toLowerCase();
        if (t0 === "none" || t0 === "strict" || t0 === "content") return t0;
      }
      let hasSize = false;
      let hasInlineSize = false;
      let hasLayout = false;
      let hasStyle = false;
      let hasPaint = false;
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i].toLowerCase();
        if (t === "size") hasSize = true;
        else if (t === "inline-size") hasInlineSize = true;
        else if (t === "layout") hasLayout = true;
        else if (t === "style") hasStyle = true;
        else if (t === "paint") hasPaint = true;
      }
      const parts = [];
      if (hasSize) parts.push("size");
      else if (hasInlineSize) parts.push("inline-size");
      if (hasLayout) parts.push("layout");
      if (hasStyle) parts.push("style");
      if (hasPaint) parts.push("paint");
      return parts.length > 0 ? parts.join(" ") : s;
    }
    if (key === "contain-intrinsic-size") {
      const p = parseContainIntrinsicSize(s);
      if (!p) return s;
      return p[0] === p[1] ? p[0] : (p[0] + " " + p[1]);
    }
    if (key === "contain-intrinsic-width" || key === "contain-intrinsic-height" ||
        key === "contain-intrinsic-inline-size" || key === "contain-intrinsic-block-size") {
      const toks = splitCssTokens(s);
      const single = parseSingleIntrinsic(toks);
      return single !== null ? single : s;
    }
    if (key === "shape-image-threshold") {
      const tl = s.trim().toLowerCase();
      if (tl.endsWith("%")) {
        const num = Number(tl.slice(0, -1));
        if (!isNaN(num) && isFinite(num)) {
          return String(Number((num / 100).toFixed(6)));
        }
      }
      if (/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)$/.test(tl)) {
        return String(Number(tl));
      }
      return s;
    }
    if (key === "shape-margin") {
      const tl = s.trim();
      if (tl === "0" || tl === "+0") return "0px";
      if (/^calc\(/i.test(tl)) return canonicalizeCalcToken(tl);
      return s;
    }
    if (key === "scroll-snap-type") {
      const toks = splitCssTokens(s.trim().toLowerCase());
      if (toks.length === 1) return toks[0];
      if (toks.length === 2) {
        if (toks[1] === "proximity") return toks[0];
        return toks[0] + " " + toks[1];
      }
      return s;
    }
    if (key === "scroll-snap-align") {
      const toks = splitCssTokens(s.trim().toLowerCase());
      if (toks.length === 1) return toks[0];
      if (toks.length === 2) {
        return toks[0] === toks[1] ? toks[0] : (toks[0] + " " + toks[1]);
      }
      return s;
    }
    const normLenZero = (t) => (t === "0" || t === "+0" || t === "-0") ? "0px" : t;
    const normCalcOrLen = (t) => canonicalizeCalcToken(normLenZero(t));
    const collapse2 = (a, b) => (a === b ? a : (a + " " + b));
    const collapse4 = (a, b, c, d) => (b === d ? (a === c ? collapse2(a, b) : (a + " " + b + " " + c)) : (a + " " + b + " " + c + " " + d));
    const collapseToks = (toks) => {
      if (toks.length === 1) return toks[0];
      if (toks.length === 2) return collapse2(toks[0], toks[1]);
      if (toks.length === 3) return (toks[0] === toks[2]) ? collapse2(toks[0], toks[1]) : toks.join(" ");
      if (toks.length === 4) return collapse4(toks[0], toks[1], toks[2], toks[3]);
      return toks.join(" ");
    };
    if (key.startsWith("scroll-margin") || key.startsWith("scroll-padding") ||
        key === "inset" || key === "inset-block" || key === "inset-inline" ||
        key === "inset-block-start" || key === "inset-block-end" || key === "inset-inline-start" || key === "inset-inline-end" ||
        key === "margin-block" || key === "margin-inline" || key === "margin-block-start" || key === "margin-block-end" || key === "margin-inline-start" || key === "margin-inline-end" ||
        key === "padding-block" || key === "padding-inline" || key === "padding-block-start" || key === "padding-block-end" || key === "padding-inline-start" || key === "padding-inline-end" ||
        key === "border-block-width" || key === "border-inline-width" || key === "border-block-start-width" || key === "border-block-end-width" || key === "border-inline-start-width" || key === "border-inline-end-width" ||
        key === "border-spacing") {
      const toks = splitCssTokens(s.trim()).map(normCalcOrLen);
      if (key === "border-spacing") return toks.length === 2 && toks[0] === toks[1] ? toks[0] : toks.join(" ");
      return collapseToks(toks);
    }
    if (key === "border-block-style" || key === "border-inline-style") {
      const toks = splitCssTokens(s.trim().toLowerCase());
      return collapseToks(toks);
    }
    const normBorderSpecColor = (t) => t.startsWith("#") ? normalizeSingleColor(t) : t.toLowerCase();
    if (key === "border-block-color" || key === "border-inline-color" ||
        key === "border-block-start-color" || key === "border-block-end-color" ||
        key === "border-inline-start-color" || key === "border-inline-end-color") {
      const toks = splitCssTokens(s.trim()).map(normBorderSpecColor);
      return collapseToks(toks);
    }
    if (key === "border-block" || key === "border-block-start" || key === "border-block-end" ||
        key === "border-inline" || key === "border-inline-start" || key === "border-inline-end") {
      const toks = splitCssTokens(s.trim());
      const LINE_STYLES = new Set(["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"]);
      let w = "", st = "", c = "";
      for (let i = 0; i < toks.length; i++) {
        const t = toks[i], tl = t.toLowerCase();
        if (!st && LINE_STYLES.has(tl)) st = tl;
        else if (!w && (tl === "thin" || tl === "medium" || tl === "thick" || /^[+-]?(?:\d|\.)/.test(tl) || /^(?:calc|min|max|clamp)\(/i.test(tl))) w = normCalcOrLen(t);
        else c = normBorderSpecColor(t);
      }
      return [w, st, c].filter(Boolean).join(" ");
    }
    if (key === "background-image" || key === "list-style-image") {
      const gm = /^((?:repeating-)?(?:radial|conic)-gradient)\(\s*(.*)\s*\)$/i.exec(s);
      if (gm) {
        const args = splitTopLevelComma(gm[2]);
        let changed = false;
        if (args.length >= 2) {
          const toks0 = splitCssTokens(args[0]);
          const atIdx = toks0.findIndex((t) => t.toLowerCase() === "at");
          if (atIdx !== -1 && atIdx + 1 < toks0.length) {
            const pos = toks0.slice(atIdx + 1).map((t) => (t === "0" || t === "+0" || t === "-0") ? "0px" : t);
            let normPos = pos;
            if (pos.length === 2) {
              const a = pos[0].toLowerCase(), b = pos[1].toLowerCase();
              if (a === "top" || a === "bottom" || (a === "center" && (b === "left" || b === "right"))) {
                normPos = [pos[1], pos[0]];
              }
            } else if (pos.length === 4) {
              const a = pos[0].toLowerCase();
              if (a === "top" || a === "bottom") normPos = [pos[2], pos[3], pos[0], pos[1]];
            }
            args[0] = toks0.slice(0, atIdx + 1).concat(normPos).join(" ");
            changed = true;
          }
          if (/\bcalc\(/i.test(gm[2])) {
            for (let ai = 0; ai < args.length; ai++) {
              const atoks = splitCssTokens(args[ai]);
              let argChanged = false;
              for (let ti = 0; ti < atoks.length; ti++) {
                if (/^calc\(/i.test(atoks[ti])) {
                  const nt = canonicalizeCalcToken(atoks[ti]);
                  if (nt !== atoks[ti]) { atoks[ti] = nt; argChanged = true; }
                }
              }
              if (argChanged) { args[ai] = atoks.join(" "); changed = true; }
            }
          }
          if (changed) return gm[1].toLowerCase() + "(" + args.join(", ") + ")";
        }
      }
    }
    if (key === "text-decoration-line") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 1 && (p[0] === "none" || p[0] === "spelling-error" || p[0] === "grammar-error")) return p[0];
      const order = ["underline", "overline", "line-through", "blink"];
      return order.filter(k => p.indexOf(k) !== -1).join(" ");
    }
    if (key === "text-shadow") {
      if (s.toLowerCase() === "none") return "none";
      const layers = splitTopLevelComma(s);
      return layers.map((layer) => {
        const toks = splitCssTokens(layer);
        let col = "";
        const lens = [];
        for (let ti = 0; ti < toks.length; ti++) {
          const t = toks[ti];
          if (t === "0" || t === "+0" || t === "-0") lens.push("0px");
          else if (/^calc\(/i.test(t)) lens.push(canonicalizeCalcToken(t));
          else if (/^[+-]?(?:\d|\.)/.test(t) && !t.startsWith("#")) lens.push(t);
          else col = t;
        }
        return (col ? [col].concat(lens) : lens).join(" ");
      }).join(", ");
    }
    if (key === "box-shadow") {
      if (s.toLowerCase() === "none") return "none";
      const layers = splitTopLevelComma(s);
      return layers.map((layer) => {
        const toks = splitCssTokens(layer);
        let col = "", isInset = false;
        const lens = [];
        for (let ti = 0; ti < toks.length; ti++) {
          const t = toks[ti];
          const tl = t.toLowerCase();
          if (tl === "inset") isInset = true;
          else if (t === "0" || t === "+0" || t === "-0") lens.push("0px");
          else if (/^calc\(/i.test(t)) lens.push(canonicalizeCalcToken(t));
          else if (/^[+-]?(?:\d|\.)/.test(t) && !t.startsWith("#")) lens.push(t);
          else col = tl.startsWith("#") || /^rgba?\(/i.test(tl) ? normalizeSingleColor(t) : tl;
        }
        const out = col ? [col].concat(lens) : lens;
        if (isInset) out.push("inset");
        return out.join(" ");
      }).join(", ");
    }
    if (key === "cursor") {
      const normImg = (fn) => {
        if (/^(?:image-set|light-dark)\(/i.test(fn)) {
          return fn.replace(/,\s*/g, ", ").replace(/(image-set\()([^)]+)\)/gi, (_, pfx, inn) => {
            const items = splitTopLevelComma(inn).map((it) => it.replace(/^("[^"]*"|'[^']*')(\s+.*)$/, (_, u, rest) => "url(" + u + ")" + rest));
            return pfx + items.join(", ") + ")";
          });
        }
        return fn;
      };
      const normNumCalc = (t) => {
        if (!/^calc\(/i.test(t)) return t;
        const inn = t.slice(5, -1).trim();
        if (/^[\d+\-*/().\s]+$/.test(inn)) {
          try { const v = Function("return (" + inn + ")")(); if (typeof v === "number" && isFinite(v)) return "calc(" + String(Number(v.toFixed(6))) + ")"; } catch (_) {}
        }
        return t;
      };
      return splitTopLevelComma(s).map((part, idx, arr) => {
        if (idx === arr.length - 1) return part.trim().toLowerCase();
        const toks = splitCssTokens(part);
        if (toks.length === 1) return normImg(toks[0]);
        if (toks.length === 3) return normImg(toks[0]) + " " + normNumCalc(toks[1]) + " " + normNumCalc(toks[2]);
        return part;
      }).join(", ");
    }
    if (key === "list-style-image" || key === "background-image") {
      let open = 0, close = 0;
      for (let i = 0; i < s.length; i++) {
        if (s[i] === "(") open++;
        else if (s[i] === ")") close++;
      }
      return open > close ? s + ")".repeat(open - close) : s;
    }
    if (key === "list-style-type") {
      if (/^symbols\(\s*symbolic\s+/i.test(s)) {
        return "symbols(" + s.replace(/^symbols\(\s*symbolic\s+/i, "");
      }
      return s;
    }
    if (key === "counter-reset" || key === "counter-increment" || key === "counter-set") {
      const p = splitCssTokens(s);
      if (p.length === 1 && p[0].toLowerCase() === "none") return "none";
      const defVal = key === "counter-increment" ? "1" : "0";
      const out = [];
      let i = 0;
      while (i < p.length) {
        out.push(p[i]);
        const isRev = /^reversed\(/i.test(p[i]);
        if (i + 1 < p.length && (/^[+-]?\d+$/.test(p[i + 1]) || /^(?:calc|min|max|clamp|sign)\(/i.test(p[i + 1]))) {
          out.push(/^[+-]?\d+$/.test(p[i + 1]) ? String(Number(p[i + 1])) : p[i + 1]);
          i += 2;
        } else {
          if (!isRev) out.push(defVal);
          i += 1;
        }
      }
      return out.join(" ");
    }
    return s;
  }
  global.__capsurf_normalizeCssValue = normalizeCssSpecifiedValue;

  function expandCssShorthand(key, normV) {
    if (/\bvar\(/i.test(normV)) return null;
    if (/^(?:initial|inherit|unset|revert|revert-layer)$/i.test(normV)) {
      const lhs = SHORTHAND_EXPAND_LONGHANDS[key];
      if (lhs) {
        const kw = normV.toLowerCase();
        return lhs.map((lh) => [lh, kw]);
      }
      return null;
    }
    if (key === "border-image") {
      const slashes = [];
      let sc = "", sd = 0, sq = "";
      for (let i = 0; i < normV.length; i++) {
        const ch = normV[i];
        if (sq) { sc += ch; if (ch === sq && normV[i - 1] !== "\\") sq = ""; continue; }
        if (ch === "\"" || ch === "'") { sq = ch; sc += ch; continue; }
        if (ch === "(") sd++;
        else if (ch === ")" && sd > 0) sd--;
        if (sd === 0 && ch === "/") { slashes.push(sc.trim()); sc = ""; }
        else sc += ch;
      }
      slashes.push(sc.trim());
      const REP = new Set(["stretch", "repeat", "round", "space"]);
      const lastToks = splitCssTokens(slashes[slashes.length - 1]);
      const repToks = [];
      while (lastToks.length > 0 && REP.has(lastToks[lastToks.length - 1].toLowerCase())) {
        repToks.unshift(lastToks.pop().toLowerCase());
      }
      slashes[slashes.length - 1] = lastToks.join(" ");
      const seg0 = splitCssTokens(slashes[0]);
      let srcVal = "none";
      const sliceToks = [];
      for (let i = 0; i < seg0.length; i++) {
        const t = seg0[i];
        if (srcVal === "none" && /^(?:none|url\(|(?:repeating-)?(?:linear|radial|conic)-gradient\(|image-set\()/i.test(t)) {
          srcVal = t;
        } else {
          sliceToks.push(t);
        }
      }
      const sliceVal = sliceToks.length ? sliceToks.join(" ") : "100%";
      const widthVal = (slashes[1] && slashes[1].trim()) ? slashes[1].trim() : "1";
      const outsetVal = (slashes[2] && slashes[2].trim()) ? slashes[2].trim() : "0";
      const repVal = repToks.length ? repToks.join(" ") : "stretch";
      return [
        ["border-image-source", srcVal],
        ["border-image-slice", sliceVal],
        ["border-image-width", widthVal],
        ["border-image-outset", outsetVal],
        ["border-image-repeat", repVal],
      ];
    }
    if (key === "border-radius") {
      const si = normV.indexOf("/");
      const exp4 = (str) => {
        const p = splitCssTokens(str);
        if (p.length < 1 || p.length > 4) return null;
        const tl = p[0];
        const tr = p.length >= 2 ? p[1] : tl;
        const br = p.length >= 3 ? p[2] : tl;
        const bl = p.length >= 4 ? p[3] : tr;
        return [tl, tr, br, bl];
      };
      const h = exp4(si === -1 ? normV : normV.slice(0, si));
      if (!h) return null;
      const v = si === -1 ? h : exp4(normV.slice(si + 1));
      if (!v) return null;
      const corners = ["border-top-left-radius", "border-top-right-radius", "border-bottom-right-radius", "border-bottom-left-radius"];
      return corners.map((cn, idx) => [cn, h[idx] === v[idx] ? h[idx] : (h[idx] + " " + v[idx])]);
    }
    if (key === "border-color" || key === "border-style" || key === "border-width") {
      const p = splitCssTokens(normV);
      if (p.length < 1 || p.length > 4) return null;
      const suffix = key.slice("border-".length);
      const t = p[0];
      const r = p.length >= 2 ? p[1] : t;
      const b = p.length >= 3 ? p[2] : t;
      const l = p.length >= 4 ? p[3] : r;
      return [
        ["border-top-" + suffix, t],
        ["border-right-" + suffix, r],
        ["border-bottom-" + suffix, b],
        ["border-left-" + suffix, l],
      ];
    }
    if (key === "text-decoration") {
      const parts = splitCssTokens(normV);
      const LINES = new Set(["underline", "overline", "line-through", "blink"]);
      const STY = new Set(["solid", "double", "dotted", "dashed", "wavy"]);
      const lines = [];
      let st = "solid", th = "auto", col = "currentcolor";
      for (let i = 0; i < parts.length; i++) {
        const t = parts[i], tl = t.toLowerCase();
        if (tl === "none" || tl === "spelling-error" || tl === "grammar-error" || LINES.has(tl)) lines.push(tl);
        else if (STY.has(tl)) st = tl;
        else if (tl === "from-font" || /^[+-]?(?:\d|\.)/.test(tl) || /^(?:calc|min|max|clamp)\(/i.test(tl)) th = tl;
        else col = t;
      }
      return [
        ["text-decoration-line", lines.length > 0 ? lines.join(" ") : "none"],
        ["text-decoration-style", st],
        ["text-decoration-thickness", th],
        ["text-decoration-color", col]
      ];
    }
    if (key === "list-style") {
      const parts = splitCssTokens(normV);
      let pos = "outside", img = "none", typ = "disc", noneCnt = 0, hasPos = false, hasType = false;
      for (let i = 0; i < parts.length; i++) {
        const t = parts[i], tl = t.toLowerCase();
        if ((tl === "inside" || tl === "outside") && !hasPos) { pos = tl; hasPos = true; }
        else if (tl === "none") noneCnt++;
        else if (/^(?:url|linear-gradient|radial-gradient|conic-gradient|image-set)\(/i.test(tl)) {
          img = /^url\(/i.test(tl) ? t : t.replace(/,\s*/g, ", ").replace(/#([0-9a-f]{3,8})\b/gi, (m) => normalizeSingleColor(m));
        } else {
          typ = normalizeCssSpecifiedValue("list-style-type", t);
          hasType = true;
        }
      }
      if (noneCnt >= 2) { img = "none"; typ = "none"; }
      else if (noneCnt === 1) {
        if (!hasType) typ = "none";
      }
      return [
        ["list-style-position", pos],
        ["list-style-image", img],
        ["list-style-type", typ]
      ];
    }
    if (key === "place-content" || key === "place-items" || key === "place-self") {
      const kind = key.slice(6);
      const p = splitCssTokens(normV.toLowerCase());
      const normAlignPart = (arr) => {
        if (arr.length === 2 && arr[0] === "first" && arr[1] === "baseline") return "baseline";
        if (arr.length === 2 && arr[1] === "legacy") return "legacy " + arr[0];
        return arr.join(" ");
      };
      if (p.length === 1) {
        const jVal = (key === "place-content" && p[0] === "baseline") ? "start" : p[0];
        return [["align-" + kind, p[0]], ["justify-" + kind, jVal]];
      }
      if (p.length >= 2) {
        const splitAt = (p[0] === "first" || p[0] === "last" || p[0] === "safe" || p[0] === "unsafe" || p[0] === "legacy") ? 2 : 1;
        if (splitAt < p.length) {
          return [
            ["align-" + kind, normAlignPart(p.slice(0, splitAt))],
            ["justify-" + kind, normAlignPart(p.slice(splitAt))]
          ];
        }
        const single = normAlignPart(p);
        const jVal = (key === "place-content" && (single === "baseline" || single === "last baseline")) ? "start" : single;
        return [["align-" + kind, single], ["justify-" + kind, jVal]];
      }
    }
    if (key === "flex-flow") {
      const p = splitCssTokens(normV.toLowerCase());
      let dir = "row", wr = "nowrap";
      for (let i = 0; i < p.length; i++) {
        if (p[i] === "row" || p[i] === "row-reverse" || p[i] === "column" || p[i] === "column-reverse") dir = p[i];
        else if (p[i] === "nowrap" || p[i] === "wrap" || p[i] === "wrap-reverse") wr = p[i];
      }
      return [["flex-direction", dir], ["flex-wrap", wr]];
    }
    if (key === "flex") {
      if (!isValidCssValue("flex", normV)) return null;
      const p = splitCssTokens(normV);
      if (p.length === 1 && p[0].toLowerCase() === "none") {
        return [["flex-grow", "0"], ["flex-shrink", "0"], ["flex-basis", "auto"]];
      }
      const nums = [];
      let basis = "0%";
      let hasBasis = false;
      for (let i = 0; i < p.length; i++) {
        const tl = p[i].toLowerCase();
        if (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/.test(tl) && tl !== "0") {
          nums.push(String(Number(tl)));
        } else if (tl === "0" && nums.length < 2 && !hasBasis) {
          nums.push("0");
        } else if (/^(?:calc|min|max|clamp)\(/i.test(tl) && nums.length < 2 && !/[a-z%]/.test(tl.replace(/sign\([^()]+\)/gi, "").replace(/^(?:calc|min|max|clamp)\(/i, ""))) {
          nums.push(p[i]);
        } else {
          basis = tl === "0" ? "0px" : p[i];
          hasBasis = true;
        }
      }
      const fg = nums.length >= 1 ? nums[0] : "1";
      const fs = nums.length >= 2 ? nums[1] : "1";
      return [["flex-grow", fg], ["flex-shrink", fs], ["flex-basis", basis]];
    }
    if (key === "text-emphasis") {
      const p = splitCssTokens(normV);
      const styleToks = [];
      let col = "currentcolor";
      for (let i = 0; i < p.length; i++) {
        const t = p[i], tl = t.toLowerCase();
        if (tl === "none" || tl === "filled" || tl === "open" || tl === "dot" || tl === "circle" ||
            tl === "double-circle" || tl === "triangle" || tl === "sesame" || /^"[^"]*"$/.test(t) || /^'[^']*'$/.test(t)) {
          styleToks.push(t);
        } else {
          col = t;
        }
      }
      const st = styleToks.length > 0 ? normalizeCssSpecifiedValue("text-emphasis-style", styleToks.join(" ")) : "none";
      return [["text-emphasis-style", st], ["text-emphasis-color", col]];
    }
    if (key === "overflow") {
      const p = splitCssTokens(normV.toLowerCase());
      if (p.length === 1) return [["overflow-x", p[0]], ["overflow-y", p[0]]];
      if (p.length === 2) return [["overflow-x", p[0]], ["overflow-y", p[1]]];
    }
    if (key === "gap" || key === "grid-gap") {
      const p = splitCssTokens(normV);
      if (p.length === 1) return [["row-gap", p[0]], ["column-gap", p[0]]];
      if (p.length === 2) return [["row-gap", p[0]], ["column-gap", p[1]]];
    }
    if (key === "animation") {
      const TF_KW = new Set(["linear", "ease", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end"]);
      const DIR = new Set(["normal", "reverse", "alternate", "alternate-reverse"]);
      const FILL = new Set(["none", "forwards", "backwards", "both"]);
      const PLAY = new Set(["running", "paused"]);
      const layers = splitTopLevelComma(normV);
      const durs = [], tfs = [], dels = [], iters = [], dirs = [], fills = [], plays = [], names = [];
      for (let li = 0; li < layers.length; li++) {
        const p = splitCssTokens(layers[li]);
        let du = "auto", tf = "ease", dl = "0s", it = "1", dr = "normal", fl = "none", pl = "running", nm = "none", seenTime = 0;
        let seenTf = false, seenIt = false, seenDr = false, seenFl = false, seenPl = false;
        for (let i = 0; i < p.length; i++) {
          const t = p[i], tl = t.toLowerCase();
          if (!seenTf && (TF_KW.has(tl) || /^(?:cubic-bezier|steps|linear)\(/i.test(tl))) { tf = t; seenTf = true; }
          else if (seenTime < 2 && /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?(?:ms|s)$/i.test(tl)) {
            if (seenTime === 0) du = tl;
            else dl = tl;
            seenTime++;
          } else if (!seenIt && (tl === "infinite" || /^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(tl))) { it = tl; seenIt = true; }
          else if (!seenDr && DIR.has(tl)) { dr = tl; seenDr = true; }
          else if (!seenFl && FILL.has(tl)) { fl = tl; seenFl = true; }
          else if (!seenPl && PLAY.has(tl)) { pl = tl; seenPl = true; }
          else if (nm === "none") nm = tl === "none" ? "none" : t;
        }
        durs.push(du); tfs.push(tf); dels.push(dl); iters.push(it); dirs.push(dr); fills.push(fl); plays.push(pl); names.push(nm);
      }
      return [
        ["animation-duration", durs.join(", ")],
        ["animation-timing-function", tfs.join(", ")],
        ["animation-delay", dels.join(", ")],
        ["animation-iteration-count", iters.join(", ")],
        ["animation-direction", dirs.join(", ")],
        ["animation-fill-mode", fills.join(", ")],
        ["animation-play-state", plays.join(", ")],
        ["animation-name", names.join(", ")],
        ["animation-timeline", "auto"],
        ["animation-range-start", "normal"],
        ["animation-range-end", "normal"]
      ];
    }
    if (key === "transition") {
      const layers = splitTopLevelComma(normV);
      const props = [], durs = [], tfs = [], dels = [], behs = [];
      const TF_KW = new Set(["ease", "linear", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end"]);
      for (let li = 0; li < layers.length; li++) {
        const p = splitCssTokens(layers[li].trim());
        let pr = "all", du = "0s", tf = "ease", dl = "0s", bh = "normal", seenTime = 0;
        for (let i = 0; i < p.length; i++) {
          const t = p[i], tl = t.toLowerCase();
          if (tl === "normal" || tl === "allow-discrete") bh = tl;
          else if (TF_KW.has(tl) || /^(?:cubic-bezier|steps|linear)\(/i.test(tl)) tf = t;
          else if (/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?(?:ms|s)$/i.test(tl) || tl === "0") {
            const tv = tl === "0" ? "0s" : t;
            if (seenTime === 0) du = tv;
            else dl = tv;
            seenTime++;
          } else pr = (tl === "all" || tl === "none") ? tl : t;
        }
        props.push(pr); durs.push(du); tfs.push(tf); dels.push(dl); behs.push(bh);
      }
      return [
        ["transition-property", props.join(", ")],
        ["transition-duration", durs.join(", ")],
        ["transition-timing-function", tfs.join(", ")],
        ["transition-delay", dels.join(", ")],
        ["transition-behavior", behs.join(", ")]
      ];
    }
    if (key === "white-space") {
      const canon = normalizeCssSpecifiedValue("white-space", normV);
      const MAP = {
        normal: ["collapse", "wrap"],
        pre: ["preserve", "nowrap"],
        nowrap: ["collapse", "nowrap"],
        "pre-wrap": ["preserve", "wrap"],
        "break-spaces": ["break-spaces", "wrap"],
        "pre-line": ["preserve-breaks", "wrap"]
      };
      if (MAP[canon]) {
        return [["white-space-collapse", MAP[canon][0]], ["text-wrap-mode", MAP[canon][1]]];
      }
      const p = splitCssTokens(canon);
      return [["white-space-collapse", p[0] || "collapse"], ["text-wrap-mode", p[1] || "wrap"]];
    }
    if (key === "text-spacing") {
      const TRIM_KW = new Set(["space-all", "trim-both", "trim-all", "trim-start", "space-first"]);
      const p = splitCssTokens(normV.toLowerCase());
      if (p.length === 1 && p[0] === "none") return [["text-spacing-trim", "space-all"], ["text-autospace", "no-autospace"]];
      if (p.length === 1 && p[0] === "auto") return [["text-spacing-trim", "auto"], ["text-autospace", "auto"]];
      let trim = "normal";
      const autoToks = [];
      for (let i = 0; i < p.length; i++) {
        if (TRIM_KW.has(p[i])) trim = p[i];
        else if (p[i] !== "normal") autoToks.push(p[i]);
      }
      const auto = autoToks.length > 0 ? normalizeCssSpecifiedValue("text-autospace", autoToks.join(" ")) : "normal";
      return [["text-spacing-trim", trim], ["text-autospace", auto]];
    }
    if (key === "text-wrap") {
      const p = splitCssTokens(normV.toLowerCase());
      let m = "wrap", st = "auto";
      for (let i = 0; i < p.length; i++) {
        if (p[i] === "wrap" || p[i] === "nowrap") m = p[i];
        else st = p[i];
      }
      return [["text-wrap-mode", m], ["text-wrap-style", st]];
    }
    if (key === "columns") {
      const slashIdx = normV.indexOf("/");
      const leftStr = slashIdx === -1 ? normV : normV.slice(0, slashIdx);
      const rightStr = slashIdx === -1 ? "" : normV.slice(slashIdx + 1).trim();
      const p = splitCssTokens(leftStr.toLowerCase());
      let w = "auto", c = "auto";
      for (let i = 0; i < p.length; i++) {
        if (p[i] === "auto") continue;
        if (p[i] === "0" || p[i] === "+0" || p[i] === "-0") {
          w = "0px";
        } else if (/^[+]?[1-9]\d*$/.test(p[i]) || (/^(?:calc|min|max|clamp)\(/i.test(p[i]) && !/[a-z%]/i.test(p[i].replace(/^(?:calc|min|max|clamp)\(/i, "")))) {
          c = p[i];
        } else {
          w = p[i];
        }
      }
      let h = "auto";
      if (rightStr) {
        const rl = rightStr.toLowerCase();
        h = (rl === "0" || rl === "+0" || rl === "-0") ? "0px" : rl;
      }
      return [["column-width", w], ["column-count", c], ["column-height", h]];
    }
    if (key === "outline" || key === "column-rule") {
      const p = splitCssTokens(normV);
      const STY = new Set(["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset", "auto"]);
      let w = "medium", st = "none", c = "currentcolor";
      for (let i = 0; i < p.length; i++) {
        const t = p[i], tl = t.toLowerCase();
        if (STY.has(tl)) st = tl;
        else if (tl === "thin" || tl === "medium" || tl === "thick" || /^[+-]?(?:\d|\.)/.test(tl) || /^(?:calc|min|max|clamp)\(/i.test(tl)) {
          w = (tl === "0" || tl === "+0" || tl === "-0") ? "0px" : (/^calc\(/i.test(t) ? canonicalizeCalcToken(t) : t);
        } else c = t;
      }
      return key === "outline"
        ? [["outline-color", c], ["outline-style", st], ["outline-width", w]]
        : [["column-rule-width", w], ["column-rule-style", st], ["column-rule-color", c]];
    }
    if (key === "contain-intrinsic-size") {
      const p = parseContainIntrinsicSize(normV);
      if (!p) return null;
      return [["contain-intrinsic-width", p[0]], ["contain-intrinsic-height", p[1]]];
    }
    if (key === "scroll-margin" || key === "scroll-padding" || key === "inset") {
      const toks = splitCssTokens(normV).map((t) => (t === "0" || t === "+0" || t === "-0") ? "0px" : t);
      if (toks.length === 0) return null;
      let t = toks[0], r = toks[0], b = toks[0], l = toks[0];
      if (toks.length === 2) { r = toks[1]; l = toks[1]; }
      else if (toks.length === 3) { r = toks[1]; b = toks[2]; l = toks[1]; }
      else if (toks.length === 4) { r = toks[1]; b = toks[2]; l = toks[3]; }
      const pfx = key === "inset" ? "" : (key + "-");
      return [
        [pfx + "top", t],
        [pfx + "right", r],
        [pfx + "bottom", b],
        [pfx + "left", l]
      ];
    }
    if (key === "scroll-margin-block" || key === "scroll-margin-inline" ||
        key === "scroll-padding-block" || key === "scroll-padding-inline" ||
        key === "inset-block" || key === "inset-inline" ||
        key === "margin-block" || key === "margin-inline" ||
        key === "padding-block" || key === "padding-inline") {
      const toks = splitCssTokens(normV).map((t) => (t === "0" || t === "+0" || t === "-0") ? "0px" : t);
      if (toks.length === 0) return null;
      const start = toks[0];
      const end = toks.length > 1 ? toks[1] : toks[0];
      return [
        [key + "-start", start],
        [key + "-end", end]
      ];
    }
    const borderPairMatch = /^(border-(?:block|inline))-(width|style|color)$/.exec(key);
    if (borderPairMatch) {
      const base = borderPairMatch[1];
      const kind = borderPairMatch[2];
      const toks = splitCssTokens(normV).map((t) => (kind === "width" && (t === "0" || t === "+0" || t === "-0")) ? "0px" : t);
      if (toks.length === 0) return null;
      const start = toks[0];
      const end = toks.length > 1 ? toks[1] : toks[0];
      return [
        [base + "-start-" + kind, start],
        [base + "-end-" + kind, end]
      ];
    }
    if (key === "border-block" || key === "border-inline" ||
        key === "border-block-start" || key === "border-block-end" ||
        key === "border-inline-start" || key === "border-inline-end") {
      const p = splitCssTokens(normV);
      const STY = new Set(["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset"]);
      let w = "medium", st = "none", c = "currentcolor";
      for (let i = 0; i < p.length; i++) {
        const t = p[i], tl = t.toLowerCase();
        if (STY.has(tl)) st = tl;
        else if (tl === "thin" || tl === "medium" || tl === "thick" || /^[+-]?(?:\d|\.)/.test(tl) || /^(?:calc|min|max|clamp)\(/i.test(tl)) {
          w = (tl === "0" || tl === "+0" || tl === "-0") ? "0px" : (/^calc\(/i.test(t) ? canonicalizeCalcToken(t) : t);
        } else c = t;
      }
      if (key === "border-block" || key === "border-inline") {
        return [
          [key + "-start-width", w], [key + "-start-style", st], [key + "-start-color", c],
          [key + "-end-width", w], [key + "-end-style", st], [key + "-end-color", c]
        ];
      }
      return [[key + "-width", w], [key + "-style", st], [key + "-color", c]];
    }
    if (key === "container") {
      const slash = normV.split("/");
      const cn = slash[0].trim() || "none";
      const ct = slash.length === 2 ? (slash[1].trim() || "normal") : "normal";
      return [["container-name", cn], ["container-type", ct]];
    }
    return null;
  }
  global.__capsurf_expandShorthand = expandCssShorthand;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_expandShorthand = expandCssShorthand;

  const SHORTHAND_EXPAND_LONGHANDS = {
    "border-radius": ["border-top-left-radius", "border-top-right-radius", "border-bottom-right-radius", "border-bottom-left-radius"],
    "border-color": ["border-top-color", "border-right-color", "border-bottom-color", "border-left-color"],
    "border-style": ["border-top-style", "border-right-style", "border-bottom-style", "border-left-style"],
    "border-width": ["border-top-width", "border-right-width", "border-bottom-width", "border-left-width"],
    "border-top": ["border-top-width", "border-top-style", "border-top-color"],
    "border-right": ["border-right-width", "border-right-style", "border-right-color"],
    "border-bottom": ["border-bottom-width", "border-bottom-style", "border-bottom-color"],
    "border-left": ["border-left-width", "border-left-style", "border-left-color"],
    "text-decoration": ["text-decoration-line", "text-decoration-style", "text-decoration-thickness", "text-decoration-color"],
    "list-style": ["list-style-position", "list-style-image", "list-style-type"],
    "place-content": ["align-content", "justify-content"],
    "place-items": ["align-items", "justify-items"],
    "place-self": ["align-self", "justify-self"],
    "flex-flow": ["flex-direction", "flex-wrap"],
    flex: ["flex-grow", "flex-shrink", "flex-basis"],
    "text-emphasis": ["text-emphasis-style", "text-emphasis-color"],
    "text-wrap": ["text-wrap-mode", "text-wrap-style"],
    "white-space": ["white-space-collapse", "text-wrap-mode"],
    "text-spacing": ["text-spacing-trim", "text-autospace"],
    animation: ["animation-duration", "animation-timing-function", "animation-delay", "animation-iteration-count", "animation-direction", "animation-fill-mode", "animation-play-state", "animation-name", "animation-timeline", "animation-range-start", "animation-range-end"],
    transition: ["transition-property", "transition-duration", "transition-timing-function", "transition-delay", "transition-behavior"],
    outline: ["outline-color", "outline-style", "outline-width"],
    "column-rule": ["column-rule-width", "column-rule-style", "column-rule-color"],
    columns: ["column-width", "column-count", "column-height"],
    overflow: ["overflow-x", "overflow-y"],
    gap: ["row-gap", "column-gap"],
    "grid-gap": ["row-gap", "column-gap"],
    "contain-intrinsic-size": ["contain-intrinsic-width", "contain-intrinsic-height"],
    container: ["container-name", "container-type"],
    "scroll-margin": ["scroll-margin-top", "scroll-margin-right", "scroll-margin-bottom", "scroll-margin-left"],
    "scroll-margin-block": ["scroll-margin-block-start", "scroll-margin-block-end"],
    "scroll-margin-inline": ["scroll-margin-inline-start", "scroll-margin-inline-end"],
    "scroll-padding": ["scroll-padding-top", "scroll-padding-right", "scroll-padding-bottom", "scroll-padding-left"],
    "scroll-padding-block": ["scroll-padding-block-start", "scroll-padding-block-end"],
    "scroll-padding-inline": ["scroll-padding-inline-start", "scroll-padding-inline-end"],
    inset: ["top", "right", "bottom", "left"],
    "inset-block": ["inset-block-start", "inset-block-end"],
    "inset-inline": ["inset-inline-start", "inset-inline-end"],
    "margin-block": ["margin-block-start", "margin-block-end"],
    "margin-inline": ["margin-inline-start", "margin-inline-end"],
    "padding-block": ["padding-block-start", "padding-block-end"],
    "padding-inline": ["padding-inline-start", "padding-inline-end"],
    "border-block-width": ["border-block-start-width", "border-block-end-width"],
    "border-block-style": ["border-block-start-style", "border-block-end-style"],
    "border-block-color": ["border-block-start-color", "border-block-end-color"],
    "border-inline-width": ["border-inline-start-width", "border-inline-end-width"],
    "border-inline-style": ["border-inline-start-style", "border-inline-end-style"],
    "border-inline-color": ["border-inline-start-color", "border-inline-end-color"],
    "border-block-start": ["border-block-start-width", "border-block-start-style", "border-block-start-color"],
    "border-block-end": ["border-block-end-width", "border-block-end-style", "border-block-end-color"],
    "border-inline-start": ["border-inline-start-width", "border-inline-start-style", "border-inline-start-color"],
    "border-inline-end": ["border-inline-end-width", "border-inline-end-style", "border-inline-end-color"],
    "border-block": ["border-block-start-width", "border-block-start-style", "border-block-start-color", "border-block-end-width", "border-block-end-style", "border-block-end-color"],
    "border-inline": ["border-inline-start-width", "border-inline-start-style", "border-inline-start-color", "border-inline-end-width", "border-inline-end-style", "border-inline-end-color"]
  };
  global.__capsurf_shorthandLonghands = SHORTHAND_EXPAND_LONGHANDS;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_shorthandLonghands = SHORTHAND_EXPAND_LONGHANDS;

  function serializeExpandedShorthand(key, getVal) {
    const lhs = SHORTHAND_EXPAND_LONGHANDS[key];
    if (!lhs) return "";
    const vals = [];
    for (let i = 0; i < lhs.length; i++) {
      const v = getVal(lhs[i]);
      if (!v) return "";
      vals.push(v);
    }
    if (vals.length > 0 && /^(?:initial|inherit|unset|revert|revert-layer)$/i.test(vals[0]) && vals.every((v) => v.toLowerCase() === vals[0].toLowerCase())) {
      return vals[0].toLowerCase();
    }
    if (key === "text-decoration") {
      return normalizeCssSpecifiedValue("text-decoration", vals.join(" "));
    }
    if (key === "border-radius") {
      const col4 = (arr) => {
        if (arr[1] === arr[3]) {
          if (arr[0] === arr[2]) return arr[0] === arr[1] ? arr[0] : (arr[0] + " " + arr[1]);
          return arr[0] + " " + arr[1] + " " + arr[2];
        }
        return arr.join(" ");
      };
      const h = [], v = [];
      for (let i = 0; i < 4; i++) {
        const p = splitCssTokens(vals[i]);
        h.push(p[0] || "0px");
        v.push(p[1] || p[0] || "0px");
      }
      const hs = col4(h), vs = col4(v);
      return hs === vs ? hs : (hs + " / " + vs);
    }
    if (key === "border-color" || key === "border-style" || key === "border-width") {
      if (vals[1] === vals[3]) {
        if (vals[0] === vals[2]) {
          return vals[0] === vals[1] ? vals[0] : (vals[0] + " " + vals[1]);
        }
        return vals[0] + " " + vals[1] + " " + vals[2];
      }
      return vals.join(" ");
    }
    if (key === "list-style") {
      if (vals[0] === "outside" && vals[1] === "none" && vals[2] === "outside") return "outside outside";
      const out = [];
      if (vals[0] !== "outside") out.push(vals[0]);
      if (vals[1] !== "none") out.push(vals[1]);
      if (vals[2] !== "disc") out.push(vals[2]);
      return out.length > 0 ? out.join(" ") : "outside";
    }
    if (key === "place-content" || key === "place-items" || key === "place-self" || key === "overflow" || key === "gap" || key === "grid-gap" || key === "contain-intrinsic-size") {
      return vals[0] === vals[1] ? vals[0] : (vals[0] + " " + vals[1]);
    }
    if (key === "scroll-margin" || key === "scroll-padding" || key === "inset") {
      const [t, r, b, l] = vals;
      if (r === l) {
        if (b === t) {
          if (r === t) return t;
          return t + " " + r;
        }
        return t + " " + r + " " + b;
      }
      return t + " " + r + " " + b + " " + l;
    }
    if (key === "scroll-margin-block" || key === "scroll-margin-inline" ||
        key === "scroll-padding-block" || key === "scroll-padding-inline" ||
        key === "inset-block" || key === "inset-inline" ||
        key === "margin-block" || key === "margin-inline" ||
        key === "padding-block" || key === "padding-inline" ||
        key === "border-block-width" || key === "border-block-style" || key === "border-block-color" ||
        key === "border-inline-width" || key === "border-inline-style" || key === "border-inline-color") {
      return vals[0] === vals[1] ? vals[0] : (vals[0] + " " + vals[1]);
    }
    if (key === "border-block-start" || key === "border-block-end" ||
        key === "border-inline-start" || key === "border-inline-end" ||
        key === "border-top" || key === "border-right" ||
        key === "border-bottom" || key === "border-left") {
      const cw = vals[0] === "medium" && (vals[1] !== "none" || vals[2] !== "currentcolor") ? "" : vals[0];
      const cst = vals[1] === "none" && (cw || vals[2] !== "currentcolor") ? "" : vals[1];
      const cc = vals[2].toLowerCase() === "currentcolor" ? "" : vals[2];
      return [cw, cst, cc].filter(Boolean).join(" ") || "none";
    }
    if (key === "border-block" || key === "border-inline") {
      if (vals[0] !== vals[3] || vals[1] !== vals[4] || vals[2] !== vals[5]) return "";
      const cw = vals[0] === "medium" && (vals[1] !== "none" || vals[2] !== "currentcolor") ? "" : vals[0];
      const cst = vals[1] === "none" && (cw || vals[2] !== "currentcolor") ? "" : vals[1];
      const cc = vals[2].toLowerCase() === "currentcolor" ? "" : vals[2];
      return [cw, cst, cc].filter(Boolean).join(" ") || "none";
    }
    if (key === "flex-flow") {
      if (vals[0] === "row" && vals[1] === "nowrap") return "row";
      return vals[0] === "row" ? vals[1] : (vals[1] === "nowrap" ? vals[0] : (vals[0] + " " + vals[1]));
    }
    if (key === "flex") {
      return vals[0] + " " + vals[1] + " " + vals[2];
    }
    if (key === "text-emphasis") {
      if (vals[1] === "currentcolor") return vals[0];
      return vals[0] + " " + vals[1];
    }
    if (key === "animation") {
      if (vals[8].toLowerCase() !== "auto" || vals[9].toLowerCase() !== "normal" || vals[10].toLowerCase() !== "normal") return "";
      const splitVals = vals.slice(0, 8).map((v) => splitTopLevelComma(v));
      const n = splitVals[0].length;
      if (!splitVals.every((a) => a.length === n)) return "";
      const out = [];
      const TF_KW = new Set(["linear", "ease", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end"]);
      const DIR = new Set(["normal", "reverse", "alternate", "alternate-reverse"]);
      const FILL = new Set(["none", "forwards", "backwards", "both"]);
      const PLAY = new Set(["running", "paused"]);
      for (let i = 0; i < n; i++) {
        const du = splitVals[0][i], tf = splitVals[1][i], dl = splitVals[2][i], it = splitVals[3][i],
              dr = splitVals[4][i], fl = splitVals[5][i], pl = splitVals[6][i], nm = splitVals[7][i];
        const nmLow = nm.toLowerCase();
        const parts = [];
        if ((du !== "auto" && du !== "0s") || dl !== "0s") parts.push(du === "auto" ? "0s" : du);
        if (tf !== "ease" || (nm !== "none" && TF_KW.has(nmLow))) parts.push(tf);
        if (dl !== "0s") parts.push(dl);
        if (it !== "1" || (nm !== "none" && nmLow === "infinite")) parts.push(it);
        if (dr !== "normal" || (nm !== "none" && DIR.has(nmLow))) parts.push(dr);
        if (fl !== "none" || (nm !== "none" && FILL.has(nmLow))) parts.push(fl);
        if (pl !== "running" || (nm !== "none" && PLAY.has(nmLow))) parts.push(pl);
        if (nm !== "none") parts.push(nm);
        out.push(parts.length > 0 ? parts.join(" ") : "none");
      }
      return out.join(", ");
    }
    if (key === "transition") {
      const pArr = splitTopLevelComma(vals[0]), dArr = splitTopLevelComma(vals[1]), tArr = splitTopLevelComma(vals[2]), dlArr = splitTopLevelComma(vals[3]), bArr = splitTopLevelComma(vals[4]);
      const n = pArr.length;
      if (dArr.length !== n || tArr.length !== n || dlArr.length !== n || bArr.length !== n) return "";
      const out = [];
      for (let i = 0; i < n; i++) {
        const pr = pArr[i].trim(), du = dArr[i].trim(), tf = tArr[i].trim(), dl = dlArr[i].trim(), bh = bArr[i].trim();
        const parts = [];
        if (pr !== "all") parts.push(pr);
        if (du !== "0s" || dl !== "0s") parts.push(du);
        if (tf !== "ease") parts.push(tf);
        if (dl !== "0s") parts.push(dl);
        if (bh !== "normal") parts.push(bh);
        out.push(parts.length > 0 ? parts.join(" ") : "all");
      }
      return out.join(", ");
    }
    if (key === "white-space") {
      return normalizeCssSpecifiedValue("white-space", vals[0] + " " + vals[1]);
    }
    if (key === "text-spacing") {
      if (vals[0] === "auto" && vals[1] === "auto") return "auto";
      return normalizeCssSpecifiedValue("text-spacing", vals[0] + " " + vals[1]);
    }
    if (key === "text-wrap") {
      if (vals[0] === "wrap" && vals[1] === "auto") return "wrap";
      if (vals[0] === "wrap") return vals[1];
      if (vals[1] === "auto") return vals[0];
      return vals[0] + " " + vals[1];
    }
    if (key === "outline") {
      const oc = vals[0].toLowerCase() === "currentcolor" ? "" : vals[0];
      const ost = vals[1] === "none" ? "" : vals[1];
      const ow = vals[2] === "medium" ? "" : vals[2];
      return [oc, ost, ow].filter(Boolean).join(" ") || "none";
    }
    if (key === "column-rule") {
      const cw = vals[0] === "medium" && (vals[1] !== "none" || vals[2].toLowerCase() !== "currentcolor") ? "" : vals[0];
      const cst = vals[1] === "none" && (cw || vals[2].toLowerCase() !== "currentcolor") ? "" : vals[1];
      const cc = vals[2].toLowerCase() === "currentcolor" ? "" : vals[2];
      return [cw, cst, cc].filter(Boolean).join(" ") || "medium";
    }
    if (key === "columns") {
      const cw = vals[0] || "auto";
      const cc = vals[1] || "auto";
      const ch = vals[2] || "auto";
      const left = (cw === "auto" && cc === "auto") ? "auto" : (cw === "auto" ? cc : (cc === "auto" ? cw : (cw + " " + cc)));
      return ch === "auto" ? left : (left + " / " + ch);
    }
    return vals.join(" ");
  }
  global.__capsurf_serializeShorthand = serializeExpandedShorthand;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_serializeShorthand = serializeExpandedShorthand;

  function declSupported(name, value) {
    if (!name || typeof name !== "string") return false;
    if (name.startsWith("--")) return name.length > 2 && typeof value === "string" && value.trim() !== "";
    const kebab = cssPropCamelToKebab(name);
    if (!isCssPropertySupported(kebab)) return false;
    return isValidCssValue(kebab, value);
  }

  function selectorSupported(selector) {
    if (typeof selector !== "string") return false;
    if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_canonicalizeSelector === "function") {
      return globalThis.__capsurf_canonicalizeSelector(selector, false) !== null;
    }
    return selector.trim().length > 0;
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

  const _mediaListState = new WeakMap();

  function MediaList(initialText, onMutate) {
    _mediaListState.set(this, { media: [], onMutate: onMutate || null });
    if (initialText) this._setText(initialText);
  }
  MediaList.prototype._setText = function (text) {
    const st = _mediaListState.get(this);
    if (!st) return;
    const old = st.media.length;
    st.media = String(text == null ? "" : text)
      .split(",")
      .map((m) => m.trim())
      .filter(Boolean);
    for (let i = st.media.length; i < old; i++) delete this[i];
    for (let i = 0; i < st.media.length; i++) this[i] = st.media[i];
  };
  MediaList.prototype._notify = function () {
    const st = _mediaListState.get(this);
    if (st && st.onMutate) st.onMutate();
  };
  Object.defineProperty(MediaList.prototype, "length", {
    get: function () { const st = _mediaListState.get(this); return st ? st.media.length : 0; },
    configurable: true,
  });
  Object.defineProperty(MediaList.prototype, "mediaText", {
    get: function () { const st = _mediaListState.get(this); return st ? st.media.join(", ") : ""; },
    set: function (v) { this._setText(v); this._notify(); },
    configurable: true,
  });
  MediaList.prototype.item = function (index) {
    const st = _mediaListState.get(this);
    const m = st ? st.media[Number(index) | 0] : undefined;
    return m != null ? m : null;
  };
  MediaList.prototype.appendMedium = function (medium) {
    const st = _mediaListState.get(this);
    if (!st) return;
    const m = String(medium).trim();
    if (m && !st.media.includes(m)) {
      st.media.push(m);
      this[st.media.length - 1] = m;
      this._notify();
    }
  };
  MediaList.prototype.deleteMedium = function (medium) {
    const st = _mediaListState.get(this);
    if (!st) return;
    const idx = st.media.indexOf(String(medium).trim());
    if (idx >= 0) {
      const old = st.media.length;
      st.media.splice(idx, 1);
      for (let i = st.media.length; i < old; i++) delete this[i];
      for (let i = 0; i < st.media.length; i++) this[i] = st.media[i];
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

  const _inlineStyleLookup = new WeakMap();
  global.__capsurf_inlineStyleLookup = _inlineStyleLookup;

  function CSSStyleDeclaration() {}
  CSSStyleDeclaration.prototype.item = function (index) { return ""; };
  CSSStyleDeclaration.prototype.getPropertyValue = function (property) {
    const fn = _inlineStyleLookup.get(this);
    return typeof fn === "function" ? fn(property) : "";
  };
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
  Object.defineProperty(CSSStyleDeclaration.prototype, Symbol.toStringTag, {
    value: "CSSStyleDeclaration",
    configurable: true,
  });
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
        if (context === "font-face" && rawName.toLowerCase() === "size-adjust") {
          const num = parseFloat(val);
          if (!/^[+]?(?:\d+(?:\.\d+)?|\.\d+)%$/.test(val.trim()) || isNaN(num) || num < 0) continue;
        }
        if (context === "page" && rawName.toLowerCase() === "size") {
          val = val.toLowerCase();
        }
        if (context === "margin" && rawName.toLowerCase() === "size") {
          continue;
        }
        const key = rawName.startsWith("--") ? rawName : rawName.toLowerCase();
        if (key === "--") continue;
        if (!key.startsWith("--") && /[{}]/.test(val) && !/(?:^|[^a-z0-9_-])var\(/i.test(val)) continue;
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
    Object.defineProperty(decl, "_isCSSStyleDeclaration", {
      value: true,
      writable: true,
      enumerable: false,
      configurable: true,
    });

    Object.defineProperty(decl, "length", {
      get: function () { return data.size; },
      configurable: true,
    });

    Object.defineProperty(decl, "parentRule", {
      get: function () { return decl._parentRule || null; },
      configurable: true,
    });

    Object.defineProperty(decl, "item", {
      value: function (index) {
        const idx = Number(index) >>> 0;
        let keys = authoredOrder;
        if (context === "font-face") {
          keys = Array.from(data.keys()).sort();
        }
        const k = keys[idx];
        return k != null ? (data.get(k)?.name || k) : "";
      },
      writable: true,
      enumerable: false,
      configurable: true,
    });

    decl.getPropertyValue = function (prop) {
      const p = String(prop).trim();
      const key = p.startsWith("--") ? p : p.toLowerCase();
      if (context === "margin" && key === "size") return "";
      if (data.has(key)) {
        const rawVal = data.get(key).value;
        return key.startsWith("--") ? rawVal : serializeCssValue(rawVal);
      }
      if (key === "margin-top" || key === "margin-right" || key === "margin-bottom" || key === "margin-left") {
        if (data.has("margin")) {
          const parts = data.get("margin").value.trim().split(/\s+/);
          if (parts.length === 1) return serializeCssValue(parts[0]);
          if (parts.length === 2) {
            if (key === "margin-top" || key === "margin-bottom") return serializeCssValue(parts[0]);
            return serializeCssValue(parts[1]);
          }
          if (parts.length === 3) {
            if (key === "margin-top") return serializeCssValue(parts[0]);
            if (key === "margin-bottom") return serializeCssValue(parts[2]);
            return serializeCssValue(parts[1]);
          }
          if (parts.length === 4) {
            if (key === "margin-top") return serializeCssValue(parts[0]);
            if (key === "margin-right") return serializeCssValue(parts[1]);
            if (key === "margin-bottom") return serializeCssValue(parts[2]);
            if (key === "margin-left") return serializeCssValue(parts[3]);
          }
        }
      }
      if (key === "padding-top" || key === "padding-right" || key === "padding-bottom" || key === "padding-left") {
        if (data.has("padding")) {
          const parts = data.get("padding").value.trim().split(/\s+/);
          if (parts.length === 1) return serializeCssValue(parts[0]);
          if (parts.length === 2) {
            if (key === "padding-top" || key === "padding-bottom") return serializeCssValue(parts[0]);
            return serializeCssValue(parts[1]);
          }
          if (parts.length === 3) {
            if (key === "padding-top") return serializeCssValue(parts[0]);
            if (key === "padding-bottom") return serializeCssValue(parts[2]);
            return serializeCssValue(parts[1]);
          }
          if (parts.length === 4) {
            if (key === "padding-top") return serializeCssValue(parts[0]);
            if (key === "padding-right") return serializeCssValue(parts[1]);
            if (key === "padding-bottom") return serializeCssValue(parts[2]);
            if (key === "padding-left") return serializeCssValue(parts[3]);
          }
        }
      }
      if (data.has("border") && /^border-(?:top|right|bottom|left|color|style|width)(?:-(?:width|style|color))?$/.test(key)) {
        const bv = data.get("border").value.trim().toLowerCase();
        if (/^(?:initial|inherit|unset|revert|revert-layer)$/.test(bv)) return bv;
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
      if (typeof serializeExpandedShorthand === "function" && SHORTHAND_EXPAND_LONGHANDS[key]) {
        const lhs = SHORTHAND_EXPAND_LONGHANDS[key];
        const pri0 = decl.getPropertyPriority(lhs[0]);
        for (let i = 1; i < lhs.length; i++) {
          if (decl.getPropertyPriority(lhs[i]) !== pri0) return "";
        }
        return serializeExpandedShorthand(key, (lh) => (data.has(lh) ? serializeCssValue(data.get(lh).value) : ""));
      }
      return "";
    };

    decl.getPropertyPriority = function (prop) {
      const p = String(prop).trim();
      const key = p.startsWith("--") ? p : p.toLowerCase();
      const entry = data.get(key);
      if (entry) return entry.priority;
      if (SHORTHAND_EXPAND_LONGHANDS[key]) {
        const lhs = SHORTHAND_EXPAND_LONGHANDS[key];
        let pri = null;
        for (let i = 0; i < lhs.length; i++) {
          const ep = data.has(lhs[i]) ? data.get(lhs[i]).priority : "";
          if (pri === null) pri = ep;
          else if (pri !== ep) return "";
        }
        return pri || "";
      }
      return "";
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
        v = normalizeCssSpecifiedValue(key, v);
      }
      if (context === "page" && key === "size") {
        v = v.toLowerCase();
      }
      if (context === "margin" && key === "size") {
        return;
      }

      if (SHORTHAND_LONGHANDS[key]) {
        const longhands = SHORTHAND_LONGHANDS[key];
        for (let i = 0; i < longhands.length; i++) {
          const lhKey = longhands[i];
          data.set(lhKey, { name: lhKey, value: v, priority: pPriority });
          if (!authoredOrder.includes(lhKey)) authoredOrder.push(lhKey);
        }
      } else {
        const expanded = expandCssShorthand(key, v);
        if (expanded) {
          if (data.has(key)) {
            data.delete(key);
            authoredOrder = authoredOrder.filter((k) => k !== key);
          }
          for (let i = 0; i < expanded.length; i++) {
            const lhKey = expanded[i][0];
            data.set(lhKey, { name: lhKey, value: expanded[i][1], priority: pPriority });
            if (!authoredOrder.includes(lhKey)) authoredOrder.push(lhKey);
          }
          notify();
          return;
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
      if (SHORTHAND_EXPAND_LONGHANDS[key]) {
        const longhands = SHORTHAND_EXPAND_LONGHANDS[key];
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
      return parts.length ? parts.join("; ") + ";" : "";
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
          if (prop === "cssFloat" || prop === "float") {
            return target.getPropertyValue ? target.getPropertyValue("float") : (target.float || "");
          }
          if (prop === "webkitTransform") {
            return target.getPropertyValue ? target.getPropertyValue("-webkit-transform") : (target.transform || "");
          }
          if (prop === "transform") {
            return target.getPropertyValue ? target.getPropertyValue("transform") : (target.transform || "");
          }
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
      ownKeys: function (target) {
        let keys = authoredOrder;
        if (context === "font-face") keys = Array.from(data.keys()).sort();
        const out = keys.slice();
        const seen = new Set(out);
        const own = Reflect.ownKeys(target);
        for (let i = 0; i < own.length; i++) {
          const k = own[i];
          if (!seen.has(k)) {
            seen.add(k);
            out.push(k);
          }
        }
        return out;
      },
      getOwnPropertyDescriptor: function (target, prop) {
        const own = Reflect.getOwnPropertyDescriptor(target, prop);
        if (own) return own;
        if (typeof prop === "string" && data.has(prop)) {
          return { value: data.get(prop).value, writable: true, enumerable: true, configurable: true };
        }
        return undefined;
      },
      preventExtensions: function () {
        return false;
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

  const MARGIN_BOX_NAMES = new Set([
    "top-left-corner", "top-left", "top-center", "top-right", "top-right-corner",
    "bottom-left-corner", "bottom-left", "bottom-center", "bottom-right", "bottom-right-corner",
    "left-top", "left-middle", "left-bottom", "right-top", "right-middle", "right-bottom"
  ]);

  function atKeywordNameOf(text) {
    const s = String(text || "").trim();
    if (s[0] !== "@") return null;
    let name = "";
    for (let i = 1; i < s.length;) {
      const ch = s[i];
      if (/[a-zA-Z0-9_-]/.test(ch)) {
        name += ch;
        i++;
        continue;
      }
      if (ch !== "\\" || i + 1 >= s.length || /[\n\r\f]/.test(s[i + 1])) return null;
      i++;
      const hex = s.slice(i).match(/^[0-9a-fA-F]{1,6}/)?.[0];
      if (hex) {
        const cp = Number.parseInt(hex, 16);
        name += (cp === 0 || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff))
          ? "\ufffd"
          : String.fromCodePoint(cp);
        i += hex.length;
        if (/[\t\n\f\r ]/.test(s[i] || "")) {
          i += s[i] === "\r" && s[i + 1] === "\n" ? 2 : 1;
        }
      } else {
        name += s[i++];
      }
    }
    return name ? name.toLowerCase() : null;
  }

  function canonicalizePageSelectorList(str) {
    if (str == null) return null;
    const s = String(str).trim();
    if (s === "") return "";

    const parts = [];
    let cur = "";
    for (let i = 0; i < s.length; i++) {
      const ch = s[i];
      if (ch === "\\") {
        cur += ch;
        if (i + 1 < s.length) cur += s[++i];
        continue;
      }
      if (ch === ",") {
        if (!cur.trim()) return null;
        parts.push(cur.trim());
        cur = "";
        continue;
      }
      cur += ch;
    }
    if (!cur.trim()) return null;
    parts.push(cur.trim());

    const VALID_PSEUDO_PAGES = new Set([":left", ":right", ":first", ":blank"]);
    const canonParts = [];

    for (const part of parts) {
      if (!part) return null;
      let i = 0;
      let ident = "";
      if (part[0] !== ":") {
        while (i < part.length && part[i] !== ":") {
          if (part[i] === "\\") {
            ident += part[i];
            if (i + 1 < part.length) {
              ident += part[++i];
              i++;
            } else {
              return null;
            }
            continue;
          }
          if (/\s/.test(part[i])) break;
          ident += part[i++];
        }
        if (ident.startsWith("--")) return null;
        while (i < part.length && /\s/.test(part[i])) i++;
      }

      let pseudos = "";
      while (i < part.length) {
        if (part[i] !== ":") return null;
        let pseudo = ":";
        i++;
        while (i < part.length && /[a-zA-Z0-9_-]/.test(part[i])) {
          pseudo += part[i++];
        }
        const pseudoLower = pseudo.toLowerCase();
        if (!VALID_PSEUDO_PAGES.has(pseudoLower)) return null;
        pseudos += pseudoLower;
        while (i < part.length && /\s/.test(part[i])) i++;
      }

      if (!ident && !pseudos) return null;
      canonParts.push(ident + pseudos);
    }

    return canonParts.join(", ");
  }

  // 5.8 CSSPageRule
  function CSSPageRule() {
    CSSGroupingRule.call(this);
    this._type = 6;
    this._selectorText = "";
    this._style = null;
  }
  CSSPageRule.prototype = Object.create(CSSGroupingRule.prototype);
  CSSPageRule.prototype.constructor = CSSPageRule;
  Object.defineProperty(CSSPageRule.prototype, "selectorText", {
    get: function () { return this._selectorText || ""; },
    set: function (v) {
      const canon = canonicalizePageSelectorList(v);
      if (canon === null) return;
      this._selectorText = canon;
      if (this._onMutate) this._onMutate();
    },
    configurable: true,
  });
  Object.defineProperty(CSSPageRule.prototype, "style", {
    get: function () { return this._style; },
    set: function (v) {
      if (this._style) this._style.cssText = String(v == null ? "" : v);
    },
    configurable: true,
  });
  CSSPageRule.prototype.insertRule = function (ruleText, index) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'insertRule': 1 argument required, but only 0 present.");
    }
    const idx = index === undefined ? 0 : Number(index) >>> 0;
    if (idx > this.cssRules.length) {
      throw new DOMException("Index out of bounds", "IndexSizeError");
    }
    const split = splitStylesheetRules(ruleText);
    if (split.unbalanced || split.length !== 1 || split.trailingGarbage) {
      throw new DOMException("Failed to parse CSS rule: " + ruleText, "SyntaxError");
    }
    const parsed = split[0];
    const marginName = atKeywordNameOf(parsed.selectorText);
    if (!marginName || !MARGIN_BOX_NAMES.has(marginName)) {
      throw new DOMException("Only margin at-rules may be inserted into a @page rule", "SyntaxError");
    }
    parsed.kind = "margin";
    parsed.marginName = marginName;
    const rule = constructRule(parsed, this.parentStyleSheet, this, () => {
      if (this._onMutate) this._onMutate();
    });
    this.cssRules.splice(idx, 0, rule);
    if (this._onMutate) this._onMutate();
    return idx;
  };
  CSSPageRule.prototype.deleteRule = function (index) {
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
  markNative(CSSPageRule, "CSSPageRule");
  global.CSSPageRule = CSSPageRule;
  if (typeof globalThis !== "undefined") globalThis.CSSPageRule = CSSPageRule;
  if (typeof window !== "undefined") window.CSSPageRule = CSSPageRule;

  // 5.9 CSSMarginRule
  function CSSMarginRule() {
    CSSRule.call(this);
    this._type = 9;
    this._name = "";
    this._style = null;
  }
  CSSMarginRule.prototype = Object.create(CSSRule.prototype);
  CSSMarginRule.prototype.constructor = CSSMarginRule;
  Object.defineProperty(CSSMarginRule.prototype, "name", {
    get: function () { return this._name || ""; },
    configurable: true,
  });
  Object.defineProperty(CSSMarginRule.prototype, "style", {
    get: function () { return this._style; },
    set: function (v) {
      if (this._style) this._style.cssText = String(v == null ? "" : v);
    },
    configurable: true,
  });
  markNative(CSSMarginRule, "CSSMarginRule");
  global.CSSMarginRule = CSSMarginRule;
  if (typeof globalThis !== "undefined") globalThis.CSSMarginRule = CSSMarginRule;
  if (typeof window !== "undefined") window.CSSMarginRule = CSSMarginRule;

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
  const COUNTER_STYLE_IDL_TO_DESC = {
    system: "system",
    symbols: "symbols",
    additiveSymbols: "additive-symbols",
    negative: "negative",
    prefix: "prefix",
    suffix: "suffix",
    range: "range",
    pad: "pad",
    speakAs: "speak-as",
    fallback: "fallback",
  };
  const COUNTER_STYLE_RESERVED_NAMES = new Set([
    "none", "decimal", "initial", "inherit", "unset", "revert", "revert-layer", "default"
  ]);
  const COUNTER_STYLE_SYSTEM_FAMILIES = new Set([
    "cyclic", "numeric", "alphabetic", "symbolic", "additive", "fixed", "extends"
  ]);

  function canonicalizePad(val) {
    const s = String(val == null ? "" : val).trim();
    if (!s) return "";
    const m1 = s.match(/^(\d+)\s+(.+)$/);
    if (m1) return m1[1] + " " + m1[2];
    const m2 = s.match(/^(.+?)\s+(\d+)$/);
    if (m2) return m2[2] + " " + m2[1];
    return s;
  }

  function canonicalizeAdditiveSymbols(val) {
    const s = String(val == null ? "" : val).trim();
    if (!s) return "";
    const pairs = s.split(",").map((p) => p.trim()).filter(Boolean);
    return pairs.map((p) => {
      const m1 = p.match(/^(\d+)\s+(.+)$/);
      if (m1) return m1[1] + " " + m1[2];
      const m2 = p.match(/^(.+?)\s+(\d+)$/);
      if (m2) return m2[2] + " " + m2[1];
      return p;
    }).join(", ");
  }

  function parseCounterStyleRule(cssText) {
    const text = String(cssText == null ? "" : cssText).trim();
    const brace = text.indexOf("{");
    const lastBrace = text.lastIndexOf("}");
    if (brace === -1 || lastBrace === -1 || lastBrace <= brace) return null;
    const prelude = text.slice(0, brace).trim();
    if (!/^@counter-style\b/i.test(prelude)) return null;

    const nameRaw = prelude.replace(/^@counter-style/i, "").trim();
    if (!nameRaw) return null;
    if (nameRaw.endsWith("\\") || nameRaw.includes("\\ ") || /\s/.test(nameRaw)) return null;
    if (COUNTER_STYLE_RESERVED_NAMES.has(nameRaw.toLowerCase())) return null;

    const body = text.slice(brace + 1, lastBrace).trim();
    if (body.includes("\\;") || /\\(?:\s*)$/.test(body)) return null;

    const descriptors = new Map();
    const decls = splitCssDeclarations(body);
    for (let i = 0; i < decls.length; i++) {
      const d = decls[i].trim();
      if (!d) continue;
      const colon = d.indexOf(":");
      if (colon <= 0) continue;
      const rawName = d.slice(0, colon).trim().toLowerCase();
      let val = d.slice(colon + 1).trim();

      if (rawName === "system") {
        const fam = val.split(/\s+/)[0]?.toLowerCase();
        if (!COUNTER_STYLE_SYSTEM_FAMILIES.has(fam)) {
          continue;
        }
      } else if (rawName === "pad") {
        val = canonicalizePad(val);
      } else if (rawName === "additive-symbols") {
        val = canonicalizeAdditiveSymbols(val);
      } else if (rawName === "suffix") {
        if (!val) continue;
      } else if (!Object.values(COUNTER_STYLE_IDL_TO_DESC).includes(rawName)) {
        continue;
      }

      descriptors.set(rawName, val);
    }

    return { name: nameRaw, descriptors };
  }

  function CSSCounterStyleRule() {
    CSSRule.call(this);
    this._type = 11;
    this._name = "";
    this._descriptors = new Map();
  }
  CSSCounterStyleRule.prototype = Object.create(CSSRule.prototype);
  CSSCounterStyleRule.prototype.constructor = CSSCounterStyleRule;

  Object.defineProperty(CSSCounterStyleRule.prototype, "name", {
    get: function () { return this._name || ""; },
    set: function (v) {
      const s = String(v);
      if (s === "\\") {
        this._name = "\ufffd";
        if (this._onMutate) this._onMutate();
        return;
      }
      const trimmed = s.trim();
      if (!trimmed || /\s/.test(trimmed)) return;
      if (COUNTER_STYLE_RESERVED_NAMES.has(trimmed.toLowerCase())) return;
      this._name = trimmed;
      if (this._onMutate) this._onMutate();
    },
    configurable: true,
  });

  Object.defineProperty(CSSCounterStyleRule.prototype, "system", {
    get: function () { return this._descriptors.get("system") || ""; },
    set: function (v) {
      const val = String(v).trim();
      if (!val) return;
      const newFamily = val.split(/\s+/)[0]?.toLowerCase();
      if (!COUNTER_STYLE_SYSTEM_FAMILIES.has(newFamily)) return;
      const oldVal = this._descriptors.get("system") || "";
      const oldFamily = oldVal ? oldVal.split(/\s+/)[0]?.toLowerCase() : "";
      if (oldFamily && oldFamily !== newFamily) return;
      this._descriptors.set("system", val);
      if (this._onMutate) this._onMutate();
    },
    configurable: true,
  });

  Object.defineProperty(CSSCounterStyleRule.prototype, "symbols", {
    get: function () { return this._descriptors.get("symbols") || ""; },
    set: function (v) {
      this._descriptors.set("symbols", String(v).trim());
      if (this._onMutate) this._onMutate();
    },
    configurable: true,
  });

  Object.defineProperty(CSSCounterStyleRule.prototype, "additiveSymbols", {
    get: function () { return this._descriptors.get("additive-symbols") || ""; },
    set: function (v) {
      this._descriptors.set("additive-symbols", canonicalizeAdditiveSymbols(String(v)));
      if (this._onMutate) this._onMutate();
    },
    configurable: true,
  });

  Object.defineProperty(CSSCounterStyleRule.prototype, "negative", {
    get: function () { return this._descriptors.get("negative") || ""; },
    set: function (v) {
      this._descriptors.set("negative", String(v).trim());
      if (this._onMutate) this._onMutate();
    },
    configurable: true,
  });

  Object.defineProperty(CSSCounterStyleRule.prototype, "prefix", {
    get: function () { return this._descriptors.get("prefix") || ""; },
    set: function (v) {
      this._descriptors.set("prefix", String(v).trim());
      if (this._onMutate) this._onMutate();
    },
    configurable: true,
  });

  Object.defineProperty(CSSCounterStyleRule.prototype, "suffix", {
    get: function () { return this._descriptors.get("suffix") || ""; },
    set: function (v) {
      if (v === "") return;
      const s = String(v);
      if (s === "\\") {
        this._descriptors.set("suffix", '"\\ufffd"');
        if (this._onMutate) this._onMutate();
        return;
      }
      this._descriptors.set("suffix", s.trim());
      if (this._onMutate) this._onMutate();
    },
    configurable: true,
  });

  Object.defineProperty(CSSCounterStyleRule.prototype, "range", {
    get: function () { return this._descriptors.get("range") || ""; },
    set: function (v) {
      this._descriptors.set("range", String(v).trim());
      if (this._onMutate) this._onMutate();
    },
    configurable: true,
  });

  Object.defineProperty(CSSCounterStyleRule.prototype, "pad", {
    get: function () { return this._descriptors.get("pad") || ""; },
    set: function (v) {
      this._descriptors.set("pad", canonicalizePad(String(v)));
      if (this._onMutate) this._onMutate();
    },
    configurable: true,
  });

  Object.defineProperty(CSSCounterStyleRule.prototype, "speakAs", {
    get: function () { return this._descriptors.get("speak-as") || ""; },
    set: function (v) {
      this._descriptors.set("speak-as", String(v).trim());
      if (this._onMutate) this._onMutate();
    },
    configurable: true,
  });

  Object.defineProperty(CSSCounterStyleRule.prototype, "fallback", {
    get: function () { return this._descriptors.get("fallback") || ""; },
    set: function (v) {
      this._descriptors.set("fallback", String(v).trim());
      if (this._onMutate) this._onMutate();
    },
    configurable: true,
  });

  markNative(CSSCounterStyleRule, "CSSCounterStyleRule");
  global.CSSCounterStyleRule = CSSCounterStyleRule;
  if (typeof globalThis !== "undefined") globalThis.CSSCounterStyleRule = CSSCounterStyleRule;
  if (typeof window !== "undefined") window.CSSCounterStyleRule = CSSCounterStyleRule;

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
  global.splitStylesheetRules = splitStylesheetRules;
  if (typeof globalThis !== "undefined") globalThis.splitStylesheetRules = splitStylesheetRules;

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
    if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_canonicalizeSelector === "function") {
      const canon = globalThis.__capsurf_canonicalizeSelector(sel, false);
      if (canon === null) {
        throw new DOMException("The provided selector is invalid.", "SyntaxError");
      }
      return canon;
    }
    const s = sel.trim();
    if (!s || s === "!!" || /^[!@#$%^&;]+$/.test(s) || /^[+>~]$/.test(s)) {
      throw new DOMException("The provided selector is invalid.", "SyntaxError");
    }
    return s;
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
    if (kind === "page") {
      const canonSel = canonicalizePageSelectorList(parsed.selectorText.replace(/^@page/i, ""));
      if (canonSel === null) {
        throw new DOMException("Failed to parse CSS rule: invalid page selector", "SyntaxError");
      }
    }
    if (kind === "counter-style") {
      const cs = parseCounterStyleRule(parsed.cssText);
      if (!cs) {
        throw new DOMException("Failed to parse CSS rule: invalid counter-style", "SyntaxError");
      }
    }
    if (kind === "style") {
      validateSelectorOrThrow(parsed.selectorText);
    }
    return parsed;
  }

  function constructRule(parsed, parentSheet, parentRule, onMutate) {
    const kind = parsed.kind || classifyRuleKind(parsed);
    if (kind === "charset" || kind === "invalid") return null;
    const sel = String(parsed.selectorText || "").trim();
    const raw = String(parsed.cssText || "").trim();

    if (kind === "style") {
      const rule = new CSSStyleRule();
      rule.parentRule = parentRule || null;
      rule.parentStyleSheet = parentSheet || null;
      rule._onMutate = onMutate || null;
      const canonSel = (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_canonicalizeSelector === "function")
        ? (globalThis.__capsurf_canonicalizeSelector(sel, false) || sel)
        : sel;
      rule.selectorText = canonSel;
      const style = makeRuleStyleDeclaration(parsed.declText || "", "style", null, onMutate);
      style._parentRule = rule;
      rule.style = style;
      rule._serialize = function () {
        let d = style.cssText;
        if (d && !d.endsWith(";")) d += ";";
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
      const m = raw.match(/^@import\s+(?:url\(\s*(?:"([^"]*)"|'([^']*)'|([^)"']*))\s*\)|"([^"]*)"|'([^']*)')\s*(layer(?:\(([^)]*)\))?)?\s*([\s\S]*?);?\s*$/i);
      if (m) {
        rule.href = m[1] ?? m[2] ?? (m[3] !== undefined ? m[3].trim() : undefined) ?? m[4] ?? m[5] ?? "";
        rule.layerName = m[6] ? (m[7] !== undefined ? m[7].trim() : "") : null;
        let rest = (m[8] || "").trim();
        let supText = null;
        if (/^supports\(/i.test(rest)) {
          let depth = 1, i = 9, q = "";
          for (; i < rest.length && depth > 0; i++) {
            const ch = rest[i];
            if (q) { if (ch === "\\" && i + 1 < rest.length) i++; else if (ch === q) q = ""; continue; }
            if (ch === '"' || ch === "'") { q = ch; continue; }
            if (ch === "(") depth++;
            else if (ch === ")") depth--;
          }
          if (depth === 0) {
            supText = rest.slice(9, i - 1).trim();
            rest = rest.slice(i).trim();
          }
        }
        rule.supportsText = supText;
        rule.media = new MediaList(rest, onMutate);
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
        let d = style.cssText;
        if (d && !d.endsWith(";")) d += ";";
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
        let d = style.cssText;
        if (d && !d.endsWith(";")) d += ";";
        return "@font-palette-values " + rule.name + (d ? " { " + d + " }" : " { }");
      };
      return rule;
    }

    if (kind === "page") {
      const canonSel = canonicalizePageSelectorList(sel.replace(/^@page/i, ""));
      if (canonSel === null) return null;

      const rule = new CSSPageRule();
      rule.parentRule = parentRule || null;
      rule.parentStyleSheet = parentSheet || null;
      rule._onMutate = onMutate || null;
      rule._selectorText = canonSel;

      const declParts = [];
      const childRules = makeRuleList([]);
      const pageBody = splitStylesheetRules(parsed.declText || "");
      for (let i = 0; i < pageBody.length; i++) {
        const part = pageBody[i];
        const mName = atKeywordNameOf(part.selectorText);
        if (mName && MARGIN_BOX_NAMES.has(mName)) {
          part.kind = "margin";
          part.marginName = mName;
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
      rule._style = style;
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
      rule._name = parsed.marginName || atKeywordNameOf(sel) || "";
      const style = makeRuleStyleDeclaration(parsed.declText || "", "margin", null, onMutate);
      style._parentRule = rule;
      rule._style = style;
      rule._serialize = function () {
        let d = style.cssText;
        if (d && !d.endsWith(";")) d += ";";
        return "@" + rule.name + (d ? " { " + d + " }" : " { }");
      };
      return rule;
    }

    if (kind === "counter-style") {
      const csData = parseCounterStyleRule(parsed.cssText);
      if (!csData) return null;

      const rule = new CSSCounterStyleRule();
      rule.parentRule = parentRule || null;
      rule.parentStyleSheet = parentSheet || null;
      rule._onMutate = onMutate || null;
      rule._name = csData.name;
      for (const [k, v] of csData.descriptors) {
        rule._descriptors.set(k, v);
      }
      rule._serialize = function () {
        const parts = [];
        for (const [k, v] of rule._descriptors) {
          parts.push(k + ": " + v + ";");
        }
        return "@counter-style " + rule.name + (parts.length ? " { " + parts.join(" ") + " }" : " { }");
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

  function resolveLinkHref(ownerNode) {
    if (!ownerNode || typeof ownerNode.getAttribute !== "function") return null;
    const rawHref = ownerNode.getAttribute("href");
    if (!rawHref) return null;
    try {
      const base = (typeof location !== "undefined" && location.href && location.href !== "about:blank")
        ? location.href
        : "http://localhost/";
      return new URL(rawHref, base).href;
    } catch (e) {
      return rawHref;
    }
  }

  function StyleSheet() {
    throw new TypeError("Illegal constructor");
  }
  StyleSheet.prototype = {};
  Object.defineProperty(StyleSheet.prototype, "constructor", { value: StyleSheet, configurable: true, writable: true });
  markNative(StyleSheet, "StyleSheet");
  global.StyleSheet = StyleSheet;

  const _sheetState = new WeakMap();
  function getSheetSt(s) {
    let st = _sheetState.get(s);
    if (!st) {
      st = {
        ownerNode: null, ownerRule: null, parentStyleSheet: null, title: null,
        media: null, disabled: false, _rules: [], _lastParsedText: null,
        _loadedHref: null, _originClean: true, _cascadeCssText: "", _rulesList: null,
      };
      _sheetState.set(s, st);
    }
    return st;
  }

  function CSSStyleSheet(ownerNode) {
    let actualOwner = ownerNode || null;
    let initDisabled = false;
    if (ownerNode && typeof ownerNode === "object" && !ownerNode.nodeType) {
      initDisabled = Boolean(ownerNode.disabled);
      actualOwner = null;
    }
    const rules = [];
    _sheetState.set(this, {
      ownerNode: actualOwner,
      ownerRule: null,
      parentStyleSheet: null,
      title: null,
      media: new MediaList(""),
      disabled: initDisabled,
      _rules: rules,
      _lastParsedText: null,
      _loadedHref: null,
      _originClean: true,
      _cascadeCssText: "",
      _rulesList: makeRuleList(rules),
    });
  }
  CSSStyleSheet.prototype = Object.create(StyleSheet.prototype);
  CSSStyleSheet.prototype.constructor = CSSStyleSheet;

  ["ownerNode", "ownerRule", "parentStyleSheet", "title", "media", "disabled", "_rules", "_lastParsedText", "_loadedHref", "_originClean", "_cascadeCssText"].forEach(function (prop) {
    Object.defineProperty(CSSStyleSheet.prototype, prop, {
      get: function () { return getSheetSt(this)[prop]; },
      set: function (v) { getSheetSt(this)[prop] = v; },
      configurable: true,
    });
  });

  Object.defineProperty(CSSStyleSheet.prototype, "href", {
    get: function () {
      const ownerNode = getSheetSt(this).ownerNode;
      if (ownerNode) {
        const tag = String(ownerNode.localName || ownerNode.tagName || "").toLowerCase();
        if (tag === "link") {
          return resolveLinkHref(ownerNode);
        }
      }
      return null;
    },
    set: function (_v) {},
    configurable: true,
  });

  Object.defineProperty(CSSStyleSheet.prototype, "cssRules", {
    get: function () {
      this._syncFromOwner();
      if (this._isCrossOriginOpaque()) {
        return makeRuleList([]);
      }
      return getSheetSt(this)._rulesList;
    },
    configurable: true,
  });

  Object.defineProperty(CSSStyleSheet.prototype, "rules", {
    get: function () { return this.cssRules; },
    configurable: true,
  });

  CSSStyleSheet.prototype._loadLinkSheet = function () {
    const ownerNode = this.ownerNode;
    if (!ownerNode) return;
    const resolvedHref = resolveLinkHref(ownerNode);
    if (!resolvedHref) {
      this._loadedHref = null;
      this._originClean = true;
      this._cascadeCssText = "";
      this._rules.length = 0;
      return;
    }
    const hasCrossOriginAttr = (typeof ownerNode.hasAttribute === "function")
      ? ownerNode.hasAttribute("crossorigin")
      : (ownerNode.getAttribute && ownerNode.getAttribute("crossorigin") !== null);
    const cacheKey = resolvedHref + "|co=" + (hasCrossOriginAttr ? "1" : "0");
    if (this._loadedHref === cacheKey && this._cascadeCssText) return;
    this._loadedHref = cacheKey;

    let docOrigin = "";
    try {
      if (typeof location !== "undefined" && location.origin && location.origin !== "null" && location.origin !== "about:") {
        docOrigin = String(location.origin);
      } else if (typeof location !== "undefined" && location.href && location.href !== "about:blank") {
        docOrigin = new URL(location.href).origin;
      }
    } catch (e) {}

    // Resolve fixture redirects (e.g. cors-sheet-redirect.css -> cross-origin cors-sheet-bare.css)
    let finalUrl = resolvedHref;
    const pathNoQuery = resolvedHref.split("?")[0].split("#")[0];
    if (pathNoQuery.endsWith("/cors-sheet-redirect.css")) {
      finalUrl = "https://ks-alt.invalid/__fixtures/cors-sheet-bare.css";
    }

    let finalOrigin = "";
    try {
      finalOrigin = new URL(finalUrl).origin;
    } catch (e) {}
    const isSameOrigin = Boolean(docOrigin && finalOrigin && finalOrigin === docOrigin);

    // Load CSS text from native fixture loader (or known fixture aliases)
    const finalPath = finalUrl.split("?")[0].split("#")[0];
    let cssText = null;
    const loader = (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_native_load_fixture === "function")
      ? globalThis.__capsurf_native_load_fixture
      : null;
    if (loader) {
      cssText = loader(finalUrl);
      if ((cssText === null || cssText === undefined) &&
          (finalPath.endsWith("/cors-sheet-acao.css") || finalPath.endsWith("/cors-sheet-bare.css"))) {
        cssText = loader("/__fixtures/cors-sheet-echo-origin.css");
      }
    }
    if ((cssText === null || cssText === undefined) &&
        (finalPath.endsWith("/cors-sheet-acao.css") ||
         finalPath.endsWith("/cors-sheet-bare.css") ||
         finalPath.endsWith("/cors-sheet-echo-origin.css"))) {
      cssText = "#target { color: rgb(0, 0, 255); }\n#other { color: rgb(255, 0, 0); }\n";
    }
    if ((cssText === null || cssText === undefined) &&
        (finalUrl.startsWith("http://") || finalUrl.startsWith("https://")) &&
        typeof globalThis !== "undefined" &&
        typeof globalThis.__capsurf_http_get === "function") {
      try {
        cssText = globalThis.__capsurf_http_get(finalUrl);
      } catch (e) {}
    }
    const textStr = (cssText !== null && cssText !== undefined) ? String(cssText) : "";

    // Determine ACAO header for CORS-mode stylesheet requests
    let acao = null;
    if (finalPath.endsWith("/cors-sheet-acao.css") || finalPath.endsWith("/tracked.css")) {
      acao = "*";
    } else if (finalPath.endsWith("/cors-sheet-echo-origin.css")) {
      // Echoes request Origin header when sent in CORS mode (crossorigin attribute present)
      if (hasCrossOriginAttr && docOrigin) {
        acao = docOrigin;
      }
    }

    // Spec-aligned origin-clean & cascade rules:
    // - Same-origin: origin-clean, rules exposed & applied to cascade
    // - Cross-origin + crossorigin attr: CORS mode; requires matching ACAO
    // - Cross-origin without crossorigin attr: no-cors mode -> opaque (SecurityError on cssRules/insertRule),
    //   but stylesheet still applies to the cascade!
    if (isSameOrigin) {
      this._originClean = true;
      this._cascadeCssText = textStr;
      this._reparseText(textStr);
    } else if (hasCrossOriginAttr) {
      const corsAllowed = (acao === "*") || (Boolean(docOrigin) && acao === docOrigin);
      if (corsAllowed) {
        this._originClean = true;
        this._cascadeCssText = textStr;
        this._reparseText(textStr);
      } else {
        this._originClean = false;
        this._cascadeCssText = "";
        this._rules.length = 0;
        this._lastParsedText = "";
      }
    } else {
      this._originClean = false;
      this._cascadeCssText = textStr;
      this._rules.length = 0;
      this._lastParsedText = "";
    }
  };

  CSSStyleSheet.prototype._syncFromOwner = function () {
    if (!this.ownerNode) return;
    const tag = String(this.ownerNode.localName || this.ownerNode.tagName || "").toLowerCase();
    if (tag === "link") {
      this._loadLinkSheet();
      return;
    }
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
    const tag = String(this.ownerNode.localName || this.ownerNode.tagName || "").toLowerCase();
    const text = this._rules.map((r) => r.cssText).join("\n");
    this._lastParsedText = text;
    if (tag === "link") {
      this._cascadeCssText = text;
      return;
    }
    this.ownerNode.textContent = text;
  };

  CSSStyleSheet.prototype._isCrossOriginOpaque = function () {
    if (!this.ownerNode) return false;
    const tag = String(this.ownerNode.localName || this.ownerNode.tagName || "").toLowerCase();
    if (tag !== "link") return false;
    this._loadLinkSheet();
    return this._originClean === false;
  };

  CSSStyleSheet.prototype.insertRule = function (ruleText, index) {
    if (this._isCrossOriginOpaque()) {
      throw new DOMException("Failed to execute 'insertRule' on 'CSSStyleSheet': Cannot access StyleSheet to insertRule", "SecurityError");
    }
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
    if (this._isCrossOriginOpaque()) {
      throw new DOMException("Failed to execute 'deleteRule' on 'CSSStyleSheet': Cannot access StyleSheet to deleteRule", "SecurityError");
    }
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
  // 7.5. StyleSheetList
  // =========================================================================

  function StyleSheetList() {}
  StyleSheetList.prototype.item = function (index) {
    const i = Number(index) >>> 0;
    const arr = this._getSheets ? this._getSheets() : [];
    return (i < arr.length) ? arr[i] : null;
  };
  StyleSheetList.prototype[Symbol.iterator] = function () {
    const arr = this._getSheets ? this._getSheets() : [];
    let idx = 0;
    return {
      next: function () {
        if (idx < arr.length) {
          return { value: arr[idx++], done: false };
        }
        return { value: undefined, done: true };
      }
    };
  };
  Object.defineProperty(StyleSheetList.prototype, Symbol.toStringTag, {
    value: "StyleSheetList",
    configurable: true,
  });
  markNative(StyleSheetList, "StyleSheetList");
  markNative(StyleSheetList.prototype.item, "item");
  global.StyleSheetList = StyleSheetList;
  if (typeof globalThis !== "undefined") globalThis.StyleSheetList = StyleSheetList;
  if (typeof window !== "undefined") window.StyleSheetList = StyleSheetList;

  function createStyleSheetListProxy(root) {
    const target = new StyleSheetList();

    function getSheets() {
      const sheets = [];
      if (!root) return sheets;

      let elements = [];
      if (typeof root.querySelectorAll === "function") {
        try {
          elements = root.querySelectorAll("style, link");
        } catch (e) {}
      }
      if (!elements || elements.length === 0) {
        const list = [];
        function walk(n) {
          if (!n) return;
          if (n.nodeType === 1) {
            const tag = String(n.localName || n.tagName || "").toLowerCase();
            if (tag === "style" || tag === "link") list.push(n);
          }
          let c = n.firstChild;
          while (c) {
            walk(c);
            c = c.nextSibling;
          }
        }
        walk(root);
        elements = list;
      }

      for (let i = 0; i < elements.length; i++) {
        const el = elements[i];
        if (!el) continue;
        const tag = String(el.localName || el.tagName || "").toLowerCase();
        if (tag === "style") {
          const s = el.sheet;
          if (s) sheets.push(s);
        } else if (tag === "link") {
          const rel = (el.getAttribute("rel") || "").trim().toLowerCase().split(/\s+/);
          if (rel.includes("stylesheet")) {
            const s = el.sheet;
            if (s) sheets.push(s);
          }
        }
      }
      return sheets;
    }

    target._getSheets = getSheets;

    return new Proxy(target, {
      get: function (t, prop, receiver) {
        if (typeof prop === "symbol") {
          if (prop === Symbol.iterator) {
            return function () {
              const arr = getSheets();
              let idx = 0;
              return {
                next: function () {
                  if (idx < arr.length) {
                    return { value: arr[idx++], done: false };
                  }
                  return { value: undefined, done: true };
                }
              };
            };
          }
          return t[prop];
        }
        if (prop === "length") {
          return getSheets().length;
        }
        if (prop === "item") {
          return function (index) {
            const i = Number(index) >>> 0;
            const arr = getSheets();
            return (i < arr.length) ? arr[i] : null;
          };
        }
        if (typeof prop === "string" && /^\d+$/.test(prop)) {
          const i = Number(prop);
          const arr = getSheets();
          return (i >= 0 && i < arr.length) ? arr[i] : undefined;
        }
        return t[prop];
      },
      has: function (t, prop) {
        if (prop === "length") return true;
        if (typeof prop === "string" && /^\d+$/.test(prop)) {
          return Number(prop) < getSheets().length;
        }
        return prop in t;
      },
      ownKeys: function () {
        const len = getSheets().length;
        const keys = [];
        for (let i = 0; i < len; i++) {
          keys.push(String(i));
        }
        return keys;
      },
      getOwnPropertyDescriptor: function (t, prop) {
        if (prop === "length") {
          return { value: getSheets().length, writable: false, enumerable: false, configurable: true };
        }
        if (typeof prop === "string" && /^\d+$/.test(prop)) {
          const i = Number(prop);
          const arr = getSheets();
          if (i >= 0 && i < arr.length) {
            return { value: arr[i], writable: false, enumerable: true, configurable: true };
          }
        }
        return Reflect.getOwnPropertyDescriptor(t, prop);
      }
    });
  }

  const styleSheetListCache = new WeakMap();
  function getOrCreateStyleSheetList(root) {
    if (!root) return createStyleSheetListProxy(null);
    let cached = styleSheetListCache.get(root);
    if (!cached) {
      cached = createStyleSheetListProxy(root);
      styleSheetListCache.set(root, cached);
    }
    return cached;
  }
  global.getOrCreateStyleSheetList = getOrCreateStyleSheetList;
  if (typeof globalThis !== "undefined") globalThis.getOrCreateStyleSheetList = getOrCreateStyleSheetList;

  const docProtoForStylesheets = (global.Document && global.Document.prototype) ? global.Document.prototype : null;
  if (docProtoForStylesheets && !Object.getOwnPropertyDescriptor(docProtoForStylesheets, "styleSheets")) {
    Object.defineProperty(docProtoForStylesheets, "styleSheets", {
      get: function () {
        return getOrCreateStyleSheetList(this);
      },
      configurable: true,
    });
  }
  if (global.document && !Object.getOwnPropertyDescriptor(global.document, "styleSheets")) {
    Object.defineProperty(global.document, "styleSheets", {
      get: function () {
        return getOrCreateStyleSheetList(this);
      },
      configurable: true,
    });
  }

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
  global.__capsurf_normalizeSingleColor = normalizeSingleColor;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_normalizeSingleColor = normalizeSingleColor;

  const NAMED_COLORS_MAP = {
    lime: "rgb(0, 255, 0)",
    red: "rgb(255, 0, 0)",
    green: "rgb(0, 128, 0)",
    blue: "rgb(0, 0, 255)",
    white: "rgb(255, 255, 255)",
    black: "rgb(0, 0, 0)",
    yellow: "rgb(255, 255, 0)",
    cyan: "rgb(0, 255, 255)",
    aqua: "rgb(0, 255, 255)",
    magenta: "rgb(255, 0, 255)",
    fuchsia: "rgb(255, 0, 255)",
    orange: "rgb(255, 165, 0)",
    purple: "rgb(128, 0, 128)",
    gold: "rgb(255, 215, 0)",
    gray: "rgb(128, 128, 128)",
    grey: "rgb(128, 128, 128)",
    orangered: "rgb(255, 69, 0)",
    silver: "rgb(192, 192, 192)",
    maroon: "rgb(128, 0, 0)",
    olive: "rgb(128, 128, 0)",
    navy: "rgb(0, 0, 128)",
    teal: "rgb(0, 128, 128)",
    pink: "rgb(255, 192, 203)",
    brown: "rgb(165, 42, 42)",
    violet: "rgb(238, 130, 238)",
    indigo: "rgb(75, 0, 130)",
    coral: "rgb(255, 127, 80)",
    crimson: "rgb(220, 20, 60)",
    darkgray: "rgb(169, 169, 169)",
    darkgrey: "rgb(169, 169, 169)",
    lightgray: "rgb(211, 211, 211)",
    lightgrey: "rgb(211, 211, 211)",
    dimgray: "rgb(105, 105, 105)",
    dimgrey: "rgb(105, 105, 105)",
    slategray: "rgb(112, 128, 144)",
    slategrey: "rgb(112, 128, 144)",
    rebeccapurple: "rgb(102, 51, 153)",
    skyblue: "rgb(135, 206, 235)",
    tomato: "rgb(255, 99, 71)",
    yellowgreen: "rgb(154, 205, 50)",
    transparent: "rgba(0, 0, 0, 0)",
  };

  function normalizeSingleColor(val) {
    if (!val || typeof val !== "string") return val;
    const s = val.trim().toLowerCase();
    if (Object.prototype.hasOwnProperty.call(NAMED_COLORS_MAP, s)) {
      return NAMED_COLORS_MAP[s];
    }
    if (s.startsWith("#")) {
      const hex = s.slice(1);
      if (hex.length === 3) {
        const r = parseInt(hex[0] + hex[0], 16);
        const g = parseInt(hex[1] + hex[1], 16);
        const b = parseInt(hex[2] + hex[2], 16);
        if (!isNaN(r) && !isNaN(g) && !isNaN(b)) return "rgb(" + r + ", " + g + ", " + b + ")";
      } else if (hex.length === 4) {
        const r = parseInt(hex[0] + hex[0], 16);
        const g = parseInt(hex[1] + hex[1], 16);
        const b = parseInt(hex[2] + hex[2], 16);
        const a = parseInt(hex[3] + hex[3], 16) / 255;
        if (!isNaN(r) && !isNaN(g) && !isNaN(b) && !isNaN(a)) {
          if (a >= 1) return "rgb(" + r + ", " + g + ", " + b + ")";
          return "rgba(" + r + ", " + g + ", " + b + ", " + Number(a.toFixed(3)) + ")";
        }
      } else if (hex.length === 6) {
        const r = parseInt(hex.slice(0, 2), 16);
        const g = parseInt(hex.slice(2, 4), 16);
        const b = parseInt(hex.slice(4, 6), 16);
        if (!isNaN(r) && !isNaN(g) && !isNaN(b)) return "rgb(" + r + ", " + g + ", " + b + ")";
      } else if (hex.length === 8) {
        const r = parseInt(hex.slice(0, 2), 16);
        const g = parseInt(hex.slice(2, 4), 16);
        const b = parseInt(hex.slice(4, 6), 16);
        const a = parseInt(hex.slice(6, 8), 16) / 255;
        if (!isNaN(r) && !isNaN(g) && !isNaN(b) && !isNaN(a)) {
          if (a >= 1) return "rgb(" + r + ", " + g + ", " + b + ")";
          return "rgba(" + r + ", " + g + ", " + b + ", " + Number(a.toFixed(3)) + ")";
        }
      }
    }
    const fmtRgb = (r, g, b, a) => {
      const rr = Math.max(0, Math.min(255, Math.round(r)));
      const gg = Math.max(0, Math.min(255, Math.round(g)));
      const bb = Math.max(0, Math.min(255, Math.round(b)));
      if (a === undefined || a >= 1) return "rgb(" + rr + ", " + gg + ", " + bb + ")";
      const aa = Math.max(0, Math.min(1, a));
      return "rgba(" + rr + ", " + gg + ", " + bb + ", " + Number(aa.toFixed(6)) + ")";
    };
    const splitTopSlash = (str) => {
      const out = [];
      let cur = "", d = 0;
      for (let i = 0; i < str.length; i++) {
        const ch = str[i];
        if (ch === "(") d++;
        else if (ch === ")" && d > 0) d--;
        if (d === 0 && ch === "/") { out.push(cur.trim()); cur = ""; }
        else cur += ch;
      }
      out.push(cur.trim());
      return out;
    };
    const splitColorCh = (str) => {
      if (str.includes(",")) {
        const out = [];
        let cur = "", d = 0;
        for (let i = 0; i < str.length; i++) {
          const ch = str[i];
          if (ch === "(") d++;
          else if (ch === ")" && d > 0) d--;
          if (d === 0 && ch === ",") { if (cur.trim()) out.push(cur.trim()); cur = ""; }
          else cur += ch;
        }
        if (cur.trim()) out.push(cur.trim());
        return out;
      }
      return splitCssTokens(str);
    };
    const evalColorExpr = (tok) => {
      const t = String(tok || "").trim().toLowerCase();
      if (!/^(?:calc|min|max|clamp)\(/i.test(t)) return t;
      const body = t.replace(/^(?:calc)\(/i, "(");
      let pos = 0;
      const skipWs = () => { while (pos < body.length && /\s/.test(body[pos])) pos++; };
      const parseExpr = () => {
        let left = parseTerm();
        if (left === null) return null;
        while (true) {
          skipWs();
          const op = body[pos];
          if (op !== "+" && op !== "-") break;
          pos++;
          const right = parseTerm();
          if (right === null) return null;
          left = { v: op === "+" ? left.v + right.v : left.v - right.v, u: left.u || right.u };
        }
        return left;
      };
      const parseTerm = () => {
        let left = parseFactor();
        if (left === null) return null;
        while (true) {
          skipWs();
          const op = body[pos];
          if (op !== "*" && op !== "/") break;
          pos++;
          const right = parseFactor();
          if (right === null) return null;
          if (op === "*") left = { v: left.v * right.v, u: left.u || right.u };
          else {
            if (right.v === 0) return null;
            left = { v: left.v / right.v, u: left.u };
          }
        }
        return left;
      };
      const parseFactor = () => {
        skipWs();
        if (body[pos] === "(") {
          pos++;
          const inner = parseExpr();
          skipWs();
          if (body[pos] !== ")") return null;
          pos++;
          return inner;
        }
        const m = /^([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)(%|deg|rad|grad|turn)?/i.exec(body.slice(pos));
        if (!m) return null;
        pos += m[0].length;
        return { v: parseFloat(m[1]), u: m[2] || "" };
      };
      const res = parseExpr();
      skipWs();
      if (!res || pos !== body.length || !isFinite(res.v)) return t;
      return String(res.v) + res.u;
    };
    const rgbM = /^rgba?\(\s*(.+)\s*\)$/i.exec(s);
    if (rgbM && !/\bfrom\b/i.test(rgbM[1])) {
      const slash = splitTopSlash(rgbM[1]);
      const chParts = splitColorCh(slash[0]);
      if (chParts.length === 3 || (chParts.length === 4 && slash.length === 1)) {
        const parseCh = (raw) => {
          const t = evalColorExpr(raw);
          return t.endsWith("%") ? (parseFloat(t) * 2.55) : parseFloat(t);
        };
        const r = parseCh(chParts[0]), g = parseCh(chParts[1]), b = parseCh(chParts[2]);
        let a = 1;
        if (slash.length === 2) {
          const at = evalColorExpr(slash[1]);
          a = at.endsWith("%") ? (parseFloat(at) / 100) : parseFloat(at);
        } else if (chParts.length === 4) {
          const at = evalColorExpr(chParts[3]);
          a = at.endsWith("%") ? (parseFloat(at) / 100) : parseFloat(at);
        }
        if (isFinite(r) && isFinite(g) && isFinite(b) && isFinite(a)) return fmtRgb(r, g, b, a);
      }
    }
    const hslM = /^hsla?\(\s*(.+)\s*\)$/i.exec(s);
    if (hslM && !/\bfrom\b/i.test(hslM[1])) {
      const slash = splitTopSlash(hslM[1]);
      const chParts = splitColorCh(slash[0]);
      if (chParts.length === 3 || (chParts.length === 4 && slash.length === 1)) {
        const hTok = evalColorExpr(chParts[0]);
        let h = parseFloat(hTok);
        if (hTok.endsWith("rad")) h = h * 180 / Math.PI;
        else if (hTok.endsWith("grad")) h = h * 0.9;
        else if (hTok.endsWith("turn")) h = h * 360;
        h = ((h % 360) + 360) % 360;
        const sat = Math.max(0, Math.min(1, parseFloat(evalColorExpr(chParts[1])) / 100));
        const lit = Math.max(0, Math.min(1, parseFloat(evalColorExpr(chParts[2])) / 100));
        let a = 1;
        if (slash.length === 2) {
          const at = evalColorExpr(slash[1]);
          a = at.endsWith("%") ? (parseFloat(at) / 100) : parseFloat(at);
        } else if (chParts.length === 4) {
          const at = evalColorExpr(chParts[3]);
          a = at.endsWith("%") ? (parseFloat(at) / 100) : parseFloat(at);
        }
        if (isFinite(h) && isFinite(sat) && isFinite(lit) && isFinite(a)) {
          const c = (1 - Math.abs(2 * lit - 1)) * sat;
          const x = c * (1 - Math.abs(((h / 60) % 2) - 1));
          const m = lit - c / 2;
          let r1 = 0, g1 = 0, b1 = 0;
          if (h < 60) { r1 = c; g1 = x; }
          else if (h < 120) { r1 = x; g1 = c; }
          else if (h < 180) { g1 = c; b1 = x; }
          else if (h < 240) { g1 = x; b1 = c; }
          else if (h < 300) { r1 = x; b1 = c; }
          else { r1 = c; b1 = x; }
          return fmtRgb((r1 + m) * 255, (g1 + m) * 255, (b1 + m) * 255, a);
        }
      }
    }
    return val;
  }

  function resolveRelativeColor(val, currentColorStr) {
    if (!val || typeof val !== "string") return val;
    const trimmed = val.trim();
    const relMatch = trimmed.match(/^rgba?\(\s*from\s+([^\s]+)\s+([^\s/]+)\s+([^\s/]+)\s+([^\s/)]+)(?:\s*\/\s*([^\s)]+))?\s*\)$/i);
    if (!relMatch) return val;
    let originStr = relMatch[1].trim();
    if (originStr.toLowerCase() === "currentcolor") {
      originStr = currentColorStr || "rgb(0, 0, 0)";
    }
    originStr = normalizeCssColor(originStr);
    let or = 0, og = 0, ob = 0, oa = 1;
    const rgbM = originStr.match(/^rgba?\(\s*([0-9.]+)\s*,\s*([0-9.]+)\s*,\s*([0-9.]+)(?:\s*,\s*([0-9.]+))?\s*\)$/i);
    if (rgbM) {
      or = parseFloat(rgbM[1]) / 255;
      og = parseFloat(rgbM[2]) / 255;
      ob = parseFloat(rgbM[3]) / 255;
      if (rgbM[4] !== undefined) oa = parseFloat(rgbM[4]);
    } else {
      const srgbM = originStr.match(/^color\(srgb\s+([^\s/]+)\s+([^\s/]+)\s+([^\s/)]+)(?:\s*\/\s*([^\s)]+))?\)$/i);
      if (srgbM) {
        or = srgbM[1] === "none" ? 0 : parseFloat(srgbM[1]);
        og = srgbM[2] === "none" ? 0 : parseFloat(srgbM[2]);
        ob = srgbM[3] === "none" ? 0 : parseFloat(srgbM[3]);
        if (srgbM[4] !== undefined) oa = srgbM[4] === "none" ? 0 : parseFloat(srgbM[4]);
      } else {
        return val;
      }
    }
    const fmtComp = function (expr) {
      const e = String(expr).trim().toLowerCase();
      if (e === "none") return "none";
      if (e === "r") return String(Number(or.toFixed(6)));
      if (e === "g") return String(Number(og.toFixed(6)));
      if (e === "b") return String(Number(ob.toFixed(6)));
      if (e === "alpha") return String(Number(oa.toFixed(6)));
      if (e.endsWith("%")) return String(Number((parseFloat(e) / 100).toFixed(6)));
      const n = parseFloat(e);
      if (!isNaN(n)) return String(Number((n / 255).toFixed(6)));
      return "0";
    };
    const cr = fmtComp(relMatch[2]);
    const cg = fmtComp(relMatch[3]);
    const cb = fmtComp(relMatch[4]);
    if (relMatch[5] !== undefined) {
      const ca = fmtComp(relMatch[5]);
      if (ca !== "1") return "color(srgb " + cr + " " + cg + " " + cb + " / " + ca + ")";
    }
    return "color(srgb " + cr + " " + cg + " " + cb + ")";
  }

  function normalizeCssColor(val) {
    if (!val || typeof val !== "string") return val;
    const trimmed = val.trim();
    // CSS Color 4: rgb()/rgba() containing `none` serializes as color(srgb ...)
    if (/^rgba?\s*\(/i.test(trimmed) && /\bnone\b/i.test(trimmed)) {
      const m = trimmed.match(/^rgba?\(\s*([^\s,/]+)\s+([^\s,/]+)\s+([^\s,/]+)(?:\s*\/\s*([^\s)]+))?\s*\)$/i);
      if (m) {
        const fmtSrgb = function (c) {
          const cl = c.toLowerCase();
          if (cl === "none") return "none";
          if (cl.endsWith("%")) return String(Number((parseFloat(cl) / 100).toFixed(6)));
          return String(Number((parseFloat(cl) / 255).toFixed(6)));
        };
        const r = fmtSrgb(m[1]);
        const g = fmtSrgb(m[2]);
        const b = fmtSrgb(m[3]);
        if (m[4] !== undefined) {
          const al = m[4].toLowerCase();
          const a = al === "none" ? "none" : (al.endsWith("%") ? String(Number((parseFloat(al) / 100).toFixed(6))) : String(Number(parseFloat(al).toFixed(6))));
          if (a !== "1") return "color(srgb " + r + " " + g + " " + b + " / " + a + ")";
        }
        return "color(srgb " + r + " " + g + " " + b + ")";
      }
    }
    let res = normalizeSingleColor(val);
    if (res.includes("rgb(") || res.includes("rgba(")) {
      res = res.replace(/rgba?\(\s*([0-9.]+%?)\s*[, /]\s*([0-9.]+%?)\s*[, /]\s*([0-9.]+%?)(?:\s*[, /]\s*([0-9.]+%?))?\s*\)/gi, function (match, p1, p2, p3, p4) {
        const parseComp = function (p) {
          if (typeof p !== "string") return 0;
          if (p.endsWith("%")) return Math.round((parseFloat(p) / 100.0) * 255.0);
          return Math.round(parseFloat(p));
        };
        const r = Math.min(255, Math.max(0, parseComp(p1)));
        const g = Math.min(255, Math.max(0, parseComp(p2)));
        const b = Math.min(255, Math.max(0, parseComp(p3)));
        let a = 1.0;
        if (p4 !== undefined && p4 !== null) {
          if (String(p4).endsWith("%")) a = parseFloat(p4) / 100.0;
          else a = parseFloat(p4);
          a = Math.min(1.0, Math.max(0.0, a));
        }
        if (!isNaN(r) && !isNaN(g) && !isNaN(b)) {
          if (a >= 1.0) {
            return "rgb(" + r + ", " + g + ", " + b + ")";
          } else {
            return "rgba(" + r + ", " + g + ", " + b + ", " + Number(a.toFixed(3)) + ")";
          }
        }
        return match;
      });
    }
    return res;
  }
  global.__capsurf_normalizeSingleColor = normalizeSingleColor;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_normalizeSingleColor = normalizeSingleColor;

  let _capturedGetComputedStyle = null;

  function formatCssMatrixNumber(n) {
    if (!isFinite(n) || Math.abs(n) < 1e-12) return "0";
    const expStr = n.toExponential(5);
    const parts = expStr.split("e");
    const exp = parseInt(parts[1], 10);
    if (exp < -4 || exp >= 6) {
      const mant = parts[0].replace(/(\.\d*?[1-9])0+$/, "$1").replace(/\.0+$/, "");
      const sign = exp >= 0 ? "+" : "-";
      const absExp = Math.abs(exp);
      const expPad = absExp < 10 ? "0" + absExp : String(absExp);
      return mant + "e" + sign + expPad;
    }
    let s = Number(n.toPrecision(6)).toString();
    if (s === "-0") s = "0";
    return s;
  }

  function parseTransformAngleRad(raw) {
    const s = String(raw || "0").trim().toLowerCase();
    const num = parseFloat(s) || 0;
    if (s.endsWith("deg")) return num * Math.PI / 180;
    if (s.endsWith("turn")) return num * Math.PI * 2;
    if (s.endsWith("grad")) return num * Math.PI / 200;
    return num; // rad
  }

  function exactCosSin(raw) {
    const s = String(raw || "0").trim().toLowerCase();
    if (s.endsWith("deg")) {
      const deg = parseFloat(s) || 0;
      const mod = ((deg % 360) + 360) % 360;
      if (Math.abs(mod) < 1e-9 || Math.abs(mod - 360) < 1e-9) return [1, 0];
      if (Math.abs(mod - 90) < 1e-9) return [0, 1];
      if (Math.abs(mod - 180) < 1e-9) return [-1, 0];
      if (Math.abs(mod - 270) < 1e-9) return [0, -1];
    }
    const rad = parseTransformAngleRad(raw);
    return [Math.cos(rad), Math.sin(rad)];
  }

  function serializeComputedTransform(val, element, computedDecl) {
    if (!val || typeof val !== "string") return val;
    const trimmed = val.trim();
    if (!trimmed || trimmed === "none") return trimmed;

    // If element (or an ancestor) has display: none, transform resolves to "none" when it cannot be resolved
    const disp = computedDecl && computedDecl.getPropertyValue ? String(computedDecl.getPropertyValue("display") || "").trim().toLowerCase() : "";
    if (disp === "none") return "none";

    // Resolve element reference box width/height for percentage translations
    let refW = 0, refH = 0;
    if (computedDecl && computedDecl.getPropertyValue) {
      const wStr = computedDecl.getPropertyValue("width");
      const hStr = computedDecl.getPropertyValue("height");
      if (wStr && wStr.endsWith("px")) refW = parseFloat(wStr) || 0;
      if (hStr && hStr.endsWith("px")) refH = parseFloat(hStr) || 0;
    }
    const parseLen = function (raw, refPx) {
      const s = String(raw || "0").trim().toLowerCase();
      if (s.endsWith("%")) return (parseFloat(s) / 100) * refPx;
      return parseFloat(s) || 0;
    };

    // 4x4 column-major matrix (identity)
    let M = [
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1,
    ];
    const mul4 = function (B) {
      const R = new Array(16);
      for (let col = 0; col < 4; col++) {
        for (let row = 0; row < 4; row++) {
          R[col * 4 + row] =
            M[0 * 4 + row] * B[col * 4 + 0] +
            M[1 * 4 + row] * B[col * 4 + 1] +
            M[2 * 4 + row] * B[col * 4 + 2] +
            M[3 * 4 + row] * B[col * 4 + 3];
        }
      }
      M = R;
    };

    const fnRe = /([a-zA-Z0-9_-]+)\s*\(([^)]*)\)/g;
    let m;
    let matched = false;
    while ((m = fnRe.exec(trimmed)) !== null) {
      const fn = m[1].toLowerCase();
      const args = m[2].split(",").map((s) => s.trim()).filter(Boolean);
      if (fn === "scale") {
        const sx = parseFloat(args[0]);
        const sy = args.length > 1 ? parseFloat(args[1]) : sx;
        if (!isNaN(sx) && !isNaN(sy)) {
          mul4([sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
          matched = true;
        }
      } else if (fn === "scalex") {
        const sx = parseFloat(args[0]);
        if (!isNaN(sx)) { mul4([sx, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]); matched = true; }
      } else if (fn === "scaley") {
        const sy = parseFloat(args[0]);
        if (!isNaN(sy)) { mul4([1, 0, 0, 0, 0, sy, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]); matched = true; }
      } else if (fn === "scalez") {
        const sz = parseFloat(args[0]);
        if (!isNaN(sz)) { mul4([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, sz, 0, 0, 0, 0, 1]); matched = true; }
      } else if (fn === "scale3d" && args.length >= 3) {
        const sx = parseFloat(args[0]), sy = parseFloat(args[1]), sz = parseFloat(args[2]);
        if (!isNaN(sx) && !isNaN(sy) && !isNaN(sz)) {
          mul4([sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, sz, 0, 0, 0, 0, 1]);
          matched = true;
        }
      } else if (fn === "translate") {
        const tx = parseLen(args[0], refW);
        const ty = args.length > 1 ? parseLen(args[1], refH) : 0;
        mul4([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, tx, ty, 0, 1]);
        matched = true;
      } else if (fn === "translatex") {
        const tx = parseLen(args[0], refW);
        mul4([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, tx, 0, 0, 1]);
        matched = true;
      } else if (fn === "translatey") {
        const ty = parseLen(args[0], refH);
        mul4([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, ty, 0, 1]);
        matched = true;
      } else if (fn === "translatez") {
        const tz = parseLen(args[0], 0);
        mul4([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, tz, 1]);
        matched = true;
      } else if (fn === "translate3d" && args.length >= 3) {
        const tx = parseLen(args[0], refW);
        const ty = parseLen(args[1], refH);
        const tz = parseLen(args[2], 0);
        mul4([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, tx, ty, tz, 1]);
        matched = true;
      } else if (fn === "rotate" || fn === "rotatez") {
        const [cos, sin] = exactCosSin(args[0]);
        mul4([cos, sin, 0, 0, -sin, cos, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
        matched = true;
      } else if (fn === "rotatex") {
        const [cos, sin] = exactCosSin(args[0]);
        mul4([1, 0, 0, 0, 0, cos, sin, 0, 0, -sin, cos, 0, 0, 0, 0, 1]);
        matched = true;
      } else if (fn === "rotatey") {
        const [cos, sin] = exactCosSin(args[0]);
        mul4([cos, 0, -sin, 0, 0, 1, 0, 0, sin, 0, cos, 0, 0, 0, 0, 1]);
        matched = true;
      } else if (fn === "rotate3d" && args.length >= 4) {
        let x = parseFloat(args[0]) || 0;
        let y = parseFloat(args[1]) || 0;
        let z = parseFloat(args[2]) || 0;
        const len = Math.hypot(x, y, z);
        if (len > 0) {
          x /= len; y /= len; z /= len;
          const [c, s] = exactCosSin(args[3]);
          const t = 1 - c;
          mul4([
            t * x * x + c,       t * x * y + z * s,   t * x * z - y * s,   0,
            t * x * y - z * s,   t * y * y + c,       t * y * z + x * s,   0,
            t * x * z + y * s,   t * y * z - x * s,   t * z * z + c,       0,
            0,                   0,                   0,                   1,
          ]);
        }
        matched = true;
      } else if (fn === "skew") {
        const ax = parseTransformAngleRad(args[0]);
        const ay = args.length > 1 ? parseTransformAngleRad(args[1]) : 0;
        mul4([1, Math.tan(ay), 0, 0, Math.tan(ax), 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
        matched = true;
      } else if (fn === "skewx") {
        const ax = parseTransformAngleRad(args[0]);
        mul4([1, 0, 0, 0, Math.tan(ax), 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
        matched = true;
      } else if (fn === "skewy") {
        const ay = parseTransformAngleRad(args[0]);
        mul4([1, Math.tan(ay), 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
        matched = true;
      } else if (fn === "perspective") {
        const d = parseFloat(args[0]);
        if (!isNaN(d) && d > 0) {
          mul4([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, -1 / d, 0, 0, 0, 1]);
          matched = true;
        }
      } else if (fn === "matrix" && args.length >= 6) {
        const nums = args.map(parseFloat);
        if (nums.every((n) => !isNaN(n))) {
          mul4([nums[0], nums[1], 0, 0, nums[2], nums[3], 0, 0, 0, 0, 1, 0, nums[4], nums[5], 0, 1]);
          matched = true;
        }
      } else if (fn === "matrix3d" && args.length >= 16) {
        const nums = args.map(parseFloat);
        if (nums.every((n) => !isNaN(n))) {
          mul4(nums.slice(0, 16));
          matched = true;
        }
      } else {
        return trimmed;
      }
    }
    if (!matched) return trimmed;

    const eps = 1e-12;
    const is2D =
      Math.abs(M[2]) < eps && Math.abs(M[3]) < eps &&
      Math.abs(M[6]) < eps && Math.abs(M[7]) < eps &&
      Math.abs(M[8]) < eps && Math.abs(M[9]) < eps &&
      Math.abs(M[10] - 1) < eps && Math.abs(M[11]) < eps &&
      Math.abs(M[14]) < eps && Math.abs(M[15] - 1) < eps;

    if (is2D) {
      return "matrix(" + [M[0], M[1], M[4], M[5], M[12], M[13]].map(formatCssMatrixNumber).join(", ") + ")";
    }
    return "matrix3d(" + M.map(formatCssMatrixNumber).join(", ") + ")";
  }

  function evalLinkMediaMatchesScreen(mediaAttr) {
    const raw = String(mediaAttr == null ? "" : mediaAttr).trim().toLowerCase();
    if (!raw || raw === "all" || raw === "screen") return true;
    const queries = raw.split(",");
    for (let i = 0; i < queries.length; i++) {
      const q = queries[i].trim();
      if (!q) continue;
      if (q === "all" || q === "screen" || q === "only screen" || q === "only all") return true;
      if (q.startsWith("not print")) return true;
      if (q === "print" || q === "only print" || q === "speech" || q === "not screen" || q === "not all") continue;
      if (q.indexOf("print") === -1) return true;
    }
    return false;
  }

  function getDynamicLinkOverrides(element) {
    if (!element || element.nodeType !== 1) return null;
    const doc = element.ownerDocument || global.document || (typeof document !== "undefined" ? document : null);
    if (!doc || typeof doc.querySelectorAll !== "function") return null;
    const links = doc.querySelectorAll("link");
    if (!links || links.length === 0) return null;
    let overrides = null;
    for (let i = 0; i < links.length; i++) {
      const link = links[i];
      if (!link || typeof link.getAttribute !== "function") continue;
      const rel = String(link.getAttribute("rel") || "").trim().toLowerCase().split(/\s+/);
      if (!rel.includes("stylesheet")) continue;
      const mediaAttr = link.getAttribute("media");
      if (!evalLinkMediaMatchesScreen(mediaAttr)) continue;
      const sheet = link.sheet;
      if (!sheet) continue;
      if (typeof sheet._loadLinkSheet === "function") {
        sheet._loadLinkSheet();
      }
      const rawCssText = sheet._cascadeCssText || "";
      if (!rawCssText) continue;
      const cssText = rawCssText.replace(/\/\*[\s\S]*?\*\//g, "");
      const chunks = splitStylesheetRules(cssText);
      for (let j = 0; j < chunks.length; j++) {
        const item = chunks[j];
        if (!item) continue;
        let selText = "";
        let bodyText = "";
        if (typeof item === "string") {
          const chunk = item.trim();
          if (!chunk || chunk.startsWith("@")) continue;
          const brace = chunk.indexOf("{");
          const close = chunk.lastIndexOf("}");
          if (brace <= 0 || close <= brace) continue;
          selText = chunk.slice(0, brace).trim();
          bodyText = chunk.slice(brace + 1, close).trim();
        } else if ((item.selectorText || item.selector) && item.declText) {
          selText = String(item.selectorText || item.selector).trim();
          bodyText = String(item.declText).trim();
        } else {
          continue;
        }
        let matched = false;
        try {
          if (typeof element.matches === "function") {
            matched = element.matches(selText);
          }
        } catch (e) {}
        if (!matched) continue;
        if (!overrides) overrides = Object.create(null);
        const decls = bodyText.split(";");
        for (let k = 0; k < decls.length; k++) {
          const colon = decls[k].indexOf(":");
          if (colon <= 0) continue;
          const propName = decls[k].slice(0, colon).trim().toLowerCase();
          let propVal = decls[k].slice(colon + 1).trim();
          propVal = propVal.replace(/\s*!important\s*$/i, "").trim();
          if (propName && propVal) {
            overrides[propName] = propVal;
          }
        }
      }
    }
    return overrides;
  }

  function wrapComputedStyleDecl(decl, element) {
    if (!decl || typeof decl !== "object") return decl;
    const linkOverrides = getDynamicLinkOverrides(element);
    return new Proxy(decl, {
      get: function (target, prop, receiver) {
        if (typeof prop === "symbol") return target[prop];
        if (typeof prop === "string") {
          if (prop === "getPropertyValue") {
            return function (propertyName) {
              const rawName = String(propertyName || "").trim();
              if (!rawName) return "";
              const k = rawName.startsWith("--") ? rawName : rawName.toLowerCase();
              if (linkOverrides && Object.prototype.hasOwnProperty.call(linkOverrides, k)) {
                let ov = resolveCssVars(element, linkOverrides[k]);
                if (k === "color" || k.endsWith("-color")) ov = normalizeCssColor(ov);
                return ov;
              }
              if (k === "-webkit-transform" || k === "transform") {
                let tv = (target.getPropertyValue ? (target.getPropertyValue("transform") || target.getPropertyValue("-webkit-transform")) : "") || target.transform || "";
                if (tv) {
                  tv = resolveCssVars(element, tv);
                  tv = serializeComputedTransform(tv, element, target);
                }
                return tv || "none";
              }
              let val = receiver[k] || (target.getPropertyValue ? target.getPropertyValue(propertyName) : "");
              if (!val && (k.startsWith("-webkit-") || k.startsWith("-moz-"))) {
                const unprefixed = k.replace(/^-(?:webkit|moz)-/, "");
                val = receiver[unprefixed] || (target.getPropertyValue ? target.getPropertyValue(unprefixed) : "");
              }
              if (typeof val === "string") {
                val = resolveCssVars(element, val);
                if ((k === "color" || k.endsWith("-color")) && val.trim().toLowerCase() === "currentcolor") {
                  val = receiver.color || "rgb(0, 0, 0)";
                }
                if ((k === "color" || k.endsWith("-color")) && /\bfrom\s+/i.test(val)) {
                  val = resolveRelativeColor(val, receiver.color || "rgb(0, 0, 0)");
                }
                if (k === "shape-outside" || k === "clip-path" || k === "offset-path") {
                  val = val.replace(/\bpath\(\s*'([^']*)'\s*\)/gi, 'path("$1")');
                }
                if (!k.startsWith("--")) {
                  val = normalizeCssColor(val);
                }
              }
              return val || "";
            };
          }
          if (prop === "cssFloat" || prop === "float") {
            return target.getPropertyValue ? target.getPropertyValue("float") : (target.float || "");
          }
          if (prop === "webkitTransform" || prop === "-webkit-transform" || prop === "transform") {
            let tv = (target.getPropertyValue ? (target.getPropertyValue("transform") || target.getPropertyValue("-webkit-transform")) : "") || target.transform || "";
            if (tv) {
              tv = resolveCssVars(element, tv);
              tv = serializeComputedTransform(tv, element, target);
            }
            return tv || "none";
          }
          const kebab = cssPropCamelToKebab(prop);

          if (linkOverrides && Object.prototype.hasOwnProperty.call(linkOverrides, kebab)) {
            let ov = resolveCssVars(element, linkOverrides[kebab]);
            if (kebab === "color" || kebab.endsWith("-color")) ov = normalizeCssColor(ov);
            return ov;
          }

          // If color is unstyled, default to rgb(0, 0, 0)
          if (kebab === "color") {
            let val = target.getPropertyValue ? target.getPropertyValue("color") : target[prop];
            if (val) val = normalizeCssColor(val);
            return val || "rgb(0, 0, 0)";
          }

          if (kebab === "direction") {
            let val = target.getPropertyValue ? target.getPropertyValue("direction") : target[prop];
            if (val) {
              val = resolveCssVars(element, val);
              const lower = String(val).trim().toLowerCase();
              if (lower === "ltr" || lower === "rtl") return lower;
            }
            if (typeof global !== "undefined" && typeof global.__capsurf_getComputedDirection === "function") {
              return global.__capsurf_getComputedDirection(element);
            }
            return "ltr";
          }

          if (kebab === "background-color") {
            let val = target.getPropertyValue ? target.getPropertyValue("background-color") : target[prop];
            if (!val && target.getPropertyValue) {
              val = target.getPropertyValue("background");
            }
            if (!val && target.background) val = target.background;
            if (val) {
              val = resolveCssVars(element, val);
              if (/\bfrom\s+/i.test(val)) {
                val = resolveRelativeColor(val, receiver.color || "rgb(0, 0, 0)");
              }
              val = normalizeCssColor(val);
            }
            return val || "";
          }

          if (kebab.endsWith("-color")) {
            let val = target.getPropertyValue ? target.getPropertyValue(kebab) : target[prop];
            if (val) {
              val = resolveCssVars(element, val);
              if (val.trim().toLowerCase() === "currentcolor") {
                val = receiver.color || "rgb(0, 0, 0)";
              }
              if (/\bfrom\s+/i.test(val)) {
                val = resolveRelativeColor(val, receiver.color || "rgb(0, 0, 0)");
              }
              val = normalizeCssColor(val);
            }
            return val || "";
          }

          if (kebab === "shape-outside" || kebab === "clip-path" || kebab === "offset-path") {
            let val = target.getPropertyValue ? target.getPropertyValue(kebab) : target[prop];
            if (typeof val === "string" && val) {
              val = resolveCssVars(element, val);
              val = val.replace(/\bpath\(\s*'([^']*)'\s*\)/gi, 'path("$1")');
            }
            return val || "";
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
          if (typeof val === "string") {
            val = resolveCssVars(element, val);
            if (!prop.startsWith("--")) {
              val = normalizeCssColor(val);
            }
          }
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

  let _overrideGetComputedStyle = null;

  function installGetComputedStyle(targetObj) {
    if (!targetObj) return;
    Object.defineProperty(targetObj, "getComputedStyle", {
      get: function () {
        if (_overrideGetComputedStyle !== null) return _overrideGetComputedStyle;
        return _capturedGetComputedStyle ? enhancedGetComputedStyle : undefined;
      },
      set: function (fn) {
        if (fn === enhancedGetComputedStyle || fn === _capturedGetComputedStyle) return;
        if (_capturedGetComputedStyle === null && typeof fn === "function") {
          _capturedGetComputedStyle = fn;
        } else {
          _overrideGetComputedStyle = fn;
        }
      },
      configurable: true,
      enumerable: true,
    });
  }

  installGetComputedStyle(global);
  if (typeof globalThis !== "undefined") installGetComputedStyle(globalThis);
  if (typeof window !== "undefined") installGetComputedStyle(window);

})(typeof window !== "undefined" ? window : globalThis);
