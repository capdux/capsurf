// src/render/painter.cp
// CapSurf 2D Software Rasterizer & Screenshot Entrypoint (Milestone 5)

import { Document, NodeData, NodeType, NodeId, NULL_NODE } from "../dom/dom.cp";
import { ComputedStyle, ResolvedPaintStyle } from "../css/cascade.cp";
import { Rect } from "../layout/layout.cp";
import { CssColor } from "cap:css";
import { PngCodec } from "./png.cp";

/// Exported aliases to align with style_extractor and test harnesses
pub type Color = CssColor;
pub type PaintStyle = ResolvedPaintStyle;

pub struct CanvasBitmap {
    node_id: NodeId = NULL_NODE,
    width: u32 = 0,
    height: u32 = 0,
    pixels: []const u8 = [],
}

pub struct ScreenshotOptions {
    viewport_w: u32 = 1280;
    viewport_h: u32 = 720;
    /// PNG 压缩级别 (0 = 不压缩极速直出, 1 = 最快 RLE 级压缩, 6 = 默认平衡压缩, 9 = 最高极限压缩)
    compress_level: u8 = 6;
    clear_color: CssColor = CssColor.rgb(255, 255, 255);
    focused_node: NodeId = NULL_NODE;
    caret_pos: usize = 0;
    canvas_bitmaps: []const CanvasBitmap = [],
}

pub struct Canvas {
    width: u32;
    height: u32;
    pixels: []u8;
    allocator: std.mem.Allocator;

    static fn init(allocator: std.mem.Allocator, width: u32, height: u32, bg: CssColor): Canvas! {
        const byte_len = @as(usize, width) * @as(usize, height) * 4;
        let pixels = allocator.alloc(u8, byte_len)?;
        let idx: usize = 0;
        while (idx < byte_len) : (idx += 4) {
            pixels[idx] = bg.r;
            pixels[idx + 1] = bg.g;
            pixels[idx + 2] = bg.b;
            pixels[idx + 3] = bg.a;
        }
        return Canvas {
            width: width,
            height: height,
            pixels: pixels,
            allocator: allocator,
        };
    }

    fn deinit(*_) {
        _.allocator.free(_.pixels);
    }

    fn getPixel(x: i32, y: i32): CssColor {
        if (x < 0 || y < 0 || x >= @as(i32, _.width) || y >= @as(i32, _.height)) {
            return CssColor.rgba(0, 0, 0, 0);
        }
        const idx = (@as(usize, @intCast(y)) * @as(usize, _.width) + @as(usize, @intCast(x))) * 4;
        return CssColor.rgba(_.pixels[idx], _.pixels[idx + 1], _.pixels[idx + 2], _.pixels[idx + 3]);
    }

    fn blendPixel(*_, x: i32, y: i32, src: CssColor) {
        if (x < 0 || y < 0 || x >= @as(i32, _.width) || y >= @as(i32, _.height) || src.a == 0) {
            return;
        }
        const idx = (@as(usize, @intCast(y)) * @as(usize, _.width) + @as(usize, @intCast(x))) * 4;
        if (src.a == 255) {
            _.pixels[idx] = src.r;
            _.pixels[idx + 1] = src.g;
            _.pixels[idx + 2] = src.b;
            _.pixels[idx + 3] = 255;
            return;
        }
        const a = @as(u32, src.a);
        const inv_a = 255 - a;
        _.pixels[idx] = @as(u8, @intCast((@as(u32, src.r) * a + @as(u32, _.pixels[idx]) * inv_a) / 255));
        _.pixels[idx + 1] = @as(u8, @intCast((@as(u32, src.g) * a + @as(u32, _.pixels[idx + 1]) * inv_a) / 255));
        _.pixels[idx + 2] = @as(u8, @intCast((@as(u32, src.b) * a + @as(u32, _.pixels[idx + 2]) * inv_a) / 255));
        _.pixels[idx + 3] = 255;
    }

