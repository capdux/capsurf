// test/net/test_headers_and_mime.cp
// Unit tests for src/net/headers_parser.cp and src/net/mime.cp

import { test, expect_eq } from "cap:test";
import {
    HeaderEntry,
    isHttpTokenChar,
    isValidHeaderName,
    isValidHeaderValue,
    stripHttpWhitespace,
    HeadersValidator,
    HeadersParser,
} from "../../src/net/headers_parser.cp";
import {
    MimeType,
    parseMime,
    normalizeBlobType,
} from "../../src/net/mime.cp";

fn lookupHeader(entries: []const HeaderEntry, name: []const u8): ([]const u8)? {
    for (entry of entries) {
        if (std.ascii.eqlIgnoreCase(entry.name, name)) {
            return entry.value;
        }
    }
    return null;
}

fn joinHeaderValues(entries: []const HeaderEntry, name: []const u8, out_buf: []u8): []const u8 {
    let pos: usize = 0;
    let first = true;
    for (entry of entries) {
        if (std.ascii.eqlIgnoreCase(entry.name, name)) {
            if (!first) {
                if (pos + 2 <= out_buf.len) {
                    out_buf[pos] = ',';
                    out_buf[pos + 1] = ' ';
                    pos += 2;
                }
            }
            const copy_len = if (pos + entry.value.len <= out_buf.len) entry.value.len else (out_buf.len - pos);
            @memcpy(out_buf[pos .. pos + copy_len], entry.value[0..copy_len]);
            pos += copy_len;
            first = false;
        }
    }
    return out_buf[0..pos];
}

fn mimeForExtension(path_or_url: []const u8): []const u8 {
    let clean = path_or_url;
    if (q_idx := std.mem.indexOfScalar(u8, clean, '?')) {
        clean = clean[0..q_idx];
    }
    if (h_idx := std.mem.indexOfScalar(u8, clean, '#')) {
        clean = clean[0..h_idx];
    }

    if (std.ascii.endsWithIgnoreCase(clean, ".html") || std.ascii.eqlIgnoreCase(clean, "html")) {
        return "text/html";
    } else if (std.ascii.endsWithIgnoreCase(clean, ".css") || std.ascii.eqlIgnoreCase(clean, "css")) {
        return "text/css";
    } else if (std.ascii.endsWithIgnoreCase(clean, ".js") || std.ascii.eqlIgnoreCase(clean, "js")) {
        return "application/javascript";
    } else if (std.ascii.endsWithIgnoreCase(clean, ".json") || std.ascii.eqlIgnoreCase(clean, "json")) {
        return "application/json";
    } else if (std.ascii.endsWithIgnoreCase(clean, ".png") || std.ascii.eqlIgnoreCase(clean, "png")) {
        return "image/png";
    } else if (std.ascii.endsWithIgnoreCase(clean, ".wasm") || std.ascii.eqlIgnoreCase(clean, "wasm")) {
        return "application/wasm";
    }
    return "application/octet-stream";
}

