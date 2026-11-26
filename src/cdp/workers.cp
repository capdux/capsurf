// src/cdp/workers.cp
// Dedicated Worker Lifecycle & CDP Target Multiplexing for CapSurf
// Clean-room CP implementation with explicit state machine and per-item heap buffers.

import { CdpEvent } from "./types.cp";
import { TargetRegistry } from "./target.cp";

pub const MAX_DEDICATED_WORKERS: usize = 8;
pub const MAX_WORKER_MSG_QUEUE: usize = 32;
pub const MAX_WORKER_CDP_EVENTS: usize = 64;

pub enum WorkerTargetState {
    idle,
    created,
    attached,
    closing,
    detached,
}

pub struct WorkerMsgPacket {
    data: []u8 = [],

    fn freeData(*_) {
        if (_.data.len > 0) {
            std.heap.c_allocator.free(_.data);
            _.data = [];
        }
    }
}

pub struct WorkerCdpEventItem {
    method: [64]u8 = [0; 64],
    method_len: usize = 0,
    session_id: [64]u8 = [0; 64],
    session_id_len: usize = 0,
    params_json: []u8 = [],

    fn freeParams(*_) {
        if (_.params_json.len > 0) {
            std.heap.c_allocator.free(_.params_json);
            _.params_json = [];
        }
    }

    fn asCdpEvent(*_): CdpEvent {
        return CdpEvent{
            method: _.method[0.._.method_len],
            params_json: if (_.params_json.len > 0) _.params_json else "{}",
            session_id: _.session_id[0.._.session_id_len],
        };
    }
}

fn dupeHeapBytes(src: []const u8): []u8 {
    if (src.len == 0) return [];
    const dup = std.heap.c_allocator.alloc(u8, src.len) catch return [];
    @memcpy(dup, src);
    return dup;
}

fn formatEscapedJsonString(src: []const u8): []u8 {
    let extra: usize = 0;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const c = src[i];
        if (c == '"' || c == '\\' || c == '\n' || c == '\r' || c == '\t') {
            extra += 1;
        } else if (c < 0x20) {
            extra += 5;
        }
    }
    let out = std.heap.c_allocator.alloc(u8, src.len + extra) catch return [];
    let pos: usize = 0;
    i = 0;
    while (i < src.len) : (i += 1) {
        const c = src[i];
        if (c == '"') {
            out[pos] = '\\';
            out[pos + 1] = '"';
            pos += 2;
        } else if (c == '\\') {
            out[pos] = '\\';
            out[pos + 1] = '\\';
            pos += 2;
        } else if (c == '\n') {
            out[pos] = '\\';
            out[pos + 1] = 'n';
            pos += 2;
        } else if (c == '\r') {
            out[pos] = '\\';
            out[pos + 1] = 'r';
            pos += 2;
        } else if (c == '\t') {
            out[pos] = '\\';
            out[pos + 1] = 't';
            pos += 2;
        } else if (c < 0x20) {
            const hex = "0123456789abcdef";
            out[pos] = '\\';
            out[pos + 1] = 'u';
            out[pos + 2] = '0';
            out[pos + 3] = '0';
            out[pos + 4] = hex[(c >> 4) & 0x0f];
            out[pos + 5] = hex[c & 0x0f];
            pos += 6;
        } else {
            out[pos] = c;
            pos += 1;
        }
    }
    return out[0..pos];
}

pub struct DedicatedWorkerEntry {
    id: u32 = 0,
    native_wid: u32 = 0,
    state: WorkerTargetState = .idle,
    flatten: bool = false,
    attached: bool = false,
    target_id: [48]u8 = [0; 48],
    target_id_len: usize = 0,
    session_id: [48]u8 = [0; 48],
    session_id_len: usize = 0,
    url: [256]u8 = [0; 256],
    url_len: usize = 0,
    title: [128]u8 = [0; 128],
    title_len: usize = 0,
    alive: bool = false,
    started: bool = false,
    runtime_enabled: bool = false,
    last_message: [512]u8 = [0; 512],
    last_message_len: usize = 0,
    down_queue: [MAX_WORKER_MSG_QUEUE]WorkerMsgPacket = [WorkerMsgPacket{}; MAX_WORKER_MSG_QUEUE],
    down_head: usize = 0,
    down_tail: usize = 0,
    down_count: usize = 0,

