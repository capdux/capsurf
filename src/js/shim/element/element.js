// src/js/shim/element/element.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/element/element.ts

const STYLE_DECL_MEMBERS = new Set([
  "cssText", "length", "parentRule", "item", "setProperty",
  "getPropertyValue", "getPropertyPriority", "removeProperty"
]);

const STYLE_LEGACY_ALIASES = {
  "cssfloat": "float",
  "css-float": "float",
  "grid-row-gap": "row-gap",
  "grid-column-gap": "column-gap",
  "grid-gap": "gap",
  "-webkit-border-radius": "border-radius",
  "-webkit-appearance": "appearance",
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
  "-webkit-order": "order",
};

const BG_LONGHANDS = [
  "background-image",
  "background-position",
  "background-size",
  "background-repeat",
  "background-attachment",
  "background-origin",
  "background-clip",
  "background-color",
];

const CSS_WIDE_KEYWORDS = new Set(["initial", "inherit", "unset", "revert", "revert-layer"]);

function normalizeStylePropKey(p) {
  const str = String(p).trim();
  if (str.startsWith("--")) return str;
  const lower = str.toLowerCase();
  return STYLE_LEGACY_ALIASES[lower] || lower;
}

function splitCssWhitespaceTokens(val) {
  const tokens = [];
  let cur = "";
  let depth = 0;
  let quote = "";
  for (let i = 0; i < val.length; i++) {
    const ch = val[i];
    if (quote) {
      cur += ch;
      if (ch === quote && val[i - 1] !== "\\") quote = "";
    } else if (ch === '"' || ch === "'") {
      quote = ch;
      cur += ch;
    } else if (ch === "(") {
      depth++;
      cur += ch;
    } else if (ch === ")") {
      if (depth > 0) depth--;
      cur += ch;
    } else if (depth === 0 && /\s/.test(ch)) {
      if (cur) {
        tokens.push(cur);
        cur = "";
      }
    } else {
      cur += ch;
    }
  }
  if (cur) tokens.push(cur);
  return tokens;
}

function expandFourSideValues(val) {
  const parts = splitCssWhitespaceTokens(val);
  if (parts.length === 1) return [parts[0], parts[0], parts[0], parts[0]];
  if (parts.length === 2) return [parts[0], parts[1], parts[0], parts[1]];
  if (parts.length === 3) return [parts[0], parts[1], parts[2], parts[1]];
  if (parts.length === 4) return [parts[0], parts[1], parts[2], parts[3]];
  return null;
}

function collapseFourSideValues(vals) {
  if (vals[0] === vals[1] && vals[1] === vals[2] && vals[2] === vals[3]) return vals[0];
  if (vals[0] === vals[2] && vals[1] === vals[3]) return vals[0] + " " + vals[1];
  if (vals[1] === vals[3]) return vals[0] + " " + vals[1] + " " + vals[2];
  return vals.join(" ");
}

function isValidInlinePropValue(key, val) {
  const v = String(val).trim();
  if (!v) return false;
  if (key === "size" || key === "page-orientation") return false;
  if (key.startsWith("--")) return key.length > 2;
  const lower = v.toLowerCase();
  if (CSS_WIDE_KEYWORDS.has(lower)) return true;
  if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_isValidCssValue === "function" && !globalThis.__capsurf_isValidCssValue(key, v)) {
    return false;
  }
  if (key === "z-index") {
    if (lower === "auto") return true;
    if (/^[+-]?\d+$/.test(v)) return true;
    if (/^(?:calc|min|max|clamp|round|mod|rem|abs|sign)\s*\(/i.test(v)) return true;
    return false;
  }
  return true;
}

function serializeInlineSpecifiedValue(key, val) {
  if (!val || typeof val !== "string") return val || "";
  const v = val.trim();
  if (/^url\([^)]*$/i.test(v)) return v + ")";
  if (key === "border-shape") return v.replace(/\bat\s+center\s*\)$/i, "at center center)");
  if (key === "flow-tolerance" && (v === "0" || v === "+0" || v === "-0")) return "0px";
  if ((key === "grid" || key === "grid-template" || key === "grid-template-areas") &&
      typeof globalThis.__capsurf_normalizeCssValue === "function") {
    return globalThis.__capsurf_normalizeCssValue(key, v);
  }
  // CSS Color 4: legacy rgb()/rgba() with `none` channel serializes `none` as `0` for specified value
  if ((key === "color" || key.endsWith("-color")) && /^rgba?\s*\(/i.test(v) && /\bnone\b/i.test(v)) {
    const m = v.match(/^rgba?\(\s*([^\s,/]+)\s+([^\s,/]+)\s+([^\s,/]+)(?:\s*\/\s*([^\s)]+))?\s*\)$/i);
    if (m) {
      const parseC = function (c) {
        const cl = c.toLowerCase();
        if (cl === "none") return 0;
        if (cl.endsWith("%")) return Math.round((parseFloat(cl) / 100) * 255);
        return Math.round(parseFloat(cl));
      };
      const r = Math.min(255, Math.max(0, parseC(m[1])));
      const g = Math.min(255, Math.max(0, parseC(m[2])));
      const b = Math.min(255, Math.max(0, parseC(m[3])));
      if (!isNaN(r) && !isNaN(g) && !isNaN(b)) {
        if (m[4] !== undefined) {
          const al = m[4].toLowerCase();
          const a = al === "none" ? 0 : (al.endsWith("%") ? parseFloat(al) / 100 : parseFloat(al));
          if (!isNaN(a) && a < 1) {
            return "rgba(" + r + ", " + g + ", " + b + ", " + a + ")";
          }
        }
        return "rgb(" + r + ", " + g + ", " + b + ")";
      }
    }
  }
  return v;
}

function setEntryInStyleMap(map, key, rawName, val, pri) {
  if (key === "margin" || key === "padding") {
    const sides = expandFourSideValues(val);
    if (sides) {
      map.delete(key);
      const names = [key + "-top", key + "-right", key + "-bottom", key + "-left"];
      for (let i = 0; i < 4; i++) {
        map.set(names[i], { name: names[i], value: sides[i], priority: pri });
      }
      return;
    }
  }
  if (key === "gap") {
    const parts = splitCssWhitespaceTokens(val);
    if (parts.length === 1 || parts.length === 2) {
      map.delete("gap");
      map.delete("grid-gap");
      map.set("row-gap", { name: "row-gap", value: parts[0], priority: pri });
      map.set("column-gap", { name: "column-gap", value: parts.length === 2 ? parts[1] : parts[0], priority: pri });
      return;
    }
  }
  if (key === "grid" || key === "grid-template") {
    const expFn = typeof globalThis !== "undefined" && typeof globalThis.__capsurf_expandShorthand === "function" ? globalThis.__capsurf_expandShorthand : null;
    const expanded = expFn ? expFn(key, val) : null;
    if (expanded) {
      map.delete(key);
      for (let i = 0; i < expanded.length; i++) {
        const [lh, lv] = expanded[i];
        map.set(lh, { name: lh, value: lv, priority: pri });
      }
      return;
    }
  }
  if (key === "background") {
    const expFn = typeof globalThis !== "undefined" && typeof globalThis.__capsurf_expandShorthand === "function" ? globalThis.__capsurf_expandShorthand : null;
    const expanded = expFn ? expFn("background", val) : null;
    if (expanded) {
      map.delete("background");
      map.delete("background-position-x");
      map.delete("background-position-y");
      for (let i = 0; i < expanded.length; i++) {
        const [lh, lv] = expanded[i];
        map.set(lh, { name: lh, value: lv, priority: pri });
      }
      return;
    }
    map.delete("background");
    map.delete("background-position-x");
    map.delete("background-position-y");
    const lower = val.trim().toLowerCase();
    const defaults = {
      "background-image": "none",
      "background-position": "0% 0%",
      "background-size": "auto",
      "background-repeat": "repeat",
      "background-attachment": "scroll",
      "background-origin": "padding-box",
      "background-clip": "border-box",
      "background-color": "transparent",
    };
    if (lower !== "none" && lower !== "initial" && lower !== "transparent") {
      if (lower.indexOf("url(") !== -1 || lower.indexOf("gradient(") !== -1) {
        defaults["background-image"] = val;
      } else {
        defaults["background-color"] = val;
      }
    }
    for (let i = 0; i < BG_LONGHANDS.length; i++) {
      const lh = BG_LONGHANDS[i];
      map.set(lh, { name: lh, value: defaults[lh], priority: pri });
    }
    return;
  }
  map.set(key, { name: key.startsWith("--") ? rawName : key, value: val, priority: pri });
}

function removeKeyFromStyleMap(map, key) {
  let oldVal = "";
  if (key === "margin" || key === "padding") {
    if (map.has(key)) {
      oldVal = map.get(key).value;
      map.delete(key);
    }
    const names = [key + "-top", key + "-right", key + "-bottom", key + "-left"];
    for (let i = 0; i < 4; i++) {
      if (map.has(names[i])) {
        if (!oldVal) oldVal = map.get(names[i]).value;
        map.delete(names[i]);
      }
    }
    return oldVal;
  }
  if (key === "gap") {
    if (map.has("gap")) { oldVal = map.get("gap").value; map.delete("gap"); }
    if (map.has("row-gap")) { if (!oldVal) oldVal = map.get("row-gap").value; map.delete("row-gap"); }
    if (map.has("column-gap")) { if (!oldVal) oldVal = map.get("column-gap").value; map.delete("column-gap"); }
    return oldVal;
  }
  if (key === "grid" || key === "grid-template") {
    const lhs = typeof globalThis.__capsurf_shorthandLonghands === "object" && globalThis.__capsurf_shorthandLonghands[key];
    if (lhs) {
      if (map.has(key)) { oldVal = map.get(key).value; map.delete(key); }
      for (let i = 0; i < lhs.length; i++) {
        if (map.has(lhs[i])) {
          if (!oldVal) oldVal = map.get(lhs[i]).value;
          map.delete(lhs[i]);
        }
      }
      return oldVal;
    }
  }
  if (key === "background") {
    if (map.has("background")) { oldVal = map.get("background").value; map.delete("background"); }
    for (let i = 0; i < BG_LONGHANDS.length; i++) {
      if (map.has(BG_LONGHANDS[i])) {
        if (!oldVal) oldVal = map.get(BG_LONGHANDS[i]).value;
        map.delete(BG_LONGHANDS[i]);
      }
    }
    map.delete("background-position-x");
    map.delete("background-position-y");
    return oldVal;
  }
  if (key === "background-position") {
    map.delete("background-position-x");
    map.delete("background-position-y");
  }
  const old = map.get(key);
  map.delete(key);
  if (key === "float") map.delete("css-float");
  if (key === "transform" || key === "-webkit-transform" || key === "-moz-transform") {
    map.delete("transform");
    map.delete("-webkit-transform");
    map.delete("-moz-transform");
  }
  return old ? old.value : "";
}

