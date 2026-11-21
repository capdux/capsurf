// test/tty/test_tty_font_bundle.cp
// Tests for Phase 22: TTY nav params, font shaping, script bundling and guards

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

import {
    MAX_SHAPED_FONT_BYTES,
    MAX_CONCURRENT_TRANSFORMS,
    FontShapingSlotManager,
    FontShapingResult,
    isSupportedFont,
    shapeFontBytes,
} from "../../../src/eval/font_shaping.cp";

import {
    INLINE_SCRIPT_BODY_KEY_PREFIX_CHARS,
    ScriptKind,
    ScriptDescriptorEntry,
    ScriptBundle,
    countLines,
    encodeVLQ,
    commentSafe,
    categorizeScript,
    buildScriptBundle,
    isOneOf,
    isNonEmptyString,
} from "../../../src/eval/script_bundle.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn testTtyNavParams() {
    std.debug.print("1. TTY Cell Dimensions & Nav Params\n", .{});
    let w: u32 = 0;
    let h: u32 = 0;
    check("parseCell 8x17", parseCell("8x17", &w, &h));
    check("w is 8", w == 8);
    check("h is 17", h == 17);

    check("parseCell 16x32", parseCell("16x32", &w, &h));
    check("w is 16", w == 16);
    check("h is 32", h == 32);

    check("reject out of bounds low", !parseCell("0x17", &w, &h));
    check("reject out of bounds high", !parseCell("8x65", &w, &h));
    check("reject invalid format", !parseCell("8-17", &w, &h));
    check("reject non-numeric", !parseCell("abc", &w, &h));

    let state = TtyNavState{};
    const query = "?ks_tty_css=2&ks_color_scheme=dark&ks_tty_cell=10x20";
    const applied = applyTtyNavParams(query, &state);
    check("applied params", applied);
    check("tty_ua_css is 2", state.tty_ua_css.? == 2);
    check("dark_color_scheme is true", state.dark_color_scheme.? == true);
    check("tty_cell_w is 10", state.tty_cell_w.? == 10);
    check("tty_cell_h is 20", state.tty_cell_h.? == 20);

    // Test Stylesheet generation
    let css_buf: [1024]u8 = [0; 1024];
    const ua_css = ttyUaStylesheet(2, 20, 10, css_buf[0..]);
    check("ua_css contains cell-w", std.mem.indexOf(u8, ua_css, "--ks-tty-cell-w: 10px") != null);
    check("ua_css contains cell-h", std.mem.indexOf(u8, ua_css, "--ks-tty-cell-h: 20px") != null);
    check("ua_css contains level 2 body overflow", std.mem.indexOf(u8, ua_css, "overflow: hidden") != null);

    const focus_css = ttyFocusStylesheet();
    check("focus css contains outline", std.mem.indexOf(u8, focus_css, "outline:") != null);
}

fn testFontShaping() {
    std.debug.print("2. Font Signature & Shaping Limits\n", .{});
    const otf_header: [4]u8 = ['O', 'T', 'T', 'O'];
    const woff_header: [4]u8 = ['w', 'O', 'F', 'F'];
    const woff2_header: [4]u8 = ['w', 'O', 'F', '2'];
    const ttf_header: [4]u8 = [0x00, 0x01, 0x00, 0x00];
    const invalid_header: [4]u8 = ['P', 'N', 'G', '8'];

    check("supports OTTO", isSupportedFont(otf_header[0..]));
    check("supports wOFF", isSupportedFont(woff_header[0..]));
    check("supports wOF2", isSupportedFont(woff2_header[0..]));
    check("supports TrueType", isSupportedFont(ttf_header[0..]));
    check("rejects invalid font", !isSupportedFont(invalid_header[0..]));

    let slots = FontShapingSlotManager{};
    check("initial slots available", slots.isAvailable());
    check("slot 1 immediate", !slots.acquireSlot());
    check("slot 2 immediate", !slots.acquireSlot());
    check("slot 3 immediate", !slots.acquireSlot());
    check("slot 4 immediate", !slots.acquireSlot());
    check("slots full", !slots.isAvailable());
    check("slot 5 queued", slots.acquireSlot());

    slots.releaseSlot();
    slots.releaseSlot();
    slots.releaseSlot();
    slots.releaseSlot();
    slots.releaseSlot();
    check("all slots released", slots.isAvailable());

    let out_buf: [128]u8 = [0; 128];
    let res = FontShapingResult{};
    const ok = shapeFontBytes(otf_header[0..], &slots, out_buf[0..], &res);
    check("shaping succeeded", ok);
    check("res is shaped", res.shaped);
    check("res output_bytes is 4", res.output_bytes == 4);
}

