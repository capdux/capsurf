// src/net/http_loader.cp
// CapSurf Real HTTP/HTTPS & Data URL Navigation Loader
// Migrated from vendor/kitesurf/packages/engine/src/eval/outbound.ts and vendor/kitesurf/packages/engine/src/cdp/page.ts

import { Base64 } from "cap:encoding";

import { CdpPageSession } from "../cdp/session.cp";
import { BlockableRequest, ResourceType, RequestBlocker } from "./request_blocker.cp";
import { RequestContext } from "./cookies.cp";
import { InterceptVerdict } from "../cdp/fetch.cp";
import { MockFixtures } from "./mock_fixtures.cp";
import {
    SockAddrIn,
    AF_INET,
    SOCK_STREAM,
    socket,
    connect,
    close,
    recv,
    socketWriteAll,
    socketReadAll
} from "../cdp/server.cp";

pub struct HostAndPort {
    host: []const u8 = "",
    port: u16 = 80,
}

pub struct ParsedHttpUrl {
    host: []const u8 = "",
    port: u16 = 80,
    path: []const u8 = "/",
    secure: bool = false,
}

pub fn parseHttpUrl(url: []const u8): ParsedHttpUrl? {
    return HttpLoader.parseHttpUrl(url);
}

pub struct HttpLoaderResult {
    status_code: u32 = 200,
    final_url: []const u8 = "",
    html: []const u8 = "",
    content_type: []const u8 = "",
    content_disposition: ([]const u8)? = null,
    raw_body: []const u8 = "",
    raw_body_base64: bool = false,
}

let g_fresh_extra_headers: bool = false;

pub fn markFreshExtraHeaders() {
    g_fresh_extra_headers = true;
}

pub fn consumeFreshExtraHeaders(): bool {
    const v = g_fresh_extra_headers;
    g_fresh_extra_headers = false;
    return v;
}

pub struct HttpLoader {
    static fn hexVal(c: u8): u8? {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return null;
    }

    /// Decode a data: URL (either data:text/html,... or data:text/html;base64,...)
    pub static fn decodeDataUrl(url: []const u8, out_buf: []u8): ([]const u8)? {
        if (!std.mem.startsWith(u8, url, "data:")) {
            return null;
        }

        const comma_pos_opt = std.mem.indexOfScalar(u8, url, ',');
        if (comma_pos_opt == null) return null;
        const comma_pos = comma_pos_opt.?;
        if (comma_pos < 5) return null;

        const header = url[5..comma_pos];
        const payload = url[comma_pos + 1..];

        const is_base64 = std.mem.indexOf(u8, header, ";base64") != null || std.mem.endsWith(u8, header, ";base64");
        if (is_base64) {
            const decoded_len = Base64.decode(out_buf, payload) catch return null;
            return out_buf[0..decoded_len];
        }

        // Percent-decoding for non-base64 payload
        let out_pos: usize = 0;
        let i: usize = 0;
        while (i < payload.len) {
            if (payload[i] == '%' && i + 2 < payload.len) {
                if (h1 := HttpLoader.hexVal(payload[i + 1])) {
                    if (h2 := HttpLoader.hexVal(payload[i + 2])) {
                        if (out_pos >= out_buf.len) return null;
                        out_buf[out_pos] = (h1 << 4) | h2;
                        out_pos += 1;
                        i += 3;
                        continue;
                    }
                }
            }
            if (out_pos >= out_buf.len) return null;
            out_buf[out_pos] = payload[i];
            out_pos += 1;
            i += 1;
        }
        return out_buf[0..out_pos];
    }

