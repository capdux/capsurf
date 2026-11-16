// Migrated from vendor/kitesurf/packages/engine/src/cdp/network-events.ts
// Tracks request lifecycles and buffered response bodies for CDP Network sessions.

pub const MAX_RETAINED_BODY_BYTES: usize = 16 * 1024 * 1024;
pub const RESPONSE_BODY_CAP: usize = 4 * 1024 * 1024;
pub const MAX_RETAINED_REQUESTS: usize = 512;
pub const MAX_SESSIONS: usize = 32;
pub const MAX_ATTRIBUTIONS: usize = 32;
pub const REQUEST_ID_CONNECTION_PREFIX_CHARS: usize = 8;
pub const MILLISECONDS_PER_SECOND: u64 = 1000;
pub const SECONDS_PER_DAY: u64 = 86400;
pub const SYNTHETIC_CERTIFICATE_VALIDITY_DAYS: u64 = 30;

pub struct RequestRecord {
    request_id: []const u8 = "",
    url: []const u8 = "",
    method: []const u8 = "GET",
    resource_type: []const u8 = "Other",
    status: u32 = 0,
    mime_type: []const u8 = "",
    headers_raw: []const u8 = "",
    body: []const u8 = "",
    body_is_base64: bool = false,
    body_bytes: usize = 0,
    in_flight: bool = false,
    completed: bool = false,
    frame_id: []const u8 = "",
    loader_id: []const u8 = "",
    session_id: []const u8 = "",
    encoded_data_length: usize = 0,
    error_text: []const u8 = "",
    canceled: bool = false,
    has_body: bool = false,
    active: bool = false,
}

pub struct SessionBuffer {
    session_id: [64]u8 = [0; 64],
    session_id_len: usize = 0,
    max_total_buffer_size: usize = MAX_RETAINED_BODY_BYTES,
    max_resource_buffer_size: usize = RESPONSE_BODY_CAP,
    active: bool = false,

    fn getSessionId(*_): []const u8 {
        return _.session_id[0.._.session_id_len];
    }
}

pub struct TargetAttribution {
    frame_id: [64]u8 = [0; 64],
    frame_id_len: usize = 0,
    loader_id: [64]u8 = [0; 64],
    loader_id_len: usize = 0,
    session_id: [64]u8 = [0; 64],
    session_id_len: usize = 0,
    active: bool = false,

    fn getFrameId(*_): []const u8 {
        return _.frame_id[0.._.frame_id_len];
    }

    fn getLoaderId(*_): []const u8 {
        return _.loader_id[0.._.loader_id_len];
    }

    fn getSessionId(*_): []const u8 {
        return _.session_id[0.._.session_id_len];
    }
}

pub fn cleanMimeType(buf: []u8, raw: []const u8): []const u8 {
    let end: usize = raw.len;
    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        if (raw[i] == ';') {
            end = i;
            break;
        }
    }
    const part = raw[0..end];
    let start: usize = 0;
    while (start < part.len && (part[start] == ' ' || part[start] == '\t' || part[start] == '\r' || part[start] == '\n')) : (start += 1) {}
    let finish: usize = part.len;
    while (finish > start && (part[finish - 1] == ' ' || part[finish - 1] == '\t' || part[finish - 1] == '\r' || part[finish - 1] == '\n')) : (finish -= 1) {}
    const trimmed = part[start..finish];
    const out_len = @min(trimmed.len, buf.len);
    let j: usize = 0;
    while (j < out_len) : (j += 1) {
        buf[j] = std.ascii.toLower(trimmed[j]);
    }
    return buf[0..out_len];
}

pub struct NetworkEventTracker {
    records: [MAX_RETAINED_REQUESTS]RequestRecord = [RequestRecord{}; MAX_RETAINED_REQUESTS],
    order: [MAX_RETAINED_REQUESTS]usize = [0; MAX_RETAINED_REQUESTS],
    order_head: usize = 0,
    order_count: usize = 0,
    record_count: usize = 0,
    sessions: [MAX_SESSIONS]SessionBuffer = [SessionBuffer{}; MAX_SESSIONS],
    session_count: usize = 0,
    attributions: [MAX_ATTRIBUTIONS]TargetAttribution = [TargetAttribution{}; MAX_ATTRIBUTIONS],
    attribution_count: usize = 0,
    connection_id: []const u8 = "conn0",
    next_seq: u64 = 1,
    retained_body_bytes_total: usize = 0,
    effective_total_bytes: usize = MAX_RETAINED_BODY_BYTES,
    effective_per_body_bytes: usize = RESPONSE_BODY_CAP,
    in_flight_count: usize = 0,
    disposed: bool = false,

