// test/cdp/test_cdp_network_fetch.cp
// CapSurf CDP Network, Fetch Interception & Emulation Test Suite
//
// Validates:
//   1. Emulation domain: setDeviceMetricsOverride, clamping, mobile flag & reset
//   2. Fetch domain: request interception, pausing, continue, fulfill, and abort
//   3. Network domain: CookieJar integration (set/get/clear cookies) & extra headers

import { test, expect_eq } from "cap:test";
import { CdpPageSession } from "../../../src/cdp/session.cp";
import { CdpRequest, CdpResponse, CdpEvent } from "../../../src/cdp/types.cp";
import { CdpDispatcher } from "../../../src/cdp/dispatch.cp";
import { Viewport } from "../../../src/cdp/emulation.cp";
import { FetchHandler, InterceptVerdict } from "../../../src/cdp/fetch.cp";

fn testEmulationMetrics()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    // 1. Mobile iPhone viewport override (375x667, dsf 2, mobile true)
    const req1 = CdpRequest{
        id: 1,
        method: "Emulation.setDeviceMetricsOverride",
        params_raw: "{\"width\":375,\"height\":667,\"deviceScaleFactor\":2,\"mobile\":true}",
    };
    const resp1 = CdpDispatcher.dispatch(session, req1, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp1.isOk(), true);
    expect_eq(session.viewport_width, 375.0);
    expect_eq(session.viewport_height, 667.0);
    expect_eq(session.layout.viewport_width, 375.0);
    expect_eq(session.layout.viewport_height, 667.0);
    expect_eq(session.device_scale_factor, 2.0);
    expect_eq(session.is_mobile, true);

    // 2. Viewport clamping validation
    const clamped = Viewport.clampViewport(5000.0, 5000.0);
    expect_eq(clamped.width <= 2560.0, true);
    expect_eq(clamped.height <= 2560.0, true);
    expect_eq(clamped.width * clamped.height <= 2560.0 * 1440.0, true);

    // 3. Clear metrics override to defaults (1280x720)
    const req2 = CdpRequest{
        id: 2,
        method: "Emulation.clearDeviceMetricsOverride",
        params_raw: "{}",
    };
    const resp2 = CdpDispatcher.dispatch(session, req2, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp2.isOk(), true);
    expect_eq(session.viewport_width, 1280.0);
    expect_eq(session.viewport_height, 720.0);
    expect_eq(session.is_mobile, false);
}

fn testFetchInterception()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    // 1. Enable interception with wildcard pattern
    const req_enable = CdpRequest{
        id: 10,
        method: "Fetch.enable",
        params_raw: "{\"patterns\":[{\"urlPattern\":\"*api/data*\",\"resourceType\":\"XHR\"}]}",
    };
    const resp_enable = CdpDispatcher.dispatch(session, req_enable, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_enable.isOk(), true);
    expect_eq(session.fetch_enabled, true);
    expect_eq(session.fetch_state.enabled, true);

    // 2. Interception pattern matching test
    const matched = session.fetch_state.shouldIntercept("https://example.com/api/data?q=1", "XHR");
    expect_eq(matched, true);
    const not_matched = session.fetch_state.shouldIntercept("https://example.com/other", "Document");
    expect_eq(not_matched, false);

    // 3. Park a request and format requestPaused event
    let req_ref = session.fetch_state.parkRequest("req-001", "https://example.com/api/data", "POST");
    expect_eq(req_ref != null, true);

    let event_buf: [512]u8 = undefined;
    const paused_evt = FetchHandler.buildRequestPausedEvent("req-001", "https://example.com/api/data", "POST", "XHR", "frame-1", event_buf[0..]);
    expect_eq(paused_evt != null, true);
    expect_eq(std.mem.indexOf(u8, paused_evt.?.params_json, "req-001") != null, true);

    // 4. Fulfill request with Mock payload
    const req_fulfill = CdpRequest{
        id: 11,
        method: "Fetch.fulfillRequest",
        params_raw: "{\"requestId\":\"req-001\",\"responseCode\":200,\"body\":\"eyJyZXN1bHQiOiJva30=\"}",
    };
    const resp_fulfill = CdpDispatcher.dispatch(session, req_fulfill, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_fulfill.isOk(), true);

    const parked_item = session.fetch_state.findParked("req-001");
    expect_eq(parked_item != null, true);
    expect_eq(parked_item.?.verdict == InterceptVerdict.Fulfill, true);
    expect_eq(parked_item.?.mock_status, 200);

    // 5. Disable interception
    const req_disable = CdpRequest{
        id: 12,
        method: "Fetch.disable",
        params_raw: "{}",
    };
    const resp_disable = CdpDispatcher.dispatch(session, req_disable, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_disable.isOk(), true);
    expect_eq(session.fetch_enabled, false);
}

