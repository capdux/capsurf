// test/cdp/test_outbound_filters.cp
// CapSurf Comprehensive Test Suite: Pike VM Linear-time Regex & CDP Outbound Egress Filter Registry
// Migrated from vendor/kitesurf/packages/engine/src/cdp/linear-regex.ts
// and vendor/kitesurf/packages/engine/src/cdp/outbound-filter-registry.ts

import {
    MAX_REPEAT,
    MAX_PROGRAM,
    LinearRegex,
    compileLinearRegex,
} from "../../../src/cdp/linear_regex.cp";
import {
    OutboundResourceType,
    OutboundFilters,
    ShouldBlockResult,
    compileOutboundFilters,
    shouldBlock,
    registerOutboundFilters,
    getOutboundFilters,
    unregisterOutboundFilters,
} from "../../../src/cdp/outbound_filters.cp";

fn expect(cond: bool) {
    if (!cond) @panic("assertion failed");
}

fn testLinearRegexBasics() {
    // 1. Literal matching
    {
        std.debug.print("1. Literal matching\n", .{});
        let res = compileLinearRegex("hello");
        expect(res.ok);
        expect(res.regex.@"test"("hello"));
        expect(res.regex.@"test"("say hello world"));
        expect(!res.regex.@"test"("hell"));
        expect(!res.regex.@"test"("world"));
        res.regex.deinit();
    }

    // 2. Anchors
    {
        std.debug.print("2. Anchors\n", .{});
        let res = compileLinearRegex("^hello$");
        expect(res.ok);
        expect(res.regex.@"test"("hello"));
        expect(!res.regex.@"test"("hello world"));
        expect(!res.regex.@"test"("say hello"));
        res.regex.deinit();
    }

    // 3. Dot any character
    {
        std.debug.print("3. Dot any character\n", .{});
        let res = compileLinearRegex("^a.c$");
        expect(res.ok);
        expect(res.regex.@"test"("abc"));
        expect(res.regex.@"test"("a1c"));
        expect(res.regex.@"test"("a-c"));
        expect(!res.regex.@"test"("ac"));
        expect(!res.regex.@"test"("abbc"));
        res.regex.deinit();
    }

    // 4. Character classes and ranges
    {
        std.debug.print("4. Character classes and ranges\n", .{});
        let res = compileLinearRegex("^[a-z0-9_]+$");
        expect(res.ok);
        expect(res.regex.@"test"("capsurf_42"));
        expect(!res.regex.@"test"("Capsurf"));
        expect(!res.regex.@"test"("cap surf"));
        expect(!res.regex.@"test"(""));
        res.regex.deinit();
    }

    // 5. Negated character class
    {
        std.debug.print("5. Negated character class\n", .{});
        let res = compileLinearRegex("^[^0-9]+$");
        expect(res.ok);
        expect(res.regex.@"test"("abc_XYZ"));
        expect(!res.regex.@"test"("abc1"));
        res.regex.deinit();
    }

    // 6. Predefined character classes (\d, \w, \s)
    {
        std.debug.print("6. Predefined character classes\n", .{});
        let res = compileLinearRegex("^\\d+\\s\\w+$");
        expect(res.ok);
        expect(res.regex.@"test"("123 agent"));
        expect(!res.regex.@"test"("abc agent"));
        expect(!res.regex.@"test"("123agent"));
        res.regex.deinit();
    }

    // 7. Alternation
    {
        std.debug.print("7. Alternation\n", .{});
        let res = compileLinearRegex("^(GET|POST|PUT|DELETE)$");
        expect(res.ok);
        expect(res.regex.@"test"("GET"));
        expect(res.regex.@"test"("POST"));
        expect(res.regex.@"test"("DELETE"));
        expect(!res.regex.@"test"("PATCH"));
        res.regex.deinit();
    }

    // 8. Quantifiers (*, +, ?, {m,n})
    {
        std.debug.print("8. Quantifiers\n", .{});
        let res = compileLinearRegex("^a{2,4}$");
        expect(res.ok);
        expect(!res.regex.@"test"("a"));
        expect(res.regex.@"test"("aa"));
        expect(res.regex.@"test"("aaa"));
        expect(res.regex.@"test"("aaaa"));
        expect(!res.regex.@"test"("aaaaa"));
        res.regex.deinit();
    }

    // 9. Word boundary \b
    {
        std.debug.print("9. Word boundary\n", .{});
        let res = compileLinearRegex("\\bcat\\b");
        expect(res.ok);
        expect(res.regex.@"test"("the cat sits"));
        expect(!res.regex.@"test"("the cats sit"));
        expect(!res.regex.@"test"("scatter"));
        res.regex.deinit();
    }

    // 10. Non-capturing groups and named groups
    {
        std.debug.print("10. Non-capturing groups and named groups\n", .{});
        let res = compileLinearRegex("^(?:https?)://(?<domain>[a-z.]+)/?$");
        expect(res.ok);
        expect(res.regex.@"test"("http://example.com/"));
        expect(res.regex.@"test"("https://sub.domain.org"));
        expect(!res.regex.@"test"("ftp://example.com"));
        res.regex.deinit();
    }

    // 11. ReDoS immunity check: exponential backtrack pattern (a+)+$ must be strictly O(N)
    {
        std.debug.print("11. ReDoS immunity check\n", .{});
        let res = compileLinearRegex("(a+)+$");
        expect(res.ok);
        expect(res.regex.@"test"("aaaaaaaaaa"));
        expect(!res.regex.@"test"("aaaaaaaaaaaaaaaaaaaaaaaa!"));
        res.regex.deinit();
    }

    // 12. Safety bound rejections
    {
        std.debug.print("12. Safety bound rejections\n", .{});
        let r1 = compileLinearRegex("a{1001}");
        expect(!r1.ok);
        expect(std.mem.indexOf(u8, r1.error_msg, "1000") != null);

        let r2 = compileLinearRegex("a{5,2}");
        expect(!r2.ok);
        expect(std.mem.indexOf(u8, r2.error_msg, "out of order") != null);

        let r3 = compileLinearRegex("\\1");
        expect(!r3.ok);
        expect(std.mem.indexOf(u8, r3.error_msg, "backreferences") != null);

        let r4 = compileLinearRegex("[z-a]");
        expect(!r4.ok);
        expect(std.mem.indexOf(u8, r4.error_msg, "out of order") != null);
    }
}

