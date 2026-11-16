// test/e2e/harness.js
// CapSurf Native Headless Browser E2E Test Harness
// Spawns ./bin/main as a native independent child process with Chromium CLI args,
// discovers CDP WebSocket endpoint, connects via Node.js native WebSocket,
// and provides Jest/Vitest-compatible test() and expect() assertors.

const { spawn } = require('node:child_process');
const http = require('node:http');

const DEFAULT_PORT = Number(process.env.REMOTE_DEBUGGING_PORT || 19230);
const DEFAULT_BIN = process.env.CAPSURF_BIN || './bin/main';

class AssertionError extends Error {
  constructor(message) {
    super(message);
    this.name = 'AssertionError';
  }
}

function deepEqual(a, b) {
  if (Object.is(a, b)) return true;
  if (typeof a !== 'object' || a === null || typeof b !== 'object' || b === null) {
    return false;
  }
  if (Array.isArray(a) !== Array.isArray(b)) return false;
  const keysA = Object.keys(a);
  const keysB = Object.keys(b);
  if (keysA.length !== keysB.length) return false;
  for (const k of keysA) {
    if (!Object.prototype.hasOwnProperty.call(b, k)) return false;
    if (!deepEqual(a[k], b[k])) return false;
  }
  return true;
}

function expect(actual) {
  return {
    toBe(expected) {
      if (!Object.is(actual, expected)) {
        throw new AssertionError(`Expected ${JSON.stringify(expected)} (${typeof expected}), but got ${JSON.stringify(actual)} (${typeof actual})`);
      }
    },
    toEqual(expected) {
      if (!deepEqual(actual, expected)) {
        throw new AssertionError(`Expected deep equality:\n  Expected: ${JSON.stringify(expected)}\n  Received: ${JSON.stringify(actual)}`);
      }
    },
    toStrictEqual(expected) {
      this.toEqual(expected);
    },
    toBeFalsy() {
      if (actual) {
        throw new AssertionError(`Expected value to be falsy, but got: ${JSON.stringify(actual)}`);
      }
    },
    toBeTruthy() {
      if (!actual) {
        throw new AssertionError(`Expected value to be truthy, but got: ${JSON.stringify(actual)}`);
      }
    },
    toBeNull() {
      if (actual !== null) {
        throw new AssertionError(`Expected null, but got: ${JSON.stringify(actual)}`);
      }
    },
    toBeUndefined() {
      if (actual !== undefined) {
        throw new AssertionError(`Expected undefined, but got: ${JSON.stringify(actual)}`);
      }
    },
    toBeDefined() {
      if (actual === undefined) {
        throw new AssertionError(`Expected value to be defined, but got undefined`);
      }
    },
    toContain(expected) {
      if (typeof actual === 'string' || Array.isArray(actual)) {
        if (!actual.includes(expected)) {
          throw new AssertionError(`Expected ${JSON.stringify(actual)} to contain ${JSON.stringify(expected)}`);
        }
      } else {
        throw new AssertionError(`toContain requires string or array, got: ${typeof actual}`);
      }
    },
    get not() {
      return {
        toBe(expected) {
          if (Object.is(actual, expected)) {
            throw new AssertionError(`Expected value NOT to be ${JSON.stringify(expected)}`);
          }
        },
        toEqual(expected) {
          if (deepEqual(actual, expected)) {
            throw new AssertionError(`Expected value NOT to equal ${JSON.stringify(expected)}`);
          }
        },
        toBeFalsy() {
          if (!actual) {
            throw new AssertionError(`Expected value NOT to be falsy`);
          }
        },
        toBeTruthy() {
          if (actual) {
            throw new AssertionError(`Expected value NOT to be truthy`);
          }
        },
        toBeNull() {
          if (actual === null) {
            throw new AssertionError(`Expected value NOT to be null`);
          }
        },
        toBeUndefined() {
          if (actual === undefined) {
            throw new AssertionError(`Expected value NOT to be undefined`);
          }
        },
      };
    },
  };
}

