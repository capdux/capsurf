// Migrated from vendor/kitesurf/packages/engine/src/eval/script-bundle.ts
// and vendor/kitesurf/packages/engine/src/guards.ts
// Builds classic-script bundles, source map VLQ mappings, and script categorization for PageScript.

pub const INLINE_SCRIPT_BODY_KEY_PREFIX_CHARS: usize = 120;
pub const MAX_CLASSIC_SOURCE_MAP_LINES: usize = 50000;
pub const MAX_BUNDLE_DESCRIPTORS: usize = 64;

const VLQ_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

pub enum ScriptKind {
    regular,
    deferred,
    async_script,
    module_script,
    injected,
}

pub struct ScriptDescriptorEntry {
    src: []const u8 = "",
    body: []const u8 = "",
    kind: ScriptKind = ScriptKind.regular,
    async_flag: bool = false,
    defer_flag: bool = false,
    injected_flag: bool = false,
    dom_index: i32 = -1,
}

pub struct ScriptBundle {
    regular_count: usize = 0,
    deferred_count: usize = 0,
    async_count: usize = 0,
    module_count: usize = 0,
    injected_count: usize = 0,
    total_scripts: usize = 0,

    fn reset(*_) {
        _.regular_count = 0;
        _.deferred_count = 0;
        _.async_count = 0;
        _.module_count = 0;
        _.injected_count = 0;
        _.total_scripts = 0;
    }
}

pub fn countLines(s: []const u8): usize {
    let n: usize = 0;
    for (i in 0..s.len) {
        if (s[i] == '\n') {
            n += 1;
        }
    }
    return n;
}

pub fn encodeVLQ(value: i32, out_buf: []u8): []const u8 {
    let v: u32 = 0;
    if (value < 0) {
        let neg = @as(u32, @intCast(-value));
        v = (neg << 1) | 1;
    } else {
        v = @as(u32, @intCast(value)) << 1;
    }

    let pos: usize = 0;
    while (pos < out_buf.len) {
        let digit = v & 0x1f;
        v = v >> 5;
        if (v > 0) {
            digit = digit | 0x20;
        }
        out_buf[pos] = VLQ_ALPHABET[@as(usize, digit)];
        pos += 1;
        if (v == 0) break;
    }
    return out_buf[0..pos];
}

pub fn commentSafe(text: []const u8, out_buf: []u8): []const u8 {
    let pos: usize = 0;
    for (i in 0..text.len) {
        let ch = text[i];
        if (ch == '\n') {
            let enc = "%0A";
            if (pos + enc.len > out_buf.len) break;
            @memcpy(out_buf[pos .. pos + enc.len], enc);
            pos += enc.len;
        } else if (ch == '\r') {
            let enc = "%0D";
            if (pos + enc.len > out_buf.len) break;
            @memcpy(out_buf[pos .. pos + enc.len], enc);
            pos += enc.len;
        } else {
            if (pos >= out_buf.len) break;
            out_buf[pos] = ch;
            pos += 1;
        }
    }
    return out_buf[0..pos];
}

pub fn categorizeScript(desc: ScriptDescriptorEntry): ScriptKind {
    if (desc.injected_flag) {
        return ScriptKind.injected;
    }
    if (desc.kind == ScriptKind.module_script) {
        return ScriptKind.module_script;
    }
    if (desc.async_flag && desc.src.len > 0) {
        return ScriptKind.async_script;
    }
    if (desc.defer_flag && desc.src.len > 0) {
        return ScriptKind.deferred;
    }
    return ScriptKind.regular;
}

pub fn buildScriptBundle(
    descriptors: []const ScriptDescriptorEntry,
    bundle: *ScriptBundle,
) {
    bundle.reset();
    bundle.total_scripts = descriptors.len;

    for (d of descriptors) {
        let kind = categorizeScript(d);
        switch (kind) {
            ScriptKind.injected => {
                bundle.injected_count += 1;
            },
            ScriptKind.module_script => {
                bundle.module_count += 1;
            },
            ScriptKind.async_script => {
                bundle.async_count += 1;
            },
            ScriptKind.deferred => {
                bundle.deferred_count += 1;
            },
            ScriptKind.regular => {
                bundle.regular_count += 1;
            },
        }
    }
}

// Guard helpers
pub fn isOneOf(val: []const u8, allowed: []const []const u8): bool {
    for (a of allowed) {
        if (std.mem.eql(u8, a, val)) return true;
    }
    return false;
}

pub fn isNonEmptyString(s: []const u8): bool {
    return s.len > 0;
}
