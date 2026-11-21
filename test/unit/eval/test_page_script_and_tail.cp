// test/eval/test_page_script_and_tail.cp
// Unit tests for src/eval/page_script.cp, src/eval/page_script_tail.cp, and src/eval/page_script_rpc_session.cp

import {
    PAGE_SCRIPT_COMPAT_DATE,
    PAGE_SCRIPT_SUBREQUEST_CAP,
    PAGE_SCRIPT_CPU_TIME_MS_CAP,
    PAGE_SCRIPT_FRAME_ID_KEY_PREFIX_CHARS,
    PAGE_SCRIPT_COMPAT_FLAGS,
    PageIsolateCaps,
    PageScriptKey,
    pageIsolateCaps,
    pageScriptCacheKey,
    hostnameOf,
    originOf,
    formatRemoteObject,
    formatConsoleLogEntry,
} from "../../../src/eval/page_script.cp";

import {
    ISOLATE_KILL_MARKER,
    HUNG_INVOCATION_TEXT,
    SPAN_LOG_CAP,
    SPAN_REC_CAP,
    MAX_PAGE_SCRIPT_TAIL_LOG_LINE_CHARS,
    TailLogRecord,
    PageScriptTail,
    formatOtelRecord,
} from "../../../src/eval/page_script_tail.cp";

import {
    TEARDOWN_RPC_GRACE_MS,
    MAX_INFLIGHT_RPC,
    DEFAULT_RPC_TIMEOUT_MS,
    InflightRpcCall,
    PageScriptRpcSession,
    openPageScriptRpcSession,
} from "../../../src/eval/page_script_rpc_session.cp";

fn check(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test failure");
    }
}

fn testPageScriptCacheKeyCapsAndUrlParsing() {
    std.debug.print("1. page_script caps, cache keys, and URL parsing\n", .{});

    check(std.mem.eql(u8, PAGE_SCRIPT_COMPAT_DATE, "2026-09-11"), "compat date");
    check(PAGE_SCRIPT_SUBREQUEST_CAP == 10000, "subrequest cap");
    check(PAGE_SCRIPT_CPU_TIME_MS_CAP == 300000, "cpu time cap");
    check(PAGE_SCRIPT_FRAME_ID_KEY_PREFIX_CHARS == 8, "frame id prefix chars");
    check(PAGE_SCRIPT_COMPAT_FLAGS.len == 3, "compat flags len");
    check(std.mem.eql(u8, PAGE_SCRIPT_COMPAT_FLAGS[0], "new_module_registry"), "compat flag 0");
    check(std.mem.eql(u8, PAGE_SCRIPT_COMPAT_FLAGS[1], "no_nodejs_compat"), "compat flag 1");
    check(std.mem.eql(u8, PAGE_SCRIPT_COMPAT_FLAGS[2], "no_nodejs_compat_v2"), "compat flag 2");

    const def_caps = pageIsolateCaps(0, 0, 0);
    check(def_caps.cpu_ms == 50, "default cpu_ms 50");
    check(def_caps.subrequests == 1000, "default subrequests 1000");
    check(def_caps.memory_mb == 128, "default memory_mb 128");

    const custom_caps = pageIsolateCaps(250, 2000, 512);
    check(custom_caps.cpu_ms == 250, "custom cpu_ms");
    check(custom_caps.subrequests == 2000, "custom subrequests");
    check(custom_caps.memory_mb == 512, "custom memory_mb");

    let buf: [256]u8 = [0; 256];
    const k1 = PageScriptKey{
        origin: "https://example.com",
        key: "k1",
        frame_id: "abcdefgh1234",
        loader_id: "l1",
    };
    const ck1 = k1.getCacheKey(&buf);
    check(std.mem.eql(u8, ck1, "page-https://example.com-k1-oopif-abcdefgh"), "cache key origin+oopif truncated");

    const k2 = PageScriptKey{
        origin: "",
        key: "k2",
        frame_id: "frm1",
        loader_id: "",
    };
    const ck2 = pageScriptCacheKey(k2, &buf);
    check(std.mem.eql(u8, ck2, "page-k2-oopif-frm1"), "cache key no-origin+oopif");

    const k3 = PageScriptKey{
        origin: "https://a.test",
        key: "k3",
        frame_id: "",
        loader_id: "",
    };
    const ck3 = pageScriptCacheKey(k3, &buf);
    check(std.mem.eql(u8, ck3, "page-https://a.test-k3"), "cache key origin only");

    const k4 = PageScriptKey{
        origin: "",
        key: "k4",
        frame_id: "",
        loader_id: "",
    };
    const ck4 = pageScriptCacheKey(k4, &buf);
    check(std.mem.eql(u8, ck4, "page-k4"), "cache key key only");

    let tiny: [4]u8 = [0; 4];
    check(pageScriptCacheKey(k1, &tiny).len == 0, "cache key returns empty on tiny buffer");

    check(std.mem.eql(u8, hostnameOf("https://sub.domain.com:8080/path?q=1#frag"), "sub.domain.com"), "hostnameOf standard");
    check(std.mem.eql(u8, hostnameOf("https://user:pass@example.org:443/a"), "example.org"), "hostnameOf strips userinfo");
    check(std.mem.eql(u8, hostnameOf("http://[::1]/test"), "[::1]"), "hostnameOf ipv6");
    check(std.mem.eql(u8, hostnameOf("https://example.com./path"), "example.com"), "hostnameOf strips trailing dot");
    check(std.mem.eql(u8, hostnameOf("localhost:3000/foo"), "localhost"), "hostnameOf bare host");

    check(std.mem.eql(u8, originOf("https://a.b:8443/x?y=1#z"), "https://a.b:8443"), "originOf https");
    check(std.mem.eql(u8, originOf("http://example.com/x"), "http://example.com"), "originOf http");
    check(std.mem.eql(u8, originOf("ws://a.b/x"), ""), "originOf rejects ws");
    check(std.mem.eql(u8, originOf("https:///path"), ""), "originOf rejects empty authority");
    check(std.mem.eql(u8, originOf("https://user@example.com/path"), ""), "originOf rejects userinfo");
}

