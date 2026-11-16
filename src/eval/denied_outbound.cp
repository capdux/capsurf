// Migrated from vendor/kitesurf/packages/engine/src/eval/denied-outbound.ts
// Rejects requests sent through the renderer's outbound binding with HTTP 403.

pub const DENIED_BLOCKED_REASON: []const u8 = "renderer-no-egress";
pub const DENIED_STATUS_CODE: u32 = 403;
pub const DENIED_BODY: []const u8 = "kitesurf: renderer egress denied";

pub struct DeniedOutboundResponse {
    status: u32 = 403,
    status_text: []const u8 = "Forbidden",
    blocked_reason: []const u8 = DENIED_BLOCKED_REASON,
    content_type: []const u8 = "text/plain; charset=utf-8",
    body: []const u8 = DENIED_BODY,
}

pub fn makeDeniedOutboundResponse(url: []const u8, out_buf: []u8): []const u8 {
    * = url;
    return std.fmt.bufPrint(
        out_buf,
        "HTTP/1.1 403 Forbidden\r\n" ++
        "Content-Type: text/plain; charset=utf-8\r\n" ++
        "Content-Length: {d}\r\n" ++
        "x-kitesurf-blocked: renderer-no-egress\r\n" ++
        "Connection: close\r\n\r\n" ++
        "{s}",
        .{ DENIED_BODY.len, DENIED_BODY },
    ) catch "";
}

pub struct DeniedOutbound {
    static fn fetch(url: []const u8, out_buf: []u8): []const u8 {
        return makeDeniedOutboundResponse(url, out_buf);
    }

    static fn getResponse(): DeniedOutboundResponse {
        return DeniedOutboundResponse{};
    }
}
