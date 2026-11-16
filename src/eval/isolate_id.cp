// Migrated from vendor/kitesurf/packages/engine/src/eval/isolate-id.ts
// src/eval/isolate_id.cp
// Tags requests with the engine isolate ID so loopback hops can detect a different isolate.

pub const UUID_LEN: usize = 36;

@tls let g_isolate_id: [36]u8 = [0; 36];
@tls let g_isolate_id_initialized: bool = false;

fn initIdIfNeeded() {
    if (g_isolate_id_initialized) return;

    let b: [16]u8 = [0; 16];
    * = std.os.linux.getrandom(&b, 16, 0);
    b[6] = (b[6] & 0x0f) | 0x40; // RFC 4122 v4
    b[8] = (b[8] & 0x3f) | 0x80;

    const hex = std.fmt.bytesToHex(b, .lower);
    @memcpy(g_isolate_id[0..8], hex[0..8]);
    g_isolate_id[8] = 45; // '-'
    @memcpy(g_isolate_id[9..13], hex[8..12]);
    g_isolate_id[13] = 45;
    @memcpy(g_isolate_id[14..18], hex[12..16]);
    g_isolate_id[18] = 45;
    @memcpy(g_isolate_id[19..23], hex[16..20]);
    g_isolate_id[23] = 45;
    @memcpy(g_isolate_id[24..36], hex[20..32]);

    g_isolate_id_initialized = true;
}

pub fn engineIsolateId(out_buf: []u8): []const u8 {
    initIdIfNeeded();
    if (out_buf.len >= UUID_LEN) {
        @memcpy(out_buf[0..UUID_LEN], g_isolate_id[0..UUID_LEN]);
        return out_buf[0..UUID_LEN];
    }
    return g_isolate_id[0..UUID_LEN];
}

pub fn getEngineIsolateId(): []const u8 {
    initIdIfNeeded();
    return g_isolate_id[0..UUID_LEN];
}

pub fn isForeignIsolate(stamped: []const u8): bool {
    if (stamped.len == 0) return false;
    initIdIfNeeded();
    return !std.mem.eql(u8, stamped, g_isolate_id[0..UUID_LEN]);
}

pub fn setEngineIsolateIdForTest(id_str: []const u8) {
    const len = @min(id_str.len, UUID_LEN);
    @memcpy(g_isolate_id[0..len], id_str[0..len]);
    g_isolate_id_initialized = true;
}

pub fn resetEngineIsolateIdForTest() {
    g_isolate_id_initialized = false;
}
