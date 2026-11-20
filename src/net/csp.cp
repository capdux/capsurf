// Migrated from vendor/kitesurf/packages/script-gate/src/csp.ts

pub struct CspDirective {
    name: []const u8 = "",
    values: [16][]const u8 = [""; 16],
    count: usize = 0,
}

pub struct CspPolicy {
    directives: [8]CspDirective = [CspDirective{}; 8],
    directive_count: usize = 0,
    raw_text: []const u8 = "",
}

fn nextToken(s: []const u8, start_idx: *usize): ([]const u8)? {
    while (start_idx.* < s.len) {
        const c = s[start_idx.*];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break;
        start_idx.* += 1;
    }
    if (start_idx.* >= s.len) return null;
    const token_start = start_idx.*;
    while (start_idx.* < s.len) {
        const c = s[start_idx.*];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') break;
        start_idx.* += 1;
    }
    return s[token_start .. start_idx.*];
}

pub fn defaultPortForScheme(scheme: []const u8): []const u8 {
    let s = scheme;
    if (std.mem.endsWith(u8, s, ":")) {
        s = s[0 .. s.len - 1];
    }
    if (std.ascii.eqlIgnoreCase(s, "http")) return "80";
    if (std.ascii.eqlIgnoreCase(s, "https")) return "443";
    if (std.ascii.eqlIgnoreCase(s, "ws")) return "80";
    if (std.ascii.eqlIgnoreCase(s, "wss")) return "443";
    if (std.ascii.eqlIgnoreCase(s, "ftp")) return "21";
    return "";
}

pub fn parseCsp(raw_header: []const u8): CspPolicy {
    let policy = CspPolicy{
        raw_text: raw_header,
    };
    let chunk_it = std.mem.splitScalar(u8, raw_header, ';');
    while (chunk_opt := chunk_it.next()) {
        const chunk = std.mem.trim(u8, chunk_opt, " \t\r\n");
        if (chunk.len == 0) continue;

        let start_idx: usize = 0;
        const name_opt = nextToken(chunk, &start_idx);
        if (name_opt == null) continue;
        const name = name_opt.?;

        let already_exists = false;
        let d_i: usize = 0;
        while (d_i < policy.directive_count) : (d_i += 1) {
            if (std.ascii.eqlIgnoreCase(policy.directives[d_i].name, name)) {
                already_exists = true;
                break;
            }
        }
        if (already_exists) continue;

        if (policy.directive_count >= 8) break;

        let directive = CspDirective{
            name: name,
        };

        while (val_opt := nextToken(chunk, &start_idx)) {
            if (directive.count < 16) {
                directive.values[directive.count] = val_opt;
                directive.count += 1;
            }
        }

        policy.directives[policy.directive_count] = directive;
        policy.directive_count += 1;
    }
    return policy;
}

pub fn schemeMatches(source_scheme: []const u8, target_scheme: []const u8): bool {
    let s1 = source_scheme;
    if (std.mem.endsWith(u8, s1, ":")) {
        s1 = s1[0 .. s1.len - 1];
    }
    let s2 = target_scheme;
    if (std.mem.endsWith(u8, s2, ":")) {
        s2 = s2[0 .. s2.len - 1];
    }

    if (std.ascii.eqlIgnoreCase(s1, s2)) return true;
    if (std.ascii.eqlIgnoreCase(s1, "http") && std.ascii.eqlIgnoreCase(s2, "https")) return true;
    if (std.ascii.eqlIgnoreCase(s1, "ws") && std.ascii.eqlIgnoreCase(s2, "wss")) return true;
    if (std.ascii.eqlIgnoreCase(s1, "ws") && std.ascii.eqlIgnoreCase(s2, "http")) return true;
    if (std.ascii.eqlIgnoreCase(s1, "ws") && std.ascii.eqlIgnoreCase(s2, "https")) return true;
    if (std.ascii.eqlIgnoreCase(s1, "wss") && std.ascii.eqlIgnoreCase(s2, "https")) return true;
    return false;
}

