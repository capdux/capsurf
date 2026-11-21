// test/cdp/test_scene_stream_envelope_edge.cp
// Phase 242 Slot A: Edge tests for scene_stream binary envelopes, in-flight limit, ACK release & idle frame dedup.

import {
    MAX_SCENES_IN_FLIGHT,
    MSG_INFO,
    MSG_SCENE,
    MSG_PARTIAL,
    CODEC_SCENE,
    SCENE_ENVELOPE_HEADER_BYTES,
    SceneStreamConfig,
    SceneStreamManager,
} from "../../../src/cdp/scene_stream.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn testInfoEnvelope() {
    std.debug.print("1. buildInfoEnvelope 11B header & boundary checks\n", .{});
    check("SCENE_ENVELOPE_HEADER_BYTES is 11", SCENE_ENVELOPE_HEADER_BYTES == 11);

    const payload = "{\"type\":\"info\"}";
    let small_buf: [20]u8 = [0xAA; 20];
    // 11 + 15 = 26 > 20 -> must return null
    check("info envelope rejects undersized buffer", SceneStreamManager.buildInfoEnvelope(payload, small_buf[0..]) == null);

    // Exact buffer size = 11 + payload.len
    let exact_buf: [26]u8 = [0xAA; 26];
    const res_opt = SceneStreamManager.buildInfoEnvelope(payload, exact_buf[0..]);
    check("info envelope accepts exact buffer", res_opt != null);
    if (env := res_opt) {
        check("info envelope total len", env.len == SCENE_ENVELOPE_HEADER_BYTES + payload.len);
        check("info byte 0 is MSG_INFO", env[0] == MSG_INFO);
        let i: usize = 1;
        while (i < SCENE_ENVELOPE_HEADER_BYTES) : (i += 1) {
            check("info header bytes 1..11 zeroed", env[i] == 0);
        }
        check("info payload matches", std.mem.eql(u8, env[SCENE_ENVELOPE_HEADER_BYTES..], payload));
    }

    // Empty payload
    let hdr_only: [11]u8 = [0xFF; 11];
    const empty_opt = SceneStreamManager.buildInfoEnvelope("", hdr_only[0..]);
    check("info envelope accepts empty payload", empty_opt != null);
    if (env := empty_opt) {
        check("empty info envelope len is 11", env.len == 11);
        check("empty info byte 0 is MSG_INFO", env[0] == MSG_INFO);
    }
}

fn testSceneEnvelope() {
    std.debug.print("2. buildSceneEnvelope 11B header, little-endian seq/width/height & truncation\n", .{});
    const payload = "SCENE_DATA";
    let short_buf: [20]u8 = [0; 20];
    // 11 + 10 = 21 > 20
    check("scene envelope rejects undersized buffer", SceneStreamManager.buildSceneEnvelope(MSG_SCENE, 1, 800, 600, payload, short_buf[0..]) == null);

    let out_buf: [64]u8 = [0xFF; 64];
    const seq: u32 = 0x12345678;
    const width: u32 = 1280; // 0x0500
    const height: u32 = 720; // 0x02D0
    const res_opt = SceneStreamManager.buildSceneEnvelope(MSG_SCENE, seq, width, height, payload, out_buf[0..]);
    check("scene envelope succeeds", res_opt != null);
    if (env := res_opt) {
        check("scene envelope len", env.len == 11 + payload.len);
        check("byte 0 is MSG_SCENE", env[0] == MSG_SCENE);
        check("byte 1 is CODEC_SCENE", env[1] == CODEC_SCENE);
        check("byte 2 is 0", env[2] == 0);

        // Little-endian seq 0x12345678 -> 78 56 34 12
        check("seq byte 0", env[3] == 0x78);
        check("seq byte 1", env[4] == 0x56);
        check("seq byte 2", env[5] == 0x34);
        check("seq byte 3", env[6] == 0x12);

        // Little-endian width 1280 (0x0500) -> 00 05
        check("width byte 0", env[7] == 0x00);
        check("width byte 1", env[8] == 0x05);

        // Little-endian height 720 (0x02D0) -> D0 02
        check("height byte 0", env[9] == 0xD0);
        check("height byte 1", env[10] == 0x02);

        check("scene payload matches", std.mem.eql(u8, env[11..], payload));
    }

    // MSG_PARTIAL + u16 truncation check (0x00010320 -> 0x0320 = 800)
    const partial_opt = SceneStreamManager.buildSceneEnvelope(MSG_PARTIAL, 0x0000002A, 0x00010320, 0x00020258, "", out_buf[0..]);
    check("partial envelope succeeds", partial_opt != null);
    if (env := partial_opt) {
        check("partial len is 11", env.len == 11);
        check("byte 0 is MSG_PARTIAL", env[0] == MSG_PARTIAL);
        check("seq is 42", std.mem.readInt(u32, env[3..7], .little) == 42);
        check("truncated width is 0x0320 (800)", std.mem.readInt(u16, env[7..9], .little) == 800);
        check("truncated height is 0x0258 (600)", std.mem.readInt(u16, env[9..11], .little) == 600);
    }
}