    fn getTargetId(*_): []const u8 {
        return _.target_id[0.._.target_id_len];
    }

    fn getSessionId(*_): []const u8 {
        return _.session_id[0.._.session_id_len];
    }

    fn getUrl(*_): []const u8 {
        return _.url[0.._.url_len];
    }

    fn getTitle(*_): []const u8 {
        return _.title[0.._.title_len];
    }

    fn getLastMessage(*_): []const u8 {
        return _.last_message[0.._.last_message_len];
    }

    fn enqueueDownMessage(*_, msg: []const u8): bool {
        if (_.down_count >= MAX_WORKER_MSG_QUEUE) return false;
        let slot = &_.down_queue[_.down_tail];
        slot.freeData();
        if (msg.len > 0) {
            const dup = dupeHeapBytes(msg);
            if (dup.len == 0) return false;
            slot.data = dup;
        }
        _.down_tail = (_.down_tail + 1) % MAX_WORKER_MSG_QUEUE;
        _.down_count += 1;
        return true;
    }

    fn dequeueDownMessage(*_, out_buf: []u8): usize {
        if (_.down_count == 0) return 0;
        let slot = &_.down_queue[_.down_head];
        const copy_len = @min(slot.data.len, out_buf.len);
        if (copy_len > 0) {
            @memcpy(out_buf[0..copy_len], slot.data[0..copy_len]);
        }
        slot.freeData();
        _.down_head = (_.down_head + 1) % MAX_WORKER_MSG_QUEUE;
        _.down_count -= 1;
        return copy_len;
    }

    fn clearDownQueue(*_) {
        let i: usize = 0;
        while (i < MAX_WORKER_MSG_QUEUE) : (i += 1) {
            _.down_queue[i].freeData();
        }
        _.down_head = 0;
        _.down_tail = 0;
        _.down_count = 0;
    }
}

pub struct DedicatedWorkerPool {
    workers: [MAX_DEDICATED_WORKERS]DedicatedWorkerEntry = [DedicatedWorkerEntry{}; MAX_DEDICATED_WORKERS],
    next_id: u32 = 1,
    count: usize = 0,
    cdp_events: [MAX_WORKER_CDP_EVENTS]WorkerCdpEventItem = [WorkerCdpEventItem{}; MAX_WORKER_CDP_EVENTS],
    cdp_event_head: usize = 0,
    cdp_event_tail: usize = 0,
    cdp_event_count: usize = 0,

    static fn init(): DedicatedWorkerPool {
        return DedicatedWorkerPool{};
    }

    fn clearCdpEvents(*_) {
        let i: usize = 0;
        while (i < MAX_WORKER_CDP_EVENTS) : (i += 1) {
            _.cdp_events[i].freeParams();
            _.cdp_events[i].method_len = 0;
            _.cdp_events[i].session_id_len = 0;
        }
        _.cdp_event_head = 0;
        _.cdp_event_tail = 0;
        _.cdp_event_count = 0;
    }

    fn deinit(*_) {
        let i: usize = 0;
        while (i < MAX_DEDICATED_WORKERS) : (i += 1) {
            _.workers[i].clearDownQueue();
            _.workers[i].alive = false;
            _.workers[i].started = false;
            _.workers[i].attached = false;
            _.workers[i].state = .idle;
        }
        _.count = 0;
        _.clearCdpEvents();
    }

