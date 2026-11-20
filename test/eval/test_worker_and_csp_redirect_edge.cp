// test/eval/test_worker_and_csp_redirect_edge.cp
// Unit tests for DedicatedWorkerRegistry slot allocation/release/capacity limits,
// /__fixtures/worker-hop/* redirect routing & hit-counter boundaries, and CSP redirect edge cases.

import {
    MAX_WORKERS,
    WorkerUpstreamEvent,
    WorkerState,
    SpawnDedicatedWorkerOptions,
    DedicatedWorker,
    DedicatedWorkerRegistry,
    buildWorkerScriptModule,
} from "../../src/eval/dedicated_worker.cp";

import { MockFixtures } from "../../src/net/mock_fixtures.cp";
import { parseCsp, isExternalScriptAllowed, isSourceAllowed } from "../../src/net/csp.cp";
import { isCrossOrigin, isProtocolDowngrade } from "../../src/net/redirect_walk.cp";

fn check(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("ASSERT FAILED: {s}\n", .{msg});
        @panic("test failure");
    }
}

fn testWorkerSlotAllocationReleaseAndCapacity() {
    std.debug.print("1. Worker slot allocation, release, reuse, and MAX_WORKERS capacity bounds\n", .{});

    let reg = DedicatedWorkerRegistry{};
    check(reg.activeCount() == 0, "initial activeCount is 0");
    check(reg.getWorker(0) == null, "empty slot 0 returns null");
    check(reg.getWorker(MAX_WORKERS) == null, "out-of-bounds slot returns null");
    check(!reg.terminateWorker(0, "empty", false), "terminate unoccupied slot 0 returns false");
    check(!reg.terminateWorker(MAX_WORKERS, "oob", false), "terminate out-of-bounds slot returns false");

    // Fill all MAX_WORKERS slots
    let i: usize = 0;
    while (i < MAX_WORKERS) : (i += 1) {
        const slot = reg.spawn(SpawnDedicatedWorkerOptions{
            connection_id: "c",
            label: "w",
            cpu_ms: @as(u32, @intCast(20 + i)),
        });
        check(slot != null, "spawn within MAX_WORKERS succeeds");
        check(slot.? == i, "slots allocated sequentially from lowest free index");
    }
    check(reg.activeCount() == MAX_WORKERS, "activeCount reaches MAX_WORKERS");

    // Overflow beyond MAX_WORKERS
    const overflow = reg.spawn(SpawnDedicatedWorkerOptions{
        connection_id: "overflow",
        label: "overflow",
    });
    check(overflow == null, "spawn beyond MAX_WORKERS returns null");
    check(reg.activeCount() == MAX_WORKERS, "activeCount unchanged after overflow");

    // Release head (0), middle (15), and tail (MAX_WORKERS - 1) slots
    check(reg.terminateWorker(0, "done-head", false), "release head slot 0");
    check(!reg.terminateWorker(0, "double-free", true), "double-terminate slot 0 returns false");
    check(reg.terminateWorker(15, "gone-mid", true), "release middle slot 15 with is_gone=true");
    check(reg.terminateWorker(MAX_WORKERS - 1, "done-tail", false), "release tail slot");

    check(reg.activeCount() == MAX_WORKERS - 3, "activeCount decremented by 3");
    check(reg.getWorker(0) == null, "released slot 0 is null");
    check(reg.getWorker(15) == null, "released slot 15 is null");
    check(reg.getWorker(MAX_WORKERS - 1) == null, "released tail slot is null");
    check(reg.workers[15].gone, "slot 15 retains gone=true before reuse");
    check(!reg.workers[0].gone, "slot 0 retains gone=false");

    // Re-allocate in lowest-free-index order (0 -> 15 -> MAX_WORKERS - 1)
    const r0 = reg.spawn(SpawnDedicatedWorkerOptions{ connection_id: "r0", label: "re0", cpu_ms: 100 });
    const r15 = reg.spawn(SpawnDedicatedWorkerOptions{ connection_id: "r15", label: "re15", cpu_ms: 101 });
    const rtail = reg.spawn(SpawnDedicatedWorkerOptions{ connection_id: "rtail", label: "retail", cpu_ms: 102 });
    check(r0 != null && r0.? == 0, "reuses lowest freed slot 0 first");
    check(r15 != null && r15.? == 15, "reuses next freed slot 15");
    check(rtail != null && rtail.? == MAX_WORKERS - 1, "reuses tail freed slot");
    check(reg.activeCount() == MAX_WORKERS, "activeCount restored to MAX_WORKERS");

    if (w15 := reg.getWorker(15)) {
        check(std.mem.eql(u8, w15.getId(), "r15"), "reused slot 15 has new id");
        check(!w15.gone, "reused slot 15 resets gone flag");
        check(w15.state == .Init, "reused slot 15 starts in Init state");
        check(w15.start(), "reused slot 15 transitions Init -> Running");
        check(w15.deliverMessage("ping"), "deliverMessage works on Running reused slot");
        check(w15.recordUpstream(.WorkerClosed), "WorkerClosed transitions to Terminated");
        check(!w15.deliverMessage("after-close"), "deliverMessage fails after WorkerClosed");
    } else {
        check(false, "reused slot 15 must be occupied");
    }

    // Verify full capacity again
    check(reg.spawn(SpawnDedicatedWorkerOptions{ connection_id: "x" }) == null, "full again after reuse");
}

