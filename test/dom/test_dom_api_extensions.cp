import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { DOMRange } from "../../src/dom/range.cp";
import { JsRuntime } from "../../src/js/qjs_engine.cp";
import { mountDom } from "../../src/js/dom_binding.cp";

fn testGetAttributeNames(doc: *Document, el: NodeId)! {
    let attr_names = doc.nodes.items[el].getAttributeNames(std.heap.page_allocator)?;
    defer std.heap.page_allocator.free(attr_names);

    // Check if we found "id" and "class"
    let found_id = false;
    let found_class = false;
    for (name of attr_names) {
        if (std.mem.eql(u8, name, "id")) found_id = true;
        if (std.mem.eql(u8, name, "class")) found_class = true;
    }
    if (!found_id || !found_class) {
        @panic("getAttributeNames failed");
    }
}

fn testInnerHtml(doc: *Document)! {
    let container = doc.createElement("div")?;

    let span = doc.createElement("span")?;
    let span_text = doc.createTextNode("hello & <world>")?;
    doc.appendChild(span, span_text);
    doc.appendChild(container, span);

    let img = doc.createElement("img")?;
    doc.setAttribute(img, "src", "avatar.png");
    doc.appendChild(container, img);

    let br = doc.createElement("br")?;
    doc.appendChild(container, br);

    let comment = doc.createCommentNode("my-comment")?;
    doc.appendChild(container, comment);

    let script = doc.createElement("script")?;
    let script_text = doc.createTextNode("if (a && b < c) {}")?;
    doc.appendChild(script, script_text);
    doc.appendChild(container, script);

    const html = doc.getInnerHtml(container)?;
    defer doc.allocator.free(html);

    const expected = "<span>hello &amp; &lt;world&gt;</span><img src=\"avatar.png\"><br><!--my-comment--><script>if (a && b < c) {}</script>";
    if (!std.mem.eql(u8, html, expected)) {
        std.debug.print("Expected:\n{s}\nGot:\n{s}\n", .{ expected, html });
        @panic("getInnerHtml mismatch");
    }
}

fn testInsertBeforeFirstChild(doc: *Document)! {
    let ul = doc.createElement("ul")?;
    let li1 = doc.createElement("li")?;
    doc.appendChild(ul, li1);

    if (doc.nodes.items[@as(usize, ul)].first_child != li1) {
        @panic("initial first_child != li1");
    }
    if (doc.nodes.items[@as(usize, ul)].last_child != li1) {
        @panic("initial last_child != li1");
    }
    if (doc.nodes.items[@as(usize, li1)].element_index_in_parent != 1) {
        @panic("initial li1 element_index_in_parent != 1");
    }

    // Insert li0 before first_child
    let li0 = doc.createElement("li")?;
    doc.insertBefore(ul, li0, li1);

    if (doc.nodes.items[@as(usize, ul)].first_child != li0) {
        @panic("after insertBefore, ul.first_child != li0");
    }
    if (doc.nodes.items[@as(usize, ul)].last_child != li1) {
        @panic("after insertBefore, ul.last_child != li1");
    }
    if (doc.nodes.items[@as(usize, li0)].prev_sibling != NULL_NODE) {
        @panic("li0 prev_sibling != NULL_NODE");
    }
    if (doc.nodes.items[@as(usize, li0)].next_sibling != li1) {
        @panic("li0 next_sibling != li1");
    }
    if (doc.nodes.items[@as(usize, li1)].prev_sibling != li0) {
        @panic("li1 prev_sibling != li0");
    }
    if (doc.nodes.items[@as(usize, li1)].next_sibling != NULL_NODE) {
        @panic("li1 next_sibling != NULL_NODE");
    }
    if (doc.nodes.items[@as(usize, ul)].child_element_count != 2) {
        @panic("ul child_element_count != 2");
    }
    if (doc.nodes.items[@as(usize, li0)].element_index_in_parent != 1) {
        @panic("li0 element_index_in_parent != 1");
    }
    if (doc.nodes.items[@as(usize, li1)].element_index_in_parent != 2) {
        @panic("li1 element_index_in_parent != 2");
    }
}

