// src/js/shim/streams.js
// WHATWG Streams Standard: ReadableStream, WritableStream, TransformStream,
// TextEncoderStream, TextDecoderStream, ByteLengthQueuingStrategy, CountQueuingStrategy

(function () {
  function createDeferred() {
    let resolve, reject;
    const promise = new Promise(function (res, rej) {
      resolve = res;
      reject = rej;
    });
    return { promise: promise, resolve: resolve, reject: reject };
  }

  function CountQueuingStrategy(init) {
    if (!init || typeof init !== "object" || init.highWaterMark === undefined) {
      throw new TypeError("CountQueuingStrategy requires an options object with highWaterMark");
    }
    this.highWaterMark = Number(init.highWaterMark);
  }
  CountQueuingStrategy.prototype.size = function () { return 1; };

  function ByteLengthQueuingStrategy(init) {
    if (!init || typeof init !== "object" || init.highWaterMark === undefined) {
      throw new TypeError("ByteLengthQueuingStrategy requires an options object with highWaterMark");
    }
    this.highWaterMark = Number(init.highWaterMark);
  }
  ByteLengthQueuingStrategy.prototype.size = function (chunk) {
    return chunk.byteLength;
  };

  function extractSize(strategy, defaultHwm) {
    if (!strategy || typeof strategy !== "object") {
      return { highWaterMark: defaultHwm, size: function () { return 1; } };
    }
    const hwm = strategy.highWaterMark !== undefined ? Number(strategy.highWaterMark) : defaultHwm;
    if (Number.isNaN(hwm) || hwm < 0) {
      throw new RangeError("Invalid highWaterMark");
    }
    const size = typeof strategy.size === "function" ? strategy.size : function () { return 1; };
    return { highWaterMark: hwm, size: size };
  }

  // ==================== ReadableStream ====================
  function ReadableStreamDefaultController(stream) {
    this._stream = stream;
  }
  Object.defineProperty(ReadableStreamDefaultController.prototype, "desiredSize", {
    get: function () {
      const s = this._stream;
      if (s._state === "errored") return null;
      if (s._state === "closed") return 0;
      return s._highWaterMark - s._queueTotalSize;
    },
    configurable: true,
  });
  ReadableStreamDefaultController.prototype.enqueue = function (chunk) {
    const s = this._stream;
    if (s._closeRequested || s._state !== "readable") {
      throw new TypeError("Cannot enqueue into a closed or errored ReadableStream");
    }
    if (s._readRequests.length > 0) {
      const req = s._readRequests.shift();
      req.resolve({ value: chunk, done: false });
      return;
    }
    let chunkSize = 1;
    try {
      chunkSize = Number(s._sizeFn(chunk));
      if (!Number.isFinite(chunkSize) || chunkSize < 0) throw new RangeError("Invalid chunk size");
    } catch (e) {
      this.error(e);
      throw e;
    }
    s._queue.push({ value: chunk, size: chunkSize });
    s._queueTotalSize += chunkSize;
    s._callPullIfNeeded();
  };
  ReadableStreamDefaultController.prototype.close = function () {
    const s = this._stream;
    if (s._closeRequested || s._state !== "readable") {
      throw new TypeError("Cannot close a ReadableStream that is not readable");
    }
    s._closeRequested = true;
    if (s._queue.length === 0) {
      s._finishClose();
    }
  };
  ReadableStreamDefaultController.prototype.error = function (e) {
    const s = this._stream;
    if (s._state !== "readable") return;
    s._errorInternal(e);
  };

  function ReadableStreamDefaultReader(stream) {
    if (!(stream instanceof ReadableStream)) {
      throw new TypeError("Illegal constructor");
    }
    if (stream.locked) {
      throw new TypeError("ReadableStream is locked");
    }
    this._stream = stream;
    stream._reader = this;
    this._closedDeferred = createDeferred();
    if (stream._state === "closed") {
      this._closedDeferred.resolve(undefined);
    } else if (stream._state === "errored") {
      this._closedDeferred.reject(stream._storedError);
    }
  }
  Object.defineProperty(ReadableStreamDefaultReader.prototype, "closed", {
    get: function () { return this._closedDeferred.promise; },
    configurable: true,
  });
  ReadableStreamDefaultReader.prototype.read = function () {
    const s = this._stream;
    if (!s) return Promise.reject(new TypeError("Reader is released"));
    if (s._state === "errored") return Promise.reject(s._storedError);
    if (s._queue.length > 0) {
      const item = s._queue.shift();
      s._queueTotalSize = Math.max(0, s._queueTotalSize - item.size);
      if (s._closeRequested && s._queue.length === 0) {
        s._finishClose();
      } else {
        s._callPullIfNeeded();
      }
      return Promise.resolve({ value: item.value, done: false });
    }
    if (s._state === "closed") {
      return Promise.resolve({ value: undefined, done: true });
    }
    const def = createDeferred();
    s._readRequests.push(def);
    s._callPullIfNeeded();
    return def.promise;
  };
  ReadableStreamDefaultReader.prototype.releaseLock = function () {
    const s = this._stream;
    if (!s) return;
    if (s._readRequests.length > 0) {
      throw new TypeError("Cannot release lock while read requests are pending");
    }
    s._reader = null;
    this._stream = null;
    if (s._state !== "readable") {
      this._closedDeferred = createDeferred();
    }
    this._closedDeferred.reject(new TypeError("Reader lock released"));
  };
  ReadableStreamDefaultReader.prototype.cancel = function (reason) {
    const s = this._stream;
    if (!s) return Promise.reject(new TypeError("Reader is released"));
    return s._cancelInternal(reason);
  };

  function ReadableStream(underlyingSource, strategy) {
    underlyingSource = underlyingSource || {};
    const strat = extractSize(strategy, 1);
    this._underlyingSource = underlyingSource;
    this._highWaterMark = strat.highWaterMark;
    this._sizeFn = strat.size;
    this._state = "readable";
    this._storedError = undefined;
    this._queue = [];
    this._queueTotalSize = 0;
    this._readRequests = [];
    this._reader = null;
    this._closeRequested = false;
    this._started = false;
    this._pulling = false;
    this._pullAgain = false;
    this._controller = new ReadableStreamDefaultController(this);

    const self = this;
    if (typeof underlyingSource.start === "function") {
      const startResult = underlyingSource.start(this._controller);
      if (startResult && typeof startResult.then === "function") {
        startResult.then(function () {
          self._started = true;
          self._callPullIfNeeded();
        }, function (e) {
          self._errorInternal(e);
        });
      } else {
        this._started = true;
        Promise.resolve().then(function () { self._callPullIfNeeded(); });
      }
    } else {
      this._started = true;
      Promise.resolve().then(function () { self._callPullIfNeeded(); });
    }
  }
  Object.defineProperty(ReadableStream.prototype, "locked", {
    get: function () { return this._reader !== null; },
    configurable: true,
  });
  ReadableStream.prototype._callPullIfNeeded = function () {
    if (!this._started || this._state !== "readable" || this._closeRequested) return;
    if (typeof this._underlyingSource.pull !== "function") return;
    const desired = this._highWaterMark - this._queueTotalSize;
    if (desired <= 0 && this._readRequests.length === 0) return;
    if (this._pulling) {
      this._pullAgain = true;
      return;
    }
    this._pulling = true;
    const self = this;
    Promise.resolve().then(function () {
      return self._underlyingSource.pull(self._controller);
    }).then(function () {
      self._pulling = false;
      if (self._pullAgain) {
        self._pullAgain = false;
        self._callPullIfNeeded();
      }
    }, function (e) {
      self._pulling = false;
      self._errorInternal(e);
    });
  };
  ReadableStream.prototype._finishClose = function () {
    if (this._state !== "readable") return;
    this._state = "closed";
    if (this._reader) {
      this._reader._closedDeferred.resolve(undefined);
    }
    while (this._readRequests.length > 0) {
      this._readRequests.shift().resolve({ value: undefined, done: true });
    }
  };
  ReadableStream.prototype._errorInternal = function (e) {
    if (this._state !== "readable") return;
    this._state = "errored";
    this._storedError = e;
    this._queue = [];
    this._queueTotalSize = 0;
    if (this._reader) {
      this._reader._closedDeferred.reject(e);
    }
    while (this._readRequests.length > 0) {
      this._readRequests.shift().reject(e);
    }
  };
  ReadableStream.prototype._cancelInternal = function (reason) {
    if (this._state === "closed") return Promise.resolve(undefined);
    if (this._state === "errored") return Promise.reject(this._storedError);
    this._queue = [];
    this._queueTotalSize = 0;
    this._finishClose();
    if (typeof this._underlyingSource.cancel === "function") {
      try {
        return Promise.resolve(this._underlyingSource.cancel(reason)).then(function () { return undefined; });
      } catch (e) {
        return Promise.reject(e);
      }
    }
    return Promise.resolve(undefined);
  };
  ReadableStream.prototype.getReader = function () {
    return new ReadableStreamDefaultReader(this);
  };
  ReadableStream.prototype.cancel = function (reason) {
    if (this.locked) return Promise.reject(new TypeError("Cannot cancel a locked ReadableStream"));
    return this._cancelInternal(reason);
  };
  ReadableStream.prototype.tee = function () {
    const reader = this.getReader();
    let canceled1 = false, canceled2 = false;
    let reason1, reason2;
    let ctrl1, ctrl2;
    const cancelPromise = createDeferred();
    let reading = false;
    let readAgain = false;
    function pullAlgorithm() {
      if (reading) { readAgain = true; return Promise.resolve(); }
      reading = true;
      return reader.read().then(function (res) {
        reading = false;
        if (res.done) {
          if (!canceled1) try { ctrl1.close(); } catch (e) {}
          if (!canceled2) try { ctrl2.close(); } catch (e) {}
          cancelPromise.resolve(undefined);
          return;
        }
        if (!canceled1) try { ctrl1.enqueue(res.value); } catch (e) {}
        if (!canceled2) try { ctrl2.enqueue(res.value); } catch (e) {}
        if (readAgain) { readAgain = false; return pullAlgorithm(); }
      }, function (e) {
        reading = false;
        try { ctrl1.error(e); } catch (_) {}
        try { ctrl2.error(e); } catch (_) {}
        cancelPromise.reject(e);
      });
    }
    const s1 = new ReadableStream({
      start: function (c) { ctrl1 = c; },
      pull: pullAlgorithm,
      cancel: function (r) {
        canceled1 = true; reason1 = r;
        if (canceled2) {
          reader.cancel([reason1, reason2]).then(cancelPromise.resolve, cancelPromise.reject);
        }
        return cancelPromise.promise;
      },
    });
    const s2 = new ReadableStream({
      start: function (c) { ctrl2 = c; },
      pull: pullAlgorithm,
      cancel: function (r) {
        canceled2 = true; reason2 = r;
        if (canceled1) {
          reader.cancel([reason1, reason2]).then(cancelPromise.resolve, cancelPromise.reject);
        }
        return cancelPromise.promise;
      },
    });
    return [s1, s2];
  };
  ReadableStream.prototype.pipeTo = function (dest, options) {
    options = options || {};
    const preventClose = Boolean(options.preventClose);
    const preventAbort = Boolean(options.preventAbort);
    const preventCancel = Boolean(options.preventCancel);
    if (this.locked) return Promise.reject(new TypeError("ReadableStream is locked"));
    if (dest.locked) return Promise.reject(new TypeError("WritableStream is locked"));
    const reader = this.getReader();
    const writer = dest.getWriter();
    function pump() {
      return reader.read().then(function (res) {
        if (res.done) {
          reader.releaseLock();
          if (!preventClose) {
            return writer.close().then(function () { writer.releaseLock(); });
          }
          writer.releaseLock();
          return undefined;
        }
        return writer.ready.then(function () {
          return writer.write(res.value);
        }).then(pump);
      }).catch(function (err) {
        if (!preventAbort) try { writer.abort(err); } catch (_) {}
        if (!preventCancel) try { reader.cancel(err); } catch (_) {}
        try { reader.releaseLock(); } catch (_) {}
        try { writer.releaseLock(); } catch (_) {}
        throw err;
      });
    }
    return pump();
  };
  ReadableStream.prototype.pipeThrough = function (transform, options) {
    if (!transform || typeof transform !== "object" || !transform.writable || !transform.readable) {
      throw new TypeError("pipeThrough requires a TransformStream");
    }
    this.pipeTo(transform.writable, options).catch(function () {});
    return transform.readable;
  };
  ReadableStream.prototype[Symbol.asyncIterator] = function (options) {
    const preventCancel = options && options.preventCancel;
    const reader = this.getReader();
    return {
      next: function () {
        return reader.read().then(function (res) {
          if (res.done) reader.releaseLock();
          return res;
        });
      },
      return: function () {
        if (!preventCancel) return reader.cancel().then(function () { reader.releaseLock(); return { value: undefined, done: true }; });
        reader.releaseLock();
        return Promise.resolve({ value: undefined, done: true });
      },
    };
  };
  ReadableStream.from = function (asyncIterable) {
    const iter = asyncIterable[Symbol.asyncIterator]
      ? asyncIterable[Symbol.asyncIterator]()
      : asyncIterable[Symbol.iterator]();
    return new ReadableStream({
      pull: function (controller) {
        return Promise.resolve(iter.next()).then(function (res) {
          if (res.done) controller.close();
          else controller.enqueue(res.value);
        });
      },
      cancel: function () {
        if (typeof iter.return === "function") return iter.return();
      },
    });
  };

  // ==================== WritableStream ====================
  function WritableStreamDefaultController(stream) {
    this._stream = stream;
  }
  WritableStreamDefaultController.prototype.error = function (e) {
    this._stream._errorInternal(e);
  };

  function WritableStreamDefaultWriter(stream) {
    if (!(stream instanceof WritableStream)) throw new TypeError("Illegal constructor");
    if (stream.locked) throw new TypeError("WritableStream is locked");
    this._stream = stream;
    stream._writer = this;
    this._readyDeferred = createDeferred();
    this._closedDeferred = createDeferred();
    if (stream._state === "writable") {
      if (stream._highWaterMark - stream._queueTotalSize > 0) {
        this._readyDeferred.resolve(undefined);
      }
    } else if (stream._state === "closed") {
      this._readyDeferred.resolve(undefined);
      this._closedDeferred.resolve(undefined);
    } else if (stream._state === "errored") {
      this._readyDeferred.reject(stream._storedError);
      this._closedDeferred.reject(stream._storedError);
    }
  }
  Object.defineProperty(WritableStreamDefaultWriter.prototype, "desiredSize", {
    get: function () {
      const s = this._stream;
      if (!s) throw new TypeError("Writer is released");
      if (s._state === "errored" || s._state === "erroring") return null;
      if (s._state === "closed") return 0;
      return s._highWaterMark - s._queueTotalSize;
    },
    configurable: true,
  });
  Object.defineProperty(WritableStreamDefaultWriter.prototype, "ready", {
    get: function () { return this._readyDeferred.promise; },
    configurable: true,
  });
  Object.defineProperty(WritableStreamDefaultWriter.prototype, "closed", {
    get: function () { return this._closedDeferred.promise; },
    configurable: true,
  });
  WritableStreamDefaultWriter.prototype.write = function (chunk) {
    const s = this._stream;
    if (!s) return Promise.reject(new TypeError("Writer is released"));
    if (s._state === "errored") return Promise.reject(s._storedError);
    if (s._closeRequest || s._state === "closed") {
      return Promise.reject(new TypeError("Cannot write to a closed WritableStream"));
    }
    let sz = 1;
    try {
      sz = Number(s._sizeFn(chunk));
    } catch (e) {
      s._errorInternal(e);
      return Promise.reject(e);
    }
    const def = createDeferred();
    s._writeQueue.push({ type: "write", chunk: chunk, size: sz, deferred: def });
    s._queueTotalSize += sz;
    if (s._highWaterMark - s._queueTotalSize <= 0 && this._readyDeferred) {
      this._readyDeferred = createDeferred();
    }
    s._advanceQueue();
    return def.promise;
  };
  WritableStreamDefaultWriter.prototype.close = function () {
    const s = this._stream;
    if (!s) return Promise.reject(new TypeError("Writer is released"));
    if (s._closeRequest || s._state === "closed") {
      return Promise.reject(new TypeError("WritableStream is already closed"));
    }
    if (s._state === "errored") return Promise.reject(new TypeError("Cannot close an errored WritableStream"));
    const def = createDeferred();
    s._closeRequest = def;
    s._writeQueue.push({ type: "close", deferred: def });
    s._advanceQueue();
    return def.promise;
  };
  WritableStreamDefaultWriter.prototype.abort = function (reason) {
    const s = this._stream;
    if (!s) return Promise.reject(new TypeError("Writer is released"));
    return s._abortInternal(reason);
  };
  WritableStreamDefaultWriter.prototype.releaseLock = function () {
    const s = this._stream;
    if (!s) return;
    s._writer = null;
    this._stream = null;
  };

  function WritableStream(underlyingSink, strategy) {
    underlyingSink = underlyingSink || {};
    const strat = extractSize(strategy, 1);
    this._underlyingSink = underlyingSink;
    this._highWaterMark = strat.highWaterMark;
    this._sizeFn = strat.size;
    this._state = "writable";
    this._storedError = undefined;
    this._writeQueue = [];
    this._queueTotalSize = 0;
    this._inFlight = false;
    this._closeRequest = null;
    this._writer = null;
    this._started = false;
    this._controller = new WritableStreamDefaultController(this);

    const self = this;
    if (typeof underlyingSink.start === "function") {
      Promise.resolve().then(function () {
        return underlyingSink.start(self._controller);
      }).then(function () {
        self._started = true;
        self._advanceQueue();
      }, function (e) {
        self._errorInternal(e);
      });
    } else {
      this._started = true;
    }
  }
  Object.defineProperty(WritableStream.prototype, "locked", {
    get: function () { return this._writer !== null; },
    configurable: true,
  });
  WritableStream.prototype._advanceQueue = function () {
    if (!this._started || this._inFlight || this._state !== "writable") return;
    if (this._writeQueue.length === 0) return;
    const item = this._writeQueue.shift();
    this._inFlight = true;
    const self = this;
    if (item.type === "close") {
      const closeFn = this._underlyingSink.close;
      Promise.resolve().then(function () {
        return typeof closeFn === "function" ? closeFn() : undefined;
      }).then(function () {
        self._inFlight = false;
        self._state = "closed";
        item.deferred.resolve(undefined);
        if (self._writer) self._writer._closedDeferred.resolve(undefined);
      }, function (e) {
        self._inFlight = false;
        item.deferred.reject(e);
        self._errorInternal(e);
      });
      return;
    }
    const writeFn = this._underlyingSink.write;
    Promise.resolve().then(function () {
      return typeof writeFn === "function" ? writeFn(item.chunk, self._controller) : undefined;
    }).then(function () {
      self._inFlight = false;
      self._queueTotalSize = Math.max(0, self._queueTotalSize - item.size);
      if (self._writer && self._highWaterMark - self._queueTotalSize > 0) {
        self._writer._readyDeferred.resolve(undefined);
      }
      item.deferred.resolve(undefined);
      self._advanceQueue();
    }, function (e) {
      self._inFlight = false;
      item.deferred.reject(e);
      self._errorInternal(e);
    });
  };
  WritableStream.prototype._errorInternal = function (e) {
    if (this._state === "errored" || this._state === "closed") return;
    this._state = "errored";
    this._storedError = e;
    while (this._writeQueue.length > 0) {
      this._writeQueue.shift().deferred.reject(e);
    }
    if (this._writer) {
      this._writer._readyDeferred.reject(e);
      this._writer._closedDeferred.reject(e);
    }
  };
  WritableStream.prototype._abortInternal = function (reason) {
    if (this._state === "closed" || this._state === "errored") return Promise.resolve(undefined);
    const err = reason !== undefined ? reason : new DOMException("Aborted", "AbortError");
    this._errorInternal(err);
    if (typeof this._underlyingSink.abort === "function") {
      try {
        return Promise.resolve(this._underlyingSink.abort(reason)).then(function () { return undefined; });
      } catch (e) {
        return Promise.reject(e);
      }
    }
    return Promise.resolve(undefined);
  };
  WritableStream.prototype.getWriter = function () {
    return new WritableStreamDefaultWriter(this);
  };
  WritableStream.prototype.abort = function (reason) {
    if (this.locked) return Promise.reject(new TypeError("Cannot abort a locked WritableStream"));
    return this._abortInternal(reason);
  };
  WritableStream.prototype.close = function () {
    if (this.locked) return Promise.reject(new TypeError("Cannot close a locked WritableStream"));
    const w = this.getWriter();
    return w.close().then(function () { w.releaseLock(); });
  };

  // ==================== TransformStream ====================
  function TransformStreamDefaultController(ts) {
    this._ts = ts;
  }
  Object.defineProperty(TransformStreamDefaultController.prototype, "desiredSize", {
    get: function () { return this._ts._readableController.desiredSize; },
    configurable: true,
  });
  TransformStreamDefaultController.prototype.enqueue = function (chunk) {
    this._ts._readableController.enqueue(chunk);
  };
  TransformStreamDefaultController.prototype.terminate = function () {
    this._ts._readableController.close();
  };
  TransformStreamDefaultController.prototype.error = function (reason) {
    this._ts._readableController.error(reason);
    this._ts._writableController.error(reason);
  };

  function TransformStream(transformer, writableStrategy, readableStrategy) {
    transformer = transformer || {};
    const self = this;
    this._transformer = transformer;
    this._controller = new TransformStreamDefaultController(this);
    let startResolve;
    const startPromise = new Promise(function (res) { startResolve = res; });

    this.readable = new ReadableStream({
      start: function (c) {
        self._readableController = c;
      },
      pull: function () {
        if (self._pullResolve) {
          const r = self._pullResolve;
          self._pullResolve = null;
          r();
        }
      },
      cancel: function (reason) {
        self._writableController.error(reason);
      },
    }, readableStrategy);

    this.writable = new WritableStream({
      start: function (c) {
        self._writableController = c;
        if (typeof transformer.start === "function") {
          return Promise.resolve(transformer.start(self._controller)).then(startResolve, function (e) {
            startResolve();
            throw e;
          });
        }
        startResolve();
      },
      write: function (chunk) {
        return startPromise.then(function () {
          try {
            if (typeof transformer.transform === "function") {
              return Promise.resolve(transformer.transform(chunk, self._controller));
            }
            self._controller.enqueue(chunk);
          } catch (e) {
            self._controller.error(e);
            throw e;
          }
        }).catch(function (e) {
          self._controller.error(e);
          throw e;
        });
      },
      close: function () {
        return startPromise.then(function () {
          try {
            if (typeof transformer.flush === "function") {
              return Promise.resolve(transformer.flush(self._controller));
            }
          } catch (e) {
            self._controller.error(e);
            throw e;
          }
        }).then(function () {
          try { self._readableController.close(); } catch (e) {}
        }, function (e) {
          self._controller.error(e);
          throw e;
        });
      },
      abort: function (reason) {
        self._readableController.error(reason);
      },
    }, writableStrategy);
  }

  // ==================== TextEncoderStream & TextDecoderStream ====================
  function TextEncoderStream() {
    const encoder = new TextEncoder();
    const ts = new TransformStream({
      transform: function (chunk, controller) {
        const str = String(chunk);
        if (str.length > 0) {
          controller.enqueue(encoder.encode(str));
        }
      },
    });
    this.readable = ts.readable;
    this.writable = ts.writable;
  }
  Object.defineProperty(TextEncoderStream.prototype, "encoding", {
    get: function () { return "utf-8"; },
    configurable: true,
  });

  function TextDecoderStream(label, options) {
    if (label !== undefined && String(label).trim().toLowerCase() === "iso-2022-kr") {
      throw new RangeError("Failed to construct 'TextDecoderStream': The encoding label provided ('iso-2022-kr') is invalid.");
    }
    const decoder = new TextDecoder(label, options);
    const enc = String(decoder.encoding || "utf-8").toLowerCase();
    const ENC_NORM = {
      "latin1": "windows-1252",
      "iso-8859-1": "windows-1252",
      "ascii": "windows-1252",
      "us-ascii": "windows-1252",
      "utf-16": "utf-16le",
      "sjis": "shift_jis",
      "gb2312": "gbk",
    };
    this._encoding = ENC_NORM[enc] || enc;
    this._fatal = decoder.fatal;
    this._ignoreBOM = decoder.ignoreBOM;
    const ts = new TransformStream({
      transform: function (chunk, controller) {
        const out = decoder.decode(chunk, { stream: true });
        if (out.length > 0) controller.enqueue(out);
      },
      flush: function (controller) {
        const out = decoder.decode();
        if (out.length > 0) controller.enqueue(out);
      },
    });
    this.readable = ts.readable;
    this.writable = ts.writable;
  }
  Object.defineProperty(TextDecoderStream.prototype, "encoding", {
    get: function () { return this._encoding; },
    configurable: true,
  });
  Object.defineProperty(TextDecoderStream.prototype, "fatal", {
    get: function () { return this._fatal; },
    configurable: true,
  });
  Object.defineProperty(TextDecoderStream.prototype, "ignoreBOM", {
    get: function () { return this._ignoreBOM; },
    configurable: true,
  });

  const exports = {
    ReadableStream: ReadableStream,
    ReadableStreamDefaultReader: ReadableStreamDefaultReader,
    ReadableStreamDefaultController: ReadableStreamDefaultController,
    WritableStream: WritableStream,
    WritableStreamDefaultWriter: WritableStreamDefaultWriter,
    WritableStreamDefaultController: WritableStreamDefaultController,
    TransformStream: TransformStream,
    TransformStreamDefaultController: TransformStreamDefaultController,
    TextEncoderStream: TextEncoderStream,
    TextDecoderStream: TextDecoderStream,
    CountQueuingStrategy: CountQueuingStrategy,
    ByteLengthQueuingStrategy: ByteLengthQueuingStrategy,
  };
  for (const k of Object.keys(exports)) {
    global[k] = exports[k];
    if (typeof globalThis !== "undefined") globalThis[k] = exports[k];
    if (typeof window !== "undefined") window[k] = exports[k];
  }
})();
