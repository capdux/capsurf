// test/cdp/test_scene_stream_facade.cp
// Tests for Phase 24: Scene Host, Scene Stream & Kitesurf Facade

import {
    CHUNK_BYTES,
    HOST_BYTES,
    SceneHostManifest,
    validateRange,
    formatManifestJson,
    formatChunkResponse,
} from "../../../src/cdp/scene_host.cp";

import {
    MIN_SCENE_STREAM_WIDTH,
    MIN_SCENE_STREAM_HEIGHT,
    MAX_SCENE_STREAM_FPS,
    SceneStreamConfig,
    SceneStreamManager,
} from "../../../src/cdp/scene_stream.cp";

import {
    KitesurfFacade,
} from "../../../src/cdp/kitesurf_facade.cp";
import { CdpPageSession } from "../../../src/cdp/session.cp";

fn assertCheck(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn testSceneHost() {
    std.debug.print("1. Scene Host Manifest & Range Validation\n", .{});
    assertCheck("valid range 0..1024", validateRange(0, 1024, 2048));
    assertCheck("invalid offset outside artifact", !validateRange(2048, 1024, 2048));
    assertCheck("invalid length 0", !validateRange(0, 0, 2048));
    assertCheck("invalid length > 4MB", !validateRange(0, CHUNK_BYTES + 1, CHUNK_BYTES * 2));

    let manifest = SceneHostManifest{
        sha256: "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
        total_bytes: 524288,
    };

    let buf: [512]u8 = [0; 512];
    const json = formatManifestJson(manifest, buf[0..]);
    assertCheck("manifest json formatted", json.len > 0);
    assertCheck("has abi 1", std.mem.indexOf(u8, json, "\"abi\":1") != null);
    assertCheck("has wasm name", std.mem.indexOf(u8, json, "\"wasm\":\"scene_host.wasm\"") != null);
    assertCheck("has sha256", std.mem.indexOf(u8, json, "0123456789abcdef") != null);

    let chunk_buf: [512]u8 = [0; 512];
    const chunk_json = formatChunkResponse(manifest.sha256, 0, 524288, "AQIDBA==", chunk_buf[0..]);
    assertCheck("chunk json formatted", chunk_json.len > 0);
    assertCheck("has dataB64", std.mem.indexOf(u8, chunk_json, "\"dataB64\":\"AQIDBA==\"") != null);
    assertCheck("has totalBytes", std.mem.indexOf(u8, chunk_json, "\"totalBytes\":524288") != null);
}

fn testSceneStreamManager() {
    std.debug.print("2. Scene Stream Flow & Frame Ack\n", .{});
    let mgr = SceneStreamManager{};
    assertCheck("initial active count 0", mgr.active_count == 0);

    let cfg = SceneStreamConfig{
        target_id: "page-target-1",
        width: 100, // should clamp to MIN_SCENE_STREAM_WIDTH (320)
        height: 100, // should clamp to MIN_SCENE_STREAM_HEIGHT (240)
        fps: 120, // should clamp to 60
    };

    const started = mgr.startStream(cfg, "sess-1", 1000);
    assertCheck("stream started", started);
    assertCheck("active count 1", mgr.active_count == 1);

    const stream_opt = mgr.getStream("page-target-1");
    assertCheck("stream exists", stream_opt != null);
    if (*stream := stream_opt) {
        assertCheck("clamped width to 320", stream.config.width == MIN_SCENE_STREAM_WIDTH);
        assertCheck("clamped height to 240", stream.config.height == MIN_SCENE_STREAM_HEIGHT);
        assertCheck("clamped fps to 60", stream.config.fps == MAX_SCENE_STREAM_FPS);
        assertCheck("initial need_full_frame is true", stream.need_full_frame);
    }

    // Sequence progression
    const seq1 = mgr.nextFrameSeq("page-target-1");
    assertCheck("seq1 is 1", seq1 == 1);
    const seq2 = mgr.nextFrameSeq("page-target-1");
    assertCheck("seq2 is 2", seq2 == 2);

    // Ack with painted = true
    const ack1 = mgr.ackFrame("page-target-1", 1, true);
    assertCheck("ack1 succeeded", ack1);

    // Ack with painted = false (client dropped frame, requests full keyframe)
    if (*stream := stream_opt) {
        stream.need_full_frame = false;
    }
    const ack2 = mgr.ackFrame("page-target-1", 2, false);
    assertCheck("ack2 succeeded", ack2);
    if (*stream := stream_opt) {
        assertCheck("need_full_frame reset to true", stream.need_full_frame);
        assertCheck("last_acked_seq is 2", stream.last_acked_seq == 2);
    }

    // Stop stream
    const stopped = mgr.stopStream("page-target-1");
    assertCheck("stopped stream", stopped);
    assertCheck("active count 0", mgr.active_count == 0);
}

fn testKitesurfFacade() {
    std.debug.print("3. Kitesurf / CapSurf Facade Dispatch\n", .{});
    let facade = KitesurfFacade{};
    facade.host_manifest.sha256 = "aabbccddeeff00112233445566778899aabbccddeeff00112233445566778899";

    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [1024]u8 = [0; 1024];

    // getSceneHostManifest
    const res_man = facade.dispatchCommand("Kitesurf.getSceneHostManifest", "{}", "sess-1", &session, out_buf[0..]);
    assertCheck("manifest dispatch succeeded", res_man != null);
    if (m := res_man) {
        assertCheck("contains wasm name", std.mem.indexOf(u8, m, "scene_host.wasm") != null);
    }

    // readSceneHost
    const res_read = facade.dispatchCommand("Kitesurf.readSceneHost", "{}", "sess-1", &session, out_buf[0..]);
    assertCheck("readSceneHost succeeded", res_read != null);

    // startSceneStream
    const res_start = facade.dispatchCommand("Kitesurf.startSceneStream", "{}", "sess-1", &session, out_buf[0..]);
    assertCheck("startSceneStream succeeded", res_start != null);
    assertCheck("stream active in mgr", facade.stream_mgr.active_count == 1);

    // sceneFrameAck
    const res_ack = facade.dispatchCommand("Kitesurf.sceneFrameAck", "{}", "sess-1", &session, out_buf[0..]);
    assertCheck("sceneFrameAck succeeded", res_ack != null);

    // getSceneResources
    const res_res = facade.dispatchCommand("Kitesurf.getSceneResources", "{}", "sess-1", &session, out_buf[0..]);
    assertCheck("getSceneResources succeeded", res_res != null);

    // stopSceneStream
    const res_stop = facade.dispatchCommand("Kitesurf.stopSceneStream", "{}", "sess-1", &session, out_buf[0..]);
    assertCheck("stopSceneStream succeeded", res_stop != null);
    assertCheck("stream inactive in mgr", facade.stream_mgr.active_count == 0);

    // Passthrough to protocol extensions (getSessionInfo)
    const res_info = facade.dispatchCommand("CapSurf.getSessionInfo", "{}", "sess-1", &session, out_buf[0..]);
    assertCheck("getSessionInfo passthrough succeeded", res_info != null);
}

pub fn main() {
    std.debug.print("\n=== Phase 24: Scene Host, Scene Stream & Facade Tests ===\n", .{});
    testSceneHost();
    testSceneStreamManager();
    testKitesurfFacade();
    std.debug.print("\nAll Phase 24 tests passed successfully!\n", .{});
}
