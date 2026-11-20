// test/tty/test_tty_render_and_selection_edge.cp
// CapSurf Pure CP White-Box Unit Tests:
// TTY Grid Cell Boundaries, Nav Params & Stylesheet Buffers,
// ScrollOffset & Viewport/Clip Clamping, SelectionSpan Clamping & Wide/Malformed UTF-8 Text Rasterization.

import {
    TTY_CELL_PX_MIN,
    TTY_CELL_PX_MAX,
    DEFAULT_TTY_CELL_WIDTH_PX,
    DEFAULT_TTY_CELL_HEIGHT_PX,
    TtyNavState,
    parseCell,
    applyTtyNavParams,
    ttyFocusStylesheet,
    ttyUaStylesheet,
} from "../../src/tty/nav_params.cp";
import { Document, NodeId, NodeType, NULL_NODE } from "../../src/dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../../src/css/cascade.cp";
import { LayoutTree, Rect } from "../../src/layout/layout.cp";
import {
    Canvas,
    ClipRect,
    SelectionSpan,
    ScrollOffset,
    ScreenshotOptions,
    renderDocumentToPng,
} from "../../src/render/painter.cp";
import { PngCodec } from "../../src/render/png.cp";
import { CssColor } from "cap:css";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn findElementById(doc: *const Document, target_id: []const u8): NodeId {
    let idx: usize = 1;
    while (idx < doc.nodeCount()) : (idx += 1) {
        const node = &doc.nodes.items[idx];
        if (node.node_type == NodeType.Element && node.hasId(target_id)) {
            return @as(NodeId, @intCast(idx));
        }
    }
    return NULL_NODE;
}

fn findFirstTextChild(doc: *const Document, parent_id: NodeId): NodeId {
    if (parent_id == NULL_NODE || @as(usize, parent_id) >= doc.nodeCount()) return NULL_NODE;
    let c = doc.nodes.items[@as(usize, parent_id)].first_child;
    while (c != NULL_NODE && @as(usize, c) < doc.nodeCount()) {
        if (doc.nodes.items[@as(usize, c)].node_type == NodeType.Text) {
            return c;
        }
        c = doc.nodes.items[@as(usize, c)].next_sibling;
    }
    return NULL_NODE;
}

fn countCanvasNonBg(canvas: *const Canvas, x0: i32, y0: i32, x1: i32, y1: i32, bg: CssColor): usize {
    let count: usize = 0;
    let py = y0;
    while (py < y1) : (py += 1) {
        let px = x0;
        while (px < x1) : (px += 1) {
            const c = canvas.getPixel(px, py);
            if (c.r != bg.r || c.g != bg.g || c.b != bg.b || c.a != bg.a) {
                count += 1;
            }
        }
    }
    return count;
}

fn testTtyGridCellAndNavBoundaries() {
    std.debug.print("1. TTY Grid Cell Boundaries & Nav Params Clamping\n", .{});
    let w: u32 = 12;
    let h: u32 = 24;

    // Exact min/max and leading-zero cell dimensions
    check("parseCell 1x1", parseCell("1x1", &w, &h));
    check("1x1 w == min", w == TTY_CELL_PX_MIN);
    check("1x1 h == min", h == TTY_CELL_PX_MIN);

    check("parseCell 64x64", parseCell("64x64", &w, &h));
    check("64x64 w == max", w == TTY_CELL_PX_MAX);
    check("64x64 h == max", h == TTY_CELL_PX_MAX);

    check("parseCell 08x17", parseCell("08x17", &w, &h));
    check("08x17 w == default", w == DEFAULT_TTY_CELL_WIDTH_PX);
    check("08x17 h == default", h == DEFAULT_TTY_CELL_HEIGHT_PX);

    // Out-of-bounds and malformed cells must fail without overwriting w/h
    check("reject 0x17", !parseCell("0x17", &w, &h));
    check("reject 8x0", !parseCell("8x0", &w, &h));
    check("reject 65x17", !parseCell("65x17", &w, &h));
    check("reject 8x65", !parseCell("8x65", &w, &h));
    check("reject empty", !parseCell("", &w, &h));
    check("reject x", !parseCell("x", &w, &h));
    check("reject 8x", !parseCell("8x", &w, &h));
    check("reject x17", !parseCell("x17", &w, &h));
    check("reject 8X17", !parseCell("8X17", &w, &h));
    check("reject -1x17", !parseCell("-1x17", &w, &h));
    check("reject 8x17x2", !parseCell("8x17x2", &w, &h));
    check("w preserved", w == DEFAULT_TTY_CELL_WIDTH_PX);
    check("h preserved", h == DEFAULT_TTY_CELL_HEIGHT_PX);

    // Query parameter clamping and trailing '&' / missing '=' resilience
    let state = TtyNavState{};
    const q = "?&ks_tty_css=0&ks_tty_css=99&ks_color_scheme=dark&ks_tty_cell=64x1&invalid_key&&ks_color_scheme=unknown&";
    check("applyTtyNavParams returns true", applyTtyNavParams(q, &state));
    check("tty_ua_css clamped to 2", state.tty_ua_css.? == 2);
    check("dark_color_scheme == true", state.dark_color_scheme.? == true);
    check("tty_cell_w == 64", state.tty_cell_w.? == 64);
    check("tty_cell_h == 1", state.tty_cell_h.? == 1);

    // Light scheme override, invalid cell preserved
    check("apply light + invalid cell", applyTtyNavParams("ks_color_scheme=light&ks_tty_cell=0x0", &state));
    check("dark_color_scheme == false", state.dark_color_scheme.? == false);
    check("tty_cell_w preserved", state.tty_cell_w.? == 64);
    check("tty_cell_h preserved", state.tty_cell_h.? == 1);
}

