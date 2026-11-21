// test/render/test_screenshot.cp
// CapSurf Milestone 5: 2D Software Rasterizer & PNG Screenshot Test Suite

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle, ResolvedPaintStyle } from "../../../src/css/cascade.cp";
import { LayoutTree, Rect } from "../../../src/layout/layout.cp";
import { Canvas, ScreenshotOptions, renderDocumentToPng, encodeCanvasToPngWithLevel } from "../../../src/render/painter.cp";
import { PngCodec, encodePng } from "../../../src/render/png.cp";
import { CssColor } from "cap:css";

fn testCanvasRasterization()! {
    // Initialize 400x300 canvas with white background
    let canvas = Canvas.init(std.heap.page_allocator, 400, 300, CssColor.rgb(255, 255, 255)) catch unreachable;
    defer canvas.deinit();

    // Paint a blue button with 8px radius and dark-blue border (x: 50, y: 40, w: 120, h: 44)
    const btn_rect = Rect{ x: 50.0, y: 40.0, width: 120.0, height: 44.0, visible: true };
    const btn_style = ResolvedPaintStyle{
        bg_color: CssColor.rgba(37, 99, 235, 255),
        border_color: CssColor.rgba(29, 78, 216, 255),
        text_color: CssColor.rgb(255, 255, 255),
        border_width: 2.0,
        border_radius: 8.0,
        font_size_px: 16.0,
    };
    canvas.paintBox(btn_rect, btn_style);
    canvas.drawText(66.0, 56.0, "ABC", btn_style.text_color);

    // Verify button center pixel (x: 100, y: 62) is painted blue (R=37, G=99, B=235)
    const center_px = canvas.getPixel(100, 62);
    expect_eq(center_px.r, 37);
    expect_eq(center_px.g, 99);
    expect_eq(center_px.b, 235);
    expect_eq(center_px.a, 255);

    // Verify canvas exterior (x: 10, y: 10) remains background white
    const bg_px = canvas.getPixel(10, 10);
    expect_eq(bg_px.r, 255);
    expect_eq(bg_px.g, 255);
    expect_eq(bg_px.b, 255);
}

fn testPngCompressionLevels()! {
    let canvas = Canvas.init(std.heap.page_allocator, 200, 150, CssColor.rgb(37, 99, 235)) catch unreachable;
    defer canvas.deinit();

    // Level 0: Raw uncompressed store
    const png_lvl0 = encodeCanvasToPngWithLevel(std.heap.page_allocator, &canvas, 0) catch unreachable;
    defer std.heap.page_allocator.free(png_lvl0);

    // Level 1: Fast RLE-grade compression
    const png_lvl1 = encodeCanvasToPngWithLevel(std.heap.page_allocator, &canvas, 1) catch unreachable;
    defer std.heap.page_allocator.free(png_lvl1);

    // Level 6: Default balanced compression
    const png_lvl6 = encodeCanvasToPngWithLevel(std.heap.page_allocator, &canvas, 6) catch unreachable;
    defer std.heap.page_allocator.free(png_lvl6);

    // Level 9: Maximum compression
    const png_lvl9 = encodeCanvasToPngWithLevel(std.heap.page_allocator, &canvas, 9) catch unreachable;
    defer std.heap.page_allocator.free(png_lvl9);

    // Verify PNG magic bytes: \x89PNG\r\n\x1a\n
    expect_eq(png_lvl0.len > 64, true);
    expect_eq(png_lvl0[0], 0x89);
    expect_eq(png_lvl0[1], 'P');
    expect_eq(png_lvl0[2], 'N');
    expect_eq(png_lvl0[3], 'G');

    // Compression efficacy: Sub-filtered compressed size must be vastly smaller than raw scanlines (> 120KB)
    expect_eq(png_lvl1.len < png_lvl0.len, true);
    expect_eq(png_lvl6.len <= png_lvl1.len, true);
    expect_eq(png_lvl9.len <= png_lvl6.len, true);

    // Round-trip decoding verification
    if (decoded := PngCodec.decode(png_lvl6)) {
        expect_eq(decoded.width, 200);
        expect_eq(decoded.height, 150);
        // Center pixel must match original blue color
        const dec_px = decoded.getPixel(100, 75);
        expect_eq(dec_px.r, 37);
        expect_eq(dec_px.g, 99);
        expect_eq(dec_px.b, 235);
        expect_eq(dec_px.a, 255);
    } else {
        expect_eq(false, true);
    }
}

fn testEndToEndScreenshot()! {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const html = "<!DOCTYPE html><html><body><div id=\"card\" style=\"width: 200px; height: 100px; background-color: #ef4444;\"><button style=\"width: 80px; height: 32px; background-color: #22c55e;\">Go</button></div></body></html>";
    doc.parseHtml(html) catch unreachable;

    // Compute Styles
    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount()) catch unreachable;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    // Compute Layout
    let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
    defer layout.deinit();
    layout.computeLayout(&doc, styles) catch unreachable;

    let rects = std.heap.page_allocator.alloc(Rect, doc.nodeCount()) catch unreachable;
    defer std.heap.page_allocator.free(rects);
    let nid: usize = 0;
    while (nid < doc.nodeCount()) : (nid += 1) {
        rects[nid] = layout.getBoundingClientRect(@as(u32, nid));
    }

    // Screenshot render with fast level 1 compression
    let opts = ScreenshotOptions{
        viewport_w: 800,
        viewport_h: 600,
        compress_level: 1,
        clear_color: CssColor.rgb(240, 240, 240),
    };

    const png_data = renderDocumentToPng(std.heap.page_allocator, &doc, styles, rects, opts) catch unreachable;
    defer std.heap.page_allocator.free(png_data);

    // Validate output PNG
    expect_eq(png_data.len > 100, true);
    expect_eq(png_data[0], 0x89);
    expect_eq(png_data[1], 'P');
    expect_eq(png_data[2], 'N');
    expect_eq(png_data[3], 'G');

    // Decode screenshot and verify dimensions
    if (img := PngCodec.decode(png_data)) {
        expect_eq(img.width, 800);
        expect_eq(img.height, 600);
    } else {
        expect_eq(false, true);
    }
}

fn main() {
    test("1. Canvas 2D Software Rasterization (SDF Rounded Box & Text)", () => {
        testCanvasRasterization() catch unreachable;
    });

    test("2. PNG Compression Levels (0, 1, 6, 9) & Round-Trip Decode", () => {
        testPngCompressionLevels() catch unreachable;
    });

    test("3. End-to-End DOM -> Cascade -> Layout -> PNG Screenshot", () => {
        testEndToEndScreenshot() catch unreachable;
    });
}
