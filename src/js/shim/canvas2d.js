// src/js/shim/canvas2d.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/canvas2d.ts
// Cloud vector recording and rendering engine for HTML5 Canvas 2D.
// Configured by globalThis.__CAP_RENDER_CONFIG.canvasRecording (true: record ops, false: headless fast stub).

const VALID_PNG_BASE64 = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=";

const _INTERNAL_CTX_SECRET = Symbol("CanvasRenderingContext2D_internal");
const _ctxInstances = new WeakSet();

function checkCtx(self) {
  if (!self || typeof self !== "object" || !_ctxInstances.has(self)) {
    throw new TypeError("Illegal invocation");
  }
}

const NAMED_COLORS = {
  transparent: [0, 0, 0, 0],
  black: [0, 0, 0, 1],
  white: [255, 255, 255, 1],
  red: [255, 0, 0, 1],
  green: [0, 128, 0, 1],
  blue: [0, 0, 255, 1],
  yellow: [255, 255, 0, 1],
  cyan: [0, 255, 255, 1],
  aqua: [0, 255, 255, 1],
  magenta: [255, 0, 255, 1],
  fuchsia: [255, 0, 255, 1],
  gray: [128, 128, 128, 1],
  grey: [128, 128, 128, 1],
  lightgray: [211, 211, 211, 1],
  lightgrey: [211, 211, 211, 1],
  darkgray: [169, 169, 169, 1],
  darkgrey: [169, 169, 169, 1],
  orange: [255, 165, 0, 1],
  pink: [255, 192, 203, 1],
  purple: [128, 0, 128, 1],
  brown: [165, 42, 42, 1],
  lime: [0, 255, 0, 1],
  maroon: [128, 0, 0, 1],
  navy: [0, 0, 128, 1],
  olive: [128, 128, 0, 1],
  silver: [192, 192, 192, 1],
  teal: [0, 128, 128, 1],
  coral: [255, 127, 80, 1],
  gold: [255, 215, 0, 1],
  indigo: [75, 0, 130, 1],
  violet: [238, 130, 238, 1],
};

function parseColor(str) {
  if (typeof str !== "string") return null;
  const s = str.trim().toLowerCase();
  if (!s) return null;

  if (Object.prototype.hasOwnProperty.call(NAMED_COLORS, s)) {
    return NAMED_COLORS[s].slice();
  }

  if (s.charCodeAt(0) === 35) { // '#'
    const hex = s.slice(1);
    if (hex.length === 3) {
      const r = parseInt(hex[0] + hex[0], 16);
      const g = parseInt(hex[1] + hex[1], 16);
      const b = parseInt(hex[2] + hex[2], 16);
      if (!Number.isNaN(r) && !Number.isNaN(g) && !Number.isNaN(b)) {
        return [r, g, b, 1.0];
      }
    } else if (hex.length === 4) {
      const r = parseInt(hex[0] + hex[0], 16);
      const g = parseInt(hex[1] + hex[1], 16);
      const b = parseInt(hex[2] + hex[2], 16);
      const a = parseInt(hex[3] + hex[3], 16) / 255.0;
      if (!Number.isNaN(r) && !Number.isNaN(g) && !Number.isNaN(b) && !Number.isNaN(a)) {
        return [r, g, b, a];
      }
    } else if (hex.length === 6) {
      const r = parseInt(hex.slice(0, 2), 16);
      const g = parseInt(hex.slice(2, 4), 16);
      const b = parseInt(hex.slice(4, 6), 16);
      if (!Number.isNaN(r) && !Number.isNaN(g) && !Number.isNaN(b)) {
        return [r, g, b, 1.0];
      }
    } else if (hex.length === 8) {
      const r = parseInt(hex.slice(0, 2), 16);
      const g = parseInt(hex.slice(2, 4), 16);
      const b = parseInt(hex.slice(4, 6), 16);
      const a = parseInt(hex.slice(6, 8), 16) / 255.0;
      if (!Number.isNaN(r) && !Number.isNaN(g) && !Number.isNaN(b) && !Number.isNaN(a)) {
        return [r, g, b, a];
      }
    }
    return null;
  }

  if (s.startsWith("rgb(") || s.startsWith("rgba(")) {
    const start = s.indexOf("(") + 1;
    const end = s.indexOf(")");
    if (end > start) {
      const parts = s.slice(start, end).split(/[, /]+/).map(function (p) { return p.trim(); }).filter(Boolean);
      if (parts.length >= 3) {
        const parseComponent = function (p) {
          if (p.endsWith("%")) return Math.round((parseFloat(p) / 100.0) * 255.0);
          return Math.round(parseFloat(p));
        };
        const r = Math.min(255, Math.max(0, parseComponent(parts[0])));
        const g = Math.min(255, Math.max(0, parseComponent(parts[1])));
        const b = Math.min(255, Math.max(0, parseComponent(parts[2])));
        let a = 1.0;
        if (parts.length >= 4) {
          const ap = parts[3];
          if (ap.endsWith("%")) a = parseFloat(ap) / 100.0;
          else a = parseFloat(ap);
          a = Math.min(1.0, Math.max(0.0, a));
        }
        if (!Number.isNaN(r) && !Number.isNaN(g) && !Number.isNaN(b) && !Number.isNaN(a)) {
          return [r, g, b, a];
        }
      }
    }
    return null;
  }

  return null;
}

