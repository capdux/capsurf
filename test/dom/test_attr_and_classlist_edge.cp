// test/dom/test_attr_and_classlist_edge.cp
// Pure CP white-box edge-case tests for DOM attributes (case-insensitivity,
// whitespace/special values, duplicate overwrite & deletion, NS attributes),
// ID index / getElementById sync, DOMTokenList (classList), and NULL_NODE boundaries.

import { Document, NodeId, NULL_NODE, MAX_ATTRS_PER_NODE, MAX_CLASSES_PER_NODE } from "../../src/dom/dom.cp";
import { DOMTokenList, MAX_TOKENS } from "../../src/dom/collections.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test_attr_and_classlist_edge assertion failed");
    }
}

fn assertOptEq(opt: ([]const u8)?, expected: []const u8, msg: []const u8) {
    if (v := opt) {
        assert(std.mem.eql(u8, v, expected), msg);
    } else {
        assert(false, msg);
    }
}

fn testAttributeCaseAndOverwrite() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const root = doc.createElement("div") catch unreachable;
    const el = doc.createElement("span") catch unreachable;
    doc.appendChild(root, el);

    // 1. Case-insensitive attribute get/has/set/remove
    doc.setAttribute(el, "DATA-Custom", "val1");
    assert(doc.getNode(el).?.hasAttribute("data-custom"), "hasAttribute lowercase matches");
    assert(doc.getNode(el).?.hasAttribute("DATA-CUSTOM"), "hasAttribute uppercase matches");
    assertOptEq(doc.getAttribute(el, "dAtA-cUsToM"), "val1", "getAttribute mixed case matches");

    // Overwrite with different case must update in-place, not duplicate
    doc.setAttribute(el, "data-custom", "val2");
    assertOptEq(doc.getAttribute(el, "DATA-Custom"), "val2", "overwritten value visible via original case");

    const names1 = doc.getAttributeNames(el) catch unreachable;
    defer doc.allocator.free(names1);
    assert(names1.len == 1, "overwrite does not create duplicate attribute");

    // 2. Special attributes (id, class, style) case-insensitivity
    doc.setAttribute(el, "ID", "my-span");
    doc.setAttribute(el, "ClAsS", "btn primary");
    doc.setAttribute(el, "STYLE", "display: none; color: red");

    assertOptEq(doc.getAttribute(el, "id"), "my-span", "ID synced to id_attr");
    assertOptEq(doc.getAttribute(el, "class"), "btn primary", "ClAsS synced to class_attr");
    assertOptEq(doc.getAttribute(el, "style"), "display: none; color: red", "STYLE synced to inline_style");
    assert(doc.getElementById("my-span") == el, "getElementById finds node set with uppercase ID");
    assert(doc.getNode(el).?.hasClass("btn"), "hasClass btn after uppercase ClAsS");
    assert(doc.getNode(el).?.hasClass("primary"), "hasClass primary after uppercase ClAsS");
    assert(doc.isNodeDisplayNone(el), "STYLE display: none synced to is_display_none");

    // 3. Remove attributes with different casing
    assert(doc.removeAttribute(el, "data-CUSTOM"), "removeAttribute returns true when present");
    assert(!doc.removeAttribute(el, "data-custom"), "removeAttribute returns false on second removal");
    assert(doc.getAttribute(el, "DATA-Custom") == null, "removed attribute returns null");

    assert(doc.removeAttribute(el, "style"), "removeAttribute style returns true");
    assert(!doc.isNodeDisplayNone(el), "removing style clears is_display_none");
    assert(doc.getAttribute(el, "STYLE") == null, "removed style returns null");

    assert(doc.removeAttribute(el, "CLASS"), "removeAttribute CLASS returns true");
    assert(!doc.getNode(el).?.hasClass("btn"), "removing CLASS clears classes");
    assert(doc.getAttribute(el, "class") == null, "removed class returns null");
}

