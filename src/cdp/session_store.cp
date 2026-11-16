// Migrated from vendor/kitesurf/packages/engine/src/state/session-store.ts
// Where a browser session keeps its data: network logs, response bodies,
// cookie jars, new-document scripts, and session recovery checkpoints.

pub const TABLE_NETWORK_LOGS: []const u8 = "network_logs";
pub const TABLE_RESPONSE_BODIES: []const u8 = "response_bodies";
pub const TABLE_SCRIPT_SOURCES: []const u8 = "script_sources";
pub const TABLE_COOKIE_JARS: []const u8 = "cookie_jars";
pub const TABLE_CHECKPOINTS: []const u8 = "checkpoints";

/// Statistics for session storage memory consumption.
pub struct StoreStats {
    backend: []const u8 = "memory",
    bytes: usize = 0,
    value_bytes: usize = 0,
    key_count: usize = 0,
    table_count: usize = 0,
}

pub const MAX_KEY_SIZE: usize = 128;
pub const INLINE_VAL_SIZE: usize = 1024;
pub const MAX_TABLE_ENTRIES: usize = 64;

/// Single key-value entry in a StoreTable.
pub struct StoreEntry {
    key_buf: [MAX_KEY_SIZE]u8 = [0; MAX_KEY_SIZE],
    key_len: usize = 0,
    val_buf: [INLINE_VAL_SIZE]u8 = [0; INLINE_VAL_SIZE],
    val_len: usize = 0,
    ext_val: []const u8 = "",
    use_ext: bool = false,
    active: bool = false,

    pub fn key(*_): []const u8 {
        return _.key_buf[0.._.key_len];
    }

    pub fn value(*_): []const u8 {
        if (_.use_ext) return _.ext_val;
        return _.val_buf[0.._.val_len];
    }
}

/// Fixed-capacity in-memory key-value table.
pub struct StoreTable {
    entries: [MAX_TABLE_ENTRIES]StoreEntry = [StoreEntry{}; MAX_TABLE_ENTRIES],
    entry_count: usize = 0,

    pub static fn init(): StoreTable {
        return StoreTable{};
    }

    /// Retrieves the value associated with key, or null if missing.
    pub fn get(*_, key: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < MAX_TABLE_ENTRIES) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].key(), key)) {
                return _.entries[i].value();
            }
        }
        return null;
    }

    /// Checks if the table contains the given key.
    pub fn has(*_, key: []const u8): bool {
        let i: usize = 0;
        while (i < MAX_TABLE_ENTRIES) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].key(), key)) {
                return true;
            }
        }
        return false;
    }

    /// Sets key to value. Updates existing entry or allocates a new slot.
    pub fn set(*_, key: []const u8, value: []const u8): bool {
        if (key.len == 0 || key.len > MAX_KEY_SIZE) return false;

        // Check for existing key to overwrite
        let i: usize = 0;
        while (i < MAX_TABLE_ENTRIES) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].key(), key)) {
                if (value.len <= INLINE_VAL_SIZE) {
                    @memcpy(_.entries[i].val_buf[0..value.len], value);
                    _.entries[i].val_len = value.len;
                    _.entries[i].use_ext = false;
                    _.entries[i].ext_val = "";
                } else {
                    _.entries[i].ext_val = value;
                    _.entries[i].val_len = value.len;
                    _.entries[i].use_ext = true;
                }
                return true;
            }
        }

        // Insert new entry
        if (_.entry_count >= MAX_TABLE_ENTRIES) return false;

        let slot: usize = 0;
        while (slot < MAX_TABLE_ENTRIES) : (slot += 1) {
            if (!_.entries[slot].active) break;
        }
        if (slot >= MAX_TABLE_ENTRIES) return false;

        @memcpy(_.entries[slot].key_buf[0..key.len], key);
        _.entries[slot].key_len = key.len;

        if (value.len <= INLINE_VAL_SIZE) {
            @memcpy(_.entries[slot].val_buf[0..value.len], value);
            _.entries[slot].val_len = value.len;
            _.entries[slot].use_ext = false;
            _.entries[slot].ext_val = "";
        } else {
            _.entries[slot].ext_val = value;
            _.entries[slot].val_len = value.len;
            _.entries[slot].use_ext = true;
        }

        _.entries[slot].active = true;
        _.entry_count += 1;
        return true;
    }

    /// Deletes the entry with the specified key.
    pub fn delete(*_, key: []const u8): bool {
        let i: usize = 0;
        while (i < MAX_TABLE_ENTRIES) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].key(), key)) {
                _.entries[i].active = false;
                _.entries[i].key_len = 0;
                _.entries[i].val_len = 0;
                _.entries[i].use_ext = false;
                _.entries[i].ext_val = "";
                _.entry_count -= 1;
                return true;
            }
        }
        return false;
    }

    /// Clears all entries in the table.
    pub fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_TABLE_ENTRIES) : (i += 1) {
            _.entries[i].active = false;
            _.entries[i].key_len = 0;
            _.entries[i].val_len = 0;
            _.entries[i].use_ext = false;
            _.entries[i].ext_val = "";
        }
        _.entry_count = 0;
    }

    /// Returns the number of active entries in the table.
    pub fn count(*_): usize {
        return _.entry_count;
    }

    pub fn isEmpty(*_): bool {
        return _.entry_count == 0;
    }

    pub fn isFull(*_): bool {
        return _.entry_count >= MAX_TABLE_ENTRIES;
    }

    /// Computes memory statistics for this table.
    pub fn stats(*_): StoreStats {
        let val_bytes: usize = 0;
        let k_bytes: usize = 0;
        let i: usize = 0;
        while (i < MAX_TABLE_ENTRIES) : (i += 1) {
            if (_.entries[i].active) {
                k_bytes += _.entries[i].key_len;
                val_bytes += _.entries[i].val_len;
            }
        }
        return StoreStats {
            backend: "memory",
            bytes: k_bytes + val_bytes,
            value_bytes: val_bytes,
            key_count: _.entry_count,
            table_count: 1,
        };
    }

    /// Retrieves active key at given index for iteration.
    pub fn keyAt(*_, index: usize): ([]const u8)? {
        if (index >= MAX_TABLE_ENTRIES) return null;
        if (!_.entries[index].active) return null;
        return _.entries[index].key();
    }

    /// Retrieves active value at given index for iteration.
    pub fn valueAt(*_, index: usize): ([]const u8)? {
        if (index >= MAX_TABLE_ENTRIES) return null;
        if (!_.entries[index].active) return null;
        return _.entries[index].value();
    }
}