function formatColor(rgba) {
  const r = Math.min(255, Math.max(0, Math.round(rgba[0])));
  const g = Math.min(255, Math.max(0, Math.round(rgba[1])));
  const b = Math.min(255, Math.max(0, Math.round(rgba[2])));
  const a = rgba[3];
  if (a >= 1.0) {
    const hex = ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
    return "#" + hex;
  }
  const aFormatted = Number(a.toFixed(3)).toString();
  return "rgba(" + r + ", " + g + ", " + b + ", " + aFormatted + ")";
}

function getCanvasWidth(canvas) {
  if (!canvas) return 300;
  const w = parseInt(canvas.width, 10);
  return Number.isFinite(w) && w > 0 ? w : 300;
}

function getCanvasHeight(canvas) {
  if (!canvas) return 150;
  const h = parseInt(canvas.height, 10);
  return Number.isFinite(h) && h > 0 ? h : 150;
}

function ensureBitmap(ctx) {
  const w = getCanvasWidth(ctx._canvas);
  const h = getCanvasHeight(ctx._canvas);
  if (!ctx._bitmap || ctx._bitmapWidth !== w || ctx._bitmapHeight !== h) {
    ctx._bitmapWidth = w;
    ctx._bitmapHeight = h;
    ctx._bitmap = new Uint8ClampedArray(w * h * 4);
  }
  return ctx._bitmap;
}

function CanvasGradient(type, x0, y0, r0, x1, y1, r1) {
  this._type = type; // "linear" or "radial"
  this._stops = [];
  this._coords = [x0, y0, r0, x1, y1, r1];
}
CanvasGradient.prototype.addColorStop = function (offset, color) {
  this._stops.push({ offset: Number(offset), color: String(color) });
};
markNative(CanvasGradient, "CanvasGradient");
markNative(CanvasGradient.prototype.addColorStop, "addColorStop");

function CanvasPattern(image, repetition) {
  this._image = image;
  this._repetition = String(repetition || "repeat");
  this._matrix = [1, 0, 0, 1, 0, 0];
}
CanvasPattern.prototype.setTransform = function (matrix) {
  if (matrix && matrix.a !== undefined) {
    this._matrix = [matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f];
  }
};
markNative(CanvasPattern, "CanvasPattern");
markNative(CanvasPattern.prototype.setTransform, "setTransform");

function Path2D(path) {
  this._ops = [];
  if (path instanceof Path2D) {
    this._ops = path._ops.slice();
  }
}
const pathMethods = ["moveTo", "lineTo", "quadraticCurveTo", "bezierCurveTo", "arcTo", "arc", "ellipse", "rect", "closePath"];
for (let i = 0; i < pathMethods.length; i++) {
  const m = pathMethods[i];
  Path2D.prototype[m] = function () {
    this._ops.push({ op: m, args: Array.prototype.slice.call(arguments) });
  };
}
Path2D.prototype.addPath = function (path, transform) {
  if (path instanceof Path2D) {
    this._ops.push({ op: "addPath", pathOps: path._ops.slice(), transform: transform });
  }
};
markNative(Path2D, "Path2D");