fn testOutboundFiltersAndRegistry() {
    const allows: [2][]const u8 = [
        "^https://api\\.example\\.com/.*",
        "^https://cdn\\.example\\.com/.*",
    ];
    const rejects: [2][]const u8 = [
        ".*\\.trackers\\.com/.*",
        ".*\\.adservice\\.net/.*",
    ];
    const allow_types: [3][]const u8 = [
        "Document",
        "Script",
        "XHR",
    ];
    const reject_types: [2][]const u8 = [
        "Image",
        "Media",
    ];

    let comp = compileOutboundFilters(&allows, &rejects, &allow_types, &reject_types);
    expect(comp.ok);

    let f = comp.filters;

    // 1. Allowed request
    {
        let r = shouldBlock(&f, "https://api.example.com/v1/user", "XHR");
        expect(!r.blocked);
    }

    // 2. Reject request pattern matches
    {
        let r = shouldBlock(&f, "https://analytics.trackers.com/collect", "XHR");
        expect(r.blocked);
        expect(std.mem.startsWith(u8, r.reason(), "rejectRequestPattern matched ("));
    }

    // 3. Reject resource type matches
    {
        let r = shouldBlock(&f, "https://api.example.com/avatar.png", "Image");
        expect(r.blocked);
        expect(std.mem.eql(u8, r.reason(), "rejectResourceTypes matched (Image)"));
    }

    // 4. Reject resource type case-insensitive
    {
        let r = shouldBlock(&f, "https://api.example.com/video.mp4", "media");
        expect(r.blocked);
        expect(std.mem.eql(u8, r.reason(), "rejectResourceTypes matched (media)"));
    }

    // 5. Allow request pattern did not match
    {
        let r = shouldBlock(&f, "https://other-domain.org/data", "XHR");
        expect(r.blocked);
        expect(std.mem.eql(u8, r.reason(), "allowRequestPattern did not match"));
    }

    // 6. Allow resource type did not match
    {
        let r = shouldBlock(&f, "https://cdn.example.com/font.woff2", "Font");
        expect(r.blocked);
        expect(std.mem.eql(u8, r.reason(), "allowResourceTypes did not match (Font)"));
    }

    // 7. Registry per-connection tests
    {
        registerOutboundFilters("conn-agent-42", f);
        const retrieved = getOutboundFilters("conn-agent-42");
        expect(retrieved != null);

        let r = shouldBlock(retrieved.?, "https://api.example.com/v1/ping", "Document");
        expect(!r.blocked);

        let blocked_r = shouldBlock(retrieved.?, "https://evil.adservice.net/ad.js", "Script");
        expect(blocked_r.blocked);

        unregisterOutboundFilters("conn-agent-42");
        expect(getOutboundFilters("conn-agent-42") == null);
    }
}

fn testValidationLimits() {
    const empty_list: []const []const u8 = [];

    // Pattern too long (> 500 chars)
    let long_buf: [505]u8 = [0; 505];
    @memset(long_buf[0..], 'a');
    const long_pat = long_buf[0..];
    const long_list: [1][]const u8 = [long_pat];

    let res = compileOutboundFilters(&long_list, empty_list, empty_list, empty_list);
    expect(!res.ok);
    expect(std.mem.indexOf(u8, res.error_msg(), "500 chars") != null);
}

fn main() {
    testLinearRegexBasics();
    testOutboundFiltersAndRegistry();
    testValidationLimits();
    std.debug.print("ALL PIKE VM REGEX & OUTBOUND FILTERS TESTS PASSED SUCCESSFULLY!\n", .{});
}
