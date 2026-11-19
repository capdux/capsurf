// test/html/parser.cp
import { test, expect_eq } from "cap:test";
import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";

fn main() {
    test("Tokenizer/TreeBuilder: Basic Parse", () => {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();
        doc.parseHtml("<html><body><p>Test</p></body></html>") catch unreachable;
        
        expect_eq(doc.root_element != NULL_NODE, true);
    });
}
