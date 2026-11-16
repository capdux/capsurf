// Migrated from vendor/kitesurf/packages/engine/src/eval/resource-store.ts
// Fixed-capacity zero-allocation memory cache for images and fonts in CDP isolate.

pub const ENGINE_KEY_MARKER: u64 = @as(u64, 1) << 63;
pub const MAX_ENTRY_BYTES: usize = 8 * 1024 * 1024;
pub const MAX_TOTAL_BYTES: usize = 16 * 1024 * 1024;
pub const MAX_STORE_ENTRIES: usize = 64;
pub const INLINE_DATA_SIZE: usize = 1024;

/// Kind of resource being cached: image or author font.
pub enum ResourceKind {
    Image,
    Font,
}

/// Single cached resource entry. Small payloads (<= 1024 bytes) are stored
/// inline in `data_buf` for zero heap allocation. Larger payloads retain
/// a reference slice in `ext_data`.
pub struct ResourceEntry {
    key: u64 = 0,
    kind: ResourceKind = ResourceKind.Image,
    connection_id: []const u8 = "",
    target_id: []const u8 = "",
    size: usize = 0,
    data_buf: [1024]u8 = [0; 1024],
    ext_data: []const u8 = "",
    active: bool = false,
    last_access: u64 = 0,

    /// Returns the resource byte slice (either inline or external).
    fn data(*_): []const u8 {
        if (_.size <= INLINE_DATA_SIZE) {
            return _.data_buf[0.._.size];
        }
        return _.ext_data;
    }

    /// Whether this entry is stored within the inline buffer.
    fn isInline(*_): bool {
        return _.size <= INLINE_DATA_SIZE;
    }

    /// Clears and resets the entry slot.
    fn reset(*_) {
        _.key = 0;
        _.kind = ResourceKind.Image;
        _.connection_id = "";
        _.target_id = "";
        _.size = 0;
        _.ext_data = "";
        _.active = false;
        _.last_access = 0;
    }
}

/// Resource store statistics.
pub struct ResourceStoreStats {
    bytes: usize = 0,
    count: usize = 0,
}

/// Fixed-capacity, zero-allocation resource cache for CDP rendering.
/// Manages image and font byte buffers scoped to connections and targets.
/// When approaching capacity or memory ceilings, LRU eviction reclaims space
/// preferentially from the admitting connection before touching other connections.
pub struct ResourceStore {
    entries: [MAX_STORE_ENTRIES]ResourceEntry = [ResourceEntry{}; MAX_STORE_ENTRIES],
    total_bytes: usize = 0,
    entry_count: usize = 0,
    seed: u64 = 0x517cc1b727220a95,
    key_counter: u64 = 1,
    tick: u64 = 0,

    static fn init(): ResourceStore {
        return ResourceStore{};
    }

    /// Generates a unique 64-bit key with the ENGINE_KEY_MARKER bit set.
    fn mintKey(*_): u64 {
        while (true) {
            let x = _.seed +% _.key_counter;
            _.key_counter += 1;
            x ^= x >> 12;
            x ^= x << 25;
            x ^= x >> 27;
            _.seed = x;
            const raw = x *% 0x2545F4914F6CDD1D;
            const candidate = (raw & (ENGINE_KEY_MARKER - 1)) | ENGINE_KEY_MARKER;
            if (!_.has(candidate)) {
                return candidate;
            }
        }
    }

    /// Puts an image into the store, evicting over-budget entries if necessary.
    fn putImage(*_, connection_id: []const u8, target_id: []const u8, bytes: []const u8): u64? {
        return _.put(ResourceKind.Image, connection_id, target_id, bytes);
    }

    /// Puts a font into the store, evicting over-budget entries if necessary.
    fn putFont(*_, connection_id: []const u8, target_id: []const u8, bytes: []const u8): u64? {
        return _.put(ResourceKind.Font, connection_id, target_id, bytes);
    }

