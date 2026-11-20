// test/css/test_container_and_layers.cp
// Pure CP white-box unit tests for:
// 1. 64-bit cascade weight ordering (!important > inline style > ID > class > tag > source_order)
// 2. @container size queries & recomputation (inline-size, size, named containers, range & colon syntax)
// 3. @container block-axis gating & container-name scoping
// 4. CSS inheritance (color, font-size, font-weight, custom props) vs initial values (background-color, width, margin)

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { Matcher } from "../../src/css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";

fn nearF32(actual: f32, expected: f32, eps: f32): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn main() {
    test("1. 64-bit cascade weight: !important > inline style > ID > class > tag > source_order", () => {
        // Direct packWeight bit-packing order checks
        const w_tag_1 = Cascade.packWeight(false, false, 0x000001, 1);
        const w_tag_2 = Cascade.packWeight(false, false, 0x000001, 2);
        const w_cls = Cascade.packWeight(false, false, 0x000100, 1);
        const w_id = Cascade.packWeight(false, false, 0x010000, 1);
        const w_inline = Cascade.packWeight(false, true, 0, 1);
        const w_imp_tag = Cascade.packWeight(true, false, 0x000001, 1);
        const w_imp_inline = Cascade.packWeight(true, true, 0, 1);

        expect_eq(w_tag_2 > w_tag_1, true);
        expect_eq(w_cls > w_tag_2, true);
        expect_eq(w_id > w_cls, true);
        expect_eq(w_inline > w_id, true);
        expect_eq(w_imp_tag > w_inline, true);
        expect_eq(w_imp_inline > w_imp_tag, true);

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html><html><body>" +
            "<p id=\"p1\" class=\"hi\" style=\"color: #0000ff;\">a</p>" +
            "<p id=\"p2\" class=\"hi\" style=\"color: #0000ff; background-color: #112233;\">b</p>" +
            "<p id=\"p3\" class=\"hi\">c</p>" +
            "<p id=\"p4\" class=\"hi\">d</p>" +
            "</body></html>";
        doc.parseHtml(html) catch unreachable;

        const css =
            "p { color: #111111; background-color: #222222; width: 100px; }\n" +
            "p { width: 150px; }\n" +
            ".hi { color: #00ff00; width: 200px; }\n" +
            "#p3 { color: #ff00ff; }\n" +
            "#p4 { width: 300px; }\n" +
            "p.hi { background-color: #ff0000 !important; }\n" +
            "p { color: #abcdef !important; }\n";
        const sheet = StyleSheet.parse(css, 800.0, false);

        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const p1 = Matcher.querySelector(&doc, "#p1");
        const p2 = Matcher.querySelector(&doc, "#p2");
        const p3 = Matcher.querySelector(&doc, "#p3");
        const p4 = Matcher.querySelector(&doc, "#p4");
        expect_eq(p1 != NULL_NODE, true);
        expect_eq(p2 != NULL_NODE, true);
        expect_eq(p3 != NULL_NODE, true);
        expect_eq(p4 != NULL_NODE, true);

        // !important on tag beats inline style on #p1
        expect_eq(styles[p1].color.r, @as(u8, 0xAB));
        expect_eq(styles[p1].color.g, @as(u8, 0xCD));
        expect_eq(styles[p1].color.b, @as(u8, 0xEF));

        // !important on p.hi beats inline background-color on #p2
        expect_eq(styles[p2].background_color.r, @as(u8, 255));
        expect_eq(styles[p2].background_color.g, @as(u8, 0));
        expect_eq(styles[p2].background_color.b, @as(u8, 0));

        // Class beats source_order tag rules for width (200px > 150px > 100px)
        expect_eq(nearF32(styles[p3].width.resolvePx(800.0, 16.0, 16.0, 800.0, 600.0), 200.0, 0.01), true);

        // ID beats class for width on #p4 (300px > 200px)
        expect_eq(nearF32(styles[p4].width.resolvePx(800.0, 16.0, 16.0, 800.0, 600.0), 300.0, 0.01), true);
    });

    test("2. @container size queries & recomputation across narrow/wide container widths", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html><html><body>" +
            "<div id=\"wrap\"><div id=\"card\"></div></div>" +
            "</body></html>";
        doc.parseHtml(html) catch unreachable;

        const css =
            "#wrap { container-type: inline-size; container-name: layout; }\n" +
            "#card { width: 100px; color: #111111; }\n" +
            "@container (min-width: 400px) { #card { width: 320px; } }\n" +
            "@container layout (width >= 500px) { #card { color: #00aa55; } }\n";
        const sheet = StyleSheet.parse(css, 800.0, false);

        const wrap_id = Matcher.querySelector(&doc, "#wrap");
        const card_id = Matcher.querySelector(&doc, "#card");
        expect_eq(wrap_id != NULL_NODE, true);
        expect_eq(card_id != NULL_NODE, true);

        let box_widths: [32]f32 = [0.0; 32];
        let box_heights: [32]f32 = [0.0; 32];
        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];

        // Narrow pass: wrap width = 250px -> neither @container matches
        box_widths[@as(usize, wrap_id)] = 250.0;
        box_heights[@as(usize, wrap_id)] = 200.0;
        Cascade.computeStylesWithContainers(
            &doc,
            &sheet,
            styles[0..doc.nodeCount()],
            box_widths[0..doc.nodeCount()],
            box_heights[0..doc.nodeCount()],
            16.0,
        );
        expect_eq(nearF32(styles[card_id].width.resolvePx(250.0, 16.0, 16.0, 800.0, 600.0), 100.0, 0.01), true);
        expect_eq(styles[card_id].color.r, @as(u8, 0x11));
        expect_eq(styles[card_id].color.g, @as(u8, 0x11));
        expect_eq(styles[card_id].color.b, @as(u8, 0x11));

        // Wide pass: wrap width = 520px -> both @container rules match
        box_widths[@as(usize, wrap_id)] = 520.0;
        Cascade.computeStylesWithContainers(
            &doc,
            &sheet,
            styles[0..doc.nodeCount()],
            box_widths[0..doc.nodeCount()],
            box_heights[0..doc.nodeCount()],
            16.0,
        );
        expect_eq(nearF32(styles[card_id].width.resolvePx(520.0, 16.0, 16.0, 800.0, 600.0), 320.0, 0.01), true);
        expect_eq(styles[card_id].color.r, @as(u8, 0x00));
        expect_eq(styles[card_id].color.g, @as(u8, 0xAA));
        expect_eq(styles[card_id].color.b, @as(u8, 0x55));
    });

    test("3. @container block-axis gating (inline-size vs size) & container-name scoping", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html><html><body>" +
            "<div id=\"inline_box\"><div id=\"child_a\"></div></div>" +
            "<div id=\"size_box\"><div id=\"child_b\"></div></div>" +
            "</body></html>";
        doc.parseHtml(html) catch unreachable;

        const css =
            "#inline_box { container-type: inline-size; container-name: alpha; }\n" +
            "#size_box { container-type: size; container-name: beta; }\n" +
            "#child_a { width: 50px; }\n" +
            "#child_b { width: 50px; }\n" +
            "@container (min-height: 200px) { #child_a { width: 111px; } }\n" +
            "@container wrong_name (min-width: 100px) { #child_a { width: 222px; } }\n" +
            "@container beta (width >= 300px) and (height >= 200px) { #child_b { width: 333px; } }\n";
        const sheet = StyleSheet.parse(css, 800.0, false);

        const inline_box = Matcher.querySelector(&doc, "#inline_box");
        const size_box = Matcher.querySelector(&doc, "#size_box");
        const child_a = Matcher.querySelector(&doc, "#child_a");
        const child_b = Matcher.querySelector(&doc, "#child_b");
        expect_eq(inline_box != NULL_NODE, true);
        expect_eq(size_box != NULL_NODE, true);
        expect_eq(child_a != NULL_NODE, true);
        expect_eq(child_b != NULL_NODE, true);

        let box_widths: [32]f32 = [0.0; 32];
        let box_heights: [32]f32 = [0.0; 32];
        box_widths[@as(usize, inline_box)] = 400.0;
        box_heights[@as(usize, inline_box)] = 400.0;
        box_widths[@as(usize, size_box)] = 400.0;
        box_heights[@as(usize, size_box)] = 300.0;

        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStylesWithContainers(
            &doc,
            &sheet,
            styles[0..doc.nodeCount()],
            box_widths[0..doc.nodeCount()],
            box_heights[0..doc.nodeCount()],
            16.0,
        );

        // inline-size container must not match min-height, and wrong_name must not match alpha
        expect_eq(nearF32(styles[child_a].width.resolvePx(400.0, 16.0, 16.0, 800.0, 600.0), 50.0, 0.01), true);
        // size container named beta matches width >= 300px and height >= 200px
        expect_eq(nearF32(styles[child_b].width.resolvePx(400.0, 16.0, 16.0, 800.0, 600.0), 333.0, 0.01), true);
    });

    test("4. CSS inheritance (color, font-size, font-weight, --var) vs non-inherited initial values", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html><html><body>" +
            "<div id=\"parent\"><span id=\"child\">text</span></div>" +
            "</body></html>";
        doc.parseHtml(html) catch unreachable;

        const css =
            "#parent { color: #224466; font-size: 24px; font-weight: 700; background-color: #ffee00; width: 400px; margin-top: 20px; --pad: 12px; }\n" +
            "#child { padding-left: var(--pad); }\n";
        const sheet = StyleSheet.parse(css, 800.0, false);

        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const parent_id = Matcher.querySelector(&doc, "#parent");
        const child_id = Matcher.querySelector(&doc, "#child");
        expect_eq(parent_id != NULL_NODE, true);
        expect_eq(child_id != NULL_NODE, true);

        // Inherited properties on child match parent
        expect_eq(styles[child_id].color.r, @as(u8, 0x22));
        expect_eq(styles[child_id].color.g, @as(u8, 0x44));
        expect_eq(styles[child_id].color.b, @as(u8, 0x66));
        expect_eq(nearF32(styles[child_id].font_size_px, 24.0, 0.01), true);
        expect_eq(nearF32(styles[child_id].font_weight, 700.0, 0.01), true);
        expect_eq(nearF32(styles[child_id].padding_left.resolvePx(400.0, 24.0, 16.0, 800.0, 600.0), 12.0, 0.01), true);

        // Non-inherited properties on child remain at initial values
        expect_eq(styles[child_id].background_color.a, @as(u8, 0));
        expect_eq(styles[child_id].width.is_auto, true);
        expect_eq(nearF32(styles[child_id].margin_top.resolvePx(400.0, 24.0, 16.0, 800.0, 600.0), 0.0, 0.01), true);
    });
}
