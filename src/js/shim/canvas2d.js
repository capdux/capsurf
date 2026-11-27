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
  const raw = canvas.width !== undefined ? canvas.width : (typeof canvas.getAttribute === "function" ? canvas.getAttribute("width") : null);
  const w = parseInt(raw, 10);
  return Number.isFinite(w) && w > 0 ? w : 300;
}

function getCanvasHeight(canvas) {
  if (!canvas) return 150;
  const raw = canvas.height !== undefined ? canvas.height : (typeof canvas.getAttribute === "function" ? canvas.getAttribute("height") : null);
  const h = parseInt(raw, 10);
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

function _mulMatrix2D(m1, m2) {
  return [
    m1[0] * m2[0] + m1[2] * m2[1],
    m1[1] * m2[0] + m1[3] * m2[1],
    m1[0] * m2[2] + m1[2] * m2[3],
    m1[1] * m2[2] + m1[3] * m2[3],
    m1[0] * m2[4] + m1[2] * m2[5] + m1[4],
    m1[1] * m2[4] + m1[3] * m2[5] + m1[5],
  ];
}

function _parseSvgPathData(d) {
  const ops = [];
  const str = String(d || "");
  const tokens = str.match(/[a-zA-Z]|[-+]?(?:\d*\.\d+|\d+\.?)(?:[eE][-+]?\d+)?/g);
  if (!tokens) return ops;

  let i = 0;
  let cmd = "";
  let cx = 0;
  let cy = 0;
  let sx = 0;
  let sy = 0;
  let lastCpX = 0;
  let lastCpY = 0;
  let lastCmd = "";

  function isNum(tok) {
    return tok !== undefined && !/^[a-zA-Z]$/.test(tok);
  }
  function nextNum() {
    return Number(tokens[i++]);
  }

  while (i < tokens.length) {
    if (!isNum(tokens[i])) {
      cmd = tokens[i++];
      if (cmd === "Z" || cmd === "z") {
        ops.push({ op: "closePath" });
        cx = sx;
        cy = sy;
        lastCmd = cmd;
        continue;
      }
    } else if (!cmd) {
      break;
    }

    const rel = cmd === cmd.toLowerCase();
    const upper = cmd.toUpperCase();

    if (upper === "M") {
      if (!isNum(tokens[i])) break;
      let first = true;
      while (isNum(tokens[i])) {
        const nx = nextNum();
        if (!isNum(tokens[i])) break;
        const ny = nextNum();
        cx = rel ? cx + nx : nx;
        cy = rel ? cy + ny : ny;
        if (first) {
          ops.push({ op: "moveTo", args: [cx, cy] });
          sx = cx;
          sy = cy;
          first = false;
          cmd = rel ? "l" : "L";
        } else {
          ops.push({ op: "lineTo", args: [cx, cy] });
        }
      }
      lastCmd = upper;
    } else if (upper === "L") {
      while (isNum(tokens[i])) {
        const nx = nextNum();
        if (!isNum(tokens[i])) break;
        const ny = nextNum();
        cx = rel ? cx + nx : nx;
        cy = rel ? cy + ny : ny;
        ops.push({ op: "lineTo", args: [cx, cy] });
      }
      lastCmd = upper;
    } else if (upper === "H") {
      while (isNum(tokens[i])) {
        const nx = nextNum();
        cx = rel ? cx + nx : nx;
        ops.push({ op: "lineTo", args: [cx, cy] });
      }
      lastCmd = upper;
    } else if (upper === "V") {
      while (isNum(tokens[i])) {
        const ny = nextNum();
        cy = rel ? cy + ny : ny;
        ops.push({ op: "lineTo", args: [cx, cy] });
      }
      lastCmd = upper;
    } else if (upper === "C") {
      while (isNum(tokens[i])) {
        const x1 = nextNum(), y1 = nextNum(), x2 = nextNum(), y2 = nextNum(), x = nextNum(), y = nextNum();
        if (Number.isNaN(y)) break;
        const ax1 = rel ? cx + x1 : x1, ay1 = rel ? cy + y1 : y1;
        const ax2 = rel ? cx + x2 : x2, ay2 = rel ? cy + y2 : y2;
        cx = rel ? cx + x : x;
        cy = rel ? cy + y : y;
        lastCpX = ax2;
        lastCpY = ay2;
        ops.push({ op: "bezierCurveTo", args: [ax1, ay1, ax2, ay2, cx, cy] });
      }
      lastCmd = "C";
    } else if (upper === "S") {
      while (isNum(tokens[i])) {
        const x2 = nextNum(), y2 = nextNum(), x = nextNum(), y = nextNum();
        if (Number.isNaN(y)) break;
        const ax1 = (lastCmd === "C" || lastCmd === "S") ? (2 * cx - lastCpX) : cx;
        const ay1 = (lastCmd === "C" || lastCmd === "S") ? (2 * cy - lastCpY) : cy;
        const ax2 = rel ? cx + x2 : x2, ay2 = rel ? cy + y2 : y2;
        cx = rel ? cx + x : x;
        cy = rel ? cy + y : y;
        lastCpX = ax2;
        lastCpY = ay2;
        ops.push({ op: "bezierCurveTo", args: [ax1, ay1, ax2, ay2, cx, cy] });
      }
      lastCmd = "S";
    } else if (upper === "Q") {
      while (isNum(tokens[i])) {
        const x1 = nextNum(), y1 = nextNum(), x = nextNum(), y = nextNum();
        if (Number.isNaN(y)) break;
        const ax1 = rel ? cx + x1 : x1, ay1 = rel ? cy + y1 : y1;
        cx = rel ? cx + x : x;
        cy = rel ? cy + y : y;
        lastCpX = ax1;
        lastCpY = ay1;
        ops.push({ op: "quadraticCurveTo", args: [ax1, ay1, cx, cy] });
      }
      lastCmd = "Q";
    } else if (upper === "T") {
      while (isNum(tokens[i])) {
        const x = nextNum(), y = nextNum();
        if (Number.isNaN(y)) break;
        const ax1 = (lastCmd === "Q" || lastCmd === "T") ? (2 * cx - lastCpX) : cx;
        const ay1 = (lastCmd === "Q" || lastCmd === "T") ? (2 * cy - lastCpY) : cy;
        cx = rel ? cx + x : x;
        cy = rel ? cy + y : y;
        lastCpX = ax1;
        lastCpY = ay1;
        ops.push({ op: "quadraticCurveTo", args: [ax1, ay1, cx, cy] });
      }
      lastCmd = "T";
    } else if (upper === "A") {
      while (isNum(tokens[i])) {
        nextNum(); nextNum(); nextNum(); nextNum(); nextNum();
        const x = nextNum(), y = nextNum();
        if (Number.isNaN(y)) break;
        cx = rel ? cx + x : x;
        cy = rel ? cy + y : y;
        ops.push({ op: "lineTo", args: [cx, cy] });
      }
      lastCmd = "A";
    } else {
      break;
    }
  }
  return ops;
}

function checkPath2D(self) {
  if (!(self instanceof Path2D) || !Array.isArray(self._ops)) {
    throw new TypeError("Illegal invocation");
  }
}

function Path2D(path) {
  this._ops = [];
  if (path instanceof Path2D) {
    this._ops = path._ops.slice();
  } else if (typeof path === "string") {
    this._ops = _parseSvgPathData(path);
  }
}
const pathMethods = ["moveTo", "lineTo", "quadraticCurveTo", "bezierCurveTo", "arcTo", "arc", "ellipse", "rect", "closePath"];
for (let i = 0; i < pathMethods.length; i++) {
  const m = pathMethods[i];
  Path2D.prototype[m] = function () {
    checkPath2D(this);
    this._ops.push({ op: m, args: Array.prototype.slice.call(arguments) });
  };
}
Path2D.prototype.addPath = function (path, transform) {
  checkPath2D(this);
  if (!(path instanceof Path2D)) {
    throw new TypeError("Failed to execute 'addPath' on 'Path2D': parameter 1 is not of type 'Path2D'.");
  }
  let a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;
  if (transform && typeof transform === "object") {
    if (transform.a !== undefined) a = Number(transform.a);
    else if (transform.m11 !== undefined) a = Number(transform.m11);
    if (transform.b !== undefined) b = Number(transform.b);
    else if (transform.m12 !== undefined) b = Number(transform.m12);
    if (transform.c !== undefined) c = Number(transform.c);
    else if (transform.m21 !== undefined) c = Number(transform.m21);
    if (transform.d !== undefined) d = Number(transform.d);
    else if (transform.m22 !== undefined) d = Number(transform.m22);
    if (transform.e !== undefined) e = Number(transform.e);
    else if (transform.m41 !== undefined) e = Number(transform.m41);
    if (transform.f !== undefined) f = Number(transform.f);
    else if (transform.m42 !== undefined) f = Number(transform.m42);
  }
  if (!Number.isFinite(a) || !Number.isFinite(b) || !Number.isFinite(c) ||
      !Number.isFinite(d) || !Number.isFinite(e) || !Number.isFinite(f)) {
    return;
  }
  this._ops.push({ op: "addPath", pathOps: path._ops.slice(), matrix: [a, b, c, d, e, f] });
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
    if (w.length === 0 || w.length % 4 !== 0) {
      throw new DOMException("The input data length is not a nonzero multiple of 4.", "InvalidStateError");
    }
    const width = Number(h) >>> 0;
    if (width === 0 || (w.length / 4) % width !== 0) {
      throw new DOMException("The input data length is not a multiple of (4 * width).", "IndexSizeError");
    }
    const height = data === undefined ? (w.length / 4 / width) : (Number(data) >>> 0);
    if (height * width * 4 !== w.length) {
      throw new DOMException("The input data length is not equal to (4 * width * height).", "IndexSizeError");
    }
    this.data = w;
    this.width = width;
    this.height = height;
  } else {
    const width = Number(w) >>> 0;
    const height = Number(h) >>> 0;
    if (width === 0 || height === 0) {
      throw new DOMException("The source width and height must be non-zero.", "IndexSizeError");
    }
    this.width = width;
    this.height = height;
    this.data = data instanceof Uint8ClampedArray ? data : new Uint8ClampedArray(this.width * this.height * 4);
  }
  this.colorSpace = "srgb";
}
markNative(ImageData, "ImageData");

