// Migrated from vendor/kitesurf/packages/engine/src/eval/next-route-chunks.ts
// Extracts likely route chunk URLs from Next.js manifests and page script text.

pub const MAX_ROUTE_CHUNKS: usize = 12;
pub const MAX_MANIFEST_ROUTES: usize = 64;
pub const MAX_CHUNKS_PER_ROUTE: usize = 16;
pub const MAX_TAIL_PARAMS: usize = 64;

const MARKER = "__BUILD_MANIFEST";
const CALLBACK = "self.__BUILD_MANIFEST_CB";
const FUNCTION = "function(";
const RETURN = "return";

pub struct ParentScriptDescriptor {
    src: []const u8 = "",
    resolved_src: []const u8 = "",
    prefetched_source: []const u8 = "",
}

pub struct RouteEntry {
    route: []const u8 = "",
    chunks: [MAX_CHUNKS_PER_ROUTE][]const u8 = [""; MAX_CHUNKS_PER_ROUTE],
    chunk_count: usize = 0,

    fn addChunk(*_, chunk: []const u8) {
        if (_.chunk_count < MAX_CHUNKS_PER_ROUTE && chunk.len > 0) {
            _.chunks[_.chunk_count] = chunk;
            _.chunk_count += 1;
        }
    }
}

pub struct TailParamValue {
    name: []const u8 = "",
    value: []const u8 = "",
}

pub struct BuildManifest {
    routes: [MAX_MANIFEST_ROUTES]RouteEntry = [RouteEntry{}; MAX_MANIFEST_ROUTES],
    route_count: usize = 0,

    fn getChunks(route: []const u8): []const []const u8 {
        for (i in 0.._.route_count) {
            if (std.mem.eql(u8, _.routes[i].route, route)) {
                return _.routes[i].chunks[0.._.routes[i].chunk_count];
            }
        }
        return [];
    }

    fn addRoute(*_, route: []const u8): (*RouteEntry)? {
        if (_.route_count >= MAX_MANIFEST_ROUTES) return null;
        for (i in 0.._.route_count) {
            if (std.mem.eql(u8, _.routes[i].route, route)) {
                return &_.routes[i];
            }
        }
        let idx = _.route_count;
        _.routes[idx] = RouteEntry{ route: route };
        _.route_count += 1;
        return &_.routes[idx];
    }
}

