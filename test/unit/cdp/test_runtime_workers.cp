// test/cdp/test_runtime_workers.cp
// CapSurf Phase 6 Comprehensive Runtime, Log & Dedicated Worker Test Suite
//
// Validates:
//   1. Runtime Domain Lifecycle: Runtime.enable -> executionContextCreated -> getProperties -> callFunctionOn
//   2. Console & Log Domain Multiplexing: JS console.log/warn/error -> Runtime.consoleAPICalled & Log.entryAdded
//   3. Dedicated Worker Lifecycle & Auto-Attach: createWorker -> Target.attachedToTarget -> postMessage -> terminateWorker

import { test, expect_eq } from "cap:test";
import { CdpPageSession } from "../../../src/cdp/session.cp";
import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt } from "../../../src/cdp/types.cp";
import { CdpDispatcher } from "../../../src/cdp/dispatch.cp";

fn testRuntimeLifecycle()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    // 1. Runtime.enable
    const req_enable = CdpRequest{
        id: 1,
        method: "Runtime.enable",
        params_raw: "{}",
    };
    const resp_enable = CdpDispatcher.dispatch(session, req_enable, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_enable.isOk(), true);
    expect_eq(session.runtime_enabled, true);
    expect_eq(evt_cnt >= 1, true);
    expect_eq(std.mem.eql(u8, out_evts[0].method, "Runtime.executionContextCreated"), true);
    expect_eq(std.mem.indexOf(u8, out_evts[0].params_json, "\"id\":1") != null, true);

    // 2. Runtime.getProperties
    const req_props = CdpRequest{
        id: 2,
        method: "Runtime.getProperties",
        params_raw: "{\"objectId\":\"obj-1\"}",
    };
    const resp_props = CdpDispatcher.dispatch(session, req_props, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_props.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp_props.result_json, "\"result\":[]") != null, true);

    // 3. Runtime.callFunctionOn
    const req_call = CdpRequest{
        id: 3,
        method: "Runtime.callFunctionOn",
        params_raw: "{\"functionDeclaration\":\"function() { return 1; }\"}",
    };
    const resp_call = CdpDispatcher.dispatch(session, req_call, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_call.isOk(), true);

    // 4. Runtime.disable
    const req_disable = CdpRequest{
        id: 4,
        method: "Runtime.disable",
        params_raw: "{}",
    };
    const resp_disable = CdpDispatcher.dispatch(session, req_disable, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_disable.isOk(), true);
    expect_eq(session.runtime_enabled, false);
}

fn testConsoleAndLogMultiplexing()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    // 1. Enable both Runtime and Log domains
    const req_rt_enable = CdpRequest{ id: 1, method: "Runtime.enable", params_raw: "{}" };
    * = CdpDispatcher.dispatch(session, req_rt_enable, out_buf[0..], &out_evts, &evt_cnt);

    const req_log_enable = CdpRequest{ id: 2, method: "Log.enable", params_raw: "{}" };
    const resp_log = CdpDispatcher.dispatch(session, req_log_enable, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_log.isOk(), true);
    expect_eq(session.log_enabled, true);

    // 2. Load page and evaluate console logs
    session.navigate("<html><body><script>console.log('Agent bootstrap ok');</script></body></html>", "http://agent.local/index.html", "") catch unreachable;

    // 3. Runtime.evaluate triggering console.warn
    const req_eval = CdpRequest{
        id: 3,
        method: "Runtime.evaluate",
        params_raw: "{\"expression\":\"console.warn('Agent attention required'); 42;\"}",
    };
    const resp_eval = CdpDispatcher.dispatch(session, req_eval, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_eval.isOk(), true);
    expect_eq(evt_cnt >= 1, true);

    // Verify console event was polled and produced Runtime.consoleAPICalled and Log.entryAdded
    let saw_console_api: bool = false;
    let saw_log_entry: bool = false;
    let i: usize = 0;
    while (i < evt_cnt) : (i += 1) {
        if (std.mem.eql(u8, out_evts[i].method, "Runtime.consoleAPICalled")) {
            if (std.mem.indexOf(u8, out_evts[i].params_json, "Agent attention required") != null) {
                saw_console_api = true;
            }
        }
        if (std.mem.eql(u8, out_evts[i].method, "Log.entryAdded")) {
            if (std.mem.indexOf(u8, out_evts[i].params_json, "warning") != null) {
                saw_log_entry = true;
            }
        }
    }
    expect_eq(saw_console_api, true);
    expect_eq(saw_log_entry, true);

    // 4. Log.clear
    const req_clear = CdpRequest{ id: 4, method: "Log.clear", params_raw: "{}" };
    const resp_clear = CdpDispatcher.dispatch(session, req_clear, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_clear.isOk(), true);

    // 5. Log.disable
    const req_log_disable = CdpRequest{ id: 5, method: "Log.disable", params_raw: "{}" };
    const resp_log_disable = CdpDispatcher.dispatch(session, req_log_disable, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_log_disable.isOk(), true);
    expect_eq(session.log_enabled, false);
}

