// src/cdp/frames.cp
// CapSurf Multi-Frame / IFrame Isolation & CDP Frame Management
// Migrated from vendor/kitesurf/packages/engine/src/cdp/frames.ts

import { isIpHost, isPublicSuffix, siteKey } from "../net/public_suffix.cp";

pub const MAX_IFRAME_DEPTH: usize = 8;

pub struct SandboxRestrictions {
    scripts: bool = true,
    same_origin: bool = true,
    forms: bool = true,
    top_navigation: bool = true,
}

pub const SANDBOX_ALLOW_ALL = SandboxRestrictions{
    scripts: true,
    same_origin: true,
    forms: true,
    top_navigation: true,
};

pub struct FrameOrigin {
    serialized: []const u8 = "://",
    trustworthy: bool = false,
    localhost: bool = false,
    site: ([]const u8)? = null,
}

pub const OPAQUE_FRAME_ORIGIN = FrameOrigin{
    serialized: "://",
    trustworthy: false,
    localhost: false,
    site: null,
};

pub struct FrameContext {
    secure_context_type: []const u8 = "InsecureScheme",
    cross_origin_isolated_context_type: []const u8 = "NotIsolated",
    same_site_as_parent: bool = false,
}

pub struct CommittedOrigin {
    kind: []const u8 = "opaque",
    id: []const u8 = "",
    origin: []const u8 = "",
}

pub struct FrameState {
    frame_id: []const u8 = "",
    parent_frame_id: ([]const u8)? = null,
    loader_id: []const u8 = "",
    url: []const u8 = "about:blank",
    sandbox: SandboxRestrictions = SANDBOX_ALLOW_ALL,
    srcdoc: ([]const u8)? = null,
    base_url: ([]const u8)? = null,
    name: []const u8 = "",
    owner_element_id: []const u8 = "",
    security_origin: []const u8 = "://",
    mime_type: []const u8 = "text/html",
    owner_backend_node_id: i64 = -1,
    lifecycle: []const u8 = "init",
    detached: bool = false,
    is_cross_origin: bool = false,
    has_script: bool = false,
    delays_load_event: bool = true,
    ready_state: []const u8 = "loading",
    page_script_url: ([]const u8)? = null,
    page_script_sandbox: (SandboxRestrictions)? = null,
    last_render_seq: i64 = -1,
    exec_context_id: i64 = 0,
    referrer_policy: ([]const u8)? = null,
    frame_policy_blocked: bool = false,
    oopif_blocked: bool = false,
    request_blocked: bool = false,
}

pub struct MainFrameArgs {
    frame_id: []const u8 = "",
    loader_id: []const u8 = "",
    url: []const u8 = "about:blank",
}

pub struct ChildFrameArgs {
    frame_id: []const u8 = "",
    parent_frame_id: []const u8 = "",
    loader_id: []const u8 = "",
    url: []const u8 = "about:blank",
    sandbox: (SandboxRestrictions)? = null,
    srcdoc: ([]const u8)? = null,
    base_url: ([]const u8)? = null,
    name: []const u8 = "",
    owner_element_id: []const u8 = "",
    owner_backend_node_id: i64 = -1,
    is_cross_origin: bool = false,
    delays_load_event: bool = true,
}

