// src/cdp/svg_document.cp
// CapSurf SVG Document Classification & Viewer HTML Synthesis
// Aligned with vendor/kitesurf/packages/engine/src/cdp/document-kind.ts

fn isAsciiSpace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn isAsciiWord(b: u8): bool {
    return (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9') || b == '_';
}

pub struct BufferWriter {
    buf: []u8 = [],
    len: usize = 0,

    static fn init(buf: []u8): BufferWriter {
        return BufferWriter{
            buf: buf,
            len: 0,
        };
    }

    fn write(*_, s: []const u8) {
        if (s.len == 0) return;
        const copy_len = @min(s.len, _.buf.len - _.len);
        if (copy_len > 0) {
            @memcpy(_.buf[_.len .. _.len + copy_len], s[0..copy_len]);
            _.len += copy_len;
        }
    }

    fn getWritten(): []const u8 {
        return _.buf[0.._.len];
    }
}

pub fn svgRootStart(source: []const u8): isize {
    let at: usize = 0;
    while (at < source.len) {
        const lt_rel = std.mem.indexOfScalar(u8, source[at..], '<');
        if (lt_rel == null) return -1;
        const lt = at + lt_rel.?;
        const rest = source[lt..];

        if (std.mem.startsWith(u8, rest, "<!--")) {
            const close_idx = std.mem.indexOf(u8, source[lt + 4 ..], "-->");
            if (close_idx == null) return -1;
            at = lt + 4 + close_idx.? + 3;
        } else if (std.mem.startsWith(u8, rest, "<?")) {
            const close_idx = std.mem.indexOf(u8, source[lt + 2 ..], "?>");
            if (close_idx == null) return -1;
            at = lt + 2 + close_idx.? + 2;
        } else if (std.ascii.startsWithIgnoreCase(rest, "<!doctype")) {
            const bracket_rel = std.mem.indexOfScalar(u8, source[lt..], '[');
            const gt_rel = std.mem.indexOfScalar(u8, source[lt..], '>');
            if (gt_rel == null) return -1;
            const gt = lt + gt_rel.?;
            if (bracket_rel != null && lt + bracket_rel.? < gt) {
                const bracket = lt + bracket_rel.?;
                const close_rel = std.mem.indexOfScalar(u8, source[bracket..], ']');
                if (close_rel == null) return -1;
                const close_bracket = bracket + close_rel.?;
                const gt_after_rel = std.mem.indexOfScalar(u8, source[close_bracket..], '>');
                if (gt_after_rel == null) return -1;
                at = close_bracket + gt_after_rel.? + 1;
            } else {
                at = gt + 1;
            }
        } else if (rest.len >= 4 && std.ascii.startsWithIgnoreCase(rest, "<svg") &&
                   (rest.len == 4 || isAsciiSpace(rest[4]) || rest[4] == '>'))
        {
            return @as(isize, @intCast(lt));
        } else if (rest.len >= 2 && rest[0] == '<' && (isAsciiWord(rest[1]) || rest[1] == '!' || rest[1] == '?' || rest[1] == '/')) {
            return -1;
        } else {
            at = lt + 1;
        }
    }
    return -1;
}

pub struct TagSpan {
    start: usize = 0,
    end: usize = 0,
}

pub fn findTagSpans(text: []const u8, name: []const u8, spans_buf: []TagSpan): usize {
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < text.len && count < spans_buf.len) {
        const start_rel = std.mem.indexOfScalar(u8, text[pos..], '<');
        if (start_rel == null) break;
        const start = pos + start_rel.?;
        const end_name = start + 1 + name.len;
        if (end_name > text.len) break;
        if (!std.ascii.eqlIgnoreCase(text[start + 1 .. end_name], name)) {
            pos = start + 1;
            continue;
        }
        if (end_name < text.len && isAsciiWord(text[end_name])) {
            pos = start + 1;
            continue;
        }
        const gt_rel = std.mem.indexOfScalar(u8, text[end_name..], '>');
        if (gt_rel == null) break;
        const end = end_name + gt_rel.? + 1;
        spans_buf[count] = TagSpan{ start: start, end: end };
        count += 1;
        pos = end;
    }
    return count;
}

pub fn findTagAttr(tag: []const u8, attr: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i + attr.len <= tag.len) : (i += 1) {
        if (i > 0) {
            const prev = tag[i - 1];
            if (!isAsciiSpace(prev) && prev != '"' && prev != '\'') continue;
        }
        if (!std.ascii.eqlIgnoreCase(tag[i .. i + attr.len], attr)) continue;

        let k = i + attr.len;
        while (k < tag.len && isAsciiSpace(tag[k])) : (k += 1) {}
        if (k >= tag.len || tag[k] != '=') continue;
        k += 1; // skip '='
        while (k < tag.len && isAsciiSpace(tag[k])) : (k += 1) {}
        if (k >= tag.len) return null;

        if (tag[k] == '"' || tag[k] == '\'') {
            const quote = tag[k];
            k += 1;
            const val_start = k;
            while (k < tag.len && tag[k] != quote && tag[k] != '>') : (k += 1) {}
            return tag[val_start..k];
        } else {
            const val_start = k;
            while (k < tag.len && !isAsciiSpace(tag[k]) && tag[k] != '>' && tag[k] != '"' && tag[k] != '\'') : (k += 1) {}
            return tag[val_start..k];
        }
    }
    return null;
}

