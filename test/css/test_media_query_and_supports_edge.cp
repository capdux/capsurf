// test/css/test_media_query_and_supports_edge.cp
// Pure CP white-box unit tests for:
// 1. CssMediaQuery: width, height, orientation, prefers-color-scheme, prefers-reduced-motion, not/only/screen/print
// 2. MediaRangeQuery (Level 4): range comparisons, flipped operands, intervals, aspect-ratio & lowerToLegacy
// 3. StyleSheet.parse + Cascade integration with @media viewport width & dark mode switching
// 4. SupportsAst (@supports): declaration, selector(), not/and/or, nested compound, format() & malformed/mixed-op edge cases

import { test, expect_eq } from "cap:test";
import { CssMediaQuery, MediaRangeQuery, SupportsAst, CssRule, CssRuleKind, CssStylesheetParser } from "cap:css";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { Matcher } from "../../src/css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";

fn nearF32(actual: f32, expected: f32, eps: f32): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn mockDeclSupport(prop: []const u8, val: []const u8): bool {
    if (std.ascii.eqlIgnoreCase(prop, "display")) {
        return std.ascii.eqlIgnoreCase(val, "grid") || std.ascii.eqlIgnoreCase(val, "flex") || std.ascii.eqlIgnoreCase(val, "block");
    }
    if (std.ascii.eqlIgnoreCase(prop, "gap")) {
        return std.ascii.eqlIgnoreCase(val, "10px");
    }
    if (std.ascii.eqlIgnoreCase(prop, "container-type")) {
        return std.ascii.eqlIgnoreCase(val, "inline-size");
    }
    return false;
}

fn mockSelSupport(sel: []const u8): bool {
    return std.mem.eql(u8, sel, ":has(*)") || std.mem.eql(u8, sel, "div > p");
}

