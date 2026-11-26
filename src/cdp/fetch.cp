// src/cdp/fetch.cp
// CDP Fetch Interception Domain for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/fetch.ts and fetch-interception.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, extractJsonArray } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { Base64 } from "cap:encoding";

pub fn httpStatusText(code: u16): []const u8 {
    return switch (code) {
        100 => "Continue",
        101 => "Switching Protocols",
        200 => "OK",
        201 => "Created",
        202 => "Accepted",
        204 => "No Content",
        206 => "Partial Content",
        301 => "Moved Permanently",
        302 => "Found",
        303 => "See Other",
        304 => "Not Modified",
        307 => "Temporary Redirect",
        308 => "Permanent Redirect",
        400 => "Bad Request",
        401 => "Unauthorized",
        403 => "Forbidden",
        404 => "Not Found",
        405 => "Method Not Allowed",
        408 => "Request Timeout",
        409 => "Conflict",
        410 => "Gone",
        413 => "Payload Too Large",
        415 => "Unsupported Media Type",
        418 => "I'm a teapot",
        422 => "Unprocessable Entity",
        429 => "Too Many Requests",
        500 => "Internal Server Error",
        501 => "Not Implemented",
        502 => "Bad Gateway",
        503 => "Service Unavailable",
        504 => "Gateway Timeout",
        else => "Unknown",
    };
}

pub enum InterceptVerdict {
    None,
    Continue,
    Fulfill,
    Fail,
}

pub struct InterceptPattern {
    url_pattern_buf: [512]u8 = [0; 512],
    url_pattern_len: usize = 0,
    resource_type_buf: [64]u8 = [0; 64],
    resource_type_len: usize = 0,
    request_stage_buf: [16]u8 = [0; 16],
    request_stage_len: usize = 0,

    fn urlPattern(): []const u8 {
        if (_.url_pattern_len == 0) return "*";
        return _.url_pattern_buf[0.._.url_pattern_len];
    }

    fn resourceType(): []const u8 {
        return _.resource_type_buf[0.._.resource_type_len];
    }

    fn requestStage(): []const u8 {
        if (_.request_stage_len == 0) return "Request";
        return _.request_stage_buf[0.._.request_stage_len];
    }

    fn set(*_, pat: []const u8, rtype: []const u8) {
        _.setWithStage(pat, rtype, "Request");
    }

    fn setWithStage(*_, pat: []const u8, rtype: []const u8, stage: []const u8) {
        const pn = @min(pat.len, _.url_pattern_buf.len);
        if (pn > 0) @memcpy(_.url_pattern_buf[0..pn], pat[0..pn]);
        _.url_pattern_len = pn;
        const rn = @min(rtype.len, _.resource_type_buf.len);
        if (rn > 0) @memcpy(_.resource_type_buf[0..rn], rtype[0..rn]);
        _.resource_type_len = rn;
        const final_stage = if (std.mem.eql(u8, stage, "Response")) "Response" else "Request";
        const sn = @min(final_stage.len, _.request_stage_buf.len);
        if (sn > 0) @memcpy(_.request_stage_buf[0..sn], final_stage[0..sn]);
        _.request_stage_len = sn;
    }
}

pub struct ParkedRequest {
    request_id_buf: [64]u8 = [0; 64],
    request_id_len: usize = 0,
    orig_url_buf: [2048]u8 = [0; 2048],
    orig_url_len: usize = 0,
    url_buf: [2048]u8 = [0; 2048],
    url_len: usize = 0,
    method_buf: [16]u8 = [0; 16],
    method_len: usize = 0,
    verdict: InterceptVerdict = InterceptVerdict.None,
    mock_status: u32 = 200,
    mock_status_text_buf: [64]u8 = [0; 64],
    mock_status_text_len: usize = 0,
    mock_body_buf: [8192]u8 = [0; 8192],
    mock_body_len: usize = 0,
    mock_body_dyn: ([]u8)? = null,
    has_mock_body: bool = false,
    mock_headers_buf: [1024]u8 = [0; 1024],
    mock_headers_len: usize = 0,
    mock_headers_dyn: ([]u8)? = null,
    error_reason_buf: [64]u8 = [0; 64],
    error_reason_len: usize = 0,
    resource_type_buf: [32]u8 = [0; 32],
    resource_type_len: usize = 0,
    network_id_buf: [64]u8 = [0; 64],
    network_id_len: usize = 0,

