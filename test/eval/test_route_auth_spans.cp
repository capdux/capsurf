// test/eval/test_route_auth_spans.cp
// Tests for Phase 21: Next.js route chunks, Web Bot Auth, span sink & carrier spans

import {
    BuildManifest,
    isManifestUrl,
    isSkipRoute,
    parseBuildManifest,
    countRouteMentions,
    rankManifestRoutes,
    RankedRoute,
    MAX_MANIFEST_ROUTES,
} from "../../src/eval/next_route_chunks.cp";

import {
    WbaEnv,
    WbaHeaders,
    WbaCache,
    formatSignatureAgent,
    isEd25519PrivateJwk,
    formatSignatureInput,
    wbaHeadersFor,
    resetWbaCaches,
    SIGNATURE_TTL_MS,
} from "../../src/eval/web_bot_auth.cp";

import {
    SpanContext,
    TailTraceRegistry,
    filterSpanRecords,
    SpanSink,
    MAX_RECORDS_PER_CALL,
    MAX_BYTES_PER_CALL,
} from "../../src/eval/span_sink.cp";

import {
    CarrierLog,
    AttachChunkedResult,
    attachChunked,
    checkAttributionTail,
    CARRIER_SPAN_BYTE_BUDGET,
    CARRIER_SPAN_LOG_CAP,
} from "../../src/eval/carrier_spans.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn testManifestUrlAndSkip() {
    std.debug.print("1. Next.js Manifest URL & Skip Routes\n", .{});
    check("valid manifest url", isManifestUrl("https://example.com/_next/static/abc123xyz/_buildManifest.js"));
    check("valid manifest url with query", isManifestUrl("/_next/static/chunks/_buildManifest.js?ts=123"));
    check("invalid manifest prefix", !isManifestUrl("/static/chunks/_buildManifest.js"));
    check("invalid manifest suffix", !isManifestUrl("/_next/static/chunks/other.js"));

    check("skip _app", isSkipRoute("/_app"));
    check("skip _error", isSkipRoute("/_error"));
    check("skip 404", isSkipRoute("/404"));
    check("skip 500", isSkipRoute("/500"));
    check("do not skip home", !isSkipRoute("/"));
    check("do not skip dashboard", !isSkipRoute("/dashboard"));
}

fn testBuildManifestParsing() {
    std.debug.print("2. Next.js Build Manifest Parser\n", .{});
    const manifest_func_src = 
        "self.__BUILD_MANIFEST = function(s, a, b) {\n" ++
        "  return {\n" ++
        "    \"/\": [s, \"static/chunks/pages/index.js\"],\n" ++
        "    \"/dashboard\": [s, a, \"static/chunks/pages/dashboard.js\"],\n" ++
        "    \"/settings\": [s, b]\n" ++
        "  };\n" ++
        "}(\"static/chunks/main.js\", \"static/chunks/chart.js\", \"static/chunks/form.js\");\n" ++
        "self.__BUILD_MANIFEST_CB && self.__BUILD_MANIFEST_CB();";

    let manifest = BuildManifest{};
    const ok = parseBuildManifest(manifest_func_src, &manifest);
    check("parseBuildManifest succeeded", ok);
    check("found 3 routes", manifest.route_count == 3);

    const home_chunks = manifest.getChunks("/");
    check("home has 2 chunks", home_chunks.len == 2);
    check("home first chunk resolved", std.mem.eql(u8, home_chunks[0], "static/chunks/main.js"));
    check("home second chunk", std.mem.eql(u8, home_chunks[1], "static/chunks/pages/index.js"));

    const dash_chunks = manifest.getChunks("/dashboard");
    check("dashboard has 3 chunks", dash_chunks.len == 3);
    check("dashboard second chunk resolved", std.mem.eql(u8, dash_chunks[1], "static/chunks/chart.js"));

    // Plain object manifest
    const manifest_obj_src = 
        "self.__BUILD_MANIFEST = {\n" ++
        "  \"/login\": [\"static/chunks/login.js\"],\n" ++
        "  \"/about\": [\"static/chunks/about.js\"]\n" ++
        "};";
    let manifest2 = BuildManifest{};
    const ok2 = parseBuildManifest(manifest_obj_src, &manifest2);
    check("parseBuildManifest obj succeeded", ok2);
    check("found 2 routes in obj form", manifest2.route_count == 2);
    const login_chunks = manifest2.getChunks("/login");
    check("login has 1 chunk", login_chunks.len == 1);
    check("login chunk name", std.mem.eql(u8, login_chunks[0], "static/chunks/login.js"));
}