fn testPageScriptFormattingAndBufferBounds() {
    std.debug.print("2. page_script formatting and buffer bounds\n", .{});

    let buf: [256]u8 = [0; 256];
    const raw_json = "{\"type\":\"number\",\"value\":42}";
    const ro_raw = formatRemoteObject(raw_json, &buf);
    check(std.mem.eql(u8, ro_raw, raw_json), "formatRemoteObject passthrough json");

    let small_json_buf: [8]u8 = [0; 8];
    check(formatRemoteObject(raw_json, &small_json_buf).len == 0, "formatRemoteObject json overflow returns empty");

    const ro_str = formatRemoteObject("hello", &buf);
    check(std.mem.eql(u8, ro_str, "{\"type\":\"string\",\"value\":\"hello\"}"), "formatRemoteObject wraps string");

    let small_str_buf: [10]u8 = [0; 10];
    check(formatRemoteObject("hello", &small_str_buf).len == 0, "formatRemoteObject string overflow returns empty");

    const log_def = formatConsoleLogEntry("", "hi", &buf);
    check(
        std.mem.eql(u8, log_def, "{\"type\":\"log\",\"args\":[{\"type\":\"string\",\"value\":\"hi\"}],\"executionContextId\":1}"),
        "formatConsoleLogEntry default level",
    );

    const log_warn = formatConsoleLogEntry("warn", "careful", &buf);
    check(
        std.mem.eql(u8, log_warn, "{\"type\":\"warn\",\"args\":[{\"type\":\"string\",\"value\":\"careful\"}],\"executionContextId\":1}"),
        "formatConsoleLogEntry warn level",
    );

    check(formatConsoleLogEntry("warn", "careful", &small_str_buf).len == 0, "formatConsoleLogEntry overflow returns empty");
}