function TextMetrics(text, font) {
  const str = String(text || "");
  const fontSizeMatch = (font || "").match(/(\d+)px/);
  const fontSize = fontSizeMatch ? parseFloat(fontSizeMatch[1]) : 10;
  const width = str.length * fontSize * 0.6;
  this.width = width;
  this.actualBoundingBoxLeft = 0.0;
  this.actualBoundingBoxRight = width;
  this.actualBoundingBoxAscent = fontSize * 0.8;
  this.actualBoundingBoxDescent = fontSize * 0.2;
  this.fontBoundingBoxAscent = fontSize;
  this.fontBoundingBoxDescent = fontSize * 0.25;
  this.alphabeticBaseline = 0.0;
}
markNative(TextMetrics, "TextMetrics");

function ImageData(w, h, data) {
  if (typeof w === "object" && w instanceof Uint8ClampedArray) {
    this.data = w;
    this.width = Number(h || 1);
    this.height = Math.max(1, (this.data.length / (4 * this.width)) | 0);
  } else {
    this.width = Math.max(1, Number(w || 1) | 0);
    this.height = Math.max(1, Number(h || 1) | 0);
    this.data = data instanceof Uint8ClampedArray ? data : new Uint8ClampedArray(this.width * this.height * 4);
  }
  this.colorSpace = "srgb";
}
markNative(ImageData, "ImageData");

function defaultCanvasState() {
  return {
    strokeStyle: "#000000",
    fillStyle: "#000000",
    lineWidth: 1.0,
    lineCap: "butt",
    lineJoin: "miter",
    miterLimit: 10.0,
    lineDash: [],
    lineDashOffset: 0.0,
    globalAlpha: 1.0,
    globalCompositeOperation: "source-over",
    shadowColor: "rgba(0, 0, 0, 0)",
    shadowBlur: 0,
    shadowOffsetX: 0,
    shadowOffsetY: 0,
    font: "10px sans-serif",
    textAlign: "start",
    textBaseline: "alphabetic",
    direction: "inherit",
    filter: "none",
    imageSmoothingEnabled: true,
    matrix: [1, 0, 0, 1, 0, 0],
  };
}

function cloneCanvasState(s) {
  return {
    strokeStyle: s.strokeStyle,
    fillStyle: s.fillStyle,
    lineWidth: s.lineWidth,
    lineCap: s.lineCap,
    lineJoin: s.lineJoin,
    miterLimit: s.miterLimit,
    lineDash: s.lineDash.slice(),
    lineDashOffset: s.lineDashOffset,
    globalAlpha: s.globalAlpha,
    globalCompositeOperation: s.globalCompositeOperation,
    shadowColor: s.shadowColor,
    shadowBlur: s.shadowBlur,
    shadowOffsetX: s.shadowOffsetX,
    shadowOffsetY: s.shadowOffsetY,
    font: s.font,
    textAlign: s.textAlign,
    textBaseline: s.textBaseline,
    direction: s.direction,
    filter: s.filter,
    imageSmoothingEnabled: s.imageSmoothingEnabled,
    matrix: s.matrix.slice(),
  };
}

function CanvasRenderingContext2D(canvas, secret) {
  if (secret !== _INTERNAL_CTX_SECRET) {
    throw new TypeError("Illegal constructor");
  }
  _ctxInstances.add(this);
  this._canvas = canvas || null;
  this._state = defaultCanvasState();
  this._stack = [];
  this._ops = []; // Recorded vector operations
  this._bitmap = null;
  this._bitmapWidth = 0;
  this._bitmapHeight = 0;
}

// State stack methods
CanvasRenderingContext2D.prototype.save = function () {
  checkCtx(this);
  this._stack.push(cloneCanvasState(this._state));
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "save" });
  }
};

CanvasRenderingContext2D.prototype.restore = function () {
  checkCtx(this);
  if (this._stack.length > 0) {
    this._state = this._stack.pop();
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "restore" });
    }
  }
};

// Transform methods
CanvasRenderingContext2D.prototype.scale = function (x, y) {
  checkCtx(this);
  this._state.matrix[0] *= x;
  this._state.matrix[3] *= y;
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "scale", x: x, y: y });
  }
};

