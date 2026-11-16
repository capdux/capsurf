// Migrated from vendor/kitesurf/packages/engine/src/eval/page-script-tail.ts
// Relays PageScript tail logs, spans, limit notices, and token-checked exceptions.

pub const ISOLATE_KILL_MARKER: []const u8 = "[ks-kill]";
pub const HUNG_INVOCATION_TEXT: []const u8 = "detected that your Worker's code had hung";
pub const SPAN_LOG_CAP: usize = 100;
pub const SPAN_REC_CAP: usize = 250;
pub const MAX_PAGE_SCRIPT_TAIL_LOG_LINE_CHARS: usize = 256;

pub struct TailLogRecord {
    level: [16]u8 = [0; 16],
    level_len: usize = 0,
    msg: [MAX_PAGE_SCRIPT_TAIL_LOG_LINE_CHARS]u8 = [0; MAX_PAGE_SCRIPT_TAIL_LOG_LINE_CHARS],
    msg_len: usize = 0,
    t_ms: u64 = 0,

    fn set(*_, level_str: []const u8, msg_str: []const u8, time_ms: u64) {
        let l_len = if (level_str.len > 16) 16 else level_str.len;
        @memcpy(_.level[0..l_len], level_str[0..l_len]);
        _.level_len = l_len;

        let m_len = if (msg_str.len > MAX_PAGE_SCRIPT_TAIL_LOG_LINE_CHARS) MAX_PAGE_SCRIPT_TAIL_LOG_LINE_CHARS else msg_str.len;
        @memcpy(_.msg[0..m_len], msg_str[0..m_len]);
        _.msg_len = m_len;

        _.t_ms = time_ms;
    }

    fn getLevel(): []const u8 {
        return _.level[0.._.level_len];
    }

    fn getMsg(): []const u8 {
        return _.msg[0.._.msg_len];
    }
}

pub struct PageScriptTail {
    script_name: []const u8 = "page-script",
    records: [SPAN_LOG_CAP]TailLogRecord = [TailLogRecord{}; 100],
    record_count: usize = 0,
    dropped_count: usize = 0,
    kill_requested: bool = false,
    hung_detected: bool = false,

    fn addLog(*_, level: []const u8, msg: []const u8, t_ms: u64) {
        if (std.mem.indexOf(u8, msg, ISOLATE_KILL_MARKER) != null) {
            _.kill_requested = true;
        }
        if (std.mem.indexOf(u8, msg, HUNG_INVOCATION_TEXT) != null) {
            _.hung_detected = true;
        }

        if (_.record_count >= SPAN_LOG_CAP) {
            _.dropped_count += 1;
            return;
        }

        _.records[_.record_count].set(level, msg, t_ms);
        _.record_count += 1;
    }

    fn reset(*_) {
        _.record_count = 0;
        _.dropped_count = 0;
        _.kill_requested = false;
        _.hung_detected = false;
    }
}

pub fn formatOtelRecord(
    level: []const u8,
    msg: []const u8,
    t_ms: u64,
    out_buf: []u8,
): []const u8 {
    let p1 = "{\"severityText\":\"";
    let p2 = "\",\"body\":{\"stringValue\":\"";
    let p3 = "\"},\"timeUnixNano\":\"";
    let p4 = "000000\"}";

    let pos: usize = 0;
    if (pos + p1.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + p1.len], p1);
    pos += p1.len;

    let l_len = if (level.len > 16) 16 else level.len;
    if (pos + l_len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + l_len], level[0..l_len]);
    pos += l_len;

    if (pos + p2.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + p2.len], p2);
    pos += p2.len;

    let m_len = if (msg.len > 256) 256 else msg.len;
    if (pos + m_len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + m_len], msg[0..m_len]);
    pos += m_len;

    if (pos + p3.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + p3.len], p3);
    pos += p3.len;

    let t_buf: [24]u8 = [0; 24];
    let t_str = formatU64(&t_buf, t_ms);
    if (pos + t_str.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + t_str.len], t_str);
    pos += t_str.len;

    if (pos + p4.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + p4.len], p4);
    pos += p4.len;

    return out_buf[0..pos];
}

fn formatU64(buf: *[24]u8, val: u64): []const u8 {
    if (val == 0) {
        buf[0] = '0';
        return buf[0..1];
    }
    let v = val;
    let temp: [24]u8 = [0; 24];
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
