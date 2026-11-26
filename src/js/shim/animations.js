// src/js/shim/animations.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/animations.ts
// Web Animations API (web-animations-1) implementation.
// Configured by globalThis.__CAP_RENDER_CONFIG.animations (true: full playback timeline, false: static headless immediate finish).

function AnimationTimeline() {}
AnimationTimeline.prototype.currentTime = 0;
markNative(AnimationTimeline, "AnimationTimeline");

function DocumentTimeline(options) {
  AnimationTimeline.call(this);
  this.originTime = Number(options && options.originTime || 0);
}
DocumentTimeline.prototype = Object.create(AnimationTimeline.prototype);
DocumentTimeline.prototype.constructor = DocumentTimeline;
Object.defineProperty(DocumentTimeline.prototype, "currentTime", {
  get: function () {
    if (this._frameTime !== undefined && this._frameTime !== null) return this._frameTime;
    const now = (typeof performance !== "undefined" && performance.now) ? performance.now() : Date.now();
    this._frameTime = now;
    const tl = this;
    Promise.resolve().then(function () { tl._frameTime = null; });
    return now;
  },
  configurable: true,
});
markNative(DocumentTimeline, "DocumentTimeline");

const globalDocTimeline = new DocumentTimeline();

function AnimationEffect() {}
markNative(AnimationEffect, "AnimationEffect");

