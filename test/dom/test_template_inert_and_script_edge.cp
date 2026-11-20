// test/dom/test_template_inert_and_script_edge.cp
// Pure CP white-box edge-case tests for <template> inert subtree isolation
// (getElementById, querySelector, querySelectorAll, isConnectedNode, nested/uppercase
// templates, reparenting out of template) and getScriptText / isScriptElement boundaries.

import { Document, NodeId, NodeType, NULL_NODE } from "../../src/dom/dom.cp";
import { Matcher } from "../../src/css/matcher.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test_template_inert_and_script_edge assertion failed");
    }
}

fn assertEqlStr(actual: []const u8, expected: []const u8, msg: []const u8) {
    if (!std.mem.eql(u8, actual, expected)) {
        std.debug.print("Assertion failed: {s}\n  actual:   '{s}'\n  expected: '{s}'\n", .{ msg, actual, expected });
        @panic("test_template_inert_and_script_edge assertion failed");
    }
}

fn findChildByTag(doc: *const Document, parent_id: NodeId, tag: []const u8): NodeId {
    let curr = doc.getFirstChild(parent_id);
    while (curr != NULL_NODE) {
        if (n := doc.getNode(curr)) {
            if (n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, tag)) {
                return curr;
            }
            curr = n.next_sibling;
        } else {
            break;
        }
    }
    return NULL_NODE;
}

fn testTemplateInertHtmlParsing() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const html =
        "<html><head>" ++
        "<style id=\"main-style\">body { color: black; }</style>" ++
        "<script id=\"main-script\">window.mainLoaded = 1;</script>" ++
        "</head><body>" ++
        "<template id=\"tpl-main\">" ++
        "<style id=\"tpl-style\">.inert { color: red; }</style>" ++
        "<script id=\"tpl-script\">window.tplRan = 1;</script>" ++
        "<div id=\"tpl-div\" class=\"inert-box\"><span id=\"tpl-span\">Inside</span></div>" ++
        "<template id=\"tpl-nested\"><p id=\"tpl-deep\">Deep</p></template>" ++
        "</template>" ++
        "<div id=\"after-tpl\" class=\"inert-box\">After</div>" ++
        "</body></html>";

    doc.parseHtml(html) catch unreachable;

    // 1. <template> element itself IS connected and queryable
    const tpl_main = doc.getElementById("tpl-main");
    assert(tpl_main != NULL_NODE, "template#tpl-main itself is found by getElementById");
    assert(doc.isConnectedNode(tpl_main), "template#tpl-main itself is connected");
    assert(Matcher.querySelector(&doc, "template#tpl-main") == tpl_main, "querySelector finds template#tpl-main");

    // Main document elements outside <template> ARE found
    const main_style = doc.getElementById("main-style");
    const main_script = doc.getElementById("main-script");
    const after_tpl = doc.getElementById("after-tpl");
    assert(main_style != NULL_NODE, "main-style found");
    assert(main_script != NULL_NODE, "main-script found");
    assert(after_tpl != NULL_NODE, "after-tpl found");

    // 2. Descendants inside <template> MUST NOT be found by getElementById
    assert(doc.getElementById("tpl-style") == NULL_NODE, "style inside template excluded from getElementById");
    assert(doc.getElementById("tpl-script") == NULL_NODE, "script inside template excluded from getElementById");
    assert(doc.getElementById("tpl-div") == NULL_NODE, "div inside template excluded from getElementById");
    assert(doc.getElementById("tpl-span") == NULL_NODE, "span inside template excluded from getElementById");
    assert(doc.getElementById("tpl-nested") == NULL_NODE, "nested template excluded from getElementById");
    assert(doc.getElementById("tpl-deep") == NULL_NODE, "deep node in nested template excluded from getElementById");

    // 3. Descendants inside <template> MUST NOT be found by querySelector
    assert(Matcher.querySelector(&doc, "#tpl-style") == NULL_NODE, "querySelector #tpl-style is NULL_NODE");
    assert(Matcher.querySelector(&doc, "#tpl-script") == NULL_NODE, "querySelector #tpl-script is NULL_NODE");
    assert(Matcher.querySelector(&doc, "#tpl-div") == NULL_NODE, "querySelector #tpl-div is NULL_NODE");
    assert(Matcher.querySelector(&doc, "#tpl-span") == NULL_NODE, "querySelector #tpl-span is NULL_NODE");
    assert(Matcher.querySelector(&doc, "#tpl-nested") == NULL_NODE, "querySelector #tpl-nested is NULL_NODE");
    assert(Matcher.querySelector(&doc, "p") == NULL_NODE, "querySelector p inside template is NULL_NODE");
    assert(Matcher.querySelector(&doc, ".inert-box") == after_tpl, "querySelector .inert-box skips template div and returns after-tpl");

    // 4. querySelectorAll only returns main document <script>, <style>, and .inert-box
    let out_ids: [16]NodeId = [NULL_NODE; 16];
    const script_count = Matcher.querySelectorAll(&doc, "script", out_ids[0..]);
    assert(script_count == 1 && out_ids[0] == main_script, "querySelectorAll script excludes template script");

    const style_count = Matcher.querySelectorAll(&doc, "style", out_ids[0..]);
    assert(style_count == 1 && out_ids[0] == main_style, "querySelectorAll style excludes template style");

    const box_count = Matcher.querySelectorAll(&doc, ".inert-box", out_ids[0..]);
    assert(box_count == 1 && out_ids[0] == after_tpl, "querySelectorAll .inert-box excludes template div");

    const tpl_count = Matcher.querySelectorAll(&doc, "template", out_ids[0..]);
    assert(tpl_count == 1 && out_ids[0] == tpl_main, "querySelectorAll template excludes nested template");

    // 5. Direct arena traversal into <template> still finds inert <script> and extracts its text
    const inert_script = findChildByTag(&doc, tpl_main, "script");
    assert(inert_script != NULL_NODE, "inert script exists as child of template in arena");
    assert(!doc.isConnectedNode(inert_script), "inert script isConnectedNode is false");
    assert(doc.isScriptElement(inert_script), "inert script isScriptElement is true");
    const inert_js = doc.getScriptText(inert_script, std.heap.page_allocator);
    assertEqlStr(inert_js, "window.tplRan = 1;", "getScriptText extracts text from inert script");
}

