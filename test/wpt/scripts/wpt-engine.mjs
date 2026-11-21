// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0

// Launch and manage CapSurf's native binary (--remote-debugging-port) for WPT,
// Puppeteer, and Playwright test runners.

import { spawn } from "node:child_process";
import { existsSync } from "node:fs";
import { resolve } from "node:path";
import { setTimeout as sleep } from "node:timers/promises";
import { fileURLToPath } from "node:url";

const REPO_ROOT = resolve(import.meta.dirname, "../../..");
export const DEFAULT_BIN = process.env.CAPSURF_BIN
  ? resolve(REPO_ROOT, process.env.CAPSURF_BIN)
  : resolve(REPO_ROOT, "bin/main");
export const DEFAULT_DIST = DEFAULT_BIN;

const USAGE = `usage: node test/wpt/scripts/wpt-engine.mjs prepare [--bin <path>]
       node test/wpt/scripts/wpt-engine.mjs serve --port <n> [--bin <path>] [--timeout <ms>]`;

function parseArgs(argv) {
  const options = { bin: DEFAULT_BIN, timeoutMs: 15_000 };
  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "--help" || arg === "-h") {
      options.help = true;
      return options;
    }
    const next = () => {
      const value = argv[++i];
      if (!value) throw new Error(`${arg} requires a value`);
      return value;
    };
    if (arg === "--bin" || arg === "--dist") options.bin = resolve(REPO_ROOT, next());
    else if (arg === "--port") options.port = Number(next());
    else if (arg === "--timeout") options.timeoutMs = Number(next());
    else if (arg === "--inspector-port" || arg === "--config" || arg === "--assets" || arg === "--var") {
      // Accepted for CLI compatibility with legacy callers.
      next();
    } else throw new Error(`unknown option: ${arg}`);
  }
  return options;
}

/**
 * Poll CapSurf's /json/version endpoint until webSocketDebuggerUrl is available.
 * @param {number} port
 * @param {import("node:child_process").ChildProcess | null} child
 * @param {number} [timeoutMs]
 * @returns {Promise<string>}
 */
export async function waitForDevTools(port, child = null, timeoutMs = 15_000) {
  const url = `http://127.0.0.1:${port}/json/version`;
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    if (child && (child.exitCode !== null || child.signalCode)) {
      throw new Error(
        `CapSurf exited prematurely on port ${port} (code=${child.exitCode}, signal=${child.signalCode})`,
      );
    }
    try {
      const res = await fetch(url, { signal: AbortSignal.timeout(1500) });
      if (res.ok) {
        const info = await res.json();
        if (info && typeof info.webSocketDebuggerUrl === "string" && info.webSocketDebuggerUrl) {
          return info.webSocketDebuggerUrl;
        }
      }
    } catch {}
    await sleep(100);
  }
  throw new Error(`Timed out after ${timeoutMs}ms waiting for CapSurf on ${url}`);
}

/**
 * Spawn CapSurf with --remote-debugging-port=<port> and wait until /json/version is ready.
 * @param {{ port: number, binPath?: string, extraArgs?: string[], logFd?: number | null, timeoutMs?: number, detached?: boolean }} opts
 * @returns {Promise<{ port: number, wsEndpoint: string, proc: import("node:child_process").ChildProcess, stop: () => void }>}
 */
export async function startCapSurfEngine({
  port,
  binPath = DEFAULT_BIN,
  extraArgs = [],
  logFd = null,
  timeoutMs = 15_000,
  detached = false,
}) {
  if (!existsSync(binPath)) {
    throw new Error(`CapSurf binary not found at ${binPath} (set CAPSURF_BIN or build ./bin/main)`);
  }
  const proc = spawn(
    binPath,
    [
      `--remote-debugging-port=${port}`,
      "--headless",
      "--no-sandbox",
      "--disable-gpu",
      ...extraArgs,
    ],
    {
      cwd: REPO_ROOT,
      stdio: logFd != null ? ["ignore", logFd, logFd] : ["ignore", "pipe", "pipe"],
      detached,
      env: process.env,
    },
  );
  if (detached) proc.unref();

  let stderrTail = "";
  if (proc.stderr) {
    proc.stderr.on("data", (chunk) => {
      stderrTail = (stderrTail + chunk.toString()).slice(-2048);
    });
  }

  const stop = () => {
    try {
      if (detached && proc.pid) process.kill(-proc.pid, "SIGKILL");
      else proc.kill("SIGKILL");
    } catch {}
  };

  try {
    const wsEndpoint = await waitForDevTools(port, proc, timeoutMs);
    return { port, wsEndpoint, proc, stop };
  } catch (err) {
    stop();
    if (stderrTail) {
      throw new Error(`${err.message}\n${stderrTail}`);
    }
    throw err;
  }
}

/**
 * Engine pool helper compatible with pptr-run.mjs and pw-run.mjs.
 * @param {{ tag?: string, firstPort?: number, binPath?: string, timeoutMs?: number }} [opts]
 */
export function localEngines({
  tag = "capsurf",
  firstPort = 9720,
  binPath = DEFAULT_BIN,
  timeoutMs = 15_000,
} = {}) {
  let nextPort = firstPort;
  return {
    async buildBundles() {
      if (!existsSync(binPath)) {
        throw new Error(
          `[${tag}] CapSurf binary not found at ${binPath} (set CAPSURF_BIN or build ./bin/main)`,
        );
      }
    },
    async startEngine() {
      const port = nextPort++;
      return startCapSurfEngine({ port, binPath, timeoutMs });
    },
  };
}

/** @param {string} [binPath] */
export async function prepare(binPath = DEFAULT_BIN) {
  if (!existsSync(binPath)) {
    throw new Error(`CapSurf binary not found at ${binPath} (set CAPSURF_BIN or build ./bin/main)`);
  }
  return { bin: binPath };
}

async function serve(options) {
  if (!Number.isInteger(options.port) || options.port < 1 || options.port > 65535) {
    throw new Error("serve needs a valid --port");
  }
  const engine = await startCapSurfEngine({
    port: options.port,
    binPath: options.bin,
    timeoutMs: options.timeoutMs,
  });
  console.log(`[wpt-engine] ready on ${engine.wsEndpoint} (bin ${options.bin})`);
  const stop = () => {
    engine.stop();
    process.exit(0);
  };
  process.once("SIGTERM", stop);
  process.once("SIGINT", stop);
  engine.proc.once("exit", (code) => {
    process.exit(code ?? 0);
  });
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const [command, ...args] = process.argv.slice(2);
  if (!command || command === "--help" || command === "-h") {
    console.log(USAGE);
    process.exit(0);
  }
  const options = parseArgs(args);
  if (options.help) {
    console.log(USAGE);
    process.exit(0);
  }
  if (command === "prepare") {
    await prepare(options.bin);
    console.log(`[wpt-engine] CapSurf binary ready: ${options.bin}`);
  } else if (command === "serve") {
    await serve(options);
  } else {
    console.error(USAGE);
    process.exit(2);
  }
}
