// src/js/shim/canvas2d.js
// Aligned with vendor/kitesurf/packages/dom-shim/src/canvas2d.ts
// Cloud vector recording and rendering engine for HTML5 Canvas 2D.
// Configured by globalThis.__CAP_RENDER_CONFIG.canvasRecording (true: record ops, false: headless fast stub).

const VALID_PNG_BASE64 = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=";

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

function CanvasRenderingContext2D(canvas) {
  this.canvas = canvas || null;
  this._state = defaultCanvasState();
  this._stack = [];
  this._ops = []; // Recorded vector operations
}

// State stack methods
CanvasRenderingContext2D.prototype.save = function () {
  this._stack.push(cloneCanvasState(this._state));
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "save" });
  }
};

CanvasRenderingContext2D.prototype.restore = function () {
  if (this._stack.length > 0) {
    this._state = this._stack.pop();
    if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
      this._ops.push({ op: "restore" });
    }
  }
};

// Transform methods
CanvasRenderingContext2D.prototype.scale = function (x, y) {
  this._state.matrix[0] *= x;
  this._state.matrix[3] *= y;
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "scale", x: x, y: y });
  }
};

CanvasRenderingContext2D.prototype.rotate = function (angle) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "rotate", angle: angle });
  }
};

CanvasRenderingContext2D.prototype.translate = function (x, y) {
  this._state.matrix[4] += x;
  this._state.matrix[5] += y;
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "translate", x: x, y: y });
  }
};

CanvasRenderingContext2D.prototype.transform = function (a, b, c, d, e, f) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "transform", args: [a, b, c, d, e, f] });
  }
};

CanvasRenderingContext2D.prototype.setTransform = function (a, b, c, d, e, f) {
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
  this._state.matrix = [1, 0, 0, 1, 0, 0];
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "resetTransform" });
  }
};

CanvasRenderingContext2D.prototype.getTransform = function () {
  const m = this._state.matrix;
  return { a: m[0], b: m[1], c: m[2], d: m[3], e: m[4], f: m[5] };
};

// Rectangle drawing
CanvasRenderingContext2D.prototype.clearRect = function (x, y, w, h) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "clearRect", args: [x, y, w, h] });
  }
};

CanvasRenderingContext2D.prototype.fillRect = function (x, y, w, h) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "fillRect", args: [x, y, w, h], fillStyle: this._state.fillStyle });
  }
};

CanvasRenderingContext2D.prototype.strokeRect = function (x, y, w, h) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "strokeRect", args: [x, y, w, h], strokeStyle: this._state.strokeStyle, lineWidth: this._state.lineWidth });
  }
};

// Path drawing methods
CanvasRenderingContext2D.prototype.beginPath = function () {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "beginPath" });
  }
};

CanvasRenderingContext2D.prototype.closePath = function () {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "closePath" });
  }
};

CanvasRenderingContext2D.prototype.moveTo = function (x, y) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "moveTo", x: x, y: y });
  }
};

CanvasRenderingContext2D.prototype.lineTo = function (x, y) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "lineTo", x: x, y: y });
  }
};

CanvasRenderingContext2D.prototype.quadraticCurveTo = function (cpx, cpy, x, y) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "quadraticCurveTo", args: [cpx, cpy, x, y] });
  }
};

CanvasRenderingContext2D.prototype.bezierCurveTo = function (cp1x, cp1y, cp2x, cp2y, x, y) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "bezierCurveTo", args: [cp1x, cp1y, cp2x, cp2y, x, y] });
  }
};

CanvasRenderingContext2D.prototype.arcTo = function (x1, y1, x2, y2, radius) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "arcTo", args: [x1, y1, x2, y2, radius] });
  }
};

CanvasRenderingContext2D.prototype.arc = function (x, y, radius, startAngle, endAngle, counterclockwise) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "arc", args: [x, y, radius, startAngle, endAngle, Boolean(counterclockwise)] });
  }
};

CanvasRenderingContext2D.prototype.ellipse = function (x, y, radiusX, radiusY, rotation, startAngle, endAngle, counterclockwise) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "ellipse", args: [x, y, radiusX, radiusY, rotation, startAngle, endAngle, Boolean(counterclockwise)] });
  }
};