const FONT_8X8 = {
  'H': [0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00],
  'i': [0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00],
  'default': [0x7e, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x7e, 0x00],
};

function encodePngBase64(width, height, rgba) {
  if (!encodePngBase64._crcTable) {
    const table = new Uint32Array(256);
    for (let i = 0; i < 256; i++) {
      let c = i;
      for (let k = 0; k < 8; k++) {
        c = (c & 1) ? (0xedb88320 ^ (c >>> 1)) : (c >>> 1);
      }
      table[i] = c;
    }
    encodePngBase64._crcTable = table;
  }
  const crcTable = encodePngBase64._crcTable;

  function crc32(buf, offset, length) {
    let c = 0xffffffff;
    for (let i = 0; i < length; i++) {
      c = crcTable[(c ^ buf[offset + i]) & 0xff] ^ (c >>> 8);
    }
    return (c ^ 0xffffffff) >>> 0;
  }

  function adler32(buf, offset, length) {
    let a = 1, b = 0;
    for (let i = 0; i < length; i++) {
      a = (a + buf[offset + i]) % 65521;
      b = (b + a) % 65521;
    }
    return ((b << 16) | a) >>> 0;
  }

  const lineLen = 1 + width * 4;
  const rawLen = height * lineLen;
  const raw = new Uint8Array(rawLen);
  let rawIdx = 0;
  let rgbaIdx = 0;
  for (let y = 0; y < height; y++) {
    raw[rawIdx++] = 0;
    for (let x = 0; x < width * 4; x++) {
      raw[rawIdx++] = rgba[rgbaIdx++];
    }
  }

  const MAX_BLOCK = 65535;
  const numBlocks = Math.ceil(rawLen / MAX_BLOCK) || 1;
  const zlibLen = 2 + numBlocks * 5 + rawLen + 4;
  const zlib = new Uint8Array(zlibLen);
  let zIdx = 0;
  zlib[zIdx++] = 0x78;
  zlib[zIdx++] = 0x01;
  let remaining = rawLen;
  let inPos = 0;
  while (remaining > 0 || numBlocks === 1 && inPos === 0) {
    const bLen = Math.min(remaining, MAX_BLOCK);
    const bFinal = (remaining <= MAX_BLOCK) ? 1 : 0;
    zlib[zIdx++] = bFinal;
    zlib[zIdx++] = bLen & 0xff;
    zlib[zIdx++] = (bLen >> 8) & 0xff;
    zlib[zIdx++] = (~bLen) & 0xff;
    zlib[zIdx++] = ((~bLen) >> 8) & 0xff;
    zlib.set(raw.subarray(inPos, inPos + bLen), zIdx);
    zIdx += bLen;
    inPos += bLen;
    remaining -= bLen;
  }
  const adler = adler32(raw, 0, rawLen);
  zlib[zIdx++] = (adler >> 24) & 0xff;
  zlib[zIdx++] = (adler >> 16) & 0xff;
  zlib[zIdx++] = (adler >> 8) & 0xff;
  zlib[zIdx++] = adler & 0xff;

  const totalLen = 8 + 25 + 12 + zlibLen + 12;
  const png = new Uint8Array(totalLen);
  let pIdx = 0;
  const sig = [137, 80, 78, 71, 13, 10, 26, 10];
  for (let i = 0; i < 8; i++) png[pIdx++] = sig[i];

  const ihdrLen = 13;
  png[pIdx++] = (ihdrLen >> 24) & 0xff;
  png[pIdx++] = (ihdrLen >> 16) & 0xff;
  png[pIdx++] = (ihdrLen >> 8) & 0xff;
  png[pIdx++] = ihdrLen & 0xff;
  const ihdrStart = pIdx;
  png[pIdx++] = 73; png[pIdx++] = 72; png[pIdx++] = 68; png[pIdx++] = 82;
  png[pIdx++] = (width >> 24) & 0xff;
  png[pIdx++] = (width >> 16) & 0xff;
  png[pIdx++] = (width >> 8) & 0xff;
  png[pIdx++] = width & 0xff;
  png[pIdx++] = (height >> 24) & 0xff;
  png[pIdx++] = (height >> 16) & 0xff;
  png[pIdx++] = (height >> 8) & 0xff;
  png[pIdx++] = height & 0xff;
  png[pIdx++] = 8; png[pIdx++] = 6; png[pIdx++] = 0; png[pIdx++] = 0; png[pIdx++] = 0;
  const ihdrCrc = crc32(png, ihdrStart, 17);
  png[pIdx++] = (ihdrCrc >> 24) & 0xff;
  png[pIdx++] = (ihdrCrc >> 16) & 0xff;
  png[pIdx++] = (ihdrCrc >> 8) & 0xff;
  png[pIdx++] = ihdrCrc & 0xff;

  png[pIdx++] = (zlibLen >> 24) & 0xff;
  png[pIdx++] = (zlibLen >> 16) & 0xff;
  png[pIdx++] = (zlibLen >> 8) & 0xff;
  png[pIdx++] = zlibLen & 0xff;
  const idatStart = pIdx;
  png[pIdx++] = 73; png[pIdx++] = 68; png[pIdx++] = 65; png[pIdx++] = 84;
  png.set(zlib, pIdx);
  pIdx += zlibLen;
  const idatCrc = crc32(png, idatStart, 4 + zlibLen);
  png[pIdx++] = (idatCrc >> 24) & 0xff;
  png[pIdx++] = (idatCrc >> 16) & 0xff;
  png[pIdx++] = (idatCrc >> 8) & 0xff;
  png[pIdx++] = idatCrc & 0xff;

  png[pIdx++] = 0; png[pIdx++] = 0; png[pIdx++] = 0; png[pIdx++] = 0;
  const iendStart = pIdx;
  png[pIdx++] = 73; png[pIdx++] = 69; png[pIdx++] = 78; png[pIdx++] = 68;
  const iendCrc = crc32(png, iendStart, 4);
  png[pIdx++] = (iendCrc >> 24) & 0xff;
  png[pIdx++] = (iendCrc >> 16) & 0xff;
  png[pIdx++] = (iendCrc >> 8) & 0xff;
  png[pIdx++] = iendCrc & 0xff;

  let binStr = "";
  for (let i = 0; i < png.length; i++) {
    binStr += String.fromCharCode(png[i]);
  }
  const b64 = (typeof btoa === "function") ? btoa(binStr) : base64Encode(png);
  return "data:image/png;base64," + b64;
}

