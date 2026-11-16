import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import {
    XPathEvaluator,
    XPathResult,
    evaluateXPath,
    ANY_TYPE,
    NUMBER_TYPE,
    STRING_TYPE,
    BOOLEAN_TYPE,
    UNORDERED_NODE_ITERATOR_TYPE,
    ORDERED_NODE_ITERATOR_TYPE,
    UNORDERED_NODE_SNAPSHOT_TYPE,
    ORDERED_NODE_SNAPSHOT_TYPE,
    ANY_UNORDERED_NODE_TYPE,
    FIRST_ORDERED_NODE_TYPE,
} from "../../src/dom/xpath.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    // Build DOM structure:
    // root (1: div, id="root", class="container")
    //   - header (2: header, id="header", class="top-nav")
    //       - h1 (3: h1) text="Title"
    //   - main (4: main, id="content", class="main")
    //       - div1 (5: div, id="section1", class="panel")
    //           - p1 (6: p, id="p1", class="paragraph") text="First paragraph"
    //           - button1 (7: button, id="btn1", class="btn primary") text="Submit"
    //       - div2 (8: div, id="section2", class="panel")
    //           - p2 (9: p, id="p2", class="paragraph") text="Second paragraph"
    //           - button2 (10: button, id="btn2", class="btn secondary") text="Cancel"
    //   - footer (11: footer, id="footer")
    //       - p3 (12: p, id="copy") text="Copyright 2026"
    let root = doc.createElementWithAttrs("div", "root", "container", "") catch unreachable;
    let header = doc.createElementWithAttrs("header", "header", "top-nav", "") catch unreachable;
    let h1 = doc.createElement("h1") catch unreachable;
    let h1_text = doc.createTextNode("Title") catch unreachable;
    doc.appendChild(h1, h1_text);
    doc.appendChild(header, h1);
    doc.appendChild(root, header);

    let main_elem = doc.createElementWithAttrs("main", "content", "main", "") catch unreachable;
    let div1 = doc.createElementWithAttrs("div", "section1", "panel", "") catch unreachable;
    let p1 = doc.createElementWithAttrs("p", "p1", "paragraph", "") catch unreachable;
    let p1_text = doc.createTextNode("First paragraph") catch unreachable;
    doc.appendChild(p1, p1_text);
    let btn1 = doc.createElementWithAttrs("button", "btn1", "btn primary", "") catch unreachable;
    let btn1_text = doc.createTextNode("Submit") catch unreachable;
    doc.appendChild(btn1, btn1_text);
    doc.appendChild(div1, p1);
    doc.appendChild(div1, btn1);
    doc.appendChild(main_elem, div1);

    let div2 = doc.createElementWithAttrs("div", "section2", "panel", "") catch unreachable;
    let p2 = doc.createElementWithAttrs("p", "p2", "paragraph", "") catch unreachable;
    let p2_text = doc.createTextNode("Second paragraph") catch unreachable;
    doc.appendChild(p2, p2_text);
    let btn2 = doc.createElementWithAttrs("button", "btn2", "btn secondary", "") catch unreachable;
    let btn2_text = doc.createTextNode("Cancel") catch unreachable;
    doc.appendChild(btn2, btn2_text);
    doc.appendChild(div2, p2);
    doc.appendChild(div2, btn2);
    doc.appendChild(main_elem, div2);
    doc.appendChild(root, main_elem);

    let footer = doc.createElementWithAttrs("footer", "footer", "", "") catch unreachable;
    let p3 = doc.createElementWithAttrs("p", "copy", "", "") catch unreachable;
    let p3_text = doc.createTextNode("Copyright 2026") catch unreachable;
    doc.appendChild(p3, p3_text);
    doc.appendChild(footer, p3);
    doc.appendChild(root, footer);

    let evaluator = XPathEvaluator{};

    // 1. Tag selection tests
    {
        let res = evaluator.evaluate(&doc, "//div", root, ORDERED_NODE_SNAPSHOT_TYPE);
        assert(res.snapshotLength() == 3, "should find 3 divs");
        assert(res.snapshotItem(0) == root, "first div is root");
        assert(res.snapshotItem(1) == div1, "second div is div1");
        assert(res.snapshotItem(2) == div2, "third div is div2");
    }

    {
        let res = evaluator.evaluate(&doc, "//button", root, ORDERED_NODE_SNAPSHOT_TYPE);
        assert(res.snapshotLength() == 2, "should find 2 buttons");
        assert(res.snapshotItem(0) == btn1, "first button is btn1");
        assert(res.snapshotItem(1) == btn2, "second button is btn2");
    }

    {
        let res = evaluator.evaluate(&doc, "//header", root, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == header, "should find header");
    }

    {
        let res = evaluator.evaluate(&doc, "//*", root, ORDERED_NODE_SNAPSHOT_TYPE);
        assert(res.snapshotLength() >= 10, "should find all elements");
    }

    // 2. Attribute filtering tests
    {
        let res = evaluator.evaluate(&doc, "//div[@id='section1']", root, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == div1, "should find section1 by id");
    }

    {
        let res = evaluator.evaluate(&doc, "//div[@class='panel']", root, ORDERED_NODE_SNAPSHOT_TYPE);
        assert(res.snapshotLength() == 2, "should find 2 panel divs by class");
    }

    {
        let res = evaluator.evaluate(&doc, "//button[@id='btn2']", root, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == btn2, "should find btn2 by id");
    }

    {
        let res = evaluator.evaluate(&doc, "//div[@id]", root, ORDERED_NODE_SNAPSHOT_TYPE);
        assert(res.snapshotLength() == 3, "all 3 divs have id attribute");
    }

    {
        let res = evaluator.evaluate(&doc, "//button[@class='primary']", root, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == btn1, "should match button with class token primary");
    }

    // 3. Hierarchy and descendant axes
    {
        let res = evaluator.evaluate(&doc, "/div/header", root, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == header, "direct child header of /div");
    }

    {
        let res = evaluator.evaluate(&doc, "/div/header/h1", root, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == h1, "path /div/header/h1");
    }

    {
        let res = evaluator.evaluate(&doc, "//main//p", root, ORDERED_NODE_SNAPSHOT_TYPE);
        assert(res.snapshotLength() == 2, "should find 2 p descendants of main");
        assert(res.snapshotItem(0) == p1, "first p is p1");
        assert(res.snapshotItem(1) == p2, "second p is p2");
    }

    {
        let res = evaluator.evaluate(&doc, "//footer/p", root, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == p3, "footer child p is p3");
    }

    // Relative evaluation from context node
    {
        let res = evaluator.evaluate(&doc, "div", main_elem, ORDERED_NODE_SNAPSHOT_TYPE);
        assert(res.snapshotLength() == 2, "relative div children of main");
        assert(res.snapshotItem(0) == div1, "first child is div1");
        assert(res.snapshotItem(1) == div2, "second child is div2");
    }

    {
        let res = evaluator.evaluate(&doc, ".//p", main_elem, ORDERED_NODE_SNAPSHOT_TYPE);
        assert(res.snapshotLength() == 2, "relative descendants of main");
    }

    {
        let res = evaluator.evaluate(&doc, "..", p1, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == div1, "parent of p1 is div1");
    }

    // 4. Text matching
    {
        let res = evaluator.evaluate(&doc, "//h1[text()='Title']", root, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == h1, "h1 with text Title");
    }

    {
        let res = evaluator.evaluate(&doc, "//p[text()='First paragraph']", root, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == p1, "p with text First paragraph");
    }

    // 5. Positional predicates
    {
        let res = evaluator.evaluate(&doc, "//button[1]", root, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == btn1, "first button");
    }

    {
        let res = evaluator.evaluate(&doc, "//button[2]", root, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == btn2, "second button");
    }

    // 6. count function
    {
        let res = evaluator.evaluate(&doc, "count(//div)", root, ANY_TYPE);
        assert(res.result_type == NUMBER_TYPE, "count result type is NUMBER_TYPE");
        assert(res.number_value == 3.0, "count(//div) is 3");
    }

    {
        let res = evaluator.evaluate(&doc, "count(//button)", root, NUMBER_TYPE);
        assert(res.number_value == 2.0, "count(//button) is 2");
    }

    {
        let res = evaluator.evaluate(&doc, "count(//p)", root, NUMBER_TYPE);
        assert(res.number_value == 3.0, "count(//p) is 3");
    }

    {
        let res = evaluator.evaluate(&doc, "count(/div/header)", root, NUMBER_TYPE);
        assert(res.number_value == 1.0, "count(/div/header) is 1");
    }

    // 7. Snapshot and iterator types
    {
        let res = evaluator.evaluate(&doc, "//button", root, ORDERED_NODE_SNAPSHOT_TYPE);
        assert(res.result_type == ORDERED_NODE_SNAPSHOT_TYPE, "snapshot type matches");
        assert(res.snapshotLength() == 2, "snapshot length is 2");
        assert(res.snapshotItem(0) == btn1, "snapshot item 0 is btn1");
        assert(res.snapshotItem(1) == btn2, "snapshot item 1 is btn2");
        assert(res.snapshotItem(2) == NULL_NODE, "snapshot item 2 is NULL_NODE");
    }

    {
        let res = evaluator.evaluate(&doc, "//button", root, ORDERED_NODE_ITERATOR_TYPE);
        assert(res.result_type == ORDERED_NODE_ITERATOR_TYPE, "iterator type matches");
        assert(res.iterateNext() == btn1, "iter 1 is btn1");
        assert(res.iterateNext() == btn2, "iter 2 is btn2");
        assert(res.iterateNext() == NULL_NODE, "iter 3 is NULL_NODE");
    }

    // 8. Helper function evaluateXPath
    {
        let res = evaluateXPath(&doc, "//header/h1", root, FIRST_ORDERED_NODE_TYPE);
        assert(res.single_node_value == h1, "evaluateXPath helper returns h1");
    }

    std.debug.print("ALL XPATH TESTS PASSED SUCCESSFULLY!\n", .{});
}