pub struct FrameJsonWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    static fn init(buf: []u8): FrameJsonWriter {
        return FrameJsonWriter{
            buf: buf,
            pos: 0,
            overflow: false,
        };
    }

    fn writeByte(*_, b: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = b;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeRaw(*_, s: []const u8) {
        if (_.pos + s.len <= _.buf.len) {
            @memcpy(_.buf[_.pos .. _.pos + s.len], s);
            _.pos += s.len;
        } else {
            _.overflow = true;
        }
    }

    fn writeEscapedString(*_, s: []const u8) {
        _.writeByte('"');
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '"') {
                _.writeRaw("\\\"");
            } else if (c == '\\') {
                _.writeRaw("\\\\");
            } else if (c == '\n') {
                _.writeRaw("\\n");
            } else if (c == '\r') {
                _.writeRaw("\\r");
            } else if (c == '\t') {
                _.writeRaw("\\t");
            } else if (c < 0x20) {
                let hex_buf: [8]u8 = [0; 8];
                const hex_str = std.fmt.bufPrint(hex_buf[0..], "\\u00{x:0>2}", .{ c }) catch "\\u0000";
                _.writeRaw(hex_str);
            } else {
                _.writeByte(c);
            }
        }
        _.writeByte('"');
    }

    fn writeBool(*_, b: bool) {
        if (b) {
            _.writeRaw("true");
        } else {
            _.writeRaw("false");
        }
    }

    fn writeInt(*_, n: i64) {
        let n_buf: [32]u8 = undefined;
        const s = std.fmt.bufPrint(n_buf[0..], "{d}", .{ n }) catch "0";
        _.writeRaw(s);
    }

    fn getWritten(): []const u8 {
        return _.buf[0.._.pos];
    }
}

let g_frame_seq: u64 = 1;
let g_loader_seq: u64 = 1;

pub fn mintFrameId(): []const u8 {
    let buf: [48]u8 = undefined;
    const n = g_frame_seq;
    g_frame_seq += 1;
    const s = std.fmt.bufPrint(buf[0..], "capsurf-frame-{d:0>4}", .{ n }) catch "capsurf-frame-0001";
    return std.heap.c_allocator.dupe(u8, s) catch "capsurf-frame-0001";
}

pub fn mintLoaderId(): []const u8 {
    let buf: [48]u8 = undefined;
    const n = g_loader_seq;
    g_loader_seq += 1;
    const s = std.fmt.bufPrint(buf[0..], "capsurf-loader-{d:0>4}", .{ n }) catch "capsurf-loader-0001";
    return std.heap.c_allocator.dupe(u8, s) catch "capsurf-loader-0001";
}

pub fn matchesAboutBlank(url: []const u8): bool {
    if (std.mem.eql(u8, url, "about:blank")) return true;
    if (std.mem.startsWith(u8, url, "about:blank?") || std.mem.startsWith(u8, url, "about:blank#")) return true;
    return false;
}

pub fn originOf(url: []const u8): []const u8 {
    if (url.len == 0) return "://";
    if (matchesAboutBlank(url)) return "about://";

    if (p_idx := std.mem.indexOf(u8, url, "://")) {
        const proto = url[0 .. p_idx + 3];
        const rest = url[p_idx + 3 ..];
        let end: usize = 0;
        while (end < rest.len) : (end += 1) {
            const c = rest[end];
            if (c == '/' || c == '?' || c == '#') break;
        }
        const authority = rest[0..end];
        if (at_idx := std.mem.indexOfScalar(u8, authority, '@')) {
            const host_with_port = authority[at_idx + 1 ..];
            let buf: [256]u8 = undefined;
            const s = std.fmt.bufPrint(buf[0..], "{s}{s}", .{ proto, host_with_port }) catch "://";
            return std.heap.c_allocator.dupe(u8, s) catch "://";
        }
        return url[0 .. p_idx + 3 + end];
    }

    return "://";
}

pub fn extractHostname(url: []const u8): []const u8 {
    let rest: []const u8 = "";
    if (p_idx := std.mem.indexOf(u8, url, "://")) {
        rest = url[p_idx + 3 ..];
    } else {
        return "";
    }

    let end: usize = 0;
    while (end < rest.len) : (end += 1) {
        const c = rest[end];
        if (c == '/' || c == '?' || c == '#') break;
    }
    let authority = rest[0..end];

    if (at_idx := std.mem.indexOfScalar(u8, authority, '@')) {
        authority = authority[at_idx + 1 ..];
    }

    let hostname = authority;
    if (colon_idx := std.mem.lastIndexOfScalar(u8, authority, ':')) {
        if (!std.mem.endsWith(u8, authority, "]")) {
            hostname = authority[0..colon_idx];
        }
    }

    if (hostname.len > 0 && hostname[hostname.len - 1] == '.') {
        hostname = hostname[0 .. hostname.len - 1];
    }
    return hostname;
}

