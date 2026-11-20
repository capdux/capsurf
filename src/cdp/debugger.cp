// Migrated from vendor/kitesurf/packages/engine/src/cdp/debugger.ts
// Chrome DevTools Protocol (CDP) Debugger Domain for CapSurf

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, extractJsonBool } from "./types.cp";
import { CdpPageSession } from "./session.cp";

pub struct CompiledScriptMeta {
    scriptId: []const u8 = "",
    url: []const u8 = "",
    source: []const u8 = "",
    startLine: u32 = 0,
    startColumn: u32 = 0,
    endLine: u32 = 0,
    endColumn: u32 = 0,
    executionContextId: u32 = 1,
    hash: []const u8 = "",
    isModule: bool = false,
    length: usize = 0,
    sourceMapURL: []const u8 = "",
    hasSourceURL: bool = false,

    static fn create(scriptId: []const u8, source: []const u8, url: []const u8): CompiledScriptMeta {
        return CompiledScriptMeta{
            scriptId: scriptId,
            url: url,
            source: source,
            length: source.len,
        };
    }
}

pub struct DebuggerState {
    enabled: bool = false,
    scripts: [32]CompiledScriptMeta = [CompiledScriptMeta{}; 32],
    script_count: usize = 0,
    next_script_id: u64 = 1,
    skip_all_pauses: bool = false,
    breakpoints_active: bool = true,
    pause_on_exceptions: []const u8 = "none",
    async_call_stack_depth: u32 = 0,
    event_buf: [16384]u8 = [0; 16384],

    fn addScript(*_, meta: CompiledScriptMeta): bool {
        if (_.script_count >= _.scripts.len) return false;
        const idx = _.script_count;
        _.scripts[idx] = meta;
        _.scripts[idx].scriptId = std.heap.c_allocator.dupe(u8, meta.scriptId) catch return false;
        _.scripts[idx].url = std.heap.c_allocator.dupe(u8, meta.url) catch return false;
        _.scripts[idx].source = std.heap.c_allocator.dupe(u8, meta.source) catch return false;
        _.scripts[idx].hash = std.heap.c_allocator.dupe(u8, meta.hash) catch return false;
        _.script_count += 1;
        return true;
    }

    fn getScript(scriptId: []const u8): (*const CompiledScriptMeta)? {
        let i: usize = 0;
        while (i < _.script_count) : (i += 1) {
            if (std.mem.eql(u8, _.scripts[i].scriptId, scriptId)) {
                return &_.scripts[i];
            }
        }
        return null;
    }

    fn reset(*_) {
        _.enabled = false;
        _.script_count = 0;
        _.skip_all_pauses = false;
        _.breakpoints_active = true;
        _.pause_on_exceptions = "none";
        _.async_call_stack_depth = 0;
    }
}

fn escapeDebuggerJson(src: []const u8, dst: []u8): []const u8 {
    let out_idx: usize = 0;
    let i: usize = 0;
    while (i < src.len && out_idx + 2 <= dst.len) : (i += 1) {
        const c = src[i];
        if (c == '"') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = '"';
            out_idx += 2;
        } else if (c == '\\') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = '\\';
            out_idx += 2;
        } else if (c == '\n') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = 'n';
            out_idx += 2;
        } else if (c == '\r') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = 'r';
            out_idx += 2;
        } else if (c == '\t') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = 't';
            out_idx += 2;
        } else if (c >= 32 && c <= 126) {
            dst[out_idx] = c;
            out_idx += 1;
        }
    }
    return dst[0..out_idx];
}

pub fn fnv1aHex(s: []const u8, buf: *[8]u8): []const u8 {
    let h: u32 = 0x811c9dc5;
    for (c of s) {
        h = (h ^ @as(u32, c)) *% 16777619;
    }
    const hex_chars = "0123456789abcdef";
    let i: usize = 8;
    while (i > 0) : (i -= 1) {
        buf[i - 1] = hex_chars[@as(usize, h & 0x0f)];
        h >>= 4;
    }
    return buf[0..8];
}

pub fn makeCompiledScriptMeta(
    scriptId: []const u8,
    expression: []const u8,
    sourceURL: []const u8,
    isModule: bool,
    hash_buf: *[8]u8,
): CompiledScriptMeta {
    let lines: u32 = 0;
    let col: u32 = 0;
    for (c of expression) {
        if (c == '\n') {
            lines += 1;
            col = 0;
        } else {
            col += 1;
        }
    }
    const hash = fnv1aHex(expression, hash_buf);
    return CompiledScriptMeta{
        scriptId: scriptId,
        url: sourceURL,
        source: expression,
        startLine: 0,
        startColumn: 0,
        endLine: lines,
        endColumn: col,
        executionContextId: 1,
        hash: hash,
        isModule: isModule,
        length: expression.len,
        sourceMapURL: "",
        hasSourceURL: sourceURL.len > 0,
    };
}