    fn requestId(): []const u8 {
        return _.request_id_buf[0.._.request_id_len];
    }

    fn origUrl(): []const u8 {
        return _.orig_url_buf[0.._.orig_url_len];
    }

    fn url(): []const u8 {
        return _.url_buf[0.._.url_len];
    }

    fn method(): []const u8 {
        return _.method_buf[0.._.method_len];
    }

    fn mockBody(): []const u8 {
        if (dyn_buf := _.mock_body_dyn) {
            return dyn_buf[0.._.mock_body_len];
        }
        return _.mock_body_buf[0.._.mock_body_len];
    }

    fn mockHeaders(): []const u8 {
        if (dyn_buf := _.mock_headers_dyn) {
            return dyn_buf[0.._.mock_headers_len];
        }
        return _.mock_headers_buf[0.._.mock_headers_len];
    }

    fn errorReason(): []const u8 {
        return _.error_reason_buf[0.._.error_reason_len];
    }

    fn resourceType(): []const u8 {
        return _.resource_type_buf[0.._.resource_type_len];
    }

    fn networkId(): []const u8 {
        return _.network_id_buf[0.._.network_id_len];
    }

    fn mockStatusText(): []const u8 {
        if (_.mock_status_text_len > 0) {
            return _.mock_status_text_buf[0.._.mock_status_text_len];
        }
        return httpStatusText(@as(u16, @intCast(@min(_.mock_status, 65535))));
    }

    fn setMockStatusText(*_, s: []const u8) {
        const n = @min(s.len, _.mock_status_text_buf.len);
        if (n > 0) @memcpy(_.mock_status_text_buf[0..n], s[0..n]);
        _.mock_status_text_len = n;
    }

    fn setRequestId(*_, s: []const u8) {
        const n = @min(s.len, _.request_id_buf.len);
        if (n > 0) @memcpy(_.request_id_buf[0..n], s[0..n]);
        _.request_id_len = n;
    }

    fn setOrigUrl(*_, s: []const u8) {
        const n = @min(s.len, _.orig_url_buf.len);
        if (n > 0) @memcpy(_.orig_url_buf[0..n], s[0..n]);
        _.orig_url_len = n;
    }

    fn setUrl(*_, s: []const u8) {
        const n = @min(s.len, _.url_buf.len);
        if (n > 0) @memcpy(_.url_buf[0..n], s[0..n]);
        _.url_len = n;
    }

    fn setMethod(*_, s: []const u8) {
        const n = @min(s.len, _.method_buf.len);
        if (n > 0) @memcpy(_.method_buf[0..n], s[0..n]);
        _.method_len = n;
    }

    fn setMockBody(*_, s: []const u8) {
        if (dyn_buf := _.mock_body_dyn) {
            std.heap.c_allocator.free(dyn_buf);
            _.mock_body_dyn = null;
        }
        if (s.len <= _.mock_body_buf.len) {
            if (s.len > 0) @memcpy(_.mock_body_buf[0..s.len], s);
            _.mock_body_len = s.len;
        } else if (buf := (std.heap.c_allocator.alloc(u8, s.len) catch null)) {
            @memcpy(buf, s);
            _.mock_body_dyn = buf;
            _.mock_body_len = s.len;
        } else {
            const n = _.mock_body_buf.len;
            @memcpy(_.mock_body_buf[0..n], s[0..n]);
            _.mock_body_len = n;
        }
    }

