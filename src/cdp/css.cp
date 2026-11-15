// src/cdp/css.cp
// CDP CSS Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/css.ts

import { CdpRequest, CdpResponse, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { NodeId, NULL_NODE } from "../dom/dom.cp";

pub struct CssHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8): CdpResponse {
        if (std.mem.eql(u8, req.method, "CSS.enable") ||
            std.mem.eql(u8, req.method, "CSS.disable") ||
            std.mem.eql(u8, req.method, "CSS.reset")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "CSS.getComputedStyleForNode")) {
            const node_id_val = extractJsonInt(req.params_raw, "nodeId") ?? 0;
            if (node_id_val <= 0) {
                return CdpResponse.ok(req.id, "{\"computedStyle\":[]}");
            }
            const node_id: NodeId = @as(NodeId, @intCast(node_id_val));
            const rect = session.layout.getBoundingClientRect(node_id);

            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"computedStyle\":[" ++
                "{{\"name\":\"width\",\"value\":\"{d:.1}px\"}}," ++
                "{{\"name\":\"height\",\"value\":\"{d:.1}px\"}}," ++
                "{{\"name\":\"display\",\"value\":\"{s}\"}}," ++
                "{{\"name\":\"box-sizing\",\"value\":\"border-box\"}}," ++
                "{{\"name\":\"visibility\",\"value\":\"{s}\"}}" ++
                "]}}",
                .{
                    rect.width,
                    rect.height,
                    if (rect.visible) "block" else "none",
                    if (rect.visible) "visible" else "hidden",
                },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "CSS.getInlineStylesForNode")) {
            return CdpResponse.ok(req.id, "{\"inlineStyle\":{\"cssProperties\":[],\"shorthandEntries\":[]}}");
        }

        if (std.mem.eql(u8, req.method, "CSS.getMatchedStylesForNode")) {
            return CdpResponse.ok(req.id, "{\"matchedCSSRules\":[],\"pseudoElements\":[]}");
        }

        return CdpResponse.ok(req.id, "{}");
    }
}
