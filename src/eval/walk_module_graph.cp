// Migrated from vendor/kitesurf/packages/engine/src/eval/walk-module-graph.ts
// Resolves ESM module graphs, import maps, modulepreloads, and synthetic imports.

pub const MAX_IMPORT_MAP_ENTRIES: usize = 16384;
pub const MAX_IMPORT_MAP_SCOPES: usize = 256;
pub const MAX_IMPORT_MAP_SOURCE_BYTES: usize = 2 * 1024 * 1024;
pub const MAX_IMPORT_MAP_KEY_CODE_UNITS: usize = 2048;
pub const MAX_RECORDED_RESOLUTIONS: usize = 8192;
pub const MAX_IMPORT_MAP_WORK: u64 = 400_000_000;

pub const MAX_STATIC_IMPORT_RULES: usize = 128;
pub const MAX_STATIC_SCOPES: usize = 16;
pub const MAX_STATIC_SCOPE_RULES: usize = 32;

pub struct ImportMapBudget {
    entries: usize = 0,
    scopes: usize = 0,
    records: usize = 0,
    work: u64 = 0,
    degraded: bool = false,

    fn chargeWork(*_, units: u64) {
        _.work += units;
        if (_.work > MAX_IMPORT_MAP_WORK) {
            _.degraded = true;
        }
    }
}

pub struct ImportMapRule {
    key: []const u8 = "",
    address: []const u8 = "",
    is_prefix: bool = false,
    is_blocked: bool = false,

    fn matches(specifier: []const u8): bool {
        if (_.is_prefix) {
            return std.mem.startsWith(u8, specifier, _.key);
        }
        return std.mem.eql(u8, specifier, _.key);
    }
}

pub struct ImportMapScopeGroup {
    prefix: []const u8 = "",
    rules: [MAX_STATIC_SCOPE_RULES]ImportMapRule = [ImportMapRule{}; 32],
    rule_count: usize = 0,

    fn addRule(*_, key: []const u8, addr: []const u8, blocked: bool): bool {
        if (_.rule_count >= MAX_STATIC_SCOPE_RULES) return false;
        let is_pref = key.len > 0 && key[key.len - 1] == '/';
        _.rules[_.rule_count] = ImportMapRule{
            key: key,
            address: addr,
            is_prefix: is_pref,
            is_blocked: blocked,
        };
        _.rule_count += 1;
        return true;
    }
}

pub struct ParsedImportMap {
    imports: [MAX_STATIC_IMPORT_RULES]ImportMapRule = [ImportMapRule{}; 128],
    import_count: usize = 0,
    scopes: [MAX_STATIC_SCOPES]ImportMapScopeGroup = [ImportMapScopeGroup{}; 16],
    scope_count: usize = 0,

    fn addImport(*_, key: []const u8, addr: []const u8, blocked: bool): bool {
        if (_.import_count >= MAX_STATIC_IMPORT_RULES) return false;
        let is_pref = key.len > 0 && key[key.len - 1] == '/';
        _.imports[_.import_count] = ImportMapRule{
            key: key,
            address: addr,
            is_prefix: is_pref,
            is_blocked: blocked,
        };
        _.import_count += 1;
        return true;
    }

    fn addScope(*_, prefix: []const u8): (*ImportMapScopeGroup)? {
        let i: usize = 0;
        while (i < _.scope_count) : (i += 1) {
            if (std.mem.eql(u8, _.scopes[i].prefix, prefix)) {
                return &_.scopes[i];
            }
        }
        if (_.scope_count >= MAX_STATIC_SCOPES) return null;
        let idx = _.scope_count;
        _.scopes[idx].prefix = prefix;
        _.scopes[idx].rule_count = 0;
        _.scope_count += 1;
        return &_.scopes[idx];
    }

