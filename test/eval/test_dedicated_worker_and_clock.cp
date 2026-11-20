// test/eval/test_dedicated_worker_and_clock.cp
// Unit tests for src/eval/dedicated_worker.cp, src/eval/host_clock.cp, and src/eval/span_sink.cp

import {
    MAX_WORKERS,
    WorkerUpstreamEvent,
    WorkerState,
    WorkerUpstreamMessage,
    SpawnDedicatedWorkerOptions,
    DedicatedWorker,
    DedicatedWorkerRegistry,
    hasUseStrict,
    buildWorkerScriptModule,
    upstreamEventToString,
    parseUpstreamEvent,
} from "../../src/eval/dedicated_worker.cp";

import {
    HostClock,
    WallClock,
    getSystemMilliTimestamp,
    makeWallClock,
} from "../../src/eval/host_clock.cp";

import {
    MAX_RECORDS_PER_CALL,
    MAX_BYTES_PER_CALL,
    MAX_TAIL_TRACE_ENTRIES,
    SpanContext,
    TailTraceRegistry,
    filterSpanRecords,
    SpanSink,
} from "../../src/eval/span_sink.cp";

fn assertTrue(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("ASSERT FAILED: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

fn mockHostNow(user_data: *!?): f64 {
    if (ptr := user_data) {
        unsafe {
            let tick_ptr: *f64 = @ptrCast(@alignCast(ptr));
            return tick_ptr.*;
        }
    }
    return 0.0;
}

// 1. DedicatedWorker lifecycle, upstream messaging, and script module wrapping
fn testDedicatedWorkerLifecycleAndMessaging() {
    assertTrue(std.mem.eql(u8, upstreamEventToString(.WorkerMessage), "worker.message"), "WorkerMessage string");
    assertTrue(std.mem.eql(u8, upstreamEventToString(.WorkerError), "worker.error"), "WorkerError string");
    assertTrue(std.mem.eql(u8, upstreamEventToString(.WorkerClosed), "worker.closed"), "WorkerClosed string");

    if (ev := parseUpstreamEvent("worker.error")) {
        assertTrue(ev == .WorkerError, "parse worker.error");
    } else {
        assertTrue(false, "expected worker.error to parse");
    }
    assertTrue(parseUpstreamEvent("worker.unknown") == null, "unknown upstream event returns null");

    let msg = WorkerUpstreamMessage{};
    msg.set(.WorkerMessage, "{\"ok\":true}", 12345);
    assertTrue(msg.event == .WorkerMessage, "msg event set");
    assertTrue(std.mem.eql(u8, msg.getPayload(), "{\"ok\":true}"), "msg payload matches");
    assertTrue(msg.timestamp_ms == 12345, "msg timestamp matches");

    let worker = DedicatedWorker{};
    worker.init("wid-1", "bg-worker", 75);
    assertTrue(worker.state == .Init, "initial state is Init");
    assertTrue(std.mem.eql(u8, worker.getId(), "wid-1"), "worker id matches");
    assertTrue(std.mem.eql(u8, worker.getLabel(), "bg-worker"), "worker label matches");
    assertTrue(worker.cpu_ms == 75, "worker cpu_ms matches");

    assertTrue(!worker.deliverMessage("early"), "deliverMessage fails before start");
    assertTrue(worker.start(), "start succeeds from Init");
    assertTrue(!worker.start(), "second start returns false");
    assertTrue(worker.state == .Running, "state is Running");

    assertTrue(worker.deliverMessage("hello"), "deliverMessage succeeds in Running");
    assertTrue(worker.deliverMessage("world"), "second deliverMessage succeeds");
    assertTrue(worker.messages_sent == 2, "messages_sent is 2");

    assertTrue(worker.recordUpstream(.WorkerMessage), "record WorkerMessage");
    assertTrue(worker.recordUpstream(.WorkerError), "record WorkerError");
    assertTrue(worker.messages_received == 1, "messages_received is 1");
    assertTrue(worker.errors_count == 1, "errors_count is 1");

    assertTrue(worker.recordUpstream(.WorkerClosed), "record WorkerClosed");
    assertTrue(worker.state == .Terminated, "WorkerClosed transitions to Terminated");
    assertTrue(!worker.recordUpstream(.WorkerMessage), "recordUpstream fails after Terminated");
    assertTrue(!worker.deliverMessage("late"), "deliverMessage fails after Terminated");

    // Script module wrapping & strict mode detection
    assertTrue(hasUseStrict("  'use strict'; const x = 1;"), "single-quoted use strict detected");
    assertTrue(hasUseStrict("\"use strict\";"), "double-quoted use strict detected");
    assertTrue(!hasUseStrict("const x = 1;"), "non-strict code detected");

    let out_buf: [512]u8 = [0; 512];
    let wrapped = buildWorkerScriptModule("\"use strict\"; postMessage(1);", &out_buf);
    assertTrue(wrapped.len > 0, "wrapped module non-empty");
    assertTrue(std.mem.indexOf(u8, wrapped, "\"use strict\"; \n") != null, "strict header included");
    assertTrue(std.mem.indexOf(u8, wrapped, "postMessage(1);") != null, "body included");

    let tiny_buf: [16]u8 = [0; 16];
    let overflow = buildWorkerScriptModule("postMessage(1);", &tiny_buf);
    assertTrue(overflow.len == 0, "insufficient output buffer returns empty slice");
}

// 2. DedicatedWorkerRegistry capacity, slot reuse, and isolation bounds
fn testDedicatedWorkerRegistryCapacityAndIsolation() {
    let registry = DedicatedWorkerRegistry{};
    assertTrue(registry.activeCount() == 0, "initial activeCount is 0");
    assertTrue(registry.getWorker(0) == null, "unoccupied slot 0 returns null");
    assertTrue(registry.getWorker(MAX_WORKERS) == null, "out-of-bounds slot returns null");
    assertTrue(!registry.terminateWorker(0, "none", false), "terminate unoccupied returns false");

    let i: usize = 0;
    while (i < MAX_WORKERS) : (i += 1) {
        let idx_opt = registry.spawn(SpawnDedicatedWorkerOptions{
            connection_id: "conn",
            label: "worker",
            cpu_ms: @as(u32, @intCast(10 + i)),
        });
        assertTrue(idx_opt != null, "spawn within capacity succeeds");
    }
    assertTrue(registry.activeCount() == MAX_WORKERS, "registry at full capacity");

    let extra = registry.spawn(SpawnDedicatedWorkerOptions{
        connection_id: "overflow",
        label: "overflow",
    });
    assertTrue(extra == null, "spawn beyond MAX_WORKERS returns null");

    // Terminate middle worker with is_gone = true and verify isolation
    assertTrue(registry.terminateWorker(5, "oom", true), "terminate slot 5 succeeds");
    assertTrue(registry.activeCount() == MAX_WORKERS - 1, "activeCount decremented to 31");
    assertTrue(registry.getWorker(5) == null, "slot 5 is now unoccupied");
    assertTrue(registry.workers[5].gone, "terminated slot 5 marked gone");

    if (w4 := registry.getWorker(4)) {
        assertTrue(w4.cpu_ms == 14, "neighbor slot 4 unaffected");
    } else {
        assertTrue(false, "neighbor slot 4 should still exist");
    }

    // Re-spawn reuses freed slot 5
    let reused = registry.spawn(SpawnDedicatedWorkerOptions{
        connection_id: "conn-reused",
        label: "reused-worker",
        cpu_ms: 99,
    });
    assertTrue(reused != null && reused.? == 5, "freed slot 5 reused");
    assertTrue(registry.activeCount() == MAX_WORKERS, "activeCount restored to MAX_WORKERS");
    if (w5 := registry.getWorker(5)) {
        assertTrue(std.mem.eql(u8, w5.getId(), "conn-reused"), "reused slot has new id");
        assertTrue(!w5.gone, "reused slot reset gone flag");
    } else {
        assertTrue(false, "reused slot 5 should be occupied");
    }
}

// 3. HostClock & WallClock: system fallback, time_origin floor, and tick advancement
fn testHostClockAndWallClock() {
    let sys_t1 = getSystemMilliTimestamp();
    let sys_t2 = getSystemMilliTimestamp();
    assertTrue(sys_t1 > 0.0, "system timestamp is positive");
    assertTrue(sys_t2 >= sys_t1, "consecutive system timestamps are monotonic");

    let default_clock = makeWallClock(null);
    assertTrue(!default_clock.has_host_clock, "null host clock uses system fallback");
    assertTrue(default_clock.nowMs() > 0, "default WallClock nowMs is positive");

    let no_fn_clock = WallClock.create(5000.0, null, null);
    assertTrue(!no_fn_clock.has_host_clock, "create with null now_fn disables host hook");
    assertTrue(no_fn_clock.now() > 0.0, "create with null now_fn falls back to system clock");

    let virtual_tick: f64 = 0.0;
    let host = HostClock{
        time_origin: 1700000000000.0,
        now_fn: mockHostNow,
        user_data: &virtual_tick,
    };
    let wc = makeWallClock(host);
    assertTrue(wc.has_host_clock, "host clock hook enabled");
    assertTrue(wc.now() == 1700000000000.0, "initial clock equals time_origin floor");
    assertTrue(wc.nowMs() == 1700000000000, "initial nowMs matches time_origin");

    // Advance virtual tick
    virtual_tick = 250.75;
    assertTrue(wc.now() == 1700000000250.75, "tick advancement reflected in now()");
    assertTrue(wc.nowMs() == 1700000000250, "nowMs truncates fractional ms");

    virtual_tick = 1000.0;
    assertTrue(wc.now() == 1700000001000.0, "subsequent tick advancement");
}

// 4. SpanSink & TailTraceRegistry: recording, filtering bounds, flushing/reset, and eviction
fn testSpanSinkAndTailTraceRegistry() {
    let sink = SpanSink{};
    let sample_records: [2][]const u8 = ["span-alpha", "span-beta-123"];

    // Inactive tracer or empty records should emit nothing
    assertTrue(sink.emit(&sample_records, "log", null, false) == 0, "inactive tracer emits 0");
    let empty_records: []const []const u8 = [];
    assertTrue(sink.emit(empty_records, "log", null, true) == 0, "empty records emits 0");
    assertTrue(sink.total_emitted_records == 0 && sink.total_emitted_bytes == 0, "stats remain 0");

    // Active tracer accumulates stats
    let emitted1 = sink.emit(&sample_records, "log", null, true);
    assertTrue(emitted1 == 2, "active tracer emits 2 records");
    assertTrue(sink.total_emitted_records == 2, "total_emitted_records is 2");
    assertTrue(sink.total_emitted_bytes == 23, "total_emitted_bytes is 10 + 13");
    assertTrue(sink.total_dropped_records == 0, "no records dropped");

    sink.resetStats();
    assertTrue(sink.total_emitted_records == 0, "resetStats clears records");
    assertTrue(sink.total_emitted_bytes == 0, "resetStats clears bytes");
    assertTrue(sink.total_dropped_records == 0, "resetStats clears dropped");

    // Record count overflow bound (MAX_RECORDS_PER_CALL = 2000)
    const OVER_RECORDS = MAX_RECORDS_PER_CALL + 5;
    let many_records: [OVER_RECORDS][]const u8 = ["a"; OVER_RECORDS];
    let emitted_many = sink.emit(&many_records, "trace", null, true);
    assertTrue(emitted_many == MAX_RECORDS_PER_CALL, "capped at MAX_RECORDS_PER_CALL");
    assertTrue(sink.total_dropped_records == 5, "5 excess records dropped");

    // Byte size overflow bound (MAX_BYTES_PER_CALL = 4MB)
    const TWO_MB: usize = 2 * 1024 * 1024;
    let big_chunk: [TWO_MB]u8 = ['z'; TWO_MB];
    let big_records: [3][]const u8 = [&big_chunk, &big_chunk, &big_chunk];
    let filtered_big = filterSpanRecords(&big_records);
    assertTrue(filtered_big.kept_count == 2, "keeps 2 chunks (4MB) within byte cap");
    assertTrue(filtered_big.kept_bytes == MAX_BYTES_PER_CALL, "kept_bytes equals MAX_BYTES_PER_CALL");
    assertTrue(filtered_big.dropped_count == 1, "drops 3rd 2MB chunk");

    // TailTraceRegistry: truncation, update, eviction past 64, and clear
    let reg = TailTraceRegistry{};
    reg.registerTailTrace("", SpanContext.create("trace-1", "span-1"));
    assertTrue(reg.count == 0, "empty script_name ignored");

    let ctx1 = SpanContext.create("0123456789abcdef0123456789abcdef_EXTRA", "0123456789abcdef_EXTRA");
    reg.registerTailTrace("worker-a", ctx1);
    if (got := reg.getTailTrace("worker-a")) {
        assertTrue(std.mem.eql(u8, &got.trace_id, "0123456789abcdef0123456789abcdef"), "trace_id truncated to 32");
        assertTrue(std.mem.eql(u8, &got.span_id, "0123456789abcdef"), "span_id truncated to 16");
    } else {
        assertTrue(false, "expected worker-a trace");
    }

    // Fill registry past MAX_TAIL_TRACE_ENTRIES to trigger circular eviction of index 0 ("worker-a")
    let k: usize = 1;
    let name_buf: [32]u8 = [0; 32];
    while (k <= MAX_TAIL_TRACE_ENTRIES) : (k += 1) {
        let name = std.fmt.bufPrint(&name_buf, "script-{d}", .{k}) catch "script-x";
        reg.registerTailTrace(name, SpanContext.create("trace-x", "span-x"));
    }
    assertTrue(reg.count == MAX_TAIL_TRACE_ENTRIES, "registry count capped at 64");
    assertTrue(reg.getTailTrace("worker-a") == null, "oldest entry worker-a evicted");
    assertTrue(reg.getTailTrace("script-64") != null, "newest entry script-64 present");

    reg.clear();
    assertTrue(reg.count == 0, "clear resets count to 0");
    assertTrue(reg.getTailTrace("script-64") == null, "clear invalidates entries");
}

pub fn main() {
    testDedicatedWorkerLifecycleAndMessaging();
    testDedicatedWorkerRegistryCapacityAndIsolation();
    testHostClockAndWallClock();
    testSpanSinkAndTailTraceRegistry();
    std.debug.print("ALL DEDICATED WORKER, HOST CLOCK, AND SPAN SINK TESTS PASSED!\n", .{});
}
