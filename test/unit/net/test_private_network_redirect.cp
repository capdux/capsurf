// test/net/test_private_network_redirect.cp
// Tests for W3C Private Network Access (PNA) and Redirect Policy modules.

import {
    AddressSpace,
    ipv4Space,
    ipv6Space,
    addressSpaceOf,
    privateNetworkRefusal,
    privateNetworkRefusalReason,
} from "../../../src/net/private_network.cp";
import {
    MAX_REDIRECT_HOPS,
    isRedirectStatus,
    computeNextMethod,
    shouldDiscardRequestBody,
    isCrossOrigin,
    isProtocolDowngrade,
    shouldStripReferrerOnRedirect,
    computeNextReferrer,
    checkRedirectLoop,
    RedirectHop,
    RedirectWalkState,
    RedirectStopReason,
} from "../../../src/net/redirect_walk.cp";

fn check_eq(actual: auto, expected: auto) {
    if (actual != expected) {
        std.debug.panic("Assertion failed!", .{});
    }
}

fn main() {
    // 1. AddressSpace enum rank & str
    {
        check_eq(@intFromEnum(AddressSpace.loopback), @as(u8, 0));
        check_eq(@intFromEnum(AddressSpace.@"private"), @as(u8, 1));
        check_eq(@intFromEnum(AddressSpace.@"public"), @as(u8, 2));

        check_eq(std.mem.eql(u8, AddressSpace.loopback.asStr(), "loopback"), true);
        check_eq(std.mem.eql(u8, AddressSpace.@"private".asStr(), "private"), true);
        check_eq(std.mem.eql(u8, AddressSpace.@"public".asStr(), "public"), true);
    }

    // 2. IPv4 address classification
    {
        // Loopback (127.0.0.0/8 and 0.0.0.0/8)
        check_eq(ipv4Space("127.0.0.1"), AddressSpace.loopback);
        check_eq(ipv4Space("127.255.255.254"), AddressSpace.loopback);
        check_eq(ipv4Space("0.0.0.0"), AddressSpace.loopback);
        check_eq(ipv4Space("127.0.0.1:8080"), AddressSpace.loopback);

        // Private RFC 1918 (10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16)
        check_eq(ipv4Space("10.0.0.1"), AddressSpace.@"private");
        check_eq(ipv4Space("10.254.1.2:3000"), AddressSpace.@"private");
        check_eq(ipv4Space("172.16.0.1"), AddressSpace.@"private");
        check_eq(ipv4Space("172.31.255.254"), AddressSpace.@"private");
        check_eq(ipv4Space("192.168.1.1"), AddressSpace.@"private");
        check_eq(ipv4Space("192.168.100.200"), AddressSpace.@"private");

        // 172.15 and 172.32 are outside 172.16.0.0/12 -> public
        check_eq(ipv4Space("172.15.255.1"), AddressSpace.@"public");
        check_eq(ipv4Space("172.32.0.1"), AddressSpace.@"public");

        // Link-local (169.254.0.0/16)
        check_eq(ipv4Space("169.254.1.1"), AddressSpace.@"private");
        check_eq(ipv4Space("169.254.254.254"), AddressSpace.@"private");

        // Shared address space / Carrier Grade NAT (RFC 6598 100.64.0.0/10)
        check_eq(ipv4Space("100.64.0.1"), AddressSpace.@"private");
        check_eq(ipv4Space("100.127.255.254"), AddressSpace.@"private");
        check_eq(ipv4Space("100.63.255.255"), AddressSpace.@"public");
        check_eq(ipv4Space("100.128.0.0"), AddressSpace.@"public");

        // Multicast (224.0.0.0/4) and Reserved (240.0.0.0/4)
        check_eq(ipv4Space("224.0.0.1"), AddressSpace.@"private");
        check_eq(ipv4Space("239.255.255.250"), AddressSpace.@"private");
        check_eq(ipv4Space("240.0.0.1"), AddressSpace.@"private");

        // Public IPs
        check_eq(ipv4Space("8.8.8.8"), AddressSpace.@"public");
        check_eq(ipv4Space("1.1.1.1"), AddressSpace.@"public");
        check_eq(ipv4Space("93.184.216.34"), AddressSpace.@"public");
    }

    // 3. IPv6 address classification
    {
        // Loopback
        check_eq(ipv6Space("::1"), AddressSpace.loopback);
        check_eq(ipv6Space("::"), AddressSpace.loopback);
        check_eq(ipv6Space("[::1]"), AddressSpace.loopback);
        check_eq(ipv6Space("0:0:0:0:0:0:0:1"), AddressSpace.loopback);

        // IPv4-mapped ::ffff:x.x.x.x and ::ffff:hi:lo
        check_eq(ipv6Space("::ffff:127.0.0.1"), AddressSpace.loopback);
        check_eq(ipv6Space("::ffff:192.168.1.1"), AddressSpace.@"private");
        check_eq(ipv6Space("::ffff:10.0.0.1"), AddressSpace.@"private");
        check_eq(ipv6Space("::ffff:c0a8:0101"), AddressSpace.@"private");
        check_eq(ipv6Space("::ffff:8.8.8.8"), AddressSpace.@"public");

        // Unique local fc00::/7
        check_eq(ipv6Space("fc00::1"), AddressSpace.@"private");
        check_eq(ipv6Space("fd12:3456:789a::1"), AddressSpace.@"private");

        // Link-local fe80::/10
        check_eq(ipv6Space("fe80::1"), AddressSpace.@"private");
        check_eq(ipv6Space("fe80::1ff:fe00:1"), AddressSpace.@"private");

        // Multicast ff00::/8
        check_eq(ipv6Space("ff02::1"), AddressSpace.@"private");
        check_eq(ipv6Space("ff05::2"), AddressSpace.@"private");

        // Global unicast public IPv6
        check_eq(ipv6Space("2001:4860:4860::8888"), AddressSpace.@"public");
        check_eq(ipv6Space("2606:4700:4700::1111"), AddressSpace.@"public");
    }

    // 4. addressSpaceOf full URLs and hosts
    {
        check_eq(addressSpaceOf("http://localhost:3000/api"), AddressSpace.loopback);
        check_eq(addressSpaceOf("http://sub.localhost/path"), AddressSpace.loopback);
        check_eq(addressSpaceOf("localhost."), AddressSpace.loopback);
        check_eq(addressSpaceOf("http://127.0.0.1:8080/"), AddressSpace.loopback);
        check_eq(addressSpaceOf("http://[::1]:8080/"), AddressSpace.loopback);

        check_eq(addressSpaceOf("http://192.168.1.1/setup"), AddressSpace.@"private");
        check_eq(addressSpaceOf("https://10.0.0.5:8443/login"), AddressSpace.@"private");
        check_eq(addressSpaceOf("http://[fe80::1]:80/"), AddressSpace.@"private");

        check_eq(addressSpaceOf("https://example.com/index.html"), AddressSpace.@"public");
        check_eq(addressSpaceOf("https://8.8.8.8/dns-query"), AddressSpace.@"public");
        check_eq(addressSpaceOf("google.com"), AddressSpace.@"public");
    }

    // 5. privateNetworkRefusal PNA policy decisions
    {
        // Public document attempting to reach private/loopback -> REFUSED
        check_eq(privateNetworkRefusal("http://localhost:8080", "https://example.com"), true);
        check_eq(privateNetworkRefusal("http://127.0.0.1", "https://example.com"), true);
        check_eq(privateNetworkRefusal("http://192.168.1.1", "https://example.com"), true);
        check_eq(privateNetworkRefusal("http://10.0.0.1", "https://example.com"), true);

        // Private document attempting to reach loopback -> REFUSED
        check_eq(privateNetworkRefusal("http://127.0.0.1", "http://192.168.1.50"), true);
        check_eq(privateNetworkRefusal("http://localhost:3000", "http://10.0.0.2"), true);

        // Private document reaching private -> ALLOWED
        check_eq(privateNetworkRefusal("http://10.0.0.1", "http://192.168.1.50"), false);
        check_eq(privateNetworkRefusal("http://192.168.1.1", "http://192.168.1.50"), false);

        // Private document reaching public -> ALLOWED
        check_eq(privateNetworkRefusal("https://example.com", "http://192.168.1.50"), false);

        // Loopback document reaching anything -> ALLOWED
        check_eq(privateNetworkRefusal("http://127.0.0.1", "http://localhost:3000"), false);
        check_eq(privateNetworkRefusal("http://192.168.1.1", "http://localhost:3000"), false);
        check_eq(privateNetworkRefusal("https://example.com", "http://localhost:3000"), false);

        // Opaque / unknown initiator counts as public -> REFUSED for private targets
        check_eq(privateNetworkRefusal("http://192.168.1.1", ""), true);
        check_eq(privateNetworkRefusal("https://example.com", ""), false);

        // Refusal reason message formatting
        let msg_buf: [256]u8 = [0; 256];
        const reason = privateNetworkRefusalReason("http://192.168.1.1:80/admin", "https://example.com", &msg_buf);
        check_eq(std.mem.startsWith(u8, reason, "capsurf: a public document may not reach the private address"), true);
    }

    // 6. HTTP redirect status codes and method computation
    {
        // Status codes
        check_eq(isRedirectStatus(301), true);
        check_eq(isRedirectStatus(302), true);
        check_eq(isRedirectStatus(303), true);
        check_eq(isRedirectStatus(307), true);
        check_eq(isRedirectStatus(308), true);
        check_eq(isRedirectStatus(200), false);
        check_eq(isRedirectStatus(304), false);
        check_eq(isRedirectStatus(404), false);

        // 301, 302, 303 convert POST to GET and discard request body
        check_eq(std.mem.eql(u8, computeNextMethod(301, "POST"), "GET"), true);
        check_eq(std.mem.eql(u8, computeNextMethod(302, "POST"), "GET"), true);
        check_eq(std.mem.eql(u8, computeNextMethod(303, "POST"), "GET"), true);
        check_eq(std.mem.eql(u8, computeNextMethod(303, "PUT"), "GET"), true);
        check_eq(std.mem.eql(u8, computeNextMethod(301, "GET"), "GET"), true);

        check_eq(shouldDiscardRequestBody(301, "POST"), true);
        check_eq(shouldDiscardRequestBody(302, "POST"), true);
        check_eq(shouldDiscardRequestBody(303, "POST"), true);
        check_eq(shouldDiscardRequestBody(303, "GET"), true);

        // 307 and 308 strictly preserve method and body
        check_eq(std.mem.eql(u8, computeNextMethod(307, "POST"), "POST"), true);
        check_eq(std.mem.eql(u8, computeNextMethod(308, "POST"), "POST"), true);
        check_eq(std.mem.eql(u8, computeNextMethod(307, "PUT"), "PUT"), true);
        check_eq(shouldDiscardRequestBody(307, "POST"), false);
        check_eq(shouldDiscardRequestBody(308, "POST"), false);
    }

    // 7. Cross-origin and Referrer stripping policies on redirect
    {
        // Same-origin checks
        check_eq(isCrossOrigin("https://a.test/page1", "https://a.test/page2"), false);
        check_eq(isCrossOrigin("https://a.test:443/page1", "https://a.test/page2"), false);
        check_eq(isCrossOrigin("http://a.test:80/page1", "http://a.test/page2"), false);

        // Cross-origin checks
        check_eq(isCrossOrigin("https://a.test/page1", "https://b.test/page2"), true);
        check_eq(isCrossOrigin("https://a.test/page1", "http://a.test/page2"), true);
        check_eq(isCrossOrigin("http://a.test:80/page1", "http://a.test/page2"), false);
        check_eq(isCrossOrigin("http://a.test:80/page1", "http://a.test:8080/page2"), true);

        // Protocol downgrade
        check_eq(isProtocolDowngrade("https://a.test/page", "http://a.test/page"), true);
        check_eq(isProtocolDowngrade("https://a.test/page", "https://b.test/page"), false);
        check_eq(isProtocolDowngrade("http://a.test/page", "http://b.test/page"), false);

        // shouldStripReferrerOnRedirect: true on downgrade or cross-origin
        check_eq(shouldStripReferrerOnRedirect("https://a.test/1", "https://a.test/2"), false);
        check_eq(shouldStripReferrerOnRedirect("https://a.test/1", "http://a.test/2"), true);
        check_eq(shouldStripReferrerOnRedirect("https://a.test/1", "https://b.test/2"), true);

        // computeNextReferrer
        let ref_buf: [256]u8 = [0; 256];
        const same_ref = computeNextReferrer("https://a.test/path?q=1", "https://a.test/1", "https://a.test/2", &ref_buf);
        check_eq(std.mem.eql(u8, same_ref, "https://a.test/path?q=1"), true);

        const cross_ref = computeNextReferrer("https://a.test/path?q=1", "https://a.test/1", "https://b.test/2", &ref_buf);
        check_eq(std.mem.eql(u8, cross_ref, "https://a.test/"), true);

        const down_ref = computeNextReferrer("https://a.test/path?q=1", "https://a.test/1", "http://a.test/2", &ref_buf);
        check_eq(down_ref.len, 0);
    }

    // 8. Redirect loop detection and RedirectWalkState tracker
    {
        let visited: [3][]const u8 = [
            "https://a.test/entry",
            "https://a.test/hop1",
            "https://a.test/hop2",
        ];
        check_eq(checkRedirectLoop(&visited, "https://a.test/entry"), true);
        check_eq(checkRedirectLoop(&visited, "https://a.test/hop1"), true);
        check_eq(checkRedirectLoop(&visited, "https://a.test/hop3"), false);

        // Test fixed-length RedirectWalkState
        let state = RedirectWalkState.initWithMaxHops("https://a.test/entry", "POST", 3);
        check_eq(state.hop_count, @as(u32, 0));
        check_eq(state.hasExceededMaxHops(), false);

        // Hop 1: 302 -> POST converts to GET
        const ok1 = state.recordHop("https://a.test/hop1", 302, "Found");
        check_eq(ok1, true);
        check_eq(state.hop_count, @as(u32, 1));
        check_eq(std.mem.eql(u8, state.current_method, "GET"), true);
        check_eq(std.mem.eql(u8, state.current_url, "https://a.test/hop1"), true);

        // Hop 2: 307 -> GET preserved
        const ok2 = state.recordHop("https://b.test/hop2", 307, "Temporary Redirect");
        check_eq(ok2, true);
        check_eq(state.hop_count, @as(u32, 2));
        check_eq(state.hops[1].cross_origin, true);

        // Attempting loop back to entry -> must fail with RedirectLoop
        const ok_loop = state.recordHop("https://a.test/entry", 302, "Found");
        check_eq(ok_loop, false);
        check_eq(state.stop_reason.?, RedirectStopReason.RedirectLoop);

        // Valid Hop 3 -> reaches max_hops
        const ok3 = state.recordHop("https://c.test/hop3", 302, "Found");
        check_eq(ok3, true);
        check_eq(state.hop_count, @as(u32, 3));
        check_eq(state.hasExceededMaxHops(), true);

        // Hop 4 -> exceeds max hops
        const ok4 = state.recordHop("https://d.test/hop4", 302, "Found");
        check_eq(ok4, false);
        check_eq(state.stop_reason.?, RedirectStopReason.MaxHopsExceeded);

        const hops = state.getHops();
        check_eq(hops.len, 3);
        check_eq(std.mem.eql(u8, hops[0].from_url, "https://a.test/entry"), true);
        check_eq(std.mem.eql(u8, hops[0].new_url, "https://a.test/hop1"), true);
        check_eq(hops[0].discard_body, true);
    }

    std.debug.print("ALL PNA AND REDIRECT TESTS PASSED SUCCESSFULLY!\n", .{});
}
