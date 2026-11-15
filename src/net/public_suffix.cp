// src/net/public_suffix.cp
// CapSurf RFC 6265 & WHATWG Public Suffix List (PSL) Engine
// Zero-allocation suffix evaluation for cookie domain and same-site checks.

@c {
    fn fopen(filename: [*c]const u8, mode: [*c]const u8): *void?;
    fn fread(ptr: [*c]u8, size: usize, count: usize, stream: *void?): usize;
    fn fclose(stream: *void?): i32;
    fn fseek(stream: *void?, offset: c_long, whence: c_int): i32;
    fn ftell(stream: *void?): c_long;
}

pub const DEFAULT_PSL_FALLBACK: []const u8 =
    "com\norg\nnet\nedu\ngov\nmil\nio\nco\nuk\nco.uk\ngov.uk\nac.uk\norg.uk\nde\nfr\njp\nco.jp\ncn\ncom.cn\nau\ncom.au\nca\neu\nru\nch\nit\nnl\nse\nno\nes\nbr\nin\nco.in\napp\ndev\nai\nme\ntv\ncc\nxyz\ntop\nsite\nonline\nstore\ntech\ngithub.io\ngitlab.io\n";

fn loadPslData(allocator: std.mem.Allocator): []const u8 {
    const p1: []const u8 = "src/net/public-suffix-list.dat\x00";
    const p2: []const u8 = "../src/net/public-suffix-list.dat\x00";
    const p3: []const u8 = "../../src/net/public-suffix-list.dat\x00";
    const p4: []const u8 = "public-suffix-list.dat\x00";

    const paths: [4][]const u8 = [ p1, p2, p3, p4 ];
    for (p of paths) {
        if (f := fopen(p.ptr, "rb")) {
            defer * = fclose(f);
            * = fseek(f, 0, 2); // SEEK_END = 2
            const len = ftell(f);
            * = fseek(f, 0, 0); // SEEK_SET = 0
            if (len > 0) {
                const ulen: usize = @intCast(len);
                let buf = allocator.alloc(u8, ulen) catch return DEFAULT_PSL_FALLBACK;
                const n = fread(buf.ptr, 1, ulen, f);
                return buf[0..n];
            }
        }
    }
    return DEFAULT_PSL_FALLBACK;
}

fn countLabels(s: []const u8): usize {
    if (s.len == 0) return 0;
    let count: usize = 1;
    for (ch of s) {
        if (ch == '.') count += 1;
    }
    return count;
}

pub struct PublicSuffixRules {
    exact: std.StringHashMapUnmanaged(bool) = .{},
    wildcard: std.StringHashMapUnmanaged(bool) = .{},
    exception: std.StringHashMapUnmanaged(bool) = .{},
    max_labels: usize = 0,
    initialized: bool = false,

    fn init(*_, allocator: std.mem.Allocator)! {
        if (_.initialized) return;
        const raw_data = loadPslData(allocator);
        let it = std.mem.splitScalar(u8, raw_data, '\n');
        while (line := it.next()) {
            const trimmed = std.mem.trim(u8, line, " \r\t");
            if (trimmed.len == 0 || (trimmed.len >= 2 && trimmed[0] == '/' && trimmed[1] == '/')) continue;
            if (trimmed[0] == '!') {
                const body = trimmed[1..];
                _.exception.put(allocator, body, true) catch return;
                const cnt = countLabels(body);
                if (cnt > _.max_labels) _.max_labels = cnt;
            } else if (trimmed.len >= 2 && trimmed[0] == '*' && trimmed[1] == '.') {
                const body = trimmed[2..];
                _.wildcard.put(allocator, body, true) catch return;
                const cnt = countLabels(body);
                if (cnt > _.max_labels) _.max_labels = cnt;
            } else {
                _.exact.put(allocator, trimmed, true) catch return;
                const cnt = countLabels(trimmed);
                if (cnt > _.max_labels) _.max_labels = cnt;
            }
        }
        _.initialized = true;
    }
}

pub threadlocal let g_rules: PublicSuffixRules = PublicSuffixRules{};

pub fn ensureRulesLoaded() {
    if (!g_rules.initialized) {
        g_rules.init(std.heap.page_allocator) catch unreachable;
    }
}

pub fn isIpHost(host: []const u8): bool {
    if (host.len == 0) return false;
    let has_dots: bool = false;
    let only_digits_dots: bool = true;
    for (ch of host) {
        if (ch == ':') return true;
        if (ch == '.') {
            has_dots = true;
        } else if (ch < '0' || ch > '9') {
            only_digits_dots = false;
        }
    }
    return only_digits_dots && has_dots;
}