fn testRouteRanking() {
    std.debug.print("3. Route Mention Ranking\n", .{});
    const src1 = "const link1 = \"/dashboard\"; const link2 = \"/settings\"; const link3 = '/dashboard';";
    const src2 = "const nav = `/_app`; const dash = \"/dashboard\"; const sett = '/settings';";
    let sources: [2][]const u8 = [src1, src2];

    const dash_count = countRouteMentions(sources[0..], "/dashboard");
    check("dashboard mentioned 3 times", dash_count == 3);
    const sett_count = countRouteMentions(sources[0..], "/settings");
    check("settings mentioned 2 times", sett_count == 2);

    let manifest = BuildManifest{};
    let r1 = manifest.addRoute("/dashboard");
    if (*entry1 := r1) {
        entry1.addChunk("static/chunks/dash.js");
    }
    let r2 = manifest.addRoute("/settings");
    if (*entry2 := r2) {
        entry2.addChunk("static/chunks/sett.js");
    }
    let r3 = manifest.addRoute("/_app");
    if (*entry3 := r3) {
        entry3.addChunk("static/chunks/app.js");
    }

    let ranked: [64]RankedRoute = [RankedRoute{}; 64];
    let ranked_count: usize = 0;
    rankManifestRoutes(&manifest, sources[0..], &ranked, &ranked_count);

    check("ranked 2 routes (skipping _app)", ranked_count == 2);
    check("first ranked is /dashboard", std.mem.eql(u8, ranked[0].route, "/dashboard"));
    check("first ranked has 3 mentions", ranked[0].mentions == 3);
    check("second ranked is /settings", std.mem.eql(u8, ranked[1].route, "/settings"));
    check("second ranked has 2 mentions", ranked[1].mentions == 2);
}

fn testWebBotAuth() {
    std.debug.print("4. Web Bot Auth Headers & Cache\n", .{});
    const valid_jwk = "{\"kty\":\"OKP\",\"crv\":\"Ed25519\",\"x\":\"11qYAYKxCrfVS_7TyWQHOg7hcvPapiMlrwIaaPcHURo\",\"d\":\"nWGxne_9WmC6hEr0kuwsxERJxWl7MmkZcDusAXYuf2A\"}";
    const invalid_jwk = "{\"kty\":\"RSA\",\"n\":\"abc\"}";

    check("valid jwk detected", isEd25519PrivateJwk(valid_jwk));
    check("invalid jwk rejected", !isEd25519PrivateJwk(invalid_jwk));

    let agent_buf: [64]u8 = [0; 64];
    const agent_hdr = formatSignatureAgent("kitesurf/1.0", agent_buf[0..]);
    check("agent formatted as JSON string", std.mem.eql(u8, agent_hdr, "\"kitesurf/1.0\""));

    let sig_in_buf: [256]u8 = [0; 256];
    const sig_in = formatSignatureInput(1700000000, 1700000300, sig_in_buf[0..]);
    check("sig_in has created", std.mem.indexOf(u8, sig_in, "created=1700000000") != null);
    check("sig_in has expires", std.mem.indexOf(u8, sig_in, "expires=1700000300") != null);
    check("sig_in has keyid", std.mem.indexOf(u8, sig_in, "keyid=\"kitesurf\"") != null);

    let env = WbaEnv{
        jwk: valid_jwk,
        signature_agent: "kitesurf/1.0",
    };

    let cache = WbaCache{};
    let headers = WbaHeaders{};
    const ok = wbaHeadersFor(env, "api.example.com", 1700000000000, &cache, &headers);
    check("wbaHeadersFor succeeded", ok);
    check("has signature", headers.signature.len > 0);
    check("has signature_input", headers.signature_input.len > 0);
    check("has signature_agent", std.mem.eql(u8, headers.signature_agent, "\"kitesurf/1.0\""));
    check("cache count is 1", cache.count == 1);

    // Second call hit cache
    let headers2 = WbaHeaders{};
    const ok2 = wbaHeadersFor(env, "api.example.com", 1700000050000, &cache, &headers2);
    check("second call hit cache", ok2);
    check("cached signature match", std.mem.eql(u8, headers.signature, headers2.signature));

    // Clear cache
    resetWbaCaches(&cache);
    check("cache cleared", cache.count == 0);
}

