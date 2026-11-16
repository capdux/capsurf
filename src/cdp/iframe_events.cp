// Migrated from vendor/kitesurf/packages/engine/src/cdp/iframe-events.ts
// CapSurf Child Frame Lifecycle Events Generator
// Emits Page.frameAttached, Page.frameStartedLoading, Page.frameNavigated, and Page.lifecycleEvent(init).

import { CdpEvent } from "./types.cp";

pub const MILLISECONDS_PER_SECOND: f64 = 1000.0;

pub struct ChildFrameInfo {
    frame_id: []const u8 = "",
    parent_frame_id: []const u8 = "",
    loader_id: []const u8 = "",
    url: []const u8 = "about:blank",
}

pub struct ChildFrameEventStrings {
    frame_attached: []const u8 = "",
    frame_started_loading: []const u8 = "",
    frame_navigated: []const u8 = "",
    lifecycle_event: []const u8 = "",
}

pub fn formatFrameAttached(frame: ChildFrameInfo, out_buf: []u8): []const u8 {
    return std.fmt.bufPrint(
        out_buf,
        "{{\"frameId\":\"{s}\",\"parentFrameId\":\"{s}\"}}",
        .{ frame.frame_id, frame.parent_frame_id },
    ) catch "";
}

pub fn formatFrameStartedLoading(frame: ChildFrameInfo, out_buf: []u8): []const u8 {
    return std.fmt.bufPrint(
        out_buf,
        "{{\"frameId\":\"{s}\"}}",
        .{ frame.frame_id },
    ) catch "";
}

pub fn formatFrameNavigated(frame: ChildFrameInfo, out_buf: []u8): []const u8 {
    return std.fmt.bufPrint(
        out_buf,
        "{{\"frame\":{{\"id\":\"{s}\",\"parentId\":\"{s}\",\"loaderId\":\"{s}\",\"url\":\"{s}\",\"securityOrigin\":\"{s}\",\"mimeType\":\"text/html\"}},\"type\":\"Navigation\"}}",
        .{ frame.frame_id, frame.parent_frame_id, frame.loader_id, frame.url, frame.url },
    ) catch "";
}

pub fn formatLifecycleEvent(frame: ChildFrameInfo, timestamp: f64, out_buf: []u8): []const u8 {
    return std.fmt.bufPrint(
        out_buf,
        "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"init\",\"timestamp\":{d:.3}}}",
        .{ frame.frame_id, frame.loader_id, timestamp },
    ) catch "";
}

pub fn formatChildFrameEvents(frame: ChildFrameInfo, timestamp: f64, out_buf: []u8): []const u8 {
    return std.fmt.bufPrint(
        out_buf,
        "[{{\"method\":\"Page.frameAttached\",\"params\":{{\"frameId\":\"{s}\",\"parentFrameId\":\"{s}\"}}}},{{\"method\":\"Page.frameStartedLoading\",\"params\":{{\"frameId\":\"{s}\"}}}},{{\"method\":\"Page.frameNavigated\",\"params\":{{\"frame\":{{\"id\":\"{s}\",\"parentId\":\"{s}\",\"loaderId\":\"{s}\",\"url\":\"{s}\",\"securityOrigin\":\"{s}\",\"mimeType\":\"text/html\"}},\"type\":\"Navigation\"}}}},{{\"method\":\"Page.lifecycleEvent\",\"params\":{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"init\",\"timestamp\":{d:.3}}}}}]",
        .{
            frame.frame_id, frame.parent_frame_id,
            frame.frame_id,
            frame.frame_id, frame.parent_frame_id, frame.loader_id, frame.url, frame.url,
            frame.frame_id, frame.loader_id, timestamp,
        },
    ) catch "";
}

pub fn emitChildFrameAttachAndStart(
    frame: ChildFrameInfo,
    timestamp: f64,
    out_buf: []u8,
): ChildFrameEventStrings {
    let res = ChildFrameEventStrings{};
    let offset: usize = 0;

    if (offset < out_buf.len) {
        const s = formatFrameAttached(frame, out_buf[offset..]);
        res.frame_attached = s;
        offset += s.len;
    }

    if (offset < out_buf.len) {
        const s = formatFrameStartedLoading(frame, out_buf[offset..]);
        res.frame_started_loading = s;
        offset += s.len;
    }

    if (offset < out_buf.len) {
        const s = formatFrameNavigated(frame, out_buf[offset..]);
        res.frame_navigated = s;
        offset += s.len;
    }

    if (offset < out_buf.len) {
        const s = formatLifecycleEvent(frame, timestamp, out_buf[offset..]);
        res.lifecycle_event = s;
        offset += s.len;
    }

    return res;
}

