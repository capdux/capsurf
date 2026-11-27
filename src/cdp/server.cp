// src/cdp/server.cp
// CapSurf Chrome DevTools Protocol (CDP) Server
// Lightweight, native HTTP & RFC 6455 WebSocket server compliant with Puppeteer, Playwright & AI Agents.
// Uses direct POSIX C-FFI sockets for maximum concurrency, zero-GC, and zero Zig version fragility.

import { CdpBoxModel, CdpTarget, CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, extractJsonFloat, extractJsonBool, unescapeJsonString, MAX_CDP_EVENTS } from "./types.cp";
import { CdpPageSession, escapeJsonString, DEFAULT_HTTP_USER_AGENT } from "./session.cp";
import { CdpDispatcher } from "./dispatch.cp";
import { drainDomMutationEvents, ByteSlice } from "./dom.cp";
import { performPendingNavigation, markFetchNavPreAnnounced, emitNewChildFrameEvents, canUseAsyncHttpNav, buildAsyncNavHttpRequest, parseRawHttpToLoaderResult, takePendingJsNavUrl, completePageNavigationFromResult, completePendingNavigationFromResult, consumeFetchNavPreAnnounced } from "./page.cp";
import { FetchHandler, InterceptVerdict } from "./fetch.cp";
import { MockFixtures } from "../net/mock_fixtures.cp";
import { NodeId, NULL_NODE, NodeType } from "../dom/dom.cp";
import { getMilliTimestamp } from "../js/dom_binding.cp";
import { Base64 } from "cap:encoding";
import { ScreencastHandler, formatScreencastFrameParams } from "./screencast.cp";
import { SceneStreamManager, MSG_SCENE, MSG_PARTIAL, SCENE_ENVELOPE_HEADER_BYTES, SCENE_PARTIAL_HEADER_BYTES } from "./scene_stream.cp";
import { SceneHostManifest, formatManifestJson } from "./scene_host.cp";
import { encodeRgbaRectToPng } from "../render/painter.cp";
import { registerDialogHook, registerWorkerContextHook } from "./session_hooks.cp";
import { DedicatedWorkerPool } from "./workers.cp";
import { TargetRegistry } from "./target.cp";
import * as HttpScheduler from "../net/http_scheduler.cp";
const NetScheduler = HttpScheduler.NetScheduler;
const NetworkIdleTracker = HttpScheduler.NetworkIdleTracker;
const NetPollFd = HttpScheduler.PollFd;
const MAX_SCHEDULER_SLOTS = HttpScheduler.MAX_SCHEDULER_SLOTS;

pub @c {
    struct SockAddrIn {
        sin_family: u16;
        sin_port: u16;
        sin_addr: u32;
        sin_zero: [8]u8;
    }

    struct PollFd {
        fd: i32;
        events: i16;
        revents: i16;
    }

    fn socket(domain: i32, @"type": i32, protocol: i32): i32;
    fn bind(fd: i32, addr: *const SockAddrIn, len: u32): i32;
    fn listen(fd: i32, backlog: i32): i32;
    fn accept(fd: i32, addr: *!?, len: *!?): i32;
    fn connect(fd: i32, addr: *const SockAddrIn, len: u32): i32;
    fn send(fd: i32, buf: [*]const u8, len: usize, flags: i32): isize;
    fn recv(fd: i32, buf: [*]u8, len: usize, flags: i32): isize;
    fn poll(fds: *PollFd, nfds: u64, timeout: i32): i32;
    fn read(fd: i32, buf: [*]u8, count: usize): isize;
    fn lseek(fd: i32, offset: i64, whence: i32): i64;
    fn shutdown(fd: i32, how: i32): i32;
    fn setsockopt(fd: i32, level: i32, optname: i32, optval: *const i32, optlen: u32): i32;
}

pub const POLLIN: i16 = 1;
pub const POLLERR: i16 = 8;
pub const POLLHUP: i16 = 16;
pub const POLLNVAL: i16 = 32;

pub const AF_INET: i32 = 2;
pub const SOCK_STREAM: i32 = 1;
pub const SOL_SOCKET: i32 = 1;
pub const SO_REUSEADDR: i32 = 2;
pub const IPPROTO_TCP: i32 = 6;
pub const TCP_NODELAY: i32 = 1;

fn setTcpNoDelay(fd: i32) {
    const one: i32 = 1;
    * = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, @sizeOf(i32));
}

fn serverNowMs(): i64 {
    return getMilliTimestamp();
}

pub struct PendingPromise {
    active: bool = false,
    req_id: i64 = 0,
    raw_id_buf: [64]u8 = [0; 64],
    raw_id_len: usize = 0,
    session_id_buf: [128]u8 = [0; 128],
    session_id_len: usize = 0,
    target_sess: (*CdpPageSession)? = null,
    slot: i32 = -1,
    return_by_value: bool = false,
}

pub struct PendingSlowEval {
    active: bool = false,
    req_id: i64 = 0,
    payload_buf: [4096]u8 = [0; 4096],
    payload_len: usize = 0,
    session_id_buf: [64]u8 = [0; 64],
    session_id_len: usize = 0,
}

pub const MAX_ASYNC_NAVS: usize = 8;

pub const AsyncNavPhase = enum {
    connecting,
    writing,
    reading,
};

pub struct AsyncNavSlot {
    active: bool = false,
    superseded: bool = false,
    is_cdp_nav: bool = false,
    fd: i32 = -1,
    phase: AsyncNavPhase = .connecting,
    target_sess: (*CdpPageSession)? = null,
    session_id_buf: [128]u8 = [0; 128],
    session_id_len: usize = 0,
    req_id: i64 = 0,
    raw_id_buf: [64]u8 = [0; 64],
    raw_id_len: usize = 0,
    loader_id_buf: [64]u8 = [0; 64],
    loader_id_len: usize = 0,
    url_buf: [2048]u8 = [0; 2048],
    url_len: usize = 0,
    referrer_buf: [2048]u8 = [0; 2048],
    referrer_len: usize = 0,
    payload_buf: [4096]u8 = [0; 4096],
    payload_len: usize = 0,
    skip_first_req_sent: bool = false,
    send_buf: [8192]u8 = [0; 8192],
    send_len: usize = 0,
    send_offset: usize = 0,
    recv_buf: ([]u8)? = null,
    recv_len: usize = 0,
    started_ms: i64 = 0,

    fn closeFd(*_) {
        if (_.fd >= 0) {
            * = std.c.close(_.fd);
            _.fd = -1;
        }
    }

    fn reset(*_) {
        _.closeFd();
        if (rb := _.recv_buf) {
            std.heap.c_allocator.free(rb);
            _.recv_buf = null;
        }
        _.active = false;
        _.superseded = false;
        _.is_cdp_nav = false;
        _.phase = .connecting;
        _.target_sess = null;
        _.session_id_len = 0;
        _.req_id = 0;
        _.raw_id_len = 0;
        _.loader_id_len = 0;
        _.url_len = 0;
        _.referrer_len = 0;
        _.payload_len = 0;
        _.skip_first_req_sent = false;
        _.send_len = 0;
        _.send_offset = 0;
        _.recv_len = 0;
        _.started_ms = 0;
    }
}

pub struct PendingNavState {
    pending_nav_active: bool = false,
    pending_nav_deadline_ms: i64 = 0,
    pending_nav_payload_buf: [4096]u8 = [0; 4096],
    pending_nav_payload_len: usize = 0,
    pending_nav_sid_buf: [64]u8 = [0; 64],
    pending_nav_sid_len: usize = 0,
    pending_nav_tid_buf: [64]u8 = [0; 64],
    pending_nav_tid_len: usize = 0,
    pending_fetch_nav_active: bool = false,
    pending_fetch_nav_payload_buf: [4096]u8 = [0; 4096],
    pending_fetch_nav_payload_len: usize = 0,
    pending_fetch_nav_req_id_buf: [64]u8 = [0; 64],
    pending_fetch_nav_req_id_len: usize = 0,
    pending_evals: [8]PendingSlowEval = [PendingSlowEval{}; 8],
    pending_eval_count: usize = 0,
    async_navs: [MAX_ASYNC_NAVS]AsyncNavSlot = [AsyncNavSlot{}; MAX_ASYNC_NAVS],
}

pub struct PendingWheelReply {
    req_id: i64 = 0,
    raw_id_buf: [32]u8 = [0; 32],
    raw_id_len: usize = 0,
}

pub struct PendingWheelBatch {
    active: bool = false,
    x: f64 = 0.0,
    y: f64 = 0.0,
    delta_x: f64 = 0.0,
    delta_y: f64 = 0.0,
    modifiers: i64 = 0,
    button_buf: [16]u8 = [0; 16],
    button_len: usize = 0,
    buttons: i64 = 0,
    session_id_buf: [64]u8 = [0; 64],
    session_id_len: usize = 0,
    replies: [64]PendingWheelReply = [PendingWheelReply{}; 64],
    reply_count: usize = 0,

    fn reset(*_) {
        _.active = false;
        _.x = 0.0;
        _.y = 0.0;
        _.delta_x = 0.0;
        _.delta_y = 0.0;
        _.modifiers = 0;
        _.button_len = 0;
        _.buttons = 0;
        _.session_id_len = 0;
        _.reply_count = 0;
    }

    fn matches(x: f64, y: f64, modifiers: i64, btn: []const u8, buttons: i64, sid: []const u8): bool {
        if (!_.active) return false;
        if (_.reply_count >= _.replies.len) return false;
        if (_.x != x || _.y != y || _.modifiers != modifiers || _.buttons != buttons) return false;
        if (!std.mem.eql(u8, _.button_buf[0.._.button_len], btn)) return false;
        if (!std.mem.eql(u8, _.session_id_buf[0.._.session_id_len], sid)) return false;
        return true;
    }

    fn addReply(*_, req_id: i64, raw_id: []const u8) {
        if (_.reply_count >= _.replies.len) return;
        _.replies[_.reply_count].req_id = req_id;
        const rlen = @min(raw_id.len, _.replies[_.reply_count].raw_id_buf.len);
        if (rlen > 0) {
            @memcpy(_.replies[_.reply_count].raw_id_buf[0..rlen], raw_id[0..rlen]);
        }
        _.replies[_.reply_count].raw_id_len = rlen;
        _.reply_count += 1;
    }
}

fn parsePositiveDecimal(s: []const u8): i64 {
    let val: i64 = 0;
    let i: usize = 0;
    let has_digit = false;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') break;
        has_digit = true;
        val = val * 10 + @as(i64, c - '0');
        if (val > 86400000) break;
    }
    if (!has_digit) return 0;
    return val;
}

fn serverHexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn decodeUriComponentLen(src: []const u8, dst: []u8): usize {
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < src.len && out_len < dst.len) {
        if (src[i] == '%' && i + 2 < src.len) {
            if (hi := serverHexVal(src[i + 1])) {
                if (lo := serverHexVal(src[i + 2])) {
                    dst[out_len] = (hi << 4) | lo;
                    out_len += 1;
                    i += 3;
                    continue;
                }
            }
        }
        dst[out_len] = if (src[i] == '+') ' ' else src[i];
        out_len += 1;
        i += 1;
    }
    return out_len;
}

fn extractQueryString(req: []const u8, key: []const u8, out_buf: []u8): ([]const u8)? {
    const line_end = std.mem.indexOf(u8, req, "\r\n") ?? req.len;
    const req_line = req[0..line_end];
    const sp1_opt = std.mem.indexOfScalar(u8, req_line, ' ');
    if (sp1_opt == null) return null;
    const after_sp1 = req_line[sp1_opt.? + 1..];
    const sp2 = std.mem.indexOfScalar(u8, after_sp1, ' ') ?? after_sp1.len;
    const path = after_sp1[0..sp2];
    const qpos_opt = std.mem.indexOfScalar(u8, path, '?');
    if (qpos_opt == null) return null;
    const query = path[qpos_opt.? + 1..];

    let it = std.mem.splitScalar(u8, query, '&');
    while (part := it.next()) {
        if (eq_pos := std.mem.indexOfScalar(u8, part, '=')) {
            const k = part[0..eq_pos];
            const v = part[eq_pos + 1..];
            if (std.mem.eql(u8, k, key)) {
                const n = decodeUriComponentLen(v, out_buf);
                return out_buf[0..n];
            }
        }
    }
    return null;
}

fn extractQueryInt(req: []const u8, key: []const u8): i64 {
    const line_end = std.mem.indexOf(u8, req, "\r\n") ?? req.len;
    const req_line = req[0..line_end];
    const sp1_opt = std.mem.indexOfScalar(u8, req_line, ' ');
    if (sp1_opt == null) return 0;
    const after_sp1 = req_line[sp1_opt.? + 1..];
    const sp2 = std.mem.indexOfScalar(u8, after_sp1, ' ') ?? after_sp1.len;
    const path = after_sp1[0..sp2];
    const qpos_opt = std.mem.indexOfScalar(u8, path, '?');
    if (qpos_opt == null) return 0;
    const query = path[qpos_opt.? + 1..];

    let it = std.mem.splitScalar(u8, query, '&');
    while (part := it.next()) {
        if (eq_pos := std.mem.indexOfScalar(u8, part, '=')) {
            const k = part[0..eq_pos];
            const v = part[eq_pos + 1..];
            if (std.mem.eql(u8, k, key)) {
                return parsePositiveDecimal(v);
            }
        }
    }
    return 0;
}

fn extractSlowScriptDelayMs(url: []const u8): i64 {
    const marker_opt = std.mem.indexOf(u8, url, "slow-script");
    if (marker_opt == null) return 0;
    const tail = url[marker_opt.?..];
    if (p1 := std.mem.indexOf(u8, tail, "ms=")) {
        const d = parsePositiveDecimal(tail[p1 + 3..]);
        if (d > 0) return if (d > 10000) 10000 else d;
    }
    if (p2 := std.mem.indexOf(u8, tail, "ms%3D")) {
        const d = parsePositiveDecimal(tail[p2 + 5..]);
        if (d > 0) return if (d > 10000) 10000 else d;
    }
    if (p3 := std.mem.indexOf(u8, tail, "ms%3d")) {
        const d = parsePositiveDecimal(tail[p3 + 5..]);
        if (d > 0) return if (d > 10000) 10000 else d;
    }
    return 0;
}

pub fn socketWriteAll(fd: i32, data: []const u8)! {
    let sent: usize = 0;
    while (sent < data.len) {
        const n = send(fd, data.ptr + sent, data.len - sent, 0);
        if (n <= 0) return error.WriteFailed;
        sent += @as(usize, @intCast(n));
    }
}

pub fn socketReadAll(fd: i32, buf: []u8)!usize {
    let received: usize = 0;
    while (received < buf.len) {
        const n = recv(fd, buf.ptr + received, buf.len - received, 0);
        if (n <= 0) return error.ReadFailed;
        received += @as(usize, @intCast(n));
    }
    return received;
}

pub fn socketRead(fd: i32, buf: []u8): usize! {
    const n = recv(fd, buf.ptr, buf.len, 0);
    if (n < 0) return error.ReadFailed;
    return @as(usize, @intCast(n));
}

fn wakeServer(port: u16) {
    const sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) return;
    const addr = SockAddrIn {
        sin_family: @as(u16, @intCast(AF_INET)),
        sin_port: std.mem.nativeToBig(u16, port),
        sin_addr: 0x0100007F,
        sin_zero: [0; 8],
    };
    if (connect(sock_fd, &addr, @sizeOf(SockAddrIn)) >= 0) {}
    * = std.c.close(sock_fd);
}

const MAX_TAB_SESSIONS: usize = 64;

struct TabSessionSlot {
    active: bool = false,
    key_buf: [64]u8 = [0; 64],
    key_len: usize = 0,
    bctx_buf: [64]u8 = [0; 64],
    bctx_len: usize = 0,
    frame_id_buf: [48]u8 = [0; 48],
    page_session: (*CdpPageSession)? = null,

    fn sessionId(): []const u8 {
        return _.key_buf[0.._.key_len];
    }

    fn browserContextId(): []const u8 {
        return _.bctx_buf[0.._.bctx_len];
    }
}

fn appendDialogJsonField(key: []const u8, val: []const u8, out_buf: []u8, pos: *usize) {
    const k_prefix = "\"";
    let i: usize = 0;
    while (i < k_prefix.len && pos.* < out_buf.len) : (i += 1) {
        out_buf[pos.*] = k_prefix[i];
        pos.* += 1;
    }
    i = 0;
    while (i < key.len && pos.* < out_buf.len) : (i += 1) {
        out_buf[pos.*] = key[i];
        pos.* += 1;
    }
    const mid = "\":\"";
    i = 0;
    while (i < mid.len && pos.* < out_buf.len) : (i += 1) {
        out_buf[pos.*] = mid[i];
        pos.* += 1;
    }
    CdpServer.appendEscapedJsonStr(val, out_buf, pos);
    if (pos.* < out_buf.len) {
        out_buf[pos.*] = '"';
        pos.* += 1;
    }
}

fn serverWorkerContextHook(
    server_ptr: *void,
    dom_ctx_ptr: (*void)?,
    out_pool: *(*DedicatedWorkerPool)?,
    out_registry: *(*TargetRegistry)?,
): bool {
    const srv: *CdpServer = unsafe { @ptrCast(@alignCast(server_ptr)) };
    if (dptr := dom_ctx_ptr) {
        if (dc := srv.session.dom_ctx) {
            const dc_ptr: *void = unsafe { @ptrCast(dc) };
            if (dc_ptr == dptr) {
                out_pool.* = &srv.session.dedicated_workers;
                out_registry.* = &srv.session.target_registry;
                return true;
            }
        }
        let ti: usize = 0;
        while (ti < MAX_TAB_SESSIONS) : (ti += 1) {
            if (srv.tab_sessions[ti].active) {
                if (ts := srv.tab_sessions[ti].page_session) {
                    if (dc := ts.dom_ctx) {
                        const dc_ptr: *void = unsafe { @ptrCast(dc) };
                        if (dc_ptr == dptr) {
                            out_pool.* = &ts.dedicated_workers;
                            out_registry.* = &ts.target_registry;
                            return true;
                        }
                    }
                }
            }
        }
    }
    out_pool.* = &srv.session.dedicated_workers;
    out_registry.* = &srv.session.target_registry;
    return true;
}

fn serverDialogHook(
    server_ptr: *void,
    dlg_type: []const u8,
    message: []const u8,
    default_prompt: []const u8,
    accepted_out: *bool,
    prompt_buf: []u8,
    prompt_len_out: *usize,
): bool {
    const srv: *CdpServer = unsafe { @ptrCast(@alignCast(server_ptr)) };
    const client_fd = srv.active_client_fd;
    if (client_fd < 0) return false;

    const sid = srv.activeDialogSessionId();
    if (srv.hasDeferredAbortForDialog(sid)) return false;
    let page_url: []const u8 = srv.session.url;
    if (sid.len > 0) {
        let ti: usize = 0;
        while (ti < MAX_TAB_SESSIONS) : (ti += 1) {
            if (srv.tab_sessions[ti].active && srv.tab_sessions[ti].page_session != null) {
                if (std.mem.eql(u8, srv.tab_sessions[ti].sessionId(), sid)) {
                    page_url = srv.tab_sessions[ti].page_session.?.url;
                    break;
                }
            }
        }
    }

    let open_params_buf = std.heap.c_allocator.alloc(u8, 8192) catch return false;
    defer std.heap.c_allocator.free(open_params_buf);
    let pos: usize = 0;
    open_params_buf[pos] = '{';
    pos += 1;
    appendDialogJsonField("url", page_url, open_params_buf, &pos);
    if (pos < open_params_buf.len) {
        open_params_buf[pos] = ',';
        pos += 1;
    }
    appendDialogJsonField("message", message, open_params_buf, &pos);
    if (pos < open_params_buf.len) {
        open_params_buf[pos] = ',';
        pos += 1;
    }
    appendDialogJsonField("type", dlg_type, open_params_buf, &pos);
    if (pos < open_params_buf.len) {
        open_params_buf[pos] = ',';
        pos += 1;
    }
    appendDialogJsonField("defaultPrompt", default_prompt, open_params_buf, &pos);
    const tail = ",\"hasBrowserHandler\":true}";
    let ti_s: usize = 0;
    while (ti_s < tail.len && pos < open_params_buf.len) : (ti_s += 1) {
        open_params_buf[pos] = tail[ti_s];
        pos += 1;
    }

    let open_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
    open_evts[0] = CdpEvent{
        method: "Page.javascriptDialogOpening",
        params_json: open_params_buf[0..pos],
        session_id: sid,
    };
    CdpServer.emitCdpEventsToFd(client_fd, &open_evts, 1) catch return false;

    const frame_payload = std.heap.c_allocator.alloc(u8, 65536) catch return false;
    defer std.heap.c_allocator.free(frame_payload);

    while (srv.running) {
        let pfd = PollFd{ fd: client_fd, events: POLLIN, revents: 0 };
        const pr = poll(&pfd, 1, 15);
        if (pr < 0) return false;
        if (pr == 0) continue;
        if ((pfd.revents & (POLLERR | POLLNVAL)) != 0) return false;

        const frame_opt = readWsTextFrame(client_fd, frame_payload) catch return false;
        const payload = frame_opt orelse return false;
        if (payload.len == 0) continue;

        const req = CdpRequest.parse(payload);
        if (std.mem.eql(u8, req.method, "Page.handleJavaScriptDialog")) {
            const dlg_accept = extractJsonBool(req.params_raw, "accept") ?? false;
            let ptext_buf: [4096]u8 = [0; 4096];
            let final_len: usize = 0;
            if (raw_pt := extractJsonString(req.params_raw, "promptText")) {
                const pt = unescapeJsonString(raw_pt, &ptext_buf);
                const n = @min(pt.len, prompt_buf.len);
                if (n > 0) @memcpy(prompt_buf[0..n], pt[0..n]);
                final_len = n;
            } else if (std.mem.eql(u8, dlg_type, "prompt") && dlg_accept) {
                const n = @min(default_prompt.len, prompt_buf.len);
                if (n > 0) @memcpy(prompt_buf[0..n], default_prompt[0..n]);
                final_len = n;
            }
            accepted_out.* = dlg_accept;
            prompt_len_out.* = final_len;

            let close_params_buf = std.heap.c_allocator.alloc(u8, 8192) catch return true;
            defer std.heap.c_allocator.free(close_params_buf);
            let cpos: usize = 0;
            const c_prefix = if (dlg_accept) "{\"result\":true,\"" else "{\"result\":false,\"";
            let ci: usize = 0;
            while (ci < c_prefix.len && cpos < close_params_buf.len) : (ci += 1) {
                close_params_buf[cpos] = c_prefix[ci];
                cpos += 1;
            }
            const u_key = "userInput\":\"";
            ci = 0;
            while (ci < u_key.len && cpos < close_params_buf.len) : (ci += 1) {
                close_params_buf[cpos] = u_key[ci];
                cpos += 1;
            }
            CdpServer.appendEscapedJsonStr(prompt_buf[0..final_len], close_params_buf, &cpos);
            if (cpos + 2 <= close_params_buf.len) {
                close_params_buf[cpos] = '"';
                cpos += 1;
                close_params_buf[cpos] = '}';
                cpos += 1;
            }

            let close_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            close_evts[0] = CdpEvent{
                method: "Page.javascriptDialogClosed",
                params_json: close_params_buf[0..cpos],
                session_id: sid,
            };
            CdpServer.emitCdpEventsToFd(client_fd, &close_evts, 1) catch {};

            let reply_buf: [512]u8 = [0; 512];
            const has_sid = req.session_id.len > 0;
            const reply_msg = if (req.raw_id.len > 0)
                (if (has_sid)
                    (std.fmt.bufPrint(&reply_buf, "{{\"id\":\"{s}\",\"result\":{{}},\"sessionId\":\"{s}\"}}", .{ req.raw_id, req.session_id }) catch "")
                else
                    (std.fmt.bufPrint(&reply_buf, "{{\"id\":\"{s}\",\"result\":{{}}}}", .{req.raw_id}) catch ""))
            else
                (if (has_sid)
                    (std.fmt.bufPrint(&reply_buf, "{{\"id\":{d},\"result\":{{}},\"sessionId\":\"{s}\"}}", .{ req.id, req.session_id }) catch "")
                else
                    (std.fmt.bufPrint(&reply_buf, "{{\"id\":{d},\"result\":{{}}}}", .{req.id}) catch ""));
            if (reply_msg.len > 0) {
                writeWsTextFrame(client_fd, reply_msg) catch {};
            }
            return true;
        }

        if (std.mem.eql(u8, req.method, "Browser.close") ||
            std.mem.eql(u8, req.method, "Target.closeTarget") ||
            std.mem.eql(u8, req.method, "Target.disposeBrowserContext"))
        {
            accepted_out.* = false;
            prompt_len_out.* = 0;
            srv.enqueueDeferredReq(payload);
            return false;
        }

        srv.enqueueDeferredReq(payload);
    }
    return false;
}

