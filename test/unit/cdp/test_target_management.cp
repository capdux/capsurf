// test/cdp/test_target_management.cp
// CapSurf Phase 5 Comprehensive Target Domain & Multi-Target Test Suite
//
// Validates:
//   1. BrowserContext lifecycle: createBrowserContext -> getBrowserContexts -> disposeBrowserContext
//   2. Multi-target lifecycle: createTarget -> getTargets -> getTargetInfo -> closeTarget
//   3. Session attach/detach: attachToTarget -> attachedToTarget event -> detachFromTarget
//   4. Auto-Attach mechanism: setAutoAttach -> createTarget -> targetCreated & attachedToTarget events

import { test, expect_eq } from "cap:test";
import { CdpPageSession } from "../../../src/cdp/session.cp";
import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, extractJsonBool } from "../../../src/cdp/types.cp";
import { CdpDispatcher } from "../../../src/cdp/dispatch.cp";

fn testBrowserContextLifecycle()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [128]CdpEvent = [CdpEvent{}; 128];
    let evt_cnt: usize = 0;

    // 1. Create a new BrowserContext
    const req_create = CdpRequest{
        id: 1,
        method: "Target.createBrowserContext",
        params_raw: "{}",
    };
    const resp_create = CdpDispatcher.dispatch(session, req_create, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_create.isOk(), true);

    let ctx_id_buf: [64]u8 = undefined;
    const ctx_id_opt = extractJsonString(resp_create.result_json, "browserContextId");
    expect_eq(ctx_id_opt != null, true);
    expect_eq(ctx_id_opt.?.len > 0, true);
    @memcpy(ctx_id_buf[0..ctx_id_opt.?.len], ctx_id_opt.?);
    const ctx_id = ctx_id_buf[0..ctx_id_opt.?.len];

    // 2. Query BrowserContexts and verify it contains the created ctx_id
    const req_get = CdpRequest{
        id: 2,
        method: "Target.getBrowserContexts",
        params_raw: "{}",
    };
    const resp_get = CdpDispatcher.dispatch(session, req_get, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_get.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp_get.result_json, ctx_id) != null, true);

    // 3. Dispose the created BrowserContext
    let dispose_buf: [256]u8 = undefined;
    const dispose_params = std.fmt.bufPrint(&dispose_buf, "{{\"browserContextId\":\"{s}\"}}", .{ctx_id}) catch "";
    const req_dispose = CdpRequest{
        id: 3,
        method: "Target.disposeBrowserContext",
        params_raw: dispose_params,
    };
    const resp_dispose = CdpDispatcher.dispatch(session, req_dispose, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_dispose.isOk(), true);

    // 4. Query BrowserContexts again and verify ctx_id is cleaned up
    const req_get2 = CdpRequest{
        id: 4,
        method: "Target.getBrowserContexts",
        params_raw: "{}",
    };
    const resp_get2 = CdpDispatcher.dispatch(session, req_get2, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_get2.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp_get2.result_json, ctx_id) == null, true);
}

