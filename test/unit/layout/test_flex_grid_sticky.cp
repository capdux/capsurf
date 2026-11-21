// test/layout/test_flex_grid_sticky.cp
// Pure CP white-box unit tests for LayoutTree:
// 1. Flexbox (flex-wrap, justify-content, align-items, gap)
// 2. CSS Grid (grid-template-columns, 1fr/px tracks, column/row-gap, grid-column span)
// 3. Box Model (box-sizing: content-box vs border-box)
// 4. Positioning & Visibility (position: sticky, position: fixed, display: none)

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle, CssPosition } from "../../../src/css/cascade.cp";
import { LayoutTree, LayoutBox, Rect } from "../../../src/layout/layout.cp";

fn main() {
    test("1. Flexbox wrap, justify-content, align-items, and gap", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;
        const wrap_box = doc.createElementWithAttrs("div", "wrap-box", "", "") catch unreachable;
        const w1 = doc.createElementWithAttrs("div", "w1", "w-item", "") catch unreachable;
        const w2 = doc.createElementWithAttrs("div", "w2", "w-item", "") catch unreachable;
        const w3 = doc.createElementWithAttrs("div", "w3", "w-item", "") catch unreachable;

        doc.appendChild(root, wrap_box);
        doc.appendChild(wrap_box, w1);
        doc.appendChild(wrap_box, w2);
        doc.appendChild(wrap_box, w3);

        const align_box = doc.createElementWithAttrs("div", "align-box", "", "") catch unreachable;
        const a1 = doc.createElementWithAttrs("div", "a1", "", "") catch unreachable;
        const a2 = doc.createElementWithAttrs("div", "a2", "", "") catch unreachable;

        doc.appendChild(root, align_box);
        doc.appendChild(align_box, a1);
        doc.appendChild(align_box, a2);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 800px; }\n" ++
            "#wrap-box { display: flex; flex-wrap: wrap; width: 300px; column-gap: 10px; row-gap: 20px; }\n" ++
            ".w-item { width: 120px; height: 40px; }\n" ++
            "#align-box { display: flex; justify-content: space-between; align-items: center; width: 300px; height: 100px; }\n" ++
            "#a1 { width: 50px; height: 20px; }\n" ++
            "#a2 { width: 50px; height: 40px; }\n";

        const sheet = StyleSheet.parse(css_text, 800.0, false);
        let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // Line 1: w1 at (0, 0), w2 at (130, 0); 120 + 10 + 120 + 10 + 120 = 380 > 300 => w3 wraps!
        const r_w1 = layout.getBoundingClientRect(w1);
        expect_eq(r_w1.x, 0.0);
        expect_eq(r_w1.y, 0.0);
        expect_eq(r_w1.width, 120.0);
        expect_eq(r_w1.height, 40.0);

        const r_w2 = layout.getBoundingClientRect(w2);
        expect_eq(r_w2.x, 130.0);
        expect_eq(r_w2.y, 0.0);

        // Line 2: w3 at (0, 40 + 20 = 60)
        const r_w3 = layout.getBoundingClientRect(w3);
        expect_eq(r_w3.x, 0.0);
        expect_eq(r_w3.y, 60.0);

        // #wrap-box total height = 40 + 20 + 40 = 100, so #align-box starts at y = 100
        const r_align = layout.getBoundingClientRect(align_box);
        expect_eq(r_align.y, 100.0);

        // justify-content: space-between & align-items: center inside 300x100 box
        // a1 (50x20): x = 0, y = 100 + (100 - 20)/2 = 140
        const r_a1 = layout.getBoundingClientRect(a1);
        expect_eq(r_a1.x, 0.0);
        expect_eq(r_a1.y, 140.0);

        // a2 (50x40): x = 300 - 50 = 250, y = 100 + (100 - 40)/2 = 130
        const r_a2 = layout.getBoundingClientRect(a2);
        expect_eq(r_a2.x, 250.0);
        expect_eq(r_a2.y, 130.0);
    });

    test("2. CSS Grid track sizing (px + 1fr), gaps, and column span", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const grid = doc.createElementWithAttrs("div", "grid", "", "") catch unreachable;
        const g1 = doc.createElementWithAttrs("div", "g1", "", "") catch unreachable;
        const g2 = doc.createElementWithAttrs("div", "g2", "", "") catch unreachable;
        const g3 = doc.createElementWithAttrs("div", "g3", "", "") catch unreachable;

        doc.appendChild(grid, g1);
        doc.appendChild(grid, g2);
        doc.appendChild(grid, g3);

        // 320px width, 3 columns (100px 1fr 1fr), column-gap 10px (total gaps 20px) => 1fr = 100px
        // col_offsets: [0, 110, 220]
        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#grid { display: grid; width: 320px; grid-template-columns: 100px 1fr 1fr; column-gap: 10px; row-gap: 15px; }\n" ++
            "#g1 { height: 30px; }\n" ++
            "#g2 { grid-column: span 2; height: 40px; }\n" ++
            "#g3 { height: 25px; }\n";

        const sheet = StyleSheet.parse(css_text, 800.0, false);
        let styles: [8]ComputedStyle = [ComputedStyle{}; 8];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // g1 in col 0: x=0, y=0, w=100, h=30
        const r_g1 = layout.getBoundingClientRect(g1);
        expect_eq(r_g1.x, 0.0);
        expect_eq(r_g1.y, 0.0);
        expect_eq(r_g1.width, 100.0);
        expect_eq(r_g1.height, 30.0);

        // g2 spans col 1 & 2: x=110, y=0, w=100+10+100=210, h=40
        const r_g2 = layout.getBoundingClientRect(g2);
        expect_eq(r_g2.x, 110.0);
        expect_eq(r_g2.y, 0.0);
        expect_eq(r_g2.width, 210.0);
        expect_eq(r_g2.height, 40.0);

        // Row 1 max height is 40px, row-gap is 15px => Row 2 starts at y=55
        const r_g3 = layout.getBoundingClientRect(g3);
        expect_eq(r_g3.x, 0.0);
        expect_eq(r_g3.y, 55.0);
        expect_eq(r_g3.width, 100.0);
        expect_eq(r_g3.height, 25.0);
    });

    test("3. Box-sizing: content-box vs border-box", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;
        const cb = doc.createElementWithAttrs("div", "cb", "", "") catch unreachable;
        const bb = doc.createElementWithAttrs("div", "bb", "", "") catch unreachable;

        doc.appendChild(root, cb);
        doc.appendChild(root, bb);

        const css_text =
            "* { margin: 0; }\n" ++
            "#root { width: 500px; }\n" ++
            "#cb { box-sizing: content-box; width: 100px; height: 50px; padding: 10px; border: 5px solid #000; }\n" ++
            "#bb { box-sizing: border-box; width: 100px; height: 50px; padding: 10px; border: 5px solid #000; }\n";

        const sheet = StyleSheet.parse(css_text, 500.0, false);
        let styles: [8]ComputedStyle = [ComputedStyle{}; 8];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 500.0, 400.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // content-box: border-box = 100 + 20 + 10 = 130, 50 + 20 + 10 = 80
        const r_cb = layout.getBoundingClientRect(cb);
        expect_eq(r_cb.width, 130.0);
        expect_eq(r_cb.height, 80.0);
        expect_eq(layout.boxes.items[@as(usize, cb)].content_width, 100.0);
        expect_eq(layout.boxes.items[@as(usize, cb)].content_height, 50.0);

        // border-box: border-box = 100 x 50, content = 70 x 20
        const r_bb = layout.getBoundingClientRect(bb);
        expect_eq(r_bb.width, 100.0);
        expect_eq(r_bb.height, 50.0);
        expect_eq(r_bb.y, 80.0);
        expect_eq(layout.boxes.items[@as(usize, bb)].content_width, 70.0);
        expect_eq(layout.boxes.items[@as(usize, bb)].content_height, 20.0);
    });

    test("4. CssPosition sticky, position fixed, and display none", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;
        const sticky_el = doc.createElementWithAttrs("div", "sticky-el", "", "") catch unreachable;
        const fixed_el = doc.createElementWithAttrs("div", "fixed-el", "", "") catch unreachable;
        const hidden_el = doc.createElementWithAttrs("div", "hidden-el", "", "") catch unreachable;
        const hidden_child = doc.createElementWithAttrs("div", "hidden-child", "", "") catch unreachable;

        doc.appendChild(root, sticky_el);
        doc.appendChild(root, fixed_el);
        doc.appendChild(root, hidden_el);
        doc.appendChild(hidden_el, hidden_child);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 1280px; height: 800px; }\n" ++
            "#sticky-el { position: sticky; top: 12px; left: 8px; width: 100px; height: 30px; }\n" ++
            "#fixed-el { position: fixed; right: 20px; bottom: 10px; width: 100px; height: 40px; }\n" ++
            "#hidden-el { display: none; width: 200px; height: 200px; }\n" ++
            "#hidden-child { width: 50px; height: 50px; }\n";

        const sheet = StyleSheet.parse(css_text, 1280.0, false);
        let styles: [8]ComputedStyle = [ComputedStyle{}; 8];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 1280.0, 800.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // Sticky metadata & static-flow box
        const sbox: LayoutBox = layout.boxes.items[@as(usize, sticky_el)];
        expect_eq(sbox.position, CssPosition.Sticky);
        expect_eq(sbox.has_sticky_top, true);
        expect_eq(sbox.sticky_top, 12.0);
        expect_eq(sbox.has_sticky_left, true);
        expect_eq(sbox.sticky_left, 8.0);
        expect_eq(sbox.rect.width, 100.0);
        expect_eq(sbox.rect.height, 30.0);

        // Fixed element anchored to viewport (1280x800): x = 1280-20-100 = 1160, y = 800-10-40 = 750
        const r_fixed = layout.getBoundingClientRect(fixed_el);
        expect_eq(r_fixed.x, 1160.0);
        expect_eq(r_fixed.y, 750.0);
        expect_eq(r_fixed.width, 100.0);
        expect_eq(r_fixed.height, 40.0);
        expect_eq(r_fixed.visible, true);

        // display: none zeros both container and descendant
        const r_hidden = layout.getBoundingClientRect(hidden_el);
        expect_eq(r_hidden.width, 0.0);
        expect_eq(r_hidden.height, 0.0);
        expect_eq(r_hidden.visible, false);
        expect_eq(r_hidden.isClickable(), false);

        const r_hidden_child = layout.getBoundingClientRect(hidden_child);
        expect_eq(r_hidden_child.width, 0.0);
        expect_eq(r_hidden_child.height, 0.0);
        expect_eq(r_hidden_child.visible, false);
    });
}
