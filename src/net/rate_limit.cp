// Migrated from vendor/kitesurf/packages/engine/src/rate-limit.ts
// Builds rate-limit responses and logs limiter failures for engine entry points.
// Pure CP, zero-GC, compile-time memory safety, token-bucket rate limiter.

pub const MAX_RATE_LIMITER_ENTRIES: usize = 256;
pub const MAX_KEY_LEN: usize = 128;
pub const DEFAULT_RATE_BURST: u32 = 60;
pub const DEFAULT_REFILL_RATE: f64 = 10.0;

/// Tier of rate limiter failure logging.
pub enum RateLimiterTier {
    Account,
    PerIp,
    Aggregate,

    fn asStr(): []const u8 {
        return switch (_) {
            .Account => "account",
            .PerIp => "per-ip",
            .Aggregate => "aggregate",
        };
    }
}

/// Logs limiter failures for engine entry points.
pub fn warnLimiterFailure(tier: RateLimiterTier, err_msg: []const u8) {
    std.debug.print("[capsurf] {s} rate limiter failed (failing open): {s}\n", .{ tier.asStr(), err_msg });
}

/// Returns whether an entrypoint request is subject to rate limiting (e.g. WebSocket upgrade).
pub fn isRateLimitedEntrypoint(is_websocket_upgrade: bool): bool {
    return is_websocket_upgrade;
}

/// Helper to inspect upgrade header string for WebSocket upgrade.
pub fn isWebSocketUpgrade(upgrade_header: []const u8): bool {
    return std.ascii.eqlIgnoreCase(upgrade_header, "websocket");
}

/// Structured rate limit response data.
pub struct RateLimitResponse {
    status_code: u16 = 429,
    retry_after: u32 = 10,
    content_type: []const u8 = "text/plain; charset=utf-8",
    body: []const u8 = "",

    static fn init(message: []const u8, retry_after_seconds: u32): RateLimitResponse {
        return RateLimitResponse{
            status_code: 429,
            retry_after: retry_after_seconds,
            content_type: "text/plain; charset=utf-8",
            body: message,
        };
    }

    /// Formats this 429 response into raw HTTP/1.1 wire format in `out_buf`.
    fn format(*_, out_buf: []u8): []const u8 {
        return rateLimitedResponse(_.body, _.retry_after, out_buf);
    }
}

/// Returns a formatted plain-text HTTP/1.1 429 response with `Retry-After` header.
pub fn rateLimitedResponse(message: []const u8, retry_after_seconds: u32, out_buf: []u8): []const u8 {
    return std.fmt.bufPrint(
        out_buf,
        "HTTP/1.1 429 Too Many Requests\r\n" ++
        "Content-Type: text/plain; charset=utf-8\r\n" ++
        "Retry-After: {d}\r\n" ++
        "Content-Length: {d}\r\n" ++
        "Connection: close\r\n\r\n" ++
        "{s}",
        .{ retry_after_seconds, message.len, message },
    ) catch "";
}

/// Detailed result of a rate limit check.
pub struct RateLimitResult {
    limited: bool = false,
    remaining_tokens: f64 = 0.0,
    retry_after_seconds: u32 = 0,
}

/// Tracked IP / connection state for token bucket rate limiting.
pub struct RateLimiterEntry {
    key_buf: [MAX_KEY_LEN]u8 = [0; MAX_KEY_LEN],
    key_len: usize = 0,
    key_ext: []const u8 = "",
    tokens: f64 = 0.0,
    last_update_ms: i64 = 0,
    active: bool = false,
    last_seen_seq: u64 = 0,

    fn keyStr(*_): []const u8 {
        if (_.key_len > 0) return _.key_buf[0.._.key_len];
        return _.key_ext;
    }

    fn matchesKey(*_, target_key: []const u8): bool {
        if (!_.active) return false;
        return std.mem.eql(u8, _.keyStr(), target_key);
    }

    fn reset(*_) {
        _.key_len = 0;
        _.key_ext = "";
        _.tokens = 0.0;
        _.last_update_ms = 0;
        _.active = false;
        _.last_seen_seq = 0;
    }
}

/// Fixed-capacity, zero-allocation Token Bucket rate limiter.
/// Tracks request rates per IP or connection identifier without heap allocations.
pub struct RateLimiter {
    entries: [MAX_RATE_LIMITER_ENTRIES]RateLimiterEntry = [RateLimiterEntry{}; MAX_RATE_LIMITER_ENTRIES],
    capacity: u32 = DEFAULT_RATE_BURST,
    refill_rate_per_sec: f64 = DEFAULT_REFILL_RATE,
    access_counter: u64 = 0,
    active_count: usize = 0,

    static fn init(capacity: u32, refill_rate_per_sec: f64): RateLimiter {
        return RateLimiter{
            capacity: capacity,
            refill_rate_per_sec: refill_rate_per_sec,
        };
    }

    static fn initDefault(): RateLimiter {
        return RateLimiter{
            capacity: DEFAULT_RATE_BURST,
            refill_rate_per_sec: DEFAULT_REFILL_RATE,
        };
    }

    /// Number of active tracked clients.
    fn count(*_): usize {
        return _.active_count;
    }

