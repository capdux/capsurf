// Migrated from vendor/kitesurf/packages/dom-shim/src/canvas2d.ts
// Native Canvas 2D Graphics Engine and Software Raster Surface for CapSurf.

import { CssColor } from "cap:css";
import { PngCodec } from "../render/png.cp";

pub const MAX_CANVAS_WIDTH: u32 = 4096;
pub const MAX_CANVAS_HEIGHT: u32 = 4096;
pub const MAX_PATH_POINTS: usize = 256;
pub const MAX_STATE_STACK: usize = 16;

pub struct Point {
    x: f32 = 0.0,
    y: f32 = 0.0,
}

pub struct CanvasState {
    fill_style: CssColor = CssColor.rgb(0, 0, 0),
    stroke_style: CssColor = CssColor.rgb(0, 0, 0),
    line_width: f32 = 1.0,
    global_alpha: f32 = 1.0,
}

pub struct ImageData {
    width: u32 = 0,
    height: u32 = 0,
    data: []u8 = [],
}

pub struct CanvasRenderingContext2D {
    width: u32 = 300,
    height: u32 = 150,
    pixels: []u8 = [],
    allocator: std.mem.Allocator,
    current_state: CanvasState = CanvasState{},
    state_stack: [MAX_STATE_STACK]CanvasState = [CanvasState{}; 16],
    state_depth: usize = 0,
    path_points: [MAX_PATH_POINTS]Point = [Point{}; 256],
    point_count: usize = 0,
    is_closed: bool = false,

    static fn init(allocator: std.mem.Allocator, width: u32, height: u32): CanvasRenderingContext2D! {
        let w = if (width > 0 && width <= MAX_CANVAS_WIDTH) width else 300;
        let h = if (height > 0 && height <= MAX_CANVAS_HEIGHT) height else 150;
        let byte_len = @as(usize, w) * @as(usize, h) * 4;
        let px = allocator.alloc(u8, byte_len)?;
        // Initialize transparent black
        @memset(px, 0);

        return CanvasRenderingContext2D{
            width: w,
            height: h,
            pixels: px,
            allocator: allocator,
            current_state: CanvasState{},
        };
    }

    fn deinit(*_) {
        if (_.pixels.len > 0) {
            _.allocator.free(_.pixels);
            _.pixels = [];
        }
    }

    fn save(*_) {
        if (_.state_depth < MAX_STATE_STACK) {
            _.state_stack[_.state_depth] = _.current_state;
            _.state_depth += 1;
        }
    }

    fn restore(*_) {
        if (_.state_depth > 0) {
            _.state_depth -= 1;
            _.current_state = _.state_stack[_.state_depth];
        }
    }

    fn setFillStyle(*_, color: CssColor) {
        _.current_state.fill_style = color;
    }

    fn setStrokeStyle(*_, color: CssColor) {
        _.current_state.stroke_style = color;
    }

    fn setLineWidth(*_, width: f32) {
        if (width > 0.0) {
            _.current_state.line_width = width;
        }
    }

    fn setGlobalAlpha(*_, alpha: f32) {
        if (alpha >= 0.0 && alpha <= 1.0) {
            _.current_state.global_alpha = alpha;
        }
    }

    fn setPixelDirect(*_, x: i32, y: i32, c: CssColor) {
        if (x < 0 || y < 0 || x >= @as(i32, @intCast(_.width)) || y >= @as(i32, @intCast(_.height))) {
            return;
        }
        let idx = (@as(usize, @intCast(y)) * @as(usize, _.width) + @as(usize, @intCast(x))) * 4;
        let alpha = @as(f32, @floatFromInt(c.a)) * _.current_state.global_alpha;
        let final_a = @as(u8, @intFromFloat(alpha));
        if (final_a == 255) {
            _.pixels[idx] = c.r;
            _.pixels[idx + 1] = c.g;
            _.pixels[idx + 2] = c.b;
            _.pixels[idx + 3] = 255;
        } else if (final_a > 0) {
            // Alpha blend over existing pixel
            let sa = @as(u32, final_a);
            let inv_a = 255 - sa;
            _.pixels[idx] = @as(u8, @intCast((@as(u32, c.r) * sa + @as(u32, _.pixels[idx]) * inv_a) / 255));
            _.pixels[idx + 1] = @as(u8, @intCast((@as(u32, c.g) * sa + @as(u32, _.pixels[idx + 1]) * inv_a) / 255));
            _.pixels[idx + 2] = @as(u8, @intCast((@as(u32, c.b) * sa + @as(u32, _.pixels[idx + 2]) * inv_a) / 255));
            let da = @as(u32, _.pixels[idx + 3]);
            _.pixels[idx + 3] = @as(u8, @intCast(sa + (da * inv_a) / 255));
        }
    }