pub fn domainAndRegistryOf(url: []const u8): []const u8 {
    const host = extractHostname(url);
    if (host.len == 0 || isIpHost(host) || std.mem.indexOfScalar(u8, host, '.') == null || isPublicSuffix(host)) {
        return "";
    }
    return siteKey(host);
}

pub fn is127Net(host: []const u8): bool {
    if (!std.mem.startsWith(u8, host, "127.")) return false;
    let dots: usize = 0;
    let cur_octet: u32 = 0;
    let octet_digits: usize = 0;
    let i: usize = 0;
    while (i < host.len) : (i += 1) {
        const c = host[i];
        if (c == '.') {
            dots += 1;
            if (octet_digits == 0 || octet_digits > 3 || cur_octet > 255) return false;
            cur_octet = 0;
            octet_digits = 0;
        } else if (c >= '0' && c <= '9') {
            cur_octet = cur_octet * 10 + @as(u32, c - '0');
            octet_digits += 1;
        } else {
            return false;
        }
    }
    if (dots != 3 || octet_digits == 0 || octet_digits > 3 || cur_octet > 255) return false;
    return true;
}

pub fn isLocalhost(host: []const u8): bool {
    if (std.mem.eql(u8, host, "localhost")) return true;
    if (std.mem.endsWith(u8, host, ".localhost")) return true;
    if (std.mem.eql(u8, host, "[::1]") || std.mem.eql(u8, host, "::1")) return true;
    if (is127Net(host)) return true;
    return false;
}

pub fn frameOriginOf(frame: *const FrameState): FrameOrigin {
    if (!frame.sandbox.same_origin) return OPAQUE_FRAME_ORIGIN;
    const url = if (frame.url.len > 0) frame.url else "about:blank";
    if (matchesAboutBlank(url)) return OPAQUE_FRAME_ORIGIN;

    let proto: []const u8 = "";
    let rest: []const u8 = "";
    if (std.mem.startsWith(u8, url, "https://")) {
        proto = "https:";
        rest = url[8..];
    } else if (std.mem.startsWith(u8, url, "http://")) {
        proto = "http:";
        rest = url[7..];
    } else if (std.mem.startsWith(u8, url, "wss://")) {
        proto = "wss:";
        rest = url[6..];
    } else if (std.mem.startsWith(u8, url, "ws://")) {
        proto = "ws:";
        rest = url[5..];
    } else {
        return OPAQUE_FRAME_ORIGIN;
    }

    let host_end: usize = 0;
    while (host_end < rest.len) : (host_end += 1) {
        const ch = rest[host_end];
        if (ch == '/' || ch == '?' || ch == '#') break;
    }
    const authority = rest[0..host_end];
    let host = authority;
    if (at_idx := std.mem.indexOfScalar(u8, authority, '@')) {
        host = authority[at_idx + 1 ..];
    }
    let hostname = host;
    if (colon_idx := std.mem.lastIndexOfScalar(u8, host, ':')) {
        if (!std.mem.endsWith(u8, host, "]")) {
            hostname = host[0..colon_idx];
        }
    }
    if (hostname.len > 0 && hostname[hostname.len - 1] == '.') {
        hostname = hostname[0 .. hostname.len - 1];
    }

    const is_lh = isLocalhost(hostname);
    const is_trustworthy = is_lh || std.mem.eql(u8, proto, "https:") || std.mem.eql(u8, proto, "wss:");

    const end_offset = (url.len - rest.len) + host_end;
    const serialized = url[0..end_offset];

    const reg_domain = domainAndRegistryOf(serialized);
    const site_domain = if (reg_domain.len > 0) reg_domain else hostname;

    let site_buf: [256]u8 = undefined;
    const site_str = std.fmt.bufPrint(site_buf[0..], "{s}//{s}", .{ proto, site_domain }) catch serialized;
    const site_dupe = std.heap.c_allocator.dupe(u8, site_str) catch site_str;

    return FrameOrigin{
        serialized: serialized,
        trustworthy: is_trustworthy,
        localhost: is_lh,
        site: site_dupe,
    };
}

