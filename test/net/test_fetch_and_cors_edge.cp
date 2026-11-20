// test/net/test_fetch_and_cors_edge.cp
// Pure CP white-box edge tests for CORS preflight/Origin, forbidden headers, redirect policy, and empty/malformed header boundaries.

import { test, expect_eq } from "cap:test";
import {
    CredentialsMode,
    CorsError,
    MAX_CORS_SAFE_HEADER_VALUE_CHARS,
    MAX_TOKEN_LIST_SIZE,
    scriptCredentialsMode,
    isSafelistMethod,
    isHttpToken,
    parseTokenList,
    hasCorsUnsafeByte,
    isSafelistedRangeHeader,
    isSafelistRequestHeader,
    isSafelistedResponseHeader,
    isCorsUnsafeRequestHeader,
    needsPreflight,
    checkCorsResponse,
    checkCorsPreflight,
    corsCheck,
    preflightCacheLookup,
    preflightCacheStore,
    clearPreflightCache,
} from "../../src/net/cors.cp";
import {
    isForbiddenRequestHeaderName,
    isMethodOverrideHeaderName,
    isForbiddenMethod,
    isForbiddenResponseHeader,
    isForbiddenRequestHeader,
} from "../../src/net/forbidden_headers.cp";
import {
    HeaderEntry,
    isValidHeaderName,
    isValidHeaderValue,
    HeadersValidator,
    HeadersParser,
    FetchOriginPolicy,
} from "../../src/net/headers_parser.cp";
import {
    MAX_REDIRECT_HOPS,
    isRedirectStatus,
    parseUrlOrigin,
    isCrossOrigin,
    isProtocolDowngrade,
    computeNextMethod,
    shouldDiscardRequestBody,
    computeNextReferrer,
    shouldStripAuthorizationOnRedirect,
    RedirectStopReason,
    RedirectWalkState,
} from "../../src/net/redirect_walk.cp";

fn testOriginPolicyEdgeCases()! {
    // Opaque origins per WHATWG Fetch / HTML
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin(""), "null"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("about:blank"), "null"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("about:blank?foo=1"), "null"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("about:blank#frag"), "null"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("about:srcdoc"), "null"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("data:text/html,<h1>hi</h1>"), "null"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("javascript:void(0)"), "null"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("file:///etc/hosts"), "null"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("://missing-scheme"), "null"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("https:///empty-host"), "null"), true);

    expect_eq(FetchOriginPolicy.isOpaqueOrigin("null"), true);
    expect_eq(FetchOriginPolicy.isOpaqueOrigin(""), true);
    expect_eq(FetchOriginPolicy.isOpaqueOrigin("https://example.com"), false);

    // Default ports are stripped, non-default ports preserved
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("http://example.com:80/a"), "http://example.com"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("https://example.com:443/a"), "https://example.com"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("ws://example.com:80/sock"), "ws://example.com"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("wss://example.com:443/sock"), "wss://example.com"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("https://example.com:8443/a?b=1"), "https://example.com:8443"), true);

    // IPv6 literals with default vs custom ports
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("http://[::1]:80/path"), "http://[::1]"), true);
    expect_eq(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("https://[2001:db8::1]:8443/path"), "https://[2001:db8::1]:8443"), true);

    // Same-origin comparison rejects opaque origins
    expect_eq(FetchOriginPolicy.isSameOrigin("about:blank", "about:blank"), false);
    expect_eq(FetchOriginPolicy.isSameOrigin("https://example.com:443/a", "https://example.com/b"), true);
    expect_eq(FetchOriginPolicy.isSameOrigin("https://example.com:443/a", "https://example.com:8443/b"), false);
}

