// test/dom/test_shadow_dom_and_slot_edge.cp
// Pure CP white-box edge-case tests for Document namespace isolation,
// <slot> / <template> / detached subtree node isolation, cross-Document
// copySubtreeFrom isolation, and ElementHandle boundary operations.

import { Document, ElementHandle, NodeId, NodeType, NULL_NODE } from "../../../src/dom/dom.cp";
import { Matcher } from "../../../src/css/matcher.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test_shadow_dom_and_slot_edge assertion failed");
    }
}

fn assertEqlStr(actual: []const u8, expected: []const u8, msg: []const u8) {
    if (!std.mem.eql(u8, actual, expected)) {
        std.debug.print("Assertion failed: {s}\n  actual:   '{s}'\n  expected: '{s}'\n", .{ msg, actual, expected });
        @panic("test_shadow_dom_and_slot_edge assertion failed");
    }
}

fn testAttributeNamespaceIsolation() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const el = doc.createElement("use") catch unreachable;
    const ns_xlink = "http://www.w3.org/1999/xlink";
    const ns_svg = "http://www.w3.org/2000/svg";
    const ns_custom = "https://example.com/ns";
    const ns_xml = "http://www.w3.org/XML/1998/namespace";
    const ns_xmlns = "http://www.w3.org/2000/xmlns/";

    // Same local name "href" under different namespaces must stay isolated
    doc.setAttributeNS(el, "", "href", "#plain");
    doc.setAttributeNS(el, ns_xlink, "xlink:href", "#icon-a");
    doc.setAttributeNS(el, ns_custom, "c:href", "#custom-a");

    if (v := doc.getAttributeNS(el, "", "href")) {
        assertEqlStr(v, "#plain", "empty ns href matches plain");
    } else {
        assert(false, "empty ns href should exist");
    }
    if (v := doc.getAttributeNS(el, ns_xlink, "href")) {
        assertEqlStr(v, "#icon-a", "xlink ns href matches icon-a");
    } else {
        assert(false, "xlink ns href should exist");
    }
    if (v := doc.getAttributeNS(el, ns_custom, "href")) {
        assertEqlStr(v, "#custom-a", "custom ns href matches custom-a");
    } else {
        assert(false, "custom ns href should exist");
    }
    assert(doc.getAttributeNS(el, ns_svg, "href") == null, "svg ns href does not exist");

    // Updating one namespace must not overwrite another
    doc.setAttributeNS(el, ns_xlink, "xlink:href", "#icon-b");
    if (v := doc.getAttributeNS(el, "", "href")) {
        assertEqlStr(v, "#plain", "empty ns href unaffected by xlink update");
    } else {
        assert(false, "empty ns href should still exist");
    }
    if (v := doc.getAttributeNS(el, ns_xlink, "href")) {
        assertEqlStr(v, "#icon-b", "xlink ns href updated to icon-b");
    } else {
        assert(false, "xlink ns href should exist after update");
    }

    // Removing one namespace leaves others intact
    assert(doc.removeAttributeNS(el, ns_xlink, "href"), "removeAttributeNS xlink href returns true");
    assert(doc.getAttributeNS(el, ns_xlink, "href") == null, "xlink href removed");
    assert(!doc.removeAttributeNS(el, ns_xlink, "href"), "removing missing xlink href returns false");
    assert(doc.getAttributeNS(el, "", "href") != null, "empty ns href remains after xlink removal");
    assert(doc.getAttributeNS(el, ns_custom, "href") != null, "custom ns href remains after xlink removal");

    // Implicit prefix namespace resolution via plain setAttribute
    doc.setAttribute(el, "xml:lang", "en-US");
    doc.setAttribute(el, "xmlns:x", "urn:x");
    if (v := doc.getAttributeNS(el, ns_xml, "lang")) {
        assertEqlStr(v, "en-US", "xml:lang resolves to XML namespace");
    } else {
        assert(false, "xml:lang should resolve under XML namespace");
    }
    if (v := doc.getAttributeNS(el, ns_xmlns, "x")) {
        assertEqlStr(v, "urn:x", "xmlns:x resolves to XMLNS namespace");
    } else {
        assert(false, "xmlns:x should resolve under XMLNS namespace");
    }
    assert(doc.getAttributeNS(el, "", "lang") == null, "prefixed xml:lang not matched under empty ns");
    assert(doc.removeAttributeNS(el, ns_xml, "lang"), "removeAttributeNS xml:lang succeeds");
    assert(doc.getAttributeNS(el, ns_xml, "lang") == null, "xml:lang removed");

    // Special id/class/style sync through setAttributeNS / removeAttributeNS
    doc.setAttributeNS(el, "", "id", "ns-el");
    doc.setAttributeNS(el, "", "class", "alpha beta");
    doc.setAttributeNS(el, "", "style", "display: none;");
    if (n := doc.getNode(el)) {
        assertEqlStr(n.id_attr, "ns-el", "setAttributeNS syncs id_attr");
        assert(n.hasClass("alpha") && n.hasClass("beta"), "setAttributeNS syncs class_list");
        assert(n.is_display_none, "setAttributeNS syncs is_display_none");
    } else {
        assert(false, "el node exists");
    }
    assert(doc.removeAttributeNS(el, "", "id"), "removeAttributeNS id returns true");
    if (n := doc.getNode(el)) {
        assertEqlStr(n.id_attr, "", "removeAttributeNS clears id_attr");
    }
}