pub fn inheritsCreatorOrigin(url: []const u8): bool {
    if (url.len == 0) return true;
    if (std.mem.eql(u8, url, "about:blank") || std.mem.eql(u8, url, "about:srcdoc")) return true;
    if (std.mem.startsWith(u8, url, "about:blank?") || std.mem.startsWith(u8, url, "about:blank#")) return true;
    if (std.mem.startsWith(u8, url, "about:srcdoc?") || std.mem.startsWith(u8, url, "about:srcdoc#")) return true;
    return false;
}

fn findFrame(id: []const u8, frames: []const FrameState): (*const FrameState)? {
    let i: usize = 0;
    while (i < frames.len) : (i += 1) {
        if (std.mem.eql(u8, frames[i].frame_id, id)) return &frames[i];
    }
    return null;
}

pub fn frameContextOf(frame: *const FrameState, all_frames: []const FrameState): FrameContext {
    let rev_chain: [16]*const FrameState = [frame; 16];
    let count: usize = 0;
    let cur: (*const FrameState)? = frame;
    while (count < 16) {
        if (c := cur) {
            let cycle = false;
            let i: usize = 0;
            while (i < count) : (i += 1) {
                if (std.mem.eql(u8, rev_chain[i].frame_id, c.frame_id)) {
                    cycle = true;
                    break;
                }
            }
            if (cycle) break;

            const pid = c.parent_frame_id;
            rev_chain[count] = c;
            count += 1;

            if (p := pid) {
                cur = findFrame(p, all_frames);
            } else {
                break;
            }
        } else {
            break;
        }
    }

    let chain: [16]*const FrameState = [frame; 16];
    let k: usize = 0;
    while (k < count) : (k += 1) {
        chain[k] = rev_chain[count - 1 - k];
    }

    let origin = OPAQUE_FRAME_ORIGIN;
    let site: ([]const u8)? = null;
    let ancestors_secure = true;
    let isolation_allowed = true;
    let context = FrameContext{
        secure_context_type: "InsecureScheme",
        cross_origin_isolated_context_type: "NotIsolated",
        same_site_as_parent: false,
    };

    let depth: usize = 0;
    while (depth < count) : (depth += 1) {
        const current = chain[depth];
        const parent_origin = origin;
        const parent_site = site;
        const is_opaque = !current.sandbox.same_origin;
        const inherits = !is_opaque && depth > 0 && inheritsCreatorOrigin(current.url);
        if (!inherits) {
            origin = frameOriginOf(current);
        }
        if (s := origin.site) {
            site = s;
        }
        if (depth > 0) {
            let same_origin = inherits;
            if (!same_origin) {
                if (origin.site != null && std.mem.eql(u8, origin.serialized, parent_origin.serialized)) {
                    same_origin = true;
                }
            }
            isolation_allowed = isolation_allowed && same_origin && !is_opaque;
            let same_site = false;
            if (!is_opaque) {
                if (s1 := site) {
                    if (s2 := parent_site) {
                        same_site = std.mem.eql(u8, s1, s2);
                    }
                }
            }
            context.same_site_as_parent = same_site;
        }

        let sec_type: []const u8 = "InsecureScheme";
        if (!origin.trustworthy) {
            sec_type = "InsecureScheme";
        } else if (!ancestors_secure) {
            sec_type = "InsecureAncestor";
        } else {
            sec_type = if (origin.localhost) "SecureLocalhost" else "Secure";
        }
        context.secure_context_type = sec_type;
        ancestors_secure = ancestors_secure && (std.mem.eql(u8, sec_type, "Secure") || std.mem.eql(u8, sec_type, "SecureLocalhost"));
    }

    context.cross_origin_isolated_context_type = if (isolation_allowed) "NotIsolated" else "NotIsolatedFeatureDisabled";
    return context;
}