fn testCorsCheckAndPreflightEdge()! {
    const origin = "https://app.example.com";

    // corsCheck error variants
    expect_eq(corsCheck(null, null, origin, false) == CorsError.MissingOrigin, true);
    expect_eq(corsCheck("   \t  ", null, origin, false) == CorsError.MissingOrigin, true);
    expect_eq(corsCheck("*", null, origin, true) == CorsError.WildcardWithCredentials, true);
    expect_eq(corsCheck("*", null, origin, false) == null, true);
    expect_eq(corsCheck("https://app.example.com, https://other.com", null, origin, false) == CorsError.MultipleOrigins, true);
    expect_eq(corsCheck("https://evil.example.com", null, origin, false) == CorsError.OriginMismatch, true);
    expect_eq(corsCheck(origin, null, origin, true) == CorsError.CredentialsNotAllowed, true);
    expect_eq(corsCheck(origin, "false", origin, true) == CorsError.CredentialsNotAllowed, true);
    expect_eq(corsCheck(origin, "TRUE", origin, true) == CorsError.CredentialsNotAllowed, true);
    expect_eq(corsCheck("  https://app.example.com  ", " true ", origin, true) == null, true);

    // checkCorsResponse & checkCorsPreflight boundaries
    expect_eq(checkCorsResponse(origin, "", false), false);
    expect_eq(checkCorsResponse(origin, "https://app.example.com, *", false), false);
    expect_eq(checkCorsResponse(origin, "  *  ", false), true);
    expect_eq(checkCorsResponse(origin, "  *  ", true), false);

    // Preflight method & header wildcards with/without credentials
    expect_eq(checkCorsPreflight(origin, "PATCH", "", "*", "*", "", false), true);
    expect_eq(checkCorsPreflight(origin, "PATCH", "", origin, "*", "", true), false);
    expect_eq(checkCorsPreflight(origin, "PATCH", "", origin, "PATCH", "", true), true);

    // Wildcard Allow-Headers covers custom headers without credentials, but never Authorization;
    // when allow_credentials=true, wildcard Allow-Headers is disabled completely.
    expect_eq(checkCorsPreflight(origin, "POST", "X-Custom", "*", "POST", "*", false), true);
    expect_eq(checkCorsPreflight(origin, "POST", "X-Custom, Authorization", "*", "POST", "*", false), false);
    expect_eq(checkCorsPreflight(origin, "POST", "X-Custom, Authorization", "*", "POST", "*, authorization", false), true);
    expect_eq(checkCorsPreflight(origin, "POST", "X-Custom", origin, "POST", "*", true), false);
    expect_eq(checkCorsPreflight(origin, "POST", "Authorization", origin, "POST", "*", true), false);
    expect_eq(checkCorsPreflight(origin, "POST", "Authorization", origin, "POST", "*, authorization", true), true);

    // Invalid token lists in Allow-Methods / Allow-Headers must fail preflight
    expect_eq(checkCorsPreflight(origin, "PUT", "", origin, "GET, bad/method", "", false), false);
    expect_eq(checkCorsPreflight(origin, "GET", "X-Foo", origin, "GET", "X-Foo, bad header", false), false);
    // Empty/comma-only request headers are skipped cleanly
    expect_eq(checkCorsPreflight(origin, "GET", " , , ", origin, "GET", "", false), true);
}

