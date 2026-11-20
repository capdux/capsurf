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
  const n1 = String(fromVal || "").trim().match(/^([+-]?(?:\d+\.?\d*|\.\d+))([a-z%]*)$/i);
  const n2 = String(toVal || "").trim().match(/^([+-]?(?:\d+\.?\d*|\.\d+))([a-z%]*)$/i);
  if (n1 && n2 && n1[2] === n2[2]) {
    const v = parseFloat(n1[1]) + (parseFloat(n2[1]) - parseFloat(n1[1])) * p;
    return String(v) + n1[2];
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
