// src/render/pdf.cp
// Pure CP CSS Paged Media Paginator & Multi-Page PDF-1.4 Generator.
// Consumes Document, StyleSheet (@page margins), ComputedStyle (break-before/after/inside),
// and LayoutTree to compute exact page breaks and emit a valid multi-page PDF binary.

import { Document, NodeType, NodeId, NULL_NODE } from "../dom/dom.cp";
import { StyleSheet, ComputedStyle } from "../css/cascade.cp";
import { BreakValue, BreakInsideValue } from "../css/paged_media.cp";
import { LayoutTree, Rect } from "../layout/layout.cp";
import { compress2, compressBound } from "cap:pack";

pub struct PdfOptions {
    paper_width_in: f32 = 8.5;
    paper_height_in: f32 = 11.0;
    margin_top_in: f32 = 0.0;
    margin_right_in: f32 = 0.0;
    margin_bottom_in: f32 = 0.0;
    margin_left_in: f32 = 0.0;
    has_explicit_margin_top: bool = false;
    has_explicit_margin_right: bool = false;
    has_explicit_margin_bottom: bool = false;
    has_explicit_margin_left: bool = false;
    landscape: bool = false;
    prefer_css_page_size: bool = false;
}

pub struct PdfPaginationResult {
    page_count: u32 = 1;
    printable_width_px: f32 = 816.0;
    printable_height_px: f32 = 1056.0;
    media_width_pt: f32 = 612.0;
    media_height_pt: f32 = 792.0;
}

fn findBodyNode(doc: *const Document): NodeId {
    let i: usize = 1;
    while (i < doc.nodes.items.len) : (i += 1) {
        const n = &doc.nodes.items[i];
        if (n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, "body")) {
            return @as(NodeId, @intCast(i));
        }
    }
    return NULL_NODE;
}

/// Computes effective printable page height and exact page count honoring:
/// - `@page { margin: ... }` (overrides default print margins)
/// - `break-before: page | always` / `page-break-before`
/// - `break-after: page | always` / `page-break-after`
/// - `break-inside: avoid` / `page-break-inside: avoid`
/// - Block-level pagination snapping so whole leaf blocks don't split across page boundaries.
pub fn computePdfPagination(
    doc: *const Document,
    sheet: *const StyleSheet,
    styles: []const ComputedStyle,
    layout: *const LayoutTree,
    opts: PdfOptions,
): PdfPaginationResult {
    const raw_w_in = if (opts.paper_width_in > 0.0) opts.paper_width_in else 8.5;
    const raw_h_in = if (opts.paper_height_in > 0.0) opts.paper_height_in else 11.0;
    const eff_w_in = if (opts.landscape) raw_h_in else raw_w_in;
    const eff_h_in = if (opts.landscape) raw_w_in else raw_h_in;

    const page_w_px = eff_w_in * 96.0;
    const page_h_px = eff_h_in * 96.0;

    let mt_px: f32 = if (opts.has_explicit_margin_top) opts.margin_top_in * 96.0 else 0.0;
    let mb_px: f32 = if (opts.has_explicit_margin_bottom) opts.margin_bottom_in * 96.0 else 0.0;
    let ml_px: f32 = if (opts.has_explicit_margin_left) opts.margin_left_in * 96.0 else 0.0;
    let mr_px: f32 = if (opts.has_explicit_margin_right) opts.margin_right_in * 96.0 else 0.0;

    if (sheet.page_margin.has_margin_top) mt_px = sheet.page_margin.margin_top_px;
    if (sheet.page_margin.has_margin_bottom) mb_px = sheet.page_margin.margin_bottom_px;
    if (sheet.page_margin.has_margin_left) ml_px = sheet.page_margin.margin_left_px;
    if (sheet.page_margin.has_margin_right) mr_px = sheet.page_margin.margin_right_px;

    const printable_w = @max(@as(f32, 1.0), page_w_px - ml_px - mr_px);
    const printable_h = @max(@as(f32, 1.0), page_h_px - mt_px - mb_px);

    let res = PdfPaginationResult{
        page_count: 1,
        printable_width_px: printable_w,
        printable_height_px: printable_h,
        media_width_pt: eff_w_in * 72.0,
        media_height_pt: eff_h_in * 72.0,
    };

    const body_id = findBodyNode(doc);
    if (body_id == NULL_NODE || @as(usize, body_id) >= doc.nodes.items.len) {
        return res;
    }

    let pages: u32 = 1;
    let cur_y: f32 = 0.0;
    let has_content_on_page = false;
    let pending_break_after = false;

    let child = doc.nodes.items[@as(usize, body_id)].first_child;
    while (child != NULL_NODE && @as(usize, child) < doc.nodes.items.len) : (child = doc.nodes.items[@as(usize, child)].next_sibling) {
        const cid = @as(usize, child);
        const cnode = &doc.nodes.items[cid];
        if (cnode.node_type != NodeType.Element) continue;
        if (cid >= styles.len) continue;
        const st = &styles[cid];
        if (st.display.isNone()) continue;

        const rect = layout.getBoundingClientRect(child);
        const h = if (rect.height > 0.0) rect.height else 0.0;
        if (h <= 0.0 && !st.break_before.forcesPageBreak() && !st.break_after.forcesPageBreak()) {
            continue;
        }

        if (pending_break_after) {
            pages += 1;
            cur_y = 0.0;
            has_content_on_page = false;
            pending_break_after = false;
        }

        if (st.break_before.forcesPageBreak() && has_content_on_page) {
            pages += 1;
            cur_y = 0.0;
            has_content_on_page = false;
        }

        const avoid_inside = st.break_inside == BreakInsideValue.Avoid;
        if (h > 0.0) {
            if (has_content_on_page && cur_y + h > printable_h + 0.5) {
                if (avoid_inside || h <= printable_h + 0.5) {
                    // Move whole block to next page
                    pages += 1;
                    cur_y = h;
                    has_content_on_page = true;
                } else {
                    // Block taller than a single page and splittable
                    const rem = (cur_y + h) - printable_h;
                    const extra_pages: u32 = @as(u32, @intFromFloat(@ceil(rem / printable_h)));
                    pages += extra_pages;
                    cur_y = rem - @as(f32, @floatFromInt(extra_pages - 1)) * printable_h;
                    has_content_on_page = cur_y > 0.5;
                }
            } else {
                cur_y += h;
                if (cur_y > printable_h + 0.5 && !avoid_inside) {
                    const extra_pages: u32 = @as(u32, @intFromFloat(@ceil((cur_y - printable_h) / printable_h)));
                    pages += extra_pages;
                    cur_y = cur_y - @as(f32, @floatFromInt(extra_pages)) * printable_h;
                }
                has_content_on_page = true;
            }
        }

        if (st.break_after.forcesPageBreak()) {
            pending_break_after = true;
        }
    }

    res.page_count = if (pages < 1) 1 else pages;
    return res;
}

