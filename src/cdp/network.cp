// src/cdp/network.cp
// CDP Network & Storage Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/network.ts and cookies.ts

import { CdpRequest, CdpResponse, extractJsonString, extractJsonInt, extractJsonBool } from "./types.cp";
import { CdpPageSession, escapeJsonString } from "./session.cp";
import { CookieEntry, entryToCdp, parseUrlParts } from "../net/cookies.cp";
import { markFreshExtraHeaders } from "../net/http_loader.cp";

pub struct StoredResponseBody {
    request_id_buf: [64]u8 = [0; 64],
    request_id_len: usize = 0,
    body: []u8 = [],
    base64_encoded: bool = false,
    used: bool = false,
}

const MAX_STORED_BODIES: usize = 64;
let g_stored_bodies: [MAX_STORED_BODIES]StoredResponseBody = [StoredResponseBody{}; MAX_STORED_BODIES];
let g_stored_body_cursor: usize = 0;
let g_doc_req_seq: u64 = 0;

pub fn nextDocRequestId(out_buf: []u8): []const u8 {
    g_doc_req_seq += 1;
    return std.fmt.bufPrint(out_buf, "doc-req-{d}", .{g_doc_req_seq}) catch "doc-req-1";
}

pub fn clearDocResponseBodies() {
    let k: usize = 0;
    while (k < MAX_STORED_BODIES) : (k += 1) {
        if (g_stored_bodies[k].used) {
            const rid = g_stored_bodies[k].request_id_buf[0..g_stored_bodies[k].request_id_len];
            if (std.mem.startsWith(u8, rid, "doc-req-") || std.mem.startsWith(u8, rid, "capsurf-loader-")) {
                if (g_stored_bodies[k].body.len > 0) {
                    std.heap.c_allocator.free(g_stored_bodies[k].body);
                }
                g_stored_bodies[k].body = [];
                g_stored_bodies[k].used = false;
            }
        }
    }
}

pub fn recordResponseBody(request_id: []const u8, body: []const u8, base64_encoded: bool) {
    if (request_id.len == 0) return;
    let slot: *StoredResponseBody = &g_stored_bodies[0];
    let found_existing = false;
    let k: usize = 0;
    while (k < MAX_STORED_BODIES) : (k += 1) {
        if (g_stored_bodies[k].used && std.mem.eql(u8, g_stored_bodies[k].request_id_buf[0..g_stored_bodies[k].request_id_len], request_id)) {
            slot = &g_stored_bodies[k];
            found_existing = true;
            break;
        }
    }
    if (!found_existing) {
        slot = &g_stored_bodies[g_stored_body_cursor % MAX_STORED_BODIES];
        g_stored_body_cursor += 1;
    }
    if (slot.used && slot.body.len > 0) {
        std.heap.c_allocator.free(slot.body);
        slot.body = [];
    }
    const id_len = @min(request_id.len, slot.request_id_buf.len);
    @memcpy(slot.request_id_buf[0..id_len], request_id[0..id_len]);
    slot.request_id_len = id_len;
    slot.base64_encoded = base64_encoded;
    if (body.len > 0) {
        const dup = std.heap.c_allocator.alloc(u8, body.len) catch {
            slot.body = [];
            slot.used = true;
            return;
        };
        @memcpy(dup, body);
        slot.body = dup;
    } else {
        slot.body = [];
    }
    slot.used = true;
}

pub fn findResponseBody(request_id: []const u8): (*const StoredResponseBody)? {
    if (request_id.len == 0) return null;
    let i: usize = 0;
    while (i < MAX_STORED_BODIES) : (i += 1) {
        const slot = &g_stored_bodies[i];
        if (slot.used && std.mem.eql(u8, slot.request_id_buf[0..slot.request_id_len], request_id)) {
            return slot;
        }
    }
    return null;
}

pub struct HttpHeaderJoinEntry {
    key: []const u8 = "",
    val: std.ArrayListUnmanaged(u8) = std.ArrayListUnmanaged(u8).empty,
}

