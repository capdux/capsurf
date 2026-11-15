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

function KeyframeEffect(target, keyframes, options) {
  AnimationEffect.call(this);
  this.target = target || null;
  this.keyframes = Array.isArray(keyframes) ? keyframes : [];
  const opts = typeof options === "number" ? { duration: options } : (options || {});
  this.delay = Number(opts.delay || 0);
  this.endDelay = Number(opts.endDelay || 0);
  this.fill = String(opts.fill || "auto");
  this.iterationStart = Number(opts.iterationStart || 0);
  this.iterations = Number(opts.iterations || 1);
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

  if (!isFullAnimation) {
    // Static headless mode: instant finish, zero background cpu burn
    this.playState = "finished";
    const dur = (effect && typeof effect.duration === "number") ? effect.duration : 0;
    this.currentTime = dur;
    this.ready = Promise.resolve(this);
    this.finished = Promise.resolve(this);
    // Apply end keyframes directly to target style
    if (effect && effect.target && effect.keyframes && effect.keyframes.length > 0) {
      const endFrame = effect.keyframes[effect.keyframes.length - 1];
      for (const k in endFrame) {
        if (k !== "offset" && k !== "easing" && k !== "composite") {
          try { effect.target.style[k] = endFrame[k]; } catch (e) {}
        }
      }
    }
  } else {
    // Full interactive animation mode
    this.playState = "idle";
    const self = this;
    this._readyResolve = null;
    this._finishedResolve = null;
    this.ready = new Promise(function (res) { self._readyResolve = res; });
    this.finished = new Promise(function (res) { self._finishedResolve = res; });
    registeredAnimations.add(this);
  }
}
Animation.prototype = Object.create(EventTarget.prototype);
Animation.prototype.constructor = Animation;

Animation.prototype.play = function () {
  if (!globalThis.__CAP_RENDER_CONFIG || !globalThis.__CAP_RENDER_CONFIG.animations) {
    this.playState = "finished";
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
  if (this.playState === "running") {
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
