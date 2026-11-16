// Migrated from vendor/kitesurf/packages/engine/src/cdp/document-kind.ts
// CapSurf navigation document classification and viewer document synthesis.

import { sniffImageDims, ImageDims } from "../render/image_dims.cp";
import { JsonParser, JsonValue } from "cap:json";

pub enum DocumentKind {
    html,
    json,
    svg,
    image,
    text,
    blocked,

    fn asStr(): []const u8 {
        return switch (_) {
            .html => "html",
            .json => "json",
            .svg => "svg",
            .image => "image",
            .text => "text",
            .blocked => "blocked",
        };
    }
}

pub const JSON_PARSED_MAX_CHARS: usize = 1 << 20;

const RASTER_MIMES: [7][]const u8 = [
    "image/png",
    "image/apng",
    "image/jpeg",
    "image/jpg",
    "image/pjpeg",
    "image/gif",
    "image/webp",
];

const TEXT_APPLICATION_MIMES: [3][]const u8 = [
    "application/javascript",
    "application/x-javascript",
    "application/ecmascript",
];

fn isRasterMime(mime: []const u8): bool {
    let i: usize = 0;
    while (i < RASTER_MIMES.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(mime, RASTER_MIMES[i])) return true;
    }
    return false;
}

fn isTextApplicationMime(mime: []const u8): bool {
    let i: usize = 0;
    while (i < TEXT_APPLICATION_MIMES.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(mime, TEXT_APPLICATION_MIMES[i])) return true;
    }
    return false;
}

pub fn mimeEssence(content_type: []const u8): []const u8 {
    let s = content_type;
    if (idx := std.mem.indexOfScalar(u8, s, ';')) {
        s = s[0..idx];
    }
    return std.mem.trim(u8, s, " \t\r\n");
}

pub fn isAttachment(content_disposition: ([]const u8)?): bool {
    if (content_disposition == null) return false;
    const cd = content_disposition.?;
    let s = cd;
    if (idx := std.mem.indexOfScalar(u8, s, ';')) {
        s = s[0..idx];
    }
    const t = std.mem.trim(u8, s, " \t\r\n");
    if (t.len == 0) return false;
    if (std.ascii.eqlIgnoreCase(t, "inline")) return false;
    if (std.mem.indexOfScalar(u8, t, '=') != null) return false;
    return true;
}

pub fn documentKindFromMime(mime_type: []const u8, content_disposition: ([]const u8)?): DocumentKind {
    if (isAttachment(content_disposition)) return .blocked;

    const mime = mimeEssence(mime_type);
    if (mime.len == 0 ||
        std.ascii.eqlIgnoreCase(mime, "text/html") ||
        std.ascii.eqlIgnoreCase(mime, "application/xhtml+xml"))
    {
        return .html;
    }
    if (std.ascii.eqlIgnoreCase(mime, "application/json") ||
        std.ascii.endsWithIgnoreCase(mime, "+json"))
    {
        return .json;
    }
    if (std.ascii.eqlIgnoreCase(mime, "image/svg+xml")) {
        return .svg;
    }
    if (isRasterMime(mime)) {
        return .image;
    }
    if (std.ascii.startsWithIgnoreCase(mime, "text/") ||
        isTextApplicationMime(mime))
    {
        return .text;
    }
    if (std.ascii.eqlIgnoreCase(mime, "application/xml") ||
        std.ascii.endsWithIgnoreCase(mime, "+xml"))
    {
        return .text;
    }
    return .blocked;
}

pub fn documentKind(content_type: []const u8, content_disposition: ([]const u8)?): DocumentKind {
    return documentKindFromMime(content_type, content_disposition);
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
        if (_.len >= _.buf.len) return;
        const avail = _.buf.len - _.len;
        const to_copy = if (s.len < avail) s.len else avail;
        @memcpy(_.buf[_.len .. _.len + to_copy], s[0..to_copy]);
        _.len += to_copy;
    }

    fn writeByte(*_, b: u8) {
        if (_.len < _.buf.len) {
            _.buf[_.len] = b;
            _.len += 1;
        }
    }

    fn writeEscaped(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '&') {
                _.write("&amp;");
            } else if (c == '<') {
                _.write("&lt;");
            } else if (c == '>') {
                _.write("&gt;");
            } else {
                _.writeByte(c);
            }
        }
    }

    fn getWritten(): []const u8 {
        return _.buf[0.._.len];
    }
}

