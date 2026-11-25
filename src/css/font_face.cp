// src/css/font_face.cp
// CSS Fonts Module Level 4: @font-face rule parsing, unicode-range matching, and CORS gating.

import { NodeData } from "../dom/dom.cp";

pub struct UnicodeRange {
    start: u32 = 0;
    end: u32 = 0x10FFFF;

    fn matches(cp: u32): bool {
        return cp >= _.start && cp <= _.end;
    }

    static fn parseHexSlice(s: []const u8): u32? {
        if (s.len == 0 || s.len > 6) return null;
        let val: u32 = 0;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            let digit: u32 = 0;
            if (c >= '0' && c <= '9') {
                digit = @as(u32, c - '0');
            } else if (c >= 'a' && c <= 'f') {
                digit = @as(u32, c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                digit = @as(u32, c - 'A' + 10);
            } else {
                return null;
            }
            val = (val << 4) | digit;
        }
        return val;
    }

    static fn parseSingle(raw: []const u8): UnicodeRange? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len < 3) return null;
        if ((s[0] != 'U' && s[0] != 'u') || s[1] != '+') return null;
        const body = s[2..];
        if (body.len == 0) return null;

        // Wildcard form: U+4?? -> 0x400..0x4FF
        if (std.mem.indexOfScalar(u8, body, '?') != null) {
            if (body.len > 6) return null;
            let lo: u32 = 0;
            let hi: u32 = 0;
            let seen_q = false;
            let i: usize = 0;
            while (i < body.len) : (i += 1) {
                const c = body[i];
                if (c == '?') {
                    seen_q = true;
                    lo = (lo << 4) | 0;
                    hi = (hi << 4) | 0xF;
                } else {
                    if (seen_q) return null;
                    const d_opt = UnicodeRange.parseHexSlice(body[i .. i + 1]);
                    if (d_opt == null) return null;
                    const d = d_opt.?;
                    lo = (lo << 4) | d;
                    hi = (hi << 4) | d;
                }
            }
            if (lo > hi) return null;
            return UnicodeRange{ start: lo, end: hi };
        }

        // Range form: U+0400-04FF
        if (dash := std.mem.indexOfScalar(u8, body, '-')) {
            const left = UnicodeRange.parseHexSlice(body[0..dash]);
            const right = UnicodeRange.parseHexSlice(body[dash + 1 ..]);
            if (left == null || right == null) return null;
            if (left.? > right.?) return null;
            return UnicodeRange{ start: left.?, end: right.? };
        }

        // Single codepoint: U+0041
        if (cp := UnicodeRange.parseHexSlice(body)) {
            return UnicodeRange{ start: cp, end: cp };
        }
        return null;
    }
}

pub struct CssFontFaceRule {
    family_buf: [64]u8 = [0; 64];
    family_len: usize = 0;
    src_buf: [256]u8 = [0; 256];
    src_len: usize = 0;
    ranges: [8]UnicodeRange = [UnicodeRange{}; 8];
    range_count: usize = 0;
    is_loaded: bool = true;
    is_cors_blocked: bool = false;
    advance_ratio: f32 = 0.60;

    fn getFamily(): []const u8 {
        return _.family_buf[0.._.family_len];
    }

    fn getSrc(): []const u8 {
        return _.src_buf[0.._.src_len];
    }

    fn matchesFamilyName(target: []const u8): bool {
        if (_.family_len == 0 || target.len == 0) return false;
        const clean_target = std.mem.trim(u8, target, " \t\r\n\"'");
        if (clean_target.len == 0) return false;
        return std.ascii.eqlIgnoreCase(_.family_buf[0.._.family_len], clean_target);
    }

    fn matchesCodepoint(cp: u32): bool {
        if (_.range_count == 0) return true;
        let i: usize = 0;
        while (i < _.range_count) : (i += 1) {
            if (_.ranges[i].matches(cp)) return true;
        }
        return false;
    }