pub fn hostMatches(source_host: []const u8, target_host: []const u8): bool {
    if (std.mem.eql(u8, source_host, "*")) return true;
    if (std.mem.startsWith(u8, source_host, "*.")) {
        const suffix = source_host[1..];
        if (target_host.len > suffix.len) {
            const target_suffix = target_host[target_host.len - suffix.len ..];
            return std.ascii.eqlIgnoreCase(target_suffix, suffix);
        }
        return false;
    }
    return std.ascii.eqlIgnoreCase(source_host, target_host);
}

fn cspPathMatches(source_path: []const u8, target_path: []const u8): bool {
    if (source_path.len == 0 || std.mem.eql(u8, source_path, "/")) return true;
    if (std.mem.endsWith(u8, source_path, "/")) {
        return std.mem.startsWith(u8, target_path, source_path);
    }
    return std.mem.eql(u8, target_path, source_path);
}

struct CspUrlView {
    protocol: []const u8 = "",
    hostname: []const u8 = "",
    port: []const u8 = "",
    pathname: []const u8 = "/",
}

fn parseCspUrl(url: []const u8): CspUrlView? {
    if (url.len == 0) return null;

    let full = url;
    if (h_idx := std.mem.indexOfScalar(u8, full, '#')) {
        full = full[0..h_idx];
    }

    const proto_sep = std.mem.indexOf(u8, full, "://");
    if (proto_sep == null) {
        if (c_idx := std.mem.indexOfScalar(u8, full, ':')) {
            return CspUrlView{
                protocol: full[0 .. c_idx + 1],
                pathname: full[c_idx + 1 ..],
            };
        }
        return null;
    }

    const p_idx = proto_sep.?;
    const protocol = full[0 .. p_idx + 1];
    let rem = full[p_idx + 3 ..];

    let path_start: usize = rem.len;
    let i: usize = 0;
    while (i < rem.len) : (i += 1) {
        const c = rem[i];
        if (c == '/' || c == '?') {
            path_start = i;
            break;
        }
    }

    const authority = rem[0..path_start];
    let pathname: []const u8 = "/";
    if (path_start < rem.len && rem[path_start] == '/') {
        let q_idx = path_start;
        while (q_idx < rem.len) : (q_idx += 1) {
            if (rem[q_idx] == '?') break;
        }
        pathname = rem[path_start..q_idx];
    }

    let host_and_port = authority;
    if (at_idx := std.mem.lastIndexOfScalar(u8, authority, '@')) {
        host_and_port = authority[at_idx + 1 ..];
    }

    let hostname = host_and_port;
    let port: []const u8 = "";

    if (host_and_port.len > 0 && host_and_port[0] == '[') {
        if (bracket_end := std.mem.indexOfScalar(u8, host_and_port, ']')) {
            hostname = host_and_port[0 .. bracket_end + 1];
            const after_bracket = host_and_port[bracket_end + 1 ..];
            if (after_bracket.len > 1 && after_bracket[0] == ':') {
                port = after_bracket[1..];
            }
        }
    } else {
        if (colon_idx := std.mem.lastIndexOfScalar(u8, host_and_port, ':')) {
            hostname = host_and_port[0..colon_idx];
            port = host_and_port[colon_idx + 1 ..];
        }
    }

    if (port.len == 0) {
        port = defaultPortForScheme(protocol);
    }

    return CspUrlView{
        protocol: protocol,
        hostname: hostname,
        port: port,
        pathname: pathname,
    };
}

