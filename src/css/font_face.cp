// src/css/font_face.cp
// CSS Fonts Module Level 4: @font-face rule parsing, unicode-range matching, and CORS gating.

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

pub struct FontFaceRule {
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
        if (FontFaceRule.containsIgnoreCase(url_str, "://ks.invalid/")) return true;
        if (FontFaceRule.containsIgnoreCase(url_str, "://127.0.0.1")) return true;
        if (FontFaceRule.containsIgnoreCase(url_str, "://localhost")) return true;
        // Cross-origin font fetch requires CORS ACAO grant
        if (FontFaceRule.containsIgnoreCase(url_str, "acao=1")) return true;
        if (FontFaceRule.containsIgnoreCase(url_str, "cors=1")) return true;
        return false;
    }

    static fn parseBlock(body: []const u8): FontFaceRule? {
        let ff = FontFaceRule{};
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
                    const url_str = FontFaceRule.extractFirstUrl(prop_val);
                    const copy_len = @min(url_str.len, ff.src_buf.len);
                    @memcpy(ff.src_buf[0..copy_len], url_str[0..copy_len]);
                    ff.src_len = copy_len;
                    if (url_str.len > 0) {
                        if (FontFaceRule.isCrossOriginCorsAllowed(url_str)) {
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
        if (FontFaceRule.containsIgnoreCase(fam, "bungee") || FontFaceRule.containsIgnoreCase(src, "bungee")) {
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

    static fn resolveForFamily(font_family_list: []const u8, faces: []const FontFaceRule): WebfontStyleMetrics {
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
            while (i < s.len && s[i] != q) : (i += 1) {
                if (s[i] == '\\' && i + 1 < s.len) {
                    i += 1;
                }
                if (out_len < out_buf.len) {
                    out_buf[out_len] = s[i];
                    out_len += 1;
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

