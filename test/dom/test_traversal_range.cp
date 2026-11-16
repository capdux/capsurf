import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import {
    TreeWalker,
    NodeIterator,
    SHOW_ALL,
    SHOW_ELEMENT,
    FILTER_ACCEPT,
} from "../../src/dom/traversal.cp";
import {
    DOMRange,
    START_TO_START,
    START_TO_END,
    END_TO_END,
    findCommonAncestor,
    comparePoint,
} from "../../src/dom/range.cp";

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
    // root (1: div)
    //   - header (2: header)
    //   - main (3: main)
    //       - p1 (4: p)
    //       - p2 (5: p)
    //   - footer (6: footer)
    let root_id = doc.createElement("div") catch @panic("create div");
    let header_id = doc.createElement("header") catch @panic("create header");
    let main_id = doc.createElement("main") catch @panic("create main");
    let p1_id = doc.createElement("p") catch @panic("create p1");
    let p2_id = doc.createElement("p") catch @panic("create p2");
    let footer_id = doc.createElement("footer") catch @panic("create footer");

    doc.appendChild(root_id, header_id);
    doc.appendChild(root_id, main_id);
    doc.appendChild(main_id, p1_id);
    doc.appendChild(main_id, p2_id);
    doc.appendChild(root_id, footer_id);

    // 1. Test TreeWalker
    {
        let walker = TreeWalker.init(root_id, SHOW_ELEMENT);
        assert(walker.current_node == root_id, "initial walker node is root");

        let first = walker.firstChild(&doc);
        assert(first == header_id, "first child is header");

        let next_sib = walker.nextSibling(&doc);
        assert(next_sib == main_id, "next sibling of header is main");

        let main_child = walker.firstChild(&doc);
        assert(main_child == p1_id, "first child of main is p1");

        let parent = walker.parentNode(&doc);
        assert(parent == main_id, "parent of p1 is main");
    }

    // 2. Test NodeIterator
    {
        let iter = NodeIterator.init(root_id, SHOW_ELEMENT);
        let n1 = iter.nextNode(&doc);
        assert(n1 == root_id, "iter first is root");

        let n2 = iter.nextNode(&doc);
        assert(n2 == header_id, "iter second is header");

        let n3 = iter.nextNode(&doc);
        assert(n3 == main_id, "iter third is main");

        let n4 = iter.nextNode(&doc);
        assert(n4 == p1_id, "iter fourth is p1");

        let n5 = iter.nextNode(&doc);
        assert(n5 == p2_id, "iter fifth is p2");

        let n6 = iter.nextNode(&doc);
        assert(n6 == footer_id, "iter sixth is footer");

        // Step backwards
        let prev = iter.previousNode(&doc);
        assert(prev == footer_id, "iter previous steps back to footer");
    }

    // 3. Test DOMRange and Common Ancestor
    {
        let ca_p = findCommonAncestor(&doc, p1_id, p2_id);
        assert(ca_p == main_id, "common ancestor of p1 and p2 is main");

        let ca_hf = findCommonAncestor(&doc, header_id, footer_id);
        assert(ca_hf == root_id, "common ancestor of header and footer is root");

        let range = DOMRange.init();
        range.setStart(&doc, p1_id, 0);
        range.setEnd(&doc, p2_id, 1);
        assert(!range.collapsed, "range is not collapsed");
        assert(range.common_ancestor == main_id, "range common ancestor is main");

        // Test collapse to start
        range.collapse(true);
        assert(range.collapsed, "range is collapsed after collapse(true)");
        assert(range.start_container == p1_id && range.end_container == p1_id, "collapsed to p1");

        // Compare points
        let cmp = comparePoint(&doc, header_id, 0, footer_id, 0);
        assert(cmp < 0, "header comes before footer in tree order");
    }

    std.debug.print("test_traversal_range: ALL ASSERTIONS PASSED\n", .{});
}