fn testNamespaceAttributes(doc: *Document)! {
    let el = doc.createElement("use")?;
    const XLINK = "http://www.w3.org/1999/xlink";

    doc.setAttributeNS(el, XLINK, "xlink:href", "#icon");

    const val1 = doc.getAttributeNS(el, XLINK, "href");
    if (val1 == null || !std.mem.eql(u8, val1.?, "#icon")) {
        @panic("getAttributeNS failed");
    }

    const val2 = doc.getAttribute(el, "xlink:href");
    if (val2 == null || !std.mem.eql(u8, val2.?, "#icon")) {
        @panic("getAttribute qualified xlink:href failed");
    }

    const removed = doc.removeAttributeNS(el, XLINK, "href");
    if (!removed) {
        @panic("removeAttributeNS returned false");
    }

    const val3 = doc.getAttributeNS(el, XLINK, "href");
    if (val3 != null) {
        @panic("getAttributeNS should be null after removal");
    }
}

fn testJsDomBindings()! {
    let rt = JsRuntime.init(16 * 1024 * 1024, 100000)?;
    defer rt.deinit();

    let ctx = rt.createContext()?;
    defer ctx.deinit();

    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    * = mountDom(&ctx, &doc, null)?;

    // 1. Test element innerHTML getter & setter
    const set_res = ctx.eval("let d = document.createElement('div'); d.innerHTML = '<span>item</span><br>'; d.innerHTML");
    defer ctx.freeValue(set_res);
    const set_str = ctx.toCString(set_res);
    defer std.heap.page_allocator.free(set_str);
    if (!std.mem.eql(u8, set_str, "<span>item</span><br>")) {
        std.debug.print("JS innerHTML got: {s}\n", .{set_str});
        @panic("JS innerHTML getter/setter failed");
    }

    // 2. Test element.cloneNode(deep)
    const clone_res = ctx.eval("let c = d.cloneNode(true); c.innerHTML");
    defer ctx.freeValue(clone_res);
    const clone_str = ctx.toCString(clone_res);
    defer std.heap.page_allocator.free(clone_str);
    if (!std.mem.eql(u8, clone_str, "<span>item</span><br>")) {
        std.debug.print("JS cloneNode got: {s}\n", .{clone_str});
        @panic("JS cloneNode failed");
    }

    // 3. Test element.getAttributeNames()
    const names_res = ctx.eval("let e = document.createElement('div'); e.setAttribute('id', 'foo'); e.setAttribute('data-k', 'val'); e.getAttributeNames().sort().join(',')");
    defer ctx.freeValue(names_res);
    const names_str = ctx.toCString(names_res);
    defer std.heap.page_allocator.free(names_str);
    if (!std.mem.eql(u8, names_str, "data-k,id")) {
        std.debug.print("JS getAttributeNames got: {s}\n", .{names_str});
        @panic("JS getAttributeNames failed");
    }

    // 4. Test element.setAttributeNS / getAttributeNS / removeAttributeNS
    const ns_res = ctx.eval(
        "let u = document.createElement('use');\n" ++
        "const XLINK = 'http://www.w3.org/1999/xlink';\n" ++
        "u.setAttributeNS(XLINK, 'xlink:href', '#target');\n" ++
        "const r1 = u.getAttributeNS(XLINK, 'href');\n" ++
        "u.removeAttributeNS(XLINK, 'href');\n" ++
        "const r2 = u.getAttributeNS(XLINK, 'href');\n" ++
        "r1 + ',' + (r2 === null ? 'null' : r2)"
    );
    defer ctx.freeValue(ns_res);
    const ns_str = ctx.toCString(ns_res);
    defer std.heap.page_allocator.free(ns_str);
    if (!std.mem.eql(u8, ns_str, "#target,null")) {
        std.debug.print("JS NS attrs got: {s}\n", .{ns_str});
        @panic("JS setAttributeNS / getAttributeNS / removeAttributeNS failed");
    }

    // 5. Test JS insertBefore first child
    const ib_res = ctx.eval(
        "(() => {\n" ++
        "  let ul = document.createElement('ul');\n" ++
        "  let li1 = document.createElement('li');\n" ++
        "  ul.appendChild(li1);\n" ++
        "  let li0 = document.createElement('li');\n" ++
        "  ul.insertBefore(li0, li1);\n" ++
        "  const ok1 = (ul.firstChild === li0);\n" ++
        "  const ok2 = (li0.nextSibling === li1);\n" ++
        "  const ok3 = (li1.previousSibling === li0);\n" ++
        "  const ok4 = (li0.previousSibling === null);\n" ++
        "  return ok1 && ok2 && ok3 && ok4 ? 'PASS' : 'FAIL';\n" ++
        "})()"
    );
    defer ctx.freeValue(ib_res);
    const ib_str = ctx.toCString(ib_res);
    defer std.heap.page_allocator.free(ib_str);
    if (!std.mem.eql(u8, ib_str, "PASS")) {
        std.debug.print("JS insertBefore got: {s}\n", .{ib_str});
        @panic("JS insertBefore first child failed");
    }

    // 6. Test cssFloat alias mapping
    const cf_res = ctx.eval(
        "(() => {\n" ++
        "  let box = document.createElement('div');\n" ++
        "  box.style.cssText = 'float: left; width: 40px; height: 40px';\n" ++
        "  const r1 = box.style.cssFloat;\n" ++
        "  const r2 = box.style.float;\n" ++
        "  box.style.cssFloat = 'right';\n" ++
        "  const r3 = box.style.float;\n" ++
        "  const r4 = box.style.cssFloat;\n" ++
        "  const hasBogus = box.style.cssText.indexOf('css-float') >= 0;\n" ++
        "  const cs = getComputedStyle(box).float;\n" ++
        "  return r1 + ',' + r2 + ',' + r3 + ',' + r4 + ',' + (hasBogus ? '1' : '0') + ',' + cs;\n" ++
        "})()"
    );
    defer ctx.freeValue(cf_res);
    const cf_str = ctx.toCString(cf_res);
    defer std.heap.page_allocator.free(cf_str);
    if (!std.mem.eql(u8, cf_str, "left,left,right,right,0,right")) {
        std.debug.print("JS cssFloat got: {s}\n", .{cf_str});
        @panic("JS cssFloat alias test failed");
    }

    // 7. Test vendor prefix transform & align-content aliases
    const tr_res = ctx.eval(
        "(() => {\n" ++
        "  let el = document.createElement('div');\n" ++
        "  el.style.webkitTransform = 'scale(4)';\n" ++
        "  const r1 = el.style.getPropertyValue('-webkit-transform');\n" ++
        "  const r2 = el.style.getPropertyValue('transform');\n" ++
        "  el.style.setProperty('-webkit-align-content', 'center');\n" ++
        "  const r3 = el.style.getPropertyValue('align-content');\n" ++
        "  el.style.cssText = 'transform: scale(5)';\n" ++
        "  const cs = getComputedStyle(el).getPropertyValue('-webkit-transform');\n" ++
        "  return r1 + ',' + r2 + ',' + r3 + ',' + cs;\n" ++
        "})()"
    );
    defer ctx.freeValue(tr_res);
    const tr_str = ctx.toCString(tr_res);
    defer std.heap.page_allocator.free(tr_str);
    if (!std.mem.eql(u8, tr_str, "scale(4),scale(4),center,matrix(5, 0, 0, 5, 0, 0)")) {
        std.debug.print("JS vendor prefix transform got: {s}\n", .{tr_str});
        @panic("JS vendor prefix transform failed");
    }
}

pub fn main()! {
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();

    // Test getAttributeNames
    let el = doc.createElement("div")?;
    doc.setAttribute(el, "id", "test-id");
    doc.setAttribute(el, "class", "test-class");

    testGetAttributeNames(&doc, el)?;

    // Test cloneNode
    let clone = doc.cloneNode(el, true)?;
    if (clone == NULL_NODE || clone == el) {
        @panic("cloneNode failed");
    }

    // Test Range.selectNodeContents
    let r = DOMRange.init();
    r.selectNodeContents(&doc, el);
    if (r.start_container != el || r.end_container != el) {
        @panic("selectNodeContents failed");
    }

    // Test getInnerHtml serialization
    testInnerHtml(&doc)?;

    // Test namespace attributes
    testNamespaceAttributes(&doc)?;

    // Test insertBefore first child boundary
    testInsertBeforeFirstChild(&doc)?;

    // Test QuickJS bindings (innerHTML, cloneNode, getAttributeNames, getAttributeNS/setAttributeNS/removeAttributeNS)
    testJsDomBindings()?;

    std.debug.print("test_dom_api_extensions OK\n", .{});
}
