// src/cdp/css.cp
// CDP CSS Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/css.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, unescapeJsonString, MAX_CDP_EVENTS } from "./types.cp";
import { CdpPageSession, escapeJsonString, hashStyleBytes } from "./session.cp";
import { Document, NodeId, NULL_NODE, NodeType, NodeData } from "../dom/dom.cp";
import { StyleSheet, Cascade, ComputedStyle, Display, DisplayOutside, DisplayInside, BoxSizing, Visibility, LengthPercentageOrAuto } from "../css/cascade.cp";
import { SelectorList, Matcher } from "../css/matcher.cp";
import { CssProperty, CssRule, CssRuleKind, CssStylesheetParser, CssColor, CssUnit } from "cap:css";

pub struct DefaultProp {
    name: []const u8 = "",
    value: []const u8 = "",
}

const DEFAULT_CSS_PROPERTIES: [149]DefaultProp = [
    DefaultProp{ name: "accent-color", value: "auto" },
    DefaultProp{ name: "align-content", value: "normal" },
    DefaultProp{ name: "align-items", value: "normal" },
    DefaultProp{ name: "align-self", value: "auto" },
    DefaultProp{ name: "animation-composition", value: "replace" },
    DefaultProp{ name: "animation-delay", value: "0s" },
    DefaultProp{ name: "animation-direction", value: "normal" },
    DefaultProp{ name: "animation-duration", value: "0s" },
    DefaultProp{ name: "animation-fill-mode", value: "none" },
    DefaultProp{ name: "animation-iteration-count", value: "1" },
    DefaultProp{ name: "animation-name", value: "none" },
    DefaultProp{ name: "animation-play-state", value: "running" },
    DefaultProp{ name: "animation-timing-function", value: "ease" },
    DefaultProp{ name: "appearance", value: "none" },
    DefaultProp{ name: "backdrop-filter", value: "none" },
    DefaultProp{ name: "backface-visibility", value: "visible" },
    DefaultProp{ name: "background-attachment", value: "scroll" },
    DefaultProp{ name: "background-blend-mode", value: "normal" },
    DefaultProp{ name: "background-clip", value: "border-box" },
    DefaultProp{ name: "background-color", value: "rgba(0, 0, 0, 0)" },
    DefaultProp{ name: "background-image", value: "none" },
    DefaultProp{ name: "background-origin", value: "padding-box" },
    DefaultProp{ name: "background-position", value: "0% 0%" },
    DefaultProp{ name: "background-repeat", value: "repeat" },
    DefaultProp{ name: "background-size", value: "auto" },
    DefaultProp{ name: "border-bottom-color", value: "rgb(0, 0, 0)" },
    DefaultProp{ name: "border-bottom-left-radius", value: "0px" },
    DefaultProp{ name: "border-bottom-right-radius", value: "0px" },
    DefaultProp{ name: "border-bottom-style", value: "none" },
    DefaultProp{ name: "border-bottom-width", value: "0px" },
    DefaultProp{ name: "border-collapse", value: "separate" },
    DefaultProp{ name: "border-left-color", value: "rgb(0, 0, 0)" },
    DefaultProp{ name: "border-left-style", value: "none" },
    DefaultProp{ name: "border-left-width", value: "0px" },
    DefaultProp{ name: "border-right-color", value: "rgb(0, 0, 0)" },
    DefaultProp{ name: "border-right-style", value: "none" },
    DefaultProp{ name: "border-right-width", value: "0px" },
    DefaultProp{ name: "border-top-color", value: "rgb(0, 0, 0)" },
    DefaultProp{ name: "border-top-left-radius", value: "0px" },
    DefaultProp{ name: "border-top-right-radius", value: "0px" },
    DefaultProp{ name: "border-top-style", value: "none" },
    DefaultProp{ name: "border-top-width", value: "0px" },
    DefaultProp{ name: "bottom", value: "auto" },
    DefaultProp{ name: "box-shadow", value: "none" },
    DefaultProp{ name: "box-sizing", value: "content-box" },
    DefaultProp{ name: "caption-side", value: "top" },
    DefaultProp{ name: "caret-color", value: "rgb(0, 0, 0)" },
    DefaultProp{ name: "clear", value: "none" },
    DefaultProp{ name: "clip", value: "auto" },
    DefaultProp{ name: "color", value: "rgb(0, 0, 0)" },
    DefaultProp{ name: "color-scheme", value: "normal" },
    DefaultProp{ name: "column-count", value: "auto" },
    DefaultProp{ name: "column-gap", value: "normal" },
    DefaultProp{ name: "column-rule-color", value: "rgb(0, 0, 0)" },
    DefaultProp{ name: "column-rule-style", value: "none" },
    DefaultProp{ name: "column-rule-width", value: "0px" },
    DefaultProp{ name: "column-width", value: "auto" },
    DefaultProp{ name: "contain", value: "none" },
    DefaultProp{ name: "content", value: "normal" },
    DefaultProp{ name: "cursor", value: "auto" },
    DefaultProp{ name: "direction", value: "ltr" },
    DefaultProp{ name: "display", value: "block" },
    DefaultProp{ name: "empty-cells", value: "show" },
    DefaultProp{ name: "filter", value: "none" },
    DefaultProp{ name: "flex-basis", value: "auto" },
    DefaultProp{ name: "flex-direction", value: "row" },
    DefaultProp{ name: "flex-grow", value: "0" },
    DefaultProp{ name: "flex-shrink", value: "1" },
    DefaultProp{ name: "flex-wrap", value: "nowrap" },
    DefaultProp{ name: "float", value: "none" },
    DefaultProp{ name: "font-family", value: "serif" },
    DefaultProp{ name: "font-kerning", value: "auto" },
    DefaultProp{ name: "font-optical-sizing", value: "auto" },
    DefaultProp{ name: "font-size", value: "16px" },
    DefaultProp{ name: "font-stretch", value: "100%" },
    DefaultProp{ name: "font-style", value: "normal" },
    DefaultProp{ name: "font-variant", value: "normal" },
    DefaultProp{ name: "font-weight", value: "400" },
    DefaultProp{ name: "gap", value: "normal" },
    DefaultProp{ name: "height", value: "auto" },
    DefaultProp{ name: "image-orientation", value: "from-image" },
    DefaultProp{ name: "image-rendering", value: "auto" },
    DefaultProp{ name: "isolation", value: "auto" },
    DefaultProp{ name: "justify-content", value: "normal" },
    DefaultProp{ name: "justify-items", value: "legacy" },
    DefaultProp{ name: "justify-self", value: "auto" },
    DefaultProp{ name: "left", value: "auto" },
    DefaultProp{ name: "letter-spacing", value: "normal" },
    DefaultProp{ name: "line-height", value: "normal" },
    DefaultProp{ name: "list-style-image", value: "none" },
    DefaultProp{ name: "list-style-position", value: "outside" },
    DefaultProp{ name: "list-style-type", value: "disc" },
    DefaultProp{ name: "margin-bottom", value: "0px" },
    DefaultProp{ name: "margin-left", value: "0px" },
    DefaultProp{ name: "margin-right", value: "0px" },
    DefaultProp{ name: "margin-top", value: "0px" },
    DefaultProp{ name: "max-height", value: "none" },
    DefaultProp{ name: "max-width", value: "none" },
    DefaultProp{ name: "min-height", value: "0px" },
    DefaultProp{ name: "min-width", value: "0px" },
    DefaultProp{ name: "mix-blend-mode", value: "normal" },
    DefaultProp{ name: "object-fit", value: "fill" },
    DefaultProp{ name: "object-position", value: "50% 50%" },
    DefaultProp{ name: "opacity", value: "1" },
    DefaultProp{ name: "order", value: "0" },
    DefaultProp{ name: "outline-color", value: "rgb(0, 0, 0)" },
    DefaultProp{ name: "outline-offset", value: "0px" },
    DefaultProp{ name: "outline-style", value: "none" },
    DefaultProp{ name: "outline-width", value: "0px" },
    DefaultProp{ name: "overflow", value: "visible" },
    DefaultProp{ name: "overflow-wrap", value: "normal" },
    DefaultProp{ name: "overflow-x", value: "visible" },
    DefaultProp{ name: "overflow-y", value: "visible" },
    DefaultProp{ name: "padding-bottom", value: "0px" },
    DefaultProp{ name: "padding-left", value: "0px" },
    DefaultProp{ name: "padding-right", value: "0px" },
    DefaultProp{ name: "padding-top", value: "0px" },
    DefaultProp{ name: "pointer-events", value: "auto" },
    DefaultProp{ name: "position", value: "static" },
    DefaultProp{ name: "resize", value: "none" },
    DefaultProp{ name: "right", value: "auto" },
    DefaultProp{ name: "row-gap", value: "normal" },
    DefaultProp{ name: "table-layout", value: "auto" },
    DefaultProp{ name: "tab-size", value: "8" },
    DefaultProp{ name: "text-align", value: "start" },
    DefaultProp{ name: "text-align-last", value: "auto" },
    DefaultProp{ name: "text-decoration", value: "none solid rgb(0, 0, 0)" },
    DefaultProp{ name: "text-decoration-color", value: "rgb(0, 0, 0)" },
    DefaultProp{ name: "text-decoration-line", value: "none" },
    DefaultProp{ name: "text-decoration-style", value: "solid" },
    DefaultProp{ name: "text-indent", value: "0px" },
    DefaultProp{ name: "text-overflow", value: "clip" },
    DefaultProp{ name: "text-shadow", value: "none" },
    DefaultProp{ name: "text-transform", value: "none" },
    DefaultProp{ name: "top", value: "auto" },
    DefaultProp{ name: "transform", value: "none" },
    DefaultProp{ name: "transition-delay", value: "0s" },
    DefaultProp{ name: "transition-duration", value: "0s" },
    DefaultProp{ name: "transition-property", value: "all" },
    DefaultProp{ name: "transition-timing-function", value: "ease" },
    DefaultProp{ name: "user-select", value: "auto" },
    DefaultProp{ name: "vertical-align", value: "baseline" },
    DefaultProp{ name: "visibility", value: "visible" },
    DefaultProp{ name: "white-space", value: "normal" },
    DefaultProp{ name: "width", value: "auto" },
    DefaultProp{ name: "word-break", value: "normal" },
    DefaultProp{ name: "word-spacing", value: "0px" },
    DefaultProp{ name: "writing-mode", value: "horizontal-tb" },
    DefaultProp{ name: "z-index", value: "auto" },
];

