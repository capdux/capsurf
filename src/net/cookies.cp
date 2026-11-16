// src/net/cookies.cp
// CapSurf RFC 6265 CookieJar Engine & CDP Network Interceptor
// Implements secure origin validation, eTLD+1 domain bounds, prefix invariants,
// SameSite context checks, and HttpOnly shadow protection.

import { isIpHost, isPublicSuffix, siteKey, schemefulSite } from "./public_suffix.cp";

@c {
    fn time(t: *!?): c_long;
}

pub const MAX_COOKIE_BYTES: usize = 4096;
pub const MAX_COOKIES_PER_HOST: usize = 180;
pub const MAX_COOKIES_TOTAL: usize = 3000;
pub const MILLISECONDS_PER_SECOND: i64 = 1000;

pub enum SameSite {
    Strict,
    Lax,
    None,
}

pub struct CookieEntry {
    name: []const u8 = "",
    value: []const u8 = "",
    host: []const u8 = "",
    path: []const u8 = "/",
    secure: bool = false,
    http_only: bool = false,
    same_site: SameSite? = null,
    host_only: bool = false,
    expires_at: i64? = null,
    created_at: i64 = 0,
}

pub struct RequestContext {
    initiator_origin: []const u8 = "",
    top_level: bool = false,
    method: []const u8 = "GET",
}

pub struct CdpCookie {
    name: []const u8 = "",
    value: []const u8 = "",
    domain: []const u8 = "",
    path: []const u8 = "/",
    expires: f64 = -1.0,
    size: usize = 0,
    http_only: bool = false,
    secure: bool = false,
    session: bool = false,
    same_site: []const u8 = "",
}

pub struct UrlParts {
    host: []const u8 = "",
    path: []const u8 = "/",
    secure: bool = false,
    trustworthy: bool = false,
}

pub fn isTrustworthy(host: []const u8, secure: bool): bool {
    if (secure) return true;
    if (std.mem.eql(u8, host, "localhost") || std.mem.endsWith(u8, host, ".localhost")) return true;
    if (std.mem.eql(u8, host, "[::1]") || std.mem.eql(u8, host, "::1")) return true;
    if (std.mem.startsWith(u8, host, "127.")) return true;
    return false;
}

pub fn parseUrlParts(url_str: []const u8): UrlParts? {
    let secure = false;
    let rest: []const u8 = "";
    if (std.mem.startsWith(u8, url_str, "https://")) {
        secure = true;
        rest = url_str[8..];
    } else if (std.mem.startsWith(u8, url_str, "http://")) {
        secure = false;
        rest = url_str[7..];
    } else {
        return null;
    }

    let host_end: usize = 0;
    while (host_end < rest.len) : (host_end += 1) {
        const ch = rest[host_end];
        if (ch == '/' || ch == ':' || ch == '?' || ch == '#') break;
    }
    const host = rest[0..host_end];
    if (host.len == 0) return null;

    let path: []const u8 = "/";
    if (host_end < rest.len && rest[host_end] == '/') {
        let path_end = host_end;
        while (path_end < rest.len) : (path_end += 1) {
            const ch = rest[path_end];
            if (ch == '?' || ch == '#') break;
        }
        path = rest[host_end..path_end];
    }

    return UrlParts {
        host: host,
        path: path,
        secure: secure,
        trustworthy: isTrustworthy(host, secure),
    };
}

pub fn pathMatches(cookie_path: []const u8, request_path: []const u8): bool {
    if (std.mem.eql(u8, cookie_path, request_path)) return true;
    if (std.mem.startsWith(u8, request_path, cookie_path)) {
        if (cookie_path.len > 0 && cookie_path[cookie_path.len - 1] == '/') return true;
        if (request_path.len > cookie_path.len && request_path[cookie_path.len] == '/') return true;
    }
    return false;
}

pub fn domainMatches(cookie_host: []const u8, host: []const u8): bool {
    if (std.mem.eql(u8, cookie_host, host)) return true;
    if (host.len > cookie_host.len + 1) {
        const suffix = host[host.len - cookie_host.len..];
        const dot = host[host.len - cookie_host.len - 1];
        if (dot == '.' && std.mem.eql(u8, suffix, cookie_host)) return true;
    }
    return false;
}