    /// SDF 圆角抗锯齿矩形填充 + 四周边框一体绘制
    fn paintBox(*_, rect: Rect, style: ResolvedPaintStyle) {
        if (!rect.visible || rect.width <= 0.0 || rect.height <= 0.0) {
            return;
        }

        const x0 = @max(0, @as(i32, @intFromFloat(@floor(rect.x))));
        const y0 = @max(0, @as(i32, @intFromFloat(@floor(rect.y))));
        const x1 = @min(@as(i32, _.width), @as(i32, @intFromFloat(@ceil(rect.x + rect.width))));
        const y1 = @min(@as(i32, _.height), @as(i32, @intFromFloat(@ceil(rect.y + rect.height))));

        const cx = rect.x + rect.width * 0.5;
        const cy = rect.y + rect.height * 0.5;
        const half_w = rect.width * 0.5;
        const half_h = rect.height * 0.5;
        const r = @min(style.border_radius, @min(half_w, half_h));

        const has_gradient = style.conic_gradient.has_gradient;

        let py = y0;
        while (py < y1) : (py += 1) {
            let px = x0;
            while (px < x1) : (px += 1) {
                const px_f = @as(f32, @floatFromInt(px)) + 0.5;
                const py_f = @as(f32, @floatFromInt(py)) + 0.5;
                const dx = @abs(px_f - cx) - (half_w - r);
                const dy = @abs(py_f - cy) - (half_h - r);
                const out_dist = @sqrt(@max(dx, 0.0) * @max(dx, 0.0) + @max(dy, 0.0) * @max(dy, 0.0)) + @min(@max(dx, dy), 0.0) - r;

                if (out_dist < 0.5) {
                    const edge_cov = std.math.clamp(0.5 - out_dist, 0.0, 1.0);
                    let final_color = if (has_gradient)
                        style.conic_gradient.sampleAt(rect.x, rect.y, rect.width, rect.height, px_f, py_f)
                    else
                        style.bg_color;

                    const neg_bw = 0.0 - style.border_width;
                    if (style.border_width > 0.0 && out_dist >= neg_bw) {
                        final_color = style.border_color;
                    }
                    if (final_color.a > 0) {
                        const blended_a = @as(u8, @intFromFloat(@as(f32, @floatFromInt(final_color.a)) * edge_cov));
                        _.blendPixel(px, py, CssColor.rgba(final_color.r, final_color.g, final_color.b, blended_a));
                    }
                }
            }
        }
    }

    fn drawText(*_, x: f32, y: f32, text: []const u8, color: CssColor) {
        if (color.a == 0 || text.len == 0) {
            return;
        }
        let cursor_x = @as(i32, @intFromFloat(x));
        const base_y = @as(i32, @intFromFloat(y));

        for (ch of text) {
            if (ch == '\n' || ch == '\r') continue;
            if (ch > 32 && ch < 127) {
                const glyph = getAsciiGlyph8x8(ch);
                let row: usize = 0;
                while (row < 8) : (row += 1) {
                    const bits = glyph[row];
                    let col: usize = 0;
                    while (col < 8) : (col += 1) {
                        const shift: u3 = @intCast(7 - col);
                        if (((bits >> shift) & 1) == 1) {
                            _.blendPixel(cursor_x + @as(i32, @intCast(col)), base_y + @as(i32, @intCast(row)) + 2, color);
                        }
                    }
                }
            }
            cursor_x += 8;
        }
    }

    fn blitRgba(*_, dst_x: i32, dst_y: i32, src_pixels: []const u8, src_w: u32, src_h: u32) {
        let sy: u32 = 0;
        while (sy < src_h) : (sy += 1) {
            let py = dst_y + @as(i32, @intCast(sy));
            if (py < 0 || py >= @as(i32, @intCast(_.height))) continue;
            let sx: u32 = 0;
            while (sx < src_w) : (sx += 1) {
                let px = dst_x + @as(i32, @intCast(sx));
                if (px < 0 || px >= @as(i32, @intCast(_.width))) continue;
                let s_idx = (@as(usize, sy) * @as(usize, src_w) + @as(usize, sx)) * 4;
                if (s_idx + 3 >= src_pixels.len) break;
                let a = src_pixels[s_idx + 3];
                if (a == 0) continue;
                if (a == 255) {
                    let d_idx = (@as(usize, @intCast(py)) * @as(usize, _.width) + @as(usize, @intCast(px))) * 4;
                    _.pixels[d_idx] = src_pixels[s_idx];
                    _.pixels[d_idx + 1] = src_pixels[s_idx + 1];
                    _.pixels[d_idx + 2] = src_pixels[s_idx + 2];
                    _.pixels[d_idx + 3] = 255;
                } else {
                    let c = CssColor.rgba(src_pixels[s_idx], src_pixels[s_idx + 1], src_pixels[s_idx + 2], a);
                    _.blendPixel(px, py, c);
                }
            }
        }
    }

