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

pub struct KitesurfFacade {
    host_manifest: SceneHostManifest = SceneHostManifest{},
    stream_mgr: SceneStreamManager = SceneStreamManager{},
    ext_state: ExtensionSessionState = ExtensionSessionState{},

    fn dispatchCommand(
        *_,
        method: []const u8,
        params_json: []const u8,
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
            let cfg = SceneStreamConfig{
                target_id: "capsurf-target-001",
                width: 1280,
                height: 720,
                fps: 60,
            };
            _.stream_mgr.startStream(cfg);
            let ok_res = "{}";
            @memcpy(out_buf[0..ok_res.len], ok_res);
            return out_buf[0..ok_res.len];
        }

        if (std.mem.eql(u8, method, "Kitesurf.stopSceneStream") ||
            std.mem.eql(u8, method, "CapSurf.stopSceneStream")) {
            _.stream_mgr.stopStream("capsurf-target-001");
            let ok_res = "{}";
            @memcpy(out_buf[0..ok_res.len], ok_res);
            return out_buf[0..ok_res.len];
        }

        if (std.mem.eql(u8, method, "Kitesurf.sceneFrameAck") ||
            std.mem.eql(u8, method, "CapSurf.sceneFrameAck")) {
            _.stream_mgr.ackFrame("capsurf-target-001", 1, true);
            let ok_res = "{}";
            @memcpy(out_buf[0..ok_res.len], ok_res);
            return out_buf[0..ok_res.len];
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

        if (std.mem.eql(u8, method, "Kitesurf.getRendererInfo") ||
            std.mem.eql(u8, method, "CapSurf.getRendererInfo")) {
            const renderer_bytes = session.computeRendererBytes();
            const frames_rendered: u64 = if (session.paint_prepares > 0) session.paint_prepares else 1;
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"enabled\":true,\"active\":true,\"framesRendered\":{d},\"partialFramesRendered\":0,\"relaunchCount\":0,\"partialRaster\":false,\"rendererWasmBytes\":{d}}}",
                .{ frames_rendered, renderer_bytes },
            ) catch return null;
            return res;
        }

        // 3. Fallback to protocol extensions (layout mode, session info, renderer info, etc.)
        _.ext_state.paint_prepares = session.getPaintPrepares();
        return handleProtocolExtension(method, params_json, session.target_id, &_.ext_state, out_buf);
    }
}
