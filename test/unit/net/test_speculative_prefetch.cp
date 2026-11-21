// test/net/test_speculative_prefetch.cp
// Tests for Speculative Prefetch Coordinator and Learned Scripts Cache

import {
    SPECULATIVE_DEADLINE_MS,
    SPECULATIVE_CONCURRENCY,
    PrefetchSignal,
    PrefetchResult,
    PrefetchBatchRunner,
    maySpeculate,
    registerInterception,
    unregisterInterception,
    registerOutboundFilters,
    unregisterOutboundFilters,
    OutboundFilters,
} from "../../../src/net/speculative_prefetch.cp";
import { FetchInterceptionState } from "../../../src/cdp/fetch_interception.cp";
import {
    MIN_LEARNED_BYTES,
    MAX_URLS_PER_ORIGIN,
    MAX_ORIGINS,
    MAX_IDLE_NAVIGATES,
    InsertedRun,
    LearnedScriptsStore,
} from "../../../src/eval/learned_scripts.cp";

fn expect(cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed!\n", .{});
        @panic("assertion failure in test_speculative_prefetch");
    }
}

fn dummyFetch(url: []const u8, signal: *const PrefetchSignal, user_data: *!?): PrefetchResult? {
    * = user_data;
    if (signal.isAborted()) return null;
    return PrefetchResult{
        url: url,
        status_code: 200,
        body: "console.log('prefetch');",
        success: true,
    };
}

fn testSpeculativePrefetchCoordinator() {
    std.debug.print("1. maySpeculate with no filters\n", .{});
    expect(maySpeculate("conn-1", "https://example.com/app.js"));

    std.debug.print("2. maySpeculate with interception route\n", .{});
    {
        let st = FetchInterceptionState{};
        st.addPattern("https://example.com/*.js", "Script", "Request");
        registerInterception("conn-intercept", st);
        expect(!maySpeculate("conn-intercept", "https://example.com/app.js"));
        expect(maySpeculate("conn-intercept", "https://other.com/app.js"));
        unregisterInterception("conn-intercept");
        expect(maySpeculate("conn-intercept", "https://example.com/app.js"));
    }

    std.debug.print("3. maySpeculate with outbound filter\n", .{});
    {
        let filters = OutboundFilters{};
        filters.addRejectPattern("https://blocked.com/*");
        registerOutboundFilters("conn-outbound", filters);
        expect(!maySpeculate("conn-outbound", "https://blocked.com/script.js"));
        expect(maySpeculate("conn-outbound", "https://allowed.com/script.js"));
        unregisterOutboundFilters("conn-outbound");
        expect(maySpeculate("conn-outbound", "https://blocked.com/script.js"));
    }

    std.debug.print("4. PrefetchBatchRunner best-effort execution\n", .{});
    {
        let runner = PrefetchBatchRunner.init(SPECULATIVE_DEADLINE_MS, SPECULATIVE_CONCURRENCY);
        expect(runner.addItem("https://example.com/1.js"));
        expect(runner.addItem("https://example.com/2.js"));
        expect(runner.addItem("https://example.com/3.js"));

        const results = runner.run(dummyFetch, null);
        expect(results.len == 3);
        expect(std.mem.eql(u8, results[0].url, "https://example.com/1.js"));
        expect(std.mem.eql(u8, results[1].url, "https://example.com/2.js"));
        expect(std.mem.eql(u8, results[2].url, "https://example.com/3.js"));
    }
}

fn testLearnedScriptsStore() {
    std.debug.print("5. LearnedScriptsStore size threshold\n", .{});
    let store = LearnedScriptsStore{};

    // Script under 8KB should NOT be learned
    store.recordInsertedScript(
        "https://example.com/index.html",
        "https://example.com/small.js",
        MIN_LEARNED_BYTES - 1,
        InsertedRun.boa,
        "ctx-1",
    );
    {
        const learned = store.learnedScriptsFor("https://example.com/index.html", "ctx-1");
        expect(learned.len == 0);
    }

    // Script >= 8KB should be learned
    store.recordInsertedScript(
        "https://example.com/index.html",
        "https://example.com/bundle.js",
        MIN_LEARNED_BYTES + 100,
        InsertedRun.boa,
        "ctx-1",
    );
    {
        const learned = store.learnedScriptsFor("https://example.com/index.html", "ctx-1");
        expect(learned.len == 1);
        expect(std.mem.eql(u8, learned[0], "https://example.com/bundle.js"));
    }

    std.debug.print("6. LearnedScriptsStore idle navigates expiration\n", .{});
    // Idle counter increments on each learnedScriptsFor query
    // Query 1 was at idle = 0 -> now idle = 1
    // Query 2: idle = 1 -> now idle = 2
    {
        const learned = store.learnedScriptsFor("https://example.com/index.html", "ctx-1");
        expect(learned.len == 1);
    }
    // Query 3: idle = 2 -> now idle = 3
    {
        const learned = store.learnedScriptsFor("https://example.com/index.html", "ctx-1");
        expect(learned.len == 1);
    }
    // Query 4: idle was 3 >= MAX_IDLE_NAVIGATES (3) -> dropped
    {
        const learned = store.learnedScriptsFor("https://example.com/index.html", "ctx-1");
        expect(learned.len == 0);
    }

    std.debug.print("7. LearnedScriptsStore stale script refusal\n", .{});
    store.recordInsertedScript(
        "https://example.com/page.html",
        "https://example.com/bad.js",
        10000,
        InsertedRun.stale,
        "ctx-1",
    );
    // Subsequent attempt to learn bad.js should be rejected because it is in refused set
    store.recordInsertedScript(
        "https://example.com/page.html",
        "https://example.com/bad.js",
        20000,
        InsertedRun.boa,
        "ctx-1",
    );
    {
        const learned = store.learnedScriptsFor("https://example.com/page.html", "ctx-1");
        expect(learned.len == 0);
    }

    std.debug.print("8. LearnedScriptsStore context clear\n", .{});
    store.recordInsertedScript(
        "https://example.com/home.html",
        "https://example.com/home.js",
        10000,
        InsertedRun.boa,
        "ctx-to-clear",
    );
    store.clearLearnedScripts("ctx-to-clear");
    {
        const learned = store.learnedScriptsFor("https://example.com/home.html", "ctx-to-clear");
        expect(learned.len == 0);
    }
}

pub fn main() {
    testSpeculativePrefetchCoordinator();
    testLearnedScriptsStore();
    std.debug.print("ALL SPECULATIVE PREFETCH & LEARNED SCRIPTS TESTS PASSED SUCCESSFULLY!\n", .{});
}
