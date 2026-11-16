// src/net/mime.cp
// WHATWG MIME specification parsing and normalization utilities

fn isHttpWhitespace(c: u8): bool {
    return c == 0x20 || c == 0x09 || c == 0x0A || c == 0x0D || c == 0x0C;
}

fn isHttpTokenByte(c: u8): bool {
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= '0' && c <= '9') return true;
    return switch (c) {
        '!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~' => true,
        * => false,
    };
}

/// WHATWG Blob type normalization.
/// If raw contains any byte < 0x20 (control characters before space) or > 0x7E
/// (non-ASCII characters after tilde), returns empty slice "".
/// Otherwise converts 'A'..'Z' to lowercase 'a'..'z', preserves leading/trailing
/// spaces, writes to out_buf, and returns out_buf[0..raw.len].
pub fn normalizeBlobType(raw: []const u8, out_buf: []u8): []const u8 {
    if (out_buf.len < raw.len) return "";

    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        const c = raw[i];
        if (c < 0x20 || c > 0x7E) {
            return "";
        }
        if (c >= 'A' && c <= 'Z') {
            out_buf[i] = c + ('a' - 'A');
        } else {
            out_buf[i] = c;
        }
    }
    return out_buf[0..raw.len];
}

pub struct MimeType {
    raw: []const u8 = "",
    essence: []const u8 = "",
    charset: []const u8 = "",

    static fn parse(raw_str: []const u8): MimeType? {
        // 1. Trim leading and trailing HTTP whitespace
        let start: usize = 0;
        while (start < raw_str.len && isHttpWhitespace(raw_str[start])) : (start += 1) {}
        let end: usize = raw_str.len;
        while (end > start && isHttpWhitespace(raw_str[end - 1])) : (end -= 1) {}

        if (start >= end) return null;
        const trimmed = raw_str[start..end];

        // 2. Find slash dividing type and subtype
        let slash_idx: usize = 0;
        let found_slash = false;
        let i: usize = 0;
        while (i < trimmed.len) : (i += 1) {
            if (trimmed[i] == '/') {
                slash_idx = i;
                found_slash = true;
                break;
            }
        }
        if (!found_slash || slash_idx == 0) return null;

        const type_part = trimmed[0..slash_idx];
        let ti: usize = 0;
        while (ti < type_part.len) : (ti += 1) {
            if (!isHttpTokenByte(type_part[ti])) return null;
        }

        // 3. Find semicolon dividing subtype and parameters
        const rest = trimmed[slash_idx + 1 ..];
        let semi_idx: usize = rest.len;
        let si: usize = 0;
        while (si < rest.len) : (si += 1) {
            if (rest[si] == ';') {
                semi_idx = si;
                break;
            }
        }

        const subtype_raw = rest[0..semi_idx];
        let subtype_end: usize = subtype_raw.len;
        while (subtype_end > 0 && isHttpWhitespace(subtype_raw[subtype_end - 1])) : (subtype_end -= 1) {}
        if (subtype_end == 0) return null;

        const subtype_part = subtype_raw[0..subtype_end];
        let sti: usize = 0;
        while (sti < subtype_part.len) : (sti += 1) {
            if (!isHttpTokenByte(subtype_part[sti])) return null;
        }

        // Essence is the slice in trimmed representing type/subtype
        const essence_len = slash_idx + 1 + subtype_end;
        const essence = trimmed[0..essence_len];

        // 4. Parse parameters to find charset
        let charset_slice: []const u8 = "";
        if (semi_idx < rest.len) {
            const params_str = rest[semi_idx + 1 ..];
            let pi: usize = 0;
            while (pi < params_str.len) {
                // Skip leading whitespace and stray semicolons
                while (pi < params_str.len && (isHttpWhitespace(params_str[pi]) || params_str[pi] == ';')) : (pi += 1) {}
                if (pi >= params_str.len) break;

                // Read parameter name
                const name_start = pi;
                while (pi < params_str.len && params_str[pi] != '=' && params_str[pi] != ';') : (pi += 1) {}
                let name_end = pi;
                while (name_end > name_start && isHttpWhitespace(params_str[name_end - 1])) : (name_end -= 1) {}
                const param_name = params_str[name_start..name_end];

                if (pi < params_str.len && params_str[pi] == ';') {
                    pi += 1;
                    continue;
                }
                if (pi >= params_str.len) break;

                // Skip '='
                pi += 1;
                // Skip whitespace after '='
                while (pi < params_str.len && isHttpWhitespace(params_str[pi])) : (pi += 1) {}
                if (pi >= params_str.len) break;

                let val_slice: []const u8 = "";
                if (params_str[pi] == '"') {
                    // Quoted string parameter
                    pi += 1;
                    const val_start = pi;
                    while (pi < params_str.len && params_str[pi] != '"') {
                        if (params_str[pi] == '\\' && pi + 1 < params_str.len) {
                            pi += 2;
                        } else {
                            pi += 1;
                        }
                    }
                    val_slice = params_str[val_start..pi];
                    if (pi < params_str.len && params_str[pi] == '"') {
                        pi += 1;
                    }
                } else {
                    // Unquoted token parameter
                    const val_start = pi;
                    while (pi < params_str.len && params_str[pi] != ';') : (pi += 1) {}
                    let val_end = pi;
                    while (val_end > val_start && isHttpWhitespace(params_str[val_end - 1])) : (val_end -= 1) {}
                    val_slice = params_str[val_start..val_end];
                }

                if (std.ascii.eqlIgnoreCase(param_name, "charset")) {
                    if (charset_slice.len == 0) {
                        charset_slice = val_slice;
                    }
                }
            }
        }

        return MimeType{
            raw: raw_str,
            essence: essence,
            charset: charset_slice,
        };
    }
}

pub fn parseMime(raw_str: []const u8): MimeType? {
    return MimeType.parse(raw_str);
}
