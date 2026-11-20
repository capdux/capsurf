// Migrated from vendor/kitesurf/packages/engine/src/cdp/protocol-extensions.ts
// and vendor/kitesurf/packages/engine/src/cdp/kitesurf.ts
// Handles CapSurf / Kitesurf extended CDP methods for layout modes, session diagnostics, and renderer state.

pub enum LayoutMode {
    full,
    off,
}

pub struct ExtensionSessionState {
    layout_mode: LayoutMode = LayoutMode.full,
    page_state_seq: u32 = 1,
    frames_rendered: u32 = 0,
    paint_prepares: u64 = 0,
    renderer_active: bool = false,
    active_target_id: []const u8 = "capsurf-target-001",
}

pub fn handleProtocolExtension(
    method: []const u8,
    params_json: []const u8,
    session_id: []const u8,
    state: *ExtensionSessionState,
    out_buf: []u8,
): ([]const u8)? {
    * = params_json;
    * = session_id;

    // Support both CapSurf.* and Kitesurf.* prefixes
    let cap_method = method;
    if (std.mem.startsWith(u8, method, "Kitesurf.")) {
        cap_method = method[9..]; // strip "Kitesurf."
    } else if (std.mem.startsWith(u8, method, "CapSurf.")) {
        cap_method = method[8..]; // strip "CapSurf."
    } else {
        return null;
    }

    if (std.mem.eql(u8, cap_method, "getSessionInfo")) {
        let json = "{\"sessionId\":\"capsurf-session-001\",\"entrypoint\":\"capsurf\",\"connectionMode\":\"browser\",\"environment\":\"production\",\"buildVersion\":\"1.0.0\"}";
        if (json.len > out_buf.len) return null;
        @memcpy(out_buf[0..json.len], json);
        return out_buf[0..json.len];
    }

    if (std.mem.eql(u8, cap_method, "getLayoutMode")) {
        let mode_str = if (state.layout_mode == LayoutMode.full) "full" else "off";
        let prefix = "{\"mode\":\"";
        let suffix = "\"}";
        let pos: usize = 0;
        @memcpy(out_buf[pos .. pos + prefix.len], prefix);
        pos += prefix.len;
        @memcpy(out_buf[pos .. pos + mode_str.len], mode_str);
        pos += mode_str.len;
        @memcpy(out_buf[pos .. pos + suffix.len], suffix);
        pos += suffix.len;
        return out_buf[0..pos];
    }

    if (std.mem.eql(u8, cap_method, "setLayoutMode")) {
        if (std.mem.indexOf(u8, params_json, "\"off\"") != null) {
            state.layout_mode = LayoutMode.off;
        } else {
            state.layout_mode = LayoutMode.full;
        }
        let mode_str = if (state.layout_mode == LayoutMode.full) "full" else "off";
        let prefix = "{\"mode\":\"";
        let suffix = "\",\"scope\":\"browser\"}";
        let pos: usize = 0;
        @memcpy(out_buf[pos .. pos + prefix.len], prefix);
        pos += prefix.len;
        @memcpy(out_buf[pos .. pos + mode_str.len], mode_str);
        pos += mode_str.len;
        @memcpy(out_buf[pos .. pos + suffix.len], suffix);
        pos += suffix.len;
        return out_buf[0..pos];
    }

    if (std.mem.eql(u8, cap_method, "getRendererInfo")) {
        const is_active = state.renderer_active || state.frames_rendered > 0;
        const active_str = if (is_active) "true" else "false";
        const res = std.fmt.bufPrint(
            out_buf,
            "{{\"enabled\":true,\"active\":{s},\"framesRendered\":{d},\"partialFramesRendered\":0,\"relaunchCount\":0,\"partialRaster\":false}}",
            .{ active_str, state.frames_rendered },
        ) catch return null;
        return res;
    }

    if (std.mem.eql(u8, cap_method, "getPageState")) {
        state.page_state_seq += 1;
        let json = "{\"seq\":1,\"scrollX\":0,\"scrollY\":0,\"href\":\"about:blank\",\"title\":\"\"}";
        if (json.len > out_buf.len) return null;
        @memcpy(out_buf[0..json.len], json);
        return out_buf[0..json.len];
    }

    if (std.mem.eql(u8, cap_method, "setControlValue")) {
        let json = "{\"changed\":true,\"value\":\"applied\"}";
        if (json.len > out_buf.len) return null;
        @memcpy(out_buf[0..json.len], json);
        return out_buf[0..json.len];
    }

    if (std.mem.eql(u8, cap_method, "markPaintDamage")) {
        let json = "{}";
        if (json.len > out_buf.len) return null;
        @memcpy(out_buf[0..json.len], json);
        return out_buf[0..json.len];
    }

    if (std.mem.eql(u8, cap_method, "getPaintDamageSince")) {
        let json = "{\"nodeIds\":[],\"overflow\":false}";
        if (json.len > out_buf.len) return null;
        @memcpy(out_buf[0..json.len], json);
        return out_buf[0..json.len];
    }

    if (std.mem.eql(u8, cap_method, "setOutboundFilters")) {
        const pat_key = "\"allowRequestPattern\"";
        if (p_pos := std.mem.indexOf(u8, params_json, pat_key)) {
            let idx = p_pos + pat_key.len;
            while (idx < params_json.len && params_json[idx] != '[') : (idx += 1) {}
            if (idx < params_json.len && params_json[idx] == '[') {
                idx += 1;
                let has_non_string = false;
                while (idx < params_json.len) {
                    while (idx < params_json.len && (params_json[idx] == ' ' || params_json[idx] == ',' || params_json[idx] == '\t' || params_json[idx] == '\r' || params_json[idx] == '\n')) : (idx += 1) {}
                    if (idx >= params_json.len || params_json[idx] == ']') break;
                    if (params_json[idx] != '"') {
                        has_non_string = true;
                        break;
                    }
                    idx += 1;
                    while (idx < params_json.len && params_json[idx] != '"') : (idx += 1) {
                        if (params_json[idx] == '\\' && idx + 1 < params_json.len) idx += 1;
                    }
                    if (idx < params_json.len && params_json[idx] == '"') idx += 1;
                }
                if (has_non_string) {
                    const err_json = "{\"ok\":false,\"error\":\"allowRequestPattern must be an array of strings\"}";
                    if (err_json.len > out_buf.len) return null;
                    @memcpy(out_buf[0..err_json.len], err_json);
                    return out_buf[0..err_json.len];
                }
            }
        }
        const ok_json = "{\"ok\":true}";
        if (ok_json.len > out_buf.len) return null;
        @memcpy(out_buf[0..ok_json.len], ok_json);
        return out_buf[0..ok_json.len];
    }

    if (std.mem.eql(u8, cap_method, "getMemoryStats")) {
        const res = std.fmt.bufPrint(
            out_buf,
            "{{\"wasmBytes\":0,\"frameWasmBytes\":0,\"timerWheel\":{{\"wakesFired\":0,\"wakesArmed\":0}},\"paint\":{{\"prepares\":{d}}}}}",
            .{state.paint_prepares},
        ) catch return null;
        return res;
    }

    return null;
}