function transformPoint(m, x, y) {
  return [
    m[0] * x + m[2] * y + m[4],
    m[1] * x + m[3] * y + m[5]
  ];
}

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
    clipRect: null,
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
    clipRect: s.clipRect ? { x0: s.clipRect.x0, y0: s.clipRect.y0, x1: s.clipRect.x1, y1: s.clipRect.y1 } : null,
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
  this._currentPath = [];
  this._bitmap = null;
  this._bitmapWidth = 0;
  this._bitmapHeight = 0;
  this._originClean = true;
  this.__ksDbg = { last: "idle", opsSent: 0, queued: 0 };
}

function recordCanvasDrawOp(ctx) {
  if (!ctx || !ctx.__ksDbg) return;
  ctx.__ksDbg.queued += 1;
  if (ctx._canvas && ctx._canvas.isConnected) {
    ctx.__ksDbg.opsSent += ctx.__ksDbg.queued;
    ctx.__ksDbg.queued = 0;
    ctx.__ksDbg.last = "flushed";
  } else {
    ctx.__ksDbg.last = "detached";
  }
}

function flushCanvasOnReadback(ctx) {
  if (!ctx || !ctx.__ksDbg) return;
  if (ctx.__ksDbg.queued > 0) {
    ctx.__ksDbg.opsSent += ctx.__ksDbg.queued;
    ctx.__ksDbg.queued = 0;
    ctx.__ksDbg.last = "readback";
  }
}

function makeSecurityError(msg) {
  if (typeof DOMException === "function") {
    return new DOMException(msg, "SecurityError");
  }
  const err = new Error(msg);
  err.name = "SecurityError";
  return err;
}

function resolveCanonicalCanvasSource(source) {
  if (!source || typeof source !== "object") return null;
  const rawId = typeof source._id === "number" ? source._id : (typeof source._nid === "number" ? source._nid : 0);
  if (rawId > 0 && typeof globalThis !== "undefined" && typeof globalThis.__capsurf_find_node_by_id === "function") {
    const canonical = globalThis.__capsurf_find_node_by_id(rawId);
    if (canonical) return canonical;
  }
  return source;
}

function getDocumentBaseUriForCanvas() {
  if (typeof document !== "undefined" && document) {
    try {
      const baseEl = typeof document.querySelector === "function" ? document.querySelector("base") : null;
      if (baseEl) {
        const hrefAttr = (typeof baseEl.getAttribute === "function" ? baseEl.getAttribute("href") : null)
          || (typeof baseEl.href === "string" ? baseEl.href : null);
        if (hrefAttr) {
          const docHref = (typeof location !== "undefined" && location && location.href) ? String(location.href) : "http://localhost/";
          return new URL(hrefAttr, docHref).href;
        }
      }
    } catch (e) {}
    if (document.baseURI) return String(document.baseURI);
  }
  if (typeof location !== "undefined" && location && location.href) return String(location.href);
  return "http://localhost/";
}

function getDocumentOriginForCanvas() {
  if (typeof location !== "undefined" && location && location.origin && location.origin !== "null" && location.origin !== "about:") {
    return String(location.origin);
  }
  if (typeof document !== "undefined" && document && document.URL) {
    try {
      return new URL(String(document.URL)).origin;
    } catch (e) {}
  }
  return "";
}