expect.poll = function (fn, { timeout = 10000, interval = 50 } = {}) {
  const startTime = Date.now();
  async function pollUntil(matcher) {
    let lastError;
    while (Date.now() - startTime < timeout) {
      try {
        const val = await fn();
        matcher(val);
        return;
      } catch (err) {
        lastError = err;
        await new Promise((r) => setTimeout(r, interval));
      }
    }
    throw lastError || new Error(`expect.poll timed out after ${timeout}ms`);
  }

  return {
    async toBe(expected) {
      await pollUntil((val) => expect(val).toBe(expected));
    },
    async toEqual(expected) {
      await pollUntil((val) => expect(val).toEqual(expected));
    },
    async toBeTruthy() {
      await pollUntil((val) => expect(val).toBeTruthy());
    },
    async toBeFalsy() {
      await pollUntil((val) => expect(val).toBeFalsy());
    },
  };
};

class CDPClient {
  constructor(wsUrl) {
    this.wsUrl = wsUrl;
    this.ws = null;
    this.nextId = 0;
    this.pending = new Map();
    this.listeners = new Map();
  }

  async connect() {
    return new Promise((resolve, reject) => {
      this.ws = new WebSocket(this.wsUrl);
      this.ws.onopen = () => resolve();
      this.ws.onerror = (err) => reject(err);
      this.ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          if (msg.id && this.pending.has(msg.id)) {
            const { resolve: res, reject: rej } = this.pending.get(msg.id);
            this.pending.delete(msg.id);
            if (msg.error) {
              const err = new Error(msg.error.message || JSON.stringify(msg.error));
              err.code = msg.error.code;
              rej(err);
            } else {
              res(msg.result);
            }
          } else if (msg.method) {
            const handlers = this.listeners.get(msg.method) || [];
            for (const h of handlers) {
              try {
                h(msg.params);
              } catch (e) {
                console.error(`Error in event handler for ${msg.method}:`, e);
              }
            }
          }
        } catch (e) {
          console.error('Failed to parse incoming CDP message:', e);
        }
      };
      this.ws.onclose = () => {
        for (const [id, { reject: rej }] of this.pending) {
          rej(new Error(`CDP connection closed while request id=${id} was pending`));
        }
        this.pending.clear();
      };
    });
  }

  async send(method, params = {}) {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      throw new Error(`WebSocket is not open (readyState=${this.ws?.readyState})`);
    }
    const id = ++this.nextId;
    return new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject });
      this.ws.send(JSON.stringify({ id, method, params }));
    });
  }

  on(event, handler) {
    if (!this.listeners.has(event)) {
      this.listeners.set(event, []);
    }
    this.listeners.get(event).push(handler);
  }

  off(event, handler) {
    const list = this.listeners.get(event);
    if (!list) return;
    const idx = list.indexOf(handler);
    if (idx !== -1) list.splice(idx, 1);
  }

  async close() {
    if (this.ws) {
      if (this.ws.readyState === WebSocket.OPEN) {
        this.ws.close();
      }
      this.ws = null;
    }
  }
}

function spawnCapSurf({ port = DEFAULT_PORT, binPath = DEFAULT_BIN } = {}) {
  const proc = spawn(binPath, [
    `--remote-debugging-port=${port}`,
    '--headless',
    '--window-size=1280,720',
    '--no-sandbox',
    '--disable-gpu',
  ], {
    stdio: ['ignore', 'pipe', 'pipe'],
  });

  const devtoolsRegex = /DevTools listening on (ws:\/\/[^\s]+)/;

  const wsUrlPromise = new Promise((resolve, reject) => {
    const timer = setTimeout(() => {
      reject(new Error(`Timed out waiting for CapSurf DevTools announcement on port ${port}`));
    }, 10000);

    function onData(chunk) {
      const text = chunk.toString();
      const match = text.match(devtoolsRegex);
      if (match) {
        clearTimeout(timer);
        resolve(match[1]);
      }
    }

    proc.stdout.on('data', onData);
    proc.stderr.on('data', onData);

    proc.on('exit', (code, signal) => {
      clearTimeout(timer);
      reject(new Error(`CapSurf process exited prematurely (code=${code}, signal=${signal})`));
    });
  });

  return { proc, wsUrlPromise };
}

