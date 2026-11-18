// src/cdp/network.cp
// CDP Network & Storage Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/network.ts and cookies.ts

import { CdpRequest, CdpResponse, extractJsonString, extractJsonInt, extractJsonBool } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { CookieEntry, entryToCdp } from "../net/cookies.cp";

fn sanitizeAcceptEncoding(allocator: std.mem.Allocator, raw: []const u8): []const u8 {
    const search_key = "accept-encoding";
    let found_idx: usize? = null;
    const key_len = search_key.len;

    let i: usize = 0;
    while (i + key_len <= raw.len) : (i += 1) {
        let is_match = true;
        let j: usize = 0;
        while (j < key_len) : (j += 1) {
            if (std.ascii.toLower(raw[i + j]) != search_key[j]) {
                is_match = false;
                break;
            }
        }
        if (is_match) {
            found_idx = i;
            break;
        }
    }

    if (found_idx == null) {
        return allocator.dupe(u8, raw) catch raw;
    }

    let cursor = found_idx.? + key_len;
    while (cursor < raw.len && (raw[cursor] == '"' || raw[cursor] == ':' || raw[cursor] == ' ' || raw[cursor] == '\t')) : (cursor += 1) {}
    if (cursor >= raw.len) return allocator.dupe(u8, raw) catch raw;

    let val_start: usize = cursor;
    let val_end: usize = cursor;
    if (cursor > 0 && raw[cursor - 1] == '"') {
        while (val_end < raw.len && raw[val_end] != '"') : (val_end += 1) {}
    } else if (raw[cursor] == '"') {
        cursor += 1;
        val_start = cursor;
        val_end = cursor;
        while (val_end < raw.len && raw[val_end] != '"') : (val_end += 1) {}
    } else {
        while (val_end < raw.len && raw[val_end] != '\r' && raw[val_end] != '\n' && raw[val_end] != '}') : (val_end += 1) {}
    }

    const val = raw[val_start..val_end];
    let needs_sanitize = false;
    if (std.mem.indexOf(u8, val, "deflate") != null || std.mem.indexOf(u8, val, "zstd") != null) {
        needs_sanitize = true;
    }

    if (!needs_sanitize) {
        return allocator.dupe(u8, raw) catch raw;
    }

    const repl = "gzip, br";
    const new_len = raw.len - val.len + repl.len;
    let buf = allocator.alloc(u8, new_len) catch return allocator.dupe(u8, raw) catch raw;
    @memcpy(buf[0..val_start], raw[0..val_start]);
    @memcpy(buf[val_start .. val_start + repl.len], repl);
    @memcpy(buf[val_start + repl.len .. new_len], raw[val_end..]);
    return buf;
}

