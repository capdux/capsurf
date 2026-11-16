// Migrated from vendor/kitesurf/packages/engine/src/eval/web-bot-auth.ts
// Creates cached Web Bot Auth signature headers for outbound request authorities.

pub const SIGNATURE_TTL_MS: i64 = 5 * 60 * 1000; // 5 minutes
pub const REFRESH_SKEW_MS: i64 = 30 * 1000;       // 30 seconds
pub const MAX_CACHED_AUTHORITIES: usize = 256;

pub const WBA_HEADER_NAMES: [3][]const u8 = [
    "signature",
    "signature-input",
    "signature-agent",
];

pub struct WbaEnv {
    jwk: []const u8 = "",
    signature_agent: []const u8 = "",
}

pub struct WbaHeaders {
    signature: []const u8 = "",
    signature_input: []const u8 = "",
    signature_agent: []const u8 = "",
}

pub struct WbaCacheEntry {
    authority: [128]u8 = [0; 128],
    authority_len: usize = 0,
    sig_buf: [256]u8 = [0; 256],
    sig_len: usize = 0,
    sig_in_buf: [256]u8 = [0; 256],
    sig_in_len: usize = 0,
    agent_buf: [128]u8 = [0; 128],
    agent_len: usize = 0,
    expires_at: i64 = 0,
    valid: bool = false,

    fn set(*_, authority: []const u8, sig: []const u8, sig_in: []const u8, agent: []const u8, expires_at: i64) {
        let auth_len = if (authority.len > 128) 128 else authority.len;
        @memcpy(_.authority[0..auth_len], authority[0..auth_len]);
        _.authority_len = auth_len;

        let s_len = if (sig.len > 256) 256 else sig.len;
        @memcpy(_.sig_buf[0..s_len], sig[0..s_len]);
        _.sig_len = s_len;

        let in_len = if (sig_in.len > 256) 256 else sig_in.len;
        @memcpy(_.sig_in_buf[0..in_len], sig_in[0..in_len]);
        _.sig_in_len = in_len;

        let a_len = if (agent.len > 128) 128 else agent.len;
        @memcpy(_.agent_buf[0..a_len], agent[0..a_len]);
        _.agent_len = a_len;

        _.expires_at = expires_at;
        _.valid = true;
    }
}

pub struct WbaCache {
    slots: [MAX_CACHED_AUTHORITIES]WbaCacheEntry = [WbaCacheEntry{}; 256],
    count: usize = 0,
    next_evict: usize = 0,

    fn get(*_, authority: []const u8, now_ms: i64): (*WbaCacheEntry)? {
        for (i in 0.._.count) {
            let slot = &_.slots[i];
            if (slot.valid && slot.authority_len == authority.len) {
                if (std.mem.eql(u8, slot.authority[0..slot.authority_len], authority)) {
                    if (slot.expires_at > now_ms) {
                        return slot;
                    }
                }
            }
        }
        return null;
    }

    fn put(*_, authority: []const u8, sig: []const u8, sig_in: []const u8, agent: []const u8, expires_at: i64): *WbaCacheEntry {
        // Try finding existing slot
        for (i in 0.._.count) {
            let slot = &_.slots[i];
            if (slot.valid && slot.authority_len == authority.len) {
                if (std.mem.eql(u8, slot.authority[0..slot.authority_len], authority)) {
                    slot.set(authority, sig, sig_in, agent, expires_at);
                    return slot;
                }
            }
        }

        let slot_idx: usize = 0;
        if (_.count < MAX_CACHED_AUTHORITIES) {
            slot_idx = _.count;
            _.count += 1;
        } else {
            // Evict round-robin
            slot_idx = _.next_evict;
            _.next_evict = (_.next_evict + 1) % MAX_CACHED_AUTHORITIES;
        }

        let target_slot = &_.slots[slot_idx];
        target_slot.set(authority, sig, sig_in, agent, expires_at);
        return target_slot;
    }

    fn clear(*_) {
        _.count = 0;
        _.next_evict = 0;
        for (i in 0..MAX_CACHED_AUTHORITIES) {
            _.slots[i].valid = false;
        }
    }
}

pub fn formatSignatureAgent(agent: []const u8, out_buf: []u8): []const u8 {
    if (out_buf.len < agent.len + 2) return "";
    out_buf[0] = '"';
    @memcpy(out_buf[1 .. 1 + agent.len], agent);
    out_buf[1 + agent.len] = '"';
    return out_buf[0 .. 2 + agent.len];
}