pub fn isSourceAllowed(policy: *const CspPolicy, directive_name: []const u8, target_url: []const u8): bool {
    let target_dir_idx: usize? = null;
    let default_dir_idx: usize? = null;

    let i: usize = 0;
    while (i < policy.directive_count) : (i += 1) {
        const dir = &policy.directives[i];
        if (std.ascii.eqlIgnoreCase(dir.name, directive_name)) {
            target_dir_idx = i;
            break;
        }
        if (std.ascii.eqlIgnoreCase(dir.name, "default-src")) {
            default_dir_idx = i;
        }
    }

    let dir_idx: usize = 0;
    if (idx := target_dir_idx) {
        dir_idx = idx;
    } else if (idx := default_dir_idx) {
        dir_idx = idx;
    } else {
        return true;
    }

    const dir = &policy.directives[dir_idx];

    if (dir.count == 0) return false;
    if (dir.count == 1 && std.ascii.eqlIgnoreCase(dir.values[0], "'none'")) {
        return false;
    }

    const target_opt = parseCspUrl(target_url);
    if (target_opt == null) return false;
    const target = target_opt.?;

    let v_i: usize = 0;
    while (v_i < dir.count) : (v_i += 1) {
        const source = dir.values[v_i];
        if (std.ascii.eqlIgnoreCase(source, "'none'")) continue;

        if (std.mem.eql(u8, source, "*")) {
            if (std.ascii.eqlIgnoreCase(target.protocol, "http:") ||
                std.ascii.eqlIgnoreCase(target.protocol, "https:") ||
                std.ascii.eqlIgnoreCase(target.protocol, "ws:") ||
                std.ascii.eqlIgnoreCase(target.protocol, "wss:") ||
                std.ascii.eqlIgnoreCase(target.protocol, "ftp:"))
            {
                return true;
            }
            continue;
        }

        if (std.mem.startsWith(u8, source, "'")) continue;

        if (std.mem.endsWith(u8, source, ":")) {
            if (schemeMatches(source, target.protocol)) return true;
            continue;
        }

        let rem = source;
        let source_scheme: []const u8 = "";
        if (sep_idx := std.mem.indexOf(u8, rem, "://")) {
            source_scheme = rem[0 .. sep_idx + 1];
            rem = rem[sep_idx + 3 ..];
        }

        let path: []const u8 = "";
        if (p_start := std.mem.indexOfScalar(u8, rem, '/')) {
            path = rem[p_start..];
            rem = rem[0..p_start];
        }

        if (rem.len > 0 && rem[0] == '[') continue;

        let source_port: []const u8 = "";
        if (port_sep := std.mem.lastIndexOfScalar(u8, rem, ':')) {
            source_port = rem[port_sep + 1 ..];
            rem = rem[0..port_sep];
        }

        const source_host = rem;
        if (source_host.len == 0) continue;

        if (source_scheme.len > 0) {
            if (!schemeMatches(source_scheme, target.protocol)) continue;
        } else {
            if (!std.ascii.eqlIgnoreCase(target.protocol, "http:") && !std.ascii.eqlIgnoreCase(target.protocol, "https:")) {
                continue;
            }
        }

        if (!hostMatches(source_host, target.hostname)) continue;

        if (std.mem.eql(u8, source_port, "*")) {
            // wildcard port matches
        } else if (source_port.len > 0) {
            if (!std.mem.eql(u8, source_port, target.port)) continue;
        } else if (source_scheme.len == 0) {
            const def_port = defaultPortForScheme(target.protocol);
            if (!std.mem.eql(u8, target.port, def_port)) continue;
        } else {
            const src_def = defaultPortForScheme(source_scheme);
            if (!std.mem.eql(u8, target.port, src_def)) {
                const is_http_to_https = std.ascii.eqlIgnoreCase(source_scheme, "http:") &&
                                        std.ascii.eqlIgnoreCase(target.protocol, "https:") &&
                                        std.mem.eql(u8, target.port, "443");
                if (!is_http_to_https) continue;
            }
        }

        if (cspPathMatches(path, target.pathname)) return true;
    }

    return false;
}

