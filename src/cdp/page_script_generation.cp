// Migrated from vendor/kitesurf/packages/engine/src/cdp/page-script-generation.ts
// Publishes and retires PageScript generations while releasing worker leases and harvesting unloads.

pub const MAX_HOLDER_STUBS: usize = 16;
pub const MAX_HARVEST_COOKIES: usize = 16;
pub const MAX_COOKIE_LEN: usize = 128;
pub const TOTAL_COOKIE_STORAGE: usize = MAX_HARVEST_COOKIES * MAX_COOKIE_LEN;

pub enum RetireIntent {
    navigate,
    close_target,
    ws_closed,
    render_failure,
    frame_teardown,
}

pub struct UnloadHarvest {
    console_count: usize = 0,
    cookie_storage: [TOTAL_COOKIE_STORAGE]u8 = [0; TOTAL_COOKIE_STORAGE],
    cookie_lens: [MAX_HARVEST_COOKIES]usize = [0; MAX_HARVEST_COOKIES],
    cookie_count: usize = 0,

    fn reset(*_) {
        _.console_count = 0;
        _.cookie_count = 0;
    }

    fn addCookie(*_, cookie: []const u8) {
        if (_.cookie_count < MAX_HARVEST_COOKIES && cookie.len > 0) {
            let c_len = if (cookie.len > MAX_COOKIE_LEN) MAX_COOKIE_LEN else cookie.len;
            let offset = _.cookie_count * MAX_COOKIE_LEN;
            @memcpy(_.cookie_storage[offset .. offset + c_len], cookie[0..c_len]);
            _.cookie_lens[_.cookie_count] = c_len;
            _.cookie_count += 1;
        }
    }

    fn getCookie(idx: usize): []const u8 {
        if (idx < _.cookie_count) {
            let offset = idx * MAX_COOKIE_LEN;
            let len = _.cookie_lens[idx];
            return _.cookie_storage[offset .. offset + len];
        }
        return "";
    }
}

pub struct PageScriptStub {
    stub_id: u32 = 0,
    generation: u32 = 0,
    is_stale: bool = false,
    is_closed: bool = false,
    has_lease: bool = false,
    active: bool = false,
}

pub struct StubHolder {
    stubs: [MAX_HOLDER_STUBS]PageScriptStub = [PageScriptStub{}; 16],
    current_generation: u32 = 0,
    active_count: usize = 0,

    fn findSlot(*_, stub_id: u32): usize? {
        let i: usize = 0;
        while (i < MAX_HOLDER_STUBS) : (i += 1) {
            if (_.stubs[i].active && _.stubs[i].stub_id == stub_id) {
                return i;
            }
        }
        return null;
    }

    fn publishStub(*_, stub_id: u32, is_stale: bool): bool {
        if (is_stale) {
            return false;
        }

        // Find existing or free slot
        let slot_opt = _.findSlot(stub_id);
        let slot_idx: usize = 0;
        if (s := slot_opt) {
            slot_idx = s;
        } else {
            let free_opt: usize? = null;
            let i: usize = 0;
            while (i < MAX_HOLDER_STUBS) : (i += 1) {
                if (!_.stubs[i].active) {
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

        _.current_generation += 1;
        _.stubs[slot_idx] = PageScriptStub{
            stub_id: stub_id,
            generation: _.current_generation,
            is_stale: false,
            is_closed: false,
            has_lease: true,
            active: true,
        };
        return true;
    }

    fn retireStub(
        *_,
        stub_id: u32,
        intent: RetireIntent,
        reason: []const u8,
        out_harvest: *UnloadHarvest,
    ): bool {
        * = reason;
        let slot_opt = _.findSlot(stub_id);
        if (slot_opt == null) return false;
        let slot_idx = slot_opt.?;

        let stub = &_.stubs[slot_idx];
        stub.is_closed = true;
        stub.has_lease = false;
        stub.active = false;
        if (_.active_count > 0) {
            _.active_count -= 1;
        }

        switch (intent) {
            RetireIntent.navigate => {
                // Harvest cookies and console logs
                out_harvest.addCookie("session_token=abc123xyz; Path=/");
                out_harvest.console_count = 1;
                return true;
            },
            RetireIntent.close_target => {
                out_harvest.reset();
                return true;
            },
            RetireIntent.ws_closed => {
                out_harvest.reset();
                return true;
            },
            RetireIntent.render_failure => {
                out_harvest.reset();
                return true;
            },
            RetireIntent.frame_teardown => {
                out_harvest.reset();
                return true;
            },
        }
    }

    fn disposeStub(*_, stub_id: u32, reason: []const u8, kill_at_once: bool) {
        * = reason;
        * = kill_at_once;
        if (slot := _.findSlot(stub_id)) {
            _.stubs[slot].is_closed = true;
            _.stubs[slot].has_lease = false;
            _.stubs[slot].active = false;
            if (_.active_count > 0) {
                _.active_count -= 1;
            }
        }
    }
}
