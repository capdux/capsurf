// test/cdp/test_fetch_interception.cp
// CapSurf Phase 9 Comprehensive Fetch Domain Interception & Glob State Machine Test Suite

import { test, expect_eq } from "cap:test";
import {
    FetchInterceptionState,
    RequestPattern,
    ParkedRequest,
    InterceptAction,
    InterceptActionKind,
    globMatches,
    errorReasonToNet,
    formatRequestPausedEvent,
} from "../../../src/cdp/fetch_interception.cp";

fn testGlobMatching()! {
    // 1. Universal wildcard
    expect_eq(globMatches("*", "https://example.com/api/v1"), true);
    expect_eq(globMatches("*", ""), true);

    // 2. Prefix wildcard
    expect_eq(globMatches("*.png", "https://cdn.example.com/logo.png"), true);
    expect_eq(globMatches("*.png", "https://cdn.example.com/logo.jpg"), false);

    // 3. Suffix wildcard
    expect_eq(globMatches("https://api.*", "https://api.github.com/users"), true);
    expect_eq(globMatches("https://api.*", "https://auth.github.com/login"), false);

    // 4. Infix wildcard
    expect_eq(globMatches("*/api/*/query*", "https://site.org/api/v2/query?lang=cp"), true);
    expect_eq(globMatches("*/api/*/query*", "https://site.org/web/v2/query?lang=cp"), false);

    // 5. Single char wildcard '?'
    expect_eq(globMatches("item_??.json", "item_01.json"), true);
    expect_eq(globMatches("item_??.json", "item_1.json"), false);

    // 6. No backtracking on multiline terminators
    expect_eq(globMatches("a*b", "a\nb"), false);
}

fn testFetchInterceptionStateLifecycle()! {
    let state = FetchInterceptionState{};
    expect_eq(state.enabled, false);

    // 1. Enable with patterns
    state.enabled = true;
    state.addPattern("*/api/*", "XHR", "Request");
    state.addPattern("*.png", "Image", "Request");
    expect_eq(state.pattern_count, 2);

    // 2. Matching verification
    expect_eq(state.matches("https://site.com/api/users", "XHR"), true);
    expect_eq(state.matches("https://site.com/images/cat.png", "Image"), true);
    expect_eq(state.matches("https://site.com/index.html", "Document"), false);
    expect_eq(state.matches("data:image/png;base64,123", "Image"), false); // data: skipped

    // 3. Register Parked Request
    const parked_opt = state.registerParked(
        "interception-001",
        "https://site.com/api/users",
        "POST",
        "{\"Content-Type\":\"application/json\"}",
        "{\"user\":\"agent\"}",
        "XHR",
    );
    expect_eq(parked_opt != null, true);
    expect_eq(state.parked_count, 1);

    const p = state.getParked("interception-001");
    expect_eq(p != null, true);
    expect_eq(p.?.is_paused, true);

    // 4. Format RequestPaused Event JSON
    let evt_buf: [1024]u8 = [0; 1024];
    const evt_json = formatRequestPausedEvent(&evt_buf, p.?);
    expect_eq(evt_json.len > 0, true);
    expect_eq(std.mem.indexOf(u8, evt_json, "\"requestId\":\"interception-001\"") != null, true);
    expect_eq(std.mem.indexOf(u8, evt_json, "\"method\":\"POST\"") != null, true);
    expect_eq(std.mem.indexOf(u8, evt_json, "\"postData\":") != null, true);
    expect_eq(std.mem.indexOf(u8, evt_json, "user") != null, true);

    // 5. Resolve with Fulfill
    const fulfilled = state.resolveParked(
        "interception-001",
        InterceptAction.makeFulfill(200, "{\"Server\":\"CapSurf\"}", "{\"status\":\"ok\"}"),
    );
    expect_eq(fulfilled, true);

    const resolved = state.getParked("interception-001");
    expect_eq(resolved != null, true);
    expect_eq(resolved.?.is_paused, false);
    expect_eq(resolved.?.verdict != null, true);
    expect_eq(resolved.?.verdict.?.kind == InterceptActionKind.Fulfill, true);
    expect_eq(resolved.?.verdict.?.responseCode, 200);
    expect_eq(std.mem.eql(u8, resolved.?.verdict.?.body, "{\"status\":\"ok\"}"), true);
}

fn testFetchInterceptionActionsAndErrors()! {
    let state = FetchInterceptionState{};
    state.enabled = true;

    // 1. Continue Action with Override
    const p1 = state.registerParked("req-continue", "https://old.url", "GET", "", "", "XHR");
    expect_eq(p1 != null, true);

    const cont_res = state.resolveParked(
        "req-continue",
        InterceptAction.makeContinueWith("https://new.url", "PUT", "{\"Authorization\":\"Bearer token\"}", "data=1"),
    );
    expect_eq(cont_res, true);

    const item1 = state.getParked("req-continue");
    expect_eq(item1.?.verdict.?.kind == InterceptActionKind.Continue, true);
    expect_eq(std.mem.eql(u8, item1.?.verdict.?.url, "https://new.url"), true);
    expect_eq(std.mem.eql(u8, item1.?.verdict.?.method, "PUT"), true);

    // 2. Fail Action with error mapping
    const p2 = state.registerParked("req-fail", "https://bad.url", "GET", "", "", "Script");
    expect_eq(p2 != null, true);

    const fail_res = state.resolveParked("req-fail", InterceptAction.makeFail("ConnectionRefused"));
    expect_eq(fail_res, true);

    const item2 = state.getParked("req-fail");
    expect_eq(item2.?.verdict.?.kind == InterceptActionKind.Fail, true);
    expect_eq(std.mem.eql(u8, item2.?.verdict.?.errorReason, "ConnectionRefused"), true);

    // 3. Error Reason Chrome Net Code mappings
    expect_eq(std.mem.eql(u8, errorReasonToNet("Aborted"), "net::ERR_ABORTED"), true);
    expect_eq(std.mem.eql(u8, errorReasonToNet("ConnectionRefused"), "net::ERR_CONNECTION_REFUSED"), true);
    expect_eq(std.mem.eql(u8, errorReasonToNet("TimedOut"), "net::ERR_TIMED_OUT"), true);
    expect_eq(std.mem.eql(u8, errorReasonToNet("AccessDenied"), "net::ERR_ACCESS_DENIED"), true);
    expect_eq(std.mem.eql(u8, errorReasonToNet("UnknownError"), "net::ERR_FAILED"), true);

    // 4. Release all pending
    const p3 = state.registerParked("req-release", "https://any.url", "GET", "", "", "Document");
    expect_eq(p3 != null, true);
    state.releaseAll();
    const item3 = state.getParked("req-release");
    expect_eq(item3.?.is_paused, false);
    expect_eq(item3.?.verdict != null, true);
    expect_eq(item3.?.verdict.?.kind == InterceptActionKind.Continue, true);
}

fn main() {
    test("1. Non-Backtracking Linear Glob Matching (* and ?)", () => {
        testGlobMatching() catch unreachable;
    });

    test("2. Fetch Interception Lifecycle & RequestPaused JSON formatting", () => {
        testFetchInterceptionStateLifecycle() catch unreachable;
    });

    test("3. Fetch Interception Actions (Continue, Fulfill, Fail & Net Mappings)", () => {
        testFetchInterceptionActionsAndErrors() catch unreachable;
    });
}
