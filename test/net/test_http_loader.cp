// test/net/test_http_loader.cp
// CapSurf Pure CP Unit Tests for HttpLoader (offline, no external network)

import { test, expect_eq } from "cap:test";
import {
    HttpLoader,
    HttpLoaderResult,
    parseHttpUrl,
    markFreshExtraHeaders,
    consumeFreshExtraHeaders,
} from "../../src/net/http_loader.cp";
import { CdpPageSession } from "../../src/cdp/session.cp";
import { InterceptVerdict } from "../../src/cdp/fetch.cp";

fn expectFetchError(session: *CdpPageSession, url: []const u8, buf: []u8, depth: usize): bool {
    * = HttpLoader.fetchDocumentWithContext(session, url, buf, depth, "", false) catch return true;
    return false;
}

fn testUrlParsingAndDataDecode()! {
    // 1. HTTP & HTTPS parsing with default/explicit ports and IPv6 literals
    const p_http = parseHttpUrl("http://example.com");
    expect_eq(p_http != null, true);
    if (u := p_http) {
        expect_eq(std.mem.eql(u8, u.host, "example.com"), true);
        expect_eq(u.port, 80);
        expect_eq(std.mem.eql(u8, u.path, "/"), true);
        expect_eq(u.secure, false);
    }

    const p_https = HttpLoader.parseHttpUrl("https://api.example.com:8443/v1/items?q=1#top");
    expect_eq(p_https != null, true);
    if (u := p_https) {
        expect_eq(std.mem.eql(u8, u.host, "api.example.com"), true);
        expect_eq(u.port, 8443);
        expect_eq(std.mem.eql(u8, u.path, "/v1/items?q=1"), true);
        expect_eq(u.secure, true);
    }

    const p_ipv6 = HttpLoader.parseHttpUrl("http://[::1]:8080/health");
    expect_eq(p_ipv6 != null, true);
    if (u := p_ipv6) {
        expect_eq(std.mem.eql(u8, u.host, "::1"), true);
        expect_eq(u.port, 8080);
        expect_eq(std.mem.eql(u8, u.path, "/health"), true);
    }

    // Invalid schemes / malformed URLs
    expect_eq(HttpLoader.parseHttpUrl("ftp://example.com/file") == null, true);
    expect_eq(HttpLoader.parseHttpUrl("about:blank") == null, true);
    expect_eq(HttpLoader.parseHttpUrl("http://") == null, true);
    expect_eq(HttpLoader.parseHttpUrl("http://[::1/bad") == null, true);

    // 2. IPv4, origin, and loopback helpers
    expect_eq(HttpLoader.parseIpv4("localhost") == 0x0100007F, true);
    expect_eq(HttpLoader.parseIpv4("127.0.0.1") == 0x0100007F, true);
    expect_eq(HttpLoader.parseIpv4("192.168.1.1") != null, true);
    expect_eq(HttpLoader.parseIpv4("999.0.0.1") == null, true);
    expect_eq(HttpLoader.parseIpv4("not-an-ip") == null, true);

    const hp = HttpLoader.parseHostAndPort("127.0.0.1:9222", 80);
    expect_eq(std.mem.eql(u8, hp.host, "127.0.0.1"), true);
    expect_eq(hp.port, 9222);

    expect_eq(std.mem.eql(u8, HttpLoader.extractOrigin("https://Example.COM:443/a/b"), "https://Example.COM:443"), true);
    expect_eq(HttpLoader.isSameOrigin("https://example.com/a", "https://EXAMPLE.COM/b"), true);
    expect_eq(HttpLoader.isSameOrigin("https://example.com/a", "http://example.com/a"), false);
    expect_eq(HttpLoader.isLoopbackUrl("http://127.0.0.1:8080/x"), true);
    expect_eq(HttpLoader.isLoopbackUrl("http://sub.localhost/x"), true);
    expect_eq(HttpLoader.isPublicHttpUrl("https://example.org/x"), true);
    expect_eq(HttpLoader.isPublicHttpUrl("http://127.0.0.1/x"), false);

    // 3. data: URL decoding (percent-encoded and base64)
    let out_buf: [512]u8 = undefined;
    const dec_pct = HttpLoader.decodeDataUrl("data:text/html,<h1>Hi%20CP</h1>", &out_buf);
    expect_eq(dec_pct != null, true);
    if (d := dec_pct) {
        expect_eq(std.mem.eql(u8, d, "<h1>Hi CP</h1>"), true);
    }

    // "PGgxPkhlbGxvPC9oMT4=" -> "<h1>Hello</h1>"
    const dec_b64 = HttpLoader.decodeDataUrl("data:text/html;base64,PGgxPkhlbGxvPC9oMT4=", &out_buf);
    expect_eq(dec_b64 != null, true);
    if (d := dec_b64) {
        expect_eq(std.mem.eql(u8, d, "<h1>Hello</h1>"), true);
    }

    // Invalid or undersized buffer
    expect_eq(HttpLoader.decodeDataUrl("http://example.com", &out_buf) == null, true);
    expect_eq(HttpLoader.decodeDataUrl("data:text/plain", &out_buf) == null, true);
    let tiny_buf: [2]u8 = undefined;
    expect_eq(HttpLoader.decodeDataUrl("data:text/plain,abcdef", &tiny_buf) == null, true);
}

