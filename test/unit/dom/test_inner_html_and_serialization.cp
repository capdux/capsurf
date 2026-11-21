import { Document, ElementHandle, NodeId, NodeType, NULL_NODE } from "../../../src/dom/dom.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test_inner_html_and_serialization assertion failed");
    }
}

fn assertEqlStr(actual: []const u8, expected: []const u8, msg: []const u8) {
    if (!std.mem.eql(u8, actual, expected)) {
        std.debug.print("Assertion failed ({s}):\n  actual:   \"{s}\"\n  expected: \"{s}\"\n", .{ msg, actual, expected });
        @panic("test_inner_html_and_serialization string mismatch");
    }
}

fn testHtmlSerializationEscapingAndVoidTags()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    const root = doc.createElement("div")?;
    doc.setAttribute(root, "id", "main");
    doc.setAttribute(root, "class", "box primary");
    doc.setAttribute(root, "data-expr", "a < b & c > \"d\"");

    // Text node with special HTML characters
    const txt = doc.createTextNode("1 < 2 & 3 > 0")?;
    doc.appendChild(root, txt);

    // Comment node (must not escape <, >, &)
    const cmt = doc.createCommentNode(" note: a < b & c > d ")?;
    doc.appendChild(root, cmt);

    // Void elements (must not emit closing tag or children)
    const br = doc.createElement("br")?;
    doc.appendChild(root, br);

    const img = doc.createElement("IMG")?;
    doc.setAttribute(img, "src", "a.png?x=1&y=2");
    doc.setAttribute(img, "alt", "hi \"there\"");
    // Even if a child is appended in arena, void tag serialization stops before children
    const ignored_child = doc.createElement("span")?;
    doc.appendChild(img, ignored_child);
    doc.appendChild(root, img);

    const input = doc.createElement("input")?;
    doc.setAttribute(input, "type", "text");
    doc.appendChild(root, input);

    // Raw text tags (script / style must NOT escape text content)
    const script = doc.createElement("script")?;
    const script_txt = doc.createTextNode("if (a < b && c > d) { x = \"ok\"; }")?;
    doc.appendChild(script, script_txt);
    doc.appendChild(root, script);

    const style = doc.createElement("STYLE")?;
    const style_txt = doc.createTextNode("div > p { content: \"a&b\"; }")?;
    doc.appendChild(style, style_txt);
    doc.appendChild(root, style);

    const inner = doc.getInnerHtml(root)?;
    defer std.heap.page_allocator.free(inner);
    assertEqlStr(
        inner,
        "1 &lt; 2 &amp; 3 &gt; 0<!-- note: a < b & c > d --><br><IMG src=\"a.png?x=1&amp;y=2\" alt=\"hi &quot;there&quot;\"><input type=\"text\"><script>if (a < b && c > d) { x = \"ok\"; }</script><STYLE>div > p { content: \"a&b\"; }</STYLE>",
        "getInnerHtml escaping, comment, void, raw text",
    );

    const outer = doc.getOuterHtml(root)?;
    defer std.heap.page_allocator.free(outer);
    assertEqlStr(
        outer,
        "<div id=\"main\" class=\"box primary\" data-expr=\"a &lt; b &amp; c &gt; &quot;d&quot;\">1 &lt; 2 &amp; 3 &gt; 0<!-- note: a < b & c > d --><br><IMG src=\"a.png?x=1&amp;y=2\" alt=\"hi &quot;there&quot;\"><input type=\"text\"><script>if (a < b && c > d) { x = \"ok\"; }</script><STYLE>div > p { content: \"a&b\"; }</STYLE></div>",
        "getOuterHtml full serialization",
    );

    // NULL_NODE and out-of-bounds boundaries
    const null_inner = doc.getInnerHtml(NULL_NODE)?;
    defer std.heap.page_allocator.free(null_inner);
    assertEqlStr(null_inner, "", "getInnerHtml(NULL_NODE) == empty");

    const oob_outer = doc.getOuterHtml(99999)?;
    defer std.heap.page_allocator.free(oob_outer);
    assertEqlStr(oob_outer, "", "getOuterHtml(oob) == empty");
}

