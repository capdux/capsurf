// test/css/test_specificity_and_important_edge.cp
// Pure CP white-box unit tests for CSS specificity calculation, 64-bit cascade
// weight packing, !important vs inline style precedence, source-order tie-breaking,
// and :is() / :where() / :not() / :has() specificity edge cases.

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { Specificity, SelectorParser, SelectorList, Matcher } from "../../src/css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";

fn expectSpec(sel_str: []const u8, expected_id: u32, expected_cls: u32, expected_el: u32)! {
    const sel = SelectorParser.parseSingleSelector(sel_str);
    expect_eq(sel.specificity.id_selectors, expected_id);
    expect_eq(sel.specificity.class_like_selectors, expected_cls);
    expect_eq(sel.specificity.element_selectors, expected_el);
}

fn expectColor(style: *const ComputedStyle, r: u8, g: u8, b: u8)! {
    expect_eq(style.color.r, r);
    expect_eq(style.color.g, g);
    expect_eq(style.color.b, b);
}

fn main() {
    test("1. Specificity component counts, 30-bit packing & 10-bit saturation", () => {
        // Universal & combinators contribute 0 specificity
        expectSpec("*", @as(u32, 0), @as(u32, 0), @as(u32, 0))?;
        expectSpec("* > * + * ~ *", @as(u32, 0), @as(u32, 0), @as(u32, 0))?;

        // Element / tag selectors
        expectSpec("div", @as(u32, 0), @as(u32, 0), @as(u32, 1))?;
        expectSpec("div > ul li + p", @as(u32, 0), @as(u32, 0), @as(u32, 4))?;

        // Class, attribute, and structural pseudo-classes
        expectSpec(".card", @as(u32, 0), @as(u32, 1), @as(u32, 0))?;
        expectSpec("div.card[data-x=\"1\"]:first-child", @as(u32, 0), @as(u32, 3), @as(u32, 1))?;

        // ID selectors (including repeated IDs)
        expectSpec("#hero", @as(u32, 1), @as(u32, 0), @as(u32, 0))?;
        expectSpec("#hero#hero", @as(u32, 2), @as(u32, 0), @as(u32, 0))?;
        expectSpec("main#app > section.wrap#hero div.card", @as(u32, 2), @as(u32, 2), @as(u32, 3))?;

        // Hierarchy: 1 ID > 20 classes + 20 tags; 1 class > 20 tags
        const one_id: u32 = Specificity.fromCounts(1, 0, 0).toU32();
        const many_cls: u32 = Specificity.fromCounts(0, 20, 20).toU32();
        const one_cls: u32 = Specificity.fromCounts(0, 1, 0).toU32();
        const many_tags: u32 = Specificity.fromCounts(0, 0, 20).toU32();
        expect_eq(one_id > many_cls, true);
        expect_eq(one_cls > many_tags, true);

        // 10-bit saturation clamping at 1023 per component
        const sat: u32 = Specificity.fromCounts(1024, 2048, 4096).toU32();
        const max_10: u32 = Specificity.fromCounts(1023, 1023, 1023).toU32();
        expect_eq(sat, max_10);
        expect_eq(sat, @as(u32, 0x3FFFFFFF));
    });

    test("2. Cascade.packWeight 64-bit bit-layout ordering invariants", () => {
        const normal_tag = Cascade.packWeight(false, false, Specificity.fromCounts(0, 0, 1).toU32(), 999);
        const normal_id = Cascade.packWeight(false, false, Specificity.fromCounts(1, 0, 0).toU32(), 1);
        const inline_norm = Cascade.packWeight(false, true, 0, 1);
        const imp_tag = Cascade.packWeight(true, false, Specificity.fromCounts(0, 0, 1).toU32(), 1);
        const imp_id = Cascade.packWeight(true, false, Specificity.fromCounts(1, 0, 0).toU32(), 1);
        const imp_id_later = Cascade.packWeight(true, false, Specificity.fromCounts(1, 0, 0).toU32(), 2);
        const imp_inline = Cascade.packWeight(true, true, 0, 1);

        expect_eq(normal_id > normal_tag, true);
        expect_eq(inline_norm > normal_id, true);
        expect_eq(imp_tag > inline_norm, true);
        expect_eq(imp_id > imp_tag, true);
        expect_eq(imp_id_later > imp_id, true);
        expect_eq(imp_inline > imp_id_later, true);
    });

    test("3. ID/Class/Tag cascade & comma-separated selector list specificity", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><div id=\"main\" class=\"box card\" data-role=\"panel\"></div><p id=\"sub\" class=\"box\"></p></body></html>";
        doc.parseHtml(html) catch unreachable;

        // #main written first, lower-specificity rules written later; #main must still win.
        // For #sub: `#nonexistent, p` only matches `p` (0,0,1), so `.box` (0,1,0) must beat it.
        const css =
            "#main { color: rgb(10, 20, 30); } " ++
            "div.box.card[data-role=\"panel\"] { color: rgb(40, 50, 60); } " ++
            "div { color: rgb(70, 80, 90); } " ++
            "p.box { color: rgb(11, 22, 33); } " ++
            "#nonexistent, p { color: rgb(200, 200, 200); }";

        const sheet = StyleSheet.parse(css, 800.0, false);
        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const main_id = Matcher.querySelector(&doc, "#main");
        const sub_id = Matcher.querySelector(&doc, "#sub");
        expect_eq(main_id != NULL_NODE, true);
        expect_eq(sub_id != NULL_NODE, true);

        expectColor(&styles[main_id], @as(u8, 10), @as(u8, 20), @as(u8, 30))?;
        expectColor(&styles[sub_id], @as(u8, 11), @as(u8, 22), @as(u8, 33))?;

        // Verify matchBestSpecificity ignores non-matching #nonexistent in comma list
        const sl = SelectorList.parse("#nonexistent, p");
        const best_sub = Matcher.matchBestSpecificity(&doc, sub_id, &sl);
        expect_eq(best_sub != null, true);
        if (bs := best_sub) {
            expect_eq(bs, Specificity.fromCounts(0, 0, 1).toU32());
        }
    });

    test("4. !important overriding inline style, inline !important, and mixed declarations", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body>" ++
            "<span id=\"a\" class=\"c\" style=\"color: rgb(255, 0, 0);\"></span>" ++
            "<div id=\"b\" class=\"c\" style=\"color: rgb(255, 0, 0); width: 200px;\"></div>" ++
            "<div id=\"c\" class=\"c\" style=\"color: rgb(12, 34, 56) !important;\"></div>" ++
            "</body></html>";
        doc.parseHtml(html) catch unreachable;

        // Rule 1: normal high-specificity rule cannot beat normal inline style on #a
        // Rule 2: low-specificity !important tag rule beats normal inline style on #b (color), while inline width wins
        // Rule 3: high-specificity stylesheet !important cannot beat inline !important on #c
        const css =
            "span#a.c { color: rgb(0, 128, 0); } " ++
            "div { color: rgb(0, 0, 255) !important; width: 100px; } " ++
            "#c.c { color: rgb(99, 99, 99) !important; }";

        const sheet = StyleSheet.parse(css, 800.0, false);
        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const a_id = Matcher.querySelector(&doc, "#a");
        const b_id = Matcher.querySelector(&doc, "#b");
        const c_id = Matcher.querySelector(&doc, "#c");

        // #a: normal inline style wins over #a.c
        expectColor(&styles[a_id], @as(u8, 255), @as(u8, 0), @as(u8, 0))?;

        // #b: div !important wins color, inline wins width
        expectColor(&styles[b_id], @as(u8, 0), @as(u8, 0), @as(u8, 255))?;
        expect_eq(styles[b_id].width.resolvePx(800.0, styles[b_id].font_size_px, 16.0, 800.0, 600.0) == 200.0, true);

        // #c: inline !important wins over #c.c !important
        expectColor(&styles[c_id], @as(u8, 12), @as(u8, 34), @as(u8, 56))?;
    });

    test("5. Source order tie-breaking (normal, !important, and inline)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body>" ++
            "<div id=\"n\" class=\"x y\"></div>" ++
            "<div id=\"imp\" class=\"x y\"></div>" ++
            "<div id=\"inl\" style=\"color: rgb(1, 1, 1); color: rgb(2, 2, 2);\"></div>" ++
            "</body></html>";
        doc.parseHtml(html) catch unreachable;

        const css =
            ".x { color: rgb(10, 10, 10); } " ++
            ".y { color: rgb(20, 20, 20); } " ++
            ".x { color: rgb(30, 30, 30) !important; } " ++
            ".y { color: rgb(40, 40, 40) !important; }";

        const sheet = StyleSheet.parse(css, 800.0, false);
        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const n_id = Matcher.querySelector(&doc, "#n");
        const imp_id = Matcher.querySelector(&doc, "#imp");
        const inl_id = Matcher.querySelector(&doc, "#inl");

        // Both #n and #imp match .x !important and .y !important; later .y !important (40,40,40) wins
        expectColor(&styles[n_id], @as(u8, 40), @as(u8, 40), @as(u8, 40))?;
        expectColor(&styles[imp_id], @as(u8, 40), @as(u8, 40), @as(u8, 40))?;

        // #inl has later inline declaration (2,2,2) winning over (1,1,1),
        // unless overridden by .x/.y (it has no class, so inline wins)
        expectColor(&styles[inl_id], @as(u8, 2), @as(u8, 2), @as(u8, 2))?;
    });

    test("6. :is(), :where(), :not(), and :has() specificity edge cases", () => {
        // :where() always contributes 0 specificity regardless of inner arguments
        expectSpec(":where(#hero, .card, div)", @as(u32, 0), @as(u32, 0), @as(u32, 0))?;
        expectSpec("div:where(#hero.card)", @as(u32, 0), @as(u32, 0), @as(u32, 1))?;
        expectSpec(":not(:where(#hero))", @as(u32, 0), @as(u32, 0), @as(u32, 0))?;

        // :is() and :not() take the maximum specificity of their argument list
        expectSpec(":is(div, .card, #hero)", @as(u32, 1), @as(u32, 0), @as(u32, 0))?;
        expectSpec(":not(.ghost, #other, span)", @as(u32, 1), @as(u32, 0), @as(u32, 0))?;
        expectSpec(":is(.a.b, .c) + :not(span)", @as(u32, 0), @as(u32, 2), @as(u32, 1))?;

        // :has() takes the maximum specificity of its relative selector list
        expectSpec(":has(> #child, .badge)", @as(u32, 1), @as(u32, 0), @as(u32, 0))?;
        expectSpec("div:has(+ span.tag)", @as(u32, 0), @as(u32, 1), @as(u32, 2))?;

        // Cascade integration with :where(), :is(), :not(), :has()
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body>" ++
            "<div id=\"w\" class=\"card\"></div>" ++
            "<div id=\"i\" class=\"low\"></div>" ++
            "<div id=\"h\" class=\"box active\"><span id=\"kid\"></span></div>" ++
            "</body></html>";
        doc.parseHtml(html) catch unreachable;

        const css =
            // On #w: div (0,0,1) must beat :where(#w.card) (0,0,0) even though :where appears later
            "div { color: rgb(1, 2, 3); } " ++
            ":where(#w.card) { color: rgb(99, 99, 99); } " ++
            // On #i: :is(.low, #nonexistent) gets (1,0,0) from #nonexistent, beating .low.low / div.low (0,1,1)
            ":is(.low, #nonexistent) { color: rgb(50, 60, 70); } " ++
            "div.low { color: rgb(80, 90, 100); } " ++
            // On #h: :has(> #kid, .none) gets (1,0,0) from #kid, beating .box.active (0,2,0)
            "div:has(> #kid, .none) { color: rgb(111, 122, 133); } " ++
            ".box.active { color: rgb(200, 210, 220); }";

        const sheet = StyleSheet.parse(css, 800.0, false);
        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const w_id = Matcher.querySelector(&doc, "#w");
        const i_id = Matcher.querySelector(&doc, "#i");
        const h_id = Matcher.querySelector(&doc, "#h");

        expectColor(&styles[w_id], @as(u8, 1), @as(u8, 2), @as(u8, 3))?;
        expectColor(&styles[i_id], @as(u8, 50), @as(u8, 60), @as(u8, 70))?;
        expectColor(&styles[h_id], @as(u8, 111), @as(u8, 122), @as(u8, 133))?;

        // Also verify :not() specificity in cascade on #w
        const css_not =
            ":not(#ghost, .missing) { color: rgb(77, 88, 99); } " ++
            "div.card { color: rgb(10, 10, 10); }";
        const sheet_not = StyleSheet.parse(css_not, 800.0, false);
        let styles_not: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet_not, styles_not[0..doc.nodeCount()], 16.0);
        expectColor(&styles_not[w_id], @as(u8, 77), @as(u8, 88), @as(u8, 99))?;
    });
}
