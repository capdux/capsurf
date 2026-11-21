// test/net/test_redirect_referrer.cp
// CapSurf Pure CP White-Box Unit Tests for Redirect Walk, Referrer Policy & Forbidden Headers

import { test, expect_eq } from "cap:test";
import {
    MAX_REDIRECT_HOPS,
    isRedirectStatus,
    isCrossOrigin,
    isProtocolDowngrade,
    computeNextMethod,
    shouldDiscardRequestBody,
    shouldStripReferrerOnRedirect,
    computeNextReferrer,
    shouldStripAuthorizationOnRedirect,
    isBodyRelatedHeader,
    RedirectStopReason,
    RedirectWalkState,
} from "../../../src/net/redirect_walk.cp";
import {
    ReferrerPolicy,
    parseReferrerPolicy,
    determineReferrer,
} from "../../../src/net/referrer_policy.cp";
import {
    isForbiddenRequestHeaderName,
    isForbiddenMethod,
    isForbiddenResponseHeader,
    isForbiddenRequestHeader,
} from "../../../src/net/forbidden_headers.cp";

fn testRedirectStatusAndMethodDowngrade()! {
    // 1. Redirect status codes (301, 302, 303, 307, 308)
    expect_eq(isRedirectStatus(301), true);
    expect_eq(isRedirectStatus(302), true);
    expect_eq(isRedirectStatus(303), true);
    expect_eq(isRedirectStatus(307), true);
    expect_eq(isRedirectStatus(308), true);
    expect_eq(isRedirectStatus(200), false);
    expect_eq(isRedirectStatus(304), false);
    expect_eq(isRedirectStatus(404), false);

    // 2. Method downgrade: 301/302 POST->GET, 303 always GET, 307/308 preserve method
    expect_eq(std.mem.eql(u8, computeNextMethod(301, "POST"), "GET"), true);
    expect_eq(std.mem.eql(u8, computeNextMethod(302, "post"), "GET"), true);
    expect_eq(std.mem.eql(u8, computeNextMethod(303, "PUT"), "GET"), true);
    expect_eq(std.mem.eql(u8, computeNextMethod(303, "DELETE"), "GET"), true);
    expect_eq(std.mem.eql(u8, computeNextMethod(307, "POST"), "POST"), true);
    expect_eq(std.mem.eql(u8, computeNextMethod(308, "PUT"), "PUT"), true);

    // 3. Request body discard rules
    expect_eq(shouldDiscardRequestBody(301, "POST"), true);
    expect_eq(shouldDiscardRequestBody(302, "POST"), true);
    expect_eq(shouldDiscardRequestBody(303, "PUT"), true);
    expect_eq(shouldDiscardRequestBody(307, "POST"), false);
    expect_eq(shouldDiscardRequestBody(308, "POST"), false);
}

fn testRedirectCrossOriginAndSensitiveHeaders()! {
    const src = "https://app.example.com/login";
    const same_tgt = "https://app.example.com/dashboard";
    const cross_tgt = "https://cdn.other.org/asset";
    const port_diff = "https://app.example.com:8443/api";
    const down_tgt = "http://app.example.com/insecure";

    // 1. Cross-origin and downgrade detection
    expect_eq(isCrossOrigin(src, same_tgt), false);
    expect_eq(isCrossOrigin(src, cross_tgt), true);
    expect_eq(isCrossOrigin(src, port_diff), true);
    expect_eq(isProtocolDowngrade(src, cross_tgt), false);
    expect_eq(isProtocolDowngrade(src, down_tgt), true);

    // 2. Sensitive header stripping on redirect
    expect_eq(shouldStripAuthorizationOnRedirect(src, same_tgt), false);
    expect_eq(shouldStripAuthorizationOnRedirect(src, cross_tgt), true);
    expect_eq(shouldStripAuthorizationOnRedirect(src, port_diff), true);

    expect_eq(shouldStripReferrerOnRedirect(src, same_tgt), false);
    expect_eq(shouldStripReferrerOnRedirect(src, cross_tgt), true);
    expect_eq(shouldStripReferrerOnRedirect(src, down_tgt), true);

    // 3. computeNextReferrer across same-origin, cross-origin, and downgrade
    let origin_buf: [256]u8 = [0; 256];
    const current_ref = "https://app.example.com/form?step=1";

    const ref_same = computeNextReferrer(current_ref, src, same_tgt, origin_buf[0..]);
    expect_eq(std.mem.eql(u8, ref_same, current_ref), true);

    const ref_cross = computeNextReferrer(current_ref, src, cross_tgt, origin_buf[0..]);
    expect_eq(std.mem.eql(u8, ref_cross, "https://app.example.com/"), true);

    const ref_down = computeNextReferrer(current_ref, src, down_tgt, origin_buf[0..]);
    expect_eq(ref_down.len, 0);

    // 4. Body-related header detection for POST->GET downgrade
    expect_eq(isBodyRelatedHeader("Content-Type"), true);
    expect_eq(isBodyRelatedHeader("content-length"), true);
    expect_eq(isBodyRelatedHeader("Content-Encoding"), true);
    expect_eq(isBodyRelatedHeader("X-Request-Id"), false);
}

