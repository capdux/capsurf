// test/render/test_rasterizer_and_clip_edge.cp
// CapSurf: Pure CP White-Box Unit Tests for 2D Rasterizer, ClipRect, Alpha Blending,
// Rounded Corners / Borders, and Text / Pixel Buffer Edge Boundaries.

import { test, expect_eq } from "cap:test";
import { Rect } from "../../../src/layout/layout.cp";
import { Canvas, ClipRect } from "../../../src/render/painter.cp";
import { ResolvedPaintStyle } from "../../../src/css/cascade.cp";
import { CssColor } from "cap:css";

fn testZeroDimAndInvalidRects()! {
    const bg = CssColor.rgba(10, 20, 30, 255);
    const red = CssColor.rgba(255, 0, 0, 255);

    // 1. Zero-dimension canvas must not crash on any operation
    let zero_canvas = Canvas.init(std.heap.page_allocator, 0, 0, bg)?;
    defer zero_canvas.deinit();
    expect_eq(zero_canvas.pixels.len, 0);

    const oob_px = zero_canvas.getPixel(0, 0);
    expect_eq(oob_px.r, 0);
    expect_eq(oob_px.a, 0);

    zero_canvas.blendPixel(0, 0, red);
    zero_canvas.fillRectClipped(0.0, 0.0, 10.0, 10.0, red, null);
    let empty_style = ResolvedPaintStyle{};
    empty_style.bg_color = red;
    zero_canvas.paintBox(Rect{ x: 0.0, y: 0.0, width: 10.0, height: 10.0, visible: true }, empty_style);
    const src_1x1: [4]u8 = [ 255, 0, 0, 255 ];
    zero_canvas.blitRgba(0, 0, src_1x1[0..4], 1, 1);
    zero_canvas.blitScaledRgba(0, 0, 2, 2, src_1x1[0..4], 1, 1);

    // 2. Valid 16x16 canvas with zero/negative-size/invisible rects must be no-ops
    let c = Canvas.init(std.heap.page_allocator, 16, 16, bg)?;
    defer c.deinit();

    c.paintBox(Rect{ x: 2.0, y: 2.0, width: 0.0, height: 8.0, visible: true }, empty_style);
    c.paintBox(Rect{ x: 2.0, y: 2.0, width: 8.0, height: 0.0, visible: true }, empty_style);
    c.paintBox(Rect{ x: 2.0, y: 2.0, width: -5.0, height: 8.0, visible: true }, empty_style);
    c.paintBox(Rect{ x: 2.0, y: 2.0, width: 8.0, height: 8.0, visible: false }, empty_style);
    c.fillRectClipped(2.0, 2.0, 0.0, 5.0, red, null);
    c.fillRectClipped(2.0, 2.0, 5.0, -3.0, red, null);
    c.fillRectClipped(2.0, 2.0, 5.0, 5.0, CssColor.rgba(255, 0, 0, 0), null);
    c.blitScaledRgba(0, 0, 0, 4, src_1x1[0..4], 1, 1);
    c.blitScaledRgba(0, 0, 4, 0, src_1x1[0..4], 1, 1);
    c.blitScaledRgba(0, 0, 4, 4, src_1x1[0..4], 0, 1);
    c.blitScaledRgba(0, 0, 4, 4, src_1x1[0..4], 1, 0);

    const p = c.getPixel(3, 3);
    expect_eq(p.r, 10);
    expect_eq(p.g, 20);
    expect_eq(p.b, 30);
    expect_eq(p.a, 255);
}