fn testInFlightLimitAndAck() {
    std.debug.print("3. MAX_SCENES_IN_FLIGHT limit & ackFrame release\n", .{});
    check("MAX_SCENES_IN_FLIGHT is 2", MAX_SCENES_IN_FLIGHT == 2);

    let mgr = SceneStreamManager{};
    check("ackFrame returns false with no active streams", !mgr.ackFrame("missing", 1, true));

    const started = mgr.startStream(SceneStreamConfig{
        target_id: "tab-1",
        width: 1024,
        height: 768,
        fps: 60,
    }, "sess-a", 1000);
    check("startStream succeeded", started);

    const stream_opt = mgr.getStream("tab-1");
    check("stream found", stream_opt != null);
    if (*s := stream_opt) {
        check("initially canSendFrame", s.canSendFrame());
        check("initial in_flight is 0", s.in_flight == 0);

        s.recordSentFrame(0x1111, 1024, 768, 0, 0, 0, 1016);
        check("in_flight is 1", s.in_flight == 1);
        check("canSendFrame when in_flight == 1", s.canSendFrame());

        s.recordSentFrame(0x2222, 1024, 768, 0, 0, 0, 1032);
        check("in_flight is 2", s.in_flight == 2);
        check("cannot send frame at MAX_SCENES_IN_FLIGHT", !s.canSendFrame());
    }

    // ACK seq 5 -> releases 1 in-flight slot
    check("ack seq 5", mgr.ackFrame("tab-1", 5, true));
    if (*s := stream_opt) {
        check("in_flight decremented to 1", s.in_flight == 1);
        check("canSendFrame restored after ack", s.canSendFrame());
        check("last_acked_seq updated to 5", s.last_acked_seq == 5);
    }

    // Out-of-order older seq 3 -> still decrements in_flight, does not regress last_acked_seq
    check("ack older seq 3", mgr.ackFrame("tab-1", 3, true));
    if (*s := stream_opt) {
        check("in_flight decremented to 0", s.in_flight == 0);
        check("last_acked_seq remains 5", s.last_acked_seq == 5);
    }

    // ACK when in_flight == 0 with painted=false -> no underflow, sets need_full_frame=true
    check("ack via primary fallback with painted=false", mgr.ackFrame("unknown-target", 6, false));
    if (*s := stream_opt) {
        check("in_flight stays 0 (no underflow)", s.in_flight == 0);
        check("last_acked_seq updated to 6", s.last_acked_seq == 6);
        check("need_full_frame set to true on painted=false", s.need_full_frame);
    }
}

fn testIdleFrameDedup() {
    std.debug.print("4. shouldSkipIdleFrame deduplication\n", .{});
    let mgr = SceneStreamManager{};
    * = mgr.startStream(SceneStreamConfig{
        target_id: "tab-idle",
        width: 800,
        height: 600,
        fps: 60,
    }, "sess-b", 2000);

    const stream_opt = mgr.getStream("tab-idle");
    check("idle stream found", stream_opt != null);
    if (*s := stream_opt) {
        // Before any frame sent: has_last_frame=false, need_full_frame=true
        check("does not skip before first frame", !s.shouldSkipIdleFrame(0xABCD, 800, 600, 0, 0, 1));

        s.recordSentFrame(0xABCD, 800, 600, 10, 20, 7, 2016);
        check("skips identical frame after recordSentFrame", s.shouldSkipIdleFrame(0xABCD, 800, 600, 10, 20, 7));

        // Any single dimension/state change must not skip
        check("does not skip on hash change", !s.shouldSkipIdleFrame(0xABCE, 800, 600, 10, 20, 7));
        check("does not skip on viewport width change", !s.shouldSkipIdleFrame(0xABCD, 801, 600, 10, 20, 7));
        check("does not skip on viewport height change", !s.shouldSkipIdleFrame(0xABCD, 800, 601, 10, 20, 7));
        check("does not skip on scroll x change", !s.shouldSkipIdleFrame(0xABCD, 800, 600, 11, 20, 7));
        check("does not skip on scroll y change", !s.shouldSkipIdleFrame(0xABCD, 800, 600, 10, 21, 7));
        check("does not skip on focused node change", !s.shouldSkipIdleFrame(0xABCD, 800, 600, 10, 20, 8));
    }

    // Client reports painted=false -> need_full_frame=true -> must not skip even with identical state
    check("ack painted=false", mgr.ackFrame("tab-idle", 1, false));
    if (*s := stream_opt) {
        check("does not skip when need_full_frame is true", !s.shouldSkipIdleFrame(0xABCD, 800, 600, 10, 20, 7));
    }
}

pub fn main() {
    std.debug.print("\n=== Phase 242 Slot A: Scene Stream Envelope & Edge Tests ===\n", .{});
    testInfoEnvelope();
    testSceneEnvelope();
    testInFlightLimitAndAck();
    testIdleFrameDedup();
    std.debug.print("\nAll Scene Stream Envelope & Edge tests passed!\n", .{});
}