fn main() {
    test("1. CssMediaQuery width, height, orientation, color-scheme & negation boundaries", () => {
        // Empty / bare @media matches everything
        const q_empty = CssMediaQuery.parse("@media");
        expect_eq(q_empty.matchesFull(320.0, 480.0, false, false), true);
        expect_eq(q_empty.matchesFull(1920.0, 1080.0, true, true), true);

        // Inclusive min-width / max-width boundaries
        const q_range = CssMediaQuery.parse("screen and (min-width: 768px) and (max-width: 1024px)");
        expect_eq(std.mem.eql(u8, q_range.media_type, "screen"), true);
        expect_eq(q_range.matches(767.9, false), false);
        expect_eq(q_range.matches(768.0, false), true);
        expect_eq(q_range.matches(900.0, false), true);
        expect_eq(q_range.matches(1024.0, false), true);
        expect_eq(q_range.matches(1024.1, false), false);

        // Height boundaries with em/px units
        const q_height = CssMediaQuery.parse("(min-height: 400px) and (max-height: 800px)");
        expect_eq(q_height.matchesFull(1280.0, 399.0, false, false), false);
        expect_eq(q_height.matchesFull(1280.0, 400.0, false, false), true);
        expect_eq(q_height.matchesFull(1280.0, 800.0, false, false), true);
        expect_eq(q_height.matchesFull(1280.0, 801.0, false, false), false);

        // Orientation: landscape vs portrait vs square (w == h matches both per spec)
        const q_land = CssMediaQuery.parse("(orientation: landscape)");
        const q_port = CssMediaQuery.parse("(orientation: portrait)");
        expect_eq(q_land.matchesFull(1280.0, 720.0, false, false), true);
        expect_eq(q_land.matchesFull(720.0, 1280.0, false, false), false);
        expect_eq(q_port.matchesFull(720.0, 1280.0, false, false), true);
        expect_eq(q_port.matchesFull(1280.0, 720.0, false, false), false);
        expect_eq(q_land.matchesFull(600.0, 600.0, false, false), true);
        expect_eq(q_port.matchesFull(600.0, 600.0, false, false), true);

        // prefers-color-scheme & prefers-reduced-motion
        const q_dark = CssMediaQuery.parse("(prefers-color-scheme: dark)");
        expect_eq(q_dark.matchesFull(800.0, 600.0, false, false), false);
        expect_eq(q_dark.matchesFull(800.0, 600.0, true, false), true);

        const q_motion = CssMediaQuery.parse("(prefers-reduced-motion: reduce)");
        expect_eq(q_motion.matchesFull(800.0, 600.0, false, false), false);
        expect_eq(q_motion.matchesFull(800.0, 600.0, false, true), true);

        // Negation (`not`) and `only print`
        const q_not_dark = CssMediaQuery.parse("not (prefers-color-scheme: dark)");
        expect_eq(q_not_dark.negated, true);
        expect_eq(q_not_dark.matchesFull(800.0, 600.0, false, false), true);
        expect_eq(q_not_dark.matchesFull(800.0, 600.0, true, false), false);

        const q_only_print = CssMediaQuery.parse("only print and (min-width: 500px)");
        expect_eq(std.mem.eql(u8, q_only_print.media_type, "print"), true);
        expect_eq(q_only_print.negated, false);
        expect_eq(q_only_print.matches(500.0, false), true);
        expect_eq(q_only_print.matches(499.0, false), false);
    });

    test("2. MediaRangeQuery Level 4 comparisons, flipped operands, intervals & lowering", () => {
        // Flipped operand: `400px <= width` -> width >= 400px
        if (mr_flip := MediaRangeQuery.parse("(400px <= width)")) {
            expect_eq(mr_flip.matches(399.0, 600.0), false);
            expect_eq(mr_flip.matches(400.0, 600.0), true);
            expect_eq(mr_flip.matches(800.0, 600.0), true);
        } else {
            expect_eq(false, true);
        }

        // Strict inequality interval: `400px < width < 900px`
        if (mr_iv := MediaRangeQuery.parse("(400px < width < 900px)")) {
            expect_eq(mr_iv.is_interval, true);
            expect_eq(mr_iv.matches(400.0, 600.0), false);
            expect_eq(mr_iv.matches(401.0, 600.0), true);
            expect_eq(mr_iv.matches(899.0, 600.0), true);
            expect_eq(mr_iv.matches(900.0, 600.0), false);

            let legacy_buf: [128]u8 = [0; 128];
            const legacy: []const u8 = mr_iv.lowerToLegacy(legacy_buf[0..128]);
            expect_eq(std.mem.indexOf(u8, legacy, "min-width:") != null, true);
            expect_eq(std.mem.indexOf(u8, legacy, "max-width:") != null, true);
        } else {
            expect_eq(false, true);
        }

        // Height & aspect-ratio range queries
        if (mr_h := MediaRangeQuery.parse("(height >= 600px)")) {
            expect_eq(mr_h.matches(800.0, 599.0), false);
            expect_eq(mr_h.matches(800.0, 600.0), true);
        } else {
            expect_eq(false, true);
        }

        if (mr_ar := MediaRangeQuery.parse("(aspect-ratio >= 16/9)")) {
            expect_eq(mr_ar.matches(1920.0, 1080.0), true);
            expect_eq(mr_ar.matches(800.0, 600.0), false);
            expect_eq(mr_ar.matches(800.0, 0.0), false);
        } else {
            expect_eq(false, true);
        }

        // Invalid / empty MediaRangeQuery
        expect_eq(MediaRangeQuery.parse("") == null, true);
        expect_eq(MediaRangeQuery.parse("()") == null, true);
        expect_eq(MediaRangeQuery.parse("(width)") == null, true);
    });

    test("3. StyleSheet + Cascade @media integration across viewport & color-scheme", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><div id=\"panel\"></div></body></html>";
        doc.parseHtml(html) catch unreachable;

        const css =
            "#panel { width: 100px; color: #111111; }\n" +
            "@media (min-width: 768px) { #panel { width: 500px; } }\n" +
            "@media (prefers-color-scheme: dark) { #panel { color: #eeeeee; } }\n" +
            "@media not (prefers-color-scheme: dark) { #panel { background-color: #fafafa; } }\n";

        const panel = Matcher.querySelector(&doc, "#panel");
        expect_eq(panel != NULL_NODE, true);

        // Narrow light mode (600px, is_dark=false)
        const sheet_narrow_light = StyleSheet.parse(css, 600.0, false);
        let styles1: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet_narrow_light, styles1[0..doc.nodeCount()], 16.0);
        expect_eq(nearF32(styles1[panel].width.resolvePx(600.0, 16.0, 16.0, 600.0, 800.0), 100.0, 0.01), true);
        expect_eq(styles1[panel].color.r, @as(u8, 0x11));
        expect_eq(styles1[panel].background_color.r, @as(u8, 0xFA));

        // Wide dark mode (1024px, is_dark=true)
        const sheet_wide_dark = StyleSheet.parse(css, 1024.0, true);
        let styles2: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet_wide_dark, styles2[0..doc.nodeCount()], 16.0);
        expect_eq(nearF32(styles2[panel].width.resolvePx(1024.0, 16.0, 16.0, 1024.0, 768.0), 500.0, 0.01), true);
        expect_eq(styles2[panel].color.r, @as(u8, 0xEE));
        expect_eq(styles2[panel].background_color.a, @as(u8, 0));
    });

    test("4. SupportsAst (@supports) evaluation, formatting & malformed syntax edge cases", () => {
        // Single declaration & selector()
        if (s1 := SupportsAst.parse("@supports (display: grid)")) {
            expect_eq(s1.eval(mockDeclSupport, mockSelSupport), true);
        } else {
            expect_eq(false, true);
        }

        if (s2 := SupportsAst.parse("@supports (display: unknown-layout)")) {
            expect_eq(s2.eval(mockDeclSupport, mockSelSupport), false);
        } else {
            expect_eq(false, true);
        }

        if (s_sel := SupportsAst.parse("selector(:has(*))")) {
            expect_eq(s_sel.eval(mockDeclSupport, mockSelSupport), true);
        } else {
            expect_eq(false, true);
        }

        // `not`, `and`, `or`, and nested compound parentheses
        if (s_not := SupportsAst.parse("not (display: masonry)")) {
            expect_eq(s_not.eval(mockDeclSupport, mockSelSupport), true);
        } else {
            expect_eq(false, true);
        }

        if (s_and := SupportsAst.parse("(display: grid) and (gap: 10px)")) {
            expect_eq(s_and.eval(mockDeclSupport, mockSelSupport), true);
        } else {
            expect_eq(false, true);
        }

        if (s_or := SupportsAst.parse("(display: masonry) or (display: flex)")) {
            expect_eq(s_or.eval(mockDeclSupport, mockSelSupport), true);
        } else {
            expect_eq(false, true);
        }

        if (s_nested := SupportsAst.parse("((display: masonry) or (selector(div > p))) and (not (foo: bar))")) {
            expect_eq(s_nested.eval(mockDeclSupport, mockSelSupport), true);
            let fmt_buf: [256]u8 = [0; 256];
            const formatted: []const u8 = s_nested.format(fmt_buf[0..256]);
            expect_eq(formatted.len > 0, true);
            expect_eq(std.mem.indexOf(u8, formatted, "selector(div > p)") != null, true);
        } else {
            expect_eq(false, true);
        }

        // Unknown non-declaration parenthesized token evaluates to false
        if (s_unk := SupportsAst.parse("(not-a-declaration)")) {
            expect_eq(s_unk.eval(mockDeclSupport, mockSelSupport), false);
        } else {
            expect_eq(false, true);
        }

        // CSS spec edge cases: empty, empty parens, unclosed paren, mixing `and` & `or` without parens
        expect_eq(SupportsAst.parse("") == null, true);
        expect_eq(SupportsAst.parse("@supports   ") == null, true);
        expect_eq(SupportsAst.parse("()") == null, true);
        expect_eq(SupportsAst.parse("(display: grid") == null, true);
        expect_eq(SupportsAst.parse("(display: grid) and (gap: 10px) or (display: flex)") == null, true);

        // CssStylesheetParser recognizes @supports as CssRuleKind.Supports
        let rules: [8]CssRule = [CssRule{}; 8];
        const rule_cnt = CssStylesheetParser.parseStylesheet("@supports (display: grid) { .a { color: red; } }", rules[0..8]);
        expect_eq(rule_cnt, @as(usize, 1));
        expect_eq(rules[0].kind == CssRuleKind.Supports, true);
    });
}
