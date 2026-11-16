// Migrated from vendor/kitesurf/packages/engine/src/rbi-protocol.ts
// Binary RBI vector stream & resource batch codec for CapSurf

pub const RBI_BATCH_HEAD: usize = 4;
pub const RBI_ENTRY_HEAD: usize = 13;
pub const RBI_ENTRY_ID_OFFSET: usize = 1;
pub const RBI_ENTRY_LENGTH_OFFSET: usize = 9;

// Kinds
pub const RBI_KIND_GLYPH_PATH: u8 = 0;
pub const RBI_KIND_FONT_METRICS: u8 = 1;
pub const RBI_KIND_IMAGE: u8 = 2;
pub const RBI_KIND_FONT_DATA: u8 = 3;

// Image Formats
pub const IMAGE_FORMAT_ENCODED: u8 = 2;
pub const IMAGE_FORMAT_ENGINE_REF: u8 = 3;
pub const IMAGE_REF_PAYLOAD: usize = 17;
pub const IMAGE_REF_FORMAT_OFFSET: usize = 8;

// Font References
pub const FONT_DATA_REF_BLOB: u8 = 0;
pub const FONT_DATA_REF_ASSET: u8 = 1;
pub const FONT_ENTRY_HEAD: usize = 5;

// Missing Resource List
pub const MISSING_RESOURCE_LIST_HEADER_BYTES: usize = 4;
pub const MISSING_RESOURCE_LIST_KIND_BYTES: usize = 1;
pub const MISSING_RESOURCE_LIST_ENTRY_BYTES: usize = 9;
pub const MAX_MISSING_RESOURCE_LIST_B64_CHARS: usize = 64 * 1024;

// Capacity
pub const DELIVERED_CAP: usize = 128;

pub struct RbiResourceEntry {
    kind: u8 = 0,
    hash: u64 = 0,
    length: u32 = 0,
    payload: []const u8 = [],
}

pub struct RbiMissingResource {
    kind: u8 = 0,
    hash: u64 = 0,
}

pub struct DeliveredKeys {
    slots: [DELIVERED_CAP]u64 = [0; DELIVERED_CAP],
    count: usize = 0,
    head: usize = 0,

    static fn init(): DeliveredKeys {
        return DeliveredKeys {
            slots: [0; DELIVERED_CAP],
            count: 0,
            head: 0,
        };
    }

    fn contains(hash: u64): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.slots[i] == hash) return true;
        }
        return false;
    }

    fn add(*_, hash: u64) {
        if (_.contains(hash)) return;

        if (_.count < DELIVERED_CAP) {
            _.slots[_.count] = hash;
            _.count += 1;
        } else {
            _.slots[_.head] = hash;
            _.head = (_.head + 1) % DELIVERED_CAP;
        }
    }

    fn clear(*_) {
        _.count = 0;
        _.head = 0;
        let i: usize = 0;
        while (i < DELIVERED_CAP) : (i += 1) {
            _.slots[i] = 0;
        }
    }

    fn size(): usize {
        return _.count;
    }

    fn isEmpty(): bool {
        return _.count == 0;
    }
}

pub fn calculateBatchSize(entries: []const RbiResourceEntry): usize {
    let total: usize = RBI_BATCH_HEAD;
    let i: usize = 0;
    while (i < entries.len) : (i += 1) {
        const payload_len = entries[i].payload.len;
        total += RBI_ENTRY_HEAD + payload_len;
    }
    return total;
}

pub fn encodeResourceBatch(entries: []const RbiResourceEntry, out_buf: []u8): usize {
    if (out_buf.len < RBI_BATCH_HEAD) return 0;

    let offset: usize = RBI_BATCH_HEAD;
    let encoded_count: u32 = 0;

    let i: usize = 0;
    while (i < entries.len) : (i += 1) {
        const entry = entries[i];
        const payload_len = entry.payload.len;
        if (offset + RBI_ENTRY_HEAD + payload_len > out_buf.len) {
            break;
        }

        out_buf[offset] = entry.kind;
        std.mem.writeInt(u64, out_buf[offset + RBI_ENTRY_ID_OFFSET .. offset + RBI_ENTRY_ID_OFFSET + 8][0..8], entry.hash, .little);
        const len_u32: u32 = if (payload_len > 0) @as(u32, @truncate(payload_len)) else entry.length;
        std.mem.writeInt(u32, out_buf[offset + RBI_ENTRY_LENGTH_OFFSET .. offset + RBI_ENTRY_LENGTH_OFFSET + 4][0..4], len_u32, .little);

        if (payload_len > 0) {
            @memcpy(out_buf[offset + RBI_ENTRY_HEAD .. offset + RBI_ENTRY_HEAD + payload_len], entry.payload);
        }

        offset += RBI_ENTRY_HEAD + payload_len;
        encoded_count += 1;
    }

    std.mem.writeInt(u32, out_buf[0..RBI_BATCH_HEAD][0..4], encoded_count, .little);
    return offset;
}

pub fn decodeResourceBatch(buf: []const u8, out_entries: []RbiResourceEntry): usize {
    if (buf.len < RBI_BATCH_HEAD) return 0;

    const count = std.mem.readInt(u32, buf[0..RBI_BATCH_HEAD][0..4], .little);
    let p: usize = RBI_BATCH_HEAD;
    let decoded_count: usize = 0;

    let n: u32 = 0;
    while (n < count && p + RBI_ENTRY_HEAD <= buf.len && decoded_count < out_entries.len) : (n += 1) {
        const kind = buf[p];
        const hash = std.mem.readInt(u64, buf[p + RBI_ENTRY_ID_OFFSET .. p + RBI_ENTRY_ID_OFFSET + 8][0..8], .little);
        const len = std.mem.readInt(u32, buf[p + RBI_ENTRY_LENGTH_OFFSET .. p + RBI_ENTRY_LENGTH_OFFSET + 4][0..4], .little);

        const payload_start = p + RBI_ENTRY_HEAD;
        const payload_end = payload_start + len;
        if (payload_end > buf.len) {
            break;
        }

        out_entries[decoded_count] = RbiResourceEntry {
            kind: kind,
            hash: hash,
            length: len,
            payload: buf[payload_start..payload_end],
        };
        decoded_count += 1;
        p = payload_end;
    }

    return decoded_count;
}