pub fn serverSyncFetchHook(
    server_ptr: *void,
    sess_ptr: *void,
    url: []const u8,
    method: []const u8,
    headers_json: []const u8,
    rtype: []const u8,
    net_id: []const u8,
    redirect_response_json: []const u8,
): bool {
    const srv: *CdpServer = unsafe { @ptrCast(@alignCast(server_ptr)) };
    const sess: *CdpPageSession = unsafe { @ptrCast(@alignCast(sess_ptr)) };
    const client_fd = srv.active_client_fd;
    if (client_fd < 0) return false;
    if (srv.hasDeferredAbortForSession(sess)) return false;
    const is_redirect = redirect_response_json.len > 0;
    const should_intercept = (sess.fetch_enabled || sess.fetch_state.enabled) && sess.fetch_state.shouldIntercept(url, rtype);
    if (!should_intercept && !(is_redirect && sess.network_enabled)) {
        return false;
    }

    let sid: []const u8 = "";
    if (sess != srv.session) {
        let ti: usize = 0;
        while (ti < MAX_TAB_SESSIONS) : (ti += 1) {
            if (srv.tab_sessions[ti].active && srv.tab_sessions[ti].page_session != null) {
                if (srv.tab_sessions[ti].page_session.? == sess) {
                    const cand = srv.tab_sessions[ti].sessionId();
                    if (cand.len > 0 && !std.mem.startsWith(u8, cand, "capsurf-cdp-session-")) {
                        sid = cand;
                    }
                    break;
                }
            }
        }
    } else if (sess.target_registry.targets[0].session_id_len > 0) {
        const cand = sess.target_registry.targets[0].sessionId();
        if (cand.len > 0 && !std.mem.startsWith(u8, cand, "capsurf-cdp-session-")) {
            sid = cand;
        }
    }

    let esc_url_buf: [2048]u8 = [0; 2048];
    const esc_url = escapeJsonString(url, &esc_url_buf);
    const hdr_obj = if (headers_json.len >= 2 && headers_json[0] == '{') headers_json else "{}";

    let evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
    let evt_count: usize = 0;
    let nev_buf: [16384]u8 = [0; 16384];
    let redir_kv_buf: [8192]u8 = [0; 8192];
    let redir_kv: []const u8 = "";
    if (is_redirect) {
        redir_kv = std.fmt.bufPrint(&redir_kv_buf, ",\"redirectResponse\":{s}", .{redirect_response_json}) catch "";
    }
    if (sess.network_enabled) {
        let esc_doc_buf: [2048]u8 = [0; 2048];
        const esc_doc = escapeJsonString(sess.url, &esc_doc_buf);
        const req_json = std.fmt.bufPrint(
            &nev_buf,
            "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"{s}\",\"headers\":{s},\"initialPriority\":\"High\",\"referrerPolicy\":\"no-referrer\"}}{s},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"parser\"}},\"type\":\"{s}\",\"frameId\":\"{s}\"}}",
            .{ net_id, sess.loader_id, esc_doc, esc_url, method, hdr_obj, redir_kv, rtype, sess.frame_id },
        ) catch "";
        if (req_json.len > 0) {
            evts[evt_count] = CdpEvent.create("Network.requestWillBeSent", req_json);
            if (sid.len > 0) evts[evt_count].session_id = sid;
            evt_count += 1;
        }
    }

    if (!should_intercept) {
        if (evt_count > 0) {
            CdpServer.emitCdpEventsToFd(client_fd, &evts, evt_count) catch return false;
        }
        return false;
    }

    let req_id_buf: [64]u8 = [0; 64];
    const req_id = if (is_redirect)
        (std.fmt.bufPrint(&req_id_buf, "fetch-redir-{d}", .{sess.fetch_state.next_id}) catch "fetch-redir-1")
    else
        (std.fmt.bufPrint(&req_id_buf, "fetch-sub-{d}", .{sess.fetch_state.next_id}) catch "fetch-sub-1");
    sess.fetch_state.next_id += 1;

    const parked_opt = sess.fetch_state.parkRequest(req_id, url, method);
    let parked = parked_opt orelse return false;
    const rt_n = @min(rtype.len, parked.resource_type_buf.len);
    if (rt_n > 0) @memcpy(parked.resource_type_buf[0..rt_n], rtype[0..rt_n]);
    parked.resource_type_len = rt_n;
    const nid_n = @min(net_id.len, parked.network_id_buf.len);
    if (nid_n > 0) @memcpy(parked.network_id_buf[0..nid_n], net_id[0..nid_n]);
    parked.network_id_len = nid_n;

    let fev_buf: [8192]u8 = [0; 8192];
    if (fev := FetchHandler.buildRequestPausedEventWithHeaders(req_id, esc_url, method, rtype, sess.frame_id, net_id, hdr_obj, &fev_buf)) {
        evts[evt_count] = fev;
        if (sid.len > 0) evts[evt_count].session_id = sid;
        evt_count += 1;
    }
    if (evt_count > 0) {
        CdpServer.emitCdpEventsToFd(client_fd, &evts, evt_count) catch return false;
    }

    const frame_payload = std.heap.c_allocator.alloc(u8, 1024 * 1024) catch return false;
    defer std.heap.c_allocator.free(frame_payload);

    while (srv.running) {
        let pfd = PollFd{ fd: client_fd, events: POLLIN, revents: 0 };
        const pr = poll(&pfd, 1, 15);
        if (pr < 0) return false;
        if (pr == 0) continue;
        if ((pfd.revents & (POLLERR | POLLNVAL)) != 0) return false;

        const frame_opt = readWsTextFrame(client_fd, frame_payload) catch return false;
        const payload = frame_opt orelse return false;
        if (payload.len == 0) continue;

        const req = CdpRequest.parse(payload);
        if (std.mem.eql(u8, req.method, "Fetch.continueRequest") ||
            std.mem.eql(u8, req.method, "Fetch.fulfillRequest") ||
            std.mem.eql(u8, req.method, "Fetch.failRequest") ||
            std.mem.eql(u8, req.method, "Fetch.disable"))
        {
            const target_sess = srv.resolveSessionForRequest(req);
            let res_buf: [4096]u8 = [0; 4096];
            let disp_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            let disp_evt_count: usize = 0;
            const resp = CdpDispatcher.dispatch(target_sess, req, &res_buf, &disp_evts, &disp_evt_count);

            let ei: usize = 0;
            while (ei < disp_evt_count) : (ei += 1) {
                if (disp_evts[ei].session_id.len == 0 && req.session_id.len > 0 &&
                    !std.mem.startsWith(u8, req.session_id, "capsurf-cdp-session-"))
                {
                    disp_evts[ei].session_id = req.session_id;
                }
            }
            if (disp_evt_count > 0) {
                CdpServer.emitCdpEventsToFd(client_fd, &disp_evts, disp_evt_count) catch {};
            }

            let reply_buf: [2048]u8 = [0; 2048];
            const has_sid = req.session_id.len > 0;
            let msg: []const u8 = "";
            if (resp.isOk()) {
                if (resp.raw_id.len > 0) {
                    if (has_sid) {
                        msg = std.fmt.bufPrint(&reply_buf, "{{\"id\":\"{s}\",\"result\":{s},\"sessionId\":\"{s}\"}}", .{ resp.raw_id, resp.result_json, req.session_id }) catch "";
                    } else {
                        msg = std.fmt.bufPrint(&reply_buf, "{{\"id\":\"{s}\",\"result\":{s}}}", .{ resp.raw_id, resp.result_json }) catch "";
                    }
                } else {
                    if (has_sid) {
                        msg = std.fmt.bufPrint(&reply_buf, "{{\"id\":{d},\"result\":{s},\"sessionId\":\"{s}\"}}", .{ resp.id, resp.result_json, req.session_id }) catch "";
                    } else {
                        msg = std.fmt.bufPrint(&reply_buf, "{{\"id\":{d},\"result\":{s}}}", .{ resp.id, resp.result_json }) catch "";
                    }
                }
            } else {
                if (resp.raw_id.len > 0) {
                    if (has_sid) {
                        msg = std.fmt.bufPrint(&reply_buf, "{{\"id\":\"{s}\",\"error\":{{\"code\":{d},\"message\":\"{s}\"}},\"sessionId\":\"{s}\"}}", .{ resp.raw_id, resp.error_code, resp.error_message, req.session_id }) catch "";
                    } else {
                        msg = std.fmt.bufPrint(&reply_buf, "{{\"id\":\"{s}\",\"error\":{{\"code\":{d},\"message\":\"{s}\"}}}}", .{ resp.raw_id, resp.error_code, resp.error_message }) catch "";
                    }
                } else {
                    if (has_sid) {
                        msg = std.fmt.bufPrint(&reply_buf, "{{\"id\":{d},\"error\":{{\"code\":{d},\"message\":\"{s}\"}},\"sessionId\":\"{s}\"}}", .{ resp.id, resp.error_code, resp.error_message, req.session_id }) catch "";
                    } else {
                        msg = std.fmt.bufPrint(&reply_buf, "{{\"id\":{d},\"error\":{{\"code\":{d},\"message\":\"{s}\"}}}}", .{ resp.id, resp.error_code, resp.error_message }) catch "";
                    }
                }
            }
            if (msg.len > 0) {
                writeWsTextFrame(client_fd, msg) catch {};
            }
            if (std.mem.eql(u8, req.method, "Fetch.disable") || parked.verdict != InterceptVerdict.None) {
                return true;
            }
            continue;
        }

        if (std.mem.eql(u8, req.method, "Browser.close") ||
            std.mem.eql(u8, req.method, "Target.closeTarget") ||
            std.mem.eql(u8, req.method, "Target.disposeBrowserContext"))
        {
            srv.enqueueDeferredReq(payload);
            return false;
        }

        srv.enqueueDeferredReq(payload);
    }
    return false;
}

pub struct CdpServer {
    allocator: std.mem.Allocator,
    port: u16,
    listen_fd: i32 = -1,
    session: *CdpPageSession,
    tab_sessions: [MAX_TAB_SESSIONS]TabSessionSlot = [TabSessionSlot{}; MAX_TAB_SESSIONS],
    running: bool = false,
    active_client_fd: i32 = -1,
    active_dialog_session_id_buf: [128]u8 = [0; 128],
    active_dialog_session_id_len: usize = 0,
    deferred_reqs: [32][4096]u8 = [[0; 4096]; 32],
    deferred_req_lens: [32]usize = [0; 32],
    deferred_req_count: usize = 0,
    net_scheduler: NetScheduler = NetScheduler.init(),
    net_idle_tracker: NetworkIdleTracker = NetworkIdleTracker.init(),

    fn setActiveDialogSessionId(*_, sid: []const u8) {
        const n = @min(sid.len, _.active_dialog_session_id_buf.len);
        if (n > 0) @memcpy(_.active_dialog_session_id_buf[0..n], sid[0..n]);
        _.active_dialog_session_id_len = n;
    }

    fn activeDialogSessionId(): []const u8 {
        return _.active_dialog_session_id_buf[0.._.active_dialog_session_id_len];
    }

    fn enqueueDeferredReq(*_, payload: []const u8) {
        if (_.deferred_req_count >= _.deferred_reqs.len) return;
        const idx = _.deferred_req_count;
        const n = @min(payload.len, _.deferred_reqs[idx].len);
        if (n > 0) @memcpy(_.deferred_reqs[idx][0..n], payload[0..n]);
        _.deferred_req_lens[idx] = n;
        _.deferred_req_count += 1;
    }

    pub static fn init(allocator: std.mem.Allocator, port: u16): (*CdpServer)! {
        const page_session = CdpPageSession.init(allocator) catch return error.SessionInitFailed;
        let ptr = std.heap.c_allocator.create(CdpServer) catch return error.AllocFailed;
        ptr.* = CdpServer {
            allocator: allocator,
            port: port,
            listen_fd: -1,
            session: page_session,
            tab_sessions: [TabSessionSlot{}; MAX_TAB_SESSIONS],
            running: false,
        };
        ptr.session.setSyncFetchHook(unsafe { @ptrCast(ptr) }, &serverSyncFetchHook);
        return ptr;
    }

    pub fn deinit(*_) {
        _.stop();
        _.net_scheduler.abortAll();
        _.net_idle_tracker.resetForNavigation();
        _.deinitTabSessions();
        _.session.deinit();
        std.heap.c_allocator.destroy(_);
    }

    fn deinitTabSessions(*_) {
        let i: usize = 0;
        while (i < MAX_TAB_SESSIONS) : (i += 1) {
            if (_.tab_sessions[i].active) {
                if (_.tab_sessions[i].page_session != null) {
                    _.tab_sessions[i].page_session.?.deinit();
                }
                _.tab_sessions[i] = TabSessionSlot{};
            }
        }
    }

    fn deinitTabSessionByKey(*_, sid: []const u8) {
        if (sid.len == 0) return;
        let i: usize = 0;
        while (i < MAX_TAB_SESSIONS) : (i += 1) {
            if (_.tab_sessions[i].active && _.tab_sessions[i].key_len > 0 && std.mem.eql(u8, _.tab_sessions[i].key_buf[0.._.tab_sessions[i].key_len], sid)) {
                if (_.tab_sessions[i].page_session != null) {
                    _.tab_sessions[i].page_session.?.deinit();
                }
                _.tab_sessions[i] = TabSessionSlot{};
            }
        }
    }

    fn deinitTabSessionMatch(*_, tid: []const u8, sid: []const u8, sess_ptr: (*CdpPageSession)?) {
        let i: usize = 0;
        while (i < MAX_TAB_SESSIONS) : (i += 1) {
            if (!_.tab_sessions[i].active) continue;
            let matched = false;
            if (sid.len > 0 && _.tab_sessions[i].key_len > 0 && std.mem.eql(u8, _.tab_sessions[i].key_buf[0.._.tab_sessions[i].key_len], sid)) {
                matched = true;
            }
            if (!matched) {
                if (sp := sess_ptr) {
                    if (ps := _.tab_sessions[i].page_session) {
                        if (ps == sp) {
                            matched = true;
                        }
                    }
                }
            }
            if (!matched && tid.len > 0) {
                if (ps := _.tab_sessions[i].page_session) {
                    if ((ps.target_id.len > 0 && std.mem.eql(u8, ps.target_id, tid)) ||
                        (ps.frame_id.len > 0 && std.mem.eql(u8, ps.frame_id, tid))) {
                        matched = true;
                    }
                }
            }
            if (matched) {
                if (_.tab_sessions[i].page_session != null) {
                    _.tab_sessions[i].page_session.?.deinit();
                }
                _.tab_sessions[i] = TabSessionSlot{};
            }
        }
    }

    fn sessionBelongsToContext(*_, sid: []const u8, bctx: []const u8): bool {
        if (sid.len == 0 || bctx.len == 0) return false;
        if (tid := _.session.target_registry.findTargetIdBySessionId(sid)) {
            if (t_idx := _.session.target_registry.findTargetIndex(tid)) {
                if (std.mem.eql(u8, _.session.target_registry.targets[t_idx].browserContextId(), bctx)) {
                    return true;
                }
            }
        }
        let i: usize = 0;
        while (i < MAX_TAB_SESSIONS) : (i += 1) {
            if (_.tab_sessions[i].active &&
                std.mem.eql(u8, _.tab_sessions[i].sessionId(), sid) &&
                std.mem.eql(u8, _.tab_sessions[i].browserContextId(), bctx))
            {
                return true;
            }
        }
        return false;
    }

    fn targetBelongsToContext(*_, tid: []const u8, bctx: []const u8): bool {
        if (tid.len == 0 || bctx.len == 0) return false;
        if (t_idx := _.session.target_registry.findTargetIndex(tid)) {
            return std.mem.eql(u8, _.session.target_registry.targets[t_idx].browserContextId(), bctx);
        }
        return false;
    }

    fn hasActiveTabSessions(*_): bool {
        let i: usize = 0;
        while (i < MAX_TAB_SESSIONS) : (i += 1) {
            if (_.tab_sessions[i].active) return true;
        }
        return false;
    }

    fn hasDeferredAbortForSession(*_, sess: *CdpPageSession): bool {
        if (_.deferred_req_count == 0) return false;
        const sess_bctx = sess.current_browser_context_id_buf[0..sess.current_browser_context_id_len];
        let i: usize = 0;
        while (i < _.deferred_req_count) : (i += 1) {
            const dlen = _.deferred_req_lens[i];
            if (dlen == 0) continue;
            const dreq = CdpRequest.parse(_.deferred_reqs[i][0..dlen]);
            if (std.mem.eql(u8, dreq.method, "Browser.close")) return true;
            if (std.mem.eql(u8, dreq.method, "Target.disposeBrowserContext")) {
                const dc_id = extractJsonString(dreq.params_raw, "browserContextId") ?? "";
                if (dc_id.len > 0 && sess_bctx.len > 0 && std.mem.eql(u8, dc_id, sess_bctx)) {
                    return true;
                }
            }
            if (std.mem.eql(u8, dreq.method, "Target.closeTarget")) {
                const ct_id = extractJsonString(dreq.params_raw, "targetId") ?? "";
                if (ct_id.len > 0 && sess.target_id.len > 0 && std.mem.eql(u8, ct_id, sess.target_id)) {
                    return true;
                }
            }
        }
        return false;
    }

    fn hasDeferredAbortForDialog(*_, sid: []const u8): bool {
        if (_.deferred_req_count == 0) return false;
        let i: usize = 0;
        while (i < _.deferred_req_count) : (i += 1) {
            const dlen = _.deferred_req_lens[i];
            if (dlen == 0) continue;
            const dreq = CdpRequest.parse(_.deferred_reqs[i][0..dlen]);
            if (std.mem.eql(u8, dreq.method, "Browser.close")) return true;
            if (std.mem.eql(u8, dreq.method, "Target.disposeBrowserContext")) {
                const dc_id = extractJsonString(dreq.params_raw, "browserContextId") ?? "";
                if (dc_id.len > 0 && _.sessionBelongsToContext(sid, dc_id)) {
                    return true;
                }
            }
            if (std.mem.eql(u8, dreq.method, "Target.closeTarget")) {
                const ct_id = extractJsonString(dreq.params_raw, "targetId") ?? "";
                if (ct_id.len > 0 && sid.len > 0) {
                    if (tid := _.session.target_registry.findTargetIdBySessionId(sid)) {
                        if (std.mem.eql(u8, tid, ct_id)) return true;
                    }
                }
            }
        }
        return false;
    }

    static fn sendCdpErrorById(client_fd: i32, req_id: i64, raw_id: []const u8, session_id: []const u8, code: i32, message: []const u8) {
        let err_buf: [512]u8 = [0; 512];
        const has_sid = session_id.len > 0;
        const msg = if (raw_id.len > 0)
            (if (has_sid)
                (std.fmt.bufPrint(&err_buf, "{{\"id\":\"{s}\",\"error\":{{\"code\":{d},\"message\":\"{s}\"}},\"sessionId\":\"{s}\"}}", .{ raw_id, code, message, session_id }) catch "")
            else
                (std.fmt.bufPrint(&err_buf, "{{\"id\":\"{s}\",\"error\":{{\"code\":{d},\"message\":\"{s}\"}}}}", .{ raw_id, code, message }) catch ""))
        else
            (if (has_sid)
                (std.fmt.bufPrint(&err_buf, "{{\"id\":{d},\"error\":{{\"code\":{d},\"message\":\"{s}\"}},\"sessionId\":\"{s}\"}}", .{ req_id, code, message, session_id }) catch "")
            else
                (std.fmt.bufPrint(&err_buf, "{{\"id\":{d},\"error\":{{\"code\":{d},\"message\":\"{s}\"}}}}", .{ req_id, code, message }) catch ""));
        if (msg.len > 0) {
            writeWsTextFrame(client_fd, msg) catch {};
        }
    }

    fn deinitTabSessionByContext(*_, bctx: []const u8) {
        if (bctx.len == 0) return;
        let i: usize = 0;
        while (i < MAX_TAB_SESSIONS) : (i += 1) {
            if (!_.tab_sessions[i].active) continue;
            const match_bctx = _.tab_sessions[i].bctx_len > 0 && std.mem.eql(u8, _.tab_sessions[i].browserContextId(), bctx);
            const match_sid = !match_bctx && _.sessionBelongsToContext(_.tab_sessions[i].sessionId(), bctx);
            if (match_bctx || match_sid) {
                if (_.tab_sessions[i].page_session != null) {
                    _.tab_sessions[i].page_session.?.deinit();
                }
                _.tab_sessions[i] = TabSessionSlot{};
            }
        }
    }

