// test/net/test_security_gate.cp
// CapSurf Phase 9 Comprehensive Security Gate Test Suite (Referrer-Policy & CSP)

import { test, expect_eq } from "cap:test";
import {
    ReferrerPolicy,
    parseReferrerPolicy,
    isPotentiallyTrustworthy,
    determineReferrer,
} from "../../src/net/referrer_policy.cp";
import {
    CspPolicy,
    parseCsp,
    schemeMatches,
    hostMatches,
    isSourceAllowed,
} from "../../src/net/csp.cp";

fn testReferrerPolicyParsingAndTrust()! {
    // 1. Parsing token precedence (last recognized wins)
    expect_eq(parseReferrerPolicy("no-referrer") == ReferrerPolicy.NoReferrer, true);
    expect_eq(parseReferrerPolicy("no-referrer, origin") == ReferrerPolicy.Origin, true);
    expect_eq(parseReferrerPolicy("strict-origin-when-cross-origin, same-origin") == ReferrerPolicy.SameOrigin, true);
    expect_eq(parseReferrerPolicy("unknown-policy") == ReferrerPolicy.StrictOriginWhenCrossOrigin, true);
    expect_eq(parseReferrerPolicy("") == ReferrerPolicy.StrictOriginWhenCrossOrigin, true);

    // 2. Potentially trustworthy URLs
    expect_eq(isPotentiallyTrustworthy("about:blank"), true);
    expect_eq(isPotentiallyTrustworthy("about:srcdoc"), true);
    expect_eq(isPotentiallyTrustworthy("data:text/html,<h1>hi</h1>"), true);
    expect_eq(isPotentiallyTrustworthy("https://example.com"), true);
    expect_eq(isPotentiallyTrustworthy("wss://example.com/ws"), true);
    expect_eq(isPotentiallyTrustworthy("http://localhost:8080/app"), true);
    expect_eq(isPotentiallyTrustworthy("http://127.0.0.1:9222"), true);
    expect_eq(isPotentiallyTrustworthy("http://insecure.site.org"), false);
}

fn testReferrerDetermination()! {
    const src_https = "https://mybank.com/account/details#sec";
    const tgt_same = "https://mybank.com/transfer";
    const tgt_cross = "https://partner.com/verify";
    const tgt_insecure = "http://insecure.partner.com/verify";

    // 1. NoReferrer: always empty
    expect_eq(determineReferrer(src_https, tgt_same, ReferrerPolicy.NoReferrer).len, 0);

    // 2. SameOrigin: full url on same origin, empty on cross origin
    const ref_same = determineReferrer(src_https, tgt_same, ReferrerPolicy.SameOrigin);
    expect_eq(std.mem.eql(u8, ref_same, "https://mybank.com/account/details"), true);
    expect_eq(determineReferrer(src_https, tgt_cross, ReferrerPolicy.SameOrigin).len, 0);

    // 3. Origin: always origin only
    const ref_orig = determineReferrer(src_https, tgt_cross, ReferrerPolicy.Origin);
    expect_eq(std.mem.eql(u8, ref_orig, "https://mybank.com/"), true);

    // 4. StrictOriginWhenCrossOrigin: full on same origin, origin on cross secure, empty on downgrade
    const ref_strict_same = determineReferrer(src_https, tgt_same, ReferrerPolicy.StrictOriginWhenCrossOrigin);
    expect_eq(std.mem.eql(u8, ref_strict_same, "https://mybank.com/account/details"), true);

    const ref_strict_cross = determineReferrer(src_https, tgt_cross, ReferrerPolicy.StrictOriginWhenCrossOrigin);
    expect_eq(std.mem.eql(u8, ref_strict_cross, "https://mybank.com/"), true);

    const ref_strict_down = determineReferrer(src_https, tgt_insecure, ReferrerPolicy.StrictOriginWhenCrossOrigin);
    expect_eq(ref_strict_down.len, 0);
}

fn testCspParsingAndMatching()! {
    // 1. Scheme and Host matching
    expect_eq(schemeMatches("http", "https"), true);  // secure upgrade allowed
    expect_eq(schemeMatches("https", "http"), false); // downgrade prohibited
    expect_eq(schemeMatches("ws", "wss"), true);

    expect_eq(hostMatches("*", "any.host.org"), true);
    expect_eq(hostMatches("*.cdn.com", "img.cdn.com"), true);
    expect_eq(hostMatches("*.cdn.com", "cdn.com"), false);
    expect_eq(hostMatches("api.example.com", "api.example.com"), true);
    expect_eq(hostMatches("api.example.com", "auth.example.com"), false);

    // 2. Policy parsing and directive evaluation
    const csp_raw = "default-src 'self'; script-src https://trusted.cdn.com 'unsafe-inline'; connect-src https://api.site.com wss://ws.site.com";
    const policy = parseCsp(csp_raw);
    expect_eq(policy.directive_count >= 3, true);

    // 3. Script-src evaluation
    expect_eq(isSourceAllowed(&policy, "script-src", "https://trusted.cdn.com/bundle.js"), true);
    expect_eq(isSourceAllowed(&policy, "script-src", "https://malicious.evil.com/payload.js"), false);

    // 4. Connect-src evaluation
    expect_eq(isSourceAllowed(&policy, "connect-src", "https://api.site.com/v1/data"), true);
    expect_eq(isSourceAllowed(&policy, "connect-src", "wss://ws.site.com/events"), true);
    expect_eq(isSourceAllowed(&policy, "connect-src", "https://unauthorized.site.com"), false);

    // 5. Fallback to default-src
    expect_eq(isSourceAllowed(&policy, "img-src", "https://site.com/logo.png"), false);
}

fn main() {
    test("1. Referrer Policy Parsing and Trustworthy URL Evaluation", () => {
        testReferrerPolicyParsingAndTrust() catch unreachable;
    });

    test("2. W3C Referrer Determination Across Navigations & Downgrades", () => {
        testReferrerDetermination() catch unreachable;
    });

    test("3. CSP Policy Parsing and Directive Source Allowed Verification", () => {
        testCspParsingAndMatching() catch unreachable;
    });
}
