// test/render/test_checkbox_and_scroll_paint.cp
// CapSurf: White-Box Unit Tests for FormControlValue.checked & ScreenshotOptions.viewport_scroll_x/y

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../../src/css/cascade.cp";
import { LayoutTree, Rect } from "../../../src/layout/layout.cp";
import { FormControlValue, ScreenshotOptions, renderDocumentToPng } from "../../../src/render/painter.cp";
import { PngCodec } from "../../../src/render/png.cp";
import { CssColor } from "cap:css";

fn testCheckboxCheckedAndUncheckedPixels()! {
    // cb_checked has NO HTML checked attr, overridden to checked=true via FormControlValue
    // cb_unchecked HAS HTML checked attr, overridden to checked=false via FormControlValue
    const html = "<!DOCTYPE html><html><body style=\"margin: 0;\"><input id=\"cb_checked\" type=\"checkbox\" style=\"display: block; width: 24px; height: 24px; margin: 10px;\"><input id=\"cb_unchecked\" type=\"checkbox\" checked style=\"display: block; width: 24px; height: 24px; margin: 10px;\"></body></html>";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let layout = LayoutTree.init(std.heap.page_allocator, 120.0, 120.0)?;
    defer layout.deinit();
    layout.computeLayout(&doc, styles)?;

    let rects = std.heap.page_allocator.alloc(Rect, doc.nodeCount())?;
    defer std.heap.page_allocator.free(rects);
    let nid: usize = 0;
    while (nid < doc.nodeCount()) : (nid += 1) {
        rects[nid] = layout.getBoundingClientRect(@as(u32, nid));
    }

    const cb_checked_id = doc.getElementById("cb_checked");
    const cb_unchecked_id = doc.getElementById("cb_unchecked");
    expect_eq(cb_checked_id != NULL_NODE, true);
    expect_eq(cb_unchecked_id != NULL_NODE, true);

    const r1 = rects[@as(usize, cb_checked_id)];
    const r2 = rects[@as(usize, cb_unchecked_id)];
    expect_eq(r1.width > 0.0 && r1.height > 0.0, true);
    expect_eq(r2.width > 0.0 && r2.height > 0.0, true);

    const form_vals: [2]FormControlValue = [
        FormControlValue{ node_id: cb_checked_id, checked: true },
        FormControlValue{ node_id: cb_unchecked_id, checked: false },
    ];

    const opts = ScreenshotOptions{
        viewport_w: 120,
        viewport_h: 120,
        compress_level: 1,
        clear_color: CssColor.rgb(255, 255, 255),
        form_values: form_vals[0..],
    };

    const png_data = renderDocumentToPng(std.heap.page_allocator, &doc, styles, rects, opts)?;
    defer std.heap.page_allocator.free(png_data);

    if (img := PngCodec.decode(png_data)) {
        // 1. Checked checkbox: top-left interior (0.25, 0.25) is accent blue (0, 117, 255)
        const chk_bg_x = @as(u32, @intFromFloat(r1.x + r1.width * 0.25));
        const chk_bg_y = @as(u32, @intFromFloat(r1.y + r1.height * 0.25));
        const chk_bg = img.getPixel(chk_bg_x, chk_bg_y);
        expect_eq(chk_bg.r, 0);
        expect_eq(chk_bg.g, 117);
        expect_eq(chk_bg.b, 255);

        // 2. Checked checkbox: checkmark vertex (0.42, 0.72) is white (255, 255, 255)
        const chk_mark_x = @as(u32, @intFromFloat(r1.x + r1.width * 0.42));
        const chk_mark_y = @as(u32, @intFromFloat(r1.y + r1.height * 0.72));
        const chk_mark = img.getPixel(chk_mark_x, chk_mark_y);
        expect_eq(chk_mark.r, 255);
        expect_eq(chk_mark.g, 255);
        expect_eq(chk_mark.b, 255);

        // 3. Unchecked checkbox (HTML checked attr overridden to false): interior is white (255, 255, 255)
        const unchk_bg_x = @as(u32, @intFromFloat(r2.x + r2.width * 0.25));
        const unchk_bg_y = @as(u32, @intFromFloat(r2.y + r2.height * 0.25));
        const unchk_bg = img.getPixel(unchk_bg_x, unchk_bg_y);
        expect_eq(unchk_bg.r, 255);
        expect_eq(unchk_bg.g, 255);
        expect_eq(unchk_bg.b, 255);

        // 4. Unchecked checkbox: border pixel at left edge mid-height is gray (118, 118, 118)
        const unchk_bdr_x = @as(u32, @intFromFloat(r2.x));
        const unchk_bdr_y = @as(u32, @intFromFloat(r2.y + r2.height * 0.5));
        const unchk_bdr = img.getPixel(unchk_bdr_x, unchk_bdr_y);
        expect_eq(unchk_bdr.r, 118);
        expect_eq(unchk_bdr.g, 118);
        expect_eq(unchk_bdr.b, 118);
    } else {
        expect_eq(false, true);
    }
}

