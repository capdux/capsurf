// src/css/cascade.cp
// CapSurf 64-Bit Bit-Packed Weight CSS Cascade & ComputedStyle Engine (Milestone 1 & 5)
//
// Packed 64-bit Weight Formula:
//   weight: u64 = (important << 63) | (is_inline << 62) | (specificity_30bit << 32) | source_order_32bit
// Resolves !important, inline style, W3C Selectors Level 4 specificity, and source order
// in a single register comparison: `if (new_weight >= existing_weight)`.

import { Document, NodeData, NodeType, NodeId, NULL_NODE, StringSlice } from "../dom/dom.cp";
import { SelectorList, Matcher } from "./matcher.cp";
import { HtmlTag } from "../html/tags.cp";
import { UaStylesheet } from "./ua_stylesheet.cp";
import {
    CssColor,
    CssUnit,
    CssLength,
    CssCalc,
    CssProperty,
    CssRule,
    CssRuleKind,
    CssStylesheetParser
} from "cap:css";

pub enum DisplayKeyword {
    None,
    Contents,
}

pub enum DisplayOutside {
    Block,
    Inline,
}

pub enum DisplayInside {
    Flow,
    FlowRoot,
    Flex,
    Grid,
    Table,
}

pub struct DisplayPair {
    outside: DisplayOutside = DisplayOutside.Block;
    inside: DisplayInside = DisplayInside.Flow;
    is_list_item: bool = false;
}

pub struct Display {
    is_keyword: bool = false;
    keyword: DisplayKeyword = DisplayKeyword.None;
    pair: DisplayPair = DisplayPair{};

    static fn block(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.Flow },
        };
    }

    static fn inlineFlow(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Inline, inside: DisplayInside.Flow },
        };
    }

    static fn inlineBlock(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Inline, inside: DisplayInside.FlowRoot },
        };
    }

    static fn flex(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.Flex },
        };
    }

    static fn inlineFlex(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Inline, inside: DisplayInside.Flex },
        };
    }

    static fn grid(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.Grid },
        };
    }

    static fn none(): Display {
        return Display {
            is_keyword: true,
            keyword: DisplayKeyword.None,
        };
    }

    static fn contents(): Display {
        return Display {
            is_keyword: true,
            keyword: DisplayKeyword.Contents,
        };
    }

    static fn parse(raw: []const u8): Display? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "none")) return Display.none();
        if (std.ascii.eqlIgnoreCase(s, "contents")) return Display.contents();
        if (std.ascii.eqlIgnoreCase(s, "block") || std.ascii.eqlIgnoreCase(s, "flow-root")) return Display.block();
        if (std.ascii.eqlIgnoreCase(s, "inline")) return Display.inlineFlow();
        if (std.ascii.eqlIgnoreCase(s, "inline-block")) return Display.inlineBlock();
        if (std.ascii.eqlIgnoreCase(s, "flex") || std.ascii.eqlIgnoreCase(s, "-webkit-flex")) return Display.flex();
        if (std.ascii.eqlIgnoreCase(s, "inline-flex")) return Display.inlineFlex();
        if (std.ascii.eqlIgnoreCase(s, "grid") || std.ascii.eqlIgnoreCase(s, "inline-grid")) return Display.grid();
        return null;
    }

    fn isNone(): bool {
        return _.is_keyword && _.keyword == DisplayKeyword.None;
    }

    fn isContents(): bool {
        return _.is_keyword && _.keyword == DisplayKeyword.Contents;
    }

    fn isFlex(): bool {
        return !_.is_keyword && _.pair.inside == DisplayInside.Flex;
    }

    fn isGrid(): bool {
        return !_.is_keyword && _.pair.inside == DisplayInside.Grid;
    }

    fn isInline(): bool {
        return !_.is_keyword && _.pair.outside == DisplayOutside.Inline && _.pair.inside == DisplayInside.Flow;
    }

    fn isInlineBlock(): bool {
        return !_.is_keyword && _.pair.outside == DisplayOutside.Inline && _.pair.inside == DisplayInside.FlowRoot;
    }
}

pub enum Visibility {
    Visible,
    Hidden,
    Collapse,

    static fn parse(raw: []const u8): Visibility? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "visible")) return Visibility.Visible;
        if (std.ascii.eqlIgnoreCase(s, "hidden")) return Visibility.Hidden;
        if (std.ascii.eqlIgnoreCase(s, "collapse")) return Visibility.Collapse;
        return null;
    }
}

pub enum BoxSizing {
    ContentBox,
    BorderBox,

    static fn parse(raw: []const u8): BoxSizing? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "content-box")) return BoxSizing.ContentBox;
        if (std.ascii.eqlIgnoreCase(s, "border-box")) return BoxSizing.BorderBox;
        return null;
    }
}

pub enum Position {
    Static,
    Relative,
    Absolute,
    Fixed,
    Sticky,

    static fn parse(raw: []const u8): Position? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "static")) return Position.Static;
        if (std.ascii.eqlIgnoreCase(s, "relative")) return Position.Relative;
        if (std.ascii.eqlIgnoreCase(s, "absolute")) return Position.Absolute;
        if (std.ascii.eqlIgnoreCase(s, "fixed")) return Position.Fixed;
        if (std.ascii.eqlIgnoreCase(s, "sticky")) return Position.Sticky;
        return null;
    }
}

pub enum FlexDirection {
    Row,
    RowReverse,
    Column,
    ColumnReverse,

    static fn parse(raw: []const u8): FlexDirection? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "row")) return FlexDirection.Row;
        if (std.ascii.eqlIgnoreCase(s, "row-reverse")) return FlexDirection.RowReverse;
        if (std.ascii.eqlIgnoreCase(s, "column")) return FlexDirection.Column;
        if (std.ascii.eqlIgnoreCase(s, "column-reverse")) return FlexDirection.ColumnReverse;
        return null;
    }

    fn isRow(): bool {
        return _ == FlexDirection.Row || _ == FlexDirection.RowReverse;
    }

    fn isColumn(): bool {
        return _ == FlexDirection.Column || _ == FlexDirection.ColumnReverse;
    }
}

pub enum JustifyContent {
    FlexStart,
    FlexEnd,
    Center,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,

    static fn parse(raw: []const u8): JustifyContent? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "flex-start") || std.ascii.eqlIgnoreCase(s, "start") || std.ascii.eqlIgnoreCase(s, "left")) {
            return JustifyContent.FlexStart;
        }
        if (std.ascii.eqlIgnoreCase(s, "flex-end") || std.ascii.eqlIgnoreCase(s, "end") || std.ascii.eqlIgnoreCase(s, "right")) {
            return JustifyContent.FlexEnd;
        }
        if (std.ascii.eqlIgnoreCase(s, "center")) return JustifyContent.Center;
        if (std.ascii.eqlIgnoreCase(s, "space-between")) return JustifyContent.SpaceBetween;
        if (std.ascii.eqlIgnoreCase(s, "space-around")) return JustifyContent.SpaceAround;
        if (std.ascii.eqlIgnoreCase(s, "space-evenly")) return JustifyContent.SpaceEvenly;
        return null;
    }
}

pub enum AlignItems {
    Stretch,
    FlexStart,
    FlexEnd,
    Center,

    static fn parse(raw: []const u8): AlignItems? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "stretch")) return AlignItems.Stretch;
        if (std.ascii.eqlIgnoreCase(s, "flex-start") || std.ascii.eqlIgnoreCase(s, "start")) return AlignItems.FlexStart;
        if (std.ascii.eqlIgnoreCase(s, "flex-end") || std.ascii.eqlIgnoreCase(s, "end")) return AlignItems.FlexEnd;
        if (std.ascii.eqlIgnoreCase(s, "center")) return AlignItems.Center;
        return null;
    }
}

pub struct LengthPercentageOrAuto {
    is_auto: bool = false;
    is_none: bool = false;
    is_calc: bool = false;
    calc_expr: []const u8 = "";
    length: CssLength = CssLength { value: 0.0, unit: CssUnit.Px };

    static fn zeroPx(): LengthPercentageOrAuto {
        return LengthPercentageOrAuto {
            is_auto: false,
            length: CssLength.px(0.0),
        };
    }

    static fn autoVal(): LengthPercentageOrAuto {
        return LengthPercentageOrAuto {
            is_auto: true,
        };
    }

    static fn noneVal(): LengthPercentageOrAuto {
        return LengthPercentageOrAuto {
            is_none: true,
        };
    }

    static fn px(v: f32): LengthPercentageOrAuto {
        return LengthPercentageOrAuto {
            is_auto: false,
            length: CssLength.px(@as(f64, v)),
        };
    }

    static fn extractAnchorFallback(raw: []const u8): StringSlice? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        let inner: []const u8 = "";
        if (std.ascii.startsWithIgnoreCase(s, "anchor-size(") && s.len > 12 && s[s.len - 1] == ')') {
            inner = s[12 .. s.len - 1];
        } else if (std.ascii.startsWithIgnoreCase(s, "anchor(") && s.len > 7 && s[s.len - 1] == ')') {
            inner = s[7 .. s.len - 1];
        } else {
            return null;
        }

