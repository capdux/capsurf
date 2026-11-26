// src/cdp/dom.cp
// CDP DOM Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/dom.ts

import { CdpRequest, CdpResponse, CdpEvent, CdpBoxModel, extractJsonString, extractJsonInt, extractJsonFloat, MAX_CDP_EVENTS } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { hitTest } from "./input.cp";
import { clearLearnedScripts } from "./session_hooks.cp";
import { MockFixtures } from "../net/mock_fixtures.cp";
import { NodeId, NULL_NODE, Document, NodeType } from "../dom/dom.cp";

fn shiftQuadScroll(q: [8]f32, sx: f32, sy: f32): [8]f32 {
    let res: [8]f32 = [0.0; 8];
    res[0] = q[0] - sx;
    res[1] = q[1] - sy;
    res[2] = q[2] - sx;
    res[3] = q[3] - sy;
    res[4] = q[4] - sx;
    res[5] = q[5] - sy;
    res[6] = q[6] - sx;
    res[7] = q[7] - sy;
    return res;
}

fn formatQuad(quad: [8]f32, buf: []u8): []const u8 {
    return std.fmt.bufPrint(
        buf,
        "[{d},{d},{d},{d},{d},{d},{d},{d}]",
        .{ quad[0], quad[1], quad[2], quad[3], quad[4], quad[5], quad[6], quad[7] },
    ) catch "[]";
}

pub fn appendJsonEscaped(allocator: std.mem.Allocator, list: *std.ArrayListUnmanaged(u8), str: []const u8)! {
    list.append(allocator, '"')?;
    let i: usize = 0;
    while (i < str.len) : (i += 1) {
        const c = str[i];
        if (c == '"') {
            list.appendSlice(allocator, "\\\"")?;
        } else if (c == '\\') {
            list.appendSlice(allocator, "\\\\")?;
        } else if (c == '\n') {
            list.appendSlice(allocator, "\\n")?;
        } else if (c == '\r') {
            list.appendSlice(allocator, "\\r")?;
        } else if (c == '\t') {
            list.appendSlice(allocator, "\\t")?;
        } else if (c == '\x08') {
            list.appendSlice(allocator, "\\b")?;
        } else if (c == '\x0c') {
            list.appendSlice(allocator, "\\f")?;
        } else if (c < 0x20) {
            const hex = "0123456789abcdef";
            let esc: [6]u8 = [ '\\', 'u', '0', '0', hex[(c >> 4) & 0xF], hex[c & 0xF] ];
            list.appendSlice(allocator, &esc)?;
        } else {
            list.append(allocator, c)?;
        }
    }
    list.append(allocator, '"')?;
}

fn appendFileToDataTransferJs(
    allocator: std.mem.Allocator,
    js_list: *std.ArrayListUnmanaged(u8),
    file_path: []const u8,
    base_name: []const u8,
    rel_path: []const u8,
)! {
    let fbytes_opt: ([]const u8)? = null;
    let f_alloc: ([]u8)? = allocator.alloc(u8, 262144) catch null;
    defer if (f_alloc != null) allocator.free(f_alloc.?);
    defer if (fb_free := fbytes_opt) {
        if (fb_free.len > 0) {
            let from_tmp = false;
            if (fb_tmp := f_alloc) {
                if (@intFromPtr(fb_free.ptr) == @intFromPtr(fb_tmp.ptr)) from_tmp = true;
            }
            if (!from_tmp) {
                std.heap.page_allocator.free(fb_free);
            }
        }
    }
    if (fb_buf := f_alloc) {
        fbytes_opt = MockFixtures.readFileContent(file_path, fb_buf);
        if (fbytes_opt == null && rel_path.len == 0) {
            let pw_path_buf: [1024]u8 = [0; 1024];
            if (pw_p := std.fmt.bufPrint(&pw_path_buf, "test/pw/.playwright/{s}", .{file_path}) catch null) {
                fbytes_opt = MockFixtures.readFileContent(pw_p, fb_buf);
            }
        }
    }

    js_list.appendSlice(allocator, "{ const b = atob('")?;
    if (fb := fbytes_opt) {
        const enc_len = std.base64.standard.Encoder.calcSize(fb.len);
        if (b64_buf := allocator.alloc(u8, enc_len) catch null) {
            defer allocator.free(b64_buf);
            const b64 = std.base64.standard.Encoder.encode(b64_buf, fb);
            js_list.appendSlice(allocator, b64)?;
        }
    }
    const mime_type = mimeTypeFromFilename(base_name);
    js_list.appendSlice(allocator, "'); const u = new Uint8Array(b.length); for (let i = 0; i < b.length; i++) u[i] = b.charCodeAt(i); const f = new File([u], ")?;
    appendJsonEscaped(allocator, js_list, base_name)?;
    js_list.appendSlice(allocator, ", { type: \"")?;
    js_list.appendSlice(allocator, mime_type)?;
    js_list.appendSlice(allocator, "\"")?;
    if (rel_path.len > 0) {
        js_list.appendSlice(allocator, ", webkitRelativePath: ")?;
        appendJsonEscaped(allocator, js_list, rel_path)?;
    }
    js_list.appendSlice(allocator, " }); dt.items.add(f); } ")?;
}

fn tryOpenDirC(path: []const u8): (*std.c.DIR)? {
    if (path.len == 0 || path.len >= 1024) return null;
    let zbuf: [1024]u8 = [0; 1024];
    @memcpy(zbuf[0..path.len], path);
    zbuf[path.len] = 0;
    return unsafe { std.c.opendir(@ptrCast(&zbuf)) };
}

fn appendDirToDataTransferJs(
    allocator: std.mem.Allocator,
    js_list: *std.ArrayListUnmanaged(u8),
    fs_dir_path: []const u8,
    rel_prefix: []const u8,
    depth: usize,
)! {
    if (depth > 32) return;
    const dir_opt = tryOpenDirC(fs_dir_path);
    if (dir_opt == null) return;
    const dir = dir_opt.?;
    defer * = unsafe { std.c.closedir(dir) };
    while (entry := unsafe { std.c.readdir(dir) }) {
        const name_slice = std.mem.sliceTo(&entry.name, 0);
        if (std.mem.eql(u8, name_slice, ".") || std.mem.eql(u8, name_slice, "..")) continue;
        const child_fs = std.fmt.allocPrint(allocator, "{s}/{s}", .{ fs_dir_path, name_slice }) catch continue;
        defer allocator.free(child_fs);
        const child_rel = std.fmt.allocPrint(allocator, "{s}/{s}", .{ rel_prefix, name_slice }) catch continue;
        defer allocator.free(child_rel);
        if (entry.type == 4) {
            appendDirToDataTransferJs(allocator, js_list, child_fs, child_rel, depth + 1)?;
        } else if (entry.type == 8 || entry.type == 0) {
            if (entry.type == 0) {
                if (sub_d := tryOpenDirC(child_fs)) {
                    * = unsafe { std.c.closedir(sub_d) };
                    appendDirToDataTransferJs(allocator, js_list, child_fs, child_rel, depth + 1)?;
                    continue;
                }
            }
            appendFileToDataTransferJs(allocator, js_list, child_fs, name_slice, child_rel)?;
        }
    }
}

pub struct ShadowRootEntry {
    nid: NodeId = NULL_NODE,
    json: []const u8 = "",
}

pub struct IframeFrameEntry {
    nid: NodeId = NULL_NODE,
    frame_id: []const u8 = "",
}

fn isNodeInSvg(doc: *const Document, nid: NodeId): bool {
    let cur = nid;
    let guard: usize = 0;
    while (cur != NULL_NODE && @as(usize, cur) < doc.nodes.items.len && guard < 128) : (guard += 1) {
        const n = &doc.nodes.items[@as(usize, cur)];
        if (n.node_type == .Element && std.ascii.eqlIgnoreCase(n.tag_name, "svg")) {
            return true;
        }
        cur = n.parent;
    }
    return false;
}

fn findShadowRootJson(entries: []const ShadowRootEntry, nid: NodeId): ([]const u8)? {
    let i: usize = 0;
    while (i < entries.len) : (i += 1) {
        if (entries[i].nid == nid) return entries[i].json;
    }
    return null;
}

fn findIframeFrameId(entries: []const IframeFrameEntry, nid: NodeId): ([]const u8)? {
    let i: usize = 0;
    while (i < entries.len) : (i += 1) {
        if (entries[i].nid == nid && entries[i].frame_id.len > 0) return entries[i].frame_id;
    }
    return null;
}

fn collectDocIframeEntries(session: *CdpPageSession, out_entries: *[32]IframeFrameEntry): usize {
    let count: usize = 0;
    let active_kfs: [48]usize = [0; 48];
    let active_kf_count: usize = 0;
    let ki: usize = 0;
    while (ki < session.known_child_frame_count && active_kf_count < active_kfs.len) : (ki += 1) {
        const kf = &session.known_child_frames[ki];
        if (kf.active && !kf.detached && kf.frame_id_len > 0) {
            active_kfs[active_kf_count] = ki;
            active_kf_count += 1;
        }
    }
    let iframe_seq: usize = 0;
    let ni: usize = 1;
    while (ni < session.doc.nodes.items.len && count < out_entries.len) : (ni += 1) {
        const n = &session.doc.nodes.items[ni];
        if (n.node_type == NodeType.Element && (std.ascii.eqlIgnoreCase(n.tag_name, "iframe") || std.ascii.eqlIgnoreCase(n.tag_name, "frame")) && session.doc.isConnectedNode(@as(NodeId, @intCast(ni)))) {
            let matched_fid: []const u8 = "";
            if (n.id_attr.len > 0) {
                let ak: usize = 0;
                while (ak < active_kf_count) : (ak += 1) {
                    const kf = &session.known_child_frames[active_kfs[ak]];
                    if (kf.owner_id_len > 0 && std.mem.eql(u8, kf.owner_id_buf[0..kf.owner_id_len], n.id_attr)) {
                        matched_fid = kf.frame_id_buf[0..kf.frame_id_len];
                        break;
                    }
                }
            }
            if (matched_fid.len == 0 && iframe_seq < active_kf_count) {
                const kf = &session.known_child_frames[active_kfs[iframe_seq]];
                matched_fid = kf.frame_id_buf[0..kf.frame_id_len];
            }
            if (matched_fid.len > 0) {
                out_entries[count] = IframeFrameEntry{
                    nid: @as(NodeId, @intCast(ni)),
                    frame_id: matched_fid,
                };
                count += 1;
            }
            iframe_seq += 1;
        }
    }
    return count;
}

pub fn serializeCdpNode(allocator: std.mem.Allocator, doc: *const Document, nid: NodeId, depth: i64, list: *std.ArrayListUnmanaged(u8))! {
    const empty_sr: []const ShadowRootEntry = [];
    const empty_ifr: []const IframeFrameEntry = [];
    const in_svg = isNodeInSvg(doc, nid);
    return serializeCdpNodeWithOptions(allocator, doc, nid, depth, in_svg, empty_sr, empty_ifr, list);
}

pub fn serializeCdpNodeWithOptions(
    allocator: std.mem.Allocator,
    doc: *const Document,
    nid: NodeId,
    depth: i64,
    in_svg: bool,
    shadow_roots: []const ShadowRootEntry,
    iframe_frames: []const IframeFrameEntry,
    list: *std.ArrayListUnmanaged(u8),
)! {
    if (nid == NULL_NODE || @as(usize, nid) >= doc.nodes.items.len) return;
    const node = &doc.nodes.items[@as(usize, nid)];

    let child_count: u32 = 0;
    let ch = node.first_child;
    while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
        child_count += 1;
        ch = doc.nodes.items[@as(usize, ch)].next_sibling;
    }

    let buf: [256]u8 = undefined;
    const header = std.fmt.bufPrint(&buf, "{{\"nodeId\":{d},\"backendNodeId\":{d},", .{ nid, nid }) catch return;
    list.appendSlice(allocator, header)?;

    let child_in_svg = in_svg;
    switch (node.node_type) {
        .Document => {
            list.appendSlice(allocator, "\"nodeType\":9,\"nodeName\":\"#document\",\"localName\":\"\",\"nodeValue\":\"\",")?;
        },
        .Element => {
            const tag = node.tag_name;
            const elem_in_svg = in_svg || std.ascii.eqlIgnoreCase(tag, "svg");
            child_in_svg = elem_in_svg;
            let upper_buf: [64]u8 = [0; 64];
            let upper_len: usize = 0;
            while (upper_len < tag.len && upper_len < upper_buf.len) : (upper_len += 1) {
                upper_buf[upper_len] = std.ascii.toUpper(tag[upper_len]);
            }
            const node_name = if (elem_in_svg) tag else upper_buf[0..upper_len];
            let el_buf: [256]u8 = undefined;
            const el_header = std.fmt.bufPrint(&el_buf, "\"nodeType\":1,\"nodeName\":\"{s}\",\"localName\":\"{s}\",\"nodeValue\":\"\",", .{ node_name, tag }) catch return;
            list.appendSlice(allocator, el_header)?;

            list.appendSlice(allocator, "\"attributes\":[")?;
            let attr_first = true;
            let has_id = false;
            let has_class = false;
            let has_style = false;
            let ai: usize = 0;
            while (ai < node.attr_count) : (ai += 1) {
                const attr = node.attrs[ai];
                if (std.ascii.eqlIgnoreCase(attr.name, "id")) has_id = true;
                if (std.ascii.eqlIgnoreCase(attr.name, "class")) has_class = true;
                if (std.ascii.eqlIgnoreCase(attr.name, "style")) has_style = true;
                if (!attr_first) list.append(allocator, ',')?;
                appendJsonEscaped(allocator, list, attr.name)?;
                list.append(allocator, ',')?;
                appendJsonEscaped(allocator, list, attr.value)?;
                attr_first = false;
            }
            if (!has_id && node.id_attr.len > 0) {
                if (!attr_first) list.append(allocator, ',')?;
                list.appendSlice(allocator, "\"id\",")?;
                appendJsonEscaped(allocator, list, node.id_attr)?;
                attr_first = false;
            }
            if (!has_class && node.class_attr.len > 0) {
                if (!attr_first) list.append(allocator, ',')?;
                list.appendSlice(allocator, "\"class\",")?;
                appendJsonEscaped(allocator, list, node.class_attr)?;
                attr_first = false;
            }
            if (!has_style && node.inline_style.len > 0) {
                if (!attr_first) list.append(allocator, ',')?;
                list.appendSlice(allocator, "\"style\",")?;
                appendJsonEscaped(allocator, list, node.inline_style)?;
                attr_first = false;
            }
            list.appendSlice(allocator, "],")?;
            if (std.ascii.eqlIgnoreCase(tag, "iframe") || std.ascii.eqlIgnoreCase(tag, "frame")) {
                if (fid := findIframeFrameId(iframe_frames, nid)) {
                    list.appendSlice(allocator, "\"frameId\":")?;
                    appendJsonEscaped(allocator, list, fid)?;
                    list.append(allocator, ',')?;
                }
            }
        },
        .Text => {
            list.appendSlice(allocator, "\"nodeType\":3,\"nodeName\":\"#text\",\"localName\":\"\",\"nodeValue\":")?;
            appendJsonEscaped(allocator, list, node.text_content)?;
            list.append(allocator, ',')?;
        },
        .Comment => {
            list.appendSlice(allocator, "\"nodeType\":8,\"nodeName\":\"#comment\",\"localName\":\"\",\"nodeValue\":")?;
            appendJsonEscaped(allocator, list, node.text_content)?;
            list.append(allocator, ',')?;
        },
    }

    let cnt_buf: [64]u8 = undefined;
    const cnt_s = std.fmt.bufPrint(&cnt_buf, "\"childNodeCount\":{d}", .{child_count}) catch return;
    list.appendSlice(allocator, cnt_s)?;

    if (depth != 0 && child_count > 0) {
        list.appendSlice(allocator, ",\"children\":[")?;
        let first_kid = true;
        let kid = node.first_child;
        const next_depth = if (depth > 0) depth - 1 else -1;
        while (kid != NULL_NODE && @as(usize, kid) < doc.nodes.items.len) {
            if (!first_kid) list.append(allocator, ',')?;
            first_kid = false;
            serializeCdpNodeWithOptions(allocator, doc, kid, next_depth, child_in_svg, shadow_roots, iframe_frames, list)?;
            kid = doc.nodes.items[@as(usize, kid)].next_sibling;
        }
        list.append(allocator, ']')?;
    }

    if (depth != 0) {
        if (sr_json := findShadowRootJson(shadow_roots, nid)) {
            list.appendSlice(allocator, ",\"shadowRoots\":[")?;
            list.appendSlice(allocator, sr_json)?;
            list.append(allocator, ']')?;
        }
    }

    list.append(allocator, '}')?;
}