    fn setMockHeaders(*_, s: []const u8) {
        if (dyn_buf := _.mock_headers_dyn) {
            std.heap.c_allocator.free(dyn_buf);
            _.mock_headers_dyn = null;
        }
        if (s.len <= _.mock_headers_buf.len) {
            if (s.len > 0) @memcpy(_.mock_headers_buf[0..s.len], s);
            _.mock_headers_len = s.len;
        } else if (buf := (std.heap.c_allocator.alloc(u8, s.len) catch null)) {
            @memcpy(buf, s);
            _.mock_headers_dyn = buf;
            _.mock_headers_len = s.len;
        } else {
            const n = _.mock_headers_buf.len;
            @memcpy(_.mock_headers_buf[0..n], s[0..n]);
            _.mock_headers_len = n;
        }
    }

    fn setErrorReason(*_, s: []const u8) {
        const n = @min(s.len, _.error_reason_buf.len);
        if (n > 0) @memcpy(_.error_reason_buf[0..n], s[0..n]);
        _.error_reason_len = n;
    }

    fn setResourceType(*_, s: []const u8) {
        const n = @min(s.len, _.resource_type_buf.len);
        if (n > 0) @memcpy(_.resource_type_buf[0..n], s[0..n]);
        _.resource_type_len = n;
    }

    fn setNetworkId(*_, s: []const u8) {
        const n = @min(s.len, _.network_id_buf.len);
        if (n > 0) @memcpy(_.network_id_buf[0..n], s[0..n]);
        _.network_id_len = n;
    }

    fn reset(*_) {
        if (dyn_b := _.mock_body_dyn) {
            std.heap.c_allocator.free(dyn_b);
            _.mock_body_dyn = null;
        }
        if (dyn_h := _.mock_headers_dyn) {
            std.heap.c_allocator.free(dyn_h);
            _.mock_headers_dyn = null;
        }
        _.verdict = InterceptVerdict.None;
        _.orig_url_len = 0;
        _.url_len = 0;
        _.request_id_len = 0;
        _.mock_body_len = 0;
        _.has_mock_body = false;
        _.mock_headers_len = 0;
        _.mock_status = 200;
        _.mock_status_text_len = 0;
        _.error_reason_len = 0;
        _.resource_type_len = 0;
        _.network_id_len = 0;
    }
}

pub struct FetchState {
    enabled: bool = false;
    patterns: [32]InterceptPattern = [InterceptPattern{}; 32],
    pattern_count: usize = 0;
    parked: [16]ParkedRequest = [ParkedRequest{}; 16];
    parked_count: usize = 0;
    next_id: u32 = 1;

    fn clearParked(*_) {
        let i: usize = 0;
        while (i < _.parked_count) : (i += 1) {
            _.parked[i].reset();
        }
        _.parked_count = 0;
    }

    fn addPattern(*_, pat: []const u8, rtype: []const u8) {
        _.addPatternWithStage(pat, rtype, "Request");
    }

    fn addPatternWithStage(*_, pat: []const u8, rtype: []const u8, stage: []const u8) {
        if (_.pattern_count < _.patterns.len) {
            _.patterns[_.pattern_count] = InterceptPattern{};
            _.patterns[_.pattern_count].setWithStage(pat, rtype, stage);
            _.pattern_count += 1;
        }
    }

    static fn patternMatches(pat: []const u8, url: []const u8): bool {
        if (pat.len == 0) return true;
        let pi: usize = 0;
        let ui: usize = 0;
        let star_pi: usize? = null;
        let star_ui: usize = 0;

        while (ui < url.len) {
            if (pi < pat.len && pat[pi] == '*') {
                while (pi < pat.len && pat[pi] == '*') : (pi += 1) {}
                star_pi = pi;
                star_ui = ui;
            } else if (pi < pat.len && pat[pi] == '?') {
                pi += 1;
                ui += 1;
            } else if (pi < pat.len && pat[pi] == '\\' && pi + 1 < pat.len) {
                if (pat[pi + 1] == url[ui]) {
                    pi += 2;
                    ui += 1;
                } else if (sp := star_pi) {
                    pi = sp;
                    star_ui += 1;
                    ui = star_ui;
                } else {
                    return false;
                }
            } else if (pi < pat.len && pat[pi] == url[ui]) {
                pi += 1;
                ui += 1;
            } else if (sp := star_pi) {
                pi = sp;
                star_ui += 1;
                ui = star_ui;
            } else {
                return false;
            }
        }

        while (pi < pat.len && pat[pi] == '*') : (pi += 1) {}
        return pi == pat.len;
    }

