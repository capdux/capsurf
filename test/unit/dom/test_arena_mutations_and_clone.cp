import { Document, NodeId, NodeType, NULL_NODE } from "../../../src/dom/dom.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test_arena_mutations_and_clone assertion failed");
    }
}

fn testNodeCreationAndArenaSentinel()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    assert(doc.nodeCount() == 1, "initial arena should contain only NULL_NODE sentinel");
    assert(doc.getNode(NULL_NODE) == null, "getNode(NULL_NODE) must return null");
    assert(doc.getNode(9999) == null, "getNode(out-of-bounds) must return null");
    assert(doc.root_element == NULL_NODE, "initial root_element should be NULL_NODE");

    const el = doc.createElement("div")?;
    assert(el != NULL_NODE, "created element id must not be NULL_NODE");
    assert(doc.root_element == el, "first element becomes root_element");
    assert(doc.nodeCount() == 2, "nodeCount after 1 element");

    if (n := doc.getNode(el)) {
        assert(n.node_type == NodeType.Element, "element node_type");
        assert(std.mem.eql(u8, n.tag_name, "div"), "element tag_name");
    } else {
        @panic("getNode(el) returned null");
    }

    const txt = doc.createTextNode("hello arena")?;
    assert(txt != NULL_NODE && txt != el, "text node id");
    if (tn := doc.getNode(txt)) {
        assert(tn.node_type == NodeType.Text, "text node_type");
        assert(std.mem.eql(u8, tn.text_content, "hello arena"), "text content");
    } else {
        @panic("getNode(txt) returned null");
    }

    const cmt = doc.createCommentNode("arena comment")?;
    assert(cmt != NULL_NODE && cmt != txt, "comment node id");
    if (cn := doc.getNode(cmt)) {
        assert(cn.node_type == NodeType.Comment, "comment node_type");
        assert(std.mem.eql(u8, cn.text_content, "arena comment"), "comment content");
    } else {
        @panic("getNode(cmt) returned null");
    }
}

