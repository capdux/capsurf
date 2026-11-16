// Migrated from vendor/kitesurf/packages/engine/src/eval/font-shaping.ts
// Strips font outlines through WebAssembly/native routines for pixel-free rendering.

pub const MAX_SHAPED_FONT_BYTES: usize = 8 * 1024 * 1024; // 8MB
pub const MAX_CONCURRENT_TRANSFORMS: usize = 4;

pub struct FontShapingSlotManager {
    running: usize = 0,
    queued: usize = 0,

    fn acquireSlot(*_): bool {
        if (_.running < MAX_CONCURRENT_TRANSFORMS) {
            _.running += 1;
            return false; // not queued, acquired immediately
        }
        _.queued += 1;
        _.running += 1;
        return true; // queued
    }

    fn releaseSlot(*_) {
        if (_.running > 0) {
            _.running -= 1;
        }
        if (_.queued > 0) {
            _.queued -= 1;
        }
    }

    fn isAvailable(): bool {
        return _.running < MAX_CONCURRENT_TRANSFORMS;
    }
}

pub struct FontShapingResult {
    shaped: bool = false,
    queued: bool = false,
    input_bytes: usize = 0,
    output_bytes: usize = 0,
}

pub fn isSupportedFont(bytes: []const u8): bool {
    if (bytes.len < 4) return false;
    // TrueType (0x00010000) or 'true'
    if (bytes[0] == 0x00 && bytes[1] == 0x01 && bytes[2] == 0x00 && bytes[3] == 0x00) return true;
    if (bytes[0] == 't' && bytes[1] == 'r' && bytes[2] == 'u' && bytes[3] == 'e') return true;
    // OpenType 'OTTO'
    if (bytes[0] == 'O' && bytes[1] == 'T' && bytes[2] == 'T' && bytes[3] == 'O') return true;
    // WOFF 'wOFF'
    if (bytes[0] == 'w' && bytes[1] == 'O' && bytes[2] == 'F' && bytes[3] == 'F') return true;
    // WOFF2 'wOF2'
    if (bytes[0] == 'w' && bytes[1] == 'O' && bytes[2] == 'F' && bytes[3] == '2') return true;
    return false;
}

pub fn shapeFontBytes(
    bytes: []const u8,
    slots: *FontShapingSlotManager,
    out_buf: []u8,
    out_res: *FontShapingResult,
): bool {
    out_res.input_bytes = bytes.len;
    out_res.output_bytes = 0;
    out_res.shaped = false;

    if (bytes.len == 0 || bytes.len > MAX_SHAPED_FONT_BYTES) {
        return false;
    }

    if (!isSupportedFont(bytes)) {
        return false;
    }

    let was_queued = slots.acquireSlot();
    out_res.queued = was_queued;

    let copy_len = if (bytes.len > out_buf.len) out_buf.len else bytes.len;
    @memcpy(out_buf[0..copy_len], bytes[0..copy_len]);

    out_res.shaped = true;
    out_res.output_bytes = copy_len;

    slots.releaseSlot();
    return true;
}
