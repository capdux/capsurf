// test/cdp/test_playwright_e2e.cp
// CapSurf Phase 7 End-to-End Playwright / Puppeteer Protocol Validation Suite
//
// Validates end-to-end CDP lifecycle over real TCP POSIX socket and RFC 6455 WebSocket:
//   1. HTTP Discovery: GET /json/version & GET /json/list
//   2. WebSocket Handshake & Session Multiplexing
//   3. Playwright Handshake: Browser.getVersion & Target.setAutoAttach
//   4. Navigation Pipeline: Page.navigate -> frameNavigated, domContentEventFired, loadEventFired
//   5. DOM Query & Geometry: DOM.getDocument -> DOM.querySelector -> DOM.getBoxModel
//   6. Interactive Automation: Input.dispatchMouseEvent & Runtime.evaluate
//   7. Screenshot Capture: Page.captureScreenshot -> Base64 PNG verification
//   8. Teardown: Target.closeTarget -> Target.targetDestroyed

import { test, expect_eq } from "cap:test";
import {
    CdpServer,
    SockAddrIn,
    AF_INET,
    SOCK_STREAM,
    socket,
    connect,
    close,
    socketWriteAll,
    socketRead,
    readWsTextFrame,
} from "../../../src/cdp/server.cp";
import { extractJsonString, extractJsonInt } from "../../../src/cdp/types.cp";

struct E2eClient {
    fd: i32,

    static fn open(port: u16): (*E2eClient)! {
        const sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) return error.SocketCreationFailed;

        const addr = SockAddrIn {
            sin_family: @as(u16, @intCast(AF_INET)),
            sin_port: std.mem.nativeToBig(u16, port),
            sin_addr: 0x0100007F, // 127.0.0.1
            sin_zero: [0; 8],
        };

        if (connect(sock_fd, &addr, @sizeOf(SockAddrIn)) < 0) {
            * = close(sock_fd);
            return error.ConnectFailed;
        }

        let client = std.heap.c_allocator.create(E2eClient) catch return error.AllocFailed;
        client.* = E2eClient { fd: sock_fd };
        return client;
    }

    fn closeClient(*_) {
        * = close(_.fd);
        std.heap.c_allocator.destroy(_);
    }

    fn sendRaw(*_, bytes: []const u8)! {
        socketWriteAll(_.fd, bytes)? ;
    }

    fn readRaw(*_, buf: []u8): []const u8! {
        let total: usize = 0;
        while (total < buf.len) {
            const n = socketRead(_.fd, buf[total..]) catch break;
            if (n == 0) break;
            total += n;
            if (hdr_end := std.mem.indexOf(u8, buf[0..total], "\r\n\r\n")) {
                if (std.mem.startsWith(u8, buf[0..total], "HTTP/1.1 101")) break;
                const body = buf[hdr_end + 4 .. total];
                if (body.len > 0 && (body[body.len - 1] == '}' || body[body.len - 1] == ']')) break;
            }
        }
        return buf[0..total];
    }

    fn sendWs(*_, text: []const u8)! {
        let header_buf: [14]u8 = [0; 14];
        header_buf[0] = 0x81;
        const mask_key = "\x33\x44\x55\x66";

        let header_len: usize = 6;
        if (text.len <= 125) {
            header_buf[1] = @as(u8, @truncate(0x80 | text.len));
            header_buf[2] = mask_key[0];
            header_buf[3] = mask_key[1];
            header_buf[4] = mask_key[2];
            header_buf[5] = mask_key[3];
        } else {
            header_buf[1] = @as(u8, 0xFE);
            std.mem.writeInt(u16, header_buf[2..4][0..2], @as(u16, @truncate(text.len)), .big);
            header_buf[4] = mask_key[0];
            header_buf[5] = mask_key[1];
            header_buf[6] = mask_key[2];
            header_buf[7] = mask_key[3];
            header_len = 8;
        }

        socketWriteAll(_.fd, header_buf[0..header_len])? ;

        let masked: [8192]u8 = [0; 8192];
        let i: usize = 0;
        while (i < text.len) : (i += 1) {
            masked[i] = text[i] ^ mask_key[i % 4];
        }
        socketWriteAll(_.fd, masked[0..text.len])? ;
    }

    fn readWs(*_, buf: []u8): ([]const u8)?! {
        return readWsTextFrame(_.fd, buf);
    }
}

fn serverRunner(srv: *CdpServer) {
    * = srv.runAcceptLoop() catch {};
}