    /// Checks if a client exceeds the rate limit and consumes 1 token if allowed.
    /// Returns true if rate limited, false if allowed.
    fn isRateLimited(*_, key: []const u8, now_ms: i64): bool {
        const res = _.checkAndConsume(key, 1.0, now_ms);
        return res.limited;
    }

    /// Convenience wrapper: returns true if request is allowed, false if limited.
    fn allow(*_, key: []const u8, now_ms: i64): bool {
        return !_.isRateLimited(key, now_ms);
    }

    /// Checks rate limit against token bucket, consumes tokens if allowed,
    /// and calculates retry-after duration if rate limited.
    fn checkAndConsume(*_, key: []const u8, cost: f64, now_ms: i64): RateLimitResult {
        if (key.len == 0) {
            return RateLimitResult{ limited: false, remaining_tokens: 0.0, retry_after_seconds: 0 };
        }

        const max_tokens = @as(f64, @floatFromInt(_.capacity));
        let slot_idx = _.findEntryIndex(key);

        if (slot_idx == null) {
            // Allocate new slot, evicting oldest if necessary
            slot_idx = _.allocateSlot(key, max_tokens, now_ms);
        }

        if (idx := slot_idx) {
            let entry = &_.entries[idx];
            _.access_counter += 1;
            entry.last_seen_seq = _.access_counter;

            // Refill tokens based on elapsed time
            if (now_ms > entry.last_update_ms) {
                const elapsed_s = @as(f64, @floatFromInt(now_ms - entry.last_update_ms)) / 1000.0;
                entry.tokens = @min(max_tokens, entry.tokens + elapsed_s * _.refill_rate_per_sec);
                entry.last_update_ms = now_ms;
            }

            if (entry.tokens >= cost) {
                entry.tokens -= cost;
                return RateLimitResult{
                    limited: false,
                    remaining_tokens: entry.tokens,
                    retry_after_seconds: 0,
                };
            } else {
                // Rate limited: compute required retry-after seconds
                let retry_sec: u32 = 1;
                if (_.refill_rate_per_sec > 0.0) {
                    const deficit = cost - entry.tokens;
                    const wait_s = @ceil(deficit / _.refill_rate_per_sec);
                    if (wait_s > 0.0) {
                        retry_sec = @as(u32, @intFromFloat(wait_s));
                    }
                }
                return RateLimitResult{
                    limited: true,
                    remaining_tokens: entry.tokens,
                    retry_after_seconds: retry_sec,
                };
            }
        }

        // Table completely full and eviction failed (fail open)
        return RateLimitResult{
            limited: false,
            remaining_tokens: 0.0,
            retry_after_seconds: 0,
        };
    }

    /// Resets rate limit quota for a specific client key.
    fn reset(*_, key: []const u8) {
        if (idx := _.findEntryIndex(key)) {
            _.entries[idx].reset();
            if (_.active_count > 0) _.active_count -= 1;
        }
    }

    /// Clears all tracked clients.
    fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_RATE_LIMITER_ENTRIES) : (i += 1) {
            _.entries[i].reset();
        }
        _.active_count = 0;
    }

    /// Finds index of active entry matching key.
    fn findEntryIndex(*_, key: []const u8): usize? {
        let i: usize = 0;
        while (i < MAX_RATE_LIMITER_ENTRIES) : (i += 1) {
            if (_.entries[i].matchesKey(key)) {
                return i;
            }
        }
        return null;
    }

    /// Allocates a slot for a new key, evicting the oldest entry if full.
    fn allocateSlot(*_, key: []const u8, initial_tokens: f64, now_ms: i64): usize? {
        if (_.active_count >= MAX_RATE_LIMITER_ENTRIES) {
            if (oldest := _.findOldestEntry()) {
                _.entries[oldest].reset();
                _.active_count -= 1;
            }
        }

        let slot_idx: usize = MAX_RATE_LIMITER_ENTRIES;
        let i: usize = 0;
        while (i < MAX_RATE_LIMITER_ENTRIES) : (i += 1) {
            if (!_.entries[i].active) {
                slot_idx = i;
                break;
            }
        }

        if (slot_idx >= MAX_RATE_LIMITER_ENTRIES) return null;

        let entry = &_.entries[slot_idx];
        if (key.len <= MAX_KEY_LEN) {
            @memcpy(entry.key_buf[0..key.len], key);
            entry.key_len = key.len;
            entry.key_ext = "";
        } else {
            entry.key_len = 0;
            entry.key_ext = key;
        }

        entry.tokens = initial_tokens;
        entry.last_update_ms = now_ms;
        entry.active = true;

        _.access_counter += 1;
        entry.last_seen_seq = _.access_counter;
        _.active_count += 1;

        return slot_idx;
    }

    /// Finds oldest active entry by access sequence.
    fn findOldestEntry(*_): usize? {
        let oldest_idx: usize = MAX_RATE_LIMITER_ENTRIES;
        let min_seq: u64 = std.math.maxInt(u64);

        let i: usize = 0;
        while (i < MAX_RATE_LIMITER_ENTRIES) : (i += 1) {
            if (_.entries[i].active) {
                if (_.entries[i].last_seen_seq < min_seq) {
                    min_seq = _.entries[i].last_seen_seq;
                    oldest_idx = i;
                }
            }
        }

        if (oldest_idx < MAX_RATE_LIMITER_ENTRIES) {
            return oldest_idx;
        }
        return null;
    }
}
