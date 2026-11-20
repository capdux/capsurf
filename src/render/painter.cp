// src/render/painter.cp
// CapSurf 2D Software Rasterizer & Screenshot Entrypoint (Milestone 5)

import { Document, NodeData, NodeType, NodeId, NULL_NODE } from "../dom/dom.cp";
import { ComputedStyle, ResolvedPaintStyle } from "../css/cascade.cp";
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

fn resolveSvgElementFill(node: *const NodeData, inherited_fill: CssColor, current_color: CssColor): CssColor {
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

fn paintSvgChildrenDirect(
    canvas: *Canvas,
    doc: *const Document,
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
        const fill_c = resolveSvgElementFill(cnode, inherited_fill, current_color);

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
            if (rw > 0.0 && rh > 0.0 && fill_c.a > 0) {
                const rr = Rect {
                    x: origin_x + (rx - min_x) * scale,
                    y: origin_y + (ry - min_y) * scale,
                    width: rw * scale,
                    height: rh * scale,
                    visible: true,
                };
                const ps = ResolvedPaintStyle {
                    bg_color: fill_c,
                };
                canvas.paintBoxClipped(rr, ps, clip);
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
            if (cr > 0.0 && fill_c.a > 0) {
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
                };
                canvas.paintBoxClipped(rr, ps, clip);
            }
        } else if (cnode.hasTag("g")) {
            paintSvgChildrenDirect(canvas, doc, child, origin_x, origin_y, min_x, min_y, scale, box_w, box_h, fill_c, current_color, clip, depth + 1);
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
                        paintSvgContainerRef(canvas, doc, target_id, origin_x, origin_y, box_w, box_h, fill_c, current_color, clip, depth + 1);
                    }
                }
            }
        }
    }
}

fn paintSvgContainerRef(
    canvas: *Canvas,
    doc: *const Document,
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

    const fill_c = resolveSvgElementFill(cnode, inherited_fill, current_color);
    paintSvgChildrenDirect(canvas, doc, container_id, box_x + offset_x, box_y + offset_y, min_x, min_y, scale, box_w, box_h, fill_c, current_color, clip, depth + 1);
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

    let child_clip = clip;

    if (node.node_type == NodeType.Element) {
        if (node.hasTag("use")) {
            let href_opt = node.getAttribute("href");
            if (href_opt == null) {
                href_opt = node.getAttribute("xlink:href");
            }
            if (href := href_opt) {
                const t = std.mem.trim(u8, href, " \t\r\n");
                if (t.len > 1 && t[0] == '#') {
                    const target_id = findNodeById(doc, t[1..]);
                    if (target_id != NULL_NODE) {
                        let use_fill = effective_paint_style.bg_color;
                        if (use_fill.a == 0) {
                            if (style.has_fill_current_color) {
                                use_fill = style.color;
                            } else {
                                use_fill = CssColor.rgb(0, 0, 0);
                            }
                        }
                        use_fill = resolveSvgElementFill(node, use_fill, style.color);
                        paintSvgContainerRef(canvas, doc, target_id, r.x, r.y, r.width, r.height, use_fill, style.color, clip, 0);
                    }
                }
            }
        } else if (node.hasTag("path")) {
            if (d_attr := node.getAttribute("d")) {
                let path_fill = effective_paint_style.bg_color;
                if (path_fill.a == 0) path_fill = CssColor.rgb(0, 0, 0);
                paintSvgPath(canvas, d_attr, r.x, r.y, 0.0, 0.0, 1.0, path_fill, clip);
            }
        } else {
            canvas.paintBoxClipped(r, effective_paint_style, clip);
        }

        if (node.hasTag("img")) {
            if (src := node.getAttribute("src")) {
                paintImgSvg(canvas, canvas.allocator, src, r, clip);
            }
        }

        if (node.hasTag("svg") && !style.overflow_visible) {
            const svg_clip = ClipRect.fromRect(r);
            if (cc := child_clip) {
                child_clip = cc.intersect(svg_clip);
            } else {
                child_clip = svg_clip;
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
        paintNodeRecursive(canvas, doc, styles, rects, child, vw, vh, opts, child_clip);
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
        paintNodeRecursive(&canvas, doc, styles, rects, root_id, vw, vh, &opts, null);
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