        let depth: i32 = 0;
        let last_comma: usize = 0;
        let found_comma = false;
        let i: usize = 0;
        while (i < inner.len) : (i += 1) {
            const c = inner[i];
            if (c == '(') {
                depth += 1;
            } else if (c == ')') {
                depth -= 1;
            } else if (c == ',' && depth == 0) {
                last_comma = i;
                found_comma = true;
            }
        }

        if (found_comma && last_comma + 1 < inner.len) {
            const fallback = std.mem.trim(u8, inner[last_comma + 1 ..], " \t\r\n");
            if (fallback.len > 0) {
                return fallback;
            }
        }
        return null;
    }

    static fn substituteAnchorFallbacks(expr: []const u8, out_buf: []u8): []const u8 {
        let in_idx: usize = 0;
        let out_idx: usize = 0;
        while (in_idx < expr.len) {
            let anchor_kind: usize = 0;
            if (in_idx + 12 <= expr.len && std.ascii.eqlIgnoreCase(expr[in_idx .. in_idx + 12], "anchor-size(")) {
                anchor_kind = 2;
            } else if (in_idx + 7 <= expr.len && std.ascii.eqlIgnoreCase(expr[in_idx .. in_idx + 7], "anchor(")) {
                anchor_kind = 1;
            }

            if (anchor_kind > 0) {
                const open_paren = if (anchor_kind == 2) in_idx + 11 else in_idx + 6;
                let depth: i32 = 1;
                let close_paren = open_paren + 1;
                let last_comma: usize = 0;
                let found_comma = false;
                while (close_paren < expr.len && depth > 0) : (close_paren += 1) {
                    const ch = expr[close_paren];
                    if (ch == '(') {
                        depth += 1;
                    } else if (ch == ')') {
                        depth -= 1;
                        if (depth == 0) break;
                    } else if (ch == ',' && depth == 1) {
                        last_comma = close_paren;
                        found_comma = true;
                    }
                }

                if (depth == 0) {
                    let fallback: []const u8 = "0px";
                    if (found_comma && last_comma + 1 < close_paren) {
                        const trimmed_fb = std.mem.trim(u8, expr[last_comma + 1 .. close_paren], " \t\r\n");
                        if (trimmed_fb.len > 0) {
                            fallback = trimmed_fb;
                        }
                    }
                    let fi: usize = 0;
                    while (fi < fallback.len) : (fi += 1) {
                        if (out_idx < out_buf.len) {
                            out_buf[out_idx] = fallback[fi];
                            out_idx += 1;
                        }
                    }
                    in_idx = close_paren + 1;
                    continue;
                }
            }

            if (out_idx < out_buf.len) {
                out_buf[out_idx] = expr[in_idx];
                out_idx += 1;
            }
            in_idx += 1;
        }
        return out_buf[0..out_idx];
    }

    static fn parse(raw: []const u8): LengthPercentageOrAuto? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        if (std.ascii.eqlIgnoreCase(s, "auto")) {
            return LengthPercentageOrAuto.autoVal();
        }
        if (std.ascii.eqlIgnoreCase(s, "none")) {
            return LengthPercentageOrAuto.noneVal();
        }
        if (std.ascii.startsWithIgnoreCase(s, "calc(")) {
            return LengthPercentageOrAuto {
                is_calc: true,
                calc_expr: s,
            };
        }
        if (std.ascii.startsWithIgnoreCase(s, "anchor(") || std.ascii.startsWithIgnoreCase(s, "anchor-size(")) {
            if (fb := LengthPercentageOrAuto.extractAnchorFallback(s)) {
                return LengthPercentageOrAuto.parse(fb);
            }
            return LengthPercentageOrAuto.zeroPx();
        }
        if (s.len > 1 && s[0] == '-') {
            if (p := LengthPercentageOrAuto.parse(s[1..])) {
                let neg_p = p;
                neg_p.length.value = 0.0 - neg_p.length.value;
                return neg_p;
            }
        }
        if (len_v := CssLength.parse(s)) {
            return LengthPercentageOrAuto {
                is_auto: false,
                length: len_v,
            };
        }
        return null;
    }

    fn resolvePx(
        parent_px: f32,
        element_font_px: f32,
        root_font_px: f32,
        viewport_w: f32,
        viewport_h: f32,
    ): f32 {
        if (_.is_auto || _.is_none) {
            return 0.0;
        }
        if (_.is_calc) {
            let expr = _.calc_expr;
            let buf: [512]u8 = std.mem.zeroes([512]u8);
            if (std.mem.indexOf(u8, expr, "anchor") != null) {
                expr = LengthPercentageOrAuto.substituteAnchorFallbacks(expr, &buf);
            }
            if (v := CssCalc.evalPx(expr, @as(f64, root_font_px), @as(f64, viewport_w), @as(f64, viewport_h))) {
                return @floatCast(v);
            }
            return 0.0;
        }
        if (_.length.unit == CssUnit.Percent) {
            const ratio: f32 = @floatCast(_.length.value / 100.0);
            return ratio * parent_px;
        }
        if (_.length.unit == CssUnit.Em) {
            const em_mult: f32 = @floatCast(_.length.value);
            return em_mult * element_font_px;
        }
        const px_f64 = _.length.toPx(@as(f64, root_font_px), @as(f64, viewport_w), @as(f64, viewport_h));
        return @floatCast(px_f64);
    }
}

// Property slot indices for per-property 64-bit cascade weights
pub const PROP_DISPLAY: usize = 0;
pub const PROP_VISIBILITY: usize = 1;
pub const PROP_BOX_SIZING: usize = 2;
pub const PROP_POSITION: usize = 3;
pub const PROP_WIDTH: usize = 4;
pub const PROP_HEIGHT: usize = 5;
pub const PROP_MIN_WIDTH: usize = 6;
pub const PROP_MIN_HEIGHT: usize = 7;
pub const PROP_MAX_WIDTH: usize = 8;
pub const PROP_MAX_HEIGHT: usize = 9;
pub const PROP_MARGIN_TOP: usize = 10;
pub const PROP_MARGIN_RIGHT: usize = 11;
pub const PROP_MARGIN_BOTTOM: usize = 12;
pub const PROP_MARGIN_LEFT: usize = 13;
pub const PROP_PADDING_TOP: usize = 14;
pub const PROP_PADDING_RIGHT: usize = 15;
pub const PROP_PADDING_BOTTOM: usize = 16;
pub const PROP_PADDING_LEFT: usize = 17;
pub const PROP_BORDER_TOP: usize = 18;
pub const PROP_BORDER_RIGHT: usize = 19;
pub const PROP_BORDER_BOTTOM: usize = 20;
pub const PROP_BORDER_LEFT: usize = 21;
pub const PROP_TOP: usize = 22;
pub const PROP_RIGHT: usize = 23;
pub const PROP_BOTTOM: usize = 24;
pub const PROP_LEFT: usize = 25;
pub const PROP_FLEX_DIRECTION: usize = 26;
pub const PROP_JUSTIFY_CONTENT: usize = 27;
pub const PROP_ALIGN_ITEMS: usize = 28;
pub const PROP_FLEX_GROW: usize = 29;
pub const PROP_FLEX_SHRINK: usize = 30;
pub const PROP_FLEX_BASIS: usize = 31;
pub const PROP_ROW_GAP: usize = 32;
pub const PROP_COLUMN_GAP: usize = 33;
pub const PROP_FONT_SIZE: usize = 34;
pub const PROP_COLOR: usize = 35;
pub const PROP_BG_COLOR: usize = 36;
pub const PROP_OPACITY: usize = 37;
pub const PROP_Z_INDEX: usize = 38;
pub const PROP_GRID_COLS: usize = 39;
pub const PROP_BORDER_COLOR: usize = 40;
pub const PROP_BORDER_RADIUS: usize = 41;
pub const PROP_LINE_HEIGHT: usize = 42;
pub const PROP_GRID_COL_SPAN: usize = 43;
pub const PROP_BG_IMAGE: usize = 44;
pub const PROP_FONT_FAMILY: usize = 45;
pub const PROP_FONT_WEIGHT: usize = 46;
pub const PROP_OVERFLOW: usize = 47;
pub const PROP_TRANSFORM: usize = 48;
pub const TOTAL_PROP_SLOTS: usize = 49;

pub struct BoxQuad {
    top: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    right: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    bottom: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    left: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
}

pub struct ConicGradientStop {
    color: CssColor = CssColor.rgba(0, 0, 0, 0);
    angle_deg: f32 = 0.0;
}

pub struct ConicGradient {
    has_gradient: bool = false;
    from_angle_deg: f32 = 0.0;
    center_x_pct: f32 = 50.0;
    center_y_pct: f32 = 50.0;
    center_x_px: f32 = 0.0;
    center_y_px: f32 = 0.0;
    center_x_is_px: bool = false;
    center_y_is_px: bool = false;
    stops: [16]ConicGradientStop = [ConicGradientStop{}; 16];
    stop_count: usize = 0;