    fn blitScaledRgba(*_, dst_x: i32, dst_y: i32, dst_w: u32, dst_h: u32, src_pixels: []const u8, src_w: u32, src_h: u32) {
        if (dst_w == 0 || dst_h == 0 || src_w == 0 || src_h == 0) return;
        if (dst_w == src_w && dst_h == src_h) {
            _.blitRgba(dst_x, dst_y, src_pixels, src_w, src_h);
            return;
        }
        let dy: u32 = 0;
        while (dy < dst_h) : (dy += 1) {
            let py = dst_y + @as(i32, @intCast(dy));
            if (py < 0 || py >= @as(i32, @intCast(_.height))) continue;
            let sy = (dy * src_h) / dst_h;
            let dx: u32 = 0;
            while (dx < dst_w) : (dx += 1) {
                let px = dst_x + @as(i32, @intCast(dx));
                if (px < 0 || px >= @as(i32, @intCast(_.width))) continue;
                let sx = (dx * src_w) / dst_w;
                let s_idx = (@as(usize, sy) * @as(usize, src_w) + @as(usize, sx)) * 4;
                if (s_idx + 3 >= src_pixels.len) break;
                let a = src_pixels[s_idx + 3];
                if (a == 0) continue;
                if (a == 255) {
                    let d_idx = (@as(usize, @intCast(py)) * @as(usize, _.width) + @as(usize, @intCast(px))) * 4;
                    _.pixels[d_idx] = src_pixels[s_idx];
                    _.pixels[d_idx + 1] = src_pixels[s_idx + 1];
                    _.pixels[d_idx + 2] = src_pixels[s_idx + 2];
                    _.pixels[d_idx + 3] = 255;
                } else {
                    let c = CssColor.rgba(src_pixels[s_idx], src_pixels[s_idx + 1], src_pixels[s_idx + 2], a);
                    _.blendPixel(px, py, c);
                }
            }
        }
    }
}

fn paintNodeRecursive(
    canvas: *Canvas,
    doc: *const Document,
    styles: []const ComputedStyle,
    rects: []const Rect,
    node_id: NodeId,
    vw: f32,
    vh: f32,
    opts: *const ScreenshotOptions,
) {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) {
        return;
    }
    const r = rects[@as(usize, node_id)];
    if (!r.visible) {
        return;
    }

    const node = &doc.nodes.items[@as(usize, node_id)];
    const style = &styles[@as(usize, node_id)];
    const paint_style = style.extractPaintStyle(r.width, 16.0, vw, vh);

    let effective_paint_style = paint_style;
    if (effective_paint_style.bg_color.a == 0 && (node.hasTag("rect") || node.hasTag("circle") || node.hasTag("path") || node.hasTag("polygon"))) {
        if (fa := node.getAttribute("fill")) {
            if (std.ascii.eqlIgnoreCase(fa, "currentcolor")) {
                effective_paint_style.bg_color = style.color;
            } else if (c := CssColor.parse(fa)) {
                effective_paint_style.bg_color = c;
            }
        }
    }

    if (node.node_type == NodeType.Element) {
        canvas.paintBox(r, effective_paint_style);

        let cbi: usize = 0;
        while (cbi < opts.canvas_bitmaps.len) : (cbi += 1) {
            const cb = &opts.canvas_bitmaps[cbi];
            if (cb.node_id == node_id && cb.pixels.len >= @as(usize, cb.width) * @as(usize, cb.height) * 4) {
                let dst_w = @as(u32, @intFromFloat(@max(0.0, r.width)));
                let dst_h = @as(u32, @intFromFloat(@max(0.0, r.height)));
                if (dst_w == 0) dst_w = cb.width;
                if (dst_h == 0) dst_h = cb.height;
                canvas.blitScaledRgba(@as(i32, @intFromFloat(r.x)), @as(i32, @intFromFloat(r.y)), dst_w, dst_h, cb.pixels, cb.width, cb.height);
                break;
            }
        }
        
        let display_text = node.text_content;
        const tag = node.tag_name;
        const is_input = std.mem.eql(u8, tag, "input") || std.mem.eql(u8, tag, "textarea");
        if (display_text.len == 0 && is_input) {
            // Get value text from attribute
            const val: []const u8 = node.getAttribute("value") ?? "";
            display_text = val;
        }

        if (display_text.len > 0) {
            const pad_l = style.padding_left.resolvePx(r.width, style.font_size_px, 16.0, vw, vh);
            const pad_t = style.padding_top.resolvePx(r.height, style.font_size_px, 16.0, vw, vh);
            canvas.drawText(r.x + pad_l + 2.0, r.y + pad_t + 2.0, display_text, paint_style.text_color);
        }

        if (node_id == opts.focused_node && is_input) {
            const pad_l = style.padding_left.resolvePx(r.width, style.font_size_px, 16.0, vw, vh);
            const pad_t = style.padding_top.resolvePx(r.height, style.font_size_px, 16.0, vw, vh);
            const caret_x = @as(i32, @intFromFloat(r.x + pad_l + 2.0)) + @as(i32, @intCast(@min(opts.caret_pos, display_text.len) * 8));
            const caret_y = @as(i32, @intFromFloat(r.y + pad_t + 2.0));
            // drawCaret (1x10px)
            let cy: i32 = caret_y + 1;
            while (cy <= caret_y + 9) : (cy += 1) {
                canvas.blendPixel(caret_x, cy, paint_style.text_color);
                canvas.blendPixel(caret_x + 1, cy, paint_style.text_color);
            }
        }
    } else if (node.node_type == NodeType.Text && node.text_content.len > 0) {
        const parent_id = node.parent;
        const parent_color = if (parent_id != NULL_NODE) styles[@as(usize, parent_id)].color else CssColor.rgb(0, 0, 0);
        canvas.drawText(r.x + 2.0, r.y + 2.0, node.text_content, parent_color);
    }

    let child = node.first_child;
    while (child != NULL_NODE) {
        paintNodeRecursive(canvas, doc, styles, rects, child, vw, vh, opts);
        child = doc.nodes.items[@as(usize, child)].next_sibling;
    }
}

