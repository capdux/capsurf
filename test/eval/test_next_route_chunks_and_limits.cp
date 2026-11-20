// test/eval/test_next_route_chunks_and_limits.cp
// Unit tests for src/eval/next_route_chunks.cp and src/eval/limit_registry.cp

import {
    MAX_ROUTE_CHUNKS,
    MAX_MANIFEST_ROUTES,
    MAX_CHUNKS_PER_ROUTE,
    MAX_TAIL_PARAMS,
    RouteEntry,
    BuildManifest,
    RankedRoute,
    isManifestUrl,
    isSkipRoute,
    parseBuildManifest,
    countRouteMentions,
    rankManifestRoutes,
} from "../../src/eval/next_route_chunks.cp";

import {
    MAX_LIMIT_LISTENERS,
    MAX_LIMIT_SCRIPT_NAME_LEN,
    MAX_LIMIT_OWNER_LEN,
    MAX_LIMIT_TARGET_LEN,
    IsolateLimitKind,
    IsolateLimitNotice,
    IsolateLimitListenerOptions,
    LimitRegistry,
    isIsolateGoneError,
} from "../../src/eval/limit_registry.cp";

fn assertTrue(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("ASSERT FAILED: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

let g_simple_calls: usize = 0;
let g_last_cap_ms: u64 = 0;
let g_last_kind: IsolateLimitKind = .wall;
let g_custom_calls: usize = 0;
let g_last_detail_len: usize = 0;

fn onSimpleLimit(notice: *const IsolateLimitNotice) {
    g_simple_calls += 1;
    g_last_cap_ms = notice.cap_ms;
    g_last_kind = notice.kind;
    g_last_detail_len = notice.detail.len;
}

fn onCustomLimit(notice: *const IsolateLimitNotice, udata: *!?) {
    * = udata;
    g_custom_calls += 1;
    g_last_cap_ms = notice.cap_ms;
    g_last_kind = notice.kind;
}

fn isLiveAlwaysTrue(udata: *!?): bool {
    * = udata;
    return true;
}

fn isLiveAlwaysFalse(udata: *!?): bool {
    * = udata;
    return false;
}

// 1. Next.js route chunk URL extraction / pattern matching / deduplication
fn testNextRouteChunkExtractionAndDedup() {
    // Manifest URL pattern matching
    assertTrue(isManifestUrl("https://example.com/_next/static/build-123/_buildManifest.js"), "standard manifest url");
    assertTrue(isManifestUrl("/_next/static/v1/_buildManifest.js"), "relative manifest url");
    assertTrue(!isManifestUrl("/_next/static//_buildManifest.js"), "empty build id rejected");
    assertTrue(!isManifestUrl("/_next/static/_buildManifest.js"), "missing build id segment rejected");
    assertTrue(!isManifestUrl("/_next/static/build-123/main.js"), "non-manifest js rejected");

    // Skip route classification
    assertTrue(isSkipRoute("/_app"), "/_app is skip route");
    assertTrue(isSkipRoute("/_error"), "/_error is skip route");
    assertTrue(isSkipRoute("/404"), "/404 is skip route");
    assertTrue(isSkipRoute("/500"), "/500 is skip route");
    assertTrue(!isSkipRoute("/"), "/ is not skip route");
    assertTrue(!isSkipRoute("/about"), "/about is not skip route");
    assertTrue(!isSkipRoute("/blog/[slug]"), "dynamic route is not skip route");

    // Function-form __BUILD_MANIFEST with tail parameters
    const fn_manifest =
        "self.__BUILD_MANIFEST = function(a, b) { return { " ++
        "\"/_app\": [\"static/chunks/app.js\"], " ++
        "\"/\": [\"static/chunks/main.js\", a], " ++
        "\"/about\": [b, \"static/css/about.css\", \"static/chunks/about.js\"] " ++
        "}; }(\"static/chunks/shared.js\", \"static/chunks/common.js\"); " ++
        "self.__BUILD_MANIFEST_CB && self.__BUILD_MANIFEST_CB();";

    let manifest = BuildManifest{};
    assertTrue(parseBuildManifest(fn_manifest, &manifest), "function manifest parses");
    assertTrue(manifest.route_count == 3, "parsed 3 routes");

    const root_chunks = manifest.getChunks("/");
    assertTrue(root_chunks.len == 2, "root has 2 chunks");
    assertTrue(std.mem.eql(u8, root_chunks[0], "static/chunks/main.js"), "root chunk 0");
    assertTrue(std.mem.eql(u8, root_chunks[1], "static/chunks/shared.js"), "root chunk 1 from tail param");

    const about_chunks = manifest.getChunks("/about");
    assertTrue(about_chunks.len == 2, "about has 2 js chunks (css filtered)");
    assertTrue(std.mem.eql(u8, about_chunks[0], "static/chunks/common.js"), "about chunk 0 from tail param");
    assertTrue(std.mem.eql(u8, about_chunks[1], "static/chunks/about.js"), "about chunk 1");

    // Object-form __BUILD_MANIFEST
    const obj_manifest = "self.__BUILD_MANIFEST = { \"/home\": [\"static/chunks/home.js\"] };";
    let obj_m = BuildManifest{};
    assertTrue(parseBuildManifest(obj_manifest, &obj_m), "object manifest parses");
    assertTrue(obj_m.getChunks("/home").len == 1, "home route has 1 chunk");

    // Route deduplication in BuildManifest.addRoute
    let dedup_m = BuildManifest{};
    const r1 = dedup_m.addRoute("/dup");
    assertTrue(r1 != null, "first addRoute succeeds");
    if (*entry := r1) {
        entry.addChunk("static/chunks/a.js");
    }
    const r2 = dedup_m.addRoute("/dup");
    assertTrue(r2 != null, "second addRoute returns existing entry");
    if (*entry := r2) {
        entry.addChunk("static/chunks/b.js");
    }
    assertTrue(dedup_m.route_count == 1, "duplicate route does not increment route_count");
    assertTrue(dedup_m.getChunks("/dup").len == 2, "chunks appended to deduplicated route");

    // Route mention counting and ranking
    const sources: [2][]const u8 = [
        "nav.push(\"/about\"); prefetch('/about?ref=1'); link(`/about#top`);",
        "goto(\"/\");",
    ];
    assertTrue(countRouteMentions(&sources, "/about") == 3, "countRouteMentions counts quoted/query/hash");
    assertTrue(countRouteMentions(&sources, "/") == 1, "countRouteMentions counts root route");

    let ranked: [MAX_MANIFEST_ROUTES]RankedRoute = [RankedRoute{}; MAX_MANIFEST_ROUTES];
    let ranked_count: usize = 0;
    rankManifestRoutes(&manifest, &sources, &ranked, &ranked_count);
    assertTrue(ranked_count == 2, "skip route /_app excluded from ranked routes");
    assertTrue(std.mem.eql(u8, ranked[0].route, "/about"), "/about ranked first");
    assertTrue(ranked[0].mentions == 3, "/about has 3 mentions");
    assertTrue(std.mem.eql(u8, ranked[1].route, "/"), "/ ranked second");
    assertTrue(ranked[1].mentions == 1, "/ has 1 mention");
}

// 2. Edge cases (empty HTML/script lists, query/hash preservation, non-chunk filtering, buffer bounds)
fn testNextRouteChunkEdgeCases() {
    // Empty and non-manifest inputs
    let empty_m = BuildManifest{};
    assertTrue(!parseBuildManifest("", &empty_m), "empty source returns false");
    assertTrue(empty_m.route_count == 0, "empty source yields 0 routes");
    assertTrue(!parseBuildManifest("console.log('hello');", &empty_m), "unrelated script returns false");
    assertTrue(empty_m.getChunks("/missing").len == 0, "missing route returns empty slice");

    // Empty script sources list
    const empty_sources: []const []const u8 = [];
    assertTrue(countRouteMentions(empty_sources, "/about") == 0, "empty sources yields 0 mentions");

    // Query and hash preservation in isManifestUrl
    assertTrue(
        isManifestUrl("https://cdn.example.com/_next/static/abc/_buildManifest.js?v=42#frag"),
        "manifest url with query and hash accepted",
    );
    assertTrue(
        !isManifestUrl("https://cdn.example.com/_next/static/abc/other.js?_buildManifest.js"),
        "non-manifest path with manifest in query rejected",
    );

    // Boundary checks in countRouteMentions: unquoted or sub-path should not match
    const edge_sources: [1][]const u8 = [
        "unquoted /blog and subpath \"/blog/post\" vs \"/blog?p=1\" and '/blog#sec'",
    ];
    assertTrue(countRouteMentions(&edge_sources, "/blog") == 2, "only exact quoted or query/hash routes match");

    // Non-chunk URL filtering (.css, .json, .svg) and malformed unclosed brackets
    const filter_src =
        "self.__BUILD_MANIFEST = { " ++
        "\"/assets\": [\"static/css/style.css\", \"static/data.json\", \"static/chunks/ok.js\", \"icon.svg\"], " ++
        "\"/broken\": [\"static/chunks/unclosed.js\"";
    let filter_m = BuildManifest{};
    assertTrue(parseBuildManifest(filter_src, &filter_m), "manifest with non-js and trailing unclosed route parses");
    const asset_chunks = filter_m.getChunks("/assets");
    assertTrue(asset_chunks.len == 1, "only .js chunk kept for /assets");
    assertTrue(std.mem.eql(u8, asset_chunks[0], "static/chunks/ok.js"), "kept chunk is ok.js");

    // Buffer bounds: MAX_CHUNKS_PER_ROUTE (16)
    let entry = RouteEntry{ route: "/many-chunks" };
    let ci: usize = 0;
    while (ci < MAX_CHUNKS_PER_ROUTE + 5) : (ci += 1) {
        entry.addChunk("static/chunks/item.js");
    }
    entry.addChunk("");
    assertTrue(entry.chunk_count == MAX_CHUNKS_PER_ROUTE, "chunk_count capped at MAX_CHUNKS_PER_ROUTE");

    // Buffer bounds: MAX_MANIFEST_ROUTES (64)
    let full_m = BuildManifest{};
    let rbufs: [MAX_MANIFEST_ROUTES][16]u8 = [[0; 16]; MAX_MANIFEST_ROUTES];
    let ri: usize = 0;
    while (ri < MAX_MANIFEST_ROUTES) : (ri += 1) {
        const rname = std.fmt.bufPrint(&rbufs[ri], "/r-{}", .{ri}) catch unreachable;
        assertTrue(full_m.addRoute(rname) != null, "route within capacity succeeds");
    }
    assertTrue(full_m.route_count == MAX_MANIFEST_ROUTES, "route_count reaches MAX_MANIFEST_ROUTES");
    assertTrue(full_m.addRoute("/overflow") == null, "65th route returns null");
    assertTrue(full_m.route_count == MAX_MANIFEST_ROUTES, "route_count remains capped");
}

// 3. LimitRegistry registration, lookup, release/eviction, and capacity limits
fn testLimitRegistryLifecycleAndCapacity() {
    assertTrue(isIsolateGoneError("Execution Context Which Hosts This was destroyed"), "case-insensitive gone match");
    assertTrue(isIsolateGoneError("too many dynamic workers"), "case-insensitive dynamic workers match");
    assertTrue(!isIsolateGoneError("timeout reached"), "unrelated error is not isolate gone");
    assertTrue(std.mem.eql(u8, IsolateLimitKind.cpu.asStr(), "cpu"), "cpu kind str");
    assertTrue(std.mem.eql(u8, IsolateLimitKind.crashed.asStr(), "crashed"), "crashed kind str");

    let reg = LimitRegistry.init();
    assertTrue(reg.size() == 0, "new registry starts empty");

    // Register, lookup, and duplicate script_name replacement
    reg.registerSimpleListener("s1.js", onSimpleLimit, .{ owner: "o1", target_id: "t1" });
    assertTrue(reg.size() == 1, "size is 1 after register");
    assertTrue(reg.hasListener("s1.js"), "hasListener finds s1.js");

    reg.registerListener("s1.js", onCustomLimit, .{ owner: "o1", target_id: "t1" });
    assertTrue(reg.size() == 1, "re-registering same script_name replaces in place");

    // One-shot notify invokes callback and removes listener
    g_custom_calls = 0;
    g_last_cap_ms = 0;
    const notice = IsolateLimitNotice{
        kind: .memory,
        cap_ms: 5000,
        from_tail: true,
        detail: "oom",
    };
    assertTrue(reg.notifyLimit("s1.js", notice), "notifyLimit returns true for active listener");
    assertTrue(g_custom_calls == 1, "custom callback invoked once");
    assertTrue(g_last_cap_ms == 5000, "cap_ms propagated");
    assertTrue(g_last_kind == .memory, "kind propagated");
    assertTrue(!reg.hasListener("s1.js"), "listener removed after one-shot notify");
    assertTrue(reg.size() == 0, "size returns to 0 after notify");
    assertTrue(!reg.notifyLimit("s1.js", notice), "second notify returns false");

    // Explicit unregister
    reg.registerSimpleListener("s2.js", onSimpleLimit, .{});
    assertTrue(reg.unregisterListener("s2.js"), "unregisterListener removes s2.js");
    assertTrue(!reg.unregisterListener("s2.js"), "second unregister returns false");

    // Retirement via is_live_fn in makeRoom
    reg.registerListener("dead.js", onCustomLimit, .{ is_live_fn: isLiveAlwaysFalse });
    reg.registerListener("alive.js", onCustomLimit, .{ is_live_fn: isLiveAlwaysTrue });
    assertTrue(reg.size() == 2, "two listeners registered before makeRoom");
    reg.makeRoom();
    assertTrue(reg.size() == 1, "retired listener reclaimed by makeRoom");
    assertTrue(!reg.hasListener("dead.js"), "dead.js evicted");
    assertTrue(reg.hasListener("alive.js"), "alive.js retained");
    reg.clear();
    assertTrue(reg.size() == 0, "clear resets size to 0");

    // Capacity limit (64) and oldest-seq eviction
    let i: usize = 0;
    while (i < MAX_LIMIT_LISTENERS) : (i += 1) {
        let name_buf: [24]u8 = [0; 24];
        const sname = std.fmt.bufPrint(&name_buf, "cap-{}.js", .{i}) catch unreachable;
        reg.registerSimpleListener(sname, onSimpleLimit, .{ is_live_fn: isLiveAlwaysTrue });
    }
    assertTrue(reg.size() == MAX_LIMIT_LISTENERS, "registry filled to MAX_LIMIT_LISTENERS");
    assertTrue(reg.hasListener("cap-0.js"), "oldest cap-0.js present before overflow");

    reg.registerSimpleListener("cap-64.js", onSimpleLimit, .{ is_live_fn: isLiveAlwaysTrue });
    assertTrue(reg.size() == MAX_LIMIT_LISTENERS, "size stays capped at MAX_LIMIT_LISTENERS");
    assertTrue(!reg.hasListener("cap-0.js"), "oldest entry cap-0.js evicted on overflow");
    assertTrue(reg.hasListener("cap-1.js"), "cap-1.js retained");
    assertTrue(reg.hasListener("cap-64.js"), "new entry cap-64.js present");
}

// 4. Multi-context isolation and invalid ID / boundary handling
fn testLimitRegistryIsolationAndBoundaries() {
    // Independent registry instances do not share state
    let ctx_a = LimitRegistry.init();
    let ctx_b = LimitRegistry.init();

    ctx_a.registerSimpleListener("shared-name.js", onSimpleLimit, .{ owner: "ctx-a", target_id: "tab-1" });
    assertTrue(ctx_a.hasListener("shared-name.js"), "ctx_a has listener");
    assertTrue(!ctx_b.hasListener("shared-name.js"), "ctx_b is isolated from ctx_a");

    ctx_b.registerSimpleListener("shared-name.js", onSimpleLimit, .{ owner: "ctx-b", target_id: "tab-1" });
    assertTrue(ctx_a.unregisterListener("shared-name.js"), "unregister on ctx_a succeeds");
    assertTrue(ctx_b.hasListener("shared-name.js"), "ctx_b listener unaffected by ctx_a unregister");

    // Owner and target_id isolation within a single registry
    let shared = LimitRegistry.init();
    shared.registerSimpleListener("a.js", onSimpleLimit, .{ owner: "owner-1", target_id: "target-1" });
    shared.registerSimpleListener("b.js", onSimpleLimit, .{ owner: "owner-1", target_id: "target-2" });
    shared.registerSimpleListener("c.js", onSimpleLimit, .{ owner: "owner-2", target_id: "target-1" });

    assertTrue(shared.unregisterForTarget("owner-1", "target-1") == 1, "unregisterForTarget removes only exact pair");
    assertTrue(!shared.hasListener("a.js"), "a.js removed");
    assertTrue(shared.hasListener("b.js"), "b.js (different target) preserved");
    assertTrue(shared.hasListener("c.js"), "c.js (different owner) preserved");

    assertTrue(shared.unregisterForOwner("owner-1") == 1, "unregisterForOwner removes remaining owner-1 entry");
    assertTrue(!shared.hasListener("b.js"), "b.js removed by owner");
    assertTrue(shared.hasListener("c.js"), "c.js preserved for owner-2");

    // Invalid / unknown / empty ID boundary handling
    assertTrue(!shared.hasListener(""), "empty script name not present");
    assertTrue(!shared.unregisterListener(""), "unregister empty script name returns false");
    assertTrue(!shared.notifyLimit("nonexistent.js", IsolateLimitNotice{}), "notify unknown script returns false");
    assertTrue(shared.unregisterForOwner("unknown-owner") == 0, "unregisterForOwner unknown returns 0");
    assertTrue(shared.unregisterForTarget("owner-2", "unknown-target") == 0, "unregisterForTarget unknown returns 0");

    // Over-length script_name / owner / target_id truncation safety
    const LONG_SCRIPT_LEN = MAX_LIMIT_SCRIPT_NAME_LEN + 32;
    const LONG_OWNER_LEN = MAX_LIMIT_OWNER_LEN + 16;
    const LONG_TARGET_LEN = MAX_LIMIT_TARGET_LEN + 16;
    let long_script: [LONG_SCRIPT_LEN]u8 = ['x'; LONG_SCRIPT_LEN];
    let long_owner: [LONG_OWNER_LEN]u8 = ['o'; LONG_OWNER_LEN];
    let long_target: [LONG_TARGET_LEN]u8 = ['t'; LONG_TARGET_LEN];

    shared.registerSimpleListener(&long_script, onSimpleLimit, .{
        owner: &long_owner,
        target_id: &long_target,
    });
    assertTrue(shared.hasListener(long_script[0..MAX_LIMIT_SCRIPT_NAME_LEN]), "truncated script_name is lookup-safe");
    assertTrue(
        shared.unregisterForTarget(long_owner[0..MAX_LIMIT_OWNER_LEN], long_target[0..MAX_LIMIT_TARGET_LEN]) == 1,
        "truncated owner and target_id match cleanly",
    );
}

pub fn main() {
    testNextRouteChunkExtractionAndDedup();
    testNextRouteChunkEdgeCases();
    testLimitRegistryLifecycleAndCapacity();
    testLimitRegistryIsolationAndBoundaries();
    std.debug.print("ALL NEXT ROUTE CHUNKS AND LIMIT REGISTRY TESTS PASSED!\n", .{});
}