pub fn resolveDomain(claimed: []const u8, host: []const u8): ([]const u8)? {
    let raw = claimed;
    if (raw.len > 0 && raw[0] == '.') raw = raw[1..];
    if (raw.len > 0 && raw[raw.len - 1] == '.') raw = raw[0..raw.len - 1];
    if (raw.len == 0) return null;
    if (std.mem.eql(u8, raw, host)) return raw;
    if (isIpHost(host)) return null;

    for (ch of raw) {
        if (ch == '/' || ch == '\\' || ch == ' ' || ch == ':' || ch == '@' || ch == '?' || ch == '#' || ch == '%' || ch == '[' || ch == ']') {
            return null;
        }
    }

    if (isIpHost(raw) || !domainMatches(raw, host) || isPublicSuffix(raw)) return null;
    return raw;
}

fn isValidCookieName(s: []const u8): bool {
    if (s.len == 0) return false;
    for (ch of s) {
        if (ch < 0x21 || ch > 0x7e) return false;
        if (ch == '(' || ch == ')' || ch == ',' || ch == '/') return false;
        if (ch >= ':' && ch <= '@') return false;
        if (ch == '[' || ch == '\\' || ch == ']') return false;
        if (ch == '{' || ch == '}') return false;
        if (ch == '"') return false;
    }
    return true;
}

fn isValidCookieValue(s: []const u8): bool {
    let v = s;
    if (v.len >= 2 && v[0] == '"' && v[v.len - 1] == '"') {
        v = v[1..v.len - 1];
    }
    for (ch of v) {
        if (ch < 0x21 || ch > 0x7e) return false;
        if (ch == '"' || ch == ',' || ch == ';' || ch == '\\') return false;
    }
    return true;
}

pub struct ParsedSetCookie {
    name: []const u8 = "",
    value: []const u8 = "",
    domain: []const u8 = "",
    has_domain: bool = false,
    path: []const u8 = "/",
    secure: bool = false,
    http_only: bool = false,
    same_site: SameSite? = null,
    has_same_site: bool = false,
    max_age: i64? = null,
    has_max_age: bool = false,
    expires: []const u8 = "",
}

pub fn parseSetCookie(raw: []const u8): ParsedSetCookie? {
    if (raw.len == 0) return null;
    let it = std.mem.splitScalar(u8, raw, ';');
    const first_opt = it.next();
    if (first_opt == null) return null;
    const first = std.mem.trim(u8, first_opt.?, " \t");
    const eq = std.mem.indexOfScalar(u8, first, '=');
    if (eq == null) return null;

    const name = std.mem.trim(u8, first[0..eq.?], " \t");
    const val = std.mem.trim(u8, first[eq.? + 1..], " \t");
    if (!isValidCookieName(name) || !isValidCookieValue(val)) return null;

    let res = ParsedSetCookie {
        name: name,
        value: val,
    };

    while (seg := it.next()) {
        const s = std.mem.trim(u8, seg, " \t");
        if (s.len == 0) continue;
        const attr_eq = std.mem.indexOfScalar(u8, s, '=');
        let attr_name = s;
        let attr_val: []const u8 = "";
        if (attr_eq != null) {
            attr_name = std.mem.trim(u8, s[0..attr_eq.?], " \t");
            attr_val = std.mem.trim(u8, s[attr_eq.? + 1..], " \t");
        }

        if (std.ascii.eqlIgnoreCase(attr_name, "domain")) {
            res.domain = attr_val;
            res.has_domain = true;
        } else if (std.ascii.eqlIgnoreCase(attr_name, "path")) {
            res.path = if (attr_val.len > 0) attr_val else "/";
        } else if (std.ascii.eqlIgnoreCase(attr_name, "secure")) {
            res.secure = true;
        } else if (std.ascii.eqlIgnoreCase(attr_name, "httponly")) {
            res.http_only = true;
        } else if (std.ascii.eqlIgnoreCase(attr_name, "samesite")) {
            res.has_same_site = true;
            if (std.ascii.eqlIgnoreCase(attr_val, "strict")) {
                res.same_site = SameSite.Strict;
            } else if (std.ascii.eqlIgnoreCase(attr_val, "lax")) {
                res.same_site = SameSite.Lax;
            } else if (std.ascii.eqlIgnoreCase(attr_val, "none")) {
                res.same_site = SameSite.None;
            }
        } else if (std.ascii.eqlIgnoreCase(attr_name, "max-age")) {
            const parsed_age = std.fmt.parseInt(i64, attr_val, 10) catch 0;
            res.max_age = parsed_age;
            res.has_max_age = true;
        } else if (std.ascii.eqlIgnoreCase(attr_name, "expires")) {
            res.expires = attr_val;
        }
    }

    return res;
}