CanvasRenderingContext2D.prototype.rect = function (x, y, w, h) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "rect", args: [x, y, w, h] });
  }
};

CanvasRenderingContext2D.prototype.fill = function (pathOrRule, rule) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "fill", fillStyle: this._state.fillStyle });
  }
};

CanvasRenderingContext2D.prototype.stroke = function (path) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "stroke", strokeStyle: this._state.strokeStyle, lineWidth: this._state.lineWidth });
  }
};

CanvasRenderingContext2D.prototype.clip = function (pathOrRule, rule) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "clip" });
  }
};

CanvasRenderingContext2D.prototype.isPointInPath = function (x, y) { return true; };
CanvasRenderingContext2D.prototype.isPointInStroke = function (x, y) { return true; };

// Text drawing
CanvasRenderingContext2D.prototype.fillText = function (text, x, y, maxWidth) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "fillText", text: String(text), x: x, y: y, maxWidth: maxWidth, font: this._state.font, fillStyle: this._state.fillStyle });
  }
};

CanvasRenderingContext2D.prototype.strokeText = function (text, x, y, maxWidth) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "strokeText", text: String(text), x: x, y: y, maxWidth: maxWidth, font: this._state.font, strokeStyle: this._state.strokeStyle });
  }
};

CanvasRenderingContext2D.prototype.measureText = function (text) {
  return new TextMetrics(text, this._state.font);
};

// Image drawing
CanvasRenderingContext2D.prototype.drawImage = function (image) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "drawImage", args: Array.prototype.slice.call(arguments, 1) });
  }
};

// Gradients and Patterns
CanvasRenderingContext2D.prototype.createLinearGradient = function (x0, y0, x1, y1) {
  return new CanvasGradient("linear", x0, y0, 0, x1, y1, 0);
};

CanvasRenderingContext2D.prototype.createRadialGradient = function (x0, y0, r0, x1, y1, r1) {
  return new CanvasGradient("radial", x0, y0, r0, x1, y1, r1);
};

CanvasRenderingContext2D.prototype.createPattern = function (image, repetition) {
  return new CanvasPattern(image, repetition);
};

// Pixel manipulation
CanvasRenderingContext2D.prototype.createImageData = function (sw, sh) {
  return new ImageData(sw, sh);
};

CanvasRenderingContext2D.prototype.getImageData = function (sx, sy, sw, sh) {
  return new ImageData(sw, sh);
};

CanvasRenderingContext2D.prototype.putImageData = function (imagedata, dx, dy) {
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "putImageData", dx: dx, dy: dy, width: imagedata.width, height: imagedata.height });
  }
};

// Dash styling
CanvasRenderingContext2D.prototype.setLineDash = function (segments) {
  this._state.lineDash = Array.isArray(segments) ? segments.slice() : [];
};

CanvasRenderingContext2D.prototype.getLineDash = function () {
  return this._state.lineDash.slice();
};

// Properties with getters and setters
const stateProps = [
  "strokeStyle", "fillStyle", "lineWidth", "lineCap", "lineJoin", "miterLimit",
  "lineDashOffset", "globalAlpha", "globalCompositeOperation", "shadowColor",
  "shadowBlur", "shadowOffsetX", "shadowOffsetY", "font", "textAlign",
  "textBaseline", "direction", "filter", "imageSmoothingEnabled"
];

for (let i = 0; i < stateProps.length; i++) {
  const p = stateProps[i];
  Object.defineProperty(CanvasRenderingContext2D.prototype, p, {
    get: function () { return this._state[p]; },
    set: function (v) { this._state[p] = v; },
    configurable: true,
  });
}

// Vector recording extraction and inspection API for CapSurf
CanvasRenderingContext2D.prototype.__getRecordingOps = function () {
  return this._ops.slice();
};

CanvasRenderingContext2D.prototype.__clearRecordingOps = function () {
  this._ops = [];
};

CanvasRenderingContext2D.prototype.__opsCount = function () {
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