    fn sampleAt(
        box_x: f32,
        box_y: f32,
        box_w: f32,
        box_h: f32,
        px: f32,
        py: f32,
    ): CssColor {
        if (!_.has_gradient || _.stop_count == 0) {
            return CssColor.rgba(0, 0, 0, 0);
        }
        if (_.stop_count == 1) {
            return _.stops[0].color;
        }

        const cx = if (_.center_x_is_px) (box_x + _.center_x_px) else (box_x + (_.center_x_pct / 100.0) * box_w);
        const cy = if (_.center_y_is_px) (box_y + _.center_y_px) else (box_y + (_.center_y_pct / 100.0) * box_h);

        const dx = px - cx;
        const dy = py - cy;

        if (@abs(dx) < 0.00001 && @abs(dy) < 0.00001) {
            return _.stops[0].color;
        }

        // CSS conic-gradient starts at 12 o'clock (0deg, up) and progresses clockwise.
        // Screen coordinates: +X is right, +Y is down (so up is negative Y).
        // sin(theta) = dx / r, cos(theta) = -dy / r.
        // atan2(y, x) takes (sin, cos) -> (dx, -dy).
        const rad = std.math.atan2(dx, 0.0 - dy);
        let deg = rad * 180.0 / std.math.pi;
        if (deg < 0.0) deg += 360.0;
        if (deg >= 360.0) deg -= 360.0;

        let rel_deg = deg - _.from_angle_deg;
        while (rel_deg < 0.0) rel_deg += 360.0;
        while (rel_deg >= 360.0) rel_deg -= 360.0;

        if (rel_deg <= _.stops[0].angle_deg) {
            return _.stops[0].color;
        }
        if (rel_deg >= _.stops[_.stop_count - 1].angle_deg) {
            return _.stops[_.stop_count - 1].color;
        }

        let i: usize = 1;
        while (i < _.stop_count) : (i += 1) {
            if (rel_deg <= _.stops[i].angle_deg) {
                const c0 = _.stops[i - 1].color;
                const c1 = _.stops[i].color;
                const span = _.stops[i].angle_deg - _.stops[i - 1].angle_deg;
                if (span <= 0.0001) {
                    return c1;
                }
                const t = (rel_deg - _.stops[i - 1].angle_deg) / span;
                return c0.mix(c1, @as(f64, t));
            }
        }

        return _.stops[_.stop_count - 1].color;
    }

    static fn parse(val_str: []const u8): ConicGradient? {
        let cg_pos: usize = 0;
        let found = false;
        if (pos := std.mem.indexOf(u8, val_str, "conic-gradient")) {
            cg_pos = pos;
            found = true;
        } else if (pos_upper := std.mem.indexOf(u8, val_str, "CONIC-GRADIENT")) {
            cg_pos = pos_upper;
            found = true;
        }
        if (!found) return null;

        const paren_open = std.mem.indexOfScalarPos(u8, val_str, cg_pos + 14, '(') orelse return null;
        const args_start = paren_open + 1;

        let paren_depth: usize = 1;
        let i = args_start;
        let args_end: usize = 0;
        while (i < val_str.len) : (i += 1) {
            if (val_str[i] == '(') {
                paren_depth += 1;
            } else if (val_str[i] == ')') {
                paren_depth -= 1;
                if (paren_depth == 0) {
                    args_end = i;
                    break;
                }
            }
        }
        if (args_end <= args_start) {
            return null;
        }

        const args_content = val_str[args_start..args_end];
        let items: [16][]const u8 = [""; 16];
        let item_count: usize = 0;

        let item_start: usize = 0;
        let pdepth: usize = 0;
        let idx: usize = 0;
        while (idx <= args_content.len) : (idx += 1) {
            const is_end = (idx == args_content.len);
            const c: u8 = if (is_end) 0 else args_content[idx];
            if (!is_end && c == '(') {
                pdepth += 1;
            } else if (!is_end && c == ')') {
                if (pdepth > 0) pdepth -= 1;
            } else if (is_end || (c == ',' && pdepth == 0)) {
                const item = std.mem.trim(u8, args_content[item_start..idx], " \t\r\n");
                if (item.len > 0 && item_count < 16) {
                    items[item_count] = item;
                    item_count += 1;
                }
                item_start = idx + 1;
            }
        }

        if (item_count == 0) {
            return null;
        }

        let cg = ConicGradient{
            has_gradient: true,
        };

        let first_is_header = false;
        const it0 = items[0];
        if (std.mem.startsWith(u8, it0, "from ") || std.mem.startsWith(u8, it0, "at ")) {
            first_is_header = true;
            let hit = std.mem.tokenizeAny(u8, it0, " \t\r\n");
            let in_from = false;
            let in_at = false;
            let at_dim: usize = 0;
            while (tok := hit.next()) {
                if (std.mem.eql(u8, tok, "from")) {
                    in_from = true;
                    in_at = false;
                    continue;
                }
                if (std.mem.eql(u8, tok, "at")) {
                    in_at = true;
                    in_from = false;
                    at_dim = 0;
                    continue;
                }
                if (in_from) {
                    if (std.mem.endsWith(u8, tok, "deg")) {
                        const deg_str = tok[0 .. tok.len - 3];
                        cg.from_angle_deg = std.fmt.parseFloat(f32, deg_str) catch 0.0;
                    } else if (std.mem.endsWith(u8, tok, "rad")) {
                        const rad_str = tok[0 .. tok.len - 3];
                        const rad_v = std.fmt.parseFloat(f32, rad_str) catch 0.0;
                        cg.from_angle_deg = rad_v * 180.0 / std.math.pi;
                    } else if (std.mem.endsWith(u8, tok, "turn")) {
                        const turn_str = tok[0 .. tok.len - 4];
                        const turn_v = std.fmt.parseFloat(f32, turn_str) catch 0.0;
                        cg.from_angle_deg = turn_v * 360.0;
                    } else {
                        cg.from_angle_deg = std.fmt.parseFloat(f32, tok) catch 0.0;
                    }
                    in_from = false;
                } else if (in_at) {
                    if (at_dim == 0) {
                        if (std.mem.eql(u8, tok, "left")) {
                            cg.center_x_pct = 0.0;
                            cg.center_x_is_px = false;
                        } else if (std.mem.eql(u8, tok, "center")) {
                            cg.center_x_pct = 50.0;
                            cg.center_x_is_px = false;
                        } else if (std.mem.eql(u8, tok, "right")) {
                            cg.center_x_pct = 100.0;
                            cg.center_x_is_px = false;
                        } else if (std.mem.endsWith(u8, tok, "%")) {
                            const pct_str = tok[0 .. tok.len - 1];
                            cg.center_x_pct = std.fmt.parseFloat(f32, pct_str) catch 50.0;
                            cg.center_x_is_px = false;
                        } else if (std.mem.endsWith(u8, tok, "px")) {
                            const px_str = tok[0 .. tok.len - 2];
                            cg.center_x_px = std.fmt.parseFloat(f32, px_str) catch 0.0;
                            cg.center_x_is_px = true;
                        }
                        at_dim = 1;
                    } else if (at_dim == 1) {
                        if (std.mem.eql(u8, tok, "top")) {
                            cg.center_y_pct = 0.0;
                            cg.center_y_is_px = false;
                        } else if (std.mem.eql(u8, tok, "center")) {
                            cg.center_y_pct = 50.0;
                            cg.center_y_is_px = false;
                        } else if (std.mem.eql(u8, tok, "bottom")) {
                            cg.center_y_pct = 100.0;
                            cg.center_y_is_px = false;
                        } else if (std.mem.endsWith(u8, tok, "%")) {
                            const pct_str = tok[0 .. tok.len - 1];
                            cg.center_y_pct = std.fmt.parseFloat(f32, pct_str) catch 50.0;
                            cg.center_y_is_px = false;
                        } else if (std.mem.endsWith(u8, tok, "px")) {
                            const px_str = tok[0 .. tok.len - 2];
                            cg.center_y_px = std.fmt.parseFloat(f32, px_str) catch 0.0;
                            cg.center_y_is_px = true;
                        }
                        at_dim = 2;
                    }
                }
            }
        }

        const stop_start: usize = if (first_is_header) 1 else 0;
        const total_stops = item_count - stop_start;
        if (total_stops == 0) {
            return null;
        }

        let stop_idx: usize = 0;
        let item_i = stop_start;
        while (item_i < item_count && stop_idx < 16) : (item_i += 1) {
            const raw_stop = items[item_i];
            let color_part: []const u8 = raw_stop;
            let angle_part: []const u8 = "";

            if (cp := std.mem.lastIndexOfScalar(u8, raw_stop, ')')) {
                color_part = std.mem.trim(u8, raw_stop[0 .. cp + 1], " \t\r\n");
                angle_part = std.mem.trim(u8, raw_stop[cp + 1 ..], " \t\r\n");
            } else if (ls := std.mem.lastIndexOfScalar(u8, raw_stop, ' ')) {
                const right_tok = std.mem.trim(u8, raw_stop[ls + 1 ..], " \t\r\n");
                if (std.mem.endsWith(u8, right_tok, "deg") ||
                    std.mem.endsWith(u8, right_tok, "turn") ||
                    std.mem.endsWith(u8, right_tok, "rad") ||
                    std.mem.endsWith(u8, right_tok, "%")) {
                    color_part = std.mem.trim(u8, raw_stop[0..ls], " \t\r\n");
                    angle_part = right_tok;
                }
            }

            const parsed_c = CssColor.parse(color_part) orelse CssColor.rgba(0, 0, 0, 255);
            let parsed_angle: f32 = 0.0;
            if (angle_part.len > 0) {
                if (std.mem.endsWith(u8, angle_part, "deg")) {
                    const deg_str = angle_part[0 .. angle_part.len - 3];
                    parsed_angle = std.fmt.parseFloat(f32, deg_str) catch 0.0;
                } else if (std.mem.endsWith(u8, angle_part, "turn")) {
                    const tv = std.fmt.parseFloat(f32, angle_part[0 .. angle_part.len - 4]) catch 0.0;
                    parsed_angle = tv * 360.0;
                } else if (std.mem.endsWith(u8, angle_part, "rad")) {
                    const rv = std.fmt.parseFloat(f32, angle_part[0 .. angle_part.len - 3]) catch 0.0;
                    parsed_angle = rv * 180.0 / std.math.pi;
                } else if (std.mem.endsWith(u8, angle_part, "%")) {
                    const pv = std.fmt.parseFloat(f32, angle_part[0 .. angle_part.len - 1]) catch 0.0;
                    parsed_angle = pv * 3.6;
                } else {
                    parsed_angle = std.fmt.parseFloat(f32, angle_part) catch 0.0;
                }
            } else {
                if (total_stops > 1) {
                    parsed_angle = (360.0 * @as(f32, @floatFromInt(stop_idx))) / @as(f32, @floatFromInt(total_stops - 1));
                } else {
                    parsed_angle = 0.0;
                }
            }

            cg.stops[stop_idx] = ConicGradientStop{
                color: parsed_c,
                angle_deg: parsed_angle,
            };
            stop_idx += 1;
        }

        cg.stop_count = stop_idx;
        return cg;
    }
}