pub fn computeHashBase64(algo: []const u8, data: []const u8, out_buf: []u8): ([]const u8)? {
    if (std.ascii.eqlIgnoreCase(algo, "sha256")) {
        let digest: [32]u8 = [0; 32];
        std.crypto.hash.sha2.Sha256.hash(data, &digest, .{});
        const enc_len = std.base64.standard.Encoder.calcSize(32);
        if (out_buf.len < enc_len) return null;
        return std.base64.standard.Encoder.encode(out_buf[0..enc_len], &digest);
    }
    if (std.ascii.eqlIgnoreCase(algo, "sha384")) {
        let digest: [48]u8 = [0; 48];
        std.crypto.hash.sha2.Sha384.hash(data, &digest, .{});
        const enc_len = std.base64.standard.Encoder.calcSize(48);
        if (out_buf.len < enc_len) return null;
        return std.base64.standard.Encoder.encode(out_buf[0..enc_len], &digest);
    }
    if (std.ascii.eqlIgnoreCase(algo, "sha512")) {
        let digest: [64]u8 = [0; 64];
        std.crypto.hash.sha2.Sha512.hash(data, &digest, .{});
        const enc_len = std.base64.standard.Encoder.calcSize(64);
        if (out_buf.len < enc_len) return null;
        return std.base64.standard.Encoder.encode(out_buf[0..enc_len], &digest);
    }
    return null;
}

pub fn verifySriIntegrity(integrity_attr: []const u8, body: []const u8): bool {
    const trimmed = std.mem.trim(u8, integrity_attr, " \t\r\n");
    if (trimmed.len == 0) return true;
    let idx: usize = 0;
    let saw_supported = false;
    while (tok := nextToken(trimmed, &idx)) {
        if (dash := std.mem.indexOfScalar(u8, tok, '-')) {
            const algo = tok[0..dash];
            let expected = tok[dash + 1 ..];
            if (q := std.mem.indexOfScalar(u8, expected, '?')) {
                expected = expected[0..q];
            }
            if (std.ascii.eqlIgnoreCase(algo, "sha256") ||
                std.ascii.eqlIgnoreCase(algo, "sha384") ||
                std.ascii.eqlIgnoreCase(algo, "sha512"))
            {
                saw_supported = true;
                let b64_buf: [128]u8 = [0; 128];
                if (actual := computeHashBase64(algo, body, &b64_buf)) {
                    if (std.mem.eql(u8, actual, expected)) {
                        return true;
                    }
                }
            }
        }
    }
    return !saw_supported;
}

pub fn isScriptNosniffBlocked(url: []const u8): bool {
    const q_pos_opt = std.mem.indexOfScalar(u8, url, '?');
    if (q_pos_opt == null) return false;
    const query = url[q_pos_opt.? + 1 ..];
    let has_nosniff = false;
    let ct_val: []const u8 = "";
    let it = std.mem.splitScalar(u8, query, '&');
    while (pair := it.next()) {
        if (eq := std.mem.indexOfScalar(u8, pair, '=')) {
            const k = pair[0..eq];
            const v = pair[eq + 1 ..];
            if (std.mem.eql(u8, k, "nosniff") && (std.mem.eql(u8, v, "1") || std.ascii.eqlIgnoreCase(v, "true"))) {
                has_nosniff = true;
            } else if (std.mem.eql(u8, k, "ct")) {
                ct_val = v;
            }
        }
    }
    if (!has_nosniff || ct_val.len == 0) return false;
    if (std.mem.indexOf(u8, ct_val, "javascript") != null ||
        std.mem.indexOf(u8, ct_val, "ecmascript") != null)
    {
        return false;
    }
    return true;
}

fn findEffectiveDirective(policy: *const CspPolicy, primary: []const u8, fallback1: []const u8, fallback2: []const u8): (*const CspDirective)? {
    let i: usize = 0;
    while (i < policy.directive_count) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(policy.directives[i].name, primary)) {
            return &policy.directives[i];
        }
    }
    if (fallback1.len > 0) {
        i = 0;
        while (i < policy.directive_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(policy.directives[i].name, fallback1)) {
                return &policy.directives[i];
            }
        }
    }
    if (fallback2.len > 0) {
        i = 0;
        while (i < policy.directive_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(policy.directives[i].name, fallback2)) {
                return &policy.directives[i];
            }
        }
    }
    return null;
}

fn stripSingleQuotes(s: []const u8): []const u8 {
    if (s.len >= 2 && s[0] == '\'' && s[s.len - 1] == '\'') {
        return s[1 .. s.len - 1];
    }
    return s;
}

