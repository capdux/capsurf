// src/cdp/dom.cp
// CDP DOM Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/dom.ts

import { CdpRequest, CdpResponse, CdpBoxModel, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { NodeId, NULL_NODE } from "../dom/dom.cp";

pub struct DomHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8): CdpResponse {
        if (std.mem.eql(u8, req.method, "DOM.enable") ||
            std.mem.eql(u8, req.method, "DOM.disable") ||
            std.mem.eql(u8, req.method, "DOM.hideHighlight") ||
            std.mem.eql(u8, req.method, "DOM.highlightNode")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "DOM.getDocument")) {
            const root_node = session.doc.root_element;
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"root\":{{\"nodeId\":{d},\"backendNodeId\":{d},\"nodeType\":9,\"nodeName\":\"#document\",\"localName\":\"\",\"nodeValue\":\"\",\"childNodeCount\":{d}}}}}",
                .{ root_node, root_node, session.doc.nodeCount() },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
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
            const node_id_val = extractJsonInt(req.params_raw, "nodeId") ?? 0;
            if (node_id_val <= 0) {
                return CdpResponse.err(req.id, -32000, "Could not compute box model");
            }
            const node_id: NodeId = @as(NodeId, @intCast(node_id_val));
            const bm = session.getBoxModel(node_id);

            let quad_buf: [256]u8 = [0; 256];
            const q = std.fmt.bufPrint(
                &quad_buf,
                "[{d:.1},{d:.1},{d:.1},{d:.1},{d:.1},{d:.1},{d:.1},{d:.1}]",
                .{
                    bm.content[0], bm.content[1], bm.content[2], bm.content[3],
                    bm.content[4], bm.content[5], bm.content[6], bm.content[7],
                },
            ) catch "[]";

            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"model\":{{\"content\":{s},\"padding\":{s},\"border\":{s},\"margin\":{s},\"width\":{d:.1},\"height\":{d:.1}}}}}",
                .{ q, q, q, q, bm.width, bm.height },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "DOM.getContentQuads")) {
            const node_id_val = extractJsonInt(req.params_raw, "nodeId") ?? 0;
            if (node_id_val <= 0) {
                return CdpResponse.ok(req.id, "{\"quads\":[]}");
            }
            const node_id: NodeId = @as(NodeId, @intCast(node_id_val));
            const bm = session.getBoxModel(node_id);

            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"quads\":[[{d:.1},{d:.1},{d:.1},{d:.1},{d:.1},{d:.1},{d:.1},{d:.1}]]}}",
                .{
                    bm.content[0], bm.content[1], bm.content[2], bm.content[3],
                    bm.content[4], bm.content[5], bm.content[6], bm.content[7],
                },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
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
