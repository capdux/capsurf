// Migrated from vendor/kitesurf/packages/engine/src/cdp/dispatch-env.ts
// Builds and manages the dispatch environment configuration passed to CDP handlers.

pub struct DispatchEnv {
    page_cpu_ms_cap: u32 = 5000,
    page_wall_ms_cap: u32 = 15000,
    resource_drain_quiet_turns: u32 = 2,
    simulate_prod_eval: bool = false,
    lag_watchdog_enabled: bool = true,
    allow_private_network: bool = false,
    font_assets_path: []const u8 = "",
    loader_url: []const u8 = "",

    fn initDefault(): DispatchEnv {
        return DispatchEnv{};
    }

    fn setOption(*_, key: []const u8, val: []const u8) {
        if (std.mem.eql(u8, key, "cpu_cap_ms")) {
            if (num := parseU32(val)) {
                _.page_cpu_ms_cap = num;
            }
        } else if (std.mem.eql(u8, key, "wall_cap_ms")) {
            if (num := parseU32(val)) {
                _.page_wall_ms_cap = num;
            }
        } else if (std.mem.eql(u8, key, "drain_turns")) {
            if (num := parseU32(val)) {
                _.resource_drain_quiet_turns = num;
            }
        } else if (std.mem.eql(u8, key, "allow_private_network")) {
            _.allow_private_network = std.mem.eql(u8, val, "true") || std.mem.eql(u8, val, "1");
        } else if (std.mem.eql(u8, key, "simulate_prod")) {
            _.simulate_prod_eval = std.mem.eql(u8, val, "true") || std.mem.eql(u8, val, "1");
        }
    }

    fn isPrivateNetworkAllowed(): bool {
        return _.allow_private_network;
    }
}

fn parseU32(s: []const u8): u32? {
    if (s.len == 0) return null;
    let val: u32 = 0;
    for (i in 0..s.len) {
        let ch = s[i];
        if (ch < '0' || ch > '9') return null;
        val = val * 10 + @as(u32, ch - '0');
    }
    return val;
}
