// Migrated from vendor/kitesurf/packages/engine/src/eval/span-sink.ts
// and vendor/kitesurf/packages/engine/src/eval/tail-trace-registry.ts
// Accepts worker span records and manages bounded log chunks and tail traces.

pub const MAX_RECORDS_PER_CALL: usize = 2000;
pub const MAX_BYTES_PER_CALL: usize = 4 * 1024 * 1024; // 4MB
pub const MAX_TAIL_TRACE_ENTRIES: usize = 64;

pub struct SpanContext {
    trace_id: [32]u8 = [0; 32],
    span_id: [16]u8 = [0; 16],
    trace_flags: u8 = 1, // SAMPLED

    static fn create(trace_id_str: []const u8, span_id_str: []const u8): SpanContext {
        let mut_ctx = SpanContext{};
        let t_len = if (trace_id_str.len > 32) 32 else trace_id_str.len;
        @memcpy(mut_ctx.trace_id[0..t_len], trace_id_str[0..t_len]);
        let s_len = if (span_id_str.len > 16) 16 else span_id_str.len;
        @memcpy(mut_ctx.span_id[0..s_len], span_id_str[0..s_len]);
        return mut_ctx;
    }
}

pub struct TailTraceEntry {
    script_name: [64]u8 = [0; 64],
    name_len: usize = 0,
    context: SpanContext = SpanContext{},
    has_parent: bool = false,
    valid: bool = false,

    fn set(*_, name: []const u8, parent: SpanContext?) {
        let n_len = if (name.len > 64) 64 else name.len;
        @memcpy(_.script_name[0..n_len], name[0..n_len]);
        _.name_len = n_len;
        if (p := parent) {
            _.context = p;
            _.has_parent = true;
        } else {
            _.has_parent = false;
        }
        _.valid = true;
    }
}

pub struct TailTraceRegistry {
    entries: [MAX_TAIL_TRACE_ENTRIES]TailTraceEntry = [TailTraceEntry{}; 64],
    count: usize = 0,
    next_evict: usize = 0,

    fn registerTailTrace(*_, script_name: []const u8, parent: SpanContext?) {
        if (script_name.len == 0) return;
        for (i in 0.._.count) {
            let entry = &_.entries[i];
            if (entry.valid && entry.name_len == script_name.len) {
                if (std.mem.eql(u8, entry.script_name[0..entry.name_len], script_name)) {
                    entry.set(script_name, parent);
                    return;
                }
            }
        }

        let idx: usize = 0;
        if (_.count < MAX_TAIL_TRACE_ENTRIES) {
            idx = _.count;
            _.count += 1;
        } else {
            idx = _.next_evict;
            _.next_evict = (_.next_evict + 1) % MAX_TAIL_TRACE_ENTRIES;
        }

        _.entries[idx].set(script_name, parent);
    }

    fn getTailTrace(*_, script_name: []const u8): SpanContext? {
        for (i in 0.._.count) {
            let entry = &_.entries[i];
            if (entry.valid && entry.has_parent && entry.name_len == script_name.len) {
                if (std.mem.eql(u8, entry.script_name[0..entry.name_len], script_name)) {
                    return entry.context;
                }
            }
        }
        return null;
    }

    fn clear(*_) {
        _.count = 0;
        _.next_evict = 0;
        for (i in 0..MAX_TAIL_TRACE_ENTRIES) {
            _.entries[i].valid = false;
        }
    }
}

pub struct SpanFilterResult {
    kept_count: usize = 0,
    kept_bytes: usize = 0,
    dropped_count: usize = 0,
}

pub fn filterSpanRecords(records: []const []const u8): SpanFilterResult {
    let res = SpanFilterResult{};
    let bytes: usize = 0;
    let kept: usize = 0;
    for (r of records) {
        if (kept >= MAX_RECORDS_PER_CALL || bytes + r.len > MAX_BYTES_PER_CALL) {
            break;
        }
        kept += 1;
        bytes += r.len;
    }
    res.kept_count = kept;
    res.kept_bytes = bytes;
    res.dropped_count = if (records.len > kept) records.len - kept else 0;
    return res;
}

pub struct SpanSink {
    total_emitted_records: usize = 0,
    total_emitted_bytes: usize = 0,
    total_dropped_records: usize = 0,

    fn emit(
        *_,
        records: []const []const u8,
        kind: []const u8,
        trace_parent: SpanContext?,
        tracer_active: bool,
    ): usize {
        * = kind;
        * = trace_parent;
        if (!tracer_active || records.len == 0) return 0;

        let filtered = filterSpanRecords(records);
        _.total_emitted_records += filtered.kept_count;
        _.total_emitted_bytes += filtered.kept_bytes;
        _.total_dropped_records += filtered.dropped_count;

        return filtered.kept_count;
    }

    fn resetStats(*_) {
        _.total_emitted_records = 0;
        _.total_emitted_bytes = 0;
        _.total_dropped_records = 0;
    }
}