function isCanvasSourceOriginClean(realSource) {
  if (!realSource || typeof realSource !== "object") return true;
  const tag = String(realSource.localName || realSource.tagName || "").toLowerCase();
  if (tag === "canvas" || (typeof OffscreenCanvas !== "undefined" && realSource instanceof OffscreenCanvas)) {
    const srcCtx = realSource._ctx2d;
    if (srcCtx && srcCtx._originClean === false) return false;
    return true;
  }
  if (tag === "img") {
    let candidate = "";
    if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_selectImageSource === "function") {
      candidate = globalThis.__capsurf_selectImageSource(realSource);
    } else if (typeof realSource.getAttribute === "function") {
      candidate = realSource.getAttribute("src") || "";
    } else if (realSource.src) {
      candidate = String(realSource.src);
    }
    if (!candidate) return true;
    const trimmed = String(candidate).trim();
    if (/^data:/i.test(trimmed) || /^blob:/i.test(trimmed)) {
      return true;
    }
    let resolved;
    try {
      resolved = new URL(trimmed, getDocumentBaseUriForCanvas());
    } catch (e) {
      return false;
    }
    const docOrigin = getDocumentOriginForCanvas();
    if (!docOrigin) return false;
    return resolved.origin === docOrigin;
  }
  return true;
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
  const sx = Number(x);
  const sy = Number(y);
  this._state.matrix[0] *= sx;
  this._state.matrix[1] *= sx;
  this._state.matrix[2] *= sy;
  this._state.matrix[3] *= sy;
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "scale", x: sx, y: sy });
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
  const numX = Number(x);
  const numY = Number(y);
  const m = this._state.matrix;
  m[4] += m[0] * numX + m[2] * numY;
  m[5] += m[1] * numX + m[3] * numY;
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "translate", x: numX, y: numY });
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

  const [startX, startY] = transformPoint(this._state.matrix, numX, numY);
  numW = numW * (this._state.matrix[0] || 1);
  numH = numH * (this._state.matrix[3] || 1);

  let rx = numW < 0 ? startX + numW : startX;
  let ry = numH < 0 ? startY + numH : startY;
  let rw = Math.abs(numW);
  let rh = Math.abs(numH);

  let x0 = Math.max(0, Math.floor(rx));
  let y0 = Math.max(0, Math.floor(ry));
  let x1 = Math.min(bw, Math.ceil(rx + rw));
  let y1 = Math.min(bh, Math.ceil(ry + rh));

  const clip = this._state.clipRect;
  if (clip) {
    x0 = Math.max(x0, Math.floor(clip.x0));
    y0 = Math.max(y0, Math.floor(clip.y0));
    x1 = Math.min(x1, Math.ceil(clip.x1));
    y1 = Math.min(y1, Math.ceil(clip.y1));
  }
  if (x0 >= x1 || y0 >= y1) return;

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
  recordCanvasDrawOp(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "fillRect", args: [x, y, w, h], fillStyle: this._state.fillStyle });
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

  const [startX, startY] = transformPoint(this._state.matrix, numX, numY);
  numW = numW * (this._state.matrix[0] || 1);
  numH = numH * (this._state.matrix[3] || 1);

  let rx = numW < 0 ? startX + numW : startX;
  let ry = numH < 0 ? startY + numH : startY;
  let rw = Math.abs(numW);
  let rh = Math.abs(numH);

  let x0 = Math.max(0, Math.floor(rx));
  let y0 = Math.max(0, Math.floor(ry));
  let x1 = Math.min(bw, Math.ceil(rx + rw));
  let y1 = Math.min(bh, Math.ceil(ry + rh));

  const clip = this._state.clipRect;
  if (clip) {
    x0 = Math.max(x0, Math.floor(clip.x0));
    y0 = Math.max(y0, Math.floor(clip.y0));
    x1 = Math.min(x1, Math.ceil(clip.x1));
    y1 = Math.min(y1, Math.ceil(clip.y1));
  }
  if (x0 >= x1 || y0 >= y1) return;

  const fill = this._state.fillStyle;

  if (fill instanceof CanvasPattern) {
    const realImg = resolveCanonicalCanvasSource(fill._image);
    if (!isCanvasSourceOriginClean(realImg)) {
      this._originClean = false;
      return;
    }
    const solid = realImg && realImg._solidRgba ? realImg._solidRgba : null;
    if (solid) {
      for (let py = y0; py < y1; py++) {
        let idx = (py * bw + x0) * 4;
        for (let px = x0; px < x1; px++) {
          bitmap[idx] = solid[0];
          bitmap[idx + 1] = solid[1];
          bitmap[idx + 2] = solid[2];
          bitmap[idx + 3] = solid[3];
          idx += 4;
        }
      }
    }
    return;
  }

  if (fill instanceof CanvasGradient) {
    const stops = fill._stops.map(function (s) {
      return {
        offset: s.offset,
        color: parseColor(s.color) || [0, 0, 0, 1]
      };
    }).sort(function (a, b) { return a.offset - b.offset; });

    const x0Grad = fill._coords[0];
    const x1Grad = fill._coords[3];
    const dxGrad = x1Grad - x0Grad;

    for (let py = y0; py < y1; py++) {
      for (let px = x0; px < x1; px++) {
        let t = dxGrad !== 0 ? (px - x0Grad) / dxGrad : 0;
        t = Math.max(0, Math.min(1, t));

        let r = 0, g = 0, b = 0, a = 1;
        if (stops.length === 0) {
          r = 0; g = 0; b = 0; a = 1;
        } else if (stops.length === 1 || t <= stops[0].offset) {
          r = stops[0].color[0]; g = stops[0].color[1]; b = stops[0].color[2]; a = stops[0].color[3];
        } else if (t >= stops[stops.length - 1].offset) {
          const last = stops[stops.length - 1];
          r = last.color[0]; g = last.color[1]; b = last.color[2]; a = last.color[3];
        } else {
          for (let s = 0; s < stops.length - 1; s++) {
            if (t >= stops[s].offset && t <= stops[s + 1].offset) {
              const span = stops[s + 1].offset - stops[s].offset;
              const frac = span > 0 ? (t - stops[s].offset) / span : 0;
              r = Math.round(stops[s].color[0] + (stops[s + 1].color[0] - stops[s].color[0]) * frac);
              g = Math.round(stops[s].color[1] + (stops[s + 1].color[1] - stops[s].color[1]) * frac);
              b = Math.round(stops[s].color[2] + (stops[s + 1].color[2] - stops[s].color[2]) * frac);
              a = stops[s].color[3] + (stops[s + 1].color[3] - stops[s].color[3]) * frac;
              break;
            }
          }
        }
        const finalA = a * this._state.globalAlpha;
        const aByte = Math.round(finalA * 255);
        const idx = (py * bw + px) * 4;
        bitmap[idx] = r;
        bitmap[idx + 1] = g;
        bitmap[idx + 2] = b;
        bitmap[idx + 3] = aByte;
      }
    }
    return;
  }

  const color = parseColor(fill);
  if (!color) return;

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
  this._currentPath = [];
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "beginPath" });
  }
};

CanvasRenderingContext2D.prototype.closePath = function () {
  checkCtx(this);
  this._currentPath.push({ op: "closePath" });
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "closePath" });
  }
};

CanvasRenderingContext2D.prototype.moveTo = function (x, y) {
  checkCtx(this);
  const [tx, ty] = transformPoint(this._state.matrix, Number(x), Number(y));
  this._currentPath.push({ op: "moveTo", x: tx, y: ty });
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "moveTo", x: x, y: y });
  }
};

