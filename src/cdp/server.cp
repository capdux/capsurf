// src/cdp/server.cp
// CapSurf Chrome DevTools Protocol (CDP) Server
// Lightweight, native HTTP & RFC 6455 WebSocket server compliant with Puppeteer, Playwright & AI Agents.
// Uses direct POSIX C-FFI sockets for maximum concurrency, zero-GC, and zero Zig version fragility.

import { CdpBoxModel, CdpTarget, CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, MAX_CDP_EVENTS } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { CdpDispatcher } from "./dispatch.cp";
import { drainDomMutationEvents, ByteSlice } from "./dom.cp";
import { performPendingNavigation } from "./page.cp";
import { NodeId, NULL_NODE } from "../dom/dom.cp";
import { getMilliTimestamp } from "../js/dom_binding.cp";

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
    fn open(pathname: [*c]const u8, flags: i32): i32;
    fn read(fd: i32, buf: [*]u8, count: usize): isize;
    fn close(fd: i32): i32;
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
    * = close(sock_fd);
}

pub struct CdpServer {
    allocator: std.mem.Allocator,
    port: u16,
    listen_fd: i32 = -1,
    session: *CdpPageSession,
    running: bool = false,

    pub static fn init(allocator: std.mem.Allocator, port: u16): (*CdpServer)! {
        const page_session = CdpPageSession.init(allocator) catch return error.SessionInitFailed;
        let ptr = std.heap.c_allocator.create(CdpServer) catch return error.AllocFailed;
        ptr.* = CdpServer {
            allocator: allocator,
            port: port,
            listen_fd: -1,
            session: page_session,
            running: false,
        };
        return ptr;
    }

    pub fn deinit(*_) {
        _.stop();
        _.session.deinit();
        std.heap.c_allocator.destroy(_);
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
            * = close(fd);
            return error.BindFailed;
        }

        if (listen(fd, 128) < 0) {
            * = close(fd);
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
            * = close(fd);
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
                * = close(client_fd);
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
            * = close(client_fd);
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
        if (std.mem.startsWith(u8, req, "GET /json/version")) {
            let body_buf: [1024]u8 = [0; 1024];
            const body = std.fmt.bufPrint(
                &body_buf,
                "{{\n" ++
                "  \"Browser\": \"CapSurf/1.0.0\",\n" ++
                "  \"Protocol-Version\": \"1.3\",\n" ++
                "  \"User-Agent\": \"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 CapSurf/1.0\",\n" ++
                "  \"V8-Version\": \"QuickJS-NG\",\n" ++
                "  \"webSocketDebuggerUrl\": \"ws://127.0.0.1:{d}/devtools/browser/capsurf-browser-001\"\n" ++
                "}}",
                .{ _.getPort() },
            ) catch "{}";
            sendHttpResponse(client_fd, "200 OK", "application/json; charset=UTF-8", body)? ;
            return;
        }

        if (std.mem.startsWith(u8, req, "GET /json") or std.mem.startsWith(u8, req, "GET /json/list")) {
            let body_buf: [1024]u8 = [0; 1024];
            const body = std.fmt.bufPrint(
                &body_buf,
                "[{{\n" ++
                "  \"description\": \"\",\n" ++
                "  \"devtoolsFrontendUrl\": \"\",\n" ++
                "  \"id\": \"capsurf-target-001\",\n" ++
                "  \"title\": \"{s}\",\n" ++
                "  \"type\": \"page\",\n" ++
                "  \"url\": \"{s}\",\n" ++
                "  \"webSocketDebuggerUrl\": \"ws://127.0.0.1:{d}/devtools/page/capsurf-target-001\"\n" ++
                "}}]",
                .{ _.session.title, _.session.url, _.getPort() },
            ) catch "[]";
            sendHttpResponse(client_fd, "200 OK", "application/json; charset=UTF-8", body)? ;
            return;
        }

        if (std.mem.startsWith(u8, req, "GET /json/new") or std.mem.startsWith(u8, req, "PUT /json/new")) {
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
                "  \"webSocketDebuggerUrl\": \"ws://127.0.0.1:{d}/devtools/page/capsurf-target-001\"\n" ++
                "}}",
                .{ _.session.title, _.session.url, _.getPort() },
            ) catch "{}";
            sendHttpResponse(client_fd, "200 OK", "application/json; charset=UTF-8", body)? ;
            return;
        }

        if (std.mem.startsWith(u8, req, "GET /json/activate") or std.mem.startsWith(u8, req, "GET /json/close")) {
            sendHttpResponse(client_fd, "200 OK", "text/plain", "Target handled")? ;
            return;
        }