fn testPageScriptTailRecordsTruncationAndMarkers() {
    std.debug.print("3. page_script_tail records, truncation, capacity, and OTEL formatting\n", .{});

    check(SPAN_LOG_CAP == 100, "SPAN_LOG_CAP == 100");
    check(SPAN_REC_CAP == 250, "SPAN_REC_CAP == 250");
    check(MAX_PAGE_SCRIPT_TAIL_LOG_LINE_CHARS == 256, "MAX_PAGE_SCRIPT_TAIL_LOG_LINE_CHARS == 256");

    let rec = TailLogRecord{};
    let long_level: [24]u8 = ['L'; 24];
    let long_msg: [300]u8 = ['M'; 300];
    rec.set(&long_level, &long_msg, 12345);
    check(rec.getLevel().len == 16, "TailLogRecord truncates level to 16");
    check(rec.getMsg().len == MAX_PAGE_SCRIPT_TAIL_LOG_LINE_CHARS, "TailLogRecord truncates msg to 256");
    check(rec.t_ms == 12345, "TailLogRecord stores t_ms");

    let tail = PageScriptTail{};
    tail.addLog("info", "normal message", 10);
    check(!tail.kill_requested && !tail.hung_detected, "normal log does not set markers");
    check(tail.record_count == 1 && tail.dropped_count == 0, "record_count 1");

    tail.addLog("error", "fatal " ++ ISOLATE_KILL_MARKER ++ " triggered", 20);
    check(tail.kill_requested, "kill marker detected");

    tail.addLog("error", HUNG_INVOCATION_TEXT, 30);
    check(tail.hung_detected, "hung marker detected");

    let i: usize = 3;
    while (i < SPAN_LOG_CAP + 5) : (i += 1) {
        tail.addLog("debug", "fill", @as(u64, i));
    }
    check(tail.record_count == SPAN_LOG_CAP, "record_count capped at SPAN_LOG_CAP");
    check(tail.dropped_count == 5, "dropped_count tracks overflow");

    tail.reset();
    check(tail.record_count == 0, "reset clears record_count");
    check(tail.dropped_count == 0, "reset clears dropped_count");
    check(!tail.kill_requested && !tail.hung_detected, "reset clears markers");

    let otel_buf: [512]u8 = [0; 512];
    const otel_zero = formatOtelRecord("info", "ok", 0, &otel_buf);
    check(
        std.mem.eql(u8, otel_zero, "{\"severityText\":\"info\",\"body\":{\"stringValue\":\"ok\"},\"timeUnixNano\":\"0000000\"}"),
        "formatOtelRecord zero timestamp",
    );

    const otel_ts = formatOtelRecord("warn", "hello", 42, &otel_buf);
    check(
        std.mem.eql(u8, otel_ts, "{\"severityText\":\"warn\",\"body\":{\"stringValue\":\"hello\"},\"timeUnixNano\":\"42000000\"}"),
        "formatOtelRecord non-zero timestamp",
    );

    const otel_trunc = formatOtelRecord(&long_level, &long_msg, 1, &otel_buf);
    check(otel_trunc.len > 0, "formatOtelRecord handles oversized level/msg");
    check(std.mem.indexOf(u8, otel_trunc, "\"severityText\":\"LLLLLLLLLLLLLLLL\"") != null, "formatOtelRecord truncates level to 16");

    let tiny: [16]u8 = [0; 16];
    check(formatOtelRecord("info", "ok", 0, &tiny).len == 0, "formatOtelRecord tiny buf returns empty");

    let almost: [70]u8 = [0; 70];
    check(formatOtelRecord("info", "ok", 0, &almost).len == 0, "formatOtelRecord partial overflow returns empty");
}