fn testInsertRemoveReplaceMutations()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    const parent = doc.createElement("div")?;
    const a = doc.createElement("span")?;
    const b = doc.createElement("p")?;
    const c = doc.createElement("em")?;
    const d = doc.createElement("strong")?;
    const txt = doc.createTextNode("lead")?;

    doc.appendChild(parent, a);
    doc.appendChild(parent, c);
    doc.insertBefore(parent, b, c);

    // Order: a -> b -> c
    assert(doc.nodes.items[@as(usize, parent)].first_child == a, "first_child == a");
    assert(doc.nodes.items[@as(usize, parent)].last_child == c, "last_child == c");
    assert(doc.nodes.items[@as(usize, parent)].child_element_count == 3, "child_element_count == 3");
    assert(doc.nodes.items[@as(usize, a)].next_sibling == b, "a.next == b");
    assert(doc.nodes.items[@as(usize, b)].prev_sibling == a, "b.prev == a");
    assert(doc.nodes.items[@as(usize, b)].next_sibling == c, "b.next == c");
    assert(doc.nodes.items[@as(usize, c)].prev_sibling == b, "c.prev == b");
    assert(doc.nodes.items[@as(usize, a)].element_index_in_parent == 1, "a index == 1");
    assert(doc.nodes.items[@as(usize, b)].element_index_in_parent == 2, "b index == 2");
    assert(doc.nodes.items[@as(usize, c)].element_index_in_parent == 3, "c index == 3");

    // Insert text node before first_child
    doc.insertBefore(parent, txt, a);
    assert(doc.nodes.items[@as(usize, parent)].first_child == txt, "first_child == txt");
    assert(doc.nodes.items[@as(usize, txt)].next_sibling == a, "txt.next == a");
    assert(doc.nodes.items[@as(usize, a)].prev_sibling == txt, "a.prev == txt");
    assert(doc.nodes.items[@as(usize, parent)].child_element_count == 3, "text does not increase child_element_count");
    assert(doc.nodes.items[@as(usize, txt)].element_index_in_parent == 0, "text element_index == 0");

    // Move already-attached node c before a -> order: txt -> c -> a -> b
    doc.insertBefore(parent, c, a);
    assert(doc.nodes.items[@as(usize, parent)].first_child == txt, "move c: first_child == txt");
    assert(doc.nodes.items[@as(usize, parent)].last_child == b, "move c: last_child == b");
    assert(doc.nodes.items[@as(usize, txt)].next_sibling == c, "move c: txt.next == c");
    assert(doc.nodes.items[@as(usize, c)].prev_sibling == txt, "move c: c.prev == txt");
    assert(doc.nodes.items[@as(usize, c)].next_sibling == a, "move c: c.next == a");
    assert(doc.nodes.items[@as(usize, a)].prev_sibling == c, "move c: a.prev == c");
    assert(doc.nodes.items[@as(usize, b)].next_sibling == NULL_NODE, "move c: b.next == NULL_NODE");
    assert(doc.nodes.items[@as(usize, c)].element_index_in_parent == 1, "move c: c index == 1");
    assert(doc.nodes.items[@as(usize, a)].element_index_in_parent == 2, "move c: a index == 2");
    assert(doc.nodes.items[@as(usize, b)].element_index_in_parent == 3, "move c: b index == 3");

    // Replace a with d -> order: txt -> c -> d -> b
    doc.replaceChild(parent, d, a);
    assert(doc.nodes.items[@as(usize, d)].parent == parent, "d.parent == parent");
    assert(doc.nodes.items[@as(usize, c)].next_sibling == d, "c.next == d");
    assert(doc.nodes.items[@as(usize, d)].prev_sibling == c, "d.prev == c");
    assert(doc.nodes.items[@as(usize, d)].next_sibling == b, "d.next == b");
    assert(doc.nodes.items[@as(usize, b)].prev_sibling == d, "b.prev == d");
    assert(doc.nodes.items[@as(usize, a)].parent == NULL_NODE, "replaced a.parent == NULL_NODE");
    assert(doc.nodes.items[@as(usize, a)].prev_sibling == NULL_NODE, "replaced a.prev == NULL_NODE");
    assert(doc.nodes.items[@as(usize, a)].next_sibling == NULL_NODE, "replaced a.next == NULL_NODE");
    assert(doc.nodes.items[@as(usize, d)].element_index_in_parent == 2, "d index == 2");

    // Remove c and b -> order: txt -> d
    doc.removeChild(c);
    doc.removeChild(b);
    assert(doc.nodes.items[@as(usize, parent)].first_child == txt, "after remove: first == txt");
    assert(doc.nodes.items[@as(usize, parent)].last_child == d, "after remove: last == d");
    assert(doc.nodes.items[@as(usize, parent)].child_element_count == 1, "after remove: count == 1");
    assert(doc.nodes.items[@as(usize, d)].element_index_in_parent == 1, "after remove: d index == 1");
    assert(doc.nodes.items[@as(usize, c)].parent == NULL_NODE, "removed c.parent == NULL_NODE");
    assert(doc.nodes.items[@as(usize, b)].parent == NULL_NODE, "removed b.parent == NULL_NODE");
}

fn testCloneNodeShallowAndDeep()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    const src = doc.createElement("section")?;
    doc.setAttribute(src, "id", "hero");
    doc.setAttribute(src, "class", "card primary");
    doc.setAttribute(src, "data-role", "banner");

    const child_el = doc.createElement("h1")?;
    const child_txt = doc.createTextNode("Title")?;
    const child_cmt = doc.createCommentNode("note")?;
    doc.appendChild(child_el, child_txt);
    doc.appendChild(src, child_el);
    doc.appendChild(src, child_cmt);

    // Shallow clone
    const shallow = doc.cloneNode(src, false)?;
    assert(shallow != NULL_NODE && shallow != src, "shallow clone id");
    assert(doc.nodes.items[@as(usize, shallow)].parent == NULL_NODE, "shallow parent is NULL_NODE");
    assert(doc.nodes.items[@as(usize, shallow)].first_child == NULL_NODE, "shallow has no first_child");
    assert(doc.nodes.items[@as(usize, shallow)].last_child == NULL_NODE, "shallow has no last_child");
    assert(doc.nodes.items[@as(usize, shallow)].child_element_count == 0, "shallow child_element_count == 0");

    if (v := doc.getAttribute(shallow, "data-role")) {
        assert(std.mem.eql(u8, v, "banner"), "shallow preserves data-role");
    } else {
        @panic("shallow missing data-role");
    }

    // Deep clone
    const deep = doc.cloneNode(src, true)?;
    assert(deep != NULL_NODE && deep != src && deep != shallow, "deep clone id");

    const cloned_el = doc.nodes.items[@as(usize, deep)].first_child;
    assert(cloned_el != NULL_NODE && cloned_el != child_el, "deep cloned child_el is new node");
    assert(doc.nodes.items[@as(usize, cloned_el)].parent == deep, "cloned_el.parent == deep");

    const cloned_txt = doc.nodes.items[@as(usize, cloned_el)].first_child;
    assert(cloned_txt != NULL_NODE && cloned_txt != child_txt, "deep cloned text is new node");
    assert(std.mem.eql(u8, doc.getTextContent(cloned_txt), "Title"), "cloned text matches");

    const cloned_cmt = doc.nodes.items[@as(usize, cloned_el)].next_sibling;
    assert(cloned_cmt != NULL_NODE && cloned_cmt != child_cmt, "deep cloned comment is new node");
    assert(doc.nodes.items[@as(usize, deep)].last_child == cloned_cmt, "deep last_child == cloned_cmt");
    assert(std.mem.eql(u8, doc.getTextContent(cloned_cmt), "note"), "cloned comment matches");

    // Mutating deep clone must not affect src
    doc.setAttribute(deep, "data-role", "mutated");
    if (sv := doc.getAttribute(src, "data-role")) {
        assert(std.mem.eql(u8, sv, "banner"), "src attribute unaffected by clone mutation");
    } else {
        @panic("src missing data-role");
    }

    // Direct clone of text & comment nodes
    const txt_clone = doc.cloneNode(child_txt, false)?;
    assert(txt_clone != NULL_NODE && txt_clone != child_txt, "direct text clone");
    assert(std.mem.eql(u8, doc.getTextContent(txt_clone), "Title"), "direct text clone content");

    const cmt_clone = doc.cloneNode(child_cmt, true)?;
    assert(cmt_clone != NULL_NODE && cmt_clone != child_cmt, "direct comment clone");
    assert(std.mem.eql(u8, doc.getTextContent(cmt_clone), "note"), "direct comment clone content");
}

