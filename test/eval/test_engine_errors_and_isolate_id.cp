// test/eval/test_engine_errors_and_isolate_id.cp
// Unit tests for src/eval/engine_errors.cp, src/eval/isolate_id.cp, and src/eval/page_isolate_entry.cp

import {
    ErrorOrigin,
    TailExceptionReport,
    classifyOrigin,
    parseKsErrLine,
} from "../../src/eval/engine_errors.cp";

import {
    UUID_LEN,
    engineIsolateId,
    getEngineIsolateId,
    isForeignIsolate,
    setEngineIsolateIdForTest,
    resetEngineIsolateIdForTest,
} from "../../src/eval/isolate_id.cp";

import {
    ENTRY_MODULE,
    pageIsolateEntry,
} from "../../src/eval/page_isolate_entry.cp";

fn check(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test failure");
    }
}

fn isLowerHex(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

fn testEngineErrorsClassificationAndFormatting() {
    std.debug.print("1. engine_errors classification, formatting, and isEngineError\n", .{});

    // ErrorOrigin.asStr()
    check(std.mem.eql(u8, ErrorOrigin.page.asStr(), "page"), "asStr page");
    check(std.mem.eql(u8, ErrorOrigin.wasm.asStr(), "wasm"), "asStr wasm");
    check(std.mem.eql(u8, ErrorOrigin.shim.asStr(), "shim"), "asStr shim");
    check(std.mem.eql(u8, ErrorOrigin.runtime.asStr(), "runtime"), "asStr runtime");
    check(std.mem.eql(u8, ErrorOrigin.unknown.asStr(), "unknown"), "asStr unknown");

    // ErrorOrigin.fromStr()
    check(ErrorOrigin.fromStr("page") == .page, "fromStr page");
    check(ErrorOrigin.fromStr("wasm") == .wasm, "fromStr wasm");
    check(ErrorOrigin.fromStr("shim") == .shim, "fromStr shim");
    check(ErrorOrigin.fromStr("runtime") == .runtime, "fromStr runtime");
    check(ErrorOrigin.fromStr("unknown") == .unknown, "fromStr unknown");
    check(ErrorOrigin.fromStr("") == .unknown, "fromStr empty");
    check(ErrorOrigin.fromStr("invalid") == .unknown, "fromStr invalid");

    // TailExceptionReport.isEngineError()
    const r_wasm = TailExceptionReport{ origin: .wasm };
    const r_shim = TailExceptionReport{ origin: .shim };
    const r_runtime = TailExceptionReport{ origin: .runtime };
    const r_page = TailExceptionReport{ origin: .page };
    const r_unknown = TailExceptionReport{ origin: .unknown };
    check(r_wasm.isEngineError(), "wasm is engine error");
    check(r_shim.isEngineError(), "shim is engine error");
    check(r_runtime.isEngineError(), "runtime is engine error");
    check(!r_page.isEngineError(), "page is not engine error");
    check(!r_unknown.isEngineError(), "unknown is not engine error");

    // classifyOrigin precedence & patterns
    check(classifyOrigin("RuntimeError", "unreachable", "at __ks_user_fn (core_bg.wasm:1)") == .page, "user frame takes precedence");
    check(classifyOrigin("RuntimeError", "boom", "at foo") == .wasm, "RuntimeError -> wasm");
    check(classifyOrigin("Error", "panicked at src/lib.rs:10", "") == .wasm, "panicked at -> wasm");
    check(classifyOrigin("Error", "unreachable executed", "") == .wasm, "unreachable -> wasm");
    check(classifyOrigin("Error", "msg", "at core_bg.wasm:100:2") == .wasm, "core_bg.wasm -> wasm");
    check(classifyOrigin("Error", "msg", "at wasm-glue.js:42:1") == .wasm, "wasm-glue.js -> wasm");
    check(classifyOrigin("Error", "msg", "at dom-shim.js:15:3") == .shim, "dom-shim.js -> shim");

    // containsPageJs word-boundary & "(page.js" checks
    check(classifyOrigin("Error", "msg", "at fn (page.js:10:2)") == .runtime, "paren page.js: -> runtime");
    check(classifyOrigin("Error", "msg", "page.js:1:1") == .runtime, "start page.js: -> runtime");
    check(classifyOrigin("Error", "msg", "at fn (page.js)") == .runtime, "(page.js -> runtime");
    check(classifyOrigin("Error", "msg", "at fn (mypage.js:10:2)") == .unknown, "mypage.js: -> unknown");
    check(classifyOrigin("Error", "msg", "at fn (_page.js:10:2)") == .unknown, "_page.js: -> unknown");
    check(classifyOrigin("Error", "msg", "at fn page.js") == .unknown, "missing colon/paren -> unknown");
    check(classifyOrigin("", "", "") == .unknown, "empty inputs -> unknown");
}

fn testEngineErrorsParseKsErrLineAndBounds() {
    std.debug.print("2. engine_errors parseKsErrLine and boundary truncation\n", .{});

    // Valid full line
    const line1 = "[ks-err] {\"token\":\"tok-123\",\"name\":\"TypeError\",\"message\":\"bad \\\"val\\\"\",\"origin\":\"shim\",\"stack\":\"at dom-shim.js:1\",\"pageUrl\":\"https://a.test/p\",\"sessionId\":\"s-1\",\"scriptName\":\"sc-1\"}";
    if (rep := parseKsErrLine(line1, "tok-123")) {
        check(rep.origin == .shim, "parsed origin shim");
        check(rep.isEngineError(), "parsed report isEngineError");
        check(std.mem.eql(u8, rep.name, "TypeError"), "parsed name");
        check(std.mem.eql(u8, rep.message, "bad \\\"val\\\""), "parsed escaped message");
        check(std.mem.eql(u8, rep.stack, "at dom-shim.js:1"), "parsed stack");
        check(std.mem.eql(u8, rep.page_url, "https://a.test/p"), "parsed pageUrl");
        check(std.mem.eql(u8, rep.session_id, "s-1"), "parsed sessionId");
        check(std.mem.eql(u8, rep.script_name, "sc-1"), "parsed scriptName");
    } else {
        check(false, "expected valid parseKsErrLine");
    }

    // Snake_case fields and whitespace around colon, empty name defaults to "Error"
    const line2 = "[ks-err] {\"token\" : \"t2\", \"name\": \"\", \"message\": \"m2\", \"origin\": \"runtime\", \"page_url\": \"https://b.test\", \"session_id\": \"s-2\", \"script_name\": \"sc-2\"}";
    if (rep2 := parseKsErrLine(line2, "t2")) {
        check(rep2.origin == .runtime, "parsed origin runtime");
        check(std.mem.eql(u8, rep2.name, "Error"), "empty name defaults to Error");
        check(std.mem.eql(u8, rep2.page_url, "https://b.test"), "parsed snake page_url");
        check(std.mem.eql(u8, rep2.session_id, "s-2"), "parsed snake session_id");
        check(std.mem.eql(u8, rep2.script_name, "sc-2"), "parsed snake script_name");
    } else {
        check(false, "expected valid snake_case parseKsErrLine");
    }

    // Invalid / boundary rejections
    check(parseKsErrLine(line1, "") == null, "empty expected_token rejected");
    check(parseKsErrLine("{\"token\":\"t\",\"message\":\"m\",\"origin\":\"wasm\"}", "t") == null, "missing prefix rejected");
    check(parseKsErrLine("[ks-err] {\"token\":\"wrong\",\"message\":\"m\",\"origin\":\"wasm\"}", "t") == null, "wrong token rejected");
    check(parseKsErrLine("[ks-err] {\"message\":\"m\",\"origin\":\"wasm\"}", "t") == null, "missing token rejected");
    check(parseKsErrLine("[ks-err] {\"token\":\"t\",\"origin\":\"wasm\"}", "t") == null, "missing message rejected");
    check(parseKsErrLine("[ks-err] {\"token\":\"t\",\"message\":\"m\"}", "t") == null, "missing origin rejected");
    check(parseKsErrLine("[ks-err] {\"token\":\"t\",\"message\":\"m\",\"origin\":\"page\"}", "t") == null, "page origin rejected");
    check(parseKsErrLine("[ks-err] {\"token\":\"t\",\"message\":\"m\",\"origin\":\"unknown\"}", "t") == null, "unknown origin rejected");
    check(parseKsErrLine("[ks-err] {\"token\":\"t\",\"origin\":\"wasm\",\"message\":\"unterminated", "t") == null, "unterminated string rejected");

    // Truncation bounds: name>64, message>1024, stack>8192, pageUrl>512
    let big_buf: [10500]u8 = [0; 10500];
    let pos: usize = 0;
    const hdr = "[ks-err] {\"token\":\"t\",\"origin\":\"wasm\",\"name\":\"";
    @memcpy(big_buf[pos .. pos + hdr.len], hdr);
    pos += hdr.len;
    let i: usize = 0;
    while (i < 80) : (i += 1) {
        big_buf[pos] = 'N';
        pos += 1;
    }
    const mid1 = "\",\"message\":\"";
    @memcpy(big_buf[pos .. pos + mid1.len], mid1);
    pos += mid1.len;
    i = 0;
    while (i < 1100) : (i += 1) {
        big_buf[pos] = 'M';
        pos += 1;
    }
    const mid2 = "\",\"stack\":\"";
    @memcpy(big_buf[pos .. pos + mid2.len], mid2);
    pos += mid2.len;
    i = 0;
    while (i < 8300) : (i += 1) {
        big_buf[pos] = 'S';
        pos += 1;
    }
    const mid3 = "\",\"pageUrl\":\"";
    @memcpy(big_buf[pos .. pos + mid3.len], mid3);
    pos += mid3.len;
    i = 0;
    while (i < 600) : (i += 1) {
        big_buf[pos] = 'U';
        pos += 1;
    }
    const tail = "\"}";
    @memcpy(big_buf[pos .. pos + tail.len], tail);
    pos += tail.len;

    if (rep3 := parseKsErrLine(big_buf[0..pos], "t")) {
        check(rep3.name.len == 64, "name truncated to 64");
        check(rep3.message.len == 1024, "message truncated to 1024");
        check(rep3.stack.len == 8192, "stack truncated to 8192");
        check(rep3.page_url.len == 512, "page_url truncated to 512");
    } else {
        check(false, "expected bounded report to parse");
    }
}

fn testIsolateIdGenerationFormattingAndBoundaries() {
    std.debug.print("3. isolate_id generation, UUIDv4 format, uniqueness, and buffer bounds\n", .{});

    resetEngineIsolateIdForTest();
    let id1_buf: [64]u8 = [0; 64];
    const id1 = engineIsolateId(&id1_buf);
    check(id1.len == UUID_LEN, "UUID length is 36");
    check(std.mem.eql(u8, id1, id1_buf[0..UUID_LEN]), "engineIsolateId wrote to out_buf");

    // Validate UUID v4 format: 8-4-4-4-12 with hyphens at 8,13,18,23
    let idx: usize = 0;
    while (idx < UUID_LEN) : (idx += 1) {
        if (idx == 8 || idx == 13 || idx == 18 || idx == 23) {
            check(id1[idx] == '-', "hyphen at expected position");
        } else {
            check(isLowerHex(id1[idx]), "lowercase hex digit");
        }
    }
    check(id1[14] == '4', "RFC 4122 version 4 nibble");
    check(id1[19] == '8' || id1[19] == '9' || id1[19] == 'a' || id1[19] == 'b', "RFC 4122 variant nibble");

    // Stability on repeated calls
    const id1_again = getEngineIsolateId();
    check(std.mem.eql(u8, id1, id1_again), "getEngineIsolateId stable within isolate");

    // Small out_buf fallback (< UUID_LEN)
    let tiny_buf: [8]u8 = [0; 8];
    const id_tiny = engineIsolateId(&tiny_buf);
    check(id_tiny.len == UUID_LEN, "small out_buf still returns 36-char ID");
    check(std.mem.eql(u8, id_tiny, id1), "small out_buf returns active isolate ID");

    // Uniqueness after reset
    resetEngineIsolateIdForTest();
    const id2 = getEngineIsolateId();
    check(id2.len == UUID_LEN, "second UUID length is 36");
    check(!std.mem.eql(u8, id1, id2), "reset generates a unique isolate ID");

    // isForeignIsolate & setEngineIsolateIdForTest
    setEngineIsolateIdForTest("11111111-2222-4333-8444-555555555555");
    check(std.mem.eql(u8, getEngineIsolateId(), "11111111-2222-4333-8444-555555555555"), "setEngineIsolateIdForTest sets ID");
    check(!isForeignIsolate(""), "empty stamped header is not foreign");
    check(!isForeignIsolate("11111111-2222-4333-8444-555555555555"), "matching ID is not foreign");
    check(isForeignIsolate("11111111-2222-4333-8444-555555555556"), "different ID is foreign");
    check(isForeignIsolate("short"), "short non-empty ID is foreign");

    // Over-long ID passed to setEngineIsolateIdForTest is safely clamped to UUID_LEN
    setEngineIsolateIdForTest("aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee-extra");
    check(std.mem.eql(u8, getEngineIsolateId(), "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee"), "overlong test ID clamped to 36");

    resetEngineIsolateIdForTest();
}

fn testPageIsolateEntryValidationAndBounds() {
    std.debug.print("4. page_isolate_entry dispatch, identifier validation, and buffer bounds\n", .{});

    check(std.mem.eql(u8, ENTRY_MODULE, "__ks_entry.js"), "ENTRY_MODULE constant");

    let out_buf: [512]u8 = [0; 512];
    const expected = "export * from \"page.js\";\nexport { default } from \"page.js\";\nexport { MyHost } from \"app.js\";\n";
    const entry1 = pageIsolateEntry("MyHost", "app.js", &out_buf);
    check(std.mem.eql(u8, entry1, ENTRY_MODULE), "valid host_class_name returns ENTRY_MODULE");
    check(std.mem.eql(u8, out_buf[0..expected.len], expected), "generated entry module source matches");

    // Other valid JS identifier starts/chars (_ and $)
    check(std.mem.eql(u8, pageIsolateEntry("_Host1", "m.js", &out_buf), ENTRY_MODULE), "underscore identifier valid");
    check(std.mem.eql(u8, pageIsolateEntry("$App$0", "m.js", &out_buf), ENTRY_MODULE), "dollar identifier valid");

    // Invalid identifiers & empty inputs fall back to "page.js"
    check(std.mem.eql(u8, pageIsolateEntry("", "app.js", &out_buf), "page.js"), "empty host_class_name -> page.js");
    check(std.mem.eql(u8, pageIsolateEntry("MyHost", "", &out_buf), "page.js"), "empty first_module -> page.js");
    check(std.mem.eql(u8, pageIsolateEntry("1Host", "app.js", &out_buf), "page.js"), "digit-leading identifier -> page.js");
    check(std.mem.eql(u8, pageIsolateEntry("My-Host", "app.js", &out_buf), "page.js"), "hyphen in identifier -> page.js");
    check(std.mem.eql(u8, pageIsolateEntry("My Host", "app.js", &out_buf), "page.js"), "space in identifier -> page.js");
    check(std.mem.eql(u8, pageIsolateEntry("My.Host", "app.js", &out_buf), "page.js"), "dot in identifier -> page.js");

    // Buffer boundary checks
    check(std.mem.eql(u8, pageIsolateEntry("MyHost", "app.js", out_buf[0..0]), "page.js"), "zero-length buf -> page.js");

    let short_buf: [60]u8 = [0; 60];
    check(std.mem.eql(u8, pageIsolateEntry("MyHost", "app.js", &short_buf), "page.js"), "too-short buf (stage 1) -> page.js");

    let exact_buf: [256]u8 = [0; 256];
    check(std.mem.eql(u8, pageIsolateEntry("MyHost", "app.js", exact_buf[0 .. expected.len - 1]), "page.js"), "exact_len - 1 -> page.js");
    check(std.mem.eql(u8, pageIsolateEntry("MyHost", "app.js", exact_buf[0..expected.len]), ENTRY_MODULE), "exact_len -> ENTRY_MODULE");
    check(std.mem.eql(u8, exact_buf[0..expected.len], expected), "exact_len source matches");
}

pub fn main() {
    std.debug.print("\n=== Engine Errors, Isolate ID & Page Isolate Entry Unit Tests ===\n", .{});
    testEngineErrorsClassificationAndFormatting();
    testEngineErrorsParseKsErrLineAndBounds();
    testIsolateIdGenerationFormattingAndBoundaries();
    testPageIsolateEntryValidationAndBounds();
    std.debug.print("\nAll Engine Errors, Isolate ID & Page Isolate Entry tests passed!\n", .{});
}