fn testScriptBundleAndVLQ() {
    std.debug.print("3. Source Map VLQ & Script Bundling\n", .{});
    let vlq_buf: [16]u8 = [0; 16];
    // In Base64 VLQ: 0 -> 'A', 1 -> 'C', -1 -> 'D'
    const vlq0 = encodeVLQ(0, vlq_buf[0..]);
    check("VLQ 0 is A", std.mem.eql(u8, vlq0, "A"));

    const vlq1 = encodeVLQ(1, vlq_buf[0..]);
    check("VLQ 1 is C", std.mem.eql(u8, vlq1, "C"));

    const vlq_neg1 = encodeVLQ(-1, vlq_buf[0..]);
    check("VLQ -1 is D", std.mem.eql(u8, vlq_neg1, "D"));

    // Line counting and comment safety
    const multiline = "line 1\nline 2\nline 3\n";
    check("countLines is 3", countLines(multiline) == 3);

    let comment_buf: [64]u8 = [0; 64];
    const safe = commentSafe("hello\nworld\r!", comment_buf[0..]);
    check("safe escapes newline", std.mem.indexOf(u8, safe, "%0A") != null);
    check("safe escapes cr", std.mem.indexOf(u8, safe, "%0D") != null);

    // Script categorization
    let d_injected = ScriptDescriptorEntry{ injected_flag: true };
    check("injected script", categorizeScript(d_injected) == ScriptKind.injected);

    let d_mod = ScriptDescriptorEntry{ kind: ScriptKind.module_script };
    check("module script", categorizeScript(d_mod) == ScriptKind.module_script);

    let d_async = ScriptDescriptorEntry{ src: "async.js", async_flag: true };
    check("async script", categorizeScript(d_async) == ScriptKind.async_script);

    let d_defer = ScriptDescriptorEntry{ src: "defer.js", defer_flag: true };
    check("deferred script", categorizeScript(d_defer) == ScriptKind.deferred);

    let d_reg = ScriptDescriptorEntry{ src: "main.js" };
    check("regular script", categorizeScript(d_reg) == ScriptKind.regular);

    let descs: [5]ScriptDescriptorEntry = [
        d_injected,
        d_mod,
        d_async,
        d_defer,
        d_reg,
    ];

    let bundle = ScriptBundle{};
    buildScriptBundle(descs[0..], &bundle);
    check("bundle total is 5", bundle.total_scripts == 5);
    check("bundle injected is 1", bundle.injected_count == 1);
    check("bundle module is 1", bundle.module_count == 1);
    check("bundle async is 1", bundle.async_count == 1);
    check("bundle deferred is 1", bundle.deferred_count == 1);
    check("bundle regular is 1", bundle.regular_count == 1);

    // Guards
    let allowed: [3][]const u8 = ["GET", "POST", "HEAD"];
    check("isOneOf POST", isOneOf("POST", allowed[0..]));
    check("isOneOf not DELETE", !isOneOf("DELETE", allowed[0..]));
    check("isNonEmptyString yes", isNonEmptyString("abc"));
    check("isNonEmptyString no", !isNonEmptyString(""));
}

pub fn main() {
    std.debug.print("\n=== Phase 22: TTY Params, Font Shaping & Script Bundle Tests ===\n", .{});
    testTtyNavParams();
    testFontShaping();
    testScriptBundleAndVLQ();
    std.debug.print("\nAll Phase 22 tests passed successfully!\n", .{});
}