function validateEasingString(easing) {
  if (easing === undefined || easing === null) return "linear";
  const s = String(easing).trim().toLowerCase();
  if (
    s === "linear" ||
    s === "ease" ||
    s === "ease-in" ||
    s === "ease-out" ||
    s === "ease-in-out" ||
    s === "step-start" ||
    s === "step-end"
  ) {
    return s;
  }
  const cb = s.match(/^cubic-bezier\(\s*([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)\s*,\s*([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)\s*,\s*([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)\s*,\s*([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)\s*\)$/);
  if (cb) {
    const x1 = parseFloat(cb[1]);
    const y1 = parseFloat(cb[2]);
    const x2 = parseFloat(cb[3]);
    const y2 = parseFloat(cb[4]);
    if (
      isFinite(x1) && isFinite(y1) && isFinite(x2) && isFinite(y2) &&
      x1 >= 0 && x1 <= 1 && x2 >= 0 && x2 <= 1
    ) {
      return s;
    }
    throw new TypeError("Failed to parse easing '" + easing + "': cubic-bezier x values must be in [0, 1].");
  }
  const st = s.match(/^steps\(\s*(\d+)(?:\s*,\s*(start|end|jump-start|jump-end|jump-none|jump-both))?\s*\)$/);
  if (st) {
    const count = parseInt(st[1], 10);
    const jump = st[2] || "end";
    if (count > 0 && !(jump === "jump-none" && count < 2)) {
      return s;
    }
  }
  if (s.startsWith("linear(") && s.endsWith(")")) {
    const body = s.slice(7, -1).trim();
    if (body) {
      const stops = body.split(",").map((x) => x.trim());
      if (stops.length >= 2) {
        let ok = true;
        for (let i = 0; i < stops.length; i++) {
          const toks = stops[i].split(/\s+/).filter(Boolean);
          if (toks.length < 1 || toks.length > 3) { ok = false; break; }
          if (!/^[+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?$/.test(toks[0])) { ok = false; break; }
          for (let j = 1; j < toks.length; j++) {
            if (!/^[+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?%$/.test(toks[j])) { ok = false; break; }
          }
          if (!ok) break;
        }
        if (ok) return s;
      }
    }
  }
  throw new TypeError("Failed to parse easing '" + easing + "': Invalid timing function.");
}

function normalizeKeyframesInput(keyframes) {
  if (!keyframes) return [];
  let frames;
  if (Array.isArray(keyframes)) {
    frames = keyframes.map(function (kf) {
      const copy = kf && typeof kf === "object" ? Object.assign({}, kf) : {};
      if (copy.easing !== undefined) {
        copy.easing = validateEasingString(copy.easing);
      }
      return copy;
    });
  } else if (typeof keyframes === "object") {
    // Property-indexed keyframes: { color: ["#205080", "#d0a040"], transform: [...], easing: "..." }
    const propArrays = {};
    let maxLen = 0;
    const sharedEasing = keyframes.easing !== undefined ? validateEasingString(keyframes.easing) : null;
    const sharedComposite = typeof keyframes.composite === "string" ? keyframes.composite : null;
    const offsets = Array.isArray(keyframes.offset)
      ? keyframes.offset
      : (keyframes.offset !== undefined && keyframes.offset !== null ? [keyframes.offset] : null);
    for (const k in keyframes) {
      if (!Object.prototype.hasOwnProperty.call(keyframes, k)) continue;
      if (k === "easing" || k === "composite" || k === "offset") continue;
      const v = keyframes[k];
      if (Array.isArray(v)) {
        propArrays[k] = v;
        if (v.length > maxLen) maxLen = v.length;
      } else if (v !== undefined) {
        propArrays[k] = [v];
        if (maxLen < 1) maxLen = 1;
      }
    }
    if (maxLen === 0) return [];
    frames = [];
    for (let i = 0; i < maxLen; i++) {
      const f = {};
      if (offsets && offsets[i] != null) f.offset = Number(offsets[i]);
      else if (maxLen > 1) f.offset = i / (maxLen - 1);
      else f.offset = 1;
      if (sharedEasing) f.easing = sharedEasing;
      if (sharedComposite) f.composite = sharedComposite;
      for (const k in propArrays) {
        const arr = propArrays[k];
        if (i < arr.length) f[k] = arr[i];
      }
      frames.push(f);
    }
  } else {
    return [];
  }

  // Validate explicit keyframe offsets are in [0, 1] and loosely sorted (non-decreasing)
  let prevOffset = -Infinity;
  for (let i = 0; i < frames.length; i++) {
    const rawOff = frames[i].offset;
    if (rawOff !== undefined && rawOff !== null) {
      const off = Number(rawOff);
      if (isNaN(off) || off < 0 || off > 1) {
        throw new TypeError("Offsets must be in the range [0, 1].");
      }
      if (off < prevOffset) {
        throw new TypeError("Keyframes with specified offsets are not loosely sorted.");
      }
      prevOffset = off;
      frames[i].offset = off;
    }
  }
  return frames;
}

function solveCubicBezier(x1, y1, x2, y2, t) {
  if (t === 0) return 0;
  if (t === 1) return 1;
  if (t < 0) {
    let slope = 0;
    if (x1 > 0) slope = y1 / x1;
    else if (x1 === 0 && y1 === 0 && x2 > 0) slope = y2 / x2;
    return slope * t;
  }
  if (t > 1) {
    let slope = 0;
    if (x2 < 1) slope = (1 - y2) / (1 - x2);
    else if (x2 === 1 && y2 === 1 && x1 < 1) slope = (1 - y1) / (1 - x1);
    return 1 + slope * (t - 1);
  }
  // Solve B_x(u) = t for u in [0, 1] using Newton-Raphson + bisection fallback
  const sampleX = function (u) {
    const inv = 1 - u;
    return 3 * inv * inv * u * x1 + 3 * inv * u * u * x2 + u * u * u;
  };
  const sampleDX = function (u) {
    const inv = 1 - u;
    return 3 * inv * inv * x1 + 6 * inv * u * (x2 - x1) + 3 * u * u * (1 - x2);
  };
  const sampleY = function (u) {
    const inv = 1 - u;
    return 3 * inv * inv * u * y1 + 3 * inv * u * u * y2 + u * u * u;
  };
  let u = t;
  for (let i = 0; i < 8; i++) {
    const err = sampleX(u) - t;
    if (Math.abs(err) < 1e-10) return sampleY(u);
    const d = sampleDX(u);
    if (Math.abs(d) < 1e-10) break;
    u -= err / d;
    if (u < 0 || u > 1) break;
  }
  let lo = 0, hi = 1;
  u = t;
  for (let i = 0; i < 32; i++) {
    const x = sampleX(u);
    if (Math.abs(x - t) < 1e-10) break;
    if (x < t) lo = u;
    else hi = u;
    u = 0.5 * (lo + hi);
  }
  return sampleY(u);
}

function evaluateAnimationEasing(easing, t) {
  if (!easing || easing === "linear") return t;
  const s = String(easing).trim().toLowerCase();
  if (s === "linear") return t;
  if (s === "ease") return solveCubicBezier(0.25, 0.1, 0.25, 1.0, t);
  if (s === "ease-in") return solveCubicBezier(0.42, 0.0, 1.0, 1.0, t);
  if (s === "ease-out") return solveCubicBezier(0.0, 0.0, 0.58, 1.0, t);
  if (s === "ease-in-out") return solveCubicBezier(0.42, 0.0, 0.58, 1.0, t);
  const cb = s.match(/^cubic-bezier\(\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^)]+)\s*\)$/);
  if (cb) {
    const x1 = parseFloat(cb[1]);
    const y1 = parseFloat(cb[2]);
    const x2 = parseFloat(cb[3]);
    const y2 = parseFloat(cb[4]);
    if (!isNaN(x1) && !isNaN(y1) && !isNaN(x2) && !isNaN(y2)) {
      return solveCubicBezier(x1, y1, x2, y2, t);
    }
  }
  let stEff = s;
  if (s === "step-start") stEff = "steps(1, start)";
  else if (s === "step-end") stEff = "steps(1, end)";
  const st = stEff.match(/^steps\(\s*(\d+)(?:\s*,\s*(start|end|jump-start|jump-end|jump-none|jump-both))?\s*\)$/);
  if (st) {
    const steps = parseInt(st[1], 10);
    const jump = st[2] || "end";
    let currentStep = Math.floor(t * steps);
    if (jump === "start" || jump === "jump-start" || jump === "jump-both") {
      currentStep += 1;
    }
    let jumps = steps;
    if (jump === "jump-none") jumps = steps - 1;
    else if (jump === "jump-both") jumps = steps + 1;
    if (t >= 0 && currentStep < 0) currentStep = 0;
    if (t <= 1 && currentStep > jumps) currentStep = jumps;
    return jumps > 0 ? currentStep / jumps : 1;
  }
  if (s.startsWith("linear(") && s.endsWith(")")) {
    const rawStops = s.slice(7, -1).split(",").map((x) => x.trim()).filter(Boolean);
    const stops = [];
    for (let i = 0; i < rawStops.length; i++) {
      const toks = rawStops[i].split(/\s+/).filter(Boolean);
      const y = parseFloat(toks[0]);
      if (toks.length === 1) stops.push({ y, p: null });
      else if (toks.length === 2) stops.push({ y, p: parseFloat(toks[1]) / 100 });
      else {
        stops.push({ y, p: parseFloat(toks[1]) / 100 });
        stops.push({ y, p: parseFloat(toks[2]) / 100 });
      }
    }
    if (stops.length >= 2) {
      if (stops[0].p === null) stops[0].p = 0;
      if (stops[stops.length - 1].p === null) stops[stops.length - 1].p = 1;
      let maxP = stops[0].p;
      for (let i = 0; i < stops.length; i++) {
        if (stops[i].p !== null) {
          if (stops[i].p < maxP) stops[i].p = maxP;
          maxP = stops[i].p;
        }
      }
      let i = 0;
      while (i < stops.length) {
        if (stops[i].p === null) {
          let j = i + 1;
          while (j < stops.length && stops[j].p === null) j++;
          const p0 = stops[i - 1].p, p1 = stops[j].p, cnt = j - i + 1;
          for (let k = i; k < j; k++) stops[k].p = p0 + ((p1 - p0) * (k - i + 1)) / cnt;
          i = j;
        } else i++;
      }
      if (t < stops[0].p) {
        let j = 1;
        while (j < stops.length && stops[j].p === stops[0].p) j++;
        if (j < stops.length && stops[j].p !== stops[0].p) {
          return stops[0].y + ((t - stops[0].p) / (stops[j].p - stops[0].p)) * (stops[j].y - stops[0].y);
        }
        return stops[0].y;
      }
      const last = stops.length - 1;
      if (t >= stops[last].p) {
        let j = last - 1;
        while (j >= 0 && stops[j].p === stops[last].p) j--;
        if (j >= 0 && stops[last].p !== stops[j].p) {
          return stops[last].y + ((t - stops[last].p) / (stops[last].p - stops[j].p)) * (stops[last].y - stops[j].y);
        }
        return stops[last].y;
      }
      for (let k = 0; k < last; k++) {
        if (t >= stops[k].p && t < stops[k + 1].p) {
          const span = stops[k + 1].p - stops[k].p;
          if (span === 0) return stops[k + 1].y;
          return stops[k].y + ((t - stops[k].p) / span) * (stops[k + 1].y - stops[k].y);
        }
      }
      return stops[last].y;
    }
  }
  return t;
}

function parseAnimColor(str) {
  if (typeof str !== "string") return null;
  const s = str.trim().toLowerCase();
  const named = {
    black: [0, 0, 0, 1], white: [255, 255, 255, 1], red: [255, 0, 0, 1],
    green: [0, 128, 0, 1], blue: [0, 0, 255, 1], yellow: [255, 255, 0, 1],
    transparent: [0, 0, 0, 0],
  };
  if (Object.prototype.hasOwnProperty.call(named, s)) return named[s];
  if (s.startsWith("#")) {
    const hex = s.slice(1);
    if (hex.length === 3) {
      return [
        parseInt(hex[0] + hex[0], 16),
        parseInt(hex[1] + hex[1], 16),
        parseInt(hex[2] + hex[2], 16),
        1,
      ];
    }
    if (hex.length === 6) {
      return [
        parseInt(hex.slice(0, 2), 16),
        parseInt(hex.slice(2, 4), 16),
        parseInt(hex.slice(4, 6), 16),
        1,
      ];
    }
    if (hex.length === 8) {
      return [
        parseInt(hex.slice(0, 2), 16),
        parseInt(hex.slice(2, 4), 16),
        parseInt(hex.slice(4, 6), 16),
        parseInt(hex.slice(6, 8), 16) / 255,
      ];
    }
  }
  const m = s.match(/^rgba?\(\s*([0-9.]+)\s*[,\s]\s*([0-9.]+)\s*[,\s]\s*([0-9.]+)(?:\s*[,/]\s*([0-9.]+))?\s*\)$/);
  if (m) {
    return [parseFloat(m[1]), parseFloat(m[2]), parseFloat(m[3]), m[4] !== undefined ? parseFloat(m[4]) : 1];
  }
  return null;
}

function interpolateAnimValue(prop, fromVal, toVal, p) {
  const k = String(prop).toLowerCase();
  if (k === "color" || k.endsWith("color")) {
    const c1 = parseAnimColor(fromVal);
    const c2 = parseAnimColor(toVal);
    if (c1 && c2) {
      const r = Math.round(c1[0] + (c2[0] - c1[0]) * p);
      const g = Math.round(c1[1] + (c2[1] - c1[1]) * p);
      const b = Math.round(c1[2] + (c2[2] - c1[2]) * p);
      const a = c1[3] + (c2[3] - c1[3]) * p;
      if (a >= 1 - 1e-6) return "rgb(" + r + ", " + g + ", " + b + ")";
      return "rgba(" + r + ", " + g + ", " + b + ", " + Number(a.toFixed(3)) + ")";
    }
  }
  if (k === "transform") {
    const f1 = String(fromVal || "").trim();
    const f2 = String(toVal || "").trim();
    const re = /([a-zA-Z0-9_-]+)\s*\(([^)]*)\)/g;
    const list1 = [];
    const list2 = [];
    let m;
    while ((m = re.exec(f1)) !== null) list1.push({ fn: m[1].toLowerCase(), args: m[2].split(",").map((x) => x.trim()) });
    re.lastIndex = 0;
    while ((m = re.exec(f2)) !== null) list2.push({ fn: m[1].toLowerCase(), args: m[2].split(",").map((x) => x.trim()) });
    if (list1.length > 0 && list1.length === list2.length) {
      const out = [];
      let ok = true;
      for (let i = 0; i < list1.length; i++) {
        if (list1[i].fn !== list2[i].fn || list1[i].args.length !== list2[i].args.length) {
          ok = false;
          break;
        }
        const iArgs = [];
        for (let j = 0; j < list1[i].args.length; j++) {
          const m1 = list1[i].args[j].match(/^([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)([a-z%]*)$/i);
          const m2 = list2[i].args[j].match(/^([+-]?(?:\d+\.?\d*|\.\d+)(?:e[+-]?\d+)?)([a-z%]*)$/i);
          if (!m1 || !m2) { ok = false; break; }
          const v1 = parseFloat(m1[1]);
          const v2 = parseFloat(m2[1]);
          const u1 = m1[2] || m2[2] || "";
          const iv = v1 + (v2 - v1) * p;
          iArgs.push(String(iv) + u1);
        }
        if (!ok) break;
        out.push(list1[i].fn + "(" + iArgs.join(", ") + ")");
      }
      if (ok) return out.join(" ");
    }
  }
  const fn1 = String(fromVal == null ? "" : fromVal).trim().match(/^([a-zA-Z0-9_-]+)\(([^)]*)\)$/);
  const fn2 = String(toVal == null ? "" : toVal).trim().match(/^([a-zA-Z0-9_-]+)\(([^)]*)\)$/);
  if (fn1 && fn2 && fn1[1].toLowerCase() === fn2[1].toLowerCase()) {
    const a1 = fn1[2].trim().split(/\s+/).filter(Boolean);
    const a2 = fn2[2].trim().split(/\s+/).filter(Boolean);
    if (a1.length > 0 && a1.length === a2.length) {
      const parts = [];
      let allNum = true;
      for (let i = 0; i < a1.length; i++) {
        if (a1[i].toLowerCase() === a2[i].toLowerCase() && (/^[a-z\/]+$/i.test(a1[i]) || a1[i] === "/")) {
          parts.push(a1[i].toLowerCase());
          continue;
        }
        const m1 = a1[i].match(/^([+-]?(?:\d+\.?\d*|\.\d+))([a-z%]*)$/i);
        const m2 = a2[i].match(/^([+-]?(?:\d+\.?\d*|\.\d+))([a-z%]*)$/i);
        if (!m1 || !m2 || m1[2] !== m2[2]) { allNum = false; break; }
        const v = parseFloat(m1[1]) + (parseFloat(m2[1]) - parseFloat(m1[1])) * p;
        const formatted = Math.abs(v - Math.round(v)) < 1e-6 ? String(Math.round(v)) : String(Number(v.toFixed(6)));
        parts.push(formatted + m1[2]);
      }
      if (allNum) return fn1[1].toLowerCase() + "(" + parts.join(" ") + ")";
    }
  }
  const n1 = String(fromVal == null ? "" : fromVal).trim().match(/^([+-]?(?:\d+\.?\d*|\.\d+))([a-z%]*)$/i);
  const n2 = String(toVal == null ? "" : toVal).trim().match(/^([+-]?(?:\d+\.?\d*|\.\d+))([a-z%]*)$/i);
  if (n1 && n2 && (n1[2] === n2[2] || (parseFloat(n1[1]) === 0 && n1[2] === "") || (parseFloat(n2[1]) === 0 && n2[2] === ""))) {
    const unit = n1[2] || n2[2];
    const v = parseFloat(n1[1]) + (parseFloat(n2[1]) - parseFloat(n1[1])) * p;
    if (unit === "" && (prop === "order" || prop === "z-index" || prop === "column-count" || prop === "orphans" || prop === "widows")) {
      const r = Math.round(v);
      return String(r === 0 ? 0 : r);
    }
    const formatted = Math.abs(v - Math.round(v)) < 1e-6 ? String(Math.round(v)) : String(Number(v.toFixed(6)));
    return formatted + unit;
  }
  return String(p < 0.5 ? fromVal : toVal);
}

const VALID_FILL_MODES = new Set(["none", "forwards", "backwards", "both", "auto"]);
const VALID_DIRECTIONS = new Set(["normal", "reverse", "alternate", "alternate-reverse"]);

function KeyframeEffect(target, keyframes, options) {
  AnimationEffect.call(this);
  this.target = target || null;
  this._animation = null;
  this.keyframes = normalizeKeyframesInput(keyframes);
  const opts = typeof options === "number" ? { duration: options } : (options || {});
  const delay = opts.delay !== undefined ? Number(opts.delay) : 0;
  const endDelay = opts.endDelay !== undefined ? Number(opts.endDelay) : 0;
  if (isNaN(delay) || isNaN(endDelay)) {
    throw new TypeError("delay and endDelay must be finite numbers.");
  }
  const fill = opts.fill !== undefined ? String(opts.fill) : "auto";
  if (!VALID_FILL_MODES.has(fill)) {
    throw new TypeError("Failed to construct 'KeyframeEffect': The provided value '" + fill + "' is not a valid enum value of type FillMode.");
  }
  const direction = opts.direction !== undefined ? String(opts.direction) : "normal";
  if (!VALID_DIRECTIONS.has(direction)) {
    throw new TypeError("Failed to construct 'KeyframeEffect': The provided value '" + direction + "' is not a valid enum value of type PlaybackDirection.");
  }
  const iterationStart = opts.iterationStart !== undefined ? Number(opts.iterationStart) : 0;
  if (isNaN(iterationStart) || iterationStart < 0) {
    throw new TypeError("iterationStart must be a non-negative number.");
  }
  const iterations = opts.iterations !== undefined ? Number(opts.iterations) : 1;
  if (isNaN(iterations) || iterations < 0) {
    throw new TypeError("iterations must be a non-negative number.");
  }
  let duration;
  if (opts.duration === undefined || opts.duration === "auto") {
    duration = opts.duration === "auto" ? "auto" : 0;
  } else {
    duration = Number(opts.duration);
    if (isNaN(duration) || duration < 0) {
      throw new TypeError("duration must be non-negative or 'auto'.");
    }
  }
  const easing = opts.easing !== undefined ? validateEasingString(opts.easing) : "linear";

  this.delay = delay;
  this.endDelay = endDelay;
  this.fill = fill;
  this.iterationStart = iterationStart;
  this.iterations = iterations;
  this.duration = duration;
  this.direction = direction;
  this.easing = easing;
  this.pseudoElement = opts.pseudoElement != null ? String(opts.pseudoElement) : null;
}
KeyframeEffect.prototype = Object.create(AnimationEffect.prototype);
KeyframeEffect.prototype.constructor = KeyframeEffect;

AnimationEffect.prototype.getTiming = function () {
  return {
    delay: this.delay,
    endDelay: this.endDelay,
    fill: this.fill,
    iterationStart: this.iterationStart,
    iterations: this.iterations,
    duration: this.duration,
    direction: this.direction,
    easing: this.easing,
  };
};

KeyframeEffect.prototype.getComputedTiming = function () {
  const iterDur = typeof this.duration === "number" ? this.duration : 0;
  const activeDuration = this.iterations === 0 ? 0 : (this.iterations === Infinity ? (iterDur > 0 ? Infinity : 0) : iterDur * this.iterations);
  const endTime = Math.max(0, this.delay + activeDuration + this.endDelay);
  const anim = this._animation;
  const localTime = (anim && anim.currentTime !== null && anim.currentTime !== undefined) ? Number(anim.currentTime) : null;
  const effectiveFill = this.fill === "auto" ? "none" : this.fill;

  let progress = null;
  let currentIteration = null;

  if (localTime !== null) {
    const activeTime = localTime - this.delay;
    if (activeTime < 0) {
      // Before phase
      if (effectiveFill === "backwards" || effectiveFill === "both") {
        const overall = this.iterationStart;
        currentIteration = Math.floor(overall);
        let simple = overall - currentIteration;
        const rev = this.direction === "reverse" ||
          (this.direction === "alternate" && currentIteration % 2 === 1) ||
          (this.direction === "alternate-reverse" && currentIteration % 2 === 0);
        const directed = rev ? 1 - simple : simple;
        progress = evaluateAnimationEasing(this.easing, directed);
      }
    } else if (activeDuration > 0 ? activeTime >= activeDuration : activeTime >= 0) {
      // After phase
      if (effectiveFill === "forwards" || effectiveFill === "both") {
        const overall = this.iterationStart + this.iterations;
        const isAtBoundary = overall > 0 && Math.abs(overall - Math.round(overall)) < 1e-12;
        currentIteration = isAtBoundary ? Math.round(overall) - 1 : Math.floor(overall);
        if (currentIteration < 0) currentIteration = 0;
        let simple = isAtBoundary ? 1 : (overall - Math.floor(overall));
        if (this.iterations === 0) simple = 0;
        const rev = this.direction === "reverse" ||
          (this.direction === "alternate" && currentIteration % 2 === 1) ||
          (this.direction === "alternate-reverse" && currentIteration % 2 === 0);
        const directed = rev ? 1 - simple : simple;
        progress = evaluateAnimationEasing(this.easing, directed);
      }
    } else {
      // Active phase
      const overall = iterDur > 0 ? (activeTime / iterDur) + this.iterationStart : this.iterationStart;
      currentIteration = Math.floor(overall);
      const simple = overall - currentIteration;
      const rev = this.direction === "reverse" ||
        (this.direction === "alternate" && currentIteration % 2 === 1) ||
        (this.direction === "alternate-reverse" && currentIteration % 2 === 0);
      const directed = rev ? 1 - simple : simple;
      progress = evaluateAnimationEasing(this.easing, directed);
    }
  }

  return {
    delay: this.delay,
    endDelay: this.endDelay,
    fill: this.fill,
    iterationStart: this.iterationStart,
    iterations: this.iterations,
    duration: this.duration,
    direction: this.direction,
    easing: this.easing,
    endTime: endTime,
    activeDuration: activeDuration,
    localTime: localTime,
    currentIteration: currentIteration,
    progress: progress,
  };
};

KeyframeEffect.prototype.getKeyframes = function () {
  return this.keyframes.slice();
};

markNative(KeyframeEffect, "KeyframeEffect");
markNative(KeyframeEffect.prototype.getComputedTiming, "getComputedTiming");
markNative(KeyframeEffect.prototype.getKeyframes, "getKeyframes");

const registeredAnimations = new Set();
let _flushCssAnimHook = null;

function Animation(effect, timeline) {
  EventTarget.call(this);
  this.id = "";
  this._effect = null;
  this.timeline = timeline || globalDocTimeline;
  this._playbackRate = 1.0;
  this.startTime = null;
  this._holdTime = null;
  this._finishTimer = null;
  this.pending = false;
  this.playState = "idle";
  this.onfinish = null;
  this.oncancel = null;
  this.onremove = null;

  const self = this;
  this._readyResolve = null;
  this._finishedResolve = null;
  this.ready = Promise.resolve(this);
  this.finished = new Promise(function (res) { self._finishedResolve = res; });

  if (effect) {
    this.effect = effect;
  }
  registeredAnimations.add(this);
}
Animation.prototype = Object.create(EventTarget.prototype);
Animation.prototype.constructor = Animation;

Object.defineProperty(Animation.prototype, "playbackRate", {
  get: function () {
    return this._playbackRate;
  },
  set: function (val) {
    if (this._playbackRate === val) return;
    this.updatePlaybackRate(val);
  },
  configurable: true,
});

Object.defineProperty(Animation.prototype, "effect", {
  get: function () {
    return this._effect;
  },
  set: function (newEffect) {
    if (this._effect && this._effect._animation === this) {
      this._effect._animation = null;
    }
    this._effect = newEffect || null;
    if (this._effect) {
      this._effect._animation = this;
    }
  },
  configurable: true,
});

Object.defineProperty(Animation.prototype, "currentTime", {
  get: function () {
    if (this._holdTime !== null) {
      return this._holdTime;
    }
    if (this.startTime === null || !this.timeline) {
      return null;
    }
    const elapsed = (this.timeline.currentTime - this.startTime) * this.playbackRate;
    const endTime = this._getEndTime();
    if (this.playState === "running" && isFinite(endTime) && this.playbackRate > 0 && elapsed >= endTime) {
      this.finish();
      return this._holdTime;
    }
    return elapsed;
  },
  set: function (val) {
    if (val === null || val === undefined) {
      this._holdTime = null;
      this.startTime = null;
      this._clearFinishTimer();
      return;
    }
    const t = Number(val);
    if (isNaN(t)) throw new TypeError("currentTime must be a valid number or null.");
    if (this.playState === "running") {
      this._holdTime = null;
      this.startTime = this.timeline.currentTime - t / (this.playbackRate || 1);
      this._scheduleFinish();
    } else {
      this._holdTime = t;
    }
  },
  configurable: true,
});

Animation.prototype._getEndTime = function () {
  if (!this._effect) return 0;
  const dur = typeof this._effect.duration === "number" ? this._effect.duration : 0;
  const iters = this._effect.iterations;
  const active = iters === 0 ? 0 : (iters === Infinity ? (dur > 0 ? Infinity : 0) : dur * iters);
  return Math.max(0, (this._effect.delay || 0) + active + (this._effect.endDelay || 0));
};

Animation.prototype._clearFinishTimer = function () {
  if (this._finishTimer !== null) {
    try { clearTimeout(this._finishTimer); } catch (e) {}
    this._finishTimer = null;
  }
};

Animation.prototype._scheduleFinish = function () {
  this._clearFinishTimer();
  if (this.playState !== "running" || this.playbackRate <= 0) return;
  const endTime = this._getEndTime();
  if (!isFinite(endTime)) return;
  const cur = this.startTime !== null ? (this.timeline.currentTime - this.startTime) * this.playbackRate : 0;
  const remaining = Math.max(0, (endTime - cur) / this.playbackRate);
  const self = this;
  if (typeof setTimeout === "function") {
    this._finishTimer = setTimeout(function () {
      self._finishTimer = null;
      if (self.playState === "running") {
        self.finish();
      }
    }, Math.ceil(remaining));
  }
};

Animation.prototype.play = function () {
  const prevTime = this._holdTime !== null ? this._holdTime : 0;
  this.playState = "running";
  this._holdTime = null;
  this.startTime = this.timeline.currentTime - prevTime / (this.playbackRate || 1);
  registeredAnimations.add(this);
  if (this._readyResolve) {
    this._readyResolve(this);
    this._readyResolve = null;
  }
  this._scheduleFinish();
};

Animation.prototype.pause = function () {
  if (this.playState === "running") {
    const cur = this.startTime !== null ? (this.timeline.currentTime - this.startTime) * this.playbackRate : 0;
    this._holdTime = cur;
    this.startTime = null;
    this._clearFinishTimer();
    this.playState = "paused";
  } else if (this.playState !== "idle") {
    this.playState = "paused";
  }
};

function _dispatchCssLifecycleEvent(anim, transitionType, animationType, elapsedMs) {
  const target = anim.effect && anim.effect.target;
  if (!target) return;
  const elapsedSec = (Number(elapsedMs) || 0) / 1000;
  const pseudo = (anim.effect && anim.effect.pseudoElement) || "";
  const dispatch = function (ev) {
    try {
      if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_dispatchTrustedEvent === "function") {
        globalThis.__capsurf_dispatchTrustedEvent(target, ev);
      } else if (typeof target.dispatchEvent === "function") {
        target.dispatchEvent(ev);
      }
    } catch (e) {}
  };
  if (anim._transitionProperty) {
    let ev;
    if (typeof TransitionEvent === "function") {
      ev = new TransitionEvent(transitionType, {
        bubbles: true,
        cancelable: false,
        propertyName: String(anim._transitionProperty),
        elapsedTime: elapsedSec,
        pseudoElement: pseudo,
      });
    } else {
      ev = new Event(transitionType, { bubbles: true, cancelable: false });
      try {
        ev.propertyName = String(anim._transitionProperty);
        ev.elapsedTime = elapsedSec;
        ev.pseudoElement = pseudo;
      } catch (e) {}
    }
    dispatch(ev);
  }
  if (anim._animationName) {
    let ev;
    if (typeof AnimationEvent === "function") {
      ev = new AnimationEvent(animationType, {
        bubbles: true,
        cancelable: false,
        animationName: String(anim._animationName),
        elapsedTime: elapsedSec,
        pseudoElement: pseudo,
      });
    } else {
      ev = new Event(animationType, { bubbles: true, cancelable: false });
      try {
        ev.animationName = String(anim._animationName);
        ev.elapsedTime = elapsedSec;
        ev.pseudoElement = pseudo;
      } catch (e) {}
    }
    dispatch(ev);
  }
}