fn formatCssColor(color: CssColor, out: []u8): []const u8 {
    if (color.a == 255) {
        return std.fmt.bufPrint(out, "rgb({d}, {d}, {d})", .{ color.r, color.g, color.b }) catch "rgb(0, 0, 0)";
    } else if (color.a == 0) {
        return "rgba(0, 0, 0, 0)";
    } else {
        const a_f = @as(f32, @floatFromInt(color.a)) / 255.0;
        return std.fmt.bufPrint(out, "rgba({d}, {d}, {d}, {d:.2})", .{ color.r, color.g, color.b, a_f }) catch "rgba(0, 0, 0, 0)";
    }
}

fn formatLengthPx(lpa: LengthPercentageOrAuto, out: []u8): []const u8 {
    if (lpa.is_auto) return "0px";
    if (lpa.is_none) return "none";
    const v: f32 = @floatCast(lpa.length.value);
    if (v == @floor(v)) {
        return std.fmt.bufPrint(out, "{d}px", .{@as(i32, @intFromFloat(v))}) catch "0px";
    }
    return std.fmt.bufPrint(out, "{d:.1}px", .{v}) catch "0px";
}

fn formatFontSize(fs: f32, out: []u8): []const u8 {
    if (fs == @floor(fs)) {
        return std.fmt.bufPrint(out, "{d}px", .{@as(i32, @intFromFloat(fs))}) catch "16px";
    }
    return std.fmt.bufPrint(out, "{d:.1}px", .{fs}) catch "16px";
}

