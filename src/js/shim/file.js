// src/js/shim/file.js
// Aligned with WHATWG File API specification
// Provides Blob and File implementations, UTF-8 conversion, MIME normalization,
// and Content-Type preservation for Request/Response .blob()

function normalizeBlobType(type) {
  if (type === undefined || type === null) return "";
  const str = String(type);
  for (let i = 0; i < str.length; i++) {
    const code = str.charCodeAt(i);
    if (code < 0x20 || code > 0x7e) {
      return "";
    }
  }
  return str.toLowerCase();
}

function stringToUtf8(str) {
  const bytes = [];
  for (let i = 0; i < str.length; i++) {
    let codePoint = str.charCodeAt(i);
    if (codePoint >= 0xd800 && codePoint <= 0xdbff && i + 1 < str.length) {
      const next = str.charCodeAt(i + 1);
      if (next >= 0xdc00 && next <= 0xdfff) {
        codePoint = ((codePoint - 0xd800) << 10) + (next - 0xdc00) + 0x10000;
        i++;
      }
    }
    if (codePoint < 0x80) {
      bytes.push(codePoint);
    } else if (codePoint < 0x800) {
      bytes.push(0xc0 | (codePoint >> 6));
      bytes.push(0x80 | (codePoint & 0x3f));
    } else if (codePoint < 0x10000) {
      bytes.push(0xe0 | (codePoint >> 12));
      bytes.push(0x80 | ((codePoint >> 6) & 0x3f));
      bytes.push(0x80 | (codePoint & 0x3f));
    } else {
      bytes.push(0xf0 | (codePoint >> 18));
      bytes.push(0x80 | ((codePoint >> 12) & 0x3f));
      bytes.push(0x80 | ((codePoint >> 6) & 0x3f));
      bytes.push(0x80 | (codePoint & 0x3f));
    }
  }
  return new Uint8Array(bytes);
}

function utf8ToString(bytes) {
  let str = "";
  let i = 0;
  while (i < bytes.length) {
    const b1 = bytes[i++];
    if (b1 < 0x80) {
      str += String.fromCharCode(b1);
    } else if ((b1 >> 5) === 0x06) {
      const b2 = bytes[i++];
      str += String.fromCharCode(((b1 & 0x1f) << 6) | (b2 & 0x3f));
    } else if ((b1 >> 4) === 0x0e) {
      const b2 = bytes[i++];
      const b3 = bytes[i++];
      str += String.fromCharCode(((b1 & 0x0f) << 12) | ((b2 & 0x3f) << 6) | (b3 & 0x3f));
    } else if ((b1 >> 3) === 0x1e) {
      const b2 = bytes[i++];
      const b3 = bytes[i++];
      const b4 = bytes[i++];
      let cp = ((b1 & 0x07) << 18) | ((b2 & 0x3f) << 12) | ((b3 & 0x3f) << 6) | (b4 & 0x3f);
      cp -= 0x10000;
      str += String.fromCharCode((cp >> 10) + 0xd800, (cp & 0x3ff) + 0xdc00);
    } else {
      str += String.fromCharCode(b1);
    }
  }
  return str;
}

function processBlobParts(blobParts) {
  if (blobParts === undefined || blobParts === null) {
    return new Uint8Array(0);
  }
  let list = [];
  if (Array.isArray(blobParts)) {
    list = blobParts;
  } else if (typeof blobParts[Symbol.iterator] === "function") {
    for (const item of blobParts) {
      list.push(item);
    }
  } else if (typeof blobParts.length === "number") {
    for (let i = 0; i < blobParts.length; i++) {
      list.push(blobParts[i]);
    }
  } else {
    throw new TypeError("Failed to construct 'Blob': The provided value cannot be converted to a sequence.");
  }

  const chunks = [];
  let totalLength = 0;

  for (let i = 0; i < list.length; i++) {
    const part = list[i];
    let u8;
    if (typeof part === "string") {
      u8 = stringToUtf8(part);
    } else if (part instanceof ArrayBuffer) {
      u8 = new Uint8Array(part.slice(0));
    } else if (ArrayBuffer.isView(part)) {
      u8 = new Uint8Array(part.buffer.slice(part.byteOffset, part.byteOffset + part.byteLength));
    } else if (part && part._bytes instanceof Uint8Array) {
      u8 = part._bytes;
    } else {
      u8 = stringToUtf8(String(part));
    }
    chunks.push(u8);
    totalLength += u8.length;
  }

  const result = new Uint8Array(totalLength);
  let offset = 0;
  for (let i = 0; i < chunks.length; i++) {
    result.set(chunks[i], offset);
    offset += chunks[i].length;
  }
  return result;
}

function Blob(blobParts, options) {
  if (!(this instanceof Blob)) {
    throw new TypeError("Failed to construct 'Blob': Please use the 'new' operator.");
  }
  if (options !== undefined && options !== null && typeof options !== "object" && typeof options !== "function") {
    throw new TypeError("Failed to construct 'Blob': parameter 2 is not an object.");
  }
  options = options || {};

  if (options._rawType != null) {
    this._type = String(options._rawType);
  } else {
    this._type = normalizeBlobType(options.type);
  }

  this._bytes = processBlobParts(blobParts);
}