fn testSetInnerHtmlReplacementAndOrphanCleanup()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    const container = doc.createElement("section")?;
    const old_a = doc.createElement("p")?;
    const old_a_txt = doc.createTextNode("old text")?;
    doc.appendChild(old_a, old_a_txt);
    const old_b = doc.createElement("span")?;
    doc.appendChild(container, old_a);
    doc.appendChild(container, old_b);

    assert(doc.nodes.items[@as(usize, container)].child_element_count == 2, "initial child_element_count == 2");

    // Replace subtree via setInnerHtml
    doc.setInnerHtml(container, "<p class=\"lead\">Hello <b>World</b></p><!--done--><hr>")?;

    // Old direct children must be detached/orphaned
    assert(doc.nodes.items[@as(usize, old_a)].parent == NULL_NODE, "old_a.parent == NULL_NODE");
    assert(doc.nodes.items[@as(usize, old_a)].prev_sibling == NULL_NODE, "old_a.prev_sibling == NULL_NODE");
    assert(doc.nodes.items[@as(usize, old_a)].next_sibling == NULL_NODE, "old_a.next_sibling == NULL_NODE");
    assert(doc.nodes.items[@as(usize, old_b)].parent == NULL_NODE, "old_b.parent == NULL_NODE");
    assert(doc.nodes.items[@as(usize, old_b)].prev_sibling == NULL_NODE, "old_b.prev_sibling == NULL_NODE");
    assert(doc.nodes.items[@as(usize, old_b)].next_sibling == NULL_NODE, "old_b.next_sibling == NULL_NODE");

    assert(doc.nodes.items[@as(usize, container)].child_element_count == 2, "new child_element_count == 2 (p, hr)");

    const new_inner = doc.getInnerHtml(container)?;
    defer std.heap.page_allocator.free(new_inner);
    assertEqlStr(new_inner, "<p class=\"lead\">Hello <b>World</b></p><!--done--><hr>", "setInnerHtml replaced subtree");

    // Empty string clears all children and orphans them
    const first_new = doc.nodes.items[@as(usize, container)].first_child;
    assert(first_new != NULL_NODE, "first_new exists before clear");
    doc.setInnerHtml(container, "")?;

    assert(doc.nodes.items[@as(usize, container)].first_child == NULL_NODE, "empty setInnerHtml clears first_child");
    assert(doc.nodes.items[@as(usize, container)].last_child == NULL_NODE, "empty setInnerHtml clears last_child");
    assert(doc.nodes.items[@as(usize, container)].child_element_count == 0, "empty setInnerHtml clears child_element_count");
    assert(doc.nodes.items[@as(usize, first_new)].parent == NULL_NODE, "cleared child is orphaned");

    const cleared_inner = doc.getInnerHtml(container)?;
    defer std.heap.page_allocator.free(cleared_inner);
    assertEqlStr(cleared_inner, "", "cleared getInnerHtml == empty");

    // ElementHandle wrapper round-trip
    let handle = ElementHandle { doc: &doc, node_id: container };
    handle.setInnerHtml("<em>via handle</em>")?;
    const h_inner = handle.getInnerHtml()?;
    defer std.heap.page_allocator.free(h_inner);
    assertEqlStr(h_inner, "<em>via handle</em>", "ElementHandle.getInnerHtml");
    const h_outer = handle.getOuterHtml()?;
    defer std.heap.page_allocator.free(h_outer);
    assertEqlStr(h_outer, "<section><em>via handle</em></section>", "ElementHandle.getOuterHtml");

    // Boundary guards on NULL_NODE and OOB
    doc.setInnerHtml(NULL_NODE, "<p>noop</p>")?;
    doc.setInnerHtml(99999, "<p>noop</p>")?;
}

fn testTextContentAndScriptText()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    const txt = doc.createTextNode("alpha")?;
    assertEqlStr(doc.getTextContent(txt), "alpha", "getTextContent on text node");
    assertEqlStr(doc.getTextContent(NULL_NODE), "", "getTextContent(NULL_NODE) == empty");
    assertEqlStr(doc.getTextContent(88888), "", "getTextContent(oob) == empty");

    // Replace text node in parent (verifies orphan cleanup + textContent replacement)
    const p = doc.createElement("p")?;
    doc.appendChild(p, txt);
    const txt2 = doc.createTextNode("beta")?;
    doc.replaceChild(p, txt2, txt);

    assert(doc.nodes.items[@as(usize, txt)].parent == NULL_NODE, "replaced text node is orphaned");
    assert(doc.nodes.items[@as(usize, p)].first_child == txt2, "p.first_child == txt2");
    assert(doc.nodes.items[@as(usize, p)].last_child == txt2, "p.last_child == txt2");
    assertEqlStr(doc.getTextContent(doc.nodes.items[@as(usize, p)].first_child), "beta", "new child textContent == beta");

    const p_outer = doc.getOuterHtml(p)?;
    defer std.heap.page_allocator.free(p_outer);
    assertEqlStr(p_outer, "<p>beta</p>", "outerHTML after text node replacement");

    // getScriptText concatenates multiple child Text nodes and ignores Comment nodes
    const script = doc.createElement("script")?;
    const s1 = doc.createTextNode("const a = 1; ")?;
    const sc = doc.createCommentNode("ignored")?;
    const s2 = doc.createTextNode("const b = 2;")?;
    doc.appendChild(script, s1);
    doc.appendChild(script, sc);
    doc.appendChild(script, s2);

    const script_text = doc.getScriptText(script, std.heap.page_allocator);
    defer std.heap.page_allocator.free(script_text);
    assertEqlStr(script_text, "const a = 1; const b = 2;", "getScriptText concatenates child text nodes");
}

