// test/render/test_rgba_dirty_rect_encode_edge.cp
// CapSurf: White-Box Unit Tests for renderDocumentToRgba & encodeRgbaRectToPng (full frame, dirty rect, out-of-bounds clip, zero-size boundary, PNG roundtrip)

import { test, expect_eq } from "cap:test";
import { Document } from "../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";
import { LayoutTree, Rect } from "../../src/layout/layout.cp";
import { ScreenshotOptions, renderDocumentToRgba, encodeRgbaRectToPng } from "../../src/render/painter.cp";
import { PngCodec } from "../../src/render/png.cp";
import { CssColor } from "cap:css";

fn testRenderDocumentToRgbaAndDirtyRectPng()! {
    const html = "<!DOCTYPE html><html><body style=\"margin: 0;\"><div style=\"width: 20px; height: 20px; background-color: rgb(255, 0, 0);\"></div><div style=\"width: 20px; height: 20px; background-color: rgb(0, 255, 0);\"></div></body></html>";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let layout = LayoutTree.init(std.heap.page_allocator, 40.0, 40.0)?;
    defer layout.deinit();
    layout.computeLayout(&doc, styles)?;

    let rects = std.heap.page_allocator.alloc(Rect, doc.nodeCount())?;
    defer std.heap.page_allocator.free(rects);
    let nid: usize = 0;
    while (nid < doc.nodeCount()) : (nid += 1) {
        rects[nid] = layout.getBoundingClientRect(@as(u32, nid));
    }

    const opts = ScreenshotOptions{
        viewport_w: 40,
        viewport_h: 40,
        compress_level: 1,
        clear_color: CssColor.rgb(255, 255, 255),
    };

    const rgba = renderDocumentToRgba(std.heap.page_allocator, &doc, styles, rects, opts)?;
    defer std.heap.page_allocator.free(rgba);
    expect_eq(rgba.len, @as(usize, 40 * 40 * 4));

    // Verify raw RGBA buffer pixels directly: (5,5)=red, (5,25)=green, (30,30)=white
    const idx_red: usize = (5 * 40 + 5) * 4;
    expect_eq(rgba[idx_red], @as(u8, 255));
    expect_eq(rgba[idx_red + 1], @as(u8, 0));
    expect_eq(rgba[idx_red + 2], @as(u8, 0));
    expect_eq(rgba[idx_red + 3], @as(u8, 255));

    const idx_green: usize = (25 * 40 + 5) * 4;
    expect_eq(rgba[idx_green], @as(u8, 0));
    expect_eq(rgba[idx_green + 1], @as(u8, 255));
    expect_eq(rgba[idx_green + 2], @as(u8, 0));
    expect_eq(rgba[idx_green + 3], @as(u8, 255));

    // 1. Full-frame PNG encode & PngCodec.decode roundtrip
    const full_png = encodeRgbaRectToPng(std.heap.page_allocator, rgba, 40, 40, 0, 0, 40, 40, 1)?;
    defer std.heap.page_allocator.free(full_png);

    if (full_img := PngCodec.decode(full_png)) {
        expect_eq(full_img.width, @as(u32, 40));
        expect_eq(full_img.height, @as(u32, 40));
        const p_red = full_img.getPixel(5, 5);
        expect_eq(p_red.r, @as(u8, 255));
        expect_eq(p_red.g, @as(u8, 0));
        expect_eq(p_red.b, @as(u8, 0));
        expect_eq(p_red.a, @as(u8, 255));

        const p_green = full_img.getPixel(5, 25);
        expect_eq(p_green.r, @as(u8, 0));
        expect_eq(p_green.g, @as(u8, 255));
        expect_eq(p_green.b, @as(u8, 0));
        expect_eq(p_green.a, @as(u8, 255));

        const p_white = full_img.getPixel(30, 30);
        expect_eq(p_white.r, @as(u8, 255));
        expect_eq(p_white.g, @as(u8, 255));
        expect_eq(p_white.b, @as(u8, 255));
        expect_eq(p_white.a, @as(u8, 255));
    } else {
        expect_eq(false, true);
    }

    // 2. Dirty-rect PNG encode (10, 15, 20x20) spanning red, green, and white regions
    const dirty_png = encodeRgbaRectToPng(std.heap.page_allocator, rgba, 40, 40, 10, 15, 20, 20, 1)?;
    defer std.heap.page_allocator.free(dirty_png);

    if (dirty_img := PngCodec.decode(dirty_png)) {
        expect_eq(dirty_img.width, @as(u32, 20));
        expect_eq(dirty_img.height, @as(u32, 20));

        // Local (2, 2) -> full (12, 17): red
        const d_red = dirty_img.getPixel(2, 2);
        expect_eq(d_red.r, @as(u8, 255));
        expect_eq(d_red.g, @as(u8, 0));
        expect_eq(d_red.b, @as(u8, 0));
        expect_eq(d_red.a, @as(u8, 255));

        // Local (2, 10) -> full (12, 25): green
        const d_green = dirty_img.getPixel(2, 10);
        expect_eq(d_green.r, @as(u8, 0));
        expect_eq(d_green.g, @as(u8, 255));
        expect_eq(d_green.b, @as(u8, 0));
        expect_eq(d_green.a, @as(u8, 255));

        // Local (15, 10) -> full (25, 25): white background
        const d_white = dirty_img.getPixel(15, 10);
        expect_eq(d_white.r, @as(u8, 255));
        expect_eq(d_white.g, @as(u8, 255));
        expect_eq(d_white.b, @as(u8, 255));
        expect_eq(d_white.a, @as(u8, 255));
    } else {
        expect_eq(false, true);
    }
}