pub fn opaqueCommittedOrigin(frame: *const FrameState): CommittedOrigin {
    let buf: [128]u8 = undefined;
    const s = std.fmt.bufPrint(buf[0..], "{s}:{s}", .{ frame.frame_id, frame.loader_id }) catch "opaque";
    const dupe = std.heap.c_allocator.dupe(u8, s) catch "opaque";
    return CommittedOrigin{
        kind: "opaque",
        id: dupe,
        origin: "",
    };
}

pub fn committedFrameOriginOf(
    frame: *const FrameState,
    frames: []const FrameState,
    main_url: []const u8,
): CommittedOrigin {
    let sb = frame.sandbox;
    if (s := frame.page_script_sandbox) {
        sb = s;
    }
    if (!sb.same_origin) return opaqueCommittedOrigin(frame);

    let cur = frame;
    let depth: usize = 0;
    while (inheritsCreatorOrigin(cur.url) && cur.parent_frame_id != null && depth < MAX_IFRAME_DEPTH) : (depth += 1) {
        const pid = cur.parent_frame_id.?;
        const parent = findFrame(pid, frames);
        if (parent == null) break;
        const p = parent.?;
        let p_sb = p.sandbox;
        if (ps := p.page_script_sandbox) {
            p_sb = ps;
        }
        if (!p_sb.same_origin) return opaqueCommittedOrigin(p);
        cur = p;
    }

    let url = cur.url;
    if (cur.parent_frame_id == null) {
        if (main_url.len > 0) {
            url = main_url;
        }
    } else {
        if (psu := cur.page_script_url) {
            url = psu;
        }
    }

    const orig = originOf(url);
    if (std.mem.eql(u8, orig, "://") || std.mem.eql(u8, orig, "null") || orig.len == 0) {
        return opaqueCommittedOrigin(cur);
    }
    return CommittedOrigin{
        kind: "tuple",
        id: "",
        origin: orig,
    };
}

pub fn frameAncestorOrigins(
    frame: *const FrameState,
    frames: []const FrameState,
    main_url: []const u8,
    out_origins: []CommittedOrigin,
): usize {
    let count: usize = 0;
    let cur_id = frame.parent_frame_id;
    while (cur_id != null && count < out_origins.len && count < MAX_IFRAME_DEPTH) {
        const parent = findFrame(cur_id.?, frames);
        if (parent == null) break;
        const p = parent.?;
        out_origins[count] = committedFrameOriginOf(p, frames, main_url);
        count += 1;
        cur_id = p.parent_frame_id;
    }
    return count;
}

pub fn makeMainFrameState(args: MainFrameArgs): FrameState {
    return FrameState{
        frame_id: args.frame_id,
        parent_frame_id: null,
        loader_id: args.loader_id,
        url: args.url,
        sandbox: SANDBOX_ALLOW_ALL,
        srcdoc: null,
        base_url: null,
        name: "",
        owner_element_id: "",
        security_origin: originOf(args.url),
        mime_type: "text/html",
        owner_backend_node_id: -1,
        lifecycle: "init",
        detached: false,
        is_cross_origin: false,
        has_script: false,
        delays_load_event: true,
        ready_state: "loading",
        page_script_url: null,
        page_script_sandbox: null,
        last_render_seq: -1,
        exec_context_id: 0,
    };
}

pub fn makeChildFrameState(args: ChildFrameArgs): FrameState {
    let sb = SANDBOX_ALLOW_ALL;
    if (s := args.sandbox) {
        sb = s;
    }
    return FrameState{
        frame_id: args.frame_id,
        parent_frame_id: args.parent_frame_id,
        loader_id: args.loader_id,
        url: args.url,
        sandbox: sb,
        srcdoc: args.srcdoc,
        base_url: args.base_url,
        name: args.name,
        owner_element_id: args.owner_element_id,
        security_origin: originOf(args.url),
        mime_type: "text/html",
        owner_backend_node_id: args.owner_backend_node_id,
        lifecycle: "init",
        detached: false,
        is_cross_origin: args.is_cross_origin,
        has_script: args.is_cross_origin,
        delays_load_event: args.delays_load_event,
        ready_state: "loading",
        page_script_url: null,
        page_script_sandbox: null,
        last_render_seq: -1,
        exec_context_id: 0,
    };
}