    /// Parse an IPv4 string literal into 32-bit big-endian/native integer
    pub static fn parseIpv4(host: []const u8): u32? {
        if (std.mem.eql(u8, host, "localhost") || std.mem.eql(u8, host, "127.0.0.1")) {
            return 0x0100007F; // 127.0.0.1
        }

        let parts: [4]u8 = [0; 4];
        let part_idx: usize = 0;
        let cur_val: u32 = 0;
        let has_digit = false;

        for (ch of host) {
            if (ch == '.') {
                if (!has_digit || part_idx >= 3 || cur_val > 255) return null;
                parts[part_idx] = @intCast(cur_val);
                part_idx += 1;
                cur_val = 0;
                has_digit = false;
            } else if (ch >= '0' && ch <= '9') {
                cur_val = cur_val * 10 + @as(u32, ch - '0');
                has_digit = true;
            } else {
                return null;
            }
        }

        if (!has_digit || part_idx != 3 || cur_val > 255) return null;
        parts[3] = @intCast(cur_val);

        const ip = @as(u32, parts[0]) |
            (@as(u32, parts[1]) << 8) |
            (@as(u32, parts[2]) << 16) |
            (@as(u32, parts[3]) << 24);
        return ip;
    }

    /// Parse an HTTP or HTTPS URL into its host, port, path, and secure components.
    /// Supports port specifications (e.g. "http://127.0.0.1:9876/index.html")
    /// and IPv6 host literals (e.g. "http://[::1]:8080/path").
    pub static fn parseHttpUrl(url: []const u8): ParsedHttpUrl? {
        let secure = false;
        let default_port: u16 = 80;
        let rest: []const u8 = "";

        if (std.mem.startsWith(u8, url, "https://")) {
            secure = true;
            default_port = 443;
            rest = url[8..];
        } else if (std.mem.startsWith(u8, url, "http://")) {
            secure = false;
            default_port = 80;
            rest = url[7..];
        } else {
            return null;
        }

        if (rest.len == 0) return null;

        // Split authority from path / query / fragment
        let auth_end: usize = 0;
        while (auth_end < rest.len) : (auth_end += 1) {
            const ch = rest[auth_end];
            if (ch == '/' || ch == '?' || ch == '#') break;
        }

        const authority = rest[0..auth_end];
        if (authority.len == 0) return null;

        let host = authority;
        let port = default_port;

        // Check for port in authority: e.g. "127.0.0.1:9876" or "[::1]:8080"
        if (authority[0] == '[') {
            if (close_bracket := std.mem.indexOfScalar(u8, authority, ']')) {
                host = authority[1..close_bracket];
                if (close_bracket + 1 < authority.len && authority[close_bracket + 1] == ':') {
                    const port_str = authority[close_bracket + 2..];
                    port = std.fmt.parseInt(u16, port_str, 10) catch return null;
                }
            } else {
                return null;
            }
        } else if (colon_idx := std.mem.lastIndexOfScalar(u8, authority, ':')) {
            host = authority[0..colon_idx];
            const port_str = authority[colon_idx + 1..];
            if (port_str.len > 0) {
                port = std.fmt.parseInt(u16, port_str, 10) catch return null;
            }
        }

        if (host.len == 0) return null;

        let path: []const u8 = "/";
        if (auth_end < rest.len) {
            let path_end = auth_end;
            while (path_end < rest.len) : (path_end += 1) {
                if (rest[path_end] == '#') break;
            }
            const candidate_path = rest[auth_end..path_end];
            if (candidate_path.len > 0) {
                path = candidate_path;
            }
        }

        return ParsedHttpUrl{
            host: host,
            port: port,
            path: path,
            secure: secure,
        };
    }

    /// Extract port from hostname if present (e.g. "127.0.0.1:8080" -> 8080)
    pub static fn parseHostAndPort(raw_host: []const u8, default_port: u16): HostAndPort {
        if (colon_pos := std.mem.indexOfScalar(u8, raw_host, ':')) {
            const h = raw_host[0..colon_pos];
            const p_str = raw_host[colon_pos + 1..];
            const p = std.fmt.parseInt(u16, p_str, 10) catch default_port;
            return HostAndPort{ host: h, port: p };
        }
        return HostAndPort{ host: raw_host, port: default_port };
    }

    pub static fn socketReadHttp(fd: i32, buf: []u8): usize! {
        let received: usize = 0;
        while (received < buf.len) {
            const n = recv(fd, buf.ptr + received, buf.len - received, 0);
            if (n < 0) return error.ReadFailed;
            if (n == 0) break; // EOF (Server finished sending)
            received += @as(usize, @intCast(n));
        }
        return received;
    }