pub type ByteSlice = []u8;

pub fn drainDomMutationEvents(session: *CdpPageSession, out_msgs: *std.ArrayListUnmanaged(ByteSlice)) {
    if (session.dom_ctx == null) return;
    let dom_ctx = session.dom_ctx.?;
    if (!session.dom_document_sent) {
        dom_ctx.clearDomMutations();
        return;
    }
    const alloc = std.heap.c_allocator;
    let i: usize = 0;
    while (i < dom_ctx.dom_mutations.items.len) : (i += 1) {
        const rec = &dom_ctx.dom_mutations.items[i];
        switch (rec.kind) {
            .AttrModified => {
                if (!session.isDomNodeKnown(rec.node_id)) continue;
                let msg = std.ArrayListUnmanaged(u8).empty;
                let hdr_buf: [128]u8 = undefined;
                const hdr = std.fmt.bufPrint(&hdr_buf, "{{\"method\":\"DOM.attributeModified\",\"params\":{{\"nodeId\":{d},\"name\":", .{rec.node_id}) catch continue;
                msg.appendSlice(alloc, hdr) catch { msg.deinit(alloc); continue; };
                appendJsonEscaped(alloc, &msg, rec.getName()) catch { msg.deinit(alloc); continue; };
                msg.appendSlice(alloc, ",\"value\":") catch { msg.deinit(alloc); continue; };
                appendJsonEscaped(alloc, &msg, rec.getValue()) catch { msg.deinit(alloc); continue; };
                msg.appendSlice(alloc, "}}") catch { msg.deinit(alloc); continue; };
                const slice = msg.toOwnedSlice(alloc) catch { msg.deinit(alloc); continue; };
                out_msgs.append(alloc, slice) catch { alloc.free(slice); };
            },
            .AttrRemoved => {
                if (!session.isDomNodeKnown(rec.node_id)) continue;
                let msg = std.ArrayListUnmanaged(u8).empty;
                let hdr_buf: [128]u8 = undefined;
                const hdr = std.fmt.bufPrint(&hdr_buf, "{{\"method\":\"DOM.attributeRemoved\",\"params\":{{\"nodeId\":{d},\"name\":", .{rec.node_id}) catch continue;
                msg.appendSlice(alloc, hdr) catch { msg.deinit(alloc); continue; };
                appendJsonEscaped(alloc, &msg, rec.getName()) catch { msg.deinit(alloc); continue; };
                msg.appendSlice(alloc, "}}") catch { msg.deinit(alloc); continue; };
                const slice = msg.toOwnedSlice(alloc) catch { msg.deinit(alloc); continue; };
                out_msgs.append(alloc, slice) catch { alloc.free(slice); };
            },
            .CharDataModified => {
                if (!session.isDomNodeKnown(rec.node_id)) continue;
                let msg = std.ArrayListUnmanaged(u8).empty;
                let hdr_buf: [128]u8 = undefined;
                const hdr = std.fmt.bufPrint(&hdr_buf, "{{\"method\":\"DOM.characterDataModified\",\"params\":{{\"nodeId\":{d},\"characterData\":", .{rec.node_id}) catch continue;
                msg.appendSlice(alloc, hdr) catch { msg.deinit(alloc); continue; };
                appendJsonEscaped(alloc, &msg, rec.getValue()) catch { msg.deinit(alloc); continue; };
                msg.appendSlice(alloc, "}}") catch { msg.deinit(alloc); continue; };
                const slice = msg.toOwnedSlice(alloc) catch { msg.deinit(alloc); continue; };
                out_msgs.append(alloc, slice) catch { alloc.free(slice); };
            },
            .ChildInserted => {
                if (!session.isDomNodeKnown(rec.parent_id)) continue;
                if (rec.node_id == NULL_NODE || @as(usize, rec.node_id) >= session.doc.nodes.items.len) continue;
                let msg = std.ArrayListUnmanaged(u8).empty;
                let hdr_buf: [160]u8 = undefined;
                const hdr = std.fmt.bufPrint(
                    &hdr_buf,
                    "{{\"method\":\"DOM.childNodeInserted\",\"params\":{{\"parentNodeId\":{d},\"previousNodeId\":{d},\"node\":",
                    .{ rec.parent_id, rec.prev_node_id },
                ) catch continue;
                msg.appendSlice(alloc, hdr) catch { msg.deinit(alloc); continue; };
                serializeCdpNode(alloc, &session.doc, rec.node_id, -1, &msg) catch { msg.deinit(alloc); continue; };
                msg.appendSlice(alloc, "}}") catch { msg.deinit(alloc); continue; };
                session.markDomSubtreeKnown(rec.node_id, -1, true);
                const slice = msg.toOwnedSlice(alloc) catch { msg.deinit(alloc); continue; };
                out_msgs.append(alloc, slice) catch { alloc.free(slice); };
            },
            .ChildRemoved => {
                if (!session.isDomNodeKnown(rec.parent_id)) continue;
                if (!session.isDomNodeKnown(rec.node_id)) continue;
                session.markDomSubtreeKnown(rec.node_id, -1, false);
                let msg = std.ArrayListUnmanaged(u8).empty;
                let hdr_buf: [160]u8 = undefined;
                const hdr = std.fmt.bufPrint(
                    &hdr_buf,
                    "{{\"method\":\"DOM.childNodeRemoved\",\"params\":{{\"parentNodeId\":{d},\"nodeId\":{d}}}}}",
                    .{ rec.parent_id, rec.node_id },
                ) catch continue;
                msg.appendSlice(alloc, hdr) catch { msg.deinit(alloc); continue; };
                const slice = msg.toOwnedSlice(alloc) catch { msg.deinit(alloc); continue; };
                out_msgs.append(alloc, slice) catch { alloc.free(slice); };
            },
            .InnerHtmlRebuilt => {
                if (!session.isDomNodeKnown(rec.node_id)) continue;
                if (rec.node_id == NULL_NODE || @as(usize, rec.node_id) >= session.doc.nodes.items.len) continue;
                const parent_node = &session.doc.nodes.items[@as(usize, rec.node_id)];
                let child_count: u32 = 0;
                let ch = parent_node.first_child;
                while (ch != NULL_NODE && @as(usize, ch) < session.doc.nodes.items.len) {
                    child_count += 1;
                    ch = session.doc.nodes.items[@as(usize, ch)].next_sibling;
                }

                // 1. DOM.childNodeCountUpdated
                {
                    let cnt_msg = std.ArrayListUnmanaged(u8).empty;
                    let cnt_buf: [160]u8 = undefined;
                    const cnt_s = std.fmt.bufPrint(
                        &cnt_buf,
                        "{{\"method\":\"DOM.childNodeCountUpdated\",\"params\":{{\"nodeId\":{d},\"childNodeCount\":{d}}}}}",
                        .{ rec.node_id, child_count },
                    ) catch "";
                    if (cnt_s.len > 0) {
                        cnt_msg.appendSlice(alloc, cnt_s) catch { cnt_msg.deinit(alloc); };
                        if (cnt_msg.items.len > 0) {
                            const slice = cnt_msg.toOwnedSlice(alloc) catch { cnt_msg.deinit(alloc); continue; };
                            out_msgs.append(alloc, slice) catch { alloc.free(slice); };
                        }
                    }
                }

                // 2. DOM.setChildNodes
                {
                    let set_msg = std.ArrayListUnmanaged(u8).empty;
                    let set_buf: [160]u8 = undefined;
                    const set_hdr = std.fmt.bufPrint(
                        &set_buf,
                        "{{\"method\":\"DOM.setChildNodes\",\"params\":{{\"parentId\":{d},\"nodes\":[",
                        .{rec.node_id},
                    ) catch continue;
                    set_msg.appendSlice(alloc, set_hdr) catch { set_msg.deinit(alloc); continue; };
                    let first_kid = true;
                    let kid = parent_node.first_child;
                    while (kid != NULL_NODE && @as(usize, kid) < session.doc.nodes.items.len) {
                        if (!first_kid) {
                            set_msg.append(alloc, ',') catch { set_msg.deinit(alloc); break; };
                        }
                        first_kid = false;
                        serializeCdpNode(alloc, &session.doc, kid, -1, &set_msg) catch { set_msg.deinit(alloc); break; };
                        session.markDomSubtreeKnown(kid, -1, true);
                        kid = session.doc.nodes.items[@as(usize, kid)].next_sibling;
                    }
                    if (set_msg.items.len > 0) {
                        set_msg.appendSlice(alloc, "]}}") catch { set_msg.deinit(alloc); continue; };
                        const slice = set_msg.toOwnedSlice(alloc) catch { set_msg.deinit(alloc); continue; };
                        out_msgs.append(alloc, slice) catch { alloc.free(slice); };
                    }
                }
            },
        }
    }
    dom_ctx.clearDomMutations();
}

fn resolveNodeIdFromObjectId(session: *CdpPageSession, object_id: []const u8): NodeId {
    if (object_id.len == 0) return NULL_NODE;

    if (std.mem.startsWith(u8, object_id, "node:")) {
        const id_val = std.fmt.parseInt(u32, object_id[5..], 10) catch 0;
        return @as(NodeId, id_val);
    }

    if (std.mem.startsWith(u8, object_id, "ps:")) {
        if (last_colon := std.mem.lastIndexOfScalar(u8, object_id, ':')) {
            const num_str = object_id[last_colon + 1 ..];
            const parsed = std.fmt.parseInt(u32, num_str, 10) catch 0;
            if (parsed > 0 && parsed < session.doc.nodeCount()) {
                return parsed;
            }
        }
        return NULL_NODE;
    }

    if (session.has_document) {
        let js_buf: [256]u8 = [0; 256];
        const js_expr = std.fmt.bufPrint(
            &js_buf,
            "(() => {{ const o = (globalThis.__capsurf_get_object) ? globalThis.__capsurf_get_object('{s}') : null; return (o && typeof o._nid === 'number') ? o._nid : 0; }})()",
            .{object_id},
        ) catch null;
        if (e := js_expr) {
            const val = session.ctx.eval(e);
            defer session.ctx.freeValue(val);
            if (val.tag == 0 && val.u.uint64 > 0) {
                const nid = @as(u32, @truncate(val.u.uint64));
                if (nid > 0 && nid < session.doc.nodeCount()) {
                    return nid;
                }
            }
        }
    }

    return NULL_NODE;
}

fn queryJsObjectRect(session: *CdpPageSession, obj_id: []const u8, node_id_val: i64, out_x: *f32, out_y: *f32, out_w: *f32, out_h: *f32): bool {
    if (!session.has_document) return false;
    let js_buf: [640]u8 = [0; 640];
    let js_expr: []const u8 = "";
    if (obj_id.len > 0) {
        js_expr = std.fmt.bufPrint(
            &js_buf,
            "(() => {{ const el = globalThis.__capsurf_get_object ? globalThis.__capsurf_get_object('{s}') : null; if (!el || !el.getBoundingClientRect) return ''; const r = el.getBoundingClientRect(); if (!r || (r.width <= 0 && r.height <= 0)) return ''; return r.left + ',' + r.top + ',' + r.width + ',' + r.height; }})()",
            .{obj_id},
        ) catch "";
    } else if (node_id_val > 0) {
        js_expr = std.fmt.bufPrint(
            &js_buf,
            "(() => {{ const el = globalThis.__capsurf_get_object ? globalThis.__capsurf_get_object('node:{d}') : null; if (!el || !el.getBoundingClientRect) return ''; const r = el.getBoundingClientRect(); if (!r || (r.width <= 0 && r.height <= 0)) return ''; return r.left + ',' + r.top + ',' + r.width + ',' + r.height; }})()",
            .{node_id_val},
        ) catch "";
    }
    if (js_expr.len == 0) return false;
    const val = session.ctx.eval(js_expr);
    defer session.ctx.freeValue(val);
    const rstr = session.ctx.toCString(val);
    if (rstr.len == 0) return false;
    let it = std.mem.splitScalar(u8, rstr, ',');
    const rx = std.fmt.parseFloat(f32, it.next() ?? "0") catch 0.0;
    const ry = std.fmt.parseFloat(f32, it.next() ?? "0") catch 0.0;
    const rw = std.fmt.parseFloat(f32, it.next() ?? "0") catch 0.0;
    const rh = std.fmt.parseFloat(f32, it.next() ?? "0") catch 0.0;
    if (rw <= 0.0 && rh <= 0.0) return false;
    out_x.* = rx;
    out_y.* = ry;
    out_w.* = rw;
    out_h.* = rh;
    return true;
}

