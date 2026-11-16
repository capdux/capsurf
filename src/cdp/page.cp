// src/cdp/page.cp
// CDP Page Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/page.ts and vendor/kitesurf/packages/engine/src/eval/outbound.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { HttpLoader } from "../net/http_loader.cp";
import {
    FrameState,
    MainFrameArgs,
    makeMainFrameState,
    buildFrameTree,
    buildFrameResourceTree
} from "./frames.cp";

pub struct PageHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8, out_events: *[4]CdpEvent, event_count: *usize): CdpResponse {
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "Page.enable") ||
            std.mem.eql(u8, req.method, "Page.disable") ||
            std.mem.eql(u8, req.method, "Page.stopLoading") ||
            std.mem.eql(u8, req.method, "Page.reload")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.navigate")) {
            const url = extractJsonString(req.params_raw, "url") ?? "about:blank";

            // Allocate buffer for document fetching
            let fetch_buf = session.allocator.alloc(u8, 65536) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(fetch_buf);

            const fetch_res = HttpLoader.fetchDocument(session, url, fetch_buf, 0) catch |e| {
                if (e == error.RequestAborted) {
                    return CdpResponse.err(req.id, -32000, "net::ERR_ABORTED");
                }
                if (e == error.BlockedByClient) {
                    return CdpResponse.err(req.id, -32000, "net::ERR_BLOCKED_BY_CLIENT");
                }
                return CdpResponse.err(req.id, -32000, "Navigation failed");
            };

            session.navigate(fetch_res.html, fetch_res.final_url) catch return CdpResponse.err(req.id, -32000, "HTML parsing failed");

            // Format dynamic Page.frameNavigated event
            let fn_buf: [256]u8 = undefined;
            const fn_json = std.fmt.bufPrint(
                fn_buf[0..],
                "{{\"frame\":{{\"id\":\"{s}\",\"loaderId\":\"{s}\",\"url\":\"{s}\",\"securityOrigin\":\"{s}\",\"mimeType\":\"text/html\"}}}}",
                .{ session.frame_id, session.loader_id, fetch_res.final_url, fetch_res.final_url },
            ) catch "{}";

            out_events[0] = CdpEvent.create("Page.frameNavigated", session.allocator.dupe(u8, fn_json) catch fn_json);
            out_events[1] = CdpEvent.create("Page.domContentEventFired", "{\"timestamp\":1700000000.0}");
            out_events[2] = CdpEvent.create("Page.loadEventFired", "{\"timestamp\":1700000000.1}");
            event_count.* = 3;

            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"frameId\":\"{s}\",\"loaderId\":\"{s}\"}}",
                .{ session.frame_id, session.loader_id },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Page.getLayoutMetrics")) {
            const w = session.viewport_width;
            const h = session.viewport_height;
            const dsf = session.device_scale_factor;
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"layoutViewport\":{{\"pageX\":0,\"pageY\":0,\"clientWidth\":{d},\"clientHeight\":{d}}},\"visualViewport\":{{\"pageX\":0,\"pageY\":0,\"clientWidth\":{d},\"clientHeight\":{d},\"scale\":{d},\"zoom\":1}},\"contentSize\":{{\"x\":0,\"y\":0,\"width\":{d},\"height\":{d}}}}}",
                .{ w, h, w, h, dsf, w, h },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Page.getFrameTree")) {
            let main_f = makeMainFrameState(MainFrameArgs{
                frame_id: session.frame_id,
                loader_id: session.loader_id,
                url: session.url,
            });
            let f_arr: [1]FrameState = [main_f];
            let tree_buf: [4096]u8 = undefined;
            const tree_json = buildFrameTree(f_arr[0..1], tree_buf[0..]);
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"frameTree\":{s}}}",
                .{ tree_json },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        if (std.mem.eql(u8, req.method, "Page.getFrameResourceTree")) {
            let main_f = makeMainFrameState(MainFrameArgs{
                frame_id: session.frame_id,
                loader_id: session.loader_id,
                url: session.url,
            });
            let f_arr: [1]FrameState = [main_f];
            let tree_buf: [4096]u8 = undefined;
            const tree_json = buildFrameResourceTree(f_arr[0..1], tree_buf[0..]);
            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"frameTree\":{s}}}",
                .{ tree_json },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        return CdpResponse.err(req.id, -32601, "'Page' domain method not implemented");
    }
}
