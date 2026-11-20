// test/html/test_entities_and_tokenizer_edge.cp
// CapSurf Pure CP White-Box Unit Tests:
// 1. HTML5 numeric entity edge cases: &#0;, >0x10FFFF, surrogates, C1 0x80..0x9F Windows-1252 remapping
// 2. Semicolonless named/numeric entity tolerance & attribute context protection
// 3. encodeCodepointUtf8 UTF-8 byte length boundaries & out-of-range fallback
// 4. Tokenizer malformed tag, attribute, bogus comment, and EOF recovery

import { test, expect_eq } from "cap:test";
import { decodeEntity, encodeCodepointUtf8 } from "../../src/html/entities.cp";
import { Tokenizer } from "../../src/html/tokenizer.cp";
import { Token, TagKind, NextTokenizerState } from "../../src/html/tokens.cp";

fn expectDecoded(slice: []const u8, in_attr: bool, expected_cp: u32, expected_consumed: usize, expected_utf8: []const u8)! {
    const res = decodeEntity(slice, in_attr);
    expect_eq(res != null, true);
    if (ent := res) {
        expect_eq(ent.isSingle(), true);
        expect_eq(ent.codepoints[0], expected_cp);
        expect_eq(ent.bytes_consumed, expected_consumed);
        let buf: [8]u8 = [0; 8];
        const written = ent.toUtf8(buf[0..8]);
        expect_eq(std.mem.eql(u8, buf[0..written], expected_utf8), true);
    }
}

fn expectNextCharacters(tok: *Tokenizer, expected: []const u8)! {
    const res = tok.next();
    expect_eq(res != null, true);
    if (t := res) {
        switch (t) {
            .Characters => |txt| {
                expect_eq(std.mem.eql(u8, txt, expected), true);
            },
            * => { expect_eq(false, true); },
        }
    }
}

fn expectNextComment(tok: *Tokenizer, expected: []const u8)! {
    const res = tok.next();
    expect_eq(res != null, true);
    if (t := res) {
        switch (t) {
            .Comment => |c| {
                expect_eq(std.mem.eql(u8, c, expected), true);
            },
            * => { expect_eq(false, true); },
        }
    }
}

fn expectNextEof(tok: *Tokenizer)! {
    const res = tok.next();
    expect_eq(res != null, true);
    if (t := res) {
        switch (t) {
            .Eof => {},
            * => { expect_eq(false, true); },
        }
    }
    expect_eq(tok.next() == null, true);
}

