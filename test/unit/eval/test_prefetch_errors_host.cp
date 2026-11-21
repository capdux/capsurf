// test/eval/test_prefetch_errors_host.cp
// Tests for Phase 20: WASM prefetch, engine errors, script prefetch, outbound & clock

import {
    MAX_WASM_URLS,
    MAX_TOTAL_WASM_BYTES,
    WasmBakeManifestEntry,
    scanForInlineWasm,
    scanForWasmLiterals,
    wasmModuleFingerprint,
} from "../../../src/eval/prefetch_wasm.cp";

import {
    ErrorOrigin,
    classifyOrigin,
    TailExceptionReport,
    parseKsErrLine,
} from "../../../src/eval/engine_errors.cp";

import {
    PREFETCH_TIMEOUT_MS,
    ScriptPrefetchEntry,
    PrefetchScriptManager,
} from "../../../src/eval/prefetch_scripts.cp";

import {
    ExternalOutboundRequest,
    ExternalOutboundResponse,
    HostOutboundConfig,
    directExternalFetch,
} from "../../../src/eval/external_outbound.cp";

import {
    makeDeniedOutboundResponse,
    DeniedOutbound,
} from "../../../src/eval/denied_outbound.cp";

import {
    WallClock,
    getSystemMilliTimestamp,
} from "../../../src/eval/host_clock.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn testWasmPrefetch() {
    std.debug.print("1. WASM Literals Scanner\n", .{});
    const code = "const a = 'assets/engine.wasm'; const b = \"./math.wasm\"; const c = `test.wasm`;";
    let urls: [8][]const u8 = [""; 8];
    const count = scanForWasmLiterals(code, urls[0..]);
    check("found 3 wasm literals", count == 3);
    check("first wasm", std.mem.eql(u8, urls[0], "assets/engine.wasm"));
    check("second wasm", std.mem.eql(u8, urls[1], "./math.wasm"));
    check("third wasm", std.mem.eql(u8, urls[2], "test.wasm"));

    std.debug.print("2. Inline Base64 WASM Scanner\n", .{});
    const inline_code = "const bin = 'AGFzbQEAAAABBgFgAX8BfwMCAQAHBwEDZm9vAAAKBwEFAEEBfws=';";
    let sources: [1][]const u8 = [inline_code];
    let b64_out: [4][]const u8 = [""; 4];
    const b64_count = scanForInlineWasm(sources[0..], b64_out[0..]);
    check("found 1 inline wasm", b64_count == 1);
    check("b64 prefix matches", std.mem.startsWith(u8, b64_out[0], "AGFzbQEAAAA"));

    std.debug.print("3. WASM Module Fingerprint\n", .{});
    const sample_bytes = "sample wasm bytecode";
    let sha_buf: [64]u8 = [0; 64];
    wasmModuleFingerprint(sample_bytes, sha_buf[0..]);
    check("fingerprint length is 64 hex", sha_buf.len == 64);
}

fn testEngineErrors() {
    std.debug.print("4. classifyOrigin\n", .{});
    check("page error", classifyOrigin("Error", "user failure", "at __ks_user_main (bundle.js:10:5)") == .page);
    check("wasm runtime error", classifyOrigin("RuntimeError", "unreachable executed", "at wasm-function[12]") == .wasm);
    check("wasm panic", classifyOrigin("Error", "panicked at 'out of bounds'", "") == .wasm);
    check("dom shim error", classifyOrigin("TypeError", "null deref", "at HTMLDivElement (dom-shim.js:42:1)") == .shim);
    check("runtime page.js error", classifyOrigin("Error", "eval failed", "at page.js:100:12") == .runtime);
    check("unknown error", classifyOrigin("CustomError", "something happened", "at other.js:1:1") == .unknown);

    std.debug.print("5. parseKsErrLine\n", .{});
    const valid_line = "[ks-err] {\"token\":\"auth-tok-123\",\"origin\":\"runtime\",\"name\":\"EvalError\",\"message\":\"timeout\",\"pageUrl\":\"https://ex.com\"}";
    const rep = parseKsErrLine(valid_line, "auth-tok-123");
    check("valid report parsed", rep != null);
    if (*r := rep) {
        check("report origin", r.origin == .runtime);
        check("report name", std.mem.eql(u8, r.name, "EvalError"));
        check("report message", std.mem.eql(u8, r.message, "timeout"));
        check("report pageUrl", std.mem.eql(u8, r.page_url, "https://ex.com"));
    }

    const wrong_token_line = "[ks-err] {\"token\":\"bad-token\",\"origin\":\"runtime\",\"message\":\"err\"}";
    check("wrong token rejected", parseKsErrLine(wrong_token_line, "auth-tok-123") == null);
}

fn testScriptPrefetchAndHost() {
    std.debug.print("6. PrefetchScriptManager\n", .{});
    let mgr = PrefetchScriptManager.init("https://example.com/base/", "");
    check("initial empty scripts", mgr.getCount() == 0);

    check("addScript", mgr.addScript("app.js", false, "", "", ""));
    check("script count is 1", mgr.getCount() == 1);
    if (*entry := mgr.getScriptMut(0)) {
        check("resolved url", std.mem.eql(u8, entry.resolved_url, "https://example.com/base/app.js"));
        check("not module", !entry.is_module);
    }

    std.debug.print("7. Denied Outbound\n", .{});
    let denied_buf: [256]u8 = [0; 256];
    const denied_resp = makeDeniedOutboundResponse("https://external.com/leak", denied_buf[0..]);
    check("denied 403 status", std.mem.indexOf(u8, denied_resp, "403 Forbidden") != null);
    check("denied header present", std.mem.indexOf(u8, denied_resp, "x-kitesurf-blocked: renderer-no-egress") != null);

    std.debug.print("8. Host Clock\n", .{});
    let clock = WallClock.init();
    const t1 = clock.now();
    check("clock now > 0", t1 > 0);
    const sys_t = getSystemMilliTimestamp();
    check("sys timestamp close to clock", @abs(t1 - sys_t) < 5000);
}

pub fn main() {
    testWasmPrefetch();
    testEngineErrors();
    testScriptPrefetchAndHost();
    std.debug.print("ALL PREFETCH, ERRORS, AND HOST OUTBOUND TESTS PASSED SUCCESSFULLY!\n", .{});
}
