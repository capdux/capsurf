import { Document, ElementHandle, NodeId, NodeType, NULL_NODE } from "../../src/dom/dom.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test_node_traversal_and_clone_edge assertion failed");
    }
}

fn testCloneShallowAndDeepAttrTextPreservation()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    const root = doc.createElement("div")?;
    const orig = doc.createElement("section")?;
    doc.setId(orig, "hero");
    doc.setClassName(orig, "card primary");
    doc.setInlineStyle(orig, "display: none; color: red");
    doc.setAttribute(orig, "data-role", "banner");
    doc.setAttribute(orig, "aria-hidden", "true");

    const child_el = doc.createElement("p")?;
    doc.setAttribute(child_el, "data-idx", "1");
    const child_txt = doc.createTextNode("nested copy")?;
    const child_cmt = doc.createCommentNode("inner note")?;

    doc.appendChild(root, orig);
    doc.appendChild(orig, child_el);
    doc.appendChild(child_el, child_txt);
    doc.appendChild(orig, child_cmt);

    // Shallow clone: preserves attributes/flags, has no children and no parent/siblings
    const shallow_id = doc.cloneNode(orig, false)?;
    assert(shallow_id != NULL_NODE && shallow_id != orig, "shallow clone distinct id");
    assert(doc.nodes.items[@as(usize, shallow_id)].parent == NULL_NODE, "shallow clone detached parent");
    assert(doc.nodes.items[@as(usize, shallow_id)].first_child == NULL_NODE, "shallow clone has no children");
    assert(doc.nodes.items[@as(usize, shallow_id)].child_element_count == 0, "shallow clone child_element_count == 0");
    assert(doc.nodes.items[@as(usize, shallow_id)].is_display_none, "shallow clone preserves is_display_none");

    if (v := doc.getAttribute(shallow_id, "id")) {
        assert(std.mem.eql(u8, v, "hero"), "shallow clone preserves id");
    } else {
        @panic("shallow clone missing id");
    }
    if (v := doc.getAttribute(shallow_id, "class")) {
        assert(std.mem.eql(u8, v, "card primary"), "shallow clone preserves class");
    } else {
        @panic("shallow clone missing class");
    }
    if (v := doc.getAttribute(shallow_id, "data-role")) {
        assert(std.mem.eql(u8, v, "banner"), "shallow clone preserves data-role");
    } else {
        @panic("shallow clone missing data-role");
    }
    if (v := doc.getAttribute(shallow_id, "aria-hidden")) {
        assert(std.mem.eql(u8, v, "true"), "shallow clone preserves aria-hidden");
    } else {
        @panic("shallow clone missing aria-hidden");
    }

    // Mutating shallow clone attribute does not affect original
    doc.setAttribute(shallow_id, "data-role", "mutated");
    if (v := doc.getAttribute(orig, "data-role")) {
        assert(std.mem.eql(u8, v, "banner"), "orig attr unaffected by clone mutation");
    } else {
        @panic("orig missing data-role");
    }

    // Deep clone via ElementHandle: preserves subtree, attributes, and text/comment nodes
    let orig_handle = ElementHandle.init(&doc, orig);
    const deep_id = orig_handle.cloneNode(true)?;
    assert(deep_id != NULL_NODE && deep_id != orig && deep_id != shallow_id, "deep clone distinct id");
    assert(doc.nodes.items[@as(usize, deep_id)].parent == NULL_NODE, "deep clone detached parent");
    assert(doc.nodes.items[@as(usize, deep_id)].child_element_count == 1, "deep clone child_element_count == 1");

    const cloned_p = doc.nodes.items[@as(usize, deep_id)].first_child;
    const cloned_cmt = doc.nodes.items[@as(usize, deep_id)].last_child;
    assert(cloned_p != NULL_NODE && cloned_p != child_el, "deep clone cloned child_el");
    assert(cloned_cmt != NULL_NODE && cloned_cmt != child_cmt, "deep clone cloned child_cmt");
    assert(doc.nodes.items[@as(usize, cloned_p)].parent == deep_id, "cloned_p parent == deep_id");
    assert(doc.nodes.items[@as(usize, cloned_p)].next_sibling == cloned_cmt, "cloned_p next == cloned_cmt");
    assert(doc.nodes.items[@as(usize, cloned_cmt)].prev_sibling == cloned_p, "cloned_cmt prev == cloned_p");

    if (v := doc.getAttribute(cloned_p, "data-idx")) {
        assert(std.mem.eql(u8, v, "1"), "cloned_p preserves data-idx");
    } else {
        @panic("cloned_p missing data-idx");
    }

    const cloned_txt = doc.nodes.items[@as(usize, cloned_p)].first_child;
    assert(cloned_txt != NULL_NODE && cloned_txt != child_txt, "deep clone cloned text node");
    assert(std.mem.eql(u8, doc.getTextContent(cloned_txt), "nested copy"), "deep clone preserves text");
    assert(std.mem.eql(u8, doc.getTextContent(cloned_cmt), "inner note"), "deep clone preserves comment");
}

