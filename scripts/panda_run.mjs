#!/usr/bin/env node
// Parallel CDP test runner for local test/panda/ HTML fixtures (using testing.assertOk()).
import { createServer } from "node:http";
import { readdirSync, readFileSync, statSync, writeFileSync, existsSync } from "node:fs";
import { join, relative, resolve, extname } from "node:path";
import { spawn } from "node:child_process";

const ROOT = resolve(import.meta.dirname, "..");
const PANDA_DIR = resolve(ROOT, "test/panda");

const args = process.argv.slice(2);
const opt = (flag, def) => {
  const i = args.indexOf(flag);
  return i >= 0 && args[i + 1] ? args[i + 1] : def;
};
const binPath = resolve(ROOT, opt("--bin", process.env.CAPSURF_BIN || "bin_dev/main"));
const basePort = parseInt(opt("--port", "19950"), 10);
const jobs = Math.max(1, parseInt(opt("-j", opt("--jobs", "4")), 10));
const filter = opt("--filter", "");
const timeoutMs = parseInt(opt("--timeout", "1500"), 10);
const resultsOut = opt("--results-out", "tmp/panda_results.json");

const MIME = {
  ".html": "text/html; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".mjs": "application/javascript; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".txt": "text/plain; charset=utf-8",
  ".svg": "image/svg+xml",
  ".png": "image/png",
  ".wasm": "application/wasm",
};

const SKIP_DIRS = new Set(["cdp", "bidi", "fixtures", "support", "runner", "integration", "webdriver"]);

function collectHtmlFiles(dir, out = []) {
  for (const name of readdirSync(dir)) {
    const full = join(dir, name);
    const st = statSync(full);
    if (st.isDirectory()) {
      if (!SKIP_DIRS.has(name)) collectHtmlFiles(full, out);
    } else if (name.endsWith(".html") && !name.endsWith(".skip.html") && !name.includes("iframe_")) {
      const rel = relative(PANDA_DIR, full);
      if ((!filter || rel.includes(filter)) && readFileSync(full, "utf8").includes("testing.js")) {
        out.push(rel);
      }
    }
  }
  return out.sort();
}

function startHttpServer() {
  return new Promise((res) => {
    const srv = createServer((req, resHttp) => {
      let urlPath = (req.url || "/").split("?")[0];
      if (urlPath.startsWith("/src/browser/tests/")) {
        urlPath = urlPath.slice("/src/browser/tests/".length);
      } else {
        urlPath = urlPath.replace(/^\/+/, "");
      }
      const filePath = resolve(PANDA_DIR, urlPath);
      if (!filePath.startsWith(PANDA_DIR) || !existsSync(filePath) || statSync(filePath).isDirectory()) {
        resHttp.writeHead(404);
        resHttp.end("Not found");
        return;
      }
      const headers = { "Content-Type": MIME[extname(filePath)] || "application/octet-stream" };
      const hdrFile = filePath + ".headers";
      if (existsSync(hdrFile)) {
        for (const line of readFileSync(hdrFile, "utf8").split(/\r?\n/)) {
          const idx = line.indexOf(":");
          if (idx > 0) headers[line.slice(0, idx).trim()] = line.slice(idx + 1).trim();
        }
      }
      resHttp.writeHead(200, headers);
      resHttp.end(readFileSync(filePath));
    });
    srv.listen(0, "127.0.0.1", () => res({ srv, port: srv.address().port }));
  });
}

async function waitForWsEndpoint(port) {
  const deadline = Date.now() + 10000;
  while (Date.now() < deadline) {
    try {
      const r = await fetch(`http://127.0.0.1:${port}/json/version`);
      if (r.ok) {
        const j = await r.json();
        if (j.webSocketDebuggerUrl) return j.webSocketDebuggerUrl;
      }
    } catch {}
    await new Promise((r) => setTimeout(r, 80));
  }
  throw new Error(`CapSurf CDP not ready on :${port}`);
}

class CdpClient {
  constructor(wsUrl) {
    this.ws = new WebSocket(wsUrl);
    this.nextId = 1;
    this.pending = new Map();
    this.ready = new Promise((res, rej) => {
      this.ws.onopen = res;
      this.ws.onerror = rej;
    });
    this.onEvent = null;
    this.ws.onmessage = (ev) => {
      let msg;
      try {
        msg = JSON.parse(typeof ev.data === "string" ? ev.data : ev.data.toString());
      } catch {
        return;
      }
      if (msg.id && this.pending.has(msg.id)) {
        const { res, rej } = this.pending.get(msg.id);
        this.pending.delete(msg.id);
        if (msg.error) rej(new Error(msg.error.message || JSON.stringify(msg.error)));
        else res(msg.result);
      } else if (msg.method && this.onEvent) {
        this.onEvent(msg);
      }
    };
  }
  send(method, params = {}, sessionId, callTimeoutMs = 5000) {
    const id = this.nextId++;
    const payload = sessionId ? { id, method, params, sessionId } : { id, method, params };
    return new Promise((res, rej) => {
      const timer = setTimeout(() => {
        if (this.pending.delete(id)) rej(new Error(`CDP timeout: ${method}`));
      }, callTimeoutMs);
      this.pending.set(id, {
        res: (v) => { clearTimeout(timer); res(v); },
        rej: (e) => { clearTimeout(timer); rej(e); },
      });
      this.ws.send(JSON.stringify(payload));
    });
  }
  close() {
    try { this.ws.close(); } catch {}
  }
}