Animation.prototype.finish = function () {
  this._clearFinishTimer();
  const endTime = this._getEndTime();
  this.playState = "finished";
  this._holdTime = isFinite(endTime) ? endTime : 0;
  this.startTime = null;
  if (typeof this._onCssFinish === "function") {
    try { this._onCssFinish(); } catch (e) {}
  }

  if (this._finishedResolve) {
    const res = this._finishedResolve;
    this._finishedResolve = null;
    res(this);
  }

  const evt = new Event("finish");
  try {
    evt.currentTime = this._holdTime;
    evt.timelineTime = this.timeline ? this.timeline.currentTime : 0;
  } catch (e) {}
  this.dispatchEvent(evt);
  let cssElapsedMs = this._holdTime;
  if (this._effect && (this._transitionProperty || this._animationName)) {
    const dur = typeof this._effect.duration === "number" ? this._effect.duration : 0;
    const iters = this._effect.iterations;
    cssElapsedMs = (iters === 0 || !isFinite(iters)) ? 0 : dur * iters;
  }
  _dispatchCssLifecycleEvent(this, "transitionend", "animationend", cssElapsedMs);
};

Animation.prototype.cancel = function () {
  let cancelElapsedMs = 0;
  if (this._holdTime !== null) {
    cancelElapsedMs = this._holdTime;
  } else if (this.startTime !== null && this.timeline) {
    cancelElapsedMs = (this.timeline.currentTime - this.startTime) * this.playbackRate;
  }
  if (cancelElapsedMs < 0 || !isFinite(cancelElapsedMs)) cancelElapsedMs = 0;

  this._clearFinishTimer();
  this.playState = "idle";
  this._holdTime = null;
  this.startTime = null;
  registeredAnimations.delete(this);
  if (typeof this._onCssCancel === "function") {
    try { this._onCssCancel(); } catch (e) {}
  }
  const evt = new Event("cancel");
  try {
    evt.currentTime = null;
    evt.timelineTime = this.timeline ? this.timeline.currentTime : 0;
  } catch (e) {}
  this.dispatchEvent(evt);
  const delayMs = (this._effect && this._effect.delay) ? this._effect.delay : 0;
  const cssCancelElapsedMs = Math.max(0, cancelElapsedMs - delayMs);
  _dispatchCssLifecycleEvent(this, "transitioncancel", "animationcancel", cssCancelElapsedMs);
};

