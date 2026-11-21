// test/eval/test_font_shaping.cp
// Unit tests for src/eval/font_shaping.cp

import {
    MAX_SHAPED_FONT_BYTES,
    MAX_CONCURRENT_TRANSFORMS,
    FontShapingSlotManager,
    FontShapingResult,
    isSupportedFont,
    shapeFontBytes,
} from "../../../src/eval/font_shaping.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn testSupportedFontFormats() {
    std.debug.print("1. Supported font format detection\n", .{});
    const ttf_bin: [6]u8 = [0x00, 0x01, 0x00, 0x00, 0xAA, 0xBB];
    const ttf_true: [4]u8 = ['t', 'r', 'u', 'e'];
    const otf_otto: [4]u8 = ['O', 'T', 'T', 'O'];
    const woff: [4]u8 = ['w', 'O', 'F', 'F'];
    const woff2: [4]u8 = ['w', 'O', 'F', '2'];

    check("TrueType 0x00010000 supported", isSupportedFont(ttf_bin[0..]));
    check("TrueType 'true' supported", isSupportedFont(ttf_true[0..]));
    check("OpenType 'OTTO' supported", isSupportedFont(otf_otto[0..]));
    check("WOFF 'wOFF' supported", isSupportedFont(woff[0..]));
    check("WOFF2 'wOF2' supported", isSupportedFont(woff2[0..]));
}

fn testShapeFontBytesAndOutputBounds() {
    std.debug.print("2. Font shaping & output buffer bounds\n", .{});
    const font_data: [8]u8 = ['O', 'T', 'T', 'O', 0x11, 0x22, 0x33, 0x44];
    let slots = FontShapingSlotManager{};
    let out_buf: [16]u8 = [0; 16];
    let res = FontShapingResult{};

    const ok = shapeFontBytes(font_data[0..], &slots, out_buf[0..], &res);
    check("shapeFontBytes succeeded", ok);
    check("res.shaped is true", res.shaped);
    check("res.queued is false", !res.queued);
    check("res.input_bytes == 8", res.input_bytes == 8);
    check("res.output_bytes == 8", res.output_bytes == 8);
    check("output matches input", std.mem.eql(u8, out_buf[0..8], font_data[0..]));
    check("slot released after shaping", slots.running == 0 && slots.queued == 0);
    check("slot manager available", slots.isAvailable());

    // Smaller output buffer truncates cleanly to out_buf.len
    let small_buf: [4]u8 = [0; 4];
    let res_small = FontShapingResult{};
    const ok_small = shapeFontBytes(font_data[0..], &slots, small_buf[0..], &res_small);
    check("shapeFontBytes with small buffer succeeded", ok_small);
    check("small buffer output_bytes == 4", res_small.output_bytes == 4);
    check("small buffer input_bytes == 8", res_small.input_bytes == 8);
    check("small buffer prefix matches", std.mem.eql(u8, small_buf[0..4], font_data[0..4]));
}

fn testSlotManagerBudgetAndQueue() {
    std.debug.print("3. Slot manager concurrency budget & queue\n", .{});
    let slots = FontShapingSlotManager{};
    check("initially available", slots.isAvailable());

    let i: usize = 0;
    while (i < MAX_CONCURRENT_TRANSFORMS) : (i += 1) {
        const queued = slots.acquireSlot();
        check("slot within budget not queued", !queued);
    }
    check("running == MAX_CONCURRENT_TRANSFORMS", slots.running == MAX_CONCURRENT_TRANSFORMS);
    check("queued == 0 at capacity", slots.queued == 0);
    check("not available at capacity", !slots.isAvailable());

    // Next acquire queues
    const q5 = slots.acquireSlot();
    check("5th slot is queued", q5);
    check("running == 5", slots.running == MAX_CONCURRENT_TRANSFORMS + 1);
    check("queued == 1", slots.queued == 1);

    // Shape while at capacity marks res.queued = true
    const woff2: [6]u8 = ['w', 'O', 'F', '2', 0x01, 0x02];
    let out_buf: [8]u8 = [0; 8];
    let res = FontShapingResult{};
    const ok = shapeFontBytes(woff2[0..], &slots, out_buf[0..], &res);
    check("queued shaping succeeded", ok);
    check("res.queued is true when saturated", res.queued);

    // Release all slots down to 0 without underflow
    let r: usize = 0;
    while (r < MAX_CONCURRENT_TRANSFORMS + 2) : (r += 1) {
        slots.releaseSlot();
    }
    check("running clamped at 0", slots.running == 0);
    check("queued clamped at 0", slots.queued == 0);
    check("available after full release", slots.isAvailable());
}

fn testInvalidAndEmptyInputBoundaries() {
    std.debug.print("4. Invalid, empty & max-size input boundaries\n", .{});
    let slots = FontShapingSlotManager{};
    let out_buf: [16]u8 = [0; 16];

    // Empty input
    const empty: []const u8 = "";
    let res_empty = FontShapingResult{ shaped: true, input_bytes: 99, output_bytes: 99 };
    check("empty isSupportedFont is false", !isSupportedFont(empty));
    check("empty shapeFontBytes returns false", !shapeFontBytes(empty, &slots, out_buf[0..], &res_empty));
    check("empty resets res.shaped", !res_empty.shaped);
    check("empty resets res.input_bytes", res_empty.input_bytes == 0);
    check("empty resets res.output_bytes", res_empty.output_bytes == 0);

    // Short (< 4 bytes) input
    const short_hdr: [3]u8 = ['O', 'T', 'T'];
    let res_short = FontShapingResult{};
    check("short header unsupported", !isSupportedFont(short_hdr[0..]));
    check("short header rejected", !shapeFontBytes(short_hdr[0..], &slots, out_buf[0..], &res_short));
    check("short header input_bytes == 3", res_short.input_bytes == 3);

    // Unsupported magic bytes
    const bad_magic: [4]u8 = ['P', 'N', 'G', 0x00];
    let res_bad = FontShapingResult{};
    check("unsupported magic rejected", !isSupportedFont(bad_magic[0..]));
    check("unsupported magic shape fails", !shapeFontBytes(bad_magic[0..], &slots, out_buf[0..], &res_bad));

    // Oversized input (> MAX_SHAPED_FONT_BYTES)
    check("MAX_SHAPED_FONT_BYTES is 8MB", MAX_SHAPED_FONT_BYTES == 8 * 1024 * 1024);
    let big_buf = std.heap.page_allocator.alloc(u8, MAX_SHAPED_FONT_BYTES + 1) catch @panic("oom in oversized font test");
    defer std.heap.page_allocator.free(big_buf);
    big_buf[0] = 'O';
    big_buf[1] = 'T';
    big_buf[2] = 'T';
    big_buf[3] = 'O';
    let res_big = FontShapingResult{};
    check("oversized font rejected", !shapeFontBytes(big_buf, &slots, out_buf[0..], &res_big));
    check("oversized font shaped is false", !res_big.shaped);
    check("oversized font output_bytes is 0", res_big.output_bytes == 0);
}

pub fn main() {
    std.debug.print("\n=== Font Shaping Unit Tests ===\n", .{});
    testSupportedFontFormats();
    testShapeFontBytesAndOutputBounds();
    testSlotManagerBudgetAndQueue();
    testInvalidAndEmptyInputBoundaries();
    std.debug.print("\nAll Font Shaping tests passed successfully!\n", .{});
}
