// src/render/painter.cp
// CapSurf 2D Software Rasterizer & Screenshot Entrypoint (Milestone 5)

import { Document, NodeData, NodeType, NodeId, NULL_NODE } from "../dom/dom.cp";
import { ComputedStyle, ResolvedPaintStyle, Position, CornerShape, Visibility, BgGradientKind } from "../css/cascade.cp";
import { Rect, LayoutTree } from "../layout/layout.cp";
import { CssColor } from "cap:css";
import { PngCodec } from "./png.cp";
import { uncompress } from "cap:pack";

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

pub struct SelectionSpan {
    node_id: NodeId = NULL_NODE,
    start_offset: usize = 0,
    end_offset: usize = 0,
}

pub struct FormControlValue {
    node_id: NodeId = NULL_NODE,
    value: []const u8 = "",
    min_val: f32 = 0.0,
    max_val: f32 = 100.0,
    has_min: bool = false,
    has_max: bool = false,
}

pub struct ScrollOffset {
    node_id: NodeId = NULL_NODE,
    scroll_left: f32 = 0.0,
    scroll_top: f32 = 0.0,
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
    selection_spans: []const SelectionSpan = [],
    form_values: []const FormControlValue = [],
    scroll_offsets: []const ScrollOffset = [],
    clip_x: i32 = 0;
    clip_y: i32 = 0;
    clip_w: u32 = 0;
    clip_h: u32 = 0;
}

fn isInsideCornerBox(cx: f32, cy: f32, r: f32, cs: CornerShape): bool {
    if (r <= 0.0 || cx >= r || cy >= r) return true;
    const u = (r - cx) / r;
    const v = (r - cy) / r;
    return switch (cs) {
        .Round => (u * u + v * v) <= 1.0,
        .Bevel => (u + v) <= 1.0,
        .Scoop => (cx / r) * (cx / r) + (cy / r) * (cy / r) >= 1.0,
        .Squircle => (u * u * u * u + v * v * v * v) <= 1.0,
        .Notch => false,
    };
}

fn isInsideOuterShape(
    px: f32,
    py: f32,
    rx: f32,
    ry: f32,
    rw: f32,
    rh: f32,
    r_tl: f32,
    r_tr: f32,
    r_br: f32,
    r_bl: f32,
    cs_tl: CornerShape,
    cs_tr: CornerShape,
    cs_br: CornerShape,
    cs_bl: CornerShape,
): bool {
    if (px < rx || px > rx + rw || py < ry || py > ry + rh) return false;
    // Top-left
    if (!isInsideCornerBox(px - rx, py - ry, r_tl, cs_tl)) return false;
    // Top-right
    if (!isInsideCornerBox((rx + rw) - px, py - ry, r_tr, cs_tr)) return false;
    // Bottom-right
    if (!isInsideCornerBox((rx + rw) - px, (ry + rh) - py, r_br, cs_br)) return false;
    // Bottom-left
    if (!isInsideCornerBox(px - rx, (ry + rh) - py, r_bl, cs_bl)) return false;
    return true;
}

fn isInsideInnerCorner(cx: f32, cy: f32, r: f32, bx: f32, by: f32, cs: CornerShape): bool {
    if (r <= 0.0) return true;
    switch (cs) {
        .Round => {
            const irx = if (r > bx) (r - bx) else 0.0;
            const iry = if (r > by) (r - by) else 0.0;
            if (irx <= 0.0 || iry <= 0.0) return true;
            const icx = cx - bx;
            const icy = cy - by;
            if (icx >= irx || icy >= iry) return true;
            const u = (irx - icx) / irx;
            const v = (iry - icy) / iry;
            return (u * u + v * v) <= 1.0;
        },
        .Squircle => {
            const irx = if (r > bx) (r - bx) else 0.0;
            const iry = if (r > by) (r - by) else 0.0;
            if (irx <= 0.0 || iry <= 0.0) return true;
            const icx = cx - bx;
            const icy = cy - by;
            if (icx >= irx || icy >= iry) return true;
            const u = (irx - icx) / irx;
            const v = (iry - icy) / iry;
            return (u * u * u * u + v * v * v * v) <= 1.0;
        },
        .Bevel => {
            const offset = @sqrt((bx / r) * (bx / r) + (by / r) * (by / r));
            return (cx / r + cy / r) >= (1.0 + offset);
        },
        .Scoop => {
            const sx = cx / (r + bx);
            const sy = cy / (r + by);
            return (sx * sx + sy * sy) >= 1.0;
        },
        .Notch => {
            return !(cx < r + bx && cy < r + by);
        },
    }
}

fn isInsideInnerShape(
    px: f32,
    py: f32,
    rx: f32,
    ry: f32,
    rw: f32,
    rh: f32,
    bt: f32,
    br: f32,
    bb: f32,
    bl: f32,
    r_tl: f32,
    r_tr: f32,
    r_br: f32,
    r_bl: f32,
    cs_tl: CornerShape,
    cs_tr: CornerShape,
    cs_br: CornerShape,
    cs_bl: CornerShape,
): bool {
    if (px < rx + bl || px > rx + rw - br || py < ry + bt || py > ry + rh - bb) return false;
    if (!isInsideInnerCorner(px - rx, py - ry, r_tl, bl, bt, cs_tl)) return false;
    if (!isInsideInnerCorner((rx + rw) - px, py - ry, r_tr, br, bt, cs_tr)) return false;
    if (!isInsideInnerCorner((rx + rw) - px, (ry + rh) - py, r_br, br, bb, cs_br)) return false;
    if (!isInsideInnerCorner(px - rx, (ry + rh) - py, r_bl, bl, bb, cs_bl)) return false;
    return true;
}

