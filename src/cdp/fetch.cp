// src/cdp/fetch.cp
// CDP Fetch Interception Domain for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/fetch.ts and fetch-interception.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession } from "./session.cp";

pub enum InterceptVerdict {
    None,
    Continue,
    Fulfill,
    Fail,
}

pub struct InterceptPattern {
    url_pattern: []const u8 = "*";
    resource_type: []const u8 = "";
}

pub struct ParkedRequest {
    request_id: []const u8 = "";
    url: []const u8 = "";
    method: []const u8 = "GET";
    verdict: InterceptVerdict = InterceptVerdict.None;
    mock_status: u32 = 200;
    mock_body: []const u8 = "";
    mock_headers: []const u8 = "";
    error_reason: []const u8 = "";
}

pub struct FetchState {
    enabled: bool = false;
    patterns: [8]InterceptPattern = [InterceptPattern{}; 8];
    pattern_count: usize = 0;
    parked: [16]ParkedRequest = [ParkedRequest{}; 16];
    parked_count: usize = 0;
    next_id: u32 = 1;

    fn clearParked(*_) {
        _.parked_count = 0;
    }

    fn addPattern(*_, pat: []const u8, rtype: []const u8) {
        if (_.pattern_count < _.patterns.len) {
            _.patterns[_.pattern_count] = InterceptPattern{
                url_pattern: pat,
                resource_type: rtype,
            };
            _.pattern_count += 1;
        }
    }

    fn patternMatches(pat: []const u8, url: []const u8): bool {
        if (pat.len == 0 || std.mem.eql(u8, pat, "*")) return true;
        if (pat.len >= 2 && pat[0] == '*' && pat[pat.len - 1] == '*') {
            const core = pat[1 .. pat.len - 1];
            return std.mem.indexOf(u8, url, core) != null;
        }
        if (pat[0] == '*') {
            return std.mem.indexOf(u8, url, pat[1..]) != null;
        }
        if (pat[pat.len - 1] == '*') {
            return std.mem.startsWith(u8, url, pat[0 .. pat.len - 1]);
        }
        return std.mem.indexOf(u8, url, pat) != null;
    }

    pub fn shouldIntercept(url: []const u8, rtype: []const u8): bool {
        if (!_.enabled || _.pattern_count == 0) return false;
        let i: usize = 0;
        while (i < _.pattern_count) : (i += 1) {
            const p = _.patterns[i];
            if (FetchState.patternMatches(p.url_pattern, url)) {
                if (p.resource_type.len == 0 || std.mem.eql(u8, p.resource_type, rtype)) {
                    return true;
                }
            }
        }
        return false;
    }

    pub fn parkRequest(*_, req_id: []const u8, url: []const u8, method: []const u8): (*ParkedRequest)? {
        if (_.parked_count >= _.parked.len) {
            _.parked_count = 0; // Rotate ring buffer if full
        }
        const idx = _.parked_count;
        _.parked[idx] = ParkedRequest{
            request_id: req_id,
            url: url,
            method: method,
            verdict: InterceptVerdict.None,
            mock_status: 200,
            mock_body: "",
            mock_headers: "",
            error_reason: "",
        };
        _.parked_count += 1;
        return &_.parked[idx];
    }

    pub fn findParked(*_, req_id: []const u8): (*ParkedRequest)? {
        let i: usize = 0;
        while (i < _.parked_count) : (i += 1) {
            if (std.mem.eql(u8, _.parked[i].request_id, req_id)) {
                return &_.parked[i];
            }
        }
        return null;
    }
}

pub struct FetchHandler {
    pub static fn handle(
        session: *CdpPageSession,
        state: *FetchState,
        req: CdpRequest,
        out_buf: []u8,
    ): CdpResponse {
        * = session;
        * = out_buf;

        // 1. Fetch.enable: activate interception
        if (std.mem.eql(u8, req.method, "Fetch.enable")) {
            state.enabled = true;
            state.pattern_count = 0;
            session.fetch_enabled = true;

            // Extract urlPattern if provided, default to wildcard "*"
            const pat = extractJsonString(req.params_raw, "urlPattern") ?? "";
            const final_pat = if (pat.len > 0) pat else "*";
            const rtype = extractJsonString(req.params_raw, "resourceType") ?? "";
            state.addPattern(final_pat, rtype);

            return CdpResponse.ok(req.id, "{}");
        }

        // 2. Fetch.disable: deactivate interception and release all parked requests
        if (std.mem.eql(u8, req.method, "Fetch.disable")) {
            state.enabled = false;
            state.pattern_count = 0;
            state.clearParked();
            session.fetch_enabled = false;
            return CdpResponse.ok(req.id, "{}");
        }

        // 3. Fetch.continueRequest: resume request, optionally overriding url/method/headers/postData
        if (std.mem.eql(u8, req.method, "Fetch.continueRequest")) {
            const req_id = extractJsonString(req.params_raw, "requestId") ?? "";
            if (p := state.findParked(req_id)) {
                p.verdict = InterceptVerdict.Continue;
                const new_url = extractJsonString(req.params_raw, "url") ?? "";
                if (new_url.len > 0) p.url = new_url;
                const new_method = extractJsonString(req.params_raw, "method") ?? "";
                if (new_method.len > 0) p.method = new_method;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        // 4. Fetch.fulfillRequest: mock response with custom status code, headers and body
        if (std.mem.eql(u8, req.method, "Fetch.fulfillRequest")) {
            const req_id = extractJsonString(req.params_raw, "requestId") ?? "";
            if (p := state.findParked(req_id)) {
                p.verdict = InterceptVerdict.Fulfill;
                const code = extractJsonInt(req.params_raw, "responseCode") ?? 200;
                p.mock_status = if (code > 0) @intCast(code) else 200;
                p.mock_body = extractJsonString(req.params_raw, "body") ?? "";
            }
            return CdpResponse.ok(req.id, "{}");
        }

        // 5. Fetch.failRequest: abort the request with specified error reason
        if (std.mem.eql(u8, req.method, "Fetch.failRequest")) {
            const req_id = extractJsonString(req.params_raw, "requestId") ?? "";
            if (p := state.findParked(req_id)) {
                p.verdict = InterceptVerdict.Fail;
                const reason = extractJsonString(req.params_raw, "errorReason") ?? "";
                p.error_reason = if (reason.len > 0) reason else "Failed";
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Fetch.continueWithAuth")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.ok(req.id, "{}");
    }

    /// Helper to format a `Fetch.requestPaused` CDP event JSON payload
    pub static fn buildRequestPausedEvent(
        req_id: []const u8,
        url: []const u8,
        method: []const u8,
        rtype: []const u8,
        frame_id: []const u8,
        out_buf: []u8,
    ): CdpEvent? {
        const payload = std.fmt.bufPrint(
            out_buf,
            "{{\"requestId\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"{s}\",\"headers\":{{}},\"initialPriority\":\"High\",\"referrerPolicy\":\"no-referrer\"}},\"frameId\":\"{s}\",\"resourceType\":\"{s}\"}}",
            .{ req_id, url, method, frame_id, rtype },
        ) catch return null;

        return CdpEvent{
            method: "Fetch.requestPaused",
            params_json: payload,
        };
    }
}
