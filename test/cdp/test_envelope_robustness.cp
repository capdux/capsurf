// test/cdp/test_envelope_robustness.cp
// Tests CDP envelope validation, error code conformance, boundary robustness,
// and newly implemented CDP methods in Page, DOM, Runtime, Browser, Target domains.

import { test, expect_eq } from "cap:test";
import { CdpRequest, CdpResponse, CdpEvent } from "../../src/cdp/types.cp";
import { CdpPageSession } from "../../src/cdp/session.cp";
import { CdpDispatcher } from "../../src/cdp/dispatch.cp";

fn runEnvelopeParsingTest()! {
    // 1. Truncated malformed JSON gets -32700
    const req1 = CdpRequest.parse("{");
    expect_eq(req1.hasEnvelopeError(), true);
    expect_eq(req1.envelope_error_code, -32700);

    // 2. JSON array gets -32600
    const req2 = CdpRequest.parse("[1, 2]");
    expect_eq(req2.hasEnvelopeError(), true);
    expect_eq(req2.envelope_error_code, -32600);
    expect_eq(req2.id, 0);

    // 3. String id gets -32600 and preserves raw_id
    const req3 = CdpRequest.parse("{\"id\": \"abc\", \"method\": \"Browser.getVersion\"}");
    expect_eq(req3.hasEnvelopeError(), true);
    expect_eq(req3.envelope_error_code, -32600);
    expect_eq(std.mem.eql(u8, req3.raw_id, "abc"), true);

    // 4. Non-string sessionId gets -32600
    const req4 = CdpRequest.parse("{\"id\": 1, \"method\": \"Browser.getVersion\", \"sessionId\": 7}");
    expect_eq(req4.hasEnvelopeError(), true);
    expect_eq(req4.envelope_error_code, -32600);
    expect_eq(req4.id, 1);

    // 5. Non-object params gets -32600
    const req5 = CdpRequest.parse("{\"id\": 2, \"method\": \"Browser.getVersion\", \"params\": \"x\"}");
    expect_eq(req5.hasEnvelopeError(), true);
    expect_eq(req5.envelope_error_code, -32600);
    expect_eq(req5.id, 2);

    // 6. Valid null params and sessionId are accepted
    const req6 = CdpRequest.parse("{\"id\": 3, \"method\": \"Browser.getVersion\", \"params\": null, \"sessionId\": null}");
    expect_eq(req6.hasEnvelopeError(), false);
    expect_eq(req6.id, 3);
    expect_eq(std.mem.eql(u8, req6.method, "Browser.getVersion"), true);

    // 7. Missing method gets -32600
    const req7 = CdpRequest.parse("{\"id\": 4}");
    expect_eq(req7.hasEnvelopeError(), true);
    expect_eq(req7.envelope_error_code, -32600);
    expect_eq(req7.id, 4);
}

fn runDispatcherCoverageTest()! {
    const allocator = std.heap.page_allocator;
    const session = CdpPageSession.init(allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = [0; 4096];
    let events: [4]CdpEvent = [CdpEvent{}; 4];
    let event_count: usize = 0;

    // 1. Dispatching envelope error directly yields formatted error response
    const req1 = CdpRequest.parse("{\"id\": \"abc\", \"method\": \"Browser.getVersion\"}");
    const resp1 = CdpDispatcher.dispatch(session, req1, &out_buf, &events, &event_count);
    expect_eq(resp1.isOk(), false);
    expect_eq(resp1.error_code, -32600);
    expect_eq(std.mem.eql(u8, resp1.raw_id, "abc"), true);

    // 2. Unknown method yields -32601 Method not found
    const req2 = CdpRequest.parse("{\"id\": 10, \"method\": \"UnknownDomain.unknownMethod\"}");
    const resp2 = CdpDispatcher.dispatch(session, req2, &out_buf, &events, &event_count);
    expect_eq(resp2.isOk(), false);
    expect_eq(resp2.error_code, -32601);

    // 3. Permissive stub domains (e.g. PerformanceTimeline) return ok {}
    const req3 = CdpRequest.parse("{\"id\": 11, \"method\": \"PerformanceTimeline.enable\"}");
    const resp3 = CdpDispatcher.dispatch(session, req3, &out_buf, &events, &event_count);
    expect_eq(resp3.isOk(), true);
    expect_eq(std.mem.eql(u8, resp3.result_json, "{}"), true);

    // 4. Page domain extended methods
    const req4 = CdpRequest.parse("{\"id\": 12, \"method\": \"Page.setLifecycleEventsEnabled\"}");
    const resp4 = CdpDispatcher.dispatch(session, req4, &out_buf, &events, &event_count);
    expect_eq(resp4.isOk(), true);

    const req5 = CdpRequest.parse("{\"id\": 13, \"method\": \"Page.addScriptToEvaluateOnNewDocument\", \"params\": {\"source\": \"1+1\"}}");
    const resp5 = CdpDispatcher.dispatch(session, req5, &out_buf, &events, &event_count);
    expect_eq(resp5.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp5.result_json, "identifier") != null, true);

    // 5. DOM domain extended methods
    const req6 = CdpRequest.parse("{\"id\": 14, \"method\": \"DOM.resolveNode\", \"params\": {\"nodeId\": 1}}");
    const resp6 = CdpDispatcher.dispatch(session, req6, &out_buf, &events, &event_count);
    expect_eq(resp6.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp6.result_json, "objectId") != null, true);

    const req7 = CdpRequest.parse("{\"id\": 15, \"method\": \"DOM.getFlattenedDocument\"}");
    const resp7 = CdpDispatcher.dispatch(session, req7, &out_buf, &events, &event_count);
    expect_eq(resp7.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp7.result_json, "nodes") != null, true);

    // 6. Runtime domain extended methods
    const req8 = CdpRequest.parse("{\"id\": 16, \"method\": \"Runtime.getHeapUsage\"}");
    const resp8 = CdpDispatcher.dispatch(session, req8, &out_buf, &events, &event_count);
    expect_eq(resp8.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp8.result_json, "usedSize") != null, true);

    // 7. Kitesurf/CapSurf outbound filter checks
    const req9 = CdpRequest.parse("{\"id\": 17, \"method\": \"Kitesurf.setOutboundFilters\", \"params\": {\"allowRequestPattern\": [\"^https://a/\", null]}}");
    const resp9 = CdpDispatcher.dispatch(session, req9, &out_buf, &events, &event_count);
    expect_eq(resp9.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp9.result_json, "\"ok\":false") != null, true);

    const req10 = CdpRequest.parse("{\"id\": 18, \"method\": \"Kitesurf.setOutboundFilters\", \"params\": {\"allowRequestPattern\": [\"^https://a/\"]}}");
    const resp10 = CdpDispatcher.dispatch(session, req10, &out_buf, &events, &event_count);
    expect_eq(resp10.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp10.result_json, "\"ok\":true") != null, true);
}

fn main() {
    test("1. CDP envelope parsing and boundary robustness", () => {
        runEnvelopeParsingTest()? ;
    });

    test("2. CDP Dispatcher domain coverage and error handling", () => {
        runDispatcherCoverageTest()? ;
    });
}