fn testMultiTargetLifecycle()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [128]CdpEvent = [CdpEvent{}; 128];
    let evt_cnt: usize = 0;

    // 1. Create a dedicated browser context
    const req_ctx = CdpRequest{
        id: 10,
        method: "Target.createBrowserContext",
        params_raw: "{}",
    };
    const resp_ctx = CdpDispatcher.dispatch(session, req_ctx, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_ctx.isOk(), true);

    let ctx_id_buf: [64]u8 = undefined;
    const ctx_id_opt = extractJsonString(resp_ctx.result_json, "browserContextId");
    expect_eq(ctx_id_opt != null, true);
    @memcpy(ctx_id_buf[0..ctx_id_opt.?.len], ctx_id_opt.?);
    const ctx_id = ctx_id_buf[0..ctx_id_opt.?.len];

    // 2. Create target under the context with specific url
    let create_buf: [512]u8 = undefined;
    const create_params = std.fmt.bufPrint(
        &create_buf,
        "{{\"url\":\"https://example.com/multi-agent\",\"browserContextId\":\"{s}\"}}",
        .{ctx_id},
    ) catch "";
    const req_create = CdpRequest{
        id: 11,
        method: "Target.createTarget",
        params_raw: create_params,
    };
    const resp_create = CdpDispatcher.dispatch(session, req_create, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_create.isOk(), true);

    let target_id_buf: [64]u8 = undefined;
    const target_id_opt = extractJsonString(resp_create.result_json, "targetId");
    expect_eq(target_id_opt != null, true);
    expect_eq(target_id_opt.?.len > 0, true);
    @memcpy(target_id_buf[0..target_id_opt.?.len], target_id_opt.?);
    const target_id = target_id_buf[0..target_id_opt.?.len];

    // 3. Query all targets via Target.getTargets
    const req_get_all = CdpRequest{
        id: 12,
        method: "Target.getTargets",
        params_raw: "{}",
    };
    const resp_get_all = CdpDispatcher.dispatch(session, req_get_all, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_get_all.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp_get_all.result_json, target_id) != null, true);
    expect_eq(std.mem.indexOf(u8, resp_get_all.result_json, "https://example.com/multi-agent") != null, true);

    // 4. Query target detail via Target.getTargetInfo
    let info_buf: [256]u8 = undefined;
    const info_params = std.fmt.bufPrint(&info_buf, "{{\"targetId\":\"{s}\"}}", .{target_id}) catch "";
    const req_info = CdpRequest{
        id: 13,
        method: "Target.getTargetInfo",
        params_raw: info_params,
    };
    const resp_info = CdpDispatcher.dispatch(session, req_info, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_info.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp_info.result_json, target_id) != null, true);
    expect_eq(std.mem.indexOf(u8, resp_info.result_json, "\"type\":\"page\"") != null || std.mem.indexOf(u8, resp_info.result_json, "\"type\": \"page\"") != null, true);
    expect_eq(std.mem.indexOf(u8, resp_info.result_json, "https://example.com/multi-agent") != null, true);

    // 5. Close target via Target.closeTarget
    const req_close = CdpRequest{
        id: 14,
        method: "Target.closeTarget",
        params_raw: info_params,
    };
    const resp_close = CdpDispatcher.dispatch(session, req_close, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_close.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp_close.result_json, "\"success\":true") != null || std.mem.indexOf(u8, resp_close.result_json, "\"success\": true") != null, true);

    // 6. Verify target is no longer listed in Target.getTargets
    const req_get_after = CdpRequest{
        id: 15,
        method: "Target.getTargets",
        params_raw: "{}",
    };
    const resp_get_after = CdpDispatcher.dispatch(session, req_get_after, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_get_after.isOk(), true);
    expect_eq(std.mem.indexOf(u8, resp_get_after.result_json, target_id) == null, true);
}

fn testSessionAttachDetach()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [128]CdpEvent = [CdpEvent{}; 128];
    let evt_cnt: usize = 0;

    // 1. Create a new target to attach
    const req_create = CdpRequest{
        id: 20,
        method: "Target.createTarget",
        params_raw: "{\"url\":\"https://example.com/session-test\"}",
    };
    const resp_create = CdpDispatcher.dispatch(session, req_create, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_create.isOk(), true);

    let target_id_buf: [64]u8 = undefined;
    const target_id_opt = extractJsonString(resp_create.result_json, "targetId");
    expect_eq(target_id_opt != null, true);
    @memcpy(target_id_buf[0..target_id_opt.?.len], target_id_opt.?);
    const target_id = target_id_buf[0..target_id_opt.?.len];

    // 2. Attach to target
    let attach_buf: [256]u8 = undefined;
    const attach_params = std.fmt.bufPrint(&attach_buf, "{{\"targetId\":\"{s}\",\"flatten\":true}}", .{target_id}) catch "";
    const req_attach = CdpRequest{
        id: 21,
        method: "Target.attachToTarget",
        params_raw: attach_params,
    };
    const resp_attach = CdpDispatcher.dispatch(session, req_attach, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_attach.isOk(), true);

    let session_id_buf: [64]u8 = undefined;
    const session_id_opt = extractJsonString(resp_attach.result_json, "sessionId");
    expect_eq(session_id_opt != null, true);
    expect_eq(session_id_opt.?.len > 0, true);
    @memcpy(session_id_buf[0..session_id_opt.?.len], session_id_opt.?);
    const session_id = session_id_buf[0..session_id_opt.?.len];

    // Verify Target.attachedToTarget event was triggered
    expect_eq(evt_cnt >= 1, true);
    let found_attached_event = false;
    let i: usize = 0;
    while (i < evt_cnt) : (i += 1) {
        if (std.mem.eql(u8, out_evts[i].method, "Target.attachedToTarget")) {
            found_attached_event = true;
            expect_eq(std.mem.indexOf(u8, out_evts[i].params_json, session_id) != null, true);
            expect_eq(std.mem.indexOf(u8, out_evts[i].params_json, target_id) != null, true);
        }
    }
    expect_eq(found_attached_event, true);

    // 3. Detach from target
    let detach_buf: [256]u8 = undefined;
    const detach_params = std.fmt.bufPrint(&detach_buf, "{{\"sessionId\":\"{s}\"}}", .{session_id}) catch "";
    const req_detach = CdpRequest{
        id: 22,
        method: "Target.detachFromTarget",
        params_raw: detach_params,
    };
    const resp_detach = CdpDispatcher.dispatch(session, req_detach, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_detach.isOk(), true);
}