Animation.prototype.updatePlaybackRate = function (rate) {
  const r = Number(rate);
  if (!isFinite(r)) {
    throw new TypeError("playbackRate must be a finite number.");
  }
  if (this.playState === "running" && this.timeline && this.startTime !== null) {
    const prevTime = (this.timeline.currentTime - this.startTime) * this._playbackRate;
    this._playbackRate = r;
    if (r === 0) {
      this._holdTime = prevTime;
      this.startTime = null;
      this._clearFinishTimer();
    } else {
      this._holdTime = null;
      this.startTime = this.timeline.currentTime - prevTime / r;
      this._scheduleFinish();
    }
  } else {
    this._playbackRate = r;
  }
};

Animation.prototype.reverse = function () {
  this.updatePlaybackRate(-this._playbackRate);
  this.play();
};

function CSSTransition(effect, timeline, transitionProperty) {
  Animation.call(this, effect, timeline);
  this.transitionProperty = String(transitionProperty || "");
  this._transitionProperty = this.transitionProperty;
}
CSSTransition.prototype = Object.create(Animation.prototype);
CSSTransition.prototype.constructor = CSSTransition;
markNative(CSSTransition, "CSSTransition");

function CSSAnimation(effect, timeline, animationName) {
  Animation.call(this, effect, timeline);
  this.animationName = String(animationName || "");
  this._animationName = this.animationName;
  this.id = this.animationName;
}
CSSAnimation.prototype = Object.create(Animation.prototype);
CSSAnimation.prototype.constructor = CSSAnimation;
markNative(CSSAnimation, "CSSAnimation");

if (typeof globalThis !== "undefined") {
  globalThis.CSSTransition = CSSTransition;
  globalThis.CSSAnimation = CSSAnimation;
}

markNative(Animation, "Animation");
markNative(Animation.prototype.play, "play");
markNative(Animation.prototype.pause, "pause");
markNative(Animation.prototype.finish, "finish");
markNative(Animation.prototype.cancel, "cancel");
markNative(Animation.prototype.updatePlaybackRate, "updatePlaybackRate");

function animateElement(element, keyframes, options) {
  const effect = new KeyframeEffect(element, keyframes, options);
  const anim = new Animation(effect, globalDocTimeline);
  anim.play();
  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_domMutationEpoch = (globalThis.__capsurf_domMutationEpoch || 0) + 1;
  }
  return anim;
}

function isAnimationRelevant(anim) {
  if (!anim || !anim.effect || !anim.effect.target) return false;
  // Trigger lazy finish check if running
  if (anim.playState === "running") {
    void anim.currentTime;
  }
  if (anim.playState === "idle") return false;
  if (anim.playState === "finished") {
    const f = anim.effect.fill;
    return f === "forwards" || f === "both";
  }
  return true;
}

function animationsFor(root) {
  if (typeof _flushCssAnimHook === "function") {
    _flushCssAnimHook();
  }
  const res = [];
  for (const anim of registeredAnimations) {
    if (!isAnimationRelevant(anim)) continue;
    if (!root || root === anim.effect.target || (root.contains && root.contains(anim.effect.target))) {
      res.push(anim);
    }
  }
  return res;
}