fn testDocumentFragmentBatchInsertAndClear()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    // Create a DocumentFragment-like container node (NodeType.Document is not serialized as a tag)
    const frag_id: NodeId = @as(NodeId, @intCast(doc.nodes.items.len));
    doc.nodes.append(std.heap.page_allocator, .{
        id: frag_id,
        node_type: NodeType.Document,
        tag_name: "#document-fragment",
    })?;

    const e1 = doc.createElement("li")?;
    doc.appendChild(e1, doc.createTextNode("one")?);
    const t_mid = doc.createTextNode(",")?;
    const e2 = doc.createElement("li")?;
    doc.appendChild(e2, doc.createTextNode("two")?);
    const e3 = doc.createElement("li")?;
    doc.appendChild(e3, doc.createTextNode("three")?);

    doc.appendChild(frag_id, e1);
    doc.appendChild(frag_id, t_mid);
    doc.appendChild(frag_id, e2);
    doc.appendChild(frag_id, e3);

    assert(doc.nodes.items[@as(usize, frag_id)].child_element_count == 3, "fragment has 3 element children");

    // Serializing DocumentFragment node itself outputs only its children
    const frag_outer = doc.getOuterHtml(frag_id)?;
    defer std.heap.page_allocator.free(frag_outer);
    assertEqlStr(frag_outer, "<li>one</li>,<li>two</li><li>three</li>", "DocumentFragment getOuterHtml");

    // Batch insert into target <ul> via reparentChildren
    const ul = doc.createElement("ul")?;
    const existing = doc.createElement("li")?;
    doc.appendChild(existing, doc.createTextNode("zero")?);
    doc.appendChild(ul, existing);

    doc.reparentChildren(frag_id, ul);

    // Fragment must be completely cleared after batch insertion
    assert(doc.nodes.items[@as(usize, frag_id)].first_child == NULL_NODE, "fragment first_child cleared");
    assert(doc.nodes.items[@as(usize, frag_id)].last_child == NULL_NODE, "fragment last_child cleared");
    assert(doc.nodes.items[@as(usize, frag_id)].child_element_count == 0, "fragment child_element_count == 0");

    const frag_after = doc.getInnerHtml(frag_id)?;
    defer std.heap.page_allocator.free(frag_after);
    assertEqlStr(frag_after, "", "fragment innerHTML after reparent == empty");

    // Target <ul> must have all nodes in exact order with updated parent & element indices
    assert(doc.nodes.items[@as(usize, ul)].child_element_count == 4, "ul child_element_count == 4");
    assert(doc.nodes.items[@as(usize, e1)].parent == ul, "e1.parent == ul");
    assert(doc.nodes.items[@as(usize, t_mid)].parent == ul, "t_mid.parent == ul");
    assert(doc.nodes.items[@as(usize, e2)].parent == ul, "e2.parent == ul");
    assert(doc.nodes.items[@as(usize, e3)].parent == ul, "e3.parent == ul");
    assert(doc.nodes.items[@as(usize, existing)].element_index_in_parent == 1, "existing index == 1");
    assert(doc.nodes.items[@as(usize, e1)].element_index_in_parent == 2, "e1 index == 2");
    assert(doc.nodes.items[@as(usize, e2)].element_index_in_parent == 3, "e2 index == 3");
    assert(doc.nodes.items[@as(usize, e3)].element_index_in_parent == 4, "e3 index == 4");

    const ul_html = doc.getOuterHtml(ul)?;
    defer std.heap.page_allocator.free(ul_html);
    assertEqlStr(ul_html, "<ul><li>zero</li><li>one</li>,<li>two</li><li>three</li></ul>", "ul outerHTML after fragment batch insert");

    // Empty fragment and self/null boundary guards
    doc.reparentChildren(frag_id, ul);
    doc.reparentChildren(ul, ul);
    doc.reparentChildren(NULL_NODE, ul);
    doc.reparentChildren(frag_id, NULL_NODE);
    assert(doc.nodes.items[@as(usize, ul)].child_element_count == 4, "boundary reparentChildren did not alter ul");
}

pub fn main()! {
    testHtmlSerializationEscapingAndVoidTags()?;
    testSetInnerHtmlReplacementAndOrphanCleanup()?;
    testTextContentAndScriptText()?;
    testDocumentFragmentBatchInsertAndClear()?;
    std.debug.print("test_inner_html_and_serialization OK\n", .{});
}