/// Resolved visual style ready for the 2D software rasterizer (`src/render/painter.cp`)
pub struct ResolvedPaintStyle {
    bg_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    border_color: CssColor = CssColor.rgb(0, 0, 0);
    text_color: CssColor = CssColor.rgb(0, 0, 0);
    border_width: f32 = 0.0;
    border_radius: f32 = 0.0;
    font_size_px: f32 = 16.0;
    conic_gradient: ConicGradient = ConicGradient{};
}

fn canonicalizeCssPropertyName(name: []const u8): []const u8 {
    if (std.ascii.eqlIgnoreCase(name, "-webkit-box-sizing")) return "box-sizing";
    if (std.ascii.eqlIgnoreCase(name, "-webkit-border-radius")) return "border-radius";
    if (std.ascii.eqlIgnoreCase(name, "-webkit-align-items")) return "align-items";
    if (std.ascii.eqlIgnoreCase(name, "-webkit-justify-content")) return "justify-content";
    if (std.ascii.eqlIgnoreCase(name, "-webkit-flex-direction")) return "flex-direction";
    if (std.ascii.eqlIgnoreCase(name, "-webkit-flex-grow")) return "flex-grow";
    if (std.ascii.eqlIgnoreCase(name, "-webkit-flex-shrink")) return "flex-shrink";
    if (std.ascii.eqlIgnoreCase(name, "-webkit-flex-basis")) return "flex-basis";
    if (std.ascii.eqlIgnoreCase(name, "-webkit-flex")) return "flex";
    if (std.ascii.eqlIgnoreCase(name, "-webkit-opacity")) return "opacity";
    if (std.ascii.eqlIgnoreCase(name, "css-float") || std.ascii.eqlIgnoreCase(name, "cssfloat")) return "float";
    if (std.ascii.eqlIgnoreCase(name, "-webkit-transform")) return "transform";
    return name;
}

pub struct ComputedStyle {
    display: Display = Display.block();
    visibility: Visibility = Visibility.Visible;
    box_sizing: BoxSizing = BoxSizing.ContentBox;
    position: Position = Position.Static;

    width: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();
    height: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();
    min_width: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    min_height: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    max_width: LengthPercentageOrAuto = LengthPercentageOrAuto.noneVal();
    max_height: LengthPercentageOrAuto = LengthPercentageOrAuto.noneVal();

    margin_top: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    margin_right: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    margin_bottom: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    margin_left: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();

    padding_top: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    padding_right: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    padding_bottom: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    padding_left: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();

    border_top_width: f32 = 0.0;
    border_right_width: f32 = 0.0;
    border_bottom_width: f32 = 0.0;
    border_left_width: f32 = 0.0;
    border_color: CssColor = CssColor.rgb(0, 0, 0);
    has_explicit_border_color: bool = false;
    border_radius: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();

    top: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();
    right: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();
    bottom: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();
    left: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();

    flex_direction: FlexDirection = FlexDirection.Row;
    justify_content: JustifyContent = JustifyContent.FlexStart;
    align_items: AlignItems = AlignItems.Stretch;
    flex_grow: f32 = 0.0;
    flex_shrink: f32 = 1.0;
    flex_basis: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();

    row_gap: f32 = 0.0;
    column_gap: f32 = 0.0;
    grid_cols: u32 = 1;
    grid_col_span: u32 = 1;

    font_size_px: f32 = 16.0;
    line_height_px: f32 = 0.0;
    font_family_buf: [64]u8 = [0; 64],
    font_family_len: usize = 0,
    font_weight: f32 = 400.0,
    color: CssColor = CssColor.rgb(0, 0, 0);
    background_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    has_fill_current_color: bool = false,
    has_fill_none: bool = false,
    opacity: f32 = 1.0;
    z_index: i32 = 0;
    conic_gradient: ConicGradient = ConicGradient{};
    overflow_visible: bool = true;
    transform_tx: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    transform_ty: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();

    // 64-bit packed cascade weight per property slot
    weights: [TOTAL_PROP_SLOTS]u64 = [0; TOTAL_PROP_SLOTS];

    /// Initializes UA default display, margins, paddings, and typography per W3C HTML5 spec.
    fn initForNode(*_, node: *const NodeData, parent_style: (*const ComputedStyle)?) {
        // [hidden] attribute takes absolute precedence
        if (node.hasAttribute("hidden")) {
            _.display = Display.none();
            return;
        }

        const tag = HtmlTag.fromStr(node.tag_name);

        switch (tag) {
            .Head, .Style, .Script, .Meta, .Link, .Title, .Template,
            .Param, .Track, .Source, .Noscript, .Datalist => {
                _.display = Display.none();
            },
            .Dialog => {
                if (node.hasAttribute("open")) {
                    _.display = Display.block();
                } else {
                    _.display = Display.none();
                }
            },
            .A, .Abbr, .B, .Bdi, .Bdo, .Cite, .Code, .Data, .Dfn, .Em,
            .I, .Kbd, .Mark, .Q, .S, .Samp, .Small, .Span, .Strong,
            .Sub, .Sup, .Time, .U, .Var, .Wbr, .Label => {
                _.display = Display.inlineFlow();
            },
            .Button, .Input, .Select, .Textarea, .Img, .Video, .Audio,
            .Canvas, .Iframe, .Embed, .Object, .Progress, .Meter => {
                _.display = Display.inlineBlock();
            },
            .Table => {
                _.display = Display.block();
            },
            * => {
                _.display = Display.block();
            },
        }

        switch (tag) {
            .Body => {
                _.margin_top = LengthPercentageOrAuto.px(8.0);
                _.margin_right = LengthPercentageOrAuto.px(8.0);
                _.margin_bottom = LengthPercentageOrAuto.px(8.0);
                _.margin_left = LengthPercentageOrAuto.px(8.0);
            },
            .H1 => {
                _.font_size_px = 32.0;
                _.margin_top = LengthPercentageOrAuto.px(21.44);
                _.margin_bottom = LengthPercentageOrAuto.px(21.44);
            },
            .H2 => {
                _.font_size_px = 24.0;
                _.margin_top = LengthPercentageOrAuto.px(19.92);
                _.margin_bottom = LengthPercentageOrAuto.px(19.92);
            },
            .H3 => {
                _.font_size_px = 18.72;
                _.margin_top = LengthPercentageOrAuto.px(18.72);
                _.margin_bottom = LengthPercentageOrAuto.px(18.72);
            },
            .H4 => {
                _.font_size_px = 16.0;
                _.margin_top = LengthPercentageOrAuto.px(21.28);
                _.margin_bottom = LengthPercentageOrAuto.px(21.28);
            },
            .H5 => {
                _.font_size_px = 13.28;
                _.margin_top = LengthPercentageOrAuto.px(22.17);
                _.margin_bottom = LengthPercentageOrAuto.px(22.17);
            },
            .H6 => {
                _.font_size_px = 10.72;
                _.margin_top = LengthPercentageOrAuto.px(24.97);
                _.margin_bottom = LengthPercentageOrAuto.px(24.97);
            },
            .P => {
                _.margin_top = LengthPercentageOrAuto.px(16.0);
                _.margin_bottom = LengthPercentageOrAuto.px(16.0);
            },
            .Ul, .Ol => {
                _.margin_top = LengthPercentageOrAuto.px(16.0);
                _.margin_bottom = LengthPercentageOrAuto.px(16.0);
                _.padding_left = LengthPercentageOrAuto.px(40.0);
            },
            .Blockquote, .Figure => {
                _.margin_top = LengthPercentageOrAuto.px(16.0);
                _.margin_bottom = LengthPercentageOrAuto.px(16.0);
                _.margin_left = LengthPercentageOrAuto.px(40.0);
                _.margin_right = LengthPercentageOrAuto.px(40.0);
            },
            .Hr => {
                _.margin_top = LengthPercentageOrAuto.px(8.0);
                _.margin_bottom = LengthPercentageOrAuto.px(8.0);
                _.border_top_width = 1.0;
                _.border_color = CssColor.rgb(204, 204, 204);
                _.has_explicit_border_color = true;
            },
            .Button => {
                _.padding_top = LengthPercentageOrAuto.px(4.0);
                _.padding_right = LengthPercentageOrAuto.px(12.0);
                _.padding_bottom = LengthPercentageOrAuto.px(4.0);
                _.padding_left = LengthPercentageOrAuto.px(12.0);
                _.background_color = CssColor.rgb(239, 239, 239);
                _.border_top_width = 1.0;
                _.border_right_width = 1.0;
                _.border_bottom_width = 1.0;
                _.border_left_width = 1.0;
                _.border_color = CssColor.rgb(118, 118, 118);
                _.has_explicit_border_color = true;
            },
            .Input, .Textarea, .Select => {
                _.padding_top = LengthPercentageOrAuto.px(2.0);
                _.padding_right = LengthPercentageOrAuto.px(4.0);
                _.padding_bottom = LengthPercentageOrAuto.px(2.0);
                _.padding_left = LengthPercentageOrAuto.px(4.0);
                _.background_color = CssColor.rgb(255, 255, 255);
                _.border_top_width = 1.0;
                _.border_right_width = 1.0;
                _.border_bottom_width = 1.0;
                _.border_left_width = 1.0;
                _.border_color = CssColor.rgb(118, 118, 118);
                _.has_explicit_border_color = true;
            },
            * => {},
        }

        if (tag == .Svg || node.hasTag("svg")) {
            _.display = Display.inlineFlow();
            _.overflow_visible = false;
        }

        if (p := parent_style) {
            _.visibility = p.visibility;
            if (tag != .H1 && tag != .H2 && tag != .H3 && tag != .H4 && tag != .H5 && tag != .H6) {
                _.font_size_px = p.font_size_px;
            }
            _.color = p.color;
            if (p.has_fill_current_color) {
                _.has_fill_current_color = true;
                _.background_color = p.color;
            } else if (p.background_color.a > 0 && (tag == .Svg || node.hasTag("use") || node.hasTag("path") || node.hasTag("rect") || node.hasTag("circle") || node.hasTag("g") || node.hasTag("polygon"))) {
                _.background_color = p.background_color;
            }
            if (p.font_family_len > 0) {
                const copy_len = @min(p.font_family_len, _.font_family_buf.len);
                @memcpy(_.font_family_buf[0..copy_len], p.font_family_buf[0..copy_len]);
                _.font_family_len = copy_len;
            }
            if (tag != .B && tag != .Strong) {
                _.font_weight = p.font_weight;
            }
        }
        if (tag == .B || tag == .Strong) {
            _.font_weight = 700.0;
        }
    }

