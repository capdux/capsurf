// src/css/ua_stylesheet.cp
// CapSurf W3C HTML5 Standard User Agent (UA) Stylesheet
// Provides default Layer 0 styling rules (display, margins, paddings, typography)
// for all standard HTML5 tags and [hidden] attribute, per W3C HTML5 Rendering Spec § 15.

import { HtmlTag } from "../html/tags.cp";
import { NodeData } from "../dom/dom.cp";
import {
    Display,
    ComputedStyle,
    LengthPercentageOrAuto,
    Visibility
} from "./cascade.cp";

pub struct UaStylesheet {
    /// Applies W3C HTML5 default styling rules for `node` onto `style` as Layer 0 defaults.
    static fn applyUaDefaults(
        node: *const NodeData,
        style: *ComputedStyle,
        parent_style: (*const ComputedStyle)?
    ) {
        // 1. [hidden] attribute takes absolute precedence in UA origin: display: none
        if (node.hasAttribute("hidden")) {
            style.display = Display.none();
            return;
        }

        const tag = HtmlTag.fromStr(node.tag_name);

        // 2. Tag-specific default display values
        switch (tag) {
            // Invisible metadata, scripts, styles and templates
            .Head, .Style, .Script, .Meta, .Link, .Title, .Template,
            .Param, .Track, .Source, .Noscript, .Datalist => {
                style.display = Display.none();
            },

            // Dialog elements default to none unless [open] attribute is present
            .Dialog => {
                if (node.hasAttribute("open")) {
                    style.display = Display.block();
                } else {
                    style.display = Display.none();
                }
            },

            // Inline text-level elements
            .A, .Abbr, .B, .Bdi, .Bdo, .Cite, .Code, .Data, .Dfn, .Em,
            .I, .Kbd, .Mark, .Q, .S, .Samp, .Small, .Span, .Strong,
            .Sub, .Sup, .Time, .U, .Var, .Wbr, .Label => {
                style.display = Display.inlineFlow();
            },

            // Form controls, replaced elements and media default to inline-block
            .Button, .Input, .Select, .Textarea, .Img, .Video, .Audio,
            .Canvas, .Iframe, .Embed, .Object, .Progress, .Meter => {
                style.display = Display.inlineBlock();
            },

            // Table elements
            .Table => {
                style.display = Display.block(); // simplified table container
            },

            // Block-level container elements
            .Html, .Body, .Div, .P, .H1, .H2, .H3, .H4, .H5, .H6,
            .Ul, .Ol, .Li, .Article, .Aside, .Nav, .Section, .Header,
            .Footer, .Main, .Address, .Blockquote, .Figure, .Figcaption,
            .Fieldset, .Form, .Pre, .Details, .Summary, .Hr => {
                style.display = Display.block();
            },

            // Fallback for custom elements or unknown tags
            * => {
                style.display = Display.block();
            },
        }

        // 3. Default typography and box margins/paddings for semantic tags
        switch (tag) {
            .Body => {
                style.margin_top = LengthPercentageOrAuto.px(8.0);
                style.margin_right = LengthPercentageOrAuto.px(8.0);
                style.margin_bottom = LengthPercentageOrAuto.px(8.0);
                style.margin_left = LengthPercentageOrAuto.px(8.0);
            },
            .H1 => {
                style.font_size_px = 32.0; // 2em
                style.margin_top = LengthPercentageOrAuto.px(21.44); // 0.67em
                style.margin_bottom = LengthPercentageOrAuto.px(21.44);
            },
            .H2 => {
                style.font_size_px = 24.0; // 1.5em
                style.margin_top = LengthPercentageOrAuto.px(19.92); // 0.83em
                style.margin_bottom = LengthPercentageOrAuto.px(19.92);
            },
            .H3 => {
                style.font_size_px = 18.72; // 1.17em
                style.margin_top = LengthPercentageOrAuto.px(18.72); // 1em
                style.margin_bottom = LengthPercentageOrAuto.px(18.72);
            },
            .H4 => {
                style.font_size_px = 16.0; // 1em
                style.margin_top = LengthPercentageOrAuto.px(21.28); // 1.33em
                style.margin_bottom = LengthPercentageOrAuto.px(21.28);
            },
            .H5 => {
                style.font_size_px = 13.28; // 0.83em
                style.margin_top = LengthPercentageOrAuto.px(22.17); // 1.67em
                style.margin_bottom = LengthPercentageOrAuto.px(22.17);
            },
            .H6 => {
                style.font_size_px = 10.72; // 0.67em
                style.margin_top = LengthPercentageOrAuto.px(24.97); // 2.33em
                style.margin_bottom = LengthPercentageOrAuto.px(24.97);
            },
            .P => {
                style.margin_top = LengthPercentageOrAuto.px(16.0); // 1em
                style.margin_bottom = LengthPercentageOrAuto.px(16.0);
            },
            .Ul, .Ol => {
                style.margin_top = LengthPercentageOrAuto.px(16.0);
                style.margin_bottom = LengthPercentageOrAuto.px(16.0);
                style.padding_left = LengthPercentageOrAuto.px(40.0);
            },
            .Blockquote, .Figure => {
                style.margin_top = LengthPercentageOrAuto.px(16.0);
                style.margin_bottom = LengthPercentageOrAuto.px(16.0);
                style.margin_left = LengthPercentageOrAuto.px(40.0);
                style.margin_right = LengthPercentageOrAuto.px(40.0);
            },
            .Hr => {
                style.margin_top = LengthPercentageOrAuto.px(8.0);
                style.margin_bottom = LengthPercentageOrAuto.px(8.0);
                style.border_top_width = 1.0;
            },
            * => {},
        }

        // 4. CSS Inheritance from parent style (visibility, font_size_px, color, font_family)
        if (p := parent_style) {
            style.visibility = p.visibility;
            if (tag != .H1 && tag != .H2 && tag != .H3 && tag != .H4 && tag != .H5 && tag != .H6) {
                style.font_size_px = p.font_size_px;
            }
            style.color = p.color;
            if (p.font_family_len > 0) {
                const copy_len = @min(p.font_family_len, style.font_family_buf.len);
                @memcpy(style.font_family_buf[0..copy_len], p.font_family_buf[0..copy_len]);
                style.font_family_len = copy_len;
            }
        }
    }
}
