// test/net/test_cookies.cp
// CapSurf RFC 6265 CookieJar Comprehensive Test Suite (1:1 aligned with e2e/ref/cookies.test.ts)

import { test, expect_eq } from "cap:test";
import { CookieJar, entryToCdp, SameSite } from "../../src/net/cookies.cp";

const HTTPS = "https://app.example.com/x";

fn main() {
    test("1. Domain attribute validation & boundary acceptance/rejection", () => {
        let jar = CookieJar.init(std.heap.page_allocator);
        defer jar.deinit();

        // 1. Accepts parent domain
        const e1 = jar.setFromHeader(HTTPS, "sid=1; Domain=example.com", null);
        expect_eq(e1 != null, true);
        expect_eq(std.mem.eql(u8, e1.?.host, "example.com"), true);
        expect_eq(e1.?.host_only, false);

        // 2. Accepts leading-dot Domain, canonicalized
        const e2 = jar.setFromHeader(HTTPS, "sid2=2; Domain=.example.com", null);
        expect_eq(e2 != null, true);
        expect_eq(std.mem.eql(u8, e2.?.host, "example.com"), true);

        // 3. Accepts Domain equal to setting host
        const e3 = jar.setFromHeader(HTTPS, "sid3=3; Domain=app.example.com", null);
        expect_eq(e3 != null, true);
        expect_eq(std.mem.eql(u8, e3.?.host, "app.example.com"), true);

        // 4. REJECTS a Domain setting host is not within
        expect_eq(jar.setFromHeader(HTTPS, "sid=1; Domain=bank.example", null) == null, true);

        // 5. REJECTS sibling domain that merely shares suffix
        expect_eq(jar.setFromHeader("https://evil-example.com/x", "sid=1; Domain=example.com", null) == null, true);

        // 6. REJECTS suffix that is not a label boundary
        expect_eq(jar.setFromHeader(HTTPS, "sid=1; Domain=ample.com", null) == null, true);

        // 7. REJECTS bare public suffix
        expect_eq(jar.setFromHeader(HTTPS, "sid=1; Domain=com", null) == null, true);
        expect_eq(jar.setFromHeader("https://a.foo.co.uk/x", "sid=1; Domain=co.uk", null) == null, true);

        // 8. IP hosts: rejects subdomain claims, accepts self
        expect_eq(jar.setFromHeader("http://1.2.3.4/x", "sid=1; Domain=3.4", null) == null, true);
        expect_eq(jar.setFromHeader("http://1.2.3.4/x", "sid=1; Domain=1.2.3.4", null) != null, true);

        // 9. Localhost self-claim
        const e_lh = jar.setFromHeader("http://localhost/x", "sid=1; Domain=localhost", null);
        expect_eq(e_lh != null, true);
        expect_eq(std.mem.eql(u8, e_lh.?.host, "localhost"), true);

        // 10. Empty Domain= is host-only
        const e_empty = jar.setFromHeader("https://example.com/x", "sid=1; Domain=", null);
        expect_eq(e_empty != null, true);
        expect_eq(e_empty.?.host_only, true);

        // 11. Self-claim on public suffix stored as host-only
        const e_psl = jar.setFromHeader("https://co.uk/x", "sid=1; Domain=co.uk", null);
        expect_eq(e_psl != null, true);
        expect_eq(e_psl.?.host_only, true);
        const vic_h = jar.cookieHeaderForUrl("https://victim.co.uk/", null, std.heap.page_allocator);
        defer if (vic_h != null) std.heap.page_allocator.free(vic_h.?);
        expect_eq(vic_h == null, true);
    });

    test("2. Domain attribute on read & Cross-Domain Isolation", () => {
        let jar = CookieJar.init(std.heap.page_allocator);
        defer jar.deinit();

        * = jar.setFromHeader(HTTPS, "sid=1; Domain=example.com", null);

        // Sends Domain cookie to subdomains and root
        const h1 = jar.cookieHeaderForUrl("https://app.example.com/", null, std.heap.page_allocator);
        defer if (h1 != null) std.heap.page_allocator.free(h1.?);
        expect_eq(h1 != null, true);
        expect_eq(std.mem.eql(u8, h1.?, "sid=1"), true);

        const h2 = jar.cookieHeaderForUrl("https://other.example.com/", null, std.heap.page_allocator);
        defer if (h2 != null) std.heap.page_allocator.free(h2.?);
        expect_eq(h2 != null, true);
        expect_eq(std.mem.eql(u8, h2.?, "sid=1"), true);

        const h3 = jar.cookieHeaderForUrl("https://example.com/", null, std.heap.page_allocator);
        defer if (h3 != null) std.heap.page_allocator.free(h3.?);
        expect_eq(h3 != null, true);
        expect_eq(std.mem.eql(u8, h3.?, "sid=1"), true);

        // Does NOT send to sibling registrable domain
        const h_org = jar.cookieHeaderForUrl("https://example.org/", null, std.heap.page_allocator);
        defer if (h_org != null) std.heap.page_allocator.free(h_org.?);
        expect_eq(h_org == null, true);

        const h_not = jar.cookieHeaderForUrl("https://notexample.com/", null, std.heap.page_allocator);
        defer if (h_not != null) std.heap.page_allocator.free(h_not.?);
        expect_eq(h_not == null, true);
    });

    test("3. Path attribute matching & prefix boundary rules", () => {
        let jar = CookieJar.init(std.heap.page_allocator);
        defer jar.deinit();

        * = jar.setFromHeader("https://example.com/doc", "root=0; Path=/", null);
        * = jar.setFromHeader("https://example.com/doc", "sub=1; Path=/sub", null);

        // /sub matches /sub, /sub/, /sub/page
        const h_sub1 = jar.cookieHeaderForUrl("https://example.com/sub", null, std.heap.page_allocator);
        defer if (h_sub1 != null) std.heap.page_allocator.free(h_sub1.?);
        expect_eq(h_sub1 != null, true);
        // Longer path (/sub) is sorted first!
        expect_eq(std.mem.eql(u8, h_sub1.?, "sub=1; root=0"), true);

        const h_sub2 = jar.cookieHeaderForUrl("https://example.com/sub/child", null, std.heap.page_allocator);
        defer if (h_sub2 != null) std.heap.page_allocator.free(h_sub2.?);
        expect_eq(std.mem.eql(u8, h_sub2.?, "sub=1; root=0"), true);

        // /subprefix does NOT match /sub
        const h_other = jar.cookieHeaderForUrl("https://example.com/subprefix", null, std.heap.page_allocator);
        defer if (h_other != null) std.heap.page_allocator.free(h_other.?);
        expect_eq(std.mem.eql(u8, h_other.?, "root=0"), true);
    });

    test("4. Secure cookies, Trustworthy Origins & Non-Trustworthy Overwrite Protection", () => {
        let jar = CookieJar.init(std.heap.page_allocator);
        defer jar.deinit();

        // HTTP non-localhost rejects Secure cookie
        expect_eq(jar.setFromHeader("http://example.com/x", "sec=1; Secure", null) == null, true);

        // HTTP localhost allows Secure cookie
        expect_eq(jar.setFromHeader("http://localhost/x", "sec=1; Secure", null) != null, true);

        // Set Secure cookie on HTTPS
        * = jar.setFromHeader("https://example.com/x", "token=secret; Secure", null);

        // HTTP non-secure CANNOT overwrite Secure cookie
        expect_eq(jar.setFromHeader("http://example.com/x", "token=fake", null) == null, true);

        // Secure cookie is NOT sent over plain HTTP
        const plain_h = jar.cookieHeaderForUrl("http://example.com/x", null, std.heap.page_allocator);
        defer if (plain_h != null) std.heap.page_allocator.free(plain_h.?);
        expect_eq(plain_h == null, true);

        // Secure cookie is sent over HTTPS
        const sec_h = jar.cookieHeaderForUrl("https://example.com/x", null, std.heap.page_allocator);
        defer if (sec_h != null) std.heap.page_allocator.free(sec_h.?);
        expect_eq(std.mem.eql(u8, sec_h.?, "token=secret"), true);
    });

    test("5. HttpOnly Protection, Shadowing Prevention & document.cookie Visibility", () => {
        let jar = CookieJar.init(std.heap.page_allocator);
        defer jar.deinit();

        // 1. Header write with HttpOnly
        * = jar.setFromHeader(HTTPS, "session=secret123; HttpOnly", null);

        // 2. document.cookie string MUST NOT include HttpOnly
        const js_cookies = jar.documentCookieString(HTTPS, std.heap.page_allocator);
        defer std.heap.page_allocator.free(js_cookies);
        expect_eq(js_cookies.len == 0, true);

        // 3. Script write CANNOT overwrite or shadow HttpOnly cookie
        expect_eq(jar.setFromDocumentCookie(HTTPS, "session=hacked") == null, true);

        // 4. Script write CAN create normal non-HttpOnly cookie
        const script_e = jar.setFromDocumentCookie(HTTPS, "theme=dark");
        expect_eq(script_e != null, true);
        expect_eq(script_e.?.http_only, false);

        const doc_str = jar.documentCookieString(HTTPS, std.heap.page_allocator);
        defer std.heap.page_allocator.free(doc_str);
        expect_eq(std.mem.eql(u8, doc_str, "theme=dark"), true);
    });

    test("6. Cookie Prefixes (__Secure- and __Host-) & SameSite=None Invariants", () => {
        let jar = CookieJar.init(std.heap.page_allocator);
        defer jar.deinit();

        // SameSite=None without Secure MUST be rejected
        expect_eq(jar.setFromHeader(HTTPS, "ss=1; SameSite=None", null) == null, true);
        expect_eq(jar.setFromHeader(HTTPS, "ss=1; SameSite=None; Secure", null) != null, true);

        // __Secure- requires Secure
        expect_eq(jar.setFromHeader(HTTPS, "__Secure-t=1", null) == null, true);
        expect_eq(jar.setFromHeader(HTTPS, "__Secure-t=1; Secure", null) != null, true);

        // __Host- requires Secure, Path=/, and NO Domain attribute
        expect_eq(jar.setFromHeader(HTTPS, "__Host-t=1; Secure; Domain=example.com", null) == null, true);
        expect_eq(jar.setFromHeader(HTTPS, "__Host-t=1; Secure; Path=/sub", null) == null, true);
        expect_eq(jar.setFromHeader(HTTPS, "__Host-t=1; Secure; Path=/", null) != null, true);
    });

    test("7. CDP Cookie Format Conversion (entryToCdp)", () => {
        let jar = CookieJar.init(std.heap.page_allocator);
        defer jar.deinit();

        const e = jar.setFromHeader(HTTPS, "sid=abc; Path=/app; Secure; HttpOnly; SameSite=Strict", null);
        expect_eq(e != null, true);

        const cdp = entryToCdp(&e.?);
        expect_eq(std.mem.eql(u8, cdp.name, "sid"), true);
        expect_eq(std.mem.eql(u8, cdp.value, "abc"), true);
        expect_eq(std.mem.eql(u8, cdp.path, "/app"), true);
        expect_eq(cdp.secure, true);
        expect_eq(cdp.http_only, true);
        expect_eq(std.mem.eql(u8, cdp.same_site, "Strict"), true);
        expect_eq(cdp.session, true);
    });
}
