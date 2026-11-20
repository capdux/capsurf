// test/cdp/test_screenshot_input.cp
// CapSurf Phase 4 Comprehensive Test Suite:
// CDP Page.captureScreenshot (Base64 PNG generation & dimension validation),
// CDP Input.dispatchMouseEvent (pixel hit-testing, onclick handler, mouseWheel scroll),
// CDP Input.insertText & dispatchKeyEvent (focused element attribute and text update),
// CDP Page.startScreencast / screencastFrameAck protocol handshake.

import { test, expect_eq } from "cap:test";
import { Base64 } from "cap:encoding";
import { CdpPageSession } from "../../src/cdp/session.cp";
import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt } from "../../src/cdp/types.cp";
import { CdpDispatcher } from "../../src/cdp/dispatch.cp";
import { Matcher } from "../../src/css/matcher.cp";
import { NULL_NODE } from "../../src/dom/dom.cp";
import { PngCodec } from "../../src/render/png.cp";

fn testCaptureScreenshot()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    // Set custom viewport
    session.viewport_width = 400.0;
    session.viewport_height = 300.0;

    let out_buf: [131072]u8 = undefined;
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    // Navigate to page
    const html_data = "data:text/html,<div style='width: 200px; height: 100px; background-color: #ef4444;'><h1 style='color: #ffffff;'>CapSurf</h1></div>";
    const nav_req = CdpRequest{
        id: 1,
        method: "Page.navigate",
        params_raw: "{\"url\":\"" ++ html_data ++ "\"}",
    };
    const nav_resp = CdpDispatcher.dispatch(session, nav_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(nav_resp.isOk(), true);

    // Call Page.captureScreenshot
    const shot_req = CdpRequest{
        id: 2,
        method: "Page.captureScreenshot",
        params_raw: "{\"format\":\"png\",\"optimizeForSpeed\":true}",
    };
    const shot_resp = CdpDispatcher.dispatch(session, shot_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(shot_resp.isOk(), true);

    // Verify valid base64 data
    const b64_str = extractJsonString(shot_resp.result_json, "data");
    expect_eq(b64_str != null, true);
    const b64 = b64_str.?;
    expect_eq(b64.len > 50, true);

    // Decode base64
    const decoded_len = Base64.decodeLen(b64.len);
    let png_buf = std.heap.page_allocator.alloc(u8, decoded_len) catch unreachable;
    defer std.heap.page_allocator.free(png_buf);
    const actual_len = Base64.decode(png_buf, b64) catch unreachable;
    expect_eq(actual_len > 30, true);

    const png_slice = png_buf[0..actual_len];

    // Verify PNG header magic: \x89 P N G \r \n \x1a \n
    expect_eq(png_slice[0], 0x89);
    expect_eq(png_slice[1], 'P');
    expect_eq(png_slice[2], 'N');
    expect_eq(png_slice[3], 'G');
    expect_eq(png_slice[4], 0x0D);
    expect_eq(png_slice[5], 0x0A);
    expect_eq(png_slice[6], 0x1A);
    expect_eq(png_slice[7], 0x0A);

    // Verify dimensions
    if (decoded_img := PngCodec.decode(png_slice)) {
        expect_eq(decoded_img.width, 400);
        expect_eq(decoded_img.height, 300);
    } else {
        expect_eq(false, true);
    }
}

fn testDispatchMouseEvent()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    session.viewport_width = 800.0;
    session.viewport_height = 600.0;

    let out_buf: [4096]u8 = undefined;
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    const html_data = "data:text/html,<html><body><button id='action-btn' onclick='globalThis.clickCount = (globalThis.clickCount || 0) + 1; globalThis.btnClicked = true;' style='width: 120px; height: 50px;'>Click Me</button><div style='width: 2000px; height: 2000px;'></div></body></html>";
    const nav_req = CdpRequest{
        id: 1,
        method: "Page.navigate",
        params_raw: "{\"url\":\"" ++ html_data ++ "\"}",
    };
    const nav_resp = CdpDispatcher.dispatch(session, nav_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(nav_resp.isOk(), true);

    const btn_id = Matcher.querySelector(&session.doc, "#action-btn");
    expect_eq(btn_id != NULL_NODE, true);
    const rect = session.layout.getBoundingClientRect(btn_id);
    expect_eq(rect.width > 0.0, true);
    expect_eq(rect.height > 0.0, true);

    const click_x = rect.x + rect.width / 2.0;
    const click_y = rect.y + rect.height / 2.0;

    // Dispatch mousePressed
    let params_buf: [256]u8 = undefined;
    const press_params = std.fmt.bufPrint(&params_buf, "{{\"type\":\"mousePressed\",\"x\":{d},\"y\":{d},\"button\":\"left\",\"clickCount\":1}}", .{ click_x, click_y }) catch unreachable;
    const press_req = CdpRequest{
        id: 2,
        method: "Input.dispatchMouseEvent",
        params_raw: press_params,
    };
    const press_resp = CdpDispatcher.dispatch(session, press_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(press_resp.isOk(), true);

    // Verify session.focused_node is updated; click fires on mouseReleased per Web/CDP semantics
    expect_eq(session.focused_node, btn_id);

    let eval_buf: [256]u8 = undefined;
    const eval_len = session.evaluate("globalThis.btnClicked === true", eval_buf[0..]);
    expect_eq(eval_len > 0, true);
    expect_eq(std.mem.indexOf(u8, eval_buf[0..eval_len], "\"value\":true") == null, true);

    // Dispatch mouseReleased
    const rel_params = std.fmt.bufPrint(&params_buf, "{{\"type\":\"mouseReleased\",\"x\":{d},\"y\":{d},\"button\":\"left\",\"clickCount\":1}}", .{ click_x, click_y }) catch unreachable;
    const rel_req = CdpRequest{
        id: 3,
        method: "Input.dispatchMouseEvent",
        params_raw: rel_params,
    };
    const rel_resp = CdpDispatcher.dispatch(session, rel_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(rel_resp.isOk(), true);

    // Verify onclick fired exactly once on release
    const eval_len2 = session.evaluate("globalThis.btnClicked === true && globalThis.clickCount === 1", eval_buf[0..]);
    expect_eq(eval_len2 > 0, true);
    expect_eq(std.mem.indexOf(u8, eval_buf[0..eval_len2], "\"value\":true") != null, true);

    // Test mouseWheel updates scroll_x / scroll_y
    const wheel_params = "{\"type\":\"mouseWheel\",\"x\":100,\"y\":100,\"deltaX\":15.5,\"deltaY\":42.0}";
    const wheel_req = CdpRequest{
        id: 4,
        method: "Input.dispatchMouseEvent",
        params_raw: wheel_params,
    };
    const wheel_resp = CdpDispatcher.dispatch(session, wheel_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(wheel_resp.isOk(), true);
    expect_eq(session.scroll_x, 15.5);
    expect_eq(session.scroll_y, 42.0);
}

fn testInsertText()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    session.viewport_width = 800.0;
    session.viewport_height = 600.0;

    let out_buf: [4096]u8 = undefined;
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    const html_data = "data:text/html,<html><body><input id='search-box' value='' style='width: 200px; height: 30px;' /><div id='target-div' style='width: 150px; height: 40px;'>Init</div></body></html>";
    const nav_req = CdpRequest{
        id: 1,
        method: "Page.navigate",
        params_raw: "{\"url\":\"" ++ html_data ++ "\"}",
    };
    const nav_resp = CdpDispatcher.dispatch(session, nav_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(nav_resp.isOk(), true);

    // 1. Focus input element via click
    const input_id = Matcher.querySelector(&session.doc, "#search-box");
    expect_eq(input_id != NULL_NODE, true);
    const rect = session.layout.getBoundingClientRect(input_id);
    const click_x = rect.x + 10.0;
    const click_y = rect.y + 10.0;

    let params_buf: [256]u8 = undefined;
    const press_params = std.fmt.bufPrint(&params_buf, "{{\"type\":\"mousePressed\",\"x\":{d},\"y\":{d},\"button\":\"left\"}}", .{ click_x, click_y }) catch unreachable;
    const press_req = CdpRequest{
        id: 2,
        method: "Input.dispatchMouseEvent",
        params_raw: press_params,
    };
    * = CdpDispatcher.dispatch(session, press_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(session.focused_node, input_id);

    // 2. Insert text into input element
    const insert_req = CdpRequest{
        id: 3,
        method: "Input.insertText",
        params_raw: "{\"text\":\"CapSurf\"}",
    };
    const insert_resp = CdpDispatcher.dispatch(session, insert_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(insert_resp.isOk(), true);

    // Verify DOM attribute value
    const val1 = session.doc.getAttribute(input_id, "value") ?? "";
    expect_eq(std.mem.eql(u8, val1, "CapSurf"), true);

    // Insert more text using dispatchKeyEvent with char
    const key_req = CdpRequest{
        id: 4,
        method: "Input.dispatchKeyEvent",
        params_raw: "{\"type\":\"char\",\"text\":\" Engine\"}",
    };
    const key_resp = CdpDispatcher.dispatch(session, key_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(key_resp.isOk(), true);

    const val2 = session.doc.getAttribute(input_id, "value") ?? "";
    expect_eq(std.mem.eql(u8, val2, "CapSurf Engine"), true);

    // Verify JS context reflects updated value
    let eval_buf: [256]u8 = undefined;
    const eval_len = session.evaluate("document.getElementById('search-box').value", eval_buf[0..]);
    expect_eq(eval_len > 0, true);
    expect_eq(std.mem.indexOf(u8, eval_buf[0..eval_len], "CapSurf Engine") != null, true);

    // 3. Test on regular element (div)
    const div_id = Matcher.querySelector(&session.doc, "#target-div");
    expect_eq(div_id != NULL_NODE, true);
    session.focused_node = div_id;

    const div_insert_req = CdpRequest{
        id: 5,
        method: "Input.insertText",
        params_raw: "{\"text\":\"-Updated\"}",
    };
    const div_insert_resp = CdpDispatcher.dispatch(session, div_insert_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(div_insert_resp.isOk(), true);

    const div_val = session.doc.getAttribute(div_id, "value") ?? "";
    expect_eq(std.mem.indexOf(u8, div_val, "-Updated") != null, true);
}

fn testScreencastProtocol()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    // 1. Page.startScreencast
    const start_req = CdpRequest{
        id: 10,
        method: "Page.startScreencast",
        params_raw: "{\"format\":\"jpeg\",\"quality\":80,\"maxWidth\":1280,\"maxHeight\":720}",
    };
    const start_resp = CdpDispatcher.dispatch(session, start_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(start_resp.isOk(), true);

    // 2. Page.screencastFrameAck
    const ack_req = CdpRequest{
        id: 11,
        method: "Page.screencastFrameAck",
        params_raw: "{\"sessionId\":1}",
    };
    const ack_resp = CdpDispatcher.dispatch(session, ack_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(ack_resp.isOk(), true);

    // 3. Page.stopScreencast
    const stop_req = CdpRequest{
        id: 12,
        method: "Page.stopScreencast",
        params_raw: "{}",
    };
    const stop_resp = CdpDispatcher.dispatch(session, stop_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(stop_resp.isOk(), true);
}

fn testDispatchKeyPressEvent()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    session.viewport_width = 800.0;
    session.viewport_height = 600.0;

    let out_buf: [4096]u8 = undefined;
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    const html_data = "data:text/html,<html><body><script>" ++
        "globalThis.__events = [];" ++
        "window.addEventListener('keydown', (e) => globalThis.__events.push('down:' + e.key + ':' + e.charCode));" ++
        "window.addEventListener('keypress', (e) => globalThis.__events.push('press:' + e.key + ':' + e.charCode));" ++
        "window.addEventListener('keyup', (e) => globalThis.__events.push('up:' + e.key + ':' + e.charCode));" ++
        "</script></body></html>";

    const nav_req = CdpRequest{
        id: 1,
        method: "Page.navigate",
        params_raw: "{\"url\":\"" ++ html_data ++ "\"}",
    };
    const nav_resp = CdpDispatcher.dispatch(session, nav_req, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(nav_resp.isOk(), true);

    // 1. Dispatch printable keydown with text -> should fire keydown AND keypress
    const kd_req = CdpRequest{
        id: 2,
        method: "Input.dispatchKeyEvent",
        params_raw: "{\"type\":\"keyDown\",\"key\":\"a\",\"code\":\"KeyA\",\"text\":\"a\"}",
    };
    * = CdpDispatcher.dispatch(session, kd_req, out_buf[0..], &out_evts, &evt_cnt);

    const ku_req = CdpRequest{
        id: 3,
        method: "Input.dispatchKeyEvent",
        params_raw: "{\"type\":\"keyUp\",\"key\":\"a\",\"code\":\"KeyA\"}",
    };
    * = CdpDispatcher.dispatch(session, ku_req, out_buf[0..], &out_evts, &evt_cnt);

    let eval_buf: [256]u8 = undefined;
    const len1 = session.evaluate("globalThis.__events.join(',')", eval_buf[0..]);
    expect_eq(len1 > 0, true);
    expect_eq(std.mem.indexOf(u8, eval_buf[0..len1], "down:a:0,press:a:97,up:a:0") != null, true);

    // 2. Dispatch char event directly -> should fire keypress
    const char_req = CdpRequest{
        id: 4,
        method: "Input.dispatchKeyEvent",
        params_raw: "{\"type\":\"char\",\"key\":\"b\",\"text\":\"b\"}",
    };
    * = CdpDispatcher.dispatch(session, char_req, out_buf[0..], &out_evts, &evt_cnt);

    const len2 = session.evaluate("globalThis.__events.slice(3).join(',')", eval_buf[0..]);
    expect_eq(len2 > 0, true);
    expect_eq(std.mem.indexOf(u8, eval_buf[0..len2], "press:b:98") != null, true);

    // 3. Dispatch Enter -> should fire keydown, keypress (charCode 13)
    const enter_req = CdpRequest{
        id: 5,
        method: "Input.dispatchKeyEvent",
        params_raw: "{\"type\":\"keyDown\",\"key\":\"Enter\",\"code\":\"Enter\"}",
    };
    * = CdpDispatcher.dispatch(session, enter_req, out_buf[0..], &out_evts, &evt_cnt);

    const len3 = session.evaluate("globalThis.__events.slice(4).join(',')", eval_buf[0..]);
    expect_eq(len3 > 0, true);
    expect_eq(std.mem.indexOf(u8, eval_buf[0..len3], "down:Enter:0,press:Enter:13") != null, true);

    // 4. Dispatch Backspace -> should NOT fire keypress
    const bs_req = CdpRequest{
        id: 6,
        method: "Input.dispatchKeyEvent",
        params_raw: "{\"type\":\"keyDown\",\"key\":\"Backspace\",\"code\":\"Backspace\"}",
    };
    * = CdpDispatcher.dispatch(session, bs_req, out_buf[0..], &out_evts, &evt_cnt);

    const len4 = session.evaluate("globalThis.__events.slice(6).join(',')", eval_buf[0..]);
    expect_eq(len4 > 0, true);
    expect_eq(std.mem.indexOf(u8, eval_buf[0..len4], "down:Backspace:0") != null, true);
}

fn main() {
    test("1. CDP Page.navigate and Page.captureScreenshot PNG Base64 Validation", () => {
        testCaptureScreenshot() catch unreachable;
    });

    test("2. CDP Input.dispatchMouseEvent Hit-Test, onclick Execution & mouseWheel", () => {
        testDispatchMouseEvent() catch unreachable;
    });

    test("3. CDP Input.insertText & dispatchKeyEvent Focus Element Property Update", () => {
        testInsertText() catch unreachable;
    });

    test("4. CDP Page.startScreencast & Page.screencastFrameAck Protocol Handshake", () => {
        testScreencastProtocol() catch unreachable;
    });

    test("5. CDP Input.dispatchKeyEvent keypress Event Dispatch for Printable Characters", () => {
        testDispatchKeyPressEvent() catch unreachable;
    });
}