    fn pushCdpEvent(*_, method: []const u8, params_json: []const u8, session_id: []const u8): bool {
        if (_.cdp_event_count >= MAX_WORKER_CDP_EVENTS) return false;
        let slot = &_.cdp_events[_.cdp_event_tail];
        slot.freeParams();

        const m_len = @min(method.len, slot.method.len);
        if (m_len > 0) @memcpy(slot.method[0..m_len], method[0..m_len]);
        slot.method_len = m_len;

        const s_len = @min(session_id.len, slot.session_id.len);
        if (s_len > 0) @memcpy(slot.session_id[0..s_len], session_id[0..s_len]);
        slot.session_id_len = s_len;

        if (params_json.len > 0) {
            const dup = dupeHeapBytes(params_json);
            if (dup.len == 0) {
                slot.method_len = 0;
                slot.session_id_len = 0;
                return false;
            }
            slot.params_json = dup;
        }

        _.cdp_event_tail = (_.cdp_event_tail + 1) % MAX_WORKER_CDP_EVENTS;
        _.cdp_event_count += 1;
        return true;
    }

    fn lastPushedEvent(*_): CdpEvent? {
        if (_.cdp_event_count == 0) return null;
        const idx = if (_.cdp_event_tail == 0) MAX_WORKER_CDP_EVENTS - 1 else _.cdp_event_tail - 1;
        return _.cdp_events[idx].asCdpEvent();
    }

    fn pollCdpEvents(*_, out_events: []CdpEvent): usize {
        let copied: usize = 0;
        while (_.cdp_event_count > 0 && copied < out_events.len) {
            out_events[copied] = _.cdp_events[_.cdp_event_head].asCdpEvent();
            _.cdp_event_head = (_.cdp_event_head + 1) % MAX_WORKER_CDP_EVENTS;
            _.cdp_event_count -= 1;
            copied += 1;
        }
        return copied;
    }

    fn createWorker(
        *_,
        registry: *TargetRegistry,
        url: []const u8,
        name: []const u8,
        out_events: []CdpEvent,
        event_count: *usize,
    ): ((*DedicatedWorkerEntry)?) {
        return _.createWorkerWithWid(registry, url, name, 0, "", out_events, event_count);
    }