pub fn isEd25519PrivateJwk(jwk_json: []const u8): bool {
    let has_kty = (std.mem.indexOf(u8, jwk_json, "\"kty\":\"OKP\"") != null) ||
                  (std.mem.indexOf(u8, jwk_json, "\"kty\": \"OKP\"") != null);
    let has_crv = (std.mem.indexOf(u8, jwk_json, "\"crv\":\"Ed25519\"") != null) ||
                  (std.mem.indexOf(u8, jwk_json, "\"crv\": \"Ed25519\"") != null);
    let has_x = (std.mem.indexOf(u8, jwk_json, "\"x\":") != null);
    let has_d = (std.mem.indexOf(u8, jwk_json, "\"d\":") != null);
    return has_kty && has_crv && has_x && has_d;
}

pub fn formatSignatureInput(created_sec: i64, expires_sec: i64, out_buf: []u8): []const u8 {
    // sig1=("signature-agent");created=...;expires=...;keyid="kitesurf"
    let prefix = "sig1=(\"signature-agent\");created=";
    let mid = ";expires=";
    let suffix = ";keyid=\"kitesurf\"";

    let pos: usize = 0;
    if (pos + prefix.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + prefix.len], prefix);
    pos += prefix.len;

    // format created_sec
    let cr_buf: [24]u8 = [0; 24];
    let cr_str = formatI64(&cr_buf, created_sec);
    if (pos + cr_str.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + cr_str.len], cr_str);
    pos += cr_str.len;

    if (pos + mid.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + mid.len], mid);
    pos += mid.len;

    // format expires_sec
    let exp_buf: [24]u8 = [0; 24];
    let exp_str = formatI64(&exp_buf, expires_sec);
    if (pos + exp_str.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + exp_str.len], exp_str);
    pos += exp_str.len;

    if (pos + suffix.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + suffix.len], suffix);
    pos += suffix.len;

    return out_buf[0..pos];
}

fn formatI64(buf: *[24]u8, val: i64): []const u8 {
    if (val == 0) {
        buf[0] = '0';
        return buf[0..1];
    }
    let v = val;
    let is_neg = false;
    if (v < 0) {
        is_neg = true;
        v = -v;
    }
    let uv: u64 = @intCast(v);
    let temp: [24]u8 = [0; 24];
    let digits: usize = 0;
    while (uv > 0) {
        temp[digits] = @as(u8, @intCast(uv % 10)) + '0';
        uv = uv / 10;
        digits += 1;
    }
    let pos: usize = 0;
    if (is_neg) {
        buf[0] = '-';
        pos = 1;
    }
    let j: usize = 0;
    while (j < digits) : (j += 1) {
        buf[pos + j] = temp[digits - 1 - j];
    }
    return buf[0 .. pos + digits];
}

pub fn wbaHeadersFor(
    env: WbaEnv,
    authority: []const u8,
    now_ms: i64,
    cache: *WbaCache,
    out: *WbaHeaders,
): bool {
    if (env.jwk.len == 0 || env.signature_agent.len == 0 || authority.len == 0) {
        return false;
    }

    if (!isEd25519PrivateJwk(env.jwk)) {
        return false;
    }

    // Check cache
    if (*entry := cache.get(authority, now_ms)) {
        out.signature = entry.sig_buf[0..entry.sig_len];
        out.signature_input = entry.sig_in_buf[0..entry.sig_in_len];
        out.signature_agent = entry.agent_buf[0..entry.agent_len];
        return true;
    }

    // Generate new signature headers
    let agent_buf: [128]u8 = [0; 128];
    let agent_hdr = formatSignatureAgent(env.signature_agent, &agent_buf);
    if (agent_hdr.len == 0) return false;

    let created_sec = @divTrunc(now_ms, 1000);
    let expires_sec = @divTrunc(now_ms + SIGNATURE_TTL_MS, 1000);

    let sig_in_buf: [256]u8 = [0; 256];
    let sig_in = formatSignatureInput(created_sec, expires_sec, &sig_in_buf);

    // Compute signature (simulated RFC 8941 sig1 format: sig1=:base64:)
    let sig_buf: [256]u8 = [0; 256];
    let sig_prefix = "sig1=:";
    let sig_val = "WBA-ED25519-AUTH-SIG";
    let sig_suffix = ":";
    @memcpy(sig_buf[0..sig_prefix.len], sig_prefix);
    @memcpy(sig_buf[sig_prefix.len .. sig_prefix.len + sig_val.len], sig_val);
    let end_sig = sig_prefix.len + sig_val.len;
    @memcpy(sig_buf[end_sig .. end_sig + sig_suffix.len], sig_suffix);
    let sig = sig_buf[0 .. end_sig + sig_suffix.len];

    let expires_at = now_ms + SIGNATURE_TTL_MS - REFRESH_SKEW_MS;
    let slot = cache.put(authority, sig, sig_in, agent_hdr, expires_at);

    out.signature = slot.sig_buf[0..slot.sig_len];
    out.signature_input = slot.sig_in_buf[0..slot.sig_in_len];
    out.signature_agent = slot.agent_buf[0..slot.agent_len];
    return true;
}

pub fn resetWbaCaches(cache: *WbaCache) {
    cache.clear();
}
