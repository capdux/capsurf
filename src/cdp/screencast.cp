// Migrated from vendor/kitesurf/packages/engine/src/cdp/page.ts
// Screencast session protocol handler and continuous visual streaming for CapSurf

import { extractJsonString, extractJsonInt } from "./types.cp";

pub const DEFAULT_SCREENCAST_FORMAT: []const u8 = "jpeg";
pub const DEFAULT_SCREENCAST_QUALITY: u8 = 80;
pub const DEFAULT_SCREENCAST_MAX_WIDTH: u32 = 1280;
pub const DEFAULT_SCREENCAST_MAX_HEIGHT: u32 = 720;
pub const DEFAULT_SCREENCAST_EVERY_NTH_FRAME: u32 = 1;

pub const MIN_SCREENCAST_QUALITY: u8 = 1;
pub const MAX_SCREENCAST_QUALITY: u8 = 100;
pub const MIN_SCREENCAST_DIMENSION: u32 = 1;
pub const MAX_SCREENCAST_DIMENSION: u32 = 8192;
pub const MAX_SCREENCAST_EVERY_NTH_FRAME: u32 = 100;

pub struct ScreencastState {
    active: bool = false,
    format: []const u8 = "jpeg",
    quality: u8 = 80,
    max_width: u32 = 1280,
    max_height: u32 = 720,
    every_nth_frame: u32 = 1,
    frame_seq: u32 = 0,
    last_acked_seq: u32 = 0,
    pending_ack_session_id: u32 = 0,
    owner_session_id: []const u8 = "",

    fn reset(*_) {
        _.active = false;
        _.format = "jpeg";
        _.quality = 80;
        _.max_width = 1280;
        _.max_height = 720;
        _.every_nth_frame = 1;
        _.frame_seq = 0;
        _.last_acked_seq = 0;
        _.pending_ack_session_id = 0;
        _.owner_session_id = "";
    }

    fn inFlight(): u32 {
        if (_.frame_seq >= _.last_acked_seq) {
            return _.frame_seq - _.last_acked_seq;
        }
        return 0;
    }

    fn hasPendingAck(): bool {
        return _.pending_ack_session_id != 0;
    }
}

pub struct ScreencastFrameMetadata {
    offset_top: f32 = 0.0,
    page_scale_factor: f32 = 1.0,
    device_width: u32 = 1280,
    device_height: u32 = 720,
    scroll_offset_x: f32 = 0.0,
    scroll_offset_y: f32 = 0.0,
    timestamp: f64 = 0.0,
}

pub struct ScreencastHandler {
    state: ScreencastState = ScreencastState{},
    max_in_flight: u32 = 1,

    static fn init(): ScreencastHandler {
        return ScreencastHandler{
            state: ScreencastState{},
            max_in_flight: 1,
        };
    }

    fn getState(): ScreencastState {
        return _.state;
    }

    fn isActive(): bool {
        return _.state.active;
    }

    fn getOwnerSession(): []const u8 {
        return _.state.owner_session_id;
    }

    fn canSendFrame(): bool {
        return _.state.active && _.state.inFlight() < _.max_in_flight;
    }

    fn startScreencast(*_, req_params_json: []const u8, owner_session: []const u8): bool {
        let fmt: []const u8 = "jpeg";
        if (f := extractJsonString(req_params_json, "format")) {
            if (std.mem.eql(u8, f, "png")) {
                fmt = "png";
            } else {
                fmt = "jpeg";
            }
        }

        let quality: u8 = DEFAULT_SCREENCAST_QUALITY;
        if (q := extractJsonInt(req_params_json, "quality")) {
            if (q < @as(i64, MIN_SCREENCAST_QUALITY)) {
                quality = MIN_SCREENCAST_QUALITY;
            } else if (q > @as(i64, MAX_SCREENCAST_QUALITY)) {
                quality = MAX_SCREENCAST_QUALITY;
            } else {
                quality = @as(u8, @intCast(q));
            }
        }

        let max_w: u32 = DEFAULT_SCREENCAST_MAX_WIDTH;
        if (w := extractJsonInt(req_params_json, "maxWidth")) {
            if (w >= @as(i64, MIN_SCREENCAST_DIMENSION)) {
                if (w > @as(i64, MAX_SCREENCAST_DIMENSION)) {
                    max_w = MAX_SCREENCAST_DIMENSION;
                } else {
                    max_w = @as(u32, @intCast(w));
                }
            }
        }

        let max_h: u32 = DEFAULT_SCREENCAST_MAX_HEIGHT;
        if (h := extractJsonInt(req_params_json, "maxHeight")) {
            if (h >= @as(i64, MIN_SCREENCAST_DIMENSION)) {
                if (h > @as(i64, MAX_SCREENCAST_DIMENSION)) {
                    max_h = MAX_SCREENCAST_DIMENSION;
                } else {
                    max_h = @as(u32, @intCast(h));
                }
            }
        }

        let every_nth: u32 = DEFAULT_SCREENCAST_EVERY_NTH_FRAME;
        if (n := extractJsonInt(req_params_json, "everyNthFrame")) {
            if (n >= 1) {
                if (n > @as(i64, MAX_SCREENCAST_EVERY_NTH_FRAME)) {
                    every_nth = MAX_SCREENCAST_EVERY_NTH_FRAME;
                } else {
                    every_nth = @as(u32, @intCast(n));
                }
            }
        }

        _.max_in_flight = 1;
        if (m := extractJsonInt(req_params_json, "maxFramesInFlight")) {
            if (m >= 1 && m <= 16) {
                _.max_in_flight = @as(u32, @intCast(m));
            }
        }
        if (m2 := extractJsonInt(req_params_json, "maxInFlight")) {
            if (m2 >= 1 && m2 <= 16) {
                _.max_in_flight = @as(u32, @intCast(m2));
            }
        }

        _.state.active = true;
        _.state.format = fmt;
        _.state.quality = quality;
        _.state.max_width = max_w;
        _.state.max_height = max_h;
        _.state.every_nth_frame = every_nth;
        _.state.owner_session_id = owner_session;
        _.state.frame_seq = 0;
        _.state.last_acked_seq = 0;
        _.state.pending_ack_session_id = 0;

        return true;
    }