function createStyleProxy(element) {
  function parseStyleMap() {
    const raw = element.getAttribute("style") || "";
    const map = new Map();
    const parts = raw.split(";");
    for (let i = 0; i < parts.length; i++) {
      const p = parts[i].trim();
      if (!p) continue;
      const colon = p.indexOf(":");
      if (colon !== -1) {
        const rawKey = p.slice(0, colon).trim();
        const k = normalizeStylePropKey(rawKey);
        let v = p.slice(colon + 1).trim();
        let pri = "";
        if (/!\s*important\s*$/i.test(v)) {
          pri = "important";
          v = v.replace(/\s*!\s*important\s*$/i, "").trim();
        }
        if (v !== "") {
          if (k === "place-self" || k === "place-items" || k === "place-content") {
            if (isValidInlinePropValue(k, v)) setEntryWithShorthand(map, k, rawKey, v, pri);
          } else if (k === "border-radius" || k === "column-rule" || k === "columns" || k === "border-color" || k === "border-style" || k === "border-width" ||
              (v.indexOf("var(") !== -1 && (k === "margin" || k === "padding" || (typeof globalThis.__capsurf_shorthandLonghands === "object" && globalThis.__capsurf_shorthandLonghands[k])))) {
            setEntryWithShorthand(map, k, rawKey, v, pri);
          } else {
            if (!k.startsWith("--") && /(?:^|[^a-z0-9_-])env\(/i.test(v) && !isValidInlinePropValue(k, v)) continue;
            const cur = map.get(k);
            if (!cur || cur.priority !== "important" || pri === "important") {
              setEntryInStyleMap(map, k, k === "appearance" ? "appearance" : rawKey, v, pri);
            }
          }
        }
      }
    }
    return map;
  }

  function formatStyleMap(map) {
    const arr = [];
    const skip = new Set();
    const foldPairs = [
      ["flex", ["flex-grow", "flex-shrink", "flex-basis"]],
      ["flex-flow", ["flex-direction", "flex-wrap"]]
    ];
    const folded = new Map();
    for (let fi = 0; fi < foldPairs.length; fi++) {
      const [sh, lhs] = foldPairs[fi];
      const entries = lhs.map((lh) => map.get(lh));
      if (entries.every((e) => e && entries[0] && e.priority === entries[0].priority && /^(?:inherit|initial|unset|revert|revert-layer)$/i.test(e.value) && e.value.toLowerCase() === entries[0].value.toLowerCase())) {
        lhs.forEach((lh) => skip.add(lh));
        folded.set(lhs[0], { name: sh, value: entries[0].value.toLowerCase(), priority: entries[0].priority });
      }
    }
    const brLhs = ["border-top-left-radius", "border-top-right-radius", "border-bottom-right-radius", "border-bottom-left-radius"];
    const brEntries = brLhs.map((lh) => map.get(lh));
    if (brEntries.every((e) => e && brEntries[0] && e.priority === brEntries[0].priority) &&
        typeof globalThis !== "undefined" && typeof globalThis.__capsurf_serializeShorthand === "function") {
      const brVal = globalThis.__capsurf_serializeShorthand("border-radius", (lh) => map.get(lh).value);
      if (brVal) {
        brLhs.forEach((lh) => skip.add(lh));
        folded.set(brLhs[0], { name: "border-radius", value: brVal, priority: brEntries[0].priority });
      }
    }
    if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_serializeShorthand === "function") {
      const foldShorthand = (shName, lhs) => {
        const entries = lhs.map((lh) => map.get(lh));
        if (!entries.every((e) => e && entries[0] && e.priority === entries[0].priority)) return false;
        const val = globalThis.__capsurf_serializeShorthand(shName, (lh) => map.get(lh).value);
        if (!val) return false;
        const lhSet = new Set(lhs);
        let firstKey = lhs[0];
        for (const mk of map.keys()) {
          if (lhSet.has(mk)) { firstKey = mk; break; }
        }
        lhs.forEach((lh) => skip.add(lh));
        folded.set(firstKey, { name: shName, value: val, priority: entries[0].priority });
        return true;
      };
      if (!foldShorthand("grid", ["grid-template-rows", "grid-template-columns", "grid-template-areas", "grid-auto-flow", "grid-auto-rows", "grid-auto-columns"])) {
        foldShorthand("grid-template", ["grid-template-rows", "grid-template-columns", "grid-template-areas"]);
      }
    }
    for (const [k, entry] of map.entries()) {
      if (folded.has(k)) {
        const fe = folded.get(k);
        let s = fe.name + ": " + fe.value;
        if (fe.priority) s += " !" + fe.priority;
        arr.push(s + ";");
        continue;
      }
      if (skip.has(k)) continue;
      if (entry.pendingVar && entry.pendingOwner) {
        const owner = entry.pendingOwner;
        const pv = entry.pendingVar;
        const pPri = entry.priority;
        for (const [ok, oe] of map.entries()) {
          if (oe.pendingOwner === owner && oe.pendingVar === pv && oe.priority === pPri) {
            skip.add(ok);
          }
        }
        let s = owner + ": " + pv;
        if (pPri) s += " !" + pPri;
        arr.push(s + ";");
        continue;
      }
      let s = entry.name + ": " + entry.value;
      if (entry.priority) s += " !" + entry.priority;
      arr.push(s + ";");
    }
    return arr.join(" ");
  }

  function serializeStyleMap(map) {
    const res = formatStyleMap(map);
    if (res.length > 0) {
      element.setAttribute("style", res);
    } else {
      element.setAttribute("style", "");
    }
  }

  const declTarget = Object.create(
    (typeof global !== "undefined" && global.CSSStyleDeclaration && global.CSSStyleDeclaration.prototype) ||
    (typeof globalThis !== "undefined" && globalThis.CSSStyleDeclaration && globalThis.CSSStyleDeclaration.prototype) ||
    Object.prototype
  );

  function propToKebab(p) {
    if (typeof p !== "string") return "";
    if (p.startsWith("--")) return p;
    if (p === "cssFloat") return "float";
    let body = p;
    let prefix = "";
    if (p.startsWith("webkit") || p.startsWith("Webkit")) {
      prefix = "-webkit-";
      body = p.slice(6);
    } else if (p.startsWith("Moz")) {
      prefix = "-moz-";
      body = p.slice(3);
    }
    if (body.length > 0) {
      body = body[0].toLowerCase() + body.slice(1);
    }
    return prefix + body.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
  }

  function lookupPropertyValue(map, key) {
    const entry = map.get(key);
    if (entry && key !== "font") return serializeInlineSpecifiedValue(key, entry.value);
    if (key === "margin" || key === "padding") {
      const names = [key + "-top", key + "-right", key + "-bottom", key + "-left"];
      const vals = [];
      let pri = null;
      let pVar = null;
      let hasPVar = false;
      let allPVar = true;
      for (let i = 0; i < 4; i++) {
        const e = map.get(names[i]);
        if (!e) return "";
        if (pri === null) pri = e.priority;
        else if (pri !== e.priority) return "";
        if (e.pendingVar) {
          hasPVar = true;
          if (pVar === null) pVar = e.pendingVar;
          else if (pVar !== e.pendingVar || e.pendingOwner !== key) return "";
        } else {
          allPVar = false;
        }
        vals.push(e.value);
      }
      if (hasPVar) return allPVar ? pVar : "";
      return collapseFourSideValues(vals);
    }
    if (key === "gap") {
      const rg = map.get("row-gap");
      const cg = map.get("column-gap");
      if (rg && cg && rg.priority === cg.priority) {
        return rg.value === cg.value ? rg.value : (rg.value + " " + cg.value);
      }
      return "";
    }
    if (typeof globalThis.__capsurf_shorthandLonghands === "object" &&
        globalThis.__capsurf_shorthandLonghands[key] &&
        typeof globalThis.__capsurf_serializeShorthand === "function") {
      const lhs = globalThis.__capsurf_shorthandLonghands[key];
      let pri = null;
      let pVar = null;
      let hasPVar = false;
      let allPVar = true;
      for (let i = 0; i < lhs.length; i++) {
        const e = map.get(lhs[i]);
        if (!e) return "";
        if (pri === null) pri = e.priority;
        else if (pri !== e.priority) return "";
        if (e.pendingVar) {
          hasPVar = true;
          if (pVar === null) pVar = e.pendingVar;
          else if (pVar !== e.pendingVar || e.pendingOwner !== key) return "";
        } else {
          allPVar = false;
        }
      }
      if (hasPVar) return allPVar ? pVar : "";
      return globalThis.__capsurf_serializeShorthand(key, (lh) => map.get(lh).value);
    }
    if (Object.prototype.hasOwnProperty.call(LEGACY_CSS_ALIASES, key)) {
      return lookupPropertyValue(map, LEGACY_CSS_ALIASES[key]);
    }
    if (key === "font" && map.has("font")) {
      const fVal = map.get("font").value;
      let fv = lookupPropertyValue(map, "font-variant");
      if (!fv && map.has("font-variant")) fv = map.get("font-variant").value;
      if (!fv && map.has("font-variant-caps")) {
        const capsVal = map.get("font-variant-caps").value;
        const otherFv = ["font-variant-ligatures", "font-variant-alternates", "font-variant-numeric", "font-variant-east-asian", "font-variant-position", "font-variant-emoji"];
        const allOtherNormal = otherFv.every((k) => !map.has(k) || map.get(k).value === "normal");
        fv = allOtherNormal ? capsVal : "__non_css2__";
      }
      if (fv && fv !== "normal") {
        if (fv === "small-caps") return fVal.startsWith("small-caps ") ? fVal : ("small-caps " + fVal);
        return "";
      }
      return fVal;
    }
    if (key === "float") {
      const e = map.get("css-float");
      if (e) return e.value;
    } else if (key === "-webkit-transform") {
      const e = map.get("transform");
      if (e) return e.value;
    } else if (key === "transform") {
      const e = map.get("-webkit-transform");
      if (e) return e.value;
    } else if (key === "font-stretch") {
      const e = map.get("font-width");
      if (e) return e.value;
    } else if (key === "font-width") {
      const e = map.get("font-stretch");
      if (e) return e.value;
    } else if (key === "-webkit-align-content") {
      const e = map.get("align-content");
      if (e) return e.value;
    } else if (key === "align-content") {
      const e = map.get("-webkit-align-content");
      if (e) return e.value;
    } else if (key === "-webkit-appearance") {
      const e = map.get("appearance");
      if (e) return e.value;
    } else if (key === "appearance") {
      const e = map.get("-webkit-appearance");
      if (e) return e.value;
    }
    return "";
  }

  const LEGACY_CSS_ALIASES = {
    "-webkit-appearance": "appearance",
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
    "-webkit-order": "order",
    "font-stretch": "font-width"
  };

  function setEntryWithShorthand(map, key, rawName, normV, pri) {
    if (Object.prototype.hasOwnProperty.call(LEGACY_CSS_ALIASES, key)) {
      const target = LEGACY_CSS_ALIASES[key];
      setEntryWithShorthand(map, target, target, normV, pri);
      return;
    }
    if (!key.startsWith("--") && typeof globalThis.__capsurf_expandShorthand === "function") {
      let expanded = globalThis.__capsurf_expandShorthand(key, normV);
      const isPendingVar = normV.indexOf("var(") !== -1;
      if (!expanded && isPendingVar && typeof globalThis.__capsurf_shorthandLonghands === "object" && globalThis.__capsurf_shorthandLonghands[key]) {
        expanded = globalThis.__capsurf_shorthandLonghands[key].map((lh) => [lh, ""]);
      }
      if (expanded) {
        map.delete(key);
        for (let i = 0; i < expanded.length; i++) {
          const lh = expanded[i][0];
          const cur = map.get(lh);
          if (cur && cur.priority === "important" && pri !== "important") continue;
          if (isPendingVar) {
            map.set(lh, { name: lh, value: "", priority: pri, pendingVar: normV, pendingOwner: key });
          } else {
            setEntryInStyleMap(map, lh, lh, expanded[i][1], pri);
          }
        }
        return;
      }
    }
    setEntryInStyleMap(map, key, rawName, normV, pri);
    if (key === "font" && typeof globalThis.__capsurf_shorthandLonghands === "object" && globalThis.__capsurf_shorthandLonghands["font-variant"]) {
      const fvLhs = globalThis.__capsurf_shorthandLonghands["font-variant"];
      for (let i = 0; i < fvLhs.length; i++) {
        setEntryInStyleMap(map, fvLhs[i], fvLhs[i], "normal", pri);
      }
    }
  }

  function removeKeyWithShorthand(map, key) {
    if (Object.prototype.hasOwnProperty.call(LEGACY_CSS_ALIASES, key)) {
      return removeKeyWithShorthand(map, LEGACY_CSS_ALIASES[key]);
    }
    let prev = removeKeyFromStyleMap(map, key);
    if (!key.startsWith("--") && typeof globalThis.__capsurf_shorthandLonghands === "object" && globalThis.__capsurf_shorthandLonghands[key]) {
      const lhs = globalThis.__capsurf_shorthandLonghands[key];
      for (let i = 0; i < lhs.length; i++) {
        const p = removeKeyFromStyleMap(map, lhs[i]);
        if (!prev && p) prev = p;
      }
    }
    return prev;
  }

  const cachedMethods = {
    item: function (idx) {
      const i = Number(idx) >>> 0;
      const map = parseStyleMap();
      let count = 0;
      for (const entry of map.values()) {
        if (count === i) return entry.name;
        count++;
      }
      return "";
    },
    getPropertyValue: function (p) {
      const map = parseStyleMap();
      const key = normalizeStylePropKey(p);
      return lookupPropertyValue(map, key);
    },
    getPropertyPriority: function (p) {
      const map = parseStyleMap();
      const key = normalizeStylePropKey(p);
      const entry = map.get(key);
      return entry ? entry.priority : "";
    },
    setProperty: function (p, val, priority) {
      if (priority !== undefined && priority !== null && priority !== "") {
        if (priority !== "important" && priority !== "!important") return;
      }
      const pPriority = (priority === "important" || priority === "!important") ? "important" : "";
      const map = parseStyleMap();
      const rawStr = String(p).trim();
      const key = normalizeStylePropKey(rawStr);
      const vStr = val === null || val === undefined ? "" : String(val).trim();
      if (vStr === "") {
        removeKeyWithShorthand(map, key);
        if (key === "display") element._displayNone = false;
        serializeStyleMap(map);
        return;
      }
      if (!isValidInlinePropValue(key, vStr)) return;
      const normV = (!key.startsWith("--") && typeof globalThis.__capsurf_normalizeCssValue === "function")
        ? globalThis.__capsurf_normalizeCssValue(key, vStr)
        : vStr;
      setEntryWithShorthand(map, key, rawStr, normV, pPriority);
      if (key === "display") {
        element._displayNone = (vStr.toLowerCase() === "none");
      }
      serializeStyleMap(map);
    },
    removeProperty: function (p) {
      const map = parseStyleMap();
      const key = normalizeStylePropKey(p);
      const prev = removeKeyWithShorthand(map, key);
      if (key === "display") {
        element._displayNone = false;
      }
      serializeStyleMap(map);
      return prev;
    },
  };

  const styleProxy = new Proxy(declTarget, {
    get: function (target, prop, receiver) {
      if (typeof prop === "symbol") {
        if (prop === Symbol.iterator) {
          return function* () {
            const map = parseStyleMap();
            for (const entry of map.values()) yield entry.name;
          };
        }
        return target[prop];
      }
      if (typeof prop !== "string") return undefined;
      if (prop === "cssText") return formatStyleMap(parseStyleMap());
      if (prop === "length") return parseStyleMap().size;
      if (prop === "parentRule") return null;
      if (Object.prototype.hasOwnProperty.call(cachedMethods, prop)) return cachedMethods[prop];
      if (prop in target) return target[prop];

      if (prop === "cssFloat" || prop === "float") {
        const map = parseStyleMap();
        const e = map.get("float") || map.get("css-float");
        return e ? e.value : "";
      }
      if (prop === "webkitTransform") {
        const map = parseStyleMap();
        const e = map.get("-webkit-transform") || map.get("transform");
        return e ? e.value : "";
      }
      if (prop === "transform") {
        const map = parseStyleMap();
        const e = map.get("transform") || map.get("-webkit-transform");
        return e ? e.value : "";
      }

      // CamelCase or kebab-case lookup
      if (!prop.startsWith("--") && typeof globalThis !== "undefined" &&
          typeof globalThis.__isCssPropertySupported === "function" &&
          !globalThis.__isCssPropertySupported(prop)) {
        return undefined;
      }
      const map = parseStyleMap();
      if (prop !== "font" && map.has(prop)) return serializeInlineSpecifiedValue(prop, map.get(prop).value);
      const kebab = normalizeStylePropKey(propToKebab(prop));
      return lookupPropertyValue(map, kebab);
    },
    set: function (target, prop, value) {
      if (typeof prop === "symbol") {
        target[prop] = value;
        return true;
      }
      if (typeof prop !== "string") return true;
      if (prop === "cssText") {
        const valStr = String(value);
        element.setAttribute("style", valStr);
        serializeStyleMap(parseStyleMap());
        element._displayNone = /(?:^|;)\s*display\s*:\s*none\b/i.test(valStr);
        return true;
      }
      if (prop === "cssFloat" || prop === "float") {
        const map = parseStyleMap();
        map.delete("css-float");
        const vStr = value === null || value === undefined ? "" : String(value).trim();
        if (vStr === "") {
          map.delete("float");
        } else {
          if (!isValidInlinePropValue("float", vStr)) return true;
          map.set("float", { name: "float", value: vStr, priority: "" });
        }
        serializeStyleMap(map);
        return true;
      }
      const rawKey = propToKebab(prop);
      const normKey = normalizeStylePropKey(rawKey);
      const map = parseStyleMap();
      const vStr = value === null || value === undefined ? "" : String(value).trim();
      if (vStr === "") {
        removeKeyWithShorthand(map, normKey);
        if (normKey === "display") element._displayNone = false;
      } else {
        if (!isValidInlinePropValue(normKey, vStr)) return true;
        const normV = (!normKey.startsWith("--") && typeof globalThis.__capsurf_normalizeCssValue === "function")
          ? globalThis.__capsurf_normalizeCssValue(normKey, vStr)
          : vStr;
        setEntryWithShorthand(map, normKey, rawKey, normV, "");
        if (normKey === "display") {
          element._displayNone = (vStr.toLowerCase() === "none");
        }
      }
      serializeStyleMap(map);
      return true;
    },
    has: function (target, prop) {
      if (typeof prop === "symbol") return prop === Symbol.iterator || prop in target;
      const pStr = String(prop);
      if (pStr === "cssFloat" || pStr === "float") return true;
      if (STYLE_DECL_MEMBERS.has(pStr)) return true;
      if (prop in target) return true;
      if (pStr.startsWith("--")) return false;
      if (/^(?:webkit|moz|ms|o)-/.test(pStr)) return false;
      if (!pStr.includes("-") && /^(?:moz|ms|o)[A-Z]/.test(pStr)) return false;
      if (typeof global !== "undefined" && typeof global.__isCssPropertySupported === "function") {
        return global.__isCssPropertySupported(pStr);
      }
      if (typeof globalThis !== "undefined" && typeof globalThis.__isCssPropertySupported === "function") {
        return globalThis.__isCssPropertySupported(pStr);
      }
      return false;
    },
    ownKeys: function (target) {
      const map = parseStyleMap();
      const keys = [];
      const seen = new Set();
      for (const [k, entry] of map.entries()) {
        const rawName = entry && entry.name ? entry.name : k;
        const name = rawName.startsWith("--") ? rawName : rawName.replace(/-([a-z])/g, function (_, c) { return c.toUpperCase(); });
        if (!seen.has(name)) {
          seen.add(name);
          keys.push(name);
        }
      }
      const targetKeys = Reflect.ownKeys(target);
      for (let i = 0; i < targetKeys.length; i++) {
        const k = targetKeys[i];
        if (!seen.has(k)) {
          seen.add(k);
          keys.push(k);
        }
      }
      return keys;
    },
    getOwnPropertyDescriptor: function (target, prop) {
      const own = Reflect.getOwnPropertyDescriptor(target, prop);
      if (own) return own;
      if (typeof prop === "string") {
        const map = parseStyleMap();
        if (map.has(prop)) {
          return { value: map.get(prop).value, writable: true, enumerable: true, configurable: true };
        }
        const kebab = propToKebab(prop);
        if (map.has(kebab)) {
          return { value: map.get(kebab).value, writable: true, enumerable: true, configurable: true };
        }
        const lower = prop.startsWith("--") ? prop : prop.toLowerCase();
        if (map.has(lower)) {
          return { value: map.get(lower).value, writable: true, enumerable: true, configurable: true };
        }
      }
      return undefined;
    },
    preventExtensions: function () {
      return false;
    }
  });
  if (typeof global !== "undefined" && global.__capsurf_inlineStyleLookup) {
    global.__capsurf_inlineStyleLookup.set(styleProxy, cachedMethods.getPropertyValue);
  }
  return styleProxy;
}

function isHtmlElement(el) {
  if (!el) return true;
  if (el._namespaceURI !== undefined) {
    return el._namespaceURI === null || el._namespaceURI === "http://www.w3.org/1999/xhtml";
  }
  let curr = el;
  while (curr) {
    if (curr._namespaceURI !== undefined) {
      return curr._namespaceURI === null || curr._namespaceURI === "http://www.w3.org/1999/xhtml";
    }
    const tag = String(curr.localName || curr.tagName || "").toLowerCase();
    if (tag === "svg" || tag === "math") return false;
    curr = curr.parentElement || curr.parentNode;
  }
  return true;
}

