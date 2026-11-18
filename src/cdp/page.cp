// src/cdp/page.cp
// CDP Page Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/page.ts and vendor/kitesurf/packages/engine/src/eval/outbound.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession, escapeJsonString } from "./session.cp";
import { HttpLoader } from "../net/http_loader.cp";
import { Base64 } from "cap:encoding";
import {
    FrameState,
    MainFrameArgs,
    makeMainFrameState,
    buildFrameTree,
    buildFrameResourceTree
} from "./frames.cp";
import { extractAckSessionId } from "./screencast.cp";

pub struct PageHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8, out_events: *[4]CdpEvent, event_count: *usize): CdpResponse {
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "Page.enable") ||
            std.mem.eql(u8, req.method, "Page.disable") ||
            std.mem.eql(u8, req.method, "Page.stopLoading") ||
            std.mem.eql(u8, req.method, "Page.reload")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.setDocumentContent")) {
            if (html := extractJsonString(req.params_raw, "html")) {
                session.navigate(html, session.url, "") catch return CdpResponse.err(req.id, -32000, "Failed to set document content");
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.startScreencast")) {
            * = session.screencast_handler.startScreencast(req.params_raw, session.target_id);
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.stopScreencast")) {
            * = session.screencast_handler.stopScreencast();
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.screencastFrameAck")) {
            const sid = extractAckSessionId(req.params_raw) ?? 0;
            * = session.screencast_handler.screencastFrameAck(sid);
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Page.captureScreenshot")) {
            let compress_lvl: u8 = 6;
            const fmt = extractJsonString(req.params_raw, "format") ?? "png";
            * = fmt;

            if (q := extractJsonInt(req.params_raw, "quality")) {
                if (q >= 0 && q <= 9) {
                    compress_lvl = @as(u8, @intCast(q));
                } else if (q < 50) {
                    compress_lvl = 1;
                } else {
                    compress_lvl = 6;
                }
            }

            if (std.mem.indexOf(u8, req.params_raw, "\"optimizeForSpeed\":true") != null) {
                compress_lvl = 1;
            }

            const png_bytes = session.captureScreenshot(compress_lvl) catch return CdpResponse.err(req.id, -32000, "Screenshot capture failed");
            defer session.allocator.free(png_bytes);

            const b64_str = Base64.encodeAlloc(session.allocator, png_bytes) catch return CdpResponse.err(req.id, -32000, "Base64 encoding failed");
            defer session.allocator.free(b64_str);

            const prefix = "{\"data\":\"";
            const suffix = "\"}";
            const total_len = prefix.len + b64_str.len + suffix.len;

            if (out_buf.len >= total_len) {
                @memcpy(out_buf[0..prefix.len], prefix);
                @memcpy(out_buf[prefix.len .. prefix.len + b64_str.len], b64_str);
                @memcpy(out_buf[prefix.len + b64_str.len .. total_len], suffix);
                return CdpResponse.ok(req.id, out_buf[0..total_len]);
            } else {
                let res_mem = session.allocator.alloc(u8, total_len) catch return CdpResponse.err(req.id, -32000, "Out of memory");
                @memcpy(res_mem[0..prefix.len], prefix);
                @memcpy(res_mem[prefix.len .. prefix.len + b64_str.len], b64_str);
                @memcpy(res_mem[prefix.len + b64_str.len .. total_len], suffix);
                return CdpResponse.ok(req.id, res_mem);
            }
        }

        if (std.mem.eql(u8, req.method, "Page.navigate")) {
            const url = extractJsonString(req.params_raw, "url") ?? "about:blank";
            const referrer = extractJsonString(req.params_raw, "referrer") ?? "";

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

            session.navigate(fetch_res.html, fetch_res.final_url, referrer) catch return CdpResponse.err(req.id, -32000, "HTML parsing failed");

            // Format dynamic Page.frameNavigated event
            const fn_json = session.formatFrameNavigatedEvent(fetch_res.final_url);

            out_events[0] = CdpEvent.create("Page.frameNavigated", fn_json);
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

        if (std.mem.eql(u8, req.method, "Page.getNavigationHistory")) {
            if (session.current_nav_index < session.nav_entry_count) {
                if (session.title.len > 0) {
                    session.updateCurrentNavTitle(session.title);
                }
            }

            let cur: usize = 0;
            const prefix = std.fmt.bufPrint(
                out_buf[cur..],
                "{{\"currentIndex\":{d},\"entries\":[",
                .{session.current_nav_index},
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            cur += prefix.len;

            let i: usize = 0;
            while (i < session.nav_entry_count) : (i += 1) {
                const entry = &session.nav_entries[i];
                let esc_url_buf: [1024]u8 = [0; 1024];
                const esc_url = escapeJsonString(entry.url, &esc_url_buf);
                let esc_title_buf: [512]u8 = [0; 512];
                const esc_title = escapeJsonString(entry.title, &esc_title_buf);

                const sep = if (i > 0) "," else "";
                const entry_str = std.fmt.bufPrint(
                    out_buf[cur..],
                    "{s}{{\"id\":{d},\"url\":\"{s}\",\"userTypedURL\":\"{s}\",\"title\":\"{s}\",\"transitionType\":\"link\"}}",
                    .{ sep, entry.id, esc_url, esc_url, esc_title },
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                cur += entry_str.len;
            }

            if (cur + 2 > out_buf.len) return CdpResponse.err(req.id, -32000, "Buffer overflow");
            out_buf[cur] = ']';
            out_buf[cur + 1] = '}';
            cur += 2;

            return CdpResponse.ok(req.id, out_buf[0..cur]);
        }

        if (std.mem.eql(u8, req.method, "Page.navigateToHistoryEntry")) {
            const entry_id = extractJsonInt(req.params_raw, "entryId") ?? -1;
            if (entry_id < 0) {
                return CdpResponse.err(req.id, -32602, "Invalid entryId");
            }
            if (!session.navigateToHistoryEntry(@as(i32, @intCast(entry_id)))) {
                return CdpResponse.err(req.id, -32000, "No entry with given id");
            }
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.err(req.id, -32601, "'Page' domain method not implemented");
    }
}
