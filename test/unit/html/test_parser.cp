// tests/test_html_parser.cp
// CapSurf Milestone 2 End-to-End Test Suite:
// HTML5 Parser (Tokenizer + TreeBuilder) -> Arena DOM -> Selector Matcher -> Layout Coordinates
// Validates Doc.parseHtml, Adoption Agency Algorithm (AAA), Foster Parenting, and full pipeline.

import { test, expect_eq } from "cap:test";
import { Doc, Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { Matcher } from "../../../src/css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../../src/css/cascade.cp";
import { LayoutTree, Rect } from "../../../src/layout/layout.cp";

fn main() {
    test("1. CapSurf HTML5 Document Structure & Tree Building (Doc.parseHtml)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html>" ++
            "<html>" ++
            "<head>" ++
            "  <title>CapSurf Engine</title>" ++
            "</head>" ++
            "<body>" ++
            "  <header id=\"main-header\" class=\"header navbar\">" ++
            "    <h1 id=\"heading\" class=\"title\">CapSurf Headless</h1>" ++
            "    <p id=\"intro\" class=\"lead text-primary\">Lightweight agent-native browser</p>" ++
            "  </header>" ++
            "  <main class=\"content\">" ++
            "    <ul class=\"feature-list\">" ++
            "      <li class=\"item\">Zero GC</li>" ++
            "      <li class=\"item\">Arena DOM</li>" ++
            "      <li class=\"item active\">Million-Agent Concurrency</li>" ++
            "    </ul>" ++
            "  </main>" ++
            "</body>" ++
            "</html>";

        doc.parseHtml(html) catch unreachable;

        // Verify root html element exists
        expect_eq(doc.root_element != NULL_NODE, true);
        expect_eq(std.ascii.eqlIgnoreCase(doc.getTagName(doc.root_element), "html"), true);

        // Verify element by ID
        const heading_id = doc.getElementById("heading");
        expect_eq(heading_id != NULL_NODE, true);
        expect_eq(std.mem.eql(u8, doc.getTagName(heading_id), "h1"), true);

        const intro_id = doc.getElementById("intro");
        expect_eq(intro_id != NULL_NODE, true);
        expect_eq(doc.nodes.items[@as(usize, intro_id)].hasClass("lead"), true);
        expect_eq(doc.nodes.items[@as(usize, intro_id)].hasClass("text-primary"), true);

        // Verify CSS selector queries on parsed HTML
        const q_title = Matcher.querySelector(&doc, "head > title");
        expect_eq(q_title != NULL_NODE, true);

        const title_child = doc.getFirstChild(q_title);
        expect_eq(title_child != NULL_NODE, true);
        expect_eq(std.mem.eql(u8, doc.getTextContent(title_child), "CapSurf Engine"), true);

        // Verify querySelectorAll on list items
        let items: [16]NodeId = [NULL_NODE; 16];
        const item_count = Matcher.querySelectorAll(&doc, "ul.feature-list > li.item", &items);
        expect_eq(item_count, 3);

        const q_active = Matcher.querySelector(&doc, "li.item.active");
        expect_eq(q_active != NULL_NODE, true);
    });

    test("2. Adoption Agency Algorithm (Misnested Formatting Tags)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        // Standard AAA test case: <div><b>Bold <i>Italic</b> Still Italic?</i></div>
        const html = "<div><b>Bold <i>Italic</b> Still Italic?</i></div>";
        doc.parseHtml(html) catch unreachable;

        // Verify <b> element exists
        const b_elem = Matcher.querySelector(&doc, "div > b");
        expect_eq(b_elem != NULL_NODE, true);

        // Verify both <i> elements exist after adoption agency restructuring
        let i_nodes: [16]NodeId = [NULL_NODE; 16];
        const i_count = Matcher.querySelectorAll(&doc, "i", &i_nodes);
        expect_eq(i_count, 2);

        // The second <i> should be a direct child of <div>, not inside <b>!
        const second_i = i_nodes[1];
        expect_eq(second_i != NULL_NODE, true);
        const div_elem = Matcher.querySelector(&doc, "div");
        expect_eq(doc.getParent(second_i), div_elem);
    });

    test("3. Table Foster Parenting & Implicit Tbody Construction", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        // Stray text around table rows triggers foster parenting and implicit tbody insertion
        const html = "<div><table>hello<tr><td id=\"cell\">data</td></tr>world</table></div>";
        doc.parseHtml(html) catch unreachable;

        // Implicit <tbody> must be constructed
        const tbody_elem = Matcher.querySelector(&doc, "table > tbody");
        expect_eq(tbody_elem != NULL_NODE, true);

        // Cell must be found under tbody > tr > td
        const td_elem = Matcher.querySelector(&doc, "table > tbody > tr > td#cell");
        expect_eq(td_elem != NULL_NODE, true);

        // The text 'hello' must be foster-parented outside <table> into <div>
        const div_elem = Matcher.querySelector(&doc, "div");
        expect_eq(div_elem != NULL_NODE, true);

        const first_child = doc.getFirstChild(div_elem);
        expect_eq(first_child != NULL_NODE, true);
        expect_eq(std.mem.eql(u8, doc.getTextContent(first_child), "hello"), true);
    });

    test("4. Full Pipeline: HTML5 Parsing -> CSS Selectors -> Layout Coordinates", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<div id=\"card\">" ++
            "  <button id=\"btn-ok\" class=\"btn\">OK</button>" ++
            "  <button id=\"btn-cancel\" class=\"btn\">Cancel</button>" ++
            "</div>";

        doc.parseHtml(html) catch unreachable;

        // Query elements via CSS selectors
        const card_id = Matcher.querySelector(&doc, "#card");
        expect_eq(card_id != NULL_NODE, true);

        let btns: [8]NodeId = [NULL_NODE; 8];
        const btn_count = Matcher.querySelectorAll(&doc, ".btn", &btns);
        expect_eq(btn_count, 2);

        const btn1 = btns[0];
        const btn2 = btns[1];
        expect_eq(btn1, doc.getElementById("btn-ok"));
        expect_eq(btn2, doc.getElementById("btn-cancel"));

        // Cascade CSS stylesheet into computed styles
        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#card { display: flex; flex-direction: row; padding: 10px; width: 400px; height: 120px; }\n" ++
            "#btn-ok { width: 100px; height: 50px; }\n" ++
            "#btn-cancel { width: 120px; height: 50px; margin-left: 20px; }\n";

        const sheet = StyleSheet.parse(css_text, 800.0, false);
        let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        // Compute geometry layout from parsed HTML and cascaded CSS
        let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
        defer layout.deinit();

        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // Verify getBoundingClientRect coordinates for AI Agent action targeting
        const card_rect = layout.getBoundingClientRect(card_id);
        expect_eq(card_rect.visible, true);
        expect_eq(card_rect.width, 400.0);
        expect_eq(card_rect.height, 120.0);

        const btn1_rect = layout.getBoundingClientRect(btn1);
        expect_eq(btn1_rect.visible, true);
        expect_eq(btn1_rect.x, 10.0); // Inside 10px padding
        expect_eq(btn1_rect.y, 10.0);
        expect_eq(btn1_rect.width, 100.0);
        expect_eq(btn1_rect.height, 50.0);

        const btn2_rect = layout.getBoundingClientRect(btn2);
        expect_eq(btn2_rect.visible, true);
        expect_eq(btn2_rect.x, 130.0); // 10 (pad) + 100 (btn1) + 20 (margin)
        expect_eq(btn2_rect.y, 10.0);
        expect_eq(btn2_rect.width, 120.0);
        expect_eq(btn2_rect.height, 50.0);
    });
}
