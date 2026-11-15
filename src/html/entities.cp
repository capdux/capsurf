// src/html/entities.cp
// CapSurf W3C HTML5 Named and Numeric Character Entity Reference Decoder
// Zero-allocation decoding of HTML entities with attribute context protection.

pub struct DecodedCharRef {
    codepoints: [2]u32 = [0, 0];
    len: u8 = 0;
    bytes_consumed: usize = 0;

    fn isSingle(): bool {
        return _.len == 1;
    }

    /// Encodes the decoded codepoints into a UTF-8 byte slice in `buf`.
    /// `buf` must have at least 8 bytes of space. Returns number of bytes written.
    fn toUtf8(buf: []u8): usize {
        let written: usize = 0;
        let i: u8 = 0;
        while (i < _.len) : (i += 1) {
            const cp = _.codepoints[@as(usize, i)];
            written += encodeCodepointUtf8(cp, buf[written..]);
        }
        return written;
    }
}

/// Encodes a single Unicode codepoint into UTF-8 in `out_buf` (at least 4 bytes).
pub fn encodeCodepointUtf8(cp: u32, out_buf: []u8): usize {
    if (cp <= 0x7F) {
        out_buf[0] = @intCast(cp);
        return 1;
    } else if (cp <= 0x7FF) {
        out_buf[0] = @intCast(0xC0 | ((cp >> 6) & 0x1F));
        out_buf[1] = @intCast(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp <= 0xFFFF) {
        out_buf[0] = @intCast(0xE0 | ((cp >> 12) & 0x0F));
        out_buf[1] = @intCast(0x80 | ((cp >> 6) & 0x3F));
        out_buf[2] = @intCast(0x80 | (cp & 0x3F));
        return 3;
    } else if (cp <= 0x10FFFF) {
        out_buf[0] = @intCast(0xF0 | ((cp >> 18) & 0x07));
        out_buf[1] = @intCast(0x80 | ((cp >> 12) & 0x3F));
        out_buf[2] = @intCast(0x80 | ((cp >> 6) & 0x3F));
        out_buf[3] = @intCast(0x80 | (cp & 0x3F));
        return 4;
    }
    // Replacement character U+FFFD (0xEF, 0xBF, 0xBD)
    out_buf[0] = 0xEF;
    out_buf[1] = 0xBF;
    out_buf[2] = 0xBD;
    return 3;
}

/// Maps Windows-1252 0x80..0x9F codepoints to correct Unicode per HTML5 spec 13.2.5.72.
fn mapWindows1252(cp: u32): u32 {
    return switch (cp) {
        0x80 => 0x20AC, // Euro
        0x82 => 0x201A, // Single Low-9 Quotation Mark
        0x83 => 0x0192, // Latin Small Letter F with Hook
        0x84 => 0x201E, // Double Low-9 Quotation Mark
        0x85 => 0x2026, // Horizontal Ellipsis
        0x86 => 0x2020, // Dagger
        0x87 => 0x2021, // Double Dagger
        0x88 => 0x02C6, // Modifier Letter Circumflex Accent
        0x89 => 0x2030, // Per Mille Sign
        0x8A => 0x0160, // Latin Capital Letter S with Caron
        0x8B => 0x2039, // Single Left-Pointing Angle Quotation Mark
        0x8C => 0x0152, // Latin Capital Ligature OE
        0x8E => 0x017D, // Latin Capital Letter Z with Caron
        0x91 => 0x2018, // Left Single Quotation Mark
        0x92 => 0x2019, // Right Single Quotation Mark
        0x93 => 0x201C, // Left Double Quotation Mark
        0x94 => 0x201D, // Right Double Quotation Mark
        0x95 => 0x2022, // Bullet
        0x96 => 0x2013, // En Dash
        0x97 => 0x2014, // Em Dash
        0x98 => 0x02DC, // Small Tilde
        0x99 => 0x2122, // Trade Mark Sign
        0x9A => 0x0161, // Latin Small Letter S with Caron
        0x9B => 0x203A, // Single Right-Pointing Angle Quotation Mark
        0x9C => 0x0153, // Latin Small Ligature OE
        0x9E => 0x017E, // Latin Small Letter Z with Caron
        0x9F => 0x0178, // Latin Capital Letter Y with Diaeresis
        * => cp,
    };
}

/// Decodes an entity starting with '&' at `slice[0]`.
/// Returns `DecodedCharRef` with Unicode codepoint and total byte length consumed (including '&' and ';').
pub fn decodeEntity(slice: []const u8, in_attribute: bool): DecodedCharRef? {
    if (slice.len < 3 || slice[0] != '&') return null;

    // 1. Numeric character references: &#1234; or &#xABCD;
    if (slice[1] == '#') {
        let is_hex = false;
        let start_idx: usize = 2;
        if (slice.len >= 4 && (slice[2] == 'x' || slice[2] == 'X')) {
            is_hex = true;
            start_idx = 3;
        }

        let num: u32 = 0;
        let idx = start_idx;
        while (idx < slice.len) : (idx += 1) {
            const ch = slice[idx];
            if (ch == ';') {
                break;
            }
            if (is_hex) {
                if (ch >= '0' && ch <= '9') {
                    num = (num << 4) | @as(u32, ch - '0');
                } else if (ch >= 'a' && ch <= 'f') {
                    num = (num << 4) | @as(u32, ch - 'a' + 10);
                } else if (ch >= 'A' && ch <= 'F') {
                    num = (num << 4) | @as(u32, ch - 'A' + 10);
                } else {
                    break;
                }
            } else {
                if (ch >= '0' && ch <= '9') {
                    num = (num * 10) + @as(u32, ch - '0');
                } else {
                    break;
                }
            }
            if (num > 0x10FFFF) {
                num = 0xFFFD;
            }
        }

        if (idx == start_idx) return null; // No digits found

        let consumed = idx;
        if (idx < slice.len && slice[idx] == ';') {
            consumed += 1;
        }

        // Apply HTML5 numeric character reference rules
        if (num == 0 || (num >= 0xD800 && num <= 0xDFFF) || num > 0x10FFFF) {
            num = 0xFFFD;
        } else if (num >= 0x80 && num <= 0x9F) {
            num = mapWindows1252(num);
        }

        return DecodedCharRef {
            codepoints: [num, 0],
            len: 1,
            bytes_consumed: consumed,
        };
    }

    // 2. High-frequency named entities (O(1) direct matching)
    // Extract up to 16 bytes for matching
    const max_len = if (slice.len > 16) 16 else slice.len;
    const cand = slice[0..max_len];

    // Priority 1: 5 essential XML entities
    if (std.mem.startsWith(u8, cand, "&amp;")) {
        return DecodedCharRef { codepoints: ['&', 0], len: 1, bytes_consumed: 5 };
    }
    if (std.mem.startsWith(u8, cand, "&lt;")) {
        return DecodedCharRef { codepoints: ['<', 0], len: 1, bytes_consumed: 4 };
    }
    if (std.mem.startsWith(u8, cand, "&gt;")) {
        return DecodedCharRef { codepoints: ['>', 0], len: 1, bytes_consumed: 4 };
    }
    if (std.mem.startsWith(u8, cand, "&quot;")) {
        return DecodedCharRef { codepoints: ['"', 0], len: 1, bytes_consumed: 6 };
    }
    if (std.mem.startsWith(u8, cand, "&apos;")) {
        return DecodedCharRef { codepoints: ['\'', 0], len: 1, bytes_consumed: 6 };
    }
    if (std.mem.startsWith(u8, cand, "&nbsp;")) {
        return DecodedCharRef { codepoints: [0x00A0, 0], len: 1, bytes_consumed: 6 };
    }

    // Priority 2: Frequent web publishing entities
    if (std.mem.startsWith(u8, cand, "&copy;")) {
        return DecodedCharRef { codepoints: [0x00A9, 0], len: 1, bytes_consumed: 6 };
    }
    if (std.mem.startsWith(u8, cand, "&reg;")) {
        return DecodedCharRef { codepoints: [0x00AE, 0], len: 1, bytes_consumed: 5 };
    }
    if (std.mem.startsWith(u8, cand, "&trade;")) {
        return DecodedCharRef { codepoints: [0x2122, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&deg;")) {
        return DecodedCharRef { codepoints: [0x00B0, 0], len: 1, bytes_consumed: 5 };
    }
    if (std.mem.startsWith(u8, cand, "&plusmn;")) {
        return DecodedCharRef { codepoints: [0x00B1, 0], len: 1, bytes_consumed: 8 };
    }
    if (std.mem.startsWith(u8, cand, "&times;")) {
        return DecodedCharRef { codepoints: [0x00D7, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&divide;")) {
        return DecodedCharRef { codepoints: [0x00F7, 0], len: 1, bytes_consumed: 8 };
    }
    if (std.mem.startsWith(u8, cand, "&cent;")) {
        return DecodedCharRef { codepoints: [0x00A2, 0], len: 1, bytes_consumed: 6 };
    }
    if (std.mem.startsWith(u8, cand, "&pound;")) {
        return DecodedCharRef { codepoints: [0x00A3, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&yen;")) {
        return DecodedCharRef { codepoints: [0x00A5, 0], len: 1, bytes_consumed: 5 };
    }
    if (std.mem.startsWith(u8, cand, "&euro;")) {
        return DecodedCharRef { codepoints: [0x20AC, 0], len: 1, bytes_consumed: 6 };
    }
    if (std.mem.startsWith(u8, cand, "&sect;")) {
        return DecodedCharRef { codepoints: [0x00A7, 0], len: 1, bytes_consumed: 6 };
    }
    if (std.mem.startsWith(u8, cand, "&micro;")) {
        return DecodedCharRef { codepoints: [0x00B5, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&para;")) {
        return DecodedCharRef { codepoints: [0x00B6, 0], len: 1, bytes_consumed: 6 };
    }
    if (std.mem.startsWith(u8, cand, "&middot;")) {
        return DecodedCharRef { codepoints: [0x00B7, 0], len: 1, bytes_consumed: 8 };
    }
    if (std.mem.startsWith(u8, cand, "&bull;")) {
        return DecodedCharRef { codepoints: [0x2022, 0], len: 1, bytes_consumed: 6 };
    }
    if (std.mem.startsWith(u8, cand, "&hellip;")) {
        return DecodedCharRef { codepoints: [0x2026, 0], len: 1, bytes_consumed: 8 };
    }
    if (std.mem.startsWith(u8, cand, "&prime;")) {
        return DecodedCharRef { codepoints: [0x2032, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&Prime;")) {
        return DecodedCharRef { codepoints: [0x2033, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&mdash;")) {
        return DecodedCharRef { codepoints: [0x2014, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&ndash;")) {
        return DecodedCharRef { codepoints: [0x2013, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&lsquo;")) {
        return DecodedCharRef { codepoints: [0x2018, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&rsquo;")) {
        return DecodedCharRef { codepoints: [0x2019, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&ldquo;")) {
        return DecodedCharRef { codepoints: [0x201C, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&rdquo;")) {
        return DecodedCharRef { codepoints: [0x201D, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&laquo;")) {
        return DecodedCharRef { codepoints: [0x00AB, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&raquo;")) {
        return DecodedCharRef { codepoints: [0x00BB, 0], len: 1, bytes_consumed: 7 };
    }
    if (std.mem.startsWith(u8, cand, "&larr;")) {
        return DecodedCharRef { codepoints: [0x2190, 0], len: 1, bytes_consumed: 6 };
    }
    if (std.mem.startsWith(u8, cand, "&rarr;")) {
        return DecodedCharRef { codepoints: [0x2192, 0], len: 1, bytes_consumed: 6 };
    }
    if (std.mem.startsWith(u8, cand, "&uarr;")) {
        return DecodedCharRef { codepoints: [0x2191, 0], len: 1, bytes_consumed: 6 };
    }
    if (std.mem.startsWith(u8, cand, "&darr;")) {
        return DecodedCharRef { codepoints: [0x2193, 0], len: 1, bytes_consumed: 6 };
    }

    // 3. Legacy semicolon-less entities (e.g. &amp, &lt, &gt, &quot, &copy)
    // HTML5 Spec 13.2.5.72: in attribute value, if entity has no trailing semicolon
    // and the next char is '=' or alphanumeric, do NOT decode it.
    if (!in_attribute || (cand.len > 4 && cand[4] != '=' && !std.ascii.isAlphanumeric(cand[4]))) {
        if (std.mem.startsWith(u8, cand, "&amp")) {
            return DecodedCharRef { codepoints: ['&', 0], len: 1, bytes_consumed: 4 };
        }
    }
    if (!in_attribute || (cand.len > 3 && cand[3] != '=' && !std.ascii.isAlphanumeric(cand[3]))) {
        if (std.mem.startsWith(u8, cand, "&lt")) {
            return DecodedCharRef { codepoints: ['<', 0], len: 1, bytes_consumed: 3 };
        }
        if (std.mem.startsWith(u8, cand, "&gt")) {
            return DecodedCharRef { codepoints: ['>', 0], len: 1, bytes_consumed: 3 };
        }
    }
    if (!in_attribute || (cand.len > 5 && cand[5] != '=' && !std.ascii.isAlphanumeric(cand[5]))) {
        if (std.mem.startsWith(u8, cand, "&quot")) {
            return DecodedCharRef { codepoints: ['"', 0], len: 1, bytes_consumed: 5 };
        }
        if (std.mem.startsWith(u8, cand, "&copy")) {
            return DecodedCharRef { codepoints: [0x00A9, 0], len: 1, bytes_consumed: 5 };
        }
        if (std.mem.startsWith(u8, cand, "&nbsp")) {
            return DecodedCharRef { codepoints: [0x00A0, 0], len: 1, bytes_consumed: 5 };
        }
    }

    return null;
}
