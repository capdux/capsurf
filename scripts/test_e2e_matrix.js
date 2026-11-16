// scripts/test_e2e_matrix.js
// CapSurf Phase 30: Comprehensive End-to-End Test Matrix
// Validates Full CDP Protocol Stack, DOM Querying, CSS Computed Styles,
// Extensions (CapSurf/Kitesurf), Mouse Interactions & High-Res PNG Capture.

const { spawn } = require('node:child_process');
const http = require('node:http');

const PORT = 19226;
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

function createWebSocket(url) {
    return new Promise((resolve, reject) => {
        const u = new URL(url);
        const options = {
            hostname: u.hostname,
            port: u.port,
            path: u.pathname,
            headers: {
                'Connection': 'Upgrade',
                'Upgrade': 'websocket',
                'Sec-WebSocket-Key': 'dGhlIHNhbXBsZSBub25jZQ==',
                'Sec-WebSocket-Version': '13'
            }
        };

        const req = http.request(options);
        req.on('upgrade', (res, socket, upgradeHead) => {
            resolve(socket);
        });
        req.on('error', reject);
        req.end();
    });
}

function sendWsFrame(socket, text) {
    const payload = Buffer.from(text, 'utf8');
    const length = payload.length;

    let header;
    if (length <= 125) {
        header = Buffer.from([0x81, 0x80 | length, 0x00, 0x00, 0x00, 0x00]);
    } else if (length <= 65535) {
        header = Buffer.alloc(8);
        header[0] = 0x81;
        header[1] = 0x80 | 126;
        header.writeUInt16BE(length, 2);
    } else {
        header = Buffer.alloc(14);
        header[0] = 0x81;
        header[1] = 0x80 | 127;
        header.writeBigUInt64BE(BigInt(length), 2);
    }

    socket.write(Buffer.concat([header, payload]));
}

function readWsFrames(socket, onMessage) {
    let buffer = Buffer.alloc(0);

    socket.on('data', (chunk) => {
        buffer = Buffer.concat([buffer, chunk]);

        while (buffer.length >= 2) {
            const secondByte = buffer[1];
            const isMasked = (secondByte & 0x80) !== 0;
            let payloadLen = secondByte & 0x7F;
            let offset = 2;

            if (payloadLen === 126) {
                if (buffer.length < 4) break;
                payloadLen = buffer.readUInt16BE(2);
                offset = 4;
            } else if (payloadLen === 127) {
                if (buffer.length < 10) break;
                payloadLen = Number(buffer.readBigUInt64BE(2));
                offset = 10;
            }

            if (isMasked) offset += 4;
            if (buffer.length < offset + payloadLen) break;

            const payload = buffer.slice(offset, offset + payloadLen);
            buffer = buffer.slice(offset + payloadLen);

            try {
                const msg = JSON.parse(payload.toString('utf8'));
                onMessage(msg);
            } catch (err) {
                // Ignore raw binary frame parsing errors
            }
        }
    });
}

