import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

fn countByName(doc: *Document, name: []const u8): usize {
    const ids = doc.getElementsByName(name) catch @panic("oom");
    defer std.heap.page_allocator.free(ids);
    return ids.len;
}

pub fn main() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const a = doc.createElement("input") catch @panic("a");
    const b = doc.createElement("input") catch @panic("b");
    const c = doc.createElement("div") catch @panic("c");
    const d = doc.createElement("meta") catch @panic("d");
    const e1 = doc.createElement("p") catch @panic("e1");
    doc.setAttribute(a, "name", "q");
    doc.setAttribute(b, "name", "q");
    doc.setAttribute(c, "name", "a\\b\"c\nd");
    doc.setAttribute(d, "name", "😀");
    doc.setAttribute(e1, "name", "");

    assert(countByName(&doc, "q") == 2, "two q");
    assert(countByName(&doc, "Q") == 0, "case sensitive value");
    assert(countByName(&doc, "a\\b\"c\nd") == 1, "special chars literal");
    assert(countByName(&doc, "a\\b") == 0, "no partial");
    assert(countByName(&doc, "😀") == 1, "emoji");
    assert(countByName(&doc, "") == 0, "empty returns empty");
    assert(countByName(&doc, "none") == 0, "none");
    std.debug.print("test_document_collections OK\n", .{});
}