    fn createWorkerWithWid(
        *_,
        registry: *TargetRegistry,
        url: []const u8,
        name: []const u8,
        native_wid: u32,
        bctx: []const u8,
        out_events: []CdpEvent,
        event_count: *usize,
    ): ((*DedicatedWorkerEntry)?) {
        event_count.* = 0;
        if (_.count >= MAX_DEDICATED_WORKERS) return null;

        let slot_idx: usize = MAX_DEDICATED_WORKERS;
        let i: usize = 0;
        while (i < MAX_DEDICATED_WORKERS) : (i += 1) {
            if (!_.workers[i].alive) {
                slot_idx = i;
                break;
            }
        }
        if (slot_idx >= MAX_DEDICATED_WORKERS) return null;

        const worker_id = _.next_id;
        _.next_id += 1;

        let entry = &_.workers[slot_idx];
        entry.clearDownQueue();
        entry.id = worker_id;
        entry.native_wid = native_wid;
        entry.state = .created;
        entry.flatten = registry.flatten;
        entry.attached = false;
        entry.target_id_len = 0;
        entry.session_id_len = 0;
        entry.last_message_len = 0;

        let title_buf: [128]u8 = [0; 128];
        const title_str = if (name.len > 0)
            std.fmt.bufPrint(&title_buf, "Worker: {s}", .{name}) catch "Worker"
        else
            std.fmt.bufPrint(&title_buf, "Worker: {s}", .{url}) catch "Worker";

        const u_len = @min(url.len, entry.url.len);
        if (u_len > 0) @memcpy(entry.url[0..u_len], url[0..u_len]);
        entry.url_len = u_len;

        const t_len = @min(title_str.len, entry.title.len);
        if (t_len > 0) @memcpy(entry.title[0..t_len], title_str[0..t_len]);
        entry.title_len = t_len;

        entry.alive = true;
        entry.started = true;
        entry.runtime_enabled = false;
        _.count += 1;

        const tid = registry.createTargetWithType(url, "worker", bctx);
        const tid_len = @min(tid.len, entry.target_id.len);
        if (tid_len > 0) @memcpy(entry.target_id[0..tid_len], tid[0..tid_len]);
        entry.target_id_len = tid_len;

        const esc_title = formatEscapedJsonString(entry.getTitle());
        defer if (esc_title.len > 0) std.heap.c_allocator.free(esc_title);
        const esc_url = formatEscapedJsonString(entry.getUrl());
        defer if (esc_url.len > 0) std.heap.c_allocator.free(esc_url);

        let tc_buf: [2048]u8 = [0; 2048];
        const tc_json = std.fmt.bufPrint(
            &tc_buf,
            "{{\"targetInfo\":{{\"targetId\":\"{s}\",\"type\":\"worker\",\"title\":\"{s}\",\"url\":\"{s}\",\"attached\":false,\"browserContextId\":\"{s}\"}}}}",
            .{ entry.getTargetId(), esc_title, esc_url, bctx },
        ) catch "";
        if (tc_json.len > 0) {
            * = _.pushCdpEvent("Target.targetCreated", tc_json, "");
        }

        if (registry.auto_attach) {
            const sid = registry.attachToTarget(tid);
            const sid_len = @min(sid.len, entry.session_id.len);
            if (sid_len > 0) @memcpy(entry.session_id[0..sid_len], sid[0..sid_len]);
            entry.session_id_len = sid_len;
            entry.attached = sid_len > 0;
            if (entry.attached) {
                entry.state = .attached;
            }

            let att_buf: [2048]u8 = [0; 2048];
            const attached_json = std.fmt.bufPrint(
                &att_buf,
                "{{\"sessionId\":\"{s}\",\"targetInfo\":{{\"targetId\":\"{s}\",\"type\":\"worker\",\"title\":\"{s}\",\"url\":\"{s}\",\"attached\":true,\"browserContextId\":\"{s}\"}},\"waitingForDebugger\":false}}",
                .{ entry.getSessionId(), entry.getTargetId(), esc_title, esc_url, bctx },
            ) catch "";
            if (attached_json.len > 0) {
                if (_.pushCdpEvent("Target.attachedToTarget", attached_json, "")) {
                    if (out_events.len > 0) {
                        if (ev := _.lastPushedEvent()) {
                            out_events[0] = ev;
                            event_count.* = 1;
                        }
                    }
                }
            }
        }

        return entry;
    }

    fn terminateWorker(
        *_,
        registry: *TargetRegistry,
        worker_id: u32,
        out_events: []CdpEvent,
        event_count: *usize,
    ): bool {
        event_count.* = 0;
        const entry_opt = _.findWorker(worker_id);
        if (entry_opt == null) return false;
        let entry = entry_opt.?;
        if (!entry.alive) return false;

        entry.state = .closing;
        const tid = entry.getTargetId();
        const sid = entry.getSessionId();
        const was_attached = entry.attached || sid.len > 0;

        * = registry.closeTarget(tid);

        let ev_idx: usize = 0;
        if (was_attached && sid.len > 0) {
            let detach_buf: [256]u8 = [0; 256];
            const detach_json = std.fmt.bufPrint(
                &detach_buf,
                "{{\"sessionId\":\"{s}\",\"targetId\":\"{s}\"}}",
                .{ sid, tid },
            ) catch "";
            if (detach_json.len > 0) {
                if (_.pushCdpEvent("Target.detachedFromTarget", detach_json, "")) {
                    if (ev_idx < out_events.len) {
                        if (ev := _.lastPushedEvent()) {
                            out_events[ev_idx] = ev;
                            ev_idx += 1;
                        }
                    }
                }
            }
        }

        let destroy_buf: [256]u8 = [0; 256];
        const destroy_json = std.fmt.bufPrint(
            &destroy_buf,
            "{{\"targetId\":\"{s}\"}}",
            .{ tid },
        ) catch "";
        if (destroy_json.len > 0) {
            if (_.pushCdpEvent("Target.targetDestroyed", destroy_json, "")) {
                if (ev_idx < out_events.len) {
                    if (ev := _.lastPushedEvent()) {
                        out_events[ev_idx] = ev;
                        ev_idx += 1;
                    }
                }
            }
        }
        event_count.* = ev_idx;

        entry.clearDownQueue();
        entry.attached = false;
        entry.state = .detached;
        entry.alive = false;
        entry.started = false;
        entry.runtime_enabled = false;
        entry.native_wid = 0;
        entry.session_id_len = 0;
        if (_.count > 0) _.count -= 1;
        return true;
    }