pub struct CookieJar {
    allocator: std.mem.Allocator = std.heap.page_allocator,
    entries: std.ArrayListUnmanaged(CookieEntry) = std.ArrayListUnmanaged(CookieEntry).empty,
    clock_skew_ms: i64 = 0,
    seq: i64 = 0,

    static fn init(allocator: std.mem.Allocator): CookieJar {
        return CookieJar {
            allocator: allocator,
            entries: std.ArrayListUnmanaged(CookieEntry).empty,
            clock_skew_ms: 0,
            seq: 0,
        };
    }

    fn deinit(*_) {
        _.entries.deinit(_.allocator);
    }

    fn now(): i64 {
        const t: i64 = @intCast(time(null));
        return t * 1000 + _.clock_skew_ms;
    }

    fn purgeExpired(*_) {
        const cur_now = _.now();
        let i: usize = 0;
        while (i < _.entries.items.len) {
            const ck = &_.entries.items[i];
            if (ck.expires_at != null && ck.expires_at.? <= cur_now) {
                * = _.entries.swapRemove(i);
            } else {
                i += 1;
            }
        }
    }

    fn getAll(*_): []CookieEntry {
        _.purgeExpired();
        return _.entries.items;
    }

    fn store(
        *_,
        url_str: []const u8,
        raw_header: []const u8,
        from_script: bool,
        ctx: (*const RequestContext)?,
    ): CookieEntry? {
        const parts = parseUrlParts(url_str) orelse return null;
        const parsed = parseSetCookie(raw_header) orelse return null;

        let host = parts.host;
        let host_only = true;
        if (parsed.has_domain && parsed.domain.len > 0) {
            const resolved = resolveDomain(parsed.domain, parts.host) orelse return null;
            host = resolved;
            host_only = isPublicSuffix(resolved);
        }

        // Prefix invariants
        if (std.mem.startsWith(u8, parsed.name, "__Secure-")) {
            if (!parsed.secure || !parts.trustworthy) return null;
        }
        if (std.mem.startsWith(u8, parsed.name, "__Host-")) {
            if (!parsed.secure || !parts.trustworthy) return null;
            if (parsed.has_domain && parsed.domain.len > 0) return null;
            if (!std.mem.eql(u8, parsed.path, "/")) return null;
        }

        if (parsed.secure && !parts.trustworthy) return null;

        // SameSite=None requires Secure
        if (parsed.has_same_site && parsed.same_site == SameSite.None && !parsed.secure) {
            return null;
        }

        // Cross-site top-level write check
        if (ctx != null && !ctx.?.top_level && parsed.same_site != SameSite.None) {
            if (ctx.?.initiator_origin.len > 0) {
                let s1_buf: [256]u8 = [0; 256];
                let s2_buf: [256]u8 = [0; 256];
                const s1 = schemefulSite(ctx.?.initiator_origin, s1_buf[0..256]);
                const s2 = schemefulSite(url_str, s2_buf[0..256]);
                if (s1 == null || s2 == null || !std.mem.eql(u8, s1.?, s2.?)) return null;
            }
        }

        let is_http_only = parsed.http_only;
        if (from_script) is_http_only = false;

        // Check if script write shadows an HttpOnly cookie
        if (from_script) {
            for (ck of _.entries.items) {
                if (std.mem.eql(u8, ck.name, parsed.name) && ck.http_only) {
                    if (host_only) {
                        const reaches = if (ck.host_only) std.mem.eql(u8, ck.host, host) else domainMatches(ck.host, host);
                        if (reaches) return null;
                    } else {
                        const reaches = if (ck.host_only) domainMatches(host, ck.host) else (domainMatches(host, ck.host) || domainMatches(ck.host, host));
                        if (reaches) return null;
                    }
                }
            }
        }

        // Non-trustworthy origin cannot overwrite secure cookie
        if (!parts.trustworthy) {
            for (ck of _.entries.items) {
                if (ck.secure && std.mem.eql(u8, ck.name, parsed.name)) {
                    if (domainMatches(ck.host, host) || domainMatches(host, ck.host)) {
                        if (pathMatches(ck.path, parsed.path)) return null;
                    }
                }
            }
        }

        // Calculate expires_at
        let expires_at: i64? = null;
        if (parsed.has_max_age) {
            if (parsed.max_age.? <= 0) {
                // Immediate removal of existing cookie
                * = _.remove(parsed.name, "", host, parsed.path);
                return CookieEntry {
                    name: parsed.name,
                    value: parsed.value,
                    host: host,
                    path: parsed.path,
                    secure: parsed.secure,
                    http_only: is_http_only,
                    same_site: parsed.same_site,
                    host_only: host_only,
                    expires_at: _.now() - 1000,
                    created_at: _.now(),
                };
            } else {
                expires_at = _.now() + parsed.max_age.? * MILLISECONDS_PER_SECOND;
            }
        }

        _.seq += 1;
        const entry = CookieEntry {
            name: parsed.name,
            value: parsed.value,
            host: host,
            path: parsed.path,
            secure: parsed.secure,
            http_only: is_http_only,
            same_site: parsed.same_site,
            host_only: host_only,
            expires_at: expires_at,
            created_at: _.seq,
        };

        // Check entry size
        if (entry.name.len + entry.value.len + entry.path.len > MAX_COOKIE_BYTES) {
            return null;
        }

        // Update or insert
        let found = false;
        let i: usize = 0;
        while (i < _.entries.items.len) : (i += 1) {
            const ex = &_.entries.items[i];
            if (std.mem.eql(u8, ex.name, entry.name) &&
                std.mem.eql(u8, ex.path, entry.path) &&
                std.mem.eql(u8, ex.host, entry.host) &&
                ex.host_only == entry.host_only)
            {
                if (from_script && ex.http_only) return null;
                const old_created = ex.created_at;
                _.entries.items[i] = entry;
                _.entries.items[i].created_at = old_created;
                found = true;
                break;
            }
        }

        if (!found) {
            if (_.entries.items.len >= MAX_COOKIES_TOTAL) {
                _.purgeExpired();
            }
            if (_.entries.items.len >= MAX_COOKIES_TOTAL) {
                return null;
            }
            _.entries.append(_.allocator, entry) catch return null;
        }

        return entry;
    }

