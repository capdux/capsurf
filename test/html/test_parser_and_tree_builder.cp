// test/html/test_parser_and_tree_builder.cp
// White-box unit tests for src/html/parser.cp (HtmlParser) and src/html/tree_builder.cp (TreeBuilder).

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NodeType, NULL_NODE } from "../../src/dom/dom.cp";
import { HtmlParser } from "../../src/html/parser.cp";
import { TreeBuilder, InsertionMode } from "../../src/html/tree_builder.cp";
import { Tokenizer } from "../../src/html/tokenizer.cp";
import { Token, NextTokenizerState } from "../../src/html/tokens.cp";
import { HtmlTag } from "../../src/html/tags.cp";
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
    test("1. HtmlParser.parse: Initial/BeforeHtml/BeforeHead/InHead/AfterHead modes", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "   <!-- leading comment -->\n" ++
            "<!DOCTYPE html>" ++
            "<!-- pre-html comment -->" ++
            "<base href=\"https://example.com/\">" ++
            "<link rel=\"stylesheet\" href=\"style.css\">" ++
            "<meta charset=\"utf-8\">" ++
            "<title>Head &amp; Title</title>" ++
            "<style>body { color: #111; }</style>" ++
            "<script>const a = 1 < 2;</script>" ++
            "<div id=\"main\">body-start</div>";

        HtmlParser.parse(&doc, html) catch unreachable;

        expect_eq(doc.root_element != NULL_NODE, true);
        expect_eq(std.ascii.eqlIgnoreCase(doc.getTagName(doc.root_element), "html"), true);

        const head_el = Matcher.querySelector(&doc, "html > head");
        const body_el = Matcher.querySelector(&doc, "html > body");
        expect_eq(head_el != NULL_NODE, true);
        expect_eq(body_el != NULL_NODE, true);

        const base_el = Matcher.querySelector(&doc, "head > base");
        const link_el = Matcher.querySelector(&doc, "head > link");
        const meta_el = Matcher.querySelector(&doc, "head > meta");
        expect_eq(base_el != NULL_NODE, true);
        expect_eq(link_el != NULL_NODE, true);
        expect_eq(meta_el != NULL_NODE, true);
        expect_eq(doc.getFirstChild(base_el), NULL_NODE);
        expect_eq(doc.getFirstChild(link_el), NULL_NODE);
        expect_eq(doc.getFirstChild(meta_el), NULL_NODE);

        const title_el = Matcher.querySelector(&doc, "head > title");
        expect_eq(title_el != NULL_NODE, true);
        const title_txt = doc.getFirstChild(title_el);
        expect_eq(title_txt != NULL_NODE, true);
        expect_eq(std.mem.eql(u8, doc.getTextContent(title_txt), "Head & Title"), true);

        const style_el = Matcher.querySelector(&doc, "head > style");
        const script_el = Matcher.querySelector(&doc, "head > script");
        expect_eq(style_el != NULL_NODE, true);
        expect_eq(script_el != NULL_NODE, true);
        expect_eq(std.mem.eql(u8, doc.getTextContent(doc.getFirstChild(script_el)), "const a = 1 < 2;"), true);

        const main_el = Matcher.querySelector(&doc, "body > div#main");
        expect_eq(main_el != NULL_NODE, true);
        expect_eq(std.mem.eql(u8, doc.getTextContent(doc.getFirstChild(main_el)), "body-start"), true);
    });

    test("2. InBody implicit closures: li, dt/dd, headings, option/optgroup, p-in-button-scope", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<p id=\"p1\">before<hr id=\"hr1\">" ++
            "<ul id=\"u1\"><li id=\"l1\">A<li id=\"l2\">B<ul id=\"u2\"><li id=\"l2a\">B1<li id=\"l2b\">B2</ul><li id=\"l3\">C</ul>" ++
            "<dl id=\"d1\"><dt id=\"t1\">T1<dd id=\"d1a\">D1<dt id=\"t2\">T2<dd id=\"d2a\">D2</dl>" ++
            "<h1 id=\"h1\">H1<h2 id=\"h2\">H2</h1>" ++
            "<select id=\"sel\"><option id=\"o1\">1<option id=\"o2\">2<optgroup id=\"og1\"><option id=\"o3\">3</optgroup></select>";

        HtmlParser.parse(&doc, html) catch unreachable;

        const body_el = Matcher.querySelector(&doc, "body");
        const p1 = doc.getElementById("p1");
        const hr1 = doc.getElementById("hr1");
        expect_eq(doc.getParent(p1), body_el);
        expect_eq(doc.getParent(hr1), body_el);

        // <li> implicit closure respects nested <ul> scope
        const u1 = doc.getElementById("u1");
        const u2 = doc.getElementById("u2");
        const l1 = doc.getElementById("l1");
        const l2 = doc.getElementById("l2");
        const l3 = doc.getElementById("l3");
        const l2a = doc.getElementById("l2a");
        const l2b = doc.getElementById("l2b");
        expect_eq(doc.getParent(l1), u1);
        expect_eq(doc.getParent(l2), u1);
        expect_eq(doc.getParent(l3), u1);
        expect_eq(doc.getParent(u2), l2);
        expect_eq(doc.getParent(l2a), u2);
        expect_eq(doc.getParent(l2b), u2);

        // <dt> and <dd> implicitly close each other under <dl>
        const dl_el = doc.getElementById("d1");
        expect_eq(doc.getParent(doc.getElementById("t1")), dl_el);
        expect_eq(doc.getParent(doc.getElementById("d1a")), dl_el);
        expect_eq(doc.getParent(doc.getElementById("t2")), dl_el);
        expect_eq(doc.getParent(doc.getElementById("d2a")), dl_el);

        // <h2> implicitly closes <h1>; </h1> closes <h2>
        const h1 = doc.getElementById("h1");
        const h2 = doc.getElementById("h2");
        expect_eq(doc.getParent(h1), body_el);
        expect_eq(doc.getParent(h2), body_el);

        // <option> and <optgroup> implicit closures
        const sel = doc.getElementById("sel");
        const og1 = doc.getElementById("og1");
        expect_eq(doc.getParent(doc.getElementById("o1")), sel);
        expect_eq(doc.getParent(doc.getElementById("o2")), sel);
        expect_eq(doc.getParent(og1), sel);
        expect_eq(doc.getParent(doc.getElementById("o3")), og1);
    });

    test("3. Active formatting reconstruction, nested <a> AAA, and multi-tag AAA", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<div id=\"reconstruct\"><b>1<p id=\"p_in_b\">2</b>3</p></div>" ++
            "<div id=\"links\"><a id=\"a1\" href=\"1\">outer<a id=\"a2\" href=\"2\">inner</a></a></div>" ++
            "<div id=\"aaa\"><b><i><u>x</b>y</u></i></div>";

        HtmlParser.parse(&doc, html) catch unreachable;

        // <b> is reconstructed inside <p> and closed by </b>
        const p_in_b = doc.getElementById("p_in_b");
        expect_eq(p_in_b != NULL_NODE, true);
        const b_in_p = Matcher.querySelector(&doc, "#p_in_b > b");
        expect_eq(b_in_p != NULL_NODE, true);
        expect_eq(std.mem.eql(u8, doc.getTextContent(doc.getFirstChild(b_in_p)), "2"), true);

        // Nested <a> triggers AAA to close #a1 before opening #a2
        const links_div = doc.getElementById("links");
        const a1 = doc.getElementById("a1");
        const a2 = doc.getElementById("a2");
        expect_eq(doc.getParent(a1), links_div);
        expect_eq(doc.getParent(a2), links_div);

        // Multi-tag AAA: <u> and <i> are split around </b>
        let u_nodes: [8]NodeId = [NULL_NODE; 8];
        const u_count = Matcher.querySelectorAll(&doc, "#aaa u", &u_nodes);
        expect_eq(u_count, 2);
        let i_nodes: [8]NodeId = [NULL_NODE; 8];
        const i_count = Matcher.querySelectorAll(&doc, "#aaa i", &i_nodes);
        expect_eq(i_count, 2);
    });

    test("4. RawText / RcData modes (xmp, iframe, style, title) and EOF recovery", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<xmp id=\"x\"><tag>&amp;</tag></xmp>" ++
            "<iframe id=\"f\"><div>fallback</div></iframe>" ++
            "<style id=\"st\">a > b { content: \"<span>\"; }</style>" ++
            "<title id=\"t\">Unclosed &amp; EOF";

        HtmlParser.parse(&doc, html) catch unreachable;

        const xmp_el = doc.getElementById("x");
        expect_eq(xmp_el != NULL_NODE, true);
        expect_eq(countDirectChildren(&doc, xmp_el), 1);
        expect_eq(doc.nodes.items[@as(usize, doc.getFirstChild(xmp_el))].node_type == NodeType.Text, true);
        expect_eq(doc.getTextContent(doc.getFirstChild(xmp_el)), "<tag>&amp;</tag>");

        const iframe_el = doc.getElementById("f");
        expect_eq(iframe_el != NULL_NODE, true);
        expect_eq(countDirectChildren(&doc, iframe_el), 1);
        expect_eq(doc.getTextContent(doc.getFirstChild(iframe_el)), "<div>fallback</div>");

        const st_el = doc.getElementById("st");
        expect_eq(st_el != NULL_NODE, true);
        expect_eq(countDirectChildren(&doc, st_el), 1);
        expect_eq(doc.getTextContent(doc.getFirstChild(st_el)), "a > b { content: \"<span>\"; }");

        const title_el = doc.getElementById("t");
        expect_eq(title_el != NULL_NODE, true);
        expect_eq(countDirectChildren(&doc, title_el), 1);
        expect_eq(doc.getTextContent(doc.getFirstChild(title_el)), "Unclosed & EOF");
    });

    test("5. Table modes (InTable/InTableBody/InRow/InCell), th/td closure & foster parenting", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<div id=\"wrap\">" ++
            "<table id=\"tbl\">" ++
            "<span id=\"foster\">fostered</span>" ++
            "<thead><tr><th id=\"h1\">H1</th><th id=\"h2\">H2</th></tr></thead>" ++
            "<th id=\"h3\">H3<td id=\"c1\">C1" ++
            "</table>" ++
            "</div>";

        HtmlParser.parse(&doc, html) catch unreachable;

        const wrap_el = doc.getElementById("wrap");
        const tbl_el = doc.getElementById("tbl");
        const foster_el = doc.getElementById("foster");
        expect_eq(doc.getParent(foster_el), wrap_el);
        expect_eq(doc.getParent(tbl_el), wrap_el);
        // Fostered span and its fostered text appear before <table> inside #wrap
        expect_eq(doc.getFirstChild(wrap_el), foster_el);
        const foster_text: NodeId = doc.nodes.items[@as(usize, foster_el)].next_sibling;
        expect_eq(foster_text != NULL_NODE, true);
        expect_eq(doc.nodes.items[@as(usize, foster_text)].node_type == NodeType.Text, true);
        expect_eq(doc.getTextContent(foster_text), "fostered");
        expect_eq(doc.nodes.items[@as(usize, foster_text)].next_sibling, tbl_el);

        // Explicit <thead> cells
        const thead_tr = Matcher.querySelector(&doc, "#tbl > thead > tr");
        expect_eq(thead_tr != NULL_NODE, true);
        expect_eq(doc.getParent(doc.getElementById("h1")), thead_tr);
        expect_eq(doc.getParent(doc.getElementById("h2")), thead_tr);

        // Direct <th> and <td> inside <table> create implicit <tbody><tr> and auto-close previous cell
        const tbody_tr = Matcher.querySelector(&doc, "#tbl > tbody > tr");
        expect_eq(tbody_tr != NULL_NODE, true);
        expect_eq(doc.getParent(doc.getElementById("h3")), tbody_tr);
        expect_eq(doc.getParent(doc.getElementById("c1")), tbody_tr);
    });

    test("6. AfterBody/AfterAfterBody recovery, self-closing tag & TreeBuilder direct API", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<html><body><div id=\"sc\"/><span id=\"after-sc\">ok</span></body>" ++
            "<!-- after-body -->" ++
            "</html>" ++
            "<div id=\"tail\">tail</div>";

        HtmlParser.parse(&doc, html) catch unreachable;

        // Self-closing <div id="sc"/> pops immediately so <span id="after-sc"> is a sibling under <body>
        const body_el = Matcher.querySelector(&doc, "body");
        const sc_el = doc.getElementById("sc");
        const after_sc = doc.getElementById("after-sc");
        expect_eq(doc.getParent(sc_el), body_el);
        expect_eq(doc.getParent(after_sc), body_el);
        expect_eq(doc.getFirstChild(sc_el), NULL_NODE);

        // <div id="tail"> after </html> transitions back to InBody
        const tail_el = doc.getElementById("tail");
        expect_eq(tail_el != NULL_NODE, true);
        expect_eq(doc.getParent(tail_el), body_el);

        // Comment in AfterBody is appended under <html>
        let saw_after_body_comment = false;
        let child = doc.getFirstChild(doc.root_element);
        while (child != NULL_NODE) {
            if (doc.nodes.items[@as(usize, child)].node_type == NodeType.Comment) {
                if (std.mem.eql(u8, doc.getTextContent(child), " after-body ")) {
                    saw_after_body_comment = true;
                }
            }
            child = doc.nodes.items[@as(usize, child)].next_sibling;
        }
        expect_eq(saw_after_body_comment, true);

        // Direct TreeBuilder unit checks
        expect_eq(TreeBuilder.isAllWhitespace(" \t\r\n"), true);
        expect_eq(TreeBuilder.isAllWhitespace(" a "), false);

        let doc2 = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc2.deinit();
        let tb = TreeBuilder.init(&doc2);
        let tok = Tokenizer.init("<p id=\"unit\">hi</p>");
        while (t := tok.next()) {
            const ns = tb.processToken(t);
            tok.setState(ns);
        }
        expect_eq(tb.mode == InsertionMode.InBody, true);
        expect_eq(tb.currentTag() == HtmlTag.Body, true);
        const unit_p = doc2.getElementById("unit");
        expect_eq(unit_p != NULL_NODE, true);
        expect_eq(std.mem.eql(u8, doc2.getTextContent(doc2.getFirstChild(unit_p)), "hi"), true);
    });
}