fn testAttributesAndNullSentinelBoundaries()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    const el = doc.createElement("button")?;
    doc.setAttribute(el, "type", "submit");
    doc.setAttribute(el, "disabled", "");

    if (v1 := doc.getAttribute(el, "type")) {
        assert(std.mem.eql(u8, v1, "submit"), "getAttribute type == submit");
    } else {
        @panic("missing type attr");
    }

    // Overwrite existing attribute
    doc.setAttribute(el, "type", "button");
    if (v2 := doc.getAttribute(el, "type")) {
        assert(std.mem.eql(u8, v2, "button"), "overwritten type == button");
    } else {
        @panic("missing overwritten type attr");
    }

    // Remove existing and non-existing attribute
    assert(doc.removeAttribute(el, "disabled"), "removeAttribute existing returns true");
    assert(!doc.removeAttribute(el, "disabled"), "removeAttribute already removed returns false");
    assert(doc.getAttribute(el, "disabled") == null, "getAttribute after removal is null");

    // NULL_NODE and out-of-bounds sentinel boundary guards
    assert((doc.cloneNode(NULL_NODE, true)?) == NULL_NODE, "cloneNode(NULL_NODE) == NULL_NODE");
    assert((doc.cloneNode(99999, false)?) == NULL_NODE, "cloneNode(oob) == NULL_NODE");
    assert(doc.getAttribute(NULL_NODE, "id") == null, "getAttribute(NULL_NODE) == null");
    assert(!doc.removeAttribute(NULL_NODE, "id"), "removeAttribute(NULL_NODE) == false");

    doc.setAttribute(NULL_NODE, "id", "noop");
    doc.appendChild(NULL_NODE, el);
    doc.appendChild(el, NULL_NODE);
    doc.appendChild(el, el);
    doc.insertBefore(NULL_NODE, el, NULL_NODE);
    doc.insertBefore(el, NULL_NODE, NULL_NODE);
    doc.insertBefore(el, el, el);
    doc.removeChild(NULL_NODE);
    doc.removeChild(99999);
    doc.replaceChild(NULL_NODE, el, el);
    doc.replaceChild(el, el, el);

    assert(doc.nodes.items[@as(usize, el)].parent == NULL_NODE, "sentinel ops did not corrupt el.parent");
    assert(doc.nodes.items[@as(usize, el)].first_child == NULL_NODE, "sentinel ops did not corrupt el.first_child");
}

pub fn main()! {
    testNodeCreationAndArenaSentinel()?;
    testInsertRemoveReplaceMutations()?;
    testCloneNodeShallowAndDeep()?;
    testAttributesAndNullSentinelBoundaries()?;
    std.debug.print("test_arena_mutations_and_clone OK\n", .{});
}
