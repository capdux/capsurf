// src/cdp/workers.cp
// Dedicated Worker Lifecycle & CDP Target Multiplexing for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/dedicated-workers.ts & eval/dedicated-worker.ts

import { CdpEvent } from "./types.cp";
import { TargetRegistry } from "./target.cp";

pub struct DedicatedWorkerEntry {
    id: u32 = 0,
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
    last_message: [512]u8 = [0; 512],
    last_message_len: usize = 0,

    pub fn getTargetId(*_): []const u8 {
        return _.target_id[0.._.target_id_len];
    }

    pub fn getSessionId(*_): []const u8 {
        return _.session_id[0.._.session_id_len];
    }

    pub fn getUrl(*_): []const u8 {
        return _.url[0.._.url_len];
    }

    pub fn getTitle(*_): []const u8 {
        return _.title[0.._.title_len];
    }

    pub fn getLastMessage(*_): []const u8 {
        return _.last_message[0.._.last_message_len];
    }
}

pub const MAX_DEDICATED_WORKERS: usize = 8;

pub struct DedicatedWorkerPool {
    workers: [MAX_DEDICATED_WORKERS]DedicatedWorkerEntry = [DedicatedWorkerEntry{}; MAX_DEDICATED_WORKERS],
    next_id: u32 = 1,
    count: usize = 0,
    target_buf: [256]u8 = [0; 256],
    attach_event_buf: [512]u8 = [0; 512],
    destroy_event_buf: [256]u8 = [0; 256],

    pub static fn init(): DedicatedWorkerPool {
        return DedicatedWorkerPool{};
    }

    pub fn createWorker(
        *_,
        registry: *TargetRegistry,
        url: []const u8,
        name: []const u8,
        out_events: []CdpEvent,
        event_count: *usize,
    ): ((*DedicatedWorkerEntry)?) {
        event_count.* = 0;
        if (_.count >= MAX_DEDICATED_WORKERS) return null;

        const worker_id = _.next_id;
        _.next_id += 1;

        // Find free slot
        let slot_idx: usize = MAX_DEDICATED_WORKERS;
        let i: usize = 0;
        while (i < MAX_DEDICATED_WORKERS) : (i += 1) {
            if (!_.workers[i].alive) {
                slot_idx = i;
                break;
            }
        }
        if (slot_idx >= MAX_DEDICATED_WORKERS) return null;

        let entry = &_.workers[slot_idx];
        entry.id = worker_id;

        // Format title
        let title_buf: [128]u8 = [0; 128];
        const title_str = if (name.len > 0)
            std.fmt.bufPrint(&title_buf, "Worker: {s}", .{name}) catch "Worker"
        else
            std.fmt.bufPrint(&title_buf, "Worker: {s}", .{url}) catch "Worker";

        const u_len = @min(url.len, entry.url.len);
        @memcpy(entry.url[0..u_len], url[0..u_len]);
        entry.url_len = u_len;

        const t_len = @min(title_str.len, entry.title.len);
        @memcpy(entry.title[0..t_len], title_str[0..t_len]);
        entry.title_len = t_len;

        entry.alive = true;
        entry.started = true;
        entry.last_message_len = 0;
        _.count += 1;

        // Create target in TargetRegistry with target_type "worker"
        const tid = registry.createTargetWithType(url, "worker", "");
        const tid_len = @min(tid.len, entry.target_id.len);
        @memcpy(entry.target_id[0..tid_len], tid[0..tid_len]);
        entry.target_id_len = tid_len;

        // If auto-attach is active in TargetRegistry, attach to target
        if (registry.auto_attach) {
            const sid = registry.attachToTarget(tid);
            const sid_len = @min(sid.len, entry.session_id.len);
            @memcpy(entry.session_id[0..sid_len], sid[0..sid_len]);
            entry.session_id_len = sid_len;

            // Produce Target.attachedToTarget event
            const attached_json = std.fmt.bufPrint(
                &_.attach_event_buf,
                "{{\"sessionId\":\"{s}\",\"targetInfo\":{{\"targetId\":\"{s}\",\"type\":\"worker\",\"title\":\"{s}\",\"url\":\"{s}\",\"attached\":true,\"browserContextId\":\"\"}},\"waitingForDebugger\":false}}",
                .{ entry.getSessionId(), entry.getTargetId(), entry.getTitle(), entry.getUrl() },
            ) catch "";
            if (attached_json.len > 0 && out_events.len > 0) {
                out_events[0] = CdpEvent.create("Target.attachedToTarget", attached_json);
                event_count.* = 1;
            }
        }

        return entry;
    }

    pub fn terminateWorker(
        *_,
        registry: *TargetRegistry,
        worker_id: u32,
        out_events: []CdpEvent,
        event_count: *usize,
    ): bool {
        event_count.* = 0;
        let entry = _.findWorker(worker_id) orelse return false;
        if (!entry.alive) return false;

        const tid = entry.getTargetId();
        const sid = entry.getSessionId();

        // Close target in registry
        * = registry.closeTarget(tid);

        let ev_idx: usize = 0;
        if (sid.len > 0 && ev_idx < out_events.len) {
            const detach_json = std.fmt.bufPrint(
                &_.attach_event_buf,
                "{{\"sessionId\":\"{s}\",\"targetId\":\"{s}\"}}",
                .{ sid, tid },
            ) catch "";
            if (detach_json.len > 0) {
                out_events[ev_idx] = CdpEvent.create("Target.detachedFromTarget", detach_json);
                ev_idx += 1;
            }
        }

        if (ev_idx < out_events.len) {
            const destroy_json = std.fmt.bufPrint(
                &_.destroy_event_buf,
                "{{\"targetId\":\"{s}\"}}",
                .{ tid },
            ) catch "";
            if (destroy_json.len > 0) {
                out_events[ev_idx] = CdpEvent.create("Target.targetDestroyed", destroy_json);
                ev_idx += 1;
            }
        }
        event_count.* = ev_idx;

        entry.alive = false;
        entry.started = false;
        if (_.count > 0) _.count -= 1;
        return true;
    }

    pub fn postMessage(*_, worker_id: u32, data_json: []const u8): bool {
        let entry = _.findWorker(worker_id) orelse return false;
        if (!entry.alive) return false;

        const to_copy = @min(data_json.len, entry.last_message.len);
        @memcpy(entry.last_message[0..to_copy], data_json[0..to_copy]);
        entry.last_message_len = to_copy;
        return true;
    }

    pub fn findWorker(*_, worker_id: u32): ((*DedicatedWorkerEntry)?) {
        let i: usize = 0;
        while (i < MAX_DEDICATED_WORKERS) : (i += 1) {
            if (_.workers[i].id == worker_id && _.workers[i].alive) {
                return &_.workers[i];
            }
        }
        return null;
    }

    pub fn findWorkerByTargetId(*_, target_id: []const u8): ((*DedicatedWorkerEntry)?) {
        let i: usize = 0;
        while (i < MAX_DEDICATED_WORKERS) : (i += 1) {
            if (_.workers[i].alive and std.mem.eql(u8, _.workers[i].getTargetId(), target_id)) {
                return &_.workers[i];
            }
        }
        return null;
    }
}