function computeAnimatedStylesForElement(element, pseudoElt) {
  const result = {};
  if (!element) return result;
  const wantPseudo = String(pseudoElt || "").toLowerCase();
  for (const anim of registeredAnimations) {
    if (!anim || !anim.effect || anim.effect.target !== element) continue;
    if (anim._transitionProperty || anim._animationName) continue;
    const animPseudo = String(anim.effect.pseudoElement || "").toLowerCase();
    if (animPseudo !== wantPseudo) continue;
    if (anim.playState === "idle") continue;
    const effect = anim.effect;
    const frames = effect.keyframes;
    if (!frames || frames.length === 0) continue;
    const timing = effect.getComputedTiming();
    const effectProgress = timing.progress;
    if (effectProgress === null || effectProgress === undefined) continue;

    if (frames.length === 1) {
      const f = frames[0];
      for (const k in f) {
        if (k !== "offset" && k !== "easing" && k !== "composite") {
          const kebab = k.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
          result[kebab] = String(f[k]);
        }
      }
      continue;
    }

    // Assign computed offsets if missing (linearly distribute between specified offsets)
    const offsets = frames.map(function (f) {
      return (f.offset != null && !isNaN(Number(f.offset))) ? Number(f.offset) : null;
    });
    if (offsets[0] === null) offsets[0] = 0;
    if (offsets[offsets.length - 1] === null) offsets[offsets.length - 1] = 1;
    let lastKnown = 0;
    for (let i = 1; i < offsets.length; i++) {
      if (offsets[i] !== null) {
        const gap = i - lastKnown;
        if (gap > 1) {
          const startOff = offsets[lastKnown];
          const endOff = offsets[i];
          for (let j = 1; j < gap; j++) {
            offsets[lastKnown + j] = startOff + (endOff - startOff) * (j / gap);
          }
        }
        lastKnown = i;
      }
    }

    let idx = 0;
    for (let i = 0; i < offsets.length - 1; i++) {
      if (effectProgress >= offsets[i] && effectProgress <= offsets[i + 1]) {
        idx = i;
        break;
      }
      if (effectProgress > offsets[i + 1]) idx = i;
    }
    const f0 = frames[idx];
    const f1 = frames[idx + 1] || f0;
    const span = offsets[idx + 1] - offsets[idx];
    const segT = span > 0 ? (effectProgress - offsets[idx]) / span : 1;
    const easedP = evaluateAnimationEasing(f0.easing || "linear", segT);

    const props = new Set();
    for (const k in f0) if (k !== "offset" && k !== "easing" && k !== "composite") props.add(k);
    for (const k in f1) if (k !== "offset" && k !== "easing" && k !== "composite") props.add(k);

    for (const prop of props) {
      const kebab = prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
      if (f0[prop] !== undefined && f1[prop] !== undefined) {
        result[kebab] = String(interpolateAnimValue(kebab, f0[prop], f1[prop], easedP));
      } else if (f1[prop] !== undefined) {
        result[kebab] = String(f1[prop]);
      } else if (f0[prop] !== undefined) {
        result[kebab] = String(f0[prop]);
      }
    }
  }
  return result;
}

if (typeof globalThis !== "undefined") {
  globalThis.__capsurf_getAnimatedStyles = computeAnimatedStylesForElement;
}