fn testTtyStylesheetBuffersAndSchemeColors() {
    std.debug.print("2. TTY Stylesheet Buffers & Color Scheme Resolution\n", .{});
    const focus = ttyFocusStylesheet();
    check("focus outline", std.mem.indexOf(u8, focus, "outline: 1px dotted currentColor") != null);

    // Zero-length and undersized buffers must return empty slice
    let empty_backing: [1]u8 = [0];
    check("zero-length buf returns empty", ttyUaStylesheet(1, 17, 8, empty_backing[0..0]).len == 0);

    let tiny_buf: [32]u8 = [0; 32];
    check("tiny buf returns empty", ttyUaStylesheet(2, 17, 8, tiny_buf[0..]).len == 0);

    // Exact-fit boundary: level 1 fits in buffer, level 2 exceeds by 1 byte -> returns empty
    let buf: [512]u8 = [0; 512];
    const css1 = ttyUaStylesheet(1, 17, 8, buf[0..]);
    check("css1 non-empty", css1.len > 0);
    check("css1 cell-w", std.mem.indexOf(u8, css1, "--ks-tty-cell-w: 8px") != null);
    check("css1 cell-h", std.mem.indexOf(u8, css1, "--ks-tty-cell-h: 17px") != null);
    check("css1 omits overflow", std.mem.indexOf(u8, css1, "overflow: hidden") == null);

    const css2 = ttyUaStylesheet(2, 17, 8, buf[0..]);
    check("css2 longer than css1", css2.len > css1.len);
    check("css2 includes overflow", std.mem.indexOf(u8, css2, "overflow: hidden") != null);

    // Truncated buffer just 1 byte short of css2.len must return empty
    const tight_slice = buf[0 .. css2.len - 1];
    check("1-byte short buf returns empty", ttyUaStylesheet(2, 17, 8, tight_slice).len == 0);

    // ComputedStyle paint extraction under viewport dimensions
    let cs = ComputedStyle{};
    cs.color = CssColor.rgb(230, 240, 250);
    cs.background_color = CssColor.rgb(16, 18, 22);
    const paint = cs.extractPaintStyle(80.0, 16.0, 800.0, 600.0);
    check("extracted text_color r", paint.text_color.r == 230);
    check("extracted text_color b", paint.text_color.b == 250);
    check("extracted bg_color r", paint.bg_color.r == 16);
    check("extracted bg_color b", paint.bg_color.b == 22);
}

