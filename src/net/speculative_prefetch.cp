// Migrated from vendor/kitesurf/packages/engine/src/eval/speculative-prefetch.ts
// src/net/speculative_prefetch.cp
// Runs speculative prefetches with a short deadline and fixed concurrency.

import { FetchInterceptionState, globMatches } from "../cdp/fetch_interception.cp";

@c {
    struct TimeVal {
        tv_sec: i64;
        tv_usec: i64;
    }
    fn gettimeofday(tv: *TimeVal, tz: *!?): i32;
}

pub fn getMilliTimestamp(): i64 {
    let tv = TimeVal{ tv_sec: 0, tv_usec: 0 };
    * = gettimeofday(&tv, null);
    return tv.tv_sec * 1000 + @divTrunc(tv.tv_usec, 1000);
}

pub const SPECULATIVE_DEADLINE_MS: u64 = 1000;
pub const SPECULATIVE_CONCURRENCY: usize = 2;
pub const MAX_PREFETCH_ITEMS: usize = 64;
pub const MAX_PREFETCH_LANES: usize = 8;
pub const MAX_FILTER_CONNECTIONS: usize = 32;
pub const MAX_PATTERNS_PER_CONNECTION: usize = 16;

pub struct PrefetchSignal {
    aborted: bool = false,
    deadline_ms: u64 = SPECULATIVE_DEADLINE_MS,
    start_time_ms: i64 = 0,

    fn isAborted(): bool {
        if (_.aborted) return true;
        const now = getMilliTimestamp();
        return (now - _.start_time_ms >= @as(i64, @intCast(_.deadline_ms)));
    }

    fn abort(*_) {
        _.aborted = true;
    }
}

pub struct PrefetchResult {
    url: []const u8 = "",
    status_code: u32 = 200,
    body: []const u8 = "",
    success: bool = false,
}

pub type PrefetchFetchFn = *const fn(url: []const u8, signal: *const PrefetchSignal, user_data: *!?): PrefetchResult?;

pub struct OutboundVerdict {
    blocked: bool = false,
    reason: []const u8 = "",
}

pub struct OutboundFilters {
    allow_patterns: [MAX_PATTERNS_PER_CONNECTION][]const u8 = [""; MAX_PATTERNS_PER_CONNECTION],
    allow_pattern_count: usize = 0,
    reject_patterns: [MAX_PATTERNS_PER_CONNECTION][]const u8 = [""; MAX_PATTERNS_PER_CONNECTION],
    reject_pattern_count: usize = 0,
    allow_resource_types: [MAX_PATTERNS_PER_CONNECTION][]const u8 = [""; MAX_PATTERNS_PER_CONNECTION],
    allow_resource_type_count: usize = 0,
    reject_resource_types: [MAX_PATTERNS_PER_CONNECTION][]const u8 = [""; MAX_PATTERNS_PER_CONNECTION],
    reject_resource_type_count: usize = 0,
    active: bool = false,

    fn addAllowPattern(*_, pattern: []const u8) {
        if (_.allow_pattern_count < MAX_PATTERNS_PER_CONNECTION) {
            _.allow_patterns[_.allow_pattern_count] = pattern;
            _.allow_pattern_count += 1;
        }
    }

    fn addRejectPattern(*_, pattern: []const u8) {
        if (_.reject_pattern_count < MAX_PATTERNS_PER_CONNECTION) {
            _.reject_patterns[_.reject_pattern_count] = pattern;
            _.reject_pattern_count += 1;
        }
    }

    fn addAllowResourceType(*_, rtype: []const u8) {
        if (_.allow_resource_type_count < MAX_PATTERNS_PER_CONNECTION) {
            _.allow_resource_types[_.allow_resource_type_count] = rtype;
            _.allow_resource_type_count += 1;
        }
    }

    fn addRejectResourceType(*_, rtype: []const u8) {
        if (_.reject_resource_type_count < MAX_PATTERNS_PER_CONNECTION) {
            _.reject_resource_types[_.reject_resource_type_count] = rtype;
            _.reject_resource_type_count += 1;
        }
    }
}

pub struct FilterRegistryEntry {
    conn_id: [64]u8 = [0; 64],
    conn_id_len: usize = 0,
    filters: OutboundFilters = OutboundFilters{},
    active: bool = false,
}

pub struct InterceptionRegistryEntry {
    conn_id: [64]u8 = [0; 64],
    conn_id_len: usize = 0,
    state: FetchInterceptionState = FetchInterceptionState{},
    active: bool = false,
}

pub struct OutboundFilterRegistry {
    entries: [MAX_FILTER_CONNECTIONS]FilterRegistryEntry = [FilterRegistryEntry{}; MAX_FILTER_CONNECTIONS],
    count: usize = 0,

