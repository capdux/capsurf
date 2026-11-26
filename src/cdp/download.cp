// src/cdp/download.cp
// Clean-room CDP Download Engine for CapSurf

import { CdpEvent, MAX_CDP_EVENTS } from "./types.cp";

@c {
    fn mkdir(path: [*c]const u8, mode: u32): i32;
    fn fopen(pathname: [*c]const u8, mode: [*c]const u8): (*void)?;
    fn fwrite(ptr: *const void, size: usize, nmemb: usize, stream: *void): usize;
    fn fclose(stream: *void): i32;
}

fn ensureDirPath(dir_path: []const u8) {
    if (dir_path.len == 0 || dir_path.len >= 512) return;
    let buf: [513]u8 = [0; 513];
    @memcpy(buf[0..dir_path.len], dir_path);
    buf[dir_path.len] = 0;
    let i: usize = 1;
    while (i < dir_path.len) : (i += 1) {
        if (buf[i] == '/') {
            buf[i] = 0;
            * = mkdir(unsafe { @ptrCast(&buf[0]) }, 0o755);
            buf[i] = '/';
        }
    }
    * = mkdir(unsafe { @ptrCast(&buf[0]) }, 0o755);
}

fn writeDownloadFile(dir_path: []const u8, full_path: []const u8, body: []const u8): bool {
    if (full_path.len == 0 || full_path.len >= 1024) return false;
    ensureDirPath(dir_path);
    let fz: [1025]u8 = [0; 1025];
    @memcpy(fz[0..full_path.len], full_path);
    fz[full_path.len] = 0;
    const fp_opt = fopen(unsafe { @ptrCast(&fz[0]) }, "wb");
    if (fp := fp_opt) {
        defer * = fclose(fp);
        if (body.len > 0) {
            const written = fwrite(unsafe { @ptrCast(body.ptr) }, 1, body.len, fp);
            return written == body.len;
        }
        return true;
    }
    return false;
}

pub enum DownloadBehavior {
    deny,
    allow,
    allow_and_name,
    stream,
}

pub struct DownloadConfig {
    browser_context_id_buf: [128]u8 = [0; 128],
    browser_context_id_len: usize = 0,
    behavior: DownloadBehavior = .deny,
    download_path_buf: [512]u8 = [0; 512],
    download_path_len: usize = 0,
    events_enabled: bool = false,
    download_session_id_buf: [128]u8 = [0; 128],
    download_session_id_len: usize = 0,

    fn browserContextId(): []const u8 {
        return _.browser_context_id_buf[0.._.browser_context_id_len];
    }

    fn downloadPath(): []const u8 {
        return _.download_path_buf[0.._.download_path_len];
    }

    fn downloadSessionId(): []const u8 {
        return _.download_session_id_buf[0.._.download_session_id_len];
    }
}

pub struct DownloadConfigStore {
    configs: [16]DownloadConfig = [DownloadConfig{}; 16],
    count: usize = 0,
    seq: u64 = 0,

    fn upsert(
        *_,
        browser_context_id: []const u8,
        behavior: DownloadBehavior,
        download_path: []const u8,
        events_enabled: bool,
        download_session_id: []const u8,
    ) {
        let idx: usize = 0;
        let found: usize? = null;
        while (idx < _.count) : (idx += 1) {
            if (std.mem.eql(u8, _.configs[idx].browserContextId(), browser_context_id)) {
                found = idx;
                break;
            }
        }

        let target_idx: usize = 0;
        if (f := found) {
            target_idx = f;
        } else {
            if (_.count < _.configs.len) {
                target_idx = _.count;
                _.count += 1;
            } else {
                // Overwrite oldest slot 0 if full
                target_idx = 0;
            }
        }

        let cfg = DownloadConfig{};
        const ctx_len = @min(browser_context_id.len, cfg.browser_context_id_buf.len);
        if (ctx_len > 0) @memcpy(cfg.browser_context_id_buf[0..ctx_len], browser_context_id[0..ctx_len]);
        cfg.browser_context_id_len = ctx_len;

        cfg.behavior = behavior;

        const path_len = @min(download_path.len, cfg.download_path_buf.len);
        if (path_len > 0) @memcpy(cfg.download_path_buf[0..path_len], download_path[0..path_len]);
        cfg.download_path_len = path_len;

        cfg.events_enabled = events_enabled;

        const sid_len = @min(download_session_id.len, cfg.download_session_id_buf.len);
        if (sid_len > 0) @memcpy(cfg.download_session_id_buf[0..sid_len], download_session_id[0..sid_len]);
        cfg.download_session_id_len = sid_len;

        _.configs[target_idx] = cfg;
    }