fn testNegativeAndViewportBoundsClipping()! {
    const white = CssColor.rgba(255, 255, 255, 255);
    const blue = CssColor.rgba(0, 0, 255, 255);
    let c = Canvas.init(std.heap.page_allocator, 20, 20, white)?;
    defer c.deinit();

    // Out-of-bounds getPixel returns transparent black
    expect_eq(c.getPixel(-1, 0).a, 0);
    expect_eq(c.getPixel(0, -1).a, 0);
    expect_eq(c.getPixel(20, 0).a, 0);
    expect_eq(c.getPixel(0, 20).a, 0);

    // Out-of-bounds blendPixel is ignored safely
    c.blendPixel(-1, 5, blue);
    c.blendPixel(20, 5, blue);
    c.blendPixel(5, -1, blue);
    c.blendPixel(5, 20, blue);

    // Partially off top-left edge: (-5, -5) size 10x10 -> covers (0..4, 0..4)
    c.fillRectClipped(-5.0, -5.0, 10.0, 10.0, blue, null);
    expect_eq(c.getPixel(0, 0).b, 255);
    expect_eq(c.getPixel(0, 0).r, 0);
    expect_eq(c.getPixel(4, 4).b, 255);
    expect_eq(c.getPixel(5, 5).r, 255);

    // Partially off bottom-right edge: (16, 16) size 10x10 -> covers (16..19, 16..19)
    let box_style = ResolvedPaintStyle{};
    box_style.bg_color = CssColor.rgba(0, 200, 0, 255);
    c.paintBoxClipped(Rect{ x: 16.0, y: 16.0, width: 10.0, height: 10.0, visible: true }, box_style, null);
    expect_eq(c.getPixel(16, 16).g, 200);
    expect_eq(c.getPixel(19, 19).g, 200);
    expect_eq(c.getPixel(15, 16).r, 255);

    // Completely off-screen rects leave interior untouched
    c.fillRectClipped(-20.0, -20.0, 10.0, 10.0, blue, null);
    c.fillRectClipped(25.0, 25.0, 10.0, 10.0, blue, null);
    expect_eq(c.getPixel(10, 10).r, 255);
    expect_eq(c.getPixel(10, 10).g, 255);
    expect_eq(c.getPixel(10, 10).b, 255);
}

fn testClipRectIntersectAndEmptyClip()! {
    const r1 = ClipRect.fromRect(Rect{ x: 2.3, y: 3.7, width: 5.2, height: 4.1, visible: true });
    expect_eq(r1.x0, 2);
    expect_eq(r1.y0, 3);
    expect_eq(r1.x1, 8);
    expect_eq(r1.y1, 8);

    const r2 = ClipRect{ x0: 4, y0: 5, x1: 12, y1: 10 };
    const inter = r1.intersect(r2);
    expect_eq(inter.x0, 4);
    expect_eq(inter.y0, 5);
    expect_eq(inter.x1, 8);
    expect_eq(inter.y1, 8);

    const disjoint = ClipRect{ x0: 10, y0: 10, x1: 15, y1: 15 };
    const empty_inter = r1.intersect(disjoint);
    expect_eq(empty_inter.x0 >= empty_inter.x1, true);

    const white = CssColor.rgba(255, 255, 255, 255);
    const red = CssColor.rgba(255, 0, 0, 255);
    let c = Canvas.init(std.heap.page_allocator, 16, 16, white)?;
    defer c.deinit();

    // Empty/inverted clip prevents any drawing
    c.fillRectClipped(0.0, 0.0, 16.0, 16.0, red, empty_inter);
    expect_eq(c.getPixel(5, 5).g, 255);

    // Tight clip restricts full-canvas fill to (4..7, 5..7)
    c.fillRectClipped(0.0, 0.0, 16.0, 16.0, red, inter);
    expect_eq(c.getPixel(4, 5).r, 255);
    expect_eq(c.getPixel(4, 5).g, 0);
    expect_eq(c.getPixel(7, 7).g, 0);
    expect_eq(c.getPixel(3, 5).g, 255);
    expect_eq(c.getPixel(8, 5).g, 255);
    expect_eq(c.getPixel(4, 4).g, 255);
    expect_eq(c.getPixel(4, 8).g, 255);
}

