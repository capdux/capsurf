// src/cdp/dom.cp
// CDP DOM Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/dom.ts

import { CdpRequest, CdpResponse, CdpBoxModel, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { NodeId, NULL_NODE, Document } from "../dom/dom.cp";

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
        } else {
            list.append(allocator, c)?;
        }
    }
    list.append(allocator, '"')?;
}

pub struct ShadowRootEntry {
    nid: NodeId = NULL_NODE,
    json: []const u8 = "",
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

pub fn serializeCdpNode(allocator: std.mem.Allocator, doc: *const Document, nid: NodeId, depth: i64, list: *std.ArrayListUnmanaged(u8))! {
    const empty_sr: []const ShadowRootEntry = [];
    const in_svg = isNodeInSvg(doc, nid);
    return serializeCdpNodeWithOptions(allocator, doc, nid, depth, in_svg, empty_sr, list);
}

pub fn serializeCdpNodeWithOptions(
    allocator: std.mem.Allocator,
    doc: *const Document,
    nid: NodeId,
    depth: i64,
    in_svg: bool,
    shadow_roots: []const ShadowRootEntry,
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
            serializeCdpNodeWithOptions(allocator, doc, kid, next_depth, child_in_svg, shadow_roots, list)?;
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

pub struct DomHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8): CdpResponse {
        if (std.mem.eql(u8, req.method, "DOM.enable")) {
            session.dom_enabled = true;
            if (!session.dom_document_sent) {
                session.markDocumentObserved(-1);
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "DOM.disable")) {
            session.setDomDisabled();
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "DOM.hideHighlight") ||
            std.mem.eql(u8, req.method, "DOM.highlightNode") ||
            std.mem.eql(u8, req.method, "DOM.focus") ||
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
            const root_node = session.doc.root_element;
            const depth = extractJsonInt(req.params_raw, "depth") ?? -1;
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
                    "var attrs=[];var rawAttrs=n.attributes||[];" ++
                    "for(var i=0;i<rawAttrs.length;i++){var a=rawAttrs[i];if(a&&typeof a.name==='string'){attrs.push(a.name,String(a.value==null?'':a.value));}}" ++
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

            const cnt: u32 = if (root_node != NULL_NODE) 1 else 0;
            let root_buf: [256]u8 = undefined;
            const root_s = std.fmt.bufPrint(
                &root_buf,
                "{{\"root\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":9,\"nodeName\":\"#document\",\"localName\":\"\",\"nodeValue\":\"\",\"childNodeCount\":{d}",
                .{ root_node, root_node, cnt },
            ) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            list.appendSlice(session.allocator, root_s) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            if (depth != 0 && root_node != NULL_NODE) {
                list.appendSlice(session.allocator, ",\"children\":[") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                serializeCdpNodeWithOptions(session.allocator, &session.doc, root_node, depth, false, sr_entries[0..sr_count], &list) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
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
                return CdpResponse.err(req.id, -32000, "Could not compute box model");
            }
            const node_id: NodeId = @as(NodeId, @intCast(node_id_val));
            const node_type = session.doc.nodes.items[@as(usize, node_id)].node_type;
            if (node_type == .Comment) {
                return CdpResponse.err(req.id, -32000, "Could not compute box model");
            }
            const bm = session.getBoxModel(node_id);
            if (bm.width <= 0.0 && bm.height <= 0.0) {
                return CdpResponse.err(req.id, -32000, "Could not compute box model");
            }

            const w_int = @as(i64, @intFromFloat(@round(bm.width)));
            const h_int = @as(i64, @intFromFloat(@round(bm.height)));
            const c0 = @as(i64, @intFromFloat(@round(bm.content[0])));
            const c1 = @as(i64, @intFromFloat(@round(bm.content[1])));
            const c2 = @as(i64, @intFromFloat(@round(bm.content[2])));
            const c3 = @as(i64, @intFromFloat(@round(bm.content[3])));
            const c4 = @as(i64, @intFromFloat(@round(bm.content[4])));
            const c5 = @as(i64, @intFromFloat(@round(bm.content[5])));
            const c6 = @as(i64, @intFromFloat(@round(bm.content[6])));
            const c7 = @as(i64, @intFromFloat(@round(bm.content[7])));

