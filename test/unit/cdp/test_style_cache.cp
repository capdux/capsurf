// test/cdp/test_style_cache.cp
// Tests for CapSurf StyleCache (least-recently-used stylesheet text cache for CDP screenshots)
// Migrated from vendor/kitesurf/packages/engine/test/style-cache.test.ts

import {
    MAX_STYLE_CACHE_BYTES,
    MAX_STYLE_CACHE_ENTRIES,
    MAX_STYLE_URL_LEN,
    StyleCacheEntry,
    StyleCache,
} from "../../../src/cdp/style_cache.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn main() {
    // 1. Initial empty cache state
    let cache = StyleCache.init(10);
    check("initial retainedBytes", cache.retainedBytes() == 0);
    check("initial count", cache.count() == 0);

    // 2. Insert two sheets of 4 bytes each
    cache.put("a", "1234", 100);
    cache.put("b", "5678", 101);
    check("count is 2", cache.count() == 2);
    check("retainedBytes is 8", cache.retainedBytes() == 8);

    // 3. Access 'a' to promote it in LRU sequence
    if (hit := cache.get("a")) {
        check("get a css", std.mem.eql(u8, hit.css, "1234"));
        check("get a fetched_at", hit.fetched_at == 100);
    } else {
        check("get a hit", false);
    }

    // 4. Insert 'c' (4 bytes) -> retained would be 8 + 4 = 12 > 10
    // Must evict least recently used: 'b' was accessed before 'a' was refreshed, so 'b' is evicted
    cache.put("c", "9012", 102);
    check("b is evicted", cache.get("b") == null);
    check("a is retained", cache.get("a") != null);
    check("c is retained", cache.get("c") != null);
    check("retainedBytes is 8", cache.retainedBytes() == 8);
    check("count is 2", cache.count() == 2);

    // 5. Replace an entry without double-counting
    let cache2 = StyleCache.init(10);
    cache2.put("a", "1234", 200);
    check("cache2 put a len 4", cache2.retainedBytes() == 4);
    cache2.put("a", "123456", 201);
    check("cache2 replace a len 6", cache2.retainedBytes() == 6);
    check("cache2 count is 1", cache2.count() == 1);

    // 6. Skip a sheet over the cap
    cache2.put("big", "0123456789X", 202); // 11 bytes > 10 max_bytes
    check("big is null", cache2.get("big") == null);
    check("a remains after big", cache2.retainedBytes() == 6);
    if (a_entry := cache2.get("a")) {
        check("a length is 6", a_entry.css.len == 6);
    } else {
        check("a exists", false);
    }

    // 7. Clear cache
    cache2.clear();
    check("cache2 clear retained", cache2.retainedBytes() == 0);
    check("cache2 clear count", cache2.count() == 0);
    check("a is gone after clear", cache2.get("a") == null);

    // 8. Default init uses MAX_STYLE_CACHE_BYTES
    let cache_default = StyleCache.initDefault();
    check("default max bytes", cache_default.max_bytes == MAX_STYLE_CACHE_BYTES);

    std.debug.print("All style_cache tests passed!\n", .{});
}
