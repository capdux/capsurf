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
    return (typeof performance !== "undefined" && performance.now) ? performance.now() : Date.now();
  },
  configurable: true,
});
markNative(DocumentTimeline, "DocumentTimeline");

const globalDocTimeline = new DocumentTimeline();

function AnimationEffect() {}
markNative(AnimationEffect, "AnimationEffect");

function normalizeKeyframesInput(keyframes) {
  if (!keyframes) return [];
  if (Array.isArray(keyframes)) {
    return keyframes.map(function (kf) {
      return kf && typeof kf === "object" ? Object.assign({}, kf) : {};
    });
  }
  if (typeof keyframes === "object") {
    // Property-indexed keyframes: { color: ["#205080", "#d0a040"], transform: [...], easing: "..." }
    const propArrays = {};
    let maxLen = 0;
    const sharedEasing = typeof keyframes.easing === "string" ? keyframes.easing : null;
    const sharedComposite = typeof keyframes.composite === "string" ? keyframes.composite : null;
    const offsets = Array.isArray(keyframes.offset) ? keyframes.offset : null;
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
    const frames = [];
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
    return frames;
  }
  return [];
}

function solveCubicBezier(x1, y1, x2, y2, t) {
  if (t <= 0) return 0;
  if (t >= 1) return 1;
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
  const fn1 = String(fromVal || "").trim().match(/^([a-zA-Z0-9_-]+)\(([^)]*)\)$/);
  const fn2 = String(toVal || "").trim().match(/^([a-zA-Z0-9_-]+)\(([^)]*)\)$/);
  if (fn1 && fn2 && fn1[1].toLowerCase() === fn2[1].toLowerCase()) {
    const a1 = fn1[2].trim().split(/\s+/).filter(Boolean);
    const a2 = fn2[2].trim().split(/\s+/).filter(Boolean);
    if (a1.length > 0 && a1.length === a2.length) {
      const parts = [];
      let allNum = true;
      for (let i = 0; i < a1.length; i++) {
        const m1 = a1[i].match(/^([+-]?(?:\d+\.?\d*|\.\d+))([a-z%]*)$/i);
        const m2 = a2[i].match(/^([+-]?(?:\d+\.?\d*|\.\d+))([a-z%]*)$/i);
        if (!m1 || !m2 || m1[2] !== m2[2]) { allNum = false; break; }
        const v = parseFloat(m1[1]) + (parseFloat(m2[1]) - parseFloat(m1[1])) * p;
        const formatted = Math.abs(v - Math.round(v)) < 1e-6 ? String(Math.round(v)) : String(Number(v.toFixed(4)));
        parts.push(formatted + m1[2]);
      }
      if (allNum) return fn1[1].toLowerCase() + "(" + parts.join(" ") + ")";
    }
  }
  const n1 = String(fromVal || "").trim().match(/^([+-]?(?:\d+\.?\d*|\.\d+))([a-z%]*)$/i);
  const n2 = String(toVal || "").trim().match(/^([+-]?(?:\d+\.?\d*|\.\d+))([a-z%]*)$/i);
  if (n1 && n2 && n1[2] === n2[2]) {
    const v = parseFloat(n1[1]) + (parseFloat(n2[1]) - parseFloat(n1[1])) * p;
    const formatted = Math.abs(v - Math.round(v)) < 1e-6 ? String(Math.round(v)) : String(Number(v.toFixed(4)));
    return formatted + n1[2];
  }
  return p < 0.5 ? fromVal : toVal;
}

function KeyframeEffect(target, keyframes, options) {
  AnimationEffect.call(this);
  this.target = target || null;
  this.keyframes = normalizeKeyframesInput(keyframes);
  const opts = typeof options === "number" ? { duration: options } : (options || {});
  this.delay = Number(opts.delay || 0);
  this.endDelay = Number(opts.endDelay || 0);
  this.fill = String(opts.fill || "auto");
  this.iterationStart = Number(opts.iterationStart || 0);
  this.iterations = opts.iterations !== undefined ? Number(opts.iterations) : 1;
  this.duration = opts.duration !== undefined ? (opts.duration === "auto" ? "auto" : Number(opts.duration)) : 0;
  this.direction = String(opts.direction || "normal");
  this.easing = String(opts.easing || "linear");
}
KeyframeEffect.prototype = Object.create(AnimationEffect.prototype);
KeyframeEffect.prototype.constructor = KeyframeEffect;