@tls let tls_escape_buf: [4096]u8 = [0; 4096];

pub fn escapeHtmlToBuf(s: []const u8, out_buf: []u8): []const u8 {
    let w = BufferWriter.init(out_buf);
    w.writeEscaped(s);
    return w.getWritten();
}

pub fn escapeHtml(s: []const u8): []const u8 {
    let has_special = false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '&' || c == '<' || c == '>') {
            has_special = true;
            break;
        }
    }
    if (!has_special) return s;
    return escapeHtmlToBuf(s, &tls_escape_buf);
}

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn decodeUriComponent(s: []const u8, out_buf: []u8): []const u8 {
    let i: usize = 0;
    let out_len: usize = 0;
    while (i < s.len && out_len < out_buf.len) {
        if (s[i] == '%' && i + 2 < s.len) {
            const h1 = hexVal(s[i + 1]);
            const h2 = hexVal(s[i + 2]);
            if (h1 != null && h2 != null) {
                out_buf[out_len] = (h1.? << 4) | h2.?;
                out_len += 1;
                i += 3;
                continue;
            }
        }
        out_buf[out_len] = s[i];
        out_len += 1;
        i += 1;
    }
    return out_buf[0..out_len];
}

@tls let tls_title_buf: [512]u8 = [0; 512];
@tls let tls_name_buf: [256]u8 = [0; 256];

pub fn imageDocumentTitleBuf(url: []const u8, bytes: []const u8, out_buf: []u8): []const u8 {
    let name: []const u8 = "";
    let is_http_or_https = false;
    let rem: []const u8 = "";

    if (std.ascii.startsWithIgnoreCase(url, "http://")) {
        is_http_or_https = true;
        rem = url[7..];
    } else if (std.ascii.startsWithIgnoreCase(url, "https://")) {
        is_http_or_https = true;
        rem = url[8..];
    }

    if (is_http_or_https) {
        let auth_end: usize = rem.len;
        let i: usize = 0;
        while (i < rem.len) : (i += 1) {
            const c = rem[i];
            if (c == '/' || c == '?' || c == '#') {
                auth_end = i;
                break;
            }
        }
        let authority = rem[0..auth_end];
        if (at_idx := std.mem.lastIndexOfScalar(u8, authority, '@')) {
            authority = authority[at_idx + 1 ..];
        }

        let hostname = authority;
        if (authority.len > 0 && authority[0] == '[') {
            if (close_bracket := std.mem.indexOfScalar(u8, authority, ']')) {
                hostname = authority[0 .. close_bracket + 1];
            }
        } else {
            if (colon_idx := std.mem.lastIndexOfScalar(u8, authority, ':')) {
                hostname = authority[0..colon_idx];
            }
        }

        if (auth_end < rem.len && rem[auth_end] == '/') {
            let path_end: usize = rem.len;
            let j: usize = auth_end;
            while (j < rem.len) : (j += 1) {
                const c = rem[j];
                if (c == '?' || c == '#') {
                    path_end = j;
                    break;
                }
            }

            while (path_end > auth_end && rem[path_end - 1] == '/') : (path_end -= 1) {}
            let last_slash: usize = auth_end;
            let k: usize = auth_end;
            while (k < path_end) : (k += 1) {
                if (rem[k] == '/') last_slash = k + 1;
            }

            if (last_slash < path_end) {
                name = decodeUriComponent(rem[last_slash..path_end], &tls_name_buf);
            }
        }
        if (name.len == 0) {
            name = hostname;
        }
    }

    let size_buf: [64]u8 = [0; 64];
    let size: []const u8 = "";
    if (dims := sniffImageDims(bytes)) {
        size = std.fmt.bufPrint(&size_buf, "{d}\xc3\x97{d}", .{ dims.width, dims.height }) catch "";
    }

    let w = BufferWriter.init(out_buf);
    if (name.len > 0 && size.len > 0) {
        w.write(name);
        w.write(" (");
        w.write(size);
        w.write(")");
    } else if (name.len > 0) {
        w.write(name);
    } else {
        w.write(size);
    }
    return w.getWritten();
}

pub fn imageDocumentTitle(url: []const u8, bytes: []const u8): []const u8 {
    return imageDocumentTitleBuf(url, bytes, &tls_title_buf);
}

