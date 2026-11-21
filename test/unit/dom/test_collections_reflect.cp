import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import {
    DOMTokenList,
    NodeList,
    HTMLCollection,
} from "../../../src/dom/collections.cp";
import {
    reflectString,
    reflectBoolean,
    reflectInteger,
    reflectUnsignedInteger,
    reflectEnum,
    reflectUrl,
} from "../../../src/dom/reflect.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. Test DOMTokenList
    {
        let list: DOMTokenList = DOMTokenList.init();
        list.fromString("btn btn-primary active");
        assert(list.length() == 3, "initial tokens count is 3");
        assert(list.contains("btn"), "contains btn");
        assert(list.contains("btn-primary"), "contains btn-primary");
        assert(list.contains("active"), "contains active");
        assert(!list.contains("disabled"), "does not contain disabled");

        assert(list.add("disabled"), "add disabled token");
        assert(list.length() == 4, "count is now 4");
        assert(list.contains("disabled"), "contains disabled");

        assert(list.remove("active"), "remove active token");
        assert(list.length() == 3, "count is now 3");
        assert(!list.contains("active"), "active removed");

        // Toggle
        let t1 = list.toggle("selected");
        assert(t1, "toggle added selected");
        assert(list.contains("selected"), "contains selected");

        let t2 = list.toggle("selected");
        assert(!t2, "toggle removed selected");
        assert(!list.contains("selected"), "does not contain selected");

        // Replace
        assert(list.replaceToken("btn-primary", "btn-danger"), "replace btn-primary with btn-danger");
        assert(list.contains("btn-danger"), "contains btn-danger");
        assert(!list.contains("btn-primary"), "btn-primary is replaced");

        let buf: [256]u8 = [0; 256];
        let serialized = list.serialize(&buf);
        assert(std.mem.eql(u8, serialized, "btn btn-danger disabled"), "serialized token list matches");
    }

    // 2. Test NodeList & HTMLCollection
    {
        let doc = Document.init(std.heap.page_allocator) catch unreachable;
        defer doc.deinit();

        let n1 = doc.createElement("input") catch @panic("create input");
        doc.setId(n1, "username_field");
        doc.setAttribute(n1, "name", "username");

        let n2 = doc.createElement("input") catch @panic("create input");
        doc.setId(n2, "password_field");

        let col: HTMLCollection = HTMLCollection.init();
        * = col.append(n1);
        * = col.append(n2);
        assert(col.length() == 2, "collection length is 2");
        assert(col.item(0) == n1, "item 0 is n1");
        assert(col.item(1) == n2, "item 1 is n2");

        // Match by ID
        let found_by_id = col.namedItem(&doc, "username_field");
        assert(found_by_id == n1, "namedItem matches id");

        // Match by name attribute
        let found_by_name = col.namedItem(&doc, "username");
        assert(found_by_name == n1, "namedItem matches name attribute");

        let not_found = col.namedItem(&doc, "non_existent");
        assert(not_found == NULL_NODE, "namedItem returns NULL_NODE for missing");
    }

    // 3. Test WebIDL reflection
    {
        // String
        assert(std.mem.eql(u8, reflectString("custom", "default"), "custom"), "reflect string with val");
        assert(std.mem.eql(u8, reflectString(null, "default"), "default"), "reflect string null fallback");

        // Boolean
        assert(reflectBoolean(true), "reflect boolean true");
        assert(!reflectBoolean(false), "reflect boolean false");

        // Integer clamping
        assert(reflectInteger("50", 10, 0, 100) == 50, "reflect int in range");
        assert(reflectInteger("150", 10, 0, 100) == 100, "reflect int clamped to max");
        assert(reflectInteger("-50", 10, 0, 100) == 0, "reflect int clamped to min");
        assert(reflectInteger("invalid", 10, 0, 100) == 10, "reflect int invalid fallback");

        // Unsigned Integer
        assert(reflectUnsignedInteger("42", 1, 1, 100) == 42, "reflect uint in range");

        // Enum reflection
        let allowed: [3][]const u8 = [ "anonymous", "use-credentials", "" ];
        assert(std.mem.eql(u8, reflectEnum("ANONYMOUS", &allowed, "none", "invalid"), "anonymous"), "case-insensitive enum");
        assert(std.mem.eql(u8, reflectEnum("unknown", &allowed, "none", "invalid"), "invalid"), "enum invalid keyword");
        assert(std.mem.eql(u8, reflectEnum(null, &allowed, "none", "invalid"), "none"), "enum null default");

        // URL reflection
        let url_buf: [256]u8 = [0; 256];
        let res_url = reflectUrl("/app/main.js", "https://example.com/sub/", &url_buf);
        assert(std.mem.eql(u8, res_url, "https://example.com/app/main.js"), "reflect url resolved");
    }

    std.debug.print("test_collections_reflect: ALL ASSERTIONS PASSED\n", .{});
}
