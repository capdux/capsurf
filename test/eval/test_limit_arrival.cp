// tests/test_eval_arrival.cp
// Tests for src/eval/limit_registry.cp, src/eval/isolate_id.cp, and src/cdp/arrival.cp

import {
    LimitRegistry,
    IsolateLimitNotice,
    IsolateLimitKind,
    IsolateLimitListenerOptions,
    registerIsolateLimitListener,
    registerSimpleIsolateLimitListener,
    unregisterIsolateLimitListener,
    unregisterIsolateLimitListenersFor,
    unregisterIsolateLimitListenersForTarget,
    notifyIsolateLimit,
    isIsolateGoneError,
} from "../../src/eval/limit_registry.cp";

import {
    engineIsolateId,
    getEngineIsolateId,
    isForeignIsolate,
    resetEngineIsolateIdForTest,
    setEngineIsolateIdForTest,
} from "../../src/eval/isolate_id.cp";

import {
    markArrival,
    arrivalOf,
    peekArrival,
    removeArrival,
    clearArrivals,
    ArrivalTracker,
} from "../../src/cdp/arrival.cp";

let g_callback_called: bool = false;
let g_last_cap_ms: u64 = 0;
let g_custom_cb_called: bool = false;

fn onLimit(notice: *const IsolateLimitNotice) {
    g_callback_called = true;
    g_last_cap_ms = notice.cap_ms;
}

fn onCustomLimit(notice: *const IsolateLimitNotice, udata: *!?) {
    g_custom_cb_called = true;
    * = notice;
    * = udata;
}

fn isLiveTrue(udata: *!?): bool {
    * = udata;
    return true;
}

fn isLiveFalse(udata: *!?): bool {
    * = udata;
    return false;
}

fn testLimitRegistry() {
    // 1. isIsolateGoneError tests
    std.debug.assert(isIsolateGoneError("execution context which hosts this is gone"));
    std.debug.assert(isIsolateGoneError("EXECUTION CONTEXT WHICH HOSTS THIS"));
    std.debug.assert(isIsolateGoneError("Too many dynamic workers in pool"));
    std.debug.assert(isIsolateGoneError("TOO MANY DYNAMIC WORKERS"));
    std.debug.assert(!isIsolateGoneError("normal error message"));
    std.debug.assert(!isIsolateGoneError(""));

    // 2. Enum strings
    std.debug.assert(std.mem.eql(u8, IsolateLimitKind.cpu.asStr(), "cpu"));
    std.debug.assert(std.mem.eql(u8, IsolateLimitKind.memory.asStr(), "memory"));
    std.debug.assert(std.mem.eql(u8, IsolateLimitKind.wall.asStr(), "wall"));
    std.debug.assert(std.mem.eql(u8, IsolateLimitKind.gone.asStr(), "gone"));
    std.debug.assert(std.mem.eql(u8, IsolateLimitKind.crashed.asStr(), "crashed"));

    // 3. Register and notify one-shot
    g_callback_called = false;
    g_last_cap_ms = 0;
    registerSimpleIsolateLimitListener("script_alpha.js", onLimit, .{});
    const notice = IsolateLimitNotice{
        kind: .wall,
        cap_ms: 1234,
        from_tail: true,
        detail: "timeout detail",
    };
    notifyIsolateLimit("script_alpha.js", notice);
    std.debug.assert(g_callback_called);
    std.debug.assert(g_last_cap_ms == 1234);

    // One-shot: listener removed after notify
    g_callback_called = false;
    notifyIsolateLimit("script_alpha.js", notice);
    std.debug.assert(!g_callback_called);

    // 4. Custom callback with user_data
    g_custom_cb_called = false;
    registerIsolateLimitListener("script_beta.js", onCustomLimit, .{
        .owner = "conn-1",
        .target_id = "target-1",
        .is_live_fn = isLiveTrue,
    });
    notifyIsolateLimit("script_beta.js", notice);
    std.debug.assert(g_custom_cb_called);

    // 5. Unregister by owner
    registerSimpleIsolateLimitListener("script_c1.js", onLimit, .{ .owner = "owner-x" });
    registerSimpleIsolateLimitListener("script_c2.js", onLimit, .{ .owner = "owner-x" });
    registerSimpleIsolateLimitListener("script_d.js", onLimit, .{ .owner = "owner-y" });
    unregisterIsolateLimitListenersFor("owner-x");

    g_callback_called = false;
    notifyIsolateLimit("script_c1.js", notice);
    std.debug.assert(!g_callback_called);
    notifyIsolateLimit("script_c2.js", notice);
    std.debug.assert(!g_callback_called);
    notifyIsolateLimit("script_d.js", notice);
    std.debug.assert(g_callback_called);

    // 6. Unregister by target
    registerSimpleIsolateLimitListener("script_t1.js", onLimit, .{ .owner = "own-z", .target_id = "t-10" });
    registerSimpleIsolateLimitListener("script_t2.js", onLimit, .{ .owner = "own-z", .target_id = "t-20" });
    unregisterIsolateLimitListenersForTarget("own-z", "t-10");

    g_callback_called = false;
    notifyIsolateLimit("script_t1.js", notice);
    std.debug.assert(!g_callback_called);
    notifyIsolateLimit("script_t2.js", notice);
    std.debug.assert(g_callback_called);

    // 7. Test retirement in makeRoom
    let reg = LimitRegistry.init();
    reg.registerListener("live1.js", onCustomLimit, .{ .is_live_fn = isLiveFalse });
    std.debug.assert(reg.size() == 1);
    reg.makeRoom();
    std.debug.assert(reg.size() == 0);
}

