// src/cdp/server.cp
// CapSurf Chrome DevTools Protocol (CDP) Server
// Lightweight, native HTTP & RFC 6455 WebSocket server compliant with Puppeteer, Playwright & AI Agents.
// Uses direct POSIX C-FFI sockets for maximum concurrency, zero-GC, and zero Zig version fragility.

import { CdpBoxModel, CdpTarget, CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, extractJsonFloat, unescapeJsonString, MAX_CDP_EVENTS } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { CdpDispatcher } from "./dispatch.cp";
import { drainDomMutationEvents, ByteSlice } from "./dom.cp";
import { performPendingNavigation } from "./page.cp";
import { MockFixtures } from "../net/mock_fixtures.cp";
import { NodeId, NULL_NODE } from "../dom/dom.cp";
import { getMilliTimestamp } from "../js/dom_binding.cp";
import { Base64 } from "cap:encoding";
import { ScreencastHandler, formatScreencastFrameParams } from "./screencast.cp";
import { SceneStreamManager, MSG_SCENE, MSG_PARTIAL, SCENE_ENVELOPE_HEADER_BYTES, SCENE_PARTIAL_HEADER_BYTES } from "./scene_stream.cp";
import { SceneHostManifest, formatManifestJson } from "./scene_host.cp";
import { encodeRgbaRectToPng } from "../render/painter.cp";

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

fn serverNowMs(): i64 {
    return getMilliTimestamp();
}

pub struct PendingSlowEval {
    active: bool = false,
    req_id: i64 = 0,
    payload_buf: [4096]u8 = [0; 4096],
    payload_len: usize = 0,
    session_id_buf: [64]u8 = [0; 64],
    session_id_len: usize = 0,
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

const MAX_TAB_SESSIONS: usize = 16;

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

pub struct CdpServer {
    allocator: std.mem.Allocator,
    port: u16,
    listen_fd: i32 = -1,
    session: *CdpPageSession,
    tab_sessions: [MAX_TAB_SESSIONS]TabSessionSlot = [TabSessionSlot{}; MAX_TAB_SESSIONS],
    running: bool = false,

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
        return ptr;
    }