fn applyAncestorScrollAndFixed(session: *CdpPageSession, node_id: NodeId, sx: *f32, sy: *f32) {
    if (!session.has_document || node_id == NULL_NODE) return;
    let js_buf: [640]u8 = [0; 640];
    const js_expr = std.fmt.bufPrint(
        &js_buf,
        "(() => {{ let el = globalThis.__capsurf_get_object ? globalThis.__capsurf_get_object('node:{d}') : null; if (!el) return ''; let ax = 0, ay = 0, fixed = false, cur = el; while (cur && cur.nodeType === 1) {{ const st = globalThis.getComputedStyle ? globalThis.getComputedStyle(cur) : null; if (st && st.position === 'fixed') {{ fixed = true; break; }} if (cur !== el) {{ const tag = String(cur.tagName || '').toLowerCase(); if (tag !== 'html' && tag !== 'body') {{ ax += Number(cur.scrollLeft || 0); ay += Number(cur.scrollTop || 0); }} }} cur = cur.parentElement; }} return (fixed ? '1,' : '0,') + ax + ',' + ay; }})()",
        .{node_id},
    ) catch return;
    const val = session.ctx.eval(js_expr);
    defer session.ctx.freeValue(val);
    const rstr = session.ctx.toCString(val);
    if (rstr.len == 0) return;
    let it = std.mem.splitScalar(u8, rstr, ',');
    const is_fixed = std.mem.eql(u8, it.next() ?? "0", "1");
    const ax = std.fmt.parseFloat(f32, it.next() ?? "0") catch 0.0;
    const ay = std.fmt.parseFloat(f32, it.next() ?? "0") catch 0.0;
    if (is_fixed) {
        sx.* = 0.0;
        sy.* = 0.0;
    } else {
        sx.* += ax;
        sy.* += ay;
    }
}

fn formatJsBoxModelResp(rx: f32, ry: f32, rw: f32, rh: f32, req_id: i64, out_buf: []u8): CdpResponse {
    const x2 = rx + rw;
    const y2 = ry + rh;
    let q_buf: [256]u8 = [0; 256];
    const q_str = std.fmt.bufPrint(
        &q_buf,
        "[{d},{d},{d},{d},{d},{d},{d},{d}]",
        .{ rx, ry, x2, ry, x2, y2, rx, y2 },
    ) catch "[]";
    const w_int = @as(i64, @intFromFloat(@round(rw)));
    const h_int = @as(i64, @intFromFloat(@round(rh)));
    const res = std.fmt.bufPrint(
        out_buf,
        "{{\"model\":{{\"content\":{s},\"padding\":{s},\"border\":{s},\"margin\":{s},\"width\":{d},\"height\":{d}}}}}",
        .{ q_str, q_str, q_str, q_str, w_int, h_int },
    ) catch return CdpResponse.err(req_id, -32000, "Buffer overflow");
    return CdpResponse.ok(req_id, res);
}

fn mimeTypeFromFilename(name: []const u8): []const u8 {
    if (dot := std.mem.lastIndexOfScalar(u8, name, '.')) {
        const ext = name[dot..];
        if (std.ascii.eqlIgnoreCase(ext, ".png")) return "image/png";
        if (std.ascii.eqlIgnoreCase(ext, ".jpg") || std.ascii.eqlIgnoreCase(ext, ".jpeg")) return "image/jpeg";
        if (std.ascii.eqlIgnoreCase(ext, ".gif")) return "image/gif";
        if (std.ascii.eqlIgnoreCase(ext, ".webp")) return "image/webp";
        if (std.ascii.eqlIgnoreCase(ext, ".svg")) return "image/svg+xml";
        if (std.ascii.eqlIgnoreCase(ext, ".txt")) return "text/plain";
        if (std.ascii.eqlIgnoreCase(ext, ".html") || std.ascii.eqlIgnoreCase(ext, ".htm")) return "text/html";
        if (std.ascii.eqlIgnoreCase(ext, ".css")) return "text/css";
        if (std.ascii.eqlIgnoreCase(ext, ".js") || std.ascii.eqlIgnoreCase(ext, ".mjs")) return "text/javascript";
        if (std.ascii.eqlIgnoreCase(ext, ".json")) return "application/json";
        if (std.ascii.eqlIgnoreCase(ext, ".pdf")) return "application/pdf";
        if (std.ascii.eqlIgnoreCase(ext, ".xml")) return "application/xml";
        if (std.ascii.eqlIgnoreCase(ext, ".csv")) return "text/csv";
    }
    return "";
}

fn formatJsQuadsResp(rx: f32, ry: f32, rw: f32, rh: f32, req_id: i64, out_buf: []u8): CdpResponse {
    const x2 = rx + rw;
    const y2 = ry + rh;
    const res = std.fmt.bufPrint(
        out_buf,
        "{{\"quads\":[[{d},{d},{d},{d},{d},{d},{d},{d}]]}}",
        .{ rx, ry, x2, ry, x2, y2, rx, y2 },
    ) catch return CdpResponse.err(req_id, -32000, "Buffer overflow");
    return CdpResponse.ok(req_id, res);
}

fn extractOriginSlice(url: []const u8): []const u8 {
    if (scheme_end := std.mem.indexOf(u8, url, "://")) {
        const after_scheme = scheme_end + 3;
        if (slash_pos := std.mem.indexOfScalarPos(u8, url, after_scheme, '/')) {
            return url[0..slash_pos];
        }
        if (q_pos := std.mem.indexOfScalarPos(u8, url, after_scheme, '?')) {
            return url[0..q_pos];
        }
        return url;
    }
    return "";
}

fn resolveExecutionContextFrameId(session: *CdpPageSession, req_session_id: []const u8, exec_ctx_opt: (i64)?): []const u8 {
    if (exec_ctx := exec_ctx_opt) {
        if (exec_ctx == 1) {
            return session.frame_id;
        }
        if (exec_ctx > 1) {
            if (iso_fid := session.isolatedWorldFrameId(exec_ctx)) {
                if (iso_fid.len > 0) return iso_fid;
            }
            let ki: usize = 0;
            while (ki < session.known_child_frame_count) : (ki += 1) {
                const kf = &session.known_child_frames[ki];
                if (kf.active && !kf.detached && kf.exec_context_id == exec_ctx && kf.frame_id_len > 0) {
                    return kf.frame_id_buf[0..kf.frame_id_len];
                }
            }
            if (session.iframe_ctx_created && session.iframe_ctx_id == exec_ctx && session.iframe_frame_id_len > 0) {
                return session.iframe_frame_id_buf[0..session.iframe_frame_id_len];
            }
        }
    }
    if (req_session_id.len > 0) {
        if (sess_fid := session.target_registry.findFrameIdBySessionId(req_session_id)) {
            return sess_fid;
        }
    }
    return "";
}

fn findActiveKnownChildFrameIndex(session: *CdpPageSession, fid: []const u8): (usize)? {
    let ki: usize = 0;
    while (ki < session.known_child_frame_count) : (ki += 1) {
        const kf = &session.known_child_frames[ki];
        if (kf.active && !kf.detached && kf.frame_id_len > 0) {
            if (std.mem.eql(u8, kf.frame_id_buf[0..kf.frame_id_len], fid)) {
                return ki;
            }
        }
    }
    return null;
}

fn canFramesScriptEachOther(session: *CdpPageSession, fid_a: []const u8, fid_b: []const u8): bool {
    if (fid_a.len == 0 || fid_b.len == 0) return true;
    if (std.mem.eql(u8, fid_a, fid_b)) return true;
    if (std.mem.eql(u8, fid_a, "__other_doc__") || std.mem.eql(u8, fid_b, "__other_doc__")) return false;

    if (session.iframe_ctx_created && session.iframe_frame_id_len > 0) {
        const legacy_fid = session.iframe_frame_id_buf[0..session.iframe_frame_id_len];
        if (std.mem.eql(u8, fid_a, legacy_fid) || std.mem.eql(u8, fid_b, legacy_fid)) {
            return false;
        }
    }

    const idx_a = findActiveKnownChildFrameIndex(session, fid_a);
    const idx_b = findActiveKnownChildFrameIndex(session, fid_b);

    if (std.mem.eql(u8, fid_a, session.frame_id)) {
        if (ib := idx_b) {
            const kfb = &session.known_child_frames[ib];
            return !kfb.is_cross_origin && !kfb.is_opaque;
        }
    }
    if (std.mem.eql(u8, fid_b, session.frame_id)) {
        if (ia := idx_a) {
            const kfa = &session.known_child_frames[ia];
            return !kfa.is_cross_origin && !kfa.is_opaque;
        }
    }
    if (ia := idx_a) {
        if (ib := idx_b) {
            const kfa = &session.known_child_frames[ia];
            const kfb = &session.known_child_frames[ib];
            if (kfa.is_opaque || kfb.is_opaque) return false;
            if (kfa.is_cross_origin != kfb.is_cross_origin) return false;
            if (kfa.is_cross_origin && kfb.is_cross_origin) {
                const orig_a = extractOriginSlice(kfa.url_buf[0..kfa.url_len]);
                const orig_b = extractOriginSlice(kfb.url_buf[0..kfb.url_len]);
                if (orig_a.len == 0 || orig_b.len == 0) return false;
                return std.mem.eql(u8, orig_a, orig_b);
            }
            return true;
        }
    }
    return true;
}

fn copyFidToOut(fid: []const u8, out_buf: []u8): usize {
    const copy_len = @min(fid.len, out_buf.len);
    @memcpy(out_buf[0..copy_len], fid[0..copy_len]);
    return copy_len;
}

fn parseJsFrameResult(session: *CdpPageSession, res_str: []const u8, out_buf: []u8): usize {
    if (std.mem.eql(u8, res_str, "MAIN")) {
        return copyFidToOut(session.frame_id, out_buf);
    }
    if (std.mem.startsWith(u8, res_str, "FID:")) {
        return copyFidToOut(res_str[4..], out_buf);
    }
    if (std.mem.startsWith(u8, res_str, "IDX:")) {
        const target_idx = std.fmt.parseInt(usize, res_str[4..], 10) catch 999999;
        let active_idx: usize = 0;
        let ki: usize = 0;
        while (ki < session.known_child_frame_count) : (ki += 1) {
            const kf = &session.known_child_frames[ki];
            if (!kf.active || kf.detached) continue;
            if (active_idx == target_idx && kf.frame_id_len > 0) {
                return copyFidToOut(kf.frame_id_buf[0..kf.frame_id_len], out_buf);
            }
            active_idx += 1;
        }
    }
    if (std.mem.eql(u8, res_str, "OTHER")) {
        return copyFidToOut("__other_doc__", out_buf);
    }
    return 0;
}