CanvasRenderingContext2D.prototype.rotate = function (angle) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "rotate", angle: angle });
  }
};

CanvasRenderingContext2D.prototype.translate = function (x, y) {
  checkCtx(this);
  this._state.matrix[4] += x;
  this._state.matrix[5] += y;
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "translate", x: x, y: y });
  }
};

CanvasRenderingContext2D.prototype.transform = function (a, b, c, d, e, f) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "transform", args: [a, b, c, d, e, f] });
  }
};

CanvasRenderingContext2D.prototype.setTransform = function (a, b, c, d, e, f) {
  checkCtx(this);
  if (typeof a === "object" && a !== null) {
    this._state.matrix = [a.a || 1, a.b || 0, a.c || 0, a.d || 1, a.e || 0, a.f || 0];
  } else {
    this._state.matrix = [a || 1, b || 0, c || 0, d || 1, e || 0, f || 0];
  }
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "setTransform", matrix: this._state.matrix.slice() });
  }
};

CanvasRenderingContext2D.prototype.resetTransform = function () {
  checkCtx(this);
  this._state.matrix = [1, 0, 0, 1, 0, 0];
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "resetTransform" });
  }
};

CanvasRenderingContext2D.prototype.getTransform = function () {
  checkCtx(this);
  const m = this._state.matrix;
  return { a: m[0], b: m[1], c: m[2], d: m[3], e: m[4], f: m[5] };
};

// Rectangle drawing
CanvasRenderingContext2D.prototype.clearRect = function (x, y, w, h) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "clearRect", args: [x, y, w, h] });
  }

  const numX = Number(x);
  const numY = Number(y);
  let numW = Number(w);
  let numH = Number(h);
  if (!Number.isFinite(numX) || !Number.isFinite(numY) || !Number.isFinite(numW) || !Number.isFinite(numH)) return;
  if (numW === 0 || numH === 0) return;

  const bitmap = ensureBitmap(this);
  const bw = this._bitmapWidth;
  const bh = this._bitmapHeight;

  let startX = numX;
  let startY = numY;
  if (numW < 0) {
    startX += numW;
    numW = -numW;
  }
  if (numH < 0) {
    startY += numH;
    numH = -numH;
  }

  const x0 = Math.max(0, Math.floor(startX));
  const y0 = Math.max(0, Math.floor(startY));
  const x1 = Math.min(bw, Math.ceil(startX + numW));
  const y1 = Math.min(bh, Math.ceil(startY + numH));

  for (let py = y0; py < y1; py++) {
    let idx = (py * bw + x0) * 4;
    for (let px = x0; px < x1; px++) {
      bitmap[idx] = 0;
      bitmap[idx + 1] = 0;
      bitmap[idx + 2] = 0;
      bitmap[idx + 3] = 0;
      idx += 4;
    }
  }
};

