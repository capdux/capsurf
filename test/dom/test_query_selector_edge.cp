// test/dom/test_query_selector_edge.cp
// Pure CP white-box edge-case tests for querySelector, querySelectorAll,
// getElementById, getElementsByName, class/tag matching, scoped subtree queries,
// case-insensitive tag matching, empty/invalid selectors, and NULL_NODE boundaries.

import { Document, NodeId, NodeType, NULL_NODE } from "../../src/dom/dom.cp";
import { Matcher, SelectorList } from "../../src/css/matcher.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test_query_selector_edge assertion failed");
    }
}

fn containsNode(ids: []const NodeId, count: usize, target: NodeId): bool {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (ids[i] == target) return true;
    }
    return false;
}

// Helper for scoped subtree querySelector using SelectorList + Matcher.matchesSelectorList
fn querySelectorInSubtree(doc: *const Document, root_id: NodeId, selector_str: []const u8): NodeId {
    if (root_id == NULL_NODE || @as(usize, root_id) >= doc.nodes.items.len) {
        return NULL_NODE;
    }
    const list = SelectorList.parse(selector_str);
    if (list.count == 0) {
        return NULL_NODE;
    }
    return querySubtreeWalk(doc, root_id, list);
}

fn querySubtreeWalk(doc: *const Document, curr: NodeId, list: SelectorList): NodeId {
    if (curr == NULL_NODE || @as(usize, curr) >= doc.nodes.items.len) {
        return NULL_NODE;
    }
    let ch = doc.nodes.items[@as(usize, curr)].first_child;
    while (ch != NULL_NODE) {
        const cnode = &doc.nodes.items[@as(usize, ch)];
        if (cnode.node_type == NodeType.Element) {
            if (Matcher.matchesSelectorList(doc, ch, list)) {
                return ch;
            }
            const found = querySubtreeWalk(doc, ch, list);
            if (found != NULL_NODE) {
                return found;
            }
        }
        ch = cnode.next_sibling;
    }
    return NULL_NODE;
}

fn testEmptyDocAndNullNodeBoundaries() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    // Empty document (only sentinel NULL_NODE at index 0)
    assert(doc.getElementById("anything") == NULL_NODE, "empty doc getElementById returns NULL_NODE");
    assert(doc.getElementById("") == NULL_NODE, "empty doc getElementById empty string returns NULL_NODE");
    assert(Matcher.querySelector(&doc, "*") == NULL_NODE, "empty doc querySelector(*) returns NULL_NODE");
    assert(Matcher.querySelector(&doc, "div") == NULL_NODE, "empty doc querySelector(div) returns NULL_NODE");

    let buf: [8]NodeId = [NULL_NODE; 8];
    assert(Matcher.querySelectorAll(&doc, "*", buf[0..8]) == 0, "empty doc querySelectorAll(*) returns 0");

    // NULL_NODE and out-of-bounds NodeId boundaries
    assert(!doc.isConnectedNode(NULL_NODE), "NULL_NODE is not connected");
    assert(!doc.isConnectedNode(99999), "out-of-bounds NodeId is not connected");
    assert(doc.getFirstChild(NULL_NODE) == NULL_NODE, "getFirstChild(NULL_NODE) == NULL_NODE");
    assert(doc.getParent(NULL_NODE) == NULL_NODE, "getParent(NULL_NODE) == NULL_NODE");
    assert(doc.getFirstChild(99999) == NULL_NODE, "getFirstChild(99999) == NULL_NODE");
    assert(doc.getParent(99999) == NULL_NODE, "getParent(99999) == NULL_NODE");
    assert(doc.getAttribute(NULL_NODE, "id") == null, "getAttribute(NULL_NODE) == null");

    const list = SelectorList.parse("div, *");
    assert(!Matcher.matchesSelectorList(&doc, NULL_NODE, list), "matchesSelectorList(NULL_NODE) == false");
    assert(!Matcher.matchesSelectorList(&doc, 99999, list), "matchesSelectorList(99999) == false");
    assert(querySelectorInSubtree(&doc, NULL_NODE, "div") == NULL_NODE, "subtree query on NULL_NODE == NULL_NODE");
}

