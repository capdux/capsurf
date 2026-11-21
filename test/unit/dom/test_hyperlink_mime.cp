// test/dom/test_hyperlink_mime.cp
// Tests WHATWG MIME parsing/normalization and HTMLHyperlinkElementUtils decomposition

import { normalizeBlobType, MimeType, parseMime } from "../../../src/net/mime.cp";
import { HyperlinkDecomposition, decomposeHyperlink } from "../../../src/dom/hyperlink.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. WHATWG normalizeBlobType tests
    {
        let buf: [256]u8 = undefined;

        // "TEXT/HTML;CHARSET=GBK" -> "text/html;charset=gbk"
        const res1 = normalizeBlobType("TEXT/HTML;CHARSET=GBK", buf[0..]);
        assert(std.mem.eql(u8, res1, "text/html;charset=gbk"), "TEXT/HTML;CHARSET=GBK -> text/html;charset=gbk");

        // "text/html;charset=\u000bgbk" -> "" (0x0B < 0x20)
        const res2 = normalizeBlobType("text/html;charset=\x0bgbk", buf[0..]);
        assert(std.mem.eql(u8, res2, ""), "control char 0x0B returns empty");

        // "text/\u00e5" -> "" (0xE5 > 0x7E)
        const res3 = normalizeBlobType("text/\xe5", buf[0..]);
        assert(std.mem.eql(u8, res3, ""), "non-ASCII byte 0xE5 returns empty");

        // " image/gif " -> " image/gif " (preserves valid leading/trailing spaces)
        const res4 = normalizeBlobType(" image/gif ", buf[0..]);
        assert(std.mem.eql(u8, res4, " image/gif "), "preserves valid spaces");

        // Empty input returns empty
        const res5 = normalizeBlobType("", buf[0..]);
        assert(std.mem.eql(u8, res5, ""), "empty input returns empty");

        // Mixed case with numbers and allowed symbols
        const res6 = normalizeBlobType("Application/VND.api+JSON", buf[0..]);
        assert(std.mem.eql(u8, res6, "application/vnd.api+json"), "vendor json mime");

        // Other control characters like \t or \n return empty
        const res7 = normalizeBlobType("text/plain\t", buf[0..]);
        assert(std.mem.eql(u8, res7, ""), "tab character returns empty");
        const res8 = normalizeBlobType("text/plain\n", buf[0..]);
        assert(std.mem.eql(u8, res8, ""), "newline character returns empty");
        const res9 = normalizeBlobType("text/plain\x7f", buf[0..]);
        assert(std.mem.eql(u8, res9, ""), "DEL (0x7F) returns empty");
    }

    // 2. MimeType.parse tests
    {
        // Parse with charset parameter
        const m1_opt = MimeType.parse("text/html; charset=gbk");
        assert(m1_opt != null, "m1 parsed successfully");
        const m1 = m1_opt.?;
        assert(std.mem.eql(u8, m1.raw, "text/html; charset=gbk"), "m1 raw preserved");
        assert(std.mem.eql(u8, m1.essence, "text/html"), "m1 essence is text/html");
        assert(std.mem.eql(u8, m1.charset, "gbk"), "m1 charset is gbk");

        // Parse simple MIME without parameters
        const m2_opt = MimeType.parse("application/json");
        assert(m2_opt != null, "m2 parsed successfully");
        const m2 = m2_opt.?;
        assert(std.mem.eql(u8, m2.essence, "application/json"), "m2 essence is application/json");
        assert(std.mem.eql(u8, m2.charset, ""), "m2 charset is empty");

        // Parse quoted charset parameter
        const m3_opt = parseMime("text/plain; charset=\"utf-8\"");
        assert(m3_opt != null, "m3 parsed successfully");
        const m3 = m3_opt.?;
        assert(std.mem.eql(u8, m3.essence, "text/plain"), "m3 essence is text/plain");
        assert(std.mem.eql(u8, m3.charset, "utf-8"), "m3 quoted charset unquoted");

        // Parse with multiple parameters
        const m4_opt = MimeType.parse("text/html; boundary=something; charset=iso-8859-1");
        assert(m4_opt != null, "m4 parsed successfully");
        const m4 = m4_opt.?;
        assert(std.mem.eql(u8, m4.essence, "text/html"), "m4 essence is text/html");
        assert(std.mem.eql(u8, m4.charset, "iso-8859-1"), "m4 charset iso-8859-1");

        // Invalid MIME strings return null
        assert(MimeType.parse("") == null, "empty string is null");
        assert(MimeType.parse("invalid_no_slash") == null, "no slash is null");
        assert(MimeType.parse("/no_type") == null, "no type is null");
        assert(MimeType.parse("text/") == null, "no subtype is null");
        assert(MimeType.parse("text/html invalid; charset=utf-8") == null, "space in subtype is null");
    }

    // 3. HyperlinkDecomposition.decompose tests
    {
        // Test invalid URL with extra colon after port: "http://example.com:80:"
        const d1 = HyperlinkDecomposition.decompose("http://example.com:80:");
        assert(std.mem.eql(u8, d1.href, "http://example.com:80:"), "d1 href preserved");
        assert(std.mem.eql(u8, d1.protocol, ":"), "d1 protocol is :");
        assert(std.mem.eql(u8, d1.origin, "null"), "d1 origin is null");
        assert(std.mem.eql(u8, d1.host, ""), "d1 host is empty");
        assert(std.mem.eql(u8, d1.hostname, ""), "d1 hostname is empty");
        assert(std.mem.eql(u8, d1.port, ""), "d1 port is empty");
        assert(std.mem.eql(u8, d1.pathname, ""), "d1 pathname is empty");
        assert(std.mem.eql(u8, d1.search, ""), "d1 search is empty");
        assert(std.mem.eql(u8, d1.hash, ""), "d1 hash is empty");

        // Test valid URL: "https://example.test:8080/path?query=1#hash"
        const d2 = HyperlinkDecomposition.decompose("https://example.test:8080/path?query=1#hash");
        assert(std.mem.eql(u8, d2.href, "https://example.test:8080/path?query=1#hash"), "d2 href matches");
        assert(std.mem.eql(u8, d2.protocol, "https:"), "d2 protocol is https:");
        assert(std.mem.eql(u8, d2.host, "example.test:8080"), "d2 host is example.test:8080");
        assert(std.mem.eql(u8, d2.hostname, "example.test"), "d2 hostname is example.test");
        assert(std.mem.eql(u8, d2.port, "8080"), "d2 port is 8080");
        assert(std.mem.eql(u8, d2.pathname, "/path"), "d2 pathname is /path");
        assert(std.mem.eql(u8, d2.search, "?query=1"), "d2 search is ?query=1");
        assert(std.mem.eql(u8, d2.hash, "#hash"), "d2 hash is #hash");
        assert(std.mem.eql(u8, d2.origin, "https://example.test:8080"), "d2 origin is https://example.test:8080");

        // Test default port stripping: "http://example.com:80/index.html"
        const d3 = decomposeHyperlink("http://example.com:80/index.html");
        assert(std.mem.eql(u8, d3.protocol, "http:"), "d3 protocol is http:");
        assert(std.mem.eql(u8, d3.host, "example.com"), "d3 default port stripped from host");
        assert(std.mem.eql(u8, d3.hostname, "example.com"), "d3 hostname is example.com");
        assert(std.mem.eql(u8, d3.port, ""), "d3 default port is empty");
        assert(std.mem.eql(u8, d3.pathname, "/index.html"), "d3 pathname is /index.html");
        assert(std.mem.eql(u8, d3.search, ""), "d3 search is empty");
        assert(std.mem.eql(u8, d3.hash, ""), "d3 hash is empty");
        assert(std.mem.eql(u8, d3.origin, "http://example.com"), "d3 origin is http://example.com");

        // Test URL without path: "https://example.org"
        const d4 = HyperlinkDecomposition.decompose("https://example.org");
        assert(std.mem.eql(u8, d4.protocol, "https:"), "d4 protocol is https:");
        assert(std.mem.eql(u8, d4.host, "example.org"), "d4 host is example.org");
        assert(std.mem.eql(u8, d4.hostname, "example.org"), "d4 hostname is example.org");
        assert(std.mem.eql(u8, d4.port, ""), "d4 port is empty");
        assert(std.mem.eql(u8, d4.pathname, "/"), "d4 pathname is /");
        assert(std.mem.eql(u8, d4.search, ""), "d4 search is empty");
        assert(std.mem.eql(u8, d4.hash, ""), "d4 hash is empty");
        assert(std.mem.eql(u8, d4.origin, "https://example.org"), "d4 origin is https://example.org");

        // Test invalid and empty inputs
        const d5 = HyperlinkDecomposition.decompose("");
        assert(std.mem.eql(u8, d5.protocol, ":"), "empty href protocol is :");
        assert(std.mem.eql(u8, d5.origin, "null"), "empty href origin is null");

        const d6 = HyperlinkDecomposition.decompose("not_a_valid_url");
        assert(std.mem.eql(u8, d6.protocol, ":"), "invalid url protocol is :");
        assert(std.mem.eql(u8, d6.origin, "null"), "invalid url origin is null");

        const d7 = HyperlinkDecomposition.decompose("http://");
        assert(std.mem.eql(u8, d7.protocol, ":"), "empty authority protocol is :");
        assert(std.mem.eql(u8, d7.origin, "null"), "empty authority origin is null");

        const d8 = HyperlinkDecomposition.decompose("http://example.com:99999");
        assert(std.mem.eql(u8, d8.protocol, ":"), "overflow port protocol is :");
        assert(std.mem.eql(u8, d8.origin, "null"), "overflow port origin is null");

        const d9 = HyperlinkDecomposition.decompose("http://example.com:abc");
        assert(std.mem.eql(u8, d9.protocol, ":"), "non-numeric port protocol is :");
        assert(std.mem.eql(u8, d9.origin, "null"), "non-numeric port origin is null");
    }

    std.debug.print("test_hyperlink_mime passed all assertions!\n", .{});
}
