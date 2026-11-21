// test/eval/test_origin_and_csp_edge.cp
// Unit tests for CSP script-src / script-src-attr / SRI / nosniff and Origin comparison edge cases

import {
    parseCsp,
    isSourceAllowed,
    isInlineScriptAllowed,
    isInlineEventHandlerAllowed,
    isExternalScriptAllowed,
    verifySriIntegrity,
    isScriptNosniffBlocked,
    schemeMatches,
    hostMatches,
    defaultPortForScheme,
} from "../../../src/net/csp.cp";

import {
    parseUrlOrigin,
    originOfUrl,
    isCrossOrigin,
    isProtocolDowngrade,
} from "../../../src/net/redirect_walk.cp";

import { originOf } from "../../../src/eval/page_script.cp";
import { learnedOriginOf } from "../../../src/eval/learned_scripts.cp";
import { originOfFrameUrl } from "../../../src/eval/oopif_render.cp";

fn check(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test failure");
    }
}

fn testCspScriptSrcDirectives() {
    std.debug.print("1. CSP script-src directives ('self', 'none', 'nonce-', 'strict-dynamic', https:)\n", .{});

    // 'self'
    const p_self = parseCsp("script-src 'self'");
    check(isExternalScriptAllowed(&p_self, "https://example.com", "https://example.com/app.js", "", false, true), "'self' allows exact same-origin");
    check(isExternalScriptAllowed(&p_self, "https://example.com/page", "https://example.com:443/app.js", "", false, true), "'self' matches implicit and explicit 443");
    check(isExternalScriptAllowed(&p_self, "http://example.com:80/a", "http://example.com/b.js", "", false, true), "'self' matches explicit 80 and implicit http");
    check(!isExternalScriptAllowed(&p_self, "https://example.com", "http://example.com/app.js", "", false, true), "'self' rejects scheme downgrade");
    check(!isExternalScriptAllowed(&p_self, "https://example.com", "https://example.com:8443/app.js", "", false, true), "'self' rejects different port");
    check(!isExternalScriptAllowed(&p_self, "https://example.com", "https://sub.example.com/app.js", "", false, true), "'self' rejects subdomain");
    check(!isExternalScriptAllowed(&p_self, "", "https://example.com/app.js", "", false, true), "'self' rejects when doc_origin is empty");

    // 'none'
    const p_none = parseCsp("script-src 'none'");
    check(!isExternalScriptAllowed(&p_none, "https://example.com", "https://example.com/app.js", "", false, true), "'none' blocks same-origin external");
    check(!isInlineScriptAllowed(&p_none, "", "console.log(1)"), "'none' blocks inline script");
    check(!isInlineEventHandlerAllowed(&p_none), "'none' blocks inline event handler");

    const p_none_and_self = parseCsp("script-src 'none' 'self'");
    check(isExternalScriptAllowed(&p_none_and_self, "https://example.com", "https://example.com/app.js", "", false, true), "'none' with 'self' allows same-origin");

    // 'nonce-...'
    const p_nonce = parseCsp("script-src 'nonce-secret123'");
    check(isExternalScriptAllowed(&p_nonce, "https://example.com", "https://cdn.other.test/a.js", "secret123", false, true), "matching nonce allows external script");
    check(!isExternalScriptAllowed(&p_nonce, "https://example.com", "https://cdn.other.test/a.js", "wrong", false, true), "mismatched nonce blocks external script");
    check(!isExternalScriptAllowed(&p_nonce, "https://example.com", "https://cdn.other.test/a.js", "", false, true), "empty nonce blocks external script");
    check(isInlineScriptAllowed(&p_nonce, "secret123", "alert(1)"), "matching nonce allows inline script");
    check(!isInlineScriptAllowed(&p_nonce, "SECRET123", "alert(1)"), "nonce value comparison is case-sensitive");

    // 'strict-dynamic'
    const p_strict = parseCsp("script-src 'strict-dynamic' 'nonce-n1' https://trusted.test");
    check(isExternalScriptAllowed(&p_strict, "https://example.com", "https://untrusted.test/dyn.js", "", false, false), "'strict-dynamic' allows non-parser-inserted script");
    check(!isExternalScriptAllowed(&p_strict, "https://example.com", "https://trusted.test/static.js", "", false, true), "'strict-dynamic' blocks parser-inserted script without nonce even if host listed");
    check(isExternalScriptAllowed(&p_strict, "https://example.com", "https://untrusted.test/static.js", "n1", false, true), "'strict-dynamic' allows parser-inserted script with matching nonce");

    // https: scheme source
    const p_https = parseCsp("script-src https:");
    check(isExternalScriptAllowed(&p_https, "https://example.com", "https://cdn.any.test/lib.js", "", false, true), "https: allows https target");
    check(!isExternalScriptAllowed(&p_https, "https://example.com", "http://cdn.any.test/lib.js", "", false, true), "https: rejects http target");
    check(!isExternalScriptAllowed(&p_https, "https://example.com", "data:text/javascript,1", "", false, true), "https: rejects data: target");

    // http: scheme source upgrades to https
    const p_http = parseCsp("script-src http:");
    check(isExternalScriptAllowed(&p_http, "http://example.com", "http://a.test/1.js", "", false, true), "http: allows http");
    check(isExternalScriptAllowed(&p_http, "http://example.com", "https://a.test/1.js", "", false, true), "http: allows https upgrade");
}

