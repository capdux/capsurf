// test/net/test_cookie_jar_registry.cp
// CapSurf CookieJarRegistry Unit Tests

import { test, expect_eq } from "cap:test";
import { CookieJar } from "../../../src/net/cookies.cp";
import {
    CookieJarRegistry,
    MAX_COOKIE_REGISTRY_ENTRIES,
    MAX_KEY_LEN,
    makeKey,
    cookieKeyFor,
    registerCookieJar,
    getCookieJar,
    unregisterCookieJar,
    unregisterConnection,
    unregisterConnectionCookieJars,
    cookieJarRegistrySize,
    hasCookieJar
} from "../../../src/net/cookie_jar_registry.cp";

fn checkCookieHeader(jar: *CookieJar, url: []const u8, expected: ([]const u8)?): bool {
    const h: ([]const u8)? = jar.cookieHeaderForUrl(url, null, std.heap.page_allocator);
    defer if (h != null) std.heap.page_allocator.free(h.?);
    if (exp := expected) {
        if (h == null) return false;
        return std.mem.eql(u8, h.?, exp);
    }
    return h == null;
}

fn deinitRegistryJars(reg: *CookieJarRegistry, keys: []const []const u8) {
    let i: usize = 0;
    while (i < keys.len) : (i += 1) {
        if (jar := reg.getCookieJar(keys[i])) {
            jar.deinit();
        }
    }
}

