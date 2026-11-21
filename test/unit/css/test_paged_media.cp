// test/css/test_paged_media.cp
// Pure CP white-box unit tests for Paged Media (@page margins, break-before/after/inside),
// UTF-8 validation, and ISO-8859-15 / Latin-1 legacy CSS transcoding.

import { test, expect_eq } from "cap:test";
import { CssProperty } from "cap:css";
import { BreakValue, BreakInsideValue, PageMarginRule, isValidUtf8, decodeLegacyCssToUtf8 } from "../../../src/css/paged_media.cp";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { Matcher } from "../../../src/css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../../src/css/cascade.cp";

fn nearF32(actual: f32, expected: f32, eps: f32): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn main() {
    test("1. BreakValue & BreakInsideValue parsing and forcesPageBreak", () => {
        if (v := BreakValue.parse("  AUTO \n")) {
            expect_eq(v == BreakValue.Auto, true);
            expect_eq(v.forcesPageBreak(), false);
        } else {
            expect_eq(false, true);
        }

        if (v := BreakValue.parse("page")) {
            expect_eq(v == BreakValue.Page, true);
            expect_eq(v.forcesPageBreak(), true);
        } else {
            expect_eq(false, true);
        }

        if (v := BreakValue.parse("ALWAYS")) {
            expect_eq(v == BreakValue.Always, true);
            expect_eq(v.forcesPageBreak(), true);
        } else {
            expect_eq(false, true);
        }

        if (v := BreakValue.parse("avoid-page")) {
            expect_eq(v == BreakValue.Avoid, true);
            expect_eq(v.forcesPageBreak(), false);
        } else {
            expect_eq(false, true);
        }

        if (v := BreakValue.parse("left")) {
            expect_eq(v == BreakValue.Left, true);
            expect_eq(v.forcesPageBreak(), true);
        } else {
            expect_eq(false, true);
        }

        if (v := BreakValue.parse("right")) {
            expect_eq(v == BreakValue.Right, true);
            expect_eq(v.forcesPageBreak(), true);
        } else {
            expect_eq(false, true);
        }

        if (v := BreakValue.parse("column")) {
            expect_eq(v == BreakValue.Column, true);
            expect_eq(v.forcesPageBreak(), false);
        } else {
            expect_eq(false, true);
        }

        expect_eq(BreakValue.parse("") == null, true);
        expect_eq(BreakValue.parse("unknown") == null, true);

        if (bi := BreakInsideValue.parse("auto")) {
            expect_eq(bi == BreakInsideValue.Auto, true);
        } else {
            expect_eq(false, true);
        }
        if (bi := BreakInsideValue.parse("avoid-column")) {
            expect_eq(bi == BreakInsideValue.Avoid, true);
        } else {
            expect_eq(false, true);
        }
        expect_eq(BreakInsideValue.parse("page") == null, true);
    });

    test("2. PageMarginRule length units & shorthand/longhand applyDecl", () => {
        if (px := PageMarginRule.parseLengthPx("0")) {
            expect_eq(nearF32(px, 0.0, 0.001), true);
        } else {
            expect_eq(false, true);
        }
        if (px := PageMarginRule.parseLengthPx("1in")) {
            expect_eq(nearF32(px, 96.0, 0.01), true);
        } else {
            expect_eq(false, true);
        }
        if (px := PageMarginRule.parseLengthPx("2.54cm")) {
            expect_eq(nearF32(px, 96.0, 0.05), true);
        } else {
            expect_eq(false, true);
        }
        if (px := PageMarginRule.parseLengthPx("25.4mm")) {
            expect_eq(nearF32(px, 96.0, 0.05), true);
        } else {
            expect_eq(false, true);
        }
        if (px := PageMarginRule.parseLengthPx("72pt")) {
            expect_eq(nearF32(px, 96.0, 0.05), true);
        } else {
            expect_eq(false, true);
        }
        if (px := PageMarginRule.parseLengthPx("2pc")) {
            expect_eq(nearF32(px, 32.0, 0.01), true);
        } else {
            expect_eq(false, true);
        }
        if (px := PageMarginRule.parseLengthPx("101.6q")) {
            expect_eq(nearF32(px, 96.0, 0.05), true);
        } else {
            expect_eq(false, true);
        }
        expect_eq(PageMarginRule.parseLengthPx("10") == null, true);
        expect_eq(PageMarginRule.parseLengthPx("10em") == null, true);
        expect_eq(PageMarginRule.parseLengthPx("") == null, true);

        let pm = PageMarginRule{};
        const p_shorthand = CssProperty{ name: "margin", value: "10px 20px 30px" };
        pm.applyDecl(&p_shorthand);
        expect_eq(pm.has_margin_top, true);
        expect_eq(pm.has_margin_right, true);
        expect_eq(pm.has_margin_bottom, true);
        expect_eq(pm.has_margin_left, true);
        expect_eq(nearF32(pm.margin_top_px, 10.0, 0.01), true);
        expect_eq(nearF32(pm.margin_right_px, 20.0, 0.01), true);
        expect_eq(nearF32(pm.margin_left_px, 20.0, 0.01), true);
        expect_eq(nearF32(pm.margin_bottom_px, 30.0, 0.01), true);

        const p_left = CssProperty{ name: "margin-left", value: "1in" };
        pm.applyDecl(&p_left);
        expect_eq(nearF32(pm.margin_left_px, 96.0, 0.01), true);
    });

    test("3. @page stylesheet parsing & break-* cascade", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><div id=\"a\">A</div><div id=\"b\">B</div></body></html>";
        doc.parseHtml(html) catch unreachable;

        const css =
            "@page { margin: 1in 20px; margin-bottom: 36pt; }\n" +
            "#a { break-before: page; break-inside: avoid-page; }\n" +
            "#b { break-after: always; }\n";
        const sheet = StyleSheet.parse(css, 800.0, false);

        expect_eq(sheet.page_margin.has_margin_top, true);
        expect_eq(sheet.page_margin.has_margin_right, true);
        expect_eq(sheet.page_margin.has_margin_bottom, true);
        expect_eq(sheet.page_margin.has_margin_left, true);
        expect_eq(nearF32(sheet.page_margin.margin_top_px, 96.0, 0.05), true);
        expect_eq(nearF32(sheet.page_margin.margin_right_px, 20.0, 0.05), true);
        expect_eq(nearF32(sheet.page_margin.margin_bottom_px, 48.0, 0.05), true);
        expect_eq(nearF32(sheet.page_margin.margin_left_px, 20.0, 0.05), true);

        let styles: [32]ComputedStyle = [ComputedStyle{}; 32];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const a_id = Matcher.querySelector(&doc, "#a");
        const b_id = Matcher.querySelector(&doc, "#b");
        expect_eq(a_id != NULL_NODE, true);
        expect_eq(b_id != NULL_NODE, true);
        expect_eq(styles[a_id].break_before == BreakValue.Page, true);
        expect_eq(styles[a_id].break_inside == BreakInsideValue.Avoid, true);
        expect_eq(styles[b_id].break_after == BreakValue.Always, true);
    });

    test("4. isValidUtf8 & decodeLegacyCssToUtf8 (ISO-8859-15 / Latin-1)", () => {
        expect_eq(isValidUtf8("body { color: red; }"), true);
        // Valid 2-byte (é = C3 A9), 3-byte (€ = E2 82 AC), 4-byte (😀 = F0 9F 98 80)
        const valid_utf8: [9]u8 = [0xC3, 0xA9, 0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x98, 0x80];
        expect_eq(isValidUtf8(valid_utf8[0..]), true);

        // Invalid UTF-8 sequences: lone continuation, truncated 2-byte, overlong E0 80, surrogate ED A0
        const bad_cont: [1]u8 = [0x80];
        const bad_trunc: [1]u8 = [0xC3];
        const bad_overlong: [3]u8 = [0xE0, 0x80, 0x80];
        const bad_surrogate: [3]u8 = [0xED, 0xA0, 0x80];
        expect_eq(isValidUtf8(bad_cont[0..]), false);
        expect_eq(isValidUtf8(bad_trunc[0..]), false);
        expect_eq(isValidUtf8(bad_overlong[0..]), false);
        expect_eq(isValidUtf8(bad_surrogate[0..]), false);

        // Valid UTF-8 passthrough
        let out_buf: [64]u8 = [0; 64];
        const n_pass = decodeLegacyCssToUtf8(valid_utf8[0..], out_buf[0..]);
        expect_eq(n_pass, @as(usize, 9));
        expect_eq(std.mem.eql(u8, out_buf[0..n_pass], valid_utf8[0..]), true);

        // ISO-8859-15: 'A' (0x41), € (0xA4 -> E2 82 AC), Š (0xA6 -> C5 A0), é (0xE9 -> C3 A9)
        const legacy_bytes: [4]u8 = [0x41, 0xA4, 0xA6, 0xE9];
        expect_eq(isValidUtf8(legacy_bytes[0..]), false);
        const n_dec = decodeLegacyCssToUtf8(legacy_bytes[0..], out_buf[0..]);
        expect_eq(n_dec, @as(usize, 8));
        const expected_utf8: [8]u8 = [0x41, 0xE2, 0x82, 0xAC, 0xC5, 0xA0, 0xC3, 0xA9];
        expect_eq(std.mem.eql(u8, out_buf[0..n_dec], expected_utf8[0..]), true);
        expect_eq(isValidUtf8(out_buf[0..n_dec]), true);

        // Destination buffer capacity bound: 1 byte 'A' + 3-byte '€' doesn't fit in 3-byte dst
        let small_buf: [3]u8 = [0; 3];
        const n_small = decodeLegacyCssToUtf8(legacy_bytes[0..], small_buf[0..]);
        expect_eq(n_small, @as(usize, 1));
        expect_eq(small_buf[0], @as(u8, 'A'));
    });
}
