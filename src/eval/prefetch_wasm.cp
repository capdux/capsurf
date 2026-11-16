// Migrated from vendor/kitesurf/packages/engine/src/eval/prefetch-wasm.ts
// Bounded WebAssembly prefetching and fingerprinting for page scripts.

pub const MAX_WASM_URLS: usize = 8;
pub const MAX_WASM_LITERAL_RESOLUTIONS: usize = 256;
pub const MAX_TOTAL_WASM_BYTES: usize = 48 * 1024 * 1024;
pub const PREFETCH_WASM_DEADLINE_MS: u64 = 10000;
pub const MAX_SINGLE_WASM_BYTES: usize = MAX_TOTAL_WASM_BYTES;
pub const TINY_MODULE_MAX_BYTES: usize = 64;
pub const MAX_INLINE_WASM_LABEL_PREFIX_CHARS: usize = 24;
pub const MAX_WASM_LITERAL_SPECIFIER_CHARS: usize = 512;
pub const MAX_BLOCKED_MESSAGE_BYTES: usize = 4 * 1024;
pub const INLINE_WASM_B64_PREFIX: []const u8 = "AGFzbQEAAAA";
pub const MIN_WASM_BYTES: usize = 8;

pub struct WasmBakeManifestEntry {
    size: usize = 0,
    sha256: [64]u8 = [0; 64],
    head16: [32]u8 = [0; 32],
    tail16: [32]u8 = [0; 32],
    head_len: usize = 0,
    tail_len: usize = 0,

    fn initFromBytes(*_, bytes: []const u8) {
        _.size = bytes.len;
        wasmModuleFingerprint(bytes, &_.sha256);

        const h_len = @min(bytes.len, 16);
        if (h_len > 0) {
            const enc = std.base64.standard.Encoder.encode(_.head16[0..], bytes[0..h_len]);
            _.head_len = enc.len;
        }

        const t_start = if (bytes.len > 16) bytes.len - 16 else 0;
        const t_slice = bytes[t_start..];
        if (t_slice.len > 0) {
            const enc = std.base64.standard.Encoder.encode(_.tail16[0..], t_slice);
            _.tail_len = enc.len;
        }
    }

    fn getSha256(*_): []const u8 {
        return _.sha256[0..64];
    }

    fn getHead16(*_): []const u8 {
        return _.head16[0.._.head_len];
    }

    fn getTail16(*_): []const u8 {
        return _.tail16[0.._.tail_len];
    }
}

pub fn wasmModuleFingerprint(bytes: []const u8, out_sha256: []u8) {
    let hasher = std.crypto.hash.sha2.Sha256.init(.{});
    hasher.update(bytes);
    let digest: [32]u8 = [0; 32];
    hasher.final(&digest);
    const hex = std.fmt.bytesToHex(digest, .lower);
    const copy_len = @min(out_sha256.len, 64);
    if (copy_len > 0) {
        @memcpy(out_sha256[0..copy_len], hex[0..copy_len]);
    }
}

pub fn inlineWasmKey(sha256: []const u8, out_buf: []u8): []const u8 {
    const prefix = "inline-wasm:sha256:";
    if (out_buf.len < prefix.len + sha256.len) return "";
    @memcpy(out_buf[0..prefix.len], prefix);
    @memcpy(out_buf[prefix.len .. prefix.len + sha256.len], sha256);
    return out_buf[0 .. prefix.len + sha256.len];
}

pub fn validateWasmHeader(bytes: []const u8): bool {
    if (bytes.len < MIN_WASM_BYTES) return false;
    return bytes[0] == 0x00 && bytes[1] == 0x61 && bytes[2] == 0x73 && bytes[3] == 0x6d &&
        bytes[4] == 0x01 && bytes[5] == 0x00 && bytes[6] == 0x00 && bytes[7] == 0x00;
}

fn isBase64Char(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/';
}

pub fn scanForInlineWasm(sources: []const []const u8, out_b64: [][]const u8): usize {
    if (out_b64.len == 0 || sources.len == 0) return 0;
    let count: usize = 0;

    let s_idx: usize = 0;
    while (s_idx < sources.len) : (s_idx += 1) {
        const body = sources[s_idx];
        if (body.len < INLINE_WASM_B64_PREFIX.len) continue;
        if (std.mem.indexOf(u8, body, INLINE_WASM_B64_PREFIX) == null) continue;

        let pos: usize = 0;
        while (pos + INLINE_WASM_B64_PREFIX.len <= body.len) {
            const hit_opt = std.mem.indexOf(u8, body[pos..], INLINE_WASM_B64_PREFIX);
            if (hit_opt == null) break;
            const prefix_start = pos + (hit_opt ?? 0);

            let end = prefix_start + INLINE_WASM_B64_PREFIX.len;
            while (end < body.len && isBase64Char(body[end])) : (end += 1) {}
            let pad: usize = 0;
            while (end < body.len && body[end] == '=' && pad < 2) : (end += 1) {
                pad += 1;
            }

            const lit = body[prefix_start..end];
            if (lit.len >= INLINE_WASM_B64_PREFIX.len) {
                let seen = false;
                let i: usize = 0;
                while (i < count) : (i += 1) {
                    if (std.mem.eql(u8, out_b64[i], lit)) {
                        seen = true;
                        break;
                    }
                }
                if (!seen) {
                    out_b64[count] = lit;
                    count += 1;
                    if (count >= out_b64.len || count >= MAX_WASM_URLS) return count;
                }
            }

            pos = end;
        }
    }
    return count;
}

pub fn scanForWasmLiterals(source: []const u8, out_urls: [][]const u8): usize {
    if (out_urls.len == 0 || source.len < 5) return 0;
    if (std.mem.indexOf(u8, source, ".wasm") == null) return 0;

    let count: usize = 0;
    let pos: usize = 0;

    while (pos + 5 <= source.len) {
        const hit_opt = std.mem.indexOf(u8, source[pos..], ".wasm");
        if (hit_opt == null) break;
        const dot_pos = pos + (hit_opt ?? 0);

        let quote_char: u8 = 0;
        let start: usize = dot_pos;
        let back: usize = dot_pos;

        while (back > 0) {
            back -= 1;
            const c = source[back];
            if (c == '\'' || c == '"' || c == '`') {
                quote_char = c;
                start = back + 1;
                break;
            }
            if (c <= ' ' || c == '\n' || c == '\r' || c == '\t') {
                break;
            }
            if (dot_pos - back > MAX_WASM_LITERAL_SPECIFIER_CHARS) {
                break;
            }
        }

        const end = dot_pos + 5;
        let matched = false;
        let lit_start: usize = 0;
        let lit_end: usize = 0;

        if (quote_char != 0 && end < source.len && source[end] == quote_char) {
            lit_start = start;
            lit_end = end;
            matched = true;
        } else if (quote_char == 0 && (end == source.len || source[end] <= ' ' || source[end] == ';' || source[end] == ',' || source[end] == ')')) {
            lit_start = if (back > 0) back + 1 else 0;
            lit_end = end;
            matched = true;
        }

        if (matched && lit_end > lit_start && (lit_end - lit_start) <= MAX_WASM_LITERAL_SPECIFIER_CHARS) {
            const lit = source[lit_start..lit_end];
            let seen = false;
            let i: usize = 0;
            while (i < count) : (i += 1) {
                if (std.mem.eql(u8, out_urls[i], lit)) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                out_urls[count] = lit;
                count += 1;
                if (count >= out_urls.len || count >= MAX_WASM_URLS) return count;
            }
        }

        pos = dot_pos + 5;
    }

    return count;
}