fn testDedicatedWorkerLifecycle()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [64]CdpEvent = [CdpEvent{}; 64];
    let evt_cnt: usize = 0;

    // 1. Enable Auto-Attach in TargetRegistry so workers are automatically attached
    session.target_registry.setAutoAttach(true, true);
    expect_eq(session.target_registry.auto_attach, true);

    // 2. Create a Dedicated Worker
    let worker_opt = session.dedicated_workers.createWorker(
        &session.target_registry,
        "http://agent.local/compute_worker.js",
        "ComputeTask-1",
        &out_evts,
        &evt_cnt,
    );
    expect_eq(worker_opt != null, true);
    let worker = worker_opt.?;

    expect_eq(worker.id, 1);
    expect_eq(worker.alive, true);
    expect_eq(std.mem.eql(u8, worker.getUrl(), "http://agent.local/compute_worker.js"), true);
    expect_eq(std.mem.eql(u8, worker.getTitle(), "Worker: ComputeTask-1"), true);
    expect_eq(worker.getSessionId().len > 0, true);

    // Verify auto-attach emitted Target.attachedToTarget
    expect_eq(evt_cnt, 1);
    expect_eq(std.mem.eql(u8, out_evts[0].method, "Target.attachedToTarget"), true);
    expect_eq(std.mem.indexOf(u8, out_evts[0].params_json, "\"type\":\"worker\"") != null, true);

    // 3. Post Message to Worker
    const post_ok = session.dedicated_workers.postMessage(worker.id, "{\"command\":\"start_inference\",\"model\":\"qjs\"}");
    expect_eq(post_ok, true);
    expect_eq(std.mem.indexOf(u8, worker.getLastMessage(), "start_inference") != null, true);

    // 4. Terminate Worker
    const term_ok = session.dedicated_workers.terminateWorker(
        &session.target_registry,
        worker.id,
        &out_evts,
        &evt_cnt,
    );
    expect_eq(term_ok, true);
    expect_eq(worker.alive, false);

    // Verify Target.detachedFromTarget and Target.targetDestroyed events
    expect_eq(evt_cnt, 2);
    expect_eq(std.mem.eql(u8, out_evts[0].method, "Target.detachedFromTarget"), true);
    expect_eq(std.mem.eql(u8, out_evts[1].method, "Target.targetDestroyed"), true);

    // Verify worker slot is cleaned up
    const find_res = session.dedicated_workers.findWorker(1);
    expect_eq(find_res == null, true);
}

fn main() {
    test("1. CDP Runtime Domain Execution Context Lifecycle", () => {
        testRuntimeLifecycle() catch unreachable;
    });

    test("2. CDP Console API & Log Domain Multiplexing", () => {
        testConsoleAndLogMultiplexing() catch unreachable;
    });

    test("3. CDP Dedicated Worker Lifecycle & Target Auto-Attach", () => {
        testDedicatedWorkerLifecycle() catch unreachable;
    });
}