pub struct NetworkHandler {
    static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8): CdpResponse {
        // 1. Network lifecycle & buffering
        if (std.mem.eql(u8, req.method, "Network.enable")) {
            const max_buf = extractJsonInt(req.params_raw, "maxTotalBufferSize");
            * = max_buf;
            session.network_enabled = true;
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Network.disable")) {
            session.network_enabled = false;
            return CdpResponse.ok(req.id, "{}");
        }

        // 2. Cache & ServiceWorker controls
        if (std.mem.eql(u8, req.method, "Network.setCacheDisabled") ||
            std.mem.eql(u8, req.method, "Network.setBypassServiceWorker") ||
            std.mem.eql(u8, req.method, "Network.clearBrowserCache")) {
            return CdpResponse.ok(req.id, "{}");
        }

        // 3. UserAgent & Extra HTTP Headers
        if (std.mem.eql(u8, req.method, "Network.setUserAgentOverride")) {
            const ua = extractJsonString(req.params_raw, "userAgent") ?? "";
            session.setUserAgentOverride(ua);
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Network.setExtraHTTPHeaders")) {
            // Save extra headers JSON for subsequent fetch and navigation pipelines
            const headers_str = extractJsonString(req.params_raw, "headers") ?? "";
            let raw_to_save: []const u8 = req.params_raw;
            if (headers_str.len > 0) {
                raw_to_save = headers_str;
            }
            session.extra_headers = sanitizeAcceptEncoding(session.allocator, raw_to_save);
            return CdpResponse.ok(req.id, "{}");
        }

        // 4. Cookie retrieval: real integration with session.cookie_jar
        if (std.mem.eql(u8, req.method, "Network.getCookies") ||
            std.mem.eql(u8, req.method, "Network.getAllCookies") ||
            std.mem.eql(u8, req.method, "Storage.getCookies")) {
            const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (bctx.len > 0 && !session.target_registry.hasBrowserContext(bctx)) {
                return CdpResponse.err(req.id, -32000, "Failed to find browser context");
            }
            const jar = session.getCookieJarForContext(bctx) ?? session.getActiveCookieJar();
            const cookies = jar.getAll();
            if (cookies.len == 0) {
                return CdpResponse.ok(req.id, "{\"cookies\":[]}");
            }

            let cursor: usize = 0;
            const prefix = "{\"cookies\":[";
            @memcpy(out_buf[cursor .. cursor + prefix.len], prefix);
            cursor += prefix.len;

            let first: bool = true;
            let i: usize = 0;
            while (i < cookies.len) : (i += 1) {
                const cdp_ck = entryToCdp(&cookies[i]);
                if (!first) {
                    if (cursor < out_buf.len) {
                        out_buf[cursor] = ',';
                        cursor += 1;
                    }
                }
                first = false;
                const chunk = std.fmt.bufPrint(
                    out_buf[cursor..],
                    "{{\"name\":\"{s}\",\"value\":\"{s}\",\"domain\":\"{s}\",\"path\":\"{s}\",\"size\":{d},\"httpOnly\":{s},\"secure\":{s},\"session\":{s},\"sameSite\":\"{s}\"}}",
                    .{
                        cdp_ck.name,
                        cdp_ck.value,
                        cdp_ck.domain,
                        cdp_ck.path,
                        cdp_ck.size,
                        if (cdp_ck.http_only) "true" else "false",
                        if (cdp_ck.secure) "true" else "false",
                        if (cdp_ck.session) "true" else "false",
                        cdp_ck.same_site,
                    },
                ) catch break;
                cursor += chunk.len;
            }
            if (cursor + 2 <= out_buf.len) {
                out_buf[cursor] = ']';
                out_buf[cursor + 1] = '}';
                cursor += 2;
            }
            return CdpResponse.ok(req.id, out_buf[0..cursor]);
        }

        // 5. Cookie mutation: real storage into session.cookie_jar
        if (std.mem.eql(u8, req.method, "Network.setCookie")) {
            const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (bctx.len > 0 && !session.target_registry.hasBrowserContext(bctx)) {
                return CdpResponse.err(req.id, -32000, "Failed to find browser context");
            }
            const jar = session.getCookieJarForContext(bctx) ?? session.getActiveCookieJar();

            const name = extractJsonString(req.params_raw, "name") ?? "";
            const val = extractJsonString(req.params_raw, "value") ?? "";
            const domain = extractJsonString(req.params_raw, "domain") ?? "";
            const path = extractJsonString(req.params_raw, "path") ?? "";

            if (name.len > 0) {
                const d = if (domain.len > 0) domain else "localhost";
                const p = if (path.len > 0) path else "/";
                const set_cookie_str = std.fmt.allocPrint(
                    session.allocator,
                    "{s}={s}; Domain={s}; Path={s}",
                    .{ name, val, d, p },
                ) catch "";

                if (set_cookie_str.len > 0) {
                    let url_buf: [128]u8 = undefined;
                    const url_str = std.fmt.bufPrint(url_buf[0..], "http://{s}{s}", .{ d, p }) catch "http://localhost/";
                    * = jar.setFromHeader(url_str, set_cookie_str, null);
                }
            }
            return CdpResponse.ok(req.id, "{\"success\":true}");
        }

        if (std.mem.eql(u8, req.method, "Network.setCookies") ||
            std.mem.eql(u8, req.method, "Storage.setCookies")) {
            const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (bctx.len > 0 && !session.target_registry.hasBrowserContext(bctx)) {
                return CdpResponse.err(req.id, -32000, "Failed to find browser context");
            }
            const jar = session.getCookieJarForContext(bctx) ?? session.getActiveCookieJar();

            const cookies_key = "\"cookies\"";
            if (cookies_pos := std.mem.indexOf(u8, req.params_raw, cookies_key)) {
                let pos = cookies_pos + cookies_key.len;
                while (pos < req.params_raw.len && req.params_raw[pos] != '[') : (pos += 1) {}
                if (pos < req.params_raw.len && req.params_raw[pos] == '[') {
                    pos += 1;
                    while (pos < req.params_raw.len) {
                        while (pos < req.params_raw.len && (req.params_raw[pos] == ' ' || req.params_raw[pos] == ',' || req.params_raw[pos] == '\t' || req.params_raw[pos] == '\r' || req.params_raw[pos] == '\n')) : (pos += 1) {}
                        if (pos >= req.params_raw.len || req.params_raw[pos] == ']') break;
                        if (req.params_raw[pos] == '{') {
                            const obj_start = pos;
                            let depth: i32 = 0;
                            let in_str = false;
                            let escaped = false;
                            while (pos < req.params_raw.len) : (pos += 1) {
                                const ch = req.params_raw[pos];
                                if (escaped) {
                                    escaped = false;
                                    continue;
                                }
                                if (ch == '\\' && in_str) {
                                    escaped = true;
                                    continue;
                                }
                                if (ch == '"') {
                                    in_str = !in_str;
                                    continue;
                                }
                                if (!in_str) {
                                    if (ch == '{') depth += 1;
                                    if (ch == '}') {
                                        depth -= 1;
                                        if (depth == 0) {
                                            pos += 1;
                                            break;
                                        }
                                    }
                                }
                            }
                            const obj_str = req.params_raw[obj_start..pos];
                            const name = extractJsonString(obj_str, "name") ?? "";
                            const val = extractJsonString(obj_str, "value") ?? "";
                            const domain = extractJsonString(obj_str, "domain") ?? "";
                            const path = extractJsonString(obj_str, "path") ?? "";
                            const is_secure = extractJsonBool(obj_str, "secure") ?? false;
                            const same_site = extractJsonString(obj_str, "sameSite") ?? "";
                            const is_http_only = extractJsonBool(obj_str, "httpOnly") ?? false;

                            if (name.len > 0) {
                                let d = if (domain.len > 0) domain else "localhost";
                                if (d.len > 0 && d[0] == '.') d = d[1..];
                                const p = if (path.len > 0) path else "/";
                                const scheme = if (is_secure) "https" else "http";
                                const url_str = std.fmt.allocPrint(
                                    session.allocator,
                                    "{s}://{s}{s}",
                                    .{ scheme, d, p },
                                ) catch "";

                                let ss_buf: [32]u8 = [0; 32];
                                let ss_str: []const u8 = "";
                                if (same_site.len > 0) {
                                    ss_str = std.fmt.bufPrint(&ss_buf, "; SameSite={s}", .{ same_site }) catch "";
                                }
                                const sec_str = if (is_secure) "; Secure" else "";
                                const ho_str = if (is_http_only) "; HttpOnly" else "";

                                const set_cookie_str = std.fmt.allocPrint(
                                    session.allocator,
                                    "{s}={s}; Domain={s}; Path={s}{s}{s}{s}",
                                    .{ name, val, d, p, sec_str, ho_str, ss_str },
                                ) catch "";

                                if (url_str.len > 0 && set_cookie_str.len > 0) {
                                    * = jar.setFromHeader(url_str, set_cookie_str, null);
                                }
                            }
                        } else {
                            pos += 1;
                        }
                    }
                }
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Network.deleteCookies")) {
            const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (bctx.len > 0 && !session.target_registry.hasBrowserContext(bctx)) {
                return CdpResponse.err(req.id, -32000, "Failed to find browser context");
            }
            const jar = session.getCookieJarForContext(bctx) ?? session.getActiveCookieJar();
            const name = extractJsonString(req.params_raw, "name") ?? "";
            if (name.len > 0) {
                const url = extractJsonString(req.params_raw, "url") ?? "";
                const domain = extractJsonString(req.params_raw, "domain") ?? "";
                const path = extractJsonString(req.params_raw, "path") ?? "";
                * = jar.remove(name, url, domain, path);
            } else {
                jar.clear();
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Network.clearBrowserCookies") ||
            std.mem.eql(u8, req.method, "Storage.clearCookies")) {
            const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (bctx.len > 0 && !session.target_registry.hasBrowserContext(bctx)) {
                return CdpResponse.err(req.id, -32000, "Failed to find browser context");
            }
            const jar = session.getCookieJarForContext(bctx) ?? session.getActiveCookieJar();
            jar.clear();
            return CdpResponse.ok(req.id, "{}");
        }

        // 6. Network emulation & response body stubs
        if (std.mem.eql(u8, req.method, "Network.emulateNetworkConditionsByRule")) {
            return CdpResponse.ok(req.id, "{\"ruleIds\":[]}");
        }

        if (std.mem.eql(u8, req.method, "Network.getResponseBody")) {
            return CdpResponse.ok(req.id, "{\"body\":\"\",\"base64Encoded\":false}");
        }

        // 7. Generic fallback for any other Network or Storage domain methods
        if (std.mem.startsWith(u8, req.method, "Network.") ||
            std.mem.startsWith(u8, req.method, "Storage.")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.err(req.id, -32601, "'Network' domain method not implemented");
    }
}