pub fn emitScriptParsedEvent(buf: []u8, meta: *const CompiledScriptMeta, frameId: []const u8): CdpEvent {
    let fid = frameId;
    if (fid.len == 0) fid = "capsurf-frame-001";
    const has_src = if (meta.hasSourceURL || meta.url.len > 0) "true" else "false";
    const is_mod = if (meta.isModule) "true" else "false";
    const script_len = if (meta.length > 0) meta.length else meta.source.len;

    const json = std.fmt.bufPrint(
        buf,
        "{{\"scriptId\":\"{s}\",\"url\":\"{s}\",\"startLine\":{d},\"startColumn\":{d},\"endLine\":{d},\"endColumn\":{d},\"executionContextId\":{d},\"hash\":\"{s}\",\"buildId\":\"\",\"isLiveEdit\":false,\"sourceMapURL\":\"{s}\",\"hasSourceURL\":{s},\"isModule\":{s},\"length\":{d},\"executionContextAuxData\":{{\"isDefault\":true,\"type\":\"default\",\"frameId\":\"{s}\"}}}}",
        .{
            meta.scriptId,
            meta.url,
            meta.startLine,
            meta.startColumn,
            meta.endLine,
            meta.endColumn,
            meta.executionContextId,
            meta.hash,
            meta.sourceMapURL,
            has_src,
            is_mod,
            script_len,
            fid,
        },
    ) catch "{}";

    return CdpEvent.create("Debugger.scriptParsed", json);
}

pub fn hasDebuggerListeners(state: *const DebuggerState): bool {
    return state.enabled;
}

pub struct DebuggerHandler {
    static fn handle(
        session: *CdpPageSession,
        state: *DebuggerState,
        req: CdpRequest,
        out_buf: []u8,
        out_events: []CdpEvent,
        event_count: *usize,
    ): CdpResponse {
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "Debugger.enable")) {
            state.enabled = true;
            let i: usize = 0;
            while (i < state.script_count && event_count.* < out_events.len) : (i += 1) {
                const chunk_size = 512;
                if ((i + 1) * chunk_size <= state.event_buf.len) {
                    const evt_buf = state.event_buf[i * chunk_size .. (i + 1) * chunk_size];
                    out_events[event_count.*] = emitScriptParsedEvent(evt_buf, &state.scripts[i], session.frame_id);
                    event_count.* += 1;
                }
            }
            return CdpResponse.ok(req.id, "{\"debuggerId\":\"dbg-001\"}");
        }

        if (std.mem.eql(u8, req.method, "Debugger.disable")) {
            state.reset();
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Debugger.setSkipAllPauses")) {
            if (v := extractJsonBool(req.params_raw, "skip")) {
                state.skip_all_pauses = v;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Debugger.setBreakpointsActive")) {
            if (v := extractJsonBool(req.params_raw, "active")) {
                state.breakpoints_active = v;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Debugger.setPauseOnExceptions")) {
            if (v := extractJsonString(req.params_raw, "state")) {
                state.pause_on_exceptions = v;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Debugger.setAsyncCallStackDepth")) {
            if (v := extractJsonInt(req.params_raw, "maxDepth")) {
                state.async_call_stack_depth = if (v > 0) @as(u32, @intCast(v)) else 0;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Debugger.setBlackboxPatterns") ||
            std.mem.eql(u8, req.method, "Debugger.setBlackboxedRanges")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Debugger.getScriptSource")) {
            const sid = extractJsonString(req.params_raw, "scriptId") ?? "";
            let i: usize = 0;
            while (i < state.script_count) : (i += 1) {
                if (std.mem.eql(u8, state.scripts[i].scriptId, sid)) {
                    const prefix = "{\"scriptSource\":\"";
                    const suffix = "\"}";
                    if (out_buf.len < prefix.len + suffix.len) {
                        return CdpResponse.err(req.id, -32000, "Buffer overflow");
                    }
                    @memcpy(out_buf[0..prefix.len], prefix);
                    const body_dst = out_buf[prefix.len .. out_buf.len - suffix.len];
                    const esc = escapeDebuggerJson(state.scripts[i].source, body_dst);
                    const total_len = prefix.len + esc.len + suffix.len;
                    @memcpy(out_buf[prefix.len + esc.len .. total_len], suffix);
                    return CdpResponse.ok(req.id, out_buf[0..total_len]);
                }
            }
            return CdpResponse.ok(req.id, "{\"scriptSource\":\"\"}");
        }

        return CdpResponse.err(req.id, -32601, "'Debugger' domain method not implemented");
    }
}