    static fn init(): NetworkEventTracker {
        return NetworkEventTracker{};
    }

    fn setConnectionId(*_, conn_id: []const u8) {
        _.connection_id = conn_id;
    }

    fn setEnabled(*_, session_id: []const u8, on: bool, max_total_buf: usize, max_resource_buf: usize) {
        if (on) {
            let slot: usize = MAX_SESSIONS;
            let i: usize = 0;
            while (i < MAX_SESSIONS) : (i += 1) {
                if (_.sessions[i].active && std.mem.eql(u8, _.sessions[i].session_id[0.._.sessions[i].session_id_len], session_id)) {
                    slot = i;
                    break;
                }
                if (!_.sessions[i].active && slot == MAX_SESSIONS) {
                    slot = i;
                }
            }
            if (slot < MAX_SESSIONS) {
                let s = &_.sessions[slot];
                const slen = @min(session_id.len, 64);
                @memcpy(s.session_id[0..slen], session_id[0..slen]);
                s.session_id_len = slen;
                s.max_total_buffer_size = if (max_total_buf > 0) @min(max_total_buf, MAX_RETAINED_BODY_BYTES) else MAX_RETAINED_BODY_BYTES;
                s.max_resource_buffer_size = if (max_resource_buf > 0) @min(max_resource_buf, RESPONSE_BODY_CAP) else RESPONSE_BODY_CAP;
                if (!s.active) {
                    s.active = true;
                    _.session_count += 1;
                }
            }
        } else {
            let i: usize = 0;
            while (i < MAX_SESSIONS) : (i += 1) {
                if (_.sessions[i].active && std.mem.eql(u8, _.sessions[i].session_id[0.._.sessions[i].session_id_len], session_id)) {
                    _.sessions[i].active = false;
                    _.sessions[i].session_id_len = 0;
                    if (_.session_count > 0) _.session_count -= 1;
                    break;
                }
            }
        }
        _.recomputeBufferLimits();
    }

    fn isEnabled(session_id: []const u8): bool {
        let i: usize = 0;
        while (i < MAX_SESSIONS) : (i += 1) {
            if (_.sessions[i].active && std.mem.eql(u8, _.sessions[i].session_id[0.._.sessions[i].session_id_len], session_id)) {
                return true;
            }
        }
        return false;
    }

    fn recomputeBufferLimits(*_) {
        if (_.session_count == 0) {
            _.effective_total_bytes = MAX_RETAINED_BODY_BYTES;
            _.effective_per_body_bytes = RESPONSE_BODY_CAP;
            _.shedBodiesOverBudget();
            return;
        }
        let total: usize = 0;
        let per_body: usize = 0;
        let i: usize = 0;
        while (i < MAX_SESSIONS) : (i += 1) {
            if (_.sessions[i].active) {
                total = @max(total, _.sessions[i].max_total_buffer_size);
                per_body = @max(per_body, _.sessions[i].max_resource_buffer_size);
            }
        }
        _.effective_total_bytes = total;
        _.effective_per_body_bytes = per_body;
        _.shedBodiesOverBudget();
    }

    fn shedBodiesOverBudget(*_) {
        let i: usize = 0;
        while (i < _.order_count && _.retained_body_bytes_total > _.effective_total_bytes) : (i += 1) {
            const idx = (_.order_head + i) % MAX_RETAINED_REQUESTS;
            const slot = _.order[idx];
            if (slot < MAX_RETAINED_REQUESTS && _.records[slot].active && _.records[slot].has_body) {
                _.dropBody(&_.records[slot]);
            }
        }
    }

    fn storeBody(*_, rec: *RequestRecord, body: []const u8, is_base64: bool) {
        if (_.disposed || _.effective_total_bytes == 0) return;
        _.dropBody(rec);
        rec.body = body;
        rec.body_is_base64 = is_base64;
        rec.body_bytes = body.len;
        rec.has_body = true;
        _.retained_body_bytes_total += body.len;
        _.shedBodiesOverBudget();
    }

