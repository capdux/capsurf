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
    current_seq: u32 = 0,
    last_acked_seq: u32 = 0,
    need_full_frame: bool = true,
    in_flight_resources: usize = 0,
    frames_sent: u32 = 0,
}

pub struct SceneStreamManager {
    streams: [MAX_STREAMS]SceneStreamState = [SceneStreamState{}; 16],
    active_count: usize = 0,

    fn findSlot(*_, target_id: []const u8): usize? {
        let i: usize = 0;
        while (i < MAX_STREAMS) : (i += 1) {
            if (_.streams[i].active && std.mem.eql(u8, _.streams[i].config.target_id, target_id)) {
                return i;
            }
        }
        return null;
    }

    fn startStream(*_, config: SceneStreamConfig): bool {
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
            current_seq: 1,
            last_acked_seq: 0,
            need_full_frame: true,
            in_flight_resources: 0,
            frames_sent: 0,
        };
        return true;
    }

    fn stopStream(*_, target_id: []const u8): bool {
        let slot_opt = _.findSlot(target_id);
        if (slot_opt == null) return false;
        let slot_idx = slot_opt.?;

        _.streams[slot_idx].active = false;
        if (_.active_count > 0) {
            _.active_count -= 1;
        }
        return true;
    }

    fn ackFrame(*_, target_id: []const u8, seq: u32, painted: bool): bool {
        let slot_opt = _.findSlot(target_id);
        if (slot_opt == null) return false;
        let slot_idx = slot_opt.?;

        let stream = &_.streams[slot_idx];
        stream.last_acked_seq = seq;
        if (!painted) {
            stream.need_full_frame = true;
        }
        return true;
    }

    fn nextFrameSeq(*_, target_id: []const u8): u32 {
        let slot_opt = _.findSlot(target_id);
        if (slot_opt == null) return 0;
        let slot_idx = slot_opt.?;

        let stream = &_.streams[slot_idx];
        let seq = stream.current_seq;
        stream.current_seq = (stream.current_seq + 1) % MAX_SCENE_FRAME_SEQUENCE;
        stream.frames_sent += 1;
        return seq;
    }

    fn getStream(*_, target_id: []const u8): (*SceneStreamState)? {
        let slot_opt = _.findSlot(target_id);
        if (slot_opt == null) return null;
        return &_.streams[slot_opt.?];
    }
}
