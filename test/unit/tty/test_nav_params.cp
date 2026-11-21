// test/tty/test_nav_params.cp
// Unit tests for src/tty/nav_params.cp

import {
    TTY_CELL_PX_MIN,
    TTY_CELL_PX_MAX,
    DEFAULT_TTY_CELL_WIDTH_PX,
    DEFAULT_TTY_CELL_HEIGHT_PX,
    TtyNavState,
    parseCell,
    applyTtyNavParams,
    ttyFocusStylesheet,
    ttyUaStylesheet,
} from "../../../src/tty/nav_params.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn testDefaultsAndConstants() {
    std.debug.print("1. Defaults, Constants & Empty Queries\n", .{});
    check("TTY_CELL_PX_MIN == 1", TTY_CELL_PX_MIN == 1);
    check("TTY_CELL_PX_MAX == 64", TTY_CELL_PX_MAX == 64);
    check("DEFAULT_TTY_CELL_WIDTH_PX == 8", DEFAULT_TTY_CELL_WIDTH_PX == 8);
    check("DEFAULT_TTY_CELL_HEIGHT_PX == 17", DEFAULT_TTY_CELL_HEIGHT_PX == 17);

    let state = TtyNavState{};
    check("default tty_ua_css is null", state.tty_ua_css == null);
    check("default tty_cell_w is null", state.tty_cell_w == null);
    check("default tty_cell_h is null", state.tty_cell_h == null);
    check("default dark_color_scheme is null", state.dark_color_scheme == null);

    check("empty query returns false", !applyTtyNavParams("", &state));
    check("bare ? returns false", !applyTtyNavParams("?", &state));
    check("unrelated params return false", !applyTtyNavParams("?foo=1&bar=baz", &state));
    check("state remains null after unrelated params", state.tty_ua_css == null && state.tty_cell_w == null && state.dark_color_scheme == null);
}

fn testParseCellBoundaries() {
    std.debug.print("2. parseCell Boundaries & Invalid Inputs\n", .{});
    let w: u32 = 99;
    let h: u32 = 99;

    check("min boundary 1x1", parseCell("1x1", &w, &h));
    check("1x1 w == 1", w == 1);
    check("1x1 h == 1", h == 1);

    check("max boundary 64x64", parseCell("64x64", &w, &h));
    check("64x64 w == 64", w == 64);
    check("64x64 h == 64", h == 64);

    check("mixed boundary 1x64", parseCell("1x64", &w, &h));
    check("1x64 w == 1", w == 1);
    check("1x64 h == 64", h == 64);

    check("default cell 8x17", parseCell("8x17", &w, &h));
    check("8x17 w == 8", w == 8);
    check("8x17 h == 17", h == 17);

    // Invalid inputs must return false and preserve previous out_w/out_h
    check("reject 0x17", !parseCell("0x17", &w, &h));
    check("reject 17x0", !parseCell("17x0", &w, &h));
    check("reject 65x17", !parseCell("65x17", &w, &h));
    check("reject 17x65", !parseCell("17x65", &w, &h));
    check("reject empty", !parseCell("", &w, &h));
    check("reject missing x", !parseCell("8", &w, &h));
    check("reject leading x", !parseCell("x17", &w, &h));
    check("reject trailing x", !parseCell("8x", &w, &h));
    check("reject uppercase X", !parseCell("8X17", &w, &h));
    check("reject negative", !parseCell("-8x17", &w, &h));
    check("reject extra x", !parseCell("8x17x2", &w, &h));
    check("reject non-numeric", !parseCell("abcxdef", &w, &h));
    check("out_w preserved on failure", w == 8);
    check("out_h preserved on failure", h == 17);
}

