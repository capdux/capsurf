// Migrated from vendor/kitesurf/packages/engine/src/cdp/scene-stream.ts
// Manages RBI scene streams, frame acknowledgements, resources, and terminal stream flow.

pub const MIN_SCENE_STREAM_WIDTH: u32 = 320;
pub const MIN_SCENE_STREAM_HEIGHT: u32 = 240;
pub const MAX_SCENE_STREAM_HEIGHT: u32 = 1440;
pub const MAX_SCENE_STREAM_FPS: u32 = 60;
pub const DEFAULT_SCENE_STREAM_FPS: u32 = 60;
pub const MAX_SCENE_FRAME_SEQUENCE: u32 = 0xffff_ffff;
pub const MAX_SCENE_RESOURCE_REQUEST_ITEMS: usize = 4096;
pub const MAX_IN_FLIGHT_RESOURCE_REQUESTS: usize = 2;
pub const MAX_STREAMS: usize = 16;
pub const MAX_SCENES_IN_FLIGHT: u32 = 2;
pub const SCENE_HEARTBEAT_MS: i64 = 1000;
pub const MSG_INFO: u8 = 2;
pub const MSG_SCENE: u8 = 3;
pub const MSG_PARTIAL: u8 = 5;
pub const CODEC_SCENE: u8 = 2;
pub const SCENE_ENVELOPE_HEADER_BYTES: usize = 11;

pub struct SceneStreamConfig {
    target_id: []const u8 = "",
    width: u32 = 800,
    height: u32 = 600,
    fps: u32 = 60,
    text: bool = false,
    terminal: bool = false,
    cell_w: u32 = 8,
    cell_h: u32 = 17,
    allow_partial: bool = false,
}

pub struct SceneStreamState {
    config: SceneStreamConfig = SceneStreamConfig{},
    active: bool = false,
    owner_session_buf: [128]u8 = [0; 128],
    owner_session_len: usize = 0,
    min_frame_interval_ms: u32 = 16,
    current_seq: u32 = 0,
    last_acked_seq: u32 = 0,
    in_flight: u32 = 0,
    need_full_frame: bool = true,
    in_flight_resources: usize = 0,
    frames_sent: u32 = 0,
    last_frame_sent_ms: i64 = 0,
    last_activity_ms: i64 = 0,
    has_last_frame: bool = false,
    last_frame_hash: u64 = 0,
    last_viewport_w: u32 = 0,
    last_viewport_h: u32 = 0,
    last_scroll_x: i64 = 0,
    last_scroll_y: i64 = 0,
    last_focused_node: u32 = 0,

    fn getOwnerSession(*_): []const u8 {
        return _.owner_session_buf[0.._.owner_session_len];
    }

    fn canSendFrame(): bool {
        return _.active && _.in_flight < MAX_SCENES_IN_FLIGHT;
    }

    fn isIntervalElapsed(now_ms: i64): bool {
        if (_.last_frame_sent_ms == 0) return true;
        return (now_ms - _.last_frame_sent_ms) >= @as(i64, @intCast(_.min_frame_interval_ms));
    }

    fn shouldSendHeartbeat(now_ms: i64): bool {
        if (!_.active || _.last_activity_ms == 0) return false;
        return (now_ms - _.last_activity_ms) >= SCENE_HEARTBEAT_MS;
    }

    fn shouldSkipIdleFrame(
        frame_hash: u64,
        vw: u32,
        vh: u32,
        sx: i64,
        sy: i64,
        focused_node: u32,
    ): bool {
        return _.has_last_frame &&
            !_.need_full_frame &&
            _.last_frame_hash == frame_hash &&
            _.last_viewport_w == vw &&
            _.last_viewport_h == vh &&
            _.last_scroll_x == sx &&
            _.last_scroll_y == sy &&
            _.last_focused_node == focused_node;
    }

    fn recordSentFrame(
        *_,
        frame_hash: u64,
        vw: u32,
        vh: u32,
        sx: i64,
        sy: i64,
        focused_node: u32,
        now_ms: i64,
    ) {
        _.has_last_frame = true;
        _.need_full_frame = false;
        _.last_frame_hash = frame_hash;
        _.last_viewport_w = vw;
        _.last_viewport_h = vh;
        _.last_scroll_x = sx;
        _.last_scroll_y = sy;
        _.last_focused_node = focused_node;
        _.last_frame_sent_ms = now_ms;
        _.last_activity_ms = now_ms;
        _.in_flight += 1;
        _.frames_sent += 1;
    }
}

pub struct SceneStreamManager {
    streams: [MAX_STREAMS]SceneStreamState = [SceneStreamState{}; 16],
    active_count: usize = 0,
    global_seq: u32 = 1,

    fn findSlot(*_, target_id: []const u8): usize? {
        let i: usize = 0;
        while (i < MAX_STREAMS) : (i += 1) {
            if (_.streams[i].active && std.mem.eql(u8, _.streams[i].config.target_id, target_id)) {
                return i;
            }
        }
        return null;
    }