fn testTemplateCaseDetachedAndReparent() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const html_el = doc.createElement("html") catch unreachable;
    const body_el = doc.createElement("body") catch unreachable;
    doc.root_element = html_el;
    doc.appendChild(html_el, body_el);

    // Uppercase TEMPLATE tag must also enforce inertness
    const upper_tpl = doc.createElement("TEMPLATE") catch unreachable;
    doc.setAttribute(upper_tpl, "id", "upper-tpl");
    doc.appendChild(body_el, upper_tpl);

    const inner_div = doc.createElement("div") catch unreachable;
    doc.setAttribute(inner_div, "id", "inside-upper");
    doc.appendChild(upper_tpl, inner_div);

    const inner_script = doc.createElement("script") catch unreachable;
    doc.setAttribute(inner_script, "id", "inside-upper-js");
    doc.appendChild(upper_tpl, inner_script);

    assert(doc.isConnectedNode(upper_tpl), "uppercase TEMPLATE itself is connected");
    assert(doc.getElementById("upper-tpl") == upper_tpl, "getElementById finds uppercase TEMPLATE");
    assert(!doc.isConnectedNode(inner_div), "child of uppercase TEMPLATE is not connected");
    assert(!doc.isConnectedNode(inner_script), "script inside uppercase TEMPLATE is not connected");
    assert(doc.getElementById("inside-upper") == NULL_NODE, "child of uppercase TEMPLATE excluded from getElementById");
    assert(Matcher.querySelector(&doc, "#inside-upper") == NULL_NODE, "child of uppercase TEMPLATE excluded from querySelector");

    // Detached node (not under root_element) is also not connected
    const detached = doc.createElement("div") catch unreachable;
    doc.setAttribute(detached, "id", "detached-div");
    assert(!doc.isConnectedNode(detached), "detached node isConnectedNode is false");
    assert(doc.getElementById("detached-div") == NULL_NODE, "detached node excluded from getElementById");
    assert(Matcher.querySelector(&doc, "#detached-div") == NULL_NODE, "detached node excluded from querySelector");

    // Reparenting inner_div out of <template> into <body> makes it connected & queryable
    doc.appendChild(body_el, inner_div);
    assert(doc.isConnectedNode(inner_div), "reparented node becomes connected");
    assert(doc.getElementById("inside-upper") == inner_div, "reparented node found by getElementById");
    assert(Matcher.querySelector(&doc, "#inside-upper") == inner_div, "reparented node found by querySelector");
}

