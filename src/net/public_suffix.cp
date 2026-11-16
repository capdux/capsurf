// Migrated from vendor/kitesurf/packages/engine/src/public-suffix.ts
// CapSurf RFC 6265 & WHATWG Public Suffix List (PSL) Zero-Allocation Engine
// Zero-heap suffix evaluation for cookie domain and same-site checks.

pub const MULTI_SUFFIXES: [390][]const u8 = [
    "ab.ca", "ac.at", "ac.cn", "ac.il", "ac.in", "ac.jp",
    "ac.kr", "ac.nz", "ac.uk", "ac.vn", "ac.za", "act.edu.au",
    "agro.pl", "ah.cn", "aid.pl", "amplifyapp.com", "app.br", "appspot.com",
    "art.br", "asn.au", "asso.fr", "atm.pl", "auto.pl", "av.tr",
    "azurestaticapps.net", "azurewebsites.net", "b-cdn.net", "bc.ca", "bel.tr", "biz.id",
    "biz.pl", "biz.tr", "biz.vn", "bj.cn", "blog.br", "blogspot.com",
    "brand.se", "busan.kr", "chungbuk.kr", "chungnam.kr", "cloudflare-ipfs.com", "cloudfront.net",
    "club.tw", "co.at", "co.cl", "co.de", "co.id", "co.il",
    "co.in", "co.jp", "co.kr", "co.nl", "co.nz", "co.uk",
    "co.za", "com.ar", "com.au", "com.br", "com.cn", "com.co",
    "com.de", "com.es", "com.fr", "com.hk", "com.mx", "com.my",
    "com.ph", "com.pl", "com.ru", "com.sg", "com.tr", "com.tw",
    "com.vn", "cq.cn", "daegu.kr", "daejeon.kr", "dev.br", "dr.tr",
    "ebiz.tw", "eco.br", "ed.jp", "edu.ar", "edu.au", "edu.br",
    "edu.cn", "edu.co", "edu.es", "edu.hk", "edu.it", "edu.mx",
    "edu.my", "edu.ph", "edu.pl", "edu.ru", "edu.sg", "edu.tr",
    "edu.tw", "edu.vn", "elasticbeanstalk.com", "emp.br", "es.kr", "esp.br",
    "etc.br", "europa.eu", "far.br", "fastly.net", "firebaseapp.com", "firm.in",
    "fj.cn", "fly.dev", "fly.io", "fot.br", "game.tw", "gangwon.kr",
    "gc.ca", "gd.cn", "geek.nz", "gen.in", "gen.tr", "geo.jp",
    "ghost.io", "github.io", "gitlab.io", "gmina.pl", "go.id", "go.jp",
    "go.kr", "gob.ar", "gob.es", "gob.mx", "gov.ar", "gov.au",
    "gov.br", "gov.cn", "gov.co", "gov.hk", "gov.ie", "gov.il",
    "gov.in", "gov.it", "gov.my", "gov.ph", "gov.pl", "gov.ru",
    "gov.sg", "gov.tr", "gov.tw", "gov.uk", "gov.vn", "gov.za",
    "govt.nz", "gs.cn", "gsm.pl", "gv.at", "gwangju.kr", "gx.cn",
    "gyeongbuk.kr", "gyeonggi.kr", "gyeongnam.kr", "gz.cn", "ha.cn", "hb.cn",
    "he.cn", "health.vn", "herokuapp.com", "hi.cn", "hl.cn", "hn.cn",
    "hs.kr", "id.au", "idf.il", "idv.hk", "idv.tw", "imb.br",
    "incheon.kr", "ind.br", "ind.in", "inf.br", "info.pl", "info.tr",
    "info.vn", "int.ar", "int.vn", "iwi.nz", "jeju.kr", "jeonbuk.kr",
    "jeonnam.kr", "jl.cn", "js.cn", "jus.br", "jx.cn", "k12.il",
    "k12.tr", "kg.kr", "klub.pl", "law.za", "leg.br", "lg.jp",
    "ln.cn", "loca.lt", "log.br", "ltd.uk", "mail.pl", "mb.ca",
    "me.uk", "med.br", "media.pl", "miasta.pl", "mil.ar", "mil.cn",
    "mil.co", "mil.in", "mil.kr", "mil.my", "mil.nz", "mil.ph",
    "mil.pl", "mp.br", "ms.kr", "msk.ru", "muni.il", "mus.br",
    "musica.ar", "my.id", "myshopify.com", "name.my", "name.tr", "name.vn",
    "nb.ca", "ne.jp", "ne.kr", "net.ar", "net.au", "net.br",
    "net.cn", "net.co", "net.hk", "net.id", "net.il", "net.in",
    "net.my", "net.nz", "net.ph", "net.pl", "net.ru", "net.sg",
    "net.tr", "net.tw", "net.uk", "net.vn", "net.za", "netlify.app",
    "nf.ca", "ngo.ph", "ngrok-free.app", "ngrok.io", "nieruchomosci.pl", "nl.ca",
    "nm.cn", "nom.br", "nom.co", "nom.es", "nom.fr", "nom.pl",
    "not.br", "nsw.edu.au", "nt.ca", "nt.edu.au", "ntr.br", "nu.ca",
    "nx.cn", "odo.br", "on.ca", "onrender.com", "or.at", "or.id",
    "or.jp", "or.kr", "org.ar", "org.au", "org.br", "org.cn",
    "org.co", "org.es", "org.hk", "org.il", "org.in", "org.my",
    "org.nz", "org.ph", "org.pl", "org.ru", "org.se", "org.sg",
    "org.tr", "org.tw", "org.uk", "org.vn", "org.za", "pages.dev",
    "parti.se", "pc.pl", "pe.ca", "pe.kr", "per.sg", "plc.uk",
    "pol.tr", "police.uk", "powiat.pl", "pp.ru", "pp.se", "ppg.br",
    "prd.fr", "press.se", "presse.fr", "priv.at", "priv.pl", "pro.br",
    "pro.vn", "psc.br", "psi.br", "qc.ca", "qh.cn", "qld.edu.au",
    "radio.br", "railway.app", "re.kr", "realestate.pl", "rec.br", "rel.pl",
    "render.com", "res.in", "s3.amazonaws.com", "sa.edu.au", "sc.cn", "sc.kr",
    "sch.id", "sch.uk", "school.nz", "school.za", "sd.cn", "seoul.kr",
    "sh.cn", "shop.pl", "sk.ca", "sklep.pl", "sn.cn", "sos.pl",
    "spb.ru", "srv.br", "supabase.co", "surge.sh", "sx.cn", "szkola.pl",
    "targi.pl", "tas.edu.au", "taxi.br", "teo.br", "tj.cn", "tm.fr",
    "tm.pl", "tm.se", "tmp.br", "tourism.pl", "travel.pl", "trd.br",
    "tur.ar", "tur.br", "turystyka.pl", "tv.br", "tv.tr", "ulsan.kr",
    "vercel.app", "vet.br", "vic.edu.au", "vlog.br", "wa.edu.au", "web.app",
    "web.id", "web.tr", "web.za", "wiki.br", "wordpress.com", "workers.dev",
    "xj.cn", "xz.cn", "yk.ca", "yn.cn", "zj.cn", "zlg.br",
];


fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn compareCaseInsensitive(a: []const u8, b: []const u8): i32 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        const ca = toLowerAscii(a[i]);
        const cb = toLowerAscii(b[i]);
        if (ca < cb) return -1;
        if (ca > cb) return 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

pub fn isMultiSuffix(suffix: []const u8): bool {
    let low: usize = 0;
    let high: usize = MULTI_SUFFIXES.len;
    while (low < high) {
        const mid = low + (high - low) / 2;
        const cmp = compareCaseInsensitive(suffix, MULTI_SUFFIXES[mid]);
        if (cmp == 0) return true;
        if (cmp < 0) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    return false;
}

pub fn isIpHost(host: []const u8): bool {
    if (host.len == 0) return false;
    let has_dots: bool = false;
    let only_digits_dots: bool = true;
    let i: usize = 0;
    while (i < host.len) : (i += 1) {
        const ch = host[i];
        if (ch == ':') return true;
        if (ch == '.') {
            has_dots = true;
        } else if (ch < '0' || ch > '9') {
            only_digits_dots = false;
        }
    }
    return only_digits_dots && has_dots;
}

pub fn splitLabels(host: []const u8, out_labels: [][]const u8): usize {
    if (host.len == 0) return 0;
    let count: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= host.len) : (i += 1) {
        if (i == host.len || host[i] == '.') {
            if (i > start) {
                if (count < out_labels.len) {
                    out_labels[count] = host[start..i];
                    count += 1;
                }
            }
            start = i + 1;
        }
    }
    return count;
}

pub fn publicSuffixLength(labels: []const []const u8): usize {
    if (labels.len == 0) return 0;
    let best: usize = 1; // Default PSL rule is *.

    const limit = if (labels.len < 4) labels.len else 4;
    let count: usize = 2;
    while (count <= limit) : (count += 1) {
        let buf: [128]u8 = [0; 128];
        let buf_len: usize = 0;
        let k: usize = labels.len - count;
        let first: bool = true;
        while (k < labels.len) : (k += 1) {
            const lab = labels[k];
            if (!first) {
                if (buf_len < buf.len) {
                    buf[buf_len] = '.';
                    buf_len += 1;
                }
            }
            first = false;
            let l: usize = 0;
            while (l < lab.len) : (l += 1) {
                if (buf_len < buf.len) {
                    buf[buf_len] = lab[l];
                    buf_len += 1;
                }
            }
        }
        if (isMultiSuffix(buf[0..buf_len])) {
            if (count > best) {
                best = count;
            }
        }
    }
    return best;
}

