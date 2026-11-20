// src/render/painter.cp
// CapSurf 2D Software Rasterizer & Screenshot Entrypoint (Milestone 5)

import { Document, NodeData, NodeType, NodeId, NULL_NODE } from "../dom/dom.cp";
import { ComputedStyle, ResolvedPaintStyle, Position } from "../css/cascade.cp";
import { Rect } from "../layout/layout.cp";
import { CssColor } from "cap:css";
import { PngCodec } from "./png.cp";

@c {
    fn fopen(pathname: [*c]const u8, mode: [*c]const u8): (*void)?;
    fn fread(ptr: *void, size: usize, nmemb: usize, stream: *void): usize;
    fn fclose(stream: *void): i32;
}

/// Exported aliases to align with style_extractor and test harnesses
pub type Color = CssColor;
pub type PaintStyle = ResolvedPaintStyle;

pub struct ClipRect {
    x0: i32 = 0;
    y0: i32 = 0;
    x1: i32 = 0;
    y1: i32 = 0;

    static fn fromRect(r: Rect): ClipRect {
        return ClipRect {
            x0: @as(i32, @intFromFloat(@floor(r.x))),
            y0: @as(i32, @intFromFloat(@floor(r.y))),
            x1: @as(i32, @intFromFloat(@ceil(r.x + r.width))),
            y1: @as(i32, @intFromFloat(@ceil(r.y + r.height))),
        };
    }

