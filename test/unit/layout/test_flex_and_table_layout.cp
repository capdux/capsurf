// test/layout/test_flex_and_table_layout.cp
// Pure CP white-box unit tests for LayoutTree:
// 1. Flexbox direction (row vs column) and gap (row-gap / column-gap)
// 2. Flexbox justify-content (flex-start, flex-end, center, space-between, space-around, space-evenly)
// 3. Flexbox align-items (flex-start, center, flex-end, stretch)
// 4. Flexbox flex-wrap: wrap with row-gap & column-gap
// 5. Flexbox flex-grow & flex-shrink distribution (including min-width clamp)
// 6. Position: fixed (viewport-relative offsets and out-of-flow behavior)
// 7. Percentage width/height boundaries (definite vs indefinite auto parent height)

import { test, expect_eq } from "cap:test";
import { Document, NodeId } from "../../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../../src/css/cascade.cp";
import { LayoutTree, LayoutBox, Rect } from "../../../src/layout/layout.cp";

fn main() {
    test("1. Flexbox direction (row vs column) and gap", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;
        const row_box = doc.createElementWithAttrs("div", "row-box", "", "") catch unreachable;
        const r1 = doc.createElementWithAttrs("div", "r1", "", "") catch unreachable;
        const r2 = doc.createElementWithAttrs("div", "r2", "", "") catch unreachable;
        const col_box = doc.createElementWithAttrs("div", "col-box", "", "") catch unreachable;
        const c1 = doc.createElementWithAttrs("div", "c1", "", "") catch unreachable;
        const c2 = doc.createElementWithAttrs("div", "c2", "", "") catch unreachable;

        doc.appendChild(root, row_box);
        doc.appendChild(row_box, r1);
        doc.appendChild(row_box, r2);
        doc.appendChild(root, col_box);
        doc.appendChild(col_box, c1);
        doc.appendChild(col_box, c2);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 400px; }\n" ++
            "#row-box { display: flex; flex-direction: row; column-gap: 15px; width: 400px; height: 50px; }\n" ++
            "#r1, #r2 { width: 60px; height: 30px; }\n" ++
            "#col-box { display: flex; flex-direction: column; row-gap: 12px; width: 400px; }\n" ++
            "#c1, #c2 { width: 80px; height: 25px; }\n";

        const sheet = StyleSheet.parse(css_text, 400.0, false);
        let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 400.0, 400.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        const rr1 = layout.getBoundingClientRect(r1);
        const rr2 = layout.getBoundingClientRect(r2);
        expect_eq(rr1.x, 0.0);
        expect_eq(rr1.y, 0.0);
        expect_eq(rr2.x, 75.0);
        expect_eq(rr2.y, 0.0);

        const rc1 = layout.getBoundingClientRect(c1);
        const rc2 = layout.getBoundingClientRect(c2);
        expect_eq(rc1.x, 0.0);
        expect_eq(rc1.y, 50.0);
        expect_eq(rc2.x, 0.0);
        expect_eq(rc2.y, 87.0);

        // col-box has auto height: 25 + 12 + 25 = 62
        const r_col_box = layout.getBoundingClientRect(col_box);
        expect_eq(r_col_box.height, 62.0);
    });

    test("2. Flexbox justify-content modes", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;

        const c_end = doc.createElementWithAttrs("div", "c-end", "", "") catch unreachable;
        const e1 = doc.createElementWithAttrs("div", "e1", "", "") catch unreachable;
        const e2 = doc.createElementWithAttrs("div", "e2", "", "") catch unreachable;

        const c_center = doc.createElementWithAttrs("div", "c-center", "", "") catch unreachable;
        const m1 = doc.createElementWithAttrs("div", "m1", "", "") catch unreachable;
        const m2 = doc.createElementWithAttrs("div", "m2", "", "") catch unreachable;

        const c_between = doc.createElementWithAttrs("div", "c-between", "", "") catch unreachable;
        const b1 = doc.createElementWithAttrs("div", "b1", "", "") catch unreachable;
        const b2 = doc.createElementWithAttrs("div", "b2", "", "") catch unreachable;
        const b3 = doc.createElementWithAttrs("div", "b3", "", "") catch unreachable;

        const c_around = doc.createElementWithAttrs("div", "c-around", "", "") catch unreachable;
        const a1 = doc.createElementWithAttrs("div", "a1", "", "") catch unreachable;
        const a2 = doc.createElementWithAttrs("div", "a2", "", "") catch unreachable;

        const c_evenly = doc.createElementWithAttrs("div", "c-evenly", "", "") catch unreachable;
        const v1 = doc.createElementWithAttrs("div", "v1", "", "") catch unreachable;
        const v2 = doc.createElementWithAttrs("div", "v2", "", "") catch unreachable;

        doc.appendChild(root, c_end);
        doc.appendChild(c_end, e1);
        doc.appendChild(c_end, e2);

        doc.appendChild(root, c_center);
        doc.appendChild(c_center, m1);
        doc.appendChild(c_center, m2);

        doc.appendChild(root, c_between);
        doc.appendChild(c_between, b1);
        doc.appendChild(c_between, b2);
        doc.appendChild(c_between, b3);

        doc.appendChild(root, c_around);
        doc.appendChild(c_around, a1);
        doc.appendChild(c_around, a2);

        doc.appendChild(root, c_evenly);
        doc.appendChild(c_evenly, v1);
        doc.appendChild(c_evenly, v2);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 300px; }\n" ++
            "#c-end { display: flex; justify-content: flex-end; width: 300px; height: 20px; }\n" ++
            "#c-center { display: flex; justify-content: center; width: 300px; height: 20px; }\n" ++
            "#c-between { display: flex; justify-content: space-between; width: 300px; height: 20px; }\n" ++
            "#c-around { display: flex; justify-content: space-around; width: 300px; height: 20px; }\n" ++
            "#c-evenly { display: flex; justify-content: space-evenly; width: 300px; height: 20px; }\n" ++
            "#e1, #e2, #m1, #m2 { width: 50px; height: 20px; }\n" ++
            "#b1, #b2, #b3 { width: 50px; height: 20px; }\n" ++
            "#a1, #a2, #v1, #v2 { width: 50px; height: 20px; }\n";

        const sheet = StyleSheet.parse(css_text, 300.0, false);
        let styles: [24]ComputedStyle = [ComputedStyle{}; 24];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 300.0, 400.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // flex-end: remaining = 200 -> 200, 250
        expect_eq(layout.getBoundingClientRect(e1).x, 200.0);
        expect_eq(layout.getBoundingClientRect(e2).x, 250.0);

        // center: remaining = 200 -> 100, 150
        expect_eq(layout.getBoundingClientRect(m1).x, 100.0);
        expect_eq(layout.getBoundingClientRect(m2).x, 150.0);

        // space-between (3 items of 50px in 300px): remaining = 150, step = 75 -> 0, 125, 250
        expect_eq(layout.getBoundingClientRect(b1).x, 0.0);
        expect_eq(layout.getBoundingClientRect(b2).x, 125.0);
        expect_eq(layout.getBoundingClientRect(b3).x, 250.0);

        // space-around (2 items of 50px in 300px): remaining = 200 -> start = 50, step = 100 -> 50, 200
        expect_eq(layout.getBoundingClientRect(a1).x, 50.0);
        expect_eq(layout.getBoundingClientRect(a2).x, 200.0);

        // space-evenly (2 items of 50px in 300px): remaining = 200 -> gap = 200/3 -> 66.666..., 183.333...
        const rv1 = layout.getBoundingClientRect(v1);
        const rv2 = layout.getBoundingClientRect(v2);
        const expected_gap: f32 = 200.0 / 3.0;
        expect_eq(rv1.x, expected_gap);
        expect_eq(rv2.x, expected_gap + 50.0 + expected_gap);
    });

    test("3. Flexbox align-items (flex-start, center, flex-end, stretch)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;
        const c_start = doc.createElementWithAttrs("div", "c-start", "", "") catch unreachable;
        const s1 = doc.createElementWithAttrs("div", "s1", "", "") catch unreachable;
        const c_center = doc.createElementWithAttrs("div", "c-center", "", "") catch unreachable;
        const m1 = doc.createElementWithAttrs("div", "m1", "", "") catch unreachable;
        const c_end = doc.createElementWithAttrs("div", "c-end", "", "") catch unreachable;
        const e1 = doc.createElementWithAttrs("div", "e1", "", "") catch unreachable;
        const c_stretch = doc.createElementWithAttrs("div", "c-stretch", "", "") catch unreachable;
        const st1 = doc.createElementWithAttrs("div", "st1", "", "") catch unreachable;

        doc.appendChild(root, c_start);
        doc.appendChild(c_start, s1);
        doc.appendChild(root, c_center);
        doc.appendChild(c_center, m1);
        doc.appendChild(root, c_end);
        doc.appendChild(c_end, e1);
        doc.appendChild(root, c_stretch);
        doc.appendChild(c_stretch, st1);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 300px; }\n" ++
            "#c-start { display: flex; align-items: flex-start; width: 300px; height: 100px; }\n" ++
            "#c-center { display: flex; align-items: center; width: 300px; height: 100px; }\n" ++
            "#c-end { display: flex; align-items: flex-end; width: 300px; height: 100px; }\n" ++
            "#c-stretch { display: flex; align-items: stretch; width: 300px; height: 100px; }\n" ++
            "#s1, #m1, #e1 { width: 50px; height: 30px; }\n" ++
            "#st1 { width: 50px; }\n";

        const sheet = StyleSheet.parse(css_text, 300.0, false);
        let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 300.0, 500.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // c-start at y=0 -> s1 at y=0
        expect_eq(layout.getBoundingClientRect(s1).y, 0.0);
        // c-center at y=100 -> m1 at 100 + (100 - 30)/2 = 135
        expect_eq(layout.getBoundingClientRect(m1).y, 135.0);
        // c-end at y=200 -> e1 at 200 + (100 - 30) = 270
        expect_eq(layout.getBoundingClientRect(e1).y, 270.0);
        // c-stretch at y=300 -> st1 stretched to height=100
        const r_st1 = layout.getBoundingClientRect(st1);
        expect_eq(r_st1.y, 300.0);
        expect_eq(r_st1.height, 100.0);
    });

    test("4. Flexbox flex-wrap: wrap with row-gap and column-gap", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const wrap_box = doc.createElementWithAttrs("div", "wrap-box", "", "") catch unreachable;
        const i1 = doc.createElementWithAttrs("div", "i1", "", "") catch unreachable;
        const i2 = doc.createElementWithAttrs("div", "i2", "", "") catch unreachable;
        const i3 = doc.createElementWithAttrs("div", "i3", "", "") catch unreachable;

        doc.appendChild(wrap_box, i1);
        doc.appendChild(wrap_box, i2);
        doc.appendChild(wrap_box, i3);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#wrap-box { display: flex; flex-wrap: wrap; column-gap: 10px; row-gap: 20px; width: 200px; }\n" ++
            "#i1, #i2, #i3 { width: 90px; height: 40px; }\n";

        const sheet = StyleSheet.parse(css_text, 400.0, false);
        let styles: [8]ComputedStyle = [ComputedStyle{}; 8];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 400.0, 400.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // Line 1: i1 (x=0, y=0), i2 (x=100, y=0) -> 90 + 10 + 90 = 190 <= 200
        // Line 2: i3 (x=0, y=60) -> 40 + row-gap(20) = 60
        const ri1 = layout.getBoundingClientRect(i1);
        const ri2 = layout.getBoundingClientRect(i2);
        const ri3 = layout.getBoundingClientRect(i3);

        expect_eq(ri1.x, 0.0);
        expect_eq(ri1.y, 0.0);
        expect_eq(ri2.x, 100.0);
        expect_eq(ri2.y, 0.0);
        expect_eq(ri3.x, 0.0);
        expect_eq(ri3.y, 60.0);

        // Auto height of wrap-box: 40 + 20 + 40 = 100
        expect_eq(layout.getBoundingClientRect(wrap_box).height, 100.0);
    });

    test("5. Flexbox flex-grow and flex-shrink with min-width", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;
        const grow_box = doc.createElementWithAttrs("div", "grow-box", "", "") catch unreachable;
        const g1 = doc.createElementWithAttrs("div", "g1", "", "") catch unreachable;
        const g2 = doc.createElementWithAttrs("div", "g2", "", "") catch unreachable;
        const shrink_box = doc.createElementWithAttrs("div", "shrink-box", "", "") catch unreachable;
        const k1 = doc.createElementWithAttrs("div", "k1", "", "") catch unreachable;
        const k2 = doc.createElementWithAttrs("div", "k2", "", "") catch unreachable;

        doc.appendChild(root, grow_box);
        doc.appendChild(grow_box, g1);
        doc.appendChild(grow_box, g2);
        doc.appendChild(root, shrink_box);
        doc.appendChild(shrink_box, k1);
        doc.appendChild(shrink_box, k2);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 400px; }\n" ++
            "#grow-box { display: flex; width: 400px; height: 40px; }\n" ++
            "#g1 { flex-basis: 100px; height: 40px; flex-grow: 1; }\n" ++
            "#g2 { flex-basis: 100px; height: 40px; flex-grow: 3; }\n" ++
            "#shrink-box { display: flex; width: 200px; height: 40px; }\n" ++
            "#k1 { flex-basis: 150px; min-width: 120px; height: 40px; flex-shrink: 1; }\n" ++
            "#k2 { flex-basis: 150px; height: 40px; flex-shrink: 1; }\n";

        const sheet = StyleSheet.parse(css_text, 400.0, false);
        let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 400.0, 400.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // grow-box: free = 400 - 200 = 200; g1 gets +50 -> 150, g2 gets +150 -> 250
        const rg1 = layout.getBoundingClientRect(g1);
        const rg2 = layout.getBoundingClientRect(g2);
        expect_eq(rg1.width, 150.0);
        expect_eq(rg2.width, 250.0);
        expect_eq(rg2.x, 150.0);

        // shrink-box: free = 200 - 300 = -100; equal shrink -> 100 each, but k1 clamped to min-width 120
        const rk1 = layout.getBoundingClientRect(k1);
        const rk2 = layout.getBoundingClientRect(k2);
        expect_eq(rk1.width, 120.0);
        expect_eq(rk2.width, 100.0);
        expect_eq(rk2.x, 120.0);
    });

    test("6. Position: fixed viewport-relative offsets and out-of-flow", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;
        const container = doc.createElementWithAttrs("div", "container", "", "") catch unreachable;
        const fix_tl = doc.createElementWithAttrs("div", "fix-tl", "", "") catch unreachable;
        const fix_rb = doc.createElementWithAttrs("div", "fix-rb", "", "") catch unreachable;
        const in_flow = doc.createElementWithAttrs("div", "in-flow", "", "") catch unreachable;

        doc.appendChild(root, container);
        doc.appendChild(container, fix_tl);
        doc.appendChild(container, fix_rb);
        doc.appendChild(container, in_flow);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 800px; }\n" ++
            "#container { position: relative; margin-top: 100px; margin-left: 50px; width: 300px; height: 200px; }\n" ++
            "#fix-tl { position: fixed; top: 10px; left: 20px; width: 80px; height: 30px; }\n" ++
            "#fix-rb { position: fixed; right: 25px; bottom: 15px; width: 60px; height: 40px; }\n" ++
            "#in-flow { width: 100px; height: 50px; }\n";

        const sheet = StyleSheet.parse(css_text, 800.0, false);
        let styles: [8]ComputedStyle = [ComputedStyle{}; 8];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // Fixed elements ignore container's (50, 100) offset and resolve against viewport 800x600
        const r_tl = layout.getBoundingClientRect(fix_tl);
        expect_eq(r_tl.x, 20.0);
        expect_eq(r_tl.y, 10.0);
        expect_eq(r_tl.width, 80.0);
        expect_eq(r_tl.height, 30.0);

        const r_rb = layout.getBoundingClientRect(fix_rb);
        expect_eq(r_rb.x, 715.0); // 800 - 25 - 60
        expect_eq(r_rb.y, 545.0); // 600 - 15 - 40

        // in-flow child is not displaced by fixed siblings
        const r_flow = layout.getBoundingClientRect(in_flow);
        expect_eq(r_flow.x, 50.0);
        expect_eq(r_flow.y, 100.0);
    });

    test("7. Percentage width and height boundaries (definite vs indefinite parent)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElementWithAttrs("div", "root", "", "") catch unreachable;
        const def_parent = doc.createElementWithAttrs("div", "def-parent", "", "") catch unreachable;
        const def_child = doc.createElementWithAttrs("div", "def-child", "", "") catch unreachable;
        const auto_parent = doc.createElementWithAttrs("div", "auto-parent", "", "") catch unreachable;
        const indef_child = doc.createElementWithAttrs("div", "indef-child", "", "") catch unreachable;
        const fix_pct = doc.createElementWithAttrs("div", "fix-pct", "", "") catch unreachable;

        doc.appendChild(root, def_parent);
        doc.appendChild(def_parent, def_child);
        doc.appendChild(root, auto_parent);
        doc.appendChild(auto_parent, indef_child);
        doc.appendChild(root, fix_pct);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#root { width: 800px; }\n" ++
            "#def-parent { width: 400px; height: 200px; }\n" ++
            "#def-child { width: 50%; height: 25%; }\n" ++
            "#auto-parent { width: 400px; }\n" ++
            "#indef-child { width: 75%; height: 50%; }\n" ++
            "#fix-pct { position: fixed; top: 0; left: 0; width: 25%; height: 50%; }\n";

        const sheet = StyleSheet.parse(css_text, 800.0, false);
        let styles: [8]ComputedStyle = [ComputedStyle{}; 8];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // Definite parent (400x200): 50% width = 200, 25% height = 50
        const r_def = layout.getBoundingClientRect(def_child);
        expect_eq(r_def.width, 200.0);
        expect_eq(r_def.height, 50.0);

        // Indefinite (auto) parent height: 75% width = 300, 50% height behaves as auto (0.0 with no content)
        const r_indef = layout.getBoundingClientRect(indef_child);
        expect_eq(r_indef.width, 300.0);
        expect_eq(r_indef.height, 0.0);

        // Fixed element percentage resolves against viewport (800x600): 25% = 200, 50% = 300
        const r_fix = layout.getBoundingClientRect(fix_pct);
        expect_eq(r_fix.width, 200.0);
        expect_eq(r_fix.height, 300.0);
    });
}
