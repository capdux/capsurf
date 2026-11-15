// src/cdp/runtime.cp
// CDP Runtime Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/runtime.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString } from "./types.cp";
import { CdpPageSession } from "./session.cp";

pub struct RuntimeHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8, out_events: *[1]CdpEvent, event_count: *usize): CdpResponse {
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "Runtime.enable")) {
            out_events[0] = CdpEvent.create(
                "Runtime.executionContextCreated",
                "{\"context\":{\"id\":1,\"origin\":\"about:blank\",\"name\":\"\",\"uniqueId\":\"ctx-1\",\"auxData\":{\"isDefault\":true,\"type\":\"default\",\"frameId\":\"capsurf-frame-001\"}}}",
            );
            event_count.* = 1;
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Runtime.disable") ||
            std.mem.eql(u8, req.method, "Runtime.releaseObject") ||
            std.mem.eql(u8, req.method, "Runtime.discardConsoleEntries")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Runtime.evaluate")) {
            const expr = extractJsonString(req.params_raw, "expression") ?? "undefined";
            let eval_res_buf: [2048]u8 = [0; 2048];
            const eval_len = session.evaluate(expr, &eval_res_buf);
            const eval_slice = eval_res_buf[0..eval_len];

            const res = std.fmt.bufPrint(
                out_buf,
                "{s}",
                .{eval_slice},
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        return CdpResponse.err(req.id, -32601, "'Runtime' domain method not implemented");
    }
}
