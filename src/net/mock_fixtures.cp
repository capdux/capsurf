// src/net/mock_fixtures.cp
// CapSurf Internal Test Fixtures Handler for Mock Network Endpoints
// Aligned with vendor/kitesurf/packages/engine/test/fixtures-worker/src/dynamic/cors.ts

pub struct MockFixtures {
    static fn extractHost(url: []const u8): []const u8 {
        let rest = url;
        if (std.mem.startsWith(u8, rest, "http://")) {
            rest = rest[7..];
        } else if (std.mem.startsWith(u8, rest, "https://")) {
            rest = rest[8..];
        } else if (idx := std.mem.indexOf(u8, rest, "://")) {
            rest = rest[idx + 3..];
        }

        let auth_end: usize = 0;
        while (auth_end < rest.len) : (auth_end += 1) {
            const ch = rest[auth_end];
            if (ch == '/' || ch == '?' || ch == '#') break;
        }
        const authority = rest[0..auth_end];
        if (authority.len == 0) return "";

        if (authority[0] == '[') {
            if (close_bracket := std.mem.indexOfScalar(u8, authority, ']')) {
                return authority[1..close_bracket];
            }
            return authority;
        }

        if (colon_pos := std.mem.indexOfScalar(u8, authority, ':')) {
            return authority[0..colon_pos];
        }

        return authority;
    }

    /// Check if a URL targets an internal test fixture endpoint.
    /// Returns true if url contains "/__fixtures/" or if the hostname ends with ".invalid".
    static fn isFixtureUrl(url: []const u8): bool {
        if (std.mem.indexOf(u8, url, "/__fixtures/") != null) {
            return true;
        }
        const host = MockFixtures.extractHost(url);
        if (host.len >= 8 && std.ascii.endsWithIgnoreCase(host, ".invalid")) {
            return true;
        }
        return false;
    }

    /// Handle fixture requests. Currently implements /echo-headers.json by synthesizing
    /// a JSON response containing incoming request headers (origin, user-agent, accept).
    static fn handleFixtureRequest(
        url: []const u8,
        origin: []const u8,
        user_agent: []const u8,
        out_buf: []u8,
    ): ([]const u8)? {
        if (std.mem.indexOf(u8, url, "/echo-headers.json") == null) {
            return null;
        }

        const ua = if (user_agent.len == 0) "CapSurf/1.0.0" else user_agent;
        const orig = if (origin.len == 0) "null" else origin;

        const res = std.fmt.bufPrint(
            out_buf,
            "{{\"accept\":\"*/*\",\"origin\":\"{s}\",\"user-agent\":\"{s}\"}}",
            .{ orig, ua },
        ) catch return null;

        return res;
    }
}

pub fn isFixtureUrl(url: []const u8): bool {
    return MockFixtures.isFixtureUrl(url);
}

pub fn handleFixtureRequest(
    url: []const u8,
    origin: []const u8,
    user_agent: []const u8,
    out_buf: []u8,
): ([]const u8)? {
    return MockFixtures.handleFixtureRequest(url, origin, user_agent, out_buf);
}
