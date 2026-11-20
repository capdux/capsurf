// src/css/paged_media.cp
// Pure CP CSS Paged Media (@page margins), Fragmentation (break-before/after/inside),
// and Legacy Charset (ISO-8859-15 / Latin-1 / Windows-1252) CSS Byte Decoding.

import { CssProperty } from "cap:css";

pub enum BreakValue {
    Auto,
    Page,
    Avoid,
    Always,
    Left,
    Right,
    Column,

    static fn parse(raw: []const u8): BreakValue? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        if (std.ascii.eqlIgnoreCase(s, "auto")) return BreakValue.Auto;
        if (std.ascii.eqlIgnoreCase(s, "page")) return BreakValue.Page;
        if (std.ascii.eqlIgnoreCase(s, "always")) return BreakValue.Always;
        if (std.ascii.eqlIgnoreCase(s, "avoid") || std.ascii.eqlIgnoreCase(s, "avoid-page")) return BreakValue.Avoid;
        if (std.ascii.eqlIgnoreCase(s, "left")) return BreakValue.Left;
        if (std.ascii.eqlIgnoreCase(s, "right")) return BreakValue.Right;
        if (std.ascii.eqlIgnoreCase(s, "column")) return BreakValue.Column;
        return null;
    }

    fn forcesPageBreak(): bool {
        return _ == BreakValue.Page || _ == BreakValue.Always || _ == BreakValue.Left || _ == BreakValue.Right;
    }
}

pub enum BreakInsideValue {
    Auto,
    Avoid,

    static fn parse(raw: []const u8): BreakInsideValue? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        if (std.ascii.eqlIgnoreCase(s, "auto")) return BreakInsideValue.Auto;
        if (std.ascii.eqlIgnoreCase(s, "avoid") ||
            std.ascii.eqlIgnoreCase(s, "avoid-page") ||
            std.ascii.eqlIgnoreCase(s, "avoid-column")) {
            return BreakInsideValue.Avoid;
        }
        return null;
    }
}

pub struct PageMarginRule {
    has_margin_top: bool = false;
    has_margin_right: bool = false;
    has_margin_bottom: bool = false;
    has_margin_left: bool = false;
    margin_top_px: f32 = 0.0;
    margin_right_px: f32 = 0.0;
    margin_bottom_px: f32 = 0.0;
    margin_left_px: f32 = 0.0;

    static fn parseLengthPx(raw: []const u8): f32? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        if (std.mem.eql(u8, s, "0")) return 0.0;

        let num_end: usize = 0;
        if (num_end < s.len && (s[num_end] == '+' || s[num_end] == '-')) {
            num_end += 1;
        }
        let has_digit = false;
        while (num_end < s.len) : (num_end += 1) {
            const c = s[num_end];
            if (c >= '0' && c <= '9') {
                has_digit = true;
            } else if (c == '.') {
                // decimal point
            } else {
                break;
            }
        }
        if (!has_digit) return null;
        const val = std.fmt.parseFloat(f32, s[0..num_end]) catch return null;
        const unit = std.mem.trim(u8, s[num_end..], " \t\r\n");
        if (unit.len == 0) {
            if (val == 0.0) return 0.0;
            return null;
        }
        if (std.ascii.eqlIgnoreCase(unit, "px")) return val;
        if (std.ascii.eqlIgnoreCase(unit, "in")) return val * 96.0;
        if (std.ascii.eqlIgnoreCase(unit, "cm")) return val * (96.0 / 2.54);
        if (std.ascii.eqlIgnoreCase(unit, "mm")) return val * (96.0 / 25.4);
        if (std.ascii.eqlIgnoreCase(unit, "pt")) return val * (96.0 / 72.0);
        if (std.ascii.eqlIgnoreCase(unit, "pc")) return val * 16.0;
        if (std.ascii.eqlIgnoreCase(unit, "q")) return val * (96.0 / 101.6);
        return null;
    }

    fn applyDecl(*_, prop: *const CssProperty) {
        const name = std.mem.trim(u8, prop.name, " \t\r\n");
        const val = std.mem.trim(u8, prop.value, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(name, "margin")) {
            let vals: [4]f32 = [0.0; 4];
            let cnt: usize = 0;
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n");
            while (tok := it.next()) {
                if (cnt >= 4) break;
                const px = PageMarginRule.parseLengthPx(tok) ?? return;
                vals[cnt] = px;
                cnt += 1;
            }
            if (cnt == 1) {
                _.margin_top_px = vals[0];
                _.margin_right_px = vals[0];
                _.margin_bottom_px = vals[0];
                _.margin_left_px = vals[0];
            } else if (cnt == 2) {
                _.margin_top_px = vals[0];
                _.margin_bottom_px = vals[0];
                _.margin_right_px = vals[1];
                _.margin_left_px = vals[1];
            } else if (cnt == 3) {
                _.margin_top_px = vals[0];
                _.margin_right_px = vals[1];
                _.margin_left_px = vals[1];
                _.margin_bottom_px = vals[2];
            } else if (cnt == 4) {
                _.margin_top_px = vals[0];
                _.margin_right_px = vals[1];
                _.margin_bottom_px = vals[2];
                _.margin_left_px = vals[3];
            } else {
                return;
            }
            _.has_margin_top = true;
            _.has_margin_right = true;
            _.has_margin_bottom = true;
            _.has_margin_left = true;
        } else if (std.ascii.eqlIgnoreCase(name, "margin-top")) {
            if (px := PageMarginRule.parseLengthPx(val)) {
                _.margin_top_px = px;
                _.has_margin_top = true;
            }
        } else if (std.ascii.eqlIgnoreCase(name, "margin-right")) {
            if (px := PageMarginRule.parseLengthPx(val)) {
                _.margin_right_px = px;
                _.has_margin_right = true;
            }
        } else if (std.ascii.eqlIgnoreCase(name, "margin-bottom")) {
            if (px := PageMarginRule.parseLengthPx(val)) {
                _.margin_bottom_px = px;
                _.has_margin_bottom = true;
            }
        } else if (std.ascii.eqlIgnoreCase(name, "margin-left")) {
            if (px := PageMarginRule.parseLengthPx(val)) {
                _.margin_left_px = px;
                _.has_margin_left = true;
            }
        }
    }
}

