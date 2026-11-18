// src/cdp/types.cp
// Chrome DevTools Protocol (CDP) Data Types & Protocol Structures for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/types.ts

pub struct CdpBoxModel {
    width: f32 = 0.0,
    height: f32 = 0.0,
    border: [8]f32 = [0.0; 8],
    content: [8]f32 = [0.0; 8],
}

pub struct TargetState {
    id: []const u8 = "capsurf-target-001",
    target_type: []const u8 = "page",
    title: []const u8 = "CapSurf Agent Blank Page",
    url: []const u8 = "about:blank",
    attached: bool = true,
    session_id: []const u8 = "capsurf-session-001",
}

pub struct CdpTarget {
    id: []const u8 = "capsurf-target-001",
    title: []const u8 = "CapSurf Agent Blank Page",
    url: []const u8 = "about:blank",
    target_type: []const u8 = "page",
    ws_url: []const u8 = "ws://127.0.0.1:9222/devtools/page/capsurf-target-001",
}

pub struct CdpRequest {
    id: i64 = 0,
    method: []const u8 = "",
    params_raw: []const u8 = "{}",
    session_id: []const u8 = "",

    pub static fn parse(json: []const u8): CdpRequest {
        const id_val = extractJsonInt(json, "id") ?? 0;
        const method_val = extractJsonString(json, "method") ?? "";
        const sid_val = extractJsonString(json, "sessionId") ?? "";
        return CdpRequest {
            id: id_val,
            method: method_val,
            params_raw: json,
            session_id: sid_val,
        };
    }
}

pub struct CdpResponse {
    id: i64 = 0,
    result_json: []const u8 = "{}",
    error_code: i32 = 0,
    error_message: []const u8 = "",
    session_id: []const u8 = "",

    pub static fn ok(id: i64, result_json: []const u8): CdpResponse {
        return CdpResponse {
            id: id,
            result_json: result_json,
            error_code: 0,
            error_message: "",
            session_id: "",
        };
    }

    pub static fn err(id: i64, code: i32, msg: []const u8): CdpResponse {
        return CdpResponse {
            id: id,
            result_json: "{}",
            error_code: code,
            error_message: msg,
            session_id: "",
        };
    }

    pub fn isOk(): bool {
        return _.error_code == 0;
    }
}

pub struct CdpEvent {
    method: []const u8 = "",
    params_json: []const u8 = "{}",
    session_id: []const u8 = "",

    pub static fn create(method: []const u8, params_json: []const u8): CdpEvent {
        return CdpEvent {
            method: method,
            params_json: params_json,
            session_id: "",
        };
    }
}

/// Zero-allocation JSON string field extractor.
pub fn extractJsonString(json: []const u8, key: []const u8): ([]const u8)? {
    let key_pat_buf: [64]u8 = [0; 64];
    if (key.len + 3 > key_pat_buf.len) return null;
    key_pat_buf[0] = '"';
    @memcpy(key_pat_buf[1 .. 1 + key.len], key);
    key_pat_buf[1 + key.len] = '"';
    const key_pat = key_pat_buf[0 .. 2 + key.len];

    const pos = std.mem.indexOf(u8, json, key_pat) orelse return null;
    let idx = pos + key_pat.len;

    // Skip whitespace and colon
    while (idx < json.len && (json[idx] == ' ' || json[idx] == ':' || json[idx] == '\t' || json[idx] == '\r' || json[idx] == '\n')) : (idx += 1) {}
    if (idx >= json.len || json[idx] != '"') return null;
    idx += 1; // Skip opening quote

    const start = idx;
    while (idx < json.len) : (idx += 1) {
        if (json[idx] == '\\') {
            idx += 1; // Skip escaped character
            continue;
        }
        if (json[idx] == '"') {
            return json[start..idx];
        }
    }
    return null;
}