fn testWhitespaceSpecialValuesAndLimits() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const root = doc.createElement("div") catch unreachable;
    const el = doc.createElement("input") catch unreachable;
    doc.appendChild(root, el);

    // 1. Empty string value on boolean/regular attribute
    doc.setAttribute(el, "disabled", "");
    assert(doc.getNode(el).?.hasAttribute("disabled"), "empty-value attribute still exists");
    assertOptEq(doc.getAttribute(el, "disabled"), "", "empty-value attribute returns empty slice");

    // 2. Whitespace, quotes, and special characters in value
    const special = "  hello\t\n\"world\" & <tag> = 100%  ";
    doc.setAttribute(el, "title", special);
    assertOptEq(doc.getAttribute(el, "title"), special, "special characters and whitespace preserved verbatim");

    // 3. Class whitespace tokenization edge cases
    doc.setAttribute(el, "class", "  alpha \t beta\n\r gamma  alpha  ");
    const n = doc.getNode(el).?;
    assert(n.hasClass("alpha"), "hasClass alpha");
    assert(n.hasClass("beta"), "hasClass beta");
    assert(n.hasClass("gamma"), "hasClass gamma");
    assert(!n.hasClass(""), "hasClass empty string is false");
    assert(!n.hasClass(" alpha"), "hasClass with leading space is false");
    assert(!n.hasClass("alph"), "hasClass partial prefix is false");
    // alpha appears twice in raw string, so NodeData.classes stores up to MAX_CLASSES_PER_NODE tokens
    assert(n.class_count == 4, "NodeData tokenizes 4 whitespace-separated class tokens");

    // Whitespace-only class string
    doc.setClassName(el, "   \t\r\n  ");
    assert(doc.getNode(el).?.class_count == 0, "whitespace-only class yields 0 classes");
    assert(!doc.getNode(el).?.hasClass("alpha"), "previous classes cleared");

    // 4. Middle attribute deletion shifts remaining attributes cleanly
    const el2 = doc.createElement("a") catch unreachable;
    doc.appendChild(root, el2);
    doc.setAttribute(el2, "a1", "v1");
    doc.setAttribute(el2, "a2", "v2");
    doc.setAttribute(el2, "a3", "v3");
    assert(doc.removeAttribute(el2, "a2"), "remove middle attribute a2");
    assertOptEq(doc.getAttribute(el2, "a1"), "v1", "a1 intact after removing a2");
    assert(doc.getAttribute(el2, "a2") == null, "a2 gone");
    assertOptEq(doc.getAttribute(el2, "a3"), "v3", "a3 shifted intact after removing a2");

    // 5. MAX_ATTRS_PER_NODE capacity boundary (no overflow/panic)
    const el3 = doc.createElement("p") catch unreachable;
    doc.appendChild(root, el3);
    const attr_names: [18][]const u8 = [
        "k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7",
        "k8", "k9", "k10", "k11", "k12", "k13", "k14", "k15",
        "k16", "k17",
    ];
    let idx: usize = 0;
    while (idx < 18) : (idx += 1) {
        doc.setAttribute(el3, attr_names[idx], "x");
    }
    assert(doc.getNode(el3).?.attr_count == MAX_ATTRS_PER_NODE, "attr_count capped at MAX_ATTRS_PER_NODE");
    assert(doc.getAttribute(el3, "k15") != null, "16th attr stored");
    assert(doc.getAttribute(el3, "k16") == null, "17th attr ignored safely");

    // 6. MAX_CLASSES_PER_NODE capacity boundary
    doc.setClassName(el3, "c0 c1 c2 c3 c4 c5 c6 c7 c8 c9 c10 c11 c12 c13 c14 c15 c16 c17");
    assert(doc.getNode(el3).?.class_count == MAX_CLASSES_PER_NODE, "class_count capped at MAX_CLASSES_PER_NODE");
    assert(doc.getNode(el3).?.hasClass("c15"), "16th class stored");
    assert(!doc.getNode(el3).?.hasClass("c16"), "17th class ignored safely");
}