/// Checks if `bytes` is valid UTF-8.
pub fn isValidUtf8(bytes: []const u8): bool {
    let i: usize = 0;
    while (i < bytes.len) {
        const b = bytes[i];
        if (b < 0x80) {
            i += 1;
        } else if (b >= 0xC2 && b <= 0xDF) {
            if (i + 1 >= bytes.len) return false;
            if ((bytes[i + 1] & 0xC0) != 0x80) return false;
            i += 2;
        } else if (b >= 0xE0 && b <= 0xEF) {
            if (i + 2 >= bytes.len) return false;
            const b1 = bytes[i + 1];
            const b2 = bytes[i + 2];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) return false;
            if (b == 0xE0 && b1 < 0xA0) return false;
            if (b == 0xED && b1 >= 0xA0) return false;
            i += 3;
        } else if (b >= 0xF0 && b <= 0xF4) {
            if (i + 3 >= bytes.len) return false;
            const b1 = bytes[i + 1];
            const b2 = bytes[i + 2];
            const b3 = bytes[i + 3];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) return false;
            if (b == 0xF0 && b1 < 0x90) return false;
            if (b == 0xF4 && b1 >= 0x90) return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

/// Transcodes raw CSS bytes into UTF-8.
/// If already valid UTF-8, copies directly; otherwise decodes as ISO-8859-15 / Latin-1.
pub fn decodeLegacyCssToUtf8(src: []const u8, dst: []u8): usize {
    if (isValidUtf8(src)) {
        const n = @min(src.len, dst.len);
        @memcpy(dst[0..n], src[0..n]);
        return n;
    }
    let out: usize = 0;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const b = src[i];
        if (b < 0x80) {
            if (out >= dst.len) break;
            dst[out] = b;
            out += 1;
        } else {
            const cp: u21 = switch (b) {
                0xA4 => 0x20AC, // €
                0xA6 => 0x0160, // Š
                0xA8 => 0x0161, // š
                0xB4 => 0x017D, // Ž
                0xB8 => 0x017E, // ž
                0xBC => 0x0152, // Œ
                0xBD => 0x0153, // œ
                0xBE => 0x0178, // Ÿ
                * => @as(u21, b),
            };
            if (cp < 0x800) {
                if (out + 2 > dst.len) break;
                dst[out] = @as(u8, @intCast(0xC0 | (cp >> 6)));
                dst[out + 1] = @as(u8, @intCast(0x80 | (cp & 0x3F)));
                out += 2;
            } else {
                if (out + 3 > dst.len) break;
                dst[out] = @as(u8, @intCast(0xE0 | (cp >> 12)));
                dst[out + 1] = @as(u8, @intCast(0x80 | ((cp >> 6) & 0x3F)));
                dst[out + 2] = @as(u8, @intCast(0x80 | (cp & 0x3F)));
                out += 3;
            }
        }
    }
    return out;
}