    fn getFontFamily(): []const u8 {
        return _.font_family_buf[0.._.font_family_len];
    }

    fn hasFontFamily(name: []const u8): bool {
        if (_.font_family_len == 0) return false;
        const ff = _.font_family_buf[0.._.font_family_len];
        if (std.mem.indexOf(u8, ff, name) != null) return true;
        let lower_ff_buf: [64]u8 = undefined;
        const copy_len = @min(ff.len, lower_ff_buf.len);
        let i: usize = 0;
        while (i < copy_len) : (i += 1) {
            lower_ff_buf[i] = std.ascii.toLower(ff[i]);
        }
        let lower_name_buf: [32]u8 = undefined;
        const name_len = @min(name.len, lower_name_buf.len);
        let j: usize = 0;
        while (j < name_len) : (j += 1) {
            lower_name_buf[j] = std.ascii.toLower(name[j]);
        }
        return std.mem.indexOf(u8, lower_ff_buf[0..copy_len], lower_name_buf[0..name_len]) != null;
    }

    fn tryClaimSlot(*_, slot: usize, weight: u64): bool {
        if (weight >= _.weights[slot]) {
            _.weights[slot] = weight;
            return true;
        }
        return false;
    }

    /// Extracts resolved visual paint parameters for software rasterization (`painter.cp`).
    fn extractPaintStyle(
        box_width: f32,
        root_font_px: f32,
        viewport_w: f32,
        viewport_h: f32,
    ): ResolvedPaintStyle {
        let bg = if (_.has_fill_current_color) _.color else _.background_color;
        let txt = _.color;
        // Per CSS spec, if border-color is not explicitly set, it defaults to `currentColor` (`_.color`)
        let bdr = if (_.has_explicit_border_color) _.border_color else _.color;

        const op = std.math.clamp(_.opacity, 0.0, 1.0);
        let cg = _.conic_gradient;
        if (op < 1.0) {
            bg.a = @intFromFloat(@as(f32, @floatFromInt(bg.a)) * op);
            txt.a = @intFromFloat(@as(f32, @floatFromInt(txt.a)) * op);
            bdr.a = @intFromFloat(@as(f32, @floatFromInt(bdr.a)) * op);
            if (cg.has_gradient) {
                let si: usize = 0;
                while (si < cg.stop_count) : (si += 1) {
                    cg.stops[si].color.a = @intFromFloat(@as(f32, @floatFromInt(cg.stops[si].color.a)) * op);
                }
            }
        }

        const radius_px = _.border_radius.resolvePx(box_width, _.font_size_px, root_font_px, viewport_w, viewport_h);
        const bw_px = @max(@max(_.border_top_width, _.border_right_width), @max(_.border_bottom_width, _.border_left_width));

        return ResolvedPaintStyle {
            bg_color: bg,
            border_color: bdr,
            text_color: txt,
            border_width: bw_px,
            border_radius: radius_px,
            font_size_px: _.font_size_px,
            conic_gradient: cg,
        };
    }