CanvasRenderingContext2D.prototype.fillRect = function (x, y, w, h) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "fillRect", args: [x, y, w, h], fillStyle: this._state.fillStyle });
  }

  const numX = Number(x);
  const numY = Number(y);
  let numW = Number(w);
  let numH = Number(h);
  if (!Number.isFinite(numX) || !Number.isFinite(numY) || !Number.isFinite(numW) || !Number.isFinite(numH)) return;
  if (numW === 0 || numH === 0) return;

  const color = parseColor(this._state.fillStyle);
  if (!color) return;

  const bitmap = ensureBitmap(this);
  const bw = this._bitmapWidth;
  const bh = this._bitmapHeight;

  let startX = numX;
  let startY = numY;
  if (numW < 0) {
    startX += numW;
    numW = -numW;
  }
  if (numH < 0) {
    startY += numH;
    numH = -numH;
  }

  const x0 = Math.max(0, Math.floor(startX));
  const y0 = Math.max(0, Math.floor(startY));
  const x1 = Math.min(bw, Math.ceil(startX + numW));
  const y1 = Math.min(bh, Math.ceil(startY + numH));

  const alpha = color[3] * this._state.globalAlpha;
  if (alpha <= 0) return;

  const rByte = Math.min(255, Math.max(0, Math.round(color[0])));
  const gByte = Math.min(255, Math.max(0, Math.round(color[1])));
  const bByte = Math.min(255, Math.max(0, Math.round(color[2])));
  const aByte = Math.min(255, Math.max(0, Math.round(alpha * 255)));

  if (alpha >= 1.0) {
    for (let py = y0; py < y1; py++) {
      let idx = (py * bw + x0) * 4;
      for (let px = x0; px < x1; px++) {
        bitmap[idx] = rByte;
        bitmap[idx + 1] = gByte;
        bitmap[idx + 2] = bByte;
        bitmap[idx + 3] = 255;
        idx += 4;
      }
    }
  } else {
    const invA = 1.0 - alpha;
    for (let py = y0; py < y1; py++) {
      let idx = (py * bw + x0) * 4;
      for (let px = x0; px < x1; px++) {
        const dstR = bitmap[idx];
        const dstG = bitmap[idx + 1];
        const dstB = bitmap[idx + 2];
        const dstA = bitmap[idx + 3] / 255.0;

        const outA = alpha + dstA * invA;
        if (outA > 0) {
          bitmap[idx] = Math.round((color[0] * alpha + dstR * dstA * invA) / outA);
          bitmap[idx + 1] = Math.round((color[1] * alpha + dstG * dstA * invA) / outA);
          bitmap[idx + 2] = Math.round((color[2] * alpha + dstB * dstA * invA) / outA);
          bitmap[idx + 3] = Math.round(outA * 255);
        }
        idx += 4;
      }
    }
  }
};

CanvasRenderingContext2D.prototype.strokeRect = function (x, y, w, h) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "strokeRect", args: [x, y, w, h], strokeStyle: this._state.strokeStyle, lineWidth: this._state.lineWidth });
  }
};

// Path drawing methods
CanvasRenderingContext2D.prototype.beginPath = function () {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "beginPath" });
  }
};

CanvasRenderingContext2D.prototype.closePath = function () {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "closePath" });
  }
};

CanvasRenderingContext2D.prototype.moveTo = function (x, y) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "moveTo", x: x, y: y });
  }
};

CanvasRenderingContext2D.prototype.lineTo = function (x, y) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "lineTo", x: x, y: y });
  }
};

CanvasRenderingContext2D.prototype.quadraticCurveTo = function (cpx, cpy, x, y) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "quadraticCurveTo", args: [cpx, cpy, x, y] });
  }
};

CanvasRenderingContext2D.prototype.bezierCurveTo = function (cp1x, cp1y, cp2x, cp2y, x, y) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "bezierCurveTo", args: [cp1x, cp1y, cp2x, cp2y, x, y] });
  }
};

CanvasRenderingContext2D.prototype.arcTo = function (x1, y1, x2, y2, radius) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "arcTo", args: [x1, y1, x2, y2, radius] });
  }
};

CanvasRenderingContext2D.prototype.arc = function (x, y, radius, startAngle, endAngle, counterclockwise) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "arc", args: [x, y, radius, startAngle, endAngle, Boolean(counterclockwise)] });
  }
};

CanvasRenderingContext2D.prototype.ellipse = function (x, y, radiusX, radiusY, rotation, startAngle, endAngle, counterclockwise) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "ellipse", args: [x, y, radiusX, radiusY, rotation, startAngle, endAngle, Boolean(counterclockwise)] });
  }
};

CanvasRenderingContext2D.prototype.rect = function (x, y, w, h) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "rect", args: [x, y, w, h] });
  }
};

CanvasRenderingContext2D.prototype.fill = function (pathOrRule, rule) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "fill", fillStyle: this._state.fillStyle });
  }
};

CanvasRenderingContext2D.prototype.stroke = function (path) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "stroke", strokeStyle: this._state.strokeStyle, lineWidth: this._state.lineWidth });
  }
};

CanvasRenderingContext2D.prototype.clip = function (pathOrRule, rule) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "clip" });
  }
};

CanvasRenderingContext2D.prototype.isPointInPath = function (x, y) { checkCtx(this); return true; };
CanvasRenderingContext2D.prototype.isPointInStroke = function (x, y) { checkCtx(this); return true; };

