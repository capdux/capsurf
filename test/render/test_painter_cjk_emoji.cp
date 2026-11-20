// test/render/test_painter_cjk_emoji.cp
// CapSurf Pure CP Unit Test: CJK Glyph & Color Emoji Rasterization + Clipping

import { test, expect_eq } from "cap:test";
import { Canvas, ClipRect } from "../../src/render/painter.cp";
import { CssColor } from "cap:css";

fn countNonBgPixels(canvas: *const Canvas, x0: i32, y0: i32, x1: i32, y1: i32, bg: CssColor): usize {
    let count: usize = 0;
    let py = y0;
    while (py < y1) : (py += 1) {
        let px = x0;
        while (px < x1) : (px += 1) {
            const c = canvas.getPixel(px, py);
            if (c.r != bg.r || c.g != bg.g || c.b != bg.b || c.a != bg.a) {
                count += 1;
            }
        }
    }
    return count;
}

fn countExactColorPixels(canvas: *const Canvas, x0: i32, y0: i32, x1: i32, y1: i32, target: CssColor): usize {
    let count: usize = 0;
    let py = y0;
    while (py < y1) : (py += 1) {
        let px = x0;
        while (px < x1) : (px += 1) {
            const c = canvas.getPixel(px, py);
            if (c.r == target.r && c.g == target.g && c.b == target.b && c.a == target.a) {
                count += 1;
            }
        }
    }
    return count;
}

fn testCjkHanziKanaHangulDrawText()! {
    const bg = CssColor.rgb(255, 255, 255);
    const fg = CssColor.rgb(12, 34, 56);
    let canvas = Canvas.init(std.heap.page_allocator, 160, 80, bg) catch unreachable;
    defer canvas.deinit();

    // Hanzi ("中文"), Hiragana/Katakana ("あア"), Hangul ("한글")
    canvas.drawText(8.0, 8.0, "中文", fg);
    canvas.drawText(8.0, 24.0, "あア", fg);
    canvas.drawText(8.0, 40.0, "한글", fg);

    // Each 2-glyph 8x8 run occupies x: [8, 24), y: [base_y+2, base_y+10)
    const hanzi_px = countNonBgPixels(&canvas, 8, 10, 24, 18, bg);
    const kana_px = countNonBgPixels(&canvas, 8, 26, 24, 34, bg);
    const hangul_px = countNonBgPixels(&canvas, 8, 42, 24, 50, bg);

    expect_eq(hanzi_px > 10, true);
    expect_eq(kana_px > 10, true);
    expect_eq(hangul_px > 10, true);

    // Verify exact foreground color is used for CJK glyphs
    const hanzi_fg = countExactColorPixels(&canvas, 8, 10, 24, 18, fg);
    expect_eq(hanzi_fg, hanzi_px);

    // Verify scaled CJK text (16px => scale 2.0) paints significantly more pixels than 1x
    canvas.drawScaledTextClipped(40.0, 8.0, "字", 16.0, fg, null);
    const scaled_px = countNonBgPixels(&canvas, 40, 8, 56, 24, bg);
    expect_eq(scaled_px > 40, true);
}

fn testDrawGlyph8x8ScaledAndClipped()! {
    const bg = CssColor.rgb(255, 255, 255);
    const fg = CssColor.rgb(200, 40, 40);
    let canvas = Canvas.init(std.heap.page_allocator, 80, 60, bg) catch unreachable;
    defer canvas.deinit();

    // 8x8 hollow square glyph
    const box_glyph: [8]u8 = [0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF];

    // 1x unclipped at (4, 4) -> [4, 12) x [4, 12)
    canvas.drawGlyph8x8ScaledClipped(4.0, 4.0, 1.0, box_glyph[0..8], fg, null);
    // Border has 8+8+6+6 = 28 set pixels; center (7, 7) must remain background
    expect_eq(countExactColorPixels(&canvas, 4, 4, 12, 12, fg), 28);
    const center_px = canvas.getPixel(7, 7);
    expect_eq(center_px.r, 255);
    expect_eq(center_px.g, 255);

    // 2x scaled at (24, 4) -> [24, 40) x [4, 20), clipped to left half x < 32, y < 14
    const clip = ClipRect{ x0: 24, y0: 4, x1: 32, y1: 14 };
    canvas.drawGlyph8x8ScaledClipped(24.0, 4.0, 2.0, box_glyph[0..8], fg, clip);

    expect_eq(countExactColorPixels(&canvas, 24, 4, 32, 14, fg) > 0, true);
    // Outside clip right & bottom must have 0 painted pixels
    expect_eq(countNonBgPixels(&canvas, 32, 4, 42, 22, bg), 0);
    expect_eq(countNonBgPixels(&canvas, 24, 14, 32, 22, bg), 0);
}

