// test/css/test_font_face_and_ua.cp
// Pure CP white-box unit tests for src/css/font_face.cp (@font-face parsing,
// UnicodeRange, CORS gating, WebfontStyleMetrics, parsePseudoContentValue)
// and src/css/ua_stylesheet.cp (UaStylesheet.applyUaDefaults).

import { test, expect_eq } from "cap:test";
import {
    UnicodeRange,
    FontFaceRule,
    WebfontStyleMetrics,
    parsePseudoContentValue
} from "../../src/css/font_face.cp";
import { UaStylesheet } from "../../src/css/ua_stylesheet.cp";
import { Document, NodeData, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { CssColor } from "cap:css";
import { ComputedStyle, Display, Visibility } from "../../src/css/cascade.cp";

fn nearF32(actual: f32, expected: f32, eps: f32): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn nearF64(actual: f64, expected: f64, eps: f64): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn strEq(a: []const u8, b: []const u8): bool {
    return std.mem.eql(u8, a, b);
}

fn checkUaDisplay(doc: *const Document, id: []const u8, expect_none: bool, expect_inline: bool)! {
    const nid = doc.getElementById(id);
    expect_eq(nid != NULL_NODE, true);
    let st = ComputedStyle{};
    UaStylesheet.applyUaDefaults(&doc.nodes.items[nid], &st, null);
    expect_eq(st.display.isNone(), expect_none);
    expect_eq(st.display.isInline(), expect_inline);
}

fn checkUaHeadingFs(doc: *const Document, id: []const u8, parent: *const ComputedStyle, expected_fs: f32)! {
    const nid = doc.getElementById(id);
    expect_eq(nid != NULL_NODE, true);
    let st = ComputedStyle{};
    UaStylesheet.applyUaDefaults(&doc.nodes.items[nid], &st, parent);
    expect_eq(nearF32(st.font_size_px, expected_fs, 0.01), true);
}

fn main() {
    test("1. UnicodeRange.parseSingle & matches (single, range, wildcard, invalid)", () => {
        // Single codepoint
        if (r1 := UnicodeRange.parseSingle("  U+0041  ")) {
            expect_eq(r1.start, @as(u32, 0x0041));
            expect_eq(r1.end, @as(u32, 0x0041));
            expect_eq(r1.matches(0x0041), true);
            expect_eq(r1.matches(0x0040), false);
            expect_eq(r1.matches(0x0042), false);
        } else {
            expect_eq(false, true);
        }

        // Lowercase u+ prefix
        if (r2 := UnicodeRange.parseSingle("u+a")) {
            expect_eq(r2.start, @as(u32, 0xA));
            expect_eq(r2.end, @as(u32, 0xA));
        } else {
            expect_eq(false, true);
        }

        // Explicit range
        if (r3 := UnicodeRange.parseSingle("U+0400-04FF")) {
            expect_eq(r3.start, @as(u32, 0x0400));
            expect_eq(r3.end, @as(u32, 0x04FF));
            expect_eq(r3.matches(0x03FF), false);
            expect_eq(r3.matches(0x0400), true);
            expect_eq(r3.matches(0x0455), true);
            expect_eq(r3.matches(0x04FF), true);
            expect_eq(r3.matches(0x0500), false);
        } else {
            expect_eq(false, true);
        }

        // Wildcard form
        if (r4 := UnicodeRange.parseSingle("U+4??")) {
            expect_eq(r4.start, @as(u32, 0x400));
            expect_eq(r4.end, @as(u32, 0x4FF));
            expect_eq(r4.matches(0x3FF), false);
            expect_eq(r4.matches(0x400), true);
            expect_eq(r4.matches(0x4FF), true);
            expect_eq(r4.matches(0x500), false);
        } else {
            expect_eq(false, true);
        }

        // Invalid forms
        expect_eq(UnicodeRange.parseSingle("0041") == null, true);
        expect_eq(UnicodeRange.parseSingle("U+") == null, true);
        expect_eq(UnicodeRange.parseSingle("U+0500-0400") == null, true);
        expect_eq(UnicodeRange.parseSingle("U+4?A") == null, true);
        expect_eq(UnicodeRange.parseSingle("U+1234567") == null, true);
        expect_eq(UnicodeRange.parseSingle("U+GHIJ") == null, true);
    });

    test("2. FontFaceRule.parseBlock: data URI semicolons, CORS gating, unicode-range, advance_ratio", () => {
        // Missing font-family -> null
        expect_eq(FontFaceRule.parseBlock("src: url(/a.woff2);") == null, true);

        // Data URI containing ';' inside url(...) must not truncate declaration
        if (ff_data := FontFaceRule.parseBlock("font-family: 'DataMono'; src: url(data:font/woff2;base64,d09GMgAB);")) {
            expect_eq(strEq(ff_data.family_buf[0..ff_data.family_len], "DataMono"), true);
            expect_eq(strEq(ff_data.src_buf[0..ff_data.src_len], "data:font/woff2;base64,d09GMgAB"), true);
            expect_eq(ff_data.is_loaded, true);
            expect_eq(ff_data.is_cors_blocked, false);
            expect_eq(ff_data.matchesFamilyName("  \"datamono\" "), true);
            expect_eq(ff_data.matchesFamilyName("other"), false);
            expect_eq(ff_data.matchesCodepoint('A'), true);
            expect_eq(nearF32(ff_data.advance_ratio, 0.60, 0.001), true);
        } else {
            expect_eq(false, true);
        }

        // Cross-origin without CORS grant -> blocked
        if (ff_blocked := FontFaceRule.parseBlock("font-family: BlockedFont; src: url(\"https://cdn.example.com/font.woff2\");")) {
            expect_eq(ff_blocked.is_loaded, false);
            expect_eq(ff_blocked.is_cors_blocked, true);
        } else {
            expect_eq(false, true);
        }

        // Cross-origin with acao=1 / cors=1 -> allowed
        expect_eq(FontFaceRule.isCrossOriginCorsAllowed("https://cdn.example.com/f.woff2?acao=1"), true);
        expect_eq(FontFaceRule.isCrossOriginCorsAllowed("https://cdn.example.com/f.woff2?cors=1"), true);
        expect_eq(FontFaceRule.isCrossOriginCorsAllowed("http://127.0.0.1:8080/f.woff2"), true);
        expect_eq(FontFaceRule.isCrossOriginCorsAllowed("http://localhost/f.woff2"), true);
        expect_eq(FontFaceRule.isCrossOriginCorsAllowed("https://ks.invalid/f.woff2"), true);
        expect_eq(FontFaceRule.isCrossOriginCorsAllowed(""), false);

        // Bungee advance_ratio (0.85) + multiple unicode-ranges
        if (ff_bungee := FontFaceRule.parseBlock("font-family: \"MyBungee\"; src: url(/fonts/bungee.woff2); unicode-range: U+0041, U+0400-04FF, U+30??;")) {
            expect_eq(nearF32(ff_bungee.advance_ratio, 0.85, 0.001), true);
            expect_eq(ff_bungee.range_count, @as(usize, 3));
            expect_eq(ff_bungee.matchesCodepoint(0x0041), true);
            expect_eq(ff_bungee.matchesCodepoint(0x0042), false);
            expect_eq(ff_bungee.matchesCodepoint(0x0410), true);
            expect_eq(ff_bungee.matchesCodepoint(0x3042), true);
        } else {
            expect_eq(false, true);
        }
    });

    test("3. WebfontStyleMetrics.resolveForFamily & advanceRatioForCodepoint", () => {
        let faces: [4]FontFaceRule = [FontFaceRule{}; 4];
        faces[0] = FontFaceRule.parseBlock("font-family: Blocked; src: url(https://evil.invalid/a.woff2);").?;
        faces[1] = FontFaceRule.parseBlock("font-family: Hybrid; src: url(/fonts/bungee.woff2); unicode-range: U+0041-005A;").?;
        faces[2] = FontFaceRule.parseBlock("font-family: Hybrid; src: url(/fonts/cyr.woff2); unicode-range: U+0400-04FF;").?;
        faces[3] = FontFaceRule.parseBlock("font-family: Hybrid; src: url(/fonts/default.woff2);").?;

        // Blocked font only -> not loaded
        const m_blocked = WebfontStyleMetrics.resolveForFamily("Blocked", faces[0..4]);
        expect_eq(m_blocked.loaded, false);
        expect_eq(nearF32(m_blocked.advanceRatioForCodepoint('A'), 0.0, 0.001), true);

        // Fallback list skips Blocked and resolves Hybrid
        const m_hybrid = WebfontStyleMetrics.resolveForFamily("Blocked, 'Hybrid', sans-serif", faces[0..4]);
        expect_eq(m_hybrid.loaded, true);
        expect_eq(m_hybrid.has_unrestricted, true);
        expect_eq(nearF32(m_hybrid.advanceRatioForCodepoint('A'), 0.85, 0.001), true);
        expect_eq(nearF32(m_hybrid.advanceRatioForCodepoint(0x0410), 0.60, 0.001), true);
        // Unmatched codepoint falls back to unrestricted default_ratio (0.60)
        expect_eq(nearF32(m_hybrid.advanceRatioForCodepoint(0x4E2D), 0.60, 0.001), true);

        // Restricted-only family (no unrestricted face) returns 0.0 for unmatched codepoint
        const m_restricted = WebfontStyleMetrics.resolveForFamily("Hybrid", faces[1..3]);
        expect_eq(m_restricted.loaded, true);
        expect_eq(m_restricted.has_unrestricted, false);
        expect_eq(nearF32(m_restricted.advanceRatioForCodepoint('A'), 0.85, 0.001), true);
        expect_eq(nearF32(m_restricted.advanceRatioForCodepoint('a'), 0.0, 0.001), true);
    });

    test("4. parsePseudoContentValue (none, normal, quoted, escaped, alt-text, truncation)", () => {
        let buf: [32]u8 = [0; 32];

        expect_eq(parsePseudoContentValue("", buf[0..]) == null, true);
        expect_eq(parsePseudoContentValue("none", buf[0..]) == null, true);
        expect_eq(parsePseudoContentValue("NORMAL", buf[0..]) == null, true);
        expect_eq(parsePseudoContentValue("counter(item)", buf[0..]) == null, true);

        if (len1 := parsePseudoContentValue("  \"Hello\" ' World'  ", buf[0..])) {
            expect_eq(strEq(buf[0..len1], "Hello World"), true);
        } else {
            expect_eq(false, true);
        }

        if (len2 := parsePseudoContentValue("\"A\\\"B\" / \"alt\"", buf[0..])) {
            expect_eq(strEq(buf[0..len2], "A\"B"), true);
        } else {
            expect_eq(false, true);
        }

        // Small buffer truncation
        let small: [3]u8 = [0; 3];
        if (len3 := parsePseudoContentValue("\"abcdef\"", small[0..])) {
            expect_eq(len3, @as(usize, 3));
            expect_eq(strEq(small[0..len3], "abc"), true);
        } else {
            expect_eq(false, true);
        }
    });

    test("5. UaStylesheet.applyUaDefaults: display categories, [hidden], <dialog>, custom tags", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><head><title>T</title><template></template></head><body>" +
            "<dialog id=\"dlg-closed\">c</dialog>" +
            "<dialog id=\"dlg-open\" open>o</dialog>" +
            "<div id=\"hid\" hidden>h</div>" +
            "<span id=\"inl\">i</span>" +
            "<img id=\"im\" />" +
            "<table id=\"tbl\"><tr id=\"tr\"><td id=\"td\">x</td></tr></table>" +
            "<custom-card id=\"cust\">y</custom-card>" +
            "</body></html>";
        doc.parseHtml(html) catch unreachable;

        checkUaDisplay(&doc, "dlg-closed", true, false)?;
        checkUaDisplay(&doc, "dlg-open", false, false)?;
        checkUaDisplay(&doc, "hid", true, false)?;
        checkUaDisplay(&doc, "inl", false, true)?;
        checkUaDisplay(&doc, "im", false, false)?;
        checkUaDisplay(&doc, "tbl", false, false)?;
        checkUaDisplay(&doc, "tr", false, false)?;
        checkUaDisplay(&doc, "td", false, false)?;
        checkUaDisplay(&doc, "cust", false, false)?;
    });

    test("6. UaStylesheet.applyUaDefaults: semantic typography, margins, and parent inheritance", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body>" +
            "<h1 id=\"h1\">1</h1><h2 id=\"h2\">2</h2><h3 id=\"h3\">3</h3>" +
            "<h4 id=\"h4\">4</h4><h5 id=\"h5\">5</h5><h6 id=\"h6\">6</h6>" +
            "<p id=\"p\">p</p><ul id=\"ul\"><li>x</li></ul><blockquote id=\"bq\">q</blockquote><hr id=\"hr\"/>" +
            "</body></html>";
        doc.parseHtml(html) catch unreachable;

        // Parent style with custom visibility, color, font_size_px, and font_family
        let parent = ComputedStyle{};
        parent.visibility = .Hidden;
        parent.font_size_px = 20.0;
        parent.color = CssColor.rgba(17, 34, 51, 255);
        const fam = "Inter";
        @memcpy(parent.font_family_buf[0..fam.len], fam);
        parent.font_family_len = fam.len;

        // H1 keeps 32.0 font_size_px despite parent font_size_px=20.0, but inherits color/visibility/font_family
        const h1_id = doc.getElementById("h1");
        let st_h1 = ComputedStyle{};
        UaStylesheet.applyUaDefaults(&doc.nodes.items[h1_id], &st_h1, &parent);
        expect_eq(nearF32(st_h1.font_size_px, 32.0, 0.01), true);
        expect_eq(st_h1.visibility == .Hidden, true);
        expect_eq(st_h1.color.r, @as(u8, 17));
        expect_eq(st_h1.color.g, @as(u8, 34));
        expect_eq(st_h1.color.b, @as(u8, 51));
        expect_eq(st_h1.color.a, @as(u8, 255));
        expect_eq(strEq(st_h1.font_family_buf[0..st_h1.font_family_len], "Inter"), true);

        // H2..H6 font sizes
        checkUaHeadingFs(&doc, "h2", &parent, 24.0)?;
        checkUaHeadingFs(&doc, "h3", &parent, 18.72)?;
        checkUaHeadingFs(&doc, "h4", &parent, 16.0)?;
        checkUaHeadingFs(&doc, "h5", &parent, 13.28)?;
        checkUaHeadingFs(&doc, "h6", &parent, 10.72)?;

        // <p> inherits parent font_size_px (20.0) and gets 16px top/bottom margin
        const p_id = doc.getElementById("p");
        let st_p = ComputedStyle{};
        UaStylesheet.applyUaDefaults(&doc.nodes.items[p_id], &st_p, &parent);
        expect_eq(nearF32(st_p.font_size_px, 20.0, 0.01), true);
        expect_eq(nearF64(st_p.margin_top.length.value, 16.0, 0.01), true);
        expect_eq(nearF64(st_p.margin_bottom.length.value, 16.0, 0.01), true);

        // <ul> gets 40px padding_left, <blockquote> gets 40px left/right margin, <hr> gets 1px border_top_width
        const ul_id = doc.getElementById("ul");
        let st_ul = ComputedStyle{};
        UaStylesheet.applyUaDefaults(&doc.nodes.items[ul_id], &st_ul, null);
        expect_eq(nearF64(st_ul.padding_left.length.value, 40.0, 0.01), true);

        const bq_id = doc.getElementById("bq");
        let st_bq = ComputedStyle{};
        UaStylesheet.applyUaDefaults(&doc.nodes.items[bq_id], &st_bq, null);
        expect_eq(nearF64(st_bq.margin_left.length.value, 40.0, 0.01), true);
        expect_eq(nearF64(st_bq.margin_right.length.value, 40.0, 0.01), true);

        const hr_id = doc.getElementById("hr");
        let st_hr = ComputedStyle{};
        UaStylesheet.applyUaDefaults(&doc.nodes.items[hr_id], &st_hr, null);
        expect_eq(nearF32(st_hr.border_top_width, 1.0, 0.01), true);
    });
}