fn testAlphaBlendingEdgeCases()! {
    const white = CssColor.rgba(255, 255, 255, 255);
    let c = Canvas.init(std.heap.page_allocator, 8, 8, white)?;
    defer c.deinit();

    // Alpha 0 must not modify pixel
    c.blendPixel(1, 1, CssColor.rgba(12, 34, 56, 0));
    expect_eq(c.getPixel(1, 1).r, 255);
    expect_eq(c.getPixel(1, 1).g, 255);
    expect_eq(c.getPixel(1, 1).b, 255);

    // Alpha 255 overwrites pixel
    c.blendPixel(2, 2, CssColor.rgba(12, 34, 56, 255));
    expect_eq(c.getPixel(2, 2).r, 12);
    expect_eq(c.getPixel(2, 2).g, 34);
    expect_eq(c.getPixel(2, 2).b, 56);
    expect_eq(c.getPixel(2, 2).a, 255);

    // Alpha 128 over white: (255, 0, 0, 128) -> g = (0*128 + 255*127)/255 = 127
    c.blendPixel(3, 3, CssColor.rgba(255, 0, 0, 128));
    expect_eq(c.getPixel(3, 3).r, 255);
    expect_eq(c.getPixel(3, 3).g, 127);
    expect_eq(c.getPixel(3, 3).b, 127);
    expect_eq(c.getPixel(3, 3).a, 255);

    // Alpha 128 over black: (200, 100, 50, 128) -> r=100, g=50, b=25
    c.blendPixel(4, 4, CssColor.rgba(0, 0, 0, 255));
    c.blendPixel(4, 4, CssColor.rgba(200, 100, 50, 128));
    expect_eq(c.getPixel(4, 4).r, 100);
    expect_eq(c.getPixel(4, 4).g, 50);
    expect_eq(c.getPixel(4, 4).b, 25);
}

fn testRoundedCornersBordersAndOutline()! {
    const white = CssColor.rgba(255, 255, 255, 255);
    const dark = CssColor.rgba(20, 20, 20, 255);

    // 1. Oversized uniform border-radius is clamped to half box dimensions
    let c1 = Canvas.init(std.heap.page_allocator, 20, 20, white)?;
    defer c1.deinit();
    let round_style = ResolvedPaintStyle{};
    round_style.bg_color = dark;
    round_style.border_radius = 100.0;
    c1.paintBox(Rect{ x: 0.0, y: 0.0, width: 20.0, height: 20.0, visible: true }, round_style);
    expect_eq(c1.getPixel(0, 0).r, 255);
    expect_eq(c1.getPixel(19, 0).r, 255);
    expect_eq(c1.getPixel(0, 19).r, 255);
    expect_eq(c1.getPixel(19, 19).r, 255);
    expect_eq(c1.getPixel(10, 10).r, 20);

    // 2. Non-uniform per-corner radius (only top-left rounded)
    let c2 = Canvas.init(std.heap.page_allocator, 20, 20, white)?;
    defer c2.deinit();
    let tl_style = ResolvedPaintStyle{};
    tl_style.bg_color = dark;
    tl_style.border_tl_radius = 12.0;
    c2.paintBox(Rect{ x: 0.0, y: 0.0, width: 20.0, height: 20.0, visible: true }, tl_style);
    expect_eq(c2.getPixel(0, 0).r, 255);
    expect_eq(c2.getPixel(19, 0).r, 20);
    expect_eq(c2.getPixel(0, 19).r, 20);
    expect_eq(c2.getPixel(19, 19).r, 20);

    // 3. Per-side borders + outline with ClipRect
    let c3 = Canvas.init(std.heap.page_allocator, 30, 30, white)?;
    defer c3.deinit();
    let b_style = ResolvedPaintStyle{};
    b_style.bg_color = white;
    b_style.border_top_width = 2.0;
    b_style.border_right_width = 2.0;
    b_style.border_bottom_width = 2.0;
    b_style.border_left_width = 2.0;
    b_style.border_top_color = CssColor.rgba(255, 0, 0, 255);
    b_style.border_right_color = CssColor.rgba(0, 255, 0, 255);
    b_style.border_bottom_color = CssColor.rgba(0, 0, 255, 255);
    b_style.border_left_color = CssColor.rgba(255, 255, 0, 255);
    b_style.outline_width = 2.0;
    b_style.outline_offset = 1.0;
    b_style.outline_color = CssColor.rgba(128, 0, 128, 255);

    const box_r = Rect{ x: 5.0, y: 5.0, width: 20.0, height: 20.0, visible: true };
    c3.paintBox(box_r, b_style);
    expect_eq(c3.getPixel(15, 5).r, 255);
    expect_eq(c3.getPixel(15, 5).g, 0);
    expect_eq(c3.getPixel(24, 15).g, 255);
    expect_eq(c3.getPixel(24, 15).r, 0);
    expect_eq(c3.getPixel(15, 24).b, 255);
    expect_eq(c3.getPixel(15, 24).r, 0);
    expect_eq(c3.getPixel(5, 15).r, 255);
    expect_eq(c3.getPixel(5, 15).g, 255);
    expect_eq(c3.getPixel(5, 15).b, 0);
    expect_eq(c3.getPixel(15, 15).r, 255);

    const outline_clip = ClipRect{ x0: 2, y0: 2, x1: 15, y1: 15 };
    c3.paintOutlineClipped(box_r, b_style, outline_clip);
    expect_eq(c3.getPixel(10, 2).r, 128);
    expect_eq(c3.getPixel(10, 2).b, 128);
    expect_eq(c3.getPixel(20, 2).r, 255);
    expect_eq(c3.getPixel(20, 2).g, 255);
}

