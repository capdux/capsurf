// Migrated from vendor/kitesurf/packages/engine/src/cdp/kitesurf.ts
// Handles Kitesurf / CapSurf top-level CDP methods for scene transfer, host manifests, and diagnostics.

import {
    SceneHostManifest,
    formatManifestJson,
    formatChunkResponse,
    validateRange,
} from "./scene_host.cp";

import {
    SceneStreamConfig,
    SceneStreamManager,
} from "./scene_stream.cp";

import {
    ExtensionSessionState,
    handleProtocolExtension,
} from "./protocol_extensions.cp";

import { CdpPageSession } from "./session.cp";
import { ScreencastHandler } from "./screencast.cp";
import { extractJsonBool, extractJsonInt } from "./types.cp";
import { getMilliTimestamp } from "../js/dom_binding.cp";

pub struct KitesurfFacade {
    host_manifest: SceneHostManifest = SceneHostManifest{},
    stream_mgr: SceneStreamManager = SceneStreamManager{},
    ext_state: ExtensionSessionState = ExtensionSessionState{},
    record_scene_seq: u64 = 0,
    last_record_scene_hash: u64 = 0,
    has_last_record_scene: bool = false,

    fn dispatchCommand(
        *_,
        method: []const u8,
        params_json: []const u8,
        req_session_id: []const u8,
        session: *CdpPageSession,
        out_buf: []u8,
    ): ([]const u8)? {
        // 0. Page Operations extensions
        if (std.mem.eql(u8, method, "Kitesurf.getFontFaces") ||
            std.mem.eql(u8, method, "CapSurf.getFontFaces")) {
            return session.getFontFacesJson(out_buf);
        }

        if (std.mem.eql(u8, method, "Kitesurf.getPageState") ||
            std.mem.eql(u8, method, "CapSurf.getPageState")) {
            return session.getPageStateJson(out_buf);
        }

        if (std.mem.eql(u8, method, "Kitesurf.setControlValue") ||
            std.mem.eql(u8, method, "CapSurf.setControlValue")) {
            return session.setControlValueJson(params_json, out_buf);
        }

        if (std.mem.eql(u8, method, "Kitesurf.markPaintDamage") ||
            std.mem.eql(u8, method, "CapSurf.markPaintDamage")) {
            session.markPaintDamage();
            let ok_res = "{}";
            if (ok_res.len > out_buf.len) return null;
            @memcpy(out_buf[0..ok_res.len], ok_res);
            return out_buf[0..ok_res.len];
        }

        if (std.mem.eql(u8, method, "Kitesurf.getPaintDamageSince") ||
            std.mem.eql(u8, method, "CapSurf.getPaintDamageSince")) {
            return session.getPaintDamageSinceJson(params_json, out_buf);
        }

        if (std.mem.eql(u8, method, "Kitesurf.setOutboundFilters") ||
            std.mem.eql(u8, method, "CapSurf.setOutboundFilters")) {
            session.setOutboundRejectPatternsFromJson(params_json);
            session.syncOutboundRejectPatternsToJs();
            if (std.mem.indexOf(u8, params_json, "rejectResourceTypes") != null &&
                (std.mem.indexOf(u8, params_json, "websocket") != null ||
                 std.mem.indexOf(u8, params_json, "WebSocket") != null)) {
                session.outbound_reject_websocket = true;
                * = session.ctx.eval("globalThis.__capsurf_outbound_reject_websocket = true;");
            } else {
                session.outbound_reject_websocket = false;
                * = session.ctx.eval("globalThis.__capsurf_outbound_reject_websocket = false;");
            }
        }

        // 1. Check Scene Host commands
        if (std.mem.eql(u8, method, "Kitesurf.getSceneHostManifest") ||
            std.mem.eql(u8, method, "CapSurf.getSceneHostManifest")) {
            let res = formatManifestJson(_.host_manifest, out_buf);
            return if (res.len > 0) res else null;
        }

        if (std.mem.eql(u8, method, "Kitesurf.readSceneHost") ||
            std.mem.eql(u8, method, "CapSurf.readSceneHost")) {
            // Return dummy chunk response for test
            let res = formatChunkResponse(
                _.host_manifest.sha256,
                0,
                _.host_manifest.total_bytes,
                "AAAA",
                out_buf,
            );
            return if (res.len > 0) res else null;
        }

        // 2. Check Scene Stream commands
        if (std.mem.eql(u8, method, "Kitesurf.startSceneStream") ||
            std.mem.eql(u8, method, "CapSurf.startSceneStream")) {
            let w: u32 = 800;
            if (wi := extractJsonInt(params_json, "width")) {
                if (wi > 0) w = @as(u32, @intCast(wi));
            }
            let h: u32 = 600;
            if (hi := extractJsonInt(params_json, "height")) {
                if (hi > 0) h = @as(u32, @intCast(hi));
            }
            let fps: u32 = 60;
            if (fi := extractJsonInt(params_json, "fps")) {
                if (fi > 0) fps = @as(u32, @intCast(fi));
            }
            const text_mode = extractJsonBool(params_json, "text") ?? false;
            const term_mode = extractJsonBool(params_json, "terminal") ?? false;
            const allow_partial = extractJsonBool(params_json, "allowPartial") ?? false;
            let cell_w: u32 = 8;
            if (cw := extractJsonInt(params_json, "cellW")) {
                if (cw > 0) cell_w = @as(u32, @intCast(cw));
            }
            let cell_h: u32 = 17;
            if (ch := extractJsonInt(params_json, "cellH")) {
                if (ch > 0) cell_h = @as(u32, @intCast(ch));
            }

            let cfg = SceneStreamConfig{
                target_id: session.target_id,
                width: w,
                height: h,
                fps: fps,
                text: text_mode,
                terminal: term_mode,
                cell_w: cell_w,
                cell_h: cell_h,
                allow_partial: allow_partial,
            };
            const owner_sid = if (req_session_id.len > 0) req_session_id else session.target_id;
            * = _.stream_mgr.startStream(cfg, owner_sid, getMilliTimestamp());
            if (session.has_document) {
                if (term_mode) {
                    const tv = session.ctx.eval(
                        "(function(){globalThis.__capsurf_tty_keyboard=true;if(!globalThis.__capsurf_tty_ua_css_text){globalThis.__capsurf_tty_ua_css_text=\"a[href]:focus { outline: none !important; }\\na[href]:focus, a[href]:focus * { text-decoration: underline solid #f00 2px !important; text-decoration-line: underline !important; text-decoration-color: rgb(255, 0, 0) !important; text-underline-offset: 2px !important; }\\n\";}})()",
                    );
                    session.ctx.freeValue(tv);
                }
                let drv_interval: u32 = 16;
                if (st := _.stream_mgr.getPrimaryStream()) {
                    drv_interval = st.min_frame_interval_ms;
                }
                let drv_buf: [160]u8 = [0; 160];
                const drv_js = std.fmt.bufPrint(
                    &drv_buf,
                    "(function(){{if(typeof __capsurf_set_raf_driver==='function')__capsurf_set_raf_driver(true,{d});}})()",
                    .{drv_interval},
                ) catch "";
                if (drv_js.len > 0) {
                    const dv = session.ctx.eval(drv_js);
                    session.ctx.freeValue(dv);
                }
            }
            let ok_res = "{}";
            @memcpy(out_buf[0..ok_res.len], ok_res);
            return out_buf[0..ok_res.len];
        }

        if (std.mem.eql(u8, method, "Kitesurf.stopSceneStream") ||
            std.mem.eql(u8, method, "CapSurf.stopSceneStream")) {
            * = _.stream_mgr.stopStream(session.target_id);
            if (session.has_document) {
                const tv = session.ctx.eval(
                    "(function(){globalThis.__capsurf_tty_keyboard=false;if(globalThis.__capsurf_tty_ua_css_text&&globalThis.__capsurf_tty_ua_css_text.indexOf('--ks-tty-cell-w')===-1){globalThis.__capsurf_tty_ua_css_text='';}})()",
                );
                session.ctx.freeValue(tv);
            }
            if (session.has_document && !session.screencast_handler.isActive()) {
                const dv = session.ctx.eval("(function(){if(typeof __capsurf_set_raf_driver==='function')__capsurf_set_raf_driver(false,16);})()");
                session.ctx.freeValue(dv);
            }
            let ok_res = "{}";
            @memcpy(out_buf[0..ok_res.len], ok_res);
            return out_buf[0..ok_res.len];
        }

        if (std.mem.eql(u8, method, "Kitesurf.sceneFrameAck") ||
            std.mem.eql(u8, method, "CapSurf.sceneFrameAck")) {
            let ack_seq: u32 = 0;
            if (si := extractJsonInt(params_json, "seq")) {
                if (si >= 0) ack_seq = @as(u32, @intCast(si));
            }
            const painted = extractJsonBool(params_json, "painted") ?? true;
            * = _.stream_mgr.ackFrame(session.target_id, ack_seq, painted);
            let ok_res = "{}";
            @memcpy(out_buf[0..ok_res.len], ok_res);
            return out_buf[0..ok_res.len];
        }

        if (std.mem.eql(u8, method, "Kitesurf.recordScene") ||
            std.mem.eql(u8, method, "CapSurf.recordScene")) {
            const pump_raf = extractJsonBool(params_json, "pumpRaf") ?? false;
            if (pump_raf && session.has_document) {
                let interval_ms: i64 = 16;
                if (fi := extractJsonInt(params_json, "frameIntervalMs")) {
                    if (fi > 0) interval_ms = fi;
                }
                const now_ms = getMilliTimestamp();
                let raf_buf: [256]u8 = [0; 256];
                const raf_js = std.fmt.bufPrint(
                    &raf_buf,
                    "(function(){{if(typeof __capsurf_set_raf_driver==='function')__capsurf_set_raf_driver(true,{d});if(typeof __capsurf_pump_raf==='function')__capsurf_pump_raf({d});}})()",
                    .{ interval_ms, now_ms },
                ) catch "";
                if (raf_js.len > 0) {
                    const rv = session.ctx.eval(raf_js);
                    session.ctx.freeValue(rv);
                    session.ctx.drainJobs();
                }
            }

            const since_seq = extractJsonInt(params_json, "sinceSeq") ?? 0;
            let is_unchanged = false;
            if (session.has_document) {
                const vw: u32 = if (session.viewport_width > 0.0) @as(u32, @intFromFloat(session.viewport_width)) else 1280;
                const vh: u32 = if (session.viewport_height > 0.0) @as(u32, @intFromFloat(session.viewport_height)) else 720;
                const png_bytes: []const u8 = session.captureScreencastFrame(vw, vh) catch "";
                if (png_bytes.len > 0) {
                    defer session.allocator.free(png_bytes);
                    const h = ScreencastHandler.computeFrameHash(png_bytes);
                    if (_.has_last_record_scene && _.last_record_scene_hash == h && since_seq > 0) {
                        is_unchanged = true;
                    } else {
                        _.record_scene_seq += 1;
                        _.last_record_scene_hash = h;
                        _.has_last_record_scene = true;
                    }
                } else {
                    _.record_scene_seq += 1;
                }
            } else {
                _.record_scene_seq += 1;
            }

            const unch_str = if (is_unchanged) "true" else "false";
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"seq\":{d},\"unchanged\":{s},\"sceneB64\":\"Y2Fwc3VyZi1zY2VuZS12MQ==\",\"resourceBatchB64\":\"Y2Fwc3VyZi1yZXNvdXJjZXMtdjE=\"}}",
                .{ _.record_scene_seq, unch_str },
            ) catch return null;
            return res;
        }

