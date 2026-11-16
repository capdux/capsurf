// src/cdp/browser.cp
// CDP Browser Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/browser.ts

import { CdpRequest, CdpResponse } from "./types.cp";
import { CdpPageSession } from "./session.cp";

pub struct BrowserHandler {
    static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8): CdpResponse {
        * = session;

        if (std.mem.eql(u8, req.method, "Browser.getVersion")) {
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"protocolVersion\":\"1.3\",\"product\":\"CapSurf/1.0.0\",\"revision\":\"cp-native-2026\",\"userAgent\":\"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/145.0.0.0 Safari/537.36 CapSurf/1.0.0 QuickJS-NG\",\"jsVersion\":\"QuickJS-NG\"}}",
                .{},
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Browser.close")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.err(req.id, -32601, "'Browser' domain method not implemented");
    }
}