fn testCrossParentReorderAndReparent()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    const p1 = doc.createElement("ul")?;
    const p2 = doc.createElement("ol")?;
    const a = doc.createElement("li")?;
    const b = doc.createElement("li")?;
    const c = doc.createElement("li")?;
    const x = doc.createElement("li")?;

    doc.appendChild(p1, a);
    doc.appendChild(p1, b);
    doc.appendChild(p1, c);
    doc.appendChild(p2, x);

    // Move middle child `b` from p1 to before `x` in p2 via insertBefore
    doc.insertBefore(p2, b, x);

    assert(doc.nodes.items[@as(usize, p1)].child_element_count == 2, "p1 child_element_count == 2");
    assert(doc.nodes.items[@as(usize, p1)].first_child == a, "p1 first == a");
    assert(doc.nodes.items[@as(usize, p1)].last_child == c, "p1 last == c");
    assert(doc.nodes.items[@as(usize, a)].next_sibling == c, "a.next == c");
    assert(doc.nodes.items[@as(usize, c)].prev_sibling == a, "c.prev == a");
    assert(doc.nodes.items[@as(usize, c)].element_index_in_parent == 2, "c reindexed to 2");

    assert(doc.nodes.items[@as(usize, p2)].child_element_count == 2, "p2 child_element_count == 2");
    assert(doc.nodes.items[@as(usize, p2)].first_child == b, "p2 first == b");
    assert(doc.nodes.items[@as(usize, p2)].last_child == x, "p2 last == x");
    assert(doc.nodes.items[@as(usize, b)].parent == p2, "b.parent == p2");
    assert(doc.nodes.items[@as(usize, b)].prev_sibling == NULL_NODE, "b.prev == NULL_NODE");
    assert(doc.nodes.items[@as(usize, b)].next_sibling == x, "b.next == x");
    assert(doc.nodes.items[@as(usize, x)].prev_sibling == b, "x.prev == b");
    assert(doc.nodes.items[@as(usize, b)].element_index_in_parent == 1, "b index == 1");
    assert(doc.nodes.items[@as(usize, x)].element_index_in_parent == 2, "x index == 2");

    // Cross-parent replaceChild: move `c` from p1 to replace `x` in p2
    doc.replaceChild(p2, c, x);
    assert(doc.nodes.items[@as(usize, p1)].child_element_count == 1, "p1 count == 1 after replaceChild");
    assert(doc.nodes.items[@as(usize, p1)].first_child == a && doc.nodes.items[@as(usize, p1)].last_child == a, "p1 only has a");
    assert(doc.nodes.items[@as(usize, a)].next_sibling == NULL_NODE, "a.next == NULL_NODE");
    assert(doc.nodes.items[@as(usize, x)].parent == NULL_NODE, "replaced x detached");
    assert(doc.nodes.items[@as(usize, c)].parent == p2, "c.parent == p2");
    assert(doc.nodes.items[@as(usize, p2)].last_child == c, "p2 last == c");
    assert(doc.nodes.items[@as(usize, b)].next_sibling == c, "b.next == c");

    // reparentChildren: move all from p2 (b, c) into p1 (after a)
    doc.reparentChildren(p2, p1);
    assert(doc.nodes.items[@as(usize, p2)].first_child == NULL_NODE, "p2 emptied");
    assert(doc.nodes.items[@as(usize, p2)].last_child == NULL_NODE, "p2 last_child == NULL_NODE");
    assert(doc.nodes.items[@as(usize, p2)].child_element_count == 0, "p2 count == 0");
    assert(doc.nodes.items[@as(usize, p1)].child_element_count == 3, "p1 count == 3");
    assert(doc.nodes.items[@as(usize, p1)].first_child == a, "p1 first == a");
    assert(doc.nodes.items[@as(usize, p1)].last_child == c, "p1 last == c");
    assert(doc.nodes.items[@as(usize, a)].next_sibling == b, "a.next == b");
    assert(doc.nodes.items[@as(usize, b)].next_sibling == c, "b.next == c");
}