pub fn unescapeJsonString(src: []const u8, dst: []u8): []const u8 {
    let out_idx: usize = 0;
    let i: usize = 0;
    while (i < src.len && out_idx < dst.len) {
        if (src[i] == '\\' && i + 1 < src.len) {
            const next = src[i + 1];
            if (next == 'n') {
                dst[out_idx] = '\n';
                out_idx += 1;
                i += 2;
            } else if (next == 'r') {
                dst[out_idx] = '\r';
                out_idx += 1;
                i += 2;
            } else if (next == 't') {
                dst[out_idx] = '\t';
                out_idx += 1;
                i += 2;
            } else if (next == '"') {
                dst[out_idx] = '"';
                out_idx += 1;
                i += 2;
            } else if (next == '\\') {
                dst[out_idx] = '\\';
                out_idx += 1;
                i += 2;
            } else if (next == '/') {
                dst[out_idx] = '/';
                out_idx += 1;
                i += 2;
            } else {
                dst[out_idx] = src[i];
                out_idx += 1;
                i += 1;
            }
        } else {
            dst[out_idx] = src[i];
            out_idx += 1;
            i += 1;
        }
    }
    return dst[0..out_idx];
}

/// Zero-allocation JSON integer field extractor.
pub fn extractJsonInt(json: []const u8, key: []const u8): (i64)? {
    let key_pat_buf: [64]u8 = [0; 64];
    if (key.len + 3 > key_pat_buf.len) return null;
    key_pat_buf[0] = '"';
    @memcpy(key_pat_buf[1 .. 1 + key.len], key);
    key_pat_buf[1 + key.len] = '"';
    const key_pat = key_pat_buf[0 .. 2 + key.len];

    const pos = std.mem.indexOf(u8, json, key_pat) orelse return null;
    let idx = pos + key_pat.len;

    // Skip whitespace and colon
    while (idx < json.len && (json[idx] == ' ' || json[idx] == ':' || json[idx] == '\t' || json[idx] == '\r' || json[idx] == '\n')) : (idx += 1) {}
    if (idx >= json.len) return null;

    let is_neg = false;
    if (json[idx] == '-') {
        is_neg = true;
        idx += 1;
    }

    let val: i64 = 0;
    let has_digits = false;
    while (idx < json.len && json[idx] >= '0' && json[idx] <= '9') : (idx += 1) {
        val = val * 10 + @as(i64, json[idx] - '0');
        has_digits = true;
    }

    if (!has_digits) return null;
    return if (is_neg) -val else val;
}

pub fn extractJsonFloat(json: []const u8, key: []const u8): (f32)? {
    let key_pat_buf: [64]u8 = [0; 64];
    if (key.len + 3 > key_pat_buf.len) return null;
    key_pat_buf[0] = '"';
    @memcpy(key_pat_buf[1 .. 1 + key.len], key);
    key_pat_buf[1 + key.len] = '"';
    const key_pat = key_pat_buf[0 .. 2 + key.len];

    const pos = std.mem.indexOf(u8, json, key_pat) orelse return null;
    let idx = pos + key_pat.len;

    // Skip whitespace and colon
    while (idx < json.len && (json[idx] == ' ' || json[idx] == ':' || json[idx] == '\t' || json[idx] == '\r' || json[idx] == '\n')) : (idx += 1) {}
    if (idx >= json.len) return null;

    const start = idx;
    let has_digits = false;
    while (idx < json.len) : (idx += 1) {
        const c = json[idx];
        if (c >= '0' && c <= '9') {
            has_digits = true;
        } else if (c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E') {
            // continue parsing valid float chars
        } else {
            break;
        }
    }

    if (!has_digits) return null;
    const slice = json[start..idx];
    return std.fmt.parseFloat(f32, slice) catch null;
}

pub fn extractJsonBool(json: []const u8, key: []const u8): (bool)? {
    let key_pat_buf: [64]u8 = [0; 64];
    if (key.len + 3 > key_pat_buf.len) return null;
    key_pat_buf[0] = '"';
    @memcpy(key_pat_buf[1 .. 1 + key.len], key);
    key_pat_buf[1 + key.len] = '"';
    const key_pat = key_pat_buf[0 .. 2 + key.len];

    const pos = std.mem.indexOf(u8, json, key_pat) orelse return null;
    let idx = pos + key_pat.len;

    // Skip whitespace and colon
    while (idx < json.len && (json[idx] == ' ' || json[idx] == ':' || json[idx] == '\t' || json[idx] == '\r' || json[idx] == '\n')) : (idx += 1) {}
    if (idx >= json.len) return null;

    if (idx + 4 <= json.len && std.mem.eql(u8, json[idx .. idx + 4], "true")) return true;
    if (idx + 5 <= json.len && std.mem.eql(u8, json[idx .. idx + 5], "false")) return false;
    return null;
}
