// test/css/test_css_counters_and_list_style.cp
// Pure CP white-box unit tests for Phase 248-249 CSS additions:
// counter-reset, counter-increment, counter(), counters(), attr() in ::before/::after content,
// and list-style-type: upper-roman | none inheritance + LayoutTree integration.

import { test, expect_eq } from "cap:test";
import { Document, NULL_NODE } from "../../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle, ListStyleType } from "../../../src/css/cascade.cp";
import { LayoutTree } from "../../../src/layout/layout.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    return std.mem.eql(u8, a, b);
}

fn runCounterResetAndIncrement()! {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const html =
        "<html><body>" +
        "<div class=\"sec\">" +
        "<h2 id=\"h1\">Alpha</h2>" +
        "<h2 id=\"h2\">Beta</h2>" +
        "<span id=\"plain\">Alpha</span>" +
        "</div>" +
        "</body></html>";
    doc.parseHtml(html) catch unreachable;

    const css =
        "body { margin: 0; font-size: 20px; }\n" +
        ".sec { counter-reset: sec 0; }\n" +
        "h2, #plain { display: inline-block; margin: 0; font-size: 20px; }\n" +
        "h2 { counter-increment: sec 1; }\n" +
        "h2::before { content: counter(sec) \". \"; }\n";
    const sheet = StyleSheet.parse(css, 800.0, false);

    const count = doc.nodeCount();
    let styles: []ComputedStyle = std.heap.page_allocator.alloc(ComputedStyle, count) catch unreachable;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    const h1 = doc.getElementById("h1");
    const h2 = doc.getElementById("h2");
    const plain = doc.getElementById("plain");
    expect_eq(h1 != NULL_NODE, true);
    expect_eq(h2 != NULL_NODE, true);
    expect_eq(plain != NULL_NODE, true);

    expect_eq(styles[h1].has_before_content, true);
    expect_eq(strEq(styles[h1].getBeforeContent(), "1. "), true);
    expect_eq(styles[h2].has_before_content, true);
    expect_eq(strEq(styles[h2].getBeforeContent(), "2. "), true);
    expect_eq(styles[plain].has_before_content, false);

    let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
    defer layout.deinit();
    layout.computeLayout(&doc, styles) catch unreachable;

    const h1_rect = layout.getBoundingClientRect(h1);
    const plain_rect = layout.getBoundingClientRect(plain);
    expect_eq(h1_rect.width > plain_rect.width, true);
    expect_eq(h1_rect.height > 0.0, true);
}

fn runNestedCounters()! {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const html =
        "<html><body>" +
        "<div class=\"list\">" +
        "  <div id=\"o1\" class=\"item\">Outer1" +
        "    <div class=\"list\">" +
        "      <div id=\"i1\" class=\"item\">Inner1</div>" +
        "      <div id=\"i2\" class=\"item\">Inner2</div>" +
        "    </div>" +
        "  </div>" +
        "  <div id=\"o2\" class=\"item\">Outer2</div>" +
        "</div>" +
        "</body></html>";
    doc.parseHtml(html) catch unreachable;

    const css =
        ".list { counter-reset: item 0; }\n" +
        ".item { counter-increment: item 1; }\n" +
        ".item::before { content: counters(item, \".\") \" \"; }\n";
    const sheet = StyleSheet.parse(css, 800.0, false);

    const count = doc.nodeCount();
    let styles: []ComputedStyle = std.heap.page_allocator.alloc(ComputedStyle, count) catch unreachable;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    const o1 = doc.getElementById("o1");
    const inner1 = doc.getElementById("i1");
    const inner2 = doc.getElementById("i2");
    const o2 = doc.getElementById("o2");
    expect_eq(o1 != NULL_NODE, true);
    expect_eq(inner1 != NULL_NODE, true);
    expect_eq(inner2 != NULL_NODE, true);
    expect_eq(o2 != NULL_NODE, true);

    expect_eq(strEq(styles[o1].getBeforeContent(), "1 "), true);
    expect_eq(strEq(styles[inner1].getBeforeContent(), "1.1 "), true);
    expect_eq(strEq(styles[inner2].getBeforeContent(), "1.2 "), true);
    expect_eq(strEq(styles[o2].getBeforeContent(), "2 "), true);
}