pub fn splitLabels(host: []const u8, out_labels: [][ ]const u8): usize {
    if (host.len == 0) return 0;
    let count: usize = 0;
    let it = std.mem.splitScalar(u8, host, '.');
    while (label := it.next()) {
        if (label.len > 0) {
            if (count < out_labels.len) {
                out_labels[count] = label;
                count += 1;
            }
        }
    }
    return count;
}

pub fn publicSuffixLength(labels: []const []const u8): usize {
    ensureRulesLoaded();
    if (labels.len == 0) return 0;
    let best: usize = 1; // PSL default rule is *.
    let exception_length: usize = 0;
    const limit = if (labels.len < g_rules.max_labels + 1) labels.len else (g_rules.max_labels + 1);

    let count: usize = 1;
    while (count <= limit) : (count += 1) {
        // Construct suffix string from last `count` labels
        const first_label = labels[labels.len - count];
        const last_label = labels[labels.len - 1];
        const start_ptr = first_label.ptr;
        const end_ptr = last_label.ptr + last_label.len;
        const suffix_len = @intFromPtr(end_ptr) - @intFromPtr(start_ptr);
        const suffix = start_ptr[0..suffix_len];

        if (g_rules.exception.contains(suffix)) {
            if (count > exception_length) exception_length = count;
        }
        if (g_rules.exact.contains(suffix)) {
            if (count > best) best = count;
        }
        if (count > 1) {
            // Wildcard match on (count - 1) suffix
            const w_first = labels[labels.len - (count - 1)];
            const w_start = w_first.ptr;
            const w_len = @intFromPtr(end_ptr) - @intFromPtr(w_start);
            const w_suffix = w_start[0..w_len];
            if (g_rules.wildcard.contains(w_suffix)) {
                if (count > best) best = count;
            }
        }
    }
    return if (exception_length > 0) (exception_length - 1) else best;
}

pub fn siteKey(host: []const u8): []const u8 {
    let h = host;
    if (h.len > 0 && h[h.len - 1] == '.') {
        h = h[0..h.len - 1];
    }
    if (h.len == 0 || isIpHost(h)) return h;

    let label_buf: [128][]const u8 = ["" ; 128];
    const count = splitLabels(h, label_buf[0..128]);
    if (count <= 1) return h;

    const suffix = publicSuffixLength(label_buf[0..count]);
    const site_labels = if (count < suffix + 1) count else (suffix + 1);
    const first_label = label_buf[count - site_labels];
    const last_label = label_buf[count - 1];
    const start_ptr = first_label.ptr;
    const end_ptr = last_label.ptr + last_label.len;
    const site_len = @intFromPtr(end_ptr) - @intFromPtr(start_ptr);
    return start_ptr[0..site_len];
}

pub fn isPublicSuffix(domain: []const u8): bool {
    let h = domain;
    if (h.len > 0 && h[0] == '.') h = h[1..];
    if (h.len > 0 && h[h.len - 1] == '.') h = h[0..h.len - 1];
    if (h.len == 0) return true;
    if (isIpHost(h)) return false;

    let label_buf: [128][]const u8 = ["" ; 128];
    const count = splitLabels(h, label_buf[0..128]);
    if (count == 0) return true;

    const suffix = publicSuffixLength(label_buf[0..count]);
    return suffix == count;
}

pub fn schemefulSite(url_str: []const u8, allocator: std.mem.Allocator): ([]const u8)? {
    let scheme: []const u8 = "";
    let rest: []const u8 = "";
    if (std.mem.startsWith(u8, url_str, "https://")) {
        scheme = "https:";
        rest = url_str[8..];
    } else if (std.mem.startsWith(u8, url_str, "http://")) {
        scheme = "http:";
        rest = url_str[7..];
    } else if (std.mem.startsWith(u8, url_str, "wss://")) {
        scheme = "https:";
        rest = url_str[6..];
    } else if (std.mem.startsWith(u8, url_str, "ws://")) {
        scheme = "http:";
        rest = url_str[5..];
    } else {
        return null;
    }

    // Extract hostname up to '/', ':', '?', '#'
    let host_end: usize = 0;
    while (host_end < rest.len) : (host_end += 1) {
        const ch = rest[host_end];
        if (ch == '/' || ch == ':' || ch == '?' || ch == '#') break;
    }
    const host = rest[0..host_end];
    if (host.len == 0) return null;

    const key = siteKey(host);
    return std.fmt.allocPrint(allocator, "{s}//{s}", .{ scheme, key }) catch null;
}