CanvasRenderingContext2D.prototype.lineTo = function (x, y) {
  checkCtx(this);
  const [tx, ty] = transformPoint(this._state.matrix, Number(x), Number(y));
  this._currentPath.push({ op: "lineTo", x: tx, y: ty });
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
  const [tx, ty] = transformPoint(this._state.matrix, Number(x), Number(y));
  const r = Number(radius) * Math.abs(this._state.matrix[0] || 1);
  this._currentPath.push({ op: "arc", x: tx, y: ty, r: r, sa: Number(startAngle), ea: Number(endAngle), ccw: Boolean(counterclockwise) });
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
  const [tx, ty] = transformPoint(this._state.matrix, Number(x), Number(y));
  const tw = Number(w) * (this._state.matrix[0] || 1);
  const th = Number(h) * (this._state.matrix[3] || 1);
  this._currentPath.push({ op: "rect", x: tx, y: ty, w: tw, h: th });
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "rect", args: [x, y, w, h] });
  }
};

function _flattenPathOps(ops, matrix, outPolygons, outArcs) {
  let curSubpath = null;
  let cx = 0;
  let cy = 0;
  function pushPt(x, y) {
    const [tx, ty] = transformPoint(matrix, x, y);
    if (!curSubpath) {
      curSubpath = [];
      outPolygons.push(curSubpath);
    }
    curSubpath.push([tx, ty]);
  }
  for (let i = 0; i < ops.length; i++) {
    const item = ops[i];
    if (!item) continue;
    const op = item.op;
    const args = item.args;
    if (op === "moveTo") {
      const x = args ? Number(args[0]) : Number(item.x);
      const y = args ? Number(args[1]) : Number(item.y);
      curSubpath = [];
      outPolygons.push(curSubpath);
      pushPt(x, y);
      cx = x;
      cy = y;
    } else if (op === "lineTo") {
      const x = args ? Number(args[0]) : Number(item.x);
      const y = args ? Number(args[1]) : Number(item.y);
      pushPt(x, y);
      cx = x;
      cy = y;
    } else if (op === "closePath") {
      if (curSubpath && curSubpath.length > 0) {
        const first = curSubpath[0];
        const last = curSubpath[curSubpath.length - 1];
        if (last[0] !== first[0] || last[1] !== first[1]) {
          curSubpath.push([first[0], first[1]]);
        }
      }
      curSubpath = null;
    } else if (op === "rect") {
      const x = args ? Number(args[0]) : Number(item.x);
      const y = args ? Number(args[1]) : Number(item.y);
      const w = args ? Number(args[2]) : Number(item.w);
      const h = args ? Number(args[3]) : Number(item.h);
      const p0 = transformPoint(matrix, x, y);
      const p1 = transformPoint(matrix, x + w, y);
      const p2 = transformPoint(matrix, x + w, y + h);
      const p3 = transformPoint(matrix, x, y + h);
      outPolygons.push([p0, p1, p2, p3, p0]);
      curSubpath = null;
      cx = x;
      cy = y;
    } else if (op === "quadraticCurveTo" && args) {
      const cpx = Number(args[0]), cpy = Number(args[1]), x = Number(args[2]), y = Number(args[3]);
      const steps = 12;
      for (let s = 1; s <= steps; s++) {
        const t = s / steps;
        const mt = 1 - t;
        const px = mt * mt * cx + 2 * mt * t * cpx + t * t * x;
        const py = mt * mt * cy + 2 * mt * t * cpy + t * t * y;
        pushPt(px, py);
      }
      cx = x;
      cy = y;
    } else if (op === "bezierCurveTo" && args) {
      const cp1x = Number(args[0]), cp1y = Number(args[1]);
      const cp2x = Number(args[2]), cp2y = Number(args[3]);
      const x = Number(args[4]), y = Number(args[5]);
      const steps = 16;
      for (let s = 1; s <= steps; s++) {
        const t = s / steps;
        const mt = 1 - t;
        const px = mt * mt * mt * cx + 3 * mt * mt * t * cp1x + 3 * mt * t * t * cp2x + t * t * t * x;
        const py = mt * mt * mt * cy + 3 * mt * mt * t * cp1y + 3 * mt * t * t * cp2y + t * t * t * y;
        pushPt(px, py);
      }
      cx = x;
      cy = y;
    } else if (op === "arc") {
      const x = args ? Number(args[0]) : Number(item.x);
      const y = args ? Number(args[1]) : Number(item.y);
      const r = args ? Number(args[2]) : Number(item.r);
      const [tx, ty] = transformPoint(matrix, x, y);
      const scale = Math.hypot(matrix[0], matrix[1]) || 1;
      outArcs.push({ x: tx, y: ty, r: r * scale });
      curSubpath = null;
    } else if (op === "addPath" && item.pathOps) {
      const subMatrix = item.matrix ? _mulMatrix2D(matrix, item.matrix) : matrix;
      _flattenPathOps(item.pathOps, subMatrix, outPolygons, outArcs);
      curSubpath = null;
    }
  }
}