KeyframeEffect.prototype.getComputedTiming = function () {
  return {
    delay: this.delay,
    endDelay: this.endDelay,
    fill: this.fill,
    iterationStart: this.iterationStart,
    iterations: this.iterations,
    duration: this.duration,
    direction: this.direction,
    easing: this.easing,
    endTime: this.delay + (typeof this.duration === "number" ? this.duration * this.iterations : 0) + this.endDelay,
    activeDuration: typeof this.duration === "number" ? this.duration * this.iterations : 0,
    localTime: 0,
    progress: 1.0,
  };
};

KeyframeEffect.prototype.getKeyframes = function () {
  return this.keyframes.slice();
};

markNative(KeyframeEffect, "KeyframeEffect");
markNative(KeyframeEffect.prototype.getComputedTiming, "getComputedTiming");
markNative(KeyframeEffect.prototype.getKeyframes, "getKeyframes");

const registeredAnimations = new Set();

function Animation(effect, timeline) {
  EventTarget.call(this);
  this.id = "";
  this.effect = effect || null;
  this.timeline = timeline || globalDocTimeline;
  this.playbackRate = 1.0;
  this.startTime = null;
  this.currentTime = 0;
  this.pending = false;
  this.onfinish = null;
  this.oncancel = null;
  this.onremove = null;

  const isFullAnimation = Boolean(globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.animations);
  registeredAnimations.add(this);

  if (!isFullAnimation) {
    // Static headless mode: active at start, can be paused/scrubbed via currentTime
    this.playState = "running";
    this.currentTime = 0;
    this.ready = Promise.resolve(this);
    this.finished = Promise.resolve(this);
  } else {
    // Full interactive animation mode
    this.playState = "idle";
    const self = this;
    this._readyResolve = null;
    this._finishedResolve = null;
    this.ready = new Promise(function (res) { self._readyResolve = res; });
    this.finished = new Promise(function (res) { self._finishedResolve = res; });
  }
}
Animation.prototype = Object.create(EventTarget.prototype);
Animation.prototype.constructor = Animation;

Animation.prototype.play = function () {
  if (!globalThis.__CAP_RENDER_CONFIG || !globalThis.__CAP_RENDER_CONFIG.animations) {
    this.playState = "running";
    return;
  }
  this.playState = "running";
  this.startTime = this.timeline.currentTime;
  if (this._readyResolve) {
    this._readyResolve(this);
    this._readyResolve = null;
  }
  const self = this;
  const dur = (this.effect && typeof this.effect.duration === "number") ? this.effect.duration : 0;
  setTimeout(function () {
    if (self.playState === "running") {
      self.finish();
    }
  }, Math.max(0, dur));
};

Animation.prototype.pause = function () {
  if (this.playState !== "idle") {
    this.playState = "paused";
  }
};

Animation.prototype.finish = function () {
  this.playState = "finished";
  const dur = (this.effect && typeof this.effect.duration === "number") ? this.effect.duration : 0;
  this.currentTime = dur;

  // Apply end keyframes to target
  if (this.effect && this.effect.target && this.effect.keyframes && this.effect.keyframes.length > 0) {
    const endFrame = this.effect.keyframes[this.effect.keyframes.length - 1];
    for (const k in endFrame) {
      if (k !== "offset" && k !== "easing" && k !== "composite") {
        try { this.effect.target.style[k] = endFrame[k]; } catch (e) {}
      }
    }
  }

  if (this._finishedResolve) {
    this._finishedResolve(this);
    this._finishedResolve = null;
  }

  const evt = new Event("finish");
  this.dispatchEvent(evt);
  if (typeof this.onfinish === "function") {
    try { this.onfinish.call(this, evt); } catch (e) {}
  }
};

