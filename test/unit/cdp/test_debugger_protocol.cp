// test/cdp/test_debugger_protocol.cp
// CapSurf Phase 10 Comprehensive CDP Debugger Domain Protocol Test Suite

import { test, expect_eq } from "cap:test";
import { CdpPageSession } from "../../../src/cdp/session.cp";
import { CdpRequest, CdpResponse, CdpEvent } from "../../../src/cdp/types.cp";
import { CdpDispatcher } from "../../../src/cdp/dispatch.cp";
import {
    CompiledScriptMeta,
    DebuggerState,
    DebuggerHandler,
    emitScriptParsedEvent,
} from "../../../src/cdp/debugger.cp";

fn testDebuggerLifecycleAndScriptParsed()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [128]CdpEvent = [CdpEvent{}; 128];
    let evt_cnt: usize = 0;

    // 1. Pre-register a compiled script in DebuggerState
    const meta = CompiledScriptMeta.create("script-101", "console.log('test');", "https://example.com/app.js");
    expect_eq(session.debugger_state.addScript(meta), true);
    expect_eq(session.debugger_state.script_count, 1);

    // 2. Debugger.enable: turns on debugging and emits Debugger.scriptParsed
    const req_enable = CdpRequest{
        id: 1,
        method: "Debugger.enable",
        params_raw: "{}",
    };
    const resp_enable = CdpDispatcher.dispatch(session, req_enable, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_enable.isOk(), true);
    expect_eq(session.debugger_state.enabled, true);
    expect_eq(evt_cnt, 1);
    expect_eq(std.mem.eql(u8, out_evts[0].method, "Debugger.scriptParsed"), true);
    expect_eq(std.mem.indexOf(u8, out_evts[0].params_json, "script-101") != null, true);
    expect_eq(std.mem.indexOf(u8, out_evts[0].params_json, "https://example.com/app.js") != null, true);

    // 3. Debugger.getScriptSource: retrieves original script source
    const req_src = CdpRequest{
        id: 2,
        method: "Debugger.getScriptSource",
        params_raw: "{\"scriptId\":\"script-101\"}",
    };
    const resp_src = CdpDispatcher.dispatch(session, req_src, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_src.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp_src.result_json, "console.log('test');") != null, true);

    // 4. Debugger.disable: resets state
    const req_disable = CdpRequest{
        id: 3,
        method: "Debugger.disable",
        params_raw: "{}",
    };
    const resp_disable = CdpDispatcher.dispatch(session, req_disable, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_disable.isOk(), true);
    expect_eq(session.debugger_state.enabled, false);
}

fn testDebuggerDriverCompatibilityStubs()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [128]CdpEvent = [CdpEvent{}; 128];
    let evt_cnt: usize = 0;

    // 1. setSkipAllPauses
    const req1 = CdpRequest{ id: 10, method: "Debugger.setSkipAllPauses", params_raw: "{\"skip\":true}" };
    expect_eq(CdpDispatcher.dispatch(session, req1, out_buf[0..], &out_evts, &evt_cnt).isOk(), true);
    expect_eq(session.debugger_state.skip_all_pauses, true);

    // 2. setBreakpointsActive
    const req2 = CdpRequest{ id: 11, method: "Debugger.setBreakpointsActive", params_raw: "{\"active\":false}" };
    expect_eq(CdpDispatcher.dispatch(session, req2, out_buf[0..], &out_evts, &evt_cnt).isOk(), true);
    expect_eq(session.debugger_state.breakpoints_active, false);

    // 3. setPauseOnExceptions
    const req3 = CdpRequest{ id: 12, method: "Debugger.setPauseOnExceptions", params_raw: "{\"state\":\"uncaught\"}" };
    expect_eq(CdpDispatcher.dispatch(session, req3, out_buf[0..], &out_evts, &evt_cnt).isOk(), true);
    expect_eq(std.mem.eql(u8, session.debugger_state.pause_on_exceptions, "uncaught"), true);

    // 4. setAsyncCallStackDepth
    const req4 = CdpRequest{ id: 13, method: "Debugger.setAsyncCallStackDepth", params_raw: "{\"maxDepth\":32}" };
    expect_eq(CdpDispatcher.dispatch(session, req4, out_buf[0..], &out_evts, &evt_cnt).isOk(), true);
    expect_eq(session.debugger_state.async_call_stack_depth, 32);

    // 5. setBlackboxPatterns & setBlackboxedRanges
    const req5 = CdpRequest{ id: 14, method: "Debugger.setBlackboxPatterns", params_raw: "{\"patterns\":[\"node_modules\"]}" };
    expect_eq(CdpDispatcher.dispatch(session, req5, out_buf[0..], &out_evts, &evt_cnt).isOk(), true);

    const req6 = CdpRequest{ id: 15, method: "Debugger.setBlackboxedRanges", params_raw: "{\"scriptId\":\"script-101\",\"positions\":[]}" };
    expect_eq(CdpDispatcher.dispatch(session, req6, out_buf[0..], &out_evts, &evt_cnt).isOk(), true);
}

fn main() {
    test("1. CDP Debugger Lifecycle, Script Registration & scriptParsed Event", () => {
        testDebuggerLifecycleAndScriptParsed() catch unreachable;
    });

    test("2. CDP Debugger Playwright / Puppeteer Driver Compatibility Stubs", () => {
        testDebuggerDriverCompatibilityStubs() catch unreachable;
    });
}
