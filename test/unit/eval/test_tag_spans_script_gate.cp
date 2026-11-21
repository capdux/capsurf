// test/eval/test_tag_spans_script_gate.cp
// Tests for src/html/tag_spans.cp and src/eval/script_gate.cp

import {
    TagSpan,
    closingTagEnd,
    htmlTagSpans,
    removeHtmlTagSpans,
} from "../../../src/html/tag_spans.cp";

import {
    GatedScriptDecision,
    documentBaseUrl,
    resolveScriptUrl,
    checkScriptPolicy,
} from "../../../src/eval/script_gate.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn testHtmlTagSpans() {
    std.debug.print("1. htmlTagSpans paired script tags\n", .{});
    const html = "<html><head><script>alert(1);</script><meta charset=\"utf-8\"><script src=\"app.js\"></script></head><body></body></html>";

    let spans: [16]TagSpan = [TagSpan{}; 16];
    const n = htmlTagSpans(html, "script", true, spans[0..]);
    check("found 2 script tags", n == 2);
    check("first script start", spans[0].start == 12);
    check("first script content", std.mem.eql(u8, html[spans[0].start..spans[0].end], "<script>alert(1);</script>"));
    check("second script content", std.mem.eql(u8, html[spans[1].start..spans[1].end], "<script src=\"app.js\"></script>"));

    std.debug.print("2. removeHtmlTagSpans\n", .{});
    let out_buf: [256]u8 = [0; 256];
    const stripped = removeHtmlTagSpans(html, spans[0..n], out_buf[0..]);
    check("stripped does not contain script", std.mem.indexOf(u8, stripped, "<script") == null);
    check("stripped contains meta", std.mem.indexOf(u8, stripped, "<meta charset=\"utf-8\">") != null);

    std.debug.print("3. htmlTagSpans unpaired meta tags\n", .{});
    let meta_spans: [16]TagSpan = [TagSpan{}; 16];
    const meta_n = htmlTagSpans(html, "meta", false, meta_spans[0..]);
    check("found 1 meta tag", meta_n == 1);
    check("meta tag match", std.mem.eql(u8, html[meta_spans[0].start..meta_spans[0].end], "<meta charset=\"utf-8\">"));

    std.debug.print("4. closingTagEnd edge cases\n", .{});
    const sample = "test</script >more";
    const end_pos = closingTagEnd(sample, "script", 0);
    check("closingTagEnd found", end_pos == 14);
}

fn testScriptGate() {
    std.debug.print("5. documentBaseUrl resolution\n", .{});
    let url_buf: [256]u8 = [0; 256];
    const base1 = documentBaseUrl("https://example.com/dir/page.html", "https://cdn.example.com/assets/", url_buf[0..]);
    check("absolute baseHref", std.mem.eql(u8, base1, "https://cdn.example.com/assets/"));

    let url_buf2: [256]u8 = [0; 256];
    const base2 = documentBaseUrl("https://example.com/dir/page.html", "sub/", url_buf2[0..]);
    check("relative baseHref", std.mem.eql(u8, base2, "https://example.com/dir/sub/"));

    std.debug.print("6. resolveScriptUrl path dot normalization\n", .{});
    let res_buf: [256]u8 = [0; 256];
    const s1 = resolveScriptUrl("../lib/vendor.js", "https://example.com/a/b/", res_buf[0..]);
    check("dot dot resolved", std.mem.eql(u8, s1, "https://example.com/a/lib/vendor.js"));

    let res_buf2: [256]u8 = [0; 256];
    const s2 = resolveScriptUrl("https://cdn.other.com/bundle.js", "https://example.com/", res_buf2[0..]);
    check("absolute script preserved", std.mem.eql(u8, s2, "https://cdn.other.com/bundle.js"));

    std.debug.print("7. checkScriptPolicy CSP checks\n", .{});
    // With empty policy -> allowed
    const dec1 = checkScriptPolicy("https://example.com/app.js", "", "", "");
    check("empty policy allowed", dec1.allowed);

    // With matching nonce -> allowed
    const dec2 = checkScriptPolicy("", "secret-nonce-123", "", "script-src 'nonce-secret-nonce-123'");
    check("matching nonce allowed", dec2.allowed);

    // With mismatched nonce -> blocked
    const dec3 = checkScriptPolicy("", "wrong-nonce", "", "script-src 'nonce-secret-nonce-123'");
    check("mismatched nonce blocked", !dec3.allowed);

    // With allowed host -> allowed
    const dec4 = checkScriptPolicy("https://cdn.example.com/bundle.js", "", "", "script-src https://cdn.example.com");
    check("allowed host allowed", dec4.allowed);

    // With forbidden host -> blocked
    const dec5 = checkScriptPolicy("https://evil.com/malware.js", "", "", "script-src https://cdn.example.com");
    check("forbidden host blocked", !dec5.allowed);
}

pub fn main() {
    testHtmlTagSpans();
    testScriptGate();
    std.debug.print("ALL TAG_SPANS & SCRIPT_GATE TESTS PASSED SUCCESSFULLY!\n", .{});
}