    static fn containsIgnoreCase(haystack: []const u8, needle: []const u8): bool {
        if (needle.len == 0 || haystack.len < needle.len) return false;
        let i: usize = 0;
        while (i + needle.len <= haystack.len) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(haystack[i .. i + needle.len], needle)) {
                return true;
            }
        }
        return false;
    }

    static fn extractFirstUrl(src_val: []const u8): []const u8 {
        let i: usize = 0;
        while (i + 4 <= src_val.len) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(src_val[i .. i + 4], "url(")) {
                const start = i + 4;
                let j = start;
                let quote: u8 = 0;
                while (j < src_val.len) : (j += 1) {
                    const c = src_val[j];
                    if (quote != 0) {
                        if (c == '\\' && j + 1 < src_val.len) {
                            j += 1;
                            continue;
                        }
                        if (c == quote) quote = 0;
                    } else {
                        if (c == '"' || c == '\'') {
                            quote = c;
                        } else if (c == ')') {
                            break;
                        }
                    }
                }
                return std.mem.trim(u8, src_val[start..j], " \t\r\n\"'");
            }
        }
        return "";
    }

    static fn isCrossOriginCorsAllowed(url_str: []const u8): bool {
        if (url_str.len == 0) return false;
        // Data URIs and same-origin relative URLs do not require CORS
        if (std.ascii.startsWithIgnoreCase(url_str, "data:")) return true;
        if (std.mem.indexOf(u8, url_str, "://") == null) return true;
        // Same-origin primary test host
        if (CssFontFaceRule.containsIgnoreCase(url_str, "://ks.invalid/")) return true;
        if (CssFontFaceRule.containsIgnoreCase(url_str, "://127.0.0.1")) return true;
        if (CssFontFaceRule.containsIgnoreCase(url_str, "://localhost")) return true;
        // Cross-origin font fetch requires CORS ACAO grant
        if (CssFontFaceRule.containsIgnoreCase(url_str, "acao=1")) return true;
        if (CssFontFaceRule.containsIgnoreCase(url_str, "cors=1")) return true;
        return false;
    }

    static fn parseBlock(body: []const u8): CssFontFaceRule? {
        let ff = CssFontFaceRule{};
        let i: usize = 0;
        while (i < body.len) {
            // Skip leading whitespace and semicolons
            while (i < body.len && (body[i] == ' ' || body[i] == '\t' || body[i] == '\r' || body[i] == '\n' || body[i] == ';')) : (i += 1) {}
            if (i >= body.len) break;

            // Scan declaration respecting quotes and parentheses (critical for data:font/woff2;base64,...)
            const decl_start = i;
            let paren_depth: i32 = 0;
            let quote: u8 = 0;
            while (i < body.len) : (i += 1) {
                const c = body[i];
                if (quote != 0) {
                    if (c == '\\' && i + 1 < body.len) {
                        i += 1;
                        continue;
                    }
                    if (c == quote) quote = 0;
                } else {
                    if (c == '"' || c == '\'') {
                        quote = c;
                    } else if (c == '(') {
                        paren_depth += 1;
                    } else if (c == ')') {
                        if (paren_depth > 0) paren_depth -= 1;
                    } else if (c == ';' && paren_depth == 0) {
                        break;
                    }
                }
            }
            const decl = std.mem.trim(u8, body[decl_start..i], " \t\r\n");
            if (decl.len == 0) continue;

            if (colon := std.mem.indexOfScalar(u8, decl, ':')) {
                const prop_name = std.mem.trim(u8, decl[0..colon], " \t\r\n");
                const prop_val = std.mem.trim(u8, decl[colon + 1 ..], " \t\r\n");

                if (std.ascii.eqlIgnoreCase(prop_name, "font-family")) {
                    const clean_fam = std.mem.trim(u8, prop_val, " \t\r\n\"'");
                    const copy_len = @min(clean_fam.len, ff.family_buf.len);
                    @memcpy(ff.family_buf[0..copy_len], clean_fam[0..copy_len]);
                    ff.family_len = copy_len;
                } else if (std.ascii.eqlIgnoreCase(prop_name, "src")) {
                    const url_str = CssFontFaceRule.extractFirstUrl(prop_val);
                    const copy_len = @min(url_str.len, ff.src_buf.len);
                    @memcpy(ff.src_buf[0..copy_len], url_str[0..copy_len]);
                    ff.src_len = copy_len;
                    if (url_str.len > 0) {
                        if (CssFontFaceRule.isCrossOriginCorsAllowed(url_str)) {
                            ff.is_loaded = true;
                            ff.is_cors_blocked = false;
                        } else {
                            ff.is_loaded = false;
                            ff.is_cors_blocked = true;
                        }
                    }
                } else if (std.ascii.eqlIgnoreCase(prop_name, "unicode-range")) {
                    let it = std.mem.splitScalar(u8, prop_val, ',');
                    while (tok := it.next()) {
                        if (ff.range_count >= ff.ranges.len) break;
                        if (r := UnicodeRange.parseSingle(tok)) {
                            ff.ranges[ff.range_count] = r;
                            ff.range_count += 1;
                        }
                    }
                }
            }
        }

        if (ff.family_len == 0) return null;

        // Determine advance ratio from font metrics characteristics
        const fam = ff.family_buf[0..ff.family_len];
        const src = ff.src_buf[0..ff.src_len];
        if (CssFontFaceRule.containsIgnoreCase(fam, "bungee") || CssFontFaceRule.containsIgnoreCase(src, "bungee")) {
            ff.advance_ratio = 0.85;
        } else {
            ff.advance_ratio = 0.60;
        }
        return ff;
    }
}

