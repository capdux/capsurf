// test/css/test_shorthand_and_computed_edge.cp
// Pure CP white-box unit tests for CSS shorthand expansion (margin, padding, border, background, flex),
// computed style inheritance (font, color, line-height, visibility), CSS-wide keyword fallback
// (inherit, initial, unset), and malformed/invalid declaration fault tolerance.

import { test, expect_eq } from "cap:test";
import { Document, NULL_NODE } from "../../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle, Visibility } from "../../../src/css/cascade.cp";

fn nearF32(actual: f32, expected: f32, eps: f32): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn testMarginAndPaddingShorthands()! {
    const html = "<!DOCTYPE html><html><body style=\"margin:0;\">" ++
        "<div id=\"m1\" style=\"margin: 10px;\"></div>" ++
        "<div id=\"m2\" style=\"margin: 10px 20px;\"></div>" ++
        "<div id=\"m3\" style=\"margin: 10px 20px 30px;\"></div>" ++
        "<div id=\"m4\" style=\"margin: 1px 2px 3px 4px;\"></div>" ++
        "<div id=\"m_ov\" style=\"margin: 10px; margin-left: 25px;\"></div>" ++
        "<div id=\"m_auto\" style=\"margin: 12px auto;\"></div>" ++
        "<div id=\"p1\" style=\"padding: 8px;\"></div>" ++
        "<div id=\"p2\" style=\"padding: 4px 12px;\"></div>" ++
        "<div id=\"p3\" style=\"padding: 2px 6px 10px;\"></div>" ++
        "<div id=\"p4\" style=\"padding: 1px 3px 5px 7px;\"></div>" ++
        "<div id=\"p_ov\" style=\"padding: 5px; padding-right: 19px;\"></div>" ++
        "</body></html>";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    // margin: 1-value
    const m1 = &styles[@as(usize, doc.getElementById("m1"))];
    expect_eq(nearF32(m1.margin_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(m1.margin_right.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(m1.margin_bottom.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(m1.margin_left.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);

    // margin: 2-value
    const m2 = &styles[@as(usize, doc.getElementById("m2"))];
    expect_eq(nearF32(m2.margin_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(m2.margin_right.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 20.0, 0.01), true);
    expect_eq(nearF32(m2.margin_bottom.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(m2.margin_left.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 20.0, 0.01), true);

    // margin: 3-value
    const m3 = &styles[@as(usize, doc.getElementById("m3"))];
    expect_eq(nearF32(m3.margin_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(m3.margin_right.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 20.0, 0.01), true);
    expect_eq(nearF32(m3.margin_bottom.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 30.0, 0.01), true);
    expect_eq(nearF32(m3.margin_left.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 20.0, 0.01), true);

    // margin: 4-value
    const m4 = &styles[@as(usize, doc.getElementById("m4"))];
    expect_eq(nearF32(m4.margin_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 1.0, 0.01), true);
    expect_eq(nearF32(m4.margin_right.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 2.0, 0.01), true);
    expect_eq(nearF32(m4.margin_bottom.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 3.0, 0.01), true);
    expect_eq(nearF32(m4.margin_left.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 4.0, 0.01), true);

    // margin shorthand + longhand override
    const m_ov = &styles[@as(usize, doc.getElementById("m_ov"))];
    expect_eq(nearF32(m_ov.margin_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(m_ov.margin_left.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 25.0, 0.01), true);

    // margin: 12px auto
    const m_auto = &styles[@as(usize, doc.getElementById("m_auto"))];
    expect_eq(nearF32(m_auto.margin_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 12.0, 0.01), true);
    expect_eq(m_auto.margin_left.is_auto, true);
    expect_eq(m_auto.margin_right.is_auto, true);

    // padding: 1/2/3/4-value + longhand override
    const p1 = &styles[@as(usize, doc.getElementById("p1"))];
    expect_eq(nearF32(p1.padding_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 8.0, 0.01), true);
    expect_eq(nearF32(p1.padding_left.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 8.0, 0.01), true);

    const p2 = &styles[@as(usize, doc.getElementById("p2"))];
    expect_eq(nearF32(p2.padding_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 4.0, 0.01), true);
    expect_eq(nearF32(p2.padding_right.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 12.0, 0.01), true);
    expect_eq(nearF32(p2.padding_bottom.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 4.0, 0.01), true);
    expect_eq(nearF32(p2.padding_left.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 12.0, 0.01), true);

    const p3 = &styles[@as(usize, doc.getElementById("p3"))];
    expect_eq(nearF32(p3.padding_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 2.0, 0.01), true);
    expect_eq(nearF32(p3.padding_right.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 6.0, 0.01), true);
    expect_eq(nearF32(p3.padding_bottom.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(p3.padding_left.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 6.0, 0.01), true);

    const p4 = &styles[@as(usize, doc.getElementById("p4"))];
    expect_eq(nearF32(p4.padding_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 1.0, 0.01), true);
    expect_eq(nearF32(p4.padding_right.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 3.0, 0.01), true);
    expect_eq(nearF32(p4.padding_bottom.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 5.0, 0.01), true);
    expect_eq(nearF32(p4.padding_left.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 7.0, 0.01), true);

    const p_ov = &styles[@as(usize, doc.getElementById("p_ov"))];
    expect_eq(nearF32(p_ov.padding_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 5.0, 0.01), true);
    expect_eq(nearF32(p_ov.padding_right.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 19.0, 0.01), true);
}

fn testBorderBackgroundAndFlexShorthands()! {
    const html = "<!DOCTYPE html><html><body style=\"margin:0;\">" ++
        "<div id=\"b_sh\" style=\"border: 2px solid #ff0000;\"></div>" ++
        "<div id=\"b_kw\" style=\"border: thick solid rgb(10, 20, 30);\"></div>" ++
        "<div id=\"bw4\" style=\"border-width: 1px 2px 3px 4px;\"></div>" ++
        "<div id=\"bw3\" style=\"border-width: 2px 5px 8px;\"></div>" ++
        "<div id=\"bc4\" style=\"border-color: #110000 #002200 #000033 #445566;\"></div>" ++
        "<div id=\"br_slash\" style=\"border-radius: 10px 20px / 4px 8px;\"></div>" ++
        "<div id=\"bg_col\" style=\"background: #336699;\"></div>" ++
        "<div id=\"bg_trans\" style=\"background: transparent;\"></div>" ++
        "<div id=\"f_none\" style=\"flex: none;\"></div>" ++
        "<div id=\"f1\" style=\"flex: 2;\"></div>" ++
        "<div id=\"f2\" style=\"flex: 2 3;\"></div>" ++
        "<div id=\"f3\" style=\"flex: 1 0 50px;\"></div>" ++
        "<div id=\"f_gb\" style=\"flex: 3 40%;\"></div>" ++
        "</body></html>";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    // border shorthand
    const b_sh = &styles[@as(usize, doc.getElementById("b_sh"))];
    expect_eq(nearF32(b_sh.border_top_width, 2.0, 0.01), true);
    expect_eq(nearF32(b_sh.border_right_width, 2.0, 0.01), true);
    expect_eq(nearF32(b_sh.border_bottom_width, 2.0, 0.01), true);
    expect_eq(nearF32(b_sh.border_left_width, 2.0, 0.01), true);
    expect_eq(b_sh.has_explicit_border_color, true);
    expect_eq(b_sh.border_color.r, @as(u8, 255));
    expect_eq(b_sh.border_color.g, @as(u8, 0));

    const b_kw = &styles[@as(usize, doc.getElementById("b_kw"))];
    expect_eq(nearF32(b_kw.border_top_width, 5.0, 0.01), true);
    expect_eq(b_kw.border_color.r, @as(u8, 10));
    expect_eq(b_kw.border_color.g, @as(u8, 20));
    expect_eq(b_kw.border_color.b, @as(u8, 30));

    // border-width 4-value & 3-value
    const bw4 = &styles[@as(usize, doc.getElementById("bw4"))];
    expect_eq(nearF32(bw4.border_top_width, 1.0, 0.01), true);
    expect_eq(nearF32(bw4.border_right_width, 2.0, 0.01), true);
    expect_eq(nearF32(bw4.border_bottom_width, 3.0, 0.01), true);
    expect_eq(nearF32(bw4.border_left_width, 4.0, 0.01), true);

    const bw3 = &styles[@as(usize, doc.getElementById("bw3"))];
    expect_eq(nearF32(bw3.border_top_width, 2.0, 0.01), true);
    expect_eq(nearF32(bw3.border_right_width, 5.0, 0.01), true);
    expect_eq(nearF32(bw3.border_bottom_width, 8.0, 0.01), true);
    expect_eq(nearF32(bw3.border_left_width, 5.0, 0.01), true);

    // border-color 4-value
    const bc4 = &styles[@as(usize, doc.getElementById("bc4"))];
    expect_eq(bc4.border_top_color.r, @as(u8, 0x11));
    expect_eq(bc4.border_right_color.g, @as(u8, 0x22));
    expect_eq(bc4.border_bottom_color.b, @as(u8, 0x33));
    expect_eq(bc4.border_left_color.r, @as(u8, 0x44));
    expect_eq(bc4.border_left_color.g, @as(u8, 0x55));

    // border-radius with '/'
    const br_slash = &styles[@as(usize, doc.getElementById("br_slash"))];
    expect_eq(nearF32(br_slash.border_tl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(br_slash.border_tr_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 20.0, 0.01), true);
    expect_eq(nearF32(br_slash.border_br_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 10.0, 0.01), true);
    expect_eq(nearF32(br_slash.border_bl_radius.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 20.0, 0.01), true);

    // background shorthand
    const bg_col = &styles[@as(usize, doc.getElementById("bg_col"))];
    expect_eq(bg_col.background_color.r, @as(u8, 0x33));
    expect_eq(bg_col.background_color.g, @as(u8, 0x66));
    expect_eq(bg_col.background_color.b, @as(u8, 0x99));
    expect_eq(bg_col.background_color.a, @as(u8, 255));

    const bg_trans = &styles[@as(usize, doc.getElementById("bg_trans"))];
    expect_eq(bg_trans.background_color.a, @as(u8, 0));

    // flex shorthand
    const f_none = &styles[@as(usize, doc.getElementById("f_none"))];
    expect_eq(nearF32(f_none.flex_grow, 0.0, 0.01), true);
    expect_eq(nearF32(f_none.flex_shrink, 0.0, 0.01), true);
    expect_eq(f_none.flex_basis.is_auto, true);

    const f1 = &styles[@as(usize, doc.getElementById("f1"))];
    expect_eq(nearF32(f1.flex_grow, 2.0, 0.01), true);
    expect_eq(nearF32(f1.flex_shrink, 1.0, 0.01), true);
    expect_eq(nearF32(f1.flex_basis.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);

    const f2 = &styles[@as(usize, doc.getElementById("f2"))];
    expect_eq(nearF32(f2.flex_grow, 2.0, 0.01), true);
    expect_eq(nearF32(f2.flex_shrink, 3.0, 0.01), true);

    const f3 = &styles[@as(usize, doc.getElementById("f3"))];
    expect_eq(nearF32(f3.flex_grow, 1.0, 0.01), true);
    expect_eq(nearF32(f3.flex_shrink, 0.0, 0.01), true);
    expect_eq(nearF32(f3.flex_basis.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 50.0, 0.01), true);

    const f_gb = &styles[@as(usize, doc.getElementById("f_gb"))];
    expect_eq(nearF32(f_gb.flex_grow, 3.0, 0.01), true);
    expect_eq(nearF32(f_gb.flex_basis.resolvePx(200.0, 16.0, 16.0, 800.0, 600.0), 80.0, 0.01), true);
}

fn testInheritanceAndCssWideKeywords()! {
    const html = "<!DOCTYPE html><html><body style=\"margin:0;\">" ++
        "<div id=\"parent\" style=\"color: #123456; font-size: 20px; font-weight: 700; font-family: 'Fira Code', monospace; line-height: 1.5; visibility: hidden; margin: 16px; padding: 12px; background-color: #abcdef;\">" ++
        "<span id=\"child_inh\"></span>" ++
        "<div id=\"child_fs\" style=\"font-size: 10px;\"></div>" ++
        "<div id=\"child_kw\" style=\"color: inherit; font-size: initial; margin: unset; padding: inherit;\"></div>" ++
        "<h1 id=\"child_h1\"></h1>" ++
        "</div>" ++
        "<b id=\"bold_el\"></b>" ++
        "</body></html>";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    // Inherited font, color, line-height, visibility; non-inherited margin/padding/background
    const child_inh = &styles[@as(usize, doc.getElementById("child_inh"))];
    expect_eq(child_inh.color.r, @as(u8, 0x12));
    expect_eq(child_inh.color.g, @as(u8, 0x34));
    expect_eq(child_inh.color.b, @as(u8, 0x56));
    expect_eq(nearF32(child_inh.font_size_px, 20.0, 0.01), true);
    expect_eq(nearF32(child_inh.font_weight, 700.0, 0.01), true);
    expect_eq(child_inh.hasFontFamily("Fira Code"), true);
    expect_eq(nearF32(child_inh.resolvedLineHeight(), 30.0, 0.01), true);
    expect_eq(child_inh.visibility == Visibility.Hidden, true);
    expect_eq(nearF32(child_inh.margin_top.resolvePx(100.0, 20.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);
    expect_eq(nearF32(child_inh.padding_top.resolvePx(100.0, 20.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);
    expect_eq(child_inh.background_color.a, @as(u8, 0));

    // Unitless line-height scales with child's own font-size
    const child_fs = &styles[@as(usize, doc.getElementById("child_fs"))];
    expect_eq(nearF32(child_fs.font_size_px, 10.0, 0.01), true);
    expect_eq(nearF32(child_fs.resolvedLineHeight(), 15.0, 0.01), true);

    // CSS-wide keywords (inherit/initial/unset) do not corrupt inherited or initial values
    const child_kw = &styles[@as(usize, doc.getElementById("child_kw"))];
    expect_eq(child_kw.color.r, @as(u8, 0x12));
    expect_eq(nearF32(child_kw.font_size_px, 20.0, 0.01), true);
    expect_eq(nearF32(child_kw.margin_top.resolvePx(100.0, 20.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);
    expect_eq(nearF32(child_kw.padding_top.resolvePx(100.0, 20.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);

    // <h1> uses UA heading font-size instead of inheriting parent's 20px; <b> forces 700 weight
    const child_h1 = &styles[@as(usize, doc.getElementById("child_h1"))];
    expect_eq(nearF32(child_h1.font_size_px, 32.0, 0.01), true);

    const bold_el = &styles[@as(usize, doc.getElementById("bold_el"))];
    expect_eq(nearF32(bold_el.font_weight, 700.0, 0.01), true);
}

fn testInvalidDeclarationFaultTolerance()! {
    const html = "<!DOCTYPE html><html><body style=\"margin:0;\">" ++
        "<div id=\"bad\" style=\"margin: 14px; margin: 10px bogus 20px; padding: not_a_len; border-width: badwidth; border-color: notacolor; flex: -5; background: notacolor; color: #zzzzzz; font-size: nope; unknown-xyz: 99px;\"></div>" ++
        "<div id=\"empty\" style=\"margin: ; padding: ; border-radius: ; flex: ;\"></div>" ++
        "</body></html>";

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml(html)?;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount())?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    // Valid margin: 14px before malformed margin: 10px bogus 20px must remain intact
    const bad = &styles[@as(usize, doc.getElementById("bad"))];
    expect_eq(nearF32(bad.margin_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 14.0, 0.01), true);
    expect_eq(nearF32(bad.padding_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);
    expect_eq(nearF32(bad.border_top_width, 0.0, 0.01), true);
    expect_eq(nearF32(bad.flex_grow, 0.0, 0.01), true);
    expect_eq(nearF32(bad.flex_shrink, 1.0, 0.01), true);
    expect_eq(bad.background_color.a, @as(u8, 0));
    expect_eq(bad.color.r, @as(u8, 0));
    expect_eq(nearF32(bad.font_size_px, 16.0, 0.01), true);

    const empty = &styles[@as(usize, doc.getElementById("empty"))];
    expect_eq(nearF32(empty.margin_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);
    expect_eq(nearF32(empty.padding_top.resolvePx(100.0, 16.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);
    expect_eq(nearF32(empty.flex_grow, 0.0, 0.01), true);
}

fn main() {
    test("1. margin & padding 1/2/3/4-value shorthand expansion and longhand override", () => {
        testMarginAndPaddingShorthands() catch unreachable;
    });

    test("2. border, border-width, border-color, border-radius, background & flex shorthands", () => {
        testBorderBackgroundAndFlexShorthands() catch unreachable;
    });

    test("3. computed style font/color/line-height/visibility inheritance & CSS-wide keywords", () => {
        testInheritanceAndCssWideKeywords() catch unreachable;
    });

    test("4. invalid and empty CSS declaration fault tolerance", () => {
        testInvalidDeclarationFaultTolerance() catch unreachable;
    });
}