fn formatDisplay(d: Display): []const u8 {
    if (d.isNone()) return "none";
    if (d.isContents()) return "contents";
    if (d.isWebkitBox()) {
        if (d.pair.outside == DisplayOutside.Inline) return "-webkit-inline-box";
        return "-webkit-box";
    }
    if (d.isFlex()) {
        if (d.pair.outside == DisplayOutside.Inline) return "inline-flex";
        return "flex";
    }
    if (d.isGrid()) {
        if (d.pair.outside == DisplayOutside.Inline) return "inline-grid";
        return "grid";
    }
    if (d.pair.outside == DisplayOutside.Inline) {
        if (d.pair.inside == DisplayInside.FlowRoot) return "inline-block";
        return "inline";
    }
    if (d.pair.inside == DisplayInside.FlowRoot) return "flow-root";
    return "block";
}

fn formatBoxSizing(bs: BoxSizing): []const u8 {
    return switch (bs) {
        .BorderBox => "border-box",
        .ContentBox => "content-box",
    };
}

fn formatVisibility(vis: Visibility): []const u8 {
    return switch (vis) {
        .Visible => "visible",
        .Hidden => "hidden",
        .Collapse => "collapse",
    };
}

fn getStyleNodeText(doc: *const Document, sn: *const NodeData, out_buf: []u8): []const u8 {
    let len: usize = 0;
    if (sn.first_child != NULL_NODE) {
        let child = sn.first_child;
        while (child != NULL_NODE && @as(usize, child) < doc.nodes.items.len) : (child = doc.nodes.items[@as(usize, child)].next_sibling) {
            const scn = &doc.nodes.items[@as(usize, child)];
            if (scn.node_type == NodeType.Text && scn.text_content.len > 0) {
                const copy_len = @min(scn.text_content.len, out_buf.len - len);
                @memcpy(out_buf[len .. len + copy_len], scn.text_content[0..copy_len]);
                len += copy_len;
            }
        }
    }
    if (len == 0 && sn.text_content.len > 0) {
        const copy_len = @min(sn.text_content.len, out_buf.len);
        @memcpy(out_buf[0..copy_len], sn.text_content[0..copy_len]);
        len = copy_len;
    }
    return out_buf[0..len];
}

const UA_STYLESHEET_TEXT = "@namespace url(http://www.w3.org/1999/xhtml);\n" ++
    ":root { --ks-tty-font-px: 13.3333px !important; --ks-tty-cell-w: 8px !important; --ks-tty-cell-h: 17px !important; }\n" ++
    "html, body, div, p, span, h1, h2, h3, h4, h5, h6, ul, ol, li, table, tr, td, th { box-sizing: border-box; }\n" ++
    "html { display: block; }\n" ++
    "head { display: none; }\n" ++
    "meta, title, link, style, script { display: none; }\n" ++
    "body { display: block; margin: 8px; }\n" ++
    "div { display: block; }\n" ++
    "p { display: block; margin-top: 1em; margin-bottom: 1em; }\n" ++
    "h1 { display: block; font-size: 2em; margin-top: 0.67em; margin-bottom: 0.67em; font-weight: bold; }\n" ++
    "h2 { display: block; font-size: 1.5em; margin-top: 0.83em; margin-bottom: 0.83em; font-weight: bold; }\n" ++
    "h3 { display: block; font-size: 1.17em; margin-top: 1em; margin-bottom: 1em; font-weight: bold; }\n" ++
    "ul, ol { display: block; padding-left: 40px; margin-top: 1em; margin-bottom: 1em; }\n" ++
    "li { display: list-item; }\n" ++
    "table { display: table; border-collapse: separate; border-spacing: 2px; }\n" ++
    "tr { display: table-row; }\n" ++
    "td, th { display: table-cell; vertical-align: inherit; }\n" ++
    "strong, b { font-weight: bold; }\n" ++
    "em, i { font-style: italic; }\n" ++
    "pre { display: block; font-family: monospace; white-space: pre; margin: 1em 0; }\n" ++
    "code { font-family: monospace; }\n" ++
    "a { color: rgb(0, 0, 238); text-decoration: underline; cursor: pointer; }\n" ++
    "/* CapSurf Native User Agent Default Stylesheet For All Browsing Contexts */\n" ++
    "/* Standard W3C CSS2.1 / HTML5 User Agent Style Specifications Rules */\n" ++
    "form { display: block; margin-top: 0em; }\n" ++
    "input, textarea, select, button { font-family: inherit; font-size: inherit; }\n";

