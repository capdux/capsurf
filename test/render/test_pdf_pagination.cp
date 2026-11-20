// test/render/test_pdf_pagination.cp
// Pure CP unit tests for CSS Paged Media (@page margins), Fragmentation (break-before/inside),
// legacy ISO-8859-15 stylesheet decoding, and multi-page PDF-1.4 generation.

import { Document } from "../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";
import { LayoutTree } from "../../src/layout/layout.cp";
import { PdfOptions, computePdfPagination, renderPdfAlloc } from "../../src/render/pdf.cp";

fn countPdfPages(pdf: []const u8): u32 {
    let count: u32 = 0;
    let i: usize = 0;
    while (i + 12 <= pdf.len) : (i += 1) {
        if (std.mem.startsWith(u8, pdf[i..], "/Type /Page")) {
            const next = if (i + 11 < pdf.len) pdf[i + 11] else ' ';
            if (next != 's') {
                count += 1;
            }
        }
    }
    return count;
}

fn runPaginationCase(html: []const u8, css: []const u8, opts: PdfOptions): u32 {
    let doc = Document.init(std.heap.page_allocator) catch return 0;
    defer doc.deinit();
    doc.parseHtml(html) catch return 0;

    const sheet = StyleSheet.parse(css, 816.0, false);
    const count = @max(@as(usize, 1), doc.nodeCount());
    const styles = std.heap.page_allocator.alloc(ComputedStyle, count) catch return 0;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let lt = LayoutTree.init(std.heap.page_allocator, 816.0, 1056.0) catch return 0;
    defer lt.deinit();
    lt.computeLayout(&doc, styles) catch return 0;

    const pg = computePdfPagination(&doc, &sheet, styles, &lt, opts);
    const pdf = renderPdfAlloc(std.heap.page_allocator, pg.page_count, pg.media_width_pt, pg.media_height_pt) catch return 0;
    defer std.heap.page_allocator.free(pdf);

    const parsed_pages = countPdfPages(pdf);
    if (parsed_pages != pg.page_count) return 0;
    return parsed_pages;
}

fn testBreakBefore() {
    const html = "<html><body><section>one</section><section>two</section><section>three</section><div class=\"keep\">keep-together</div></body></html>";
    const css =
        "body { margin: 0; font: 16px/1.5 sans-serif; }\n" ++
        "section { height: 400px; break-before: page; background: #eee; }\n" ++
        "section:first-of-type { break-before: auto; }\n" ++
        ".keep { height: 300px; break-inside: avoid; background: #cde; }\n";
    const pages = runPaginationCase(html, css, PdfOptions{
        paper_width_in: 8.5,
        paper_height_in: 11.0,
    });
    std.debug.assert(pages == 3);
}

fn testBreakInside() {
    const html = "<html><body><div class=\"filler\"></div><div class=\"keep\"></div><div class=\"keep\"></div></body></html>";
    const css =
        "body { margin: 0; }\n" ++
        ".filler { height: 500px; background: #eee; }\n" ++
        ".keep { height: 600px; break-inside: avoid; background: #cde; }\n" ++
        ".keep + .keep { background: #dec; }\n";
    const pages = runPaginationCase(html, css, PdfOptions{
        paper_width_in: 8.5,
        paper_height_in: 11.0,
    });
    std.debug.assert(pages == 3);
}

fn testPageMargins() {
    let html_buf: [4096]u8 = undefined;
    let pos: usize = 0;
    const head = "<html><body>";
    @memcpy(html_buf[pos .. pos + head.len], head);
    pos += head.len;
    let bi: usize = 0;
    const p_tag = "<p style=\"height:100px;margin:0\">block</p>";
    while (bi < 40) : (bi += 1) {
        @memcpy(html_buf[pos .. pos + p_tag.len], p_tag);
        pos += p_tag.len;
    }
    const tail = "</body></html>";
    @memcpy(html_buf[pos .. pos + tail.len], tail);
    pos += tail.len;
    const html = html_buf[0..pos];

    const p_no_margin = runPaginationCase(html, "body { margin: 0; }", PdfOptions{
        paper_width_in: 8.5,
        paper_height_in: 11.0,
    });
    std.debug.assert(p_no_margin == 4);

    const p_2in = runPaginationCase(html, "@page { margin: 2in }\nbody { margin: 0; }", PdfOptions{
        paper_width_in: 8.5,
        paper_height_in: 11.0,
    });
    std.debug.assert(p_2in == 7);
}

fn testLegacyCharset()! {
    const latin1_css = "/* Configura\xe7\xe3o espec\xedfica (legacy sheet) */\n#legacy { background-color: rgb(9, 87, 65); }\n";
    let doc = Document.init(std.heap.page_allocator)?;
    defer doc.deinit();
    doc.parseHtml("<html><body><div id=\"legacy\">legacy</div></body></html>")?;
    const sheet = StyleSheet.parse(latin1_css, 800.0, false);
    const count = @max(@as(usize, 1), doc.nodeCount());
    const styles = std.heap.page_allocator.alloc(ComputedStyle, count)?;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let found = false;
    let nid: usize = 0;
    while (nid < doc.nodes.items.len) : (nid += 1) {
        if (id_val := doc.nodes.items[nid].getAttribute("id")) {
            if (std.mem.eql(u8, id_val, "legacy")) {
                const bg = styles[nid].background_color;
                std.debug.assert(bg.r == 9 && bg.g == 87 && bg.b == 65 && bg.a == 255);
                found = true;
            }
        }
    }
    std.debug.assert(found);
}

pub fn main()! {
    testBreakBefore();
    testBreakInside();
    testPageMargins();
    testLegacyCharset()?;
    std.debug.print("test_pdf_pagination: ALL 4 ASSERTION GROUPS PASSED\n", .{});
}
