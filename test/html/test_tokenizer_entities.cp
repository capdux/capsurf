// test/html/test_tokenizer_entities.cp
// CapSurf Pure CP White-Box Unit Tests:
// 1. Named character entity decoding (&amp;, &lt;, &quot;, &apos;, &nbsp;, &copy;)
// 2. Decimal & hexadecimal numeric character references (&#65;, &#x1F600;, Windows-1252, U+0000)
// 3. Tokenizer DOCTYPE, comments, and attribute quoting (double, single, unquoted, boolean)
// 4. Tokenizer RawText (<style>) & ScriptData (<script>) state transitions

import { test, expect_eq } from "cap:test";
import { decodeEntity } from "../../src/html/entities.cp";
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

fn main() {
    test("1. Named Character Entity Decoding", () => {
        expectDecoded("&amp;rest", false, '&', 5, "&")?;
        expectDecoded("&lt;div&gt;", false, '<', 4, "<")?;
        expectDecoded("&quot;hi&quot;", false, '"', 6, "\"")?;
        expectDecoded("&apos;ok&apos;", false, '\'', 6, "'")?;
        expectDecoded("&nbsp;x", false, 0x00A0, 6, "\xc2\xa0")?;
        expectDecoded("&copy;2026", false, 0x00A9, 6, "\xc2\xa9")?;

        // Attribute context: semicolonless entity followed by '=' must NOT decode
        expect_eq(decodeEntity("&copy=1", true) == null, true);
        // Semicolonless entity followed by non-alnum non-'=' decodes in attribute
        expectDecoded("&copy 2026", true, 0x00A9, 5, "\xc2\xa9")?;
    });

    test("2. Decimal & Hexadecimal Numeric Character Entities", () => {
        expectDecoded("&#65;BC", false, 65, 5, "A")?;
        expectDecoded("&#x1F600;!", false, 0x1F600, 9, "\xf0\x9f\x98\x80")?;
        expectDecoded("&#X41;", false, 0x41, 6, "A")?;

        // HTML5 Windows-1252 remapping (0x80 -> U+20AC Euro) & U+0000 -> U+FFFD
        expectDecoded("&#128;", false, 0x20AC, 6, "\xe2\x82\xac")?;
        expectDecoded("&#0;", false, 0xFFFD, 4, "\xef\xbf\xbd")?;
    });

    test("3. Tokenizer DOCTYPE, Comments & Attribute Quoting", () => {
        const html =
            "<!DOCTYPE html>" ++
            "<!-- hello world -->" ++
            "<div id=\"main\" data-q='single' disabled checked=yes>" ++
            "<span/>" ++
            "</div>";

        let tok = Tokenizer.init(html);

        // 1) DOCTYPE
        const t1 = tok.next();
        expect_eq(t1 != null, true);
        if (tok1 := t1) {
            switch (tok1) {
                .Doctype => |dt| {
                    expect_eq(std.mem.eql(u8, dt.name, "html"), true);
                    expect_eq(dt.force_quirks, false);
                },
                * => { expect_eq(false, true); },
            }
        }

        // 2) Comment
        const t2 = tok.next();
        expect_eq(t2 != null, true);
        if (tok2 := t2) {
            switch (tok2) {
                .Comment => |c| {
                    expect_eq(std.mem.eql(u8, c, " hello world "), true);
                },
                * => { expect_eq(false, true); },
            }
        }

        // 3) <div ...> Start Tag with double-quoted, single-quoted, boolean, unquoted attrs
        const t3 = tok.next();
        expect_eq(t3 != null, true);
        if (tok3 := t3) {
            switch (tok3) {
                .Tag => |tag| {
                    expect_eq(tag.kind == TagKind.Start, true);
                    expect_eq(std.mem.eql(u8, tag.name, "div"), true);
                    expect_eq(tag.self_closing, false);
                    expect_eq(tag.attr_count, 4);
                    expect_eq(std.mem.eql(u8, tag.getAttribute("id") ?? "", "main"), true);
                    expect_eq(std.mem.eql(u8, tag.getAttribute("data-q") ?? "", "single"), true);
                    expect_eq(tag.hasAttribute("disabled"), true);
                    expect_eq(std.mem.eql(u8, tag.getAttribute("checked") ?? "", "yes"), true);
                },
                * => { expect_eq(false, true); },
            }
        }

        // 4) <span/> Self-closing Start Tag
        const t4 = tok.next();
        expect_eq(t4 != null, true);
        if (tok4 := t4) {
            switch (tok4) {
                .Tag => |tag| {
                    expect_eq(tag.kind == TagKind.Start, true);
                    expect_eq(std.mem.eql(u8, tag.name, "span"), true);
                    expect_eq(tag.self_closing, true);
                },
                * => { expect_eq(false, true); },
            }
        }

        // 5) </div> End Tag
        const t5 = tok.next();
        expect_eq(t5 != null, true);
        if (tok5 := t5) {
            switch (tok5) {
                .Tag => |tag| {
                    expect_eq(tag.kind == TagKind.End, true);
                    expect_eq(std.mem.eql(u8, tag.name, "div"), true);
                },
                * => { expect_eq(false, true); },
            }
        }

        // 6) EOF
        const t6 = tok.next();
        expect_eq(t6 != null, true);
        if (tok6 := t6) {
            switch (tok6) {
                .Eof => {},
                * => { expect_eq(false, true); },
            }
        }
        expect_eq(tok.next() == null, true);
    });

    test("4. Tokenizer RawText (<style>) & ScriptData (<script>) Splitting", () => {
        const html =
            "<style>.a > .b { content: \"&amp;\"; }</style>" ++
            "<script>const ok = 1 < 2 && \"&lt;\";</script>";

        let tok = Tokenizer.init(html);

        // <style> start tag
        const s_open = tok.next();
        expect_eq(s_open != null, true);
        if (t := s_open) {
            switch (t) {
                .Tag => |tag| {
                    expect_eq(tag.kind == TagKind.Start, true);
                    expect_eq(std.mem.eql(u8, tag.name, "style"), true);
                },
                * => { expect_eq(false, true); },
            }
        }
        tok.setState(NextTokenizerState.RawText);

        // RawText content: &amp; must remain un-decoded
        const s_body = tok.next();
        expect_eq(s_body != null, true);
        if (t := s_body) {
            switch (t) {
                .Characters => |txt| {
                    expect_eq(std.mem.eql(u8, txt, ".a > .b { content: \"&amp;\"; }"), true);
                },
                * => { expect_eq(false, true); },
            }
        }

        // </style> end tag
        const s_close = tok.next();
        expect_eq(s_close != null, true);
        if (t := s_close) {
            switch (t) {
                .Tag => |tag| {
                    expect_eq(tag.kind == TagKind.End, true);
                    expect_eq(std.mem.eql(u8, tag.name, "style"), true);
                },
                * => { expect_eq(false, true); },
            }
        }
        tok.setState(NextTokenizerState.Data);

        // <script> start tag
        const js_open = tok.next();
        expect_eq(js_open != null, true);
        if (t := js_open) {
            switch (t) {
                .Tag => |tag| {
                    expect_eq(tag.kind == TagKind.Start, true);
                    expect_eq(std.mem.eql(u8, tag.name, "script"), true);
                },
                * => { expect_eq(false, true); },
            }
        }
        tok.setState(NextTokenizerState.ScriptData);

        // ScriptData content: '< 2' and '&lt;' must remain intact
        const js_body = tok.next();
        expect_eq(js_body != null, true);
        if (t := js_body) {
            switch (t) {
                .Characters => |txt| {
                    expect_eq(std.mem.eql(u8, txt, "const ok = 1 < 2 && \"&lt;\";"), true);
                },
                * => { expect_eq(false, true); },
            }
        }

        // </script> end tag
        const js_close = tok.next();
        expect_eq(js_close != null, true);
        if (t := js_close) {
            switch (t) {
                .Tag => |tag| {
                    expect_eq(tag.kind == TagKind.End, true);
                    expect_eq(std.mem.eql(u8, tag.name, "script"), true);
                },
                * => { expect_eq(false, true); },
            }
        }
    });
}
