// src/cdp/target.cp
// CDP Target Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/target.ts

import { CdpRequest, CdpResponse, CdpEvent, TargetState } from "./types.cp";
import { CdpPageSession } from "./session.cp";

pub struct TargetHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8, out_events: *[1]CdpEvent, event_count: *usize): CdpResponse {
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "Target.setDiscoverTargets") ||
            std.mem.eql(u8, req.method, "Target.setAutoAttach")) {
            out_events[0] = CdpEvent.create(
                "Target.targetCreated",
                "{\"targetInfo\":{\"targetId\":\"capsurf-target-001\",\"type\":\"page\",\"title\":\"CapSurf Agent Blank Page\",\"url\":\"about:blank\",\"attached\":true}}",
            );
            event_count.* = 1;
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Target.getTargets")) {
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"targetInfos\":[{{\"targetId\":\"capsurf-target-001\",\"type\":\"page\",\"title\":\"{s}\",\"url\":\"{s}\",\"attached\":true,\"browserContextId\":\"capsurf-context-001\"}}]}}",
                .{ session.title, session.url },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Target.attachToTarget")) {
            return CdpResponse.ok(req.id, "{\"sessionId\":\"capsurf-session-001\"}");
        }

        if (std.mem.eql(u8, req.method, "Target.detachFromTarget")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Target.createTarget")) {
            return CdpResponse.ok(req.id, "{\"targetId\":\"capsurf-target-001\"}");
        }

        if (std.mem.eql(u8, req.method, "Target.closeTarget")) {
            return CdpResponse.ok(req.id, "{\"success\":true}");
        }

        return CdpResponse.err(req.id, -32601, "'Target' domain method not implemented");
    }
}