fn runAttrContentAndLayout()! {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const html =
        "<html><body>" +
        "<span id=\"badge\" data-count=\"42\"></span>" +
        "<span id=\"empty\"></span>" +
        "</body></html>";
    doc.parseHtml(html) catch unreachable;

    const css =
        "span { display: inline-block; font-size: 16px; }\n" +
        "#badge::after { content: \"[\" attr(data-count) \"]\"; }\n" +
        "#empty::after { content: attr(data-missing); }\n";
    const sheet = StyleSheet.parse(css, 800.0, false);

    const count = doc.nodeCount();
    let styles: []ComputedStyle = std.heap.page_allocator.alloc(ComputedStyle, count) catch unreachable;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    const badge = doc.getElementById("badge");
    const empty = doc.getElementById("empty");
    expect_eq(badge != NULL_NODE, true);
    expect_eq(empty != NULL_NODE, true);

    expect_eq(styles[badge].has_after_content, true);
    expect_eq(strEq(styles[badge].getAfterContent(), "[42]"), true);
    expect_eq(strEq(styles[empty].getAfterContent(), ""), true);

    let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
    defer layout.deinit();
    layout.computeLayout(&doc, styles) catch unreachable;

    const badge_rect = layout.getBoundingClientRect(badge);
    expect_eq(badge_rect.width > 0.0, true);
    expect_eq(badge_rect.height > 0.0, true);
}

fn runListStyleType()! {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const html =
        "<html><body>" +
        "<ol id=\"ol_roman\"><li id=\"li_roman\">First</li></ol>" +
        "<ol id=\"ol_none\"><li id=\"li_none\">Hidden</li></ol>" +
        "</body></html>";
    doc.parseHtml(html) catch unreachable;

    const css =
        "#ol_roman { list-style-type: upper-roman; }\n" +
        "#ol_none { list-style-type: none; }\n";
    const sheet = StyleSheet.parse(css, 800.0, false);

    const count = doc.nodeCount();
    let styles: []ComputedStyle = std.heap.page_allocator.alloc(ComputedStyle, count) catch unreachable;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    const ol_roman = doc.getElementById("ol_roman");
    const li_roman = doc.getElementById("li_roman");
    const ol_none = doc.getElementById("ol_none");
    const li_none = doc.getElementById("li_none");
    expect_eq(ol_roman != NULL_NODE, true);
    expect_eq(li_roman != NULL_NODE, true);
    expect_eq(ol_none != NULL_NODE, true);
    expect_eq(li_none != NULL_NODE, true);

    expect_eq(styles[ol_roman].list_style_type == ListStyleType.UpperRoman, true);
    expect_eq(styles[li_roman].list_style_type == ListStyleType.UpperRoman, true);
    expect_eq(styles[ol_none].list_style_type == ListStyleType.None, true);
    expect_eq(styles[li_none].list_style_type == ListStyleType.None, true);

    let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
    defer layout.deinit();
    layout.computeLayout(&doc, styles) catch unreachable;

    const r1 = layout.getBoundingClientRect(li_roman);
    const r2 = layout.getBoundingClientRect(li_none);
    expect_eq(r1.height > 0.0, true);
    expect_eq(r2.height > 0.0, true);
    expect_eq(r2.y > r1.y, true);
}

fn main() {
    test("1. counter-reset, counter-increment, counter() & LayoutTree", () => {
        runCounterResetAndIncrement()?;
    });

    test("2. nested counters(item, \".\") scoping", () => {
        runNestedCounters()?;
    });

    test("3. attr() in ::after content & LayoutTree", () => {
        runAttrContentAndLayout()?;
    });

    test("4. list-style-type: upper-roman | none inheritance & LayoutTree", () => {
        runListStyleType()?;
    });
}
