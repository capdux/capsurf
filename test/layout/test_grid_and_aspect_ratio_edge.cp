// test/layout/test_grid_and_aspect_ratio_edge.cp
// Pure CP white-box unit tests for LayoutTree:
// 1. CSS Grid: grid-template-columns / grid-template-rows (fr, px, auto, repeat), gap/row-gap/column-gap
// 2. CSS Grid edge cases: repeat(0, ...), span clamping, negative free space clamping, grid-placed abspos
// 3. Intrinsic aspect-ratio derivation: <svg> viewBox, width/height auto derivation
// 4. Zero / negative / invalid aspect-ratio & dimension edge cases

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";
import { LayoutTree, Rect } from "../../src/layout/layout.cp";

fn main() {
    test("1. Grid repeat(), fr/px/auto tracks, and row/column gap", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;
        const g_rep = doc.createElementWithAttrs("div", "g-rep", "", "") catch unreachable;
        const r1 = doc.createElementWithAttrs("div", "r1", "cell", "") catch unreachable;
        const r2 = doc.createElementWithAttrs("div", "r2", "cell", "") catch unreachable;
        const r3 = doc.createElementWithAttrs("div", "r3", "cell", "") catch unreachable;
        const r4 = doc.createElementWithAttrs("div", "r4", "cell", "") catch unreachable;

        doc.appendChild(root, g_rep);
        doc.appendChild(g_rep, r1);
        doc.appendChild(g_rep, r2);
        doc.appendChild(g_rep, r3);
        doc.appendChild(g_rep, r4);

        const g_mix = doc.createElementWithAttrs("div", "g-mix", "", "") catch unreachable;
        const m1 = doc.createElementWithAttrs("div", "m1", "mcell", "") catch unreachable;
        const m2 = doc.createElementWithAttrs("div", "m2", "mcell", "") catch unreachable;
        const m3 = doc.createElementWithAttrs("div", "m3", "mcell", "") catch unreachable;

        doc.appendChild(root, g_mix);
        doc.appendChild(g_mix, m1);
        doc.appendChild(g_mix, m2);
        doc.appendChild(g_mix, m3);

        // g-rep: width 340px, 3 cols repeat(3, 1fr), gap: 10px 20px (row=10, col=20)
        // Each col = (340 - 2*20) / 3 = 100px; col offsets = [0, 120, 240]
        // grid-template-rows: 30px 50px 50px => explicit total height = 30 + 10 + 50 + 10 + 50 = 150px
        // g-mix: width 280px, cols: 80px auto 1fr, rows: repeat(2, 50px), gap: 10px
        // remaining = 280 - 80 - 20 = 180px split across auto and 1fr => 90px each; col offsets = [0, 90, 190]
        // explicit rows: 50 + 10 + 50 = 110px
        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 800px; }\n" ++
            "#g-rep { display: grid; width: 340px; grid-template-columns: repeat(3, 1fr); grid-template-rows: 30px 50px 50px; gap: 10px 20px; }\n" ++
            ".cell { height: 20px; }\n" ++
            "#g-mix { display: grid; width: 280px; grid-template-columns: 80px auto 1fr; grid-template-rows: repeat(2, 50px); gap: 10px; }\n" ++
            ".mcell { height: 25px; }\n";

        const sheet = StyleSheet.parse(css_text, 800.0, false);
        let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        const rr1 = layout.getBoundingClientRect(r1);
        expect_eq(rr1.x, 0.0);
        expect_eq(rr1.y, 0.0);
        expect_eq(rr1.width, 100.0);

        const rr2 = layout.getBoundingClientRect(r2);
        expect_eq(rr2.x, 120.0);
        expect_eq(rr2.y, 0.0);
        expect_eq(rr2.width, 100.0);

        const rr3 = layout.getBoundingClientRect(r3);
        expect_eq(rr3.x, 240.0);
        expect_eq(rr3.y, 0.0);
        expect_eq(rr3.width, 100.0);

        // Row 0 explicit height = 30px, row-gap = 10px => row 1 starts at y = 40
        const rr4 = layout.getBoundingClientRect(r4);
        expect_eq(rr4.x, 0.0);
        expect_eq(rr4.y, 40.0);
        expect_eq(rr4.width, 100.0);

        // g-rep auto height includes all 3 explicit rows: 30 + 10 + 50 + 10 + 50 = 150
        const r_grep = layout.getBoundingClientRect(g_rep);
        expect_eq(r_grep.height, 150.0);

        // g-mix starts at y = 150
        const rm1 = layout.getBoundingClientRect(m1);
        expect_eq(rm1.x, 0.0);
        expect_eq(rm1.y, 150.0);
        expect_eq(rm1.width, 80.0);

        const rm2 = layout.getBoundingClientRect(m2);
        expect_eq(rm2.x, 90.0);
        expect_eq(rm2.y, 150.0);
        expect_eq(rm2.width, 90.0);

        const rm3 = layout.getBoundingClientRect(m3);
        expect_eq(rm3.x, 190.0);
        expect_eq(rm3.y, 150.0);
        expect_eq(rm3.width, 90.0);

        const r_gmix = layout.getBoundingClientRect(g_mix);
        expect_eq(r_gmix.height, 110.0);
    });

    test("2. Grid edge cases: repeat(0), span clamping, overflow clamping, and grid-placed abspos", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;

        // Edge A: repeat(0, 100px) falls back to 1 column
        const g_zero = doc.createElementWithAttrs("div", "g-zero", "", "") catch unreachable;
        const gz1 = doc.createElementWithAttrs("div", "gz1", "", "") catch unreachable;
        const gz2 = doc.createElementWithAttrs("div", "gz2", "", "") catch unreachable;
        doc.appendChild(root, g_zero);
        doc.appendChild(g_zero, gz1);
        doc.appendChild(g_zero, gz2);

        // Edge B: fixed tracks exceed container width -> fr track clamps to 0.0 (never negative)
        const g_over = doc.createElementWithAttrs("div", "g-over", "", "") catch unreachable;
        const go1 = doc.createElementWithAttrs("div", "go1", "", "") catch unreachable;
        const go2 = doc.createElementWithAttrs("div", "go2", "", "") catch unreachable;
        doc.appendChild(root, g_over);
        doc.appendChild(g_over, go1);
        doc.appendChild(g_over, go2);

        // Edge C: span > cols clamped to cols; abspos child placed via grid lines
        const g_abs = doc.createElementWithAttrs("div", "g-abs", "", "") catch unreachable;
        const ga1 = doc.createElementWithAttrs("div", "ga1", "", "") catch unreachable;
        const ga_abs = doc.createElementWithAttrs("div", "ga-abs", "", "") catch unreachable;
        doc.appendChild(root, g_abs);
        doc.appendChild(g_abs, ga1);
        doc.appendChild(g_abs, ga_abs);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 500px; }\n" ++
            "#g-zero { display: grid; width: 200px; grid-template-columns: repeat(0, 100px); gap: 5px; }\n" ++
            "#gz1, #gz2 { height: 20px; }\n" ++
            "#g-over { display: grid; width: 100px; grid-template-columns: 150px 1fr; column-gap: 10px; }\n" ++
            "#go1, #go2 { height: 20px; }\n" ++
            "#g-abs { display: grid; position: relative; width: 320px; grid-template-columns: 100px 100px 100px; grid-template-rows: 40px 40px; gap: 10px; }\n" ++
            "#ga1 { grid-column: span 10; height: 40px; }\n" ++
            "#ga-abs { position: absolute; grid-column: 2 / 4; grid-row: 1 / 3; left: 0; top: 0; right: 0; bottom: 0; }\n";

        const sheet = StyleSheet.parse(css_text, 500.0, false);
        let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 500.0, 500.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // g-zero: 1 col (200px), gz2 wraps to row 1 at y = 25
        const rgz1 = layout.getBoundingClientRect(gz1);
        expect_eq(rgz1.width, 100.0);
        expect_eq(rgz1.y, 0.0);
        const rgz2 = layout.getBoundingClientRect(gz2);
        expect_eq(rgz2.x, 0.0);
        expect_eq(rgz2.y, 25.0);

        // g-over: go1 gets 150px, go2 (1fr) clamps to 0.0px
        const rgo1 = layout.getBoundingClientRect(go1);
        expect_eq(rgo1.width, 150.0);
        const rgo2 = layout.getBoundingClientRect(go2);
        expect_eq(rgo2.width, 0.0);

        // g-abs: ga1 spans all 3 cols => 100 + 10 + 100 + 10 + 100 = 320px
        const rga1 = layout.getBoundingClientRect(ga1);
        expect_eq(rga1.width, 320.0);

        // ga-abs: col 2..4 => x = g_abs.x + 110, width = 210; row 1..3 => y = g_abs.y, height = 40 + 10 + 40 = 90
        const rg_abs = layout.getBoundingClientRect(g_abs);
        const rga_abs = layout.getBoundingClientRect(ga_abs);
        expect_eq(rga_abs.x, rg_abs.x + 110.0);
        expect_eq(rga_abs.y, rg_abs.y);
        expect_eq(rga_abs.width, 210.0);
        expect_eq(rga_abs.height, 90.0);
    });

    test("3. Intrinsic aspect-ratio derivation on SVG viewBox & width/height auto", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;

        // svg1: viewBox only (200x100 => 2:1), fills parent 400px => 400x200
        const svg1 = doc.createElementWithAttrs("svg", "svg1", "", "") catch unreachable;
        doc.setAttribute(svg1, "viewBox", "0 0 200 100");
        doc.appendChild(root, svg1);

        // svg2: viewBox (160x90 => 16:9), CSS width: 320px, height: auto => 320x180
        const svg2 = doc.createElementWithAttrs("svg", "svg2", "", "") catch unreachable;
        doc.setAttribute(svg2, "viewBox", "0 0 160 90");
        doc.appendChild(root, svg2);

        // svg3: attrs width=200 height=100, CSS width: auto, height: 50px => 100x50
        const svg3 = doc.createElementWithAttrs("svg", "svg3", "", "") catch unreachable;
        doc.setAttribute(svg3, "width", "200");
        doc.setAttribute(svg3, "height", "100");
        doc.appendChild(root, svg3);

        // svg4: attr width=100 only + viewBox 200x50 => 100x25
        const svg4 = doc.createElementWithAttrs("svg", "svg4", "", "") catch unreachable;
        doc.setAttribute(svg4, "width", "100");
        doc.setAttribute(svg4, "viewBox", "0 0 200 50");
        doc.appendChild(root, svg4);

        // svg5: attr height=60 only + viewBox 200x50 => 240x60
        const svg5 = doc.createElementWithAttrs("svg", "svg5", "", "") catch unreachable;
        doc.setAttribute(svg5, "height", "60");
        doc.setAttribute(svg5, "viewBox", "0 0 200 50");
        doc.appendChild(root, svg5);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 400px; }\n" ++
            "#svg1, #svg2, #svg3, #svg4, #svg5 { display: block; }\n" ++
            "#svg2 { width: 320px; height: auto; }\n" ++
            "#svg3 { width: auto; height: 50px; }\n";

        const sheet = StyleSheet.parse(css_text, 800.0, false);
        let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 800.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        const r1 = layout.getBoundingClientRect(svg1);
        expect_eq(r1.width, 400.0);
        expect_eq(r1.height, 200.0);

        const r2 = layout.getBoundingClientRect(svg2);
        expect_eq(r2.width, 320.0);
        expect_eq(r2.height, 180.0);

        const r3 = layout.getBoundingClientRect(svg3);
        expect_eq(r3.width, 100.0);
        expect_eq(r3.height, 50.0);

        const r4 = layout.getBoundingClientRect(svg4);
        expect_eq(r4.width, 100.0);
        expect_eq(r4.height, 25.0);

        const r5 = layout.getBoundingClientRect(svg5);
        expect_eq(r5.width, 240.0);
        expect_eq(r5.height, 60.0);
    });

    test("4. Zero, negative, and invalid aspect-ratio / viewBox / dimension edge cases", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;

        // Zero width in viewBox -> falls back to default 300x150
        const z_w = doc.createElementWithAttrs("svg", "z-w", "", "") catch unreachable;
        doc.setAttribute(z_w, "viewBox", "0 0 0 100");
        doc.appendChild(root, z_w);

        // Zero height in viewBox -> falls back to default 300x150
        const z_h = doc.createElementWithAttrs("svg", "z-h", "", "") catch unreachable;
        doc.setAttribute(z_h, "viewBox", "0 0 100 0");
        doc.appendChild(root, z_h);

        // Negative width in viewBox -> falls back to default 300x150
        const neg_w = doc.createElementWithAttrs("svg", "neg-w", "", "") catch unreachable;
        doc.setAttribute(neg_w, "viewBox", "0 0 -50 100");
        doc.appendChild(root, neg_w);

        // Invalid non-numeric viewBox -> falls back to default 300x150
        const bad_vb = doc.createElementWithAttrs("svg", "bad-vb", "", "") catch unreachable;
        doc.setAttribute(bad_vb, "viewBox", "0 0 abc def");
        doc.appendChild(root, bad_vb);

        // Zero & negative width/height attributes -> ignored, falls back to default 300x150
        const bad_dims = doc.createElementWithAttrs("svg", "bad-dims", "", "") catch unreachable;
        doc.setAttribute(bad_dims, "width", "0");
        doc.setAttribute(bad_dims, "height", "-20");
        doc.appendChild(root, bad_dims);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 500px; }\n" ++
            "svg { display: block; }\n";

        const sheet = StyleSheet.parse(css_text, 800.0, false);
        let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 900.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        const r_zw = layout.getBoundingClientRect(z_w);
        expect_eq(r_zw.width, 300.0);
        expect_eq(r_zw.height, 150.0);

        const r_zh = layout.getBoundingClientRect(z_h);
        expect_eq(r_zh.width, 300.0);
        expect_eq(r_zh.height, 150.0);

        const r_neg = layout.getBoundingClientRect(neg_w);
        expect_eq(r_neg.width, 300.0);
        expect_eq(r_neg.height, 150.0);

        const r_bad_vb = layout.getBoundingClientRect(bad_vb);
        expect_eq(r_bad_vb.width, 300.0);
        expect_eq(r_bad_vb.height, 150.0);

        const r_bad_dims = layout.getBoundingClientRect(bad_dims);
        expect_eq(r_bad_dims.width, 300.0);
        expect_eq(r_bad_dims.height, 150.0);
    });
}
