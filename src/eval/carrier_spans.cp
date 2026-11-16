// Migrated from vendor/kitesurf/packages/engine/src/eval/carrier-spans.ts
// and vendor/kitesurf/packages/engine/src/eval/attribution-tail.ts
// Splits worker logs into byte-bounded tracing spans and manages CPU attribution tails.

pub const CARRIER_SPAN_BYTE_BUDGET: usize = 48 * 1024; // 48KB
pub const CARRIER_SPAN_LOG_CAP: usize = 1000;

pub struct CarrierLog {
    key: []const u8 = "",
    value: []const u8 = "",

    fn size(): usize {
        return _.key.len + _.value.len;
    }
}

pub struct AttachChunkedResult {
    attached: usize = 0,
    oversized: usize = 0,
    chunks: usize = 0,
}

pub fn attachChunked(logs: []const CarrierLog): AttachChunkedResult {
    let res = AttachChunkedResult{};
    let i: usize = 0;
    while (i < logs.len) {
        let chunk_count: usize = 0;
        let bytes: usize = 0;
        while (i < logs.len && chunk_count < CARRIER_SPAN_LOG_CAP) {
            let log = logs[i];
            let sz = log.size();
            if (sz > CARRIER_SPAN_BYTE_BUDGET) {
                res.oversized += 1;
                i += 1;
                continue;
            }
            if (bytes + sz > CARRIER_SPAN_BYTE_BUDGET) {
                break;
            }
            bytes += sz;
            chunk_count += 1;
            i += 1;
        }
        if (chunk_count == 0) {
            // All remaining or current was oversized
            continue;
        }
        res.chunks += 1;
        res.attached += chunk_count;
    }
    return res;
}

pub struct AttributionTailProps {
    headers: []const u8 = "",
    script_name: []const u8 = "",
}

pub struct AttributionTailResult {
    is_active: bool = false,
    has_headers: bool = false,
}

pub fn checkAttributionTail(headers: []const u8, script_name: []const u8): AttributionTailResult {
    let res = AttributionTailResult{};
    if (headers.len > 0 && script_name.len > 0) {
        res.is_active = true;
        res.has_headers = true;
    }
    return res;
}