CanvasRenderingContext2D.prototype.fill = function (pathOrRule, rule) {
  checkCtx(this);
  recordCanvasDrawOp(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "fill", fillStyle: this._state.fillStyle });
  }

  const color = parseColor(this._state.fillStyle);
  if (!color) return;

  const isPath2D = pathOrRule instanceof Path2D;
  const ops = isPath2D ? pathOrRule._ops : this._currentPath;
  const fillRule = (isPath2D ? rule : pathOrRule) === "evenodd" ? "evenodd" : "nonzero";
  const matrix = isPath2D ? this._state.matrix : [1, 0, 0, 1, 0, 0];

  const polygons = [];
  const arcs = [];
  _flattenPathOps(ops, matrix, polygons, arcs);
  if (polygons.length === 0 && arcs.length === 0) return;

  const bitmap = ensureBitmap(this);
  const bw = this._bitmapWidth;
  const bh = this._bitmapHeight;

  const rByte = Math.min(255, Math.max(0, Math.round(color[0])));
  const gByte = Math.min(255, Math.max(0, Math.round(color[1])));
  const bByte = Math.min(255, Math.max(0, Math.round(color[2])));
  const alpha = color[3] * this._state.globalAlpha;
  const aByte = Math.min(255, Math.max(0, Math.round(alpha * 255)));

  const clip = this._state.clipRect;

  for (let i = 0; i < arcs.length; i++) {
    const item = arcs[i];
    const cx = item.x;
    const cy = item.y;
    const r = item.r;
    const r2 = r * r;

    let x0 = Math.max(0, Math.floor(cx - r));
    let y0 = Math.max(0, Math.floor(cy - r));
    let x1 = Math.min(bw, Math.ceil(cx + r));
    let y1 = Math.min(bh, Math.ceil(cy + r));

    if (clip) {
      x0 = Math.max(x0, Math.floor(clip.x0));
      y0 = Math.max(y0, Math.floor(clip.y0));
      x1 = Math.min(x1, Math.ceil(clip.x1));
      y1 = Math.min(y1, Math.ceil(clip.y1));
    }

    for (let py = y0; py < y1; py++) {
      const dy = py - cy;
      for (let px = x0; px < x1; px++) {
        const dx = px - cx;
        if (dx * dx + dy * dy <= r2) {
          const idx = (py * bw + px) * 4;
          bitmap[idx] = rByte;
          bitmap[idx + 1] = gByte;
          bitmap[idx + 2] = bByte;
          bitmap[idx + 3] = aByte;
        }
      }
    }
  }

  if (polygons.length > 0) {
    let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
    for (let p = 0; p < polygons.length; p++) {
      const poly = polygons[p];
      for (let v = 0; v < poly.length; v++) {
        const vx = poly[v][0], vy = poly[v][1];
        if (vx < minX) minX = vx;
        if (vy < minY) minY = vy;
        if (vx > maxX) maxX = vx;
        if (vy > maxY) maxY = vy;
      }
    }
    if (Number.isFinite(minX) && Number.isFinite(minY)) {
      let x0 = Math.max(0, Math.floor(minX));
      let y0 = Math.max(0, Math.floor(minY));
      let x1 = Math.min(bw, Math.ceil(maxX));
      let y1 = Math.min(bh, Math.ceil(maxY));
      if (clip) {
        x0 = Math.max(x0, Math.floor(clip.x0));
        y0 = Math.max(y0, Math.floor(clip.y0));
        x1 = Math.min(x1, Math.ceil(clip.x1));
        y1 = Math.min(y1, Math.ceil(clip.y1));
      }
      for (let py = y0; py < y1; py++) {
        const sy = py + 0.5;
        const hits = [];
        for (let p = 0; p < polygons.length; p++) {
          const poly = polygons[p];
          const n = poly.length;
          if (n < 2) continue;
          for (let v = 0; v < n; v++) {
            const p0 = poly[v];
            const p1 = poly[(v + 1) % n];
            const yA = p0[1], yB = p1[1];
            if ((yA <= sy && yB > sy) || (yB <= sy && yA > sy)) {
              const t = (sy - yA) / (yB - yA);
              const ix = p0[0] + t * (p1[0] - p0[0]);
              hits.push({ x: ix, dir: yB > yA ? 1 : -1 });
            }
          }
        }
        if (hits.length < 2) continue;
        hits.sort(function (a, b) { return a.x - b.x; });
        if (fillRule === "evenodd") {
          for (let h = 0; h + 1 < hits.length; h += 2) {
            const sx0 = Math.max(x0, Math.ceil(hits[h].x - 0.5));
            const sx1 = Math.min(x1, Math.ceil(hits[h + 1].x - 0.5));
            for (let px = sx0; px < sx1; px++) {
              const idx = (py * bw + px) * 4;
              bitmap[idx] = rByte;
              bitmap[idx + 1] = gByte;
              bitmap[idx + 2] = bByte;
              bitmap[idx + 3] = aByte;
            }
          }
        } else {
          let winding = 0;
          for (let h = 0; h + 1 < hits.length; h++) {
            winding += hits[h].dir;
            if (winding !== 0) {
              const sx0 = Math.max(x0, Math.ceil(hits[h].x - 0.5));
              const sx1 = Math.min(x1, Math.ceil(hits[h + 1].x - 0.5));
              for (let px = sx0; px < sx1; px++) {
                const idx = (py * bw + px) * 4;
                bitmap[idx] = rByte;
                bitmap[idx + 1] = gByte;
                bitmap[idx + 2] = bByte;
                bitmap[idx + 3] = aByte;
              }
            }
          }
        }
      }
    }
  }
};

CanvasRenderingContext2D.prototype.stroke = function (path) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "stroke", strokeStyle: this._state.strokeStyle, lineWidth: this._state.lineWidth });
  }

  const color = parseColor(this._state.strokeStyle);
  if (!color) return;

  const bitmap = ensureBitmap(this);
  const bw = this._bitmapWidth;
  const bh = this._bitmapHeight;

  const rByte = Math.min(255, Math.max(0, Math.round(color[0])));
  const gByte = Math.min(255, Math.max(0, Math.round(color[1])));
  const bByte = Math.min(255, Math.max(0, Math.round(color[2])));
  const aByte = Math.min(255, Math.max(0, Math.round(color[3] * this._state.globalAlpha * 255)));
  const lw = Math.max(1, this._state.lineWidth || 1);
  const halfLw = lw / 2;

  let curX = 0, curY = 0;
  for (let i = 0; i < this._currentPath.length; i++) {
    const item = this._currentPath[i];
    if (item.op === "moveTo") {
      curX = item.x;
      curY = item.y;
    } else if (item.op === "lineTo") {
      const nextX = item.x;
      const nextY = item.y;
      const xMin = Math.max(0, Math.floor(Math.min(curX, nextX) - halfLw));
      const xMax = Math.min(bw, Math.ceil(Math.max(curX, nextX) + halfLw));
      const yMin = Math.max(0, Math.floor(Math.min(curY, nextY) - halfLw));
      const yMax = Math.min(bh, Math.ceil(Math.max(curY, nextY) + halfLw));

      const dx = nextX - curX;
      const dy = nextY - curY;
      const len2 = dx * dx + dy * dy;

      for (let py = yMin; py < yMax; py++) {
        for (let px = xMin; px < xMax; px++) {
          let dist2;
          if (len2 === 0) {
            dist2 = (px - curX) * (px - curX) + (py - curY) * (py - curY);
          } else {
            let t = ((px - curX) * dx + (py - curY) * dy) / len2;
            t = Math.max(0, Math.min(1, t));
            const projX = curX + t * dx;
            const projY = curY + t * dy;
            dist2 = (px - projX) * (px - projX) + (py - projY) * (py - projY);
          }
          if (dist2 <= halfLw * halfLw) {
            const idx = (py * bw + px) * 4;
            bitmap[idx] = rByte;
            bitmap[idx + 1] = gByte;
            bitmap[idx + 2] = bByte;
            bitmap[idx + 3] = aByte;
          }
        }
      }
      curX = nextX;
      curY = nextY;
    }
  }
};

CanvasRenderingContext2D.prototype.clip = function (pathOrRule, rule) {
  checkCtx(this);
  for (let i = 0; i < this._currentPath.length; i++) {
    const item = this._currentPath[i];
    if (item.op === "rect") {
      let x0 = item.w < 0 ? item.x + item.w : item.x;
      let y0 = item.h < 0 ? item.y + item.h : item.y;
      let x1 = x0 + Math.abs(item.w);
      let y1 = y0 + Math.abs(item.h);
      if (this._state.clipRect) {
        x0 = Math.max(x0, this._state.clipRect.x0);
        y0 = Math.max(y0, this._state.clipRect.y0);
        x1 = Math.min(x1, this._state.clipRect.x1);
        y1 = Math.min(y1, this._state.clipRect.y1);
      }
      this._state.clipRect = { x0: x0, y0: y0, x1: x1, y1: y1 };
      break;
    }
  }
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "clip" });
  }
};

