// tests/test_css_layout.cp
// CapSurf Milestone 1 End-to-End Test Suite:
// Arena DOM -> Servo Selector Engine -> 64-bit Cascade -> Flexbox Geometry Layout
// Validates querySelectorAll and getBoundingClientRect screen coordinates for AI Agents.

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { Matcher } from "../../../src/css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../../src/css/cascade.cp";
import { LayoutTree, Rect } from "../../../src/layout/layout.cp";
import { CssColor } from "cap:css";

fn main() {
    test("1. CapSurf Arena DOM & W3C Selectors Level 4 Matching (querySelectorAll)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        // Build DOM tree:
        // <div id="app" class="container">
        //   <header id="navbar" class="nav flex-row">
        //     <div id="logo" class="brand">Logo</div>
        //     <button id="submit-btn" class="btn primary" data-action="submit">Submit</button>
        //     <div id="secret" class="hidden">Secret</div>
        //   </header>
        //   <ul id="menu" class="list">
        //     <li class="item active" data-role="tab">Tab 1</li>
        //     <li class="item" data-role="tab">Tab 2</li>
        //     <li class="item disabled" data-role="tab">Tab 3</li>
        //   </ul>
        // </div>

        const app = doc.createElementWithAttrs("div", "app", "container", "") catch unreachable;
        const navbar = doc.createElementWithAttrs("header", "navbar", "nav flex-row", "") catch unreachable;
        const logo = doc.createElementWithAttrs("div", "logo", "brand", "") catch unreachable;
        const submit_btn = doc.createElementWithAttrs("button", "submit-btn", "btn primary", "font-size: 18px;") catch unreachable;
        doc.setAttribute(submit_btn, "data-action", "submit");
        const secret = doc.createElementWithAttrs("div", "secret", "hidden", "") catch unreachable;

        doc.appendChild(app, navbar);
        doc.appendChild(navbar, logo);
        doc.appendChild(navbar, submit_btn);
        doc.appendChild(navbar, secret);

        const menu = doc.createElementWithAttrs("ul", "menu", "list", "") catch unreachable;
        const tab1 = doc.createElementWithAttrs("li", "", "item active", "") catch unreachable;
        doc.setAttribute(tab1, "data-role", "tab");
        const tab2 = doc.createElementWithAttrs("li", "", "item", "") catch unreachable;
        doc.setAttribute(tab2, "data-role", "tab");
        const tab3 = doc.createElementWithAttrs("li", "", "item disabled", "") catch unreachable;
        doc.setAttribute(tab3, "data-role", "tab");

        doc.appendChild(app, menu);
        doc.appendChild(menu, tab1);
        doc.appendChild(menu, tab2);
        doc.appendChild(menu, tab3);

        // Test 1: ID and class selectors
        const q_btn = Matcher.querySelector(&doc, "#submit-btn");
        expect_eq(q_btn, submit_btn);

        const q_logo = Matcher.querySelector(&doc, ".brand");
        expect_eq(q_logo, logo);

        // Test 2: Child and descendant combinators
        const q_comb = Matcher.querySelector(&doc, "div#app.container > header#navbar button.btn");
        expect_eq(q_comb, submit_btn);

        // Test 3: Structural pseudo-classes (:first-child, :nth-child)
        const q_first = Matcher.querySelector(&doc, "ul#menu li:first-child");
        expect_eq(q_first, tab1);

        const q_second = Matcher.querySelector(&doc, "ul#menu li:nth-child(2)");
        expect_eq(q_second, tab2);

        const q_third = Matcher.querySelector(&doc, "ul#menu li:nth-child(2n+1)");
        expect_eq(q_third, tab1); // 2(0)+1 = 1st child

        // Test 4: Attribute selectors
        const q_attr = Matcher.querySelector(&doc, "button[data-action='submit']");
        expect_eq(q_attr, submit_btn);

        // Test 5: Logical pseudo-class (:not)
        let not_disabled: [8]NodeId = [0; 8];
        const not_cnt = Matcher.querySelectorAll(&doc, "ul#menu li.item:not(.disabled)", not_disabled[0..8]);
        expect_eq(not_cnt, 2);
        expect_eq(not_disabled[0], tab1);
        expect_eq(not_disabled[1], tab2);
    });

    test("2. CapSurf 64-Bit Bit-Packed Weight Cascade (!important & Inline Style Resolution)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const app = doc.createElementWithAttrs("div", "app", "container", "") catch unreachable;
        const navbar = doc.createElementWithAttrs("header", "navbar", "nav", "") catch unreachable;
        const logo = doc.createElementWithAttrs("div", "logo", "brand", "") catch unreachable;
        const submit_btn = doc.createElementWithAttrs("button", "submit-btn", "btn primary", "font-size: 18px;") catch unreachable;
        const secret = doc.createElementWithAttrs("div", "secret", "hidden", "") catch unreachable;

        doc.appendChild(app, navbar);
        doc.appendChild(navbar, logo);
        doc.appendChild(navbar, submit_btn);
        doc.appendChild(navbar, secret);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            ".btn { width: 100px; height: 40px; background-color: #cccccc; }\n" ++
            "#submit-btn { width: 120px; height: 44px; background-color: #0077cc; }\n" ++
            ".primary { background-color: #ff0000 !important; }\n" ++
            ".hidden { display: none; }\n" ++
            "#navbar { display: flex; flex-direction: row; padding: 20px; gap: 15px; width: 1280px; height: 80px; }\n" ++
            "#logo { width: 190px; height: 40px; }\n";

        const sheet = StyleSheet.parse(css_text, 1280.0, false);
        let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        const btn_style = &styles[@as(usize, submit_btn)];
        // ID selector specificity (1, 0, 0) beats class selector (0, 1, 0) for width & height
        expect_eq(btn_style.width.resolvePx(1280.0, 18.0, 16.0, 1280.0, 800.0), 120.0);
        expect_eq(btn_style.height.resolvePx(800.0, 18.0, 16.0, 1280.0, 800.0), 44.0);

        // .primary has !important (bit 63), so it overrides #submit-btn's non-important background-color!
        const red = CssColor.parse("#ff0000").?;
        expect_eq(btn_style.background_color.r, red.r);
        expect_eq(btn_style.background_color.g, red.g);
        expect_eq(btn_style.background_color.b, red.b);

        // Inline style style="font-size: 18px;" has is_inline=true (bit 62)
        expect_eq(btn_style.font_size_px, 18.0);

        // #secret has display: none
        const secret_style = &styles[@as(usize, secret)];
        expect_eq(secret_style.display.isNone(), true);
    });

    test("3. CapSurf Box Model & Flexbox Geometry Layout (getBoundingClientRect for AI Agents)", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const app = doc.createElementWithAttrs("div", "app", "container", "") catch unreachable;
        const navbar = doc.createElementWithAttrs("header", "navbar", "nav", "") catch unreachable;
        const logo = doc.createElementWithAttrs("div", "logo", "brand", "") catch unreachable;
        const submit_btn = doc.createElementWithAttrs("button", "submit-btn", "btn primary", "") catch unreachable;
        const secret = doc.createElementWithAttrs("div", "secret", "hidden", "") catch unreachable;

        doc.appendChild(app, navbar);
        doc.appendChild(navbar, logo);
        doc.appendChild(navbar, submit_btn);
        doc.appendChild(navbar, secret);

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#app { width: 1280px; }\n" ++
            "#navbar { display: flex; flex-direction: row; padding: 20px; gap: 15px; width: 1280px; height: 80px; }\n" ++
            "#logo { width: 190px; height: 40px; }\n" ++
            "#submit-btn { width: 120px; height: 44px; }\n" ++
            ".hidden { display: none; }\n";

        const sheet = StyleSheet.parse(css_text, 1280.0, false);
        let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 1280.0, 800.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        // Verify #logo coordinates:
        // Navbar padding is 20px, so #logo starts at x: 20, y: 20, with width: 190, height: 40
        const logo_rect = layout.getBoundingClientRect(logo);
        expect_eq(logo_rect.x, 20.0);
        expect_eq(logo_rect.y, 20.0);
        expect_eq(logo_rect.width, 190.0);
        expect_eq(logo_rect.height, 40.0);
        expect_eq(logo_rect.visible, true);
        expect_eq(logo_rect.isClickable(), true);

        // Verify #submit-btn coordinates:
        // Navbar padding 20px + logo width 190px + gap 15px = 225px!
        // Y coordinate is navbar padding 20px
        // Width: 120px, Height: 44px
        const btn_rect = layout.getBoundingClientRect(submit_btn);
        expect_eq(btn_rect.x, 225.0);
        expect_eq(btn_rect.y, 20.0);
        expect_eq(btn_rect.width, 120.0);
        expect_eq(btn_rect.height, 44.0);
        expect_eq(btn_rect.visible, true);
        expect_eq(btn_rect.isClickable(), true);

        // Verify #secret with display: none:
        // Geometry is zeroed, visible is false, not clickable!
        const secret_rect = layout.getBoundingClientRect(secret);
        expect_eq(secret_rect.width, 0.0);
        expect_eq(secret_rect.height, 0.0);
        expect_eq(secret_rect.visible, false);
        expect_eq(secret_rect.isClickable(), false);
    });
}