fn testRedirectWalkMaxHopsAndStopReasons()! {
    // 1. Max hops limit enforcement
    let state = RedirectWalkState.initWithMaxHops("https://a.example.com/0", "POST", 2);
    expect_eq(state.max_hops, 2);
    expect_eq(state.hasExceededMaxHops(), false);

    expect_eq(state.recordHop("https://a.example.com/1", 302, "Found"), true);
    expect_eq(std.mem.eql(u8, state.current_method, "GET"), true);
    expect_eq(state.hop_count, 1);

    expect_eq(state.recordHop("https://b.example.com/2", 307, "Temporary Redirect"), true);
    expect_eq(state.hop_count, 2);
    expect_eq(state.hasExceededMaxHops(), true);

    const hops = state.getHops();
    expect_eq(hops.len, 2);
    expect_eq(hops[0].discard_body, true);
    expect_eq(hops[1].cross_origin, true);

    // 3rd hop exceeds max_hops=2
    expect_eq(state.recordHop("https://c.example.com/3", 302, "Found"), false);
    expect_eq(state.stop_reason != null, true);
    if (reason := state.stop_reason) {
        expect_eq(reason == RedirectStopReason.MaxHopsExceeded, true);
    }

    // 2. Redirect loop detection
    let loop_state = RedirectWalkState.init("https://loop.example.com/a", "GET");
    expect_eq(loop_state.max_hops, MAX_REDIRECT_HOPS);
    expect_eq(loop_state.recordHop("https://loop.example.com/b", 301, "Moved"), true);
    expect_eq(loop_state.recordHop("https://loop.example.com/a", 302, "Found"), false);
    if (reason := loop_state.stop_reason) {
        expect_eq(reason == RedirectStopReason.RedirectLoop, true);
    }

    // 3. Invalid scheme & non-redirect status
    let scheme_state = RedirectWalkState.init("https://safe.example.com/", "GET");
    expect_eq(scheme_state.recordHop("javascript:alert(1)", 302, "Found"), false);
    if (reason := scheme_state.stop_reason) {
        expect_eq(reason == RedirectStopReason.InvalidScheme, true);
    }

    let non_redir = RedirectWalkState.init("https://safe.example.com/", "GET");
    expect_eq(non_redir.recordHop("https://safe.example.com/ok", 200, "OK"), false);
    if (reason := non_redir.stop_reason) {
        expect_eq(reason == RedirectStopReason.NonRedirectStatus, true);
    }
}

