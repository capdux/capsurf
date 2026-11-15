// src/cdp/emulation.cp
// CDP Emulation Domain & Viewport Clamping for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/viewport.ts

import { CdpRequest, CdpResponse, extractJsonString, extractJsonInt } from "./types.cp";
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

            if (dsf > 0) {
                session.device_scale_factor = @floatFromInt(dsf);
            }

            // Check if "mobile" flag is set
            if (_ := std.mem.indexOf(u8, req.params_raw, "\"mobile\":true")) {
                session.is_mobile = true;
            } else if (_ := std.mem.indexOf(u8, req.params_raw, "\"mobile\":false")) {
                session.is_mobile = false;
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
            return CdpResponse.ok(req.id, "{}");
        }

        // 3. UserAgent override
        if (std.mem.eql(u8, req.method, "Emulation.setUserAgentOverride")) {
            const ua = extractJsonString(req.params_raw, "userAgent") ?? "";
            if (ua.len > 0) {
                session.user_agent = ua;
            }
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