async function runWorker(workerIdx, queue, httpPort, state) {
  const port = basePort + workerIdx;
  let proc = spawn(binPath, [`--remote-debugging-port=${port}`], { stdio: "ignore" });
  let cdp = null;

  const ensureCdp = async () => {
    if (cdp && cdp.ws.readyState === 1) return cdp;
    if (cdp) cdp.close();
    try { proc.kill("SIGKILL"); } catch {}
    proc = spawn(binPath, [`--remote-debugging-port=${port}`], { stdio: "ignore" });
    const wsUrl = await waitForWsEndpoint(port);
    cdp = new CdpClient(wsUrl);
    await cdp.ready;
    return cdp;
  };

  try {
    await ensureCdp();
    while (queue.length > 0) {
      const rel = queue.shift();
      if (!rel) break;
      let status = "TIMEOUT", errMsg = "";
      try {
        const client = await ensureCdp();
        const logs = [];
        client.onEvent = (m) => {
          if (m.method === "Runtime.consoleAPICalled" && m.params?.type === "error") {
            const txt = (m.params.args || []).map((a) => a.value ?? a.description ?? "").join(" ");
            if (txt) logs.push(txt);
          }
        };
        const { targetId } = await client.send("Target.createTarget", { url: "about:blank" });
        const { sessionId } = await client.send("Target.attachToTarget", { targetId, flatten: true });
        await client.send("Page.enable", {}, sessionId);
        await client.send("Runtime.enable", {}, sessionId);

        const url = `http://127.0.0.1:${httpPort}/${rel}`;
        await client.send("Page.navigate", { url }, sessionId, timeoutMs + 2000);
        const deadline = Date.now() + timeoutMs;
        while (Date.now() < deadline) {
          const ev = await client.send("Runtime.evaluate", {
            expression: "typeof testing !== 'undefined' && typeof testing.assertOk === 'function' ? testing.assertOk() : 'NO_TESTING'",
            returnByValue: true,
          }, sessionId, 2000);
          if (ev.exceptionDetails) {
            status = "FAIL";
            errMsg = logs.length > 0 ? logs.join(" | ") : (ev.exceptionDetails.exception?.description || ev.exceptionDetails.text || "Exception");
            break;
          }
          const val = ev.result?.value;
          if (val === true) {
            status = "PASS";
            break;
          }
          if (val === "NO_TESTING") {
            status = "FAIL";
            errMsg = "testing.assertOk not defined";
            break;
          }
          await new Promise((r) => setTimeout(r, 40));
        }
        await client.send("Target.closeTarget", { targetId }, undefined, 2000).catch(() => {});
      } catch (e) {
        status = String(e.message || e).includes("timeout") ? "TIMEOUT" : "FAIL";
        errMsg = String(e.message || e);
        if (cdp) { cdp.close(); cdp = null; }
      }

      state.results[rel] = status;
      if (errMsg) state.errors[rel] = errMsg.slice(0, 300);
      if (status === "PASS") state.pass++;
      else if (status === "FAIL") state.fail++;
      else state.timeout++;
      state.done++;
      if (state.done % 50 === 0 || state.done === state.total) {
        console.log(`[panda-run] ${state.done}/${state.total}: PASS=${state.pass} FAIL=${state.fail} TIMEOUT=${state.timeout}`);
      }
    }
  } finally {
    if (cdp) cdp.close();
    try { proc.kill("SIGKILL"); } catch {}
  }
}

async function run() {
  const files = collectHtmlFiles(PANDA_DIR);
  const actualJobs = Math.min(jobs, Math.max(1, files.length));
  console.log(`[panda-run] ${files.length} test(s), jobs=${actualJobs}, timeout=${timeoutMs}ms`);
  const { srv, port: httpPort } = await startHttpServer();
  const queue = [...files];
  const state = { total: files.length, done: 0, pass: 0, fail: 0, timeout: 0, results: {}, errors: {} };

  try {
    await Promise.all(Array.from({ length: actualJobs }, (_, i) => runWorker(i, queue, httpPort, state)));
  } finally {
    srv.close();
  }

  writeFileSync(resolve(ROOT, resultsOut), JSON.stringify({
    total: state.total, pass: state.pass, fail: state.fail, timeout: state.timeout,
    results: state.results, errors: state.errors,
  }, null, 2));
  console.log(`[panda-run] DONE PASS=${state.pass} FAIL=${state.fail} TIMEOUT=${state.timeout} (total ${state.total}) -> ${resultsOut}`);
}

run().catch((e) => {
  console.error(e);
  process.exit(1);
});
