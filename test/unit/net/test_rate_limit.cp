// test/net/test_rate_limit.cp
// Tests for CapSurf RateLimitResponse and Token Bucket RateLimiter
// Migrated from vendor/kitesurf/packages/engine/src/rate-limit.ts

import {
    MAX_RATE_LIMITER_ENTRIES,
    DEFAULT_RATE_BURST,
    DEFAULT_REFILL_RATE,
    RateLimiterTier,
    RateLimitResponse,
    RateLimitResult,
    rateLimitedResponse,
    isRateLimitedEntrypoint,
    isWebSocketUpgrade,
    RateLimiter,
} from "../../../src/net/rate_limit.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn main() {
    // 1. Rate limiter entrypoint predicates
    check("isRateLimitedEntrypoint true", isRateLimitedEntrypoint(true));
    check("isRateLimitedEntrypoint false", !isRateLimitedEntrypoint(false));
    check("isWebSocketUpgrade websocket", isWebSocketUpgrade("websocket"));
    check("isWebSocketUpgrade WebSocket", isWebSocketUpgrade("WebSocket"));
    check("isWebSocketUpgrade UPGRADE", !isWebSocketUpgrade("upgrade"));

    // 2. Plain-text 429 response formatting
    let resp_buf: [512]u8 = [0; 512];
    const wire = rateLimitedResponse("Rate limit exceeded", 10, &resp_buf);
    check("wire non-empty", wire.len > 0);
    check("wire status 429", std.mem.indexOf(u8, wire, "HTTP/1.1 429 Too Many Requests") != null);
    check("wire content-type", std.mem.indexOf(u8, wire, "Content-Type: text/plain; charset=utf-8") != null);
    check("wire retry-after", std.mem.indexOf(u8, wire, "Retry-After: 10") != null);
    check("wire body", std.mem.indexOf(u8, wire, "Rate limit exceeded") != null);

    // 3. Structured RateLimitResponse
    let struct_resp = RateLimitResponse.init("Too many requests from this IP", 30);
    check("struct status 429", struct_resp.status_code == 429);
    check("struct retry 30", struct_resp.retry_after == 30);
    let wire2_buf: [512]u8 = [0; 512];
    const wire2 = struct_resp.format(&wire2_buf);
    check("wire2 retry-after 30", std.mem.indexOf(u8, wire2, "Retry-After: 30") != null);

    // 4. RateLimiter token bucket behavior
    let limiter = RateLimiter.init(3, 1.0);
    check("initial count 0", limiter.count() == 0);

    const client = "192.168.1.50";
    let now: i64 = 1000;

    // Burst: 3 requests allowed at time 1000
    check("req 1 allowed", limiter.allow(client, now));
    check("req 2 allowed", limiter.allow(client, now));
    check("req 3 allowed", limiter.allow(client, now));
    check("client tracked", limiter.count() == 1);

    // 4th request at time 1000: burst exhausted, rate-limited
    check("req 4 rate limited", limiter.isRateLimited(client, now));
    check("req 4 not allowed", !limiter.allow(client, now));

    // Inspect checkAndConsume result
    const res = limiter.checkAndConsume(client, 1.0, now);
    check("res limited", res.limited);
    check("res retry_after >= 1", res.retry_after_seconds >= 1);

    // Advance 2 seconds -> refilled 2 tokens
    now = 3000;
    check("req 5 allowed after refill", limiter.allow(client, now));

    // Reset quota for client
    limiter.reset(client);
    check("count after reset", limiter.count() == 0);

    // Clear all
    limiter.clear();
    check("count after clear", limiter.count() == 0);

    // Default limiter constants
    let def_limiter = RateLimiter.initDefault();
    check("def capacity", def_limiter.capacity == DEFAULT_RATE_BURST);

    std.debug.print("All rate_limit tests passed!\n", .{});
}
