// test/css/test_calc_and_colors_edge.cp
// Pure CP white-box unit tests for nested calc(), mixed units, division-by-zero / invalid
// expression protection, and all-format CSS color parsing edge cases.

import { test, expect_eq } from "cap:test";
import { CssColor, CssCalc } from "cap:css";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { Matcher } from "../../../src/css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../../src/css/cascade.cp";

fn nearF64(actual: f64, expected: f64, eps: f64): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn nearF32(actual: f32, expected: f32, eps: f32): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn expectCalcPx(expr: []const u8, root_px: f64, vw: f64, vh: f64, expected: f64, eps: f64)! {
    if (v := CssCalc.evalPx(expr, root_px, vw, vh)) {
        expect_eq(nearF64(v, expected, eps), true);
    } else {
        expect_eq(false, true);
    }
}

fn expectColorRgba(input: []const u8, r: u8, g: u8, b: u8, a: u8)! {
    if (c := CssColor.parse(input)) {
        expect_eq(c.r, r);
        expect_eq(c.g, g);
        expect_eq(c.b, b);
        expect_eq(c.a, a);
    } else {
        expect_eq(false, true);
    }
}

fn main() {
    test("1. Nested calc(), parentheses, operator precedence & unary signs", () => {
        // Nested calc() inside calc()
        expectCalcPx("calc(calc(10px + 20px) * 2 - calc(4px + 1px))", 16.0, 800.0, 600.0, 55.0, 0.01)?;

        // Deeply nested parentheses
        expectCalcPx("calc(((10px + 20px) * (3 - 1)) / 4)", 16.0, 800.0, 600.0, 15.0, 0.01)?;

        // Operator precedence (* and / before + and -)
        expectCalcPx("calc(10px + 20px * 3 - 6px / 2)", 16.0, 800.0, 600.0, 67.0, 0.01)?;

        // Unary + and - operators, including double unary minus
        expectCalcPx("calc(-10px + +25px)", 16.0, 800.0, 600.0, 15.0, 0.01)?;
        expectCalcPx("calc(-(10px + 5px))", 16.0, 800.0, 600.0, -15.0, 0.01)?;
        expectCalcPx("calc(--10px)", 16.0, 800.0, 600.0, 10.0, 0.01)?;

        // Nested min(), max(), clamp() compositions
        expectCalcPx("calc(min(100px, 2rem) + max(10px, 5vh))", 16.0, 800.0, 600.0, 62.0, 0.01)?;
        expectCalcPx("clamp(10px, calc(2rem + 1vw), 100px)", 16.0, 800.0, 600.0, 40.0, 0.01)?;
        expectCalcPx("clamp(50px, 10px, 100px)", 16.0, 800.0, 600.0, 50.0, 0.01)?;
        expectCalcPx("clamp(50px, 200px, 100px)", 16.0, 800.0, 600.0, 100.0, 0.01)?;

        // Case-insensitive math function names and multi-arg min/max
        expectCalcPx("CALC(10PX + 5PX)", 16.0, 800.0, 600.0, 15.0, 0.01)?;
        expectCalcPx("Min(90px, 50px, 2rem, 80px)", 16.0, 800.0, 600.0, 32.0, 0.01)?;
        expectCalcPx("MAX(10px, 2rem, 5vh, 12px)", 16.0, 800.0, 600.0, 32.0, 0.01)?;
    });

    test("2. Mixed units in CssCalc & Cascade percentage/em calc() resolution", () => {
        // Mixed absolute + root + viewport units:
        // 1in (96) + 6pt (8) - 2rem (32) + 10vw (80) = 152px
        expectCalcPx("calc(1in + 6pt - 2rem + 10vw)", 16.0, 800.0, 600.0, 152.0, 0.05)?;

        // 1pc (16) + 10vh (60) + 10vmin (60) + 10vmax (80) = 216px
        expectCalcPx("calc(1pc + 10vh + 10vmin + 10vmax)", 16.0, 800.0, 600.0, 216.0, 0.05)?;

        // Cascade integration with % and em inside calc()
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><div id=\"box\"></div></body></html>";
        doc.parseHtml(html) catch unreachable;

        const css = "#box { font-size: 20px; width: calc(50% - 20px); margin-left: calc(2em + 10px); height: calc(100px / 0); }";
        const sheet = StyleSheet.parse(css, 800.0, false);

        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const box_id = Matcher.querySelector(&doc, "#box");
        expect_eq(box_id != NULL_NODE, true);

        // 50% of 800 = 400; 400 - 20 = 380
        expect_eq(nearF32(styles[box_id].width.resolvePx(800.0, styles[box_id].font_size_px, 16.0, 800.0, 600.0), 380.0, 0.01), true);
        // 2em (2 * 20 = 40) + 10 = 50
        expect_eq(nearF32(styles[box_id].margin_left.resolvePx(800.0, styles[box_id].font_size_px, 16.0, 800.0, 600.0), 50.0, 0.01), true);
        // Division by zero in calc() safely resolves to 0.0
        expect_eq(nearF32(styles[box_id].height.resolvePx(600.0, styles[box_id].font_size_px, 16.0, 800.0, 600.0), 0.0, 0.01), true);
    });

    test("3. Division by zero, malformed calc() & recursion depth protection", () => {
        // Direct and computed division by zero
        expect_eq(CssCalc.evalPx("calc(100px / 0)", 16.0, 800.0, 600.0) == null, true);
        expect_eq(CssCalc.evalPx("calc(100px / (10px - 10px))", 16.0, 800.0, 600.0) == null, true);
        expect_eq(CssCalc.evalPx("calc(10px / calc(2 - 2))", 16.0, 800.0, 600.0) == null, true);

        // Empty / whitespace-only / empty calc()
        expect_eq(CssCalc.evalPx("", 16.0, 800.0, 600.0) == null, true);
        expect_eq(CssCalc.evalPx("   \t\n ", 16.0, 800.0, 600.0) == null, true);
        expect_eq(CssCalc.evalPx("calc()", 16.0, 800.0, 600.0) == null, true);

        // Unbalanced parentheses & trailing garbage
        expect_eq(CssCalc.evalPx("calc(10px + 20px", 16.0, 800.0, 600.0) == null, true);
        expect_eq(CssCalc.evalPx("calc(10px + 20px))", 16.0, 800.0, 600.0) == null, true);
        expect_eq(CssCalc.evalPx("calc(10px) 20px", 16.0, 800.0, 600.0) == null, true);
        expect_eq(CssCalc.evalPx("10px 20px", 16.0, 800.0, 600.0) == null, true);

        // Unknown math function, invalid operator sequence, wrong clamp arity
        expect_eq(CssCalc.evalPx("foo(10px)", 16.0, 800.0, 600.0) == null, true);
        expect_eq(CssCalc.evalPx("calc(10px + * 20px)", 16.0, 800.0, 600.0) == null, true);
        expect_eq(CssCalc.evalPx("calc(+)", 16.0, 800.0, 600.0) == null, true);
        expect_eq(CssCalc.evalPx("clamp(10px, 20px)", 16.0, 800.0, 600.0) == null, true);

        // Excessive nesting depth (> 32) rejected safely without stack overflow
        const deep_expr = "((((((((((((((((((((((((((((((((((10px))))))))))))))))))))))))))))))))))";
        expect_eq(CssCalc.evalPx(deep_expr, 16.0, 800.0, 600.0) == null, true);
    });

    test("4. Hex color edge cases (#rgb, #rgba, #rrggbb, #rrggbbaa & invalid hex)", () => {
        expectColorRgba("#000", @as(u8, 0), @as(u8, 0), @as(u8, 0), @as(u8, 255))?;
        expectColorRgba("#FFF", @as(u8, 255), @as(u8, 255), @as(u8, 255), @as(u8, 255))?;
        expectColorRgba("#AbC", @as(u8, 0xAA), @as(u8, 0xBB), @as(u8, 0xCC), @as(u8, 255))?;
        expectColorRgba("#f0a8", @as(u8, 255), @as(u8, 0), @as(u8, 170), @as(u8, 0x88))?;
        expectColorRgba("#123456", @as(u8, 0x12), @as(u8, 0x34), @as(u8, 0x56), @as(u8, 255))?;
        expectColorRgba("#12345600", @as(u8, 0x12), @as(u8, 0x34), @as(u8, 0x56), @as(u8, 0))?;
        expectColorRgba("  #00ff7f80  ", @as(u8, 0), @as(u8, 255), @as(u8, 127), @as(u8, 128))?;

        // Invalid hex lengths and non-hex characters
        expect_eq(CssColor.parse("#") == null, true);
        expect_eq(CssColor.parse("#1") == null, true);
        expect_eq(CssColor.parse("#12") == null, true);
        expect_eq(CssColor.parse("#12345") == null, true);
        expect_eq(CssColor.parse("#1234567") == null, true);
        expect_eq(CssColor.parse("#123456789") == null, true);
        expect_eq(CssColor.parse("#ggg") == null, true);
        expect_eq(CssColor.parse("#12345z") == null, true);
    });

    test("5. rgb() / rgba() legacy & CSS Color 4 space/slash syntax edge cases", () => {
        expectColorRgba("rgb(255, 0, 128)", @as(u8, 255), @as(u8, 0), @as(u8, 128), @as(u8, 255))?;
        expectColorRgba("RGB( 1 , 2 , 3 )", @as(u8, 1), @as(u8, 2), @as(u8, 3), @as(u8, 255))?;
        expectColorRgba("rgb(100%, 0%, 50%)", @as(u8, 255), @as(u8, 0), @as(u8, 128), @as(u8, 255))?;

        // Modern space-separated and slash alpha syntax
        expectColorRgba("rgb(255 0 128)", @as(u8, 255), @as(u8, 0), @as(u8, 128), @as(u8, 255))?;
        expectColorRgba("rgb(100% 0% 50% / 100%)", @as(u8, 255), @as(u8, 0), @as(u8, 128), @as(u8, 255))?;
        expectColorRgba("rgba(10 20 30 / 0%)", @as(u8, 10), @as(u8, 20), @as(u8, 30), @as(u8, 0))?;

        // Clamping out-of-bounds channels and alpha
        expectColorRgba("rgb(-20, 300, 128)", @as(u8, 0), @as(u8, 255), @as(u8, 128), @as(u8, 255))?;
        expectColorRgba("rgba(10, 20, 30, -0.5)", @as(u8, 10), @as(u8, 20), @as(u8, 30), @as(u8, 0))?;
        expectColorRgba("rgba(10, 20, 30, 150%)", @as(u8, 10), @as(u8, 20), @as(u8, 30), @as(u8, 255))?;

        // Malformed rgb/rgba
        expect_eq(CssColor.parse("rgb(1, 2)") == null, true);
        expect_eq(CssColor.parse("rgb(1, 2, 3") == null, true);
        expect_eq(CssColor.parse("rgb(a, b, c)") == null, true);
        expect_eq(CssColor.parse("rgba()") == null, true);
    });

    test("6. hsl() / hsla() angles, wrap-around, grayscale & named/transparent colors", () => {
        // Angle units: unitless, deg, turn, grad, rad
        expectColorRgba("hsl(240, 100%, 50%)", @as(u8, 0), @as(u8, 0), @as(u8, 255), @as(u8, 255))?;
        expectColorRgba("hsl(0.5turn, 100%, 50%)", @as(u8, 0), @as(u8, 255), @as(u8, 255), @as(u8, 255))?;
        expectColorRgba("hsl(200grad, 100%, 50%)", @as(u8, 0), @as(u8, 255), @as(u8, 255), @as(u8, 255))?;
        expectColorRgba("hsl(3.141592653589793rad, 100%, 50%)", @as(u8, 0), @as(u8, 255), @as(u8, 255), @as(u8, 255))?;

        // Hue wrap-around (< 0 and > 360)
        expectColorRgba("hsl(-120deg, 100%, 50%)", @as(u8, 0), @as(u8, 0), @as(u8, 255), @as(u8, 255))?;
        expectColorRgba("hsl(480deg, 100%, 50%)", @as(u8, 0), @as(u8, 255), @as(u8, 0), @as(u8, 255))?;

        // Saturation 0% -> grayscale, Lightness 0% -> black, 100% -> white
        expectColorRgba("hsl(123deg, 0%, 50%)", @as(u8, 128), @as(u8, 128), @as(u8, 128), @as(u8, 255))?;
        expectColorRgba("hsl(120deg, 100%, 0%)", @as(u8, 0), @as(u8, 0), @as(u8, 0), @as(u8, 255))?;
        expectColorRgba("hsl(120deg, 100%, 100%)", @as(u8, 255), @as(u8, 255), @as(u8, 255), @as(u8, 255))?;

        // Space/slash hsla
        expectColorRgba("hsla(0deg 100% 50% / 0%)", @as(u8, 255), @as(u8, 0), @as(u8, 0), @as(u8, 0))?;
        expect_eq(CssColor.parse("hsl(120deg, 100%)") == null, true);

        // Named colors (boundary entries, case-insensitivity, green vs lime, gray vs grey, transparent)
        expectColorRgba("aliceblue", @as(u8, 240), @as(u8, 248), @as(u8, 255), @as(u8, 255))?;
        expectColorRgba("YELLOWGREEN", @as(u8, 154), @as(u8, 205), @as(u8, 50), @as(u8, 255))?;
        expectColorRgba("green", @as(u8, 0), @as(u8, 128), @as(u8, 0), @as(u8, 255))?;
        expectColorRgba("lime", @as(u8, 0), @as(u8, 255), @as(u8, 0), @as(u8, 255))?;
        expectColorRgba("gray", @as(u8, 128), @as(u8, 128), @as(u8, 128), @as(u8, 255))?;
        expectColorRgba("grey", @as(u8, 128), @as(u8, 128), @as(u8, 128), @as(u8, 255))?;
        expectColorRgba("TRANSPARENT", @as(u8, 0), @as(u8, 0), @as(u8, 0), @as(u8, 0))?;

        // Unknown / unsupported names
        expect_eq(CssColor.parse("notacolor") == null, true);
        expect_eq(CssColor.parse("   ") == null, true);
    });
}
