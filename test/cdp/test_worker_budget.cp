// test/cdp/test_worker_budget.cp
// CapSurf Phase 15 Comprehensive Test Suite: Dynamic Worker & Isolate Budget Manager

import {
    MAX_DYNAMIC_WORKERS,
    RENDERER_RESERVED_SLOTS,
    NAVIGATE_HEADROOM_SLOTS,
    MAX_CONCURRENT_OOPIF,
    DynamicWorkerKind,
    WorkerLease,
    WorkerLeaseStats,
    DynamicWorkerBudget,
} from "../../src/cdp/worker_budget.cp";

fn expect(cond: bool) {
    if (!cond) @panic("assertion failed");
}

fn testConstants() {
    expect(MAX_DYNAMIC_WORKERS == 4);
    expect(RENDERER_RESERVED_SLOTS == 1);
    expect(NAVIGATE_HEADROOM_SLOTS == 1);
    expect(MAX_CONCURRENT_OOPIF == 1);
}

fn testBudgetAcquisitionAndRelease() {
    let budget = DynamicWorkerBudget.init();
    expect(budget.live() == 0);
    expect(budget.freeSlots() == 2); // 4 total - (1 reserved renderer + 1 navigate headroom)
    expect(budget.canSpawnOopif());

    // 1. Acquire first Worker
    let opt_w1 = budget.acquire(DynamicWorkerKind.Worker);
    expect(opt_w1 != null);
    if (w1 := opt_w1) {
        expect(w1.active);
        expect(w1.kind == .Worker);
        expect(budget.live() == 1);
        expect(budget.freeSlots() == 1);
    }

    // 2. Acquire second Worker (exhausting remaining free slots)
    let opt_w2 = budget.acquire(DynamicWorkerKind.Worker);
    expect(opt_w2 != null);
    if (w2 := opt_w2) {
        expect(w2.active);
        expect(budget.live() == 2);
        expect(budget.freeSlots() == 0);
    }

    // 3. Third Worker must be denied (freeSlots == 0)
    let opt_w3 = budget.acquire(DynamicWorkerKind.Worker);
    expect(opt_w3 == null);
    expect(budget.worker_denied == 1);

    // 4. OOPIF must also be denied when freeSlots == 0
    expect(!budget.canSpawnOopif());
    let opt_oopif1 = budget.acquire(DynamicWorkerKind.Oopif);
    expect(opt_oopif1 == null);
    expect(budget.oopif_denied == 1);

    // 5. Release w2 -> frees 1 slot
    if (*w2 := opt_w2) {
        w2.relinquish(&budget);
        expect(!w2.active);
        expect(budget.live() == 1);
        expect(budget.freeSlots() == 1);
        expect(budget.canSpawnOopif());

        // Idempotent double release
        w2.relinquish(&budget);
        expect(budget.live() == 1);
    }

    // 6. Now OOPIF can acquire
    let opt_oopif2 = budget.acquire(DynamicWorkerKind.Oopif);
    expect(opt_oopif2 != null);
    if (o2 := opt_oopif2) {
        expect(o2.active);
        expect(o2.kind == .Oopif);
        expect(budget.liveOf(.Oopif) == 1);
        // OOPIF cap is 1, so second OOPIF cannot spawn even if slots exist
        expect(!budget.canSpawnOopif());
        let opt_oopif3 = budget.acquire(DynamicWorkerKind.Oopif);
        expect(opt_oopif3 == null);
        expect(budget.oopif_denied == 2);
    }

    // 7. Cleanup remaining leases
    if (*w1 := opt_w1) {
        w1.relinquish(&budget);
        expect(!w1.active);
    }
    if (*o2 := opt_oopif2) {
        o2.relinquish(&budget);
        expect(!o2.active);
    }
    expect(budget.live() == 0);
    expect(budget.freeSlots() == 2);
}

fn testDiagnosticsAndSnapshot() {
    let budget = DynamicWorkerBudget.init();

    budget.noteIsolateKilled();
    budget.noteIsolateKilled();
    expect(budget.isolates_killed == 2);

    let opt_ps = budget.acquire(DynamicWorkerKind.PageScript);
    expect(opt_ps != null);

    const snap = budget.snapshot();
    expect(snap.live == 1);
    expect(snap.peak == 1);
    expect(snap.page_script_live == 1);
    expect(snap.page_script_peak == 1);
    expect(snap.isolates_killed == 2);

    let desc_buf: [256]u8 = [0; 256];
    const desc = budget.describe(desc_buf[0..]);
    expect(desc.len > 0);
    expect(std.mem.indexOf(u8, desc, "live=1/4") != null);
    expect(std.mem.indexOf(u8, desc, "pageScript=1") != null);
    expect(std.mem.indexOf(u8, desc, "killed=2") != null);

    if (*ps := opt_ps) {
        ps.relinquish(&budget);
    }
    expect(budget.live() == 0);
}

fn main() {
    testConstants();
    testBudgetAcquisitionAndRelease();
    testDiagnosticsAndSnapshot();
    std.debug.print("ALL PHASE 15 WORKER BUDGET TESTS PASSED SUCCESSFULLY!\n", .{});
}