    fn findForContext(browser_context_id: []const u8): DownloadConfig? {
        let idx: usize = 0;
        while (idx < _.count) : (idx += 1) {
            if (std.mem.eql(u8, _.configs[idx].browserContextId(), browser_context_id)) {
                return _.configs[idx];
            }
        }
        // Fallback to default context ("") if a specific context has no override
        if (browser_context_id.len > 0) {
            idx = 0;
            while (idx < _.count) : (idx += 1) {
                if (_.configs[idx].browser_context_id_len == 0) {
                    return _.configs[idx];
                }
            }
        }
        return null;
    }

    fn nextGuid(*_, out_buf: []u8): []const u8 {
        _.seq += 1;
        return std.fmt.bufPrint(out_buf, "capsurf-dl-{d}", .{_.seq}) catch "capsurf-dl-0";
    }
}

fn sanitizeFilename(raw: []const u8, fallback: []const u8, out_buf: []u8): []const u8 {
    let s = std.mem.trim(u8, raw, " \t\r\n\"'");
    if (s.len == 0) {
        const flen = @min(fallback.len, out_buf.len);
        if (flen > 0) @memcpy(out_buf[0..flen], fallback[0..flen]);
        return out_buf[0..flen];
    }

    // Take last path segment after '/' or '\\'
    let last_sep: usize? = null;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '/' || s[i] == '\\') {
            last_sep = i;
        }
    }
    if (sep := last_sep) {
        s = s[sep + 1 ..];
    }

    // Strip any remaining "..", '/', '\\', or control chars
    let out_len: usize = 0;
    i = 0;
    while (i < s.len && out_len < out_buf.len) : (i += 1) {
        const c = s[i];
        if (c == '/' || c == '\\' || c < 0x20 || c == 0x7F) {
            continue;
        }
        if (c == '.' && i + 1 < s.len && s[i + 1] == '.') {
            i += 1;
            continue;
        }
        out_buf[out_len] = c;
        out_len += 1;
    }

    const cleaned = std.mem.trim(u8, out_buf[0..out_len], " .");
    if (cleaned.len == 0) {
        const flen = @min(fallback.len, out_buf.len);
        if (flen > 0) @memcpy(out_buf[0..flen], fallback[0..flen]);
        return out_buf[0..flen];
    }
    if (unsafe { @intFromPtr(cleaned.ptr) != @intFromPtr(out_buf.ptr) }) {
        // Shift left so slice starts at out_buf[0]
        let k: usize = 0;
        while (k < cleaned.len) : (k += 1) {
            out_buf[k] = cleaned[k];
        }
    }
    return out_buf[0..cleaned.len];
}

