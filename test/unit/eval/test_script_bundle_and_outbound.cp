// test/eval/test_script_bundle_and_outbound.cp
// White-box unit tests for script_bundle.cp, learned_scripts.cp,
// external_outbound.cp, and denied_outbound.cp.

import {
    INLINE_SCRIPT_BODY_KEY_PREFIX_CHARS,
    MAX_CLASSIC_SOURCE_MAP_LINES,
    MAX_BUNDLE_DESCRIPTORS,
    ScriptKind,
    ScriptDescriptorEntry,
    ScriptBundle,
    countLines,
    encodeVLQ,
    commentSafe,
    categorizeScript,
    buildScriptBundle,
    isOneOf,
    isNonEmptyString,
} from "../../../src/eval/script_bundle.cp";

import {
    MIN_LEARNED_BYTES,
    MAX_URLS_PER_ORIGIN,
    MAX_ORIGINS,
    MAX_IDLE_NAVIGATES,
    InsertedRun,
    LearnedScriptsStore,
    learnedOriginOf,
    hasQuery,
    storeKey,
    recordInsertedScript,
    forgetLearnedScript,
    clearLearnedScripts,
    learnedScriptsFor,
} from "../../../src/eval/learned_scripts.cp";

import {
    ExternalOutboundRequest,
    ExternalOutboundResponse,
    ExternalOutbound,
    HostOutboundConfig,
    defaultExternalFetch,
    directExternalFetch,
} from "../../../src/eval/external_outbound.cp";

import {
    DENIED_BLOCKED_REASON,
    DENIED_STATUS_CODE,
    DENIED_BODY,
    DeniedOutbound,
    makeDeniedOutboundResponse,
} from "../../../src/eval/denied_outbound.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn mockCustomFetch(req: *const ExternalOutboundRequest, user_data: *!?): ExternalOutboundResponse {
    if (ud := user_data) {
        unsafe {
            const hits: *usize = @ptrCast(@alignCast(ud));
            hits.* += 1;
        }
    }
    if (std.mem.startsWith(u8, req.url, "https://blocked.example/")) {
        return ExternalOutboundResponse{
            status: 403,
            status_text: "Forbidden",
            headers: "x-mock: blocked",
            body: "denied by hook",
            ok: false,
        };
    }
    return ExternalOutboundResponse{
        status: 201,
        status_text: "Created",
        headers: "x-mock: ok",
        body: req.method,
        ok: true,
    };
}

