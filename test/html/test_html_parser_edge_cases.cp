// test/html/test_html_parser_edge_cases.cp
// Unit tests for HTML5 parsing edge cases and tag_spans.cp scanning.

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { Matcher } from "../../src/css/matcher.cp";
import { TagSpan, closingTagEnd, htmlTagSpans, removeHtmlTagSpans } from "../../src/html/tag_spans.cp";

fn main() {
    test("1. Unclosed tags & void elements with nested block recovery", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<html><head>" ++
            "<meta charset=\"utf-8\">" ++
            "<meta name=\"viewport\" content=\"width=device-width\">" ++
            "</head><body>" ++
            "<div id=\"box\">" ++
            "<p id=\"p1\">line1<br>line2<hr><img id=\"pic\" src=\"a.png\"><input id=\"inp\" type=\"text\">" ++
            "<p id=\"p2\">second" ++
            "<div id=\"inner\"><span id=\"nested-span\">text</div>" ++
            "</div>" ++
            "<div id=\"after\">tail</div>" ++
            "</body></html>";

        doc.parseHtml(html) catch unreachable;

        // Void <meta> elements in <head> must be siblings, not nested
        let metas: [4]NodeId = [NULL_NODE; 4];
        const meta_count = Matcher.querySelectorAll(&doc, "head > meta", &metas);
        expect_eq(meta_count, 2);
        expect_eq(doc.getFirstChild(metas[0]), NULL_NODE);
        expect_eq(doc.getFirstChild(metas[1]), NULL_NODE);

        const charset_opt = doc.getAttribute(metas[0], "charset");
        expect_eq(charset_opt != null, true);
        if (charset := charset_opt) {
            expect_eq(std.mem.eql(u8, charset, "utf-8"), true);
        }

        // Void elements inside body have no children
        const pic = doc.getElementById("pic");
        const inp = doc.getElementById("inp");
        expect_eq(pic != NULL_NODE, true);
        expect_eq(inp != NULL_NODE, true);
        expect_eq(doc.getFirstChild(pic), NULL_NODE);
        expect_eq(doc.getFirstChild(inp), NULL_NODE);

        // Unclosed <p id="p1"> is implicitly closed by <p id="p2">
        const box = doc.getElementById("box");
        const p1 = doc.getElementById("p1");
        const p2 = doc.getElementById("p2");
        expect_eq(p1 != NULL_NODE, true);
        expect_eq(p2 != NULL_NODE, true);
        expect_eq(doc.getParent(p1), box);
        expect_eq(doc.getParent(p2), box);

        // Unclosed <span> inside #inner is recovered when </div> closes #inner
        const inner = doc.getElementById("inner");
        const nested_span = doc.getElementById("nested-span");
        const after = doc.getElementById("after");
        expect_eq(doc.getParent(nested_span), inner);
        expect_eq(doc.getParent(after), doc.getParent(box));
    });

    test("2. Implicit html/head/body and table/tbody/tr/td structure creation", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<title>Implicit</title>" ++
            "<div id=\"wrap\">" ++
            "<table>" ++
            "<tr><td id=\"c1\">A<td id=\"c2\">B" ++
            "<tr><td id=\"c3\">C" ++
            "</table>" ++
            "</div>";

        doc.parseHtml(html) catch unreachable;

        // Implicit <html>, <head>, and <body>
        expect_eq(doc.root_element != NULL_NODE, true);
        expect_eq(std.ascii.eqlIgnoreCase(doc.getTagName(doc.root_element), "html"), true);

        const title_el = Matcher.querySelector(&doc, "html > head > title");
        expect_eq(title_el != NULL_NODE, true);

        const wrap_el = Matcher.querySelector(&doc, "html > body > div#wrap");
        expect_eq(wrap_el != NULL_NODE, true);

        // Implicit <tbody> and auto-closed <td> / <tr>
        const tbody_el = Matcher.querySelector(&doc, "table > tbody");
        expect_eq(tbody_el != NULL_NODE, true);

        let trs: [4]NodeId = [NULL_NODE; 4];
        const tr_count = Matcher.querySelectorAll(&doc, "table > tbody > tr", &trs);
        expect_eq(tr_count, 2);

        let tds: [8]NodeId = [NULL_NODE; 8];
        const td_count = Matcher.querySelectorAll(&doc, "table > tbody > tr > td", &tds);
        expect_eq(td_count, 3);

        const c1 = doc.getElementById("c1");
        const c2 = doc.getElementById("c2");
        const c3 = doc.getElementById("c3");
        expect_eq(doc.getParent(c1), trs[0]);
        expect_eq(doc.getParent(c2), trs[0]);
        expect_eq(doc.getParent(c3), trs[1]);
    });

    test("3. Raw-text script/style spans with < and </div> lookalikes plus comments/DOCTYPE", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html>" ++
            "<!-- comment with <span id=\"ghost\"></span> and </script> -->" ++
            "<html><head>" ++
            "<style id=\"st\">div > p { content: \"<div>\"; } .a < .b {}</style>" ++
            "</head><body>" ++
            "<script id=\"sc\">if (a < b && c > d) { const s = \"</div><span id='fake'>x</span>\"; }</script>" ++
            "<div id=\"real\">ok</div>" ++
            "</body></html>";

        doc.parseHtml(html) catch unreachable;

        const st = doc.getElementById("st");
        const sc = doc.getElementById("sc");
        const real = doc.getElementById("real");
        expect_eq(st != NULL_NODE, true);
        expect_eq(sc != NULL_NODE, true);
        expect_eq(real != NULL_NODE, true);

        // Comment and script/style lookalike tags must not become DOM elements
        expect_eq(doc.getElementById("ghost"), NULL_NODE);
        expect_eq(doc.getElementById("fake"), NULL_NODE);
        expect_eq(Matcher.querySelector(&doc, "#sc span"), NULL_NODE);

        const sc_text = doc.getFirstChild(sc);
        expect_eq(sc_text != NULL_NODE, true);
        expect_eq(std.mem.indexOf(u8, doc.getTextContent(sc_text), "</div><span id='fake'>x</span>") != null, true);

        const real_text = doc.getFirstChild(real);
        expect_eq(real_text != NULL_NODE, true);
        expect_eq(std.mem.eql(u8, doc.getTextContent(real_text), "ok"), true);
    });

    test("4. tag_spans.cp scanning for script/style/link/meta and attributes", () => {
        const html =
            "<meta charset=\"utf-8\">" ++
            "<metadata ignored=\"1\"></metadata>" ++
            "<link rel=\"stylesheet\" href=\"app.css\">" ++
            "<style>.a{color:red}</style>" ++
            "<script src=\"a.js\"></script>" ++
            "<scripting>ignored</scripting>" ++
            "<script>const x = \"<div>\";</script >" ++
            "<div id=\"keep\">hello</div>";

        // Paired <script> spans (ignores <scripting>, handles whitespace in </script >)
        let script_spans: [4]TagSpan = [TagSpan{}; 4];
        const script_count = htmlTagSpans(html, "script", true, &script_spans);
        expect_eq(script_count, 2);
        expect_eq(std.mem.eql(u8, html[script_spans[0].start .. script_spans[0].end], "<script src=\"a.js\"></script>"), true);
        expect_eq(std.mem.eql(u8, html[script_spans[1].start .. script_spans[1].end], "<script>const x = \"<div>\";</script >"), true);

        // Paired <style> span
        let style_spans: [4]TagSpan = [TagSpan{}; 4];
        const style_count = htmlTagSpans(html, "style", true, &style_spans);
        expect_eq(style_count, 1);
        expect_eq(std.mem.eql(u8, html[style_spans[0].start .. style_spans[0].end], "<style>.a{color:red}</style>"), true);

        // Unpaired <link> and <meta> spans with attribute matching (ignores <metadata>)
        let link_spans: [4]TagSpan = [TagSpan{}; 4];
        const link_count = htmlTagSpans(html, "link", false, &link_spans);
        expect_eq(link_count, 1);
        const link_slice = html[link_spans[0].start .. link_spans[0].end];
        expect_eq(std.mem.indexOf(u8, link_slice, "rel=\"stylesheet\"") != null, true);
        expect_eq(std.mem.indexOf(u8, link_slice, "href=\"app.css\"") != null, true);

        let meta_spans: [4]TagSpan = [TagSpan{}; 4];
        const meta_count = htmlTagSpans(html, "meta", false, &meta_spans);
        expect_eq(meta_count, 1);
        const meta_slice = html[meta_spans[0].start .. meta_spans[0].end];
        expect_eq(std.mem.indexOf(u8, meta_slice, "charset=\"utf-8\"") != null, true);

        // closingTagEnd negative and positive checks
        expect_eq(closingTagEnd("<script>no close", "script", 8), -1);
        expect_eq(closingTagEnd("<script>ok</script  >", "script", 8), 21);

        // removeHtmlTagSpans strips script spans cleanly while keeping <scripting> and <div>
        let out_buf: [512]u8 = [0; 512];
        const stripped = removeHtmlTagSpans(html, script_spans[0..script_count], &out_buf);
        expect_eq(std.mem.indexOf(u8, stripped, "<script src=") == null, true);
        expect_eq(std.mem.indexOf(u8, stripped, "<script>const") == null, true);
        expect_eq(std.mem.indexOf(u8, stripped, "<scripting>ignored</scripting>") != null, true);
        expect_eq(std.mem.indexOf(u8, stripped, "<div id=\"keep\">hello</div>") != null, true);
    });
}