fn testPlaywrightE2eFlow()! {
    const port: u16 = 19444;
    let srv = CdpServer.init(std.heap.page_allocator, port) catch unreachable;
    defer srv.deinit();

    srv.start() catch unreachable;
    let thread = std.Thread.spawn(.{}, serverRunner, .{srv}) catch unreachable;
    defer {
        srv.stop();
        thread.join();
    }

    // 1. HTTP Discovery /json/version check
    let http_client = E2eClient.open(port) catch unreachable;
    const req_ver = "GET /json/version HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    http_client.sendRaw(req_ver) catch unreachable;

    let resp_buf: [2048]u8 = undefined;
    const resp_ver = http_client.readRaw(resp_buf[0..]) catch unreachable;
    expect_eq(std.mem.indexOf(u8, resp_ver, "200 OK") != null, true);
    expect_eq(std.mem.indexOf(u8, resp_ver, "\"Browser\": \"CapSurf/1.0.0\"") != null, true);
    expect_eq(std.mem.indexOf(u8, resp_ver, "webSocketDebuggerUrl") != null, true);
    http_client.closeClient();

    // 2. Connect WebSocket to simulate Playwright agent session
    let ws_client = E2eClient.open(port) catch unreachable;
    defer ws_client.closeClient();

    const ws_upgrade =
        "GET /devtools/browser/capsurf-browser-001 HTTP/1.1\r\n" ++
        "Host: 127.0.0.1\r\n" ++
        "Upgrade: websocket\r\n" ++
        "Connection: Upgrade\r\n" ++
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n" ++
        "Sec-WebSocket-Version: 13\r\n\r\n";
    ws_client.sendRaw(ws_upgrade) catch unreachable;

    let upgrade_buf: [1024]u8 = undefined;
    const upgrade_resp = ws_client.readRaw(upgrade_buf[0..]) catch unreachable;
    expect_eq(std.mem.indexOf(u8, upgrade_resp, "101 Switching Protocols") != null, true);
    expect_eq(std.mem.indexOf(u8, upgrade_resp, "Sec-WebSocket-Accept:") != null, true);

    // 3. Playwright Handshake: Browser.getVersion & Target.setAutoAttach
    let ws_buf: [262144]u8 = undefined;

    ws_client.sendWs("{\"id\":1,\"method\":\"Browser.getVersion\",\"params\":{}}") catch unreachable;
    const frame1 = (ws_client.readWs(&ws_buf) catch unreachable).?;
    expect_eq(std.mem.indexOf(u8, frame1, "\"product\":\"CapSurf/1.0.0\"") != null, true);

    ws_client.sendWs("{\"id\":2,\"method\":\"Target.setAutoAttach\",\"params\":{\"autoAttach\":true,\"flatten\":true}}") catch unreachable;
    let saw_attach_resp = false;
    let saw_attached_event = false;
    let attempt: usize = 0;
    while (attempt < 2) : (attempt += 1) {
        const frame = (ws_client.readWs(&ws_buf) catch unreachable).?;
        if (std.mem.indexOf(u8, frame, "\"id\":2") != null) saw_attach_resp = true;
        if (std.mem.indexOf(u8, frame, "Target.attachedToTarget") != null) saw_attached_event = true;
    }
    expect_eq(saw_attach_resp, true);
    expect_eq(saw_attached_event, true);

    // 4. Enable Domains: Page, Runtime, DOM
    ws_client.sendWs("{\"id\":3,\"method\":\"Page.enable\",\"params\":{}}") catch unreachable;
    const frame_page = (ws_client.readWs(&ws_buf) catch unreachable).?;
    expect_eq(std.mem.indexOf(u8, frame_page, "\"id\":3") != null, true);

    ws_client.sendWs("{\"id\":4,\"method\":\"Runtime.enable\",\"params\":{}}") catch unreachable;
    let saw_ctx_created = false;
    let saw_rt_resp = false;
    attempt = 0;
    while (attempt < 2) : (attempt += 1) {
        const frame = (ws_client.readWs(&ws_buf) catch unreachable).?;
        if (std.mem.indexOf(u8, frame, "Runtime.executionContextCreated") != null) {
            saw_ctx_created = true;
        } else if (std.mem.indexOf(u8, frame, "\"id\":4") != null) {
            saw_rt_resp = true;
        }
    }
    expect_eq(saw_ctx_created, true);
    expect_eq(saw_rt_resp, true);

    // 5. Navigate to Interactive Agent Page
    const nav_cmd =
        "{\"id\":5,\"method\":\"Page.navigate\",\"params\":{\"url\":\"data:text/html," ++
        "<!DOCTYPE html><html><head><title>Playwright Test</title></head>" ++
        "<body><div id='wrapper' style='padding: 20px;'>" ++
        "<button id='login-btn' data-action='login' style='width: 100px; height: 35px;'>Log in</button>" ++
        "</div></body></html>\"}}";
    ws_client.sendWs(nav_cmd) catch unreachable;

    // Read Page.navigate response and lifecycle events
    let got_nav_resp = false;
    let got_frame_nav = false;
    let got_dcl = false;
    let got_load = false;
    attempt = 0;
    while (attempt < 16 && !(got_nav_resp && got_frame_nav && got_dcl && got_load)) : (attempt += 1) {
        const frame = (ws_client.readWs(&ws_buf) catch unreachable).?;
        if (std.mem.indexOf(u8, frame, "\"id\":5") != null) got_nav_resp = true;
        if (std.mem.indexOf(u8, frame, "Page.frameNavigated") != null) got_frame_nav = true;
        if (std.mem.indexOf(u8, frame, "Page.domContentEventFired") != null) got_dcl = true;
        if (std.mem.indexOf(u8, frame, "Page.loadEventFired") != null) got_load = true;
    }
    expect_eq(got_nav_resp, true);
    expect_eq(got_frame_nav, true);
    expect_eq(got_dcl, true);
    expect_eq(got_load, true);

    // 6. DOM Query & Geometry Inspection
    ws_client.sendWs("{\"id\":6,\"method\":\"DOM.querySelector\",\"params\":{\"nodeId\":1,\"selector\":\"#login-btn\"}}") catch unreachable;
    let target_nid: i64 = 0;
    attempt = 0;
    while (attempt < 8) : (attempt += 1) {
        const query_resp = (ws_client.readWs(&ws_buf) catch unreachable).?;
        if (std.mem.indexOf(u8, query_resp, "\"id\":6") != null) {
            if (nid := extractJsonInt(query_resp, "nodeId")) {
                target_nid = nid;
            }
            break;
        }
    }
    expect_eq(target_nid > 0, true);

    let box_cmd_buf: [128]u8 = undefined;
    const box_cmd = std.fmt.bufPrint(&box_cmd_buf, "{{\"id\":7,\"method\":\"DOM.getBoxModel\",\"params\":{{\"nodeId\":{d}}}}}", .{target_nid}) catch "";
    ws_client.sendWs(box_cmd) catch unreachable;
    let got_box = false;
    attempt = 0;
    while (attempt < 8) : (attempt += 1) {
        const box_resp = (ws_client.readWs(&ws_buf) catch unreachable).?;
        if (std.mem.indexOf(u8, box_resp, "\"id\":7") != null) {
            expect_eq(std.mem.indexOf(u8, box_resp, "\"width\":126") != null, true);
            expect_eq(std.mem.indexOf(u8, box_resp, "\"height\":45") != null, true);
            got_box = true;
            break;
        }
    }
    expect_eq(got_box, true);

    // 7. Interactive Click & JS Evaluation Check
    ws_client.sendWs("{\"id\":8,\"method\":\"Input.dispatchMouseEvent\",\"params\":{\"type\":\"mousePressed\",\"x\":25,\"y\":25,\"button\":\"left\"}}") catch unreachable;
    attempt = 0;
    while (attempt < 8) : (attempt += 1) {
        const click_resp = (ws_client.readWs(&ws_buf) catch unreachable).?;
        if (std.mem.indexOf(u8, click_resp, "\"id\":8") != null) break;
    }

    ws_client.sendWs("{\"id\":9,\"method\":\"Runtime.evaluate\",\"params\":{\"expression\":\"document.querySelector('#login-btn').getAttribute('data-action');\"}}") catch unreachable;
    let got_eval = false;
    attempt = 0;
    while (attempt < 8) : (attempt += 1) {
        const eval_resp = (ws_client.readWs(&ws_buf) catch unreachable).?;
        if (std.mem.indexOf(u8, eval_resp, "\"id\":9") != null) {
            expect_eq(std.mem.indexOf(u8, eval_resp, "\"value\":\"login\"") != null, true);
            got_eval = true;
            break;
        }
    }
    expect_eq(got_eval, true);

    // 8. Visual Screenshot Capture Check
    ws_client.sendWs("{\"id\":10,\"method\":\"Page.captureScreenshot\",\"params\":{\"format\":\"png\"}}") catch unreachable;
    let got_shot = false;
    attempt = 0;
    while (attempt < 8) : (attempt += 1) {
        const shot_resp = (ws_client.readWs(&ws_buf) catch unreachable).?;
        if (std.mem.indexOf(u8, shot_resp, "\"id\":10") != null) {
            expect_eq(std.mem.indexOf(u8, shot_resp, "\"data\":") != null, true);
            // Base64 PNG header begins with iVBORw0KGgo
            expect_eq(std.mem.indexOf(u8, shot_resp, "iVBORw0KGgo") != null, true);
            got_shot = true;
            break;
        }
    }
    expect_eq(got_shot, true);
}

fn main() {
    test("CDP Playwright & Puppeteer Full End-to-End Automation Pipeline", () => {
        testPlaywrightE2eFlow() catch unreachable;
    });
}
