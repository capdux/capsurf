// test/css/test_box_shadow_and_border_radius_edge.cp
// Pure CP white-box unit tests for CSS box-shadow (inset, offset, blur, spread, rgba, cascade override, multi-shadow)
// and border-radius (1/2/3/4 values, elliptical slash syntax, per-corner longhands, %, radius clamping, and PNG pixel verification).

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle, CornerShape } from "../../src/css/cascade.cp";
import { LayoutTree, Rect } from "../../src/layout/layout.cp";
import { ScreenshotOptions, renderDocumentToPng } from "../../src/render/painter.cp";
import { PngCodec } from "../../src/render/png.cp";
import { CssColor } from "cap:css";

fn nearF32(actual: f32, expected: f32, eps: f32): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

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

fn testBoxShadowCascadeEdge()! {
    const html = "<!DOCTYPE html><html><body style=\"margin:0;\">" ++
        "<div id=\"s_2len\" style=\"box-shadow: 3px -4px;\"></div>" ++
        "<div id=\"s_4len\" style=\"box-shadow: #112233 5px 6px 8px 2px;\"></div>" ++
        "<div id=\"s_inset_end\" style=\"box-shadow: 2px 4px 6px -1px rgba(10, 20, 30, 0.5) inset;\"></div>" ++
        "<div id=\"s_multi\" style=\"box-shadow: 4px 5px 2px 1px #ff0000, inset 0px 0px 3px #00ff00;\"></div>" ++
        "<div id=\"s_override\" class=\"shadowed cleared\"></div>" ++
        "<div id=\"s_invalid\" style=\"box-shadow: 10px;\"></div>" ++
        "</body></html>";
    const css = ".shadowed { box-shadow: 10px 12px 4px 2px #ff00ff; } .cleared { box-shadow: none; }";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet.parse(css, 800.0, false);
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    // 1. 2-length shadow: default blur=0, spread=0, inset=false
    const s2 = styles[@as(usize, doc.getElementById("s_2len"))].box_shadow;
    expect_eq(s2.has_shadow, true);
    expect_eq(s2.inset, false);
    expect_eq(nearF32(s2.offset_x, 3.0, 0.01), true);
    expect_eq(nearF32(s2.offset_y, -4.0, 0.01), true);
    expect_eq(nearF32(s2.blur, 0.0, 0.01), true);
    expect_eq(nearF32(s2.spread, 0.0, 0.01), true);

    // 2. Color before lengths + 4 lengths (offset_x, offset_y, blur, spread)
    const s4 = styles[@as(usize, doc.getElementById("s_4len"))].box_shadow;
    expect_eq(s4.has_shadow, true);
    expect_eq(s4.color.r, 0x11);
    expect_eq(s4.color.g, 0x22);
    expect_eq(s4.color.b, 0x33);
    expect_eq(s4.color.a, 255);
    expect_eq(nearF32(s4.offset_x, 5.0, 0.01), true);
    expect_eq(nearF32(s4.offset_y, 6.0, 0.01), true);
    expect_eq(nearF32(s4.blur, 8.0, 0.01), true);
    expect_eq(nearF32(s4.spread, 2.0, 0.01), true);

    // 3. inset at end + negative spread + rgba() color
    const si = styles[@as(usize, doc.getElementById("s_inset_end"))].box_shadow;
    expect_eq(si.has_shadow, true);
    expect_eq(si.inset, true);
    expect_eq(nearF32(si.offset_x, 2.0, 0.01), true);
    expect_eq(nearF32(si.offset_y, 4.0, 0.01), true);
    expect_eq(nearF32(si.blur, 6.0, 0.01), true);
    expect_eq(nearF32(si.spread, -1.0, 0.01), true);
    expect_eq(si.color.r, 10);
    expect_eq(si.color.g, 20);
    expect_eq(si.color.b, 30);
    expect_eq(si.color.a >= 127 && si.color.a <= 128, true);

    // 4. Multi-shadow list parses cleanly without error and records shadow presence
    const sm = styles[@as(usize, doc.getElementById("s_multi"))].box_shadow;
    expect_eq(sm.has_shadow, true);
    expect_eq(nearF32(sm.offset_x, 4.0, 0.01), true);
    expect_eq(nearF32(sm.offset_y, 5.0, 0.01), true);

    // 5. Cascade override with `box-shadow: none`
    const so = styles[@as(usize, doc.getElementById("s_override"))].box_shadow;
    expect_eq(so.has_shadow, false);

    // 6. Invalid single-length box-shadow must be ignored
    const sinv = styles[@as(usize, doc.getElementById("s_invalid"))].box_shadow;
    expect_eq(sinv.has_shadow, false);
}