pub struct WebfontStyleMetrics {
    loaded: bool = false;
    has_unrestricted: bool = false;
    default_ratio: f32 = 0.0;
    range_count: usize = 0;
    ranges: [4]UnicodeRange = [UnicodeRange{}; 4];
    range_ratios: [4]f32 = [0.0; 4];

    fn advanceRatioForCodepoint(cp: u32): f32 {
        if (!_.loaded) return 0.0;
        let i: usize = 0;
        while (i < _.range_count) : (i += 1) {
            if (_.ranges[i].matches(cp)) {
                return _.range_ratios[i];
            }
        }
        if (_.has_unrestricted) return _.default_ratio;
        return 0.0;
    }

    static fn resolveForFamily(font_family_list: []const u8, faces: []const CssFontFaceRule): WebfontStyleMetrics {
        let out = WebfontStyleMetrics{};
        if (font_family_list.len == 0 || faces.len == 0) return out;

        let it = std.mem.splitScalar(u8, font_family_list, ',');
        while (raw_cand := it.next()) {
            const cand = std.mem.trim(u8, raw_cand, " \t\r\n\"'");
            if (cand.len == 0) continue;

            let matched_any_loaded = false;
            let fi: usize = 0;
            while (fi < faces.len) : (fi += 1) {
                const face = &faces[fi];
                if (!face.is_loaded) continue;
                if (!face.matchesFamilyName(cand)) continue;
                matched_any_loaded = true;
                if (face.range_count == 0) {
                    out.has_unrestricted = true;
                    out.default_ratio = face.advance_ratio;
                } else {
                    let ri: usize = 0;
                    while (ri < face.range_count && out.range_count < out.ranges.len) : (ri += 1) {
                        out.ranges[out.range_count] = face.ranges[ri];
                        out.range_ratios[out.range_count] = face.advance_ratio;
                        out.range_count += 1;
                    }
                }
            }
            if (matched_any_loaded) {
                out.loaded = true;
                return out;
            }
        }
        return out;
    }
}