fn testSpanSinkAndTailRegistry() {
    std.debug.print("5. Tail Trace Registry & Span Sink\n", .{});
    let reg = TailTraceRegistry{};
    const ctx = SpanContext.create("0123456789abcdef0123456789abcdef", "fedcba9876543210");
    reg.registerTailTrace("worker-fetch", ctx);

    const lookup = reg.getTailTrace("worker-fetch");
    check("lookup succeeded", lookup != null);
    if (found := lookup) {
        check("trace_flags is 1", found.trace_flags == 1);
        check("span_id matches", std.mem.startsWith(u8, &found.span_id, "fedcba9876543210"));
    }

    const missing = reg.getTailTrace("non-existent");
    check("missing entry is null", missing == null);

    // Span filter
    let recs: [3][]const u8 = [
        "log entry 1: page navigation started",
        "log entry 2: dom tree parsed",
        "log entry 3: layout computed",
    ];
    const filt = filterSpanRecords(recs[0..]);
    check("kept all 3 records", filt.kept_count == 3);
    check("dropped count is 0", filt.dropped_count == 0);
    check("kept bytes > 0", filt.kept_bytes > 50);

    // SpanSink emit
    let sink = SpanSink{};
    const emitted = sink.emit(recs[0..], "page.render", ctx, true);
    check("sink emitted 3 records", emitted == 3);
    check("total emitted records is 3", sink.total_emitted_records == 3);

    // Emitting when tracer inactive returns 0
    const emitted_inactive = sink.emit(recs[0..], "page.render", ctx, false);
    check("inactive tracer returns 0", emitted_inactive == 0);
}

fn testCarrierSpansAndAttributionTail() {
    std.debug.print("6. Carrier Spans & Attribution Tail\n", .{});
    let logs: [4]CarrierLog = [
        CarrierLog{ key: "span.1", value: "init" },
        CarrierLog{ key: "span.2", value: "network_fetch" },
        CarrierLog{ key: "span.3", value: "js_eval" },
        CarrierLog{ key: "span.4", value: "render" },
    ];
    const res = attachChunked(logs[0..]);
    check("attached all 4 logs", res.attached == 4);
    check("oversized is 0", res.oversized == 0);
    check("chunk count is 1", res.chunks == 1);

    // Attribution tail
    const attr1 = checkAttributionTail("x-cf-trace: 123", "renderer-worker");
    check("valid attr tail is active", attr1.is_active);
    check("valid attr tail has headers", attr1.has_headers);

    const attr2 = checkAttributionTail("", "worker");
    check("empty headers is inactive", !attr2.is_active);
}

pub fn main() {
    std.debug.print("\n=== Phase 21: Next Route Chunks, Web Bot Auth & Span Sink Tests ===\n", .{});
    testManifestUrlAndSkip();
    testBuildManifestParsing();
    testRouteRanking();
    testWebBotAuth();
    testSpanSinkAndTailRegistry();
    testCarrierSpansAndAttributionTail();
    std.debug.print("\nAll Phase 21 tests passed successfully!\n", .{});
}
