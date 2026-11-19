// src/cdp/log.cp
// CDP Log Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/log.ts

import { CdpRequest, CdpResponse, CdpEvent, MAX_CDP_EVENTS } from "./types.cp";
import { CdpPageSession } from "./session.cp";

pub struct LogHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8, out_events: *[MAX_CDP_EVENTS]CdpEvent, event_count: *usize): CdpResponse {
        * = out_buf;
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "Log.enable")) {
            session.log_enabled = true;
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Log.disable")) {
            session.log_enabled = false;
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Log.clear")) {
            session.clearConsoleLogs();
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Log.startViolationsReport") ||
            std.mem.eql(u8, req.method, "Log.stopViolationsReport")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.err(req.id, -32601, "'Log' domain method not implemented");
    }
}
