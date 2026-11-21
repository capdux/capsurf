// tests/test_cors_headers.cp
// Tests for src/net/forbidden_headers.cp and src/net/cors.cp

import {
    isForbiddenRequestHeader,
    isForbiddenResponseHeader,
    isForbiddenRequestHeaderName,
    isForbiddenMethod,
} from "../../../src/net/forbidden_headers.cp";
import {
    scriptCredentialsMode,
    CredentialsMode,
    isSafelistMethod,
    isSafelistRequestHeader,
    isSafelistedResponseHeader,
    isHttpToken,
    parseTokenList,
    checkCorsResponse,
    checkCorsPreflight,
    corsCheck,
    PreflightCache,
} from "../../../src/net/cors.cp";
import { stripHttpWhitespace } from "../../../src/net/headers_parser.cp";

fn expect(cond: bool) {
    if (!cond) @panic("assertion failed");
}

fn testForbiddenHeaders() {
    // Forbidden request header names
    expect(isForbiddenRequestHeader("Host", "example.com"));
    expect(isForbiddenRequestHeader("host", "example.com"));
    expect(isForbiddenRequestHeader("Cookie", "session=123"));
    expect(isForbiddenRequestHeader("Cookie2", "val"));
    expect(isForbiddenRequestHeader("Origin", "https://foo.com"));
    expect(isForbiddenRequestHeader("Referer", "https://foo.com/bar"));
    expect(isForbiddenRequestHeader("Connection", "keep-alive"));
    expect(isForbiddenRequestHeader("Upgrade", "websocket"));
    expect(isForbiddenRequestHeader("Sec-Fetch-Mode", "cors"));
    expect(isForbiddenRequestHeader("Proxy-Authorization", "Basic 123"));

    // Allowed request headers
    expect(!isForbiddenRequestHeader("Accept", "text/html"));
    expect(!isForbiddenRequestHeader("Content-Type", "application/json"));
    expect(!isForbiddenRequestHeader("X-Custom-Header", "hello"));

    // Method override checks
    expect(isForbiddenRequestHeader("X-HTTP-Method-Override", "CONNECT"));
    expect(isForbiddenRequestHeader("X-HTTP-Method-Override", "TRACE"));
    expect(isForbiddenRequestHeader("X-HTTP-Method-Override", "TRACK"));
    expect(isForbiddenRequestHeader("X-Method-Override", "connect"));
    expect(isForbiddenRequestHeader("X-HTTP-Method", "GET, TRACE"));
    expect(!isForbiddenRequestHeader("X-HTTP-Method-Override", "DELETE"));
    expect(!isForbiddenRequestHeader("X-HTTP-Method-Override", "PUT, PATCH"));

    // Forbidden response headers
    expect(isForbiddenResponseHeader("Set-Cookie"));
    expect(isForbiddenResponseHeader("set-cookie2"));
    expect(!isForbiddenResponseHeader("Content-Type"));
}

