// src/net/mock_fixtures.cp
// CapSurf Internal Test Fixtures Handler for Mock Network Endpoints
// Aligned with vendor/kitesurf/packages/engine/test/fixtures-worker/src/serve.ts

import { open, read, close, lseek } from "../cdp/server.cp";
import { CookieJar, RequestContext } from "./cookies.cp";
import { parseCsp, isInlineScriptAllowed } from "./csp.cp";

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

        if (std.mem.indexOf(u8, path_or_url, "euckr-child.html") != null) {
            return "text/html; charset=euc-kr";
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
        } else if (std.mem.eql(u8, filename, "euckr-child.html")) {
            target_name = "euckr-child.html.bin";
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
        while (in_idx < src.len && out_idx + 6 < dst.len) : (in_idx += 1) {
            const b = src[in_idx];
            if (b == '"' || b == '\\') {
                dst[out_idx] = '\\';
                out_idx += 1;
                dst[out_idx] = b;
                out_idx += 1;
            } else if (b == '<') {
                @memcpy(dst[out_idx .. out_idx + 6], "\\u003c");
                out_idx += 6;
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

            let val_to_esc = raw_v;
            if (std.mem.eql(u8, lk, "referer")) {
                if (hash_pos := std.mem.indexOfScalar(u8, raw_v, '#')) {
                    val_to_esc = raw_v[0..hash_pos];
                }
            }
            let esc_val_buf: [1024]u8 = undefined;
            const esc_val = MockFixtures.escapeJson(val_to_esc, &esc_val_buf);

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

        if (std.mem.indexOf(u8, url, "/cors-sheet-acao.css") != null || std.mem.indexOf(u8, url, "/cors-sheet-bare.css") != null) {
            if (content := MockFixtures.readPageFile("cors-sheet-echo-origin.css", out_buf)) {
                return content;
            }
        }

        if (std.mem.indexOf(u8, url, "/cors-sheet-private-marker.css") != null) {
            if (content := MockFixtures.readPageFile("cors-sheet-private-marker.css", out_buf)) {
                return content;
            }
        }

        if (std.mem.indexOf(u8, url, "/image-heavy.html") != null) {
            let n_val: usize = 12;
            if (raw_n := MockFixtures.extractQueryParam(url, "n")) {
                const parsed_n = std.fmt.parseInt(usize, raw_n, 10) catch 12;
                n_val = @max(@as(usize, 1), @min(parsed_n, @as(usize, 64)));
            }
            let side_val: usize = 512;
            if (raw_side := MockFixtures.extractQueryParam(url, "side")) {
                const parsed_s = std.fmt.parseInt(usize, raw_side, 10) catch 512;
                side_val = @max(@as(usize, 8), @min(parsed_s, @as(usize, 2048)));
            }
            let pos: usize = 0;
            const head_s = std.fmt.bufPrint(
                out_buf[pos..],
                "<!doctype html>\n<html>\n<head><meta charset=\"utf-8\"><title>image-heavy fixture ({d} x {d}px)</title>\n<style>img {{ display: block; margin: 4px; max-width: 100%; }}</style></head>\n<body>\n<h1>image-heavy: {d} unique {d}x{d} images</h1>\n",
                .{ n_val, side_val, n_val, side_val, side_val },
            ) catch return null;
            pos += head_s.len;
            let seed: usize = 0;
            while (seed < n_val) : (seed += 1) {
                const img_s = std.fmt.bufPrint(
                    out_buf[pos..],
                    "<img id=\"i{d}\" src=\"/__fixtures/big-image.png?seed={d}&side={d}\">\n",
                    .{ seed, seed, side_val },
                ) catch break;
                pos += img_s.len;
            }
            const tail_s = "</body>\n</html>";
            if (pos + tail_s.len <= out_buf.len) {
                @memcpy(out_buf[pos .. pos + tail_s.len], tail_s);
                pos += tail_s.len;
            }
            return out_buf[0..pos];
        }

        if (std.mem.indexOf(u8, url, "/big-image.png") != null) {
            // Minimal valid 1x1 red PNG
            const png_bytes: [69]u8 = [
                0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
                0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
                0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
                0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53,
                0xDE, 0x00, 0x00, 0x00, 0x0C, 0x49, 0x44, 0x41,
                0x54, 0x08, 0xD7, 0x63, 0xF8, 0xCF, 0xC0, 0x00,
                0x00, 0x03, 0x01, 0x01, 0x00, 0x18, 0xDD, 0x8D,
                0xB0, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E,
                0x44, 0xAE, 0x42, 0x60, 0x82,
            ];
            if (png_bytes.len <= out_buf.len) {
                @memcpy(out_buf[0..png_bytes.len], &png_bytes);
                return out_buf[0..png_bytes.len];
            }
        }

        if (std.mem.indexOf(u8, url, "/policy-doc.html") != null) {
            let body_val: []const u8 = "";
            let decoded_buf: [32768]u8 = undefined;
            if (raw_body := MockFixtures.extractQueryParam(url, "body")) {
                body_val = MockFixtures.decodeUri(raw_body, &decoded_buf);
            }
            let csp_meta_buf: [1024]u8 = undefined;
            let csp_meta: []const u8 = "";
            if (raw_csp := MockFixtures.extractQueryParam(url, "csp")) {
                let csp_dec_buf: [512]u8 = undefined;
                let csp_esc_buf: [768]u8 = undefined;
                const csp_val = MockFixtures.decodeUri(raw_csp, &csp_dec_buf);
                const csp_esc = escapeHtmlAttr(csp_val, &csp_esc_buf);
                csp_meta = std.fmt.bufPrint(&csp_meta_buf, "<meta http-equiv=\"Content-Security-Policy\" content=\"{s}\">", .{ csp_esc }) catch "";
            }
            const title_val = if (body_val.len == 0) "policy" else "policy-doc";
            const actual_body = if (body_val.len == 0) "policy doc" else body_val;
            const res = std.fmt.bufPrint(
                out_buf,
                "<!doctype html><html><head><meta charset=\"utf-8\">{s}<title>{s}</title></head><body>{s}</body></html>",
                .{ csp_meta, title_val, actual_body },
            ) catch return null;
            return res;
        }

        if (std.mem.indexOf(u8, url, "/policy-script.js") != null) {
            let tag: []const u8 = "script";
            let tag_buf: [128]u8 = undefined;
            if (raw_tag := MockFixtures.extractQueryParam(url, "tag")) {
                tag = MockFixtures.decodeUri(raw_tag, &tag_buf);
            }
            let has_extra = false;
            if (raw_extra := MockFixtures.extractQueryParam(url, "extra")) {
                if (std.mem.eql(u8, raw_extra, "1")) has_extra = true;
            }
            const extra_suffix = if (has_extra) "/* tampered */\n" else "";
            const res = std.fmt.bufPrint(
                out_buf,
                "globalThis.__ksRan = (globalThis.__ksRan || []).concat([\"{s}\"]);\n{s}",
                .{ tag, extra_suffix },
            ) catch return null;
            return res;
        }

        if (std.mem.indexOf(u8, url, "/oopif-csp-hits/") != null) {
            if (h_pos := std.mem.indexOf(u8, url, "/oopif-csp-hits/")) {
                let key = url[h_pos + "/oopif-csp-hits/".len ..];
                if (q := std.mem.indexOfScalar(u8, key, '?')) key = key[0..q];
                let hit_key_buf: [160]u8 = undefined;
                const hit_key = std.fmt.bufPrint(&hit_key_buf, "oopif-csp:{s}", .{ key }) catch key;
                const hits = getFramePolicyHits(hit_key);
                const res = std.fmt.bufPrint(out_buf, "{{\"hits\":{d}}}", .{ hits }) catch return null;
                return res;
            }
        }

        if (std.mem.indexOf(u8, url, "/oopif-csp-hop/") != null) {
            if (h_pos := std.mem.indexOf(u8, url, "/oopif-csp-hop/")) {
                let key = url[h_pos + "/oopif-csp-hop/".len ..];
                if (q := std.mem.indexOfScalar(u8, key, '?')) key = key[0..q];
                if (std.mem.endsWith(u8, key, ".js")) key = key[0 .. key.len - 3];
                let hit_key_buf: [160]u8 = undefined;
                const hit_key = std.fmt.bufPrint(&hit_key_buf, "oopif-csp:{s}", .{ key }) catch key;
                incrementFramePolicyHits(hit_key);
                const body = "parent.postMessage(\"hop\", \"*\");";
                const len = @min(body.len, out_buf.len);
                @memcpy(out_buf[0..len], body[0..len]);
                return out_buf[0..len];
            }
        }

        if (std.mem.indexOf(u8, url, "/oopif-csp-frame.html") != null) {
            let csp_buf: [512]u8 = undefined;
            let csp_esc_buf: [768]u8 = undefined;
            let csp_meta_buf: [1024]u8 = undefined;
            let csp_meta: []const u8 = "";
            if (raw_csp := MockFixtures.extractQueryParam(url, "csp")) {
                const csp_val = MockFixtures.decodeUri(raw_csp, &csp_buf);
                const csp_esc = escapeHtmlAttr(csp_val, &csp_esc_buf);
                csp_meta = std.fmt.bufPrint(&csp_meta_buf, "<meta http-equiv=\"Content-Security-Policy\" content=\"{s}\">", .{ csp_esc }) catch "";
            }
            let src_buf: [1024]u8 = undefined;
            let src_esc_buf: [1536]u8 = undefined;
            let src_tag_buf: [2048]u8 = undefined;
            let src_tag: []const u8 = "";
            if (raw_src := MockFixtures.extractQueryParam(url, "src")) {
                const src_val = MockFixtures.decodeUri(raw_src, &src_buf);
                if (src_val.len > 0) {
                    const src_esc = escapeHtmlAttr(src_val, &src_esc_buf);
                    src_tag = std.fmt.bufPrint(&src_tag_buf, "<script src=\"{s}\"></script>", .{ src_esc }) catch "";
                }
            }
            let inline_tag: []const u8 = "";
            if (raw_inl := MockFixtures.extractQueryParam(url, "inline")) {
                if (std.mem.eql(u8, raw_inl, "1")) {
                    let allow_inl = true;
                    if (raw_csp2 := MockFixtures.extractQueryParam(url, "csp")) {
                        let csp2_buf: [512]u8 = undefined;
                        const csp2_val = MockFixtures.decodeUri(raw_csp2, &csp2_buf);
                        if (csp2_val.len > 0) {
                            const pol = parseCsp(csp2_val);
                            if (!isInlineScriptAllowed(&pol, "", "parent.postMessage(\"inline\", \"*\")")) {
                                allow_inl = false;
                            }
                        }
                    }
                    if (allow_inl) {
                        inline_tag = "<script>parent.postMessage(\"inline\", \"*\")</script>";
                    }
                }
            }
            const res = std.fmt.bufPrint(
                out_buf,
                "<!doctype html><head>{s}<title>oopif csp</title></head><body>{s}{s}<script nonce=\"n0nce\">parent.postMessage(\"parsed\", \"*\")</script></body>",
                .{ csp_meta, src_tag, inline_tag },
            ) catch return null;
            return res;
        }

        if (std.mem.indexOf(u8, url, "/status-204") != null) {
            return "";
        }

        if (std.mem.indexOf(u8, url, "/oversize-doc.html") != null) {
            const small_doc = "<html><body>small</body></html>";
            const len = @min(small_doc.len, out_buf.len);
            @memcpy(out_buf[0..len], small_doc[0..len]);
            return out_buf[0..len];
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

        if (std.mem.indexOf(u8, url, "/chatty-xhr.html") != null) {
            let fetch_count: u64 = 50;
            let bytes_val: u64 = 200000;
            let conc_val: u64 = 4;
            if (n_str := MockFixtures.extractQueryParam(url, "n")) {
                const parsed_n = std.fmt.parseInt(u64, n_str, 10) catch 50;
                fetch_count = @min(@max(parsed_n, @as(u64, 1)), @as(u64, 2000));
            }
            if (b_str := MockFixtures.extractQueryParam(url, "bytes")) {
                const parsed_b = std.fmt.parseInt(u64, b_str, 10) catch 200000;
                bytes_val = @min(@max(parsed_b, @as(u64, 1)), @as(u64, 4 * 1024 * 1024));
            }
            if (c_str := MockFixtures.extractQueryParam(url, "conc")) {
                const parsed_c = std.fmt.parseInt(u64, c_str, 10) catch 4;
                conc_val = @min(@max(parsed_c, @as(u64, 1)), @as(u64, 16));
            }
            const res = std.fmt.bufPrint(
                out_buf,
                "<!doctype html>\n" ++
                "<html><head><meta charset=\"utf-8\"><title>chatty-xhr</title></head>\n" ++
                "<body><h1 id=\"h\">chatty</h1>\n" ++
                "<script>\n" ++
                "window.__done = false;\n" ++
                "window.__fetched = 0;\n" ++
                "window.__failed = 0;\n" ++
                "(function () {{\n" ++
                "  var FETCH_COUNT = {d}, BYTES = {d}, LANES = {d};\n" ++
                "  var issued = 0;\n" ++
                "  function next() {{\n" ++
                "    if (issued >= FETCH_COUNT) return Promise.resolve();\n" ++
                "    var seed = issued++;\n" ++
                "    return fetch(\"/__fixtures/chatty-json?bytes=\" + BYTES + \"&seed=\" + seed)\n" ++
                "      .then(function (res) {{ return res.text(); }})\n" ++
                "      .then(function () {{ window.__fetched++; }})\n" ++
                "      .catch(function () {{ window.__failed++; }})\n" ++
                "      .then(next);\n" ++
                "  }}\n" ++
                "  var lanes = [];\n" ++
                "  for (var lane = 0; lane < LANES; lane++) lanes.push(next());\n" ++
                "  Promise.all(lanes).then(function () {{\n" ++
                "    window.__done = true;\n" ++
                "    document.getElementById(\"h\").textContent = \"done \" + window.__fetched;\n" ++
                "    var marker = document.createElement(\"div\");\n" ++
                "    marker.id = \"done\";\n" ++
                "    document.body.appendChild(marker);\n" ++
                "  }});\n" ++
                "}})();\n" ++
                "</script>\n" ++
                "</body></html>",
                .{ fetch_count, bytes_val, conc_val },
            ) catch return null;
            return res;
        }

        if (std.mem.indexOf(u8, url, "/chatty-json") != null) {
            let bytes_val: usize = 1024;
            let seed_val: usize = 0;
            if (b_str := MockFixtures.extractQueryParam(url, "bytes")) {
                bytes_val = std.fmt.parseInt(usize, b_str, 10) catch 1024;
            }
            if (s_str := MockFixtures.extractQueryParam(url, "seed")) {
                seed_val = std.fmt.parseInt(usize, s_str, 10) catch 0;
            }
            const prefix = std.fmt.bufPrint(out_buf, "{{\"seed\":{d},\"filler\":\"", .{seed_val}) catch return null;
            const target_len = @min(@max(bytes_val, prefix.len + 2), out_buf.len);
            const fill_end = target_len - 2;
            let fi: usize = prefix.len;
            while (fi < fill_end) : (fi += 1) {
                out_buf[fi] = 'a';
            }
            out_buf[fill_end] = '"';
            out_buf[fill_end + 1] = '}';
            return out_buf[0..target_len];
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

        if (std.mem.indexOf(u8, url, "/font-heavy.html") != null) {
            let declared: u32 = 12;
            let used: u32 = 3;
            if (raw_dec := MockFixtures.extractQueryParam(url, "declared")) {
                const p_dec = std.fmt.parseInt(u32, raw_dec, 10) catch 12;
                declared = @min(@max(p_dec, @as(u32, 1)), @as(u32, 64));
            }
            if (raw_used := MockFixtures.extractQueryParam(url, "used")) {
                const p_used = std.fmt.parseInt(u32, raw_used, 10) catch 3;
                used = @min(p_used, declared);
            }
            let off: usize = 0;
            const head_part = std.fmt.bufPrint(
                out_buf[off..],
                "<!doctype html>\n<html>\n<head><meta charset=\"utf-8\"><title>font-heavy fixture ({d} declared / {d} used)</title>\n<style>\n",
                .{ declared, used },
            ) catch return null;
            off += head_part.len;
            let fi: u32 = 0;
            while (fi < declared) : (fi += 1) {
                const face_part = std.fmt.bufPrint(
                    out_buf[off..],
                    "@font-face {{\n  font-family: \"wf-{d}\";\n  src: url(\"/__fixtures/noto-var-latin-subset.woff2?v={d}\") format(\"woff2\");\n}}\n",
                    .{ fi, fi },
                ) catch break;
                off += face_part.len;
            }
            const mid_part = std.fmt.bufPrint(
                out_buf[off..],
                "p {{ font-size: 18px; margin: 4px; }}\n</style></head>\n<body>\n<h1>font-heavy: {d} declared, {d} used</h1>\n",
                .{ declared, used },
            ) catch return null;
            off += mid_part.len;
            let ui: u32 = 0;
            while (ui < used) : (ui += 1) {
                const p_part = std.fmt.bufPrint(
                    out_buf[off..],
                    "<p style=\"font-family: 'wf-{d}', sans-serif\">used family wf-{d}: lorem ipsum dolor</p>\n",
                    .{ ui, ui },
                ) catch break;
                off += p_part.len;
            }
            const tail_part = "<p style=\"font-family: sans-serif\">control paragraph (UA font)</p>\n</body>\n</html>";
            if (off + tail_part.len <= out_buf.len) {
                @memcpy(out_buf[off .. off + tail_part.len], tail_part);
                off += tail_part.len;
            }
            return out_buf[0..off];
        }

        if (std.mem.indexOf(u8, url, "noto-var-latin-subset.woff2") != null) {
            if (content := MockFixtures.readPageFile("noto-var-latin-subset.woff2", out_buf)) {
                return content;
            }
            if (content := MockFixtures.readPageFile("ks-test-mono.woff2", out_buf)) {
                return content;
            }
            return "";
        }

        if (std.mem.indexOf(u8, url, "/font-defer-probe-hits") != null) {
            let probe_buf: [128]u8 = undefined;
            let probe: []const u8 = "";
            if (raw_p := MockFixtures.extractQueryParam(url, "probe")) {
                probe = MockFixtures.decodeUri(raw_p, &probe_buf);
            }
            const hits = getFramePolicyHits(probe);
            const res = std.fmt.bufPrint(out_buf, "{{\"hits\":{d}}}", .{ hits }) catch return null;
            return res;
        }

        if (std.mem.indexOf(u8, url, "/font-defer-probe.woff2") != null) {
            let probe_buf: [128]u8 = undefined;
            let probe: []const u8 = "";
            if (raw_p := MockFixtures.extractQueryParam(url, "probe")) {
                probe = MockFixtures.decodeUri(raw_p, &probe_buf);
            }
            incrementFramePolicyHits(probe);
            if (content := MockFixtures.readPageFile("ks-test-mono.woff2", out_buf)) {
                return content;
            }
            return "";
        }

        if (std.mem.indexOf(u8, url, "/font-defer.html") != null) {
            let probe_buf: [128]u8 = undefined;
            let probe: []const u8 = "p";
            if (raw_p := MockFixtures.extractQueryParam(url, "probe")) {
                if (raw_p.len > 0) {
                    probe = MockFixtures.decodeUri(raw_p, &probe_buf);
                }
            }
            let ms_val: u32 = 0;
            if (raw_ms := MockFixtures.extractQueryParam(url, "ms")) {
                ms_val = std.fmt.parseInt(u32, raw_ms, 10) catch 0;
            }
            let mode_str: []const u8 = "";
            if (raw_mode := MockFixtures.extractQueryParam(url, "mode")) {
                mode_str = raw_mode;
            }
            let src_buf: [256]u8 = undefined;
            const font_src = if (ms_val > 0)
                (std.fmt.bufPrint(&src_buf, "/__fixtures/font-defer-probe.woff2?probe={s}&ms={d}&bungee=1", .{ probe, ms_val }) catch "/__fixtures/font-defer-probe.woff2?bungee=1")
            else
                (std.fmt.bufPrint(&src_buf, "/__fixtures/font-defer-probe.woff2?probe={s}&bungee=1", .{ probe }) catch "/__fixtures/font-defer-probe.woff2?bungee=1");

            let extra_script: []const u8 = "";
            if (std.mem.eql(u8, mode_str, "measure")) {
                extra_script = "<script>var early=document.getElementById(\"wf\").getBoundingClientRect().width;window.addEventListener(\"load\",function(){var wf=document.getElementById(\"wf\").getBoundingClientRect().width;var fb=document.getElementById(\"fallback\").getBoundingClientRect().width;document.body.setAttribute(\"data-early\",String(early));document.body.setAttribute(\"data-wf\",String(wf));document.body.setAttribute(\"data-fb\",String(fb));var m=document.createElement(\"div\");m.id=\"done\";document.body.appendChild(m);});</script>";
            } else if (std.mem.eql(u8, mode_str, "ready")) {
                extra_script = "<script>document.fonts.ready.then(function(){var wf=document.getElementById(\"wf\").getBoundingClientRect().width;var fb=document.getElementById(\"fallback\").getBoundingClientRect().width;document.body.setAttribute(\"data-wf\",String(wf));document.body.setAttribute(\"data-fb\",String(fb));var m=document.createElement(\"div\");m.id=\"ready\";document.body.appendChild(m);});</script>";
            } else if (std.mem.eql(u8, mode_str, "canvas")) {
                extra_script = "<script>var ctx=document.createElement(\"canvas\").getContext(\"2d\");ctx.font=\"24px sans-serif\";var b=ctx.measureText(\"WebFont123.test\");document.body.setAttribute(\"data-mt1\",String(b.width));document.fonts.ready.then(function(){var a=ctx.measureText(\"WebFont123.test\");document.body.setAttribute(\"data-mt2\",String(a.width));});</script>";
            }

            if (ms_val > 0) {
                const res = std.fmt.bufPrint(
                    out_buf,
                    "<!doctype html>\n<html>\n<head><meta charset=\"utf-8\" /><title>font-defer</title>\n<style>\n  body {{ margin: 0; background: #fff; font-size: 24px; }}\n  span {{ display: inline-block; }}\n  #wf {{ font-family: KsDeferMono; }}\n</style>\n<script>setTimeout(function(){{ var st = document.createElement('style'); st.textContent = '@font-face {{ font-family: KsDeferMono; src: url({s}); }}'; document.head.appendChild(st); st.setAttribute('data-font-loaded', '1'); }}, {d});</script>\n</head>\n<body>\n  <div><span id=\"fallback\">WebFont123.test</span></div>\n  <div><span id=\"wf\">WebFont123.test</span></div>\n  {s}\n</body>\n</html>",
                    .{ font_src, ms_val, extra_script },
                ) catch return null;
                return res;
            } else {
                const res = std.fmt.bufPrint(
                    out_buf,
                    "<!doctype html>\n<html>\n<head><meta charset=\"utf-8\" /><title>font-defer</title>\n<style>\n  @font-face {{ font-family: KsDeferMono; src: url({s}); }}\n  body {{ margin: 0; background: #fff; font-size: 24px; }}\n  span {{ display: inline-block; }}\n  #wf {{ font-family: KsDeferMono; }}\n</style>\n</head>\n<body>\n  <div><span id=\"fallback\">WebFont123.test</span></div>\n  <div><span id=\"wf\">WebFont123.test</span></div>\n  {s}\n</body>\n</html>",
                    .{ font_src, extra_script },
                ) catch return null;
                return res;
            }
        }

        if (std.mem.indexOf(u8, url, "ce-chunk") != null) {
            if (content := MockFixtures.readPageFile("ce-mixin-chunk.js", out_buf)) {
                return content;
            }
        }

        if (std.mem.indexOf(u8, url, "/allow-headers") != null) {
            const body = "ok";
            const len = @min(body.len, out_buf.len);
            @memcpy(out_buf[0..len], body[0..len]);
            return out_buf[0..len];
        }

        if (std.mem.indexOf(u8, url, "/frame-policy-host.html") != null) {
            let child_buf: [2048]u8 = undefined;
            let child_url: []const u8 = "/__fixtures/frame-policy-child.html";
            if (raw_child := MockFixtures.extractQueryParam(url, "child")) {
                child_url = MockFixtures.decodeUri(raw_child, &child_buf);
            }
            let is_dynamic = false;
            if (raw_dyn := MockFixtures.extractQueryParam(url, "dynamic")) {
                if (std.mem.eql(u8, raw_dyn, "1")) {
                    is_dynamic = true;
                }
            }
            if (is_dynamic) {
                let esc_js_buf: [4096]u8 = undefined;
                const esc_js = MockFixtures.escapeJson(child_url, &esc_js_buf);
                const res = std.fmt.bufPrint(
                    out_buf,
                    "<!doctype html>\n<html><head><meta charset=\"utf-8\"><title>frame policy host</title></head>\n<body style=\"margin:0\">\n<script>\nwindow.__msgs = [];\nwindow.__frameLoaded = false;\naddEventListener(\"message\", (e) => window.__msgs.push({{ data: e.data, origin: e.origin }}));\n</script>\n<script>\nconst f = document.createElement(\"iframe\");\nf.id = \"f\";\nf.src = \"{s}\";\nf.style.cssText = \"border:0;width:200px;height:120px\";\nf.addEventListener(\"load\", () => {{ window.__frameLoaded = true; }});\ndocument.body.appendChild(f);\n</script>\n</body></html>",
                    .{ esc_js },
                ) catch return null;
                return res;
            } else {
                let esc_attr_buf: [4096]u8 = undefined;
                const esc_attr = escapeHtmlAttr(child_url, &esc_attr_buf);
                const res = std.fmt.bufPrint(
                    out_buf,
                    "<!doctype html>\n<html><head><meta charset=\"utf-8\"><title>frame policy host</title></head>\n<body style=\"margin:0\">\n<script>\nwindow.__msgs = [];\nwindow.__frameLoaded = false;\naddEventListener(\"message\", (e) => window.__msgs.push({{ data: e.data, origin: e.origin }}));\n</script>\n<iframe id=\"f\" onload=\"window.__frameLoaded = true\" src=\"{s}\" style=\"border:0;width:200px;height:120px\"></iframe>\n</body></html>",
                    .{ esc_attr },
                ) catch return null;
                return res;
            }
        }

        if (std.mem.indexOf(u8, url, "/frame-policy-hits") != null) {
            let marker_buf: [128]u8 = undefined;
            let marker: []const u8 = "";
            if (raw_marker := MockFixtures.extractQueryParam(url, "marker")) {
                marker = MockFixtures.decodeUri(raw_marker, &marker_buf);
            }
            const hits = getFramePolicyHits(marker);
            const res = std.fmt.bufPrint(out_buf, "{{\"hits\":{d}}}", .{ hits }) catch return null;
            return res;
        }

        if (std.mem.indexOf(u8, url, "/frame-policy-child.html") != null) {
            let marker_buf: [128]u8 = undefined;
            let marker: []const u8 = "child";
            if (raw_marker := MockFixtures.extractQueryParam(url, "marker")) {
                marker = MockFixtures.decodeUri(raw_marker, &marker_buf);
            }
            incrementFramePolicyHits(marker);

            let csp_buf: [512]u8 = undefined;
            let csp_opt: ([]const u8)? = null;
            if (raw_csp := MockFixtures.extractQueryParam(url, "csp")) {
                csp_opt = MockFixtures.decodeUri(raw_csp, &csp_buf);
            }
            let xfo_buf: [512]u8 = undefined;
            let xfo_opt: ([]const u8)? = null;
            if (raw_xfo := MockFixtures.extractQueryParam(url, "xfo")) {
                xfo_opt = MockFixtures.decodeUri(raw_xfo, &xfo_buf);
            }

            const ancestor_origin = if (origin.len > 0) origin else "https://ks.invalid";
            if (!isFrameEmbeddingAllowed(url, ancestor_origin, csp_opt, xfo_opt)) {
                const blank = "<!doctype html><html><head></head><body></body></html>";
                const len = @min(blank.len, out_buf.len);
                @memcpy(out_buf[0..len], blank[0..len]);
                return out_buf[0..len];
            }

            let no_script = false;
            if (raw_script := MockFixtures.extractQueryParam(url, "script")) {
                if (std.mem.eql(u8, raw_script, "0")) {
                    no_script = true;
                }
            }
            let esc_marker_buf: [256]u8 = undefined;
            const esc_marker = MockFixtures.escapeJson(marker, &esc_marker_buf);
            if (no_script) {
                const res = std.fmt.bufPrint(
                    out_buf,
                    "<!doctype html>\n<html><head><meta charset=\"utf-8\"><title>frame policy child</title></head>\n<body><p id=\"secret\">protected {s}</p>\n</body></html>",
                    .{ marker },
                ) catch return null;
                return res;
            } else {
                const res = std.fmt.bufPrint(
                    out_buf,
                    "<!doctype html>\n<html><head><meta charset=\"utf-8\"><title>frame policy child</title></head>\n<body><p id=\"secret\">protected {s}</p>\n<script>parent.postMessage(\"ran:{s}\", \"*\")</script>\n</body></html>",
                    .{ marker, esc_marker },
                ) catch return null;
                return res;
            }
        }

        // 2.9. /inserted-bake.js
        if (std.mem.indexOf(u8, url, "/inserted-bake.js") != null) {
            const len = @min(INSERTED_BAKE_JS.len, out_buf.len);
            @memcpy(out_buf[0..len], INSERTED_BAKE_JS[0..len]);
            return out_buf[0..len];
        }

        // 2.10. /inserted-probe-hits/:key
        if (pos := std.mem.indexOf(u8, url, "/inserted-probe-hits/")) {
            let key = url[pos + 21 ..];
            if (q := std.mem.indexOfScalar(u8, key, '?')) key = key[0..q];
            if (h := std.mem.indexOfScalar(u8, key, '#')) key = key[0..h];
            const hits = getInsertedProbeHits(key);
            const res = std.fmt.bufPrint(out_buf, "{{\"hits\":{d}}}", .{ hits }) catch return null;
            return res;
        }

        // 2.11. /inserted-probe-headers/:key
        if (pos := std.mem.indexOf(u8, url, "/inserted-probe-headers/")) {
            let key = url[pos + 24 ..];
            if (q := std.mem.indexOfScalar(u8, key, '?')) key = key[0..q];
            if (h := std.mem.indexOfScalar(u8, key, '#')) key = key[0..h];
            return formatInsertedProbeHeadersJson(key, out_buf);
        }

        // 2.12. /inserted-probe/:mode/:key.js
        if (pos := std.mem.indexOf(u8, url, "/inserted-probe/")) {
            let rest = url[pos + 16 ..];
            if (q := std.mem.indexOfScalar(u8, rest, '?')) rest = rest[0..q];
            if (h := std.mem.indexOfScalar(u8, rest, '#')) rest = rest[0..h];
            if (slash := std.mem.indexOfScalar(u8, rest, '/')) {
                const mode = rest[0..slash];
                let file_part = rest[slash + 1 ..];
                if (std.mem.endsWith(u8, file_part, ".js")) {
                    file_part = file_part[0 .. file_part.len - 3];
                }
                let secret_buf: [128]u8 = [0; 128];
                const secret_opt = extractExtraHeaderIgnoreCase(extra_headers, "x-probe-secret", &secret_buf);
                const hit = recordInsertedProbeHit(file_part, secret_opt);
                if (hit > 1 && std.mem.eql(u8, mode, "gone")) {
                    return null;
                }
                if (std.mem.eql(u8, mode, "volatile")) {
                    const res = std.fmt.bufPrint(out_buf, "{s}\n// hit {d}\n", .{ INSERTED_BAKE_JS, hit }) catch return null;
                    return res;
                }
                if (hit > 1 && std.mem.eql(u8, mode, "hostile")) {
                    const len = @min(HOSTILE_PROBE_JS.len, out_buf.len);
                    @memcpy(out_buf[0..len], HOSTILE_PROBE_JS[0..len]);
                    return out_buf[0..len];
                }
                const len = @min(INSERTED_BAKE_JS.len, out_buf.len);
                @memcpy(out_buf[0..len], INSERTED_BAKE_JS[0..len]);
                return out_buf[0..len];
            }
        }

        // 2.13. /inserted-module/:file
        if (pos := std.mem.indexOf(u8, url, "/inserted-module/")) {
            let file = url[pos + 17 ..];
            if (q := std.mem.indexOfScalar(u8, file, '?')) file = file[0..q];
            if (h := std.mem.indexOfScalar(u8, file, '#')) file = file[0..h];
            if (std.mem.eql(u8, file, "tool.js")) {
                const body = "import { label } from \"./dep.js\";\ndocument.modelContext.registerTool({\n  name: \"inserted_tool\",\n  description: \"Registered by an inserted module\",\n  execute: () => label,\n});\nglobalThis.__insertedModule = (globalThis.__insertedModule || []).concat([\"tool\"]);\n";
                const len = @min(body.len, out_buf.len);
                @memcpy(out_buf[0..len], body[0..len]);
                return out_buf[0..len];
            }
            if (std.mem.eql(u8, file, "dep.js")) {
                const body = "export const label = \"from-dep\";\n";
                const len = @min(body.len, out_buf.len);
                @memcpy(out_buf[0..len], body[0..len]);
                return out_buf[0..len];
            }
            if (std.mem.eql(u8, file, "throws.js")) {
                const body = "globalThis.__insertedModule = (globalThis.__insertedModule || []).concat([\"throws\"]);\nthrow new Error(\"inserted module threw\");\n";
                const len = @min(body.len, out_buf.len);
                @memcpy(out_buf[0..len], body[0..len]);
                return out_buf[0..len];
            }
            if (std.mem.eql(u8, file, "throws-after-lazy-404.js")) {
                const body = "globalThis.__insertedModule = (globalThis.__insertedModule || []).concat([\"lazy\"]);\nawait import(\"./lazy-missing.js\").catch(() => {});\nthrow new Error(\"inserted module threw after a lazy 404\");\n";
                const len = @min(body.len, out_buf.len);
                @memcpy(out_buf[0..len], body[0..len]);
                return out_buf[0..len];
            }
            if (std.mem.eql(u8, file, "syntax.js")) {
                const body = "globalThis.__insertedModule = [\"unreachable\"];\nexport const = ;\n";
                const len = @min(body.len, out_buf.len);
                @memcpy(out_buf[0..len], body[0..len]);
                return out_buf[0..len];
            }
            if (std.mem.eql(u8, file, "broken.js")) {
                const body = "import \"./missing.js\";\nglobalThis.__insertedModule = (globalThis.__insertedModule || []).concat([\"broken\"]);\n";
                const len = @min(body.len, out_buf.len);
                @memcpy(out_buf[0..len], body[0..len]);
                return out_buf[0..len];
            }
            if (std.mem.eql(u8, file, "big.js")) {
                const body = "/*__KS_OVER_BUDGET__*/\nglobalThis.__insertedModule = [\"big\"];\n";
                const len = @min(body.len, out_buf.len);
                @memcpy(out_buf[0..len], body[0..len]);
                return out_buf[0..len];
            }
            return null;
        }

        // 2.14. /written-script.js
        if (std.mem.indexOf(u8, url, "/written-script.js") != null) {
            let tag: []const u8 = "";
            let tag_buf: [128]u8 = undefined;
            if (raw_tag := MockFixtures.extractQueryParam(url, "tag")) {
                tag = MockFixtures.decodeUri(raw_tag, &tag_buf);
            }
            const res = std.fmt.bufPrint(
                out_buf,
                "globalThis.__written = (globalThis.__written || []).concat([\"{s}\"]);\n",
                .{ tag },
            ) catch return null;
            return res;
        }

        // 2.15. /no-acao-secret.json
        if (std.mem.indexOf(u8, url, "/no-acao-secret.json") != null) {
            const body = "{\"secret\":\"kitesurf-cross-origin-secret\",\"cookie\":\"\",\"origin\":\"\",\"headers\":\"\"}";
            const len = @min(body.len, out_buf.len);
            @memcpy(out_buf[0..len], body[0..len]);
            return out_buf[0..len];
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

struct FramePolicyHitSlot {
    marker_buf: [64]u8 = [0; 64],
    marker_len: usize = 0,
    hits: u32 = 0,
}

const INSERTED_BAKE_JS: []const u8 =
    "\"use strict\";\n" ++
    "window.__insertedStrict = [this != null, (function () { return this; })() === undefined];\n" ++
    "var insertedHello = function () { return \"hi\"; };\n" ++
    "window.__insertedRuns = (window.__insertedRuns || 0) + 1;\n" ++
    "window.__insertedNative = typeof Error.captureStackTrace === \"function\";\n" ++
    "window.__insertedCurrentSrc = document.currentScript ? document.currentScript.getAttribute(\"src\") : null;\n" ++
    "// padding to cross the engine's minimum learned-script size. padding to cross the engine's minimum learned-script size.\n";

const HOSTILE_PROBE_JS: []const u8 =
    "window.__insertedNative = typeof Error.captureStackTrace === \"function\";\n" ++
    "window.__insertedRuns = 1;\n" ++
    "window.__hostileResult = \"pending\";\n" ++
    "import(\"file:///bundle/internal-state.js\").then(function (ns) {\n" ++
    "  window.__hostileResult = typeof ns.getInternalState;\n" ++
    "}, function () { window.__hostileResult = \"refused\"; });\n";

struct ProbeHeaderRecord {
    present: bool = false,
    val_buf: [64]u8 = [0; 64],
    val_len: usize = 0,
}

struct InsertedProbeSlot {
    key_buf: [64]u8 = [0; 64],
    key_len: usize = 0,
    hits: u32 = 0,
    headers: [16]ProbeHeaderRecord = [ProbeHeaderRecord{}; 16],
    header_count: usize = 0,
}

let g_inserted_probes: [64]InsertedProbeSlot = [InsertedProbeSlot{}; 64];
let g_inserted_probe_count: usize = 0;

fn extractExtraHeaderIgnoreCase(extra_json: []const u8, header_name: []const u8, out_buf: []u8): ([]const u8)? {
    if (extra_json.len < 2) return null;
    let i: usize = 0;
    while (i < extra_json.len) : (i += 1) {
        if (extra_json[i] != '"') continue;
        const ks = i + 1;
        let ke = ks;
        while (ke < extra_json.len && extra_json[ke] != '"') : (ke += 1) {
            if (extra_json[ke] == '\\' && ke + 1 < extra_json.len) ke += 1;
        }
        if (ke >= extra_json.len) break;
        const key_slice = extra_json[ks..ke];
        let p = ke + 1;
        while (p < extra_json.len && (extra_json[p] == ' ' || extra_json[p] == '\t' || extra_json[p] == '\r' || extra_json[p] == '\n')) : (p += 1) {}
        if (p < extra_json.len && extra_json[p] == ':') {
            p += 1;
            while (p < extra_json.len && (extra_json[p] == ' ' || extra_json[p] == '\t' || extra_json[p] == '\r' || extra_json[p] == '\n')) : (p += 1) {}
            if (p < extra_json.len && extra_json[p] == '"') {
                const vs = p + 1;
                let ve = vs;
                while (ve < extra_json.len && extra_json[ve] != '"') : (ve += 1) {
                    if (extra_json[ve] == '\\' && ve + 1 < extra_json.len) ve += 1;
                }
                if (ve < extra_json.len) {
                    if (std.ascii.eqlIgnoreCase(key_slice, header_name)) {
                        const raw_v = extra_json[vs..ve];
                        const n = @min(raw_v.len, out_buf.len);
                        @memcpy(out_buf[0..n], raw_v[0..n]);
                        return out_buf[0..n];
                    }
                    i = ve;
                    continue;
                }
            }
        }
        i = ke;
    }
    return null;
}

fn recordInsertedProbeHit(key: []const u8, secret_opt: ([]const u8)?): u32 {
    let idx: (usize)? = null;
    let i: usize = 0;
    while (i < g_inserted_probe_count) : (i += 1) {
        if (std.mem.eql(u8, g_inserted_probes[i].key_buf[0..g_inserted_probes[i].key_len], key)) {
            idx = i;
            break;
        }
    }
    if (idx == null) {
        if (g_inserted_probe_count >= g_inserted_probes.len) {
            // Reuse slot 0 if full
            idx = 0;
            g_inserted_probes[0] = InsertedProbeSlot{};
        } else {
            idx = g_inserted_probe_count;
            g_inserted_probe_count += 1;
        }
        const n = @min(key.len, g_inserted_probes[idx.?].key_buf.len);
        @memcpy(g_inserted_probes[idx.?].key_buf[0..n], key[0..n]);
        g_inserted_probes[idx.?].key_len = n;
    }
    let slot = &g_inserted_probes[idx.?];
    slot.hits += 1;
    if (slot.header_count < slot.headers.len) {
        let rec = &slot.headers[slot.header_count];
        if (sv := secret_opt) {
            rec.present = true;
            const vn = @min(sv.len, rec.val_buf.len);
            @memcpy(rec.val_buf[0..vn], sv[0..vn]);
            rec.val_len = vn;
        } else {
            rec.present = false;
            rec.val_len = 0;
        }
        slot.header_count += 1;
    }
    return slot.hits;
}

fn getInsertedProbeHits(key: []const u8): u32 {
    let i: usize = 0;
    while (i < g_inserted_probe_count) : (i += 1) {
        if (std.mem.eql(u8, g_inserted_probes[i].key_buf[0..g_inserted_probes[i].key_len], key)) {
            return g_inserted_probes[i].hits;
        }
    }
    return 0;
}

fn formatInsertedProbeHeadersJson(key: []const u8, out_buf: []u8): ([]const u8)? {
    let pos: usize = 0;
    const pre = "{\"headers\":[";
    if (pre.len > out_buf.len) return null;
    @memcpy(out_buf[0..pre.len], pre);
    pos += pre.len;

    let i: usize = 0;
    while (i < g_inserted_probe_count) : (i += 1) {
        if (std.mem.eql(u8, g_inserted_probes[i].key_buf[0..g_inserted_probes[i].key_len], key)) {
            const slot = &g_inserted_probes[i];
            let j: usize = 0;
            while (j < slot.header_count) : (j += 1) {
                if (j > 0) {
                    if (pos >= out_buf.len) break;
                    out_buf[pos] = ',';
                    pos += 1;
                }
                const rec = &slot.headers[j];
                if (rec.present) {
                    const part = std.fmt.bufPrint(out_buf[pos..], "\"{s}\"", .{ rec.val_buf[0..rec.val_len] }) catch break;
                    pos += part.len;
                } else {
                    const part = std.fmt.bufPrint(out_buf[pos..], "null", .{}) catch break;
                    pos += part.len;
                }
            }
            break;
        }
    }

    if (pos + 2 > out_buf.len) return null;
    out_buf[pos] = ']';
    out_buf[pos + 1] = '}';
    pos += 2;
    return out_buf[0..pos];
}

let g_frame_policy_hits: [64]FramePolicyHitSlot = [FramePolicyHitSlot{}; 64];
let g_frame_policy_hit_count: usize = 0;

fn incrementFramePolicyHits(marker: []const u8) {
    let i: usize = 0;
    while (i < g_frame_policy_hit_count) : (i += 1) {
        let slot = &g_frame_policy_hits[i];
        if (std.mem.eql(u8, slot.marker_buf[0..slot.marker_len], marker)) {
            slot.hits += 1;
            return;
        }
    }
    if (g_frame_policy_hit_count < g_frame_policy_hits.len) {
        let slot = &g_frame_policy_hits[g_frame_policy_hit_count];
        const n = @min(marker.len, slot.marker_buf.len);
        @memcpy(slot.marker_buf[0..n], marker[0..n]);
        slot.marker_len = n;
        slot.hits = 1;
        g_frame_policy_hit_count += 1;
    }
}

fn getFramePolicyHits(marker: []const u8): u32 {
    let i: usize = 0;
    while (i < g_frame_policy_hit_count) : (i += 1) {
        const slot = &g_frame_policy_hits[i];
        if (std.mem.eql(u8, slot.marker_buf[0..slot.marker_len], marker)) {
            return slot.hits;
        }
    }
    return 0;
}

fn escapeHtmlAttr(src: []const u8, dst: []u8): []const u8 {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const c = src[i];
        if (c == '"') {
            if (pos + 6 > dst.len) break;
            @memcpy(dst[pos .. pos + 6], "&quot;");
            pos += 6;
        } else if (c == '<') {
            if (pos + 4 > dst.len) break;
            @memcpy(dst[pos .. pos + 4], "&lt;");
            pos += 4;
        } else if (c == '>') {
            if (pos + 4 > dst.len) break;
            @memcpy(dst[pos .. pos + 4], "&gt;");
            pos += 4;
        } else {
            if (pos + 1 > dst.len) break;
            dst[pos] = c;
            pos += 1;
        }
    }
    return dst[0..pos];
}

fn extractOriginSlice(url: []const u8): []const u8 {
    const scheme_pos = std.mem.indexOf(u8, url, "://");
    if (scheme_pos == null) return "";
    const host_start = scheme_pos.? + 3;
    const slash_pos = std.mem.indexOfScalarPos(u8, url, host_start, '/');
    if (slash_pos != null) return url[0..slash_pos.?];
    const q_pos = std.mem.indexOfScalarPos(u8, url, host_start, '?');
    if (q_pos != null) return url[0..q_pos.?];
    return url;
}

fn trimAsciiWs(s: []const u8): []const u8 {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn isFrameEmbeddingAllowed(
    child_url: []const u8,
    ancestor_origin: []const u8,
    csp_opt: ([]const u8)?,
    xfo_opt: ([]const u8)?,
): bool {
    const child_origin = extractOriginSlice(child_url);
    const same_origin = (child_origin.len > 0 && ancestor_origin.len > 0 && std.ascii.eqlIgnoreCase(child_origin, ancestor_origin));

    // 1. CSP frame-ancestors takes precedence over X-Frame-Options
    if (csp := csp_opt) {
        let dir_it = std.mem.splitScalar(u8, csp, ';');
        while (raw_dir := dir_it.next()) {
            const dir = trimAsciiWs(raw_dir);
            if (dir.len < 15) continue;
            if (std.ascii.eqlIgnoreCase(dir[0..15], "frame-ancestors") &&
                (dir.len == 15 || dir[15] == ' ' || dir[15] == '\t')) {
                const rest = if (dir.len > 15) trimAsciiWs(dir[15..]) else "";
                if (rest.len == 0) return false;
                let tok_it = std.mem.tokenizeAny(u8, rest, " \t\r\n");
                let matched = false;
                while (tok := tok_it.next()) {
                    if (std.ascii.eqlIgnoreCase(tok, "'none'")) {
                        return false;
                    } else if (std.mem.eql(u8, tok, "*")) {
                        if (std.mem.startsWith(u8, ancestor_origin, "http://") || std.mem.startsWith(u8, ancestor_origin, "https://")) {
                            matched = true;
                        }
                    } else if (std.ascii.eqlIgnoreCase(tok, "'self'")) {
                        if (same_origin) matched = true;
                    } else if (std.ascii.eqlIgnoreCase(tok, ancestor_origin)) {
                        matched = true;
                    } else if (tok.len > 1 && tok[tok.len - 1] == ':') {
                        if (ancestor_origin.len >= tok.len && std.ascii.eqlIgnoreCase(ancestor_origin[0..tok.len], tok)) {
                            matched = true;
                        }
                    }
                }
                return matched;
            }
        }
    }

    // 2. X-Frame-Options (HTML Living Standard)
    if (xfo := xfo_opt) {
        let part_it = std.mem.splitScalar(u8, xfo, ',');
        let has_any_non_empty = false;
        let has_empty = false;
        let seen_deny = false;
        let seen_sameorigin = false;
        let seen_other = false;
        while (raw_part := part_it.next()) {
            const part = trimAsciiWs(raw_part);
            if (part.len == 0) {
                has_empty = true;
                continue;
            }
            has_any_non_empty = true;
            if (std.ascii.eqlIgnoreCase(part, "DENY")) {
                seen_deny = true;
            } else if (std.ascii.eqlIgnoreCase(part, "SAMEORIGIN")) {
                seen_sameorigin = true;
            } else {
                seen_other = true;
            }
        }
        if (!has_any_non_empty) return true;
        if (seen_deny) return false;
        if (seen_sameorigin) {
            // Trailing/empty comma or conflicting tokens invalidate SAMEORIGIN -> DENY
            if (has_empty || seen_other) return false;
            return same_origin;
        }
        // Obsolete or unrecognized directives (e.g. ALLOW-FROM) are ignored
        return true;
    }

    return true;
}
