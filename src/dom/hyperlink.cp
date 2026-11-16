// src/dom/hyperlink.cp
// HTMLHyperlinkElementUtils URL decomposition implementation

pub struct HyperlinkDecomposition {
    href: []const u8 = "",
    protocol: []const u8 = ":",
    host: []const u8 = "",
    hostname: []const u8 = "",
    port: []const u8 = "",
    pathname: []const u8 = "",
    search: []const u8 = "",
    hash: []const u8 = "",
    origin: []const u8 = "null",

    static fn invalid(href: []const u8): HyperlinkDecomposition {
        return HyperlinkDecomposition{
            href: href,
            protocol: ":",
            host: "",
            hostname: "",
            port: "",
            pathname: "",
            search: "",
            hash: "",
            origin: "null",
        };
    }

    /// Decompose a hyperlink URL into its standard HTMLHyperlinkElementUtils components.
    /// If the URL is invalid (e.g. malformed host, invalid port, extra colons),
    /// protocol is ":" and origin is "null", while other parsed components are empty.
    static fn decompose(href: []const u8): HyperlinkDecomposition {
        if (href.len == 0) return HyperlinkDecomposition.invalid(href);

        // 1. Find scheme ending with ':'
        let colon_idx_opt = std.mem.indexOfScalar(u8, href, ':');
        if (colon_idx_opt == null) return HyperlinkDecomposition.invalid(href);
        const colon_idx = colon_idx_opt.?;
        if (colon_idx == 0) return HyperlinkDecomposition.invalid(href);

        const scheme = href[0..colon_idx];
        const c0 = scheme[0];
        if (!((c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z'))) {
            return HyperlinkDecomposition.invalid(href);
        }

        let si: usize = 1;
        while (si < scheme.len) : (si += 1) {
            const sc = scheme[si];
            const is_alpha = (sc >= 'a' && sc <= 'z') || (sc >= 'A' && sc <= 'Z');
            const is_digit = sc >= '0' && sc <= '9';
            const is_valid_scheme_char = is_alpha || is_digit || sc == '+' || sc == '-' || sc == '.';
            if (!is_valid_scheme_char) return HyperlinkDecomposition.invalid(href);
        }

        const protocol = href[0 .. colon_idx + 1];

        // 2. Identify special schemes that require authority //
        const is_http = std.ascii.eqlIgnoreCase(scheme, "http");
        const is_https = std.ascii.eqlIgnoreCase(scheme, "https");
        const is_ws = std.ascii.eqlIgnoreCase(scheme, "ws");
        const is_wss = std.ascii.eqlIgnoreCase(scheme, "wss");
        const is_ftp = std.ascii.eqlIgnoreCase(scheme, "ftp");
        const is_special = is_http || is_https || is_ws || is_wss || is_ftp;

        if (!is_special) {
            // Opaque or non-network schemes (e.g. javascript:, mailto:, data:)
            const rem = href[colon_idx + 1 ..];
            return HyperlinkDecomposition{
                href: href,
                protocol: protocol,
                host: "",
                hostname: "",
                port: "",
                pathname: rem,
                search: "",
                hash: "",
                origin: "null",
            };
        }

        // Special schemes must have authority "//"
        const rem = href[colon_idx + 1 ..];
        if (rem.len < 2 || rem[0] != '/' || rem[1] != '/') {
            return HyperlinkDecomposition.invalid(href);
        }
        const after_slashes = rem[2..];

        // 3. Scan authority
        let auth_end: usize = 0;
        while (auth_end < after_slashes.len) : (auth_end += 1) {
            const ch = after_slashes[auth_end];
            if (ch == '/' || ch == '?' || ch == '#') break;
        }
        const authority = after_slashes[0..auth_end];
        if (authority.len == 0) return HyperlinkDecomposition.invalid(href);

        // Strip userinfo if present
        let host_port_part = authority;
        if (at_idx := std.mem.lastIndexOfScalar(u8, authority, '@')) {
            host_port_part = authority[at_idx + 1 ..];
        }
        if (host_port_part.len == 0) return HyperlinkDecomposition.invalid(href);

        let hostname_slice: []const u8 = "";
        let port_slice: []const u8 = "";
        let host_slice: []const u8 = "";
        let is_default_port = false;

        if (host_port_part[0] == '[') {
            // IPv6 host literal
            const close_opt = std.mem.indexOfScalar(u8, host_port_part, ']');
            if (close_opt == null) return HyperlinkDecomposition.invalid(href);
            const cb = close_opt.?;
            hostname_slice = host_port_part[0 .. cb + 1];

            if (cb + 1 == host_port_part.len) {
                port_slice = "";
                host_slice = hostname_slice;
            } else if (host_port_part[cb + 1] == ':') {
                const p_raw = host_port_part[cb + 2 ..];
                if (p_raw.len == 0) return HyperlinkDecomposition.invalid(href);
                let pi: usize = 0;
                while (pi < p_raw.len) : (pi += 1) {
                    const pc = p_raw[pi];
                    if (pc < '0' || pc > '9') return HyperlinkDecomposition.invalid(href);
                }
                const p_num = std.fmt.parseInt(u16, p_raw, 10) catch return HyperlinkDecomposition.invalid(href);

                if ((is_http || is_ws) && p_num == 80) {
                    is_default_port = true;
                } else if ((is_https || is_wss) && p_num == 443) {
                    is_default_port = true;
                } else if (is_ftp && p_num == 21) {
                    is_default_port = true;
                }

                if (is_default_port) {
                    port_slice = "";
                    host_slice = hostname_slice;
                } else {
                    port_slice = p_raw;
                    host_slice = host_port_part;
                }
            } else {
                return HyperlinkDecomposition.invalid(href);
            }
        } else {
            // Normal host name: count colons to forbid extra colons like ":80:"
            let colon_count: usize = 0;
            let colon_pos: usize = 0;
            let ci: usize = 0;
            while (ci < host_port_part.len) : (ci += 1) {
                if (host_port_part[ci] == ':') {
                    colon_count += 1;
                    colon_pos = ci;
                }
            }

            if (colon_count > 1) {
                return HyperlinkDecomposition.invalid(href);
            }

            if (colon_count == 1) {
                hostname_slice = host_port_part[0..colon_pos];
                const p_raw = host_port_part[colon_pos + 1 ..];
                if (hostname_slice.len == 0 || p_raw.len == 0) return HyperlinkDecomposition.invalid(href);

                let pi: usize = 0;
                while (pi < p_raw.len) : (pi += 1) {
                    const pc = p_raw[pi];
                    if (pc < '0' || pc > '9') return HyperlinkDecomposition.invalid(href);
                }
                const p_num = std.fmt.parseInt(u16, p_raw, 10) catch return HyperlinkDecomposition.invalid(href);

                if ((is_http || is_ws) && p_num == 80) {
                    is_default_port = true;
                } else if ((is_https || is_wss) && p_num == 443) {
                    is_default_port = true;
                } else if (is_ftp && p_num == 21) {
                    is_default_port = true;
                }

                if (is_default_port) {
                    port_slice = "";
                    host_slice = hostname_slice;
                } else {
                    port_slice = p_raw;
                    host_slice = host_port_part;
                }
            } else {
                hostname_slice = host_port_part;
                port_slice = "";
                host_slice = hostname_slice;
            }
        }

        // Validate hostname characters
        if (hostname_slice.len == 0) return HyperlinkDecomposition.invalid(href);
        let hi: usize = 0;
        while (hi < hostname_slice.len) : (hi += 1) {
            const hc = hostname_slice[hi];
            if (hc <= 0x20 || hc >= 0x7F) return HyperlinkDecomposition.invalid(href);
            if (hc == '/' || hc == '\\' || hc == '?' || hc == '#' || hc == '@' ||
                hc == '<' || hc == '>' || hc == '"' || hc == '^' || hc == '`' ||
                hc == '{' || hc == '}' || hc == '|') {
                return HyperlinkDecomposition.invalid(href);
            }
        }

        // 4. Compute origin
        let origin_slice: []const u8 = "null";
        if (is_http || is_https) {
            if (is_default_port) {
                origin_slice = href[0 .. colon_idx + 3 + hostname_slice.len];
            } else {
                origin_slice = href[0 .. colon_idx + 3 + host_slice.len];
            }
        }

        // 5. Compute pathname, search, and hash
        const tail = after_slashes[auth_end..];
        let pathname_slice: []const u8 = "/";
        let search_slice: []const u8 = "";
        let hash_slice: []const u8 = "";

        if (tail.len > 0) {
            let before_hash = tail;
            if (h_idx := std.mem.indexOfScalar(u8, tail, '#')) {
                hash_slice = tail[h_idx..];
                before_hash = tail[0..h_idx];
            }

            let path_candidate = before_hash;
            if (q_idx := std.mem.indexOfScalar(u8, before_hash, '?')) {
                search_slice = before_hash[q_idx..];
                path_candidate = before_hash[0..q_idx];
            }

            if (path_candidate.len > 0) {
                pathname_slice = path_candidate;
            } else {
                pathname_slice = "/";
            }
        }

        return HyperlinkDecomposition{
            href: href,
            protocol: protocol,
            host: host_slice,
            hostname: hostname_slice,
            port: port_slice,
            pathname: pathname_slice,
            search: search_slice,
            hash: hash_slice,
            origin: origin_slice,
        };
    }
}

pub fn decomposeHyperlink(href: []const u8): HyperlinkDecomposition {
    return HyperlinkDecomposition.decompose(href);
}