fn resolveNodeFrameId(session: *CdpPageSession, nid_val: i64, obj_id_opt: ([]const u8)?, out_buf: []u8): []const u8 {
    if (obj_id := obj_id_opt) {
        if (std.mem.eql(u8, obj_id, "doc:0")) {
            return session.frame_id;
        }
        if (!std.mem.startsWith(u8, obj_id, "node:") && !std.mem.startsWith(u8, obj_id, "ps:") && session.has_document) {
            const oid_val = session.ctx.newString(obj_id);
            defer session.ctx.freeValue(oid_val);
            session.ctx.setPropertyStr("__capsurf_resolve_oid", oid_val);
            const js_expr =
                "(() => {" ++
                " const oid = globalThis.__capsurf_resolve_oid;" ++
                " const o = globalThis.__capsurf_get_object ? globalThis.__capsurf_get_object(oid) : null;" ++
                " if (!o) return '';" ++
                " const d = (o.nodeType === 9) ? o : o.ownerDocument;" ++
                " if (!d || d === document) return 'MAIN';" ++
                " const dv = d.defaultView;" ++
                " if (dv && dv.__capsurf_frame_id) return 'FID:' + dv.__capsurf_frame_id;" ++
                " if (dv && dv.frameElement && dv.frameElement._capsurf_frame_id) return 'FID:' + dv.frameElement._capsurf_frame_id;" ++
                " let idx = 0; const seen = new Set();" ++
                " function walk(r, depth) {" ++
                "  if (!r) return '';" ++
                "  const cs = r.childNodes || r.children || [];" ++
                "  for (let i = 0; i < cs.length; i++) {" ++
                "   const el = cs[i];" ++
                "   if (!el || el.nodeType !== 1) continue;" ++
                "   const t = String(el.localName || el.tagName || '').toLowerCase();" ++
                "   if (t === 'template') continue;" ++
                "   if ((t === 'iframe' || t === 'frame') && !seen.has(el)) {" ++
                "    seen.add(el);" ++
                "    const cd = el._contentDocument || (el._contentWindow && el._contentWindow.document) || el.contentDocument || (el.contentWindow && el.contentWindow.document);" ++
                "    if (cd === d || (dv && el._contentWindow && dv === el._contentWindow)) {" ++
                "     if (el._capsurf_frame_id) return 'FID:' + el._capsurf_frame_id;" ++
                "     return 'IDX:' + idx;" ++
                "    }" ++
                "    idx++;" ++
                "    if (depth < 4 && cd && cd.documentElement) { const sub = walk(cd.documentElement, depth + 1); if (sub) return sub; }" ++
                "   }" ++
                "   if (el._shadowRoot) { const sr = walk(el._shadowRoot, depth); if (sr) return sr; }" ++
                "   const ch = walk(el, depth); if (ch) return ch;" ++
                "  }" ++
                "  return '';" ++
                " }" ++
                " const found = document.documentElement ? walk(document.documentElement, 1) : '';" ++
                " return found || 'OTHER';" ++
                "})()";
            const val = session.ctx.eval(js_expr);
            defer session.ctx.freeValue(val);
            const res_str = session.ctx.toCString(val);
            const n_len = parseJsFrameResult(session, res_str, out_buf);
            if (n_len > 0) return out_buf[0..n_len];
        }
    }
    if (nid_val >= 1000000 && session.has_document) {
        let js_buf: [2048]u8 = [0; 2048];
        const js_expr = std.fmt.bufPrint(
            &js_buf,
            "(() => {{ const cm = globalThis.__capsurf_child_dom_nodes; const sm = globalThis.__capsurf_sr_bmap; let o = cm ? cm[{d}] : null; if (!o && sm && globalThis.__capsurf_get_object) {{ const oid = sm.get({d}); if (oid) o = globalThis.__capsurf_get_object(oid); }} if (!o) return ''; const d = (o.nodeType === 9) ? o : o.ownerDocument; if (!d || d === document) return 'MAIN'; const dv = d.defaultView; if (dv && dv.__capsurf_frame_id) return 'FID:' + dv.__capsurf_frame_id; if (dv && dv.frameElement && dv.frameElement._capsurf_frame_id) return 'FID:' + dv.frameElement._capsurf_frame_id; let idx = 0; const seen = new Set(); function walk(r, depth) {{ if (!r) return ''; const cs = r.childNodes || r.children || []; for (let i = 0; i < cs.length; i++) {{ const el = cs[i]; if (!el || el.nodeType !== 1) continue; const t = String(el.localName || el.tagName || '').toLowerCase(); if (t === 'template') continue; if ((t === 'iframe' || t === 'frame') && !seen.has(el)) {{ seen.add(el); const cd = el._contentDocument || (el._contentWindow && el._contentWindow.document) || el.contentDocument || (el.contentWindow && el.contentWindow.document); if (cd === d || (dv && el._contentWindow && dv === el._contentWindow)) {{ if (el._capsurf_frame_id) return 'FID:' + el._capsurf_frame_id; return 'IDX:' + idx; }} idx++; if (depth < 4 && cd && cd.documentElement) {{ const sub = walk(cd.documentElement, depth + 1); if (sub) return sub; }} }} if (el._shadowRoot) {{ const sr = walk(el._shadowRoot, depth); if (sr) return sr; }} const ch = walk(el, depth); if (ch) return ch; }} return ''; }} const found = document.documentElement ? walk(document.documentElement, 1) : ''; return found || 'OTHER'; }})()",
            .{ nid_val, nid_val },
        ) catch "";
        if (js_expr.len > 0) {
            const val = session.ctx.eval(js_expr);
            defer session.ctx.freeValue(val);
            const res_str = session.ctx.toCString(val);
            const n_len = parseJsFrameResult(session, res_str, out_buf);
            if (n_len > 0) return out_buf[0..n_len];
        }
    }
    if (nid_val == 999999 || (nid_val > 0 && @as(usize, @intCast(nid_val)) < session.doc.nodes.items.len)) {
        return session.frame_id;
    }
    return "";
}

