// test/dom/test_storage.cp
// Tests WHATWG Web Storage implementation in src/dom/storage.cp

import { Storage, StorageEntry, StorageManager, MAX_STORAGE_ENTRIES, MAX_STORAGE_BYTES } from "../../../src/dom/storage.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. Basic Storage setItem, getItem, length
    {
        let storage = Storage.init("https://example.com");
        assert(storage.length() == 0, "initial length is 0");

        const ok1 = storage.setItem("theme", "dark");
        assert(ok1, "setItem theme succeeded");
        assert(storage.length() == 1, "length is 1");

        let val1 = storage.getItem("theme");
        assert(val1 != null, "getItem theme not null");
        assert(std.mem.eql(u8, val1.?, "dark"), "value is dark");

        // Update existing item
        const ok2 = storage.setItem("theme", "light");
        assert(ok2, "update theme succeeded");
        assert(storage.length() == 1, "length still 1 after update");

        let val2 = storage.getItem("theme");
        assert(val2 != null, "getItem theme not null");
        assert(std.mem.eql(u8, val2.?, "light"), "value updated to light");
    }

    // 2. Storage removeItem, clear, key index
    {
        let storage = Storage.init("https://example.com");
        * = storage.setItem("k1", "v1");
        * = storage.setItem("k2", "v2");
        * = storage.setItem("k3", "v3");
        assert(storage.length() == 3, "length is 3");

        // Test key index access
        assert(storage.key(0) != null, "key 0 not null");
        assert(std.mem.eql(u8, storage.key(0).?, "k1"), "key 0 is k1");
        assert(std.mem.eql(u8, storage.key(1).?, "k2"), "key 1 is k2");
        assert(std.mem.eql(u8, storage.key(2).?, "k3"), "key 2 is k3");
        assert(storage.key(3) == null, "key 3 is null out of bounds");

        // Remove middle element k2
        const removed = storage.removeItem("k2");
        assert(removed, "k2 removed");
        assert(storage.length() == 2, "length is 2 after removal");
        assert(storage.getItem("k2") == null, "k2 getItem returns null");
        assert(std.mem.eql(u8, storage.key(1).?, "k3"), "k3 shifted to index 1");

        // Remove non-existent key
        assert(!storage.removeItem("unknown"), "removing unknown returns false");

        // Clear all
        storage.clear();
        assert(storage.length() == 0, "length is 0 after clear");
        assert(storage.getItem("k1") == null, "k1 is null");
        assert(storage.key(0) == null, "key 0 is null");
    }

    // 3. StorageManager isolation
    {
        let mgr = StorageManager.init("https://example.com");
        * = mgr.localStorage.setItem("user", "alice");
        * = mgr.sessionStorage.setItem("token", "xyz");

        assert(mgr.localStorage.length() == 1, "localStorage length is 1");
        assert(mgr.sessionStorage.length() == 1, "sessionStorage length is 1");

        assert(mgr.localStorage.getItem("token") == null, "localStorage does not leak into sessionStorage");
        assert(mgr.sessionStorage.getItem("user") == null, "sessionStorage does not leak into localStorage");

        mgr.clearAll();
        assert(mgr.localStorage.length() == 0, "localStorage cleared");
        assert(mgr.sessionStorage.length() == 0, "sessionStorage cleared");
    }

    // 4. Quota limits (MAX_STORAGE_BYTES & MAX_STORAGE_ENTRIES) and boundary guards
    {
        let storage = Storage.init("https://example.com");
        assert(storage.usedBytes() == 0, "initial usedBytes is 0");
        assert(storage.remainingBytes() == MAX_STORAGE_BYTES, "initial remainingBytes is MAX_STORAGE_BYTES");

        assert(storage.setItem("a", "12345"), "setItem a succeeded");
        assert(storage.usedBytes() == 6, "usedBytes is 6");

        // Update existing key adjusts usedBytes
        assert(storage.setItem("a", "12"), "update a succeeded");
        assert(storage.usedBytes() == 3, "usedBytes adjusted to 3");

        // Reject oversized value without corrupting existing entry
        let huge_buf: [MAX_STORAGE_BYTES + 1]u8 = [0; MAX_STORAGE_BYTES + 1];
        assert(!storage.setItem("a", huge_buf[0..]), "oversized update rejected");
        assert(std.mem.eql(u8, storage.getItem("a").?, "12"), "existing value preserved on quota failure");
        assert(storage.usedBytes() == 3, "usedBytes unchanged on quota failure");
        assert(!storage.setItem("b", huge_buf[0..]), "oversized insert rejected");

        // Remove & clear reclaim quota
        assert(storage.removeItem("a"), "removeItem a succeeded");
        assert(storage.usedBytes() == 0, "usedBytes is 0 after removeItem");
        assert(storage.setItem("x", "y"), "setItem x succeeded");
        storage.clear();
        assert(storage.usedBytes() == 0, "usedBytes is 0 after clear");
    }

    std.debug.print("test_storage passed all assertions!\n", .{});
}
