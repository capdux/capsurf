// test/dom/test_collections_attributes.cp
// Tests data: URL percent-decoding & base64 decoding, NodeData attribute mutation & removal,
// and WHATWG NamedNodeMap collection operations.

import { HttpLoader } from "../../../src/net/http_loader.cp";
import { Document, ElementHandle, NodeId, NULL_NODE, Attr, NodeData } from "../../../src/dom/dom.cp";
import { NamedNodeMap } from "../../../src/dom/collections.cp";

fn assertCheck(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // =========================================================================
    // 1. Test HttpLoader.decodeDataUrl
    // =========================================================================
    {
        let out_buf: [1024]u8 = [0; 1024];

        // 1.1 Percent-encoded HTML data: URL
        const url_percent = "data:text/html;charset=utf-8,%3Cdiv%3EHello%3C/div%3E";
        const dec1_opt = HttpLoader.decodeDataUrl(url_percent, &out_buf);
        assertCheck(dec1_opt != null, "decodeDataUrl percent decoded should not be null");
        const dec1 = dec1_opt.?;
        assertCheck(std.mem.eql(u8, dec1, "<div>Hello</div>"), "percent-decoding matches <div>Hello</div>");

        // 1.2 Base64-encoded HTML data: URL
        // "PHA+Y2Fwc3VyZjwvcD4=" -> "<p>capsurf</p>"
        const url_b64 = "data:text/html;base64,PHA+Y2Fwc3VyZjwvcD4=";
        const dec2_opt = HttpLoader.decodeDataUrl(url_b64, &out_buf);
        assertCheck(dec2_opt != null, "decodeDataUrl base64 decoded should not be null");
        const dec2 = dec2_opt.?;
        assertCheck(std.mem.eql(u8, dec2, "<p>capsurf</p>"), "base64-decoding matches <p>capsurf</p>");

        // 1.3 Percent-decoding with hex uppercase, lowercase, digits, and punctuation
        const url_mixed = "data:text/plain,%48%65%6c%6c%6f%20%57%6f%72%6c%64%21";
        const dec3_opt = HttpLoader.decodeDataUrl(url_mixed, &out_buf);
        assertCheck(dec3_opt != null, "decodeDataUrl mixed percent should not be null");
        const dec3 = dec3_opt.?;
        assertCheck(std.mem.eql(u8, dec3, "Hello World!"), "percent-decoding matches Hello World!");

        // 1.4 Plain data: URL without encoding
        const url_plain = "data:,simple text";
        const dec4_opt = HttpLoader.decodeDataUrl(url_plain, &out_buf);
        assertCheck(dec4_opt != null, "decodeDataUrl plain should not be null");
        const dec4 = dec4_opt.?;
        assertCheck(std.mem.eql(u8, dec4, "simple text"), "plain data url decoded matches");

        // 1.5 Invalid data URLs
        assertCheck(HttpLoader.decodeDataUrl("https://example.com/page.html", &out_buf) == null, "non-data url returns null");
        assertCheck(HttpLoader.decodeDataUrl("data:text/html", &out_buf) == null, "data url without comma returns null");
    }

    // =========================================================================
    // 2. Test NodeData, ElementHandle, and Document setAttribute & removeAttribute
    // =========================================================================
    {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        const node_id = doc.createElement("section") catch @panic("create section failed");
        let elem = ElementHandle.init(&doc, node_id);

        // 2.1 Set attributes
        elem.setAttribute("id", "main-section");
        elem.setAttribute("class", "container fluid active");
        elem.setAttribute("style", "color: blue; padding: 10px;");
        elem.setAttribute("data-role", "dialog");
        elem.setAttribute("aria-hidden", "false");

        // Verify initial state
        const n_ptr = doc.getNode(node_id).?;
        assertCheck(n_ptr.hasId("main-section"), "hasId matches main-section");
        assertCheck(n_ptr.hasClass("container"), "hasClass matches container");
        assertCheck(n_ptr.hasClass("fluid"), "hasClass matches fluid");
        assertCheck(n_ptr.hasClass("active"), "hasClass matches active");
        assertCheck(n_ptr.class_count == 3, "class_count is 3");
        assertCheck(std.mem.eql(u8, elem.getAttribute("id").?, "main-section"), "elem getAttribute id");
        assertCheck(std.mem.eql(u8, elem.getAttribute("data-role").?, "dialog"), "elem getAttribute data-role");
        assertCheck(std.mem.eql(u8, elem.getAttribute("aria-hidden").?, "false"), "elem getAttribute aria-hidden");

        // 2.2 Remove normal attribute via ElementHandle
        const removed_role = elem.removeAttribute("data-role");
        assertCheck(removed_role, "removeAttribute data-role returned true");
        assertCheck(elem.getAttribute("data-role") == null, "data-role attribute is removed");
        assertCheck(!elem.removeAttribute("data-role"), "removeAttribute second time returns false");

        // 2.3 Remove attribute via Document
        const removed_aria = doc.removeAttribute(node_id, "aria-hidden");
        assertCheck(removed_aria, "doc.removeAttribute aria-hidden returned true");
        assertCheck(elem.getAttribute("aria-hidden") == null, "aria-hidden is removed");
        assertCheck(!doc.removeAttribute(node_id, "aria-hidden"), "doc.removeAttribute second time returns false");

        // 2.4 Remove id attribute
        const removed_id = elem.removeAttribute("id");
        assertCheck(removed_id, "removeAttribute id returned true");
        const n_after_id = doc.getNode(node_id).?;
        assertCheck(!n_after_id.hasId("main-section"), "hasId returns false after id removed");
        assertCheck(n_after_id.id_attr.len == 0, "id_attr is empty string");
        assertCheck(elem.getAttribute("id") == null, "getAttribute id returns null");
        assertCheck(!elem.removeAttribute("id"), "removing id again returns false");

        // 2.5 Remove class attribute
        const removed_class = elem.removeAttribute("class");
        assertCheck(removed_class, "removeAttribute class returned true");
        const n_after_cls = doc.getNode(node_id).?;
        assertCheck(n_after_cls.class_count == 0, "class_count reset to 0");
        assertCheck(n_after_cls.class_attr.len == 0, "class_attr is empty string");
        assertCheck(!n_after_cls.hasClass("container"), "hasClass container returns false");
        assertCheck(elem.getAttribute("class") == null, "getAttribute class returns null");
        assertCheck(!elem.removeAttribute("class"), "removing class again returns false");

        // 2.6 Remove style attribute
        const removed_style = elem.removeAttribute("style");
        assertCheck(removed_style, "removeAttribute style returned true");
        const n_after_style = doc.getNode(node_id).?;
        assertCheck(n_after_style.inline_style.len == 0, "inline_style is empty string");
        assertCheck(elem.getAttribute("style") == null, "getAttribute style returns null");
        assertCheck(!elem.removeAttribute("style"), "removing style again returns false");

        // 2.7 Case-insensitive removeAttribute
        elem.setAttribute("title", "Hello CapSurf");
        assertCheck(elem.getAttribute("TITLE") != null, "getAttribute TITLE case-insensitive");
        assertCheck(elem.removeAttribute("TiTlE"), "removeAttribute with mixed case");
        assertCheck(elem.getAttribute("title") == null, "title removed completely");
    }

    // =========================================================================
    // 3. Test NamedNodeMap
    // =========================================================================
    {
        let map: NamedNodeMap = NamedNodeMap.init();
        assertCheck(map.length() == 0, "initial length is 0");
        assertCheck(map.item(0) == null, "item(0) on empty map is null");
        assertCheck(map.getNamedItem("src") == null, "getNamedItem on empty map is null");

        // 3.1 setNamedItem
        map.setNamedItem(Attr{ name: "src", value: "/assets/logo.png" });
        map.setNamedItem(Attr{ name: "alt", value: "CapSurf Logo" });
        map.setNamedItem(Attr{ name: "width", value: "320" });
        map.setNamedItem(Attr{ name: "height", value: "240" });
        assertCheck(map.length() == 4, "map length is 4");

        // 3.2 item(index)
        const item0 = map.item(0);
        assertCheck(item0 != null, "item(0) is not null");
        assertCheck(std.mem.eql(u8, item0.?.name, "src"), "item(0) name is src");
        assertCheck(std.mem.eql(u8, item0.?.value, "/assets/logo.png"), "item(0) value is /assets/logo.png");
        assertCheck(map.item(4) == null, "item(4) out of bounds is null");

        // 3.3 getNamedItem (case-insensitive)
        const src_attr = map.getNamedItem("SRC");
        assertCheck(src_attr != null, "getNamedItem SRC is not null");
        assertCheck(std.mem.eql(u8, src_attr.?.value, "/assets/logo.png"), "SRC value matches");

        const width_attr = map.getNamedItem("width");
        assertCheck(width_attr != null, "getNamedItem width is not null");
        assertCheck(std.mem.eql(u8, width_attr.?.value, "320"), "width value matches");

        // 3.4 Overwrite attribute with same name
        map.setNamedItem(Attr{ name: "alt", value: "Updated Logo" });
        assertCheck(map.length() == 4, "length remains 4 after overwrite");
        const updated_alt = map.getNamedItem("alt");
        assertCheck(updated_alt != null, "updated alt exists");
        assertCheck(std.mem.eql(u8, updated_alt.?.value, "Updated Logo"), "alt value updated");

        // 3.5 removeNamedItem
        const removed_alt = map.removeNamedItem("ALT");
        assertCheck(removed_alt != null, "removeNamedItem ALT returned removed Attr");
        assertCheck(std.mem.eql(u8, removed_alt.?.value, "Updated Logo"), "removed Attr value matches");
        assertCheck(map.length() == 3, "length decremented to 3");
        assertCheck(map.getNamedItem("alt") == null, "alt no longer in map");

        const removed_missing = map.removeNamedItem("nonexistent");
        assertCheck(removed_missing == null, "remove nonexistent returns null");
        assertCheck(map.length() == 3, "length still 3 after failed remove");

        // 3.6 Remove remaining items until empty
        assertCheck(map.removeNamedItem("src") != null, "removed src");
        assertCheck(map.removeNamedItem("width") != null, "removed width");
        assertCheck(map.removeNamedItem("height") != null, "removed height");
        assertCheck(map.length() == 0, "map is empty");
        assertCheck(map.item(0) == null, "item(0) on empty map is null");
    }

    std.debug.print("test_collections_attributes: ALL ASSERTIONS PASSED\n", .{});
}