fn main() {
    test("1. HTTP header parsing, case-insensitive lookup, multi-value joining, and whitespace trimming", () => {
        const raw =
            "Host:   example.com \r\n" ++
            "Content-Type: \t application/json; charset=utf-8 \t\r\n" ++
            "X-Custom: alpha\r\n" ++
            "x-custom:   beta  \r\n" ++
            "X-CUSTOM: gamma\r\n";

        let entries: [8]HeaderEntry = [HeaderEntry{}; 8];
        const count = HeadersParser.parseRaw(raw, entries[0..]);
        expect_eq(count, @as(usize, 5));

        const parsed = entries[0..count];
        expect_eq(std.mem.eql(u8, parsed[0].name, "Host"), true);
        expect_eq(std.mem.eql(u8, parsed[0].value, "example.com"), true);

        // Case-insensitive lookup
        const host_val = lookupHeader(parsed, "host");
        expect_eq(host_val != null, true);
        expect_eq(std.mem.eql(u8, host_val.?, "example.com"), true);

        const ct_val = lookupHeader(parsed, "CONTENT-TYPE");
        expect_eq(ct_val != null, true);
        expect_eq(std.mem.eql(u8, ct_val.?, "application/json; charset=utf-8"), true);

        expect_eq(lookupHeader(parsed, "X-Missing") == null, true);

        // Multi-value header joining
        let join_buf: [128]u8 = [0; 128];
        const joined = joinHeaderValues(parsed, "X-Custom", join_buf[0..]);
        expect_eq(std.mem.eql(u8, joined, "alpha, beta, gamma"), true);

        // HTTP whitespace trimming
        expect_eq(std.mem.eql(u8, stripHttpWhitespace("  \t\r\n hello world \r\n\t "), "hello world"), true);
        expect_eq(std.mem.eql(u8, stripHttpWhitespace(" \t\r\n "), ""), true);
    });

    test("2. Malformed header line, control-character, and CRLF injection boundary guards", () => {
        // Header name token validation
        expect_eq(isHttpTokenChar('A'), true);
        expect_eq(isHttpTokenChar('-'), true);
        expect_eq(isHttpTokenChar('~'), true);
        expect_eq(isHttpTokenChar(' '), false);
        expect_eq(isHttpTokenChar(':'), false);

        expect_eq(isValidHeaderName("X-Valid-Name_1"), true);
        expect_eq(HeadersValidator.isValidName(""), false);
        expect_eq(HeadersValidator.isValidName("Bad Name"), false);
        expect_eq(HeadersValidator.isValidName("Bad:Name"), false);
        expect_eq(HeadersValidator.isValidName("Bad\r\nName"), false);
        expect_eq(HeadersValidator.isValidName("Bad\x00Name"), false);

        // Header value CRLF / NUL injection guards
        expect_eq(isValidHeaderValue("normal value\twith tab"), true);
        expect_eq(isValidHeaderValue(""), true);
        expect_eq(HeadersValidator.isValidValue("val\r\nX-Injected: evil"), false);
        expect_eq(HeadersValidator.isValidValue("val\nnext"), false);
        expect_eq(HeadersValidator.isValidValue("val\rnext"), false);
        expect_eq(HeadersValidator.isValidValue("val\x00nul"), false);

        expect_eq(HeadersValidator.validateHeader("X-Ok", "safe"), true);
        expect_eq(HeadersValidator.validateHeader("X Bad", "safe"), false);
        expect_eq(HeadersValidator.validateHeader("X-Ok", "un\r\nsafe"), false);

        // Malformed raw lines (no colon, empty lines) and output capacity bound
        const malformed = "\r\nno-colon-here\r\nValid-One: 1\r\nstill-no-colon\r\nValid-Two: 2\r\n";
        let small_out: [1]HeaderEntry = [HeaderEntry{}; 1];
        const parsed_count = HeadersParser.parseRaw(malformed, small_out[0..]);
        expect_eq(parsed_count, @as(usize, 1));
        expect_eq(std.mem.eql(u8, small_out[0].name, "Valid-One"), true);
        expect_eq(std.mem.eql(u8, small_out[0].value, "1"), true);
    });

    test("3. MIME type parsing, charset extraction, and lowercase normalization", () => {
        // Basic type/subtype + charset
        const m1 = parseMime("text/html; charset=utf-8");
        expect_eq(m1 != null, true);
        expect_eq(std.mem.eql(u8, m1.?.essence, "text/html"), true);
        expect_eq(std.mem.eql(u8, m1.?.charset, "utf-8"), true);

        // Quoted charset, extra whitespace, and additional params
        const m2 = MimeType.parse("  TEXT/PLAIN ;  foo=bar ; CHARSET = \"UTF-8\" ; baz=1 ");
        expect_eq(m2 != null, true);
        expect_eq(std.mem.eql(u8, m2.?.essence, "TEXT/PLAIN"), true);
        expect_eq(std.mem.eql(u8, m2.?.charset, "UTF-8"), true);

        // First charset parameter wins per WHATWG MIME spec
        const m3 = parseMime("application/json; charset=iso-8859-1; charset=utf-8");
        expect_eq(m3 != null, true);
        expect_eq(std.mem.eql(u8, m3.?.essence, "application/json"), true);
        expect_eq(std.mem.eql(u8, m3.?.charset, "iso-8859-1"), true);

        // Malformed MIME strings rejected
        expect_eq(parseMime("") == null, true);
        expect_eq(parseMime("   \t ") == null, true);
        expect_eq(parseMime("text") == null, true);
        expect_eq(parseMime("/html") == null, true);
        expect_eq(parseMime("text/") == null, true);
        expect_eq(parseMime("text /html") == null, true);
        expect_eq(parseMime("text/ht ml") == null, true);

        // Lowercase normalization via normalizeBlobType
        let norm_buf: [64]u8 = [0; 64];
        const norm_ok = normalizeBlobType(" TEXT/HTML; Charset=UTF-8 ", norm_buf[0..]);
        expect_eq(std.mem.eql(u8, norm_ok, " text/html; charset=utf-8 "), true);

        // Control chars (< 0x20) or non-ASCII (> 0x7E) rejected with ""
        expect_eq(std.mem.eql(u8, normalizeBlobType("text/plain\n", norm_buf[0..]), ""), true);
        expect_eq(std.mem.eql(u8, normalizeBlobType("text/\x7f", norm_buf[0..]), ""), true);
    });

    test("4. MIME sniffing and extension-to-MIME mapping", () => {
        expect_eq(std.mem.eql(u8, mimeForExtension("/index.html"), "text/html"), true);
        expect_eq(std.mem.eql(u8, mimeForExtension("/styles/app.CSS?v=2#top"), "text/css"), true);
        expect_eq(std.mem.eql(u8, mimeForExtension("/bundle.js"), "application/javascript"), true);
        expect_eq(std.mem.eql(u8, mimeForExtension("/data/config.json"), "application/json"), true);
        expect_eq(std.mem.eql(u8, mimeForExtension("/assets/logo.PNG"), "image/png"), true);
        expect_eq(std.mem.eql(u8, mimeForExtension("/pkg/engine.wasm"), "application/wasm"), true);
        expect_eq(std.mem.eql(u8, mimeForExtension("/raw/blob.bin"), "application/octet-stream"), true);
        expect_eq(std.mem.eql(u8, mimeForExtension("/no_extension"), "application/octet-stream"), true);

        // Every mapped MIME type must parse cleanly via parseMime
        const exts: [7][]const u8 = ["html", "css", "js", "json", "png", "wasm", "unknown"];
        let idx: usize = 0;
        while (idx < exts.len) : (idx += 1) {
            const mapped = mimeForExtension(exts[idx]);
            const parsed = parseMime(mapped);
            expect_eq(parsed != null, true);
        }
    });
}