    fn fillRect(*_, x: f32, y: f32, w: f32, h: f32) {
        if (w <= 0.0 || h <= 0.0) return;
        let x0: i32 = @as(i32, @intFromFloat(x));
        let y0: i32 = @as(i32, @intFromFloat(y));
        let x1: i32 = @as(i32, @intFromFloat(x + w));
        let y1: i32 = @as(i32, @intFromFloat(y + h));

        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        let max_w: i32 = @as(i32, @intCast(_.width));
        let max_h: i32 = @as(i32, @intCast(_.height));
        if (x1 > max_w) x1 = max_w;
        if (y1 > max_h) y1 = max_h;

        let py: i32 = y0;
        while (py < y1) : (py += 1) {
            let px: i32 = x0;
            while (px < x1) : (px += 1) {
                _.setPixelDirect(px, py, _.current_state.fill_style);
            }
        }
    }

    fn clearRect(*_, x: f32, y: f32, w: f32, h: f32) {
        if (w <= 0.0 || h <= 0.0) return;
        let x0: i32 = @as(i32, @intFromFloat(x));
        let y0: i32 = @as(i32, @intFromFloat(y));
        let x1: i32 = @as(i32, @intFromFloat(x + w));
        let y1: i32 = @as(i32, @intFromFloat(y + h));

        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        let max_w: i32 = @as(i32, @intCast(_.width));
        let max_h: i32 = @as(i32, @intCast(_.height));
        if (x1 > max_w) x1 = max_w;
        if (y1 > max_h) y1 = max_h;

        let py: i32 = y0;
        while (py < y1) : (py += 1) {
            let px: i32 = x0;
            while (px < x1) : (px += 1) {
                let idx = (@as(usize, @intCast(py)) * @as(usize, _.width) + @as(usize, @intCast(px))) * 4;
                _.pixels[idx] = 0;
                _.pixels[idx + 1] = 0;
                _.pixels[idx + 2] = 0;
                _.pixels[idx + 3] = 0;
            }
        }
    }

    fn strokeRect(*_, x: f32, y: f32, w: f32, h: f32) {
        if (w <= 0.0 || h <= 0.0) return;
        let lw = _.current_state.line_width;
        // Top edge
        _.fillRect(x, y, w, lw);
        // Bottom edge
        _.fillRect(x, y + h - lw, w, lw);
        // Left edge
        _.fillRect(x, y, lw, h);
        // Right edge
        _.fillRect(x + w - lw, y, lw, h);
    }

    fn beginPath(*_) {
        _.point_count = 0;
        _.is_closed = false;
    }

    fn moveTo(*_, x: f32, y: f32) {
        if (_.point_count < MAX_PATH_POINTS) {
            _.path_points[_.point_count] = Point{ x: x, y: y };
            _.point_count += 1;
        }
    }

    fn lineTo(*_, x: f32, y: f32) {
        if (_.point_count < MAX_PATH_POINTS) {
            _.path_points[_.point_count] = Point{ x: x, y: y };
            _.point_count += 1;
        }
    }

    fn rect(*_, x: f32, y: f32, w: f32, h: f32) {
        _.moveTo(x, y);
        _.lineTo(x + w, y);
        _.lineTo(x + w, y + h);
        _.lineTo(x, y + h);
        _.closePath();
    }

    fn closePath(*_) {
        if (_.point_count > 0 && !_.is_closed) {
            _.is_closed = true;
        }
    }