fn testScrollOffsetAndViewportClipClamping() {
    std.debug.print("3. ScrollOffset & Viewport Clipping Boundaries\n", .{});
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const html = "<!DOCTYPE html><html><body style=\"margin: 0;\">" ++
        "<div id=\"scroller\" style=\"width: 100px; height: 100px; overflow: hidden; background-color: #ffffff;\">" ++
        "<div id=\"box\" style=\"width: 40px; height: 40px; background-color: #ef4444;\"></div>" ++
        "</div></body></html>";
    doc.parseHtml(html) catch unreachable;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount()) catch unreachable;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let layout = LayoutTree.init(std.heap.page_allocator, 200.0, 200.0) catch unreachable;
    defer layout.deinit();
    layout.computeLayout(&doc, styles) catch unreachable;

    let rects = std.heap.page_allocator.alloc(Rect, doc.nodeCount()) catch unreachable;
    defer std.heap.page_allocator.free(rects);
    let nid: usize = 0;
    while (nid < doc.nodeCount()) : (nid += 1) {
        rects[nid] = layout.getBoundingClientRect(@as(u32, @intCast(nid)));
    }

    const scroller_id = findElementById(&doc, "scroller");
    check("scroller_id valid", scroller_id != NULL_NODE);

    const clear_bg = CssColor.rgb(10, 20, 30);

    // 1. Negative & out-of-viewport clip_x/clip_y clamping
    const clip_opts = ScreenshotOptions{
        viewport_w: 200,
        viewport_h: 200,
        compress_level: 1,
        clear_color: clear_bg,
        clip_x: -10,
        clip_y: -10,
        clip_w: 30,
        clip_h: 30,
    };
    const clip_png = renderDocumentToPng(std.heap.page_allocator, &doc, styles, rects, clip_opts) catch unreachable;
    defer std.heap.page_allocator.free(clip_png);
    if (img := PngCodec.decode(clip_png)) {
        check("clip width == 30", img.width == 30);
        check("clip height == 30", img.height == 30);
        // (-10, -10) maps outside canvas -> remains clear_color
        const oob_px = img.getPixel(2, 2);
        check("oob pixel r == 10", oob_px.r == 10);
        check("oob pixel g == 20", oob_px.g == 20);
        check("oob pixel b == 30", oob_px.b == 30);
        // (15, 15) in cropped image maps to (5, 5) on canvas -> red #ef4444 (239, 68, 68)
        const box_px = img.getPixel(15, 15);
        check("box pixel r == 239", box_px.r == 239);
        check("box pixel g == 68", box_px.g == 68);
        check("box pixel b == 68", box_px.b == 68);
    } else {
        check("decode clip_png", false);
    }

    // 2. ScrollOffset shifts child out of top-left corner and clips against container
    const scroll_arr: [1]ScrollOffset = [
        ScrollOffset{ node_id: scroller_id, scroll_left: 25.0, scroll_top: 25.0 },
    ];
    const scroll_opts = ScreenshotOptions{
        viewport_w: 200,
        viewport_h: 200,
        compress_level: 1,
        clear_color: clear_bg,
        scroll_offsets: scroll_arr[0..1],
    };
    const scroll_png = renderDocumentToPng(std.heap.page_allocator, &doc, styles, rects, scroll_opts) catch unreachable;
    defer std.heap.page_allocator.free(scroll_png);
    if (simg := PngCodec.decode(scroll_png)) {
        // Box was 40x40 at (0, 0); scrolled by (25, 25) -> visible region is [0, 15) x [0, 15)
        const in_px = simg.getPixel(5, 5);
        check("scrolled in_px r == 239", in_px.r == 239);
        check("scrolled in_px g == 68", in_px.g == 68);
        check("scrolled in_px b == 68", in_px.b == 68);
        // At (20, 20), box has scrolled past -> container white #ffffff
        const out_px = simg.getPixel(20, 20);
        check("scrolled out_px r == 255", out_px.r == 255);
        check("scrolled out_px g == 255", out_px.g == 255);
        check("scrolled out_px b == 255", out_px.b == 255);
    } else {
        check("decode scroll_png", false);
    }
}