pub fn formatHttpHeadersJsonWithSep(raw_headers: []const u8, allocator: std.mem.Allocator, default_sep: []const u8): []const u8 {
    if (raw_headers.len == 0) return "{}";
    let entries = std.ArrayListUnmanaged(HttpHeaderJoinEntry).empty;
    let it = std.mem.splitSequence(u8, raw_headers, "\r\n");
    while (line := it.next()) {
        if (line.len == 0) continue;
        if (colon := std.mem.indexOfScalar(u8, line, ':')) {
            const k_raw = std.mem.trim(u8, line[0..colon], " \t");
            const v_raw = std.mem.trim(u8, line[colon + 1 ..], " \t");
            if (k_raw.len == 0) continue;
            let matched = false;
            let ei: usize = 0;
            while (ei < entries.items.len) : (ei += 1) {
                if (std.ascii.eqlIgnoreCase(entries.items[ei].key, k_raw)) {
                    const sep = if (std.ascii.eqlIgnoreCase(k_raw, "set-cookie")) "\n" else default_sep;
                    entries.items[ei].val.appendSlice(allocator, sep) catch break;
                    entries.items[ei].val.appendSlice(allocator, v_raw) catch break;
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                let entry = HttpHeaderJoinEntry{ key: k_raw };
                entry.val.appendSlice(allocator, v_raw) catch continue;
                entries.append(allocator, entry) catch continue;
            }
        }
    }
    if (entries.items.len == 0) return "{}";
    let list = std.ArrayListUnmanaged(u8).empty;
    list.append(allocator, '{') catch return "{}";
    let i: usize = 0;
    while (i < entries.items.len) : (i += 1) {
        let k_esc_buf: [256]u8 = [0; 256];
        let v_esc_buf: [4096]u8 = [0; 4096];
        const k_esc = escapeJsonString(entries.items[i].key, &k_esc_buf);
        const v_esc = escapeJsonString(entries.items[i].val.items, &v_esc_buf);
        if (i > 0) list.append(allocator, ',') catch break;
        list.append(allocator, '"') catch break;
        list.appendSlice(allocator, k_esc) catch break;
        list.appendSlice(allocator, "\":\"") catch break;
        list.appendSlice(allocator, v_esc) catch break;
        list.append(allocator, '"') catch break;
    }
    list.append(allocator, '}') catch return "{}";
    return list.items;
}

pub fn formatHttpHeadersJson(raw_headers: []const u8, allocator: std.mem.Allocator): []const u8 {
    return formatHttpHeadersJsonWithSep(raw_headers, allocator, ", ");
}

pub fn formatHttpHeadersExtraInfoJson(raw_headers: []const u8, allocator: std.mem.Allocator): []const u8 {
    return formatHttpHeadersJsonWithSep(raw_headers, allocator, "\n");
}

pub struct PendingFetchBody {
    url_buf: [1024]u8 = [0; 1024],
    url_len: usize = 0,
    body: []u8 = [],
    status_code: u32 = 200,
    status_text_buf: [64]u8 = [0; 64],
    status_text_len: usize = 0,
    mime_buf: [128]u8 = [0; 128],
    mime_len: usize = 0,
    raw_headers: []u8 = [],
    has_meta: bool = false,
    active: bool = false,
}

const MAX_PENDING_FETCH_BODIES: usize = 32;
let g_pending_fetch_bodies: [MAX_PENDING_FETCH_BODIES]PendingFetchBody = [PendingFetchBody{}; MAX_PENDING_FETCH_BODIES];
let g_pending_fetch_cursor: usize = 0;

fn writePendingFetchSlot(url: []const u8, body: []const u8, status_code: u32, status_text: []const u8, raw_headers: []const u8, mime_type: []const u8, has_meta: bool) {
    if (url.len == 0) return;
    let slot = &g_pending_fetch_bodies[g_pending_fetch_cursor % MAX_PENDING_FETCH_BODIES];
    g_pending_fetch_cursor += 1;
    if (slot.active && slot.body.len > 0) {
        std.heap.c_allocator.free(slot.body);
        slot.body = [];
    }
    if (slot.active && slot.raw_headers.len > 0) {
        std.heap.c_allocator.free(slot.raw_headers);
        slot.raw_headers = [];
    }
    const u_len = @min(url.len, slot.url_buf.len);
    @memcpy(slot.url_buf[0..u_len], url[0..u_len]);
    slot.url_len = u_len;
    slot.status_code = status_code;
    const st_len = @min(status_text.len, slot.status_text_buf.len);
    if (st_len > 0) @memcpy(slot.status_text_buf[0..st_len], status_text[0..st_len]);
    slot.status_text_len = st_len;
    const m_len = @min(mime_type.len, slot.mime_buf.len);
    if (m_len > 0) @memcpy(slot.mime_buf[0..m_len], mime_type[0..m_len]);
    slot.mime_len = m_len;
    slot.has_meta = has_meta;
    if (body.len > 0) {
        const dup = std.heap.c_allocator.alloc(u8, body.len) catch {
            slot.body = [];
            slot.active = true;
            return;
        };
        @memcpy(dup, body);
        slot.body = dup;
    } else {
        slot.body = [];
    }
    if (raw_headers.len > 0) {
        const hdup = std.heap.c_allocator.alloc(u8, raw_headers.len) catch {
            slot.raw_headers = [];
            slot.active = true;
            return;
        };
        @memcpy(hdup, raw_headers);
        slot.raw_headers = hdup;
    } else {
        slot.raw_headers = [];
    }
    slot.active = true;
}

pub fn recordPendingFetchBody(url: []const u8, body: []const u8) {
    writePendingFetchSlot(url, body, 200, "OK", "", "", false);
}

pub fn recordPendingFetchMeta(url: []const u8, body: []const u8, status_code: u32, status_text: []const u8, raw_headers: []const u8, mime_type: []const u8) {
    writePendingFetchSlot(url, body, status_code, status_text, raw_headers, mime_type, true);
}

pub struct PendingFetchResult {
    found: bool = false,
    body: []const u8 = "",
    status_code: u32 = 200,
    status_text_buf: [64]u8 = [0; 64],
    status_text_len: usize = 0,
    mime_buf: [128]u8 = [0; 128],
    mime_len: usize = 0,
    raw_headers: []const u8 = "",
    has_meta: bool = false,
}

pub fn consumePendingFetchBody(url: []const u8): PendingFetchResult {
    if (url.len == 0) return PendingFetchResult{};
    let i: usize = 0;
    while (i < MAX_PENDING_FETCH_BODIES) : (i += 1) {
        let slot = &g_pending_fetch_bodies[i];
        if (slot.active && std.mem.eql(u8, slot.url_buf[0..slot.url_len], url)) {
            const b = slot.body;
            const h = slot.raw_headers;
            slot.body = [];
            slot.raw_headers = [];
            slot.active = false;
            let res = PendingFetchResult{
                found: true,
                body: b,
                status_code: slot.status_code,
                status_text_len: slot.status_text_len,
                mime_len: slot.mime_len,
                raw_headers: h,
                has_meta: slot.has_meta,
            };
            if (slot.status_text_len > 0) @memcpy(res.status_text_buf[0..slot.status_text_len], slot.status_text_buf[0..slot.status_text_len]);
            if (slot.mime_len > 0) @memcpy(res.mime_buf[0..slot.mime_len], slot.mime_buf[0..slot.mime_len]);
            return res;
        }
    }
    return PendingFetchResult{};
}

fn jsonEscapeBody(src: []const u8, allocator: std.mem.Allocator): ([]u8)! {
    let extra: usize = 0;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const c = src[i];
        if (c == '"' || c == '\\' || c == '\n' || c == '\r' || c == '\t') {
            extra += 1;
        } else if (c < 0x20) {
            extra += 5;
        }
    }
    let out = allocator.alloc(u8, src.len + extra) catch return error.OutOfMemory;
    let pos: usize = 0;
    i = 0;
    while (i < src.len) : (i += 1) {
        const c = src[i];
        if (c == '"') {
            out[pos] = '\\'; out[pos + 1] = '"'; pos += 2;
        } else if (c == '\\') {
            out[pos] = '\\'; out[pos + 1] = '\\'; pos += 2;
        } else if (c == '\n') {
            out[pos] = '\\'; out[pos + 1] = 'n'; pos += 2;
        } else if (c == '\r') {
            out[pos] = '\\'; out[pos + 1] = 'r'; pos += 2;
        } else if (c == '\t') {
            out[pos] = '\\'; out[pos + 1] = 't'; pos += 2;
        } else if (c < 0x20) {
            const hex = "0123456789abcdef";
            out[pos] = '\\';
            out[pos + 1] = 'u';
            out[pos + 2] = '0';
            out[pos + 3] = '0';
            out[pos + 4] = hex[(c >> 4) & 0x0F];
            out[pos + 5] = hex[c & 0x0F];
            pos += 6;
        } else {
            out[pos] = c;
            pos += 1;
        }
    }
    return out[0..pos];
}

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
            if (req.session_id.len > 0 && !std.mem.eql(u8, req.session_id, "capsurf-session-001")) {
                if (session.target_registry.findFrameIdBySessionId(req.session_id) != null) {
                    const slen = @min(req.session_id.len, session.child_network_session_id_buf.len);
                    @memcpy(session.child_network_session_id_buf[0..slen], req.session_id[0..slen]);
                    session.child_network_session_id_len = slen;
                }
            }
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
            // Save extra headers JSON object `{...}` for subsequent fetch and navigation pipelines
            let raw_to_save: []const u8 = "{}";
            if (h_key_pos := std.mem.indexOf(u8, req.params_raw, "\"headers\"")) {
                let p = h_key_pos + 9;
                while (p < req.params_raw.len && (req.params_raw[p] == ' ' || req.params_raw[p] == ':' || req.params_raw[p] == '\t')) : (p += 1) {}
                if (p < req.params_raw.len && req.params_raw[p] == '{') {
                    const obj_start = p;
                    let depth: usize = 0;
                    let in_str = false;
                    let esc = false;
                    while (p < req.params_raw.len) : (p += 1) {
                        const ch = req.params_raw[p];
                        if (in_str) {
                            if (esc) { esc = false; }
                            else if (ch == '\\') { esc = true; }
                            else if (ch == '"') { in_str = false; }
                        } else {
                            if (ch == '"') { in_str = true; }
                            else if (ch == '{') { depth += 1; }
                            else if (ch == '}') {
                                if (depth > 0) depth -= 1;
                                if (depth == 0) {
                                    p += 1;
                                    break;
                                }
                            }
                        }
                    }
                    if (depth == 0 && p > obj_start) {
                        raw_to_save = req.params_raw[obj_start..p];
                    }
                }
            } else {
                const headers_str = extractJsonString(req.params_raw, "headers") ?? "";
                if (headers_str.len > 0) raw_to_save = headers_str;
            }
            session.extra_headers = sanitizeAcceptEncoding(session.allocator, raw_to_save);
            markFreshExtraHeaders();
            if (session.dom_ctx != null) {
                session.dom_ctx.?.extra_headers = session.extra_headers;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        // 4. Cookie retrieval: real integration with session.cookie_jar
        if (std.mem.eql(u8, req.method, "Network.getCookies") ||
            std.mem.eql(u8, req.method, "Network.getAllCookies") ||
            std.mem.eql(u8, req.method, "Storage.getCookies")) {
            const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (bctx.len > 0 && !session.getBrowserTargetRegistry().hasBrowserContext(bctx)) {
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
            if (bctx.len > 0 && !session.getBrowserTargetRegistry().hasBrowserContext(bctx)) {
                return CdpResponse.err(req.id, -32000, "Failed to find browser context");
            }
            const jar = session.getCookieJarForContext(bctx) ?? session.getActiveCookieJar();

            const name = extractJsonString(req.params_raw, "name") ?? "";
            const val = extractJsonString(req.params_raw, "value") ?? "";
            const domain = extractJsonString(req.params_raw, "domain") ?? "";
            const path = extractJsonString(req.params_raw, "path") ?? "";
            const url_param = extractJsonString(req.params_raw, "url") ?? "";
            const is_secure_opt = extractJsonBool(req.params_raw, "secure");
            const same_site = extractJsonString(req.params_raw, "sameSite") ?? "";
            const is_http_only = extractJsonBool(req.params_raw, "httpOnly") ?? false;

            if (name.len > 0) {
                let url_str: []const u8 = "";
                if (url_param.len > 0) {
                    url_str = url_param;
                } else {
                    let d = if (domain.len > 0) domain else "localhost";
                    if (d.len > 0 && d[0] == '.') d = d[1..];
                    const p = if (path.len > 0) path else "/";
                    const is_sec = is_secure_opt ?? false;
                    const scheme = if (is_sec) "https" else "http";
                    url_str = std.fmt.allocPrint(
                        session.allocator,
                        "{s}://{s}{s}",
                        .{ scheme, d, p },
                    ) catch "";
                }

                let d_buf: [256]u8 = [0; 256];
                let d_str: []const u8 = "";
                if (domain.len > 0) {
                    let d = domain;
                    if (d.len > 0 && d[0] == '.') d = d[1..];
                    d_str = std.fmt.bufPrint(&d_buf, "; Domain={s}", .{d}) catch "";
                }

                let p_buf: [256]u8 = [0; 256];
                let p = path;
                if (p.len == 0 && url_param.len > 0) {
                    if (up := parseUrlParts(url_param)) {
                        p = up.path;
                    }
                }
                if (p.len == 0) p = "/";
                const p_str = std.fmt.bufPrint(&p_buf, "; Path={s}", .{p}) catch "; Path=/";

                let ss_buf: [32]u8 = [0; 32];
                let ss_str: []const u8 = "";
                if (same_site.len > 0) {
                    ss_str = std.fmt.bufPrint(&ss_buf, "; SameSite={s}", .{same_site}) catch "";
                }

                let is_sec = is_secure_opt ?? false;
                if (is_secure_opt == null && url_param.len > 0 && std.mem.startsWith(u8, url_param, "https://")) {
                    is_sec = true;
                }
                const sec_str = if (is_sec) "; Secure" else "";
                const ho_str = if (is_http_only) "; HttpOnly" else "";

                const set_cookie_str = std.fmt.allocPrint(
                    session.allocator,
                    "{s}={s}{s}{s}{s}{s}{s}",
                    .{ name, val, d_str, p_str, sec_str, ho_str, ss_str },
                ) catch "";
                defer if (set_cookie_str.len > 0) session.allocator.free(set_cookie_str);

                if (url_str.len > 0 && set_cookie_str.len > 0) {
                    * = jar.setFromHeader(url_str, set_cookie_str, null);
                }
            }
            return CdpResponse.ok(req.id, "{\"success\":true}");
        }

        if (std.mem.eql(u8, req.method, "Network.setCookies") ||
            std.mem.eql(u8, req.method, "Storage.setCookies")) {
            const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (bctx.len > 0 && !session.getBrowserTargetRegistry().hasBrowserContext(bctx)) {
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
                            const url_param = extractJsonString(obj_str, "url") ?? "";
                            const is_secure_opt = extractJsonBool(obj_str, "secure");
                            const same_site = extractJsonString(obj_str, "sameSite") ?? "";
                            const is_http_only = extractJsonBool(obj_str, "httpOnly") ?? false;

                            if (name.len > 0) {
                                let url_str: []const u8 = "";
                                if (url_param.len > 0) {
                                    url_str = url_param;
                                } else {
                                    let d = if (domain.len > 0) domain else "localhost";
                                    if (d.len > 0 && d[0] == '.') d = d[1..];
                                    const p = if (path.len > 0) path else "/";
                                    const is_sec = is_secure_opt ?? false;
                                    const scheme = if (is_sec) "https" else "http";
                                    url_str = std.fmt.allocPrint(
                                        session.allocator,
                                        "{s}://{s}{s}",
                                        .{ scheme, d, p },
                                    ) catch "";
                                }

                                let d_buf: [256]u8 = [0; 256];
                                let d_str: []const u8 = "";
                                if (domain.len > 0) {
                                    let d = domain;
                                    if (d.len > 0 && d[0] == '.') d = d[1..];
                                    d_str = std.fmt.bufPrint(&d_buf, "; Domain={s}", .{d}) catch "";
                                }

                                let p_buf: [256]u8 = [0; 256];
                                let p = path;
                                if (p.len == 0 && url_param.len > 0) {
                                    if (up := parseUrlParts(url_param)) {
                                        p = up.path;
                                    }
                                }
                                if (p.len == 0) p = "/";
                                const p_str = std.fmt.bufPrint(&p_buf, "; Path={s}", .{p}) catch "; Path=/";

                                let ss_buf: [32]u8 = [0; 32];
                                let ss_str: []const u8 = "";
                                if (same_site.len > 0) {
                                    ss_str = std.fmt.bufPrint(&ss_buf, "; SameSite={s}", .{same_site}) catch "";
                                }

                                let is_sec = is_secure_opt ?? false;
                                if (is_secure_opt == null && url_param.len > 0 && std.mem.startsWith(u8, url_param, "https://")) {
                                    is_sec = true;
                                }
                                const sec_str = if (is_sec) "; Secure" else "";
                                const ho_str = if (is_http_only) "; HttpOnly" else "";

                                const set_cookie_str = std.fmt.allocPrint(
                                    session.allocator,
                                    "{s}={s}{s}{s}{s}{s}{s}",
                                    .{ name, val, d_str, p_str, sec_str, ho_str, ss_str },
                                ) catch "";
                                defer if (set_cookie_str.len > 0) session.allocator.free(set_cookie_str);

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
            if (bctx.len > 0 && !session.getBrowserTargetRegistry().hasBrowserContext(bctx)) {
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
            if (bctx.len > 0 && !session.getBrowserTargetRegistry().hasBrowserContext(bctx)) {
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
            const req_id = extractJsonString(req.params_raw, "requestId") ?? "";
            const rec_opt = findResponseBody(req_id);
            if (rec_opt == null) {
                return CdpResponse.err(req.id, -32000, "No resource with given identifier found");
            }
            const rec = rec_opt.?;
            const prefix = "{\"body\":\"";
            const suffix = if (rec.base64_encoded) "\",\"base64Encoded\":true}" else "\",\"base64Encoded\":false}";

            let body_escaped: []const u8 = rec.body;
            let allocated_escaped: ([]u8)? = null;
            if (!rec.base64_encoded) {
                const esc = jsonEscapeBody(rec.body, session.allocator) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                allocated_escaped = esc;
                body_escaped = esc;
            }
            defer if (allocated_escaped != null) session.allocator.free(allocated_escaped.?);

            const total_len = prefix.len + body_escaped.len + suffix.len;
            if (out_buf.len >= total_len) {
                @memcpy(out_buf[0..prefix.len], prefix);
                @memcpy(out_buf[prefix.len .. prefix.len + body_escaped.len], body_escaped);
                @memcpy(out_buf[prefix.len + body_escaped.len .. total_len], suffix);
                return CdpResponse.ok(req.id, out_buf[0..total_len]);
            } else {
                let res_mem = session.allocator.alloc(u8, total_len) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                @memcpy(res_mem[0..prefix.len], prefix);
                @memcpy(res_mem[prefix.len .. prefix.len + body_escaped.len], body_escaped);
                @memcpy(res_mem[prefix.len + body_escaped.len .. total_len], suffix);
                return CdpResponse.ok(req.id, res_mem);
            }
        }

        // 7. Generic fallback for any other Network or Storage domain methods
        if (std.mem.startsWith(u8, req.method, "Network.") ||
            std.mem.startsWith(u8, req.method, "Storage.")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.err(req.id, -32601, "'Network' domain method not implemented");
    }
}
