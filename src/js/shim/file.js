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
  if (init) {
    if (init instanceof Headers) {
      init._map.forEach((v, k) => this._map.set(k, v));
    } else if (Array.isArray(init)) {
      for (let i = 0; i < init.length; i++) {
        const entry = init[i];
        if (Array.isArray(entry) && entry.length >= 2) {
          this.append(entry[0], entry[1]);
        }
      }
    } else if (typeof init === "object") {
      const keys = Object.keys(init);
      for (let i = 0; i < keys.length; i++) {
        const k = keys[i];
        this.set(k, init[k]);
      }
    }
  }
}

Headers.prototype.append = function (k, v) {
  const key = String(k).toLowerCase();
  const val = String(v);
  const existing = this._map.get(key);
  this._map.set(key, existing ? existing + ", " + val : val);
};

Headers.prototype.set = function (k, v) {
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
  this._map.delete(String(k).toLowerCase());
};

Headers.prototype.forEach = function (cb, thisArg) {
  this._map.forEach(cb, thisArg);
};

markNative(Headers, "Headers");

function Request(input, init) {
  init = init || {};
  this.url = String(input);
  this.method = (init.method || "GET").toUpperCase();
  this._rawMimeType = extractMimeFromHeaders(init.headers);
  this._body = init.body !== undefined ? init.body : "";
  this.headers = new Headers(init.headers);
}

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
markNative(Request.prototype.blob, "blob");
markNative(Request.prototype.text, "text");
markNative(Request.prototype.json, "json");
markNative(Request.prototype.arrayBuffer, "arrayBuffer");

function Response(body, init) {
  init = init || {};
  this.status = init.status !== undefined ? Number(init.status) : 200;
  this.statusText = init.statusText !== undefined ? String(init.statusText) : "OK";
  this._rawMimeType = extractMimeFromHeaders(init.headers);
  this._body = body !== undefined ? body : "";
  this.headers = new Headers(init.headers);
}

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
  return Promise.resolve(stringToUtf8(String(this._body)).buffer);
};

markNative(Response, "Response");
markNative(Response.prototype.blob, "blob");
markNative(Response.prototype.text, "text");
markNative(Response.prototype.json, "json");
markNative(Response.prototype.arrayBuffer, "arrayBuffer");

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

// Expose globals
global.Blob = Blob;
global.File = File;

if (!global.Headers) global.Headers = Headers;
if (!global.Request) global.Request = Request; else patchExistingBodyBlob(global.Request);
if (!global.Response) global.Response = Response; else patchExistingBodyBlob(global.Response);

if (typeof window !== "undefined") {
  window.Blob = Blob;
  window.File = File;
  if (!window.Headers) window.Headers = global.Headers;
  if (!window.Request) window.Request = global.Request;
  if (!window.Response) window.Response = global.Response;
}
if (typeof globalThis !== "undefined") {
  globalThis.Blob = Blob;
  globalThis.File = File;
  if (!globalThis.Headers) globalThis.Headers = global.Headers;
  if (!globalThis.Request) globalThis.Request = global.Request;
  if (!globalThis.Response) globalThis.Response = global.Response;
}
