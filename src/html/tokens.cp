// src/html/tokens.cp
// CapSurf HTML5 Token Data Structures
// Algebraic data types for zero-copy streaming lexer tokens and tokenizer feedback directives.

import { HtmlTag } from "./tags.cp";

pub struct Attribute {
    name: []const u8 = "";
    value: []const u8 = "";
}

pub enum TagKind: u8 {
    Start = 0,
    End = 1,
}

pub struct TagToken {
    kind: TagKind = TagKind.Start;
    tag: HtmlTag = HtmlTag.Unknown;
    name: []const u8 = "";
    attrs: [24]Attribute = [Attribute{}; 24];
    attr_count: usize = 0;
    self_closing: bool = false;

    fn getAttribute(attr_name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.attr_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.attrs[i].name, attr_name)) {
                return _.attrs[i].value;
            }
        }
        return null;
    }

    fn hasAttribute(attr_name: []const u8): bool {
        return _.getAttribute(attr_name) != null;
    }

    fn addAttribute(*_, name: []const u8, value: []const u8) {
        if (_.attr_count >= 24) return;
        // HTML5 13.2.5.34: When there are multiple attributes with the same name,
        // the first one must be kept, subsequent ones must be ignored.
        let i: usize = 0;
        while (i < _.attr_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.attrs[i].name, name)) {
                return;
            }
        }
        _.attrs[_.attr_count] = Attribute {
            name: name,
            value: value,
        };
        _.attr_count += 1;
    }
}

pub struct DoctypeToken {
    name: []const u8 = "";
    public_id: ([]const u8)? = null;
    system_id: ([]const u8)? = null;
    force_quirks: bool = false;
}

pub union(enum) Token {
    Doctype: DoctypeToken,
    Tag: TagToken,
    Comment: []const u8,
    Characters: []const u8,
    NullChar: bool,
    Eof: bool,
}

/// Tokenizer state transition directives sent back from TreeBuilder.
pub enum NextTokenizerState: u8 {
    Data = 0,
    RawText = 1,      // for <style>, <xmp>, <iframe>
    RcData = 2,       // for <title>, <textarea>
    ScriptData = 3,   // for <script>
    PlainText = 4,    // for <plaintext>
}