fn testPageScriptRpcSessionLifecycleAndTimeouts() {
    std.debug.print("4. page_script_rpc_session IDs, capacity, timeouts, and close boundaries\n", .{});

    check(TEARDOWN_RPC_GRACE_MS == 1000, "TEARDOWN_RPC_GRACE_MS == 1000");
    check(MAX_INFLIGHT_RPC == 64, "MAX_INFLIGHT_RPC == 64");
    check(DEFAULT_RPC_TIMEOUT_MS == 15000, "DEFAULT_RPC_TIMEOUT_MS == 15000");

    let sess = openPageScriptRpcSession("rpc-sess-1");
    check(std.mem.eql(u8, sess.session_id, "rpc-sess-1"), "session_id matches");
    check(!sess.is_closed, "initially open");
    check(sess.getInflightCount() == 0, "initially 0 inflight");

    check(!sess.completeCall(0), "completeCall(0) returns false");
    check(!sess.completeCall(999), "completeCall unknown id returns false");

    const r1 = sess.trackCall("Runtime.evaluate", 0);
    check(r1 == 1, "first req id is 1");
    check(sess.getInflightCount() == 1, "inflight 1");
    check(sess.inflight[0].timeout_ms == DEFAULT_RPC_TIMEOUT_MS, "0 timeout defaults to DEFAULT_RPC_TIMEOUT_MS");

    const send_t = sess.inflight[0].send_time_ms;
    check(!sess.isCallTimedOut(r1, send_t + DEFAULT_RPC_TIMEOUT_MS), "not timed out at exact boundary");
    check(sess.isCallTimedOut(r1, send_t + DEFAULT_RPC_TIMEOUT_MS + 1), "timed out past boundary");
    check(!sess.isCallTimedOut(999, send_t + 999999), "unknown req id not timed out");

    check(sess.completeCall(r1), "completeCall r1 succeeds");
    check(!sess.completeCall(r1), "duplicate completeCall r1 returns false");
    check(sess.getInflightCount() == 0, "inflight 0 after complete");

    let i: usize = 0;
    let last_id: u64 = 0;
    while (i < MAX_INFLIGHT_RPC) : (i += 1) {
        last_id = sess.trackCall("Page.noop", 5000);
        check(last_id > 0, "tracked id positive");
    }
    check(sess.getInflightCount() == MAX_INFLIGHT_RPC, "inflight saturated at MAX_INFLIGHT_RPC");

    const overflow_id = sess.trackCall("Page.overflow", 5000);
    check(overflow_id == last_id + 1, "request_seq still increments on overflow");
    check(sess.getInflightCount() == MAX_INFLIGHT_RPC, "inflight stays bounded at MAX_INFLIGHT_RPC");

    check(sess.completeCall(last_id), "completeCall frees slot");
    check(sess.getInflightCount() == MAX_INFLIGHT_RPC - 1, "inflight decremented");

    const reuse_id = sess.trackCall("Page.reuse", 2500);
    check(reuse_id == overflow_id + 1, "reuse id incremented");
    check(sess.getInflightCount() == MAX_INFLIGHT_RPC, "freed slot reused");

    sess.finishSession("teardown");
    check(sess.is_closed, "is_closed true after finishSession");
    check(std.mem.eql(u8, sess.close_reason, "teardown"), "close_reason set");
    check(sess.getInflightCount() == 0, "all inflight cleared on finishSession");
    check(sess.trackCall("Runtime.evaluate", 1000) == 0, "trackCall returns 0 when closed");
    check(sess.getInflightCount() == 0, "inflight remains 0 when closed");
}

pub fn main() {
    std.debug.print("\n=== PageScript, Tail & RPC Session Unit Tests ===\n", .{});
    testPageScriptCacheKeyCapsAndUrlParsing();
    testPageScriptFormattingAndBufferBounds();
    testPageScriptTailRecordsTruncationAndMarkers();
    testPageScriptRpcSessionLifecycleAndTimeouts();
    std.debug.print("\nAll PageScript, Tail & RPC Session tests passed!\n", .{});
}
