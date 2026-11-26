// src/net/single_flight.cp
// Clean-room pure-CP single-flight request coalescing gate.
// Maps a request key to a leader TransferId and a list of coalesced waiter TransferIds.

pub type TransferId = u32;
pub const INVALID_TRANSFER_ID: TransferId = 0;

pub const MAX_SINGLE_FLIGHT_KEYS: usize = 64;
pub const MAX_SINGLE_FLIGHT_WAITERS: usize = 32;
pub const MAX_SINGLE_FLIGHT_KEY_LEN: usize = 512;

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    for (b of data) {
        h ^= @as(u64, b);
        h *%= 1099511628211;
    }
    return h;
}

pub struct SingleFlightEnterResult {
    is_leader: bool = false,
    coalesced: bool = false,
    gate_full: bool = false,
    leader_id: TransferId = INVALID_TRANSFER_ID,
}

pub struct SingleFlightEntry {
    active: bool = false,
    key_hash: u64 = 0,
    key_len: usize = 0,
    key_prefix_len: usize = 0,
    key_prefix: [MAX_SINGLE_FLIGHT_KEY_LEN]u8 = [0; MAX_SINGLE_FLIGHT_KEY_LEN],
    leader_id: TransferId = INVALID_TRANSFER_ID,
    waiters: [MAX_SINGLE_FLIGHT_WAITERS]TransferId = [INVALID_TRANSFER_ID; MAX_SINGLE_FLIGHT_WAITERS],
    waiter_count: usize = 0,

    fn matchesKey(key: []const u8, key_hash: u64): bool {
        if (!_.active) return false;
        if (_.key_len != key.len || _.key_hash != key_hash) return false;
        const cmp_len = if (key.len < MAX_SINGLE_FLIGHT_KEY_LEN) key.len else MAX_SINGLE_FLIGHT_KEY_LEN;
        if (_.key_prefix_len != cmp_len) return false;
        return std.mem.eql(u8, _.key_prefix[0..cmp_len], key[0..cmp_len]);
    }

    fn reset(*_) {
        _.active = false;
        _.key_hash = 0;
        _.key_len = 0;
        _.key_prefix_len = 0;
        _.leader_id = INVALID_TRANSFER_ID;
        _.waiter_count = 0;
    }
}

pub struct SingleFlightGate {
    entries: [MAX_SINGLE_FLIGHT_KEYS]SingleFlightEntry = [SingleFlightEntry{}; MAX_SINGLE_FLIGHT_KEYS],
    active_keys: usize = 0,

    static fn init(): SingleFlightGate {
        return SingleFlightGate{};
    }

    fn findEntryIndex(key: []const u8, key_hash: u64): usize? {
        let i: usize = 0;
        while (i < MAX_SINGLE_FLIGHT_KEYS) : (i += 1) {
            if (_.entries[i].matchesKey(key, key_hash)) {
                return i;
            }
        }
        return null;
    }

    fn findFreeIndex(): usize? {
        let i: usize = 0;
        while (i < MAX_SINGLE_FLIGHT_KEYS) : (i += 1) {
            if (!_.entries[i].active) {
                return i;
            }
        }
        return null;
    }