    /// Extract scheme://host[:port] origin from an HTTP(S) URL.
    pub static fn extractOrigin(url: []const u8): []const u8 {
        let scheme_len: usize = 0;
        if (std.mem.startsWith(u8, url, "https://")) {
            scheme_len = 8;
        } else if (std.mem.startsWith(u8, url, "http://")) {
            scheme_len = 7;
        } else {
            return "";
        }
        const rest = url[scheme_len..];
        let auth_end: usize = 0;
        while (auth_end < rest.len) : (auth_end += 1) {
            const ch = rest[auth_end];
            if (ch == '/' || ch == '?' || ch == '#') break;
        }
        if (auth_end == 0) return "";
        return url[0 .. scheme_len + auth_end];
    }

    pub static fn isSameOrigin(a: []const u8, b: []const u8): bool {
        const oa = HttpLoader.extractOrigin(a);
        const ob = HttpLoader.extractOrigin(b);
        if (oa.len == 0 || ob.len == 0) return false;
        return std.ascii.eqlIgnoreCase(oa, ob);
    }

    pub static fn isLoopbackHost(host: []const u8): bool {
        if (std.ascii.eqlIgnoreCase(host, "localhost") || std.ascii.endsWithIgnoreCase(host, ".localhost")) return true;
        if (std.mem.eql(u8, host, "::1") || std.mem.eql(u8, host, "[::1]")) return true;
        if (std.mem.startsWith(u8, host, "127.")) return true;
        return false;
    }

    pub static fn isLoopbackUrl(url: []const u8): bool {
        if (parsed := HttpLoader.parseHttpUrl(url)) {
            return HttpLoader.isLoopbackHost(parsed.host);
        }
        return false;
    }

    pub static fn isPublicHttpUrl(url: []const u8): bool {
        if (parsed := HttpLoader.parseHttpUrl(url)) {
            return !HttpLoader.isLoopbackHost(parsed.host);
        }
        return false;
    }

    pub static fn resolveRelativeUrl(base_url: []const u8, target: []const u8, out_buf: []u8): []const u8 {
        if (std.mem.startsWith(u8, target, "http://") || std.mem.startsWith(u8, target, "https://")) {
            const n = @min(target.len, out_buf.len);
            @memcpy(out_buf[0..n], target[0..n]);
            return out_buf[0..n];
        }
        const origin = HttpLoader.extractOrigin(base_url);
        if (origin.len == 0) {
            const n = @min(target.len, out_buf.len);
            @memcpy(out_buf[0..n], target[0..n]);
            return out_buf[0..n];
        }
        if (std.mem.startsWith(u8, target, "/")) {
            return std.fmt.bufPrint(out_buf, "{s}{s}", .{ origin, target }) catch target;
        }
        let base_dir = origin;
        if (last_slash := std.mem.lastIndexOfScalar(u8, base_url, '/')) {
            if (last_slash >= origin.len) {
                base_dir = base_url[0..last_slash];
            }
        }
        return std.fmt.bufPrint(out_buf, "{s}/{s}", .{ base_dir, target }) catch target;
    }

    pub static fn isMockRedirectUrl(url: []const u8): bool {
        let path_only = url;
        if (q_idx := std.mem.indexOfScalar(u8, path_only, '?')) {
            path_only = path_only[0..q_idx];
        }
        if (h_idx := std.mem.indexOfScalar(u8, path_only, '#')) {
            path_only = path_only[0..h_idx];
        }
        return std.mem.endsWith(u8, path_only, "/loud-redirect") ||
            std.mem.endsWith(u8, path_only, "/cors-redirect") ||
            std.mem.endsWith(u8, path_only, "/invalid-location-redirect");
    }

    pub static fn resolveMockRedirectTarget(url: []const u8, out_buf: []u8): ([]const u8)? {
        if (!HttpLoader.isMockRedirectUrl(url)) return null;
        let target_raw: []const u8 = "/__fixtures/echo-headers.json";
        let decoded_buf: [1024]u8 = undefined;
        if (raw_to := MockFixtures.extractQueryParam(url, "to")) {
            target_raw = MockFixtures.decodeUri(raw_to, &decoded_buf);
        }
        return HttpLoader.resolveRelativeUrl(url, target_raw, out_buf);
    }

