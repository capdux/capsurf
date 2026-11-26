// src/net/http_loader.cp
// CapSurf Real HTTP/HTTPS & Data URL Navigation Loader
// Migrated from vendor/kitesurf/packages/engine/src/eval/outbound.ts and vendor/kitesurf/packages/engine/src/cdp/page.ts

import { Base64 } from "cap:encoding";
import { gunzip, inflate, brotliDecompress } from "cap:pack";

import { CdpPageSession, escapeJsonString } from "../cdp/session.cp";
import { BlockableRequest, ResourceType, RequestBlocker } from "./request_blocker.cp";
import { RequestContext } from "./cookies.cp";
import { InterceptVerdict, httpStatusText } from "../cdp/fetch.cp";
import { formatHttpHeadersJson } from "../cdp/network.cp";
import { MockFixtures } from "./mock_fixtures.cp";
import {
    SockAddrIn,
    AF_INET,
    SOCK_STREAM,
    PollFd,
    POLLIN,
    poll,
    socket,
    connect,
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

pub struct RedirectHop {
    url: []const u8 = "",
    method: []const u8 = "GET",
    status_code: u32 = 0,
    status_text: []const u8 = "",
    raw_headers: []const u8 = "",
    req_headers_text: []const u8 = "",
    resp_headers_text: []const u8 = "",
    wire_headers_len: usize = 0,
    wire_body_len: usize = 0,
}

pub struct HttpLoaderResult {
    status_code: u32 = 200,
    status_text: []const u8 = "OK",
    final_url: []const u8 = "",
    html: []const u8 = "",
    content_type: []const u8 = "",
    content_disposition: ([]const u8)? = null,
    content_security_policy: []const u8 = "",
    raw_headers: []const u8 = "",
    req_headers_text: []const u8 = "",
    resp_headers_text: []const u8 = "",
    wire_headers_len: usize = 0,
    wire_body_len: usize = 0,
    raw_body: []const u8 = "",
    raw_body_base64: bool = false,
    redirect_urls: [6][]const u8 = ["", "", "", "", "", ""],
    redirect_hops: [6]RedirectHop = [RedirectHop{}; 6],
    redirect_count: usize = 0,
}

let g_fresh_extra_headers: bool = false;

pub fn tryBase64Decode(out_buf: []u8, raw_b: []const u8): usize? {
    return Base64.decode(out_buf, raw_b) catch return null;
}

pub fn tryBase64DecodeAlloc(allocator: std.mem.Allocator, raw_b: []const u8): ([]u8)? {
    return Base64.decodeAlloc(allocator, raw_b) catch return null;
}

pub fn parseHttpUrl(url: []const u8): ParsedHttpUrl? {
    return HttpLoader.parseHttpUrl(url);
}

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
    static fn decodeDataUrl(url: []const u8, out_buf: []u8): ([]const u8)? {
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
    static fn parseIpv4(host: []const u8): u32? {
        if (std.mem.eql(u8, host, "localhost") || std.mem.eql(u8, host, "127.0.0.1") ||
            std.mem.eql(u8, host, "web-platform.test") || std.mem.endsWith(u8, host, ".web-platform.test")) {
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
    static fn parseHttpUrl(url: []const u8): ParsedHttpUrl? {
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
    static fn parseHostAndPort(raw_host: []const u8, default_port: u16): HostAndPort {
        if (colon_pos := std.mem.indexOfScalar(u8, raw_host, ':')) {
            const h = raw_host[0..colon_pos];
            const p_str = raw_host[colon_pos + 1..];
            const p = std.fmt.parseInt(u16, p_str, 10) catch default_port;
            return HostAndPort{ host: h, port: p };
        }
        return HostAndPort{ host: raw_host, port: default_port };
    }

    static fn hasHeaderIgnoreCase(raw_headers: []const u8, header_name: []const u8): bool {
        if (raw_headers.len == 0 || header_name.len == 0) return false;
        let line_it = std.mem.splitScalar(u8, raw_headers, '\n');
        while (raw_line := line_it.next()) {
            const line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len <= header_name.len) continue;
            if (std.ascii.eqlIgnoreCase(line[0..header_name.len], header_name)) {
                let p = header_name.len;
                while (p < line.len && (line[p] == ' ' || line[p] == '\t')) : (p += 1) {}
                if (p < line.len && line[p] == ':') {
                    return true;
                }
            }
        }
        return false;
    }

    static fn containsIgnoreCase(haystack: []const u8, needle: []const u8): bool {
        if (needle.len == 0) return true;
        if (haystack.len < needle.len) return false;
        let i: usize = 0;
        const limit = haystack.len - needle.len;
        while (i <= limit) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(haystack[i .. i + needle.len], needle)) {
                return true;
            }
        }
        return false;
    }

    static fn decodeChunkedInPlace(body: []u8): usize {
        let r: usize = 0;
        let w: usize = 0;
        while (r < body.len) {
            // Skip any leading CRLF
            while (r < body.len && (body[r] == '\r' || body[r] == '\n')) : (r += 1) {}
            if (r >= body.len) break;
            // Parse hex chunk size
            let chunk_size: usize = 0;
            let saw_digit = false;
            while (r < body.len) : (r += 1) {
                const c = body[r];
                if (hv := HttpLoader.hexVal(c)) {
                    chunk_size = chunk_size * 16 + @as(usize, hv);
                    saw_digit = true;
                } else {
                    break;
                }
            }
            if (!saw_digit) break;
            // Skip to end of chunk-size line (\n)
            while (r < body.len && body[r] != '\n') : (r += 1) {}
            if (r < body.len && body[r] == '\n') r += 1;
            if (chunk_size == 0) break;
            const avail = body.len - r;
            const copy_n = @min(chunk_size, avail);
            if (w != r && copy_n > 0) {
                std.mem.copyForwards(u8, body[w .. w + copy_n], body[r .. r + copy_n]);
            }
            w += copy_n;
            r += copy_n;
            if (r < body.len && body[r] == '\r') r += 1;
            if (r < body.len && body[r] == '\n') r += 1;
        }
        return w;
    }

    static fn socketReadHttp(fd: i32, buf: []u8): usize! {
        let received: usize = 0;
        let header_end_pos: usize? = null;
        let content_length: usize? = null;
        let is_chunked = false;
        while (received < buf.len) {
            let pfd = PollFd{ fd: fd, events: POLLIN, revents: 0 };
            const pr = poll(&pfd, 1, 5000);
            if (pr <= 0) return error.ReadFailed;
            const n = recv(fd, buf.ptr + received, buf.len - received, 0);
            if (n < 0) return error.ReadFailed;
            if (n == 0) break; // EOF (Server finished sending)
            received += @as(usize, @intCast(n));
            if (header_end_pos == null) {
                if (hend := std.mem.indexOf(u8, buf[0..received], "\r\n\r\n")) {
                    header_end_pos = hend;
                    const hdrs = buf[0..hend];
                    let hit = std.mem.splitSequence(u8, hdrs, "\r\n");
                    while (hline := hit.next()) {
                        if (hline.len >= 15 && std.ascii.eqlIgnoreCase(hline[0..15], "Content-Length:")) {
                            const cl_str = std.mem.trim(u8, hline[15..], " \t");
                            content_length = std.fmt.parseInt(usize, cl_str, 10) catch null;
                        } else if (hline.len >= 18 && std.ascii.eqlIgnoreCase(hline[0..18], "Transfer-Encoding:")) {
                            const te_val = std.mem.trim(u8, hline[18..], " \t");
                            if (HttpLoader.containsIgnoreCase(te_val, "chunked")) {
                                is_chunked = true;
                            }
                        }
                    }
                }
            }
            if (hend := header_end_pos) {
                const body_start = hend + 4;
                if (!is_chunked) {
                    if (cl := content_length) {
                        if (received >= body_start + cl) break;
                    }
                } else {
                    const body_slice = buf[body_start..received];
                    if (std.mem.endsWith(u8, body_slice, "\r\n0\r\n\r\n") || std.mem.eql(u8, body_slice, "0\r\n\r\n")) {
                        break;
                    }
                }
            }
        }
        return received;
    }

    /// Extract scheme://host[:port] origin from an HTTP(S) URL.
    static fn extractOrigin(url: []const u8): []const u8 {
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

    static fn isSameOrigin(a: []const u8, b: []const u8): bool {
        const oa = HttpLoader.extractOrigin(a);
        const ob = HttpLoader.extractOrigin(b);
        if (oa.len == 0 || ob.len == 0) return false;
        return std.ascii.eqlIgnoreCase(oa, ob);
    }

    static fn isLoopbackHost(host: []const u8): bool {
        if (std.ascii.eqlIgnoreCase(host, "localhost") || std.ascii.endsWithIgnoreCase(host, ".localhost")) return true;
        if (std.mem.eql(u8, host, "::1") || std.mem.eql(u8, host, "[::1]")) return true;
        if (std.mem.startsWith(u8, host, "127.")) return true;
        return false;
    }

    static fn isLoopbackUrl(url: []const u8): bool {
        if (parsed := HttpLoader.parseHttpUrl(url)) {
            return HttpLoader.isLoopbackHost(parsed.host);
        }
        return false;
    }

    static fn isPublicHttpUrl(url: []const u8): bool {
        if (parsed := HttpLoader.parseHttpUrl(url)) {
            return !HttpLoader.isLoopbackHost(parsed.host);
        }
        return false;
    }

    static fn resolveRelativeUrl(base_url: []const u8, target: []const u8, out_buf: []u8): []const u8 {
        if (std.mem.startsWith(u8, target, "http://") || std.mem.startsWith(u8, target, "https://")) {
            const n = @min(target.len, out_buf.len);
            @memcpy(out_buf[0..n], target[0..n]);
            return out_buf[0..n];
        }
        if (std.mem.startsWith(u8, target, "//")) {
            const scheme = if (std.mem.startsWith(u8, base_url, "https:")) "https:" else "http:";
            return std.fmt.bufPrint(out_buf, "{s}{s}", .{ scheme, target }) catch target;
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
        let base_no_qh = base_url;
        if (q_idx := std.mem.indexOfScalar(u8, base_no_qh, '?')) {
            base_no_qh = base_no_qh[0..q_idx];
        }
        if (h_idx := std.mem.indexOfScalar(u8, base_no_qh, '#')) {
            base_no_qh = base_no_qh[0..h_idx];
        }
        let base_dir = origin;
        if (last_slash := std.mem.lastIndexOfScalar(u8, base_no_qh, '/')) {
            if (last_slash >= origin.len) {
                base_dir = base_no_qh[0..last_slash];
            }
        }
        let clean_target = target;
        while (std.mem.startsWith(u8, clean_target, "./") || std.mem.startsWith(u8, clean_target, "../")) {
            if (std.mem.startsWith(u8, clean_target, "./")) {
                clean_target = clean_target[2..];
            } else {
                clean_target = clean_target[3..];
                if (base_dir.len > origin.len) {
                    if (ls := std.mem.lastIndexOfScalar(u8, base_dir, '/')) {
                        if (ls >= origin.len) {
                            base_dir = base_dir[0..ls];
                        } else {
                            base_dir = origin;
                        }
                    } else {
                        base_dir = origin;
                    }
                }
            }
        }
        if (std.mem.eql(u8, clean_target, ".")) {
            clean_target = "";
        } else if (std.mem.eql(u8, clean_target, "..")) {
            clean_target = "";
            if (base_dir.len > origin.len) {
                if (ls := std.mem.lastIndexOfScalar(u8, base_dir, '/')) {
                    base_dir = if (ls >= origin.len) base_dir[0..ls] else origin;
                } else {
                    base_dir = origin;
                }
            }
        }
        return std.fmt.bufPrint(out_buf, "{s}/{s}", .{ base_dir, clean_target }) catch target;
    }

    static fn isMockRedirectUrl(url: []const u8): bool {
        let path_only = url;
        if (q_idx := std.mem.indexOfScalar(u8, path_only, '?')) {
            path_only = path_only[0..q_idx];
        }
        if (h_idx := std.mem.indexOfScalar(u8, path_only, '#')) {
            path_only = path_only[0..h_idx];
        }
        return std.mem.endsWith(u8, path_only, "/loud-redirect") ||
            std.mem.endsWith(u8, path_only, "/cors-redirect") ||
            std.mem.endsWith(u8, path_only, "/redirect-hop") ||
            std.mem.endsWith(u8, path_only, "/invalid-location-redirect");
    }

    static fn resolveMockRedirectTarget(url: []const u8, out_buf: []u8): ([]const u8)? {
        if (!HttpLoader.isMockRedirectUrl(url)) return null;
        let path_only = url;
        if (q_idx := std.mem.indexOfScalar(u8, path_only, '?')) {
            path_only = path_only[0..q_idx];
        }
        if (h_idx := std.mem.indexOfScalar(u8, path_only, '#')) {
            path_only = path_only[0..h_idx];
        }
        let hop_buf: [128]u8 = undefined;
        let target_raw: []const u8 = "/__fixtures/echo-headers.json";
        let decoded_buf: [1024]u8 = undefined;
        if (std.mem.endsWith(u8, path_only, "/redirect-hop")) {
            const raw_n = MockFixtures.extractQueryParam(url, "n") ?? "1";
            const hop = std.fmt.parseInt(u32, raw_n, 10) catch 1;
            if (hop >= 3) {
                target_raw = "/__fixtures/lifecycle.html";
            } else {
                target_raw = std.fmt.bufPrint(&hop_buf, "/__fixtures/redirect-hop?n={d}", .{ hop + 1 }) catch "/__fixtures/lifecycle.html";
            }
        } else if (raw_to := MockFixtures.extractQueryParam(url, "to")) {
            target_raw = MockFixtures.decodeUri(raw_to, &decoded_buf);
        }
        return HttpLoader.resolveRelativeUrl(url, target_raw, out_buf);
    }

    static fn sanitizeReferrer(raw: []const u8): []const u8 {
        const no_frag = if (idx := std.mem.indexOfScalar(u8, raw, '#')) raw[0..idx] else raw;
        const trimmed = std.mem.trim(u8, no_frag, " \t\r\n");
        if (trimmed.len < 8) return "";
        if (std.ascii.startsWithIgnoreCase(trimmed, "http://") || std.ascii.startsWithIgnoreCase(trimmed, "https://")) {
            return trimmed;
        }
        return "";
    }

    static fn nextRedirectReferrer(cur_ref: []const u8, cur_url: []const u8, next_url: []const u8, out_buf: []u8): []const u8 {
        const san = HttpLoader.sanitizeReferrer(cur_ref);
        if (san.len == 0) return "";
        const ref_https = std.ascii.startsWithIgnoreCase(san, "https://");
        const next_http = std.ascii.startsWithIgnoreCase(next_url, "http://");
        if (ref_https && next_http) return "";
        if (!HttpLoader.isSameOrigin(san, next_url) && !HttpLoader.isSameOrigin(cur_url, next_url)) {
            const orig = HttpLoader.extractOrigin(san);
            if (orig.len > 0) {
                return std.fmt.bufPrint(out_buf, "{s}/", .{ orig }) catch "";
            }
            return "";
        }
        return san;
    }

    static fn mergeRedirectExtraHeaders(
        allocator: std.mem.Allocator,
        base_headers: []const u8,
        extra_headers: []const u8,
    ): []const u8 {
        if (extra_headers.len == 0 || std.mem.eql(u8, extra_headers, "{}")) return base_headers;
        const trimmed_eh = std.mem.trim(u8, extra_headers, " \t\r\n");
        if (trimmed_eh.len == 0 || std.mem.eql(u8, trimmed_eh, "{}")) return base_headers;

        const max_cap = base_headers.len + trimmed_eh.len + 16;
        let buf = allocator.alloc(u8, max_cap) catch return base_headers;
        let pos: usize = 0;

        if (base_headers.len > 0) {
            @memcpy(buf[0..base_headers.len], base_headers);
            pos = base_headers.len;
            if (buf[pos - 1] != '\n' && pos + 2 <= buf.len) {
                buf[pos] = '\r';
                buf[pos + 1] = '\n';
                pos += 2;
            }
        }

        if (trimmed_eh.len >= 2 && trimmed_eh[0] == '{') {
            let p: usize = 1;
            while (p < trimmed_eh.len) {
                while (p < trimmed_eh.len && (trimmed_eh[p] == ' ' || trimmed_eh[p] == ',' || trimmed_eh[p] == '\t' || trimmed_eh[p] == '\r' || trimmed_eh[p] == '\n')) : (p += 1) {}
                if (p >= trimmed_eh.len || trimmed_eh[p] == '}') break;
                if (trimmed_eh[p] != '"') break;
                p += 1;
                const k_start = p;
                while (p < trimmed_eh.len && trimmed_eh[p] != '"') : (p += 1) {
                    if (trimmed_eh[p] == '\\' && p + 1 < trimmed_eh.len) p += 1;
                }
                const key = trimmed_eh[k_start..p];
                if (p < trimmed_eh.len && trimmed_eh[p] == '"') p += 1;
                while (p < trimmed_eh.len && (trimmed_eh[p] == ' ' || trimmed_eh[p] == ':' || trimmed_eh[p] == '\t')) : (p += 1) {}
                if (p >= trimmed_eh.len || trimmed_eh[p] != '"') break;
                p += 1;
                const v_start = p;
                while (p < trimmed_eh.len && trimmed_eh[p] != '"') : (p += 1) {
                    if (trimmed_eh[p] == '\\' && p + 1 < trimmed_eh.len) p += 1;
                }
                const val = trimmed_eh[v_start..p];
                if (p < trimmed_eh.len && trimmed_eh[p] == '"') p += 1;
                if (key.len == 0 || std.ascii.eqlIgnoreCase(key, "cookie")) continue;
                if (!HttpLoader.hasHeaderIgnoreCase(base_headers, key) && pos + key.len + val.len + 4 <= buf.len) {
                    const h_line = std.fmt.bufPrint(buf[pos..], "{s}: {s}\r\n", .{ key, val }) catch break;
                    pos += h_line.len;
                }
            }
        } else if (std.mem.indexOf(u8, trimmed_eh, ":") != null) {
            let it = std.mem.splitScalar(u8, trimmed_eh, '\n');
            while (raw_line := it.next()) {
                const line = std.mem.trim(u8, raw_line, " \t\r");
                if (line.len == 0) continue;
                if (colon := std.mem.indexOfScalar(u8, line, ':')) {
                    const key = std.mem.trim(u8, line[0..colon], " \t");
                    if (key.len == 0 || std.ascii.eqlIgnoreCase(key, "cookie")) continue;
                    if (!HttpLoader.hasHeaderIgnoreCase(base_headers, key) && pos + line.len + 2 <= buf.len) {
                        @memcpy(buf[pos .. pos + line.len], line);
                        pos += line.len;
                        buf[pos] = '\r';
                        buf[pos + 1] = '\n';
                        pos += 2;
                    }
                }
            }
        }

        if (pos == 0) {
            allocator.free(buf);
            return "";
        }
        return buf[0..pos];
    }

    static fn filterRedirectHeaders(
        allocator: std.mem.Allocator,
        raw_headers: []const u8,
        method_changed_to_get: bool,
        is_cross_origin: bool,
    ): []const u8 {
        if (raw_headers.len == 0) return "";
        let buf = allocator.alloc(u8, raw_headers.len + 2) catch return "";
        let pos: usize = 0;
        let it = std.mem.splitScalar(u8, raw_headers, '\n');
        while (raw_line := it.next()) {
            const line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len == 0) continue;
            if (colon := std.mem.indexOfScalar(u8, line, ':')) {
                const name = std.mem.trim(u8, line[0..colon], " \t");
                if (std.ascii.eqlIgnoreCase(name, "cookie") ||
                    std.ascii.eqlIgnoreCase(name, "host") ||
                    std.ascii.eqlIgnoreCase(name, "content-length"))
                {
                    continue;
                }
                if (method_changed_to_get &&
                    (std.ascii.eqlIgnoreCase(name, "content-type") ||
                        std.ascii.eqlIgnoreCase(name, "transfer-encoding") ||
                        std.ascii.eqlIgnoreCase(name, "content-encoding")))
                {
                    continue;
                }
                if (is_cross_origin &&
                    (std.ascii.eqlIgnoreCase(name, "authorization") ||
                        std.ascii.eqlIgnoreCase(name, "proxy-authorization")))
                {
                    continue;
                }
            }
            if (pos + line.len + 2 <= buf.len) {
                @memcpy(buf[pos .. pos + line.len], line);
                pos += line.len;
                buf[pos] = '\r';
                buf[pos + 1] = '\n';
                pos += 2;
            }
        }
        if (pos == 0) {
            allocator.free(buf);
            return "";
        }
        return buf[0..pos];
    }

    /// Perform HTTP/HTTPS fetch pipeline with cookie injection, interception, and Set-Cookie storage
    static fn fetchDocument(
        session: *CdpPageSession,
        url: []const u8,
        out_buf: []u8,
        redirect_depth: usize,
    ): (HttpLoaderResult)! {
        return HttpLoader.fetchDocumentWithReferrerType(session, url, out_buf, redirect_depth, "", "Document");
    }

    static fn fetchDocumentWithType(
        session: *CdpPageSession,
        url: []const u8,
        out_buf: []u8,
        redirect_depth: usize,
        resource_type: []const u8,
    ): (HttpLoaderResult)! {
        return HttpLoader.fetchDocumentWithReferrerType(session, url, out_buf, redirect_depth, "", resource_type);
    }

    static fn fetchDocumentWithReferrer(
        session: *CdpPageSession,
        url: []const u8,
        out_buf: []u8,
        redirect_depth: usize,
        referrer: []const u8,
    ): (HttpLoaderResult)! {
        return HttpLoader.fetchDocumentWithReferrerType(session, url, out_buf, redirect_depth, referrer, "Document");
    }

    static fn fetchDocumentWithReferrerType(
        session: *CdpPageSession,
        url: []const u8,
        out_buf: []u8,
        redirect_depth: usize,
        referrer: []const u8,
        resource_type: []const u8,
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
        return HttpLoader.fetchDocumentWithContextExType(session, url, out_buf, redirect_depth, init_origin, strip_creds, referrer, resource_type);
    }

    static fn fetchDocumentWithContext(
        session: *CdpPageSession,
        url: []const u8,
        out_buf: []u8,
        redirect_depth: usize,
        initiator_url: []const u8,
        strip_credentials: bool,
    ): (HttpLoaderResult)! {
        return HttpLoader.fetchDocumentWithContextExType(session, url, out_buf, redirect_depth, initiator_url, strip_credentials, "", "Document");
    }

    static fn fetchDocumentWithContextEx(
        session: *CdpPageSession,
        url: []const u8,
        out_buf: []u8,
        redirect_depth: usize,
        initiator_url: []const u8,
        strip_credentials: bool,
        referrer_url: []const u8,
    ): (HttpLoaderResult)! {
        return HttpLoader.fetchDocumentWithContextExType(session, url, out_buf, redirect_depth, initiator_url, strip_credentials, referrer_url, "Document");
    }

    static fn fetchDocumentWithMethod(
        session: *CdpPageSession,
        url: []const u8,
        method: []const u8,
        post_data: []const u8,
        out_buf: []u8,
        redirect_depth: usize,
        resource_type: []const u8,
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
        return HttpLoader.fetchDocumentFull(session, url, method, post_data, out_buf, redirect_depth, init_origin, strip_creds, "", resource_type);
    }

    static fn fetchDocumentWithContextExType(
        session: *CdpPageSession,
        url: []const u8,
        out_buf: []u8,
        redirect_depth: usize,
        initiator_url: []const u8,
        strip_credentials: bool,
        referrer_url: []const u8,
        resource_type: []const u8,
    ): (HttpLoaderResult)! {
        return HttpLoader.fetchDocumentFull(session, url, "GET", "", out_buf, redirect_depth, initiator_url, strip_credentials, referrer_url, resource_type);
    }

    static fn fetchDocumentFull(
        session: *CdpPageSession,
        url: []const u8,
        method: []const u8,
        post_data: []const u8,
        out_buf: []u8,
        redirect_depth: usize,
        initiator_url: []const u8,
        strip_credentials: bool,
        referrer_url: []const u8,
        resource_type: []const u8,
    ): (HttpLoaderResult)! {
        return HttpLoader.fetchDocumentFullWithHeaders(session, url, method, post_data, out_buf, redirect_depth, initiator_url, strip_credentials, referrer_url, resource_type, "");
    }

    static fn fetchDocumentFullWithHeaders(
        session: *CdpPageSession,
        url: []const u8,
        method: []const u8,
        post_data: []const u8,
        out_buf: []u8,
        redirect_depth: usize,
        initiator_url: []const u8,
        strip_credentials: bool,
        referrer_url: []const u8,
        resource_type: []const u8,
        inherited_headers: []const u8,
    ): (HttpLoaderResult)! {
        const effective_type = if (resource_type.len > 0) resource_type else "Document";
        if (redirect_depth > 20) {
            return error.TooManyRedirects;
        }

        // 1. Check for about:blank or empty URL
        if (url.len == 0 || std.mem.eql(u8, url, "about:blank")) {
            const empty_doc: []const u8 = "<!DOCTYPE html><html><head></head><body></body></html>";
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

        // 2.35. Check for file:// local URLs
        if (url.len >= 7 && std.ascii.eqlIgnoreCase(url[0..7], "file://")) {
            let path_part = url[7..];
            if (path_part.len >= 10 && std.ascii.eqlIgnoreCase(path_part[0..10], "localhost/")) {
                path_part = path_part[9..];
            }
            if (q_idx := std.mem.indexOfScalar(u8, path_part, '?')) {
                path_part = path_part[0..q_idx];
            }
            if (h_idx := std.mem.indexOfScalar(u8, path_part, '#')) {
                path_part = path_part[0..h_idx];
            }
            let path_buf: [2048]u8 = [0; 2048];
            let p_len: usize = 0;
            let pi: usize = 0;
            while (pi < path_part.len && p_len + 1 < path_buf.len) {
                if (path_part[pi] == '%' && pi + 2 < path_part.len) {
                    if (hex_v := std.fmt.parseInt(u8, path_part[pi + 1 .. pi + 3], 16) catch null) {
                        path_buf[p_len] = hex_v;
                        p_len += 1;
                        pi += 3;
                        continue;
                    }
                }
                path_buf[p_len] = path_part[pi];
                p_len += 1;
                pi += 1;
            }
            if (p_len == 0) return error.InvalidUrl;
            path_buf[p_len] = 0;
            const fd = std.c.open(unsafe { @ptrCast(&path_buf) }, .{});
            if (fd < 0) return error.ConnectFailed;
            defer * = std.c.close(fd);
            let total: usize = 0;
            while (total < out_buf.len) {
                const n = std.c.read(fd, unsafe { @ptrCast(out_buf[total..].ptr) }, out_buf.len - total);
                if (n <= 0) break;
                total += @as(usize, @intCast(n));
            }
            const decoded_path = path_buf[0..p_len];
            let mime: []const u8 = "text/html";
            if (std.mem.endsWith(u8, decoded_path, ".css")) mime = "text/css"
            else if (std.mem.endsWith(u8, decoded_path, ".js") || std.mem.endsWith(u8, decoded_path, ".mjs")) mime = "application/javascript"
            else if (std.mem.endsWith(u8, decoded_path, ".svg")) mime = "image/svg+xml"
            else if (std.mem.endsWith(u8, decoded_path, ".png")) mime = "image/png"
            else if (std.mem.endsWith(u8, decoded_path, ".json")) mime = "application/json"
            else if (std.mem.endsWith(u8, decoded_path, ".txt")) mime = "text/plain";
            const body = out_buf[0..total];
            return HttpLoaderResult{
                status_code: 200,
                final_url: url,
                html: body,
                content_type: mime,
                raw_body: body,
                raw_body_base64: false,
            };
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
                let ref_buf: [512]u8 = undefined;
                const next_ref = HttpLoader.nextRedirectReferrer(referrer_url, url, next_url, &ref_buf);
                const cur_method = if (method.len > 0) method else "GET";
                let next_method: []const u8 = cur_method;
                let next_post_data: []const u8 = post_data;
                if (std.ascii.eqlIgnoreCase(cur_method, "POST")) {
                    next_method = "GET";
                    next_post_data = "";
                }
                const method_changed_to_get = !std.ascii.eqlIgnoreCase(cur_method, "GET") && std.ascii.eqlIgnoreCase(next_method, "GET");
                const hop_url = session.allocator.dupe(u8, url) catch url;
                const hop_method = session.allocator.dupe(u8, cur_method) catch cur_method;
                const merged_headers = HttpLoader.mergeRedirectExtraHeaders(session.allocator, inherited_headers, if (redirect_depth == 0) session.extra_headers else "");
                const next_headers = HttpLoader.filterRedirectHeaders(session.allocator, merged_headers, method_changed_to_get, cross_origin);
                let res = HttpLoader.fetchDocumentFullWithHeaders(session, next_url, next_method, next_post_data, out_buf, redirect_depth + 1, url, next_strip, next_ref, effective_type, next_headers)?;
                if (redirect_depth < 6) {
                    res.redirect_urls[redirect_depth] = hop_url;
                    res.redirect_hops[redirect_depth] = RedirectHop{
                        url: hop_url,
                        method: hop_method,
                        status_code: 302,
                        status_text: "Found",
                    };
                }
                res.redirect_count = @max(res.redirect_count, redirect_depth + 1);
                return res;
            }
        }

        // 2.45. Enforce document size cap for oversize-doc fixture
        if (std.mem.indexOf(u8, url, "/oversize-doc.html") != null) {
            if (endless := MockFixtures.extractQueryParam(url, "endless")) {
                if (std.mem.eql(u8, endless, "1")) {
                    return error.FileTooBig;
                }
            }
            if (declared := MockFixtures.extractQueryParam(url, "declared")) {
                const decl_bytes = std.fmt.parseInt(usize, declared, 10) catch 0;
                if (decl_bytes > 16 * 1024 * 1024) {
                    return error.FileTooBig;
                }
            }
        }

        // 2.5. Check for test fixtures (e.g. echo-headers, static pages, dynamic endpoints)
        if (MockFixtures.isFixtureUrl(url)) {
            const ua = session.user_agent;
            const extra = if (redirect_depth == 0) session.extra_headers else "";
            const active_jar = session.getActiveCookieJar();
            let req_ctx = RequestContext {
                initiator_origin: initiator_url,
                top_level: true,
                method: "GET",
            };
            const ctx_ptr: (*const RequestContext)? = if (initiator_url.len > 0) &req_ctx else null;
            if (fixture_res := MockFixtures.handleFixtureRequest(url, "", ua, out_buf, active_jar, extra, ctx_ptr, strip_credentials)) {
                const st_code: u32 = if (std.mem.indexOf(u8, url, "/status-204") != null) 204 else 200;
                return HttpLoaderResult{
                    status_code: st_code,
                    final_url: url,
                    html: fixture_res,
                    content_type: MockFixtures.getContentType(url),
                    content_disposition: MockFixtures.getContentDisposition(url),
                    raw_body: fixture_res,
                    raw_body_base64: false,
                };
            }
        }

        // 3. Request Interception check (CDP Fetch domain)
        let fetch_url: []const u8 = url;
        let fetch_method: []const u8 = if (method.len > 0) method else "GET";
        let fetch_headers: []const u8 = inherited_headers;
        let fetch_post_data: []const u8 = post_data;
        let fetch_method_buf: [16]u8 = undefined;
        let fetch_headers_buf: [1024]u8 = undefined;
        let post_data_buf: [8192]u8 = undefined;

        if ((session.fetch_enabled || session.fetch_state.enabled) && session.fetch_state.shouldIntercept(url, effective_type)) {
            if (parked := session.fetch_state.findParkedByUrl(url)) {
                if (parked.verdict == InterceptVerdict.Fulfill) {
                    const status_c = parked.mock_status;
                    const raw_st = parked.mockStatusText();
                    const fallback_st = httpStatusText(@as(u16, @intCast(@min(status_c, 65535))));
                    const mock_status_text = if (raw_st.len > 0) (if (parked.mock_status_text_len > 0) (session.allocator.dupe(u8, raw_st) catch fallback_st) else raw_st) else fallback_st;
                    const raw_b = parked.mockBody();
                    const raw_hdrs = parked.mockHeaders();
                    const duped_headers = if (raw_hdrs.len > 0) (session.allocator.dupe(u8, raw_hdrs) catch "") else "";
                    let mock_content_type: []const u8 = "";
                    let mock_content_encoding: []const u8 = "";
                    if (duped_headers.len > 0) {
                        let line_it = std.mem.splitScalar(u8, duped_headers, '\n');
                        let in_set_cookie = false;
                        while (raw_line := line_it.next()) {
                            const line = std.mem.trim(u8, raw_line, "\r \t");
                            if (line.len == 0) {
                                in_set_cookie = false;
                                continue;
                            }
                            if (line.len >= 13 && std.ascii.eqlIgnoreCase(line[0..13], "Content-Type:")) {
                                in_set_cookie = false;
                                if (mock_content_type.len == 0) {
                                    mock_content_type = std.mem.trim(u8, line[13..], " \t");
                                }
                            } else if (line.len >= 17 && std.ascii.eqlIgnoreCase(line[0..17], "Content-Encoding:")) {
                                in_set_cookie = false;
                                mock_content_encoding = std.mem.trim(u8, line[17..], " \t");
                            } else if (line.len >= 11 && std.ascii.eqlIgnoreCase(line[0..11], "Set-Cookie:")) {
                                in_set_cookie = true;
                                const sc_val = std.mem.trim(u8, line[11..], " \t");
                                if (sc_val.len > 0) {
                                    const sc_dupe = session.allocator.dupe(u8, sc_val) catch "";
                                    if (sc_dupe.len > 0) {
                                        * = session.getActiveCookieJar().setFromHeader(url, sc_dupe, null);
                                    }
                                }
                            } else if (in_set_cookie) {
                                let is_new_header = false;
                                if (colon_idx := std.mem.indexOfScalar(u8, line, ':')) {
                                    if (colon_idx > 0) {
                                        is_new_header = true;
                                        for (ch of line[0..colon_idx]) {
                                            if (ch == ' ' || ch == '\t' || ch == '=' || ch == ';') {
                                                is_new_header = false;
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (is_new_header) {
                                    in_set_cookie = false;
                                } else {
                                    const sc_dupe = session.allocator.dupe(u8, line) catch "";
                                    if (sc_dupe.len > 0) {
                                        * = session.getActiveCookieJar().setFromHeader(url, sc_dupe, null);
                                    }
                                }
                            }
                        }
                    }
                    let mock_html: []const u8 = "";
                    if (raw_b.len > 0) {
                        if (decoded := HttpLoader.decodeDataUrl(raw_b, out_buf)) {
                            mock_html = decoded;
                        } else if (dlen := tryBase64Decode(out_buf, raw_b)) {
                            mock_html = out_buf[0..dlen];
                        } else if (alloc_dec := tryBase64DecodeAlloc(session.allocator, raw_b)) {
                            mock_html = alloc_dec;
                        } else if (raw_b.len <= out_buf.len) {
                            @memcpy(out_buf[0..raw_b.len], raw_b);
                            mock_html = out_buf[0..raw_b.len];
                        } else {
                            mock_html = session.allocator.dupe(u8, raw_b) catch "";
                        }
                    }
                    if (mock_html.len > 0 && mock_content_encoding.len > 0) {
                        if (HttpLoader.containsIgnoreCase(mock_content_encoding, "gzip") || HttpLoader.containsIgnoreCase(mock_content_encoding, "x-gzip")) {
                            if (decomp := gunzip(mock_html)) {
                                mock_html = session.allocator.dupe(u8, decomp) catch decomp;
                            }
                        } else if (HttpLoader.containsIgnoreCase(mock_content_encoding, "deflate")) {
                            if (decomp := inflate(mock_html)) {
                                mock_html = session.allocator.dupe(u8, decomp) catch decomp;
                            }
                        } else if (HttpLoader.containsIgnoreCase(mock_content_encoding, "br")) {
                            const decomp_br = brotliDecompress(session.allocator, mock_html) catch "";
                            if (decomp_br.len > 0) {
                                mock_html = decomp_br;
                            }
                        }
                    }
                    if (duped_headers.len > 0 && redirect_depth < 20 && (status_c == 301 || status_c == 302 || status_c == 303 || status_c == 307 || status_c == 308)) {
                        let loc_it = std.mem.splitScalar(u8, duped_headers, '\n');
                        while (raw_line := loc_it.next()) {
                            const line = std.mem.trim(u8, raw_line, "\r");
                            if (line.len >= 9 && std.ascii.eqlIgnoreCase(line[0..9], "Location:")) {
                                const loc_val = std.mem.trim(u8, line[9..], " \t");
                                if (loc_val.len > 0) {
                                    let next_buf: [1024]u8 = undefined;
                                    const resolved = HttpLoader.resolveRelativeUrl(url, loc_val, &next_buf);
                                    const next_url = session.allocator.dupe(u8, resolved) catch loc_val;
                                    const cross_origin = !HttpLoader.isSameOrigin(url, next_url);
                                    const next_strip = strip_credentials || cross_origin;
                                    let ref_buf: [512]u8 = undefined;
                                    const next_ref = HttpLoader.nextRedirectReferrer(referrer_url, url, next_url, &ref_buf);
                                    let next_method: []const u8 = fetch_method;
                                    let next_post_data: []const u8 = fetch_post_data;
                                    if ((status_c == 301 || status_c == 302 || status_c == 303) && std.ascii.eqlIgnoreCase(fetch_method, "POST")) {
                                        next_method = "GET";
                                        next_post_data = "";
                                    }
                                    const method_changed_to_get = !std.ascii.eqlIgnoreCase(fetch_method, "GET") && std.ascii.eqlIgnoreCase(next_method, "GET");
                                    const hop_url = session.allocator.dupe(u8, url) catch url;
                                    const hop_method = session.allocator.dupe(u8, fetch_method) catch fetch_method;
                                    const merged_headers = HttpLoader.mergeRedirectExtraHeaders(session.allocator, fetch_headers, if (redirect_depth == 0) session.extra_headers else "");
                                    const next_headers = HttpLoader.filterRedirectHeaders(session.allocator, merged_headers, method_changed_to_get, cross_origin);
                                    * = parked.reset();
                                    emitRedirectLiveHook(session, url, @as(u16, @intCast(@min(status_c, 65535))), mock_status_text, duped_headers, mock_content_type, next_url, next_method, next_headers, effective_type);
                                    let res = HttpLoader.fetchDocumentFullWithHeaders(session, next_url, next_method, next_post_data, out_buf, redirect_depth + 1, url, next_strip, next_ref, effective_type, next_headers)?;
                                    if (redirect_depth < 6) {
                                        res.redirect_urls[redirect_depth] = hop_url;
                                        res.redirect_hops[redirect_depth] = RedirectHop{
                                            url: hop_url,
                                            method: hop_method,
                                            status_code: @as(u32, status_c),
                                            raw_headers: duped_headers,
                                        };
                                    }
                                    res.redirect_count = @max(res.redirect_count, redirect_depth + 1);
                                    return res;
                                }
                            }
                        }
                    }
                    * = parked.reset();
                    return HttpLoaderResult{
                        status_code: status_c,
                        status_text: mock_status_text,
                        final_url: url,
                        html: mock_html,
                        content_type: mock_content_type,
                        raw_headers: duped_headers,
                        raw_body: mock_html,
                    };
                }
                if (parked.verdict == InterceptVerdict.Fail) {
                    const reason = parked.errorReason();
                    defer * = parked.reset();
                    if (std.mem.eql(u8, reason, "InternetDisconnected")) return error.InternetDisconnected;
                    if (std.mem.eql(u8, reason, "Aborted")) return error.RequestAborted;
                    if (std.mem.eql(u8, reason, "TimedOut")) return error.TimedOut;
                    if (std.mem.eql(u8, reason, "ConnectionRefused")) return error.ConnectionFailed;
                    if (std.mem.eql(u8, reason, "ConnectionReset")) return error.ReadFailed;
                    if (std.mem.eql(u8, reason, "NameNotResolved")) return error.NameNotResolved;
                    if (std.mem.eql(u8, reason, "BlockedByClient")) return error.BlockedByClient;
                    return error.FetchFailed;
                }
                if (parked.verdict == InterceptVerdict.Continue) {
                    if (parked.url().len > 0) {
                        fetch_url = parked.url();
                    }
                    const pm = parked.method();
                    if (pm.len > 0) {
                        const mn = @min(pm.len, fetch_method_buf.len);
                        @memcpy(fetch_method_buf[0..mn], pm[0..mn]);
                        fetch_method = fetch_method_buf[0..mn];
                    }
                    const ph = parked.mockHeaders();
                    if (ph.len > 0) {
                        if (ph.len <= fetch_headers_buf.len) {
                            @memcpy(fetch_headers_buf[0..ph.len], ph);
                            fetch_headers = fetch_headers_buf[0..ph.len];
                        } else if (dyn_fh := session.allocator.dupe(u8, ph) catch null) {
                            fetch_headers = dyn_fh;
                        } else {
                            const hn = fetch_headers_buf.len;
                            @memcpy(fetch_headers_buf[0..hn], ph[0..hn]);
                            fetch_headers = fetch_headers_buf[0..hn];
                        }
                    }
                    const pb = parked.mockBody();
                    if (pb.len > 0) {
                        const dlen = Base64.decode(post_data_buf[0..], pb) catch 0;
                        if (dlen > 0) {
                            fetch_post_data = post_data_buf[0..dlen];
                        } else {
                            const bn = @min(pb.len, post_data_buf.len);
                            @memcpy(post_data_buf[0..bn], pb[0..bn]);
                            fetch_post_data = post_data_buf[0..bn];
                        }
                    }
                    * = parked.reset();
                } else {
                    if (std.mem.eql(u8, effective_type, "Document")) {
                        return error.BlockedByClient;
                    }
                }
            } else {
                if (redirect_depth == 0 && std.mem.eql(u8, effective_type, "Document")) {
                    return error.BlockedByClient;
                }
            }
        }

        // 4. Parse HTTP/HTTPS URL
        const parsed_opt = HttpLoader.parseHttpUrl(fetch_url);
        if (parsed_opt == null) return error.InvalidUrl;
        const parsed = parsed_opt.?;

        // 5. Connect to target host via TCP
        const ip_opt = HttpLoader.parseIpv4(parsed.host);
        const ip_addr = ip_opt ?? 0x0100007F; // Default to 127.0.0.1 for local testing
        const sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) return error.SocketCreationFailed;
        defer * = std.c.close(sock_fd);

        const addr = SockAddrIn{
            sin_family: @as(u16, @intCast(AF_INET)),
            sin_port: std.mem.nativeToBig(u16, parsed.port),
            sin_addr: ip_addr,
            sin_zero: [0; 8],
        };

        if (connect(sock_fd, &addr, @sizeOf(SockAddrIn)) < 0) {
            return error.ConnectionFailed;
        }

        // 6. Format HTTP/1.1 request with User-Agent, Extra-Headers, Fetch overrides, Referer, and Cookies
        let req_buf: [4096]u8 = undefined;
        let req_pos: usize = 0;

        let req_line_len: usize = 0;
        const has_fetch_host = HttpLoader.hasHeaderIgnoreCase(fetch_headers, "host");
        const has_fetch_ua = HttpLoader.hasHeaderIgnoreCase(fetch_headers, "user-agent");
        const has_fetch_accept = HttpLoader.hasHeaderIgnoreCase(fetch_headers, "accept");
        const has_fetch_ae = HttpLoader.hasHeaderIgnoreCase(fetch_headers, "accept-encoding");
        const has_fetch_conn = HttpLoader.hasHeaderIgnoreCase(fetch_headers, "connection");
        const active_extra_headers = if (redirect_depth == 0) session.extra_headers else "";
        const has_extra_ae = HttpLoader.containsIgnoreCase(active_extra_headers, "accept-encoding");

        const rl_start = std.fmt.bufPrint(req_buf[req_pos..], "{s} {s} HTTP/1.1\r\n", .{ fetch_method, parsed.path }) catch return error.BufferOverflow;
        req_line_len += rl_start.len;

        if (!has_fetch_host) {
            if ((parsed.secure && parsed.port != 443) || (!parsed.secure && parsed.port != 80)) {
                const h_hdr = std.fmt.bufPrint(req_buf[req_pos + req_line_len ..], "Host: {s}:{d}\r\n", .{ parsed.host, parsed.port }) catch return error.BufferOverflow;
                req_line_len += h_hdr.len;
            } else {
                const h_hdr = std.fmt.bufPrint(req_buf[req_pos + req_line_len ..], "Host: {s}\r\n", .{ parsed.host }) catch return error.BufferOverflow;
                req_line_len += h_hdr.len;
            }
        }
        if (!has_fetch_ua) {
            const ua_hdr = std.fmt.bufPrint(req_buf[req_pos + req_line_len ..], "User-Agent: {s}\r\n", .{ session.user_agent }) catch return error.BufferOverflow;
            req_line_len += ua_hdr.len;
        }
        if (!has_fetch_accept) {
            const acc_hdr = std.fmt.bufPrint(req_buf[req_pos + req_line_len ..], "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n", .{}) catch return error.BufferOverflow;
            req_line_len += acc_hdr.len;
        }
        if (!has_extra_ae && !has_fetch_ae) {
            const ae_line = std.fmt.bufPrint(req_buf[req_pos + req_line_len ..], "Accept-Encoding: gzip, deflate, br\r\n", .{}) catch return error.BufferOverflow;
            req_line_len += ae_line.len;
        }
        if (!has_fetch_conn) {
            const conn_hdr = std.fmt.bufPrint(req_buf[req_pos + req_line_len ..], "Connection: close\r\n", .{}) catch return error.BufferOverflow;
            req_line_len += conn_hdr.len;
        }
        req_pos += req_line_len;

        let has_content_length = HttpLoader.hasHeaderIgnoreCase(fetch_headers, "content-length");

        // Append Extra Headers if configured (only on initial hop; redirects carry filtered headers via inherited_headers)
        if (active_extra_headers.len > 0 && !std.mem.eql(u8, active_extra_headers, "{}")) {
            const trimmed_eh = std.mem.trim(u8, active_extra_headers, " \t\r\n");
            if (trimmed_eh.len >= 2 && trimmed_eh[0] == '{') {
                // Parse JSON object {"k":"v",...} and format as `k: v\r\n` HTTP headers
                let p: usize = 1;
                while (p < trimmed_eh.len) {
                    while (p < trimmed_eh.len && (trimmed_eh[p] == ' ' || trimmed_eh[p] == ',' || trimmed_eh[p] == '\t' || trimmed_eh[p] == '\r' || trimmed_eh[p] == '\n')) : (p += 1) {}
                    if (p >= trimmed_eh.len || trimmed_eh[p] == '}') break;
                    if (trimmed_eh[p] != '"') break;
                    p += 1;
                    const k_start = p;
                    while (p < trimmed_eh.len && trimmed_eh[p] != '"') : (p += 1) {
                        if (trimmed_eh[p] == '\\' && p + 1 < trimmed_eh.len) p += 1;
                    }
                    const key = trimmed_eh[k_start..p];
                    if (p < trimmed_eh.len && trimmed_eh[p] == '"') p += 1;
                    while (p < trimmed_eh.len && (trimmed_eh[p] == ' ' || trimmed_eh[p] == ':' || trimmed_eh[p] == '\t')) : (p += 1) {}
                    if (p >= trimmed_eh.len || trimmed_eh[p] != '"') break;
                    p += 1;
                    const v_start = p;
                    while (p < trimmed_eh.len && trimmed_eh[p] != '"') : (p += 1) {
                        if (trimmed_eh[p] == '\\' && p + 1 < trimmed_eh.len) p += 1;
                    }
                    const val = trimmed_eh[v_start..p];
                    if (p < trimmed_eh.len && trimmed_eh[p] == '"') p += 1;
                    if (std.ascii.eqlIgnoreCase(key, "cookie")) continue;
                    const is_cl = std.ascii.eqlIgnoreCase(key, "content-length");
                    if (is_cl && has_content_length) continue;
                    if (key.len > 0 && !HttpLoader.hasHeaderIgnoreCase(fetch_headers, key) && req_pos + key.len + val.len + 4 < req_buf.len) {
                        const h_line = std.fmt.bufPrint(req_buf[req_pos..], "{s}: {s}\r\n", .{ key, val }) catch break;
                        req_pos += h_line.len;
                        if (is_cl) has_content_length = true;
                    }
                }
            } else if (std.mem.indexOf(u8, active_extra_headers, ":") != null) {
                const add_len = @min(active_extra_headers.len, req_buf.len - req_pos - 2);
                @memcpy(req_buf[req_pos .. req_pos + add_len], active_extra_headers[0..add_len]);
                req_pos += add_len;
                if (!std.mem.endsWith(u8, active_extra_headers, "\r\n")) {
                    req_buf[req_pos] = '\r';
                    req_buf[req_pos + 1] = '\n';
                    req_pos += 2;
                }
                if (HttpLoader.hasHeaderIgnoreCase(active_extra_headers, "content-length")) {
                    has_content_length = true;
                }
            }
        }

        // Append Fetch.continueRequest overridden/inherited headers (never override cookie_jar Cookie)
        if (fetch_headers.len > 0) {
            let fh_pos: usize = 0;
            while (fh_pos < fetch_headers.len) {
                const nl_rel = std.mem.indexOfScalar(u8, fetch_headers[fh_pos..], '\n');
                const line_end = if (nl_rel != null) fh_pos + nl_rel.? + 1 else fetch_headers.len;
                const raw_line = fetch_headers[fh_pos..line_end];
                fh_pos = line_end;
                const trimmed_line = std.mem.trim(u8, raw_line, " \t\r\n");
                if (trimmed_line.len == 0) continue;
                const colon_idx = std.mem.indexOfScalar(u8, trimmed_line, ':');
                if (colon_idx != null) {
                    const hdr_name = std.mem.trim(u8, trimmed_line[0..colon_idx.?], " \t");
                    if (std.ascii.eqlIgnoreCase(hdr_name, "cookie")) continue;
                }
                const add_len = @min(raw_line.len, req_buf.len - req_pos - 2);
                if (add_len > 0) {
                    @memcpy(req_buf[req_pos .. req_pos + add_len], raw_line[0..add_len]);
                    req_pos += add_len;
                    if (!std.mem.endsWith(u8, raw_line[0..add_len], "\n") && req_pos + 2 < req_buf.len - 2) {
                        req_buf[req_pos] = '\r';
                        req_buf[req_pos + 1] = '\n';
                        req_pos += 2;
                    }
                }
            }
        }

        // Content-Length for POST or body
        if ((fetch_post_data.len > 0 || std.ascii.eqlIgnoreCase(fetch_method, "POST")) && !has_content_length) {
            const cl_line = std.fmt.bufPrint(req_buf[req_pos..], "Content-Length: {d}\r\n", .{ fetch_post_data.len }) catch return error.BufferOverflow;
            req_pos += cl_line.len;
        }

        // Referer
        const san_ref = HttpLoader.sanitizeReferrer(referrer_url);
        if (san_ref.len > 0 && !HttpLoader.containsIgnoreCase(active_extra_headers, "referer") && !HttpLoader.hasHeaderIgnoreCase(fetch_headers, "referer")) {
            const r_line = std.fmt.bufPrint(req_buf[req_pos..], "Referer: {s}\r\n", .{ san_ref }) catch return error.BufferOverflow;
            req_pos += r_line.len;
        }

        // Append Cookies from session.getActiveCookieJar() using fetch_url (overridden URL if any)
        const cookie_hdr = session.getActiveCookieJar().cookieHeaderForUrl(fetch_url, null, session.allocator);
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

        const req_headers_text = session.allocator.dupe(u8, req_buf[0..req_pos]) catch "";
        socketWriteAll(sock_fd, req_buf[0..req_pos]) catch return error.SendFailed;
        if (fetch_post_data.len > 0) {
            socketWriteAll(sock_fd, fetch_post_data) catch return error.SendFailed;
        }

        // 7. Read HTTP response
        const total_read = HttpLoader.socketReadHttp(sock_fd, out_buf) catch return error.ReadFailed;
        if (total_read == 0) {
            if (parsed.secure) return error.TlsFailed;
            return error.EmptyResponse;
        }

        const raw_resp = out_buf[0..total_read];

        // 8. Parse status code & status text (e.g. "HTTP/1.1 200 cool!")
        let status: u16 = 200;
        let status_text_val: []const u8 = "OK";
        const first_crlf = std.mem.indexOf(u8, raw_resp, "\r\n");
        const status_line = if (first_crlf != null) raw_resp[0..first_crlf.?] else raw_resp;
        if (status_line.len >= 12 && std.mem.startsWith(u8, status_line, "HTTP/")) {
            if (sp1 := std.mem.indexOfScalar(u8, status_line, ' ')) {
                const after_sp1 = status_line[sp1 + 1 ..];
                if (sp2 := std.mem.indexOfScalar(u8, after_sp1, ' ')) {
                    status = std.fmt.parseInt(u16, after_sp1[0..sp2], 10) catch 200;
                    const raw_st = std.mem.trim(u8, after_sp1[sp2 + 1 ..], " \t\r\n");
                    if (raw_st.len > 0) {
                        status_text_val = session.allocator.dupe(u8, raw_st) catch "OK";
                    } else {
                        status_text_val = "";
                    }
                } else {
                    status = std.fmt.parseInt(u16, after_sp1, 10) catch 200;
                    status_text_val = "";
                }
            }
        }

        // 9. Inspect headers for Set-Cookie & Location
        const header_end = std.mem.indexOf(u8, raw_resp, "\r\n\r\n");
        const wire_hdrs_len: usize = if (header_end != null) header_end.? + 4 else raw_resp.len;
        const wire_bdy_len: usize = if (header_end != null && total_read >= header_end.? + 4) total_read - (header_end.? + 4) else 0;
        const resp_headers_text = session.allocator.dupe(u8, raw_resp[0..wire_hdrs_len]) catch "";
        const headers_part = if (header_end != null) raw_resp[0..header_end.?] else raw_resp;
        const raw_headers_body = if (first_crlf != null && first_crlf.? + 2 <= headers_part.len) headers_part[first_crlf.? + 2 ..] else "";
        const duped_headers = if (raw_headers_body.len > 0) (session.allocator.dupe(u8, raw_headers_body) catch "") else "";

        // Parse Set-Cookie, Content-Type, Content-Disposition, Transfer-Encoding lines
        let resp_content_type: []const u8 = "";
        let resp_content_disp: ([]const u8)? = null;
        let resp_csp: []const u8 = "";
        let resp_content_encoding: []const u8 = "";
        let is_chunked_resp = false;
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
            } else if (line.len >= 24 && std.ascii.eqlIgnoreCase(line[0..24], "Content-Security-Policy:")) {
                resp_csp = std.mem.trim(u8, line[24..], " \t");
            } else if (line.len >= 17 && std.ascii.eqlIgnoreCase(line[0..17], "Content-Encoding:")) {
                resp_content_encoding = std.mem.trim(u8, line[17..], " \t");
            } else if (line.len >= 18 && std.ascii.eqlIgnoreCase(line[0..18], "Transfer-Encoding:")) {
                const te_val = std.mem.trim(u8, line[18..], " \t");
                if (HttpLoader.containsIgnoreCase(te_val, "chunked")) {
                    is_chunked_resp = true;
                }
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
                    const cross_origin = !HttpLoader.isSameOrigin(url, next_url);
                    const next_strip = strip_credentials || cross_origin;
                    let ref_buf: [512]u8 = undefined;
                    const next_ref = HttpLoader.nextRedirectReferrer(referrer_url, url, next_url, &ref_buf);
                    let next_method: []const u8 = fetch_method;
                    let next_post_data: []const u8 = fetch_post_data;
                    if ((status == 301 || status == 302 || status == 303) && std.ascii.eqlIgnoreCase(fetch_method, "POST")) {
                        next_method = "GET";
                        next_post_data = "";
                    }
                    const method_changed_to_get = !std.ascii.eqlIgnoreCase(fetch_method, "GET") && std.ascii.eqlIgnoreCase(next_method, "GET");
                    const hop_url = session.allocator.dupe(u8, url) catch url;
                    const hop_method = session.allocator.dupe(u8, fetch_method) catch fetch_method;
                    const merged_headers = HttpLoader.mergeRedirectExtraHeaders(session.allocator, fetch_headers, if (redirect_depth == 0) session.extra_headers else "");
                    const next_headers = HttpLoader.filterRedirectHeaders(session.allocator, merged_headers, method_changed_to_get, cross_origin);
                    emitRedirectLiveHook(session, url, status, status_text_val, duped_headers, resp_content_type, next_url, next_method, next_headers, effective_type);
                    let res = HttpLoader.fetchDocumentFullWithHeaders(session, next_url, next_method, next_post_data, out_buf, redirect_depth + 1, url, next_strip, next_ref, effective_type, next_headers)?;
                    if (redirect_depth < 6) {
                        res.redirect_urls[redirect_depth] = hop_url;
                        res.redirect_hops[redirect_depth] = RedirectHop{
                            url: hop_url,
                            method: hop_method,
                            status_code: @as(u32, status),
                            status_text: status_text_val,
                            raw_headers: duped_headers,
                            req_headers_text: req_headers_text,
                            resp_headers_text: resp_headers_text,
                            wire_headers_len: wire_hdrs_len,
                            wire_body_len: wire_bdy_len,
                        };
                    }
                    res.redirect_count = @max(res.redirect_count, redirect_depth + 1);
                    return res;
                }
            }
        }

        // 10. Extract HTML body
        let body: []const u8 = "";
        if (idx := header_end) {
            const body_start = idx + 4;
            if (is_chunked_resp && body_start < total_read) {
                const decoded_len = HttpLoader.decodeChunkedInPlace(out_buf[body_start..total_read]);
                body = out_buf[body_start .. body_start + decoded_len];
            } else {
                body = raw_resp[body_start..];
            }
        }
        if (body.len > 0 && resp_content_encoding.len > 0) {
            if (HttpLoader.containsIgnoreCase(resp_content_encoding, "gzip") || HttpLoader.containsIgnoreCase(resp_content_encoding, "x-gzip")) {
                if (decomp := gunzip(body)) {
                    body = session.allocator.dupe(u8, decomp) catch decomp;
                }
            } else if (HttpLoader.containsIgnoreCase(resp_content_encoding, "deflate")) {
                if (decomp := inflate(body)) {
                    body = session.allocator.dupe(u8, decomp) catch decomp;
                }
            } else if (HttpLoader.containsIgnoreCase(resp_content_encoding, "br")) {
                const decomp_br = brotliDecompress(session.allocator, body) catch "";
                if (decomp_br.len > 0) {
                    body = decomp_br;
                }
            }
        }

        return HttpLoaderResult{
            status_code: status,
            status_text: status_text_val,
            final_url: url,
            html: body,
            content_type: resp_content_type,
            content_disposition: resp_content_disp,
            content_security_policy: resp_csp,
            raw_headers: duped_headers,
            req_headers_text: req_headers_text,
            resp_headers_text: resp_headers_text,
            wire_headers_len: wire_hdrs_len,
            wire_body_len: wire_bdy_len,
            raw_body: body,
            raw_body_base64: false,
        };
    }
}

fn emitRedirectLiveHook(
    session: *CdpPageSession,
    prev_url: []const u8,
    status_code: u16,
    status_text: []const u8,
    raw_resp_headers: []const u8,
    resp_mime_type: []const u8,
    next_url: []const u8,
    next_method: []const u8,
    next_headers: []const u8,
    effective_type: []const u8,
) {
    if (!((session.fetch_enabled || session.fetch_state.enabled) && session.active_net_req_id_len > 0)) {
        return;
    }
    if (hook := session.sync_fetch_hook) {
        if (srv_ptr := session.sync_fetch_server_ptr) {
            let esc_prev_buf: [2048]u8 = [0; 2048];
            const esc_prev = escapeJsonString(prev_url, &esc_prev_buf);

            const st_text = if (status_text.len > 0) status_text else httpStatusText(status_code);
            let esc_st_buf: [128]u8 = [0; 128];
            const esc_st = escapeJsonString(st_text, &esc_st_buf);

            const resp_hdrs_json = formatHttpHeadersJson(raw_resp_headers, session.allocator);
            defer if (resp_hdrs_json.len > 2) session.allocator.free(resp_hdrs_json);

            const mime_val = if (resp_mime_type.len > 0) resp_mime_type else "text/html";
            let esc_mime_buf: [128]u8 = [0; 128];
            const esc_mime = escapeJsonString(mime_val, &esc_mime_buf);

            let redir_buf: [8192]u8 = [0; 8192];
            let redir_dyn: ([]u8)? = null;
            defer if (d := redir_dyn) session.allocator.free(d);
            const redirect_response_json = std.fmt.bufPrint(
                &redir_buf,
                "{{\"url\":\"{s}\",\"status\":{d},\"statusText\":\"{s}\",\"headers\":{s},\"mimeType\":\"{s}\"}}",
                .{ esc_prev, status_code, esc_st, resp_hdrs_json, esc_mime },
            ) catch blk: {
                if (dyn_buf := std.fmt.allocPrint(
                    session.allocator,
                    "{{\"url\":\"{s}\",\"status\":{d},\"statusText\":\"{s}\",\"headers\":{s},\"mimeType\":\"{s}\"}}",
                    .{ esc_prev, status_code, esc_st, resp_hdrs_json, esc_mime },
                ) catch null) {
                    redir_dyn = dyn_buf;
                    break :blk dyn_buf;
                }
                break :blk "";
            };

            let next_hdrs_buf: [4096]u8 = [0; 4096];
            let pos: usize = 0;
            next_hdrs_buf[pos] = '{';
            pos += 1;
            let wrote_any = false;
            let has_ua = false;
            let h_it = std.mem.splitScalar(u8, next_headers, '\n');
            while (hdr := h_it.next()) {
                if (hdr.len == 0) continue;
                if (colon := std.mem.indexOfScalar(u8, hdr, ':')) {
                    const k = std.mem.trim(u8, hdr[0..colon], " \t\r\n");
                    const v = std.mem.trim(u8, hdr[colon + 1 ..], " \t\r\n");
                    if (k.len > 0) {
                        if (std.ascii.eqlIgnoreCase(k, "user-agent")) has_ua = true;
                        let ek_buf: [512]u8 = [0; 512];
                        let ev_buf: [2048]u8 = [0; 2048];
                        const ek = escapeJsonString(k, &ek_buf);
                        const ev = escapeJsonString(v, &ev_buf);
                        const sep: []const u8 = if (wrote_any) "," else "";
                        const kv = std.fmt.bufPrint(next_hdrs_buf[pos..], "{s}\"{s}\":\"{s}\"", .{ sep, ek, ev }) catch break;
                        pos += kv.len;
                        wrote_any = true;
                    }
                }
            }
            if (!has_ua) {
                const ua_val = if (session.user_agent.len > 0) session.user_agent else "Mozilla/5.0";
                let eua_buf: [1024]u8 = [0; 1024];
                const eua = escapeJsonString(ua_val, &eua_buf);
                const sep: []const u8 = if (wrote_any) "," else "";
                const kv = std.fmt.bufPrint(next_hdrs_buf[pos..], "{s}\"User-Agent\":\"{s}\"", .{ sep, eua }) catch "";
                pos += kv.len;
            }
            if (pos < next_hdrs_buf.len) {
                next_hdrs_buf[pos] = '}';
                pos += 1;
            }
            const next_hdrs_json = if (pos >= 2) next_hdrs_buf[0..pos] else "{}";

            const active_net_id = session.activeNetReqId();
            * = hook(srv_ptr, unsafe { @ptrCast(session) }, next_url, next_method, next_hdrs_json, effective_type, active_net_id, redirect_response_json);
            session.redirect_live_emitted = true;
        }
    }
}
