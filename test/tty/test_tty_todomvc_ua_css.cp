// test/tty/test_tty_todomvc_ua_css.cp
// White-box unit tests for ks_tty_css=0 reset, TodoMVC TTY UA stylesheet rules,
// and ttyUaStylesheet buffer boundary checks in src/tty/nav_params.cp.

import {
    TtyNavState,
    applyTtyNavParams,
    ttyUaStylesheet,
} from "../../src/tty/nav_params.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn testKsTtyCssZeroReset() {
    std.debug.print("1. ks_tty_css=0 Reset & Re-enable\n", .{});
    let state = TtyNavState{};

    check("set level 2 with theme and cell", applyTtyNavParams("?ks_tty_css=2&ks_color_scheme=dark&ks_tty_cell=10x20", &state));
    check("tty_ua_css == 2", state.tty_ua_css.? == 2);
    check("dark_color_scheme == true", state.dark_color_scheme.? == true);
    check("tty_cell_w == 10", state.tty_cell_w.? == 10);
    check("tty_cell_h == 20", state.tty_cell_h.? == 20);

    // ks_tty_css=0 must reset tty_ua_css to 0 while preserving unrelated fields
    check("ks_tty_css=0 seen", applyTtyNavParams("?ks_tty_css=0", &state));
    check("tty_ua_css reset to 0", state.tty_ua_css != null && state.tty_ua_css.? == 0);
    check("dark_color_scheme preserved after reset", state.dark_color_scheme.? == true);
    check("tty_cell_w preserved after reset", state.tty_cell_w.? == 10);
    check("tty_cell_h preserved after reset", state.tty_cell_h.? == 20);

    // Re-enable to level 1 after reset
    check("re-enable ks_tty_css=1", applyTtyNavParams("ks_tty_css=1", &state));
    check("tty_ua_css == 1", state.tty_ua_css.? == 1);

    // Non-numeric ks_tty_css also resets to 0 per nav_params.cp
    check("non-numeric ks_tty_css=off seen", applyTtyNavParams("ks_tty_css=off", &state));
    check("tty_ua_css reset to 0 on non-numeric", state.tty_ua_css != null && state.tty_ua_css.? == 0);
}

fn testTodoMvcUaStylesheetRules() {
    std.debug.print("2. TodoMVC TTY UA Stylesheet Rules (toggle-all & new-todo 3ch)\n", .{});
    let buf: [1024]u8 = [0; 1024];

    const css1 = ttyUaStylesheet(1, 17, 8, buf[0..]);
    check("level 1 non-empty", css1.len > 0);
    check("level 1 has toggle-all label rule", std.mem.indexOf(u8, css1, "input[type=\"checkbox\"][class*=\"toggle-all\" i] + label") != null);
    check("level 1 has font-size: 0", std.mem.indexOf(u8, css1, "font-size: 0 !important;") != null);
    check("level 1 has toggle-all label::before rule", std.mem.indexOf(u8, css1, "input[type=\"checkbox\"][class*=\"toggle-all\" i] + label::before") != null);
    check("level 1 has content: none", std.mem.indexOf(u8, css1, "content: none !important;") != null);
    check("level 1 has new-todo selector", std.mem.indexOf(u8, css1, "input:not([type=\"checkbox\"]):not([type=\"radio\"]).new-todo") != null);
    check("level 1 has padding-inline-start: 3ch", std.mem.indexOf(u8, css1, "padding-inline-start: 3ch !important;") != null);
    check("level 1 has cell-w 8px", std.mem.indexOf(u8, css1, "--ks-tty-cell-w: 8px !important;") != null);
    check("level 1 has cell-h 17px", std.mem.indexOf(u8, css1, "--ks-tty-cell-h: 17px !important;") != null);
    check("level 1 omits level 2 body overflow", std.mem.indexOf(u8, css1, "overflow: hidden") == null);

    let buf2: [1024]u8 = [0; 1024];
    const css2 = ttyUaStylesheet(2, 24, 12, buf2[0..]);
    check("level 2 non-empty", css2.len > css1.len);
    check("level 2 has toggle-all label rule", std.mem.indexOf(u8, css2, "input[type=\"checkbox\"][class*=\"toggle-all\" i] + label") != null);
    check("level 2 has new-todo 3ch rule", std.mem.indexOf(u8, css2, "padding-inline-start: 3ch !important;") != null);
    check("level 2 has body overflow rule", std.mem.indexOf(u8, css2, "body { margin: 0 !important; overflow: hidden !important; }") != null);
}

fn testStylesheetBufferBoundaries() {
    std.debug.print("3. ttyUaStylesheet Buffer Boundaries\n", .{});
    let full_buf: [1024]u8 = [0; 1024];

    const full1 = ttyUaStylesheet(1, 17, 8, full_buf[0..]);
    check("full1 len > 256", full1.len > 256);

    // Empty / tiny / mid-base buffers must return empty slice
    let zero_buf: [1]u8 = [0; 1];
    check("zero-length buffer returns empty", ttyUaStylesheet(1, 17, 8, zero_buf[0..0]).len == 0);

    let tiny_buf: [16]u8 = [0; 16];
    check("16-byte buffer returns empty", ttyUaStylesheet(1, 17, 8, tiny_buf[0..]).len == 0);

    let mid_buf: [256]u8 = [0; 256];
    check("256-byte buffer returns empty for base_css", ttyUaStylesheet(1, 17, 8, mid_buf[0..]).len == 0);

    // Exact N-1 vs N boundary for level 1
    let exact_buf1: [1024]u8 = [0; 1024];
    const short1 = ttyUaStylesheet(1, 17, 8, exact_buf1[0 .. full1.len - 1]);
    check("level 1 (len - 1) buffer returns empty", short1.len == 0);

    const fit1 = ttyUaStylesheet(1, 17, 8, exact_buf1[0..full1.len]);
    check("level 1 exact len succeeds", fit1.len == full1.len);
    check("level 1 exact len matches content", std.mem.eql(u8, fit1, full1));

    // Exact N-1 vs N boundary for level 2 (fails on level2_css append at N-1)
    let full_buf2: [1024]u8 = [0; 1024];
    const full2 = ttyUaStylesheet(2, 17, 8, full_buf2[0..]);
    check("full2 > full1", full2.len > full1.len);

    let exact_buf2: [1024]u8 = [0; 1024];
    const short2 = ttyUaStylesheet(2, 17, 8, exact_buf2[0 .. full2.len - 1]);
    check("level 2 (len - 1) buffer returns empty", short2.len == 0);

    const fit2 = ttyUaStylesheet(2, 17, 8, exact_buf2[0..full2.len]);
    check("level 2 exact len succeeds", fit2.len == full2.len);
    check("level 2 exact len matches content", std.mem.eql(u8, fit2, full2));
}

pub fn main() {
    std.debug.print("\n=== TTY TodoMVC UA CSS Unit Tests ===\n", .{});
    testKsTtyCssZeroReset();
    testTodoMvcUaStylesheetRules();
    testStylesheetBufferBoundaries();
    std.debug.print("\nAll 3 TTY TodoMVC UA CSS tests passed!\n", .{});
}
