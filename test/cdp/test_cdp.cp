// tests/test_cdp.cp
// CapSurf Milestone 4 Comprehensive Test Suite:
// CDP Protocol Server, HTTP Discovery, RFC 6455 WebSocket Handshake & Masking,
// JSON-RPC Domains (Browser, Page, DOM, Runtime), and BoxModel Quad Geometry Extraction.

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
    socketReadAll,
    writeWsTextFrame,
    readWsTextFrame
} from "../../src/cdp/server.cp";
import { CdpBoxModel, CdpRequest, extractJsonString, extractJsonInt } from "../../src/cdp/types.cp";
import { NodeId, NULL_NODE } from "../../src/dom/dom.cp";

struct CdpTestClient {
    fd: i32,

    static fn openClient(port: u16): (*CdpTestClient)! {
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

        let client = std.heap.c_allocator.create(CdpTestClient) catch return error.AllocFailed;
        client.* = CdpTestClient { fd: sock_fd };
        return client;
    }

    fn disconnect(*_) {
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

    fn sendClientWsFrame(*_, text: []const u8)! {
        let header_buf: [14]u8 = [0; 14];
        header_buf[0] = 0x81; // FIN + Text opcode
        const mask_key = "\x11\x22\x33\x44";

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

    fn readClientWsFrame(*_, buf: []u8): ([]const u8)?! {
        return readWsTextFrame(_.fd, buf);
    }
}

fn serverWorker(srv: *CdpServer) {
    * = srv.runAcceptLoop() catch {};
}

fn runHttpDiscoveryTest()! {
    let srv = CdpServer.init(std.heap.page_allocator, 19222) catch unreachable;
    defer srv.deinit();

    srv.start() catch unreachable;
    const port = srv.getPort();

    const t = std.Thread.spawn(.{}, serverWorker, .{srv}) catch unreachable;
    defer {
        srv.stop();
        t.join();
    }

    // 1. GET /json/version
    let client1 = CdpTestClient.openClient(port) catch unreachable;
    defer client1.disconnect();

    const req_ver = "GET /json/version HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    client1.sendRaw(req_ver) catch unreachable;
    let buf1: [2048]u8 = [0; 2048];
    const res1 = client1.readRaw(&buf1) catch unreachable;

    expect_eq(std.mem.indexOf(u8, res1, "200 OK") != null, true);
    expect_eq(std.mem.indexOf(u8, res1, "CapSurf/1.0.0") != null, true);
    expect_eq(std.mem.indexOf(u8, res1, "Protocol-Version") != null, true);

    // 2. GET /json/list
    let client2 = CdpTestClient.openClient(port) catch unreachable;
    defer client2.disconnect();

    const req_list = "GET /json/list HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    client2.sendRaw(req_list) catch unreachable;
    let buf2: [2048]u8 = [0; 2048];
    const res2 = client2.readRaw(&buf2) catch unreachable;

    expect_eq(std.mem.indexOf(u8, res2, "200 OK") != null, true);
    expect_eq(std.mem.indexOf(u8, res2, "\"id\": \"kitesurf\"") != null, true);
    expect_eq(std.mem.indexOf(u8, res2, "\"type\": \"page\"") != null, true);

    // 3. GET /json/new
    let client3 = CdpTestClient.openClient(port) catch unreachable;
    defer client3.disconnect();

    const req_new = "GET /json/new HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    client3.sendRaw(req_new) catch unreachable;
    let buf3: [2048]u8 = [0; 2048];
    const res3 = client3.readRaw(&buf3) catch unreachable;

    expect_eq(std.mem.indexOf(u8, res3, "200 OK") != null, true);
    expect_eq(std.mem.indexOf(u8, res3, "capsurf-target-001") != null, true);
}

fn runWebSocketCdpTest()! {
    let srv = CdpServer.init(std.heap.page_allocator, 19223) catch unreachable;
    defer srv.deinit();

    srv.start() catch unreachable;
    const port = srv.getPort();

    const t = std.Thread.spawn(.{}, serverWorker, .{srv}) catch unreachable;
    defer {
        srv.stop();
        t.join();
    }

    let client = CdpTestClient.openClient(port) catch unreachable;
    defer client.disconnect();

    // 1. WebSocket Upgrade Handshake
    const ws_upgrade =
        "GET /devtools/page/capsurf-target-001 HTTP/1.1\r\n" ++
        "Host: 127.0.0.1\r\n" ++
        "Upgrade: websocket\r\n" ++
        "Connection: Upgrade\r\n" ++
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n" ++
        "Sec-WebSocket-Version: 13\r\n\r\n";

    client.sendRaw(ws_upgrade) catch unreachable;
    let hs_buf: [1024]u8 = [0; 1024];
    const hs_res = client.readRaw(&hs_buf) catch unreachable;
    std.debug.print("\n[DEBUG] Handshake response: {s}\n", .{hs_res});

    expect_eq(std.mem.indexOf(u8, hs_res, "101 Switching Protocols") != null, true);
    expect_eq(std.mem.indexOf(u8, hs_res, "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") != null, true);

    // 2. CDP Command: Browser.getVersion
    const cmd_version = "{\"id\":101,\"method\":\"Browser.getVersion\"}";
    client.sendClientWsFrame(cmd_version) catch unreachable;

    let ws_buf: [4096]u8 = [0; 4096];
    const frame_opt1 = client.readClientWsFrame(&ws_buf) catch unreachable;
    std.debug.print("[DEBUG] Frame 1: {s}\n", .{frame_opt1 ?? "NULL"});
    expect_eq(frame_opt1 != null, true);
    if (f1 := frame_opt1) {
        expect_eq(std.mem.indexOf(u8, f1, "\"id\":101") != null, true);
        expect_eq(std.mem.indexOf(u8, f1, "CapSurf/1.0.0") != null, true);
        expect_eq(std.mem.indexOf(u8, f1, "QuickJS-NG") != null, true);
    }

    // 3. CDP Command: Page.navigate
    const cmd_nav =
        "{\"id\":102,\"method\":\"Page.navigate\",\"params\":{\"url\":\"data:text/html," ++
        "<div id='card' class='box' style='width: 400px; height: 200px; padding: 20px; margin: 30px;'>" ++
        "<button id='action-btn' style='width: 120px; height: 40px;'>Click Me</button>" ++
        "</div>\"}}";
    client.sendClientWsFrame(cmd_nav) catch unreachable;

    // Read Page.navigate response + lifecycle events until all expected ones arrive
    let got_nav_resp = false;
    let got_frame_nav = false;
    let got_dcl = false;
    let got_load = false;
    let nav_attempt: usize = 0;
    while (nav_attempt < 16 && !(got_nav_resp && got_frame_nav && got_dcl && got_load)) : (nav_attempt += 1) {
        const frame_opt = client.readClientWsFrame(&ws_buf) catch unreachable;
        expect_eq(frame_opt != null, true);
        if (fr := frame_opt) {
            if (std.mem.indexOf(u8, fr, "\"id\":102") != null) {
                expect_eq(std.mem.indexOf(u8, fr, "frameId") != null, true);
                got_nav_resp = true;
            }
            if (std.mem.indexOf(u8, fr, "Page.frameNavigated") != null) got_frame_nav = true;
            if (std.mem.indexOf(u8, fr, "Page.domContentEventFired") != null) got_dcl = true;
            if (std.mem.indexOf(u8, fr, "Page.loadEventFired") != null) got_load = true;
        }
    }
    expect_eq(got_nav_resp, true);
    expect_eq(got_frame_nav, true);
    expect_eq(got_dcl, true);
    expect_eq(got_load, true);

    // 4. CDP Command: DOM.querySelector
    const cmd_qs = "{\"id\":103,\"method\":\"DOM.querySelector\",\"params\":{\"selector\":\"#card\"}}";
    client.sendClientWsFrame(cmd_qs) catch unreachable;

    let card_node_id: i64 = 0;
    let qs_attempt: usize = 0;
    while (qs_attempt < 8) : (qs_attempt += 1) {
        const frame_opt3 = client.readClientWsFrame(&ws_buf) catch unreachable;
        expect_eq(frame_opt3 != null, true);
        if (f3 := frame_opt3) {
            if (std.mem.indexOf(u8, f3, "\"id\":103") != null) {
                if (nid := extractJsonInt(f3, "nodeId")) {
                    card_node_id = nid;
                }
                break;
            }
        }
    }
    expect_eq(card_node_id > 0, true);

    // 5. CDP Command: DOM.getBoxModel (Validates Screen Pixel Coordinates & Quads)
    let cmd_bm_buf: [256]u8 = [0; 256];
    const cmd_bm = std.fmt.bufPrint(&cmd_bm_buf, "{{\"id\":104,\"method\":\"DOM.getBoxModel\",\"params\":{{\"nodeId\":{d}}}}}", .{card_node_id}) catch "";
    client.sendClientWsFrame(cmd_bm) catch unreachable;

    let got_bm = false;
    let bm_attempt: usize = 0;
    while (bm_attempt < 8) : (bm_attempt += 1) {
        const frame_opt4 = client.readClientWsFrame(&ws_buf) catch unreachable;
        expect_eq(frame_opt4 != null, true);
        if (f4 := frame_opt4) {
            if (std.mem.indexOf(u8, f4, "\"id\":104") != null) {
                expect_eq(std.mem.indexOf(u8, f4, "\"width\":400") != null, true);
                expect_eq(std.mem.indexOf(u8, f4, "\"height\":200") != null, true);
                expect_eq(std.mem.indexOf(u8, f4, "\"border\":[") != null, true);
                got_bm = true;
                break;
            }
        }
    }
    expect_eq(got_bm, true);

    // 6. CDP Command: Runtime.evaluate (JavaScript expression evaluation)
    const cmd_eval = "{\"id\":105,\"method\":\"Runtime.evaluate\",\"params\":{\"expression\":\"'Cap' + 'Surf' + ' ' + (2000 + 26)\"}}";
    client.sendClientWsFrame(cmd_eval) catch unreachable;

    let got_eval = false;
    let eval_attempt: usize = 0;
    while (eval_attempt < 8) : (eval_attempt += 1) {
        const frame_opt5 = client.readClientWsFrame(&ws_buf) catch unreachable;
        expect_eq(frame_opt5 != null, true);
        if (f5 := frame_opt5) {
            if (std.mem.indexOf(u8, f5, "\"id\":105") != null) {
                expect_eq(std.mem.indexOf(u8, f5, "CapSurf 2026") != null, true);
                got_eval = true;
                break;
            }
        }
    }
    expect_eq(got_eval, true);
}

fn main() {
    test("1. CDP HTTP Discovery Endpoints (/json/version, /json/list, /json/new)", () => {
        runHttpDiscoveryTest()? ;
    });

    test("2. CDP WebSocket Protocol, Handshake, JSON-RPC Domains, DOM & getBoxModel", () => {
        runWebSocketCdpTest()? ;
    });
}
