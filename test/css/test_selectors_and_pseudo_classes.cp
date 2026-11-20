// test/css/test_selectors_and_pseudo_classes.cp
// Pure CP white-box unit tests for CSS3/4 selectors, pseudo-classes, attribute operators,
// combinators, and cascade specificity (:is, :where, :not, :has, :nth-child(An+B), etc.).

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { Matcher, SelectorList } from "../../src/css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";

fn containsNode(ids: []const NodeId, count: usize, target: NodeId): bool {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (ids[i] == target) return true;
    }
    return false;
}

fn main() {
    test("1. :nth-child(An+B) formulas, :nth-last-child, :first-child, :last-child, :only-child, :empty", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElement("div") catch unreachable;
        doc.setId(root, "root");

        const ul = doc.createElement("ul") catch unreachable;
        doc.setId(ul, "items");
        doc.appendChild(root, ul);

        // Add 6 <li> elements with a text node interspersed to ensure element-only indexing
        const ws = doc.createTextNode(" ") catch unreachable;
        doc.appendChild(ul, ws);

        let items: [6]NodeId = [NULL_NODE; 6];
        let idx: usize = 0;
        while (idx < 6) : (idx += 1) {
            items[idx] = doc.createElement("li") catch unreachable;
            doc.appendChild(ul, items[idx]);
        }

        // :first-child and :last-child
        expect_eq(Matcher.querySelector(&doc, "ul#items > li:first-child"), items[0]);
        expect_eq(Matcher.querySelector(&doc, "ul#items > li:last-child"), items[5]);

        // :nth-child(odd) & :nth-child(even)
        let buf: [8]NodeId = [NULL_NODE; 8];
        const odd_cnt = Matcher.querySelectorAll(&doc, "ul#items > li:nth-child(odd)", buf[0..8]);
        expect_eq(odd_cnt, @as(usize, 3));
        expect_eq(containsNode(buf[0..8], odd_cnt, items[0]), true);
        expect_eq(containsNode(buf[0..8], odd_cnt, items[2]), true);
        expect_eq(containsNode(buf[0..8], odd_cnt, items[4]), true);

        const even_cnt = Matcher.querySelectorAll(&doc, "ul#items > li:nth-child(even)", buf[0..8]);
        expect_eq(even_cnt, @as(usize, 3));
        expect_eq(containsNode(buf[0..8], even_cnt, items[1]), true);
        expect_eq(containsNode(buf[0..8], even_cnt, items[3]), true);
        expect_eq(containsNode(buf[0..8], even_cnt, items[5]), true);

        // :nth-child(3n+2) -> 2nd and 5th (items[1], items[4])
        const step_cnt = Matcher.querySelectorAll(&doc, "ul#items > li:nth-child(3n+2)", buf[0..8]);
        expect_eq(step_cnt, @as(usize, 2));
        expect_eq(containsNode(buf[0..8], step_cnt, items[1]), true);
        expect_eq(containsNode(buf[0..8], step_cnt, items[4]), true);

        // :nth-child(-n+3) -> first 3 items (items[0], items[1], items[2])
        const first3_cnt = Matcher.querySelectorAll(&doc, "ul#items > li:nth-child(-n+3)", buf[0..8]);
        expect_eq(first3_cnt, @as(usize, 3));
        expect_eq(containsNode(buf[0..8], first3_cnt, items[0]), true);
        expect_eq(containsNode(buf[0..8], first3_cnt, items[1]), true);
        expect_eq(containsNode(buf[0..8], first3_cnt, items[2]), true);

        // :nth-child(n+5) -> 5th and 6th (items[4], items[5])
        const tail_cnt = Matcher.querySelectorAll(&doc, "ul#items > li:nth-child(n+5)", buf[0..8]);
        expect_eq(tail_cnt, @as(usize, 2));
        expect_eq(containsNode(buf[0..8], tail_cnt, items[4]), true);
        expect_eq(containsNode(buf[0..8], tail_cnt, items[5]), true);

        // :nth-child(4) -> exact 4th (items[3])
        expect_eq(Matcher.querySelector(&doc, "ul#items > li:nth-child(4)"), items[3]);

        // :nth-last-child(-n+2) -> last 2 items (items[4], items[5])
        const last2_cnt = Matcher.querySelectorAll(&doc, "ul#items > li:nth-last-child(-n+2)", buf[0..8]);
        expect_eq(last2_cnt, @as(usize, 2));
        expect_eq(containsNode(buf[0..8], last2_cnt, items[4]), true);
        expect_eq(containsNode(buf[0..8], last2_cnt, items[5]), true);

        // :only-child (even with sibling text node, element child count is 1)
        const solo_box = doc.createElement("div") catch unreachable;
        doc.appendChild(root, solo_box);
        const solo_txt = doc.createTextNode("label:") catch unreachable;
        doc.appendChild(solo_box, solo_txt);
        const solo_span = doc.createElement("span") catch unreachable;
        doc.setId(solo_span, "solo");
        doc.appendChild(solo_box, solo_span);
        expect_eq(Matcher.querySelector(&doc, "span:only-child"), solo_span);
        expect_eq(Matcher.querySelector(&doc, "li:only-child"), NULL_NODE);

        // :empty (empty element vs element with text vs element with child element)
        const empty_div = doc.createElement("article") catch unreachable;
        doc.setId(empty_div, "empty-art");
        doc.appendChild(root, empty_div);

        const text_p = doc.createElement("p") catch unreachable;
        const p_txt = doc.createTextNode("non-empty") catch unreachable;
        doc.appendChild(text_p, p_txt);
        doc.appendChild(root, text_p);

        expect_eq(Matcher.querySelector(&doc, "article:empty"), empty_div);
        expect_eq(Matcher.querySelector(&doc, "p:empty"), NULL_NODE);
        expect_eq(Matcher.querySelector(&doc, "div#root:empty"), NULL_NODE);
    });

    test("2. Logical & relational pseudo-classes (:is, :where, :not, :has)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElement("main") catch unreachable;
        const nav = doc.createElement("nav") catch unreachable;
        doc.appendChild(root, nav);

        const b1 = doc.createElement("button") catch unreachable;
        doc.setId(b1, "b1");
        doc.setClassName(b1, "btn primary");
        doc.appendChild(nav, b1);

        const b2 = doc.createElement("button") catch unreachable;
        doc.setId(b2, "b2");
        doc.setClassName(b2, "btn disabled");
        doc.appendChild(nav, b2);

        const b3 = doc.createElement("button") catch unreachable;
        doc.setId(b3, "b3");
        doc.setClassName(b3, "btn hidden");
        doc.appendChild(nav, b3);

        const link = doc.createElement("a") catch unreachable;
        doc.setId(link, "lnk");
        doc.setClassName(link, "cta");
        doc.appendChild(nav, link);

        // :is() matching multiple selector types
        let buf: [8]NodeId = [NULL_NODE; 8];
        const is_cnt = Matcher.querySelectorAll(&doc, ":is(button.primary, a.cta)", buf[0..8]);
        expect_eq(is_cnt, @as(usize, 2));
        expect_eq(containsNode(buf[0..8], is_cnt, b1), true);
        expect_eq(containsNode(buf[0..8], is_cnt, link), true);

        // :where() matching multiple classes
        const where_cnt = Matcher.querySelectorAll(&doc, ":where(.disabled, .hidden)", buf[0..8]);
        expect_eq(where_cnt, @as(usize, 2));
        expect_eq(containsNode(buf[0..8], where_cnt, b2), true);
        expect_eq(containsNode(buf[0..8], where_cnt, b3), true);

        // :not() with selector list
        const not_cnt = Matcher.querySelectorAll(&doc, "button.btn:not(.disabled, .hidden)", buf[0..8]);
        expect_eq(not_cnt, @as(usize, 1));
        expect_eq(buf[0], b1);

        // :has() with child, descendant, next-sibling, and later-sibling combinators
        const sec = doc.createElement("section") catch unreachable;
        doc.appendChild(root, sec);

        const card1 = doc.createElement("div") catch unreachable;
        doc.setClassName(card1, "card");
        doc.appendChild(sec, card1);
        const icon1 = doc.createElement("i") catch unreachable;
        doc.setClassName(icon1, "ok");
        doc.appendChild(card1, icon1);

        const sep = doc.createElement("hr") catch unreachable;
        doc.appendChild(sec, sep);

        const card2 = doc.createElement("div") catch unreachable;
        doc.setClassName(card2, "card");
        doc.appendChild(sec, card2);
        const wrap2 = doc.createElement("div") catch unreachable;
        doc.appendChild(card2, wrap2);
        const err_badge = doc.createElement("span") catch unreachable;
        doc.setClassName(err_badge, "err");
        doc.appendChild(wrap2, err_badge);

        const tail_note = doc.createElement("aside") catch unreachable;
        doc.setClassName(tail_note, "footnote");
        doc.appendChild(sec, tail_note);

        expect_eq(Matcher.querySelector(&doc, "div.card:has(> i.ok)"), card1);
        expect_eq(Matcher.querySelector(&doc, "div.card:has(span.err)"), card2);
        expect_eq(Matcher.querySelector(&doc, "div.card:has(+ hr)"), card1);
        expect_eq(Matcher.querySelector(&doc, "div.card:has(+ aside.footnote)"), card2);

        const has_later_cnt = Matcher.querySelectorAll(&doc, "div.card:has(~ aside.footnote)", buf[0..8]);
        expect_eq(has_later_cnt, @as(usize, 2));
        expect_eq(containsNode(buf[0..8], has_later_cnt, card1), true);
        expect_eq(containsNode(buf[0..8], has_later_cnt, card2), true);

        // :not(:has(...)) combination
        expect_eq(Matcher.querySelector(&doc, "div.card:not(:has(span.err))"), card1);
    });

    test("3. Attribute selectors (=, ~=, |=, ^=, $=, *=, existence, case-insensitive, empty-val edge cases)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElement("div") catch unreachable;

        const e1 = doc.createElement("a") catch unreachable;
        doc.setAttribute(e1, "href", "https://capsurf.dev/docs/intro.html");
        doc.setAttribute(e1, "data-tags", "fast zero-gc  native");
        doc.setAttribute(e1, "lang", "en-US");
        doc.setAttribute(e1, "data-empty", "");
        doc.appendChild(root, e1);

        const e2 = doc.createElement("a") catch unreachable;
        doc.setAttribute(e2, "href", "http://example.org/GUIDE.HTML");
        doc.setAttribute(e2, "data-tags", "native browser");
        doc.setAttribute(e2, "lang", "en");
        doc.appendChild(root, e2);

        const e3 = doc.createElement("a") catch unreachable;
        doc.setAttribute(e3, "href", "/assets/logo.svg");
        doc.setAttribute(e3, "lang", "english");
        doc.appendChild(root, e3);

        // Existence [attr] including empty attribute value
        expect_eq(Matcher.querySelector(&doc, "a[data-empty]"), e1);

        // Exact [=]
        expect_eq(Matcher.querySelector(&doc, "a[href=\"/assets/logo.svg\"]"), e3);

        // Whitespace word list [~=]
        let buf: [8]NodeId = [NULL_NODE; 8];
        const ws_cnt = Matcher.querySelectorAll(&doc, "a[data-tags~=\"native\"]", buf[0..8]);
        expect_eq(ws_cnt, @as(usize, 2));
        expect_eq(containsNode(buf[0..8], ws_cnt, e1), true);
        expect_eq(containsNode(buf[0..8], ws_cnt, e2), true);
        expect_eq(Matcher.querySelector(&doc, "a[data-tags~=\"nat\"]"), NULL_NODE);
        expect_eq(Matcher.querySelector(&doc, "a[data-empty~=\"\"]"), NULL_NODE);

        // Dash-separated prefix [|=]
        const lang_cnt = Matcher.querySelectorAll(&doc, "a[lang|=\"en\"]", buf[0..8]);
        expect_eq(lang_cnt, @as(usize, 2));
        expect_eq(containsNode(buf[0..8], lang_cnt, e1), true);
        expect_eq(containsNode(buf[0..8], lang_cnt, e2), true);

        // Prefix [^=], Suffix [$=], Substring [*=]
        expect_eq(Matcher.querySelector(&doc, "a[href^=\"https://\"]"), e1);
        expect_eq(Matcher.querySelector(&doc, "a[href$=\".svg\"]"), e3);
        expect_eq(Matcher.querySelector(&doc, "a[href*=\"docs/intro\"]"), e1);

        // Case-insensitive flag `i`
        expect_eq(Matcher.querySelector(&doc, "a[href$=\".html\" i]"), e1);
        const html_ci_cnt = Matcher.querySelectorAll(&doc, "a[href$=\".html\" i]", buf[0..8]);
        expect_eq(html_ci_cnt, @as(usize, 2));
        expect_eq(Matcher.querySelector(&doc, "a[href*=\"guide\" i]"), e2);

        // Empty value with ^= / $= / *= must never match
        expect_eq(Matcher.querySelector(&doc, "a[data-empty^=\"\"]"), NULL_NODE);
        expect_eq(Matcher.querySelector(&doc, "a[data-empty$=\"\"]"), NULL_NODE);
        expect_eq(Matcher.querySelector(&doc, "a[data-empty*=\"\"]"), NULL_NODE);
    });

    test("4. Combinators (' ', '>', '+', '~') and chained compound matching", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const app = doc.createElement("section") catch unreachable;
        doc.setId(app, "app");

        const nav = doc.createElement("ul") catch unreachable;
        doc.setClassName(nav, "nav");
        doc.appendChild(app, nav);

        const li1 = doc.createElement("li") catch unreachable;
        doc.setClassName(li1, "active");
        doc.appendChild(nav, li1);

        // Text node between li1 and li2 must not break '+' adjacent sibling matching
        const gap_txt = doc.createTextNode("\n") catch unreachable;
        doc.appendChild(nav, gap_txt);

        const li2 = doc.createElement("li") catch unreachable;
        doc.setId(li2, "second");
        doc.appendChild(nav, li2);

        const li3 = doc.createElement("li") catch unreachable;
        doc.setId(li3, "third");
        doc.appendChild(nav, li3);

        const li4 = doc.createElement("li") catch unreachable;
        doc.setId(li4, "fourth");
        doc.appendChild(nav, li4);

        const a3 = doc.createElement("a") catch unreachable;
        doc.setAttribute(a3, "href", "https://capsurf.dev");
        doc.appendChild(li3, a3);

        const a4 = doc.createElement("a") catch unreachable;
        doc.setAttribute(a4, "href", "https://example.com");
        doc.appendChild(li4, a4);

        // Adjacent sibling '+'
        expect_eq(Matcher.querySelector(&doc, "li.active + li"), li2);
        expect_eq(Matcher.querySelector(&doc, "li.active + li#third"), NULL_NODE);

        // General sibling '~'
        let buf: [8]NodeId = [NULL_NODE; 8];
        const sib_cnt = Matcher.querySelectorAll(&doc, "li.active ~ li", buf[0..8]);
        expect_eq(sib_cnt, @as(usize, 3));
        expect_eq(containsNode(buf[0..8], sib_cnt, li2), true);
        expect_eq(containsNode(buf[0..8], sib_cnt, li3), true);
        expect_eq(containsNode(buf[0..8], sib_cnt, li4), true);

        // Complex chained combinators: section#app > ul.nav > li.active + li ~ li > a[href^="https"]
        const chain_cnt = Matcher.querySelectorAll(&doc, "section#app > ul.nav > li.active + li ~ li > a[href^=\"https\"]", buf[0..8]);
        expect_eq(chain_cnt, @as(usize, 2));
        expect_eq(containsNode(buf[0..8], chain_cnt, a3), true);
        expect_eq(containsNode(buf[0..8], chain_cnt, a4), true);

        // Descendant ' ' vs direct child '>'
        expect_eq(Matcher.querySelector(&doc, "section#app a[href^=\"https\"]"), a3);
        expect_eq(Matcher.querySelector(&doc, "section#app > a"), NULL_NODE);
    });

    test("5. Specificity & Cascade integration for :where(), :is(), and :not()", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElement("div") catch unreachable;
        const box = doc.createElement("p") catch unreachable;
        doc.setId(box, "hero");
        doc.setClassName(box, "lead");
        doc.appendChild(root, box);

        // Specificity unit checks
        const s_where = SelectorList.parse(":where(#hero, .lead)");
        const s_is = SelectorList.parse(":is(#hero, .lead)");
        const s_not = SelectorList.parse("p:not(.other)");
        expect_eq(s_where.selectors[0].specificityU32(), @as(u32, 0));
        expect_eq(s_is.selectors[0].specificityU32() > s_not.selectors[0].specificityU32(), true);

        // Cascade check: :where(#hero) has 0 specificity, so `p` (0,0,1) overrides it even when declared earlier.
        // Meanwhile :is(#hero, .other) has (1,0,0), overriding `.lead` (0,1,0).
        const css_src =
            "p { opacity: 0.5; z-index: 10; } " ++
            ":where(#hero) { opacity: 0.2; } " ++
            ":is(#hero, .other) { z-index: 99; } " ++
            ".lead { z-index: 20; }";

        const sheet = StyleSheet.parse(css_src, 1280.0, false);

        let styles: [4]ComputedStyle = [ComputedStyle{}; 4];
        Cascade.computeStyles(&doc, &sheet, styles[0..4], 16.0);

        const box_style = &styles[@as(usize, box)];
        expect_eq(box_style.opacity, @as(f32, 0.5));
        expect_eq(box_style.z_index, @as(i32, 99));
    });
}
