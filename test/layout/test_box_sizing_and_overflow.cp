// test/layout/test_box_sizing_and_overflow.cp
// Pure CP white-box unit tests for LayoutTree:
// 1. Box-sizing (content-box vs border-box with padding & border)
// 2. Display none vs visibility hidden (Rect.visible, size, sibling flow)
// 3. Position relative & absolute (top/left/right/bottom offsets)
// 4. Margin (auto centering, negative margin) & min-width/max-width constraints

import { test, expect_eq } from "cap:test";
import { Document, NodeId } from "../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";
import { LayoutTree, LayoutBox, Rect } from "../../src/layout/layout.cp";

fn main() {
    test("1. Box-sizing: content-box vs border-box with padding and border", () => {
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
            "#cb { box-sizing: content-box; width: 120px; height: 60px; padding: 10px 20px; border: 5px solid #000; }\n" ++
            "#bb { box-sizing: border-box; width: 120px; height: 60px; padding: 10px 20px; border: 5px solid #000; }\n";

        const sheet = StyleSheet.parse(css_text, 500.0, false);
        let styles: [8]ComputedStyle = [ComputedStyle{}; 8];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 500.0, 400.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // content-box: border-box = 120 + 40 + 10 = 170, 60 + 20 + 10 = 90
        const r_cb = layout.getBoundingClientRect(cb);
        expect_eq(r_cb.width, 170.0);
        expect_eq(r_cb.height, 90.0);
        expect_eq(layout.boxes.items[@as(usize, cb)].content_width, 120.0);
        expect_eq(layout.boxes.items[@as(usize, cb)].content_height, 60.0);

        // border-box: border-box = 120 x 60, content = 70 x 30, positioned below cb at y = 90
        const r_bb = layout.getBoundingClientRect(bb);
        expect_eq(r_bb.width, 120.0);
        expect_eq(r_bb.height, 60.0);
        expect_eq(r_bb.y, 90.0);
        expect_eq(layout.boxes.items[@as(usize, bb)].content_width, 70.0);
        expect_eq(layout.boxes.items[@as(usize, bb)].content_height, 30.0);
    });

    test("2. Display none vs visibility hidden layout space and Rect.visible", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;
        const d_none = doc.createElementWithAttrs("div", "d-none", "", "") catch unreachable;
        const v_hidden = doc.createElementWithAttrs("div", "v-hidden", "", "") catch unreachable;
        const after = doc.createElementWithAttrs("div", "after", "", "") catch unreachable;

        doc.appendChild(root, d_none);
        doc.appendChild(root, v_hidden);
        doc.appendChild(root, after);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 400px; }\n" ++
            "#d-none { display: none; width: 100px; height: 80px; }\n" ++
            "#v-hidden { visibility: hidden; width: 100px; height: 50px; }\n" ++
            "#after { width: 100px; height: 30px; }\n";

        const sheet = StyleSheet.parse(css_text, 400.0, false);
        let styles: [8]ComputedStyle = [ComputedStyle{}; 8];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 400.0, 400.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // display: none has zero size, visible=false, and takes no space
        const r_none = layout.getBoundingClientRect(d_none);
        expect_eq(r_none.width, 0.0);
        expect_eq(r_none.height, 0.0);
        expect_eq(r_none.visible, false);
        expect_eq(r_none.isClickable(), false);

        // visibility: hidden starts at y=0 (since #d-none takes no space), keeps 100x50 box, visible=false
        const r_hidden = layout.getBoundingClientRect(v_hidden);
        expect_eq(r_hidden.x, 0.0);
        expect_eq(r_hidden.y, 0.0);
        expect_eq(r_hidden.width, 100.0);
        expect_eq(r_hidden.height, 50.0);
        expect_eq(r_hidden.visible, false);
        expect_eq(r_hidden.isClickable(), false);

        // #after is pushed down by #v-hidden's 50px height
        const r_after = layout.getBoundingClientRect(after);
        expect_eq(r_after.x, 0.0);
        expect_eq(r_after.y, 50.0);
        expect_eq(r_after.width, 100.0);
        expect_eq(r_after.height, 30.0);
        expect_eq(r_after.visible, true);
    });

    test("3. Position relative and position absolute (top/left/right/bottom)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const container = doc.createElementWithAttrs("div", "container", "", "") catch unreachable;
        const rel1 = doc.createElementWithAttrs("div", "rel1", "", "") catch unreachable;
        const rel2 = doc.createElementWithAttrs("div", "rel2", "", "") catch unreachable;
        const abs_tl = doc.createElementWithAttrs("div", "abs-tl", "", "") catch unreachable;
        const abs_rb = doc.createElementWithAttrs("div", "abs-rb", "", "") catch unreachable;

        doc.appendChild(container, rel1);
        doc.appendChild(container, rel2);
        doc.appendChild(container, abs_tl);
        doc.appendChild(container, abs_rb);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#container { position: relative; width: 400px; height: 300px; }\n" ++
            "#rel1 { position: relative; top: 15px; left: 25px; width: 80px; height: 40px; }\n" ++
            "#rel2 { position: relative; right: 10px; bottom: 5px; width: 80px; height: 40px; }\n" ++
            "#abs-tl { position: absolute; top: 20px; left: 30px; width: 60px; height: 50px; }\n" ++
            "#abs-rb { position: absolute; right: 15px; bottom: 25px; width: 60px; height: 50px; }\n";

        const sheet = StyleSheet.parse(css_text, 800.0, false);
        let styles: [8]ComputedStyle = [ComputedStyle{}; 8];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // rel1: static (0, 0) + (25, 15) = (25, 15)
        const r_rel1 = layout.getBoundingClientRect(rel1);
        expect_eq(r_rel1.x, 25.0);
        expect_eq(r_rel1.y, 15.0);
        expect_eq(r_rel1.width, 80.0);
        expect_eq(r_rel1.height, 40.0);

        // rel2: static (0, 40) + (-10, -5) = (-10, 35)
        const r_rel2 = layout.getBoundingClientRect(rel2);
        expect_eq(r_rel2.x, -10.0);
        expect_eq(r_rel2.y, 35.0);

        // abs-tl: relative to container (400x300 at 0,0) -> (30, 20)
        const r_abs_tl = layout.getBoundingClientRect(abs_tl);
        expect_eq(r_abs_tl.x, 30.0);
        expect_eq(r_abs_tl.y, 20.0);
        expect_eq(r_abs_tl.width, 60.0);
        expect_eq(r_abs_tl.height, 50.0);

        // abs-rb: x = 400 - 15 - 60 = 325, y = 300 - 25 - 50 = 225
        const r_abs_rb = layout.getBoundingClientRect(abs_rb);
        expect_eq(r_abs_rb.x, 325.0);
        expect_eq(r_abs_rb.y, 225.0);
    });

    test("4. Margin auto centering, negative margin, and min-width/max-width constraints", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;
        const centered = doc.createElementWithAttrs("div", "centered", "", "") catch unreachable;
        const neg = doc.createElementWithAttrs("div", "neg", "", "") catch unreachable;
        const min_w = doc.createElementWithAttrs("div", "min-w", "", "") catch unreachable;
        const max_w = doc.createElementWithAttrs("div", "max-w", "", "") catch unreachable;

        doc.appendChild(root, centered);
        doc.appendChild(root, neg);
        doc.appendChild(root, min_w);
        doc.appendChild(root, max_w);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 400px; }\n" ++
            "#centered { width: 100px; height: 40px; margin: 0 auto; }\n" ++
            "#neg { width: 100px; height: 40px; margin-left: -20px; margin-top: -10px; }\n" ++
            "#min-w { width: 50px; min-width: 120px; height: 30px; }\n" ++
            "#max-w { width: 300px; max-width: 180px; height: 30px; }\n";

        const sheet = StyleSheet.parse(css_text, 400.0, false);
        let styles: [8]ComputedStyle = [ComputedStyle{}; 8];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 400.0, 400.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // centered: (400 - 100) / 2 = 150
        const r_centered = layout.getBoundingClientRect(centered);
        expect_eq(r_centered.x, 150.0);
        expect_eq(r_centered.y, 0.0);
        expect_eq(r_centered.width, 100.0);

        // neg: x = -20, y = 40 - 10 = 30
        const r_neg = layout.getBoundingClientRect(neg);
        expect_eq(r_neg.x, -20.0);
        expect_eq(r_neg.y, 30.0);

        // min-w: y = 30 + 40 = 70, width clamped up to 120
        const r_min_w = layout.getBoundingClientRect(min_w);
        expect_eq(r_min_w.y, 70.0);
        expect_eq(r_min_w.width, 120.0);

        // max-w: y = 70 + 30 = 100, width clamped down to 180
        const r_max_w = layout.getBoundingClientRect(max_w);
        expect_eq(r_max_w.y, 100.0);
        expect_eq(r_max_w.width, 180.0);
    });
}