// Text drawing
CanvasRenderingContext2D.prototype.fillText = function (text, x, y, maxWidth) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "fillText", text: String(text), x: x, y: y, maxWidth: maxWidth, font: this._state.font, fillStyle: this._state.fillStyle });
  }
};

CanvasRenderingContext2D.prototype.strokeText = function (text, x, y, maxWidth) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "strokeText", text: String(text), x: x, y: y, maxWidth: maxWidth, font: this._state.font, strokeStyle: this._state.strokeStyle });
  }
};

CanvasRenderingContext2D.prototype.measureText = function (text) {
  checkCtx(this);
  return new TextMetrics(text, this._state.font);
};

// Image drawing
CanvasRenderingContext2D.prototype.drawImage = function (image) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "drawImage", args: Array.prototype.slice.call(arguments, 1) });
  }
};

// Gradients and Patterns
CanvasRenderingContext2D.prototype.createLinearGradient = function (x0, y0, x1, y1) {
  checkCtx(this);
  return new CanvasGradient("linear", x0, y0, 0, x1, y1, 0);
};

CanvasRenderingContext2D.prototype.createRadialGradient = function (x0, y0, r0, x1, y1, r1) {
  checkCtx(this);
  return new CanvasGradient("radial", x0, y0, r0, x1, y1, r1);
};

CanvasRenderingContext2D.prototype.createPattern = function (image, repetition) {
  checkCtx(this);
  return new CanvasPattern(image, repetition);
};

// Pixel manipulation
CanvasRenderingContext2D.prototype.createImageData = function (sw, sh) {
  checkCtx(this);
  if (typeof sw === "object" && sw !== null) {
    const w = (sw.width | 0);
    const h = (sw.height | 0);
    return new ImageData(Math.abs(w) || 1, Math.abs(h) || 1);
  }
  const w = (sw | 0);
  const h = (sh | 0);
  return new ImageData(Math.abs(w) || 1, Math.abs(h) || 1);
};

CanvasRenderingContext2D.prototype.getImageData = function (sx, sy, sw, sh) {
  checkCtx(this);
  let intSx = (sx | 0);
  let intSy = (sy | 0);
  let intSw = (sw | 0);
  let intSh = (sh | 0);

  if (intSw === 0 || intSh === 0) {
    const err = new Error("IndexSizeError");
    err.name = "IndexSizeError";
    throw err;
  }

  if (intSw < 0) {
    intSx += intSw;
    intSw = -intSw;
  }
  if (intSh < 0) {
    intSy += intSh;
    intSh = -intSh;
  }

  const imgData = new ImageData(intSw, intSh);
  const outData = imgData.data;

  const bitmap = ensureBitmap(this);
  const bw = this._bitmapWidth;
  const bh = this._bitmapHeight;

  for (let y = 0; y < intSh; y++) {
    const srcY = intSy + y;
    for (let x = 0; x < intSw; x++) {
      const srcX = intSx + x;
      const dstIdx = (y * intSw + x) * 4;
      if (srcX >= 0 && srcX < bw && srcY >= 0 && srcY < bh) {
        const srcIdx = (srcY * bw + srcX) * 4;
        outData[dstIdx] = bitmap[srcIdx];
        outData[dstIdx + 1] = bitmap[srcIdx + 1];
        outData[dstIdx + 2] = bitmap[srcIdx + 2];
        outData[dstIdx + 3] = bitmap[srcIdx + 3];
      } else {
        outData[dstIdx] = 0;
        outData[dstIdx + 1] = 0;
        outData[dstIdx + 2] = 0;
        outData[dstIdx + 3] = 0;
      }
    }
  }

  return imgData;
};