/// Emits a valid multi-page `%PDF-1.4` binary with `page_count` `/Type /Page` objects.
pub fn renderPdfAlloc(
    allocator: std.mem.Allocator,
    page_count: u32,
    media_width_pt: f32,
    media_height_pt: f32,
): ([]u8)! {
    const n_pages: u32 = if (page_count < 1) 1 else @min(page_count, 500);
    const mw: i32 = @max(@as(i32, 72), @as(i32, @intFromFloat(@round(media_width_pt))));
    const mh: i32 = @max(@as(i32, 72), @as(i32, @intFromFloat(@round(media_height_pt))));

    const est_cap: usize = @as(usize, 2048) + @as(usize, n_pages) * @as(usize, 160);
    let buf = allocator.alloc(u8, est_cap) catch return error.OutOfMemory;
    let pos: usize = 0;

    let offsets_opt = allocator.alloc(usize, @as(usize, n_pages) + @as(usize, 3)) catch null;
    if (offsets_opt == null) {
        allocator.free(buf);
        return error.OutOfMemory;
    }
    let offsets = offsets_opt.?;
    defer allocator.free(offsets);

    const hdr = "%PDF-1.4\n";
    @memcpy(buf[pos .. pos + hdr.len], hdr);
    pos += hdr.len;

    // Object 1: Catalog
    offsets[1] = pos;
    const obj1 = "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    @memcpy(buf[pos .. pos + obj1.len], obj1);
    pos += obj1.len;

    // Object 2: Pages
    offsets[2] = pos;
    const obj2_pre = "2 0 obj\n<< /Type /Pages /Kids [";
    @memcpy(buf[pos .. pos + obj2_pre.len], obj2_pre);
    pos += obj2_pre.len;

    let p: u32 = 0;
    let tmp: [128]u8 = undefined;
    while (p < n_pages) : (p += 1) {
        if (p > 0) {
            buf[pos] = ' ';
            pos += 1;
        }
        const kid = std.fmt.bufPrint(&tmp, "{d} 0 R", .{ p + 3 }) catch "";
        @memcpy(buf[pos .. pos + kid.len], kid);
        pos += kid.len;
    }
    const count_part = std.fmt.bufPrint(&tmp, "] /Count {d} >>\nendobj\n", .{ n_pages }) catch "";
    @memcpy(buf[pos .. pos + count_part.len], count_part);
    pos += count_part.len;

    // Objects 3..(2 + n_pages): individual Page objects
    p = 0;
    while (p < n_pages) : (p += 1) {
        const obj_id = p + 3;
        offsets[@as(usize, obj_id)] = pos;
        const page_obj = std.fmt.bufPrint(
            &tmp,
            "{d} 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 {d} {d}] >>\nendobj\n",
            .{ obj_id, mw, mh },
        ) catch "";
        @memcpy(buf[pos .. pos + page_obj.len], page_obj);
        pos += page_obj.len;
    }

    // XRef table
    const xref_pos = pos;
    const total_objs = n_pages + 3;
    const xref_hdr = std.fmt.bufPrint(&tmp, "xref\n0 {d}\n0000000000 65535 f \n", .{ total_objs }) catch "";
    @memcpy(buf[pos .. pos + xref_hdr.len], xref_hdr);
    pos += xref_hdr.len;

    let oi: usize = 1;
    while (oi < @as(usize, total_objs)) : (oi += 1) {
        const entry = std.fmt.bufPrint(&tmp, "{d:0>10} 00000 n \n", .{ offsets[oi] }) catch "";
        @memcpy(buf[pos .. pos + entry.len], entry);
        pos += entry.len;
    }

    const trailer = std.fmt.bufPrint(
        &tmp,
        "trailer\n<< /Size {d} /Root 1 0 R >>\nstartxref\n{d}\n%%EOF\n",
        .{ total_objs, xref_pos },
    ) catch "";
    @memcpy(buf[pos .. pos + trailer.len], trailer);
    pos += trailer.len;

    const exact_opt = allocator.alloc(u8, pos) catch null;
    if (exact_opt == null) {
        allocator.free(buf);
        return error.OutOfMemory;
    }
    const exact = exact_opt.?;
    @memcpy(exact, buf[0..pos]);
    allocator.free(buf);
    return exact;
}