CanvasRenderingContext2D.prototype.isPointInPath = function (x, y) {
  checkCtx(this);
  const px = Number(x);
  const py = Number(y);
  for (let i = 0; i < this._currentPath.length; i++) {
    const item = this._currentPath[i];
    if (item.op === "rect") {
      const rx = item.w < 0 ? item.x + item.w : item.x;
      const ry = item.h < 0 ? item.y + item.h : item.y;
      const rw = Math.abs(item.w);
      const rh = Math.abs(item.h);
      if (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh) {
        return true;
      }
    } else if (item.op === "arc") {
      const dx = px - item.x;
      const dy = py - item.y;
      if (dx * dx + dy * dy <= item.r * item.r) {
        return true;
      }
    }
  }
  return false;
};
CanvasRenderingContext2D.prototype.isPointInStroke = function (x, y) { checkCtx(this); return false; };

// Text drawing
CanvasRenderingContext2D.prototype.fillText = function (text, x, y, maxWidth) {
  checkCtx(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "fillText", text: String(text), x: x, y: y, maxWidth: maxWidth, font: this._state.font, fillStyle: this._state.fillStyle });
  }

  const str = String(text != null ? text : "");
  if (!str) return;

  const color = parseColor(this._state.fillStyle);
  if (!color) return;

  const bitmap = ensureBitmap(this);
  const bw = this._bitmapWidth;
  const bh = this._bitmapHeight;

  const [startX, startY] = transformPoint(this._state.matrix, Number(x), Number(y));
  const fontMatch = (this._state.font || "").match(/(\d+)px/);
  const fontSize = fontMatch ? parseFloat(fontMatch[1]) : 10;
  const scaleY = Math.max(1, Math.round(fontSize / 8));
  const charAdvance = Math.max(6, Math.round(fontSize * 0.55));
  const scaleX = Math.max(1, Math.round(charAdvance / 8));

  const rByte = Math.min(255, Math.max(0, Math.round(color[0])));
  const gByte = Math.min(255, Math.max(0, Math.round(color[1])));
  const bByte = Math.min(255, Math.max(0, Math.round(color[2])));
  const aByte = Math.min(255, Math.max(0, Math.round(color[3] * this._state.globalAlpha * 255)));

  let curX = Math.round(startX);
  const baselineY = Math.round(startY);
  const topY = baselineY - Math.round(fontSize * 0.75);

  const clip = this._state.clipRect;

  for (let ci = 0; ci < str.length; ci++) {
    const ch = str.charAt(ci);
    const glyph = FONT_8X8[ch] || FONT_8X8['default'];
    for (let row = 0; row < 8; row++) {
      const byte = glyph[row];
      for (let col = 0; col < 8; col++) {
        if ((byte >> (7 - col)) & 1) {
          for (let sy = 0; sy < scaleY; sy++) {
            const py = topY + row * scaleY + sy;
            if (py < 0 || py >= bh) continue;
            if (clip && (py < clip.y0 || py >= clip.y1)) continue;
            for (let sx = 0; sx < scaleX; sx++) {
              const px = curX + col * scaleX + sx;
              if (px < 0 || px >= bw) continue;
              if (clip && (px < clip.x0 || px >= clip.x1)) continue;
              const idx = (py * bw + px) * 4;
              bitmap[idx] = rByte;
              bitmap[idx + 1] = gByte;
              bitmap[idx + 2] = bByte;
              bitmap[idx + 3] = aByte;
            }
          }
        }
      }
    }
    curX += charAdvance;
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
  recordCanvasDrawOp(this);
  if (globalThis.__CAP_RENDER_CONFIG && globalThis.__CAP_RENDER_CONFIG.canvasRecording) {
    this._ops.push({ op: "drawImage", args: Array.prototype.slice.call(arguments, 1) });
  }
  const realSource = resolveCanonicalCanvasSource(image);
  if (!realSource) return;
  if (!isCanvasSourceOriginClean(realSource)) {
    this._originClean = false;
    return;
  }

  let sx = 0, sy = 0, sw = 0, sh = 0;
  let dx = 0, dy = 0, dw = 0, dh = 0;
  const tag = String(realSource.localName || realSource.tagName || "").toLowerCase();

  let srcW = 0, srcH = 0;
  let srcBitmap = null;
  let solidRgba = null;

  if (tag === "canvas" || (typeof OffscreenCanvas !== "undefined" && realSource instanceof OffscreenCanvas)) {
    const srcCtx = realSource._ctx2d;
    if (srcCtx && srcCtx._bitmap && srcCtx._bitmapWidth > 0 && srcCtx._bitmapHeight > 0) {
      srcW = srcCtx._bitmapWidth;
      srcH = srcCtx._bitmapHeight;
      srcBitmap = srcCtx._bitmap;
    }
  } else if (tag === "img") {
    srcW = Number(realSource.naturalWidth || 0);
    srcH = Number(realSource.naturalHeight || 0);
    if (realSource._solidRgba) {
      solidRgba = realSource._solidRgba;
    } else if (typeof globalThis !== "undefined" && typeof globalThis.__capsurf_selectImageSource === "function" && typeof globalThis.__capsurf_getKnownFixtureImageInfo === "function") {
      const cand = globalThis.__capsurf_selectImageSource(realSource);
      const info = globalThis.__capsurf_getKnownFixtureImageInfo(cand);
      if (info && info.solidRgba) {
        solidRgba = info.solidRgba;
        if (!srcW) srcW = info.width;
        if (!srcH) srcH = info.height;
      }
    }
  }

  if (srcW <= 0 || srcH <= 0) return;

  if (arguments.length === 3) {
    dx = Number(arguments[1]) || 0;
    dy = Number(arguments[2]) || 0;
    dw = srcW;
    dh = srcH;
    sw = srcW;
    sh = srcH;
  } else if (arguments.length === 5) {
    dx = Number(arguments[1]) || 0;
    dy = Number(arguments[2]) || 0;
    dw = Number(arguments[3]) || 0;
    dh = Number(arguments[4]) || 0;
    sw = srcW;
    sh = srcH;
  } else if (arguments.length >= 9) {
    sx = Number(arguments[1]) || 0;
    sy = Number(arguments[2]) || 0;
    sw = Number(arguments[3]) || 0;
    sh = Number(arguments[4]) || 0;
    dx = Number(arguments[5]) || 0;
    dy = Number(arguments[6]) || 0;
    dw = Number(arguments[7]) || 0;
    dh = Number(arguments[8]) || 0;
  } else {
    return;
  }
  if (dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0) return;

  const bitmap = ensureBitmap(this);
  const bw = this._bitmapWidth;
  const bh = this._bitmapHeight;
  const x0 = Math.max(0, Math.floor(dx));
  const y0 = Math.max(0, Math.floor(dy));
  const x1 = Math.min(bw, Math.ceil(dx + dw));
  const y1 = Math.min(bh, Math.ceil(dy + dh));

  if (solidRgba) {
    for (let py = y0; py < y1; py++) {
      let idx = (py * bw + x0) * 4;
      for (let px = x0; px < x1; px++) {
        bitmap[idx] = solidRgba[0];
        bitmap[idx + 1] = solidRgba[1];
        bitmap[idx + 2] = solidRgba[2];
        bitmap[idx + 3] = solidRgba[3];
        idx += 4;
      }
    }
  } else if (srcBitmap) {
    for (let py = y0; py < y1; py++) {
      const sampleY = Math.min(srcH - 1, Math.max(0, Math.floor(sy + ((py - dy) * sh) / dh)));
      for (let px = x0; px < x1; px++) {
        const sampleX = Math.min(srcW - 1, Math.max(0, Math.floor(sx + ((px - dx) * sw) / dw)));
        const sIdx = (sampleY * srcW + sampleX) * 4;
        const dIdx = (py * bw + px) * 4;
        bitmap[dIdx] = srcBitmap[sIdx];
        bitmap[dIdx + 1] = srcBitmap[sIdx + 1];
        bitmap[dIdx + 2] = srcBitmap[sIdx + 2];
        bitmap[dIdx + 3] = srcBitmap[sIdx + 3];
      }
    }
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
  const realSource = resolveCanonicalCanvasSource(image);
  return new CanvasPattern(realSource || image, repetition);
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
  if (this._originClean === false) {
    throw makeSecurityError("Failed to execute 'getImageData' on 'CanvasRenderingContext2D': The canvas has been tainted by cross-origin data.");
  }
  flushCanvasOnReadback(this);
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

Object.defineProperty(CanvasRenderingContext2D.prototype, Symbol.toStringTag, {
  value: "CanvasRenderingContext2D",
  configurable: true,
});
markNative(CanvasRenderingContext2D, "CanvasRenderingContext2D");
markNative(CanvasRenderingContext2D.prototype.save, "save");
markNative(CanvasRenderingContext2D.prototype.restore, "restore");
markNative(CanvasRenderingContext2D.prototype.fillRect, "fillRect");
markNative(CanvasRenderingContext2D.prototype.strokeRect, "strokeRect");
markNative(CanvasRenderingContext2D.prototype.clearRect, "clearRect");
markNative(CanvasRenderingContext2D.prototype.measureText, "measureText");
markNative(CanvasRenderingContext2D.prototype.getImageData, "getImageData");

// -------------------------------------------------------------
// OffscreenCanvas Implementation
// -------------------------------------------------------------
function OffscreenCanvas(width, height) {
  if (new.target === undefined) {
    throw new TypeError("Failed to construct 'OffscreenCanvas': Please use the 'new' operator");
  }
  if (arguments.length < 2) {
    throw new TypeError("Failed to construct 'OffscreenCanvas': 2 arguments required, but only " + arguments.length + " present.");
  }
  let canvas = null;
  if (typeof document !== "undefined" && typeof document.createElement === "function") {
    canvas = document.createElement("canvas");
  }
  const w = (Number(width) >>> 0) || 0;
  const h = (Number(height) >>> 0) || 0;
  if (canvas) {
    canvas.width = w;
    canvas.height = h;
  }
  this._width = w;
  this._height = h;
  this.__backingCanvas = canvas;
  this._ctx2d = null;
}

OffscreenCanvas.prototype.getContext = function (contextType, attributes) {
  const t = String(contextType).toLowerCase();
  if (t === "webgl" || t === "webgl2" || t === "experimental-webgl") {
    return null;
  }
  if (t === "2d") {
    if (this.__backingCanvas && typeof this.__backingCanvas.getContext === "function") {
      return this.__backingCanvas.getContext(contextType, attributes);
    }
    if (!this._ctx2d) {
      this._ctx2d = new CanvasRenderingContext2D(this, _INTERNAL_CTX_SECRET);
    }
    return this._ctx2d;
  }
  return null;
};

Object.defineProperty(OffscreenCanvas.prototype, "width", {
  configurable: true,
  enumerable: true,
  get: function () {
    return this.__backingCanvas ? this.__backingCanvas.width : (this._width || 0);
  },
  set: function (v) {
    const n = (Number(v) >>> 0) || 0;
    this._width = n;
    if (this.__backingCanvas) this.__backingCanvas.width = n;
  },
});

Object.defineProperty(OffscreenCanvas.prototype, "height", {
  configurable: true,
  enumerable: true,
  get: function () {
    return this.__backingCanvas ? this.__backingCanvas.height : (this._height || 0);
  },
  set: function (v) {
    const n = (Number(v) >>> 0) || 0;
    this._height = n;
    if (this.__backingCanvas) this.__backingCanvas.height = n;
  },
});

OffscreenCanvas.prototype.convertToBlob = function (options) {
  const type = (options && options.type) || "image/png";
  if (typeof Blob !== "undefined") {
    return Promise.resolve(new Blob([], { type: type }));
  }
  return Promise.resolve({ type: type, size: 0 });
};

OffscreenCanvas.prototype.transferToImageBitmap = function () {
  return {};
};

Object.defineProperty(OffscreenCanvas.prototype, Symbol.toStringTag, {
  value: "OffscreenCanvas",
  configurable: true,
});

markNative(OffscreenCanvas, "OffscreenCanvas");
markNative(OffscreenCanvas.prototype.getContext, "getContext");
markNative(OffscreenCanvas.prototype.convertToBlob, "convertToBlob");
markNative(OffscreenCanvas.prototype.transferToImageBitmap, "transferToImageBitmap");

if (typeof globalThis !== "undefined") {
  globalThis.__capsurf_encode_png_base64 = encodePngBase64;
  globalThis.__capsurf_get_canvas_bitmaps = function () {
    const canvases = (typeof document !== "undefined" && document.querySelectorAll) ? document.querySelectorAll("canvas") : [];
    const parts = [];
    for (let i = 0; i < canvases.length; i++) {
      const c = canvases[i];
      const nid = (typeof c._nid === "number") ? c._nid : 0;
      const ctx = c._ctx2d;
      if (nid > 0 && ctx && ctx._bitmap && ctx._bitmapWidth > 0 && ctx._bitmapHeight > 0) {
        const w = ctx._bitmapWidth;
        const h = ctx._bitmapHeight;
        const bm = ctx._bitmap;
        const len = w * h * 4;
        let binStr = "";
        for (let j = 0; j < len; j++) {
          binStr += String.fromCharCode(bm[j]);
        }
        const b64 = (typeof btoa === "function") ? btoa(binStr) : base64Encode(bm.subarray(0, len));
        parts.push(nid + ":" + w + ":" + h + ":" + b64);
      }
    }
    return parts.join(";");
  };
}
