// src/render/pdf.cp
// Pure CP CSS Paged Media Paginator & Multi-Page PDF-1.4 Generator.
// Consumes Document, StyleSheet (@page margins), ComputedStyle (break-before/after/inside),
// and LayoutTree to compute exact page breaks and emit a valid multi-page PDF binary.

import { Document, NodeType, NodeId, NULL_NODE } from "../dom/dom.cp";
import { StyleSheet, ComputedStyle } from "../css/cascade.cp";
import { BreakValue, BreakInsideValue } from "../css/paged_media.cp";
import { LayoutTree } from "../layout/layout.cp";

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
