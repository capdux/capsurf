// Migrated from vendor/kitesurf/packages/engine/src/eval/dedicated-worker.ts
// Loads page-created dedicated workers and bridges their RPC and upstream events.

pub const WORKER_SCRIPT_LINE_OFFSET: u32 = 3;
pub const TEARDOWN_GRACE_MS: u64 = 1000;
pub const MAX_WORKERS: usize = 32;
pub const MAX_WORKER_PAYLOAD_CHARS: usize = 512;
pub const MAX_WORKER_LABEL_CHARS: usize = 64;
pub const MAX_WORKER_ID_CHARS: usize = 64;

pub enum WorkerUpstreamEvent {
    WorkerMessage,
    WorkerError,
    WorkerClosed,
}

pub enum WorkerState {
    Init,
    Running,
    Closing,
    Terminated,
}

pub fn upstreamEventToString(ev: WorkerUpstreamEvent): []const u8 {
    return switch (ev) {
        .WorkerMessage => "worker.message",
        .WorkerError => "worker.error",
        .WorkerClosed => "worker.closed",
    };
}

pub fn parseUpstreamEvent(name: []const u8): WorkerUpstreamEvent? {
    if (std.mem.eql(u8, name, "worker.message")) return .WorkerMessage;
    if (std.mem.eql(u8, name, "worker.error")) return .WorkerError;
    if (std.mem.eql(u8, name, "worker.closed")) return .WorkerClosed;
    return null;
}

pub struct WorkerUpstreamMessage {
    event: WorkerUpstreamEvent = .WorkerMessage,
    payload: []const u8 = "",
    timestamp_ms: u64 = 0,

    fn set(*_, ev: WorkerUpstreamEvent, data: []const u8, t_ms: u64) {
        _.event = ev;
        _.payload = data;
        _.timestamp_ms = t_ms;
    }

    fn getPayload(): []const u8 {
        return _.payload;
    }
}

pub struct SpawnDedicatedWorkerOptions {
    connection_id: []const u8 = "",
    cookie_key: []const u8 = "",
    label: []const u8 = "dedicated-worker",
    origin: []const u8 = "",
    creator_url: []const u8 = "",
    frame_id: []const u8 = "",
    loader_id: []const u8 = "",
    cpu_ms: u32 = 50,
    source: []const u8 = "",
}

pub struct DedicatedWorker {
    id: []const u8 = "",
    label: []const u8 = "",
    state: WorkerState = .Init,
    cpu_ms: u32 = 50,
    messages_sent: u32 = 0,
    messages_received: u32 = 0,
    errors_count: u32 = 0,
    gone: bool = false,

    fn init(*_, wid: []const u8, lbl: []const u8, budget_cpu: u32) {
        _.id = wid;
        _.label = lbl;
        _.state = .Init;
        _.cpu_ms = budget_cpu;
        _.messages_sent = 0;
        _.messages_received = 0;
        _.errors_count = 0;
        _.gone = false;
    }

    fn getId(): []const u8 {
        return _.id;
    }

    fn getLabel(): []const u8 {
        return _.label;
    }

    fn start(*_): bool {
        if (_.state != .Init) return false;
        _.state = .Running;
        return true;
    }

    fn deliverMessage(*_, data: []const u8): bool {
        * = data;
        if (_.state != .Running) return false;
        _.messages_sent += 1;
        return true;
    }

    fn recordUpstream(*_, ev: WorkerUpstreamEvent): bool {
        if (_.state == .Terminated) return false;
        switch (ev) {
            .WorkerMessage => {
                _.messages_received += 1;
            },
            .WorkerError => {
                _.errors_count += 1;
            },
            .WorkerClosed => {
                _.state = .Terminated;
            },
        }
        return true;
    }

    fn terminate(*_, reason: []const u8, is_gone: bool) {
        * = reason;
        if (is_gone) {
            _.gone = true;
        }
        _.state = .Terminated;
    }
}

pub struct DedicatedWorkerRegistry {
    workers: [MAX_WORKERS]DedicatedWorker = [DedicatedWorker{}; 32],
    occupied: [MAX_WORKERS]bool = [false; 32],
    active_count: usize = 0,

    fn spawn(*_, opts: SpawnDedicatedWorkerOptions): usize? {
        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < MAX_WORKERS) : (i += 1) {
            if (!_.occupied[i]) {
                free_idx = i;
                break;
            }
        }
        if (free_idx == null) return null;
        let idx = free_idx.?;

        _.workers[idx].init(opts.connection_id, opts.label, opts.cpu_ms);
        _.occupied[idx] = true;
        _.active_count += 1;
        return idx;
    }

    fn getWorker(*_, idx: usize): (*DedicatedWorker)? {
        if (idx >= MAX_WORKERS || !_.occupied[idx]) return null;
        return &_.workers[idx];
    }

    fn terminateWorker(*_, idx: usize, reason: []const u8, is_gone: bool): bool {
        if (idx >= MAX_WORKERS || !_.occupied[idx]) return false;
        _.workers[idx].terminate(reason, is_gone);
        _.occupied[idx] = false;
        if (_.active_count > 0) {
            _.active_count -= 1;
        }
        return true;
    }

    fn activeCount(): usize {
        return _.active_count;
    }
}

pub fn hasUseStrict(source: []const u8): bool {
    let trimmed = std.mem.trim(u8, source, " \t\r\n");
    if (trimmed.len < 12) return false;
    let head = trimmed[0..12];
    return std.mem.eql(u8, head, "\"use strict\"") || std.mem.eql(u8, head, "'use strict'");
}

pub fn buildWorkerScriptModule(source: []const u8, out_buf: []u8): []const u8 {
    let p_run = "module.exports.run = async function () { ";
    let strict_hdr = if (hasUseStrict(source)) "\"use strict\"; \n" else "\n";
    let p_defs = "var module=undefined,exports=undefined,require=undefined;\n";
    let p_tail = "\n};\n";

    let total_len = p_run.len + strict_hdr.len + p_defs.len + source.len + p_tail.len;
    if (total_len > out_buf.len) return "";

    let pos: usize = 0;
    @memcpy(out_buf[pos .. pos + p_run.len], p_run);
    pos += p_run.len;

    @memcpy(out_buf[pos .. pos + strict_hdr.len], strict_hdr);
    pos += strict_hdr.len;

    @memcpy(out_buf[pos .. pos + p_defs.len], p_defs);
    pos += p_defs.len;

    @memcpy(out_buf[pos .. pos + source.len], source);
    pos += source.len;

    @memcpy(out_buf[pos .. pos + p_tail.len], p_tail);
    pos += p_tail.len;

    return out_buf[0..pos];
}