fn fillRgbRect(
    rgb: []u8,
    buf_w: u32,
    buf_h: u32,
    rx: f32,
    ry: f32,
    rw: f32,
    rh: f32,
    cr: u8,
    cg: u8,
    cb: u8,
    ca: u8,
) {
    if (ca == 0 || rw <= 0.0 || rh <= 0.0) return;
    const x0: i32 = @max(@as(i32, 0), @as(i32, @intFromFloat(@floor(rx))));
    const y0: i32 = @max(@as(i32, 0), @as(i32, @intFromFloat(@floor(ry))));
    const x1: i32 = @min(@as(i32, @intCast(buf_w)), @as(i32, @intFromFloat(@ceil(rx + rw))));
    const y1: i32 = @min(@as(i32, @intCast(buf_h)), @as(i32, @intFromFloat(@ceil(ry + rh))));
    if (x0 >= x1 || y0 >= y1) return;

    const alpha: u32 = @as(u32, ca);
    const inv_alpha: u32 = @as(u32, 255) - alpha;
    let y: i32 = y0;
    while (y < y1) : (y += 1) {
        const row_off = @as(usize, @intCast(y)) * @as(usize, buf_w) * 3;
        let x: i32 = x0;
        while (x < x1) : (x += 1) {
            const idx = row_off + @as(usize, @intCast(x)) * 3;
            if (ca == 255) {
                rgb[idx] = cr;
                rgb[idx + 1] = cg;
                rgb[idx + 2] = cb;
            } else {
                rgb[idx] = @as(u8, @intCast((@as(u32, cr) * alpha + @as(u32, rgb[idx]) * inv_alpha) / 255));
                rgb[idx + 1] = @as(u8, @intCast((@as(u32, cg) * alpha + @as(u32, rgb[idx + 1]) * inv_alpha) / 255));
                rgb[idx + 2] = @as(u8, @intCast((@as(u32, cb) * alpha + @as(u32, rgb[idx + 2]) * inv_alpha) / 255));
            }
        }
    }
}

