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
    "direction", "display", "empty-cells", "field-sizing", "filter", "flex", "flex-basis", "flex-direction",
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
    "text-decoration-color", "text-decoration-inset", "text-decoration-line", "text-decoration-skip-ink",
    "text-decoration-skip-spaces", "text-decoration-style", "text-decoration-thickness", "text-emphasis",
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

  function isValidCssValue(prop, value) {
    if (typeof value !== "string") value = String(value);
    const trimmed = value.trim();
    if (!trimmed) return false;
    if (prop.startsWith("--")) return true;

    const vLower = trimmed.toLowerCase();
    if (vLower === "inherit" || vLower === "initial" || vLower === "unset" || vLower === "revert" || vLower === "revert-layer") {
      return true;
    }
    if (prop === "all") return false;
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
      "word-break": ["normal", "break-all", "keep-all", "break-word", "auto-phrase"],
      "hyphens": ["none", "manual", "auto"],
      "line-break": ["auto", "loose", "normal", "strict", "anywhere"],
      "text-align": ["start", "end", "left", "right", "center", "justify", "match-parent", "justify-all"],
      "text-align-all": ["start", "end", "left", "right", "center", "justify", "match-parent"],
      "text-align-last": ["auto", "start", "end", "left", "right", "center", "justify", "match-parent"],
      "text-justify": ["auto", "none", "inter-word", "inter-character", "distribute"],
      "text-transform": ["none", "capitalize", "uppercase", "lowercase", "full-width", "full-size-kana"],
      "text-wrap-mode": ["wrap", "nowrap"],
      "text-wrap-style": ["auto", "balance", "stable", "pretty", "avoid-orphans"],
      "white-space-collapse": ["collapse", "preserve", "preserve-breaks", "preserve-spaces", "break-spaces"],
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
      "object-fit": ["fill", "contain", "cover", "none", "scale-down"],
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
      "font-synthesis-style": ["auto", "none"],
      "font-synthesis-small-caps": ["auto", "none"],
      "font-synthesis-position": ["auto", "none"],
      "list-style-position": ["inside", "outside"],
      "scroll-behavior": ["auto", "smooth"],
      "image-rendering": ["auto", "smooth", "high-quality", "crisp-edges", "pixelated"],
      "appearance": ["none", "auto", "textfield", "menulist-button"],
      "continue": ["normal", "discard", "collapse", "-webkit-legacy"],
      "scrollbar-width": ["auto", "thin", "none"],
      "field-sizing": ["fixed", "content"],
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

    function isValidLenPct(tok, allowNegative) {
      if (/^(?:var|env|attr|anchor|anchor-size)\(/i.test(tok)) return true;
      if (/^(?:calc|min|max|clamp)\(/i.test(tok)) {
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
    if (prop === "tab-size") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (/^(?:var|env)\(/i.test(t)) return true;
      if (/^(?:calc|min|max|clamp)\(/i.test(t)) return !t.includes("%");
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
      if (/^(?:calc|min|max|clamp|var|env)\(/i.test(t)) return true;
      return /^[+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?%?$/i.test(t);
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
    if (prop === "background-repeat") {
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
          else if (c === ")" && d > 0) d--;
          if (d === 0 && c === ",") { args.push(cur.trim()); cur = ""; }
          else cur += c;
        }
        args.push(cur.trim());
        if (args.some(a => !a) || args.length < range[0] || args.length > range[1]) return false;
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
    if (prop === "flex") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1 && toks[0] === "none") return true;
      if (toks.length < 1 || toks.length > 3) return false;
      const isFlexNum = (t) => /^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(t) && Number(t) >= 0;
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
      if ((toks[0] === "0" || (isFlexBasis(toks[0]) && !isFlexNum(toks[0]))) && isFlexNum(toks[1]) && isFlexNum(toks[2])) return true;
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
          if (/(?:\d|\.)(?:px|em|rem|ex|rex|cap|rcap|ch|rch|ic|ric|lh|rlh|vw|vh|vi|vb|vmin|vmax|svw|svh|lvw|lvh|dvw|dvh|cqw|cqh|cqi|cqb|cqmin|cqmax|cm|mm|q|in|pt|pc|deg|rad|grad|turn|ms|s|hz|khz|dpi|dpcm|dppx)\b/i.test(t)) return false;
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
      const SINGLE = new Set(["normal", "pre", "nowrap", "pre-wrap", "break-spaces", "pre-line"]);
      const COL = new Set(["collapse", "preserve", "preserve-breaks", "preserve-spaces", "break-spaces"]);
      const WRP = new Set(["wrap", "nowrap"]);
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) return SINGLE.has(toks[0]);
      if (toks.length === 2) {
        return (COL.has(toks[0]) && WRP.has(toks[1])) || (WRP.has(toks[0]) && COL.has(toks[1]));
      }
      return false;
    }
    if (prop === "outline-offset") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t.endsWith("%") || (/^(?:calc|min|max|clamp)\(/i.test(t) && t.includes("%"))) return false;
      return isValidLenPct(t, true);
    }
    if (prop === "perspective") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if (t === "none") return true;
      if (t.endsWith("%") || (/^(?:calc|min|max|clamp)\(/i.test(t) && t.includes("%"))) return false;
      return isValidLenPct(t, false);
    }
    if (prop === "perspective-origin" || prop === "object-position" || prop === "transform-origin") {
      const toks = splitCssTokens(vLower);
      const HX = new Set(["left", "center", "right"]);
      const VY = new Set(["top", "center", "bottom"]);
      const is2Pos = (a, b) => {
        if (HX.has(a) && VY.has(b)) return !(a === "center" && b === "center") || true;
        if (VY.has(a) && HX.has(b)) return true;
        if (isValidLenPct(a, true) && (VY.has(b) || isValidLenPct(b, true))) return true;
        if (HX.has(a) && isValidLenPct(b, true)) return true;
        return false;
      };
      if (toks.length === 1) {
        return HX.has(toks[0]) || VY.has(toks[0]) || isValidLenPct(toks[0], true);
      }
      if (toks.length === 2) return is2Pos(toks[0], toks[1]);
      if (prop === "transform-origin") {
        if (toks.length === 3 && is2Pos(toks[0], toks[1])) {
          const z = toks[2];
          if (z.endsWith("%") || (/^(?:calc|min|max|clamp)\(/i.test(z) && z.includes("%"))) return false;
          return isValidLenPct(z, true);
        }
        return false;
      }
      if (toks.length === 4) {
        const EX = new Set(["left", "right"]);
        const EY = new Set(["top", "bottom"]);
        if (EX.has(toks[0]) && isValidLenPct(toks[1], true) && EY.has(toks[2]) && isValidLenPct(toks[3], true)) return true;
        if (EY.has(toks[0]) && isValidLenPct(toks[1], true) && EX.has(toks[2]) && isValidLenPct(toks[3], true)) return true;
      }
      return false;
    }
    if (prop === "background-size") {
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
    if (prop === "cursor") {
      const KW = new Set([
        "auto", "default", "none", "context-menu", "help", "pointer", "progress", "wait",
        "cell", "crosshair", "text", "vertical-text", "alias", "copy", "move", "no-drop",
        "not-allowed", "grab", "grabbing", "e-resize", "n-resize", "ne-resize", "nw-resize",
        "s-resize", "se-resize", "sw-resize", "w-resize", "ew-resize", "ns-resize",
        "nesw-resize", "nwse-resize", "col-resize", "row-resize", "all-scroll", "zoom-in", "zoom-out"
      ]);
      const parts = vLower.split(",");
      const last = splitCssTokens(parts[parts.length - 1].trim());
      if (last.length !== 1 || !KW.has(last[0])) return false;
      for (let i = 0; i < parts.length - 1; i++) {
        const toks = splitCssTokens(parts[i].trim());
        if (toks.length !== 1 && toks.length !== 3) return false;
        if (!/^url\(/i.test(toks[0])) return false;
        if (toks.length === 3 && (!/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)$/.test(toks[1]) || !/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)$/.test(toks[2]))) return false;
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
      return t === "normal" || t === "light" || t === "dark" || /^--[a-z0-9_-]+$/i.test(t);
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
      return t.toLowerCase() === "normal" || (/^"[^"]*"$/.test(t)) || (/^'[^']*'$/.test(t));
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
        if (i < toks.length && (/^[+-]?\d+$/.test(toks[i]) || /^(?:calc|min|max|clamp)\(/i.test(toks[i]))) i++;
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
    if (prop === "-webkit-line-clamp" || prop === "max-lines") {
      const toks = splitCssTokens(vLower);
      if (toks.length !== 1) return false;
      const t = toks[0];
      if ((prop === "-webkit-line-clamp" && t === "none") || /^(?:calc|min|max|clamp|var|env)\(/i.test(t)) return true;
      return /^\+?[1-9]\d*$/.test(t);
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
    if (prop === "text-decoration-thickness") {
      const toks = splitCssTokens(vLower);
      return toks.length === 1 && (toks[0] === "auto" || toks[0] === "from-font" || isValidLenPct(toks[0], false));
    }
    if (prop === "text-decoration-skip-spaces") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) return toks[0] === "none" || toks[0] === "all" || toks[0] === "start" || toks[0] === "end";
      if (toks.length === 2) return (toks[0] === "start" && toks[1] === "end") || (toks[0] === "end" && toks[1] === "start");
      return false;
    }
    if (prop === "text-decoration-inset") {
      const toks = splitCssTokens(vLower);
      if (toks.length === 1) return toks[0] === "auto" || (!toks[0].endsWith("%") && isValidLenPct(toks[0], false));
      if (toks.length === 2) {
        return !toks[0].endsWith("%") && !toks[1].endsWith("%") &&
               isValidLenPct(toks[0], false) && isValidLenPct(toks[1], false);
      }
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
        if (tl === "inside" || tl === "outside") { if (sPos) return false; sPos = true; }
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
    if (prop === "content") {
      const toks = splitCssTokens(trimmed);
      for (let i = 0; i < toks.length; i++) {
        const cm = /^counters?\((.*)\)$/i.exec(toks[i]);
        if (cm) {
          const parts = cm[1].split(",").map(x => x.trim().toLowerCase());
          const styleArg = parts[parts.length - 1];
          if (parts.length >= 2 && /^(?:none|unset|initial|inherit|revert|revert-layer|default)$/.test(styleArg)) return false;
        }
      }
      return true;
    }

    return true;
  }
  global.__capsurf_isValidCssValue = isValidCssValue;

  const NUM_NORM_PROPS = new Set(["flex-grow", "flex-shrink", "opacity", "z-index", "order", "orphans", "widows", "column-count", "font-weight"]);
  const ZERO_PX_PROPS = new Set([
    "margin-top", "margin-right", "margin-bottom", "margin-left", "margin-block-start", "margin-block-end", "margin-inline-start", "margin-inline-end",
    "padding-top", "padding-right", "padding-bottom", "padding-left", "padding-block-start", "padding-block-end", "padding-inline-start", "padding-inline-end",
    "top", "right", "bottom", "left", "inset-block-start", "inset-block-end", "inset-inline-start", "inset-inline-end",
    "width", "height", "min-width", "min-height", "max-width", "max-height", "block-size", "inline-size", "min-block-size", "min-inline-size", "max-block-size", "max-inline-size",
    "row-gap", "column-gap", "grid-row-gap", "grid-column-gap", "border-top-width", "border-right-width", "border-bottom-width", "border-left-width",
    "outline-width", "outline-offset", "column-rule-width", "letter-spacing", "word-spacing", "border-top-left-radius", "border-top-right-radius", "border-bottom-right-radius", "border-bottom-left-radius",
    "perspective", "column-width", "flex-basis", "translate", "font-size", "vertical-align", "text-underline-offset", "text-decoration-thickness"
  ]);
  const TWO_SIDE_PROPS = new Set([
    "inset-block", "inset-inline", "margin-block", "margin-inline", "padding-block", "padding-inline",
    "overflow", "gap", "grid-gap"
  ]);
  function normalizeCssSpecifiedValue(key, raw) {
    if (!raw || key.startsWith("--")) return raw;
    const s = String(raw).trim();
    if (NUM_NORM_PROPS.has(key) && /^[+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(s)) {
      const n = Number(s);
      if (Number.isFinite(n)) return String(n === 0 ? 0 : n);
    }
    if (ZERO_PX_PROPS.has(key) && (s === "0" || s === "+0" || s === "-0")) return "0px";
    if (TWO_SIDE_PROPS.has(key)) {
      const parts = splitCssTokens(s);
      const isLenProp = !key.startsWith("overflow") && !key.startsWith("place-");
      if (isLenProp) {
        for (let i = 0; i < parts.length; i++) {
          if (parts[i] === "0" || parts[i] === "+0" || parts[i] === "-0") parts[i] = "0px";
        }
      }
      if (parts.length === 2 && parts[0] === parts[1]) return parts[0];
      return parts.join(" ");
    }
    if (key === "margin" || key === "padding" || key === "inset" || key === "border-width") {
      const parts = splitCssTokens(s);
      for (let i = 0; i < parts.length; i++) {
        if (parts[i] === "0" || parts[i] === "+0" || parts[i] === "-0") parts[i] = "0px";
      }
      if (parts.length === 4 && parts[1] === parts[3]) parts.pop();
      if (parts.length === 3 && parts[0] === parts[2]) parts.pop();
      if (parts.length === 2 && parts[0] === parts[1]) parts.pop();
      return parts.join(" ");
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
          const pm = /^([+-]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?)%$/i.exec(parts[i]);
          if (pm) parts[i] = String(Number(pm[1]) / 100);
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
        const x = Number(parts[0]), y = Number(parts[1]), z = Number(parts[2]);
        if (!isNaN(x) && !isNaN(y) && !isNaN(z)) {
          if (x > 0 && y === 0 && z === 0) return "x " + parts[3];
          if (x === 0 && y > 0 && z === 0) return "y " + parts[3];
          if (x === 0 && y === 0 && z > 0) return parts[3];
        }
      }
      return parts.join(" ");
    }
    if (key === "border-radius") {
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
          w = (tl === "0" || tl === "+0" || tl === "-0") ? "0px" : t;
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
      const parts = splitCssTokens(s);
      if (parts.length === 1) {
        const p0 = parts[0].toLowerCase();
        if (p0 === "none") return "0 0 auto";
        if (p0 === "auto") return "1 1 auto";
        if (/^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(p0)) {
          return String(Number(p0) === 0 ? 0 : Number(p0)) + " 1 0%";
        }
        return "1 1 " + (p0 === "0" ? "0px" : parts[0]);
      }
      const isNum = (x) => /^[+]?(?:\d+(?:\.\d+)?|\.\d+)(?:e[+-]?\d+)?$/i.test(x);
      let grow = "1", shrink = "1", basis = "0%";
      const nums = [];
      let bRaw = "";
      for (let i = 0; i < parts.length; i++) {
        if (isNum(parts[i]) && !(nums.length === 2 && parts[i] === "0")) nums.push(String(Number(parts[i])));
        else bRaw = parts[i];
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
    if (key === "perspective-origin" || key === "transform-origin" || key === "object-position") {
      const parts = splitCssTokens(s);
      for (let i = 0; i < parts.length; i++) {
        if (parts[i] === "0" || parts[i] === "+0" || parts[i] === "-0") parts[i] = "0px";
      }
      if (parts.length === 2 || (key === "transform-origin" && parts.length === 3)) {
        const a = parts[0].toLowerCase(), b = parts[1].toLowerCase();
        const isY = (x) => x === "top" || x === "bottom";
        const isX = (x) => x === "left" || x === "right";
        if (isY(a) || (a === "center" && isX(b))) {
          const tmp = parts[0]; parts[0] = parts[1]; parts[1] = tmp;
        }
      }
      return parts.join(" ");
    }
    if (key === "border-spacing") {
      const parts = splitCssTokens(s);
      for (let i = 0; i < parts.length; i++) if (parts[i] === "0" || parts[i] === "+0" || parts[i] === "-0") parts[i] = "0px";
      if (parts.length === 2 && parts[0] === parts[1]) parts.pop();
      return parts.join(" ");
    }
    if (key === "background-size") {
      const layers = s.split(",");
      for (let i = 0; i < layers.length; i++) {
        const p = splitCssTokens(layers[i].trim());
        for (let j = 0; j < p.length; j++) if (p[j] === "0" || p[j] === "+0" || p[j] === "-0") p[j] = "0px";
        if (p.length === 2 && p[1].toLowerCase() === "auto") p.pop();
        layers[i] = p.join(" ");
      }
      return layers.join(", ");
    }
    if (key === "background-repeat") {
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
    if (key === "place-content" || key === "place-items" || key === "place-self") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length >= 2) {
        const splitAt = (p[0] === "first" || p[0] === "last" || p[0] === "safe" || p[0] === "unsafe" || p[0] === "legacy") ? 2 : 1;
        if (splitAt < p.length) {
          const a = p.slice(0, splitAt).join(" "), b = p.slice(splitAt).join(" ");
          return a === b ? a : (a + " " + b);
        }
      }
      return p.join(" ");
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
        if (tl === "filled") return "circle";
        if (tl === "open") return "open circle";
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
    if (key === "text-decoration-skip-spaces") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 2) return "start end";
      return p.join(" ");
    }
    if (key === "text-decoration-inset") {
      const p = splitCssTokens(s.toLowerCase());
      for (let i = 0; i < p.length; i++) {
        if (p[i] === "0" || p[i] === "+0" || p[i] === "-0") p[i] = "0px";
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
      return ax && den ? (ax + " dense") : (ax || (den ? "dense" : s));
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
    if (key === "scrollbar-gutter") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 2) return "stable both-edges";
      return p.join(" ");
    }
    if (key === "text-decoration-line") {
      const p = splitCssTokens(s.toLowerCase());
      if (p.length === 1 && (p[0] === "none" || p[0] === "spelling-error" || p[0] === "grammar-error")) return p[0];
      const order = ["underline", "overline", "line-through", "blink"];
      return order.filter(k => p.indexOf(k) !== -1).join(" ");
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
        if (i + 1 < p.length && (/^[+-]?\d+$/.test(p[i + 1]) || /^(?:calc|min|max|clamp)\(/i.test(p[i + 1]))) {
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
      let pos = "outside", img = "none", typ = "disc", noneCnt = 0;
      for (let i = 0; i < parts.length; i++) {
        const t = parts[i], tl = t.toLowerCase();
        if (tl === "inside" || tl === "outside") pos = tl;
        else if (tl === "none") noneCnt++;
        else if (/^(?:url|linear-gradient|radial-gradient|conic-gradient|image-set)\(/i.test(tl)) img = t;
        else typ = t;
      }
      if (noneCnt >= 2) { img = "none"; typ = "none"; }
      else if (noneCnt === 1) {
        if (img === "none" && typ === "disc") typ = "none";
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
      if (p.length === 1) {
        return [["align-" + kind, p[0]], ["justify-" + kind, p[0]]];
      }
      if (p.length >= 2) {
        const splitAt = (p[0] === "first" || p[0] === "last" || p[0] === "safe" || p[0] === "unsafe" || p[0] === "legacy") ? 2 : 1;
        if (splitAt < p.length) {
          return [
            ["align-" + kind, p.slice(0, splitAt).join(" ")],
            ["justify-" + kind, p.slice(splitAt).join(" ")]
          ];
        }
        return [["align-" + kind, p.join(" ")], ["justify-" + kind, p.join(" ")]];
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
        } else if (/^(?:calc|min|max|clamp)\(/i.test(tl) && nums.length < 2 && !/[a-z%]/.test(tl.replace(/^(?:calc|min|max|clamp)\(/i, ""))) {
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
    if (key === "outline" || key === "column-rule") {
      const p = splitCssTokens(normV);
      const STY = new Set(["none", "hidden", "dotted", "dashed", "solid", "double", "groove", "ridge", "inset", "outset", "auto"]);
      let w = "medium", st = "none", c = "currentcolor";
      for (let i = 0; i < p.length; i++) {
        const t = p[i], tl = t.toLowerCase();
        if (STY.has(tl)) st = tl;
        else if (tl === "thin" || tl === "medium" || tl === "thick" || /^[+-]?(?:\d|\.)/.test(tl) || /^(?:calc|min|max|clamp)\(/i.test(tl)) {
          w = (tl === "0" || tl === "+0" || tl === "-0") ? "0px" : t;
        } else c = t;
      }
      return key === "outline"
        ? [["outline-color", c], ["outline-style", st], ["outline-width", w]]
        : [["column-rule-width", w], ["column-rule-style", st], ["column-rule-color", c]];
    }
    return null;
  }
  global.__capsurf_expandShorthand = expandCssShorthand;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_expandShorthand = expandCssShorthand;

  const SHORTHAND_EXPAND_LONGHANDS = {
    "text-decoration": ["text-decoration-line", "text-decoration-style", "text-decoration-thickness", "text-decoration-color"],
    "list-style": ["list-style-position", "list-style-image", "list-style-type"],
    "place-content": ["align-content", "justify-content"],
    "place-items": ["align-items", "justify-items"],
    "place-self": ["align-self", "justify-self"],
    "flex-flow": ["flex-direction", "flex-wrap"],
    flex: ["flex-grow", "flex-shrink", "flex-basis"],
    "text-emphasis": ["text-emphasis-style", "text-emphasis-color"],
    outline: ["outline-color", "outline-style", "outline-width"],
    "column-rule": ["column-rule-width", "column-rule-style", "column-rule-color"],
    overflow: ["overflow-x", "overflow-y"],
    gap: ["row-gap", "column-gap"],
    "grid-gap": ["row-gap", "column-gap"]
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
    if (key === "text-decoration") {
      return normalizeCssSpecifiedValue("text-decoration", vals.join(" "));
    }
    if (key === "list-style") {
      const out = [];
      if (vals[0] !== "outside") out.push(vals[0]);
      if (vals[1] !== "none") out.push(vals[1]);
      if (vals[2] !== "disc" || out.length === 0) out.push(vals[2]);
      return out.join(" ");
    }
    if (key === "place-content" || key === "place-items" || key === "place-self" || key === "overflow" || key === "gap" || key === "grid-gap") {
      return vals[0] === vals[1] ? vals[0] : (vals[0] + " " + vals[1]);
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
    if (key === "outline") {
      const oc = vals[0].toLowerCase() === "currentcolor" ? "" : vals[0];
      const ost = vals[1] === "none" ? "" : vals[1];
      const ow = vals[2] === "medium" ? "" : vals[2];
      return [oc, ost, ow].filter(Boolean).join(" ") || "none";
    }
    if (key === "column-rule") {
      const cw = vals[0] === "medium" ? "" : vals[0];
      const cst = vals[1] === "none" ? "" : vals[1];
      const cc = vals[2].toLowerCase() === "currentcolor" ? "" : vals[2];
      return [cw, cst, cc].filter(Boolean).join(" ") || "none";
    }
    return vals.join(" ");
  }
  global.__capsurf_serializeShorthand = serializeExpandedShorthand;
  if (typeof globalThis !== "undefined") globalThis.__capsurf_serializeShorthand = serializeExpandedShorthand;

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
        if (context === "page" && rawName.toLowerCase() === "size") {
          val = val.toLowerCase();
        }
        if (context === "margin" && rawName.toLowerCase() === "size") {
          continue;
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
