// Migrated from vendor/kitesurf/packages/engine/src/cdp/style-cache.ts
// Connection-scoped cache of fetched stylesheet text for the screenshot fallback.
// Fixed-capacity zero-allocation LRU cache in pure CP.

import { MAX_STYLESHEET_BYTES } from "../net/read_capped.cp";

/// Retained bytes per connection; at least one sheet at the read cap fits.
pub const MAX_STYLE_CACHE_BYTES: usize = MAX_STYLESHEET_BYTES;
pub const MAX_STYLE_CACHE_ENTRIES: usize = 256;
pub const MAX_STYLE_URL_LEN: usize = 1024;

/// Cached stylesheet entry containing the CSS text and fetch timestamp.
pub struct StyleCacheEntry {
    css: []const u8 = "",
    fetched_at: i64 = 0,
}

/// Internal slot tracking cached stylesheet, URL key, and LRU access sequence.
pub struct StyleCacheSlot {
    url_buf: [MAX_STYLE_URL_LEN]u8 = [0; MAX_STYLE_URL_LEN],
    url_len: usize = 0,
    url_ext: []const u8 = "",
    entry: StyleCacheEntry = StyleCacheEntry{},
    active: bool = false,
    access_seq: u64 = 0,

    fn urlStr(*_): []const u8 {
        if (_.url_len > 0) return _.url_buf[0.._.url_len];
        return _.url_ext;
    }

    fn matchesUrl(*_, target_url: []const u8): bool {
        if (!_.active) return false;
        return std.mem.eql(u8, _.urlStr(), target_url);
    }

    fn reset(*_) {
        _.url_len = 0;
        _.url_ext = "";
        _.entry.css = "";
        _.entry.fetched_at = 0;
        _.active = false;
        _.access_seq = 0;
    }
}

/// Least-recently-used eviction once the retained stylesheet text passes `max_bytes`.
pub struct StyleCache {
    slots: [MAX_STYLE_CACHE_ENTRIES]StyleCacheSlot = [StyleCacheSlot{}; MAX_STYLE_CACHE_ENTRIES],
    max_bytes: usize = MAX_STYLE_CACHE_BYTES,
    retained_bytes: usize = 0,
    access_counter: u64 = 0,
    active_count: usize = 0,

    static fn init(max_bytes: usize): StyleCache {
        return StyleCache{
            max_bytes: max_bytes,
        };
    }

    static fn initDefault(): StyleCache {
        return StyleCache{
            max_bytes: MAX_STYLE_CACHE_BYTES,
        };
    }

    /// Returns retained stylesheet text bytes.
    fn retainedBytes(*_): usize {
        return _.retained_bytes;
    }

    /// Returns the number of cached stylesheets.
    fn count(*_): usize {
        return _.active_count;
    }

    /// Looks up a stylesheet by URL. On hit, updates LRU access order.
    fn get(*_, url: []const u8): (*StyleCacheEntry)? {
        let i: usize = 0;
        while (i < MAX_STYLE_CACHE_ENTRIES) : (i += 1) {
            if (_.slots[i].matchesUrl(url)) {
                _.access_counter += 1;
                _.slots[i].access_seq = _.access_counter;
                return &_.slots[i].entry;
            }
        }
        return null;
    }

    /// Puts a stylesheet into the cache. If an existing entry exists for `url`,
    /// it is replaced without double counting. If `css.len > max_bytes`, it is skipped.
    /// Evicts least-recently-used sheets until `retained_bytes + css.len <= max_bytes`
    /// and a slot is available.
    fn put(*_, url: []const u8, css: []const u8, fetched_at: i64) {
        * = _.remove(url);
        if (css.len > _.max_bytes) return;

        // Evict least recently used entries if over capacity or slots full
        while ((_.retained_bytes + css.len > _.max_bytes || _.active_count >= MAX_STYLE_CACHE_ENTRIES) && _.active_count > 0) {
            if (victim := _.findOldestSlot()) {
                _.evictSlot(victim);
            } else {
                break;
            }
        }

        if (_.retained_bytes + css.len > _.max_bytes || _.active_count >= MAX_STYLE_CACHE_ENTRIES) {
            return;
        }

        // Find a free slot
        let slot_idx: usize = MAX_STYLE_CACHE_ENTRIES;
        let i: usize = 0;
        while (i < MAX_STYLE_CACHE_ENTRIES) : (i += 1) {
            if (!_.slots[i].active) {
                slot_idx = i;
                break;
            }
        }

        if (slot_idx >= MAX_STYLE_CACHE_ENTRIES) return;

        let slot = &_.slots[slot_idx];
        if (url.len <= MAX_STYLE_URL_LEN) {
            @memcpy(slot.url_buf[0..url.len], url);
            slot.url_len = url.len;
            slot.url_ext = "";
        } else {
            slot.url_len = 0;
            slot.url_ext = url;
        }

        slot.entry.css = css;
        slot.entry.fetched_at = fetched_at;
        slot.active = true;

        _.access_counter += 1;
        slot.access_seq = _.access_counter;

        _.retained_bytes += css.len;
        _.active_count += 1;
    }

    /// Removes an entry by URL if present, returning whether an entry was removed.
    fn remove(*_, url: []const u8): bool {
        let i: usize = 0;
        while (i < MAX_STYLE_CACHE_ENTRIES) : (i += 1) {
            if (_.slots[i].matchesUrl(url)) {
                _.evictSlot(i);
                return true;
            }
        }
        return false;
    }

    /// Clears all entries from the cache.
    fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_STYLE_CACHE_ENTRIES) : (i += 1) {
            _.slots[i].reset();
        }
        _.retained_bytes = 0;
        _.active_count = 0;
    }

    /// Helper to evict a specific slot index.
    fn evictSlot(*_, idx: usize) {
        if (idx >= MAX_STYLE_CACHE_ENTRIES) return;
        if (!_.slots[idx].active) return;

        const css_len = _.slots[idx].entry.css.len;
        if (_.retained_bytes >= css_len) {
            _.retained_bytes -= css_len;
        } else {
            _.retained_bytes = 0;
        }

        _.slots[idx].reset();
        if (_.active_count > 0) {
            _.active_count -= 1;
        }
    }

    /// Finds the index of the least recently used active slot.
    fn findOldestSlot(*_): usize? {
        let oldest_idx: usize = MAX_STYLE_CACHE_ENTRIES;
        let min_seq: u64 = std.math.maxInt(u64);

        let i: usize = 0;
        while (i < MAX_STYLE_CACHE_ENTRIES) : (i += 1) {
            if (_.slots[i].active) {
                if (_.slots[i].access_seq < min_seq) {
                    min_seq = _.slots[i].access_seq;
                    oldest_idx = i;
                }
            }
        }

        if (oldest_idx < MAX_STYLE_CACHE_ENTRIES) {
            return oldest_idx;
        }
        return null;
    }
}