fn testCorsSafelistAndPreflightCacheEdge()! {
    expect_eq(scriptCredentialsMode(null) == CredentialsMode.SameOrigin, true);
    expect_eq(scriptCredentialsMode("anonymous") == CredentialsMode.SameOrigin, true);
    expect_eq(scriptCredentialsMode("USE-CREDENTIALS") == CredentialsMode.Include, true);

    // 128-char safelist header value cap
    const buf_129: [129]u8 = ['a'; 129];
    const ok_128 = buf_129[0..MAX_CORS_SAFE_HEADER_VALUE_CHARS];
    const too_long_129 = buf_129[0 .. MAX_CORS_SAFE_HEADER_VALUE_CHARS + 1];
    expect_eq(isSafelistRequestHeader("Accept", ok_128), true);
    expect_eq(isSafelistRequestHeader("Accept", too_long_129), false);

    // Unsafe bytes in Accept / Content-Type
    expect_eq(hasCorsUnsafeByte("text/html"), false);
    expect_eq(hasCorsUnsafeByte("a\"b"), true);
    expect_eq(hasCorsUnsafeByte("a(b)"), true);
    expect_eq(hasCorsUnsafeByte("a\\b"), true);
    expect_eq(hasCorsUnsafeByte("a\x7fb"), true);
    expect_eq(isSafelistRequestHeader("Accept", "text/html;level=1"), true);
    expect_eq(isSafelistRequestHeader("Accept", "text/(html)"), false);

    // Range header safelist boundaries
    expect_eq(isSafelistedRangeHeader("bytes=0-0"), true);
    expect_eq(isSafelistedRangeHeader("bytes=0-"), true);
    expect_eq(isSafelistedRangeHeader("bytes=10-5"), false);
    expect_eq(isSafelistedRangeHeader("bytes=-5"), false);
    expect_eq(isSafelistedRangeHeader("bytes=0-10x"), false);
    expect_eq(isSafelistedRangeHeader("items=0-10"), false);

    // Forbidden/internal headers are not treated as CORS-unsafe (handled by forbidden filter)
    expect_eq(isCorsUnsafeRequestHeader("x-kitesurf-internal", "1"), false);
    expect_eq(isCorsUnsafeRequestHeader("Proxy-Connection", "keep-alive"), false);
    expect_eq(isCorsUnsafeRequestHeader("Sec-Fetch-Dest", "empty"), false);
    expect_eq(isCorsUnsafeRequestHeader("Host", "example.com"), false);
    expect_eq(isCorsUnsafeRequestHeader("X-Custom", "1"), true);
    expect_eq(needsPreflight("GET", false), false);
    expect_eq(needsPreflight("GET", true), true);
    expect_eq(needsPreflight("DELETE", false), true);

    expect_eq(isSafelistedResponseHeader("Pragma"), true);
    expect_eq(isSafelistedResponseHeader("Expires"), true);
    expect_eq(isSafelistedResponseHeader("Set-Cookie"), false);

    // PreflightCache partition, credentials, max-age=0, clamping to 600s, and wildcard rules
    clearPreflightCache();
    const now: i64 = 10_000;

    // Empty partition or max_age=0 must not store
    preflightCacheStore("", "https://a.com", "https://api.com", false, "PUT", "x-a", 60, now);
    expect_eq(preflightCacheLookup("", "https://a.com", "https://api.com", "PUT", "x-a", false, now + 1), false);
    preflightCacheStore("p1", "https://a.com", "https://api.com", false, "PUT", "x-a", 0, now);
    expect_eq(preflightCacheLookup("p1", "https://a.com", "https://api.com", "PUT", "x-a", false, now + 1), false);

    // Max-age > 600s is clamped to 600s (600_000ms)
    preflightCacheStore("p1", "https://a.com", "https://api.com", false, "*", "*", 3600, now);
    expect_eq(preflightCacheLookup("p1", "https://a.com", "https://api.com", "DELETE", "x-any", false, now + 599_000), true);
    expect_eq(preflightCacheLookup("p1", "https://a.com", "https://api.com", "DELETE", "x-any", false, now + 600_000), false);

    // Partition & credentials isolation
    preflightCacheStore("p1", "https://a.com", "https://api.com", false, "*", "*", 120, now);
    expect_eq(preflightCacheLookup("p2", "https://a.com", "https://api.com", "PUT", "", false, now + 1000), false);
    expect_eq(preflightCacheLookup("p1", "https://a.com", "https://api.com", "PUT", "", true, now + 1000), false);

    // Credentials=true disables method/header wildcards in cache too
    preflightCacheStore("p1", "https://a.com", "https://api.com", true, "*", "*", 120, now);
    expect_eq(preflightCacheLookup("p1", "https://a.com", "https://api.com", "PUT", "", true, now + 1000), false);
    expect_eq(preflightCacheLookup("p1", "https://a.com", "https://api.com", "GET", "Authorization", true, now + 1000), false);

    clearPreflightCache();
    expect_eq(preflightCacheLookup("p1", "https://a.com", "https://api.com", "GET", "", false, now + 1000), false);
}

