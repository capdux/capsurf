// test/cdp/test_runtime_bindings.cp
// CapSurf CDP Runtime Bindings, Exception Details & Lifecycle Events Unit Tests

import { CdpPageSession } from "../../src/cdp/session.cp";
import { CdpRequest, CdpResponse, CdpEvent } from "../../src/cdp/types.cp";
import { CdpDispatcher } from "../../src/cdp/dispatch.cp";

fn check(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. Test Runtime.addBinding via CdpDispatcher
    {
        let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
        session.runtime_enabled = true;

        let out_buf: [4096]u8 = [0; 4096];
        let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
        let evt_cnt: usize = 0;

        const req_binding = CdpRequest{
            id: 101,
            method: "Runtime.addBinding",
            params_raw: "{\"name\":\"ksTestBinding\"}",
        };
        const resp_binding = CdpDispatcher.dispatch(session, req_binding, out_buf[0..], &out_evts, &evt_cnt);
        check(resp_binding.isOk(), "Runtime.addBinding response ok");

        // Verify function installed
        let eval_buf: [1024]u8 = [0; 1024];
        const eval_len = session.evaluate("typeof globalThis.ksTestBinding === 'function' && typeof window.ksTestBinding === 'function'", &eval_buf);
        check(std.mem.indexOf(u8, eval_buf[0..eval_len], "\"value\":true") != null, "ksTestBinding is function");

        // 2. Test binding call and event emission
        const call_expr = "window.ksTestBinding('payload-xyz'); null";
        let call_res_buf: [4096]u8 = [0; 4096];
        let call_evts: [64]CdpEvent = [CdpEvent{}; 64];
        let call_evt_cnt: usize = 0;

        const req_eval = CdpRequest{
            id: 102,
            method: "Runtime.evaluate",
            params_raw: "{\"expression\":\"window.ksTestBinding('payload-xyz'); null\"}",
        };
        const resp_eval = CdpDispatcher.dispatch(session, req_eval, call_res_buf[0..], &call_evts, &call_evt_cnt);
        check(resp_eval.isOk(), "Runtime.evaluate call ok");
        check(call_evt_cnt >= 1, "has bindingCalled event");
        check(std.mem.eql(u8, call_evts[0].method, "Runtime.bindingCalled"), "method is Runtime.bindingCalled");
        check(std.mem.indexOf(u8, call_evts[0].params_json, "\"name\":\"ksTestBinding\"") != null, "bindingCalled name");
        check(std.mem.indexOf(u8, call_evts[0].params_json, "\"payload\":\"payload-xyz\"") != null, "bindingCalled payload");

        std.debug.print("PASS: 1. Runtime.addBinding & Runtime.bindingCalled\n", .{});
    }

    // 2. Test Exception Details className and description formatting
    {
        let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;

        let exc_buf: [4096]u8 = [0; 4096];
        const exc_len = session.evaluate("throw new TypeError('bad arguments')", &exc_buf);
        const exc_json = exc_buf[0..exc_len];

        check(std.mem.indexOf(u8, exc_json, "\"className\":\"TypeError\"") != null, "has className TypeError");
        check(std.mem.indexOf(u8, exc_json, "\"description\":\"TypeError: bad arguments\\n\"") != null, "has description with newline");
        check(std.mem.indexOf(u8, exc_json, "\"subtype\":\"error\"") != null, "has subtype error");

        std.debug.print("PASS: 2. Exception className & description formatting\n", .{});
    }

    // 3. Test HTML5 Lifecycle Events dispatching (DOMContentLoaded & load)
    {
        let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;

        const test_html =
            "<!DOCTYPE html><html><head><script>" ++
            "window.__events = [];" ++
            "document.addEventListener('DOMContentLoaded', function() { __events.push('DOMContentLoaded'); });" ++
            "window.addEventListener('load', function() { __events.push('load'); });" ++
            "</script></head><body><p>Hello</p></body></html>";

        session.navigate(test_html, "https://example.com/test.html", "") catch unreachable;

        let eval_buf: [1024]u8 = [0; 1024];
        const eval_len = session.evaluate("JSON.stringify(window.__events)", &eval_buf);
        const eval_json = eval_buf[0..eval_len];

        check(std.mem.indexOf(u8, eval_json, "DOMContentLoaded") != null, "DOMContentLoaded event fired");
        check(std.mem.indexOf(u8, eval_json, "load") != null, "load event fired");

        std.debug.print("PASS: 3. HTML5 Lifecycle Events dispatching\n", .{});
    }
}