CanvasRenderingContext2D.prototype.putImageData = function (imagedata, dx, dy) {
  checkCtx(this);
  if (!imagedata || !imagedata.data) {
    throw new TypeError("Failed to execute 'putImageData' on 'CanvasRenderingContext2D': parameter 1 is not of type 'ImageData'.");
  }
  const intDx = (dx | 0);
  const intDy = (dy | 0);
  const intW = (imagedata.width | 0);
  const intH = (imagedata.height | 0);
  const inData = imagedata.data;

  const bitmap = ensureBitmap(this);
  const bw = this._bitmapWidth;
  const bh = this._bitmapHeight;

  for (let y = 0; y < intH; y++) {
    const dstY = intDy + y;
    if (dstY < 0 || dstY >= bh) continue;
    for (let x = 0; x < intW; x++) {
      const dstX = intDx + x;
      if (dstX < 0 || dstX >= bw) continue;
      const srcIdx = (y * intW + x) * 4;
      const dstIdx = (dstY * bw + dstX) * 4;
      bitmap[dstIdx] = inData[srcIdx];
      bitmap[dstIdx + 1] = inData[srcIdx + 1];
      bitmap[dstIdx + 2] = inData[srcIdx + 2];
      bitmap[dstIdx + 3] = inData[srcIdx + 3];
    }
  }

  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "putImageData", dx: intDx, dy: intDy, width: intW, height: intH });
  }
};

// Dash styling
CanvasRenderingContext2D.prototype.setLineDash = function (segments) {
  checkCtx(this);
  this._state.lineDash = Array.isArray(segments) ? segments.slice() : [];
};

CanvasRenderingContext2D.prototype.getLineDash = function () {
  checkCtx(this);
  return this._state.lineDash.slice();
};

CanvasRenderingContext2D.prototype._setPaintStyle = function (which, v) {
  if (typeof v === "string") {
    const parsed = parseColor(v);
    if (!parsed) return;
    const formatted = formatColor(parsed);
    this._state[which] = formatted;
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: which, color: formatted });
    }
  } else if (v instanceof CanvasGradient || v instanceof CanvasPattern) {
    this._state[which] = v;
  }
};

// Properties with getters and setters
const stateProps = [
  "lineWidth", "lineCap", "lineJoin", "miterLimit",
  "lineDashOffset", "globalAlpha", "globalCompositeOperation", "shadowColor",
  "shadowBlur", "shadowOffsetX", "shadowOffsetY", "font", "textAlign",
  "textBaseline", "direction", "filter", "imageSmoothingEnabled"
];

for (let i = 0; i < stateProps.length; i++) {
  const p = stateProps[i];
  Object.defineProperty(CanvasRenderingContext2D.prototype, p, {
    get: function () {
      checkCtx(this);
      return this._state[p];
    },
    set: function (v) {
      checkCtx(this);
      this._state[p] = v;
    },
    configurable: true,
    enumerable: true,
  });
}

Object.defineProperty(CanvasRenderingContext2D.prototype, "fillStyle", {
  get: function () {
    checkCtx(this);
    return this._state.fillStyle;
  },
  set: function (v) {
    checkCtx(this);
    this._setPaintStyle("fillStyle", v);
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(CanvasRenderingContext2D.prototype, "strokeStyle", {
  get: function () {
    checkCtx(this);
    return this._state.strokeStyle;
  },
  set: function (v) {
    checkCtx(this);
    this._setPaintStyle("strokeStyle", v);
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(CanvasRenderingContext2D.prototype, "canvas", {
  get: function () {
    checkCtx(this);
    return this._canvas;
  },
  configurable: true,
  enumerable: true,
});

// Vector recording extraction and inspection API for CapSurf
CanvasRenderingContext2D.prototype.__getRecordingOps = function () {
  checkCtx(this);
  return this._ops.slice();
};

CanvasRenderingContext2D.prototype.__clearRecordingOps = function () {
  checkCtx(this);
  this._ops = [];
};

CanvasRenderingContext2D.prototype.__opsCount = function () {
  checkCtx(this);
  return this._ops.length;
};

markNative(CanvasRenderingContext2D, "CanvasRenderingContext2D");
markNative(CanvasRenderingContext2D.prototype.save, "save");
markNative(CanvasRenderingContext2D.prototype.restore, "restore");
markNative(CanvasRenderingContext2D.prototype.fillRect, "fillRect");
markNative(CanvasRenderingContext2D.prototype.strokeRect, "strokeRect");
markNative(CanvasRenderingContext2D.prototype.clearRect, "clearRect");
markNative(CanvasRenderingContext2D.prototype.measureText, "measureText");
markNative(CanvasRenderingContext2D.prototype.getImageData, "getImageData");
