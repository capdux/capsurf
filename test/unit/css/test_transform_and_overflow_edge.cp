// test/css/test_transform_and_overflow_edge.cp
// Pure CP white-box unit tests for CSS transform (translate/translate3d/translateX/translateY/scale/none),
// overflow (visible/hidden/clip/scroll/auto, 2-value shorthand, axis clipping), and position: fixed boundaries.

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle, OverflowMode, Position } from "../../../src/css/cascade.cp";
import { LayoutTree, Rect } from "../../../src/layout/layout.cp";

fn nearF32(actual: f32, expected: f32, eps: f32): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn testTransformCascade()! {
    const html = "<!DOCTYPE html><html><body style=\"margin:0;\">" ++
        "<div id=\"t1\" style=\"transform: translate(12px, -8px);\"></div>" ++
        "<div id=\"t2\" style=\"transform: translate3d(15px, 25px, 99px);\"></div>" ++
        "<div id=\"t3\" style=\"transform: translateX(50%);\"></div>" ++
        "<div id=\"t4\" style=\"transform: translateY(25%);\"></div>" ++
        "<div id=\"t5\" style=\"-webkit-transform: translate(7px, 9px);\"></div>" ++
        "<div id=\"t6\" class=\"moved reset\"></div>" ++
        "</body></html>";
    const css = ".moved { transform: translate(30px, 40px); } .reset { transform: none; }";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet.parse(css, 800.0, false);
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    const s1 = &styles[@as(usize, doc.getElementById("t1"))];
    expect_eq(nearF32(s1.transform_tx.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 12.0, 0.01), true);
    expect_eq(nearF32(s1.transform_ty.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), -8.0, 0.01), true);

    const s2 = &styles[@as(usize, doc.getElementById("t2"))];
    expect_eq(nearF32(s2.transform_tx.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 15.0, 0.01), true);
    expect_eq(nearF32(s2.transform_ty.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 25.0, 0.01), true);

    const s3 = &styles[@as(usize, doc.getElementById("t3"))];
    expect_eq(nearF32(s3.transform_tx.resolvePx(200.0, 16.0, 16.0, 800.0, 600.0), 100.0, 0.01), true);
    expect_eq(nearF32(s3.transform_ty.resolvePx(200.0, 16.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);

    const s4 = &styles[@as(usize, doc.getElementById("t4"))];
    expect_eq(nearF32(s4.transform_tx.resolvePx(200.0, 16.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);
    expect_eq(nearF32(s4.transform_ty.resolvePx(80.0, 16.0, 16.0, 800.0, 600.0), 20.0, 0.01), true);

    const s5 = &styles[@as(usize, doc.getElementById("t5"))];
    expect_eq(nearF32(s5.transform_tx.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 7.0, 0.01), true);
    expect_eq(nearF32(s5.transform_ty.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 9.0, 0.01), true);

    const s6 = &styles[@as(usize, doc.getElementById("t6"))];
    expect_eq(nearF32(s6.transform_tx.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);
    expect_eq(nearF32(s6.transform_ty.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);
}

fn testTransformLayoutAndScaleFallback()! {
    const html = "<!DOCTYPE html><html><body style=\"margin:0;\">" ++
        "<div id=\"parent\" style=\"width: 100px; height: 80px; transform: translate(50%, 25%);\">" ++
        "<div id=\"child\" style=\"width: 20px; height: 10px;\"></div>" ++
        "</div>" ++
        "<div id=\"scaled\" style=\"width: 40px; height: 30px; transform: scale(2, 0.5);\"></div>" ++
        "</body></html>";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let layout = LayoutTree.init(std.heap.page_allocator, 400.0, 300.0)?;
    defer layout.deinit();
    layout.computeLayout(&doc, styles)?;

    const parent_rect = layout.getBoundingClientRect(doc.getElementById("parent"));
    expect_eq(nearF32(parent_rect.x, 50.0, 0.1), true);
    expect_eq(nearF32(parent_rect.y, 20.0, 0.1), true);
    expect_eq(nearF32(parent_rect.width, 100.0, 0.1), true);
    expect_eq(nearF32(parent_rect.height, 80.0, 0.1), true);

    const child_rect = layout.getBoundingClientRect(doc.getElementById("child"));
    expect_eq(nearF32(child_rect.x, 50.0, 0.1), true);
    expect_eq(nearF32(child_rect.y, 20.0, 0.1), true);

    // scale() is not a translation; must safely keep tx=0, ty=0 and valid box geometry
    const scaled_id = doc.getElementById("scaled");
    const scaled_style = &styles[@as(usize, scaled_id)];
    expect_eq(nearF32(scaled_style.transform_tx.resolvePx(40.0, 16.0, 16.0, 400.0, 300.0), 0.0, 0.01), true);
    expect_eq(nearF32(scaled_style.transform_ty.resolvePx(30.0, 16.0, 16.0, 400.0, 300.0), 0.0, 0.01), true);

    const scaled_rect = layout.getBoundingClientRect(scaled_id);
    expect_eq(nearF32(scaled_rect.x, 0.0, 0.1), true);
    expect_eq(nearF32(scaled_rect.y, 80.0, 0.1), true);
    expect_eq(nearF32(scaled_rect.width, 40.0, 0.1), true);
    expect_eq(nearF32(scaled_rect.height, 30.0, 0.1), true);
}

fn testOverflowModesAndAxisClipping()! {
    const html = "<!DOCTYPE html><html><body style=\"margin:0;\">" ++
        "<div id=\"ov_def\"></div>" ++
        "<div id=\"ov_hid\" style=\"overflow: hidden;\"></div>" ++
        "<div id=\"ov_clp\" style=\"overflow: clip;\"></div>" ++
        "<div id=\"ov_scr\" style=\"overflow: scroll;\"></div>" ++
        "<div id=\"ov_aut\" style=\"overflow: auto;\"></div>" ++
        "<div id=\"ov_hv\" style=\"overflow: hidden visible;\"></div>" ++
        "<div id=\"ov_cv\" style=\"overflow: clip visible;\"></div>" ++
        "<div id=\"ov_xy\" style=\"overflow-x: scroll; overflow-y: clip;\"></div>" ++
        "<div id=\"ov_bad\" style=\"overflow: bogus_value;\"></div>" ++
        "</body></html>";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let layout = LayoutTree.init(std.heap.page_allocator, 400.0, 300.0)?;
    defer layout.deinit();
    layout.computeLayout(&doc, styles)?;

    const s_def = &styles[@as(usize, doc.getElementById("ov_def"))];
    expect_eq(s_def.overflow_x == OverflowMode.Visible, true);
    expect_eq(s_def.overflow_y == OverflowMode.Visible, true);
    expect_eq(s_def.overflow_visible, true);
    expect_eq(s_def.clipsOverflowX(), false);
    expect_eq(s_def.clipsOverflowY(), false);

    const s_hid = &styles[@as(usize, doc.getElementById("ov_hid"))];
    expect_eq(s_hid.overflow_x == OverflowMode.Hidden, true);
    expect_eq(s_hid.overflow_y == OverflowMode.Hidden, true);
    expect_eq(s_hid.overflow_visible, false);
    expect_eq(s_hid.clipsOverflowX(), true);
    expect_eq(s_hid.clipsOverflowY(), true);
    expect_eq(layout.boxes.items[@as(usize, doc.getElementById("ov_hid"))].overflow_visible, false);

    const s_clp = &styles[@as(usize, doc.getElementById("ov_clp"))];
    expect_eq(s_clp.overflow_x == OverflowMode.Clip, true);
    expect_eq(s_clp.overflow_y == OverflowMode.Clip, true);
    expect_eq(s_clp.overflow_visible, false);
    expect_eq(s_clp.clipsOverflowX(), true);
    expect_eq(s_clp.clipsOverflowY(), true);

    const s_scr = &styles[@as(usize, doc.getElementById("ov_scr"))];
    expect_eq(s_scr.overflow_x == OverflowMode.Scroll, true);
    expect_eq(s_scr.overflow_y == OverflowMode.Scroll, true);
    expect_eq(s_scr.clipsOverflowX(), true);
    expect_eq(s_scr.clipsOverflowY(), true);

    const s_aut = &styles[@as(usize, doc.getElementById("ov_aut"))];
    expect_eq(s_aut.overflow_x == OverflowMode.Auto, true);
    expect_eq(s_aut.overflow_y == OverflowMode.Auto, true);
    expect_eq(s_aut.clipsOverflowX(), true);
    expect_eq(s_aut.clipsOverflowY(), true);

    // overflow: hidden visible -> hidden on X forces Y clipping per CSS Overflow
    const s_hv = &styles[@as(usize, doc.getElementById("ov_hv"))];
    expect_eq(s_hv.overflow_x == OverflowMode.Hidden, true);
    expect_eq(s_hv.overflow_y == OverflowMode.Visible, true);
    expect_eq(s_hv.overflow_visible, false);
    expect_eq(s_hv.clipsOverflowX(), true);
    expect_eq(s_hv.clipsOverflowY(), true);

    // overflow: clip visible -> clip on X does NOT force Y clipping
    const s_cv = &styles[@as(usize, doc.getElementById("ov_cv"))];
    expect_eq(s_cv.overflow_x == OverflowMode.Clip, true);
    expect_eq(s_cv.overflow_y == OverflowMode.Visible, true);
    expect_eq(s_cv.overflow_visible, false);
    expect_eq(s_cv.clipsOverflowX(), true);
    expect_eq(s_cv.clipsOverflowY(), false);

    const s_xy = &styles[@as(usize, doc.getElementById("ov_xy"))];
    expect_eq(s_xy.overflow_x == OverflowMode.Scroll, true);
    expect_eq(s_xy.overflow_y == OverflowMode.Clip, true);
    expect_eq(s_xy.clipsOverflowX(), true);
    expect_eq(s_xy.clipsOverflowY(), true);

    // Invalid keyword is ignored, leaving default visible
    const s_bad = &styles[@as(usize, doc.getElementById("ov_bad"))];
    expect_eq(s_bad.overflow_x == OverflowMode.Visible, true);
    expect_eq(s_bad.overflow_y == OverflowMode.Visible, true);
    expect_eq(s_bad.overflow_visible, true);
}

fn testPositionFixedBoundaries()! {
    const html = "<!DOCTYPE html><html><body style=\"margin:0;\">" ++
        "<div style=\"position: relative; left: 100px; top: 80px; width: 200px; height: 200px;\">" ++
        "<div id=\"fix_tl\" style=\"position: fixed; left: 15px; top: 25px; width: 60px; height: 40px;\"></div>" ++
        "<div id=\"fix_rb\" style=\"position: fixed; right: 20px; bottom: 10px; width: 50px; height: 30px;\"></div>" ++
        "<div id=\"fix_tr\" style=\"position: fixed; left: 10px; top: 10px; width: 40px; height: 20px; transform: translate(12px, -4px);\"></div>" ++
        "</div>" ++
        "<div id=\"fix_static\" style=\"width: 100px; height: 50px;\">" ++
        "<div id=\"fix_auto\" style=\"position: fixed; width: 30px; height: 20px;\"></div>" ++
        "</div>" ++
        "</body></html>";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    const fix_tl_id = doc.getElementById("fix_tl");
    expect_eq(styles[@as(usize, fix_tl_id)].position == Position.Fixed, true);

    let layout = LayoutTree.init(std.heap.page_allocator, 500.0, 400.0)?;
    defer layout.deinit();
    layout.computeLayout(&doc, styles)?;

    // Fixed ignores relative parent offset (100, 80) and resolves against viewport (0, 0, 500, 400)
    const r_tl = layout.getBoundingClientRect(fix_tl_id);
    expect_eq(nearF32(r_tl.x, 15.0, 0.1), true);
    expect_eq(nearF32(r_tl.y, 25.0, 0.1), true);
    expect_eq(nearF32(r_tl.width, 60.0, 0.1), true);
    expect_eq(nearF32(r_tl.height, 40.0, 0.1), true);

    // right: 20px; bottom: 10px on 500x400 viewport -> x = 500 - 20 - 50 = 430, y = 400 - 10 - 30 = 360
    const r_rb = layout.getBoundingClientRect(doc.getElementById("fix_rb"));
    expect_eq(nearF32(r_rb.x, 430.0, 0.1), true);
    expect_eq(nearF32(r_rb.y, 360.0, 0.1), true);

    // fixed + transform: (10 + 12, 10 - 4) = (22, 6)
    const r_tr = layout.getBoundingClientRect(doc.getElementById("fix_tr"));
    expect_eq(nearF32(r_tr.x, 22.0, 0.1), true);
    expect_eq(nearF32(r_tr.y, 6.0, 0.1), true);

    // fixed with auto insets falls back to static position (0, 200 after the 200px relative div)
    const r_auto = layout.getBoundingClientRect(doc.getElementById("fix_auto"));
    expect_eq(nearF32(r_auto.x, 0.0, 0.1), true);
    expect_eq(nearF32(r_auto.y, 200.0, 0.1), true);
}

fn main() {
    test("1. transform translate/translate3d/translateX/translateY/none & -webkit-transform cascade", () => {
        testTransformCascade() catch unreachable;
    });

    test("2. transform percentage layout shift, subtree propagation & scale() safe fallback", () => {
        testTransformLayoutAndScaleFallback() catch unreachable;
    });

    test("3. overflow: visible/hidden/clip/scroll/auto, 2-value shorthand & axis clipping", () => {
        testOverflowModesAndAxisClipping() catch unreachable;
    });

    test("4. position: fixed viewport resolution, right/bottom anchoring & transform", () => {
        testPositionFixedBoundaries() catch unreachable;
    });
}
