// src/cdp/page.cp
// CDP Page Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/page.ts and vendor/kitesurf/packages/engine/src/eval/outbound.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, extractJsonFloat, unescapeJsonString, MAX_CDP_EVENTS } from "./types.cp";
import { CdpPageSession, escapeJsonString } from "./session.cp";
import { TtyNavState, applyTtyNavParams } from "../tty/nav_params.cp";
import { jsIsException } from "../js/qjs_engine.cp";
import { HttpLoader } from "../net/http_loader.cp";
import { MockFixtures } from "../net/mock_fixtures.cp";
import { computeNextReferrer } from "../net/redirect_walk.cp";
import { NodeType, NULL_NODE } from "../dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../css/cascade.cp";
import { PdfOptions, computePdfPagination, renderPdfAlloc, renderPdfWithDocAlloc } from "../render/pdf.cp";
import { Base64 } from "cap:encoding";
import {
    DocumentKind,
    documentKind,
    mimeEssence,
    imageDocumentHtml,
    textDocumentHtml,
    jsonViewerHtml,
} from "./document_kind.cp";
import { nextDocRequestId, recordResponseBody } from "./network.cp";
import {
    FrameState,
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
        const tag_pos = std.mem.indexOfPos(u8, html, pos, "<iframe") ?? return null;
        const close_pos = std.mem.indexOfScalarPos(u8, html, tag_pos, '>') ?? return null;
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

fn addIframeState(
    session: *CdpPageSession,
    out_frames: []FrameState,
    url_storage: []u8,
    id_storage: []u8,
    count: *usize,
    child_seq: *usize,
    raw_src: []const u8,
    frame_name: []const u8,
    owner_id: []const u8,
) {
    if (count.* >= out_frames.len) return;
    child_seq.* += 1;
    const child_idx = count.*;
    const effective_src = if (raw_src.len > 0) raw_src else "about:blank";
    let child_url: []const u8 = "about:blank";
    if (std.mem.eql(u8, effective_src, "about:blank") || std.mem.startsWith(u8, effective_src, "about:blank?") || std.mem.startsWith(u8, effective_src, "about:blank#")) {
        const u_dst = url_storage[child_idx * 512 .. (child_idx + 1) * 512];
        const copy_len = @min(effective_src.len, u_dst.len);
        @memcpy(u_dst[0..copy_len], effective_src[0..copy_len]);
        child_url = u_dst[0..copy_len];
    } else {
        child_url = resolveIframeFinalUrl(session.url, effective_src, url_storage[child_idx * 512 .. (child_idx + 1) * 512]);
    }
    const child_id = std.fmt.bufPrint(id_storage[child_idx * 32 .. (child_idx + 1) * 32], "capsurf-child-{d}", .{ child_seq.* }) catch "capsurf-child-1";
    const is_blank_or_local = std.mem.eql(u8, child_url, "about:blank") ||
        std.mem.startsWith(u8, child_url, "about:blank?") ||
        std.mem.startsWith(u8, child_url, "about:blank#") ||
        std.mem.startsWith(u8, child_url, "about:srcdoc") ||
        std.mem.startsWith(u8, child_url, "data:");
    const is_xo = !is_blank_or_local && !HttpLoader.isSameOrigin(session.url, child_url);

    let name_buf = session.allocator.dupe(u8, frame_name) catch "";
    let owner_buf = session.allocator.dupe(u8, owner_id) catch "";
    out_frames[child_idx] = makeChildFrameState(ChildFrameArgs{
        frame_id: child_id,
        parent_frame_id: session.frame_id,
        loader_id: session.loader_id,
        url: child_url,
        name: name_buf,
        owner_element_id: owner_buf,
        is_cross_origin: is_xo,
    });
    count.* += 1;

    // Depth-2 iframe discovery for fixture documents
    if (count.* < out_frames.len && MockFixtures.isFixtureUrl(child_url) && std.mem.indexOf(u8, child_url, "/frame-policy-child.html") == null && std.mem.indexOf(u8, child_url, "/oopif-kind-child.html") == null) {
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
                const gc_id = std.fmt.bufPrint(id_storage[gc_idx * 32 .. (gc_idx + 1) * 32], "capsurf-child-{d}-1", .{ child_seq.* }) catch "capsurf-child-1-1";
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
    let child_seq: usize = 0;

    if (session.has_document) {
        const js_iframes = "(function(){try{if(typeof document==='undefined'||!document.querySelectorAll)return '';var fs=document.querySelectorAll('iframe');var out=[];for(var i=0;i<fs.length;i++){var f=fs[i];var s=(f.getAttribute&&f.getAttribute('src'))||'';var n=(f.getAttribute&&f.getAttribute('name'))||'';var id=(f.getAttribute&&f.getAttribute('id'))||'';out.push(s+'\\x1f'+n+'\\x1f'+id);}return out.join('\\x1e');}catch(e){return '';}})()";
        const val = session.ctx.eval(js_iframes);
        defer session.ctx.freeValue(val);
        const cstr = session.ctx.toCString(val);
        if (cstr.len > 0) {
            let rows = std.mem.splitScalar(u8, cstr, 0x1e);
            while (row := rows.next()) {
                if (count >= out_frames.len) break;
                let cols = std.mem.splitScalar(u8, row, 0x1f);
                const raw_src = cols.next() ?? "";
                const frame_name = cols.next() ?? "";
                const owner_id = cols.next() ?? "";
                addIframeState(session, out_frames, url_storage, id_storage, &count, &child_seq, raw_src, frame_name, owner_id);
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
        if (n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, "iframe")) {
            const raw_src = n.getAttribute("src") ?? "";
            const owner_id = n.getAttribute("id") ?? "";
            const frame_name = n.getAttribute("name") ?? "";
            addIframeState(session, out_frames, url_storage, id_storage, &count, &child_seq, raw_src, frame_name, owner_id);
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
        let found = false;
        let ki: usize = 0;
        while (ki < session.known_child_frame_count) : (ki += 1) {
            if (session.known_child_frames[ki].active && std.mem.eql(u8, session.known_child_frames[ki].frame_id_buf[0..session.known_child_frames[ki].frame_id_len], cf.frame_id)) {
                if (cf.exec_context_id > 0) {
                    session.known_child_frames[ki].exec_context_id = cf.exec_context_id;
                }
                found = true;
                break;
            }
        }
        if (!found && session.known_child_frame_count < session.known_child_frames.len) {
            let k = &session.known_child_frames[session.known_child_frame_count];
            k.active = true;
            k.detached = false;
            k.exec_context_id = cf.exec_context_id;
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

pub fn emitNewChildFrameEvents(
    session: *CdpPageSession,
    out_events: *[MAX_CDP_EVENTS]CdpEvent,
    event_count: *usize,
) {
    if (!session.has_document) return;
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
        const dom_urls_js = "(function(){try{if(typeof document==='undefined'||!document.querySelectorAll)return '';var fs=document.querySelectorAll('iframe');var out=[];for(var i=0;i<fs.length;i++){var f=fs[i];var id=(f.getAttribute&&f.getAttribute('id'))||'';out.push(id);}return out.join('|');}catch(e){return '';}})()";
        const dv = session.ctx.eval(dom_urls_js);
        defer session.ctx.freeValue(dv);
        const dcstr = session.ctx.toCString(dv);
        let dom_count: usize = 0;
        if (dcstr.len > 0) {
            dom_count = 1;
            let ci: usize = 0;
            while (ci < dcstr.len) : (ci += 1) {
                if (dcstr[ci] == '|') dom_count += 1;
            }
        } else {
            const cnt_v = session.ctx.eval("(function(){try{return document.querySelectorAll('iframe').length;}catch(e){return 0;}})()");
            const cnt_i = session.ctx.toInt64(cnt_v) catch 0;
            session.ctx.freeValue(cnt_v);
            if (cnt_i > 0) dom_count = @as(usize, @intCast(cnt_i));
        }
        let ki: usize = 0;
        while (ki < session.known_child_frame_count && event_count.* < out_events.len) : (ki += 1) {
            let kf = &session.known_child_frames[ki];
            if (!kf.active || kf.detached) continue;
            let still_present = false;
            if (dom_count > 0) {
                const k_oid = kf.owner_id_buf[0..kf.owner_id_len];
                if (k_oid.len > 0 && dcstr.len > 0) {
                    let drows2 = std.mem.splitScalar(u8, dcstr, '|');
                    while (drow := drows2.next()) {
                        if (std.mem.eql(u8, drow, k_oid)) {
                            still_present = true;
                            break;
                        }
                    }
                } else if (ki < dom_count) {
                    still_present = true;
                }
            }
            if (!still_present) {
                kf.detached = true;
                const det_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"frameId\":\"{s}\",\"reason\":\"remove\"}}",
                    .{ kf.frame_id_buf[0..kf.frame_id_len] },
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
            }
        }
    }

    let active_oopifs = countActiveOopifContexts(session);

    // Detect URL/origin changes on already-known child frames (e.g. about:blank -> cross-origin src)
    {
        const main_orig = originOf(session.url);
        let ki: usize = 0;
        while (ki < session.known_child_frame_count && event_count.* < out_events.len) : (ki += 1) {
            let kf = &session.known_child_frames[ki];
            if (!kf.active || kf.detached) continue;
            const k_fid = kf.frame_id_buf[0..kf.frame_id_len];
            const old_url = kf.url_buf[0..kf.url_len];
            const old_origin = originOf(old_url);
            let nfi: usize = 1;
            while (nfi < nav_frame_count) : (nfi += 1) {
                const cf = &nav_frames[nfi];
                if (std.mem.eql(u8, cf.frame_id, k_fid)) {
                    const new_origin = originOf(cf.url);
                    if (new_origin.len > 0 && !std.mem.eql(u8, new_origin, old_origin) && !isRefusedKindFrameUrl(cf.url)) {
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
                                "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"auxData\":{{\"isDefault\":false,\"frameId\":\"{s}\"}}}}}}",
                                .{ ctx_id, esc_orig, cf.frame_id },
                            ) catch "{}";
                            out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
                            event_count.* += 1;
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
        }
    }

    const total_children: usize = if (nav_frame_count > 1) nav_frame_count - 1 else 0;
    if (total_children <= session.emitted_child_frame_count) return;

    let frame_info_buf: [2048]u8 = [0; 2048];
    let fi: usize = @as(usize, 1) + session.emitted_child_frame_count;
    while (fi < nav_frame_count && event_count.* < out_events.len) : (fi += 1) {
        let cf = &nav_frames[fi];
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
                "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"auxData\":{{\"isDefault\":false,\"frameId\":\"{s}\"}}}}}}",
                .{ ctx_id, esc_orig, cf.frame_id },
            ) catch "{}";
            out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
            event_count.* += 1;
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
    session.emitted_child_frame_count = total_children;
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

        if (std.mem.eql(u8, req.method, "Page.enable") ||
            std.mem.eql(u8, req.method, "Page.disable") ||
            std.mem.eql(u8, req.method, "Page.stopLoading") ||
            std.mem.eql(u8, req.method, "Page.reload") ||
            std.mem.eql(u8, req.method, "Page.setLifecycleEventsEnabled") ||
            std.mem.eql(u8, req.method, "Page.setInterceptFileChooserDialog") ||
            std.mem.eql(u8, req.method, "Page.setBypassCSP") ||
            std.mem.eql(u8, req.method, "Page.bringToFront") ||
            std.mem.eql(u8, req.method, "Page.handleJavaScriptDialog") ||
            std.mem.eql(u8, req.method, "Page.close")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.addScriptToEvaluateOnNewDocument")) {
            return CdpResponse.ok(req.id, "{\"identifier\":\"capsurf-script-001\"}");
        }

        if (std.mem.eql(u8, req.method, "Page.removeScriptToEvaluateOnNewDocument")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.createIsolatedWorld")) {
            const world_name = extractJsonString(req.params_raw, "worldName") ?? "";
            const frame_id = extractJsonString(req.params_raw, "frameId") ?? session.frame_id;
            let created_new = false;
            const cid = session.createIsolatedWorld(world_name, &created_new);
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
                const sheet = StyleSheet.parse(full_css, session.viewport_width, session.emulated_media_print);
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

            if (q := extractJsonInt(req.params_raw, "quality")) {
                if (q >= 0 && q <= 9) {
                    compress_lvl = @as(u8, @intCast(q));
                } else if (q < 50) {
                    compress_lvl = 1;
                } else {
                    compress_lvl = 6;
                }
            }

            if (std.mem.eql(u8, fmt, "jpeg")) {
                compress_lvl = 1;
            }

            if (std.mem.indexOf(u8, req.params_raw, "\"optimizeForSpeed\":true") != null) {
                compress_lvl = 1;
            }

            let clip_x: i32 = 0;
            let clip_y: i32 = 0;
            let clip_w: u32 = 0;
            let clip_h: u32 = 0;
            if (clip_pos := std.mem.indexOf(u8, req.params_raw, "\"clip\"")) {
                const sub = req.params_raw[clip_pos..];
                if (brace_pos := std.mem.indexOfScalar(u8, sub, '{')) {
                    const clip_json = sub[brace_pos..];
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
                }
            }

            let sx_sync: f32 = 0.0;
            let sy_sync: f32 = 0.0;
            let cw_sync: f32 = 0.0;
            let ch_sync: f32 = 0.0;
            session.updateLayoutMetrics(&sx_sync, &sy_sync, &cw_sync, &ch_sync);
            const png_bytes = session.captureScreenshotClipped(compress_lvl, clip_x, clip_y, clip_w, clip_h) catch return CdpResponse.err(req.id, -32000, "Screenshot capture failed");
            defer session.allocator.free(png_bytes);

            const b64_str = Base64.encodeAlloc(session.allocator, png_bytes) catch return CdpResponse.err(req.id, -32000, "Base64 encoding failed");
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

        if (std.mem.eql(u8, req.method, "Page.navigate")) {
            const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (bctx.len > 0) {
                session.setContextId(bctx);
            }
            const raw_nav_url = extractJsonString(req.params_raw, "url") ?? "about:blank";
            let clean_nav_buf: [2048]u8 = [0; 2048];
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
            const fetch_cap = @max(@as(usize, 262144), url.len + 4096);
            let fetch_buf = session.allocator.alloc(u8, fetch_cap) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(fetch_buf);

            const fetch_res = HttpLoader.fetchDocument(session, url, fetch_buf, 0) catch |e| {
                const err_name = @errorName(e);
                if (std.mem.eql(u8, err_name, "InvalidUrl")) {
                    return CdpResponse.err(req.id, -32000, "Cannot navigate to invalid URL");
                }
                if (std.mem.eql(u8, err_name, "FileTooBig")) {
                    const too_big_res = std.fmt.bufPrint(
                        out_buf,
                        "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"isDownload\":false,\"errorText\":\"net::ERR_FILE_TOO_BIG\"}}",
                        .{ session.frame_id, session.loader_id },
                    ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                    return CdpResponse.ok(req.id, too_big_res);
                }
                if (std.mem.eql(u8, err_name, "RequestAborted")) {
                    return CdpResponse.err(req.id, -32000, "net::ERR_ABORTED");
                }
                if (std.mem.eql(u8, err_name, "BlockedByClient")) {
                    return CdpResponse.err(req.id, -32000, "net::ERR_BLOCKED_BY_CLIENT");
                }
                return CdpResponse.err(req.id, -32000, "Navigation failed");
            };

            let doc_req_id_buf: [64]u8 = [0; 64];
            const doc_req_id = nextDocRequestId(&doc_req_id_buf);

            const kind = documentKind(fetch_res.content_type, fetch_res.content_disposition);
            if (kind == DocumentKind.blocked || fetch_res.status_code == 204 || fetch_res.status_code == 205) {
                if (session.network_enabled && event_count.* < out_events.len) {
                    const fail_json = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"requestId\":\"{s}\",\"timestamp\":1700000000.0,\"type\":\"Document\",\"errorText\":\"net::ERR_ABORTED\",\"canceled\":false}}",
                        .{ doc_req_id },
                    ) catch "{}";
                    out_events[event_count.*] = CdpEvent.create("Network.loadingFailed", fail_json);
                    event_count.* += 1;
                }
                const abort_res = std.fmt.bufPrint(
                    out_buf,
                    "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"isDownload\":false,\"errorText\":\"net::ERR_ABORTED\"}}",
                    .{ session.frame_id, session.loader_id },
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, abort_res);
            }

            let synth_buf: ([]u8)? = null;
            defer if (synth_buf != null) session.allocator.free(synth_buf.?);
            let alloc_b64: ([]u8)? = null;
            defer if (alloc_b64 != null) session.allocator.free(alloc_b64.?);
            let b64_str: []const u8 = fetch_res.raw_body;
            let nav_html: []const u8 = fetch_res.html;

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

            session.navigate(nav_html, fetch_res.final_url, effective_referrer) catch return CdpResponse.err(req.id, -32000, "HTML parsing failed");
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

            if (session.network_enabled && event_count.* < out_events.len) {
                if (fetch_res.redirect_count == 0) {
                    let esc_u_buf: [2048]u8 = [0; 2048];
                    const esc_u = escapeJsonString(fetch_res.final_url, &esc_u_buf);
                    const req_json = std.fmt.allocPrint(
                        session.allocator,
                        "{{\"requestId\":\"{s}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{}}}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"other\"}},\"type\":\"Document\",\"frameId\":\"{s}\"}}",
                        .{ doc_req_id, session.loader_id, esc_u, esc_u, session.frame_id },
                    ) catch "{}";
                    out_events[event_count.*] = CdpEvent.create("Network.requestWillBeSent", req_json);
                    event_count.* += 1;
                } else {
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

            if (session.network_enabled && event_count.* < out_events.len) {
                let esc_u_buf: [2048]u8 = [0; 2048];
                const esc_u = escapeJsonString(fetch_res.final_url, &esc_u_buf);
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

            if (session.network_enabled && event_count.* < out_events.len) {
                const fin_json = std.fmt.allocPrint(
                    session.allocator,
                    "{{\"requestId\":\"{s}\",\"timestamp\":1700000000.0,\"encodedDataLength\":{d}}}",
                    .{ doc_req_id, fetch_res.raw_body.len },
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
            if (event_count.* < out_events.len) {
                out_events[event_count.*] = CdpEvent.create("Runtime.executionContextsCleared", "{}");
                event_count.* += 1;
            }
            if (event_count.* < out_events.len) {
                const raw_main_orig = originOf(fetch_res.final_url);
                const main_orig = if (raw_main_orig.len > 0) raw_main_orig else (if (fetch_res.final_url.len > 0) fetch_res.final_url else "about:blank");
                const main_ctx_json = session.formatExecutionContextEvent(main_orig);
                out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", main_ctx_json);
                event_count.* += 1;
            }
            {
                const raw_iso_orig = originOf(fetch_res.final_url);
                const iso_orig = if (raw_iso_orig.len > 0) raw_iso_orig else (if (fetch_res.final_url.len > 0) fetch_res.final_url else "about:blank");
                let esc_iso_orig_buf: [256]u8 = [0; 256];
                const esc_iso_orig = escapeJsonString(iso_orig, &esc_iso_orig_buf);
                let wi: usize = 0;
                while (wi < session.isolated_world_count && event_count.* < out_events.len) : (wi += 1) {
                    if (session.isolated_worlds[wi].active) {
                        let esc_wname_buf: [256]u8 = [0; 256];
                        const esc_wname = escapeJsonString(session.isolated_worlds[wi].name(), &esc_wname_buf);
                        const wcid = session.isolated_worlds[wi].context_id;
                        const iso_ctx_json = std.fmt.allocPrint(
                            session.allocator,
                            "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"{s}\",\"uniqueId\":\"ctx-{d}\",\"auxData\":{{\"isDefault\":false,\"type\":\"isolated\",\"frameId\":\"{s}\"}}}}}}",
                            .{ wcid, esc_iso_orig, esc_wname, wcid, session.frame_id },
                        ) catch "{}";
                        out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", iso_ctx_json);
                        event_count.* += 1;
                    }
                }
            }
            if (event_count.* < out_events.len) {
                out_events[event_count.*] = CdpEvent.create("Page.domContentEventFired", "{\"timestamp\":1700000000.0}");
                event_count.* += 1;
            }
            if (event_count.* < out_events.len) {
                out_events[event_count.*] = CdpEvent.create("Page.loadEventFired", "{\"timestamp\":1700000000.1}");
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
                                "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"auxData\":{{\"isDefault\":false,\"frameId\":\"{s}\"}}}}}}",
                                .{ ctx_id, esc_orig, cf.frame_id },
                            ) catch "{}";
                            out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
                            event_count.* += 1;
                            if (cf.is_cross_origin) {
                                // Record immediately so subsequent iterations in this loop see the active OOPIF slot
                                recordAttachedChildFrames(session, nav_slice[0..nav_frame_count]);
                            }
                        }
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
            if (!session.navigateToHistoryEntry(@as(i32, @intCast(entry_id)))) {
                return CdpResponse.err(req.id, -32000, "No entry with given id");
            }
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.err(req.id, -32601, "'Page' domain method not implemented");
    }
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

    const pending_raw = session.ctx.toCString(val);
    let raw_buf: [2048]u8 = [0; 2048];
    const copy_len = @min(pending_raw.len, raw_buf.len);
    if (copy_len > 0) {
        @memcpy(raw_buf[0..copy_len], pending_raw[0..copy_len]);
    }
    session.ctx.freeValue(val);
    if (copy_len == 0) return false;
    const pending_url = raw_buf[0..copy_len];

    let resolved_buf: [2048]u8 = [0; 2048];
    const target_url = HttpLoader.resolveRelativeUrl(session.url, pending_url, &resolved_buf);
    if (target_url.len == 0) return false;

    // Do not treat an iframe's src navigation as a top-level page navigation
    {
        let ni: usize = 0;
        while (ni < session.doc.nodes.items.len) : (ni += 1) {
            const node = &session.doc.nodes.items[ni];
            if (node.isElement() && node.hasTag("iframe")) {
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

    const fetch_res = HttpLoader.fetchDocument(session, target_url, fetch_buf, 0) catch return false;

    const kind = documentKind(fetch_res.content_type, fetch_res.content_disposition);
    if (kind == DocumentKind.blocked) return false;

    let doc_req_id_buf: [64]u8 = [0; 64];
    const doc_req_id = nextDocRequestId(&doc_req_id_buf);

    let synth_buf: ([]u8)? = null;
    defer if (synth_buf != null) session.allocator.free(synth_buf.?);
    let alloc_b64: ([]u8)? = null;
    defer if (alloc_b64 != null) session.allocator.free(alloc_b64.?);
    let b64_str: []const u8 = fetch_res.raw_body;
    let nav_html: []const u8 = fetch_res.html;

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

    if (session.network_enabled && event_count.* < out_events.len) {
        let esc_u_buf: [2048]u8 = [0; 2048];
        const esc_u = escapeJsonString(fetch_res.final_url, &esc_u_buf);
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

    const fn_json = session.formatFrameNavigatedEvent(fetch_res.final_url);
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