    fn register(*_, conn_id: []const u8, filters: OutboundFilters) {
        let i: usize = 0;
        while (i < MAX_FILTER_CONNECTIONS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].conn_id[0.._.entries[i].conn_id_len], conn_id)) {
                _.entries[i].filters = filters;
                return;
            }
        }
        i = 0;
        while (i < MAX_FILTER_CONNECTIONS) : (i += 1) {
            if (!_.entries[i].active) {
                const len = @min(conn_id.len, 64);
                @memcpy(_.entries[i].conn_id[0..len], conn_id[0..len]);
                _.entries[i].conn_id_len = len;
                _.entries[i].filters = filters;
                _.entries[i].active = true;
                _.count += 1;
                return;
            }
        }
    }

    fn unregister(*_, conn_id: []const u8) {
        let i: usize = 0;
        while (i < MAX_FILTER_CONNECTIONS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].conn_id[0.._.entries[i].conn_id_len], conn_id)) {
                _.entries[i].active = false;
                _.entries[i].conn_id_len = 0;
                if (_.count > 0) _.count -= 1;
                return;
            }
        }
    }

    fn get(*_, conn_id: []const u8): (*OutboundFilters)? {
        let i: usize = 0;
        while (i < MAX_FILTER_CONNECTIONS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].conn_id[0.._.entries[i].conn_id_len], conn_id)) {
                return &_.entries[i].filters;
            }
        }
        return null;
    }
}

pub struct FetchInterceptionRegistry {
    entries: [MAX_FILTER_CONNECTIONS]InterceptionRegistryEntry = [InterceptionRegistryEntry{}; MAX_FILTER_CONNECTIONS],
    count: usize = 0,

    fn register(*_, conn_id: []const u8, state: FetchInterceptionState) {
        let i: usize = 0;
        while (i < MAX_FILTER_CONNECTIONS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].conn_id[0.._.entries[i].conn_id_len], conn_id)) {
                _.entries[i].state = state;
                return;
            }
        }
        i = 0;
        while (i < MAX_FILTER_CONNECTIONS) : (i += 1) {
            if (!_.entries[i].active) {
                const len = @min(conn_id.len, 64);
                @memcpy(_.entries[i].conn_id[0..len], conn_id[0..len]);
                _.entries[i].conn_id_len = len;
                _.entries[i].state = state;
                _.entries[i].active = true;
                _.count += 1;
                return;
            }
        }
    }

    fn unregister(*_, conn_id: []const u8) {
        let i: usize = 0;
        while (i < MAX_FILTER_CONNECTIONS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].conn_id[0.._.entries[i].conn_id_len], conn_id)) {
                _.entries[i].active = false;
                _.entries[i].conn_id_len = 0;
                if (_.count > 0) _.count -= 1;
                return;
            }
        }
    }

    fn get(*_, conn_id: []const u8): (*FetchInterceptionState)? {
        let i: usize = 0;
        while (i < MAX_FILTER_CONNECTIONS) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].conn_id[0.._.entries[i].conn_id_len], conn_id)) {
                return &_.entries[i].state;
            }
        }
        return null;
    }
}

@tls let g_interception_registry: FetchInterceptionRegistry = FetchInterceptionRegistry{};
@tls let g_outbound_registry: OutboundFilterRegistry = OutboundFilterRegistry{};

pub fn registerInterception(connection_id: []const u8, state: FetchInterceptionState) {
    g_interception_registry.register(connection_id, state);
}

pub fn unregisterInterception(connection_id: []const u8) {
    g_interception_registry.unregister(connection_id);
}

pub fn getInterception(connection_id: []const u8): (*FetchInterceptionState)? {
    return g_interception_registry.get(connection_id);
}

pub fn registerOutboundFilters(connection_id: []const u8, filters: OutboundFilters) {
    g_outbound_registry.register(connection_id, filters);
}

pub fn unregisterOutboundFilters(connection_id: []const u8) {
    g_outbound_registry.unregister(connection_id);
}

pub fn getOutboundFilters(connection_id: []const u8): (*OutboundFilters)? {
    return g_outbound_registry.get(connection_id);
}

pub fn interceptionMatches(st: *const FetchInterceptionState, url: []const u8, resource_type: []const u8): bool {
    if (std.mem.startsWith(u8, url, "data:") || std.mem.startsWith(u8, url, "blob:")) {
        return false;
    }
    let idx: usize = 0;
    while (idx < st.pattern_count) : (idx += 1) {
        const p = st.patterns[idx];
        if (p.resource_type.len > 0 && resource_type.len > 0) {
            if (!std.mem.eql(u8, p.resource_type, resource_type)) {
                continue;
            }
        }
        if (globMatches(p.pattern, url)) {
            return true;
        }
    }
    return false;
}