fn isSpace(ch: u8): bool {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

fn skipSpace(text: []const u8, from_idx: usize): usize {
    let cursor = from_idx;
    while (cursor < text.len && isSpace(text[cursor])) {
        cursor += 1;
    }
    return cursor;
}

fn isLineBreak(ch: u8): bool {
    return ch == '\n' || ch == '\r';
}

pub fn isManifestUrl(url: []const u8): bool {
    let clean_url = url;
    let i: usize = 0;
    while (i < url.len) : (i += 1) {
        if (url[i] == '?' || url[i] == '#') {
            clean_url = url[0..i];
            break;
        }
    }
    let static_idx: usize = 0;
    if (idx := std.mem.indexOf(u8, clean_url, "/_next/static/")) {
        static_idx = idx;
    } else {
        return false;
    }
    let suffix = "/_buildManifest.js";
    if (!std.mem.endsWith(u8, clean_url, suffix)) {
        return false;
    }
    let after_static = clean_url[static_idx + "/_next/static/".len ..];
    if (slash_idx := std.mem.indexOf(u8, after_static, "/")) {
        return slash_idx > 0;
    }
    return false;
}

pub fn isSkipRoute(route: []const u8): bool {
    return std.mem.eql(u8, route, "/_app") ||
           std.mem.eql(u8, route, "/_error") ||
           std.mem.eql(u8, route, "/404") ||
           std.mem.eql(u8, route, "/500");
}

fn hasObjectForm(source: []const u8): bool {
    let pos: usize = 0;
    while (pos < source.len) {
        if (idx := std.mem.indexOf(u8, source[pos..], MARKER)) {
            let at = pos + idx;
            pos = at + MARKER.len;
            let cursor = skipSpace(source, pos);
            if (cursor < source.len && source[cursor] == '=') {
                let next_cur = skipSpace(source, cursor + 1);
                if (next_cur < source.len && source[next_cur] == '{') {
                    return true;
                }
            }
        } else {
            break;
        }
    }
    return false;
}

fn extractFunctionParams(
    source: []const u8,
    out_params: *[MAX_TAIL_PARAMS][]const u8,
    out_count: *usize,
): bool {
    out_count.* = 0;
    let pos: usize = 0;
    while (pos < source.len) {
        if (idx := std.mem.indexOf(u8, source[pos..], MARKER)) {
            let at = pos + idx;
            pos = at + MARKER.len;
            let cursor = skipSpace(source, pos);
            if (cursor >= source.len || source[cursor] != '=') continue;
            cursor = skipSpace(source, cursor + 1);
            if (!std.mem.startsWith(u8, source[cursor..], FUNCTION)) continue;
            let params_start = cursor + FUNCTION.len;
            if (close_paren := std.mem.indexOf(u8, source[params_start..], ")")) {
                let paren = params_start + close_paren;
                let cur2 = skipSpace(source, paren + 1);
                if (cur2 < source.len && source[cur2] == '{') {
                    cur2 = skipSpace(source, cur2 + 1);
                    if (std.mem.startsWith(u8, source[cur2..], RETURN)) {
                        cur2 = skipSpace(source, cur2 + RETURN.len);
                        if (cur2 < source.len && source[cur2] == '{') {
                            // Extract comma-separated parameter names
                            let raw_params = source[params_start..paren];
                            let p_idx: usize = 0;
                            while (p_idx < raw_params.len) {
                                let start = skipSpace(raw_params, p_idx);
                                if (start >= raw_params.len) break;
                                let comma = std.mem.indexOf(u8, raw_params[start..], ",");
                                let end = raw_params.len;
                                if (c := comma) {
                                    end = start + c;
                                }
                                // Trim trailing space
                                let trim_end = end;
                                while (trim_end > start && isSpace(raw_params[trim_end - 1])) {
                                    trim_end -= 1;
                                }
                                if (trim_end > start && out_count.* < MAX_TAIL_PARAMS) {
                                    out_params[out_count.*] = raw_params[start..trim_end];
                                    out_count.* += 1;
                                }
                                p_idx = end + 1;
                            }
                            return true;
                        }
                    }
                }
            }
        } else {
            break;
        }
    }
    return false;
}

fn findTailRange(source: []const u8, out_open: *usize, out_end: *usize): bool {
    let open: usize = 0;
    let found_open = false;
    let pos: usize = 0;
    while (pos < source.len) {
        if (at := std.mem.indexOf(u8, source[pos..], "}")) {
            let idx = pos + at;
            let second = skipSpace(source, idx + 1);
            if (second < source.len && source[second] == ';') {
                second = skipSpace(source, second + 1);
            }
            if (second < source.len && source[second] == '}') {
                let paren = skipSpace(source, second + 1);
                if (paren < source.len && source[paren] == '(') {
                    open = paren + 1;
                    found_open = true;
                    break;
                }
            }
            pos = idx + 1;
        } else {
            break;
        }
    }
    if (!found_open) return false;

    let end: usize = 0;
    let found_end = false;
    pos = open;
    while (pos < source.len) {
        if (at := std.mem.indexOf(u8, source[pos..], ")")) {
            let idx = pos + at;
            let cursor = skipSpace(source, idx + 1);
            if (cursor < source.len && (source[cursor] == ';' || source[cursor] == ',')) {
                cursor = skipSpace(source, cursor + 1);
            }
            if (cursor == source.len || std.mem.startsWith(u8, source[cursor..], CALLBACK)) {
                end = idx;
                found_end = true;
            }
            pos = idx + 1;
        } else {
            break;
        }
    }
    if (found_end) {
        out_open.* = open;
        out_end.* = end;
        return true;
    }
    return false;
}

fn parseTailValues(
    source: []const u8,
    params: []const []const u8,
    out_values: *[MAX_TAIL_PARAMS]TailParamValue,
    out_count: *usize,
) {
    out_count.* = 0;
    let open: usize = 0;
    let end: usize = 0;
    if (!findTailRange(source, &open, &end)) return;

    let pos = open;
    let index: usize = 0;
    while (pos < end) {
        let ch = source[pos];
        if (ch == ',' || isSpace(ch)) {
            pos += 1;
            continue;
        }
        if (ch != '"') {
            let stop = pos + 1;
            while (stop < end && source[stop] != ',' && source[stop] != '"') {
                stop += 1;
            }
            index += 1;
            pos = stop;
            continue;
        }

        let cursor = pos + 1;
        let escaped_line_break = false;
        while (cursor < end) {
            let c = source[cursor];
            if (c == '"') break;
            if (c != '\\') {
                cursor += 1;
                continue;
            }
            if (cursor + 1 >= end || isLineBreak(source[cursor + 1])) {
                escaped_line_break = true;
                break;
            }
            cursor += 2;
        }

        if (escaped_line_break || cursor >= end) {
            pos += 1;
            continue;
        }

        if (index < params.len && out_count.* < MAX_TAIL_PARAMS) {
            let val = source[pos + 1 .. cursor];
            out_values[out_count.*] = TailParamValue{
                name: params[index],
                value: val,
            };
            out_count.* += 1;
        }
        index += 1;
        pos = cursor + 1;
    }
}

fn resolveTailValue(name: []const u8, values: []const TailParamValue): []const u8 {
    for (i in 0..values.len) {
        if (std.mem.eql(u8, values[i].name, name)) {
            return values[i].value;
        }
    }
    return "";
}

fn parseRouteChunks(
    source: []const u8,
    values: []const TailParamValue,
    manifest: *BuildManifest,
) {
    let pos: usize = 0;
    while (pos < source.len) {
        let key_opt = std.mem.indexOf(u8, source[pos..], "\"");
        if (key_opt == null) break;
        let key = pos + key_opt.?;
        if (key + 1 >= source.len || source[key + 1] != '/') {
            pos = key + 1;
            continue;
        }
        let end_opt = std.mem.indexOf(u8, source[key + 2 ..], "\"");
        if (end_opt == null) break;
        let key_end = key + 2 + end_opt.?;
        let cursor = skipSpace(source, key_end + 1);
        if (cursor >= source.len || source[cursor] != ':') {
            pos = key_end;
            continue;
        }
        cursor = skipSpace(source, cursor + 1);
        if (cursor >= source.len || source[cursor] != '[') {
            pos = key_end;
            continue;
        }

        let route = source[key + 1 .. key_end];
        let r_entry = manifest.addRoute(route);

        let item = cursor + 1;
        let closed = false;
        while (item < source.len) {
            let ch = source[item];
            if (ch == ']') {
                closed = true;
                break;
            }
            if (ch == '"') {
                if (stop_opt := std.mem.indexOf(u8, source[item + 1 ..], "\"")) {
                    let stop = item + 1 + stop_opt;
                    let chunk = source[item + 1 .. stop];
                    if (*entry := r_entry) {
                        if (std.mem.endsWith(u8, chunk, ".js")) {
                            entry.addChunk(chunk);
                        }
                    }
                    item = stop + 1;
                    continue;
                } else {
                    break;
                }
            }
            if (ch == ',' || isSpace(ch)) {
                item += 1;
                continue;
            }
            let stop = item + 1;
            while (stop < source.len) {
                let c = source[stop];
                if (c == ',' || c == ']' || c == '"' || isSpace(c)) break;
                stop += 1;
            }
            let id_name = source[item..stop];
            let resolved = resolveTailValue(id_name, values);
            if (*entry := r_entry) {
                if (std.mem.endsWith(u8, resolved, ".js")) {
                    entry.addChunk(resolved);
                }
            }
            item = stop;
        }
        if (!closed) break;
        pos = item + 1;
    }
}

pub fn parseBuildManifest(source: []const u8, manifest: *BuildManifest): bool {
    let params: [64][]const u8 = [""[0..0]; 64];
    let param_count: usize = 0;
    let has_params = extractFunctionParams(source, &params, &param_count);
    let has_obj = hasObjectForm(source);
    if (!has_params && !has_obj) return false;

    let tail_values: [64]TailParamValue = [TailParamValue{}; 64];
    let tail_count: usize = 0;
    if (has_params) {
        parseTailValues(source, params[0..param_count], &tail_values, &tail_count);
    }
    parseRouteChunks(source, tail_values[0..tail_count], manifest);
    return manifest.route_count > 0;
}

pub fn countRouteMentions(sources: []const []const u8, route: []const u8): usize {
    let count: usize = 0;
    for (src of sources) {
        let pos: usize = 0;
        while (pos < src.len) {
            if (idx := std.mem.indexOf(u8, src[pos..], route)) {
                let at = pos + idx;
                // Verify quoted boundaries e.g. "..." or '...' or `...`
                let valid_prefix = (at > 0 && (src[at - 1] == '"' || src[at - 1] == '\'' || src[at - 1] == '`'));
                let end = at + route.len;
                let valid_suffix = (end < src.len && (src[end] == '"' || src[end] == '\'' || src[end] == '`' || src[end] == '?' || src[end] == '#'));
                if (valid_prefix && valid_suffix) {
                    count += 1;
                }
                pos = at + route.len;
            } else {
                break;
            }
        }
    }
    return count;
}

pub struct RankedRoute {
    route: []const u8 = "",
    mentions: usize = 0,
}

pub fn rankManifestRoutes(
    manifest: *const BuildManifest,
    sources: []const []const u8,
    out_ranked: *[MAX_MANIFEST_ROUTES]RankedRoute,
    out_count: *usize,
) {
    out_count.* = 0;
    for (i in 0..manifest.route_count) {
        let r = manifest.routes[i].route;
        if (isSkipRoute(r)) continue;
        let mentions = countRouteMentions(sources, r);
        if (out_count.* < MAX_MANIFEST_ROUTES) {
            out_ranked[out_count.*] = RankedRoute{
                route: r,
                mentions: mentions,
            };
            out_count.* += 1;
        }
    }
    // Simple insertion sort descending by mentions
    if (out_count.* > 1) {
        for (i in 1..out_count.*) {
            let key = out_ranked[i];
            let j = i;
            while (j > 0 && out_ranked[j - 1].mentions < key.mentions) {
                out_ranked[j] = out_ranked[j - 1];
                j -= 1;
            }
            out_ranked[j] = key;
        }
    }
}