    fn resolve(*_, specifier: []const u8, importer: []const u8, budget: *ImportMapBudget, out_buf: []u8): ([]const u8)? {
        if (budget.degraded) return null;
        budget.chargeWork(10);

        // 1. Try scopes (longest matching prefix first)
        let best_scope_len: usize = 0;
        let best_scope_idx: usize? = null;

        let s: usize = 0;
        while (s < _.scope_count) : (s += 1) {
            let sc = &_.scopes[s];
            if (std.mem.startsWith(u8, importer, sc.prefix)) {
                if (sc.prefix.len >= best_scope_len) {
                    best_scope_len = sc.prefix.len;
                    best_scope_idx = s;
                }
            }
        }

        if (best_scope_idx != null) {
            let sc_ptr = &_.scopes[best_scope_idx.?];
            let r: usize = 0;
            while (r < sc_ptr.rule_count) : (r += 1) {
                let rule = &sc_ptr.rules[r];
                if (rule.matches(specifier)) {
                    if (rule.is_blocked) return null;
                    return resolveRuleAddress(rule, specifier, out_buf);
                }
            }
        }

        // 2. Try top-level imports
        let r: usize = 0;
        while (r < _.import_count) : (r += 1) {
            let rule = &_.imports[r];
            if (rule.matches(specifier)) {
                if (rule.is_blocked) return null;
                return resolveRuleAddress(rule, specifier, out_buf);
            }
        }

        // 3. Fallback: if URL-like, resolve against importer or return specifier
        if (isUrlLikeSpecifier(specifier)) {
            return specifier;
        }

        // Bare specifier not in import map fails
        return null;
    }
}

fn resolveRuleAddress(rule: *const ImportMapRule, specifier: []const u8, out_buf: []u8): ([]const u8)? {
    if (!rule.is_prefix) {
        return rule.address;
    }

    // Prefix rule replacement: rule.address + (specifier - rule.key)
    let suffix = specifier[rule.key.len..];
    let total_len = rule.address.len + suffix.len;
    if (total_len > out_buf.len) return null;

    @memcpy(out_buf[0..rule.address.len], rule.address);
    @memcpy(out_buf[rule.address.len .. total_len], suffix);
    return out_buf[0..total_len];
}

pub fn isUrlLikeSpecifier(specifier: []const u8): bool {
    if (std.mem.startsWith(u8, specifier, "/") ||
        std.mem.startsWith(u8, specifier, "./") ||
        std.mem.startsWith(u8, specifier, "../")) {
        return true;
    }
    // Check scheme: e.g. "https://", "http://", "file:"
    let colon_idx = std.mem.indexOfScalar(u8, specifier, ':');
    if (colon_idx != null && colon_idx.? > 0) {
        return true;
    }
    return false;
}

pub fn astroIslandModuleUrls(html: []const u8, out_urls: *[16][]const u8): usize {
    let count: usize = 0;
    let target = "component-url=\"";
    let pos: usize = 0;

    while (pos < html.len && count < 16) {
        let remaining = html[pos..];
        let found = std.mem.indexOf(u8, remaining, target);
        if (found == null) break;

        let start = pos + found.? + target.len;
        if (start >= html.len) break;

        let quote_end = std.mem.indexOfScalar(u8, html[start..], '"');
        if (quote_end == null) break;

        out_urls[count] = html[start .. start + quote_end.?];
        count += 1;
        pos = start + quote_end.? + 1;
    }

    return count;
}

pub fn extractModulePreloadSeeds(html: []const u8, out_urls: *[16][]const u8): usize {
    let count: usize = 0;
    let marker = "rel=\"modulepreload\"";
    let href_attr = "href=\"";
    let pos: usize = 0;

    while (pos < html.len && count < 16) {
        let remaining = html[pos..];
        let link_idx = std.mem.indexOf(u8, remaining, "<link");
        if (link_idx == null) break;

        let tag_start = pos + link_idx.?;
        let tag_end_idx = std.mem.indexOfScalar(u8, html[tag_start..], '>');
        if (tag_end_idx == null) break;

        let tag = html[tag_start .. tag_start + tag_end_idx.?];
        if (std.mem.indexOf(u8, tag, marker) != null) {
            let h_idx = std.mem.indexOf(u8, tag, href_attr);
            if (h_idx != null) {
                let val_start = h_idx.? + href_attr.len;
                let q_end = std.mem.indexOfScalar(u8, tag[val_start..], '"');
                if (q_end != null) {
                    out_urls[count] = tag[val_start .. val_start + q_end.?];
                    count += 1;
                }
            }
        }

        pos = tag_start + tag_end_idx.? + 1;
    }

    return count;
}

pub fn buildFallbackModuleResolverSource(out_buf: []u8): []const u8 {
    let src = "export function resolveImportMap(spec, importer) {\n" ++
              "  if (spec.startsWith('/') || spec.startsWith('./') || spec.startsWith('../')) return spec;\n" ++
              "  throw new TypeError('Bare specifier requires import map: ' + spec);\n" ++
              "}\n";
    if (src.len > out_buf.len) return "";
    @memcpy(out_buf[0..src.len], src);
    return out_buf[0..src.len];
}
