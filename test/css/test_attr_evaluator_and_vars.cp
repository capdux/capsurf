// test/css/test_attr_evaluator_and_vars.cp
// Pure CP white-box unit tests for attr_evaluator.cp (attr() parsing & evaluation),
// CSS custom properties (--foo) inheritance/override/var()/cycle protection,
// calc() expressions, and !important / specificity cascade weights.

import { test, expect_eq } from "cap:test";
import { ParsedAttr, parseAttrExpr, isUrlType, isNumberType, evaluateCssAttr } from "../../src/css/attr_evaluator.cp";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { Matcher } from "../../src/css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";

fn nearF32(actual: f32, expected: f32, eps: f32): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn strEq(a: []const u8, b: []const u8): bool {
    return std.mem.eql(u8, a, b);
}

fn main() {
    test("1. attr_evaluator parseAttrExpr & evaluateCssAttr (present, fallback, px/string/number/color/url)", () => {
        // Basic attr(name)
        const p_basic = parseAttrExpr("  attr(data-label)  ");
        expect_eq(p_basic.is_valid, true);
        expect_eq(strEq(p_basic.attr_name, "data-label"), true);
        expect_eq(strEq(p_basic.type_name, ""), true);
        expect_eq(p_basic.fallback == null, true);

        // attr(name px, fallback)
        const p_px = parseAttrExpr("attr(data-width px, 100px)");
        expect_eq(p_px.is_valid, true);
        expect_eq(strEq(p_px.attr_name, "data-width"), true);
        expect_eq(strEq(p_px.type_name, "px"), true);
        if (fb := p_px.fallback) {
            expect_eq(strEq(fb, "100px"), true);
        } else {
            expect_eq(false, true);
        }

        // attr(name string, fallback)
        const p_str = parseAttrExpr("attr(data-title string, \"untitled\")");
        expect_eq(p_str.is_valid, true);
        expect_eq(strEq(p_str.attr_name, "data-title"), true);
        expect_eq(strEq(p_str.type_name, "string"), true);
        if (fb := p_str.fallback) {
            expect_eq(strEq(fb, "\"untitled\""), true);
        } else {
            expect_eq(false, true);
        }

        // attr(name type(<number>), fallback) & attr(name color, fallback)
        const p_num = parseAttrExpr("attr(data-count type(<number>), 42)");
        expect_eq(p_num.is_valid, true);
        expect_eq(isNumberType(p_num.type_name), true);
        expect_eq(isUrlType(p_num.type_name), false);

        const p_color = parseAttrExpr("attr(data-bg color, #112233)");
        expect_eq(p_color.is_valid, true);
        expect_eq(strEq(p_color.attr_name, "data-bg"), true);
        expect_eq(strEq(p_color.type_name, "color"), true);

        // attr(name type(<url>), fallback)
        const p_url = parseAttrExpr("attr(href type(<url>), url(fallback.png))");
        expect_eq(p_url.is_valid, true);
        expect_eq(isUrlType(p_url.type_name), true);
        if (fb := p_url.fallback) {
            expect_eq(strEq(fb, "url(fallback.png)"), true);
        } else {
            expect_eq(false, true);
        }

        // Invalid expressions
        expect_eq(parseAttrExpr("attr()").is_valid, false);
        expect_eq(parseAttrExpr("not-attr(x)").is_valid, false);
        expect_eq(parseAttrExpr("attr(x").is_valid, false);

        // Evaluation: present attribute vs missing attribute fallback
        expect_eq(strEq(evaluateCssAttr("attr(data-label, default)", "hello"), "hello"), true);
        expect_eq(strEq(evaluateCssAttr("attr(data-label, default)", null), "default"), true);
        expect_eq(strEq(evaluateCssAttr("attr(data-label)", null), ""), true);

        // Evaluation with unit/type: px, string, number, color
        expect_eq(strEq(evaluateCssAttr("attr(data-width px, 100px)", "240px"), "240px"), true);
        expect_eq(strEq(evaluateCssAttr("attr(data-width px, 100px)", null), "100px"), true);
        expect_eq(strEq(evaluateCssAttr("attr(data-title string, \"none\")", "\"hi\""), "\"hi\""), true);
        expect_eq(strEq(evaluateCssAttr("attr(data-title string, \"none\")", null), "\"none\""), true);
        expect_eq(strEq(evaluateCssAttr("attr(data-count type(<number>), 42)", "7"), "7"), true);
        expect_eq(strEq(evaluateCssAttr("attr(data-count type(<number>), 42)", null), "42"), true);
        expect_eq(strEq(evaluateCssAttr("attr(data-count type(<number>))", null), ""), true);
        expect_eq(strEq(evaluateCssAttr("attr(data-bg color, #000000)", "#ff00aa"), "#ff00aa"), true);
        expect_eq(strEq(evaluateCssAttr("attr(data-bg color, #000000)", null), "#000000"), true);

        // W3C CSS Values L5: type(<url>) is attr()-tainted -> returns fallback or "" even if attr present
        expect_eq(strEq(evaluateCssAttr("attr(href type(<url>), url(safe.png))", "https://example.com/a.png"), "url(safe.png)"), true);
        expect_eq(strEq(evaluateCssAttr("attr(href type(<url>))", "https://example.com/a.png"), ""), true);
    });

    test("2. Cascade custom properties (--foo) inheritance, override, var() fallback & cycle protection", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><div id=\"parent\"><div id=\"child\"></div><div id=\"chain\"></div><div id=\"cycle\"></div></div></body></html>";
        doc.parseHtml(html) catch unreachable;

        const css =
            "#parent { --accent: #102030; --w: 120px; --h: 30px; }\n" +
            "#child { --h: 60px; color: var(--accent); width: var(--w); height: var(--h); background-color: var(--missing, #00ff00); }\n" +
            "#chain { --a: var(--b); --b: 85px; width: var(--a); }\n" +
            "#cycle { --c1: var(--c2); --c2: var(--c1); width: 45px; width: var(--c1); color: #224466; color: var(--c1); }\n";
        const sheet = StyleSheet.parse(css, 800.0, false);

        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const child_id = Matcher.querySelector(&doc, "#child");
        const chain_id = Matcher.querySelector(&doc, "#chain");
        const cycle_id = Matcher.querySelector(&doc, "#cycle");
        expect_eq(child_id != NULL_NODE, true);
        expect_eq(chain_id != NULL_NODE, true);
        expect_eq(cycle_id != NULL_NODE, true);

        // Inherited --accent, --w and overridden --h + fallback --missing
        expect_eq(styles[child_id].color.r, @as(u8, 0x10));
        expect_eq(styles[child_id].color.g, @as(u8, 0x20));
        expect_eq(styles[child_id].color.b, @as(u8, 0x30));
        expect_eq(nearF32(styles[child_id].width.resolvePx(800.0, 16.0, 16.0, 800.0, 600.0), 120.0, 0.01), true);
        expect_eq(nearF32(styles[child_id].height.resolvePx(600.0, 16.0, 16.0, 800.0, 600.0), 60.0, 0.01), true);
        expect_eq(styles[child_id].background_color.r, @as(u8, 0));
        expect_eq(styles[child_id].background_color.g, @as(u8, 255));
        expect_eq(styles[child_id].background_color.b, @as(u8, 0));

        // Chained var(--a) -> var(--b) -> 85px
        expect_eq(nearF32(styles[chain_id].width.resolvePx(800.0, 16.0, 16.0, 800.0, 600.0), 85.0, 0.01), true);

        // Cycle protection: depth-bounded resolution stops without infinite loop and preserves prior valid values
        expect_eq(nearF32(styles[cycle_id].width.resolvePx(800.0, 16.0, 16.0, 800.0, 600.0), 45.0, 0.01), true);
        expect_eq(styles[cycle_id].color.r, @as(u8, 0x22));
        expect_eq(styles[cycle_id].color.g, @as(u8, 0x44));
        expect_eq(styles[cycle_id].color.b, @as(u8, 0x66));
    });

    test("3. Cascade calc() expression evaluation across px, rem, and percentage", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><div id=\"c1\"></div><div id=\"c2\"></div></body></html>";
        doc.parseHtml(html) catch unreachable;

        const css =
            "#c1 { width: calc(10px + 20px * 2); height: calc(2rem + 8px); }\n" +
            "#c2 { width: calc(50% - 10px); }\n";
        const sheet = StyleSheet.parse(css, 800.0, false);

        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const c1_id = Matcher.querySelector(&doc, "#c1");
        const c2_id = Matcher.querySelector(&doc, "#c2");
        expect_eq(c1_id != NULL_NODE, true);
        expect_eq(c2_id != NULL_NODE, true);

        expect_eq(nearF32(styles[c1_id].width.resolvePx(400.0, 16.0, 16.0, 800.0, 600.0), 50.0, 0.01), true);
        expect_eq(nearF32(styles[c1_id].height.resolvePx(400.0, 16.0, 16.0, 800.0, 600.0), 40.0, 0.01), true);
        expect_eq(nearF32(styles[c2_id].width.resolvePx(200.0, 16.0, 16.0, 800.0, 600.0), 90.0, 0.01), true);
    });

    test("4. Cascade !important weight, specificity & inline style precedence", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><div id=\"imp\" class=\"box\" style=\"color: #00ff00; width: 150px !important;\"></div></body></html>";
        doc.parseHtml(html) catch unreachable;

        // Low-specificity !important beats high-specificity & normal inline style;
        // Inline !important beats stylesheet !important.
        const css =
            "div { color: #ff0000 !important; --tag-w: 33px !important; }\n" +
            "#imp.box { color: #0000ff; width: 80px !important; --tag-w: 99px; height: var(--tag-w); }\n";
        const sheet = StyleSheet.parse(css, 800.0, false);

        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const imp_id = Matcher.querySelector(&doc, "#imp");
        expect_eq(imp_id != NULL_NODE, true);

        // color: #ff0000 !important beats inline #00ff00 and #imp.box #0000ff
        expect_eq(styles[imp_id].color.r, @as(u8, 255));
        expect_eq(styles[imp_id].color.g, @as(u8, 0));
        expect_eq(styles[imp_id].color.b, @as(u8, 0));

        // width: inline 150px !important beats #imp.box 80px !important
        expect_eq(nearF32(styles[imp_id].width.resolvePx(800.0, 16.0, 16.0, 800.0, 600.0), 150.0, 0.01), true);

        // --tag-w: 33px !important beats #imp.box --tag-w: 99px
        expect_eq(nearF32(styles[imp_id].height.resolvePx(600.0, 16.0, 16.0, 800.0, 600.0), 33.0, 0.01), true);
    });
}