fn testSlotAndSubtreeIsolation() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const html =
        "<html><body>" ++
        "<my-card id=\"card-host\">" ++
        "<slot id=\"light-slot\" name=\"header\"><span id=\"slot-default\">Default</span></slot>" ++
        "<span id=\"slotted-child\" slot=\"header\">Header Text</span>" ++
        "</my-card>" ++
        "<template id=\"card-tpl\">" ++
        "<slot id=\"tpl-slot\" name=\"body\"><p id=\"tpl-slot-fallback\">Tpl Fallback</p></slot>" ++
        "<div id=\"tpl-inner\" slot=\"body\">Inert</div>" ++
        "</template>" ++
        "</body></html>";

    doc.parseHtml(html) catch unreachable;

    // Light DOM <slot> and [slot=...] nodes are connected and queryable
    const light_slot = doc.getElementById("light-slot");
    const slot_default = doc.getElementById("slot-default");
    const slotted_child = doc.getElementById("slotted-child");
    assert(light_slot != NULL_NODE, "light DOM slot found by id");
    assert(slot_default != NULL_NODE, "light DOM slot fallback found by id");
    assert(slotted_child != NULL_NODE, "slotted light child found by id");
    assert(doc.isConnectedNode(light_slot), "light DOM slot is connected");
    assert(Matcher.querySelector(&doc, "my-card > slot[name=\"header\"]") == light_slot, "querySelector matches slot[name]");
    assert(Matcher.querySelector(&doc, "[slot=\"header\"]") == slotted_child, "querySelector matches [slot=header]");

    // <template> inert subtree isolates its <slot> and descendants
    const card_tpl = doc.getElementById("card-tpl");
    assert(card_tpl != NULL_NODE, "template#card-tpl itself is found");
    assert(doc.isConnectedNode(card_tpl), "template#card-tpl itself is connected");
    assert(doc.getElementById("tpl-slot") == NULL_NODE, "slot inside template excluded from getElementById");
    assert(doc.getElementById("tpl-slot-fallback") == NULL_NODE, "slot fallback inside template excluded");
    assert(doc.getElementById("tpl-inner") == NULL_NODE, "slotted node inside template excluded");
    assert(Matcher.querySelector(&doc, "#tpl-slot") == NULL_NODE, "querySelector excludes #tpl-slot in template");
    assert(Matcher.querySelector(&doc, "[slot=\"body\"]") == NULL_NODE, "querySelector excludes [slot=body] in template");

    // Detached subtree isolation via removeChild
    doc.removeChild(slotted_child);
    assert(!doc.isConnectedNode(slotted_child), "removed slotted child is disconnected");
    assert(doc.getElementById("slotted-child") == NULL_NODE, "removed node excluded from getElementById");
    assert(Matcher.querySelector(&doc, "[slot=\"header\"]") == NULL_NODE, "removed node excluded from querySelector");

    // Re-attaching restores connectivity
    const host = doc.getElementById("card-host");
    doc.appendChild(host, slotted_child);
    assert(doc.isConnectedNode(slotted_child), "re-attached slotted child is connected");
    assert(doc.getElementById("slotted-child") == slotted_child, "re-attached node found by getElementById");

    // NULL_NODE and out-of-bounds id checks
    assert(!doc.isConnectedNode(NULL_NODE), "NULL_NODE is not connected");
    assert(!doc.isConnectedNode(99999), "out-of-bounds NodeId is not connected");
    assert(doc.getElementById("") == NULL_NODE, "empty id returns NULL_NODE");
}

fn testCrossDocumentArenaIsolation() {
    let doc_a = Document.init(std.heap.page_allocator) catch unreachable;
    let doc_b = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc_b.deinit();

    doc_a.parseHtml("<html><body><div id=\"root-a\"></div></body></html>") catch unreachable;
    doc_b.parseHtml("<html><body><div id=\"root-b\"></div></body></html>") catch unreachable;

    const src_slot = doc_a.createElement("slot") catch unreachable;
    const ns_custom = "urn:cap:slot";
    doc_a.setAttributeNS(src_slot, ns_custom, "c:role", "primary");
    doc_a.setAttribute(src_slot, "id", "shared-id");
    doc_a.setAttribute(src_slot, "name", "main");

    const src_child = doc_a.createElement("span") catch unreachable;
    doc_a.setAttribute(src_child, "id", "copied-child");
    doc_a.appendChild(src_slot, src_child);

    const root_a = doc_a.getElementById("root-a");
    doc_a.appendChild(root_a, src_slot);

    // Before copy, doc_b does not see doc_a's elements
    assert(doc_b.getElementById("shared-id") == NULL_NODE, "doc_b isolated from doc_a before copy");

    const root_b = doc_b.getElementById("root-b");
    doc_b.copySubtreeFrom(&doc_a, src_slot, root_b) catch unreachable;

    const copied_slot = doc_b.getElementById("shared-id");
    const copied_child = doc_b.getElementById("copied-child");
    assert(copied_slot != NULL_NODE, "copied slot exists in doc_b");
    assert(copied_child != NULL_NODE, "copied child exists in doc_b");
    assert(doc_b.isConnectedNode(copied_slot), "copied slot is connected in doc_b");

    if (v := doc_b.getAttributeNS(copied_slot, ns_custom, "role")) {
        assertEqlStr(v, "primary", "copySubtreeFrom preserves custom attribute namespace");
    } else {
        assert(false, "copied slot should retain namespaced attribute");
    }

    // Mutating and deiniting doc_a must not affect doc_b
    doc_a.setAttributeNS(src_slot, ns_custom, "c:role", "mutated");
    doc_a.deinit();

    if (v := doc_b.getAttributeNS(copied_slot, ns_custom, "role")) {
        assertEqlStr(v, "primary", "doc_b remains isolated after doc_a mutation and deinit");
    } else {
        assert(false, "doc_b copied slot attribute still readable after doc_a deinit");
    }
}