    /// Internal insertion implementation for any resource kind.
    fn put(*_, kind: ResourceKind, connection_id: []const u8, target_id: []const u8, bytes: []const u8): u64? {
        if (bytes.len == 0 || bytes.len > MAX_ENTRY_BYTES) return null;
        if (bytes.len > MAX_TOTAL_BYTES) return null;

        // Evict until we have enough space and an available slot.
        while ((_.total_bytes + bytes.len > MAX_TOTAL_BYTES || _.entry_count >= MAX_STORE_ENTRIES) && _.entry_count > 0) {
            if (victim := _.findEvictionVictim(connection_id, kind)) {
                _.dropIndex(victim);
            } else {
                break;
            }
        }

        if (_.total_bytes + bytes.len > MAX_TOTAL_BYTES || _.entry_count >= MAX_STORE_ENTRIES) {
            return null;
        }

        // Find free slot
        let slot: usize = MAX_STORE_ENTRIES;
        let s: usize = 0;
        while (s < MAX_STORE_ENTRIES) : (s += 1) {
            if (!_.entries[s].active) {
                slot = s;
                break;
            }
        }
        if (slot >= MAX_STORE_ENTRIES) return null;

        const key = _.mintKey();
        _.tick += 1;

        _.entries[slot].key = key;
        _.entries[slot].kind = kind;
        _.entries[slot].connection_id = connection_id;
        _.entries[slot].target_id = target_id;
        _.entries[slot].size = bytes.len;
        _.entries[slot].last_access = _.tick;
        _.entries[slot].active = true;

        if (bytes.len <= INLINE_DATA_SIZE) {
            @memcpy(_.entries[slot].data_buf[0..bytes.len], bytes);
            _.entries[slot].ext_data = "";
        } else {
            _.entries[slot].ext_data = bytes;
        }

        _.total_bytes += bytes.len;
        _.entry_count += 1;
        return key;
    }

    /// Retrieves an image by key, updating its LRU access recency.
    fn getImage(*_, key: u64): ([]const u8)? {
        return _.get(key, ResourceKind.Image);
    }

    /// Retrieves a font by key, updating its LRU access recency.
    fn getFont(*_, key: u64): ([]const u8)? {
        return _.get(key, ResourceKind.Font);
    }

    /// Internal getter by key and kind.
    fn get(*_, key: u64, kind: ResourceKind): ([]const u8)? {
        let i: usize = 0;
        while (i < MAX_STORE_ENTRIES) : (i += 1) {
            if (_.entries[i].active && _.entries[i].key == key && _.entries[i].kind == kind) {
                _.tick += 1;
                _.entries[i].last_access = _.tick;
                return _.entries[i].data();
            }
        }
        return null;
    }

    /// Retrieves an image by key with strict connection ownership verification.
    fn getImageForConnection(*_, key: u64, connection_id: []const u8): ([]const u8)? {
        return _.getForConnection(key, ResourceKind.Image, connection_id);
    }

    /// Retrieves a font by key with strict connection ownership verification.
    fn getFontForConnection(*_, key: u64, connection_id: []const u8): ([]const u8)? {
        return _.getForConnection(key, ResourceKind.Font, connection_id);
    }

