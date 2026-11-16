// Migrated from vendor/kitesurf/packages/engine/src/eval/page-script-rpc-session.ts
// Adapts PageScript RPC calls with per-call timeouts and bounded teardown.

@c {
    struct TimeVal {
        tv_sec: i64;
        tv_usec: i64;
    }
    fn gettimeofday(tv: *TimeVal, tz: *!?): i32;
}

fn getNowMs(): u64 {
    let tv = TimeVal{ tv_sec: 0, tv_usec: 0 };
    * = gettimeofday(&tv, null);
    if (tv.tv_sec <= 0) return 0;
    return @as(u64, @intCast(tv.tv_sec)) * 1000 + @as(u64, @intCast(tv.tv_usec)) / 1000;
}

pub const TEARDOWN_RPC_GRACE_MS: u64 = 1000;
pub const MAX_INFLIGHT_RPC: usize = 64;
pub const DEFAULT_RPC_TIMEOUT_MS: u64 = 15000;

pub struct InflightRpcCall {
    id: u64 = 0,
    method: []const u8 = "",
    send_time_ms: u64 = 0,
    timeout_ms: u64 = 15000,
    is_active: bool = false,
}

pub struct PageScriptRpcSession {
    session_id: []const u8 = "",
    inflight: [MAX_INFLIGHT_RPC]InflightRpcCall = [InflightRpcCall{}; 64],
    request_seq: u64 = 1,
    is_closed: bool = false,
    close_reason: []const u8 = "",

    fn nextRequestId(*_): u64 {
        let id = _.request_seq;
        _.request_seq += 1;
        return id;
    }

    fn trackCall(*_, method: []const u8, timeout_ms: u64): u64 {
        if (_.is_closed) return 0;
        let req_id = _.nextRequestId();
        let t_ms = getNowMs();

        let found_idx: usize = MAX_INFLIGHT_RPC;
        let i: usize = 0;
        while (i < MAX_INFLIGHT_RPC) : (i += 1) {
            if (!_.inflight[i].is_active) {
                found_idx = i;
                break;
            }
        }

        if (found_idx < MAX_INFLIGHT_RPC) {
            _.inflight[found_idx] = InflightRpcCall{
                id: req_id,
                method: method,
                send_time_ms: t_ms,
                timeout_ms: if (timeout_ms > 0) timeout_ms else DEFAULT_RPC_TIMEOUT_MS,
                is_active: true,
            };
        }
        return req_id;
    }

    fn completeCall(*_, req_id: u64): bool {
        if (req_id == 0) return false;
        let i: usize = 0;
        while (i < MAX_INFLIGHT_RPC) : (i += 1) {
            if (_.inflight[i].is_active && _.inflight[i].id == req_id) {
                _.inflight[i].is_active = false;
                return true;
            }
        }
        return false;
    }

    fn finishSession(*_, reason: []const u8) {
        _.is_closed = true;
        _.close_reason = reason;
        let i: usize = 0;
        while (i < MAX_INFLIGHT_RPC) : (i += 1) {
            if (_.inflight[i].is_active) {
                _.inflight[i].is_active = false;
            }
        }
    }

    fn getInflightCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < MAX_INFLIGHT_RPC) : (i += 1) {
            if (_.inflight[i].is_active) {
                count += 1;
            }
        }
        return count;
    }

    fn isCallTimedOut(req_id: u64, now_ms: u64): bool {
        let i: usize = 0;
        while (i < MAX_INFLIGHT_RPC) : (i += 1) {
            if (_.inflight[i].is_active && _.inflight[i].id == req_id) {
                return (now_ms > _.inflight[i].send_time_ms + _.inflight[i].timeout_ms);
            }
        }
        return false;
    }
}

pub fn openPageScriptRpcSession(session_id: []const u8): PageScriptRpcSession {
    return PageScriptRpcSession{
        session_id: session_id,
    };
}
