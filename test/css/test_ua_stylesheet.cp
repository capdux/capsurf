// test/css/test_ua_stylesheet.cp
// CapSurf W3C UA Stylesheet Verification Suite

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { Matcher } from "../../src/css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";

fn main() {
    test("1. W3C HTML5 UA Stylesheet: Default display rules and [hidden] attribute", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><head><title>UA Test</title><style></style><script></script></head><body><h1>Title</h1><p id=\"para\">Hello <span id=\"text\">World</span></p><button id=\"btn\">Click</button><div id=\"hidden-box\" hidden>Should be hidden</div></body></html>";
        doc.parseHtml(html) catch unreachable;

        let styles: [64]ComputedStyle = [ComputedStyle{}; 64];
        const empty_sheet = StyleSheet.parse("", 1280.0, false);
        Cascade.computeStyles(&doc, &empty_sheet, styles[0..doc.nodeCount()], 16.0);

        // 1. Head, style, script must be display: none
        const head_id = Matcher.querySelector(&doc, "head");
        expect_eq(head_id != NULL_NODE, true);
        expect_eq(styles[head_id].display.isNone(), true);

        const style_id = Matcher.querySelector(&doc, "style");
        expect_eq(style_id != NULL_NODE, true);
        expect_eq(styles[style_id].display.isNone(), true);

        const script_id = Matcher.querySelector(&doc, "script");
        expect_eq(script_id != NULL_NODE, true);
        expect_eq(styles[script_id].display.isNone(), true);

        // 2. [hidden] element must be display: none
        const hidden_id = Matcher.querySelector(&doc, "#hidden-box");
        expect_eq(hidden_id != NULL_NODE, true);
        expect_eq(styles[hidden_id].display.isNone(), true);

        // 3. Block and inline tags
        const h1_id = Matcher.querySelector(&doc, "h1");
        expect_eq(h1_id != NULL_NODE, true);
        expect_eq(styles[h1_id].display.isNone(), false);
        expect_eq(styles[h1_id].display.isInline(), false);
        expect_eq(styles[h1_id].font_size_px, 32.0);

        const span_id = Matcher.querySelector(&doc, "#text");
        expect_eq(span_id != NULL_NODE, true);
        expect_eq(styles[span_id].display.isInline(), true);

        const btn_id = Matcher.querySelector(&doc, "#btn");
        expect_eq(btn_id != NULL_NODE, true);
        expect_eq(styles[btn_id].display.isInline(), false);
        expect_eq(styles[btn_id].display.isNone(), false);
    });
}
