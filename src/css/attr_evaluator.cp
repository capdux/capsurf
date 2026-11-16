// src/css/attr_evaluator.cp
// CSS attr() expression parser and evaluator per W3C CSS Values and Units Module Level 5.

pub struct ParsedAttr {
    attr_name: []const u8 = "",
    type_name: []const u8 = "",
    fallback: ([]const u8)? = null,
    is_valid: bool = false,
}

pub fn parseAttrExpr(expr: []const u8): ParsedAttr {
    const trimmed = std.mem.trim(u8, expr, " \t\r\n");
    if (trimmed.len < 6) {
        return ParsedAttr{
            attr_name: "",
            type_name: "",
            fallback: null,
            is_valid: false,
        };
    }

    if (!std.ascii.eqlIgnoreCase(trimmed[0..5], "attr(") || trimmed[trimmed.len - 1] != ')') {
        return ParsedAttr{
            attr_name: "",
            type_name: "",
            fallback: null,
            is_valid: false,
        };
    }

    const inner = std.mem.trim(u8, trimmed[5 .. trimmed.len - 1], " \t\r\n");
    if (inner.len == 0) {
        return ParsedAttr{
            attr_name: "",
            type_name: "",
            fallback: null,
            is_valid: false,
        };
    }

    // Find comma at depth 0 separating `<attr-name> [type(<type-name>)]?` and `[, <fallback>]?`
    let comma_pos_opt: usize? = null;
    let depth: usize = 0;
    let i: usize = 0;
    while (i < inner.len) : (i += 1) {
        const c = inner[i];
        if (c == '(') {
            depth += 1;
        } else if (c == ')') {
            if (depth > 0) depth -= 1;
        } else if (c == ',' && depth == 0) {
            comma_pos_opt = i;
            break;
        }
    }

    let head: []const u8 = "";
    let fallback: ([]const u8)? = null;

    if (comma_pos := comma_pos_opt) {
        head = std.mem.trim(u8, inner[0..comma_pos], " \t\r\n");
        fallback = std.mem.trim(u8, inner[comma_pos + 1 ..], " \t\r\n");
    } else {
        head = inner;
        fallback = null;
    }

    // Parse head: `<attr-name> [type(<type-name>)]?`
    let attr_name: []const u8 = "";
    let type_name: []const u8 = "";

    const space_pos_opt = std.mem.indexOfAny(u8, head, " \t\r\n");
    if (space_pos := space_pos_opt) {
        attr_name = head[0..space_pos];
        type_name = std.mem.trim(u8, head[space_pos..], " \t\r\n");
    } else {
        attr_name = head;
        type_name = "";
    }

    return ParsedAttr{
        attr_name: attr_name,
        type_name: type_name,
        fallback: fallback,
        is_valid: true,
    };
}

pub fn isUrlType(type_str: []const u8): bool {
    if (std.ascii.eqlIgnoreCase(type_str, "url") || std.ascii.eqlIgnoreCase(type_str, "<url>")) return true;
    if (type_str.len >= 11) {
        if (std.ascii.eqlIgnoreCase(type_str[0..5], "type(") && type_str[type_str.len - 1] == ')') {
            const inner = std.mem.trim(u8, type_str[5 .. type_str.len - 1], " \t\r\n");
            if (std.ascii.eqlIgnoreCase(inner, "url") || std.ascii.eqlIgnoreCase(inner, "<url>")) return true;
        }
    }
    let i: usize = 0;
    while (i + 11 <= type_str.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(type_str[i .. i + 11], "type(<url>)")) return true;
    }
    return false;
}

pub fn isNumberType(type_str: []const u8): bool {
    if (std.ascii.eqlIgnoreCase(type_str, "number") || std.ascii.eqlIgnoreCase(type_str, "<number>")) return true;
    if (type_str.len >= 14) {
        if (std.ascii.eqlIgnoreCase(type_str[0..5], "type(") && type_str[type_str.len - 1] == ')') {
            const inner = std.mem.trim(u8, type_str[5 .. type_str.len - 1], " \t\r\n");
            if (std.ascii.eqlIgnoreCase(inner, "number") || std.ascii.eqlIgnoreCase(inner, "<number>")) return true;
        }
    }
    let i: usize = 0;
    while (i + 14 <= type_str.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(type_str[i .. i + 14], "type(<number>)")) return true;
    }
    return false;
}

/// Evaluates a CSS attr() expression against an optional attribute value.
/// Per W3C CSS Values and Units Module Level 5:
/// - attr() output with type(<url>) is attr()-tainted and invalid at computed-value time:
///   if fallback is provided, returns fallback; otherwise returns "".
/// - attr() with type(<number>): if attr_val is present, returns attr_val;
///   else if fallback is provided, returns fallback; otherwise returns "".
/// - If no type parameter is specified: if attr_val is present, returns attr_val;
///   else returns fallback (if any) or "".
pub fn evaluateCssAttr(expr: []const u8, attr_val: ([]const u8)?): []const u8 {
    const parsed = parseAttrExpr(expr);
    if (!parsed.is_valid) {
        if (v := attr_val) return v;
        return "";
    }

    const type_str = parsed.type_name;
    const is_url_type = isUrlType(type_str);
    const is_num_type = isNumberType(type_str);

    // W3C CSS Values and Units Level 5:
    // attr() with type(<url>) produces an attr()-tainted URL value.
    // Tainted URLs are invalid at computed-value time:
    // - Prohibit returning a valid url;
    // - If fallback provided, return fallback;
    // - If no fallback provided, return "" (empty slice).
    if (is_url_type) {
        if (fb := parsed.fallback) {
            return fb;
        }
        return "";
    }

    // If type is type(<number>):
    // - If attr_val exists, return attr_val;
    // - If attr_val is absent and fallback provided, return fallback;
    // - Otherwise return "".
    if (is_num_type) {
        if (v := attr_val) {
            return v;
        }
        if (fb := parsed.fallback) {
            return fb;
        }
        return "";
    }

    // If no type specified (or general type):
    // - If attr_val exists, return attr_val;
    // - Otherwise return fallback (if provided) or "".
    if (v := attr_val) {
        return v;
    }
    if (fb := parsed.fallback) {
        return fb;
    }
    return "";
}
