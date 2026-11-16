// Migrated from vendor/kitesurf/packages/engine/src/eval/page-isolate-entry.ts
// The page isolate's entry module generator. Generates an entry that re-exports page.js plus a host class.

pub const ENTRY_MODULE: []const u8 = "__ks_entry.js";

fn isValidIdentifier(name: []const u8): bool {
    if (name.len == 0) return false;
    let first = name[0];
    let is_alpha = (first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z') || first == '_' || first == '$';
    if (!is_alpha) return false;

    for (i in 1..name.len) {
        let ch = name[i];
        let is_ident = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '$';
        if (!is_ident) return false;
    }
    return true;
}

pub fn pageIsolateEntry(
    host_class_name: []const u8,
    first_module: []const u8,
    out_source_buf: []u8,
): []const u8 {
    if (host_class_name.len == 0 || first_module.len == 0 || !isValidIdentifier(host_class_name)) {
        return "page.js";
    }

    let p1 = "export * from \"page.js\";\nexport { default } from \"page.js\";\nexport { ";
    let p2 = " } from \"";
    let p3 = "\";\n";

    let pos: usize = 0;
    if (pos + p1.len > out_source_buf.len) return "page.js";
    @memcpy(out_source_buf[pos .. pos + p1.len], p1);
    pos += p1.len;

    if (pos + host_class_name.len > out_source_buf.len) return "page.js";
    @memcpy(out_source_buf[pos .. pos + host_class_name.len], host_class_name);
    pos += host_class_name.len;

    if (pos + p2.len > out_source_buf.len) return "page.js";
    @memcpy(out_source_buf[pos .. pos + p2.len], p2);
    pos += p2.len;

    if (pos + first_module.len > out_source_buf.len) return "page.js";
    @memcpy(out_source_buf[pos .. pos + first_module.len], first_module);
    pos += first_module.len;

    if (pos + p3.len > out_source_buf.len) return "page.js";
    @memcpy(out_source_buf[pos .. pos + p3.len], p3);
    pos += p3.len;

    return ENTRY_MODULE;
}
