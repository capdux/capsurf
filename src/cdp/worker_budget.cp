// Migrated from vendor/kitesurf/packages/engine/src/eval/worker-budget.ts
// CapSurf Dynamic Worker & Isolate Budget Manager.
// Zero GC, compile-time borrow & exclusivity safety, zero heap allocation.

pub const MAX_DYNAMIC_WORKERS: usize = 4;
pub const RENDERER_RESERVED_SLOTS: usize = 1;
pub const NAVIGATE_HEADROOM_SLOTS: usize = 1;
pub const MAX_CONCURRENT_OOPIF: usize = 1;

pub enum DynamicWorkerKind: u8 {
    PageScript = 0,
    Renderer = 1,
    Oopif = 2,
    Worker = 3,

    fn asStr(): []const u8 {
        return switch (_) {
            .PageScript => "pageScript",
            .Renderer => "renderer",
            .Oopif => "oopif",
            .Worker => "worker",
        };
    }
}

pub struct WorkerLease {
    kind: DynamicWorkerKind = .Worker,
    active: bool = false,
    lease_id: u32 = 0,

    fn relinquish(*_, budget: *DynamicWorkerBudget) {
        if (!_.active) return;
        _.active = false;
        budget.releaseKind(_.kind);
    }
}

pub struct WorkerLeaseStats {
    live: usize = 0,
    peak: usize = 0,
    page_script_live: usize = 0,
    renderer_live: usize = 0,
    oopif_live: usize = 0,
    worker_live: usize = 0,
    page_script_peak: usize = 0,
    renderer_peak: usize = 0,
    oopif_peak: usize = 0,
    worker_peak: usize = 0,
    oopif_denied: usize = 0,
    worker_denied: usize = 0,
    isolates_killed: usize = 0,

    fn liveOf(kind: DynamicWorkerKind): usize {
        return switch (kind) {
            .PageScript => _.page_script_live,
            .Renderer => _.renderer_live,
            .Oopif => _.oopif_live,
            .Worker => _.worker_live,
        };
    }

    fn peakOf(kind: DynamicWorkerKind): usize {
        return switch (kind) {
            .PageScript => _.page_script_peak,
            .Renderer => _.renderer_peak,
            .Oopif => _.oopif_peak,
            .Worker => _.worker_peak,
        };
    }
}

pub struct DynamicWorkerBudget {
    counts: [4]usize = [0; 4],
    peaks: [4]usize = [0; 4],
    peak: usize = 0,
    oopif_denied: usize = 0,
    worker_denied: usize = 0,
    isolates_killed: usize = 0,
    next_lease_id: u32 = 1,
    page_targets: usize = 1,

    static fn init(): DynamicWorkerBudget {
        return DynamicWorkerBudget{};
    }

    fn reset(*_) {
        _.counts = [0; 4];
        _.peaks = [0; 4];
        _.peak = 0;
        _.oopif_denied = 0;
        _.worker_denied = 0;
        _.isolates_killed = 0;
        _.next_lease_id = 1;
        _.page_targets = 1;
    }

    fn noteIsolateKilled(*_) {
        _.isolates_killed += 1;
    }

    fn setPageTargets(*_, count: usize) {
        let n = count;
        if (n < 1) n = 1;
        _.page_targets = n;
    }

    fn navigateHeadroom(): usize {
        let targets = _.page_targets;
        if (targets < 1) targets = 1;
        return NAVIGATE_HEADROOM_SLOTS * targets;
    }

    fn liveOf(kind: DynamicWorkerKind): usize {
        return _.counts[@intFromEnum(kind)];
    }

    fn peakOf(kind: DynamicWorkerKind): usize {
        return _.peaks[@intFromEnum(kind)];
    }

    fn live(): usize {
        return _.counts[0] + _.counts[1] + _.counts[2] + _.counts[3];
    }

