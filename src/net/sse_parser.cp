// Migrated from vendor/kitesurf/packages/page-script/src/event-source.ts
// Server-Sent Events (SSE) streaming protocol parser.

pub struct ParsedEvent {
    event_type: []const u8 = "message",
    data: []const u8 = "",
    last_event_id: []const u8 = "",
    retry_ms: u32 = 0,
}

pub struct EventStreamParser {
    last_event_id: []const u8 = "",
    reconnection_time_ms: u32 = 3000,
    buffer: [4096]u8 = [0; 4096],
    buf_len: usize = 0,
    current_event: []const u8 = "message",
    data_buf: [4096]u8 = [0; 4096],
    data_len: usize = 0,

    id_buf: [256]u8 = [0; 256],
    id_len: usize = 0,
    last_id_buf: [256]u8 = [0; 256],
    last_id_len: usize = 0,
    event_buf: [256]u8 = [0; 256],
    event_len: usize = 0,
    has_data: bool = false,
    current_retry_ms: u32 = 0,
    started: bool = false,
    skip_leading_lf: bool = false,
    event_data_start: usize = 0,

    static fn init(): EventStreamParser {
        return EventStreamParser{};
    }

    fn reset(*_) {
        _.last_event_id = "";
        _.reconnection_time_ms = 3000;
        _.buf_len = 0;
        _.current_event = "message";
        _.data_len = 0;
        _.id_len = 0;
        _.last_id_len = 0;
        _.event_len = 0;
        _.has_data = false;
        _.current_retry_ms = 0;
        _.started = false;
        _.skip_leading_lf = false;
        _.event_data_start = 0;
    }

    fn setLastEventId(*_, id: []const u8) {
        let to_copy = if (id.len < _.id_buf.len) id.len else _.id_buf.len;
        if (to_copy > 0) {
            @memcpy(_.id_buf[0..to_copy], id[0..to_copy]);
            @memcpy(_.last_id_buf[0..to_copy], id[0..to_copy]);
        }
        _.id_len = to_copy;
        _.last_id_len = to_copy;
        _.last_event_id = if (to_copy > 0) _.last_id_buf[0..to_copy] else "";
    }

    fn processLine(*_, line: []const u8, out_events: []ParsedEvent): usize {
        if (line.len == 0) {
            if (_.id_len > 0) {
                @memcpy(_.last_id_buf[0.._.id_len], _.id_buf[0.._.id_len]);
                _.last_id_len = _.id_len;
                _.last_event_id = _.last_id_buf[0.._.last_id_len];
            } else {
                _.last_id_len = 0;
                _.last_event_id = "";
            }

            if (!_.has_data) {
                _.event_len = 0;
                _.current_event = "message";
                _.current_retry_ms = 0;
                return 0;
            }

            let evt_type: []const u8 = "message";
            if (_.event_len > 0) {
                evt_type = _.event_buf[0.._.event_len];
            }

            let data_slice: []const u8 = "";
            if (_.data_len > _.event_data_start) {
                let end = _.data_len - 1;
                if (end >= _.event_data_start) {
                    data_slice = _.data_buf[_.event_data_start..end];
                }
            }

            let emitted: usize = 0;
            if (out_events.len > 0) {
                out_events[0] = ParsedEvent{
                    event_type: evt_type,
                    data: data_slice,
                    last_event_id: _.last_event_id,
                    retry_ms: _.current_retry_ms,
                };
                emitted = 1;
            }

            _.event_data_start = _.data_len;
            _.has_data = false;
            _.event_len = 0;
            _.current_event = "message";
            _.current_retry_ms = 0;
            return emitted;
        }

        if (line[0] == ':') {
            return 0;
        }

        let colon_idx: usize = line.len;
        let j: usize = 0;
        while (j < line.len) : (j += 1) {
            if (line[j] == ':') {
                colon_idx = j;
                break;
            }
        }

        let field: []const u8 = line;
        let val_slice: []const u8 = line[0..0];
        if (colon_idx < line.len) {
            field = line[0..colon_idx];
            let raw_val = line[colon_idx + 1 ..];
            if (raw_val.len > 0 && raw_val[0] == ' ') {
                val_slice = raw_val[1..];
            } else {
                val_slice = raw_val;
            }
        }

        if (std.mem.eql(u8, field, "data")) {
            _.has_data = true;
            let space = if (_.data_buf.len > _.data_len) _.data_buf.len - _.data_len else 0;
            let to_copy = if (val_slice.len < space) val_slice.len else space;
            if (to_copy > 0) {
                @memcpy(_.data_buf[_.data_len .. _.data_len + to_copy], val_slice[0..to_copy]);
                _.data_len += to_copy;
            }
            if (_.data_len < _.data_buf.len) {
                _.data_buf[_.data_len] = '\n';
                _.data_len += 1;
            }
        } else if (std.mem.eql(u8, field, "event")) {
            let max_len = _.event_buf.len;
            let to_copy = if (val_slice.len < max_len) val_slice.len else max_len;
            if (to_copy > 0) {
                @memcpy(_.event_buf[0..to_copy], val_slice[0..to_copy]);
            }
            _.event_len = to_copy;
            _.current_event = if (to_copy > 0) _.event_buf[0..to_copy] else "message";
        } else if (std.mem.eql(u8, field, "id")) {
            let has_null = false;
            let k: usize = 0;
            while (k < val_slice.len) : (k += 1) {
                if (val_slice[k] == 0) {
                    has_null = true;
                    break;
                }
            }
            if (!has_null) {
                let max_len = _.id_buf.len;
                let to_copy = if (val_slice.len < max_len) val_slice.len else max_len;
                if (to_copy > 0) {
                    @memcpy(_.id_buf[0..to_copy], val_slice[0..to_copy]);
                }
                _.id_len = to_copy;
            }
        } else if (std.mem.eql(u8, field, "retry")) {
            if (val_slice.len > 0) {
                let all_digits = true;
                let k: usize = 0;
                while (k < val_slice.len) : (k += 1) {
                    if (val_slice[k] < '0' || val_slice[k] > '9') {
                        all_digits = false;
                        break;
                    }
                }
                if (all_digits) {
                    let parsed_num: u32 = 0;
                    let overflow = false;
                    let k2: usize = 0;
                    while (k2 < val_slice.len) : (k2 += 1) {
                        let digit: u32 = @as(u32, val_slice[k2] - '0');
                        let next = parsed_num * 10 + digit;
                        if (next < parsed_num) {
                            overflow = true;
                            break;
                        }
                        parsed_num = next;
                    }
                    if (!overflow) {
                        _.reconnection_time_ms = parsed_num;
                        _.current_retry_ms = parsed_num;
                    }
                }
            }
        }

        return 0;
    }