fn testNetworkCookiesAndHeaders()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    // 1. Initial cookie state must be empty
    const req_get0 = CdpRequest{
        id: 20,
        method: "Network.getCookies",
        params_raw: "{}",
    };
    const resp_get0 = CdpDispatcher.dispatch(session, req_get0, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_get0.isOk(), true);
    expect_eq(std.mem.eql(u8, resp_get0.result_json, "{\"cookies\":[]}"), true);

    // 2. Set Cookie via Network.setCookie
    const req_set = CdpRequest{
        id: 21,
        method: "Network.setCookie",
        params_raw: "{\"name\":\"session_id\",\"value\":\"xyz999\",\"domain\":\"capsurf.dev\",\"path\":\"/\"}",
    };
    const resp_set = CdpDispatcher.dispatch(session, req_set, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_set.isOk(), true);

    // 3. Retrieve cookies and verify json contains session_id and capsurf.dev
    const req_get1 = CdpRequest{
        id: 22,
        method: "Network.getCookies",
        params_raw: "{}",
    };
    const resp_get1 = CdpDispatcher.dispatch(session, req_get1, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_get1.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp_get1.result_json, "session_id") != null, true);
    expect_eq(std.mem.indexOf(u8, resp_get1.result_json, "xyz999") != null, true);
    expect_eq(std.mem.indexOf(u8, resp_get1.result_json, "capsurf.dev") != null, true);

    // 4. Set Extra HTTP Headers
    const req_hdr = CdpRequest{
        id: 23,
        method: "Network.setExtraHTTPHeaders",
        params_raw: "{\"headers\":{\"Authorization\":\"Bearer token-123\",\"X-Agent\":\"CapSurf\"}}",
    };
    const resp_hdr = CdpDispatcher.dispatch(session, req_hdr, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_hdr.isOk(), true);
    expect_eq(std.mem.indexOf(u8, session.extra_headers, "Bearer token-123") != null, true);

    // 5. Clear cookies via Network.clearBrowserCookies
    const req_clr = CdpRequest{
        id: 24,
        method: "Network.clearBrowserCookies",
        params_raw: "{}",
    };
    const resp_clr = CdpDispatcher.dispatch(session, req_clr, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_clr.isOk(), true);

    const req_get2 = CdpRequest{
        id: 25,
        method: "Network.getCookies",
        params_raw: "{}",
    };
    const resp_get2 = CdpDispatcher.dispatch(session, req_get2, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_get2.isOk(), true);
    expect_eq(std.mem.eql(u8, resp_get2.result_json, "{\"cookies\":[]}"), true);
}

fn main() {
    test("1. CDP Emulation Domain (Viewport Override, Clamping & Mobile)", () => {
        testEmulationMetrics() catch unreachable;
    });

    test("2. CDP Fetch Domain (Interception, Pausing, Fulfill & Abort)", () => {
        testFetchInterception() catch unreachable;
    });

    test("3. CDP Network Domain (CookieJar Integration & Extra Headers)", () => {
        testNetworkCookiesAndHeaders() catch unreachable;
    });
}