fn main() {
    test("1. Numeric Entity Edge Cases: NUL, >0x10FFFF, Surrogates", () => {
        // NUL -> U+FFFD
        expectDecoded("&#0;", false, 0xFFFD, 4, "\xef\xbf\xbd")?;
        expectDecoded("&#0000;", false, 0xFFFD, 7, "\xef\xbf\xbd")?;
        expectDecoded("&#x0;", false, 0xFFFD, 5, "\xef\xbf\xbd")?;
        expectDecoded("&#X00;", false, 0xFFFD, 6, "\xef\xbf\xbd")?;

        // Out of Unicode range (> 0x10FFFF) -> U+FFFD
        expectDecoded("&#x110000;", false, 0xFFFD, 10, "\xef\xbf\xbd")?;
        expectDecoded("&#1114112;", false, 0xFFFD, 10, "\xef\xbf\xbd")?;
        expectDecoded("&#99999999;", false, 0xFFFD, 11, "\xef\xbf\xbd")?;
        expectDecoded("&#x11000000;", false, 0xFFFD, 12, "\xef\xbf\xbd")?;
        expectDecoded("&#x10FFFF;", false, 0x10FFFF, 10, "\xf4\x8f\xbf\xbf")?;

        // UTF-16 Surrogates (0xD800..0xDFFF) -> U+FFFD
        expectDecoded("&#xD800;", false, 0xFFFD, 8, "\xef\xbf\xbd")?;
        expectDecoded("&#xDFFF;", false, 0xFFFD, 8, "\xef\xbf\xbd")?;
        expectDecoded("&#55296;", false, 0xFFFD, 8, "\xef\xbf\xbd")?;

        // Invalid / empty numeric references
        expect_eq(decodeEntity("&", false) == null, true);
        expect_eq(decodeEntity("&#", false) == null, true);
        expect_eq(decodeEntity("&#;", false) == null, true);
        expect_eq(decodeEntity("&#x;", false) == null, true);
        expect_eq(decodeEntity("&unknown;", false) == null, true);
    });

    test("2. C1 Control Range 0x80..0x9F Windows-1252 Remapping Table", () => {
        // Mapped Windows-1252 entries
        expectDecoded("&#x80;", false, 0x20AC, 6, "\xe2\x82\xac")?; // Euro
        expectDecoded("&#x82;", false, 0x201A, 6, "\xe2\x80\x9a")?;
        expectDecoded("&#x83;", false, 0x0192, 6, "\xc6\x92")?;
        expectDecoded("&#x85;", false, 0x2026, 6, "\xe2\x80\xa6")?; // Ellipsis
        expectDecoded("&#x8C;", false, 0x0152, 6, "\xc5\x92")?;
        expectDecoded("&#x91;", false, 0x2018, 6, "\xe2\x80\x98")?;
        expectDecoded("&#x93;", false, 0x201C, 6, "\xe2\x80\x9c")?;
        expectDecoded("&#x96;", false, 0x2013, 6, "\xe2\x80\x93")?; // En Dash
        expectDecoded("&#x97;", false, 0x2014, 6, "\xe2\x80\x94")?; // Em Dash
        expectDecoded("&#x99;", false, 0x2122, 6, "\xe2\x84\xa2")?; // Trade Mark
        expectDecoded("&#x9C;", false, 0x0153, 6, "\xc5\x93")?;
        expectDecoded("&#x9F;", false, 0x0178, 6, "\xc5\xb8")?;

        // Unmapped C1 entries (0x81, 0x8D, 0x8F, 0x90, 0x9D) remain unchanged
        expectDecoded("&#x81;", false, 0x0081, 6, "\xc2\x81")?;
        expectDecoded("&#x8D;", false, 0x008D, 6, "\xc2\x8d")?;
        expectDecoded("&#x8F;", false, 0x008F, 6, "\xc2\x8f")?;
        expectDecoded("&#x90;", false, 0x0090, 6, "\xc2\x90")?;
        expectDecoded("&#x9D;", false, 0x009D, 6, "\xc2\x9d")?;
    });

    test("3. Semicolonless Tolerance & Named Entity Coverage", () => {
        // Numeric without semicolon
        expectDecoded("&#65B", false, 65, 4, "A")?;
        expectDecoded("&#x41!", false, 0x41, 5, "A")?;

        // Legacy named entities without semicolon in text context
        expectDecoded("&amp ", false, '&', 4, "&")?;
        expectDecoded("&lt;", false, '<', 4, "<")?;
        expectDecoded("&lt!", false, '<', 3, "<")?;
        expectDecoded("&gt ", false, '>', 3, ">")?;
        expectDecoded("&quot.", false, '"', 5, "\"")?;
        expectDecoded("&nbsp;", false, 0x00A0, 6, "\xc2\xa0")?;
        expectDecoded("&nbsp!", false, 0x00A0, 5, "\xc2\xa0")?;

        // Attribute context: semicolonless followed by '=' or alphanumeric must NOT decode
        expect_eq(decodeEntity("&amp=1", true) == null, true);
        expect_eq(decodeEntity("&amp1", true) == null, true);
        expect_eq(decodeEntity("&lt=2", true) == null, true);
        expect_eq(decodeEntity("&ltA", true) == null, true);
        expect_eq(decodeEntity("&copy1", true) == null, true);
        expect_eq(decodeEntity("&nbsp=1", true) == null, true);

        // Attribute context: semicolonless followed by non-alnum non-'=' DOES decode
        expectDecoded("&amp x", true, '&', 4, "&")?;
        expectDecoded("&lt!x", true, '<', 3, "<")?;

        // High-frequency named entities
        expectDecoded("&trade;", false, 0x2122, 7, "\xe2\x84\xa2")?;
        expectDecoded("&hellip;", false, 0x2026, 8, "\xe2\x80\xa6")?;
        expectDecoded("&mdash;", false, 0x2014, 7, "\xe2\x80\x94")?;
        expectDecoded("&larr;", false, 0x2190, 6, "\xe2\x86\x90")?;
        expectDecoded("&rarr;", false, 0x2192, 6, "\xe2\x86\x92")?;

        // encodeCodepointUtf8 byte-width boundaries & out-of-range fallback
        let buf: [8]u8 = [0; 8];
        expect_eq(encodeCodepointUtf8(0x7F, buf[0..8]), 1);
        expect_eq(encodeCodepointUtf8(0x80, buf[0..8]), 2);
        expect_eq(encodeCodepointUtf8(0x7FF, buf[0..8]), 2);
        expect_eq(encodeCodepointUtf8(0x800, buf[0..8]), 3);
        expect_eq(encodeCodepointUtf8(0xFFFF, buf[0..8]), 3);
        expect_eq(encodeCodepointUtf8(0x10000, buf[0..8]), 4);
        expect_eq(encodeCodepointUtf8(0x10FFFF, buf[0..8]), 4);
        expect_eq(encodeCodepointUtf8(0x110000, buf[0..8]), 3);
        expect_eq(std.mem.eql(u8, buf[0..3], "\xef\xbf\xbd"), true);
    });

    test("4. Tokenizer Malformed Tag & Attribute EOF Recovery", () => {
        // EOF right after tag name
        let t1 = Tokenizer.init("<div");
        const r1 = t1.next();
        expect_eq(r1 != null, true);
        if (tok := r1) {
            switch (tok) {
                .Tag => |tag| {
                    expect_eq(tag.kind == TagKind.Start, true);
                    expect_eq(std.mem.eql(u8, tag.name, "div"), true);
                },
                * => { expect_eq(false, true); },
            }
        }
        expectNextEof(&t1)?;

        // EOF inside double-quoted attribute value: recovers cleanly to Eof without hang
        let t2 = Tokenizer.init("<a href=\"https://ex");
        expectNextEof(&t2)?;

        // EOF inside single-quoted attribute value: recovers cleanly to Eof without hang
        let t3 = Tokenizer.init("<p title='unclosed");
        expectNextEof(&t3)?;

        // EOF after '=' in attribute: recovers cleanly to Eof without hang
        let t4 = Tokenizer.init("<input data-x=val data-y=");
        expectNextEof(&t4)?;

        // EOF in SelfClosingStartTag (<br/  without >)
        let t5 = Tokenizer.init("<br/ ");
        const r5 = t5.next();
        expect_eq(r5 != null, true);
        if (tok := r5) {
            switch (tok) {
                .Tag => |tag| {
                    expect_eq(std.mem.eql(u8, tag.name, "br"), true);
                    expect_eq(tag.self_closing, false);
                },
                * => { expect_eq(false, true); },
            }
        }
        expectNextEof(&t5)?;
    });

    test("5. Tokenizer Malformed Comments, Bogus Tags & RcData/PlainText", () => {
        // Unclosed <!-- comment at EOF
        let t1 = Tokenizer.init("<!-- unclosed");
        expectNextComment(&t1, " unclosed")?;
        expectNextEof(&t1)?;

        // Invalid '<' followed by space/digit, empty </>, bogus </123>, PI <?xml?>, <!decl>
        let t2 = Tokenizer.init("a < 1</></123><?xml?> <!decl>");
        expectNextCharacters(&t2, "a ")?;
        expectNextCharacters(&t2, "<")?;
        expectNextCharacters(&t2, " 1")?;
        // </> is ignored per spec
        expectNextComment(&t2, "123")?;
        expectNextComment(&t2, "xml?")?;
        expectNextCharacters(&t2, " ")?;
        expectNextComment(&t2, "decl")?;
        expectNextEof(&t2)?;

        // RcData (<title>) decodes entities & stops at </title>
        let t3 = Tokenizer.init("<title>A &amp; &#x43;</title>");
        const open_title = t3.next();
        expect_eq(open_title != null, true);
        t3.setState(NextTokenizerState.RcData);
        expectNextCharacters(&t3, "A ")?;
        expectNextCharacters(&t3, "&")?;
        expectNextCharacters(&t3, " ")?;
        expectNextCharacters(&t3, "C")?;
        const close_title = t3.next();
        expect_eq(close_title != null, true);
        if (tok := close_title) {
            switch (tok) {
                .Tag => |tag| {
                    expect_eq(tag.kind == TagKind.End, true);
                    expect_eq(std.mem.eql(u8, tag.name, "title"), true);
                },
                * => { expect_eq(false, true); },
            }
        }
        expectNextEof(&t3)?;

        // PlainText (<plaintext>) consumes all remaining bytes verbatim
        let t4 = Tokenizer.init("<plaintext><b>&amp;</b>");
        const open_pt = t4.next();
        expect_eq(open_pt != null, true);
        t4.setState(NextTokenizerState.PlainText);
        expectNextCharacters(&t4, "<b>&amp;</b>")?;
        expectNextEof(&t4)?;
    });
}