    fn terminateAllWorkers(*_, registry: *TargetRegistry) {
        let dummy_events: [4]CdpEvent = [CdpEvent{}; 4];
        let dummy_count: usize = 0;
        let i: usize = 0;
        while (i < MAX_DEDICATED_WORKERS) : (i += 1) {
            if (_.workers[i].alive) {
                * = _.terminateWorker(registry, _.workers[i].id, dummy_events[0..], &dummy_count);
            } else {
                _.workers[i].clearDownQueue();
            }
        }
    }

    fn postMessage(*_, worker_id: u32, data_json: []const u8): bool {
        const entry_opt = _.findWorker(worker_id);
        if (entry_opt == null) return false;
        let entry = entry_opt.?;
        if (!entry.alive) return false;

        const to_copy = @min(data_json.len, entry.last_message.len);
        if (to_copy > 0) @memcpy(entry.last_message[0..to_copy], data_json[0..to_copy]);
        entry.last_message_len = to_copy;
        return entry.enqueueDownMessage(data_json);
    }

    fn emitMessageFromWorker(*_, native_wid: u32, payload_json: []const u8): bool {
        const entry_opt = _.findWorkerByNativeWid(native_wid);
        if (entry_opt == null) return false;
        const entry = entry_opt.?;
        if (!entry.alive || !entry.attached) return false;

        const sid = entry.getSessionId();
        const tid = entry.getTargetId();
        if (sid.len == 0) return false;

        const escaped = formatEscapedJsonString(payload_json);
        defer if (escaped.len > 0) std.heap.c_allocator.free(escaped);

        const params_json = std.fmt.allocPrint(
            std.heap.c_allocator,
            "{{\"sessionId\":\"{s}\",\"message\":\"{s}\",\"targetId\":\"{s}\"}}",
            .{ sid, escaped, tid },
        ) catch return false;
        defer std.heap.c_allocator.free(params_json);

        return _.pushCdpEvent("Target.receivedMessageFromTarget", params_json, "");
    }

    fn findWorker(*_, worker_id: u32): ((*DedicatedWorkerEntry)?) {
        let i: usize = 0;
        while (i < MAX_DEDICATED_WORKERS) : (i += 1) {
            if (_.workers[i].id == worker_id && _.workers[i].alive) {
                return &_.workers[i];
            }
        }
        return null;
    }

    fn findWorkerBySessionId(*_, sid: []const u8): ((*DedicatedWorkerEntry)?) {
        if (sid.len == 0) return null;
        let i: usize = 0;
        while (i < MAX_DEDICATED_WORKERS) : (i += 1) {
            if (_.workers[i].alive && std.mem.eql(u8, _.workers[i].getSessionId(), sid)) {
                return &_.workers[i];
            }
        }
        return null;
    }

    fn findWorkerByTargetId(*_, tid: []const u8): ((*DedicatedWorkerEntry)?) {
        if (tid.len == 0) return null;
        let i: usize = 0;
        while (i < MAX_DEDICATED_WORKERS) : (i += 1) {
            if (_.workers[i].alive && std.mem.eql(u8, _.workers[i].getTargetId(), tid)) {
                return &_.workers[i];
            }
        }
        return null;
    }

    fn findWorkerByNativeWid(*_, wid: u32): ((*DedicatedWorkerEntry)?) {
        if (wid == 0) return null;
        let i: usize = 0;
        while (i < MAX_DEDICATED_WORKERS) : (i += 1) {
            if (_.workers[i].alive && _.workers[i].native_wid == wid) {
                return &_.workers[i];
            }
        }
        return null;
    }
}