    fn startStream(*_, config: SceneStreamConfig, owner_session: []const u8, now_ms: i64): bool {
        let slot_opt = _.findSlot(config.target_id);
        let slot_idx: usize = 0;
        if (s := slot_opt) {
            slot_idx = s;
        } else {
            let free_opt: usize? = null;
            let i: usize = 0;
            while (i < MAX_STREAMS) : (i += 1) {
                if (!_.streams[i].active) {
                    free_opt = i;
                    break;
                }
            }
            if (f := free_opt) {
                slot_idx = f;
                _.active_count += 1;
            } else {
                return false; // full
            }
        }

        let w = if (config.width < MIN_SCENE_STREAM_WIDTH) MIN_SCENE_STREAM_WIDTH else config.width;
        let h = if (config.height < MIN_SCENE_STREAM_HEIGHT) MIN_SCENE_STREAM_HEIGHT else config.height;
        if (h > MAX_SCENE_STREAM_HEIGHT) h = MAX_SCENE_STREAM_HEIGHT;
        let fps = if (config.fps == 0 || config.fps > MAX_SCENE_STREAM_FPS) DEFAULT_SCENE_STREAM_FPS else config.fps;
        let interval_ms: u32 = if (fps >= 60) 16 else (@as(u32, 1000) / fps);

        let final_cfg = SceneStreamConfig{
            target_id: config.target_id,
            width: w,
            height: h,
            fps: fps,
            text: config.text,
            terminal: config.terminal,
            cell_w: config.cell_w,
            cell_h: config.cell_h,
            allow_partial: config.allow_partial,
        };

        _.streams[slot_idx] = SceneStreamState{
            config: final_cfg,
            active: true,
            min_frame_interval_ms: interval_ms,
            current_seq: _.global_seq,
            last_acked_seq: 0,
            in_flight: 0,
            need_full_frame: true,
            in_flight_resources: 0,
            frames_sent: 0,
            last_frame_sent_ms: 0,
            last_activity_ms: now_ms,
            has_last_frame: false,
            last_frame_hash: 0,
            last_viewport_w: 0,
            last_viewport_h: 0,
            last_scroll_x: 0,
            last_scroll_y: 0,
            last_focused_node: 0,
        };
        const olen = @min(owner_session.len, _.streams[slot_idx].owner_session_buf.len);
        if (olen > 0) {
            @memcpy(_.streams[slot_idx].owner_session_buf[0..olen], owner_session[0..olen]);
        }
        _.streams[slot_idx].owner_session_len = olen;
        return true;
    }

    fn stopStream(*_, target_id: []const u8): bool {
        let slot_opt = _.findSlot(target_id);
        if (slot_opt == null) {
            let i: usize = 0;
            let stopped_any = false;
            while (i < MAX_STREAMS) : (i += 1) {
                if (_.streams[i].active) {
                    _.streams[i].active = false;
                    if (_.active_count > 0) _.active_count -= 1;
                    stopped_any = true;
                }
            }
            return stopped_any;
        }
        let slot_idx = slot_opt.?;

        _.streams[slot_idx].active = false;
        if (_.active_count > 0) {
            _.active_count -= 1;
        }
        return true;
    }

    fn ackFrame(*_, target_id: []const u8, seq: u32, painted: bool): bool {
        let stream_opt = _.getStream(target_id);
        if (stream_opt == null) {
            stream_opt = _.getPrimaryStream();
        }
        if (stream_opt == null) return false;

        let stream = stream_opt.?;
        if (seq > stream.last_acked_seq) {
            stream.last_acked_seq = seq;
        }
        if (stream.in_flight > 0) {
            stream.in_flight -= 1;
        }
        if (!painted) {
            stream.need_full_frame = true;
        }
        return true;
    }

    fn nextFrameSeq(*_, target_id: []const u8): u32 {
        let seq = _.global_seq;
        _.global_seq += 1;
        if (_.global_seq == 0 || _.global_seq >= MAX_SCENE_FRAME_SEQUENCE) {
            _.global_seq = 1;
        }
        if (stream := _.getStream(target_id)) {
            stream.current_seq = seq;
        } else if (stream := _.getPrimaryStream()) {
            stream.current_seq = seq;
        }
        return seq;
    }

    fn getStream(*_, target_id: []const u8): (*SceneStreamState)? {
        let slot_opt = _.findSlot(target_id);
        if (slot_opt == null) return null;
        return &_.streams[slot_opt.?];
    }

    fn getPrimaryStream(*_): (*SceneStreamState)? {
        let i: usize = 0;
        while (i < MAX_STREAMS) : (i += 1) {
            if (_.streams[i].active) {
                return &_.streams[i];
            }
        }
        return null;
    }

    fn isActive(): bool {
        return _.active_count > 0;
    }

    fn isTerminalActive(): bool {
        let i: usize = 0;
        while (i < MAX_STREAMS) : (i += 1) {
            if (_.streams[i].active && _.streams[i].config.terminal) {
                return true;
            }
        }
        return false;
    }

    static fn buildInfoEnvelope(json_payload: []const u8, out_buf: []u8): ([]const u8)? {
        const total = SCENE_ENVELOPE_HEADER_BYTES + json_payload.len;
        if (total > out_buf.len) return null;
        @memset(out_buf[0..SCENE_ENVELOPE_HEADER_BYTES], 0);
        out_buf[0] = MSG_INFO;
        if (json_payload.len > 0) {
            @memcpy(out_buf[SCENE_ENVELOPE_HEADER_BYTES..total], json_payload);
        }
        return out_buf[0..total];
    }

    static fn buildSceneEnvelope(
        msg_type: u8,
        seq: u32,
        width: u32,
        height: u32,
        payload: []const u8,
        out_buf: []u8,
    ): ([]const u8)? {
        const total = SCENE_ENVELOPE_HEADER_BYTES + payload.len;
        if (total > out_buf.len) return null;
        out_buf[0] = msg_type;
        out_buf[1] = CODEC_SCENE;
        out_buf[2] = 0;
        std.mem.writeInt(u32, out_buf[3..7], seq, .little);
        std.mem.writeInt(u16, out_buf[7..9], @as(u16, @truncate(width)), .little);
        std.mem.writeInt(u16, out_buf[9..11], @as(u16, @truncate(height)), .little);
        if (payload.len > 0) {
            @memcpy(out_buf[SCENE_ENVELOPE_HEADER_BYTES..total], payload);
        }
        return out_buf[0..total];
    }
}