fn testColorEmojiAndClipping()! {
    const bg = CssColor.rgb(20, 20, 30);
    let canvas = Canvas.init(std.heap.page_allocator, 160, 80, bg) catch unreachable;
    defer canvas.deinit();

    // U+2705 ✅: green rounded square (46,184,92) with white check mark (255,255,255)
    canvas.drawColorEmojiClipped(8.0, 8.0, 20.0, 0x2705, 255, null);
    const green = CssColor.rgb(46, 184, 92);
    const white = CssColor.rgb(255, 255, 255);
    expect_eq(countExactColorPixels(&canvas, 8, 8, 28, 28, green) > 50, true);
    expect_eq(countExactColorPixels(&canvas, 8, 8, 28, 28, white) > 10, true);

    // U+274C ❌: red cross mark (229,57,53)
    canvas.drawColorEmojiClipped(36.0, 8.0, 20.0, 0x274C, 255, null);
    const red = CssColor.rgb(229, 57, 53);
    expect_eq(countExactColorPixels(&canvas, 36, 8, 56, 28, red) > 20, true);
    // Top-left exterior corner of cross bounding box remains background
    const corner_px = canvas.getPixel(36, 8);
    expect_eq(corner_px.r, 20);
    expect_eq(corner_px.g, 20);
    expect_eq(corner_px.b, 30);

    // Generic SMP emoji via drawText ("😀" = U+1F600)
    canvas.drawText(64.0, 8.0, "😀", CssColor.rgb(255, 255, 255));
    expect_eq(countNonBgPixels(&canvas, 64, 8, 76, 20, bg) > 20, true);

    // Clipped emoji: draw U+2705 at (8, 40) size 20, clipped to [8, 18) x [40, 50)
    const emoji_clip = ClipRect{ x0: 8, y0: 40, x1: 18, y1: 50 };
    canvas.drawColorEmojiClipped(8.0, 40.0, 20.0, 0x2705, 255, emoji_clip);
    expect_eq(countNonBgPixels(&canvas, 8, 40, 18, 50, bg) > 20, true);
    expect_eq(countNonBgPixels(&canvas, 18, 40, 30, 62, bg), 0);
    expect_eq(countNonBgPixels(&canvas, 8, 50, 18, 62, bg), 0);

    // Edge cases: alpha=0 or size=0 must not paint
    canvas.drawColorEmojiClipped(100.0, 40.0, 20.0, 0x2705, 0, null);
    canvas.drawColorEmojiClipped(100.0, 40.0, 0.0, 0x2705, 255, null);
    expect_eq(countNonBgPixels(&canvas, 100, 40, 120, 60, bg), 0);
}

fn main() {
    test("1. CJK Hanzi, Kana, Hangul & Scaled Text Rasterization", () => {
        testCjkHanziKanaHangulDrawText() catch unreachable;
    });

    test("2. drawGlyph8x8ScaledClipped Scale & ClipRect Boundary", () => {
        testDrawGlyph8x8ScaledAndClipped() catch unreachable;
    });

    test("3. Color Emoji (U+2705, U+274C, SMP) & ClipRect Boundary", () => {
        testColorEmojiAndClipping() catch unreachable;
    });
}
