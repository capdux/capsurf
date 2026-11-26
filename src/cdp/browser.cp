// src/cdp/browser.cp
// CDP Browser Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/browser.ts

import { CdpRequest, CdpResponse, extractJsonString, extractJsonBool } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { DownloadBehavior, DownloadConfigStore } from "./download.cp";

pub let global_download_store: DownloadConfigStore = DownloadConfigStore{};

pub fn getDownloadConfigStore(): *DownloadConfigStore {
    return &global_download_store;
}

fn parseDownloadBehavior(raw: []const u8): DownloadBehavior? {
    if (std.mem.eql(u8, raw, "deny") || std.mem.eql(u8, raw, "default")) {
        return .deny;
    }
    if (std.mem.eql(u8, raw, "allow")) {
        return .allow;
    }
    if (std.mem.eql(u8, raw, "allowAndName")) {
        return .allow_and_name;
    }
    if (std.mem.eql(u8, raw, "stream")) {
        return .stream;
    }
    return null;
}

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

        if (std.mem.eql(u8, req.method, "Browser.setDownloadBehavior")) {
            const raw_beh = extractJsonString(req.params_raw, "behavior") ?? "deny";
            const beh = parseDownloadBehavior(raw_beh) orelse {
                return CdpResponse.err(req.id, -32602, "Invalid download behavior");
            };
            const ctx_id = extractJsonString(req.params_raw, "browserContextId") ?? "";
            const dl_path = extractJsonString(req.params_raw, "downloadPath") ?? "";
            const events_enabled = extractJsonBool(req.params_raw, "eventsEnabled") ?? false;
            global_download_store.upsert(ctx_id, beh, dl_path, events_enabled, req.session_id);
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Browser.close")) {
            return CdpResponse.ok(req.id, "{}");
        }

        // Permissive fallback for probes (setPermission, getWindowBounds, etc.)
        return CdpResponse.ok(req.id, "{}");
    }
}