fn testScriptBundle() {
    std.debug.print("1. script_bundle categorization, bundle summary, VLQ & guards\n", .{});
    check("constants sanity", INLINE_SCRIPT_BODY_KEY_PREFIX_CHARS == 120 && MAX_CLASSIC_SOURCE_MAP_LINES == 50000 && MAX_BUNDLE_DESCRIPTORS == 64);

    // Categorization rules
    const d_reg = ScriptDescriptorEntry{ src: "a.js", body: "" };
    const d_inline_async = ScriptDescriptorEntry{ src: "", body: "console.log(1)", async_flag: true };
    const d_defer = ScriptDescriptorEntry{ src: "defer.js", defer_flag: true };
    const d_async = ScriptDescriptorEntry{ src: "async.js", async_flag: true };
    const d_mod = ScriptDescriptorEntry{ src: "mod.mjs", kind: ScriptKind.module_script, defer_flag: true };
    const d_inj = ScriptDescriptorEntry{ src: "inj.js", injected_flag: true, async_flag: true };

    check("regular external", categorizeScript(d_reg) == ScriptKind.regular);
    check("inline async without src is regular", categorizeScript(d_inline_async) == ScriptKind.regular);
    check("deferred with src", categorizeScript(d_defer) == ScriptKind.deferred);
    check("async with src", categorizeScript(d_async) == ScriptKind.async_script);
    check("module overrides defer", categorizeScript(d_mod) == ScriptKind.module_script);
    check("injected overrides async", categorizeScript(d_inj) == ScriptKind.injected);

    const descs: [6]ScriptDescriptorEntry = [d_reg, d_inline_async, d_defer, d_async, d_mod, d_inj];
    let bundle = ScriptBundle{};
    buildScriptBundle(descs[0..], &bundle);
    check("total_scripts == 6", bundle.total_scripts == 6);
    check("regular_count == 2", bundle.regular_count == 2);
    check("deferred_count == 1", bundle.deferred_count == 1);
    check("async_count == 1", bundle.async_count == 1);
    check("module_count == 1", bundle.module_count == 1);
    check("injected_count == 1", bundle.injected_count == 1);

    // Rebuild with empty slice resets counters
    buildScriptBundle(descs[0..0], &bundle);
    check("reset total_scripts == 0", bundle.total_scripts == 0);
    check("reset regular_count == 0", bundle.regular_count == 0);

    // countLines, encodeVLQ, commentSafe
    check("countLines 0", countLines("single line") == 0);
    check("countLines 2", countLines("a\nb\nc") == 2);

    let vlq_buf: [16]u8 = [0; 16];
    check("VLQ(0) == A", std.mem.eql(u8, encodeVLQ(0, vlq_buf[0..]), "A"));
    check("VLQ(-1) == D", std.mem.eql(u8, encodeVLQ(-1, vlq_buf[0..]), "D"));
    check("VLQ(16) == gB", std.mem.eql(u8, encodeVLQ(16, vlq_buf[0..]), "gB"));

    let cs_buf: [32]u8 = [0; 32];
    const safe = commentSafe("a\nb\rc", cs_buf[0..]);
    check("commentSafe escapes CR/LF", std.mem.eql(u8, safe, "a%0Ab%0Dc"));

    let small_buf: [3]u8 = [0; 3];
    const trunc = commentSafe("x\ny", small_buf[0..]);
    check("commentSafe respects buffer budget", std.mem.eql(u8, trunc, "x"));

    const allowed: [2][]const u8 = ["GET", "POST"];
    check("isOneOf hit", isOneOf("POST", allowed[0..]));
    check("isOneOf miss", !isOneOf("DELETE", allowed[0..]));
    check("isNonEmptyString", isNonEmptyString("ok") && !isNonEmptyString(""));
}

fn testLearnedScriptsRecordAndHit() {
    std.debug.print("2. learned_scripts record, query-string gate, dedup & hit\n", .{});
    // Helper checks
    if (org := learnedOriginOf("https://app.example.com:8443/path/index.html?x=1")) {
        check("originOf extracts scheme+host+port", std.mem.eql(u8, org, "https://app.example.com:8443"));
    } else {
        check("originOf should succeed", false);
    }
    check("originOf rejects userinfo", learnedOriginOf("https://user@example.com/a.js") == null);
    check("originOf rejects file scheme", learnedOriginOf("file:///tmp/a.js") == null);
    check("hasQuery checks", hasQuery("https://a.com/x.js?v=1") && !hasQuery("https://a.com/x.js?#frag"));

    let key_buf: [128]u8 = [0; 128];
    if (k := storeKey(key_buf[0..], "https://app.example.com/p", "ctx-1")) {
        check("storeKey format", std.mem.eql(u8, k, "ctx-1|https://app.example.com"));
    } else {
        check("storeKey should succeed", false);
    }

    let store = LearnedScriptsStore.init();
    const page = "https://app.example.com/home";
    const s_small = "https://cdn.example.com/small.js";
    const s_main = "https://cdn.example.com/main.js";
    const s_query = "https://cdn.example.com/chunk.js?v=42";

    // Below MIN_LEARNED_BYTES -> ignored
    recordInsertedScript(&store, page, s_small, MIN_LEARNED_BYTES - 1, InsertedRun.boa, "ctx-1");
    check("small script not learned", learnedScriptsFor(&store, page, "ctx-1").len == 0);

    // Baked run on unknown script -> ignored
    recordInsertedScript(&store, page, s_main, MIN_LEARNED_BYTES, InsertedRun.baked, "ctx-1");
    check("baked on unlearned script ignored", learnedScriptsFor(&store, page, "ctx-1").len == 0);

    // Boa run >= MIN_LEARNED_BYTES -> learned
    recordInsertedScript(&store, page, s_main, MIN_LEARNED_BYTES, InsertedRun.boa, "ctx-1");
    // Duplicate boa record -> deduped
    recordInsertedScript(&store, page, s_main, MIN_LEARNED_BYTES + 100, InsertedRun.boa, "ctx-1");

    const hits1 = learnedScriptsFor(&store, page, "ctx-1");
    check("1 learned script hit", hits1.len == 1);
    check("learned script url matches", std.mem.eql(u8, hits1[0], s_main));

    // Query-string script requires 2 boa observations before promotion
    recordInsertedScript(&store, page, s_query, MIN_LEARNED_BYTES, InsertedRun.boa, "ctx-1");
    const hits2 = learnedScriptsFor(&store, page, "ctx-1");
    check("query script not yet promoted on 1st boa", hits2.len == 1);

    recordInsertedScript(&store, page, s_query, MIN_LEARNED_BYTES, InsertedRun.boa, "ctx-1");
    const hits3 = learnedScriptsFor(&store, page, "ctx-1");
    check("query script promoted on 2nd boa", hits3.len == 2);
}