pub const MAX_TABLE_NAME_SIZE: usize = 64;
pub const MAX_STORE_TABLES: usize = 16;

/// Named table wrapper inside SessionStore.
pub struct NamedTableEntry {
    name_buf: [MAX_TABLE_NAME_SIZE]u8 = [0; MAX_TABLE_NAME_SIZE],
    name_len: usize = 0,
    table: StoreTable = StoreTable.init(),
    active: bool = false,

    pub fn name(*_): []const u8 {
        return _.name_buf[0.._.name_len];
    }
}

/// Multi-table session store managing network logs, response bodies, script sources, etc.
pub struct SessionStore {
    tables: [MAX_STORE_TABLES]NamedTableEntry = [NamedTableEntry{}; MAX_STORE_TABLES],
    table_count: usize = 0,
    backend: []const u8 = "memory",

    pub static fn init(): SessionStore {
        return SessionStore{};
    }

    /// Returns a pointer to the table called `name`, creating it if it does not exist.
    pub fn table(*_, name: []const u8): (*StoreTable)? {
        if (name.len == 0 || name.len > MAX_TABLE_NAME_SIZE) return null;

        // Check if table already exists
        let i: usize = 0;
        while (i < MAX_STORE_TABLES) : (i += 1) {
            if (_.tables[i].active && std.mem.eql(u8, _.tables[i].name(), name)) {
                return &_.tables[i].table;
            }
        }

        // Allocate slot for new table
        if (_.table_count >= MAX_STORE_TABLES) return null;

        let slot: usize = 0;
        while (slot < MAX_STORE_TABLES) : (slot += 1) {
            if (!_.tables[slot].active) break;
        }
        if (slot >= MAX_STORE_TABLES) return null;

        @memcpy(_.tables[slot].name_buf[0..name.len], name);
        _.tables[slot].name_len = name.len;
        _.tables[slot].table.clear();
        _.tables[slot].active = true;
        _.table_count += 1;

        return &_.tables[slot].table;
    }

    /// Looks up an existing table without creating one.
    pub fn getTable(*_, name: []const u8): (*StoreTable)? {
        let i: usize = 0;
        while (i < MAX_STORE_TABLES) : (i += 1) {
            if (_.tables[i].active && std.mem.eql(u8, _.tables[i].name(), name)) {
                return &_.tables[i].table;
            }
        }
        return null;
    }

    /// Checks if a named table exists.
    pub fn hasTable(*_, name: []const u8): bool {
        let i: usize = 0;
        while (i < MAX_STORE_TABLES) : (i += 1) {
            if (_.tables[i].active && std.mem.eql(u8, _.tables[i].name(), name)) {
                return true;
            }
        }
        return false;
    }

    /// Drops a named table and clears all its contents.
    pub fn dropTable(*_, name: []const u8): bool {
        let i: usize = 0;
        while (i < MAX_STORE_TABLES) : (i += 1) {
            if (_.tables[i].active && std.mem.eql(u8, _.tables[i].name(), name)) {
                _.tables[i].table.clear();
                _.tables[i].active = false;
                _.tables[i].name_len = 0;
                _.table_count -= 1;
                return true;
            }
        }
        return false;
    }

    /// Clears all tables in the session store.
    pub fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_STORE_TABLES) : (i += 1) {
            if (_.tables[i].active) {
                _.tables[i].table.clear();
                _.tables[i].active = false;
                _.tables[i].name_len = 0;
            }
        }
        _.table_count = 0;
    }

    /// Returns the count of active tables.
    pub fn tableCount(*_): usize {
        return _.table_count;
    }

    /// Global memory statistics aggregating all active tables.
    pub fn stats(*_): StoreStats {
        let total_bytes: usize = 0;
        let total_val_bytes: usize = 0;
        let total_keys: usize = 0;

        let i: usize = 0;
        while (i < MAX_STORE_TABLES) : (i += 1) {
            if (_.tables[i].active) {
                const s = _.tables[i].table.stats();
                total_bytes += s.bytes;
                total_val_bytes += s.value_bytes;
                total_keys += s.key_count;
            }
        }

        return StoreStats {
            backend: _.backend,
            bytes: total_bytes,
            value_bytes: total_val_bytes,
            key_count: total_keys,
            table_count: _.table_count,
        };
    }
}