function installElementExtensions(proto) {
  if (!proto) return;

  // Make Element inherit from Node / EventTarget
  Object.setPrototypeOf(proto, Node.prototype);
  proto.appendChild = Node.prototype.appendChild;
  proto.insertBefore = Node.prototype.insertBefore;
  proto.removeChild = Node.prototype.removeChild;

  if (proto.nodeType === undefined) {
    Object.defineProperty(proto, "nodeType", {
      value: ELEMENT_NODE,
      writable: false,
      configurable: true,
    });
  }

  Object.defineProperty(proto, "nodeName", {
    get: function () {
      return this.tagName || "";
    },
    configurable: true,
  });

  Object.defineProperty(proto, "localName", {
    get: function () {
      return (this.tagName || "").replace(/[A-Z]/g, (c) => String.fromCharCode(c.charCodeAt(0) + 32));
    },
    configurable: true,
  });

  Object.defineProperty(proto, "prefix", {
    get: function () {
      if (this._prefix !== undefined) return this._prefix;
      const tag = this.tagName || "";
      const colon = tag.indexOf(":");
      return colon !== -1 ? tag.slice(0, colon) : null;
    },
    configurable: true,
  });

  let _internalGetAttribute = typeof proto.getAttribute === "function" ? proto.getAttribute : null;
  Object.defineProperty(proto, "namespaceURI", {
    get: function () {
      if (this._namespaceURI !== undefined) return this._namespaceURI;
      const ga = _internalGetAttribute || proto.getAttribute || this.getAttribute;
      if (ga) {
        const xmlns = ga.call(this, "xmlns");
        if (xmlns !== null && xmlns !== undefined && xmlns !== "") return xmlns;
      }
      let curr = this;
      while (curr) {
        const tag = String(curr.localName || curr.tagName || "").toLowerCase();
        if (tag === "svg") return "http://www.w3.org/2000/svg";
        if (tag === "math") return "http://www.w3.org/1998/Math/MathML";
        curr = curr.parentElement || curr.parentNode;
      }
      const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
      if (!doc || !doc.contentType || doc.contentType === "text/html") {
        return "http://www.w3.org/1999/xhtml";
      }
      return null;
    },
    set: function (v) {
      this._namespaceURI = v;
    },
    configurable: true,
  });

  Object.defineProperty(proto, "data", {
    get: function () {
      if (this.nodeType === 3 || this.nodeType === 8) {
        if (this._data !== undefined) return this._data;
        if (nativeElementGetTextContent) {
          try { return String(nativeElementGetTextContent.call(this) || ""); } catch (e) {}
        }
      }
      return this.textContent || "";
    },
    set: function (v) {
      const str = v === null ? "" : String(v);
      if (this.nodeType === 3 || this.nodeType === 8) {
        let cur = "";
        if (this._data !== undefined) {
          cur = String(this._data);
        } else if (nativeElementGetTextContent) {
          try { cur = String(nativeElementGetTextContent.call(this) || ""); } catch (e) {}
        }
        characterDataReplaceData(this, 0, cur.length, str);
        return;
      }
      this.textContent = str;
      if (typeof notifyMutation === "function") {
        notifyMutation(new MutationRecord("characterData", this));
      }
    },
    configurable: true,
  });

  if (!proto.lookupNamespaceURI) {
    proto.lookupNamespaceURI = Node.prototype.lookupNamespaceURI;
  }

  if (!proto.append) {
    proto.append = function (...nodes) {
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
    markNative(proto.append, "append");
  }

  if (!proto.replaceChildren) {
    proto.replaceChildren = function (...nodes) {
      replaceChildrenImpl(this, nodes);
    };
    markNative(proto.replaceChildren, "replaceChildren");
  }

  if (!proto.remove) {
    proto.remove = function () {
      const parent = this.parentElement || this.parentNode;
      if (parent && parent.removeChild) {
        parent.removeChild(this);
      }
    };
    markNative(proto.remove, "remove");
  }

  // Children and Element Traversal ([SameObject] semantics)
  Object.defineProperty(proto, "children", {
    get: function () {
      return getElementChildrenCollection(this);
    },
    configurable: true,
  });

  Object.defineProperty(proto, "firstElementChild", {
    get: function () {
      const kids = this.children;
      return (kids && kids.length > 0) ? kids[0] : null;
    },
    configurable: true,
  });

  Object.defineProperty(proto, "lastElementChild", {
    get: function () {
      const kids = this.children;
      return (kids && kids.length > 0) ? kids[kids.length - 1] : null;
    },
    configurable: true,
  });

  Object.defineProperty(proto, "childElementCount", {
    get: function () {
      const kids = this.children;
      return kids ? kids.length : 0;
    },
    configurable: true,
  });

  const origPrevSibDesc = Object.getOwnPropertyDescriptor(proto, "previousSibling");
  const nodePrevSibDesc = typeof Node !== "undefined" ? Object.getOwnPropertyDescriptor(Node.prototype, "previousSibling") : null;
  if (nodePrevSibDesc && nodePrevSibDesc.get) {
    Object.defineProperty(proto, "previousSibling", {
      get: function () {
        if (origPrevSibDesc && origPrevSibDesc.get) {
          const v = origPrevSibDesc.get.call(this);
          if (v) return v;
        }
        return nodePrevSibDesc.get.call(this);
      },
      configurable: true,
    });
  }

  const origNextSibDesc = Object.getOwnPropertyDescriptor(proto, "nextSibling");
  const nodeNextSibDesc = typeof Node !== "undefined" ? Object.getOwnPropertyDescriptor(Node.prototype, "nextSibling") : null;
  if (nodeNextSibDesc && nodeNextSibDesc.get) {
    Object.defineProperty(proto, "nextSibling", {
      get: function () {
        if (origNextSibDesc && origNextSibDesc.get) {
          const v = origNextSibDesc.get.call(this);
          if (v) return v;
        }
        return nodeNextSibDesc.get.call(this);
      },
      configurable: true,
    });
  }

  Object.defineProperty(proto, "previousElementSibling", {
    get: function () {
      let sib = this.previousSibling;
      while (sib) {
        if (sib.nodeType === 1) return sib;
        sib = sib.previousSibling;
      }
      return null;
    },
    configurable: true,
  });

  Object.defineProperty(proto, "nextElementSibling", {
    get: function () {
      let sib = this.nextSibling;
      while (sib) {
        if (sib.nodeType === 1) return sib;
        sib = sib.nextSibling;
      }
      return null;
    },
    configurable: true,
  });

  // classList getter
  Object.defineProperty(proto, "classList", {
    get: function () {
      if (!this._classList) {
        const self = this;
        this._classList = new DOMTokenList(
          function () {
            const cn = self.className;
            if (typeof cn === "string") return cn;
            if (cn && typeof cn.baseVal === "string") return cn.baseVal;
            return (self.getAttribute && self.getAttribute("class")) || "";
          },
          function (val) {
            if (self.className && typeof self.className === "object" && "baseVal" in self.className) {
              if (self.setAttribute) self.setAttribute("class", String(val));
            } else {
              self.className = val;
            }
            if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_notifyCssStyleChange === "function") {
              globalThis.__capsurf_notifyCssStyleChange(self);
            }
          }
        );
      }
      return this._classList;
    },
    configurable: true,
  });

  // part getter/setter ([SameObject, PutForwards=value] readonly attribute DOMTokenList part)
  Object.defineProperty(proto, "part", {
    get: function () {
      if (!this || this === proto || typeof this.getAttribute !== "function") {
        throw new TypeError("Illegal invocation");
      }
      if (!this._partList) {
        const self = this;
        this._partList = new DOMTokenList(
          function () { return self.getAttribute("part") || ""; },
          function (val) {
            if (self.setAttribute) self.setAttribute("part", String(val));
            if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_notifyCssStyleChange === "function") {
              globalThis.__capsurf_notifyCssStyleChange(self);
            }
          }
        );
      }
      return this._partList;
    },
    set: function (v) {
      if (!this || this === proto || typeof this.getAttribute !== "function") {
        throw new TypeError("Illegal invocation");
      }
      this.part.value = v == null ? "" : String(v);
    },
    enumerable: true,
    configurable: true,
  });

  // relList getter/setter (HTML a/area/link, SVG a, MathML a)
  Object.defineProperty(proto, "relList", {
    get: function () {
      const ns = this.namespaceURI;
      const local = String(this.localName || this.tagName || "").toLowerCase();
      const ok =
        (ns === "http://www.w3.org/1999/xhtml" && (local === "a" || local === "area" || local === "link")) ||
        ((ns === "http://www.w3.org/2000/svg" || ns === "http://www.w3.org/1998/Math/MathML") && local === "a");
      if (!ok) return undefined;
      if (!this._relList) {
        const self = this;
        const supportedRel = local === "link"
          ? new Set(["stylesheet", "icon", "preload", "prefetch", "preconnect", "dns-prefetch", "modulepreload", "canonical", "manifest", "alternate", "pingback"])
          : new Set(["noopener", "noreferrer", "opener", "nofollow", "external", "help", "license", "next", "prev", "search", "tag"]);
        this._relList = new DOMTokenList(
          function () {
            return (self.getAttribute && self.getAttribute("rel")) || "";
          },
          function (val) {
            if (self.setAttribute) self.setAttribute("rel", String(val));
          },
          supportedRel
        );
      }
      return this._relList;
    },
    set: function (v) {
      if (this.relList === undefined) return;
      if (this.setAttribute) this.setAttribute("rel", String(v));
    },
    configurable: true,
  });

  // Private internal slots stored in WeakMaps to avoid leaking own properties on Element instances
  const elementStyleProxyMap = new WeakMap();
  const elementAttrsMap = new WeakMap();
  const elementAttrListMap = new WeakMap();
  const elementDisplayNoneMap = new WeakMap();

  Object.defineProperty(proto, "_attrs", {
    get: function () { return elementAttrsMap.get(this); },
    set: function (v) { elementAttrsMap.set(this, v); },
    configurable: true,
  });
  Object.defineProperty(proto, "_attrList", {
    get: function () { return elementAttrListMap.get(this); },
    set: function (v) { elementAttrListMap.set(this, v); },
    configurable: true,
  });
  Object.defineProperty(proto, "_displayNone", {
    get: function () { return elementDisplayNoneMap.get(this); },
    set: function (v) { elementDisplayNoneMap.set(this, v); },
    configurable: true,
  });

  // style getter/setter
  Object.defineProperty(proto, "style", {
    get: function () {
      let sp = elementStyleProxyMap.get(this);
      if (!sp) {
        sp = createStyleProxy(this);
        elementStyleProxyMap.set(this, sp);
      }
      return sp;
    },
    set: function (val) {
      this.style.cssText = val == null ? "" : String(val);
    },
    configurable: true,
  });

  // Internal attribute synchronization
  proto._syncAttrs = function () {
    if (!this._attrs) {
      this._attrs = new Map();
      this._attrList = [];
      const isHtml = isHtmlElement(this);
      if (typeof this._getRawAttrs === "function") {
        const raw = this._getRawAttrs();
        if (raw && Array.isArray(raw)) {
          for (let i = 0; i < raw.length; i++) {
            const item = raw[i];
            if (item && item.name) {
              const colon = item.name.indexOf(":");
              const prefix = colon !== -1 ? item.name.slice(0, colon) : null;
              const localName = colon !== -1 ? item.name.slice(colon + 1) : item.name;
              let ns = null;
              if (prefix === "xml") ns = "http://www.w3.org/XML/1998/namespace";
              else if (prefix === "xmlns" || item.name === "xmlns") ns = "http://www.w3.org/2000/xmlns/";
              else if (prefix === "xlink") ns = "http://www.w3.org/1999/xlink";
              const entry = {
                name: item.name,
                value: item.value || "",
                namespaceURI: ns,
                localName: localName,
                prefix: prefix,
              };
              this._attrList.push(entry);
              const mapKey = isHtml ? item.name.toLowerCase() : item.name;
              this._attrs.set(mapKey, entry);
            }
          }
        }
      }
      if (this.id && !this._attrs.has("id")) {
        const entry = { name: "id", value: this.id, namespaceURI: null, localName: "id", prefix: null };
        this._attrList.push(entry);
        this._attrs.set("id", entry);
      }
      if (this.className && typeof this.className === "string" && !this._attrs.has("class")) {
        const entry = { name: "class", value: this.className, namespaceURI: null, localName: "class", prefix: null };
        this._attrList.push(entry);
        this._attrs.set("class", entry);
      }
      if (this.hasAttribute && this.hasAttribute("style") && !this._attrs.has("style")) {
        const entry = { name: "style", value: this.getAttribute("style") || "", namespaceURI: null, localName: "style", prefix: null };
        this._attrList.push(entry);
        this._attrs.set("style", entry);
      }
    }
  };

  if (!Object.getOwnPropertyDescriptor(proto, "currentCSSZoom")) {
    Object.defineProperty(proto, "currentCSSZoom", {
      get: function () {
        if (!this.isConnected) return 1;
        let total = 1;
        let cur = this;
        while (cur && cur.nodeType === 1) {
          const cs = (typeof getComputedStyle === "function") ? getComputedStyle(cur) : null;
          if (!cs || cs.display === "none") return 1;
          const rawZoom = String(cs.zoom || "1").trim();
          if (rawZoom && rawZoom !== "normal" && rawZoom !== "1") {
            let z = parseFloat(rawZoom);
            if (rawZoom.endsWith("%")) z /= 100;
            if (Number.isFinite(z) && z > 0) total *= z;
          }
          cur = cur.parentElement;
        }
        return total;
      },
      configurable: true,
    });
  }

  // attributes getter returning live NamedNodeMap
  Object.defineProperty(proto, "attributes", {
    get: function () {
      if (!this._attributesMap) {
        this._attributesMap = new NamedNodeMap(this);
      } else {
        this._attributesMap._sync();
      }
      return this._attributesMap;
    },
    configurable: true,
  });

  // setAttribute
  const origSetAttribute = proto.setAttribute;
  proto.setAttribute = function (name, value) {
    const isHtml = isHtmlElement(this);
    const key = isHtml ? String(name).toLowerCase() : String(name);
    const val = String(value);
    if (!this._attrs) this._syncAttrs();
    let existing = null;
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const match = isHtml
          ? this._attrList[i].name.toLowerCase() === key
          : this._attrList[i].name === key;
        if (match) {
          existing = this._attrList[i];
          break;
        }
      }
    }
    const oldVal = existing ? existing.value : (this._attrs.get(key)?.value ?? null);
    if (existing) {
      existing.value = val;
    } else {
      const colon = key.indexOf(":");
      const prefix = colon !== -1 ? key.slice(0, colon) : null;
      const localName = colon !== -1 ? key.slice(colon + 1) : key;
      let ns = null;
      if (prefix === "xml") ns = "http://www.w3.org/XML/1998/namespace";
      else if (prefix === "xmlns" || key === "xmlns") ns = "http://www.w3.org/2000/xmlns/";
      else if (prefix === "xlink") ns = "http://www.w3.org/1999/xlink";
      const entry = {
        name: key,
        value: val,
        namespaceURI: ns,
        localName: localName,
        prefix: prefix,
      };
      if (!this._attrList) this._attrList = [];
      this._attrList.push(entry);
    }
    this._attrs.set(key, { name: key, value: val });
    if (key === "style" || key === "class" || key === "id") {
      if (typeof globalThis !== "undefined") {
        globalThis.__capsurf_cssom_epoch = (globalThis.__capsurf_cssom_epoch || 0) + 1;
        globalThis.__capsurf_domMutationEpoch = (globalThis.__capsurf_domMutationEpoch || 0) + 1;
      }
    }
    if (key.toLowerCase() === "async") {
      if (typeof globalThis !== "undefined" && globalThis.__capsurf_clearScriptForceAsync) {
        globalThis.__capsurf_clearScriptForceAsync(this);
      }
    }
    if (key.toLowerCase() === "id") {
      this.id = val;
      if (this.isConnected && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_syncNamedGlobals === "function") {
        globalThis.__capsurf_syncNamedGlobals();
      }
    } else if (key.toLowerCase() === "name") {
      if (this.isConnected && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_syncNamedGlobals === "function") {
        globalThis.__capsurf_syncNamedGlobals();
      }
    } else if (key.toLowerCase() === "style") {
      this._displayNone = /(?:^|;)\s*display\s*:\s*none\b/i.test(val);
    } else if (key.toLowerCase() === "class" && typeof this.className === "string") {
      this.className = val;
    } else if (key.toLowerCase() === "src" && this.isConnected && !this._suppressSetAttrNav && ((this.tagName || "").toLowerCase() === "iframe" || (this.tagName || "").toLowerCase() === "frame") && !(this.hasAttribute && this.hasAttribute("srcdoc"))) {
      if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_navigate_iframe === "function") {
        let resolved = val;
        try {
          const base = (typeof document !== "undefined" && document.baseURI)
            ? document.baseURI
            : (globalThis.location ? globalThis.location.href : "about:blank");
          resolved = new URL(val, base).href;
        } catch (e) {}
        globalThis.__capsurf_navigate_iframe(this, resolved);
      }
    } else if (key.toLowerCase() === "selected" && (this.tagName || "").toLowerCase() === "option") {
      this._selected = true;
      let curr = this.parentElement || this.parentNode;
      while (curr) {
        if ((curr.tagName || "").toLowerCase() === "select") {
          if (!curr.multiple) {
            curr._selectedIndex = undefined;
            const opts = curr.options;
            for (let i = 0; i < opts.length; i++) {
              if (opts[i] !== this && (!this._nid || opts[i]._nid !== this._nid)) {
                opts[i]._selected = false;
              }
            }
          }
          break;
        }
        curr = curr.parentElement || curr.parentNode;
      }
    }
    if (origSetAttribute) {
      origSetAttribute.call(this, key, val);
    }
    if (this._attributesMap) {
      this._attributesMap._sync();
    }
    if (typeof notifyMutation === "function") {
      const rec = new MutationRecord("attributes", this);
      rec.attributeName = key;
      rec.oldValue = oldVal;
      notifyMutation(rec);
    }
    if (typeof notifyCustomElementAttributeChanged === "function") {
      notifyCustomElementAttributeChanged(this, key, oldVal, val, null);
    }
    if ((key === "class" || key === "style") && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_notifyCssStyleChange === "function") {
      globalThis.__capsurf_notifyCssStyleChange(this);
    }
  };
  markNative(proto.setAttribute, "setAttribute");

  // removeAttribute
  const origRemoveAttribute = proto.removeAttribute;
  proto.removeAttribute = function (name) {
    const isHtml = isHtmlElement(this);
    const key = isHtml ? String(name).toLowerCase() : String(name);
    if (!this._attrs) this._syncAttrs();
    let oldVal = null;
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const match = isHtml
          ? this._attrList[i].name.toLowerCase() === key
          : this._attrList[i].name === key;
        if (match) {
          oldVal = this._attrList[i].value;
          this._attrList.splice(i, 1);
          break;
        }
      }
    }
    const oldEntry = this._attrs.get(key);
    if (!oldVal && oldEntry) oldVal = oldEntry.value;
    this._attrs.delete(key);
    if (key.toLowerCase() === "async") {
      if (typeof globalThis !== "undefined" && globalThis.__capsurf_clearScriptForceAsync) {
        globalThis.__capsurf_clearScriptForceAsync(this);
      }
    }
    if (key.toLowerCase() === "id") {
      this.id = "";
    } else if (key.toLowerCase() === "style") {
      this._displayNone = false;
    } else if (key.toLowerCase() === "class" && typeof this.className === "string") {
      this.className = "";
    } else if (key.toLowerCase() === "src" && this.isConnected && !this._suppressSetAttrNav && ((this.tagName || "").toLowerCase() === "iframe" || (this.tagName || "").toLowerCase() === "frame") && !(this.hasAttribute && this.hasAttribute("srcdoc"))) {
      if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_navigate_iframe === "function") {
        globalThis.__capsurf_navigate_iframe(this, "about:blank");
      }
    } else if (key.toLowerCase() === "selected" && (this.tagName || "").toLowerCase() === "option") {
      this._selected = false;
      let curr = this.parentElement || this.parentNode;
      while (curr) {
        if ((curr.tagName || "").toLowerCase() === "select") {
          if (!curr.multiple) {
            curr._selectedIndex = undefined;
          }
          break;
        }
        curr = curr.parentElement || curr.parentNode;
      }
    }

    if (origRemoveAttribute) {
      origRemoveAttribute.call(this, name);
    }
    if (this._attributesMap) {
      this._attributesMap._sync();
    }
    if (typeof notifyMutation === "function") {
      const rec = new MutationRecord("attributes", this);
      rec.attributeName = name;
      rec.oldValue = oldVal;
      notifyMutation(rec);
    }
    if (typeof notifyCustomElementAttributeChanged === "function") {
      notifyCustomElementAttributeChanged(this, key, oldVal, null, null);
    }
    if ((key === "id" || key === "name") && this.isConnected && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_syncNamedGlobals === "function") {
      globalThis.__capsurf_syncNamedGlobals();
    }
  };
  markNative(proto.removeAttribute, "removeAttribute");

  // removeAttributeNode
  proto.removeAttributeNode = function (attr) {
    if (!attr || !attr.name) return null;
    if (attr.ownerElement && attr.ownerElement !== this) {
      const err = new Error("NotFoundError: Attr does not belong to this element");
      err.name = "NotFoundError";
      throw err;
    }
    this.removeAttribute(attr.name);
    attr.ownerElement = null;
    return attr;
  };
  markNative(proto.removeAttributeNode, "removeAttributeNode");

  // getAttributeNode & setAttributeNode
  proto.getAttributeNode = function (name) {
    if (!this._attributesMap) this._attributesMap = new NamedNodeMap(this);
    return this._attributesMap.getNamedItem(name);
  };
  markNative(proto.getAttributeNode, "getAttributeNode");

  proto.getAttributeNodeNS = function (namespace, localName) {
    if (!this._attributesMap) this._attributesMap = new NamedNodeMap(this);
    return this._attributesMap.getNamedItemNS(namespace, localName);
  };
  markNative(proto.getAttributeNodeNS, "getAttributeNodeNS");

  proto.setAttributeNode = function (attr) {
    if (!attr || !attr.name) throw new TypeError("Attr required");
    if (!this._attributesMap) this._attributesMap = new NamedNodeMap(this);
    return this._attributesMap.setNamedItem(attr);
  };
  markNative(proto.setAttributeNode, "setAttributeNode");

  // getAttribute
  const origGetAttribute = proto.getAttribute;
  proto.getAttribute = function (name) {
    const isHtml = isHtmlElement(this);
    const key = isHtml ? String(name).toLowerCase() : String(name);
    if (!this._attrs) this._syncAttrs();
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const match = isHtml
          ? this._attrList[i].name.toLowerCase() === key
          : this._attrList[i].name === key;
        if (match) {
          return this._attrList[i].value;
        }
      }
    }
    if (this._attrs) {
      const entry = this._attrs.get(key);
      if (entry) return entry.value;
      if (isHtml && origGetAttribute) {
        const val = origGetAttribute.call(this, name);
        if (val !== null) return val;
      }
      return null;
    }
    if (isHtml && origGetAttribute) {
      return origGetAttribute.call(this, name);
    }
    return null;
  };
  markNative(proto.getAttribute, "getAttribute");

  // hasAttributes
  proto.hasAttributes = function () {
    if (!this._attrs) this._syncAttrs();
    if (this._attrList && this._attrList.length > 0) return true;
    return !!(this._attrs && this._attrs.size > 0);
  };
  markNative(proto.hasAttributes, "hasAttributes");

  // hasAttribute
  const origHasAttribute = proto.hasAttribute;
  proto.hasAttribute = function (name) {
    const isHtml = isHtmlElement(this);
    const key = isHtml ? String(name).toLowerCase() : String(name);
    if (!this._attrs) this._syncAttrs();
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const match = isHtml
          ? this._attrList[i].name.toLowerCase() === key
          : this._attrList[i].name === key;
        if (match) {
          return true;
        }
      }
    }
    if (this._attrs) {
      if (this._attrs.has(key)) return true;
      if (isHtml && origHasAttribute) return origHasAttribute.call(this, name);
      return false;
    }
    if (isHtml && origHasAttribute) {
      return origHasAttribute.call(this, name);
    }
    return false;
  };
  markNative(proto.hasAttribute, "hasAttribute");

  // getAttributeNames
  proto.getAttributeNames = function () {
    if (!this._attrs) this._syncAttrs();
    const names = [];
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        names.push(this._attrList[i].name);
      }
    } else {
      for (const entry of this._attrs.values()) {
        names.push(entry.name);
      }
    }
    return names;
  };
  markNative(proto.getAttributeNames, "getAttributeNames");

  // getAttributeNS
  proto.getAttributeNS = function (namespace, localName) {
    if (typeof localName !== "string") return null;
    if (!this._attrs) this._syncAttrs();
    const ns = (namespace === undefined || namespace === null || namespace === "") ? null : String(namespace);
    const loc = String(localName).toLowerCase();
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const item = this._attrList[i];
        const itemNs = item.namespaceURI || null;
        const itemLoc = (item.localName || item.name).toLowerCase();
        if (itemNs === ns && itemLoc === loc) {
          return item.value;
        }
      }
      if (ns === null) {
        for (let i = 0; i < this._attrList.length; i++) {
          const item = this._attrList[i];
          if ((item.namespaceURI === null || item.namespaceURI === undefined) &&
              item.name.toLowerCase() === loc) {
            return item.value;
          }
        }
      }
      return null;
    }
    return null;
  };
  markNative(proto.getAttributeNS, "getAttributeNS");

  // hasAttributes
  proto.hasAttributes = function () {
    if (!this._attrs) this._syncAttrs();
    if (this._attrList && this._attrList.length > 0) return true;
    return !!(this._attrs && this._attrs.size > 0);
  };
  markNative(proto.hasAttributes, "hasAttributes");

  // hasAttributeNS
  proto.hasAttributeNS = function (namespace, localName) {
    return this.getAttributeNS(namespace, localName) !== null;
  };
  markNative(proto.hasAttributeNS, "hasAttributeNS");

  // setAttributeNS
  proto.setAttributeNS = function (namespace, qualifiedName, value) {
    const qname = String(qualifiedName);
    const val = String(value);
    const ns = (namespace === undefined || namespace === null || namespace === "") ? null : String(namespace);
    const colon = qname.indexOf(":");
    const prefix = colon !== -1 ? qname.slice(0, colon) : null;
    const localName = colon !== -1 ? qname.slice(colon + 1) : qname;

    if (!this._attrs) this._syncAttrs();

    let existing = null;
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const item = this._attrList[i];
        const itemNs = item.namespaceURI || null;
        const itemLoc = (item.localName || item.name).toLowerCase();
        if (itemNs === ns && itemLoc === localName.toLowerCase()) {
          existing = item;
          break;
        }
      }
    }

    const oldVal = existing ? existing.value : null;

    if (existing) {
      existing.value = val;
      existing.name = qname;
      existing.prefix = prefix;
    } else {
      const newEntry = {
        name: qname,
        value: val,
        namespaceURI: ns,
        localName: localName,
        prefix: prefix,
      };
      if (!this._attrList) this._attrList = [];
      this._attrList.push(newEntry);
    }

    this._attrs.set(qname.toLowerCase(), {
      name: qname,
      value: val,
      namespaceURI: ns,
      localName: localName,
      prefix: prefix,
    });

    if (qname.toLowerCase() === "async" || (localName && String(localName).toLowerCase() === "async")) {
      if (typeof globalThis !== "undefined" && globalThis.__capsurf_clearScriptForceAsync) {
        globalThis.__capsurf_clearScriptForceAsync(this);
      }
    }

    if (qname.toLowerCase() === "id" && ns === null) {
      this.id = val;
    } else if (qname.toLowerCase() === "class" && ns === null) {
      this.className = val;
    }

    if (origSetAttribute) {
      origSetAttribute.call(this, qname, val);
    }
    if (this._attributesMap) {
      this._attributesMap._sync();
    }
    if (typeof notifyMutation === "function") {
      const rec = new MutationRecord("attributes", this);
      rec.attributeName = qname;
      rec.attributeNamespace = ns;
      rec.oldValue = oldVal;
      notifyMutation(rec);
    }
    if (typeof notifyCustomElementAttributeChanged === "function") {
      notifyCustomElementAttributeChanged(this, localName, oldVal, val, ns);
    }
  };
  markNative(proto.setAttributeNS, "setAttributeNS");

  // removeAttributeNS
  proto.removeAttributeNS = function (namespace, localName) {
    if (typeof localName !== "string" || localName.length === 0) return;
    if (!this._attrs) this._syncAttrs();
    const ns = (namespace === undefined || namespace === null || namespace === "") ? null : String(namespace);
    const loc = String(localName).toLowerCase();

    let removed = null;
    if (this._attrList) {
      for (let i = 0; i < this._attrList.length; i++) {
        const item = this._attrList[i];
        const itemNs = item.namespaceURI || null;
        const itemLoc = (item.localName || item.name).toLowerCase();
        if (itemNs === ns && itemLoc === loc) {
          removed = item;
          this._attrList.splice(i, 1);
          break;
        }
      }
      if (!removed && ns === null) {
        for (let i = 0; i < this._attrList.length; i++) {
          const item = this._attrList[i];
          if ((item.namespaceURI === null || item.namespaceURI === undefined) &&
              item.name.toLowerCase() === loc) {
            removed = item;
            this._attrList.splice(i, 1);
            break;
          }
        }
      }
    }

    const oldVal = removed ? removed.value : null;
    const removedName = removed ? removed.name : localName;

    this._attrs.delete(removedName.toLowerCase());
    this._attrs.delete(localName.toLowerCase());

    if (removedName.toLowerCase() === "id" && ns === null) {
      this.id = "";
    } else if (removedName.toLowerCase() === "class" && ns === null) {
      this.className = "";
    }

    if (loc === "async") {
      if (typeof globalThis !== "undefined" && globalThis.__capsurf_clearScriptForceAsync) {
        globalThis.__capsurf_clearScriptForceAsync(this);
      }
    }

    if (origRemoveAttribute) {
      origRemoveAttribute.call(this, removedName);
    }
    if (this._attributesMap) {
      this._attributesMap._sync();
    }
    if (typeof notifyMutation === "function") {
      const rec = new MutationRecord("attributes", this);
      rec.attributeName = removedName;
      rec.attributeNamespace = ns;
      rec.oldValue = oldVal;
      notifyMutation(rec);
    }
    if (typeof notifyCustomElementAttributeChanged === "function") {
      notifyCustomElementAttributeChanged(this, loc, oldVal, null, ns);
    }
  };
  markNative(proto.removeAttributeNS, "removeAttributeNS");

  // Wrap id & className accessors if present
  const idDesc = Object.getOwnPropertyDescriptor(proto, "id");
  if (idDesc && idDesc.set) {
    const origIdSet = idDesc.set;
    Object.defineProperty(proto, "id", {
      get: idDesc.get,
      set: function (v) {
        origIdSet.call(this, v);
        const str = String(v);
        if (!this._attrs) this._syncAttrs();
        const entry = { name: "id", value: str, namespaceURI: null, localName: "id", prefix: null };
        this._attrs.set("id", entry);
        if (this._attrList) {
          let found = false;
          for (let i = 0; i < this._attrList.length; i++) {
            if (this._attrList[i].name.toLowerCase() === "id") {
              this._attrList[i].value = str;
              found = true;
              break;
            }
          }
          if (!found) this._attrList.push(entry);
        }
        if (this._attributesMap) this._attributesMap._sync();
        if (this.isConnected && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_syncNamedGlobals === "function") {
          globalThis.__capsurf_syncNamedGlobals();
        }
      },
      configurable: true,
    });
  }
  const _pointerCaptures = new WeakMap();
  if (!proto.setPointerCapture) {
    proto.setPointerCapture = markNative(function (pointerId) {
      let set = _pointerCaptures.get(this);
      if (!set) {
        set = new Set();
        _pointerCaptures.set(this, set);
      }
      set.add(Number(pointerId));
    }, "setPointerCapture");
  }
  if (!proto.releasePointerCapture) {
    proto.releasePointerCapture = markNative(function (pointerId) {
      const set = _pointerCaptures.get(this);
      if (set) set.delete(Number(pointerId));
    }, "releasePointerCapture");
  }
  if (!proto.hasPointerCapture) {
    proto.hasPointerCapture = markNative(function (pointerId) {
      const set = _pointerCaptures.get(this);
      return Boolean(set && set.has(Number(pointerId)));
    }, "hasPointerCapture");
  }
  const clsDesc = Object.getOwnPropertyDescriptor(proto, "className");
  const origClsGet = clsDesc ? clsDesc.get : null;
  const origClsSet = clsDesc ? clsDesc.set : null;
  Object.defineProperty(proto, "className", {
    get: function () {
      if (!isHtmlElement(this)) {
        if (!this._svgClassName) {
          const animStr = Object.create(SVGAnimatedString.prototype);
          animStr._element = this;
          this._svgClassName = animStr;
        }
        return this._svgClassName;
      }
      if (origClsGet) return origClsGet.call(this);
      return this.getAttribute("class") || "";
    },
    set: function (v) {
      if (!isHtmlElement(this) && typeof v === "object" && v !== null && v.baseVal !== undefined) {
        v = v.baseVal;
      }
      if (origClsSet) {
        origClsSet.call(this, v);
      }
      const str = String(v);
      if (!this._attrs) this._syncAttrs();
      if (str) {
        const entry = { name: "class", value: str, namespaceURI: null, localName: "class", prefix: null };
        this._attrs.set("class", entry);
        if (this._attrList) {
          let found = false;
          for (let i = 0; i < this._attrList.length; i++) {
            if (this._attrList[i].name.toLowerCase() === "class") {
              this._attrList[i].value = str;
              found = true;
              break;
            }
          }
          if (!found) this._attrList.push(entry);
        }
      } else {
        this._attrs.delete("class");
        if (this._attrList) {
          for (let i = 0; i < this._attrList.length; i++) {
            if (this._attrList[i].name.toLowerCase() === "class") {
              this._attrList.splice(i, 1);
              break;
            }
          }
        }
      }
      if (this._attributesMap) this._attributesMap._sync();
      if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_notifyCssStyleChange === "function") {
        globalThis.__capsurf_notifyCssStyleChange(this);
      }
    },
    configurable: true,
  });

  // toggleAttribute
  if (!proto.toggleAttribute) {
    proto.toggleAttribute = function (name, force) {
      const has = this.hasAttribute(name);
      if (force === true) {
        if (!has) this.setAttribute(name, "");
        return true;
      } else if (force === false) {
        if (has) this.removeAttribute(name);
        return false;
      } else {
        if (has) {
          this.removeAttribute(name);
          return false;
        } else {
          this.setAttribute(name, "");
          return true;
        }
      }
    };
    markNative(proto.toggleAttribute, "toggleAttribute");
  }

  // Directionality helpers (WHATWG HTML §3.2.6.4 + Shadow DOM + Selectors 4 :dir())
  function isStrongRtlCodePoint(cp) {
    return (
      (cp >= 0x0590 && cp <= 0x05FF) || // Hebrew
      (cp >= 0x0600 && cp <= 0x06FF) || // Arabic
      (cp >= 0x0700 && cp <= 0x074F) || // Syriac
      (cp >= 0x0750 && cp <= 0x077F) || // Arabic Supplement
      (cp >= 0x0780 && cp <= 0x07BF) || // Thaana
      (cp >= 0x07C0 && cp <= 0x07FF) || // NKo
      (cp >= 0x0800 && cp <= 0x085F) || // Samaritan / Mandaic
      (cp >= 0x08A0 && cp <= 0x08FF) || // Arabic Extended-A
      (cp >= 0xFB1D && cp <= 0xFDFF) || // Hebrew/Arabic Presentation Forms-A
      (cp >= 0xFE70 && cp <= 0xFEFF) || // Arabic Presentation Forms-B
      (cp >= 0x10800 && cp <= 0x10FFF) || // Ancient RTL scripts
      (cp >= 0x1E800 && cp <= 0x1EFFF) // Adlam / Arabic Math
    );
  }

  function isStrongLtrCodePoint(cp) {
    return (
      (cp >= 0x0041 && cp <= 0x005A) || // A-Z
      (cp >= 0x0061 && cp <= 0x007A) || // a-z
      (cp >= 0x00C0 && cp <= 0x00D6) ||
      (cp >= 0x00D8 && cp <= 0x00F6) ||
      (cp >= 0x00F8 && cp <= 0x02B8) || // Latin Extended
      (cp >= 0x0370 && cp <= 0x058F) || // Greek, Cyrillic, Armenian
      (cp >= 0x0900 && cp <= 0x1FFF) || // Indic, SE Asian, etc.
      (cp >= 0x2C00 && cp <= 0x2DFF) ||
      (cp >= 0x3040 && cp <= 0xA4CF) || // CJK, Hiragana, Katakana
      (cp >= 0xAC00 && cp <= 0xD7AF) // Hangul
    );
  }

  function firstStrongDirectionInText(str) {
    if (!str || typeof str !== "string") return null;
    for (let i = 0; i < str.length; i++) {
      const cp = str.codePointAt(i);
      if (cp > 0xFFFF) i++;
      if (isStrongRtlCodePoint(cp)) return "rtl";
      if (isStrongLtrCodePoint(cp)) return "ltr";
    }
    return null;
  }

  function getElementDirAttr(el) {
    if (!el || el.nodeType !== 1 || typeof el.getAttribute !== "function") return "";
    const raw = el.getAttribute("dir");
    if (!raw) return "";
    const v = String(raw).trim().toLowerCase();
    if (v === "ltr" || v === "rtl" || v === "auto") return v;
    return "";
  }

  function getLightChildren(node) {
    if (!node) return [];
    if (typeof ensureChildren === "function") return ensureChildren(node);
    if (node._children) return node._children;
    if (node.childNodes) return Array.from(node.childNodes);
    return [];
  }

  function findSlotForLightChild(host, child) {
    if (!host || !host._shadowRoot) return null;
    const slotName = (child && child.nodeType === 1 && typeof child.getAttribute === "function")
      ? (child.getAttribute("slot") || "")
      : "";
    function walk(root) {
      const kids = getLightChildren(root);
      for (let i = 0; i < kids.length; i++) {
        const k = kids[i];
        if (!k || k.nodeType !== 1) continue;
        const t = String(k.localName || k.tagName || "").toLowerCase();
        if (t === "slot") {
          const name = (typeof k.getAttribute === "function" ? k.getAttribute("name") : "") || "";
          if (name === slotName) return k;
        }
        const nested = walk(k);
        if (nested) return nested;
      }
      return null;
    }
    return walk(host._shadowRoot);
  }

  function getSlotAssignedNodes(slotEl) {
    let cur = slotEl;
    let shadowRoot = null;
    while (cur) {
      if (cur.nodeType === 11 && cur.host) {
        shadowRoot = cur;
        break;
      }
      cur = cur.parentNode || cur.parentElement;
    }
    if (!shadowRoot || !shadowRoot.host) return [];
    const host = shadowRoot.host;
    const targetName = (typeof slotEl.getAttribute === "function" ? slotEl.getAttribute("name") : "") || "";
    const hostKids = getLightChildren(host);
    const assigned = [];
    for (let i = 0; i < hostKids.length; i++) {
      const c = hostKids[i];
      if (!c) continue;
      if (c.nodeType === 1) {
        const cSlot = (typeof c.getAttribute === "function" ? c.getAttribute("slot") : "") || "";
        if (cSlot === targetName) assigned.push(c);
      } else if (c.nodeType === 3) {
        if (targetName === "") assigned.push(c);
      }
    }
    return assigned;
  }

  function scanSubtreeForFirstStrong(node, isRoot, followSlotAssigned) {
    if (!node) return null;
    if (node.nodeType === 3) {
      const txt = node.data !== undefined ? node.data : (node.textContent || "");
      return firstStrongDirectionInText(txt);
    }
    if (node.nodeType !== 1 && node.nodeType !== 11) return null;
    if (!isRoot && node.nodeType === 1) {
      const tag = String(node.localName || node.tagName || "").toLowerCase();
      if (tag === "bdi" || tag === "script" || tag === "style" || tag === "textarea") {
        return null;
      }
      if (getElementDirAttr(node) !== "") {
        return null;
      }
    }
    if (node.nodeType === 1) {
      const tag = String(node.localName || node.tagName || "").toLowerCase();
      if (tag === "slot" && followSlotAssigned) {
        const assigned = getSlotAssignedNodes(node);
        if (assigned.length > 0) {
          for (let i = 0; i < assigned.length; i++) {
            const d = scanSubtreeForFirstStrong(assigned[i], false, followSlotAssigned);
            if (d) return d;
          }
          return null;
        }
      }
    }
    // IMPORTANT: Never descend into node._shadowRoot (light tree only for dir=auto)
    const kids = getLightChildren(node);
    for (let i = 0; i < kids.length; i++) {
      const d = scanSubtreeForFirstStrong(kids[i], false, followSlotAssigned);
      if (d) return d;
    }
    return null;
  }

  function getNodeDirectionality(el) {
    let cur = el;
    while (cur) {
      if (cur.nodeType === 1) {
        const dirAttr = getElementDirAttr(cur);
        const tag = String(cur.localName || cur.tagName || "").toLowerCase();
        if (dirAttr === "ltr" || dirAttr === "rtl") return dirAttr;
        if (tag === "input") {
          const inType = String(cur.getAttribute("type") || cur.type || "text").trim().toLowerCase();
          if (dirAttr === "auto") {
            if (inType === "text" || inType === "search" || inType === "tel" || inType === "url" || inType === "email" || inType === "password" || inType === "submit" || inType === "reset" || inType === "button" || inType === "hidden") {
              return firstStrongDirectionInText(String(cur.value || "")) || "ltr";
            }
            return "ltr";
          }
          if (inType === "tel") return "ltr";
        }
        if (tag === "textarea" && dirAttr === "auto") {
          const tv = cur._valueDirty ? String(cur.value || "") : String(cur.value || cur.textContent || "");
          return firstStrongDirectionInText(tv) || "ltr";
        }
        if (dirAttr === "auto" || (tag === "bdi" && dirAttr === "")) {
          const found = scanSubtreeForFirstStrong(cur, true, tag === "slot");
          return found || "ltr";
        }
      }
      if (cur.parentNode && cur.parentNode.nodeType === 11 && cur.parentNode.host) {
        cur = cur.parentNode.host;
        continue;
      }
      cur = cur.parentElement || cur.parentNode;
    }
    return "ltr";
  }

  function getFlatTreeParent(el) {
    if (!el) return null;
    const p = el.parentNode || el.parentElement;
    if (!p) return null;
    if (p.nodeType === 1 && p._shadowRoot) {
      const slot = findSlotForLightChild(p, el);
      if (slot) return slot;
    }
    if (p.nodeType === 11 && p.host) {
      return p.host;
    }
    return p.nodeType === 1 ? p : null;
  }

  function getComputedDirection(el) {
    let cur = el;
    while (cur && cur.nodeType === 1) {
      // 1. Explicit inline style direction
      const inlineDir = (cur.style && typeof cur.style.getPropertyValue === "function")
        ? String(cur.style.getPropertyValue("direction") || "").trim().toLowerCase()
        : "";
      if (inlineDir === "ltr" || inlineDir === "rtl") return inlineDir;

      // 2. HTML dir attribute or <bdi>
      const dirAttr = getElementDirAttr(cur);
      const tag = String(cur.localName || cur.tagName || "").toLowerCase();
      if (dirAttr === "ltr" || dirAttr === "rtl") return dirAttr;
      if (tag === "input") {
        const inType = String(cur.getAttribute("type") || cur.type || "text").trim().toLowerCase();
        if (dirAttr === "auto") {
          if (inType === "text" || inType === "search" || inType === "tel" || inType === "url" || inType === "email" || inType === "password") {
            return firstStrongDirectionInText(String(cur.value || "")) || "ltr";
          }
          return "ltr";
        }
        if (inType === "tel") return "ltr";
      }
      if (dirAttr === "auto" || (tag === "bdi" && dirAttr === "")) {
        const found = scanSubtreeForFirstStrong(cur, true, tag === "slot");
        if (found) return found;
        if (tag !== "slot") return "ltr";
      }
      cur = getFlatTreeParent(cur);
    }
    return "ltr";
  }

  global.__capsurf_getNodeDirectionality = getNodeDirectionality;
  global.__capsurf_getComputedDirection = getComputedDirection;

  if (!Object.getOwnPropertyDescriptor(proto, "dir")) {
    Object.defineProperty(proto, "dir", {
      get: function () { return getElementDirAttr(this); },
      set: function (v) { this.setAttribute("dir", v == null ? "" : String(v)); },
      configurable: true,
    });
  }

  // Fallback firstChild / lastChild to _children when native DOM lacks JS-only children (e.g. CDATASection/PI)
  const origElFirstChildDesc = Object.getOwnPropertyDescriptor(proto, "firstChild") ||
    (global.Node && global.Node.prototype ? Object.getOwnPropertyDescriptor(global.Node.prototype, "firstChild") : null);
  const origElLastChildDesc = Object.getOwnPropertyDescriptor(proto, "lastChild") ||
    (global.Node && global.Node.prototype ? Object.getOwnPropertyDescriptor(global.Node.prototype, "lastChild") : null);
  if (origElFirstChildDesc && typeof origElFirstChildDesc.get === "function") {
    const origGet = origElFirstChildDesc.get;
    Object.defineProperty(proto, "firstChild", {
      get: function () {
        let out = origGet.call(this);
        if (!out && this._children && this._children.length > 0) out = this._children[0];
        return (out && out.nodeType === 1 && typeof upgradeElement === "function") ? upgradeElement(out) : out;
      },
      configurable: true,
    });
  }
  if (origElLastChildDesc && typeof origElLastChildDesc.get === "function") {
    const origGet = origElLastChildDesc.get;
    Object.defineProperty(proto, "lastChild", {
      get: function () {
        let res = origGet.call(this);
        if (this._children && this._children.length > 0 && !(res && (!this._children || this._children.length === 0))) {
          res = this._children[this._children.length - 1];
        }
        return (res && res.nodeType === 1 && typeof upgradeElement === "function") ? upgradeElement(res) : res;
      },
      configurable: true,
    });
  }

  // matches & closest
  const origProtoMatches = typeof proto.matches === "function" ? proto.matches : null;
  proto.matches = function (selector) {
    if (arguments.length === 0) {
      throw new TypeError("Failed to execute 'matches' on 'Element': 1 argument required, but only 0 present.");
    }
    if (typeof validateSelector === "function") {
      selector = validateSelector(selector);
    }
    const s = String(selector).trim();
    const dirCheckRe = /:dir\(([^)]*)\)/gi;
    let dcm;
    while ((dcm = dirCheckRe.exec(s)) !== null) {
      if (!/^\s*[a-zA-Z_-][a-zA-Z0-9_-]*\s*$/.test(dcm[1])) {
        throw new DOMException("The provided selector is invalid.", "SyntaxError");
      }
    }
    const dirMatch = s.match(/^:dir\(\s*([a-zA-Z0-9_-]+)\s*\)$/i);
    if (dirMatch) {
      const targetDir = dirMatch[1].toLowerCase();
      if (targetDir !== "ltr" && targetDir !== "rtl") {
        return false;
      }
      return getNodeDirectionality(this) === targetDir;
    }
    if (/::slotted\(/i.test(s) && s.indexOf(":not(") === -1) return false;
    if (origProtoMatches && s.indexOf("&") === -1) {
      try {
        if (origProtoMatches.call(this, selector)) return true;
        const rootNode = typeof this.getRootNode === "function" ? this.getRootNode() : null;
        if (!(rootNode && rootNode.host) &&
            !/:(?:is|where|has|not|scope|first-child|last-child|only-child|first-of-type|last-of-type|only-of-type|nth-)/i.test(s)) {
          return false;
        }
      } catch (e) {
        if (e && e.name === "SyntaxError") throw e;
      }
    }
    if (s.indexOf("&") !== -1 && typeof matchesElementSelector === "function") {
      const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
      const sr = doc ? doc.documentElement : null;
      return matchesElementSelector(this, s, sr, true);
    }
    if (s === ":checked") return Boolean(this.checked || (String(this.tagName || "").toUpperCase() === "OPTION" && this.selected));
    if (s === ":disabled") return Boolean(this.disabled);
    if (s === ":enabled") return !this.disabled;
    if (s === ":focus") return Boolean(typeof document !== "undefined" && (document._activeElement === this || document.activeElement === this));
    if (s.endsWith(":focus")) {
      const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
      if (!doc || (doc._activeElement !== this && doc.activeElement !== this)) return false;
      const base = s.slice(0, -6).trim();
      if (!base || base === "*") return true;
      return this.matches(base);
    }
    if (s === "*") return true;
    if (s.startsWith("#") && this.id === s.slice(1)) return true;
    if (s.startsWith(".") && this.classList.contains(s.slice(1))) return true;
    if (s.toLowerCase() === this.localName) return true;
    const root = typeof this.getRootNode === "function" ? this.getRootNode() : (this.ownerDocument || (typeof document !== "undefined" ? document : null));
    if (typeof matchesElementSelector === "function") {
      try {
        const scopeEl = (root && root.nodeType === 9) ? root.documentElement : root;
        return matchesElementSelector(this, s, scopeEl, true);
      } catch (e) {
        if (e && e.name === "SyntaxError") throw e;
      }
    }
    if (root && typeof root.querySelectorAll === "function") {
      try {
        const matches = root.querySelectorAll(s);
        for (let i = 0; i < matches.length; i++) {
          if (matches[i] === this) return true;
        }
      } catch (e) {
        if (e && e.name === "SyntaxError") throw e;
      }
    }
    return false;
  };
  markNative(proto.matches, "matches");

  if (!proto.closest) {
    proto.closest = function (selector) {
      if (arguments.length === 0) {
        throw new TypeError("Failed to execute 'closest' on 'Element': 1 argument required, but only 0 present.");
      }
      if (typeof validateSelector === "function") {
        validateSelector(selector);
      }
      let curr = this;
      while (curr && curr.nodeType === ELEMENT_NODE) {
        if (curr.matches && curr.matches(selector)) return curr;
        curr = curr.parentElement;
      }
      return null;
    };
    markNative(proto.closest, "closest");
  }

  // getClientRects
  if (!proto.getClientRects) {
    proto.getClientRects = function () {
      if (isElementDisplayNone(this)) {
        if (typeof global.__capsurf_createDOMRectList === "function") {
          return global.__capsurf_createDOMRectList([]);
        }
        return [];
      }
      const rect = this.getBoundingClientRect();
      if (typeof global.__capsurf_createDOMRectList === "function") {
        return global.__capsurf_createDOMRectList([rect]);
      }
      return [rect];
    };
    markNative(proto.getClientRects, "getClientRects");
  }

  // Scroll and Box Geometry properties
  function triggerScrollEvents(el) {
    if (!el) return;
    const Ev = (typeof Event !== "undefined")
      ? Event
      : ((typeof global !== "undefined" && global.Event) || (typeof globalThis !== "undefined" && globalThis.Event) || null);
    if (!Ev) return;

    const scrollEv = new Ev("scroll", { bubbles: false, cancelable: false });
    scrollEv._isTrusted = true;
    if (typeof dispatchTrustedEvent === "function") {
      dispatchTrustedEvent(el, scrollEv);
    } else if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
      global.__capsurf_dispatchTrustedEvent(el, scrollEv);
    } else if (typeof el.dispatchEvent === "function") {
      scrollEv._isTrusted = true;
      el.dispatchEvent(scrollEv);
    }

    if (el._scrollendTimer) {
      clearTimeout(el._scrollendTimer);
    }
    el._scrollendTimer = setTimeout(function () {
      el._scrollendTimer = null;
      const endEv = new Ev("scrollend", { bubbles: false, cancelable: false });
      endEv._isTrusted = true;
      if (typeof dispatchTrustedEvent === "function") {
        dispatchTrustedEvent(el, endEv);
      } else if (typeof global.__capsurf_dispatchTrustedEvent === "function") {
        global.__capsurf_dispatchTrustedEvent(el, endEv);
      } else if (typeof el.dispatchEvent === "function") {
        endEv._isTrusted = true;
        el.dispatchEvent(endEv);
      }
    }, 20);
  }

  Object.defineProperty(proto, "scrollTop", {
    get: function () {
      if (this === document.documentElement) {
        return (typeof window !== "undefined" && (window.scrollY || window.pageYOffset)) || this._scrollTop || 0;
      }
      return this._scrollTop || 0;
    },
    set: function (v) {
      const num = Number(v) || 0;
      let target = Math.max(0, num);
      const sh = this.scrollHeight;
      const ch = this.clientHeight;
      if (ch > 0) {
        target = Math.min(target, Math.max(0, sh - ch));
      }
      const prev = this._scrollTop || 0;
      this._scrollTop = target;
      if (this === document.documentElement && typeof window !== "undefined" && typeof window.scrollTo === "function") {
        window.scrollTo(window.scrollX || 0, target);
        return;
      }
      if (prev !== target) {
        triggerScrollEvents(this);
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "scrollLeft", {
    get: function () {
      if (this === document.documentElement) {
        return (typeof window !== "undefined" && (window.scrollX || window.pageXOffset)) || this._scrollLeft || 0;
      }
      return this._scrollLeft || 0;
    },
    set: function (v) {
      const num = Number(v) || 0;
      let target = Math.max(0, num);
      const sw = this.scrollWidth;
      const cw = this.clientWidth;
      if (cw > 0) {
        target = Math.min(target, Math.max(0, sw - cw));
      }
      const prev = this._scrollLeft || 0;
      this._scrollLeft = target;
      if (this === document.documentElement && typeof window !== "undefined" && typeof window.scrollTo === "function") {
        window.scrollTo(target, window.scrollY || 0);
        return;
      }
      if (prev !== target) {
        triggerScrollEvents(this);
      }
    },
    configurable: true,
  });

  proto.scrollTo = function (xOrOptions, y) {
    let targetX = this.scrollLeft;
    let targetY = this.scrollTop;
    if (typeof xOrOptions === "object" && xOrOptions !== null) {
      if (xOrOptions.left !== undefined) targetX = Number(xOrOptions.left) || 0;
      if (xOrOptions.top !== undefined) targetY = Number(xOrOptions.top) || 0;
    } else if (arguments.length >= 2) {
      targetX = Number(xOrOptions) || 0;
      targetY = Number(y) || 0;
    }
    this.scrollLeft = targetX;
    this.scrollTop = targetY;
  };
  proto.scroll = proto.scrollTo;
  proto.scrollBy = function (xOrOptions, y) {
    let dx = 0;
    let dy = 0;
    if (typeof xOrOptions === "object" && xOrOptions !== null) {
      if (xOrOptions.left !== undefined) dx = Number(xOrOptions.left) || 0;
      if (xOrOptions.top !== undefined) dy = Number(xOrOptions.top) || 0;
    } else if (arguments.length >= 2) {
      dx = Number(xOrOptions) || 0;
      dy = Number(y) || 0;
    }
    this.scrollLeft = (this.scrollLeft || 0) + dx;
    this.scrollTop = (this.scrollTop || 0) + dy;
  };

  function getScrollParent(node) {
    if (!node) return null;
    if (node.nodeType === 1 && node.parentNode && node.parentNode._shadowRoot) {
      if (node.assignedSlot) return node.assignedSlot;
      return null;
    }
    if (node.parentElement) return node.parentElement;
    if (node.parentNode) {
      if (node.parentNode.nodeType === 11 && node.parentNode.host) { // ShadowRoot
        return node.parentNode.host;
      }
      if (node.parentNode.nodeType === 1) {
        return node.parentNode;
      }
      if (node.parentNode.host) {
        return node.parentNode.host;
      }
    }
    if (node.host) return node.host;
    return null;
  }

  function isHiddenInClosedDetails(node) {
    let child = node;
    let p = child ? (child.parentNode || child.parentElement) : null;
    while (p && p.nodeType === 1) {
      const ptag = String(p.localName || p.tagName || "").toLowerCase();
      if (ptag === "details" && !(p.hasAttribute && p.hasAttribute("open"))) {
        let firstSummary = null;
        const kids = p.childNodes || p._children || [];
        for (let i = 0; i < kids.length; i++) {
          const k = kids[i];
          if (k && k.nodeType === 1 && String(k.localName || k.tagName || "").toLowerCase() === "summary") {
            firstSummary = k;
            break;
          }
        }
        if (child !== firstSummary) return true;
      }
      child = p;
      p = p.parentNode || p.parentElement;
    }
    return false;
  }

  function docHasCssSheets() {
    if (typeof document === "undefined" || !document || typeof document.querySelectorAll !== "function") return false;
    return document.querySelectorAll("style, link").length > 0;
  }

  function isElementInlineOrJsDisplayNone(el) {
    if (!el || el.nodeType !== 1) return false;
    if (isHiddenInClosedDetails(el)) return true;
    let cur = el;
    while (cur && cur.nodeType === 1) {
      if (cur.parentNode && cur.parentNode._shadowRoot && !cur.assignedSlot) {
        return true;
      }
      if (cur._displayNone === true) return true;
      if (cur.style) {
        const d = (typeof cur.style.getPropertyValue === "function")
          ? cur.style.getPropertyValue("display")
          : cur.style.display;
        if (d === "none") return true;
      }
      const rawStyle = (typeof cur.getAttribute === "function") ? cur.getAttribute("style") : "";
      if (rawStyle && /(?:^|;)\s*display\s*:\s*none\b/i.test(rawStyle)) {
        return true;
      }
      if (cur._computedStyle && cur._computedStyle.display === "none") {
        return true;
      }
      cur = getScrollParent(cur);
    }
    return false;
  }

  function isElementDisplayNone(el) {
    if (!el || el.nodeType !== 1) return false;
    if (isElementInlineOrJsDisplayNone(el)) return true;
    const hasSheets = docHasCssSheets();
    const hasGcs = hasSheets && typeof window !== "undefined" && typeof window.getComputedStyle === "function";
    if (!hasGcs) return false;
    let cur = el;
    while (cur && cur.nodeType === 1) {
      try {
        const cs = window.getComputedStyle(cur);
        if (cs) {
          const cd = (typeof cs.getPropertyValue === "function") ? cs.getPropertyValue("display") : cs.display;
          if (cd === "none") return true;
        }
      } catch (e) {}
      cur = getScrollParent(cur);
    }
    return false;
  }

  function makeDomRect(x, y, width, height) {
    if (typeof DOMRect === "function") {
      return new DOMRect(x, y, width, height);
    }
    const nx = x || 0, ny = y || 0, nw = width || 0, nh = height || 0;
    const obj = {
      x: nx,
      y: ny,
      width: nw,
      height: nh,
      top: ny,
      right: nx + nw,
      bottom: ny + nh,
      left: nx,
    };
    obj.toJSON = function () {
      return { x: this.x, y: this.y, width: this.width, height: this.height, top: this.top, right: this.right, bottom: this.bottom, left: this.left };
    };
    return obj;
  }

  let inSiblingOffsetCalc = false;
  function getShadowEffectiveText(el) {
    if (!el) return "";
    let txt = String(el.textContent || "").trim();
    if (txt.length > 0) return txt;
    const t = String(el.localName || el.tagName || "").toUpperCase();
    const collectSlot = function (sl) {
      if (!sl || typeof sl.assignedNodes !== "function") return "";
      const nodes = sl.assignedNodes({ flatten: true });
      let s = "";
      for (let i = 0; i < nodes.length; i++) {
        if (nodes[i] && nodes[i].textContent) s += nodes[i].textContent;
      }
      return s.trim();
    };
    if (t === "SLOT") return collectSlot(el);
    if (typeof el.querySelectorAll === "function") {
      const slots = el.querySelectorAll("slot");
      for (let i = 0; i < slots.length; i++) {
        txt = collectSlot(slots[i]);
        if (txt.length > 0) return txt;
      }
    }
    return "";
  }

  const origGetBoundingClientRect = proto.getBoundingClientRect;
  proto.getBoundingClientRect = function () {
    const fastCheck = (typeof this._nid === "number" && this._nid > 0 && origGetBoundingClientRect)
      ? isElementInlineOrJsDisplayNone(this)
      : isElementDisplayNone(this);
    if (fastCheck) {
      return makeDomRect(0, 0, 0, 0);
    }
    const r = origGetBoundingClientRect ? origGetBoundingClientRect.call(this) : { x: 0, y: 0, width: 0, height: 0, left: 0, top: 0, right: 0, bottom: 0 };
    if (this._shadowRoot) {
      if (this._inShadowRect) {
        const hw = r.width || 800;
        const hx = r.x || 0;
        const hy = r.y || 0;
        return makeDomRect(hx, hy, hw, 0);
      }
      let explicitH = 0;
      if (this.style && this.style.height) explicitH = parseFloat(this.style.height) || 0;
      if (!explicitH && typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
        try {
          const hcs = window.getComputedStyle(this);
          if (hcs && hcs.height && hcs.height !== "auto") explicitH = parseFloat(hcs.height) || 0;
        } catch (e) {}
      }
      if (!explicitH) {
        this._inShadowRect = true;
        let shadowH = 0;
        try {
          const sKids = Array.from(this._shadowRoot.childNodes || this._shadowRoot._children || []);
          for (let i = 0; i < sKids.length; i++) {
            const sk = sKids[i];
            if (sk && sk.nodeType === 1) {
              const stag = String(sk.localName || sk.tagName || "").toLowerCase();
              if (stag === "style" || stag === "script") continue;
              const sr = typeof sk.getBoundingClientRect === "function" ? sk.getBoundingClientRect() : null;
              if (sr && sr.height > 0) {
                shadowH += sr.height;
              } else if (getShadowEffectiveText(sk).length > 0) {
                shadowH += 16;
              }
            }
          }
        } finally {
          this._inShadowRect = false;
        }
        const hostW = r.width || 800;
        const hostX = r.x || 0;
        const hostY = r.y || 0;
        return makeDomRect(hostX, hostY, hostW, shadowH);
      }
    }
    if (r.width === 0 && r.height === 0 && r.x === 0 && r.y === 0) {
      const tag = String(this.tagName || this.localName || "").toUpperCase();
      if (tag === "IMG") {
        const nw = Number(this.naturalWidth || 0);
        const nh = Number(this.naturalHeight || 0);
        if (nw > 0 || nh > 0) {
          return makeDomRect(0, 0, nw, nh);
        }
      }
      let childW = 0, childH = 0;
      if (this.style) {
        if (this.style.width) childW = parseFloat(this.style.width) || 0;
        if (this.style.height) childH = parseFloat(this.style.height) || 0;
      }
      if ((!childW || !childH) && (docHasCssSheets() || (this.getAttribute && this.getAttribute("style"))) && typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
        try {
          const cs = window.getComputedStyle(this);
          if (cs) {
            if (!childW && cs.width && cs.width !== "auto") childW = parseFloat(cs.width) || 0;
            if (!childH && cs.height && cs.height !== "auto") childH = parseFloat(cs.height) || 0;
          }
        } catch (e) {}
      }
      const parent = getScrollParent(this);
      if (childW === 0 && childH === 0 && parent && parent.nodeType === 1) {
        const rn = typeof this.getRootNode === "function" ? this.getRootNode() : null;
        if (rn && rn.host) {
          const txt = getShadowEffectiveText(this);
          if (txt.length > 0 || tag === "BUTTON" || tag === "INPUT" || tag === "SELECT" || tag === "TEXTAREA") {
            childW = Math.max(20, txt.length * 8 + 16);
            childH = 24;
          }
        }
      }
      if (parent && parent.nodeType === 1 && (childW > 0 || childH > 0)) {
        const winX = (typeof window !== "undefined" && (window.scrollX || window.pageXOffset)) || 0;
        const winY = (typeof window !== "undefined" && (window.scrollY || window.pageYOffset)) || 0;
        const pos = getComputedPosition(this);
        if (pos === "absolute" || pos === "fixed") {
          let absLeft = 0;
          let absTop = 0;
          if (this.style && this.style.left) absLeft = parseFloat(this.style.left) || 0;
          if (this.style && this.style.top) absTop = parseFloat(this.style.top) || 0;
          if (typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
            try {
              const cs = window.getComputedStyle(this);
              if (cs) {
                if (!this.style?.left && cs.left && cs.left !== "auto") absLeft = parseFloat(cs.left) || 0;
                if (!this.style?.top && cs.top && cs.top !== "auto") absTop = parseFloat(cs.top) || 0;
              }
            } catch (e) {}
          }
          if (pos === "fixed") {
            return makeDomRect(absLeft, absTop, childW, childH);
          }
          return makeDomRect(absLeft - winX, absTop - winY, childW, childH);
        }
        let pr;
        if (parent._shadowRoot && !parent._inShadowRect) {
          parent._inShadowRect = true;
          try {
            pr = parent.getBoundingClientRect();
          } finally {
            parent._inShadowRect = false;
          }
        } else {
          pr = parent.getBoundingClientRect();
        }
        if (!childW) childW = pr.width;
        let offsetTop = 0;
        if (!inSiblingOffsetCalc) {
          const siblings = (this.parentNode && (this.parentNode.children || this.parentNode._children || this.parentNode.childNodes)) ||
                           (parent.children || parent._children || []);
          inSiblingOffsetCalc = true;
          try {
            for (let i = 0; i < siblings.length; i++) {
              const sib = siblings[i];
              if (sib === this) break;
              if (sib && sib.nodeType === 1) {
                let sh = (sib.getBoundingClientRect ? sib.getBoundingClientRect().height : 0) || 0;
                if (!sh && sib.style && sib.style.height) sh = parseFloat(sib.style.height) || 0;
                offsetTop += sh;
              }
            }
          } finally {
            inSiblingOffsetCalc = false;
          }
        }
        const relX = pr.left;
        const relY = pr.top + offsetTop - (parent._scrollTop || 0);
        return makeDomRect(relX, relY, childW, childH);
      }
      return makeDomRect(0, 0, 0, 0);
    }
    const winX = (typeof window !== "undefined" && (window.scrollX || window.pageXOffset)) || 0;
    const winY = (typeof window !== "undefined" && (window.scrollY || window.pageYOffset)) || 0;
    let scrollOffsetParentX = 0;
    let scrollOffsetParentY = 0;
    let cur = getScrollParent(this);
    while (cur && cur !== document.body && cur !== document.documentElement && cur !== document) {
      if (cur._scrollTop) scrollOffsetParentY += cur._scrollTop;
      if (cur._scrollLeft) scrollOffsetParentX += cur._scrollLeft;
      cur = getScrollParent(cur);
    }
    const x = r.x - winX - scrollOffsetParentX;
    const y = r.y - winY - scrollOffsetParentY;
    const w = r.width;
    const h = r.height;
    return makeDomRect(x, y, w, h);
  };
  markNative(proto.getBoundingClientRect, "getBoundingClientRect");

  function getBorderWidthPx(el, side) {
    if (!el || el.nodeType !== 1 || isElementDisplayNone(el)) return 0;
    if (typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
      try {
        const cs = window.getComputedStyle(el);
        if (cs) {
          const bStyle = String(cs.getPropertyValue("border-" + side + "-style") || "").trim().toLowerCase();
          if (bStyle === "none" || bStyle === "hidden") return 0;
          const bw = String(cs.getPropertyValue("border-" + side + "-width") || "").trim().toLowerCase();
          if (bw === "thin") return 1;
          if (bw === "medium") return 3;
          if (bw === "thick") return 5;
          const n = parseFloat(bw);
          if (!isNaN(n) && n > 0) return n;
        }
      } catch (e) {}
    }
    return 0;
  }

  function getComputedPosition(el) {
    if (!el || el.nodeType !== 1) return "static";
    if (el.style) {
      const p = (typeof el.style.getPropertyValue === "function")
        ? el.style.getPropertyValue("position")
        : el.style.position;
      if (p) return String(p).trim().toLowerCase();
    }
    if (typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
      try {
        const cs = window.getComputedStyle(el);
        if (cs) {
          const cp = (typeof cs.getPropertyValue === "function")
            ? cs.getPropertyValue("position")
            : cs.position;
          if (cp) return String(cp).trim().toLowerCase();
        }
      } catch (e) {}
    }
    return "static";
  }

  function isOverflowClipped(el) {
    if (!el || el.nodeType !== 1) return false;
    if (typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
      try {
        const cs = window.getComputedStyle(el);
        if (cs) {
          const ox = String(cs.getPropertyValue("overflow-x") || cs.getPropertyValue("overflow") || "").trim().toLowerCase();
          const oy = String(cs.getPropertyValue("overflow-y") || cs.getPropertyValue("overflow") || "").trim().toLowerCase();
          if (ox === "hidden" || ox === "clip" || ox === "scroll" || ox === "auto" ||
              oy === "hidden" || oy === "clip" || oy === "scroll" || oy === "auto") {
            return true;
          }
        }
      } catch (e) {}
    }
    return false;
  }

  Object.defineProperty(proto, "offsetWidth", {
    get: function () {
      if (isElementInlineOrJsDisplayNone(this)) return 0;
      let r = (typeof this._nid === "number" && this._nid > 0 && origGetBoundingClientRect) ? origGetBoundingClientRect.call(this) : null;
      if (!r || (r.width === 0 && r.height === 0)) r = this.getBoundingClientRect();
      return Math.round((r && r.width) || 0);
    },
    configurable: true,
  });

  Object.defineProperty(proto, "offsetHeight", {
    get: function () {
      if (isElementInlineOrJsDisplayNone(this)) return 0;
      let r = (typeof this._nid === "number" && this._nid > 0 && origGetBoundingClientRect) ? origGetBoundingClientRect.call(this) : null;
      if (!r || (r.width === 0 && r.height === 0)) r = this.getBoundingClientRect();
      return Math.round((r && r.height) || 0);
    },
    configurable: true,
  });

  Object.defineProperty(proto, "clientTop", {
    get: function () {
      return Math.round(getBorderWidthPx(this, "top"));
    },
    configurable: true,
  });

  Object.defineProperty(proto, "clientLeft", {
    get: function () {
      return Math.round(getBorderWidthPx(this, "left"));
    },
    configurable: true,
  });

  Object.defineProperty(proto, "clientHeight", {
    get: function () {
      if (this === document.documentElement || this === document.body) {
        return (typeof window !== "undefined" && window.innerHeight) || 720;
      }
      if (isElementDisplayNone(this)) return 0;
      const r = origGetBoundingClientRect ? origGetBoundingClientRect.call(this) : this.getBoundingClientRect();
      const bt = getBorderWidthPx(this, "top");
      const bb = getBorderWidthPx(this, "bottom");
      return Math.max(0, Math.round((r.height || 0) - bt - bb));
    },
    configurable: true,
  });

  Object.defineProperty(proto, "clientWidth", {
    get: function () {
      if (this === document.documentElement || this === document.body) {
        return (typeof window !== "undefined" && window.innerWidth) || 1280;
      }
      if (isElementDisplayNone(this)) return 0;
      const r = origGetBoundingClientRect ? origGetBoundingClientRect.call(this) : this.getBoundingClientRect();
      const bl = getBorderWidthPx(this, "left");
      const br = getBorderWidthPx(this, "right");
      return Math.max(0, Math.round((r.width || 0) - bl - br));
    },
    configurable: true,
  });

  function getDocRectForScroll(el, checkCss) {
    if (!el || el.nodeType !== 1) return null;
    const r = origGetBoundingClientRect ? origGetBoundingClientRect.call(el) : (el.getBoundingClientRect ? el.getBoundingClientRect() : null);
    if (r && (r.width > 0 || r.height > 0 || r.x !== 0 || r.y !== 0)) {
      const rx = Number(r.x || 0);
      const ry = Number(r.y || 0);
      const rw = Number(r.width || 0);
      const rh = Number(r.height || 0);
      return { x: rx, y: ry, width: rw, height: rh, right: rx + rw, bottom: ry + rh };
    }
    let w = 0, h = 0, x = 0, y = 0;
    if (el.style) {
      if (el.style.width) w = parseFloat(el.style.width) || 0;
      if (el.style.height) h = parseFloat(el.style.height) || 0;
      if (el.style.left) x = parseFloat(el.style.left) || 0;
      if (el.style.top) y = parseFloat(el.style.top) || 0;
    }
    if ((checkCss || (el.getAttribute && el.getAttribute("style"))) && typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
      try {
        const cs = window.getComputedStyle(el);
        if (cs) {
          if (!w && cs.width && cs.width !== "auto") w = parseFloat(cs.width) || 0;
          if (!h && cs.height && cs.height !== "auto") h = parseFloat(cs.height) || 0;
          if (!el.style?.left && cs.left && cs.left !== "auto") x = parseFloat(cs.left) || 0;
          if (!el.style?.top && cs.top && cs.top !== "auto") y = parseFloat(cs.top) || 0;
        }
      } catch (e) {}
    }
    if (w > 0 || h > 0) {
      return { x: x, y: y, width: w, height: h, right: x + w, bottom: y + h };
    }
    return null;
  }

  function collectRootScrollExtents(rootEl, isDocEl) {
    const vw = (typeof window !== "undefined" && window.innerWidth) || 1280;
    const vh = (typeof window !== "undefined" && window.innerHeight) || 720;

    let originX = 0;
    let originY = 0;
    let maxW = vw;
    let maxH = vh;

    const hasDocSheets = typeof document !== "undefined" && document &&
      typeof document.querySelectorAll === "function" &&
      document.querySelectorAll("style, link").length > 0;
    let hasFixedOrOverflowCss = false;
    if (hasDocSheets) {
      const links = document.querySelectorAll("link");
      for (let li = 0; li < links.length; li++) {
        const rel = String((links[li].getAttribute && links[li].getAttribute("rel")) || "").toLowerCase();
        if (rel.indexOf("stylesheet") !== -1) { hasFixedOrOverflowCss = true; break; }
      }
      if (!hasFixedOrOverflowCss) {
        const sts = document.querySelectorAll("style");
        for (let si = 0; si < sts.length; si++) {
          const txt = String(sts[si].textContent || "").toLowerCase();
          if (txt.indexOf("fixed") !== -1 || txt.indexOf("overflow") !== -1) {
            hasFixedOrOverflowCss = true;
            break;
          }
        }
      }
    }

    const rr = getDocRectForScroll(rootEl, hasDocSheets);
    if (rr) {
      originX = rr.x;
      originY = rr.y;
      if (!isDocEl) {
        if (rr.width > 0) maxW = Math.round(rr.width);
        if (rr.height > 0) maxH = Math.round(rr.height);
      }
    }
    if (!isDocEl && maxW < vw && originX === 0) maxW = vw;

    if (rootEl.style && rootEl.style.height) {
      const ph = parseFloat(rootEl.style.height);
      if (!isNaN(ph) && ph > maxH) maxH = ph;
    }
    if (rootEl.style && rootEl.style.width) {
      const pw = parseFloat(rootEl.style.width);
      if (!isNaN(pw) && pw > maxW) maxW = pw;
    }

    const gcsFn = (typeof window !== "undefined" && typeof window.getComputedStyle === "function")
      ? window.getComputedStyle
      : null;

    function walk(parent) {
      const kids = parent.children || [];
      for (let i = 0; i < kids.length; i++) {
        const child = kids[i];
        if (!child || child.nodeType !== 1) continue;
        const tag = String(child.localName || child.tagName || "").toLowerCase();
        if (tag === "head" || tag === "style" || tag === "script" || tag === "meta" || tag === "link" || tag === "title" || tag === "template") {
          continue;
        }
        if (isElementInlineOrJsDisplayNone(child)) continue;
        let isFixed = false;
        let clipped = false;
        const inlineStyle = (child.getAttribute && child.getAttribute("style")) || "";
        if (hasFixedOrOverflowCss || inlineStyle) {
          if (gcsFn) {
            try {
              const cs = gcsFn(child);
              if (cs) {
                const pos = String(cs.getPropertyValue("position") || cs.position || "").trim().toLowerCase();
                if (pos === "fixed") isFixed = true;
                const ox = String(cs.getPropertyValue("overflow-x") || cs.getPropertyValue("overflow") || "").trim().toLowerCase();
                const oy = String(cs.getPropertyValue("overflow-y") || cs.getPropertyValue("overflow") || "").trim().toLowerCase();
                if (ox === "hidden" || ox === "clip" || ox === "scroll" || ox === "auto" ||
                    oy === "hidden" || oy === "clip" || oy === "scroll" || oy === "auto") {
                  clipped = true;
                }
              }
            } catch (e) {}
          }
        }
        if (isFixed) continue;
        const cr = (tag === "br" || tag === "wbr") ? null : getDocRectForScroll(child, hasFixedOrOverflowCss);
        if (cr) {
          const relRight = cr.right - originX;
          const relBottom = cr.bottom - originY;
          if (relRight > maxW) maxW = relRight;
          if (relBottom > maxH) maxH = relBottom;
        }
        if (!clipped) {
          walk(child);
        }
      }
    }

    walk(rootEl);
    return { width: Math.round(maxW), height: Math.round(maxH) };
  }

  Object.defineProperty(proto, "scrollHeight", {
    get: function () {
      if (this === document.documentElement || this === document.body) {
        return collectRootScrollExtents(this, this === document.documentElement).height;
      }

      let maxBottom = this.clientHeight;
      const parentR = this.getBoundingClientRect();
      const shadow = this._shadowRoot || this.shadowRoot;
      const shadowKids = shadow ? (shadow.children || shadow._children || shadow.childNodes || []) : [];
      const all = (shadowKids.length > 0) ? shadowKids : (this.children || []);
      let accumY = 0;
      for (let i = 0; i < all.length; i++) {
        const child = all[i];
        if (!child || child.nodeType !== 1) continue;
        if (isElementDisplayNone(child)) continue;
        const cr = child.getBoundingClientRect ? child.getBoundingClientRect() : null;
        let childH = (cr && cr.height) || 0;
        if (!childH && child.style && child.style.height) {
          childH = parseFloat(child.style.height) || 0;
        }
        let relY = (parentR && cr && cr.height > 0) ? (cr.y - parentR.y - (this.clientTop || 0) + (this._scrollTop || 0)) : accumY;
        accumY += childH;
        const b = relY + childH;
        if (b > maxBottom) maxBottom = b;
      }
      return Math.round(maxBottom);
    },
    configurable: true,
  });

  Object.defineProperty(proto, "scrollWidth", {
    get: function () {
      if (this === document.documentElement || this === document.body) {
        return collectRootScrollExtents(this, this === document.documentElement).width;
      }

      let maxRight = this.clientWidth;
      const parentR = this.getBoundingClientRect();
      const shadow = this._shadowRoot || this.shadowRoot;
      const shadowKids = shadow ? (shadow.children || shadow._children || shadow.childNodes || []) : [];
      const all = (shadowKids.length > 0) ? shadowKids : (this.children || []);
      let accumX = 0;
      for (let i = 0; i < all.length; i++) {
        const child = all[i];
        if (!child || child.nodeType !== 1) continue;
        if (isElementDisplayNone(child)) continue;
        const cr = child.getBoundingClientRect ? child.getBoundingClientRect() : null;
        let childW = (cr && cr.width) || 0;
        if (!childW && child.style && child.style.width) {
          childW = parseFloat(child.style.width) || 0;
        }
        let relX = (parentR && cr && cr.width > 0) ? (cr.x - parentR.x - (this.clientLeft || 0) + (this._scrollLeft || 0)) : accumX;
        accumX += childW;
        const r = relX + childW;
        if (r > maxRight) maxRight = r;
      }
      return Math.round(maxRight);
    },
    configurable: true,
  });

  function computeAxis(elPos, elSize, viewSize, curScroll, mode) {
    switch (mode) {
      case "start":
        return elPos;
      case "end":
        return elPos + elSize - viewSize;
      case "center":
        return elPos + elSize / 2 - viewSize / 2;
      case "nearest": {
        if (elPos >= curScroll && elPos + elSize <= curScroll + viewSize) {
          return curScroll;
        }
        if (elPos < curScroll) return elPos;
        return elPos + elSize - viewSize;
      }
      default:
        return elPos;
    }
  }

  function scrollElementIntoView(el, block, inline, onlyIfNeeded) {
    if (!el) return;
    const vw = (typeof window !== "undefined" && window.innerWidth) || 1280;
    const vh = (typeof window !== "undefined" && window.innerHeight) || 720;

    const r = el.getBoundingClientRect();
    const inWin = (r.top >= 0 && r.bottom <= vh && r.left >= 0 && r.right <= vw);

    let clipped = false;
    let cur = getScrollParent(el);
    while (cur && cur !== document.body && cur !== document.documentElement && cur !== document) {
      if (cur.nodeType === 1) {
        const style = (typeof window !== "undefined" && window.getComputedStyle) ? window.getComputedStyle(cur) : cur.style;
        const overflow = (style && (style.overflow || style.overflowY || style.overflowX)) || "";
        const isScrollable = (overflow === "auto" || overflow === "scroll" || overflow === "hidden" || (cur.getAttribute && cur.getAttribute("style") && cur.getAttribute("style").includes("overflow")));
        if (isScrollable) {
          const cr = cur.getBoundingClientRect();
          if (r.top < cr.top || r.bottom > cr.top + cur.clientHeight || r.left < cr.left || r.right > cr.left + cur.clientWidth) {
            clipped = true;
            break;
          }
        }
      }
      cur = getScrollParent(cur);
    }

    if (onlyIfNeeded && inWin && !clipped) {
      return;
    }

    // 1. Scroll container ancestors
    cur = getScrollParent(el);
    let safety = 0;
    while (cur && cur !== document.body && cur !== document.documentElement && cur !== document && safety < 50) {
      safety++;
      if (cur.nodeType === 1) {
        const style = (typeof window !== "undefined" && window.getComputedStyle) ? window.getComputedStyle(cur) : cur.style;
        const ox = (style && (style.overflowX || style.overflow)) || "";
        const oy = (style && (style.overflowY || style.overflow)) || "";
        const isScrollableX = ox === "auto" || ox === "scroll" || ox === "hidden";
        const isScrollableY = oy === "auto" || oy === "scroll" || oy === "hidden";
        const ch = isScrollableY ? (cur.clientHeight || 0) : 0;
        const sh = (isScrollableY && ch > 0) ? (cur.scrollHeight || 0) : 0;
        const cw = isScrollableX ? (cur.clientWidth || 0) : 0;
        const sw = (isScrollableX && cw > 0) ? (cur.scrollWidth || 0) : 0;
        const overflowsY = isScrollableY && sh > ch && ch > 0;
        const overflowsX = isScrollableX && sw > cw && cw > 0;

        if (overflowsY || overflowsX) {
          const cr = cur.getBoundingClientRect();
          const er = el.getBoundingClientRect();
          const curScrollTop = cur.scrollTop || 0;
          const curScrollLeft = cur.scrollLeft || 0;
          const elInContentY = (er.top - cr.top) + curScrollTop;
          const elInContentX = (er.left - cr.left) + curScrollLeft;

          if (overflowsY) {
            const rawY = computeAxis(elInContentY, er.height, ch, curScrollTop, block);
            const maxY = Math.max(0, sh - ch);
            const targetY = Math.max(0, Math.min(rawY, maxY > 0 ? maxY : rawY));
            if (targetY !== curScrollTop) {
              cur.scrollTop = targetY;
            }
          }

          if (overflowsX) {
            const rawX = computeAxis(elInContentX, er.width, cw, curScrollLeft, inline);
            const maxX = Math.max(0, sw - cw);
            const targetX = Math.max(0, Math.min(rawX, maxX > 0 ? maxX : rawX));
            if (targetX !== curScrollLeft) {
              cur.scrollLeft = targetX;
            }
          }
        }
      }
      cur = getScrollParent(cur);
    }

    // 2. Scroll window if needed
    const afterR = el.getBoundingClientRect();
    const curWinX = (typeof window !== "undefined" && (window.scrollX || window.pageXOffset)) || 0;
    const curWinY = (typeof window !== "undefined" && (window.scrollY || window.pageYOffset)) || 0;
    const targetDocY = afterR.top + curWinY;
    const targetDocX = afterR.left + curWinX;

    const newWinY = computeAxis(targetDocY, afterR.height, vh, curWinY, block);
    const newWinX = computeAxis(targetDocX, afterR.width, vw, curWinX, inline);

    if (typeof window !== "undefined" && window.scrollTo) {
      window.scrollTo(Math.max(0, Math.round(newWinX)), Math.max(0, Math.round(newWinY)));
    }
  }

  proto.scrollIntoView = function (arg) {
    let block = "start";
    let inline = "nearest";
    if (arg === false) {
      block = "end";
      inline = "nearest";
    } else if (arg === true || arg == null) {
      block = "start";
      inline = "nearest";
    } else if (typeof arg === "object") {
      block = String(arg.block || "start");
      inline = String(arg.inline || "nearest");
    }
    scrollElementIntoView(this, block, inline, false);
  };
  markNative(proto.scrollIntoView, "scrollIntoView");

  proto.scrollIntoViewIfNeeded = function (centerIfNeeded) {
    const center = centerIfNeeded !== false;
    scrollElementIntoView(this, center ? "center" : "nearest", center ? "center" : "nearest", true);
  };
  markNative(proto.scrollIntoViewIfNeeded, "scrollIntoViewIfNeeded");

  proto.checkVisibility = function (options) {
    if (!this.isConnected || isElementDisplayNone(this)) return false;

    function getStyle(el) {
      if (typeof window !== "undefined" && typeof window.getComputedStyle === "function") {
        try {
          const cs = window.getComputedStyle(el);
          if (cs) return cs;
        } catch (e) {}
      }
      if (typeof getComputedStyle === "function") {
        try {
          const cs = getComputedStyle(el);
          if (cs) return cs;
        } catch (e) {}
      }
      if (typeof globalThis !== "undefined" && typeof globalThis.getComputedStyle === "function") {
        try {
          const cs = globalThis.getComputedStyle(el);
          if (cs) return cs;
        } catch (e) {}
      }
      return el.style || null;
    }

    function getProp(el, name) {
      if (!el) return "";
      const s = getStyle(el);
      let v = "";
      if (s) {
        if (typeof s.getPropertyValue === "function") {
          const val = s.getPropertyValue(name);
          if (val !== undefined && val !== null && val !== "") v = String(val).trim();
        }
        if (!v) {
          const camel = name.replace(/-([a-z])/g, function (_, c) { return c.toUpperCase(); });
          if (s[camel] !== undefined && s[camel] !== null && s[camel] !== "") {
            v = String(s[camel]).trim();
          } else if (s[name] !== undefined && s[name] !== null && s[name] !== "") {
            v = String(s[name]).trim();
          }
        }
      }
      if (!v && el.style) {
        if (typeof el.style.getPropertyValue === "function") {
          const val = el.style.getPropertyValue(name);
          if (val !== undefined && val !== null && val !== "") v = String(val).trim();
        }
        if (!v) {
          const camel = name.replace(/-([a-z])/g, function (_, c) { return c.toUpperCase(); });
          if (el.style[camel] !== undefined && el.style[camel] !== null && el.style[camel] !== "") {
            v = String(el.style[camel]).trim();
          }
        }
      }
      return v;
    }

    function isZero(val) {
      if (val === "" || val === null || val === undefined) return false;
      const num = Number(val);
      return !isNaN(num) && num === 0;
    }

    const selfDisplay = getProp(this, "display").toLowerCase();
    if (selfDisplay === "none" || selfDisplay === "contents") return false;

    const checkOpacity = Boolean(options && options.checkOpacity === true);
    const checkVisibilityCSS = Boolean(options && options.checkVisibilityCSS === true);

    if (checkOpacity) {
      const selfOpacity = getProp(this, "opacity");
      if (isZero(selfOpacity)) return false;
    }

    if (checkVisibilityCSS) {
      const selfVis = getProp(this, "visibility").toLowerCase();
      if (selfVis === "hidden" || selfVis === "collapse") return false;
    }

    let cur = this.parentElement || this.parentNode;
    while (cur) {
      if (cur.nodeType === 1) { // ELEMENT_NODE
        const d = getProp(cur, "display").toLowerCase();
        if (d === "none") return false;

        if (checkOpacity) {
          const op = getProp(cur, "opacity");
          if (isZero(op)) return false;
        }
      } else if (cur.nodeType === 11 && cur.host) { // ShadowRoot
        cur = cur.host;
        continue;
      }
      cur = cur.parentElement || cur.parentNode;
    }

    return true;
  };
  markNative(proto.checkVisibility, "checkVisibility");

  const elementShadowRootMap = new WeakMap();

  proto.attachShadow = function (init) {
    if (!init || typeof init !== "object" || !init.mode) {
      throw new TypeError("Failed to execute 'attachShadow' on 'Element': 1 argument required, but only 0 present.");
    }
    const mode = String(init.mode);
    if (mode !== "open" && mode !== "closed") {
      throw new TypeError("Failed to execute 'attachShadow' on 'Element': The provided value '" + mode + "' is not a valid enum value of type ShadowRootMode.");
    }
    if (elementShadowRootMap.get(this)) {
      if (typeof DOMException !== "undefined") {
        throw new DOMException("Shadow root cannot be created on a host which already hosts a shadow tree", "NotSupportedError");
      }
      const err = new Error("Shadow root cannot be created on a host which already hosts a shadow tree");
      err.name = "NotSupportedError";
      throw err;
    }
    const ShadowRootCtor = typeof ShadowRoot !== "undefined" ? ShadowRoot : (global.ShadowRoot || null);
    const shadow = ShadowRootCtor ? new ShadowRootCtor(this, mode) : new DocumentFragment();
    shadow.slotAssignment = init.slotAssignment === "manual" ? "manual" : "named";
    elementShadowRootMap.set(this, shadow);
    try {
      Object.defineProperty(this, "_shadowRoot", {
        value: shadow,
        writable: true,
        configurable: true,
      });
    } catch (e) {
      this._shadowRoot = shadow;
    }
    return shadow;
  };
  markNative(proto.attachShadow, "attachShadow");

  Object.defineProperty(proto, "_shadowRoot", {
    get: function () {
      return elementShadowRootMap.get(this) || null;
    },
    configurable: true,
  });

  Object.defineProperty(proto, "shadowRoot", {
    get: function () {
      const sr = elementShadowRootMap.get(this);
      if (sr && sr.mode === "open") {
        return sr;
      }
      return null;
    },
    configurable: true,
  });

  // Universal Form & Control Properties on Element (only for elements that define .name in HTML)
  const ELEMENTS_WITH_NAME = new Set([
    "a", "button", "embed", "fieldset", "form", "frame", "frameset",
    "iframe", "img", "input", "map", "meta", "object", "output",
    "param", "select", "slot", "textarea",
  ]);
  Object.defineProperty(proto, "name", {
    get: function () {
      const tag = String(this.localName || this.tagName || "").toLowerCase();
      if (!ELEMENTS_WITH_NAME.has(tag)) return undefined;
      return this.getAttribute("name") || "";
    },
    set: function (v) {
      const tag = String(this.localName || this.tagName || "").toLowerCase();
      if (!ELEMENTS_WITH_NAME.has(tag)) {
        Object.defineProperty(this, "name", { value: v, writable: true, enumerable: true, configurable: true });
        return;
      }
      this.setAttribute("name", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(proto, "type", {
    get: function () {
      const tag = (this.tagName || "").toLowerCase();
      const t = this.getAttribute("type");
      if (t) return t.toLowerCase();
      if (tag === "button") return "submit";
      if (tag === "select") return this.multiple ? "select-multiple" : "select-one";
      if (tag === "textarea") return "textarea";
      if (tag === "input") return "text";
      return "";
    },
    set: function (v) { this.setAttribute("type", String(v)); },
    configurable: true,
  });

  Object.defineProperty(proto, "value", {
    get: function () {
      const tag = (this.tagName || "").toLowerCase();
      if (tag === "select") {
        const idx = this.selectedIndex;
        const opts = this.options;
        if (idx >= 0 && idx < opts.length) {
          return opts[idx].value;
        }
        return "";
      }
      if (this._value !== undefined) return this._value;
      if (tag === "checkbox" || tag === "radio") {
        return this.getAttribute("value") ?? "on";
      }
      if (this.hasAttribute && this.hasAttribute("value")) return this.getAttribute("value");
      if (tag === "textarea" || tag === "option") return this.textContent || "";
      return "";
    },
    set: function (v) {
      const tag = (this.tagName || "").toLowerCase();
      if (tag === "select") {
        const str = String(v);
        const opts = this.options;
        for (let i = 0; i < opts.length; i++) {
          if (opts[i].value === str) {
            this.selectedIndex = i;
            return;
          }
        }
        this.selectedIndex = -1;
        return;
      }
      this._value = String(v);
      if (this.setAttribute) {
        this.setAttribute("value", String(v));
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "defaultValue", {
    get: function () {
      return this.getAttribute("value") || "";
    },
    set: function (v) {
      this.setAttribute("value", String(v));
    },
    configurable: true,
  });

  Object.defineProperty(proto, "checked", {
    get: function () {
      if (this._checkedDirty) return this._checked;
      return this.hasAttribute("checked");
    },
    set: function (v) {
      this._checkedDirty = true;
      this._checked = Boolean(v);
      const isRadio = (this.getAttribute && (this.getAttribute("type") || "").toLowerCase() === "radio");
      if (isRadio && v && this.name) {
        const root = (this.ownerDocument || (typeof document !== "undefined" ? document : null));
        if (root && root.querySelectorAll) {
          const radios = root.querySelectorAll('input[type="radio"][name="' + this.name + '"]');
          for (let i = 0; i < radios.length; i++) {
            if (radios[i] !== this) {
              radios[i]._checkedDirty = true;
              radios[i]._checked = false;
              if (radios[i].removeAttribute) radios[i].removeAttribute("checked");
            }
          }
        }
      }
      if (v) {
        if (this.setAttribute) this.setAttribute("checked", "");
      } else {
        if (this.removeAttribute) this.removeAttribute("checked");
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "selected", {
    get: function () {
      if ((this.tagName || "").toLowerCase() === "option" && typeof global !== "undefined" && global.HTMLOptionElement) {
        const desc = Object.getOwnPropertyDescriptor(global.HTMLOptionElement.prototype, "selected");
        if (desc && desc.get) return desc.get.call(this);
      }
      if (this._selected !== undefined) return this._selected;
      return this.hasAttribute("selected");
    },
    set: function (v) {
      if ((this.tagName || "").toLowerCase() === "option" && typeof global !== "undefined" && global.HTMLOptionElement) {
        const desc = Object.getOwnPropertyDescriptor(global.HTMLOptionElement.prototype, "selected");
        if (desc && desc.set) return desc.set.call(this, v);
      }
      this._selected = Boolean(v);
      if (v) {
        if (this.setAttribute) this.setAttribute("selected", "");
      } else {
        if (this.removeAttribute) this.removeAttribute("selected");
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "disabled", {
    get: function () { return this.hasAttribute("disabled"); },
    set: function (v) { if (v) this.setAttribute("disabled", ""); else this.removeAttribute("disabled"); },
    configurable: true,
  });

  Object.defineProperty(proto, "required", {
    get: function () { return this.hasAttribute("required"); },
    set: function (v) { if (v) this.setAttribute("required", ""); else this.removeAttribute("required"); },
    configurable: true,
  });

  Object.defineProperty(proto, "readOnly", {
    get: function () { return this.hasAttribute("readonly"); },
    set: function (v) { if (v) this.setAttribute("readonly", ""); else this.removeAttribute("readonly"); },
    configurable: true,
  });

  Object.defineProperty(proto, "placeholder", {
    get: function () { return this.getAttribute("placeholder") || ""; },
    set: function (v) { this.setAttribute("placeholder", String(v)); },
    configurable: true,
  });

  Object.defineProperty(proto, "options", {
    get: function () {
      if ((this.tagName || "").toLowerCase() === "select") {
        if (typeof getSelectOptionsCollection === "function") {
          return getSelectOptionsCollection(this);
        }
      }
      const opts = [];
      const up = typeof upgradeElement === "function" ? upgradeElement : (typeof globalThis !== "undefined" && globalThis.upgradeElement);
      let child = this.firstChild;
      while (child) {
        if ((child.tagName || "").toLowerCase() === "option") {
          if (up) up(child);
          opts.push(child);
        }
        child = child.nextSibling;
      }
      return new HTMLCollection(opts);
    },
    configurable: true,
  });

  Object.defineProperty(proto, "selectedIndex", {
    get: function () {
      if ((this.tagName || "").toLowerCase() === "select" && typeof global !== "undefined" && global.HTMLSelectElement) {
        const desc = Object.getOwnPropertyDescriptor(global.HTMLSelectElement.prototype, "selectedIndex");
        if (desc && desc.get) return desc.get.call(this);
      }
      const opts = this.options;
      for (let i = 0; i < opts.length; i++) {
        if (opts[i].selected) return i;
      }
      return opts.length > 0 && !this.multiple ? 0 : -1;
    },
    set: function (idx) {
      if ((this.tagName || "").toLowerCase() === "select" && typeof global !== "undefined" && global.HTMLSelectElement) {
        const desc = Object.getOwnPropertyDescriptor(global.HTMLSelectElement.prototype, "selectedIndex");
        if (desc && desc.set) return desc.set.call(this, idx);
      }
      const i = Number(idx);
      const opts = this.options;
      for (let j = 0; j < opts.length; j++) {
        opts[j].selected = (j === i);
      }
    },
    configurable: true,
  });

  Object.defineProperty(proto, "validity", {
    get: function () { return makeValidityState(this); },
    configurable: true,
  });

  Object.defineProperty(proto, "willValidate", {
    get: function () {
      return computeWillValidate(this);
    },
    configurable: true,
  });

  proto.checkValidity = function () {
    if (!this.willValidate) return true;
    if (this.validity && !this.validity.valid) {
      const evt = new Event("invalid", { bubbles: false, cancelable: true });
      this.dispatchEvent(evt);
      return false;
    }
    return true;
  };
  markNative(proto.checkValidity, "checkValidity");

  proto.reportValidity = function () {
    return this.checkValidity();
  };
  markNative(proto.reportValidity, "reportValidity");

  proto.setCustomValidity = function (msg) {
    this._customValidityMessage = String(msg || "");
  };
  markNative(proto.setCustomValidity, "setCustomValidity");

  proto.insertAdjacentElement = function (position, element) {
    const pos = String(position).toLowerCase();
    if (pos === "beforebegin") {
      if (!this.parentNode) return null;
      return this.parentNode.insertBefore(element, this);
    }
    if (pos === "afterbegin") {
      return this.insertBefore(element, this.firstChild);
    }
    if (pos === "beforeend") {
      return this.appendChild(element);
    }
    if (pos === "afterend") {
      if (!this.parentNode) return null;
      return this.parentNode.insertBefore(element, this.nextSibling);
    }
    if (typeof DOMException !== "undefined") {
      throw new DOMException("The value provided ('" + position + "') is not one of 'beforeBegin', 'afterBegin', 'beforeEnd', or 'afterEnd'.", "SyntaxError");
    }
    const e = new Error("SyntaxError");
    e.name = "SyntaxError";
    throw e;
  };
  markNative(proto.insertAdjacentElement, "insertAdjacentElement");

  proto.insertAdjacentText = function (position, data) {
    const pos = String(position).toLowerCase();
    const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
    const textNode = doc && typeof doc.createTextNode === "function" ? doc.createTextNode(String(data)) : new Text(String(data));
    if (pos === "beforebegin") {
      if (!this.parentNode) return;
      this.parentNode.insertBefore(textNode, this);
      return;
    }
    if (pos === "afterbegin") {
      this.insertBefore(textNode, this.firstChild);
      return;
    }
    if (pos === "beforeend") {
      this.appendChild(textNode);
      return;
    }
    if (pos === "afterend") {
      if (!this.parentNode) return;
      this.parentNode.insertBefore(textNode, this.nextSibling);
      return;
    }
    if (typeof DOMException !== "undefined") {
      throw new DOMException("The value provided ('" + position + "') is not one of 'beforeBegin', 'afterBegin', 'beforeEnd', or 'afterEnd'.", "SyntaxError");
    }
    const e = new Error("SyntaxError");
    e.name = "SyntaxError";
    throw e;
  };
  markNative(proto.insertAdjacentText, "insertAdjacentText");

  proto.insertAdjacentHTML = function (position, text) {
    const pos = String(position).toLowerCase();
    if (pos !== "beforebegin" && pos !== "afterbegin" && pos !== "beforeend" && pos !== "afterend") {
      if (typeof DOMException !== "undefined") {
        throw new DOMException("The value provided ('" + position + "') is not one of 'beforeBegin', 'afterBegin', 'beforeEnd', or 'afterEnd'.", "SyntaxError");
      }
      const e = new Error("SyntaxError");
      e.name = "SyntaxError";
      throw e;
    }
    if ((pos === "beforebegin" || pos === "afterend") && !this.parentNode) {
      if (typeof DOMException !== "undefined") {
        throw new DOMException("The element has no parent.", "NoModificationAllowedError");
      }
      const e = new Error("NoModificationAllowedError");
      e.name = "NoModificationAllowedError";
      throw e;
    }
    const doc = this.ownerDocument || (typeof document !== "undefined" ? document : null);
    const temp = doc && typeof doc.createElement === "function" ? doc.createElement("div") : null;
    if (!temp) return;
    temp.innerHTML = String(text);
    const nodes = [];
    let ch = temp.firstChild;
    while (ch) {
      const next = ch.nextSibling;
      nodes.push(ch);
      ch = next;
    }
    if (pos === "beforebegin") {
      const parent = this.parentNode;
      for (let i = 0; i < nodes.length; i++) {
        parent.insertBefore(nodes[i], this);
      }
    } else if (pos === "afterbegin") {
      const ref = this.firstChild;
      for (let i = 0; i < nodes.length; i++) {
        this.insertBefore(nodes[i], ref);
      }
    } else if (pos === "beforeend") {
      for (let i = 0; i < nodes.length; i++) {
        this.appendChild(nodes[i]);
      }
    } else if (pos === "afterend") {
      const parent = this.parentNode;
      const ref = this.nextSibling;
      for (let i = 0; i < nodes.length; i++) {
        parent.insertBefore(nodes[i], ref);
      }
    }
  };
  markNative(proto.insertAdjacentHTML, "insertAdjacentHTML");
}