fn extractFilenameFromContentDisposition(cd: []const u8): ([]const u8)? {
    if (cd.len == 0) return null;

    // Prefer filename*= (RFC 5987, e.g. filename*=UTF-8''report.pdf)
    let i: usize = 0;
    while (i + 9 <= cd.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(cd[i .. i + 9], "filename*")) {
            let j = i + 9;
            while (j < cd.len && (cd[j] == ' ' || cd[j] == '\t')) : (j += 1) {}
            if (j < cd.len && cd[j] == '=') {
                j += 1;
                while (j < cd.len && (cd[j] == ' ' || cd[j] == '\t')) : (j += 1) {}
                let end = j;
                while (end < cd.len && cd[end] != ';') : (end += 1) {}
                let val = std.mem.trim(u8, cd[j..end], " \t\r\n\"'");
                if (qq := std.mem.indexOf(u8, val, "''")) {
                    val = val[qq + 2 ..];
                }
                if (val.len > 0) return val;
            }
        }
    }

    // Fallback to filename=
    i = 0;
    while (i + 8 <= cd.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(cd[i .. i + 8], "filename")) {
            let j = i + 8;
            if (j < cd.len && cd[j] == '*') continue;
            while (j < cd.len && (cd[j] == ' ' || cd[j] == '\t')) : (j += 1) {}
            if (j < cd.len && cd[j] == '=') {
                j += 1;
                while (j < cd.len && (cd[j] == ' ' || cd[j] == '\t')) : (j += 1) {}
                if (j < cd.len && cd[j] == '"') {
                    j += 1;
                    let end = j;
                    while (end < cd.len && cd[end] != '"') : (end += 1) {}
                    if (end > j) return cd[j..end];
                } else {
                    let end = j;
                    while (end < cd.len && cd[end] != ';') : (end += 1) {}
                    const val = std.mem.trim(u8, cd[j..end], " \t\r\n");
                    if (val.len > 0) return val;
                }
            }
        }
    }
    return null;
}

fn extractUrlBasename(url: []const u8): ([]const u8)? {
    if (url.len == 0) return null;
    if (url.len >= 5 && std.ascii.eqlIgnoreCase(url[0..5], "data:")) return "";
    let end = url.len;
    if (q := std.mem.indexOfScalar(u8, url, '?')) {
        if (q < end) end = q;
    }
    if (h := std.mem.indexOfScalar(u8, url[0..end], '#')) {
        if (h < end) end = h;
    }
    let path = url[0..end];
    if (scheme := std.mem.indexOf(u8, path, "://")) {
        const after_scheme = path[scheme + 3 ..];
        if (slash := std.mem.indexOfScalar(u8, after_scheme, '/')) {
            path = after_scheme[slash + 1 ..];
        } else {
            return null;
        }
    }
    while (path.len > 0 && path[path.len - 1] == '/') {
        path = path[0 .. path.len - 1];
    }
    if (path.len == 0) return null;
    if (last := std.mem.lastIndexOfScalar(u8, path, '/')) {
        path = path[last + 1 ..];
    }
    if (path.len == 0) return null;
    return path;
}

fn escapeJsonString(src: []const u8, out_buf: []u8): []const u8 {
    let out_len: usize = 0;
    for (c of src) {
        if (c == '"' || c == '\\') {
            if (out_len + 2 > out_buf.len) break;
            out_buf[out_len] = '\\';
            out_buf[out_len + 1] = c;
            out_len += 2;
        } else if (c == '\n') {
            if (out_len + 2 > out_buf.len) break;
            out_buf[out_len] = '\\';
            out_buf[out_len + 1] = 'n';
            out_len += 2;
        } else if (c == '\r') {
            if (out_len + 2 > out_buf.len) break;
            out_buf[out_len] = '\\';
            out_buf[out_len + 1] = 'r';
            out_len += 2;
        } else if (c == '\t') {
            if (out_len + 2 > out_buf.len) break;
            out_buf[out_len] = '\\';
            out_buf[out_len + 1] = 't';
            out_len += 2;
        } else if (c >= 0x20) {
            if (out_len + 1 > out_buf.len) break;
            out_buf[out_len] = c;
            out_len += 1;
        }
    }
    return out_buf[0..out_len];
}

fn pushDownloadEvent(
    allocator: std.mem.Allocator,
    out_events: *[MAX_CDP_EVENTS]CdpEvent,
    event_count: *usize,
    method: []const u8,
    params_json: []const u8,
    session_id: []const u8,
) {
    if (event_count.* >= out_events.len) return;
    let ev = CdpEvent.create(method, params_json);
    if (session_id.len > 0) {
        ev.session_id = allocator.dupe(u8, session_id) catch "";
    }
    out_events[event_count.*] = ev;
    event_count.* += 1;
}

