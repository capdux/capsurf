// src/net/mock_fixtures.cp
// CapSurf Internal Test Fixtures Handler for Mock Network Endpoints
// Aligned with vendor/kitesurf/packages/engine/test/fixtures-worker/src/serve.ts

import { open, read, close, lseek } from "../cdp/server.cp";

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
        } else if (std.ascii.endsWithIgnoreCase(clean, ".js") || std.ascii.eqlIgnoreCase(clean, "js")) {
            return "application/javascript";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".css") || std.ascii.eqlIgnoreCase(clean, "css")) {
            return "text/css";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".png") || std.ascii.eqlIgnoreCase(clean, "png")) {
            return "image/png";
        } else if (std.ascii.endsWithIgnoreCase(clean, ".json") || std.ascii.eqlIgnoreCase(clean, "json")) {
            return "application/json";
        }
        return "application/octet-stream";
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

        // 1. Try relative path: e2e/page/<filename>
        let path_buf: [512]u8 = [0; 512];
        const rel_path = std.fmt.bufPrint(&path_buf, "e2e/page/{s}", .{ filename }) catch return null;
        if (res1 := MockFixtures.readFileContent(rel_path, out_buf)) {
            return res1;
        }

        // 2. Try absolute path: /data/dev/capsurf/e2e/page/<filename>
        let abs_buf: [512]u8 = [0; 512];
        const abs_path = std.fmt.bufPrint(&abs_buf, "/data/dev/capsurf/e2e/page/{s}", .{ filename }) catch return null;
        if (res2 := MockFixtures.readFileContent(abs_path, out_buf)) {
            return res2;
        }

        // 3. Fallback to basename if filename contains '/'
        if (last_slash := std.mem.lastIndexOfScalar(u8, filename, '/')) {
            const base = filename[last_slash + 1..];
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
    ): ([]const u8)? {
        // 1. /echo-headers.json
        if (std.mem.indexOf(u8, url, "/echo-headers.json") != null) {
            const ua = if (user_agent.len == 0) "CapSurf/1.0.0" else user_agent;
            const orig = if (origin.len == 0) "null" else origin;

            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"accept\":\"*/*\",\"origin\":\"{s}\",\"user-agent\":\"{s}\"}}",
                .{ orig, ua },
            ) catch return null;

            return res;
        }

        // 1.5. /echo-headers.html
        if (std.mem.indexOf(u8, url, "/echo-headers.html") != null && std.mem.indexOf(u8, url, ".invalid") == null) {
            const ua = if (user_agent.len == 0) "CapSurf/1.0.0" else user_agent;
            let esc_ua_buf: [1024]u8 = undefined;
            const esc_ua = MockFixtures.escapeJson(ua, &esc_ua_buf);
            const res = std.fmt.bufPrint(
                out_buf,
                "<!doctype html><html><head><meta charset=\"utf-8\"><title>echo-headers</title></head><body><script>globalThis.__ks_doc_headers = {{\"user-agent\":\"{s}\",\"accept\":\"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\"}};globalThis.__ks_fetch_headers = null;globalThis.__ks_fetch_ready = fetch(\"/__fixtures/echo-headers.json\").then(function(r){{return r.json();}}).then(function(d){{globalThis.__ks_fetch_headers = d;return d;}});</script></body></html>",
                .{ esc_ua },
            ) catch return null;

            return res;
        }

        // 2. Dynamic endpoints
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
): ([]const u8)? {
    return MockFixtures.handleFixtureRequest(url, origin, user_agent, out_buf);
}