    /// Applies a single `CssProperty` with prepacked 64-bit `weight` onto this ComputedStyle.
    fn applyProperty(*_, prop: *const CssProperty, weight: u64, root_font_px: f32) {
        const name = canonicalizeCssPropertyName(prop.name);
        const val = prop.value;

        if (std.ascii.eqlIgnoreCase(name, "display")) {
            if (d := Display.parse(val)) {
                if (_.tryClaimSlot(PROP_DISPLAY, weight)) {
                    _.display = d;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "visibility")) {
            if (v := Visibility.parse(val)) {
                if (_.tryClaimSlot(PROP_VISIBILITY, weight)) {
                    _.visibility = v;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "box-sizing")) {
            if (bs := BoxSizing.parse(val)) {
                if (_.tryClaimSlot(PROP_BOX_SIZING, weight)) {
                    _.box_sizing = bs;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "position")) {
            if (pos := Position.parse(val)) {
                if (_.tryClaimSlot(PROP_POSITION, weight)) {
                    _.position = pos;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "width")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_WIDTH, weight)) {
                    _.width = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "height")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_HEIGHT, weight)) {
                    _.height = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "min-width")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MIN_WIDTH, weight)) {
                    _.min_width = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "min-height")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MIN_HEIGHT, weight)) {
                    _.min_height = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "max-width")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MAX_WIDTH, weight)) {
                    _.max_width = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "max-height")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MAX_HEIGHT, weight)) {
                    _.max_height = lpa;
                }
            }
            return;
        }

        // Margin shorthand & longhands
        if (std.ascii.eqlIgnoreCase(name, "margin")) {
            if (quad := Cascade.parseBoxQuad(val)) {
                if (_.tryClaimSlot(PROP_MARGIN_TOP, weight)) _.margin_top = quad.top;
                if (_.tryClaimSlot(PROP_MARGIN_RIGHT, weight)) _.margin_right = quad.right;
                if (_.tryClaimSlot(PROP_MARGIN_BOTTOM, weight)) _.margin_bottom = quad.bottom;
                if (_.tryClaimSlot(PROP_MARGIN_LEFT, weight)) _.margin_left = quad.left;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "margin-top")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MARGIN_TOP, weight)) _.margin_top = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "margin-right")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MARGIN_RIGHT, weight)) _.margin_right = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "margin-bottom")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MARGIN_BOTTOM, weight)) _.margin_bottom = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "margin-left")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MARGIN_LEFT, weight)) _.margin_left = lpa;
            }
            return;
        }

        // Padding shorthand & longhands
        if (std.ascii.eqlIgnoreCase(name, "padding")) {
            if (quad := Cascade.parseBoxQuad(val)) {
                if (_.tryClaimSlot(PROP_PADDING_TOP, weight)) _.padding_top = quad.top;
                if (_.tryClaimSlot(PROP_PADDING_RIGHT, weight)) _.padding_right = quad.right;
                if (_.tryClaimSlot(PROP_PADDING_BOTTOM, weight)) _.padding_bottom = quad.bottom;
                if (_.tryClaimSlot(PROP_PADDING_LEFT, weight)) _.padding_left = quad.left;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "padding-top")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_PADDING_TOP, weight)) _.padding_top = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "padding-right")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_PADDING_RIGHT, weight)) _.padding_right = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "padding-bottom")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_PADDING_BOTTOM, weight)) _.padding_bottom = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "padding-left")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_PADDING_LEFT, weight)) _.padding_left = lpa;
            }
            return;
        }

        // Border width & color shorthand & longhands
        if (std.ascii.eqlIgnoreCase(name, "border") || std.ascii.eqlIgnoreCase(name, "border-width")) {
            const bw = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_BORDER_TOP, weight)) _.border_top_width = bw;
            if (_.tryClaimSlot(PROP_BORDER_RIGHT, weight)) _.border_right_width = bw;
            if (_.tryClaimSlot(PROP_BORDER_BOTTOM, weight)) _.border_bottom_width = bw;
            if (_.tryClaimSlot(PROP_BORDER_LEFT, weight)) _.border_left_width = bw;
            if (std.ascii.eqlIgnoreCase(name, "border")) {
                if (c := Cascade.parseColorTokenFromShorthand(val)) {
                    if (_.tryClaimSlot(PROP_BORDER_COLOR, weight)) {
                        _.border_color = c;
                        _.has_explicit_border_color = true;
                    }
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-top") || std.ascii.eqlIgnoreCase(name, "border-top-width")) {
            const bw = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_BORDER_TOP, weight)) _.border_top_width = bw;
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BORDER_COLOR, weight)) {
                    _.border_color = c;
                    _.has_explicit_border_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-right") || std.ascii.eqlIgnoreCase(name, "border-right-width")) {
            const bw = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_BORDER_RIGHT, weight)) _.border_right_width = bw;
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-bottom") || std.ascii.eqlIgnoreCase(name, "border-bottom-width")) {
            const bw = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_BORDER_BOTTOM, weight)) _.border_bottom_width = bw;
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-left") || std.ascii.eqlIgnoreCase(name, "border-left-width")) {
            const bw = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_BORDER_LEFT, weight)) _.border_left_width = bw;
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-color") || std.ascii.eqlIgnoreCase(name, "border-top-color")) {
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BORDER_COLOR, weight)) {
                    _.border_color = c;
                    _.has_explicit_border_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-radius") || std.ascii.eqlIgnoreCase(name, "border-top-left-radius")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n/");
            if (first_tok := it.next()) {
                if (lpa := LengthPercentageOrAuto.parse(first_tok)) {
                    if (_.tryClaimSlot(PROP_BORDER_RADIUS, weight)) {
                        _.border_radius = lpa;
                    }
                }
            }
            return;
        }

        // Inset / Top / Right / Bottom / Left
        if (std.ascii.eqlIgnoreCase(name, "inset")) {
            if (quad := Cascade.parseBoxQuad(val)) {
                if (_.tryClaimSlot(PROP_TOP, weight)) _.top = quad.top;
                if (_.tryClaimSlot(PROP_RIGHT, weight)) _.right = quad.right;
                if (_.tryClaimSlot(PROP_BOTTOM, weight)) _.bottom = quad.bottom;
                if (_.tryClaimSlot(PROP_LEFT, weight)) _.left = quad.left;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "top")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_TOP, weight)) _.top = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "right")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_RIGHT, weight)) _.right = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "bottom")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_BOTTOM, weight)) _.bottom = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "left")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_LEFT, weight)) _.left = lpa;
            }
            return;
        }

        // Flexbox properties
        if (std.ascii.eqlIgnoreCase(name, "flex-direction")) {
            if (fd := FlexDirection.parse(val)) {
                if (_.tryClaimSlot(PROP_FLEX_DIRECTION, weight)) _.flex_direction = fd;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "justify-content")) {
            if (jc := JustifyContent.parse(val)) {
                if (_.tryClaimSlot(PROP_JUSTIFY_CONTENT, weight)) _.justify_content = jc;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "align-items")) {
            if (ai := AlignItems.parse(val)) {
                if (_.tryClaimSlot(PROP_ALIGN_ITEMS, weight)) _.align_items = ai;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "flex-grow")) {
            const fg = std.fmt.parseFloat(f32, std.mem.trim(u8, val, " \t")) catch 0.0;
            if (_.tryClaimSlot(PROP_FLEX_GROW, weight)) _.flex_grow = fg;
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "flex-shrink")) {
            const fs = std.fmt.parseFloat(f32, std.mem.trim(u8, val, " \t")) catch 1.0;
            if (_.tryClaimSlot(PROP_FLEX_SHRINK, weight)) _.flex_shrink = fs;
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "flex-basis")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_FLEX_BASIS, weight)) _.flex_basis = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "flex")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n");
            if (first_tok := it.next()) {
                if (std.ascii.eqlIgnoreCase(first_tok, "none")) {
                    if (_.tryClaimSlot(PROP_FLEX_GROW, weight)) _.flex_grow = 0.0;
                    if (_.tryClaimSlot(PROP_FLEX_SHRINK, weight)) _.flex_shrink = 0.0;
                    if (_.tryClaimSlot(PROP_FLEX_BASIS, weight)) _.flex_basis = LengthPercentageOrAuto.autoVal();
                    return;
                }
                const fg = std.fmt.parseFloat(f32, first_tok) catch -1.0;
                if (fg >= 0.0) {
                    if (_.tryClaimSlot(PROP_FLEX_GROW, weight)) _.flex_grow = fg;
                    if (second_tok := it.next()) {
                        const fs = std.fmt.parseFloat(f32, second_tok) catch -1.0;
                        if (fs >= 0.0) {
                            if (_.tryClaimSlot(PROP_FLEX_SHRINK, weight)) _.flex_shrink = fs;
                            if (third_tok := it.next()) {
                                if (lpa := LengthPercentageOrAuto.parse(third_tok)) {
                                    if (_.tryClaimSlot(PROP_FLEX_BASIS, weight)) _.flex_basis = lpa;
                                }
                            }
                        } else if (lpa := LengthPercentageOrAuto.parse(second_tok)) {
                            if (_.tryClaimSlot(PROP_FLEX_BASIS, weight)) _.flex_basis = lpa;
                        }
                    } else {
                        if (_.tryClaimSlot(PROP_FLEX_SHRINK, weight)) _.flex_shrink = 1.0;
                        if (_.tryClaimSlot(PROP_FLEX_BASIS, weight)) _.flex_basis = LengthPercentageOrAuto.zeroPx();
                    }
                }
            }
            return;
        }

        // Gap / Row-Gap / Column-Gap
        if (std.ascii.eqlIgnoreCase(name, "gap")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n");
            if (t1 := it.next()) {
                if (l1 := CssLength.parse(t1)) {
                    const g1: f32 = @floatCast(l1.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                    let g2: f32 = g1;
                    if (t2 := it.next()) {
                        if (l2 := CssLength.parse(t2)) {
                            const g2_px: f32 = @floatCast(l2.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                            g2 = g2_px;
                        }
                    }
                    if (_.tryClaimSlot(PROP_ROW_GAP, weight)) _.row_gap = g1;
                    if (_.tryClaimSlot(PROP_COLUMN_GAP, weight)) _.column_gap = g2;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "row-gap")) {
            if (l := CssLength.parse(val)) {
                const g: f32 = @floatCast(l.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                if (_.tryClaimSlot(PROP_ROW_GAP, weight)) _.row_gap = g;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "column-gap")) {
            if (l := CssLength.parse(val)) {
                const g: f32 = @floatCast(l.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                if (_.tryClaimSlot(PROP_COLUMN_GAP, weight)) _.column_gap = g;
            }
            return;
        }

        // Grid template columns (e.g. `repeat(3, 1fr)` or `100px 200px`)
        if (std.ascii.eqlIgnoreCase(name, "grid-template-columns")) {
            const trimmed = std.mem.trim(u8, val, " \t\r\n");
            if (std.ascii.startsWithIgnoreCase(trimmed, "repeat(")) {
                if (comma_idx := std.mem.indexOfScalar(u8, trimmed, ',')) {
                    const cnt_str = std.mem.trim(u8, trimmed[7..comma_idx], " \t");
                    const cols = std.fmt.parseInt(u32, cnt_str, 10) catch 1;
                    if (_.tryClaimSlot(PROP_GRID_COLS, weight)) {
                        _.grid_cols = if (cols > 0) cols else 1;
                    }
                }
            } else {
                let cols: u32 = 0;
                let it = std.mem.tokenizeAny(u8, trimmed, " \t\r\n");
                while (it.next() != null) {
                    cols += 1;
                }
                if (cols > 0 && _.tryClaimSlot(PROP_GRID_COLS, weight)) {
                    _.grid_cols = cols;
                }
            }
            return;
        }

        // Grid column placement (e.g. `grid-column: span 6` or `grid-column: 1 / 7`)
        if (std.ascii.eqlIgnoreCase(name, "grid-column") || std.ascii.eqlIgnoreCase(name, "grid-column-start") || std.ascii.eqlIgnoreCase(name, "grid-column-end")) {
            const trimmed = std.mem.trim(u8, val, " \t\r\n");
            if (span_idx := std.mem.indexOf(u8, trimmed, "span")) {
                let pos = span_idx + 4;
                while (pos < trimmed.len && (trimmed[pos] == ' ' || trimmed[pos] == '\t')) : (pos += 1) {}
                const num_start = pos;
                while (pos < trimmed.len && std.ascii.isDigit(trimmed[pos])) : (pos += 1) {}
                if (pos > num_start) {
                    const span_cnt = std.fmt.parseInt(u32, trimmed[num_start..pos], 10) catch 1;
                    if (_.tryClaimSlot(PROP_GRID_COL_SPAN, weight)) {
                        _.grid_col_span = if (span_cnt > 0) span_cnt else 1;
                    }
                }
            } else if (slash_idx := std.mem.indexOfScalar(u8, trimmed, '/')) {
                const s1 = std.mem.trim(u8, trimmed[0..slash_idx], " \t");
                const s2 = std.mem.trim(u8, trimmed[slash_idx + 1 ..], " \t");
                const n1 = std.fmt.parseInt(u32, s1, 10) catch 0;
                const n2 = std.fmt.parseInt(u32, s2, 10) catch 0;
                if (n2 > n1 && n1 > 0) {
                    if (_.tryClaimSlot(PROP_GRID_COL_SPAN, weight)) {
                        _.grid_col_span = n2 - n1;
                    }
                }
            }
            return;
        }

        // Typography & Visuals
        if (std.ascii.eqlIgnoreCase(name, "font-family")) {
            if (_.tryClaimSlot(PROP_FONT_FAMILY, weight)) {
                const copy_len = @min(val.len, _.font_family_buf.len);
                @memcpy(_.font_family_buf[0..copy_len], val[0..copy_len]);
                _.font_family_len = copy_len;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "font-weight")) {
            let fw: f32 = 400.0;
            if (std.ascii.eqlIgnoreCase(val, "bold")) {
                fw = 700.0;
            } else if (std.ascii.eqlIgnoreCase(val, "normal")) {
                fw = 400.0;
            } else if (std.ascii.eqlIgnoreCase(val, "lighter")) {
                fw = 300.0;
            } else if (std.ascii.eqlIgnoreCase(val, "bolder")) {
                fw = 700.0;
            } else {
                const trimmed = std.mem.trim(u8, val, " \t\r\n");
                if (p_val := std.fmt.parseFloat(f32, trimmed) catch null) {
                    fw = p_val;
                }
            }
            if (_.tryClaimSlot(PROP_FONT_WEIGHT, weight)) {
                _.font_weight = fw;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "font-size")) {
            if (l := CssLength.parse(val)) {
                const fs: f32 = @floatCast(l.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                if (_.tryClaimSlot(PROP_FONT_SIZE, weight)) _.font_size_px = fs;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "line-height")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_LINE_HEIGHT, weight)) {
                    _.line_height_px = lpa.resolvePx(root_font_px, _.font_size_px, root_font_px, 1280.0, 800.0);
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "color")) {
            if (c := CssColor.parse(val)) {
                if (_.tryClaimSlot(PROP_COLOR, weight)) _.color = c;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "background-image")) {
            if (cg := ConicGradient.parse(val)) {
                if (_.tryClaimSlot(PROP_BG_IMAGE, weight)) {
                    _.conic_gradient = cg;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "fill")) {
            const fv = std.mem.trim(u8, val, " \t\r\n");
            if (std.ascii.eqlIgnoreCase(fv, "none")) {
                if (_.tryClaimSlot(PROP_BG_COLOR, weight)) {
                    _.background_color = CssColor.rgba(0, 0, 0, 0);
                    _.has_fill_current_color = false;
                    _.has_fill_none = true;
                }
            } else if (std.ascii.eqlIgnoreCase(fv, "currentcolor")) {
                if (_.tryClaimSlot(PROP_BG_COLOR, weight)) {
                    _.background_color = _.color;
                    _.has_fill_current_color = true;
                    _.has_fill_none = false;
                }
            } else if (c_opt := CssColor.parse(fv)) {
                if (_.tryClaimSlot(PROP_BG_COLOR, weight)) {
                    _.background_color = c_opt;
                    _.has_fill_current_color = false;
                    _.has_fill_none = false;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "background-color") || std.ascii.eqlIgnoreCase(name, "background")) {
            if (cg := ConicGradient.parse(val)) {
                if (_.tryClaimSlot(PROP_BG_IMAGE, weight)) {
                    _.conic_gradient = cg;
                }
            }
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BG_COLOR, weight)) _.background_color = c;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "opacity")) {
            const op = std.fmt.parseFloat(f32, std.mem.trim(u8, val, " \t")) catch 1.0;
            if (_.tryClaimSlot(PROP_OPACITY, weight)) _.opacity = op;
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "z-index")) {
            const z = std.fmt.parseInt(i32, std.mem.trim(u8, val, " \t"), 10) catch 0;
            if (_.tryClaimSlot(PROP_Z_INDEX, weight)) _.z_index = z;
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "overflow") || std.ascii.eqlIgnoreCase(name, "overflow-x") || std.ascii.eqlIgnoreCase(name, "overflow-y")) {
            if (_.tryClaimSlot(PROP_OVERFLOW, weight)) {
                const v = std.mem.trim(u8, val, " \t\r\n");
                if (std.ascii.eqlIgnoreCase(v, "visible")) {
                    _.overflow_visible = true;
                } else if (std.ascii.eqlIgnoreCase(v, "hidden") || std.ascii.eqlIgnoreCase(v, "clip") || std.ascii.eqlIgnoreCase(v, "auto") || std.ascii.eqlIgnoreCase(v, "scroll")) {
                    _.overflow_visible = false;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "transform") || std.ascii.eqlIgnoreCase(name, "-webkit-transform")) {
            if (_.tryClaimSlot(PROP_TRANSFORM, weight)) {
                _.parseTransform(val);
            }
            return;
        }
    }

    fn parseTransform(*_, val: []const u8) {
        const s = std.mem.trim(u8, val, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "none") || s.len == 0) {
            _.transform_tx = LengthPercentageOrAuto.zeroPx();
            _.transform_ty = LengthPercentageOrAuto.zeroPx();
            return;
        }
        if (open_idx := std.mem.indexOfScalar(u8, s, '(')) {
            if (close_idx := std.mem.lastIndexOfScalar(u8, s, ')')) {
                if (close_idx > open_idx) {
                    const fn_name = std.mem.trim(u8, s[0..open_idx], " \t\r\n");
                    const args = s[open_idx + 1 .. close_idx];
                    let it = std.mem.splitScalar(u8, args, ',');
                    if (std.ascii.eqlIgnoreCase(fn_name, "translate3d") || std.ascii.eqlIgnoreCase(fn_name, "translate")) {
                        if (x_str := it.next()) {
                            if (p1 := LengthPercentageOrAuto.parse(x_str)) _.transform_tx = p1;
                        }
                        if (y_str := it.next()) {
                            if (p2 := LengthPercentageOrAuto.parse(y_str)) _.transform_ty = p2;
                        }
                    } else if (std.ascii.eqlIgnoreCase(fn_name, "translateX")) {
                        if (x_str := it.next()) {
                            if (p1 := LengthPercentageOrAuto.parse(x_str)) _.transform_tx = p1;
                        }
                    } else if (std.ascii.eqlIgnoreCase(fn_name, "translateY")) {
                        if (y_str := it.next()) {
                            if (p2 := LengthPercentageOrAuto.parse(y_str)) _.transform_ty = p2;
                        }
                    }
                }
            }
        }
    }
}