fn testTextAndPixelBufferBoundaries()! {
    const white = CssColor.rgba(255, 255, 255, 255);
    const black = CssColor.rgba(0, 0, 0, 255);
    let c = Canvas.init(std.heap.page_allocator, 16, 16, white)?;
    defer c.deinit();

    // Text edge cases: empty string, zero alpha, zero scale, negative coordinates + clip
    c.drawTextClipped(2.0, 2.0, "", black, null);
    c.drawTextClipped(2.0, 2.0, "A", CssColor.rgba(0, 0, 0, 0), null);
    c.drawScaledTextClipped(2.0, 2.0, "A", 0.0, black, null);
    c.drawScaledTextClipped(2.0, 2.0, "A", -4.0, black, null);
    expect_eq(c.getPixel(4, 4).r, 255);

    // Draw text partially off-screen with a clip restricting to x >= 4
    const text_clip = ClipRect{ x0: 4, y0: 0, x1: 12, y1: 16 };
    c.drawTextClipped(-2.0, 2.0, "Hi", black, text_clip);
    expect_eq(c.getPixel(2, 5).r, 255);

    // Truncated pixel buffer in blitRgba must stop safely at buffer end
    const short_buf: [8]u8 = [ 255, 0, 0, 255, 0, 255, 0, 128 ];
    c.blitRgba(0, 0, short_buf[0..8], 4, 4);
    expect_eq(c.getPixel(0, 0).r, 255);
    expect_eq(c.getPixel(0, 0).g, 0);
    expect_eq(c.getPixel(1, 0).r, 127);
    expect_eq(c.getPixel(1, 0).g, 255);
    expect_eq(c.getPixel(2, 0).r, 255);

    // blitScaledRgbaClipped with negative origin & ClipRect
    const src_2x2: [16]u8 = [
        0, 0, 255, 255, 0, 0, 255, 255,
        0, 0, 255, 0,   0, 0, 255, 255,
    ];
    const blit_clip = ClipRect{ x0: 2, y0: 2, x1: 4, y1: 4 };
    c.blitScaledRgbaClipped(-2, -2, 8, 8, src_2x2[0..16], 2, 2, blit_clip);
    expect_eq(c.getPixel(2, 2).b, 255);
    expect_eq(c.getPixel(2, 2).r, 0);
    expect_eq(c.getPixel(4, 2).r, 255);
}

fn main() {
    test("1. zero-dimension canvas & zero/negative/invisible rect no-ops", () => {
        testZeroDimAndInvalidRects() catch unreachable;
    });

    test("2. negative coordinates & viewport out-of-bounds clipping", () => {
        testNegativeAndViewportBoundsClipping() catch unreachable;
    });

    test("3. ClipRect.fromRect, intersect, and empty clip guard", () => {
        testClipRectIntersectAndEmptyClip() catch unreachable;
    });

    test("4. blendPixel alpha=0, alpha=255, and semi-transparent blending", () => {
        testAlphaBlendingEdgeCases() catch unreachable;
    });

    test("5. oversized/non-uniform border-radius, per-side borders & clipped outline", () => {
        testRoundedCornersBordersAndOutline() catch unreachable;
    });

    test("6. text empty/zero-size/negative-clip & truncated/clipped RGBA blit", () => {
        testTextAndPixelBufferBoundaries() catch unreachable;
    });
}