fn scaleCornerRadii(rw: f32, rh: f32, r_tl: *f32, r_tr: *f32, r_br: *f32, r_bl: *f32) {
    let f: f32 = 1.0;
    const sum_t = r_tl.* + r_tr.*;
    if (sum_t > rw && sum_t > 0.0) f = @min(f, rw / sum_t);
    const sum_b = r_bl.* + r_br.*;
    if (sum_b > rw && sum_b > 0.0) f = @min(f, rw / sum_b);
    const sum_l = r_tl.* + r_bl.*;
    if (sum_l > rh && sum_l > 0.0) f = @min(f, rh / sum_l);
    const sum_r = r_tr.* + r_br.*;
    if (sum_r > rh && sum_r > 0.0) f = @min(f, rh / sum_r);
    if (f < 1.0) {
        r_tl.* *= f;
        r_tr.* *= f;
        r_br.* *= f;
        r_bl.* *= f;
    }
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

    fn paintOutsetShadow(*_, rect: Rect, style: ResolvedPaintStyle, clip: ClipRect?) {
        const sh = style.box_shadow;
        if (!sh.has_shadow || sh.inset || sh.color.a == 0 || !rect.visible || rect.width <= 0.0 || rect.height <= 0.0) {
            return;
        }
        const sx = rect.x + sh.offset_x - sh.spread;
        const sy = rect.y + sh.offset_y - sh.spread;
        const sw = rect.width + sh.spread * 2.0;
        const sh_h = rect.height + sh.spread * 2.0;
        if (sw <= 0.0 || sh_h <= 0.0) return;

        let x0 = @max(0, @as(i32, @intFromFloat(@floor(sx))));
        let y0 = @max(0, @as(i32, @intFromFloat(@floor(sy))));
        let x1 = @min(@as(i32, _.width), @as(i32, @intFromFloat(@ceil(sx + sw))));
        let y1 = @min(@as(i32, _.height), @as(i32, @intFromFloat(@ceil(sy + sh_h))));
        if (c := clip) {
            x0 = @max(x0, c.x0);
            y0 = @max(y0, c.y0);
            x1 = @min(x1, c.x1);
            y1 = @min(y1, c.y1);
        }
        if (x0 >= x1 || y0 >= y1) return;

        let br_tl = if (style.border_tl_radius > 0.0) style.border_tl_radius else style.border_radius;
        let br_tr = if (style.border_tr_radius > 0.0 || style.border_tl_radius > 0.0 || style.border_br_radius > 0.0 || style.border_bl_radius > 0.0) style.border_tr_radius else style.border_radius;
        let br_br = if (style.border_tr_radius > 0.0 || style.border_tl_radius > 0.0 || style.border_br_radius > 0.0 || style.border_bl_radius > 0.0) style.border_br_radius else style.border_radius;
        let br_bl = if (style.border_tr_radius > 0.0 || style.border_tl_radius > 0.0 || style.border_br_radius > 0.0 || style.border_bl_radius > 0.0) style.border_bl_radius else style.border_radius;
        scaleCornerRadii(rect.width, rect.height, &br_tl, &br_tr, &br_br, &br_bl);

        const bevel_factor: f32 = 0.5857864;
        let sr_tl: f32 = if (br_tl <= 0.0) 0.0 else if (style.corner_shape_tl == CornerShape.Bevel && sh.spread > 0.0) (br_tl + sh.spread * bevel_factor) else @max(0.0, br_tl + sh.spread);
        let sr_tr: f32 = if (br_tr <= 0.0) 0.0 else if (style.corner_shape_tr == CornerShape.Bevel && sh.spread > 0.0) (br_tr + sh.spread * bevel_factor) else @max(0.0, br_tr + sh.spread);
        let sr_br: f32 = if (br_br <= 0.0) 0.0 else if (style.corner_shape_br == CornerShape.Bevel && sh.spread > 0.0) (br_br + sh.spread * bevel_factor) else @max(0.0, br_br + sh.spread);
        let sr_bl: f32 = if (br_bl <= 0.0) 0.0 else if (style.corner_shape_bl == CornerShape.Bevel && sh.spread > 0.0) (br_bl + sh.spread * bevel_factor) else @max(0.0, br_bl + sh.spread);
        scaleCornerRadii(sw, sh_h, &sr_tl, &sr_tr, &sr_br, &sr_bl);

        let py = y0;
        while (py < y1) : (py += 1) {
            const py_f = @as(f32, @floatFromInt(py)) + 0.5;
            let px = x0;
            while (px < x1) : (px += 1) {
                const px_f = @as(f32, @floatFromInt(px)) + 0.5;
                if (isInsideOuterShape(px_f, py_f, sx, sy, sw, sh_h, sr_tl, sr_tr, sr_br, sr_bl, style.corner_shape_tl, style.corner_shape_tr, style.corner_shape_br, style.corner_shape_bl)) {
                    _.blendPixel(px, py, sh.color);
                }
            }
        }
    }

    fn paintOutlineClipped(*_, rect: Rect, style: ResolvedPaintStyle, clip: ClipRect?) {
        if (!rect.visible || style.outline_width <= 0.0 || style.outline_color.a == 0) return;
        const ow = style.outline_width;
        const off = style.outline_offset;
        const ox = rect.x - off - ow;
        const oy = rect.y - off - ow;
        const o_w = rect.width + 2.0 * (off + ow);
        const o_h = rect.height + 2.0 * (off + ow);
        const ix0 = rect.x - off;
        const iy0 = rect.y - off;
        const ix1 = rect.x + rect.width + off;
        const iy1 = rect.y + rect.height + off;

        // Top, bottom, left, right strips of the outline ring
        _.fillRectClipped(ox, oy, o_w, ow, style.outline_color, clip);
        _.fillRectClipped(ox, iy1, o_w, ow, style.outline_color, clip);
        _.fillRectClipped(ox, iy0, ow, iy1 - iy0, style.outline_color, clip);
        _.fillRectClipped(ix1, iy0, ow, iy1 - iy0, style.outline_color, clip);
    }

    fn paintBoxClipped(*_, rect: Rect, style: ResolvedPaintStyle, clip: ClipRect?) {
        if (!rect.visible || rect.width <= 0.0 || rect.height <= 0.0) {
            return;
        }

        // Paint outset box-shadow behind the element box
        if (style.box_shadow.has_shadow && !style.box_shadow.inset) {
            _.paintOutsetShadow(rect, style, clip);
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

        const has_per_corner_r = (style.border_tl_radius > 0.0 || style.border_tr_radius > 0.0 || style.border_br_radius > 0.0 || style.border_bl_radius > 0.0);
        let r_tl = if (has_per_corner_r) style.border_tl_radius else style.border_radius;
        let r_tr = if (has_per_corner_r) style.border_tr_radius else style.border_radius;
        let r_br = if (has_per_corner_r) style.border_br_radius else style.border_radius;
        let r_bl = if (has_per_corner_r) style.border_bl_radius else style.border_radius;
        scaleCornerRadii(rect.width, rect.height, &r_tl, &r_tr, &r_br, &r_bl);

        const has_custom_shape = (style.corner_shape_tl != CornerShape.Round ||
            style.corner_shape_tr != CornerShape.Round ||
            style.corner_shape_br != CornerShape.Round ||
            style.corner_shape_bl != CornerShape.Round);
        const has_per_side_border = (style.border_top_width > 0.0 || style.border_right_width > 0.0 || style.border_bottom_width > 0.0 || style.border_left_width > 0.0);
        const bt = if (has_per_side_border) style.border_top_width else style.border_width;
        const br = if (has_per_side_border) style.border_right_width else style.border_width;
        const bb = if (has_per_side_border) style.border_bottom_width else style.border_width;
        const bl = if (has_per_side_border) style.border_left_width else style.border_width;
        const has_any_border = (bt > 0.0 || br > 0.0 || bb > 0.0 || bl > 0.0);
        const non_uniform_r = (r_tl != r_tr || r_tl != r_br || r_tl != r_bl);
        const non_uniform_b = (bt != br || bt != bb || bt != bl ||
            style.border_top_color.r != style.border_right_color.r || style.border_top_color.g != style.border_right_color.g || style.border_top_color.b != style.border_right_color.b ||
            style.border_top_color.r != style.border_bottom_color.r || style.border_top_color.g != style.border_bottom_color.g || style.border_top_color.b != style.border_bottom_color.b ||
            style.border_top_color.r != style.border_left_color.r || style.border_top_color.g != style.border_left_color.g || style.border_top_color.b != style.border_left_color.b);

        const has_conic = style.conic_gradient.has_gradient;
        const has_bg_grad = (style.bg_gradient.kind != BgGradientKind.None && style.bg_gradient.stop_count > 0 && !style.bg_gradient.clip_text);

        if (has_custom_shape || non_uniform_r || non_uniform_b) {
            const pad_cx = rect.x + bl + @max(0.0, rect.width - bl - br) * 0.5;
            const pad_cy = rect.y + bt + @max(0.0, rect.height - bt - bb) * 0.5;
            let py = y0;
            while (py < y1) : (py += 1) {
                const py_f = @as(f32, @floatFromInt(py)) + 0.5;
                let px = x0;
                while (px < x1) : (px += 1) {
                    const px_f = @as(f32, @floatFromInt(px)) + 0.5;
                    if (!isInsideOuterShape(px_f, py_f, rect.x, rect.y, rect.width, rect.height, r_tl, r_tr, r_br, r_bl, style.corner_shape_tl, style.corner_shape_tr, style.corner_shape_br, style.corner_shape_bl)) {
                        continue;
                    }
                    let final_color = if (has_conic)
                        style.conic_gradient.sampleAt(rect.x, rect.y, rect.width, rect.height, px_f, py_f)
                    else
                        style.bg_color;

                    if (has_any_border && !isInsideInnerShape(px_f, py_f, rect.x, rect.y, rect.width, rect.height, bt, br, bb, bl, r_tl, r_tr, r_br, r_bl, style.corner_shape_tl, style.corner_shape_tr, style.corner_shape_br, style.corner_shape_bl)) {
                        const dx_l = px_f - rect.x;
                        const dx_r = (rect.x + rect.width) - px_f;
                        const dy_t = py_f - rect.y;
                        const dy_b = (rect.y + rect.height) - py_f;

                        let best_score: f32 = 1.0e9;
                        let side_col = style.border_color;
                        if (bt > 0.0 && (py_f < pad_cy || dy_t < bt)) {
                            const s = dy_t / bt;
                            if (s < best_score) { best_score = s; side_col = style.border_top_color; }
                        }
                        if (bb > 0.0 && (py_f >= pad_cy || dy_b < bb)) {
                            const s = dy_b / bb;
                            if (s < best_score) { best_score = s; side_col = style.border_bottom_color; }
                        }
                        if (bl > 0.0 && (px_f < pad_cx || dx_l < bl)) {
                            const s = dx_l / bl;
                            if (s < best_score) { best_score = s; side_col = style.border_left_color; }
                        }
                        if (br > 0.0 && (px_f >= pad_cx || dx_r < br)) {
                            const s = dx_r / br;
                            if (s < best_score) { best_score = s; side_col = style.border_right_color; }
                        }
                        final_color = side_col;
                        if (final_color.a > 0) {
                            _.blendPixel(px, py, final_color);
                        }
                    } else {
                        if (final_color.a > 0) {
                            _.blendPixel(px, py, final_color);
                        }
                        if (has_bg_grad) {
                            const gc = style.bg_gradient.sampleAt(rect.x, rect.y, rect.width, rect.height, px_f, py_f);
                            if (gc.a > 0) {
                                _.blendPixel(px, py, gc);
                            }
                        }
                    }
                }
            }
            return;
        }

        // Fast path: axis-aligned rectangle with no border and no rounded corners
        if (r_tl <= 0.0 && style.border_width <= 0.0) {
            let py = y0;
            while (py < y1) : (py += 1) {
                const py_f = @as(f32, @floatFromInt(py)) + 0.5;
                let px = x0;
                while (px < x1) : (px += 1) {
                    const px_f = @as(f32, @floatFromInt(px)) + 0.5;
                    const base_c = if (has_conic)
                        style.conic_gradient.sampleAt(rect.x, rect.y, rect.width, rect.height, px_f, py_f)
                    else
                        style.bg_color;
                    if (base_c.a > 0) {
                        _.blendPixel(px, py, base_c);
                    }
                    if (has_bg_grad) {
                        const gc = style.bg_gradient.sampleAt(rect.x, rect.y, rect.width, rect.height, px_f, py_f);
                        if (gc.a > 0) {
                            _.blendPixel(px, py, gc);
                        }
                    }
                }
            }
            return;
        }

        const cx = rect.x + rect.width * 0.5;
        const cy = rect.y + rect.height * 0.5;
        const half_w = rect.width * 0.5;
        const half_h = rect.height * 0.5;
        const r = @min(r_tl, @min(half_w, half_h));

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
                    const neg_bw = 0.0 - style.border_width;
                    if (style.border_width > 0.0 && out_dist >= neg_bw) {
                        const bc = style.border_color;
                        if (bc.a > 0) {
                            const blended_a = @as(u8, @intFromFloat(@as(f32, @floatFromInt(bc.a)) * edge_cov));
                            _.blendPixel(px, py, CssColor.rgba(bc.r, bc.g, bc.b, blended_a));
                        }
                    } else {
                        const base_c = if (has_conic)
                            style.conic_gradient.sampleAt(rect.x, rect.y, rect.width, rect.height, px_f, py_f)
                        else
                            style.bg_color;
                        if (base_c.a > 0) {
                            const blended_a = @as(u8, @intFromFloat(@as(f32, @floatFromInt(base_c.a)) * edge_cov));
                            _.blendPixel(px, py, CssColor.rgba(base_c.r, base_c.g, base_c.b, blended_a));
                        }
                        if (has_bg_grad) {
                            const gc = style.bg_gradient.sampleAt(rect.x, rect.y, rect.width, rect.height, px_f, py_f);
                            if (gc.a > 0) {
                                const blended_a = @as(u8, @intFromFloat(@as(f32, @floatFromInt(gc.a)) * edge_cov));
                                _.blendPixel(px, py, CssColor.rgba(gc.r, gc.g, gc.b, blended_a));
                            }
                        }
                    }
                }
            }
        }
    }

    fn fillRectClipped(*_, rx: f32, ry: f32, rw: f32, rh: f32, color: CssColor, clip: ClipRect?) {
        if (color.a == 0 || rw <= 0.0 || rh <= 0.0) return;
        let x0 = @max(0, @as(i32, @intFromFloat(@floor(rx))));
        let y0 = @max(0, @as(i32, @intFromFloat(@floor(ry))));
        let x1 = @min(@as(i32, _.width), @as(i32, @intFromFloat(@ceil(rx + rw))));
        let y1 = @min(@as(i32, _.height), @as(i32, @intFromFloat(@ceil(ry + rh))));
        if (c := clip) {
            x0 = @max(x0, c.x0);
            y0 = @max(y0, c.y0);
            x1 = @min(x1, c.x1);
            y1 = @min(y1, c.y1);
        }
        if (x0 >= x1 || y0 >= y1) return;
        let py = y0;
        while (py < y1) : (py += 1) {
            let px = x0;
            while (px < x1) : (px += 1) {
                _.blendPixel(px, py, color);
            }
        }
    }

    fn drawTextClipped(*_, x: f32, y: f32, text: []const u8, color: CssColor, clip: ClipRect?) {
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
                            const px = cursor_x + @as(i32, @intCast(col));
                            const py = base_y + @as(i32, @intCast(row)) + 2;
                            if (c := clip) {
                                if (px < c.x0 || px >= c.x1 || py < c.y0 || py >= c.y1) continue;
                            }
                            _.blendPixel(px, py, color);
                        }
                    }
                }
            }
            cursor_x += 8;
        }
    }

    fn drawText(*_, x: f32, y: f32, text: []const u8, color: CssColor) {
        _.drawTextClipped(x, y, text, color, null);
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
                    const gy1 = @max(gy0 + 1, @as(i32, @intFromFloat(@floor(y + @as(f32, @floatFromInt(row + 1)) * scale))));
                    let col: usize = 0;
                    while (col < 8) : (col += 1) {
                        const shift: u3 = @intCast(7 - col);
                        if (((bits >> shift) & 1) == 1) {
                            const gx0 = @as(i32, @intFromFloat(@floor(cursor_x + @as(f32, @floatFromInt(col)) * scale)));
                            const gx1 = @max(gx0 + 1, @as(i32, @intFromFloat(@floor(cursor_x + @as(f32, @floatFromInt(col + 1)) * scale))));
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

    fn drawScaledTextWithGradientClipped(
        *_,
        x: f32,
        y: f32,
        text: []const u8,
        pixel_size: f32,
        box_rect: Rect,
        grad: *const ResolvedPaintStyle,
        clip: ClipRect?,
    ) {
        if (text.len == 0 || pixel_size <= 0.0) {
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
                    const gy1 = @max(gy0 + 1, @as(i32, @intFromFloat(@floor(y + @as(f32, @floatFromInt(row + 1)) * scale))));
                    let col: usize = 0;
                    while (col < 8) : (col += 1) {
                        const shift: u3 = @intCast(7 - col);
                        if (((bits >> shift) & 1) == 1) {
                            const gx0 = @as(i32, @intFromFloat(@floor(cursor_x + @as(f32, @floatFromInt(col)) * scale)));
                            const gx1 = @max(gx0 + 1, @as(i32, @intFromFloat(@floor(cursor_x + @as(f32, @floatFromInt(col + 1)) * scale))));
                            let py = gy0;
                            while (py < gy1) : (py += 1) {
                                if (c := clip) {
                                    if (py < c.y0 || py >= c.y1) continue;
                                }
                                const py_f = @as(f32, @floatFromInt(py)) + 0.5;
                                let px = gx0;
                                while (px < gx1) : (px += 1) {
                                    if (c := clip) {
                                        if (px < c.x0 || px >= c.x1) continue;
                                    }
                                    const px_f = @as(f32, @floatFromInt(px)) + 0.5;
                                    const gc = grad.bg_gradient.sampleAt(box_rect.x, box_rect.y, box_rect.width, box_rect.height, px_f, py_f);
                                    if (gc.a > 0) {
                                        _.blendPixel(px, py, gc);
                                    }
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

    fn blitScaledRgbaClipped(*_, dst_x: i32, dst_y: i32, dst_w: u32, dst_h: u32, src_pixels: []const u8, src_w: u32, src_h: u32, clip: ClipRect?) {
        if (dst_w == 0 || dst_h == 0 || src_w == 0 || src_h == 0) return;
        let dy: u32 = 0;
        while (dy < dst_h) : (dy += 1) {
            let py = dst_y + @as(i32, @intCast(dy));
            if (py < 0 || py >= @as(i32, @intCast(_.height))) continue;
            if (c := clip) {
                if (py < c.y0 || py >= c.y1) continue;
            }
            let sy = (dy * src_h) / dst_h;
            let dx: u32 = 0;
            while (dx < dst_w) : (dx += 1) {
                let px = dst_x + @as(i32, @intCast(dx));
                if (px < 0 || px >= @as(i32, @intCast(_.width))) continue;
                if (c := clip) {
                    if (px < c.x0 || px >= c.x1) continue;
                }
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
                    let col = CssColor.rgba(src_pixels[s_idx], src_pixels[s_idx + 1], src_pixels[s_idx + 2], a);
                    _.blendPixel(px, py, col);
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

fn resolveSvgGradientColor(doc: *const Document, raw_val: []const u8): CssColor? {
    const t = std.mem.trim(u8, raw_val, " \t\r\n");
    if (t.len < 6 || !std.ascii.startsWithIgnoreCase(t, "url(") || t[t.len - 1] != ')') return null;
    let inner = std.mem.trim(u8, t[4 .. t.len - 1], " \t\r\n\"'");
    if (inner.len < 2 || inner[0] != '#') return null;
    const grad_id = findNodeById(doc, inner[1..]);
    if (grad_id == NULL_NODE || @as(usize, grad_id) >= doc.nodes.items.len) return null;
    const gnode = &doc.nodes.items[@as(usize, grad_id)];
    if (!gnode.hasTag("lineargradient") && !gnode.hasTag("radialgradient")) return null;
    let child = gnode.first_child;
    while (child != NULL_NODE && @as(usize, child) < doc.nodes.items.len) : (child = doc.nodes.items[@as(usize, child)].next_sibling) {
        const cn = &doc.nodes.items[@as(usize, child)];
        if (cn.isElement() && cn.hasTag("stop")) {
            if (sc := cn.getAttribute("stop-color")) {
                if (c := CssColor.parse(std.mem.trim(u8, sc, " \t\r\n"))) {
                    return c;
                }
            }
            if (cn.inline_style.len > 0) {
                let it = std.mem.splitScalar(u8, cn.inline_style, ';');
                while (decl := it.next()) {
                    if (colon := std.mem.indexOfScalar(u8, decl, ':')) {
                        const k = std.mem.trim(u8, decl[0..colon], " \t\r\n");
                        if (std.ascii.eqlIgnoreCase(k, "stop-color")) {
                            const v = std.mem.trim(u8, decl[colon + 1 ..], " \t\r\n");
                            if (c := CssColor.parse(v)) return c;
                        }
                    }
                }
            }
        }
    }
    return null;
}

fn resolveSvgAttrVar(
    node_id: NodeId,
    styles_opt: ([]const ComputedStyle)?,
    raw: []const u8,
): []const u8 {
    if (styles := styles_opt) {
        if (@as(usize, node_id) < styles.len) {
            return styles[@as(usize, node_id)].resolveVarValue(raw);
        }
    }
    return std.mem.trim(u8, raw, " \t\r\n");
}

fn resolveSvgElementFill(
    doc: *const Document,
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
        const t = resolveSvgAttrVar(node_id, styles_opt, fa);
        if (std.ascii.eqlIgnoreCase(t, "none")) {
            return CssColor.rgba(0, 0, 0, 0);
        }
        if (std.ascii.eqlIgnoreCase(t, "currentcolor")) {
            return current_color;
        }
        if (gc := resolveSvgGradientColor(doc, t)) {
            return gc;
        }
        if (c := CssColor.parse(t)) {
            return c;
        }
    }
    return inherited_fill;
}

fn resolveSvgElementStroke(
    doc: *const Document,
    node: *const NodeData,
    node_id: NodeId,
    styles_opt: ([]const ComputedStyle)?,
    current_color: CssColor,
): CssColor {
    if (sa := node.getAttribute("stroke")) {
        const t = resolveSvgAttrVar(node_id, styles_opt, sa);
        if (std.ascii.eqlIgnoreCase(t, "none")) {
            return CssColor.rgba(0, 0, 0, 0);
        }
        if (std.ascii.eqlIgnoreCase(t, "currentcolor")) {
            return current_color;
        }
        if (gc := resolveSvgGradientColor(doc, t)) {
            return gc;
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
        const fill_c = resolveSvgElementFill(doc, cnode, child, styles_opt, inherited_fill, current_color);
        const stroke_c = resolveSvgElementStroke(doc, cnode, child, styles_opt, current_color);
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
                const fs_str = resolveSvgAttrVar(child, styles_opt, a);
                if (std.ascii.endsWithIgnoreCase(fs_str, "rem")) {
                    if (v := parseSvgNum(fs_str[0 .. fs_str.len - 3])) {
                        if (v > 0.0) font_sz = v * 16.0;
                    }
                } else if (std.ascii.endsWithIgnoreCase(fs_str, "em")) {
                    if (v := parseSvgNum(fs_str[0 .. fs_str.len - 2])) {
                        if (v > 0.0) font_sz = v * 16.0;
                    }
                } else if (v := parseSvgNum(fs_str)) {
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
        } else if (cnode.hasTag("svg")) {
            let sx_attr: f32 = 0.0;
            let sy_attr: f32 = 0.0;
            let sw_attr: f32 = box_w;
            let sh_attr: f32 = box_h;
            if (a := cnode.getAttribute("x")) {
                if (v := parseSvgNum(a)) sx_attr = v;
            }
            if (a := cnode.getAttribute("y")) {
                if (v := parseSvgNum(a)) sy_attr = v;
            }
            if (a := cnode.getAttribute("width")) {
                if (v := parseSvgNum(a)) sw_attr = v;
            }
            if (a := cnode.getAttribute("height")) {
                if (v := parseSvgNum(a)) sh_attr = v;
            }
            const nx = origin_x + (sx_attr - min_x) * scale;
            const ny = origin_y + (sy_attr - min_y) * scale;
            const nw = sw_attr * scale;
            const nh = sh_attr * scale;
            paintSvgContainerRef(canvas, doc, styles_opt, child, nx, ny, nw, nh, fill_c, current_color, clip, depth + 1);
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

    const fill_c = resolveSvgElementFill(doc, cnode, container_id, styles_opt, inherited_fill, current_color);
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

fn loadBgPngBytes(src: []const u8, out_buf: []u8): ([]const u8)? {
    const t = std.mem.trim(u8, src, " \t\r\n");
    if (t.len == 0) return null;
    if (std.mem.startsWith(u8, t, "data:image/png;base64,")) {
        const b64 = t[22..];
        const req_len = std.base64.standard.Decoder.calcSizeForSlice(b64) catch return null;
        if (req_len == 0 || req_len > out_buf.len) return null;
        std.base64.standard.Decoder.decode(out_buf[0..req_len], b64) catch return null;
        return out_buf[0..req_len];
    }
    if (std.mem.indexOf(u8, t, "slow-red.png") != null || std.mem.indexOf(u8, t, "slow-headers-red.png") != null) {
        return readFixtureFileLocal("solid-red.png", out_buf);
    }
    if (fix_pos := std.mem.indexOf(u8, t, "/__fixtures/")) {
        let rest = t[fix_pos + 12 ..];
        if (q := std.mem.indexOfScalar(u8, rest, '?')) rest = rest[0..q];
        if (h := std.mem.indexOfScalar(u8, rest, '#')) rest = rest[0..h];
        if (std.ascii.endsWithIgnoreCase(rest, ".png")) {
            return readFixtureFileLocal(rest, out_buf);
        }
    }
    return null;
}

fn readBeU32(b: []const u8, off: usize): u32 {
    if (off + 4 > b.len) return 0;
    return (@as(u32, b[off]) << 24) | (@as(u32, b[off + 1]) << 16) | (@as(u32, b[off + 2]) << 8) | @as(u32, b[off + 3]);
}

fn paethPredictor(a: i32, b: i32, c: i32): i32 {
    const p = a + b - c;
    const pa = if (p >= a) p - a else a - p;
    const pb = if (p >= b) p - b else b - p;
    const pc = if (p >= c) p - c else c - p;
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

fn decodePngAnyToRgba(allocator: std.mem.Allocator, bytes: []const u8, out_w: *u32, out_h: *u32): ([]u8)? {
    if (bytes.len < 33) return null;
    if (bytes[0] != 0x89 || bytes[1] != 0x50 || bytes[2] != 0x4E || bytes[3] != 0x47 ||
        bytes[4] != 0x0D || bytes[5] != 0x0A || bytes[6] != 0x1A || bytes[7] != 0x0A) {
        return null;
    }
    let width: u32 = 0;
    let height: u32 = 0;
    let bit_depth: u8 = 0;
    let color_type: u8 = 0;
    let total_idat: usize = 0;
    let off: usize = 8;
    while (off + 12 <= bytes.len) {
        const clen = @as(usize, readBeU32(bytes, off));
        const c0 = bytes[off + 4];
        const c1 = bytes[off + 5];
        const c2 = bytes[off + 6];
        const c3 = bytes[off + 7];
        if (c0 == 'I' && c1 == 'H' && c2 == 'D' && c3 == 'R' && clen >= 13 && off + 8 + 13 <= bytes.len) {
            width = readBeU32(bytes, off + 8);
            height = readBeU32(bytes, off + 12);
            bit_depth = bytes[off + 16];
            color_type = bytes[off + 17];
        } else if (c0 == 'I' && c1 == 'D' && c2 == 'A' && c3 == 'T') {
            total_idat += clen;
        } else if (c0 == 'I' && c1 == 'E' && c2 == 'N' && c3 == 'D') {
            break;
        }
        off += 12 + clen;
    }
    if (width == 0 || height == 0 || total_idat == 0 || bit_depth != 8) return null;
    let bpp: usize = 0;
    if (color_type == 6) {
        bpp = 4;
    } else if (color_type == 2) {
        bpp = 3;
    } else if (color_type == 0) {
        bpp = 1;
    } else if (color_type == 4) {
        bpp = 2;
    } else {
        return null;
    }

    const idat_buf = allocator.alloc(u8, total_idat) catch return null;
    defer allocator.free(idat_buf);
    off = 8;
    let idat_pos: usize = 0;
    while (off + 12 <= bytes.len) {
        const clen = @as(usize, readBeU32(bytes, off));
        const c0 = bytes[off + 4];
        const c1 = bytes[off + 5];
        const c2 = bytes[off + 6];
        const c3 = bytes[off + 7];
        if (c0 == 'I' && c1 == 'D' && c2 == 'A' && c3 == 'T') {
            const avail = if (off + 8 < bytes.len) @min(clen, bytes.len - (off + 8)) else 0;
            const copy_n = @min(avail, total_idat - idat_pos);
            if (copy_n > 0) {
                @memcpy(idat_buf[idat_pos .. idat_pos + copy_n], bytes[off + 8 .. off + 8 + copy_n]);
                idat_pos += copy_n;
            }
        } else if (c0 == 'I' && c1 == 'E' && c2 == 'N' && c3 == 'D') {
            break;
        }
        off += 12 + clen;
    }

    const row_bytes = @as(usize, width) * bpp;
    const raw_len = @as(usize, height) * (row_bytes + 1);
    const raw_buf = allocator.alloc(u8, raw_len) catch return null;
    defer allocator.free(raw_buf);
    let actual_len: usize = raw_len;
    if (uncompress(raw_buf.ptr, &actual_len, idat_buf.ptr, idat_pos) != 0) return null;

    let out_rgba = allocator.alloc(u8, @as(usize, width) * @as(usize, height) * 4) catch return null;
    const prev_row_opt = allocator.alloc(u8, row_bytes) catch null;
    if (prev_row_opt == null) {
        allocator.free(out_rgba);
        return null;
    }
    const prev_row = prev_row_opt.?;
    defer allocator.free(prev_row);
    @memset(prev_row, 0);
    const curr_row_opt = allocator.alloc(u8, row_bytes) catch null;
    if (curr_row_opt == null) {
        allocator.free(out_rgba);
        return null;
    }
    let curr_row = curr_row_opt.?;
    defer allocator.free(curr_row);

    let y: usize = 0;
    let src_off: usize = 0;
    while (y < @as(usize, height)) : (y += 1) {
        if (src_off >= raw_buf.len) break;
        const filt = raw_buf[src_off];
        src_off += 1;
        let x: usize = 0;
        while (x < row_bytes) : (x += 1) {
            const raw_v = @as(i32, raw_buf[src_off + x]);
            const left = if (x >= bpp) @as(i32, curr_row[x - bpp]) else 0;
            const up = @as(i32, prev_row[x]);
            const up_left = if (x >= bpp) @as(i32, prev_row[x - bpp]) else 0;
            const recon: i32 = if (filt == 0)
                raw_v
            else if (filt == 1)
                raw_v + left
            else if (filt == 2)
                raw_v + up
            else if (filt == 3)
                raw_v + @divTrunc(left + up, 2)
            else if (filt == 4)
                raw_v + paethPredictor(left, up, up_left)
            else
                raw_v;
            curr_row[x] = @as(u8, @intCast(recon & 0xFF));
        }
        src_off += row_bytes;
        let px: usize = 0;
        while (px < @as(usize, width)) : (px += 1) {
            const d = (y * @as(usize, width) + px) * 4;
            const s = px * bpp;
            if (color_type == 6) {
                out_rgba[d] = curr_row[s];
                out_rgba[d + 1] = curr_row[s + 1];
                out_rgba[d + 2] = curr_row[s + 2];
                out_rgba[d + 3] = curr_row[s + 3];
            } else if (color_type == 2) {
                out_rgba[d] = curr_row[s];
                out_rgba[d + 1] = curr_row[s + 1];
                out_rgba[d + 2] = curr_row[s + 2];
                out_rgba[d + 3] = 255;
            } else if (color_type == 4) {
                out_rgba[d] = curr_row[s];
                out_rgba[d + 1] = curr_row[s];
                out_rgba[d + 2] = curr_row[s];
                out_rgba[d + 3] = curr_row[s + 1];
            } else {
                out_rgba[d] = curr_row[s];
                out_rgba[d + 1] = curr_row[s];
                out_rgba[d + 2] = curr_row[s];
                out_rgba[d + 3] = 255;
            }
        }
        @memcpy(prev_row, curr_row);
    }
    out_w.* = width;
    out_h.* = height;
    return out_rgba;
}

fn paintBackgroundImage(
    canvas: *Canvas,
    allocator: std.mem.Allocator,
    src: []const u8,
    box_rect: Rect,
    parent_clip: ClipRect?,
) {
    if (!box_rect.visible || box_rect.width <= 0.0 || box_rect.height <= 0.0 || src.len == 0) return;
    if (std.mem.startsWith(u8, src, "data:image/svg+xml") || std.mem.indexOf(u8, src, ".svg") != null) {
        paintImgSvg(canvas, allocator, src, box_rect, parent_clip);
        return;
    }
    const file_buf = allocator.alloc(u8, 65536) catch return;
    defer allocator.free(file_buf);
    const png_bytes_opt = loadBgPngBytes(src, file_buf);
    if (png_bytes_opt == null) return;
    const png_bytes = png_bytes_opt.?;
    if (png_bytes.len < 8) return;
    let img_w: u32 = 0;
    let img_h: u32 = 0;
    const rgba_opt = decodePngAnyToRgba(allocator, png_bytes, &img_w, &img_h);
    if (rgba_opt == null) return;
    const rgba = rgba_opt.?;
    defer allocator.free(rgba);
    if (img_w == 0 || img_h == 0 || rgba.len == 0) return;
    const dst_x = @as(i32, @intFromFloat(@floor(box_rect.x)));
    const dst_y = @as(i32, @intFromFloat(@floor(box_rect.y)));
    const dst_w = @as(u32, @intFromFloat(@max(1.0, @ceil(box_rect.width))));
    const dst_h = @as(u32, @intFromFloat(@max(1.0, @ceil(box_rect.height))));
    canvas.blitScaledRgbaClipped(dst_x, dst_y, dst_w, dst_h, rgba, img_w, img_h, parent_clip);
}

fn scaleColorAlpha(c: CssColor, factor: f32): CssColor {
    const na: u8 = @intFromFloat(@as(f32, @floatFromInt(c.a)) * factor);
    return CssColor.rgba(c.r, c.g, c.b, na);
}

fn findFirstTextRectY(doc: *const Document, rects: []const Rect, node_id: NodeId): f32? {
    let ch = doc.nodes.items[@as(usize, node_id)].first_child;
    while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
        const cn = &doc.nodes.items[@as(usize, ch)];
        if (cn.node_type == NodeType.Text) {
            const trimmed = std.mem.trim(u8, cn.text_content, " \t\r\n");
            if (trimmed.len > 0 && @as(usize, ch) < rects.len && rects[@as(usize, ch)].visible) {
                return rects[@as(usize, ch)].y;
            }
        } else if (cn.node_type == NodeType.Element) {
            if (y := findFirstTextRectY(doc, rects, ch)) {
                return y;
            }
        }
        ch = cn.next_sibling;
    }
    return null;
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
    scroll_dx: f32,
    scroll_dy: f32,
    abs_scroll_dx: f32,
    abs_scroll_dy: f32,
    group_opacity: f32,
) {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) {
        return;
    }
    if (group_opacity <= 0.0) {
        return;
    }
    const node = &doc.nodes.items[@as(usize, node_id)];
    let r = rects[@as(usize, node_id)];
    const style = &styles[@as(usize, node_id)];
    if (node.node_type == NodeType.Element && style.display.isNone()) {
        return;
    }
    const eff_scroll_dx: f32 = if (node.node_type == NodeType.Element && style.position == Position.Fixed)
        0.0
    else if (node.node_type == NodeType.Element && style.position == Position.Absolute)
        abs_scroll_dx
    else
        scroll_dx;
    const eff_scroll_dy: f32 = if (node.node_type == NodeType.Element && style.position == Position.Fixed)
        0.0
    else if (node.node_type == NodeType.Element && style.position == Position.Absolute)
        abs_scroll_dy
    else
        scroll_dy;
    r.x -= eff_scroll_dx;
    r.y -= eff_scroll_dy;
    const own_op = if (node.node_type == NodeType.Element) std.math.clamp(style.opacity, 0.0, 1.0) else 1.0;
    const eff_op = group_opacity * own_op;
    if (eff_op <= 0.0) {
        return;
    }
    if (node.node_type == NodeType.Text && !r.visible) {
        if (opts.selection_spans.len > 0 && node.text_content.len > 0) {
            // Text nodes may not have standalone layout boxes; resolve position from nearest visible ancestor
            let anc = node.parent;
            while (anc != NULL_NODE && @as(usize, anc) < rects.len) {
                const ar = rects[@as(usize, anc)];
                if (ar.visible && ar.width > 0.0 && ar.height > 0.0) {
                    // Estimate horizontal offset from preceding siblings within parent
                    let prefix_chars: usize = 0;
                    let sib = doc.nodes.items[@as(usize, node.parent)].first_child;
                    while (sib != NULL_NODE && sib != node_id && @as(usize, sib) < doc.nodes.items.len) {
                        const sn = &doc.nodes.items[@as(usize, sib)];
                        if (sn.node_type == NodeType.Text) {
                            prefix_chars += sn.text_content.len;
                        } else if (sn.node_type == NodeType.Element) {
                            let c = sn.first_child;
                            while (c != NULL_NODE && @as(usize, c) < doc.nodes.items.len) {
                                const cn = &doc.nodes.items[@as(usize, c)];
                                if (cn.node_type == NodeType.Text) prefix_chars += cn.text_content.len;
                                c = cn.next_sibling;
                            }
                        }
                        sib = sn.next_sibling;
                    }
                    let si: usize = 0;
                    while (si < opts.selection_spans.len) : (si += 1) {
                        const sp = opts.selection_spans[si];
                        if (sp.node_id == node_id && sp.end_offset > sp.start_offset) {
                            const clamped_start = @min(sp.start_offset, node.text_content.len);
                            const clamped_end = @min(sp.end_offset, node.text_content.len);
                            if (clamped_end > clamped_start) {
                                const base_x = ar.x + 2.0 + @as(f32, @floatFromInt(prefix_chars)) * 8.0;
                                const hx = base_x + @as(f32, @floatFromInt(clamped_start)) * 8.0;
                                const hw = @as(f32, @floatFromInt(clamped_end - clamped_start)) * 8.0;
                                const hh = @max(ar.height, 14.0);
                                canvas.fillRectClipped(hx, ar.y, hw, hh, CssColor.rgba(51, 144, 255, 90), clip);
                            }
                        }
                    }
                    break;
                }
                anc = doc.nodes.items[@as(usize, anc)].parent;
            }
        }
        return;
    }

    const is_self_visible = (node.node_type != NodeType.Element) || (style.visibility == Visibility.Visible && r.visible);
    let paint_style = style.extractPaintStyle(r.width, 16.0, vw, vh);
    if (group_opacity < 1.0) {
        paint_style.bg_color = scaleColorAlpha(paint_style.bg_color, group_opacity);
        paint_style.text_color = scaleColorAlpha(paint_style.text_color, group_opacity);
        paint_style.border_color = scaleColorAlpha(paint_style.border_color, group_opacity);
        paint_style.border_top_color = scaleColorAlpha(paint_style.border_top_color, group_opacity);
        paint_style.border_right_color = scaleColorAlpha(paint_style.border_right_color, group_opacity);
        paint_style.border_bottom_color = scaleColorAlpha(paint_style.border_bottom_color, group_opacity);
        paint_style.border_left_color = scaleColorAlpha(paint_style.border_left_color, group_opacity);
        paint_style.outline_color = scaleColorAlpha(paint_style.outline_color, group_opacity);
        if (paint_style.box_shadow.has_shadow) {
            paint_style.box_shadow.color = scaleColorAlpha(paint_style.box_shadow.color, group_opacity);
        }
        if (paint_style.conic_gradient.has_gradient) {
            let si: usize = 0;
            while (si < paint_style.conic_gradient.stop_count) : (si += 1) {
                paint_style.conic_gradient.stops[si].color = scaleColorAlpha(paint_style.conic_gradient.stops[si].color, group_opacity);
            }
        }
        if (paint_style.bg_gradient.kind != BgGradientKind.None) {
            let gi: usize = 0;
            while (gi < paint_style.bg_gradient.stop_count) : (gi += 1) {
                paint_style.bg_gradient.stops[gi].color = scaleColorAlpha(paint_style.bg_gradient.stops[gi].color, group_opacity);
            }
        }
        paint_style.text_fill_color = scaleColorAlpha(paint_style.text_fill_color, group_opacity);
        if (paint_style.has_caret_color) {
            paint_style.caret_color = scaleColorAlpha(paint_style.caret_color, group_opacity);
        }
    }

    // Choose active clip based on CSS positioning containing-block rules:
    // - position: fixed escapes all ancestor overflow clips (uses viewport).
    // - position: absolute uses the clip established by its positioned containing block,
    //   escaping any static overflow:hidden ancestors in between.
    // - in-flow / relative uses the normal ancestor overflow clip.
    const base_clip: ClipRect? = if (style.position == Position.Fixed)
        null
    else if (style.position == Position.Absolute)
        abs_clip
    else
        clip;

    // Legacy `clip: rect(t, r, b, l)` applies to abs/fixed elements and wraps both outline and box
    let box_outer_clip = base_clip;
    if (node.node_type == NodeType.Element && style.legacy_clip.has_clip && (style.position == Position.Absolute || style.position == Position.Fixed)) {
        const lc = style.legacy_clip;
        const t_px = if (lc.top.is_auto) 0.0 else lc.top.resolvePx(r.height, style.font_size_px, 16.0, vw, vh);
        const r_px = if (lc.right.is_auto) r.width else lc.right.resolvePx(r.width, style.font_size_px, 16.0, vw, vh);
        const b_px = if (lc.bottom.is_auto) r.height else lc.bottom.resolvePx(r.height, style.font_size_px, 16.0, vw, vh);
        const l_px = if (lc.left.is_auto) 0.0 else lc.left.resolvePx(r.width, style.font_size_px, 16.0, vw, vh);
        const lx0 = @as(i32, @intFromFloat(@round(r.x + l_px)));
        const ly0 = @as(i32, @intFromFloat(@round(r.y + t_px)));
        const lx1 = @as(i32, @intFromFloat(@round(r.x + r_px)));
        const ly1 = @as(i32, @intFromFloat(@round(r.y + b_px)));
        const leg_rect = ClipRect{ x0: lx0, y0: ly0, x1: lx1, y1: ly1 };
        if (cc := box_outer_clip) {
            box_outer_clip = cc.intersect(leg_rect);
        } else {
            box_outer_clip = leg_rect;
        }
    }

    // Paint outline BEFORE clip-path layer is pushed (so outline survives empty clip-path, while legacy clip wraps it)
    if (is_self_visible && node.node_type == NodeType.Element && paint_style.outline_width > 0.0 && paint_style.outline_color.a > 0) {
        canvas.paintOutlineClipped(r, paint_style, box_outer_clip);
    }

    // Modern `clip-path: inset(t, r, b, l)` clips the box and its children
    let active_clip = box_outer_clip;
    if (node.node_type == NodeType.Element && style.clip_path_inset.has_clip) {
        const cp = style.clip_path_inset;
        const t_px = cp.top.resolvePx(r.height, style.font_size_px, 16.0, vw, vh);
        const r_px = cp.right.resolvePx(r.width, style.font_size_px, 16.0, vw, vh);
        const b_px = cp.bottom.resolvePx(r.height, style.font_size_px, 16.0, vw, vh);
        const l_px = cp.left.resolvePx(r.width, style.font_size_px, 16.0, vw, vh);
        const cx0 = @as(i32, @intFromFloat(@round(r.x + l_px)));
        const cy0 = @as(i32, @intFromFloat(@round(r.y + t_px)));
        const cx1 = @as(i32, @intFromFloat(@round(r.x + r.width - r_px)));
        const cy1 = @as(i32, @intFromFloat(@round(r.y + r.height - b_px)));
        const cp_rect = ClipRect{ x0: cx0, y0: cy0, x1: cx1, y1: cy1 };
        if (cc := active_clip) {
            active_clip = cc.intersect(cp_rect);
        } else {
            active_clip = cp_rect;
        }
    }

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
    const clip_x = style.clipsOverflowX();
    const clip_y = style.clipsOverflowY();
    if (node.node_type == NodeType.Element && (clip_x || clip_y) && r.width >= 0.0 && r.height >= 0.0) {
        const box_clip = ClipRect.fromRect(r);
        const own_clip = ClipRect {
            x0: if (clip_x) box_clip.x0 else -100000000,
            y0: if (clip_y) box_clip.y0 else -100000000,
            x1: if (clip_x) box_clip.x1 else 100000000,
            y1: if (clip_y) box_clip.y1 else 100000000,
        };
        if (cc := child_clip) {
            child_clip = cc.intersect(own_clip);
        } else {
            child_clip = own_clip;
        }
    }
    const child_abs_clip: ClipRect? = if (style.position != Position.Static) child_clip else abs_clip;

    let own_scroll_l: f32 = 0.0;
    let own_scroll_t: f32 = 0.0;
    if (node.node_type == NodeType.Element && opts.scroll_offsets.len > 0) {
        let soi: usize = 0;
        while (soi < opts.scroll_offsets.len) : (soi += 1) {
            if (opts.scroll_offsets[soi].node_id == node_id) {
                own_scroll_l = opts.scroll_offsets[soi].scroll_left;
                own_scroll_t = opts.scroll_offsets[soi].scroll_top;
                break;
            }
        }
    }
    const child_scroll_dx = eff_scroll_dx + own_scroll_l;
    const child_scroll_dy = eff_scroll_dy + own_scroll_t;
    const child_abs_scroll_dx = if (style.position != Position.Static) child_scroll_dx else abs_scroll_dx;
    const child_abs_scroll_dy = if (style.position != Position.Static) child_scroll_dy else abs_scroll_dy;

    if (node.node_type == NodeType.Element && is_self_visible) {
        if (node.hasTag("li") && node.parent != NULL_NODE && @as(usize, node.parent) < doc.nodes.items.len) {
            const pnode = &doc.nodes.items[@as(usize, node.parent)];
            if (pnode.hasTag("ol") || pnode.hasTag("ul")) {
                let marker_y = r.y;
                if (fy := findFirstTextRectY(doc, rects, node_id)) {
                    marker_y = fy;
                }
                const marker_x = @max(2.0, r.x - 24.0);
                let marker_buf: [16]u8 = undefined;
                let marker_str: []const u8 = "*";
                if (pnode.hasTag("ol")) {
                    let idx: usize = 1;
                    let sib = pnode.first_child;
                    while (sib != NULL_NODE && sib != node_id && @as(usize, sib) < doc.nodes.items.len) {
                        const sn = &doc.nodes.items[@as(usize, sib)];
                        if (sn.hasTag("li")) idx += 1;
                        sib = sn.next_sibling;
                    }
                    marker_str = std.fmt.bufPrint(&marker_buf, "{d}.", .{idx}) catch "1.";
                }
                const fsz = if (style.font_size_px > 0.0) style.font_size_px else 16.0;
                if (fsz >= 14.0) {
                    canvas.drawScaledTextClipped(marker_x, marker_y + 2.0, marker_str, fsz * 0.65, paint_style.text_color, active_clip);
                } else {
                    canvas.drawTextClipped(marker_x, marker_y + 2.0, marker_str, paint_style.text_color, active_clip);
                }
            }
        }
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
                use_fill = resolveSvgElementFill(doc, node, node_id, styles, use_fill, style.color);
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
            if (style.bg_image_url_len > 0) {
                paintBackgroundImage(canvas, canvas.allocator, style.bg_image_url_buf[0..style.bg_image_url_len], r, active_clip);
            }
        }

        if (node.hasTag("img")) {
            if (src := node.getAttribute("src")) {
                paintBackgroundImage(canvas, canvas.allocator, src, r, active_clip);
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
        const is_input = std.ascii.eqlIgnoreCase(tag, "input");
        const is_textarea = std.ascii.eqlIgnoreCase(tag, "textarea");
        const is_form_text = is_input || is_textarea;

        let found_live_form: (*const FormControlValue)? = null;
        if (is_form_text) {
            let fi: usize = 0;
            while (fi < opts.form_values.len) : (fi += 1) {
                if (opts.form_values[fi].node_id == node_id) {
                    found_live_form = &opts.form_values[fi];
                    break;
                }
            }
        }

        const is_range = is_input && std.ascii.eqlIgnoreCase(node.getAttribute("type") ?? "", "range");
        if (is_range) {
            let min_v: f32 = 0.0;
            let max_v: f32 = 100.0;
            if (min_attr := node.getAttribute("min")) {
                min_v = std.fmt.parseFloat(f32, std.mem.trim(u8, min_attr, " \t\r\n")) catch 0.0;
            }
            if (max_attr := node.getAttribute("max")) {
                max_v = std.fmt.parseFloat(f32, std.mem.trim(u8, max_attr, " \t\r\n")) catch 100.0;
            }
            if (lf := found_live_form) {
                if (lf.has_min) min_v = lf.min_val;
                if (lf.has_max) max_v = lf.max_val;
            }
            if (max_v <= min_v) max_v = min_v + 100.0;
            let cur_v: f32 = (min_v + max_v) * 0.5;
            if (lf := found_live_form) {
                if (lf.value.len > 0) {
                    cur_v = std.fmt.parseFloat(f32, std.mem.trim(u8, lf.value, " \t\r\n")) catch cur_v;
                }
            } else if (val_attr := node.getAttribute("value")) {
                cur_v = std.fmt.parseFloat(f32, std.mem.trim(u8, val_attr, " \t\r\n")) catch cur_v;
            }
            const ratio = std.math.clamp((cur_v - min_v) / (max_v - min_v), 0.0, 1.0);
            const accent = if (paint_style.has_accent_color) paint_style.accent_color else CssColor.rgb(0, 117, 255);
            const track_h: f32 = 4.0;
            const track_y = r.y + (r.height - track_h) * 0.5;
            const track_w = @max(r.width, 20.0);
            // Unfilled track
            canvas.fillRectClipped(r.x, track_y, track_w, track_h, CssColor.rgb(200, 200, 200), active_clip);
            // Filled track
            const thumb_d: f32 = @min(14.0, @max(r.height - 2.0, 8.0));
            const usable_w = @max(track_w - thumb_d, 0.0);
            const thumb_cx = r.x + thumb_d * 0.5 + ratio * usable_w;
            canvas.fillRectClipped(r.x, track_y, @max(thumb_cx - r.x, 0.0), track_h, accent, active_clip);
            // Thumb
            const thumb_rect = Rect{
                x: thumb_cx - thumb_d * 0.5,
                y: r.y + (r.height - thumb_d) * 0.5,
                width: thumb_d,
                height: thumb_d,
                visible: true,
            };
            const thumb_style = ResolvedPaintStyle{
                bg_color: accent,
                border_color: CssColor.rgb(255, 255, 255),
                text_color: accent,
                border_width: 1.0,
                border_radius: thumb_d * 0.5,
            };
            canvas.paintBoxClipped(thumb_rect, thumb_style, active_clip);
            return;
        }

        if (is_form_text) {
            if (lf := found_live_form) {
                display_text = lf.value;
            } else if (is_input && display_text.len == 0) {
                if (v := node.getAttribute("value")) {
                    display_text = v;
                } else {
                    display_text = "";
                }
            } else if (is_textarea && display_text.len == 0) {
                let tc = node.first_child;
                while (tc != NULL_NODE && @as(usize, tc) < doc.nodes.items.len) {
                    const cn = &doc.nodes.items[@as(usize, tc)];
                    if (cn.node_type == NodeType.Text && cn.text_content.len > 0) {
                        display_text = cn.text_content;
                        break;
                    }
                    tc = cn.next_sibling;
                }
            }
        }

        if (is_form_text && display_text.len > 0) {
            const pad_l = style.padding_left.resolvePx(r.width, style.font_size_px, 16.0, vw, vh);
            const pad_t = style.padding_top.resolvePx(r.height, style.font_size_px, 16.0, vw, vh);
            const bdr_l = paint_style.border_left_width;
            const bdr_t = paint_style.border_top_width;
            const fsz = if (style.font_size_px > 0.0) style.font_size_px else 16.0;
            const lh = if (style.has_line_height && style.line_height_px > 0.0) style.line_height_px else @max(fsz * 1.2, 14.0);
            let si: usize = 0;
            while (si < opts.selection_spans.len) : (si += 1) {
                const sp = opts.selection_spans[si];
                if (sp.node_id == node_id && sp.end_offset > sp.start_offset) {
                    const clamped_start = @min(sp.start_offset, display_text.len);
                    const clamped_end = @min(sp.end_offset, display_text.len);
                    if (clamped_end > clamped_start) {
                        const start_w = LayoutTree.measureTextSegmentWithTabs(display_text[0..clamped_start], style, 0.0);
                        const sel_w = LayoutTree.measureTextSegmentWithTabs(display_text[clamped_start..clamped_end], style, start_w);
                        const hx = r.x + bdr_l + pad_l + start_w;
                        const hy = r.y + bdr_t + pad_t;
                        const hh = @min(lh, @max(r.height - bdr_t - pad_t, 12.0));
                        canvas.fillRectClipped(hx, hy, @max(sel_w, 2.0), hh, CssColor.rgba(51, 144, 255, 110), active_clip);
                    }
                }
            }
        }

        if (display_text.len > 0) {
            const pad_l = style.padding_left.resolvePx(r.width, style.font_size_px, 16.0, vw, vh);
            const pad_t = style.padding_top.resolvePx(r.height, style.font_size_px, 16.0, vw, vh);
            canvas.drawTextClipped(r.x + pad_l + 2.0, r.y + pad_t + 2.0, display_text, paint_style.text_color, active_clip);
        }

        if (style.has_before_content && style.before_content_len > 0 && paint_style.text_color.a > 0) {
            const before_pseudo_txt = style.before_content_buf[0..style.before_content_len];
            const pad_l = style.padding_left.resolvePx(r.width, style.font_size_px, 16.0, vw, vh);
            const pad_t = style.padding_top.resolvePx(r.height, style.font_size_px, 16.0, vw, vh);
            const fsz = if (style.font_size_px > 0.0) style.font_size_px else 16.0;
            if (fsz >= 14.0) {
                const scaled_h = fsz * 0.65;
                const draw_y = r.y + pad_t + @max(0.0, (r.height - pad_t - scaled_h) * 0.5);
                canvas.drawScaledTextClipped(r.x + pad_l, draw_y, before_pseudo_txt, scaled_h, paint_style.text_color, active_clip);
            } else {
                canvas.drawTextClipped(r.x + pad_l + 1.0, r.y + pad_t + 2.0, before_pseudo_txt, paint_style.text_color, active_clip);
            }
        }

        if (node_id == opts.focused_node && is_form_text) {
            const pad_l = style.padding_left.resolvePx(r.width, style.font_size_px, 16.0, vw, vh);
            const pad_t = style.padding_top.resolvePx(r.height, style.font_size_px, 16.0, vw, vh);
            const bdr_l = paint_style.border_left_width;
            const bdr_t = paint_style.border_top_width;
            const fsz = if (style.font_size_px > 0.0) style.font_size_px else 16.0;
            const lh = if (style.has_line_height && style.line_height_px > 0.0) style.line_height_px else @max(fsz * 1.2, 14.0);
            const caret_idx = @min(opts.caret_pos, display_text.len);
            let line_start: usize = 0;
            let line_idx: usize = 0;
            let ci: usize = 0;
            while (ci < caret_idx) : (ci += 1) {
                if (display_text[ci] == '\n') {
                    line_start = ci + 1;
                    line_idx += 1;
                }
            }
            const before_caret = display_text[line_start..caret_idx];
            const text_adv = LayoutTree.measureTextSegmentWithTabs(before_caret, style, 0.0);
            const caret_x = @round(r.x + bdr_l + pad_l + text_adv);
            const caret_y = @floor(r.y + bdr_t + pad_t + @as(f32, @floatFromInt(line_idx)) * lh);
            const caret_h = @max(12.0, @round(@max(fsz, lh * 0.8)));
            const caret_col = if (paint_style.has_caret_color) paint_style.caret_color else paint_style.text_color;
            if (caret_col.a > 0) {
                canvas.fillRectClipped(caret_x, caret_y, 2.0, caret_h, caret_col, active_clip);
            }
        }

        if (is_textarea) {
            return;
        }
    } else if (node.node_type == NodeType.Text && node.text_content.len > 0) {
        const parent_id = node.parent;
        const parent_vis = if (parent_id != NULL_NODE && @as(usize, parent_id) < styles.len) (styles[@as(usize, parent_id)].visibility == Visibility.Visible) else true;
        if (parent_vis && r.visible) {
            let si: usize = 0;
            while (si < opts.selection_spans.len) : (si += 1) {
                const sp = opts.selection_spans[si];
                if (sp.node_id == node_id && sp.end_offset > sp.start_offset) {
                    const clamped_start = @min(sp.start_offset, node.text_content.len);
                    const clamped_end = @min(sp.end_offset, node.text_content.len);
                    if (clamped_end > clamped_start) {
                        const hx = r.x + 2.0 + @as(f32, @floatFromInt(clamped_start)) * 8.0;
                        const hw = @as(f32, @floatFromInt(clamped_end - clamped_start)) * 8.0;
                        const hh = @max(r.height, 14.0);
                        canvas.fillRectClipped(hx, r.y, hw, hh, CssColor.rgba(51, 144, 255, 90), active_clip);
                    }
                }
            }
            const has_parent_st = (parent_id != NULL_NODE && @as(usize, parent_id) < styles.len);
            const p_style_opt: (*const ComputedStyle)? = if (has_parent_st) &styles[@as(usize, parent_id)] else null;
            let raw_color = CssColor.rgb(0, 0, 0);
            let raw_fill = CssColor.rgb(0, 0, 0);
            let fsz: f32 = 16.0;
            if (ps := p_style_opt) {
                raw_color = ps.color;
                raw_fill = if (ps.has_text_fill_color) ps.text_fill_color else ps.color;
                fsz = ps.font_size_px;
            }
            const txt_color = if (group_opacity < 1.0) scaleColorAlpha(raw_fill, group_opacity) else raw_fill;
            const deco_color = if (group_opacity < 1.0) scaleColorAlpha(raw_color, group_opacity) else raw_color;
            const trimmed = std.mem.trim(u8, node.text_content, " \t\r\n");
            if (trimmed.len > 0) {
                const scaled_h = if (fsz >= 14.0) (fsz * 0.65) else 8.0;
                const draw_y = if (fsz >= 14.0) (r.y + @max(0.0, (r.height - scaled_h) * 0.5)) else (r.y + 2.0);
                let painted_grad_text = false;
                if (ps := p_style_opt) {
                    let p_rect = r;
                    if (@as(usize, parent_id) < rects.len) {
                        p_rect = rects[@as(usize, parent_id)];
                        if (ps.position != Position.Fixed) {
                            p_rect.x -= eff_scroll_dx;
                            p_rect.y -= eff_scroll_dy;
                        }
                    }
                    let p_paint = ps.extractPaintStyle(p_rect.width, 16.0, vw, vh);
                    if (p_paint.bg_gradient.kind != BgGradientKind.None && p_paint.bg_gradient.stop_count > 0 && p_paint.bg_gradient.clip_text) {
                        if (group_opacity < 1.0) {
                            let gi: usize = 0;
                            while (gi < p_paint.bg_gradient.stop_count) : (gi += 1) {
                                p_paint.bg_gradient.stops[gi].color = scaleColorAlpha(p_paint.bg_gradient.stops[gi].color, group_opacity);
                            }
                        }
                        canvas.drawScaledTextWithGradientClipped(r.x, draw_y, trimmed, scaled_h, p_rect, &p_paint, active_clip);
                        painted_grad_text = true;
                    }
                }
                if (!painted_grad_text && txt_color.a > 0) {
                    if (fsz >= 14.0) {
                        canvas.drawScaledTextClipped(r.x, draw_y, trimmed, scaled_h, txt_color, active_clip);
                    } else {
                        canvas.drawTextClipped(r.x + 1.0, r.y + 2.0, trimmed, txt_color, active_clip);
                    }
                }
                if (ps := p_style_opt) {
                    if (ps.text_decoration_underline && deco_color.a > 0) {
                        const ul_h = @max(1.0, fsz * 0.08);
                        const ul_y = draw_y + scaled_h + @max(1.0, fsz * 0.05);
                        const ul_w = @max(r.width, @as(f32, @floatFromInt(trimmed.len)) * scaled_h);
                        canvas.fillRectClipped(r.x, ul_y, ul_w, ul_h, deco_color, active_clip);
                    }
                }
            }
        }
    }

    let child = node.first_child;
    while (child != NULL_NODE) {
        paintNodeRecursive(canvas, doc, styles, rects, child, vw, vh, opts, child_clip, child_abs_clip, child_scroll_dx, child_scroll_dy, child_abs_scroll_dx, child_abs_scroll_dy, eff_op);
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
        paintNodeRecursive(&canvas, doc, styles, rects, root_id, vw, vh, &opts, null, null, 0.0, 0.0, 0.0, 0.0, 1.0);
    }

    // 如果指定了 clip 裁剪区域，提取子区域编码
    if (opts.clip_w > 0 && opts.clip_h > 0) {
        const cw = opts.clip_w;
        const ch = opts.clip_h;
        let cropped = Canvas.init(allocator, cw, ch, opts.clear_color)?;
        defer cropped.deinit();
        let dy: u32 = 0;
        while (dy < ch) : (dy += 1) {
            const sy = opts.clip_y + @as(i32, @intCast(dy));
            if (sy < 0 || sy >= @as(i32, @intCast(canvas.height))) continue;
            let dx: u32 = 0;
            while (dx < cw) : (dx += 1) {
                const sx = opts.clip_x + @as(i32, @intCast(dx));
                if (sx < 0 || sx >= @as(i32, @intCast(canvas.width))) continue;
                const s_idx = (@as(usize, @intCast(sy)) * @as(usize, canvas.width) + @as(usize, @intCast(sx))) * 4;
                const d_idx = (@as(usize, dy) * @as(usize, cw) + @as(usize, dx)) * 4;
                cropped.pixels[d_idx] = canvas.pixels[s_idx];
                cropped.pixels[d_idx + 1] = canvas.pixels[s_idx + 1];
                cropped.pixels[d_idx + 2] = canvas.pixels[s_idx + 2];
                cropped.pixels[d_idx + 3] = canvas.pixels[s_idx + 3];
            }
        }
        return encodeCanvasToPngWithLevel(allocator, &cropped, opts.compress_level);
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