    /// Perform HTTP/HTTPS fetch pipeline with cookie injection, interception, and Set-Cookie storage
    pub static fn fetchDocument(
        session: *CdpPageSession,
        url: []const u8,
        out_buf: []u8,
        redirect_depth: usize,
    ): (HttpLoaderResult)! {
        const fresh_headers = consumeFreshExtraHeaders();
        let init_origin: []const u8 = "";
        let strip_creds = false;
        if (!fresh_headers && HttpLoader.extractOrigin(session.url).len > 0) {
            init_origin = session.url;
            if (!HttpLoader.isSameOrigin(session.url, url)) {
                strip_creds = true;
            }
        }
        return HttpLoader.fetchDocumentWithContext(session, url, out_buf, redirect_depth, init_origin, strip_creds);
    }

    pub static fn fetchDocumentWithContext(
        session: *CdpPageSession,
        url: []const u8,
        out_buf: []u8,
        redirect_depth: usize,
        initiator_url: []const u8,
        strip_credentials: bool,
    ): (HttpLoaderResult)! {
        if (redirect_depth > 5) {
            return error.TooManyRedirects;
        }

        // 1. Check for about:blank or empty URL
        if (url.len == 0 || std.mem.eql(u8, url, "about:blank")) {
            const empty_doc = "<!DOCTYPE html><html><head></head><body></body></html>";
            return HttpLoaderResult{
                status_code: 200,
                final_url: "about:blank",
                html: empty_doc,
                content_type: "text/html",
                raw_body: empty_doc,
                raw_body_base64: false,
            };
        }

        // 2. Check for data: URLs
        if (std.mem.startsWith(u8, url, "data:")) {
            const comma_pos_opt = std.mem.indexOfScalar(u8, url, ',');
            if (comma_pos_opt == null) return error.InvalidDataUrl;
            const comma_pos = comma_pos_opt.?;
            if (comma_pos < 5) return error.InvalidDataUrl;
            const header = url[5..comma_pos];
            const payload = url[comma_pos + 1..];
            let is_base64 = false;
            let mime = header;
            if (b64_pos := std.mem.indexOf(u8, header, ";base64")) {
                is_base64 = true;
                mime = header[0..b64_pos];
            }
            if (mime.len == 0) {
                mime = "text/plain;charset=US-ASCII";
            }
            if (decoded := HttpLoader.decodeDataUrl(url, out_buf)) {
                return HttpLoaderResult{
                    status_code: 200,
                    final_url: url,
                    html: decoded,
                    content_type: mime,
                    raw_body: if (is_base64) payload else decoded,
                    raw_body_base64: is_base64,
                };
            }
            return error.InvalidDataUrl;
        }

        // 2.4. Follow mock redirect endpoints per-hop (Fetch §http-redirect-fetch)
        if (HttpLoader.isMockRedirectUrl(url)) {
            if (std.mem.indexOf(u8, url, "/loud-redirect") != null) {
                * = session.getActiveCookieJar().store(url, "hopsession=kitesurf-cross-origin-secret; Path=/", false, null);
            }
            let next_buf: [1024]u8 = undefined;
            if (next_slice := HttpLoader.resolveMockRedirectTarget(url, &next_buf)) {
                const next_url = session.allocator.dupe(u8, next_slice) catch return error.OutOfMemory;
                const cross_origin = !HttpLoader.isSameOrigin(url, next_url);
                const next_strip = strip_credentials || cross_origin;
                return HttpLoader.fetchDocumentWithContext(session, next_url, out_buf, redirect_depth + 1, url, next_strip);
            }
        }

        // 2.5. Check for test fixtures (e.g. echo-headers, static pages, dynamic endpoints)
        if (MockFixtures.isFixtureUrl(url)) {
            const ua = session.user_agent;
            const extra = session.extra_headers;
            const active_jar = session.getActiveCookieJar();
            let req_ctx = RequestContext {
                initiator_origin: initiator_url,
                top_level: true,
                method: "GET",
            };
            const ctx_ptr: (*const RequestContext)? = if (initiator_url.len > 0) &req_ctx else null;
            if (fixture_res := MockFixtures.handleFixtureRequest(url, "", ua, out_buf, active_jar, extra, ctx_ptr, strip_credentials)) {
                return HttpLoaderResult{
                    status_code: 200,
                    final_url: url,
                    html: fixture_res,
                    content_type: MockFixtures.getContentType(url),
                    content_disposition: MockFixtures.getContentDisposition(url),
                    raw_body: fixture_res,
                    raw_body_base64: false,
                };
            }
        }

        // 3. Parse HTTP/HTTPS URL
        const parsed_opt = HttpLoader.parseHttpUrl(url);
        if (parsed_opt == null) return error.InvalidUrl;
        const parsed = parsed_opt.?;

        // 4. Request Interception check (CDP Fetch domain)
        if ((session.fetch_enabled || session.fetch_state.enabled) && session.fetch_state.shouldIntercept(url, "Document")) {
            let parked = session.fetch_state.findParkedByUrl(url);
            if (parked == null) {
                parked = session.fetch_state.parkRequest("nav-req-001", url, "GET");
            }
            if (parked != null) {
                // If immediately fulfilled by a preset rule
                if (parked.?.verdict == InterceptVerdict.Fulfill) {
                    let mock_html = parked.?.mock_body;
                    if (decoded := HttpLoader.decodeDataUrl(mock_html, out_buf)) {
                        mock_html = decoded;
                    }
                    return HttpLoaderResult{
                        status_code: parked.?.mock_status,
                        final_url: url,
                        html: mock_html,
                    };
                }
                if (parked.?.verdict == InterceptVerdict.Fail) {
                    return error.RequestAborted;
                }
                return error.BlockedByClient;
            }
        }

        // 5. Connect to target host via TCP
        const ip_opt = HttpLoader.parseIpv4(parsed.host);
        const ip_addr = ip_opt ?? 0x0100007F; // Default to 127.0.0.1 for local testing
        const sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) return error.SocketCreationFailed;
        defer * = close(sock_fd);

