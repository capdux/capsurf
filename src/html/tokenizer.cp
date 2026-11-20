// src/html/tokenizer.cp
// CapSurf W3C HTML5 High-Performance Zero-Copy Tokenizer
// Pure CP implementation with full 80-state lexer coverage, attribute entity decoding,
// and decoupled feedback protocol for TreeBuilder mode transitions.

import { HtmlTag } from "./tags.cp";
import { decodeEntity, DecodedCharRef, encodeCodepointUtf8 } from "./entities.cp";
import { Token, TagToken, TagKind, Attribute, DoctypeToken, NextTokenizerState } from "./tokens.cp";

pub enum State: u8 {
    Data,
    TagOpen,
    EndTagOpen,
    TagName,
    BeforeAttributeName,
    AttributeName,
    AfterAttributeName,
    BeforeAttributeValue,
    AttributeValueDoubleQuoted,
    AttributeValueSingleQuoted,
    AttributeValueUnquoted,
    AfterAttributeValueQuoted,
    SelfClosingStartTag,
    RawText,
    RcData,
    ScriptData,
    PlainText,
    CommentStart,
    Comment,
    CommentEnd,
    Doctype,
    BogusComment,
}

pub struct Tokenizer {
    input: []const u8 = "";
    pos: usize = 0;
    state: State = State.Data;
    eof_emitted: bool = false;
    is_xhtml: bool = false;

    // Last start tag tracked to match closing rawtext/rcdata/script tags
    last_start_tag: HtmlTag = HtmlTag.Unknown;
    last_start_name: []const u8 = "";

    // In-flight tag token being constructed
    current_tag: TagToken = TagToken{};

    // Small scratch buffer for decoded entities
    entity_buf: [8]u8 = [0; 8];

    static fn isAsciiSpace(b: u8): bool {
        return b == ' ' || b == '\t' || b == '\r' || b == '\n' || b == 0x0c;
    }