pub fn emitChildFrameEvents(
    frames: []const ChildFrameInfo,
    timestamp: f64,
    session_id: []const u8,
    out_events: []CdpEvent,
    event_count: *usize,
    json_buf: []u8,
) {
    event_count.* = 0;
    let offset: usize = 0;
    let idx: usize = 0;

    while (idx < frames.len) : (idx += 1) {
        const f = frames[idx];
        if (f.parent_frame_id.len == 0) {
            continue;
        }

        if (event_count.* < out_events.len && offset < json_buf.len) {
            const p = formatFrameAttached(f, json_buf[offset..]);
            if (p.len > 0) {
                out_events[event_count.*] = CdpEvent{
                    method: "Page.frameAttached",
                    params_json: p,
                    session_id: session_id,
                };
                event_count.* += 1;
                offset += p.len;
            }
        }

        if (event_count.* < out_events.len && offset < json_buf.len) {
            const p = formatFrameStartedLoading(f, json_buf[offset..]);
            if (p.len > 0) {
                out_events[event_count.*] = CdpEvent{
                    method: "Page.frameStartedLoading",
                    params_json: p,
                    session_id: session_id,
                };
                event_count.* += 1;
                offset += p.len;
            }
        }

        if (event_count.* < out_events.len && offset < json_buf.len) {
            const p = formatFrameNavigated(f, json_buf[offset..]);
            if (p.len > 0) {
                out_events[event_count.*] = CdpEvent{
                    method: "Page.frameNavigated",
                    params_json: p,
                    session_id: session_id,
                };
                event_count.* += 1;
                offset += p.len;
            }
        }

        if (event_count.* < out_events.len && offset < json_buf.len) {
            const p = formatLifecycleEvent(f, timestamp, json_buf[offset..]);
            if (p.len > 0) {
                out_events[event_count.*] = CdpEvent{
                    method: "Page.lifecycleEvent",
                    params_json: p,
                    session_id: session_id,
                };
                event_count.* += 1;
                offset += p.len;
            }
        }
    }
}

pub struct ChildFrameEventEmitter {
    buf: []u8,
    offset: usize = 0,

    static fn init(buf: []u8): ChildFrameEventEmitter {
        return ChildFrameEventEmitter{
            buf: buf,
            offset: 0,
        };
    }

    fn reset(*_) {
        _.offset = 0;
    }

    fn emitFrameAttached(*_, frame: ChildFrameInfo): []const u8 {
        if (_.offset >= _.buf.len) {
            return "";
        }
        const s = formatFrameAttached(frame, _.buf[_.offset..]);
        _.offset += s.len;
        return s;
    }

    fn emitFrameStartedLoading(*_, frame: ChildFrameInfo): []const u8 {
        if (_.offset >= _.buf.len) {
            return "";
        }
        const s = formatFrameStartedLoading(frame, _.buf[_.offset..]);
        _.offset += s.len;
        return s;
    }

    fn emitFrameNavigated(*_, frame: ChildFrameInfo): []const u8 {
        if (_.offset >= _.buf.len) {
            return "";
        }
        const s = formatFrameNavigated(frame, _.buf[_.offset..]);
        _.offset += s.len;
        return s;
    }

    fn emitLifecycleEvent(*_, frame: ChildFrameInfo, timestamp: f64): []const u8 {
        if (_.offset >= _.buf.len) {
            return "";
        }
        const s = formatLifecycleEvent(frame, timestamp, _.buf[_.offset..]);
        _.offset += s.len;
        return s;
    }

    fn emitAll(*_, frame: ChildFrameInfo, timestamp: f64): ChildFrameEventStrings {
        return ChildFrameEventStrings{
            frame_attached: _.emitFrameAttached(frame),
            frame_started_loading: _.emitFrameStartedLoading(frame),
            frame_navigated: _.emitFrameNavigated(frame),
            lifecycle_event: _.emitLifecycleEvent(frame, timestamp),
        };
    }
}
