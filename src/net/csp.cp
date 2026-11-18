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
