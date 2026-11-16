// Migrated from vendor/kitesurf/packages/engine/src/engine-errors.ts
// Classifies engine exceptions and validates token-protected PageScript reports.

pub enum ErrorOrigin {
    page,
    wasm,
    shim,
    runtime,
    unknown,

    fn asStr(): []const u8 {
        return switch (_) {
            .page => "page",
            .wasm => "wasm",
            .shim => "shim",
            .runtime => "runtime",
            .unknown => "unknown",
        };
    }

    static fn fromStr(s: []const u8): ErrorOrigin {
        if (std.mem.eql(u8, s, "page")) return .page;
        if (std.mem.eql(u8, s, "wasm")) return .wasm;
        if (std.mem.eql(u8, s, "shim")) return .shim;
        if (std.mem.eql(u8, s, "runtime")) return .runtime;
        return .unknown;
    }
}

pub struct TailExceptionReport {
    script_name: []const u8 = "",
    name: []const u8 = "Error",
    message: []const u8 = "",
    stack: []const u8 = "",
    origin: ErrorOrigin = .unknown,
    page_url: []const u8 = "",
    session_id: []const u8 = "",

    fn isEngineError(): bool {
        return switch (_.origin) {
            .wasm, .shim, .runtime => true,
            * => false,
        };
    }
}

fn isWordChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

fn containsPageJs(stack: []const u8): bool {
    const target = "page.js:";
    if (stack.len < target.len) return false;
    let pos: usize = 0;
    while (pos + target.len <= stack.len) {
        const hit_opt = std.mem.indexOf(u8, stack[pos..], target);
        if (hit_opt == null) return false;
        const idx = pos + (hit_opt ?? 0);
        if (idx == 0 || !isWordChar(stack[idx - 1])) {
            return true;
        }
        pos = idx + 1;
    }
    return false;
}

pub fn classifyOrigin(name: []const u8, message: []const u8, stack: []const u8): ErrorOrigin {
    // Page bundle frames identify exceptions that escaped page-error handling.
    if (std.mem.indexOf(u8, stack, "__ks_user_") != null) return .page;

    if (std.mem.eql(u8, name, "RuntimeError") ||
        std.mem.indexOf(u8, message, "panicked at") != null ||
        std.mem.indexOf(u8, message, "unreachable") != null)
    {
        return .wasm;
    }

    if (std.mem.indexOf(u8, stack, "core_bg.wasm") != null ||
        std.mem.indexOf(u8, stack, "wasm-glue.js") != null)
    {
        return .wasm;
    }

    if (std.mem.indexOf(u8, stack, "dom-shim.js") != null) return .shim;

    if (containsPageJs(stack) || std.mem.indexOf(u8, stack, "(page.js") != null) {
        return .runtime;
    }

    return .unknown;
}

fn extractJsonString(json: []const u8, key: []const u8): ([]const u8)? {
    let key_pat_buf: [64]u8 = [0; 64];
    if (key.len + 3 > key_pat_buf.len) return null;
    key_pat_buf[0] = '"';
    @memcpy(key_pat_buf[1 .. 1 + key.len], key);
    key_pat_buf[1 + key.len] = '"';
    const key_pat = key_pat_buf[0 .. 2 + key.len];

    const pos_opt = std.mem.indexOf(u8, json, key_pat);
    if (pos_opt == null) return null;
    const pos = pos_opt ?? 0;
    let idx = pos + key_pat.len;

    while (idx < json.len && (json[idx] == ' ' || json[idx] == ':' || json[idx] == '\t' || json[idx] == '\r' || json[idx] == '\n')) : (idx += 1) {}
    if (idx >= json.len || json[idx] != '"') return null;
    idx += 1;

    const start = idx;
    while (idx < json.len) : (idx += 1) {
        if (json[idx] == '\\') {
            idx += 1;
            continue;
        }
        if (json[idx] == '"') {
            return json[start..idx];
        }
    }
    return null;
}

pub fn parseKsErrLine(line: []const u8, expected_token: []const u8): TailExceptionReport? {
    if (expected_token.len == 0) return null;
    const prefix = "[ks-err] ";
    if (!std.mem.startsWith(u8, line, prefix)) return null;

    const json = line[prefix.len..];
    const tok_opt = extractJsonString(json, "token");
    if (tok_opt == null) return null;
    const tok = tok_opt ?? "";
    if (!std.mem.eql(u8, tok, expected_token)) return null;

    const msg_opt = extractJsonString(json, "message");
    if (msg_opt == null) return null;
    let msg = msg_opt ?? "";

    const orig_opt = extractJsonString(json, "origin");
    if (orig_opt == null) return null;
    const orig_str = orig_opt ?? "";

    let origin: ErrorOrigin = .unknown;
    if (std.mem.eql(u8, orig_str, "wasm")) {
        origin = .wasm;
    } else if (std.mem.eql(u8, orig_str, "shim")) {
        origin = .shim;
    } else if (std.mem.eql(u8, orig_str, "runtime")) {
        origin = .runtime;
    } else {
        return null;
    }

    let name: []const u8 = "Error";
    if (n_opt := extractJsonString(json, "name")) {
        if (n_opt.len > 0) {
            name = n_opt;
        }
    }
    if (name.len > 64) {
        name = name[0..64];
    }

    if (msg.len > 1024) {
        msg = msg[0..1024];
    }

    let stack: []const u8 = "";
    if (s_opt := extractJsonString(json, "stack")) {
        stack = if (s_opt.len > 8192) s_opt[0..8192] else s_opt;
    }

    let page_url: []const u8 = "";
    if (u_opt := extractJsonString(json, "pageUrl")) {
        page_url = if (u_opt.len > 512) u_opt[0..512] else u_opt;
    } else if (u2_opt := extractJsonString(json, "page_url")) {
        page_url = if (u2_opt.len > 512) u2_opt[0..512] else u2_opt;
    }

    let session_id: []const u8 = "";
    if (sid_opt := extractJsonString(json, "sessionId")) {
        session_id = sid_opt;
    } else if (sid2_opt := extractJsonString(json, "session_id")) {
        session_id = sid2_opt;
    }

    let script_name: []const u8 = "";
    if (sn_opt := extractJsonString(json, "scriptName")) {
        script_name = sn_opt;
    } else if (sn2_opt := extractJsonString(json, "script_name")) {
        script_name = sn2_opt;
    }

    return TailExceptionReport{
        script_name: script_name,
        name: name,
        message: msg,
        stack: stack,
        origin: origin,
        page_url: page_url,
        session_id: session_id,
    };
}
