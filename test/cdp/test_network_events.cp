// test/cdp/test_network_events.cp
// Tests for CapSurf NetworkEventTracker & NetworkEventTrackerRegistry
// Migrated from vendor/kitesurf/packages/engine/test/network-events.test.ts

import {
    MAX_RETAINED_BODY_BYTES,
    RESPONSE_BODY_CAP,
    RequestRecord,
    NetworkEventTracker,
    inferResourceType,
    isTextLikeMime,
} from "../../src/cdp/network_events.cp";

import {
    CONNECTION_ID_HEADER,
    FRAME_ID_HEADER,
    NetworkEventTrackerRegistry,
    registerTracker,
    unregisterTracker,
    getTracker,
} from "../../src/cdp/network_events_registry.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn main() {
    // 1. Constants
    check("MAX_RETAINED_BODY_BYTES", MAX_RETAINED_BODY_BYTES == 16 * 1024 * 1024);
    check("RESPONSE_BODY_CAP", RESPONSE_BODY_CAP == 4 * 1024 * 1024);
    check("CONNECTION_ID_HEADER", std.mem.eql(u8, CONNECTION_ID_HEADER, "x-kitesurf-connection-id"));
    check("FRAME_ID_HEADER", std.mem.eql(u8, FRAME_ID_HEADER, "x-kitesurf-frame-id"));

    // 2. Tracker initialization & sequence generation
    let tracker = NetworkEventTracker.init();
    tracker.setConnectionId("conn-abc12345");

    let req_buf: [64]u8 = [0; 64];
    const req_id = tracker.nextRequestId(req_buf[0..]);
    check("nextRequestId format", std.mem.startsWith(u8, req_id, "cs-conn-abc-1"));

    // 3. Session enablement
    check("session not enabled initially", !tracker.isEnabled("session-1"));
    tracker.setEnabled("session-1", true, 2 * 1024 * 1024, 512 * 1024);
    check("session enabled after setEnabled", tracker.isEnabled("session-1"));

    // 4. Target attribution
    tracker.setTargetAttribution("frame-1", "loader-1", "session-1");
    if (*attr := tracker.attributionForTarget("frame-1")) {
        check("attribution frame_id", std.mem.eql(u8, attr.getFrameId(), "frame-1"));
        check("attribution loader_id", std.mem.eql(u8, attr.getLoaderId(), "loader-1"));
        check("attribution session_id", std.mem.eql(u8, attr.getSessionId(), "session-1"));
    } else {
        check("attribution found", false);
    }

    // 5. Request lifecycle: begin -> response -> body -> finish
    tracker.recordBeginRequest(req_id, "https://api.example.com/v1/items", "GET", "Fetch", "frame-1", "loader-1");
    check("in flight count is 1", tracker.getInFlight() == 1);
    check("knows request", tracker.knowsRequest(req_id));

    tracker.recordResponseReceived(req_id, 200, "application/json", "content-type: application/json\r\n");
    tracker.recordResponseBody(req_id, "{\"items\":[1,2,3]}", false);

    if (rec := tracker.getResponseBody(req_id)) {
        check("record body matches", std.mem.eql(u8, rec.body, "{\"items\":[1,2,3]}"));
        check("record body_is_base64 is false", !rec.body_is_base64);
        check("record status is 200", rec.status == 200);
        check("record mime_type", std.mem.eql(u8, rec.mime_type, "application/json"));
    } else {
        check("getResponseBody hit", false);
    }

    tracker.recordLoadingFinished(req_id, 17);
    check("in flight count after finish is 0", tracker.getInFlight() == 0);

    // 6. Request failure flow
    let req2_buf: [64]u8 = [0; 64];
    const req2_id = tracker.nextRequestId(req2_buf[0..]);
    tracker.recordBeginRequest(req2_id, "https://api.example.com/failed", "POST", "Fetch", "frame-1", "loader-1");
    check("req2 in flight", tracker.getInFlight() == 1);
    tracker.recordLoadingFailed(req2_id, "net::ERR_CONNECTION_REFUSED", false);
    check("req2 finished in flight", tracker.getInFlight() == 0);

    // 7. MIME type and resource inference
    check("infer html", std.mem.eql(u8, inferResourceType("text/html; charset=UTF-8", ""), "Document"));
    check("infer css", std.mem.eql(u8, inferResourceType("text/css", ""), "Stylesheet"));
    check("infer js", std.mem.eql(u8, inferResourceType("application/javascript", ""), "Script"));
    check("infer json", std.mem.eql(u8, inferResourceType("application/json", ""), "Fetch"));
    check("infer png", std.mem.eql(u8, inferResourceType("image/png", ""), "Image"));
    check("infer font", std.mem.eql(u8, inferResourceType("font/woff2", ""), "Font"));
    check("infer video", std.mem.eql(u8, inferResourceType("video/mp4", ""), "Media"));
    check("infer sse", std.mem.eql(u8, inferResourceType("text/event-stream", ""), "EventSource"));
    check("infer manifest", std.mem.eql(u8, inferResourceType("application/manifest+json", ""), "Manifest"));
    check("infer with hint", std.mem.eql(u8, inferResourceType("application/octet-stream", "Image"), "Image"));

    check("text mime plain", isTextLikeMime("text/plain"));
    check("text mime html", isTextLikeMime("text/html"));
    check("text mime json", isTextLikeMime("application/json"));
    check("text mime js", isTextLikeMime("application/javascript"));
    check("text mime xml", isTextLikeMime("application/xml"));
    check("binary mime png", !isTextLikeMime("image/png"));
    check("binary mime woff2", !isTextLikeMime("font/woff2"));

    // 8. Registry operations
    registerTracker("conn-test-reg", &tracker);
    if (reg_tracker := getTracker("conn-test-reg")) {
        check("registered tracker finds body", reg_tracker.getResponseBody(req_id) != null);
    } else {
        check("getTracker returned tracker", false);
    }

    unregisterTracker("conn-test-reg");
    check("tracker unregistered", getTracker("conn-test-reg") == null);

    // 9. Attribution deletion
    tracker.deleteTargetAttribution("frame-1");
    check("attribution deleted", tracker.attributionForTarget("frame-1") == null);

    std.debug.print("All network_events tests passed!\n", .{});
}