fn testCorsSafelistAndTokens() {
    expect(scriptCredentialsMode(null) == CredentialsMode.SameOrigin);
    expect(scriptCredentialsMode("anonymous") == CredentialsMode.SameOrigin);
    expect(scriptCredentialsMode("use-credentials") == CredentialsMode.Include);
    expect(scriptCredentialsMode("USE-CREDENTIALS") == CredentialsMode.Include);

    // HTTP token validation
    expect(isHttpToken("content-type"));
    expect(isHttpToken("token_123*"));
    expect(!isHttpToken(""));
    expect(!isHttpToken("hello world"));
    expect(!isHttpToken("foo/bar"));

    // Whitespace stripping
    const stripped = stripHttpWhitespace("  \t\r\n hello \r\n ");
    expect(std.mem.eql(u8, stripped, "hello"));

    // Parse token list
    const tokens_opt = parseTokenList("  GET, POST , custom-header ");
    expect(tokens_opt != null);
    const tokens = tokens_opt.?;
    expect(tokens.len == 3);
    expect(tokens.contains("get"));
    expect(tokens.contains("POST"));
    expect(tokens.contains("custom-header"));
    expect(!tokens.contains("DELETE"));

    // Invalid token list
    expect(parseTokenList("GET, invalid token") == null);

    // Safelist methods
    expect(isSafelistMethod("GET"));
    expect(isSafelistMethod("head"));
    expect(isSafelistMethod("Post"));
    expect(!isSafelistMethod("PUT"));
    expect(!isSafelistMethod("DELETE"));
    expect(!isSafelistMethod("PATCH"));

    // Safelist request headers
    expect(isSafelistRequestHeader("accept", "text/html"));
    expect(isSafelistRequestHeader("accept-language", "en-US, en;q=0.9"));
    expect(isSafelistRequestHeader("content-type", "application/x-www-form-urlencoded"));
    expect(isSafelistRequestHeader("content-type", "multipart/form-data"));
    expect(isSafelistRequestHeader("content-type", "text/plain; charset=utf-8"));
    expect(!isSafelistRequestHeader("content-type", "application/json"));
    expect(isSafelistRequestHeader("range", "bytes=0-1024"));
    expect(isSafelistRequestHeader("range", "bytes=500-"));
    expect(!isSafelistRequestHeader("range", "bytes=100-50"));
    expect(!isSafelistRequestHeader("authorization", "Bearer token"));

    // Safelist response headers
    expect(isSafelistedResponseHeader("Cache-Control"));
    expect(isSafelistedResponseHeader("Content-Type"));
    expect(!isSafelistedResponseHeader("X-Custom"));
}

fn testCorsResponseAndPreflight() {
    const origin = "https://example.com";

    // checkCorsResponse
    expect(checkCorsResponse(origin, "*", false));
    expect(!checkCorsResponse(origin, "*", true)); // wildcard disallowed with credentials
    expect(checkCorsResponse(origin, "https://example.com", true));
    expect(!checkCorsResponse(origin, "https://other.com", false));
    expect(!checkCorsResponse(origin, "https://example.com, https://other.com", false));

    // checkCorsPreflight
    // Safelisted method & no extra headers
    expect(checkCorsPreflight(origin, "GET", "", "*", "", "", false));

    // Custom method allowed
    expect(checkCorsPreflight(origin, "PUT", "", "https://example.com", "GET, PUT", "", true));
    // Custom method not allowed
    expect(!checkCorsPreflight(origin, "DELETE", "", "https://example.com", "GET, PUT", "", true));

    // Custom headers allowed
    expect(checkCorsPreflight(origin, "POST", "X-Custom, Authorization", "https://example.com", "POST", "x-custom, authorization", true));
    // Header not allowed
    expect(!checkCorsPreflight(origin, "POST", "X-Unallowed", "https://example.com", "POST", "x-custom", true));

    // Wildcard header without credentials
    expect(checkCorsPreflight(origin, "POST", "X-Custom", "*", "POST", "*", false));
    // Wildcard header with credentials does NOT cover authorization
    expect(!checkCorsPreflight(origin, "POST", "Authorization", "https://example.com", "POST", "*", true));
}

fn testPreflightCache() {
    let cache = PreflightCache{};
    const partition = "ctx1";
    const origin = "https://app.com";
    const url = "https://api.com/v1";
    const now: i64 = 1000000;

    // Store preflight
    cache.store(partition, origin, url, false, "GET, POST, DELETE", "x-token, x-request-id", 300, now);

    // Lookup matching
    expect(cache.lookup(partition, origin, url, "DELETE", "x-token", false, now + 1000));
    expect(cache.lookup(partition, origin, url, "GET", "", false, now + 1000));

    // Method not in cache
    expect(!cache.lookup(partition, origin, url, "PATCH", "", false, now + 1000));

    // Header not in cache
    expect(!cache.lookup(partition, origin, url, "GET", "x-unknown", false, now + 1000));

    // Expired entry (ttl = 300s = 300000ms)
    expect(!cache.lookup(partition, origin, url, "DELETE", "x-token", false, now + 350000));
}

pub fn main() {
    testForbiddenHeaders();
    testCorsSafelistAndTokens();
    testCorsResponseAndPreflight();
    testPreflightCache();
}