pub fn svgDocumentHtml(
    svg_source: []const u8,
    viewport_width: u32,
    viewport_height: u32,
    out_buf: []u8,
): []const u8 {
    const root_start = svgRootStart(svg_source);
    const svg = if (root_start >= 0) svg_source[@as(usize, @intCast(root_start))..] else svg_source;

    let meta_spans: [32]TagSpan = undefined;
    const meta_count = findTagSpans(svg, "meta", &meta_spans);

    const gt_pos = std.mem.indexOfScalar(u8, svg, '>') ?? svg.len;
    const root_tag = svg[0 .. if (gt_pos < svg.len) gt_pos + 1 else svg.len];

    const has_view_box = (findTagAttr(root_tag, "viewBox") != null);
    let has_abs_w = false;
    if (w_val := findTagAttr(root_tag, "width")) {
        has_abs_w = !std.mem.endsWith(u8, w_val, "%");
    }
    let has_abs_h = false;
    if (h_val := findTagAttr(root_tag, "height")) {
        has_abs_h = !std.mem.endsWith(u8, h_val, "%");
    }

    const need_inject_view_box = !has_view_box && !(has_abs_w && has_abs_h);
    const effective_has_view_box = has_view_box || need_inject_view_box;

    let width_rule: []const u8 = "";
    let width_rule_buf: [32]u8 = [0; 32];
    if (w_val := findTagAttr(root_tag, "width")) {
        if (std.mem.endsWith(u8, w_val, "%")) {
            const pct = w_val[0 .. w_val.len - 1];
            if (effective_has_view_box) {
                width_rule = std.fmt.bufPrint(&width_rule_buf, "width:{s}vw;", .{pct}) catch "";
            }
        }
    } else {
        if (effective_has_view_box) {
            width_rule = "width:100vw;";
        }
    }

    let height_rule: []const u8 = "";
    let height_rule_buf: [32]u8 = [0; 32];
    if (h_val := findTagAttr(root_tag, "height")) {
        if (std.mem.endsWith(u8, h_val, "%")) {
            const pct = h_val[0 .. h_val.len - 1];
            if (effective_has_view_box) {
                height_rule = std.fmt.bufPrint(&height_rule_buf, "height:{s}vh;", .{pct}) catch "";
            }
        }
    } else {
        if (effective_has_view_box) {
            height_rule = "height:100vh;";
        }
    }

    let w = BufferWriter.init(out_buf);
    w.write("<!doctype html><html><head><style>html,body{margin:0;padding:0}body>svg{display:block;");
    w.write(width_rule);
    w.write(height_rule);
    w.write("}</style>");

    let m_idx: usize = 0;
    while (m_idx < meta_count) : (m_idx += 1) {
        const span = meta_spans[m_idx];
        w.write(svg[span.start..span.end]);
    }

    w.write("</head><body>");

    let cursor: usize = 0;
    if (need_inject_view_box) {
        if (svg.len >= 4 && std.ascii.startsWithIgnoreCase(svg, "<svg")) {
            w.write(svg[0..4]);
            w.write(" viewBox=\"0 0 ");
            let vb_buf: [64]u8 = [0; 64];
            const vb_str = std.fmt.bufPrint(&vb_buf, "{d} {d}\"", .{ viewport_width, viewport_height }) catch "0 0\"";
            w.write(vb_str);
            cursor = 4;
        }
    }

    m_idx = 0;
    while (m_idx < meta_count) : (m_idx += 1) {
        const span = meta_spans[m_idx];
        if (span.start > cursor) {
            w.write(svg[cursor..span.start]);
        }
        cursor = span.end;
    }
    if (cursor < svg.len) {
        w.write(svg[cursor..]);
    }

    w.write("</body></html>");
    return w.getWritten();
}

pub fn isSvgDocument(url: []const u8, content_type: []const u8, source: []const u8): bool {
    if (std.mem.indexOf(u8, content_type, "image/svg+xml") != null) return true;
    if (std.mem.indexOf(u8, url, ".svg") != null) return true;
    const root_s = svgRootStart(source);
    if (root_s >= 0) {
        if (std.mem.indexOf(u8, source, "<html") == null && std.mem.indexOf(u8, source, "<body") == null) {
            return true;
        }
    }
    return false;
}
