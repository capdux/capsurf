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
        assert(MockFixtures.isFixtureUrl("/__fixtures/bench-static.html"), "relative fixtures url");
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

    // 2. MockFixtures.extractFilename extraction
    {
        const f1 = MockFixtures.extractFilename("https://ks.invalid/__fixtures/echo-headers.json");
        assert(std.mem.eql(u8, f1, "echo-headers.json"), "extract echo-headers.json");

        const f2 = MockFixtures.extractFilename("/__fixtures/bench-static.html");
        assert(std.mem.eql(u8, f2, "bench-static.html"), "extract bench-static.html");

        const f3 = MockFixtures.extractFilename("https://ks.invalid/__fixtures/graphprobe/g1.js?v=1#hash");
        assert(std.mem.eql(u8, f3, "graphprobe/g1.js"), "extract with query and hash");

        const f4 = MockFixtures.extractFilename("https://ks.invalid/regular.js");
        assert(std.mem.eql(u8, f4, "regular.js"), "extract regular.js on .invalid host");

        const f5 = MockFixtures.extractFilename("http://other.domain.invalid");
        assert(std.mem.eql(u8, f5, ""), "extract empty path on .invalid host");

        const f6 = MockFixtures.extractFilename("https://example.com/index.html");
        assert(std.mem.eql(u8, f6, ""), "extract non-fixture returns empty");

        std.debug.print("PASS: 2. MockFixtures.extractFilename extraction\n", .{});
    }

    // 3. MockFixtures.getContentType MIME mapping
    {
        assert(std.mem.eql(u8, MockFixtures.getContentType(".html"), "text/html"), "content-type .html");
        assert(std.mem.eql(u8, MockFixtures.getContentType("page.html"), "text/html"), "content-type page.html");
        assert(std.mem.eql(u8, MockFixtures.getContentType(".js"), "application/javascript"), "content-type .js");
        assert(std.mem.eql(u8, MockFixtures.getContentType("script.js"), "application/javascript"), "content-type script.js");
        assert(std.mem.eql(u8, MockFixtures.getContentType(".css"), "text/css"), "content-type .css");
        assert(std.mem.eql(u8, MockFixtures.getContentType("style.css"), "text/css"), "content-type style.css");
        assert(std.mem.eql(u8, MockFixtures.getContentType(".png"), "image/png"), "content-type .png");
        assert(std.mem.eql(u8, MockFixtures.getContentType("icon.png"), "image/png"), "content-type icon.png");
        assert(std.mem.eql(u8, MockFixtures.getContentType(".json"), "application/json"), "content-type .json");
        assert(std.mem.eql(u8, MockFixtures.getContentType("data.json"), "application/json"), "content-type data.json");

        // URLs with query and hash
        assert(std.mem.eql(u8, MockFixtures.getContentType("https://ks.invalid/__fixtures/policy-doc.html?body=1#h"), "text/html"), "url with query .html");
        assert(std.mem.eql(u8, MockFixtures.getContentType("https://ks.invalid/__fixtures/policy-script.js?v=2"), "application/javascript"), "url with query .js");

        std.debug.print("PASS: 3. MockFixtures.getContentType MIME mapping\n", .{});
    }

    // 4. MockFixtures.handleFixtureRequest synthesis & dynamic endpoints
    {
        let buf: [16384]u8 = [0; 16384];

        // 4.1 /echo-headers.json
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

        // 4.2 Dynamic endpoint: /policy-doc.html
        const pdoc1 = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/policy-doc.html",
            "",
            "",
            &buf,
        );
        assert(pdoc1 != null, "pdoc1 not null");
        if (doc := pdoc1) {
            assert(std.mem.indexOf(u8, doc, "<title>policy</title>") != null, "policy doc title");
            assert(std.mem.indexOf(u8, doc, "<body>policy doc</body>") != null, "policy doc body");
        }

        const pdoc2 = MockFixtures.handleFixtureRequest(
            "/policy-doc.html?csp=default-src",
            "",
            "",
            &buf,
        );
        assert(pdoc2 != null, "pdoc2 with query not null");
        if (doc2 := pdoc2) {
            assert(std.mem.indexOf(u8, doc2, "<title>policy</title>") != null, "policy doc2 title");
        }

        // 4.3 Dynamic endpoint: /policy-script.js
        const pscript = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/policy-script.js",
            "",
            "",
            &buf,
        );
        assert(pscript != null, "pscript not null");
        if (scr := pscript) {
            assert(std.mem.indexOf(u8, scr, "window.__policy_loaded = true;") != null, "policy script content");
        }

        // 4.4 Static files from e2e/page/<filename>
        const reg_res = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/regular.js",
            "",
            "",
            &buf,
        );
        assert(reg_res != null, "regular.js not null");
        if (reg := reg_res) {
            assert(std.mem.indexOf(u8, reg, "globalThis.__ks_log(\"regular\")") != null, "regular.js content check");
        }

        // Static file on .invalid host without /__fixtures/
        const reg_inv = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/regular.js",
            "",
            "",
            &buf,
        );
        assert(reg_inv != null, "regular.js on .invalid host not null");
        if (reg2 := reg_inv) {
            assert(std.mem.indexOf(u8, reg2, "globalThis.__ks_log(\"regular\")") != null, "regular.js host content");
        }

        // Static file in subdirectory
        const g1_res = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/graphprobe/g1.js",
            "",
            "",
            &buf,
        );
        assert(g1_res != null, "graphprobe/g1.js not null");
        if (g1 := g1_res) {
            assert(g1.len > 0, "g1 content not empty");
        }

        // Non-matching / non-existent fixture returns null
        const res3 = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/echo-headers.html",
            "",
            "",
            &buf,
        );
        assert(res3 == null, "res3 is null");

        const res_nonexistent = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/does-not-exist.html",
            "",
            "",
            &buf,
        );
        assert(res_nonexistent == null, "nonexistent is null");

        std.debug.print("PASS: 4. MockFixtures.handleFixtureRequest synthesis & dynamic endpoints\n", .{});
    }

    // 5. HttpLoader.fetchDocument fixture request interception
    {
        let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
        defer session.deinit();

        session.user_agent = "CapSurf-TestRunner/1.0";

        let fetch_buf: [16384]u8 = [0; 16384];

        // 5.1 Echo headers
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

        // 5.2 Dynamic policy-doc
        const doc_url = "https://ks.invalid/__fixtures/policy-doc.html";
        const doc_res = HttpLoader.fetchDocument(
            session,
            doc_url,
            &fetch_buf,
            0,
        ) catch unreachable;
        assert(doc_res.status_code == 200, "doc status 200");
        assert(std.mem.indexOf(u8, doc_res.html, "<title>policy</title>") != null, "doc content");

        // 5.3 Static file regular.js
        const file_url = "https://ks.invalid/__fixtures/regular.js";
        const file_res = HttpLoader.fetchDocument(
            session,
            file_url,
            &fetch_buf,
            0,
        ) catch unreachable;
        assert(file_res.status_code == 200, "file status 200");
        assert(std.mem.indexOf(u8, file_res.html, "globalThis.__ks_log(\"regular\")") != null, "file content");

        std.debug.print("PASS: 5. HttpLoader.fetchDocument fixture request interception\n", .{});
    }
}
