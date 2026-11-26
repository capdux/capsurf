// test/cdp/test_page_navigation.cp
// CapSurf Phase 2 Comprehensive Test Suite:
// Real HTTP Loading, Data URL Decoding (Plain & Base64), Full Navigation Lifecycle Events,
// Cookie Synchronization, and Fetch Interception during Page.navigate.

import { test, expect_eq } from "cap:test";
import { Base64 } from "cap:encoding";
import { CdpPageSession } from "../../../src/cdp/session.cp";
import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt } from "../../../src/cdp/types.cp";
import { CdpDispatcher } from "../../../src/cdp/dispatch.cp";
import { InterceptVerdict } from "../../../src/cdp/fetch.cp";
import { HttpLoader } from "../../../src/net/http_loader.cp";
import { Matcher } from "../../../src/css/matcher.cp";
import { NULL_NODE } from "../../../src/dom/dom.cp";
import {
    SockAddrIn,
    AF_INET,
    SOCK_STREAM,
    SOL_SOCKET,
    SO_REUSEADDR,
    socket,
    bind,
    listen,
    accept,
    setsockopt,
    socketWriteAll,
    socketRead
} from "../../../src/cdp/server.cp";

fn testDataUrlNavigation()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [128]CdpEvent = [CdpEvent{}; 128];
    let evt_cnt: usize = 0;

    // 1. Plaintext data: URL navigation
    const html_data = "data:text/html,<div id='hero' style='width: 300px; height: 100px;'><button id='btn-start'>Get Started</button></div>";
    const req1 = CdpRequest{
        id: 1,
        method: "Page.navigate",
        params_raw: "{\"url\":\"" ++ html_data ++ "\"}",
    };
    const resp1 = CdpDispatcher.dispatch(session, req1, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp1.isOk(), true);
    expect_eq(evt_cnt >= 3, true);
    let saw_nav = false;
    let saw_dcl = false;
    let saw_load = false;
    let ei: usize = 0;
    while (ei < evt_cnt) : (ei += 1) {
        if (std.mem.eql(u8, out_evts[ei].method, "Page.frameNavigated")) saw_nav = true;
        if (std.mem.eql(u8, out_evts[ei].method, "Page.domContentEventFired")) saw_dcl = true;
        if (std.mem.eql(u8, out_evts[ei].method, "Page.loadEventFired")) saw_load = true;
    }
    expect_eq(saw_nav, true);
    expect_eq(saw_dcl, true);
    expect_eq(saw_load, true);

    // Verify DOM and Layout after navigation
    const hero_id = Matcher.querySelector(&session.doc, "#hero");
    expect_eq(hero_id != NULL_NODE, true);
    const hero_rect = session.layout.getBoundingClientRect(hero_id);
    expect_eq(hero_rect.width, 300.0);
    expect_eq(hero_rect.height, 100.0);
    expect_eq(hero_rect.visible, true);

    // 2. Base64 data: URL navigation
    // Raw: "<h1 id='headline'>Native CP Engine</h1>"
    // Base64: "PGgxIGlkPSdoZWFkbGluZSc+TmF0aXZlIENQIEVuZ2luZTwvaDE+"
    const b64_url = "data:text/html;base64,PGgxIGlkPSdoZWFkbGluZSc+TmF0aXZlIENQIEVuZ2luZTwvaDE+";
    const req2 = CdpRequest{
        id: 2,
        method: "Page.navigate",
        params_raw: "{\"url\":\"" ++ b64_url ++ "\"}",
    };
    const resp2 = CdpDispatcher.dispatch(session, req2, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp2.isOk(), true);
    expect_eq(evt_cnt >= 3, true);

    const h1_id = Matcher.querySelector(&session.doc, "#headline");
    expect_eq(h1_id != NULL_NODE, true);
    const h1_text = session.doc.getTextContent(session.doc.getFirstChild(h1_id));
    expect_eq(std.mem.eql(u8, h1_text, "Native CP Engine"), true);
}