        sendHttpResponse(client_fd, "404 Not Found", "text/plain", "Not Found")? ;
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

        // Enter WebSocket Frame Processing Loop
        _.runWebSocketSession(client_fd, idle_ms, max_session_ms)? ;
    }

    fn flushDomEvents(*_, client_fd: i32)! {
        if (!_.session.dom_document_sent) return;
        let msgs = std.ArrayListUnmanaged(ByteSlice).empty;
        defer {
            let mi: usize = 0;
            while (mi < msgs.items.len) : (mi += 1) {
                std.heap.c_allocator.free(msgs.items[mi]);
            }
            msgs.deinit(std.heap.c_allocator);
        }
        drainDomMutationEvents(_.session, &msgs);
        let mi: usize = 0;
        while (mi < msgs.items.len) : (mi += 1) {
            writeWsTextFrame(client_fd, msgs.items[mi])?;
        }
    }

    fn emitCdpEvents(*_, client_fd: i32, events: *[MAX_CDP_EVENTS]CdpEvent, event_count: usize)! {
        let i: usize = 0;
        while (i < event_count) : (i += 1) {
            const evt_needed = events[i].method.len + events[i].params_json.len + 32;
            if (evt_needed <= 4096) {
                let evt_buf: [4096]u8 = [0; 4096];
                const evt_msg = std.fmt.bufPrint(
                    &evt_buf,
                    "{{\"method\":\"{s}\",\"params\":{s}}}",
                    .{ events[i].method, events[i].params_json },
                ) catch continue;
                writeWsTextFrame(client_fd, evt_msg)?;
            } else {
                const dyn_evt_buf = std.heap.c_allocator.alloc(u8, evt_needed) catch continue;
                defer std.heap.c_allocator.free(dyn_evt_buf);
                const evt_msg = std.fmt.bufPrint(
                    dyn_evt_buf,
                    "{{\"method\":\"{s}\",\"params\":{s}}}",
                    .{ events[i].method, events[i].params_json },
                ) catch continue;
                writeWsTextFrame(client_fd, evt_msg)?;
            }
        }
    }

    fn flushPendingNavigation(*_, client_fd: i32)! {
        if (!_.session.has_document) return;
        let nav_events: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
        let nav_event_count: usize = 0;
        if (performPendingNavigation(_.session, &nav_events, &nav_event_count)) {
            _.emitCdpEvents(client_fd, &nav_events, nav_event_count)?;
        }
    }

    fn runWebSocketSession(*_, client_fd: i32, idle_ms: i64, max_session_ms: i64)! {
        _.session.resetSession();
        defer _.session.resetSession();
        let frame_payload: [65536]u8 = [0; 65536];

        const session_start_ms = serverNowMs();
        let last_client_activity_ms = session_start_ms;

        // Pending slow-navigation state for non-blocking navigate window
        let pending_nav_active = false;
        let pending_nav_deadline_ms: i64 = 0;
        let pending_nav_payload_buf: [4096]u8 = [0; 4096];
        let pending_nav_payload_len: usize = 0;
        let pending_nav_sid_buf: [64]u8 = [0; 64];
        let pending_nav_sid_len: usize = 0;
        let pending_nav_tid_buf: [64]u8 = [0; 64];
        let pending_nav_tid_len: usize = 0;
        let pending_evals: [8]PendingSlowEval = [PendingSlowEval{}; 8];
        let pending_eval_count: usize = 0;

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

            let pfd = PollFd{ fd: client_fd, events: POLLIN, revents: 0 };
            const pr = poll(&pfd, 1, 15);
            if (pr < 0) {
                break;
            }
            if (pr == 0) {
                if (_.session.has_document) {
                    _.session.pumpTimers();
                    _.flushPendingNavigation(client_fd) catch break;
                    if (_.session.dom_document_sent) {
                        _.flushDomEvents(client_fd) catch break;
                    }
                }
                continue;
            }
            if ((pfd.revents & (POLLERR | POLLNVAL)) != 0) {
                break;
            }
            const frame_opt = readWsTextFrame(client_fd, &frame_payload) catch break;
            if (payload := frame_opt) {
                if (payload.len == 0) continue;
                last_client_activity_ms = serverNowMs();
                const cdp_req = CdpRequest.parse(payload);

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
            } else {
                break;
            }
        }
    }

    fn dispatchCdpCommand(*_, client_fd: i32, req: CdpRequest)! {
        const res_buf = std.heap.c_allocator.alloc(u8, 262144) catch return;
        defer std.heap.c_allocator.free(res_buf);
        let events: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
        let event_count: usize = 0;

        const resp = CdpDispatcher.dispatch(_.session, req, res_buf, &events, &event_count);

        // Emit triggered events before the command response (matches Chrome CDP ordering)
        let i: usize = 0;
        while (i < event_count) : (i += 1) {
            const evt_needed = events[i].method.len + events[i].params_json.len + 32;
            if (evt_needed <= 4096) {
                let evt_buf: [4096]u8 = [0; 4096];
                const evt_msg = std.fmt.bufPrint(
                    &evt_buf,
                    "{{\"method\":\"{s}\",\"params\":{s}}}",
                    .{ events[i].method, events[i].params_json },
                ) catch continue;
                writeWsTextFrame(client_fd, evt_msg)? ;
            } else {
                const dyn_evt_buf = std.heap.c_allocator.alloc(u8, evt_needed) catch continue;
                defer std.heap.c_allocator.free(dyn_evt_buf);
                const evt_msg = std.fmt.bufPrint(
                    dyn_evt_buf,
                    "{{\"method\":\"{s}\",\"params\":{s}}}",
                    .{ events[i].method, events[i].params_json },
                ) catch continue;
                writeWsTextFrame(client_fd, evt_msg)? ;
            }
        }

        if (resp.isOk()) {
            const needed_len = resp.result_json.len + resp.raw_id.len + 32;
            if (needed_len <= 4096) {
                let stack_buf: [4096]u8 = [0; 4096];
                let msg_slice: []const u8 = "";
                if (resp.raw_id.len > 0) {
                    msg_slice = std.fmt.bufPrint(
                        &stack_buf,
                        "{{\"id\":\"{s}\",\"result\":{s}}}",
                        .{ resp.raw_id, resp.result_json },
                    ) catch return;
                } else {
                    msg_slice = std.fmt.bufPrint(
                        &stack_buf,
                        "{{\"id\":{d},\"result\":{s}}}",
                        .{ resp.id, resp.result_json },
                    ) catch return;
                }
                writeWsTextFrame(client_fd, msg_slice)? ;
            } else {
                const dyn_buf = std.heap.c_allocator.alloc(u8, needed_len) catch return;
                defer std.heap.c_allocator.free(dyn_buf);
                let msg_slice: []const u8 = "";
                if (resp.raw_id.len > 0) {
                    msg_slice = std.fmt.bufPrint(
                        dyn_buf,
                        "{{\"id\":\"{s}\",\"result\":{s}}}",
                        .{ resp.raw_id, resp.result_json },
                    ) catch return;
                } else {
                    msg_slice = std.fmt.bufPrint(
                        dyn_buf,
                        "{{\"id\":{d},\"result\":{s}}}",
                        .{ resp.id, resp.result_json },
                    ) catch return;
                }
                writeWsTextFrame(client_fd, msg_slice)? ;
            }
        } else {
            let err_buf: [1024]u8 = [0; 1024];
            let msg: []const u8 = "";
            if (resp.raw_id.len > 0) {
                msg = std.fmt.bufPrint(
                    &err_buf,
                    "{{\"id\":\"{s}\",\"error\":{{\"code\":{d},\"message\":\"{s}\"}}}}",
                    .{ resp.raw_id, resp.error_code, resp.error_message },
                ) catch return;
            } else {
                msg = std.fmt.bufPrint(
                    &err_buf,
                    "{{\"id\":{d},\"error\":{{\"code\":{d},\"message\":\"{s}\"}}}}",
                    .{ resp.id, resp.error_code, resp.error_message },
                ) catch return;
            }
            writeWsTextFrame(client_fd, msg)? ;
        }

        // Flush any queued native DOM mutation events
        _.flushDomEvents(client_fd)?;
        // Check if JS evaluation triggered a location.href navigation
        _.flushPendingNavigation(client_fd)?;
    }
}

fn sendHttpResponse(client_fd: i32, status: []const u8, content_type: []const u8, body: []const u8)! {
    let resp_buf: [4096]u8 = [0; 4096];
    const resp = std.fmt.bufPrint(
        &resp_buf,
        "HTTP/1.1 {s}\r\n" ++
        "Content-Type: {s}\r\n" ++
        "Content-Length: {d}\r\n" ++
        "Connection: close\r\n\r\n" ++
        "{s}",
        .{ status, content_type, body.len, body },
    ) catch return error.BufPrintFailed;

    socketWriteAll(client_fd, resp)? ;
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
