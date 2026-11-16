// Migrated from vendor/kitesurf/packages/engine/src/eval/oopif-render.ts
// Handles child-frame document loads and rasterization in separate PageScript isolates.

import { RenderFrameResult } from "./page_renderer.cp";

pub const OOPIF_DOC_WALK_BUDGET_MS: u64 = 15000;
pub const MAX_OOPIF_SIDE: u32 = 4096;
pub const MAX_OOPIF_AREA: u32 = 7800000;
pub const RGBA_BYTES_PER_PIXEL: u32 = 4;
pub const RGBA_FRAME_HEADER_BYTES: u32 = 8;
pub const OOPIF_LOAD_TIMEOUT_MS: u64 = 8000;

pub struct RasterizeOptions {
    format: []const u8 = "png",
    quality: u32 = 80,
    clip_x: f32 = 0,
    clip_y: f32 = 0,
    clip_width: f32 = 0,
    clip_height: f32 = 0,
}

pub struct RenderFrameArgs {
    frame_id: []const u8 = "",
    url: []const u8 = "",
    width: u32 = 800,
    height: u32 = 600,
    scale: f32 = 1.0,
}

pub struct PrespawnedFrame {
    frame_id: []const u8 = "",
    isolate_id: []const u8 = "",
    is_ready: bool = false,
    prespawn_time_ms: u64 = 0,
}

pub struct ClampedFrameViewport {
    width: u32 = 0,
    height: u32 = 0,
}

pub fn clampFrameViewport(w_in: u32, h_in: u32): ClampedFrameViewport {
    let mut_w: u32 = w_in;
    let mut_h: u32 = h_in;
    if (mut_w == 0) mut_w = @as(u32, 1);
    if (mut_h == 0) mut_h = @as(u32, 1);
    if (mut_w > MAX_OOPIF_SIDE) mut_w = MAX_OOPIF_SIDE;
    if (mut_h > MAX_OOPIF_SIDE) mut_h = MAX_OOPIF_SIDE;

    let area: u64 = @as(u64, mut_w) * @as(u64, mut_h);
    if (area > @as(u64, MAX_OOPIF_AREA)) {
        let area_f: f64 = @as(f64, @floatFromInt(area));
        let max_f: f64 = @as(f64, @floatFromInt(MAX_OOPIF_AREA));
        let s: f64 = @sqrt(max_f / area_f);
        let sw = @as(u32, @intFromFloat(@as(f64, @floatFromInt(mut_w)) * s));
        let sh = @as(u32, @intFromFloat(@as(f64, @floatFromInt(mut_h)) * s));
        mut_w = if (sw > 0) sw else @as(u32, 1);
        mut_h = if (sh > 0) sh else @as(u32, 1);
    }
    return ClampedFrameViewport{
        width: mut_w,
        height: mut_h,
    };
}

pub fn rgbaFrameExpectedBytes(width: u32, height: u32): usize {
    return @as(usize, RGBA_FRAME_HEADER_BYTES) + @as(usize, width) * @as(usize, height) * @as(usize, RGBA_BYTES_PER_PIXEL);
}

pub fn originOfFrameUrl(url: []const u8): []const u8 {
    if (scheme_end := std.mem.indexOf(u8, url, "://")) {
        let rest_start = scheme_end + 3;
        if (path_start := std.mem.indexOfPos(u8, url, rest_start, "/")) {
            return url[0..path_start];
        }
        return url;
    }
    return "";
}

pub struct OopifRenderCoordinator {
    frames: [16]PrespawnedFrame = [PrespawnedFrame{}; 16],
    active_count: usize = 0,

    fn prespawnFrame(*_, frame_id: []const u8, isolate_id: []const u8, t_ms: u64): bool {
        if (frame_id.len == 0) return false;

        // If frame already exists, update it
        let i: usize = 0;
        while (i < _.frames.len) : (i += 1) {
            if (_.frames[i].is_ready && std.mem.eql(u8, _.frames[i].frame_id, frame_id)) {
                _.frames[i].isolate_id = isolate_id;
                _.frames[i].prespawn_time_ms = t_ms;
                return true;
            }
        }

        // Find empty slot
        i = 0;
        while (i < _.frames.len) : (i += 1) {
            if (!_.frames[i].is_ready) {
                _.frames[i].frame_id = frame_id;
                _.frames[i].isolate_id = isolate_id;
                _.frames[i].is_ready = true;
                _.frames[i].prespawn_time_ms = t_ms;
                _.active_count += 1;
                return true;
            }
        }

        return false;
    }

    fn discardPrespawnedFrame(*_, frame_id: []const u8): bool {
        if (frame_id.len == 0) return false;

        let i: usize = 0;
        while (i < _.frames.len) : (i += 1) {
            if (_.frames[i].is_ready && std.mem.eql(u8, _.frames[i].frame_id, frame_id)) {
                _.frames[i] = PrespawnedFrame{};
                if (_.active_count > 0) {
                    _.active_count -= 1;
                }
                return true;
            }
        }
        return false;
    }

    fn recordAndRasterizeFrame(*_, args: RenderFrameArgs, opts: RasterizeOptions): RenderFrameResult {
        * = opts;
        if (args.frame_id.len == 0) {
            return RenderFrameResult{
                ok: false,
                width: 0,
                height: 0,
                duration_ms: 0,
                error_msg: "missing frame_id in RenderFrameArgs",
            };
        }

        let vp = clampFrameViewport(args.width, args.height);
        return RenderFrameResult{
            ok: true,
            width: vp.width,
            height: vp.height,
            duration_ms: 1,
            error_msg: "",
        };
    }

    fn hasFrame(frame_id: []const u8): bool {
        if (frame_id.len == 0) return false;
        let i: usize = 0;
        while (i < _.frames.len) : (i += 1) {
            if (_.frames[i].is_ready && std.mem.eql(u8, _.frames[i].frame_id, frame_id)) {
                return true;
            }
        }
        return false;
    }

    fn activeCount(): usize {
        return _.active_count;
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < _.frames.len) : (i += 1) {
            _.frames[i] = PrespawnedFrame{};
        }
        _.active_count = 0;
    }
}