fn testCspInlineUnsafeAndScriptSrcAttr() {
    std.debug.print("2. CSP 'unsafe-inline', script-src-elem, script-src-attr, and directive fallback\n", .{});

    // 'unsafe-inline' alone vs with nonce/hash
    const p_unsafe = parseCsp("script-src 'unsafe-inline'");
    check(isInlineScriptAllowed(&p_unsafe, "", "console.log(1)"), "'unsafe-inline' allows inline script");
    check(isInlineEventHandlerAllowed(&p_unsafe), "'unsafe-inline' in script-src allows event handlers when script-src-attr absent");

    const p_unsafe_nonce = parseCsp("script-src 'unsafe-inline' 'nonce-abc'");
    check(!isInlineScriptAllowed(&p_unsafe_nonce, "", "console.log(1)"), "'unsafe-inline' ignored when nonce present");
    check(isInlineScriptAllowed(&p_unsafe_nonce, "abc", "console.log(1)"), "matching nonce still allowed");
    check(!isInlineEventHandlerAllowed(&p_unsafe_nonce), "'unsafe-inline' ignored for event handlers when nonce present in script-src");

    // script-src-attr overrides script-src for event handlers
    const p_attr_allow = parseCsp("script-src 'none'; script-src-attr 'unsafe-inline'");
    check(!isInlineScriptAllowed(&p_attr_allow, "", "x()"), "script-src 'none' still blocks inline script elements");
    check(isInlineEventHandlerAllowed(&p_attr_allow), "script-src-attr 'unsafe-inline' allows event handlers");

    const p_attr_block = parseCsp("script-src 'unsafe-inline'; script-src-attr 'none'");
    check(isInlineScriptAllowed(&p_attr_block, "", "x()"), "script-src 'unsafe-inline' allows inline script elements");
    check(!isInlineEventHandlerAllowed(&p_attr_block), "script-src-attr 'none' blocks event handlers");

    const p_attr_empty = parseCsp("script-src 'unsafe-inline'; script-src-attr ;");
    check(!isInlineEventHandlerAllowed(&p_attr_empty), "empty script-src-attr blocks event handlers");

    // script-src-elem overrides script-src for inline/external script elements
    const p_elem = parseCsp("script-src 'unsafe-inline'; script-src-elem 'self'");
    check(!isInlineScriptAllowed(&p_elem, "", "x()"), "script-src-elem 'self' blocks inline script");
    check(isExternalScriptAllowed(&p_elem, "https://a.test", "https://a.test/s.js", "", false, true), "script-src-elem 'self' allows same-origin external");

    // duplicate directive ignored (first wins)
    const p_dup = parseCsp("script-src 'none'; script-src 'unsafe-inline'");
    check(!isInlineScriptAllowed(&p_dup, "", "x()"), "duplicate script-src directive ignored (first wins)");

    // default-src fallback and missing directive
    const p_default = parseCsp("default-src 'self'");
    check(isExternalScriptAllowed(&p_default, "https://a.test", "https://a.test/s.js", "", false, true), "falls back to default-src 'self'");
    check(!isInlineEventHandlerAllowed(&p_default), "falls back to default-src for event handler");

    const p_empty = parseCsp("img-src 'none'");
    check(isInlineScriptAllowed(&p_empty, "", "x()"), "no script/default directive allows inline script");
    check(isInlineEventHandlerAllowed(&p_empty), "no script/default directive allows event handler");
}

