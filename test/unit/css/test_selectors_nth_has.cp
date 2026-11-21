// test/css/test_selectors_nth_has.cp
// Pure CP white-box unit tests for CSS3/4 Selectors (:nth-child, :first-child, :last-child,
// :only-child, :has, :is, :where, :not, attribute operators, and combinators) on Arena DOM.

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { Matcher, SelectorList } from "../../../src/css/matcher.cp";

fn containsNode(ids: []const NodeId, count: usize, target: NodeId): bool {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (ids[i] == target) return true;
    }
    return false;
}

fn main() {
    test("1. Structural pseudo-classes (:root, :first-child, :last-child, :only-child, :nth-child, :nth-last-child, :empty)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElement("div") catch unreachable;
        doc.setId(root, "root");

        const list = doc.createElement("ul") catch unreachable;
        doc.setId(list, "list");
        doc.appendChild(root, list);

        const li1 = doc.createElement("li") catch unreachable;
        const li2 = doc.createElement("li") catch unreachable;
        const li3 = doc.createElement("li") catch unreachable;
        const li4 = doc.createElement("li") catch unreachable;
        const li5 = doc.createElement("li") catch unreachable;
        doc.setId(li1, "li1");
        doc.setId(li2, "li2");
        doc.setId(li3, "li3");
        doc.setId(li4, "li4");
        doc.setId(li5, "li5");
        doc.appendChild(list, li1);
        doc.appendChild(list, li2);
        doc.appendChild(list, li3);
        doc.appendChild(list, li4);
        doc.appendChild(list, li5);

        const single_wrap = doc.createElement("div") catch unreachable;
        doc.setId(single_wrap, "single-wrap");
        doc.appendChild(root, single_wrap);

        const only_span = doc.createElement("span") catch unreachable;
        doc.setId(only_span, "only");
        doc.appendChild(single_wrap, only_span);

        const non_empty = doc.createElement("p") catch unreachable;
        doc.setId(non_empty, "non-empty");
        const txt = doc.createTextNode("hello") catch unreachable;
        doc.appendChild(non_empty, txt);
        doc.appendChild(root, non_empty);

        const empty_box = doc.createElement("section") catch unreachable;
        doc.setId(empty_box, "empty-box");
        doc.appendChild(root, empty_box);

        expect_eq(Matcher.querySelector(&doc, ":root"), root);
        expect_eq(Matcher.querySelector(&doc, "ul#list > li:first-child"), li1);
        expect_eq(Matcher.querySelector(&doc, "ul#list > li:last-child"), li5);
        expect_eq(Matcher.querySelector(&doc, ":only-child"), only_span);
        expect_eq(Matcher.querySelector(&doc, "ul#list > li:nth-last-child(2)"), li4);
        expect_eq(Matcher.querySelector(&doc, "section:empty"), empty_box);
        expect_eq(Matcher.querySelector(&doc, "p:empty"), NULL_NODE);

        let odd_ids: [8]NodeId = [NULL_NODE; 8];
        const odd_count = Matcher.querySelectorAll(&doc, "ul#list > li:nth-child(2n+1)", odd_ids[0..8]);
        expect_eq(odd_count, @as(usize, 3));
        expect_eq(containsNode(odd_ids[0..8], odd_count, li1), true);
        expect_eq(containsNode(odd_ids[0..8], odd_count, li3), true);
        expect_eq(containsNode(odd_ids[0..8], odd_count, li5), true);

        let even_ids: [8]NodeId = [NULL_NODE; 8];
        const even_count = Matcher.querySelectorAll(&doc, "ul#list > li:nth-child(2n)", even_ids[0..8]);
        expect_eq(even_count, @as(usize, 2));
        expect_eq(containsNode(even_ids[0..8], even_count, li2), true);
        expect_eq(containsNode(even_ids[0..8], even_count, li4), true);
    });

    test("2. Logical & relational pseudo-classes (:has, :is, :where, :not)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElement("main") catch unreachable;
        const sec = doc.createElement("section") catch unreachable;
        doc.setId(sec, "s1");
        doc.appendChild(root, sec);

        const c1 = doc.createElement("div") catch unreachable;
        doc.setId(c1, "c1");
        doc.setClassName(c1, "card");
        doc.appendChild(sec, c1);

        const badge = doc.createElement("span") catch unreachable;
        doc.setClassName(badge, "badge");
        doc.appendChild(c1, badge);

        const note = doc.createElement("p") catch unreachable;
        doc.setClassName(note, "note");
        doc.appendChild(sec, note);

        const c2 = doc.createElement("div") catch unreachable;
        doc.setId(c2, "c2");
        doc.setClassName(c2, "card");
        doc.appendChild(sec, c2);

        const inner = doc.createElement("div") catch unreachable;
        doc.appendChild(c2, inner);
        const hi = doc.createElement("em") catch unreachable;
        doc.setClassName(hi, "highlight");
        doc.appendChild(inner, hi);

        const c3 = doc.createElement("div") catch unreachable;
        doc.setId(c3, "c3");
        doc.setClassName(c3, "card");
        doc.appendChild(sec, c3);

        const summary = doc.createElement("p") catch unreachable;
        doc.setClassName(summary, "summary");
        doc.appendChild(sec, summary);

        // :has(> .badge) -> only c1
        let has_child_ids: [8]NodeId = [NULL_NODE; 8];
        const has_child_cnt = Matcher.querySelectorAll(&doc, "div.card:has(> span.badge)", has_child_ids[0..8]);
        expect_eq(has_child_cnt, @as(usize, 1));
        expect_eq(has_child_ids[0], c1);

        // :has(.highlight) -> c2 (descendant)
        expect_eq(Matcher.querySelector(&doc, "div.card:has(.highlight)"), c2);

        // :has(+ p.note) -> c1
        expect_eq(Matcher.querySelector(&doc, "div.card:has(+ p.note)"), c1);

        // :has(~ p.summary) -> c1, c2, c3
        let has_later_ids: [8]NodeId = [NULL_NODE; 8];
        const has_later_cnt = Matcher.querySelectorAll(&doc, "div.card:has(~ p.summary)", has_later_ids[0..8]);
        expect_eq(has_later_cnt, @as(usize, 3));

        // :is(...) -> badge and highlight
        let is_ids: [8]NodeId = [NULL_NODE; 8];
        const is_cnt = Matcher.querySelectorAll(&doc, ":is(span.badge, em.highlight)", is_ids[0..8]);
        expect_eq(is_cnt, @as(usize, 2));
        expect_eq(containsNode(is_ids[0..8], is_cnt, badge), true);
        expect_eq(containsNode(is_ids[0..8], is_cnt, hi), true);

        // :where(...) -> 3 cards + 1 note = 4
        let where_ids: [8]NodeId = [NULL_NODE; 8];
        const where_cnt = Matcher.querySelectorAll(&doc, ":where(.card, .note)", where_ids[0..8]);
        expect_eq(where_cnt, @as(usize, 4));

        // :not(...) -> cards without span.badge => c2, c3
        let not_ids: [8]NodeId = [NULL_NODE; 8];
        const not_cnt = Matcher.querySelectorAll(&doc, "div.card:not(:has(span.badge))", not_ids[0..8]);
        expect_eq(not_cnt, @as(usize, 2));
        expect_eq(containsNode(not_ids[0..8], not_cnt, c2), true);
        expect_eq(containsNode(not_ids[0..8], not_cnt, c3), true);

        // Specificity check: :where() has 0 specificity, :is() takes highest inner specificity
        const where_list = SelectorList.parse(":where(#c1, .card)");
        const is_list = SelectorList.parse(":is(#c1, .card)");
        expect_eq(where_list.selectors[0].specificityU32(), @as(u32, 0));
        expect_eq(is_list.selectors[0].specificityU32() > @as(u32, 0), true);
    });

    test("3. Attribute selectors (=, ~=, |=, ^=, *=, $=, existence, case-insensitive)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const root = doc.createElement("div") catch unreachable;

        const a1 = doc.createElement("a") catch unreachable;
        doc.setId(a1, "a1");
        doc.setAttribute(a1, "href", "https://example.com/docs/guide.html");
        doc.setAttribute(a1, "data-tags", "alpha beta gamma");
        doc.setAttribute(a1, "lang", "en-US");
        doc.appendChild(root, a1);

        const a2 = doc.createElement("a") catch unreachable;
        doc.setId(a2, "a2");
        doc.setAttribute(a2, "href", "http://test.org/api");
        doc.setAttribute(a2, "data-tags", "beta delta");
        doc.setAttribute(a2, "lang", "en");
        doc.appendChild(root, a2);

        const a3 = doc.createElement("a") catch unreachable;
        doc.setId(a3, "a3");
        doc.setAttribute(a3, "href", "/local/page.pdf");
        doc.setAttribute(a3, "data-role", "AdminUser");
        doc.appendChild(root, a3);

        let all_href: [8]NodeId = [NULL_NODE; 8];
        expect_eq(Matcher.querySelectorAll(&doc, "a[href]", all_href[0..8]), @as(usize, 3));

        expect_eq(Matcher.querySelector(&doc, "a[href=\"http://test.org/api\"]"), a2);
        expect_eq(Matcher.querySelector(&doc, "a[href^=\"https://\"]"), a1);
        expect_eq(Matcher.querySelector(&doc, "a[href$=\".html\"]"), a1);
        expect_eq(Matcher.querySelector(&doc, "a[href*=\"docs/guide\"]"), a1);
        expect_eq(Matcher.querySelector(&doc, "a[data-role=\"adminuser\" i]"), a3);

        let ws_ids: [8]NodeId = [NULL_NODE; 8];
        const ws_cnt = Matcher.querySelectorAll(&doc, "a[data-tags~=\"beta\"]", ws_ids[0..8]);
        expect_eq(ws_cnt, @as(usize, 2));
        expect_eq(containsNode(ws_ids[0..8], ws_cnt, a1), true);
        expect_eq(containsNode(ws_ids[0..8], ws_cnt, a2), true);
        expect_eq(Matcher.querySelector(&doc, "a[data-tags~=\"alp\"]"), NULL_NODE);

        let lang_ids: [8]NodeId = [NULL_NODE; 8];
        const lang_cnt = Matcher.querySelectorAll(&doc, "a[lang|=\"en\"]", lang_ids[0..8]);
        expect_eq(lang_cnt, @as(usize, 2));
        expect_eq(containsNode(lang_ids[0..8], lang_cnt, a1), true);
        expect_eq(containsNode(lang_ids[0..8], lang_cnt, a2), true);
    });

    test("4. Combinators (descendant ' ', child '>', next-sibling '+', later-sibling '~')", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const tree = doc.createElement("div") catch unreachable;
        doc.setId(tree, "tree");

        const hdr = doc.createElement("header") catch unreachable;
        doc.setId(hdr, "hdr");
        doc.appendChild(tree, hdr);

        const mn = doc.createElement("main") catch unreachable;
        doc.setId(mn, "mn");
        doc.appendChild(tree, mn);

        const post = doc.createElement("article") catch unreachable;
        doc.setClassName(post, "post");
        doc.appendChild(mn, post);

        const title = doc.createElement("h2") catch unreachable;
        doc.setClassName(title, "title");
        doc.appendChild(post, title);

        const p_lead = doc.createElement("p") catch unreachable;
        doc.setClassName(p_lead, "lead");
        doc.appendChild(post, p_lead);

        const p_body = doc.createElement("p") catch unreachable;
        doc.setClassName(p_body, "body");
        doc.appendChild(post, p_body);

        const meta = doc.createElement("footer") catch unreachable;
        doc.setClassName(meta, "meta");
        doc.appendChild(post, meta);

        const sb = doc.createElement("aside") catch unreachable;
        doc.setId(sb, "sb");
        doc.appendChild(tree, sb);

        // Descendant combinator
        let desc_ids: [8]NodeId = [NULL_NODE; 8];
        const desc_cnt = Matcher.querySelectorAll(&doc, "div#tree p", desc_ids[0..8]);
        expect_eq(desc_cnt, @as(usize, 2));
        expect_eq(containsNode(desc_ids[0..8], desc_cnt, p_lead), true);
        expect_eq(containsNode(desc_ids[0..8], desc_cnt, p_body), true);

        // Child combinator '>'
        expect_eq(Matcher.querySelector(&doc, "div#tree > main#mn > article.post > h2.title"), title);
        expect_eq(Matcher.querySelector(&doc, "div#tree > h2.title"), NULL_NODE);

        // Next-sibling combinator '+'
        let next_ids: [8]NodeId = [NULL_NODE; 8];
        const next_cnt = Matcher.querySelectorAll(&doc, "h2.title + p", next_ids[0..8]);
        expect_eq(next_cnt, @as(usize, 1));
        expect_eq(next_ids[0], p_lead);

        // Later-sibling combinator '~'
        let later_ids: [8]NodeId = [NULL_NODE; 8];
        const later_cnt = Matcher.querySelectorAll(&doc, "h2.title ~ p", later_ids[0..8]);
        expect_eq(later_cnt, @as(usize, 2));
        expect_eq(containsNode(later_ids[0..8], later_cnt, p_lead), true);
        expect_eq(containsNode(later_ids[0..8], later_cnt, p_body), true);

        expect_eq(Matcher.querySelector(&doc, "header#hdr ~ aside#sb"), sb);
    });
}
