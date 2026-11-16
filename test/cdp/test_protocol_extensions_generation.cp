// test/cdp/test_protocol_extensions_generation.cp
// Tests for Phase 23: CDP dispatch env, page script generation lifecycle & protocol extensions

import {
    DispatchEnv,
} from "../../src/cdp/dispatch_env.cp";

import {
    MAX_HOLDER_STUBS,
    RetireIntent,
    UnloadHarvest,
    PageScriptStub,
    StubHolder,
} from "../../src/cdp/page_script_generation.cp";

import {
    LayoutMode,
    ExtensionSessionState,
    handleProtocolExtension,
} from "../../src/cdp/protocol_extensions.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn testDispatchEnv() {
    std.debug.print("1. Dispatch Environment Configuration\n", .{});
    let env = DispatchEnv.initDefault();
    check("default cpu cap 5000", env.page_cpu_ms_cap == 5000);
    check("default wall cap 15000", env.page_wall_ms_cap == 15000);
    check("default private network disallowed", !env.isPrivateNetworkAllowed());

    env.setOption("cpu_cap_ms", "8000");
    check("updated cpu cap", env.page_cpu_ms_cap == 8000);

    env.setOption("wall_cap_ms", "25000");
    check("updated wall cap", env.page_wall_ms_cap == 25000);

    env.setOption("allow_private_network", "true");
    check("private network allowed", env.isPrivateNetworkAllowed());
}

fn testPageScriptGeneration() {
    std.debug.print("2. PageScript Generation & Stub Retirement\n", .{});
    let holder = StubHolder{};
    check("initial active count 0", holder.active_count == 0);

    // Reject stale stub
    const rejected = holder.publishStub(101, true);
    check("rejected stale stub", !rejected);
    check("active count still 0", holder.active_count == 0);

    // Publish valid stub
    const published = holder.publishStub(101, false);
    check("published valid stub", published);
    check("active count is 1", holder.active_count == 1);
    check("current generation is 1", holder.current_generation == 1);

    // Publish second stub
    const pub2 = holder.publishStub(102, false);
    check("published second stub", pub2);
    check("active count is 2", holder.active_count == 2);
    check("current generation is 2", holder.current_generation == 2);

    // Retire stub on navigation (harvest cookies)
    let harvest = UnloadHarvest{};
    const retired = holder.retireStub(101, RetireIntent.navigate, "user navigation", &harvest);
    check("retired 101 successfully", retired);
    check("active count is 1", holder.active_count == 1);
    check("harvested cookies > 0", harvest.cookie_count == 1);
    check("harvested cookie content", std.mem.indexOf(u8, harvest.getCookie(0), "session_token") != null);

    // Close remaining stub
    let harvest2 = UnloadHarvest{};
    const closed = holder.retireStub(102, RetireIntent.close_target, "tab close", &harvest2);
    check("retired 102 successfully", closed);
    check("active count is 0", holder.active_count == 0);
    check("close target harvest is empty", harvest2.cookie_count == 0);
}

fn testProtocolExtensions() {
    std.debug.print("3. CapSurf / Kitesurf Protocol Extensions\n", .{});
    let state = ExtensionSessionState{};
    let buf: [1024]u8 = [0; 1024];

    // getSessionInfo (CapSurf prefix)
    const res_info = handleProtocolExtension("CapSurf.getSessionInfo", "{}", "sess-1", &state, buf[0..]);
    check("getSessionInfo succeeded", res_info != null);
    if (info := res_info) {
        check("has sessionId", std.mem.indexOf(u8, info, "\"sessionId\":\"capsurf-session-001\"") != null);
        check("has entrypoint", std.mem.indexOf(u8, info, "\"entrypoint\":\"capsurf\"") != null);
    }

    // getSessionInfo (Kitesurf prefix compatibility)
    const res_ks_info = handleProtocolExtension("Kitesurf.getSessionInfo", "{}", "sess-1", &state, buf[0..]);
    check("Kitesurf.getSessionInfo succeeded", res_ks_info != null);

    // getLayoutMode & setLayoutMode
    const res_mode1 = handleProtocolExtension("CapSurf.getLayoutMode", "{}", "sess-1", &state, buf[0..]);
    check("default layout mode is full", res_mode1 != null && std.mem.indexOf(u8, res_mode1.?, "\"mode\":\"full\"") != null);

    const res_set_mode = handleProtocolExtension("CapSurf.setLayoutMode", "{\"mode\":\"off\"}", "sess-1", &state, buf[0..]);
    check("setLayoutMode succeeded", res_set_mode != null && std.mem.indexOf(u8, res_set_mode.?, "\"mode\":\"off\"") != null);
    check("state mode updated to off", state.layout_mode == LayoutMode.off);

    const res_mode2 = handleProtocolExtension("CapSurf.getLayoutMode", "{}", "sess-1", &state, buf[0..]);
    check("verified layout mode is off", res_mode2 != null && std.mem.indexOf(u8, res_mode2.?, "\"mode\":\"off\"") != null);

    // getRendererInfo
    const res_rend = handleProtocolExtension("CapSurf.getRendererInfo", "{}", "sess-1", &state, buf[0..]);
    check("getRendererInfo succeeded", res_rend != null && std.mem.indexOf(u8, res_rend.?, "\"enabled\":true") != null);

    // getPageState
    const res_page = handleProtocolExtension("CapSurf.getPageState", "{}", "sess-1", &state, buf[0..]);
    check("getPageState succeeded", res_page != null && std.mem.indexOf(u8, res_page.?, "\"href\":\"about:blank\"") != null);

    // setControlValue
    const res_ctrl = handleProtocolExtension("CapSurf.setControlValue", "{\"nodeId\":5,\"value\":\"test\"}", "sess-1", &state, buf[0..]);
    check("setControlValue succeeded", res_ctrl != null && std.mem.indexOf(u8, res_ctrl.?, "\"changed\":true") != null);

    // Unhandled method returns null
    const res_unknown = handleProtocolExtension("Page.navigate", "{}", "sess-1", &state, buf[0..]);
    check("unknown extension method returns null", res_unknown == null);
}

pub fn main() {
    std.debug.print("\n=== Phase 23: CDP Dispatch Env, Generation & Extensions Tests ===\n", .{});
    testDispatchEnv();
    testPageScriptGeneration();
    testProtocolExtensions();
    std.debug.print("\nAll Phase 23 tests passed successfully!\n", .{});
}