    pub fn shouldIntercept(url: []const u8, rtype: []const u8): bool {
        return _.shouldInterceptStage(url, rtype, "Request");
    }

    pub fn shouldInterceptStage(url: []const u8, rtype: []const u8, stage: []const u8): bool {
        if (std.ascii.startsWithIgnoreCase(url, "data:") ||
            std.ascii.startsWithIgnoreCase(url, "blob:") ||
            std.ascii.startsWithIgnoreCase(url, "about:") ||
            std.ascii.startsWithIgnoreCase(url, "javascript:")) {
            return false;
        }
        if (!_.enabled || _.pattern_count == 0) return false;
        let i: usize = 0;
        while (i < _.pattern_count) : (i += 1) {
            const p = _.patterns[i];
            if (FetchState.patternMatches(p.urlPattern(), url)) {
                if (p.resourceType().len == 0 || std.mem.eql(u8, p.resourceType(), rtype)) {
                    if (stage.len == 0 || std.mem.eql(u8, p.requestStage(), stage)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    pub fn parkRequest(*_, req_id: []const u8, url: []const u8, method: []const u8): (*ParkedRequest)? {
        if (existing := _.findParked(req_id)) {
            existing.setOrigUrl(url);
            existing.setUrl(url);
            existing.setMethod(method);
            return existing;
        }
        if (_.parked_count >= _.parked.len) {
            _.parked_count = 0; // Rotate ring buffer if full
        }
        const idx = _.parked_count;
        _.parked[idx].reset();
        _.parked[idx] = ParkedRequest{};
        _.parked[idx].setRequestId(req_id);
        _.parked[idx].setOrigUrl(url);
        _.parked[idx].setUrl(url);
        _.parked[idx].setMethod(method);
        _.parked_count += 1;
        return &_.parked[idx];
    }

    pub fn findParked(*_, req_id: []const u8): (*ParkedRequest)? {
        let i: usize = 0;
        while (i < _.parked_count) : (i += 1) {
            if (std.mem.eql(u8, _.parked[i].requestId(), req_id)) {
                return &_.parked[i];
            }
        }
        return null;
    }

    pub fn findParkedByUrl(*_, url: []const u8): (*ParkedRequest)? {
        let i: usize = 0;
        while (i < _.parked_count) : (i += 1) {
            if (_.parked[i].origUrl().len > 0 && std.mem.eql(u8, _.parked[i].origUrl(), url)) {
                return &_.parked[i];
            }
            if (_.parked[i].url().len > 0 && std.mem.eql(u8, _.parked[i].url(), url)) {
                return &_.parked[i];
            }
        }
        return null;
    }
}

pub struct FetchHandler {
    static fn parseHeaderArray(hdrs_arr: []const u8, hdr_buf: []u8): usize {
        let hdr_pos: usize = 0;
        let idx: usize = 0;
        let depth: i32 = 0;
        let in_str = false;
        let escaped = false;
        let obj_start: usize = 0;
        while (idx < hdrs_arr.len) : (idx += 1) {
            const ch = hdrs_arr[idx];
            if (escaped) {
                escaped = false;
                continue;
            }
            if (ch == '\\' && in_str) {
                escaped = true;
                continue;
            }
            if (ch == '"') {
                in_str = !in_str;
                continue;
            }
            if (!in_str) {
                if (ch == '{') {
                    if (depth == 0) obj_start = idx;
                    depth += 1;
                } else if (ch == '}') {
                    depth -= 1;
                    if (depth == 0) {
                        const obj = hdrs_arr[obj_start .. idx + 1];
                        const h_name = extractJsonString(obj, "name") ?? "";
                        const h_val = extractJsonString(obj, "value") ?? "";
                        if (h_name.len > 0 && hdr_pos + h_name.len + 2 + h_val.len + 2 <= hdr_buf.len) {
                            @memcpy(hdr_buf[hdr_pos .. hdr_pos + h_name.len], h_name);
                            hdr_pos += h_name.len;
                            hdr_buf[hdr_pos] = ':';
                            hdr_buf[hdr_pos + 1] = ' ';
                            hdr_pos += 2;
                            if (h_val.len > 0) {
                                @memcpy(hdr_buf[hdr_pos .. hdr_pos + h_val.len], h_val);
                                hdr_pos += h_val.len;
                            }
                            hdr_buf[hdr_pos] = '\r';
                            hdr_buf[hdr_pos + 1] = '\n';
                            hdr_pos += 2;
                        }
                    }
                }
            }
        }
        return hdr_pos;
    }

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
            session.fetch_enabled = true;
            state.pattern_count = 0;

            if (pats_arr := extractJsonArray(req.params_raw, "patterns")) {
                let idx: usize = 0;
                let depth: i32 = 0;
                let in_str = false;
                let escaped = false;
                let obj_start: usize = 0;
                while (idx < pats_arr.len) : (idx += 1) {
                    const ch = pats_arr[idx];
                    if (escaped) {
                        escaped = false;
                        continue;
                    }
                    if (ch == '\\' && in_str) {
                        escaped = true;
                        continue;
                    }
                    if (ch == '"') {
                        in_str = !in_str;
                        continue;
                    }
                    if (!in_str) {
                        if (ch == '{') {
                            if (depth == 0) obj_start = idx;
                            depth += 1;
                        } else if (ch == '}') {
                            depth -= 1;
                            if (depth == 0) {
                                const obj = pats_arr[obj_start .. idx + 1];
                                const pat = extractJsonString(obj, "urlPattern") ?? "";
                                const final_pat = if (pat.len > 0) pat else "*";
                                const rtype = extractJsonString(obj, "resourceType") ?? "";
                                const stage_raw = extractJsonString(obj, "requestStage") ?? "";
                                const stage = if (std.mem.eql(u8, stage_raw, "Response")) "Response" else "Request";
                                state.addPatternWithStage(final_pat, rtype, stage);
                            }
                        }
                    }
                }
            } else {
                // Fallback: single-field urlPattern / resourceType / requestStage
                const pat = extractJsonString(req.params_raw, "urlPattern") ?? "";
                const final_pat = if (pat.len > 0) pat else "*";
                const rtype = extractJsonString(req.params_raw, "resourceType") ?? "";
                const stage_raw = extractJsonString(req.params_raw, "requestStage") ?? "";
                const stage = if (std.mem.eql(u8, stage_raw, "Response")) "Response" else "Request";
                state.addPatternWithStage(final_pat, rtype, stage);
            }

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
                if (new_url.len > 0) p.setUrl(new_url);
                const new_method = extractJsonString(req.params_raw, "method") ?? "";
                if (new_method.len > 0) p.setMethod(new_method);
                if (hdrs_arr := extractJsonArray(req.params_raw, "headers")) {
                    if (hdrs_arr.len <= 2048) {
                        let hdr_buf: [2048]u8 = undefined;
                        const hdr_pos = FetchHandler.parseHeaderArray(hdrs_arr, hdr_buf[0..]);
                        p.setMockHeaders(hdr_buf[0..hdr_pos]);
                    } else if (dyn_hdr := (std.heap.c_allocator.alloc(u8, hdrs_arr.len) catch null)) {
                        defer std.heap.c_allocator.free(dyn_hdr);
                        const hdr_pos = FetchHandler.parseHeaderArray(hdrs_arr, dyn_hdr);
                        p.setMockHeaders(dyn_hdr[0..hdr_pos]);
                    } else {
                        let hdr_buf: [2048]u8 = undefined;
                        const hdr_pos = FetchHandler.parseHeaderArray(hdrs_arr, hdr_buf[0..]);
                        p.setMockHeaders(hdr_buf[0..hdr_pos]);
                    }
                } else {
                    p.setMockHeaders("");
                }
                if (pd := extractJsonString(req.params_raw, "postData")) {
                    p.setMockBody(pd);
                    p.has_mock_body = true;
                } else {
                    p.setMockBody("");
                    p.has_mock_body = false;
                }
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
                if (phrase := extractJsonString(req.params_raw, "responsePhrase")) {
                    if (phrase.len > 0) {
                        p.setMockStatusText(phrase);
                    } else {
                        p.setMockStatusText(httpStatusText(@as(u16, @intCast(@min(p.mock_status, 65535)))));
                    }
                } else {
                    p.setMockStatusText(httpStatusText(@as(u16, @intCast(@min(p.mock_status, 65535)))));
                }
                if (hdrs_arr := extractJsonArray(req.params_raw, "responseHeaders")) {
                    if (hdrs_arr.len <= 2048) {
                        let hdr_buf: [2048]u8 = undefined;
                        const hdr_pos = FetchHandler.parseHeaderArray(hdrs_arr, hdr_buf[0..]);
                        p.setMockHeaders(hdr_buf[0..hdr_pos]);
                    } else if (dyn_hdr := (std.heap.c_allocator.alloc(u8, hdrs_arr.len) catch null)) {
                        defer std.heap.c_allocator.free(dyn_hdr);
                        const hdr_pos = FetchHandler.parseHeaderArray(hdrs_arr, dyn_hdr);
                        p.setMockHeaders(dyn_hdr[0..hdr_pos]);
                    } else {
                        let hdr_buf: [2048]u8 = undefined;
                        const hdr_pos = FetchHandler.parseHeaderArray(hdrs_arr, hdr_buf[0..]);
                        p.setMockHeaders(hdr_buf[0..hdr_pos]);
                    }
                } else {
                    p.setMockHeaders("");
                }
                if (b := extractJsonString(req.params_raw, "body")) {
                    p.setMockBody(b);
                } else {
                    p.setMockBody("");
                }
            }
            return CdpResponse.ok(req.id, "{}");
        }

        // 5. Fetch.failRequest: abort the request with specified error reason
        if (std.mem.eql(u8, req.method, "Fetch.failRequest")) {
            const req_id = extractJsonString(req.params_raw, "requestId") ?? "";
            if (p := state.findParked(req_id)) {
                p.verdict = InterceptVerdict.Fail;
                const reason = extractJsonString(req.params_raw, "errorReason") ?? "";
                p.setErrorReason(if (reason.len > 0) reason else "Failed");
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Fetch.continueWithAuth")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.ok(req.id, "{}");
    }

    static fn utf8ValidSeqLen(src: []const u8, i: usize): usize {
        const b0 = src[i];
        if (b0 >= 0xC2 && b0 <= 0xDF) {
            if (i + 1 < src.len) {
                const b1 = src[i + 1];
                if ((b1 & 0xC0) == 0x80) return 2;
            }
            return 0;
        }
        if (b0 >= 0xE0 && b0 <= 0xEF) {
            if (i + 2 < src.len) {
                const b1 = src[i + 1];
                const b2 = src[i + 2];
                if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
                    if (b0 == 0xE0 && b1 < 0xA0) return 0;
                    if (b0 == 0xED && b1 > 0x9F) return 0;
                    return 3;
                }
            }
            return 0;
        }
        if (b0 >= 0xF0 && b0 <= 0xF4) {
            if (i + 3 < src.len) {
                const b1 = src[i + 1];
                const b2 = src[i + 2];
                const b3 = src[i + 3];
                if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
                    if (b0 == 0xF0 && b1 < 0x90) return 0;
                    if (b0 == 0xF4 && b1 > 0x8F) return 0;
                    return 4;
                }
            }
            return 0;
        }
        return 0;
    }

    static fn jsonEscapedLen(src: []const u8): usize {
        let len: usize = 0;
        let i: usize = 0;
        while (i < src.len) {
            const c = src[i];
            if (c == '"' || c == '\\' || c == '\n' || c == '\r' || c == '\t') {
                len += 2;
                i += 1;
            } else if (c < 0x20) {
                len += 6;
                i += 1;
            } else if (c < 0x80) {
                len += 1;
                i += 1;
            } else {
                const seq_len = FetchHandler.utf8ValidSeqLen(src, i);
                if (seq_len > 0) {
                    len += seq_len;
                    i += seq_len;
                } else {
                    len += 6;
                    i += 1;
                }
            }
        }
        return len;
    }

    static fn escapeJsonStringExact(src: []const u8, dst: []u8): []const u8 {
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < src.len) {
            const c = src[i];
            if (c == '"') {
                if (out_idx + 2 > dst.len) break;
                dst[out_idx] = '\\';
                dst[out_idx + 1] = '"';
                out_idx += 2;
                i += 1;
            } else if (c == '\\') {
                if (out_idx + 2 > dst.len) break;
                dst[out_idx] = '\\';
                dst[out_idx + 1] = '\\';
                out_idx += 2;
                i += 1;
            } else if (c == '\n') {
                if (out_idx + 2 > dst.len) break;
                dst[out_idx] = '\\';
                dst[out_idx + 1] = 'n';
                out_idx += 2;
                i += 1;
            } else if (c == '\r') {
                if (out_idx + 2 > dst.len) break;
                dst[out_idx] = '\\';
                dst[out_idx + 1] = 'r';
                out_idx += 2;
                i += 1;
            } else if (c == '\t') {
                if (out_idx + 2 > dst.len) break;
                dst[out_idx] = '\\';
                dst[out_idx + 1] = 't';
                out_idx += 2;
                i += 1;
            } else if (c < 0x20) {
                if (out_idx + 6 > dst.len) break;
                const hex = "0123456789abcdef";
                dst[out_idx] = '\\';
                dst[out_idx + 1] = 'u';
                dst[out_idx + 2] = '0';
                dst[out_idx + 3] = '0';
                dst[out_idx + 4] = hex[(c >> 4) & 0x0f];
                dst[out_idx + 5] = hex[c & 0x0f];
                out_idx += 6;
                i += 1;
            } else if (c < 0x80) {
                if (out_idx + 1 > dst.len) break;
                dst[out_idx] = c;
                out_idx += 1;
                i += 1;
            } else {
                const seq_len = FetchHandler.utf8ValidSeqLen(src, i);
                if (seq_len > 0) {
                    if (out_idx + seq_len > dst.len) break;
                    let k: usize = 0;
                    while (k < seq_len) : (k += 1) {
                        dst[out_idx + k] = src[i + k];
                    }
                    out_idx += seq_len;
                    i += seq_len;
                } else {
                    if (out_idx + 6 > dst.len) break;
                    dst[out_idx] = '\\';
                    dst[out_idx + 1] = 'u';
                    dst[out_idx + 2] = 'f';
                    dst[out_idx + 3] = 'f';
                    dst[out_idx + 4] = 'f';
                    dst[out_idx + 5] = 'd';
                    out_idx += 6;
                    i += 1;
                }
            }
        }
        return dst[0..out_idx];
    }

    /// Helper to format a `Fetch.requestPaused` CDP event JSON payload with optional request headers and postData.
    /// Allocates into `out_dyn` when `post_data.len > 0` or the payload exceeds `out_buf.len`.
    pub static fn buildRequestPausedEventWithHeadersAndPost(
        req_id: []const u8,
        url: []const u8,
        method: []const u8,
        rtype: []const u8,
        frame_id: []const u8,
        network_id: []const u8,
        headers_json: []const u8,
        post_data: []const u8,
        out_buf: []u8,
        allocator: std.mem.Allocator,
        out_dyn: *([]u8)?,
    ): CdpEvent? {
        out_dyn.* = null;
        const net_id = if (network_id.len > 0) network_id else req_id;
        const hdr_obj = if (headers_json.len >= 2 && headers_json[0] == '{') headers_json else "{}";
        if (post_data.len == 0) {
            const payload = std.fmt.bufPrint(
                out_buf,
                "{{\"requestId\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"{s}\",\"headers\":{s},\"initialPriority\":\"High\",\"referrerPolicy\":\"no-referrer\"}},\"frameId\":\"{s}\",\"resourceType\":\"{s}\",\"networkId\":\"{s}\"}}",
                .{ req_id, url, method, hdr_obj, frame_id, rtype, net_id },
            ) catch {
                const dyn_buf = std.fmt.allocPrint(
                    allocator,
                    "{{\"requestId\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"{s}\",\"headers\":{s},\"initialPriority\":\"High\",\"referrerPolicy\":\"no-referrer\"}},\"frameId\":\"{s}\",\"resourceType\":\"{s}\",\"networkId\":\"{s}\"}}",
                    .{ req_id, url, method, hdr_obj, frame_id, rtype, net_id },
                ) catch return null;
                out_dyn.* = dyn_buf;
                return CdpEvent{
                    method: "Fetch.requestPaused",
                    params_json: dyn_buf,
                };
            };
            return CdpEvent{
                method: "Fetch.requestPaused",
                params_json: payload,
            };
        }

        const esc_len = FetchHandler.jsonEscapedLen(post_data);
        const esc_buf = allocator.alloc(u8, esc_len) catch return null;
        defer allocator.free(esc_buf);
        const pd_esc = FetchHandler.escapeJsonStringExact(post_data, esc_buf);

        const pd_b64 = Base64.encodeAlloc(allocator, post_data) catch return null;
        defer allocator.free(pd_b64);

        const dyn_buf = std.fmt.allocPrint(
            allocator,
            "{{\"requestId\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"{s}\",\"headers\":{s},\"initialPriority\":\"High\",\"referrerPolicy\":\"no-referrer\",\"hasPostData\":true,\"postData\":\"{s}\",\"postDataEntries\":[{{\"bytes\":\"{s}\"}}]}},\"frameId\":\"{s}\",\"resourceType\":\"{s}\",\"networkId\":\"{s}\"}}",
            .{ req_id, url, method, hdr_obj, pd_esc, pd_b64, frame_id, rtype, net_id },
        ) catch return null;
        out_dyn.* = dyn_buf;
        return CdpEvent{
            method: "Fetch.requestPaused",
            params_json: dyn_buf,
        };
    }

    /// Helper to format a `Fetch.requestPaused` CDP event JSON payload with optional request headers JSON object
    pub static fn buildRequestPausedEventWithHeaders(
        req_id: []const u8,
        url: []const u8,
        method: []const u8,
        rtype: []const u8,
        frame_id: []const u8,
        network_id: []const u8,
        headers_json: []const u8,
        out_buf: []u8,
    ): CdpEvent? {
        const net_id = if (network_id.len > 0) network_id else req_id;
        const hdr_obj = if (headers_json.len >= 2 && headers_json[0] == '{') headers_json else "{}";
        const payload = std.fmt.bufPrint(
            out_buf,
            "{{\"requestId\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"{s}\",\"headers\":{s},\"initialPriority\":\"High\",\"referrerPolicy\":\"no-referrer\"}},\"frameId\":\"{s}\",\"resourceType\":\"{s}\",\"networkId\":\"{s}\"}}",
            .{ req_id, url, method, hdr_obj, frame_id, rtype, net_id },
        ) catch return null;

        return CdpEvent{
            method: "Fetch.requestPaused",
            params_json: payload,
        };
    }

    /// Helper to format a `Fetch.requestPaused` CDP event JSON payload
    pub static fn buildRequestPausedEvent(
        req_id: []const u8,
        url: []const u8,
        method: []const u8,
        rtype: []const u8,
        frame_id: []const u8,
        network_id: []const u8,
        out_buf: []u8,
    ): CdpEvent? {
        return FetchHandler.buildRequestPausedEventWithHeaders(req_id, url, method, rtype, frame_id, network_id, "{}", out_buf);
    }
}
