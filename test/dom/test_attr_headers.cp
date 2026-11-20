// test/dom/test_attr_headers.cp
// Tests CSS attr() evaluator and FetchOriginPolicy origin computation & headers validation.

import { evaluateCssAttr, parseAttrExpr, ParsedAttr } from "../../src/css/attr_evaluator.cp";
import {
    FetchOriginPolicy,
    HeadersValidator,
    HeadersParser,
    HeaderEntry,
    isValidHeaderName,
    isValidHeaderValue,
} from "../../src/net/headers_parser.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. evaluateCssAttr tests
    {
        // 1.1 attr(data-u type(<url>)) with url value -> ""
        const res1 = evaluateCssAttr("attr(data-u type(<url>))", "url(\"https://example.test/a.png\")");
        assert(std.mem.eql(u8, res1, ""), "attr(data-u type(<url>)) returns empty");

        // 1.2 attr(data-u type(<url>), none) with url value -> "none"
        const res2 = evaluateCssAttr("attr(data-u type(<url>), none)", "url(\"https://example.test/a.png\")");
        assert(std.mem.eql(u8, res2, "none"), "attr(data-u type(<url>), none) returns none");

        // 1.3 attr(data-n type(<number>)) with "3" -> "3"
        const res3 = evaluateCssAttr("attr(data-n type(<number>))", "3");
        assert(std.mem.eql(u8, res3, "3"), "attr(data-n type(<number>)) returns 3");

        // 1.4 attr(missing, fallback-val) with null -> "fallback-val"
        const res4 = evaluateCssAttr("attr(missing, fallback-val)", null);
        assert(std.mem.eql(u8, res4, "fallback-val"), "attr(missing, fallback-val) returns fallback-val");

        // Additional attr() tests
        // Missing number with fallback
        const res5 = evaluateCssAttr("attr(data-n type(<number>), 42)", null);
        assert(std.mem.eql(u8, res5, "42"), "missing number with fallback returns fallback");

        // Missing number without fallback
        const res6 = evaluateCssAttr("attr(data-n type(<number>))", null);
        assert(std.mem.eql(u8, res6, ""), "missing number without fallback returns empty");

        // Untyped with value present
        const res7 = evaluateCssAttr("attr(data-text)", "hello world");
        assert(std.mem.eql(u8, res7, "hello world"), "untyped with value returns value");

        // Untyped without value and without fallback
        const res8 = evaluateCssAttr("attr(data-text)", null);
        assert(std.mem.eql(u8, res8, ""), "untyped missing returns empty");

        // Untyped with value and fallback (value takes precedence)
        const res9 = evaluateCssAttr("attr(data-text, default-val)", "custom-val");
        assert(std.mem.eql(u8, res9, "custom-val"), "untyped with value ignores fallback");

        // Whitespace & case tolerance
        const res10 = evaluateCssAttr("  attr(  data-u  type(<url>)  ,  none  )  ", "url(\"foo\")");
        assert(std.mem.eql(u8, res10, "none"), "whitespace tolerance in attr()");

        const res11 = evaluateCssAttr("ATTR(data-u TYPE(<url>))", "url(\"foo\")");
        assert(std.mem.eql(u8, res11, ""), "case insensitive ATTR() and TYPE(<url>)");

        // parseAttrExpr verification
        const parsed = parseAttrExpr("attr(data-custom type(<number>), 99)");
        assert(parsed.is_valid, "parseAttrExpr is_valid");
        assert(std.mem.eql(u8, parsed.attr_name, "data-custom"), "parsed attr_name");
        assert(std.mem.eql(u8, parsed.type_name, "type(<number>)"), "parsed type_name");
        assert(parsed.fallback != null, "parsed fallback is not null");
        assert(std.mem.eql(u8, parsed.fallback.?, "99"), "parsed fallback value");
    }

    // 2. FetchOriginPolicy.computeOrigin tests
    {
        // 2.1 "about:blank" -> "null"
        const o1 = FetchOriginPolicy.computeOrigin("about:blank");
        assert(std.mem.eql(u8, o1, "null"), "about:blank origin is null");

        // 2.2 "data:text/html,..." -> "null"
        const o2 = FetchOriginPolicy.computeOrigin("data:text/html,<h1>Hello</h1>");
        assert(std.mem.eql(u8, o2, "null"), "data: url origin is null");

        // 2.3 "https://example.com:8080/path" -> "https://example.com:8080"
        const o3 = FetchOriginPolicy.computeOrigin("https://example.com:8080/path");
        assert(std.mem.eql(u8, o3, "https://example.com:8080"), "https with custom port origin");

        // Additional origin tests
        // Empty string -> "null"
        assert(std.mem.eql(u8, FetchOriginPolicy.computeOrigin(""), "null"), "empty url origin is null");

        // about:blank with query / hash -> "null"
        assert(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("about:blank?foo=bar#hash"), "null"), "about:blank with query is null");

        // about:srcdoc -> "null"
        assert(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("about:srcdoc"), "null"), "about:srcdoc origin is null");

        // javascript: -> "null"
        assert(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("javascript:void(0)"), "null"), "javascript url origin is null");

        // Standard https without port
        assert(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("https://example.com/path"), "https://example.com"), "standard https origin");
        assert(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("https://example.com"), "https://example.com"), "standard https origin without path");

        // Standard https with default port 443 -> stripped
        assert(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("https://example.com:443/path"), "https://example.com"), "default https port stripped");

        // Standard http with default port 80 -> stripped
        assert(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("http://example.com:80/path"), "http://example.com"), "default http port stripped");

        // Standard http with non-default port
        assert(std.mem.eql(u8, FetchOriginPolicy.computeOrigin("http://example.com:3000/index.html?a=1"), "http://example.com:3000"), "http with port 3000");

        // Same origin checks
        assert(FetchOriginPolicy.isSameOrigin("https://example.com:8080/a", "https://example.com:8080/b"), "same origin with port");
        assert(!FetchOriginPolicy.isSameOrigin("https://example.com:8080/a", "https://example.com/a"), "different port is not same origin");
        assert(!FetchOriginPolicy.isSameOrigin("about:blank", "about:blank"), "opaque origin is not same origin");
    }

    // 3. HeadersInit validation and parsing tests
    {
        // Header name validation
        assert(isValidHeaderName("content-type"), "content-type is valid header name");
        assert(isValidHeaderName("X-Custom_Header-123"), "alphanumeric with hyphen/underscore is valid");
        assert(!isValidHeaderName(""), "empty header name is invalid");
        assert(!isValidHeaderName("invalid header"), "header name with space is invalid");
        assert(!isValidHeaderName("header:name"), "header name with colon is invalid");

        // Header value validation
        assert(isValidHeaderValue("text/html; charset=utf-8"), "normal header value is valid");
        assert(!isValidHeaderValue("bad\x00val"), "header value with NUL is invalid");
        assert(!isValidHeaderValue("bad\r\nval"), "header value with CRLF is invalid");

        // HeadersValidator
        assert(HeadersValidator.validateHeader("Accept", "text/html"), "valid header pair");
        assert(!HeadersValidator.validateHeader("Bad Header", "val"), "invalid header name in pair");
        assert(!HeadersValidator.validateHeader("Accept", "bad\x00val"), "invalid header value in pair");
        assert(HeadersValidator.isForbiddenRequest("cookie"), "cookie is forbidden request header");
        assert(HeadersValidator.isForbiddenRequest("sec-fetch-mode"), "sec- header is forbidden request header");
        assert(!HeadersValidator.isForbiddenRequest("x-api-key"), "custom header is not forbidden request header");
        assert(HeadersValidator.isForbiddenResponse("set-cookie"), "set-cookie is forbidden response header");
        assert(!HeadersValidator.isForbiddenResponse("content-type"), "content-type is not forbidden response header");

        // HeadersParser.parseRaw
        let entries: [4]HeaderEntry = [HeaderEntry{} ; 4];
        const raw_headers = "Content-Type: text/plain\r\nAuthorization: Bearer token123\r\nX-Custom: value\r\n";
        const count = HeadersParser.parseRaw(raw_headers, entries[0..]);
        assert(count == 3, "parsed 3 headers");
        assert(std.mem.eql(u8, entries[0].name, "Content-Type"), "entry 0 name");
        assert(std.mem.eql(u8, entries[0].value, "text/plain"), "entry 0 value");
        assert(std.mem.eql(u8, entries[1].name, "Authorization"), "entry 1 name");
        assert(std.mem.eql(u8, entries[1].value, "Bearer token123"), "entry 1 value");
        assert(std.mem.eql(u8, entries[2].name, "X-Custom"), "entry 2 name");
        assert(std.mem.eql(u8, entries[2].value, "value"), "entry 2 value");
    }

    std.debug.print("test_attr_headers passed all assertions!\n", .{});
}
