// test/eval/test_prefetch_scripts_and_wasm.cp
// Unit tests for src/eval/prefetch_scripts.cp and src/eval/prefetch_wasm.cp

import {
    CredentialsMode,
} from "../../src/net/cors.cp";

import {
    PREFETCH_TIMEOUT_MS,
    MAX_PREFETCH_SCRIPTS,
    OriginClassification,
    classifyScriptOrigin,
    credentialsModeToString,
    ScriptPrefetchEntry,
    ScriptBodyOut,
    PrefetchScriptManager,
} from "../../src/eval/prefetch_scripts.cp";

import {
    MAX_WASM_URLS,
    MAX_WASM_LITERAL_SPECIFIER_CHARS,
    MIN_WASM_BYTES,
    INLINE_WASM_B64_PREFIX,
    WasmBakeManifestEntry,
    wasmModuleFingerprint,
    inlineWasmKey,
    validateWasmHeader,
    scanForInlineWasm,
    scanForWasmLiterals,
} from "../../src/eval/prefetch_wasm.cp";

import {
    getSystemMilliTimestamp,
} from "../../src/eval/host_clock.cp";

fn assertTrue(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("ASSERT FAILED: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

// 1. prefetch_scripts.cp: URL resolution/extraction, origin classification, and integrity/crossorigin/type preservation
fn testScriptUrlResolutionAndPreservation() {
    assertTrue(classifyScriptOrigin("https://example.com/a.js", "") == .Invalid, "empty page_url -> Invalid");
    assertTrue(classifyScriptOrigin("https://example.com/a.js", "https://kitesurf.local/index.html") == .Synthetic, "kitesurf.local -> Synthetic");
    assertTrue(classifyScriptOrigin("https://example.com/a.js", "https://example.com/page.html") == .SameOrigin, "same origin");
    assertTrue(classifyScriptOrigin("https://cdn.example.com/a.js", "https://example.com/page.html") == .CrossOrigin, "cross origin");

    assertTrue(std.mem.eql(u8, credentialsModeToString(.Omit), "omit"), "omit string");
    assertTrue(std.mem.eql(u8, credentialsModeToString(.SameOrigin), "same-origin"), "same-origin string");
    assertTrue(std.mem.eql(u8, credentialsModeToString(.Include), "include"), "include string");

    let mgr = PrefetchScriptManager.init("https://example.com/app/index.html", "");
    assertTrue(!mgr.addScript("", false, "", "", ""), "empty src rejected");
    assertTrue(mgr.getCount() == 0, "count stays 0 after empty src");

    mgr.setBaseHref("https://cdn.example.com/v2/");
    assertTrue(
        mgr.addScript("bundle.js", false, "anonymous", "nonce-123", "sha256-deadbeef"),
        "add classic script with integrity/crossorigin/nonce",
    );
    assertTrue(
        mgr.addScript("/esm/mod.js", true, "use-credentials", "", "sha384-cafebabe"),
        "add module script with use-credentials",
    );
    assertTrue(mgr.collectScript("helper.js", false), "collectScript convenience");
    assertTrue(mgr.getCount() == 3, "count is 3");
    assertTrue(mgr.getScripts().len == 3, "getScripts slice length is 3");

    if (e0 := mgr.getScript(0)) {
        assertTrue(std.mem.eql(u8, e0.resolved_url, "https://cdn.example.com/v2/bundle.js"), "resolved against base_href");
        assertTrue(!e0.is_module, "e0 is classic script");
        assertTrue(std.mem.eql(u8, e0.cross_origin, "anonymous"), "cross_origin preserved");
        assertTrue(e0.credentials_mode == .SameOrigin, "anonymous -> SameOrigin credentials");
        assertTrue(std.mem.eql(u8, e0.nonce, "nonce-123"), "nonce preserved");
        assertTrue(std.mem.eql(u8, e0.integrity, "sha256-deadbeef"), "integrity preserved");
    } else {
        assertTrue(false, "expected script 0");
    }

    if (e1 := mgr.getScript(1)) {
        assertTrue(std.mem.eql(u8, e1.resolved_url, "https://cdn.example.com/esm/mod.js"), "root-relative resolved against base_href");
        assertTrue(e1.is_module, "e1 is module");
        assertTrue(std.mem.eql(u8, e1.cross_origin, "use-credentials"), "use-credentials preserved");
        assertTrue(e1.credentials_mode == .Include, "use-credentials -> Include credentials");
        assertTrue(std.mem.eql(u8, e1.integrity, "sha384-cafebabe"), "sha384 integrity preserved");
    } else {
        assertTrue(false, "expected script 1");
    }

    assertTrue(mgr.getScript(3) == null, "out-of-bounds getScript returns null");
}

// 2. prefetch_scripts.cp: budget/concurrency caps, CSP/timeout/capacity boundaries
fn testScriptBudgetAndConcurrencyCaps() {
    let mgr = PrefetchScriptManager.init("https://example.com/page.html", "");
    mgr.setConcurrency(0);
    assertTrue(mgr.concurrency == 1, "concurrency 0 clamps to 1");
    mgr.setConcurrency(2);
    assertTrue(mgr.concurrency == 2, "concurrency set to 2");

    mgr.setTimeoutMs(0);
    assertTrue(mgr.timeout_ms == PREFETCH_TIMEOUT_MS, "timeout 0 defaults to PREFETCH_TIMEOUT_MS");
    mgr.setTimeoutMs(5000);
    assertTrue(mgr.timeout_ms == 5000, "timeout set to 5000");

    mgr.setCspPolicy("script-src https://example.com");
    assertTrue(std.mem.eql(u8, mgr.csp_policy, "script-src https://example.com"), "csp policy set");
    mgr.setConnectionInfo("conn-1", "frame-2");
    assertTrue(std.mem.eql(u8, mgr.connection_id, "conn-1"), "connection_id set");
    assertTrue(std.mem.eql(u8, mgr.frame_id, "frame-2"), "frame_id set");

    assertTrue(mgr.isTimedOut(0.0), "start_time 0.0 is timed out");
    const future_ts = getSystemMilliTimestamp() + 60000.0;
    assertTrue(!mgr.isTimedOut(future_ts), "future start_time is not timed out");

    // Capacity cap at MAX_PREFETCH_SCRIPTS (32)
    let cap_mgr = PrefetchScriptManager.init("https://example.com/", "");
    let i: usize = 0;
    while (i < MAX_PREFETCH_SCRIPTS) : (i += 1) {
        assertTrue(cap_mgr.collectScript("item.js", false), "within MAX_PREFETCH_SCRIPTS");
    }
    assertTrue(cap_mgr.getCount() == MAX_PREFETCH_SCRIPTS, "count reaches MAX_PREFETCH_SCRIPTS");
    assertTrue(!cap_mgr.collectScript("overflow.js", false), "33rd script rejected");
    assertTrue(cap_mgr.getCount() == MAX_PREFETCH_SCRIPTS, "count remains capped at MAX_PREFETCH_SCRIPTS");
    assertTrue(cap_mgr.getScript(MAX_PREFETCH_SCRIPTS) == null, "getScript at cap returns null");
    assertTrue(cap_mgr.getScriptMut(MAX_PREFETCH_SCRIPTS) == null, "getScriptMut at cap returns null");
}

// 3. prefetch_wasm.cp: Wasm magic/size validation and manifest/key lookup
fn testWasmValidationAndManifestLookup() {
    const valid_wasm = "\x00asm\x01\x00\x00\x00\x01\x04\x01\x60\x00\x00\x03\x02\x01\x00\x0a\x04\x01\x02\x00\x0b";
    assertTrue(valid_wasm.len >= MIN_WASM_BYTES, "sample wasm >= MIN_WASM_BYTES");
    assertTrue(validateWasmHeader(valid_wasm), "valid wasm header accepted");

    const short_wasm = "\x00asm\x01\x00\x00";
    assertTrue(!validateWasmHeader(short_wasm), "7-byte header rejected");
    const bad_magic = "\x00asn\x01\x00\x00\x00";
    assertTrue(!validateWasmHeader(bad_magic), "bad magic rejected");
    const bad_version = "\x00asm\x02\x00\x00\x00";
    assertTrue(!validateWasmHeader(bad_version), "bad version rejected");

    let entry = WasmBakeManifestEntry{};
    entry.initFromBytes(valid_wasm);
    assertTrue(entry.size == valid_wasm.len, "manifest size matches");
    assertTrue(entry.getSha256().len == 64, "sha256 is 64 hex chars");
    assertTrue(entry.getHead16().len > 0, "head16 populated");
    assertTrue(entry.getTail16().len > 0, "tail16 populated");
    assertTrue(std.mem.startsWith(u8, entry.getHead16(), "AGFzbQEAAAA"), "head16 starts with wasm b64 magic");

    let empty_entry = WasmBakeManifestEntry{};
    empty_entry.initFromBytes("");
    assertTrue(empty_entry.size == 0, "empty entry size 0");
    assertTrue(empty_entry.getHead16().len == 0 && empty_entry.getTail16().len == 0, "empty head/tail");

    let key_buf: [128]u8 = [0; 128];
    const key = inlineWasmKey(entry.getSha256(), key_buf[0..]);
    assertTrue(std.mem.startsWith(u8, key, "inline-wasm:sha256:"), "inlineWasmKey prefix");
    assertTrue(key.len == 19 + 64, "inlineWasmKey total length");

    let tiny_buf: [16]u8 = [0; 16];
    const empty_key = inlineWasmKey(entry.getSha256(), tiny_buf[0..]);
    assertTrue(empty_key.len == 0, "undersized inlineWasmKey buffer returns empty");
}

// 4. prefetch_wasm.cp: Wasm URL/inline extraction, dedup, and overflow boundaries
fn testWasmScanningDedupAndOverflow() {
    // Deduplication and unquoted + quoted extraction
    const src = "fetch('a.wasm'); fetch(\"a.wasm\"); import(`b.wasm`); const x = c.wasm; load('a.wasm');";
    let urls: [8][]const u8 = [""; 8];
    const found = scanForWasmLiterals(src, urls[0..]);
    assertTrue(found == 3, "deduped 3 unique wasm literals");
    assertTrue(std.mem.eql(u8, urls[0], "a.wasm"), "url 0 is a.wasm");
    assertTrue(std.mem.eql(u8, urls[1], "b.wasm"), "url 1 is b.wasm");
    assertTrue(std.mem.eql(u8, urls[2], "c.wasm"), "url 2 is c.wasm");

    // Empty / short / zero-capacity boundaries
    assertTrue(scanForWasmLiterals(src, urls[0..0]) == 0, "zero-len out_urls returns 0");
    assertTrue(scanForWasmLiterals("a.wa", urls[0..]) == 0, "short source returns 0");

    // Output buffer smaller than match count
    let small_urls: [2][]const u8 = [""; 2];
    assertTrue(scanForWasmLiterals(src, small_urls[0..]) == 2, "capped by out_urls.len == 2");

    // MAX_WASM_URLS (8) cap even when output slice has capacity 12
    const many_src = "'1.wasm' '2.wasm' '3.wasm' '4.wasm' '5.wasm' '6.wasm' '7.wasm' '8.wasm' '9.wasm' '10.wasm'";
    let large_urls: [12][]const u8 = [""; 12];
    const capped = scanForWasmLiterals(many_src, large_urls[0..]);
    assertTrue(capped == MAX_WASM_URLS, "scanForWasmLiterals capped at MAX_WASM_URLS (8)");

    // Specifier exceeding MAX_WASM_LITERAL_SPECIFIER_CHARS (512) is skipped
    let long_lit: [530]u8 = ['x'; 530];
    long_lit[0] = '\'';
    long_lit[523] = '.';
    long_lit[524] = 'w';
    long_lit[525] = 'a';
    long_lit[526] = 's';
    long_lit[527] = 'm';
    long_lit[528] = '\'';
    long_lit[529] = ' ';
    assertTrue(long_lit.len > MAX_WASM_LITERAL_SPECIFIER_CHARS, "long specifier > 512");
    assertTrue(scanForWasmLiterals(long_lit[0..], urls[0..]) == 0, "overlong specifier ignored");

    // Inline base64 Wasm scanning & dedup across sources
    const s1 = "const m1 = 'AGFzbQEAAAABBg=='; const dup = \"AGFzbQEAAAABBg==\";";
    const s2 = "const m2 = 'AGFzbQEAAAABAg=='; const dup2 = 'AGFzbQEAAAABBg==';";
    let sources: [2][]const u8 = [s1, s2];
    let b64_out: [4][]const u8 = [""; 4];
    const b64_count = scanForInlineWasm(sources[0..], b64_out[0..]);
    assertTrue(b64_count == 2, "deduped 2 inline wasm modules across sources");
    assertTrue(std.mem.startsWith(u8, b64_out[0], INLINE_WASM_B64_PREFIX), "first inline prefix");
    assertTrue(std.mem.eql(u8, b64_out[1], "AGFzbQEAAAABAg=="), "second inline literal");
}

fn mockScriptFetch(
    url: []const u8,
    credentials_mode: CredentialsMode,
    user_data: *!?,
    out_body: *ScriptBodyOut,
    out_status: *u32,
): bool {
    * = credentials_mode;
    * = user_data;
    if (std.mem.indexOf(u8, url, "fail.js") != null) {
        out_status.* = 404;
        out_body.value = "not found";
        return false;
    }
    if (std.mem.indexOf(u8, url, "zero.js") != null) {
        out_status.* = 0;
        out_body.value = "";
        return false;
    }
    out_status.* = 200;
    out_body.value = "console.log('ok');";
    return true;
}

// 5. prefetch_scripts.cp: direct prefetchSingle & prefetchAll with custom_fetch, CSP, and timeout
fn testPrefetchSingleAndAll() {
    let mgr = PrefetchScriptManager.init("https://example.com/app/index.html", "");
    mgr.setFetchCallback(&mockScriptFetch, null);
    mgr.setCspPolicy("script-src https://example.com");

    assertTrue(mgr.addScript("/ok.js", false, "", "", ""), "add ok.js");
    assertTrue(mgr.addScript("/fail.js", false, "anonymous", "", ""), "add fail.js");
    assertTrue(mgr.addScript("/zero.js", false, "", "", ""), "add zero.js");
    assertTrue(mgr.addScript("https://evil.example.net/bad.js", false, "", "", ""), "add evil.js");

    const now = getSystemMilliTimestamp();
    assertTrue(mgr.prefetchSingle(0, now), "prefetchSingle ok.js succeeds");
    if (s0 := mgr.getScript(0)) {
        assertTrue(s0.status == 200, "ok.js status 200");
        assertTrue(std.mem.eql(u8, s0.body, "console.log('ok');"), "ok.js body populated via *[]const u8");
        assertTrue(!s0.blocked, "ok.js not blocked");
    } else {
        assertTrue(false, "expected s0");
    }

    assertTrue(!mgr.prefetchSingle(1, now), "prefetchSingle fail.js returns false");
    if (s1 := mgr.getScript(1)) {
        assertTrue(s1.status == 404, "fail.js preserves 404 status");
        assertTrue(std.mem.eql(u8, s1.body, "not found"), "fail.js body populated");
        assertTrue(std.mem.eql(u8, s1.reason, "fetch failed"), "fail.js reason");
    } else {
        assertTrue(false, "expected s1");
    }

    assertTrue(!mgr.prefetchSingle(2, now), "prefetchSingle zero.js returns false");
    if (s2 := mgr.getScript(2)) {
        assertTrue(s2.status == 500, "zero.js defaults status 0 to 500");
    } else {
        assertTrue(false, "expected s2");
    }

    assertTrue(!mgr.prefetchSingle(3, now), "prefetchSingle evil.js blocked by CSP");
    if (s3 := mgr.getScript(3)) {
        assertTrue(s3.blocked, "evil.js blocked");
        assertTrue(s3.status == 403, "evil.js status 403");
    } else {
        assertTrue(false, "expected s3");
    }

    assertTrue(!mgr.prefetchSingle(99, now), "out-of-bounds prefetchSingle returns false");
    assertTrue(!mgr.prefetchSingle(0, 0.0), "timed out prefetchSingle returns false");

    // prefetchAll across multiple scripts
    let all_mgr = PrefetchScriptManager.init("https://example.com/page.html", "");
    all_mgr.setConcurrency(2);
    all_mgr.setFetchCallback(&mockScriptFetch, null);
    assertTrue(all_mgr.addScript("/a.js", false, "", "", ""), "add a.js");
    assertTrue(all_mgr.addScript("/fail.js", false, "", "", ""), "add fail.js");
    assertTrue(all_mgr.addScript("/b.js", false, "", "", ""), "add b.js");
    const ok_count = all_mgr.prefetchAll();
    assertTrue(ok_count == 2, "prefetchAll returns 2 successes");

    if (a0 := all_mgr.getScript(0)) {
        assertTrue(a0.status == 200 && std.mem.eql(u8, a0.body, "console.log('ok');"), "a.js fetched");
    } else {
        assertTrue(false, "expected a0");
    }
    if (a2 := all_mgr.getScript(2)) {
        assertTrue(a2.status == 200 && std.mem.eql(u8, a2.body, "console.log('ok');"), "b.js fetched");
    } else {
        assertTrue(false, "expected a2");
    }
}

pub fn main() {
    testScriptUrlResolutionAndPreservation();
    testScriptBudgetAndConcurrencyCaps();
    testWasmValidationAndManifestLookup();
    testWasmScanningDedupAndOverflow();
    testPrefetchSingleAndAll();
    std.debug.print("ALL PREFETCH SCRIPTS AND WASM TESTS PASSED!\n", .{});
}
