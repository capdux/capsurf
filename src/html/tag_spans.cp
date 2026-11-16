// Migrated from vendor/kitesurf/packages/engine/src/eval/html-tag-spans.ts
// Finds markup tag spans with monotone cursors, so page-controlled documents cost linear work.

pub struct TagSpan {
    start: usize = 0,
    end: usize = 0,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0c || c == 0x0b;
}

fn isWordChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

pub fn closingTagEnd(text: []const u8, name: []const u8, from_offset: usize): i64 {
    let cursor: usize = from_offset;
    while (cursor < text.len) {
        const close_rel_opt = std.mem.indexOfScalar(u8, text[cursor..], '<');
        if (close_rel_opt == null) return -1;
        const close = cursor + close_rel_opt.?;

        if (close + 1 >= text.len || text[close + 1] != '/') {
            cursor = close + 1;
            continue;
        }

        const close_name_end = close + 2 + name.len;
        if (close_name_end > text.len) {
            cursor = close + 1;
            continue;
        }

        if (!std.ascii.eqlIgnoreCase(text[close + 2 .. close_name_end], name)) {
            cursor = close + 1;
            continue;
        }

        let next = close_name_end;
        while (next < text.len && isSpace(text[next])) {
            next += 1;
        }

        if (next < text.len && text[next] == '>') {
            return @as(i64, @intCast(next + 1));
        }

        cursor = if (next > close) next else close + 1;
    }
    return -1;
}

pub fn htmlTagSpans(text: []const u8, name: []const u8, paired: bool, out_spans: []TagSpan): usize {
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < text.len) {
        const start_rel_opt = std.mem.indexOfScalar(u8, text[pos..], '<');
        if (start_rel_opt == null) break;
        const start = pos + start_rel_opt.?;

        const end_name = start + 1 + name.len;
        if (end_name > text.len) break;

        const name_matches = std.ascii.eqlIgnoreCase(text[start + 1 .. end_name], name);
        const has_trailing_word = (end_name < text.len && isWordChar(text[end_name]));
        if (!name_matches || has_trailing_word) {
            pos = start + 1;
            continue;
        }

        const gt_rel_opt = std.mem.indexOfScalar(u8, text[end_name..], '>');
        if (gt_rel_opt == null) break;
        const gt = end_name + gt_rel_opt.?;
        let end = gt + 1;

        if (paired) {
            const close_end = closingTagEnd(text, name, end);
            if (close_end < 0) break;
            end = @as(usize, @intCast(close_end));
        }

        if (out_spans.len > 0) {
            if (count < out_spans.len) {
                out_spans[count] = TagSpan{ start: start, end: end };
                count += 1;
            } else {
                break;
            }
        } else {
            count += 1;
        }

        pos = end;
    }
    return count;
}

pub fn removeHtmlTagSpans(text: []const u8, spans: []const TagSpan, out_buf: []u8): []const u8 {
    if (spans.len == 0) return text;
    let out_len: usize = 0;
    let previous_end: usize = 0;

    let i: usize = 0;
    while (i < spans.len) : (i += 1) {
        const span = spans[i];
        if (span.start >= previous_end && span.start <= text.len) {
            const part = text[previous_end..span.start];
            if (out_len + part.len <= out_buf.len) {
                @memcpy(out_buf[out_len .. out_len + part.len], part);
                out_len += part.len;
            } else if (out_len < out_buf.len) {
                const avail = out_buf.len - out_len;
                @memcpy(out_buf[out_len..], part[0..avail]);
                out_len += avail;
            }
        }
        if (span.end > previous_end) {
            previous_end = @min(span.end, text.len);
        }
    }

    if (previous_end < text.len) {
        const part = text[previous_end..];
        if (out_len + part.len <= out_buf.len) {
            @memcpy(out_buf[out_len .. out_len + part.len], part);
            out_len += part.len;
        } else if (out_len < out_buf.len) {
            const avail = out_buf.len - out_len;
            @memcpy(out_buf[out_len..], part[0..avail]);
            out_len += avail;
        }
    }

    return out_buf[0..out_len];
}