pub struct DomHandler {
    pub static fn handle(
        session: *CdpPageSession,
        req: CdpRequest,
        out_buf: []u8,
        out_events: *[MAX_CDP_EVENTS]CdpEvent,
        event_count: *usize,
    ): CdpResponse {
        if (std.mem.eql(u8, req.method, "DOM.enable")) {
            session.dom_enabled = true;
            if (!session.dom_document_sent) {
                session.markDocumentObserved(-1);
            }
            if (req.session_id.len > 0 && session.target_registry.findFrameIdBySessionId(req.session_id) != null) {
                if (event_count.* < out_events.len) {
                    let ev = CdpEvent.create("DOM.documentUpdated", "{}");
                    ev.session_id = req.session_id;
                    out_events[event_count.*] = ev;
                    event_count.* += 1;
                }
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "DOM.disable")) {
            if (!session.dom_enabled) {
                session.fetch_enabled = false;
                clearLearnedScripts();
            }
            session.setDomDisabled();
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "DOM.focus")) {
            let nid = @as(NodeId, @intCast(@max(0, extractJsonInt(req.params_raw, "nodeId") ?? 0)));
            if (nid == NULL_NODE) {
                if (bnid := extractJsonInt(req.params_raw, "backendNodeId")) {
                    if (bnid > 0) nid = @as(NodeId, @intCast(bnid));
                }
            }
            if (nid == NULL_NODE) {
                if (obj_id := extractJsonString(req.params_raw, "objectId")) {
                    nid = resolveNodeIdFromObjectId(session, obj_id);
                }
            }
            if (nid != NULL_NODE) {
                session.focused_node = nid;
                session.doc.focused_node = nid;
                if (session.has_document) {
                    const obj_id_s = extractJsonString(req.params_raw, "objectId") ?? "";
                    let js_buf: [512]u8 = [0; 512];
                    const js_expr = std.fmt.bufPrint(
                        &js_buf,
                        "(() => {{ const g = globalThis.__capsurf_get_object; const f = globalThis.__capsurf_find_node_by_id; const el = (typeof g === 'function' ? (('{s}'.length > 0 ? g('{s}') : null) || g('node:{d}')) : null) || (typeof f === 'function' ? f({d}) : null); if (el && typeof el.focus === 'function') el.focus(); }})()",
                        .{ obj_id_s, obj_id_s, nid, nid },
                    ) catch null;
                    if (e := js_expr) {
                        const v = session.ctx.eval(e);
                        session.ctx.freeValue(v);
                        session.ctx.drainJobs();
                    }
                    let polled_binding_evts: [4]CdpEvent = [CdpEvent{}; 4];
                    const polled_b_count = session.pollBindingEvents(&polled_binding_evts);
                    let b_i: usize = 0;
                    while (b_i < polled_b_count && event_count.* < out_events.len) : (b_i += 1) {
                        out_events[event_count.*] = polled_binding_evts[b_i];
                        event_count.* += 1;
                    }
                }
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "DOM.hideHighlight") ||
            std.mem.eql(u8, req.method, "DOM.highlightNode") ||
            std.mem.eql(u8, req.method, "DOM.setInspectedNode") ||
            std.mem.eql(u8, req.method, "DOM.discardSearchResults") ||
            std.mem.eql(u8, req.method, "DOM.markUndoableState") ||
            std.mem.eql(u8, req.method, "DOM.undo") ||
            std.mem.eql(u8, req.method, "DOM.redo") ||
            std.mem.eql(u8, req.method, "DOM.setNodeStackTracesEnabled") ||
            std.mem.eql(u8, req.method, "DOM.requestChildNodes")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "DOM.getDocument")) {
            const depth = extractJsonInt(req.params_raw, "depth") ?? -1;
            if (req.session_id.len > 0 && session.target_registry.findFrameIdBySessionId(req.session_id) != null) {
                const child_dom_script =
                    "(function(){" ++
                    "if(typeof document==='undefined'||!document.querySelectorAll)return '';" ++
                    "var ifr=document.querySelector('iframe');" ++
                    "var cwin=(ifr&&ifr.contentWindow)?ifr.contentWindow:null;" ++
                    "var cdoc=(ifr&&ifr.contentDocument)?ifr.contentDocument:((cwin&&cwin.document)?cwin.document:null);" ++
                    "if(!cdoc)return '';" ++
                    "var curl=String((cdoc&&cdoc.URL)||(cwin&&cwin.location&&cwin.location.href)||(ifr&&ifr.getAttribute&&ifr.getAttribute('src'))||'');" ++
                    "var map=globalThis.__capsurf_child_dom_nodes=globalThis.__capsurf_child_dom_nodes||{};" ++
                    "var nextId=1000001;" ++
                    "function ser(n,inSvg){" ++
                    "if(!n)return null;" ++
                    "var nt=n.nodeType||1;" ++
                    "var nid=nextId++;" ++
                    "map[nid]=n;" ++
                    "var obj={nodeId:nid,backendNodeId:nid,nodeType:nt,nodeName:n.nodeName||'',localName:n.localName||'',nodeValue:n.nodeValue||''};" ++
                    "if(nt===1){" ++
                    "var tag=n.localName||String(n.tagName||'').toLowerCase();" ++
                    "var svgNow=inSvg||tag==='svg';" ++
                    "obj.nodeName=svgNow?tag:String(n.tagName||tag).toUpperCase();" ++
                    "obj.localName=tag;obj.nodeValue='';" ++
                    "var attrs=[];" ++
                    "if(n.attributes&&typeof n.attributes.length==='number'){for(var i=0;i<n.attributes.length;i++){var a=n.attributes[i];if(a&&typeof a.name==='string'){attrs.push(a.name,String(a.value==null?'':a.value));}}}" ++
                    "else if(typeof n.getAttributeNames==='function'){var ns=n.getAttributeNames();for(var i=0;i<ns.length;i++)attrs.push(ns[i],String(n.getAttribute(ns[i])||''));}" ++
                    "else if(n._attributes&&typeof n._attributes==='object'){var ks=Object.keys(n._attributes);for(var i=0;i<ks.length;i++)attrs.push(ks[i],String(n._attributes[ks[i]]||''));}" ++
                    "if(attrs.indexOf('style')===-1&&typeof n.getAttribute==='function'){var st=n.getAttribute('style');if(st)attrs.push('style',String(st));}" ++
                    "if(attrs.indexOf('style')===-1&&n.style&&n.style.cssText){attrs.push('style',String(n.style.cssText));}" ++
                    "obj.attributes=attrs;" ++
                    "}else if(nt===9){obj.nodeName='#document';obj.localName='';obj.nodeValue='';obj.documentURL=curl;}" ++
                    "var kids=n.childNodes||[];obj.childNodeCount=kids.length;" ++
                    "if(kids.length>0){obj.children=[];var svgChild=inSvg||(nt===1&&(n.localName||String(n.tagName||'').toLowerCase())==='svg');for(var j=0;j<kids.length;j++){var c=ser(kids[j],svgChild);if(c)obj.children.push(c);}}" ++
                    "return obj;}" ++
                    "var rootObj=ser(cdoc,false);" ++
                    "if(!rootObj)return '';" ++
                    "return JSON.stringify({root:rootObj});})()";
                const cd_val = session.ctx.eval(child_dom_script);
                defer session.ctx.freeValue(cd_val);
                const cd_slice = session.ctx.toCString(cd_val);
                if (cd_slice.len > 0) {
                    const res = std.fmt.bufPrint(out_buf, "{s}", .{cd_slice}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                    return CdpResponse.ok(req.id, res);
                }
            }

            const root_node = session.doc.root_element;
            let list = std.ArrayListUnmanaged(u8).empty;
            defer list.deinit(session.allocator);

            let sr_entries: [32]ShadowRootEntry = [ShadowRootEntry{}; 32];
            let sr_count: usize = 0;
            let sr_raw_owned: ([]u8)? = null;
            defer if (sr_raw_owned != null) session.allocator.free(sr_raw_owned.?);

            if (depth != 0 && session.has_document) {
                const sr_script =
                    "(function(){" ++
                    "if(typeof document==='undefined'||!document.querySelectorAll)return '';" ++
                    "var nextId=900000;" ++
                    "function ser(n,inSvg){" ++
                    "if(!n)return null;" ++
                    "var nt=n.nodeType||1;" ++
                    "var nid=(typeof n._nid==='number'&&n._nid>0)?n._nid:(nextId++);" ++
                    "var obj={nodeId:nid,backendNodeId:nid,nodeType:nt,nodeName:n.nodeName||'',localName:n.localName||'',nodeValue:n.nodeValue||''};" ++
                    "if(nt===1){" ++
                    "var tag=n.localName||String(n.tagName||'').toLowerCase();" ++
                    "var svgNow=inSvg||tag==='svg';" ++
                    "obj.nodeName=svgNow?tag:String(n.tagName||tag).toUpperCase();" ++
                    "obj.localName=tag;obj.nodeValue='';" ++
                    "var attrs=[];" ++
                    "if(n.attributes&&typeof n.attributes.length==='number'){for(var i=0;i<n.attributes.length;i++){var a=n.attributes[i];if(a&&typeof a.name==='string'){attrs.push(a.name,String(a.value==null?'':a.value));}}}" ++
                    "else if(typeof n.getAttributeNames==='function'){var ns=n.getAttributeNames();for(var i=0;i<ns.length;i++)attrs.push(ns[i],String(n.getAttribute(ns[i])||''));}" ++
                    "else if(n._attributes&&typeof n._attributes==='object'){var ks=Object.keys(n._attributes);for(var i=0;i<ks.length;i++)attrs.push(ks[i],String(n._attributes[ks[i]]||''));}" ++
                    "if(attrs.indexOf('style')===-1&&typeof n.getAttribute==='function'){var st=n.getAttribute('style');if(st)attrs.push('style',String(st));}" ++
                    "if(attrs.indexOf('style')===-1&&n.style&&n.style.cssText){attrs.push('style',String(n.style.cssText));}" ++
                    "obj.attributes=attrs;" ++
                    "}else if(nt===11){obj.nodeName='#document-fragment';obj.localName='';obj.nodeValue='';if(n.mode)obj.shadowRootType=n.mode;}" ++
                    "var kids=n.childNodes||[];obj.childNodeCount=kids.length;" ++
                    "if(kids.length>0){obj.children=[];var svgChild=inSvg||(nt===1&&(n.localName||String(n.tagName||'').toLowerCase())==='svg');for(var j=0;j<kids.length;j++){var c=ser(kids[j],svgChild);if(c)obj.children.push(c);}}" ++
                    "if(nt===1&&n.shadowRoot){var sr=ser(n.shadowRoot,false);if(sr)obj.shadowRoots=[sr];}" ++
                    "return obj;}" ++
                    "var all=document.querySelectorAll('*');var out='';" ++
                    "for(var k=0;k<all.length;k++){var el=all[k];if(el&&el.shadowRoot&&typeof el._nid==='number'&&el._nid>0){var sr=ser(el.shadowRoot,false);if(sr)out+=el._nid+'\\x1e'+JSON.stringify(sr)+'\\x1f';}}" ++
                    "return out;})()";
                const sr_val = session.ctx.eval(sr_script);
                defer session.ctx.freeValue(sr_val);
                const sr_slice = session.ctx.toCString(sr_val);
                if (sr_slice.len > 0) {
                    const duped = session.allocator.dupe(u8, sr_slice) catch null;
                    if (duped != null) {
                        sr_raw_owned = duped;
                        let rec_it = std.mem.splitScalar(u8, duped.?, 0x1f);
                        while (rec := rec_it.next()) {
                            if (rec.len == 0 || sr_count >= sr_entries.len) continue;
                            if (sep := std.mem.indexOfScalar(u8, rec, 0x1e)) {
                                const parsed_nid = std.fmt.parseInt(u32, rec[0..sep], 10) catch 0;
                                if (parsed_nid > 0 && sep + 1 < rec.len) {
                                    sr_entries[sr_count] = ShadowRootEntry{
                                        nid: parsed_nid,
                                        json: rec[sep + 1 ..],
                                    };
                                    sr_count += 1;
                                }
                            }
                        }
                    }
                }
            }

            let iframe_entries: [32]IframeFrameEntry = [IframeFrameEntry{}; 32];
            const iframe_count = collectDocIframeEntries(session, &iframe_entries);

            const cnt: u32 = if (root_node != NULL_NODE) 1 else 0;
            let root_buf: [256]u8 = undefined;
            const root_s = std.fmt.bufPrint(
                &root_buf,
                "{{\"root\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":9,\"nodeName\":\"#document\",\"localName\":\"\",\"nodeValue\":\"\",\"documentURL\":",
                .{ root_node, root_node },
            ) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            list.appendSlice(session.allocator, root_s) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            appendJsonEscaped(session.allocator, &list, session.url) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            let cnt_buf: [64]u8 = undefined;
            const cnt_s = std.fmt.bufPrint(&cnt_buf, ",\"childNodeCount\":{d}", .{cnt}) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            list.appendSlice(session.allocator, cnt_s) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            if (depth != 0 && root_node != NULL_NODE) {
                list.appendSlice(session.allocator, ",\"children\":[") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                serializeCdpNodeWithOptions(session.allocator, &session.doc, root_node, depth, false, sr_entries[0..sr_count], iframe_entries[0..iframe_count], &list) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                list.appendSlice(session.allocator, "]") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            }
            list.appendSlice(session.allocator, "}}") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            session.markDocumentObserved(depth);
            const res = std.fmt.bufPrint(out_buf, "{s}", .{list.items}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.querySelector")) {
            const sel = extractJsonString(req.params_raw, "selector") ?? "";
            const matched_id = session.querySelector(sel);
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"nodeId\":{d}}}",
                .{matched_id},
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.querySelectorAll")) {
            const sel = extractJsonString(req.params_raw, "selector") ?? "";
            let node_ids: [128]NodeId = [@as(NodeId, 0); 128];
            const count = session.querySelectorAll(sel, &node_ids);

            let str_buf: [1024]u8 = [0; 1024];
            let offset: usize = 0;
            const header = "{\"nodeIds\":[";
            @memcpy(str_buf[0..header.len], header);
            offset += header.len;

            let i: usize = 0;
            while (i < count) : (i += 1) {
                if (i > 0) {
                    str_buf[offset] = ',';
                    offset += 1;
                }
                const num = std.fmt.bufPrint(str_buf[offset..], "{d}", .{node_ids[i]}) catch break;
                offset += num.len;
            }

            if (offset + 2 <= str_buf.len) {
                str_buf[offset] = ']';
                str_buf[offset + 1] = '}';
                offset += 2;
            }

            const res = std.fmt.bufPrint(out_buf, "{s}", .{str_buf[0..offset]}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.getNodeForLocation")) {
            session.ensureLayout();
            let x: f32 = 0.0;
            let y: f32 = 0.0;
            if (xf := extractJsonFloat(req.params_raw, "x")) {
                x = @floatCast(xf);
            } else if (xi := extractJsonInt(req.params_raw, "x")) {
                x = @floatFromInt(xi);
            }
            if (yf := extractJsonFloat(req.params_raw, "y")) {
                y = @floatCast(yf);
            } else if (yi := extractJsonInt(req.params_raw, "y")) {
                y = @floatFromInt(yi);
            }
            const hit_id = hitTest(session, x, y);
            if (hit_id == NULL_NODE) {
                return CdpResponse.err(req.id, -32000, "No node found at given location");
            }
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"backendNodeId\":{d},\"frameId\":\"{s}\",\"nodeId\":{d}}}",
                .{ hit_id, session.frame_id, hit_id },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.getBoxModel")) {
            let node_id_val: i64 = 0;
            if (n_id := extractJsonInt(req.params_raw, "nodeId")) {
                node_id_val = n_id;
            } else if (b_id := extractJsonInt(req.params_raw, "backendNodeId")) {
                node_id_val = b_id;
            } else if (obj_id := extractJsonString(req.params_raw, "objectId")) {
                const resolved = resolveNodeIdFromObjectId(session, obj_id);
                if (resolved > 0) node_id_val = @as(i64, resolved);
            }
            if (node_id_val <= 0 || @as(usize, @intCast(node_id_val)) >= session.doc.nodes.items.len) {
                const obj_s = extractJsonString(req.params_raw, "objectId") ?? "";
                let rx: f32 = 0.0;
                let ry: f32 = 0.0;
                let rw: f32 = 0.0;
                let rh: f32 = 0.0;
                if (queryJsObjectRect(session, obj_s, node_id_val, &rx, &ry, &rw, &rh)) {
                    return formatJsBoxModelResp(rx, ry, rw, rh, req.id, out_buf);
                }
                return CdpResponse.err(req.id, -32000, "Could not compute box model");
            }
            const node_id: NodeId = @as(NodeId, @intCast(node_id_val));
            const node_type = session.doc.nodes.items[@as(usize, node_id)].node_type;
            if (node_type == .Comment) {
                return CdpResponse.err(req.id, -32000, "Could not compute box model");
            }
            session.ensureLayout();
            let sx: f32 = 0.0;
            let sy: f32 = 0.0;
            let cw: f32 = 0.0;
            let ch: f32 = 0.0;
            session.updateLayoutMetrics(&sx, &sy, &cw, &ch);
            applyAncestorScrollAndFixed(session, node_id, &sx, &sy);
            const bm = session.getBoxModel(node_id);
            if (bm.width <= 0.0 && bm.height <= 0.0) {
                const obj_s = extractJsonString(req.params_raw, "objectId") ?? "";
                let rx: f32 = 0.0;
                let ry: f32 = 0.0;
                let rw: f32 = 0.0;
                let rh: f32 = 0.0;
                if (queryJsObjectRect(session, obj_s, node_id_val, &rx, &ry, &rw, &rh)) {
                    return formatJsBoxModelResp(rx, ry, rw, rh, req.id, out_buf);
                }
                return CdpResponse.err(req.id, -32000, "Could not compute box model");
            }

            const w_int = @as(i64, @intFromFloat(@round(bm.width)));
            const h_int = @as(i64, @intFromFloat(@round(bm.height)));

            let c_buf: [256]u8 = [0; 256];
            let p_buf: [256]u8 = [0; 256];
            let b_buf: [256]u8 = [0; 256];
            let m_buf: [256]u8 = [0; 256];
            const cq = formatQuad(shiftQuadScroll(bm.content, sx, sy), &c_buf);
            const pq = formatQuad(shiftQuadScroll(bm.padding, sx, sy), &p_buf);
            const bq = formatQuad(shiftQuadScroll(bm.border, sx, sy), &b_buf);
            const mq = formatQuad(shiftQuadScroll(bm.margin, sx, sy), &m_buf);

            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"model\":{{\"content\":{s},\"padding\":{s},\"border\":{s},\"margin\":{s},\"width\":{d},\"height\":{d}}}}}",
                .{ cq, pq, bq, mq, w_int, h_int },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.getContentQuads")) {
            let node_id_val: i64 = 0;
            if (n_id := extractJsonInt(req.params_raw, "nodeId")) {
                node_id_val = n_id;
            } else if (b_id := extractJsonInt(req.params_raw, "backendNodeId")) {
                node_id_val = b_id;
            } else if (obj_id := extractJsonString(req.params_raw, "objectId")) {
                const resolved = resolveNodeIdFromObjectId(session, obj_id);
                if (resolved > 0) node_id_val = @as(i64, resolved);
            }
            if (node_id_val <= 0 || @as(usize, @intCast(node_id_val)) >= session.doc.nodes.items.len) {
                const obj_s = extractJsonString(req.params_raw, "objectId") ?? "";
                let rx: f32 = 0.0;
                let ry: f32 = 0.0;
                let rw: f32 = 0.0;
                let rh: f32 = 0.0;
                if (queryJsObjectRect(session, obj_s, node_id_val, &rx, &ry, &rw, &rh)) {
                    return formatJsQuadsResp(rx, ry, rw, rh, req.id, out_buf);
                }
                return CdpResponse.ok(req.id, "{\"quads\":[]}");
            }
            const node_id: NodeId = @as(NodeId, @intCast(node_id_val));
            const node_type = session.doc.nodes.items[@as(usize, node_id)].node_type;
            if (node_type == .Comment) {
                return CdpResponse.ok(req.id, "{\"quads\":[]}");
            }
            session.ensureLayout();
            let sx: f32 = 0.0;
            let sy: f32 = 0.0;
            let cw: f32 = 0.0;
            let ch: f32 = 0.0;
            session.updateLayoutMetrics(&sx, &sy, &cw, &ch);
            applyAncestorScrollAndFixed(session, node_id, &sx, &sy);
            const bm = session.getBoxModel(node_id);
            if (bm.width <= 0.0 && bm.height <= 0.0) {
                const obj_s = extractJsonString(req.params_raw, "objectId") ?? "";
                let rx: f32 = 0.0;
                let ry: f32 = 0.0;
                let rw: f32 = 0.0;
                let rh: f32 = 0.0;
                if (queryJsObjectRect(session, obj_s, node_id_val, &rx, &ry, &rw, &rh)) {
                    return formatJsQuadsResp(rx, ry, rw, rh, req.id, out_buf);
                }
                return CdpResponse.ok(req.id, "{\"quads\":[]}");
            }

            const c0 = bm.content[0] - sx;
            const c1 = bm.content[1] - sy;
            const c2 = bm.content[2] - sx;
            const c3 = bm.content[3] - sy;
            const c4 = bm.content[4] - sx;
            const c5 = bm.content[5] - sy;
            const c6 = bm.content[6] - sx;
            const c7 = bm.content[7] - sy;

            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"quads\":[[{d},{d},{d},{d},{d},{d},{d},{d}]]}}",
                .{ c0, c1, c2, c3, c4, c5, c6, c7 },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.scrollIntoViewIfNeeded")) {
            let node_id_val: i64 = 0;
            let obj_id_val: []const u8 = "";
            if (n_id := extractJsonInt(req.params_raw, "nodeId")) {
                node_id_val = n_id;
            } else if (b_id := extractJsonInt(req.params_raw, "backendNodeId")) {
                node_id_val = b_id;
            } else if (obj_id := extractJsonString(req.params_raw, "objectId")) {
                obj_id_val = obj_id;
                const resolved = resolveNodeIdFromObjectId(session, obj_id);
                if (resolved > 0) node_id_val = @as(i64, resolved);
            }
            if (node_id_val <= 0 || @as(usize, @intCast(node_id_val)) >= session.doc.nodes.items.len) {
                let rx: f32 = 0.0;
                let ry: f32 = 0.0;
                let rw: f32 = 0.0;
                let rh: f32 = 0.0;
                if (obj_id_val.len == 0 || !queryJsObjectRect(session, obj_id_val, node_id_val, &rx, &ry, &rw, &rh)) {
                    return CdpResponse.err(req.id, -32000, "Node not found");
                }
            }
            const node_id: NodeId = if (node_id_val > 0 && @as(usize, @intCast(node_id_val)) < session.doc.nodes.items.len) @as(NodeId, @intCast(node_id_val)) else NULL_NODE;

            // Execute scroll into view in JS environment
            if (session.has_document) {
                let conn_buf: [1024]u8 = [0; 1024];
                const conn_js = std.fmt.bufPrint(
                    &conn_buf,
                    "(() => {{ let el = null; if ('{s}'.length > 0 && globalThis.__capsurf_get_object) {{ el = globalThis.__capsurf_get_object('{s}'); }} if (!el && {d} > 0) {{ function f(r, nid) {{ if (!r) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; }} return null; }} el = f(document, {d}); }} return (el && el.isConnected !== false) ? 1 : 0; }})()",
                    .{ obj_id_val, obj_id_val, node_id, node_id },
                ) catch null;
                if (ce := conn_js) {
                    const cval = session.ctx.eval(ce);
                    const is_conn = !(cval.tag == 0 && cval.u.uint64 == 0);
                    session.ctx.freeValue(cval);
                    if (!is_conn) {
                        return CdpResponse.err(req.id, -32000, "Node is detached from document");
                    }
                }
                session.ensureLayout();
                let has_box = false;
                if (node_id != NULL_NODE) {
                    const bm = session.getBoxModel(node_id);
                    has_box = (bm.width > 0.0 || bm.height > 0.0);
                }
                if (!has_box) {
                    let rx: f32 = 0.0;
                    let ry: f32 = 0.0;
                    let rw: f32 = 0.0;
                    let rh: f32 = 0.0;
                    if (!queryJsObjectRect(session, obj_id_val, node_id_val, &rx, &ry, &rw, &rh)) {
                        return CdpResponse.err(req.id, -32000, "Node does not have a layout object");
                    }
                }

                let has_rect = false;
                let rect_x: f32 = 0.0;
                let rect_y: f32 = 0.0;
                let rect_w: f32 = 0.0;
                let rect_h: f32 = 0.0;
                if (rect_pos := std.mem.indexOf(u8, req.params_raw, "\"rect\"")) {
                    const after_key = req.params_raw[rect_pos + 6 ..];
                    if (ob := std.mem.indexOfScalar(u8, after_key, '{')) {
                        if (cb := std.mem.indexOfScalar(u8, after_key[ob..], '}')) {
                            const rect_json = after_key[ob .. ob + cb + 1];
                            has_rect = true;
                            rect_x = extractJsonFloat(rect_json, "x") ?? 0.0;
                            rect_y = extractJsonFloat(rect_json, "y") ?? 0.0;
                            rect_w = extractJsonFloat(rect_json, "width") ?? 0.0;
                            rect_h = extractJsonFloat(rect_json, "height") ?? 0.0;
                        }
                    }
                }

                let js_buf: [4096]u8 = [0; 4096];
                const js_expr = if (has_rect) (std.fmt.bufPrint(
                    &js_buf,
                    "(() => {{ let el = null; if ('{s}'.length > 0 && globalThis.__capsurf_get_object) {{ el = globalThis.__capsurf_get_object('{s}'); }} if (!el && {d} > 0) {{ function f(r, nid) {{ if (!r) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; }} return null; }} el = f(document, {d}); }} if (!el) return; const doc = el.ownerDocument || document; const win = (doc && doc.defaultView) || window; const er = el.getBoundingClientRect(); let tx = er.left + ({d}); let ty = er.top + ({d}); const tw = {d}; const th = {d}; let cur = el.parentElement; let safety = 0; while (cur && cur !== doc.body && cur !== doc.documentElement && safety < 50) {{ safety++; const st = win.getComputedStyle ? win.getComputedStyle(cur) : cur.style; const ox = (st && (st.overflowX || st.overflow)) || ''; const oy = (st && (st.overflowY || st.overflow)) || ''; const sxOk = (ox === 'auto' || ox === 'scroll' || ox === 'hidden') && cur.clientWidth > 0 && cur.scrollWidth > cur.clientWidth; const syOk = (oy === 'auto' || oy === 'scroll' || oy === 'hidden') && cur.clientHeight > 0 && cur.scrollHeight > cur.clientHeight; if (sxOk || syOk) {{ const cr = cur.getBoundingClientRect(); if (syOk && (ty < cr.top || ty + th > cr.top + cur.clientHeight)) {{ const curT = cur.scrollTop || 0; const inY = (ty - cr.top) + curT; const nextT = Math.max(0, Math.min(inY + th / 2 - cur.clientHeight / 2, cur.scrollHeight - cur.clientHeight)); cur.scrollTop = nextT; ty -= (nextT - curT); }} if (sxOk && (tx < cr.left || tx + tw > cr.left + cur.clientWidth)) {{ const curL = cur.scrollLeft || 0; const inX = (tx - cr.left) + curL; const nextL = Math.max(0, Math.min(inX + tw / 2 - cur.clientWidth / 2, cur.scrollWidth - cur.clientWidth)); cur.scrollLeft = nextL; tx -= (nextL - curL); }} }} cur = cur.parentElement; }} const vw = win.innerWidth || 1280; const vh = win.innerHeight || 720; if (ty < 0 || ty + th > vh || tx < 0 || tx + tw > vw) {{ const wx = win.scrollX || 0; const wy = win.scrollY || 0; const nwx = (tx < 0 || tx + tw > vw) ? Math.max(0, tx + wx + tw / 2 - vw / 2) : wx; const nwy = (ty < 0 || ty + th > vh) ? Math.max(0, ty + wy + th / 2 - vh / 2) : wy; if (win.scrollTo) win.scrollTo(Math.round(nwx), Math.round(nwy)); }} let cw = win; let fs = 0; while (cw && cw.frameElement && fs < 16) {{ fs++; const fe = cw.frameElement; if (typeof fe.scrollIntoViewIfNeeded === 'function') fe.scrollIntoViewIfNeeded(true); else if (typeof fe.scrollIntoView === 'function') fe.scrollIntoView({{ block: 'center', inline: 'center' }}); cw = (cw.parent && cw.parent !== cw) ? cw.parent : null; }} }})()",
                    .{ obj_id_val, obj_id_val, node_id, node_id, rect_x, rect_y, rect_w, rect_h },
                ) catch null) else (std.fmt.bufPrint(
                    &js_buf,
                    "(() => {{ let el = null; if ('{s}'.length > 0 && globalThis.__capsurf_get_object) {{ el = globalThis.__capsurf_get_object('{s}'); }} if (!el && {d} > 0) {{ function f(r, nid) {{ if (!r) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; }} return null; }} el = f(document, {d}); }} if (el) {{ if (typeof el.scrollIntoViewIfNeeded === 'function') {{ el.scrollIntoViewIfNeeded(true); }} else if (typeof el.scrollIntoView === 'function') {{ el.scrollIntoView({{ block: 'center', inline: 'center' }}); }} let cw = (el.ownerDocument && el.ownerDocument.defaultView) || null; let fs = 0; while (cw && cw.frameElement && fs < 16) {{ fs++; const fe = cw.frameElement; if (typeof fe.scrollIntoViewIfNeeded === 'function') fe.scrollIntoViewIfNeeded(true); else if (typeof fe.scrollIntoView === 'function') fe.scrollIntoView({{ block: 'center', inline: 'center' }}); cw = (cw.parent && cw.parent !== cw) ? cw.parent : null; }} }} }})()",
                    .{ obj_id_val, obj_id_val, node_id, node_id },
                ) catch null);
                if (e := js_expr) {
                    const val = session.ctx.eval(e);
                    session.ctx.freeValue(val);
                }
                let sx: f32 = 0.0;
                let sy: f32 = 0.0;
                let cw: f32 = 0.0;
                let ch: f32 = 0.0;
                session.updateLayoutMetrics(&sx, &sy, &cw, &ch);
            }

            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "DOM.getFlattenedDocument")) {
            const root_node = session.doc.root_element;
            const depth = extractJsonInt(req.params_raw, "depth") ?? -1;
            let list = std.ArrayListUnmanaged(u8).empty;
            defer list.deinit(session.allocator);
            const cnt: u32 = if (root_node != NULL_NODE) 1 else 0;
            let root_buf: [256]u8 = undefined;
            const root_s = std.fmt.bufPrint(
                &root_buf,
                "{{\"nodes\":[{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":9,\"nodeName\":\"#document\",\"localName\":\"\",\"nodeValue\":\"\",\"childNodeCount\":{d}",
                .{ root_node, root_node, cnt },
            ) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            list.appendSlice(session.allocator, root_s) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            if (depth != 0 && root_node != NULL_NODE) {
                list.appendSlice(session.allocator, ",\"children\":[") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                serializeCdpNode(session.allocator, &session.doc, root_node, depth, &list) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                list.appendSlice(session.allocator, "]") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            }
            list.appendSlice(session.allocator, "}]}") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            const res = std.fmt.bufPrint(out_buf, "{s}", .{list.items}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.resolveNode")) {
            const obj_id_opt = extractJsonString(req.params_raw, "objectId");
            const exec_ctx_opt = extractJsonInt(req.params_raw, "executionContextId");
            let nid_val = extractJsonInt(req.params_raw, "nodeId") ?? extractJsonInt(req.params_raw, "backendNodeId") ?? 0;
            if (nid_val == 0) {
                if (obj_id := obj_id_opt) {
                    if (std.mem.eql(u8, obj_id, "doc:0")) {
                        nid_val = 999999;
                    } else {
                        const r_nid = resolveNodeIdFromObjectId(session, obj_id);
                        if (r_nid > 0) nid_val = @as(i32, @intCast(r_nid));
                    }
                }
            }
            const target_frame_id = resolveExecutionContextFrameId(session, req.session_id, exec_ctx_opt);
            if (target_frame_id.len > 0) {
                let node_fid_buf: [64]u8 = [0; 64];
                const node_frame_id = resolveNodeFrameId(session, nid_val, obj_id_opt, &node_fid_buf);
                if (!canFramesScriptEachOther(session, target_frame_id, node_frame_id)) {
                    return CdpResponse.err(req.id, -32000, "Unable to adopt element handle from a different document");
                }
            }
            if (nid_val == 999999 && session.has_document) {
                if (exec_ctx := extractJsonInt(req.params_raw, "executionContextId")) {
                    if (exec_ctx > 1) {
                        const iso_v = session.ctx.eval("(function(){globalThis.__capsurf_isolated_oids=globalThis.__capsurf_isolated_oids||new Set();globalThis.__capsurf_isolated_oids.add('doc:0');})()");
                        session.ctx.freeValue(iso_v);
                    }
                }
                return CdpResponse.ok(req.id, "{\"object\":{\"type\":\"object\",\"subtype\":\"node\",\"className\":\"HTMLDocument\",\"description\":\"#document\",\"objectId\":\"doc:0\"}}");
            }
            if (nid_val >= 1000000 && session.has_document) {
                let sr_buf: [1024]u8 = [0; 1024];
                const sr_js = std.fmt.bufPrint(
                    &sr_buf,
                    "(() => {{ const m = globalThis.__capsurf_sr_bmap; let oid = m ? (m.get({d}) || '') : ''; let o = oid && globalThis.__capsurf_get_object ? globalThis.__capsurf_get_object(oid) : null; if (!o && globalThis.__capsurf_child_dom_nodes) {{ o = globalThis.__capsurf_child_dom_nodes[{d}] || null; if (o && globalThis.__capsurf_store_object) {{ oid = globalThis.__capsurf_store_object(o) || ''; if (oid && m) m.set({d}, oid); }} }} if (!oid) return ''; const nt = o ? o.nodeType : 11; return String(nt) + '|' + oid; }})()",
                    .{ nid_val, nid_val, nid_val },
                ) catch "";
                if (sr_js.len > 0) {
                    const sr_val = session.ctx.eval(sr_js);
                    defer session.ctx.freeValue(sr_val);
                    const sr_res = session.ctx.toCString(sr_val);
                    if (sep := std.mem.indexOfScalar(u8, sr_res, '|')) {
                        const nt_str = sr_res[0..sep];
                        const oid = sr_res[sep + 1 ..];
                        if (oid.len > 0) {
                            if (exec_ctx := extractJsonInt(req.params_raw, "executionContextId")) {
                                if (exec_ctx > 1) {
                                    let iso_buf: [256]u8 = [0; 256];
                                    const iso_js = std.fmt.bufPrint(
                                        &iso_buf,
                                        "(function(){{globalThis.__capsurf_isolated_oids=globalThis.__capsurf_isolated_oids||new Set();globalThis.__capsurf_isolated_oids.add('{s}');}})()",
                                        .{oid},
                                    ) catch "";
                                    if (iso_js.len > 0) {
                                        const v = session.ctx.eval(iso_js);
                                        session.ctx.freeValue(v);
                                    }
                                }
                            }
                            const cls: []const u8 = if (std.mem.eql(u8, nt_str, "1"))
                                "Element"
                            else if (std.mem.eql(u8, nt_str, "3"))
                                "Text"
                            else if (std.mem.eql(u8, nt_str, "8"))
                                "Comment"
                            else if (std.mem.eql(u8, nt_str, "9"))
                                "HTMLDocument"
                            else
                                "ShadowRoot";
                            const desc: []const u8 = if (std.mem.eql(u8, nt_str, "9")) "#document" else cls;
                            const res = std.fmt.bufPrint(
                                out_buf,
                                "{{\"object\":{{\"type\":\"object\",\"subtype\":\"node\",\"className\":\"{s}\",\"description\":\"{s}\",\"objectId\":\"{s}\"}}}}",
                                .{ cls, desc, oid },
                            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                            return CdpResponse.ok(req.id, res);
                        }
                    }
                }
            }
            if (nid_val <= 0 || @as(usize, @intCast(nid_val)) >= session.doc.nodes.items.len) {
                return CdpResponse.err(req.id, -32000, "No node with given id found");
            }
            if (exec_ctx := extractJsonInt(req.params_raw, "executionContextId")) {
                if (exec_ctx > 1 && session.has_document) {
                    let iso_buf: [256]u8 = [0; 256];
                    const iso_js = std.fmt.bufPrint(
                        &iso_buf,
                        "(function(){{globalThis.__capsurf_isolated_oids=globalThis.__capsurf_isolated_oids||new Set();globalThis.__capsurf_isolated_oids.add('node:{d}');}})()",
                        .{nid_val},
                    ) catch "";
                    if (iso_js.len > 0) {
                        const v = session.ctx.eval(iso_js);
                        session.ctx.freeValue(v);
                    }
                }
            }
            const res_cls = if (nid_val > 0 && @as(usize, @intCast(nid_val)) < session.doc.nodes.items.len)
                switch (session.doc.nodes.items[@as(usize, @intCast(nid_val))].node_type) {
                    .Text => "Text",
                    .Comment => "Comment",
                    else => "Element",
                }
            else
                "Element";
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"object\":{{\"type\":\"object\",\"subtype\":\"node\",\"className\":\"{s}\",\"description\":\"Node#{d}\",\"objectId\":\"node:{d}\"}}}}",
                .{ res_cls, nid_val, nid_val },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.requestNode")) {
            const obj_id = extractJsonString(req.params_raw, "objectId") ?? "";
            const nid = resolveNodeIdFromObjectId(session, obj_id);
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"nodeId\":{d}}}",
                .{nid},
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.getOuterHTML")) {
            const nid_val = extractJsonInt(req.params_raw, "nodeId") ?? 0;
            if (nid_val >= 1000000 && req.session_id.len > 0 && session.target_registry.findFrameIdBySessionId(req.session_id) != null) {
                let js_buf: [512]u8 = [0; 512];
                const js_expr = std.fmt.bufPrint(
                    &js_buf,
                    "(function(){{var m=globalThis.__capsurf_child_dom_nodes;var n=m?m[{d}]:null;if(!n)return '';var h=n.outerHTML;if(typeof h!=='string'){{var t=String(n.localName||n.tagName||'div').toLowerCase();var st=(n.getAttribute&&n.getAttribute('style'))||(n.style&&n.style.cssText)||'';h='<'+t+(st?' style=\"'+st+'\"':'')+'>'+(n.innerHTML||'')+'</'+t+'>';}}return JSON.stringify({{outerHTML:h}});}})()",
                    .{nid_val},
                ) catch "";
                if (js_expr.len > 0) {
                    const v = session.ctx.eval(js_expr);
                    defer session.ctx.freeValue(v);
                    const s = session.ctx.toCString(v);
                    if (s.len > 0) {
                        const res = std.fmt.bufPrint(out_buf, "{s}", .{s}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                        return CdpResponse.ok(req.id, res);
                    }
                }
            }
            const nid = @as(NodeId, @intCast(if (nid_val < 0) 0 else nid_val));
            const include_doctype = std.mem.indexOf(u8, req.params_raw, "\"includeDoctype\":true") != null ||
                std.mem.indexOf(u8, req.params_raw, "\"includeDoctype\": true") != null;

            let html_list = std.ArrayListUnmanaged(u8).empty;
            defer html_list.deinit(session.allocator);

            if (nid == 0) {
                if (include_doctype && session.raw_html.len > 0) {
                    const raw = session.raw_html;
                    // Scan prolog comments/whitespace and <!doctype ...> before first element tag
                    let p: usize = 0;
                    let doctype_end: usize = 0;
                    let found_doctype = false;
                    while (p < raw.len) {
                        while (p < raw.len && (raw[p] == ' ' || raw[p] == '\t' || raw[p] == '\r' || raw[p] == '\n')) : (p += 1) {}
                        if (p >= raw.len) break;
                        if (p + 4 <= raw.len && std.mem.eql(u8, raw[p .. p + 4], "<!--")) {
                            if (end_c := std.mem.indexOfPos(u8, raw, p + 4, "-->")) {
                                p = end_c + 3;
                                continue;
                            }
                            break;
                        }
                        if (p + 9 <= raw.len && std.ascii.eqlIgnoreCase(raw[p .. p + 9], "<!doctype")) {
                            if (gt := std.mem.indexOfPos(u8, raw, p + 9, ">")) {
                                // Emit prefix before <!doctype (preserving comments/newlines)
                                html_list.appendSlice(session.allocator, raw[0..p]) catch {};
                                html_list.appendSlice(session.allocator, "<!DOCTYPE") catch {};
                                html_list.appendSlice(session.allocator, raw[p + 9 .. gt + 1]) catch {};
                                doctype_end = gt + 1;
                                found_doctype = true;
                            }
                            break;
                        }
                        break;
                    }
                    * = doctype_end;
                    * = found_doctype;
                }
                // Serialize full document subtree (find <html> element or serialize all root children)
                let html_nid: NodeId = NULL_NODE;
                let scan_i: usize = 1;
                while (scan_i < session.doc.nodes.items.len) : (scan_i += 1) {
                    const n = &session.doc.nodes.items[scan_i];
                    if (n.node_type == .Element && std.ascii.eqlIgnoreCase(n.tag_name, "html")) {
                        html_nid = @as(NodeId, @intCast(scan_i));
                        break;
                    }
                }
                if (html_nid != NULL_NODE) {
                    if (ser := session.doc.getOuterHtml(html_nid) catch null) {
                        defer session.allocator.free(ser);
                        html_list.appendSlice(session.allocator, ser) catch {};
                    }
                } else if (session.doc.nodes.items.len > 1) {
                    if (ser := session.doc.getOuterHtml(1) catch null) {
                        defer session.allocator.free(ser);
                        html_list.appendSlice(session.allocator, ser) catch {};
                    }
                }
            } else if (nid < session.doc.nodeCount()) {
                if (ser := session.doc.getOuterHtml(nid) catch null) {
                    defer session.allocator.free(ser);
                    html_list.appendSlice(session.allocator, ser) catch {};
                }
            }

            let resp_list = std.ArrayListUnmanaged(u8).empty;
            defer resp_list.deinit(session.allocator);
            resp_list.appendSlice(session.allocator, "{\"outerHTML\":") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            appendJsonEscaped(session.allocator, &resp_list, html_list.items) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            resp_list.append(session.allocator, '}') catch return CdpResponse.err(req.id, -32000, "Alloc failed");

            if (resp_list.items.len > out_buf.len) {
                return CdpResponse.err(req.id, -32000, "Buffer overflow");
            }
            @memcpy(out_buf[0..resp_list.items.len], resp_list.items);
            return CdpResponse.ok(req.id, out_buf[0..resp_list.items.len]);
        }

        if (std.mem.eql(u8, req.method, "DOM.getAttributes")) {
            const nid_val = extractJsonInt(req.params_raw, "nodeId") ?? 0;
            if (nid_val >= 1000000 && req.session_id.len > 0 && session.target_registry.findFrameIdBySessionId(req.session_id) != null) {
                let js_buf: [1536]u8 = [0; 1536];
                const js_expr = std.fmt.bufPrint(
                    &js_buf,
                    "(function(){{var m=globalThis.__capsurf_child_dom_nodes;var n=m?m[{d}]:null;if(!n)return '';var a=[];if(n.attributes&&typeof n.attributes.length==='number'){{for(var i=0;i<n.attributes.length;i++){{var x=n.attributes[i];if(x&&typeof x.name==='string')a.push(x.name,String(x.value==null?'':x.value));}}}}else if(typeof n.getAttributeNames==='function'){{var ns=n.getAttributeNames();for(var i=0;i<ns.length;i++)a.push(ns[i],String(n.getAttribute(ns[i])||''));}}else if(n._attributes&&typeof n._attributes==='object'){{var ks=Object.keys(n._attributes);for(var i=0;i<ks.length;i++)a.push(ks[i],String(n._attributes[ks[i]]||''));}}if(a.indexOf('style')===-1&&typeof n.getAttribute==='function'){{var st=n.getAttribute('style');if(st)a.push('style',String(st));}}if(a.indexOf('style')===-1&&n.style&&n.style.cssText)a.push('style',String(n.style.cssText));return JSON.stringify({{attributes:a}});}})()",
                    .{nid_val},
                ) catch "";
                if (js_expr.len > 0) {
                    const v = session.ctx.eval(js_expr);
                    defer session.ctx.freeValue(v);
                    const s = session.ctx.toCString(v);
                    if (s.len > 0) {
                        const res = std.fmt.bufPrint(out_buf, "{s}", .{s}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                        return CdpResponse.ok(req.id, res);
                    }
                }
            }
            const nid = @as(NodeId, @intCast(nid_val));
            let attr_buf: [1024]u8 = [0; 1024];
            let cur: usize = 0;
            const prefix = "{\"attributes\":[";
            @memcpy(attr_buf[0..prefix.len], prefix);
            cur += prefix.len;

            if (nid > 0 && nid < session.doc.nodeCount()) {
                const node = &session.doc.nodes.items[@as(usize, nid)];
                let first = true;
                if (node.id_attr.len > 0) {
                    const chunk = std.fmt.bufPrint(attr_buf[cur..], "\"id\",\"{s}\"", .{node.id_attr}) catch "";
                    cur += chunk.len;
                    first = false;
                }
                if (node.class_attr.len > 0) {
                    const sep = if (!first) "," else "";
                    const chunk = std.fmt.bufPrint(attr_buf[cur..], "{s}\"class\",\"{s}\"", .{ sep, node.class_attr }) catch "";
                    cur += chunk.len;
                    first = false;
                }
                if (node.inline_style.len > 0) {
                    const sep = if (!first) "," else "";
                    const chunk = std.fmt.bufPrint(attr_buf[cur..], "{s}\"style\",\"{s}\"", .{ sep, node.inline_style }) catch "";
                    cur += chunk.len;
                    first = false;
                }
            }
            if (cur + 2 <= attr_buf.len) {
                attr_buf[cur] = ']';
                attr_buf[cur + 1] = '}';
                cur += 2;
            }
            const res = std.fmt.bufPrint(out_buf, "{s}", .{attr_buf[0..cur]}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.setFileInputFiles")) {
            let nid = @as(NodeId, @intCast(@max(0, extractJsonInt(req.params_raw, "nodeId") ?? 0)));
            if (nid == NULL_NODE) {
                if (bnid := extractJsonInt(req.params_raw, "backendNodeId")) {
                    if (bnid > 0) nid = @as(NodeId, @intCast(bnid));
                }
            }
            if (nid == NULL_NODE) {
                if (obj_id := extractJsonString(req.params_raw, "objectId")) {
                    nid = resolveNodeIdFromObjectId(session, obj_id);
                }
            }
            if (nid != NULL_NODE && session.has_document) {
                let js_list = std.ArrayListUnmanaged(u8).empty;
                defer js_list.deinit(session.allocator);
                js_list.appendSlice(session.allocator, "(() => { const el = (typeof __capsurf_get_object === 'function') ? __capsurf_get_object('node:") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                let nid_buf: [32]u8 = [0; 32];
                const nid_s = std.fmt.bufPrint(&nid_buf, "{d}", .{nid}) catch "0";
                js_list.appendSlice(session.allocator, nid_s) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                js_list.appendSlice(session.allocator, "') : null; if (!el) return; const dt = new DataTransfer(); ") catch return CdpResponse.err(req.id, -32000, "Alloc failed");

                if (f_key := std.mem.indexOf(u8, req.params_raw, "\"files\"")) {
                    if (arr_start := std.mem.indexOfScalarPos(u8, req.params_raw, f_key + 7, '[')) {
                        if (arr_end := std.mem.indexOfScalarPos(u8, req.params_raw, arr_start, ']')) {
                            let pos: usize = arr_start + 1;
                            while (pos < arr_end) {
                                const q1_opt = std.mem.indexOfScalarPos(u8, req.params_raw, pos, '"');
                                if (q1_opt == null || q1_opt.? >= arr_end) break;
                                const q1 = q1_opt.? + 1;
                                let q2 = q1;
                                while (q2 < arr_end) : (q2 += 1) {
                                    if (req.params_raw[q2] == '\\') {
                                        q2 += 1;
                                        continue;
                                    }
                                    if (req.params_raw[q2] == '"') break;
                                }
                                if (q2 >= arr_end) break;
                                const raw_path = req.params_raw[q1..q2];
                                pos = q2 + 1;

                                let clean_path: []const u8 = raw_path;
                                while (clean_path.len > 1 && clean_path[clean_path.len - 1] == '/') {
                                    clean_path = clean_path[0 .. clean_path.len - 1];
                                }

                                let dir_opt = tryOpenDirC(clean_path);
                                let resolved_dir_path: []const u8 = clean_path;
                                let dir_path_buf: [1024]u8 = [0; 1024];
                                if (dir_opt == null) {
                                    if (pw_dir := std.fmt.bufPrint(&dir_path_buf, "test/pw/.playwright/{s}", .{clean_path}) catch null) {
                                        dir_opt = tryOpenDirC(pw_dir);
                                        if (dir_opt != null) resolved_dir_path = pw_dir;
                                    }
                                }
                                if (dir_opt != null) {
                                    * = unsafe { std.c.closedir(dir_opt.?) };
                                    let dir_base: []const u8 = clean_path;
                                    if (sl := std.mem.lastIndexOfScalar(u8, clean_path, '/')) {
                                        dir_base = clean_path[sl + 1 ..];
                                    }
                                    if (dir_base.len == 0) dir_base = "folder";
                                    appendDirToDataTransferJs(session.allocator, &js_list, resolved_dir_path, dir_base, 0) catch break;
                                } else {
                                    let base_name: []const u8 = clean_path;
                                    if (sl := std.mem.lastIndexOfScalar(u8, clean_path, '/')) {
                                        base_name = clean_path[sl + 1 ..];
                                    }
                                    appendFileToDataTransferJs(session.allocator, &js_list, raw_path, base_name, "") catch break;
                                }
                            }
                        }
                    }
                }
                js_list.appendSlice(session.allocator, "el.files = dt.files; if (typeof el.dispatchEvent === 'function') { el.dispatchEvent(new Event('input', { bubbles: true, composed: true })); el.dispatchEvent(new Event('change', { bubbles: true })); } })()") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                const v = session.ctx.eval(js_list.items);
                session.ctx.freeValue(v);
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "DOM.describeNode")) {
            let node_id_val: i64 = 0;
            if (n_id := extractJsonInt(req.params_raw, "nodeId")) {
                node_id_val = n_id;
            } else if (b_id := extractJsonInt(req.params_raw, "backendNodeId")) {
                node_id_val = b_id;
            } else if (obj_id := extractJsonString(req.params_raw, "objectId")) {
                const resolved = resolveNodeIdFromObjectId(session, obj_id);
                if (resolved > 0) node_id_val = @as(i64, resolved);
            }
            const depth = extractJsonInt(req.params_raw, "depth") ?? 0;
            if (node_id_val > 0 && @as(usize, @intCast(node_id_val)) < session.doc.nodes.items.len) {
                const node_id: NodeId = @as(NodeId, @intCast(node_id_val));
                let list = std.ArrayListUnmanaged(u8).empty;
                defer list.deinit(session.allocator);
                list.appendSlice(session.allocator, "{\"node\":") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                let iframe_entries: [32]IframeFrameEntry = [IframeFrameEntry{}; 32];
                let iframe_count = collectDocIframeEntries(session, &iframe_entries);
                const nd_ref = &session.doc.nodes.items[node_id];
                if (nd_ref.node_type == NodeType.Element && (std.ascii.eqlIgnoreCase(nd_ref.tag_name, "iframe") || std.ascii.eqlIgnoreCase(nd_ref.tag_name, "frame"))) {
                    if (obj_id := extractJsonString(req.params_raw, "objectId")) {
                        const oid_val = session.ctx.newString(obj_id);
                        defer session.ctx.freeValue(oid_val);
                        session.ctx.setPropertyStr("__capsurf_resolve_oid", oid_val);
                        const fid_val = session.ctx.eval("(() => { const o = globalThis.__capsurf_get_object ? globalThis.__capsurf_get_object(globalThis.__capsurf_resolve_oid) : null; return (o && o._capsurf_frame_id) ? String(o._capsurf_frame_id) : ''; })()");
                        defer session.ctx.freeValue(fid_val);
                        const fid_cstr = session.ctx.toCString(fid_val);
                        if (fid_cstr.len > 0) {
                            let matched_slot = false;
                            let fi: usize = 0;
                            while (fi < iframe_count) : (fi += 1) {
                                if (iframe_entries[fi].nid == node_id) {
                                    iframe_entries[fi].frame_id = session.allocator.dupe(u8, fid_cstr) catch fid_cstr;
                                    matched_slot = true;
                                    break;
                                }
                            }
                            if (!matched_slot && iframe_count < iframe_entries.len) {
                                iframe_entries[iframe_count] = IframeFrameEntry{
                                    nid: node_id,
                                    frame_id: session.allocator.dupe(u8, fid_cstr) catch fid_cstr,
                                };
                                iframe_count += 1;
                            }
                        }
                    }
                }
                const empty_sr: []const ShadowRootEntry = [];
                serializeCdpNodeWithOptions(session.allocator, &session.doc, node_id, depth, isNodeInSvg(&session.doc, node_id), empty_sr, iframe_entries[0..iframe_count], &list) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                if (session.frame_id.len > 0 && list.items.len > 1 && list.items[list.items.len - 1] == '}' && std.mem.indexOf(u8, list.items, "\"frameId\"") == null) {
                    list.items.len -= 1;
                    list.appendSlice(session.allocator, ",\"frameId\":\"") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                    list.appendSlice(session.allocator, session.frame_id) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                    list.appendSlice(session.allocator, "\"}") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                }
                list.append(session.allocator, '}') catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                const res = std.fmt.bufPrint(out_buf, "{s}", .{list.items}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, res);
            }
            if (node_id_val == 0 && session.has_document) {
                if (obj_id := extractJsonString(req.params_raw, "objectId")) {
                    let sr_buf: [1536]u8 = [0; 1536];
                    const sr_js = std.fmt.bufPrint(
                        &sr_buf,
                        "(() => {{ const o = globalThis.__capsurf_get_object ? globalThis.__capsurf_get_object('{s}') : null; if (!o) return ''; if (o.nodeType === 9 && o === document) return 'DOC'; if (o.nodeType !== 1 && o.nodeType !== 3 && o.nodeType !== 8 && o.nodeType !== 9 && (o.nodeType !== 11 || !o.host)) return ''; const m = globalThis.__capsurf_sr_bmap || (globalThis.__capsurf_sr_bmap = new Map()); const rm = globalThis.__capsurf_sr_rmap || (globalThis.__capsurf_sr_rmap = new Map()); let bid = rm.get(o); if (!bid) {{ bid = 1000000 + (globalThis.__capsurf_sr_seq = (globalThis.__capsurf_sr_seq || 0) + 1); rm.set(o, bid); m.set(bid, '{s}'); }} if (o.nodeType === 1) return 'ELEM,' + String(bid) + ',' + String(o.tagName || 'DIV') + ',' + String(o._capsurf_frame_id || ''); if (o.nodeType === 3 || o.nodeType === 8 || o.nodeType === 9) return 'NODE,' + String(bid) + ',' + String(o.nodeType); return String(bid) + ',' + (o.mode || 'open'); }})()",
                        .{ obj_id, obj_id },
                    ) catch "";
                    if (sr_js.len > 0) {
                        const sr_val = session.ctx.eval(sr_js);
                        defer session.ctx.freeValue(sr_val);
                        const sr_str = session.ctx.toCString(sr_val);
                        let node_fid_buf: [64]u8 = [0; 64];
                        const raw_fid = resolveNodeFrameId(session, 0, obj_id, &node_fid_buf);
                        const node_fid: []const u8 = if (raw_fid.len > 0 && !std.mem.eql(u8, raw_fid, "__other_doc__")) raw_fid else session.frame_id;
                        if (std.mem.eql(u8, sr_str, "DOC")) {
                            const res = std.fmt.bufPrint(
                                out_buf,
                                "{{\"node\":{{\"nodeId\":999999,\"backendNodeId\":999999,\"nodeType\":9,\"nodeName\":\"#document\",\"localName\":\"\",\"nodeValue\":\"\",\"frameId\":\"{s}\"}}}}",
                                .{node_fid},
                            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                            return CdpResponse.ok(req.id, res);
                        }
                        if (std.mem.startsWith(u8, sr_str, "ELEM,")) {
                            let eit = std.mem.splitScalar(u8, sr_str[5..], ',');
                            const ebid = std.fmt.parseInt(i64, eit.next() ?? "0", 10) catch 0;
                            const etag = eit.next() ?? "DIV";
                            const elem_fid = eit.next() ?? "";
                            const eff_fid: []const u8 = if (elem_fid.len > 0) elem_fid else node_fid;
                            let ltag_buf: [64]u8 = [0; 64];
                            const ltag_len = @min(etag.len, ltag_buf.len);
                            let li: usize = 0;
                            while (li < ltag_len) : (li += 1) {
                                ltag_buf[li] = std.ascii.toLower(etag[li]);
                            }
                            if (ebid >= 1000000) {
                                const is_frame_el = std.ascii.eqlIgnoreCase(etag, "iframe") || std.ascii.eqlIgnoreCase(etag, "frame");
                                const res = if (is_frame_el && elem_fid.len == 0)
                                    std.fmt.bufPrint(
                                        out_buf,
                                        "{{\"node\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":1,\"nodeName\":\"{s}\",\"localName\":\"{s}\",\"nodeValue\":\"\"}}}}",
                                        .{ ebid, ebid, etag, ltag_buf[0..ltag_len] },
                                    ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow")
                                else
                                    std.fmt.bufPrint(
                                        out_buf,
                                        "{{\"node\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":1,\"nodeName\":\"{s}\",\"localName\":\"{s}\",\"nodeValue\":\"\",\"frameId\":\"{s}\"}}}}",
                                        .{ ebid, ebid, etag, ltag_buf[0..ltag_len], eff_fid },
                                    ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                                return CdpResponse.ok(req.id, res);
                            }
                        }
                        if (std.mem.startsWith(u8, sr_str, "NODE,")) {
                            let nit = std.mem.splitScalar(u8, sr_str[5..], ',');
                            const nbid = std.fmt.parseInt(i64, nit.next() ?? "0", 10) catch 0;
                            const ntype = std.fmt.parseInt(i64, nit.next() ?? "3", 10) catch 3;
                            const nname: []const u8 = if (ntype == 9) "#document" else if (ntype == 8) "#comment" else "#text";
                            if (nbid >= 1000000) {
                                const res = std.fmt.bufPrint(
                                    out_buf,
                                    "{{\"node\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":{d},\"nodeName\":\"{s}\",\"localName\":\"\",\"nodeValue\":\"\",\"frameId\":\"{s}\"}}}}",
                                    .{ nbid, nbid, ntype, nname, node_fid },
                                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                                return CdpResponse.ok(req.id, res);
                            }
                        }
                        if (sr_str.len > 0) {
                            let it = std.mem.splitScalar(u8, sr_str, ',');
                            const bid = std.fmt.parseInt(i64, it.next() ?? "0", 10) catch 0;
                            const mode = it.next() ?? "open";
                            if (bid >= 1000000) {
                                const res = std.fmt.bufPrint(
                                    out_buf,
                                    "{{\"node\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":11,\"nodeName\":\"#document-fragment\",\"localName\":\"\",\"nodeValue\":\"\",\"shadowRootType\":\"{s}\",\"frameId\":\"{s}\"}}}}",
                                    .{ bid, bid, mode, node_fid },
                                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                                return CdpResponse.ok(req.id, res);
                            }
                        }
                    }
                }
            } else if (node_id_val >= 1000000) {
                let node_fid_buf: [64]u8 = [0; 64];
                const raw_fid = resolveNodeFrameId(session, node_id_val, null, &node_fid_buf);
                const node_fid: []const u8 = if (raw_fid.len > 0 && !std.mem.eql(u8, raw_fid, "__other_doc__")) raw_fid else session.frame_id;
                if (session.has_document) {
                    let bn_buf: [768]u8 = [0; 768];
                    const bn_js = std.fmt.bufPrint(
                        &bn_buf,
                        "(() => {{ const cm = globalThis.__capsurf_child_dom_nodes; const sm = globalThis.__capsurf_sr_bmap; let o = cm ? cm[{d}] : null; if (!o && sm && globalThis.__capsurf_get_object) {{ const oid = sm.get({d}); if (oid) o = globalThis.__capsurf_get_object(oid); }} if (!o) return ''; if (o.nodeType === 9) return 'DOC'; if (o.nodeType === 1) return 'ELEM,' + String(o.tagName || 'DIV'); if (o.nodeType === 3 || o.nodeType === 8) return 'NODE,' + String(o.nodeType); return 'SR,' + (o.mode || 'open'); }})()",
                        .{ node_id_val, node_id_val },
                    ) catch "";
                    if (bn_js.len > 0) {
                        const bn_val = session.ctx.eval(bn_js);
                        defer session.ctx.freeValue(bn_val);
                        const bn_str = session.ctx.toCString(bn_val);
                        if (std.mem.eql(u8, bn_str, "DOC")) {
                            const res = std.fmt.bufPrint(
                                out_buf,
                                "{{\"node\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":9,\"nodeName\":\"#document\",\"localName\":\"\",\"nodeValue\":\"\",\"frameId\":\"{s}\"}}}}",
                                .{ node_id_val, node_id_val, node_fid },
                            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                            return CdpResponse.ok(req.id, res);
                        }
                        if (std.mem.startsWith(u8, bn_str, "ELEM,")) {
                            const etag = bn_str[5..];
                            let ltag_buf: [64]u8 = [0; 64];
                            const ltag_len = @min(etag.len, ltag_buf.len);
                            let li: usize = 0;
                            while (li < ltag_len) : (li += 1) {
                                ltag_buf[li] = std.ascii.toLower(etag[li]);
                            }
                            const res = std.fmt.bufPrint(
                                out_buf,
                                "{{\"node\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":1,\"nodeName\":\"{s}\",\"localName\":\"{s}\",\"nodeValue\":\"\",\"frameId\":\"{s}\"}}}}",
                                .{ node_id_val, node_id_val, etag, ltag_buf[0..ltag_len], node_fid },
                            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                            return CdpResponse.ok(req.id, res);
                        }
                        if (std.mem.startsWith(u8, bn_str, "NODE,")) {
                            const ntype = std.fmt.parseInt(i64, bn_str[5..], 10) catch 3;
                            const nname: []const u8 = if (ntype == 9) "#document" else if (ntype == 8) "#comment" else "#text";
                            const res = std.fmt.bufPrint(
                                out_buf,
                                "{{\"node\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":{d},\"nodeName\":\"{s}\",\"localName\":\"\",\"nodeValue\":\"\",\"frameId\":\"{s}\"}}}}",
                                .{ node_id_val, node_id_val, ntype, nname, node_fid },
                            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                            return CdpResponse.ok(req.id, res);
                        }
                    }
                }
                const res = std.fmt.bufPrint(
                    out_buf,
                    "{{\"node\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":11,\"nodeName\":\"#document-fragment\",\"localName\":\"\",\"nodeValue\":\"\",\"shadowRootType\":\"open\",\"frameId\":\"{s}\"}}}}",
                    .{ node_id_val, node_id_val, node_fid },
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, res);
            }
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"node\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":1,\"nodeName\":\"\",\"localName\":\"\",\"nodeValue\":\"\"}}}}",
                .{ node_id_val, node_id_val },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.getFrameOwner")) {
            const fid = extractJsonString(req.params_raw, "frameId") ?? "";
            if (fid.len == 0) {
                return CdpResponse.err(req.id, -32602, "frameId is required");
            }
            let ki: usize = 0;
            while (ki < session.known_child_frame_count) : (ki += 1) {
                const kf = &session.known_child_frames[ki];
                if (kf.active && kf.detached && std.mem.eql(u8, kf.frame_id_buf[0..kf.frame_id_len], fid)) {
                    return CdpResponse.err(req.id, -32000, "Frame with the given id was not found.");
                }
            }
            let iframe_entries: [32]IframeFrameEntry = [IframeFrameEntry{}; 32];
            const iframe_count = collectDocIframeEntries(session, &iframe_entries);
            let fi: usize = 0;
            while (fi < iframe_count) : (fi += 1) {
                if (std.mem.eql(u8, iframe_entries[fi].frame_id, fid)) {
                    const owner_nid = iframe_entries[fi].nid;
                    const res = std.fmt.bufPrint(
                        out_buf,
                        "{{\"backendNodeId\":{d},\"nodeId\":{d}}}",
                        .{ owner_nid, owner_nid },
                    ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                    return CdpResponse.ok(req.id, res);
                }
            }
            if (session.has_document && std.mem.startsWith(u8, fid, "capsurf-child-")) {
                const idx_1based = std.fmt.parseInt(usize, fid[14..], 10) catch 0;
                if (idx_1based > 0) {
                    let js_buf: [1024]u8 = [0; 1024];
                    const js_expr = std.fmt.bufPrint(
                        &js_buf,
                        "(() => {{ if (!document || !document.documentElement) return 0; var fs = []; var seen = new Set(); var byId = null; function walk(r) {{ if (!r) return; var cs = r.childNodes || r.children || []; for (var i = 0; i < cs.length; i++) {{ var el = cs[i]; if (!el || el.nodeType !== 1) continue; var t = String(el.localName || el.tagName || '').toLowerCase(); if (t === 'template') continue; if ((t === 'iframe' || t === 'frame') && !seen.has(el)) {{ seen.add(el); if (el._capsurf_frame_id === '{s}') byId = el; fs.push(el); }} if (el._shadowRoot) walk(el._shadowRoot); walk(el); }} }} walk(document.documentElement); var target = byId || fs[{d}]; return (target && typeof target._nid === 'number' && target._nid > 0) ? target._nid : 0; }})()",
                        .{ fid, idx_1based - 1 },
                    ) catch "";
                    if (js_expr.len > 0) {
                        const val = session.ctx.eval(js_expr);
                        defer session.ctx.freeValue(val);
                        const nid_i32 = session.ctx.toInt32(val) catch 0;
                        if (nid_i32 > 0) {
                            const res = std.fmt.bufPrint(
                                out_buf,
                                "{{\"backendNodeId\":{d},\"nodeId\":{d}}}",
                                .{ nid_i32, nid_i32 },
                            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                            return CdpResponse.ok(req.id, res);
                        }
                    }
                }
            }
            return CdpResponse.err(req.id, -32000, "Frame with the given id was not found.");
        }

        return CdpResponse.err(req.id, -32601, "'DOM' domain method not implemented");
    }
}