fn testBorderRadiusCascadeEdge()! {
    const html = "<!DOCTYPE html><html><body style=\"margin:0;\">" ++
        "<div id=\"br_1\" style=\"border-radius: 12px;\"></div>" ++
        "<div id=\"br_2\" style=\"border-radius: 10px 20px;\"></div>" ++
        "<div id=\"br_3\" style=\"border-radius: 5px 15px 25px;\"></div>" ++
        "<div id=\"br_4\" style=\"border-radius: 4px 8px 12px 16px;\"></div>" ++
        "<div id=\"br_ellip\" style=\"border-radius: 10px 22px / 30px 40px;\"></div>" ++
        "<div id=\"br_pct\" style=\"border-radius: 50%;\"></div>" ++
        "<div id=\"br_long\" style=\"border-top-left-radius: 6px 12px; border-top-right-radius: 14px; border-bottom-right-radius: 18px; border-bottom-left-radius: 2px;\"></div>" ++
        "<div id=\"br_cs\" style=\"border-radius: 12px; corner-shape: bevel notch;\"></div>" ++
        "</body></html>";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    // 1-value shorthand
    const s1 = &styles[@as(usize, doc.getElementById("br_1"))];
    expect_eq(nearF32(s1.border_tl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 12.0, 0.01), true);
    expect_eq(nearF32(s1.border_tr_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 12.0, 0.01), true);
    expect_eq(nearF32(s1.border_br_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 12.0, 0.01), true);
    expect_eq(nearF32(s1.border_bl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 12.0, 0.01), true);

    // 2-value shorthand: tl=br=10, tr=bl=20
    const s2 = &styles[@as(usize, doc.getElementById("br_2"))];
    expect_eq(nearF32(s2.border_tl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(s2.border_tr_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 20.0, 0.01), true);
    expect_eq(nearF32(s2.border_br_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(s2.border_bl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 20.0, 0.01), true);

    // 3-value shorthand: tl=5, tr=bl=15, br=25
    const s3 = &styles[@as(usize, doc.getElementById("br_3"))];
    expect_eq(nearF32(s3.border_tl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 5.0, 0.01), true);
    expect_eq(nearF32(s3.border_tr_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 15.0, 0.01), true);
    expect_eq(nearF32(s3.border_br_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 25.0, 0.01), true);
    expect_eq(nearF32(s3.border_bl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 15.0, 0.01), true);

    // 4-value shorthand: tl=4, tr=8, br=12, bl=16
    const s4 = &styles[@as(usize, doc.getElementById("br_4"))];
    expect_eq(nearF32(s4.border_tl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 4.0, 0.01), true);
    expect_eq(nearF32(s4.border_tr_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 8.0, 0.01), true);
    expect_eq(nearF32(s4.border_br_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 12.0, 0.01), true);
    expect_eq(nearF32(s4.border_bl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 16.0, 0.01), true);

    // Elliptical slash syntax: horizontal radii before '/' are preserved cleanly
    const se = &styles[@as(usize, doc.getElementById("br_ellip"))];
    expect_eq(nearF32(se.border_tl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(se.border_tr_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 22.0, 0.01), true);
    expect_eq(nearF32(se.border_br_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(se.border_bl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 22.0, 0.01), true);

    // Percentage radius: 50% of 80px = 40px
    const sp = &styles[@as(usize, doc.getElementById("br_pct"))];
    expect_eq(nearF32(sp.border_tl_radius.resolvePx(80.0, 16.0, 16.0, 800.0, 600.0), 40.0, 0.01), true);

    // Per-corner longhands (including elliptical 2-token on top-left)
    const sl = &styles[@as(usize, doc.getElementById("br_long"))];
    expect_eq(nearF32(sl.border_tl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 6.0, 0.01), true);
    expect_eq(nearF32(sl.border_tr_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 14.0, 0.01), true);
    expect_eq(nearF32(sl.border_br_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 18.0, 0.01), true);
    expect_eq(nearF32(sl.border_bl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 2.0, 0.01), true);

    // corner-shape 2-value: tl=br=Bevel, tr=bl=Notch
    const scs = &styles[@as(usize, doc.getElementById("br_cs"))];
    expect_eq(scs.corner_shape_tl == CornerShape.Bevel, true);
    expect_eq(scs.corner_shape_tr == CornerShape.Notch, true);
    expect_eq(scs.corner_shape_br == CornerShape.Bevel, true);
    expect_eq(scs.corner_shape_bl == CornerShape.Notch, true);
}

fn testShadowSpreadAndRadiusPixels()! {
    // Box 1 (y: 20..80, x: 20..80): 60x60 red box with 0,0 offset and +10px spread blue shadow -> shadow covers x:10..90, y:10..90
    // Box 2 (y: 110..190, x: 20..100): 80x80 green box with asymmetric border-radius: 30px 0px 0px 0px
    const html = "<!DOCTYPE html><html><body style=\"margin:0;\">" ++
        "<div style=\"margin-left:20px; margin-top:20px; width:60px; height:60px; background-color:#ff0000; box-shadow: 0px 0px 0px 10px #0000ff;\"></div>" ++
        "<div style=\"margin-left:20px; margin-top:30px; width:80px; height:80px; background-color:#00ff00; border-radius: 30px 0px 0px 0px;\"></div>" ++
        "</body></html>";

    const png_data = renderHtmlToPng(html, 140, 220)?;
    defer std.heap.page_allocator.free(png_data);

    if (img := PngCodec.decode(png_data)) {
        // Box 1 center (50, 50): red (255, 0, 0)
        const b1_center = img.getPixel(50, 50);
        expect_eq(b1_center.r, 255);
        expect_eq(b1_center.g, 0);
        expect_eq(b1_center.b, 0);

        // Box 1 spread ring at top (50, 14) and left (14, 50): blue (0, 0, 255)
        const spread_top = img.getPixel(50, 14);
        expect_eq(spread_top.r, 0);
        expect_eq(spread_top.g, 0);
        expect_eq(spread_top.b, 255);

        const spread_left = img.getPixel(14, 50);
        expect_eq(spread_left.r, 0);
        expect_eq(spread_left.g, 0);
        expect_eq(spread_left.b, 255);

        // Outside spread ring (5, 50): white (255, 255, 255)
        const outside_spread = img.getPixel(5, 50);
        expect_eq(outside_spread.r, 255);
        expect_eq(outside_spread.g, 255);
        expect_eq(outside_spread.b, 255);

        // Box 2 (x: 20..100, y: 110..190) asymmetric border-radius: 30px 0 0 0
        // Top-left corner (22, 112) is rounded away -> white
        const tl_corner = img.getPixel(22, 112);
        expect_eq(tl_corner.r, 255);
        expect_eq(tl_corner.g, 255);
        expect_eq(tl_corner.b, 255);

        // Top-right corner (98, 112), bottom-right (98, 188), bottom-left (22, 188) remain sharp green (0, 255, 0)
        const tr_corner = img.getPixel(98, 112);
        expect_eq(tr_corner.r, 0);
        expect_eq(tr_corner.g, 255);
        expect_eq(tr_corner.b, 0);

        const br_corner = img.getPixel(98, 188);
        expect_eq(br_corner.r, 0);
        expect_eq(br_corner.g, 255);
        expect_eq(br_corner.b, 0);

        const bl_corner = img.getPixel(22, 188);
        expect_eq(bl_corner.r, 0);
        expect_eq(bl_corner.g, 255);
        expect_eq(bl_corner.b, 0);
    } else {
        expect_eq(false, true);
    }
}

fn testOverlargeRadiusClampingAndNegativeSpread()! {
    // Overlarge border-radius: 100px on 60x40 box -> clamped by scaleCornerRadii without corruption
    // Negative spread: -40px on 60x40 box -> shadow width <= 0, safely skipped
    const html = "<!DOCTYPE html><html><body style=\"margin:0;\">" ++
        "<div style=\"margin-left:10px; margin-top:10px; width:60px; height:40px; background-color:#10b981; border-radius: 100px; box-shadow: 5px 5px 0px -40px #000000;\"></div>" ++
        "</body></html>";

    const png_data = renderHtmlToPng(html, 100, 80)?;
    defer std.heap.page_allocator.free(png_data);

    if (img := PngCodec.decode(png_data)) {
        // Center (40, 30) must be #10b981 (16, 185, 129)
        const center = img.getPixel(40, 30);
        expect_eq(center.r, 16);
        expect_eq(center.g, 185);
        expect_eq(center.b, 129);

        // Extreme corner (11, 11) is rounded off -> white
        const corner = img.getPixel(11, 11);
        expect_eq(corner.r, 255);
        expect_eq(corner.g, 255);
        expect_eq(corner.b, 255);

        // Outside box at (72, 30) must remain white because negative spread collapsed the shadow
        const outside = img.getPixel(72, 30);
        expect_eq(outside.r, 255);
        expect_eq(outside.g, 255);
        expect_eq(outside.b, 255);
    } else {
        expect_eq(false, true);
    }
}

fn main() {
    test("1. box-shadow cascade edge cases (2/4-len, inset, rgba, multi-shadow, none override, invalid)", () => {
        testBoxShadowCascadeEdge() catch unreachable;
    });

    test("2. border-radius cascade edge cases (1/2/3/4-val, elliptical slash, %, longhands, corner-shape)", () => {
        testBorderRadiusCascadeEdge() catch unreachable;
    });

    test("3. renderDocumentToPng box-shadow spread ring & asymmetric border-radius pixel verification", () => {
        testShadowSpreadAndRadiusPixels() catch unreachable;
    });

    test("4. renderDocumentToPng overlarge border-radius clamping & negative spread collapse", () => {
        testOverlargeRadiusClampingAndNegativeSpread() catch unreachable;
    });
}
