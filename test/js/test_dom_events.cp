// test/js/test_dom_events.cp
// CapSurf DOM Events, classList & High-Fidelity Web API Shim Test Suite

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { JsRuntime, JsContext, jsToSlice, jsIsException } from "../../src/js/qjs_engine.cp";
import { mountDom } from "../../src/js/dom_binding.cp";

fn main() {
    test("1. W3C DOM Events & Three-Phase Bubbling (target -> parent -> document -> window)", () => {
        let rt = JsRuntime.init(2 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><div id=\"parent\"><button id=\"btn\">Click</button></div></body></html>";
        doc.parseHtml(html) catch unreachable;

        * = mountDom(&ctx, &doc, null) catch unreachable;

        const script =
            "let log = [];\n" ++
            "window.addEventListener('click', () => { log.push('window'); });\n" ++
            "document.addEventListener('click', () => { log.push('document'); });\n" ++
            "const parent = document.getElementById('parent');\n" ++
            "parent.addEventListener('click', (e) => { log.push('parent:' + e.currentTarget.id); });\n" ++
            "const btn = document.getElementById('btn');\n" ++
            "btn.addEventListener('click', (e) => { log.push('btn:' + e.target.id); });\n" ++
            "const evt = new Event('click', { bubbles: true, cancelable: true });\n" ++
            "btn.dispatchEvent(evt);\n" ++
            "log.join(',');\n";

        const val = ctx.eval(script);
        defer ctx.freeValue(val);
        const res_str = ctx.toCString(val);
        defer std.heap.page_allocator.free(res_str);
        expect_eq(std.mem.eql(u8, res_str, "btn:btn,parent:parent,document,window"), true);
    });

    test("2. Event stopPropagation & preventDefault", () => {
        let rt = JsRuntime.init(2 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><div id=\"box\"><span id=\"target\">Text</span></div></body></html>";
        doc.parseHtml(html) catch unreachable;

        * = mountDom(&ctx, &doc, null) catch unreachable;

        const script =
            "let reachedBox = false;\n" ++
            "document.getElementById('box').addEventListener('custom', () => { reachedBox = true; });\n" ++
            "const span = document.getElementById('target');\n" ++
            "span.addEventListener('custom', (e) => {\n" ++
            "  e.stopPropagation();\n" ++
            "  e.preventDefault();\n" ++
            "});\n" ++
            "const ce = new CustomEvent('custom', { bubbles: true, cancelable: true, detail: { x: 10 } });\n" ++
            "const notPrevented = span.dispatchEvent(ce);\n" ++
            "reachedBox + ',' + notPrevented + ',' + ce.defaultPrevented + ',' + ce.detail.x;\n";

        const val = ctx.eval(script);
        defer ctx.freeValue(val);
        const res_str = ctx.toCString(val);
        defer std.heap.page_allocator.free(res_str);
        expect_eq(std.mem.eql(u8, res_str, "false,false,true,10"), true);
    });

    test("3. DOMTokenList (classList) Synchronization with Arena DOM Attributes", () => {
        let rt = JsRuntime.init(2 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><button id=\"btn\" class=\"btn primary\">Go</button></body></html>";
        doc.parseHtml(html) catch unreachable;

        * = mountDom(&ctx, &doc, null) catch unreachable;

        const script =
            "const btn = document.getElementById('btn');\n" ++
            "const cl = btn.classList;\n" ++
            "const hasPrimary = cl.contains('primary');\n" ++
            "cl.add('active', 'rounded');\n" ++
            "cl.remove('primary');\n" ++
            "const toggledOn = cl.toggle('shadow');\n" ++
            "const toggledOff = cl.toggle('shadow');\n" ++
            "const attr = btn.getAttribute('class');\n" ++
            "hasPrimary + '|' + toggledOn + '|' + toggledOff + '|' + attr;\n";

        const val = ctx.eval(script);
        defer ctx.freeValue(val);
        const res_str = ctx.toCString(val);
        defer std.heap.page_allocator.free(res_str);
        expect_eq(std.mem.eql(u8, res_str, "true|true|false|btn active rounded"), true);
    });

    test("4. High-Fidelity Anti-Detect Stealth Shield (Canvas, WebGL Graceful, Native Masquerade)", () => {
        let rt = JsRuntime.init(2 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><canvas id=\"cvs\"></canvas></body></html>";
        doc.parseHtml(html) catch unreachable;

        * = mountDom(&ctx, &doc, null) catch unreachable;

        // 1. MeasureText calculation
        const measure_val = ctx.eval(
            "const cvs = document.getElementById('cvs');\n" ++
            "const ctx = cvs.getContext('2d');\n" ++
            "ctx.font = '20px sans-serif';\n" ++
            "const m = ctx.measureText('Hello');\n" ++
            "m.width;\n"
        );
        defer ctx.freeValue(measure_val);
        const m_str = ctx.toCString(measure_val);
        defer std.heap.page_allocator.free(m_str);
        // 5 chars * 20px * 0.6 = 60
        expect_eq(std.mem.eql(u8, m_str, "60"), true);

        // 2. Valid PNG base64 toDataURL
        const png_val = ctx.eval("document.getElementById('cvs').toDataURL().slice(0, 22)");
        defer ctx.freeValue(png_val);
        const png_str = ctx.toCString(png_val);
        defer std.heap.page_allocator.free(png_str);
        expect_eq(std.mem.eql(u8, png_str, "data:image/png;base64,"), true);

        // 3. WebGL returns null (standard headless fallback, no exception)
        const webgl_val = ctx.eval("document.getElementById('cvs').getContext('webgl') === null");
        defer ctx.freeValue(webgl_val);
        const webgl_str = ctx.toCString(webgl_val);
        defer std.heap.page_allocator.free(webgl_str);
        expect_eq(std.mem.eql(u8, webgl_str, "true"), true);

        // 4. Native function masquerade
        const nat_val = ctx.eval("document.getElementById('cvs').toDataURL.toString()");
        defer ctx.freeValue(nat_val);
        const nat_str = ctx.toCString(nat_val);
        defer std.heap.page_allocator.free(nat_str);
        expect_eq(std.mem.eql(u8, nat_str, "function () { [native code] }"), true);
    });

    test("5. Web Storage, Location & Navigator Parity", () => {
        let rt = JsRuntime.init(2 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body></body></html>";
        doc.parseHtml(html) catch unreachable;

        * = mountDom(&ctx, &doc, null) catch unreachable;

        const script =
            "localStorage.setItem('agent_id', '007');\n" ++
            "sessionStorage.setItem('temp_key', 'val123');\n" ++
            "const agent = localStorage.getItem('agent_id');\n" ++
            "const sess = sessionStorage.getItem('temp_key');\n" ++
            "const uaValid = navigator.userAgent.includes('CapSurf/1.0.0');\n" ++
            "const plat = navigator.platform;\n" ++
            "const href = document.location.href;\n" ++
            "agent + '|' + sess + '|' + uaValid + '|' + plat + '|' + href;\n";

        const val = ctx.eval(script);
        defer ctx.freeValue(val);
        const res_str = ctx.toCString(val);
        defer std.heap.page_allocator.free(res_str);
        expect_eq(std.mem.eql(u8, res_str, "007|val123|true|Linux x86_64|about:blank"), true);
    });
}