fn testLayoutMetricsIntegration()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [128]CdpEvent = [CdpEvent{}; 128];
    let evt_cnt: usize = 0;

    // Override viewport metrics via Emulation
    const req_emul = CdpRequest{
        id: 10,
        method: "Emulation.setDeviceMetricsOverride",
        params_raw: "{\"width\":800,\"height\":600,\"deviceScaleFactor\":2,\"mobile\":false}",
    };
    const resp_emul = CdpDispatcher.dispatch(session, req_emul, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_emul.isOk(), true);

    // Query Page.getLayoutMetrics and verify match
    const req_metrics = CdpRequest{
        id: 11,
        method: "Page.getLayoutMetrics",
        params_raw: "{}",
    };
    const resp_metrics = CdpDispatcher.dispatch(session, req_metrics, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_metrics.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp_metrics.result_json, "\"clientWidth\":800") != null, true);
    expect_eq(std.mem.indexOf(u8, resp_metrics.result_json, "\"clientHeight\":600") != null, true);
    expect_eq(std.mem.indexOf(u8, resp_metrics.result_json, "\"scale\":1") != null, true);

    // Query Page.getFrameTree
    const req_tree = CdpRequest{
        id: 12,
        method: "Page.getFrameTree",
        params_raw: "{}",
    };
    const resp_tree = CdpDispatcher.dispatch(session, req_tree, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_tree.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp_tree.result_json, "\"frameTree\":") != null, true);
    expect_eq(std.mem.indexOf(u8, resp_tree.result_json, "\"capsurf-frame-001\"") != null, true);

    // Query Page.getFrameResourceTree
    const req_res_tree = CdpRequest{
        id: 13,
        method: "Page.getFrameResourceTree",
        params_raw: "{}",
    };
    const resp_res_tree = CdpDispatcher.dispatch(session, req_res_tree, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_res_tree.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp_res_tree.result_json, "\"resources\":[]") != null, true);
}

fn testInterceptedNavigation()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [128]CdpEvent = [CdpEvent{}; 128];
    let evt_cnt: usize = 0;

    // 1. Enable Fetch interception for pattern *mocked*
    const req_enable = CdpRequest{
        id: 20,
        method: "Fetch.enable",
        params_raw: "{\"patterns\":[{\"urlPattern\":\"*mocked*\",\"resourceType\":\"Document\"}]}",
    };
    const resp_enable = CdpDispatcher.dispatch(session, req_enable, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_enable.isOk(), true);

    // 2. Pre-park a fulfill rule for the navigation
    let parked = session.fetch_state.parkRequest("nav-req-001", "http://example.com/mocked/page", "GET");
    expect_eq(parked != null, true);
    parked.?.verdict = InterceptVerdict.Fulfill;
    parked.?.mock_status = 200;
    parked.?.setMockBody("<html><body><h3 id='mock-target'>Mocked Response Success</h3></body></html>");

    // 3. Navigate to intercepted URL
    const req_nav = CdpRequest{
        id: 21,
        method: "Page.navigate",
        params_raw: "{\"url\":\"http://example.com/mocked/page\"}",
    };
    const resp_nav = CdpDispatcher.dispatch(session, req_nav, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_nav.isOk(), true);

    // 4. Verify mocked content was parsed and placed in DOM
    const target_id = Matcher.querySelector(&session.doc, "#mock-target");
    expect_eq(target_id != NULL_NODE, true);
    const target_text = session.doc.getTextContent(session.doc.getFirstChild(target_id));
    expect_eq(std.mem.eql(u8, target_text, "Mocked Response Success"), true);
}