fn testOutOfBoundsDirtyRectClipping()! {
    // 4x4 distinct RGBA buffer: pixel(x, y) = (x*40+10, y*40+20, (x+y)*20+30, 255)
    let rgba: [64]u8 = [0; 64];
    let y: u32 = 0;
    while (y < 4) : (y += 1) {
        let x: u32 = 0;
        while (x < 4) : (x += 1) {
            const idx = (@as(usize, y) * 4 + @as(usize, x)) * 4;
            rgba[idx] = @as(u8, x * 40 + 10);
            rgba[idx + 1] = @as(u8, y * 40 + 20);
            rgba[idx + 2] = @as(u8, (x + y) * 20 + 30);
            rgba[idx + 3] = 255;
        }
    }

    // A. Negative origin (-2, -1) with size 5x4 -> out-of-bounds pixels stay (0,0,0,0)
    const png_neg = encodeRgbaRectToPng(std.heap.page_allocator, rgba[0..64], 4, 4, -2, -1, 5, 4, 1)?;
    defer std.heap.page_allocator.free(png_neg);

    if (img_neg := PngCodec.decode(png_neg)) {
        expect_eq(img_neg.width, @as(u32, 5));
        expect_eq(img_neg.height, @as(u32, 4));

        // (0, 0) -> (-2, -1) out-of-bounds: transparent black
        const oob0 = img_neg.getPixel(0, 0);
        expect_eq(oob0.r, @as(u8, 0));
        expect_eq(oob0.g, @as(u8, 0));
        expect_eq(oob0.b, @as(u8, 0));
        expect_eq(oob0.a, @as(u8, 0));

        // (1, 1) -> (-1, 0) out-of-bounds: transparent black
        const oob1 = img_neg.getPixel(1, 1);
        expect_eq(oob1.a, @as(u8, 0));

        // (2, 1) -> (0, 0) valid
        const p00 = img_neg.getPixel(2, 1);
        expect_eq(p00.r, @as(u8, 10));
        expect_eq(p00.g, @as(u8, 20));
        expect_eq(p00.b, @as(u8, 30));
        expect_eq(p00.a, @as(u8, 255));

        // (4, 3) -> (2, 2) valid
        const p22 = img_neg.getPixel(4, 3);
        expect_eq(p22.r, @as(u8, 90));
        expect_eq(p22.g, @as(u8, 100));
        expect_eq(p22.b, @as(u8, 110));
        expect_eq(p22.a, @as(u8, 255));
    } else {
        expect_eq(false, true);
    }

    // B. Bottom-right overflow (2, 2, 4x4) on 4x4 buffer
    const png_overflow = encodeRgbaRectToPng(std.heap.page_allocator, rgba[0..64], 4, 4, 2, 2, 4, 4, 1)?;
    defer std.heap.page_allocator.free(png_overflow);

    if (img_ov := PngCodec.decode(png_overflow)) {
        expect_eq(img_ov.width, @as(u32, 4));
        expect_eq(img_ov.height, @as(u32, 4));

        // (1, 1) -> (3, 3) valid
        const p33 = img_ov.getPixel(1, 1);
        expect_eq(p33.r, @as(u8, 130));
        expect_eq(p33.g, @as(u8, 140));
        expect_eq(p33.b, @as(u8, 150));
        expect_eq(p33.a, @as(u8, 255));

        // (2, 0) -> (4, 2) out-of-bounds
        const oob_r = img_ov.getPixel(2, 0);
        expect_eq(oob_r.a, @as(u8, 0));

        // (0, 2) -> (2, 4) out-of-bounds
        const oob_b = img_ov.getPixel(0, 2);
        expect_eq(oob_b.a, @as(u8, 0));
    } else {
        expect_eq(false, true);
    }

    // C. Completely out-of-bounds rect (10, 10, 3x3) -> all transparent black
    const png_far = encodeRgbaRectToPng(std.heap.page_allocator, rgba[0..64], 4, 4, 10, 10, 3, 3, 1)?;
    defer std.heap.page_allocator.free(png_far);

    if (img_far := PngCodec.decode(png_far)) {
        expect_eq(img_far.width, @as(u32, 3));
        expect_eq(img_far.height, @as(u32, 3));
        expect_eq(img_far.getPixel(0, 0).a, @as(u8, 0));
        expect_eq(img_far.getPixel(2, 2).a, @as(u8, 0));
    } else {
        expect_eq(false, true);
    }
}