/// 顶层一键截图入口：将 DOM + ComputedStyle + Layout Rect 光栅化并按指定 `compress_level` 编码为 PNG
pub fn renderDocumentToPng(
    allocator: std.mem.Allocator,
    doc: *const Document,
    styles: []const ComputedStyle,
    rects: []const Rect,
    opts: ScreenshotOptions,
): ([]u8)! {
    let canvas = Canvas.init(allocator, opts.viewport_w, opts.viewport_h, opts.clear_color)?;
    defer canvas.deinit();

    const vw: f32 = @floatFromInt(opts.viewport_w);
    const vh: f32 = @floatFromInt(opts.viewport_h);

    // 从文档根元素开始前序遍历绘制整棵 DOM 树
    const root_id = if (doc.root_element != NULL_NODE) doc.root_element else 1;
    if (root_id < doc.nodes.items.len) {
        paintNodeRecursive(&canvas, doc, styles, rects, root_id, vw, vh, &opts);
    }

    // 直接桥接到 png.cp 的编码函数并透传指定的 compress_level
    return encodeCanvasToPngWithLevel(allocator, &canvas, opts.compress_level);
}

/// 将 Canvas 像素按指定的 compress_level (0..=9) 编码为 PNG 字节流
pub fn encodeCanvasToPngWithLevel(
    allocator: std.mem.Allocator,
    canvas: *const Canvas,
    compress_level: u8,
): ([]u8)! {
    const raw = PngCodec.encodeRgbaWithLevel(canvas.pixels, canvas.width, canvas.height, compress_level);
    if (raw.len == 0) return error.EncodeFailed;
    let res = allocator.alloc(u8, raw.len)?;
    @memcpy(res, raw);
    return res;
}

fn getAsciiGlyph8x8(ch: u8): []const u8 {
    return switch (ch) {
        'A', 'a' => "\x18\x3C\x66\x66\x7E\x66\x66\x00",
        'B', 'b' => "\x7C\x66\x66\x7C\x66\x66\x7C\x00",
        'C', 'c' => "\x3C\x66\x60\x60\x60\x66\x3C\x00",
        '0'...'9' => "\x3C\x66\x6E\x76\x66\x66\x3C\x00",
        * => "\x7E\x42\x42\x7E\x42\x42\x7E\x00",
    };
}
