// Migrated from vendor/kitesurf/packages/page-script/src/globals.ts
// Implements WHATWG Web Storage API (localStorage, sessionStorage) for CapSurf DOM.

pub const MAX_STORAGE_ENTRIES: usize = 128;

pub struct StorageEntry {
    key: []const u8 = "",
    value: []const u8 = "",
}

pub struct Storage {
    entries: [MAX_STORAGE_ENTRIES]StorageEntry = [StorageEntry{}; 128],
    entry_count: usize = 0,
    origin: []const u8 = "",

    static fn init(origin: []const u8): Storage {
        return Storage{ origin: origin };
    }

    fn getItem(*_, key_name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.entry_count) : (i += 1) {
            if (std.mem.eql(u8, _.entries[i].key, key_name)) {
                return _.entries[i].value;
            }
        }
        return null;
    }

    fn setItem(*_, key_name: []const u8, value: []const u8): bool {
        let i: usize = 0;
        while (i < _.entry_count) : (i += 1) {
            if (std.mem.eql(u8, _.entries[i].key, key_name)) {
                _.entries[i].value = value;
                return true;
            }
        }
        if (_.entry_count >= MAX_STORAGE_ENTRIES) {
            return false;
        }
        _.entries[_.entry_count] = StorageEntry{
            key: key_name,
            value: value,
        };
        _.entry_count += 1;
        return true;
    }

    fn removeItem(*_, key_name: []const u8): bool {
        let i: usize = 0;
        while (i < _.entry_count) : (i += 1) {
            if (std.mem.eql(u8, _.entries[i].key, key_name)) {
                let j: usize = i;
                while (j + 1 < _.entry_count) : (j += 1) {
                    _.entries[j] = _.entries[j + 1];
                }
                _.entry_count -= 1;
                _.entries[_.entry_count] = StorageEntry{};
                return true;
            }
        }
        return false;
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < _.entry_count) : (i += 1) {
            _.entries[i] = StorageEntry{};
        }
        _.entry_count = 0;
    }

    fn key(*_, index: usize): ([]const u8)? {
        if (index >= _.entry_count) return null;
        return _.entries[index].key;
    }

    fn length(): usize {
        return _.entry_count;
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