fn appendPdfEscapedText(out: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, text: []const u8): bool {
    let i: usize = 0;
    const limit = @min(text.len, @as(usize, 512));
    while (i < limit) : (i += 1) {
        const c = text[i];
        if (c == '(' || c == ')' || c == '\\') {
            out.append(allocator, '\\') catch return false;
            out.append(allocator, c) catch return false;
        } else if (c >= 0x20 && c <= 0x7E) {
            out.append(allocator, c) catch return false;
        } else if (c == '\n' || c == '\r' || c == '\t') {
            out.append(allocator, ' ') catch return false;
        }
    }
    return true;
}

/// Emits a valid multi-page `%PDF-1.4` binary with real content streams:
/// - Rasterized RGB24 `/XObject /Subtype /Image` with `/Filter /FlateDecode` per page
/// - Vector/text `/Contents` stream (`q ... /Im1 Do Q` + `BT /F1 ... Tj ET`)
pub fn renderPdfWithDocAlloc(
    allocator: std.mem.Allocator,
    doc: *const Document,
    styles: []const ComputedStyle,
    layout: *const LayoutTree,
    page_count: u32,
    width_pt: f32,
    height_pt: f32,
): ([]u8)! {
    const n_pages: u32 = if (page_count < 1) 1 else @min(page_count, 500);
    const mw: i32 = @max(@as(i32, 72), @as(i32, @intFromFloat(@round(width_pt))));
    const mh: i32 = @max(@as(i32, 72), @as(i32, @intFromFloat(@round(height_pt))));

    // Convert points (72 dpi) to CSS pixels (96 dpi) for rasterizing layout coordinates
    const page_w_px: u32 = @max(@as(u32, 72), @as(u32, @intFromFloat(@round(width_pt * (96.0 / 72.0)))));
    const page_h_px: u32 = @max(@as(u32, 72), @as(u32, @intFromFloat(@round(height_pt * (96.0 / 72.0)))));
    const raster_pages: u32 = @min(n_pages, @as(u32, 16));
    const px_to_pt: f32 = 72.0 / 96.0;

    let out = std.ArrayListUnmanaged(u8).empty;
    errdefer out.deinit(allocator);

    // Objects 1..3: Catalog, Pages, Helvetica Font; then 3 objects per page (Page, Content, Image)
    const total_objs: u32 = @as(u32, 4) + n_pages * @as(u32, 3);
    let offsets = allocator.alloc(usize, @as(usize, total_objs))?;
    defer allocator.free(offsets);

    out.appendSlice(allocator, "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n") catch return error.OutOfMemory;

    let tmp: [512]u8 = undefined;

    // Object 1: Catalog
    offsets[1] = out.items.len;
    out.appendSlice(allocator, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n") catch return error.OutOfMemory;

    // Object 2: Pages
    offsets[2] = out.items.len;
    out.appendSlice(allocator, "2 0 obj\n<< /Type /Pages /Kids [") catch return error.OutOfMemory;
    let p: u32 = 0;
    while (p < n_pages) : (p += 1) {
        if (p > 0) {
            out.append(allocator, ' ') catch return error.OutOfMemory;
        }
        const page_obj_id: u32 = @as(u32, 4) + p * @as(u32, 3);
        const kid = std.fmt.bufPrint(&tmp, "{d} 0 R", .{ page_obj_id }) catch "";
        out.appendSlice(allocator, kid) catch return error.OutOfMemory;
    }
    const count_part = std.fmt.bufPrint(&tmp, "] /Count {d} >>\nendobj\n", .{ n_pages }) catch "";
    out.appendSlice(allocator, count_part) catch return error.OutOfMemory;

    // Object 3: Standard Type1 Helvetica Font
    offsets[3] = out.items.len;
    out.appendSlice(allocator, "3 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\nendobj\n") catch return error.OutOfMemory;

    // Pre-allocate per-page RGB and zlib buffers
    const rgb_len: usize = @as(usize, page_w_px) * @as(usize, page_h_px) * 3;
    const z_bound: usize = compressBound(rgb_len);
    let page_rgb = allocator.alloc(u8, rgb_len)?;
    defer allocator.free(page_rgb);
    let z_buf = allocator.alloc(u8, z_bound)?;
    defer allocator.free(z_buf);

    let stream_buf = std.ArrayListUnmanaged(u8).empty;
    defer stream_buf.deinit(allocator);

    // Emit each page's Page object, Content stream, and Image XObject
    p = 0;
    while (p < n_pages) : (p += 1) {
        const page_obj_id: u32 = @as(u32, 4) + p * @as(u32, 3);
        const content_obj_id = page_obj_id + 1;
        const image_obj_id = page_obj_id + 2;

        const page_top_px: f32 = @as(f32, @floatFromInt(p)) * @as(f32, @floatFromInt(page_h_px));
        const page_bot_px: f32 = page_top_px + @as(f32, @floatFromInt(page_h_px));

        let page_has_img = false;
        let z_len: usize = 0;

        stream_buf.clearRetainingCapacity();

        if (p < raster_pages) {
            @memset(page_rgb, 255);
            let nid: usize = 1;
            while (nid < doc.nodes.items.len) : (nid += 1) {
                const node = &doc.nodes.items[nid];
                if (node.node_type != NodeType.Element) continue;
                if (nid >= styles.len) continue;
                const st = &styles[nid];
                if (st.display.isNone()) continue;
                const r = layout.getBoundingClientRect(@as(NodeId, @intCast(nid)));
                if (r.width <= 0.0 || r.height <= 0.0) continue;
                if (r.y + r.height <= page_top_px || r.y >= page_bot_px) continue;

                const local_y = r.y - page_top_px;
                if (st.background_color.a > 0) {
                    fillRgbRect(
                        page_rgb,
                        page_w_px,
                        page_h_px,
                        r.x,
                        local_y,
                        r.width,
                        r.height,
                        st.background_color.r,
                        st.background_color.g,
                        st.background_color.b,
                        st.background_color.a,
                    );
                }
                if (st.border_top_width > 0.0 && st.border_top_color.a > 0) {
                    fillRgbRect(
                        page_rgb,
                        page_w_px,
                        page_h_px,
                        r.x,
                        local_y,
                        r.width,
                        st.border_top_width,
                        st.border_top_color.r,
                        st.border_top_color.g,
                        st.border_top_color.b,
                        st.border_top_color.a,
                    );
                }
            }

            z_len = z_bound;
            const z_res = compress2(z_buf.ptr, &z_len, page_rgb.ptr, rgb_len, 1);
            if (z_res == 0 && z_len > 0) {
                page_has_img = true;
            }
        }

        if (page_has_img) {
            const img_cmd = std.fmt.bufPrint(&tmp, "q\n{d} 0 0 {d} 0 0 cm\n/Im1 Do\nQ\n", .{ mw, mh }) catch "";
            stream_buf.appendSlice(allocator, img_cmd) catch return error.OutOfMemory;
        } else {
            const bg_cmd = std.fmt.bufPrint(&tmp, "q\n1 1 1 rg\n0 0 {d} {d} re f\nQ\n", .{ mw, mh }) catch "";
            stream_buf.appendSlice(allocator, bg_cmd) catch return error.OutOfMemory;
        }

        // Emit selectable PDF text operators for Text nodes on this page
        let tnid: usize = 1;
        while (tnid < doc.nodes.items.len) : (tnid += 1) {
            const tnode = &doc.nodes.items[tnid];
            if (tnode.node_type != NodeType.Text || tnode.text_content.len == 0) continue;
            const trimmed = std.mem.trim(u8, tnode.text_content, " \t\r\n");
            if (trimmed.len == 0) continue;

            const pid = @as(usize, tnode.parent);
            if (pid == 0 || pid >= styles.len) continue;
            const pst = &styles[pid];
            if (pst.display.isNone()) continue;

            let tr = layout.getBoundingClientRect(@as(NodeId, @intCast(tnid)));
            if (tr.width <= 0.0 && tr.height <= 0.0) {
                tr = layout.getBoundingClientRect(@as(NodeId, @intCast(pid)));
            }
            if (tr.y + tr.height <= page_top_px || tr.y >= page_bot_px) continue;

            const fsz_px = if (pst.font_size_px > 0.0) pst.font_size_px else 16.0;
            const fsz_pt = fsz_px * px_to_pt;
            const tx_pt = tr.x * px_to_pt;
            const ty_pt = @as(f32, @floatFromInt(mh)) - ((tr.y - page_top_px) * px_to_pt) - fsz_pt;
            const cr: f32 = @as(f32, @floatFromInt(pst.color.r)) / 255.0;
            const cg: f32 = @as(f32, @floatFromInt(pst.color.g)) / 255.0;
            const cb: f32 = @as(f32, @floatFromInt(pst.color.b)) / 255.0;

            const t_pre = std.fmt.bufPrint(
                &tmp,
                "BT\n/F1 {d:.2} Tf\n{d:.3} {d:.3} {d:.3} rg\n{d:.2} {d:.2} Td\n(",
                .{ fsz_pt, cr, cg, cb, tx_pt, ty_pt },
            ) catch "";
            stream_buf.appendSlice(allocator, t_pre) catch return error.OutOfMemory;
            if (!appendPdfEscapedText(&stream_buf, allocator, trimmed)) return error.OutOfMemory;
            stream_buf.appendSlice(allocator, ") Tj\nET\n") catch return error.OutOfMemory;
        }

        // Page object
        offsets[@as(usize, page_obj_id)] = out.items.len;
        if (page_has_img) {
            const p_str = std.fmt.bufPrint(
                &tmp,
                "{d} 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 {d} {d}] /Resources << /Font << /F1 3 0 R >> /XObject << /Im1 {d} 0 R >> >> /Contents {d} 0 R >>\nendobj\n",
                .{ page_obj_id, mw, mh, image_obj_id, content_obj_id },
            ) catch "";
            out.appendSlice(allocator, p_str) catch return error.OutOfMemory;
        } else {
            const p_str = std.fmt.bufPrint(
                &tmp,
                "{d} 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 {d} {d}] /Resources << /Font << /F1 3 0 R >> >> /Contents {d} 0 R >>\nendobj\n",
                .{ page_obj_id, mw, mh, content_obj_id },
            ) catch "";
            out.appendSlice(allocator, p_str) catch return error.OutOfMemory;
        }

        // Content stream object
        offsets[@as(usize, content_obj_id)] = out.items.len;
        const c_hdr = std.fmt.bufPrint(
            &tmp,
            "{d} 0 obj\n<< /Length {d} >>\nstream\n",
            .{ content_obj_id, stream_buf.items.len },
        ) catch "";
        out.appendSlice(allocator, c_hdr) catch return error.OutOfMemory;
        out.appendSlice(allocator, stream_buf.items) catch return error.OutOfMemory;
        out.appendSlice(allocator, "\nendstream\nendobj\n") catch return error.OutOfMemory;

        // Image XObject (or null placeholder object to keep xref contiguous)
        offsets[@as(usize, image_obj_id)] = out.items.len;
        if (page_has_img) {
            const i_hdr = std.fmt.bufPrint(
                &tmp,
                "{d} 0 obj\n<< /Type /XObject /Subtype /Image /Width {d} /Height {d} /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /FlateDecode /Length {d} >>\nstream\n",
                .{ image_obj_id, page_w_px, page_h_px, z_len },
            ) catch "";
            out.appendSlice(allocator, i_hdr) catch return error.OutOfMemory;
            out.appendSlice(allocator, z_buf[0..z_len]) catch return error.OutOfMemory;
            out.appendSlice(allocator, "\nendstream\nendobj\n") catch return error.OutOfMemory;
        } else {
            const dummy = std.fmt.bufPrint(&tmp, "{d} 0 obj\nnull\nendobj\n", .{ image_obj_id }) catch "";
            out.appendSlice(allocator, dummy) catch return error.OutOfMemory;
        }
    }

    // XRef table
    const xref_pos = out.items.len;
    const xref_hdr = std.fmt.bufPrint(&tmp, "xref\n0 {d}\n0000000000 65535 f \n", .{ total_objs }) catch "";
    out.appendSlice(allocator, xref_hdr) catch return error.OutOfMemory;

    let oi: usize = 1;
    while (oi < @as(usize, total_objs)) : (oi += 1) {
        const entry = std.fmt.bufPrint(&tmp, "{d:0>10} 00000 n \n", .{ offsets[oi] }) catch "";
        out.appendSlice(allocator, entry) catch return error.OutOfMemory;
    }

    const trailer = std.fmt.bufPrint(
        &tmp,
        "trailer\n<< /Size {d} /Root 1 0 R >>\nstartxref\n{d}\n%%EOF\n",
        .{ total_objs, xref_pos },
    ) catch "";
    out.appendSlice(allocator, trailer) catch return error.OutOfMemory;

    return out.toOwnedSlice(allocator);
}