    fn freeSlots(): usize {
        const live_total = _.live();
        const reserved_renderer = if (_.counts[@intFromEnum(DynamicWorkerKind.Renderer)] > 0) @as(usize, 0) else RENDERER_RESERVED_SLOTS;
        const reserved = reserved_renderer + _.navigateHeadroom();
        if (live_total + reserved >= MAX_DYNAMIC_WORKERS) {
            return 0;
        }
        return MAX_DYNAMIC_WORKERS - (live_total + reserved);
    }

    fn freeForOopif(): usize {
        if (_.counts[@intFromEnum(DynamicWorkerKind.Oopif)] >= MAX_CONCURRENT_OOPIF) {
            return 0;
        }
        return _.freeSlots();
    }

    fn canSpawnOopif(): bool {
        return _.freeForOopif() >= 1;
    }

    fn acquire(*_, kind: DynamicWorkerKind): WorkerLease? {
        if (kind == .Oopif && _.freeForOopif() < 1) {
            _.oopif_denied += 1;
            return null;
        }
        if (kind == .Worker && _.freeSlots() < 1) {
            _.worker_denied += 1;
            return null;
        }
        const idx = @intFromEnum(kind);
        _.counts[idx] += 1;
        if (_.counts[idx] > _.peaks[idx]) {
            _.peaks[idx] = _.counts[idx];
        }
        const current_live = _.live();
        if (current_live > _.peak) {
            _.peak = current_live;
        }
        const lid = _.next_lease_id;
        _.next_lease_id += 1;
        return WorkerLease{
            kind: kind,
            active: true,
            lease_id: lid,
        };
    }

    fn releaseKind(*_, kind: DynamicWorkerKind) {
        const idx = @intFromEnum(kind);
        if (_.counts[idx] > 0) {
            _.counts[idx] -= 1;
        }
    }

    fn releaseLease(*_, lease: *WorkerLease) {
        if (!lease.active) return;
        lease.active = false;
        _.releaseKind(lease.kind);
    }

    fn snapshot(): WorkerLeaseStats {
        return WorkerLeaseStats{
            live: _.live(),
            peak: _.peak,
            page_script_live: _.counts[@intFromEnum(DynamicWorkerKind.PageScript)],
            renderer_live: _.counts[@intFromEnum(DynamicWorkerKind.Renderer)],
            oopif_live: _.counts[@intFromEnum(DynamicWorkerKind.Oopif)],
            worker_live: _.counts[@intFromEnum(DynamicWorkerKind.Worker)],
            page_script_peak: _.peaks[@intFromEnum(DynamicWorkerKind.PageScript)],
            renderer_peak: _.peaks[@intFromEnum(DynamicWorkerKind.Renderer)],
            oopif_peak: _.peaks[@intFromEnum(DynamicWorkerKind.Oopif)],
            worker_peak: _.peaks[@intFromEnum(DynamicWorkerKind.Worker)],
            oopif_denied: _.oopif_denied,
            worker_denied: _.worker_denied,
            isolates_killed: _.isolates_killed,
        };
    }

    fn describe(out_buf: []u8): []const u8 {
        return std.fmt.bufPrint(
            out_buf,
            "live={d}/{d} pageScript={d} renderer={d} oopif={d}/{d} worker={d} headroom={d} denied={d} workerDenied={d} killed={d}",
            .{
                _.live(),
                MAX_DYNAMIC_WORKERS,
                _.counts[@intFromEnum(DynamicWorkerKind.PageScript)],
                _.counts[@intFromEnum(DynamicWorkerKind.Renderer)],
                _.counts[@intFromEnum(DynamicWorkerKind.Oopif)],
                MAX_CONCURRENT_OOPIF,
                _.counts[@intFromEnum(DynamicWorkerKind.Worker)],
                _.navigateHeadroom(),
                _.oopif_denied,
                _.worker_denied,
                _.isolates_killed,
            },
        ) catch "";
    }
}
