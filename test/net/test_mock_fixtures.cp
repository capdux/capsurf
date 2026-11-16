// test/net/test_mock_fixtures.cp
// CapSurf Mock Fixtures Unit Test Suite

import { MockFixtures } from "../../src/net/mock_fixtures.cp";
import { HttpLoader } from "../../src/net/http_loader.cp";
import { CdpPageSession } from "../../src/cdp/session.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. MockFixtures.isFixtureUrl identification
    {
        // Standard fixture URLs
        assert(MockFixtures.isFixtureUrl("https://ks.invalid/__fixtures/echo-headers.json"), "ks.invalid echo-headers");
        assert(MockFixtures.isFixtureUrl("http://127.0.0.1:8787/__fixtures/echo-headers.json"), "127.0.0.1 echo-headers");
        assert(!MockFixtures.isFixtureUrl("https://example.com/index.html"), "example.com not fixture");

        // Host ending with .invalid
        assert(MockFixtures.isFixtureUrl("https://ks.invalid/test/page"), "ks.invalid test page");
        assert(MockFixtures.isFixtureUrl("http://evil.invalid:8080/foo"), "evil.invalid");
        assert(MockFixtures.isFixtureUrl("http://other.domain.invalid"), "other domain invalid");

        // Non-fixture URLs
        assert(!MockFixtures.isFixtureUrl("https://example.invalid.com/test"), "example.invalid.com");
        assert(!MockFixtures.isFixtureUrl("http://localhost:8080/index.html"), "localhost");
        assert(!MockFixtures.isFixtureUrl("https://google.com/search"), "google.com");
        std.debug.print("PASS: 1. MockFixtures.isFixtureUrl identification\n", .{});
    }

    // 2. MockFixtures.handleFixtureRequest synthesis
    {
        let buf: [1024]u8 = [0; 1024];

        // Default fallbacks when origin and user-agent are empty
        const res1 = MockFixtures.handleFixtureRequest(
            "/__fixtures/echo-headers.json",
            "",
            "",
            &buf,
        );
        assert(res1 != null, "res1 not null");
        if (json1 := res1) {
            assert(std.mem.indexOf(u8, json1, "\"origin\":\"null\"") != null, "origin null");
            assert(std.mem.indexOf(u8, json1, "\"user-agent\":\"CapSurf/1.0.0\"") != null, "user-agent default");
            assert(std.mem.indexOf(u8, json1, "\"accept\":\"*/*\"") != null, "accept */*");
        }

        // Custom parameters
        const res2 = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/echo-headers.json",
            "https://app.test",
            "CapSurf-Custom/2.0",
            &buf,
        );
        assert(res2 != null, "res2 not null");
        if (json2 := res2) {
            assert(std.mem.indexOf(u8, json2, "\"origin\":\"https://app.test\"") != null, "origin custom");
            assert(std.mem.indexOf(u8, json2, "\"user-agent\":\"CapSurf-Custom/2.0\"") != null, "user-agent custom");
        }

        // Non-matching fixture returns null
        const res3 = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/echo-headers.html",
            "",
            "",
            &buf,
        );
        assert(res3 == null, "res3 is null");
        std.debug.print("PASS: 2. MockFixtures.handleFixtureRequest synthesis\n", .{});
    }

    // 3. HttpLoader.fetchDocument fixture request interception
    {
        let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
        defer session.deinit();

        session.user_agent = "CapSurf-TestRunner/1.0";

        let fetch_buf: [4096]u8 = [0; 4096];
        const fixture_url = "https://ks.invalid/__fixtures/echo-headers.json";

        const fetch_res = HttpLoader.fetchDocument(
            session,
            fixture_url,
            &fetch_buf,
            0,
        ) catch unreachable;

        assert(fetch_res.status_code == 200, "status 200");
        assert(std.mem.eql(u8, fetch_res.final_url, fixture_url), "final_url matches");
        assert(std.mem.indexOf(u8, fetch_res.html, "\"origin\":\"null\"") != null, "fetch origin null");
        assert(std.mem.indexOf(u8, fetch_res.html, "\"user-agent\":\"CapSurf-TestRunner/1.0\"") != null, "fetch ua");
        assert(std.mem.indexOf(u8, fetch_res.html, "\"accept\":\"*/*\"") != null, "fetch accept");
        std.debug.print("PASS: 3. HttpLoader.fetchDocument fixture request interception\n", .{});
    }
}