Animation.prototype.cancel = function () {
  this.playState = "idle";
  this.currentTime = null;
  this.startTime = null;
  registeredAnimations.delete(this);
  const evt = new Event("cancel");
  this.dispatchEvent(evt);
  if (typeof this.oncancel === "function") {
    try { this.oncancel.call(this, evt); } catch (e) {}
  }
};

Animation.prototype.reverse = function () {
  this.playbackRate = -this.playbackRate;
  this.play();
};

markNative(Animation, "Animation");
markNative(Animation.prototype.play, "play");
markNative(Animation.prototype.pause, "pause");
markNative(Animation.prototype.finish, "finish");
markNative(Animation.prototype.cancel, "cancel");

function animateElement(element, keyframes, options) {
  const effect = new KeyframeEffect(element, keyframes, options);
  const anim = new Animation(effect, globalDocTimeline);
  anim.play();
  return anim;
}

function animationsFor(root) {
  const res = [];
  for (const anim of registeredAnimations) {
    if (anim.effect && anim.effect.target) {
      if (!root || root === anim.effect.target || (root.contains && root.contains(anim.effect.target))) {
        res.push(anim);
      }
    }
  }
  return res;
}

function computeAnimatedStylesForElement(element) {
  const result = {};
  if (!element) return result;
  for (const anim of registeredAnimations) {
    if (!anim || !anim.effect || anim.effect.target !== element) continue;
    if (anim.playState === "idle") continue;
    const effect = anim.effect;
    const frames = effect.keyframes;
    if (!frames || frames.length === 0) continue;
    const dur = typeof effect.duration === "number" ? effect.duration : 0;
    const delay = effect.delay || 0;
    const localTime = anim.currentTime != null ? Number(anim.currentTime) : 0;
    const activeTime = localTime - delay;
    if (activeTime < 0 && effect.fill !== "backwards" && effect.fill !== "both") continue;
    const totalDur = dur * (effect.iterations || 1);
    if (activeTime >= totalDur && totalDur > 0 && effect.fill !== "forwards" && effect.fill !== "both") {
      continue;
    }
    let overallProgress = dur > 0 ? Math.max(0, Math.min(1, (activeTime % dur === 0 && activeTime > 0 && activeTime >= totalDur) ? 1 : (activeTime % dur) / dur)) : 1;
    if (activeTime <= 0) overallProgress = 0;
    else if (activeTime >= totalDur && dur > 0) overallProgress = 1;
    const effectProgress = evaluateAnimationEasing(effect.easing, overallProgress);

    if (frames.length === 1) {
      const f = frames[0];
      for (const k in f) {
        if (k !== "offset" && k !== "easing" && k !== "composite") {
          const kebab = k.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
          result[kebab] = f[k];
        }
      }
      continue;
    }

    // Assign computed offsets if missing
    const offsets = frames.map(function (f, idx) {
      if (f.offset != null && !isNaN(Number(f.offset))) return Number(f.offset);
      return idx / (frames.length - 1);
    });
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
    const segT = span > 0 ? Math.max(0, Math.min(1, (effectProgress - offsets[idx]) / span)) : 1;
    const easedP = evaluateAnimationEasing(f0.easing || "linear", segT);

    const props = new Set();
    for (const k in f0) if (k !== "offset" && k !== "easing" && k !== "composite") props.add(k);
    for (const k in f1) if (k !== "offset" && k !== "easing" && k !== "composite") props.add(k);

    for (const prop of props) {
      const kebab = prop.replace(/[A-Z]/g, function (m) { return "-" + m.toLowerCase(); });
      if (f0[prop] !== undefined && f1[prop] !== undefined) {
        result[kebab] = interpolateAnimValue(kebab, f0[prop], f1[prop], easedP);
      } else if (f1[prop] !== undefined) {
        result[kebab] = f1[prop];
      } else if (f0[prop] !== undefined) {
        result[kebab] = f0[prop];
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
    const m = token.trim().match(/^([+-]?(?:\d+\.?\d*|\.\d+))(ms|s)$/i);
    if (!m) return null;
    const val = parseFloat(m[1]);
    if (isNaN(val)) return null;
    return m[2].toLowerCase() === "s" ? val * 1000 : val;
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
            // ignore play-state for now
          } else {
            name = t;
          }
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
            if (k && v) props[k] = v;
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

  function evalTransitionValue(tr, now) {
    const activeMs = (now - tr.startTime) - tr.delayMs;
    if (activeMs <= 0) return tr.fromVal;
    if (activeMs >= tr.durationMs) return tr.toVal;
    const rawT = activeMs / tr.durationMs;
    const easedT = evaluateAnimationEasing(tr.easing, rawT);
    return interpolateAnimValue(tr.prop, tr.fromVal, tr.toVal, easedT);
  }

  function evalCssAnimationStyles(anim, frames, now) {
    const out = {};
    if (!frames || frames.length === 0) return out;
    const activeMs = (now - anim.startTime) - anim.delayMs;
    const totalMs = anim.iterations === Infinity ? Infinity : anim.durationMs * anim.iterations;
    if (activeMs < 0 && anim.fill !== "backwards" && anim.fill !== "both") return out;
    if (activeMs >= totalMs && anim.fill !== "forwards" && anim.fill !== "both") return out;

    let progress;
    if (activeMs <= 0) {
      progress = 0;
    } else if (activeMs >= totalMs) {
      progress = 1;
    } else {
      progress = (activeMs % anim.durationMs) / anim.durationMs;
    }
    const easedP = evaluateAnimationEasing(anim.easing, progress);

    let idx = 0;
    for (let i = 0; i < frames.length - 1; i++) {
      if (easedP >= frames[i].offset && easedP <= frames[i + 1].offset) {
        idx = i;
        break;
      }
      if (easedP > frames[i + 1].offset) idx = i;
    }
    const f0 = frames[idx];
    const f1 = frames[idx + 1] || f0;
    const span = f1.offset - f0.offset;
    const segT = span > 0 ? Math.max(0, Math.min(1, (easedP - f0.offset) / span)) : (easedP >= f1.offset ? 1 : 0);

    const allProps = new Set();
    for (const k in f0.props) allProps.add(k);
    for (const k in f1.props) allProps.add(k);
    allProps.forEach(function (prop) {
      if (f0.props[prop] !== undefined && f1.props[prop] !== undefined) {
        out[prop] = interpolateAnimValue(prop, f0.props[prop], f1.props[prop], segT);
      } else if (f1.props[prop] !== undefined) {
        out[prop] = f1.props[prop];
      } else if (f0.props[prop] !== undefined) {
        out[prop] = f0.props[prop];
      }
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
        else specByProp.set(trSpecs[i].prop, trSpecs[i]);
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
          if (existing.endTimer !== null) {
            try { clearTimeout(existing.endTimer); } catch (e) {}
            existing.endTimer = null;
          }
          if (existing.startTimer !== null) {
            try { clearTimeout(existing.startTimer); } catch (e) {}
            existing.startTimer = null;
          }
          const cancelElapsed = Math.max(0, Math.min(existing.durationMs, (now - existing.startTime) - existing.delayMs)) / 1000;
          st.transitions.delete(prop);
          fireTransitionEvent(element, "transitioncancel", prop, cancelElapsed);
        }

        const tr = {
          prop: prop,
          fromVal: startFromVal,
          toVal: nextVal,
          startTime: now,
          durationMs: spec.durationMs,
          delayMs: spec.delayMs,
          easing: spec.easing,
          startTimer: null,
          endTimer: null,
        };
        st.transitions.set(prop, tr);

        const initialElapsed = Math.max(0, -tr.delayMs) / 1000;
        const remainingMs = Math.max(0, tr.durationMs + tr.delayMs);

        // Fire transitionrun and transitionstart asynchronously (or in microtask / timer)
        Promise.resolve().then(function () {
          if (st.transitions.get(prop) !== tr) return;
          fireTransitionEvent(element, "transitionrun", prop, initialElapsed);
          if (tr.delayMs <= 0) {
            if (st.transitions.get(prop) !== tr) return;
            fireTransitionEvent(element, "transitionstart", prop, initialElapsed);
          } else if (typeof setTimeout === "function") {
            tr.startTimer = setTimeout(function () {
              tr.startTimer = null;
              if (st.transitions.get(prop) !== tr) return;
              fireTransitionEvent(element, "transitionstart", prop, 0);
            }, tr.delayMs);
          }
        });

        if (typeof setTimeout === "function") {
          tr.endTimer = setTimeout(function () {
            tr.endTimer = null;
            if (st.transitions.get(prop) !== tr) return;
            st.transitions.delete(prop);
            fireTransitionEvent(element, "transitionend", prop, tr.durationMs / 1000);
          }, remainingMs);
        }
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
      if (!st.animations.has(sp.name)) {
        const animRec = {
          name: sp.name,
          startTime: now,
          durationMs: sp.durationMs,
          delayMs: sp.delayMs,
          easing: sp.easing,
          iterations: sp.iterations,
          direction: sp.direction,
          fill: sp.fill,
          finished: false,
          endTimer: null,
        };
        st.animations.set(sp.name, animRec);
        const initialElapsed = Math.max(0, -sp.delayMs) / 1000;
        Promise.resolve().then(function () {
          if (st.animations.get(sp.name) !== animRec) return;
          fireAnimationEvent(element, "animationstart", sp.name, initialElapsed);
        });
        if (sp.iterations !== Infinity && typeof setTimeout === "function") {
          const totalMs = Math.max(0, sp.durationMs * sp.iterations + sp.delayMs);
          animRec.endTimer = setTimeout(function () {
            animRec.endTimer = null;
            if (st.animations.get(sp.name) !== animRec) return;
            animRec.finished = true;
            fireAnimationEvent(element, "animationend", sp.name, (sp.durationMs * sp.iterations) / 1000);
          }, totalMs);
        }
      }
    }

    // Cancel removed CSS animations
    st.animations.forEach(function (animRec, animName) {
      if (!activeNames.has(animName)) {
        if (animRec.endTimer !== null) {
          try { clearTimeout(animRec.endTimer); } catch (e) {}
          animRec.endTimer = null;
        }
        st.animations.delete(animName);
        if (!animRec.finished) {
          const cancelElapsed = Math.max(0, (now - animRec.startTime) - animRec.delayMs) / 1000;
          fireAnimationEvent(element, "animationcancel", animName, cancelElapsed);
        }
      }
    });

    // 3. Overlay active CSS transitions onto computedMap
    st.transitions.forEach(function (tr, prop) {
      const activeMs = (now - tr.startTime) - tr.delayMs;
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
        const vals = evalCssAnimationStyles(animRec, frames, now);
        for (const k in vals) {
          if (Object.prototype.hasOwnProperty.call(vals, k)) {
            computedMap.set(k, vals[k]);
          }
        }
      });
    }
  }

  function flushPendingStyleChecks() {
    _pendingStyleCheckScheduled = false;
    if (_pendingStyleCheckElements.size === 0) return;
    const list = Array.from(_pendingStyleCheckElements);
    _pendingStyleCheckElements.clear();
    for (let i = 0; i < list.length; i++) {
      const el = list[i];
      if (!el || el.nodeType !== 1) continue;
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
      for (let i = 0; i < styles.length; i++) {
        const txt = String(styles[i].textContent || "");
        if (txt.indexOf("transition") !== -1 || txt.indexOf("animation") !== -1 || txt.indexOf("keyframes") !== -1) {
          hasAnimOrTrans = true;
          break;
        }
      }
      if (!hasAnimOrTrans) return;
      const allEls = document.querySelectorAll("*");
      for (let i = 0; i < allEls.length; i++) {
        const el = allEls[i];
        const tag = String(el.localName || el.tagName || "").toLowerCase();
        if (tag === "html" || tag === "head" || tag === "style" || tag === "script" || tag === "meta" || tag === "title" || tag === "link") {
          continue;
        }
        try { getComputedStyle(el); } catch (e) {}
      }
    } catch (e) {}
  }

  if (typeof globalThis !== "undefined") {
    globalThis.__capsurf_syncCssTransitionsAndAnimations = syncCssTransitionsAndAnimations;
    globalThis.__capsurf_notifyCssStyleChange = notifyCssStyleChange;
    globalThis.__capsurf_initCssTransitionSnapshots = initCssTransitionSnapshots;
  }
})();