pub fn writeFrameJson(writer: *FrameJsonWriter, frame: *const FrameState, all_frames: []const FrameState) {
    const href = if (frame.url.len > 0) frame.url else "about:blank";
    let url = href;
    let url_fragment: []const u8 = "";
    if (hash_idx := std.mem.indexOfScalar(u8, href, '#')) {
        url = href[0..hash_idx];
        url_fragment = href[hash_idx..];
    }
    const origin = frameOriginOf(frame);
    const context = frameContextOf(frame, all_frames);
    const domain_reg = domainAndRegistryOf(url);
    const mime = if (frame.mime_type.len > 0) frame.mime_type else "text/html";

    writer.writeRaw("{\"id\":");
    writer.writeEscapedString(frame.frame_id);
    writer.writeRaw(",\"loaderId\":");
    writer.writeEscapedString(frame.loader_id);
    writer.writeRaw(",\"url\":");
    writer.writeEscapedString(url);
    writer.writeRaw(",\"domainAndRegistry\":");
    writer.writeEscapedString(domain_reg);
    writer.writeRaw(",\"securityOrigin\":");
    writer.writeEscapedString(origin.serialized);
    writer.writeRaw(",\"securityOriginDetails\":{\"isLocalhost\":");
    writer.writeBool(origin.localhost);
    writer.writeRaw("},\"mimeType\":");
    writer.writeEscapedString(mime);
    writer.writeRaw(",\"secureContextType\":");
    writer.writeEscapedString(context.secure_context_type);
    writer.writeRaw(",\"crossOriginIsolatedContextType\":");
    writer.writeEscapedString(context.cross_origin_isolated_context_type);
    writer.writeRaw(",\"gatedAPIFeatures\":[]");

    if (pid := frame.parent_frame_id) {
        writer.writeRaw(",\"adFrameStatus\":{\"adFrameType\":\"none\",\"explanations\":[]}");
        writer.writeRaw(",\"parentId\":");
        writer.writeEscapedString(pid);
        let name_str = frame.name;
        if (name_str.len == 0) {
            name_str = frame.owner_element_id;
        }
        writer.writeRaw(",\"name\":");
        writer.writeEscapedString(name_str);
    } else {
        writer.writeRaw(",\"adFrameStatus\":{\"adFrameType\":\"none\"}");
    }

    if (url_fragment.len > 0) {
        writer.writeRaw(",\"urlFragment\":");
        writer.writeEscapedString(url_fragment);
    }

    writer.writeRaw("}");
}

pub fn makeFrameInfoWithFrames(frame: *const FrameState, all_frames: []const FrameState, out_buf: []u8): []const u8 {
    let writer = FrameJsonWriter.init(out_buf);
    writeFrameJson(&writer, frame, all_frames);
    if (writer.overflow) return "{}";
    return writer.getWritten();
}

pub fn makeFrameInfo(frame: *FrameState, out_buf: []u8): []const u8 {
    let empty_frames: [1]FrameState = [frame.*];
    return makeFrameInfoWithFrames(frame, empty_frames[0..1], out_buf);
}

fn writeFrameSubtree(writer: *FrameJsonWriter, frame: *const FrameState, all_frames: []const FrameState) {
    writer.writeRaw("{\"frame\":");
    writeFrameJson(writer, frame, all_frames);

    let child_count: usize = 0;
    let i: usize = 0;
    while (i < all_frames.len) : (i += 1) {
        if (pid := all_frames[i].parent_frame_id) {
            if (std.mem.eql(u8, pid, frame.frame_id)) {
                child_count += 1;
            }
        }
    }

    if (child_count > 0) {
        writer.writeRaw(",\"childFrames\":[");
        let wrote_child: usize = 0;
        let j: usize = 0;
        while (j < all_frames.len) : (j += 1) {
            if (pid := all_frames[j].parent_frame_id) {
                if (std.mem.eql(u8, pid, frame.frame_id)) {
                    if (wrote_child > 0) {
                        writer.writeRaw(",");
                    }
                    writeFrameSubtree(writer, &all_frames[j], all_frames);
                    wrote_child += 1;
                }
            }
        }
        writer.writeRaw("]");
    }
    writer.writeRaw("}");
}