pub struct StyleRuleEntry {
    selectors: SelectorList = SelectorList{};
    // Separated normal declarations and !important declarations per LightningCSS DeclarationBlock
    declarations: [24]CssProperty = [CssProperty{}; 24];
    decl_count: usize = 0;
    important_declarations: [16]CssProperty = [CssProperty{}; 16];
    important_count: usize = 0;
}

pub struct StyleSheet {
    rules: [64]StyleRuleEntry = [StyleRuleEntry{}; 64];
    rule_count: usize = 0;

    static fn parse(css_text: []const u8, viewport_w: f32, is_dark: bool): StyleSheet {
        let sheet = StyleSheet{};
        let raw_rules: [64]CssRule = [CssRule{}; 64];
        const parsed_count = CssStylesheetParser.parseStylesheet(css_text, raw_rules[0..64]);

        let i: usize = 0;
        while (i < parsed_count && sheet.rule_count < 64) : (i += 1) {
            const r = &raw_rules[i];
            if (r.kind == CssRuleKind.Style) {
                StyleSheet.appendStyleRule(&sheet, r);
            } else if (r.kind == CssRuleKind.Media) {
                if (r.media.matches(@as(f64, viewport_w), is_dark)) {
                    let nested_rules: [32]CssRule = [CssRule{}; 32];
                    const n_cnt = CssStylesheetParser.parseStylesheet(r.nested_body, nested_rules[0..32]);
                    let j: usize = 0;
                    while (j < n_cnt && sheet.rule_count < 64) : (j += 1) {
                        if (nested_rules[j].kind == CssRuleKind.Style) {
                            StyleSheet.appendStyleRule(&sheet, &nested_rules[j]);
                        }
                    }
                }
            }
        }
        return sheet;
    }