fn isInheritedProperty(name: []const u8): bool {
    if (std.mem.startsWith(u8, name, "--")) return true;
    const INHERITED_NAMES: [70][]const u8 = [
        "color",
        "cursor",
        "direction",
        "font",
        "font-family",
        "font-feature-settings",
        "font-kerning",
        "font-language-override",
        "font-optical-sizing",
        "font-palette",
        "font-size",
        "font-size-adjust",
        "font-stretch",
        "font-style",
        "font-synthesis",
        "font-synthesis-position",
        "font-synthesis-small-caps",
        "font-synthesis-style",
        "font-synthesis-weight",
        "font-variant",
        "font-variant-alternates",
        "font-variant-caps",
        "font-variant-east-asian",
        "font-variant-emoji",
        "font-variant-ligatures",
        "font-variant-numeric",
        "font-variant-position",
        "font-variation-settings",
        "font-weight",
        "hanging-punctuation",
        "hyphenate-character",
        "hyphenate-limit-chars",
        "hyphens",
        "letter-spacing",
        "line-break",
        "line-height",
        "list-style",
        "list-style-image",
        "list-style-position",
        "list-style-type",
        "overflow-wrap",
        "paint-order",
        "quotes",
        "tab-size",
        "text-align",
        "text-align-last",
        "text-anchor",
        "text-decoration-skip",
        "text-decoration-skip-ink",
        "text-emphasis",
        "text-emphasis-color",
        "text-emphasis-position",
        "text-emphasis-style",
        "text-indent",
        "text-justify",
        "text-orientation",
        "text-rendering",
        "text-shadow",
        "text-size-adjust",
        "text-transform",
        "visibility",
        "white-space",
        "word-break",
        "word-spacing",
        "word-wrap",
        "writing-mode",
        "border-collapse",
        "border-spacing",
        "caption-side",
        "empty-cells",
    ];
    let i: usize = 0;
    while (i < INHERITED_NAMES.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(name, INHERITED_NAMES[i])) return true;
    }
    return false;
}

fn formatRuleMatch(
    doc: *const Document,
    target_nid: NodeId,
    snid: usize,
    r: *const CssRule,
    only_inherited: bool,
    out_buf: []u8,
    pos: *usize,
    rule_idx: *usize,
): bool {
    if (r.kind != CssRuleKind.Style || r.prelude.len == 0) return false;
    const sel_list = SelectorList.parse(r.prelude);
    if (!Matcher.matchesSelectorList(doc, target_nid, sel_list)) return false;

    if (only_inherited) {
        let has_inh = false;
        let pi: usize = 0;
        while (pi < r.prop_count) : (pi += 1) {
            if (isInheritedProperty(r.properties[pi].name)) {
                has_inh = true;
                break;
            }
        }
        if (!has_inh) return false;
    }

    // Determine matching selectors
    let match_indices: [8]usize = [0; 8];
    let match_cnt: usize = 0;
    let si: usize = 0;
    while (si < sel_list.count) : (si += 1) {
        if (Matcher.matchesSelector(doc, target_nid, &sel_list.selectors[si])) {
            match_indices[match_cnt] = si;
            match_cnt += 1;
        }
    }
    if (match_cnt == 0) {
        match_indices[0] = 0;
        match_cnt = 1;
    }

    const sep = if (rule_idx.* > 0) "," else "";
    const p0 = pos.*;
    const r_start = std.fmt.bufPrint(
        out_buf[p0..],
        "{s}{{\"rule\":{{\"styleSheetId\":\"sheet_{d}\",\"origin\":\"regular\",\"selectorList\":{{\"selectors\":[",
        .{ sep, snid },
    ) catch return false;
    pos.* += r_start.len;

    // Output selectorList.selectors
    let s_out_i: usize = 0;
    while (s_out_i < sel_list.count) : (s_out_i += 1) {
        const sel = &sel_list.selectors[s_out_i];
        const s_sep = if (s_out_i > 0) "," else "";
        const s_chunk = std.fmt.bufPrint(
            out_buf[pos.*..],
            "{s}{{\"text\":\"{s}\",\"specificity\":{{\"a\":{d},\"b\":{d},\"c\":{d}}}}}",
            .{ s_sep, sel.raw, sel.specificity.id_selectors, sel.specificity.class_like_selectors, sel.specificity.element_selectors },
        ) catch return false;
        pos.* += s_chunk.len;
    }

    const mid_chunk = std.fmt.bufPrint(
        out_buf[pos.*..],
        "],\"text\":\"{s}\"}},\"style\":{{\"styleSheetId\":\"sheet_{d}\",\"cssProperties\":[",
        .{ r.prelude, snid },
    ) catch return false;
    pos.* += mid_chunk.len;

    // Output cssProperties
    let p_out_count: usize = 0;
    let pi: usize = 0;
    while (pi < r.prop_count) : (pi += 1) {
        const p = &r.properties[pi];
        if (only_inherited && !isInheritedProperty(p.name)) continue;
        const p_sep = if (p_out_count > 0) "," else "";
        const p_chunk = std.fmt.bufPrint(
            out_buf[pos.*..],
            "{s}{{\"name\":\"{s}\",\"value\":\"{s}\"}}",
            .{ p_sep, p.name, p.value },
        ) catch return false;
        pos.* += p_chunk.len;
        p_out_count += 1;
    }

    const style_end = std.fmt.bufPrint(
        out_buf[pos.*..],
        "],\"shorthandEntries\":[],\"cssText\":\"\",\"range\":{{\"startLine\":0,\"startColumn\":0,\"endLine\":0,\"endColumn\":0}}}}}},\"matchingSelectors\":[",
        .{},
    ) catch return false;
    pos.* += style_end.len;

    // Output matchingSelectors
    let mi: usize = 0;
    while (mi < match_cnt) : (mi += 1) {
        const m_sep = if (mi > 0) "," else "";
        const m_chunk = std.fmt.bufPrint(out_buf[pos.*..], "{s}{d}", .{ m_sep, match_indices[mi] }) catch return false;
        pos.* += m_chunk.len;
    }

    const r_end = std.fmt.bufPrint(out_buf[pos.*..], "]}}", .{}) catch return false;
    pos.* += r_end.len;
    rule_idx.* += 1;
    return true;
}

