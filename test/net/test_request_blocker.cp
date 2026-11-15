// test/net/test_request_blocker.cp
// CapSurf Request Blocker Unit Tests (1:1 aligned with e2e/ref/request-blocker.test.ts)

import { test, expect_eq } from "cap:test";
import {
    ResourceType,
    BlockableRequest,
    RequestBlockVerdict,
    RequestBlocker,
    parseResourceType,
    classifyRequest,
    registerRequestBlocker,
    unregisterRequestBlocker,
    getRequestBlocker,
} from "../../src/net/request_blocker.cp";

pub struct TestTrackerContext {
    seen_reqs: [8]BlockableRequest = [BlockableRequest{} ; 8],
    seen_count: usize = 0,
}

fn trackerClassify(ctx: *void?, req: *const BlockableRequest): RequestBlockVerdict? {
    if (ctx_ptr := ctx) {
        let t: *TestTrackerContext = @ptrCast(@alignCast(ctx_ptr));
        if (t.seen_count < t.seen_reqs.len) {
            t.seen_reqs[t.seen_count] = req.*;
            t.seen_count += 1;
        }
    }
    if (std.mem.eql(u8, req.url_host, "tracker.example")) {
        return RequestBlockVerdict {
            matched: "tracker.example",
            reason: "request blocklist matched (tracker.example)",
        };
    }
    return null;
}

fn main() {
    test("1. classifyRequest passes normalized request facts to the host blocker", () => {
        let tracker = TestTrackerContext{};
        let blocker = RequestBlocker {
            ctx: @ptrCast(&tracker),
            classify_fn: trackerClassify,
        };

        const verdict1 = classifyRequest(
            &blocker,
            "https://Tracker.Example./a.js",
            "WWW.Site.Example.",
            ResourceType.Script,
        );
        expect_eq(verdict1 == null, true);
        expect_eq(tracker.seen_count, 1);
        expect_eq(std.mem.eql(u8, tracker.seen_reqs[0].page_host, "www.site.example"), true);
        expect_eq(tracker.seen_reqs[0].third_party, true);
        expect_eq(tracker.seen_reqs[0].resource_type == ResourceType.Script, true);

        const verdict2 = classifyRequest(
            &blocker,
            "https://tracker.example/a.js",
            "site.example",
            ResourceType.Image,
        );
        expect_eq(verdict2 != null, true);
        expect_eq(std.mem.eql(u8, verdict2.?.matched, "tracker.example"), true);
        expect_eq(std.mem.eql(u8, verdict2.?.reason, "request blocklist matched (tracker.example)"), true);
    });

    test("2. Marks same-site requests as first party and opaque pages as third party", () => {
        let tracker = TestTrackerContext{};
        let blocker = RequestBlocker {
            ctx: @ptrCast(&tracker),
            classify_fn: trackerClassify,
        };

        * = classifyRequest(&blocker, "https://cdn.site.example/x.js", "www.site.example", ResourceType.Script);
        * = classifyRequest(&blocker, "https://cdn.site.co.uk/x.js", "www.site.co.uk", ResourceType.Script);
        * = classifyRequest(&blocker, "https://cdn.site.example/x.js", "", ResourceType.Script);

        expect_eq(tracker.seen_count, 3);
        expect_eq(tracker.seen_reqs[0].third_party, false);
        expect_eq(tracker.seen_reqs[1].third_party, false);
        expect_eq(tracker.seen_reqs[2].third_party, true);
    });

    test("3. Never blocks without a blocker, for non-http schemes or unparseable URLs", () => {
        let tracker = TestTrackerContext{};
        let blocker = RequestBlocker {
            ctx: @ptrCast(&tracker),
            classify_fn: trackerClassify,
        };

        expect_eq(classifyRequest(null, "https://tracker.example/", "site.example", ResourceType.Script) == null, true);
        expect_eq(classifyRequest(&blocker, "data:text/plain,hi", "site.example", ResourceType.Script) == null, true);
        expect_eq(classifyRequest(&blocker, "blob:https://tracker.example/1", "site.example", ResourceType.Script) == null, true);
        expect_eq(classifyRequest(&blocker, "not a url", "site.example", ResourceType.Script) == null, true);
        expect_eq(tracker.seen_count, 0);
    });

    test("4. Request blocker registry scopes blockers to their connection", () => {
        let tracker = TestTrackerContext{};
        let blocker = RequestBlocker {
            ctx: @ptrCast(&tracker),
            classify_fn: trackerClassify,
        };

        registerRequestBlocker(std.heap.page_allocator, "conn-a", &blocker);
        expect_eq(getRequestBlocker("conn-a") != null, true);
        expect_eq(getRequestBlocker("conn-b") == null, true);

        unregisterRequestBlocker("conn-a");
        expect_eq(getRequestBlocker("conn-a") == null, true);
    });

    test("5. parseResourceType accepts CDP names and maps anything else to Other", () => {
        expect_eq(parseResourceType("Fetch") == ResourceType.Fetch, true);
        expect_eq(parseResourceType("Document") == ResourceType.Document, true);
        expect_eq(parseResourceType("fetch") == ResourceType.Other, true);
        expect_eq(parseResourceType("") == ResourceType.Other, true);
    });
}
