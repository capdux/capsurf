// src/cdp/server.cp
// CapSurf Chrome DevTools Protocol (CDP) Server
// Lightweight, native HTTP & RFC 6455 WebSocket server compliant with Puppeteer, Playwright & AI Agents.
// Uses direct POSIX C-FFI sockets for maximum concurrency, zero-GC, and zero Zig version fragility.

import { CdpBoxModel, CdpTarget, CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { CdpDispatcher } from "./dispatch.cp";
import { NodeId, NULL_NODE } from "../dom/dom.cp";

pub @c {
    struct SockAddrIn {
        sin_family: u16;
        sin_port: u16;
        sin_addr: u32;
        sin_zero: [8]u8;
    }

    fn socket(domain: i32, @"type": i32, protocol: i32): i32;
    fn bind(fd: i32, addr: *const SockAddrIn, len: u32): i32;
    fn listen(fd: i32, backlog: i32): i32;
    fn accept(fd: i32, addr: *!?, len: *!?): i32;
    fn connect(fd: i32, addr: *const SockAddrIn, len: u32): i32;
    fn send(fd: i32, buf: [*]const u8, len: usize, flags: i32): isize;
    fn recv(fd: i32, buf: [*]u8, len: usize, flags: i32): isize;
    fn open(pathname: [*c]const u8, flags: i32): i32;
    fn read(fd: i32, buf: [*]u8, count: usize): isize;
    fn close(fd: i32): i32;
    fn shutdown(fd: i32, how: i32): i32;
    fn setsockopt(fd: i32, level: i32, optname: i32, optval: *const i32, optlen: u32): i32;
}

pub const AF_INET: i32 = 2;
pub const SOCK_STREAM: i32 = 1;
pub const SOL_SOCKET: i32 = 1;
pub const SO_REUSEADDR: i32 = 2;

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

    fn handleWebSocketUpgrade(*_, client_fd: i32, req: []const u8)! {
        const marker = "sec-websocket-key";
        const pos = findHeaderCaseInsensitive(req, marker) orelse return error.MissingWsKey;
        let idx = pos + marker.len;
        while (idx < req.len and req[idx] != ':') : (idx += 1) {}
        if (idx < req.len and req[idx] == ':') {
            idx += 1;
        }
        while (idx < req.len and (req[idx] == ' ' or req[idx] == '\t')) : (idx += 1) {}
        const key_start = idx;
        while (idx < req.len and req[idx] != '\r' and req[idx] != '\n') : (idx += 1) {}
        const key = std.mem.trim(u8, req[key_start..idx], " \t\r\n");

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

        // Enter WebSocket Frame Processing Loop
        _.runWebSocketSession(client_fd)? ;
    }

    fn runWebSocketSession(*_, client_fd: i32)! {
        let frame_payload: [65536]u8 = [0; 65536];

        while (_.running) {
            const frame_opt = readWsTextFrame(client_fd, &frame_payload) catch break;
            if (payload := frame_opt) {
                if (payload.len == 0) continue;
                const cdp_req = CdpRequest.parse(payload);
                _.dispatchCdpCommand(client_fd, cdp_req) catch break;
            } else {
                break;
            }
        }
    }

    fn dispatchCdpCommand(*_, client_fd: i32, req: CdpRequest)! {
        let res_buf: [4096]u8 = [0; 4096];
        let events: [4]CdpEvent = [CdpEvent{}; 4];
        let event_count: usize = 0;

        const resp = CdpDispatcher.dispatch(_.session, req, &res_buf, &events, &event_count);

        if (resp.isOk()) {
            let stack_buf: [4096]u8 = [0; 4096];
            let msg_slice: []const u8 = "";
            let dyn_buf: []u8 = [];
            const needed_len = resp.result_json.len + 32;
            if (needed_len <= stack_buf.len) {
                msg_slice = std.fmt.bufPrint(
                    &stack_buf,
                    "{{\"id\":{d},\"result\":{s}}}",
                    .{ resp.id, resp.result_json },
                ) catch return;
            } else {
                dyn_buf = std.heap.c_allocator.alloc(u8, needed_len) catch return;
                msg_slice = std.fmt.bufPrint(
                    dyn_buf,
                    "{{\"id\":{d},\"result\":{s}}}",
                    .{ resp.id, resp.result_json },
                ) catch return;
            }
            writeWsTextFrame(client_fd, msg_slice)? ;
            if (dyn_buf.len > 0) {
                std.heap.c_allocator.free(dyn_buf);
            }
        } else {
            let err_buf: [1024]u8 = [0; 1024];
            const msg = std.fmt.bufPrint(
                &err_buf,
                "{{\"id\":{d},\"error\":{{\"code\":{d},\"message\":\"{s}\"}}}}",
                .{ resp.id, resp.error_code, resp.error_message },
            ) catch return;
            writeWsTextFrame(client_fd, msg)? ;
        }

        // Emit any triggered asynchronous events
        let i: usize = 0;
        while (i < event_count) : (i += 1) {
            let evt_buf: [1024]u8 = [0; 1024];
            const evt_msg = std.fmt.bufPrint(
                &evt_buf,
                "{{\"method\":\"{s}\",\"params\":{s}}}",
                .{ events[i].method, events[i].params_json },
            ) catch continue;
            writeWsTextFrame(client_fd, evt_msg)? ;
        }
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