fn testIdIndexSyncAndNamespaceAttrs() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const root = doc.createElement("html") catch unreachable;
    const c1 = doc.createElement("div") catch unreachable;
    const c2 = doc.createElement("div") catch unreachable;
    const orphan = doc.createElement("div") catch unreachable;

    doc.appendChild(root, c1);
    doc.appendChild(root, c2);

    // 1. Disconnected node with id is NOT returned by getElementById
    doc.setId(orphan, "only-orphan");
    assert(doc.getElementById("only-orphan") == NULL_NODE, "disconnected node not returned by getElementById");
    doc.appendChild(root, orphan);
    assert(doc.getElementById("only-orphan") == orphan, "newly connected node found by getElementById");

    // 2. Overwriting id updates getElementById lookup immediately
    doc.setAttribute(c1, "id", "first-id");
    assert(doc.getElementById("first-id") == c1, "first-id resolves to c1");
    doc.setAttribute(c1, "id", "second-id");
    assert(doc.getElementById("first-id") == NULL_NODE, "old first-id no longer resolves");
    assert(doc.getElementById("second-id") == c1, "new second-id resolves to c1");

    // 3. Duplicate IDs: first in document node order wins; removing first exposes second
    doc.setAttribute(c2, "id", "shared-id");
    doc.setAttribute(c1, "id", "shared-id");
    assert(doc.getElementById("shared-id") == c1, "duplicate id returns earlier node c1");
    assert(doc.removeAttribute(c1, "id"), "remove id from c1");
    assert(doc.getElementById("shared-id") == c2, "after removing c1 id, getElementById finds c2");

    // 4. Empty id value does not match getElementById("")
    doc.setAttribute(c2, "id", "");
    assert(doc.getElementById("shared-id") == NULL_NODE, "empty id clears shared-id");
    assert(doc.getElementById("") == NULL_NODE, "getElementById empty string returns NULL_NODE");

    // 5. Namespace attribute set/get/remove and id sync
    doc.setAttributeNS(c1, "http://www.w3.org/1999/xlink", "xlink:href", "#icon");
    assertOptEq(doc.getAttributeNS(c1, "http://www.w3.org/1999/xlink", "href"), "#icon", "xlink:href found by NS");
    assert(doc.getAttributeNS(c1, "", "href") == null, "xlink:href not matched under empty NS");
    assert(doc.removeAttributeNS(c1, "http://www.w3.org/1999/xlink", "href"), "removeAttributeNS xlink:href");
    assert(doc.getAttributeNS(c1, "http://www.w3.org/1999/xlink", "href") == null, "xlink:href removed");

    doc.setAttributeNS(c1, "", "id", "ns-synced-id");
    assert(doc.getElementById("ns-synced-id") == c1, "setAttributeNS empty-ns id syncs id_attr");
    assert(doc.removeAttributeNS(c1, "", "id"), "removeAttributeNS empty-ns id returns true");
    assert(doc.getElementById("ns-synced-id") == NULL_NODE, "removeAttributeNS clears id_attr");
}

