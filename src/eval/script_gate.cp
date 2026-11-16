// Migrated from vendor/kitesurf/packages/engine/src/eval/script-gate.ts
// Builds document script policy and gates scripts before fetch.

import { CspPolicy, CspDirective, parseCsp, isSourceAllowed } from "../net/csp.cp";

pub struct GatedScriptDecision {
    allowed: bool = true,
    reason: []const u8 = "",
    message: []const u8 = "",
}

fn isSchemeChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
}

fn hasExplicitScheme(s: []const u8): bool {
    if (s.len < 2) return false;
    const first = s[0];
    if (!((first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z'))) return false;
    let i: usize = 1;
    while (i < s.len) : (i += 1) {
        if (s[i] == ':') return true;
        if (!isSchemeChar(s[i])) return false;
    }
    return false;
}

fn indexOfQueryOrHash(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '?' || s[i] == '#') return i;
    }
    return null;
}

fn normalizePathSegments(prefix: []const u8, suffix: []const u8, out: []u8): []const u8 {
    let segments: [64][]const u8 = [""; 64];
    let seg_count: usize = 0;

    let p_it = std.mem.splitScalar(u8, prefix, '/');
    while (seg := p_it.next()) {
        if (seg.len == 0 || std.mem.eql(u8, seg, ".")) continue;
        if (std.mem.eql(u8, seg, "..")) {
            if (seg_count > 0) seg_count -= 1;
        } else {
            if (seg_count < 64) {
                segments[seg_count] = seg;
                seg_count += 1;
            }
        }
    }

    let s_it = std.mem.splitScalar(u8, suffix, '/');
    while (seg := s_it.next()) {
        if (seg.len == 0 || std.mem.eql(u8, seg, ".")) continue;
        if (std.mem.eql(u8, seg, "..")) {
            if (seg_count > 0) seg_count -= 1;
        } else {
            if (seg_count < 64) {
                segments[seg_count] = seg;
                seg_count += 1;
            }
        }
    }

    const target_slice = if (suffix.len > 0) suffix else prefix;
    const ends_with_slash = std.mem.endsWith(u8, target_slice, "/") ||
        std.mem.endsWith(u8, target_slice, "/.") ||
        std.mem.eql(u8, target_slice, ".") ||
        std.mem.endsWith(u8, target_slice, "/..") ||
        std.mem.eql(u8, target_slice, "..");

    if (out.len == 0) return "";
    out[0] = '/';
    let out_len: usize = 1;

    let s_i: usize = 0;
    while (s_i < seg_count) : (s_i += 1) {
        const s = segments[s_i];
        if (s_i > 0) {
            if (out_len >= out.len) break;
            out[out_len] = '/';
            out_len += 1;
        }
        const copy_len = @min(s.len, out.len - out_len);
        @memcpy(out[out_len .. out_len + copy_len], s[0..copy_len]);
        out_len += copy_len;
    }

    if (ends_with_slash && seg_count > 0 && out_len < out.len) {
        out[out_len] = '/';
        out_len += 1;
    }

    return out[0..out_len];
}

pub fn resolveUrl(rel: []const u8, base: []const u8, out_buf: []u8): []const u8 {
    if (rel.len == 0) return base;
    const r = std.mem.trim(u8, rel, " \t\r\n");
    if (r.len == 0) return base;

    if (hasExplicitScheme(r)) {
        const copy_len = @min(r.len, out_buf.len);
        @memcpy(out_buf[0..copy_len], r[0..copy_len]);
        return out_buf[0..copy_len];
    }

    if (base.len == 0) return "";

    // Protocol-relative URL: starts with "//"
    if (std.mem.startsWith(u8, r, "//")) {
        const colon_opt = std.mem.indexOfScalar(u8, base, ':');
        if (colon_opt == null) return "";
        const scheme = base[0 .. colon_opt.? + 1]; // e.g. "https:"
        const total = scheme.len + r.len;
        if (total > out_buf.len) return "";
        @memcpy(out_buf[0..scheme.len], scheme);
        @memcpy(out_buf[scheme.len..total], r);
        return out_buf[0..total];
    }

    // Parse base URL into origin and after_auth
    let origin: []const u8 = "";
    let after_auth: []const u8 = "";

    const proto_end = std.mem.indexOf(u8, base, "://");
    if (proto_end != null) {
        const p_idx = proto_end.?;
        const rem = base[p_idx + 3 ..];
        let auth_end: usize = rem.len;
        let i: usize = 0;
        while (i < rem.len) : (i += 1) {
            const c = rem[i];
            if (c == '/' || c == '?' || c == '#') {
                auth_end = i;
                break;
            }
        }
        origin = base[0 .. p_idx + 3 + auth_end];
        after_auth = rem[auth_end..];
    } else if (colon_idx := std.mem.indexOfScalar(u8, base, ':')) {
        origin = base[0 .. colon_idx + 1];
        after_auth = base[colon_idx + 1 ..];
    } else {
        origin = "";
        after_auth = base;
    }

    // If relative URL starts with '#', preserve base query if present
    if (r[0] == '#') {
        let base_path_and_query = after_auth;
        if (hash_pos := std.mem.indexOfScalar(u8, after_auth, '#')) {
            base_path_and_query = after_auth[0..hash_pos];
        }
        const total = origin.len + base_path_and_query.len + r.len;
        if (total > out_buf.len) return "";
        @memcpy(out_buf[0..origin.len], origin);
        @memcpy(out_buf[origin.len .. origin.len + base_path_and_query.len], base_path_and_query);
        @memcpy(out_buf[origin.len + base_path_and_query.len .. total], r);
        return out_buf[0..total];
    }

    // Extract base_path without query or hash
    let base_path = after_auth;
    if (qh_idx := indexOfQueryOrHash(after_auth)) {
        base_path = after_auth[0..qh_idx];
    }
    if (base_path.len == 0) {
        base_path = "/";
    }

    // Split r into rel_path and rel_query_hash
    let rel_path = r;
    let rel_query_hash: []const u8 = "";
    if (qh_idx := indexOfQueryOrHash(r)) {
        rel_path = r[0..qh_idx];
        rel_query_hash = r[qh_idx..];
    }

    // If r starts with '?', base_path is preserved
    let path_buf: [1024]u8 = [0; 1024];
    let norm_path: []const u8 = "";
    let base_dir: []const u8 = "/";
    if (slash_idx := std.mem.lastIndexOfScalar(u8, base_path, '/')) {
        base_dir = base_path[0 .. slash_idx + 1];
    }

    if (r[0] == '?') {
        norm_path = base_path;
    } else if (std.mem.startsWith(u8, rel_path, "/")) {
        norm_path = normalizePathSegments("", rel_path, path_buf[0..]);
    } else {
        norm_path = normalizePathSegments(base_dir, rel_path, path_buf[0..]);
    }

    const total = origin.len + norm_path.len + rel_query_hash.len;
    if (total > out_buf.len) return "";
    @memcpy(out_buf[0..origin.len], origin);
    @memcpy(out_buf[origin.len .. origin.len + norm_path.len], norm_path);
    @memcpy(out_buf[origin.len + norm_path.len .. total], rel_query_hash);
    return out_buf[0..total];
}

pub fn documentBaseUrl(page_url: []const u8, base_href: []const u8, out_buf: []u8): []const u8 {
    if (base_href.len == 0) return page_url;
    const resolved = resolveUrl(base_href, page_url, out_buf);
    if (resolved.len == 0) return page_url;
    return resolved;
}

pub fn resolveScriptUrl(src: []const u8, base_url: []const u8, out_buf: []u8): []const u8 {
    if (src.len == 0) return "";
    return resolveUrl(src, base_url, out_buf);
}

pub fn checkScriptPolicy(url: []const u8, nonce: []const u8, integrity: []const u8, csp_policy: []const u8): GatedScriptDecision {
    if (csp_policy.len == 0) {
        return GatedScriptDecision{
            allowed: true,
            reason: "",
            message: "",
        };
    }

    const policy = parseCsp(csp_policy);

    let script_dir_idx: usize? = null;

    let i: usize = 0;
    while (i < policy.directive_count) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(policy.directives[i].name, "script-src")) {
            script_dir_idx = i;
            break;
        }
    }
    if (script_dir_idx == null) {
        let j: usize = 0;
        while (j < policy.directive_count) : (j += 1) {
            if (std.ascii.eqlIgnoreCase(policy.directives[j].name, "default-src")) {
                script_dir_idx = j;
                break;
            }
        }
    }

    if (script_dir_idx == null) {
        return GatedScriptDecision{
            allowed: true,
            reason: "",
            message: "",
        };
    }

    const script_dir = &policy.directives[script_dir_idx.?];

    if (script_dir.count == 0) {
        return GatedScriptDecision{
            allowed: false,
            reason: "csp",
            message: "blocked by empty script directive",
        };
    }

    if (script_dir.count == 1 && std.ascii.eqlIgnoreCase(script_dir.values[0], "'none'")) {
        return GatedScriptDecision{
            allowed: false,
            reason: "csp",
            message: "blocked by 'none' script directive",
        };
    }

    if (nonce.len > 0) {
        let v_i: usize = 0;
        while (v_i < script_dir.count) : (v_i += 1) {
            const val = script_dir.values[v_i];
            if (std.mem.startsWith(u8, val, "'nonce-") && std.mem.endsWith(u8, val, "'") && val.len >= 8) {
                const inner_nonce = val[7 .. val.len - 1];
                if (std.mem.eql(u8, inner_nonce, nonce)) {
                    return GatedScriptDecision{
                        allowed: true,
                        reason: "",
                        message: "",
                    };
                }
            }
        }
    }

    if (integrity.len > 0) {
        let v_i: usize = 0;
        while (v_i < script_dir.count) : (v_i += 1) {
            const val = script_dir.values[v_i];
            if (val.len >= 2 && val[0] == '\'' && val[val.len - 1] == '\'') {
                const inner_hash = val[1 .. val.len - 1];
                if (std.mem.indexOf(u8, integrity, inner_hash) != null) {
                    return GatedScriptDecision{
                        allowed: true,
                        reason: "",
                        message: "",
                    };
                }
            }
        }
    }

    if (url.len > 0) {
        if (isSourceAllowed(&policy, "script-src", url)) {
            return GatedScriptDecision{
                allowed: true,
                reason: "",
                message: "",
            };
        }
        return GatedScriptDecision{
            allowed: false,
            reason: "csp",
            message: "blocked by script-src policy",
        };
    }

    // Inline script: check unsafe-inline
    let has_nonce_or_hash = false;
    let has_unsafe_inline = false;
    let v_i: usize = 0;
    while (v_i < script_dir.count) : (v_i += 1) {
        const val = script_dir.values[v_i];
        if (std.mem.startsWith(u8, val, "'nonce-") ||
            std.mem.startsWith(u8, val, "'sha256-") ||
            std.mem.startsWith(u8, val, "'sha384-") ||
            std.mem.startsWith(u8, val, "'sha512-"))
        {
            has_nonce_or_hash = true;
        }
        if (std.ascii.eqlIgnoreCase(val, "'unsafe-inline'")) {
            has_unsafe_inline = true;
        }
    }

    if (!has_nonce_or_hash && has_unsafe_inline) {
        return GatedScriptDecision{
            allowed: true,
            reason: "",
            message: "",
        };
    }

    return GatedScriptDecision{
        allowed: false,
        reason: "csp",
        message: "inline script blocked by CSP policy",
    };
}
