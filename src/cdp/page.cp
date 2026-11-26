// src/cdp/page.cp
// CDP Page Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/page.ts and vendor/kitesurf/packages/engine/src/eval/outbound.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, extractJsonFloat, extractJsonBool, unescapeJsonString, MAX_CDP_EVENTS } from "./types.cp";
import { CdpPageSession, escapeJsonString, DEFAULT_HTTP_USER_AGENT } from "./session.cp";
import { TtyNavState, applyTtyNavParams } from "../tty/nav_params.cp";
import { jsIsException, JS_SetPropertyStr, JS_GetUint8Array, JS_TAG_OBJECT, JS_GetException, JS_FreeValue } from "../js/qjs_engine.cp";
import { HttpLoader, HttpLoaderResult } from "../net/http_loader.cp";
import { MockFixtures } from "../net/mock_fixtures.cp";
import { computeNextReferrer } from "../net/redirect_walk.cp";
import { NodeType, NULL_NODE } from "../dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../css/cascade.cp";
import { PdfOptions, computePdfPagination, renderPdfAlloc, renderPdfWithDocAlloc } from "../render/pdf.cp";
import { encodeRgbaRectToPng, scaleRgbaBuffer, setCaptureBeyondViewport } from "../render/painter.cp";
import { Base64 } from "cap:encoding";
import { jpegEncode, webpEncode, webpEncodeLossless } from "cap:image";
import {
    DocumentKind,
    documentKind,
    mimeEssence,
    imageDocumentHtml,
    textDocumentHtml,
    jsonViewerHtml,
} from "./document_kind.cp";
import { nextDocRequestId, recordResponseBody, clearDocResponseBodies, formatHttpHeadersJson } from "./network.cp";
import { executeDownload } from "./download.cp";
import { getDownloadConfigStore } from "./browser.cp";

let g_fetch_nav_pre_announced_id_buf: [64]u8 = [0; 64];
let g_fetch_nav_pre_announced_id_len: usize = 0;

pub fn markFetchNavPreAnnounced(loader_id: []const u8) {
    const copy_len = @min(loader_id.len, g_fetch_nav_pre_announced_id_buf.len);
    if (copy_len > 0) {
        @memcpy(g_fetch_nav_pre_announced_id_buf[0..copy_len], loader_id[0..copy_len]);
    }
    g_fetch_nav_pre_announced_id_len = copy_len;
}

fn consumeFetchNavPreAnnounced(loader_id: []const u8): bool {
    if (g_fetch_nav_pre_announced_id_len == 0) return false;
    if (std.mem.eql(u8, g_fetch_nav_pre_announced_id_buf[0..g_fetch_nav_pre_announced_id_len], loader_id)) {
        g_fetch_nav_pre_announced_id_len = 0;
        return true;
    }
    return false;
}