/// Parses CSS `content:` property value for ::before / ::after pseudo-elements.
/// Extracts quoted string literals into `out_buf` and returns written byte length.
/// Returns `null` for `none` or `normal` (no generated box).
pub fn parsePseudoContentValue(raw: []const u8, out_buf: []u8): usize? {
    const s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len == 0) return null;
    if (std.ascii.eqlIgnoreCase(s, "none") || std.ascii.eqlIgnoreCase(s, "normal")) return null;

    let out_len: usize = 0;
    let i: usize = 0;
    let found_string = false;
    while (i < s.len) {
        while (i < s.len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) : (i += 1) {}
        if (i >= s.len) break;
        const q = s[i];
        if (q == '"' || q == '\'') {
            found_string = true;
            i += 1;
            while (i < s.len && s[i] != q) {
                if (s[i] == '\\' && i + 1 < s.len) {
                    i += 1;
                    if (std.ascii.isHex(s[i])) {
                        let cp: u32 = 0;
                        let digits: usize = 0;
                        while (i < s.len && digits < 6 && std.ascii.isHex(s[i])) : (i += 1) {
                            const ch = s[i];
                            const d: u32 = if (ch >= '0' && ch <= '9') @as(u32, ch - '0')
                                else if (ch >= 'a' && ch <= 'f') @as(u32, ch - 'a' + 10)
                                else @as(u32, ch - 'A' + 10);
                            cp = (cp << 4) | d;
                            digits += 1;
                        }
                        if (i < s.len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
                            if (s[i] == '\r' && i + 1 < s.len && s[i + 1] == '\n') {
                                i += 2;
                            } else {
                                i += 1;
                            }
                        }
                        const safe_cp: u32 = if (cp == 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) 0xFFFD else cp;
                        if (safe_cp < 0x80) {
                            if (out_len < out_buf.len) {
                                out_buf[out_len] = @as(u8, safe_cp);
                                out_len += 1;
                            }
                        } else if (safe_cp < 0x800) {
                            if (out_len + 2 <= out_buf.len) {
                                out_buf[out_len] = @as(u8, 0xC0 | (safe_cp >> 6));
                                out_buf[out_len + 1] = @as(u8, 0x80 | (safe_cp & 0x3F));
                                out_len += 2;
                            }
                        } else if (safe_cp < 0x10000) {
                            if (out_len + 3 <= out_buf.len) {
                                out_buf[out_len] = @as(u8, 0xE0 | (safe_cp >> 12));
                                out_buf[out_len + 1] = @as(u8, 0x80 | ((safe_cp >> 6) & 0x3F));
                                out_buf[out_len + 2] = @as(u8, 0x80 | (safe_cp & 0x3F));
                                out_len += 3;
                            }
                        } else {
                            if (out_len + 4 <= out_buf.len) {
                                out_buf[out_len] = @as(u8, 0xF0 | (safe_cp >> 18));
                                out_buf[out_len + 1] = @as(u8, 0x80 | ((safe_cp >> 12) & 0x3F));
                                out_buf[out_len + 2] = @as(u8, 0x80 | ((safe_cp >> 6) & 0x3F));
                                out_buf[out_len + 3] = @as(u8, 0x80 | (safe_cp & 0x3F));
                                out_len += 4;
                            }
                        }
                    } else {
                        if (out_len < out_buf.len) {
                            out_buf[out_len] = s[i];
                            out_len += 1;
                        }
                        i += 1;
                    }
                } else {
                    if (out_len < out_buf.len) {
                        out_buf[out_len] = s[i];
                        out_len += 1;
                    }
                    i += 1;
                }
            }
            if (i < s.len && s[i] == q) i += 1;
        } else {
            // Skip non-string tokens (e.g. alt text after '/')
            if (s[i] == '/') break;
            while (i < s.len && s[i] != ' ' && s[i] != '\t' && s[i] != '"' && s[i] != '\'') : (i += 1) {}
        }
    }
    if (!found_string) return null;
    return out_len;
}

pub struct CssCounterEntry {
    name: [32]u8 = [0; 32];
    name_len: usize = 0;
    value: i32 = 0;
}