pub fn shouldBlock(filters: *const OutboundFilters, url: []const u8, resource_type: []const u8): OutboundVerdict {
    if (filters.reject_pattern_count > 0) {
        let i: usize = 0;
        while (i < filters.reject_pattern_count) : (i += 1) {
            if (globMatches(filters.reject_patterns[i], url)) {
                return OutboundVerdict{
                    blocked: true,
                    reason: "rejectRequestPattern matched",
                };
            }
        }
    }
    if (filters.reject_resource_type_count > 0) {
        let i: usize = 0;
        while (i < filters.reject_resource_type_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(filters.reject_resource_types[i], resource_type)) {
                return OutboundVerdict{
                    blocked: true,
                    reason: "rejectResourceTypes matched",
                };
            }
        }
    }
    if (filters.allow_pattern_count > 0) {
        let ok = false;
        let i: usize = 0;
        while (i < filters.allow_pattern_count) : (i += 1) {
            if (globMatches(filters.allow_patterns[i], url)) {
                ok = true;
                break;
            }
        }
        if (!ok) {
            return OutboundVerdict{
                blocked: true,
                reason: "allowRequestPattern did not match",
            };
        }
    }
    if (filters.allow_resource_type_count > 0) {
        let ok = false;
        let i: usize = 0;
        while (i < filters.allow_resource_type_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(filters.allow_resource_types[i], resource_type)) {
                ok = true;
                break;
            }
        }
        if (!ok) {
            return OutboundVerdict{
                blocked: true,
                reason: "allowResourceTypes did not match",
            };
        }
    }
    return OutboundVerdict{ blocked: false, reason: "" };
}

pub fn maySpeculate(connection_id: []const u8, url: []const u8): bool {
    if (connection_id.len > 0) {
        if (routes := getInterception(connection_id)) {
            if (interceptionMatches(routes, url, "Script")) {
                return false;
            }
        }
        if (filters := getOutboundFilters(connection_id)) {
            if (shouldBlock(filters, url, "Script").blocked) {
                return false;
            }
        }
    }
    return true;
}

pub fn prefetchBestEffort(
    items: []const []const u8,
    fetch_one: PrefetchFetchFn,
    user_data: *!?,
    deadline_ms: u64,
    concurrency: usize,
    out_settled: []PrefetchResult,
): usize {
    if (items.len == 0 || out_settled.len == 0) return 0;

    let signal = PrefetchSignal{
        aborted: false,
        deadline_ms: deadline_ms,
        start_time_ms: getMilliTimestamp(),
    };

    const eff_concurrency = if (concurrency == 0) 1 else @min(concurrency, items.len);
    let next: usize = 0;
    let settled_count: usize = 0;

    while (!signal.isAborted() && next < items.len) {
        let lane: usize = 0;
        while (lane < eff_concurrency && !signal.isAborted() && next < items.len) : (lane += 1) {
            const item = items[next];
            next += 1;
            if (item.len == 0) continue;

            const res_opt = fetch_one(item, &signal, user_data);
            if (!signal.isAborted()) {
                if (out := res_opt) {
                    if (settled_count < out_settled.len) {
                        out_settled[settled_count] = out;
                        settled_count += 1;
                    }
                }
            }
        }
    }

    signal.abort();
    return settled_count;
}

pub struct PrefetchBatchRunner {
    items: [MAX_PREFETCH_ITEMS][]const u8 = [""; MAX_PREFETCH_ITEMS],
    item_count: usize = 0,
    settled: [MAX_PREFETCH_ITEMS]PrefetchResult = [PrefetchResult{}; MAX_PREFETCH_ITEMS],
    settled_count: usize = 0,
    deadline_ms: u64 = SPECULATIVE_DEADLINE_MS,
    concurrency: usize = SPECULATIVE_CONCURRENCY,

    static fn init(deadline_ms: u64, concurrency: usize): PrefetchBatchRunner {
        return PrefetchBatchRunner{
            deadline_ms: deadline_ms,
            concurrency: if (concurrency == 0) 1 else concurrency,
        };
    }

    fn addItem(*_, url: []const u8): bool {
        if (_.item_count >= MAX_PREFETCH_ITEMS) return false;
        _.items[_.item_count] = url;
        _.item_count += 1;
        return true;
    }

    fn run(*_, fetch_one: PrefetchFetchFn, user_data: *!?): []const PrefetchResult {
        _.settled_count = prefetchBestEffort(
            _.items[0.._.item_count],
            fetch_one,
            user_data,
            _.deadline_ms,
            _.concurrency,
            &_.settled,
        );
        return _.settled[0.._.settled_count];
    }
}
