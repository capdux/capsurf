// src/cdp/emulation.cp
// CDP Emulation Domain & Viewport Clamping for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/viewport.ts

import { CdpRequest, CdpResponse, extractJsonString, extractJsonInt, extractJsonArray } from "./types.cp";
import { CdpPageSession } from "./session.cp";

pub const DEFAULT_VIEWPORT_WIDTH: f32 = 1280.0;
pub const DEFAULT_VIEWPORT_HEIGHT: f32 = 720.0;
pub const MAX_SIDE: f32 = 2560.0;
pub const MIN_DIM: f32 = 16.0;
pub const MAX_PIXELS: f32 = 2560.0 * 1440.0;

pub struct ViewportSize {
    width: f32 = DEFAULT_VIEWPORT_WIDTH;
    height: f32 = DEFAULT_VIEWPORT_HEIGHT;
}

pub class Viewport {
    static fn clampSide(n: f32): f32 {
        if (std.math.isNan(n) || std.math.isInf(n)) return MIN_DIM;
        return @max(MIN_DIM, @min(MAX_SIDE, @floor(n)));
    }

    pub static fn clampViewport(w: f32, h: f32): ViewportSize {
        let width = clampSide(w);
        let height = clampSide(h);
        const px = width * height;
        if (px > MAX_PIXELS) {
            const scale = @sqrt(MAX_PIXELS / px);
            width = @max(MIN_DIM, @floor(width * scale));
            height = @max(MIN_DIM, @floor(height * scale));
        }
        return ViewportSize{ width: width, height: height };
    }
}

pub struct EmulationHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8): CdpResponse {
        * = out_buf;

        // 1. Device metrics & screen viewport override
        if (std.mem.eql(u8, req.method, "Emulation.setDeviceMetricsOverride")) {
            const req_w = extractJsonInt(req.params_raw, "width") ?? 0;
            const req_h = extractJsonInt(req.params_raw, "height") ?? 0;
            const dsf = extractJsonInt(req.params_raw, "deviceScaleFactor") ?? 0;

            const w_f32: f32 = if (req_w > 0) @floatFromInt(req_w) else DEFAULT_VIEWPORT_WIDTH;
            const h_f32: f32 = if (req_h > 0) @floatFromInt(req_h) else DEFAULT_VIEWPORT_HEIGHT;
            const clamped = Viewport.clampViewport(w_f32, h_f32);

            session.viewport_width = clamped.width;
            session.viewport_height = clamped.height;
            session.layout.viewport_width = clamped.width;
            session.layout.viewport_height = clamped.height;
            if (dctx := session.dom_ctx) {
                dctx.layout_dirty = true;
            }

            if (dsf > 0) {
                session.device_scale_factor = @floatFromInt(dsf);
            }

            // Check if "mobile" flag is set
            if (std.mem.indexOf(u8, req.params_raw, "\"mobile\":true") != null) {
                session.is_mobile = true;
            } else if (std.mem.indexOf(u8, req.params_raw, "\"mobile\":false") != null) {
                session.is_mobile = false;
            }

            if (session.has_document) {
                let js_buf: [1024]u8 = [0; 1024];
                const js_expr = std.fmt.bufPrint(
                    &js_buf,
                    "(() => {{ const w = {d}; const h = {d}; if (typeof window !== 'undefined') {{ window.innerWidth = w; window.innerHeight = h; }} if (typeof global !== 'undefined') {{ global.innerWidth = w; global.innerHeight = h; }} if (typeof globalThis !== 'undefined') {{ globalThis.innerWidth = w; globalThis.innerHeight = h; }} }})()",
                    .{ @as(i64, @intFromFloat(clamped.width)), @as(i64, @intFromFloat(clamped.height)) },
                ) catch null;
                if (e := js_expr) {
                    const val = session.ctx.eval(e);
                    session.ctx.freeValue(val);
                }
            }

            return CdpResponse.ok(req.id, "{}");
        }

        // 2. Clear device metrics override to defaults
        if (std.mem.eql(u8, req.method, "Emulation.clearDeviceMetricsOverride")) {
            session.viewport_width = DEFAULT_VIEWPORT_WIDTH;
            session.viewport_height = DEFAULT_VIEWPORT_HEIGHT;
            session.layout.viewport_width = DEFAULT_VIEWPORT_WIDTH;
            session.layout.viewport_height = DEFAULT_VIEWPORT_HEIGHT;
            session.device_scale_factor = 1.0;
            session.is_mobile = false;
            if (dctx := session.dom_ctx) {
                dctx.layout_dirty = true;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        // 3. UserAgent override
        if (std.mem.eql(u8, req.method, "Emulation.setUserAgentOverride")) {
            const ua = extractJsonString(req.params_raw, "userAgent") ?? "";
            session.setUserAgentOverride(ua);
            return CdpResponse.ok(req.id, "{}");
        }

        // 3.5. Emulated media override (e.g., "print", "", and features array)
        if (std.mem.eql(u8, req.method, "Emulation.setEmulatedMedia")) {
            if (raw_media := extractJsonString(req.params_raw, "media")) {
                const mlen = @min(raw_media.len, session.emulated_media_buf.len);
                if (mlen > 0) @memcpy(session.emulated_media_buf[0..mlen], raw_media[0..mlen]);
                session.emulated_media_len = mlen;
                session.emulated_media_print = std.ascii.eqlIgnoreCase(raw_media, "print");
            }
            if (raw_features := extractJsonArray(req.params_raw, "features")) {
                const flen = @min(raw_features.len, session.emulated_features_buf.len);
                if (flen > 0) @memcpy(session.emulated_features_buf[0..flen], raw_features[0..flen]);
                session.emulated_features_len = flen;
            }
            if (dctx := session.dom_ctx) {
                dctx.layout_dirty = true;
            }
            session.applyEmulatedMediaToJs();
            return CdpResponse.ok(req.id, "{}");
        }

        // 4. Timezone, Locale & Touch Emulation
        if (std.mem.eql(u8, req.method, "Emulation.setTimezoneOverride") ||
            std.mem.eql(u8, req.method, "Emulation.setLocaleOverride") ||
            std.mem.eql(u8, req.method, "Emulation.setTouchEmulationEnabled") ||
            std.mem.eql(u8, req.method, "Emulation.setFocusEmulationEnabled") ||
            std.mem.eql(u8, req.method, "Emulation.setVirtualTimePolicy")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.ok(req.id, "{}");
    }
}