fn testLearnedScriptsEvictionAndStale() {
    std.debug.print("3. learned_scripts idle eviction, stale refusal, LRU cap & clear\n", .{});
    let store = LearnedScriptsStore.init();
    const page = "https://evict.example.com/index.html";
    const s1 = "https://cdn.example.com/one.js";
    const s2 = "https://cdn.example.com/two.js";

    recordInsertedScript(&store, page, s1, MIN_LEARNED_BYTES, InsertedRun.boa, "ctx-a");
    // MAX_IDLE_NAVIGATES == 3: calls 1, 2, 3 return s1 and increment idle 0->1->2->3
    let nav: u32 = 0;
    while (nav < MAX_IDLE_NAVIGATES) : (nav += 1) {
        const res = learnedScriptsFor(&store, page, "ctx-a");
        check("still alive before idle limit", res.len == 1);
    }
    // 4th call sees idle >= 3 and evicts s1
    check("evicted after MAX_IDLE_NAVIGATES", learnedScriptsFor(&store, page, "ctx-a").len == 0);

    // Stale marks script refused so subsequent boa does not re-learn it
    recordInsertedScript(&store, page, s2, MIN_LEARNED_BYTES, InsertedRun.boa, "ctx-a");
    check("s2 learned", learnedScriptsFor(&store, page, "ctx-a").len == 1);
    recordInsertedScript(&store, page, s2, MIN_LEARNED_BYTES, InsertedRun.stale, "ctx-a");
    check("s2 removed on stale", learnedScriptsFor(&store, page, "ctx-a").len == 0);
    recordInsertedScript(&store, page, s2, MIN_LEARNED_BYTES, InsertedRun.boa, "ctx-a");
    check("s2 stays refused after stale", learnedScriptsFor(&store, page, "ctx-a").len == 0);

    // Per-origin URL capacity eviction (MAX_URLS_PER_ORIGIN == 24)
    let url_buf: [64]u8 = [0; 64];
    let idx: usize = 0;
    while (idx < MAX_URLS_PER_ORIGIN + 2) : (idx += 1) {
        const u = std.fmt.bufPrint(url_buf[0..], "https://cdn.example.com/u{d}.js", .{idx}) catch "";
        recordInsertedScript(&store, page, u, MIN_LEARNED_BYTES, InsertedRun.boa, "ctx-cap");
    }
    const cap_res = learnedScriptsFor(&store, page, "ctx-cap");
    check("capped at MAX_URLS_PER_ORIGIN", cap_res.len == MAX_URLS_PER_ORIGIN);
    // Oldest u0.js and u1.js were evicted; first remaining is u2.js
    check("oldest evicted, u2.js retained", std.mem.eql(u8, cap_res[0], "https://cdn.example.com/u2.js"));

    // forgetLearnedScript & clearLearnedScripts context isolation
    forgetLearnedScript(&store, page, "https://cdn.example.com/u2.js", "ctx-cap");
    check("forgetLearnedScript removes u2.js", learnedScriptsFor(&store, page, "ctx-cap").len == MAX_URLS_PER_ORIGIN - 1);

    recordInsertedScript(&store, page, s1, MIN_LEARNED_BYTES, InsertedRun.boa, "ctx-keep");
    clearLearnedScripts(&store, "ctx-cap");
    check("clearLearnedScripts clears ctx-cap", learnedScriptsFor(&store, page, "ctx-cap").len == 0);
    check("ctx-keep remains intact", learnedScriptsFor(&store, page, "ctx-keep").len == 1);
}