pub struct CssHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8, out_events: *[MAX_CDP_EVENTS]CdpEvent, event_count: *usize): CdpResponse {
        if (std.mem.eql(u8, req.method, "CSS.enable")) {
            session.css_enabled = true;
            session.tracked_style_count = 0;
            session.pending_css_event_count = 0;
            session.css_event_storage_used = 0;
            let clr_i: usize = 0;
            while (clr_i < session.tracked_style_nodes.len) : (clr_i += 1) {
                session.tracked_style_nodes[clr_i] = NULL_NODE;
                session.tracked_style_hashes[clr_i] = 0;
                session.tracked_style_lens[clr_i] = 0;
            }
            // Emit styleSheetAdded for each <style> element
            let snid: usize = 0;
            while (snid < session.doc.nodes.items.len && event_count.* < out_events.len) : (snid += 1) {
                const sn = &session.doc.nodes.items[snid];
                if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style") && session.doc.isConnectedNode(@intCast(snid))) {
                    const current_nid: NodeId = @intCast(snid);
                    let st_buf: [8192]u8 = [0; 8192];
                    const st_text = session.getStyleText(current_nid, &st_buf);

                    if (session.tracked_style_count < session.tracked_style_nodes.len) {
                        session.tracked_style_nodes[session.tracked_style_count] = current_nid;
                        session.tracked_style_hashes[session.tracked_style_count] = hashStyleBytes(st_text);
                        session.tracked_style_lens[session.tracked_style_count] = st_text.len;
                        session.tracked_style_count += 1;
                    }

                    const offset = event_count.* * 512;
                    if (offset + 512 <= session.css_event_buf.len) {
                        const chunk = session.css_event_buf[offset .. offset + 512];
                        let end_line: usize = 0;
                        let last_line_start: usize = 0;
                        let ti: usize = 0;
                        while (ti < st_text.len) : (ti += 1) {
                            if (st_text[ti] == '\n') {
                                end_line += 1;
                                last_line_start = ti + 1;
                            }
                        }
                        const end_col = st_text.len - last_line_start;
                        const evt_json = std.fmt.bufPrint(
                            chunk,
                            "{{\"header\":{{\"styleSheetId\":\"sheet_{d}\",\"frameId\":\"{s}\",\"sourceURL\":\"\",\"origin\":\"regular\",\"title\":\"\",\"disabled\":false,\"isInline\":true,\"isMutable\":false,\"isConstructed\":false,\"startLine\":0,\"startColumn\":0,\"length\":{d},\"endLine\":{d},\"endColumn\":{d},\"ownerNode\":{d}}}}}",
                            .{ snid, session.frame_id, st_text.len, end_line, end_col, snid },
                        ) catch "";
                        if (evt_json.len > 0) {
                            out_events[event_count.*] = CdpEvent.create("CSS.styleSheetAdded", evt_json);
                            event_count.* += 1;
                        }
                    }
                }
            }

