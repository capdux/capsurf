// src/cdp/types.cp
// Chrome DevTools Protocol (CDP) Data Types & Protocol Structures for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/types.ts

pub const MAX_CDP_EVENTS: usize = 64;

pub struct CdpBoxModel {
    width: f32 = 0.0,
    height: f32 = 0.0,
    border: [8]f32 = [0.0; 8],
    content: [8]f32 = [0.0; 8],
    padding: [8]f32 = [0.0; 8],
    margin: [8]f32 = [0.0; 8],
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

pub fn findTopLevelJsonKey(json: []const u8, key: []const u8): usize? {
    if (json.len < 2) return null;
    let i: usize = 0;
    while (i < json.len && json[i] != '{') : (i += 1) {}
    if (i >= json.len) return null;
    i += 1;

    let depth: i32 = 0;
    let in_str = false;
    let escaped = false;

    while (i < json.len) : (i += 1) {
        const c = json[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (c == '\\' && in_str) {
            escaped = true;
            continue;
        }
        if (c == '"') {
            if (!in_str) {
                if (depth == 0) {
                    const start = i + 1;
                    let k_end = start;
                    while (k_end < json.len && json[k_end] != '"') : (k_end += 1) {}
                    if (k_end < json.len) {
                        const k = json[start..k_end];
                        if (std.mem.eql(u8, k, key)) {
                            return k_end + 1;
                        }
                        i = k_end;
                        continue;
                    }
                }
                in_str = true;
            } else {
                in_str = false;
            }
            continue;
        }
        if (!in_str) {
            if (c == '{' || c == '[') {
                depth += 1;
            } else if (c == '}' || c == ']') {
                depth -= 1;
                if (depth < 0) break;
            }
        }
    }
    return null;
}

pub struct CdpRequest {
    id: i64 = 0,
    raw_id: []const u8 = "",
    method: []const u8 = "",
    params_raw: []const u8 = "{}",
    session_id: []const u8 = "",
    envelope_error_code: i32 = 0,
    envelope_error_message: []const u8 = "",

    pub fn hasEnvelopeError(): bool {
        return _.envelope_error_code != 0;
    }

    pub static fn parse(json: []const u8): CdpRequest {
        const trimmed = std.mem.trim(u8, json, " \t\r\n");
        if (trimmed.len == 0) {
            return CdpRequest {
                id: 0,
                envelope_error_code: -32700,
                envelope_error_message: "Parse error",
            };
        }

        // Check if JSON array: e.g. [1, 2] -> -32600 Invalid Request
        if (trimmed[0] == '[' && trimmed[trimmed.len - 1] == ']') {
            return CdpRequest {
                id: 0,
                envelope_error_code: -32600,
                envelope_error_message: "Invalid Request",
            };
        }

        // Must start with '{' and end with '}'
        if (trimmed[0] != '{' || trimmed[trimmed.len - 1] != '}') {
            return CdpRequest {
                id: 0,
                envelope_error_code: -32700,
                envelope_error_message: "Parse error",
            };
        }

        let id_val: i64 = 0;
        let raw_id_val: []const u8 = "";
        let id_is_string = false;

        if (id_pos := findTopLevelJsonKey(trimmed, "id")) {
            let idx = id_pos;
            while (idx < trimmed.len && (trimmed[idx] == ' ' || trimmed[idx] == ':' || trimmed[idx] == '\t' || trimmed[idx] == '\r' || trimmed[idx] == '\n')) : (idx += 1) {}
            if (idx < trimmed.len) {
                if (trimmed[idx] == '"') {
                    id_is_string = true;
                    idx += 1;
                    const s_start = idx;
                    while (idx < trimmed.len && trimmed[idx] != '"') : (idx += 1) {
                        if (trimmed[idx] == '\\' && idx + 1 < trimmed.len) idx += 1;
                    }
                    raw_id_val = trimmed[s_start..idx];
                } else if (trimmed[idx] == 'n') {
                    id_val = 0;
                } else {
                    if (extracted_id := extractJsonInt(trimmed, "id")) {
                        id_val = extracted_id;
                    }
                }
            }
        }

        if (id_is_string) {
            return CdpRequest {
                id: 0,
                raw_id: raw_id_val,
                envelope_error_code: -32600,
                envelope_error_message: "Invalid Request",
            };
        }

        const method_pos = findTopLevelJsonKey(trimmed, "method");
        if (method_pos == null) {
            return CdpRequest {
                id: id_val,
                envelope_error_code: -32600,
                envelope_error_message: "Invalid Request",
            };
        }
        let m_idx = method_pos.?;
        while (m_idx < trimmed.len && (trimmed[m_idx] == ' ' || trimmed[m_idx] == ':' || trimmed[m_idx] == '\t' || trimmed[m_idx] == '\r' || trimmed[m_idx] == '\n')) : (m_idx += 1) {}
        if (m_idx >= trimmed.len || trimmed[m_idx] != '"') {
            return CdpRequest {
                id: id_val,
                envelope_error_code: -32600,
                envelope_error_message: "Invalid Request",
            };
        }
        const method_val = extractJsonString(trimmed, "method") ?? "";
        if (method_val.len == 0) {
            return CdpRequest {
                id: id_val,
                envelope_error_code: -32600,
                envelope_error_message: "Invalid Request",
            };
        }

        let sid_val: []const u8 = "";
        if (sid_pos := findTopLevelJsonKey(trimmed, "sessionId")) {
            let s_idx = sid_pos;
            while (s_idx < trimmed.len && (trimmed[s_idx] == ' ' || trimmed[s_idx] == ':' || trimmed[s_idx] == '\t' || trimmed[s_idx] == '\r' || trimmed[s_idx] == '\n')) : (s_idx += 1) {}
            if (s_idx < trimmed.len) {
                if (trimmed[s_idx] == '"') {
                    s_idx += 1;
                    const s_start = s_idx;
                    while (s_idx < trimmed.len && trimmed[s_idx] != '"') : (s_idx += 1) {
                        if (trimmed[s_idx] == '\\' && s_idx + 1 < trimmed.len) s_idx += 1;
                    }
                    sid_val = trimmed[s_start..s_idx];
                } else if (trimmed[s_idx] == 'n') {
                    sid_val = "";
                } else {
                    return CdpRequest {
                        id: id_val,
                        envelope_error_code: -32600,
                        envelope_error_message: "Invalid Request",
                    };
                }
            }
        }

        let p_raw: []const u8 = trimmed;
        if (p_pos := findTopLevelJsonKey(trimmed, "params")) {
            let p_idx = p_pos;
            while (p_idx < trimmed.len && (trimmed[p_idx] == ' ' || trimmed[p_idx] == ':' || trimmed[p_idx] == '\t' || trimmed[p_idx] == '\r' || trimmed[p_idx] == '\n')) : (p_idx += 1) {}
            if (p_idx < trimmed.len) {
                if (trimmed[p_idx] == '{') {
                    p_raw = trimmed;
                } else if (trimmed[p_idx] == 'n') {
                    p_raw = "{}";
                } else {
                    return CdpRequest {
                        id: id_val,
                        envelope_error_code: -32600,
                        envelope_error_message: "Invalid Request",
                    };
                }
            }
        }

        return CdpRequest {
            id: id_val,
            raw_id: raw_id_val,
            method: method_val,
            params_raw: p_raw,
            session_id: sid_val,
            envelope_error_code: 0,
            envelope_error_message: "",
        };
    }
}

pub struct CdpResponse {
    id: i64 = 0,
    raw_id: []const u8 = "",
    result_json: []const u8 = "{}",
    error_code: i32 = 0,
    error_message: []const u8 = "",
    session_id: []const u8 = "",

    pub static fn ok(id: i64, result_json: []const u8): CdpResponse {
        return CdpResponse {
            id: id,
            raw_id: "",
            result_json: result_json,
            error_code: 0,
            error_message: "",
            session_id: "",
        };
    }

    pub static fn okWithRawId(raw_id: []const u8, result_json: []const u8): CdpResponse {
        return CdpResponse {
            id: 0,
            raw_id: raw_id,
            result_json: result_json,
            error_code: 0,
            error_message: "",
            session_id: "",
        };
    }

    pub static fn err(id: i64, code: i32, msg: []const u8): CdpResponse {
        return CdpResponse {
            id: id,
            raw_id: "",
            result_json: "{}",
            error_code: code,
            error_message: msg,
            session_id: "",
        };
    }

    pub static fn errWithRawId(raw_id: []const u8, code: i32, msg: []const u8): CdpResponse {
        return CdpResponse {
            id: 0,
            raw_id: raw_id,
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

pub fn extractJsonArray(json: []const u8, key: []const u8): ([]const u8)? {
    let key_pat_buf: [64]u8 = [0; 64];
    if (key.len + 3 > key_pat_buf.len) return null;
    key_pat_buf[0] = '"';
    @memcpy(key_pat_buf[1 .. 1 + key.len], key);
    key_pat_buf[1 + key.len] = '"';
    const key_pat = key_pat_buf[0 .. 2 + key.len];

    const pos = std.mem.indexOf(u8, json, key_pat) orelse return null;
    let idx = pos + key_pat.len;

    while (idx < json.len && (json[idx] == ' ' || json[idx] == ':' || json[idx] == '\t' || json[idx] == '\r' || json[idx] == '\n')) : (idx += 1) {}
    if (idx >= json.len || json[idx] != '[') return null;

    const start = idx;
    let depth: i32 = 0;
    let in_str = false;
    let escaped = false;
    while (idx < json.len) : (idx += 1) {
        const ch = json[idx];
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
            if (ch == '[') depth += 1;
            if (ch == ']') {
                depth -= 1;
                if (depth == 0) {
                    return json[start .. idx + 1];
                }
            }
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

pub fn normalizeExceptionMessage(input: []const u8, out: []u8): []const u8 {
    let in_idx: usize = 0;
    let out_idx: usize = 0;

    while (in_idx < input.len && out_idx < out.len) {
        const rem = input[in_idx..];
        let is_read = false;
        let is_set = false;
        let is_upper = false;
        let match_len: usize = 0;

        if (std.mem.startsWith(u8, rem, "cannot read property '")) {
            is_read = true; match_len = 22;
        } else if (std.mem.startsWith(u8, rem, "Cannot read property '")) {
            is_read = true; is_upper = true; match_len = 22;
        } else if (std.mem.startsWith(u8, rem, "cannot set property '")) {
            is_set = true; match_len = 21;
        } else if (std.mem.startsWith(u8, rem, "Cannot set property '")) {
            is_set = true; is_upper = true; match_len = 21;
        }

        if (is_read || is_set) {
            const prop_start = in_idx + match_len;
            if (quote_pos := std.mem.indexOfScalar(u8, input[prop_start..], '\'')) {
                const prop_end = prop_start + quote_pos;
                const prop_name = input[prop_start..prop_end];
                const after_quote = input[prop_end + 1 ..];

                let target: []const u8 = "";
                let target_len: usize = 0;
                if (std.mem.startsWith(u8, after_quote, " of null")) {
                    target = "null";
                    target_len = 8;
                } else if (std.mem.startsWith(u8, after_quote, " of undefined")) {
                    target = "undefined";
                    target_len = 13;
                }

                if (target.len > 0) {
                    const prefix = if (is_read)
                        (if (is_upper) "Cannot read properties of " else "cannot read properties of ")
                    else
                        (if (is_upper) "Cannot set properties of " else "cannot set properties of ");
                    const action = if (is_read) " (reading '" else " (setting '";

                    const total_len = prefix.len + target.len + action.len + prop_name.len + 2;
                    if (out_idx + total_len <= out.len) {
                        @memcpy(out[out_idx .. out_idx + prefix.len], prefix);
                        out_idx += prefix.len;
                        @memcpy(out[out_idx .. out_idx + target.len], target);
                        out_idx += target.len;
                        @memcpy(out[out_idx .. out_idx + action.len], action);
                        out_idx += action.len;
                        @memcpy(out[out_idx .. out_idx + prop_name.len], prop_name);
                        out_idx += prop_name.len;
                        out[out_idx] = '\'';
                        out[out_idx + 1] = ')';
                        out_idx += 2;

                        in_idx = prop_end + 1 + target_len;
                        continue;
                    }
                }
            }
        }

        out[out_idx] = input[in_idx];
        out_idx += 1;
        in_idx += 1;
    }

    return out[0..out_idx];
}