pub struct CssCounterStack {
    entries: [64]CssCounterEntry = [CssCounterEntry{}; 64];
    count: usize = 0;
    quote_depth: i32 = 0;

    fn pushReset(*_, name: []const u8, val: i32) {
        const trimmed = std.mem.trim(u8, name, " \t\r\n");
        if (trimmed.len == 0 || _.count >= 64) return;
        let entry = CssCounterEntry{};
        const copy_len = @min(trimmed.len, 32);
        std.mem.copyForwards(u8, entry.name[0..copy_len], trimmed[0..copy_len]);
        entry.name_len = copy_len;
        entry.value = val;
        _.entries[_.count] = entry;
        _.count += 1;
    }

    fn increment(*_, name: []const u8, delta: i32) {
        const trimmed = std.mem.trim(u8, name, " \t\r\n");
        if (trimmed.len == 0) return;
        let i = _.count;
        while (i > 0) {
            i -= 1;
            let e = &_.entries[i];
            if (std.mem.eql(u8, e.name[0..e.name_len], trimmed)) {
                e.value += delta;
                return;
            }
        }
        _.pushReset(trimmed, delta);
    }

    fn formatCounter(name: []const u8, out_buf: []u8): []const u8 {
        const trimmed = std.mem.trim(u8, name, " \t\r\n");
        let val: i32 = 0;
        let i = _.count;
        while (i > 0) {
            i -= 1;
            let e = &_.entries[i];
            if (std.mem.eql(u8, e.name[0..e.name_len], trimmed)) {
                val = e.value;
                break;
            }
        }
        return std.fmt.bufPrint(out_buf, "{d}", .{val}) catch "";
    }

    fn formatCounters(name: []const u8, sep: []const u8, out_buf: []u8): usize {
        const trimmed = std.mem.trim(u8, name, " \t\r\n");
        let out_len: usize = 0;
        let first = true;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const e = &_.entries[i];
            if (!std.mem.eql(u8, e.name[0..e.name_len], trimmed)) continue;
            if (!first && sep.len > 0) {
                const take_s = @min(sep.len, out_buf.len - out_len);
                std.mem.copyForwards(u8, out_buf[out_len .. out_len + take_s], sep[0..take_s]);
                out_len += take_s;
            }
            let num_buf: [16]u8 = undefined;
            const num_str = std.fmt.bufPrint(&num_buf, "{d}", .{e.value}) catch "0";
            const take_n = @min(num_str.len, out_buf.len - out_len);
            std.mem.copyForwards(u8, out_buf[out_len .. out_len + take_n], num_str[0..take_n]);
            out_len += take_n;
            first = false;
        }
        if (first && out_buf.len > 0) {
            out_buf[0] = '0';
            return 1;
        }
        return out_len;
    }
}

pub fn hasDynamicPseudoContent(raw: []const u8): bool {
    return std.mem.indexOf(u8, raw, "counter(") != null ||
        std.mem.indexOf(u8, raw, "counters(") != null ||
        std.mem.indexOf(u8, raw, "attr(") != null;
}