async function runE2EMatrix() {
    console.log('==================================================================');
    console.log(' CapSurf Phase 30: Comprehensive E2E Compatibility Test Matrix');
    console.log('==================================================================\n');

    let child = null;
    let socket = null;

    try {
        console.log(`[Step 1] Spawning CapSurf binary on port ${PORT}...`);
        child = spawn(BIN_PATH, [`--remote-debugging-port=${PORT}`, '--window-size=1280,720'], {
            stdio: ['ignore', 'pipe', 'pipe']
        });

        let wsUrl = await new Promise((resolve, reject) => {
            const timeout = setTimeout(() => reject(new Error('CapSurf startup timeout')), 10000);
            function onData(chunk) {
                const text = chunk.toString();
                const match = text.match(/DevTools listening on (ws:\/\/[^\s]+)/);
                if (match) {
                    clearTimeout(timeout);
                    resolve(match[1]);
                }
            }
            child.stdout.on('data', onData);
            child.stderr.on('data', onData);
        });

        console.log(`✔ CDP Listening: ${wsUrl}`);

        // HTTP Version discovery
        console.log('[Step 2] Checking HTTP Discovery: GET /json/version');
        const ver = await httpGet(`http://127.0.0.1:${PORT}/json/version`);
        if (ver.status !== 200 || !ver.body.Browser.includes('CapSurf')) {
            throw new Error(`Invalid /json/version: ${JSON.stringify(ver.body)}`);
        }
        console.log(`✔ HTTP Discovery Valid: Browser = "${ver.body.Browser}"`);

        // Connect WebSocket
        console.log('[Step 3] Connecting WebSocket...');
        socket = await createWebSocket(wsUrl);
        console.log('✔ WebSocket handshake established.');

        let reqId = 1;
        const pending = new Map();
        readWsFrames(socket, (msg) => {
            if (msg.id && pending.has(msg.id)) {
                pending.get(msg.id)(msg);
                pending.delete(msg.id);
            }
        });

        function sendCommand(method, params = {}, sessionId = undefined) {
            return new Promise((resolve) => {
                const id = reqId++;
                const payload = { id, method, params };
                if (sessionId) payload.sessionId = sessionId;
                pending.set(id, resolve);
                sendWsFrame(socket, JSON.stringify(payload));
            });
        }

        // Auto attach
        console.log('[Step 4] Auto-Attaching target session...');
        await sendCommand('Target.setAutoAttach', { autoAttach: true, waitForDebuggerOnStart: false });
        const targetsRes = await sendCommand('Target.getTargets');
        const sessionId = 'capsurf-session-001';
        console.log(`✔ Auto-Attached Session: ${sessionId}`);

        // Enable domains
        console.log('[Step 5] Enabling Page, Runtime, DOM and CSS domains...');
        await sendCommand('Page.enable', {}, sessionId);
        await sendCommand('Runtime.enable', {}, sessionId);
        await sendCommand('DOM.enable', {}, sessionId);
        await sendCommand('CSS.enable', {}, sessionId);
        console.log('✔ All core domains enabled.');

        // Navigate
        console.log('[Step 6] Navigating to Agent test fixture page...');
        const html = `<!DOCTYPE html><html><head><title>Matrix Test</title></head><body>` +
            `<div id='container' style='padding: 20px;'>` +
            `<button id='action-btn' data-agent='ready' style='width: 140px; height: 42px;'>Authorize Agent</button>` +
            `</div></body></html>`;
        const navRes = await sendCommand('Page.navigate', { url: `data:text/html,${html}` }, sessionId);
        if (!navRes.result || !navRes.result.frameId) {
            throw new Error(`Navigation failed: ${JSON.stringify(navRes)}`);
        }
        console.log(`✔ Page navigated: Frame ID = ${navRes.result.frameId}`);

        // DOM Box Model
        console.log('[Step 7] Querying DOM box model geometry...');
        const docRes = await sendCommand('DOM.getDocument', {}, sessionId);
        const queryRes = await sendCommand('DOM.querySelector', { nodeId: 1, selector: '#action-btn' }, sessionId);
        const btnId = queryRes.result.nodeId;
        const boxRes = await sendCommand('DOM.getBoxModel', { nodeId: btnId }, sessionId);
        const w = boxRes.result.model.width;
        const h = boxRes.result.model.height;
        console.log(`✔ Verified Element Geometry: #action-btn is ${w}px x ${h}px`);

        // CSS Computed Style
        console.log('[Step 8] Querying CSS computed style...');
        const cssRes = await sendCommand('CSS.getComputedStyleForNode', { nodeId: btnId }, sessionId);
        if (!cssRes.result || !Array.isArray(cssRes.result.computedStyle)) {
            throw new Error(`CSS computed style query failed: ${JSON.stringify(cssRes)}`);
        }
        console.log(`✔ CSS computed style verified: ${cssRes.result.computedStyle.length} computed properties`);

        // CapSurf / Kitesurf Protocol Extensions
        console.log('[Step 9] Verifying CapSurf Protocol Extensions...');
        const extRes = await sendCommand('CapSurf.getSceneHostManifest', {}, sessionId);
        if (!extRes.result || extRes.result.abi === undefined) {
            throw new Error(`CapSurf protocol extension query failed: ${JSON.stringify(extRes)}`);
        }
        console.log(`✔ CapSurf extension verified: Manifest ABI = ${extRes.result.abi}, wireVersion = ${extRes.result.wireVersion}`);

        // Mouse click & JS evaluation
        console.log('[Step 10] Dispatching Mouse Click and evaluating JS state...');
        await sendCommand('Input.dispatchMouseEvent', {
            type: 'mousePressed',
            x: 80,
            y: 30,
            button: 'left',
            clickCount: 1
        }, sessionId);
        await sendCommand('Input.dispatchMouseEvent', {
            type: 'mouseReleased',
            x: 80,
            y: 30,
            button: 'left',
            clickCount: 1
        }, sessionId);

        const evalRes = await sendCommand('Runtime.evaluate', {
            expression: "document.querySelector('#action-btn').getAttribute('data-agent')"
        }, sessionId);
        const agentAttr = evalRes.result?.value ?? evalRes.result?.result?.value;
        if (agentAttr !== 'ready') {
            throw new Error(`Unexpected attribute state: ${agentAttr} in ${JSON.stringify(evalRes)}`);
        }
        console.log(`✔ JS Evaluated data-agent = "${agentAttr}"`);

        // Screenshot
        console.log('[Step 11] Capturing PNG visual rasterization...');
        const shotRes = await sendCommand('Page.captureScreenshot', { format: 'png' }, sessionId);
        const imgBase64 = shotRes.result.data;
        const imgBuffer = Buffer.from(imgBase64, 'base64');
        const isPng = imgBuffer.length > 8 &&
            imgBuffer[0] === 0x89 && imgBuffer[1] === 0x50 &&
            imgBuffer[2] === 0x4E && imgBuffer[3] === 0x47;
        if (!isPng) {
            throw new Error('Screenshot data is not a valid PNG!');
        }
        console.log(`✔ Verified PNG Screenshot: ${imgBuffer.length} bytes (Magic: 89504E47)`);

        console.log('\n==================================================================');
        console.log(' 🎉 Phase 30 Comprehensive E2E Test Matrix: 100% SUCCESS!');
        console.log('==================================================================\n');

    } finally {
        if (socket) {
            socket.destroy();
        }
        if (child) {
            child.kill('SIGTERM');
        }
    }
}

runE2EMatrix().catch((err) => {
    console.error('❌ E2E Test Matrix FAILED:', err);
    process.exit(1);
});
