// test/js/test_dom_events.cp
// CapSurf DOM Events, classList & High-Fidelity Web API Shim Test Suite

import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { JsRuntime, JsContext, JSValue, jsToSlice, jsIsException } from "../../src/js/qjs_engine.cp";
import { mountDom } from "../../src/js/dom_binding.cp";

fn main() {
    test("1. W3C DOM Events & Three-Phase Bubbling (target -> parent -> document -> window)", () => {
        let rt = JsRuntime.init(32 * 1024 * 1024, 100000) catch unreachable;
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
        let rt = JsRuntime.init(32 * 1024 * 1024, 100000) catch unreachable;
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
        let rt = JsRuntime.init(32 * 1024 * 1024, 100000) catch unreachable;
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
        let rt = JsRuntime.init(32 * 1024 * 1024, 100000) catch unreachable;
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
        expect_eq(std.mem.eql(u8, nat_str, "function toDataURL() { [native code] }"), true);
    });

    test("5. Web Storage, Location & Navigator Parity", () => {
        let rt = JsRuntime.init(32 * 1024 * 1024, 100000) catch unreachable;
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
            "const uaValid = navigator.userAgent.includes('CapSurf/1.0.0') || navigator.userAgent.includes('Kitesurf');\n" ++
            "const plat = navigator.platform;\n" ++
            "const href = document.location.href;\n" ++
            "agent + '|' + sess + '|' + uaValid + '|' + plat + '|' + href;\n";

        const val = ctx.eval(script);
        defer ctx.freeValue(val);
        const res_str = ctx.toCString(val);
        defer std.heap.page_allocator.free(res_str);
        expect_eq(std.mem.eql(u8, res_str, "007|val123|true|Linux x86_64|about:blank"), true);
    });

    test("6. High-Fidelity Form Controls & Constraint Validation", () => {
        let rt = JsRuntime.init(32 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body></body></html>";
        doc.parseHtml(html) catch unreachable;

        * = mountDom(&ctx, &doc, null) catch unreachable;

        const script =
            "const form = document.createElement('form');\n" ++
            "const input = document.createElement('input');\n" ++
            "input.type = 'text';\n" ++
            "input.name = 'username';\n" ++
            "input.value = 'agent007';\n" ++
            "input.required = true;\n" ++
            "form.appendChild(input);\n" ++
            "const chk = document.createElement('input');\n" ++
            "chk.type = 'checkbox';\n" ++
            "chk.name = 'agree';\n" ++
            "chk.checked = true;\n" ++
            "form.appendChild(chk);\n" ++
            "const ta = document.createElement('textarea');\n" ++
            "ta.name = 'bio';\n" ++
            "ta.value = 'Autonomous browser agent';\n" ++
            "form.appendChild(ta);\n" ++
            "const sel = document.createElement('select');\n" ++
            "sel.name = 'role';\n" ++
            "const opt1 = document.createElement('option');\n" ++
            "opt1.value = 'admin';\n" ++
            "opt1.text = 'Admin';\n" ++
            "const opt2 = document.createElement('option');\n" ++
            "opt2.value = 'user';\n" ++
            "opt2.text = 'User';\n" ++
            "opt2.selected = true;\n" ++
            "sel.appendChild(opt1);\n" ++
            "sel.appendChild(opt2);\n" ++
            "form.appendChild(sel);\n" ++
            "const valid = form.checkValidity();\n" ++
            "const inputValid = input.validity.valid;\n" ++
            "input.value = '';\n" ++
            "const inputMissing = input.validity.valueMissing;\n" ++
            "input.value = 'agent007';\n" ++
            "input.value + '|' + chk.checked + '|' + ta.value + '|' + sel.value + '|' + valid + '|' + inputValid + '|' + inputMissing;\n";

        const val = ctx.eval(script);
        defer ctx.freeValue(val);
        const res_str = ctx.toCString(val);
        defer std.heap.page_allocator.free(res_str);
        expect_eq(std.mem.eql(u8, res_str, "agent007|true|Autonomous browser agent|user|true|true|true"), true);
    });

    test("7. WHATWG FormData Parity (Lifecycle & Form Extraction)", () => {
        let rt = JsRuntime.init(32 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body></body></html>";
        doc.parseHtml(html) catch unreachable;

        * = mountDom(&ctx, &doc, null) catch unreachable;

        const script =
            "const form = document.createElement('form');\n" ++
            "const input1 = document.createElement('input');\n" ++
            "input1.name = 'q';\n" ++
            "input1.value = 'capsurf';\n" ++
            "form.appendChild(input1);\n" ++
            "const input2 = document.createElement('input');\n" ++
            "input2.name = 'tag';\n" ++
            "input2.value = 'ai';\n" ++
            "form.appendChild(input2);\n" ++
            "const fd = new FormData(form);\n" ++
            "fd.append('tag', 'agent');\n" ++
            "fd.set('q', 'capsurf-engine');\n" ++
            "const q = fd.get('q');\n" ++
            "const tags = fd.getAll('tag').join(',');\n" ++
            "const hasQ = fd.has('q');\n" ++
            "fd.delete('tag');\n" ++
            "const hasTag = fd.has('tag');\n" ++
            "q + '|' + tags + '|' + hasQ + '|' + hasTag;\n";

        const val = ctx.eval(script);
        defer ctx.freeValue(val);
        const res_str = ctx.toCString(val);
        defer std.heap.page_allocator.free(res_str);
        expect_eq(std.mem.eql(u8, res_str, "capsurf-engine|ai,agent|true|false"), true);
    });

    test("8. Traversal (TreeWalker/NodeIterator), Range & DOMParser", () => {
        let rt = JsRuntime.init(32 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body></body></html>";
        doc.parseHtml(html) catch unreachable;

        * = mountDom(&ctx, &doc, null) catch unreachable;

        const script =
            "const root = document.createElement('div');\n" ++
            "const p1 = document.createElement('p');\n" ++
            "p1.textContent = 'hello';\n" ++
            "const p2 = document.createElement('p');\n" ++
            "p2.textContent = 'world';\n" ++
            "root.appendChild(p1);\n" ++
            "root.appendChild(p2);\n" ++
            "const walker = document.createTreeWalker(root, NodeFilter.SHOW_ELEMENT);\n" ++
            "let walkerNodes = [];\n" ++
            "while (walker.nextNode()) {\n" ++
            "  walkerNodes.push(walker.currentNode.tagName.toLowerCase());\n" ++
            "}\n" ++
            "const range = document.createRange();\n" ++
            "range.selectNode(p1);\n" ++
            "const collapsed = range.collapsed;\n" ++
            "const parser = new DOMParser();\n" ++
            "const parsedDoc = parser.parseFromString('<html><head><title>Test Title</title></head><body><h1>Hello</h1></body></html>', 'text/html');\n" ++
            "const title = parsedDoc.title;\n" ++
            "walkerNodes.join(',') + '|' + collapsed + '|' + title;\n";

        const val = ctx.eval(script);
        defer ctx.freeValue(val);
        const res_str = ctx.toCString(val);
        defer std.heap.page_allocator.free(res_str);
        expect_eq(std.mem.eql(u8, res_str, "p,p|false|Test Title"), true);
    });

    test("9. Cloud Vector Recording & Web Animations Toggleable Engine", () => {
        let rt = JsRuntime.init(32 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const html = "<!DOCTYPE html><html><body><canvas id=\"cvs2\"></canvas></body></html>";
        doc.parseHtml(html) catch unreachable;

        * = mountDom(&ctx, &doc, null) catch unreachable;

        const script =
            "const cvs = document.getElementById('cvs2');\n" ++
            "const c = cvs.getContext('2d');\n" ++
            "c.beginPath();\n" ++
            "c.rect(10, 20, 100, 50);\n" ++
            "c.fill();\n" ++
            "const opsBefore = c.__opsCount();\n" ++
            "globalThis.__CAP_RENDER_CONFIG.canvasRecording = true;\n" ++
            "c.beginPath();\n" ++
            "c.rect(10, 20, 100, 50);\n" ++
            "c.fill();\n" ++
            "const opsAfter = c.__opsCount();\n" ++
            "globalThis.__CAP_RENDER_CONFIG.canvasRecording = false;\n" ++
            "const animEl = document.createElement('div');\n" ++
            "const anim = animEl.animate([{ opacity: 0 }, { opacity: 1 }], 500);\n" ++
            "const playState = anim.playState;\n" ++
            "opsBefore + '|' + (opsAfter > 0) + '|' + playState;\n";

        const val = ctx.eval(script);
        defer ctx.freeValue(val);
        const res_str = ctx.toCString(val);
        defer std.heap.page_allocator.free(res_str);
        expect_eq(std.mem.eql(u8, res_str, "0|true|running"), true);
    });
}