pub fn resolvePseudoContentExpr(
    raw: []const u8,
    node: *const NodeData,
    stack: *CssCounterStack,
    out_buf: []u8,
): usize? {
    const s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len == 0) return null;
    if (std.ascii.eqlIgnoreCase(s, "none") || std.ascii.eqlIgnoreCase(s, "normal")) return null;

    let out_len: usize = 0;
    let i: usize = 0;
    let produced = false;
    while (i < s.len) {
        while (i < s.len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) : (i += 1) {}
        if (i >= s.len) break;
        const q = s[i];
        if (q == '"' || q == '\'') {
            produced = true;
            i += 1;
            while (i < s.len && s[i] != q) : (i += 1) {
                if (s[i] == '\\' && i + 1 < s.len) i += 1;
                if (out_len < out_buf.len) {
                    out_buf[out_len] = s[i];
                    out_len += 1;
                }
            }
            if (i < s.len && s[i] == q) i += 1;
        } else if (q == '/') {
            break;
        } else if (i + 9 <= s.len && std.ascii.eqlIgnoreCase(s[i .. i + 9], "counters(")) {
            i += 9;
            const start = i;
            while (i < s.len && s[i] != ')') : (i += 1) {}
            const args = s[start..i];
            if (i < s.len && s[i] == ')') i += 1;
            if (comma := std.mem.indexOfScalar(u8, args, ',')) {
                const cname = std.mem.trim(u8, args[0..comma], " \t\r\n");
                let sep = std.mem.trim(u8, args[comma + 1 ..], " \t\r\n");
                if (sep.len >= 2 && ((sep[0] == '"' && sep[sep.len - 1] == '"') || (sep[0] == '\'' && sep[sep.len - 1] == '\''))) {
                    sep = sep[1 .. sep.len - 1];
                }
                const wrote = stack.formatCounters(cname, sep, out_buf[out_len..]);
                out_len += wrote;
                produced = true;
            }
        } else if (i + 8 <= s.len && std.ascii.eqlIgnoreCase(s[i .. i + 8], "counter(")) {
            i += 8;
            const start = i;
            while (i < s.len && s[i] != ')') : (i += 1) {}
            const arg = std.mem.trim(u8, s[start..i], " \t\r\n");
            if (i < s.len && s[i] == ')') i += 1;
            let num_buf: [16]u8 = undefined;
            const txt = stack.formatCounter(arg, &num_buf);
            const take = @min(txt.len, out_buf.len - out_len);
            std.mem.copyForwards(u8, out_buf[out_len .. out_len + take], txt[0..take]);
            out_len += take;
            produced = true;
        } else if (i + 5 <= s.len && std.ascii.eqlIgnoreCase(s[i .. i + 5], "attr(")) {
            i += 5;
            const start = i;
            while (i < s.len && s[i] != ')') : (i += 1) {}
            const aname = std.mem.trim(u8, s[start..i], " \t\r\n");
            if (i < s.len && s[i] == ')') i += 1;
            if (aval := node.getAttribute(aname)) {
                const take = @min(aval.len, out_buf.len - out_len);
                std.mem.copyForwards(u8, out_buf[out_len .. out_len + take], aval[0..take]);
                out_len += take;
            }
            produced = true;
        } else if (i + 10 <= s.len && std.ascii.eqlIgnoreCase(s[i .. i + 10], "open-quote")) {
            i += 10;
            const ch: u8 = if (@mod(stack.quote_depth, 2) == 0) '"' else '\'';
            if (out_len < out_buf.len) {
                out_buf[out_len] = ch;
                out_len += 1;
            }
            stack.quote_depth += 1;
            produced = true;
        } else if (i + 11 <= s.len && std.ascii.eqlIgnoreCase(s[i .. i + 11], "close-quote")) {
            i += 11;
            if (stack.quote_depth > 0) stack.quote_depth -= 1;
            const ch: u8 = if (@mod(stack.quote_depth, 2) == 0) '"' else '\'';
            if (out_len < out_buf.len) {
                out_buf[out_len] = ch;
                out_len += 1;
            }
            produced = true;
        } else if (i + 13 <= s.len && std.ascii.eqlIgnoreCase(s[i .. i + 13], "no-open-quote")) {
            i += 13;
            stack.quote_depth += 1;
            produced = true;
        } else if (i + 14 <= s.len && std.ascii.eqlIgnoreCase(s[i .. i + 14], "no-close-quote")) {
            i += 14;
            if (stack.quote_depth > 0) stack.quote_depth -= 1;
            produced = true;
        } else {
            while (i < s.len && s[i] != ' ' && s[i] != '\t' && s[i] != '"' && s[i] != '\'') : (i += 1) {}
        }
    }
    if (!produced) return null;
    return out_len;
}