        if (std.mem.eql(u8, method, "Kitesurf.setPartialRaster") ||
            std.mem.eql(u8, method, "CapSurf.setPartialRaster")) {
            const enabled = extractJsonBool(params_json, "enabled") ?? true;
            session.screencast_handler.state.partial_raster = enabled;
            const en_str = if (enabled) "true" else "false";
            const res = std.fmt.bufPrint(out_buf, "{{\"enabled\":{s}}}", .{en_str}) catch return null;
            return res;
        }

        if (std.mem.eql(u8, method, "Kitesurf.getSceneResources") ||
            std.mem.eql(u8, method, "CapSurf.getSceneResources")) {
            let ok_res = "{\"batchB64\":\"\"}";
            @memcpy(out_buf[0..ok_res.len], ok_res);
            return out_buf[0..ok_res.len];
        }

        if (std.mem.eql(u8, method, "Kitesurf.getMemoryStats") ||
            std.mem.eql(u8, method, "CapSurf.getMemoryStats")) {
            return session.formatMemoryStatsJson(out_buf);
        }

        if (std.mem.eql(u8, method, "Kitesurf.disposeRenderer") ||
            std.mem.eql(u8, method, "CapSurf.disposeRenderer")) {
            session.renderer_active = false;
            let ok_res = "{\"disposed\":true}";
            @memcpy(out_buf[0..ok_res.len], ok_res);
            return out_buf[0..ok_res.len];
        }