    fn getOrCreateTabSession(*_, sid: []const u8, bctx: []const u8): *CdpPageSession {
        let i: usize = 0;
        while (i < MAX_TAB_SESSIONS) : (i += 1) {
            if (!_.tab_sessions[i].active) continue;
            if (sid.len > 0 && _.tab_sessions[i].key_len > 0 && std.mem.eql(u8, _.tab_sessions[i].key_buf[0.._.tab_sessions[i].key_len], sid)) {
                if (_.tab_sessions[i].page_session != null) return _.tab_sessions[i].page_session.?;
            }
            if (sid.len == 0 && bctx.len > 0 && _.tab_sessions[i].bctx_len > 0 && std.mem.eql(u8, _.tab_sessions[i].bctx_buf[0.._.tab_sessions[i].bctx_len], bctx)) {
                if (_.tab_sessions[i].page_session != null) return _.tab_sessions[i].page_session.?;
            }
        }
        if (sid.len > 0 && bctx.len > 0) {
            let j: usize = 0;
            while (j < MAX_TAB_SESSIONS) : (j += 1) {
                if (!_.tab_sessions[j].active) continue;
                if (_.tab_sessions[j].key_len == 0 && _.tab_sessions[j].bctx_len > 0 && std.mem.eql(u8, _.tab_sessions[j].bctx_buf[0.._.tab_sessions[j].bctx_len], bctx)) {
                    const slen = @min(sid.len, _.tab_sessions[j].key_buf.len);
                    @memcpy(_.tab_sessions[j].key_buf[0..slen], sid[0..slen]);
                    _.tab_sessions[j].key_len = slen;
                    if (_.tab_sessions[j].page_session != null) {
                        if (tid := _.session.target_registry.findTargetIdBySessionId(sid)) {
                            const flen = @min(tid.len, _.tab_sessions[j].frame_id_buf.len);
                            @memcpy(_.tab_sessions[j].frame_id_buf[0..flen], tid[0..flen]);
                            const fid_slice = _.tab_sessions[j].frame_id_buf[0..flen];
                            _.tab_sessions[j].page_session.?.frame_id = fid_slice;
                            if (dctx := _.tab_sessions[j].page_session.?.dom_ctx) {
                                const dlen = @min(fid_slice.len, dctx.frame_id_buf.len);
                                @memcpy(dctx.frame_id_buf[0..dlen], fid_slice[0..dlen]);
                                dctx.frame_id_len = dlen;
                            }
                        }
                        return _.tab_sessions[j].page_session.?;
                    }
                }
            }
        }
        let slot_idx: usize = 0;
        let found_slot = false;
        while (slot_idx < MAX_TAB_SESSIONS) : (slot_idx += 1) {
            if (!_.tab_sessions[slot_idx].active) {
                found_slot = true;
                break;
            }
        }
        if (!found_slot) return _.session;

        const new_sess = CdpPageSession.init(_.allocator) catch return _.session;
        new_sess.setSyncFetchHook(unsafe { @ptrCast(_) }, &serverSyncFetchHook);
        new_sess.skip_raster = _.session.skip_raster;
        new_sess.viewport_width = _.session.viewport_width;
        new_sess.viewport_height = _.session.viewport_height;
        new_sess.layout.viewport_width = _.session.viewport_width;
        new_sess.layout.viewport_height = _.session.viewport_height;
        new_sess.device_scale_factor = _.session.device_scale_factor;
        new_sess.is_mobile = _.session.is_mobile;
        if (new_sess.viewport_width != 1280.0 || new_sess.viewport_height != 720.0) {
            new_sess.ensureLayout();
        }
        new_sess.owner_target_registry = &_.session.target_registry;
        new_sess.owner_cookie_jar = &_.session.cookie_jar;
        if (_.session.binding_count > 0) {
            let bi: usize = 0;
            while (bi < _.session.binding_count) : (bi += 1) {
                const blen = _.session.binding_name_lens[bi];
                const boff = bi * 128;
                if (blen > 0) {
                    new_sess.addBinding(_.session.binding_names[boff .. boff + blen]);
                }
            }
        }
        if (_.session.fetch_enabled) {
            new_sess.fetch_enabled = true;
            new_sess.fetch_state = _.session.fetch_state;
        }
        if (bctx.len > 0) {
            new_sess.setContextId(bctx);
        }
        _.tab_sessions[slot_idx] = TabSessionSlot{};
        if (sid.len > 0) {
            if (tid := _.session.target_registry.findTargetIdBySessionId(sid)) {
                const flen = @min(tid.len, _.tab_sessions[slot_idx].frame_id_buf.len);
                @memcpy(_.tab_sessions[slot_idx].frame_id_buf[0..flen], tid[0..flen]);
                new_sess.frame_id = _.tab_sessions[slot_idx].frame_id_buf[0..flen];
                new_sess.target_id = new_sess.frame_id;
                if (dctx := new_sess.dom_ctx) {
                    const dlen = @min(new_sess.frame_id.len, dctx.frame_id_buf.len);
                    @memcpy(dctx.frame_id_buf[0..dlen], new_sess.frame_id[0..dlen]);
                    dctx.frame_id_len = dlen;
                }
                if (t_idx := _.session.target_registry.findTargetIndex(tid)) {
                    const tgt = &_.session.target_registry.targets[t_idx];
                    if (tgt.opener_id_len > 0 && std.mem.eql(u8, tgt.url(), "about:blank")) {
                        new_sess.is_about_blank_popup = true;
                    } else if (tgt.opener_id_len > 0 && tgt.url().len > 0) {
                        const pu = tgt.url();
                        const plen = @min(pu.len, new_sess.pending_popup_nav_url_buf.len);
                        @memcpy(new_sess.pending_popup_nav_url_buf[0..plen], pu[0..plen]);
                        new_sess.pending_popup_nav_url_len = plen;
                    }
                    if (tgt.opener_id_len > 0 && !tgt.popup_noopener) {
                        new_sess.has_opener = true;
                        const op_v = new_sess.ctx.eval("window.opener = window.opener || { closed: false };");
                        new_sess.ctx.freeValue(op_v);
                    }
                }
            } else {
                new_sess.frame_id = std.fmt.bufPrint(&_.tab_sessions[slot_idx].frame_id_buf, "capsurf-frame-tab-{d}", .{slot_idx + 2}) catch "capsurf-frame-001";
                if (dctx := new_sess.dom_ctx) {
                    const dlen = @min(new_sess.frame_id.len, dctx.frame_id_buf.len);
                    @memcpy(dctx.frame_id_buf[0..dlen], new_sess.frame_id[0..dlen]);
                    dctx.frame_id_len = dlen;
                }
            }
        } else {
            new_sess.frame_id = std.fmt.bufPrint(&_.tab_sessions[slot_idx].frame_id_buf, "capsurf-frame-tab-{d}", .{slot_idx + 2}) catch "capsurf-frame-001";
            if (dctx := new_sess.dom_ctx) {
                const dlen = @min(new_sess.frame_id.len, dctx.frame_id_buf.len);
                @memcpy(dctx.frame_id_buf[0..dlen], new_sess.frame_id[0..dlen]);
                dctx.frame_id_len = dlen;
            }
        }
        _.tab_sessions[slot_idx].active = true;
        if (sid.len > 0) {
            const slen = @min(sid.len, _.tab_sessions[slot_idx].key_buf.len);
            @memcpy(_.tab_sessions[slot_idx].key_buf[0..slen], sid[0..slen]);
            _.tab_sessions[slot_idx].key_len = slen;
        }
        if (bctx.len > 0) {
            const blen = @min(bctx.len, _.tab_sessions[slot_idx].bctx_buf.len);
            @memcpy(_.tab_sessions[slot_idx].bctx_buf[0..blen], bctx[0..blen]);
            _.tab_sessions[slot_idx].bctx_len = blen;
        }
        _.tab_sessions[slot_idx].page_session = new_sess;
        return _.tab_sessions[slot_idx].page_session.?;
    }

    fn resolveSessionForRequest(*_, req: CdpRequest): *CdpPageSession {
        if (std.mem.startsWith(u8, req.method, "Target.") || std.mem.startsWith(u8, req.method, "Browser.")) {
            return _.session;
        }
        const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
        if (req.session_id.len > 0) {
            if (std.mem.startsWith(u8, req.session_id, "capsurf-cdp-session-")) {
                if (bctx.len > 0 && !std.mem.eql(u8, bctx, "default")) {
                    return _.getOrCreateTabSession("", bctx);
                }
                if (!_.session.has_document || _.session.url.len == 0 || std.mem.eql(u8, _.session.url, "about:blank")) {
                    if (std.mem.eql(u8, req.method, "Page.captureScreenshot") ||
                        std.mem.eql(u8, req.method, "Kitesurf.getRendererInfo") ||
                        std.mem.eql(u8, req.method, "CapSurf.getRendererInfo")) {
                        let ti: usize = MAX_TAB_SESSIONS;
                        while (ti > 0) {
                            ti -= 1;
                            if (_.tab_sessions[ti].active) {
                                if (ts := _.tab_sessions[ti].page_session) {
                                    if (ts.has_document && ts.url.len > 0 && !std.mem.eql(u8, ts.url, "about:blank")) {
                                        return ts;
                                    }
                                }
                            }
                        }
                    }
                }
                return _.session;
            }
            if (std.mem.eql(u8, req.session_id, "capsurf-session-renderer") ||
                std.mem.eql(u8, req.session_id, "capsurf-session-engine") ||
                std.mem.eql(u8, req.session_id, "capsurf-session-001")) {
                return _.session;
            }
            if (_.session.target_registry.findFrameIdBySessionId(req.session_id) != null) {
                return _.session;
            }
            if (_.session.dedicated_workers.findWorkerBySessionId(req.session_id) != null) {
                return _.session;
            }
            {
                let wi: usize = 0;
                while (wi < MAX_TAB_SESSIONS) : (wi += 1) {
                    if (_.tab_sessions[wi].active) {
                        if (ts := _.tab_sessions[wi].page_session) {
                            if (ts.dedicated_workers.findWorkerBySessionId(req.session_id) != null) {
                                return ts;
                            }
                        }
                    }
                }
            }
            let resolved_bctx: []const u8 = bctx;
            if (resolved_bctx.len == 0) {
                if (tid := _.session.target_registry.findTargetIdBySessionId(req.session_id)) {
                    if (t_idx := _.session.target_registry.findTargetIndex(tid)) {
                        const tbctx = _.session.target_registry.targets[t_idx].browserContextId();
                        if (!std.mem.eql(u8, tbctx, "default")) {
                            resolved_bctx = tbctx;
                        }
                    }
                }
            }
            return _.getOrCreateTabSession(req.session_id, resolved_bctx);
        }
        if (bctx.len > 0 && !std.mem.eql(u8, bctx, "default")) {
            return _.getOrCreateTabSession("", bctx);
        }
        if (!_.session.has_document || _.session.url.len == 0 || std.mem.eql(u8, _.session.url, "about:blank")) {
            if (std.mem.eql(u8, req.method, "Page.captureScreenshot") ||
                std.mem.eql(u8, req.method, "Kitesurf.getRendererInfo") ||
                std.mem.eql(u8, req.method, "CapSurf.getRendererInfo")) {
                let ti: usize = MAX_TAB_SESSIONS;
                while (ti > 0) {
                    ti -= 1;
                    if (_.tab_sessions[ti].active) {
                        if (ts := _.tab_sessions[ti].page_session) {
                            if (ts.has_document && ts.url.len > 0 && !std.mem.eql(u8, ts.url, "about:blank")) {
                                return ts;
                            }
                        }
                    }
                }
            }
        }
        return _.session;
    }

    pub fn start(*_)! {
        const fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return error.SocketCreationFailed;

        const one: i32 = 1;
        * = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, @sizeOf(i32));

        const addr = SockAddrIn {
            sin_family: @as(u16, @intCast(AF_INET)),
            sin_port: std.mem.nativeToBig(u16, _.port),
            sin_addr: 0x0100007F, // 127.0.0.1
            sin_zero: [0; 8],
        };

        if (bind(fd, &addr, @sizeOf(SockAddrIn)) < 0) {
            * = std.c.close(fd);
            return error.BindFailed;
        }

        if (listen(fd, 128) < 0) {
            * = std.c.close(fd);
            return error.ListenFailed;
        }

        _.listen_fd = fd;
        _.running = true;
    }

    pub fn stop(*_) {
        if (!_.running) return;
        _.running = false;
        const port = _.port;
        const fd = _.listen_fd;
        _.listen_fd = -1;

        if (fd >= 0) {
            * = shutdown(fd, 2);
            * = std.c.close(fd);
        }
        wakeServer(port);
    }

    pub fn getPort(*_): u16 {
        return _.port;
    }

    /// Primary accept loop: runs and dispatches connections until stopped.
    pub fn runAcceptLoop(*_)! {
        while (_.running) {
            const client_fd = accept(_.listen_fd, null, null);
            if (client_fd < 0) {
                if (!_.running) break;
                continue;
            }
            if (!_.running) {
                * = std.c.close(client_fd);
                break;
            }
            setTcpNoDelay(client_fd);
            _.handleClient(client_fd) catch {};
        }
    }