    static fn appendStyleRule(sheet: *StyleSheet, r: *const CssRule) {
        if (sheet.rule_count >= 64) return;
        let entry = StyleRuleEntry {
            selectors: SelectorList.parse(r.prelude),
        };
        let p_i: usize = 0;
        while (p_i < r.prop_count) : (p_i += 1) {
            const prop = r.properties[p_i];
            if (prop.important) {
                if (entry.important_count < 16) {
                    entry.important_declarations[entry.important_count] = prop;
                    entry.important_count += 1;
                }
            } else {
                if (entry.decl_count < 24) {
                    entry.declarations[entry.decl_count] = prop;
                    entry.decl_count += 1;
                }
            }
        }
        sheet.rules[sheet.rule_count] = entry;
        sheet.rule_count += 1;
    }
}

pub struct Cascade {
    /// Packs `important`, `is_inline`, 30-bit `specificity`, and 32-bit `source_order` into a single `u64` register:
    /// `weight = (important << 63) | (is_inline << 62) | (specificity_30bit << 32) | source_order`
    static fn packWeight(important: bool, is_inline: bool, specificity: u32, source_order: u32): u64 {
        const imp_bit: u64 = if (important) (@as(u64, 1) << @as(u6, 63)) else 0;
        const inl_bit: u64 = if (is_inline) (@as(u64, 1) << @as(u6, 62)) else 0;
        const spec_bits: u64 = (@as(u64, specificity & 0x3FFFFFFF) << @as(u6, 32));
        return imp_bit | inl_bit | spec_bits | @as(u64, source_order);
    }

    /// Expands 1..4 space-separated box values (`margin: 10px 20px`) into `BoxQuad`.
    static fn parseBoxQuad(val_str: []const u8): BoxQuad? {
        let items: [4]LengthPercentageOrAuto = [LengthPercentageOrAuto.zeroPx(); 4];
        let count: usize = 0;
        let i: usize = 0;
        const s = std.mem.trim(u8, val_str, " \t\r\n");
        while (i < s.len && count < 4) {
            while (i < s.len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) : (i += 1) {}
            if (i >= s.len) break;
            const start = i;
            let depth: i32 = 0;
            while (i < s.len) : (i += 1) {
                const c = s[i];
                if (c == '(') {
                    depth += 1;
                } else if (c == ')') {
                    if (depth > 0) depth -= 1;
                } else if (depth == 0 && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
                    break;
                }
            }
            const tok = s[start..i];
            if (tok.len > 0) {
                if (parsed := LengthPercentageOrAuto.parse(tok)) {
                    items[count] = parsed;
                    count += 1;
                } else {
                    return null;
                }
            }
        }
        if (count == 0) return null;
        if (count == 1) {
            return BoxQuad{ top: items[0], right: items[0], bottom: items[0], left: items[0] };
        }
        if (count == 2) {
            return BoxQuad{ top: items[0], right: items[1], bottom: items[0], left: items[1] };
        }
        if (count == 3) {
            return BoxQuad{ top: items[0], right: items[1], bottom: items[2], left: items[1] };
        }
        return BoxQuad{ top: items[0], right: items[1], bottom: items[2], left: items[3] };
    }

    static fn parseBorderWidthToken(val_str: []const u8, root_font_px: f32): f32 {
        let it = std.mem.tokenizeAny(u8, val_str, " \t\r\n");
        while (tok := it.next()) {
            if (std.ascii.eqlIgnoreCase(tok, "none") || std.ascii.eqlIgnoreCase(tok, "hidden")) {
                return 0.0;
            }
            if (std.ascii.eqlIgnoreCase(tok, "thin")) return 1.0;
            if (std.ascii.eqlIgnoreCase(tok, "medium")) return 3.0;
            if (std.ascii.eqlIgnoreCase(tok, "thick")) return 5.0;
            if (len_v := CssLength.parse(tok)) {
                return @floatCast(len_v.toPx(@as(f64, root_font_px), 1280.0, 800.0));
            }
        }
        return 0.0;
    }

    /// Parses a color value either directly (`#2563eb`, `rgb(1, 2, 3)`) or from a shorthand (`1px solid #2563eb`).
    static fn parseColorTokenFromShorthand(val_str: []const u8): CssColor? {
        const s = std.mem.trim(u8, val_str, " \t\r\n");
        if (direct := CssColor.parse(s)) {
            return direct;
        }
        // Check for `rgb(...)` / `rgba(...)` / `hsl(...)` functional notation inside shorthand
        if (paren_open := std.mem.indexOfScalar(u8, s, '(')) {
            if (paren_close := std.mem.indexOfScalarPos(u8, s, paren_open, ')')) {
                let start = paren_open;
                while (start > 0 && s[start - 1] != ' ' && s[start - 1] != '\t') : (start -= 1) {}
                if (func_color := CssColor.parse(s[start .. paren_close + 1])) {
                    return func_color;
                }
            }
        }
        let it = std.mem.tokenizeAny(u8, s, " \t\r\n");
        while (tok := it.next()) {
            if (std.ascii.eqlIgnoreCase(tok, "solid") ||
                std.ascii.eqlIgnoreCase(tok, "dashed") ||
                std.ascii.eqlIgnoreCase(tok, "dotted") ||
                std.ascii.eqlIgnoreCase(tok, "double") ||
                std.ascii.eqlIgnoreCase(tok, "none") ||
                std.ascii.eqlIgnoreCase(tok, "hidden") ||
                std.ascii.eqlIgnoreCase(tok, "thin") ||
                std.ascii.eqlIgnoreCase(tok, "medium") ||
                std.ascii.eqlIgnoreCase(tok, "thick")) {
                continue;
            }
            if (c := CssColor.parse(tok)) {
                return c;
            }
        }
        return null;
    }

    /// Forwarder to ComputedStyle.applyProperty
    static fn applyProperty(style: *ComputedStyle, prop: *const CssProperty, weight: u64, root_font_px: f32) {
        style.applyProperty(prop, weight, root_font_px);
    }

    /// Computes `ComputedStyle` for every node in `doc` into `out_styles[0..doc.nodeCount()]`
    /// in a single topological pass (`parent_id < node_id`), resolving stylesheet rules,
    /// inline `style="..."` attributes, `!important` flags, and CSS inheritance.
    static fn computeStyles(
        doc: *const Document,
        sheet: *const StyleSheet,
        out_styles: []ComputedStyle,
        root_font_px: f32,
    ) {
        const total = doc.nodes.items.len;
        if (out_styles.len == 0) return;
        out_styles[0] = ComputedStyle{};

        let nid: usize = 1;
        while (nid < total && nid < out_styles.len) : (nid += 1) {
            const node = &doc.nodes.items[nid];
            let style = ComputedStyle{};

            if (node.node_type != NodeType.Element) {
                out_styles[nid] = style;
                continue;
            }

            let parent_style_ptr: (*const ComputedStyle)? = null;
            if (node.parent != NULL_NODE && @as(usize, node.parent) < nid) {
                parent_style_ptr = &out_styles[@as(usize, node.parent)];
            }
            style.initForNode(node, parent_style_ptr);

            let source_order: u32 = 1;
            const node_id_u32: NodeId = @intCast(nid);

            // 0. Presentation attributes for SVG (e.g. fill)
            if (fill_attr := node.getAttribute("fill")) {
                const prop = CssProperty{
                    name: "fill",
                    value: fill_attr,
                    important: false,
                };
                const w = Cascade.packWeight(false, false, 0, 0);
                style.applyProperty(&prop, w, root_font_px);
            }

            // 1. Match stylesheet rules in source order
            let r_i: usize = 0;
            while (r_i < sheet.rule_count) : (r_i += 1) {
                const rule = &sheet.rules[r_i];
                if (best_spec := Matcher.matchBestSpecificity(doc, node_id_u32, &rule.selectors)) {
                    // Normal declarations
                    let d_i: usize = 0;
                    while (d_i < rule.decl_count) : (d_i += 1) {
                        const w = Cascade.packWeight(false, false, best_spec, source_order);
                        source_order += 1;
                        style.applyProperty(&rule.declarations[d_i], w, root_font_px);
                    }
                    // !important declarations
                    let imp_i: usize = 0;
                    while (imp_i < rule.important_count) : (imp_i += 1) {
                        const w = Cascade.packWeight(true, false, best_spec, source_order);
                        source_order += 1;
                        style.applyProperty(&rule.important_declarations[imp_i], w, root_font_px);
                    }
                }
            }

            // 2. Apply inline `style="..."` declarations (`is_inline = true`, bit 62 set)
            if (node.inline_style.len > 0) {
                let it = std.mem.splitScalar(u8, node.inline_style, ';');
                while (raw_decl := it.next()) {
                    if (prop := CssProperty.parse(raw_decl)) {
                        const w = Cascade.packWeight(prop.important, true, 0, source_order);
                        source_order += 1;
                        style.applyProperty(&prop, w, root_font_px);
                    }
                }
            }

            out_styles[nid] = style;
        }
    }
}
