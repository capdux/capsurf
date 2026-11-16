// Migrated from vendor/kitesurf/packages/engine/src/cdp/scene-host.ts
// Validates scene-host metadata and serves range-checked WebAssembly chunks over CDP.

pub const CHUNK_BYTES: usize = 4 * 1024 * 1024;
pub const MANIFEST_BYTES: usize = 16 * 1024;
pub const HOST_BYTES: usize = 32 * 1024 * 1024;

pub struct SceneHostManifest {
    abi: u32 = 1,
    wire_version: u32 = 1,
    wasm_name: []const u8 = "scene_host.wasm",
    sha256: []const u8 = "",
    total_bytes: usize = 1024 * 1024, // 1MB default artifact
    retained_frames: bool = true,
}

pub fn validateRange(offset: usize, length: usize, total_bytes: usize): bool {
    if (offset >= total_bytes) return false;
    if (length == 0 || length > CHUNK_BYTES) return false;
    return true;
}

pub fn formatManifestJson(m: SceneHostManifest, out_buf: []u8): []const u8 {
    let prefix = "{\"abi\":1,\"wireVersion\":1,\"wasm\":\"scene_host.wasm\",\"sha256\":\"";
    let mid = "\",\"bytes\":";
    let suffix = ",\"retainedFrames\":true}";

    let pos: usize = 0;
    if (pos + prefix.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + prefix.len], prefix);
    pos += prefix.len;

    if (pos + m.sha256.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + m.sha256.len], m.sha256);
    pos += m.sha256.len;

    if (pos + mid.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + mid.len], mid);
    pos += mid.len;

    let b_buf: [20]u8 = [0; 20];
    let b_str = formatUsize(&b_buf, m.total_bytes);
    if (pos + b_str.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + b_str.len], b_str);
    pos += b_str.len;

    if (pos + suffix.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + suffix.len], suffix);
    pos += suffix.len;

    return out_buf[0..pos];
}

pub fn formatChunkResponse(
    sha256: []const u8,
    offset: usize,
    total_bytes: usize,
    data_b64: []const u8,
    out_buf: []u8,
): []const u8 {
    let p1 = "{\"sha256\":\"";
    let p2 = "\",\"offset\":";
    let p3 = ",\"totalBytes\":";
    let p4 = ",\"dataB64\":\"";
    let p5 = "\"}";

    let pos: usize = 0;
    if (pos + p1.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + p1.len], p1);
    pos += p1.len;

    if (pos + sha256.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + sha256.len], sha256);
    pos += sha256.len;

    if (pos + p2.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + p2.len], p2);
    pos += p2.len;

    let off_buf: [20]u8 = [0; 20];
    let off_str = formatUsize(&off_buf, offset);
    if (pos + off_str.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + off_str.len], off_str);
    pos += off_str.len;

    if (pos + p3.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + p3.len], p3);
    pos += p3.len;

    let tot_buf: [20]u8 = [0; 20];
    let tot_str = formatUsize(&tot_buf, total_bytes);
    if (pos + tot_str.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + tot_str.len], tot_str);
    pos += tot_str.len;

    if (pos + p4.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + p4.len], p4);
    pos += p4.len;

    if (pos + data_b64.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + data_b64.len], data_b64);
    pos += data_b64.len;

    if (pos + p5.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + p5.len], p5);
    pos += p5.len;

    return out_buf[0..pos];
}

fn formatUsize(buf: *[20]u8, val: usize): []const u8 {
    if (val == 0) {
        buf[0] = '0';
        return buf[0..1];
    }
    let v = val;
    let temp: [20]u8 = [0; 20];
    let digits: usize = 0;
    while (v > 0) {
        temp[digits] = @as(u8, @intCast(v % 10)) + '0';
        v = v / 10;
        digits += 1;
    }
    let j: usize = 0;
    while (j < digits) : (j += 1) {
        buf[j] = temp[digits - 1 - j];
    }
    return buf[0..digits];
}
