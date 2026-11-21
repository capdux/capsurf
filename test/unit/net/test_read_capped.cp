// tests/test_read_capped.cp
// Tests for CapSurf streaming byte budget & in-flight protection

import {
    MAX_DOCUMENT_BYTES,
    MAX_STYLESHEET_BYTES,
    MAX_CONCURRENT_READ_BYTES,
    MAX_CONNECTION_READ_BYTES,
    CappedFailure,
    ConnectionBudgetEntry,
    ReadBudgetStats,
    ReadBudgetTracker,
    ReadLease,
    CappedBuffer,
    declaredOverCap,
    cappedErrorText,
} from "../../../src/net/read_capped.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn main() {
    // 1. Standard budget constants
    check("MAX_DOCUMENT_BYTES", MAX_DOCUMENT_BYTES == 16 * 1024 * 1024);
    check("MAX_STYLESHEET_BYTES", MAX_STYLESHEET_BYTES == 8 * 1024 * 1024);
    check("MAX_CONCURRENT_READ_BYTES", MAX_CONCURRENT_READ_BYTES == 48 * 1024 * 1024);
    check("MAX_CONNECTION_READ_BYTES", MAX_CONNECTION_READ_BYTES == 24 * 1024 * 1024);

    // 2. CappedFailure enum and error text
    check("CappedFailure.None", std.mem.eql(u8, cappedErrorText(CappedFailure.None), "none"));
    check("CappedFailure.CapExceeded", std.mem.eql(u8, cappedErrorText(CappedFailure.CapExceeded), "net::ERR_FILE_TOO_BIG"));
    check("CappedFailure.BudgetExceeded", std.mem.eql(u8, cappedErrorText(CappedFailure.BudgetExceeded), "net::ERR_INSUFFICIENT_RESOURCES"));
    check("CappedFailure.IoError", std.mem.eql(u8, cappedErrorText(CappedFailure.IoError), "net::ERR_CONNECTION_CLOSED"));

    // 3. ReadBudgetTracker connection and global budget charge & refund
    let tracker = ReadBudgetTracker.init();
    check("init inFlightBytes", tracker.inFlightBytes() == 0);
    check("init peakInFlightBytes", tracker.peakInFlightBytes() == 0);

    // Charge conn1 10MB
    const ok1 = tracker.charge("conn-1", 10 * 1024 * 1024);
    check("ok1", ok1);
    check("inFlight 10MB", tracker.inFlightBytes() == 10 * 1024 * 1024);
    check("peak 10MB", tracker.peakInFlightBytes() == 10 * 1024 * 1024);
    check("conn-1 10MB", tracker.connectionInFlightBytes("conn-1") == 10 * 1024 * 1024);

    // Charge conn1 another 10MB (total 20MB, under 24MB cap)
    const ok2 = tracker.charge("conn-1", 10 * 1024 * 1024);
    check("ok2", ok2);
    check("inFlight 20MB", tracker.inFlightBytes() == 20 * 1024 * 1024);
    check("conn-1 20MB", tracker.connectionInFlightBytes("conn-1") == 20 * 1024 * 1024);

    // Charge conn1 5MB more -> would exceed 24MB connection cap, must be rejected!
    const ok3 = tracker.charge("conn-1", 5 * 1024 * 1024);
    check("ok3 rejected", !ok3);
    check("inFlight unchanged", tracker.inFlightBytes() == 20 * 1024 * 1024);

    // Charge conn2 20MB (conn2 has 20MB, global total 40MB)
    const ok4 = tracker.charge("conn-2", 20 * 1024 * 1024);
    check("ok4", ok4);
    check("inFlight 40MB", tracker.inFlightBytes() == 40 * 1024 * 1024);
    check("peak 40MB", tracker.peakInFlightBytes() == 40 * 1024 * 1024);

    // Charge conn3 10MB -> global would be 50MB, exceeds 48MB concurrent cap!
    const ok5 = tracker.charge("conn-3", 10 * 1024 * 1024);
    check("ok5 rejected", !ok5);

    // Refund conn1 15MB
    tracker.refund("conn-1", 15 * 1024 * 1024);
    check("conn-1 after refund", tracker.connectionInFlightBytes("conn-1") == 5 * 1024 * 1024);
    check("inFlight after refund", tracker.inFlightBytes() == 25 * 1024 * 1024);
    check("peak persists", tracker.peakInFlightBytes() == 40 * 1024 * 1024);

    // Now conn3 can charge 8MB (global becomes 33MB <= 48MB)
    const ok6 = tracker.charge("conn-3", 8 * 1024 * 1024);
    check("ok6", ok6);
    check("inFlight 33MB", tracker.inFlightBytes() == 33 * 1024 * 1024);

    // Reset peak
    tracker.resetPeak();
    check("reset peak", tracker.peakInFlightBytes() == 33 * 1024 * 1024);

    // Check stats
    const st = tracker.stats();
    check("stats inFlight", st.in_flight_bytes == 33 * 1024 * 1024);
    check("stats active", st.active_connections == 3);

    // 4. ReadLease RAII scoped tracking
    let tracker2 = ReadBudgetTracker.init();
    {
        let lease = tracker2.acquireLease("conn-lease", MAX_CONNECTION_READ_BYTES);
        const ok = lease.charge(5 * 1024 * 1024);
        check("lease charge ok", ok);
        check("tracker2 inFlight 5MB", tracker2.inFlightBytes() == 5 * 1024 * 1024);

        lease.release();
        check("after lease release", tracker2.inFlightBytes() == 0);
    }
    check("tracker2 empty", tracker2.inFlightBytes() == 0);

    // 5. CappedBuffer with fixed slice and dynamic allocation
    let fixed_mem: [32]u8 = [0; 32];
    let fixed_buf = CappedBuffer.fromSlice(fixed_mem[0..]);
    const r1 = fixed_buf.appendChunk("hello, ", 16);
    check("r1 None", r1 == CappedFailure.None);
    const r2 = fixed_buf.appendChunk("world!", 16);
    check("r2 None", r2 == CappedFailure.None);
    check("fixed_buf content", std.mem.eql(u8, fixed_buf.getBytes(), "hello, world!"));

    // Exceed limit
    const r3 = fixed_buf.appendChunk("123456", 16);
    check("r3 CapExceeded", r3 == CappedFailure.CapExceeded);

    // Dynamic buffer test
    let dyn_buf = CappedBuffer.init(std.heap.page_allocator);
    defer dyn_buf.deinit();

    const d1 = dyn_buf.appendChunk("Dynamic ", 64);
    check("d1 None", d1 == CappedFailure.None);
    const d2 = dyn_buf.appendChunk("Buffering!", 64);
    check("d2 None", d2 == CappedFailure.None);
    check("dyn_buf content", std.mem.eql(u8, dyn_buf.getBytes(), "Dynamic Buffering!"));

    // 6. declaredOverCap content length check
    check("over 1", declaredOverCap("1000", 500));
    check("under 2", !declaredOverCap("500", 1000));
    check("over 3", declaredOverCap("  16777217  ", 16 * 1024 * 1024));
    check("invalid 4", !declaredOverCap("not-a-number", 1000));
    check("empty 5", !declaredOverCap("", 1000));

    std.debug.print("All read_capped tests passed!\n", .{});
}