// =========================================================================
// CSS Transitions & CSS @keyframes Animations Engine
// =========================================================================
(function () {
  const _CapturedTransitionEvent = typeof TransitionEvent === "function" ? TransitionEvent : null;
  const _CapturedAnimationEvent = typeof AnimationEvent === "function" ? AnimationEvent : null;

  const _cssAnimStateMap = new WeakMap();
  const _activeCssAnimElements = new Set();
  const _pendingStyleCheckElements = new Set();
  let _pendingStyleCheckScheduled = false;
  let _snapshotsInitialized = false;

  // Freeze sampled time within a single synchronous JS turn so repeated getComputedStyle()
  // calls during synchronous style mutations observe a consistent animation clock.
  let _cssAnimTurnNow = null;
  function getCssAnimNow() {
    if (_cssAnimTurnNow === null) {
      _cssAnimTurnNow = (typeof performance !== "undefined" && performance.now) ? performance.now() : Date.now();
      Promise.resolve().then(function () {
        _cssAnimTurnNow = null;
      });
    }
    return _cssAnimTurnNow;
  }

  function dispatchCssTrustedEvent(el, ev) {
    if (!el || !ev) return;
    try {
      if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_dispatchTrustedEvent === "function") {
        globalThis.__capsurf_dispatchTrustedEvent(el, ev);
      } else if (typeof el.dispatchEvent === "function") {
        el.dispatchEvent(ev);
      }
    } catch (e) {}
  }

  function fireTransitionEvent(el, type, propertyName, elapsedTimeSec) {
    if (!_CapturedTransitionEvent) return;
    const ev = new _CapturedTransitionEvent(type, {
      bubbles: true,
      cancelable: false,
      propertyName: propertyName,
      elapsedTime: Number(elapsedTimeSec) || 0,
      pseudoElement: "",
    });
    dispatchCssTrustedEvent(el, ev);
  }

  function fireAnimationEvent(el, type, animationName, elapsedTimeSec) {
    if (!_CapturedAnimationEvent) return;
    const ev = new _CapturedAnimationEvent(type, {
      bubbles: true,
      cancelable: false,
      animationName: animationName,
      elapsedTime: Number(elapsedTimeSec) || 0,
      pseudoElement: "",
    });
    dispatchCssTrustedEvent(el, ev);
  }

  function parseCssTimeMs(token) {
    if (typeof token !== "string") return null;
    const t = token.trim();
    const m = t.match(/^([+-]?(?:\d+\.?\d*|\.\d+))(ms|s)$/i);
    if (m) {
      const val = parseFloat(m[1]);
      if (isNaN(val)) return null;
      return m[2].toLowerCase() === "s" ? val * 1000 : val;
    }
    const cm = t.match(/^calc\(\s*(.+)\s*\)$/i);
    if (cm) {
      const e = cm[1]
        .replace(/([+-]?(?:\d+\.?\d*|\.\d+))ms\b/gi, "($1)")
        .replace(/([+-]?(?:\d+\.?\d*|\.\d+))s\b/gi, "($1*1000)");
      if (/^[\d+\-*/().\s]+$/.test(e)) {
        try {
          const v = Function("return (" + e + ")")();
          if (typeof v === "number" && isFinite(v)) return v;
        } catch (_) {}
      }
    }
    return null;
  }

  function splitTopLevelCommas(str) {
    const out = [];
    let cur = "";
    let depth = 0;
    for (let i = 0; i < str.length; i++) {
      const ch = str[i];
      if (ch === "(") depth++;
      else if (ch === ")" && depth > 0) depth--;
      if (ch === "," && depth === 0) {
        out.push(cur.trim());
        cur = "";
      } else {
        cur += ch;
      }
    }
    if (cur.trim()) out.push(cur.trim());
    return out;
  }

  function splitTopLevelSpaces(str) {
    const out = [];
    let cur = "";
    let depth = 0;
    for (let i = 0; i < str.length; i++) {
      const ch = str[i];
      if (ch === "(") depth++;
      else if (ch === ")" && depth > 0) depth--;
      if (/\s/.test(ch) && depth === 0) {
        if (cur) { out.push(cur); cur = ""; }
      } else {
        cur += ch;
      }
    }
    if (cur) out.push(cur);
    return out;
  }

  const TIMING_KEYWORDS = new Set(["linear", "ease", "ease-in", "ease-out", "ease-in-out", "step-start", "step-end"]);
  function isTimingFunction(tok) {
    const l = tok.toLowerCase();
    return TIMING_KEYWORDS.has(l) || l.startsWith("cubic-bezier(") || l.startsWith("steps(");
  }

  const FALLBACK_TRANSITION_SHORTHANDS = {
    margin: ["margin-top", "margin-right", "margin-bottom", "margin-left"],
    padding: ["padding-top", "padding-right", "padding-bottom", "padding-left"],
    "border-width": ["border-top-width", "border-right-width", "border-bottom-width", "border-left-width"],
    "border-color": ["border-top-color", "border-right-color", "border-bottom-color", "border-left-color"],
    "border-style": ["border-top-style", "border-right-style", "border-bottom-style", "border-left-style"],
    "border-radius": ["border-top-left-radius", "border-top-right-radius", "border-bottom-right-radius", "border-bottom-left-radius"],
    inset: ["top", "right", "bottom", "left"],
    gap: ["row-gap", "column-gap"],
    "grid-gap": ["row-gap", "column-gap"],
    overflow: ["overflow-x", "overflow-y"],
    outline: ["outline-color", "outline-style", "outline-width"],
    flex: ["flex-grow", "flex-shrink", "flex-basis"],
  };

  function expandTransitionProp(prop) {
    const shMap = (typeof globalThis !== "undefined" && globalThis.__capsurf_shorthandLonghands) || null;
    if (shMap && Object.prototype.hasOwnProperty.call(shMap, prop)) {
      const lhs = shMap[prop];
      if (Array.isArray(lhs) && lhs.length > 0) return lhs;
    }
    if (Object.prototype.hasOwnProperty.call(FALLBACK_TRANSITION_SHORTHANDS, prop)) {
      return FALLBACK_TRANSITION_SHORTHANDS[prop];
    }
    return [prop];
  }

  function parseTransitionSpecs(computedMap) {
    const specs = [];
    const shorthand = computedMap.get("transition");
    if (shorthand && String(shorthand).trim() !== "none") {
      const items = splitTopLevelCommas(String(shorthand));
      for (let i = 0; i < items.length; i++) {
        const toks = splitTopLevelSpaces(items[i]);
        let prop = "all";
        let durationMs = 0;
        let delayMs = 0;
        let easing = "ease";
        let timeIdx = 0;
        for (let j = 0; j < toks.length; j++) {
          const t = toks[j];
          const ms = parseCssTimeMs(t);
          if (ms !== null) {
            if (timeIdx === 0) durationMs = ms;
            else if (timeIdx === 1) delayMs = ms;
            timeIdx++;
          } else if (isTimingFunction(t)) {
            easing = t.toLowerCase();
          } else {
            prop = t.toLowerCase();
          }
        }
        if (prop !== "none" && durationMs > 0) {
          specs.push({ prop: prop, durationMs: durationMs, delayMs: delayMs, easing: easing });
        }
      }
      return specs;
    }

    const propStr = computedMap.get("transition-property");
    const durStr = computedMap.get("transition-duration");
    if (!durStr) return specs;
    const props = propStr ? splitTopLevelCommas(String(propStr)) : ["all"];
    const durs = splitTopLevelCommas(String(durStr));
    const delays = computedMap.get("transition-delay") ? splitTopLevelCommas(String(computedMap.get("transition-delay"))) : ["0s"];
    const easings = computedMap.get("transition-timing-function") ? splitTopLevelCommas(String(computedMap.get("transition-timing-function"))) : ["ease"];
    for (let i = 0; i < props.length; i++) {
      const p = props[i].toLowerCase();
      const d = parseCssTimeMs(durs[i % durs.length]) || 0;
      const dl = parseCssTimeMs(delays[i % delays.length]) || 0;
      const ea = (easings[i % easings.length] || "ease").toLowerCase();
      if (p !== "none" && d > 0) {
        specs.push({ prop: p, durationMs: d, delayMs: dl, easing: ea });
      }
    }
    return specs;
  }

  const ANIM_DIRECTIONS = new Set(["normal", "reverse", "alternate", "alternate-reverse"]);
  const ANIM_FILLS = new Set(["none", "forwards", "backwards", "both"]);
  const ANIM_PLAY_STATES = new Set(["running", "paused"]);

  function parseAnimationSpecs(computedMap) {
    const specs = [];
    const shorthand = computedMap.get("animation");
    if (shorthand && String(shorthand).trim() !== "none") {
      const items = splitTopLevelCommas(String(shorthand));
      for (let i = 0; i < items.length; i++) {
        const toks = splitTopLevelSpaces(items[i]);
        let name = "none";
        let durationMs = 0;
        let delayMs = 0;
        let easing = "ease";
        let iterations = 1;
        let direction = "normal";
        let fill = "none";
        let playState = "running";
        let timeIdx = 0;
        for (let j = 0; j < toks.length; j++) {
          const t = toks[j];
          const tl = t.toLowerCase();
          const ms = parseCssTimeMs(t);
          if (ms !== null) {
            if (timeIdx === 0) durationMs = ms;
            else if (timeIdx === 1) delayMs = ms;
            timeIdx++;
          } else if (isTimingFunction(t)) {
            easing = tl;
          } else if (tl === "infinite") {
            iterations = Infinity;
          } else if (/^\d+(\.\d+)?$/.test(t)) {
            iterations = parseFloat(t);
          } else if (ANIM_DIRECTIONS.has(tl)) {
            direction = tl;
          } else if (ANIM_FILLS.has(tl)) {
            fill = tl;
          } else if (ANIM_PLAY_STATES.has(tl)) {
            playState = tl;
          } else {
            name = t;
          }
        }
        if (computedMap.has("animation-delay")) {
          const dlOv = splitTopLevelCommas(String(computedMap.get("animation-delay")));
          const dlMs = parseCssTimeMs(dlOv[i % dlOv.length]);
          if (dlMs !== null) delayMs = dlMs;
        }
        if (computedMap.has("animation-play-state")) {
          const psOv = splitTopLevelCommas(String(computedMap.get("animation-play-state")));
          const psVal = (psOv[i % psOv.length] || "").trim().toLowerCase();
          if (ANIM_PLAY_STATES.has(psVal)) playState = psVal;
        }
        if (name && name !== "none" && durationMs > 0) {
          specs.push({
            name: name,
            durationMs: durationMs,
            delayMs: delayMs,
            easing: easing,
            iterations: iterations,
            direction: direction,
            fill: fill,
            playState: playState,
          });
        }
      }
      return specs;
    }

    const nameStr = computedMap.get("animation-name");
    const durStr = computedMap.get("animation-duration");
    if (!nameStr || !durStr) return specs;
    const names = splitTopLevelCommas(String(nameStr));
    const durs = splitTopLevelCommas(String(durStr));
    const delays = computedMap.get("animation-delay") ? splitTopLevelCommas(String(computedMap.get("animation-delay"))) : ["0s"];
    const easings = computedMap.get("animation-timing-function") ? splitTopLevelCommas(String(computedMap.get("animation-timing-function"))) : ["ease"];
    const iters = computedMap.get("animation-iteration-count") ? splitTopLevelCommas(String(computedMap.get("animation-iteration-count"))) : ["1"];
    const dirs = computedMap.get("animation-direction") ? splitTopLevelCommas(String(computedMap.get("animation-direction"))) : ["normal"];
    const fills = computedMap.get("animation-fill-mode") ? splitTopLevelCommas(String(computedMap.get("animation-fill-mode"))) : ["none"];
    for (let i = 0; i < names.length; i++) {
      const nm = names[i];
      const d = parseCssTimeMs(durs[i % durs.length]) || 0;
      const dl = parseCssTimeMs(delays[i % delays.length]) || 0;
      const ea = (easings[i % easings.length] || "ease").toLowerCase();
      const itRaw = iters[i % iters.length];
      const it = itRaw === "infinite" ? Infinity : (parseFloat(itRaw) || 1);
      if (nm && nm !== "none" && d > 0) {
        specs.push({
          name: nm,
          durationMs: d,
          delayMs: dl,
          easing: ea,
          iterations: it,
          direction: (dirs[i % dirs.length] || "normal").toLowerCase(),
          fill: (fills[i % fills.length] || "none").toLowerCase(),
        });
      }
    }
    return specs;
  }

  function extractKeyframesMap(styleNodes) {
    const map = new Map();
    if (!styleNodes || !styleNodes.length) return map;
    const kfRe = /@(?:-(?:webkit|moz)-)?keyframes\s+([^\s{]+)\s*\{([\s\S]*?)\}\s*(?=\}|@|[a-zA-Z0-9_.#*:\[\s]|$)/gi;
    for (let s = 0; s < styleNodes.length; s++) {
      const css = String(styleNodes[s] && styleNodes[s].textContent ? styleNodes[s].textContent : "");
      if (css.indexOf("keyframes") === -1) continue;
      // Balanced brace extraction for @keyframes blocks
      const headerRe = /@(?:-(?:webkit|moz)-)?keyframes\s+([^\s{]+)\s*\{/gi;
      let hm;
      while ((hm = headerRe.exec(css)) !== null) {
        const kfName = hm[1].trim();
        let depth = 1;
        let idx = headerRe.lastIndex;
        const bodyStart = idx;
        while (idx < css.length && depth > 0) {
          if (css[idx] === "{") depth++;
          else if (css[idx] === "}") depth--;
          idx++;
        }
        const body = css.slice(bodyStart, idx - 1);
        const frames = [];
        const frameRe = /([^{}]+)\{([^{}]*)\}/g;
        let fm;
        while ((fm = frameRe.exec(body)) !== null) {
          const sels = fm[1].split(",").map(function (x) { return x.trim().toLowerCase(); }).filter(Boolean);
          const declText = fm[2];
          const props = {};
          const declParts = declText.split(";");
          for (let di = 0; di < declParts.length; di++) {
            const col = declParts[di].indexOf(":");
            if (col === -1) continue;
            const k = declParts[di].slice(0, col).trim().toLowerCase();
            const v = declParts[di].slice(col + 1).replace(/!important/i, "").trim();
            if (k && v) {
              const exp = (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_expandShorthand === "function") ? globalThis.__capsurf_expandShorthand(k, v) : null;
              if (Array.isArray(exp) && exp.length > 0) {
                for (let ei = 0; ei < exp.length; ei++) {
                  const pair = exp[ei];
                  if (pair && pair[0] && pair[1] !== undefined) props[pair[0]] = String(pair[1]);
                }
              } else {
                props[k] = v;
              }
            }
          }
          for (let si = 0; si < sels.length; si++) {
            let offset = null;
            if (sels[si] === "from") offset = 0;
            else if (sels[si] === "to") offset = 1;
            else if (sels[si].endsWith("%")) offset = parseFloat(sels[si]) / 100;
            if (offset !== null && !isNaN(offset)) {
              frames.push({ offset: offset, props: props });
            }
          }
        }
        frames.sort(function (a, b) { return a.offset - b.offset; });
        if (frames.length > 0) {
          map.set(kfName, frames);
        }
      }
    }
    return map;
  }

  function kebabToCamelCase(prop) {
    return String(prop).replace(/-([a-z])/g, function (_, c) { return c.toUpperCase(); });
  }

  function safeEasingForEffect(easing) {
    try { return validateEasingString(easing); } catch (e) { return "ease"; }
  }

  function evalTransitionValue(tr, now) {
    let activeMs;
    if (tr.anim) {
      const curTime = tr.anim.currentTime;
      if (curTime === null) return tr.toVal;
      activeMs = curTime - tr.delayMs;
    } else {
      activeMs = (now - tr.startTime) - tr.delayMs;
    }
    if (activeMs <= 0) return tr.fromVal;
    if (activeMs >= tr.durationMs) return tr.toVal;
    const rawT = activeMs / tr.durationMs;
    const easedT = evaluateAnimationEasing(tr.easing, rawT);
    return interpolateAnimValue(tr.prop, tr.fromVal, tr.toVal, easedT);
  }

  function evalCssAnimationStyles(anim, frames, now, baseStyles) {
    const out = {};
    if (!frames || frames.length === 0) return out;
    let localTime;
    if (anim.anim) {
      if (anim.anim.playState === "idle") return out;
      const curTime = anim.anim.currentTime;
      if (curTime === null) return out;
      localTime = curTime;
    } else {
      localTime = anim.playState === "paused" ? 0 : (now - anim.startTime);
    }
    const activeMs = localTime - anim.delayMs;
    const totalMs = anim.iterations === Infinity ? Infinity : anim.durationMs * anim.iterations;
    if (activeMs < 0 && anim.fill !== "backwards" && anim.fill !== "both") return out;
    if (activeMs >= totalMs && anim.fill !== "forwards" && anim.fill !== "both") return out;

    let progress;
    if (activeMs <= 0) {
      progress = (anim.direction === "reverse" || anim.direction === "alternate-reverse") ? 1 : 0;
    } else if (activeMs >= totalMs) {
      if (anim.iterations === 0) {
        const rev0 = anim.direction === "reverse" || anim.direction === "alternate-reverse";
        progress = rev0 ? 1 : 0;
      } else {
        const frac = anim.iterations % 1;
        const baseP = frac === 0 ? 1 : frac;
        const lastIter = Math.max(0, Math.ceil(anim.iterations) - 1);
        const rev = anim.direction === "reverse" ||
          (anim.direction === "alternate" && (lastIter % 2) === 1) ||
          (anim.direction === "alternate-reverse" && (lastIter % 2) === 0);
        progress = rev ? (1 - baseP) : baseP;
      }
    } else {
      const iterIdx = Math.floor(activeMs / anim.durationMs);
      progress = (activeMs % anim.durationMs) / anim.durationMs;
      if (
        anim.direction === "reverse" ||
        (anim.direction === "alternate" && (iterIdx % 2) === 1) ||
        (anim.direction === "alternate-reverse" && (iterIdx % 2) === 0)
      ) {
        progress = 1 - progress;
      }
    }
    const easedP = evaluateAnimationEasing(anim.easing, progress);

    const allProps = new Set();
    for (let i = 0; i < frames.length; i++) {
      for (const k in frames[i].props) allProps.add(k);
    }

    allProps.forEach(function (prop) {
      // Collect keyframes that define this property
      const propFrames = [];
      for (let i = 0; i < frames.length; i++) {
        if (frames[i].props[prop] !== undefined) {
          propFrames.push({ offset: frames[i].offset, val: frames[i].props[prop] });
        }
      }
      if (propFrames.length === 0) return;
      let baseVal = baseStyles && typeof baseStyles.get === "function" ? baseStyles.get(prop) : undefined;
      if (baseVal === undefined && prop === "background-color" && baseStyles && typeof baseStyles.get === "function") {
        const bg = baseStyles.get("background");
        if (bg !== undefined && parseAnimColor(bg)) {
          baseVal = bg;
        }
      }
      if (propFrames[0].offset > 0 && baseVal !== undefined) {
        propFrames.unshift({ offset: 0, val: baseVal });
      }
      if (propFrames[propFrames.length - 1].offset < 1 && baseVal !== undefined) {
        propFrames.push({ offset: 1, val: baseVal });
      }
      if (propFrames.length === 1) {
        out[prop] = propFrames[0].val;
        return;
      }
      let idx = 0;
      for (let i = 0; i < propFrames.length - 1; i++) {
        if (easedP >= propFrames[i].offset && easedP <= propFrames[i + 1].offset) {
          idx = i;
          break;
        }
        if (easedP > propFrames[i + 1].offset) idx = i;
      }
      const f0 = propFrames[idx];
      const f1 = propFrames[idx + 1] || f0;
      const span = f1.offset - f0.offset;
      const segT = span > 0 ? Math.max(0, Math.min(1, (easedP - f0.offset) / span)) : (easedP >= f1.offset ? 1 : 0);
      out[prop] = interpolateAnimValue(prop, f0.val, f1.val, segT);
    });
    return out;
  }

  function isNonAnimatableMetaProp(prop) {
    return prop.startsWith("transition") || prop.startsWith("animation");
  }

  function getOrCreateState(el) {
    let st = _cssAnimStateMap.get(el);
    if (!st) {
      st = {
        lastStyle: null,
        transitions: new Map(),
        animations: new Map(),
      };
      _cssAnimStateMap.set(el, st);
    }
    return st;
  }

  function syncCssTransitionsAndAnimations(element, computedMap, styleNodes) {
    if (!element || !computedMap) return;
    const st = getOrCreateState(element);
    const now = getCssAnimNow();

    // Snapshot of current static (non-animated) styles before overlaying transitions/animations
    const staticSnapshot = new Map(computedMap);

    // 1. Handle CSS Transitions
    const trSpecs = parseTransitionSpecs(staticSnapshot);
    if (st.lastStyle !== null && trSpecs.length > 0) {
      // Build property -> spec map (later spec wins; "all" acts as wildcard)
      let allSpec = null;
      const specByProp = new Map();
      for (let i = 0; i < trSpecs.length; i++) {
        if (trSpecs[i].prop === "all") allSpec = trSpecs[i];
        else {
          const expanded = expandTransitionProp(trSpecs[i].prop);
          for (let j = 0; j < expanded.length; j++) {
            specByProp.set(expanded[j], trSpecs[i]);
          }
        }
      }

      const candidateProps = new Set();
      if (allSpec) {
        staticSnapshot.forEach(function (_v, k) { if (!isNonAnimatableMetaProp(k)) candidateProps.add(k); });
        st.lastStyle.forEach(function (_v, k) { if (!isNonAnimatableMetaProp(k)) candidateProps.add(k); });
      } else {
        specByProp.forEach(function (_v, k) { candidateProps.add(k); });
      }

      candidateProps.forEach(function (prop) {
        const spec = specByProp.get(prop) || allSpec;
        if (!spec || spec.durationMs <= 0) return;
        const prevVal = st.lastStyle.get(prop);
        const nextVal = staticSnapshot.get(prop);
        if (prevVal === undefined || nextVal === undefined || prevVal === nextVal) return;

        let startFromVal = prevVal;
        const existing = st.transitions.get(prop);
        if (existing) {
          if (existing.toVal === nextVal) return;
          // Retarget: cancel existing transition and start from its current interpolated value
          startFromVal = evalTransitionValue(existing, now);
          if (existing.startTimer !== null) {
            try { clearTimeout(existing.startTimer); } catch (e) {}
            existing.startTimer = null;
          }
          st.transitions.delete(prop);
          if (existing.anim) {
            existing.anim.cancel();
          } else {
            const cancelElapsed = Math.max(0, Math.min(existing.durationMs, (now - existing.startTime) - existing.delayMs)) / 1000;
            fireTransitionEvent(element, "transitioncancel", prop, cancelElapsed);
          }
        }

        const kfProp = kebabToCamelCase(prop);
        const f0 = {};
        f0[kfProp] = startFromVal;
        const f1 = {};
        f1[kfProp] = nextVal;
        const trEffect = new KeyframeEffect(element, [f0, f1], {
          duration: spec.durationMs,
          delay: spec.delayMs,
          easing: safeEasingForEffect(spec.easing),
          fill: "backwards",
        });
        const trAnim = new CSSTransition(trEffect, globalDocTimeline, prop);

        const tr = {
          prop: prop,
          fromVal: startFromVal,
          toVal: nextVal,
          startTime: now,
          durationMs: spec.durationMs,
          delayMs: spec.delayMs,
          easing: spec.easing,
          startTimer: null,
          anim: trAnim,
        };
        trAnim._onCssFinish = function () {
          if (tr.startTimer !== null) {
            try { clearTimeout(tr.startTimer); } catch (e) {}
            tr.startTimer = null;
          }
          if (st.transitions.get(prop) === tr) {
            st.transitions.delete(prop);
            if (st.transitions.size === 0 && st.animations.size === 0) {
              _activeCssAnimElements.delete(element);
            }
          }
        };
        trAnim._onCssCancel = function () {
          if (tr.startTimer !== null) {
            try { clearTimeout(tr.startTimer); } catch (e) {}
            tr.startTimer = null;
          }
          if (st.transitions.get(prop) === tr) {
            st.transitions.delete(prop);
            if (st.transitions.size === 0 && st.animations.size === 0) {
              _activeCssAnimElements.delete(element);
            }
          }
        };
        st.transitions.set(prop, tr);
        trAnim.play();

        const initialElapsed = Math.max(0, -tr.delayMs) / 1000;

        // Fire transitionrun and transitionstart asynchronously (or in microtask / timer)
        Promise.resolve().then(function () {
          if (st.transitions.get(prop) !== tr || trAnim.playState === "idle") return;
          fireTransitionEvent(element, "transitionrun", prop, initialElapsed);
          if (tr.delayMs <= 0) {
            if (st.transitions.get(prop) !== tr || trAnim.playState === "idle") return;
            fireTransitionEvent(element, "transitionstart", prop, initialElapsed);
          } else if (typeof setTimeout === "function") {
            tr.startTimer = setTimeout(function () {
              tr.startTimer = null;
              if (st.transitions.get(prop) !== tr || trAnim.playState === "idle") return;
              fireTransitionEvent(element, "transitionstart", prop, 0);
            }, tr.delayMs);
          }
        });
      });
    }

    // Update lastStyle snapshot with static styles
    st.lastStyle = staticSnapshot;

    // 2. Handle CSS @keyframes Animations
    const animSpecs = parseAnimationSpecs(staticSnapshot);
    const activeNames = new Set();
    const keyframesMap = (animSpecs.length > 0 || st.animations.size > 0) ? extractKeyframesMap(styleNodes) : null;

    for (let i = 0; i < animSpecs.length; i++) {
      const sp = animSpecs[i];
      activeNames.add(sp.name);
      const existingRec = st.animations.get(sp.name);
      if (!existingRec) {
        const frames = keyframesMap ? keyframesMap.get(sp.name) : null;
        const kfList = frames ? frames.map(function (fr) {
          const o = { offset: fr.offset };
          for (const k in fr.props) {
            if (Object.prototype.hasOwnProperty.call(fr.props, k)) {
              o[kebabToCamelCase(k)] = fr.props[k];
            }
          }
          return o;
        }) : [];
        const cssAnimEffect = new KeyframeEffect(element, kfList, {
          duration: sp.durationMs,
          delay: sp.delayMs,
          easing: safeEasingForEffect(sp.easing),
          iterations: sp.iterations,
          direction: sp.direction,
          fill: sp.fill,
        });
        const cssAnim = new CSSAnimation(cssAnimEffect, globalDocTimeline, sp.name);
        const animRec = {
          name: sp.name,
          startTime: now,
          durationMs: sp.durationMs,
          delayMs: sp.delayMs,
          easing: sp.easing,
          iterations: sp.iterations,
          direction: sp.direction,
          fill: sp.fill,
          playState: sp.playState || "running",
          cssPlayState: sp.playState || "running",
          finished: false,
          anim: cssAnim,
        };
        cssAnim._onCssFinish = function () {
          animRec.finished = true;
        };
        cssAnim._onCssCancel = function () {
          if (st.animations.get(sp.name) === animRec) {
            st.animations.delete(sp.name);
            if (st.transitions.size === 0 && st.animations.size === 0) {
              _activeCssAnimElements.delete(element);
            }
          }
        };
        st.animations.set(sp.name, animRec);
        if (animRec.playState === "paused") {
          cssAnim.playState = "paused";
          cssAnim._holdTime = 0;
          cssAnim.startTime = null;
        } else {
          cssAnim.play();
        }
        const initialElapsed = Math.max(0, -sp.delayMs) / 1000;
        Promise.resolve().then(function () {
          if (st.animations.get(sp.name) !== animRec || cssAnim.playState === "idle") return;
          fireAnimationEvent(element, "animationstart", sp.name, initialElapsed);
        });
      } else if (existingRec.anim) {
        const nextCssPlayState = sp.playState || "running";
        if (nextCssPlayState !== existingRec.cssPlayState) {
          existingRec.cssPlayState = nextCssPlayState;
          if (nextCssPlayState === "paused") {
            existingRec.anim.pause();
          } else {
            existingRec.anim.play();
          }
        }
      }
    }

    // Cancel removed CSS animations
    st.animations.forEach(function (animRec, animName) {
      if (!activeNames.has(animName)) {
        st.animations.delete(animName);
        if (animRec.anim) {
          if (!animRec.finished && animRec.anim.playState !== "finished" && animRec.anim.playState !== "idle") {
            animRec.anim.cancel();
          } else {
            animRec.anim._clearFinishTimer();
            animRec.anim.playState = "idle";
            animRec.anim._holdTime = null;
            animRec.anim.startTime = null;
            registeredAnimations.delete(animRec.anim);
          }
        } else if (!animRec.finished) {
          const cancelElapsed = Math.max(0, (now - animRec.startTime) - animRec.delayMs) / 1000;
          fireAnimationEvent(element, "animationcancel", animName, cancelElapsed);
        }
      }
    });

    // 3. Overlay active CSS transitions onto computedMap
    st.transitions.forEach(function (tr, prop) {
      const curTime = tr.anim ? tr.anim.currentTime : (now - tr.startTime);
      const activeMs = (curTime !== null ? curTime : tr.durationMs + tr.delayMs) - tr.delayMs;
      if (activeMs >= tr.durationMs) {
        computedMap.set(prop, tr.toVal);
      } else {
        computedMap.set(prop, evalTransitionValue(tr, now));
      }
    });

    // 4. Overlay active CSS animations onto computedMap
    if (keyframesMap && st.animations.size > 0) {
      st.animations.forEach(function (animRec, animName) {
        const frames = keyframesMap.get(animName);
        if (!frames) return;
        const vals = evalCssAnimationStyles(animRec, frames, now, st.lastStyle);
        for (const k in vals) {
          if (Object.prototype.hasOwnProperty.call(vals, k)) {
            computedMap.set(k, vals[k]);
          }
        }
      });
    }

    if (st.transitions.size > 0 || st.animations.size > 0) {
      _activeCssAnimElements.add(element);
    } else {
      _activeCssAnimElements.delete(element);
    }
  }

  let _pageHasAnimOrTrans = false;
  let _checkedStyleCount = -1;

  function flushPendingStyleChecks() {
    _pendingStyleCheckScheduled = false;
    if (_pendingStyleCheckElements.size === 0) return;
    const list = Array.from(_pendingStyleCheckElements);
    _pendingStyleCheckElements.clear();
    if (!_pageHasAnimOrTrans && typeof document !== "undefined" && document && typeof document.querySelectorAll === "function") {
      try {
        const styles = document.querySelectorAll("style");
        if (styles.length !== _checkedStyleCount) {
          _checkedStyleCount = styles.length;
          for (let si = 0; si < styles.length; si++) {
            const txt = String(styles[si].textContent || "");
            if (txt.indexOf("transition") !== -1 || txt.indexOf("animation") !== -1 || txt.indexOf("keyframes") !== -1) {
              _pageHasAnimOrTrans = true;
              break;
            }
          }
        }
      } catch (e) {}
    }
    for (let i = 0; i < list.length; i++) {
      const el = list[i];
      if (!el || el.nodeType !== 1) continue;
      if (!_pageHasAnimOrTrans && _activeCssAnimElements.size === 0) {
        const stAttr = (el.getAttribute && el.getAttribute("style")) || "";
        const low = typeof stAttr === "string" ? stAttr.toLowerCase() : "";
        if (low.indexOf("transition") === -1 && low.indexOf("animation") === -1) {
          continue;
        }
      }
      // Only trigger transitions automatically if before-change style is already known
      // (either via explicit getComputedStyle or via post-DCL initial snapshot), or if
      // an animation may have been added/removed.
      const st = _cssAnimStateMap.get(el);
      if (!_snapshotsInitialized && (!st || st.lastStyle === null)) {
        continue;
      }
      try {
        if (typeof getComputedStyle === "function") {
          getComputedStyle(el);
        }
      } catch (e) {}
    }
  }

  function ensureBeforeChangeSnapshot(el) {
    if (!_snapshotsInitialized || !el || el.nodeType !== 1) return;
    const st = _cssAnimStateMap.get(el);
    if (st && st.lastStyle !== null) return;
    try {
      if (typeof getComputedStyle === "function") {
        getComputedStyle(el);
      }
    } catch (e) {}
  }

  function notifyCssStyleChange(el) {
    if (!el || el.nodeType !== 1) return;
    _pendingStyleCheckElements.add(el);
    if (!_pendingStyleCheckScheduled) {
      _pendingStyleCheckScheduled = true;
      Promise.resolve().then(flushPendingStyleChecks);
    }
  }

  function initCssTransitionSnapshots() {
    _snapshotsInitialized = true;
    try {
      if (typeof document === "undefined" || !document || typeof document.querySelectorAll !== "function") return;
      if (typeof getComputedStyle !== "function") return;
      const styles = document.querySelectorAll("style");
      let hasAnimOrTrans = false;
      const animSels = [];
      for (let i = 0; i < styles.length; i++) {
        const txt = String(styles[i].textContent || "");
        if (txt.indexOf("transition") !== -1 || txt.indexOf("animation") !== -1 || txt.indexOf("keyframes") !== -1) {
          hasAnimOrTrans = true;
          const ruleRe = /([^{}@]+)\{([^{}]*(?:transition|animation)[^{}]*)\}/gi;
          let rm;
          while ((rm = ruleRe.exec(txt)) !== null) {
            const s = rm[1].trim();
            if (s) animSels.push(s);
          }
        }
      }
      if (!hasAnimOrTrans) return;
      _pageHasAnimOrTrans = true;
      const visited = new Set();
      if (animSels.length > 0) {
        try {
          const matched = document.querySelectorAll(animSels.join(","));
          for (let i = 0; i < matched.length; i++) {
            const el = matched[i];
            if (!visited.has(el)) {
              visited.add(el);
              try { getComputedStyle(el); } catch (e) {}
            }
          }
        } catch (e) {
          const allEls = document.querySelectorAll("*");
          for (let i = 0; i < allEls.length; i++) {
            const el = allEls[i];
            const tag = String(el.localName || el.tagName || "").toLowerCase();
            if (tag === "html" || tag === "head" || tag === "style" || tag === "script" || tag === "meta" || tag === "title" || tag === "link") continue;
            try { getComputedStyle(el); } catch (e) {}
          }
        }
      }
      const inlineStyled = document.querySelectorAll("[style]");
      for (let i = 0; i < inlineStyled.length; i++) {
        const el = inlineStyled[i];
        if (visited.has(el)) continue;
        const stStr = String((el.getAttribute && el.getAttribute("style")) || "").toLowerCase();
        if (stStr.indexOf("transition") !== -1 || stStr.indexOf("animation") !== -1) {
          try { getComputedStyle(el); } catch (e) {}
        }
      }
    } catch (e) {}
  }

  function sampleActiveAnimations() {
    if (!_snapshotsInitialized) {
      initCssTransitionSnapshots();
    }
    if (_activeCssAnimElements.size === 0) return "";
    const now = (typeof performance !== "undefined" && performance.now) ? performance.now() : Date.now();
    let keyframesMap = null;
    const lines = [];
    _activeCssAnimElements.forEach(function (el) {
      if (!el || typeof el._nid !== "number" || el._nid <= 0) {
        _activeCssAnimElements.delete(el);
        return;
      }
      const st = _cssAnimStateMap.get(el);
      if (!st || (st.transitions.size === 0 && st.animations.size === 0)) {
        _activeCssAnimElements.delete(el);
        return;
      }
      const decls = [];
      st.transitions.forEach(function (tr, prop) {
        const curTime = tr.anim ? tr.anim.currentTime : (now - tr.startTime);
        const activeMs = (curTime !== null ? curTime : tr.durationMs + tr.delayMs) - tr.delayMs;
        if (activeMs >= tr.durationMs) {
          decls.push(prop + ":" + tr.toVal);
        } else {
          decls.push(prop + ":" + evalTransitionValue(tr, now));
        }
      });
      if (st.animations.size > 0) {
        if (!keyframesMap) {
          const styleNodes = (typeof document !== "undefined" && document && typeof document.querySelectorAll === "function")
            ? document.querySelectorAll("style")
            : [];
          keyframesMap = extractKeyframesMap(styleNodes);
        }
        st.animations.forEach(function (animRec, animName) {
          const frames = keyframesMap.get(animName);
          if (!frames) return;
          const vals = evalCssAnimationStyles(animRec, frames, now, st.lastStyle);
          for (const k in vals) {
            if (Object.prototype.hasOwnProperty.call(vals, k)) {
              decls.push(k + ":" + vals[k]);
            }
          }
        });
      }
      if (decls.length > 0) {
        lines.push(el._nid + "|" + decls.join(";"));
      }
    });
    return lines.join("\n");
  }

  _flushCssAnimHook = function () {
    if (!_snapshotsInitialized) {
      initCssTransitionSnapshots();
    }
    if (_pendingStyleCheckElements.size > 0) {
      flushPendingStyleChecks();
    }
  };

  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_syncCssTransitionsAndAnimations = syncCssTransitionsAndAnimations;
    globalThis.__capsurf_notifyCssStyleChange = notifyCssStyleChange;
    globalThis.__capsurf_ensureBeforeChangeSnapshot = ensureBeforeChangeSnapshot;
    globalThis.__capsurf_initCssTransitionSnapshots = initCssTransitionSnapshots;
    globalThis.__capsurf_sample_active_animations = sampleActiveAnimations;
  }
})();