fn testIsolateId() {
    resetEngineIsolateIdForTest();

    let id_buf: [36]u8 = [0; 36];
    const id1 = engineIsolateId(&id_buf);
    std.debug.assert(id1.len == 36);
    std.debug.assert(id1[8] == 45); // '-'
    std.debug.assert(id1[13] == 45);
    std.debug.assert(id1[18] == 45);
    std.debug.assert(id1[23] == 45);

    // Calling again returns identical ID
    const id2 = getEngineIsolateId();
    std.debug.assert(std.mem.eql(u8, id1, id2));

    // Stamped check
    std.debug.assert(!isForeignIsolate(""));
    std.debug.assert(!isForeignIsolate(id1));
    std.debug.assert(isForeignIsolate("foreign-isolate-uuid-55555"));

    // Explicit test set
    setEngineIsolateIdForTest("test-isolate-id-0000000000000000000");
    std.debug.assert(std.mem.eql(u8, getEngineIsolateId()[0..19], "test-isolate-id-000"));
}

fn testArrival() {
    clearArrivals();

    markArrival("req-101", 1000);
    markArrival("req-102", 2000);

    // Peek without updating LRU
    std.debug.assert(peekArrival("req-101") != null && peekArrival("req-101").? == 1000);
    std.debug.assert(peekArrival("req-102") != null && peekArrival("req-102").? == 2000);
    std.debug.assert(peekArrival("req-unknown") == null);

    // arrivalOf retrieval
    const a1 = arrivalOf("req-101");
    std.debug.assert(a1 != null && a1.? == 1000);

    // Update timestamp
    markArrival("req-101", 1500);
    std.debug.assert(arrivalOf("req-101").? == 1500);

    // Removal
    std.debug.assert(removeArrival("req-101"));
    std.debug.assert(arrivalOf("req-101") == null);
    std.debug.assert(!removeArrival("req-101"));

    // Clear
    clearArrivals();
    std.debug.assert(arrivalOf("req-102") == null);

    // Test LRU eviction on tracker
    let tracker = ArrivalTracker.init();
    let i: usize = 0;
    while (i < 512) : (i += 1) {
        let key_buf: [16]u8 = [0; 16];
        const s = std.fmt.bufPrint(&key_buf, "req-{}", .{i}) catch unreachable;
        tracker.mark(s, @as(i64, @intCast(i)));
    }
    std.debug.assert(tracker.size() == 512);

    // Oldest is req-0. Accessing req-1 touches it.
    * = tracker.get("req-1");

    // Adding req-512 must evict req-0
    tracker.mark("req-512", 9999);
    std.debug.assert(tracker.peek("req-0") == null);
    std.debug.assert(tracker.peek("req-1") != null);
    std.debug.assert(tracker.peek("req-512") != null);
}

pub fn main() {
    testLimitRegistry();
    testIsolateId();
    testArrival();
    std.debug.print("ALL LIMIT REGISTRY, ISOLATE ID, AND ARRIVAL TESTS PASSED!\n", .{});
}