fn isNonceSource(s: []const u8): bool {
    const inner = stripSingleQuotes(s);
    return inner.len > 6 && std.ascii.eqlIgnoreCase(inner[0..6], "nonce-");
}

fn isHashSource(s: []const u8): bool {
    const inner = stripSingleQuotes(s);
    return (inner.len > 7 && std.ascii.eqlIgnoreCase(inner[0..7], "sha256-")) ||
        (inner.len > 7 && std.ascii.eqlIgnoreCase(inner[0..7], "sha384-")) ||
        (inner.len > 7 && std.ascii.eqlIgnoreCase(inner[0..7], "sha512-"));
}

fn matchesHashSource(s: []const u8, body: []const u8): bool {
    const inner = stripSingleQuotes(s);
    if (dash := std.mem.indexOfScalar(u8, inner, '-')) {
        const algo = inner[0..dash];
        const expected = inner[dash + 1 ..];
        let b64_buf: [128]u8 = [0; 128];
        if (actual := computeHashBase64(algo, body, &b64_buf)) {
            return std.mem.eql(u8, actual, expected);
        }
    }
    return false;
}

fn matchesNonceSource(s: []const u8, nonce: []const u8): bool {
    if (nonce.len == 0) return false;
    const inner = stripSingleQuotes(s);
    if (inner.len > 6 && std.ascii.eqlIgnoreCase(inner[0..6], "nonce-")) {
        return std.mem.eql(u8, inner[6..], nonce);
    }
    return false;
}

pub fn isInlineScriptAllowed(policy: *const CspPolicy, nonce: []const u8, body: []const u8): bool {
    const dir_opt = findEffectiveDirective(policy, "script-src-elem", "script-src", "default-src");
    if (dir_opt == null) return true;
    const dir = dir_opt.?;
    if (dir.count == 0) return false;

    let has_unsafe_inline = false;
    let has_nonce_or_hash = false;
    let i: usize = 0;
    while (i < dir.count) : (i += 1) {
        const src = dir.values[i];
        if (std.ascii.eqlIgnoreCase(src, "'unsafe-inline'")) {
            has_unsafe_inline = true;
        } else if (isNonceSource(src)) {
            has_nonce_or_hash = true;
            if (matchesNonceSource(src, nonce)) return true;
        } else if (isHashSource(src)) {
            has_nonce_or_hash = true;
            if (matchesHashSource(src, body)) return true;
        }
    }
    return has_unsafe_inline && !has_nonce_or_hash;
}

pub fn isInlineEventHandlerAllowed(policy: *const CspPolicy): bool {
    const dir_opt = findEffectiveDirective(policy, "script-src-attr", "script-src", "default-src");
    if (dir_opt == null) return true;
    const dir = dir_opt.?;
    if (dir.count == 0) return false;

    let has_unsafe_inline = false;
    let has_nonce_or_hash = false;
    let i: usize = 0;
    while (i < dir.count) : (i += 1) {
        const src = dir.values[i];
        if (std.ascii.eqlIgnoreCase(src, "'unsafe-inline'")) {
            has_unsafe_inline = true;
        } else if (isNonceSource(src) || isHashSource(src)) {
            has_nonce_or_hash = true;
        }
    }
    return has_unsafe_inline && !has_nonce_or_hash;
}

fn urlsSameOrigin(url_a: []const u8, url_b: []const u8): bool {
    const p1 = parseCspUrl(url_a) orelse return false;
    const p2 = parseCspUrl(url_b) orelse return false;
    return std.ascii.eqlIgnoreCase(p1.protocol, p2.protocol) &&
        std.ascii.eqlIgnoreCase(p1.hostname, p2.hostname) &&
        std.mem.eql(u8, p1.port, p2.port);
}