    pub fn deinit(*_) {
        _.stop();
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

    fn deinitTabSessionByContext(*_, bctx: []const u8) {
        if (bctx.len == 0) return;
        let i: usize = 0;
        while (i < MAX_TAB_SESSIONS) : (i += 1) {
            if (_.tab_sessions[i].active && _.tab_sessions[i].bctx_len > 0 && std.mem.eql(u8, _.tab_sessions[i].bctx_buf[0.._.tab_sessions[i].bctx_len], bctx)) {
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
                            _.tab_sessions[j].page_session.?.frame_id = _.tab_sessions[j].frame_id_buf[0..flen];
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
                if (t_idx := _.session.target_registry.findTargetIndex(tid)) {
                    if (_.session.target_registry.targets[t_idx].opener_id_len > 0) {
                        new_sess.has_opener = true;
                        const op_v = new_sess.ctx.eval("window.opener = window.opener || { closed: false };");
                        new_sess.ctx.freeValue(op_v);
                    }
                }
            } else {
                new_sess.frame_id = std.fmt.bufPrint(&_.tab_sessions[slot_idx].frame_id_buf, "capsurf-frame-tab-{d}", .{slot_idx + 2}) catch "capsurf-frame-001";
            }
        } else {
            new_sess.frame_id = std.fmt.bufPrint(&_.tab_sessions[slot_idx].frame_id_buf, "capsurf-frame-tab-{d}", .{slot_idx + 2}) catch "capsurf-frame-001";
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
                    if (nav_events[ei].session_id.len == 0) {
                        nav_events[ei].session_id = sid;
                    }
                }
            }
            emitCdpEventsToFd(client_fd, &nav_events, nav_event_count)?;
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
        let pending_initial_url = initial_url;
        let frame_payload = std.heap.c_allocator.alloc(u8, 2 * 1024 * 1024) catch return;
        defer std.heap.c_allocator.free(frame_payload);
        @memset(frame_payload, 0);

        const session_start_ms = serverNowMs();
        let last_client_activity_ms = session_start_ms;

        // Pending slow-navigation state for non-blocking navigate window
        let pending_nav_active = false;
        let pending_nav_deadline_ms: i64 = 0;
        let pending_nav_payload_buf = std.heap.c_allocator.alloc(u8, 4096) catch return;
        defer std.heap.c_allocator.free(pending_nav_payload_buf);
        @memset(pending_nav_payload_buf, 0);
        let pending_nav_payload_len: usize = 0;
        let pending_nav_sid_buf: [64]u8 = [0; 64];
        let pending_nav_sid_len: usize = 0;
        let pending_nav_tid_buf: [64]u8 = [0; 64];
        let pending_nav_tid_len: usize = 0;
        let pending_evals = std.heap.c_allocator.alloc(PendingSlowEval, 8) catch return;
        defer std.heap.c_allocator.free(pending_evals);
        let pei: usize = 0;
        while (pei < pending_evals.len) : (pei += 1) {
            pending_evals[pei] = PendingSlowEval{};
        }
        let pending_eval_count: usize = 0;
        let pending_wheel = std.heap.c_allocator.create(PendingWheelBatch) catch return;
        defer std.heap.c_allocator.destroy(pending_wheel);
        pending_wheel.* = PendingWheelBatch{};

        while (_.running) {
            const loop_now_ms = serverNowMs();
            if (max_session_ms > 0 && (loop_now_ms - session_start_ms) >= max_session_ms) {
                writeWsCloseFrame(client_fd, 1001, "Max session exceeded") catch {};
                break;
            }
            if (idle_ms > 0 && (loop_now_ms - last_client_activity_ms) >= idle_ms) {
                writeWsCloseFrame(client_fd, 1001, "Session idle timeout") catch {};
                break;
            }
            if (pending_nav_active && loop_now_ms >= pending_nav_deadline_ms) {
                pending_nav_active = false;
                const nav_req = CdpRequest.parse(pending_nav_payload_buf[0..pending_nav_payload_len]);
                _.dispatchCdpCommand(client_fd, nav_req) catch break;
                let ei: usize = 0;
                while (ei < pending_eval_count) : (ei += 1) {
                    if (pending_evals[ei].active) {
                        pending_evals[ei].active = false;
                        const ev_req = CdpRequest.parse(pending_evals[ei].payload_buf[0..pending_evals[ei].payload_len]);
                        _.dispatchCdpCommand(client_fd, ev_req) catch break;
                    }
                }
                pending_eval_count = 0;
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

            let pfds: [2]PollFd = [
                PollFd{ fd: client_fd, events: POLLIN, revents: 0 },
                PollFd{ fd: _.listen_fd, events: POLLIN, revents: 0 },
            ];
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
            const base_poll_ms: i32 = if (fast_poll) 5 else 15;
            const poll_ms: i32 = if (pending_wheel.active) 1 else base_poll_ms;
            const nfds: u64 = if (_.listen_fd >= 0) 2 else 1;
            const pr = poll(&pfds[0], nfds, poll_ms);
            if (pr < 0) {
                break;
            }
            if (_.listen_fd >= 0 && (pfds[1].revents & POLLIN) != 0) {
                const extra_fd = accept(_.listen_fd, null, null);
                if (extra_fd >= 0) {
                    let extra_buf: [4096]u8 = [0; 4096];
                    const en = recv(extra_fd, &extra_buf, extra_buf.len, 0);
                    if (en > 0) {
                        const extra_req = extra_buf[0..@as(usize, @intCast(en))];
                        _.handleHttpRequest(extra_fd, extra_req) catch {};
                    }
                    * = std.c.close(extra_fd);
                }
                if ((pfds[0].revents & (POLLIN | POLLERR | POLLNVAL)) == 0) {
                    continue;
                }
            }
            if (pr == 0) {
                if (pending_wheel.active) {
                    _.flushPendingWheelBatch(client_fd, pending_wheel) catch break;
                }
                const tick_now_ms = serverNowMs();
                if (_.session.has_document) {
                    _.session.pumpTimers();
                    _.flushReopenLoadEvents(client_fd) catch break;
                    _.flushPendingNavigation(client_fd) catch break;
                    if (_.session.dom_document_sent) {
                        _.flushDomEvents(client_fd) catch break;
                    }
                }
                let ti: usize = 0;
                while (ti < MAX_TAB_SESSIONS) : (ti += 1) {
                    if (_.tab_sessions[ti].active && _.tab_sessions[ti].page_session != null) {
                        const tab_sess = _.tab_sessions[ti].page_session.?;
                        const tab_sid = _.tab_sessions[ti].key_buf[0.._.tab_sessions[ti].key_len];
                        if (tab_sess.has_document) {
                            tab_sess.pumpTimers();
                            flushReopenLoadEventsForSession(client_fd, tab_sess, tab_sid) catch break;
                            flushPendingNavigationForSession(client_fd, tab_sess, tab_sid) catch break;
                            if (tab_sess.dom_document_sent) {
                                flushDomEventsForSession(client_fd, tab_sess) catch break;
                            }
                        }
                    }
                }
                let did_pump_main_sc = false;
                if (_.session.has_document && _.session.screencast_handler.canSendFrame()) {
                    did_pump_main_sc = _.pumpScreencastFrame(client_fd, tick_now_ms) catch false;
                }
                _.pumpAllSceneStreams(client_fd, tick_now_ms, did_pump_main_sc) catch break;
                continue;
            }
            if ((pfds[0].revents & (POLLERR | POLLNVAL)) != 0) {
                break;
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

                // Intercept slow Page.navigate to keep navigate window non-blocking
                if (std.mem.eql(u8, cdp_req.method, "Page.navigate") && cdp_req.session_id.len > 0 && payload.len <= pending_nav_payload_buf.len) {
                    const nav_url = extractJsonString(cdp_req.params_raw, "url") ?? "";
                    const delay_ms = extractSlowScriptDelayMs(nav_url);
                    if (delay_ms > 0) {
                        @memcpy(pending_nav_payload_buf[0..payload.len], payload);
                        pending_nav_payload_len = payload.len;
                        const slen = @min(cdp_req.session_id.len, pending_nav_sid_buf.len);
                        @memcpy(pending_nav_sid_buf[0..slen], cdp_req.session_id[0..slen]);
                        pending_nav_sid_len = slen;
                        pending_nav_tid_len = 0;
                        if (tid := _.session.target_registry.findTargetIdBySessionId(cdp_req.session_id)) {
                            const tlen = @min(tid.len, pending_nav_tid_buf.len);
                            @memcpy(pending_nav_tid_buf[0..tlen], tid[0..tlen]);
                            pending_nav_tid_len = tlen;
                        }
                        pending_nav_deadline_ms = serverNowMs() + delay_ms;
                        pending_nav_active = true;
                        pending_eval_count = 0;
                        continue;
                    }
                }

                // Queue Runtime.evaluate arriving during the navigate window for the same session
                if (pending_nav_active && std.mem.eql(u8, cdp_req.method, "Runtime.evaluate") && cdp_req.session_id.len > 0 && std.mem.eql(u8, cdp_req.session_id, pending_nav_sid_buf[0..pending_nav_sid_len])) {
                    if (pending_eval_count < pending_evals.len && payload.len <= pending_evals[pending_eval_count].payload_buf.len) {
                        pending_evals[pending_eval_count].active = true;
                        pending_evals[pending_eval_count].req_id = cdp_req.id;
                        @memcpy(pending_evals[pending_eval_count].payload_buf[0..payload.len], payload);
                        pending_evals[pending_eval_count].payload_len = payload.len;
                        const slen = @min(cdp_req.session_id.len, pending_evals[pending_eval_count].session_id_buf.len);
                        @memcpy(pending_evals[pending_eval_count].session_id_buf[0..slen], cdp_req.session_id[0..slen]);
                        pending_evals[pending_eval_count].session_id_len = slen;
                        pending_eval_count += 1;
                    }
                    continue;
                }

                // Cancel pending navigate and reject queued evals if Target.closeTarget closes the navigating target
                if (pending_nav_active && std.mem.eql(u8, cdp_req.method, "Target.closeTarget")) {
                    const close_tid = extractJsonString(cdp_req.params_raw, "targetId") ?? "";
                    if (close_tid.len > 0 && pending_nav_tid_len > 0 && std.mem.eql(u8, close_tid, pending_nav_tid_buf[0..pending_nav_tid_len])) {
                        pending_nav_active = false;
                        let ei: usize = 0;
                        while (ei < pending_eval_count) : (ei += 1) {
                            if (pending_evals[ei].active) {
                                pending_evals[ei].active = false;
                                let err_buf: [256]u8 = [0; 256];
                                if (err_msg := std.fmt.bufPrint(&err_buf, "{{\"id\":{d},\"error\":{{\"code\":-32000,\"message\":\"Target closed\"}}}}", .{pending_evals[ei].req_id}) catch null) {
                                    writeWsTextFrame(client_fd, err_msg) catch {};
                                }
                            }
                        }
                        pending_eval_count = 0;
                    }
                }

                const is_browser_close = std.mem.eql(u8, cdp_req.method, "Browser.close");
                _.dispatchCdpCommand(client_fd, cdp_req) catch break;
                if (is_browser_close) {
                    writeWsCloseFrame(client_fd, 1000, "Browser.close") catch {};
                    break;
                }
                const post_now_ms = serverNowMs();
                let did_post_sc = false;
                if (_.session.has_document && _.session.screencast_handler.canSendFrame()) {
                    if (!std.mem.eql(u8, cdp_req.method, "Page.startScreencast")) {
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

    fn dispatchCdpCommand(*_, client_fd: i32, req: CdpRequest)! {
        const res_buf = std.heap.c_allocator.alloc(u8, 262144) catch return;
        defer std.heap.c_allocator.free(res_buf);
        let events: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
        let event_count: usize = 0;

        if (std.mem.eql(u8, req.method, "Target.disposeBrowserContext")) {
            const dc_id = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (dc_id.len > 0) {
                _.deinitTabSessionByContext(dc_id);
            }
        } else if (std.mem.eql(u8, req.method, "Target.closeTarget")) {
            const ct_id = extractJsonString(req.params_raw, "targetId") ?? "";
            if (t_idx := _.session.target_registry.findTargetIndex(ct_id)) {
                const c_sid = _.session.target_registry.targets[t_idx].sessionId();
                if (c_sid.len > 0) {
                    _.deinitTabSessionByKey(c_sid);
                }
            }
        }

        const target_sess = _.resolveSessionForRequest(req);
        const resp = CdpDispatcher.dispatch(target_sess, req, res_buf, &events, &event_count);

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
                !std.mem.startsWith(u8, events[i].method, "Target.")) {
                events[i].session_id = req.session_id;
            }
        }
        emitCdpEventsToFd(client_fd, &events, event_count)? ;

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