pub fn isPublicSuffix(domain: []const u8): bool {
    let h = domain;
    if (h.len > 0 && h[0] == '.') h = h[1..];
    if (h.len > 0 && h[h.len - 1] == '.') h = h[0..h.len - 1];
    if (h.len == 0) return true;
    if (isIpHost(h)) return false;

    let label_buf: [64][]const u8 = ["" ; 64];
    const count = splitLabels(h, label_buf[0..64]);
    if (count == 0) return true;
    if (count == 1) return true;

    const suffix = publicSuffixLength(label_buf[0..count]);
    return suffix == count;
}

pub fn siteKey(host: []const u8): []const u8 {
    let h = host;
    if (h.len > 0 && h[h.len - 1] == '.') {
        h = h[0..h.len - 1];
    }
    if (h.len == 0 || isIpHost(h)) return h;

    let label_starts: [64]usize = [0; 64];
    let label_lens: [64]usize = [0; 64];
    let count: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= h.len) : (i += 1) {
        if (i == h.len || h[i] == '.') {
            if (i > start) {
                if (count < 64) {
                    label_starts[count] = start;
                    label_lens[count] = i - start;
                    count += 1;
                }
            }
            start = i + 1;
        }
    }
    if (count <= 1) return h;

    let label_slices: [64][]const u8 = ["" ; 64];
    let idx: usize = 0;
    while (idx < count) : (idx += 1) {
        label_slices[idx] = h[label_starts[idx] .. label_starts[idx] + label_lens[idx]];
    }

    const suffix = publicSuffixLength(label_slices[0..count]);
    const site_labels = if (count < suffix + 1) count else (suffix + 1);
    const start_idx = label_starts[count - site_labels];
    return h[start_idx..];
}

pub fn schemefulSite(url_str: []const u8, out_buf: []u8): ([]const u8)? {
    let scheme_prefix: []const u8 = "";
    let rest: []const u8 = "";

    if (std.mem.startsWith(u8, url_str, "https://")) {
        scheme_prefix = "https://";
        rest = url_str[8..];
    } else if (std.mem.startsWith(u8, url_str, "http://")) {
        scheme_prefix = "http://";
        rest = url_str[7..];
    } else if (std.mem.startsWith(u8, url_str, "wss://")) {
        scheme_prefix = "https://";
        rest = url_str[6..];
    } else if (std.mem.startsWith(u8, url_str, "ws://")) {
        scheme_prefix = "http://";
        rest = url_str[5..];
    } else {
        return null;
    }

    if (rest.len == 0) return null;

    // Extract authority component (up to '/', '?', '#', or end of string)
    let auth_end: usize = 0;
    while (auth_end < rest.len) : (auth_end += 1) {
        const c = rest[auth_end];
        if (c == '/' || c == '?' || c == '#') break;
    }
    const authority = rest[0..auth_end];
    if (authority.len == 0) return null;

    // Skip userinfo if present: find '@'
    let host_part = authority;
    let at_idx: usize = 0;
    let has_at: bool = false;
    let i: usize = 0;
    while (i < authority.len) : (i += 1) {
        if (authority[i] == '@') {
            at_idx = i;
            has_at = true;
        }
    }
    if (has_at) {
        host_part = authority[at_idx + 1 ..];
    }
    if (host_part.len == 0) return null;

    // Extract hostname without port
    // If IPv6 (starts with '['), find matching ']'
    let hostname = host_part;
    if (host_part[0] == '[') {
        let bracket_end: usize = 0;
        let found_bracket: bool = false;
        let j: usize = 1;
        while (j < host_part.len) : (j += 1) {
            if (host_part[j] == ']') {
                bracket_end = j;
                found_bracket = true;
                break;
            }
        }
        if (found_bracket) {
            hostname = host_part[0 .. bracket_end + 1];
        }
    } else {
        // Find ':' for port
        let colon_idx: usize = 0;
        let has_colon: bool = false;
        let j: usize = 0;
        while (j < host_part.len) : (j += 1) {
            if (host_part[j] == ':') {
                colon_idx = j;
                has_colon = true;
                break;
            }
        }
        if (has_colon) {
            hostname = host_part[0..colon_idx];
        }
    }

    if (hostname.len == 0) return null;

    const site = siteKey(hostname);
    if (site.len == 0) return null;

    const total_len = scheme_prefix.len + site.len;
    if (total_len > out_buf.len) return null;

    std.mem.copyForwards(u8, out_buf[0..scheme_prefix.len], scheme_prefix);
    std.mem.copyForwards(u8, out_buf[scheme_prefix.len..total_len], site);
    return out_buf[0..total_len];
}

pub fn schemefulSiteAlloc(url_str: []const u8, allocator: std.mem.Allocator): ([]const u8)? {
    let buf: [256]u8 = [0; 256];
    const s = schemefulSite(url_str, buf[0..256]);
    if (s == null) return null;
    return allocator.dupe(u8, s.?) catch null;
}