        const addr = SockAddrIn{
            sin_family: @as(u16, @intCast(AF_INET)),
            sin_port: std.mem.nativeToBig(u16, parsed.port),
            sin_addr: ip_addr,
            sin_zero: [0; 8],
        };

        if (connect(sock_fd, &addr, @sizeOf(SockAddrIn)) < 0) {
            return error.ConnectionFailed;
        }

        // 6. Format HTTP/1.1 GET request with User-Agent, Extra-Headers, and Cookies
        let req_buf: [4096]u8 = undefined;
        let req_pos: usize = 0;

        let req_line_len: usize = 0;
        if ((parsed.secure && parsed.port != 443) || (!parsed.secure && parsed.port != 80)) {
            const req_line = std.fmt.bufPrint(
                req_buf[req_pos..],
                "GET {s} HTTP/1.1\r\nHost: {s}:{d}\r\nUser-Agent: {s}\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nAccept-Encoding: gzip, br\r\nConnection: close\r\n",
                .{ parsed.path, parsed.host, parsed.port, session.user_agent },
            ) catch return error.BufferOverflow;
            req_line_len = req_line.len;
        } else {
            const req_line = std.fmt.bufPrint(
                req_buf[req_pos..],
                "GET {s} HTTP/1.1\r\nHost: {s}\r\nUser-Agent: {s}\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nAccept-Encoding: gzip, br\r\nConnection: close\r\n",
                .{ parsed.path, parsed.host, session.user_agent },
            ) catch return error.BufferOverflow;
            req_line_len = req_line.len;
        }
        req_pos += req_line_len;

        // Append Extra Headers if configured
        if (session.extra_headers.len > 0 && !std.mem.eql(u8, session.extra_headers, "{}")) {
            // If extra headers are plain header lines, append them
            if (std.mem.indexOf(u8, session.extra_headers, ":") != null) {
                const add_len = @min(session.extra_headers.len, req_buf.len - req_pos - 2);
                @memcpy(req_buf[req_pos .. req_pos + add_len], session.extra_headers[0..add_len]);
                req_pos += add_len;
                if (!std.mem.endsWith(u8, session.extra_headers, "\r\n")) {
                    req_buf[req_pos] = '\r';
                    req_buf[req_pos + 1] = '\n';
                    req_pos += 2;
                }
            }
        }