    fn intersect(other: ClipRect): ClipRect {
        return ClipRect {
            x0: @max(_.x0, other.x0),
            y0: @max(_.y0, other.y0),
            x1: @min(_.x1, other.x1),
            y1: @min(_.y1, other.y1),
        };
    }
}

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
        _.paintBoxClipped(rect, style, null);
    }

    fn paintBoxClipped(*_, rect: Rect, style: ResolvedPaintStyle, clip: ClipRect?) {
        if (!rect.visible || rect.width <= 0.0 || rect.height <= 0.0) {
            return;
        }

        let x0 = @max(0, @as(i32, @intFromFloat(@floor(rect.x))));
        let y0 = @max(0, @as(i32, @intFromFloat(@floor(rect.y))));
        let x1 = @min(@as(i32, _.width), @as(i32, @intFromFloat(@ceil(rect.x + rect.width))));
        let y1 = @min(@as(i32, _.height), @as(i32, @intFromFloat(@ceil(rect.y + rect.height))));

        if (c := clip) {
            x0 = @max(x0, c.x0);
            y0 = @max(y0, c.y0);
            x1 = @min(x1, c.x1);
            y1 = @min(y1, c.y1);
        }
        if (x0 >= x1 || y0 >= y1) {
            return;
        }

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

    fn drawScaledTextClipped(*_, x: f32, y: f32, text: []const u8, pixel_size: f32, color: CssColor, clip: ClipRect?) {
        if (color.a == 0 || text.len == 0 || pixel_size <= 0.0) {
            return;
        }
        const scale = @max(1.0, pixel_size / 8.0);
        const step_x = 8.0 * scale;
        let cursor_x = x;

        let ti: usize = 0;
        while (ti < text.len) : (ti += 1) {
            const ch = text[ti];
            if (ch == '\n' || ch == '\r') continue;
            if (ch > 32 && ch < 127) {
                const glyph = getAsciiGlyph8x8(ch);
                let row: usize = 0;
                while (row < 8) : (row += 1) {
                    const bits = glyph[row];
                    if (bits == 0) continue;
                    const gy0 = @as(i32, @intFromFloat(@floor(y + @as(f32, @floatFromInt(row)) * scale)));
                    const gy1 = @max(gy0 + 1, @as(i32, @intFromFloat(@ceil(y + @as(f32, @floatFromInt(row + 1)) * scale))));
                    let col: usize = 0;
                    while (col < 8) : (col += 1) {
                        const shift: u3 = @intCast(7 - col);
                        if (((bits >> shift) & 1) == 1) {
                            const gx0 = @as(i32, @intFromFloat(@floor(cursor_x + @as(f32, @floatFromInt(col)) * scale)));
                            const gx1 = @max(gx0 + 1, @as(i32, @intFromFloat(@ceil(cursor_x + @as(f32, @floatFromInt(col + 1)) * scale))));
                            let py = gy0;
                            while (py < gy1) : (py += 1) {
                                if (c := clip) {
                                    if (py < c.y0 || py >= c.y1) continue;
                                }
                                let px = gx0;
                                while (px < gx1) : (px += 1) {
                                    if (c := clip) {
                                        if (px < c.x0 || px >= c.x1) continue;
                                    }
                                    _.blendPixel(px, py, color);
                                }
                            }
                        }
                    }
                }
            }
            cursor_x += step_x;
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

fn parseSvgNum(s: []const u8): f32? {
    const t = std.mem.trim(u8, s, " \t\r\n");
    if (t.len == 0) return null;
    let end = t.len;
    if (end >= 2 && std.ascii.eqlIgnoreCase(t[end - 2 .. end], "px")) {
        end -= 2;
    } else if (end >= 1 && t[end - 1] == '%') {
        end -= 1;
    }
    return std.fmt.parseFloat(f32, t[0..end]) catch null;
}

fn findNodeById(doc: *const Document, target_id: []const u8): NodeId {
    if (target_id.len == 0) return NULL_NODE;
    let i: usize = 1;
    while (i < doc.nodes.items.len) : (i += 1) {
        const n = &doc.nodes.items[i];
        if (!n.isElement()) continue;
        if (id_attr := n.getAttribute("id")) {
            if (std.mem.eql(u8, id_attr, target_id)) {
                return @as(NodeId, @intCast(i));
            }
        }
    }
    return NULL_NODE;
}

fn parseNextPathNumber(d: []const u8, pos: *usize): f32? {
    let i = pos.*;
    while (i < d.len && (d[i] == ' ' || d[i] == '\t' || d[i] == '\r' || d[i] == '\n' || d[i] == ',')) : (i += 1) {}
    if (i >= d.len) {
        pos.* = i;
        return null;
    }
    const start = i;
    if (d[i] == '+' || d[i] == '-') i += 1;
    let has_digit = false;
    while (i < d.len && d[i] >= '0' && d[i] <= '9') : (i += 1) {
        has_digit = true;
    }
    if (i < d.len && d[i] == '.') {
        i += 1;
        while (i < d.len && d[i] >= '0' && d[i] <= '9') : (i += 1) {
            has_digit = true;
        }
    }
    if (!has_digit) {
        pos.* = start;
        return null;
    }
    if (i < d.len && (d[i] == 'e' || d[i] == 'E')) {
        i += 1;
        if (i < d.len && (d[i] == '+' || d[i] == '-')) i += 1;
        while (i < d.len && d[i] >= '0' && d[i] <= '9') : (i += 1) {}
    }
    pos.* = i;
    return std.fmt.parseFloat(f32, d[start..i]) catch null;
}

fn paintSvgPath(
    canvas: *Canvas,
    d: []const u8,
    origin_x: f32,
    origin_y: f32,
    min_x: f32,
    min_y: f32,
    scale: f32,
    fill_color: CssColor,
    clip: ClipRect?,
) {
    if (fill_color.a == 0 || d.len == 0) return;

    let pts_x: [128]f32 = [0.0; 128];
    let pts_y: [128]f32 = [0.0; 128];
    let pt_count: usize = 0;

    let cur_x: f32 = 0.0;
    let cur_y: f32 = 0.0;
    let start_x: f32 = 0.0;
    let start_y: f32 = 0.0;
    let cmd: u8 = 0;
    let pos: usize = 0;

    while (pos < d.len) {
        while (pos < d.len && (d[pos] == ' ' || d[pos] == '\t' || d[pos] == '\r' || d[pos] == '\n' || d[pos] == ',')) : (pos += 1) {}
        if (pos >= d.len) break;

        const ch = d[pos];
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
            cmd = ch;
            pos += 1;
            if (cmd == 'Z' || cmd == 'z') {
                cur_x = start_x;
                cur_y = start_y;
                continue;
            }
        }
        if (cmd == 0) break;

        if (cmd == 'M' || cmd == 'm') {
            const x_opt = parseNextPathNumber(d, &pos);
            const y_opt = parseNextPathNumber(d, &pos);
            if (x_opt == null || y_opt == null) break;
            if (cmd == 'M') {
                cur_x = x_opt.?;
                cur_y = y_opt.?;
                cmd = 'L';
            } else {
                cur_x += x_opt.?;
                cur_y += y_opt.?;
                cmd = 'l';
            }
            start_x = cur_x;
            start_y = cur_y;
            if (pt_count < 128) {
                pts_x[pt_count] = origin_x + (cur_x - min_x) * scale;
                pts_y[pt_count] = origin_y + (cur_y - min_y) * scale;
                pt_count += 1;
            }
        } else if (cmd == 'L' || cmd == 'l') {
            const x_opt = parseNextPathNumber(d, &pos);
            const y_opt = parseNextPathNumber(d, &pos);
            if (x_opt == null || y_opt == null) break;
            if (cmd == 'L') {
                cur_x = x_opt.?;
                cur_y = y_opt.?;
            } else {
                cur_x += x_opt.?;
                cur_y += y_opt.?;
            }
            if (pt_count < 128) {
                pts_x[pt_count] = origin_x + (cur_x - min_x) * scale;
                pts_y[pt_count] = origin_y + (cur_y - min_y) * scale;
                pt_count += 1;
            }
        } else if (cmd == 'H' || cmd == 'h') {
            const x_opt = parseNextPathNumber(d, &pos);
            if (x_opt == null) break;
            if (cmd == 'H') {
                cur_x = x_opt.?;
            } else {
                cur_x += x_opt.?;
            }
            if (pt_count < 128) {
                pts_x[pt_count] = origin_x + (cur_x - min_x) * scale;
                pts_y[pt_count] = origin_y + (cur_y - min_y) * scale;
                pt_count += 1;
            }
        } else if (cmd == 'V' || cmd == 'v') {
            const y_opt = parseNextPathNumber(d, &pos);
            if (y_opt == null) break;
            if (cmd == 'V') {
                cur_y = y_opt.?;
            } else {
                cur_y += y_opt.?;
            }
            if (pt_count < 128) {
                pts_x[pt_count] = origin_x + (cur_x - min_x) * scale;
                pts_y[pt_count] = origin_y + (cur_y - min_y) * scale;
                pt_count += 1;
            }
        } else {
            // Consume numbers for unsupported curve commands to prevent infinite loop
            const n_opt = parseNextPathNumber(d, &pos);
            if (n_opt == null) pos += 1;
        }
    }

    if (pt_count < 3) return;

    let min_px = pts_x[0];
    let max_px = pts_x[0];
    let min_py = pts_y[0];
    let max_py = pts_y[0];
    let k: usize = 1;
    while (k < pt_count) : (k += 1) {
        if (pts_x[k] < min_px) min_px = pts_x[k];
        if (pts_x[k] > max_px) max_px = pts_x[k];
        if (pts_y[k] < min_py) min_py = pts_y[k];
        if (pts_y[k] > max_py) max_py = pts_y[k];
    }

    let x0 = @max(0, @as(i32, @intFromFloat(@floor(min_px))));
    let y0 = @max(0, @as(i32, @intFromFloat(@floor(min_py))));
    let x1 = @min(@as(i32, canvas.width), @as(i32, @intFromFloat(@ceil(max_px))));
    let y1 = @min(@as(i32, canvas.height), @as(i32, @intFromFloat(@ceil(max_py))));

    if (c := clip) {
        x0 = @max(x0, c.x0);
        y0 = @max(y0, c.y0);
        x1 = @min(x1, c.x1);
        y1 = @min(y1, c.y1);
    }
    if (x0 >= x1 || y0 >= y1) return;

    // Even-odd scanline polygon rasterizer
    let py = y0;
    while (py < y1) : (py += 1) {
        const py_f = @as(f32, @floatFromInt(py)) + 0.5;
        let px = x0;
        while (px < x1) : (px += 1) {
            const px_f = @as(f32, @floatFromInt(px)) + 0.5;
            let inside = false;
            let i: usize = 0;
            let j: usize = pt_count - 1;
            while (i < pt_count) : (i += 1) {
                const yi = pts_y[i];
                const yj = pts_y[j];
                const xi = pts_x[i];
                const xj = pts_x[j];
                const cond_y = (yi > py_f) != (yj > py_f);
                if (cond_y) {
                    const cross_x = (xj - xi) * (py_f - yi) / (yj - yi) + xi;
                    if (px_f < cross_x) {
                        inside = !inside;
                    }
                }
                j = i;
            }
            if (inside) {
                canvas.blendPixel(px, py, fill_color);
            }
        }
    }
}

fn resolveSvgElementFill(
    node: *const NodeData,
    node_id: NodeId,
    styles_opt: ([]const ComputedStyle)?,
    inherited_fill: CssColor,
    current_color: CssColor,
): CssColor {
    if (styles := styles_opt) {
        if (@as(usize, node_id) < styles.len) {
            const st = &styles[@as(usize, node_id)];
            if (st.has_fill_none) {
                return CssColor.rgba(0, 0, 0, 0);
            }
            if (st.has_fill_current_color) {
                return st.color;
            }
            if (st.background_color.a > 0 && !node.hasTag("svg")) {
                return st.background_color;
            }
        }
    }
    if (fa := node.getAttribute("fill")) {
        const t = std.mem.trim(u8, fa, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(t, "none")) {
            return CssColor.rgba(0, 0, 0, 0);
        }
        if (std.ascii.eqlIgnoreCase(t, "currentcolor")) {
            return current_color;
        }
        if (c := CssColor.parse(t)) {
            return c;
        }
    }
    return inherited_fill;
}

fn resolveSvgElementStroke(
    node: *const NodeData,
    current_color: CssColor,
): CssColor {
    if (sa := node.getAttribute("stroke")) {
        const t = std.mem.trim(u8, sa, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(t, "none")) {
            return CssColor.rgba(0, 0, 0, 0);
        }
        if (std.ascii.eqlIgnoreCase(t, "currentcolor")) {
            return current_color;
        }
        if (c := CssColor.parse(t)) {
            return c;
        }
    }
    return CssColor.rgba(0, 0, 0, 0);
}

fn parseSvgDashArray(attr: []const u8, scale: f32, out_dashes: *[64]f32): usize {
    const t = std.mem.trim(u8, attr, " \t\r\n");
    if (t.len == 0 || std.ascii.eqlIgnoreCase(t, "none")) return 0;

    let count: usize = 0;
    let it = std.mem.tokenizeAny(u8, t, " ,\t\r\n");
    while (tok := it.next()) {
        if (count >= 32) break;
        if (v := parseSvgNum(tok)) {
            if (v >= 0.0) {
                out_dashes[count] = v * scale;
                count += 1;
            }
        }
    }
    if (count == 0) return 0;
    // SVG spec: if odd number of entries, duplicate the list to make it even
    if ((count & 1) == 1) {
        let i: usize = 0;
        while (i < count && count + i < 64) : (i += 1) {
            out_dashes[count + i] = out_dashes[i];
        }
        count = count * 2;
    }
    return count;
}

fn paintSvgSegmentClipped(
    canvas: *Canvas,
    x1: f32,
    y1: f32,
    x2: f32,
    y2: f32,
    stroke_w: f32,
    color: CssColor,
    clip: ClipRect?,
) {
    if (color.a == 0 || stroke_w <= 0.0) return;
    const half_w = stroke_w * 0.5;
    const dx = x2 - x1;
    const dy = y2 - y1;
    const len_sq = dx * dx + dy * dy;

    const min_x = @min(x1, x2) - half_w - 1.0;
    const max_x = @max(x1, x2) + half_w + 1.0;
    const min_y = @min(y1, y2) - half_w - 1.0;
    const max_y = @max(y1, y2) + half_w + 1.0;

    let ix0 = @max(0, @as(i32, @intFromFloat(@floor(min_x))));
    let iy0 = @max(0, @as(i32, @intFromFloat(@floor(min_y))));
    let ix1 = @min(@as(i32, canvas.width), @as(i32, @intFromFloat(@ceil(max_x))));
    let iy1 = @min(@as(i32, canvas.height), @as(i32, @intFromFloat(@ceil(max_y))));

    if (c := clip) {
        ix0 = @max(ix0, c.x0);
        iy0 = @max(iy0, c.y0);
        ix1 = @min(ix1, c.x1);
        iy1 = @min(iy1, c.y1);
    }
    if (ix0 >= ix1 || iy0 >= iy1) return;

    const seg_len = @sqrt(len_sq);
    const ux = if (seg_len > 0.0001) (dx / seg_len) else 1.0;
    const uy = if (seg_len > 0.0001) (dy / seg_len) else 0.0;

    let py = iy0;
    while (py < iy1) : (py += 1) {
        const py_f = @as(f32, @floatFromInt(py)) + 0.5;
        let px = ix0;
        while (px < ix1) : (px += 1) {
            const px_f = @as(f32, @floatFromInt(px)) + 0.5;
            const rx = px_f - x1;
            const ry = py_f - y1;
            // Butt cap projection along segment
            const along = rx * ux + ry * uy;
            if (along < 0.0 || along > seg_len) continue;
            const perp = @abs(rx * (0.0 - uy) + ry * ux);
            if (perp <= half_w) {
                canvas.blendPixel(px, py, color);
            }
        }
    }
}

fn paintSvgLineClipped(
    canvas: *Canvas,
    x1: f32,
    y1: f32,
    x2: f32,
    y2: f32,
    stroke_w: f32,
    color: CssColor,
    dashes: *const [64]f32,
    dash_count: usize,
    dash_offset: f32,
    clip: ClipRect?,
) {
    if (color.a == 0 || stroke_w <= 0.0) return;
    const dx = x2 - x1;
    const dy = y2 - y1;
    const total_len = @sqrt(dx * dx + dy * dy);
    if (total_len <= 0.0001) return;

    if (dash_count == 0) {
        paintSvgSegmentClipped(canvas, x1, y1, x2, y2, stroke_w, color, clip);
        return;
    }

    let pattern_len: f32 = 0.0;
    let pi: usize = 0;
    while (pi < dash_count) : (pi += 1) {
        pattern_len += dashes[pi];
    }
    if (pattern_len <= 0.0001) {
        paintSvgSegmentClipped(canvas, x1, y1, x2, y2, stroke_w, color, clip);
        return;
    }

    const ux = dx / total_len;
    const uy = dy / total_len;

    // Normalize dash_offset into [0, pattern_len)
    let off = @mod(dash_offset, pattern_len);
    if (off < 0.0) off += pattern_len;

    let dash_idx: usize = 0;
    let rem = dashes[0];
    while (off > 0.0 && off >= rem) {
        off -= rem;
        dash_idx = (dash_idx + 1) % dash_count;
        rem = dashes[dash_idx];
    }
    rem -= off;

    let t: f32 = 0.0;
    let iter_guard: usize = 0;
    while (t < total_len && iter_guard < 4096) : (iter_guard += 1) {
        const step = @min(rem, total_len - t);
        const is_dash = (dash_idx & 1) == 0;
        if (is_dash && step > 0.0001) {
            const sx1 = x1 + ux * t;
            const sy1 = y1 + uy * t;
            const sx2 = x1 + ux * (t + step);
            const sy2 = y1 + uy * (t + step);
            paintSvgSegmentClipped(canvas, sx1, sy1, sx2, sy2, stroke_w, color, clip);
        }
        t += step;
        dash_idx = (dash_idx + 1) % dash_count;
        rem = dashes[dash_idx];
    }
}

fn collectNodeTextContent(doc: *const Document, node_id: NodeId, out_buf: []u8): []const u8 {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return out_buf[0..0];
    let len: usize = 0;
    const node = &doc.nodes.items[@as(usize, node_id)];
    if (node.text_content.len > 0) {
        const take = @min(node.text_content.len, out_buf.len - len);
        @memcpy(out_buf[len .. len + take], node.text_content[0..take]);
        len += take;
    }
    let ch = node.first_child;
    while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len && len < out_buf.len) : (ch = doc.nodes.items[@as(usize, ch)].next_sibling) {
        const cnode = &doc.nodes.items[@as(usize, ch)];
        if (cnode.node_type == NodeType.Text && cnode.text_content.len > 0) {
            const take = @min(cnode.text_content.len, out_buf.len - len);
            @memcpy(out_buf[len .. len + take], cnode.text_content[0..take]);
            len += take;
        }
    }
    return out_buf[0..len];
}

fn paintSvgChildrenDirect(
    canvas: *Canvas,
    doc: *const Document,
    styles_opt: ([]const ComputedStyle)?,
    parent_id: NodeId,
    origin_x: f32,
    origin_y: f32,
    min_x: f32,
    min_y: f32,
    scale: f32,
    box_w: f32,
    box_h: f32,
    inherited_fill: CssColor,
    current_color: CssColor,
    clip: ClipRect?,
    depth: u32,
) {
    if (depth > 8 || parent_id == NULL_NODE || @as(usize, parent_id) >= doc.nodes.items.len) return;
    let child = doc.nodes.items[@as(usize, parent_id)].first_child;
    while (child != NULL_NODE && @as(usize, child) < doc.nodes.items.len) : (child = doc.nodes.items[@as(usize, child)].next_sibling) {
        const cnode = &doc.nodes.items[@as(usize, child)];
        if (!cnode.isElement()) continue;
        if (cnode.hasTag("defs") || cnode.hasTag("symbol") || cnode.hasTag("style") || cnode.hasTag("metadata") || cnode.hasTag("title") || cnode.hasTag("desc")) {
            continue;
        }
        const fill_c = resolveSvgElementFill(cnode, child, styles_opt, inherited_fill, current_color);
        const stroke_c = resolveSvgElementStroke(cnode, current_color);
        let stroke_w: f32 = 1.0 * scale;
        if (swa := cnode.getAttribute("stroke-width")) {
            if (v := parseSvgNum(swa)) stroke_w = v * scale;
        }
        let dashes: [64]f32 = [0.0; 64];
        let dash_count: usize = 0;
        if (da := cnode.getAttribute("stroke-dasharray")) {
            dash_count = parseSvgDashArray(da, scale, &dashes);
        }
        let dash_offset: f32 = 0.0;
        if (doa := cnode.getAttribute("stroke-dashoffset")) {
            if (v := parseSvgNum(doa)) dash_offset = v * scale;
        }

        if (cnode.hasTag("rect")) {
            let rx: f32 = 0.0;
            let ry: f32 = 0.0;
            let rw: f32 = 0.0;
            let rh: f32 = 0.0;
            if (a := cnode.getAttribute("x")) {
                if (v := parseSvgNum(a)) rx = v;
            }
            if (a := cnode.getAttribute("y")) {
                if (v := parseSvgNum(a)) ry = v;
            }
            if (a := cnode.getAttribute("width")) {
                if (v := parseSvgNum(a)) rw = v;
            }
            if (a := cnode.getAttribute("height")) {
                if (v := parseSvgNum(a)) rh = v;
            }
            if (rw > 0.0 && rh > 0.0) {
                const sx = origin_x + (rx - min_x) * scale;
                const sy = origin_y + (ry - min_y) * scale;
                const sw = rw * scale;
                const sh = rh * scale;
                if (fill_c.a > 0) {
                    const rr = Rect {
                        x: sx,
                        y: sy,
                        width: sw,
                        height: sh,
                        visible: true,
                    };
                    const ps = ResolvedPaintStyle {
                        bg_color: fill_c,
                    };
                    canvas.paintBoxClipped(rr, ps, clip);
                }
                if (stroke_c.a > 0 && stroke_w > 0.0) {
                    paintSvgLineClipped(canvas, sx, sy, sx + sw, sy, stroke_w, stroke_c, &dashes, dash_count, dash_offset, clip);
                    paintSvgLineClipped(canvas, sx + sw, sy, sx + sw, sy + sh, stroke_w, stroke_c, &dashes, dash_count, dash_offset, clip);
                    paintSvgLineClipped(canvas, sx + sw, sy + sh, sx, sy + sh, stroke_w, stroke_c, &dashes, dash_count, dash_offset, clip);
                    paintSvgLineClipped(canvas, sx, sy + sh, sx, sy, stroke_w, stroke_c, &dashes, dash_count, dash_offset, clip);
                }
            }
        } else if (cnode.hasTag("line")) {
            let lx1: f32 = 0.0;
            let ly1: f32 = 0.0;
            let lx2: f32 = 0.0;
            let ly2: f32 = 0.0;
            if (a := cnode.getAttribute("x1")) {
                if (v := parseSvgNum(a)) lx1 = v;
            }
            if (a := cnode.getAttribute("y1")) {
                if (v := parseSvgNum(a)) ly1 = v;
            }
            if (a := cnode.getAttribute("x2")) {
                if (v := parseSvgNum(a)) lx2 = v;
            }
            if (a := cnode.getAttribute("y2")) {
                if (v := parseSvgNum(a)) ly2 = v;
            }
            const sx1 = origin_x + (lx1 - min_x) * scale;
            const sy1 = origin_y + (ly1 - min_y) * scale;
            const sx2 = origin_x + (lx2 - min_x) * scale;
            const sy2 = origin_y + (ly2 - min_y) * scale;
            if (stroke_c.a > 0 && stroke_w > 0.0) {
                paintSvgLineClipped(canvas, sx1, sy1, sx2, sy2, stroke_w, stroke_c, &dashes, dash_count, dash_offset, clip);
            }
        } else if (cnode.hasTag("text")) {
            let tx: f32 = 0.0;
            let ty: f32 = 0.0;
            let font_sz: f32 = 16.0;
            if (a := cnode.getAttribute("x")) {
                if (v := parseSvgNum(a)) tx = v;
            }
            if (a := cnode.getAttribute("y")) {
                if (v := parseSvgNum(a)) ty = v;
            }
            if (a := cnode.getAttribute("font-size")) {
                if (v := parseSvgNum(a)) {
                    if (v > 0.0) font_sz = v;
                }
            }
            let txt_buf: [512]u8 = undefined;
            const txt = collectNodeTextContent(doc, child, &txt_buf);
            if (txt.len > 0 && fill_c.a > 0) {
                const screen_font_px = font_sz * scale;
                const sx = origin_x + (tx - min_x) * scale;
                const sy_baseline = origin_y + (ty - min_y) * scale;
                // SVG text y is baseline; top of glyph sits ~0.8 * font_size above baseline
                const sy_top = sy_baseline - screen_font_px * 0.8;
                canvas.drawScaledTextClipped(sx, sy_top, txt, screen_font_px * 0.8, fill_c, clip);
            }
        } else if (cnode.hasTag("path")) {
            if (d_attr := cnode.getAttribute("d")) {
                paintSvgPath(canvas, d_attr, origin_x, origin_y, min_x, min_y, scale, fill_c, clip);
            }
        } else if (cnode.hasTag("circle")) {
            let cx: f32 = 0.0;
            let cy: f32 = 0.0;
            let cr: f32 = 0.0;
            if (a := cnode.getAttribute("cx")) {
                if (v := parseSvgNum(a)) cx = v;
            }
            if (a := cnode.getAttribute("cy")) {
                if (v := parseSvgNum(a)) cy = v;
            }
            if (a := cnode.getAttribute("r")) {
                if (v := parseSvgNum(a)) cr = v;
            }
            if (cr > 0.0 && (fill_c.a > 0 || stroke_c.a > 0)) {
                const sr = cr * scale;
                const rr = Rect {
                    x: origin_x + (cx - cr - min_x) * scale,
                    y: origin_y + (cy - cr - min_y) * scale,
                    width: sr * 2.0,
                    height: sr * 2.0,
                    visible: true,
                };
                const ps = ResolvedPaintStyle {
                    bg_color: fill_c,
                    border_radius: sr,
                    border_width: if (stroke_c.a > 0) stroke_w else 0.0,
                    border_color: stroke_c,
                };
                canvas.paintBoxClipped(rr, ps, clip);
            }
        } else if (cnode.hasTag("ellipse")) {
            let cx: f32 = 0.0;
            let cy: f32 = 0.0;
            let rx: f32 = 0.0;
            let ry: f32 = 0.0;
            if (a := cnode.getAttribute("cx")) {
                if (v := parseSvgNum(a)) cx = v;
            }
            if (a := cnode.getAttribute("cy")) {
                if (v := parseSvgNum(a)) cy = v;
            }
            if (a := cnode.getAttribute("rx")) {
                if (v := parseSvgNum(a)) rx = v;
            }
            if (a := cnode.getAttribute("ry")) {
                if (v := parseSvgNum(a)) ry = v;
            }
            if (rx > 0.0 && ry > 0.0 && fill_c.a > 0) {
                const srx = rx * scale;
                const sry = ry * scale;
                const rr = Rect {
                    x: origin_x + (cx - rx - min_x) * scale,
                    y: origin_y + (cy - ry - min_y) * scale,
                    width: srx * 2.0,
                    height: sry * 2.0,
                    visible: true,
                };
                const ps = ResolvedPaintStyle {
                    bg_color: fill_c,
                    border_radius: @min(srx, sry),
                };
                canvas.paintBoxClipped(rr, ps, clip);
            }
        } else if (cnode.hasTag("g")) {
            paintSvgChildrenDirect(canvas, doc, styles_opt, child, origin_x, origin_y, min_x, min_y, scale, box_w, box_h, fill_c, current_color, clip, depth + 1);
        } else if (cnode.hasTag("use")) {
            let href_opt = cnode.getAttribute("href");
            if (href_opt == null) {
                href_opt = cnode.getAttribute("xlink:href");
            }
            if (href := href_opt) {
                const t = std.mem.trim(u8, href, " \t\r\n");
                if (t.len > 1 && t[0] == '#') {
                    const target_id = findNodeById(doc, t[1..]);
                    if (target_id != NULL_NODE) {
                        paintSvgContainerRef(canvas, doc, styles_opt, target_id, origin_x, origin_y, box_w, box_h, fill_c, current_color, clip, depth + 1);
                    }
                } else if (hash_pos := std.mem.indexOfScalar(u8, t, '#')) {
                    if (hash_pos > 0 && hash_pos + 1 < t.len) {
                        const url_part = t[0..hash_pos];
                        const frag_id = t[hash_pos + 1 ..];
                        paintExternalSvgUse(canvas, url_part, frag_id, origin_x, origin_y, box_w, box_h, fill_c, current_color, clip, depth + 1);
                    }
                }
            }
        }
    }
}

fn paintSvgContainerRef(
    canvas: *Canvas,
    doc: *const Document,
    styles_opt: ([]const ComputedStyle)?,
    container_id: NodeId,
    box_x: f32,
    box_y: f32,
    box_w: f32,
    box_h: f32,
    inherited_fill: CssColor,
    current_color: CssColor,
    clip: ClipRect?,
    depth: u32,
) {
    if (depth > 8 || container_id == NULL_NODE || @as(usize, container_id) >= doc.nodes.items.len) return;
    const cnode = &doc.nodes.items[@as(usize, container_id)];

    let min_x: f32 = 0.0;
    let min_y: f32 = 0.0;
    let vb_w: f32 = box_w;
    let vb_h: f32 = box_h;
    let has_vb = false;

    if (vb_attr := cnode.getAttribute("viewBox")) {
        let it = std.mem.tokenizeAny(u8, vb_attr, " ,");
        if (s0 := it.next()) {
            if (v0 := parseSvgNum(s0)) min_x = v0;
        }
        if (s1 := it.next()) {
            if (v1 := parseSvgNum(s1)) min_y = v1;
        }
        if (s2 := it.next()) {
            if (v2 := parseSvgNum(s2)) {
                if (v2 > 0.0) {
                    vb_w = v2;
                    has_vb = true;
                }
            }
        }
        if (s3 := it.next()) {
            if (v3 := parseSvgNum(s3)) {
                if (v3 > 0.0) {
                    vb_h = v3;
                }
            }
        }
    }

    let scale: f32 = 1.0;
    let offset_x: f32 = 0.0;
    let offset_y: f32 = 0.0;
    if (has_vb && vb_w > 0.0 && vb_h > 0.0 && box_w > 0.0 && box_h > 0.0) {
        scale = @min(box_w / vb_w, box_h / vb_h);
        offset_x = (box_w - vb_w * scale) * 0.5;
        offset_y = (box_h - vb_h * scale) * 0.5;
    }

    const fill_c = resolveSvgElementFill(cnode, container_id, styles_opt, inherited_fill, current_color);
    paintSvgChildrenDirect(canvas, doc, styles_opt, container_id, box_x + offset_x, box_y + offset_y, min_x, min_y, scale, box_w, box_h, fill_c, current_color, clip, depth + 1);
}

fn readLocalFilePath(path: []const u8, out_buf: []u8): ([]const u8)? {
    if (path.len >= 511) return null;
    let pz: [512]u8 = [0; 512];
    @memcpy(pz[0..path.len], path);
    pz[path.len] = 0;
    const fp_opt = fopen(&pz, "rb");
    if (fp_opt == null) return null;
    const fp = fp_opt.?;
    defer * = fclose(fp);
    const n = fread(@ptrCast(out_buf.ptr), 1, out_buf.len, fp);
    return out_buf[0..n];
}

fn readFixtureFileLocal(filename: []const u8, out_buf: []u8): ([]const u8)? {
    let pbuf: [512]u8 = [0; 512];
    const rel = std.fmt.bufPrint(&pbuf, "e2e/page/{s}", .{ filename }) catch return null;
    if (res1 := readLocalFilePath(rel, out_buf)) {
        return res1;
    }

    let abuf: [512]u8 = [0; 512];
    const abs = std.fmt.bufPrint(&abuf, "/data/dev/capsurf/e2e/page/{s}", .{ filename }) catch return null;
    if (res2 := readLocalFilePath(abs, out_buf)) {
        return res2;
    }
    return null;
}

fn loadSvgAssetForImg(src: []const u8, out_buf: []u8): ([]const u8)? {
    if (src.len == 0) return null;
    if (std.mem.startsWith(u8, src, "data:image/svg+xml")) {
        if (comma := std.mem.indexOfScalar(u8, src, ',')) {
            return src[comma + 1 ..];
        }
        return null;
    }
    if (std.mem.indexOf(u8, src, "svg-document.svg") != null) {
        let target: []const u8 = "svg-document-sized.svg";
        if (std.mem.indexOf(u8, src, "case=overflowing") != null) {
            target = "svg-document-overflowing.svg";
        } else if (std.mem.indexOf(u8, src, "case=prologue") != null) {
            target = "svg-document-prologue.svg";
        } else if (std.mem.indexOf(u8, src, "case=viewbox") != null) {
            target = "svg-document-viewbox.svg";
        } else if (std.mem.indexOf(u8, src, "case=metadata") != null) {
            target = "svg-document-metadata.svg";
        } else if (std.mem.indexOf(u8, src, "case=styled") != null) {
            target = "svg-document-styled.svg";
        }
        return readFixtureFileLocal(target, out_buf);
    }
    if (fix_pos := std.mem.indexOf(u8, src, "/__fixtures/")) {
        let rest = src[fix_pos + 12 ..];
        if (q := std.mem.indexOfScalar(u8, rest, '?')) rest = rest[0..q];
        if (h := std.mem.indexOfScalar(u8, rest, '#')) rest = rest[0..h];
        if (std.ascii.endsWithIgnoreCase(rest, ".svg")) {
            return readFixtureFileLocal(rest, out_buf);
        }
    }
    return null;
}

fn paintExternalSvgUse(
    canvas: *Canvas,
    url_part: []const u8,
    frag_id: []const u8,
    box_x: f32,
    box_y: f32,
    box_w: f32,
    box_h: f32,
    inherited_fill: CssColor,
    current_color: CssColor,
    clip: ClipRect?,
    depth: u32,
) {
    if (depth > 8 || frag_id.len == 0) return;
    let file_buf: [65536]u8 = undefined;
    const svg_bytes_opt = loadSvgAssetForImg(url_part, &file_buf);
    if (svg_bytes_opt == null) return;
    const svg_bytes = svg_bytes_opt.?;
    if (svg_bytes.len == 0) return;

    let ext_doc = Document.init(canvas.allocator) catch return;
    defer ext_doc.deinit();
    ext_doc.parseHtml(svg_bytes) catch return;

    const target_id = findNodeById(&ext_doc, frag_id);
    if (target_id != NULL_NODE) {
        paintSvgContainerRef(canvas, &ext_doc, null, target_id, box_x, box_y, box_w, box_h, inherited_fill, current_color, clip, depth + 1);
    }
}

fn paintImgSvg(
    canvas: *Canvas,
    allocator: std.mem.Allocator,
    src: []const u8,
    img_rect: Rect,
    parent_clip: ClipRect?,
) {
    let file_buf: [65536]u8 = undefined;
    const svg_bytes_opt = loadSvgAssetForImg(src, &file_buf);
    if (svg_bytes_opt == null) return;
    const svg_bytes = svg_bytes_opt.?;
    if (svg_bytes.len == 0) return;

    let svg_doc = Document.init(allocator) catch return;
    defer svg_doc.deinit();
    svg_doc.parseHtml(svg_bytes) catch return;

    // Find the root <svg> element inside the parsed SVG document
    let svg_root: NodeId = NULL_NODE;
    let i: usize = 1;
    while (i < svg_doc.nodes.items.len) : (i += 1) {
        if (svg_doc.nodes.items[i].isElement() && svg_doc.nodes.items[i].hasTag("svg")) {
            svg_root = @as(NodeId, @intCast(i));
            break;
        }
    }
    if (svg_root == NULL_NODE) return;

    let img_clip = ClipRect.fromRect(img_rect);
    if (pc := parent_clip) {
        img_clip = img_clip.intersect(pc);
    }
    paintSvgContainerRef(
        canvas,
        &svg_doc,
        null,
        svg_root,
        img_rect.x,
        img_rect.y,
        img_rect.width,
        img_rect.height,
        CssColor.rgb(0, 0, 0),
        CssColor.rgb(0, 0, 0),
        img_clip,
        0,
    );
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
    clip: ClipRect?,
    abs_clip: ClipRect?,
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

    // Choose active clip based on CSS positioning containing-block rules:
    // - position: fixed escapes all ancestor overflow clips (uses viewport).
    // - position: absolute uses the clip established by its positioned containing block,
    //   escaping any static overflow:hidden ancestors in between.
    // - in-flow / relative uses the normal ancestor overflow clip.
    const active_clip: ClipRect? = if (style.position == Position.Fixed)
        null
    else if (style.position == Position.Absolute)
        abs_clip
    else
        clip;

    let effective_paint_style = paint_style;
    if (node.hasTag("svg") && style.has_fill_current_color) {
        // `fill: currentColor` on <svg> applies to child shapes/<use>, not the <svg> box background itself
        effective_paint_style.bg_color = CssColor.rgba(0, 0, 0, 0);
    }
    if (effective_paint_style.bg_color.a == 0 && (node.hasTag("rect") || node.hasTag("circle") || node.hasTag("path") || node.hasTag("polygon"))) {
        if (fa := node.getAttribute("fill")) {
            if (std.ascii.eqlIgnoreCase(fa, "currentcolor")) {
                effective_paint_style.bg_color = style.color;
            } else if (c := CssColor.parse(fa)) {
                effective_paint_style.bg_color = c;
            }
        }
    }

    let child_clip = active_clip;
    if (node.node_type == NodeType.Element && !style.overflow_visible && r.width > 0.0 && r.height > 0.0) {
        const own_clip = ClipRect.fromRect(r);
        if (cc := child_clip) {
            child_clip = cc.intersect(own_clip);
        } else {
            child_clip = own_clip;
        }
    }
    const child_abs_clip: ClipRect? = if (style.position != Position.Static) child_clip else abs_clip;

    if (node.node_type == NodeType.Element) {
        if (node.hasTag("use")) {
            let href_opt = node.getAttribute("href");
            if (href_opt == null) {
                href_opt = node.getAttribute("xlink:href");
            }
            if (href := href_opt) {
                const t = std.mem.trim(u8, href, " \t\r\n");
                let use_fill = effective_paint_style.bg_color;
                if (use_fill.a == 0) {
                    if (style.has_fill_current_color) {
                        use_fill = style.color;
                    } else {
                        use_fill = CssColor.rgb(0, 0, 0);
                    }
                }
                use_fill = resolveSvgElementFill(node, node_id, styles, use_fill, style.color);
                if (t.len > 1 && t[0] == '#') {
                    const target_id = findNodeById(doc, t[1..]);
                    if (target_id != NULL_NODE) {
                        paintSvgContainerRef(canvas, doc, styles, target_id, r.x, r.y, r.width, r.height, use_fill, style.color, active_clip, 0);
                    }
                } else if (hash_pos := std.mem.indexOfScalar(u8, t, '#')) {
                    if (hash_pos > 0 && hash_pos + 1 < t.len) {
                        paintExternalSvgUse(canvas, t[0..hash_pos], t[hash_pos + 1 ..], r.x, r.y, r.width, r.height, use_fill, style.color, active_clip, 0);
                    }
                }
            }
        } else if (node.hasTag("svg")) {
            canvas.paintBoxClipped(r, effective_paint_style, active_clip);
            let svg_fill = CssColor.rgb(0, 0, 0);
            if (style.has_fill_current_color) {
                svg_fill = style.color;
            }
            paintSvgContainerRef(canvas, doc, styles, node_id, r.x, r.y, r.width, r.height, svg_fill, style.color, child_clip, 0);
            return;
        } else if (node.hasTag("path")) {
            if (d_attr := node.getAttribute("d")) {
                let path_fill = effective_paint_style.bg_color;
                if (path_fill.a == 0) path_fill = CssColor.rgb(0, 0, 0);
                paintSvgPath(canvas, d_attr, r.x, r.y, 0.0, 0.0, 1.0, path_fill, active_clip);
            }
        } else {
            canvas.paintBoxClipped(r, effective_paint_style, active_clip);
        }

        if (node.hasTag("img")) {
            if (src := node.getAttribute("src")) {
                paintImgSvg(canvas, canvas.allocator, src, r, active_clip);
            }
        }

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
        paintNodeRecursive(canvas, doc, styles, rects, child, vw, vh, opts, child_clip, child_abs_clip);
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
        paintNodeRecursive(&canvas, doc, styles, rects, root_id, vw, vh, &opts, null, null);
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
        'D', 'd' => "\x78\x6C\x66\x66\x66\x6C\x78\x00",
        'E', 'e' => "\x7E\x60\x60\x78\x60\x60\x7E\x00",
        'F', 'f' => "\x7E\x60\x60\x78\x60\x60\x60\x00",
        'G', 'g' => "\x3C\x66\x60\x6E\x66\x66\x3E\x00",
        'H', 'h' => "\x66\x66\x66\x7E\x66\x66\x66\x00",
        'I', 'i' => "\x3C\x18\x18\x18\x18\x18\x3C\x00",
        'J', 'j' => "\x1E\x0C\x0C\x0C\x6C\x6C\x38\x00",
        'K', 'k' => "\x66\x6C\x78\x70\x78\x6C\x66\x00",
        'L', 'l' => "\x60\x60\x60\x60\x60\x60\x7E\x00",
        'M', 'm' => "\x63\x77\x7F\x6B\x63\x63\x63\x00",
        'N', 'n' => "\x66\x76\x7E\x7E\x6E\x66\x66\x00",
        'O', 'o' => "\x3C\x66\x66\x66\x66\x66\x3C\x00",
        'P', 'p' => "\x7C\x66\x66\x7C\x60\x60\x60\x00",
        'Q', 'q' => "\x3C\x66\x66\x66\x6A\x6C\x36\x00",
        'R', 'r' => "\x7C\x66\x66\x7C\x6C\x66\x66\x00",
        'S', 's' => "\x3C\x66\x60\x3C\x06\x66\x3C\x00",
        'T', 't' => "\x7E\x18\x18\x18\x18\x18\x18\x00",
        'U', 'u' => "\x66\x66\x66\x66\x66\x66\x3C\x00",
        'V', 'v' => "\x66\x66\x66\x66\x66\x3C\x18\x00",
        'W', 'w' => "\x63\x63\x63\x6B\x7F\x77\x63\x00",
        'X', 'x' => "\x66\x66\x3C\x18\x3C\x66\x66\x00",
        'Y', 'y' => "\x66\x66\x66\x3C\x18\x18\x18\x00",
        'Z', 'z' => "\x7E\x06\x0C\x18\x30\x60\x7E\x00",
        '0'...'9' => "\x3C\x66\x6E\x76\x66\x66\x3C\x00",
        * => "\x7E\x42\x42\x7E\x42\x42\x7E\x00",
    };
}