fn testApplyTtyNavParamsParsingAndClamping() {
    std.debug.print("3. Query Parsing, Theme Extraction & Clamping\n", .{});
    let state = TtyNavState{};

    const q1 = "?ks_tty_css=1&ks_color_scheme=dark&ks_tty_cell=12x24";
    check("q1 seen", applyTtyNavParams(q1, &state));
    check("q1 tty_ua_css == 1", state.tty_ua_css.? == 1);
    check("q1 dark_color_scheme == true", state.dark_color_scheme.? == true);
    check("q1 tty_cell_w == 12", state.tty_cell_w.? == 12);
    check("q1 tty_cell_h == 24", state.tty_cell_h.? == 24);

    // Without leading '?' + clamping ks_tty_css > 2 -> 2 + light scheme
    const q2 = "ks_tty_css=99&ks_color_scheme=light&ks_tty_cell=8x17";
    check("q2 seen", applyTtyNavParams(q2, &state));
    check("q2 clamped tty_ua_css == 2", state.tty_ua_css.? == 2);
    check("q2 dark_color_scheme == false", state.dark_color_scheme.? == false);
    check("q2 tty_cell_w == 8", state.tty_cell_w.? == 8);
    check("q2 tty_cell_h == 17", state.tty_cell_h.? == 17);

    // Invalid / zero values must not clobber valid state
    const q3 = "?ks_tty_css=0&ks_color_scheme=sepia&ks_tty_cell=0x99";
    check("q3 seen", applyTtyNavParams(q3, &state));
    check("tty_ua_css reset on 0", state.tty_ua_css.? == 0);
    check("dark_color_scheme preserved on unknown", state.dark_color_scheme.? == false);
    check("tty_cell_w preserved on invalid", state.tty_cell_w.? == 8);
    check("tty_cell_h preserved on invalid", state.tty_cell_h.? == 17);

    // Pairs without '=' are ignored without breaking subsequent valid pairs
    let state2 = TtyNavState{};
    const q4 = "ks_tty_css&foo&ks_color_scheme=dark&ks_tty_css=abc";
    check("q4 seen", applyTtyNavParams(q4, &state2));
    check("q4 dark_color_scheme == true", state2.dark_color_scheme.? == true);
    check("q4 tty_ua_css reset to 0 on non-numeric", state2.tty_ua_css.? == 0);
}

fn testStylesheetGenerationAndBuffers() {
    std.debug.print("4. Stylesheet Generation & Buffer Bounds\n", .{});
    const focus = ttyFocusStylesheet();
    check("focus stylesheet exact", std.mem.eql(u8, focus, ":focus { outline: 1px dotted currentColor !important; }"));

    let buf: [1024]u8 = [0; 1024];
    const css1 = ttyUaStylesheet(1, 17, 8, buf[0..]);
    check("level 1 non-empty", css1.len > 0);
    check("level 1 has monospace", std.mem.indexOf(u8, css1, "font-family: monospace !important;") != null);
    check("level 1 has cell-w 8px", std.mem.indexOf(u8, css1, "--ks-tty-cell-w: 8px !important;") != null);
    check("level 1 has cell-h 17px", std.mem.indexOf(u8, css1, "--ks-tty-cell-h: 17px !important;") != null);
    check("level 1 omits body overflow", std.mem.indexOf(u8, css1, "overflow: hidden") == null);

    const css2 = ttyUaStylesheet(2, 24, 12, buf[0..]);
    check("level 2 has cell-w 12px", std.mem.indexOf(u8, css2, "--ks-tty-cell-w: 12px !important;") != null);
    check("level 2 has cell-h 24px", std.mem.indexOf(u8, css2, "--ks-tty-cell-h: 24px !important;") != null);
    check("level 2 has body overflow", std.mem.indexOf(u8, css2, "body { margin: 0 !important; overflow: hidden !important; }") != null);

    let tiny_buf: [16]u8 = [0; 16];
    const css_tiny = ttyUaStylesheet(2, 17, 8, tiny_buf[0..]);
    check("undersized buffer returns empty", css_tiny.len == 0);
}

pub fn main() {
    std.debug.print("\n=== TTY Nav Params Unit Tests ===\n", .{});
    testDefaultsAndConstants();
    testParseCellBoundaries();
    testApplyTtyNavParamsParsingAndClamping();
    testStylesheetGenerationAndBuffers();
    std.debug.print("\nAll 4 TTY Nav Params tests passed!\n", .{});
}
