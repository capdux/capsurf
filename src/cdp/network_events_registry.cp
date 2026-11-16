// Migrated from vendor/kitesurf/packages/engine/src/cdp/network-events-registry.ts
// Stores the NetworkEventTracker for each active CDP connection.

import { NetworkEventTracker } from "./network_events.cp";

pub const CONNECTION_ID_HEADER: []const u8 = "x-kitesurf-connection-id";
pub const FRAME_ID_HEADER: []const u8 = "x-kitesurf-frame-id";

pub const MAX_TRACKER_CONNECTIONS: usize = 64;

pub struct TrackerRegistryEntry {
    conn_id: [64]u8 = [0; 64],
    conn_id_len: usize = 0,
    tracker_ptr: (*NetworkEventTracker)? = null,
    owned_tracker: NetworkEventTracker = NetworkEventTracker{},
    is_owned: bool = false,
    active: bool = false,
}

pub struct NetworkEventTrackerRegistry {
    entries: [MAX_TRACKER_CONNECTIONS]TrackerRegistryEntry = [TrackerRegistryEntry{}; MAX_TRACKER_CONNECTIONS],
    count: usize = 0,

    static fn init(): NetworkEventTrackerRegistry {
        return NetworkEventTrackerRegistry{};
    }

    fn register(*_, conn_id: []const u8, tracker: *NetworkEventTracker) {
        let i: usize = 0;
        while (i < MAX_TRACKER_CONNECTIONS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].conn_id[0.._.entries[i].conn_id_len], conn_id)) {
                _.entries[i].tracker_ptr = tracker;
                _.entries[i].is_owned = false;
                return;
            }
        }
        i = 0;
        while (i < MAX_TRACKER_CONNECTIONS) : (i += 1) {
            if (!_.entries[i].active) {
                const len = @min(conn_id.len, 64);
                @memcpy(_.entries[i].conn_id[0..len], conn_id[0..len]);
                _.entries[i].conn_id_len = len;
                _.entries[i].tracker_ptr = tracker;
                _.entries[i].is_owned = false;
                _.entries[i].active = true;
                _.count += 1;
                return;
            }
        }
    }

    fn registerOwned(*_, conn_id: []const u8, tracker: NetworkEventTracker) {
        let i: usize = 0;
        while (i < MAX_TRACKER_CONNECTIONS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].conn_id[0.._.entries[i].conn_id_len], conn_id)) {
                _.entries[i].owned_tracker = tracker;
                _.entries[i].is_owned = true;
                _.entries[i].tracker_ptr = null;
                return;
            }
        }
        i = 0;
        while (i < MAX_TRACKER_CONNECTIONS) : (i += 1) {
            if (!_.entries[i].active) {
                const len = @min(conn_id.len, 64);
                @memcpy(_.entries[i].conn_id[0..len], conn_id[0..len]);
                _.entries[i].conn_id_len = len;
                _.entries[i].owned_tracker = tracker;
                _.entries[i].is_owned = true;
                _.entries[i].tracker_ptr = null;
                _.entries[i].active = true;
                _.count += 1;
                return;
            }
        }
    }

    fn unregister(*_, conn_id: []const u8) {
        let i: usize = 0;
        while (i < MAX_TRACKER_CONNECTIONS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].conn_id[0.._.entries[i].conn_id_len], conn_id)) {
                _.entries[i].active = false;
                _.entries[i].conn_id_len = 0;
                _.entries[i].tracker_ptr = null;
                _.entries[i].is_owned = false;
                if (_.count > 0) _.count -= 1;
                return;
            }
        }
    }

    fn get(*_, conn_id: []const u8): (*NetworkEventTracker)? {
        let i: usize = 0;
        while (i < MAX_TRACKER_CONNECTIONS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].conn_id[0.._.entries[i].conn_id_len], conn_id)) {
                if (_.entries[i].is_owned) {
                    return &_.entries[i].owned_tracker;
                } else {
                    return _.entries[i].tracker_ptr;
                }
            }
        }
        return null;
    }

    fn has(*_, conn_id: []const u8): bool {
        let i: usize = 0;
        while (i < MAX_TRACKER_CONNECTIONS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].conn_id[0.._.entries[i].conn_id_len], conn_id)) {
                return true;
            }
        }
        return false;
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_TRACKER_CONNECTIONS) : (i += 1) {
            _.entries[i].active = false;
            _.entries[i].conn_id_len = 0;
            _.entries[i].tracker_ptr = null;
            _.entries[i].is_owned = false;
        }
        _.count = 0;
    }

    fn size(*_): usize {
        return _.count;
    }
}

@tls let g_tracker_registry: NetworkEventTrackerRegistry = NetworkEventTrackerRegistry.init();

pub fn registerTracker(connection_id: []const u8, tracker: *NetworkEventTracker) {
    g_tracker_registry.register(connection_id, tracker);
}

pub fn registerTrackerOwned(connection_id: []const u8, tracker: NetworkEventTracker) {
    g_tracker_registry.registerOwned(connection_id, tracker);
}

pub fn unregisterTracker(connection_id: []const u8) {
    g_tracker_registry.unregister(connection_id);
}

pub fn getTracker(connection_id: []const u8): (*NetworkEventTracker)? {
    return g_tracker_registry.get(connection_id);
}