fn findHeaderCaseInsensitive(headers: []const u8, name: []const u8): usize? {
    if (headers.len < name.len) return null;
    let i: usize = 0;
    while (i + name.len <= headers.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(headers[i..i + name.len], name)) {
            return i;
        }
    }
    return null;
}

    /// Handles a single incoming client connection (HTTP Discovery or WebSocket Session).
    pub fn handleClient(*_, client_fd: i32)! {
        defer {
            * = std.c.close(client_fd);
        }

        let req_buf: [4096]u8 = [0; 4096];
        const n = recv(client_fd, &req_buf, req_buf.len, 0);
        if (n <= 0) return;
        const req_str = req_buf[0..@as(usize, @intCast(n))];

        // Check if WebSocket Upgrade request
        if (findHeaderCaseInsensitive(req_str, "upgrade: websocket") != null or
            (findHeaderCaseInsensitive(req_str, "upgrade") != null and findHeaderCaseInsensitive(req_str, "websocket") != null))
        {
            _.handleWebSocketUpgrade(client_fd, req_str)? ;
            return;
        }

        // Handle HTTP Discovery Endpoints
        _.handleHttpRequest(client_fd, req_str)? ;
    }

    fn handleHttpRequest(*_, client_fd: i32, req: []const u8)! {
        // Parse request line: METHOD TARGET HTTP/1.x
        let req_line = req;
        if (crlf := std.mem.indexOf(u8, req, "\r\n")) {
            req_line = req[0..crlf];
        }
        let target: []const u8 = "/";
        if (sp1 := std.mem.indexOfScalar(u8, req_line, ' ')) {
            const after_m = req_line[sp1 + 1..];
            if (sp2 := std.mem.indexOfScalar(u8, after_m, ' ')) {
                target = after_m[0..sp2];
            } else {
                target = after_m;
            }
        }
        let path = target;
        let query: []const u8 = "";
        if (q_idx := std.mem.indexOfScalar(u8, target, '?')) {
            path = target[0..q_idx];
            query = target[q_idx + 1..];
        }

        // Determine host, proto & inspector base from headers (fallback to 127.0.0.1:<port> & ws)
        let host_buf: [256]u8 = [0; 256];
        const default_host = std.fmt.bufPrint(&host_buf, "127.0.0.1:{d}", .{_.getPort()}) catch "127.0.0.1";
        let req_host = CdpServer.extractHttpHeader(req, "X-Forwarded-Host") ?? (CdpServer.extractHttpHeader(req, "Host") ?? default_host);
        if (req_host.len == 0) req_host = default_host;
        let http_scheme: []const u8 = "http";
        let ws_scheme: []const u8 = "ws";
        if (proto := CdpServer.extractHttpHeader(req, "X-Forwarded-Proto")) {
            if (std.ascii.eqlIgnoreCase(proto, "https")) {
                http_scheme = "https";
                ws_scheme = "wss";
            }
        }
        const inspector_base = CdpServer.extractHttpHeader(req, "X-Inspector-Base") ?? "https://example.com/devtools";
        const no_inspector = inspector_base.len == 0 || std.ascii.eqlIgnoreCase(inspector_base, "none");
        const path_only_inspector = !no_inspector && inspector_base[0] == '/';

        // 1. /health
        if (std.mem.eql(u8, path, "/health")) {
            sendHttpResponse(client_fd, "200 OK", "text/plain; charset=utf-8", "kitesurf: OK\n")?;
            return;
        }

        // 1b. /rbi/scene_host.json
        if (std.mem.eql(u8, path, "/rbi/scene_host.json")) {
            let m_buf: [512]u8 = [0; 512];
            const manifest = SceneHostManifest{
                sha256: "0000000000000000000000000000000000000000000000000000000000000000",
                total_bytes: 1024,
                retained_frames: true,
            };
            const m_json = formatManifestJson(manifest, &m_buf);
            sendHttpResponse(client_fd, "200 OK", "application/json; charset=utf-8", m_json)?;
            return;
        }

        // 2. /json/protocol (must be matched before /json)
        if (std.mem.eql(u8, path, "/json/protocol") || std.mem.eql(u8, path, "/json/protocol/")) {
            sendHttpResponse(client_fd, "200 OK", "application/json; charset=UTF-8", "{}")?;
            return;
        }

        // 3. /json/version and /json/version/
        if (std.mem.eql(u8, path, "/json/version") || std.mem.eql(u8, path, "/json/version/")) {
            let body_buf: [1024]u8 = [0; 1024];
            const body = std.fmt.bufPrint(
                &body_buf,
                "{{\n" ++
                "  \"Browser\": \"CapSurf/1.0.0\",\n" ++
                "  \"Protocol-Version\": \"1.3\",\n" ++
                "  \"User-Agent\": \"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 CapSurf/1.0\",\n" ++
                "  \"V8-Version\": \"QuickJS-NG\",\n" ++
                "  \"webSocketDebuggerUrl\": \"{s}://{s}/devtools/browser\"\n" ++
                "}}",
                .{ ws_scheme, req_host },
            ) catch "{}";
            sendHttpResponse(client_fd, "200 OK", "application/json; charset=UTF-8", body)?;
            return;
        }

        // 4. /json, /json/list, /json/list/
        if (std.mem.eql(u8, path, "/json") || std.mem.eql(u8, path, "/json/list") || std.mem.eql(u8, path, "/json/list/")) {
            let fe_buf: [1024]u8 = [0; 1024];
            let fe_url: []const u8 = "";
            if (no_inspector) {
                fe_url = std.fmt.bufPrint(&fe_buf, "devtools://devtools/bundled/inspector.html?{s}={s}/devtools/page/kitesurf", .{ ws_scheme, req_host }) catch "";
            } else {
                fe_url = std.fmt.bufPrint(&fe_buf, "{s}?{s}={s}/devtools/page/kitesurf", .{ inspector_base, ws_scheme, req_host }) catch "";
            }
            let body_buf: [2048]u8 = [0; 2048];
            const body = std.fmt.bufPrint(
                &body_buf,
                "[{{\n" ++
                "  \"description\": \"\",\n" ++
                "  \"devtoolsFrontendUrl\": \"{s}\",\n" ++
                "  \"id\": \"kitesurf\",\n" ++
                "  \"title\": \"{s}\",\n" ++
                "  \"type\": \"page\",\n" ++
                "  \"url\": \"{s}\",\n" ++
                "  \"webSocketDebuggerUrl\": \"{s}://{s}/devtools/page/kitesurf\"\n" ++
                "}}]",
                .{ fe_url, _.session.title, _.session.url, ws_scheme, req_host },
            ) catch "[]";
            sendHttpResponse(client_fd, "200 OK", "application/json; charset=UTF-8", body)?;
            return;
        }

        // 5. /json/new
        if (std.mem.startsWith(u8, path, "/json/new")) {
            let body_buf: [1024]u8 = [0; 1024];
            const body = std.fmt.bufPrint(
                &body_buf,
                "{{\n" ++
                "  \"description\": \"\",\n" ++
                "  \"devtoolsFrontendUrl\": \"\",\n" ++
                "  \"id\": \"capsurf-target-001\",\n" ++
                "  \"title\": \"{s}\",\n" ++
                "  \"type\": \"page\",\n" ++
                "  \"url\": \"{s}\",\n" ++
                "  \"webSocketDebuggerUrl\": \"{s}://{s}/devtools/page/capsurf-target-001\"\n" ++
                "}}",
                .{ _.session.title, _.session.url, ws_scheme, req_host },
            ) catch "{}";
            sendHttpResponse(client_fd, "200 OK", "application/json; charset=UTF-8", body)?;
            return;
        }

        if (std.mem.startsWith(u8, path, "/json/activate") || std.mem.startsWith(u8, path, "/json/close")) {
            sendHttpResponse(client_fd, "200 OK", "text/plain", "Target handled")?;
            return;
        }

        // 6. /?inspect=1 or /?url=http(s)://...
        if (std.mem.eql(u8, path, "/") && query.len > 0) {
            const has_inspect = std.mem.indexOf(u8, query, "inspect=1") != null;
            const url_param = MockFixtures.extractQueryParam(target, "url");
            let dec_url_buf: [1024]u8 = undefined;
            let is_http_url = false;
            if (url_param != null && url_param.?.len > 0) {
                const du = MockFixtures.decodeUri(url_param.?, &dec_url_buf);
                is_http_url = std.ascii.startsWithIgnoreCase(du, "http://") || std.ascii.startsWithIgnoreCase(du, "https://");
            }
            if (has_inspect || is_http_url) {
                if (no_inspector) {
                    sendHttpResponse(client_fd, "501 Not Implemented", "text/plain", "Inspector base URL not configured")?;
                    return;
                }
                let base_buf: [512]u8 = [0; 512];
                const resolved_base = if (path_only_inspector)
                    (std.fmt.bufPrint(&base_buf, "{s}://{s}{s}", .{ http_scheme, req_host, inspector_base }) catch inspector_base)
                else
                    inspector_base;
                let loc_buf: [2048]u8 = [0; 2048];
                let loc: []const u8 = "";
                if (is_http_url) {
                    let dec_buf: [1024]u8 = undefined;
                    const decoded_url = MockFixtures.decodeUri(url_param.?, &dec_buf);
                    let enc_inner_buf: [1536]u8 = undefined;
                    const inner_encoded = encodeUriComponent(decoded_url, &enc_inner_buf);
                    let ws_target_buf: [1800]u8 = undefined;
                    const ws_target = std.fmt.bufPrint(
                        &ws_target_buf,
                        "{s}/devtools/page/kitesurf?initialUrl={s}",
                        .{ req_host, inner_encoded },
                    ) catch "";
                    let enc_outer_buf: [2048]u8 = undefined;
                    const outer_encoded = encodeUriComponent(ws_target, &enc_outer_buf);
                    loc = std.fmt.bufPrint(
                        &loc_buf,
                        "{s}?{s}={s}",
                        .{ resolved_base, ws_scheme, outer_encoded },
                    ) catch "";
                } else {
                    loc = std.fmt.bufPrint(
                        &loc_buf,
                        "{s}?{s}={s}%2Fdevtools%2Fpage%2Fkitesurf",
                        .{ resolved_base, ws_scheme, req_host },
                    ) catch "";
                }
                if (loc.len > 0) {
                    sendHttpRedirect(client_fd, loc)?;
                    return;
                }
            }
        }

        // 6b. Landing page at /
        if (std.mem.eql(u8, path, "/")) {
            sendLandingPage(client_fd, req_host, http_scheme, ws_scheme, inspector_base, no_inspector, path_only_inspector)?;
            return;
        }

        // 7. Non-upgrade /__fixtures/* requests
        if (std.mem.startsWith(u8, path, "/__fixtures/")) {
            const key = path[12..];
            if (key.len == 0 ||
                std.mem.eql(u8, key, "__proto__") ||
                std.mem.eql(u8, key, "constructor") ||
                std.mem.eql(u8, key, "toString") ||
                std.mem.eql(u8, key, "valueOf") ||
                std.mem.eql(u8, key, "hasOwnProperty") ||
                std.mem.indexOf(u8, key, "..") != null)
            {
                sendHttpResponse(client_fd, "404 Not Found", "text/plain", "not found")?;
                return;
            }
            const fix_buf = std.heap.c_allocator.alloc(u8, 65536) catch {
                sendHttpResponse(client_fd, "500 Internal Server Error", "text/plain", "oom")?;
                return;
            };
            defer std.heap.c_allocator.free(fix_buf);
            let hdr_json_buf: [8192]u8 = undefined;
            const extra_json = CdpServer.buildExtraHeadersJson(req, &hdr_json_buf);
            if (body := MockFixtures.handleFixtureRequest(target, "", _.session.user_agent, fix_buf, null, extra_json)) {
                const ct = MockFixtures.getContentType(path);
                sendHttpResponse(client_fd, "200 OK", ct, body)?;
                return;
            }
            sendHttpResponse(client_fd, "404 Not Found", "text/plain", "not found")?;
            return;
        }

        sendHttpResponse(client_fd, "404 Not Found", "text/plain", "not found")?;
    }

    static fn extractHttpHeader(req: []const u8, name: []const u8): ([]const u8)? {
        let it = std.mem.splitSequence(u8, req, "\r\n");
        * = it.next(); // skip request line
        while (line := it.next()) {
            if (line.len == 0) break;
            if (colon := std.mem.indexOfScalar(u8, line, ':')) {
                const hdr_name = std.mem.trim(u8, line[0..colon], " \t");
                if (std.ascii.eqlIgnoreCase(hdr_name, name)) {
                    return std.mem.trim(u8, line[colon + 1..], " \t");
                }
            }
        }
        return null;
    }

    static fn appendEscapedJsonStr(src: []const u8, out_buf: []u8, pos: *usize) {
        let i: usize = 0;
        while (i < src.len && pos.* + 2 < out_buf.len) : (i += 1) {
            const b = src[i];
            if (b == '"' || b == '\\') {
                out_buf[pos.*] = '\\';
                pos.* += 1;
                out_buf[pos.*] = b;
                pos.* += 1;
            } else if (b == '\n') {
                out_buf[pos.*] = '\\';
                pos.* += 1;
                out_buf[pos.*] = 'n';
                pos.* += 1;
            } else if (b == '\r') {
                out_buf[pos.*] = '\\';
                pos.* += 1;
                out_buf[pos.*] = 'r';
                pos.* += 1;
            } else {
                out_buf[pos.*] = b;
                pos.* += 1;
            }
        }
    }

    static fn buildExtraHeadersJson(req: []const u8, out_buf: []u8): []const u8 {
        if (out_buf.len < 4) return "{}";
        let pos: usize = 0;
        out_buf[pos] = '{';
        pos += 1;
        let first = true;
        let it = std.mem.splitSequence(u8, req, "\r\n");
        * = it.next();
        while (line := it.next()) {
            if (line.len == 0) break;
            if (colon := std.mem.indexOfScalar(u8, line, ':')) {
                const hdr_name = std.mem.trim(u8, line[0..colon], " \t");
                const hdr_val = std.mem.trim(u8, line[colon + 1..], " \t");
                if (hdr_name.len == 0) continue;
                if (pos + hdr_name.len + hdr_val.len + 16 >= out_buf.len) break;
                if (!first) {
                    out_buf[pos] = ',';
                    pos += 1;
                }
                first = false;
                out_buf[pos] = '"';
                pos += 1;
                CdpServer.appendEscapedJsonStr(hdr_name, out_buf, &pos);
                out_buf[pos] = '"';
                pos += 1;
                out_buf[pos] = ':';
                pos += 1;
                out_buf[pos] = '"';
                pos += 1;
                CdpServer.appendEscapedJsonStr(hdr_val, out_buf, &pos);
                out_buf[pos] = '"';
                pos += 1;
            }
        }
        out_buf[pos] = '}';
        pos += 1;
        return out_buf[0..pos];
    }

    static fn isAllowedWsOrigin(req: []const u8): bool {
        const origin_opt = CdpServer.extractHttpHeader(req, "Origin");
        if (origin_opt == null) return true;
        const origin = origin_opt.?;
        if (origin.len == 0) return true;
        if (std.ascii.eqlIgnoreCase(origin, "devtools://devtools")) return true;
        if (host := CdpServer.extractHttpHeader(req, "Host")) {
            let http_buf: [256]u8 = [0; 256];
            if (http_origin := std.fmt.bufPrint(&http_buf, "http://{s}", .{host}) catch null) {
                if (std.ascii.eqlIgnoreCase(origin, http_origin)) return true;
            }
            let https_buf: [256]u8 = [0; 256];
            if (https_origin := std.fmt.bufPrint(&https_buf, "https://{s}", .{host}) catch null) {
                if (std.ascii.eqlIgnoreCase(origin, https_origin)) return true;
            }
        }
        return false;
    }

    fn handleWebSocketUpgrade(*_, client_fd: i32, req: []const u8)! {
        if (!CdpServer.isAllowedWsOrigin(req)) {
            sendHttpResponse(client_fd, "403 Forbidden", "text/plain", "Forbidden")?;
            return;
        }

        const key = CdpServer.extractHttpHeader(req, "Sec-WebSocket-Key") ?? "";

        let accept_buf: [64]u8 = [0; 64];
        const accept_len = computeSecWebSocketAccept(key, &accept_buf) catch return error.AcceptFailed;
        const accept_str = accept_buf[0..accept_len];

        let resp_buf: [512]u8 = [0; 512];
        const resp = std.fmt.bufPrint(
            &resp_buf,
            "HTTP/1.1 101 Switching Protocols\r\n" ++
            "Upgrade: websocket\r\n" ++
            "Connection: Upgrade\r\n" ++
            "Sec-WebSocket-Accept: {s}\r\n\r\n",
            .{accept_str},
        ) catch return error.BufPrintFailed;

        socketWriteAll(client_fd, resp)? ;

        const idle_ms = extractQueryInt(req, "ksIdleMs");
        const max_session_ms = extractQueryInt(req, "ksMaxSessionMs");
        let initial_url_buf: [2048]u8 = [0; 2048];
        const initial_url = extractQueryString(req, "initialUrl", &initial_url_buf) ?? "";
        const skip_hdr = CdpServer.extractHttpHeader(req, "x-kitesurf-skip-raster");
        if (skip_hdr != null && std.mem.eql(u8, std.mem.trim(u8, skip_hdr.?, " \t\r\n"), "1")) {
            _.session.skip_raster = true;
        } else {
            _.session.skip_raster = false;
        }
        _.session.outbound_reject_pattern_count = 0;
        _.session.outbound_reject_websocket = false;

        // Enter WebSocket Frame Processing Loop
        _.runWebSocketSession(client_fd, idle_ms, max_session_ms, initial_url)? ;
    }

    fn flushDomEvents(*_, client_fd: i32)! {
        flushDomEventsForSession(client_fd, _.session)?;
    }

    fn flushPendingNavigation(*_, client_fd: i32)! {
        flushPendingNavigationForSession(client_fd, _.session, "")?;
    }

    fn flushReopenLoadEvents(*_, client_fd: i32)! {
        flushReopenLoadEventsForSession(client_fd, _.session, "")?;
    }

    static fn shouldSyncChildFrames(sess: *CdpPageSession): bool {
        if (sess.known_child_frame_count > 0) return true;
        let nid: usize = 1;
        while (nid < sess.doc.nodes.items.len) : (nid += 1) {
            const node = &sess.doc.nodes.items[nid];
            if (node.node_type == NodeType.Element &&
                (std.ascii.eqlIgnoreCase(node.tag_name, "iframe") || std.ascii.eqlIgnoreCase(node.tag_name, "frame")))
            {
                return true;
            }
        }
        return false;
    }

    static fn flushDomEventsForSession(client_fd: i32, sess: *CdpPageSession)! {
        if (!sess.dom_document_sent) return;
        let msgs = std.ArrayListUnmanaged(ByteSlice).empty;
        defer {
            let mi: usize = 0;
            while (mi < msgs.items.len) : (mi += 1) {
                std.heap.c_allocator.free(msgs.items[mi]);
            }
            msgs.deinit(std.heap.c_allocator);
        }
        drainDomMutationEvents(sess, &msgs);
        let mi: usize = 0;
        while (mi < msgs.items.len) : (mi += 1) {
            writeWsTextFrame(client_fd, msgs.items[mi])?;
        }
    }

    static fn emitCdpEventsToFd(client_fd: i32, events: *[MAX_CDP_EVENTS]CdpEvent, event_count: usize)! {
        let i: usize = 0;
        while (i < event_count) : (i += 1) {
            const has_sid = events[i].session_id.len > 0;
            const evt_needed = events[i].method.len + events[i].params_json.len + events[i].session_id.len + 64;
            if (evt_needed <= 4096) {
                let evt_buf: [4096]u8 = [0; 4096];
                const evt_msg = if (has_sid)
                    (std.fmt.bufPrint(
                        &evt_buf,
                        "{{\"method\":\"{s}\",\"params\":{s},\"sessionId\":\"{s}\"}}",
                        .{ events[i].method, events[i].params_json, events[i].session_id },
                    ) catch continue)
                else
                    (std.fmt.bufPrint(
                        &evt_buf,
                        "{{\"method\":\"{s}\",\"params\":{s}}}",
                        .{ events[i].method, events[i].params_json },
                    ) catch continue);
                writeWsTextFrame(client_fd, evt_msg)?;
            } else {
                const dyn_evt_buf = std.heap.c_allocator.alloc(u8, evt_needed) catch continue;
                defer std.heap.c_allocator.free(dyn_evt_buf);
                const evt_msg = if (has_sid)
                    (std.fmt.bufPrint(
                        dyn_evt_buf,
                        "{{\"method\":\"{s}\",\"params\":{s},\"sessionId\":\"{s}\"}}",
                        .{ events[i].method, events[i].params_json, events[i].session_id },
                    ) catch continue)
                else
                    (std.fmt.bufPrint(
                        dyn_evt_buf,
                        "{{\"method\":\"{s}\",\"params\":{s}}}",
                        .{ events[i].method, events[i].params_json },
                    ) catch continue);
                writeWsTextFrame(client_fd, evt_msg)?;
            }
        }
    }

    static fn flushPendingNavigationForSession(client_fd: i32, sess: *CdpPageSession, sid: []const u8)! {
        if (!sess.has_document) return;
        let nav_events: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
        let nav_event_count: usize = 0;
        if (performPendingNavigation(sess, &nav_events, &nav_event_count)) {
            if (sid.len > 0) {
                let ei: usize = 0;
                while (ei < nav_event_count) : (ei += 1) {
                    if (nav_events[ei].session_id.len == 0 &&
                        !std.mem.startsWith(u8, nav_events[ei].method, "Browser."))
                    {
                        nav_events[ei].session_id = sid;
                    }
                }
            }
            emitCdpEventsToFd(client_fd, &nav_events, nav_event_count)?;
        }
    }

    static fn emitAsyncNavAbortEventsAndReply(client_fd: i32, slot: *AsyncNavSlot)! {
        const sess = slot.target_sess orelse return;
        const lid = slot.loader_id_buf[0..slot.loader_id_len];
        const sid = slot.session_id_buf[0..slot.session_id_len];
        const has_sid = sid.len > 0 && !std.mem.startsWith(u8, sid, "capsurf-cdp-session-");

        if (sess.network_enabled) {
            let evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            let ecnt: usize = 0;
            const nav_u = slot.url_buf[0..slot.url_len];
            if (!slot.skip_first_req_sent && nav_u.len > 0) {
                let esc_u_buf: [2048]u8 = [0; 2048];
                const esc_u = escapeJsonString(nav_u, &esc_u_buf);
                const req_p = std.fmt.allocPrint(
                    sess.allocator,
                    "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{}}}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                    .{ lid, lid, esc_u, esc_u, sess.frame_id },
                ) catch "";
                if (req_p.len > 0) {
                    evts[ecnt] = CdpEvent.create("Network.requestWillBeSent", req_p);
                    if (has_sid) evts[ecnt].session_id = sid;
                    ecnt += 1;
                }
            }
            const fail_p = std.fmt.allocPrint(
                sess.allocator,
                "{{\"requestId\":\"{s}\",\"timestamp\":1700000000.01,\"type\":\"Document\",\"errorText\":\"net::ERR_ABORTED\",\"canceled\":true}}",
                .{lid},
            ) catch "";
            if (fail_p.len > 0) {
                evts[ecnt] = CdpEvent.create("Network.loadingFailed", fail_p);
                if (has_sid) evts[ecnt].session_id = sid;
                ecnt += 1;
            }
            if (ecnt > 0) {
                emitCdpEventsToFd(client_fd, &evts, ecnt)?;
            }
        }

        let res_buf: [512]u8 = [0; 512];
        const res_json = std.fmt.bufPrint(
            &res_buf,
            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"isDownload\":false,\"errorText\":\"net::ERR_ABORTED\"}}",
            .{ sess.frame_id, lid },
        ) catch "{\"isDownload\":false,\"errorText\":\"net::ERR_ABORTED\"}";
        let msg_buf: [1024]u8 = [0; 1024];
        const raw_id = slot.raw_id_buf[0..slot.raw_id_len];
        let msg: []const u8 = "";
        if (raw_id.len > 0) {
            if (sid.len > 0) {
                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":\"{s}\",\"result\":{s},\"sessionId\":\"{s}\"}}", .{ raw_id, res_json, sid }) catch return;
            } else {
                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":\"{s}\",\"result\":{s}}}", .{ raw_id, res_json }) catch return;
            }
        } else {
            if (sid.len > 0) {
                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":{d},\"result\":{s},\"sessionId\":\"{s}\"}}", .{ slot.req_id, res_json, sid }) catch return;
            } else {
                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":{d},\"result\":{s}}}", .{ slot.req_id, res_json }) catch return;
            }
        }
        writeWsTextFrame(client_fd, msg)?;
    }

    static fn abortAsyncNavForSession(client_fd: i32, nav_state: *PendingNavState, sess: *CdpPageSession)! {
        let i: usize = 0;
        while (i < MAX_ASYNC_NAVS) : (i += 1) {
            let slot = &nav_state.async_navs[i];
            if (!slot.active || slot.superseded) continue;
            if (slot.target_sess == sess) {
                if (slot.is_cdp_nav) {
                    emitAsyncNavAbortEventsAndReply(client_fd, slot)?;
                    slot.reset();
                } else {
                    slot.superseded = true;
                }
            }
        }
    }

    static fn startAsyncNavSlot(
        nav_state: *PendingNavState,
        sess: *CdpPageSession,
        sid: []const u8,
        is_cdp_nav: bool,
        req_id: i64,
        raw_id: []const u8,
        payload: []const u8,
        url: []const u8,
        referrer: []const u8,
        skip_first_req_sent: bool,
    ): bool {
        let free_idx: usize = MAX_ASYNC_NAVS;
        let i: usize = 0;
        while (i < MAX_ASYNC_NAVS) : (i += 1) {
            if (!nav_state.async_navs[i].active) {
                free_idx = i;
                break;
            }
        }
        if (free_idx >= MAX_ASYNC_NAVS) return false;

        let slot = &nav_state.async_navs[free_idx];
        slot.reset();

        let ip_addr: u32 = 0;
        let port: u16 = 80;
        const slen = buildAsyncNavHttpRequest(sess, url, referrer, &slot.send_buf, &ip_addr, &port);
        if (slen == 0) return false;

        const fd = socket(AF_INET, SOCK_STREAM | HttpScheduler.SOCK_NONBLOCK, 0);
        if (fd < 0) return false;

        const zero_sin: [8]u8 = [0; 8];
        const addr = HttpScheduler.NetSockAddrIn{
            sin_family: @as(u16, AF_INET),
            sin_port: HttpScheduler.htonsU16(port),
            sin_addr: ip_addr,
            sin_zero: zero_sin,
        };
        const rc = HttpScheduler.connect(fd, &addr, @as(u32, @sizeOf(HttpScheduler.NetSockAddrIn)));
        let initial_phase = AsyncNavPhase.connecting;
        if (rc == 0) {
            initial_phase = .writing;
        } else {
            let so_err: i32 = 0;
            let so_len: u32 = @as(u32, @sizeOf(i32));
            const gsrc = HttpScheduler.getsockopt(fd, HttpScheduler.SOL_SOCKET, HttpScheduler.SO_ERROR, &so_err, &so_len);
            if (!(gsrc == 0 && (so_err == 0 || so_err == HttpScheduler.EINPROGRESS))) {
                * = std.c.close(fd);
                return false;
            }
        }

        const rbuf = std.heap.c_allocator.alloc(u8, 262144) catch {
            * = std.c.close(fd);
            return false;
        };

        slot.active = true;
        slot.superseded = false;
        slot.is_cdp_nav = is_cdp_nav;
        slot.fd = fd;
        slot.phase = initial_phase;
        slot.target_sess = sess;
        slot.req_id = req_id;
        slot.skip_first_req_sent = skip_first_req_sent;
        slot.send_len = slen;
        slot.send_offset = 0;
        slot.recv_buf = rbuf;
        slot.recv_len = 0;
        slot.started_ms = serverNowMs();

        const sid_copy = @min(sid.len, slot.session_id_buf.len);
        if (sid_copy > 0) @memcpy(slot.session_id_buf[0..sid_copy], sid[0..sid_copy]);
        slot.session_id_len = sid_copy;

        const rid_copy = @min(raw_id.len, slot.raw_id_buf.len);
        if (rid_copy > 0) @memcpy(slot.raw_id_buf[0..rid_copy], raw_id[0..rid_copy]);
        slot.raw_id_len = rid_copy;

        const lid_copy = @min(sess.loader_id.len, slot.loader_id_buf.len);
        if (lid_copy > 0) @memcpy(slot.loader_id_buf[0..lid_copy], sess.loader_id[0..lid_copy]);
        slot.loader_id_len = lid_copy;

        const u_copy = @min(url.len, slot.url_buf.len);
        if (u_copy > 0) @memcpy(slot.url_buf[0..u_copy], url[0..u_copy]);
        slot.url_len = u_copy;

        const ref_copy = @min(referrer.len, slot.referrer_buf.len);
        if (ref_copy > 0) @memcpy(slot.referrer_buf[0..ref_copy], referrer[0..ref_copy]);
        slot.referrer_len = ref_copy;

        const p_copy = @min(payload.len, slot.payload_buf.len);
        if (p_copy > 0) @memcpy(slot.payload_buf[0..p_copy], payload[0..p_copy]);
        slot.payload_len = p_copy;

        return true;
    }

    static fn tryStartAsyncPendingNav(nav_state: *PendingNavState, sess: *CdpPageSession, sid: []const u8): bool {
        if (!sess.has_document) return false;
        let i: usize = 0;
        while (i < MAX_ASYNC_NAVS) : (i += 1) {
            if (nav_state.async_navs[i].active && !nav_state.async_navs[i].superseded && nav_state.async_navs[i].target_sess == sess) {
                return false;
            }
        }
        let url_buf: [2048]u8 = [0; 2048];
        const ulen = takePendingJsNavUrl(sess, &url_buf);
        if (ulen == 0) return false;
        return startAsyncNavSlot(nav_state, sess, sid, false, 0, "", "", url_buf[0..ulen], "", false);
    }

    fn tryInterceptAsyncPageNav(*_, client_fd: i32, cdp_req: CdpRequest, payload: []const u8, nav_state: *PendingNavState): bool! {
        if (!std.mem.eql(u8, cdp_req.method, "Page.navigate")) return false;
        const nav_sess = _.resolveSessionForRequest(cdp_req);
        abortAsyncNavForSession(client_fd, nav_state, nav_sess)?;
        const nav_url_raw = extractJsonString(cdp_req.params_raw, "url") ?? "";
        const req_frame_id = extractJsonString(cdp_req.params_raw, "frameId") ?? nav_sess.frame_id;
        if (req_frame_id.len > 0 && !std.mem.eql(u8, req_frame_id, nav_sess.frame_id)) return false;
        let unesc_nav_buf: [2048]u8 = [0; 2048];
        let nav_url_clean: []const u8 = nav_url_raw;
        if (nav_url_raw.len > 0 && nav_url_raw.len <= unesc_nav_buf.len) {
            nav_url_clean = unescapeJsonString(nav_url_raw, &unesc_nav_buf);
        }
        if (!canUseAsyncHttpNav(nav_sess, nav_url_clean)) return false;
        const bctx = extractJsonString(cdp_req.params_raw, "browserContextId") ?? "";
        if (bctx.len > 0) nav_sess.setContextId(bctx);
        nav_sess.nextLoaderId();
        const skip_req_sent = consumeFetchNavPreAnnounced(nav_sess.loader_id);
        const nav_ref = extractJsonString(cdp_req.params_raw, "referrer") ?? "";
        return startAsyncNavSlot(
            nav_state,
            nav_sess,
            cdp_req.session_id,
            true,
            cdp_req.id,
            cdp_req.raw_id,
            payload,
            nav_url_clean,
            nav_ref,
            skip_req_sent,
        );
    }

    fn completeAsyncNavSlot(*_, client_fd: i32, slot: *AsyncNavSlot, pending_promises: *[]PendingPromise, nav_state: *PendingNavState, net_ok: bool)! {
        if (slot.superseded) {
            slot.reset();
            return;
        }
        const sess = slot.target_sess orelse {
            slot.reset();
            return;
        };
        const sid = slot.session_id_buf[0..slot.session_id_len];
        const has_sid = sid.len > 0 && !std.mem.startsWith(u8, sid, "capsurf-cdp-session-");
        const nav_url = slot.url_buf[0..slot.url_len];
        const ref_url = slot.referrer_buf[0..slot.referrer_len];

        if (!slot.is_cdp_nav) {
            if (net_ok) {
                if (rb := slot.recv_buf) {
                    if (fetch_res := parseRawHttpToLoaderResult(rb[0..slot.recv_len], nav_url, slot.send_buf[0..slot.send_len])) {
                        let nav_events: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                        let nav_event_count: usize = 0;
                        if (completePendingNavigationFromResult(sess, &nav_events, &nav_event_count, fetch_res)) {
                            if (has_sid) {
                                let ei: usize = 0;
                                while (ei < nav_event_count) : (ei += 1) {
                                    if (nav_events[ei].session_id.len == 0 && !std.mem.startsWith(u8, nav_events[ei].method, "Browser.")) {
                                        nav_events[ei].session_id = sid;
                                    }
                                }
                            }
                            emitCdpEventsToFd(client_fd, &nav_events, nav_event_count)?;
                        }
                    }
                }
            }
            slot.reset();
            return;
        }

        if (net_ok) {
            if (rb := slot.recv_buf) {
                if (fetch_res := parseRawHttpToLoaderResult(rb[0..slot.recv_len], nav_url, slot.send_buf[0..slot.send_len])) {
                    const out_buf = std.heap.c_allocator.alloc(u8, 65536) catch {
                        slot.reset();
                        return;
                    };
                    defer std.heap.c_allocator.free(out_buf);
                    let events: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                    let event_count: usize = 0;
                    const resp = completePageNavigationFromResult(
                        sess,
                        slot.req_id,
                        out_buf,
                        &events,
                        &event_count,
                        ref_url,
                        slot.skip_first_req_sent,
                        fetch_res,
                    );
                    if (has_sid) {
                        let ei: usize = 0;
                        while (ei < event_count) : (ei += 1) {
                            if (events[ei].session_id.len == 0 && !std.mem.startsWith(u8, events[ei].method, "Browser.")) {
                                events[ei].session_id = sid;
                            }
                        }
                    }
                    emitCdpEventsToFd(client_fd, &events, event_count)?;

                    let msg_buf: [4096]u8 = [0; 4096];
                    const raw_id = slot.raw_id_buf[0..slot.raw_id_len];
                    let msg: []const u8 = "";
                    if (resp.error_code == 0) {
                        if (raw_id.len > 0) {
                            if (sid.len > 0) {
                                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":\"{s}\",\"result\":{s},\"sessionId\":\"{s}\"}}", .{ raw_id, resp.result_json, sid }) catch "";
                            } else {
                                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":\"{s}\",\"result\":{s}}}", .{ raw_id, resp.result_json }) catch "";
                            }
                        } else {
                            if (sid.len > 0) {
                                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":{d},\"result\":{s},\"sessionId\":\"{s}\"}}", .{ resp.id, resp.result_json, sid }) catch "";
                            } else {
                                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":{d},\"result\":{s}}}", .{ resp.id, resp.result_json }) catch "";
                            }
                        }
                    } else {
                        if (raw_id.len > 0) {
                            if (sid.len > 0) {
                                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":\"{s}\",\"error\":{{\"code\":{d},\"message\":\"{s}\"}},\"sessionId\":\"{s}\"}}", .{ raw_id, resp.error_code, resp.error_message, sid }) catch "";
                            } else {
                                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":\"{s}\",\"error\":{{\"code\":{d},\"message\":\"{s}\"}}}}", .{ raw_id, resp.error_code, resp.error_message }) catch "";
                            }
                        } else {
                            if (sid.len > 0) {
                                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":{d},\"error\":{{\"code\":{d},\"message\":\"{s}\"}},\"sessionId\":\"{s}\"}}", .{ resp.id, resp.error_code, resp.error_message, sid }) catch "";
                            } else {
                                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":{d},\"error\":{{\"code\":{d},\"message\":\"{s}\"}}}}", .{ resp.id, resp.error_code, resp.error_message }) catch "";
                            }
                        }
                    }
                    if (msg.len > 0) {
                        writeWsTextFrame(client_fd, msg)?;
                    }
                    slot.reset();
                    return;
                }
            }
            // Fallback for 3xx redirects or Content-Encoding: gzip/br
            if (slot.payload_len > 0) {
                const fallback_req = CdpRequest.parse(slot.payload_buf[0..slot.payload_len]);
                slot.reset();
                _.dispatchCdpCommand(client_fd, fallback_req, pending_promises, nav_state)?;
                return;
            }
        }

        // Network failure
        let res_buf: [512]u8 = [0; 512];
        const lid = slot.loader_id_buf[0..slot.loader_id_len];
        const err_res = std.fmt.bufPrint(
            &res_buf,
            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"isDownload\":false,\"errorText\":\"net::ERR_CONNECTION_RESET\"}}",
            .{ sess.frame_id, lid },
        ) catch "{\"isDownload\":false,\"errorText\":\"net::ERR_CONNECTION_RESET\"}";
        let msg_buf: [1024]u8 = [0; 1024];
        const raw_id = slot.raw_id_buf[0..slot.raw_id_len];
        let msg: []const u8 = "";
        if (raw_id.len > 0) {
            if (sid.len > 0) {
                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":\"{s}\",\"result\":{s},\"sessionId\":\"{s}\"}}", .{ raw_id, err_res, sid }) catch "";
            } else {
                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":\"{s}\",\"result\":{s}}}", .{ raw_id, err_res }) catch "";
            }
        } else {
            if (sid.len > 0) {
                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":{d},\"result\":{s},\"sessionId\":\"{s}\"}}", .{ slot.req_id, err_res, sid }) catch "";
            } else {
                msg = std.fmt.bufPrint(&msg_buf, "{{\"id\":{d},\"result\":{s}}}", .{ slot.req_id, err_res }) catch "";
            }
        }
        if (msg.len > 0) {
            writeWsTextFrame(client_fd, msg)?;
        }
        slot.reset();
    }

    static fn flushDeferredLoadEventsForSession(client_fd: i32, sess: *CdpPageSession, sid: []const u8)! {
        if (!sess.has_document) return;
        if (!sess.pending_main_load_deferred && !sess.pending_child_load_deferred) return;
        if (sess.activeAsyncFetchCount() > 0) return;

        let evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
        let cnt: usize = 0;
        if (sess.pending_main_load_deferred) {
            sess.pending_main_load_deferred = false;
            if (!sess.navigation_aborted && !sess.checkWindowStopped()) {
                evts[cnt] = CdpEvent.create("Page.loadEventFired", "{\"timestamp\":1700000000.1}");
                cnt += 1;
                const lc_load = std.fmt.allocPrint(
                    sess.allocator,
                    "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"load\",\"timestamp\":1700000000.1}}",
                    .{ sess.frame_id, sess.loader_id },
                ) catch "{}";
                evts[cnt] = CdpEvent.create("Page.lifecycleEvent", lc_load);
                cnt += 1;
                const stop_json = std.fmt.allocPrint(
                    sess.allocator,
                    "{{\"frameId\":\"{s}\"}}",
                    .{ sess.frame_id },
                ) catch "{}";
                evts[cnt] = CdpEvent.create("Page.frameStoppedLoading", stop_json);
                cnt += 1;
            }
        }
        if (sess.pending_child_load_deferred) {
            sess.pending_child_load_deferred = false;
            const cfid = sess.pending_child_load_frame_id_buf[0..sess.pending_child_load_frame_id_len];
            const clid = sess.pending_child_load_loader_id_buf[0..sess.pending_child_load_loader_id_len];
            if (cfid.len > 0 && !sess.navigation_aborted && !sess.checkWindowStopped()) {
                const lc_load = std.fmt.allocPrint(
                    sess.allocator,
                    "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"load\",\"timestamp\":1700000000.1}}",
                    .{ cfid, clid },
                ) catch "{}";
                evts[cnt] = CdpEvent.create("Page.lifecycleEvent", lc_load);
                cnt += 1;
                const stop_json = std.fmt.allocPrint(
                    sess.allocator,
                    "{{\"frameId\":\"{s}\"}}",
                    .{ cfid },
                ) catch "{}";
                evts[cnt] = CdpEvent.create("Page.frameStoppedLoading", stop_json);
                cnt += 1;
            }
            sess.pending_child_load_frame_id_len = 0;
            sess.pending_child_load_loader_id_len = 0;
        }
        if (cnt > 0) {
            if (sid.len > 0) {
                let ei: usize = 0;
                while (ei < cnt) : (ei += 1) {
                    if (evts[ei].session_id.len == 0) {
                        evts[ei].session_id = sid;
                    }
                }
            }
            emitCdpEventsToFd(client_fd, &evts, cnt)?;
        }
    }

    static fn flushReopenLoadEventsForSession(client_fd: i32, sess: *CdpPageSession, sid: []const u8)! {
        if (!sess.has_document) return;
        let reopen_events: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
        const reopen_count = sess.pollReopenLoadEvents(&reopen_events);
        if (reopen_count > 0) {
            if (sid.len > 0) {
                let ei: usize = 0;
                while (ei < reopen_count) : (ei += 1) {
                    if (reopen_events[ei].session_id.len == 0) {
                        reopen_events[ei].session_id = sid;
                    }
                }
            }
            emitCdpEventsToFd(client_fd, &reopen_events, reopen_count)?;
        }
        let console_events: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
        const console_count = sess.pollConsoleEvents(&console_events);
        if (console_count > 0) {
            if (sid.len > 0) {
                let ei: usize = 0;
                while (ei < console_count) : (ei += 1) {
                    if (console_events[ei].session_id.len == 0) {
                        console_events[ei].session_id = sid;
                    }
                }
            }
            emitCdpEventsToFd(client_fd, &console_events, console_count)?;
        }
        let webmcp_events: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
        const webmcp_count = sess.pollWebMcpEvents(&webmcp_events);
        if (webmcp_count > 0) {
            if (sid.len > 0) {
                let ei: usize = 0;
                while (ei < webmcp_count) : (ei += 1) {
                    if (webmcp_events[ei].session_id.len == 0) {
                        webmcp_events[ei].session_id = sid;
                    }
                }
            }
            emitCdpEventsToFd(client_fd, &webmcp_events, webmcp_count)?;
        }
    }

    fn pumpScreencastFrame(*_, client_fd: i32, now_ms: i64): bool! {
        if (!_.session.has_document) return false;
        if (!_.session.screencast_handler.canSendFrame()) return false;
        if (!_.session.screencast_handler.isIntervalElapsed(now_ms)) return false;

        let raf_buf: [128]u8 = [0; 128];
        const raf_js = std.fmt.bufPrint(
            &raf_buf,
            "(function(){{if(typeof __capsurf_pump_raf==='function')return __capsurf_pump_raf({d});return 0;}})()",
            .{now_ms},
        ) catch "";
        if (raf_js.len > 0) {
            const rv = _.session.ctx.eval(raf_js);
            _.session.ctx.freeValue(rv);
            _.session.ctx.drainJobs();
        }

        let sx: f32 = 0.0;
        let sy: f32 = 0.0;
        let cw: f32 = 0.0;
        let ch: f32 = 0.0;
        _.session.updateLayoutMetrics(&sx, &sy, &cw, &ch);

        const st = _.session.screencast_handler.getState();
        const vw: u32 = if (_.session.viewport_width > 0.0) @as(u32, @intFromFloat(_.session.viewport_width)) else 1280;
        const vh: u32 = if (_.session.viewport_height > 0.0) @as(u32, @intFromFloat(_.session.viewport_height)) else 720;

        const png_bytes = _.session.captureScreencastFrame(st.max_width, st.max_height) catch return false;
        defer _.session.allocator.free(png_bytes);

        const sx_i: i64 = @as(i64, @intFromFloat(sx));
        const sy_i: i64 = @as(i64, @intFromFloat(sy));
        const frame_hash = ScreencastHandler.computeFrameHash(png_bytes);
        if (_.session.screencast_handler.shouldSkipIdleFrame(frame_hash, vw, vh, sx_i, sy_i)) {
            if (_.session.frames_rendered > 0) _.session.frames_rendered -= 1;
            return false;
        }

        const b64_str = Base64.encodeAlloc(_.session.allocator, png_bytes) catch return false;
        defer _.session.allocator.free(b64_str);

        const seq = _.session.screencast_handler.nextFrameSeq();
        const ts_sec = @as(f64, @floatFromInt(now_ms)) / 1000.0;
        const params_buf = std.heap.c_allocator.alloc(u8, b64_str.len + 512) catch return false;
        defer std.heap.c_allocator.free(params_buf);

        if (params_json := formatScreencastFrameParams(seq, b64_str, st.max_width, st.max_height, sx, sy, ts_sec, params_buf)) {
            let evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            evts[0] = CdpEvent.create("Page.screencastFrame", params_json);
            const owner = _.session.screencast_handler.getOwnerSession();
            if (owner.len > 0 && !std.mem.eql(u8, owner, _.session.target_id)) {
                evts[0].session_id = owner;
            }
            emitCdpEventsToFd(client_fd, &evts, 1)?;
            _.session.screencast_handler.recordSentFrame(frame_hash, vw, vh, sx_i, sy_i, now_ms);
            return true;
        }
        return false;
    }

    static fn pumpSceneStreamFrameForSession(
        client_fd: i32,
        sess: *CdpPageSession,
        now_ms: i64,
        raf_already_pumped: bool,
    ): bool! {
        const stream_opt = sess.facade.stream_mgr.getPrimaryStream();
        if (stream_opt == null) return false;
        let stream = stream_opt.?;
        if (!stream.active) return false;

        let sent_scene = false;
        if (sess.has_document && stream.canSendFrame() && stream.isIntervalElapsed(now_ms)) {
            if (!raf_already_pumped) {
                let raf_buf: [128]u8 = [0; 128];
                const raf_js = std.fmt.bufPrint(
                    &raf_buf,
                    "(function(){{if(typeof __capsurf_pump_raf==='function')return __capsurf_pump_raf({d});return 0;}})()",
                    .{now_ms},
                ) catch "";
                if (raf_js.len > 0) {
                    const rv = sess.ctx.eval(raf_js);
                    sess.ctx.freeValue(rv);
                    sess.ctx.drainJobs();
                }
            }

            let sx: f32 = 0.0;
            let sy: f32 = 0.0;
            let cw: f32 = 0.0;
            let ch: f32 = 0.0;
            sess.updateLayoutMetrics(&sx, &sy, &cw, &ch);

            const vw: u32 = if (sess.viewport_width > 0.0) @as(u32, @intFromFloat(sess.viewport_width)) else stream.config.width;
            const vh: u32 = if (sess.viewport_height > 0.0) @as(u32, @intFromFloat(sess.viewport_height)) else stream.config.height;

            let img_w: u32 = 0;
            let img_h: u32 = 0;
            let rects_hash: u64 = 0;
            const cur_rgba = sess.captureSceneRgbaAndRectsHash(stream.config.width, stream.config.height, &img_w, &img_h, &rects_hash) catch "";
            if (cur_rgba.len > 0 && img_w > 0 && img_h > 0) {
                defer sess.allocator.free(cur_rgba);
                const sx_i: i64 = @as(i64, @intFromFloat(sx));
                const sy_i: i64 = @as(i64, @intFromFloat(sy));
                const frame_hash = ScreencastHandler.computeFrameHash(cur_rgba);
                if (!stream.shouldSkipIdleFrame(frame_hash, vw, vh, sx_i, sy_i, sess.focused_node)) {
                    let use_partial = false;
                    let clip_x: i32 = 0;
                    let clip_y: i32 = 0;
                    let clip_w: u32 = 0;
                    let clip_h: u32 = 0;

                    const can_partial = stream.config.allow_partial &&
                        stream.has_last_frame &&
                        !stream.need_full_frame &&
                        stream.last_sent_seq > 0 &&
                        stream.last_viewport_w == vw &&
                        stream.last_viewport_h == vh &&
                        stream.last_scroll_x == sx_i &&
                        stream.last_scroll_y == sy_i &&
                        stream.last_rects_hash == rects_hash &&
                        stream.last_rgba_w == img_w &&
                        stream.last_rgba_h == img_h &&
                        stream.last_rgba != null;

                    if (can_partial) {
                        const prev_rgba = stream.last_rgba.?;
                        if (prev_rgba.len == cur_rgba.len) {
                            let min_x: u32 = img_w;
                            let min_y: u32 = img_h;
                            let max_x: u32 = 0;
                            let max_y: u32 = 0;
                            let py: u32 = 0;
                            while (py < img_h) : (py += 1) {
                                const row_off = @as(usize, py) * @as(usize, img_w) * 4;
                                let px: u32 = 0;
                                while (px < img_w) : (px += 1) {
                                    const idx = row_off + @as(usize, px) * 4;
                                    if (cur_rgba[idx] != prev_rgba[idx] ||
                                        cur_rgba[idx + 1] != prev_rgba[idx + 1] ||
                                        cur_rgba[idx + 2] != prev_rgba[idx + 2] ||
                                        cur_rgba[idx + 3] != prev_rgba[idx + 3])
                                    {
                                        if (px < min_x) min_x = px;
                                        if (px > max_x) max_x = px;
                                        if (py < min_y) min_y = py;
                                        if (py > max_y) max_y = py;
                                    }
                                }
                            }
                            if (min_x <= max_x && min_y <= max_y) {
                                const dw = max_x - min_x + 1;
                                const dh = max_y - min_y + 1;
                                if (dw < img_w || dh < img_h) {
                                    use_partial = true;
                                    clip_x = @as(i32, @intCast(min_x));
                                    clip_y = @as(i32, @intCast(min_y));
                                    clip_w = dw;
                                    clip_h = dh;
                                }
                            }
                        }
                    }

                    const png_bytes = if (use_partial)
                        (encodeRgbaRectToPng(sess.allocator, cur_rgba, img_w, img_h, clip_x, clip_y, clip_w, clip_h, 1) catch "")
                    else
                        (encodeRgbaRectToPng(sess.allocator, cur_rgba, img_w, img_h, 0, 0, img_w, img_h, 1) catch "");

                    if (png_bytes.len > 0) {
                        defer sess.allocator.free(png_bytes);
                        const seq = sess.facade.stream_mgr.nextFrameSeq(sess.target_id);
                        const base_seq = stream.last_sent_seq;
                        const hdr_bytes = if (use_partial) SCENE_PARTIAL_HEADER_BYTES else SCENE_ENVELOPE_HEADER_BYTES;
                        const env_buf = std.heap.c_allocator.alloc(u8, hdr_bytes + png_bytes.len) catch return false;
                        defer std.heap.c_allocator.free(env_buf);
                        const env_opt = if (use_partial)
                            SceneStreamManager.buildPartialEnvelope(seq, base_seq, clip_x, clip_y, clip_w, clip_h, png_bytes, env_buf)
                        else
                            SceneStreamManager.buildSceneEnvelope(MSG_SCENE, seq, img_w, img_h, png_bytes, env_buf);
                        if (env_bytes := env_opt) {
                            const b64_str = Base64.encodeAlloc(sess.allocator, env_bytes) catch return false;
                            defer sess.allocator.free(b64_str);
                            const params_buf = std.heap.c_allocator.alloc(u8, b64_str.len + 64) catch return false;
                            defer std.heap.c_allocator.free(params_buf);
                            if (params_json := std.fmt.bufPrint(params_buf, "{{\"data\":\"{s}\"}}", .{b64_str}) catch null) {
                                let evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                                evts[0] = CdpEvent.create("Kitesurf.sceneFrame", params_json);
                                const owner = stream.getOwnerSession();
                                if (owner.len > 0 && !std.mem.eql(u8, owner, sess.target_id)) {
                                    evts[0].session_id = owner;
                                }
                                emitCdpEventsToFd(client_fd, &evts, 1)?;
                                stream.updateLastRgba(cur_rgba, img_w, img_h);
                                stream.last_rects_hash = rects_hash;
                                stream.last_sent_seq = seq;
                                stream.recordSentFrame(frame_hash, vw, vh, sx_i, sy_i, sess.focused_node, now_ms);
                                sent_scene = true;
                            }
                        }
                    }
                } else {
                    if (sess.frames_rendered > 0) sess.frames_rendered -= 1;
                }
            }
        }

        if (!sent_scene && stream.shouldSendHeartbeat(now_ms)) {
            const hb_json = "{\"t\":\"sceneAlive\"}";
            let hb_env_buf: [64]u8 = [0; 64];
            if (env_bytes := SceneStreamManager.buildInfoEnvelope(hb_json, &hb_env_buf)) {
                const b64_str = Base64.encodeAlloc(sess.allocator, env_bytes) catch return false;
                defer sess.allocator.free(b64_str);
                let params_buf: [256]u8 = [0; 256];
                if (params_json := std.fmt.bufPrint(&params_buf, "{{\"data\":\"{s}\"}}", .{b64_str}) catch null) {
                    let evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                    evts[0] = CdpEvent.create("Kitesurf.sceneFrame", params_json);
                    const owner = stream.getOwnerSession();
                    if (owner.len > 0 && !std.mem.eql(u8, owner, sess.target_id)) {
                        evts[0].session_id = owner;
                    }
                    emitCdpEventsToFd(client_fd, &evts, 1)?;
                    stream.last_activity_ms = now_ms;
                    return true;
                }
            }
        }

        return sent_scene;
    }

    fn pumpAllSceneStreams(*_, client_fd: i32, now_ms: i64, main_raf_pumped: bool)! {
        if (_.session.facade.stream_mgr.isActive()) {
            * = CdpServer.pumpSceneStreamFrameForSession(client_fd, _.session, now_ms, main_raf_pumped) catch false;
        }
        let ti: usize = 0;
        while (ti < MAX_TAB_SESSIONS) : (ti += 1) {
            if (_.tab_sessions[ti].active && _.tab_sessions[ti].page_session != null) {
                const tab_sess = _.tab_sessions[ti].page_session.?;
                if (tab_sess.facade.stream_mgr.isActive()) {
                    * = CdpServer.pumpSceneStreamFrameForSession(client_fd, tab_sess, now_ms, false) catch false;
                }
            }
        }
    }

    fn runWebSocketSession(*_, client_fd: i32, idle_ms: i64, max_session_ms: i64, initial_url: []const u8)! {
        _.deinitTabSessions();
        defer _.deinitTabSessions();
        _.session.resetSession();
        defer _.session.resetSession();
        _.net_scheduler.abortAll();
        _.net_idle_tracker.resetForNavigation();
        defer {
            _.net_scheduler.abortAll();
            _.net_idle_tracker.resetForNavigation();
        }
        _.active_client_fd = client_fd;
        defer _.active_client_fd = -1;
        _.deferred_req_count = 0;
        _.setActiveDialogSessionId("");
        registerDialogHook(unsafe { @ptrCast(_) }, &serverDialogHook);
        registerWorkerContextHook(unsafe { @ptrCast(_) }, &serverWorkerContextHook);
        let pending_initial_url = initial_url;
        let frame_payload = std.heap.c_allocator.alloc(u8, 8 * 1024 * 1024) catch return;
        defer std.heap.c_allocator.free(frame_payload);
        @memset(frame_payload, 0);

        const session_start_ms = serverNowMs();
        let last_client_activity_ms = session_start_ms;
        let last_idle_tick_ms: i64 = serverNowMs();

        // Pending slow-navigation state for non-blocking navigate window
        let nav_state = std.heap.c_allocator.create(PendingNavState) catch return;
        defer {
            let ani_d: usize = 0;
            while (ani_d < MAX_ASYNC_NAVS) : (ani_d += 1) {
                nav_state.async_navs[ani_d].reset();
            }
            std.heap.c_allocator.destroy(nav_state);
        }
        nav_state.* = PendingNavState{};
        defer markFetchNavPreAnnounced("");
        let pending_promises = std.heap.c_allocator.alloc(PendingPromise, 16) catch return;
        defer {
            let ppi_d: usize = 0;
            while (ppi_d < pending_promises.len) : (ppi_d += 1) {
                if (pending_promises[ppi_d].active) {
                    if (ts_d := pending_promises[ppi_d].target_sess) {
                        ts_d.freeParkedPromiseSlot(pending_promises[ppi_d].slot);
                    }
                }
            }
            std.heap.c_allocator.free(pending_promises);
        }
        let ppi_i: usize = 0;
        while (ppi_i < pending_promises.len) : (ppi_i += 1) {
            pending_promises[ppi_i] = PendingPromise{};
        }
        let pending_wheel = std.heap.c_allocator.create(PendingWheelBatch) catch return;
        defer std.heap.c_allocator.destroy(pending_wheel);
        pending_wheel.* = PendingWheelBatch{};

        while (_.running) {
            if (_.deferred_req_count > 0) {
                let close_after_drain = false;
                while (_.deferred_req_count > 0 && _.running) {
                    let def_buf: [4096]u8 = [0; 4096];
                    const def_len = _.deferred_req_lens[0];
                    if (def_len > 0) @memcpy(def_buf[0..def_len], _.deferred_reqs[0][0..def_len]);
                    let di: usize = 1;
                    while (di < _.deferred_req_count) : (di += 1) {
                        const l = _.deferred_req_lens[di];
                        if (l > 0) @memcpy(_.deferred_reqs[di - 1][0..l], _.deferred_reqs[di][0..l]);
                        _.deferred_req_lens[di - 1] = l;
                    }
                    _.deferred_req_count -= 1;
                    _.deferred_req_lens[_.deferred_req_count] = 0;
                    if (def_len == 0) continue;
                    const def_req = CdpRequest.parse(def_buf[0..def_len]);
                    const is_def_close = std.mem.eql(u8, def_req.method, "Browser.close");
                    _.dispatchCdpCommand(client_fd, def_req, pending_promises, nav_state) catch {
                        close_after_drain = true;
                        break;
                    };
                    if (is_def_close) {
                        writeWsCloseFrame(client_fd, 1000, "Browser.close") catch {};
                        close_after_drain = true;
                        break;
                    }
                }
                if (close_after_drain) break;
            }
            const loop_now_ms = serverNowMs();
            if (max_session_ms > 0 && (loop_now_ms - session_start_ms) >= max_session_ms) {
                writeWsCloseFrame(client_fd, 1001, "Max session exceeded") catch {};
                break;
            }
            if (idle_ms > 0 && (loop_now_ms - last_client_activity_ms) >= idle_ms) {
                writeWsCloseFrame(client_fd, 1001, "Session idle timeout") catch {};
                break;
            }
            if (nav_state.pending_nav_active && loop_now_ms >= nav_state.pending_nav_deadline_ms) {
                nav_state.pending_nav_active = false;
                const nav_req = CdpRequest.parse(nav_state.pending_nav_payload_buf[0..nav_state.pending_nav_payload_len]);
                _.dispatchCdpCommand(client_fd, nav_req, pending_promises, nav_state) catch break;
                let ei: usize = 0;
                while (ei < nav_state.pending_eval_count) : (ei += 1) {
                    if (nav_state.pending_evals[ei].active) {
                        nav_state.pending_evals[ei].active = false;
                        const ev_req = CdpRequest.parse(nav_state.pending_evals[ei].payload_buf[0..nav_state.pending_evals[ei].payload_len]);
                        _.dispatchCdpCommand(client_fd, ev_req, pending_promises, nav_state) catch break;
                    }
                }
                nav_state.pending_eval_count = 0;
            }

            if (pending_initial_url.len > 0) {
                const init_u = pending_initial_url;
                pending_initial_url = "";
                let nav_params_buf: [2304]u8 = [0; 2304];
                if (nav_params := std.fmt.bufPrint(&nav_params_buf, "{{\"url\":\"{s}\"}}", .{init_u}) catch null) {
                    const init_res_buf = std.heap.c_allocator.alloc(u8, 65536) catch null;
                    if (init_res_buf != null) {
                        defer std.heap.c_allocator.free(init_res_buf.?);
                        let init_events: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                        let init_event_count: usize = 0;
                        const init_req = CdpRequest{
                            id: 0,
                            method: "Page.navigate",
                            params_raw: nav_params,
                        };
                        * = CdpDispatcher.dispatch(_.session, init_req, init_res_buf.?, &init_events, &init_event_count);
                        emitCdpEventsToFd(client_fd, &init_events, init_event_count) catch break;
                    }
                }
            }

            let pfds: [2 + MAX_SCHEDULER_SLOTS + MAX_ASYNC_NAVS]PollFd = [PollFd{ fd: -1, events: 0, revents: 0 }; 2 + MAX_SCHEDULER_SLOTS + MAX_ASYNC_NAVS];
            pfds[0] = PollFd{ fd: client_fd, events: POLLIN, revents: 0 };
            pfds[1] = PollFd{ fd: _.listen_fd, events: POLLIN, revents: 0 };
            let fast_poll = _.session.screencast_handler.isActive() && _.session.screencast_handler.state.min_frame_interval_ms <= 16;
            if (!fast_poll) {
                if (st := _.session.facade.stream_mgr.getPrimaryStream()) {
                    if (st.min_frame_interval_ms <= 16) fast_poll = true;
                }
            }
            if (!fast_poll) {
                let ti_p: usize = 0;
                while (ti_p < MAX_TAB_SESSIONS) : (ti_p += 1) {
                    if (_.tab_sessions[ti_p].active && _.tab_sessions[ti_p].page_session != null) {
                        const tab_sess_p = _.tab_sessions[ti_p].page_session.?;
                        if (st := tab_sess_p.facade.stream_mgr.getPrimaryStream()) {
                            if (st.min_frame_interval_ms <= 16) {
                                fast_poll = true;
                                break;
                            }
                        }
                    }
                }
            }
            let has_active_promise = false;
            let ppi_chk: usize = 0;
            while (ppi_chk < pending_promises.len) : (ppi_chk += 1) {
                if (pending_promises[ppi_chk].active) {
                    has_active_promise = true;
                    break;
                }
            }
            let has_active_async_nav = false;
            let ani_chk: usize = 0;
            while (ani_chk < MAX_ASYNC_NAVS) : (ani_chk += 1) {
                if (nav_state.async_navs[ani_chk].active) {
                    has_active_async_nav = true;
                    break;
                }
            }
            const base_poll_ms: i32 = if (fast_poll) 5 else 15;
            let async_fetch_active: u32 = if (_.session.has_document) _.session.activeAsyncFetchCount() else 0;
            if (async_fetch_active == 0) {
                let ati: usize = 0;
                while (ati < MAX_TAB_SESSIONS) : (ati += 1) {
                    if (_.tab_sessions[ati].active) {
                        if (ts := _.tab_sessions[ati].page_session) {
                            if (ts.has_document) {
                                async_fetch_active += ts.activeAsyncFetchCount();
                            }
                        }
                    }
                }
            }
            const poll_ms: i32 = if (pending_wheel.active || has_active_promise || has_active_async_nav || async_fetch_active > 0) 1 else base_poll_ms;
            const base_nfds: usize = if (_.listen_fd >= 0) 2 else 1;
            let net_pfds: [MAX_SCHEDULER_SLOTS]NetPollFd = [NetPollFd{ fd: -1, events: 0, revents: 0 }; MAX_SCHEDULER_SLOTS];
            let slot_indices: [MAX_SCHEDULER_SLOTS]usize = [0; MAX_SCHEDULER_SLOTS];
            const net_nfds = _.net_scheduler.fillPollFds(&net_pfds, &slot_indices);
            let nfi: usize = 0;
            while (nfi < net_nfds) : (nfi += 1) {
                pfds[base_nfds + nfi] = PollFd{
                    fd: net_pfds[nfi].fd,
                    events: net_pfds[nfi].events,
                    revents: 0,
                };
            }
            let anav_pfd_idx: [MAX_ASYNC_NAVS]usize = [0; MAX_ASYNC_NAVS];
            let anav_nfds: usize = 0;
            let ani_p: usize = 0;
            while (ani_p < MAX_ASYNC_NAVS) : (ani_p += 1) {
                if (nav_state.async_navs[ani_p].active && nav_state.async_navs[ani_p].fd >= 0) {
                    const ev: i16 = if (nav_state.async_navs[ani_p].phase == .reading) POLLIN else HttpScheduler.POLLOUT;
                    pfds[base_nfds + net_nfds + anav_nfds] = PollFd{
                        fd: nav_state.async_navs[ani_p].fd,
                        events: ev,
                        revents: 0,
                    };
                    anav_pfd_idx[anav_nfds] = ani_p;
                    anav_nfds += 1;
                }
            }
            const nfds: u64 = @as(u64, @intCast(base_nfds + net_nfds + anav_nfds));
            const pr = poll(&pfds[0], nfds, poll_ms);
            if (pr < 0) {
                break;
            }
            const tick_now_ms = serverNowMs();
            if (net_nfds > 0) {
                let rfi: usize = 0;
                while (rfi < net_nfds) : (rfi += 1) {
                    net_pfds[rfi].revents = pfds[base_nfds + rfi].revents;
                }
                * = _.net_scheduler.stepSockets(&net_pfds, &slot_indices, net_nfds, tick_now_ms);
            }
            if (anav_nfds > 0) {
                let afi: usize = 0;
                while (afi < anav_nfds) : (afi += 1) {
                    const si = anav_pfd_idx[afi];
                    let aslot = &nav_state.async_navs[si];
                    if (!aslot.active || aslot.fd < 0) continue;
                    const rev = pfds[base_nfds + net_nfds + afi].revents;
                    if ((rev & (POLLERR | POLLNVAL)) != 0) {
                        aslot.closeFd();
                        _.completeAsyncNavSlot(client_fd, aslot, pending_promises, nav_state, false) catch break;
                        continue;
                    }
                    if (aslot.phase == .connecting) {
                        if ((rev & HttpScheduler.POLLOUT) != 0) {
                            let so_err: i32 = 0;
                            let so_len: u32 = @as(u32, @sizeOf(i32));
                            const gsrc = HttpScheduler.getsockopt(aslot.fd, HttpScheduler.SOL_SOCKET, HttpScheduler.SO_ERROR, &so_err, &so_len);
                            if (gsrc == 0 && so_err == 0) {
                                aslot.phase = if (aslot.send_len > 0) .writing else .reading;
                            } else {
                                aslot.closeFd();
                                _.completeAsyncNavSlot(client_fd, aslot, pending_promises, nav_state, false) catch break;
                            }
                        }
                    } else if (aslot.phase == .writing) {
                        if ((rev & HttpScheduler.POLLOUT) != 0) {
                            const rem = aslot.send_len - aslot.send_offset;
                            if (rem > 0) {
                                const off = aslot.send_offset;
                                const send_slice: []const u8 = aslot.send_buf[off..aslot.send_len];
                                const sent = HttpScheduler.send(aslot.fd, send_slice.ptr, rem, HttpScheduler.MSG_DONTWAIT | HttpScheduler.MSG_NOSIGNAL);
                                if (sent > 0) {
                                    aslot.send_offset += @as(usize, @intCast(sent));
                                } else if (sent == 0) {
                                    aslot.closeFd();
                                    _.completeAsyncNavSlot(client_fd, aslot, pending_promises, nav_state, false) catch break;
                                    continue;
                                }
                            }
                            if (aslot.send_offset >= aslot.send_len) {
                                aslot.phase = .reading;
                            }
                        }
                    } else if (aslot.phase == .reading) {
                        if ((rev & (POLLIN | POLLHUP)) != 0) {
                            if (rb := aslot.recv_buf) {
                                const avail = rb.len - aslot.recv_len;
                                if (avail == 0) {
                                    aslot.closeFd();
                                    _.completeAsyncNavSlot(client_fd, aslot, pending_promises, nav_state, true) catch break;
                                } else {
                                    const recv_slice = rb[aslot.recv_len..];
                                    const rn = HttpScheduler.recv(aslot.fd, recv_slice.ptr, avail, HttpScheduler.MSG_DONTWAIT);
                                    if (rn > 0) {
                                        aslot.recv_len += @as(usize, @intCast(rn));
                                    } else if (rn == 0 || (rev & POLLHUP) != 0) {
                                        aslot.closeFd();
                                        _.completeAsyncNavSlot(client_fd, aslot, pending_promises, nav_state, true) catch break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            * = _.net_scheduler.startPending(tick_now_ms);
            const net_active: u32 = @as(u32, @intCast(_.net_scheduler.inflightCount())) + async_fetch_active;
            const net_queued: u32 = @as(u32, @intCast(_.net_scheduler.queuedCount()));
            const net_dispatch_pending = _.net_scheduler.hasPendingDispatch();
            _.net_idle_tracker.sampleIdle(net_active, net_queued, net_dispatch_pending, tick_now_ms);
            const idle_status = _.net_idle_tracker.checkIdleReady(tick_now_ms, 500);
            if (_.session.has_document && (idle_status.almost_idle || idle_status.network_idle)) {
                let lc_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                let lc_cnt: usize = 0;
                let lc_buf_almost: [256]u8 = [0; 256];
                let lc_buf_idle: [256]u8 = [0; 256];
                const ts_sec: f64 = @as(f64, @floatFromInt(tick_now_ms)) / 1000.0;
                if (idle_status.almost_idle) {
                    const lc_json = std.fmt.bufPrint(
                        &lc_buf_almost,
                        "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"networkalmostidle\",\"timestamp\":{d:.3}}}",
                        .{ _.session.frame_id, _.session.loader_id, ts_sec },
                    ) catch "{}";
                    lc_evts[lc_cnt] = CdpEvent.create("Page.lifecycleEvent", lc_json);
                    lc_cnt += 1;
                }
                if (idle_status.network_idle) {
                    const lc_json = std.fmt.bufPrint(
                        &lc_buf_idle,
                        "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"networkIdle\",\"timestamp\":{d:.3}}}",
                        .{ _.session.frame_id, _.session.loader_id, ts_sec },
                    ) catch "{}";
                    lc_evts[lc_cnt] = CdpEvent.create("Page.lifecycleEvent", lc_json);
                    lc_cnt += 1;
                }
                if (lc_cnt > 0) {
                    emitCdpEventsToFd(client_fd, &lc_evts, lc_cnt) catch break;
                }
            }
            if (_.listen_fd >= 0 && (pfds[1].revents & POLLIN) != 0) {
                const extra_fd = accept(_.listen_fd, null, null);
                if (extra_fd >= 0) {
                    setTcpNoDelay(extra_fd);
                    let extra_buf: [4096]u8 = [0; 4096];
                    const en = recv(extra_fd, &extra_buf, extra_buf.len, 0);
                    if (en > 0) {
                        const extra_req = extra_buf[0..@as(usize, @intCast(en))];
                        _.handleHttpRequest(extra_fd, extra_req) catch {};
                    }
                    * = std.c.close(extra_fd);
                }
            }
            const should_run_idle_tick = (pr == 0) || ((tick_now_ms - last_idle_tick_ms) >= @as(i64, base_poll_ms));
            if (should_run_idle_tick) {
                last_idle_tick_ms = tick_now_ms;
                if (pending_wheel.active) {
                    _.flushPendingWheelBatch(client_fd, pending_wheel) catch break;
                }
                pumpPendingPromises(client_fd, pending_promises) catch break;
                if (_.session.has_document) {
                    _.setActiveDialogSessionId("");
                    _.session.pumpTimers();
                    _.flushReopenLoadEvents(client_fd) catch break;
                    flushDeferredLoadEventsForSession(client_fd, _.session, "") catch break;
                    if (shouldSyncChildFrames(_.session)) {
                        let cf_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                        let cf_cnt: usize = 0;
                        emitNewChildFrameEvents(_.session, &cf_evts, &cf_cnt);
                        if (cf_cnt > 0) emitCdpEventsToFd(client_fd, &cf_evts, cf_cnt) catch break;
                    }
                    if (!tryStartAsyncPendingNav(nav_state, _.session, "")) {
                        _.flushPendingNavigation(client_fd) catch break;
                    }
                    if (_.session.binding_count > 0) {
                        let b_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                        const b_cnt = _.session.pollBindingEvents(&b_evts);
                        if (b_cnt > 0) emitCdpEventsToFd(client_fd, &b_evts, b_cnt) catch break;
                    }
                    if (_.session.intercept_file_chooser) {
                        let fc_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                        const fc_cnt = _.session.pollFileChooserEvents(&fc_evts);
                        if (fc_cnt > 0) emitCdpEventsToFd(client_fd, &fc_evts, fc_cnt) catch break;
                    }
                    if (_.session.network_enabled) {
                        let net_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                        const net_cnt = _.session.pollNetworkEvents(&net_evts);
                        if (net_cnt > 0) emitCdpEventsToFd(client_fd, &net_evts, net_cnt) catch break;
                    }
                    {
                        let f_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                        let f_emit_ok = true;
                        while (true) {
                            const f_cnt = _.session.pollFetchEvents(&f_evts);
                            if (f_cnt == 0) break;
                            emitCdpEventsToFd(client_fd, &f_evts, f_cnt) catch {
                                f_emit_ok = false;
                                break;
                            };
                        }
                        if (!f_emit_ok) break;
                    }
                    if (_.session.dom_document_sent) {
                        _.flushDomEvents(client_fd) catch break;
                    }
                }
                if (_.session.dedicated_workers.cdp_event_count > 0) {
                    let w_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                    const w_cnt = _.session.dedicated_workers.pollCdpEvents(&w_evts);
                    if (w_cnt > 0) {
                        const main_sid = if (_.session.target_registry.targets[0].session_id_len > 0)
                            _.session.target_registry.targets[0].sessionId()
                        else
                            "";
                        if (main_sid.len > 0) {
                            let wi: usize = 0;
                            while (wi < w_cnt) : (wi += 1) {
                                if (w_evts[wi].session_id.len == 0) {
                                    w_evts[wi].session_id = main_sid;
                                }
                            }
                        }
                        emitCdpEventsToFd(client_fd, &w_evts, w_cnt) catch break;
                    }
                }
                let ti: usize = 0;
                while (ti < MAX_TAB_SESSIONS) : (ti += 1) {
                    if (_.tab_sessions[ti].active && _.tab_sessions[ti].page_session != null) {
                        const tab_sess = _.tab_sessions[ti].page_session.?;
                        const tab_sid = _.tab_sessions[ti].key_buf[0.._.tab_sessions[ti].key_len];
                        if (tab_sess.has_document) {
                            _.setActiveDialogSessionId(tab_sid);
                            tab_sess.pumpTimers();
                            flushReopenLoadEventsForSession(client_fd, tab_sess, tab_sid) catch break;
                            flushDeferredLoadEventsForSession(client_fd, tab_sess, tab_sid) catch break;
                            if (shouldSyncChildFrames(tab_sess)) {
                                let tcf_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                                let tcf_cnt: usize = 0;
                                emitNewChildFrameEvents(tab_sess, &tcf_evts, &tcf_cnt);
                                if (tcf_cnt > 0) {
                                    let tcfi: usize = 0;
                                    while (tcfi < tcf_cnt) : (tcfi += 1) {
                                        if (tcf_evts[tcfi].session_id.len == 0 && tab_sid.len > 0) tcf_evts[tcfi].session_id = tab_sid;
                                    }
                                    emitCdpEventsToFd(client_fd, &tcf_evts, tcf_cnt) catch break;
                                }
                            }
                            if (!tryStartAsyncPendingNav(nav_state, tab_sess, tab_sid)) {
                                flushPendingNavigationForSession(client_fd, tab_sess, tab_sid) catch break;
                            }
                            if (tab_sess.binding_count > 0) {
                                let tb_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                                const tb_cnt = tab_sess.pollBindingEvents(&tb_evts);
                                if (tb_cnt > 0) {
                                    let tbi: usize = 0;
                                    while (tbi < tb_cnt) : (tbi += 1) {
                                        if (tb_evts[tbi].session_id.len == 0 && tab_sid.len > 0) tb_evts[tbi].session_id = tab_sid;
                                    }
                                    emitCdpEventsToFd(client_fd, &tb_evts, tb_cnt) catch break;
                                }
                            }
                            if (tab_sess.intercept_file_chooser) {
                                let tfc_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                                const tfc_cnt = tab_sess.pollFileChooserEvents(&tfc_evts);
                                if (tfc_cnt > 0) {
                                    let tfci: usize = 0;
                                    while (tfci < tfc_cnt) : (tfci += 1) {
                                        if (tfc_evts[tfci].session_id.len == 0 && tab_sid.len > 0) tfc_evts[tfci].session_id = tab_sid;
                                    }
                                    emitCdpEventsToFd(client_fd, &tfc_evts, tfc_cnt) catch break;
                                }
                            }
                            if (tab_sess.network_enabled) {
                                let tnet_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                                const tnet_cnt = tab_sess.pollNetworkEvents(&tnet_evts);
                                if (tnet_cnt > 0) {
                                    let tneti: usize = 0;
                                    while (tneti < tnet_cnt) : (tneti += 1) {
                                        if (tnet_evts[tneti].session_id.len == 0 && tab_sid.len > 0) tnet_evts[tneti].session_id = tab_sid;
                                    }
                                    emitCdpEventsToFd(client_fd, &tnet_evts, tnet_cnt) catch break;
                                }
                            }
                            {
                                let tf_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                                let tf_emit_ok = true;
                                while (true) {
                                    const tf_cnt = tab_sess.pollFetchEvents(&tf_evts);
                                    if (tf_cnt == 0) break;
                                    let tfi: usize = 0;
                                    while (tfi < tf_cnt) : (tfi += 1) {
                                        if (tf_evts[tfi].session_id.len == 0 && tab_sid.len > 0) tf_evts[tfi].session_id = tab_sid;
                                    }
                                    emitCdpEventsToFd(client_fd, &tf_evts, tf_cnt) catch {
                                        tf_emit_ok = false;
                                        break;
                                    };
                                }
                                if (!tf_emit_ok) break;
                            }
                            if (tab_sess.dom_document_sent) {
                                flushDomEventsForSession(client_fd, tab_sess) catch break;
                            }
                        }
                        if (tab_sess.dedicated_workers.cdp_event_count > 0) {
                            let tw_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                            const tw_cnt = tab_sess.dedicated_workers.pollCdpEvents(&tw_evts);
                            if (tw_cnt > 0) {
                                let twi: usize = 0;
                                while (twi < tw_cnt) : (twi += 1) {
                                    if (tw_evts[twi].session_id.len == 0 && tab_sid.len > 0) tw_evts[twi].session_id = tab_sid;
                                }
                                emitCdpEventsToFd(client_fd, &tw_evts, tw_cnt) catch break;
                            }
                        }
                    }
                }
                let did_pump_main_sc = false;
                if (_.session.has_document && _.session.screencast_handler.canSendFrame()) {
                    did_pump_main_sc = _.pumpScreencastFrame(client_fd, tick_now_ms) catch false;
                }
                _.pumpAllSceneStreams(client_fd, tick_now_ms, did_pump_main_sc) catch break;
                if (pr == 0) continue;
            }
            if ((pfds[0].revents & (POLLERR | POLLNVAL)) != 0) {
                break;
            }
            if ((pfds[0].revents & (POLLIN | POLLERR | POLLNVAL)) == 0) {
                continue;
            }
            const frame_opt = readWsTextFrame(client_fd, frame_payload) catch break;
            if (payload := frame_opt) {
                if (payload.len == 0) continue;
                last_client_activity_ms = serverNowMs();
                const cdp_req = CdpRequest.parse(payload);

                if (std.mem.eql(u8, cdp_req.method, "Input.dispatchMouseEvent")) {
                    const target_sess = _.resolveSessionForRequest(cdp_req);
                    const sc_active = target_sess.screencast_handler.isActive();
                    const st_term_active = target_sess.facade.stream_mgr.isTerminalActive();
                    if (sc_active || st_term_active) {
                        const mtype = extractJsonString(cdp_req.params_raw, "type") ?? "";
                        if (std.mem.eql(u8, mtype, "mouseWheel")) {
                            const caller_sid = if (cdp_req.session_id.len > 0) cdp_req.session_id else target_sess.target_id;
                            let owner_match = false;
                            if (sc_active) {
                                const owner_sid = target_sess.screencast_handler.getOwnerSession();
                                if (owner_sid.len > 0 && std.mem.eql(u8, caller_sid, owner_sid)) {
                                    owner_match = true;
                                }
                            }
                            if (!owner_match && st_term_active) {
                                if (st := target_sess.facade.stream_mgr.getPrimaryStream()) {
                                    const st_owner = st.getOwnerSession();
                                    if (st_owner.len == 0 || std.mem.eql(u8, caller_sid, st_owner)) {
                                        owner_match = true;
                                    }
                                }
                            }
                            if (owner_match) {
                            const wx = extractJsonFloat(cdp_req.params_raw, "x") ?? 0.0;
                            const wy = extractJsonFloat(cdp_req.params_raw, "y") ?? 0.0;
                            const wdx = extractJsonFloat(cdp_req.params_raw, "deltaX") ?? 0.0;
                            const wdy = extractJsonFloat(cdp_req.params_raw, "deltaY") ?? 0.0;
                            const wmods = extractJsonInt(cdp_req.params_raw, "modifiers") ?? 0;
                            const wbtn = extractJsonString(cdp_req.params_raw, "button") ?? "none";
                            const wbtns = extractJsonInt(cdp_req.params_raw, "buttons") ?? 0;

                            if (pending_wheel.active && !pending_wheel.matches(wx, wy, wmods, wbtn, wbtns, cdp_req.session_id)) {
                                _.flushPendingWheelBatch(client_fd, pending_wheel) catch break;
                            }
                            if (!pending_wheel.active) {
                                pending_wheel.reset();
                                pending_wheel.active = true;
                                pending_wheel.x = wx;
                                pending_wheel.y = wy;
                                pending_wheel.modifiers = wmods;
                                pending_wheel.buttons = wbtns;
                                const blen = @min(wbtn.len, pending_wheel.button_buf.len);
                                if (blen > 0) @memcpy(pending_wheel.button_buf[0..blen], wbtn[0..blen]);
                                pending_wheel.button_len = blen;
                                const slen = @min(cdp_req.session_id.len, pending_wheel.session_id_buf.len);
                                if (slen > 0) @memcpy(pending_wheel.session_id_buf[0..slen], cdp_req.session_id[0..slen]);
                                pending_wheel.session_id_len = slen;
                            }
                            pending_wheel.delta_x += wdx;
                            pending_wheel.delta_y += wdy;
                            pending_wheel.addReply(cdp_req.id, cdp_req.raw_id);
                            continue;
                            }
                        }
                    }
                }

                if (pending_wheel.active) {
                    _.flushPendingWheelBatch(client_fd, pending_wheel) catch break;
                }

                // Intercept Page.navigate / Page.reload / Page.navigateToHistoryEntry when Fetch interception is enabled and emit Fetch.requestPaused
                if (!nav_state.pending_fetch_nav_active && (std.mem.eql(u8, cdp_req.method, "Page.navigate") || std.mem.eql(u8, cdp_req.method, "Page.reload") || std.mem.eql(u8, cdp_req.method, "Page.navigateToHistoryEntry")) && payload.len <= nav_state.pending_fetch_nav_payload_buf.len) {
                    const nav_sess = _.resolveSessionForRequest(cdp_req);
                    let nav_url: []const u8 = "";
                    let nav_ref: []const u8 = "";
                    let has_nav_url = false;
                    if (std.mem.eql(u8, cdp_req.method, "Page.navigate")) {
                        nav_url = extractJsonString(cdp_req.params_raw, "url") ?? "";
                        nav_ref = extractJsonString(cdp_req.params_raw, "referrer") ?? "";
                        has_nav_url = true;
                    } else if (std.mem.eql(u8, cdp_req.method, "Page.reload")) {
                        nav_url = if (nav_sess.url.len > 0) nav_sess.url else "about:blank";
                        has_nav_url = true;
                    } else {
                        const entry_id = extractJsonInt(cdp_req.params_raw, "entryId") ?? -1;
                        if (entry_id >= 0) {
                            let ei: usize = 0;
                            while (ei < nav_sess.nav_entry_count) : (ei += 1) {
                                if (nav_sess.nav_entries[ei].id == @as(i32, @intCast(entry_id))) {
                                    nav_url = nav_sess.nav_entries[ei].url;
                                    has_nav_url = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (has_nav_url && (nav_sess.fetch_enabled || nav_sess.fetch_state.enabled) && nav_sess.fetch_state.shouldIntercept(nav_url, "Document")) {
                        let already_decided = false;
                        if (existing_p := nav_sess.fetch_state.findParkedByUrl(nav_url)) {
                            if (existing_p.verdict != InterceptVerdict.None) already_decided = true;
                        }
                        if (!already_decided) {
                            let req_id_buf: [64]u8 = [0; 64];
                            const req_id = std.fmt.bufPrint(&req_id_buf, "fetch-nav-{d}", .{nav_sess.fetch_state.next_id}) catch "fetch-nav-1";
                            nav_sess.fetch_state.next_id += 1;
                            * = nav_sess.fetch_state.parkRequest(req_id, nav_url, "GET");
                            let net_id_buf: [64]u8 = [0; 64];
                            const next_net_id = std.fmt.bufPrint(&net_id_buf, "capsurf-loader-{d:0>3}", .{nav_sess.loader_seq + 1}) catch req_id;
                            let fevts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                            let fevt_count: usize = 0;
                            let esc_nav_buf: [2048]u8 = [0; 2048];
                            const esc_nav_url = escapeJsonString(nav_url, &esc_nav_buf);
                            let ua_esc_buf: [1024]u8 = [0; 1024];
                            const ua_val = if (nav_sess.user_agent.len > 0) nav_sess.user_agent else DEFAULT_HTTP_USER_AGENT;
                            const esc_ua = escapeJsonString(ua_val, &ua_esc_buf);
                            let ref_kv_buf: [2048]u8 = [0; 2048];
                            let ref_kv: []const u8 = "";
                            if (nav_ref.len > 0) {
                                let ref_esc_buf: [2048]u8 = [0; 2048];
                                const esc_ref = escapeJsonString(nav_ref, &ref_esc_buf);
                                ref_kv = std.fmt.bufPrint(&ref_kv_buf, ",\"Referer\":\"{s}\"", .{esc_ref}) catch "";
                            }
                            let has_accept_hdr = false;
                            let has_cookie_hdr = false;
                            let extra_kv_buf: [2048]u8 = [0; 2048];
                            let extra_kv: []const u8 = "";
                            const trimmed_eh = std.mem.trim(u8, nav_sess.extra_headers, " \t\r\n");
                            if (trimmed_eh.len > 2 && trimmed_eh[0] == '{' && trimmed_eh[trimmed_eh.len - 1] == '}') {
                                const inner_eh = std.mem.trim(u8, trimmed_eh[1 .. trimmed_eh.len - 1], " \t\r\n");
                                if (inner_eh.len > 0) {
                                    extra_kv = std.fmt.bufPrint(&extra_kv_buf, ",{s}", .{inner_eh}) catch "";
                                    let ai: usize = 0;
                                    while (ai + 8 <= inner_eh.len) : (ai += 1) {
                                        if (std.ascii.eqlIgnoreCase(inner_eh[ai .. ai + 8], "\"accept\"")) {
                                            has_accept_hdr = true;
                                        } else if (std.ascii.eqlIgnoreCase(inner_eh[ai .. ai + 8], "\"cookie\"")) {
                                            has_cookie_hdr = true;
                                        }
                                        if (has_accept_hdr && has_cookie_hdr) break;
                                    }
                                }
                            }
                            const accept_kv: []const u8 = if (has_accept_hdr) "" else ",\"Accept\":\"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\"";
                            let cookie_kv_buf: [2048]u8 = [0; 2048];
                            let cookie_kv: []const u8 = "";
                            if (!has_cookie_hdr) {
                                const cookie_hdr = nav_sess.getActiveCookieJar().cookieHeaderForUrl(nav_url, null, nav_sess.allocator);
                                if (cookie_hdr != null) {
                                    defer nav_sess.allocator.free(cookie_hdr.?);
                                    if (cookie_hdr.?.len > 0) {
                                        let cookie_esc_buf: [2048]u8 = [0; 2048];
                                        const esc_cookie = escapeJsonString(cookie_hdr.?, &cookie_esc_buf);
                                        cookie_kv = std.fmt.bufPrint(&cookie_kv_buf, ",\"Cookie\":\"{s}\"", .{esc_cookie}) catch "";
                                    }
                                }
                            }
                            let hdrs_buf: [4096]u8 = [0; 4096];
                            const headers_json = std.fmt.bufPrint(
                                &hdrs_buf,
                                "{{\"User-Agent\":\"{s}\"{s}{s}{s}{s}}}",
                                .{ esc_ua, accept_kv, ref_kv, cookie_kv, extra_kv },
                            ) catch "{}";
                            let nev_buf: [8192]u8 = [0; 8192];
                            if (nav_sess.network_enabled) {
                                const req_json = std.fmt.bufPrint(
                                    &nev_buf,
                                    "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{s}}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                                    .{ next_net_id, next_net_id, esc_nav_url, esc_nav_url, headers_json, nav_sess.frame_id },
                                ) catch "";
                                if (req_json.len > 0) {
                                    fevts[fevt_count] = CdpEvent.create("Network.requestWillBeSent", req_json);
                                    if (cdp_req.session_id.len > 0 && !std.mem.startsWith(u8, cdp_req.session_id, "capsurf-cdp-session-")) {
                                        fevts[fevt_count].session_id = cdp_req.session_id;
                                    }
                                    fevt_count += 1;
                                    markFetchNavPreAnnounced(next_net_id);
                                }
                            }
                            let fev_buf: [8192]u8 = [0; 8192];
                            const paused_json = std.fmt.bufPrint(
                                &fev_buf,
                                "{{\"requestId\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{s},\"initialPriority\":\"High\",\"referrerPolicy\":\"no-referrer\"}},\"frameId\":\"{s}\",\"resourceType\":\"Document\",\"networkId\":\"{s}\"}}",
                                .{ req_id, esc_nav_url, headers_json, nav_sess.frame_id, next_net_id },
                            ) catch "";
                            if (paused_json.len > 0) {
                                fevts[fevt_count] = CdpEvent{
                                    method: "Fetch.requestPaused",
                                    params_json: paused_json,
                                };
                                if (cdp_req.session_id.len > 0 && !std.mem.startsWith(u8, cdp_req.session_id, "capsurf-cdp-session-")) {
                                    fevts[fevt_count].session_id = cdp_req.session_id;
                                }
                                fevt_count += 1;
                            }
                            if (fevt_count > 0) {
                                emitCdpEventsToFd(client_fd, &fevts, fevt_count) catch break;
                            }
                            @memcpy(nav_state.pending_fetch_nav_payload_buf[0..payload.len], payload);
                            nav_state.pending_fetch_nav_payload_len = payload.len;
                            const rlen = @min(req_id.len, nav_state.pending_fetch_nav_req_id_buf.len);
                            @memcpy(nav_state.pending_fetch_nav_req_id_buf[0..rlen], req_id[0..rlen]);
                            nav_state.pending_fetch_nav_req_id_len = rlen;
                            nav_state.pending_fetch_nav_active = true;
                            continue;
                        }
                    }
                }

                // Intercept slow Page.navigate to keep navigate window non-blocking
                if (std.mem.eql(u8, cdp_req.method, "Page.navigate") && cdp_req.session_id.len > 0 && payload.len <= nav_state.pending_nav_payload_buf.len) {
                    const nav_url = extractJsonString(cdp_req.params_raw, "url") ?? "";
                    const delay_ms = extractSlowScriptDelayMs(nav_url);
                    if (delay_ms > 0) {
                        @memcpy(nav_state.pending_nav_payload_buf[0..payload.len], payload);
                        nav_state.pending_nav_payload_len = payload.len;
                        const slen = @min(cdp_req.session_id.len, nav_state.pending_nav_sid_buf.len);
                        @memcpy(nav_state.pending_nav_sid_buf[0..slen], cdp_req.session_id[0..slen]);
                        nav_state.pending_nav_sid_len = slen;
                        nav_state.pending_nav_tid_len = 0;
                        if (tid := _.session.target_registry.findTargetIdBySessionId(cdp_req.session_id)) {
                            const tlen = @min(tid.len, nav_state.pending_nav_tid_buf.len);
                            @memcpy(nav_state.pending_nav_tid_buf[0..tlen], tid[0..tlen]);
                            nav_state.pending_nav_tid_len = tlen;
                        }
                        nav_state.pending_nav_deadline_ms = serverNowMs() + delay_ms;
                        nav_state.pending_nav_active = true;
                        nav_state.pending_eval_count = 0;
                        continue;
                    }
                }

                // Queue Runtime.evaluate arriving during the navigate window for the same session
                if (nav_state.pending_nav_active && std.mem.eql(u8, cdp_req.method, "Runtime.evaluate") && cdp_req.session_id.len > 0 && std.mem.eql(u8, cdp_req.session_id, nav_state.pending_nav_sid_buf[0..nav_state.pending_nav_sid_len])) {
                    if (nav_state.pending_eval_count < nav_state.pending_evals.len && payload.len <= nav_state.pending_evals[nav_state.pending_eval_count].payload_buf.len) {
                        nav_state.pending_evals[nav_state.pending_eval_count].active = true;
                        nav_state.pending_evals[nav_state.pending_eval_count].req_id = cdp_req.id;
                        @memcpy(nav_state.pending_evals[nav_state.pending_eval_count].payload_buf[0..payload.len], payload);
                        nav_state.pending_evals[nav_state.pending_eval_count].payload_len = payload.len;
                        const slen = @min(cdp_req.session_id.len, nav_state.pending_evals[nav_state.pending_eval_count].session_id_buf.len);
                        @memcpy(nav_state.pending_evals[nav_state.pending_eval_count].session_id_buf[0..slen], cdp_req.session_id[0..slen]);
                        nav_state.pending_evals[nav_state.pending_eval_count].session_id_len = slen;
                        nav_state.pending_eval_count += 1;
                    }
                    continue;
                }

                // Intercept plain http:// Page.navigate for non-blocking concurrent navigation
                const async_nav_started = _.tryInterceptAsyncPageNav(client_fd, cdp_req, payload, nav_state) catch break;
                if (async_nav_started) {
                    continue;
                }

                const is_browser_close = std.mem.eql(u8, cdp_req.method, "Browser.close");
                _.dispatchCdpCommand(client_fd, cdp_req, pending_promises, nav_state) catch break;
                if (nav_state.pending_fetch_nav_active) {
                    const is_fetch_verdict = std.mem.eql(u8, cdp_req.method, "Fetch.continueRequest") ||
                        std.mem.eql(u8, cdp_req.method, "Fetch.fulfillRequest") ||
                        std.mem.eql(u8, cdp_req.method, "Fetch.failRequest");
                    const is_fetch_disable = std.mem.eql(u8, cdp_req.method, "Fetch.disable");
                    if (is_fetch_disable || (is_fetch_verdict && std.mem.eql(u8, extractJsonString(cdp_req.params_raw, "requestId") ?? "", nav_state.pending_fetch_nav_req_id_buf[0..nav_state.pending_fetch_nav_req_id_len]))) {
                        nav_state.pending_fetch_nav_active = false;
                        const fnav_req = CdpRequest.parse(nav_state.pending_fetch_nav_payload_buf[0..nav_state.pending_fetch_nav_payload_len]);
                        _.dispatchCdpCommand(client_fd, fnav_req, pending_promises, nav_state) catch break;
                    }
                }
                pumpPendingPromises(client_fd, pending_promises) catch break;
                if (is_browser_close) {
                    writeWsCloseFrame(client_fd, 1000, "Browser.close") catch {};
                    break;
                }
                const post_now_ms = serverNowMs();
                let did_post_sc = false;
                if (_.session.has_document && _.session.screencast_handler.canSendFrame()) {
                    if (!std.mem.eql(u8, cdp_req.method, "Page.startScreencast")) {
                        _.setActiveDialogSessionId("");
                        _.session.pumpTimers();
                    }
                    did_post_sc = _.pumpScreencastFrame(client_fd, post_now_ms) catch false;
                }
                _.pumpAllSceneStreams(client_fd, post_now_ms, did_post_sc) catch break;
            } else {
                break;
            }
        }
    }

    fn flushPendingWheelBatch(*_, client_fd: i32, batch: *PendingWheelBatch)! {
        if (!batch.active) return;
        const reply_count = batch.reply_count;
        let params_buf: [512]u8 = [0; 512];
        const params_raw = std.fmt.bufPrint(
            &params_buf,
            "{{\"type\":\"mouseWheel\",\"x\":{d:.4},\"y\":{d:.4},\"deltaX\":{d:.4},\"deltaY\":{d:.4},\"modifiers\":{d},\"button\":\"{s}\",\"buttons\":{d}}}",
            .{
                batch.x,
                batch.y,
                batch.delta_x,
                batch.delta_y,
                batch.modifiers,
                batch.button_buf[0..batch.button_len],
                batch.buttons,
            },
        ) catch "{}";
        const sid = batch.session_id_buf[0..batch.session_id_len];
        const res_buf = std.heap.c_allocator.alloc(u8, 4096) catch return;
        defer std.heap.c_allocator.free(res_buf);
        let events: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
        let event_count: usize = 0;
        const synth_req = CdpRequest{
            id: 0,
            method: "Input.dispatchMouseEvent",
            params_raw: params_raw,
            session_id: sid,
        };
        * = CdpDispatcher.dispatch(_.session, synth_req, res_buf, &events, &event_count);
        let ei: usize = 0;
        while (ei < event_count) : (ei += 1) {
            if (events[ei].session_id.len == 0 && sid.len > 0 &&
                !std.mem.startsWith(u8, sid, "capsurf-cdp-session-")) {
                events[ei].session_id = sid;
            }
        }
        emitCdpEventsToFd(client_fd, &events, event_count)?;

        let ri: usize = 0;
        while (ri < reply_count) : (ri += 1) {
            let reply_buf: [256]u8 = [0; 256];
            let msg_slice: []const u8 = "";
            if (batch.replies[ri].raw_id_len > 0) {
                const rid = batch.replies[ri].raw_id_buf[0..batch.replies[ri].raw_id_len];
                msg_slice = std.fmt.bufPrint(&reply_buf, "{{\"id\":\"{s}\",\"result\":{{}}}}", .{rid}) catch continue;
            } else {
                msg_slice = std.fmt.bufPrint(&reply_buf, "{{\"id\":{d},\"result\":{{}}}}", .{batch.replies[ri].req_id}) catch continue;
            }
            writeWsTextFrame(client_fd, msg_slice)?;
        }
        batch.reset();
    }

    static fn pumpPendingPromises(client_fd: i32, pending_promises: *[]PendingPromise)! {
        const res_buf = std.heap.c_allocator.alloc(u8, 262144) catch return;
        defer std.heap.c_allocator.free(res_buf);
        let ppi: usize = 0;
        while (ppi < pending_promises.len) : (ppi += 1) {
            if (!pending_promises[ppi].active) continue;
            let pp = &pending_promises[ppi];
            const sess = pp.target_sess orelse {
                pp.active = false;
                continue;
            };
            let res_len: usize = 0;
            const st = sess.tryResolveParkedPromiseSlot(pp.slot, pp.return_by_value, res_buf, &res_len);
            const sid = pp.session_id_buf[0..pp.session_id_len];
            // Flush binding & console events from this session
            let b_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            const b_cnt = sess.pollBindingEvents(&b_evts);
            if (b_cnt > 0) {
                let bi: usize = 0;
                while (bi < b_cnt) : (bi += 1) {
                    if (b_evts[bi].session_id.len == 0 && sid.len > 0) b_evts[bi].session_id = sid;
                }
                emitCdpEventsToFd(client_fd, &b_evts, b_cnt)?;
            }
            let c_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            const c_cnt = sess.pollConsoleEvents(&c_evts);
            if (c_cnt > 0) {
                let ci: usize = 0;
                while (ci < c_cnt) : (ci += 1) {
                    if (c_evts[ci].session_id.len == 0 && sid.len > 0) c_evts[ci].session_id = sid;
                }
                emitCdpEventsToFd(client_fd, &c_evts, c_cnt)?;
            }
            let f_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            while (true) {
                const f_cnt = sess.pollFetchEvents(&f_evts);
                if (f_cnt == 0) break;
                let fi: usize = 0;
                while (fi < f_cnt) : (fi += 1) {
                    if (f_evts[fi].session_id.len == 0 && sid.len > 0) f_evts[fi].session_id = sid;
                }
                emitCdpEventsToFd(client_fd, &f_evts, f_cnt)?;
            }
            if (st == 0) continue;

            sess.freeParkedPromiseSlot(pp.slot);
            pp.active = false;

            const has_sid = sid.len > 0;
            const rid = pp.raw_id_buf[0..pp.raw_id_len];
            if (st == 3) {
                let err_buf: [512]u8 = [0; 512];
                const err_slice = if (rid.len > 0) (if (has_sid) std.fmt.bufPrint(&err_buf, "{{\"id\":\"{s}\",\"error\":{{\"code\":-32000,\"message\":\"Cannot find context with specified id\"}},\"sessionId\":\"{s}\"}}", .{ rid, sid }) catch "" else std.fmt.bufPrint(&err_buf, "{{\"id\":\"{s}\",\"error\":{{\"code\":-32000,\"message\":\"Cannot find context with specified id\"}}}}", .{rid}) catch "") else (if (has_sid) std.fmt.bufPrint(&err_buf, "{{\"id\":{d},\"error\":{{\"code\":-32000,\"message\":\"Cannot find context with specified id\"}},\"sessionId\":\"{s}\"}}", .{ pp.req_id, sid }) catch "" else std.fmt.bufPrint(&err_buf, "{{\"id\":{d},\"error\":{{\"code\":-32000,\"message\":\"Cannot find context with specified id\"}}}}", .{pp.req_id}) catch "");
                if (err_slice.len > 0) writeWsTextFrame(client_fd, err_slice)?;
                continue;
            }
            const res_json = res_buf[0..res_len];
            const dyn_buf = std.heap.c_allocator.alloc(u8, res_json.len + rid.len + sid.len + 64) catch continue;
            defer std.heap.c_allocator.free(dyn_buf);
            const msg_slice = if (rid.len > 0) (if (has_sid) std.fmt.bufPrint(dyn_buf, "{{\"id\":\"{s}\",\"result\":{s},\"sessionId\":\"{s}\"}}", .{ rid, res_json, sid }) catch "" else std.fmt.bufPrint(dyn_buf, "{{\"id\":\"{s}\",\"result\":{s}}}", .{ rid, res_json }) catch "") else (if (has_sid) std.fmt.bufPrint(dyn_buf, "{{\"id\":{d},\"result\":{s},\"sessionId\":\"{s}\"}}", .{ pp.req_id, res_json, sid }) catch "" else std.fmt.bufPrint(dyn_buf, "{{\"id\":{d},\"result\":{s}}}", .{ pp.req_id, res_json }) catch "");
            if (msg_slice.len > 0) writeWsTextFrame(client_fd, msg_slice)?;
        }
    }

    fn dispatchCdpCommand(*_, client_fd: i32, req: CdpRequest, pending_promises: *[]PendingPromise, nav_state: *PendingNavState)! {
        const res_buf = std.heap.c_allocator.alloc(u8, 262144) catch return;
        defer std.heap.c_allocator.free(res_buf);
        let events: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
        let event_count: usize = 0;

        if (std.mem.eql(u8, req.method, "Target.disposeBrowserContext")) {
            const dc_id = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (dc_id.len > 0) {
                if (nav_state.pending_nav_active) {
                    const nav_tid = nav_state.pending_nav_tid_buf[0..nav_state.pending_nav_tid_len];
                    const nav_sid = nav_state.pending_nav_sid_buf[0..nav_state.pending_nav_sid_len];
                    if (_.targetBelongsToContext(nav_tid, dc_id) || _.sessionBelongsToContext(nav_sid, dc_id)) {
                        nav_state.pending_nav_active = false;
                        if (nav_state.pending_nav_payload_len > 0) {
                            const nav_req = CdpRequest.parse(nav_state.pending_nav_payload_buf[0..nav_state.pending_nav_payload_len]);
                            CdpServer.sendCdpErrorById(client_fd, nav_req.id, nav_req.raw_id, nav_req.session_id, -32000, "Target closed");
                        }
                        let ei: usize = 0;
                        while (ei < nav_state.pending_eval_count) : (ei += 1) {
                            if (nav_state.pending_evals[ei].active) {
                                nav_state.pending_evals[ei].active = false;
                                const esid = nav_state.pending_evals[ei].session_id_buf[0..nav_state.pending_evals[ei].session_id_len];
                                CdpServer.sendCdpErrorById(client_fd, nav_state.pending_evals[ei].req_id, "", esid, -32000, "Target closed");
                            }
                        }
                        nav_state.pending_eval_count = 0;
                    }
                }
                if (nav_state.pending_fetch_nav_active && nav_state.pending_fetch_nav_payload_len > 0) {
                    const fnav_req = CdpRequest.parse(nav_state.pending_fetch_nav_payload_buf[0..nav_state.pending_fetch_nav_payload_len]);
                    const fnav_bctx = extractJsonString(fnav_req.params_raw, "browserContextId") ?? "";
                    if (std.mem.eql(u8, fnav_bctx, dc_id) || _.sessionBelongsToContext(fnav_req.session_id, dc_id)) {
                        nav_state.pending_fetch_nav_active = false;
                        markFetchNavPreAnnounced("");
                        CdpServer.sendCdpErrorById(client_fd, fnav_req.id, fnav_req.raw_id, fnav_req.session_id, -32000, "Target closed");
                    }
                }
                let ppi_c: usize = 0;
                while (ppi_c < pending_promises.len) : (ppi_c += 1) {
                    if (pending_promises[ppi_c].active) {
                        const psid = pending_promises[ppi_c].session_id_buf[0..pending_promises[ppi_c].session_id_len];
                        let match_ctx = _.sessionBelongsToContext(psid, dc_id);
                        if (!match_ctx) {
                            if (ts_c := pending_promises[ppi_c].target_sess) {
                                const ts_bctx = ts_c.current_browser_context_id_buf[0..ts_c.current_browser_context_id_len];
                                if (ts_bctx.len > 0 && std.mem.eql(u8, ts_bctx, dc_id)) {
                                    match_ctx = true;
                                }
                            }
                        }
                        if (match_ctx) {
                            const prid = pending_promises[ppi_c].raw_id_buf[0..pending_promises[ppi_c].raw_id_len];
                            CdpServer.sendCdpErrorById(client_fd, pending_promises[ppi_c].req_id, prid, psid, -32000, "Target closed");
                            if (ts_c := pending_promises[ppi_c].target_sess) {
                                ts_c.freeParkedPromiseSlot(pending_promises[ppi_c].slot);
                            }
                            pending_promises[ppi_c].active = false;
                        }
                    }
                }
                _.deinitTabSessionByContext(dc_id);
                if (!_.hasActiveTabSessions()) {
                    _.net_scheduler.abortAll();
                    _.net_idle_tracker.resetForNavigation();
                }
            }
        } else if (std.mem.eql(u8, req.method, "Target.closeTarget")) {
            const ct_id = extractJsonString(req.params_raw, "targetId") ?? "";
            if (nav_state.pending_nav_active && ct_id.len > 0 && nav_state.pending_nav_tid_len > 0 && std.mem.eql(u8, ct_id, nav_state.pending_nav_tid_buf[0..nav_state.pending_nav_tid_len])) {
                nav_state.pending_nav_active = false;
                if (nav_state.pending_nav_payload_len > 0) {
                    const nav_req = CdpRequest.parse(nav_state.pending_nav_payload_buf[0..nav_state.pending_nav_payload_len]);
                    CdpServer.sendCdpErrorById(client_fd, nav_req.id, nav_req.raw_id, nav_req.session_id, -32000, "Target closed");
                }
                let ei: usize = 0;
                while (ei < nav_state.pending_eval_count) : (ei += 1) {
                    if (nav_state.pending_evals[ei].active) {
                        nav_state.pending_evals[ei].active = false;
                        const esid = nav_state.pending_evals[ei].session_id_buf[0..nav_state.pending_evals[ei].session_id_len];
                        CdpServer.sendCdpErrorById(client_fd, nav_state.pending_evals[ei].req_id, "", esid, -32000, "Target closed");
                    }
                }
                nav_state.pending_eval_count = 0;
            }
            if (t_idx := _.session.target_registry.findTargetIndex(ct_id)) {
                const c_sid = _.session.target_registry.targets[t_idx].sessionId();
                if (c_sid.len > 0) {
                    if (nav_state.pending_fetch_nav_active && nav_state.pending_fetch_nav_payload_len > 0) {
                        const fnav_req = CdpRequest.parse(nav_state.pending_fetch_nav_payload_buf[0..nav_state.pending_fetch_nav_payload_len]);
                        if (std.mem.eql(u8, fnav_req.session_id, c_sid)) {
                            nav_state.pending_fetch_nav_active = false;
                            markFetchNavPreAnnounced("");
                            CdpServer.sendCdpErrorById(client_fd, fnav_req.id, fnav_req.raw_id, fnav_req.session_id, -32000, "Target closed");
                        }
                    }
                    let ppi_c: usize = 0;
                    while (ppi_c < pending_promises.len) : (ppi_c += 1) {
                        if (pending_promises[ppi_c].active) {
                            const psid = pending_promises[ppi_c].session_id_buf[0..pending_promises[ppi_c].session_id_len];
                            if (std.mem.eql(u8, psid, c_sid)) {
                                const prid = pending_promises[ppi_c].raw_id_buf[0..pending_promises[ppi_c].raw_id_len];
                                CdpServer.sendCdpErrorById(client_fd, pending_promises[ppi_c].req_id, prid, psid, -32000, "Target closed");
                                if (ts_c := pending_promises[ppi_c].target_sess) {
                                    ts_c.freeParkedPromiseSlot(pending_promises[ppi_c].slot);
                                }
                                pending_promises[ppi_c].active = false;
                            }
                        }
                    }
                    _.deinitTabSessionByKey(c_sid);
                }
            }
            if (ct_id.len > 0) {
                _.deinitTabSessionMatch(ct_id, "", null);
            }
        }

        const target_sess = _.resolveSessionForRequest(req);
        if (target_sess == _.session) {
            _.setActiveDialogSessionId("");
        } else {
            _.setActiveDialogSessionId(req.session_id);
        }
        const resp = CdpDispatcher.dispatch(target_sess, req, res_buf, &events, &event_count);

        if (std.mem.eql(u8, req.method, "Fetch.continueRequest") ||
            std.mem.eql(u8, req.method, "Fetch.fulfillRequest") ||
            std.mem.eql(u8, req.method, "Fetch.failRequest") ||
            std.mem.eql(u8, req.method, "Fetch.disable"))
        {
            target_sess.pumpFetchVerdicts();
        }

        if (target_sess == _.session &&
            (std.mem.eql(u8, req.method, "Runtime.addBinding") || std.mem.eql(u8, req.method, "Runtime.removeBinding")))
        {
            const raw_bname = extractJsonString(req.params_raw, "name") ?? "";
            let bname_buf: [128]u8 = [0; 128];
            const bname = unescapeJsonString(raw_bname, &bname_buf);
            if (bname.len > 0) {
                let ti: usize = 0;
                while (ti < MAX_TAB_SESSIONS) : (ti += 1) {
                    if (_.tab_sessions[ti].active) {
                        if (ts := _.tab_sessions[ti].page_session) {
                            if (std.mem.eql(u8, req.method, "Runtime.addBinding")) {
                                ts.addBinding(bname);
                            } else {
                                ts.removeBinding(bname);
                            }
                        }
                    }
                }
            }
        }

        // Emit triggered events before the command response (matches Chrome CDP ordering)
        let i: usize = 0;
        while (i < event_count) : (i += 1) {
            if (events[i].session_id.len == 0 && req.session_id.len > 0 &&
                !std.mem.startsWith(u8, req.session_id, "capsurf-cdp-session-") &&
                !std.mem.startsWith(u8, events[i].method, "Target.") &&
                !std.mem.startsWith(u8, events[i].method, "Browser.")) {
                events[i].session_id = req.session_id;
            }
        }
        emitCdpEventsToFd(client_fd, &events, event_count)? ;

        {
            let f_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
            while (true) {
                const f_cnt = target_sess.pollFetchEvents(&f_evts);
                if (f_cnt == 0) break;
                let fi: usize = 0;
                while (fi < f_cnt) : (fi += 1) {
                    if (f_evts[fi].session_id.len == 0 && req.session_id.len > 0 &&
                        !std.mem.startsWith(u8, req.session_id, "capsurf-cdp-session-")) {
                        f_evts[fi].session_id = req.session_id;
                    }
                }
                emitCdpEventsToFd(client_fd, &f_evts, f_cnt)?;
            }
        }

        if (resp.isOk() && std.mem.eql(u8, resp.result_json, "{\"__capsurf_promise_pending\":true}")) {
            if (slot := target_sess.takeLastParkedPromiseSlot()) {
                let parked = false;
                let ppi_s: usize = 0;
                while (ppi_s < pending_promises.len) : (ppi_s += 1) {
                    if (!pending_promises[ppi_s].active) {
                        let pp = &pending_promises[ppi_s];
                        pp.active = true;
                        pp.req_id = req.id;
                        pp.raw_id_len = @min(req.raw_id.len, pp.raw_id_buf.len);
                        if (pp.raw_id_len > 0) @memcpy(pp.raw_id_buf[0..pp.raw_id_len], req.raw_id[0..pp.raw_id_len]);
                        pp.session_id_len = @min(req.session_id.len, pp.session_id_buf.len);
                        if (pp.session_id_len > 0) @memcpy(pp.session_id_buf[0..pp.session_id_len], req.session_id[0..pp.session_id_len]);
                        pp.target_sess = target_sess;
                        pp.slot = slot;
                        pp.return_by_value = extractJsonBool(req.params_raw, "returnByValue") ?? false;
                        parked = true;
                        break;
                    }
                }
                if (parked) return;
                target_sess.freeParkedPromiseSlot(slot);
            }
        }

        if (resp.isOk()) {
            const has_sid = req.session_id.len > 0;
            const needed_len = resp.result_json.len + resp.raw_id.len + req.session_id.len + 64;
            if (needed_len <= 4096) {
                let stack_buf: [4096]u8 = [0; 4096];
                let msg_slice: []const u8 = "";
                if (resp.raw_id.len > 0) {
                    if (has_sid) {
                        msg_slice = std.fmt.bufPrint(
                            &stack_buf,
                            "{{\"id\":\"{s}\",\"result\":{s},\"sessionId\":\"{s}\"}}",
                            .{ resp.raw_id, resp.result_json, req.session_id },
                        ) catch return;
                    } else {
                        msg_slice = std.fmt.bufPrint(
                            &stack_buf,
                            "{{\"id\":\"{s}\",\"result\":{s}}}",
                            .{ resp.raw_id, resp.result_json },
                        ) catch return;
                    }
                } else {
                    if (has_sid) {
                        msg_slice = std.fmt.bufPrint(
                            &stack_buf,
                            "{{\"id\":{d},\"result\":{s},\"sessionId\":\"{s}\"}}",
                            .{ resp.id, resp.result_json, req.session_id },
                        ) catch return;
                    } else {
                        msg_slice = std.fmt.bufPrint(
                            &stack_buf,
                            "{{\"id\":{d},\"result\":{s}}}",
                            .{ resp.id, resp.result_json },
                        ) catch return;
                    }
                }
                writeWsTextFrame(client_fd, msg_slice)? ;
            } else {
                const dyn_buf = std.heap.c_allocator.alloc(u8, needed_len) catch return;
                defer std.heap.c_allocator.free(dyn_buf);
                let msg_slice: []const u8 = "";
                if (resp.raw_id.len > 0) {
                    if (has_sid) {
                        msg_slice = std.fmt.bufPrint(
                            dyn_buf,
                            "{{\"id\":\"{s}\",\"result\":{s},\"sessionId\":\"{s}\"}}",
                            .{ resp.raw_id, resp.result_json, req.session_id },
                        ) catch return;
                    } else {
                        msg_slice = std.fmt.bufPrint(
                            dyn_buf,
                            "{{\"id\":\"{s}\",\"result\":{s}}}",
                            .{ resp.raw_id, resp.result_json },
                        ) catch return;
                    }
                } else {
                    if (has_sid) {
                        msg_slice = std.fmt.bufPrint(
                            dyn_buf,
                            "{{\"id\":{d},\"result\":{s},\"sessionId\":\"{s}\"}}",
                            .{ resp.id, resp.result_json, req.session_id },
                        ) catch return;
                    } else {
                        msg_slice = std.fmt.bufPrint(
                            dyn_buf,
                            "{{\"id\":{d},\"result\":{s}}}",
                            .{ resp.id, resp.result_json },
                        ) catch return;
                    }
                }
                writeWsTextFrame(client_fd, msg_slice)? ;
            }
        } else {
            let err_buf: [1280]u8 = [0; 1280];
            let msg: []const u8 = "";
            const has_sid = req.session_id.len > 0;
            if (resp.raw_id.len > 0) {
                if (has_sid) {
                    msg = std.fmt.bufPrint(
                        &err_buf,
                        "{{\"id\":\"{s}\",\"error\":{{\"code\":{d},\"message\":\"{s}\"}},\"sessionId\":\"{s}\"}}",
                        .{ resp.raw_id, resp.error_code, resp.error_message, req.session_id },
                    ) catch return;
                } else {
                    msg = std.fmt.bufPrint(
                        &err_buf,
                        "{{\"id\":\"{s}\",\"error\":{{\"code\":{d},\"message\":\"{s}\"}}}}",
                        .{ resp.raw_id, resp.error_code, resp.error_message },
                    ) catch return;
                }
            } else {
                if (has_sid) {
                    msg = std.fmt.bufPrint(
                        &err_buf,
                        "{{\"id\":{d},\"error\":{{\"code\":{d},\"message\":\"{s}\"}},\"sessionId\":\"{s}\"}}",
                        .{ resp.id, resp.error_code, resp.error_message, req.session_id },
                    ) catch return;
                } else {
                    msg = std.fmt.bufPrint(
                        &err_buf,
                        "{{\"id\":{d},\"error\":{{\"code\":{d},\"message\":\"{s}\"}}}}",
                        .{ resp.id, resp.error_code, resp.error_message },
                    ) catch return;
                }
            }
            writeWsTextFrame(client_fd, msg)? ;
        }

        // Flush any queued native DOM mutation events
        flushDomEventsForSession(client_fd, target_sess)?;
        // Check if JS evaluation triggered a location.href navigation
        const nav_sid = if (req.session_id.len > 0 && !std.mem.startsWith(u8, req.session_id, "capsurf-cdp-session-")) req.session_id else "";
        flushPendingNavigationForSession(client_fd, target_sess, nav_sid)?;

        if (std.mem.eql(u8, req.method, "Page.close")) {
            if (target_sess != _.session) {
                _.deinitTabSessionMatch("", req.session_id, target_sess);
            } else if (req.session_id.len > 0) {
                _.deinitTabSessionByKey(req.session_id);
            }
        }
    }
}

fn sendHttpResponse(client_fd: i32, status: []const u8, content_type: []const u8, body: []const u8)! {
    let hdr_buf: [1024]u8 = [0; 1024];
    const hdr = std.fmt.bufPrint(
        &hdr_buf,
        "HTTP/1.1 {s}\r\n" ++
        "Content-Type: {s}\r\n" ++
        "Content-Length: {d}\r\n" ++
        "Connection: close\r\n\r\n",
        .{ status, content_type, body.len },
    ) catch return error.BufPrintFailed;

    socketWriteAll(client_fd, hdr)?;
    if (body.len > 0) {
        socketWriteAll(client_fd, body)?;
    }
}

fn sendLandingPage(
    client_fd: i32,
    req_host: []const u8,
    http_scheme: []const u8,
    ws_scheme: []const u8,
    inspector_base: []const u8,
    no_inspector: bool,
    path_only_inspector: bool,
)! {
    const is_https = std.ascii.eqlIgnoreCase(http_scheme, "https");

    // Discovery target: host:port (with default port 443/80 when omitted)
    let disc_buf: [300]u8 = [0; 300];
    const discovery_target = if (std.mem.indexOfScalar(u8, req_host, ':') != null)
        req_host
    else
        (std.fmt.bufPrint(&disc_buf, "{s}:{s}", .{ req_host, if (is_https) "443" else "80" }) catch req_host);

    // Hosted example host: map localhost[:port] -> 127.0.0.1[:port]
    let ex_host_buf: [300]u8 = [0; 300];
    let example_host = req_host;
    if (!path_only_inspector) {
        if (std.ascii.eqlIgnoreCase(req_host, "localhost")) {
            example_host = "127.0.0.1";
        } else if (req_host.len > 10 && std.ascii.startsWithIgnoreCase(req_host, "localhost:")) {
            example_host = std.fmt.bufPrint(&ex_host_buf, "127.0.0.1{s}", .{ req_host[9..] }) catch req_host;
        }
    }

    let base_buf: [512]u8 = [0; 512];
    const resolved_base = if (path_only_inspector)
        (std.fmt.bufPrint(&base_buf, "{s}://{s}{s}", .{ http_scheme, req_host, inspector_base }) catch inspector_base)
    else
        inspector_base;

    // Extract host from resolved_base for the DevTools note
    let insp_host: []const u8 = "example.com";
    if (sp := std.mem.indexOf(u8, resolved_base, "://")) {
        const after = resolved_base[sp + 3..];
        if (sl := std.mem.indexOfScalar(u8, after, '/')) {
            insp_host = after[0..sl];
        } else {
            insp_host = after;
        }
    }

    const ex_titles: [4][]const u8 = [
        "Example Domain",
        "Wikipedia: Kitesurfing",
        "The Cloudflare Blog",
        "Browser Run docs",
    ];
    const ex_urls: [4][]const u8 = [
        "https://example.com/",
        "https://en.wikipedia.org/wiki/Kitesurfing",
        "https://blog.cloudflare.com/",
        "https://developers.cloudflare.com/browser-run/",
    ];

    let body_buf: [16384]u8 = [0; 16384];
    let pos: usize = 0;

    const head_html =
        "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n" ++
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n" ++
        "<title>Kitesurf</title>\n</head>\n<body>\n<h1>Kitesurf</h1>\n";
    @memcpy(body_buf[pos..pos + head_html.len], head_html);
    pos += head_html.len;

    if (no_inspector) {
        const s =
            "<p>This engine speaks the Chrome DevTools Protocol and serves no DevTools frontend. " ++
            "Connect your agent or DevTools client to the endpoints below.</p>\n";
        @memcpy(body_buf[pos..pos + s.len], s);
        pos += s.len;
    } else {
        const s1 =
            "<p>This engine speaks the Chrome DevTools Protocol. Enter a URL to open it in Chrome DevTools.</p>" ++
            "<form class=\"omnibox\" action=\"/\" method=\"get\" autocomplete=\"off\">\n" ++
            "<input id=\"inspect-url\" type=\"url\" name=\"url\" placeholder=\"Enter a URL to inspect\" required>\n" ++
            "<button type=\"submit\">Inspect</button>\n</form>\n" ++
            "<p>Or open an example page in DevTools:</p>\n<ul class=\"examples\">\n";
        @memcpy(body_buf[pos..pos + s1.len], s1);
        pos += s1.len;

        let ei: usize = 0;
        while (ei < ex_titles.len) : (ei += 1) {
            const title = ex_titles[ei];
            const page_url = ex_urls[ei];
            let enc_page_buf: [512]u8 = [0; 512];
            const enc_page = encodeUriComponent(page_url, &enc_page_buf);
            let ep_buf: [1024]u8 = [0; 1024];
            const ep = std.fmt.bufPrint(&ep_buf, "{s}/devtools/page/kitesurf?initialUrl={s}", .{ example_host, enc_page }) catch "";
            let enc_ep_buf: [1536]u8 = [0; 1536];
            const enc_ep = encodeUriComponent(ep, &enc_ep_buf);
            const li = std.fmt.bufPrint(
                body_buf[pos..],
                "<li><a href=\"{s}?{s}={s}\">{s}</a> <code>{s}</code></li>\n",
                .{ resolved_base, ws_scheme, enc_ep, title, page_url },
            ) catch "";
            pos += li.len;
        }

        const ul_end = "</ul>\n";
        @memcpy(body_buf[pos..pos + ul_end.len], ul_end);
        pos += ul_end.len;

        if (!path_only_inspector) {
            const note = std.fmt.bufPrint(
                body_buf[pos..],
                "<p class=\"muted\">DevTools loads from <code>{s}</code>.</p>\n",
                .{ insp_host },
            ) catch "";
            pos += note.len;
        }
    }

    const cdp_sec = std.fmt.bufPrint(
        body_buf[pos..],
        "<h2>Connect a CDP client</h2>\n<dl class=\"endpoints\">\n" ++
        "<dd><code>{s}://{s}/devtools/browser</code></dd>\n" ++
        "<dd><code>{s}://{s}/devtools/page/kitesurf</code></dd>\n" ++
        "<dd><a href=\"/json/version\"><code>{s}://{s}/json/version</code></a>, <a href=\"/json/list\"><code>{s}://{s}/json/list</code></a></dd>\n" ++
        "</dl>\n<pre><code>const browser = await puppeteer.connect({{\n  browserWSEndpoint: \"{s}://{s}/devtools/browser\",\n}});</code></pre>\n",
        .{ ws_scheme, req_host, ws_scheme, req_host, http_scheme, req_host, http_scheme, req_host, ws_scheme, req_host },
    ) catch "";
    pos += cdp_sec.len;

    if (!is_https) {
        const ci_sec = std.fmt.bufPrint(
            body_buf[pos..],
            "<h2>Use chrome://inspect</h2>\n<ol>\n" ++
            "<li>Open <code>chrome://inspect/#devices</code> in Chrome.</li>\n" ++
            "<li>Add <code>{s}</code>.</li>\n</ol>\n",
            .{ discovery_target },
        ) catch "";
        pos += ci_sec.len;
    }

    const tail_html = "</body>\n</html>\n";
    if (pos + tail_html.len <= body_buf.len) {
        @memcpy(body_buf[pos..pos + tail_html.len], tail_html);
        pos += tail_html.len;
    }

    const body = body_buf[0..pos];
    let hdr_buf: [1024]u8 = [0; 1024];
    const hdr = std.fmt.bufPrint(
        &hdr_buf,
        "HTTP/1.1 200 OK\r\n" ++
        "Content-Type: text/html; charset=utf-8\r\n" ++
        "Content-Security-Policy: default-src 'none'; style-src 'unsafe-inline'; base-uri 'none'; form-action 'self'; frame-ancestors 'none'\r\n" ++
        "X-Content-Type-Options: nosniff\r\n" ++
        "Content-Length: {d}\r\n" ++
        "Connection: close\r\n\r\n",
        .{ body.len },
    ) catch return error.BufPrintFailed;

    socketWriteAll(client_fd, hdr)?;
    if (body.len > 0) {
        socketWriteAll(client_fd, body)?;
    }
}

fn encodeUriComponent(src: []const u8, dst: []u8): []const u8 {
    const hex = "0123456789ABCDEF";
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const c = src[i];
        const is_unreserved = (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '!' || c == '~' || c == '*' || c == '\'' || c == '(' || c == ')';
        if (is_unreserved) {
            if (out_len < dst.len) {
                dst[out_len] = c;
                out_len += 1;
            }
        } else {
            if (out_len + 3 <= dst.len) {
                dst[out_len] = '%';
                dst[out_len + 1] = hex[@as(usize, (c >> 4) & 0x0F)];
                dst[out_len + 2] = hex[@as(usize, c & 0x0F)];
                out_len += 3;
            }
        }
    }
    return dst[0..out_len];
}

fn sendHttpRedirect(client_fd: i32, location: []const u8)! {
    let resp_buf: [2560]u8 = [0; 2560];
    const resp = std.fmt.bufPrint(
        &resp_buf,
        "HTTP/1.1 302 Found\r\n" ++
        "Location: {s}\r\n" ++
        "Content-Length: 0\r\n" ++
        "Connection: close\r\n\r\n",
        .{location},
    ) catch return error.BufPrintFailed;

    socketWriteAll(client_fd, resp)?;
}

fn computeSecWebSocketAccept(key: []const u8, out_buf: []u8)!usize {
    const magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    let hasher = std.crypto.hash.Sha1.init(.{});
    hasher.update(key);
    hasher.update(magic);
    let digest: [20]u8 = [0; 20];
    hasher.final(&digest);
    const encoder = std.base64.standard.Encoder;
    return encoder.encode(out_buf, &digest).len;
}

pub fn writeWsTextFrame(client_fd: i32, text: []const u8)! {
    let header_buf: [10]u8 = [0; 10];
    header_buf[0] = 0x81; // FIN + Text

    let header_len: usize = 2;
    if (text.len <= 125) {
        header_buf[1] = @as(u8, @truncate(text.len));
    } else if (text.len <= 0xFFFF) {
        header_buf[1] = 126;
        std.mem.writeInt(u16, header_buf[2..4][0..2], @as(u16, @truncate(text.len)), .big);
        header_len = 4;
    } else {
        header_buf[1] = 127;
        std.mem.writeInt(u64, header_buf[2..10][0..8], @as(u64, text.len), .big);
        header_len = 10;
    }

    socketWriteAll(client_fd, header_buf[0..header_len])? ;
    socketWriteAll(client_fd, text)? ;
}

pub fn writeWsCloseFrame(client_fd: i32, code: u16, reason: []const u8)! {
    let buf: [128]u8 = [0; 128];
    buf[0] = 0x88; // FIN + Close
    const payload_len = 2 + reason.len;
    buf[1] = @as(u8, @truncate(payload_len));
    std.mem.writeInt(u16, buf[2..4][0..2], code, .big);
    if (reason.len > 0) {
        @memcpy(buf[4..4 + reason.len], reason);
    }
    socketWriteAll(client_fd, buf[0..2 + payload_len])? ;
}

pub fn readWsTextFrame(client_fd: i32, payload_buf: []u8): ([]const u8)?! {
    let hdr: [2]u8 = [0; 2];
    const n = recv(client_fd, &hdr, 2, 0);
    if (n < 2) return null;

    const b0 = hdr[0];
    const b1 = hdr[1];
    const opcode = b0 & 0x0F;
    if (opcode == 0x08) {
        let close_resp: [2]u8 = [ 0x88, 0x00 ];
        * = socketWriteAll(client_fd, &close_resp) catch {};
        return null; // Close frame
    }

    const masked = (b1 & 0x80) != 0;
    const len7 = b1 & 0x7F;

    let payload_len: usize = len7;
    if (len7 == 126) {
        let ext: [2]u8 = [0; 2];
        * = socketReadAll(client_fd, &ext)? ;
        payload_len = std.mem.readInt(u16, ext[0..2], .big);
    } else if (len7 == 127) {
        let ext: [8]u8 = [0; 8];
        * = socketReadAll(client_fd, &ext)? ;
        payload_len = @as(usize, @truncate(std.mem.readInt(u64, ext[0..8], .big)));
    }

    if (payload_len > payload_buf.len) return error.PayloadTooLarge;

    let mask_key: [4]u8 = [0; 4];
    if (masked) {
        * = socketReadAll(client_fd, &mask_key)? ;
    }

    const read_len = socketReadAll(client_fd, payload_buf[0..payload_len])? ;
    if (read_len < payload_len) return error.IncompletePayload;

    if (masked) {
        let i: usize = 0;
        while (i < payload_len) : (i += 1) {
            payload_buf[i] ^= mask_key[i % 4];
        }
    }

    return payload_buf[0..payload_len];
}
