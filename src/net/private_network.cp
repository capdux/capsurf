// Migrated from vendor/kitesurf/packages/engine/src/eval/private-network.ts
// CapSurf W3C Private Network Access (PNA) Specification Implementation.
// Zero GC, compile-time borrow & exclusivity safety, zero heap allocation.

pub enum AddressSpace: u8 {
    loopback = 0,
    @"private" = 1,
    @"public" = 2,

    fn rank(): u8 {
        return @intFromEnum(_);
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .loopback => "loopback",
            .@"private" => "private",
            .@"public" => "public",
        };
    }
}

fn parseIpv4Bytes(raw_host: []const u8): ([4]u8)? {
    let host = raw_host;
    if (host.len > 0 && host[host.len - 1] == '.') {
        host = host[0 .. host.len - 1];
    }
    if (colon_idx := std.mem.lastIndexOfScalar(u8, host, ':')) {
        let colon_count: usize = 0;
        for (c of host) {
            if (c == ':') colon_count += 1;
        }
        if (colon_count == 1) {
            host = host[0..colon_idx];
        }
    }
    if (host.len < 7 || host.len > 15) return null;

    let bytes: [4]u8 = [0; 4];
    let part_idx: usize = 0;
    let seg_start: usize = 0;

    let i: usize = 0;
    while (i <= host.len) : (i += 1) {
        if (i == host.len || host[i] == '.') {
            if (part_idx >= 4) return null;
            const seg = host[seg_start..i];
            if (seg.len == 0 || seg.len > 3) return null;

            let val: u32 = 0;
            for (c of seg) {
                if (c < '0' || c > '9') return null;
                val = val * 10 + (c - '0');
            }
            if (val > 255) return null;
            bytes[part_idx] = @as(u8, @intCast(val));
            part_idx += 1;
            seg_start = i + 1;
        }
    }

    if (part_idx != 4) return null;
    return bytes;
}

pub fn parseIpv4Space(host: []const u8): AddressSpace? {
    if (bytes := parseIpv4Bytes(host)) {
        const a = bytes[0];
        const b = bytes[1];
        if (a == 127 || a == 0) return AddressSpace.loopback;
        if (a == 10 || (a == 172 && b >= 16 && b <= 31) || (a == 192 && b == 168)) return AddressSpace.@"private";
        if ((a == 169 && b == 254) || (a == 100 && b >= 64 && b <= 127) || a >= 224) return AddressSpace.@"private";
        return AddressSpace.@"public";
    }
    return null;
}

pub fn ipv4Space(host: []const u8): AddressSpace {
    return parseIpv4Space(host) ?? AddressSpace.@"public";
}

pub fn ipv6Space(host: []const u8): AddressSpace {
    let raw = host;
    if (raw.len >= 2 && raw[0] == '[' && raw[raw.len - 1] == ']') {
        raw = raw[1 .. raw.len - 1];
    }

    let lower_buf: [128]u8 = [0; 128];
    if (raw.len > lower_buf.len || raw.len == 0) return AddressSpace.@"public";
    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        lower_buf[i] = std.ascii.toLower(raw[i]);
    }
    const lower = lower_buf[0..raw.len];

    if (std.mem.eql(u8, lower, "::1") || std.mem.eql(u8, lower, "::")) return AddressSpace.loopback;

    // Check full-form loopback like 0:0:0:0:0:0:0:1
    if (std.mem.endsWith(u8, lower, ":1")) {
        let all_zero_prefix = true;
        let k: usize = 0;
        const prefix_len = lower.len - 2;
        while (k < prefix_len) : (k += 1) {
            const ch = lower[k];
            if (ch != '0' && ch != ':') {
                all_zero_prefix = false;
                break;
            }
        }
        if (all_zero_prefix && prefix_len > 0) return AddressSpace.loopback;
    }

    // IPv4-mapped address: ::ffff:x.x.x.x or ::ffff:hi:lo
    if (std.mem.startsWith(u8, lower, "::ffff:")) {
        const rest = lower[7..];
        if (std.mem.indexOfScalar(u8, rest, '.') != null) {
            return ipv4Space(rest);
        }
        if (colon_pos := std.mem.indexOfScalar(u8, rest, ':')) {
            const hi_str = rest[0..colon_pos];
            const lo_str = rest[colon_pos + 1 ..];
            const hi = std.fmt.parseInt(u16, hi_str, 16) catch 0;
            const lo = std.fmt.parseInt(u16, lo_str, 16) catch 0;
            const a = @as(u8, @intCast(hi >> 8));
            const b = @as(u8, @intCast(hi & 0xff));
            if (a == 127 || a == 0) return AddressSpace.loopback;
            if (a == 10 || (a == 172 && b >= 16 && b <= 31) || (a == 192 && b == 168)) return AddressSpace.@"private";
            if ((a == 169 && b == 254) || (a == 100 && b >= 64 && b <= 127) || a >= 224) return AddressSpace.@"private";
            return AddressSpace.@"public";
        }
    }

    // Parse leading 16-bit hex segment
    let first_str = lower;
    if (colon_pos := std.mem.indexOfScalar(u8, lower, ':')) {
        first_str = lower[0..colon_pos];
    }

    let first: u16 = 0;
    if (first_str.len > 0) {
        first = std.fmt.parseInt(u16, first_str, 16) catch 0;
    }

    // Unique local fc00::/7 (fc00 - fdff)
    if ((first & 0xfe00) == 0xfc00) return AddressSpace.@"private";
    // Link-local fe80::/10 (fe80 - febf)
    if ((first & 0xffc0) == 0xfe80) return AddressSpace.@"private";
    // Multicast ff00::/8 (ff00 - ffff)
    if ((first & 0xff00) == 0xff00) return AddressSpace.@"private";

    return AddressSpace.@"public";
}

