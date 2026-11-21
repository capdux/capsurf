// test/render/test_box_shadow_and_clip.cp
// CapSurf: Pure CP White-Box Unit Tests for box-shadow (inset/outset), clip-path: inset(), opacity, and overflow: clip

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../../src/css/cascade.cp";
import { LayoutTree, Rect } from "../../../src/layout/layout.cp";
import { ScreenshotOptions, renderDocumentToPng } from "../../../src/render/painter.cp";
import { PngCodec } from "../../../src/render/png.cp";
import { CssColor } from "cap:css";

fn renderHtmlToPng(html: []const u8, vw: u32, vh: u32): ([]u8)! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let layout = LayoutTree.init(std.heap.page_allocator, @floatFromInt(vw), @floatFromInt(vh))?;
    defer layout.deinit();
    layout.computeLayout(&doc, styles)?;

    let rects = std.heap.page_allocator.alloc(Rect, doc.nodeCount())?;
    defer std.heap.page_allocator.free(rects);
    let nid: usize = 0;
    while (nid < doc.nodeCount()) : (nid += 1) {
        rects[nid] = layout.getBoundingClientRect(@as(u32, nid));
    }

    let opts = ScreenshotOptions{
        viewport_w: vw,
        viewport_h: vh,
        compress_level: 1,
        clear_color: CssColor.rgb(255, 255, 255),
    };
    return renderDocumentToPng(std.heap.page_allocator, &doc, styles, rects, opts);
}

fn testBoxShadowInsetAndOutset()! {
    const html = "<!DOCTYPE html><html><body style=\"margin: 0;\"><div id=\"inset_box\" style=\"width: 100px; height: 80px; background-color: #ffffff; box-shadow: inset 4px 6px 0px 2px #000000;\"></div><div id=\"outset_box\" style=\"width: 100px; height: 80px; background-color: #ef4444; box-shadow: 10px 10px 0px 0px #0000ff;\"></div></body></html>";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    // 1. Verify CSS cascade parsed `box-shadow: inset ...` accurately
    const inset_id = doc.getElementById("inset_box");
    expect_eq(inset_id != NULL_NODE, true);
    const inset_sh = styles[@as(usize, inset_id)].box_shadow;
    expect_eq(inset_sh.has_shadow, true);
    expect_eq(inset_sh.inset, true);
    expect_eq(inset_sh.offset_x == 4.0, true);
    expect_eq(inset_sh.offset_y == 6.0, true);
    expect_eq(inset_sh.spread == 2.0, true);
    expect_eq(inset_sh.color.r, 0);
    expect_eq(inset_sh.color.g, 0);
    expect_eq(inset_sh.color.b, 0);
    expect_eq(inset_sh.color.a, 255);

    const outset_id = doc.getElementById("outset_box");
    expect_eq(outset_id != NULL_NODE, true);
    const outset_sh = styles[@as(usize, outset_id)].box_shadow;
    expect_eq(outset_sh.has_shadow, true);
    expect_eq(outset_sh.inset, false);

    // 2. Render & verify pixels: inset shadow does not spill outside box; outset shadow paints at +10,+10
    const png_data = renderHtmlToPng(html, 200, 220)?;
    defer std.heap.page_allocator.free(png_data);

    if (img := PngCodec.decode(png_data)) {
        // Exterior right of inset_box (x=108, y=40) must remain white
        const inset_ext = img.getPixel(108, 40);
        expect_eq(inset_ext.r, 255);
        expect_eq(inset_ext.g, 255);
        expect_eq(inset_ext.b, 255);

        // Center of outset_box (x=50, y=120) must be red (#ef4444 -> 239, 68, 68)
        const outset_center = img.getPixel(50, 120);
        expect_eq(outset_center.r, 239);
        expect_eq(outset_center.g, 68);
        expect_eq(outset_center.b, 68);

        // Outset shadow region to the right of outset_box (x=105, y=120) must be blue (0, 0, 255)
        const outset_shadow_px = img.getPixel(105, 120);
        expect_eq(outset_shadow_px.r, 0);
        expect_eq(outset_shadow_px.g, 0);
        expect_eq(outset_shadow_px.b, 255);
    } else {
        expect_eq(false, true);
    }
}