fn testGetScriptTextAndIsScriptElementEdge() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    // 1. isScriptElement boundaries
    assert(!doc.isScriptElement(NULL_NODE), "isScriptElement(NULL_NODE) is false");
    assert(!doc.isScriptElement(@as(NodeId, 99999)), "isScriptElement(oob) is false");

    const s_lower = doc.createElement("script") catch unreachable;
    const s_upper = doc.createElement("SCRIPT") catch unreachable;
    const s_mixed = doc.createElement("ScRiPt") catch unreachable;
    const div_el = doc.createElement("div") catch unreachable;
    const txt_el = doc.createTextNode("hello") catch unreachable;
    const cmt_el = doc.createCommentNode("comment") catch unreachable;

    assert(doc.isScriptElement(s_lower), "isScriptElement lowercase script");
    assert(doc.isScriptElement(s_upper), "isScriptElement uppercase SCRIPT");
    assert(doc.isScriptElement(s_mixed), "isScriptElement mixed-case ScRiPt");
    assert(!doc.isScriptElement(div_el), "isScriptElement false for div");
    assert(!doc.isScriptElement(txt_el), "isScriptElement false for text node");
    assert(!doc.isScriptElement(cmt_el), "isScriptElement false for comment node");

    // 2. getScriptText NULL_NODE / out-of-bounds / empty script
    assertEqlStr(doc.getScriptText(NULL_NODE, std.heap.page_allocator), "", "getScriptText(NULL_NODE) is empty");
    assertEqlStr(doc.getScriptText(@as(NodeId, 99999), std.heap.page_allocator), "", "getScriptText(oob) is empty");
    assertEqlStr(doc.getScriptText(s_lower, std.heap.page_allocator), "", "getScriptText(empty script) is empty");

    // 3. Direct text_content on script node takes precedence over child text nodes
    const t_child = doc.createTextNode("child text;") catch unreachable;
    doc.appendChild(s_upper, t_child);
    doc.nodes.items[@as(usize, s_upper)].text_content = "direct text;";
    assertEqlStr(doc.getScriptText(s_upper, std.heap.page_allocator), "direct text;", "direct text_content takes precedence");

    // 4. Multiple child Text nodes interleaved with empty Text, Comment, and Element nodes
    const t1 = doc.createTextNode("const a = 1; ") catch unreachable;
    const t_empty = doc.createTextNode("") catch unreachable;
    const c_mid = doc.createCommentNode("ignored comment") catch unreachable;
    const el_mid = doc.createElement("span") catch unreachable;
    const el_mid_txt = doc.createTextNode("nested span text") catch unreachable;
    doc.appendChild(el_mid, el_mid_txt);
    const t2 = doc.createTextNode("const b = 2;") catch unreachable;

    doc.appendChild(s_mixed, t1);
    doc.appendChild(s_mixed, t_empty);
    doc.appendChild(s_mixed, c_mid);
    doc.appendChild(s_mixed, el_mid);
    doc.appendChild(s_mixed, t2);

    const combined = doc.getScriptText(s_mixed, std.heap.page_allocator);
    assertEqlStr(combined, "const a = 1; const b = 2;", "getScriptText concatenates direct Text children and ignores Comment/Element/empty Text");
}

pub fn main() {
    testTemplateInertHtmlParsing();
    testTemplateCaseDetachedAndReparent();
    testGetScriptTextAndIsScriptElementEdge();
    std.debug.print("test_template_inert_and_script_edge: ALL PASS\n", .{});
}
