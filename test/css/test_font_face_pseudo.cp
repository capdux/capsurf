// test/css/test_font_face_pseudo.cp
// Native CP unit tests for CSS Fonts 4 (@font-face, CORS gating, unicode-range)
// and CSS Pseudo-Elements (::before / ::after content intrinsic & inline layout).

import { Document } from "../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";
import { LayoutTree } from "../../src/layout/layout.cp";

fn assertNear(actual: f32, expected: f32, eps: f32) {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    if (diff > eps) {
        std.debug.print("ASSERT FAILED: actual={d:.3}, expected={d:.3}\n", .{ actual, expected });
        std.process.exit(1);
    }
}

fn assertTrue(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("ASSERT FAILED: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn testWebfontCors(alloc: std.mem.Allocator)! {
    const css =
        "@font-face { font-family: KsTestMono; src: url(/__fixtures/ks-test-mono.woff2); }\n" +
        "@font-face { font-family: KsTestMonoData; src: url(data:font/woff2;base64,d09GMgABAAAA); }\n" +
        "@font-face { font-family: KsTestMonoPct; src: url(__KS_PCT_URI__); }\n" +
        "@font-face { font-family: KsCorsFont; src: url(https://ks-fonts.invalid/__fixtures/cors-font.woff2?acao=1); }\n" +
        "@font-face { font-family: KsNoCorsFont; src: url(https://ks-fonts.invalid/__fixtures/cors-font.woff2); }\n" +
        "body { margin: 0; font-size: 24px; }\n" +
        "span { display: inline-block; }\n" +
        "#wf { font-family: KsTestMono; }\n" +
        "#wfdata { font-family: KsTestMonoData; }\n" +
        "#wfpct { font-family: KsTestMonoPct; }\n" +
        "#cors { font-family: KsCorsFont; }\n" +
        "#nocors { font-family: KsNoCorsFont; }\n";

    let doc = Document.init(alloc)?;
    defer doc.deinit();
    doc.parseHtml(
        "<html><body>" +
        "<div><span id=\"fallback\">WebFont123.test</span></div>" +
        "<div><span id=\"wf\">WebFont123.test</span></div>" +
        "<div><span id=\"wfdata\">WebFont123.test</span></div>" +
        "<div><span id=\"wfpct\">WebFont123.test</span></div>" +
        "<div><span id=\"cors\">WebFont123.test</span></div>" +
        "<div><span id=\"nocors\">WebFont123.test</span></div>" +
        "</body></html>"
    )?;

    const sheet = StyleSheet.parse(css, 600.0, false);
    assertTrue(sheet.font_face_count == 5, "expected 5 @font-face rules");

    const count = doc.nodeCount();
    let styles: []ComputedStyle = alloc.alloc(ComputedStyle, count)?;
    defer alloc.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let layout = LayoutTree.init(alloc, 600.0, 300.0)?;
    defer layout.deinit();
    layout.computeLayout(&doc, styles)?;

    const fb_w = layout.getBoundingClientRect(doc.getElementById("fallback")).width;
    const wf_w = layout.getBoundingClientRect(doc.getElementById("wf")).width;
    const wfdata_w = layout.getBoundingClientRect(doc.getElementById("wfdata")).width;
    const wfpct_w = layout.getBoundingClientRect(doc.getElementById("wfpct")).width;
    const cors_w = layout.getBoundingClientRect(doc.getElementById("cors")).width;
    const nocors_w = layout.getBoundingClientRect(doc.getElementById("nocors")).width;

    assertTrue(wf_w > fb_w * 1.1, "wf width must exceed fallback * 1.1");
    assertNear(wfdata_w, wf_w, 0.01);
    assertNear(wfpct_w, wf_w, 0.01);
    assertTrue(cors_w > fb_w * 1.1, "cors width must exceed fallback * 1.1");
    assertNear(nocors_w, fb_w, 0.01);
    std.debug.print("PASS 1: WebFont & CORS shaping (fb={d:.2}, wf={d:.2}, cors={d:.2}, nocors={d:.2})\n", .{ fb_w, wf_w, cors_w, nocors_w });
}

fn testUnicodeRange(alloc: std.mem.Allocator)! {
    const css =
        "@font-face { font-family: \"RangeFont\"; src: url(\"/__fixtures/noto-cyrillic-subset.woff2\"); unicode-range: U+0400-04FF; }\n" +
        "@font-face { font-family: \"RangeFont\"; src: url(\"/__fixtures/noto-greek-subset.woff2\"); unicode-range: U+0370-03FF; }\n" +
        "@font-face { font-family: \"RangeFont\"; src: url(\"/__fixtures/bungee.woff2\"); unicode-range: U+0000-00FF; }\n" +
        "p { font-size: 32px; display: inline-block; }\n" +
        "#target { font-family: \"RangeFont\", serif; }\n" +
        "#control { font-family: serif; }\n";

    let doc = Document.init(alloc)?;
    defer doc.deinit();
    doc.parseHtml("<html><body><p id=\"target\">Hamburgefonstiv</p><p id=\"control\">Hamburgefonstiv</p></body></html>")?;

    const sheet = StyleSheet.parse(css, 800.0, false);
    assertTrue(sheet.font_face_count == 3, "expected 3 unicode-range @font-face rules");

    const count = doc.nodeCount();
    let styles: []ComputedStyle = alloc.alloc(ComputedStyle, count)?;
    defer alloc.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let layout = LayoutTree.init(alloc, 800.0, 600.0)?;
    defer layout.deinit();
    layout.computeLayout(&doc, styles)?;

    const target_w = layout.getBoundingClientRect(doc.getElementById("target")).width;
    const control_w = layout.getBoundingClientRect(doc.getElementById("control")).width;
    assertTrue((target_w - control_w) >= 30.0, "RangeFont Latin bungee subset must widen target by >= 30px");
    std.debug.print("PASS 2: unicode-range subset shaping (target={d:.2}, control={d:.2}, delta={d:.2})\n", .{ target_w, control_w, target_w - control_w });
}

fn testPseudoBefore(alloc: std.mem.Allocator)! {
    const css =
        "@font-face { font-family: KsDeferMono; src: url(/__fixtures/ks-test-mono.woff2); }\n" +
        ".ic { font-family: KsDeferMono; font-size: 24px; }\n" +
        ".ic::before { content: \"WiWi\"; }\n";

    let doc = Document.init(alloc)?;
    defer doc.deinit();
    doc.parseHtml(
        "<html><body>" +
        "<i id=\"block\" class=\"ic\" style=\"display:block;width:max-content\"></i>" +
        "<i id=\"inline-block\" class=\"ic\" style=\"display:inline-block\"></i>" +
        "<div style=\"display:flex\"><i id=\"flex-item\" class=\"ic\"></i></div>" +
        "<button><i id=\"button\" class=\"ic\"></i></button>" +
        "<span id=\"control\" style=\"display:inline-block;font:24px KsDeferMono\">WiWi</span>" +
        "</body></html>"
    )?;

    const sheet = StyleSheet.parse(css, 800.0, false);
    const count = doc.nodeCount();
    let styles: []ComputedStyle = alloc.alloc(ComputedStyle, count)?;
    defer alloc.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let layout = LayoutTree.init(alloc, 800.0, 600.0)?;
    defer layout.deinit();
    layout.computeLayout(&doc, styles)?;

    const ctrl_w = layout.getBoundingClientRect(doc.getElementById("control")).width;
    const blk_w = layout.getBoundingClientRect(doc.getElementById("block")).width;
    const ib_w = layout.getBoundingClientRect(doc.getElementById("inline-block")).width;
    const flex_w = layout.getBoundingClientRect(doc.getElementById("flex-item")).width;
    const btn_w = layout.getBoundingClientRect(doc.getElementById("button")).width;

    assertTrue(ctrl_w > 50.0, "control width must be > 50px");
    assertNear(blk_w, ctrl_w, 0.5);
    assertNear(ib_w, ctrl_w, 0.5);
    assertNear(flex_w, ctrl_w, 0.5);
    assertNear(btn_w, ctrl_w, 0.5);
    std.debug.print("PASS 3: ::before pseudo-element layout (ctrl={d:.2}, block={d:.2}, ib={d:.2}, flex={d:.2}, btn={d:.2})\n", .{ ctrl_w, blk_w, ib_w, flex_w, btn_w });
}

pub fn main()! {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    testWebfontCors(alloc)?;
    testUnicodeRange(alloc)?;
    testPseudoBefore(alloc)?;
}