fn testEmptyAndInvalidSelectors() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const root = doc.createElement("div") catch unreachable;
    doc.setId(root, "root");
    const child = doc.createElement("span") catch unreachable;
    doc.setClassName(child, "item");
    doc.appendChild(root, child);

    let buf: [8]NodeId = [NULL_NODE; 8];

    // Empty and whitespace-only selectors
    assert(Matcher.querySelector(&doc, "") == NULL_NODE, "empty selector returns NULL_NODE");
    assert(Matcher.querySelector(&doc, "   \t\r\n  ") == NULL_NODE, "whitespace selector returns NULL_NODE");
    assert(Matcher.querySelectorAll(&doc, "", buf[0..8]) == 0, "empty querySelectorAll returns 0");
    assert(Matcher.querySelectorAll(&doc, "   ", buf[0..8]) == 0, "whitespace querySelectorAll returns 0");

    // Comma-only / empty segments
    assert(Matcher.querySelector(&doc, ",") == NULL_NODE, "single comma returns NULL_NODE");
    assert(Matcher.querySelector(&doc, " , , ") == NULL_NODE, "multiple empty commas return NULL_NODE");

    // Malformed selectors must not panic and must safely return NULL_NODE / 0
    assert(Matcher.querySelector(&doc, "[") == NULL_NODE, "unclosed [ returns NULL_NODE");
    assert(Matcher.querySelector(&doc, "]") == NULL_NODE, "stray ] returns NULL_NODE");
    assert(Matcher.querySelector(&doc, "div[") == NULL_NODE, "div[ returns NULL_NODE");
    assert(Matcher.querySelector(&doc, "div[attr=") == NULL_NODE, "div[attr= returns NULL_NODE");
    const not_unclosed = Matcher.querySelector(&doc, ":not(");
    assert(not_unclosed == NULL_NODE || not_unclosed == root, "unclosed :not( handled without crash");
    assert(Matcher.querySelector(&doc, ":nth-child(") == NULL_NODE, "unclosed :nth-child( returns NULL_NODE");
    assert(Matcher.querySelector(&doc, "#") == NULL_NODE, "bare # returns NULL_NODE");
    assert(Matcher.querySelector(&doc, ".") == NULL_NODE, "bare . returns NULL_NODE");
    const lead_comb = Matcher.querySelector(&doc, "> div");
    assert(lead_comb == NULL_NODE || lead_comb == root, "leading combinator > div handled without crash");
    const trail_comb = Matcher.querySelector(&doc, "div >");
    assert(trail_comb == NULL_NODE || trail_comb == root, "trailing combinator div > handled without crash");
    assert(Matcher.querySelectorAll(&doc, "[invalid", buf[0..8]) == 0, "malformed querySelectorAll returns 0");

    // Out-buffer truncation & zero-length out-buffer
    let one_buf: [1]NodeId = [NULL_NODE; 1];
    assert(Matcher.querySelectorAll(&doc, "*", one_buf[0..0]) == 0, "zero-len out_ids returns 0");
    assert(Matcher.querySelectorAll(&doc, "*", one_buf[0..1]) == 1, "1-len out_ids caps at 1");
    assert(one_buf[0] == root, "1-len out_ids captures first element");
}

