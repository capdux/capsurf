// test/html/test_svg_and_template.cp
// CapSurf Pure CP White-Box Unit Tests:
// 1. HTML5 <template> content parsing & isolation from connected DOM queries
// 2. SVG element & self-closing children (<path/>, <rect/>, <circle/>) tree structure
// 3. Unclosed & misnested HTML5 tag recovery (implicit <p>/<li> closure & AAA)

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { Matcher } from "../../../src/css/matcher.cp";

fn main() {
    test("1. HTML5 <template> Content Parsing & Document Isolation", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html><html><body>" ++
            "<template id=\"tpl\">" ++
            "<div id=\"tpl-inner\" class=\"secret\"><span class=\"txt\">Inside</span></div>" ++
            "</template>" ++
            "<div id=\"outside\" class=\"secret\">Outside</div>" ++
            "</body></html>";

        doc.parseHtml(html) catch unreachable;

        // <template> itself is connected to the document
        const tpl = doc.getElementById("tpl");
        expect_eq(tpl != NULL_NODE, true);
        expect_eq(std.mem.eql(u8, doc.getTagName(tpl), "template"), true);
        expect_eq(doc.isConnectedNode(tpl), true);

        // Elements inside <template> are parsed into its subtree, but isolated from document queries
        expect_eq(doc.getElementById("tpl-inner"), NULL_NODE);
        expect_eq(Matcher.querySelector(&doc, "template > div"), NULL_NODE);

        let matches: [8]NodeId = [NULL_NODE; 8];
        const secret_count = Matcher.querySelectorAll(&doc, ".secret", &matches);
        expect_eq(secret_count, 1);
        expect_eq(matches[0], doc.getElementById("outside"));

        // Direct tree traversal from <template> still observes the parsed child nodes
        const tpl_child = doc.getFirstChild(tpl);
        expect_eq(tpl_child != NULL_NODE, true);
        expect_eq(doc.isConnectedNode(tpl_child), false);
        expect_eq(std.mem.eql(u8, doc.getTagName(tpl_child), "div"), true);
        expect_eq(doc.nodes.items[@as(usize, tpl_child)].hasId("tpl-inner"), true);

        const span_node = doc.getFirstChild(tpl_child);
        expect_eq(span_node != NULL_NODE, true);
        expect_eq(std.mem.eql(u8, doc.getTagName(span_node), "span"), true);
        const text_node = doc.getFirstChild(span_node);
        expect_eq(text_node != NULL_NODE, true);
        expect_eq(std.mem.eql(u8, doc.getTextContent(text_node), "Inside"), true);
    });

    test("2. SVG Container & Self-Closing Children (<path/>, <rect/>, <circle/>)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html><html><body>" ++
            "<div id=\"wrap\">" ++
            "<svg id=\"icon\" viewBox=\"0 0 24 24\">" ++
            "<path id=\"p1\" d=\"M0 0\" fill=\"red\"/>" ++
            "<rect id=\"r1\" x=\"1\" y=\"2\" width=\"10\" height=\"10\"/>" ++
            "<circle id=\"c1\" cx=\"5\" cy=\"5\" r=\"3\"/>" ++
            "</svg>" ++
            "<p id=\"after\">Done</p>" ++
            "</div>" ++
            "</body></html>";

        doc.parseHtml(html) catch unreachable;

        const wrap = doc.getElementById("wrap");
        const svg = Matcher.querySelector(&doc, "div#wrap > svg#icon");
        expect_eq(wrap != NULL_NODE, true);
        expect_eq(svg != NULL_NODE, true);
        expect_eq(doc.getParent(svg), wrap);

        // Self-closing SVG children must be siblings under <svg>, not nested inside each other
        const path_node = doc.getElementById("p1");
        const rect_node = doc.getElementById("r1");
        const circle_node = doc.getElementById("c1");
        expect_eq(path_node != NULL_NODE, true);
        expect_eq(rect_node != NULL_NODE, true);
        expect_eq(circle_node != NULL_NODE, true);

        expect_eq(doc.getParent(path_node), svg);
        expect_eq(doc.getParent(rect_node), svg);
        expect_eq(doc.getParent(circle_node), svg);

        expect_eq(doc.getFirstChild(path_node), NULL_NODE);
        expect_eq(doc.getFirstChild(rect_node), NULL_NODE);
        expect_eq(doc.getFirstChild(circle_node), NULL_NODE);

        expect_eq(doc.nodes.items[@as(usize, path_node)].next_sibling, rect_node);
        expect_eq(doc.nodes.items[@as(usize, rect_node)].next_sibling, circle_node);
        expect_eq(doc.nodes.items[@as(usize, circle_node)].next_sibling, NULL_NODE);

        // Following HTML sibling <p id="after"> remains outside <svg> under #wrap
        const after = doc.getElementById("after");
        expect_eq(after != NULL_NODE, true);
        expect_eq(doc.getParent(after), wrap);
        expect_eq(doc.nodes.items[@as(usize, svg)].next_sibling, after);
    });

    test("3. Unclosed & Misnested Tag Recovery (Implicit Closure & AAA)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html><html><body>" ++
            "<div id=\"blocks\"><p id=\"p1\">First<p id=\"p2\">Second<div id=\"inner\">Block</div></div>" ++
            "<ul id=\"list\"><li id=\"l1\">A<li id=\"l2\">B</ul>" ++
            "<div id=\"fmt\"><b><i>Text</b><span>After</span></div>" ++
            "</body></html>";

        doc.parseHtml(html) catch unreachable;

        // Implicit <p> closure by subsequent <p> and <div>
        const blocks = doc.getElementById("blocks");
        const p1 = doc.getElementById("p1");
        const p2 = doc.getElementById("p2");
        const inner = doc.getElementById("inner");
        expect_eq(blocks != NULL_NODE, true);
        expect_eq(p1 != NULL_NODE, true);
        expect_eq(p2 != NULL_NODE, true);
        expect_eq(inner != NULL_NODE, true);
        expect_eq(doc.getParent(p1), blocks);
        expect_eq(doc.getParent(p2), blocks);
        expect_eq(doc.getParent(inner), blocks);
        expect_eq(doc.nodes.items[@as(usize, p1)].next_sibling, p2);
        expect_eq(doc.nodes.items[@as(usize, p2)].next_sibling, inner);

        // Implicit <li> closure inside <ul>
        const list = doc.getElementById("list");
        const l1 = doc.getElementById("l1");
        const l2 = doc.getElementById("l2");
        expect_eq(doc.getParent(l1), list);
        expect_eq(doc.getParent(l2), list);
        expect_eq(doc.nodes.items[@as(usize, l1)].next_sibling, l2);

        // Misnested <b><i>Text</b><span>After</span>: <b> closes, <i> reopens under #fmt, so <span> is inside <i>
        const fmt = doc.getElementById("fmt");
        const span_after = Matcher.querySelector(&doc, "#fmt > i > span");
        expect_eq(span_after != NULL_NODE, true);
        expect_eq(doc.getParent(doc.getParent(span_after)), fmt);
    });
}