fn testRelativeUrlRedirectAndHeaderHelpers()! {
    let buf: [512]u8 = undefined;

    // 1. Relative URL resolution
    const abs = HttpLoader.resolveRelativeUrl("https://example.com/a/b.html", "https://other.test/c", &buf);
    expect_eq(std.mem.eql(u8, abs, "https://other.test/c"), true);

    const root_rel = HttpLoader.resolveRelativeUrl("https://example.com/dir/page.html", "/assets/app.js", &buf);
    expect_eq(std.mem.eql(u8, root_rel, "https://example.com/assets/app.js"), true);

    const dir_rel = HttpLoader.resolveRelativeUrl("https://example.com/dir/page.html", "sub/next.html", &buf);
    expect_eq(std.mem.eql(u8, dir_rel, "https://example.com/dir/sub/next.html"), true);

    const no_orig = HttpLoader.resolveRelativeUrl("about:blank", "foo.html", &buf);
    expect_eq(std.mem.eql(u8, no_orig, "foo.html"), true);

    // 2. Mock redirect URL detection and target resolution
    expect_eq(HttpLoader.isMockRedirectUrl("http://127.0.0.1/__fixtures/loud-redirect?to=%2Fnext"), true);
    expect_eq(HttpLoader.isMockRedirectUrl("http://127.0.0.1/__fixtures/redirect-hop?n=1"), true);
    expect_eq(HttpLoader.isMockRedirectUrl("http://127.0.0.1/__fixtures/normal.html"), false);

    const hop2 = HttpLoader.resolveMockRedirectTarget("http://127.0.0.1:8080/__fixtures/redirect-hop?n=1", &buf);
    expect_eq(hop2 != null, true);
    if (t := hop2) {
        expect_eq(std.mem.eql(u8, t, "http://127.0.0.1:8080/__fixtures/redirect-hop?n=2"), true);
    }

    const hop_done = HttpLoader.resolveMockRedirectTarget("http://127.0.0.1:8080/__fixtures/redirect-hop?n=3", &buf);
    expect_eq(hop_done != null, true);
    if (t := hop_done) {
        expect_eq(std.mem.eql(u8, t, "http://127.0.0.1:8080/__fixtures/lifecycle.html"), true);
    }

    const loud_tgt = HttpLoader.resolveMockRedirectTarget("http://127.0.0.1:8080/__fixtures/loud-redirect?to=%2Fdone.html", &buf);
    expect_eq(loud_tgt != null, true);
    if (t := loud_tgt) {
        expect_eq(std.mem.eql(u8, t, "http://127.0.0.1:8080/done.html"), true);
    }

    // 3. Fresh extra-headers flag lifecycle
    * = consumeFreshExtraHeaders(); // reset
    expect_eq(consumeFreshExtraHeaders(), false);
    markFreshExtraHeaders();
    expect_eq(consumeFreshExtraHeaders(), true);
    expect_eq(consumeFreshExtraHeaders(), false);
}