pub fn executeDownload(
    allocator: std.mem.Allocator,
    store: *DownloadConfigStore,
    browser_context_id: []const u8,
    frame_id: []const u8,
    url: []const u8,
    content_disposition: []const u8,
    download_attr_hint: []const u8,
    status_code: u16,
    body: []const u8,
    out_events: *[MAX_CDP_EVENTS]CdpEvent,
    event_count: *usize,
): bool {
    const cfg_opt = store.findForContext(browser_context_id);
    let cfg = DownloadConfig{};
    if (c := cfg_opt) {
        cfg = c;
    } else {
        return false;
    }

    if (cfg.behavior == .deny && !cfg.events_enabled) {
        return false;
    }

    let guid_buf: [64]u8 = [0; 64];
    const guid = store.nextGuid(&guid_buf);

    let raw_name: []const u8 = guid;
    if (cd_name := extractFilenameFromContentDisposition(content_disposition)) {
        raw_name = cd_name;
    } else if (download_attr_hint.len > 0) {
        raw_name = download_attr_hint;
    } else if (url_name := extractUrlBasename(url)) {
        raw_name = url_name;
    }

    let name_buf: [256]u8 = [0; 256];
    const suggested_filename = sanitizeFilename(raw_name, guid, &name_buf);

    const download_path = cfg.downloadPath();
    const sid = cfg.downloadSessionId();
    const ok_status = status_code >= 200 && status_code < 300;
    let completed = ok_status && cfg.behavior != .deny;

    if (cfg.events_enabled) {
        let esc_frame_buf: [256]u8 = [0; 256];
        let esc_url_buf: [2048]u8 = [0; 2048];
        let esc_name_buf: [512]u8 = [0; 512];
        const esc_frame = escapeJsonString(frame_id, &esc_frame_buf);
        const esc_url = escapeJsonString(url, &esc_url_buf);
        const esc_name = escapeJsonString(suggested_filename, &esc_name_buf);

        const begin_json = std.fmt.allocPrint(
            allocator,
            "{{\"frameId\":\"{s}\",\"guid\":\"{s}\",\"url\":\"{s}\",\"suggestedFilename\":\"{s}\"}}",
            .{ esc_frame, guid, esc_url, esc_name },
        ) catch "{}";
        pushDownloadEvent(allocator, out_events, event_count, "Browser.downloadWillBegin", begin_json, sid);

        const prog_json = std.fmt.allocPrint(
            allocator,
            "{{\"guid\":\"{s}\",\"totalBytes\":{d},\"receivedBytes\":0,\"state\":\"inProgress\"}}",
            .{ guid, body.len },
        ) catch "{}";
        pushDownloadEvent(allocator, out_events, event_count, "Browser.downloadProgress", prog_json, sid);
    }

    if (ok_status && (cfg.behavior == .allow || cfg.behavior == .allow_and_name) && download_path.len > 0) {
        const target_name = if (cfg.behavior == .allow_and_name) guid else suggested_filename;
        let full_path_buf: [1024]u8 = [0; 1024];
        const sep = if (download_path[download_path.len - 1] == '/') "" else "/";
        const full_path_res = std.fmt.bufPrint(&full_path_buf, "{s}{s}{s}", .{ download_path, sep, target_name }) catch null;

        if (full_path := full_path_res) {
            if (!writeDownloadFile(download_path, full_path, body)) {
                completed = false;
            }
        } else {
            completed = false;
        }
    }

    if (cfg.events_enabled) {
        const state_str: []const u8 = if (completed) "completed" else "canceled";
        const recv_bytes: usize = if (completed) body.len else 0;
        const done_json = std.fmt.allocPrint(
            allocator,
            "{{\"guid\":\"{s}\",\"totalBytes\":{d},\"receivedBytes\":{d},\"state\":\"{s}\"}}",
            .{ guid, body.len, recv_bytes, state_str },
        ) catch "{}";
        pushDownloadEvent(allocator, out_events, event_count, "Browser.downloadProgress", done_json, sid);
    }

    if (cfg.behavior == .deny) {
        return true;
    }
    return completed;
}