pub fn extractHostname(input: []const u8): []const u8 {
    let rem = input;
    if (proto_end := std.mem.indexOf(u8, rem, "://")) {
        rem = rem[proto_end + 3 ..];
        let end_idx = rem.len;
        let i: usize = 0;
        while (i < rem.len) : (i += 1) {
            const ch = rem[i];
            if (ch == '/' || ch == '?' || ch == '#') {
                end_idx = i;
                break;
            }
        }
        rem = rem[0..end_idx];
    }

    if (at_idx := std.mem.lastIndexOfScalar(u8, rem, '@')) {
        rem = rem[at_idx + 1 ..];
    }

    if (rem.len > 0 && rem[0] == '[') {
        if (close_bracket := std.mem.indexOfScalar(u8, rem, ']')) {
            return rem[0 .. close_bracket + 1];
        }
    }

    if (colon_idx := std.mem.lastIndexOfScalar(u8, rem, ':')) {
        let colon_count: usize = 0;
        for (c of rem) {
            if (c == ':') colon_count += 1;
        }
        if (colon_count == 1) {
            rem = rem[0..colon_idx];
        }
    }

    return rem;
}

pub fn addressSpaceOf(host_or_url: []const u8): AddressSpace {
    let raw_host = extractHostname(host_or_url);
    if (raw_host.len == 0) return AddressSpace.@"public";

    if (raw_host.len > 0 && raw_host[raw_host.len - 1] == '.') {
        raw_host = raw_host[0 .. raw_host.len - 1];
    }
    if (raw_host.len == 0) return AddressSpace.@"public";

    if (raw_host.len >= 2 && raw_host[0] == '[' && raw_host[raw_host.len - 1] == ']') {
        return ipv6Space(raw_host[1 .. raw_host.len - 1]);
    }

    let lower_buf: [256]u8 = [0; 256];
    if (raw_host.len > lower_buf.len) return AddressSpace.@"public";
    let i: usize = 0;
    while (i < raw_host.len) : (i += 1) {
        lower_buf[i] = std.ascii.toLower(raw_host[i]);
    }
    const host = lower_buf[0..raw_host.len];

    if (std.mem.eql(u8, host, "localhost") || std.mem.endsWith(u8, host, ".localhost")) {
        return AddressSpace.loopback;
    }

    if (std.mem.indexOfScalar(u8, host, ':') != null) {
        return ipv6Space(host);
    }

    if (space := parseIpv4Space(host)) {
        return space;
    }

    return AddressSpace.@"public";
}

pub fn privateNetworkRefusal(target_host: []const u8, initiator_host: []const u8): bool {
    const target_space = addressSpaceOf(target_host);
    if (target_space == AddressSpace.@"public") return false;

    let initiator_space = AddressSpace.@"public";
    if (initiator_host.len > 0) {
        initiator_space = addressSpaceOf(initiator_host);
    }

    if (@intFromEnum(target_space) < @intFromEnum(initiator_space)) {
        return true;
    }
    return false;
}

pub fn privateNetworkRefusalReason(
    target_host: []const u8,
    initiator_host: []const u8,
    out_buf: []u8,
): []const u8 {
    if (!privateNetworkRefusal(target_host, initiator_host)) return "";
    const target_space = addressSpaceOf(target_host);
    let initiator_space = AddressSpace.@"public";
    if (initiator_host.len > 0) {
        initiator_space = addressSpaceOf(initiator_host);
    }
    const target_name = extractHostname(target_host);

    return std.fmt.bufPrint(
        out_buf,
        "capsurf: a {s} document may not reach the {s} address {s}",
        .{ initiator_space.asStr(), target_space.asStr(), target_name },
    ) catch "";
}