        // Append Cookies from session.getActiveCookieJar()
        const cookie_hdr = session.getActiveCookieJar().cookieHeaderForUrl(url, null, session.allocator);
        if (cookie_hdr != null) {
            defer session.allocator.free(cookie_hdr.?);
            if (cookie_hdr.?.len > 0) {
                const c_line = std.fmt.bufPrint(
                    req_buf[req_pos..],
                    "Cookie: {s}\r\n",
                    .{ cookie_hdr.? },
                ) catch return error.BufferOverflow;
                req_pos += c_line.len;
            }
        }

        // End headers
        req_buf[req_pos] = '\r';
        req_buf[req_pos + 1] = '\n';
        req_pos += 2;

        socketWriteAll(sock_fd, req_buf[0..req_pos]) catch return error.SendFailed;

        // 7. Read HTTP response
        const total_read = HttpLoader.socketReadHttp(sock_fd, out_buf) catch return error.ReadFailed;
        if (total_read == 0) {
            return error.EmptyResponse;
        }

        const raw_resp = out_buf[0..total_read];

        // 8. Parse status code (e.g. "HTTP/1.1 200 OK")
        let status: u16 = 200;
        if (raw_resp.len >= 12 && std.mem.startsWith(u8, raw_resp, "HTTP/")) {
            let it = std.mem.splitScalar(u8, raw_resp, ' ');
            * = it.next();
            if (s_str := it.next()) {
                status = std.fmt.parseInt(u16, s_str, 10) catch 200;
            }
        }

        // 9. Inspect headers for Set-Cookie & Location
        const header_end = std.mem.indexOf(u8, raw_resp, "\r\n\r\n");
        const headers_part = if (header_end != null) raw_resp[0..header_end.?] else raw_resp;

        // Parse Set-Cookie, Content-Type, Content-Disposition lines
        let resp_content_type: []const u8 = "";
        let resp_content_disp: ([]const u8)? = null;
        let line_it = std.mem.splitSequence(u8, headers_part, "\r\n");
        while (line := line_it.next()) {
            if (line.len >= 11 && std.ascii.eqlIgnoreCase(line[0..11], "Set-Cookie:")) {
                const sc_val = std.mem.trim(u8, line[11..], " \t");
                const sc_dupe = session.allocator.dupe(u8, sc_val) catch "";
                if (sc_dupe.len > 0) {
                    * = session.getActiveCookieJar().setFromHeader(url, sc_dupe, null);
                }
            } else if (line.len >= 13 && std.ascii.eqlIgnoreCase(line[0..13], "Content-Type:")) {
                resp_content_type = std.mem.trim(u8, line[13..], " \t");
            } else if (line.len >= 20 && std.ascii.eqlIgnoreCase(line[0..20], "Content-Disposition:")) {
                resp_content_disp = std.mem.trim(u8, line[20..], " \t");
            }
        }

        // Handle HTTP Redirects (301, 302, 303, 307, 308)
        if (status == 301 || status == 302 || status == 303 || status == 307 || status == 308) {
            let loc_it = std.mem.splitSequence(u8, headers_part, "\r\n");
            while (line := loc_it.next()) {
                if (std.mem.startsWith(u8, line, "Location:") || std.mem.startsWith(u8, line, "location:")) {
                    const loc_val = std.mem.trim(u8, line[9..], " \t");
                    let next_buf: [1024]u8 = undefined;
                    const resolved = HttpLoader.resolveRelativeUrl(url, loc_val, &next_buf);
                    const next_url = session.allocator.dupe(u8, resolved) catch loc_val;
                    const next_strip = strip_credentials || !HttpLoader.isSameOrigin(url, next_url);
                    return HttpLoader.fetchDocumentWithContext(session, next_url, out_buf, redirect_depth + 1, url, next_strip);
                }
            }
        }

        // 10. Extract HTML body
        let body: []const u8 = "";
        if (idx := header_end) {
            body = raw_resp[idx + 4..];
        }

        return HttpLoaderResult{
            status_code: status,
            final_url: url,
            html: body,
            content_type: resp_content_type,
            content_disposition: resp_content_disp,
            raw_body: body,
            raw_body_base64: false,
        };
    }
}
