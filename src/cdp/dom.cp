// src/cdp/dom.cp
// CDP DOM Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/dom.ts

import { CdpRequest, CdpResponse, CdpBoxModel, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { NodeId, NULL_NODE, Document } from "../dom/dom.cp";

fn appendJsonEscaped(allocator: std.mem.Allocator, list: *std.ArrayListUnmanaged(u8), str: []const u8) !void {
    try list.append(allocator, '"');
    let i: usize = 0;
    while (i < str.len) : (i += 1) {
        const c = str[i];
        if (c == '"') {
            try list.appendSlice(allocator, "\\\"");
        } else if (c == '\\') {
            try list.appendSlice(allocator, "\\\\");
        } else if (c == '\n') {
            try list.appendSlice(allocator, "\\n");
        } else if (c == '\r') {
            try list.appendSlice(allocator, "\\r");
        } else if (c == '\t') {
            try list.appendSlice(allocator, "\\t");
        } else {
            try list.append(allocator, c);
        }
    }
    try list.append(allocator, '"');
}

fn serializeCdpNode(allocator: std.mem.Allocator, doc: *const Document, nid: NodeId, depth: i64, list: *std.ArrayListUnmanaged(u8)) !void {
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
    try list.appendSlice(allocator, header);

    switch (node.node_type) {
        .Document => {
            try list.appendSlice(allocator, "\"nodeType\":9,\"nodeName\":\"#document\",\"localName\":\"\",\"nodeValue\":\"\",");
        },
        .Element => {
            const tag = node.tag_name;
            let upper_buf: [64]u8 = [0; 64];
            let upper_len: usize = 0;
            while (upper_len < tag.len && upper_len < upper_buf.len) : (upper_len += 1) {
                upper_buf[upper_len] = std.ascii.toUpper(tag[upper_len]);
            }
            const upper_tag = upper_buf[0..upper_len];
            let el_buf: [256]u8 = undefined;
            const el_header = std.fmt.bufPrint(&el_buf, "\"nodeType\":1,\"nodeName\":\"{s}\",\"localName\":\"{s}\",\"nodeValue\":\"\",", .{ upper_tag, tag }) catch return;
            try list.appendSlice(allocator, el_header);

            try list.appendSlice(allocator, "\"attributes\":[");
            let attr_first = true;
            if (node.id_attr.len > 0) {
                let id_b: [128]u8 = undefined;
                const id_s = std.fmt.bufPrint(&id_b, "\"id\",\"{s}\"", .{node.id_attr}) catch "";
                try list.appendSlice(allocator, id_s);
                attr_first = false;
            }
            if (node.class_attr.len > 0) {
                if (!attr_first) try list.append(allocator, ',');
                let cls_b: [128]u8 = undefined;
                const cls_s = std.fmt.bufPrint(&cls_b, "\"class\",\"{s}\"", .{node.class_attr}) catch "";
                try list.appendSlice(allocator, cls_s);
                attr_first = false;
            }
            if (node.inline_style.len > 0) {
                if (!attr_first) try list.append(allocator, ',');
                let sty_b: [512]u8 = undefined;
                const sty_s = std.fmt.bufPrint(&sty_b, "\"style\",\"{s}\"", .{node.inline_style}) catch "";
                try list.appendSlice(allocator, sty_s);
                attr_first = false;
            }
            try list.appendSlice(allocator, "],");
        },
        .Text => {
            try list.appendSlice(allocator, "\"nodeType\":3,\"nodeName\":\"#text\",\"localName\":\"\",\"nodeValue\":");
            try appendJsonEscaped(allocator, list, node.text_content);
            try list.append(allocator, ',');
        },
        .Comment => {
            try list.appendSlice(allocator, "\"nodeType\":8,\"nodeName\":\"#comment\",\"localName\":\"\",\"nodeValue\":");
            try appendJsonEscaped(allocator, list, node.text_content);
            try list.append(allocator, ',');
        },
    }

    let cnt_buf: [64]u8 = undefined;
    const cnt_s = std.fmt.bufPrint(&cnt_buf, "\"childNodeCount\":{d}", .{child_count}) catch return;
    try list.appendSlice(allocator, cnt_s);

    if (depth != 0 && child_count > 0) {
        try list.appendSlice(allocator, ",\"children\":[");
        let first_kid = true;
        let kid = node.first_child;
        const next_depth = if (depth > 0) depth - 1 else -1;
        while (kid != NULL_NODE && @as(usize, kid) < doc.nodes.items.len) {
            if (!first_kid) try list.append(allocator, ',');
            first_kid = false;
            try serializeCdpNode(allocator, doc, kid, next_depth, list);
            kid = doc.nodes.items[@as(usize, kid)].next_sibling;
        }
        try list.append(allocator, ']');
    }

    try list.append(allocator, '}');
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
        if (std.mem.eql(u8, req.method, "DOM.enable") ||
            std.mem.eql(u8, req.method, "DOM.disable") ||
            std.mem.eql(u8, req.method, "DOM.hideHighlight") ||
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
                serializeCdpNode(session.allocator, &session.doc, root_node, depth, &list) catch return CdpResponse.err(req.id, -32000, "Alloc failed");
                list.appendSlice(session.allocator, "]") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
            }
            list.appendSlice(session.allocator, "}}") catch return CdpResponse.err(req.id, -32000, "Alloc failed");
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
            const node_id_val = extractJsonInt(req.params_raw, "nodeId") ?? 0;
            const node_id: NodeId = @as(NodeId, @intCast(node_id_val));
            const tag = if (node_id > 0 && node_id < session.doc.nodeCount()) session.doc.getTagName(node_id) else "";
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"node\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":1,\"nodeName\":\"{s}\",\"localName\":\"{s}\",\"nodeValue\":\"\"}}}}",
                .{ node_id, node_id, tag, tag },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        return CdpResponse.err(req.id, -32601, "'DOM' domain method not implemented");
    }
}
