// tests/test_html_tokenizer.cp
// Comprehensive Unit Test Suite for CapSurf HTML5 Tokenizer

import { test, expect_eq } from "cap:test";
import { Tokenizer } from "../../../src/html/tokenizer.cp";
import { Token, TagKind } from "../../../src/html/tokens.cp";
import { HtmlTag } from "../../../src/html/tags.cp";

fn main() {
    test("1. HTML5 Tokenizer: Basic tags, boolean & quoted attributes", () => {
        const html = "<div id=\"main\" class='hero active' disabled><span data-val=123>Text</span></div>";
        let tok = Tokenizer.init(html);

        // 1. <div id="main" class="hero active" disabled>
        if (t1 := tok.next()) {
            switch (t1) {
                .Tag => {
                    expect_eq(t1.Tag.kind == TagKind.Start, true);
                    expect_eq(t1.Tag.tag == HtmlTag.Div, true);
                    expect_eq(std.mem.eql(u8, t1.Tag.name, "div"), true);
                    expect_eq(std.mem.eql(u8, t1.Tag.getAttribute("id").?, "main"), true);
                    expect_eq(std.mem.eql(u8, t1.Tag.getAttribute("class").?, "hero active"), true);
                    expect_eq(t1.Tag.hasAttribute("disabled"), true);
                },
                * => unreachable,
            }
        } else {
            unreachable;
        }

        // 2. <span data-val=123>
        if (t2 := tok.next()) {
            switch (t2) {
                .Tag => {
                    expect_eq(t2.Tag.kind == TagKind.Start, true);
                    expect_eq(t2.Tag.tag == HtmlTag.Span, true);
                    expect_eq(std.mem.eql(u8, t2.Tag.getAttribute("data-val").?, "123"), true);
                },
                * => unreachable,
            }
        } else {
            unreachable;
        }

        // 3. Characters "Text"
        if (t3 := tok.next()) {
            switch (t3) {
                .Characters => {
                    expect_eq(std.mem.eql(u8, t3.Characters, "Text"), true);
                },
                * => unreachable,
            }
        } else {
            unreachable;
        }

        // 4. </span>
        if (t4 := tok.next()) {
            switch (t4) {
                .Tag => {
                    expect_eq(t4.Tag.kind == TagKind.End, true);
                    expect_eq(t4.Tag.tag == HtmlTag.Span, true);
                },
                * => unreachable,
            }
        } else {
            unreachable;
        }

        // 5. </div>
        if (t5 := tok.next()) {
            switch (t5) {
                .Tag => {
                    expect_eq(t5.Tag.kind == TagKind.End, true);
                    expect_eq(t5.Tag.tag == HtmlTag.Div, true);
                },
                * => unreachable,
            }
        } else {
            unreachable;
        }

        // 6. EOF
        if (t6 := tok.next()) {
            switch (t6) {
                .Eof => {},
                * => unreachable,
            }
        } else {
            unreachable;
        }
    });

    test("2. HTML5 Tokenizer: Named and numeric character entity references", () => {
        const html = "<span>&amp; &lt; &gt; &#65; &#x42; &copy;</span>";
        let tok = Tokenizer.init(html);

        // Start tag <span>
        * = tok.next();

        // Check decoded entities: &amp; -> '&', ' ', &lt; -> '<', ' ', &gt; -> '>', ' ', &#65; -> 'A', ' ', &#x42; -> 'B', ' ', &copy; -> U+00A9
        let acc_buf: [64]u8 = [0; 64];
        let acc_len: usize = 0;

        while (t := tok.next()) {
            switch (t) {
                .Characters => {
                    @memcpy(acc_buf[acc_len..acc_len + t.Characters.len], t.Characters);
                    acc_len += t.Characters.len;
                },
                .Tag => {
                    if (t.Tag.kind == TagKind.End) break;
                },
                * => {},
            }
        }

        const decoded_text = acc_buf[0..acc_len];
        expect_eq(std.mem.startsWith(u8, decoded_text, "& < > A B"), true);
    });

    test("3. HTML5 Tokenizer: RawText (<style>) & Comments", () => {
        const html = "<!-- my comment --><style>h1 { color: red; } /* </script> should not close style */</style>";
        let tok = Tokenizer.init(html);

        // 1. Comment
        if (t1 := tok.next()) {
            switch (t1) {
                .Comment => {
                    expect_eq(std.mem.eql(u8, t1.Comment, " my comment "), true);
                },
                * => unreachable,
            }
        }

        // 2. <style>
        if (t2 := tok.next()) {
            switch (t2) {
                .Tag => {
                    expect_eq(t2.Tag.tag == HtmlTag.Style, true);
                    tok.setState(.RawText);
                },
                * => unreachable,
            }
        }

        // 3. Raw text content
        if (t3 := tok.next()) {
            switch (t3) {
                .Characters => {
                    expect_eq(std.mem.indexOf(u8, t3.Characters, "color: red;") != null, true);
                    expect_eq(std.mem.indexOf(u8, t3.Characters, "</script>") != null, true);
                },
                * => unreachable,
            }
        }

        // 4. </style>
        if (t4 := tok.next()) {
            switch (t4) {
                .Tag => {
                    expect_eq(t4.Tag.kind == TagKind.End, true);
                    expect_eq(t4.Tag.tag == HtmlTag.Style, true);
                },
                * => unreachable,
            }
        }
    });
}