fn testForbiddenHeadersAndValidationEdge()! {
    // Prefix & exact forbidden request header names
    expect_eq(isForbiddenRequestHeaderName("PrOxY-Authenticate"), true);
    expect_eq(isForbiddenRequestHeaderName("proxy-"), true);
    expect_eq(isForbiddenRequestHeaderName("sEc-CH-UA-Platform"), true);
    expect_eq(isForbiddenRequestHeaderName("sec-"), true);
    expect_eq(isForbiddenRequestHeaderName("Access-Control-Request-Headers"), true);
    expect_eq(isForbiddenRequestHeaderName("Access-Control-Request-Method"), true);
    expect_eq(isForbiddenRequestHeaderName("Transfer-Encoding"), true);
    expect_eq(isForbiddenRequestHeaderName("TE"), true);
    expect_eq(isForbiddenRequestHeaderName("Trailer"), true);
    expect_eq(isForbiddenRequestHeaderName("Via"), true);
    expect_eq(isForbiddenRequestHeaderName("DNT"), true);
    expect_eq(isForbiddenRequestHeaderName("X-Requested-With"), false);

    // Method override headers
    expect_eq(isMethodOverrideHeaderName("X-HTTP-Method"), true);
    expect_eq(isMethodOverrideHeaderName("x-http-method-override"), true);
    expect_eq(isMethodOverrideHeaderName("X-Method-Override"), true);
    expect_eq(isMethodOverrideHeaderName("X-Custom-Override"), false);

    expect_eq(isForbiddenMethod("CONNECT"), true);
    expect_eq(isForbiddenMethod("trace"), true);
    expect_eq(isForbiddenMethod("TrAcK"), true);
    expect_eq(isForbiddenMethod("POST"), false);

    expect_eq(isForbiddenRequestHeader("X-HTTP-Method-Override", " \tGET , track "), true);
    expect_eq(isForbiddenRequestHeader("X-Method-Override", "PUT, PATCH, DELETE"), false);

    // Forbidden response headers
    expect_eq(isForbiddenResponseHeader("Set-Cookie"), true);
    expect_eq(isForbiddenResponseHeader("SET-COOKIE2"), true);
    expect_eq(isForbiddenResponseHeader("Set-Cookie3"), false);

    // Header name & value syntax validation
    expect_eq(isValidHeaderName(""), false);
    expect_eq(isValidHeaderName("X-Bad Name"), false);
    expect_eq(isValidHeaderName("X-Bad:Name"), false);
    expect_eq(HeadersValidator.validateHeader("X-Good_1", "ok value"), true);
    expect_eq(isValidHeaderValue("bad\x00val"), false);
    expect_eq(isValidHeaderValue("bad\nval"), false);
    expect_eq(isValidHeaderValue("bad\rval"), false);
    expect_eq(HeadersValidator.validateHeader("X-Good", "bad\r\nInjected: 1"), false);
}