fn testAutoAttachMechanism()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [128]CdpEvent = [CdpEvent{}; 128];
    let evt_cnt: usize = 0;

    // 1. Configure setAutoAttach with autoAttach: true and flatten: true
    const req_auto = CdpRequest{
        id: 30,
        method: "Target.setAutoAttach",
        params_raw: "{\"autoAttach\":true,\"waitForDebuggerOnStart\":false,\"flatten\":true}",
    };
    const resp_auto = CdpDispatcher.dispatch(session, req_auto, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_auto.isOk(), true);

    // 2. Create target after auto-attach enabled
    const req_create = CdpRequest{
        id: 31,
        method: "Target.createTarget",
        params_raw: "{\"url\":\"https://example.com/auto-attached-worker\"}",
    };
    const resp_create = CdpDispatcher.dispatch(session, req_create, out_buf[0..], &out_evts, &evt_cnt);
    expect_eq(resp_create.isOk(), true);

    let target_id_buf: [64]u8 = undefined;
    const target_id_opt = extractJsonString(resp_create.result_json, "targetId");
    expect_eq(target_id_opt != null, true);
    @memcpy(target_id_buf[0..target_id_opt.?.len], target_id_opt.?);
    const target_id = target_id_buf[0..target_id_opt.?.len];

    // 3. Assert synchronous emission of BOTH Target.targetCreated AND Target.attachedToTarget events
    expect_eq(evt_cnt >= 2, true);

    let has_target_created = false;
    let has_attached_to_target = false;
    let i: usize = 0;
    while (i < evt_cnt) : (i += 1) {
        if (std.mem.eql(u8, out_evts[i].method, "Target.targetCreated")) {
            has_target_created = true;
            expect_eq(std.mem.indexOf(u8, out_evts[i].params_json, target_id) != null, true);
        } else if (std.mem.eql(u8, out_evts[i].method, "Target.attachedToTarget")) {
            has_attached_to_target = true;
            expect_eq(std.mem.indexOf(u8, out_evts[i].params_json, target_id) != null, true);
            expect_eq(std.mem.indexOf(u8, out_evts[i].params_json, "sessionId") != null, true);
        }
    }
    expect_eq(has_target_created, true);
    expect_eq(has_attached_to_target, true);
}

fn main() {
    test("1. CDP BrowserContext Lifecycle (create -> list -> dispose)", () => {
        testBrowserContextLifecycle() catch unreachable;
    });

    test("2. CDP Multi-Target Lifecycle (createTarget -> getTargets -> getTargetInfo -> closeTarget)", () => {
        testMultiTargetLifecycle() catch unreachable;
    });

    test("3. CDP Session Attach & Detach (attachToTarget -> event -> detachFromTarget)", () => {
        testSessionAttachDetach() catch unreachable;
    });

    test("4. CDP Auto-Attach Mechanism (setAutoAttach -> createTarget -> targetCreated & attachedToTarget events)", () => {
        testAutoAttachMechanism() catch unreachable;
    });
}
