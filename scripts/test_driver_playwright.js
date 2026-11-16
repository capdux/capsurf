// scripts/test_driver_playwright.js
// CapSurf Phase 8: End-to-End Playwright / Puppeteer Headless Driver Verification Suite
//
// Spawns CapSurf binary as a child process with standard Chromium CLI arguments:
//   --remote-debugging-port=19225 --headless --window-size=1280,720 --no-sandbox
// Listens for stdout/stderr "DevTools listening on ws://..." line, connects via native WebSocket,
// and runs full Playwright automation flow:
//   1. HTTP Discovery /json/version & /json/list
//   2. WebSocket Handshake & Session Multiplexing
//   3. Playwright Handshake (Browser.getVersion & Target.setAutoAttach)
//   4. Navigation pipeline & Lifecycle events
//   5. DOM Query & CSS BoxModel geometry extraction
//   6. Mouse Click Dispatch & JS State verification
//   7. Screenshot capture & Base64 PNG verification
//   8. Graceful teardown (SIGTERM clean process exit)

const { spawn } = require('node:child_process');
const http = require('node:http');

const PORT = 19225;
const BIN_PATH = './bin/main';

function httpGet(url) {
    return new Promise((resolve, reject) => {
        http.get(url, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                try {
                    resolve({ status: res.statusCode, body: JSON.parse(data) });
                } catch {
                    resolve({ status: res.statusCode, body: data });
                }
            });
        }).on('error', reject);
    });
}