async function stopCapSurf(proc) {
  if (!proc || proc.killed) return;
  return new Promise((resolve) => {
    const timer = setTimeout(() => {
      try {
        proc.kill('SIGKILL');
      } catch {}
      resolve();
    }, 2000);
    proc.once('exit', () => {
      clearTimeout(timer);
      resolve();
    });
    try {
      proc.kill('SIGTERM');
    } catch {
      clearTimeout(timer);
      resolve();
    }
  });
}

// Test Registry
const testQueue = [];
let suiteScheduled = false;

function test(name, fn, timeout = 120000) {
  testQueue.push({ name, fn, timeout });
  if (!suiteScheduled && !process.env.MANUAL_RUNNER) {
    suiteScheduled = true;
    process.nextTick(runAllQueuedTests);
  }
}

async function runAllQueuedTests({ port = DEFAULT_PORT, binPath = DEFAULT_BIN } = {}) {
  if (testQueue.length === 0) return { total: 0, passed: 0, failed: 0, results: [] };

  const testsToRun = [...testQueue];
  testQueue.length = 0;

  console.log(`\n==================================================================`);
  console.log(` CapSurf E2E Test Suite (Total: ${testsToRun.length} tests)`);
  console.log(` Binary: ${binPath} | Port: ${port}`);
  console.log(`==================================================================\n`);

  let proc = null;
  let cdp = null;
  const results = [];
  let passedCount = 0;
  let failedCount = 0;

  try {
    const spawned = spawnCapSurf({ port, binPath });
    proc = spawned.proc;
    const wsUrl = await spawned.wsUrlPromise;
    console.log(`[Harness] Connected to CapSurf CDP at ${wsUrl}\n`);

    cdp = new CDPClient(wsUrl);
    await cdp.connect();

    for (let i = 0; i < testsToRun.length; i++) {
      const { name, fn, timeout } = testsToRun[i];
      const start = Date.now();
      process.stdout.write(`  [${i + 1}/${testsToRun.length}] ${name} ... `);

      let timer;
      try {
        const timeoutPromise = new Promise((_, reject) => {
          timer = setTimeout(() => reject(new Error(`Test timed out after ${timeout}ms`)), timeout);
        });

        await Promise.race([
          fn({ cdpSession: cdp, cdp, browser: cdp, page: cdp }),
          timeoutPromise,
        ]);
        clearTimeout(timer);

        const duration = Date.now() - start;
        console.log(`✔ PASS (${duration}ms)`);
        passedCount++;
        results.push({ name, passed: true, duration });
      } catch (err) {
        clearTimeout(timer);
        const duration = Date.now() - start;
        console.log(`✘ FAIL (${duration}ms)`);
        console.error(`\n    ${err.stack || err.message}\n`);
        failedCount++;
        results.push({ name, passed: false, duration, error: err });
      }
    }
  } catch (startupErr) {
    console.error(`[Harness] Failed during test suite execution:`, startupErr);
    failedCount += testsToRun.length - passedCount;
  } finally {
    if (cdp) {
      await cdp.close().catch(() => {});
    }
    if (proc) {
      await stopCapSurf(proc);
    }
  }

  console.log(`\n------------------------------------------------------------------`);
  console.log(` Test Summary: ${passedCount} passed, ${failedCount} failed of ${testsToRun.length} total`);
  console.log(` Status: ${failedCount === 0 ? '✔ ALL TESTS PASSED' : '✘ TESTS FAILED'}`);
  console.log(`------------------------------------------------------------------\n`);

  const summary = {
    total: testsToRun.length,
    passed: passedCount,
    failed: failedCount,
    results,
  };

  if (!process.env.MANUAL_RUNNER && failedCount > 0) {
    process.exitCode = 1;
  }

  return summary;
}

module.exports = {
  expect,
  test,
  CDPClient,
  spawnCapSurf,
  stopCapSurf,
  runAllQueuedTests,
};