    fn stopScreencast(*_): bool {
        if (!_.state.active) return false;
        _.state.active = false;
        _.state.pending_ack_session_id = 0;
        return true;
    }

    fn screencastFrameAck(*_, ack_session_id: u32): bool {
        if (!_.state.active) return false;
        if (ack_session_id > _.state.last_acked_seq) {
            _.state.last_acked_seq = ack_session_id;
        }
        if (_.state.pending_ack_session_id != 0 && ack_session_id >= _.state.pending_ack_session_id) {
            _.state.pending_ack_session_id = 0;
        }
        return true;
    }

    fn nextFrameSeq(*_): u32 {
        _.state.frame_seq += 1;
        if (_.state.pending_ack_session_id == 0) {
            _.state.pending_ack_session_id = _.state.frame_seq;
        }
        return _.state.frame_seq;
    }

    static fn formatScreencastFrameEvent(
        frame_seq: u32,
        base64_data: []const u8,
        width: u32,
        height: u32,
        timestamp_sec: f64,
        out_buf: []u8,
    ): ([]const u8)? {
        return std.fmt.bufPrint(
            out_buf,
            "{{\"method\":\"Page.screencastFrame\",\"params\":{{\"data\":\"{s}\",\"metadata\":{{\"offsetTop\":0,\"pageScaleFactor\":1,\"deviceWidth\":{d},\"deviceHeight\":{d},\"scrollOffsetX\":0,\"scrollOffsetY\":0,\"timestamp\":{d}}},\"sessionId\":{d}}}}}",
            .{ base64_data, width, height, timestamp_sec, frame_seq },
        ) catch null;
    }

    fn formatCurrentFrameEvent(
        *_,
        base64_data: []const u8,
        timestamp_sec: f64,
        out_buf: []u8,
    ): ([]const u8)? {
        const seq = _.nextFrameSeq();
        return ScreencastHandler.formatScreencastFrameEvent(seq, base64_data, _.state.max_width, _.state.max_height, timestamp_sec, out_buf);
    }
}

pub fn formatScreencastFrameEvent(
    frame_seq: u32,
    base64_data: []const u8,
    width: u32,
    height: u32,
    timestamp_sec: f64,
    out_buf: []u8,
): ([]const u8)? {
    return ScreencastHandler.formatScreencastFrameEvent(frame_seq, base64_data, width, height, timestamp_sec, out_buf);
}

pub fn formatScreencastFrameParams(
    frame_seq: u32,
    base64_data: []const u8,
    width: u32,
    height: u32,
    timestamp_sec: f64,
    out_buf: []u8,
): ([]const u8)? {
    return std.fmt.bufPrint(
        out_buf,
        "{{\"data\":\"{s}\",\"metadata\":{{\"offsetTop\":0,\"pageScaleFactor\":1,\"deviceWidth\":{d},\"deviceHeight\":{d},\"scrollOffsetX\":0,\"scrollOffsetY\":0,\"timestamp\":{d}}},\"sessionId\":{d}}}",
        .{ base64_data, width, height, timestamp_sec, frame_seq },
    ) catch null;
}

pub fn formatScreencastVisibilityChangedEvent(visible: bool, out_buf: []u8): ([]const u8)? {
    return std.fmt.bufPrint(
        out_buf,
        "{{\"method\":\"Page.screencastVisibilityChanged\",\"params\":{{\"visible\":{s}}}}}",
        .{if (visible) "true" else "false"},
    ) catch null;
}

pub fn extractAckSessionId(params_json: []const u8): (u32)? {
    if (sid := extractJsonInt(params_json, "sessionId")) {
        if (sid >= 0) {
            return @as(u32, @intCast(sid));
        }
    }
    return null;
}