fn testComplexSelectorsAndCaseSensitivity() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    // Mixed-case tag names
    const root = doc.createElement("DIV") catch unreachable;
    doc.setId(root, "App");
    doc.setClassName(root, "Shell Primary");

    const header = doc.createElement("HeAdEr") catch unreachable;
    doc.setId(header, "top-bar");
    doc.appendChild(root, header);

    const nav = doc.createElement("NAV") catch unreachable;
    doc.setClassName(nav, "  nav-list   active  ");
    doc.setAttribute(nav, "data-role", "main-nav");
    doc.appendChild(header, nav);

    const btn1 = doc.createElement("Button") catch unreachable;
    doc.setClassName(btn1, "btn primary");
    doc.setAttribute(btn1, "type", "submit");
    doc.appendChild(nav, btn1);

    const btn2 = doc.createElement("button") catch unreachable;
    doc.setClassName(btn2, "btn disabled");
    doc.setAttribute(btn2, "type", "button");
    doc.appendChild(nav, btn2);

    const txt = doc.createTextNode("click") catch unreachable;
    doc.appendChild(btn1, txt);

    // Case-insensitive tag matching
    assert(Matcher.querySelector(&doc, "div") == root, "lowercase div matches DIV");
    assert(Matcher.querySelector(&doc, "DIV") == root, "uppercase DIV matches DIV");
    assert(Matcher.querySelector(&doc, "dIv") == root, "mixed-case dIv matches DIV");
    assert(Matcher.querySelector(&doc, "header") == header, "header matches HeAdEr");
    assert(Matcher.querySelector(&doc, "HEADER") == header, "HEADER matches HeAdEr");

    let btns: [4]NodeId = [NULL_NODE; 4];
    const btn_cnt = Matcher.querySelectorAll(&doc, "BUTTON", btns[0..4]);
    assert(btn_cnt == 2, "BUTTON matches both Button and button");
    assert(btns[0] == btn1 && btns[1] == btn2, "BUTTON preserves document order");

    // Case-sensitive ID and class matching
    assert(Matcher.querySelector(&doc, "#App") == root, "#App matches");
    assert(Matcher.querySelector(&doc, "#app") == NULL_NODE, "#app must not match #App");
    assert(Matcher.querySelector(&doc, ".Primary") == root, ".Primary matches");
    assert(Matcher.querySelector(&doc, ".primary") == btn1, ".primary matches btn1, not root");

    // Complex compound + combinator + pseudo-class + comma list
    assert(
        Matcher.querySelector(&doc, "div#App.Shell.Primary > header#top-bar > nav.nav-list.active[data-role=\"main-nav\"] > button.btn:not(.disabled)") == btn1,
        "complex chain matches btn1"
    );
    assert(
        Matcher.querySelector(&doc, "  span.missing  , \n button.btn.disabled[type=\"button\"]  ") == btn2,
        "comma-separated selector with whitespace matches btn2"
    );
    assert(
        Matcher.querySelector(&doc, "nav:has(> button.disabled)") == nav,
        ":has(> button.disabled) matches nav"
    );
    assert(
        Matcher.querySelector(&doc, "button:empty") == btn2,
        "button:empty matches btn2 (btn1 has text node)"
    );
}

fn testScopedSubtreeAndConnectivity() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const root = doc.createElement("div") catch unreachable;
    doc.setId(root, "root");

    const sec1 = doc.createElement("section") catch unreachable;
    doc.setId(sec1, "sec1");
    const p1 = doc.createElement("p") catch unreachable;
    doc.setClassName(p1, "note");
    doc.appendChild(sec1, p1);
    doc.appendChild(root, sec1);

    const sec2 = doc.createElement("section") catch unreachable;
    doc.setId(sec2, "sec2");
    const p2 = doc.createElement("p") catch unreachable;
    doc.setClassName(p2, "note special");
    doc.appendChild(sec2, p2);
    doc.appendChild(root, sec2);

    // Scoped subtree query: sec2 should find p2, not p1; sec1 should not find .special
    assert(Matcher.querySelector(&doc, "p.note") == p1, "document-wide querySelector finds p1 first");
    assert(querySelectorInSubtree(&doc, sec2, "p.note") == p2, "scoped query in sec2 finds p2");
    assert(querySelectorInSubtree(&doc, sec1, "p.special") == NULL_NODE, "scoped query in sec1 does not find p2");

    // Disconnected element must not be returned by querySelector / querySelectorAll / getElementById
    const detached = doc.createElement("p") catch unreachable;
    doc.setId(detached, "detached-id");
    doc.setClassName(detached, "note");
    assert(doc.getElementById("detached-id") == NULL_NODE, "disconnected element not found by getElementById");

    let notes: [8]NodeId = [NULL_NODE; 8];
    const note_cnt = Matcher.querySelectorAll(&doc, "p.note", notes[0..8]);
    assert(note_cnt == 2, "querySelectorAll skips disconnected p.note");
    assert(!containsNode(notes[0..8], note_cnt, detached), "detached node not in querySelectorAll");

    // <template> element itself is connected, but its descendants are not
    const tpl = doc.createElement("template") catch unreachable;
    doc.setId(tpl, "tpl-host");
    doc.appendChild(root, tpl);
    const tpl_inner = doc.createElement("p") catch unreachable;
    doc.setId(tpl_inner, "inside-tpl");
    doc.setClassName(tpl_inner, "note");
    doc.appendChild(tpl, tpl_inner);

    assert(doc.getElementById("tpl-host") == tpl, "template element itself found by getElementById");
    assert(doc.getElementById("inside-tpl") == NULL_NODE, "template descendant hidden from getElementById");
    assert(Matcher.querySelector(&doc, "#inside-tpl") == NULL_NODE, "template descendant hidden from querySelector");
}