    /// Enters `key` with `transfer_id`.
    /// First caller for `key` becomes leader; subsequent callers coalesce as waiters.
    fn enter(*_, key: []const u8, transfer_id: TransferId): SingleFlightEnterResult {
        if (transfer_id == INVALID_TRANSFER_ID) {
            return SingleFlightEnterResult{ gate_full: true };
        }
        const h = fnv1a64(key);
        if (idx := _.findEntryIndex(key, h)) {
            if (_.entries[idx].leader_id == transfer_id) {
                return SingleFlightEnterResult{
                    is_leader: true,
                    coalesced: false,
                    leader_id: transfer_id,
                };
            }
            let w: usize = 0;
            while (w < _.entries[idx].waiter_count) : (w += 1) {
                if (_.entries[idx].waiters[w] == transfer_id) {
                    return SingleFlightEnterResult{
                        is_leader: false,
                        coalesced: true,
                        leader_id: _.entries[idx].leader_id,
                    };
                }
            }
            if (_.entries[idx].waiter_count >= MAX_SINGLE_FLIGHT_WAITERS) {
                return SingleFlightEnterResult{
                    gate_full: true,
                    leader_id: _.entries[idx].leader_id,
                };
            }
            const wc = _.entries[idx].waiter_count;
            _.entries[idx].waiters[wc] = transfer_id;
            _.entries[idx].waiter_count = wc + 1;
            return SingleFlightEnterResult{
                is_leader: false,
                coalesced: true,
                leader_id: _.entries[idx].leader_id,
            };
        }

        const free_idx = _.findFreeIndex() ?? return SingleFlightEnterResult{ gate_full: true };
        const copy_len = if (key.len < MAX_SINGLE_FLIGHT_KEY_LEN) key.len else MAX_SINGLE_FLIGHT_KEY_LEN;
        _.entries[free_idx].active = true;
        _.entries[free_idx].key_hash = h;
        _.entries[free_idx].key_len = key.len;
        _.entries[free_idx].key_prefix_len = copy_len;
        if (copy_len > 0) {
            std.mem.copyForwards(u8, _.entries[free_idx].key_prefix[0..copy_len], key[0..copy_len]);
        }
        _.entries[free_idx].leader_id = transfer_id;
        _.entries[free_idx].waiter_count = 0;
        _.active_keys += 1;

        return SingleFlightEnterResult{
            is_leader: true,
            coalesced: false,
            leader_id: transfer_id,
        };
    }

    /// Removes a specific `transfer_id` (waiter or leader) from `key`.
    /// If the leader is removed and no waiters remain, the entry is cleared.
    /// Returns true if `transfer_id` was found and removed.
    fn remove(*_, key: []const u8, transfer_id: TransferId): bool {
        const h = fnv1a64(key);
        const idx = _.findEntryIndex(key, h) ?? return false;

        if (_.entries[idx].leader_id == transfer_id) {
            if (_.entries[idx].waiter_count == 0) {
                _.entries[idx].reset();
                if (_.active_keys > 0) _.active_keys -= 1;
                return true;
            }
            // Promote first waiter to leader so remaining waiters stay grouped.
            _.entries[idx].leader_id = _.entries[idx].waiters[0];
            let j: usize = 1;
            while (j < _.entries[idx].waiter_count) : (j += 1) {
                _.entries[idx].waiters[j - 1] = _.entries[idx].waiters[j];
            }
            _.entries[idx].waiter_count -= 1;
            return true;
        }

        let w: usize = 0;
        while (w < _.entries[idx].waiter_count) : (w += 1) {
            if (_.entries[idx].waiters[w] == transfer_id) {
                let k: usize = w + 1;
                while (k < _.entries[idx].waiter_count) : (k += 1) {
                    _.entries[idx].waiters[k - 1] = _.entries[idx].waiters[k];
                }
                _.entries[idx].waiter_count -= 1;
                return true;
            }
        }
        return false;
    }

    /// Takes all coalesced waiter TransferIds for `key` into `out_waiters` and removes the entry.
    /// Returns the number of waiter TransferIds written to `out_waiters`.
    fn take(*_, key: []const u8, out_waiters: []TransferId): usize {
        const h = fnv1a64(key);
        const idx = _.findEntryIndex(key, h) ?? return 0;

        const total = _.entries[idx].waiter_count;
        const copy_count = if (total < out_waiters.len) total else out_waiters.len;
        let i: usize = 0;
        while (i < copy_count) : (i += 1) {
            out_waiters[i] = _.entries[idx].waiters[i];
        }
        _.entries[idx].reset();
        if (_.active_keys > 0) _.active_keys -= 1;
        return copy_count;
    }

    /// Clears all in-flight entries and waiters.
    fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_SINGLE_FLIGHT_KEYS) : (i += 1) {
            _.entries[i].reset();
        }
        _.active_keys = 0;
    }

    fn hasKey(key: []const u8): bool {
        const h = fnv1a64(key);
        return _.findEntryIndex(key, h) != null;
    }

    fn waiterCount(key: []const u8): usize {
        const h = fnv1a64(key);
        if (idx := _.findEntryIndex(key, h)) {
            return _.entries[idx].waiter_count;
        }
        return 0;
    }

    fn activeKeyCount(): usize {
        return _.active_keys;
    }
}