fn testClipPathInset()! {
    const html = "<!DOCTYPE html><html><body style=\"margin: 0;\"><div id=\"clipped\" style=\"width: 100px; height: 100px; background-color: #22c55e; clip-path: inset(20px 10px 20px 10px);\"></div></body></html>";
    const png_data = renderHtmlToPng(html, 150, 150)?;
    defer std.heap.page_allocator.free(png_data);

    if (img := PngCodec.decode(png_data)) {
        // Inside unclipped center (50, 50): green (#22c55e -> 34, 197, 94)
        const center = img.getPixel(50, 50);
        expect_eq(center.r, 34);
        expect_eq(center.g, 197);
        expect_eq(center.b, 94);

        // Clipped top margin (50, 10): white
        const top_px = img.getPixel(50, 10);
        expect_eq(top_px.r, 255);
        expect_eq(top_px.g, 255);
        expect_eq(top_px.b, 255);

        // Clipped bottom margin (50, 90): white
        const bot_px = img.getPixel(50, 90);
        expect_eq(bot_px.r, 255);
        expect_eq(bot_px.g, 255);
        expect_eq(bot_px.b, 255);

        // Clipped left margin (5, 50): white
        const left_px = img.getPixel(5, 50);
        expect_eq(left_px.r, 255);
        expect_eq(left_px.g, 255);
        expect_eq(left_px.b, 255);

        // Clipped right margin (95, 50): white
        const right_px = img.getPixel(95, 50);
        expect_eq(right_px.r, 255);
        expect_eq(right_px.g, 255);
        expect_eq(right_px.b, 255);
    } else {
        expect_eq(false, true);
    }
}

fn testOpacityBlending()! {
    const html = "<!DOCTYPE html><html><body style=\"margin: 0;\"><div id=\"half\" style=\"width: 80px; height: 80px; background-color: #ff0000; opacity: 0.5;\"></div><div id=\"zero\" style=\"width: 80px; height: 80px; background-color: #0000ff; opacity: 0;\"></div></body></html>";
    const png_data = renderHtmlToPng(html, 120, 200)?;
    defer std.heap.page_allocator.free(png_data);

    if (img := PngCodec.decode(png_data)) {
        // Half-transparent red over white: R=255, G & B blended around 127..128
        const half_px = img.getPixel(40, 40);
        expect_eq(half_px.r, 255);
        expect_eq(half_px.g > 100 && half_px.g < 160, true);
        expect_eq(half_px.b > 100 && half_px.b < 160, true);

        // Zero-opacity blue box (y=120): completely skipped, remains white (255, 255, 255)
        const zero_px = img.getPixel(40, 120);
        expect_eq(zero_px.r, 255);
        expect_eq(zero_px.g, 255);
        expect_eq(zero_px.b, 255);
    } else {
        expect_eq(false, true);
    }
}

fn testOverflowClip()! {
    const html = "<!DOCTYPE html><html><body style=\"margin: 0;\"><div id=\"parent\" style=\"width: 60px; height: 60px; overflow: clip; background-color: #ffffff;\"><div id=\"child\" style=\"width: 120px; height: 120px; background-color: #2563eb;\"></div></div></body></html>";
    const png_data = renderHtmlToPng(html, 150, 150)?;
    defer std.heap.page_allocator.free(png_data);

    if (img := PngCodec.decode(png_data)) {
        // Inside parent clip box (30, 30): blue (#2563eb -> 37, 99, 235)
        const inside = img.getPixel(30, 30);
        expect_eq(inside.r, 37);
        expect_eq(inside.g, 99);
        expect_eq(inside.b, 235);

        // Overflowing right (90, 30): clipped, remains white
        const over_r = img.getPixel(90, 30);
        expect_eq(over_r.r, 255);
        expect_eq(over_r.g, 255);
        expect_eq(over_r.b, 255);

        // Overflowing bottom (30, 90): clipped, remains white
        const over_b = img.getPixel(30, 90);
        expect_eq(over_b.r, 255);
        expect_eq(over_b.g, 255);
        expect_eq(over_b.b, 255);
    } else {
        expect_eq(false, true);
    }
}

fn main() {
    test("1. box-shadow inset & outset cascade + pixel verification", () => {
        testBoxShadowInsetAndOutset() catch unreachable;
    });

    test("2. clip-path: inset(t, r, b, l) pixel clipping", () => {
        testClipPathInset() catch unreachable;
    });

    test("3. opacity blending (0.5 and 0.0)", () => {
        testOpacityBlending() catch unreachable;
    });

    test("4. overflow: clip child bounding-box clipping", () => {
        testOverflowClip() catch unreachable;
    });
}