fn main() {
    test("1. Context/jar creation, key formatting & multi-tenant isolation", () => {
        let buf: [128]u8 = [0; 128];
        const k_conn = makeKey(buf[0..], "conn-1", "");
        expect_eq(std.mem.eql(u8, k_conn, "conn-1"), true);

        let buf2: [128]u8 = [0; 128];
        const k_ctx = cookieKeyFor(buf2[0..], "conn-1", "ctx-a");
        expect_eq(std.mem.eql(u8, k_ctx, "conn-1::ctx-a"), true);

        let small_buf: [4]u8 = [0; 4];
        expect_eq(makeKey(small_buf[0..], "conn-1", "ctx-a").len == 0, true);
        expect_eq(makeKey(small_buf[0..], "conn-1", "").len == 0, true);

        // Global registry wrappers
        expect_eq(cookieJarRegistrySize() == 0, true);
        const jar_a = CookieJar.init(std.heap.page_allocator);
        const jar_b = CookieJar.init(std.heap.page_allocator);
        const jar_c = CookieJar.init(std.heap.page_allocator);

        expect_eq(registerCookieJar("conn-1::ctx-a", jar_a), true);
        expect_eq(registerCookieJar("conn-1::ctx-b", jar_b), true);
        expect_eq(registerCookieJar("conn-2", jar_c), true);
        expect_eq(cookieJarRegistrySize() == 3, true);
        expect_eq(hasCookieJar("conn-1::ctx-a"), true);
        expect_eq(hasCookieJar("conn-1::ctx-b"), true);
        expect_eq(hasCookieJar("conn-2"), true);
        expect_eq(hasCookieJar("conn-missing"), false);

        // Isolation between contexts and connections
        if (ja := getCookieJar("conn-1::ctx-a")) {
            * = ja.setFromHeader("https://example.com/", "tenant=alpha; Path=/", null);
        } else {
            expect_eq(false, true);
        }

        if (jb := getCookieJar("conn-1::ctx-b")) {
            expect_eq(checkCookieHeader(jb, "https://example.com/", null), true);
            * = jb.setFromHeader("https://example.com/", "tenant=beta; Path=/", null);
        } else {
            expect_eq(false, true);
        }

        if (jc := getCookieJar("conn-2")) {
            expect_eq(checkCookieHeader(jc, "https://example.com/", null), true);
        } else {
            expect_eq(false, true);
        }

        if (ja2 := getCookieJar("conn-1::ctx-a")) {
            expect_eq(checkCookieHeader(ja2, "https://example.com/", "tenant=alpha"), true);
            ja2.deinit();
        }

        // Overwrite existing key does not increment count
        const jar_b2 = CookieJar.init(std.heap.page_allocator);
        if (jb_old := getCookieJar("conn-1::ctx-b")) {
            jb_old.deinit();
        }
        expect_eq(registerCookieJar("conn-1::ctx-b", jar_b2), true);
        expect_eq(cookieJarRegistrySize() == 3, true);

        if (jb_new := getCookieJar("conn-1::ctx-b")) {
            jb_new.deinit();
        }
        if (jc_old := getCookieJar("conn-2")) {
            jc_old.deinit();
        }
        unregisterConnection("conn-1");
        expect_eq(unregisterCookieJar("conn-2"), true);
        expect_eq(cookieJarRegistrySize() == 0, true);
    });

    test("2. Cookie set/get/delete across domains and paths via registry", () => {
        let reg = CookieJarRegistry.init();
        const jar = CookieJar.init(std.heap.page_allocator);
        expect_eq(reg.registerCookieJar("c1::default", jar), true);

        if (j := reg.getCookieJar("c1::default")) {
            * = j.setFromHeader("https://app.example.com/app/home", "sid=111; Domain=example.com; Path=/app", null);
            * = j.setFromHeader("https://app.example.com/", "root=222; Domain=example.com; Path=/", null);

            // Matching subdomain & /app path gets both (longer path first)
            expect_eq(checkCookieHeader(j, "https://api.example.com/app/v1", "sid=111; root=222"), true);

            // Non-matching path /other only gets root=222
            expect_eq(checkCookieHeader(j, "https://api.example.com/other", "root=222"), true);

            // Unrelated domain gets nothing
            expect_eq(checkCookieHeader(j, "https://example.org/app/v1", null), true);

            // Delete sid cookie and verify root remains
            expect_eq(j.remove("sid", "", "example.com", "/app") == 1, true);
            expect_eq(j.getAll().len == 1, true);

            expect_eq(checkCookieHeader(j, "https://api.example.com/app/v1", "root=222"), true);

            j.deinit();
        } else {
            expect_eq(false, true);
        }
        reg.clear();
    });

    test("3. Expiry & registry capacity/key-length bounds", () => {
        let reg = CookieJarRegistry.init();

        // Empty key and > MAX_KEY_LEN key are rejected
        let empty_jar = CookieJar.init(std.heap.page_allocator);
        expect_eq(reg.registerCookieJar("", empty_jar), false);
        empty_jar.deinit();

        let long_key: [129]u8 = ['a'; 129];
        let long_jar = CookieJar.init(std.heap.page_allocator);
        expect_eq(reg.registerCookieJar(long_key[0..], long_jar), false);
        long_jar.deinit();

        // Fill all 16 slots
        const keys: [16][]const u8 = [
            "k00", "k01", "k02", "k03", "k04", "k05", "k06", "k07",
            "k08", "k09", "k10", "k11", "k12", "k13", "k14", "k15"
        ];
        let i: usize = 0;
        while (i < MAX_COOKIE_REGISTRY_ENTRIES) : (i += 1) {
            const j = CookieJar.init(std.heap.page_allocator);
            expect_eq(reg.registerCookieJar(keys[i], j), true);
        }
        expect_eq(reg.size() == MAX_COOKIE_REGISTRY_ENTRIES, true);

        // 17th entry fails
        let extra_jar = CookieJar.init(std.heap.page_allocator);
        expect_eq(reg.registerCookieJar("k16", extra_jar), false);
        extra_jar.deinit();
        expect_eq(reg.size() == MAX_COOKIE_REGISTRY_ENTRIES, true);

        // Cookie expiry in a registered jar (Max-Age=0 & negative Max-Age)
        if (j0 := reg.getCookieJar("k00")) {
            * = j0.setFromHeader("https://example.com/", "live=yes; Max-Age=3600; Path=/", null);
            * = j0.setFromHeader("https://example.com/", "dead1=yes; Path=/", null);
            * = j0.setFromHeader("https://example.com/", "dead1=no; Max-Age=0; Path=/", null);
            * = j0.setFromHeader("https://example.com/", "dead2=no; Max-Age=-1; Path=/", null);
            j0.purgeExpired();

            expect_eq(j0.getAll().len == 1, true);
            expect_eq(checkCookieHeader(j0, "https://example.com/", "live=yes"), true);
            j0.deinit();
        } else {
            expect_eq(false, true);
        }

        // Unregistering slot frees capacity for a new key
        expect_eq(reg.unregisterCookieJar("k00"), true);
        expect_eq(reg.size() == MAX_COOKIE_REGISTRY_ENTRIES - 1, true);
        const reuse_jar = CookieJar.init(std.heap.page_allocator);
        expect_eq(reg.registerCookieJar("k16", reuse_jar), true);
        expect_eq(reg.size() == MAX_COOKIE_REGISTRY_ENTRIES, true);

        if (j16 := reg.getCookieJar("k16")) {
            j16.deinit();
        }
        deinitRegistryJars(&reg, keys[1..]);
        reg.clear();
        expect_eq(reg.size() == 0, true);
    });

    test("4. Clear & destroy lifecycle (unregisterConnection / unregisterConnectionCookieJars / clear)", () => {
        let reg = CookieJarRegistry.init();

        const j_root = CookieJar.init(std.heap.page_allocator);
        const j_c1 = CookieJar.init(std.heap.page_allocator);
        const j_c2 = CookieJar.init(std.heap.page_allocator);
        const j_similar = CookieJar.init(std.heap.page_allocator);
        const j_other = CookieJar.init(std.heap.page_allocator);

        expect_eq(reg.registerCookieJar("connA", j_root), true);
        expect_eq(reg.registerCookieJar("connA::ctx1", j_c1), true);
        expect_eq(reg.registerCookieJar("connA::ctx2", j_c2), true);
        expect_eq(reg.registerCookieJar("connA2", j_similar), true);
        expect_eq(reg.registerCookieJar("connB::ctx1", j_other), true);
        expect_eq(reg.size() == 5, true);

        // unregisterConnectionCookieJars removes connA and specified ctx1, leaving ctx2
        const ctx_slice: [1][]const u8 = ["ctx1"];
        if (jr := reg.getCookieJar("connA")) jr.deinit();
        if (jc1 := reg.getCookieJar("connA::ctx1")) jc1.deinit();
        reg.unregisterConnectionCookieJars("connA", ctx_slice[0..]);

        expect_eq(reg.has("connA"), false);
        expect_eq(reg.has("connA::ctx1"), false);
        expect_eq(reg.has("connA::ctx2"), true);
        expect_eq(reg.has("connA2"), true);
        expect_eq(reg.has("connB::ctx1"), true);
        expect_eq(reg.size() == 3, true);

        // unregisterConnection("connA") removes remaining connA::ctx2, but NOT connA2 or connB::ctx1
        if (jc2 := reg.getCookieJar("connA::ctx2")) jc2.deinit();
        reg.unregisterConnection("connA");
        expect_eq(reg.has("connA::ctx2"), false);
        expect_eq(reg.has("connA2"), true);
        expect_eq(reg.has("connB::ctx1"), true);
        expect_eq(reg.size() == 2, true);

        // Unregistering non-existent key returns false
        expect_eq(reg.unregisterCookieJar("does-not-exist"), false);

        // clear() resets all remaining entries
        if (js := reg.getCookieJar("connA2")) js.deinit();
        if (jo := reg.getCookieJar("connB::ctx1")) jo.deinit();
        reg.clear();
        expect_eq(reg.size() == 0, true);
        expect_eq(reg.has("connA2"), false);
        expect_eq(reg.has("connB::ctx1"), false);
        expect_eq(reg.getCookieJar("connA2") == null, true);
    });
}