    fn feedChunk(*_, chunk: []const u8, out_events: []ParsedEvent): usize {
        if (_.event_data_start == _.data_len) {
            _.data_len = 0;
            _.event_data_start = 0;
        } else if (_.event_data_start > 0) {
            let remaining_len = _.data_len - _.event_data_start;
            let m: usize = 0;
            while (m < remaining_len) : (m += 1) {
                _.data_buf[m] = _.data_buf[_.event_data_start + m];
            }
            _.data_len = remaining_len;
            _.event_data_start = 0;
        }

        let mut_idx: usize = 0;
        if (!_.started && chunk.len > 0) {
            _.started = true;
            if (chunk.len >= 3 && chunk[0] == 0xef && chunk[1] == 0xbb && chunk[2] == 0xbf) {
                mut_idx = 3;
            }
        }

        if (chunk.len > 0) {
            if (_.skip_leading_lf && mut_idx < chunk.len && chunk[mut_idx] == 0x0a) {
                mut_idx += 1;
            }
            _.skip_leading_lf = false;
        }

        let line_start: usize = mut_idx;
        let i: usize = mut_idx;
        let out_count: usize = 0;

        while (i < chunk.len) : (i += 1) {
            let code = chunk[i];
            if (code != 0x0a && code != 0x0d) {
                continue;
            }

            let line_slice = chunk[line_start..i];
            if (_.buf_len > 0) {
                let avail = if (_.buffer.len > _.buf_len) _.buffer.len - _.buf_len else 0;
                let to_copy = if (line_slice.len < avail) line_slice.len else avail;
                if (to_copy > 0) {
                    @memcpy(_.buffer[_.buf_len .. _.buf_len + to_copy], line_slice[0..to_copy]);
                    _.buf_len += to_copy;
                }
                let slice_to_pass = if (out_count < out_events.len) out_events[out_count..] else out_events[0..0];
                let added = _.processLine(_.buffer[0.._.buf_len], slice_to_pass);
                out_count += added;
                _.buf_len = 0;
            } else {
                let slice_to_pass = if (out_count < out_events.len) out_events[out_count..] else out_events[0..0];
                let added = _.processLine(line_slice, slice_to_pass);
                out_count += added;
            }

            if (code == 0x0d) {
                if (i + 1 < chunk.len) {
                    if (chunk[i + 1] == 0x0a) {
                        i += 1;
                    }
                } else {
                    _.skip_leading_lf = true;
                }
            }
            line_start = i + 1;
        }

        if (line_start < chunk.len) {
            let rem = chunk[line_start..];
            let avail = if (_.buffer.len > _.buf_len) _.buffer.len - _.buf_len else 0;
            let to_copy = if (rem.len < avail) rem.len else avail;
            if (to_copy > 0) {
                @memcpy(_.buffer[_.buf_len .. _.buf_len + to_copy], rem[0..to_copy]);
                _.buf_len += to_copy;
            }
        }

        return out_count;
    }
}
