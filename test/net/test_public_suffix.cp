// test/net/test_public_suffix.cp
// CapSurf Phase 14 Comprehensive Test Suite: Public Suffix List & Schemeful Site

import {
    isIpHost,
    isPublicSuffix,
    siteKey,
    schemefulSite,
} from "../../src/net/public_suffix.cp";

fn expect(cond: bool) {
    if (!cond) @panic("assertion failed");
}

fn testIpHost() {
    expect(isIpHost("127.0.0.1"));
    expect(isIpHost("192.168.0.1"));
    expect(isIpHost("10.0.0.254"));
    expect(isIpHost("::1"));
    expect(isIpHost("2001:db8::ff00:42:8329"));
    expect(isIpHost("[::1]"));

    expect(!isIpHost("example.com"));
    expect(!isIpHost("localhost"));
    expect(!isIpHost("co.uk"));
    expect(!isIpHost("127.0.0.1.nip.io"));
}

fn testPublicSuffix() {
    // 1. Single TLDs are public suffixes
    expect(isPublicSuffix("com"));
    expect(isPublicSuffix("org"));
    expect(isPublicSuffix("net"));
    expect(isPublicSuffix("cn"));
    expect(isPublicSuffix("uk"));
    expect(isPublicSuffix("io"));

    // 2. Multi-part suffixes are public suffixes
    expect(isPublicSuffix("co.uk"));
    expect(isPublicSuffix("com.au"));
    expect(isPublicSuffix("co.jp"));
    expect(isPublicSuffix("edu.cn"));
    expect(isPublicSuffix("gov.uk"));
    expect(isPublicSuffix("github.io"));
    expect(isPublicSuffix("workers.dev"));
    expect(isPublicSuffix("pages.dev"));
    expect(isPublicSuffix("s3.amazonaws.com"));

    // 3. Registrable domains are NOT public suffixes
    expect(!isPublicSuffix("example.com"));
    expect(!isPublicSuffix("bbc.co.uk"));
    expect(!isPublicSuffix("cloudflare.com"));
    expect(!isPublicSuffix("my-site.github.io"));
    expect(!isPublicSuffix("tsinghua.edu.cn"));

    // 4. IP hosts are NOT public suffixes
    expect(!isPublicSuffix("127.0.0.1"));
    expect(!isPublicSuffix("::1"));
}

fn testSiteKey() {
    // 1. Standard single TLD eTLD+1
    expect(std.mem.eql(u8, siteKey("example.com"), "example.com"));
    expect(std.mem.eql(u8, siteKey("api.example.com"), "example.com"));
    expect(std.mem.eql(u8, siteKey("deep.sub.api.example.com"), "example.com"));

    // 2. Multi-part TLD eTLD+1
    expect(std.mem.eql(u8, siteKey("bbc.co.uk"), "bbc.co.uk"));
    expect(std.mem.eql(u8, siteKey("news.bbc.co.uk"), "bbc.co.uk"));
    expect(std.mem.eql(u8, siteKey("sub.news.bbc.co.uk"), "bbc.co.uk"));
    expect(std.mem.eql(u8, siteKey("tsinghua.edu.cn"), "tsinghua.edu.cn"));
    expect(std.mem.eql(u8, siteKey("cs.tsinghua.edu.cn"), "tsinghua.edu.cn"));

    // 3. Private multi-level public suffixes
    expect(std.mem.eql(u8, siteKey("project.github.io"), "project.github.io"));
    expect(std.mem.eql(u8, siteKey("sub.project.github.io"), "project.github.io"));

    // 4. IP address & single label hosts remain unmodified
    expect(std.mem.eql(u8, siteKey("127.0.0.1"), "127.0.0.1"));
    expect(std.mem.eql(u8, siteKey("::1"), "::1"));
    expect(std.mem.eql(u8, siteKey("localhost"), "localhost"));
}

fn testSchemefulSite() {
    let buf: [256]u8 = [0; 256];

    // 1. Standard HTTPS / HTTP extraction
    const s1 = schemefulSite("https://api.example.com:8443/v1/resource", buf[0..]);
    expect(s1 != null);
    if (v := s1) {
        expect(std.mem.eql(u8, v, "https://example.com"));
    }

    // 2. Multi-part suffix schemeful site
    const s2 = schemefulSite("http://news.bbc.co.uk/world/europe", buf[0..]);
    expect(s2 != null);
    if (v := s2) {
        expect(std.mem.eql(u8, v, "http://bbc.co.uk"));
    }

    // 3. WebSocket scheme normalization (ws: -> http:, wss: -> https:)
    const s3 = schemefulSite("ws://stream.example.com/feed", buf[0..]);
    expect(s3 != null);
    if (v := s3) {
        expect(std.mem.eql(u8, v, "http://example.com"));
    }

    const s4 = schemefulSite("wss://live.finance.co.uk/ticks", buf[0..]);
    expect(s4 != null);
    if (v := s4) {
        expect(std.mem.eql(u8, v, "https://finance.co.uk"));
    }

    // 4. IP host schemeful site
    const s5 = schemefulSite("http://192.168.1.1:8080/dashboard", buf[0..]);
    expect(s5 != null);
    if (v := s5) {
        expect(std.mem.eql(u8, v, "http://192.168.1.1"));
    }
}

fn main() {
    testIpHost();
    testPublicSuffix();
    testSiteKey();
    testSchemefulSite();
    std.debug.print("ALL PHASE 14 PUBLIC SUFFIX TESTS PASSED SUCCESSFULLY!\n", .{});
}