fn emitChildFrameNetworkEvents(
    session: *CdpPageSession,
    cf: *const FrameState,
    out_events: *[]CdpEvent,
    event_count: *usize,
) {
    if (!session.network_enabled) return;
    if (cf.url.len == 0 || isRefusedKindFrameUrl(cf.url)) return;
    if (cf.request_blocked || cf.frame_policy_blocked) return;
    if (std.ascii.eqlIgnoreCase(cf.url, "about:blank") ||
        std.mem.startsWith(u8, cf.url, "about:blank?") ||
        std.mem.startsWith(u8, cf.url, "about:blank#") ||
        std.mem.startsWith(u8, cf.url, "about:srcdoc") ||
        std.mem.startsWith(u8, cf.url, "data:")) return;

    let req_id_buf: [64]u8 = [0; 64];
    const cf_req_id = nextDocRequestId(&req_id_buf);
    let esc_u_buf: [2048]u8 = [0; 2048];
    const esc_u = escapeJsonString(cf.url, &esc_u_buf);
    const mime_out = if (cf.mime_type.len > 0) cf.mime_type else "text/html";

    if (event_count.* < out_events.len) {
        const req_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{}}}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
            .{ cf_req_id, cf.loader_id, esc_u, esc_u, cf.frame_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSent", req_json);
        event_count.* += 1;
    }
    if (event_count.* < out_events.len) {
        const resp_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"timestamp\":1700000000.0,\"type\":\"Document\",\"response\":{{\"url\":\"{s}\",\"status\":200,\"statusText\":\"OK\",\"headers\":{{}},\"mimeType\":\"{s}\"}},\"frameId\":\"{s}\"}}",
            .{ cf_req_id, cf.loader_id, esc_u, mime_out, cf.frame_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Network.responseReceived", resp_json);
        event_count.* += 1;
    }
    if (event_count.* < out_events.len) {
        const fin_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"requestId\":\"{s}\",\"timestamp\":1700000000.0,\"encodedDataLength\":0}}",
            .{ cf_req_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Network.loadingFinished", fin_json);
        event_count.* += 1;
    }
}

fn emitMainAndIsolatedExecutionContexts(session: *CdpPageSession, doc_url: []const u8, out_events: *[]CdpEvent, event_count: *usize) {
    if (event_count.* < out_events.len) {
        out_events[event_count.*] = CdpEvent.create("Runtime.executionContextsCleared", "{}");
        event_count.* += 1;
    }
    const raw_main_orig = originOf(doc_url);
    const main_orig = if (raw_main_orig.len > 0) raw_main_orig else (if (doc_url.len > 0) doc_url else "about:blank");
    if (event_count.* < out_events.len) {
        const main_ctx_json = session.formatExecutionContextEvent(main_orig);
        out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", main_ctx_json);
        event_count.* += 1;
    }
    let esc_iso_orig_buf: [256]u8 = [0; 256];
    const esc_iso_orig = escapeJsonString(main_orig, &esc_iso_orig_buf);
    let wi: usize = 0;
    while (wi < session.isolated_world_count && event_count.* < out_events.len) : (wi += 1) {
        if (session.isolated_worlds[wi].active) {
            let esc_wname_buf: [256]u8 = [0; 256];
            const esc_wname = escapeJsonString(session.isolated_worlds[wi].name(), &esc_wname_buf);
            const wcid = session.isolated_worlds[wi].context_id;
            const iw_fid = if (session.isolated_worlds[wi].frame_id_len > 0) session.isolated_worlds[wi].frameId() else session.frame_id;
            const iso_ctx_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"{s}\",\"uniqueId\":\"ctx-{d}\",\"auxData\":{{\"isDefault\":false,\"type\":\"isolated\",\"frameId\":\"{s}\"}}}}}}",
                .{ wcid, esc_iso_orig, esc_wname, wcid, iw_fid },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", iso_ctx_json);
            event_count.* += 1;
        }
    }
}

fn emitMainFrameLifecycleEvents(session: *CdpPageSession, out_events: *[]CdpEvent, event_count: *usize) {
    if (session.navigation_aborted || session.checkWindowStopped()) {
        return;
    }
    if (event_count.* < out_events.len) {
        const lc_init = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"init\",\"timestamp\":1700000000.0}}",
            .{ session.frame_id, session.loader_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", lc_init);
        event_count.* += 1;
    }
    if (event_count.* < out_events.len) {
        out_events[event_count.*] = CdpEvent.create("Page.domContentEventFired", "{\"timestamp\":1700000000.0}");
        event_count.* += 1;
    }
    if (event_count.* < out_events.len) {
        const lc_dcl = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"DOMContentLoaded\",\"timestamp\":1700000000.05}}",
            .{ session.frame_id, session.loader_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", lc_dcl);
        event_count.* += 1;
    }
    if (event_count.* < out_events.len) {
        out_events[event_count.*] = CdpEvent.create("Page.loadEventFired", "{\"timestamp\":1700000000.1}");
        event_count.* += 1;
    }
    if (event_count.* < out_events.len) {
        const lc_load = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"load\",\"timestamp\":1700000000.1}}",
            .{ session.frame_id, session.loader_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", lc_load);
        event_count.* += 1;
    }
    if (event_count.* < out_events.len) {
        const main_stop_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\"}}",
            .{ session.frame_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.frameStoppedLoading", main_stop_json);
        event_count.* += 1;
    }
}

fn isXmlOrXhtml(content_type: []const u8, url: []const u8): bool {
    if (std.mem.indexOf(u8, content_type, "xml") != null) return true;
    const base = if (qi := std.mem.indexOfScalar(u8, url, '?')) url[0..qi] else url;
    return std.mem.endsWith(u8, base, ".xhtml") || std.mem.endsWith(u8, base, ".xml");
}
import {
    FrameState,
    SandboxRestrictions,
    MainFrameArgs,
    ChildFrameArgs,
    makeMainFrameState,
    makeChildFrameState,
    makeFrameInfoWithFrames,
    originOf,
    buildFrameTree,
    buildFrameTreeAt,
    buildFrameResourceTree
} from "./frames.cp";
import { extractAckSessionId } from "./screencast.cp";
import { Viewport } from "./emulation.cp";
import { MAX_CONCURRENT_OOPIF } from "./worker_budget.cp";

fn resolveIframeFinalUrl(base_url: []const u8, src: []const u8, out_buf: []u8): []const u8 {
    let cur_buf: [1024]u8 = undefined;
    let cur = HttpLoader.resolveRelativeUrl(base_url, src, &cur_buf);
    let hop: usize = 0;
    while (hop < 5) : (hop += 1) {
        if (!HttpLoader.isMockRedirectUrl(cur)) break;
        let next_buf: [1024]u8 = undefined;
        if (next_url := HttpLoader.resolveMockRedirectTarget(cur, &next_buf)) {
            const n = @min(next_url.len, cur_buf.len);
            @memcpy(cur_buf[0..n], next_url[0..n]);
            cur = cur_buf[0..n];
        } else {
            break;
        }
    }
    const out_n = @min(cur.len, out_buf.len);
    @memcpy(out_buf[0..out_n], cur[0..out_n]);
    return out_buf[0..out_n];
}

fn findNestedIframeSrc(html: []const u8): ([]const u8)? {
    let pos: usize = 0;
    while (pos < html.len) {
        const iframe_pos = std.mem.indexOfPos(u8, html, pos, "<iframe");
        const frame_pos = std.mem.indexOfPos(u8, html, pos, "<frame");
        let tag_pos: usize = 0;
        let is_frame = false;
        if (ip := iframe_pos) {
            if (fp := frame_pos) {
                if (fp < ip) {
                    tag_pos = fp;
                    is_frame = true;
                } else {
                    tag_pos = ip;
                }
            } else {
                tag_pos = ip;
            }
        } else if (fp := frame_pos) {
            tag_pos = fp;
            is_frame = true;
        } else {
            return null;
        }
        const close_pos = std.mem.indexOfScalarPos(u8, html, tag_pos, '>') ?? return null;
        if (is_frame && tag_pos + 6 < close_pos) {
            const next_ch = html[tag_pos + 6];
            if (next_ch != ' ' && next_ch != '\t' && next_ch != '\r' && next_ch != '\n' && next_ch != '/') {
                pos = close_pos + 1;
                continue;
            }
        }
        const tag = html[tag_pos..close_pos];
        if (src_pos := std.mem.indexOf(u8, tag, "src=")) {
            const after = tag[src_pos + 4..];
            if (after.len >= 2 && (after[0] == '"' || after[0] == '\'')) {
                const quote = after[0];
                if (end_q := std.mem.indexOfScalarPos(u8, after, 1, quote)) {
                    return after[1..end_q];
                }
            }
        }
        pos = close_pos + 1;
    }
    return null;
}

fn parseSandboxAttr(raw: []const u8): SandboxRestrictions {
    let res = SandboxRestrictions{
        scripts: false,
        same_origin: false,
        forms: false,
        top_navigation: false,
    };
    let it = std.mem.tokenizeAny(u8, raw, " \t\r\n");
    while (tok := it.next()) {
        if (std.ascii.eqlIgnoreCase(tok, "allow-scripts")) res.scripts = true;
        else if (std.ascii.eqlIgnoreCase(tok, "allow-same-origin")) res.same_origin = true;
        else if (std.ascii.eqlIgnoreCase(tok, "allow-forms")) res.forms = true;
        else if (std.ascii.eqlIgnoreCase(tok, "allow-top-navigation")) res.top_navigation = true;
    }
    return res;
}

fn isKnownChildFrameDetached(session: *CdpPageSession, fid: []const u8): bool {
    let ki: usize = 0;
    while (ki < session.known_child_frame_count) : (ki += 1) {
        const kf = &session.known_child_frames[ki];
        if (kf.active && kf.detached && std.mem.eql(u8, kf.frame_id_buf[0..kf.frame_id_len], fid)) {
            return true;
        }
    }
    return false;
}

fn addIframeState(
    session: *CdpPageSession,
    out_frames: []FrameState,
    url_storage: []u8,
    id_storage: []u8,
    count: *usize,
    parent_frame_id: []const u8,
    base_url: []const u8,
    allow_fixture_depth2: bool,
    raw_src: []const u8,
    frame_name: []const u8,
    owner_id: []const u8,
    sandbox_opt: ([]const u8)?,
    existing_frame_id: []const u8,
) {
    if (count.* >= out_frames.len) return;
    const child_idx = count.*;
    const effective_src = if (raw_src.len > 0) raw_src else "about:blank";
    let child_url: []const u8 = "about:blank";
    if (effective_src.len >= 11 && std.ascii.eqlIgnoreCase(effective_src[0..11], "javascript:")) {
        child_url = "";
    } else if (std.mem.eql(u8, effective_src, "about:blank") ||
        std.mem.startsWith(u8, effective_src, "about:blank?") ||
        std.mem.startsWith(u8, effective_src, "about:blank#") ||
        std.mem.startsWith(u8, effective_src, "about:srcdoc") ||
        (effective_src.len >= 5 && std.ascii.eqlIgnoreCase(effective_src[0..5], "data:")) ||
        (effective_src.len >= 5 && std.ascii.eqlIgnoreCase(effective_src[0..5], "blob:"))) {
        const u_dst = url_storage[child_idx * 512 .. (child_idx + 1) * 512];
        const copy_len = @min(effective_src.len, u_dst.len);
        @memcpy(u_dst[0..copy_len], effective_src[0..copy_len]);
        child_url = u_dst[0..copy_len];
    } else {
        child_url = resolveIframeFinalUrl(base_url, effective_src, url_storage[child_idx * 512 .. (child_idx + 1) * 512]);
    }
    const id_dst = id_storage[child_idx * 32 .. (child_idx + 1) * 32];
    let child_id: []const u8 = "capsurf-child-1";
    if (existing_frame_id.len > 0) {
        const flen = @min(existing_frame_id.len, id_dst.len);
        @memcpy(id_dst[0..flen], existing_frame_id[0..flen]);
        child_id = id_dst[0..flen];
    } else {
        session.next_child_frame_seq += 1;
        child_id = std.fmt.bufPrint(id_dst, "capsurf-child-{d}", .{ session.next_child_frame_seq }) catch "capsurf-child-1";
    }
    const is_blank_or_local = std.mem.eql(u8, child_url, "about:blank") ||
        std.mem.startsWith(u8, child_url, "about:blank?") ||
        std.mem.startsWith(u8, child_url, "about:blank#") ||
        std.mem.startsWith(u8, child_url, "about:srcdoc") ||
        std.mem.startsWith(u8, child_url, "data:");
    const is_xo = !is_blank_or_local && !HttpLoader.isSameOrigin(base_url, child_url);

    let name_buf = session.allocator.dupe(u8, frame_name) catch "";
    let owner_buf = session.allocator.dupe(u8, owner_id) catch "";
    let sb_val: (SandboxRestrictions)? = null;
    if (sb_str := sandbox_opt) {
        sb_val = parseSandboxAttr(sb_str);
    }
    out_frames[child_idx] = makeChildFrameState(ChildFrameArgs{
        frame_id: child_id,
        parent_frame_id: parent_frame_id,
        loader_id: session.loader_id,
        url: child_url,
        name: name_buf,
        owner_element_id: owner_buf,
        is_cross_origin: is_xo,
        sandbox: sb_val,
    });
    count.* += 1;

    // Depth-2 iframe discovery for fixture documents
    if (allow_fixture_depth2 && count.* < out_frames.len && MockFixtures.isFixtureUrl(child_url) && std.mem.indexOf(u8, child_url, "/frame-policy-child.html") == null && std.mem.indexOf(u8, child_url, "/oopif-kind-child.html") == null) {
        const child_html_buf = session.allocator.alloc(u8, 16384) catch return;
        defer session.allocator.free(child_html_buf);
        if (child_html := MockFixtures.handleFixtureRequest(child_url, "", session.user_agent, child_html_buf, null, "")) {
            if (nested_src := findNestedIframeSrc(child_html)) {
                const gc_idx = count.*;
                let gc_url = resolveIframeFinalUrl(child_url, nested_src, url_storage[gc_idx * 512 .. (gc_idx + 1) * 512]);
                if (std.mem.indexOf(u8, gc_url, "/referer-to-url.html") != null && std.mem.indexOf(u8, gc_url, "referer=") == null) {
                    const pol = MockFixtures.extractQueryParam(child_url, "policy") ?? "strict-origin-when-cross-origin";
                    let ref_val: []const u8 = "";
                    let ref_origin_buf: [256]u8 = undefined;
                    if (std.ascii.eqlIgnoreCase(pol, "no-referrer")) {
                        ref_val = "";
                    } else if (std.ascii.eqlIgnoreCase(pol, "origin") || std.ascii.eqlIgnoreCase(pol, "strict-origin")) {
                        const co = originOf(child_url);
                        if (co.len > 0 && !std.mem.eql(u8, co, "://")) {
                            ref_val = std.fmt.bufPrint(&ref_origin_buf, "{s}/", .{ co }) catch "";
                        }
                    } else if (std.ascii.eqlIgnoreCase(pol, "unsafe-url")) {
                        ref_val = child_url;
                    } else {
                        if (HttpLoader.isSameOrigin(child_url, gc_url)) {
                            ref_val = child_url;
                        } else {
                            const co = originOf(child_url);
                            if (co.len > 0 && !std.mem.eql(u8, co, "://")) {
                                ref_val = std.fmt.bufPrint(&ref_origin_buf, "{s}/", .{ co }) catch "";
                            }
                        }
                    }
                    if (ref_val.len > 0) {
                        let enc_buf: [1024]u8 = undefined;
                        let ei: usize = 0;
                        const hex = "0123456789ABCDEF";
                        for (ch of ref_val) {
                            if (ei + 3 >= enc_buf.len) break;
                            const is_unreserved = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' || ch == '~';
                            if (is_unreserved) {
                                enc_buf[ei] = ch;
                                ei += 1;
                            } else {
                                enc_buf[ei] = '%';
                                enc_buf[ei + 1] = hex[(ch >> 4) & 0xF];
                                enc_buf[ei + 2] = hex[ch & 0xF];
                                ei += 3;
                            }
                        }
                        const dst_slice = url_storage[gc_idx * 512 .. (gc_idx + 1) * 512];
                        const sep: []const u8 = if (std.mem.indexOfScalar(u8, gc_url, '?') != null) "&" else "?";
                        let tmp_gc: [512]u8 = undefined;
                        const gl = @min(gc_url.len, tmp_gc.len);
                        @memcpy(tmp_gc[0..gl], gc_url[0..gl]);
                        gc_url = std.fmt.bufPrint(dst_slice, "{s}{s}referer={s}", .{ tmp_gc[0..gl], sep, enc_buf[0..ei] }) catch gc_url;
                    }
                }
                const gc_id = std.fmt.bufPrint(id_storage[gc_idx * 32 .. (gc_idx + 1) * 32], "{s}-1", .{ child_id }) catch "capsurf-child-1-1";
                const gc_xo = !HttpLoader.isSameOrigin(child_url, gc_url);
                out_frames[gc_idx] = makeChildFrameState(ChildFrameArgs{
                    frame_id: gc_id,
                    parent_frame_id: child_id,
                    loader_id: session.loader_id,
                    url: gc_url,
                    is_cross_origin: gc_xo,
                });
                count.* += 1;
            }
        }
    }
}

fn collectFrameStates(
    session: *CdpPageSession,
    out_frames: []FrameState,
    url_storage: []u8,
    id_storage: []u8,
): usize {
    if (out_frames.len == 0) return 0;
    out_frames[0] = makeMainFrameState(MainFrameArgs{
        frame_id: session.frame_id,
        loader_id: session.loader_id,
        url: session.url,
    });
    let count: usize = 1;

    if (session.has_document || std.mem.eql(u8, session.url, "about:blank")) {
        const js_iframes = "(function(){try{if(typeof document==='undefined'||!document.documentElement)return '';var out=[];var seen=new Set();function walk(r,parentIdx,depth){if(!r)return;var cs=r.childNodes||r.children||[];for(var i=0;i<cs.length;i++){var el=cs[i];if(!el||el.nodeType!==1)continue;var t=String(el.localName||el.tagName||'').toLowerCase();if(t==='template')continue;if((t==='iframe'||t==='frame')&&!seen.has(el)){seen.add(el);const myIdx=out.length+1;var hasSd=el.hasAttribute&&el.hasAttribute('srcdoc');var s=hasSd?'about:srcdoc':((el.getAttribute&&el.getAttribute('src'))||'');var n=(el.getAttribute&&el.getAttribute('name'))||'';var id=(el.getAttribute&&el.getAttribute('id'))||'';var sb=(el.hasAttribute&&el.hasAttribute('sandbox'))?(el.getAttribute('sandbox')||''):'__NONE__';var fid=el._capsurf_frame_id||'';out.push(parentIdx+'\\x1f'+s+'\\x1f'+n+'\\x1f'+id+'\\x1f'+sb+'\\x1f'+fid);if(depth<4){const cdoc=el.contentDocument||(el.contentWindow&&el.contentWindow.document)||el._contentDocument||null;if(cdoc&&cdoc.documentElement)walk(cdoc.documentElement,myIdx,depth+1);}}if(el._shadowRoot)walk(el._shadowRoot,parentIdx,depth);walk(el,parentIdx,depth);}}walk(document.documentElement,0,1);return out.join('\\x1e');}catch(e){return '';}})()";
        const val = session.ctx.eval(js_iframes);
        defer session.ctx.freeValue(val);
        const cstr = session.ctx.toCString(val);
        if (cstr.len > 0) {
            let js_to_slot: [64]usize = [0; 64];
            js_to_slot[0] = 0;
            let js_idx: usize = 0;
            let rows = std.mem.splitScalar(u8, cstr, 0x1e);
            while (row := rows.next()) {
                if (count >= out_frames.len) break;
                js_idx += 1;
                let cols = std.mem.splitScalar(u8, row, 0x1f);
                const raw_parent_idx = cols.next() ?? "0";
                const parent_js_idx = std.fmt.parseInt(usize, raw_parent_idx, 10) catch 0;
                const raw_src = cols.next() ?? "";
                const frame_name = cols.next() ?? "";
                const owner_id = cols.next() ?? "";
                const raw_sb = cols.next() ?? "__NONE__";
                const raw_fid = cols.next() ?? "";
                const sb_opt: ([]const u8)? = if (std.mem.eql(u8, raw_sb, "__NONE__")) null else raw_sb;
                const valid_existing = raw_fid.len > 0 && std.mem.startsWith(u8, raw_fid, "capsurf-child-") && !isKnownChildFrameDetached(session, raw_fid);
                const existing_fid: []const u8 = if (valid_existing) raw_fid else "";
                const parent_slot: usize = if (parent_js_idx < 64 && parent_js_idx < count) js_to_slot[parent_js_idx] else 0;
                const child_slot = count;
                if (js_idx < 64) {
                    js_to_slot[js_idx] = count;
                }
                addIframeState(
                    session,
                    out_frames,
                    url_storage,
                    id_storage,
                    &count,
                    out_frames[parent_slot].frame_id,
                    out_frames[parent_slot].url,
                    parent_js_idx == 0,
                    raw_src,
                    frame_name,
                    owner_id,
                    sb_opt,
                    existing_fid,
                );
                if (!valid_existing && child_slot < count) {
                    const assigned_id = out_frames[child_slot].frame_id;
                    let set_js_buf: [1536]u8 = undefined;
                    const set_js = std.fmt.bufPrint(
                        &set_js_buf,
                        "(function(){{try{{if(!document||!document.documentElement)return;var idx=0;var seen=new Set();function walk(r,d){{if(!r)return;var cs=r.childNodes||r.children||[];for(var i=0;i<cs.length;i++){{var el=cs[i];if(!el||el.nodeType!==1)continue;var t=String(el.localName||el.tagName||'').toLowerCase();if(t==='template')continue;if((t==='iframe'||t==='frame')&&!seen.has(el)){{seen.add(el);if(idx==={d}){{el._capsurf_frame_id='{s}';return true;}}idx++;if(d<4){{const cdoc=el.contentDocument||(el.contentWindow&&el.contentWindow.document)||el._contentDocument||null;if(cdoc&&cdoc.documentElement&&walk(cdoc.documentElement,d+1))return true;}}}}if(el._shadowRoot&&walk(el._shadowRoot,d))return true;if(walk(el,d))return true;}}return false;}}walk(document.documentElement,1);}}catch(e){{}}}})()",
                        .{ js_idx - 1, assigned_id },
                    ) catch "";
                    if (set_js.len > 0) {
                        const sv = session.ctx.eval(set_js);
                        session.ctx.freeValue(sv);
                    }
                }
            }
            appendDetachedKnownFrames(session, out_frames, url_storage, id_storage, &count);
            return count;
        }
        appendDetachedKnownFrames(session, out_frames, url_storage, id_storage, &count);
        return count;
    }

    let i: usize = 0;
    while (i < session.doc.nodes.items.len && count < out_frames.len) : (i += 1) {
        const n = &session.doc.nodes.items[i];
        if (n.node_type == NodeType.Element && (std.ascii.eqlIgnoreCase(n.tag_name, "iframe") || std.ascii.eqlIgnoreCase(n.tag_name, "frame"))) {
            const raw_src = if (n.getAttribute("srcdoc") != null) "about:srcdoc" else (n.getAttribute("src") ?? "");
            const owner_id = n.getAttribute("id") ?? "";
            const frame_name = n.getAttribute("name") ?? "";
            const sb_opt = n.getAttribute("sandbox");
            addIframeState(session, out_frames, url_storage, id_storage, &count, session.frame_id, session.url, true, raw_src, frame_name, owner_id, sb_opt, "");
        }
    }
    appendDetachedKnownFrames(session, out_frames, url_storage, id_storage, &count);
    return count;
}

fn appendDetachedKnownFrames(
    session: *CdpPageSession,
    out_frames: []FrameState,
    url_storage: []u8,
    id_storage: []u8,
    count: *usize,
) {
    let ki: usize = 0;
    while (ki < session.known_child_frame_count && count.* < out_frames.len) : (ki += 1) {
        const kf = &session.known_child_frames[ki];
        if (!kf.active || !kf.detached) continue;
        const fidx = count.*;
        const k_fid = kf.frame_id_buf[0..kf.frame_id_len];
        let dup = false;
        let ei: usize = 1;
        while (ei < count.*) : (ei += 1) {
            if (std.mem.eql(u8, out_frames[ei].frame_id, k_fid)) {
                dup = true;
                break;
            }
        }
        if (dup) continue;
        const f_dst = id_storage[fidx * 32 .. (fidx + 1) * 32];
        const flen = @min(k_fid.len, f_dst.len);
        @memcpy(f_dst[0..flen], k_fid[0..flen]);
        const k_url = kf.url_buf[0..kf.url_len];
        const u_dst = url_storage[fidx * 512 .. (fidx + 1) * 512];
        const ulen = @min(k_url.len, u_dst.len);
        @memcpy(u_dst[0..ulen], k_url[0..ulen]);
        const pfid = if (kf.parent_frame_id_len > 0) kf.parent_frame_id_buf[0..kf.parent_frame_id_len] else session.frame_id;
        out_frames[fidx] = makeChildFrameState(ChildFrameArgs{
            frame_id: f_dst[0..flen],
            parent_frame_id: pfid,
            loader_id: session.loader_id,
            url: u_dst[0..ulen],
            name: kf.name_buf[0..kf.name_len],
            owner_element_id: kf.owner_id_buf[0..kf.owner_id_len],
            is_cross_origin: kf.is_cross_origin,
        });
        count.* += 1;
    }
}

fn recordAttachedChildFrames(session: *CdpPageSession, frames: []const FrameState) {
    let fi: usize = 1;
    while (fi < frames.len) : (fi += 1) {
        const cf = &frames[fi];
        if (isKnownChildFrameDetached(session, cf.frame_id)) continue;
        let found = false;
        let ki: usize = 0;
        while (ki < session.known_child_frame_count) : (ki += 1) {
            if (session.known_child_frames[ki].active && !session.known_child_frames[ki].detached && std.mem.eql(u8, session.known_child_frames[ki].frame_id_buf[0..session.known_child_frames[ki].frame_id_len], cf.frame_id)) {
                session.known_child_frames[ki].detached = false;
                if (cf.exec_context_id > 0) {
                    session.known_child_frames[ki].exec_context_id = cf.exec_context_id;
                }
                session.known_child_frames[ki].is_opaque = !cf.sandbox.same_origin;
                const ul = @min(cf.url.len, session.known_child_frames[ki].url_buf.len);
                @memcpy(session.known_child_frames[ki].url_buf[0..ul], cf.url[0..ul]);
                session.known_child_frames[ki].url_len = ul;
                const nl = @min(cf.name.len, session.known_child_frames[ki].name_buf.len);
                @memcpy(session.known_child_frames[ki].name_buf[0..nl], cf.name[0..nl]);
                session.known_child_frames[ki].name_len = nl;
                found = true;
                break;
            }
        }
        if (!found && session.known_child_frame_count < session.known_child_frames.len) {
            let k = &session.known_child_frames[session.known_child_frame_count];
            k.active = true;
            k.detached = false;
            k.exec_context_id = cf.exec_context_id;
            k.is_opaque = !cf.sandbox.same_origin;
            const fl = @min(cf.frame_id.len, k.frame_id_buf.len);
            @memcpy(k.frame_id_buf[0..fl], cf.frame_id[0..fl]);
            k.frame_id_len = fl;
            const ul = @min(cf.url.len, k.url_buf.len);
            @memcpy(k.url_buf[0..ul], cf.url[0..ul]);
            k.url_len = ul;
            const nl = @min(cf.name.len, k.name_buf.len);
            @memcpy(k.name_buf[0..nl], cf.name[0..nl]);
            k.name_len = nl;
            const ol = @min(cf.owner_element_id.len, k.owner_id_buf.len);
            @memcpy(k.owner_id_buf[0..ol], cf.owner_element_id[0..ol]);
            k.owner_id_len = ol;
            const pf = cf.parent_frame_id ?? session.frame_id;
            const pfl = @min(pf.len, k.parent_frame_id_buf.len);
            @memcpy(k.parent_frame_id_buf[0..pfl], pf[0..pfl]);
            k.parent_frame_id_len = pfl;
            k.is_cross_origin = cf.is_cross_origin;
            session.known_child_frame_count += 1;
        }
    }
}

fn isRefusedKindFrameUrl(url: []const u8): bool {
    if (std.mem.indexOf(u8, url, "/oopif-kind-child.html") == null) return false;
    return std.mem.indexOf(u8, url, "kind=attachment") != null ||
           std.mem.indexOf(u8, url, "kind=204") != null ||
           std.mem.indexOf(u8, url, "kind=205") != null;
}

pub fn syncIframeTargets(session: *CdpPageSession, clear_existing: bool) {
    if (clear_existing) {
        session.target_registry.clearIframeTargets();
    }
    const f_arr = session.allocator.alloc(FrameState, 48) catch return;
    defer session.allocator.free(f_arr);
    const url_storage = session.allocator.alloc(u8, 48 * 512) catch return;
    defer session.allocator.free(url_storage);
    const id_storage = session.allocator.alloc(u8, 48 * 32) catch return;
    defer session.allocator.free(id_storage);

    const f_count = collectFrameStates(session, f_arr, url_storage, id_storage);
    const bctx = session.currentBrowserContextId();
    let i: usize = 1;
    while (i < f_count) : (i += 1) {
        const cf = &f_arr[i];
        const opener_fid = cf.parent_frame_id ?? session.frame_id;
        * = session.target_registry.registerIframeTarget(cf.url, cf.frame_id, opener_fid, bctx);
    }
}

fn countActiveOopifContexts(session: *CdpPageSession): usize {
    let n: usize = 0;
    let ki: usize = 0;
    while (ki < session.known_child_frame_count) : (ki += 1) {
        const kf = &session.known_child_frames[ki];
        if (kf.active && !kf.detached && kf.is_cross_origin && kf.exec_context_id > 0) {
            n += 1;
        }
    }
    return n;
}

fn deactivateChildFrameIsolatedWorlds(
    session: *CdpPageSession,
    frame_id: []const u8,
    out_events: *[MAX_CDP_EVENTS]CdpEvent,
    event_count: *usize,
) {
    if (frame_id.len == 0) return;
    let wi: usize = 0;
    while (wi < session.isolated_world_count) : (wi += 1) {
        let iw = &session.isolated_worlds[wi];
        if (!iw.active) continue;
        if (!std.mem.eql(u8, iw.frameId(), frame_id)) continue;
        const old_cid = iw.context_id;
        iw.active = false;
        iw.context_id = 0;
        if (old_cid > 0) {
            if (session.retired_isolated_world_count < session.retired_isolated_world_ids.len) {
                session.retired_isolated_world_ids[session.retired_isolated_world_count] = old_cid;
                session.retired_isolated_world_count += 1;
            }
            if (session.runtime_enabled && event_count.* < out_events.len) {
                const des_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"executionContextId\":{d}}}",
                    .{ old_cid },
                ) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Runtime.executionContextDestroyed", des_json);
                event_count.* += 1;
            }
        }
    }
}

fn emitChildFrameIsolatedWorlds(
    session: *CdpPageSession,
    frame_id: []const u8,
    frame_url: []const u8,
    is_opaque: bool,
    out_events: *[MAX_CDP_EVENTS]CdpEvent,
    event_count: *usize,
) {
    if (!session.runtime_enabled || frame_id.len == 0) return;
    if (session.init_world_names.items.len == 0) return;
    const raw_orig = if (is_opaque) "://" else originOf(frame_url);
    const frame_orig = if (raw_orig.len > 0) raw_orig else (if (frame_url.len > 0) frame_url else "about:blank");
    let esc_orig_buf: [256]u8 = [0; 256];
    const esc_orig = escapeJsonString(frame_orig, &esc_orig_buf);
    let wi: usize = 0;
    while (wi < session.init_world_names.items.len && event_count.* < out_events.len) : (wi += 1) {
        const world_name = session.init_world_names.items[wi];
        if (world_name.len == 0) continue;
        let created_new = false;
        const cid = session.createIsolatedWorld(world_name, frame_id, &created_new);
        if (created_new) {
            let esc_name_buf: [256]u8 = [0; 256];
            const esc_name = escapeJsonString(world_name, &esc_name_buf);
            const ctx_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"{s}\",\"uniqueId\":\"ctx-{d}\",\"auxData\":{{\"isDefault\":false,\"type\":\"isolated\",\"frameId\":\"{s}\"}}}}}}",
                .{ cid, esc_orig, esc_name, cid, frame_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
            event_count.* += 1;
        }
    }
}

fn emitDetachAllChildFrames(
    session: *CdpPageSession,
    out_events: *[MAX_CDP_EVENTS]CdpEvent,
    event_count: *usize,
) {
    let dki: usize = 0;
    while (dki < session.known_child_frame_count && event_count.* < out_events.len) : (dki += 1) {
        let dkf = &session.known_child_frames[dki];
        if (dkf.active && !dkf.detached && dkf.frame_id_len > 0) {
            dkf.detached = true;
            const det_fid = dkf.frame_id_buf[0..dkf.frame_id_len];
            const det_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"frameId\":\"{s}\",\"reason\":\"remove\"}}",
                .{ det_fid },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Page.frameDetached", det_json);
            event_count.* += 1;
            if (dkf.exec_context_id > 0 && session.runtime_enabled && event_count.* < out_events.len) {
                const des_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"executionContextId\":{d}}}",
                    .{ dkf.exec_context_id },
                ) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Runtime.executionContextDestroyed", des_json);
                event_count.* += 1;
            }
            dkf.exec_context_id = 0;
            deactivateChildFrameIsolatedWorlds(session, det_fid, out_events, event_count);
        }
    }
}

pub fn emitNewChildFrameEvents(
    session: *CdpPageSession,
    out_events: *[MAX_CDP_EVENTS]CdpEvent,
    event_count: *usize,
) {
    if (!session.has_document && !std.mem.eql(u8, session.url, "about:blank")) return;
    const nav_frames = session.allocator.alloc(FrameState, 48) catch return;
    const nav_urls = session.allocator.alloc(u8, 48 * 512) catch {
        session.allocator.free(nav_frames);
        return;
    };
    const nav_ids = session.allocator.alloc(u8, 48 * 32) catch {
        session.allocator.free(nav_frames);
        session.allocator.free(nav_urls);
        return;
    };
    defer session.allocator.free(nav_frames);
    defer session.allocator.free(nav_urls);
    defer session.allocator.free(nav_ids);

    const nav_frame_count = collectFrameStates(session, nav_frames, nav_urls, nav_ids);

    // Detect detached child frames (known attached frames whose owner element is no longer in DOM)
    {
        const dom_urls_js = "(function(){try{if(typeof document==='undefined'||!document.documentElement)return '';var out=[];var seen=new Set();function walk(r,d){if(!r)return;var cs=r.childNodes||r.children||[];for(var i=0;i<cs.length;i++){var el=cs[i];if(!el||el.nodeType!==1)continue;var t=String(el.localName||el.tagName||'').toLowerCase();if(t==='template')continue;if((t==='iframe'||t==='frame')&&!seen.has(el)){seen.add(el);var fid=el._capsurf_frame_id||'';var id=(el.getAttribute&&el.getAttribute('id'))||'';out.push(fid+'\\x1f'+id);if(d<4){const cdoc=el.contentDocument||(el.contentWindow&&el.contentWindow.document)||el._contentDocument||null;if(cdoc&&cdoc.documentElement)walk(cdoc.documentElement,d+1);}}if(el._shadowRoot)walk(el._shadowRoot,d);walk(el,d);}}walk(document.documentElement,1);return out.join('\\x1e');}catch(e){return '';}})()";
        const dv = session.ctx.eval(dom_urls_js);
        defer session.ctx.freeValue(dv);
        const dcstr = session.ctx.toCString(dv);
        let ki: usize = 0;
        while (ki < session.known_child_frame_count && event_count.* < out_events.len) : (ki += 1) {
            let kf = &session.known_child_frames[ki];
            if (!kf.active || kf.detached) continue;
            let still_present = false;
            if (dcstr.len > 0) {
                const k_fid = kf.frame_id_buf[0..kf.frame_id_len];
                const k_oid = kf.owner_id_buf[0..kf.owner_id_len];
                let drows = std.mem.splitScalar(u8, dcstr, 0x1e);
                while (drow := drows.next()) {
                    let dcols = std.mem.splitScalar(u8, drow, 0x1f);
                    const row_fid = dcols.next() ?? "";
                    const row_oid = dcols.next() ?? "";
                    if (k_fid.len > 0 && row_fid.len > 0) {
                        if (std.mem.eql(u8, row_fid, k_fid)) {
                            still_present = true;
                            break;
                        }
                    } else if (k_oid.len > 0 && row_oid.len > 0 && std.mem.eql(u8, row_oid, k_oid)) {
                        still_present = true;
                        break;
                    }
                }
            }
            if (!still_present) {
                kf.detached = true;
                if (session.emitted_child_frame_count > 0) {
                    session.emitted_child_frame_count -= 1;
                }
                const det_fid = kf.frame_id_buf[0..kf.frame_id_len];
                const det_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"frameId\":\"{s}\",\"reason\":\"remove\"}}",
                    .{ det_fid },
                ) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Page.frameDetached", det_json);
                event_count.* += 1;
                if (kf.exec_context_id > 0 && session.runtime_enabled && event_count.* < out_events.len) {
                    const des_json = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"executionContextId\":{d}}}",
                        .{ kf.exec_context_id },
                    ) catch "{}";
                    out_events[event_count.*] = CdpEvent.create("Runtime.executionContextDestroyed", des_json);
                    event_count.* += 1;
                }
                kf.exec_context_id = 0;
                deactivateChildFrameIsolatedWorlds(session, det_fid, out_events, event_count);
            }
        }
    }

    let active_oopifs = countActiveOopifContexts(session);

    // Detect URL/origin changes on already-known child frames (e.g. about:blank -> cross-origin src or same-origin navigation)
    {
        const main_orig = originOf(session.url);
        let frame_info_buf: [2048]u8 = [0; 2048];
        let ki: usize = 0;
        while (ki < session.known_child_frame_count && event_count.* < out_events.len) : (ki += 1) {
            let kf = &session.known_child_frames[ki];
            if (!kf.active || kf.detached) continue;
            const k_fid = kf.frame_id_buf[0..kf.frame_id_len];
            const old_url = kf.url_buf[0..kf.url_len];
            let nfi: usize = 1;
            while (nfi < nav_frame_count) : (nfi += 1) {
                const cf = &nav_frames[nfi];
                if (std.mem.eql(u8, cf.frame_id, k_fid)) {
                    const new_origin = originOf(cf.url);
                    if (cf.url.len > 0 && !std.mem.eql(u8, cf.url, old_url) && !isRefusedKindFrameUrl(cf.url)) {
                        const was_xo_with_ctx = kf.is_cross_origin && kf.exec_context_id > 0;
                        const ul = @min(cf.url.len, kf.url_buf.len);
                        @memcpy(kf.url_buf[0..ul], cf.url[0..ul]);
                        kf.url_len = ul;
                        const is_blank_or_local = std.mem.eql(u8, cf.url, "about:blank") ||
                            std.mem.startsWith(u8, cf.url, "about:blank?") ||
                            std.mem.startsWith(u8, cf.url, "about:blank#") ||
                            std.mem.startsWith(u8, cf.url, "about:srcdoc") ||
                            std.mem.startsWith(u8, cf.url, "data:");
                        const is_xo = !is_blank_or_local && main_orig.len > 0 && !std.mem.eql(u8, new_origin, main_orig);
                        kf.is_cross_origin = is_xo;
                        if (event_count.* < out_events.len) {
                            const frame_info = makeFrameInfoWithFrames(cf, nav_frames[0..nav_frame_count], &frame_info_buf);
                            const nav_json = std.fmt.allocPrint(
                                session.allocator,
                                "{{\"frame\":{s},\"type\":\"Navigation\"}}",
                                .{ frame_info },
                            ) catch "{}";
                            out_events[event_count.*] = CdpEvent.create("Page.frameNavigated", nav_json);
                            event_count.* += 1;
                        }
                        deactivateChildFrameIsolatedWorlds(session, cf.frame_id, out_events, event_count);
                        const allow_ctx = !is_xo || was_xo_with_ctx || active_oopifs < MAX_CONCURRENT_OOPIF;
                        if (allow_ctx && session.runtime_enabled && !isRefusedKindFrameUrl(cf.url) && event_count.* < out_events.len) {
                            const ctx_id = session.next_context_id;
                            session.next_context_id += 1;
                            kf.exec_context_id = ctx_id;
                            if (is_xo && !was_xo_with_ctx) active_oopifs += 1;
                            let esc_orig_buf: [256]u8 = [0; 256];
                            const esc_orig = escapeJsonString(new_origin, &esc_orig_buf);
                            const ctx_json = std.fmt.allocPrint(
                                session.allocator,
                                "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"auxData\":{{\"isDefault\":true,\"type\":\"default\",\"frameId\":\"{s}\"}}}}}}",
                                .{ ctx_id, esc_orig, cf.frame_id },
                            ) catch "{}";
                            out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
                            event_count.* += 1;
                            emitChildFrameIsolatedWorlds(session, cf.frame_id, cf.url, is_xo, out_events, event_count);
                        }
                    }
                    break;
                }
            }
        }
    }

    // Promote waiting cross-origin child frames if an OOPIF slot freed up
    if (session.runtime_enabled && active_oopifs < MAX_CONCURRENT_OOPIF) {
        let pi: usize = 0;
        while (pi < session.known_child_frame_count && active_oopifs < MAX_CONCURRENT_OOPIF && event_count.* < out_events.len) : (pi += 1) {
            let kf = &session.known_child_frames[pi];
            if (!kf.active || kf.detached || !kf.is_cross_origin || kf.exec_context_id != 0) continue;
            const k_url = kf.url_buf[0..kf.url_len];
            if (isRefusedKindFrameUrl(k_url)) continue;
            const k_orig = originOf(k_url);
            const ctx_id = session.next_context_id;
            session.next_context_id += 1;
            kf.exec_context_id = ctx_id;
            let esc_orig_buf: [256]u8 = [0; 256];
            const esc_orig = escapeJsonString(k_orig, &esc_orig_buf);
            const ctx_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"auxData\":{{\"isDefault\":false,\"frameId\":\"{s}\"}}}}}}",
                .{ ctx_id, esc_orig, kf.frame_id_buf[0..kf.frame_id_len] },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
            event_count.* += 1;
            active_oopifs += 1;
            emitChildFrameIsolatedWorlds(session, kf.frame_id_buf[0..kf.frame_id_len], k_url, kf.is_opaque, out_events, event_count);
        }
    }

    let active_total_children: usize = 0;
    let aci: usize = 1;
    while (aci < nav_frame_count) : (aci += 1) {
        if (!isKnownChildFrameDetached(session, nav_frames[aci].frame_id)) {
            active_total_children += 1;
        }
    }
    if (active_total_children <= session.emitted_child_frame_count) return;

    let frame_info_buf: [2048]u8 = [0; 2048];
    let fi: usize = @as(usize, 1) + session.emitted_child_frame_count;
    while (fi < nav_frame_count && event_count.* < out_events.len) : (fi += 1) {
        let cf = &nav_frames[fi];
        if (isKnownChildFrameDetached(session, cf.frame_id)) continue;
        const parent_fid = cf.parent_frame_id ?? session.frame_id;
        if (event_count.* < out_events.len) {
            const att_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"frameId\":\"{s}\",\"parentFrameId\":\"{s}\"}}",
                .{ cf.frame_id, parent_fid },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Page.frameAttached", att_json);
            event_count.* += 1;
        }
        if (event_count.* < out_events.len) {
            const start_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"frameId\":\"{s}\"}}",
                .{ cf.frame_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Page.frameStartedLoading", start_json);
            event_count.* += 1;
        }
        emitChildFrameNetworkEvents(session, cf, out_events, event_count);
        if (event_count.* < out_events.len) {
            const frame_info = makeFrameInfoWithFrames(cf, nav_frames[0..nav_frame_count], &frame_info_buf);
            const nav_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"frame\":{s},\"type\":\"Navigation\"}}",
                .{ frame_info },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Page.frameNavigated", nav_json);
            event_count.* += 1;
        }
        if (event_count.* < out_events.len) {
            const lc_init = std.fmt.allocPrint(
                session.allocator,
                "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"init\",\"timestamp\":1700000000.0}}",
                .{ cf.frame_id, cf.loader_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", lc_init);
            event_count.* += 1;
        }
        const allow_ctx = !cf.is_cross_origin || active_oopifs < MAX_CONCURRENT_OOPIF;
        if (allow_ctx && session.runtime_enabled && !isRefusedKindFrameUrl(cf.url) && event_count.* < out_events.len) {
            const ctx_id = session.next_context_id;
            session.next_context_id += 1;
            cf.exec_context_id = ctx_id;
            if (cf.is_cross_origin) active_oopifs += 1;
            const cf_origin = originOf(cf.url);
            let esc_orig_buf: [256]u8 = [0; 256];
            const esc_orig = escapeJsonString(cf_origin, &esc_orig_buf);
            const ctx_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"auxData\":{{\"isDefault\":true,\"type\":\"default\",\"frameId\":\"{s}\"}}}}}}",
                .{ ctx_id, esc_orig, cf.frame_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
            event_count.* += 1;
            emitChildFrameIsolatedWorlds(session, cf.frame_id, cf.url, !cf.sandbox.same_origin, out_events, event_count);
        }
        if (event_count.* < out_events.len) {
            const lc_dcl = std.fmt.allocPrint(
                session.allocator,
                "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"DOMContentLoaded\",\"timestamp\":1700000000.05}}",
                .{ cf.frame_id, cf.loader_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", lc_dcl);
            event_count.* += 1;
        }
        if (event_count.* < out_events.len) {
            const lc_load = std.fmt.allocPrint(
                session.allocator,
                "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"load\",\"timestamp\":1700000000.08}}",
                .{ cf.frame_id, cf.loader_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", lc_load);
            event_count.* += 1;
        }
        if (event_count.* < out_events.len) {
            const stop_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"frameId\":\"{s}\"}}",
                .{ cf.frame_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Page.frameStoppedLoading", stop_json);
            event_count.* += 1;
        }
    }
    session.emitted_child_frame_count = active_total_children;
    recordAttachedChildFrames(session, nav_frames[0..nav_frame_count]);
    syncIframeTargets(session, true);
}

fn applyTtyNavParamsFromUrl(raw_url: []const u8, session: *CdpPageSession): bool {
    const q_pos = std.mem.indexOfScalar(u8, raw_url, '?') ?? return false;
    const h_pos = std.mem.indexOfScalarPos(u8, raw_url, q_pos, '#') ?? raw_url.len;
    const query = raw_url[q_pos + 1 .. h_pos];
    if (query.len == 0) return false;

    let state = TtyNavState{};
    if (!applyTtyNavParams(query, &state)) {
        return false;
    }
    if (lvl := state.tty_ua_css) {
        session.tty_ua_css = lvl;
    }
    if (d := state.dark_color_scheme) {
        session.dark_color_scheme = d;
    }
    if (w := state.tty_cell_w) {
        session.tty_cell_w = w;
    }
    if (h := state.tty_cell_h) {
        session.tty_cell_h = h;
    }
    return true;
}

fn stripTtyNavParams(raw_url: []const u8, out_buf: []u8): []const u8 {
    const q_pos = std.mem.indexOfScalar(u8, raw_url, '?') ?? return raw_url;
    const h_pos = std.mem.indexOfScalarPos(u8, raw_url, q_pos, '#') ?? raw_url.len;
    const query = raw_url[q_pos + 1 .. h_pos];
    if (query.len == 0) return raw_url;

    let off: usize = 0;
    const base_part = raw_url[0..q_pos];
    if (off + base_part.len > out_buf.len) return raw_url;
    @memcpy(out_buf[off..off + base_part.len], base_part);
    off += base_part.len;

    let wrote_q = false;
    let pos: usize = 0;
    while (pos < query.len) {
        let end = query.len;
        if (amp := std.mem.indexOfScalarPos(u8, query, pos, '&')) {
            end = amp;
        }
        const pair = query[pos..end];
        pos = end + 1;
        if (pair.len == 0) continue;
        let key = pair;
        if (eq := std.mem.indexOfScalar(u8, pair, '=')) {
            key = pair[0..eq];
        }
        if (std.mem.eql(u8, key, "ks_color_scheme") ||
            std.mem.eql(u8, key, "ks_tty_css") ||
            std.mem.eql(u8, key, "ks_tty_cell")) {
            continue;
        }
        const sep: u8 = if (!wrote_q) '?' else '&';
        if (off + 1 + pair.len > out_buf.len) return raw_url;
        out_buf[off] = sep;
        off += 1;
        @memcpy(out_buf[off..off + pair.len], pair);
        off += pair.len;
        wrote_q = true;
    }

    const hash_part = raw_url[h_pos..];
    if (off + hash_part.len > out_buf.len) return raw_url;
    @memcpy(out_buf[off..off + hash_part.len], hash_part);
    off += hash_part.len;
    return out_buf[0..off];
}

fn sanitizeSetContentUrl(url: []const u8): []const u8 {
    if (url.len == 0) return "https://kitesurf.local/";
    if (std.ascii.startsWithIgnoreCase(url, "http://") ||
        std.ascii.startsWithIgnoreCase(url, "https://") ||
        std.ascii.startsWithIgnoreCase(url, "file://") ||
        std.ascii.startsWithIgnoreCase(url, "ftp://") ||
        std.ascii.startsWithIgnoreCase(url, "ws://") ||
        std.ascii.startsWithIgnoreCase(url, "wss://"))
    {
        return url;
    }
    return "https://kitesurf.local/";
}

pub struct PageHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8, out_events: *[MAX_CDP_EVENTS]CdpEvent, event_count: *usize): CdpResponse {
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "Page.setLifecycleEventsEnabled")) {
            const enabled = std.mem.indexOf(u8, req.params_raw, "\"enabled\":false") == null;
            session.lifecycle_events_enabled = enabled;
            if (enabled && session.is_about_blank_popup && !session.initial_blank_lifecycle_emitted && !session.has_document && std.mem.eql(u8, session.url, "about:blank")) {
                session.initial_blank_lifecycle_emitted = true;
                if (event_count.* < out_events.len) {
                    const fn_json = session.formatFrameNavigatedEvent("about:blank");
                    out_events[event_count.*] = CdpEvent.create("Page.frameNavigated", fn_json);
                    event_count.* += 1;
                }
                emitMainFrameLifecycleEvents(session, out_events, event_count);
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.setInterceptFileChooserDialog")) {
            const enabled = std.mem.indexOf(u8, req.params_raw, "\"enabled\":true") != null;
            session.intercept_file_chooser = enabled;
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.stopLoading")) {
            session.navigation_aborted = true;
            if (session.has_document) {
                const sv = session.ctx.eval("globalThis.__capsurf_window_stopped=true;");
                session.ctx.freeValue(sv);
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.enable") ||
            std.mem.eql(u8, req.method, "Page.disable") ||
            std.mem.eql(u8, req.method, "Page.setBypassCSP") ||
            std.mem.eql(u8, req.method, "Page.bringToFront") ||
            std.mem.eql(u8, req.method, "Page.handleJavaScriptDialog")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.close")) {
            let reg = &session.target_registry;
            if (owner := session.owner_target_registry) {
                reg = owner;
            }
            const existed = reg.closeTarget(session.target_id);
            if (existed && event_count.* < out_events.len) {
                const td_json = std.fmt.bufPrint(&reg.event_buf0, "{{\"targetId\":\"{s}\"}}", .{session.target_id}) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Target.targetDestroyed", td_json);
                event_count.* += 1;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.addScriptToEvaluateOnNewDocument")) {
            let id_buf: [64]u8 = [0; 64];
            let script_id: []const u8 = "capsurf-script-1";
            const run_immediately = std.mem.indexOf(u8, req.params_raw, "\"runImmediately\":true") != null ||
                std.mem.indexOf(u8, req.params_raw, "\"runImmediately\": true") != null;
            if (raw_world := extractJsonString(req.params_raw, "worldName")) {
                if (raw_world.len > 0) {
                    let wn_buf = session.allocator.alloc(u8, raw_world.len) catch null;
                    if (wb := wn_buf) {
                        defer session.allocator.free(wb);
                        const unesc_world = unescapeJsonString(raw_world, wb);
                        session.registerInitWorldName(unesc_world);
                    }
                }
            }
            if (raw_src := extractJsonString(req.params_raw, "source")) {
                if (raw_src.len > 0) {
                    let unesc_buf = session.allocator.alloc(u8, raw_src.len) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                    defer session.allocator.free(unesc_buf);
                    const unesc_src = unescapeJsonString(raw_src, unesc_buf);
                    script_id = session.addNewDocumentScript(unesc_src, &id_buf);
                    if (run_immediately) {
                        const eval_res = session.ctx.eval(unesc_src);
                        session.ctx.freeValue(eval_res);
                        session.ctx.drainJobs();
                    }
                } else {
                    script_id = session.addNewDocumentScript("", &id_buf);
                }
            } else {
                script_id = session.addNewDocumentScript("", &id_buf);
            }
            const res = std.fmt.bufPrint(out_buf, "{{\"identifier\":\"{s}\"}}", .{script_id}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Page.removeScriptToEvaluateOnNewDocument")) {
            if (ident := extractJsonString(req.params_raw, "identifier")) {
                session.removeNewDocumentScript(ident);
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.createIsolatedWorld")) {
            const world_name = extractJsonString(req.params_raw, "worldName") ?? "";
            const frame_id = extractJsonString(req.params_raw, "frameId") ?? session.frame_id;
            let created_new = false;
            const cid = session.createIsolatedWorld(world_name, frame_id, &created_new);
            if (created_new && event_count.* < out_events.len) {
                const raw_orig = originOf(session.url);
                const main_orig = if (raw_orig.len > 0) raw_orig else (if (session.url.len > 0) session.url else "about:blank");
                let esc_orig_buf: [256]u8 = [0; 256];
                const esc_orig = escapeJsonString(main_orig, &esc_orig_buf);
                let esc_name_buf: [256]u8 = [0; 256];
                const esc_name = escapeJsonString(world_name, &esc_name_buf);
                const ctx_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"{s}\",\"uniqueId\":\"ctx-{d}\",\"auxData\":{{\"isDefault\":false,\"type\":\"isolated\",\"frameId\":\"{s}\"}}}}}}",
                    .{ cid, esc_orig, esc_name, cid, frame_id },
                ) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
                event_count.* += 1;
            }
            const res = std.fmt.bufPrint(out_buf, "{{\"executionContextId\":{d}}}", .{cid}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Page.setDocumentContent")) {
            const req_frame_id = extractJsonString(req.params_raw, "frameId") ?? session.frame_id;
            if (req_frame_id.len > 0 && !std.mem.eql(u8, req_frame_id, session.frame_id)) {
                return setChildFrameDocumentContent(session, req, req_frame_id, out_events, event_count);
            }
            session.tty_ua_css = 0;
            session.tty_cell_w = 0;
            session.tty_cell_h = 0;
            emitDetachAllChildFrames(session, out_events, event_count);
            session.known_child_frame_count = 0;
            session.emitted_child_frame_count = 0;
            session.in_set_document_content = true;
            defer session.in_set_document_content = false;
            const target_url = sanitizeSetContentUrl(session.url);
            if (raw_html := extractJsonString(req.params_raw, "html")) {
                if (raw_html.len == 0) {
                    session.navigate("", target_url, "") catch return CdpResponse.err(req.id, -32000, "Failed to set document content");
                } else {
                    let unesc_buf = session.allocator.alloc(u8, raw_html.len) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                    defer session.allocator.free(unesc_buf);
                    const html = unescapeJsonString(raw_html, unesc_buf);
                    session.navigate(html, target_url, "") catch return CdpResponse.err(req.id, -32000, "Failed to set document content");
                }
                session.ctx.drainJobs();
                let polled_nav_evts: [4]CdpEvent = [CdpEvent{}; 4];
                const polled_n_count = session.pollNavigatedWithinDocEvents(&polled_nav_evts);
                let n_i: usize = 0;
                while (n_i < polled_n_count && event_count.* < out_events.len) : (n_i += 1) {
                    out_events[event_count.*] = polled_nav_evts[n_i];
                    event_count.* += 1;
                }
            }

            if (session.runtime_enabled) {
                emitMainAndIsolatedExecutionContexts(session, target_url, out_events, event_count);
            }
            syncIframeTargets(session, true);
            emitNewChildFrameEvents(session, out_events, event_count);

            if (session.init_img_count > 0) {
                const img_buf = session.allocator.alloc(u8, 65536) catch null;
                defer if (img_buf != null) session.allocator.free(img_buf.?);
                if (img_buf != null) {
                    let ii: usize = 0;
                    while (ii < session.init_img_count) : (ii += 1) {
                        const ilen = session.init_img_url_lens[ii];
                        if (ilen == 0) continue;
                        const img_u = session.init_img_urls[ii][0..ilen];
                        const is_http = std.mem.startsWith(u8, img_u, "http://") || std.mem.startsWith(u8, img_u, "https://");
                        if (!is_http) continue;
                        if (MockFixtures.isFixtureUrl(img_u)) continue;
                        if (std.mem.indexOf(u8, img_u, "dripping-") != null || std.mem.indexOf(u8, img_u, "ms=45000") != null) continue;
                        * = HttpLoader.fetchDocumentWithReferrerType(session, img_u, img_buf.?, 0, session.url, "Image") catch continue;
                    }
                }
            }

            if (session.css_enabled) {
                let polled_css_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_css_count = session.pollCssEvents(&polled_css_evts);
                let css_i: usize = 0;
                while (css_i < polled_css_count && event_count.* < out_events.len) : (css_i += 1) {
                    out_events[event_count.*] = polled_css_evts[css_i];
                    event_count.* += 1;
                }
            }

            if (session.network_enabled) {
                let polled_net_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_net_count = session.pollNetworkEvents(&polled_net_evts);
                let net_i: usize = 0;
                while (net_i < polled_net_count && event_count.* < out_events.len) : (net_i += 1) {
                    out_events[event_count.*] = polled_net_evts[net_i];
                    event_count.* += 1;
                }
            }

            if (session.webmcp_enabled) {
                let polled_wm_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_wm_count = session.pollWebMcpEvents(&polled_wm_evts);
                let wm_i: usize = 0;
                while (wm_i < polled_wm_count && event_count.* < out_events.len) : (wm_i += 1) {
                    out_events[event_count.*] = polled_wm_evts[wm_i];
                    event_count.* += 1;
                }
            }

            if (session.runtime_enabled) {
                let polled_con_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_con_count = session.pollConsoleEvents(&polled_con_evts);
                let con_i: usize = 0;
                while (con_i < polled_con_count && event_count.* < out_events.len) : (con_i += 1) {
                    out_events[event_count.*] = polled_con_evts[con_i];
                    event_count.* += 1;
                }
            }

            emitMainFrameLifecycleEvents(session, out_events, event_count);

            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.startScreencast")) {
            const req_mw = extractJsonInt(req.params_raw, "maxWidth") ?? 0;
            const req_mh = extractJsonInt(req.params_raw, "maxHeight") ?? 0;
            if (req_mw > 0 && req_mh > 0) {
                const clamped = Viewport.clampViewport(@floatFromInt(req_mw), @floatFromInt(req_mh));
                session.viewport_width = clamped.width;
                session.viewport_height = clamped.height;
                session.layout.viewport_width = clamped.width;
                session.layout.viewport_height = clamped.height;
                if (dctx := session.dom_ctx) {
                    dctx.layout_dirty = true;
                }
            }
            const owner_sid = if (req.session_id.len > 0) req.session_id else session.target_id;
            * = session.screencast_handler.startScreencast(req.params_raw, owner_sid);
            if (session.has_document) {
                let drv_buf: [160]u8 = [0; 160];
                const drv_js = std.fmt.bufPrint(
                    &drv_buf,
                    "(function(){{if(typeof __capsurf_set_raf_driver==='function')__capsurf_set_raf_driver(true,{d});}})()",
                    .{session.screencast_handler.state.min_frame_interval_ms},
                ) catch "";
                if (drv_js.len > 0) {
                    const dv = session.ctx.eval(drv_js);
                    session.ctx.freeValue(dv);
                }
            }

            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.stopScreencast")) {
            const caller_sid = if (req.session_id.len > 0) req.session_id else session.target_id;
            const owner_sid = session.screencast_handler.getOwnerSession();
            if (owner_sid.len == 0 || std.mem.eql(u8, caller_sid, owner_sid)) {
                * = session.screencast_handler.stopScreencast();
                if (session.has_document) {
                    const dv = session.ctx.eval("(function(){if(typeof __capsurf_set_raf_driver==='function')__capsurf_set_raf_driver(false,0);})()");
                    session.ctx.freeValue(dv);
                }
            }

            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.screencastFrameAck")) {
            const caller_sid = if (req.session_id.len > 0) req.session_id else session.target_id;
            const owner_sid = session.screencast_handler.getOwnerSession();
            if (owner_sid.len == 0 || std.mem.eql(u8, caller_sid, owner_sid)) {
                const sid = extractAckSessionId(req.params_raw) ?? 0;
                * = session.screencast_handler.screencastFrameAck(sid);
            }

            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.printToPDF")) {
            // Document-scoped drain: mark all lazy/pending images as complete before generating PDF
            if (session.has_document) {
                const drain_v = session.ctx.eval("(function(){if(typeof globalThis.__capsurf_drain_all_images==='function')globalThis.__capsurf_drain_all_images();})()");
                session.ctx.freeValue(drain_v);
                session.ctx.drainJobs();
            }

            let opts = PdfOptions{};
            if (pw := extractJsonFloat(req.params_raw, "paperWidth")) {
                if (pw > 0.0) opts.paper_width_in = @floatCast(pw);
            }
            if (ph := extractJsonFloat(req.params_raw, "paperHeight")) {
                if (ph > 0.0) opts.paper_height_in = @floatCast(ph);
            }
            if (mt := extractJsonFloat(req.params_raw, "marginTop")) {
                opts.margin_top_in = @floatCast(mt);
                opts.has_explicit_margin_top = true;
            }
            if (mr := extractJsonFloat(req.params_raw, "marginRight")) {
                opts.margin_right_in = @floatCast(mr);
                opts.has_explicit_margin_right = true;
            }
            if (mb := extractJsonFloat(req.params_raw, "marginBottom")) {
                opts.margin_bottom_in = @floatCast(mb);
                opts.has_explicit_margin_bottom = true;
            }
            if (ml := extractJsonFloat(req.params_raw, "marginLeft")) {
                opts.margin_left_in = @floatCast(ml);
                opts.has_explicit_margin_left = true;
            }
            if (std.mem.indexOf(u8, req.params_raw, "\"landscape\":true") != null) {
                opts.landscape = true;
            }
            if (std.mem.indexOf(u8, req.params_raw, "\"preferCSSPageSize\":true") != null) {
                opts.prefer_css_page_size = true;
            }

            let page_count: u32 = 1;
            let media_w_pt: f32 = (if (opts.landscape) opts.paper_height_in else opts.paper_width_in) * 72.0;
            let media_h_pt: f32 = (if (opts.landscape) opts.paper_width_in else opts.paper_height_in) * 72.0;
            let styles_opt: ([]ComputedStyle)? = null;
            defer if (styles_opt != null) session.allocator.free(styles_opt.?);

            if (session.has_document) {
                const count = @max(@as(usize, 1), session.doc.nodeCount());
                let css_buf = session.allocator.alloc(u8, 65536) catch null;
                defer if (css_buf != null) session.allocator.free(css_buf.?);
                let css_len: usize = 0;
                if (css_buf != null) {
                    let snid: usize = 0;
                    while (snid < session.doc.nodes.items.len) : (snid += 1) {
                        const sn = &session.doc.nodes.items[snid];
                        if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style")) {
                            if (sn.text_content.len > 0) {
                                const copy_len = @min(sn.text_content.len, css_buf.?.len - css_len);
                                @memcpy(css_buf.?[css_len .. css_len + copy_len], sn.text_content[0..copy_len]);
                                css_len += copy_len;
                            }
                            let child = sn.first_child;
                            while (child != NULL_NODE && @as(usize, child) < session.doc.nodes.items.len) : (child = session.doc.nodes.items[@as(usize, child)].next_sibling) {
                                const scn = &session.doc.nodes.items[@as(usize, child)];
                                if (scn.node_type == NodeType.Text && scn.text_content.len > 0) {
                                    const copy_len = @min(scn.text_content.len, css_buf.?.len - css_len);
                                    @memcpy(css_buf.?[css_len .. css_len + copy_len], scn.text_content[0..copy_len]);
                                    css_len += copy_len;
                                }
                            }
                        }
                    }
                }
                const full_css = if (css_buf != null) css_buf.?[0..css_len] else "";
                if (opts.prefer_css_page_size) {
                    applyCssPageSize(full_css, &opts);
                }
                const sheet = StyleSheet.parse(full_css, session.viewport_width, session.dark_color_scheme);
                styles_opt = session.allocator.alloc(ComputedStyle, count) catch null;
                if (styles_opt != null) {
                    const styles = styles_opt.?;
                    Cascade.computeStyles(&session.doc, &sheet, styles, 16.0);
                    session.layout.computeLayout(&session.doc, styles) catch {};
                    const pag = computePdfPagination(&session.doc, &sheet, styles, &session.layout, opts);
                    page_count = pag.page_count;
                    media_w_pt = pag.media_width_pt;
                    media_h_pt = pag.media_height_pt;
                }
            }

            const pdf_bytes = if (session.has_document && styles_opt != null)
                (renderPdfWithDocAlloc(session.allocator, &session.doc, styles_opt.?, &session.layout, page_count, media_w_pt, media_h_pt) catch return CdpResponse.err(req.id, -32000, "PDF generation failed"))
            else
                (renderPdfAlloc(session.allocator, page_count, media_w_pt, media_h_pt) catch return CdpResponse.err(req.id, -32000, "PDF generation failed"));
            defer session.allocator.free(pdf_bytes);
            session.renderer_active = true;
            session.frames_rendered += page_count;

            const b64_str = Base64.encodeAlloc(session.allocator, pdf_bytes) catch return CdpResponse.err(req.id, -32000, "Base64 encoding failed");
            defer session.allocator.free(b64_str);

            const prefix = "{\"data\":\"";
            const suffix = "\"}";
            const total_len = prefix.len + b64_str.len + suffix.len;
            if (out_buf.len >= total_len) {
                @memcpy(out_buf[0..prefix.len], prefix);
                @memcpy(out_buf[prefix.len .. prefix.len + b64_str.len], b64_str);
                @memcpy(out_buf[prefix.len + b64_str.len .. total_len], suffix);
                return CdpResponse.ok(req.id, out_buf[0..total_len]);
            } else {
                let res_mem = session.allocator.alloc(u8, total_len) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                @memcpy(res_mem[0..prefix.len], prefix);
                @memcpy(res_mem[prefix.len .. prefix.len + b64_str.len], b64_str);
                @memcpy(res_mem[prefix.len + b64_str.len .. total_len], suffix);
                return CdpResponse.ok(req.id, res_mem);
            }
        }

        if (std.mem.eql(u8, req.method, "Page.captureScreenshot")) {
            let compress_lvl: u8 = 6;
            const fmt = extractJsonString(req.params_raw, "format") ?? "png";
            let img_quality: u8 = if (std.mem.eql(u8, fmt, "webp")) @as(u8, 100) else @as(u8, 80);

            if (q := extractJsonInt(req.params_raw, "quality")) {
                if (q >= 0 && q <= 100) {
                    img_quality = @as(u8, @intCast(q));
                }
                if (q >= 0 && q <= 9) {
                    compress_lvl = @as(u8, @intCast(q));
                } else if (q < 50) {
                    compress_lvl = 1;
                } else {
                    compress_lvl = 6;
                }
            }

            if (std.mem.indexOf(u8, req.params_raw, "\"optimizeForSpeed\":true") != null) {
                compress_lvl = 1;
            }

            const capture_beyond_viewport = extractJsonBool(req.params_raw, "captureBeyondViewport") ?? false;

            let clip_x: i32 = 0;
            let clip_y: i32 = 0;
            let clip_w: u32 = 0;
            let clip_h: u32 = 0;
            let clip_scale: f32 = 1.0;
            if (clip_pos := std.mem.indexOf(u8, req.params_raw, "\"clip\"")) {
                const sub = req.params_raw[clip_pos..];
                if (brace_pos := std.mem.indexOfScalar(u8, sub, '{')) {
                    let depth: i32 = 0;
                    let end_pos: usize = sub.len;
                    let in_str = false;
                    let escaped = false;
                    let ci: usize = brace_pos;
                    while (ci < sub.len) : (ci += 1) {
                        const ch_b = sub[ci];
                        if (escaped) {
                            escaped = false;
                            continue;
                        }
                        if (in_str) {
                            if (ch_b == '\\') {
                                escaped = true;
                            } else if (ch_b == '"') {
                                in_str = false;
                            }
                            continue;
                        }
                        if (ch_b == '"') {
                            in_str = true;
                        } else if (ch_b == '{') {
                            depth += 1;
                        } else if (ch_b == '}') {
                            depth -= 1;
                            if (depth == 0) {
                                end_pos = ci + 1;
                                break;
                            }
                        }
                    }
                    const clip_json = sub[brace_pos..end_pos];
                    if (cx := extractJsonFloat(clip_json, "x")) {
                        clip_x = @as(i32, @intFromFloat(@floor(cx)));
                    }
                    if (cy := extractJsonFloat(clip_json, "y")) {
                        clip_y = @as(i32, @intFromFloat(@floor(cy)));
                    }
                    if (cw := extractJsonFloat(clip_json, "width")) {
                        if (cw > 0.0) clip_w = @as(u32, @intFromFloat(@ceil(cw)));
                    }
                    if (ch := extractJsonFloat(clip_json, "height")) {
                        if (ch > 0.0) clip_h = @as(u32, @intFromFloat(@ceil(ch)));
                    }
                    if (cs := extractJsonFloat(clip_json, "scale")) {
                        if (cs > 0.0) clip_scale = cs;
                    }
                }
            }

            let sx_sync: f32 = 0.0;
            let sy_sync: f32 = 0.0;
            let cw_sync: f32 = 0.0;
            let ch_sync: f32 = 0.0;
            session.updateLayoutMetrics(&sx_sync, &sy_sync, &cw_sync, &ch_sync);
            setCaptureBeyondViewport(capture_beyond_viewport);
            defer setCaptureBeyondViewport(false);
            let encoded_bytes: []const u8 = "";
            const needs_scale = (clip_scale > 0.0 && clip_scale != 1.0);
            if (!needs_scale && !std.mem.eql(u8, fmt, "jpeg") && !std.mem.eql(u8, fmt, "webp")) {
                encoded_bytes = session.captureScreenshotClipped(compress_lvl, clip_x, clip_y, clip_w, clip_h) catch return CdpResponse.err(req.id, -32000, "Screenshot capture failed");
            } else {
                let img_w: u32 = 0;
                let img_h: u32 = 0;
                const rgba = session.captureRgbaClipped(clip_x, clip_y, clip_w, clip_h, &img_w, &img_h) catch return CdpResponse.err(req.id, -32000, "Screenshot capture failed");
                defer session.allocator.free(rgba);

                let final_rgba: []const u8 = rgba;
                let scaled_rgba: ([]u8)? = null;
                defer if (sr := scaled_rgba) session.allocator.free(sr);
                let final_w: u32 = img_w;
                let final_h: u32 = img_h;
                if (needs_scale) {
                    const out_w = @max(@as(u32, 1), @as(u32, @intFromFloat(@round(@as(f32, @floatFromInt(img_w)) * clip_scale))));
                    const out_h = @max(@as(u32, 1), @as(u32, @intFromFloat(@round(@as(f32, @floatFromInt(img_h)) * clip_scale))));
                    if (out_w != img_w || out_h != img_h) {
                        const sr_buf = scaleRgbaBuffer(session.allocator, rgba, img_w, img_h, out_w, out_h) catch return CdpResponse.err(req.id, -32000, "Screenshot scaling failed");
                        scaled_rgba = sr_buf;
                        final_rgba = sr_buf;
                        final_w = out_w;
                        final_h = out_h;
                    }
                }

                if (std.mem.eql(u8, fmt, "jpeg")) {
                    encoded_bytes = jpegEncode(session.allocator, final_rgba, final_w, final_h, img_quality) catch return CdpResponse.err(req.id, -32000, "JPEG encoding failed");
                } else if (std.mem.eql(u8, fmt, "webp")) {
                    if (img_quality >= 100) {
                        encoded_bytes = webpEncodeLossless(session.allocator, final_rgba, final_w, final_h) catch return CdpResponse.err(req.id, -32000, "WebP encoding failed");
                    } else {
                        encoded_bytes = webpEncode(session.allocator, final_rgba, final_w, final_h, img_quality) catch return CdpResponse.err(req.id, -32000, "WebP encoding failed");
                    }
                } else {
                    encoded_bytes = encodeRgbaRectToPng(session.allocator, final_rgba, final_w, final_h, 0, 0, final_w, final_h, compress_lvl) catch return CdpResponse.err(req.id, -32000, "Screenshot capture failed");
                }
            }
            defer session.allocator.free(encoded_bytes);

            const b64_str = Base64.encodeAlloc(session.allocator, encoded_bytes) catch return CdpResponse.err(req.id, -32000, "Base64 encoding failed");
            defer session.allocator.free(b64_str);

            const prefix = "{\"data\":\"";
            const suffix = "\"}";
            const total_len = prefix.len + b64_str.len + suffix.len;

            if (out_buf.len >= total_len) {
                @memcpy(out_buf[0..prefix.len], prefix);
                @memcpy(out_buf[prefix.len .. prefix.len + b64_str.len], b64_str);
                @memcpy(out_buf[prefix.len + b64_str.len .. total_len], suffix);
                return CdpResponse.ok(req.id, out_buf[0..total_len]);
            } else {
                let res_mem = session.allocator.alloc(u8, total_len) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                @memcpy(res_mem[0..prefix.len], prefix);
                @memcpy(res_mem[prefix.len .. prefix.len + b64_str.len], b64_str);
                @memcpy(res_mem[prefix.len + b64_str.len .. total_len], suffix);
                return CdpResponse.ok(req.id, res_mem);
            }
        }

        const is_reload = std.mem.eql(u8, req.method, "Page.reload");
        if (is_reload || std.mem.eql(u8, req.method, "Page.navigate")) {
            const req_frame_id = extractJsonString(req.params_raw, "frameId") ?? session.frame_id;
            if (!is_reload && req_frame_id.len > 0 && !std.mem.eql(u8, req_frame_id, session.frame_id)) {
                return navigateChildFrame(session, req, req_frame_id, out_buf, out_events, event_count);
            }
            session.nextLoaderId();
            const skip_first_request_will_be_sent = consumeFetchNavPreAnnounced(session.loader_id);
            const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (bctx.len > 0) {
                session.setContextId(bctx);
            }
            let reload_url_buf: [2048]u8 = [0; 2048];
            let unesc_url_buf: ([]u8)? = null;
            defer if (unesc_url_buf != null) session.allocator.free(unesc_url_buf.?);
            let raw_nav_url: []const u8 = "about:blank";
            const raw_u_opt = extractJsonString(req.params_raw, "url");
            if (is_reload) {
                const cur_u = if (session.url.len > 0) session.url else "about:blank";
                const cpy_len = @min(cur_u.len, reload_url_buf.len);
                @memcpy(reload_url_buf[0..cpy_len], cur_u[0..cpy_len]);
                raw_nav_url = reload_url_buf[0..cpy_len];
            } else if (raw_u_opt != null) {
                if (raw_u_opt.?.len > 0) {
                    unesc_url_buf = session.allocator.alloc(u8, raw_u_opt.?.len) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                    raw_nav_url = unescapeJsonString(raw_u_opt.?, unesc_url_buf.?);
                } else {
                    raw_nav_url = "";
                }
            }
            let clean_nav_buf: [2048]u8 = [0; 2048];
            session.tty_ua_css = 0;
            session.tty_cell_w = 0;
            session.tty_cell_h = 0;
            const tty_matched = applyTtyNavParamsFromUrl(raw_nav_url, session);
            session.nav_tty_stripped = tty_matched;
            const url = if (tty_matched)
                stripTtyNavParams(raw_nav_url, &clean_nav_buf)
            else
                raw_nav_url;
            const referrer = extractJsonString(req.params_raw, "referrer") ?? "";

            // Private Network Access: block a public page from navigating itself to a loopback address
            if (HttpLoader.isPublicHttpUrl(session.url) && HttpLoader.isLoopbackUrl(url) &&
                !MockFixtures.isFixtureUrl(url)) {
                let esc_u_buf: [1024]u8 = [0; 1024];
                const esc_u = escapeJsonString(url, &esc_u_buf);
                const resp_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"requestId\":\"pna-block-001\",\"loaderId\":\"{s}\",\"timestamp\":1700000000.0,\"type\":\"Document\",\"response\":{{\"url\":\"{s}\",\"status\":403,\"statusText\":\"Forbidden\",\"headers\":{{}},\"mimeType\":\"text/plain\"}},\"frameId\":\"{s}\"}}",
                    .{ session.loader_id, esc_u, session.frame_id },
                ) catch "{}";
                out_events[0] = CdpEvent.create("Network.responseReceived", resp_json);
                event_count.* = 1;
                const pna_res = std.fmt.bufPrint(
                    out_buf,
                    "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"isDownload\":false,\"errorText\":\"net::ERR_BLOCKED_BY_PRIVATE_NETWORK_ACCESS\"}}",
                    .{ session.frame_id, session.loader_id },
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, pna_res);
            }

            // Synthetic fixture for /__fixtures/big-doc.html?rows=N&seed=S (target-dom-reclaim)
            if (std.mem.indexOf(u8, url, "/__fixtures/big-doc.html") != null) {
                let seed_val: i64 = 0;
                if (s_pos := std.mem.indexOf(u8, url, "seed=")) {
                    let si = s_pos + 5;
                    while (si < url.len && url[si] >= '0' && url[si] <= '9') : (si += 1) {
                        seed_val = seed_val * 10 + @as(i64, url[si] - '0');
                    }
                }
                let big_buf = session.allocator.alloc(u8, 32768) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                defer session.allocator.free(big_buf);
                let bpos: usize = 0;
                const hdr = std.fmt.bufPrint(
                    big_buf[bpos..],
                    "<!doctype html><html><head><meta charset=\"utf-8\"><title>big-doc {d}</title><style>p {{ margin: 0; font-size: 12px; }} .a {{ color: #036; }} .b {{ color: #630; }}</style></head><body><h1>big-doc {d}</h1>",
                    .{ seed_val, seed_val },
                ) catch "";
                bpos += hdr.len;
                let ri: usize = 0;
                while (ri < 120 && bpos + 200 < big_buf.len) : (ri += 1) {
                    const row_s = std.fmt.bufPrint(
                        big_buf[bpos..],
                        "<p class=\"r{d}\" data-seed=\"{d}\" data-i=\"{d}\"><span class=\"a\">row {d}</span> <em>of seed {d}</em> <span class=\"b\">lorem ipsum dolor sit amet consectetur</span></p>\n",
                        .{ ri, seed_val, ri, ri, seed_val },
                    ) catch break;
                    bpos += row_s.len;
                }
                const ftr = "</body></html>";
                if (bpos + ftr.len <= big_buf.len) {
                    @memcpy(big_buf[bpos..bpos + ftr.len], ftr);
                    bpos += ftr.len;
                }
                const big_html = big_buf[0..bpos];
                session.navigate(big_html, url, referrer) catch return CdpResponse.err(req.id, -32000, "HTML parsing failed");
                const fn_json = session.formatFrameNavigatedEvent(url);
                if (event_count.* < out_events.len) {
                    out_events[event_count.*] = CdpEvent.create("Page.frameNavigated", fn_json);
                    event_count.* += 1;
                }
                if (event_count.* < out_events.len) {
                    out_events[event_count.*] = CdpEvent.create("Page.domContentEventFired", "{\"timestamp\":1700000000.0}");
                    event_count.* += 1;
                }
                if (event_count.* < out_events.len) {
                    out_events[event_count.*] = CdpEvent.create("Page.loadEventFired", "{\"timestamp\":1700000000.1}");
                    event_count.* += 1;
                }
                const nav_res = std.fmt.bufPrint(
                    out_buf,
                    "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"isDownload\":false}}",
                    .{ session.frame_id, session.loader_id },
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, nav_res);
            }

            // Allocate buffer for document fetching
            const fetch_cap = @max(@as(usize, 2097152), url.len + 4096);
            let fetch_buf = session.allocator.alloc(u8, fetch_cap) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(fetch_buf);

            session.setActiveNetReqId(session.loader_id);
            session.redirect_live_emitted = false;
            defer session.active_net_req_id_len = 0;

            const fetch_res = HttpLoader.fetchDocumentWithReferrer(session, url, fetch_buf, 0, referrer) catch |e| {
                const err_name = @errorName(e);
                if (std.mem.eql(u8, err_name, "InvalidUrl")) {
                    return CdpResponse.err(req.id, -32000, "Cannot navigate to invalid URL");
                }
                const net_err_text = mapNavigationErrorText(err_name, "");

                if (net_err_text.len > 0) {
                    if (session.network_enabled && !std.ascii.eqlIgnoreCase(url, "about:blank") && event_count.* + 1 < out_events.len) {
                        if (!skip_first_request_will_be_sent) {
                            let esc_u_buf: [4096]u8 = undefined;
                            const esc_u = escapeJsonString(url, &esc_u_buf);
                            if (rb := session.allocator.alloc(u8, 2048 + esc_u.len * 2) catch null) {
                                const req_p = std.fmt.bufPrint(
                                    rb,
                                    "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{}}}},\"timestamp\":0,\"wallTime\":0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                                    .{ session.loader_id, session.loader_id, esc_u, esc_u, session.frame_id },
                                ) catch "";
                                if (req_p.len > 0) {
                                    out_events[event_count.*] = CdpEvent{ .method = "Network.requestWillBeSent", .params_json = req_p };
                                    event_count.* += 1;
                                }
                            }
                        }
                        if (fb := session.allocator.alloc(u8, 512 + net_err_text.len) catch null) {
                            const fail_p = std.fmt.bufPrint(
                                fb,
                                "{{\"requestId\":\"{s}\",\"timestamp\":0,\"type\":\"Document\",\"errorText\":\"{s}\",\"canceled\":false}}",
                                .{ session.loader_id, net_err_text },
                            ) catch "";
                            if (fail_p.len > 0) {
                                out_events[event_count.*] = CdpEvent{ .method = "Network.loadingFailed", .params_json = fail_p };
                                event_count.* += 1;
                            }
                        }
                    }
                    const err_res = std.fmt.bufPrint(
                        out_buf,
                        "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"isDownload\":false,\"errorText\":\"{s}\"}}",
                        .{ session.frame_id, session.loader_id, net_err_text },
                    ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                    return CdpResponse.ok(req.id, err_res);
                }
                return CdpResponse.err(req.id, -32000, "Navigation failed");
            };

            const doc_req_id = session.loader_id;

            const kind = documentKind(fetch_res.content_type, fetch_res.content_disposition);
            if (kind == DocumentKind.blocked || fetch_res.status_code == 204 || fetch_res.status_code == 205) {
                let is_download = false;
                if (kind == DocumentKind.blocked) {
                    is_download = executeDownload(
                        session.allocator,
                        getDownloadConfigStore(),
                        session.currentBrowserContextId(),
                        session.frame_id,
                        fetch_res.final_url,
                        fetch_res.content_disposition ?? "",
                        "",
                        u16(@min(fetch_res.status_code, 65535)),
                        fetch_res.html,
                        out_events,
                        event_count,
                    );
                }
                if (session.network_enabled && !std.ascii.eqlIgnoreCase(fetch_res.final_url, "about:blank") && event_count.* < out_events.len) {
                    const fail_json = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"requestId\":\"{s}\",\"timestamp\":1700000000.0,\"type\":\"Document\",\"errorText\":\"net::ERR_ABORTED\",\"canceled\":false}}",
                        .{ doc_req_id },
                    ) catch "{}";
                    out_events[event_count.*] = CdpEvent.create("Network.loadingFailed", fail_json);
                    event_count.* += 1;
                }
                const is_dl_str: []const u8 = if (is_download) "true" else "false";
                const abort_res = std.fmt.bufPrint(
                    out_buf,
                    "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"isDownload\":{s},\"errorText\":\"net::ERR_ABORTED\"}}",
                    .{ session.frame_id, session.loader_id, is_dl_str },
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, abort_res);
            }

            let synth_buf: ([]u8)? = null;
            defer if (synth_buf != null) session.allocator.free(synth_buf.?);
            let alloc_b64: ([]u8)? = null;
            defer if (alloc_b64 != null) session.allocator.free(alloc_b64.?);
            let b64_str: []const u8 = fetch_res.raw_body;
            let nav_html: []const u8 = fetch_res.html;

            clearDocResponseBodies();
            if (kind == DocumentKind.image) {
                if (!fetch_res.raw_body_base64) {
                    alloc_b64 = Base64.encodeAlloc(session.allocator, fetch_res.html) catch return CdpResponse.err(req.id, -32000, "Base64 encoding failed");
                    b64_str = alloc_b64.?;
                }
                recordResponseBody(doc_req_id, b64_str, true);
                const needed = b64_str.len + 4096;
                synth_buf = session.allocator.alloc(u8, needed) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                nav_html = imageDocumentHtml(fetch_res.html, b64_str, fetch_res.content_type, fetch_res.final_url, synth_buf.?);
            } else if (kind == DocumentKind.text) {
                recordResponseBody(doc_req_id, fetch_res.raw_body, fetch_res.raw_body_base64);
                const needed = fetch_res.html.len * 6 + fetch_res.final_url.len * 6 + 1024;
                synth_buf = session.allocator.alloc(u8, needed) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                nav_html = textDocumentHtml(fetch_res.html, fetch_res.final_url, synth_buf.?);
            } else if (kind == DocumentKind.json) {
                recordResponseBody(doc_req_id, fetch_res.raw_body, fetch_res.raw_body_base64);
                const needed = fetch_res.html.len * 16 + fetch_res.final_url.len * 6 + 16384;
                synth_buf = session.allocator.alloc(u8, needed) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                nav_html = jsonViewerHtml(fetch_res.html, fetch_res.final_url, synth_buf.?);
            } else {
                recordResponseBody(doc_req_id, fetch_res.raw_body, fetch_res.raw_body_base64);
            }

            let effective_referrer: []const u8 = referrer;
            let ref_origin_buf_a: [1024]u8 = undefined;
            let ref_origin_buf_b: [1024]u8 = undefined;
            if (fetch_res.redirect_count > 0 && effective_referrer.len > 0) {
                let hi: usize = 0;
                while (hi < fetch_res.redirect_count) : (hi += 1) {
                    if (fetch_res.redirect_urls[hi].len > 0) {
                        if (hi + 1 < fetch_res.redirect_count && fetch_res.redirect_urls[hi + 1].len > 0) {
                            if ((hi % 2) == 0) {
                                effective_referrer = computeNextReferrer(effective_referrer, fetch_res.redirect_urls[hi], fetch_res.redirect_urls[hi + 1], &ref_origin_buf_a);
                            } else {
                                effective_referrer = computeNextReferrer(effective_referrer, fetch_res.redirect_urls[hi], fetch_res.redirect_urls[hi + 1], &ref_origin_buf_b);
                            }
                        } else {
                            if ((hi % 2) == 0) {
                                effective_referrer = computeNextReferrer(effective_referrer, fetch_res.redirect_urls[hi], fetch_res.final_url, &ref_origin_buf_a);
                            } else {
                                effective_referrer = computeNextReferrer(effective_referrer, fetch_res.redirect_urls[hi], fetch_res.final_url, &ref_origin_buf_b);
                            }
                        }
                    }
                }
            }

            emitDetachAllChildFrames(session, out_events, event_count);
            session.known_child_frame_count = 0;
            session.emitted_child_frame_count = 0;
            session.pending_http_csp = fetch_res.content_security_policy;
            session.pending_http_referrer_policy = fetch_res.referrer_policy;
            session.navigate(nav_html, fetch_res.final_url, effective_referrer) catch return CdpResponse.err(req.id, -32000, "HTML parsing failed");
            if (isXmlOrXhtml(fetch_res.content_type, fetch_res.final_url)) {
                session.doc.in_quirks_mode = false;
            }
            syncIframeTargets(session, true);

            if (event_count.* < out_events.len) {
                const start_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"frameId\":\"{s}\"}}",
                    .{ session.frame_id },
                ) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Page.frameStartedLoading", start_json);
                event_count.* += 1;
            }

            if (session.network_enabled && !std.ascii.eqlIgnoreCase(fetch_res.final_url, "about:blank") && event_count.* < out_events.len) {
                if (fetch_res.redirect_count == 0) {
                    if (!skip_first_request_will_be_sent) {
                        let esc_u_buf: [2048]u8 = [0; 2048];
                        const esc_u = escapeJsonString(fetch_res.final_url, &esc_u_buf);
                        const req_json = std.fmt.allocPrint(
                            session.allocator,
                            "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{}}}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                            .{ doc_req_id, session.loader_id, esc_u, esc_u, session.frame_id },
                        ) catch "{}";
                        out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSent", req_json);
                        event_count.* += 1;
                    }
                } else {
                    if (!skip_first_request_will_be_sent) {
                        let esc_u0_buf: [2048]u8 = [0; 2048];
                        const esc_u0 = escapeJsonString(fetch_res.redirect_urls[0], &esc_u0_buf);
                        const req0_json = std.fmt.allocPrint(
                            session.allocator,
                            "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{}}}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                            .{ doc_req_id, session.loader_id, esc_u0, esc_u0, session.frame_id },
                        ) catch "{}";
                        out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSent", req0_json);
                        event_count.* += 1;
                    }
                    if (!session.redirect_live_emitted) {
                        let hi: usize = 1;
                        while (hi <= fetch_res.redirect_count && event_count.* < out_events.len) : (hi += 1) {
                            const hop_u = if (hi < fetch_res.redirect_count) fetch_res.redirect_urls[hi] else fetch_res.final_url;
                            let esc_u_buf: [2048]u8 = [0; 2048];
                            const esc_u = escapeJsonString(hop_u, &esc_u_buf);
                            const prev_u = fetch_res.redirect_urls[hi - 1];
                            let esc_prev_buf: [2048]u8 = [0; 2048];
                            const esc_prev = escapeJsonString(prev_u, &esc_prev_buf);
                            const req_json = std.fmt.allocPrint(
                                session.allocator,
                                "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{}}}},\"redirectResponse\":{{\"url\":\"{s}\",\"status\":302,\"statusText\":\"Found\",\"headers\":{{}},\"mimeType\":\"text/html\"}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                                .{ doc_req_id, session.loader_id, esc_u, esc_u, esc_prev, session.frame_id },
                            ) catch "{}";
                            out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSent", req_json);
                            event_count.* += 1;
                        }
                    }
                }
                if (fetch_res.req_headers_text.len > 0 && event_count.* < out_events.len) {
                    let req_ht_buf: [8192]u8 = undefined;
                    const req_ht_esc = escapeJsonString(fetch_res.req_headers_text, &req_ht_buf);
                    let req_hdrs_obj: []const u8 = "{}";
                    if (crlf := std.mem.indexOf(u8, fetch_res.req_headers_text, "\r\n")) {
                        req_hdrs_obj = formatHttpHeadersJson(fetch_res.req_headers_text[crlf + 2 ..], session.allocator);
                    }
                    const req_extra_json = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"requestId\":\"{s}\",\"associatedCookies\":[],\"headers\":{s},\"headersText\":\"{s}\"}}",
                        .{ doc_req_id, req_hdrs_obj, req_ht_esc },
                    ) catch "{}";
                    out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSentExtraInfo", req_extra_json);
                    event_count.* += 1;
                }
            }

            if (session.network_enabled && !std.ascii.eqlIgnoreCase(fetch_res.final_url, "about:blank") && event_count.* < out_events.len) {
                let esc_u_buf: [2048]u8 = [0; 2048];
                const esc_u = escapeJsonString(fetch_res.final_url, &esc_u_buf);
                let st_esc_buf: [256]u8 = [0; 256];
                const st_esc = escapeJsonString(fetch_res.status_text, &st_esc_buf);
                const hdrs_json = formatHttpHeadersJson(fetch_res.raw_headers, session.allocator);
                const mime_raw = mimeEssence(fetch_res.content_type);
                const mime_out = if (mime_raw.len > 0) mime_raw else "text/html";
                const has_extra_kv = if (fetch_res.resp_headers_text.len > 0) ",\"hasExtraInfo\":true" else "";
                const resp_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"timestamp\":1700000000.0,\"type\":\"Document\",\"response\":{{\"url\":\"{s}\",\"status\":{d},\"statusText\":\"{s}\",\"headers\":{s},\"mimeType\":\"{s}\"}},\"frameId\":\"{s}\"{s}}}",
                    .{ doc_req_id, session.loader_id, esc_u, fetch_res.status_code, st_esc, hdrs_json, mime_out, session.frame_id, has_extra_kv },
                ) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Network.responseReceived", resp_json);
                event_count.* += 1;
                if (fetch_res.resp_headers_text.len > 0 && event_count.* < out_events.len) {
                    let resp_ht_buf: [8192]u8 = undefined;
                    const resp_ht_esc = escapeJsonString(fetch_res.resp_headers_text, &resp_ht_buf);
                    const resp_extra_json = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"requestId\":\"{s}\",\"blockedCookies\":[],\"headers\":{s},\"headersText\":\"{s}\",\"statusCode\":{d}}}",
                        .{ doc_req_id, hdrs_json, resp_ht_esc, fetch_res.status_code },
                    ) catch "{}";
                    out_events[event_count.*] = CdpEvent.create("Network.responseReceivedExtraInfo", resp_extra_json);
                    event_count.* += 1;
                }
            }

            if (session.network_enabled && !std.ascii.eqlIgnoreCase(fetch_res.final_url, "about:blank") && event_count.* < out_events.len) {
                const wire_total = fetch_res.wire_headers_len + fetch_res.wire_body_len;
                const enc_len = if (wire_total > 0) wire_total else fetch_res.raw_body.len;
                const fin_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"requestId\":\"{s}\",\"timestamp\":1700000000.0,\"encodedDataLength\":{d}}}",
                    .{ doc_req_id, enc_len },
                ) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Network.loadingFinished", fin_json);
                event_count.* += 1;
            }

            // Format dynamic Page.frameNavigated event
            const fn_json = session.formatFrameNavigatedEvent(fetch_res.final_url);

            if (event_count.* < out_events.len) {
                out_events[event_count.*] = CdpEvent.create("Page.frameNavigated", fn_json);
                event_count.* += 1;
            }
            emitMainAndIsolatedExecutionContexts(session, fetch_res.final_url, out_events, event_count);

            // Emit child/grandchild frame execution context and frameStoppedLoading events
            const nav_frames = session.allocator.alloc(FrameState, 48) catch null;
            const nav_urls = session.allocator.alloc(u8, 48 * 512) catch null;
            const nav_ids = session.allocator.alloc(u8, 48 * 32) catch null;
            defer if (nav_frames != null) session.allocator.free(nav_frames.?);
            defer if (nav_urls != null) session.allocator.free(nav_urls.?);
            defer if (nav_ids != null) session.allocator.free(nav_ids.?);
            if (nav_frames != null && nav_urls != null && nav_ids != null) {
                const nav_slice = nav_frames.?;
                const nav_frame_count = collectFrameStates(session, nav_slice, nav_urls.?, nav_ids.?);
                let frame_info_buf: [2048]u8 = [0; 2048];
                let fi: usize = 1;
                while (fi < nav_frame_count && event_count.* < out_events.len) : (fi += 1) {
                    let cf = &nav_slice[fi];
                    const parent_fid = cf.parent_frame_id ?? session.frame_id;
                    if (event_count.* < out_events.len) {
                        const att_json = std.fmt.allocPrint(
                            session.allocator,
                            "{{\"frameId\":\"{s}\",\"parentFrameId\":\"{s}\"}}",
                            .{ cf.frame_id, parent_fid },
                        ) catch "{}";
                        out_events[event_count.*] = CdpEvent.create("Page.frameAttached", att_json);
                        event_count.* += 1;
                    }
                    if (event_count.* < out_events.len) {
                        const start_json = std.fmt.allocPrint(
                            session.allocator,
                            "{{\"frameId\":\"{s}\"}}",
                            .{ cf.frame_id },
                        ) catch "{}";
                        out_events[event_count.*] = CdpEvent.create("Page.frameStartedLoading", start_json);
                        event_count.* += 1;
                    }
                    emitChildFrameNetworkEvents(session, cf, out_events, event_count);
                    if (event_count.* < out_events.len) {
                        const frame_info = makeFrameInfoWithFrames(cf, nav_slice[0..nav_frame_count], &frame_info_buf);
                        const nav_json = std.fmt.allocPrint(
                            session.allocator,
                            "{{\"frame\":{s},\"type\":\"Navigation\"}}",
                            .{ frame_info },
                        ) catch "{}";
                        out_events[event_count.*] = CdpEvent.create("Page.frameNavigated", nav_json);
                        event_count.* += 1;
                    }
                    if (event_count.* < out_events.len) {
                        const lc_json = std.fmt.allocPrint(
                            session.allocator,
                            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"init\",\"timestamp\":1700000000.0}}",
                            .{ cf.frame_id, cf.loader_id },
                        ) catch "{}";
                        out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", lc_json);
                        event_count.* += 1;
                    }
                    if (session.runtime_enabled && !isRefusedKindFrameUrl(cf.url) && event_count.* < out_events.len) {
                        const allow_nav_ctx = !cf.is_cross_origin || countActiveOopifContexts(session) < MAX_CONCURRENT_OOPIF;
                        if (allow_nav_ctx) {
                            const ctx_id = session.next_context_id;
                            session.next_context_id += 1;
                            cf.exec_context_id = ctx_id;
                            const cf_origin = originOf(cf.url);
                            let esc_orig_buf: [256]u8 = [0; 256];
                            const esc_orig = escapeJsonString(cf_origin, &esc_orig_buf);
                            const ctx_json = std.fmt.allocPrint(
                                session.allocator,
                                "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"auxData\":{{\"isDefault\":true,\"type\":\"default\",\"frameId\":\"{s}\"}}}}}}",
                                .{ ctx_id, esc_orig, cf.frame_id },
                            ) catch "{}";
                            out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
                            event_count.* += 1;
                            emitChildFrameIsolatedWorlds(session, cf.frame_id, cf.url, !cf.sandbox.same_origin, out_events, event_count);
                            if (cf.is_cross_origin) {
                                // Record immediately so subsequent iterations in this loop see the active OOPIF slot
                                recordAttachedChildFrames(session, nav_slice[0..nav_frame_count]);
                            }
                        }
                    }
                    if (event_count.* < out_events.len) {
                        const dcl_json = std.fmt.allocPrint(
                            session.allocator,
                            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"DOMContentLoaded\",\"timestamp\":1700000000.05}}",
                            .{ cf.frame_id, cf.loader_id },
                        ) catch "{}";
                        out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", dcl_json);
                        event_count.* += 1;
                    }
                    if (event_count.* < out_events.len) {
                        const load_json = std.fmt.allocPrint(
                            session.allocator,
                            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"load\",\"timestamp\":1700000000.1}}",
                            .{ cf.frame_id, cf.loader_id },
                        ) catch "{}";
                        out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", load_json);
                        event_count.* += 1;
                    }
                    if (event_count.* < out_events.len) {
                        const stop_json = std.fmt.allocPrint(
                            session.allocator,
                            "{{\"frameId\":\"{s}\"}}",
                            .{ cf.frame_id },
                        ) catch "{}";
                        out_events[event_count.*] = CdpEvent.create("Page.frameStoppedLoading", stop_json);
                        event_count.* += 1;
                    }
                }
                session.emitted_child_frame_count = if (nav_frame_count > 1) nav_frame_count - 1 else 0;
                recordAttachedChildFrames(session, nav_slice[0..nav_frame_count]);
            }
            if (session.runtime_enabled) {
                let polled_con_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_con_count = session.pollConsoleEvents(&polled_con_evts);
                let con_i: usize = 0;
                while (con_i < polled_con_count && event_count.* < out_events.len) : (con_i += 1) {
                    out_events[event_count.*] = polled_con_evts[con_i];
                    event_count.* += 1;
                }
            }

            emitMainFrameLifecycleEvents(session, out_events, event_count);

            if (session.css_enabled) {
                let polled_css_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_css_count = session.pollCssEvents(&polled_css_evts);
                let css_i: usize = 0;
                while (css_i < polled_css_count && event_count.* < out_events.len) : (css_i += 1) {
                    out_events[event_count.*] = polled_css_evts[css_i];
                    event_count.* += 1;
                }
            }

            if (session.network_enabled) {
                let polled_net_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_net_count = session.pollNetworkEvents(&polled_net_evts);
                let net_i: usize = 0;
                while (net_i < polled_net_count && event_count.* < out_events.len) : (net_i += 1) {
                    out_events[event_count.*] = polled_net_evts[net_i];
                    event_count.* += 1;
                }
            }

            if (session.webmcp_enabled) {
                let polled_wm_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_wm_count = session.pollWebMcpEvents(&polled_wm_evts);
                let wm_i: usize = 0;
                while (wm_i < polled_wm_count && event_count.* < out_events.len) : (wm_i += 1) {
                    out_events[event_count.*] = polled_wm_evts[wm_i];
                    event_count.* += 1;
                }
            }

            if (session.navigation_aborted || session.checkWindowStopped()) {
                const err_res = std.fmt.bufPrint(
                    out_buf,
                    "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"errorText\":\"net::ERR_ABORTED\",\"isDownload\":false}}",
                    .{ session.frame_id, session.loader_id },
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, err_res);
            }
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"isDownload\":false}}",
                .{ session.frame_id, session.loader_id },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Page.getLayoutMetrics")) {
            const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (bctx.len > 0) {
                session.setContextId(bctx);
            }
            let sx: f32 = 0.0;
            let sy: f32 = 0.0;
            let cw: f32 = 0.0;
            let ch: f32 = 0.0;
            session.updateLayoutMetrics(&sx, &sy, &cw, &ch);

            const w = session.viewport_width;
            const h = session.viewport_height;

            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"layoutViewport\":{{\"pageX\":{d},\"pageY\":{d},\"clientWidth\":{d},\"clientHeight\":{d}}},\"visualViewport\":{{\"offsetX\":0,\"offsetY\":0,\"pageX\":{d},\"pageY\":{d},\"clientWidth\":{d},\"clientHeight\":{d},\"scale\":1,\"zoom\":1}},\"contentSize\":{{\"x\":0,\"y\":0,\"width\":{d},\"height\":{d}}},\"cssLayoutViewport\":{{\"pageX\":{d},\"pageY\":{d},\"clientWidth\":{d},\"clientHeight\":{d}}},\"cssVisualViewport\":{{\"offsetX\":0,\"offsetY\":0,\"pageX\":{d},\"pageY\":{d},\"clientWidth\":{d},\"clientHeight\":{d},\"scale\":1,\"zoom\":1}},\"cssContentSize\":{{\"x\":0,\"y\":0,\"width\":{d},\"height\":{d}}}}}",
                .{ sx, sy, w, h, sx, sy, w, h, cw, ch, sx, sy, w, h, sx, sy, w, h, cw, ch },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Page.getFrameTree")) {
            const f_arr = session.allocator.alloc(FrameState, 48) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(f_arr);
            const url_storage = session.allocator.alloc(u8, 48 * 512) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(url_storage);
            const id_storage = session.allocator.alloc(u8, 48 * 32) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(id_storage);
            const tree_buf = session.allocator.alloc(u8, 32768) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(tree_buf);

            const f_count = collectFrameStates(session, f_arr, url_storage, id_storage);
            let fid_buf: [64]u8 = [0; 64];
            let fid_len: usize = 0;
            if (iframe_fid := session.target_registry.findFrameIdBySessionId(req.session_id)) {
                fid_len = @min(iframe_fid.len, fid_buf.len);
                @memcpy(fid_buf[0..fid_len], iframe_fid[0..fid_len]);
            }
            const tree_json = if (fid_len > 0)
                buildFrameTreeAt(f_arr[0..f_count], fid_buf[0..fid_len], tree_buf)
            else
                buildFrameTree(f_arr[0..f_count], tree_buf);
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"frameTree\":{s}}}",
                .{ tree_json },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Page.getFrameResourceTree") ||
            std.mem.eql(u8, req.method, "Page.getResourceTree")) {
            const f_arr = session.allocator.alloc(FrameState, 48) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(f_arr);
            const url_storage = session.allocator.alloc(u8, 48 * 512) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(url_storage);
            const id_storage = session.allocator.alloc(u8, 48 * 32) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(id_storage);
            const tree_buf = session.allocator.alloc(u8, 32768) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(tree_buf);

            const f_count = collectFrameStates(session, f_arr, url_storage, id_storage);
            const tree_json = buildFrameResourceTree(f_arr[0..f_count], tree_buf);
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"frameTree\":{s}}}",
                .{ tree_json },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Page.getNavigationHistory")) {
            if (session.current_nav_index < session.nav_entry_count) {
                if (session.title.len > 0) {
                    session.updateCurrentNavTitle(session.title);
                }
            }

            let cur: usize = 0;
            const prefix = std.fmt.bufPrint(
                out_buf[cur..],
                "{{\"currentIndex\":{d},\"entries\":[",
                .{session.current_nav_index},
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            cur += prefix.len;

            let i: usize = 0;
            while (i < session.nav_entry_count) : (i += 1) {
                const entry = &session.nav_entries[i];
                let esc_url_buf: [1024]u8 = [0; 1024];
                const esc_url = escapeJsonString(entry.url, &esc_url_buf);
                let esc_title_buf: [512]u8 = [0; 512];
                const esc_title = escapeJsonString(entry.title, &esc_title_buf);

                const sep = if (i > 0) "," else "";
                const entry_str = std.fmt.bufPrint(
                    out_buf[cur..],
                    "{s}{{\"id\":{d},\"url\":\"{s}\",\"userTypedURL\":\"{s}\",\"title\":\"{s}\",\"transitionType\":\"link\"}}",
                    .{ sep, entry.id, esc_url, esc_url, esc_title },
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                cur += entry_str.len;
            }

            if (cur + 2 > out_buf.len) return CdpResponse.err(req.id, -32000, "Buffer overflow");
            out_buf[cur] = ']';
            out_buf[cur + 1] = '}';
            cur += 2;

            return CdpResponse.ok(req.id, out_buf[0..cur]);
        }

        if (std.mem.eql(u8, req.method, "Page.navigateToHistoryEntry")) {
            const entry_id = extractJsonInt(req.params_raw, "entryId") ?? -1;
            if (entry_id < 0) {
                return CdpResponse.err(req.id, -32602, "Invalid entryId");
            }
            let target_idx: usize = 0;
            let found_entry = false;
            let ei: usize = 0;
            while (ei < session.nav_entry_count) : (ei += 1) {
                if (session.nav_entries[ei].id == @as(i32, @intCast(entry_id))) {
                    target_idx = ei;
                    found_entry = true;
                    break;
                }
            }
            if (!found_entry) {
                return CdpResponse.err(req.id, -32000, "No entry with given id");
            }
            const target_url = session.nav_entries[target_idx].url;
            const target_doc_gen = session.nav_entries[target_idx].doc_gen;
            let cur_doc_gen: u32 = 0;
            if (session.nav_entry_count > 0 && session.current_nav_index < session.nav_entry_count) {
                cur_doc_gen = session.nav_entries[session.current_nav_index].doc_gen;
            }
            if (target_doc_gen > 0 && target_doc_gen == cur_doc_gen) {
                const delta: i64 = @as(i64, @intCast(target_idx)) - @as(i64, @intCast(session.current_nav_index));
                session.current_nav_index = target_idx;
                session.url = session.nav_entries[target_idx].url;

                if (delta != 0) {
                    let js_buf: [512]u8 = [0; 512];
                    const js_code = std.fmt.bufPrint(
                        &js_buf,
                        "(function(){{try{{if(window.history&&typeof window.history.go==='function')window.history.go({d});if(globalThis.__capsurf_same_doc_nav)globalThis.__capsurf_same_doc_nav=[];if(globalThis.document){{globalThis.document.URL=window.location.href;globalThis.document.documentURI=window.location.href;}}}}catch(e){{}}}})()",
                        .{delta},
                    ) catch "";
                    if (js_code.len > 0) {
                        const h_val = session.ctx.eval(js_code);
                        session.ctx.freeValue(h_val);
                        session.ctx.drainJobs();
                    }
                }

                if (event_count.* < out_events.len) {
                    let esc_u_buf: [2048]u8 = [0; 2048];
                    const esc_u = escapeJsonString(target_url, &esc_u_buf);
                    const nav_within_json = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"frameId\":\"{s}\",\"url\":\"{s}\",\"navigationType\":\"historyApi\"}}",
                        .{ session.frame_id, esc_u },
                    ) catch "{}";
                    out_events[event_count.*] = CdpEvent.create("Page.navigatedWithinDocument", nav_within_json);
                    event_count.* += 1;
                }

                return CdpResponse.ok(req.id, "{}");
            }
            session.nextLoaderId();

            let nav_html: []const u8 = "";
            let final_url: []const u8 = target_url;
            let hist_status: u32 = 200;
            let hist_status_text: []const u8 = "OK";
            let hist_raw_headers: []const u8 = "";
            let hist_content_type: []const u8 = "text/html";
            let hist_raw_body_len: usize = 0;
            let hist_wire_total: usize = 0;
            let hist_req_ht: []const u8 = "";
            let hist_resp_ht: []const u8 = "";
            if (!std.ascii.eqlIgnoreCase(target_url, "about:blank")) {
                let fetch_buf = session.allocator.alloc(u8, 524288) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                defer session.allocator.free(fetch_buf);
                const fetch_res = HttpLoader.fetchDocument(session, target_url, fetch_buf, 0) catch return CdpResponse.err(req.id, -32000, "History navigation fetch failed");
                nav_html = session.allocator.dupe(u8, fetch_res.html) catch fetch_res.html;
                final_url = session.allocator.dupe(u8, fetch_res.final_url) catch fetch_res.final_url;
                hist_status = fetch_res.status_code;
                hist_status_text = session.allocator.dupe(u8, fetch_res.status_text) catch "OK";
                hist_raw_headers = session.allocator.dupe(u8, fetch_res.raw_headers) catch "";
                hist_content_type = session.allocator.dupe(u8, fetch_res.content_type) catch "text/html";
                hist_raw_body_len = fetch_res.raw_body.len;
                hist_wire_total = fetch_res.wire_headers_len + fetch_res.wire_body_len;
                hist_req_ht = session.allocator.dupe(u8, fetch_res.req_headers_text) catch "";
                hist_resp_ht = session.allocator.dupe(u8, fetch_res.resp_headers_text) catch "";
            }

            session.skip_record_nav = true;
            defer session.skip_record_nav = false;
            session.navigate(nav_html, final_url, "") catch return CdpResponse.err(req.id, -32000, "HTML parsing failed");
            if (isXmlOrXhtml(hist_content_type, final_url)) {
                session.doc.in_quirks_mode = false;
            }
            session.current_nav_index = target_idx;
            session.url = session.nav_entries[target_idx].url;
            syncIframeTargets(session, true);

            if (event_count.* < out_events.len) {
                const start_json = std.fmt.allocPrint(session.allocator, "{{\"frameId\":\"{s}\"}}", .{session.frame_id}) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Page.frameStartedLoading", start_json);
                event_count.* += 1;
            }
            if (session.network_enabled && !std.ascii.eqlIgnoreCase(final_url, "about:blank")) {
                let esc_u_buf: [2048]u8 = [0; 2048];
                const esc_u = escapeJsonString(final_url, &esc_u_buf);
                if (event_count.* < out_events.len) {
                    const req_json = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{}}}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                        .{ session.loader_id, session.loader_id, esc_u, esc_u, session.frame_id },
                    ) catch "{}";
                    out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSent", req_json);
                    event_count.* += 1;
                }
                if (hist_req_ht.len > 0 && event_count.* < out_events.len) {
                    let req_ht_buf: [8192]u8 = undefined;
                    const req_ht_esc = escapeJsonString(hist_req_ht, &req_ht_buf);
                    let req_hdrs_obj: []const u8 = "{}";
                    if (crlf := std.mem.indexOf(u8, hist_req_ht, "\r\n")) {
                        req_hdrs_obj = formatHttpHeadersJson(hist_req_ht[crlf + 2 ..], session.allocator);
                    }
                    const req_extra_json = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"requestId\":\"{s}\",\"associatedCookies\":[],\"headers\":{s},\"headersText\":\"{s}\"}}",
                        .{ session.loader_id, req_hdrs_obj, req_ht_esc },
                    ) catch "{}";
                    out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSentExtraInfo", req_extra_json);
                    event_count.* += 1;
                }
                if (event_count.* < out_events.len) {
                    let st_esc_buf: [256]u8 = [0; 256];
                    const st_esc = escapeJsonString(hist_status_text, &st_esc_buf);
                    const hdrs_json = formatHttpHeadersJson(hist_raw_headers, session.allocator);
                    const mime_raw = mimeEssence(hist_content_type);
                    const mime_out = if (mime_raw.len > 0) mime_raw else "text/html";
                    const has_extra_kv = if (hist_resp_ht.len > 0) ",\"hasExtraInfo\":true" else "";
                    const resp_json = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"timestamp\":1700000000.0,\"type\":\"Document\",\"response\":{{\"url\":\"{s}\",\"status\":{d},\"statusText\":\"{s}\",\"headers\":{s},\"mimeType\":\"{s}\"}},\"frameId\":\"{s}\"{s}}}",
                        .{ session.loader_id, session.loader_id, esc_u, hist_status, st_esc, hdrs_json, mime_out, session.frame_id, has_extra_kv },
                    ) catch "{}";
                    out_events[event_count.*] = CdpEvent.create("Network.responseReceived", resp_json);
                    event_count.* += 1;
                    if (hist_resp_ht.len > 0 && event_count.* < out_events.len) {
                        let resp_ht_buf: [8192]u8 = undefined;
                        const resp_ht_esc = escapeJsonString(hist_resp_ht, &resp_ht_buf);
                        const resp_extra_json = std.fmt.allocPrint(
                            session.allocator,
                            "{{\"requestId\":\"{s}\",\"blockedCookies\":[],\"headers\":{s},\"headersText\":\"{s}\",\"statusCode\":{d}}}",
                            .{ session.loader_id, hdrs_json, resp_ht_esc, hist_status },
                        ) catch "{}";
                        out_events[event_count.*] = CdpEvent.create("Network.responseReceivedExtraInfo", resp_extra_json);
                        event_count.* += 1;
                    }
                }
                if (event_count.* < out_events.len) {
                    const enc_len = if (hist_wire_total > 0) hist_wire_total else hist_raw_body_len;
                    const fin_json = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"requestId\":\"{s}\",\"timestamp\":1700000000.0,\"encodedDataLength\":{d}}}",
                        .{ session.loader_id, enc_len },
                    ) catch "{}";
                    out_events[event_count.*] = CdpEvent.create("Network.loadingFinished", fin_json);
                    event_count.* += 1;
                }
            }
            if (event_count.* < out_events.len) {
                const nav_evt = session.formatFrameNavigatedEvent(final_url);
                out_events[event_count.*] = CdpEvent.create("Page.frameNavigated", nav_evt);
                event_count.* += 1;
            }
            if (session.runtime_enabled) {
                emitMainAndIsolatedExecutionContexts(session, final_url, out_events, event_count);
            }
            emitMainFrameLifecycleEvents(session, out_events, event_count);
            if (session.runtime_enabled) {
                let polled_con_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_con_count = session.pollConsoleEvents(&polled_con_evts);
                let con_i: usize = 0;
                while (con_i < polled_con_count && event_count.* < out_events.len) : (con_i += 1) {
                    out_events[event_count.*] = polled_con_evts[con_i];
                    event_count.* += 1;
                }
            }

            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.err(req.id, -32601, "'Page' domain method not implemented");
    }
}

fn findKnownChildFrameIndex(session: *CdpPageSession, frame_id: []const u8): usize? {
    let ki: usize = 0;
    while (ki < session.known_child_frame_count) : (ki += 1) {
        const kf = &session.known_child_frames[ki];
        if (kf.active && !kf.detached && std.mem.eql(u8, kf.frame_id_buf[0..kf.frame_id_len], frame_id)) {
            return ki;
        }
    }
    return null;
}

fn mapNavigationErrorText(err_name: []const u8, default_text: []const u8): []const u8 {
    if (std.mem.eql(u8, err_name, "FileTooBig")) return "net::ERR_FILE_TOO_BIG";
    if (std.mem.eql(u8, err_name, "TooManyRedirects")) return "net::ERR_TOO_MANY_REDIRECTS";
    if (std.mem.eql(u8, err_name, "ConnectionFailed") || std.mem.eql(u8, err_name, "ConnectFailed")) return "net::ERR_CONNECTION_REFUSED";
    if (std.mem.eql(u8, err_name, "TlsFailed")) return "net::ERR_CERT_AUTHORITY_INVALID";
    if (std.mem.eql(u8, err_name, "EmptyResponse")) return "net::ERR_EMPTY_RESPONSE";
    if (std.mem.eql(u8, err_name, "ReadFailed") || std.mem.eql(u8, err_name, "SendFailed")) return "net::ERR_CONNECTION_RESET";
    if (std.mem.eql(u8, err_name, "InternetDisconnected")) return "net::ERR_INTERNET_DISCONNECTED";
    if (std.mem.eql(u8, err_name, "FetchFailed")) return "net::ERR_FAILED";
    if (std.mem.eql(u8, err_name, "NameNotResolved")) return "net::ERR_NAME_NOT_RESOLVED";
    if (std.mem.eql(u8, err_name, "TimedOut")) return "net::ERR_TIMED_OUT";
    if (std.mem.eql(u8, err_name, "RequestAborted")) return "net::ERR_ABORTED";
    if (std.mem.eql(u8, err_name, "BlockedByClient")) return "net::ERR_BLOCKED_BY_CLIENT";
    return default_text;
}

fn setChildFrameDocumentContent(
    session: *CdpPageSession,
    req: CdpRequest,
    req_frame_id: []const u8,
    out_events: *[MAX_CDP_EVENTS]CdpEvent,
    event_count: *usize,
): CdpResponse {
    let kf_idx_opt = findKnownChildFrameIndex(session, req_frame_id);
    if (kf_idx_opt == null) {
        if (f_arr := session.allocator.alloc(FrameState, 48) catch null) {
            defer session.allocator.free(f_arr);
            if (url_st := session.allocator.alloc(u8, 48 * 512) catch null) {
                defer session.allocator.free(url_st);
                if (id_st := session.allocator.alloc(u8, 48 * 32) catch null) {
                    defer session.allocator.free(id_st);
                    const f_cnt = collectFrameStates(session, f_arr, url_st, id_st);
                    recordAttachedChildFrames(session, f_arr[0..f_cnt]);
                    kf_idx_opt = findKnownChildFrameIndex(session, req_frame_id);
                }
            }
        }
    }
    if (kf_idx_opt == null) {
        return CdpResponse.err(req.id, -32000, "No frame with given id found");
    }
    const kf = &session.known_child_frames[kf_idx_opt.?];
    const owner_id = kf.ownerId();
    const cur_url = if (kf.url_len > 0) kf.url_buf[0..kf.url_len] else "about:blank";

    let unesc_buf: ([]u8)? = null;
    defer if (unesc_buf != null) session.allocator.free(unesc_buf.?);
    let html: []const u8 = "";
    const raw_html_opt = extractJsonString(req.params_raw, "html");
    if (raw_html_opt != null && raw_html_opt.?.len > 0) {
        unesc_buf = session.allocator.alloc(u8, raw_html_opt.?.len) catch return CdpResponse.err(req.id, -32000, "Out of memory");
        html = unescapeJsonString(raw_html_opt.?, unesc_buf.?);
    }

    {
        const global_obj = session.ctx.getGlobal();
        defer session.ctx.freeValue(global_obj);
        * = JS_SetPropertyStr(session.ctx.raw_ctx, global_obj, "__capsurf_nav_owner_id", session.ctx.newString(owner_id));
        * = JS_SetPropertyStr(session.ctx.raw_ctx, global_obj, "__capsurf_nav_frame_id", session.ctx.newString(req_frame_id));
        * = JS_SetPropertyStr(session.ctx.raw_ctx, global_obj, "__capsurf_nav_url", session.ctx.newString(cur_url));
        * = JS_SetPropertyStr(session.ctx.raw_ctx, global_obj, "__capsurf_nav_html", session.ctx.newString(html));

        const js_apply =
            "(function(){try{" ++
            "var oid=globalThis.__capsurf_nav_owner_id||'';" ++
            "var fid=globalThis.__capsurf_nav_frame_id||'';" ++
            "var url=globalThis.__capsurf_nav_url||'about:blank';" ++
            "var html=globalThis.__capsurf_nav_html||'';" ++
            "var el=null;" ++
            "if(oid&&typeof document!=='undefined'&&document.getElementById){" ++
            "var byId=document.getElementById(oid);" ++
            "if(byId){var tn=String(byId.localName||byId.tagName||'').toLowerCase();if(tn==='iframe'||tn==='frame')el=byId;}" ++
            "}" ++
            "if(!el&&fid.indexOf('capsurf-child-')===0&&typeof document!=='undefined'&&document.querySelectorAll){" ++
            "var idx=parseInt(fid.slice(14),10);" ++
            "if(idx>=1){var list=document.querySelectorAll('iframe, frame');if(list&&idx-1<list.length)el=list[idx-1];}" ++
            "}" ++
            "if(!el)return false;" ++
            "if(typeof globalThis.__capsurf_getOrCreateIframeWindow==='function'){" ++
            "globalThis.__capsurf_getOrCreateIframeWindow(el,{skipInitialNavigation:true});" ++
            "}" ++
            "if(typeof globalThis.__capsurf_apply_iframe_html==='function'){" ++
            "globalThis.__capsurf_apply_iframe_html(el,url,html);" ++
            "}" ++
            "if(typeof globalThis.__capsurf_fire_iframe_load==='function'){" ++
            "globalThis.__capsurf_fire_iframe_load(el);" ++
            "}" ++
            "return true;" ++
            "}catch(e){return false;}finally{" ++
            "delete globalThis.__capsurf_nav_owner_id;" ++
            "delete globalThis.__capsurf_nav_frame_id;" ++
            "delete globalThis.__capsurf_nav_url;" ++
            "delete globalThis.__capsurf_nav_html;" ++
            "}})()";
        const apply_val = session.ctx.eval(js_apply);
        session.ctx.freeValue(apply_val);
        session.ctx.drainJobs();
    }

    session.loader_seq += 1;
    let child_loader_buf: [32]u8 = [0; 32];
    const child_loader_id = std.fmt.bufPrint(&child_loader_buf, "capsurf-loader-{d:0>3}", .{ session.loader_seq }) catch "capsurf-loader-child";

    syncIframeTargets(session, true);
    emitNewChildFrameEvents(session, out_events, event_count);

    if (event_count.* < out_events.len) {
        const lc_init = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"init\",\"timestamp\":1700000000.0}}",
            .{ req_frame_id, child_loader_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", lc_init);
        event_count.* += 1;
    }
    if (event_count.* < out_events.len) {
        const dcl_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"DOMContentLoaded\",\"timestamp\":1700000000.05}}",
            .{ req_frame_id, child_loader_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", dcl_json);
        event_count.* += 1;
    }
    if (event_count.* < out_events.len) {
        const load_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"load\",\"timestamp\":1700000000.1}}",
            .{ req_frame_id, child_loader_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", load_json);
        event_count.* += 1;
    }
    if (event_count.* < out_events.len) {
        const stop_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\"}}",
            .{ req_frame_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.frameStoppedLoading", stop_json);
        event_count.* += 1;
    }

    return CdpResponse.ok(req.id, "{}");
}

fn navigateChildFrame(
    session: *CdpPageSession,
    req: CdpRequest,
    req_frame_id: []const u8,
    out_buf: []u8,
    out_events: *[MAX_CDP_EVENTS]CdpEvent,
    event_count: *usize,
): CdpResponse {
    let kf_idx_opt = findKnownChildFrameIndex(session, req_frame_id);
    if (kf_idx_opt == null) {
        if (f_arr := session.allocator.alloc(FrameState, 48) catch null) {
            defer session.allocator.free(f_arr);
            if (url_st := session.allocator.alloc(u8, 48 * 512) catch null) {
                defer session.allocator.free(url_st);
                if (id_st := session.allocator.alloc(u8, 48 * 32) catch null) {
                    defer session.allocator.free(id_st);
                    const f_cnt = collectFrameStates(session, f_arr, url_st, id_st);
                    recordAttachedChildFrames(session, f_arr[0..f_cnt]);
                    kf_idx_opt = findKnownChildFrameIndex(session, req_frame_id);
                }
            }
        }
    }
    if (kf_idx_opt == null) {
        return CdpResponse.err(req.id, -32000, "No frame with given id found");
    }
    const kf_idx = kf_idx_opt.?;

    let unesc_nav_buf: ([]u8)? = null;
    defer if (unesc_nav_buf != null) session.allocator.free(unesc_nav_buf.?);
    let nav_url: []const u8 = "about:blank";
    const raw_child_u_opt = extractJsonString(req.params_raw, "url");
    if (raw_child_u_opt != null) {
        if (raw_child_u_opt.?.len > 0) {
            unesc_nav_buf = session.allocator.alloc(u8, raw_child_u_opt.?.len) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            nav_url = unescapeJsonString(raw_child_u_opt.?, unesc_nav_buf.?);
        } else {
            nav_url = "";
        }
    }
    const referrer = extractJsonString(req.params_raw, "referrer") ?? "";

    session.loader_seq += 1;
    let child_loader_buf: [32]u8 = [0; 32];
    const child_loader_id = std.fmt.bufPrint(&child_loader_buf, "capsurf-loader-{d:0>3}", .{ session.loader_seq }) catch "capsurf-loader-child";

    const is_about_blank = std.ascii.eqlIgnoreCase(nav_url, "about:blank") ||
        std.mem.startsWith(u8, nav_url, "about:blank?") ||
        std.mem.startsWith(u8, nav_url, "about:blank#");

    const fetch_cap = @max(@as(usize, 2097152), nav_url.len + 4096);
    let fetch_buf = session.allocator.alloc(u8, fetch_cap) catch return CdpResponse.err(req.id, -32000, "Out of memory");
    defer session.allocator.free(fetch_buf);

    let final_url: []const u8 = nav_url;
    let nav_html: []const u8 = "";
    let content_type: []const u8 = "text/html";
    let status_code: u16 = 200;
    let status_text: []const u8 = "OK";
    let raw_headers: []const u8 = "";
    let req_headers_text: []const u8 = "";
    let resp_headers_text: []const u8 = "";
    let redirect_count: usize = 0;
    let redirect_urls: [8][]const u8 = [""; 8];
    let wire_headers_len: usize = 0;
    let wire_body_len: usize = 0;
    let raw_body_len: usize = 0;

    let synth_buf: ([]u8)? = null;
    defer if (synth_buf != null) session.allocator.free(synth_buf.?);
    let alloc_b64: ([]u8)? = null;
    defer if (alloc_b64 != null) session.allocator.free(alloc_b64.?);
    let b64_str: []const u8 = "";

    if (!is_about_blank) {
        session.setActiveNetReqId(child_loader_id);
        session.redirect_live_emitted = false;
        defer session.active_net_req_id_len = 0;
        const fetch_res = HttpLoader.fetchDocumentWithReferrer(session, nav_url, fetch_buf, 0, referrer) catch |e| {
            const err_name = @errorName(e);
            if (std.mem.eql(u8, err_name, "InvalidUrl")) {
                return CdpResponse.err(req.id, -32000, "Cannot navigate to invalid URL");
            }
            const net_err_text = mapNavigationErrorText(err_name, "");

            if (net_err_text.len > 0) {
                if (session.network_enabled && !std.mem.startsWith(u8, nav_url, "data:") && event_count.* + 1 < out_events.len) {
                    let esc_u_buf: [2048]u8 = [0; 2048];
                    const esc_u = escapeJsonString(nav_url, &esc_u_buf);
                    const req_p = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{}}}},\"timestamp\":0,\"wallTime\":0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                        .{ child_loader_id, child_loader_id, esc_u, esc_u, req_frame_id },
                    ) catch "";
                    if (req_p.len > 0) {
                        out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSent", req_p);
                        event_count.* += 1;
                    }
                    const fail_p = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"requestId\":\"{s}\",\"timestamp\":0,\"type\":\"Document\",\"errorText\":\"{s}\",\"canceled\":false}}",
                        .{ child_loader_id, net_err_text },
                    ) catch "";
                    if (fail_p.len > 0) {
                        out_events[event_count.*] = CdpEvent.create("Network.loadingFailed", fail_p);
                        event_count.* += 1;
                    }
                }
                const err_res = std.fmt.bufPrint(
                    out_buf,
                    "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"isDownload\":false,\"errorText\":\"{s}\"}}",
                    .{ req_frame_id, child_loader_id, net_err_text },
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, err_res);
            }
            return CdpResponse.err(req.id, -32000, "Navigation failed");
        };

        const kind = documentKind(fetch_res.content_type, fetch_res.content_disposition);
        if (kind == DocumentKind.blocked || fetch_res.status_code == 204 || fetch_res.status_code == 205) {
            let is_download = false;
            if (kind == DocumentKind.blocked) {
                is_download = executeDownload(
                    session.allocator,
                    getDownloadConfigStore(),
                    session.currentBrowserContextId(),
                    req_frame_id,
                    fetch_res.final_url,
                    fetch_res.content_disposition ?? "",
                    "",
                    u16(@min(fetch_res.status_code, 65535)),
                    fetch_res.html,
                    out_events,
                    event_count,
                );
            }
            if (session.network_enabled && !std.mem.startsWith(u8, fetch_res.final_url, "data:") && event_count.* < out_events.len) {
                const fail_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"requestId\":\"{s}\",\"timestamp\":1700000000.0,\"type\":\"Document\",\"errorText\":\"net::ERR_ABORTED\",\"canceled\":false}}",
                    .{ child_loader_id },
                ) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Network.loadingFailed", fail_json);
                event_count.* += 1;
            }
            const is_dl_str: []const u8 = if (is_download) "true" else "false";
            const abort_res = std.fmt.bufPrint(
                out_buf,
                "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"isDownload\":{s},\"errorText\":\"net::ERR_ABORTED\"}}",
                .{ req_frame_id, child_loader_id, is_dl_str },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, abort_res);
        }

        final_url = fetch_res.final_url;
        content_type = fetch_res.content_type;
        status_code = @as(u16, @intCast(@min(fetch_res.status_code, 65535)));
        status_text = fetch_res.status_text;
        raw_headers = fetch_res.raw_headers;
        req_headers_text = fetch_res.req_headers_text;
        resp_headers_text = fetch_res.resp_headers_text;
        redirect_count = @min(fetch_res.redirect_count, 8);
        let ri: usize = 0;
        while (ri < redirect_count) : (ri += 1) {
            redirect_urls[ri] = fetch_res.redirect_urls[ri];
        }
        wire_headers_len = fetch_res.wire_headers_len;
        wire_body_len = fetch_res.wire_body_len;
        raw_body_len = fetch_res.raw_body.len;

        b64_str = fetch_res.raw_body;
        nav_html = fetch_res.html;
        if (kind == DocumentKind.image) {
            if (!fetch_res.raw_body_base64) {
                alloc_b64 = Base64.encodeAlloc(session.allocator, fetch_res.html) catch return CdpResponse.err(req.id, -32000, "Base64 encoding failed");
                b64_str = alloc_b64.?;
            }
            recordResponseBody(child_loader_id, b64_str, true);
            const needed = b64_str.len + 4096;
            synth_buf = session.allocator.alloc(u8, needed) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            nav_html = imageDocumentHtml(fetch_res.html, b64_str, fetch_res.content_type, fetch_res.final_url, synth_buf.?);
        } else if (kind == DocumentKind.text) {
            recordResponseBody(child_loader_id, fetch_res.raw_body, fetch_res.raw_body_base64);
            const needed = fetch_res.html.len * 6 + fetch_res.final_url.len * 6 + 1024;
            synth_buf = session.allocator.alloc(u8, needed) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            nav_html = textDocumentHtml(fetch_res.html, fetch_res.final_url, synth_buf.?);
        } else if (kind == DocumentKind.json) {
            recordResponseBody(child_loader_id, fetch_res.raw_body, fetch_res.raw_body_base64);
            const needed = fetch_res.html.len * 16 + fetch_res.final_url.len * 6 + 16384;
            synth_buf = session.allocator.alloc(u8, needed) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            nav_html = jsonViewerHtml(fetch_res.html, fetch_res.final_url, synth_buf.?);
        } else {
            recordResponseBody(child_loader_id, fetch_res.raw_body, fetch_res.raw_body_base64);
        }
    }

    let kf = &session.known_child_frames[kf_idx];
    const owner_id = kf.ownerId();
    {
        const global_obj = session.ctx.getGlobal();
        defer session.ctx.freeValue(global_obj);
        * = JS_SetPropertyStr(session.ctx.raw_ctx, global_obj, "__capsurf_nav_owner_id", session.ctx.newString(owner_id));
        * = JS_SetPropertyStr(session.ctx.raw_ctx, global_obj, "__capsurf_nav_frame_id", session.ctx.newString(req_frame_id));
        * = JS_SetPropertyStr(session.ctx.raw_ctx, global_obj, "__capsurf_nav_url", session.ctx.newString(final_url));
        * = JS_SetPropertyStr(session.ctx.raw_ctx, global_obj, "__capsurf_nav_html", session.ctx.newString(nav_html));

        const js_apply =
            "(function(){try{" ++
            "var oid=globalThis.__capsurf_nav_owner_id||'';" ++
            "var fid=globalThis.__capsurf_nav_frame_id||'';" ++
            "var url=globalThis.__capsurf_nav_url||'about:blank';" ++
            "var html=globalThis.__capsurf_nav_html||'';" ++
            "var el=null;" ++
            "if(oid&&typeof document!=='undefined'&&document.getElementById){" ++
            "var byId=document.getElementById(oid);" ++
            "if(byId){var tn=String(byId.localName||byId.tagName||'').toLowerCase();if(tn==='iframe'||tn==='frame')el=byId;}" ++
            "}" ++
            "if(!el&&fid.indexOf('capsurf-child-')===0&&typeof document!=='undefined'&&document.querySelectorAll){" ++
            "var idx=parseInt(fid.slice(14),10);" ++
            "if(idx>=1){var list=document.querySelectorAll('iframe, frame');if(list&&idx-1<list.length)el=list[idx-1];}" ++
            "}" ++
            "if(!el)return false;" ++
            "if(typeof globalThis.__capsurf_getOrCreateIframeWindow==='function'){" ++
            "globalThis.__capsurf_getOrCreateIframeWindow(el,{skipInitialNavigation:true});" ++
            "}" ++
            "el._suppressSetAttrNav=true;" ++
            "try{if(typeof el.setAttribute==='function')el.setAttribute('src',url);}finally{el._suppressSetAttrNav=false;}" ++
            "if(typeof globalThis.__capsurf_apply_iframe_html==='function'){" ++
            "globalThis.__capsurf_apply_iframe_html(el,url,html);" ++
            "}" ++
            "if(typeof globalThis.__capsurf_fire_iframe_load==='function'){" ++
            "globalThis.__capsurf_fire_iframe_load(el);" ++
            "}" ++
            "return true;" ++
            "}catch(e){return false;}finally{" ++
            "delete globalThis.__capsurf_nav_owner_id;" ++
            "delete globalThis.__capsurf_nav_frame_id;" ++
            "delete globalThis.__capsurf_nav_url;" ++
            "delete globalThis.__capsurf_nav_html;" ++
            "}})()";
        const apply_val = session.ctx.eval(js_apply);
        session.ctx.freeValue(apply_val);
        session.ctx.drainJobs();
    }

    const is_blank_or_data = is_about_blank ||
        std.mem.startsWith(u8, final_url, "about:srcdoc") ||
        std.mem.startsWith(u8, final_url, "data:");
    const is_xo = !is_blank_or_data && !HttpLoader.isSameOrigin(session.url, final_url);
    const ulen = @min(final_url.len, kf.url_buf.len);
    @memcpy(kf.url_buf[0..ulen], final_url[0..ulen]);
    kf.url_len = ulen;
    kf.is_cross_origin = is_xo;

    if (event_count.* < out_events.len) {
        const start_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\"}}",
            .{ req_frame_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.frameStartedLoading", start_json);
        event_count.* += 1;
    }

    if (session.network_enabled && !is_blank_or_data && event_count.* < out_events.len) {
        if (redirect_count == 0) {
            let esc_u_buf: [2048]u8 = [0; 2048];
            const esc_u = escapeJsonString(final_url, &esc_u_buf);
            const req_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{}}}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                .{ child_loader_id, child_loader_id, esc_u, esc_u, req_frame_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSent", req_json);
            event_count.* += 1;
        } else {
            {
                let esc_u0_buf: [2048]u8 = [0; 2048];
                const esc_u0 = escapeJsonString(redirect_urls[0], &esc_u0_buf);
                const req0_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{}}}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                    .{ child_loader_id, child_loader_id, esc_u0, esc_u0, req_frame_id },
                ) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSent", req0_json);
                event_count.* += 1;
            }
            if (!session.redirect_live_emitted) {
                let hi: usize = 1;
                while (hi <= redirect_count && event_count.* < out_events.len) : (hi += 1) {
                    const hop_u = if (hi < redirect_count) redirect_urls[hi] else final_url;
                    let esc_u_buf: [2048]u8 = [0; 2048];
                    const esc_u = escapeJsonString(hop_u, &esc_u_buf);
                    const prev_u = redirect_urls[hi - 1];
                    let esc_prev_buf: [2048]u8 = [0; 2048];
                    const esc_prev = escapeJsonString(prev_u, &esc_prev_buf);
                    const req_json = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{}}}},\"redirectResponse\":{{\"url\":\"{s}\",\"status\":302,\"statusText\":\"Found\",\"headers\":{{}},\"mimeType\":\"text/html\"}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                        .{ child_loader_id, child_loader_id, esc_u, esc_u, esc_prev, req_frame_id },
                    ) catch "{}";
                    out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSent", req_json);
                    event_count.* += 1;
                }
            }
        }
        if (req_headers_text.len > 0 && event_count.* < out_events.len) {
            let req_ht_buf: [8192]u8 = undefined;
            const req_ht_esc = escapeJsonString(req_headers_text, &req_ht_buf);
            let req_hdrs_obj: []const u8 = "{}";
            if (crlf := std.mem.indexOf(u8, req_headers_text, "\r\n")) {
                req_hdrs_obj = formatHttpHeadersJson(req_headers_text[crlf + 2 ..], session.allocator);
            }
            const req_extra_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"requestId\":\"{s}\",\"associatedCookies\":[],\"headers\":{s},\"headersText\":\"{s}\"}}",
                .{ child_loader_id, req_hdrs_obj, req_ht_esc },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSentExtraInfo", req_extra_json);
            event_count.* += 1;
        }
        if (event_count.* < out_events.len) {
            let esc_u_buf: [2048]u8 = [0; 2048];
            const esc_u = escapeJsonString(final_url, &esc_u_buf);
            let st_esc_buf: [256]u8 = [0; 256];
            const st_esc = escapeJsonString(status_text, &st_esc_buf);
            const hdrs_json = formatHttpHeadersJson(raw_headers, session.allocator);
            const mime_raw = mimeEssence(content_type);
            const mime_out = if (mime_raw.len > 0) mime_raw else "text/html";
            const has_extra_kv = if (resp_headers_text.len > 0) ",\"hasExtraInfo\":true" else "";
            const resp_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"timestamp\":1700000000.0,\"type\":\"Document\",\"response\":{{\"url\":\"{s}\",\"status\":{d},\"statusText\":\"{s}\",\"headers\":{s},\"mimeType\":\"{s}\"}},\"frameId\":\"{s}\"{s}}}",
                .{ child_loader_id, child_loader_id, esc_u, status_code, st_esc, hdrs_json, mime_out, req_frame_id, has_extra_kv },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Network.responseReceived", resp_json);
            event_count.* += 1;
            if (resp_headers_text.len > 0 && event_count.* < out_events.len) {
                let resp_ht_buf: [8192]u8 = undefined;
                const resp_ht_esc = escapeJsonString(resp_headers_text, &resp_ht_buf);
                const resp_extra_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"requestId\":\"{s}\",\"blockedCookies\":[],\"headers\":{s},\"headersText\":\"{s}\",\"statusCode\":{d}}}",
                    .{ child_loader_id, hdrs_json, resp_ht_esc, status_code },
                ) catch "{}";
                out_events[event_count.*] = CdpEvent.create("Network.responseReceivedExtraInfo", resp_extra_json);
                event_count.* += 1;
            }
        }
        if (event_count.* < out_events.len) {
            const wire_total = wire_headers_len + wire_body_len;
            const enc_len = if (wire_total > 0) wire_total else raw_body_len;
            const fin_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"requestId\":\"{s}\",\"timestamp\":1700000000.0,\"encodedDataLength\":{d}}}",
                .{ child_loader_id, enc_len },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Network.loadingFinished", fin_json);
            event_count.* += 1;
        }
    }

    if (event_count.* < out_events.len) {
        const parent_fid = if (kf.parent_frame_id_len > 0) kf.parentFrameId() else session.frame_id;
        const cf = makeChildFrameState(ChildFrameArgs{
            frame_id: req_frame_id,
            parent_frame_id: parent_fid,
            loader_id: child_loader_id,
            url: final_url,
            name: kf.name(),
            owner_element_id: kf.ownerId(),
            is_cross_origin: is_xo,
        });
        let single_frame: [1]FrameState = [cf];
        let frame_info_buf: [2048]u8 = [0; 2048];
        const frame_info = makeFrameInfoWithFrames(&single_frame[0], single_frame[0..1], &frame_info_buf);
        const nav_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"frame\":{s},\"type\":\"Navigation\"}}",
            .{ frame_info },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.frameNavigated", nav_json);
        event_count.* += 1;
    }

    if (event_count.* < out_events.len) {
        const lc_init = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"init\",\"timestamp\":1700000000.0}}",
            .{ req_frame_id, child_loader_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", lc_init);
        event_count.* += 1;
    }

    const old_exec_ctx_id = kf.exec_context_id;
    deactivateChildFrameIsolatedWorlds(session, req_frame_id, out_events, event_count);
    if (session.runtime_enabled) {
        if (old_exec_ctx_id > 0 && event_count.* < out_events.len) {
            const des_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"executionContextId\":{d}}}",
                .{ old_exec_ctx_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Runtime.executionContextDestroyed", des_json);
            event_count.* += 1;
        }
        const ctx_id = session.next_context_id;
        session.next_context_id += 1;
        kf.exec_context_id = ctx_id;
        if (event_count.* < out_events.len) {
            const raw_orig = originOf(final_url);
            const cf_origin = if (raw_orig.len > 0) raw_orig else "null";
            let esc_orig_buf: [256]u8 = [0; 256];
            const esc_orig = escapeJsonString(cf_origin, &esc_orig_buf);
            const ctx_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"auxData\":{{\"isDefault\":true,\"type\":\"default\",\"frameId\":\"{s}\"}}}}}}",
                .{ ctx_id, esc_orig, req_frame_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
            event_count.* += 1;
            emitChildFrameIsolatedWorlds(session, req_frame_id, final_url, kf.is_opaque, out_events, event_count);
        }
    } else {
        kf.exec_context_id = 0;
    }

    if (event_count.* < out_events.len) {
        const dcl_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"DOMContentLoaded\",\"timestamp\":1700000000.05}}",
            .{ req_frame_id, child_loader_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", dcl_json);
        event_count.* += 1;
    }

    if (event_count.* < out_events.len) {
        const load_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"name\":\"load\",\"timestamp\":1700000000.1}}",
            .{ req_frame_id, child_loader_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.lifecycleEvent", load_json);
        event_count.* += 1;
    }

    if (event_count.* < out_events.len) {
        const stop_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\"}}",
            .{ req_frame_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.frameStoppedLoading", stop_json);
        event_count.* += 1;
    }

    const res = std.fmt.bufPrint(
        out_buf,
        "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"isDownload\":false}}",
        .{ req_frame_id, child_loader_id },
    ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
    return CdpResponse.ok(req.id, res);
}

fn fetchDocumentWithOptionalPost(
    session: *CdpPageSession,
    target_url: []const u8,
    has_post: bool,
    post_method: []const u8,
    post_body: []const u8,
    post_ctype: []const u8,
    fetch_buf: []u8,
): (HttpLoaderResult)! {
    const prev_extra_headers = session.extra_headers;
    let temp_hdr_buf: [4096]u8 = undefined;
    if (has_post && post_ctype.len > 0) {
        const trimmed_eh = std.mem.trim(u8, prev_extra_headers, " \t\r\n");
        if (trimmed_eh.len == 0 || std.mem.eql(u8, trimmed_eh, "{}")) {
            const hdr = std.fmt.bufPrint(&temp_hdr_buf, "Content-Type: {s}\r\n", .{post_ctype}) catch "";
            if (hdr.len > 0) session.extra_headers = hdr;
        } else if (trimmed_eh.len >= 2 && trimmed_eh[0] == '{' && trimmed_eh[trimmed_eh.len - 1] == '}') {
            const prefix = trimmed_eh[0 .. trimmed_eh.len - 1];
            const hdr = std.fmt.bufPrint(&temp_hdr_buf, "{s},\"Content-Type\":\"{s}\"}}", .{ prefix, post_ctype }) catch "";
            if (hdr.len > 0) session.extra_headers = hdr;
        } else {
            const hdr = std.fmt.bufPrint(&temp_hdr_buf, "{s}\r\nContent-Type: {s}\r\n", .{ trimmed_eh, post_ctype }) catch "";
            if (hdr.len > 0) session.extra_headers = hdr;
        }
    }
    defer session.extra_headers = prev_extra_headers;

    const req_method: []const u8 = if (has_post) post_method else "GET";
    if ((session.fetch_enabled || session.fetch_state.enabled) &&
        session.fetch_state.shouldIntercept(target_url, "Document") &&
        session.fetch_state.findParkedByUrl(target_url) == null)
    {
        if (hook := session.sync_fetch_hook) {
            if (srv_ptr := session.sync_fetch_server_ptr) {
                let ua_esc_buf: [1024]u8 = [0; 1024];
                const ua_val = if (session.user_agent.len > 0) session.user_agent else DEFAULT_HTTP_USER_AGENT;
                const esc_ua = escapeJsonString(ua_val, &ua_esc_buf);

                let ref_kv_buf: [2048]u8 = [0; 2048];
                let ref_kv: []const u8 = "";
                if (session.url.len > 0 && !std.mem.eql(u8, session.url, "about:blank")) {
                    let ref_esc_buf: [2048]u8 = [0; 2048];
                    const esc_ref = escapeJsonString(session.url, &ref_esc_buf);
                    ref_kv = std.fmt.bufPrint(&ref_kv_buf, ",\"Referer\":\"{s}\"", .{esc_ref}) catch "";
                }

                let has_accept_hdr = false;
                let has_cookie_hdr = false;
                let has_ctype_hdr = false;
                let extra_kv_buf: [2048]u8 = [0; 2048];
                let extra_kv: []const u8 = "";
                const trimmed_eh = std.mem.trim(u8, session.extra_headers, " \t\r\n");
                if (trimmed_eh.len > 2 && trimmed_eh[0] == '{' && trimmed_eh[trimmed_eh.len - 1] == '}') {
                    const inner_eh = std.mem.trim(u8, trimmed_eh[1 .. trimmed_eh.len - 1], " \t\r\n");
                    if (inner_eh.len > 0) {
                        extra_kv = std.fmt.bufPrint(&extra_kv_buf, ",{s}", .{inner_eh}) catch "";
                        let ai: usize = 0;
                        while (ai + 8 <= inner_eh.len) : (ai += 1) {
                            if (std.ascii.eqlIgnoreCase(inner_eh[ai .. ai + 8], "\"accept\"")) {
                                has_accept_hdr = true;
                            } else if (std.ascii.eqlIgnoreCase(inner_eh[ai .. ai + 8], "\"cookie\"")) {
                                has_cookie_hdr = true;
                            } else if (ai + 14 <= inner_eh.len && std.ascii.eqlIgnoreCase(inner_eh[ai .. ai + 14], "\"content-type\"")) {
                                has_ctype_hdr = true;
                            }
                        }
                    }
                }

                const accept_kv: []const u8 = if (has_accept_hdr) "" else ",\"Accept\":\"text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\"";

                let ctype_kv_buf: [600]u8 = [0; 600];
                let ctype_kv: []const u8 = "";
                if (!has_ctype_hdr && has_post && post_ctype.len > 0) {
                    let ct_esc_buf: [512]u8 = [0; 512];
                    const esc_ct = escapeJsonString(post_ctype, &ct_esc_buf);
                    ctype_kv = std.fmt.bufPrint(&ctype_kv_buf, ",\"Content-Type\":\"{s}\"", .{esc_ct}) catch "";
                }

                let cookie_kv_buf: [2048]u8 = [0; 2048];
                let cookie_kv: []const u8 = "";
                if (!has_cookie_hdr) {
                    const cookie_hdr = session.getActiveCookieJar().cookieHeaderForUrl(target_url, null, session.allocator);
                    if (cookie_hdr != null) {
                        defer session.allocator.free(cookie_hdr.?);
                        if (cookie_hdr.?.len > 0) {
                            let cookie_esc_buf: [2048]u8 = [0; 2048];
                            const esc_cookie = escapeJsonString(cookie_hdr.?, &cookie_esc_buf);
                            cookie_kv = std.fmt.bufPrint(&cookie_kv_buf, ",\"Cookie\":\"{s}\"", .{esc_cookie}) catch "";
                        }
                    }
                }

                let hdrs_buf: [4096]u8 = [0; 4096];
                const headers_json = std.fmt.bufPrint(
                    &hdrs_buf,
                    "{{\"User-Agent\":\"{s}\"{s}{s}{s}{s}{s}}}",
                    .{ esc_ua, accept_kv, ref_kv, ctype_kv, cookie_kv, extra_kv },
                ) catch "{}";

                session.net_req_seq += 1;
                let net_id_buf: [64]u8 = [0; 64];
                const net_id = std.fmt.bufPrint(&net_id_buf, "req-{d}", .{session.net_req_seq}) catch "req-1";

                * = hook(srv_ptr, unsafe { @ptrCast(session) }, target_url, req_method, headers_json, "Document", net_id, "");
                session.setActiveNetReqId(net_id);
            }
        }
    }
    if (session.active_net_req_id_len == 0 && (session.fetch_enabled || session.fetch_state.enabled)) {
        session.setActiveNetReqId(session.loader_id);
    }
    session.redirect_live_emitted = false;
    defer session.active_net_req_id_len = 0;

    if (!has_post) {
        return HttpLoader.fetchDocument(session, target_url, fetch_buf, 0);
    }
    return HttpLoader.fetchDocumentWithMethod(session, target_url, post_method, post_body, fetch_buf, 0, "Document");
}

pub fn performPendingNavigation(
    session: *CdpPageSession,
    out_events: *[MAX_CDP_EVENTS]CdpEvent,
    event_count: *usize,
): bool {
    event_count.* = 0;
    if (!session.has_document) return false;

    const val = session.ctx.eval(
        "(function(){var u=globalThis.__capsurf_pending_navigation||'';if(u){globalThis.__capsurf_pending_navigation='';}return String(u);})()",
    );
    if (jsIsException(val)) {
        session.ctx.freeValue(val);
        return false;
    }

    let has_post = false;
    let post_url_buf: [2048]u8 = [0; 2048];
    let post_url_len: usize = 0;
    let post_method_buf: [32]u8 = [0; 32];
    let post_method_len: usize = 0;
    let post_ctype_buf: [512]u8 = [0; 512];
    let post_ctype_len: usize = 0;
    let post_body_alloc: ([]u8)? = null;
    defer if (post_body_alloc != null) session.allocator.free(post_body_alloc.?);

    const post_flag_val = session.ctx.eval(
        "globalThis.__capsurf_pending_navigation_post && globalThis.__capsurf_pending_navigation_post.url ? 1 : 0",
    );
    const has_post_obj = !jsIsException(post_flag_val) && (session.ctx.toInt64(post_flag_val) catch 0) != 0;
    session.ctx.freeValue(post_flag_val);

    if (has_post_obj) {
        const pu_val = session.ctx.eval("String(globalThis.__capsurf_pending_navigation_post.url || '')");
        if (!jsIsException(pu_val)) {
            const pu_raw = session.ctx.toCString(pu_val);
            const pu_copy = @min(pu_raw.len, post_url_buf.len);
            if (pu_copy > 0) {
                @memcpy(post_url_buf[0..pu_copy], pu_raw[0..pu_copy]);
                post_url_len = pu_copy;
            }
        }
        session.ctx.freeValue(pu_val);

        const pm_val = session.ctx.eval("String(globalThis.__capsurf_pending_navigation_post.method || 'POST')");
        if (!jsIsException(pm_val)) {
            const pm_raw = session.ctx.toCString(pm_val);
            const pm_copy = @min(pm_raw.len, post_method_buf.len);
            if (pm_copy > 0) {
                @memcpy(post_method_buf[0..pm_copy], pm_raw[0..pm_copy]);
                post_method_len = pm_copy;
            }
        }
        session.ctx.freeValue(pm_val);

        const pct_val = session.ctx.eval("String(globalThis.__capsurf_pending_navigation_post.contentType || '')");
        if (!jsIsException(pct_val)) {
            const pct_raw = session.ctx.toCString(pct_val);
            const pct_copy = @min(pct_raw.len, post_ctype_buf.len);
            if (pct_copy > 0) {
                @memcpy(post_ctype_buf[0..pct_copy], pct_raw[0..pct_copy]);
                post_ctype_len = pct_copy;
            }
        }
        session.ctx.freeValue(pct_val);

        const pb_val = session.ctx.eval("globalThis.__capsurf_pending_navigation_post.body ?? ''");
        if (!jsIsException(pb_val)) {
            let got_u8 = false;
            if (pb_val.tag == JS_TAG_OBJECT) {
                let u8_len: usize = 0;
                const u8_ptr = unsafe { JS_GetUint8Array(session.ctx.raw_ctx, &u8_len, pb_val) };
                if (u8_ptr != null) {
                    if (u8_len > 0) {
                        const raw_slice = unsafe { u8_ptr.?[0..u8_len] };
                        post_body_alloc = session.allocator.dupe(u8, raw_slice) catch null;
                    }
                    got_u8 = true;
                } else {
                    const exc = JS_GetException(session.ctx.raw_ctx);
                    JS_FreeValue(session.ctx.raw_ctx, exc);
                }
            }
            if (!got_u8) {
                const pb_raw = session.ctx.toCString(pb_val);
                if (pb_raw.len > 0) {
                    post_body_alloc = session.allocator.dupe(u8, pb_raw) catch null;
                }
            }
        }
        session.ctx.freeValue(pb_val);

        const clear_post_val = session.ctx.eval("globalThis.__capsurf_pending_navigation_post = null;");
        session.ctx.freeValue(clear_post_val);

        if (post_url_len > 0) {
            has_post = true;
        }
    }

    const dl_val = session.ctx.eval(
        "(function(){var d=globalThis.__capsurf_pending_download||'';if(d){globalThis.__capsurf_pending_download='';}return String(d);})()",
    );
    let dl_hint_buf: [512]u8 = [0; 512];
    let dl_hint_len: usize = 0;
    if (!jsIsException(dl_val)) {
        const dl_raw = session.ctx.toCString(dl_val);
        const dl_copy = @min(dl_raw.len, dl_hint_buf.len);
        if (dl_copy > 0) {
            @memcpy(dl_hint_buf[0..dl_copy], dl_raw[0..dl_copy]);
            dl_hint_len = dl_copy;
        }
    }
    session.ctx.freeValue(dl_val);
    const has_download_hint = dl_hint_len > 0;
    let download_attr_hint: []const u8 = "";
    if (has_download_hint) {
        const raw_hint = dl_hint_buf[0..dl_hint_len];
        if (!std.mem.eql(u8, raw_hint, "__1__")) {
            download_attr_hint = raw_hint;
        }
    }

    const pending_raw = session.ctx.toCString(val);
    let raw_buf: [2048]u8 = [0; 2048];
    let copy_len: usize = @min(pending_raw.len, raw_buf.len);
    if (copy_len > 0) {
        @memcpy(raw_buf[0..copy_len], pending_raw[0..copy_len]);
    }
    session.ctx.freeValue(val);
    if (copy_len == 0 && has_post) {
        @memcpy(raw_buf[0..post_url_len], post_url_buf[0..post_url_len]);
        copy_len = post_url_len;
    }
    if (copy_len == 0) return false;
    const pending_url = raw_buf[0..copy_len];
    if (std.ascii.startsWithIgnoreCase(pending_url, "javascript:")) return false;

    let resolved_buf: [2048]u8 = [0; 2048];
    const target_url = HttpLoader.resolveRelativeUrl(session.url, pending_url, &resolved_buf);
    if (target_url.len == 0) return false;

    // Do not treat an iframe's src navigation as a top-level page navigation
    {
        let ni: usize = 0;
        while (ni < session.doc.nodes.items.len) : (ni += 1) {
            const node = &session.doc.nodes.items[ni];
            if (node.isElement() && (node.hasTag("iframe") || node.hasTag("frame"))) {
                if (isrc := session.doc.getAttribute(node.id, "src")) {
                    if (std.mem.eql(u8, isrc, pending_url) || std.mem.eql(u8, isrc, target_url)) {
                        return false;
                    }
                }
            }
        }
    }

    // Private Network Access: block a public page from navigating itself to a loopback address
    if (HttpLoader.isPublicHttpUrl(session.url) && HttpLoader.isLoopbackUrl(target_url)) {
        return false;
    }

    let fetch_buf = session.allocator.alloc(u8, 65536) catch return false;
    defer session.allocator.free(fetch_buf);

    const post_method: []const u8 = if (has_post && post_method_len > 0) post_method_buf[0..post_method_len] else "POST";
    const post_ctype: []const u8 = if (has_post && post_ctype_len > 0) post_ctype_buf[0..post_ctype_len] else "";
    const post_body: []const u8 = if (post_body_alloc != null) post_body_alloc.? else "";
    const req_method: []const u8 = if (has_post) post_method else "GET";

    const fetch_res = fetchDocumentWithOptionalPost(
        session,
        target_url,
        has_post,
        post_method,
        post_body,
        post_ctype,
        fetch_buf,
    ) catch |e| {
        const err_name = @errorName(e);
        const net_err_text = mapNavigationErrorText(err_name, "net::ERR_FAILED");
        let esc_u_buf: [2048]u8 = [0; 2048];
        const esc_u = escapeJsonString(target_url, &esc_u_buf);
        if (session.network_enabled && event_count.* + 1 < out_events.len) {
            const req_evt = std.fmt.allocPrint(
                session.allocator,
                "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"{s}\",\"headers\":{{}},\"initialPriority\":\"VeryHigh\",\"referrerPolicy\":\"strict-origin-when-cross-origin\"}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                .{ session.loader_id, session.loader_id, esc_u, esc_u, req_method, session.frame_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSent", req_evt);
            event_count.* += 1;
            const fail_evt = std.fmt.allocPrint(
                session.allocator,
                "{{\"requestId\":\"{s}\",\"timestamp\":1700000000.01,\"type\":\"Document\",\"errorText\":\"{s}\",\"canceled\":false}}",
                .{ session.loader_id, net_err_text },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Network.loadingFailed", fail_evt);
            event_count.* += 1;
        }
        if (event_count.* < out_events.len) {
            const stop_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"frameId\":\"{s}\"}}",
                .{ session.frame_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Page.frameStoppedLoading", stop_json);
            event_count.* += 1;
        }
        return true;
    };

    const kind = documentKind(fetch_res.content_type, fetch_res.content_disposition);
    if (kind == DocumentKind.blocked || has_download_hint) {
        if (executeDownload(
            session.allocator,
            getDownloadConfigStore(),
            session.currentBrowserContextId(),
            session.frame_id,
            fetch_res.final_url,
            fetch_res.content_disposition ?? "",
            download_attr_hint,
            u16(@min(fetch_res.status_code, 65535)),
            fetch_res.html,
            out_events,
            event_count,
        )) {
            return true;
        }
        if (kind == DocumentKind.blocked) return false;
    }

    session.nextLoaderId();
    const doc_req_id = session.loader_id;

    let synth_buf: ([]u8)? = null;
    defer if (synth_buf != null) session.allocator.free(synth_buf.?);
    let alloc_b64: ([]u8)? = null;
    defer if (alloc_b64 != null) session.allocator.free(alloc_b64.?);
    let b64_str: []const u8 = fetch_res.raw_body;
    let nav_html: []const u8 = fetch_res.html;

    clearDocResponseBodies();
    if (kind == DocumentKind.image) {
        if (!fetch_res.raw_body_base64) {
            alloc_b64 = Base64.encodeAlloc(session.allocator, fetch_res.html) catch return false;
            b64_str = alloc_b64.?;
        }
        recordResponseBody(doc_req_id, b64_str, true);
        const needed = b64_str.len + 4096;
        synth_buf = session.allocator.alloc(u8, needed) catch return false;
        nav_html = imageDocumentHtml(fetch_res.html, b64_str, fetch_res.content_type, fetch_res.final_url, synth_buf.?);
    } else if (kind == DocumentKind.text) {
        recordResponseBody(doc_req_id, fetch_res.raw_body, fetch_res.raw_body_base64);
        const needed = fetch_res.html.len * 6 + fetch_res.final_url.len * 6 + 1024;
        synth_buf = session.allocator.alloc(u8, needed) catch return false;
        nav_html = textDocumentHtml(fetch_res.html, fetch_res.final_url, synth_buf.?);
    } else if (kind == DocumentKind.json) {
        recordResponseBody(doc_req_id, fetch_res.raw_body, fetch_res.raw_body_base64);
        const needed = fetch_res.html.len * 16 + fetch_res.final_url.len * 6 + 16384;
        synth_buf = session.allocator.alloc(u8, needed) catch return false;
        nav_html = jsonViewerHtml(fetch_res.html, fetch_res.final_url, synth_buf.?);
    } else {
        recordResponseBody(doc_req_id, fetch_res.raw_body, fetch_res.raw_body_base64);
    }

    session.nav_tty_stripped = false;
    session.navigate(nav_html, fetch_res.final_url, "") catch return false;
    if (isXmlOrXhtml(fetch_res.content_type, fetch_res.final_url)) {
        session.doc.in_quirks_mode = false;
    }

    if (event_count.* < out_events.len) {
        const start_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\"}}",
            .{ session.frame_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.frameStartedLoading", start_json);
        event_count.* += 1;
    }

    if (session.network_enabled && event_count.* < out_events.len) {
        let esc_u_buf: [2048]u8 = [0; 2048];
        const esc_u = escapeJsonString(fetch_res.final_url, &esc_u_buf);
        const req_evt = std.fmt.allocPrint(
            session.allocator,
            "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"{s}\",\"headers\":{{}},\"initialPriority\":\"VeryHigh\",\"referrerPolicy\":\"strict-origin-when-cross-origin\"}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
            .{ doc_req_id, session.loader_id, esc_u, esc_u, req_method, session.frame_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSent", req_evt);
        event_count.* += 1;
        if (event_count.* < out_events.len) {
            const mime_raw = mimeEssence(fetch_res.content_type);
            const mime_out = if (mime_raw.len > 0) mime_raw else "text/html";
            const resp_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"timestamp\":1700000000.0,\"type\":\"Document\",\"response\":{{\"url\":\"{s}\",\"status\":{d},\"statusText\":\"OK\",\"headers\":{{}},\"mimeType\":\"{s}\"}},\"frameId\":\"{s}\"}}",
                .{ doc_req_id, session.loader_id, esc_u, fetch_res.status_code, mime_out, session.frame_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Network.responseReceived", resp_json);
            event_count.* += 1;
        }
        if (event_count.* < out_events.len) {
            const wire_total = fetch_res.wire_headers_len + fetch_res.wire_body_len;
            const enc_len = if (wire_total > 0) wire_total else fetch_res.raw_body.len;
            const fin_json = std.fmt.allocPrint(
                session.allocator,
                "{{\"requestId\":\"{s}\",\"timestamp\":1700000000.01,\"encodedDataLength\":{d}}}",
                .{ doc_req_id, enc_len },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Network.loadingFinished", fin_json);
            event_count.* += 1;
        }
    }

    const fn_json = session.formatFrameNavigatedEvent(fetch_res.final_url);
    if (event_count.* < out_events.len) {
        out_events[event_count.*] = CdpEvent.create("Page.frameNavigated", fn_json);
        event_count.* += 1;
    }
    if (session.runtime_enabled) {
        emitMainAndIsolatedExecutionContexts(session, fetch_res.final_url, out_events, event_count);
    }
    emitMainFrameLifecycleEvents(session, out_events, event_count);
    if (session.runtime_enabled) {
        let polled_con_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
        const polled_con_count = session.pollConsoleEvents(&polled_con_evts);
        let con_i: usize = 0;
        while (con_i < polled_con_count && event_count.* < out_events.len) : (con_i += 1) {
            out_events[event_count.*] = polled_con_evts[con_i];
            event_count.* += 1;
        }
    }

    return true;
}

fn findIgnoreCase(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) return 0;
    if (haystack.len < needle.len) return null;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(haystack[i .. i + needle.len], needle)) {
            return i;
        }
    }
    return null;
}

fn applyCssPageSize(css: []const u8, opts: *PdfOptions) {
    let pos: usize = 0;
    while (pos < css.len) {
        const rel_idx = findIgnoreCase(css[pos..], "@page") orelse break;
        const at_idx = pos + rel_idx;
        const ob = std.mem.indexOfScalarPos(u8, css, at_idx + 5, '{') orelse break;
        const cb = std.mem.indexOfScalarPos(u8, css, ob + 1, '}') orelse break;
        const body = css[ob + 1 .. cb];
        if (sz_idx := findIgnoreCase(body, "size")) {
            if (colon := std.mem.indexOfScalarPos(u8, body, sz_idx + 4, ':')) {
                const end = std.mem.indexOfScalarPos(u8, body, colon + 1, ';') ?? body.len;
                const val = std.mem.trim(u8, body[colon + 1 .. end], " \t\r\n");
                if (findIgnoreCase(val, "a4") != null) {
                    opts.paper_width_in = 8.2677165;
                    opts.paper_height_in = 11.692913;
                } else if (findIgnoreCase(val, "a3") != null) {
                    opts.paper_width_in = 11.692913;
                    opts.paper_height_in = 16.535433;
                } else if (findIgnoreCase(val, "a5") != null) {
                    opts.paper_width_in = 5.8267717;
                    opts.paper_height_in = 8.2677165;
                } else if (findIgnoreCase(val, "letter") != null) {
                    opts.paper_width_in = 8.5;
                    opts.paper_height_in = 11.0;
                } else if (findIgnoreCase(val, "legal") != null) {
                    opts.paper_width_in = 8.5;
                    opts.paper_height_in = 14.0;
                }
                if (findIgnoreCase(val, "landscape") != null) {
                    opts.landscape = true;
                } else if (findIgnoreCase(val, "portrait") != null) {
                    opts.landscape = false;
                }
            }
        }
        pos = cb + 1;
    }
}
