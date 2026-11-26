// Migrated from vendor/kitesurf/packages/page-script/src/globals.ts
// Implements WHATWG Web Storage API (localStorage, sessionStorage) for CapSurf DOM.

pub const MAX_STORAGE_ENTRIES: usize = 128;
pub const MAX_STORAGE_BYTES: usize = 5 * 1024 * 1024;

pub struct StorageEntry {
    key: []const u8 = "",
    value: []const u8 = "",
}

pub struct Storage {
    entries: [MAX_STORAGE_ENTRIES]StorageEntry = [StorageEntry{}; 128],
    entry_count: usize = 0,
    used_bytes: usize = 0,
    origin: []const u8 = "",

    static fn init(origin: []const u8): Storage {
        return Storage{ origin: origin };
    }

    fn safeCount(): usize {
        return if (_.entry_count <= MAX_STORAGE_ENTRIES) _.entry_count else MAX_STORAGE_ENTRIES;
    }

    fn usedBytes(): usize {
        return _.used_bytes;
    }

    fn remainingBytes(): usize {
        if (_.used_bytes >= MAX_STORAGE_BYTES) return 0;
        return MAX_STORAGE_BYTES - _.used_bytes;
    }

    fn getItem(*_, key_name: []const u8): ([]const u8)? {
        const count = _.safeCount();
        let i: usize = 0;
        while (i < count) : (i += 1) {
            if (std.mem.eql(u8, _.entries[i].key, key_name)) {
                return _.entries[i].value;
            }
        }
        return null;
    }

    fn setItem(*_, key_name: []const u8, value: []const u8): bool {
        if (key_name.len > MAX_STORAGE_BYTES || value.len > MAX_STORAGE_BYTES - key_name.len) {
            return false;
        }
        const new_size = key_name.len + value.len;
        const count = _.safeCount();
        let i: usize = 0;
        while (i < count) : (i += 1) {
            if (std.mem.eql(u8, _.entries[i].key, key_name)) {
                const old_size = _.entries[i].key.len + _.entries[i].value.len;
                const base_used = if (_.used_bytes >= old_size) _.used_bytes - old_size else 0;
                if (new_size > MAX_STORAGE_BYTES || base_used > MAX_STORAGE_BYTES - new_size) {
                    return false;
                }
                _.entries[i].value = value;
                _.used_bytes = base_used + new_size;
                return true;
            }
        }
        if (count >= MAX_STORAGE_ENTRIES) {
            return false;
        }
        if (_.used_bytes > MAX_STORAGE_BYTES - new_size) {
            return false;
        }
        _.entries[count] = StorageEntry{
            key: key_name,
            value: value,
        };
        _.entry_count = count + 1;
        _.used_bytes += new_size;
        return true;
    }

    fn removeItem(*_, key_name: []const u8): bool {
        const count = _.safeCount();
        let i: usize = 0;
        while (i < count) : (i += 1) {
            if (std.mem.eql(u8, _.entries[i].key, key_name)) {
                const removed_size = _.entries[i].key.len + _.entries[i].value.len;
                let j: usize = i;
                while (j + 1 < count) : (j += 1) {
                    _.entries[j] = _.entries[j + 1];
                }
                _.entry_count = count - 1;
                _.entries[_.entry_count] = StorageEntry{};
                if (_.used_bytes >= removed_size) {
                    _.used_bytes -= removed_size;
                } else {
                    _.used_bytes = 0;
                }
                return true;
            }
        }
        return false;
    }

    fn clear(*_) {
        const count = _.safeCount();
        let i: usize = 0;
        while (i < count) : (i += 1) {
            _.entries[i] = StorageEntry{};
        }
        _.entry_count = 0;
        _.used_bytes = 0;
    }

    fn key(*_, index: usize): ([]const u8)? {
        const count = _.safeCount();
        if (index >= count) return null;
        return _.entries[index].key;
    }

    fn length(): usize {
        return _.safeCount();
    }
}

pub struct StorageManager {
    localStorage: Storage = Storage{},
    sessionStorage: Storage = Storage{},

    static fn init(origin: []const u8): StorageManager {
        return StorageManager{
            localStorage: Storage.init(origin),
            sessionStorage: Storage.init(origin),
        };
    }

    fn clearAll(*_) {
        _.localStorage.clear();
        _.sessionStorage.clear();
    }
}