fn testCspHostPathRedirectSriAndNosniff() {
    std.debug.print("3. CSP wildcard hosts, paths, redirect hops, SRI, and nosniff\n", .{});

    const p_host = parseCsp("script-src *.example.com/lib/ https://exact.test/app.js");
    check(isExternalScriptAllowed(&p_host, "https://a.test", "https://cdn.example.com/lib/v1/main.js", "", false, true), "wildcard subdomain + directory prefix match");
    check(!isExternalScriptAllowed(&p_host, "https://a.test", "https://example.com/lib/main.js", "", false, true), "wildcard *.example.com does not match bare example.com");
    check(!isExternalScriptAllowed(&p_host, "https://a.test", "https://cdn.example.com/other/main.js", "", false, true), "directory prefix mismatch rejected");
    check(isExternalScriptAllowed(&p_host, "https://a.test", "https://exact.test/app.js", "", false, true), "exact path match");
    check(!isExternalScriptAllowed(&p_host, "https://a.test", "https://exact.test/other.js", "", false, true), "exact path mismatch rejected before redirect");
    check(isExternalScriptAllowed(&p_host, "https://a.test", "https://exact.test/other.js", "", true, true), "redirect hop ignores source path");

    // wildcard * in isSourceAllowed
    const p_star = parseCsp("script-src *");
    check(isSourceAllowed(&p_star, "script-src", "https://any.test/a.js"), "* allows https");
    check(!isSourceAllowed(&p_star, "script-src", "data:text/javascript,1"), "* does not allow data:");

    // SRI integrity
    check(verifySriIntegrity("", "alert(1)"), "empty integrity passes");
    check(verifySriIntegrity("sha256-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=", ""), "sha256 of empty body passes");
    check(!verifySriIntegrity("sha256-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=", "alert(1)"), "mismatched sha256 fails");
    check(verifySriIntegrity("sha999-unknown", "alert(1)"), "unsupported algo only passes");

    // nosniff
    check(isScriptNosniffBlocked("https://a.test/x.js?nosniff=1&ct=text/plain"), "nosniff blocks text/plain");
    check(!isScriptNosniffBlocked("https://a.test/x.js?nosniff=1&ct=application/javascript"), "nosniff allows application/javascript");
    check(!isScriptNosniffBlocked("https://a.test/x.js?ct=text/plain"), "without nosniff param not blocked");

    // helpers
    check(schemeMatches("http", "https:"), "schemeMatches http -> https:");
    check(hostMatches("*.Example.COM", "sub.example.com"), "hostMatches case-insensitive wildcard");
    check(std.mem.eql(u8, defaultPortForScheme("wss:"), "443"), "defaultPortForScheme wss: -> 443");
}