pub fn dropAlreadySent(batch: []const u8, sent: *const DeliveredKeys, out_buf: []u8): usize {
    if (batch.len < RBI_BATCH_HEAD || out_buf.len < RBI_BATCH_HEAD) return 0;

    const count = std.mem.readInt(u32, batch[0..RBI_BATCH_HEAD][0..4], .little);
    let p: usize = RBI_BATCH_HEAD;
    let out_p: usize = RBI_BATCH_HEAD;
    let kept_count: u32 = 0;

    let n: u32 = 0;
    while (n < count && p + RBI_ENTRY_HEAD <= batch.len) : (n += 1) {
        const hash = std.mem.readInt(u64, batch[p + RBI_ENTRY_ID_OFFSET .. p + RBI_ENTRY_ID_OFFSET + 8][0..8], .little);
        const len = std.mem.readInt(u32, batch[p + RBI_ENTRY_LENGTH_OFFSET .. p + RBI_ENTRY_LENGTH_OFFSET + 4][0..4], .little);
        const entry_end = p + RBI_ENTRY_HEAD + len;
        if (entry_end > batch.len) break;

        const entry_size = entry_end - p;
        if (!sent.contains(hash)) {
            if (out_p + entry_size > out_buf.len) break;
            @memcpy(out_buf[out_p .. out_p + entry_size], batch[p..entry_end]);
            out_p += entry_size;
            kept_count += 1;
        }
        p = entry_end;
    }

    std.mem.writeInt(u32, out_buf[0..RBI_BATCH_HEAD][0..4], kept_count, .little);
    return out_p;
}

pub fn noteDelivered(batch: []const u8, sent: *DeliveredKeys) {
    if (batch.len < RBI_BATCH_HEAD) return;

    const count = std.mem.readInt(u32, batch[0..RBI_BATCH_HEAD][0..4], .little);
    let p: usize = RBI_BATCH_HEAD;

    let n: u32 = 0;
    while (n < count && p + RBI_ENTRY_HEAD <= batch.len) : (n += 1) {
        const kind = batch[p];
        const hash = std.mem.readInt(u64, batch[p + RBI_ENTRY_ID_OFFSET .. p + RBI_ENTRY_ID_OFFSET + 8][0..8], .little);
        const len = std.mem.readInt(u32, batch[p + RBI_ENTRY_LENGTH_OFFSET .. p + RBI_ENTRY_LENGTH_OFFSET + 4][0..4], .little);
        const payload_start = p + RBI_ENTRY_HEAD;
        const entry_end = payload_start + len;
        if (entry_end > batch.len) break;

        let is_ref = false;
        if (kind == RBI_KIND_IMAGE && len == IMAGE_REF_PAYLOAD) {
            if (batch[payload_start + IMAGE_REF_FORMAT_OFFSET] == IMAGE_FORMAT_ENGINE_REF) {
                is_ref = true;
            }
        } else if (kind == RBI_KIND_FONT_DATA && len > FONT_ENTRY_HEAD) {
            if (batch[payload_start] == FONT_DATA_REF_ASSET) {
                is_ref = true;
            }
        }

        if (!is_ref) {
            sent.add(hash);
        }

        p = entry_end;
    }
}

pub fn encodeMissingResources(items: []const RbiMissingResource, out_buf: []u8): usize {
    if (out_buf.len < MISSING_RESOURCE_LIST_HEADER_BYTES) return 0;

    let offset: usize = MISSING_RESOURCE_LIST_HEADER_BYTES;
    let count: u32 = 0;

    let i: usize = 0;
    while (i < items.len) : (i += 1) {
        if (offset + MISSING_RESOURCE_LIST_ENTRY_BYTES > out_buf.len) break;
        out_buf[offset] = items[i].kind;
        std.mem.writeInt(u64, out_buf[offset + 1 .. offset + 9][0..8], items[i].hash, .little);
        offset += MISSING_RESOURCE_LIST_ENTRY_BYTES;
        count += 1;
    }

    std.mem.writeInt(u32, out_buf[0..MISSING_RESOURCE_LIST_HEADER_BYTES][0..4], count, .little);
    return offset;
}

pub fn decodeMissingResources(buf: []const u8, out_items: []RbiMissingResource): usize {
    if (buf.len < MISSING_RESOURCE_LIST_HEADER_BYTES) return 0;

    const count = std.mem.readInt(u32, buf[0..MISSING_RESOURCE_LIST_HEADER_BYTES][0..4], .little);
    let p: usize = MISSING_RESOURCE_LIST_HEADER_BYTES;
    let decoded_count: usize = 0;

    let n: u32 = 0;
    while (n < count && p + MISSING_RESOURCE_LIST_ENTRY_BYTES <= buf.len && decoded_count < out_items.len) : (n += 1) {
        const kind = buf[p];
        const hash = std.mem.readInt(u64, buf[p + 1 .. p + 9][0..8], .little);
        out_items[decoded_count] = RbiMissingResource {
            kind: kind,
            hash: hash,
        };
        decoded_count += 1;
        p += MISSING_RESOURCE_LIST_ENTRY_BYTES;
    }

    return decoded_count;
}
