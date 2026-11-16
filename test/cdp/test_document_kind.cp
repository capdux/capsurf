// test/cdp/test_document_kind.cp
// Unit test suite for src/cdp/document_kind.cp
// Migrated from vendor/kitesurf/packages/engine/test/document-kind.test.ts

import {
    DocumentKind,
    mimeEssence,
    isAttachment,
    documentKindFromMime,
    documentKind,
    escapeHtml,
    imageDocumentTitle,
    imageDocumentHtml,
    svgDocumentHtml,
    jsonViewerHtml,
    textDocumentHtml,
    JSON_PARSED_MAX_CHARS,
} from "../../src/cdp/document_kind.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

pub fn main() {
    // 1. DocumentKind MIME Essence & Classification
    {
        check("text/html utf-8", documentKind("text/html; charset=utf-8", null) == .html);
        check("TEXT/HTML upper", documentKind("TEXT/HTML", null) == .html);
        check("xhtml", documentKind("application/xhtml+xml", null) == .html);
        check("json", documentKind("application/json", null) == .json);
        check("ld+json", documentKind("application/ld+json", null) == .json);
        check("svg", documentKind("image/svg+xml", null) == .svg);
        check("png", documentKind("image/png", null) == .image);
        check("jpeg", documentKind("image/jpeg", null) == .image);
        check("gif", documentKind("image/gif", null) == .image);
        check("webp", documentKind("image/webp", null) == .image);
        check("plain text", documentKind("text/plain; charset=utf-8", null) == .text);
        check("css", documentKind("text/css", null) == .text);
        check("text xml", documentKind("text/xml", null) == .text);
        check("javascript", documentKind("application/javascript", null) == .text);
        check("app xml", documentKind("application/xml", null) == .text);
        check("rss", documentKind("application/rss+xml", null) == .text);
        check("atom", documentKind("application/atom+xml", null) == .text);
        check("pdf blocked", documentKind("application/pdf", null) == .blocked);
        check("octet-stream blocked", documentKind("application/octet-stream", null) == .blocked);
        check("zip blocked", documentKind("application/zip", null) == .blocked);
        check("mp4 blocked", documentKind("video/mp4", null) == .blocked);
        check("mpeg blocked", documentKind("audio/mpeg", null) == .blocked);
        check("woff2 blocked", documentKind("font/woff2", null) == .blocked);
        check("avif blocked", documentKind("image/avif", null) == .blocked);
        check("bmp blocked", documentKind("image/bmp", null) == .blocked);
        check("x-icon blocked", documentKind("image/x-icon", null) == .blocked);
        check("empty default html", documentKind("", null) == .html);
    }

    // 2. Content-Disposition Attachments vs Inline
    {
        check("attachment filename", isAttachment("attachment; filename=a.html"));
        check("ATTACHMENT upper", isAttachment("ATTACHMENT"));
        check("form-data", isAttachment("form-data"));
        check("inline false", !isAttachment("inline"));
        check("inline with filename", !isAttachment("inline; filename=a.html"));
        check("filename only", !isAttachment("filename=a.html"));
        check("empty false", !isAttachment(""));
        check("null false", !isAttachment(null));

        check("attachment blocks html", documentKind("text/html", "attachment; filename=a.html") == .blocked);
        check("attachment blocks image", documentKind("image/png", "ATTACHMENT") == .blocked);
        check("form-data blocks", documentKind("text/html", "form-data") == .blocked);
        check("inline allows html", documentKind("text/html", "inline") == .html);
        check("inline filename allows html", documentKind("text/html", "inline; filename=a.html") == .html);
        check("filename allows html", documentKind("text/html", "filename=a.html") == .html);
        check("empty disposition allows html", documentKind("text/html", "") == .html);
    }

    // 3. escapeHtml and HTML entity escaping
    {
        check("escapeHtml basic", std.mem.eql(u8, escapeHtml("a & b < c > d"), "a &amp; b &lt; c &gt; d"));
        check("escapeHtml plain", std.mem.eql(u8, escapeHtml("plain text"), "plain text"));
    }

    // 4. imageDocumentTitle & imageDocumentHtml
    {
        const png: [29]u8 = [
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0, 0, 0, 13, 0x49, 0x48, 0x44, 0x52, 0, 0, 0, 2,
            0, 0, 0, 3, 8, 6, 0, 0, 0,
        ];

        const title1 = imageDocumentTitle("https://ex.com/a/cat%20pic.png?x=1", &png);
        check("title1", std.mem.eql(u8, title1, "cat pic.png (2\xc3\x973)"));

        const title2 = imageDocumentTitle("https://ex.com/", &png);
        check("title2", std.mem.eql(u8, title2, "ex.com (2\xc3\x973)"));

        const title3 = imageDocumentTitle("data:image/png;base64,", &png);
        check("title3", std.mem.eql(u8, title3, "2\xc3\x973"));

        const title4 = imageDocumentTitle("https://ex.com/%3Cb%3E.png", &png);
        check("title4", std.mem.eql(u8, title4, "<b>.png (2\xc3\x973)"));

        let out_buf_img: [2048]u8 = [0; 2048];
        const html = imageDocumentHtml(&png, "QUJD", "image/png; foo=bar", "https://ex.com/a/cat%20pic.png?x=1", &out_buf_img);
        check("html img src", std.mem.indexOf(u8, html, "<img src=\"data:image/png;base64,QUJD\">") != null);
        check("html title", std.mem.indexOf(u8, html, "<title>cat pic.png (2\xc3\x973)</title>") != null);

        let out_esc: [2048]u8 = [0; 2048];
        const html_esc = imageDocumentHtml(&png, "", "image/png", "https://ex.com/%3Cb%3E.png", &out_esc);
        check("html esc title", std.mem.indexOf(u8, html_esc, "<title>&lt;b&gt;.png (2\xc3\x973)</title>") != null);
    }

    // 5. svgDocumentHtml injection & layout rules
    {
        let out_buf_svg1: [2048]u8 = [0; 2048];
        const svg_raw = "<svg width=\"50%\" height=\"80%\" viewBox=\"0 0 10 10\"><rect/></svg>";
        const html = svgDocumentHtml(svg_raw, 800, 600, &out_buf_svg1);
        check("svg pct size", std.mem.indexOf(u8, html, "body>svg{display:block;width:50vw;height:80vh;}") != null);
        check("svg rect", std.mem.indexOf(u8, html, "<rect/></svg>") != null);

        let out_buf_svg2: [2048]u8 = [0; 2048];
        const svg_meta = "<svg><meta charset=\"utf-8\"><circle/></svg>";
        const html2 = svgDocumentHtml(svg_meta, 800, 600, &out_buf_svg2);
        check("svg meta lift", std.mem.indexOf(u8, html2, "<meta charset=\"utf-8\">") != null);
        check("svg auto viewBox", std.mem.indexOf(u8, html2, "viewBox=\"0 0 800 600\"") != null);
        check("svg 100vw/vh", std.mem.indexOf(u8, html2, "width:100vw;height:100vh;") != null);
    }

    // 6. jsonViewerHtml parsed vs raw
    {
        let out_buf_json: [4096]u8 = [0; 4096];
        const html = jsonViewerHtml("{\"a\":1}", "https://ex.com/a.json", &out_buf_json);
        check("json parsed id", std.mem.indexOf(u8, html, "id=\"json-parsed\"") != null);
        check("json key span", std.mem.indexOf(u8, html, "<span class=\"json-key\">\"a\"</span>") != null);
        check("json num span", std.mem.indexOf(u8, html, "<span class=\"json-number\">1</span>") != null);

        let out_raw: [4096]u8 = [0; 4096];
        const html_invalid = jsonViewerHtml("not json", "https://ex.com/invalid.json", &out_raw);
        check("json invalid raw pre", std.mem.indexOf(u8, html_invalid, "<pre>not json</pre>") != null);
    }

    // 7. textDocumentHtml preformatted text
    {
        let out_buf_text: [1024]u8 = [0; 1024];
        const html = textDocumentHtml("hello <world> & friends", "https://ex.com/test.txt", &out_buf_text);
        check("text title", std.mem.indexOf(u8, html, "<title>https://ex.com/test.txt</title>") != null);
        check("text pre", std.mem.indexOf(u8, html, "<pre>hello &lt;world&gt; &amp; friends</pre>") != null);
    }

    std.debug.print("ALL DOCUMENT_KIND TESTS PASSED SUCCESSFULLY!\n", .{});
}