fn testSelectionSpanClampingAndWideText() {
    std.debug.print("4. SelectionSpan Clamping & Wide/Malformed UTF-8 Text Rasterization\n", .{});
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const html = "<!DOCTYPE html><html><body style=\"margin: 0;\">" ++
        "<div id=\"line\" style=\"width: 240px; height: 32px; background-color: #ffffff; color: #111111;\">Hello</div>" ++
        "</body></html>";
    doc.parseHtml(html) catch unreachable;

    const sheet = StyleSheet{};
    let styles = std.heap.page_allocator.alloc(ComputedStyle, doc.nodeCount()) catch unreachable;
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(&doc, &sheet, styles, 16.0);

    let layout = LayoutTree.init(std.heap.page_allocator, 240.0, 100.0) catch unreachable;
    defer layout.deinit();
    layout.computeLayout(&doc, styles) catch unreachable;

    let rects = std.heap.page_allocator.alloc(Rect, doc.nodeCount()) catch unreachable;
    defer std.heap.page_allocator.free(rects);
    let nid: usize = 0;
    while (nid < doc.nodeCount()) : (nid += 1) {
        rects[nid] = layout.getBoundingClientRect(@as(u32, @intCast(nid)));
    }

    const line_id = findElementById(&doc, "line");
    const text_id = findFirstTextChild(&doc, line_id);
    check("text_id valid", text_id != NULL_NODE);

    // Invalid / empty / out-of-range SelectionSpans must not paint highlight
    const invalid_spans: [3]SelectionSpan = [
        SelectionSpan{ node_id: text_id, start_offset: 3, end_offset: 3 },
        SelectionSpan{ node_id: text_id, start_offset: 4, end_offset: 1 },
        SelectionSpan{ node_id: text_id, start_offset: 50, end_offset: 100 },
    ];
    const base_opts = ScreenshotOptions{
        viewport_w: 240,
        viewport_h: 100,
        compress_level: 1,
        clear_color: CssColor.rgb(255, 255, 255),
        selection_spans: invalid_spans[0..3],
    };
    const base_png = renderDocumentToPng(std.heap.page_allocator, &doc, styles, rects, base_opts) catch unreachable;
    defer std.heap.page_allocator.free(base_png);

    // Clamped SelectionSpan (end_offset 999 -> clamped to text_content.len = 5)
    const valid_spans: [1]SelectionSpan = [
        SelectionSpan{ node_id: text_id, start_offset: 0, end_offset: 999 },
    ];
    const sel_opts = ScreenshotOptions{
        viewport_w: 240,
        viewport_h: 100,
        compress_level: 1,
        clear_color: CssColor.rgb(255, 255, 255),
        selection_spans: valid_spans[0..1],
    };
    const sel_png = renderDocumentToPng(std.heap.page_allocator, &doc, styles, rects, sel_opts) catch unreachable;
    defer std.heap.page_allocator.free(sel_png);

    if (bimg := PngCodec.decode(base_png)) {
        if (simg := PngCodec.decode(sel_png)) {
            // Baseline (no valid selection) at (12, 20) is pure white; with selection it has blue tint (B > R)
            const b_px = bimg.getPixel(12, 20);
            const s_px = simg.getPixel(12, 20);
            check("baseline pixel r == 255", b_px.r == 255);
            check("baseline pixel b == 255", b_px.b == 255);
            check("selection pixel has blue tint", s_px.b > s_px.r);
            // Beyond clamped 5 chars (5*8 + 2 = 42), e.g. at x=80, y=20 remains white
            const after_px = simg.getPixel(80, 20);
            check("after selection r == 255", after_px.r == 255);
            check("after selection b == 255", after_px.b == 255);
        } else {
            check("decode sel_png", false);
        }
    } else {
        check("decode base_png", false);
    }

    // Wide CJK + Emoji + Malformed UTF-8 bytes under tight ClipRect
    const bg = CssColor.rgb(0, 0, 0);
    const fg = CssColor.rgb(240, 240, 240);
    let canvas = Canvas.init(std.heap.page_allocator, 64, 32, bg) catch unreachable;
    defer canvas.deinit();

    const clip = ClipRect{ x0: 4, y0: 4, x1: 28, y1: 24 };
    canvas.drawScaledTextClipped(4.0, 4.0, "A中あ✅\x80\xFF", 12.0, fg, clip);
    check("wide/mixed text painted inside clip", countCanvasNonBg(&canvas, 4, 4, 28, 24, bg) > 0);
    // Exterior of clip rect must remain untouched
    check("clip right exterior untouched", countCanvasNonBg(&canvas, 28, 4, 64, 32, bg) == 0);
    check("clip top exterior untouched", countCanvasNonBg(&canvas, 0, 0, 64, 4, bg) == 0);
}

pub fn main() {
    std.debug.print("\n=== TTY Render & Selection Edge Unit Tests ===\n", .{});
    testTtyGridCellAndNavBoundaries();
    testTtyStylesheetBuffersAndSchemeColors();
    testScrollOffsetAndViewportClipClamping();
    testSelectionSpanClampingAndWideText();
    std.debug.print("\nAll 4 TTY Render & Selection Edge tests passed!\n", .{});
}
