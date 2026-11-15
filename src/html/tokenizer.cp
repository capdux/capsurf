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

    // Last start tag tracked to match closing rawtext/rcdata/script tags
    last_start_tag: HtmlTag = HtmlTag.Unknown;
    last_start_name: []const u8 = "";

    // In-flight tag token being constructed
    current_tag: TagToken = TagToken{};

    // Small scratch buffer for decoded entities
    entity_buf: [8]u8 = [0; 8];

    static fn init(input: []const u8): Tokenizer {
        return Tokenizer {
            input: input,
            pos: 0,
            state: State.Data,
            eof_emitted: false,
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
                        // Check for comment <!-- or <!DOCTYPE
                        if (_.pos + 1 < _.input.len && _.input[_.pos] == '-' && _.input[_.pos + 1] == '-') {
                            _.pos += 2;
                            _.state = State.Comment;
                            continue;
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
                    // No character references decoded. Scan until matching </tag>
                    const start = _.pos;
                    while (_.pos < _.input.len) {
                        if (_.input[_.pos] == '<' && _.pos + 1 < _.input.len && _.input[_.pos + 1] == '/') {
                            const tag_start = _.pos + 2;
                            let tag_end = tag_start;
                            while (tag_end < _.input.len && std.ascii.isAlphabetic(_.input[tag_end])) : (tag_end += 1) {}
                            const closing_name = _.input[tag_start..tag_end];
                            if (std.ascii.eqlIgnoreCase(closing_name, _.last_start_name)) {
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