fn testRedirectPolicyEdgeCases()! {
    // initWithMaxHops(0) and (> MAX_REDIRECT_HOPS) clamp to MAX_REDIRECT_HOPS (20)
    let clamped_zero = RedirectWalkState.initWithMaxHops("https://a.example.com/0", "GET", 0);
    expect_eq(clamped_zero.max_hops, MAX_REDIRECT_HOPS);

    let clamped_over = RedirectWalkState.initWithMaxHops("https://a.example.com/0", "GET", 99);
    expect_eq(clamped_over.max_hops, MAX_REDIRECT_HOPS);

    // Verify 20-hop limit enforcement
    const urls: [21][]const u8 = [
        "https://hop.example.com/1",
        "https://hop.example.com/2",
        "https://hop.example.com/3",
        "https://hop.example.com/4",
        "https://hop.example.com/5",
        "https://hop.example.com/6",
        "https://hop.example.com/7",
        "https://hop.example.com/8",
        "https://hop.example.com/9",
        "https://hop.example.com/10",
        "https://hop.example.com/11",
        "https://hop.example.com/12",
        "https://hop.example.com/13",
        "https://hop.example.com/14",
        "https://hop.example.com/15",
        "https://hop.example.com/16",
        "https://hop.example.com/17",
        "https://hop.example.com/18",
        "https://hop.example.com/19",
        "https://hop.example.com/20",
        "https://hop.example.com/21",
    ];
    let i: usize = 0;
    while (i < 20) : (i += 1) {
        expect_eq(clamped_zero.recordHop(urls[i], 302, "Found"), true);
    }
    expect_eq(clamped_zero.hasExceededMaxHops(), true);
    expect_eq(clamped_zero.recordHop(urls[20], 302, "Found"), false);
    if (reason := clamped_zero.stop_reason) {
        expect_eq(reason == RedirectStopReason.MaxHopsExceeded, true);
    } else {
        expect_eq(false, true);
    }

    // Invalid schemes & non-redirect statuses
    let bad_scheme = RedirectWalkState.init("https://a.example.com/", "GET");
    expect_eq(bad_scheme.recordHop("/relative/path", 302, "Found"), false);
    expect_eq(bad_scheme.stop_reason.? == RedirectStopReason.InvalidScheme, true);

    let ftp_scheme = RedirectWalkState.init("https://a.example.com/", "GET");
    expect_eq(ftp_scheme.recordHop("ftp://files.example.com/a", 301, "Moved"), false);
    expect_eq(ftp_scheme.stop_reason.? == RedirectStopReason.InvalidScheme, true);

    let non_redir = RedirectWalkState.init("https://a.example.com/", "GET");
    expect_eq(non_redir.recordHop("https://a.example.com/next", 304, "Not Modified"), false);
    expect_eq(non_redir.stop_reason.? == RedirectStopReason.NonRedirectStatus, true);

    // Redirect loop detection
    let loop_walk = RedirectWalkState.init("https://a.example.com/start", "POST");
    expect_eq(loop_walk.recordHop("https://b.example.com/mid", 307, "Temp"), true);
    expect_eq(std.mem.eql(u8, loop_walk.current_method, "POST"), true);
    expect_eq(loop_walk.recordHop("https://a.example.com/start", 303, "See Other"), false);
    expect_eq(loop_walk.stop_reason.? == RedirectStopReason.RedirectLoop, true);

    // Origin tainting across cross-origin hops
    let taint_walk = RedirectWalkState.init("https://a.example.com/1", "GET");
    taint_walk.taint_origin_from = "https://init.example.com";
    expect_eq(taint_walk.recordHop("https://b.example.com/2", 302, "Found"), true);
    expect_eq(taint_walk.origin_tainted, true);

    // Method/body/auth/referrer rules
    expect_eq(isRedirectStatus(308), true);
    expect_eq(isRedirectStatus(200), false);
    expect_eq(std.mem.eql(u8, computeNextMethod(303, "DELETE"), "GET"), true);
    expect_eq(shouldDiscardRequestBody(308, "POST"), false);
    expect_eq(isCrossOrigin("https://a.com", "https://a.com:443/x"), false);
    expect_eq(isProtocolDowngrade("https://a.com", "http://a.com"), true);
    expect_eq(shouldStripAuthorizationOnRedirect("https://a.com", "https://b.com"), true);
    expect_eq(parseUrlOrigin("not-a-url") == null, true);

    let ref_buf: [128]u8 = [0; 128];
    expect_eq(computeNextReferrer("", "https://a.com/1", "https://a.com/2", ref_buf[0..]).len, 0);
}

