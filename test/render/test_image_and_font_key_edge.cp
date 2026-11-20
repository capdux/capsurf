// test/render/test_image_and_font_key_edge.cp
// CapSurf: Pure CP White-Box Unit Tests for loadBgPngBytes, /font-key-images.html,
// and font-key-red.png / font-key-orange.png mapping & rasterization boundaries.

import { test, expect_eq } from "cap:test";
import { Document } from "../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";
import { LayoutTree, Rect } from "../../src/layout/layout.cp";
import { ScreenshotOptions, renderDocumentToPng, loadBgPngBytes } from "../../src/render/painter.cp";
import { PngCodec } from "../../src/render/png.cp";
import { CssColor } from "cap:css";

// 1x1 transparent PNG in base64 (68 bytes decoded)
const TINY_1X1_PNG_B64 = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=";

fn testLoadBgPngBytesDataUriAndGuards()! {
    let buf: [4096]u8 = [0; 4096];

    // Empty and whitespace-only inputs
    expect_eq(loadBgPngBytes("", buf[0..]) == null, true);
    expect_eq(loadBgPngBytes("   \t\r\n  ", buf[0..]) == null, true);

    // Valid data:image/png;base64,... with leading/trailing whitespace
    const valid_res = loadBgPngBytes("  " ++ TINY_1X1_PNG_B64 ++ " \n", buf[0..]);
    expect_eq(valid_res != null, true);
    if (bytes := valid_res) {
        expect_eq(bytes.len > 32, true);
        expect_eq(bytes[0], 0x89);
        expect_eq(bytes[1], 'P');
        expect_eq(bytes[2], 'N');
        expect_eq(bytes[3], 'G');
    }

    // Empty base64 payload & invalid base64 payload
    expect_eq(loadBgPngBytes("data:image/png;base64,", buf[0..]) == null, true);
    expect_eq(loadBgPngBytes("data:image/png;base64,!!!invalid!!!", buf[0..]) == null, true);

    // Buffer too small for decoded PNG
    let tiny_buf: [8]u8 = [0; 8];
    expect_eq(loadBgPngBytes(TINY_1X1_PNG_B64, tiny_buf[0..]) == null, true);
}

fn expectMapsTo(src: []const u8, expected: []const u8, out_buf: []u8)! {
    const res = loadBgPngBytes(src, out_buf);
    expect_eq(res != null, true);
    if (bytes := res) {
        expect_eq(std.mem.eql(u8, bytes, expected), true);
    }
}

fn testFontKeyAndFixtureMappingBoundaries()! {
    let ref_red_buf: [4096]u8 = [0; 4096];
    let ref_tiny_buf: [4096]u8 = [0; 4096];
    let out_buf: [4096]u8 = [0; 4096];

    const solid_red_opt = loadBgPngBytes("/__fixtures/solid-red.png", ref_red_buf[0..]);
    expect_eq(solid_red_opt != null, true);
    const solid_red = solid_red_opt.?;

    const tiny_opt = loadBgPngBytes("/__fixtures/tiny.png", ref_tiny_buf[0..]);
    expect_eq(tiny_opt != null, true);
    const tiny = tiny_opt.?;

    // font-key-red.png, slow-red.png, slow-headers-red.png all map to solid-red.png
    expectMapsTo("font-key-red.png", solid_red, out_buf[0..])?;
    expectMapsTo("/__fixtures/font-key-red.png", solid_red, out_buf[0..])?;
    expectMapsTo("  https://ks.invalid/__fixtures/font-key-red.png?v=1#frag  ", solid_red, out_buf[0..])?;
    expectMapsTo("slow-red.png", solid_red, out_buf[0..])?;
    expectMapsTo("slow-headers-red.png", solid_red, out_buf[0..])?;

    // font-key-orange.png maps to tiny.png
    expectMapsTo("font-key-orange.png", tiny, out_buf[0..])?;
    expectMapsTo("/__fixtures/font-key-orange.png", tiny, out_buf[0..])?;
    expectMapsTo("https://ks.invalid/__fixtures/font-key-orange.png?k=2#top", tiny, out_buf[0..])?;

    // Query and fragment stripping on generic /__fixtures/*.png
    expectMapsTo("/__fixtures/solid-red.png?cache=bust#section", solid_red, out_buf[0..])?;

    // Non-.png extension or missing fixture returns null
    expect_eq(loadBgPngBytes("/__fixtures/solid-red.jpg", out_buf[0..]) == null, true);
    expect_eq(loadBgPngBytes("/__fixtures/does-not-exist-12345.png", out_buf[0..]) == null, true);
    expect_eq(loadBgPngBytes("https://example.com/other.png", out_buf[0..]) == null, true);
}

fn testFontKeyImagesHtmlRender()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    // Exact markup from MockFixtures /font-key-images.html
    const html =
        "<!doctype html><body style=\"margin:0;background:#fff\">\n" ++
        "<img id=\"a\" src=\"/__fixtures/font-key-red.png\" style=\"display:block;width:100px;height:100px\">\n" ++
        "<img id=\"b\" src=\"/__fixtures/font-key-orange.png\" style=\"display:block;width:100px;height:100px\">\n";
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let layout = LayoutTree.init(std.heap.page_allocator, 120.0, 220.0)?;
    defer layout.deinit();
    layout.computeLayout(&doc, styles)?;

    let rects = std.heap.page_allocator.alloc(Rect, doc.nodeCount())?;
    defer std.heap.page_allocator.free(rects);
    let nid: usize = 0;
    while (nid < doc.nodeCount()) : (nid += 1) {
        rects[nid] = layout.getBoundingClientRect(@as(u32, nid));
    }

    const opts = ScreenshotOptions{
        viewport_w: 120,
        viewport_h: 220,
        clear_color: CssColor.rgb(255, 255, 255),
        compress_level: 1,
    };
    const png_bytes = renderDocumentToPng(std.heap.page_allocator, &doc, styles, rects, opts)?;
    defer std.heap.page_allocator.free(png_bytes);

    expect_eq(png_bytes.len > 64, true);
    if (decoded := PngCodec.decode(png_bytes)) {
        expect_eq(decoded.width, 120);
        expect_eq(decoded.height, 220);

        // #a (0..100, 0..100) uses font-key-red.png -> solid-red.png (255, 0, 0)
        const px_a = decoded.getPixel(50, 50);
        expect_eq(px_a.r, 255);
        expect_eq(px_a.g, 0);
        expect_eq(px_a.b, 0);
        expect_eq(px_a.a, 255);

        // #b (0..100, 100..200) uses font-key-orange.png -> tiny.png (non-white pixel)
        const px_b = decoded.getPixel(50, 150);
        expect_eq(px_b.a, 255);

        // Below both images (y=210) remains white background
        const px_bg = decoded.getPixel(50, 210);
        expect_eq(px_bg.r, 255);
        expect_eq(px_bg.g, 255);
        expect_eq(px_bg.b, 255);
    } else {
        expect_eq(false, true);
    }
}

fn main() {
    test("1. loadBgPngBytes data URI, whitespace, invalid base64 & small buffer guards", () => {
        testLoadBgPngBytesDataUriAndGuards() catch unreachable;
    });

    test("2. font-key-red.png / font-key-orange.png / slow-red.png & /__fixtures/ mapping", () => {
        testFontKeyAndFixtureMappingBoundaries() catch unreachable;
    });

    test("3. /font-key-images.html end-to-end renderDocumentToPng pixel verification", () => {
        testFontKeyImagesHtmlRender() catch unreachable;
    });
}
