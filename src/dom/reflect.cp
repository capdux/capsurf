// Migrated from vendor/kitesurf/packages/dom-shim/src/element/reflect.ts
// Implements WebIDL attribute reflection rules (string, boolean, numeric, enum, and URL).

import { resolveUrl } from "../eval/script_gate.cp";

pub fn reflectString(attr_val: ([]const u8)?, default_val: []const u8): []const u8 {
    return attr_val ?? default_val;
}

pub fn reflectBoolean(has_attr: bool): bool {
    return has_attr;
}

pub fn reflectInteger(attr_val: ([]const u8)?, default_val: i32, min_val: i32, max_val: i32): i32 {
    if (attr_val == null) return default_val;
    let v = attr_val.?;
    let trimmed = std.mem.trim(u8, v, " \t\r\n");
    if (trimmed.len == 0) return default_val;

    let parsed = std.fmt.parseInt(i32, trimmed, 10) catch return default_val;
    if (parsed < min_val) return min_val;
    if (parsed > max_val) return max_val;
    return parsed;
}

pub fn reflectUnsignedInteger(attr_val: ([]const u8)?, default_val: u32, min_val: u32, max_val: u32): u32 {
    if (attr_val == null) return default_val;
    let v = attr_val.?;
    let trimmed = std.mem.trim(u8, v, " \t\r\n");
    if (trimmed.len == 0) return default_val;

    let parsed = std.fmt.parseInt(u32, trimmed, 10) catch return default_val;
    if (parsed < min_val) return min_val;
    if (parsed > max_val) return max_val;
    return parsed;
}

pub fn reflectEnum(
    attr_val: ([]const u8)?,
    allowed_list: []const []const u8,
    default_val: []const u8,
    invalid_val: []const u8,
): []const u8 {
    if (attr_val == null) return default_val;
    let v = attr_val.?;
    let trimmed = std.mem.trim(u8, v, " \t\r\n");
    if (trimmed.len == 0) return default_val;

    for (item of allowed_list) {
        if (std.ascii.eqlIgnoreCase(trimmed, item)) {
            return item;
        }
    }
    return invalid_val;
}

pub fn reflectUrl(attr_val: ([]const u8)?, base_url: []const u8, out_buf: []u8): []const u8 {
    if (attr_val == null) return "";
    let v = attr_val.?;
    if (v.len == 0) return "";
    return resolveUrl(v, base_url, out_buf);
}
