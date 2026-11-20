// test/css/test_bidi.cp
// Native CP unit tests for UAX #9 Bidirectional Algorithm, HTML dir="auto"
// first-strong scanning, <bdi> isolation, and <bdo> forced override.

import { Document } from "../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";
import {
    Direction,
    UnicodeBidi,
    BidiClass,
    CP_LRM,
    CP_RLM,
    CP_LRE,
    CP_RLE,
    CP_PDF,
    CP_LRO,
    CP_RLO,
    CP_LRI,
    CP_RLI,
    CP_FSI,
    CP_PDI,
    decodeUtf8Next,
    isBidiFormatControl,
    textHasBidiSignal,
    bidiClassOf,
    resolveFirstStrongDirection,
    resolveParagraphFirstStrong,
    reorderVisual,
} from "../../src/css/bidi.cp";

fn assertTrue(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("ASSERT FAILED: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn assertOrder(actual: []const usize, expected: []const usize, msg: []const u8) {
    if (actual.len != expected.len) {
        std.debug.print("ASSERT FAILED ({s}): len {d} != {d}\n", .{ msg, actual.len, expected.len });
        std.process.exit(1);
    }
    let i: usize = 0;
    while (i < actual.len) : (i += 1) {
        if (actual[i] != expected[i]) {
            std.debug.print("ASSERT FAILED ({s}): idx {d} got {d}, expected {d}\n", .{ msg, i, actual[i], expected[i] });
            std.process.exit(1);
        }
    }
}

fn testParsersAndClasses() {
    assertTrue(Direction.parse("ltr") == Direction.Ltr, "parse ltr");
    assertTrue(Direction.parse("  RTL ") == Direction.Rtl, "parse RTL");
    assertTrue(Direction.parse("auto") == null, "Direction.parse rejects auto");
    assertTrue(Direction.Rtl.isRtl(), "Rtl.isRtl");
    assertTrue(!Direction.Ltr.isRtl(), "!Ltr.isRtl");

    assertTrue(UnicodeBidi.parse("normal") == UnicodeBidi.Normal, "parse normal");
    assertTrue(UnicodeBidi.parse("embed") == UnicodeBidi.Embed, "parse embed");
    assertTrue(UnicodeBidi.parse("isolate") == UnicodeBidi.Isolate, "parse isolate");
    assertTrue(UnicodeBidi.parse("bidi-override") == UnicodeBidi.BidiOverride, "parse bidi-override");
    assertTrue(UnicodeBidi.parse("ISOLATE-OVERRIDE") == UnicodeBidi.IsolateOverride, "parse isolate-override");
    assertTrue(UnicodeBidi.parse("plaintext") == UnicodeBidi.Plaintext, "parse plaintext");
    assertTrue(UnicodeBidi.parse("unknown") == null, "reject unknown unicode-bidi");

    assertTrue(bidiClassOf('A') == BidiClass.L, "ASCII A -> L");
    assertTrue(bidiClassOf('5') == BidiClass.EN, "ASCII 5 -> EN");
    assertTrue(bidiClassOf(0x05D0) == BidiClass.R, "Hebrew Alef -> R");
    assertTrue(bidiClassOf(0x0627) == BidiClass.AL, "Arabic Alef -> AL");
    assertTrue(bidiClassOf(0x0665) == BidiClass.AN, "Arabic-Indic digit -> AN");
    assertTrue(bidiClassOf(CP_LRI) == BidiClass.LRI, "CP_LRI");
    assertTrue(bidiClassOf(CP_PDI) == BidiClass.PDI, "CP_PDI");

    assertTrue(isBidiFormatControl(CP_LRM), "LRM is format control");
    assertTrue(isBidiFormatControl(CP_RLO), "RLO is format control");
    assertTrue(isBidiFormatControl(CP_FSI), "FSI is format control");
    assertTrue(!isBidiFormatControl('A'), "A is not format control");

    assertTrue(!textHasBidiSignal("Hello 123!"), "pure ASCII has no bidi signal");
    assertTrue(textHasBidiSignal("Hello שלום"), "Hebrew triggers bidi signal");
    assertTrue(textHasBidiSignal("مرحبا"), "Arabic triggers bidi signal");
    assertTrue(textHasBidiSignal("a\u{200F}b"), "RLM triggers bidi signal");

    // UTF-8 decode check on Hebrew Alef (0xD7 0x90 -> 0x05D0)
    const hebrew = "\u{05D0}\u{05D1}";
    let pos: usize = 0;
    assertTrue(decodeUtf8Next(hebrew, &pos) == 0x05D0, "decodeUtf8Next Alef");
    assertTrue(pos == 2, "decodeUtf8Next pos == 2");
    assertTrue(decodeUtf8Next(hebrew, &pos) == 0x05D1, "decodeUtf8Next Bet");
    assertTrue(pos == 4, "decodeUtf8Next pos == 4");

    // P2-P3 paragraph first-strong skipping isolates
    const p_iso: [6]u32 = [CP_RLI, 0x05D0, 0x05D1, CP_PDI, 'a', 'b'];
    assertTrue(resolveParagraphFirstStrong(p_iso[0..]) == Direction.Ltr, "resolveParagraphFirstStrong skips RLI..PDI");
    const p_rtl: [5]u32 = ['1', '2', ' ', 0x0627, 'a'];
    assertTrue(resolveParagraphFirstStrong(p_rtl[0..]) == Direction.Rtl, "resolveParagraphFirstStrong skips weak digits");

    std.debug.print("PASS 1: parsers, BidiClass, UTF-8, and P2-P3 first-strong\n", .{});
}

fn testReorderVisualCore() {
    let out: [32]usize = [0; 32];

    // 1. Pure LTR
    const ltr: [3]u32 = ['a', 'b', 'c'];
    const e1: [3]usize = [0, 1, 2];
    const n1 = reorderVisual(ltr[0..], false, out[0..]);
    assertOrder(out[0..n1], e1[0..], "pure LTR");

    // 2. Pure RTL (Hebrew)
    const rtl: [3]u32 = [0x05D0, 0x05D1, 0x05D2];
    const e2: [3]usize = [2, 1, 0];
    const n2 = reorderVisual(rtl[0..], true, out[0..]);
    assertOrder(out[0..n2], e2[0..], "pure RTL");

    // 3. Mixed LTR base with RTL run: "a אב b"
    const mixed_ltr: [6]u32 = ['a', ' ', 0x05D0, 0x05D1, ' ', 'b'];
    const e3: [6]usize = [0, 1, 3, 2, 4, 5];
    const n3 = reorderVisual(mixed_ltr[0..], false, out[0..]);
    assertOrder(out[0..n3], e3[0..], "mixed LTR base with RTL");

    // 4. RTL base with embedded LTR run: "א ab ב"
    const mixed_rtl: [6]u32 = [0x05D0, ' ', 'a', 'b', ' ', 0x05D1];
    const e4: [6]usize = [5, 4, 2, 3, 1, 0];
    const n4 = reorderVisual(mixed_rtl[0..], true, out[0..]);
    assertOrder(out[0..n4], e4[0..], "RTL base with embedded LTR");

    // 5. RTL base with European numbers: "א 12 ב"
    const rtl_num: [6]u32 = [0x05D0, ' ', '1', '2', ' ', 0x05D1];
    const e5: [6]usize = [5, 4, 2, 3, 1, 0];
    const n5 = reorderVisual(rtl_num[0..], true, out[0..]);
    assertOrder(out[0..n5], e5[0..], "RTL base preserves number LTR order");

    // 6. RLO override in LTR base: RLO "abc" PDF -> reverses "abc"
    const rlo: [5]u32 = [CP_RLO, 'a', 'b', 'c', CP_PDF];
    const e6: [5]usize = [0, 3, 2, 1, 4];
    const n6 = reorderVisual(rlo[0..], false, out[0..]);
    assertOrder(out[0..n6], e6[0..], "RLO forced override");

    // 7. LRO override in RTL base: LRO "אב" PDF -> keeps logical order of "אב"
    const lro: [4]u32 = [CP_LRO, 0x05D0, 0x05D1, CP_PDF];
    const e7: [4]usize = [3, 1, 2, 0];
    const n7 = reorderVisual(lro[0..], true, out[0..]);
    assertOrder(out[0..n7], e7[0..], "LRO forced override in RTL base");

    // 8. Isolate boundary prevents RTL run from merging with adjacent RTL run before LTR:
    // "א" + LRI + "ב" + PDI + "a" -> LRI..PDI resolves to L (level 0), keeping "א" and "ב" separate.
    const iso: [5]u32 = [0x05D0, CP_LRI, 0x05D1, CP_PDI, 'a'];
    const e8: [5]usize = [0, 1, 2, 3, 4];
    const n8 = reorderVisual(iso[0..], false, out[0..]);
    assertOrder(out[0..n8], e8[0..], "LRI..PDI isolates RTL runs");

    // 9. FSI auto-direction: FSI + Hebrew + PDI inside LTR base
    const fsi: [8]u32 = ['a', ' ', CP_FSI, 0x05D0, 0x05D1, CP_PDI, ' ', 'b'];
    const e9: [8]usize = [0, 1, 2, 4, 3, 5, 6, 7];
    const n9 = reorderVisual(fsi[0..], false, out[0..]);
    assertOrder(out[0..n9], e9[0..], "FSI auto-RTL reverses inner Hebrew");

    // 10. LRE / RLE embedding
    const rle: [4]u32 = [CP_RLE, '1', '2', CP_PDF];
    const e10: [4]usize = [0, 1, 2, 3];
    const n10 = reorderVisual(rle[0..], false, out[0..]);
    assertOrder(out[0..n10], e10[0..], "RLE embedding keeps EN LTR");

    std.debug.print("PASS 2: UAX #9 reorderVisual (LTR, RTL, mixed, RLO/LRO, LRI/FSI, RLE)\n", .{});
}

fn testDomDirAutoAndBdiBdo(alloc: std.mem.Allocator)! {
    let doc = Document.init(alloc)?;
    defer doc.deinit();
    doc.parseHtml(
        "<html><body>" +
        "<div id=\"d1\" dir=\"auto\">Hello \u{05E9}\u{05DC}\u{05D5}\u{05DD}</div>" +
        "<div id=\"d2\" dir=\"auto\">123 \u{05E9}\u{05DC}\u{05D5}\u{05DD} Hello</div>" +
        "<div id=\"d3\" dir=\"auto\"><bdi>\u{05E9}\u{05DC}\u{05D5}\u{05DD}</bdi>Hello</div>" +
        "<div id=\"d4\" dir=\"auto\"><span dir=\"rtl\">\u{05E9}\u{05DC}\u{05D5}\u{05DD}</span>Hello</div>" +
        "<div id=\"d5\" dir=\"auto\"><script>\u{05E9}\u{05DC}\u{05D5}\u{05DD}</script><style>\u{05E9}\u{05DC}\u{05D5}\u{05DD}</style><textarea>\u{05E9}\u{05DC}\u{05D5}\u{05DD}</textarea>Hello</div>" +
        "<textarea id=\"ta\" dir=\"auto\">\u{05E9}\u{05DC}\u{05D5}\u{05DD}</textarea>" +
        "<bdi id=\"bdi1\">\u{05E9}\u{05DC}\u{05D5}\u{05DD}</bdi>" +
        "<bdi id=\"bdi2\" dir=\"ltr\">\u{05E9}\u{05DC}\u{05D5}\u{05DD}</bdi>" +
        "<bdo id=\"bdo1\" dir=\"rtl\">abc</bdo>" +
        "<bdo id=\"bdo2\" dir=\"ltr\">\u{05E9}\u{05DC}\u{05D5}\u{05DD}</bdo>" +
        "<span id=\"css_ov\">abc</span>" +
        "</body></html>"
    )?;

    const d1 = doc.getElementById("d1");
    const d2 = doc.getElementById("d2");
    const d3 = doc.getElementById("d3");
    const d4 = doc.getElementById("d4");
    const d5 = doc.getElementById("d5");
    const ta = doc.getElementById("ta");
    const bdi1 = doc.getElementById("bdi1");
    const bdi2 = doc.getElementById("bdi2");
    const bdo1 = doc.getElementById("bdo1");
    const bdo2 = doc.getElementById("bdo2");
    const css_ov = doc.getElementById("css_ov");

    // Direct resolveFirstStrongDirection checks
    assertTrue(resolveFirstStrongDirection(&doc, d1) == Direction.Ltr, "d1 first strong LTR");
    assertTrue(resolveFirstStrongDirection(&doc, d2) == Direction.Rtl, "d2 first strong RTL after digits");
    assertTrue(resolveFirstStrongDirection(&doc, d3) == Direction.Ltr, "d3 skips <bdi>");
    assertTrue(resolveFirstStrongDirection(&doc, d4) == Direction.Ltr, "d4 skips dir-attributed child");
    assertTrue(resolveFirstStrongDirection(&doc, d5) == Direction.Ltr, "d5 skips script/style/textarea");

    const css = "#css_ov { direction: rtl; unicode-bidi: bidi-override; }";
    const sheet = StyleSheet.parse(css, 800.0, false);
    const count = doc.nodeCount();
    let styles: []ComputedStyle = alloc.alloc(ComputedStyle, count)?;
    defer alloc.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    // dir="auto" cascade
    assertTrue(styles[d1].is_dir_auto && styles[d1].direction == Direction.Ltr && styles[d1].unicode_bidi == UnicodeBidi.Isolate, "d1 cascade");
    assertTrue(styles[d2].is_dir_auto && styles[d2].direction == Direction.Rtl && styles[d2].unicode_bidi == UnicodeBidi.Isolate, "d2 cascade");
    assertTrue(styles[d3].is_dir_auto && styles[d3].direction == Direction.Ltr, "d3 cascade");
    assertTrue(styles[d4].is_dir_auto && styles[d4].direction == Direction.Ltr, "d4 cascade");
    assertTrue(styles[ta].is_dir_auto && styles[ta].direction == Direction.Rtl && styles[ta].unicode_bidi == UnicodeBidi.Plaintext, "textarea dir=auto uses plaintext");

    // <bdi> isolation
    assertTrue(styles[bdi1].is_dir_auto && styles[bdi1].direction == Direction.Rtl && styles[bdi1].unicode_bidi == UnicodeBidi.Isolate, "<bdi> default auto+isolate");
    assertTrue(!styles[bdi2].is_dir_auto && styles[bdi2].direction == Direction.Ltr && styles[bdi2].unicode_bidi == UnicodeBidi.Isolate, "<bdi dir=ltr> isolate");

    // <bdo> forced override (isolate-override)
    assertTrue(!styles[bdo1].is_dir_auto && styles[bdo1].direction == Direction.Rtl && styles[bdo1].unicode_bidi == UnicodeBidi.IsolateOverride, "<bdo dir=rtl> isolate-override");
    assertTrue(!styles[bdo2].is_dir_auto && styles[bdo2].direction == Direction.Ltr && styles[bdo2].unicode_bidi == UnicodeBidi.IsolateOverride, "<bdo dir=ltr> isolate-override");

    // CSS direction + unicode-bidi
    assertTrue(!styles[css_ov].is_dir_auto && styles[css_ov].direction == Direction.Rtl && styles[css_ov].unicode_bidi == UnicodeBidi.BidiOverride, "CSS direction + unicode-bidi");

    std.debug.print("PASS 3: dir=\"auto\", <bdi> isolation, <bdo> override, and CSS unicode-bidi\n", .{});
}

pub fn main()! {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    testParsersAndClasses();
    testReorderVisualCore();
    testDomDirAutoAndBdiBdo(alloc)?;
}
