// tests/test_js_dom.cp
// CapSurf Milestone 3 Test Suite:
// QuickJS-NG Engine Binding, DOM API Mounting, getBoundingClientRect & Preemption

import { test, expect_eq } from "cap:test";
import { Doc, Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";
import { JsRuntime, JsContext, jsToSlice, jsIsException } from "../../src/js/qjs_engine.cp";
import { mountDom } from "../../src/js/dom_binding.cp";
import { LayoutTree } from "../../src/layout/layout.cp";

fn main() {
    test("1. QuickJS Engine Execution & Expression Evaluation", () => {
        let rt = JsRuntime.init(1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        // 1. Math evaluation
        const math_val = ctx.eval("10 + 20 * 3");
        defer ctx.freeValue(math_val);
        const math_str = ctx.toCString(math_val);
        defer std.heap.page_allocator.free(math_str);
        expect_eq(std.mem.eql(u8, math_str, "70"), true);

        // 2. String evaluation
        const str_val = ctx.eval("'Cap' + 'Surf ' + (2026)");
        defer ctx.freeValue(str_val);
        const res_str = ctx.toCString(str_val);
        defer std.heap.page_allocator.free(res_str);
        expect_eq(std.mem.eql(u8, res_str, "CapSurf 2026"), true);

        // 3. Object & Array manipulation
        const arr_val = ctx.eval("[1, 2, 3].map(x => x * 2).join(',')");
        defer ctx.freeValue(arr_val);
        const arr_str = ctx.toCString(arr_val);
        defer std.heap.page_allocator.free(arr_str);
        expect_eq(std.mem.eql(u8, arr_str, "2,4,6"), true);
    });

    test("2. DOM Queries via JavaScript (querySelector, getElementById, properties)", () => {
        let rt = JsRuntime.init(2 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html>\n" ++
            "<html>\n" ++
            "<head><title>CapSurf Engine</title></head>\n" ++
            "<body>\n" ++
            "  <div id=\"main\" class=\"container flex\">\n" ++
            "    <h1 id=\"title\">CapSurf Headless</h1>\n" ++
            "    <p class=\"item\">Agent Browser</p>\n" ++
            "    <button id=\"submit-btn\" class=\"btn active\">Run Agent</button>\n" ++
            "  </div>\n" ++
            "</body>\n" ++
            "</html>\n";

        doc.parseHtml(html) catch unreachable;

        * = mountDom(&ctx, &doc, null) catch unreachable;

        // Query by ID and read attributes & properties
        const id_val = ctx.eval("document.getElementById('title').textContent");
        defer ctx.freeValue(id_val);
        const title_str = ctx.toCString(id_val);
        defer std.heap.page_allocator.free(title_str);
        expect_eq(std.mem.eql(u8, title_str, "CapSurf Headless"), true);

        // Query button tag name and class attribute
        const tag_val = ctx.eval("document.getElementById('submit-btn').tagName");
        defer ctx.freeValue(tag_val);
        const tag_str = ctx.toCString(tag_val);
        defer std.heap.page_allocator.free(tag_str);
        expect_eq(std.mem.eql(u8, tag_str, "BUTTON"), true);

        const cls_val = ctx.eval("document.getElementById('submit-btn').getAttribute('class')");
        defer ctx.freeValue(cls_val);
        const cls_str = ctx.toCString(cls_val);
        defer std.heap.page_allocator.free(cls_str);
        expect_eq(std.mem.eql(u8, cls_str, "btn active"), true);

        // Query selector with CSS class
        const btn_val = ctx.eval("document.querySelector('.btn.active').id");
        defer ctx.freeValue(btn_val);
        const btn_id = ctx.toCString(btn_val);
        defer std.heap.page_allocator.free(btn_id);
        expect_eq(std.mem.eql(u8, btn_id, "submit-btn"), true);

        // QuerySelectorAll
        const all_val = ctx.eval("document.querySelectorAll('p').length");
        defer ctx.freeValue(all_val);
        const all_str = ctx.toCString(all_val);
        defer std.heap.page_allocator.free(all_str);
        expect_eq(std.mem.eql(u8, all_str, "1"), true);

        // document.body
        const body_val = ctx.eval("document.body !== null");
        defer ctx.freeValue(body_val);
        const body_str = ctx.toCString(body_val);
        defer std.heap.page_allocator.free(body_str);
        expect_eq(std.mem.eql(u8, body_str, "true"), true);
    });

    test("3. DOM Mutation via JavaScript reflected in Arena DOM", () => {
        let rt = JsRuntime.init(2 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html>\n" ++
            "<html><body><div id=\"root\"></div></body></html>\n";
        doc.parseHtml(html) catch unreachable;

        * = mountDom(&ctx, &doc, null) catch unreachable;

        // Create new element, set attributes, and append via JS
        const js_code =
            "const root = document.getElementById('root');\n" ++
            "const card = document.createElement('section');\n" ++
            "card.id = 'agent-card';\n" ++
            "card.className = 'card featured';\n" ++
            "card.setAttribute('data-status', 'ready');\n" ++
            "root.appendChild(card);\n" ++
            "'ok';";

        const res_val = ctx.eval(js_code);
        defer ctx.freeValue(res_val);

        // Verify changes directly in Arena DOM
        const card_id = doc.getElementById("agent-card");
        expect_eq(card_id != NULL_NODE, true);

        expect_eq(std.mem.eql(u8, doc.getTagName(card_id), "section"), true);
        if (cls_val := doc.getAttribute(card_id, "class")) {
            expect_eq(std.mem.eql(u8, cls_val, "card featured"), true);
        } else {
            expect_eq(true, false);
        }

        if (status_val := doc.getAttribute(card_id, "data-status")) {
            expect_eq(std.mem.eql(u8, status_val, "ready"), true);
        } else {
            expect_eq(true, false);
        }
    });

    test("4. getBoundingClientRect() Pixel Geometry Extraction", () => {
        let rt = JsRuntime.init(2 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html =
            "<!DOCTYPE html>\n" ++
            "<html>\n" ++
            "<body>\n" ++
            "  <div id=\"box\"></div>\n" ++
            "</body>\n" ++
            "</html>\n";
        doc.parseHtml(html) catch unreachable;

        const css_text =
            "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
            "#box { width: 320px; height: 180px; }\n";
        const sheet = StyleSheet.parse(css_text, 800.0, false);
        let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
        Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

        let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
        defer layout.deinit();
        layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

        * = mountDom(&ctx, &doc, &layout) catch unreachable;

        // Query getBoundingClientRect() via JS
        const js_code =
            "const box = document.getElementById('box');\n" ++
            "const r = box.getBoundingClientRect();\n" ++
            "r.width + 'x' + r.height + '@' + r.left + ',' + r.top;";

        const r_val = ctx.eval(js_code);
        defer ctx.freeValue(r_val);
        const r_str = ctx.toCString(r_val);
        defer std.heap.page_allocator.free(r_str);

        // Box width: 320, height: 180, left: 0, top: 0
        expect_eq(std.mem.eql(u8, r_str, "320x180@0,0"), true);
    });

    test("5. Reductions Preemptive Interrupt Control (Infinite Loop Abort)", () => {
        // Initialize runtime with small reduction limit: 10,000 reductions
        let rt = JsRuntime.init(1024 * 1024, 10000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        expect_eq(!rt.isInterrupted(), true);

        // Execute infinite loop
        const loop_val = ctx.eval("while (true) {}");
        defer ctx.freeValue(loop_val);

        // QuickJS returned an exception because execution was preemptively interrupted!
        expect_eq(jsIsException(loop_val), true);
        expect_eq(rt.isInterrupted(), true);
    });
}