            let quad_buf: [256]u8 = [0; 256];
            const q = std.fmt.bufPrint(
                &quad_buf,
                "[{d},{d},{d},{d},{d},{d},{d},{d}]",
                .{ c0, c1, c2, c3, c4, c5, c6, c7 },
            ) catch "[]";

            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"model\":{{\"content\":{s},\"padding\":{s},\"border\":{s},\"margin\":{s},\"width\":{d},\"height\":{d}}}}}",
                .{ q, q, q, q, w_int, h_int },
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
                return CdpResponse.ok(req.id, "{\"quads\":[]}");
            }
            const node_id: NodeId = @as(NodeId, @intCast(node_id_val));
            const node_type = session.doc.nodes.items[@as(usize, node_id)].node_type;
            if (node_type == .Comment) {
                return CdpResponse.ok(req.id, "{\"quads\":[]}");
            }
            const bm = session.getBoxModel(node_id);
            if (bm.width <= 0.0 && bm.height <= 0.0) {
                return CdpResponse.ok(req.id, "{\"quads\":[]}");
            }

            const c0 = @as(i64, @intFromFloat(@round(bm.content[0])));
            const c1 = @as(i64, @intFromFloat(@round(bm.content[1])));
            const c2 = @as(i64, @intFromFloat(@round(bm.content[2])));
            const c3 = @as(i64, @intFromFloat(@round(bm.content[3])));
            const c4 = @as(i64, @intFromFloat(@round(bm.content[4])));
            const c5 = @as(i64, @intFromFloat(@round(bm.content[5])));
            const c6 = @as(i64, @intFromFloat(@round(bm.content[6])));
            const c7 = @as(i64, @intFromFloat(@round(bm.content[7])));

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
                return CdpResponse.err(req.id, -32000, "Node not found");
            }
            const node_id: NodeId = @as(NodeId, @intCast(node_id_val));

            // Execute scroll into view in JS environment
            if (session.has_document) {
                let js_buf: [1024]u8 = [0; 1024];
                const js_expr = std.fmt.bufPrint(
                    &js_buf,
                    "(() => {{ let el = null; if ('{s}'.length > 0 && globalThis.__capsurf_get_object) {{ el = globalThis.__capsurf_get_object('{s}'); }} if (!el && {d} > 0) {{ function f(r, nid) {{ if (!r) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; }} return null; }} el = f(document, {d}); }} if (el) {{ if (typeof el.scrollIntoViewIfNeeded === 'function') {{ el.scrollIntoViewIfNeeded(true); }} else if (typeof el.scrollIntoView === 'function') {{ el.scrollIntoView({{ block: 'center', inline: 'center' }}); }} }} }})()",
                    .{ obj_id_val, obj_id_val, node_id, node_id },
                ) catch null;
                if (e := js_expr) {
                    const val = session.ctx.eval(e);
                    session.ctx.freeValue(val);
                }
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
            const nid_val = extractJsonInt(req.params_raw, "nodeId") ?? 0;
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"object\":{{\"type\":\"object\",\"subtype\":\"node\",\"className\":\"Element\",\"description\":\"Node#{d}\",\"objectId\":\"node:{d}\"}}}}",
                .{ nid_val, nid_val },
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
            const nid = @as(NodeId, @intCast(nid_val));
            let str_buf: [2048]u8 = [0; 2048];
            let out_str: []const u8 = "";
            if (nid > 0 && nid < session.doc.nodeCount()) {
                const tag = session.doc.getTagName(nid);
                out_str = std.fmt.bufPrint(&str_buf, "<{s}></{s}>", .{ tag, tag }) catch "";
            }
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"outerHTML\":\"{s}\"}}",
                .{out_str},
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.getAttributes")) {
            const nid_val = extractJsonInt(req.params_raw, "nodeId") ?? 0;
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
                serializeCdpNode(session.allocator, &session.doc, node_id, depth, &list) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                list.append(session.allocator, '}') catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                const res = std.fmt.bufPrint(out_buf, "{s}", .{list.items}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, res);
            }
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"node\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":1,\"nodeName\":\"\",\"localName\":\"\",\"nodeValue\":\"\"}}}}",
                .{ node_id_val, node_id_val },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        return CdpResponse.err(req.id, -32601, "'DOM' domain method not implemented");
    }
}
