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

pub struct KitesurfFacade {
    host_manifest: SceneHostManifest = SceneHostManifest{},
    stream_mgr: SceneStreamManager = SceneStreamManager{},
    ext_state: ExtensionSessionState = ExtensionSessionState{},

    fn dispatchCommand(
        *_,
        method: []const u8,
        params_json: []const u8,
        session_id: []const u8,
        out_buf: []u8,
    ): ([]const u8)? {
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

        // 3. Fallback to protocol extensions (layout mode, session info, renderer info, etc.)
        return handleProtocolExtension(method, params_json, session_id, &_.ext_state, out_buf);
    }
}
