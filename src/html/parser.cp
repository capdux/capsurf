// src/html/parser.cp
// CapSurf W3C HTML5 High-Performance Parser Driver
// Connects streaming Tokenizer and TreeBuilder via the decoupled feedback loop.

import { Document } from "../dom/dom.cp";
import { Tokenizer } from "./tokenizer.cp";
import { TreeBuilder } from "./tree_builder.cp";
import { Token, NextTokenizerState } from "./tokens.cp";

pub struct HtmlParser {
    static fn parse(doc: *Document, html: []const u8)! {
        let tok = Tokenizer.init(html);
        let tb = TreeBuilder.init(doc);

        while (t := tok.next()) {
            const next_state = tb.processToken(&t);
            tok.setState(next_state);
        }
    }
}