fn testWorkerHopRedirectRoutingEdge() {
    std.debug.print("2. /__fixtures/worker-hop/* redirect routing & hit-counter boundaries\n", .{});

    let buf: [512]u8 = [0; 512];

    // Basic /__fixtures/worker-hop/<key>.js?query stripping
    if (res := MockFixtures.handleFixtureRequest(
        "https://a.test/__fixtures/worker-hop/alpha.js?cb=123",
        "https://a.test",
        "CapSurfTest",
        &buf,
    )) {
        check(std.mem.eql(u8, res, "postMessage(\"hop:alpha\");"), "worker-hop strips ?query and .js suffix");
    } else {
        check(false, "worker-hop alpha.js should return script");
    }

    // Second hit without .js suffix increments same key "alpha"
    if (res2 := MockFixtures.handleFixtureRequest(
        "https://a.test/__fixtures/worker-hop/alpha",
        "https://a.test",
        "CapSurfTest",
        &buf,
    )) {
        check(std.mem.eql(u8, res2, "postMessage(\"hop:alpha\");"), "worker-hop without .js returns same script");
    } else {
        check(false, "worker-hop alpha should return script");
    }

    // Query hit counter for "alpha" -> 2
    if (hits_alpha := MockFixtures.handleFixtureRequest(
        "https://a.test/__fixtures/worker-hop-hits/alpha?t=9",
        "https://a.test",
        "CapSurfTest",
        &buf,
    )) {
        check(std.mem.eql(u8, hits_alpha, "{\"hits\":2}"), "worker-hop-hits strips query and reports 2 hits for alpha");
    } else {
        check(false, "worker-hop-hits alpha should return json");
    }

    // Unknown key reports 0 hits
    if (hits_miss := MockFixtures.handleFixtureRequest(
        "https://a.test/__fixtures/worker-hop-hits/never-seen",
        "https://a.test",
        "CapSurfTest",
        &buf,
    )) {
        check(std.mem.eql(u8, hits_miss, "{\"hits\":0}"), "unseen worker-hop-hits key reports 0");
    } else {
        check(false, "worker-hop-hits unseen should return json");
    }

    // Empty key edge: /__fixtures/worker-hop/ and /__fixtures/worker-hop/.js
    if (empty_res := MockFixtures.handleFixtureRequest(
        "https://a.test/__fixtures/worker-hop/.js",
        "https://a.test",
        "CapSurfTest",
        &buf,
    )) {
        check(std.mem.eql(u8, empty_res, "postMessage(\"hop:\");"), "worker-hop/.js produces empty key body");
    } else {
        check(false, "worker-hop/.js should return script");
    }
    if (empty_hits := MockFixtures.handleFixtureRequest(
        "https://a.test/__fixtures/worker-hop-hits/",
        "https://a.test",
        "CapSurfTest",
        &buf,
    )) {
        check(std.mem.eql(u8, empty_hits, "{\"hits\":0}"), "empty key is not recorded in hop slots");
    } else {
        check(false, "worker-hop-hits/ should return json");
    }

    // /__fixtures/worker-hop-back with and without key param
    if (back_no_key := MockFixtures.handleFixtureRequest(
        "https://a.test/__fixtures/worker-hop-back",
        "https://a.test",
        "CapSurfTest",
        &buf,
    )) {
        check(back_no_key.len == 0, "worker-hop-back without key returns empty body");
    } else {
        check(false, "worker-hop-back without key should return non-null empty slice");
    }

    if (back_with_key := MockFixtures.handleFixtureRequest(
        "https://a.test/__fixtures/worker-hop-back?foo=1&key=hop1",
        "https://a.test",
        "CapSurfTest",
        &buf,
    )) {
        check(back_with_key.len == 0, "worker-hop-back?key=hop1 returns empty body");
    } else {
        check(false, "worker-hop-back?key=hop1 should return non-null empty slice");
    }

    if (back_hits := MockFixtures.handleFixtureRequest(
        "https://a.test/__fixtures/worker-hop-hits/back-hop1",
        "https://a.test",
        "CapSurfTest",
        &buf,
    )) {
        check(std.mem.eql(u8, back_hits, "{\"hits\":1}"), "worker-hop-back increments back-<key> counter");
    } else {
        check(false, "worker-hop-hits/back-hop1 should return json");
    }

    // Small output buffer boundary returns null instead of overflowing
    let tiny_buf: [8]u8 = [0; 8];
    const tiny_res = MockFixtures.handleFixtureRequest(
        "https://a.test/__fixtures/worker-hop/tiny",
        "https://a.test",
        "CapSurfTest",
        &tiny_buf,
    );
    check(tiny_res == null, "worker-hop returns null when out_buf is too small");

    // Fill remaining MAX_WORKER_HOP_SLOTS (32 total; 2 used so far: alpha, back-hop1)
    let k: usize = 0;
    let url_buf: [128]u8 = [0; 128];
    while (k < 35) : (k += 1) {
        const u = std.fmt.bufPrint(&url_buf, "https://a.test/__fixtures/worker-hop/slot-{d}.js", .{ k }) catch "";
        * = MockFixtures.handleFixtureRequest(u, "https://a.test", "CapSurfTest", &buf);
    }
    // Slot 35 exceeds MAX_WORKER_HOP_SLOTS (32), so it is not persisted
    if (overflow_hits := MockFixtures.handleFixtureRequest(
        "https://a.test/__fixtures/worker-hop-hits/slot-34",
        "https://a.test",
        "CapSurfTest",
        &buf,
    )) {
        check(std.mem.eql(u8, overflow_hits, "{\"hits\":0}"), "keys beyond MAX_WORKER_HOP_SLOTS are not stored");
    } else {
        check(false, "worker-hop-hits slot-34 should return json");
    }
    // Existing slot ("alpha") still increments cleanly
    * = MockFixtures.handleFixtureRequest("https://a.test/__fixtures/worker-hop/alpha", "https://a.test", "CapSurfTest", &buf);
    if (alpha_after := MockFixtures.handleFixtureRequest(
        "https://a.test/__fixtures/worker-hop-hits/alpha",
        "https://a.test",
        "CapSurfTest",
        &buf,
    )) {
        check(std.mem.eql(u8, alpha_after, "{\"hits\":3}"), "existing slot still increments when table is full");
    } else {
        check(false, "worker-hop-hits alpha after full should return json");
    }
}