fn testReferrerPolicyDetermineReferrer()! {
    const src_https = "https://user:secret@portal.example.com/app/page?q=1#hash";
    const tgt_same = "https://portal.example.com/app/next";
    const tgt_cross = "https://external.example.org/hook";
    const tgt_http = "http://legacy.example.org/plain";

    // Policy parsing sanity
    expect_eq(parseReferrerPolicy("no-referrer") == ReferrerPolicy.NoReferrer, true);
    expect_eq(parseReferrerPolicy("unsafe-url") == ReferrerPolicy.UnsafeUrl, true);

    // 1. no-referrer: always empty
    expect_eq(determineReferrer(src_https, tgt_same, ReferrerPolicy.NoReferrer).len, 0);
    expect_eq(determineReferrer(src_https, tgt_cross, ReferrerPolicy.NoReferrer).len, 0);
    expect_eq(determineReferrer(src_https, tgt_http, ReferrerPolicy.NoReferrer).len, 0);

    // 2. origin: always origin only (userinfo stripped)
    expect_eq(std.mem.eql(u8, determineReferrer(src_https, tgt_same, ReferrerPolicy.Origin), "https://portal.example.com/"), true);
    expect_eq(std.mem.eql(u8, determineReferrer(src_https, tgt_cross, ReferrerPolicy.Origin), "https://portal.example.com/"), true);
    expect_eq(std.mem.eql(u8, determineReferrer(src_https, tgt_http, ReferrerPolicy.Origin), "https://portal.example.com/"), true);

    // 3. strict-origin-when-cross-origin: full on same-origin, origin on cross HTTPS, empty on HTTPS->HTTP
    expect_eq(
        std.mem.eql(u8, determineReferrer(src_https, tgt_same, ReferrerPolicy.StrictOriginWhenCrossOrigin), "https://portal.example.com/app/page?q=1"),
        true,
    );
    expect_eq(
        std.mem.eql(u8, determineReferrer(src_https, tgt_cross, ReferrerPolicy.StrictOriginWhenCrossOrigin), "https://portal.example.com/"),
        true,
    );
    expect_eq(determineReferrer(src_https, tgt_http, ReferrerPolicy.StrictOriginWhenCrossOrigin).len, 0);

    // 4. unsafe-url: full URL (without userinfo/fragment) even on cross-origin and downgrade
    expect_eq(
        std.mem.eql(u8, determineReferrer(src_https, tgt_cross, ReferrerPolicy.UnsafeUrl), "https://portal.example.com/app/page?q=1"),
        true,
    );
    expect_eq(
        std.mem.eql(u8, determineReferrer(src_https, tgt_http, ReferrerPolicy.UnsafeUrl), "https://portal.example.com/app/page?q=1"),
        true,
    );
}

fn testForbiddenHeadersFiltering()! {
    // 1. Case-insensitive forbidden request header names
    expect_eq(isForbiddenRequestHeaderName("Host"), true);
    expect_eq(isForbiddenRequestHeaderName("HOST"), true);
    expect_eq(isForbiddenRequestHeaderName("Cookie"), true);
    expect_eq(isForbiddenRequestHeaderName("cookie"), true);
    expect_eq(isForbiddenRequestHeaderName("Origin"), true);
    expect_eq(isForbiddenRequestHeaderName("Referer"), true);
    expect_eq(isForbiddenRequestHeaderName("Sec-Fetch-Site"), true);
    expect_eq(isForbiddenRequestHeaderName("sec-ch-ua"), true);
    expect_eq(isForbiddenRequestHeaderName("Proxy-Authorization"), true);
    expect_eq(isForbiddenRequestHeaderName("Content-Length"), true);

    // 2. Normal custom & allowed headers pass through
    expect_eq(isForbiddenRequestHeaderName("X-Custom-Token"), false);
    expect_eq(isForbiddenRequestHeaderName("Authorization"), false);
    expect_eq(isForbiddenRequestHeaderName("Content-Type"), false);
    expect_eq(isForbiddenRequestHeader("X-Custom-Token", "abc123"), false);
    expect_eq(isForbiddenRequestHeader("Accept", "application/json"), false);

    // 3. Method-override headers block forbidden methods only
    expect_eq(isForbiddenMethod("TRACE"), true);
    expect_eq(isForbiddenMethod("connect"), true);
    expect_eq(isForbiddenMethod("DELETE"), false);
    expect_eq(isForbiddenRequestHeader("X-HTTP-Method-Override", "DELETE"), false);
    expect_eq(isForbiddenRequestHeader("x-http-method-override", "GET, trace"), true);
    expect_eq(isForbiddenRequestHeader("X-Method-Override", "TRACK"), true);

    // 4. Forbidden response headers
    expect_eq(isForbiddenResponseHeader("Set-Cookie"), true);
    expect_eq(isForbiddenResponseHeader("set-cookie2"), true);
    expect_eq(isForbiddenResponseHeader("Cache-Control"), false);
}

fn main() {
    test("1. Redirect Status Codes & Method Downgrade Rules", () => {
        testRedirectStatusAndMethodDowngrade() catch unreachable;
    });

    test("2. Cross-Origin Sensitive Header & Referrer Stripping on Redirect", () => {
        testRedirectCrossOriginAndSensitiveHeaders() catch unreachable;
    });

    test("3. RedirectWalkState Max Hops, Loop & Stop Reason Enforcement", () => {
        testRedirectWalkMaxHopsAndStopReasons() catch unreachable;
    });

    test("4. Referrer-Policy Evaluation Across Same-Origin, Cross-Origin & Downgrade", () => {
        testReferrerPolicyDetermineReferrer() catch unreachable;
    });

    test("5. Forbidden Request/Response Headers & Method Override Filtering", () => {
        testForbiddenHeadersFiltering() catch unreachable;
    });
}