fn testInsertBeforeAndReplaceChildBoundaries()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    const parent = doc.createElement("div")?;
    const other = doc.createElement("div")?;
    const a = doc.createElement("span")?;
    const b = doc.createElement("p")?;
    const foreign = doc.createElement("b")?;
    const orphan = doc.createElement("i")?;

    doc.appendChild(other, foreign);

    // insertBefore with NULL_NODE ref_id appends to end
    let ph = ElementHandle.init(&doc, parent);
    ph.insertBefore(a, NULL_NODE);
    ph.insertBefore(b, NULL_NODE);
    assert(doc.nodes.items[@as(usize, parent)].first_child == a, "insertBefore NULL ref first == a");
    assert(doc.nodes.items[@as(usize, parent)].last_child == b, "insertBefore NULL ref last == b");

    // insertBefore when child is already immediately before ref is a no-op
    const seq_before = doc.mutation_seq;
    doc.insertBefore(parent, a, b);
    assert(doc.mutation_seq == seq_before, "insertBefore already in place no-op");

    // insertBefore with ref_id belonging to another parent is a no-op
    doc.insertBefore(parent, orphan, foreign);
    assert(doc.nodes.items[@as(usize, orphan)].parent == NULL_NODE, "insertBefore foreign ref no-op");

    // insertBefore self as ref is a no-op
    doc.insertBefore(parent, a, a);
    assert(doc.nodes.items[@as(usize, parent)].first_child == a, "insertBefore self ref no-op");

    // replaceChild with old_child belonging to another parent is a no-op
    doc.replaceChild(parent, orphan, foreign);
    assert(doc.nodes.items[@as(usize, orphan)].parent == NULL_NODE, "replaceChild foreign old no-op");
    assert(doc.nodes.items[@as(usize, foreign)].parent == other, "foreign still under other");

    // replaceChild with identical new and old is a no-op
    doc.replaceChild(parent, a, a);
    assert(doc.nodes.items[@as(usize, a)].parent == parent, "replaceChild same child no-op");

    // replaceChild on first_child and last_child via ElementHandle
    ph.replaceChild(orphan, a);
    assert(doc.nodes.items[@as(usize, parent)].first_child == orphan, "replaceChild first_child updated");
    assert(doc.nodes.items[@as(usize, orphan)].next_sibling == b, "orphan.next == b");
    assert(doc.nodes.items[@as(usize, a)].parent == NULL_NODE, "old a detached");
}