    fn dropBody(*_, rec: *RequestRecord) {
        if (!rec.has_body) return;
        if (_.retained_body_bytes_total >= rec.body_bytes) {
            _.retained_body_bytes_total -= rec.body_bytes;
        } else {
            _.retained_body_bytes_total = 0;
        }
        rec.body = "";
        rec.body_bytes = 0;
        rec.body_is_base64 = false;
        rec.has_body = false;
    }

    fn nextRequestId(*_, out_buf: []u8): []const u8 {
        const n = _.next_seq;
        _.next_seq += 1;
        const prefix_len = @min(_.connection_id.len, REQUEST_ID_CONNECTION_PREFIX_CHARS);
        const prefix = if (prefix_len > 0) _.connection_id[0..prefix_len] else "conn";
        return std.fmt.bufPrint(out_buf, "cs-{s}-{d}", .{ prefix, n }) catch "cs-req-0";
    }

    fn setTargetAttribution(*_, frame_id: []const u8, loader_id: []const u8, session_id: []const u8) {
        let slot: usize = MAX_ATTRIBUTIONS;
        let i: usize = 0;
        while (i < MAX_ATTRIBUTIONS) : (i += 1) {
            if (_.attributions[i].active && std.mem.eql(u8, _.attributions[i].frame_id[0.._.attributions[i].frame_id_len], frame_id)) {
                slot = i;
                break;
            }
            if (!_.attributions[i].active && slot == MAX_ATTRIBUTIONS) {
                slot = i;
            }
        }
        if (slot >= MAX_ATTRIBUTIONS) return;
        let attr = &_.attributions[slot];
        const flen = @min(frame_id.len, 64);
        @memcpy(attr.frame_id[0..flen], frame_id[0..flen]);
        attr.frame_id_len = flen;

        const llen = @min(loader_id.len, 64);
        @memcpy(attr.loader_id[0..llen], loader_id[0..llen]);
        attr.loader_id_len = llen;

        const slen = @min(session_id.len, 64);
        @memcpy(attr.session_id[0..slen], session_id[0..slen]);
        attr.session_id_len = slen;

        if (!attr.active) {
            attr.active = true;
            _.attribution_count += 1;
        }
    }

    fn deleteTargetAttribution(*_, frame_id: []const u8) {
        let i: usize = 0;
        while (i < MAX_ATTRIBUTIONS) : (i += 1) {
            if (_.attributions[i].active && std.mem.eql(u8, _.attributions[i].frame_id[0.._.attributions[i].frame_id_len], frame_id)) {
                _.attributions[i].active = false;
                _.attributions[i].frame_id_len = 0;
                _.attributions[i].loader_id_len = 0;
                _.attributions[i].session_id_len = 0;
                if (_.attribution_count > 0) _.attribution_count -= 1;
                return;
            }
        }
    }

    fn attributionForTarget(*_, frame_id: []const u8): (*TargetAttribution)? {
        let i: usize = 0;
        while (i < MAX_ATTRIBUTIONS) : (i += 1) {
            if (_.attributions[i].active && std.mem.eql(u8, _.attributions[i].frame_id[0.._.attributions[i].frame_id_len], frame_id)) {
                return &_.attributions[i];
            }
        }
        return null;
    }

    fn dropStaleLoaderBodies(*_, frame_id: []const u8) {
        const attr_opt = _.attributionForTarget(frame_id);
        if (attr_opt == null) return;
        const attr = attr_opt.?;
        const current_loader_id = attr.loader_id[0..attr.loader_id_len];
        let i: usize = 0;
        while (i < MAX_RETAINED_REQUESTS) : (i += 1) {
            if (_.records[i].active && std.mem.eql(u8, _.records[i].frame_id, frame_id)) {
                if (!std.mem.eql(u8, _.records[i].loader_id, current_loader_id)) {
                    _.dropBody(&_.records[i]);
                }
            }
        }
    }

    fn findSlot(request_id: []const u8): usize? {
        let i: usize = 0;
        while (i < MAX_RETAINED_REQUESTS) : (i += 1) {
            if (_.records[i].active && std.mem.eql(u8, _.records[i].request_id, request_id)) {
                return i;
            }
        }
        return null;
    }