fn testGetElementByIdAndCollectionsEdge() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const root = doc.createElement("div") catch unreachable;
    doc.setId(root, "dup");

    const c1 = doc.createElement("span") catch unreachable;
    doc.setId(c1, "dup");
    doc.setAttribute(c1, "name", "field-a");
    doc.setClassName(c1, "tag-x tag-y");
    doc.appendChild(root, c1);

    const c2 = doc.createElement("span") catch unreachable;
    doc.setId(c2, "unique-2");
    doc.setAttribute(c2, "name", "field-a");
    doc.setClassName(c2, "tag-y");
    doc.appendChild(root, c2);

    // getElementById edge cases
    assert(doc.getElementById("") == NULL_NODE, "getElementById empty string returns NULL_NODE");
    assert(doc.getElementById("missing") == NULL_NODE, "getElementById missing returns NULL_NODE");
    assert(doc.getElementById("DUP") == NULL_NODE, "getElementById is case-sensitive");
    assert(doc.getElementById("dup") == root, "getElementById returns first matching element in document order");
    assert(doc.getElementById("unique-2") == c2, "getElementById finds unique-2");

    // Dynamic id removal & re-assignment
    assert(doc.removeAttribute(root, "id"), "removeAttribute id on root");
    assert(doc.getElementById("dup") == c1, "after root id removed, getElementById(dup) returns c1");

    // getElementsByName edge cases
    const empty_names = doc.getElementsByName("") catch unreachable;
    defer doc.allocator.free(empty_names);
    assert(empty_names.len == 0, "getElementsByName empty string returns 0");

    const found_names = doc.getElementsByName("field-a") catch unreachable;
    defer doc.allocator.free(found_names);
    assert(found_names.len == 2, "getElementsByName(field-a) returns 2");
    assert(found_names[0] == c1 && found_names[1] == c2, "getElementsByName preserves document order");

    const case_names = doc.getElementsByName("FIELD-A") catch unreachable;
    defer doc.allocator.free(case_names);
    assert(case_names.len == 0, "getElementsByName is case-sensitive");

    // ClassName & TagName collection queries via Matcher.querySelectorAll
    let cls_buf: [4]NodeId = [NULL_NODE; 4];
    assert(Matcher.querySelectorAll(&doc, ".tag-y", cls_buf[0..4]) == 2, ".tag-y matches 2");
    assert(Matcher.querySelectorAll(&doc, ".tag-x.tag-y", cls_buf[0..4]) == 1, ".tag-x.tag-y matches 1");
    assert(Matcher.querySelectorAll(&doc, ".tag", cls_buf[0..4]) == 0, "partial class .tag matches 0");
    assert(Matcher.querySelectorAll(&doc, "span", cls_buf[0..4]) == 2, "tag span matches 2");
    assert(Matcher.querySelectorAll(&doc, "*", cls_buf[0..4]) == 3, "universal * matches all 3 elements");
}

pub fn main() {
    testEmptyDocAndNullNodeBoundaries();
    testEmptyAndInvalidSelectors();
    testComplexSelectorsAndCaseSensitivity();
    testScopedSubtreeAndConnectivity();
    testGetElementByIdAndCollectionsEdge();
    std.debug.print("test_query_selector_edge: ALL PASSED\n", .{});
}