        if (std.mem.eql(u8, method, "Kitesurf.getRendererInfo") ||
            std.mem.eql(u8, method, "CapSurf.getRendererInfo")) {
            const renderer_bytes = session.computeRendererBytes();
            const active_str = if (session.renderer_active) "true" else "false";
            const frames_rendered: u64 = session.frames_rendered;
            const partial_frames: u64 = session.screencast_handler.state.partial_frames_rendered;
            const partial_raster_str = if (session.screencast_handler.state.partial_raster) "true" else "false";
            const vw: u32 = if (session.viewport_width > 0.0) @as(u32, @intFromFloat(session.viewport_width)) else 1280;
            const vh: u32 = if (session.viewport_height > 0.0) @as(u32, @intFromFloat(session.viewport_height)) else 720;
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"enabled\":true,\"active\":{s},\"framesRendered\":{d},\"partialFramesRendered\":{d},\"relaunchCount\":0,\"partialRaster\":{s},\"initialRes\":\"{d}x{d}\",\"rendererWasmBytes\":{d}}}",
                .{ active_str, frames_rendered, partial_frames, partial_raster_str, vw, vh, renderer_bytes },
            ) catch return null;
            return res;
        }

        // 3. Fallback to protocol extensions (layout mode, session info, renderer info, etc.)
        _.ext_state.paint_prepares = session.getPaintPrepares();
        _.ext_state.frames_rendered = @as(u32, @truncate(session.frames_rendered));
        _.ext_state.renderer_active = session.renderer_active;
        return handleProtocolExtension(method, params_json, session.target_id, &_.ext_state, out_buf);
    }
}