    fn findOrAllocSlot(*_, request_id: []const u8): usize {
        const existing = _.findSlot(request_id);
        if (existing != null) {
            return existing.?;
        }
        if (_.record_count >= MAX_RETAINED_REQUESTS && _.order_count > 0) {
            const evict_slot = _.order[_.order_head];
            _.order_head = (_.order_head + 1) % MAX_RETAINED_REQUESTS;
            _.order_count -= 1;
            _.dropBody(&_.records[evict_slot]);
            if (_.records[evict_slot].in_flight && _.in_flight_count > 0) {
                _.in_flight_count -= 1;
            }
            _.records[evict_slot] = RequestRecord{};
            _.records[evict_slot].active = true;
            _.records[evict_slot].request_id = request_id;
            const tail = (_.order_head + _.order_count) % MAX_RETAINED_REQUESTS;
            _.order[tail] = evict_slot;
            _.order_count += 1;
            return evict_slot;
        }
        let slot: usize = 0;
        while (slot < MAX_RETAINED_REQUESTS) : (slot += 1) {
            if (!_.records[slot].active) break;
        }
        if (slot >= MAX_RETAINED_REQUESTS) slot = 0;
        _.records[slot] = RequestRecord{};
        _.records[slot].active = true;
        _.records[slot].request_id = request_id;
        _.record_count += 1;
        const tail = (_.order_head + _.order_count) % MAX_RETAINED_REQUESTS;
        _.order[tail] = slot;
        _.order_count += 1;
        return slot;
    }

    fn recordBeginRequest(
        *_,
        request_id: []const u8,
        url: []const u8,
        method: []const u8,
        resource_type: []const u8,
        frame_id: []const u8,
        loader_id: []const u8,
    ) {
        const slot = _.findOrAllocSlot(request_id);
        let rec = &_.records[slot];
        rec.request_id = request_id;
        rec.url = url;
        rec.method = method;
        rec.resource_type = resource_type;
        rec.frame_id = frame_id;
        rec.loader_id = loader_id;
        if (!rec.in_flight) {
            rec.in_flight = true;
            _.in_flight_count += 1;
        }
        rec.completed = false;
    }

    fn recordResponseReceived(
        *_,
        request_id: []const u8,
        status: u32,
        mime_type: []const u8,
        headers_raw: []const u8,
    ) {
        const slot_opt = _.findSlot(request_id);
        if (slot_opt == null) return;
        const slot = slot_opt.?;
        _.records[slot].status = status;
        _.records[slot].mime_type = mime_type;
        _.records[slot].headers_raw = headers_raw;
    }

    fn recordResponseBody(*_, request_id: []const u8, body: []const u8, is_base64: bool) {
        const slot_opt = _.findSlot(request_id);
        if (slot_opt == null) return;
        const slot = slot_opt.?;
        if (_.effective_per_body_bytes > 0 && body.len <= _.effective_per_body_bytes) {
            _.storeBody(&_.records[slot], body, is_base64);
        }
    }

    fn recordLoadingFinished(*_, request_id: []const u8, encoded_data_length: usize) {
        const slot_opt = _.findSlot(request_id);
        if (slot_opt == null) return;
        const slot = slot_opt.?;
        _.records[slot].encoded_data_length = encoded_data_length;
        if (_.records[slot].in_flight) {
            _.records[slot].in_flight = false;
            if (_.in_flight_count > 0) {
                _.in_flight_count -= 1;
            }
        }
        _.records[slot].completed = true;
    }

    fn recordLoadingFailed(*_, request_id: []const u8, error_text: []const u8, canceled: bool) {
        const slot_opt = _.findSlot(request_id);
        if (slot_opt == null) return;
        const slot = slot_opt.?;
        _.records[slot].error_text = error_text;
        _.records[slot].canceled = canceled;
        if (_.records[slot].in_flight) {
            _.records[slot].in_flight = false;
            if (_.in_flight_count > 0) {
                _.in_flight_count -= 1;
            }
        }
        _.records[slot].completed = true;
    }