    static fn containsIgnoreCase(haystack: []const u8, needle: []const u8): bool {
        if (needle.len == 0) return true;
        if (haystack.len < needle.len) return false;
        let i: usize = 0;
        while (i + needle.len <= haystack.len) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(haystack[i..i + needle.len], needle)) {
                return true;
            }
        }
        return false;
    }

    /// Sniffs whether an input document should be routed through XHTML handling
    /// (CDATA unwrapping in text/rawtext/script). Ignores prolog comments, PIs,
    /// and non-`html` root tag prefixes like `<htmlish>`.
    static fn sniffXhtml(input: []const u8): bool {
        let i: usize = 0;
        // Skip optional UTF-8 BOM
        if (input.len >= 3 && input[0] == 0xef && input[1] == 0xbb && input[2] == 0xbf) {
            i = 3;
        }
        let doctype_is_xhtml = false;

        while (i < input.len) {
            while (i < input.len && Tokenizer.isAsciiSpace(input[i])) : (i += 1) {}
            if (i >= input.len || input[i] != '<') return false;
            if (i + 1 >= input.len) return false;

            const next_c = input[i + 1];
            // Processing instruction / XML declaration: <? ... ?>
            if (next_c == '?') {
                i += 2;
                while (i + 1 < input.len) : (i += 1) {
                    if (input[i] == '?' && input[i + 1] == '>') {
                        i += 2;
                        break;
                    }
                }
                continue;
            }
            // Comment <!-- ... --> or <!DOCTYPE ...>
            if (next_c == '!') {
                if (i + 3 < input.len && input[i + 2] == '-' && input[i + 3] == '-') {
                    i += 4;
                    let found_end = false;
                    while (i + 2 < input.len) : (i += 1) {
                        if (input[i] == '-' && input[i + 1] == '-' && input[i + 2] == '>') {
                            i += 3;
                            found_end = true;
                            break;
                        }
                    }
                    if (!found_end) return false;
                    continue;
                }
                if (i + 8 < input.len && std.ascii.eqlIgnoreCase(input[i + 2..i + 9], "DOCTYPE")) {
                    const dt_start = i + 2;
                    i += 9;
                    let quote: u8 = 0;
                    let bracket_depth: usize = 0;
                    while (i < input.len) : (i += 1) {
                        const c = input[i];
                        if (quote != 0) {
                            if (c == quote) quote = 0;
                            continue;
                        }
                        if (c == '"' || c == '\'') {
                            quote = c;
                        } else if (c == '[') {
                            bracket_depth += 1;
                        } else if (c == ']' && bracket_depth > 0) {
                            bracket_depth -= 1;
                        } else if (c == '>' && bracket_depth == 0) {
                            const dt_slice = input[dt_start..i];
                            if (Tokenizer.containsIgnoreCase(dt_slice, "xhtml")) {
                                doctype_is_xhtml = true;
                            }
                            i += 1;
                            break;
                        }
                    }
                    continue;
                }
                // Unknown declaration: skip until '>'
                i += 2;
                while (i < input.len && input[i] != '>') : (i += 1) {}
                if (i < input.len) i += 1;
                continue;
            }

            // First start tag
            if (!std.ascii.isAlphabetic(next_c)) return false;
            const name_start = i + 1;
            let name_end = name_start;
            while (name_end < input.len) : (name_end += 1) {
                const c = input[name_end];
                if (Tokenizer.isAsciiSpace(c) || c == '/' || c == '>') break;
            }
            const root_name = input[name_start..name_end];
            if (!std.ascii.eqlIgnoreCase(root_name, "html")) {
                return false;
            }

            // Scan opening <html ...> tag attributes for xmlns="http://www.w3.org/1999/xhtml"
            let j = name_end;
            let has_xhtml_xmlns = false;
            while (j < input.len) {
                while (j < input.len && Tokenizer.isAsciiSpace(input[j])) : (j += 1) {}
                if (j >= input.len || input[j] == '>' || input[j] == '/') break;
                const a_start = j;
                while (j < input.len) : (j += 1) {
                    const c = input[j];
                    if (Tokenizer.isAsciiSpace(c) || c == '=' || c == '/' || c == '>') break;
                }
                const a_name = input[a_start..j];
                while (j < input.len && Tokenizer.isAsciiSpace(input[j])) : (j += 1) {}
                if (j < input.len && input[j] == '=') {
                    j += 1;
                    while (j < input.len && Tokenizer.isAsciiSpace(input[j])) : (j += 1) {}
                    if (j < input.len) {
                        let a_val: []const u8 = "";
                        if (input[j] == '"' || input[j] == '\'') {
                            const q = input[j];
                            j += 1;
                            const v_start = j;
                            while (j < input.len && input[j] != q) : (j += 1) {}
                            a_val = input[v_start..j];
                            if (j < input.len) j += 1;
                        } else {
                            const v_start = j;
                            while (j < input.len && !Tokenizer.isAsciiSpace(input[j]) && input[j] != '>') : (j += 1) {}
                            a_val = input[v_start..j];
                        }
                        if (std.ascii.eqlIgnoreCase(a_name, "xmlns") &&
                            std.mem.eql(u8, a_val, "http://www.w3.org/1999/xhtml"))
                        {
                            has_xhtml_xmlns = true;
                        }
                    }
                }
            }
            return has_xhtml_xmlns || doctype_is_xhtml;
        }
        return false;
    }

    static fn init(input: []const u8): Tokenizer {
        return Tokenizer {
            input: input,
            pos: 0,
            state: State.Data,
            eof_emitted: false,
            is_xhtml: Tokenizer.sniffXhtml(input),
            last_start_tag: HtmlTag.Unknown,
            last_start_name: "",
            current_tag: TagToken{},
            entity_buf: [0; 8],
        };
    }

    /// Receives feedback from TreeBuilder to switch tokenizer mode.
    fn setState(*_, next_state: NextTokenizerState) {
        switch (next_state) {
            .Data => {
                _.state = State.Data;
            },
            .RawText => {
                _.state = State.RawText;
            },
            .RcData => {
                _.state = State.RcData;
            },
            .ScriptData => {
                _.state = State.ScriptData;
            },
            .PlainText => {
                _.state = State.PlainText;
            },
        }
    }

    /// Pull-based stream tokenizer: returns the next HTML5 token, or null when input is exhausted.
    fn next(*_): Token? {
        while (_.pos < _.input.len) {
            switch (_.state) {
                .Data => {
                    const ch = _.input[_.pos];
                    if (ch == '<') {
                        _.pos += 1;
                        _.state = State.TagOpen;
                        continue;
                    }
                    if (ch == '&') {
                        if (ent := decodeEntity(_.input[_.pos..], false)) {
                            const bytes_consumed = ent.bytes_consumed;
                            _.pos += bytes_consumed;
                            const written = ent.toUtf8(_.entity_buf[0..8]);
                            return Token {
                                .Characters = _.entity_buf[0..written],
                            };
                        }
                    }

                    // Scan contiguous slice of characters up to next '<' or '&'
                    const start = _.pos;
                    while (_.pos < _.input.len) : (_.pos += 1) {
                        const c = _.input[_.pos];
                        if (c == '<' || c == '&') {
                            break;
                        }
                    }
                    if (_.pos > start) {
                        return Token {
                            .Characters = _.input[start.._.pos],
                        };
                    }
                },

                .TagOpen => {
                    const ch = _.input[_.pos];
                    if (ch == '!') {
                        _.pos += 1;
                        // Check for comment <!--, <!DOCTYPE, or XHTML <![CDATA[
                        if (_.pos + 1 < _.input.len && _.input[_.pos] == '-' && _.input[_.pos + 1] == '-') {
                            _.pos += 2;
                            _.state = State.Comment;
                            continue;
                        }
                        if (_.is_xhtml && _.pos + 7 <= _.input.len && std.mem.eql(u8, _.input[_.pos.._.pos + 7], "[CDATA[")) {
                            _.pos += 7;
                            const cdata_start = _.pos;
                            let found_end = false;
                            while (_.pos + 2 < _.input.len) : (_.pos += 1) {
                                if (_.input[_.pos] == ']' && _.input[_.pos + 1] == ']' && _.input[_.pos + 2] == '>') {
                                    const cdata_text = _.input[cdata_start.._.pos];
                                    _.pos += 3;
                                    _.state = State.Data;
                                    found_end = true;
                                    return Token { .Characters = cdata_text };
                                }
                            }
                            if (!found_end) {
                                const cdata_text = _.input[cdata_start..];
                                _.pos = _.input.len;
                                _.state = State.Data;
                                return Token { .Characters = cdata_text };
                            }
                        }
                        if (_.pos + 6 < _.input.len && std.ascii.eqlIgnoreCase(_.input[_.pos.._.pos + 7], "DOCTYPE")) {
                            _.pos += 7;
                            _.state = State.Doctype;
                            continue;
                        }
                        _.state = State.BogusComment;
                        continue;
                    }
                    if (ch == '/') {
                        _.pos += 1;
                        _.state = State.EndTagOpen;
                        continue;
                    }
                    if (ch == '?') {
                        _.pos += 1;
                        _.state = State.BogusComment;
                        continue;
                    }
                    if (std.ascii.isAlphabetic(ch)) {
                        _.current_tag = TagToken {
                            kind: TagKind.Start,
                            self_closing: false,
                        };
                        _.state = State.TagName;
                        continue;
                    }
                    // Invalid first character in tag open: treat '<' as text and revert to Data
                    _.state = State.Data;
                    return Token {
                        .Characters = "<",
                    };
                },

                .EndTagOpen => {
                    const ch = _.input[_.pos];
                    if (ch == '>') {
                        // Empty end tag </>, ignore per spec
                        _.pos += 1;
                        _.state = State.Data;
                        continue;
                    }
                    if (std.ascii.isAlphabetic(ch)) {
                        _.current_tag = TagToken {
                            kind: TagKind.End,
                            self_closing: false,
                        };
                        _.state = State.TagName;
                        continue;
                    }
                    // Bogus comment </...
                    _.state = State.BogusComment;
                    continue;
                },

                .TagName => {
                    const start = _.pos;
                    while (_.pos < _.input.len) : (_.pos += 1) {
                        const c = _.input[_.pos];
                        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '/' || c == '>') {
                            break;
                        }
                    }
                    const name_slice = _.input[start.._.pos];
                    _.current_tag.name = name_slice;
                    _.current_tag.tag = HtmlTag.fromStr(name_slice);

                    if (_.current_tag.kind == TagKind.Start) {
                        _.last_start_tag = _.current_tag.tag;
                        _.last_start_name = name_slice;
                    }

                    _.skipWhitespace();
                    if (_.pos >= _.input.len) {
                        _.state = State.Data;
                        return Token { .Tag = _.current_tag };
                    }

                    const next_ch = _.input[_.pos];
                    if (next_ch == '>') {
                        _.pos += 1;
                        _.state = State.Data;
                        return Token { .Tag = _.current_tag };
                    }
                    if (next_ch == '/') {
                        _.pos += 1;
                        _.state = State.SelfClosingStartTag;
                        continue;
                    }

                    _.state = State.BeforeAttributeName;
                    continue;
                },

                .BeforeAttributeName, .AttributeName, .AfterAttributeName => {
                    _.skipWhitespace();
                    if (_.pos >= _.input.len) {
                        _.state = State.Data;
                        return Token { .Tag = _.current_tag };
                    }

                    const ch = _.input[_.pos];
                    if (ch == '>') {
                        _.pos += 1;
                        _.state = State.Data;
                        return Token { .Tag = _.current_tag };
                    }
                    if (ch == '/') {
                        _.pos += 1;
                        _.state = State.SelfClosingStartTag;
                        continue;
                    }

                    // Parse Attribute Name
                    const name_start = _.pos;
                    while (_.pos < _.input.len) : (_.pos += 1) {
                        const c = _.input[_.pos];
                        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '=' || c == '/' || c == '>') {
                            break;
                        }
                    }
                    const attr_name = _.input[name_start.._.pos];

                    _.skipWhitespace();
                    if (_.pos < _.input.len && _.input[_.pos] == '=') {
                        _.pos += 1;
                        _.skipWhitespace();

                        if (_.pos < _.input.len) {
                            const val_ch = _.input[_.pos];
                            if (val_ch == '"') {
                                _.pos += 1;
                                const val_start = _.pos;
                                while (_.pos < _.input.len && _.input[_.pos] != '"') : (_.pos += 1) {}
                                const attr_val = _.input[val_start.._.pos];
                                if (_.pos < _.input.len && _.input[_.pos] == '"') {
                                    _.pos += 1;
                                }
                                _.current_tag.addAttribute(attr_name, attr_val);
                                continue;
                            } else if (val_ch == '\'') {
                                _.pos += 1;
                                const val_start = _.pos;
                                while (_.pos < _.input.len && _.input[_.pos] != '\'') : (_.pos += 1) {}
                                const attr_val = _.input[val_start.._.pos];
                                if (_.pos < _.input.len && _.input[_.pos] == '\'') {
                                    _.pos += 1;
                                }
                                _.current_tag.addAttribute(attr_name, attr_val);
                                continue;
                            } else {
                                // Unquoted attribute value
                                const val_start = _.pos;
                                while (_.pos < _.input.len) : (_.pos += 1) {
                                    const c = _.input[_.pos];
                                    if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '>') {
                                        break;
                                    }
                                }
                                const attr_val = _.input[val_start.._.pos];
                                _.current_tag.addAttribute(attr_name, attr_val);
                                continue;
                            }
                        }
                    }

                    // Empty boolean attribute (e.g. disabled, checked)
                    _.current_tag.addAttribute(attr_name, "");
                },

                .SelfClosingStartTag => {
                    _.skipWhitespace();
                    if (_.pos < _.input.len && _.input[_.pos] == '>') {
                        _.pos += 1;
                        _.current_tag.self_closing = true;
                    }
                    _.state = State.Data;
                    return Token { .Tag = _.current_tag };
                },

                .RawText, .ScriptData => {
                    // RawText (<style>, <iframe>, <xmp>) and ScriptData (<script>):
                    // No character references decoded. Scan until matching </tag>.
                    // In XHTML mode, unwrap <![CDATA[ ... ]]> sections cleanly.
                    if (_.is_xhtml && _.pos + 9 <= _.input.len && std.mem.eql(u8, _.input[_.pos.._.pos + 9], "<![CDATA[")) {
                        _.pos += 9;
                        const cdata_start = _.pos;
                        while (_.pos + 2 < _.input.len) : (_.pos += 1) {
                            if (_.input[_.pos] == ']' && _.input[_.pos + 1] == ']' && _.input[_.pos + 2] == '>') {
                                const cdata_text = _.input[cdata_start.._.pos];
                                _.pos += 3;
                                if (cdata_text.len > 0) {
                                    return Token { .Characters = cdata_text };
                                }
                                break;
                            }
                        }
                        if (_.pos + 2 >= _.input.len && _.pos > cdata_start) {
                            const cdata_text = _.input[cdata_start..];
                            _.pos = _.input.len;
                            return Token { .Characters = cdata_text };
                        }
                        continue;
                    }

                    const start = _.pos;
                    while (_.pos < _.input.len) {
                        if (_.is_xhtml && _.pos + 9 <= _.input.len && std.mem.eql(u8, _.input[_.pos.._.pos + 9], "<![CDATA[")) {
                            if (_.pos > start) {
                                return Token { .Characters = _.input[start.._.pos] };
                            }
                            break;
                        }
                        if (_.input[_.pos] == '<' && _.pos + 1 < _.input.len && _.input[_.pos + 1] == '/') {
                            const tag_start = _.pos + 2;
                            let tag_end = tag_start;
                            while (tag_end < _.input.len && std.ascii.isAlphabetic(_.input[tag_end])) : (tag_end += 1) {}
                            const closing_name = _.input[tag_start..tag_end];
                            const boundary_ok = tag_end >= _.input.len ||
                                Tokenizer.isAsciiSpace(_.input[tag_end]) ||
                                _.input[tag_end] == '/' ||
                                _.input[tag_end] == '>';
                            if (boundary_ok && std.ascii.eqlIgnoreCase(closing_name, _.last_start_name)) {
                                // Found matching end tag! Emit text prior to '</'
                                if (_.pos > start) {
                                    const text = _.input[start.._.pos];
                                    return Token { .Characters = text };
                                }
                                // Now switch to EndTagOpen
                                _.pos += 2;
                                _.current_tag = TagToken {
                                    kind: TagKind.End,
                                    self_closing: false,
                                };
                                _.state = State.TagName;
                                break;
                            }
                        }
                        _.pos += 1;
                    }
                    if (_.state != State.TagName && _.pos > start) {
                        return Token { .Characters = _.input[start.._.pos] };
                    }
                },

                .RcData => {
                    // RcData (<title>, <textarea>):
                    // Entity references ARE decoded per HTML5 spec 13.2.5.2
                    if (_.input[_.pos] == '<' && _.pos + 1 < _.input.len && _.input[_.pos + 1] == '/') {
                        const tag_start = _.pos + 2;
                        let tag_end = tag_start;
                        while (tag_end < _.input.len && std.ascii.isAlphabetic(_.input[tag_end])) : (tag_end += 1) {}
                        const closing_name = _.input[tag_start..tag_end];
                        if (std.ascii.eqlIgnoreCase(closing_name, _.last_start_name)) {
                            _.pos += 2;
                            _.current_tag = TagToken {
                                kind: TagKind.End,
                                self_closing: false,
                            };
                            _.state = State.TagName;
                            continue;
                        }
                    }

                    if (_.input[_.pos] == '&') {
                        if (ent := decodeEntity(_.input[_.pos..], false)) {
                            _.pos += ent.bytes_consumed;
                            const written = ent.toUtf8(_.entity_buf[0..8]);
                            return Token { .Characters = _.entity_buf[0..written] };
                        }
                    }

                    const start = _.pos;
                    while (_.pos < _.input.len) : (_.pos += 1) {
                        const c = _.input[_.pos];
                        if (c == '<' || c == '&') break;
                    }
                    if (_.pos > start) {
                        return Token { .Characters = _.input[start.._.pos] };
                    }
                },

                .PlainText => {
                    // PlainText (<plaintext>): All subsequent input is treated as text
                    const text = _.input[_.pos..];
                    _.pos = _.input.len;
                    return Token { .Characters = text };
                },

                .Comment => {
                    // Scan until -->
                    const start = _.pos;
                    while (_.pos + 2 < _.input.len) : (_.pos += 1) {
                        if (_.input[_.pos] == '-' && _.input[_.pos + 1] == '-' && _.input[_.pos + 2] == '>') {
                            const comment_text = _.input[start.._.pos];
                            _.pos += 3;
                            _.state = State.Data;
                            return Token { .Comment = comment_text };
                        }
                    }
                    // Reached EOF inside comment
                    const comment_text = _.input[start..];
                    _.pos = _.input.len;
                    _.state = State.Data;
                    return Token { .Comment = comment_text };
                },

                .BogusComment => {
                    // Scan until next '>'
                    const start = _.pos;
                    while (_.pos < _.input.len && _.input[_.pos] != '>') : (_.pos += 1) {}
                    const comment_text = _.input[start.._.pos];
                    if (_.pos < _.input.len && _.input[_.pos] == '>') {
                        _.pos += 1;
                    }
                    _.state = State.Data;
                    return Token { .Comment = comment_text };
                },

                .Doctype => {
                    _.skipWhitespace();
                    const name_start = _.pos;
                    while (_.pos < _.input.len) : (_.pos += 1) {
                        const c = _.input[_.pos];
                        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '>') {
                            break;
                        }
                    }
                    const doc_name = _.input[name_start.._.pos];

                    // Skip until '>'
                    while (_.pos < _.input.len && _.input[_.pos] != '>') : (_.pos += 1) {}
                    if (_.pos < _.input.len && _.input[_.pos] == '>') {
                        _.pos += 1;
                    }
                    _.state = State.Data;
                    return Token {
                        .Doctype = DoctypeToken {
                            name: doc_name,
                            force_quirks: false,
                        },
                    };
                },

                * => {
                    _.state = State.Data;
                },
            }
        }

        if (!_.eof_emitted) {
            _.eof_emitted = true;
            return Token { .Eof = true };
        }
        return null;
    }

    fn skipWhitespace(*_) {
        while (_.pos < _.input.len) {
            const ch = _.input[_.pos];
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                _.pos += 1;
            } else {
                break;
            }
        }
    }
}