pub fn buildFrameTree(frames: []FrameState, out_buf: []u8): []const u8 {
    if (frames.len == 0) {
        let def = makeMainFrameState(MainFrameArgs{
            frame_id: "capsurf-frame-001",
            loader_id: "capsurf-loader-001",
            url: "about:blank",
        });
        let def_arr: [1]FrameState = [def];
        return buildFrameTree(def_arr[0..1], out_buf);
    }

    let root_ptr: *const FrameState = &frames[0];
    let i: usize = 0;
    while (i < frames.len) : (i += 1) {
        if (frames[i].parent_frame_id == null) {
            root_ptr = &frames[i];
            break;
        }
    }

    let writer = FrameJsonWriter.init(out_buf);
    writeFrameSubtree(&writer, root_ptr, frames);
    if (writer.overflow) return "{}";
    return writer.getWritten();
}

pub fn buildFrameTreeAt(frames: []FrameState, root_id: []const u8, out_buf: []u8): []const u8 {
    let root_ptr: (*const FrameState)? = null;
    let i: usize = 0;
    while (i < frames.len) : (i += 1) {
        if (std.mem.eql(u8, frames[i].frame_id, root_id)) {
            root_ptr = &frames[i];
            break;
        }
    }
    if (r := root_ptr) {
        let writer = FrameJsonWriter.init(out_buf);
        writeFrameSubtree(&writer, r, frames);
        if (writer.overflow) return "{}";
        return writer.getWritten();
    }
    return buildFrameTree(frames, out_buf);
}

fn writeFrameResourceSubtree(writer: *FrameJsonWriter, frame: *const FrameState, all_frames: []const FrameState) {
    writer.writeRaw("{\"frame\":");
    writeFrameJson(writer, frame, all_frames);
    writer.writeRaw(",\"resources\":[]");

    let child_count: usize = 0;
    let i: usize = 0;
    while (i < all_frames.len) : (i += 1) {
        if (pid := all_frames[i].parent_frame_id) {
            if (std.mem.eql(u8, pid, frame.frame_id)) {
                child_count += 1;
            }
        }
    }

    if (child_count > 0) {
        writer.writeRaw(",\"childFrames\":[");
        let wrote_child: usize = 0;
        let j: usize = 0;
        while (j < all_frames.len) : (j += 1) {
            if (pid := all_frames[j].parent_frame_id) {
                if (std.mem.eql(u8, pid, frame.frame_id)) {
                    if (wrote_child > 0) {
                        writer.writeRaw(",");
                    }
                    writeFrameResourceSubtree(writer, &all_frames[j], all_frames);
                    wrote_child += 1;
                }
            }
        }
        writer.writeRaw("]");
    }
    writer.writeRaw("}");
}

pub fn buildFrameResourceTree(frames: []FrameState, out_buf: []u8): []const u8 {
    if (frames.len == 0) {
        let def = makeMainFrameState(MainFrameArgs{
            frame_id: "capsurf-frame-001",
            loader_id: "capsurf-loader-001",
            url: "about:blank",
        });
        let def_arr: [1]FrameState = [def];
        return buildFrameResourceTree(def_arr[0..1], out_buf);
    }

    let root_ptr: *const FrameState = &frames[0];
    let i: usize = 0;
    while (i < frames.len) : (i += 1) {
        if (frames[i].parent_frame_id == null) {
            root_ptr = &frames[i];
            break;
        }
    }

    let writer = FrameJsonWriter.init(out_buf);
    writeFrameResourceSubtree(&writer, root_ptr, frames);
    if (writer.overflow) return "{}";
    return writer.getWritten();
}