fn testViewportScrollOffsetPaint()! {
    // top_box at (0, 0, 100, 100) red (#ef4444 -> 239, 68, 68)
    // target_box at (150, 300, 80, 80) green (#22c55e -> 34, 197, 94)
    const html = "<!DOCTYPE html><html><body style=\"margin: 0;\"><div id=\"top_box\" style=\"width: 100px; height: 100px; background-color: #ef4444;\"></div><div id=\"target_box\" style=\"margin-left: 150px; margin-top: 200px; width: 80px; height: 80px; background-color: #22c55e;\"></div></body></html>";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let layout = LayoutTree.init(std.heap.page_allocator, 200.0, 200.0)?;
    defer layout.deinit();
    layout.computeLayout(&doc, styles)?;

    let rects = std.heap.page_allocator.alloc(Rect, doc.nodeCount())?;
    defer std.heap.page_allocator.free(rects);
    let nid: usize = 0;
    while (nid < doc.nodeCount()) : (nid += 1) {
        rects[nid] = layout.getBoundingClientRect(@as(u32, nid));
    }

    // 1. Unscrolled (0, 0): (50, 50) is red top_box; target_box is off-screen
    const opts_unscrolled = ScreenshotOptions{
        viewport_w: 200,
        viewport_h: 200,
        compress_level: 1,
        clear_color: CssColor.rgb(255, 255, 255),
        viewport_scroll_x: 0.0,
        viewport_scroll_y: 0.0,
    };
    const png0 = renderDocumentToPng(std.heap.page_allocator, &doc, styles, rects, opts_unscrolled)?;
    defer std.heap.page_allocator.free(png0);

    if (img0 := PngCodec.decode(png0)) {
        const px_top = img0.getPixel(50, 50);
        expect_eq(px_top.r, 239);
        expect_eq(px_top.g, 68);
        expect_eq(px_top.b, 68);
    } else {
        expect_eq(false, true);
    }

    // 2. Scrolled (150, 300) simulating window.scrollTo(150, 300):
    // target_box at (150..230, 300..380) shifts to (0..80, 0..80) in viewport!
    const opts_scrolled = ScreenshotOptions{
        viewport_w: 200,
        viewport_h: 200,
        compress_level: 1,
        clear_color: CssColor.rgb(255, 255, 255),
        viewport_scroll_x: 150.0,
        viewport_scroll_y: 300.0,
    };
    const png1 = renderDocumentToPng(std.heap.page_allocator, &doc, styles, rects, opts_scrolled)?;
    defer std.heap.page_allocator.free(png1);

    if (img1 := PngCodec.decode(png1)) {
        // Inside shifted target_box (40, 40) must be green (34, 197, 94)
        const px_in = img1.getPixel(40, 40);
        expect_eq(px_in.r, 34);
        expect_eq(px_in.g, 197);
        expect_eq(px_in.b, 94);

        // Outside shifted target_box (120, 40) must be white background (255, 255, 255)
        const px_out = img1.getPixel(120, 40);
        expect_eq(px_out.r, 255);
        expect_eq(px_out.g, 255);
        expect_eq(px_out.b, 255);
    } else {
        expect_eq(false, true);
    }
}

fn main() {
    test("1. FormControlValue.checked checked vs unchecked pixel distinction", () => {
        testCheckboxCheckedAndUncheckedPixels() catch unreachable;
    });

    test("2. ScreenshotOptions.viewport_scroll_x/y viewport translation paint", () => {
        testViewportScrollOffsetPaint() catch unreachable;
    });
}