fn testElementHandleBoundaries() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    doc.parseHtml("<html><body><div id=\"host\"></div></body></html>") catch unreachable;
    const host_id = doc.getElementById("host");
    let host = ElementHandle.init(&doc, host_id);

    assert(host.id() == host_id, "ElementHandle.id matches");
    assertEqlStr(host.tagName(), "div", "ElementHandle.tagName matches div");

    const child_a_id = doc.createElement("slot") catch unreachable;
    const child_b_id = doc.createElement("span") catch unreachable;
    const child_c_id = doc.createElement("p") catch unreachable;

    let child_a = ElementHandle.init(&doc, child_a_id);
    child_a.setId("slot-a");
    child_a.setClassName("slot-cls");
    child_a.setInlineStyle("color: red;");
    child_a.setAttribute("name", "alpha");
    child_a.setAttributeNS("urn:test", "t:key", "val1");

    if (v := child_a.getAttribute("name")) {
        assertEqlStr(v, "alpha", "ElementHandle.getAttribute returns alpha");
    } else {
        assert(false, "ElementHandle.getAttribute should return alpha");
    }
    if (v := child_a.getAttributeNS("urn:test", "key")) {
        assertEqlStr(v, "val1", "ElementHandle.getAttributeNS returns val1");
    } else {
        assert(false, "ElementHandle.getAttributeNS should return val1");
    }

    const attr_names = child_a.getAttributeNames() catch unreachable;
    defer doc.allocator.free(attr_names);
    assert(attr_names.len >= 5, "getAttributeNames includes regular, ns, id, class, style");

    // appendChild, insertBefore, replaceChild via ElementHandle
    host.appendChild(child_a_id);
    host.insertBefore(child_b_id, child_a_id);
    assert(doc.getFirstChild(host_id) == child_b_id, "insertBefore places child_b before child_a");
    host.replaceChild(child_c_id, child_b_id);
    assert(doc.getFirstChild(host_id) == child_c_id, "replaceChild replaces child_b with child_c");
    assert(!doc.isConnectedNode(child_b_id), "replaced child_b is no longer connected");

    // Self-append and invalid node boundaries are safe no-ops
    host.appendChild(host_id);
    host.appendChild(NULL_NODE);
    host.insertBefore(child_a_id, child_a_id);
    host.appendChild(99999);
    assert(doc.getFirstChild(host_id) == child_c_id, "invalid append/insert operations leave tree intact");

    // cloneNode + innerHTML / outerHTML
    const cloned_id = child_a.cloneNode(true) catch unreachable;
    assert(cloned_id != child_a_id, "cloneNode produces distinct NodeId");
    if (v := doc.getAttributeNS(cloned_id, "urn:test", "key")) {
        assertEqlStr(v, "val1", "ElementHandle.cloneNode preserves namespaced attrs");
    } else {
        assert(false, "cloned node should preserve namespaced attrs");
    }

    let child_c = ElementHandle.init(&doc, child_c_id);
    child_c.setInnerHtml("<b>Hi</b>") catch unreachable;
    const inner = child_c.getInnerHtml() catch unreachable;
    defer doc.allocator.free(inner);
    assertEqlStr(inner, "<b>Hi</b>", "ElementHandle.getInnerHtml matches setInnerHtml");

    const outer = child_c.getOuterHtml() catch unreachable;
    defer doc.allocator.free(outer);
    assertEqlStr(outer, "<p><b>Hi</b></p>", "ElementHandle.getOuterHtml matches");

    assert(child_a.removeAttributeNS("urn:test", "key"), "ElementHandle.removeAttributeNS succeeds");
    assert(!child_a.removeAttribute("nonexistent"), "ElementHandle.removeAttribute missing returns false");
}

pub fn main() {
    testAttributeNamespaceIsolation();
    testSlotAndSubtreeIsolation();
    testCrossDocumentArenaIsolation();
    testElementHandleBoundaries();
    std.debug.print("test_shadow_dom_and_slot_edge: ALL PASS\n", .{});
}
