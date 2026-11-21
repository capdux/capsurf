// test/css/test_color_and_units.cp
// Pure CP white-box unit tests for CssColor (CSS Color 4), CssCalc (calc/min/max/clamp),
// LengthPercentageOrAuto, CSS custom properties var(), and light-dark() cascade resolution.

import { test, expect_eq } from "cap:test";
import { CssColor, CssCalc } from "cap:css";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { Matcher } from "../../../src/css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle, LengthPercentageOrAuto } from "../../../src/css/cascade.cp";

fn nearF64(actual: f64, expected: f64, eps: f64): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn nearF32(actual: f32, expected: f32, eps: f32): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn main() {
    test("1. CssColor parsing (hex, rgb/rgba, hsl, hwb, oklab, oklch, named) & transforms", () => {
        if (c_hex3 := CssColor.parse("#f0a")) {
            expect_eq(c_hex3.r, @as(u8, 255));
            expect_eq(c_hex3.g, @as(u8, 0));
            expect_eq(c_hex3.b, @as(u8, 170));
            expect_eq(c_hex3.a, @as(u8, 255));
        } else {
            expect_eq(false, true);
        }

        if (c_hex8 := CssColor.parse("#11223380")) {
            expect_eq(c_hex8.r, @as(u8, 0x11));
            expect_eq(c_hex8.g, @as(u8, 0x22));
            expect_eq(c_hex8.b, @as(u8, 0x33));
            expect_eq(c_hex8.a, @as(u8, 0x80));
        } else {
            expect_eq(false, true);
        }

        if (c_rgb := CssColor.parse("rgba(10, 20, 30, 0.5)")) {
            expect_eq(c_rgb.r, @as(u8, 10));
            expect_eq(c_rgb.g, @as(u8, 20));
            expect_eq(c_rgb.b, @as(u8, 30));
            expect_eq(c_rgb.a >= @as(u8, 127) && c_rgb.a <= @as(u8, 128), true);
        } else {
            expect_eq(false, true);
        }

        if (c_hsl := CssColor.parse("hsl(120deg, 100%, 50%)")) {
            expect_eq(c_hsl.r, @as(u8, 0));
            expect_eq(c_hsl.g, @as(u8, 255));
            expect_eq(c_hsl.b, @as(u8, 0));
            expect_eq(c_hsl.a, @as(u8, 255));
        } else {
            expect_eq(false, true);
        }

        if (c_hwb := CssColor.parse("hwb(0 0% 0%)")) {
            expect_eq(c_hwb.r, @as(u8, 255));
            expect_eq(c_hwb.g, @as(u8, 0));
            expect_eq(c_hwb.b, @as(u8, 0));
        } else {
            expect_eq(false, true);
        }

        expect_eq(CssColor.parse("oklab(0.6 0.1 -0.1)") != null, true);
        expect_eq(CssColor.parse("oklch(0.7 0.15 120deg)") != null, true);

        if (c_named := CssColor.parse("rebeccapurple")) {
            expect_eq(c_named.r, @as(u8, 102));
            expect_eq(c_named.g, @as(u8, 51));
            expect_eq(c_named.b, @as(u8, 153));
            expect_eq(c_named.a, @as(u8, 255));
        } else {
            expect_eq(false, true);
        }

        if (c_trans := CssColor.parse("transparent")) {
            expect_eq(c_trans.a, @as(u8, 0));
        } else {
            expect_eq(false, true);
        }

        expect_eq(CssColor.parse("") == null, true);
        expect_eq(CssColor.parse("notacolor") == null, true);

        const base: CssColor = CssColor.rgb(100, 150, 200);
        const faded: CssColor = base.withAlpha(64);
        expect_eq(faded.a, @as(u8, 64));
        expect_eq(faded.r, @as(u8, 100));

        const mixed: CssColor = CssColor.rgb(0, 0, 0).mix(CssColor.rgb(200, 100, 50), 0.5);
        expect_eq(mixed.r >= @as(u8, 99) && mixed.r <= @as(u8, 101), true);
        expect_eq(mixed.g >= @as(u8, 49) && mixed.g <= @as(u8, 51), true);
        expect_eq(mixed.b >= @as(u8, 24) && mixed.b <= @as(u8, 26), true);
    });

    test("2. CssCalc (calc, min, max, clamp) & LengthPercentageOrAuto resolution", () => {
        if (v1 := CssCalc.evalPx("calc(10px + 2rem * 3)", 16.0, 800.0, 600.0)) {
            expect_eq(nearF64(v1, 106.0, 0.01), true);
        } else {
            expect_eq(false, true);
        }

        if (v2 := CssCalc.evalPx("min(100px, 10vw, 5rem)", 16.0, 800.0, 600.0)) {
            expect_eq(nearF64(v2, 80.0, 0.01), true);
        } else {
            expect_eq(false, true);
        }

        if (v3 := CssCalc.evalPx("max(20px, 5vh, 2rem)", 16.0, 800.0, 600.0)) {
            expect_eq(nearF64(v3, 32.0, 0.01), true);
        } else {
            expect_eq(false, true);
        }

        if (v4 := CssCalc.evalPx("clamp(20px, 10vw, 60px)", 16.0, 800.0, 600.0)) {
            expect_eq(nearF64(v4, 60.0, 0.01), true);
        } else {
            expect_eq(false, true);
        }

        if (v5 := CssCalc.evalPx("clamp(20px, 5vw, 60px)", 16.0, 800.0, 600.0)) {
            expect_eq(nearF64(v5, 40.0, 0.01), true);
        } else {
            expect_eq(false, true);
        }

        if (lpa := LengthPercentageOrAuto.parse("calc(50% - 10px)")) {
            const px: f32 = lpa.resolvePx(200.0, 14.0, 16.0, 800.0, 600.0);
            expect_eq(nearF32(px, 90.0, 0.01), true);
        } else {
            expect_eq(false, true);
        }

        if (lpa_em := LengthPercentageOrAuto.parse("2em")) {
            const px_em: f32 = lpa_em.resolvePx(200.0, 14.0, 16.0, 800.0, 600.0);
            expect_eq(nearF32(px_em, 28.0, 0.01), true);
        } else {
            expect_eq(false, true);
        }
    });

    test("3. Cascade.computeStyles custom properties var() inheritance & fallback", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><div id=\"root\"><div id=\"box\"></div><div id=\"fb\"></div></div></body></html>";
        doc.parseHtml(html) catch unreachable;

        const css =
            "#root { --accent: #102030; --box-w: 120px; }\n" +
            "#box { color: var(--accent); width: var(--box-w); }\n" +
            "#fb { color: var(--missing, #ff0000); width: var(--missing-w, 80px); }\n";
        const sheet = StyleSheet.parse(css, 800.0, false);

        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const box_id = Matcher.querySelector(&doc, "#box");
        const fb_id = Matcher.querySelector(&doc, "#fb");
        expect_eq(box_id != NULL_NODE, true);
        expect_eq(fb_id != NULL_NODE, true);

        expect_eq(styles[box_id].color.r, @as(u8, 0x10));
        expect_eq(styles[box_id].color.g, @as(u8, 0x20));
        expect_eq(styles[box_id].color.b, @as(u8, 0x30));
        expect_eq(nearF32(styles[box_id].width.resolvePx(800.0, 16.0, 16.0, 800.0, 600.0), 120.0, 0.01), true);

        expect_eq(styles[fb_id].color.r, @as(u8, 255));
        expect_eq(styles[fb_id].color.g, @as(u8, 0));
        expect_eq(styles[fb_id].color.b, @as(u8, 0));
        expect_eq(nearF32(styles[fb_id].width.resolvePx(800.0, 16.0, 16.0, 800.0, 600.0), 80.0, 0.01), true);
    });

    test("4. Cascade.computeStyles color-scheme & light-dark() resolution", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><div id=\"light-box\"></div><div id=\"dark-box\"></div></body></html>";
        doc.parseHtml(html) catch unreachable;

        const css =
            "#light-box { color-scheme: light; color: light-dark(#111111, #eeeeee); background-color: light-dark(white, black); }\n" +
            "#dark-box { color-scheme: dark; color: light-dark(#111111, #eeeeee); background-color: light-dark(white, black); }\n";
        const sheet = StyleSheet.parse(css, 800.0, false);

        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const light_id = Matcher.querySelector(&doc, "#light-box");
        const dark_id = Matcher.querySelector(&doc, "#dark-box");
        expect_eq(light_id != NULL_NODE, true);
        expect_eq(dark_id != NULL_NODE, true);

        expect_eq(styles[light_id].color.r, @as(u8, 0x11));
        expect_eq(styles[light_id].color.g, @as(u8, 0x11));
        expect_eq(styles[light_id].color.b, @as(u8, 0x11));
        expect_eq(styles[light_id].background_color.r, @as(u8, 255));
        expect_eq(styles[light_id].background_color.g, @as(u8, 255));
        expect_eq(styles[light_id].background_color.b, @as(u8, 255));

        expect_eq(styles[dark_id].color.r, @as(u8, 0xEE));
        expect_eq(styles[dark_id].color.g, @as(u8, 0xEE));
        expect_eq(styles[dark_id].color.b, @as(u8, 0xEE));
        expect_eq(styles[dark_id].background_color.r, @as(u8, 0));
        expect_eq(styles[dark_id].background_color.g, @as(u8, 0));
        expect_eq(styles[dark_id].background_color.b, @as(u8, 0));
    });
}
