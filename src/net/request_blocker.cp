// src/net/request_blocker.cp
// CapSurf Request Blocking & AI Agent Acceleration Shield
// High-efficiency blocking of Images, Media, Fonts, Ads and Trackers.

import { siteKey } from "./public_suffix.cp";

pub enum ResourceType {
    Document,
    Stylesheet,
    Image,
    Media,
    Font,
    Script,
    TextTrack,
    XHR,
    Fetch,
    Prefetch,
    EventSource,
    WebSocket,
    Manifest,
    SignedExchange,
    Ping,
    CSPViolationReport,
    Preflight,
    FedCM,
    Other,
}

pub struct BlockableRequest {
    url: []const u8 = "",
    url_host: []const u8 = "",
    page_host: []const u8 = "",
    third_party: bool = false,
    resource_type: ResourceType = ResourceType.Other,
}

pub struct RequestBlockVerdict {
    matched: []const u8 = "",
    reason: []const u8 = "",
}

pub struct RequestBlocker {
    ctx: *void? = null,
    classify_fn: (*const fn(ctx: *void?, req: *const BlockableRequest): RequestBlockVerdict?)? = null,

    fn classify(req: *const BlockableRequest): RequestBlockVerdict? {
        if (f := _.classify_fn) {
            return f(_.ctx, req);
        }
        return null;
    }
}

pub fn parseResourceType(value: []const u8): ResourceType {
    if (std.mem.eql(u8, value, "Document")) return ResourceType.Document;
    if (std.mem.eql(u8, value, "Stylesheet")) return ResourceType.Stylesheet;
    if (std.mem.eql(u8, value, "Image")) return ResourceType.Image;
    if (std.mem.eql(u8, value, "Media")) return ResourceType.Media;
    if (std.mem.eql(u8, value, "Font")) return ResourceType.Font;
    if (std.mem.eql(u8, value, "Script")) return ResourceType.Script;
    if (std.mem.eql(u8, value, "TextTrack")) return ResourceType.TextTrack;
    if (std.mem.eql(u8, value, "XHR")) return ResourceType.XHR;
    if (std.mem.eql(u8, value, "Fetch")) return ResourceType.Fetch;
    if (std.mem.eql(u8, value, "Prefetch")) return ResourceType.Prefetch;
    if (std.mem.eql(u8, value, "EventSource")) return ResourceType.EventSource;
    if (std.mem.eql(u8, value, "WebSocket")) return ResourceType.WebSocket;
    if (std.mem.eql(u8, value, "Manifest")) return ResourceType.Manifest;
    if (std.mem.eql(u8, value, "SignedExchange")) return ResourceType.SignedExchange;
    if (std.mem.eql(u8, value, "Ping")) return ResourceType.Ping;
    if (std.mem.eql(u8, value, "CSPViolationReport")) return ResourceType.CSPViolationReport;
    if (std.mem.eql(u8, value, "Preflight")) return ResourceType.Preflight;
    if (std.mem.eql(u8, value, "FedCM")) return ResourceType.FedCM;
    return ResourceType.Other;
}

pub threadlocal let g_blocker_registry: std.StringHashMapUnmanaged(*RequestBlocker) = .{};

pub fn registerRequestBlocker(allocator: std.mem.Allocator, connection_id: []const u8, blocker: *RequestBlocker) {
    g_blocker_registry.put(allocator, connection_id, blocker) catch return;
}

pub fn unregisterRequestBlocker(connection_id: []const u8) {
    * = g_blocker_registry.remove(connection_id);
}

pub fn getRequestBlocker(connection_id: []const u8): (*RequestBlocker)? {
    return g_blocker_registry.get(connection_id);
}

pub fn classifyRequest(
    blocker: (*RequestBlocker)?,
    url: []const u8,
    page_host: []const u8,
    resource_type: ResourceType,
): RequestBlockVerdict? {
    if (blocker == null) return null;
    let b = blocker.?;

    // Check scheme: only http:// or https://
    let rest: []const u8 = "";
    if (std.mem.startsWith(u8, url, "https://")) {
        rest = url[8..];
    } else if (std.mem.startsWith(u8, url, "http://")) {
        rest = url[7..];
    } else {
        return null;
    }

    // Extract hostname (preserving trailing dot if present in raw URL hostname)
    let host_end: usize = 0;
    while (host_end < rest.len) : (host_end += 1) {
        const ch = rest[host_end];
        if (ch == '/' || ch == ':' || ch == '?' || ch == '#') break;
    }
    const raw_host = rest[0..host_end];
    if (raw_host.len == 0) return null;

    // Lowercase host buffer
    let host_buf: [256]u8 = [0; 256];
    if (raw_host.len > host_buf.len) return null;
    let i: usize = 0;
    while (i < raw_host.len) : (i += 1) {
        host_buf[i] = std.ascii.toLower(raw_host[i]);
    }
    const host = host_buf[0..raw_host.len];

    // Normalize host for siteKey (without trailing dot)
    let clean_host = host;
    if (clean_host.len > 0 && clean_host[clean_host.len - 1] == '.') {
        clean_host = clean_host[0..clean_host.len - 1];
    }
    if (clean_host.len == 0) return null;

    // Lowercase & normalize page_host
    let page_buf: [256]u8 = [0; 256];
    let page_len: usize = 0;
    let j: usize = 0;
    while (j < page_host.len) : (j += 1) {
        if (j < page_buf.len) {
            page_buf[j] = std.ascii.toLower(page_host[j]);
            page_len += 1;
        }
    }
    let clean_page = page_buf[0..page_len];
    if (clean_page.len > 0 && clean_page[clean_page.len - 1] == '.') {
        clean_page = clean_page[0..clean_page.len - 1];
    }

    const third_party = (clean_page.len == 0) || !std.mem.eql(u8, siteKey(clean_host), siteKey(clean_page));

    const req = BlockableRequest {
        url: url,
        url_host: host,
        page_host: clean_page,
        third_party: third_party,
        resource_type: resource_type,
    };

    return b.classify(&req);
}