fn testDOMTokenListEdge() {
    let list = DOMTokenList.init();

    // 1. Empty & whitespace-only strings
    list.fromString("");
    assert(list.length() == 0, "fromString empty -> length 0");
    assert(list.item(0) == null, "item(0) on empty -> null");

    list.fromString("   \t\r\n  ");
    assert(list.length() == 0, "fromString whitespace-only -> length 0");

    // 2. Deduplication on fromString
    list.fromString("  foo  bar \t foo \n baz  bar ");
    assert(list.length() == 3, "fromString deduplicates tokens");
    assertOptEq(list.item(0), "foo", "token 0 is foo");
    assertOptEq(list.item(1), "bar", "token 1 is bar");
    assertOptEq(list.item(2), "baz", "token 2 is baz");
    assert(list.item(3) == null, "out-of-bounds item(3) is null");

    // 3. add / remove / toggle / replaceToken
    assert(!list.add(""), "add empty string returns false");
    assert(!list.add("foo"), "add duplicate returns false");
    assert(list.add("qux"), "add new token returns true");
    assert(list.length() == 4, "length is 4 after add");

    assert(!list.remove("missing"), "remove missing returns false");
    assert(list.remove("bar"), "remove bar returns true");
    assert(list.length() == 3, "length is 3 after remove");
    assert(!list.contains("bar"), "bar removed");
    assertOptEq(list.item(1), "baz", "baz shifted to index 1");

    assert(list.toggle("bar"), "toggle absent bar adds it and returns true");
    assert(list.contains("bar"), "bar present after toggle");
    assert(!list.toggle("bar"), "toggle present bar removes it and returns false");
    assert(!list.contains("bar"), "bar absent after second toggle");

    assert(!list.replaceToken("nonexistent", "x"), "replaceToken missing returns false");
    assert(list.replaceToken("baz", "BAZ"), "replaceToken baz -> BAZ returns true");
    assertOptEq(list.item(1), "BAZ", "token 1 replaced with BAZ");

    // 4. serialize normal and truncated buffer
    let buf: [64]u8 = [0; 64];
    const s1 = list.serialize(buf[0..64]);
    assert(std.mem.eql(u8, s1, "foo BAZ qux"), "serialize produces space-joined tokens");

    let small_buf: [5]u8 = [0; 5];
    const s2 = list.serialize(small_buf[0..5]);
    assert(std.mem.eql(u8, s2, "foo "), "serialize stops safely before overflowing small buffer");

    // 5. MAX_TOKENS capacity boundary
    let full = DOMTokenList.init();
    const tok_names: [34][]const u8 = [
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "t8", "t9", "t10", "t11", "t12", "t13", "t14", "t15",
        "t16", "t17", "t18", "t19", "t20", "t21", "t22", "t23",
        "t24", "t25", "t26", "t27", "t28", "t29", "t30", "t31",
        "t32", "t33",
    ];
    let i: usize = 0;
    while (i < 34) : (i += 1) {
        const added = full.add(tok_names[i]);
        if (i < MAX_TOKENS) {
            assert(added, "add within MAX_TOKENS succeeds");
        } else {
            assert(!added, "add beyond MAX_TOKENS returns false");
        }
    }
    assert(full.length() == MAX_TOKENS, "length capped at MAX_TOKENS");
}

fn testNullNodeAndOutOfBoundsBoundaries() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    // NULL_NODE (0) boundaries
    assert(doc.getAttribute(NULL_NODE, "id") == null, "getAttribute(NULL_NODE) is null");
    assert(doc.getAttributeNS(NULL_NODE, "", "id") == null, "getAttributeNS(NULL_NODE) is null");
    doc.setAttribute(NULL_NODE, "id", "x");
    doc.setAttributeNS(NULL_NODE, "", "class", "y");
    doc.setClassName(NULL_NODE, "z");
    doc.setId(NULL_NODE, "w");
    assert(!doc.removeAttribute(NULL_NODE, "id"), "removeAttribute(NULL_NODE) is false");
    assert(!doc.removeAttributeNS(NULL_NODE, "", "id"), "removeAttributeNS(NULL_NODE) is false");

    // Out-of-bounds NodeId boundaries
    const oob: NodeId = @as(NodeId, 99999);
    assert(doc.getAttribute(oob, "id") == null, "getAttribute(oob) is null");
    assert(doc.getAttributeNS(oob, "", "id") == null, "getAttributeNS(oob) is null");
    doc.setAttribute(oob, "id", "x");
    doc.setAttributeNS(oob, "", "class", "y");
    doc.setClassName(oob, "z");
    doc.setId(oob, "w");
    assert(!doc.removeAttribute(oob, "id"), "removeAttribute(oob) is false");
    assert(!doc.removeAttributeNS(oob, "", "id"), "removeAttributeNS(oob) is false");

    assert(doc.getElementById("x") == NULL_NODE, "oob/NULL_NODE sets did not pollute document");
}

pub fn main() {
    testAttributeCaseAndOverwrite();
    testWhitespaceSpecialValuesAndLimits();
    testIdIndexSyncAndNamespaceAttrs();
    testDOMTokenListEdge();
    testNullNodeAndOutOfBoundsBoundaries();
    std.debug.print("test_attr_and_classlist_edge: ALL PASS\n", .{});
}