pub fn isExternalScriptAllowed(
    policy: *const CspPolicy,
    doc_origin: []const u8,
    target_url: []const u8,
    nonce: []const u8,
    is_redirect_hop: bool,
    is_parser_inserted: bool,
): bool {
    const dir_opt = findEffectiveDirective(policy, "script-src-elem", "script-src", "default-src");
    if (dir_opt == null) return true;
    const dir = dir_opt.?;
    if (dir.count == 0) return false;

    let has_strict_dynamic = false;
    let i: usize = 0;
    while (i < dir.count) : (i += 1) {
        const src = dir.values[i];
        if (std.ascii.eqlIgnoreCase(src, "'strict-dynamic'")) {
            has_strict_dynamic = true;
        } else if (isNonceSource(src)) {
            if (matchesNonceSource(src, nonce)) return true;
        }
    }

    if (has_strict_dynamic) {
        if (!is_parser_inserted) return true;
        return false;
    }

    const target_opt = parseCspUrl(target_url);
    if (target_opt == null) return false;
    const target = target_opt.?;

    i = 0;
    while (i < dir.count) : (i += 1) {
        const source = dir.values[i];
        if (std.ascii.eqlIgnoreCase(source, "'none'")) continue;
        if (std.ascii.eqlIgnoreCase(source, "'self'")) {
            if (doc_origin.len > 0 && urlsSameOrigin(doc_origin, target_url)) return true;
            continue;
        }
        if (std.mem.eql(u8, source, "*")) {
            if (std.ascii.eqlIgnoreCase(target.protocol, "http:") ||
                std.ascii.eqlIgnoreCase(target.protocol, "https:") ||
                std.ascii.eqlIgnoreCase(target.protocol, "ws:") ||
                std.ascii.eqlIgnoreCase(target.protocol, "wss:") ||
                std.ascii.eqlIgnoreCase(target.protocol, "ftp:"))
            {
                return true;
            }
            continue;
        }
        if (std.mem.startsWith(u8, source, "'")) continue;
        if (std.mem.endsWith(u8, source, ":")) {
            if (schemeMatches(source, target.protocol)) return true;
            continue;
        }

        let rem = source;
        let source_scheme: []const u8 = "";
        if (sep_idx := std.mem.indexOf(u8, rem, "://")) {
            source_scheme = rem[0 .. sep_idx + 1];
            rem = rem[sep_idx + 3 ..];
        }
        let path: []const u8 = "";
        if (p_start := std.mem.indexOfScalar(u8, rem, '/')) {
            path = rem[p_start..];
            rem = rem[0..p_start];
        }
        if (rem.len > 0 && rem[0] == '[') continue;
        let source_port: []const u8 = "";
        if (port_sep := std.mem.lastIndexOfScalar(u8, rem, ':')) {
            source_port = rem[port_sep + 1 ..];
            rem = rem[0..port_sep];
        }
        const source_host = rem;
        if (source_host.len == 0) continue;

        if (source_scheme.len > 0) {
            if (!schemeMatches(source_scheme, target.protocol)) continue;
        } else {
            if (!std.ascii.eqlIgnoreCase(target.protocol, "http:") && !std.ascii.eqlIgnoreCase(target.protocol, "https:")) {
                continue;
            }
        }
        if (!hostMatches(source_host, target.hostname)) continue;

        if (std.mem.eql(u8, source_port, "*")) {
            // wildcard port matches
        } else if (source_port.len > 0) {
            if (!std.mem.eql(u8, source_port, target.port)) continue;
        } else if (source_scheme.len == 0) {
            const def_port = defaultPortForScheme(target.protocol);
            if (!std.mem.eql(u8, target.port, def_port)) continue;
        } else {
            const src_def = defaultPortForScheme(source_scheme);
            if (!std.mem.eql(u8, target.port, src_def)) {
                const is_http_to_https = std.ascii.eqlIgnoreCase(source_scheme, "http:") &&
                                        std.ascii.eqlIgnoreCase(target.protocol, "https:") &&
                                        std.mem.eql(u8, target.port, "443");
                if (!is_http_to_https) continue;
            }
        }

        // CSP3 §6.6.1.2: after a redirect, ignore the path part of the source expression
        if (is_redirect_hop || cspPathMatches(path, target.pathname)) return true;
    }

    return false;
}