fn testTraversalConnectivityAndNullOobGuards()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    // Before any element is created, root_element == NULL_NODE
    assert(!doc.isConnectedNode(NULL_NODE), "NULL_NODE not connected");
    assert(!doc.isConnectedNode(1), "node 1 not connected before creation");

    const root = doc.createElement("html")?;
    const body = doc.createElement("body")?;
    const t1 = doc.createTextNode("")?;
    const e1 = doc.createElement("div")?;
    const c1 = doc.createCommentNode("sep")?;
    const t2 = doc.createTextNode("word")?;
    const e2 = doc.createElement("span")?;
    const tmpl = doc.createElement("template")?;
    const in_tmpl = doc.createElement("p")?;
    const detached = doc.createElement("aside")?;

    doc.appendChild(root, body);
    doc.appendChild(body, t1);
    doc.appendChild(body, e1);
    doc.appendChild(body, c1);
    doc.appendChild(body, t2);
    doc.appendChild(body, e2);
    doc.appendChild(body, tmpl);
    doc.appendChild(tmpl, in_tmpl);

    // Element sibling traversal skips Text and Comment nodes
    assert(doc.prevElementSibling(e1) == NULL_NODE, "e1 has no prev element sibling");
    assert(doc.nextElementSibling(e1) == e2, "e1 next element sibling skips c1 and t2");
    assert(doc.prevElementSibling(e2) == e1, "e2 prev element sibling skips t2 and c1");
    assert(doc.nextElementSibling(t1) == e1, "t1 next element sibling == e1");
    assert(doc.prevElementSibling(t2) == e1, "t2 prev element sibling == e1");

    // isEmptyElement behavior with empty text, comment, non-empty text, and child elements
    const empty_div = doc.createElement("div")?;
    const empty_txt = doc.createTextNode("")?;
    const cmt_only = doc.createCommentNode("ignored")?;
    doc.appendChild(empty_div, empty_txt);
    doc.appendChild(empty_div, cmt_only);
    assert(doc.isEmptyElement(empty_div), "empty text + comment counts as empty");
    assert(!doc.isEmptyElement(body), "body with elements is not empty");

    const text_div = doc.createElement("div")?;
    const non_empty_txt = doc.createTextNode("x")?;
    doc.appendChild(text_div, non_empty_txt);
    assert(!doc.isEmptyElement(text_div), "non-empty text makes element non-empty");

    // isConnectedNode checks root_element and <template> boundary
    assert(doc.isConnectedNode(root), "root is connected");
    assert(doc.isConnectedNode(e2), "e2 is connected");
    assert(doc.isConnectedNode(tmpl), "template itself is connected");
    assert(!doc.isConnectedNode(in_tmpl), "node inside template is not connected");
    assert(!doc.isConnectedNode(detached), "detached node is not connected");
    assert(!doc.isConnectedNode(NULL_NODE), "NULL_NODE is not connected");
    assert(!doc.isConnectedNode(99999), "oob node is not connected");

    // NULL_NODE and out-of-bounds guards on traversal/mutation helpers
    assert(doc.prevElementSibling(NULL_NODE) == NULL_NODE, "prevElementSibling(NULL_NODE)");
    assert(doc.prevElementSibling(99999) == NULL_NODE, "prevElementSibling(oob)");
    assert(doc.nextElementSibling(NULL_NODE) == NULL_NODE, "nextElementSibling(NULL_NODE)");
    assert(doc.nextElementSibling(99999) == NULL_NODE, "nextElementSibling(oob)");
    assert(doc.isEmptyElement(NULL_NODE), "isEmptyElement(NULL_NODE) == true");
    assert(doc.isEmptyElement(99999), "isEmptyElement(oob) == true");
    assert((doc.cloneNode(NULL_NODE, true)?) == NULL_NODE, "cloneNode(NULL_NODE) == NULL_NODE");
    assert((doc.cloneNode(99999, false)?) == NULL_NODE, "cloneNode(oob) == NULL_NODE");

    doc.appendChild(99999, detached);
    doc.appendChild(body, 99999);
    doc.insertBefore(99999, detached, e1);
    doc.insertBefore(body, 99999, e1);
    doc.insertBefore(body, detached, 99999);
    doc.replaceChild(99999, detached, e1);
    doc.replaceChild(body, 99999, e1);
    doc.replaceChild(body, detached, 99999);
    doc.removeChild(NULL_NODE);
    doc.removeChild(99999);
    doc.reparentChildren(NULL_NODE, body);
    doc.reparentChildren(body, NULL_NODE);
    doc.reparentChildren(body, body);

    assert(doc.nodes.items[@as(usize, detached)].parent == NULL_NODE, "oob ops did not attach detached");
    assert(doc.isConnectedNode(e1), "oob ops did not corrupt connected e1");
}

pub fn main()! {
    testCloneShallowAndDeepAttrTextPreservation()?;
    testCrossParentReorderAndReparent()?;
    testInsertBeforeAndReplaceChildBoundaries()?;
    testTraversalConnectivityAndNullOobGuards()?;
    std.debug.print("test_node_traversal_and_clone_edge OK\n", .{});
}