    fn setFromHeader(*_, url_str: []const u8, set_cookie_header: []const u8, ctx: (*const RequestContext)?): CookieEntry? {
        return _.store(url_str, set_cookie_header, false, ctx);
    }

    fn setFromDocumentCookie(*_, url_str: []const u8, raw: []const u8): CookieEntry? {
        return _.store(url_str, raw, true, null);
    }

    fn cookieHeaderForUrl(*_, url_str: []const u8, ctx: (*const RequestContext)?, allocator: std.mem.Allocator): ([]const u8)? {
        const parts = parseUrlParts(url_str) orelse return null;
        _.purgeExpired();

        const cur_now = _.now();
        let matched: [64]CookieEntry = [CookieEntry{} ; 64];
        let match_count: usize = 0;

        for (ck of _.entries.items) {
            if (ck.expires_at != null && ck.expires_at.? <= cur_now) continue;
            const host_ok = if (ck.host_only) std.mem.eql(u8, ck.host, parts.host) else domainMatches(ck.host, parts.host);
            if (!host_ok) continue;
            if (ck.secure && !parts.trustworthy) continue;
            if (!pathMatches(ck.path, parts.path)) continue;

            if (match_count < matched.len) {
                matched[match_count] = ck;
                match_count += 1;
            }
        }

        if (match_count == 0) return null;

        // Sort: longer path first, then older created_at
        let s: usize = 0;
        while (s < match_count) : (s += 1) {
            let min_idx = s;
            let k = s + 1;
            while (k < match_count) : (k += 1) {
                if (matched[k].path.len > matched[min_idx].path.len) {
                    min_idx = k;
                } else if (matched[k].path.len == matched[min_idx].path.len && matched[k].created_at < matched[min_idx].created_at) {
                    min_idx = k;
                }
            }
            if (min_idx != s) {
                const tmp = matched[s];
                matched[s] = matched[min_idx];
                matched[min_idx] = tmp;
            }
        }

        // Format name=value; name2=value2
        let out = std.ArrayListUnmanaged(u8).empty;
        let idx: usize = 0;
        while (idx < match_count) : (idx += 1) {
            if (idx > 0) out.appendSlice(allocator, "; ") catch return null;
            out.appendSlice(allocator, matched[idx].name) catch return null;
            out.append(allocator, '=') catch return null;
            out.appendSlice(allocator, matched[idx].value) catch return null;
        }
        return out.toOwnedSlice(allocator) catch null;
    }

