// test/cdp/test_cdp_screenshot.cp
// Tests for Page.captureScreenshot and Screencast protocol stubs in CapSurf CDP Page Domain

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { CdpPageSession } from "../../../src/cdp/session.cp";
import { CdpDispatcher } from "../../../src/cdp/dispatch.cp";
import { CdpRequest, CdpResponse, CdpEvent, extractJsonString } from "../../../src/cdp/types.cp";
import { Base64 } from "cap:encoding";
import { PngCodec } from "../../../src/render/png.cp";

fn testSessionCaptureScreenshot()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    session.viewport_width = 800.0;
    session.viewport_height = 600.0;

    const html = "<!DOCTYPE html><html><body><div style=\"width: 300px; height: 150px; background-color: #3b82f6;\"><h1>Native CapSurf</h1></div></body></html>";
    session.navigate(html, "http://example.com/test.html", "") catch unreachable;

    // Test captureScreenshot directly
    const png_bytes = session.captureScreenshot(1) catch unreachable;
    defer session.allocator.free(png_bytes);

    expect_eq(png_bytes.len > 64, true);
    expect_eq(png_bytes[0], 0x89);
    expect_eq(png_bytes[1], 'P');
    expect_eq(png_bytes[2], 'N');
    expect_eq(png_bytes[3], 'G');

    // Decode and verify dimensions
    if (img := PngCodec.decode(png_bytes)) {
        expect_eq(img.width, 800);
        expect_eq(img.height, 600);
    } else {
        expect_eq(false, true);
    }
}

fn testPageCaptureScreenshotCdp()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    session.viewport_width = 640.0;
    session.viewport_height = 480.0;

    const html = "<html><body><button style=\"width: 100px; height: 40px; background-color: #10b981;\">Click</button></body></html>";
    session.navigate(html, "about:blank", "") catch unreachable;

    let out_buf: [131072]u8 = undefined;
    let events: [64]CdpEvent = [CdpEvent{}; 64];
    let event_count: usize = 0;

    // 1. Page.captureScreenshot default
    const req1 = CdpRequest{
        id: 101,
        method: "Page.captureScreenshot",
        params_raw: "{\"format\":\"png\"}",
    };
    const resp1 = CdpDispatcher.dispatch(session, req1, out_buf[0..], &events, &event_count);
    expect_eq(resp1.isOk(), true);

    const b64_data = extractJsonString(resp1.result_json, "data") ?? "";
    expect_eq(b64_data.len > 0, true);

    // Decode base64 and verify PNG magic
    let decode_buf = session.allocator.alloc(u8, b64_data.len) catch unreachable;
    defer session.allocator.free(decode_buf);
    const decoded_len = Base64.decode(decode_buf, b64_data) catch unreachable;
    expect_eq(decoded_len > 64, true);
    expect_eq(decode_buf[0], 0x89);
    expect_eq(decode_buf[1], 'P');
    expect_eq(decode_buf[2], 'N');
    expect_eq(decode_buf[3], 'G');

    // 2. Page.captureScreenshot with quality and speed options
    const req2 = CdpRequest{
        id: 102,
        method: "Page.captureScreenshot",
        params_raw: "{\"format\":\"png\",\"quality\":80,\"optimizeForSpeed\":true}",
    };
    const resp2 = CdpDispatcher.dispatch(session, req2, out_buf[0..], &events, &event_count);
    expect_eq(resp2.isOk(), true);
    const b64_data2 = extractJsonString(resp2.result_json, "data") ?? "";
    expect_eq(b64_data2.len > 0, true);
}

fn testScreencastProtocolStubs()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let events: [64]CdpEvent = [CdpEvent{}; 64];
    let event_count: usize = 0;

    // 1. Page.startScreencast
    const req1 = CdpRequest{
        id: 201,
        method: "Page.startScreencast",
        params_raw: "{\"format\":\"jpeg\",\"quality\":80,\"maxWidth\":800,\"maxHeight\":600,\"everyNthFrame\":1}",
    };
    const resp1 = CdpDispatcher.dispatch(session, req1, out_buf[0..], &events, &event_count);
    expect_eq(resp1.isOk(), true);
    expect_eq(std.mem.eql(u8, resp1.result_json, "{}"), true);

    // 2. Page.screencastFrameAck
    const req2 = CdpRequest{
        id: 202,
        method: "Page.screencastFrameAck",
        params_raw: "{\"sessionId\":1}",
    };
    const resp2 = CdpDispatcher.dispatch(session, req2, out_buf[0..], &events, &event_count);
    expect_eq(resp2.isOk(), true);
    expect_eq(std.mem.eql(u8, resp2.result_json, "{}"), true);

    // 3. Page.stopScreencast
    const req3 = CdpRequest{
        id: 203,
        method: "Page.stopScreencast",
        params_raw: "{}",
    };
    const resp3 = CdpDispatcher.dispatch(session, req3, out_buf[0..], &events, &event_count);
    expect_eq(resp3.isOk(), true);
    expect_eq(std.mem.eql(u8, resp3.result_json, "{}"), true);
}

fn main() {
    test("1. CdpPageSession.captureScreenshot PNG generation and dimensions", () => {
        testSessionCaptureScreenshot() catch unreachable;
    });

    test("2. Page.captureScreenshot CDP JSON response and Base64 roundtrip", () => {
        testPageCaptureScreenshotCdp() catch unreachable;
    });

    test("3. Screencast protocol stubs (startScreencast, screencastFrameAck, stopScreencast)", () => {
        testScreencastProtocolStubs() catch unreachable;
    });
}