fn testZeroWidthHeightAndEmptyBufferBoundary()! {
    let rgba: [16]u8 = [
        11, 22, 33, 255,
        44, 55, 66, 255,
        77, 88, 99, 255,
        111, 122, 133, 255,
    ];

    // 1. clip_w == 0 triggers full-frame fallback (2x2)
    const png_zw = encodeRgbaRectToPng(std.heap.page_allocator, rgba[0..16], 2, 2, 1, 1, 0, 2, 1)?;
    defer std.heap.page_allocator.free(png_zw);
    if (img_zw := PngCodec.decode(png_zw)) {
        expect_eq(img_zw.width, @as(u32, 2));
        expect_eq(img_zw.height, @as(u32, 2));
        const p00 = img_zw.getPixel(0, 0);
        expect_eq(p00.r, @as(u8, 11));
        expect_eq(p00.g, @as(u8, 22));
        expect_eq(p00.b, @as(u8, 33));
        expect_eq(p00.a, @as(u8, 255));
    } else {
        expect_eq(false, true);
    }

    // 2. clip_h == 0 triggers full-frame fallback (2x2)
    const png_zh = encodeRgbaRectToPng(std.heap.page_allocator, rgba[0..16], 2, 2, 1, 1, 2, 0, 1)?;
    defer std.heap.page_allocator.free(png_zh);
    if (img_zh := PngCodec.decode(png_zh)) {
        expect_eq(img_zh.width, @as(u32, 2));
        expect_eq(img_zh.height, @as(u32, 2));
        const p11 = img_zh.getPixel(1, 1);
        expect_eq(p11.r, @as(u8, 111));
        expect_eq(p11.g, @as(u8, 122));
        expect_eq(p11.b, @as(u8, 133));
        expect_eq(p11.a, @as(u8, 255));
    } else {
        expect_eq(false, true);
    }

    // 3. full_w == 0 or short rgba returns error.EncodeFailed
    let failed_zero_full = false;
    * = encodeRgbaRectToPng(std.heap.page_allocator, rgba[0..0], 0, 0, 0, 0, 0, 0, 1) catch |e| {
        * = e;
        failed_zero_full = true;
    };
    expect_eq(failed_zero_full, true);

    let failed_short_buf = false;
    * = encodeRgbaRectToPng(std.heap.page_allocator, rgba[0..4], 2, 2, 0, 0, 2, 2, 1) catch |e| {
        * = e;
        failed_short_buf = true;
    };
    expect_eq(failed_short_buf, true);
}

fn main() {
    test("1. renderDocumentToRgba & encodeRgbaRectToPng full-frame and dirty-rect PNG roundtrip", () => {
        testRenderDocumentToRgbaAndDirtyRectPng() catch unreachable;
    });
    test("2. encodeRgbaRectToPng negative/overflow/disjoint out-of-bounds clipping", () => {
        testOutOfBoundsDirtyRectClipping() catch unreachable;
    });
    test("3. encodeRgbaRectToPng zero-width/zero-height fallback and empty buffer error", () => {
        testZeroWidthHeightAndEmptyBufferBoundary() catch unreachable;
    });
}