            // Also emit UA stylesheet if space remains
            if (event_count.* < out_events.len) {
                const offset = event_count.* * 512;
                if (offset + 512 <= session.css_event_buf.len) {
                    const chunk = session.css_event_buf[offset .. offset + 512];
                    const ua_evt = std.fmt.bufPrint(
                        chunk,
                        "{{\"header\":{{\"styleSheetId\":\"ua_sheet_0\",\"frameId\":\"{s}\",\"sourceURL\":\"\",\"origin\":\"user-agent\",\"title\":\"\",\"disabled\":false,\"isInline\":false,\"isMutable\":false,\"isConstructed\":false,\"startLine\":0,\"startColumn\":0,\"length\":{d},\"endLine\":26,\"endColumn\":0}}}}",
                        .{ session.frame_id, UA_STYLESHEET_TEXT.len },
                    ) catch "";
                    if (ua_evt.len > 0) {
                        out_events[event_count.*] = CdpEvent.create("CSS.styleSheetAdded", ua_evt);
                        event_count.* += 1;
                    }
                }
            }

            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "CSS.disable") ||
            std.mem.eql(u8, req.method, "CSS.reset")) {
            session.css_enabled = false;
            session.tracked_style_count = 0;
            session.pending_css_event_count = 0;
            session.css_event_storage_used = 0;
            let clr_i: usize = 0;
            while (clr_i < session.tracked_style_nodes.len) : (clr_i += 1) {
                session.tracked_style_nodes[clr_i] = NULL_NODE;
                session.tracked_style_hashes[clr_i] = 0;
                session.tracked_style_lens[clr_i] = 0;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "CSS.setStyleSheetText")) {
            const sheet_id = extractJsonString(req.params_raw, "styleSheetId") ?? "";
            const raw_text = extractJsonString(req.params_raw, "text") ?? "";
            let text_buf = session.allocator.alloc(u8, @max(raw_text.len + 1, @as(usize, 65536))) catch return CdpResponse.err(req.id, -32000, "Out of memory");
            defer session.allocator.free(text_buf);
            const text = unescapeJsonString(raw_text, text_buf);

            if (std.mem.startsWith(u8, sheet_id, "sheet_")) {
                const id_num_str = sheet_id[6..];
                const snid = std.fmt.parseInt(usize, id_num_str, 10) catch 999999;
                if (snid < session.doc.nodes.items.len) {
                    if (session.setStyleSheetText(@intCast(snid), text)) {
                        const offset = event_count.* * 512;
                        if (offset + 512 <= session.css_event_buf.len) {
                            const chunk = session.css_event_buf[offset .. offset + 512];
                            const chg_evt = std.fmt.bufPrint(chunk, "{{\"styleSheetId\":\"{s}\"}}", .{sheet_id}) catch "";
                            if (chg_evt.len > 0 && event_count.* < out_events.len) {
                                out_events[event_count.*] = CdpEvent.create("CSS.styleSheetChanged", chg_evt);
                                event_count.* += 1;
                            }
                        }
                        return CdpResponse.ok(req.id, "{}");
                    }
                }
            }
            return CdpResponse.err(req.id, -32000, "StyleSheet not found");
        }

        if (std.mem.eql(u8, req.method, "CSS.getStyleSheetText")) {
            const sheet_id = extractJsonString(req.params_raw, "styleSheetId") ?? "";
            if (std.mem.eql(u8, sheet_id, "ua_sheet_0")) {
                let esc_buf: [8192]u8 = [0; 8192];
                const esc_text = escapeJsonString(UA_STYLESHEET_TEXT, &esc_buf);
                const res = std.fmt.bufPrint(
                    out_buf,
                    "{{\"text\":\"{s}\"}}",
                    .{esc_text},
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, res);
            }
            if (std.mem.startsWith(u8, sheet_id, "sheet_")) {
                const id_num_str = sheet_id[6..];
                const snid = std.fmt.parseInt(usize, id_num_str, 10) catch 999999;
                if (snid < session.doc.nodes.items.len) {
                    const sn = &session.doc.nodes.items[snid];
                    let st_buf: [8192]u8 = [0; 8192];
                    const st_text = getStyleNodeText(&session.doc, sn, &st_buf);
                    let esc_buf: [8192]u8 = [0; 8192];
                    const esc_text = escapeJsonString(st_text, &esc_buf);
                    const res = std.fmt.bufPrint(
                        out_buf,
                        "{{\"text\":\"{s}\"}}",
                        .{esc_text},
                    ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                    return CdpResponse.ok(req.id, res);
                }
            }
            return CdpResponse.ok(req.id, "{\"text\":\"\"}");
        }

        if (std.mem.eql(u8, req.method, "CSS.getEnvironmentVariables")) {
            return CdpResponse.ok(req.id, "{\"environmentVariables\":{}}");
        }

        if (std.mem.eql(u8, req.method, "CSS.getPlatformFontsForNode")) {
            return CdpResponse.ok(req.id, "{\"fonts\":[]}");
        }

        if (std.mem.eql(u8, req.method, "CSS.getComputedStyleForNode")) {
            const node_id_val = extractJsonInt(req.params_raw, "nodeId") ?? 0;
            if (node_id_val <= 0 || node_id_val >= session.doc.nodes.items.len) {
                return CdpResponse.ok(req.id, "{\"computedStyle\":[],\"extraFields\":{\"isAppearanceBase\":false}}");
            }
            const nid: NodeId = @as(NodeId, @intCast(node_id_val));

            // Parse stylesheets and compute cascade styles for document
            let css_buf = session.allocator.alloc(u8, 65536) catch null;
            defer if (css_buf != null) session.allocator.free(css_buf.?);
            let css_len: usize = 0;
            if (css_buf != null) {
                const base_css = "";
                @memcpy(css_buf.?[0..base_css.len], base_css);
                css_len = base_css.len;
                let snid: usize = 0;
                while (snid < session.doc.nodes.items.len) : (snid += 1) {
                    const sn = &session.doc.nodes.items[snid];
                    if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style")) {
                        if (sn.text_content.len > 0) {
                            const copy_len = @min(sn.text_content.len, css_buf.?.len - css_len);
                            @memcpy(css_buf.?[css_len .. css_len + copy_len], sn.text_content[0..copy_len]);
                            css_len += copy_len;
                        }
                        let child = sn.first_child;
                        while (child != NULL_NODE && @as(usize, child) < session.doc.nodes.items.len) : (child = session.doc.nodes.items[@as(usize, child)].next_sibling) {
                            const scn = &session.doc.nodes.items[@as(usize, child)];
                            if (scn.node_type == NodeType.Text && scn.text_content.len > 0) {
                                const copy_len = @min(scn.text_content.len, css_buf.?.len - css_len);
                                @memcpy(css_buf.?[css_len .. css_len + copy_len], scn.text_content[0..copy_len]);
                                css_len += copy_len;
                            }
                        }
                    }
                }
            }
            const full_css = if (css_buf != null) css_buf.?[0..css_len] else "";
            const sheet = StyleSheet.parse(full_css, session.viewport_width, false);
            const count = @max(@as(usize, 1), session.doc.nodeCount());
            let styles = session.allocator.alloc(ComputedStyle, count) catch null;
            defer if (styles != null) session.allocator.free(styles.?);
            if (styles != null) {
                Cascade.computeStyles(&session.doc, &sheet, styles.?, 16.0);
            }

            let style = ComputedStyle{};
            if (styles != null && @as(usize, nid) < count) {
                style = styles.?[@as(usize, nid)];
            }

            let color_b: [64]u8 = [0; 64];
            const color_val = formatCssColor(style.color, &color_b);

            let bg_color_b: [64]u8 = [0; 64];
            const bg_color_val = formatCssColor(style.background_color, &bg_color_b);

            let font_size_b: [32]u8 = [0; 32];
            const font_size_val = formatFontSize(style.font_size_px, &font_size_b);

            let mt_b: [32]u8 = [0; 32];
            const mt_val = formatLengthPx(style.margin_top, &mt_b);

            let mr_b: [32]u8 = [0; 32];
            const mr_val = formatLengthPx(style.margin_right, &mr_b);

            let mb_b: [32]u8 = [0; 32];
            const mb_val = formatLengthPx(style.margin_bottom, &mb_b);

            let ml_b: [32]u8 = [0; 32];
            const ml_val = formatLengthPx(style.margin_left, &ml_b);

            let pt_b: [32]u8 = [0; 32];
            const pt_val = formatLengthPx(style.padding_top, &pt_b);

            let pr_b: [32]u8 = [0; 32];
            const pr_val = formatLengthPx(style.padding_right, &pr_b);

            let pb_b: [32]u8 = [0; 32];
            const pb_val = formatLengthPx(style.padding_bottom, &pb_b);

            let pl_b: [32]u8 = [0; 32];
            const pl_val = formatLengthPx(style.padding_left, &pl_b);

            const display_val = formatDisplay(style.display);
            const box_sizing_val = formatBoxSizing(style.box_sizing);
            const visibility_val = formatVisibility(style.visibility);

            const rect = session.layout.getBoundingClientRect(nid);
            let w_b: [32]u8 = [0; 32];
            let h_b: [32]u8 = [0; 32];
            const w_val = if (rect.width > 0.0) std.fmt.bufPrint(&w_b, "{d:.1}px", .{rect.width}) catch "auto" else "auto";
            const h_val = if (rect.height > 0.0) std.fmt.bufPrint(&h_b, "{d:.1}px", .{rect.height}) catch "auto" else "auto";

            let pos: usize = 0;
            const prefix = "{\"computedStyle\":[";
            @memcpy(out_buf[pos .. pos + prefix.len], prefix);
            pos += prefix.len;

            let pi: usize = 0;
            while (pi < DEFAULT_CSS_PROPERTIES.len) : (pi += 1) {
                const prop = &DEFAULT_CSS_PROPERTIES[pi];
                let val = prop.value;
                if (std.mem.eql(u8, prop.name, "color")) {
                    val = color_val;
                } else if (std.mem.eql(u8, prop.name, "font-size")) {
                    val = font_size_val;
                } else if (std.mem.eql(u8, prop.name, "margin-top")) {
                    val = mt_val;
                } else if (std.mem.eql(u8, prop.name, "margin-right")) {
                    val = mr_val;
                } else if (std.mem.eql(u8, prop.name, "margin-bottom")) {
                    val = mb_val;
                } else if (std.mem.eql(u8, prop.name, "margin-left")) {
                    val = ml_val;
                } else if (std.mem.eql(u8, prop.name, "padding-top")) {
                    val = pt_val;
                } else if (std.mem.eql(u8, prop.name, "padding-right")) {
                    val = pr_val;
                } else if (std.mem.eql(u8, prop.name, "padding-bottom")) {
                    val = pb_val;
                } else if (std.mem.eql(u8, prop.name, "padding-left")) {
                    val = pl_val;
                } else if (std.mem.eql(u8, prop.name, "display")) {
                    val = display_val;
                } else if (std.mem.eql(u8, prop.name, "box-sizing")) {
                    val = box_sizing_val;
                } else if (std.mem.eql(u8, prop.name, "visibility")) {
                    val = visibility_val;
                } else if (std.mem.eql(u8, prop.name, "background-color")) {
                    val = bg_color_val;
                } else if (std.mem.eql(u8, prop.name, "width") && !std.mem.eql(u8, w_val, "auto")) {
                    val = w_val;
                } else if (std.mem.eql(u8, prop.name, "height") && !std.mem.eql(u8, h_val, "auto")) {
                    val = h_val;
                }

                const sep = if (pi > 0) "," else "";
                const entry = std.fmt.bufPrint(
                    out_buf[pos..],
                    "{s}{{\"name\":\"{s}\",\"value\":\"{s}\"}}",
                    .{ sep, prop.name, val },
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                pos += entry.len;
            }

            const suffix = "],\"extraFields\":{\"isAppearanceBase\":false}}";
            @memcpy(out_buf[pos .. pos + suffix.len], suffix);
            pos += suffix.len;

            return CdpResponse.ok(req.id, out_buf[0..pos]);
        }

        if (std.mem.eql(u8, req.method, "CSS.getInlineStylesForNode")) {
            const node_id_val = extractJsonInt(req.params_raw, "nodeId") ?? 0;
            if (node_id_val <= 0 || node_id_val >= session.doc.nodes.items.len) {
                return CdpResponse.ok(req.id, "{}");
            }
            const node = &session.doc.nodes.items[@as(usize, @intCast(node_id_val))];
            if (!node.hasAttribute("style")) {
                return CdpResponse.ok(req.id, "{}");
            }

            const style_val = node.getAttribute("style") ?? "";
            if (style_val.len == 0) {
                return CdpResponse.ok(req.id, "{\"inlineStyle\":{\"cssProperties\":[],\"shorthandEntries\":[],\"cssText\":\"\"}}");
            }

            let pos: usize = 0;
            const prefix = "{\"inlineStyle\":{\"cssProperties\":[";
            @memcpy(out_buf[pos .. pos + prefix.len], prefix);
            pos += prefix.len;

            let it = std.mem.splitScalar(u8, style_val, ';');
            let prop_idx: usize = 0;
            while (decl_item := it.next()) {
                const trimmed = std.mem.trim(u8, decl_item, " \t\r\n");
                if (trimmed.len == 0) continue;
                if (colon_idx := std.mem.indexOfScalar(u8, trimmed, ':')) {
                    const pname = std.mem.trim(u8, trimmed[0..colon_idx], " \t\r\n");
                    let pval = std.mem.trim(u8, trimmed[colon_idx + 1 ..], " \t\r\n");
                    let is_important = false;
                    if (std.mem.endsWith(u8, pval, "!important")) {
                        is_important = true;
                        pval = std.mem.trim(u8, pval[0 .. pval.len - 10], " \t\r\n");
                    }
                    const sep = if (prop_idx > 0) "," else "";
                    const chunk = if (is_important)
                        std.fmt.bufPrint(
                            out_buf[pos..],
                            "{s}{{\"name\":\"{s}\",\"value\":\"{s}\",\"important\":true}}",
                            .{ sep, pname, pval },
                        ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow")
                    else
                        std.fmt.bufPrint(
                            out_buf[pos..],
                            "{s}{{\"name\":\"{s}\",\"value\":\"{s}\"}}",
                            .{ sep, pname, pval },
                        ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                    pos += chunk.len;
                    prop_idx += 1;
                }
            }

            const suffix = std.fmt.bufPrint(
                out_buf[pos..],
                "],\"shorthandEntries\":[],\"cssText\":\"{s}\"}}}}",
                .{style_val},
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            pos += suffix.len;

            return CdpResponse.ok(req.id, out_buf[0..pos]);
        }

        if (std.mem.eql(u8, req.method, "CSS.getMatchedStylesForNode")) {
            const node_id_val = extractJsonInt(req.params_raw, "nodeId") ?? 0;
            if (node_id_val <= 0 || node_id_val >= session.doc.nodes.items.len) {
                return CdpResponse.ok(req.id, "{\"matchedCSSRules\":[],\"pseudoElements\":[],\"inherited\":[]}");
            }
            const nid: NodeId = @as(NodeId, @intCast(node_id_val));
            const node = &session.doc.nodes.items[@as(usize, @intCast(node_id_val))];
            if (node.node_type != NodeType.Element) {
                return CdpResponse.ok(req.id, "{\"matchedCSSRules\":[],\"pseudoElements\":[],\"inherited\":[]}");
            }

            let pos: usize = 0;
            const prefix = "{\"matchedCSSRules\":[";
            @memcpy(out_buf[pos .. pos + prefix.len], prefix);
            pos += prefix.len;

            let rule_idx: usize = 0;

            // 1. Author rules from document <style> elements (all properties)
            let snid: usize = 0;
            while (snid < session.doc.nodes.items.len) : (snid += 1) {
                const sn = &session.doc.nodes.items[snid];
                if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style")) {
                    let st_buf: [8192]u8 = [0; 8192];
                    const st_text = getStyleNodeText(&session.doc, sn, &st_buf);
                    if (st_text.len > 0) {
                        let raw_rules: [64]CssRule = [CssRule{}; 64];
                        const count = CssStylesheetParser.parseStylesheet(st_text, raw_rules[0..64]);
                        let ri: usize = 0;
                        while (ri < count) : (ri += 1) {
                            const r = &raw_rules[ri];
                            * = formatRuleMatch(&session.doc, nid, snid, r, false, out_buf, &pos, &rule_idx);
                        }
                    }
                }
            }

            // 2. UA rules (e.g. tag { display: block })
            const sep = if (rule_idx > 0) "," else "";
            const ua_rule = std.fmt.bufPrint(
                out_buf[pos..],
                "{s}{{\"rule\":{{\"origin\":\"user-agent\",\"selectorList\":{{\"selectors\":[{{\"text\":\"{s}\",\"specificity\":{{\"a\":0,\"b\":0,\"c\":1}}}}],\"text\":\"{s}\"}},\"style\":{{\"cssProperties\":[{{\"name\":\"display\",\"value\":\"block\"}}],\"shorthandEntries\":[],\"cssText\":\"display: block;\",\"range\":{{\"startLine\":0,\"startColumn\":0,\"endLine\":0,\"endColumn\":0}}}}}},\"matchingSelectors\":[0]}}",
                .{ sep, node.tag_name, node.tag_name },
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            pos += ua_rule.len;

            // 3. Inherited rules for element ancestors
            const inh_prefix = "],\"pseudoElements\":[],\"inherited\":[";
            @memcpy(out_buf[pos .. pos + inh_prefix.len], inh_prefix);
            pos += inh_prefix.len;

            let ancestors: [16]NodeId = [NULL_NODE; 16];
            let ancestor_count: usize = 0;
            let cur = node.parent;
            while (cur != NULL_NODE && ancestor_count < ancestors.len) {
                if (@as(usize, cur) < session.doc.nodes.items.len) {
                    const anc_node = &session.doc.nodes.items[@as(usize, cur)];
                    if (anc_node.node_type == NodeType.Element) {
                        ancestors[ancestor_count] = cur;
                        ancestor_count += 1;
                    }
                    cur = anc_node.parent;
                } else {
                    break;
                }
            }

            let ai: usize = 0;
            while (ai < ancestor_count) : (ai += 1) {
                const anc_nid = ancestors[ai];
                const asep = if (ai > 0) "," else "";
                const anc_start = std.fmt.bufPrint(
                    out_buf[pos..],
                    "{s}{{\"matchedCSSRules\":[",
                    .{asep},
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                pos += anc_start.len;

                let anc_rule_idx: usize = 0;
                let a_snid: usize = 0;
                while (a_snid < session.doc.nodes.items.len) : (a_snid += 1) {
                    const sn = &session.doc.nodes.items[a_snid];
                    if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style")) {
                        let st_buf: [8192]u8 = [0; 8192];
                        const st_text = getStyleNodeText(&session.doc, sn, &st_buf);
                        if (st_text.len > 0) {
                            let raw_rules: [64]CssRule = [CssRule{}; 64];
                            const count = CssStylesheetParser.parseStylesheet(st_text, raw_rules[0..64]);
                            let ri: usize = 0;
                            while (ri < count) : (ri += 1) {
                                const r = &raw_rules[ri];
                                * = formatRuleMatch(&session.doc, anc_nid, a_snid, r, true, out_buf, &pos, &anc_rule_idx);
                            }
                        }
                    }
                }

                const anc_end = "],\"pseudoElements\":[]}";
                @memcpy(out_buf[pos .. pos + anc_end.len], anc_end);
                pos += anc_end.len;
            }

            const suffix = "]}";
            @memcpy(out_buf[pos .. pos + suffix.len], suffix);
            pos += suffix.len;

            return CdpResponse.ok(req.id, out_buf[0..pos]);
        }

        return CdpResponse.ok(req.id, "{}");
    }
}