    fn getResponseBody(*_, request_id: []const u8): (*RequestRecord)? {
        let i: usize = 0;
        while (i < MAX_RETAINED_REQUESTS) : (i += 1) {
            if (_.records[i].active && std.mem.eql(u8, _.records[i].request_id, request_id)) {
                if (_.records[i].has_body) {
                    return &_.records[i];
                }
                return null;
            }
        }
        return null;
    }

    fn getInFlight(): usize {
        return _.in_flight_count;
    }

    fn retainedBodyBytes(): usize {
        return _.retained_body_bytes_total;
    }

    fn retainedRequestCount(): usize {
        return _.record_count;
    }

    fn knowsRequest(request_id: []const u8): bool {
        return _.findSlot(request_id) != null;
    }

    fn dispose(*_) {
        _.disposed = true;
        let i: usize = 0;
        while (i < MAX_RETAINED_REQUESTS) : (i += 1) {
            if (_.records[i].active) {
                _.dropBody(&_.records[i]);
                _.records[i].active = false;
            }
        }
        _.order_head = 0;
        _.order_count = 0;
        _.record_count = 0;
        _.in_flight_count = 0;
        _.retained_body_bytes_total = 0;
        i = 0;
        while (i < MAX_SESSIONS) : (i += 1) {
            _.sessions[i].active = false;
        }
        _.session_count = 0;
        i = 0;
        while (i < MAX_ATTRIBUTIONS) : (i += 1) {
            _.attributions[i].active = false;
        }
        _.attribution_count = 0;
    }

    static fn inferResourceType(content_type: []const u8, hint: []const u8): []const u8 {
        if (hint.len > 0 && !std.mem.eql(u8, hint, "Other")) {
            return hint;
        }
        let buf: [128]u8 = [0; 128];
        const ct = cleanMimeType(buf[0..], content_type);
        if (ct.len == 0) return "Other";
        if (std.mem.eql(u8, ct, "text/html") || std.mem.eql(u8, ct, "application/xhtml+xml")) {
            return "Document";
        }
        if (std.mem.eql(u8, ct, "text/css")) {
            return "Stylesheet";
        }
        if (std.mem.eql(u8, ct, "text/javascript") ||
            std.mem.eql(u8, ct, "application/javascript") ||
            std.mem.eql(u8, ct, "application/x-javascript") ||
            (std.mem.endsWith(u8, ct, "+json") && std.mem.indexOf(u8, ct, "javascript") != null)) {
            return "Script";
        }
        if (std.mem.eql(u8, ct, "application/manifest+json")) {
            return "Manifest";
        }
        if (std.mem.eql(u8, ct, "application/json") || std.mem.endsWith(u8, ct, "+json")) {
            return "Fetch";
        }
        if (std.mem.startsWith(u8, ct, "image/")) {
            return "Image";
        }
        if (std.mem.startsWith(u8, ct, "font/") || std.mem.startsWith(u8, ct, "application/font-")) {
            return "Font";
        }
        if (std.mem.startsWith(u8, ct, "video/") || std.mem.startsWith(u8, ct, "audio/")) {
            return "Media";
        }
        if (std.mem.eql(u8, ct, "text/event-stream")) {
            return "EventSource";
        }
        return "Other";
    }

    static fn isTextLikeMime(mime_type: []const u8): bool {
        let buf: [128]u8 = [0; 128];
        const ct = cleanMimeType(buf[0..], mime_type);
        if (ct.len == 0) return false;
        if (std.mem.startsWith(u8, ct, "text/")) return true;
        if (std.mem.eql(u8, ct, "application/json") ||
            std.mem.eql(u8, ct, "application/javascript") ||
            std.mem.eql(u8, ct, "application/x-javascript") ||
            std.mem.eql(u8, ct, "application/xhtml+xml") ||
            std.mem.eql(u8, ct, "application/xml") ||
            std.mem.eql(u8, ct, "application/manifest+json")) {
            return true;
        }
        if (std.mem.endsWith(u8, ct, "+json") || std.mem.endsWith(u8, ct, "+xml")) {
            return true;
        }
        return false;
    }
}

pub fn inferResourceType(content_type: []const u8, hint: []const u8): []const u8 {
    return NetworkEventTracker.inferResourceType(content_type, hint);
}

pub fn isTextLikeMime(mime_type: []const u8): bool {
    return NetworkEventTracker.isTextLikeMime(mime_type);
}
