// test/html/test_after_head_and_foreign.cp
// CapSurf Pure CP White-Box Unit Tests:
// 1. AfterHead mode: <script>/<style>/<meta>/<link>/<title> between </head> and <body style="...">
// 2. AfterHead edge cases: RawText/RcData/comments/whitespace before <body>
// 3. SVG foreign elements, self-closing children, and HTML breakout tag (<p>/<div>) recovery
// 4. MathML foreign elements (<math>/<mrow>/<mspace/>) and HTML breakout tag recovery

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { HtmlParser } from "../../src/html/parser.cp";
import { Matcher } from "../../src/css/matcher.cp";

fn countDirectChildren(doc: *const Document, parent: NodeId): usize {
    let count: usize = 0;
    let cur = doc.getFirstChild(parent);
    while (cur != NULL_NODE) {
        count += 1;
        cur = doc.nodes.items[@as(usize, cur)].next_sibling;
    }
    return count;
}

fn main() {
    test("1. AfterHead: <script>/<style>/<meta>/<link>/<title> before <body style='margin:0;padding:0'>", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\"></head>" ++
            "<script id=\"ah-script\">var x = 1;</script>" ++
            "<style id=\"ah-style\">.a { color: red; }</style>" ++
            "<meta id=\"ah-meta\" name=\"viewport\" content=\"width=device-width\">" ++
            "<link id=\"ah-link\" rel=\"stylesheet\" href=\"app.css\">" ++
            "<title id=\"ah-title\">AfterHead Title</title>" ++
            "<body id=\"main-body\" style=\"margin:0;padding:0\">" ++
            "<div id=\"app\">Hello</div>" ++
            "</body></html>";

        HtmlParser.parse(&doc, html) catch unreachable;

        // Only one <body> element may exist, and its attributes must be preserved
        let bodies: [8]NodeId = [NULL_NODE; 8];
        const body_count = Matcher.querySelectorAll(&doc, "body", &bodies);
        expect_eq(body_count, 1);

        const body_node = bodies[0];
        expect_eq(body_node != NULL_NODE, true);
        expect_eq(body_node, doc.getElementById("main-body"));

        if (style_attr := doc.getAttribute(body_node, "style")) {
            expect_eq(std.mem.eql(u8, style_attr, "margin:0;padding:0"), true);
        } else {
            expect_eq(false, true);
        }

        // Body child content is intact
        const app_node = doc.getElementById("app");
        expect_eq(app_node != NULL_NODE, true);
        expect_eq(doc.getParent(app_node), body_node);
        expect_eq(std.mem.eql(u8, doc.getTextContent(doc.getFirstChild(app_node)), "Hello"), true);

        // Elements inserted during AfterHead are parsed properly
        const script_node = doc.getElementById("ah-script");
        const style_node = doc.getElementById("ah-style");
        const meta_node = doc.getElementById("ah-meta");
        const link_node = doc.getElementById("ah-link");
        const title_node = doc.getElementById("ah-title");

        expect_eq(script_node != NULL_NODE, true);
        expect_eq(style_node != NULL_NODE, true);
        expect_eq(meta_node != NULL_NODE, true);
        expect_eq(link_node != NULL_NODE, true);
        expect_eq(title_node != NULL_NODE, true);

        expect_eq(doc.getFirstChild(meta_node), NULL_NODE);
        expect_eq(doc.getFirstChild(link_node), NULL_NODE);
        expect_eq(std.mem.eql(u8, doc.getTextContent(doc.getFirstChild(title_node)), "AfterHead Title"), true);
    });

    test("2. AfterHead: comments, whitespace, RawText (<) & RcData (&amp;) before <body>", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<html><head></head>" ++
            "  \n\t  <!-- after-head comment --> " ++
            "<title id=\"t1\">A &amp; B</title>" ++
            "<script id=\"s1\">if (a < b) { c = 1; }</script>" ++
            "<style id=\"st1\">p > span { margin: 0; }</style>" ++
            "<body style=\"margin:0;padding:0\" data-ok=\"1\">" ++
            "<p id=\"p1\">BodyText</p>" ++
            "</body></html>";

        doc.parseHtml(html) catch unreachable;

        let bodies: [4]NodeId = [NULL_NODE; 4];
        expect_eq(Matcher.querySelectorAll(&doc, "body", &bodies), 1);
        const body_node = bodies[0];

        if (style_attr := doc.getAttribute(body_node, "style")) {
            expect_eq(std.mem.eql(u8, style_attr, "margin:0;padding:0"), true);
        } else {
            expect_eq(false, true);
        }
        if (ok_attr := doc.getAttribute(body_node, "data-ok")) {
            expect_eq(std.mem.eql(u8, ok_attr, "1"), true);
        } else {
            expect_eq(false, true);
        }

        const t1 = doc.getElementById("t1");
        const s1 = doc.getElementById("s1");
        const st1 = doc.getElementById("st1");
        expect_eq(std.mem.eql(u8, doc.getTextContent(doc.getFirstChild(t1)), "A & B"), true);
        expect_eq(std.mem.eql(u8, doc.getTextContent(doc.getFirstChild(s1)), "if (a < b) { c = 1; }"), true);
        expect_eq(std.mem.eql(u8, doc.getTextContent(doc.getFirstChild(st1)), "p > span { margin: 0; }"), true);

        const p1 = doc.getElementById("p1");
        expect_eq(doc.getParent(p1), body_node);
    });

    test("3. Foreign Content (SVG): self-closing children and HTML breakout tags (<p>, <div>)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html><html><body>" ++
            "<div id=\"host\">" ++
            "<svg id=\"svg1\" width=\"100\" height=\"100\">" ++
            "<g id=\"g1\">" ++
            "<rect id=\"r1\" x=\"0\" y=\"0\" width=\"10\" height=\"10\"/>" ++
            "<circle id=\"c1\" cx=\"5\" cy=\"5\" r=\"2\"/>" ++
            "<path id=\"p1\" d=\"M0 0\"/>" ++
            "</g>" ++
            "<p id=\"break-p\">BreakP</p>" ++
            "<div id=\"break-div\">BreakDiv</div>" ++
            "</svg>" ++
            "<span id=\"after-svg\">After</span>" ++
            "</div>" ++
            "</body></html>";

        HtmlParser.parse(&doc, html) catch unreachable;

        const host = doc.getElementById("host");
        const svg1 = doc.getElementById("svg1");
        const g1 = doc.getElementById("g1");
        const r1 = doc.getElementById("r1");
        const c1 = doc.getElementById("c1");
        const p1 = doc.getElementById("p1");
        const after_svg = doc.getElementById("after-svg");

        expect_eq(host != NULL_NODE, true);
        expect_eq(svg1 != NULL_NODE, true);
        expect_eq(doc.getParent(svg1), host);
        expect_eq(doc.getParent(g1), svg1);

        // Self-closing SVG elements do not swallow siblings
        expect_eq(doc.getParent(r1), g1);
        expect_eq(doc.getParent(c1), g1);
        expect_eq(doc.getParent(p1), g1);
        expect_eq(countDirectChildren(&doc, g1), 3);

        // HTML breakout tags exist and subsequent sibling after </svg> is attached to #host
        expect_eq(doc.getElementById("break-p") != NULL_NODE, true);
        expect_eq(doc.getElementById("break-div") != NULL_NODE, true);
        expect_eq(after_svg != NULL_NODE, true);
        expect_eq(doc.getParent(after_svg), host);
    });

    test("4. Foreign Content (MathML): <math>/<mrow>/<mspace/> and HTML breakout recovery", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html><html><body>" ++
            "<div id=\"math-host\">" ++
            "<math id=\"m1\">" ++
            "<mrow id=\"row1\">" ++
            "<mi id=\"mi1\">x</mi>" ++
            "<mspace id=\"sp1\" width=\"1em\"/>" ++
            "<mo id=\"mo1\">+</mo>" ++
            "<mi id=\"mi2\">y</mi>" ++
            "</mrow>" ++
            "<div id=\"math-break\">Fallback</div>" ++
            "</math>" ++
            "<p id=\"after-math\">Done</p>" ++
            "</div>" ++
            "</body></html>";

        HtmlParser.parse(&doc, html) catch unreachable;

        const math_host = doc.getElementById("math-host");
        const m1 = doc.getElementById("m1");
        const row1 = doc.getElementById("row1");
        const mi1 = doc.getElementById("mi1");
        const sp1 = doc.getElementById("sp1");
        const mo1 = doc.getElementById("mo1");
        const mi2 = doc.getElementById("mi2");
        const after_math = doc.getElementById("after-math");

        expect_eq(doc.getParent(m1), math_host);
        expect_eq(doc.getParent(row1), m1);
        expect_eq(doc.getParent(mi1), row1);
        expect_eq(doc.getParent(sp1), row1);
        expect_eq(doc.getParent(mo1), row1);
        expect_eq(doc.getParent(mi2), row1);
        expect_eq(doc.getFirstChild(sp1), NULL_NODE);
        expect_eq(countDirectChildren(&doc, row1), 4);

        expect_eq(doc.getElementById("math-break") != NULL_NODE, true);
        expect_eq(after_math != NULL_NODE, true);
        expect_eq(doc.getParent(after_math), math_host);
    });
}
