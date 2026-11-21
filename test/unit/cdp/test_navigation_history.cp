// test/cdp/test_navigation_history.cp
// CapSurf CDP Page.getNavigationHistory & Same-Document Navigation Unit Tests

import { CdpPageSession } from "../../../src/cdp/session.cp";
import { CdpRequest, CdpResponse, CdpEvent } from "../../../src/cdp/types.cp";
import { CdpDispatcher } from "../../../src/cdp/dispatch.cp";

fn check(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

fn testInitialState() {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = [0; 4096];
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    const req = CdpRequest{
        id: 1,
        method: "Page.getNavigationHistory",
        params_raw: "{}",
    };
    const resp = CdpDispatcher.dispatch(session, req, out_buf[0..], &out_evts, &evt_cnt);
    check(resp.isOk(), "Page.getNavigationHistory response isOk");

    const json = resp.result_json;
    check(std.mem.indexOf(u8, json, "\"currentIndex\":0") != null, "initial currentIndex is 0");
    check(std.mem.indexOf(u8, json, "\"entries\":[") != null, "has entries array");
    check(std.mem.indexOf(u8, json, "\"id\":1") != null, "entry id is 1");
    check(std.mem.indexOf(u8, json, "\"url\":\"about:blank\"") != null, "entry url is about:blank");
    check(std.mem.indexOf(u8, json, "\"userTypedURL\":\"about:blank\"") != null, "entry userTypedURL is about:blank");
    check(std.mem.indexOf(u8, json, "\"title\":\"Blank Page\"") != null, "entry title is Blank Page");
    check(std.mem.indexOf(u8, json, "\"transitionType\":\"link\"") != null, "entry transitionType is link");

    std.debug.print("PASS: 1. Initial Navigation History State\n", .{});
}

fn testMultipleNavigations() {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = [0; 4096];
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    // Navigate to Page 1
    const html1 = "<!DOCTYPE html><html><head><title>First Page</title></head><body><h1>One</h1></body></html>";
    session.navigate(html1, "https://example.com/first", "") catch unreachable;

    const req1 = CdpRequest{ id: 2, method: "Page.getNavigationHistory", params_raw: "{}" };
    const resp1 = CdpDispatcher.dispatch(session, req1, out_buf[0..], &out_evts, &evt_cnt);
    check(resp1.isOk(), "nav 1 response isOk");
    check(std.mem.indexOf(u8, resp1.result_json, "\"currentIndex\":1") != null, "nav 1 currentIndex is 1");
    check(std.mem.indexOf(u8, resp1.result_json, "\"id\":2") != null, "nav 1 has id 2");
    check(std.mem.indexOf(u8, resp1.result_json, "\"url\":\"https://example.com/first\"") != null, "nav 1 has url /first");
    check(std.mem.indexOf(u8, resp1.result_json, "\"title\":\"First Page\"") != null, "nav 1 has title First Page");

    // Navigate to Page 2
    const html2 = "<!DOCTYPE html><html><head><title>Second Page</title></head><body><h1>Two</h1></body></html>";
    session.navigate(html2, "https://example.com/second", "") catch unreachable;

    const req2 = CdpRequest{ id: 3, method: "Page.getNavigationHistory", params_raw: "{}" };
    const resp2 = CdpDispatcher.dispatch(session, req2, out_buf[0..], &out_evts, &evt_cnt);
    check(resp2.isOk(), "nav 2 response isOk");
    check(std.mem.indexOf(u8, resp2.result_json, "\"currentIndex\":2") != null, "nav 2 currentIndex is 2");
    check(std.mem.indexOf(u8, resp2.result_json, "\"id\":3") != null, "nav 2 has id 3");
    check(std.mem.indexOf(u8, resp2.result_json, "\"url\":\"https://example.com/second\"") != null, "nav 2 has url /second");
    check(std.mem.indexOf(u8, resp2.result_json, "\"title\":\"Second Page\"") != null, "nav 2 has title Second Page");

    // Navigate to Page 3
    const html3 = "<!DOCTYPE html><html><head><title>Third Page</title></head><body><h1>Three</h1></body></html>";
    session.navigate(html3, "https://example.com/third", "") catch unreachable;

    const req3 = CdpRequest{ id: 4, method: "Page.getNavigationHistory", params_raw: "{}" };
    const resp3 = CdpDispatcher.dispatch(session, req3, out_buf[0..], &out_evts, &evt_cnt);
    check(resp3.isOk(), "nav 3 response isOk");
    check(std.mem.indexOf(u8, resp3.result_json, "\"currentIndex\":3") != null, "nav 3 currentIndex is 3");
    check(std.mem.indexOf(u8, resp3.result_json, "\"id\":4") != null, "nav 3 has id 4");
    check(std.mem.indexOf(u8, resp3.result_json, "\"url\":\"https://example.com/third\"") != null, "nav 3 has url /third");
    check(std.mem.indexOf(u8, resp3.result_json, "\"title\":\"Third Page\"") != null, "nav 3 has title Third Page");

    std.debug.print("PASS: 2. Multiple Navigations & Index Tracking\n", .{});
}

fn testReload() {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = [0; 4096];
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    const html_v1 = "<!DOCTYPE html><html><head><title>Version 1</title></head><body><h1>V1</h1></body></html>";
    session.navigate(html_v1, "https://example.com/page", "") catch unreachable;

    const req1 = CdpRequest{ id: 10, method: "Page.getNavigationHistory", params_raw: "{}" };
    const resp1 = CdpDispatcher.dispatch(session, req1, out_buf[0..], &out_evts, &evt_cnt);
    check(std.mem.indexOf(u8, resp1.result_json, "\"currentIndex\":1") != null, "v1 currentIndex is 1");
    check(std.mem.indexOf(u8, resp1.result_json, "\"title\":\"Version 1\"") != null, "v1 title is Version 1");

    // Reload with updated title
    const html_v2 = "<!DOCTYPE html><html><head><title>Version 2</title></head><body><h1>V2</h1></body></html>";
    session.navigate(html_v2, "https://example.com/page", "") catch unreachable;

    const req2 = CdpRequest{ id: 11, method: "Page.getNavigationHistory", params_raw: "{}" };
    const resp2 = CdpDispatcher.dispatch(session, req2, out_buf[0..], &out_evts, &evt_cnt);
    check(std.mem.indexOf(u8, resp2.result_json, "\"currentIndex\":1") != null, "reload keeps currentIndex at 1");
    check(std.mem.indexOf(u8, resp2.result_json, "\"title\":\"Version 2\"") != null, "title updated to Version 2 in place");

    std.debug.print("PASS: 3. Reload / Same URL Update in Place\n", .{});
}

fn testBackwardAndTruncation() {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = [0; 4096];
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    session.navigate("<!DOCTYPE html><html><head><title>Page A</title></head><body><h1>A</h1></body></html>", "https://example.com/a", "") catch unreachable;
    session.navigate("<!DOCTYPE html><html><head><title>Page B</title></head><body><h1>B</h1></body></html>", "https://example.com/b", "") catch unreachable;

    // Navigate back to entry id 2 (Page A)
    const req_back = CdpRequest{ id: 20, method: "Page.navigateToHistoryEntry", params_raw: "{\"entryId\":2}" };
    const resp_back = CdpDispatcher.dispatch(session, req_back, out_buf[0..], &out_evts, &evt_cnt);
    check(resp_back.isOk(), "Page.navigateToHistoryEntry isOk");

    const req_hist1 = CdpRequest{ id: 21, method: "Page.getNavigationHistory", params_raw: "{}" };
    const resp_hist1 = CdpDispatcher.dispatch(session, req_hist1, out_buf[0..], &out_evts, &evt_cnt);
    check(std.mem.indexOf(u8, resp_hist1.result_json, "\"currentIndex\":1") != null, "currentIndex back to 1");

    // Navigate to new branch (Page C) - should truncate forward entry Page B
    session.navigate("<!DOCTYPE html><html><head><title>Page C</title></head><body><h1>C</h1></body></html>", "https://example.com/c", "") catch unreachable;

    const req_hist2 = CdpRequest{ id: 22, method: "Page.getNavigationHistory", params_raw: "{}" };
    const resp_hist2 = CdpDispatcher.dispatch(session, req_hist2, out_buf[0..], &out_evts, &evt_cnt);
    check(std.mem.indexOf(u8, resp_hist2.result_json, "\"currentIndex\":2") != null, "currentIndex is 2");
    check(std.mem.indexOf(u8, resp_hist2.result_json, "\"url\":\"https://example.com/c\"") != null, "has /c");
    check(std.mem.indexOf(u8, resp_hist2.result_json, "\"url\":\"https://example.com/b\"") == null, "forward /b was truncated");

    std.debug.print("PASS: 4. Backward Navigation & Forward History Truncation\n", .{});
}

fn testSameDocPoll() {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    session.navigate("<!DOCTYPE html><html><head><title>Single Page App</title></head><body><div id='content'>SPA</div></body></html>", "https://example.com/spa", "") catch unreachable;

    // Test manual pushNavigatedWithinDocEvent
    session.pushNavigatedWithinDocEvent("https://example.com/spa#section-intro");

    let polled_evts: [64]CdpEvent = [CdpEvent{}; 64];
    const polled_cnt = session.pollNavigatedWithinDocEvents(&polled_evts);
    check(polled_cnt == 1, "polled 1 event");
    check(std.mem.eql(u8, polled_evts[0].method, "Page.navigatedWithinDocument"), "event method is Page.navigatedWithinDocument");
    check(std.mem.indexOf(u8, polled_evts[0].params_json, "\"frameId\":\"capsurf-frame-001\"") != null, "has frameId capsurf-frame-001");
    check(std.mem.indexOf(u8, polled_evts[0].params_json, "\"url\":\"https://example.com/spa#section-intro\"") != null, "has fragment url");

    // Verify history recorded the same-doc push
    let out_buf: [4096]u8 = [0; 4096];
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    const req_hist = CdpRequest{ id: 30, method: "Page.getNavigationHistory", params_raw: "{}" };
    const resp_hist = CdpDispatcher.dispatch(session, req_hist, out_buf[0..], &out_evts, &evt_cnt);
    check(std.mem.indexOf(u8, resp_hist.result_json, "\"currentIndex\":2") != null, "currentIndex is 2 after same-doc push");
    check(std.mem.indexOf(u8, resp_hist.result_json, "\"url\":\"https://example.com/spa#section-intro\"") != null, "history has fragment entry");

    std.debug.print("PASS: 5. Same-Document Navigation & pollNavigatedWithinDocEvents\n", .{});
}

fn testJsPushStateSecurity() {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    session.navigate("<!DOCTYPE html><html><head><title>Origin Test</title></head><body><div id='main'>App</div></body></html>", "https://ks.invalid/app", "") catch unreachable;

    // Valid same-origin pushState
    let out_buf: [4096]u8 = [0; 4096];
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    const req_eval1 = CdpRequest{
        id: 40,
        method: "Runtime.evaluate",
        params_raw: "{\"expression\":\"history.pushState({}, '', 'https://ks.invalid/app#details'); null\"}",
    };

    const resp_eval1 = CdpDispatcher.dispatch(session, req_eval1, out_buf[0..], &out_evts, &evt_cnt);
    check(resp_eval1.isOk(), "eval 1 isOk");
    check(evt_cnt >= 1, "eval 1 emitted event");
    check(std.mem.eql(u8, out_evts[0].method, "Page.navigatedWithinDocument"), "event is Page.navigatedWithinDocument");
    check(std.mem.indexOf(u8, out_evts[0].params_json, "https://ks.invalid/app#details") != null, "event has url");

    // Invalid cross-origin pushState: should be rejected by canRewriteDocumentUrl
    let out_evts2: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt2: usize = 0;

    const req_eval2 = CdpRequest{
        id: 41,
        method: "Runtime.evaluate",
        params_raw: "{\"expression\":\"history.pushState({}, '', 'https://other.invalid/hacked'); null\"}",
    };
    const resp_eval2 = CdpDispatcher.dispatch(session, req_eval2, out_buf[0..], &out_evts2, &evt_cnt2);
    check(resp_eval2.isOk(), "eval 2 isOk");
    check(evt_cnt2 == 0, "cross-origin pushState emitted zero events");

    // Verify session URL was NOT moved to cross-origin
    check(std.mem.indexOf(u8, session.url, "other.invalid") == null, "session url not changed to other.invalid");

    std.debug.print("PASS: 6. JS history.pushState & Cross-Origin Security Check\n", .{});
}

pub fn main() {
    testInitialState();
    testMultipleNavigations();
    testReload();
    testBackwardAndTruncation();
    testSameDocPoll();
    testJsPushStateSecurity();

    std.debug.print("\n=== ALL NAVIGATION HISTORY TESTS PASSED (100% OK) ===\n", .{});
}
