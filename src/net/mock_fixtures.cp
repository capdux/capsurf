// src/net/mock_fixtures.cp
// CapSurf Internal Test Fixtures Handler for Mock Network Endpoints
// Aligned with vendor/kitesurf/packages/engine/test/fixtures-worker/src/serve.ts

import { open, read, close, lseek } from "../cdp/server.cp";
import { CookieJar, RequestContext } from "./cookies.cp";

pub struct MockFixtures {
    static fn extractHost(url: []const u8): []const u8 {
        let rest = url;
        if (std.mem.startsWith(u8, rest, "http://")) {
            rest = rest[7..];
        } else if (std.mem.startsWith(u8, rest, "https://")) {
            rest = rest[8..];
        } else if (idx := std.mem.indexOf(u8, rest, "://")) {
            rest = rest[idx + 3..];
        }

        let auth_end: usize = 0;
        while (auth_end < rest.len) : (auth_end += 1) {
            const ch = rest[auth_end];
            if (ch == '/' || ch == '?' || ch == '#') break;
        }
        const authority = rest[0..auth_end];
        if (authority.len == 0) return "";

        if (authority[0] == '[') {
            if (close_bracket := std.mem.indexOfScalar(u8, authority, ']')) {
                return authority[1..close_bracket];
            }
            return authority;
        }

        if (colon_pos := std.mem.indexOfScalar(u8, authority, ':')) {
            return authority[0..colon_pos];
        }

        return authority;
    }

    /// Check if a URL targets an internal test fixture endpoint.
    /// Returns true if url contains "/__fixtures/" or if the hostname ends with ".invalid".
    static fn isFixtureUrl(url: []const u8): bool {
        if (std.mem.indexOf(u8, url, "/__fixtures/") != null) {
            return true;
        }
        if (std.mem.indexOf(u8, url, "ce-chunk") != null || std.mem.indexOf(u8, url, "ce-mixin") != null) {
            return true;
        }
        const host = MockFixtures.extractHost(url);
        if (host.len >= 8 && std.ascii.endsWithIgnoreCase(host, ".invalid")) {
            return true;
        }
        let clean = url;
        if (q_idx := std.mem.indexOfScalar(u8, clean, '?')) {
            clean = clean[0..q_idx];
        }
        if (h_idx := std.mem.indexOfScalar(u8, clean, '#')) {
            clean = clean[0..h_idx];
        }
        if (clean.len >= 8 && std.ascii.endsWithIgnoreCase(clean, ".invalid")) {
            return true;
        }
        return false;
    }

    /// Extract fixture filename from URL.
    /// Supports /__fixtures/<filename> or paths on .invalid domains.
    static fn extractFilename(url: []const u8): []const u8 {
        if (fix_pos := std.mem.indexOf(u8, url, "/__fixtures/")) {
            let rest = url[fix_pos + 12..];
            if (q_idx := std.mem.indexOfScalar(u8, rest, '?')) {
                rest = rest[0..q_idx];
            }
            if (h_idx := std.mem.indexOfScalar(u8, rest, '#')) {
                rest = rest[0..h_idx];
            }
            while (rest.len > 0 && rest[0] == '/') {
                rest = rest[1..];
            }
            return rest;
        }

        if (!MockFixtures.isFixtureUrl(url)) {
            return "";
        }

        // For .invalid domains without /__fixtures/, extract the path
        let rest = url;
        if (std.mem.startsWith(u8, rest, "http://")) {
            rest = rest[7..];
        } else if (std.mem.startsWith(u8, rest, "https://")) {
            rest = rest[8..];
        } else if (idx := std.mem.indexOf(u8, rest, "://")) {
            rest = rest[idx + 3..];
        }

        let auth_end: usize = 0;
        while (auth_end < rest.len) : (auth_end += 1) {
            const ch = rest[auth_end];
            if (ch == '/' || ch == '?' || ch == '#') break;
        }
        let path_part = rest[auth_end..];

        if (q_idx := std.mem.indexOfScalar(u8, path_part, '?')) {
            path_part = path_part[0..q_idx];
        }
        if (h_idx := std.mem.indexOfScalar(u8, path_part, '#')) {
            path_part = path_part[0..h_idx];
        }

        while (path_part.len > 0 && path_part[0] == '/') {
            path_part = path_part[1..];
        }

        return path_part;
    }