fn localHttpServerWorker(server_fd: i32) {
    let client_sock = accept(server_fd, null, null);
    if (client_sock >= 0) {
        defer * = std.c.close(client_sock);

        let req_buf: [2048]u8 = undefined;
        const r_len = socketRead(client_sock, req_buf[0..]) catch 0;
        if (r_len > 0) {
            // Reply with HTTP 200, Set-Cookie, and HTML body
            const resp =
                "HTTP/1.1 200 OK\r\n" ++
                "Content-Type: text/html\r\n" ++
                "Set-Cookie: srv_token=srv_999; Domain=127.0.0.1; Path=/\r\n" ++
                "Connection: close\r\n" ++
                "\r\n" ++
                "<!DOCTYPE html><html><body><h1 id='network-h1'>Loaded Over Real HTTP</h1></body></html>";
            socketWriteAll(client_sock, resp) catch {};
        }
    }
}

fn testRealHttpLoadingWithServer()! {
    // 1. Start a local HTTP test server on port 9876
    const server_port: u16 = 9876;
    const server_fd = socket(AF_INET, SOCK_STREAM, 0);
    expect_eq(server_fd >= 0, true);
    defer * = std.c.close(server_fd);

    const opt: i32 = 1;
    * = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, 4);

    const srv_addr = SockAddrIn{
        sin_family: @as(u16, @intCast(AF_INET)),
        sin_port: std.mem.nativeToBig(u16, server_port),
        sin_addr: 0x0100007F, // 127.0.0.1
        sin_zero: [0; 8],
    };

    const b_res = bind(server_fd, &srv_addr, @sizeOf(SockAddrIn));
    expect_eq(b_res >= 0, true);
    const l_res = listen(server_fd, 4);
    expect_eq(l_res >= 0, true);

    // 2. Perform client navigation via Thread
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    // Set an initial cookie to test client sending cookie
    const HTTPS_URL = "http://127.0.0.1:9876/";
    * = session.cookie_jar.setFromHeader(HTTPS_URL, "client_token=cli_777; Domain=127.0.0.1; Path=/", null);

    // Set custom User-Agent
    session.user_agent = "CapSurf-Agent/2026";

    // 3. Client initiates fetch via HttpLoader
    const thread = std.Thread.spawn(.{}, localHttpServerWorker, .{server_fd}) catch unreachable;

    // 4. Navigate via CDP Page.navigate
    let out_buf: [4096]u8 = undefined;
    let out_evts: [128]CdpEvent = [CdpEvent{}; 128];
    let evt_cnt: usize = 0;

    const req_nav = CdpRequest{
        id: 30,
        method: "Page.navigate",
        params_raw: "{\"url\":\"http://127.0.0.1:9876/index.html\"}",
    };
    const resp_nav = CdpDispatcher.dispatch(session, req_nav, out_buf[0..], &out_evts, &evt_cnt);
    thread.join();

    expect_eq(resp_nav.isOk(), true);
    expect_eq(evt_cnt >= 3, true);

    // 5. Verify DOM has been updated with real HTTP content
    const h1_id = Matcher.querySelector(&session.doc, "#network-h1");
    expect_eq(h1_id != NULL_NODE, true);
    const h1_text = session.doc.getTextContent(session.doc.getFirstChild(h1_id));
    expect_eq(std.mem.eql(u8, h1_text, "Loaded Over Real HTTP"), true);

    // 6. Verify Set-Cookie sent by server was stored into session.cookie_jar
    const cookies = session.cookie_jar.getAll();
    let found_srv_cookie = false;
    for (ck of cookies) {
        if (std.mem.eql(u8, ck.name, "srv_token") && std.mem.eql(u8, ck.value, "srv_999")) {
            found_srv_cookie = true;
            break;
        }
    }
    expect_eq(found_srv_cookie, true);
}

fn main() {
    test("1. Page Navigation via Data URLs (Plain & Base64 HTML)", () => {
        testDataUrlNavigation() catch unreachable;
    });

    test("2. Page Layout Metrics Integration (Emulation Sync)", () => {
        testLayoutMetricsIntegration() catch unreachable;
    });

    test("3. Fetch Interception during Page Navigation (Mock Fulfill)", () => {
        testInterceptedNavigation() catch unreachable;
    });

    test("4. Real HTTP Loading with Local Server (Roundtrip Cookies & DOM)", () => {
        testRealHttpLoadingWithServer() catch unreachable;
    });
}