fn testEmptyAndMalformedHeaderBoundaries()! {
    let entries: [4]HeaderEntry = [HeaderEntry{}; 4];

    // Empty & CRLF-only payloads
    expect_eq(HeadersParser.parseRaw("", entries[0..]), 0);
    expect_eq(HeadersParser.parseRaw("\r\n\r\n\r\n", entries[0..]), 0);

    // Malformed lines (no colon), empty name, empty value, whitespace trimming, and buffer cap truncation
    const malformed =
        "NoColonLine\r\n" ++
        ":empty-name-val\r\n" ++
        "X-Empty-Val:   \t \r\n" ++
        "  X-Trimmed  : \t hello world \t\r\n" ++
        "X-Third: 3\r\n" ++
        "X-Fourth: 4\r\n" ++
        "X-Fifth-Dropped: 5\r\n";

    const parsed_count = HeadersParser.parseRaw(malformed, entries[0..]);
    expect_eq(parsed_count, 4);
    expect_eq(entries[0].name.len, 0);
    expect_eq(std.mem.eql(u8, entries[0].value, "empty-name-val"), true);
    expect_eq(std.mem.eql(u8, entries[1].name, "X-Empty-Val"), true);
    expect_eq(entries[1].value.len, 0);
    expect_eq(std.mem.eql(u8, entries[2].name, "X-Trimmed"), true);
    expect_eq(std.mem.eql(u8, entries[2].value, "hello world"), true);
    expect_eq(std.mem.eql(u8, entries[3].name, "X-Third"), true);

    // Zero-capacity output slice
    expect_eq(HeadersParser.parseRaw("Host: example.com\r\n", entries[0..0]), 0);

    // parseTokenList empty, comma-only, invalid token, and > MAX_TOKEN_LIST_SIZE (32) truncation
    if (empty_list := parseTokenList("")) {
        expect_eq(empty_list.len, 0);
    } else {
        expect_eq(false, true);
    }

    if (comma_list := parseTokenList(" , \t , ")) {
        expect_eq(comma_list.len, 0);
    } else {
        expect_eq(false, true);
    }

    expect_eq(isHttpToken("bad token"), false);
    expect_eq(parseTokenList("GET, bad token") == null, true);
    expect_eq(parseTokenList("GET, bad\nheader") == null, true);

    // 35 valid tokens -> capped at MAX_TOKEN_LIST_SIZE (32) without overflow
    const many_tokens =
        "t1,t2,t3,t4,t5,t6,t7,t8,t9,t10," ++
        "t11,t12,t13,t14,t15,t16,t17,t18,t19,t20," ++
        "t21,t22,t23,t24,t25,t26,t27,t28,t29,t30," ++
        "t31,t32,t33,t34,t35";
    if (capped := parseTokenList(many_tokens)) {
        expect_eq(capped.len, MAX_TOKEN_LIST_SIZE);
        expect_eq(capped.contains("t32"), true);
        expect_eq(capped.contains("t33"), false);
    } else {
        expect_eq(false, true);
    }
}

fn main() {
    test("1. Origin Policy Edge Cases (Opaque, Default Ports, IPv6)", () => {
        testOriginPolicyEdgeCases() catch unreachable;
    });

    test("2. CORS Check & Preflight Boundary Conditions", () => {
        testCorsCheckAndPreflightEdge() catch unreachable;
    });

    test("3. CORS Safelist Caps, Unsafe Bytes & PreflightCache Eviction/TTL", () => {
        testCorsSafelistAndPreflightCacheEdge() catch unreachable;
    });

    test("4. Forbidden Request/Response Headers & Syntax Validation", () => {
        testForbiddenHeadersAndValidationEdge() catch unreachable;
    });

    test("5. Redirect Policy Max-Hops Clamping, Loops, Schemes & Tainting", () => {
        testRedirectPolicyEdgeCases() catch unreachable;
    });

    test("6. Empty & Malformed Header/Token-List Parsing Boundaries", () => {
        testEmptyAndMalformedHeaderBoundaries() catch unreachable;
    });
}
