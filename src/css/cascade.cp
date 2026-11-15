// src/css/cascade.cp
// CapSurf 64-Bit Bit-Packed Weight CSS Cascade & ComputedStyle Engine (Milestone 1)
//
// Packed 64-bit Weight Formula:
//   weight: u64 = (important << 63) | (is_inline << 62) | (specificity_30bit << 32) | source_order_32bit
// Resolves !important, inline style, W3C Selectors Level 4 specificity, and source order
// in a single register comparison: `if (new_weight >= existing_weight)`.

import { Document, NodeData, NodeType, NodeId, NULL_NODE } from "../dom/dom.cp";
import { SelectorList, Matcher } from "./matcher.cp";
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

    static fn parse(raw: []const u8): Position? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "static")) return Position.Static;
        if (std.ascii.eqlIgnoreCase(s, "relative")) return Position.Relative;
        if (std.ascii.eqlIgnoreCase(s, "absolute")) return Position.Absolute;
        if (std.ascii.eqlIgnoreCase(s, "fixed") || std.ascii.eqlIgnoreCase(s, "sticky")) return Position.Fixed;
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
            if (v := CssCalc.evalPx(_.calc_expr, @as(f64, root_font_px), @as(f64, viewport_w), @as(f64, viewport_h))) {
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
pub const TOTAL_PROP_SLOTS: usize = 40;

pub struct BoxQuad {
    top: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    right: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    bottom: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    left: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
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

    font_size_px: f32 = 16.0;
    color: CssColor = CssColor.rgb(0, 0, 0);
    background_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    opacity: f32 = 1.0;
    z_index: i32 = 0;

    // 64-bit packed cascade weight per property slot
    weights: [40]u64 = [0; 40];

    /// Initializes UA default display and inherits inheritable properties (`visibility`, `font-size`, `color`) from parent.
    fn initForNode(*_, tag_name: []const u8, parent_style: (*const ComputedStyle)?) {
        // Inline-level HTML tags default to `display: inline`, `<button>` / `<input>` to `inline-block`
        if (std.ascii.eqlIgnoreCase(tag_name, "span") ||
            std.ascii.eqlIgnoreCase(tag_name, "a") ||
            std.ascii.eqlIgnoreCase(tag_name, "strong") ||
            std.ascii.eqlIgnoreCase(tag_name, "em") ||
            std.ascii.eqlIgnoreCase(tag_name, "b") ||
            std.ascii.eqlIgnoreCase(tag_name, "i") ||
            std.ascii.eqlIgnoreCase(tag_name, "label"))
        {
            _.display = Display.inlineFlow();
        } else if (std.ascii.eqlIgnoreCase(tag_name, "button") ||
            std.ascii.eqlIgnoreCase(tag_name, "input") ||
            std.ascii.eqlIgnoreCase(tag_name, "select"))
        {
            _.display = Display.inlineBlock();
        } else if (std.ascii.eqlIgnoreCase(tag_name, "head") ||
            std.ascii.eqlIgnoreCase(tag_name, "style") ||
            std.ascii.eqlIgnoreCase(tag_name, "script") ||
            std.ascii.eqlIgnoreCase(tag_name, "meta") ||
            std.ascii.eqlIgnoreCase(tag_name, "title") ||
            std.ascii.eqlIgnoreCase(tag_name, "template"))
        {
            _.display = Display.none();
        } else {
            _.display = Display.block();
        }

        if (p := parent_style) {
            _.visibility = p.visibility;
            _.font_size_px = p.font_size_px;
            _.color = p.color;
        }
    }

    fn tryClaimSlot(*_, slot: usize, weight: u64): bool {
        if (weight >= _.weights[slot]) {
            _.weights[slot] = weight;
            return true;
        }
        return false;
    }

    /// Applies a single `CssProperty` with prepacked 64-bit `weight` onto this ComputedStyle.
    fn applyProperty(*_, prop: *const CssProperty, weight: u64, root_font_px: f32) {
        const name = prop.name;
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

        // Border width shorthand & longhands
        if (std.ascii.eqlIgnoreCase(name, "border") || std.ascii.eqlIgnoreCase(name, "border-width")) {
            const bw = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_BORDER_TOP, weight)) _.border_top_width = bw;
            if (_.tryClaimSlot(PROP_BORDER_RIGHT, weight)) _.border_right_width = bw;
            if (_.tryClaimSlot(PROP_BORDER_BOTTOM, weight)) _.border_bottom_width = bw;
            if (_.tryClaimSlot(PROP_BORDER_LEFT, weight)) _.border_left_width = bw;
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-top") || std.ascii.eqlIgnoreCase(name, "border-top-width")) {
            const bw = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_BORDER_TOP, weight)) _.border_top_width = bw;
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

        // Typography & Visuals
        if (std.ascii.eqlIgnoreCase(name, "font-size")) {
            if (l := CssLength.parse(val)) {
                const fs: f32 = @floatCast(l.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                if (_.tryClaimSlot(PROP_FONT_SIZE, weight)) _.font_size_px = fs;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "color")) {
            if (c := CssColor.parse(val)) {
                if (_.tryClaimSlot(PROP_COLOR, weight)) _.color = c;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "background-color") || std.ascii.eqlIgnoreCase(name, "background")) {
            if (c := CssColor.parse(val)) {
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
        let it = std.mem.tokenizeAny(u8, val_str, " \t\r\n");
        while (tok := it.next()) {
            if (count >= 4) break;
            if (parsed := LengthPercentageOrAuto.parse(tok)) {
                items[count] = parsed;
                count += 1;
            } else {
                return null;
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
            style.initForNode(node.tag_name, parent_style_ptr);

            let source_order: u32 = 1;
            const node_id_u32: NodeId = @intCast(nid);

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