pub fn imageDocumentHtml(
    bytes: []const u8,
    base64_str: []const u8,
    content_type: []const u8,
    url: []const u8,
    out_buf: []u8,
): []const u8 {
    const mime = mimeEssence(content_type);
    let title_buf: [512]u8 = [0; 512];
    const title = imageDocumentTitleBuf(url, bytes, &title_buf);

    let w = BufferWriter.init(out_buf);
    w.write("<!doctype html><html><head><meta name=\"viewport\" content=\"width=device-width, minimum-scale=0.1\"><title>");
    w.writeEscaped(title);
    w.write("</title><style>html,body{height:100%}body{margin:0;background-color:rgb(14,14,14);display:flex;align-items:center;justify-content:center}img{display:block;max-width:100%;max-height:100%;min-width:0;min-height:0;user-select:none;background-color:hsl(0,0%,90%)}</style></head><body><img src=\"data:");
    w.write(mime);
    w.write(";base64,");
    w.write(base64_str);
    w.write("\"></body></html>");
    return w.getWritten();
}

fn isAsciiSpace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn isAsciiWord(b: u8): bool {
    return (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9') || b == '_';
}

fn svgRootStart(source: []const u8): isize {
    let at: usize = 0;
    while (at < source.len) {
        const lt_rel = std.mem.indexOfScalar(u8, source[at..], '<');
        if (lt_rel == null) return -1;
        const lt = at + lt_rel.?;
        const rest = source[lt..];

        if (std.mem.startsWith(u8, rest, "<!--")) {
            const close = std.mem.indexOf(u8, source[lt + 4 ..], "-->");
            if (close == null) return -1;
            at = lt + 4 + close.? + 3;
        } else if (std.mem.startsWith(u8, rest, "<?")) {
            const close = std.mem.indexOf(u8, source[lt + 2 ..], "?>");
            if (close == null) return -1;
            at = lt + 2 + close.? + 2;
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
        } else if (rest.len >= 2 && rest[0] == '<' &&
                   ((rest[1] >= 'a' && rest[1] <= 'z') || (rest[1] >= 'A' && rest[1] <= 'Z') ||
                    rest[1] == '!' || rest[1] == '?' || rest[1] == '/'))
        {
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

fn findTagAttr(tag: []const u8, attr: []const u8): ([]const u8)? {
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

pub fn textDocumentHtml(text: []const u8, url: []const u8, out_buf: []u8): []const u8 {
    let w = BufferWriter.init(out_buf);
    w.write("<html><head><title>");
    w.writeEscaped(url);
    w.write("</title></head><body><pre>");
    w.writeEscaped(text);
    w.write("</pre></body></html>");
    return w.getWritten();
}

fn writeJsonStringEscaped(w: *BufferWriter, s: []const u8) {
    w.writeByte('"');
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '"') {
            w.write("\\\"");
        } else if (c == '\\') {
            w.write("\\\\");
        } else if (c == '\n') {
            w.write("\\n");
        } else if (c == '\r') {
            w.write("\\r");
        } else if (c == '\t') {
            w.write("\\t");
        } else if (c == '&') {
            w.write("&amp;");
        } else if (c == '<') {
            w.write("&lt;");
        } else if (c == '>') {
            w.write("&gt;");
        } else {
            w.writeByte(c);
        }
    }
    w.writeByte('"');
}

fn renderJsonScalar(w: *BufferWriter, value: *const JsonValue) {
    switch (value.tag) {
        .Null => {
            w.write("<span class=\"json-null\">null</span>");
        },
        .Bool => {
            if (value.bool_val) {
                w.write("<span class=\"json-bool\">true</span>");
            } else {
                w.write("<span class=\"json-bool\">false</span>");
            }
        },
        .Number => {
            let num_buf: [64]u8 = [0; 64];
            let num_str: []const u8 = "";
            const v = value.num_val;
            if (v == @trunc(v) && v >= -9007199254740991.0 && v <= 9007199254740991.0) {
                num_str = std.fmt.bufPrint(&num_buf, "{d}", .{@as(i64, @intFromFloat(v))}) catch "0";
            } else {
                num_str = std.fmt.bufPrint(&num_buf, "{d}", .{v}) catch "0";
            }
            w.write("<span class=\"json-number\">");
            w.write(num_str);
            w.write("</span>");
        },
        .String => {
            w.write("<span class=\"json-string\">");
            writeJsonStringEscaped(w, value.str_val);
            w.write("</span>");
        },
        else => {
            w.write("<span class=\"json-null\">null</span>");
        },
    }
}

fn renderJsonRow(w: *BufferWriter, value: *const JsonValue, key_opt: ([]const u8)?, last: bool) {
    if (value.tag != .Array && value.tag != .Object) {
        w.write("<div class=\"json-row\">");
        if (k := key_opt) {
            w.write("<span class=\"json-key\">");
            writeJsonStringEscaped(w, k);
            w.write("</span><span class=\"json-punct\">: </span>");
        }
        renderJsonScalar(w, value);
        if (!last) {
            w.write("<span class=\"json-punct\">,</span>");
        }
        w.write("</div>");
        return;
    }

    if (value.tag == .Array) {
        if (value.arr_val.len == 0) {
            w.write("<div class=\"json-row\">");
            if (k := key_opt) {
                w.write("<span class=\"json-key\">");
                writeJsonStringEscaped(w, k);
                w.write("</span><span class=\"json-punct\">: </span>");
            }
            w.write("<span class=\"json-punct\">[]</span>");
            if (!last) {
                w.write("<span class=\"json-punct\">,</span>");
            }
            w.write("</div>");
            return;
        }

        w.write("<div class=\"json-row json-collapsible\">");
        w.write("<span class=\"json-toggle\" aria-hidden=\"true\">\xe2\x96\xbc</span>");
        w.write("<span class=\"json-summary\">");
        if (k := key_opt) {
            w.write("<span class=\"json-key\">");
            writeJsonStringEscaped(w, k);
            w.write("</span><span class=\"json-punct\">: </span>");
        }
        w.write("<span class=\"json-punct\">[</span><span class=\"json-preview\">\xe2\x80\xa6]");
        if (!last) {
            w.write(",");
        }
        w.write("</span></span>");
        w.write("<div class=\"json-children\">");

        let i: usize = 0;
        const total = value.arr_val.len;
        while (i < total) : (i += 1) {
            renderJsonRow(w, &value.arr_val[i], null, i == total - 1);
        }

        w.write("</div><div class=\"json-close\"><span class=\"json-punct\">]</span>");
        if (!last) {
            w.write("<span class=\"json-punct\">,</span>");
        }
        w.write("</div></div>");
        return;
    }

    if (value.tag == .Object) {
        if (value.obj_val.len == 0) {
            w.write("<div class=\"json-row\">");
            if (k := key_opt) {
                w.write("<span class=\"json-key\">");
                writeJsonStringEscaped(w, k);
                w.write("</span><span class=\"json-punct\">: </span>");
            }
            w.write("<span class=\"json-punct\">{}</span>");
            if (!last) {
                w.write("<span class=\"json-punct\">,</span>");
            }
            w.write("</div>");
            return;
        }

        w.write("<div class=\"json-row json-collapsible\">");
        w.write("<span class=\"json-toggle\" aria-hidden=\"true\">\xe2\x96\xbc</span>");
        w.write("<span class=\"json-summary\">");
        if (k := key_opt) {
            w.write("<span class=\"json-key\">");
            writeJsonStringEscaped(w, k);
            w.write("</span><span class=\"json-punct\">: </span>");
        }
        w.write("<span class=\"json-punct\">{</span><span class=\"json-preview\">\xe2\x80\xa6}");
        if (!last) {
            w.write(",");
        }
        w.write("</span></span>");
        w.write("<div class=\"json-children\">");

        let i: usize = 0;
        const total = value.obj_val.len;
        while (i < total) : (i += 1) {
            const field = &value.obj_val[i];
            renderJsonRow(w, &field.value, field.key, i == total - 1);
        }

        w.write("</div><div class=\"json-close\"><span class=\"json-punct\">}</span>");
        if (!last) {
            w.write("<span class=\"json-punct\">,</span>");
        }
        w.write("</div></div>");
        return;
    }
}

pub fn jsonViewerHtml(text: []const u8, url: []const u8, out_buf: []u8): []const u8 {
    if (text.len > JSON_PARSED_MAX_CHARS) {
        return textDocumentHtml(text, url, out_buf);
    }

    let mem_arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer mem_arena.deinit();

    const parsed = JsonParser.parse(mem_arena.allocator(), text) catch {
        return textDocumentHtml(text, url, out_buf);
    };

    let w = BufferWriter.init(out_buf);
    w.write("<!doctype html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n<title>");
    w.writeEscaped(url);
    w.write("</title>\n<style>\n  :root { color-scheme: light; }\n  html, body {\n    margin: 0;\n    background: #fff;\n    color: #111;\n    font: 13px/1.4 ui-monospace, SFMono-Regular, Menlo, Consolas, \"Liberation Mono\", monospace;\n  }\n  body { padding: 16px 24px 32px; }\n  .json-toolbar {\n    position: fixed;\n    top: 12px;\n    right: 20px;\n    display: inline-flex;\n    border: 1px solid #aaa;\n    border-radius: 3px;\n    overflow: hidden;\n    background: linear-gradient(#fdfdfd, #dadada);\n    box-shadow: 0 1px 2px rgba(0, 0, 0, 0.22);\n    z-index: 1;\n  }\n  .json-toolbar button {\n    min-width: 70px;\n    padding: 6px 14px;\n    border: 0;\n    border-left: 1px solid #aaa;\n    background: transparent;\n    color: #333;\n    font: 12px -apple-system, BlinkMacSystemFont, \"Segoe UI\", sans-serif;\n    cursor: pointer;\n  }\n  .json-toolbar button:first-child { border-left: 0; }\n  .json-toolbar button.active { background: linear-gradient(#d8d8d8, #f5f5f5); }\n  .json-panel { padding-right: 200px; }\n  pre { margin: 0; white-space: pre-wrap; overflow-wrap: anywhere; }\n  .json-row {\n    position: relative;\n    padding-left: 14px;\n    white-space: pre;\n    min-height: 1.4em;\n  }\n  .json-row > .json-toggle {\n    position: absolute;\n    left: 0;\n    top: 0;\n    width: 12px;\n    color: #aaa;\n    cursor: pointer;\n    user-select: none;\n    font-size: 10px;\n    line-height: 1.4em;\n  }\n  .json-children { margin-left: 14px; }\n  .json-collapsed > .json-children,\n  .json-collapsed > .json-close { display: none; }\n  .json-preview { color: #888; }\n  .json-row:not(.json-collapsed) > .json-summary > .json-preview { display: none; }\n  .json-key { color: #881391; }\n  .json-string { color: #c41a16; }\n  .json-number { color: #1a01cc; }\n  .json-bool { color: #1a01cc; font-weight: 600; }\n  .json-null { color: #808080; font-weight: 600; }\n  .json-punct { color: #444; }\n</style>\n</head>\n<body>\n<div class=\"json-toolbar\" role=\"tablist\" aria-label=\"JSON view mode\">\n  <button type=\"button\" class=\"active\" data-view=\"parsed\" role=\"tab\" aria-selected=\"true\">Parsed</button>\n  <button type=\"button\" data-view=\"raw\" role=\"tab\" aria-selected=\"false\">Raw</button>\n</div>\n<main>\n  <section id=\"json-parsed\" class=\"json-panel\">");

    renderJsonRow(&w, &parsed, null, true);

    w.write("</section>\n  <section id=\"json-raw\" class=\"json-panel\" style=\"display:none\"><pre>");
    w.writeEscaped(text);
    w.write("</pre></section>\n</main>\n<script>\n  (function () {\n    var parsedPanel = document.getElementById(\"json-parsed\");\n    var rawPanel = document.getElementById(\"json-raw\");\n    var buttons = document.querySelectorAll(\".json-toolbar button\");\n    function show(view) {\n      parsedPanel.style.display = view === \"parsed\" ? \"\" : \"none\";\n      rawPanel.style.display = view === \"raw\" ? \"\" : \"none\";\n      buttons.forEach(function (button) {\n        var active = button.getAttribute(\"data-view\") === view;\n        button.classList.toggle(\"active\", active);\n        button.setAttribute(\"aria-selected\", active ? \"true\" : \"false\");\n      });\n    }\n    buttons.forEach(function (button) {\n      button.addEventListener(\"click\", function () {\n        show(button.getAttribute(\"data-view\"));\n      });\n    });\n    parsedPanel.addEventListener(\"click\", function (ev) {\n      var summary = ev.target.closest && ev.target.closest(\".json-summary\");\n      if (!summary) return;\n      var row = summary.parentElement;\n      if (!row || !row.classList.contains(\"json-collapsible\")) return;\n      row.classList.toggle(\"json-collapsed\");\n    });\n  })();\n</script>\n</body>\n</html>");

    return w.getWritten();
}