fn testCspRedirectAndOriginEdges() {
    std.debug.print("3. CSP redirect path-stripping & cross-origin hop boundaries\n", .{});

    const csp = parseCsp("script-src https://cdn.test/v1/worker.js 'self'");
    check(isExternalScriptAllowed(&csp, "https://app.test", "https://cdn.test/v1/worker.js", "", false, true), "exact path allowed pre-redirect");
    check(!isExternalScriptAllowed(&csp, "https://app.test", "https://cdn.test/v2/other.js", "", false, true), "different path blocked pre-redirect");
    check(isExternalScriptAllowed(&csp, "https://app.test", "https://cdn.test/v2/other.js", "", true, true), "redirect hop ignores source path on allowed host");
    check(!isExternalScriptAllowed(&csp, "https://app.test", "https://evil.test/v1/worker.js", "", true, true), "redirect hop still blocks unlisted host");
    check(isSourceAllowed(&csp, "script-src", "https://cdn.test/v1/worker.js"), "isSourceAllowed matches exact host+path");

    check(isCrossOrigin("https://app.test/page", "https://cdn.test/worker.js"), "worker hop to different host is cross-origin");
    check(!isCrossOrigin("https://app.test/page", "https://app.test:443/__fixtures/worker-hop/a.js"), "same host with default 443 is same-origin");
    check(isProtocolDowngrade("https://app.test/w.js", "http://app.test/w.js"), "https -> http hop is protocol downgrade");
}

pub fn main() {
    std.debug.print("\n=== Worker Slot & Redirect Edge Unit Tests ===\n", .{});
    testWorkerSlotAllocationReleaseAndCapacity();
    testWorkerHopRedirectRoutingEdge();
    testCspRedirectAndOriginEdges();
    std.debug.print("\nAll Worker & CSP redirect edge tests passed!\n", .{});
}
