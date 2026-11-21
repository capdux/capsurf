// test/render/test_png_encoder.cp
// CapSurf: Pure CP White-Box Unit Tests for PngCodec (chunks, RGBA roundtrip, 0x0 boundary) and style extraction fields

import { test, expect_eq } from "cap:test";
import { PngCodec } from "../../../src/render/png.cp";
import { ComputedStyle, LengthPercentageOrAuto } from "../../../src/css/cascade.cp";
import { CssColor } from "cap:css";

fn readU32BeTest(b: []const u8, offset: usize): u32 {
    const b0 = b[offset] as u32;
    const b1 = b[offset + 1] as u32;
    const b2 = b[offset + 2] as u32;
    const b3 = b[offset + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn testPngChunks()! {
    let rgba: [16]u8 = [
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        255, 255, 0, 128,
    ];
    const png = PngCodec.encodeRgbaWithLevel(rgba[0..16], 2, 2, 0);
    // Minimum size: 8 (sig) + 25 (IHDR) + 12 (IDAT hdr/crc) + 12 (IEND) = 57
    expect_eq(png.len > 57, true);

    // 1. PNG 8-byte signature: 89 50 4E 47 0D 0A 1A 0A
    expect_eq(png[0], @as(u8, 0x89));
    expect_eq(png[1], @as(u8, 0x50));
    expect_eq(png[2], @as(u8, 0x4E));
    expect_eq(png[3], @as(u8, 0x47));
    expect_eq(png[4], @as(u8, 0x0D));
    expect_eq(png[5], @as(u8, 0x0A));
    expect_eq(png[6], @as(u8, 0x1A));
    expect_eq(png[7], @as(u8, 0x0A));

    // 2. IHDR chunk at offset 8: length=13, type="IHDR", width=2, height=2, bit_depth=8, color_type=6 (RGBA)
    expect_eq(readU32BeTest(png, 8), @as(u32, 13));
    expect_eq(png[12], @as(u8, 'I'));
    expect_eq(png[13], @as(u8, 'H'));
    expect_eq(png[14], @as(u8, 'D'));
    expect_eq(png[15], @as(u8, 'R'));
    expect_eq(readU32BeTest(png, 16), @as(u32, 2));
    expect_eq(readU32BeTest(png, 20), @as(u32, 2));
    expect_eq(png[24], @as(u8, 8)); // bit depth
    expect_eq(png[25], @as(u8, 6)); // color type RGBA
    expect_eq(png[26], @as(u8, 0)); // compression
    expect_eq(png[27], @as(u8, 0)); // filter
    expect_eq(png[28], @as(u8, 0)); // interlace

    // 3. IDAT chunk at offset 33
    const idat_len = readU32BeTest(png, 33) as usize;
    expect_eq(idat_len > 0, true);
    expect_eq(png[37], @as(u8, 'I'));
    expect_eq(png[38], @as(u8, 'D'));
    expect_eq(png[39], @as(u8, 'A'));
    expect_eq(png[40], @as(u8, 'T'));

    // 4. IEND chunk at offset 33 + 12 + idat_len
    const iend_off = 33 + 12 + idat_len;
    expect_eq(iend_off + 12, png.len);
    expect_eq(readU32BeTest(png, iend_off), @as(u32, 0));
    expect_eq(png[iend_off + 4], @as(u8, 'I'));
    expect_eq(png[iend_off + 5], @as(u8, 'E'));
    expect_eq(png[iend_off + 6], @as(u8, 'N'));
    expect_eq(png[iend_off + 7], @as(u8, 'D'));
}

fn testRgba2x2And4x4Encoding()! {
    // 2x2 RGBA roundtrip at default level 6
    let rgba2: [16]u8 = [
        10, 20, 30, 255,
        40, 50, 60, 200,
        70, 80, 90, 150,
        100, 110, 120, 100,
    ];
    const png2 = PngCodec.encode(rgba2[0..16], 2, 2);
    expect_eq(png2.len > 0, true);
    if (img2 := PngCodec.decode(png2)) {
        expect_eq(img2.width, @as(u32, 2));
        expect_eq(img2.height, @as(u32, 2));
        const p00 = img2.getPixel(0, 0);
        expect_eq(p00.r, @as(u8, 10));
        expect_eq(p00.g, @as(u8, 20));
        expect_eq(p00.b, @as(u8, 30));
        expect_eq(p00.a, @as(u8, 255));
        const p11 = img2.getPixel(1, 1);
        expect_eq(p11.r, @as(u8, 100));
        expect_eq(p11.g, @as(u8, 110));
        expect_eq(p11.b, @as(u8, 120));
        expect_eq(p11.a, @as(u8, 100));
    } else {
        expect_eq(false, true);
    }

    // 4x4 RGBA roundtrip with Sub-filter (level 1) and max compression (level 9)
    let rgba4: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        rgba4[i * 4 + 0] = @as(u8, (i * 15) & 0xFF);
        rgba4[i * 4 + 1] = @as(u8, (255 - i * 12) & 0xFF);
        rgba4[i * 4 + 2] = @as(u8, (i * 7 + 30) & 0xFF);
        rgba4[i * 4 + 3] = 255;
    }

    const png4_lvl1 = PngCodec.encodeRgbaWithLevel(rgba4[0..64], 4, 4, 1);
    const png4_lvl9 = PngCodec.encodeRgbaWithLevel(rgba4[0..64], 4, 4, 9);
    expect_eq(png4_lvl1.len > 0, true);
    expect_eq(png4_lvl9.len > 0, true);

    if (img4 := PngCodec.decode(png4_lvl1)) {
        expect_eq(img4.width, @as(u32, 4));
        expect_eq(img4.height, @as(u32, 4));
        const p23 = img4.getPixel(2, 3); // index = 3*4 + 2 = 14
        expect_eq(p23.r, @as(u8, 14 * 15));
        expect_eq(p23.g, @as(u8, 255 - 14 * 12));
        expect_eq(p23.b, @as(u8, 14 * 7 + 30));
        expect_eq(p23.a, @as(u8, 255));
    } else {
        expect_eq(false, true);
    }

    if (img4_9 := PngCodec.decode(png4_lvl9)) {
        expect_eq(img4_9.width, @as(u32, 4));
        expect_eq(img4_9.height, @as(u32, 4));
        const p01 = img4_9.getPixel(1, 0); // index = 1
        expect_eq(p01.r, @as(u8, 15));
        expect_eq(p01.g, @as(u8, 255 - 12));
        expect_eq(p01.b, @as(u8, 37));
        expect_eq(p01.a, @as(u8, 255));
    } else {
        expect_eq(false, true);
    }
}

fn testZeroAndBoundaryHandling()! {
    let small: [4]u8 = [255, 0, 0, 255];
    // 0x0, 0xH, Wx0 must return empty slice
    expect_eq(PngCodec.encodeRgbaWithLevel(small[0..4], 0, 0, 6).len, @as(usize, 0));
    expect_eq(PngCodec.encodeRgbaWithLevel(small[0..4], 0, 2, 6).len, @as(usize, 0));
    expect_eq(PngCodec.encodeRgbaWithLevel(small[0..4], 2, 0, 6).len, @as(usize, 0));

    // Buffer smaller than width * height * 4 must return empty slice
    expect_eq(PngCodec.encodeRgbaWithLevel(small[0..4], 2, 2, 6).len, @as(usize, 0));

    // Invalid PNG bytes must decode to null
    expect_eq(PngCodec.decode("") == null, true);
    expect_eq(PngCodec.decode("NOT_A_PNG_STREAM") == null, true);
}

fn testStyleExtractionFields()! {
    // 1. Test ComputedStyle.extractPaintStyle field resolution
    let cs = ComputedStyle{};
    cs.color = CssColor.rgb(20, 40, 60);
    cs.background_color = CssColor.rgba(200, 100, 50, 255);
    cs.border_top_width = 4.0;
    cs.border_right_width = 2.0;
    cs.border_radius = LengthPercentageOrAuto.px(12.0);
    cs.border_tl_radius = LengthPercentageOrAuto.px(8.0);
    cs.opacity = 0.5;

    const ps = cs.extractPaintStyle(100.0, 16.0, 800.0, 600.0);
    expect_eq(ps.bg_color.r, @as(u8, 200));
    expect_eq(ps.bg_color.g, @as(u8, 100));
    expect_eq(ps.bg_color.b, @as(u8, 50));
    // Opacity 0.5 scales alpha from 255 -> ~127
    expect_eq(ps.bg_color.a >= 127 && ps.bg_color.a <= 128, true);
    // Border color falls back to currentColor (cs.color) when has_explicit_border_color is false
    expect_eq(ps.border_color.r, @as(u8, 20));
    expect_eq(ps.border_color.g, @as(u8, 40));
    expect_eq(ps.border_color.b, @as(u8, 60));
    expect_eq(ps.border_top_width == 4.0, true);
    expect_eq(ps.border_width == 4.0, true);
    expect_eq(ps.border_radius == 12.0, true);
    expect_eq(ps.border_tl_radius == 8.0, true);

    // 2. Test explicit border_color, opacity = 1.0, and text decoration flags
    let cs2 = ComputedStyle{};
    cs2.color = CssColor.rgb(11, 22, 33);
    cs2.border_color = CssColor.rgb(12, 34, 56);
    cs2.has_explicit_border_color = true;
    cs2.text_decoration_underline = true;
    cs2.opacity = 1.0;

    const ps2 = cs2.extractPaintStyle(100.0, 16.0, 800.0, 600.0);
    expect_eq(ps2.text_color.r, @as(u8, 11));
    expect_eq(ps2.text_color.g, @as(u8, 22));
    expect_eq(ps2.text_color.b, @as(u8, 33));
    expect_eq(ps2.border_color.r, @as(u8, 12));
    expect_eq(ps2.border_color.g, @as(u8, 34));
    expect_eq(ps2.border_color.b, @as(u8, 56));
    expect_eq(ps2.border_color.a, @as(u8, 255));
    expect_eq(ps2.text_decoration_underline, true);
}

fn main() {
    test("1. PNG signature, IHDR, IDAT, IEND chunk layout", () => {
        testPngChunks() catch unreachable;
    });

    test("2. 2x2 and 4x4 RGBA encoding/decoding roundtrip", () => {
        testRgba2x2And4x4Encoding() catch unreachable;
    });

    test("3. 0x0 and invalid buffer boundary handling", () => {
        testZeroAndBoundaryHandling() catch unreachable;
    });

    test("4. Style extraction fields (extractPaintStyle & resolveCssColor)", () => {
        testStyleExtractionFields() catch unreachable;
    });
}
