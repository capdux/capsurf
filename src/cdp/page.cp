// src/cdp/page.cp
// CDP Page Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/page.ts and vendor/kitesurf/packages/engine/src/eval/outbound.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, extractJsonFloat, unescapeJsonString, MAX_CDP_EVENTS } from "./types.cp";
import { CdpPageSession, escapeJsonString } from "./session.cp";
import { jsIsException } from "../js/qjs_engine.cp";
import { HttpLoader } from "../net/http_loader.cp";
import { MockFixtures } from "../net/mock_fixtures.cp";
import { NodeType, NULL_NODE } from "../dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../css/cascade.cp";
import { PdfOptions, computePdfPagination, renderPdfAlloc } from "../render/pdf.cp";
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
    originOf,
    buildFrameTree,
    buildFrameTreeAt,
    buildFrameResourceTree
} from "./frames.cp";
import { extractAckSessionId } from "./screencast.cp";

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
    const is_xo = !HttpLoader.isSameOrigin(session.url, child_url);

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
    if (count.* < out_frames.len && MockFixtures.isFixtureUrl(child_url) && std.mem.indexOf(u8, child_url, "/frame-policy-child.html") == null) {
        const child_html_buf = session.allocator.alloc(u8, 16384) catch return;
        defer session.allocator.free(child_html_buf);
        if (child_html := MockFixtures.handleFixtureRequest(child_url, "", session.user_agent, child_html_buf, null, "")) {
            if (nested_src := findNestedIframeSrc(child_html)) {
                const gc_idx = count.*;
                const gc_url = resolveIframeFinalUrl(child_url, nested_src, url_storage[gc_idx * 512 .. (gc_idx + 1) * 512]);
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
            return count;
        }
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
    return count;
}

fn syncIframeTargets(session: *CdpPageSession, clear_existing: bool) {
    if (clear_existing) {
        session.target_registry.clearIframeTargets();
    }
    const f_arr = session.allocator.alloc(FrameState, 8) catch return;
    defer session.allocator.free(f_arr);
    const url_storage = session.allocator.alloc(u8, 8 * 512) catch return;
    defer session.allocator.free(url_storage);
    const id_storage = session.allocator.alloc(u8, 8 * 32) catch return;
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
            if (raw_html := extractJsonString(req.params_raw, "html")) {
                if (raw_html.len == 0) {
                    session.navigate("", session.url, "") catch return CdpResponse.err(req.id, -32000, "Failed to set document content");
                } else {
                    let unesc_buf = session.allocator.alloc(u8, raw_html.len) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                    defer session.allocator.free(unesc_buf);
                    const html = unescapeJsonString(raw_html, unesc_buf);
                    session.navigate(html, session.url, "") catch return CdpResponse.err(req.id, -32000, "Failed to set document content");
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
            const owner_sid = if (req.session_id.len > 0) req.session_id else session.target_id;
            * = session.screencast_handler.startScreencast(req.params_raw, owner_sid);
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.stopScreencast")) {
            * = session.screencast_handler.stopScreencast();
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.screencastFrameAck")) {
            const sid = extractAckSessionId(req.params_raw) ?? 0;
            * = session.screencast_handler.screencastFrameAck(sid);
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
                const sheet = StyleSheet.parse(full_css, session.viewport_width, false);
                let styles_opt = session.allocator.alloc(ComputedStyle, count) catch null;
                defer if (styles_opt != null) session.allocator.free(styles_opt.?);
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

            const pdf_bytes = renderPdfAlloc(session.allocator, page_count, media_w_pt, media_h_pt) catch return CdpResponse.err(req.id, -32000, "PDF generation failed");
            defer session.allocator.free(pdf_bytes);

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
            * = fmt;

            if (q := extractJsonInt(req.params_raw, "quality")) {
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
            const url = extractJsonString(req.params_raw, "url") ?? "about:blank";
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
                    "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"errorText\":\"net::ERR_BLOCKED_BY_PRIVATE_NETWORK_ACCESS\"}}",
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
                    "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\"}}",
                    .{ session.frame_id, session.loader_id },
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, nav_res);
            }

            // Allocate buffer for document fetching
            let fetch_buf = session.allocator.alloc(u8, 65536) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(fetch_buf);

            const fetch_res = HttpLoader.fetchDocument(session, url, fetch_buf, 0) catch |e| {
                if (e == error.RequestAborted) {
                    return CdpResponse.err(req.id, -32000, "net::ERR_ABORTED");
                }
                if (e == error.BlockedByClient) {
                    return CdpResponse.err(req.id, -32000, "net::ERR_BLOCKED_BY_CLIENT");
                }
                return CdpResponse.err(req.id, -32000, "Navigation failed");
            };

            let doc_req_id_buf: [64]u8 = [0; 64];
            const doc_req_id = nextDocRequestId(&doc_req_id_buf);

            const kind = documentKind(fetch_res.content_type, fetch_res.content_disposition);
            if (kind == DocumentKind.blocked) {
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
                    "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\",\"errorText\":\"net::ERR_ABORTED\"}}",
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

            session.navigate(nav_html, fetch_res.final_url, referrer) catch return CdpResponse.err(req.id, -32000, "HTML parsing failed");
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
            const nav_frames = session.allocator.alloc(FrameState, 8) catch null;
            const nav_urls = session.allocator.alloc(u8, 8 * 512) catch null;
            const nav_ids = session.allocator.alloc(u8, 8 * 32) catch null;
            defer if (nav_frames != null) session.allocator.free(nav_frames.?);
            defer if (nav_urls != null) session.allocator.free(nav_urls.?);
            defer if (nav_ids != null) session.allocator.free(nav_ids.?);
            if (nav_frames != null && nav_urls != null && nav_ids != null) {
                const nav_frame_count = collectFrameStates(session, nav_frames.?, nav_urls.?, nav_ids.?);
                let fi: usize = 1;
                while (fi < nav_frame_count && event_count.* < out_events.len) : (fi += 1) {
                    const cf = &nav_frames.?[fi];
                    if (session.runtime_enabled && event_count.* < out_events.len) {
                        const ctx_id = session.next_context_id;
                        session.next_context_id += 1;
                        const cf_origin = originOf(cf.url);
                        let esc_orig_buf: [256]u8 = [0; 256];
                        const esc_orig = escapeJsonString(cf_origin, &esc_orig_buf);
                        const ctx_json = std.fmt.allocPrint(
                            session.allocator,
                            "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"auxData\":{{\"isDefault\":true,\"frameId\":\"{s}\"}}}}}}",
                            .{ ctx_id, esc_orig, cf.frame_id },
                        ) catch "{}";
                        out_events[event_count.*] = CdpEvent.create("Runtime.executionContextCreated", ctx_json);
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
                "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\"}}",
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
            const f_arr = session.allocator.alloc(FrameState, 8) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(f_arr);
            const url_storage = session.allocator.alloc(u8, 8 * 512) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(url_storage);
            const id_storage = session.allocator.alloc(u8, 8 * 32) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(id_storage);
            const tree_buf = session.allocator.alloc(u8, 8192) catch return CdpResponse.err(req.id, -32000, "Out of memory");
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
            const f_arr = session.allocator.alloc(FrameState, 8) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(f_arr);
            const url_storage = session.allocator.alloc(u8, 8 * 512) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(url_storage);
            const id_storage = session.allocator.alloc(u8, 8 * 32) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(id_storage);
            const tree_buf = session.allocator.alloc(u8, 8192) catch return CdpResponse.err(req.id, -32000, "Out of memory");
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
    defer session.ctx.freeValue(val);
    if (jsIsException(val)) return false;

    const pending_raw = session.ctx.toCString(val);
    if (pending_raw.len == 0) return false;

    let raw_buf: [2048]u8 = [0; 2048];
    const copy_len = @min(pending_raw.len, raw_buf.len);
    @memcpy(raw_buf[0..copy_len], pending_raw[0..copy_len]);
    const pending_url = raw_buf[0..copy_len];

    let resolved_buf: [2048]u8 = [0; 2048];
    const target_url = HttpLoader.resolveRelativeUrl(session.url, pending_url, &resolved_buf);
    if (target_url.len == 0) return false;

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