    /// Internal getter with connection ownership check.
    fn getForConnection(*_, key: u64, kind: ResourceKind, connection_id: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < MAX_STORE_ENTRIES) : (i += 1) {
            if (_.entries[i].active && _.entries[i].key == key && _.entries[i].kind == kind) {
                if (!std.mem.eql(u8, _.entries[i].connection_id, connection_id)) {
                    return null;
                }
                _.tick += 1;
                _.entries[i].last_access = _.tick;
                return _.entries[i].data();
            }
        }
        return null;
    }

    /// Checks if a key exists in the store.
    fn has(*_, key: u64): bool {
        let i: usize = 0;
        while (i < MAX_STORE_ENTRIES) : (i += 1) {
            if (_.entries[i].active && _.entries[i].key == key) {
                return true;
            }
        }
        return false;
    }

    /// Drops a single entry by key.
    fn dropKey(*_, key: u64): bool {
        let i: usize = 0;
        while (i < MAX_STORE_ENTRIES) : (i += 1) {
            if (_.entries[i].active && _.entries[i].key == key) {
                _.dropIndex(i);
                return true;
            }
        }
        return false;
    }

    /// Cascades and drops all resources associated with a connection upon disconnection,
    /// updating total memory accounting. Returns the number of dropped entries.
    fn dropConnection(*_, connection_id: []const u8): usize {
        let dropped: usize = 0;
        let i: usize = 0;
        while (i < MAX_STORE_ENTRIES) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].connection_id, connection_id)) {
                _.dropIndex(i);
                dropped += 1;
            }
        }
        return dropped;
    }

    /// Drops all resources associated with a specific target within a connection.
    /// Returns the number of dropped entries.
    fn dropTarget(*_, connection_id: []const u8, target_id: []const u8): usize {
        let dropped: usize = 0;
        let i: usize = 0;
        while (i < MAX_STORE_ENTRIES) : (i += 1) {
            if (_.entries[i].active &&
                std.mem.eql(u8, _.entries[i].connection_id, connection_id) &&
                std.mem.eql(u8, _.entries[i].target_id, target_id))
            {
                _.dropIndex(i);
                dropped += 1;
            }
        }
        return dropped;
    }

    /// Returns total bytes consumed by all stored resources.
    fn totalBytes(*_): usize {
        return _.total_bytes;
    }

    /// Returns the count of active entries.
    fn entryCount(*_): usize {
        return _.entry_count;
    }

    /// Checks if the store is empty.
    fn isEmpty(*_): bool {
        return _.entry_count == 0;
    }

    /// Clears all stored resources and resets memory accounting.
    fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_STORE_ENTRIES) : (i += 1) {
            _.entries[i].reset();
        }
        _.total_bytes = 0;
        _.entry_count = 0;
    }

    /// Memory and entry statistics.
    fn stats(*_): ResourceStoreStats {
        return ResourceStoreStats{
            bytes: _.total_bytes,
            count: _.entry_count,
        };
    }

    /// Internal: frees an entry slot and adjusts total bytes and entry count.
    fn dropIndex(*_, idx: usize) {
        if (idx >= MAX_STORE_ENTRIES || !_.entries[idx].active) return;
        const sz = _.entries[idx].size;
        if (_.total_bytes >= sz) {
            _.total_bytes -= sz;
        } else {
            _.total_bytes = 0;
        }
        _.entries[idx].reset();
        if (_.entry_count > 0) {
            _.entry_count -= 1;
        }
    }

    /// Finds the best victim entry index to evict according to fairness & LRU rules:
    /// 1. The admitting connection's oldest entry of the same kind;
    /// 2. Any oldest entry of the admitting connection;
    /// 3. The oldest entry of whichever other connection holds the most bytes;
    /// 4. Any oldest active entry.
    fn findEvictionVictim(*_, admitting_connection: []const u8, kind: ResourceKind): usize? {
        // 1. Admitting connection's oldest entry of the same kind
        let own_kind_idx: usize = MAX_STORE_ENTRIES;
        let own_kind_tick: u64 = std.math.maxInt(u64);
        let i: usize = 0;
        while (i < MAX_STORE_ENTRIES) : (i += 1) {
            if (_.entries[i].active &&
                _.entries[i].kind == kind &&
                std.mem.eql(u8, _.entries[i].connection_id, admitting_connection))
            {
                if (_.entries[i].last_access < own_kind_tick) {
                    own_kind_tick = _.entries[i].last_access;
                    own_kind_idx = i;
                }
            }
        }
        if (own_kind_idx < MAX_STORE_ENTRIES) return own_kind_idx;

        // 2. Admitting connection's oldest entry of any kind
        let own_any_idx: usize = MAX_STORE_ENTRIES;
        let own_any_tick: u64 = std.math.maxInt(u64);
        let j: usize = 0;
        while (j < MAX_STORE_ENTRIES) : (j += 1) {
            if (_.entries[j].active &&
                std.mem.eql(u8, _.entries[j].connection_id, admitting_connection))
            {
                if (_.entries[j].last_access < own_any_tick) {
                    own_any_tick = _.entries[j].last_access;
                    own_any_idx = j;
                }
            }
        }
        if (own_any_idx < MAX_STORE_ENTRIES) return own_any_idx;

        // 3. Other connection holding the most bytes
        let largest_bytes: usize = 0;
        let largest_victim_idx: usize = MAX_STORE_ENTRIES;
        let k: usize = 0;
        while (k < MAX_STORE_ENTRIES) : (k += 1) {
            if (!_.entries[k].active) continue;
            if (std.mem.eql(u8, _.entries[k].connection_id, admitting_connection)) continue;

            const conn = _.entries[k].connection_id;
            let conn_bytes: usize = 0;
            let m: usize = 0;
            while (m < MAX_STORE_ENTRIES) : (m += 1) {
                if (_.entries[m].active && std.mem.eql(u8, _.entries[m].connection_id, conn)) {
                    conn_bytes += _.entries[m].size;
                }
            }

            if (conn_bytes > largest_bytes) {
                largest_bytes = conn_bytes;
                // Find oldest entry for this connection
                let oldest_idx: usize = k;
                let oldest_tick: u64 = _.entries[k].last_access;
                let n: usize = 0;
                while (n < MAX_STORE_ENTRIES) : (n += 1) {
                    if (_.entries[n].active && std.mem.eql(u8, _.entries[n].connection_id, conn)) {
                        if (_.entries[n].last_access < oldest_tick) {
                            oldest_tick = _.entries[n].last_access;
                            oldest_idx = n;
                        }
                    }
                }
                largest_victim_idx = oldest_idx;
            }
        }
        if (largest_victim_idx < MAX_STORE_ENTRIES) return largest_victim_idx;

        // 4. Any oldest active entry
        let any_oldest_idx: usize = MAX_STORE_ENTRIES;
        let any_oldest_tick: u64 = std.math.maxInt(u64);
        let p: usize = 0;
        while (p < MAX_STORE_ENTRIES) : (p += 1) {
            if (_.entries[p].active && _.entries[p].last_access < any_oldest_tick) {
                any_oldest_tick = _.entries[p].last_access;
                any_oldest_idx = p;
            }
        }
        if (any_oldest_idx < MAX_STORE_ENTRIES) return any_oldest_idx;

        return null;
    }
}

pub fn putOpaqueImage(store: *ResourceStore, connection_id: []const u8, target_id: []const u8, bytes: []const u8): u64? {
    return store.putImage(connection_id, target_id, bytes);
}

pub fn putAuthorFont(store: *ResourceStore, connection_id: []const u8, target_id: []const u8, bytes: []const u8): u64? {
    return store.putFont(connection_id, target_id, bytes);
}

pub fn getImageBytes(store: *ResourceStore, key: u64, connection_id: []const u8): ([]const u8)? {
    return store.getImageForConnection(key, connection_id);
}

pub fn getFontBytes(store: *ResourceStore, key: u64, connection_id: []const u8): ([]const u8)? {
    return store.getFontForConnection(key, connection_id);
}

pub fn dropTargetImages(store: *ResourceStore, connection_id: []const u8, target_id: []const u8): usize {
    return store.dropTarget(connection_id, target_id);
}

pub fn dropConnectionImages(store: *ResourceStore, connection_id: []const u8): usize {
    return store.dropConnection(connection_id);
}

pub fn imageStoreStats(store: *ResourceStore): ResourceStoreStats {
    return store.stats();
}