fn testOriginExtractionAndComparisonEdges() {
    std.debug.print("4. Origin extraction and same-origin / cross-origin boundary cases\n", .{});

    // redirect_walk: parseUrlOrigin & isCrossOrigin
    check(!isCrossOrigin("https://Example.COM/a", "https://example.com:443/b?x=1#h"), "case-insensitive host & default 443 port are same-origin");
    check(!isCrossOrigin("http://example.com:80/a", "http://example.com/b"), "http default port 80 equivalence");
    check(!isCrossOrigin("ws://sock.test:80/1", "ws://sock.test/2"), "ws default port 80 equivalence");
    check(!isCrossOrigin("wss://sock.test:443/1", "wss://sock.test/2"), "wss default port 443 equivalence");
    check(isCrossOrigin("https://example.com:8443/a", "https://example.com/b"), "non-default port is cross-origin");
    check(isCrossOrigin("http://example.com/a", "https://example.com/a"), "http vs https is cross-origin");
    check(!isCrossOrigin("http://[::1]:80/a", "http://[::1]/b"), "IPv6 default port equivalence");
    check(isCrossOrigin("http://[::1]:8080/a", "http://[::1]/b"), "IPv6 different port is cross-origin");
    check(isCrossOrigin("about:blank", "https://example.com/"), "opaque URL without :// is cross-origin");
    check(isCrossOrigin("data:text/html,hi", "data:text/html,hi"), "two opaque URLs are cross-origin");
    check(!isCrossOrigin("https://user:pass@example.com/a", "https://example.com/b"), "userinfo stripped in parseUrlOrigin comparison");

    const o1 = parseUrlOrigin("https://[::1]:8443/path").?;
    check(std.mem.eql(u8, o1.scheme, "https"), "parseUrlOrigin ipv6 scheme");
    check(std.mem.eql(u8, o1.host, "[::1]"), "parseUrlOrigin ipv6 host");
    check(std.mem.eql(u8, o1.port, "8443"), "parseUrlOrigin ipv6 port");
    check(parseUrlOrigin("mailto:user@example.com") == null, "parseUrlOrigin returns null without ://");

    check(std.mem.eql(u8, originOfUrl("https://a.b:8443/p?q=1#f"), "https://a.b:8443"), "originOfUrl strips path/query/hash");
    check(std.mem.eql(u8, originOfUrl("about:blank"), "null"), "originOfUrl returns null string for opaque URL");
    check(std.mem.eql(u8, originOfUrl("https://user@host.test/p"), "host.test"), "originOfUrl with userinfo returns host");

    check(isProtocolDowngrade("https://a.test/1", "http://a.test/1"), "https -> http is downgrade");
    check(!isProtocolDowngrade("http://a.test/1", "https://a.test/1"), "http -> https is not downgrade");

    // page_script.originOf
    check(std.mem.eql(u8, originOf("https://a.test:8443/x?y=1#z"), "https://a.test:8443"), "PageScript.originOf valid https");
    check(std.mem.eql(u8, originOf("http://a.test/x"), "http://a.test"), "PageScript.originOf valid http");
    check(std.mem.eql(u8, originOf("ws://a.test/x"), ""), "PageScript.originOf rejects ws");
    check(std.mem.eql(u8, originOf("https:///x"), ""), "PageScript.originOf rejects empty authority");
    check(std.mem.eql(u8, originOf("https://u:p@a.test/x"), ""), "PageScript.originOf rejects userinfo");

    // learned_scripts.learnedOriginOf
    check(std.mem.eql(u8, learnedOriginOf("https://a.test/x").?, "https://a.test"), "LearnedScripts.learnedOriginOf valid");
    check(learnedOriginOf("ftp://a.test/x") == null, "LearnedScripts.learnedOriginOf rejects ftp");
    check(learnedOriginOf("https://u@a.test/x") == null, "LearnedScripts.learnedOriginOf rejects userinfo");

    // oopif_render.originOfFrameUrl
    check(std.mem.eql(u8, originOfFrameUrl("wss://frame.test:9000/ws"), "wss://frame.test:9000"), "originOfFrameUrl supports wss");
    check(std.mem.eql(u8, originOfFrameUrl("about:blank"), ""), "originOfFrameUrl returns empty for opaque");
}

pub fn main() {
    std.debug.print("\n=== Origin & CSP Edge Case Unit Tests ===\n", .{});
    testCspScriptSrcDirectives();
    testCspInlineUnsafeAndScriptSrcAttr();
    testCspHostPathRedirectSriAndNosniff();
    testOriginExtractionAndComparisonEdges();
    std.debug.print("\nAll Origin & CSP edge case tests passed!\n", .{});
}