    fn documentCookieString(*_, url_str: []const u8, allocator: std.mem.Allocator): []const u8 {
        const parts = parseUrlParts(url_str) orelse return "";
        _.purgeExpired();

        const cur_now = _.now();
        let matched: [64]CookieEntry = [CookieEntry{} ; 64];
        let match_count: usize = 0;

        for (ck of _.entries.items) {
            if (ck.http_only) continue; // HttpOnly omitted from document.cookie
            if (ck.expires_at != null && ck.expires_at.? <= cur_now) continue;
            const host_ok = if (ck.host_only) std.mem.eql(u8, ck.host, parts.host) else domainMatches(ck.host, parts.host);
            if (!host_ok) continue;
            if (ck.secure && !parts.trustworthy) continue;
            if (!pathMatches(ck.path, parts.path)) continue;

            if (match_count < matched.len) {
                matched[match_count] = ck;
                match_count += 1;
            }
        }

        if (match_count == 0) return "";

        let out = std.ArrayListUnmanaged(u8).empty;
        let idx: usize = 0;
        while (idx < match_count) : (idx += 1) {
            if (idx > 0) out.appendSlice(allocator, "; ") catch return "";
            out.appendSlice(allocator, matched[idx].name) catch return "";
            out.append(allocator, '=') catch return "";
            out.appendSlice(allocator, matched[idx].value) catch return "";
        }
        return out.toOwnedSlice(allocator) catch "";
    }

    fn remove(*_, name: []const u8, url: []const u8, domain: []const u8, path: []const u8): usize {
        let filter_host: []const u8 = domain;
        if (filter_host.len > 0 && filter_host[0] == '.') {
            filter_host = filter_host[1..];
        } else if (url.len > 0) {
            if (parts := parseUrlParts(url)) {
                filter_host = parts.host;
            }
        }

        let removed: usize = 0;
        let i: usize = 0;
        while (i < _.entries.items.len) {
            const ck = &_.entries.items[i];
            let is_match = std.mem.eql(u8, ck.name, name);
            if (is_match && filter_host.len > 0) {
                is_match = std.mem.eql(u8, ck.host, filter_host);
            }
            if (is_match && path.len > 0) {
                is_match = std.mem.eql(u8, ck.path, path);
            }
            if (is_match) {
                * = _.entries.swapRemove(i);
                removed += 1;
            } else {
                i += 1;
            }
        }
        return removed;
    }

    fn clear(*_) {
        _.entries.clearRetainingCapacity();
    }
}

pub fn entryToCdp(cookie: *const CookieEntry): CdpCookie {
    let ss_str: []const u8 = "";
    if (cookie.same_site != null) {
        ss_str = switch (cookie.same_site.?) {
            SameSite.Strict => "Strict",
            SameSite.Lax => "Lax",
            SameSite.None => "None",
        };
    }

    let exp_sec: f64 = -1.0;
    if (cookie.expires_at != null) {
        const ms: f64 = @floatFromInt(cookie.expires_at.?);
        exp_sec = ms / 1000.0;
    }

    return CdpCookie {
        name: cookie.name,
        value: cookie.value,
        domain: cookie.host,
        path: cookie.path,
        expires: exp_sec,
        size: cookie.name.len + cookie.value.len,
        http_only: cookie.http_only,
        secure: cookie.secure,
        session: cookie.expires_at == null,
        same_site: ss_str,
    };
}