    fn drawLineBresenham(*_, x0_f: f32, y0_f: f32, x1_f: f32, y1_f: f32, color: CssColor) {
        let x0: i32 = @as(i32, @intFromFloat(x0_f));
        let y0: i32 = @as(i32, @intFromFloat(y0_f));
        let x1: i32 = @as(i32, @intFromFloat(x1_f));
        let y1: i32 = @as(i32, @intFromFloat(y1_f));

        let dx: i32 = if (x1 >= x0) x1 - x0 else x0 - x1;
        let sx: i32 = if (x0 < x1) 1 else -1;
        let dy: i32 = if (y1 >= y0) -(y1 - y0) else -(y0 - y1);
        let sy: i32 = if (y0 < y1) 1 else -1;
        let err: i32 = dx + dy;

        while (true) {
            _.setPixelDirect(x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            let e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }

    fn stroke(*_) {
        if (_.point_count < 2) return;
        let i: usize = 0;
        while (i < _.point_count - 1) : (i += 1) {
            let p0 = _.path_points[i];
            let p1 = _.path_points[i + 1];
            _.drawLineBresenham(p0.x, p0.y, p1.x, p1.y, _.current_state.stroke_style);
        }
        if (_.is_closed && _.point_count > 2) {
            let p0 = _.path_points[_.point_count - 1];
            let p1 = _.path_points[0];
            _.drawLineBresenham(p0.x, p0.y, p1.x, p1.y, _.current_state.stroke_style);
        }
    }

    fn getImageData(*_, sx: i32, sy: i32, sw: u32, sh: u32, out_buf: []u8): ImageData {
        let req_len = @as(usize, sw) * @as(usize, sh) * 4;
        if (out_buf.len < req_len) {
            return ImageData{};
        }
        let dy: u32 = 0;
        while (dy < sh) : (dy += 1) {
            let src_y = sy + @as(i32, @intCast(dy));
            let dx: u32 = 0;
            while (dx < sw) : (dx += 1) {
                let src_x = sx + @as(i32, @intCast(dx));
                let dst_idx = (@as(usize, dy) * @as(usize, sw) + @as(usize, dx)) * 4;
                if (src_x >= 0 && src_y >= 0 && src_x < @as(i32, @intCast(_.width)) && src_y < @as(i32, @intCast(_.height))) {
                    let src_idx = (@as(usize, @intCast(src_y)) * @as(usize, _.width) + @as(usize, @intCast(src_x))) * 4;
                    out_buf[dst_idx] = _.pixels[src_idx];
                    out_buf[dst_idx + 1] = _.pixels[src_idx + 1];
                    out_buf[dst_idx + 2] = _.pixels[src_idx + 2];
                    out_buf[dst_idx + 3] = _.pixels[src_idx + 3];
                } else {
                    out_buf[dst_idx] = 0;
                    out_buf[dst_idx + 1] = 0;
                    out_buf[dst_idx + 2] = 0;
                    out_buf[dst_idx + 3] = 0;
                }
            }
        }
        return ImageData{
            width: sw,
            height: sh,
            data: out_buf[0..req_len],
        };
    }

    fn putImageData(*_, img: ImageData, dx: i32, dy: i32) {
        let py: u32 = 0;
        while (py < img.height) : (py += 1) {
            let target_y = dy + @as(i32, @intCast(py));
            if (target_y < 0 || target_y >= @as(i32, @intCast(_.height))) continue;

            let px: u32 = 0;
            while (px < img.width) : (px += 1) {
                let target_x = dx + @as(i32, @intCast(px));
                if (target_x < 0 || target_x >= @as(i32, @intCast(_.width))) continue;

                let src_idx = (@as(usize, py) * @as(usize, img.width) + @as(usize, px)) * 4;
                let dst_idx = (@as(usize, @intCast(target_y)) * @as(usize, _.width) + @as(usize, @intCast(target_x))) * 4;

                _.pixels[dst_idx] = img.data[src_idx];
                _.pixels[dst_idx + 1] = img.data[src_idx + 1];
                _.pixels[dst_idx + 2] = img.data[src_idx + 2];
                _.pixels[dst_idx + 3] = img.data[src_idx + 3];
            }
        }
    }

    fn encodePng(*_): []const u8 {
        return PngCodec.encodeRgbaWithLevel(_.pixels, _.width, _.height, 6);
    }
}