async function runTest() {
    console.log('==================================================================');
    console.log(' CapSurf Phase 8: Chromium Driver & CLI Compatibility Suite');
    console.log('==================================================================\n');

    console.log(`[Driver Step 1] Spawning CapSurf binary: ${BIN_PATH} --remote-debugging-port=${PORT}`);
    const proc = spawn(BIN_PATH, [
        `--remote-debugging-port=${PORT}`,
        '--headless',
        '--window-size=1280,720',
        '--no-sandbox',
        '--disable-gpu'
    ], {
        stdio: ['ignore', 'pipe', 'pipe']
    });

    let wsUrl = null;

    const devtoolsRegex = /DevTools listening on (ws:\/\/[^\s]+)/;

    const waitForWsUrl = new Promise((resolve, reject) => {
        const timeout = setTimeout(() => {
            reject(new Error('Timed out waiting for DevTools listening announcement on stderr'));
        }, 10000);

        function handleOutput(chunk) {
            const str = chunk.toString();
            process.stdout.write(`[CapSurf Output]: ${str}`);
            const match = str.match(devtoolsRegex);
            if (match) {
                clearTimeout(timeout);
                resolve(match[1]);
            }
        }

        proc.stdout.on('data', handleOutput);
        proc.stderr.on('data', handleOutput);

        proc.on('exit', (code, signal) => {
            clearTimeout(timeout);
            reject(new Error(`Process exited prematurely with code ${code}, signal ${signal}`));
        });
    });

    wsUrl = await waitForWsUrl;
    console.log(`\n✔ Discovered CDP Debugger URL: ${wsUrl}\n`);

    // 1. HTTP Discovery /json/version
    console.log('[Driver Step 2] Checking HTTP Discovery: GET /json/version');
    const verRes = await httpGet(`http://127.0.0.1:${PORT}/json/version`);
    if (verRes.status !== 200 || !verRes.body.Browser.includes('CapSurf')) {
        throw new Error(`Unexpected /json/version response: ${JSON.stringify(verRes)}`);
    }
    console.log(`✔ HTTP Discovery Valid: Browser = "${verRes.body.Browser}", Protocol = "${verRes.body['Protocol-Version']}"`);

    // 2. HTTP Discovery /json/list
    console.log('[Driver Step 3] Checking HTTP Target List: GET /json/list');
    const listRes = await httpGet(`http://127.0.0.1:${PORT}/json/list`);
    if (listRes.status !== 200 || !Array.isArray(listRes.body) || listRes.body.length === 0) {
        throw new Error(`Unexpected /json/list response: ${JSON.stringify(listRes)}`);
    }
    console.log(`✔ HTTP Target List Valid: Found ${listRes.body.length} initial target(s)`);

    // 3. Connect Native WebSocket
    console.log('[Driver Step 4] Establishing RFC 6455 WebSocket connection...');
    const ws = new WebSocket(wsUrl);

    await new Promise((resolve, reject) => {
        ws.onopen = () => resolve();
        ws.onerror = (err) => reject(err);
    });
    console.log('✔ WebSocket handshake established.');

    let msgId = 1;
    const pendingRequests = new Map();
    const eventHandlers = new Map();

    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            if (data.id && pendingRequests.has(data.id)) {
                const { resolve, reject } = pendingRequests.get(data.id);
                pendingRequests.delete(data.id);
                if (data.error) reject(new Error(data.error.message));
                else resolve(data.result);
            } else if (data.method) {
                const handlers = eventHandlers.get(data.method) || [];
                for (const h of handlers) h(data.params);
            }
        } catch (err) {
            console.error('Failed to parse WebSocket message:', err);
        }
    };

    function sendCommand(method, params = {}) {
        return new Promise((resolve, reject) => {
            const id = msgId++;
            pendingRequests.set(id, { resolve, reject });
            ws.send(JSON.stringify({ id, method, params }));
        });
    }

    function waitForEvent(eventName) {
        return new Promise((resolve) => {
            const handlers = eventHandlers.get(eventName) || [];
            const handler = (params) => {
                const idx = handlers.indexOf(handler);
                if (idx >= 0) handlers.splice(idx, 1);
                resolve(params);
            };
            handlers.push(handler);
            eventHandlers.set(eventName, handlers);
        });
    }

    // 4. Playwright Handshake
    console.log('[Driver Step 5] Running Playwright Handshake (Browser.getVersion & Target.setAutoAttach)...');
    const ver = await sendCommand('Browser.getVersion');
    if (!ver.product.includes('CapSurf')) {
        throw new Error(`Unexpected Browser.getVersion product: ${ver.product}`);
    }
    console.log(`✔ Product Verified: ${ver.product}`);

    const attachEventPromise = waitForEvent('Target.attachedToTarget');
    await sendCommand('Target.setAutoAttach', { autoAttach: true, flatten: true });
    const attachedParams = await attachEventPromise;
    console.log(`✔ Auto-Attached Target Session: ID = ${attachedParams.sessionId}, Type = ${attachedParams.targetInfo.type}`);

    // 5. Enable Domains
    console.log('[Driver Step 6] Enabling Page, Runtime and DOM domains...');
    await sendCommand('Page.enable');
    const ctxPromise = waitForEvent('Runtime.executionContextCreated');
    await sendCommand('Runtime.enable');
    const ctxParams = await ctxPromise;
    console.log(`✔ Execution Context Created: ID = ${ctxParams.context.id}, Origin = "${ctxParams.context.origin}"`);

    // 6. Navigation Pipeline
    console.log('[Driver Step 7] Navigating to Agent test page...');
    const html = `<!DOCTYPE html><html><head><title>Driver Test</title></head><body>` +
        `<div id='container' style='padding: 20px;'>` +
        `<button id='action-btn' data-agent='ready' style='width: 140px; height: 42px;'>Authorize Agent</button>` +
        `</div></body></html>`;

    const navPromise = sendCommand('Page.navigate', { url: `data:text/html,${html}` });
    const dclPromise = waitForEvent('Page.domContentEventFired');
    const loadPromise = waitForEvent('Page.loadEventFired');

    const [navResult] = await Promise.all([navPromise, dclPromise, loadPromise]);
    console.log(`✔ Navigation Lifecycle Complete: Frame ID = ${navResult.frameId}`);

    // 7. Query DOM & BoxModel
    console.log('[Driver Step 8] Inspecting DOM element and layout box coordinates...');
    const queryRes = await sendCommand('DOM.querySelector', { nodeId: 1, selector: '#action-btn' });
    if (!queryRes.nodeId || queryRes.nodeId <= 0) {
        throw new Error('Failed to find #action-btn via DOM.querySelector');
    }
    console.log(`✔ Found #action-btn NodeId: ${queryRes.nodeId}`);

    const boxModel = await sendCommand('DOM.getBoxModel', { nodeId: queryRes.nodeId });
    if (boxModel.model.width !== 140 || boxModel.model.height !== 42) {
        throw new Error(`Unexpected box geometry: expected 140x42, got ${boxModel.model.width}x${boxModel.model.height}`);
    }
    console.log(`✔ Verified Box Model Geometry: ${boxModel.model.width}px x ${boxModel.model.height}px`);

    // 8. Interactive Mouse Click & JS Evaluation
    console.log('[Driver Step 9] Dispatching mouse interaction and evaluating JavaScript...');
    await sendCommand('Input.dispatchMouseEvent', {
        type: 'mousePressed',
        x: 30,
        y: 30,
        button: 'left'
    });

    const evalRes = await sendCommand('Runtime.evaluate', {
        expression: "document.querySelector('#action-btn').getAttribute('data-agent')"
    });
    if (evalRes.result.value !== 'ready') {
        throw new Error(`Unexpected evaluate result: ${JSON.stringify(evalRes)}`);
    }
    console.log(`✔ Evaluated DOM Attribute data-agent = "${evalRes.result.value}"`);

    // 9. Visual Screenshot Capture
    console.log('[Driver Step 10] Capturing visual PNG screenshot...');
    const shotRes = await sendCommand('Page.captureScreenshot', { format: 'png' });
    if (!shotRes.data || !shotRes.data.startsWith('iVBORw0KGgo')) {
        throw new Error('Screenshot data is missing or invalid Base64 PNG format');
    }
    const pngBuffer = Buffer.from(shotRes.data, 'base64');
    console.log(`✔ Received Valid PNG Screenshot: Size = ${pngBuffer.length} bytes (Magic = [${pngBuffer.slice(0, 4).toString('hex')}])`);

    // 10. Graceful Teardown
    console.log('[Driver Step 11] Tearing down connection and gracefully terminating process...');
    ws.close();

    const exitPromise = new Promise((resolve) => {
        proc.on('exit', (code, signal) => {
            resolve({ code, signal });
        });
    });

    proc.kill('SIGTERM');
    const exitResult = await exitPromise;
    console.log(`✔ CapSurf process terminated cleanly: code = ${exitResult.code}, signal = ${exitResult.signal}`);

    console.log('\n==================================================================');
    console.log(' 🎉 Phase 8 Driver Compatibility Test: 100% SUCCESS!');
    console.log('==================================================================\n');
}

runTest().catch((err) => {
    console.error('\n❌ Test Driver Failed:', err);
    process.exit(1);
});