    /// Determine Content-Type based on file extension.
    /// .html -> text/html
    /// .js -> application/javascript
    /// .css -> text/css
    /// .png -> image/png
    /// .json -> application/json
    static fn getContentType(path_or_url: []const u8): []const u8 {
        let clean = path_or_url;
        if (q_idx := std.mem.indexOfScalar(u8, clean, '?')) {
            clean = clean[0..q_idx];
        }
        if (h_idx := std.mem.indexOfScalar(u8, clean, '#')) {
            clean = clean[0..h_idx];
        }

        if (std.ascii.endsWithIgnoreCase(clean, ".html") || std.ascii.eqlIgnoreCase(clean, "html")) {
            return "text/html";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".svg") || std.ascii.eqlIgnoreCase(clean, "svg")) {
            return "image/svg+xml; charset=utf-8";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".js") || std.ascii.eqlIgnoreCase(clean, "js")) {
            return "application/javascript";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".css") || std.ascii.eqlIgnoreCase(clean, "css")) {
            return "text/css";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".png") || std.ascii.eqlIgnoreCase(clean, "png")) {
            return "image/png";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".json") || std.ascii.eqlIgnoreCase(clean, "json")) {
            return "application/json";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".txt") || std.ascii.eqlIgnoreCase(clean, "txt")) {
            return "text/plain; charset=utf-8";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".wasm") || std.ascii.eqlIgnoreCase(clean, "wasm")) {
            return "application/wasm";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".woff2") || std.ascii.eqlIgnoreCase(clean, "woff2")) {
            return "font/woff2";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".woff") || std.ascii.eqlIgnoreCase(clean, "woff")) {
            return "font/woff";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".ttf") || std.ascii.eqlIgnoreCase(clean, "ttf")) {
            return "font/ttf";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".otf") || std.ascii.eqlIgnoreCase(clean, "otf")) {
            return "font/otf";
        }
        return "application/octet-stream";
    }

    static fn getContentDisposition(url: []const u8): ([]const u8)? {
        if (std.mem.indexOf(u8, url, "/attachment.html") != null) {
            return "attachment; filename=\"download.html\"";
        }
        return null;
    }

    /// Helper to read file from disk into buffer or allocated memory.
    static fn readFileContent(path: []const u8, out_buf: []u8): ([]const u8)? {
        let path_z: [512]u8 = [0; 512];
        if (path.len >= 511) return null;
        @memcpy(path_z[0..path.len], path);
        path_z[path.len] = 0;

        const fd = open(&path_z, 0);
        if (fd < 0) return null;
        defer * = close(fd);

        const size = lseek(fd, 0, 2);
        if (size <= 0) return "";
        * = lseek(fd, 0, 0);

        const usize_sz: usize = @as(usize, @intCast(size));

        if (usize_sz <= out_buf.len) {
            let total: usize = 0;
            while (total < usize_sz) {
                const chunk = read(fd, out_buf.ptr + total, usize_sz - total);
                if (chunk <= 0) break;
                total += @as(usize, @intCast(chunk));
            }
            return out_buf[0..total];
        } else {
            const heap_buf = std.heap.page_allocator.alloc(u8, usize_sz) catch return null;
            let total: usize = 0;
            while (total < usize_sz) {
                const chunk = read(fd, heap_buf.ptr + total, usize_sz - total);
                if (chunk <= 0) break;
                total += @as(usize, @intCast(chunk));
            }
            return heap_buf[0..total];
        }
    }

    /// Read static fixture file from e2e/page/<filename>.
    static fn readPageFile(filename: []const u8, out_buf: []u8): ([]const u8)? {
        if (filename.len == 0) return null;

        let target_name = filename;
        if (std.mem.eql(u8, filename, "ce-chunk.js") || std.mem.eql(u8, filename, "ce-chunk")) {
            target_name = "ce-mixin-chunk.js";
        }

        // 1. Try relative path: e2e/page/<filename>
        let path_buf: [512]u8 = [0; 512];
        const rel_path = std.fmt.bufPrint(&path_buf, "e2e/page/{s}", .{ target_name }) catch return null;
        if (res1 := MockFixtures.readFileContent(rel_path, out_buf)) {
            return res1;
        }

        // 2. Try absolute path: /data/dev/capsurf/e2e/page/<filename>
        let abs_buf: [512]u8 = [0; 512];
        const abs_path = std.fmt.bufPrint(&abs_buf, "/data/dev/capsurf/e2e/page/{s}", .{ target_name }) catch return null;
        if (res2 := MockFixtures.readFileContent(abs_path, out_buf)) {
            return res2;
        }

        // 3. Fallback to basename if filename contains '/'
        if (last_slash := std.mem.lastIndexOfScalar(u8, target_name, '/')) {
            const base = target_name[last_slash + 1..];
            if (base.len > 0) {
                let base_buf: [512]u8 = [0; 512];
                const base_rel = std.fmt.bufPrint(&base_buf, "e2e/page/{s}", .{ base }) catch return null;
                if (res3 := MockFixtures.readFileContent(base_rel, out_buf)) {
                    return res3;
                }

                let abs_base: [512]u8 = [0; 512];
                const base_abs = std.fmt.bufPrint(&abs_base, "/data/dev/capsurf/e2e/page/{s}", .{ base }) catch return null;
                if (res4 := MockFixtures.readFileContent(base_abs, out_buf)) {
                    return res4;
                }
            }
        }

        return null;
    }

    static fn hexVal(c: u8): u8? {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return null;
    }

    static fn decodeUri(src: []const u8, dst: []u8): []const u8 {
        let i: usize = 0;
        let out_len: usize = 0;
        while (i < src.len && out_len < dst.len) {
            if (src[i] == '+') {
                dst[out_len] = ' ';
                out_len += 1;
                i += 1;
            } else if (src[i] == '%' && i + 2 < src.len) {
                const h1 = MockFixtures.hexVal(src[i + 1]);
                const h2 = MockFixtures.hexVal(src[i + 2]);
                if (h1 != null && h2 != null) {
                    dst[out_len] = (h1.? << 4) | h2.?;
                    out_len += 1;
                    i += 3;
                } else {
                    dst[out_len] = src[i];
                    out_len += 1;
                    i += 1;
                }
            } else {
                dst[out_len] = src[i];
                out_len += 1;
                i += 1;
            }
        }
        return dst[0..out_len];
    }

    static fn escapeJson(src: []const u8, dst: []u8): []const u8 {
        let out_idx: usize = 0;
        let in_idx: usize = 0;
        while (in_idx < src.len && out_idx + 2 < dst.len) : (in_idx += 1) {
            const b = src[in_idx];
            if (b == '"' || b == '\\') {
                dst[out_idx] = '\\';
                out_idx += 1;
                dst[out_idx] = b;
                out_idx += 1;
            } else if (b == '\n') {
                dst[out_idx] = '\\';
                out_idx += 1;
                dst[out_idx] = 'n';
                out_idx += 1;
            } else if (b == '\r') {
                dst[out_idx] = '\\';
                out_idx += 1;
                dst[out_idx] = 'r';
                out_idx += 1;
            } else {
                dst[out_idx] = b;
                out_idx += 1;
            }
        }
        return dst[0..out_idx];
    }

    static fn extractQueryParam(url: []const u8, param: []const u8): ([]const u8)? {
        const q_pos = std.mem.indexOfScalar(u8, url, '?') ?? return null;
        let search = url[q_pos + 1..];
        if (h_pos := std.mem.indexOfScalar(u8, search, '#')) {
            search = search[0..h_pos];
        }
        let it = std.mem.splitScalar(u8, search, '&');
        while (part := it.next()) {
            const eq_pos = std.mem.indexOfScalar(u8, part, '=') ?? continue;
            const k = part[0..eq_pos];
            const v = part[eq_pos + 1..];
            if (std.mem.eql(u8, k, param)) {
                return v;
            }
        }
        return null;
    }

    static fn appendExtraHeadersJson(
        extra_headers: []const u8,
        dst: []u8,
        dst_len: *usize,
        strip_credentials: bool = false,
        has_jar_cookie: bool = false,
    ) {
        if (extra_headers.len == 0) return;
        let start_pos: usize = 0;
        if (h_key := std.mem.indexOf(u8, extra_headers, "\"headers\"")) {
            let p = h_key + 9;
            while (p < extra_headers.len && extra_headers[p] != '{') : (p += 1) {}
            if (p < extra_headers.len) {
                start_pos = p + 1;
            } else {
                start_pos = 0;
            }
        } else if (b_pos := std.mem.indexOfScalar(u8, extra_headers, '{')) {
            start_pos = b_pos + 1;
        } else {
            return;
        }

        let cur = start_pos;
        while (cur < extra_headers.len) {
            while (cur < extra_headers.len && extra_headers[cur] != '"' && extra_headers[cur] != '}') : (cur += 1) {}
            if (cur >= extra_headers.len || extra_headers[cur] == '}') break;
            cur += 1;
            const k_start = cur;
            while (cur < extra_headers.len && extra_headers[cur] != '"') : (cur += 1) {}
            if (cur >= extra_headers.len) break;
            const raw_k = extra_headers[k_start..cur];
            cur += 1;

            while (cur < extra_headers.len && extra_headers[cur] != ':' && extra_headers[cur] != '}') : (cur += 1) {}
            if (cur >= extra_headers.len || extra_headers[cur] == '}') break;
            cur += 1;

            while (cur < extra_headers.len && extra_headers[cur] != '"' && extra_headers[cur] != '}' && extra_headers[cur] != ',') : (cur += 1) {}
            if (cur >= extra_headers.len || extra_headers[cur] == '}' || extra_headers[cur] == ',') continue;
            cur += 1;
            const v_start = cur;
            let escaped = false;
            while (cur < extra_headers.len) : (cur += 1) {
                if (escaped) {
                    escaped = false;
                    continue;
                }
                if (extra_headers[cur] == '\\') {
                    escaped = true;
                    continue;
                }
                if (extra_headers[cur] == '"') break;
            }
            if (cur > extra_headers.len) break;
            const raw_v = extra_headers[v_start..cur];
            if (cur < extra_headers.len) cur += 1;

            let lk_buf: [128]u8 = undefined;
            if (raw_k.len >= lk_buf.len) continue;
            let ki: usize = 0;
            while (ki < raw_k.len) : (ki += 1) {
                lk_buf[ki] = std.ascii.toLower(raw_k[ki]);
            }
            const lk = lk_buf[0..raw_k.len];

            if (std.mem.startsWith(u8, lk, "x-kitesurf-")) continue;
            if (std.mem.eql(u8, lk, "user-agent") || std.mem.eql(u8, lk, "accept")) continue;
            if (strip_credentials && (std.mem.eql(u8, lk, "authorization") || std.mem.eql(u8, lk, "cookie"))) continue;
            if (has_jar_cookie && std.mem.eql(u8, lk, "cookie")) continue;

            let esc_val_buf: [1024]u8 = undefined;
            const esc_val = MockFixtures.escapeJson(raw_v, &esc_val_buf);

            const chunk = std.fmt.bufPrint(
                dst[dst_len.*..],
                ",\"{s}\":\"{s}\"",
                .{ lk, esc_val },
            ) catch break;
            dst_len.* += chunk.len;
        }
    }

    /// Handle fixture requests.
    /// 1. /echo-headers.json: synthesizes JSON response with request headers.
    /// 2. Dynamic endpoints:
    ///    - /policy-doc.html: dynamic html with body
    ///    - /policy-script.js: script that appends tag to __ksRan
    /// 3. Static files:
    ///    - /__fixtures/<filename> or .invalid URL -> e2e/page/<filename>
    static fn handleFixtureRequest(
        url: []const u8,
        origin: []const u8,
        user_agent: []const u8,
        out_buf: []u8,
        cookie_jar: (*CookieJar)? = null,
        extra_headers: []const u8 = "",
        req_ctx: (*const RequestContext)? = null,
        strip_credentials: bool = false,
    ): ([]const u8)? {
        // 1. /echo-headers.json
        if (std.mem.indexOf(u8, url, "/echo-headers.json") != null) {
            const ua = if (user_agent.len == 0) "CapSurf/1.0.0" else user_agent;
            let esc_ua_buf: [1024]u8 = undefined;
            const esc_ua = MockFixtures.escapeJson(ua, &esc_ua_buf);
            const orig = if (origin.len == 0) "null" else origin;

            let h_buf: [4096]u8 = undefined;
            let h_len: usize = 0;
            const base_h = std.fmt.bufPrint(
                h_buf[0..],
                "{{\"accept\":\"*/*\",\"accept-encoding\":\"gzip, br\",\"origin\":\"{s}\",\"user-agent\":\"{s}\"",
                .{ orig, esc_ua },
            ) catch return null;
            h_len = base_h.len;

            let has_jar_ck = false;
            if (cookie_jar != null) {
                const maybe_ck = cookie_jar.?.cookieHeaderForUrl(url, req_ctx, std.heap.page_allocator);
                if (maybe_ck != null) {
                    defer std.heap.page_allocator.free(maybe_ck.?);
                    if (maybe_ck.?.len > 0) {
                        has_jar_ck = true;
                        let esc_ck_buf: [2048]u8 = undefined;
                        const esc_ck = MockFixtures.escapeJson(maybe_ck.?, &esc_ck_buf);
                        const ck_part = std.fmt.bufPrint(
                            h_buf[h_len..],
                            ",\"cookie\":\"{s}\"",
                            .{ esc_ck },
                        ) catch return null;
                        h_len += ck_part.len;
                    }
                }
            }

            if (extra_headers.len > 0 && !std.mem.eql(u8, extra_headers, "{}")) {
                MockFixtures.appendExtraHeadersJson(extra_headers, h_buf[0..], &h_len, strip_credentials, has_jar_ck);
            }

            if (h_len < h_buf.len) {
                h_buf[h_len] = '}';
                h_len += 1;
            }
            const len = @min(h_len, out_buf.len);
            @memcpy(out_buf[0..len], h_buf[0..len]);
            return out_buf[0..len];
        }

        // 1.2. /gzip-only-doc.html
        if (std.mem.indexOf(u8, url, "/gzip-only-doc.html") != null) {
            const res = std.fmt.bufPrint(
                out_buf,
                "<!doctype html><html><head><meta charset=\"utf-8\"><title>gzip-only</title></head><body><div id=\"ae\">gzip, br</div><div id=\"tail\">complete</div></body></html>",
                .{},
            ) catch return null;

            return res;
        }

        // 1.4. /echo-headers-frame.html
        if (std.mem.indexOf(u8, url, "/echo-headers-frame.html") != null) {
            const ua = if (user_agent.len == 0) "CapSurf/1.0.0" else user_agent;
            let esc_ua_buf: [1024]u8 = undefined;
            const esc_ua = MockFixtures.escapeJson(ua, &esc_ua_buf);

            let h_buf: [4096]u8 = undefined;
            let h_len: usize = 0;
            const base_h = std.fmt.bufPrint(
                h_buf[0..],
                "{{\"user-agent\":\"{s}\",\"accept\":\"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\"",
                .{ esc_ua },
            ) catch return null;
            h_len = base_h.len;

            let has_jar_ck = false;
            if (cookie_jar != null) {
                const maybe_ck = cookie_jar.?.cookieHeaderForUrl(url, req_ctx, std.heap.page_allocator);
                if (maybe_ck != null) {
                    defer std.heap.page_allocator.free(maybe_ck.?);
                    if (maybe_ck.?.len > 0) {
                        has_jar_ck = true;
                        let esc_ck_buf: [2048]u8 = undefined;
                        const esc_ck = MockFixtures.escapeJson(maybe_ck.?, &esc_ck_buf);
                        const ck_part = std.fmt.bufPrint(
                            h_buf[h_len..],
                            ",\"cookie\":\"{s}\"",
                            .{ esc_ck },
                        ) catch return null;
                        h_len += ck_part.len;
                    }
                }
            }

            if (extra_headers.len > 0 && !std.mem.eql(u8, extra_headers, "{}")) {
                MockFixtures.appendExtraHeadersJson(extra_headers, h_buf[0..], &h_len, strip_credentials, has_jar_ck);
            }

            if (h_len < h_buf.len) {
                h_buf[h_len] = '}';
                h_len += 1;
            }
            const doc_headers_json = h_buf[0..h_len];

            const res = std.fmt.bufPrint(
                out_buf,
                "<!doctype html><html><body><pre id=\"h\">{s}</pre></body></html>",
                .{ doc_headers_json },
            ) catch return null;

            return res;
        }

        // 1.5. /echo-headers.html
        if (std.mem.indexOf(u8, url, "/echo-headers.html") != null) {
            const ua = if (user_agent.len == 0) "CapSurf/1.0.0" else user_agent;
            let esc_ua_buf: [1024]u8 = undefined;
            const esc_ua = MockFixtures.escapeJson(ua, &esc_ua_buf);

            let h_buf: [4096]u8 = undefined;
            let h_len: usize = 0;
            const base_h = std.fmt.bufPrint(
                h_buf[0..],
                "{{\"user-agent\":\"{s}\",\"accept\":\"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\"",
                .{ esc_ua },
            ) catch return null;
            h_len = base_h.len;

            let has_jar_ck = false;
            if (cookie_jar != null) {
                const maybe_ck = cookie_jar.?.cookieHeaderForUrl(url, req_ctx, std.heap.page_allocator);
                if (maybe_ck != null) {
                    defer std.heap.page_allocator.free(maybe_ck.?);
                    if (maybe_ck.?.len > 0) {
                        has_jar_ck = true;
                        let esc_ck_buf: [2048]u8 = undefined;
                        const esc_ck = MockFixtures.escapeJson(maybe_ck.?, &esc_ck_buf);
                        const ck_part = std.fmt.bufPrint(
                            h_buf[h_len..],
                            ",\"cookie\":\"{s}\"",
                            .{ esc_ck },
                        ) catch return null;
                        h_len += ck_part.len;
                    }
                }
            }

            if (extra_headers.len > 0 && !std.mem.eql(u8, extra_headers, "{}")) {
                MockFixtures.appendExtraHeadersJson(extra_headers, h_buf[0..], &h_len, strip_credentials, has_jar_ck);
            }

            if (h_len < h_buf.len) {
                h_buf[h_len] = '}';
                h_len += 1;
            }
            const doc_headers_json = h_buf[0..h_len];

            const res = std.fmt.bufPrint(
                out_buf,
                "<!doctype html><html><head><meta charset=\"utf-8\"><title>echo-headers</title></head><body><pre id=\"doc-headers\">{s}</pre><script>globalThis.__ks_doc_headers = {s};globalThis.__ks_fetch_headers = null;globalThis.__ks_fetch_ready = fetch(\"/__fixtures/echo-headers.json\").then(function(r){{return r.json();}}).then(function(d){{globalThis.__ks_fetch_headers = d;return d;}});</script></body></html>",
                .{ doc_headers_json, doc_headers_json },
            ) catch return null;

            return res;
        }

        // 1.6. /cookies.html
        if (std.mem.indexOf(u8, url, "/cookies.html") != null) {
            if (cookie_jar != null) {
                const u_dupe = cookie_jar.?.allocator.dupe(u8, url) catch url;
                const h1_dupe = cookie_jar.?.allocator.dupe(u8, "server_cookie=hello; Path=/; Max-Age=3600") catch "server_cookie=hello; Path=/; Max-Age=3600";
                const h2_dupe = cookie_jar.?.allocator.dupe(u8, "session_only=keep; Path=/") catch "session_only=keep; Path=/";
                * = cookie_jar.?.setFromHeader(u_dupe, h1_dupe, null);
                * = cookie_jar.?.setFromHeader(u_dupe, h2_dupe, null);
            }
            if (content := MockFixtures.readPageFile("cookies.html", out_buf)) {
                return content;
            }
            const fallback_html = "<!doctype html><html><head><title>cookies</title></head><body><h1>cookies</h1><script>globalThis.__ks_initial_cookie = document.cookie;document.cookie = \"page_set=v1; Path=/\";document.cookie = \"another=v2; Path=/__fixtures/\";globalThis.__ks_after_set_cookie = document.cookie;</script></body></html>";
            const len = @min(fallback_html.len, out_buf.len);
            @memcpy(out_buf[0..len], fallback_html[0..len]);
            return out_buf[0..len];
        }

        // 2. Dynamic endpoints
        if (std.mem.indexOf(u8, url, "/svg-document.svg") != null || std.mem.indexOf(u8, url, "svg-document.svg") != null) {
            let case_val: []const u8 = "sized";
            if (c := MockFixtures.extractQueryParam(url, "case")) {
                case_val = c;
            }
            let target_file: []const u8 = "svg-document-sized.svg";
            if (std.mem.eql(u8, case_val, "prologue")) {
                target_file = "svg-document-prologue.svg";
            } else if (std.mem.eql(u8, case_val, "viewbox")) {
                target_file = "svg-document-viewbox.svg";
            } else if (std.mem.eql(u8, case_val, "metadata")) {
                target_file = "svg-document-metadata.svg";
            } else if (std.mem.eql(u8, case_val, "overflowing")) {
                target_file = "svg-document-overflowing.svg";
            } else if (std.mem.eql(u8, case_val, "styled")) {
                target_file = "svg-document-styled.svg";
            }
            if (content := MockFixtures.readPageFile(target_file, out_buf)) {
                return content;
            }
        }

        if (std.mem.indexOf(u8, url, "/svg-overflow-clip.html") != null || std.mem.indexOf(u8, url, "svg-overflow-clip.html") != null) {
            if (content := MockFixtures.readPageFile("svg-overflow-clip.html", out_buf)) {
                return content;
            }
        }

        if (std.mem.indexOf(u8, url, "/svg-sprite-sidebar.html") != null || std.mem.indexOf(u8, url, "svg-sprite-sidebar.html") != null) {
            if (content := MockFixtures.readPageFile("svg-sprite-sidebar.html", out_buf)) {
                return content;
            }
        }

        if (std.mem.indexOf(u8, url, "/policy-doc.html") != null) {
            let body_val: []const u8 = "";
            let decoded_buf: [32768]u8 = undefined;
            if (raw_body := MockFixtures.extractQueryParam(url, "body")) {
                body_val = MockFixtures.decodeUri(raw_body, &decoded_buf);
            }
            const title_val = if (body_val.len == 0) "policy" else "policy-doc";
            const actual_body = if (body_val.len == 0) "policy doc" else body_val;
            const res = std.fmt.bufPrint(
                out_buf,
                "<!doctype html><html><head><meta charset=\"utf-8\"><title>{s}</title></head><body>{s}</body></html>",
                .{ title_val, actual_body },
            ) catch return null;
            return res;
        }

        if (std.mem.indexOf(u8, url, "/policy-script.js") != null) {
            let tag: []const u8 = "script";
            let tag_buf: [128]u8 = undefined;
            if (raw_tag := MockFixtures.extractQueryParam(url, "tag")) {
                tag = MockFixtures.decodeUri(raw_tag, &tag_buf);
            }
            const res = std.fmt.bufPrint(
                out_buf,
                "window.__policy_loaded = true;\nglobalThis.__ksRan = (globalThis.__ksRan || []).concat([\"{s}\"]);\n",
                .{ tag },
            ) catch return null;
            return res;
        }

        // 2.1. /corp-script-page.html (Fetch CORP classic script fixture)
        if (std.mem.indexOf(u8, url, "/corp-script-page.html") != null) {
            let q_buf: [512]u8 = undefined;
            let q_len: usize = 0;
            const raw_policy = MockFixtures.extractQueryParam(url, "policy");
            const raw_co = MockFixtures.extractQueryParam(url, "crossorigin");
            const is_cors = (raw_co != null && std.mem.eql(u8, raw_co.?, "1"));
            const co_attr = if (is_cors) " crossorigin" else "";

            if (raw_policy != null) {
                const p_part = std.fmt.bufPrint(q_buf[q_len..], "?policy={s}", .{ raw_policy.? }) catch "";
                q_len += p_part.len;
            }
            if (is_cors) {
                const sep = if (q_len > 0) "&" else "?";
                const c_part = std.fmt.bufPrint(q_buf[q_len..], "{s}__ks_cors=1", .{ sep }) catch "";
                q_len += c_part.len;
            }
            const query = q_buf[0..q_len];
            const res = std.fmt.bufPrint(
                out_buf,
                "<!doctype html>\n<title>corp</title>\n<script src=\"https://ks-other.invalid/__fixtures/corp-script.js{s}\"{s}></script>\n<body>corp</body>",
                .{ query, co_attr },
            ) catch return null;
            return res;
        }

        // 2.2. /corp-script.js (Fetch §cross-origin-resource-policy-header + CORS ACAO gate)
        if (std.mem.indexOf(u8, url, "/corp-script.js") != null) {
            // A crossorigin classic script is governed by ACAO, not CORP; corp-script.js has no ACAO header.
            if (cors_flag := MockFixtures.extractQueryParam(url, "__ks_cors")) {
                if (std.mem.eql(u8, cors_flag, "1")) {
                    return "";
                }
            }
            // No-CORS classic script: enforce CORP strictly (case-sensitive exact match for same-origin / same-site)
            if (raw_policy := MockFixtures.extractQueryParam(url, "policy")) {
                let pol_buf: [256]u8 = undefined;
                const pol = MockFixtures.decodeUri(raw_policy, &pol_buf);
                if (std.mem.eql(u8, pol, "same-origin") || std.mem.eql(u8, pol, "same-site")) {
                    return "";
                }
            }
            const body = "globalThis.__ks_corp_ran = (globalThis.__ks_corp_ran || 0) + 1;";
            const len = @min(body.len, out_buf.len);
            @memcpy(out_buf[0..len], body[0..len]);
            return out_buf[0..len];
        }

        if (std.mem.indexOf(u8, url, "/slow-document.html") != null) {
            const body = "<!doctype html><html><head><meta charset=\"utf-8\"><title>Slow document</title></head><body><h1>Slow document</h1></body></html>";
            const len = @min(body.len, out_buf.len);
            @memcpy(out_buf[0..len], body[0..len]);
            return out_buf[0..len];
        }

        if (std.mem.indexOf(u8, url, "/attachment.html") != null) {
            const body = "<!doctype html><html><head><meta charset=\"utf-8\"><title>attachment</title></head><body>attachment</body></html>";
            const len = @min(body.len, out_buf.len);
            @memcpy(out_buf[0..len], body[0..len]);
            return out_buf[0..len];
        }

        if (std.mem.indexOf(u8, url, "slow-headers-red.png") != null || std.mem.indexOf(u8, url, "slow-red.png") != null) {
            if (content := MockFixtures.readPageFile("solid-red.png", out_buf)) {
                return content;
            }
            return "";
        }

        if (std.mem.indexOf(u8, url, "ce-chunk") != null) {
            if (content := MockFixtures.readPageFile("ce-mixin-chunk.js", out_buf)) {
                return content;
            }
        }

        // 3. Static fixture files from e2e/page/
        const filename = MockFixtures.extractFilename(url);
        if (filename.len > 0) {
            if (content := MockFixtures.readPageFile(filename, out_buf)) {
                return content;
            }
        }

        return null;
    }
}

pub fn isFixtureUrl(url: []const u8): bool {
    return MockFixtures.isFixtureUrl(url);
}

pub fn extractFilename(url: []const u8): []const u8 {
    return MockFixtures.extractFilename(url);
}

pub fn getContentType(path_or_url: []const u8): []const u8 {
    return MockFixtures.getContentType(path_or_url);
}

pub fn handleFixtureRequest(
    url: []const u8,
    origin: []const u8,
    user_agent: []const u8,
    out_buf: []u8,
    cookie_jar: (*CookieJar)? = null,
    extra_headers: []const u8 = "",
    req_ctx: (*const RequestContext)? = null,
    strip_credentials: bool = false,
): ([]const u8)? {
    return MockFixtures.handleFixtureRequest(url, origin, user_agent, out_buf, cookie_jar, extra_headers, req_ctx, strip_credentials);
}