fn testFetchErrorAndSizeBoundaries()! {
    const session = CdpPageSession.init(std.heap.c_allocator) catch return error.InitFailed;
    defer session.deinit();

    let out_buf: [8192]u8 = undefined;

    // 1. about:blank and empty URL
    const blank_res = HttpLoader.fetchDocumentWithContext(session, "about:blank", &out_buf, 0, "", false)?;
    expect_eq(blank_res.status_code, 200);
    expect_eq(std.mem.eql(u8, blank_res.final_url, "about:blank"), true);
    expect_eq(std.mem.eql(u8, blank_res.content_type, "text/html"), true);

    const empty_res = HttpLoader.fetchDocument(session, "", &out_buf, 0)?;
    expect_eq(empty_res.status_code, 200);
    expect_eq(std.mem.eql(u8, empty_res.final_url, "about:blank"), true);

    // 2. Valid data: URL (base64)
    const data_res = HttpLoader.fetchDocumentWithContext(session, "data:text/html;base64,PGgxPkhlbGxvPC9oMT4=", &out_buf, 0, "", false)?;
    expect_eq(data_res.status_code, 200);
    expect_eq(data_res.raw_body_base64, true);
    expect_eq(std.mem.eql(u8, data_res.content_type, "text/html"), true);
    expect_eq(std.mem.eql(u8, data_res.html, "<h1>Hello</h1>"), true);

    // 3. Redirect depth limit (> 5)
    expect_eq(expectFetchError(session, "about:blank", &out_buf, 6), true);

    // 4. Invalid data: URL and unsupported scheme
    expect_eq(expectFetchError(session, "data:text/plain", &out_buf, 0), true);
    expect_eq(expectFetchError(session, "ftp://example.com/resource", &out_buf, 0), true);

    // 5. Oversize document fixture boundaries
    expect_eq(expectFetchError(session, "http://127.0.0.1/__fixtures/oversize-doc.html?endless=1", &out_buf, 0), true);
    expect_eq(expectFetchError(session, "http://127.0.0.1/__fixtures/oversize-doc.html?declared=20000000", &out_buf, 0), true);
}

fn testOfflineMockFixturesAndInterception()! {
    const session = CdpPageSession.init(std.heap.c_allocator) catch return error.InitFailed;
    defer session.deinit();

    let out_buf: [32768]u8 = undefined;

    // 1. Offline mock fixture: echo-headers.json & status-204
    const json_res = HttpLoader.fetchDocumentWithContext(session, "http://127.0.0.1:9999/__fixtures/echo-headers.json", &out_buf, 0, "", false)?;
    expect_eq(json_res.status_code, 200);
    expect_eq(std.mem.eql(u8, json_res.content_type, "application/json"), true);
    expect_eq(json_res.html.len > 0, true);

    const no_content = HttpLoader.fetchDocumentWithContext(session, "http://127.0.0.1:9999/__fixtures/status-204", &out_buf, 0, "", false)?;
    expect_eq(no_content.status_code, 204);

    // 2. Offline multi-hop mock redirect chain (n=1 -> n=2 -> n=3 -> lifecycle.html)
    const redir_res = HttpLoader.fetchDocumentWithContext(session, "http://127.0.0.1:9999/__fixtures/redirect-hop?n=1", &out_buf, 0, "", false)?;
    expect_eq(redir_res.status_code, 200);
    expect_eq(redir_res.redirect_count >= 3, true);
    expect_eq(std.mem.endsWith(u8, redir_res.final_url, "/__fixtures/lifecycle.html"), true);

    // 3. Offline CDP Fetch interception (Fulfill & Fail) without opening any socket
    session.fetch_enabled = true;
    session.fetch_state.enabled = true;
    session.fetch_state.addPattern("https://offline.intercept.test/*", "Document");

    const fulfill_url = "https://offline.intercept.test/page.html";
    const parked_ok = session.fetch_state.parkRequest("req-fulfill", fulfill_url, "GET");
    expect_eq(parked_ok != null, true);
    if (p := parked_ok) {
        p.verdict = InterceptVerdict.Fulfill;
        p.mock_status = 201;
        p.mock_body = "data:text/html,<div>Intercepted</div>";
    }
    const int_res = HttpLoader.fetchDocumentWithContext(session, fulfill_url, &out_buf, 0, "", false)?;
    expect_eq(int_res.status_code, 201);
    expect_eq(std.mem.eql(u8, int_res.html, "<div>Intercepted</div>"), true);

    const fail_url = "https://offline.intercept.test/fail.html";
    const parked_fail = session.fetch_state.parkRequest("req-fail", fail_url, "GET");
    expect_eq(parked_fail != null, true);
    if (p := parked_fail) {
        p.verdict = InterceptVerdict.Fail;
    }
    expect_eq(expectFetchError(session, fail_url, &out_buf, 0), true);
}

fn main() {
    test("1. URL Parsing, IPv4/Origin/Loopback Helpers & Data URL Decoding", () => {
        testUrlParsingAndDataDecode() catch unreachable;
    });

    test("2. Relative URL Resolution, Mock Redirect Helpers & Extra Headers Flag", () => {
        testRelativeUrlRedirectAndHeaderHelpers() catch unreachable;
    });

    test("3. Fetch Error Boundaries, Redirect Depth Cap & Oversize Document Gate", () => {
        testFetchErrorAndSizeBoundaries() catch unreachable;
    });

    test("4. Offline Mock Fixtures, Redirect Chain & CDP Fetch Interception", () => {
        testOfflineMockFixturesAndInterception() catch unreachable;
    });
}
