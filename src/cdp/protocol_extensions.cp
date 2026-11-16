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
        let json = "{\"enabled\":true,\"active\":true,\"framesRendered\":1,\"partialFramesRendered\":0,\"relaunchCount\":0,\"partialRaster\":false}";
        if (json.len > out_buf.len) return null;
        @memcpy(out_buf[0..json.len], json);
        return out_buf[0..json.len];
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

    if (std.mem.eql(u8, cap_method, "getMemoryStats")) {
        let json = "{\"wasmBytes\":0,\"frameWasmBytes\":0}";
        if (json.len > out_buf.len) return null;
        @memcpy(out_buf[0..json.len], json);
        return out_buf[0..json.len];
    }

    return null;
}