Object.defineProperty(Blob.prototype, "size", {
  get: function () { return this._bytes.length; },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(Blob.prototype, "type", {
  get: function () { return this._type; },
  configurable: true,
  enumerable: true,
});

Blob.prototype.slice = function (start, end, contentType) {
  const size = this._bytes.length;
  let relativeStart = start !== undefined ? Number(start) : 0;
  let relativeEnd = end !== undefined ? Number(end) : size;

  if (Number.isNaN(relativeStart)) relativeStart = 0;
  if (Number.isNaN(relativeEnd)) relativeEnd = 0;

  let spanStart;
  if (relativeStart < 0) {
    spanStart = Math.max(size + relativeStart, 0);
  } else {
    spanStart = Math.min(relativeStart, size);
  }

  let spanEnd;
  if (relativeEnd < 0) {
    spanEnd = Math.max(size + relativeEnd, 0);
  } else {
    spanEnd = Math.min(relativeEnd, size);
  }

  const span = Math.max(spanEnd - spanStart, 0);
  const slicedBytes = this._bytes.subarray(spanStart, spanStart + span);

  const newBlob = new Blob([], {
    type: contentType !== undefined ? String(contentType) : "",
  });
  newBlob._bytes = new Uint8Array(slicedBytes);
  return newBlob;
};

Blob.prototype.text = function () {
  try {
    return Promise.resolve(utf8ToString(this._bytes));
  } catch (e) {
    return Promise.reject(e);
  }
};

Blob.prototype.arrayBuffer = function () {
  try {
    const buffer = this._bytes.buffer.slice(
      this._bytes.byteOffset,
      this._bytes.byteOffset + this._bytes.byteLength
    );
    return Promise.resolve(buffer);
  } catch (e) {
    return Promise.reject(e);
  }
};

Blob.prototype.bytes = function () {
  try {
    return Promise.resolve(new Uint8Array(this._bytes));
  } catch (e) {
    return Promise.reject(e);
  }
};

Blob.prototype.stream = function () {
  const bytes = this._bytes;
  if (typeof ReadableStream !== "undefined") {
    return new ReadableStream({
      start(controller) {
        controller.enqueue(bytes);
        controller.close();
      }
    });
  }
  throw new Error("ReadableStream not implemented");
};

Object.defineProperty(Blob.prototype, Symbol.toStringTag, {
  value: "Blob",
  configurable: true,
});

markNative(Blob, "Blob");
markNative(Blob.prototype.slice, "slice");
markNative(Blob.prototype.text, "text");
markNative(Blob.prototype.arrayBuffer, "arrayBuffer");
markNative(Blob.prototype.bytes, "bytes");
markNative(Blob.prototype.stream, "stream");

function File(fileBits, fileName, options) {
  if (!(this instanceof File)) {
    throw new TypeError("Failed to construct 'File': Please use the 'new' operator.");
  }
  if (arguments.length < 2) {
    throw new TypeError("Failed to construct 'File': 2 arguments required, but only " + arguments.length + " present.");
  }
  if (options !== undefined && options !== null && typeof options !== "object" && typeof options !== "function") {
    throw new TypeError("Failed to construct 'File': parameter 3 is not an object.");
  }
  options = options || {};
  Blob.call(this, fileBits, options);

  this._name = String(fileName);
  this._lastModified = options.lastModified != null ? Number(options.lastModified) : Date.now();
}

File.prototype = Object.create(Blob.prototype);
File.prototype.constructor = File;

Object.defineProperty(File.prototype, "name", {
  get: function () { return this._name; },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(File.prototype, "lastModified", {
  get: function () { return this._lastModified; },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(File.prototype, "webkitRelativePath", {
  get: function () { return ""; },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(File.prototype, Symbol.toStringTag, {
  value: "File",
  configurable: true,
});

markNative(File, "File");

// WHATWG MIME serialization
function parseAndSerializeMimeType(contentType) {
  if (!contentType) return "";
  const parts = String(contentType).split(";");
  const mime = parts[0].trim().toLowerCase();
  if (!mime || mime.indexOf("/") === -1) return "";
  const params = [];
  for (let i = 1; i < parts.length; i++) {
    const part = parts[i].trim();
    if (!part) continue;
    const eq = part.indexOf("=");
    if (eq !== -1) {
      const key = part.slice(0, eq).trim().toLowerCase();
      const val = part.slice(eq + 1).trim();
      params.push(key + "=" + val);
    } else {
      params.push(part.toLowerCase());
    }
  }
  return params.length > 0 ? mime + ";" + params.join(";") : mime;
}

function extractMimeFromHeaders(headers) {
  if (!headers) return "";
  let ct = null;
  if (typeof headers.get === "function") {
    ct = headers.get("content-type");
  } else if (typeof headers === "object") {
    for (const k of Object.keys(headers)) {
      if (k.toLowerCase() === "content-type") {
        ct = headers[k];
        break;
      }
    }
  }
  return ct ? parseAndSerializeMimeType(ct) : "";
}

// Request & Response support for .blob() Content-Type passthrough
function Headers(init) {
  this._map = new Map();
  this._guard = "none";
  if (init === undefined) {
    return;
  }
  if (typeof init !== "object" || init === null) {
    throw new TypeError(
      "Failed to construct 'Headers': The provided value is not of type '(sequence<sequence<ByteString>> or record<ByteString, ByteString>)'"
    );
  }

  if (init instanceof Headers) {
    init._map.forEach(function (v, k) {
      this._map.set(k, v);
    }, this);
    return;
  }

  // If init is iterable and not a string: follow Web IDL single-read sequence conversion
  if (typeof init[Symbol.iterator] === "function") {
    const it = init[Symbol.iterator]();
    if (!it || typeof it !== "object") {
      throw new TypeError("Failed to construct 'Headers': The provided value is not iterable");
    }
    const nextMethod = it.next;
    if (typeof nextMethod !== "function") {
      throw new TypeError("Failed to construct 'Headers': The provided value is not iterable");
    }
    while (true) {
      const step = nextMethod.call(it);
      if (step.done) break;
      const item = step.value;
      let pair = null;
      if (Array.isArray(item)) {
        pair = item;
      } else if (item && typeof item === "object" && typeof item[Symbol.iterator] === "function") {
        const subIt = item[Symbol.iterator]();
        if (!subIt || typeof subIt !== "object") {
          throw new TypeError("Failed to construct 'Headers': Value in sequence is not iterable");
        }
        const subNext = subIt.next;
        if (typeof subNext !== "function") {
          throw new TypeError("Failed to construct 'Headers': Value in sequence is not iterable");
        }
        const items = [];
        while (true) {
          const subStep = subNext.call(subIt);
          if (subStep.done) break;
          items.push(subStep.value);
          if (items.length > 2) break;
        }
        pair = items;
      } else {
        throw new TypeError("Failed to construct 'Headers': Value in sequence is not a sequence");
      }

      if (pair.length !== 2) {
        throw new TypeError("Failed to construct 'Headers': Each header entry must be a 2-element sequence");
      }
      this.append(pair[0], pair[1]);
    }
    return;
  }

  // Plain record / dictionary: iterate own enumerable properties
  const keys = Object.keys(init);
  for (let i = 0; i < keys.length; i++) {
    const k = keys[i];
    this.set(k, init[k]);
  }
}

Headers.prototype.append = function (k, v) {
  if (this._guard === "immutable") {
    throw new TypeError("Failed to execute 'append' on 'Headers': Headers are immutable");
  }
  const key = String(k).toLowerCase();
  const val = String(v);
  const existing = this._map.get(key);
  this._map.set(key, existing ? existing + ", " + val : val);
};

Headers.prototype.set = function (k, v) {
  if (this._guard === "immutable") {
    throw new TypeError("Failed to execute 'set' on 'Headers': Headers are immutable");
  }
  this._map.set(String(k).toLowerCase(), String(v));
};

Headers.prototype.get = function (k) {
  const key = String(k).toLowerCase();
  return this._map.has(key) ? this._map.get(key) : null;
};

Headers.prototype.has = function (k) {
  return this._map.has(String(k).toLowerCase());
};

Headers.prototype.delete = function (k) {
  if (this._guard === "immutable") {
    throw new TypeError("Failed to execute 'delete' on 'Headers': Headers are immutable");
  }
  this._map.delete(String(k).toLowerCase());
};

Headers.prototype.forEach = function (cb, thisArg) {
  this._map.forEach(cb, thisArg);
};

Headers.prototype.entries = function () {
  return this._map.entries();
};

Headers.prototype.keys = function () {
  return this._map.keys();
};

Headers.prototype.values = function () {
  return this._map.values();
};

Headers.prototype[Symbol.iterator] = function () {
  return this._map.entries();
};

markNative(Headers, "Headers");
markNative(Headers.prototype.append, "append");
markNative(Headers.prototype.set, "set");
markNative(Headers.prototype.get, "get");
markNative(Headers.prototype.has, "has");
markNative(Headers.prototype.delete, "delete");
markNative(Headers.prototype.forEach, "forEach");
markNative(Headers.prototype.entries, "entries");
markNative(Headers.prototype.keys, "keys");
markNative(Headers.prototype.values, "values");

const VALID_REFERRER_POLICIES = new Set([
  "",
  "no-referrer",
  "no-referrer-when-downgrade",
  "same-origin",
  "origin",
  "strict-origin",
  "origin-when-cross-origin",
  "strict-origin-when-cross-origin",
  "unsafe-url",
]);

function resolveRequestReferrer(rawRef) {
  const s = String(rawRef);
  if (s === "" || s === "no-referrer") return "";
  if (s === "client" || s === "about:client") return "about:client";
  try {
    const loc = (typeof location !== "undefined" && location) || (typeof globalThis !== "undefined" && globalThis.location);
    const base = (loc && loc.href && loc.href !== "about:blank") ? loc.href : "http://localhost/";
    const parsed = new URL(s, base);
    const curOrigin = (loc && loc.origin && loc.origin !== "null") ? loc.origin : "";
    if (curOrigin && parsed.origin !== curOrigin) {
      return "about:client";
    }
    return parsed.href;
  } catch (e) {
    throw new TypeError("Failed to construct 'Request': Invalid referrer URL '" + s + "'");
  }
}

function Request(input, init) {
  const isReq = input && typeof input === "object" && input instanceof Request;
  init = init || {};
  const rawUrl = isReq ? input.url : String(input);
  let resolvedUrl = rawUrl;
  try {
    const loc = (typeof location !== "undefined" && location) || (typeof globalThis !== "undefined" && globalThis.location);
    const base = (loc && loc.href && loc.href !== "about:blank") ? loc.href : "http://localhost/";
    resolvedUrl = new URL(rawUrl, base).href;
  } catch (e) {
    resolvedUrl = rawUrl;
  }
  this._url = resolvedUrl;
  this._method = (init.method || (isReq ? input.method : "GET")).toUpperCase();
  this._headers = new Headers(init.headers !== undefined ? init.headers : (isReq ? input.headers : undefined));
  this._rawMimeType = extractMimeFromHeaders(this._headers);
  this._body = init.body !== undefined ? init.body : (isReq ? input._body : "");
  this._mode = init.mode !== undefined ? String(init.mode) : (isReq ? input.mode : "cors");
  this._credentials = init.credentials !== undefined ? String(init.credentials) : (isReq ? input.credentials : "same-origin");
  this._cache = init.cache !== undefined ? String(init.cache) : (isReq ? input.cache : "default");

  const rp = init.referrerPolicy !== undefined ? String(init.referrerPolicy) : (isReq ? input.referrerPolicy : "");
  if (!VALID_REFERRER_POLICIES.has(rp)) {
    throw new TypeError("Failed to construct 'Request': The provided value '" + rp + "' is not a valid enum value of type ReferrerPolicy.");
  }
  this._referrerPolicy = rp;

  if (init.referrer !== undefined) {
    this._referrer = resolveRequestReferrer(init.referrer);
  } else if (isReq) {
    this._referrer = input.referrer;
  } else {
    this._referrer = "about:client";
  }
}

Object.defineProperty(Request.prototype, "url", {
  get: function () { return this._url; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Request.prototype, "method", {
  get: function () { return this._method; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Request.prototype, "headers", {
  get: function () { return this._headers; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Request.prototype, "mode", {
  get: function () { return this._mode; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Request.prototype, "credentials", {
  get: function () { return this._credentials; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Request.prototype, "cache", {
  get: function () { return this._cache; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Request.prototype, "referrerPolicy", {
  get: function () { return this._referrerPolicy; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Request.prototype, "referrer", {
  get: function () { return this._referrer; },
  configurable: true,
  enumerable: true,
});

Request.prototype.clone = function () {
  return new Request(this);
};

Request.prototype.blob = function () {
  const mime = this._rawMimeType || "";
  const parts = this._body != null ? [this._body] : [];
  const blob = new Blob(parts, { _rawType: mime, type: mime });
  const promise = Promise.resolve(blob);
  promise._blob = blob;
  Object.defineProperty(promise, "type", {
    get: function () { return blob.type; },
    configurable: true,
  });
  return promise;
};

Request.prototype.text = function () {
  return Promise.resolve(String(this._body));
};

Request.prototype.json = function () {
  return Promise.resolve(JSON.parse(String(this._body)));
};

Request.prototype.arrayBuffer = function () {
  return Promise.resolve(stringToUtf8(String(this._body)).buffer);
};

markNative(Request, "Request");
markNative(Request.prototype.clone, "clone");
markNative(Request.prototype.blob, "blob");
markNative(Request.prototype.text, "text");
markNative(Request.prototype.json, "json");
markNative(Request.prototype.arrayBuffer, "arrayBuffer");

function Response(body, init) {
  init = init || {};
  this._status = init.status !== undefined ? Number(init.status) : 200;
  this._statusText = init.statusText !== undefined ? String(init.statusText) : "OK";
  this._ok = this._status >= 200 && this._status < 300;
  this._type = init.type !== undefined ? String(init.type) : "default";
  this._url = init.url !== undefined ? String(init.url) : "";
  this._redirected = Boolean(init.redirected);
  this._headers = new Headers(init.headers);
  if (init._immutableHeaders) {
    this._headers._guard = "immutable";
  }
  this._rawMimeType = extractMimeFromHeaders(this._headers);
  this._body = body !== undefined ? body : "";
}

Object.defineProperty(Response.prototype, "status", {
  get: function () { return this._status; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Response.prototype, "statusText", {
  get: function () { return this._statusText; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Response.prototype, "ok", {
  get: function () { return this._ok; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Response.prototype, "type", {
  get: function () { return this._type; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Response.prototype, "url", {
  get: function () { return this._url; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Response.prototype, "redirected", {
  get: function () { return this._redirected; },
  configurable: true,
  enumerable: true,
});
Object.defineProperty(Response.prototype, "headers", {
  get: function () { return this._headers; },
  configurable: true,
  enumerable: true,
});

Response.prototype.blob = function () {
  const mime = this._rawMimeType || "";
  const parts = this._body != null ? [this._body] : [];
  const blob = new Blob(parts, { _rawType: mime, type: mime });
  const promise = Promise.resolve(blob);
  promise._blob = blob;
  Object.defineProperty(promise, "type", {
    get: function () { return blob.type; },
    configurable: true,
  });
  return promise;
};

Response.prototype.text = function () {
  return Promise.resolve(String(this._body));
};

Response.prototype.json = function () {
  return Promise.resolve(JSON.parse(String(this._body)));
};

Response.prototype.arrayBuffer = function () {
  if (this._body instanceof ArrayBuffer) {
    return Promise.resolve(this._body.slice(0));
  }
  if (ArrayBuffer.isView(this._body)) {
    const view = new Uint8Array(this._body.buffer, this._body.byteOffset, this._body.byteLength);
    const copy = new Uint8Array(view.byteLength);
    copy.set(view);
    return Promise.resolve(copy.buffer);
  }
  return Promise.resolve(stringToUtf8(String(this._body)).buffer);
};

Response.prototype.formData = function () {
  if (typeof FormData !== "function") {
    return Promise.reject(new TypeError("FormData is not defined"));
  }
  const bodyStr = String(this._body != null ? this._body : "");
  const ct = (this._headers && typeof this._headers.get === "function") ? String(this._headers.get("content-type") || "") : "";
  const fd = new FormData();
  if (ct.toLowerCase().indexOf("application/x-www-form-urlencoded") !== -1) {
    const sp = new URLSearchParams(bodyStr);
    sp.forEach(function (v, k) { fd.append(k, v); });
    return Promise.resolve(fd);
  }
  let boundary = "";
  const bm = /boundary=(?:"([^"]+)"|([^;\s]+))/i.exec(ct);
  if (bm) {
    boundary = bm[1] || bm[2] || "";
  } else if (bodyStr.startsWith("--")) {
    const eol = bodyStr.indexOf("\n");
    if (eol > 2) boundary = bodyStr.slice(2, eol).replace(/\r$/, "");
  }
  if (!boundary) {
    return Promise.reject(new TypeError("Invalid MIME type for formData()"));
  }
  const delim = "--" + boundary;
  const parts = bodyStr.split(delim);
  for (let i = 1; i < parts.length; i++) {
    let part = parts[i];
    if (!part || part.startsWith("--")) continue;
    if (part.startsWith("\r\n")) part = part.slice(2);
    else if (part.startsWith("\n")) part = part.slice(1);
    let sepIdx = part.indexOf("\r\n\r\n");
    let sepLen = 4;
    if (sepIdx === -1) { sepIdx = part.indexOf("\n\n"); sepLen = 2; }
    if (sepIdx === -1) continue;
    const hdrRaw = part.slice(0, sepIdx);
    let val = part.slice(sepIdx + sepLen);
    if (val.endsWith("\r\n")) val = val.slice(0, -2);
    else if (val.endsWith("\n")) val = val.slice(0, -1);
    const nm = /name="([^"]*)"/i.exec(hdrRaw);
    if (!nm) continue;
    const fieldName = nm[1];
    const fnm = /filename="([^"]*)"/i.exec(hdrRaw);
    if (fnm && typeof File === "function") {
      const ctm = /content-type:\s*([^\r\n]+)/i.exec(hdrRaw);
      fd.append(fieldName, new File([val], fnm[1], { type: ctm ? ctm[1].trim() : "application/octet-stream" }));
    } else {
      fd.append(fieldName, val);
    }
  }
  return Promise.resolve(fd);
};

markNative(Response, "Response");
markNative(Response.prototype.blob, "blob");
markNative(Response.prototype.text, "text");
markNative(Response.prototype.json, "json");
markNative(Response.prototype.arrayBuffer, "arrayBuffer");
markNative(Response.prototype.formData, "formData");

// If Request or Response were already defined by environment, enhance their .blob()
function patchExistingBodyBlob(Ctor) {
  if (Ctor && Ctor.prototype && typeof Ctor.prototype.blob === "function") {
    const origBlob = Ctor.prototype.blob;
    Ctor.prototype.blob = function () {
      const self = this;
      return origBlob.call(self).then(function (b) {
        if (self._rawMimeType != null) {
          b._type = self._rawMimeType;
        } else {
          try {
            const ct = self.headers && self.headers.get ? self.headers.get("content-type") : null;
            if (ct) {
              const mime = parseAndSerializeMimeType(ct);
              if (mime) b._type = mime;
            }
          } catch (e) {}
        }
        return b;
      });
    };
  }
}

// ... (existing code, added at the end)

function TextDecoder(label, options) {
  let enc = (label || 'utf-8').trim().toLowerCase();
  if (enc === 'utf8') enc = 'utf-8';
  this.label = enc;
  this.encoding = enc;
  this.fatal = options ? !!options.fatal : false;
  this.ignoreBOM = options ? !!options.ignoreBOM : false;
}

TextDecoder.prototype.decode = function(input, options) {
  if (input === undefined || input === null) {
    return "";
  }
  let label = (this.label || 'utf-8').trim().toLowerCase();
  if (label === 'utf8') label = 'utf-8';
  
  let bytes;
  if (input instanceof ArrayBuffer) {
    bytes = new Uint8Array(input);
  } else if (ArrayBuffer.isView(input)) {
    bytes = new Uint8Array(input.buffer, input.byteOffset, input.byteLength);
  } else {
    throw new TypeError("The provided value is not of type '(ArrayBufferView or ArrayBuffer)'");
  }
  
  if (label === 'utf-8') {
    return utf8ToString(bytes);
  } else if (label === 'latin1' || label === 'iso-8859-1' || label === 'windows-1252') {
    let str = "";
    for (let i = 0; i < bytes.length; i++) {
      str += String.fromCharCode(bytes[i]);
    }
    return str;
  }
  
  throw new Error('Encoding not supported: ' + this.label);
};

markNative(TextDecoder, "TextDecoder");
markNative(TextDecoder.prototype.decode, "decode");

function TextEncoder() {
  this.encoding = "utf-8";
}
TextEncoder.prototype.encode = function (input) {
  return stringToUtf8(input === undefined ? "" : String(input));
};
markNative(TextEncoder, "TextEncoder");
markNative(TextEncoder.prototype.encode, "encode");

// -------------------------------------------------------------
// FileReader
// -------------------------------------------------------------
const _b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
function base64Encode(bytes) {
  let result = "";
  let i = 0;
  const len = bytes.length;
  for (; i + 2 < len; i += 3) {
    result += _b64chars[bytes[i] >> 2];
    result += _b64chars[((bytes[i] & 3) << 4) | (bytes[i + 1] >> 4)];
    result += _b64chars[((bytes[i + 1] & 15) << 2) | (bytes[i + 2] >> 6)];
    result += _b64chars[bytes[i + 2] & 63];
  }
  if (i < len) {
    result += _b64chars[bytes[i] >> 2];
    if (i + 1 < len) {
      result += _b64chars[((bytes[i] & 3) << 4) | (bytes[i + 1] >> 4)];
      result += _b64chars[(bytes[i + 1] & 15) << 2];
      result += "=";
    } else {
      result += _b64chars[(bytes[i] & 3) << 4];
      result += "==";
    }
  }
  return result;
}

function _safeQueueMicrotask(fn) {
  if (typeof queueMicrotask === "function") {
    queueMicrotask(fn);
  } else {
    Promise.resolve().then(fn);
  }
}

function FileReader() {
  if (!(this instanceof FileReader)) {
    throw new TypeError("Failed to construct 'FileReader': Please use the 'new' operator.");
  }
  EventTarget.call(this);
  this._gen = 0;
  this.readyState = 0;
  this.result = null;
  this.error = null;
  this.onloadstart = null;
  this.onprogress = null;
  this.onload = null;
  this.onabort = null;
  this.onerror = null;
  this.onloadend = null;
}

FileReader.EMPTY = 0;
FileReader.LOADING = 1;
FileReader.DONE = 2;

FileReader.prototype = Object.create(EventTarget.prototype);
FileReader.prototype.constructor = FileReader;

FileReader.prototype.EMPTY = 0;
FileReader.prototype.LOADING = 1;
FileReader.prototype.DONE = 2;

FileReader.prototype._fire = function (type, detail) {
  let event;
  try {
    event = new Event(type, { bubbles: false, cancelable: false });
    if (detail) {
      for (const k in detail) {
        if (Object.prototype.hasOwnProperty.call(detail, k)) {
          event[k] = detail[k];
        }
      }
    }
  } catch (e) {
    event = { type: type, bubbles: false, cancelable: false };
    if (detail) {
      for (const k in detail) {
        if (Object.prototype.hasOwnProperty.call(detail, k)) {
          event[k] = detail[k];
        }
      }
    }
  }
  const handler = this["on" + type];
  if (typeof handler === "function") {
    try {
      handler.call(this, event);
    } catch (e) {
      if (typeof console !== "undefined" && console.error) console.error(e);
    }
  }
  try {
    this.dispatchEvent(event);
  } catch (e) {}
};

FileReader.prototype._read = function (blob, readType, encoding) {
  const readGeneration = ++this._gen;
  const later = (callback) => {
    setTimeout(() => {
      setTimeout(() => {
        if (this._gen !== readGeneration) return;
        callback();
      }, 0);
    }, 0);
  };
  const fireLoadendLater = () => later(() => this._fire("loadend"));

  if (blob == null || typeof blob.arrayBuffer !== "function") {
    let error;
    const DOMEx = (typeof window !== "undefined" && window.DOMException) ||
                  (typeof globalThis !== "undefined" && globalThis.DOMException);
    if (typeof DOMEx === "function") {
      error = new DOMEx("Argument is not a Blob", "TypeError");
    } else {
      error = new TypeError("FileReader: argument is not a Blob");
    }
    this.error = error;
    this.readyState = 2; // DONE
    _safeQueueMicrotask(() => {
      if (this._gen !== readGeneration) return;
      this._fire("error");
      fireLoadendLater();
    });
    return;
  }

  this.readyState = 1; // LOADING
  this.result = null;
  this.error = null;

  _safeQueueMicrotask(() => {
    if (this._gen !== readGeneration) return;
    this._fire("loadstart");
  });

  Promise.resolve()
    .then(() => blob.arrayBuffer())
    .then((buffer) => {
      if (this._gen !== readGeneration) return;
      const bytes = new Uint8Array(buffer);
      let result;
      if (readType === "arrayBuffer") {
        result = buffer;
      } else if (readType === "text") {
        try {
          const decoder = new TextDecoder(encoding || "utf-8");
          result = decoder.decode(bytes);
        } catch (e) {
          result = "";
        }
      } else if (readType === "binaryString") {
        let binaryString = "";
        for (let i = 0; i < bytes.length; i++) {
          binaryString += String.fromCharCode(bytes[i]);
        }
        result = binaryString;
      } else {
        let binaryString = "";
        for (let i = 0; i < bytes.length; i++) {
          binaryString += String.fromCharCode(bytes[i]);
        }
        const base64 = (typeof btoa === "function") ? btoa(binaryString) : base64Encode(bytes);
        const mediaType = (blob.type && String(blob.type)) || "application/octet-stream";
        result = "data:" + mediaType + ";base64," + base64;
      }

      this._fire("progress", {
        lengthComputable: true,
        loaded: bytes.length,
        total: bytes.length,
      });

      later(() => {
        this.result = result;
        this.readyState = 2; // DONE
        this._fire("load");
        fireLoadendLater();
      });
    })
    .catch((error) => {
      if (this._gen !== readGeneration) return;
      this.error = error;
      this.readyState = 2; // DONE
      this._fire("error");
      fireLoadendLater();
    });
};

FileReader.prototype.readAsText = function (blob, encoding) {
  this._read(blob, "text", encoding);
};
FileReader.prototype.readAsArrayBuffer = function (blob) {
  this._read(blob, "arrayBuffer");
};
FileReader.prototype.readAsDataURL = function (blob) {
  this._read(blob, "dataURL");
};
FileReader.prototype.readAsBinaryString = function (blob) {
  this._read(blob, "binaryString");
};

FileReader.prototype.abort = function () {
  if (this.readyState === 0) {
    this.result = null;
    return;
  }
  if (this.readyState === 2) return;
  this._gen++;
  this.readyState = 2;
  this.result = null;
  this._fire("abort");
  this._fire("loadend");
};

markNative(FileReader, "FileReader");
markNative(FileReader.prototype.readAsText, "readAsText");
markNative(FileReader.prototype.readAsArrayBuffer, "readAsArrayBuffer");
markNative(FileReader.prototype.readAsDataURL, "readAsDataURL");
markNative(FileReader.prototype.readAsBinaryString, "readAsBinaryString");
markNative(FileReader.prototype.abort, "abort");

// -------------------------------------------------------------
// FileList, DataTransferItem, DataTransferItemList, DataTransfer
// -------------------------------------------------------------
function FileList() {
  throw new TypeError("Illegal constructor");
}

FileList.prototype.item = function (index) {
  const files = this._files || [];
  const i = Number(index) >>> 0;
  return i < files.length ? files[i] : null;
};

Object.defineProperty(FileList.prototype, "length", {
  get: function () {
    return this._files ? this._files.length : 0;
  },
  configurable: true,
  enumerable: true,
});

FileList.prototype[Symbol.iterator] = function () {
  const files = this._files ? this._files.slice() : [];
  return files[Symbol.iterator]();
};

Object.defineProperty(FileList.prototype, Symbol.toStringTag, {
  value: "FileList",
  configurable: true,
});

markNative(FileList, "FileList");
markNative(FileList.prototype.item, "item");

function createFileList(initialFiles) {
  const target = Object.create(FileList.prototype);
  target._files = Array.isArray(initialFiles) ? initialFiles.slice() : [];
  return new Proxy(target, {
    get: function (t, prop, receiver) {
      if (typeof prop === "string" && /^\d+$/.test(prop)) {
        const idx = Number(prop);
        return idx < t._files.length ? t._files[idx] : undefined;
      }
      if (prop === "length") return t._files.length;
      const val = Reflect.get(t, prop, t);
      return typeof val === "function" ? val.bind(t) : val;
    },
    has: function (t, prop) {
      if (typeof prop === "string" && /^\d+$/.test(prop)) {
        return Number(prop) < t._files.length;
      }
      return Reflect.has(t, prop);
    },
    ownKeys: function (t) {
      const keys = [];
      for (let i = 0; i < t._files.length; i++) keys.push(String(i));
      return keys;
    },
    getOwnPropertyDescriptor: function (t, prop) {
      if (typeof prop === "string" && /^\d+$/.test(prop)) {
        const idx = Number(prop);
        if (idx < t._files.length) {
          return {
            value: t._files[idx],
            writable: false,
            enumerable: true,
            configurable: true,
          };
        }
      }
      return Reflect.getOwnPropertyDescriptor(t, prop);
    },
  });
}

function normalizeDataTransferFormat(format) {
  const lower = String(format == null ? "" : format).toLowerCase();
  if (lower === "text") return "text/plain";
  if (lower === "url") return "text/uri-list";
  return lower;
}

const _dtiState = new WeakMap();
const _dtilState = new WeakMap();
const _dtState = new WeakMap();
const _dtInternalToken = {};

function DataTransferItem() {
  if (arguments[3] !== _dtInternalToken) {
    throw new TypeError("Illegal constructor");
  }
  _dtiState.set(this, { kind: arguments[0], type: arguments[1], payload: arguments[2] });
}

Object.defineProperty(DataTransferItem.prototype, "kind", {
  get: function () { const st = _dtiState.get(this); if (!st) throw new TypeError("Illegal invocation"); return st.kind; },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(DataTransferItem.prototype, "type", {
  get: function () { const st = _dtiState.get(this); if (!st) throw new TypeError("Illegal invocation"); return st.type; },
  configurable: true,
  enumerable: true,
});

DataTransferItem.prototype.getAsFile = function () {
  const st = _dtiState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  return st.kind === "file" ? st.payload : null;
};

DataTransferItem.prototype.getAsString = function (callback) {
  const st = _dtiState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  if (st.kind !== "string" || typeof callback !== "function") return;
  const data = String(st.payload || "");
  setTimeout(function () {
    callback(data);
  }, 0);
};

Object.defineProperty(DataTransferItem.prototype, Symbol.toStringTag, {
  value: "DataTransferItem",
  configurable: true,
});
markNative(DataTransferItem, "DataTransferItem");

function DataTransferItemList() {
  if (arguments[1] !== _dtInternalToken) {
    throw new TypeError("Illegal constructor");
  }
  _dtilState.set(this, arguments[0]);
}

Object.defineProperty(DataTransferItemList.prototype, "length", {
  get: function () { const dt = _dtilState.get(this); if (!dt) throw new TypeError("Illegal invocation"); return _dtState.get(dt).items.length; },
  configurable: true,
  enumerable: true,
});

DataTransferItemList.prototype.add = function (data) {
  const dt = _dtilState.get(this);
  if (!dt) throw new TypeError("Illegal invocation");
  const dtSt = _dtState.get(dt);
  if (arguments.length === 0) {
    throw new TypeError("Failed to execute 'add' on 'DataTransferItemList': 1 argument required, but only 0 present.");
  }
  if (arguments.length === 1) {
    if (!(data instanceof File)) {
      throw new TypeError("Failed to execute 'add' on 'DataTransferItemList': parameter 1 is not of type 'File'.");
    }
    const item = new DataTransferItem("file", data.type || "", data, _dtInternalToken);
    dtSt.items.push(item);
    return item;
  }
  const type = arguments[1];
  const norm = normalizeDataTransferFormat(type);
  const str = String(data);
  for (let i = 0; i < dtSt.items.length; i++) {
    if (dtSt.items[i].kind === "string" && dtSt.items[i].type === norm) {
      const DOMEx = typeof DOMException !== "undefined" ? DOMException : Error;
      throw new DOMEx("An item already exists for type '" + norm + "'.", "NotSupportedError");
    }
  }
  const item = new DataTransferItem("string", norm, str, _dtInternalToken);
  dtSt.items.push(item);
  dtSt.dataMap.set(norm, str);
  return item;
};

DataTransferItemList.prototype.remove = function (index) {
  const dt = _dtilState.get(this);
  if (!dt) throw new TypeError("Illegal invocation");
  const dtSt = _dtState.get(dt);
  const i = Number(index) >>> 0;
  if (i >= dtSt.items.length) return;
  const removed = dtSt.items.splice(i, 1)[0];
  if (removed && removed.kind === "string") {
    dtSt.dataMap.delete(removed.type);
  }
};

DataTransferItemList.prototype.clear = function () {
  const dt = _dtilState.get(this);
  if (!dt) throw new TypeError("Illegal invocation");
  const dtSt = _dtState.get(dt);
  dtSt.items.length = 0;
  dtSt.dataMap.clear();
};

DataTransferItemList.prototype[Symbol.iterator] = function () {
  const dt = _dtilState.get(this);
  if (!dt) throw new TypeError("Illegal invocation");
  return _dtState.get(dt).items.slice()[Symbol.iterator]();
};

Object.defineProperty(DataTransferItemList.prototype, Symbol.toStringTag, {
  value: "DataTransferItemList",
  configurable: true,
});
markNative(DataTransferItemList, "DataTransferItemList");

function createDataTransferItemList(dt) {
  const target = new DataTransferItemList(dt, _dtInternalToken);
  return new Proxy(target, {
    get: function (t, prop) {
      const dtObj = _dtilState.get(t);
      const dtSt = dtObj ? _dtState.get(dtObj) : null;
      if (dtSt && typeof prop === "string" && /^\d+$/.test(prop)) {
        const idx = Number(prop);
        return idx < dtSt.items.length ? dtSt.items[idx] : undefined;
      }
      if (dtSt && prop === "length") return dtSt.items.length;
      const val = Reflect.get(t, prop, t);
      return typeof val === "function" ? val.bind(t) : val;
    },
  });
}

function DataTransfer() {
  if (!(this instanceof DataTransfer)) {
    throw new TypeError("Failed to construct 'DataTransfer': Please use the 'new' operator.");
  }
  _dtState.set(this, {
    items: [],
    dataMap: new Map(),
    itemList: createDataTransferItemList(this),
    dropEffect: "none",
    effectAllowed: "uninitialized",
  });
}

Object.defineProperty(DataTransfer.prototype, "dropEffect", {
  get: function () { const st = _dtState.get(this); if (!st) throw new TypeError("Illegal invocation"); return st.dropEffect; },
  set: function (v) { const st = _dtState.get(this); if (!st) throw new TypeError("Illegal invocation"); st.dropEffect = String(v); },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(DataTransfer.prototype, "effectAllowed", {
  get: function () { const st = _dtState.get(this); if (!st) throw new TypeError("Illegal invocation"); return st.effectAllowed; },
  set: function (v) { const st = _dtState.get(this); if (!st) throw new TypeError("Illegal invocation"); st.effectAllowed = String(v); },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(DataTransfer.prototype, "items", {
  get: function () { const st = _dtState.get(this); if (!st) throw new TypeError("Illegal invocation"); return st.itemList; },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(DataTransfer.prototype, "files", {
  get: function () {
    const st = _dtState.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    const files = [];
    for (let i = 0; i < st.items.length; i++) {
      if (st.items[i].kind === "file") {
        files.push(st.items[i].getAsFile());
      }
    }
    return createFileList(files);
  },
  configurable: true,
  enumerable: true,
});

Object.defineProperty(DataTransfer.prototype, "types", {
  get: function () {
    const st = _dtState.get(this);
    if (!st) throw new TypeError("Illegal invocation");
    const out = [];
    let hasFiles = false;
    for (let i = 0; i < st.items.length; i++) {
      const it = st.items[i];
      if (it.kind === "file") {
        hasFiles = true;
      } else if (out.indexOf(it.type) === -1) {
        out.push(it.type);
      }
    }
    st.dataMap.forEach(function (_v, k) {
      if (out.indexOf(k) === -1) out.push(k);
    });
    if (hasFiles && out.indexOf("Files") === -1) {
      out.push("Files");
    }
    return out;
  },
  configurable: true,
  enumerable: true,
});

DataTransfer.prototype.setData = function (format, data) {
  const st = _dtState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  const norm = normalizeDataTransferFormat(format);
  const str = String(data == null ? "" : data);
  st.dataMap.set(norm, str);
  for (let i = 0; i < st.items.length; i++) {
    if (st.items[i].kind === "string" && st.items[i].type === norm) {
      const itemSt = _dtiState.get(st.items[i]);
      if (itemSt) itemSt.payload = str;
      return;
    }
  }
  st.items.push(new DataTransferItem("string", norm, str, _dtInternalToken));
};

DataTransfer.prototype.getData = function (format) {
  const st = _dtState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  const norm = normalizeDataTransferFormat(format);
  return st.dataMap.has(norm) ? st.dataMap.get(norm) : "";
};

DataTransfer.prototype.clearData = function (format) {
  const st = _dtState.get(this);
  if (!st) throw new TypeError("Illegal invocation");
  if (format === undefined) {
    st.dataMap.clear();
    st.items = st.items.filter(function (it) { return it.kind !== "string"; });
    return;
  }
  const norm = normalizeDataTransferFormat(format);
  st.dataMap.delete(norm);
  st.items = st.items.filter(function (it) { return !(it.kind === "string" && it.type === norm); });
};

DataTransfer.prototype.setDragImage = function () {};

Object.defineProperty(DataTransfer.prototype, Symbol.toStringTag, {
  value: "DataTransfer",
  configurable: true,
});

markNative(DataTransfer, "DataTransfer");
markNative(DataTransfer.prototype.setData, "setData");
markNative(DataTransfer.prototype.getData, "getData");
markNative(DataTransfer.prototype.clearData, "clearData");

// Expose globals
global.Blob = Blob;
global.File = File;
global.FileList = FileList;
global.DataTransfer = DataTransfer;
global.DataTransferItem = DataTransferItem;
global.DataTransferItemList = DataTransferItemList;
global.FileReader = FileReader;
global.TextDecoder = TextDecoder;
global.TextEncoder = TextEncoder;

global.Headers = Headers;
if (!global.Request) global.Request = Request; else patchExistingBodyBlob(global.Request);
if (!global.Response) global.Response = Response; else patchExistingBodyBlob(global.Response);

if (typeof window !== "undefined") {
  window.Blob = Blob;
  window.File = File;
  window.FileList = FileList;
  window.DataTransfer = DataTransfer;
  window.DataTransferItem = DataTransferItem;
  window.DataTransferItemList = DataTransferItemList;
  window.FileReader = FileReader;
  window.TextDecoder = TextDecoder;
  window.TextEncoder = TextEncoder;
  window.Headers = Headers;
  window.Request = Request;
  window.Response = Response;
}
if (typeof globalThis !== "undefined") {
  globalThis.Blob = Blob;
  globalThis.File = File;
  globalThis.FileList = FileList;
  globalThis.DataTransfer = DataTransfer;
  globalThis.DataTransferItem = DataTransferItem;
  globalThis.DataTransferItemList = DataTransferItemList;
  globalThis.FileReader = FileReader;
  globalThis.TextDecoder = TextDecoder;
  globalThis.TextEncoder = TextEncoder;
  globalThis.Headers = Headers;
  globalThis.Request = Request;
  globalThis.Response = Response;
}

