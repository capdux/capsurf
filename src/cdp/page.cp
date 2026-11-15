// src/cdp/page.cp
// CDP Page Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/page.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString } from "./types.cp";
import { CdpPageSession } from "./session.cp";

pub struct PageHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8, out_events: *[2]CdpEvent, event_count: *usize): CdpResponse {
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "Page.enable") ||
            std.mem.eql(u8, req.method, "Page.disable") ||
            std.mem.eql(u8, req.method, "Page.stopLoading") ||
            std.mem.eql(u8, req.method, "Page.reload")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.navigate")) {
            let url = extractJsonString(req.params_raw, "url") ?? "about:blank";
            let html_content: []const u8 = "<html><body></body></html>";

            const data_prefix = "data:text/html,";
            if (std.mem.startsWith(u8, url, data_prefix)) {
                html_content = url[data_prefix.len..];
            } else if (std.mem.startsWith(u8, url, "http://") || std.mem.startsWith(u8, url, "https://")) {
                // When navigating to external URL, can load via Doc
                html_content = "<html><body><p>Navigated</p></body></html>";
            }

            session.navigate(html_content, url) catch return CdpResponse.err(req.id, -32000, "Navigation failed");

            // Prepare emitted events
            out_events[0] = CdpEvent.create(
                "Page.frameNavigated",
                "{\"frame\":{\"id\":\"capsurf-frame-001\",\"loaderId\":\"capsurf-loader-001\",\"url\":\"about:blank\",\"securityOrigin\":\"\",\"mimeType\":\"text/html\"}}",
            );
            out_events[1] = CdpEvent.create(
                "Page.loadEventFired",
                "{\"timestamp\":1700000000.0}",
            );
            event_count.* = 2;

            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\"}}",
                .{ session.frame_id, session.loader_id },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Page.getLayoutMetrics")) {
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"layoutViewport\":{{\"pageX\":0,\"pageY\":0,\"clientWidth\":1280,\"clientHeight\":800}},\"visualViewport\":{{\"pageX\":0,\"pageY\":0,\"clientWidth\":1280,\"clientHeight\":800,\"scale\":1,\"zoom\":1}},\"contentSize\":{{\"x\":0,\"y\":0,\"width\":1280,\"height\":800}}}}",
                .{},
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        return CdpResponse.err(req.id, -32601, "'Page' domain method not implemented");
    }
}