fn testExternalAndDeniedOutbound() {
    std.debug.print("4. external_outbound & denied_outbound decision, hook stats & 403\n", .{});
    const req_ok = ExternalOutboundRequest{
        url: "https://api.example.com/data",
        method: "POST",
    };
    const req_blocked = ExternalOutboundRequest{
        url: "https://blocked.example/secret",
        method: "GET",
    };

    // Default outbound without custom hook
    let ext = ExternalOutbound{};
    const def_resp = ext.fetch(&req_ok);
    check("default fetch 200 ok", def_resp.status == 200 && def_resp.ok);

    // Custom hook with user_data call counter
    let hit_count: usize = 0;
    let cfg = HostOutboundConfig.init();
    check("unconfigured initially", !cfg.isConfigured());
    check("defaultExternalFetch 200 ok", defaultExternalFetch(&req_ok).status == 200);

    cfg.setCustomFetch(mockCustomFetch, unsafe { @ptrCast(&hit_count) });
    check("configured after setCustomFetch", cfg.isConfigured());

    const r1 = cfg.fetch(&req_ok);
    check("custom fetch returns 201 with method", r1.status == 201 && r1.ok && std.mem.eql(u8, r1.body, "POST"));

    let cfg2 = HostOutboundConfig.withFetch(mockCustomFetch, unsafe { @ptrCast(&hit_count) });
    const r2 = cfg2.fetch(&req_blocked);
    check("withFetch invokes hook 403", r2.status == 403 && !r2.ok);
    check("outbound hook hit_count == 2", hit_count == 2);

    // Redirect policy checks
    check("302 allowed at hop 0", cfg.shouldFollowRedirect(302, 0));
    check("308 allowed at hop 19", cfg.shouldFollowRedirect(308, 19));
    check("hop 20 exceeds max_redirects", !cfg.shouldFollowRedirect(301, 20));
    check("200 is not a redirect", !cfg.shouldFollowRedirect(200, 0));
    cfg.setRedirectPolicy(false, 5);
    check("redirect disabled rejects 302", !cfg.shouldFollowRedirect(302, 0));

    // DeniedOutbound 403 response checks
    const d_resp = DeniedOutbound.getResponse();
    check("DeniedOutboundResponse status 403", d_resp.status == DENIED_STATUS_CODE && d_resp.status == 403);
    check("DeniedOutboundResponse reason", std.mem.eql(u8, d_resp.blocked_reason, DENIED_BLOCKED_REASON));

    let http_buf: [512]u8 = [0; 512];
    const raw_http = DeniedOutbound.fetch("https://evil.example/exfil", http_buf[0..]);
    check("HTTP 403 status line", std.mem.startsWith(u8, raw_http, "HTTP/1.1 403 Forbidden\r\n"));
    check("x-kitesurf-blocked header present", std.mem.indexOf(u8, raw_http, "x-kitesurf-blocked: renderer-no-egress") != null);
    check("denied body present", std.mem.endsWith(u8, raw_http, DENIED_BODY));

    let tiny_buf: [8]u8 = [0; 8];
    const empty_http = makeDeniedOutboundResponse("https://evil.example/", tiny_buf[0..]);
    check("small buffer returns empty on overflow", empty_http.len == 0);
}

pub fn main() {
    testScriptBundle();
    testLearnedScriptsRecordAndHit();
    testLearnedScriptsEvictionAndStale();
    testExternalAndDeniedOutbound();
    std.debug.print("ALL SCRIPT_BUNDLE & OUTBOUND TESTS PASSED!\n", .{});
}
