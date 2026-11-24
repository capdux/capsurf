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
import { UnicodeRange, CssFontFaceRule, WebfontStyleMetrics, parsePseudoContentValue, CssCounterStack, hasDynamicPseudoContent, resolvePseudoContentExpr } from "./font_face.cp";
import { parseAttrExpr, evaluateCssAttr } from "./attr_evaluator.cp";
import { BreakValue, BreakInsideValue, PageMarginRule, isValidUtf8, decodeLegacyCssToUtf8 } from "./paged_media.cp";
import { Direction, UnicodeBidi, resolveFirstStrongDirection } from "./bidi.cp";
import {
    CssColor,
    CssUnit,
    CssLength,
    CssCalc,
    CssProperty,
    CssRule,
    CssRuleKind,
    CssStylesheetParser,
    SupportsTree,
    MediaQueryList,
    MediaEnv,
    MediaType
} from "cap:css";

pub enum DisplayKeyword {
    None,
    Contents,
}

pub enum ListStyleType {
    Decimal,
    UpperRoman,
    LowerRoman,
    None,

    static fn parse(raw: []const u8): ListStyleType? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "none")) return ListStyleType.None;
        if (std.ascii.eqlIgnoreCase(s, "decimal")) return ListStyleType.Decimal;
        if (std.ascii.eqlIgnoreCase(s, "upper-roman")) return ListStyleType.UpperRoman;
        if (std.ascii.eqlIgnoreCase(s, "lower-roman")) return ListStyleType.LowerRoman;
        return null;
    }
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
    TableRowGroup,
    TableHeaderGroup,
    TableFooterGroup,
    TableRow,
    TableCell,
    TableCaption,
    TableColumn,
    TableColumnGroup,
    WebkitBox,
}

pub enum WhiteSpace {
    Normal,
    NoWrap,
    Pre,
    PreWrap,
    PreLine,
    BreakSpaces,

    static fn parse(raw: []const u8): WhiteSpace? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "normal")) return WhiteSpace.Normal;
        if (std.ascii.eqlIgnoreCase(s, "nowrap")) return WhiteSpace.NoWrap;
        if (std.ascii.eqlIgnoreCase(s, "pre")) return WhiteSpace.Pre;
        if (std.ascii.eqlIgnoreCase(s, "pre-wrap")) return WhiteSpace.PreWrap;
        if (std.ascii.eqlIgnoreCase(s, "pre-line")) return WhiteSpace.PreLine;
        if (std.ascii.eqlIgnoreCase(s, "break-spaces")) return WhiteSpace.BreakSpaces;
        return null;
    }

    fn preservesSpaces(): bool {
        return _ == WhiteSpace.Pre || _ == WhiteSpace.PreWrap || _ == WhiteSpace.BreakSpaces;
    }

    fn preservesNewlines(): bool {
        return _ == WhiteSpace.Pre || _ == WhiteSpace.PreWrap || _ == WhiteSpace.PreLine || _ == WhiteSpace.BreakSpaces;
    }

    fn allowsWrap(): bool {
        return _ != WhiteSpace.NoWrap && _ != WhiteSpace.Pre;
    }
}

pub enum BoxOrient {
    Horizontal,
    Vertical,

    static fn parse(raw: []const u8): BoxOrient? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "horizontal") || std.ascii.eqlIgnoreCase(s, "inline-axis")) return BoxOrient.Horizontal;
        if (std.ascii.eqlIgnoreCase(s, "vertical") || std.ascii.eqlIgnoreCase(s, "block-axis")) return BoxOrient.Vertical;
        return null;
    }
}

pub enum IntrinsicSizeKeyword {
    None,
    MinContent,
    MaxContent,
    FitContent,
    Stretch,
}

pub enum BorderCollapse {
    Separate,
    Collapse,

    static fn parse(raw: []const u8): BorderCollapse? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "separate")) return BorderCollapse.Separate;
        if (std.ascii.eqlIgnoreCase(s, "collapse")) return BorderCollapse.Collapse;
        return null;
    }
}

pub enum CaptionSide {
    Top,
    Bottom,

    static fn parse(raw: []const u8): CaptionSide? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "top")) return CaptionSide.Top;
        if (std.ascii.eqlIgnoreCase(s, "bottom")) return CaptionSide.Bottom;
        return null;
    }
}

pub enum TableLayout {
    Auto,
    Fixed,

    static fn parse(raw: []const u8): TableLayout? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "auto")) return TableLayout.Auto;
        if (std.ascii.eqlIgnoreCase(s, "fixed")) return TableLayout.Fixed;
        return null;
    }
}

pub enum FloatMode {
    None,
    Left,
    Right,

    static fn parse(raw: []const u8): FloatMode? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "none")) return FloatMode.None;
        if (std.ascii.eqlIgnoreCase(s, "left") || std.ascii.eqlIgnoreCase(s, "inline-start")) return FloatMode.Left;
        if (std.ascii.eqlIgnoreCase(s, "right") || std.ascii.eqlIgnoreCase(s, "inline-end")) return FloatMode.Right;
        return null;
    }
}

pub enum ClearMode {
    None,
    Left,
    Right,
    Both,

    static fn parse(raw: []const u8): ClearMode? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "none")) return ClearMode.None;
        if (std.ascii.eqlIgnoreCase(s, "left") || std.ascii.eqlIgnoreCase(s, "inline-start")) return ClearMode.Left;
        if (std.ascii.eqlIgnoreCase(s, "right") || std.ascii.eqlIgnoreCase(s, "inline-end")) return ClearMode.Right;
        if (std.ascii.eqlIgnoreCase(s, "both")) return ClearMode.Both;
        return null;
    }
}

pub enum FlexWrap {
    NoWrap,
    Wrap,
    WrapReverse,

    static fn parse(raw: []const u8): FlexWrap? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "nowrap")) return FlexWrap.NoWrap;
        if (std.ascii.eqlIgnoreCase(s, "wrap")) return FlexWrap.Wrap;
        if (std.ascii.eqlIgnoreCase(s, "wrap-reverse")) return FlexWrap.WrapReverse;
        return null;
    }
}

pub enum CornerShape {
    Round,
    Bevel,
    Scoop,
    Squircle,
    Notch,

    static fn parse(raw: []const u8): CornerShape? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "round")) return CornerShape.Round;
        if (std.ascii.eqlIgnoreCase(s, "bevel")) return CornerShape.Bevel;
        if (std.ascii.eqlIgnoreCase(s, "scoop")) return CornerShape.Scoop;
        if (std.ascii.eqlIgnoreCase(s, "squircle")) return CornerShape.Squircle;
        if (std.ascii.eqlIgnoreCase(s, "notch")) return CornerShape.Notch;
        return null;
    }
}

pub struct LegacyClipRect {
    has_clip: bool = false;
    top: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();
    right: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();
    bottom: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();
    left: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();
}

pub struct ClipPathInset {
    has_clip: bool = false;
    top: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    right: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    bottom: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    left: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
}

pub struct CssBoxShadow {
    has_shadow: bool = false;
    inset: bool = false;
    offset_x: f32 = 0.0;
    offset_y: f32 = 0.0;
    blur: f32 = 0.0;
    spread: f32 = 0.0;
    color: CssColor = CssColor.rgba(0, 0, 0, 255);
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

    static fn flowRoot(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.FlowRoot },
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

    static fn webkitBox(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.WebkitBox },
        };
    }

    static fn inlineWebkitBox(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Inline, inside: DisplayInside.WebkitBox },
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

    static fn table(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.Table },
        };
    }

    static fn inlineTable(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Inline, inside: DisplayInside.Table },
        };
    }

    static fn tableRowGroup(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.TableRowGroup },
        };
    }

    static fn tableHeaderGroup(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.TableHeaderGroup },
        };
    }

    static fn tableFooterGroup(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.TableFooterGroup },
        };
    }

    static fn tableRow(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.TableRow },
        };
    }

    static fn tableCell(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.TableCell },
        };
    }

    static fn tableCaption(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.TableCaption },
        };
    }

    static fn tableColumn(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.TableColumn },
        };
    }

    static fn tableColumnGroup(): Display {
        return Display {
            is_keyword: false,
            pair: DisplayPair { outside: DisplayOutside.Block, inside: DisplayInside.TableColumnGroup },
        };
    }

    static fn parse(raw: []const u8): Display? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "none")) return Display.none();
        if (std.ascii.eqlIgnoreCase(s, "contents")) return Display.contents();
        if (std.ascii.eqlIgnoreCase(s, "block")) return Display.block();
        if (std.ascii.eqlIgnoreCase(s, "flow-root")) return Display.flowRoot();
        if (std.ascii.eqlIgnoreCase(s, "inline")) return Display.inlineFlow();
        if (std.ascii.eqlIgnoreCase(s, "inline-block")) return Display.inlineBlock();
        if (std.ascii.eqlIgnoreCase(s, "flex") || std.ascii.eqlIgnoreCase(s, "-webkit-flex")) return Display.flex();
        if (std.ascii.eqlIgnoreCase(s, "inline-flex") || std.ascii.eqlIgnoreCase(s, "-webkit-inline-flex")) return Display.inlineFlex();
        if (std.ascii.eqlIgnoreCase(s, "-webkit-box") || std.ascii.eqlIgnoreCase(s, "-moz-box")) return Display.webkitBox();
        if (std.ascii.eqlIgnoreCase(s, "-webkit-inline-box") || std.ascii.eqlIgnoreCase(s, "-moz-inline-box")) return Display.inlineWebkitBox();
        if (std.ascii.eqlIgnoreCase(s, "grid") || std.ascii.eqlIgnoreCase(s, "inline-grid")) return Display.grid();
        if (std.ascii.eqlIgnoreCase(s, "table")) return Display.table();
        if (std.ascii.eqlIgnoreCase(s, "inline-table")) return Display.inlineTable();
        if (std.ascii.eqlIgnoreCase(s, "table-row-group")) return Display.tableRowGroup();
        if (std.ascii.eqlIgnoreCase(s, "table-header-group")) return Display.tableHeaderGroup();
        if (std.ascii.eqlIgnoreCase(s, "table-footer-group")) return Display.tableFooterGroup();
        if (std.ascii.eqlIgnoreCase(s, "table-row")) return Display.tableRow();
        if (std.ascii.eqlIgnoreCase(s, "table-cell")) return Display.tableCell();
        if (std.ascii.eqlIgnoreCase(s, "table-caption")) return Display.tableCaption();
        if (std.ascii.eqlIgnoreCase(s, "table-column")) return Display.tableColumn();
        if (std.ascii.eqlIgnoreCase(s, "table-column-group")) return Display.tableColumnGroup();
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

    fn isWebkitBox(): bool {
        return !_.is_keyword && _.pair.inside == DisplayInside.WebkitBox;
    }

    fn isFlowRoot(): bool {
        return !_.is_keyword && _.pair.inside == DisplayInside.FlowRoot;
    }

    fn isGrid(): bool {
        return !_.is_keyword && _.pair.inside == DisplayInside.Grid;
    }

    fn isTable(): bool {
        return !_.is_keyword && _.pair.inside == DisplayInside.Table;
    }

    fn isTableRowGroup(): bool {
        return !_.is_keyword && (_.pair.inside == DisplayInside.TableRowGroup || _.pair.inside == DisplayInside.TableHeaderGroup || _.pair.inside == DisplayInside.TableFooterGroup);
    }

    fn isTableHeaderGroup(): bool {
        return !_.is_keyword && _.pair.inside == DisplayInside.TableHeaderGroup;
    }

    fn isTableFooterGroup(): bool {
        return !_.is_keyword && _.pair.inside == DisplayInside.TableFooterGroup;
    }

    fn isTableRow(): bool {
        return !_.is_keyword && _.pair.inside == DisplayInside.TableRow;
    }

    fn isTableCell(): bool {
        return !_.is_keyword && _.pair.inside == DisplayInside.TableCell;
    }

    fn isTableCaption(): bool {
        return !_.is_keyword && _.pair.inside == DisplayInside.TableCaption;
    }

    fn isTableColumn(): bool {
        return !_.is_keyword && (_.pair.inside == DisplayInside.TableColumn || _.pair.inside == DisplayInside.TableColumnGroup);
    }

    fn isTableInternal(): bool {
        return _.isTableRowGroup() || _.isTableRow() || _.isTableCell() || _.isTableCaption() || _.isTableColumn();
    }

    fn isInline(): bool {
        return !_.is_keyword && _.pair.outside == DisplayOutside.Inline && _.pair.inside == DisplayInside.Flow;
    }

    fn isInlineBlock(): bool {
        return !_.is_keyword && _.pair.outside == DisplayOutside.Inline && _.pair.inside == DisplayInside.FlowRoot;
    }

    fn isInlineFlex(): bool {
        return !_.is_keyword && _.pair.outside == DisplayOutside.Inline && _.pair.inside == DisplayInside.Flex;
    }

    fn isInlineTable(): bool {
        return !_.is_keyword && _.pair.outside == DisplayOutside.Inline && _.pair.inside == DisplayInside.Table;
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

pub enum CssPosition {
    Static,
    Relative,
    Absolute,
    Fixed,
    Sticky,

    static fn parse(raw: []const u8): CssPosition? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "static")) return CssPosition.Static;
        if (std.ascii.eqlIgnoreCase(s, "relative")) return CssPosition.Relative;
        if (std.ascii.eqlIgnoreCase(s, "absolute")) return CssPosition.Absolute;
        if (std.ascii.eqlIgnoreCase(s, "fixed")) return CssPosition.Fixed;
        if (std.ascii.eqlIgnoreCase(s, "sticky")) return CssPosition.Sticky;
        return null;
    }
}

pub enum VerticalAlignKind {
    Baseline,
    Top,
    Bottom,
    Middle,
    TextTop,
    TextBottom,
    Super,
    Sub,
    Length,
    Percent,
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
    intrinsic: IntrinsicSizeKeyword = IntrinsicSizeKeyword.None;
    calc_expr: []const u8 = "";
    length: CssLength = CssLength { value: 0.0, unit: CssUnit.Px };

    fn isIntrinsic(): bool {
        return _.intrinsic != IntrinsicSizeKeyword.None;
    }

    static fn minContentVal(): LengthPercentageOrAuto {
        return LengthPercentageOrAuto { intrinsic: IntrinsicSizeKeyword.MinContent };
    }

    static fn maxContentVal(): LengthPercentageOrAuto {
        return LengthPercentageOrAuto { intrinsic: IntrinsicSizeKeyword.MaxContent };
    }

    static fn fitContentVal(): LengthPercentageOrAuto {
        return LengthPercentageOrAuto { intrinsic: IntrinsicSizeKeyword.FitContent };
    }

    static fn stretchVal(): LengthPercentageOrAuto {
        return LengthPercentageOrAuto { intrinsic: IntrinsicSizeKeyword.Stretch };
    }

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
        if (std.ascii.eqlIgnoreCase(s, "min-content") || std.ascii.eqlIgnoreCase(s, "-webkit-min-content") || std.ascii.eqlIgnoreCase(s, "-moz-min-content")) {
            return LengthPercentageOrAuto.minContentVal();
        }
        if (std.ascii.eqlIgnoreCase(s, "max-content") || std.ascii.eqlIgnoreCase(s, "-webkit-max-content") || std.ascii.eqlIgnoreCase(s, "-moz-max-content")) {
            return LengthPercentageOrAuto.maxContentVal();
        }
        if (std.ascii.eqlIgnoreCase(s, "fit-content") || std.ascii.eqlIgnoreCase(s, "-webkit-fit-content") || std.ascii.eqlIgnoreCase(s, "-moz-fit-content")) {
            return LengthPercentageOrAuto.fitContentVal();
        }
        if (std.ascii.eqlIgnoreCase(s, "stretch") || std.ascii.eqlIgnoreCase(s, "-webkit-fill-available") || std.ascii.eqlIgnoreCase(s, "-moz-available") || std.ascii.eqlIgnoreCase(s, "fill-available")) {
            return LengthPercentageOrAuto.stretchVal();
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

    static fn evalCalcPx(
        expr: []const u8,
        parent_px: f32,
        element_font_px: f32,
        root_font_px: f32,
        viewport_w: f32,
        viewport_h: f32,
    ): f64? {
        let s = std.mem.trim(u8, expr, " \t\r\n");
        if (std.ascii.startsWithIgnoreCase(s, "calc(") && s.len >= 6 && s[s.len - 1] == ')') {
            s = std.mem.trim(u8, s[5 .. s.len - 1], " \t\r\n");
        }
        if (s.len == 0) return null;

        let values: [16]f64 = std.mem.zeroes([16]f64);
        let ops: [16]u8 = std.mem.zeroes([16]u8);
        let val_count: usize = 0;
        let op_count: usize = 0;

        let it = std.mem.tokenizeAny(u8, s, " \t\r\n");
        while (tok := it.next()) {
            if (tok.len == 1 && (tok[0] == '+' || tok[0] == '-' || tok[0] == '*' || tok[0] == '/')) {
                if (op_count >= 16) return null;
                ops[op_count] = tok[0];
                op_count += 1;
            } else {
                if (val_count >= 16) return null;
                let term_v: f64 = 0.0;
                if (len_val := CssLength.parse(tok)) {
                    if (len_val.unit == CssUnit.Percent) {
                        term_v = (len_val.value / 100.0) * @as(f64, parent_px);
                    } else if (len_val.unit == CssUnit.Em) {
                        term_v = len_val.value * @as(f64, element_font_px);
                    } else if (len_val.unit == CssUnit.Ch || len_val.unit == CssUnit.Ex) {
                        term_v = len_val.value * @as(f64, element_font_px) * 0.5;
                    } else {
                        term_v = len_val.toPx(@as(f64, root_font_px), @as(f64, viewport_w), @as(f64, viewport_h));
                    }
                } else {
                    const num = std.fmt.parseFloat(f64, tok) catch return null;
                    term_v = num;
                }
                values[val_count] = term_v;
                val_count += 1;
            }
        }

        if (val_count == 0 || op_count + 1 != val_count) {
            return null;
        }

        let collapsed_vals: [16]f64 = std.mem.zeroes([16]f64);
        let collapsed_ops: [16]u8 = std.mem.zeroes([16]u8);
        collapsed_vals[0] = values[0];
        let c_idx: usize = 0;

        let i: usize = 0;
        while (i < op_count) : (i += 1) {
            const op = ops[i];
            const rhs = values[i + 1];
            if (op == '*') {
                collapsed_vals[c_idx] = collapsed_vals[c_idx] * rhs;
            } else if (op == '/') {
                if (rhs == 0.0) return null;
                collapsed_vals[c_idx] = collapsed_vals[c_idx] / rhs;
            } else {
                collapsed_ops[c_idx] = op;
                c_idx += 1;
                collapsed_vals[c_idx] = rhs;
            }
        }

        let acc = collapsed_vals[0];
        let j: usize = 0;
        while (j < c_idx) : (j += 1) {
            if (collapsed_ops[j] == '+') {
                acc += collapsed_vals[j + 1];
            } else {
                acc -= collapsed_vals[j + 1];
            }
        }
        return acc;
    }

    fn resolvePx(
        parent_px: f32,
        element_font_px: f32,
        root_font_px: f32,
        viewport_w: f32,
        viewport_h: f32,
    ): f32 {
        if (_.is_auto || _.is_none || _.isIntrinsic()) {
            return 0.0;
        }
        if (_.is_calc) {
            let expr = _.calc_expr;
            let buf: [512]u8 = std.mem.zeroes([512]u8);
            if (std.mem.indexOf(u8, expr, "anchor") != null) {
                expr = LengthPercentageOrAuto.substituteAnchorFallbacks(expr, &buf);
            }
            if (v := LengthPercentageOrAuto.evalCalcPx(expr, parent_px, element_font_px, root_font_px, viewport_w, viewport_h)) {
                return @floatCast(v);
            }
            if (v2 := CssCalc.evalPx(expr, @as(f64, root_font_px), @as(f64, viewport_w), @as(f64, viewport_h))) {
                return @floatCast(v2);
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
        if (_.length.unit == CssUnit.Ch || _.length.unit == CssUnit.Ex) {
            const ch_mult: f32 = @floatCast(_.length.value);
            return ch_mult * element_font_px * 0.5;
        }
        const px_f64 = _.length.toPx(@as(f64, root_font_px), @as(f64, viewport_w), @as(f64, viewport_h));
        return @floatCast(px_f64);
    }
}

pub enum OverflowMode {
    Visible,
    Clip,
    Hidden,
    Scroll,
    Auto,

    static fn parse(raw: []const u8): OverflowMode? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "visible")) return OverflowMode.Visible;
        if (std.ascii.eqlIgnoreCase(s, "clip")) return OverflowMode.Clip;
        if (std.ascii.eqlIgnoreCase(s, "hidden")) return OverflowMode.Hidden;
        if (std.ascii.eqlIgnoreCase(s, "scroll")) return OverflowMode.Scroll;
        if (std.ascii.eqlIgnoreCase(s, "auto")) return OverflowMode.Auto;
        return null;
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
pub const PROP_OVERFLOW_X: usize = 47;
pub const PROP_TRANSFORM: usize = 48;
pub const PROP_BORDER_COLLAPSE: usize = 49;
pub const PROP_BORDER_SPACING: usize = 50;
pub const PROP_CAPTION_SIDE: usize = 51;
pub const PROP_TABLE_LAYOUT: usize = 52;
pub const PROP_FLOAT: usize = 53;
pub const PROP_CLEAR: usize = 54;
pub const PROP_ACCENT_COLOR: usize = 55;
pub const PROP_FLEX_WRAP: usize = 56;
pub const PROP_CLIP: usize = 57;
pub const PROP_CLIP_PATH: usize = 58;
pub const PROP_OUTLINE_WIDTH: usize = 59;
pub const PROP_OUTLINE_COLOR: usize = 60;
pub const PROP_OUTLINE_STYLE: usize = 61;
pub const PROP_OUTLINE_OFFSET: usize = 62;
pub const PROP_BORDER_TOP_COLOR: usize = 63;
pub const PROP_BORDER_RIGHT_COLOR: usize = 64;
pub const PROP_BORDER_BOTTOM_COLOR: usize = 65;
pub const PROP_BORDER_LEFT_COLOR: usize = 66;
pub const PROP_BORDER_TL_RADIUS: usize = 67;
pub const PROP_BORDER_TR_RADIUS: usize = 68;
pub const PROP_BORDER_BR_RADIUS: usize = 69;
pub const PROP_BORDER_BL_RADIUS: usize = 70;
pub const PROP_CORNER_SHAPE: usize = 71;
pub const PROP_BOX_SHADOW: usize = 72;
pub const PROP_OVERFLOW_Y: usize = 73;
pub const PROP_GRID_ROWS: usize = 74;
pub const PROP_GRID_COL_START: usize = 75;
pub const PROP_GRID_COL_END: usize = 76;
pub const PROP_GRID_ROW_START: usize = 77;
pub const PROP_GRID_ROW_END: usize = 78;
pub const PROP_VERTICAL_ALIGN: usize = 79;
pub const PROP_WHITE_SPACE: usize = 80;
pub const PROP_BOX_ORIENT: usize = 81;
pub const PROP_LINE_CLAMP: usize = 82;
pub const PROP_CONTAINER_TYPE: usize = 83;
pub const PROP_CONTAINER_NAME: usize = 84;
pub const PROP_TEXT_FILL_COLOR: usize = 85;
pub const PROP_TEXT_DECORATION: usize = 86;
pub const PROP_BG_CLIP: usize = 87;
pub const PROP_BG_SIZE: usize = 88;
pub const PROP_CARET_COLOR: usize = 89;
pub const PROP_POINTER_EVENTS: usize = 90;
pub const PROP_ISOLATION: usize = 91;
pub const PROP_TEXT_DECORATION_COLOR: usize = 92;
pub const PROP_TEXT_DECORATION_THICKNESS: usize = 93;
pub const PROP_TEXT_UNDERLINE_OFFSET: usize = 94;
pub const PROP_COLOR_SCHEME: usize = 95;
pub const PROP_BREAK_BEFORE: usize = 96;
pub const PROP_BREAK_AFTER: usize = 97;
pub const PROP_BREAK_INSIDE: usize = 98;
pub const PROP_DIRECTION: usize = 99;
pub const PROP_UNICODE_BIDI: usize = 100;
pub const TOTAL_PROP_SLOTS: usize = 101;

pub enum ColorScheme {
    Normal,
    Light,
    Dark,
    LightDark,

    static fn parse(raw: []const u8): ColorScheme? {
        const trimmed = std.mem.trim(u8, raw, " \t\r\n");
        if (trimmed.len == 0) return null;
        if (std.ascii.eqlIgnoreCase(trimmed, "normal")) return ColorScheme.Normal;
        let has_light = false;
        let has_dark = false;
        let it = std.mem.tokenizeAny(u8, trimmed, " \t\r\n");
        while (tok := it.next()) {
            if (std.ascii.eqlIgnoreCase(tok, "light")) {
                has_light = true;
            } else if (std.ascii.eqlIgnoreCase(tok, "dark")) {
                has_dark = true;
            }
        }
        if (has_light && has_dark) return ColorScheme.LightDark;
        if (has_dark) return ColorScheme.Dark;
        if (has_light) return ColorScheme.Light;
        return null;
    }

    fn isDarkUsed(): bool {
        return _ == ColorScheme.Dark;
    }
}

struct LightDarkPair {
    light: CssColor = CssColor.rgba(0, 0, 0, 0),
    dark: CssColor = CssColor.rgba(0, 0, 0, 0),
}

fn parseLightDarkPair(raw: []const u8): LightDarkPair? {
    const s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len < 13) return null;
    if (!std.ascii.startsWithIgnoreCase(s, "light-dark(") || s[s.len - 1] != '\)') return null;
    const inner = std.mem.trim(u8, s[11 .. s.len - 1], " \t\r\n");
    let depth: i32 = 0;
    let split_idx: usize? = null;
    let i: usize = 0;
    while (i < inner.len) : (i += 1) {
        if (inner[i] == '(') {
            depth += 1;
        } else if (inner[i] == '\)') {
            depth -= 1;
        } else if (inner[i] == ',' && depth == 0) {
            split_idx = i;
            break;
        }
    }
    if (si := split_idx) {
        const light_s = std.mem.trim(u8, inner[0..si], " \t\r\n");
        const dark_s = std.mem.trim(u8, inner[si + 1 ..], " \t\r\n");
        if (lc := CssColor.parse(light_s)) {
            if (dc := CssColor.parse(dark_s)) {
                return LightDarkPair{ light: lc, dark: dc };
            }
        }
    }
    return null;
}

pub enum ContainerType {
    Normal,
    Size,
    InlineSize,

    static fn parse(raw: []const u8): ContainerType? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "normal")) return ContainerType.Normal;
        if (std.ascii.eqlIgnoreCase(s, "size")) return ContainerType.Size;
        if (std.ascii.eqlIgnoreCase(s, "inline-size")) return ContainerType.InlineSize;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Normal => "normal",
            .Size => "size",
            .InlineSize => "inline-size",
        };
    }
}

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

pub enum BgGradientKind {
    None,
    Linear,
    Radial,
}

pub struct BgGradientStop {
    color: CssColor = CssColor.rgba(0, 0, 0, 0);
    pos_val: f32 = 0.0;
    is_px: bool = false;
    has_pos: bool = false;
}

fn parseGradColor(tok: []const u8): CssColor? {
    const s = std.mem.trim(u8, tok, " \t\r\n");
    if (s.len == 0) return null;
    if (std.ascii.eqlIgnoreCase(s, "transparent")) {
        return CssColor.rgba(0, 0, 0, 0);
    }
    return CssColor.parse(s);
}

fn parseGradPxToken(tok: []const u8): f32? {
    const s = std.mem.trim(u8, tok, " \t\r\n");
    if (s.len == 0) return null;
    if (std.mem.eql(u8, s, "0")) return 0.0;
    if (std.ascii.endsWithIgnoreCase(s, "px") && s.len > 2) {
        return std.fmt.parseFloat(f32, s[0 .. s.len - 2]) catch null;
    }
    return std.fmt.parseFloat(f32, s) catch null;
}

pub struct BgGradient {
    kind: BgGradientKind = BgGradientKind.None;
    angle_deg: f32 = 180.0;
    stops: [16]BgGradientStop = [BgGradientStop{}; 16];
    stop_count: usize = 0;
    tile_w: f32 = 0.0;
    tile_h: f32 = 0.0;
    pos_x: f32 = 0.0;
    pos_y: f32 = 0.0;
    clip_text: bool = false;

    fn hasGradient(): bool {
        return _.kind != BgGradientKind.None && _.stop_count > 0;
    }

    fn sampleAt(
        box_x: f32,
        box_y: f32,
        box_w: f32,
        box_h: f32,
        px: f32,
        py: f32,
    ): CssColor {
        if (_.kind == BgGradientKind.None || _.stop_count == 0) {
            return CssColor.rgba(0, 0, 0, 0);
        }
        if (_.stop_count == 1) {
            return _.stops[0].color;
        }
        const tw = if (_.tile_w > 0.0) _.tile_w else box_w;
        const th = if (_.tile_h > 0.0) _.tile_h else box_h;
        if (tw <= 0.0 || th <= 0.0) {
            return CssColor.rgba(0, 0, 0, 0);
        }

        let lx = px - box_x - _.pos_x;
        let ly = py - box_y - _.pos_y;
        if (_.tile_w > 0.0) {
            lx = lx - @floor(lx / tw) * tw;
        }
        if (_.tile_h > 0.0) {
            ly = ly - @floor(ly / th) * th;
        }

        let sample_pos: f32 = 0.0;
        let axis_len: f32 = 1.0;

        if (_.kind == BgGradientKind.Radial) {
            const cx = tw * 0.5;
            const cy = th * 0.5;
            const dx = lx - cx;
            const dy = ly - cy;
            sample_pos = @sqrt(dx * dx + dy * dy);
            axis_len = @sqrt(cx * cx + cy * cy);
        } else {
            const rad = _.angle_deg * std.math.pi / 180.0;
            const dx = @sin(rad);
            const dy = 0.0 - @cos(rad);
            axis_len = @abs(tw * dx) + @abs(th * dy);
            if (axis_len <= 0.0001) axis_len = 1.0;
            sample_pos = (lx - tw * 0.5) * dx + (ly - th * 0.5) * dy + axis_len * 0.5;
        }

        let resolved_pos: [16]f32 = [0.0; 16];
        let si: usize = 0;
        while (si < _.stop_count) : (si += 1) {
            const st = _.stops[si];
            if (st.has_pos) {
                resolved_pos[si] = if (st.is_px) st.pos_val else ((st.pos_val / 100.0) * axis_len);
            } else if (_.stop_count == 1) {
                resolved_pos[si] = 0.0;
            } else {
                resolved_pos[si] = (@as(f32, @floatFromInt(si)) / @as(f32, @floatFromInt(_.stop_count - 1))) * axis_len;
            }
        }

        if (sample_pos <= resolved_pos[0]) {
            return _.stops[0].color;
        }
        if (sample_pos >= resolved_pos[_.stop_count - 1]) {
            return _.stops[_.stop_count - 1].color;
        }

        let i: usize = 1;
        while (i < _.stop_count) : (i += 1) {
            if (sample_pos <= resolved_pos[i]) {
                const c0 = _.stops[i - 1].color;
                const c1 = _.stops[i].color;
                const span = resolved_pos[i] - resolved_pos[i - 1];
                if (span <= 0.0001) {
                    return c1;
                }
                const t = (sample_pos - resolved_pos[i - 1]) / span;
                return c0.mix(c1, @as(f64, t));
            }
        }
        return _.stops[_.stop_count - 1].color;
    }

    static fn parse(val_str: []const u8): BgGradient? {
        let kind = BgGradientKind.None;
        let g_pos: usize = 0;
        let prefix_len: usize = 0;

        if (pos := std.mem.indexOf(u8, val_str, "radial-gradient")) {
            kind = BgGradientKind.Radial;
            g_pos = pos;
            prefix_len = 15;
        } else if (pos_l := std.mem.indexOf(u8, val_str, "linear-gradient")) {
            kind = BgGradientKind.Linear;
            g_pos = pos_l;
            prefix_len = 15;
        } else {
            return null;
        }

        const paren_opt = std.mem.indexOfScalarPos(u8, val_str, g_pos + prefix_len, '(');
        if (paren_opt == null) return null;
        const paren_open = paren_opt.?;
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
        if (args_end <= args_start) return null;

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
        if (item_count == 0) return null;

        let bg = BgGradient{
            kind: kind,
            angle_deg: 180.0,
        };

        let start_idx: usize = 0;
        if (kind == BgGradientKind.Linear) {
            const it0 = items[0];
            if (std.ascii.startsWithIgnoreCase(it0, "to ")) {
                start_idx = 1;
                const has_top = std.mem.indexOf(u8, it0, "top") != null;
                const has_bottom = std.mem.indexOf(u8, it0, "bottom") != null;
                const has_left = std.mem.indexOf(u8, it0, "left") != null;
                const has_right = std.mem.indexOf(u8, it0, "right") != null;
                if (has_top && has_right) {
                    bg.angle_deg = 45.0;
                } else if (has_bottom && has_right) {
                    bg.angle_deg = 135.0;
                } else if (has_bottom && has_left) {
                    bg.angle_deg = 225.0;
                } else if (has_top && has_left) {
                    bg.angle_deg = 315.0;
                } else if (has_top) {
                    bg.angle_deg = 0.0;
                } else if (has_right) {
                    bg.angle_deg = 90.0;
                } else if (has_bottom) {
                    bg.angle_deg = 180.0;
                } else if (has_left) {
                    bg.angle_deg = 270.0;
                }
            } else if (std.ascii.endsWithIgnoreCase(it0, "deg") && it0.len > 3) {
                if (deg := std.fmt.parseFloat(f32, it0[0 .. it0.len - 3]) catch null) {
                    bg.angle_deg = deg;
                    start_idx = 1;
                }
            }
        } else if (kind == BgGradientKind.Radial) {
            const it0 = items[0];
            if (std.ascii.startsWithIgnoreCase(it0, "circle") ||
                std.ascii.startsWithIgnoreCase(it0, "ellipse") ||
                std.ascii.startsWithIgnoreCase(it0, "at ") ||
                std.ascii.startsWithIgnoreCase(it0, "closest-") ||
                std.ascii.startsWithIgnoreCase(it0, "farthest-")) {
                start_idx = 1;
            }
        }

        let stop_idx: usize = 0;
        let k = start_idx;
        while (k < item_count && stop_idx < 16) : (k += 1) {
            const stop_str = items[k];
            let pos: usize = 0;
            let color_opt: CssColor? = null;
            let has_pos = false;
            let pos_val: f32 = 0.0;
            let is_px = false;

            while (tok := nextCssValueToken(stop_str, &pos)) {
                if (color_opt == null) {
                    if (c := parseGradColor(tok)) {
                        color_opt = c;
                        continue;
                    }
                }
                if (tok.len > 2 && std.ascii.endsWithIgnoreCase(tok, "px")) {
                    if (v := std.fmt.parseFloat(f32, tok[0 .. tok.len - 2]) catch null) {
                        pos_val = v;
                        is_px = true;
                        has_pos = true;
                    }
                } else if (tok.len > 1 && tok[tok.len - 1] == '%') {
                    if (v := std.fmt.parseFloat(f32, tok[0 .. tok.len - 1]) catch null) {
                        pos_val = v;
                        is_px = false;
                        has_pos = true;
                    }
                } else if (std.mem.eql(u8, tok, "0")) {
                    pos_val = 0.0;
                    is_px = true;
                    has_pos = true;
                }
            }
            if (c := color_opt) {
                bg.stops[stop_idx] = BgGradientStop{
                    color: c,
                    pos_val: pos_val,
                    is_px: is_px,
                    has_pos: has_pos,
                };
                stop_idx += 1;
            }
        }
        if (stop_idx == 0) return null;
        bg.stop_count = stop_idx;

        // Parse trailing shorthand `pos_x pos_y / tile_w tile_h` after `)`
        if (args_end + 1 < val_str.len) {
            const tail = std.mem.trim(u8, val_str[args_end + 1 ..], " \t\r\n");
            if (slash := std.mem.indexOfScalar(u8, tail, '/')) {
                const pos_part = std.mem.trim(u8, tail[0..slash], " \t\r\n");
                let pit = std.mem.tokenizeAny(u8, pos_part, " \t\r\n");
                if (p0 := pit.next()) {
                    if (v := parseGradPxToken(p0)) bg.pos_x = v;
                }
                if (p1 := pit.next()) {
                    if (v := parseGradPxToken(p1)) bg.pos_y = v;
                }

                const size_part = std.mem.trim(u8, tail[slash + 1 ..], " \t\r\n");
                let sit = std.mem.tokenizeAny(u8, size_part, " \t\r\n");
                if (s0 := sit.next()) {
                    if (v := parseGradPxToken(s0)) {
                        if (v > 0.0) {
                            bg.tile_w = v;
                            bg.tile_h = v;
                        }
                    }
                }
                if (s1 := sit.next()) {
                    if (v := parseGradPxToken(s1)) {
                        if (v > 0.0) {
                            bg.tile_h = v;
                        }
                    }
                }
            }
        }

        return bg;
    }
}

/// Resolved visual style ready for the 2D software rasterizer (`src/render/painter.cp`)
pub struct ResolvedPaintStyle {
    bg_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    border_color: CssColor = CssColor.rgb(0, 0, 0);
    border_top_color: CssColor = CssColor.rgb(0, 0, 0);
    border_right_color: CssColor = CssColor.rgb(0, 0, 0);
    border_bottom_color: CssColor = CssColor.rgb(0, 0, 0);
    border_left_color: CssColor = CssColor.rgb(0, 0, 0);
    border_top_width: f32 = 0.0;
    border_right_width: f32 = 0.0;
    border_bottom_width: f32 = 0.0;
    border_left_width: f32 = 0.0;
    text_color: CssColor = CssColor.rgb(0, 0, 0);
    text_fill_color: CssColor = CssColor.rgb(0, 0, 0);
    has_text_fill_color: bool = false;
    text_decoration_underline: bool = false;
    text_decoration_line_through: bool = false;
    text_decoration_overline: bool = false;
    accent_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    has_accent_color: bool = false;
    caret_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    has_caret_color: bool = false;
    placeholder_color: CssColor = CssColor.rgb(117, 117, 117);
    has_placeholder_color: bool = false;
    border_width: f32 = 0.0;
    border_radius: f32 = 0.0;
    border_tl_radius: f32 = 0.0;
    border_tr_radius: f32 = 0.0;
    border_br_radius: f32 = 0.0;
    border_bl_radius: f32 = 0.0;
    corner_shape_tl: CornerShape = CornerShape.Round;
    corner_shape_tr: CornerShape = CornerShape.Round;
    corner_shape_br: CornerShape = CornerShape.Round;
    corner_shape_bl: CornerShape = CornerShape.Round;
    outline_width: f32 = 0.0;
    outline_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    outline_offset: f32 = 0.0;
    box_shadow: CssBoxShadow = CssBoxShadow{};
    font_size_px: f32 = 16.0;
    conic_gradient: ConicGradient = ConicGradient{};
    bg_gradient: BgGradient = BgGradient{};
}

/// Paren-aware CSS value tokenizer: splits on whitespace at parenthesis depth 0.
fn nextCssValueToken(s: []const u8, pos: *usize): ([]const u8)? {
    let i = pos.*;
    while (i < s.len && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) : (i += 1) {}
    if (i >= s.len) {
        pos.* = i;
        return null;
    }
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
    pos.* = i;
    return s[start..i];
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

pub struct CustomProp {
    name: []const u8 = "",
    value: []const u8 = "",
    weight: u64 = 0,
}

pub struct ComputedStyle {
    display: Display = Display.block();
    visibility: Visibility = Visibility.Visible;
    box_sizing: BoxSizing = BoxSizing.ContentBox;
    position: CssPosition = CssPosition.Static;

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
    border_top_color: CssColor = CssColor.rgb(0, 0, 0);
    border_right_color: CssColor = CssColor.rgb(0, 0, 0);
    border_bottom_color: CssColor = CssColor.rgb(0, 0, 0);
    border_left_color: CssColor = CssColor.rgb(0, 0, 0);
    has_border_top_color: bool = false;
    has_border_right_color: bool = false;
    has_border_bottom_color: bool = false;
    has_border_left_color: bool = false;
    border_radius: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    border_tl_radius: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    border_tr_radius: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    border_br_radius: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    border_bl_radius: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    corner_shape_tl: CornerShape = CornerShape.Round;
    corner_shape_tr: CornerShape = CornerShape.Round;
    corner_shape_br: CornerShape = CornerShape.Round;
    corner_shape_bl: CornerShape = CornerShape.Round;

    outline_width: f32 = 0.0;
    outline_color: CssColor = CssColor.rgb(0, 0, 0);
    has_explicit_outline_color: bool = false;
    outline_style_none: bool = true;
    outline_offset: f32 = 0.0;

    legacy_clip: LegacyClipRect = LegacyClipRect{};
    clip_path_inset: ClipPathInset = ClipPathInset{};
    box_shadow: CssBoxShadow = CssBoxShadow{};

    top: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();
    right: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();
    bottom: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();
    left: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();

    flex_direction: FlexDirection = FlexDirection.Row;
    flex_wrap: FlexWrap = FlexWrap.NoWrap;
    justify_content: JustifyContent = JustifyContent.FlexStart;
    align_items: AlignItems = AlignItems.Stretch;
    flex_grow: f32 = 0.0;
    flex_shrink: f32 = 1.0;
    flex_basis: LengthPercentageOrAuto = LengthPercentageOrAuto.autoVal();

    row_gap: f32 = 0.0;
    column_gap: f32 = 0.0;
    grid_cols: u32 = 1;
    grid_col_span: u32 = 1;
    grid_col_tracks: [16]LengthPercentageOrAuto = [LengthPercentageOrAuto.autoVal(); 16];
    grid_col_track_count: u32 = 0;
    grid_row_tracks: [16]LengthPercentageOrAuto = [LengthPercentageOrAuto.autoVal(); 16];
    grid_row_track_count: u32 = 0;
    grid_col_start: i32 = 0;
    grid_col_end: i32 = 0;
    grid_row_start: i32 = 0;
    grid_row_end: i32 = 0;

    font_size_px: f32 = 16.0;
    line_height_px: f32 = 0.0;
    has_line_height: bool = false;
    line_height_is_number: bool = false;
    line_height_number: f32 = 0.0;
    vertical_align_kind: VerticalAlignKind = VerticalAlignKind.Baseline;
    vertical_align_length: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    white_space: WhiteSpace = WhiteSpace.Normal;
    direction: Direction = Direction.Ltr;
    unicode_bidi: UnicodeBidi = UnicodeBidi.Normal;
    is_dir_auto: bool = false;
    box_orient: BoxOrient = BoxOrient.Horizontal;
    webkit_line_clamp: u32 = 0;
    is_line_clamped: bool = false;
    font_family_buf: [64]u8 = [0; 64],
    font_family_len: usize = 0,
    webfont: WebfontStyleMetrics = WebfontStyleMetrics{},
    before_content_buf: [64]u8 = [0; 64],
    before_content_len: usize = 0,
    has_before_content: bool = false,
    before_visibility: Visibility? = null,
    before_content_expr_buf: [128]u8 = [0; 128],
    before_content_expr_len: usize = 0,
    after_content_buf: [64]u8 = [0; 64],
    after_content_len: usize = 0,
    has_after_content: bool = false,
    after_visibility: Visibility? = null,
    after_content_expr_buf: [128]u8 = [0; 128],
    after_content_expr_len: usize = 0,
    list_style_type: ListStyleType = ListStyleType.Decimal,
    has_counter_reset: bool = false,
    counter_reset_name: [32]u8 = [0; 32],
    counter_reset_len: usize = 0,
    counter_reset_val: i32 = 0,
    has_counter_inc: bool = false,
    counter_inc_name: [32]u8 = [0; 32],
    counter_inc_len: usize = 0,
    counter_inc_val: i32 = 1,
    font_weight: f32 = 400.0,
    color: CssColor = CssColor.rgb(0, 0, 0);
    text_fill_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    has_text_fill_color: bool = false;
    text_decoration_underline: bool = false;
    text_decoration_line_through: bool = false;
    text_decoration_overline: bool = false;
    text_decoration_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    has_text_decoration_color: bool = false;
    text_decoration_thickness: f32 = 0.0;
    text_underline_offset: f32 = 0.0;
    pointer_events_none: bool = false;
    isolation_isolate: bool = false;
    background_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    accent_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    has_accent_color: bool = false;
    caret_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    has_caret_color: bool = false;
    placeholder_color: CssColor = CssColor.rgb(117, 117, 117);
    has_placeholder_color: bool = false;
    has_fill_current_color: bool = false,
    has_fill_none: bool = false,
    opacity: f32 = 1.0;
    z_index: i32 = 0;
    has_z_index: bool = false;
    conic_gradient: ConicGradient = ConicGradient{};
    bg_gradient: BgGradient = BgGradient{};
    bg_gradient_layer_idx: usize = 0;
    bg_clip_is_text: [4]bool = [false; 4];
    bg_clip_count: usize = 0;
    bg_image_url_buf: [256]u8 = [0; 256];
    bg_image_url_len: usize = 0;
    container_type: ContainerType = ContainerType.Normal;
    container_name_buf: [64]u8 = [0; 64];
    container_name_len: usize = 0;
    overflow_visible: bool = true;
    overflow_x: OverflowMode = OverflowMode.Visible;
    overflow_y: OverflowMode = OverflowMode.Visible;
    transform_tx: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();
    transform_ty: LengthPercentageOrAuto = LengthPercentageOrAuto.zeroPx();

    // Table & float properties
    color_scheme: ColorScheme = ColorScheme.Normal;
    has_bg_light_dark: bool = false;
    bg_light_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    bg_dark_color: CssColor = CssColor.rgba(0, 0, 0, 0);
    has_color_light_dark: bool = false;
    color_light: CssColor = CssColor.rgb(0, 0, 0);
    color_dark: CssColor = CssColor.rgb(0, 0, 0);

    border_collapse: BorderCollapse = BorderCollapse.Separate;
    border_spacing_h: f32 = 0.0;
    border_spacing_v: f32 = 0.0;
    caption_side: CaptionSide = CaptionSide.Top;
    table_layout: TableLayout = TableLayout.Auto;
    float_mode: FloatMode = FloatMode.None;
    clear_mode: ClearMode = ClearMode.None;
    break_before: BreakValue = BreakValue.Auto;
    break_after: BreakValue = BreakValue.Auto;
    break_inside: BreakInsideValue = BreakInsideValue.Auto;

    // 64-bit packed cascade weight per property slot
    weights: [TOTAL_PROP_SLOTS]u64 = [0; TOTAL_PROP_SLOTS];

    // Inherited CSS custom properties (`--*`)
    custom_props: [16]CustomProp = [CustomProp{}; 16],
    custom_prop_count: usize = 0,

    fn clipsOverflowX(): bool {
        if (_.overflow_x != OverflowMode.Visible) return true;
        if (_.overflow_y != OverflowMode.Visible && _.overflow_y != OverflowMode.Clip) return true;
        return false;
    }

    fn bgImageUrl(): []const u8 {
        return _.bg_image_url_buf[0.._.bg_image_url_len];
    }

    fn containerName(): []const u8 {
        return _.container_name_buf[0.._.container_name_len];
    }

    fn containerTypeStr(): []const u8 {
        return _.container_type.asStr();
    }

    fn clipsOverflowY(): bool {
        if (_.overflow_y != OverflowMode.Visible) return true;
        if (_.overflow_x != OverflowMode.Visible && _.overflow_x != OverflowMode.Clip) return true;
        return false;
    }

    fn syncOverflowVisible(*_) {
        _.overflow_visible = (_.overflow_x == OverflowMode.Visible && _.overflow_y == OverflowMode.Visible);
    }

    fn naturalAscent(): f32 {
        const fs = if (_.font_size_px > 0.0) _.font_size_px else 16.0;
        return @round(fs * (17.0 / 16.0));
    }

    fn naturalDescent(): f32 {
        const fs = if (_.font_size_px > 0.0) _.font_size_px else 16.0;
        return @round(fs * (5.0 / 16.0));
    }

    fn naturalLineHeight(): f32 {
        return _.naturalAscent() + _.naturalDescent();
    }

    fn xHeightRatio(): f32 {
        if (_.hasFontFamily("Bungee")) return 0.72;
        if (_.hasFontFamily("serif") && !_.hasFontFamily("sans-serif")) return 0.45;
        return 0.50;
    }

    fn resolvedLineHeight(): f32 {
        if (!_.has_line_height) {
            return _.naturalLineHeight();
        }
        if (_.line_height_is_number) {
            const fs = if (_.font_size_px > 0.0) _.font_size_px else 16.0;
            return fs * _.line_height_number;
        }
        return _.line_height_px;
    }

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
                _.display = Display.table();
            },
            .Thead => {
                _.display = Display.tableHeaderGroup();
            },
            .Tbody => {
                _.display = Display.tableRowGroup();
            },
            .Tfoot => {
                _.display = Display.tableFooterGroup();
            },
            .Tr => {
                _.display = Display.tableRow();
            },
            .Td, .Th => {
                _.display = Display.tableCell();
            },
            .Caption => {
                _.display = Display.tableCaption();
            },
            .Col => {
                _.display = Display.tableColumn();
            },
            .Colgroup => {
                _.display = Display.tableColumnGroup();
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
            .Iframe => {
                _.border_top_width = 2.0;
                _.border_right_width = 2.0;
                _.border_bottom_width = 2.0;
                _.border_left_width = 2.0;
            },
            * => {},
        }

        if (node.hasTag("br")) {
            _.display = Display.inlineFlow();
        }
        if (tag == .Svg || node.hasTag("svg")) {
            _.display = Display.inlineFlow();
            _.overflow_x = OverflowMode.Hidden;
            _.overflow_y = OverflowMode.Hidden;
            _.overflow_visible = false;
        }

        if (node.hasTag("pre")) {
            _.white_space = WhiteSpace.Pre;
        }

        if (p := parent_style) {
            _.visibility = p.visibility;
            _.direction = p.direction;
            _.list_style_type = p.list_style_type;
            if (!node.hasTag("pre")) {
                _.white_space = p.white_space;
            }
            if (tag != .H1 && tag != .H2 && tag != .H3 && tag != .H4 && tag != .H5 && tag != .H6) {
                _.font_size_px = p.font_size_px;
            }
            _.has_line_height = p.has_line_height;
            _.line_height_is_number = p.line_height_is_number;
            _.line_height_number = p.line_height_number;
            if (p.line_height_is_number) {
                _.line_height_px = _.font_size_px * p.line_height_number;
            } else {
                _.line_height_px = p.line_height_px;
            }
            _.color = p.color;
            _.color_scheme = p.color_scheme;
            _.text_fill_color = p.text_fill_color;
            _.has_text_fill_color = p.has_text_fill_color;
            _.accent_color = p.accent_color;
            _.has_accent_color = p.has_accent_color;
            _.caret_color = p.caret_color;
            _.has_caret_color = p.has_caret_color;
            _.pointer_events_none = p.pointer_events_none;
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
            _.webfont = p.webfont;
            if (tag != .B && tag != .Strong) {
                _.font_weight = p.font_weight;
            }
            _.border_collapse = p.border_collapse;
            _.border_spacing_h = p.border_spacing_h;
            _.border_spacing_v = p.border_spacing_v;
            _.caption_side = p.caption_side;
            let cpi: usize = 0;
            while (cpi < p.custom_prop_count && cpi < 16) : (cpi += 1) {
                _.custom_props[cpi] = CustomProp{
                    name: p.custom_props[cpi].name,
                    value: p.custom_props[cpi].value,
                    weight: 0,
                };
            }
            _.custom_prop_count = @min(p.custom_prop_count, @as(usize, 16));
        }
        if (tag == .B || tag == .Strong) {
            _.font_weight = 700.0;
        }
    }

    fn getFontFamily(): []const u8 {
        return _.font_family_buf[0.._.font_family_len];
    }

    fn getBeforeContent(): []const u8 {
        if (!_.has_before_content) return "";
        return _.before_content_buf[0.._.before_content_len];
    }

    fn getAfterContent(): []const u8 {
        if (!_.has_after_content) return "";
        return _.after_content_buf[0.._.after_content_len];
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
        let txt_fill = if (_.has_text_fill_color) _.text_fill_color else _.color;
        // Per CSS spec, if border-color is not explicitly set, it defaults to `currentColor` (`_.color`)
        let bdr = if (_.has_explicit_border_color) _.border_color else _.color;
        let bdr_t = if (_.has_border_top_color) _.border_top_color else bdr;
        let bdr_r = if (_.has_border_right_color) _.border_right_color else bdr;
        let bdr_b = if (_.has_border_bottom_color) _.border_bottom_color else bdr;
        let bdr_l = if (_.has_border_left_color) _.border_left_color else bdr;
        let out_col = if (_.outline_style_none || _.outline_width <= 0.0)
            CssColor.rgba(0, 0, 0, 0)
        else if (_.has_explicit_outline_color)
            _.outline_color
        else
            _.color;
        let shadow = _.box_shadow;

        const op = std.math.clamp(_.opacity, 0.0, 1.0);
        let cg = _.conic_gradient;
        let bg_grad = _.bg_gradient;
        if (bg_grad.kind != BgGradientKind.None && _.bg_clip_count > 0) {
            bg_grad.clip_text = _.bg_clip_is_text[_.bg_gradient_layer_idx % _.bg_clip_count];
        } else {
            bg_grad.clip_text = false;
        }
        if (op < 1.0) {
            bg.a = @intFromFloat(@as(f32, @floatFromInt(bg.a)) * op);
            txt.a = @intFromFloat(@as(f32, @floatFromInt(txt.a)) * op);
            txt_fill.a = @intFromFloat(@as(f32, @floatFromInt(txt_fill.a)) * op);
            bdr.a = @intFromFloat(@as(f32, @floatFromInt(bdr.a)) * op);
            bdr_t.a = @intFromFloat(@as(f32, @floatFromInt(bdr_t.a)) * op);
            bdr_r.a = @intFromFloat(@as(f32, @floatFromInt(bdr_r.a)) * op);
            bdr_b.a = @intFromFloat(@as(f32, @floatFromInt(bdr_b.a)) * op);
            bdr_l.a = @intFromFloat(@as(f32, @floatFromInt(bdr_l.a)) * op);
            out_col.a = @intFromFloat(@as(f32, @floatFromInt(out_col.a)) * op);
            if (shadow.has_shadow) {
                shadow.color.a = @intFromFloat(@as(f32, @floatFromInt(shadow.color.a)) * op);
            }
            if (cg.has_gradient) {
                let si: usize = 0;
                while (si < cg.stop_count) : (si += 1) {
                    cg.stops[si].color.a = @intFromFloat(@as(f32, @floatFromInt(cg.stops[si].color.a)) * op);
                }
            }
            if (bg_grad.kind != BgGradientKind.None) {
                let gi: usize = 0;
                while (gi < bg_grad.stop_count) : (gi += 1) {
                    bg_grad.stops[gi].color.a = @intFromFloat(@as(f32, @floatFromInt(bg_grad.stops[gi].color.a)) * op);
                }
            }
        }

        const radius_px = _.border_radius.resolvePx(box_width, _.font_size_px, root_font_px, viewport_w, viewport_h);
        const tl_r = _.border_tl_radius.resolvePx(box_width, _.font_size_px, root_font_px, viewport_w, viewport_h);
        const tr_r = _.border_tr_radius.resolvePx(box_width, _.font_size_px, root_font_px, viewport_w, viewport_h);
        const br_r = _.border_br_radius.resolvePx(box_width, _.font_size_px, root_font_px, viewport_w, viewport_h);
        const bl_r = _.border_bl_radius.resolvePx(box_width, _.font_size_px, root_font_px, viewport_w, viewport_h);
        const bw_px = @max(@max(_.border_top_width, _.border_right_width), @max(_.border_bottom_width, _.border_left_width));

        return ResolvedPaintStyle {
            bg_color: bg,
            border_color: bdr,
            border_top_color: bdr_t,
            border_right_color: bdr_r,
            border_bottom_color: bdr_b,
            border_left_color: bdr_l,
            border_top_width: _.border_top_width,
            border_right_width: _.border_right_width,
            border_bottom_width: _.border_bottom_width,
            border_left_width: _.border_left_width,
            text_color: txt,
            text_fill_color: txt_fill,
            has_text_fill_color: _.has_text_fill_color,
            text_decoration_underline: _.text_decoration_underline,
            text_decoration_line_through: _.text_decoration_line_through,
            text_decoration_overline: _.text_decoration_overline,
            accent_color: _.accent_color,
            has_accent_color: _.has_accent_color,
            caret_color: _.caret_color,
            has_caret_color: _.has_caret_color,
            border_width: bw_px,
            border_radius: radius_px,
            border_tl_radius: tl_r,
            border_tr_radius: tr_r,
            border_br_radius: br_r,
            border_bl_radius: bl_r,
            corner_shape_tl: _.corner_shape_tl,
            corner_shape_tr: _.corner_shape_tr,
            corner_shape_br: _.corner_shape_br,
            corner_shape_bl: _.corner_shape_bl,
            outline_width: if (_.outline_style_none) 0.0 else _.outline_width,
            outline_color: out_col,
            outline_offset: _.outline_offset,
            box_shadow: shadow,
            font_size_px: _.font_size_px,
            conic_gradient: cg,
            bg_gradient: bg_grad,
        };
    }

    fn serializeAccentColor(buf: []u8): []const u8 {
        if (!_.has_accent_color) return "auto";
        if (_.accent_color.a == 255) {
            return std.fmt.bufPrint(buf, "rgb({d}, {d}, {d})", .{ _.accent_color.r, _.accent_color.g, _.accent_color.b }) catch "auto";
        }
        const a_f: f32 = @as(f32, @floatFromInt(_.accent_color.a)) / 255.0;
        return std.fmt.bufPrint(buf, "rgba({d}, {d}, {d}, {d:.2})", .{ _.accent_color.r, _.accent_color.g, _.accent_color.b, a_f }) catch "auto";
    }

    fn setCustomProp(*_, name: []const u8, value: []const u8, weight: u64) {
        let i: usize = 0;
        while (i < _.custom_prop_count) : (i += 1) {
            if (std.mem.eql(u8, _.custom_props[i].name, name)) {
                if (weight >= _.custom_props[i].weight) {
                    _.custom_props[i].value = value;
                    _.custom_props[i].weight = weight;
                }
                return;
            }
        }
        if (_.custom_prop_count < 16) {
            _.custom_props[_.custom_prop_count] = CustomProp{
                name: name,
                value: value,
                weight: weight,
            };
            _.custom_prop_count += 1;
        }
    }

    fn getCustomProp(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.custom_prop_count) : (i += 1) {
            if (std.mem.eql(u8, _.custom_props[i].name, name)) {
                return _.custom_props[i].value;
            }
        }
        return null;
    }

    fn resolveVarValue(raw: []const u8): []const u8 {
        let cur = std.mem.trim(u8, raw, " \t\r\n");
        let depth: u32 = 0;
        while (depth < 8) : (depth += 1) {
            if (cur.len < 6) break;
            if (!std.ascii.startsWithIgnoreCase(cur, "var(")) break;
            if (cur[cur.len - 1] != ')') break;
            const inner = std.mem.trim(u8, cur[4 .. cur.len - 1], " \t\r\n");
            let var_name = inner;
            let fallback: []const u8 = "";
            if (comma := std.mem.indexOfScalar(u8, inner, ',')) {
                var_name = std.mem.trim(u8, inner[0..comma], " \t\r\n");
                fallback = std.mem.trim(u8, inner[comma + 1 ..], " \t\r\n");
            }
            if (resolved := _.getCustomProp(var_name)) {
                cur = std.mem.trim(u8, resolved, " \t\r\n");
            } else if (fallback.len > 0) {
                cur = fallback;
            } else {
                return "";
            }
        }
        return cur;
    }

    /// Applies a single `CssProperty` with prepacked 64-bit `weight` onto this ComputedStyle.
    static fn isValidVarCall(inner: []const u8): bool {
        // Split top-level comma inside var(...)
        let depth: i32 = 0;
        let in_sq = false;
        let in_dq = false;
        let comma_idx: usize? = null;
        let i: usize = 0;
        while (i < inner.len) : (i += 1) {
            const c = inner[i];
            if (in_sq) {
                if (c == '\\' && i + 1 < inner.len) { i += 1; continue; }
                if (c == '\'') in_sq = false;
                continue;
            }
            if (in_dq) {
                if (c == '\\' && i + 1 < inner.len) { i += 1; continue; }
                if (c == '"') in_dq = false;
                continue;
            }
            if (c == '\'') { in_sq = true; continue; }
            if (c == '"') { in_dq = true; continue; }
            if (c == '(' || c == '[' || c == '{') { depth += 1; continue; }
            if (c == ')' || c == ']' || c == '}') {
                if (depth == 0) return false;
                depth -= 1;
                continue;
            }
            if (c == ',' && depth == 0) {
                comma_idx = i;
                break;
            }
        }
        if (in_sq || in_dq || depth != 0) return false;
        const name_raw = if (comma_idx != null) inner[0..comma_idx.?] else inner;
        const name_trim = std.mem.trim(u8, name_raw, " \t\r\n");
        if (name_trim.len == 0) return false;
        if (name_trim[0] == '{') {
            if (name_trim[name_trim.len - 1] != '}') return false;
            const b_inner = std.mem.trim(u8, name_trim[1 .. name_trim.len - 1], " \t\r\n");
            if (b_inner.len == 0) return false;
        } else {
            if (std.mem.indexOfAny(u8, name_trim, "{}") != null) return false;
        }
        if (comma_idx != null) {
            const fb = inner[comma_idx.? + 1 ..];
            if (!ComputedStyle.isValidDeclValueInner(fb, true)) return false;
        }
        return true;
    }

    static fn isValidDeclValueInner(raw: []const u8, allow_empty: bool): bool {
        const trimmed = std.mem.trim(u8, raw, " \t\r\n");
        if (!allow_empty && raw.len == 0) return false;
        if (!allow_empty && trimmed.len == 0 && raw.len == 0) return false;
        let stack: [64]u8 = undefined;
        let sp: usize = 0;
        let in_sq = false;
        let in_dq = false;
        let i: usize = 0;
        while (i < raw.len) : (i += 1) {
            const c = raw[i];
            if (in_sq) {
                if (c == '\\' && i + 1 < raw.len) { i += 1; continue; }
                if (c == '\n' || c == '\r') return false;
                if (c == '\'') in_sq = false;
                continue;
            }
            if (in_dq) {
                if (c == '\\' && i + 1 < raw.len) { i += 1; continue; }
                if (c == '\n' || c == '\r') return false;
                if (c == '"') in_dq = false;
                continue;
            }
            if (c == '\'') { in_sq = true; continue; }
            if (c == '"') { in_dq = true; continue; }
            if (sp == 0 && (c == '!' || c == ';')) return false;
            if (c == '(' || c == '[' || c == '{') {
                // Check var(...)
                if (c == '(' && i >= 3) {
                    const prev3 = raw[i - 3 .. i];
                    const is_ident_boundary = (i == 3) || (!std.ascii.isAlphanumeric(raw[i - 4]) && raw[i - 4] != '_' && raw[i - 4] != '-');
                    if (is_ident_boundary && std.ascii.eqlIgnoreCase(prev3, "var")) {
                        // Find matching ')'
                        let vdepth: i32 = 1;
                        let vsq = false;
                        let vdq = false;
                        let j = i + 1;
                        while (j < raw.len && vdepth > 0) : (j += 1) {
                            const vc = raw[j];
                            if (vsq) {
                                if (vc == '\\' && j + 1 < raw.len) { j += 1; continue; }
                                if (vc == '\'') vsq = false;
                                continue;
                            }
                            if (vdq) {
                                if (vc == '\\' && j + 1 < raw.len) { j += 1; continue; }
                                if (vc == '"') vdq = false;
                                continue;
                            }
                            if (vc == '\'') { vsq = true; continue; }
                            if (vc == '"') { vdq = true; continue; }
                            if (vc == '(') vdepth += 1;
                            else if (vc == ')') vdepth -= 1;
                        }
                        if (vdepth != 0) return false;
                        if (!ComputedStyle.isValidVarCall(raw[i + 1 .. j - 1])) return false;
                        i = j - 1;
                        continue;
                    }
                }
                if (sp >= 64) return false;
                stack[sp] = c;
                sp += 1;
                continue;
            }
            if (c == ')' || c == ']' || c == '}') {
                if (sp == 0) return false;
                const open = stack[sp - 1];
                if ((c == ')' && open != '(') || (c == ']' && open != '[') || (c == '}' && open != '{')) return false;
                sp -= 1;
                continue;
            }
        }
        return !in_sq && !in_dq && sp == 0;
    }

    static fn isValidCustomPropValue(raw: []const u8): bool {
        if (raw.len == 0) return false;
        return ComputedStyle.isValidDeclValueInner(raw, false);
    }

    fn applyProperty(*_, prop: *const CssProperty, weight: u64, root_font_px: f32) {
        const raw_name = std.mem.trim(u8, prop.name, " \t\r\n");
        if (std.mem.startsWith(u8, raw_name, "--")) {
            if (!ComputedStyle.isValidCustomPropValue(prop.value)) return;
            _.setCustomProp(raw_name, std.mem.trim(u8, prop.value, " \t\r\n"), weight);
            return;
        }
        const name = canonicalizeCssPropertyName(raw_name);
        const val = _.resolveVarValue(prop.value);

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
        if (std.ascii.eqlIgnoreCase(name, "list-style-type") || std.ascii.eqlIgnoreCase(name, "list-style")) {
            if (lt := ListStyleType.parse(val)) {
                _.list_style_type = lt;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "counter-reset")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n");
            if (cname := it.next()) {
                if (!std.ascii.eqlIgnoreCase(cname, "none")) {
                    const take = @min(cname.len, 32);
                    std.mem.copyForwards(u8, _.counter_reset_name[0..take], cname[0..take]);
                    _.counter_reset_len = take;
                    _.counter_reset_val = 0;
                    if (num_s := it.next()) {
                        _.counter_reset_val = std.fmt.parseInt(i32, num_s, 10) catch 0;
                    }
                    _.has_counter_reset = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "counter-increment")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n");
            if (cname := it.next()) {
                if (!std.ascii.eqlIgnoreCase(cname, "none")) {
                    const take = @min(cname.len, 32);
                    std.mem.copyForwards(u8, _.counter_inc_name[0..take], cname[0..take]);
                    _.counter_inc_len = take;
                    _.counter_inc_val = 1;
                    if (num_s := it.next()) {
                        _.counter_inc_val = std.fmt.parseInt(i32, num_s, 10) catch 1;
                    }
                    _.has_counter_inc = true;
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
            if (pos := CssPosition.parse(val)) {
                if (_.tryClaimSlot(PROP_POSITION, weight)) {
                    _.position = pos;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "width") || std.ascii.eqlIgnoreCase(name, "inline-size")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_WIDTH, weight)) {
                    _.width = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "height") || std.ascii.eqlIgnoreCase(name, "block-size")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_HEIGHT, weight)) {
                    _.height = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "min-width") || std.ascii.eqlIgnoreCase(name, "min-inline-size")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MIN_WIDTH, weight)) {
                    _.min_width = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "min-height") || std.ascii.eqlIgnoreCase(name, "min-block-size")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MIN_HEIGHT, weight)) {
                    _.min_height = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "max-width") || std.ascii.eqlIgnoreCase(name, "max-inline-size")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MAX_WIDTH, weight)) {
                    _.max_width = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "max-height") || std.ascii.eqlIgnoreCase(name, "max-block-size")) {
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
        if (std.ascii.eqlIgnoreCase(name, "margin-top") || std.ascii.eqlIgnoreCase(name, "margin-block-start")) {
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
        if (std.ascii.eqlIgnoreCase(name, "margin-bottom") || std.ascii.eqlIgnoreCase(name, "margin-block-end")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MARGIN_BOTTOM, weight)) _.margin_bottom = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "margin-left") || std.ascii.eqlIgnoreCase(name, "margin-inline-start")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MARGIN_LEFT, weight)) _.margin_left = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "margin-inline-end")) {
            if (lpa := LengthPercentageOrAuto.parse(val)) {
                if (_.tryClaimSlot(PROP_MARGIN_RIGHT, weight)) _.margin_right = lpa;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "margin-inline")) {
            if (quad := Cascade.parseBoxQuad(val)) {
                // For 1 token: top=t1, right=t1. For 2 tokens: top=t1, right=t2.
                if (_.tryClaimSlot(PROP_MARGIN_LEFT, weight)) _.margin_left = quad.top;
                if (_.tryClaimSlot(PROP_MARGIN_RIGHT, weight)) _.margin_right = quad.right;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "margin-block")) {
            if (quad := Cascade.parseBoxQuad(val)) {
                if (_.tryClaimSlot(PROP_MARGIN_TOP, weight)) _.margin_top = quad.top;
                if (_.tryClaimSlot(PROP_MARGIN_BOTTOM, weight)) _.margin_bottom = quad.right;
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
        if (std.ascii.eqlIgnoreCase(name, "border")) {
            const bw = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_BORDER_TOP, weight)) _.border_top_width = bw;
            if (_.tryClaimSlot(PROP_BORDER_RIGHT, weight)) _.border_right_width = bw;
            if (_.tryClaimSlot(PROP_BORDER_BOTTOM, weight)) _.border_bottom_width = bw;
            if (_.tryClaimSlot(PROP_BORDER_LEFT, weight)) _.border_left_width = bw;
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BORDER_COLOR, weight)) {
                    _.border_color = c;
                    _.has_explicit_border_color = true;
                }
                if (_.tryClaimSlot(PROP_BORDER_TOP_COLOR, weight)) { _.border_top_color = c; _.has_border_top_color = true; }
                if (_.tryClaimSlot(PROP_BORDER_RIGHT_COLOR, weight)) { _.border_right_color = c; _.has_border_right_color = true; }
                if (_.tryClaimSlot(PROP_BORDER_BOTTOM_COLOR, weight)) { _.border_bottom_color = c; _.has_border_bottom_color = true; }
                if (_.tryClaimSlot(PROP_BORDER_LEFT_COLOR, weight)) { _.border_left_color = c; _.has_border_left_color = true; }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-width")) {
            let bws: [4]f32 = [0.0; 4];
            let cnt: usize = 0;
            let pos: usize = 0;
            while (cnt < 4) {
                const tok_opt = nextCssValueToken(val, &pos);
                if (tok_opt == null) break;
                bws[cnt] = Cascade.parseBorderWidthToken(tok_opt.?, root_font_px);
                cnt += 1;
            }
            if (cnt > 0) {
                const t_w = bws[0];
                const r_w = if (cnt >= 2) bws[1] else bws[0];
                const b_w = if (cnt >= 3) bws[2] else bws[0];
                const l_w = if (cnt >= 4) bws[3] else r_w;
                if (_.tryClaimSlot(PROP_BORDER_TOP, weight)) _.border_top_width = t_w;
                if (_.tryClaimSlot(PROP_BORDER_RIGHT, weight)) _.border_right_width = r_w;
                if (_.tryClaimSlot(PROP_BORDER_BOTTOM, weight)) _.border_bottom_width = b_w;
                if (_.tryClaimSlot(PROP_BORDER_LEFT, weight)) _.border_left_width = l_w;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-top") || std.ascii.eqlIgnoreCase(name, "border-top-width")) {
            const bw = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_BORDER_TOP, weight)) _.border_top_width = bw;
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BORDER_TOP_COLOR, weight)) {
                    _.border_top_color = c;
                    _.has_border_top_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-right") || std.ascii.eqlIgnoreCase(name, "border-right-width")) {
            const bw = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_BORDER_RIGHT, weight)) _.border_right_width = bw;
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BORDER_RIGHT_COLOR, weight)) {
                    _.border_right_color = c;
                    _.has_border_right_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-bottom") || std.ascii.eqlIgnoreCase(name, "border-bottom-width")) {
            const bw = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_BORDER_BOTTOM, weight)) _.border_bottom_width = bw;
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BORDER_BOTTOM_COLOR, weight)) {
                    _.border_bottom_color = c;
                    _.has_border_bottom_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-left") || std.ascii.eqlIgnoreCase(name, "border-left-width")) {
            const bw = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_BORDER_LEFT, weight)) _.border_left_width = bw;
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BORDER_LEFT_COLOR, weight)) {
                    _.border_left_color = c;
                    _.has_border_left_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-color")) {
            let cols: [4]CssColor = [CssColor.rgb(0, 0, 0); 4];
            let cnt: usize = 0;
            let pos: usize = 0;
            while (cnt < 4) {
                const tok_opt = nextCssValueToken(val, &pos);
                if (tok_opt == null) break;
                if (c := CssColor.parse(tok_opt.?)) {
                    cols[cnt] = c;
                    cnt += 1;
                }
            }
            if (cnt > 0) {
                const tc = cols[0];
                const rc = if (cnt >= 2) cols[1] else cols[0];
                const bc = if (cnt >= 3) cols[2] else cols[0];
                const lc = if (cnt >= 4) cols[3] else rc;
                if (_.tryClaimSlot(PROP_BORDER_COLOR, weight)) {
                    _.border_color = tc;
                    _.has_explicit_border_color = true;
                }
                if (_.tryClaimSlot(PROP_BORDER_TOP_COLOR, weight)) { _.border_top_color = tc; _.has_border_top_color = true; }
                if (_.tryClaimSlot(PROP_BORDER_RIGHT_COLOR, weight)) { _.border_right_color = rc; _.has_border_right_color = true; }
                if (_.tryClaimSlot(PROP_BORDER_BOTTOM_COLOR, weight)) { _.border_bottom_color = bc; _.has_border_bottom_color = true; }
                if (_.tryClaimSlot(PROP_BORDER_LEFT_COLOR, weight)) { _.border_left_color = lc; _.has_border_left_color = true; }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-top-color")) {
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BORDER_TOP_COLOR, weight)) {
                    _.border_top_color = c;
                    _.has_border_top_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-right-color")) {
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BORDER_RIGHT_COLOR, weight)) {
                    _.border_right_color = c;
                    _.has_border_right_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-bottom-color")) {
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BORDER_BOTTOM_COLOR, weight)) {
                    _.border_bottom_color = c;
                    _.has_border_bottom_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-left-color")) {
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BORDER_LEFT_COLOR, weight)) {
                    _.border_left_color = c;
                    _.has_border_left_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-radius")) {
            // Take the horizontal radii part (before optional '/')
            const slash_idx = std.mem.indexOfScalar(u8, val, '/');
            const part = if (slash_idx != null) val[0..slash_idx.?] else val;
            let rads: [4]LengthPercentageOrAuto = [LengthPercentageOrAuto.zeroPx(); 4];
            let cnt: usize = 0;
            let it = std.mem.tokenizeAny(u8, part, " \t\r\n");
            while (cnt < 4) {
                const tok_opt = it.next();
                if (tok_opt == null) break;
                if (lpa := LengthPercentageOrAuto.parse(tok_opt.?)) {
                    rads[cnt] = lpa;
                    cnt += 1;
                }
            }
            if (cnt > 0) {
                const tl = rads[0];
                const tr = if (cnt >= 2) rads[1] else rads[0];
                const br = if (cnt >= 3) rads[2] else rads[0];
                const bl = if (cnt >= 4) rads[3] else tr;
                if (_.tryClaimSlot(PROP_BORDER_RADIUS, weight)) _.border_radius = tl;
                if (_.tryClaimSlot(PROP_BORDER_TL_RADIUS, weight)) _.border_tl_radius = tl;
                if (_.tryClaimSlot(PROP_BORDER_TR_RADIUS, weight)) _.border_tr_radius = tr;
                if (_.tryClaimSlot(PROP_BORDER_BR_RADIUS, weight)) _.border_br_radius = br;
                if (_.tryClaimSlot(PROP_BORDER_BL_RADIUS, weight)) _.border_bl_radius = bl;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-top-left-radius")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n/");
            if (t := it.next()) {
                if (lpa := LengthPercentageOrAuto.parse(t)) {
                    if (_.tryClaimSlot(PROP_BORDER_TL_RADIUS, weight)) _.border_tl_radius = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-top-right-radius")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n/");
            if (t := it.next()) {
                if (lpa := LengthPercentageOrAuto.parse(t)) {
                    if (_.tryClaimSlot(PROP_BORDER_TR_RADIUS, weight)) _.border_tr_radius = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-bottom-right-radius")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n/");
            if (t := it.next()) {
                if (lpa := LengthPercentageOrAuto.parse(t)) {
                    if (_.tryClaimSlot(PROP_BORDER_BR_RADIUS, weight)) _.border_br_radius = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-bottom-left-radius")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n/");
            if (t := it.next()) {
                if (lpa := LengthPercentageOrAuto.parse(t)) {
                    if (_.tryClaimSlot(PROP_BORDER_BL_RADIUS, weight)) _.border_bl_radius = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "corner-shape")) {
            let shapes: [4]CornerShape = [CornerShape.Round; 4];
            let cnt: usize = 0;
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n");
            while (cnt < 4) {
                const tok_opt = it.next();
                if (tok_opt == null) break;
                if (cs := CornerShape.parse(tok_opt.?)) {
                    shapes[cnt] = cs;
                    cnt += 1;
                }
            }
            if (cnt > 0 && _.tryClaimSlot(PROP_CORNER_SHAPE, weight)) {
                _.corner_shape_tl = shapes[0];
                _.corner_shape_tr = if (cnt >= 2) shapes[1] else shapes[0];
                _.corner_shape_br = if (cnt >= 3) shapes[2] else shapes[0];
                _.corner_shape_bl = if (cnt >= 4) shapes[3] else _.corner_shape_tr;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "box-shadow")) {
            if (_.tryClaimSlot(PROP_BOX_SHADOW, weight)) {
                const trimmed = std.mem.trim(u8, val, " \t\r\n");
                if (std.ascii.eqlIgnoreCase(trimmed, "none") || trimmed.len == 0) {
                    _.box_shadow = CssBoxShadow{};
                } else {
                    let sh = CssBoxShadow{ has_shadow: true };
                    let nums: [4]f32 = [0.0; 4];
                    let n_cnt: usize = 0;
                    let pos: usize = 0;
                    while (true) {
                        const tok_opt = nextCssValueToken(trimmed, &pos);
                        if (tok_opt == null) break;
                        const tok = tok_opt.?;
                        if (std.ascii.eqlIgnoreCase(tok, "inset")) {
                            sh.inset = true;
                        } else if (c := CssColor.parse(tok)) {
                            sh.color = c;
                        } else if (l := CssLength.parse(tok)) {
                            if (n_cnt < 4) {
                                nums[n_cnt] = @floatCast(l.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                                n_cnt += 1;
                            }
                        }
                    }
                    if (n_cnt >= 2) {
                        sh.offset_x = nums[0];
                        sh.offset_y = nums[1];
                        if (n_cnt >= 3) sh.blur = nums[2];
                        if (n_cnt >= 4) sh.spread = nums[3];
                        _.box_shadow = sh;
                    }
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "outline")) {
            const trimmed = std.mem.trim(u8, val, " \t\r\n");
            if (std.ascii.eqlIgnoreCase(trimmed, "none")) {
                if (_.tryClaimSlot(PROP_OUTLINE_STYLE, weight)) _.outline_style_none = true;
                if (_.tryClaimSlot(PROP_OUTLINE_WIDTH, weight)) _.outline_width = 0.0;
                return;
            }
            let pos: usize = 0;
            let saw_style = false;
            let saw_width = false;
            while (true) {
                const tok_opt = nextCssValueToken(trimmed, &pos);
                if (tok_opt == null) break;
                const tok = tok_opt.?;
                if (std.ascii.eqlIgnoreCase(tok, "none") || std.ascii.eqlIgnoreCase(tok, "hidden")) {
                    if (_.tryClaimSlot(PROP_OUTLINE_STYLE, weight)) _.outline_style_none = true;
                    saw_style = true;
                } else if (std.ascii.eqlIgnoreCase(tok, "solid") || std.ascii.eqlIgnoreCase(tok, "dashed") || std.ascii.eqlIgnoreCase(tok, "dotted") || std.ascii.eqlIgnoreCase(tok, "double") || std.ascii.eqlIgnoreCase(tok, "auto") || std.ascii.eqlIgnoreCase(tok, "groove") || std.ascii.eqlIgnoreCase(tok, "ridge") || std.ascii.eqlIgnoreCase(tok, "inset") || std.ascii.eqlIgnoreCase(tok, "outset")) {
                    if (_.tryClaimSlot(PROP_OUTLINE_STYLE, weight)) _.outline_style_none = false;
                    saw_style = true;
                } else if (c := CssColor.parse(tok)) {
                    if (_.tryClaimSlot(PROP_OUTLINE_COLOR, weight)) {
                        _.outline_color = c;
                        _.has_explicit_outline_color = true;
                    }
                } else {
                    const ow = Cascade.parseBorderWidthToken(tok, root_font_px);
                    if (ow > 0.0 || std.mem.eql(u8, tok, "0") || std.ascii.eqlIgnoreCase(tok, "0px")) {
                        if (_.tryClaimSlot(PROP_OUTLINE_WIDTH, weight)) _.outline_width = ow;
                        saw_width = true;
                    }
                }
            }
            if (saw_style && !_.outline_style_none && !saw_width) {
                if (_.tryClaimSlot(PROP_OUTLINE_WIDTH, weight)) _.outline_width = 3.0;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "outline-width")) {
            const ow = Cascade.parseBorderWidthToken(val, root_font_px);
            if (_.tryClaimSlot(PROP_OUTLINE_WIDTH, weight)) _.outline_width = ow;
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "outline-style")) {
            const s = std.mem.trim(u8, val, " \t\r\n");
            if (_.tryClaimSlot(PROP_OUTLINE_STYLE, weight)) {
                _.outline_style_none = std.ascii.eqlIgnoreCase(s, "none") || std.ascii.eqlIgnoreCase(s, "hidden");
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "outline-color")) {
            if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_OUTLINE_COLOR, weight)) {
                    _.outline_color = c;
                    _.has_explicit_outline_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "outline-offset")) {
            if (l := CssLength.parse(val)) {
                if (_.tryClaimSlot(PROP_OUTLINE_OFFSET, weight)) {
                    _.outline_offset = @floatCast(l.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "clip")) {
            if (_.tryClaimSlot(PROP_CLIP, weight)) {
                const s = std.mem.trim(u8, val, " \t\r\n");
                if (std.ascii.eqlIgnoreCase(s, "auto") || std.ascii.eqlIgnoreCase(s, "none")) {
                    _.legacy_clip = LegacyClipRect{};
                } else if (std.ascii.startsWithIgnoreCase(s, "rect(")) {
                    if (close_idx := std.mem.lastIndexOfScalar(u8, s, ')')) {
                        if (close_idx > 5) {
                            const inner = s[5..close_idx];
                            let args: [4]LengthPercentageOrAuto = [LengthPercentageOrAuto.autoVal(); 4];
                            let cnt: usize = 0;
                            let it = std.mem.tokenizeAny(u8, inner, ", \t\r\n");
                            while (cnt < 4) {
                                const tok_opt = it.next();
                                if (tok_opt == null) break;
                                if (lpa := LengthPercentageOrAuto.parse(tok_opt.?)) {
                                    args[cnt] = lpa;
                                    cnt += 1;
                                }
                            }
                            if (cnt == 4) {
                                _.legacy_clip = LegacyClipRect {
                                    has_clip: true,
                                    top: args[0],
                                    right: args[1],
                                    bottom: args[2],
                                    left: args[3],
                                };
                            }
                        }
                    }
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "clip-path")) {
            if (_.tryClaimSlot(PROP_CLIP_PATH, weight)) {
                const s = std.mem.trim(u8, val, " \t\r\n");
                if (std.ascii.eqlIgnoreCase(s, "none")) {
                    _.clip_path_inset = ClipPathInset{};
                } else if (std.ascii.startsWithIgnoreCase(s, "inset(")) {
                    if (close_idx := std.mem.lastIndexOfScalar(u8, s, ')')) {
                        if (close_idx > 6) {
                            const inner = s[6..close_idx];
                            let args: [4]LengthPercentageOrAuto = [LengthPercentageOrAuto.zeroPx(); 4];
                            let cnt: usize = 0;
                            let it = std.mem.tokenizeAny(u8, inner, " \t\r\n");
                            while (cnt < 4) {
                                const tok_opt = it.next();
                                if (tok_opt == null) break;
                                if (std.ascii.eqlIgnoreCase(tok_opt.?, "round")) break;
                                if (lpa := LengthPercentageOrAuto.parse(tok_opt.?)) {
                                    args[cnt] = lpa;
                                    cnt += 1;
                                }
                            }
                            if (cnt > 0) {
                                const t = args[0];
                                const r = if (cnt >= 2) args[1] else args[0];
                                const b = if (cnt >= 3) args[2] else args[0];
                                const l = if (cnt >= 4) args[3] else r;
                                _.clip_path_inset = ClipPathInset {
                                    has_clip: true,
                                    top: t,
                                    right: r,
                                    bottom: b,
                                    left: l,
                                };
                            }
                        }
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
        if (std.ascii.eqlIgnoreCase(name, "flex-wrap") || std.ascii.eqlIgnoreCase(name, "-webkit-flex-wrap")) {
            if (fw := FlexWrap.parse(val)) {
                if (_.tryClaimSlot(PROP_FLEX_WRAP, weight)) _.flex_wrap = fw;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "flex-flow")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n");
            while (tok := it.next()) {
                if (fd := FlexDirection.parse(tok)) {
                    if (_.tryClaimSlot(PROP_FLEX_DIRECTION, weight)) _.flex_direction = fd;
                } else if (fw := FlexWrap.parse(tok)) {
                    if (_.tryClaimSlot(PROP_FLEX_WRAP, weight)) _.flex_wrap = fw;
                }
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

        // Grid template shorthand (`grid: 40px 40px / 40px 40px` or `grid-template: ...`)
        if (std.ascii.eqlIgnoreCase(name, "grid") || std.ascii.eqlIgnoreCase(name, "grid-template")) {
            const trimmed = std.mem.trim(u8, val, " \t\r\n");
            if (slash_idx := std.mem.indexOfScalar(u8, trimmed, '/')) {
                const rows_part = std.mem.trim(u8, trimmed[0..slash_idx], " \t\r\n");
                const cols_part = std.mem.trim(u8, trimmed[slash_idx + 1 ..], " \t\r\n");
                if (rows_part.len > 0 && _.tryClaimSlot(PROP_GRID_ROWS, weight)) {
                    let r_cnt: u32 = 0;
                    let rit = std.mem.tokenizeAny(u8, rows_part, " \t\r\n");
                    while (rtok := rit.next()) {
                        if (r_cnt < 16) {
                            if (std.mem.endsWith(u8, rtok, "fr")) {
                                _.grid_row_tracks[@as(usize, r_cnt)] = LengthPercentageOrAuto.autoVal();
                            } else if (lpa := LengthPercentageOrAuto.parse(rtok)) {
                                _.grid_row_tracks[@as(usize, r_cnt)] = lpa;
                            } else {
                                _.grid_row_tracks[@as(usize, r_cnt)] = LengthPercentageOrAuto.autoVal();
                            }
                        }
                        r_cnt += 1;
                    }
                    _.grid_row_track_count = @min(r_cnt, 16);
                }
                if (cols_part.len > 0 && _.tryClaimSlot(PROP_GRID_COLS, weight)) {
                    let c_cnt: u32 = 0;
                    let cit = std.mem.tokenizeAny(u8, cols_part, " \t\r\n");
                    while (ctok := cit.next()) {
                        if (c_cnt < 16) {
                            if (std.mem.endsWith(u8, ctok, "fr")) {
                                _.grid_col_tracks[@as(usize, c_cnt)] = LengthPercentageOrAuto.autoVal();
                            } else if (lpa := LengthPercentageOrAuto.parse(ctok)) {
                                _.grid_col_tracks[@as(usize, c_cnt)] = lpa;
                            } else {
                                _.grid_col_tracks[@as(usize, c_cnt)] = LengthPercentageOrAuto.autoVal();
                            }
                        }
                        c_cnt += 1;
                    }
                    if (c_cnt > 0) {
                        _.grid_cols = c_cnt;
                        _.grid_col_track_count = @min(c_cnt, 16);
                    }
                }
            }
            return;
        }

        // Grid template rows (e.g. `40px 40px` or `repeat(2, 40px)`)
        if (std.ascii.eqlIgnoreCase(name, "grid-template-rows")) {
            const trimmed = std.mem.trim(u8, val, " \t\r\n");
            if (_.tryClaimSlot(PROP_GRID_ROWS, weight)) {
                if (std.ascii.startsWithIgnoreCase(trimmed, "repeat(")) {
                    if (comma_idx := std.mem.indexOfScalar(u8, trimmed, ',')) {
                        const cnt_str = std.mem.trim(u8, trimmed[7..comma_idx], " \t");
                        const rows = std.fmt.parseInt(u32, cnt_str, 10) catch 0;
                        let track_val = LengthPercentageOrAuto.autoVal();
                        if (trimmed.len > comma_idx + 1 && trimmed[trimmed.len - 1] == ')') {
                            const inner = std.mem.trim(u8, trimmed[comma_idx + 1 .. trimmed.len - 1], " \t");
                            if (!std.mem.endsWith(u8, inner, "fr")) {
                                if (lpa := LengthPercentageOrAuto.parse(inner)) {
                                    track_val = lpa;
                                }
                            }
                        }
                        const clamped = @min(rows, 16);
                        let ri: u32 = 0;
                        while (ri < clamped) : (ri += 1) {
                            _.grid_row_tracks[@as(usize, ri)] = track_val;
                        }
                        _.grid_row_track_count = clamped;
                    }
                } else {
                    let r_cnt: u32 = 0;
                    let rit = std.mem.tokenizeAny(u8, trimmed, " \t\r\n");
                    while (rtok := rit.next()) {
                        if (r_cnt < 16) {
                            if (std.mem.endsWith(u8, rtok, "fr")) {
                                _.grid_row_tracks[@as(usize, r_cnt)] = LengthPercentageOrAuto.autoVal();
                            } else if (lpa := LengthPercentageOrAuto.parse(rtok)) {
                                _.grid_row_tracks[@as(usize, r_cnt)] = lpa;
                            } else {
                                _.grid_row_tracks[@as(usize, r_cnt)] = LengthPercentageOrAuto.autoVal();
                            }
                        }
                        r_cnt += 1;
                    }
                    _.grid_row_track_count = @min(r_cnt, 16);
                }
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
                        let track_val = LengthPercentageOrAuto.autoVal();
                        if (trimmed.len > comma_idx + 1 && trimmed[trimmed.len - 1] == ')') {
                            const inner = std.mem.trim(u8, trimmed[comma_idx + 1 .. trimmed.len - 1], " \t");
                            if (!std.mem.endsWith(u8, inner, "fr")) {
                                if (lpa := LengthPercentageOrAuto.parse(inner)) {
                                    track_val = lpa;
                                }
                            }
                        }
                        const clamped = @min(_.grid_cols, 16);
                        let ci: u32 = 0;
                        while (ci < clamped) : (ci += 1) {
                            _.grid_col_tracks[@as(usize, ci)] = track_val;
                        }
                        _.grid_col_track_count = clamped;
                    }
                }
            } else {
                let cols: u32 = 0;
                let parsed_tracks: [16]LengthPercentageOrAuto = [LengthPercentageOrAuto.autoVal(); 16];
                let it = std.mem.tokenizeAny(u8, trimmed, " \t\r\n");
                while (ctok := it.next()) {
                    if (cols < 16) {
                        if (std.mem.endsWith(u8, ctok, "fr")) {
                            parsed_tracks[@as(usize, cols)] = LengthPercentageOrAuto.autoVal();
                        } else if (lpa := LengthPercentageOrAuto.parse(ctok)) {
                            parsed_tracks[@as(usize, cols)] = lpa;
                        } else {
                            parsed_tracks[@as(usize, cols)] = LengthPercentageOrAuto.autoVal();
                        }
                    }
                    cols += 1;
                }
                if (cols > 0 && _.tryClaimSlot(PROP_GRID_COLS, weight)) {
                    _.grid_cols = cols;
                    const clamped = @min(cols, 16);
                    let ci: u32 = 0;
                    while (ci < clamped) : (ci += 1) {
                        _.grid_col_tracks[@as(usize, ci)] = parsed_tracks[@as(usize, ci)];
                    }
                    _.grid_col_track_count = clamped;
                }
            }
            return;
        }

        // Grid area shorthand (`grid-area: row-start / col-start / row-end / col-end`)
        if (std.ascii.eqlIgnoreCase(name, "grid-area")) {
            let parts: [4][]const u8 = [""; 4];
            let pcount: usize = 0;
            let pit = std.mem.splitScalar(u8, val, '/');
            while (seg := pit.next()) {
                if (pcount < 4) {
                    parts[pcount] = std.mem.trim(u8, seg, " \t\r\n");
                    pcount += 1;
                }
            }
            if (pcount >= 1) {
                const n = std.fmt.parseInt(i32, parts[0], 10) catch 0;
                if (n > 0 && _.tryClaimSlot(PROP_GRID_ROW_START, weight)) {
                    _.grid_row_start = n;
                }
            }
            if (pcount >= 2) {
                const n = std.fmt.parseInt(i32, parts[1], 10) catch 0;
                if (n > 0 && _.tryClaimSlot(PROP_GRID_COL_START, weight)) {
                    _.grid_col_start = n;
                }
            }
            if (pcount >= 3) {
                const n = std.fmt.parseInt(i32, parts[2], 10) catch 0;
                if (n > 0 && _.tryClaimSlot(PROP_GRID_ROW_END, weight)) {
                    _.grid_row_end = n;
                }
            }
            if (pcount >= 4) {
                const n = std.fmt.parseInt(i32, parts[3], 10) catch 0;
                if (n > 0 && _.tryClaimSlot(PROP_GRID_COL_END, weight)) {
                    _.grid_col_end = n;
                }
            }
            return;
        }

        // Grid row placement (`grid-row`, `grid-row-start`, `grid-row-end`)
        if (std.ascii.eqlIgnoreCase(name, "grid-row")) {
            const trimmed = std.mem.trim(u8, val, " \t\r\n");
            if (slash_idx := std.mem.indexOfScalar(u8, trimmed, '/')) {
                const s1 = std.mem.trim(u8, trimmed[0..slash_idx], " \t");
                const s2 = std.mem.trim(u8, trimmed[slash_idx + 1 ..], " \t");
                const n1 = std.fmt.parseInt(i32, s1, 10) catch 0;
                const n2 = std.fmt.parseInt(i32, s2, 10) catch 0;
                if (n1 > 0 && _.tryClaimSlot(PROP_GRID_ROW_START, weight)) _.grid_row_start = n1;
                if (n2 > 0 && _.tryClaimSlot(PROP_GRID_ROW_END, weight)) _.grid_row_end = n2;
            } else {
                const n1 = std.fmt.parseInt(i32, trimmed, 10) catch 0;
                if (n1 > 0 && _.tryClaimSlot(PROP_GRID_ROW_START, weight)) _.grid_row_start = n1;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "grid-row-start")) {
            const n = std.fmt.parseInt(i32, std.mem.trim(u8, val, " \t\r\n"), 10) catch 0;
            if (n > 0 && _.tryClaimSlot(PROP_GRID_ROW_START, weight)) _.grid_row_start = n;
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "grid-row-end")) {
            const n = std.fmt.parseInt(i32, std.mem.trim(u8, val, " \t\r\n"), 10) catch 0;
            if (n > 0 && _.tryClaimSlot(PROP_GRID_ROW_END, weight)) _.grid_row_end = n;
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
                if (n1 > 0 && _.tryClaimSlot(PROP_GRID_COL_START, weight)) {
                    _.grid_col_start = @intCast(n1);
                }
                if (n2 > 0 && _.tryClaimSlot(PROP_GRID_COL_END, weight)) {
                    _.grid_col_end = @intCast(n2);
                }
                if (n2 > n1 && n1 > 0) {
                    if (_.tryClaimSlot(PROP_GRID_COL_SPAN, weight)) {
                        _.grid_col_span = n2 - n1;
                    }
                }
            } else {
                const n = std.fmt.parseInt(i32, trimmed, 10) catch 0;
                if (n > 0) {
                    if (std.ascii.eqlIgnoreCase(name, "grid-column-end")) {
                        if (_.tryClaimSlot(PROP_GRID_COL_END, weight)) _.grid_col_end = n;
                    } else {
                        if (_.tryClaimSlot(PROP_GRID_COL_START, weight)) _.grid_col_start = n;
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
        if (std.ascii.eqlIgnoreCase(name, "font")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n");
            let found_size = false;
            let fam_pos: usize = 0;
            while (tok := it.next()) {
                if (!found_size) {
                    if (slash_idx := std.mem.indexOfScalar(u8, tok, '/')) {
                        const sz_str = tok[0..slash_idx];
                        const lh_str = tok[slash_idx + 1 ..];
                        if (l := CssLength.parse(sz_str)) {
                            let fs: f32 = 16.0;
                            if (l.unit == CssUnit.Percent) {
                                const ratio: f32 = @floatCast(l.value / 100.0);
                                fs = _.font_size_px * ratio;
                            } else if (l.unit == CssUnit.Em) {
                                const mult: f32 = @floatCast(l.value);
                                fs = _.font_size_px * mult;
                            } else {
                                fs = @floatCast(l.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                            }
                            if (_.tryClaimSlot(PROP_FONT_SIZE, weight)) _.font_size_px = fs;
                            found_size = true;
                        }
                        if (std.ascii.eqlIgnoreCase(std.mem.trim(u8, lh_str, " \t\r\n"), "normal")) {
                            if (_.tryClaimSlot(PROP_LINE_HEIGHT, weight)) {
                                _.has_line_height = false;
                                _.line_height_is_number = false;
                                _.line_height_number = 0.0;
                                _.line_height_px = 0.0;
                            }
                        } else if (lh_l := CssLength.parse(lh_str)) {
                            let lh_px: f32 = 0.0;
                            let is_num = false;
                            let num_v: f32 = 0.0;
                            if (lh_l.unit == CssUnit.Unitless) {
                                is_num = true;
                                num_v = @floatCast(lh_l.value);
                                lh_px = _.font_size_px * num_v;
                            } else if (lh_l.unit == CssUnit.Em) {
                                const mult: f32 = @floatCast(lh_l.value);
                                lh_px = _.font_size_px * mult;
                            } else if (lh_l.unit == CssUnit.Percent) {
                                const ratio: f32 = @floatCast(lh_l.value / 100.0);
                                lh_px = _.font_size_px * ratio;
                            } else {
                                lh_px = @floatCast(lh_l.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                            }
                            if (_.tryClaimSlot(PROP_LINE_HEIGHT, weight)) {
                                _.has_line_height = true;
                                _.line_height_is_number = is_num;
                                _.line_height_number = num_v;
                                _.line_height_px = lh_px;
                            }
                        }
                    } else if (l := CssLength.parse(tok)) {
                        if (l.unit != CssUnit.Unitless) {
                            let fs: f32 = 16.0;
                            if (l.unit == CssUnit.Percent) {
                                const ratio: f32 = @floatCast(l.value / 100.0);
                                fs = _.font_size_px * ratio;
                            } else if (l.unit == CssUnit.Em) {
                                const mult: f32 = @floatCast(l.value);
                                fs = _.font_size_px * mult;
                            } else {
                                fs = @floatCast(l.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                            }
                            if (_.tryClaimSlot(PROP_FONT_SIZE, weight)) _.font_size_px = fs;
                            if (_.tryClaimSlot(PROP_LINE_HEIGHT, weight)) {
                                _.has_line_height = false;
                                _.line_height_is_number = false;
                                _.line_height_number = 0.0;
                                _.line_height_px = 0.0;
                            }
                            found_size = true;
                        }
                    }
                } else {
                    if (fam_pos == 0) {
                        if (idx := std.mem.indexOf(u8, val, tok)) {
                            const fam = std.mem.trim(u8, val[idx..], " \t\r\n");
                            if (_.tryClaimSlot(PROP_FONT_FAMILY, weight)) {
                                const copy_len = @min(fam.len, _.font_family_buf.len);
                                @memcpy(_.font_family_buf[0..copy_len], fam[0..copy_len]);
                                _.font_family_len = copy_len;
                            }
                            fam_pos = 1;
                        }
                    }
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "font-size")) {
            if (l := CssLength.parse(val)) {
                let fs: f32 = 16.0;
                if (l.unit == CssUnit.Percent) {
                    const ratio: f32 = @floatCast(l.value / 100.0);
                    fs = _.font_size_px * ratio;
                } else if (l.unit == CssUnit.Em) {
                    const mult: f32 = @floatCast(l.value);
                    fs = _.font_size_px * mult;
                } else {
                    fs = @floatCast(l.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                }
                if (_.tryClaimSlot(PROP_FONT_SIZE, weight)) _.font_size_px = fs;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "line-height")) {
            const trimmed_lh = std.mem.trim(u8, val, " \t\r\n");
            if (std.ascii.eqlIgnoreCase(trimmed_lh, "normal")) {
                if (_.tryClaimSlot(PROP_LINE_HEIGHT, weight)) {
                    _.has_line_height = false;
                    _.line_height_is_number = false;
                    _.line_height_number = 0.0;
                    _.line_height_px = 0.0;
                }
            } else if (l := CssLength.parse(trimmed_lh)) {
                let lh_px: f32 = 0.0;
                let is_num = false;
                let num_v: f32 = 0.0;
                if (l.unit == CssUnit.Unitless) {
                    is_num = true;
                    num_v = @floatCast(l.value);
                    lh_px = _.font_size_px * num_v;
                } else if (l.unit == CssUnit.Em) {
                    const mult: f32 = @floatCast(l.value);
                    lh_px = _.font_size_px * mult;
                } else if (l.unit == CssUnit.Percent) {
                    const ratio: f32 = @floatCast(l.value / 100.0);
                    lh_px = _.font_size_px * ratio;
                } else {
                    lh_px = @floatCast(l.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                }
                if (_.tryClaimSlot(PROP_LINE_HEIGHT, weight)) {
                    _.has_line_height = true;
                    _.line_height_is_number = is_num;
                    _.line_height_number = num_v;
                    _.line_height_px = lh_px;
                }
            } else if (lpa := LengthPercentageOrAuto.parse(trimmed_lh)) {
                if (_.tryClaimSlot(PROP_LINE_HEIGHT, weight)) {
                    _.has_line_height = true;
                    _.line_height_is_number = false;
                    _.line_height_number = 0.0;
                    _.line_height_px = lpa.resolvePx(_.font_size_px, _.font_size_px, root_font_px, 1280.0, 800.0);
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "vertical-align")) {
            const trimmed_va = std.mem.trim(u8, val, " \t\r\n");
            if (std.ascii.eqlIgnoreCase(trimmed_va, "baseline")) {
                if (_.tryClaimSlot(PROP_VERTICAL_ALIGN, weight)) {
                    _.vertical_align_kind = VerticalAlignKind.Baseline;
                    _.vertical_align_length = LengthPercentageOrAuto.zeroPx();
                }
            } else if (std.ascii.eqlIgnoreCase(trimmed_va, "top")) {
                if (_.tryClaimSlot(PROP_VERTICAL_ALIGN, weight)) {
                    _.vertical_align_kind = VerticalAlignKind.Top;
                }
            } else if (std.ascii.eqlIgnoreCase(trimmed_va, "bottom")) {
                if (_.tryClaimSlot(PROP_VERTICAL_ALIGN, weight)) {
                    _.vertical_align_kind = VerticalAlignKind.Bottom;
                }
            } else if (std.ascii.eqlIgnoreCase(trimmed_va, "middle")) {
                if (_.tryClaimSlot(PROP_VERTICAL_ALIGN, weight)) {
                    _.vertical_align_kind = VerticalAlignKind.Middle;
                }
            } else if (std.ascii.eqlIgnoreCase(trimmed_va, "text-top")) {
                if (_.tryClaimSlot(PROP_VERTICAL_ALIGN, weight)) {
                    _.vertical_align_kind = VerticalAlignKind.TextTop;
                }
            } else if (std.ascii.eqlIgnoreCase(trimmed_va, "text-bottom")) {
                if (_.tryClaimSlot(PROP_VERTICAL_ALIGN, weight)) {
                    _.vertical_align_kind = VerticalAlignKind.TextBottom;
                }
            } else if (std.ascii.eqlIgnoreCase(trimmed_va, "super")) {
                if (_.tryClaimSlot(PROP_VERTICAL_ALIGN, weight)) {
                    _.vertical_align_kind = VerticalAlignKind.Super;
                }
            } else if (std.ascii.eqlIgnoreCase(trimmed_va, "sub")) {
                if (_.tryClaimSlot(PROP_VERTICAL_ALIGN, weight)) {
                    _.vertical_align_kind = VerticalAlignKind.Sub;
                }
            } else if (lpa := LengthPercentageOrAuto.parse(trimmed_va)) {
                if (_.tryClaimSlot(PROP_VERTICAL_ALIGN, weight)) {
                    if (lpa.length.unit == CssUnit.Percent) {
                        _.vertical_align_kind = VerticalAlignKind.Percent;
                    } else {
                        _.vertical_align_kind = VerticalAlignKind.Length;
                    }
                    _.vertical_align_length = lpa;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "white-space")) {
            if (ws := WhiteSpace.parse(val)) {
                if (_.tryClaimSlot(PROP_WHITE_SPACE, weight)) {
                    _.white_space = ws;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "direction")) {
            if (dir_v := Direction.parse(val)) {
                if (_.tryClaimSlot(PROP_DIRECTION, weight)) {
                    _.direction = dir_v;
                    _.is_dir_auto = false;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "unicode-bidi")) {
            if (ub := UnicodeBidi.parse(val)) {
                if (_.tryClaimSlot(PROP_UNICODE_BIDI, weight)) {
                    _.unicode_bidi = ub;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "-webkit-box-orient") || std.ascii.eqlIgnoreCase(name, "box-orient")) {
            if (bo := BoxOrient.parse(val)) {
                if (_.tryClaimSlot(PROP_BOX_ORIENT, weight)) {
                    _.box_orient = bo;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "-webkit-line-clamp") || std.ascii.eqlIgnoreCase(name, "line-clamp")) {
            const trimmed_lc = std.mem.trim(u8, val, " \t\r\n");
            if (std.ascii.eqlIgnoreCase(trimmed_lc, "none")) {
                if (_.tryClaimSlot(PROP_LINE_CLAMP, weight)) {
                    _.webkit_line_clamp = 0;
                }
            } else if (n := std.fmt.parseInt(u32, trimmed_lc, 10) catch null) {
                if (n > 0 && _.tryClaimSlot(PROP_LINE_CLAMP, weight)) {
                    _.webkit_line_clamp = n;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "color-scheme")) {
            if (cs := ColorScheme.parse(val)) {
                if (_.tryClaimSlot(PROP_COLOR_SCHEME, weight)) _.color_scheme = cs;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "break-before") || std.ascii.eqlIgnoreCase(name, "page-break-before")) {
            if (bv := BreakValue.parse(val)) {
                if (_.tryClaimSlot(PROP_BREAK_BEFORE, weight)) _.break_before = bv;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "break-after") || std.ascii.eqlIgnoreCase(name, "page-break-after")) {
            if (bv := BreakValue.parse(val)) {
                if (_.tryClaimSlot(PROP_BREAK_AFTER, weight)) _.break_after = bv;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "break-inside") || std.ascii.eqlIgnoreCase(name, "page-break-inside")) {
            if (bi := BreakInsideValue.parse(val)) {
                if (_.tryClaimSlot(PROP_BREAK_INSIDE, weight)) _.break_inside = bi;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "color")) {
            if (ld := parseLightDarkPair(val)) {
                if (_.tryClaimSlot(PROP_COLOR, weight)) {
                    _.has_color_light_dark = true;
                    _.color_light = ld.light;
                    _.color_dark = ld.dark;
                    _.color = if (_.color_scheme.isDarkUsed()) ld.dark else ld.light;
                }
            } else if (c := CssColor.parse(val)) {
                if (_.tryClaimSlot(PROP_COLOR, weight)) {
                    _.has_color_light_dark = false;
                    _.color = c;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "-webkit-text-fill-color") || std.ascii.eqlIgnoreCase(name, "text-fill-color")) {
            const tfv = std.mem.trim(u8, val, " \t\r\n");
            if (std.ascii.eqlIgnoreCase(tfv, "currentcolor")) {
                if (_.tryClaimSlot(PROP_TEXT_FILL_COLOR, weight)) {
                    _.has_text_fill_color = false;
                }
            } else if (c := parseGradColor(tfv)) {
                if (_.tryClaimSlot(PROP_TEXT_FILL_COLOR, weight)) {
                    _.text_fill_color = c;
                    _.has_text_fill_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "text-decoration") || std.ascii.eqlIgnoreCase(name, "text-decoration-line")) {
            const is_shorthand = std.ascii.eqlIgnoreCase(name, "text-decoration");
            let ul = false;
            let lt = false;
            let ol = false;
            let parsed_color: CssColor? = null;
            let parsed_thickness: f32 = 0.0;
            let dit = std.mem.tokenizeAny(u8, val, " \t\r\n");
            while (dtok := dit.next()) {
                if (std.ascii.eqlIgnoreCase(dtok, "underline")) {
                    ul = true;
                } else if (std.ascii.eqlIgnoreCase(dtok, "line-through")) {
                    lt = true;
                } else if (std.ascii.eqlIgnoreCase(dtok, "overline")) {
                    ol = true;
                } else if (std.ascii.eqlIgnoreCase(dtok, "none")) {
                    ul = false;
                    lt = false;
                    ol = false;
                } else if (is_shorthand) {
                    if (std.ascii.eqlIgnoreCase(dtok, "solid") || std.ascii.eqlIgnoreCase(dtok, "double") ||
                        std.ascii.eqlIgnoreCase(dtok, "dotted") || std.ascii.eqlIgnoreCase(dtok, "dashed") ||
                        std.ascii.eqlIgnoreCase(dtok, "wavy") || std.ascii.eqlIgnoreCase(dtok, "auto")) {
                        // decoration style keyword
                    } else if (px_v := parseGradPxToken(dtok)) {
                        if (px_v > 0.0) parsed_thickness = px_v;
                    } else if (c := CssColor.parse(dtok)) {
                        parsed_color = c;
                    }
                }
            }
            if (_.tryClaimSlot(PROP_TEXT_DECORATION, weight)) {
                _.text_decoration_underline = ul;
                _.text_decoration_line_through = lt;
                _.text_decoration_overline = ol;
            }
            if (is_shorthand) {
                if (pc := parsed_color) {
                    if (_.tryClaimSlot(PROP_TEXT_DECORATION_COLOR, weight)) {
                        _.text_decoration_color = pc;
                        _.has_text_decoration_color = true;
                    }
                }
                if (parsed_thickness > 0.0) {
                    if (_.tryClaimSlot(PROP_TEXT_DECORATION_THICKNESS, weight)) {
                        _.text_decoration_thickness = parsed_thickness;
                    }
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "text-decoration-color")) {
            if (c := CssColor.parse(std.mem.trim(u8, val, " \t\r\n"))) {
                if (_.tryClaimSlot(PROP_TEXT_DECORATION_COLOR, weight)) {
                    _.text_decoration_color = c;
                    _.has_text_decoration_color = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "text-decoration-thickness")) {
            if (px_v := parseGradPxToken(std.mem.trim(u8, val, " \t\r\n"))) {
                if (px_v > 0.0 && _.tryClaimSlot(PROP_TEXT_DECORATION_THICKNESS, weight)) {
                    _.text_decoration_thickness = px_v;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "text-underline-offset")) {
            if (px_v := parseGradPxToken(std.mem.trim(u8, val, " \t\r\n"))) {
                if (_.tryClaimSlot(PROP_TEXT_UNDERLINE_OFFSET, weight)) {
                    _.text_underline_offset = px_v;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "pointer-events")) {
            const tv = std.mem.trim(u8, val, " \t\r\n");
            if (_.tryClaimSlot(PROP_POINTER_EVENTS, weight)) {
                _.pointer_events_none = std.ascii.eqlIgnoreCase(tv, "none");
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "isolation")) {
            const tv = std.mem.trim(u8, val, " \t\r\n");
            if (_.tryClaimSlot(PROP_ISOLATION, weight)) {
                _.isolation_isolate = std.ascii.eqlIgnoreCase(tv, "isolate");
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "background-clip") || std.ascii.eqlIgnoreCase(name, "-webkit-background-clip")) {
            if (_.tryClaimSlot(PROP_BG_CLIP, weight)) {
                _.bg_clip_count = 0;
                let cit = std.mem.splitScalar(u8, val, ',');
                while (seg := cit.next()) {
                    if (_.bg_clip_count >= 4) break;
                    const t = std.mem.trim(u8, seg, " \t\r\n");
                    _.bg_clip_is_text[_.bg_clip_count] = std.ascii.eqlIgnoreCase(t, "text");
                    _.bg_clip_count += 1;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "background-size")) {
            if (_.tryClaimSlot(PROP_BG_SIZE, weight)) {
                let sit = std.mem.tokenizeAny(u8, val, " \t\r\n");
                if (s0 := sit.next()) {
                    if (v0 := parseGradPxToken(s0)) {
                        if (v0 > 0.0) {
                            _.bg_gradient.tile_w = v0;
                            _.bg_gradient.tile_h = v0;
                        }
                    }
                }
                if (s1 := sit.next()) {
                    if (v1 := parseGradPxToken(s1)) {
                        if (v1 > 0.0) {
                            _.bg_gradient.tile_h = v1;
                        }
                    }
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "background-image")) {
            const trimmed_bg = std.mem.trim(u8, val, " \t\r\n");
            if (std.ascii.eqlIgnoreCase(trimmed_bg, "none")) {
                if (_.tryClaimSlot(PROP_BG_IMAGE, weight)) {
                    _.bg_image_url_len = 0;
                    _.conic_gradient = ConicGradient{};
                    _.bg_gradient = BgGradient{};
                    _.bg_gradient_layer_idx = 0;
                }
            } else {
                // Split top-level comma-separated background-image layers
                let layer_idx: usize = 0;
                let seg_start: usize = 0;
                let pdepth: usize = 0;
                let idx: usize = 0;
                let found_grad = false;
                let found_conic = false;
                let found_url = false;
                let parsed_bg = BgGradient{};
                let parsed_bg_layer: usize = 0;
                let parsed_cg = ConicGradient{};
                let parsed_url: []const u8 = "";

                while (idx <= trimmed_bg.len) : (idx += 1) {
                    const is_end = (idx == trimmed_bg.len);
                    const c: u8 = if (is_end) 0 else trimmed_bg[idx];
                    if (!is_end && c == '(') {
                        pdepth += 1;
                    } else if (!is_end && c == ')') {
                        if (pdepth > 0) pdepth -= 1;
                    } else if (is_end || (c == ',' && pdepth == 0)) {
                        const layer = std.mem.trim(u8, trimmed_bg[seg_start..idx], " \t\r\n");
                        if (layer.len > 0 && !std.ascii.eqlIgnoreCase(layer, "none")) {
                            if (!found_conic) {
                                if (cg := ConicGradient.parse(layer)) {
                                    parsed_cg = cg;
                                    found_conic = true;
                                }
                            }
                            if (!found_grad) {
                                if (bg := BgGradient.parse(layer)) {
                                    parsed_bg = bg;
                                    parsed_bg_layer = layer_idx;
                                    found_grad = true;
                                }
                            }
                            if (!found_url) {
                                if (u_pos := std.mem.indexOf(u8, layer, "url(")) {
                                    if (end_p := std.mem.indexOfScalarPos(u8, layer, u_pos + 4, ')')) {
                                        const raw_u = std.mem.trim(u8, layer[u_pos + 4 .. end_p], " \t\r\n\"'");
                                        if (raw_u.len > 0) {
                                            parsed_url = raw_u;
                                            found_url = true;
                                        }
                                    }
                                }
                            }
                        }
                        layer_idx += 1;
                        seg_start = idx + 1;
                    }
                }
                if (found_conic || found_grad || found_url) {
                    if (_.tryClaimSlot(PROP_BG_IMAGE, weight)) {
                        if (found_conic) {
                            _.conic_gradient = parsed_cg;
                            _.bg_gradient = BgGradient{};
                            _.bg_image_url_len = 0;
                        } else if (found_grad) {
                            // Preserve previously set background-size if gradient itself didn't specify one
                            if (parsed_bg.tile_w <= 0.0 && _.bg_gradient.tile_w > 0.0) {
                                parsed_bg.tile_w = _.bg_gradient.tile_w;
                                parsed_bg.tile_h = _.bg_gradient.tile_h;
                            }
                            _.bg_gradient = parsed_bg;
                            _.bg_gradient_layer_idx = parsed_bg_layer;
                            _.conic_gradient = ConicGradient{};
                            _.bg_image_url_len = 0;
                        } else if (found_url) {
                            const clen = @min(parsed_url.len, _.bg_image_url_buf.len);
                            @memcpy(_.bg_image_url_buf[0..clen], parsed_url[0..clen]);
                            _.bg_image_url_len = clen;
                            _.conic_gradient = ConicGradient{};
                            _.bg_gradient = BgGradient{};
                        }
                    }
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "container-type")) {
            if (ct := ContainerType.parse(val)) {
                if (_.tryClaimSlot(PROP_CONTAINER_TYPE, weight)) {
                    _.container_type = ct;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "container-name")) {
            const cn = std.mem.trim(u8, val, " \t\r\n");
            if (_.tryClaimSlot(PROP_CONTAINER_NAME, weight)) {
                if (cn.len == 0 || std.ascii.eqlIgnoreCase(cn, "none")) {
                    _.container_name_len = 0;
                } else {
                    const clen = @min(cn.len, _.container_name_buf.len);
                    @memcpy(_.container_name_buf[0..clen], cn[0..clen]);
                    _.container_name_len = clen;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "container")) {
            const cv = std.mem.trim(u8, val, " \t\r\n");
            if (slash := std.mem.indexOfScalar(u8, cv, '/')) {
                const name_part = std.mem.trim(u8, cv[0..slash], " \t\r\n");
                const type_part = std.mem.trim(u8, cv[slash + 1 ..], " \t\r\n");
                if (_.tryClaimSlot(PROP_CONTAINER_NAME, weight)) {
                    if (name_part.len == 0 || std.ascii.eqlIgnoreCase(name_part, "none")) {
                        _.container_name_len = 0;
                    } else {
                        const clen = @min(name_part.len, _.container_name_buf.len);
                        @memcpy(_.container_name_buf[0..clen], name_part[0..clen]);
                        _.container_name_len = clen;
                    }
                }
                if (ct := ContainerType.parse(type_part)) {
                    if (_.tryClaimSlot(PROP_CONTAINER_TYPE, weight)) {
                        _.container_type = ct;
                    }
                }
            } else {
                if (_.tryClaimSlot(PROP_CONTAINER_NAME, weight)) {
                    if (cv.len == 0 || std.ascii.eqlIgnoreCase(cv, "none")) {
                        _.container_name_len = 0;
                    } else {
                        const clen = @min(cv.len, _.container_name_buf.len);
                        @memcpy(_.container_name_buf[0..clen], cv[0..clen]);
                        _.container_name_len = clen;
                    }
                }
                if (_.tryClaimSlot(PROP_CONTAINER_TYPE, weight)) {
                    _.container_type = ContainerType.Normal;
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
            if (std.ascii.eqlIgnoreCase(name, "background")) {
                if (cg := ConicGradient.parse(val)) {
                    if (_.tryClaimSlot(PROP_BG_IMAGE, weight)) {
                        _.conic_gradient = cg;
                        _.bg_gradient = BgGradient{};
                    }
                } else if (bg := BgGradient.parse(val)) {
                    if (_.tryClaimSlot(PROP_BG_IMAGE, weight)) {
                        _.bg_gradient = bg;
                        _.bg_gradient_layer_idx = 0;
                        _.conic_gradient = ConicGradient{};
                    }
                }
            }
            if (ld := parseLightDarkPair(val)) {
                if (_.tryClaimSlot(PROP_BG_COLOR, weight)) {
                    _.has_bg_light_dark = true;
                    _.bg_light_color = ld.light;
                    _.bg_dark_color = ld.dark;
                    _.background_color = if (_.color_scheme.isDarkUsed()) ld.dark else ld.light;
                }
            } else if (c := Cascade.parseColorTokenFromShorthand(val)) {
                if (_.tryClaimSlot(PROP_BG_COLOR, weight)) {
                    _.has_bg_light_dark = false;
                    _.background_color = c;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "accent-color")) {
            const av = std.mem.trim(u8, val, " \t\r\n");
            if (std.ascii.eqlIgnoreCase(av, "auto")) {
                if (_.tryClaimSlot(PROP_ACCENT_COLOR, weight)) {
                    _.has_accent_color = false;
                    _.accent_color = CssColor.rgba(0, 0, 0, 0);
                }
            } else if (std.ascii.eqlIgnoreCase(av, "currentcolor")) {
                if (_.tryClaimSlot(PROP_ACCENT_COLOR, weight)) {
                    _.has_accent_color = true;
                    _.accent_color = _.color;
                }
            } else if (c := CssColor.parse(av)) {
                if (_.tryClaimSlot(PROP_ACCENT_COLOR, weight)) {
                    _.has_accent_color = true;
                    _.accent_color = c;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "caret-color")) {
            const cv = std.mem.trim(u8, val, " \t\r\n");
            if (std.ascii.eqlIgnoreCase(cv, "auto")) {
                if (_.tryClaimSlot(PROP_CARET_COLOR, weight)) {
                    _.has_caret_color = false;
                    _.caret_color = CssColor.rgba(0, 0, 0, 0);
                }
            } else if (std.ascii.eqlIgnoreCase(cv, "currentcolor")) {
                if (_.tryClaimSlot(PROP_CARET_COLOR, weight)) {
                    _.has_caret_color = true;
                    _.caret_color = _.color;
                }
            } else if (c := CssColor.parse(cv)) {
                if (_.tryClaimSlot(PROP_CARET_COLOR, weight)) {
                    _.has_caret_color = true;
                    _.caret_color = c;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "opacity")) {
            const op = std.fmt.parseFloat(f32, std.mem.trim(u8, val, " \t")) catch 1.0;
            if (_.tryClaimSlot(PROP_OPACITY, weight)) _.opacity = op;
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "z-index")) {
            const tv = std.mem.trim(u8, val, " \t\r\n");
            if (std.ascii.eqlIgnoreCase(tv, "auto")) {
                if (_.tryClaimSlot(PROP_Z_INDEX, weight)) {
                    _.z_index = 0;
                    _.has_z_index = false;
                }
            } else {
                const z = std.fmt.parseInt(i32, tv, 10) catch 0;
                if (_.tryClaimSlot(PROP_Z_INDEX, weight)) {
                    _.z_index = z;
                    _.has_z_index = true;
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "overflow")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n");
            if (t1 := it.next()) {
                if (m1 := OverflowMode.parse(t1)) {
                    let m2 = m1;
                    if (t2 := it.next()) {
                        if (parsed2 := OverflowMode.parse(t2)) {
                            m2 = parsed2;
                        }
                    }
                    if (_.tryClaimSlot(PROP_OVERFLOW_X, weight)) _.overflow_x = m1;
                    if (_.tryClaimSlot(PROP_OVERFLOW_Y, weight)) _.overflow_y = m2;
                    _.syncOverflowVisible();
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "overflow-x")) {
            if (m := OverflowMode.parse(val)) {
                if (_.tryClaimSlot(PROP_OVERFLOW_X, weight)) {
                    _.overflow_x = m;
                    _.syncOverflowVisible();
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "overflow-y")) {
            if (m := OverflowMode.parse(val)) {
                if (_.tryClaimSlot(PROP_OVERFLOW_Y, weight)) {
                    _.overflow_y = m;
                    _.syncOverflowVisible();
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
        if (std.ascii.eqlIgnoreCase(name, "border-collapse")) {
            if (bc := BorderCollapse.parse(val)) {
                if (_.tryClaimSlot(PROP_BORDER_COLLAPSE, weight)) _.border_collapse = bc;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "border-spacing")) {
            let it = std.mem.tokenizeAny(u8, val, " \t\r\n");
            if (t1 := it.next()) {
                if (l1 := CssLength.parse(t1)) {
                    const h_px: f32 = @floatCast(l1.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                    let v_px: f32 = h_px;
                    if (t2 := it.next()) {
                        if (l2 := CssLength.parse(t2)) {
                            v_px = @floatCast(l2.toPx(@as(f64, root_font_px), 1280.0, 800.0));
                        }
                    }
                    if (_.tryClaimSlot(PROP_BORDER_SPACING, weight)) {
                        _.border_spacing_h = h_px;
                        _.border_spacing_v = v_px;
                    }
                }
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "caption-side")) {
            if (cs := CaptionSide.parse(val)) {
                if (_.tryClaimSlot(PROP_CAPTION_SIDE, weight)) _.caption_side = cs;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "table-layout")) {
            if (tl := TableLayout.parse(val)) {
                if (_.tryClaimSlot(PROP_TABLE_LAYOUT, weight)) _.table_layout = tl;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "float")) {
            if (fm := FloatMode.parse(val)) {
                if (_.tryClaimSlot(PROP_FLOAT, weight)) _.float_mode = fm;
            }
            return;
        }
        if (std.ascii.eqlIgnoreCase(name, "clear")) {
            if (cm := ClearMode.parse(val)) {
                if (_.tryClaimSlot(PROP_CLEAR, weight)) _.clear_mode = cm;
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

pub struct ContainerRuleEntry {
    name: []const u8 = "";
    condition: []const u8 = "";
    nested_body: []const u8 = "";
}

pub struct StyleSheet {
    rules: [64]StyleRuleEntry = [StyleRuleEntry{}; 64];
    rule_count: usize = 0;
    container_rules: [8]ContainerRuleEntry = [ContainerRuleEntry{}; 8];
    container_rule_count: usize = 0;
    font_faces: [16]CssFontFaceRule = [CssFontFaceRule{}; 16];
    font_face_count: usize = 0;
    page_margin: PageMarginRule = PageMarginRule{};

    static fn supportsDeclPredicate(p_name: []const u8, p_val_raw: []const u8): bool {
        let p_val = std.mem.trim(u8, p_val_raw, " \t\r\n");
        if (p_val.len >= 10 && std.ascii.endsWithIgnoreCase(p_val, "important")) {
            if (bang := std.mem.lastIndexOfScalar(u8, p_val, '!')) {
                const after_bang = std.mem.trim(u8, p_val[bang + 1 ..], " \t\r\n");
                if (std.ascii.eqlIgnoreCase(after_bang, "important")) {
                    p_val = std.mem.trim(u8, p_val[0..bang], " \t\r\n");
                }
            }
        }
        if (std.mem.startsWith(u8, p_name, "--") && p_name.len > 2) {
            return ComputedStyle.isValidCustomPropValue(p_val);
        }
        if (p_val.len == 0) return false;
        return ComputedStyle.isValidCustomPropValue(p_val);
    }

    static fn supportsSelPredicate(sel_in: []const u8): bool {
        return SelectorList.parse(sel_in).count > 0;
    }

    static fn evalSupportsCondition(cond_raw: []const u8): bool {
        let s = std.mem.trim(u8, cond_raw, " \t\r\n");
        if (s.len >= 9 && std.ascii.startsWithIgnoreCase(s, "@supports")) {
            s = std.mem.trim(u8, s[9..], " \t\r\n");
        }
        if (s.len == 0) return false;
        if (tree := SupportsTree.parse(s)) {
            return tree.eval(StyleSheet.supportsDeclPredicate, StyleSheet.supportsSelPredicate);
        }
        return false;
    }

    static fn appendNestedRules(sheet: *StyleSheet, nested_body: []const u8) {
        const nested_opt = std.heap.page_allocator.alloc(CssRule, 32) catch null;
        if (nested_opt != null) {
            let nested_rules = nested_opt.?;
            defer std.heap.page_allocator.free(nested_rules);
            let ni_init: usize = 0;
            while (ni_init < 32) : (ni_init += 1) {
                nested_rules[ni_init] = CssRule{};
            }
            const n_cnt = CssStylesheetParser.parseStylesheet(nested_body, nested_rules);
            let j: usize = 0;
            while (j < n_cnt) : (j += 1) {
                if (nested_rules[j].kind == CssRuleKind.Style && sheet.rule_count < 64) {
                    StyleSheet.appendStyleRule(sheet, &nested_rules[j]);
                } else if (nested_rules[j].kind == CssRuleKind.FontFace && sheet.font_face_count < 16) {
                    if (ff := CssFontFaceRule.parseBlock(nested_rules[j].nested_body)) {
                        sheet.font_faces[sheet.font_face_count] = ff;
                        sheet.font_face_count += 1;
                    }
                }
            }
        }
    }

    static fn parse(css_text: []const u8, viewport_w: f32, is_dark: bool): StyleSheet {
        let sheet = StyleSheet{};
        let raw_rules_opt = std.heap.page_allocator.alloc(CssRule, 64) catch null;
        if (raw_rules_opt == null) return sheet;
        let raw_rules = raw_rules_opt.?;
        defer std.heap.page_allocator.free(raw_rules);
        let ri_init: usize = 0;
        while (ri_init < 64) : (ri_init += 1) {
            raw_rules[ri_init] = CssRule{};
        }
        const parsed_count = CssStylesheetParser.parseStylesheet(css_text, raw_rules);

        let i: usize = 0;
        while (i < parsed_count) : (i += 1) {
            const r = &raw_rules[i];
            if (r.kind == CssRuleKind.Style) {
                if (sheet.rule_count < 64) {
                    StyleSheet.appendStyleRule(&sheet, r);
                }
            } else if (r.kind == CssRuleKind.Page) {
                let pi: usize = 0;
                while (pi < r.prop_count) : (pi += 1) {
                    sheet.page_margin.applyDecl(&r.properties[pi]);
                }
            } else if (r.kind == CssRuleKind.FontFace) {
                if (sheet.font_face_count < 16) {
                    if (ff := CssFontFaceRule.parseBlock(r.nested_body)) {
                        sheet.font_faces[sheet.font_face_count] = ff;
                        sheet.font_face_count += 1;
                    }
                }
            } else if (r.kind == CssRuleKind.Media) {
                let media_matched = false;
                if (mql := MediaQueryList.parse(r.prelude) catch null) {
                    const env = MediaEnv {
                        media_type: MediaType.screen,
                        width: @as(f64, viewport_w),
                        height: 720.0,
                        color_scheme: if (is_dark) .dark else .light,
                    };
                    media_matched = mql.evaluate(env);
                } else {
                    media_matched = r.media.matches(@as(f64, viewport_w), is_dark);
                }
                if (media_matched) {
                    StyleSheet.appendNestedRules(&sheet, r.nested_body);
                }
            } else if (r.kind == CssRuleKind.Supports) {
                if (StyleSheet.evalSupportsCondition(r.prelude)) {
                    StyleSheet.appendNestedRules(&sheet, r.nested_body);
                }
            } else if (r.kind == CssRuleKind.Container && sheet.container_rule_count < 8) {
                StyleSheet.appendContainerRule(&sheet, r);
            }
        }
        return sheet;
    }

    static fn fillStyleRuleEntry(entry: *StyleRuleEntry, r: *const CssRule) {
        entry.selectors = SelectorList.parse(r.prelude);
        entry.decl_count = 0;
        entry.important_count = 0;
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
    }

    static fn appendStyleRule(sheet: *StyleSheet, r: *const CssRule) {
        if (sheet.rule_count >= 64) return;
        StyleSheet.fillStyleRuleEntry(&sheet.rules[sheet.rule_count], r);
        sheet.rule_count += 1;
    }

    static fn appendContainerRule(sheet: *StyleSheet, r: *const CssRule) {
        if (sheet.container_rule_count >= 8) return;
        let prel = std.mem.trim(u8, r.prelude, " \t\r\n");
        if (prel.len >= 10 && std.ascii.startsWithIgnoreCase(prel, "@container")) {
            prel = std.mem.trim(u8, prel[10..], " \t\r\n");
        }
        let cname: []const u8 = "";
        let cond: []const u8 = prel;
        if (lp := std.mem.indexOfScalar(u8, prel, '(')) {
            const before = std.mem.trim(u8, prel[0..lp], " \t\r\n");
            if (before.len > 0 && !std.ascii.eqlIgnoreCase(before, "not")) {
                cname = before;
                cond = std.mem.trim(u8, prel[lp..], " \t\r\n");
            }
        }
        sheet.container_rules[sheet.container_rule_count] = ContainerRuleEntry {
            name: cname,
            condition: cond,
            nested_body: r.nested_body,
        };
        sheet.container_rule_count += 1;
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
        let pos: usize = 0;
        while (tok := nextCssValueToken(s, &pos)) {
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
            if (std.ascii.startsWithIgnoreCase(tok, "linear-gradient(") ||
                std.ascii.startsWithIgnoreCase(tok, "radial-gradient(") ||
                std.ascii.startsWithIgnoreCase(tok, "conic-gradient(") ||
                std.ascii.startsWithIgnoreCase(tok, "url(")) {
                continue;
            }
            if (c := CssColor.parse(tok)) {
                return c;
            }
        }
        return null;
    }

    static fn findBeforePseudo(raw_sel: []const u8): usize? {
        if (p := std.mem.indexOf(u8, raw_sel, "::before")) return p;
        let i: usize = 0;
        while (i < raw_sel.len) : (i += 1) {
            if (raw_sel[i] == ':') {
                if (i + 8 <= raw_sel.len && std.ascii.eqlIgnoreCase(raw_sel[i .. i + 8], "::before")) return i;
                if (i + 7 <= raw_sel.len && std.ascii.eqlIgnoreCase(raw_sel[i .. i + 7], ":before")) {
                    if (i + 7 == raw_sel.len || raw_sel[i + 7] == ' ' || raw_sel[i + 7] == '\t') return i;
                }
            }
        }
        return null;
    }

    static fn findAfterPseudo(raw_sel: []const u8): usize? {
        if (p := std.mem.indexOf(u8, raw_sel, "::after")) return p;
        let i: usize = 0;
        while (i < raw_sel.len) : (i += 1) {
            if (raw_sel[i] == ':') {
                if (i + 7 <= raw_sel.len && std.ascii.eqlIgnoreCase(raw_sel[i .. i + 7], "::after")) return i;
                if (i + 6 <= raw_sel.len && std.ascii.eqlIgnoreCase(raw_sel[i .. i + 6], ":after")) {
                    if (i + 6 == raw_sel.len || raw_sel[i + 6] == ' ' || raw_sel[i + 6] == '\t') return i;
                }
            }
        }
        return null;
    }

    static fn findPlaceholderPseudo(raw_sel: []const u8): usize? {
        if (p := std.mem.indexOf(u8, raw_sel, "::placeholder")) return p;
        if (p := std.mem.indexOf(u8, raw_sel, "::-webkit-input-placeholder")) return p;
        if (p := std.mem.indexOf(u8, raw_sel, "::-moz-placeholder")) return p;
        if (p := std.mem.indexOf(u8, raw_sel, ":-ms-input-placeholder")) return p;
        // Case-insensitive fallback scan
        let i: usize = 0;
        while (i < raw_sel.len) : (i += 1) {
            if (raw_sel[i] == ':') {
                if (i + 13 <= raw_sel.len && std.ascii.eqlIgnoreCase(raw_sel[i .. i + 13], "::placeholder")) return i;
                if (i + 27 <= raw_sel.len && std.ascii.eqlIgnoreCase(raw_sel[i .. i + 27], "::-webkit-input-placeholder")) return i;
                if (i + 18 <= raw_sel.len && std.ascii.eqlIgnoreCase(raw_sel[i .. i + 18], "::-moz-placeholder")) return i;
                if (i + 22 <= raw_sel.len && std.ascii.eqlIgnoreCase(raw_sel[i .. i + 22], ":-ms-input-placeholder")) return i;
            }
        }
        return null;
    }

    static fn expandCssAttrInValue(raw: []const u8, node: *const NodeData, out_buf: []u8): []const u8 {
        let out_len: usize = 0;
        let i: usize = 0;
        let changed = false;
        while (i < raw.len) {
            let is_attr_start = false;
            if (i + 5 <= raw.len && std.ascii.eqlIgnoreCase(raw[i .. i + 5], "attr(")) {
                if (i == 0) {
                    is_attr_start = true;
                } else {
                    const prev = raw[i - 1];
                    const is_ident_prev = (prev >= 'a' && prev <= 'z') || (prev >= 'A' && prev <= 'Z') || (prev >= '0' && prev <= '9') || prev == '_' || prev == '-';
                    if (!is_ident_prev) is_attr_start = true;
                }
            }
            if (is_attr_start) {
                let depth: usize = 0;
                let j: usize = i + 4;
                let close_pos: usize? = null;
                while (j < raw.len) : (j += 1) {
                    if (raw[j] == '(') {
                        depth += 1;
                    } else if (raw[j] == ')') {
                        if (depth > 0) depth -= 1;
                        if (depth == 0) {
                            close_pos = j;
                            break;
                        }
                    }
                }
                if (cp := close_pos) {
                    const expr = raw[i .. cp + 1];
                    const parsed = parseAttrExpr(expr);
                    if (parsed.is_valid) {
                        const attr_val = node.getAttribute(parsed.attr_name);
                        const repl = evaluateCssAttr(expr, attr_val);
                        const copy_len = @min(repl.len, out_buf.len - out_len);
                        if (copy_len > 0) {
                            @memcpy(out_buf[out_len .. out_len + copy_len], repl[0..copy_len]);
                            out_len += copy_len;
                        }
                        changed = true;
                        i = cp + 1;
                        continue;
                    }
                }
            }
            if (out_len < out_buf.len) {
                out_buf[out_len] = raw[i];
                out_len += 1;
            }
            i += 1;
        }
        if (!changed) return raw;
        return out_buf[0..out_len];
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
            if (node.parent != NULL_NODE && @as(usize, node.parent) < doc.nodes.items.len) {
                const p_node = &doc.nodes.items[@as(usize, node.parent)];
                if (p_node.hasTag("details") && !p_node.hasAttribute("open")) {
                    let first_sum: NodeId = NULL_NODE;
                    let ch = p_node.first_child;
                    while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
                        const ch_node = &doc.nodes.items[@as(usize, ch)];
                        if (ch_node.node_type == .Element && ch_node.hasTag("summary")) {
                            first_sum = ch;
                            break;
                        }
                        ch = ch_node.next_sibling;
                    }
                    if (nid != @as(usize, first_sum)) {
                        style.display = Display.none();
                    }
                }
            }

            let source_order: u32 = 1;
            const node_id_u32: NodeId = @intCast(nid);

            // 0a. HTML BiDi presentational hints (`dir` attribute, <bdi>, <bdo>)
            if (node.node_type == NodeType.Element) {
                const is_bdi = node.hasTag("bdi");
                const is_bdo = node.hasTag("bdo");
                if (dir_attr := node.getAttribute("dir")) {
                    const ds = std.mem.trim(u8, dir_attr, " \t\r\n");
                    if (std.ascii.eqlIgnoreCase(ds, "ltr")) {
                        style.direction = Direction.Ltr;
                        style.is_dir_auto = false;
                        style.unicode_bidi = if (is_bdo) UnicodeBidi.IsolateOverride else UnicodeBidi.Isolate;
                    } else if (std.ascii.eqlIgnoreCase(ds, "rtl")) {
                        style.direction = Direction.Rtl;
                        style.is_dir_auto = false;
                        style.unicode_bidi = if (is_bdo) UnicodeBidi.IsolateOverride else UnicodeBidi.Isolate;
                    } else if (std.ascii.eqlIgnoreCase(ds, "auto")) {
                        style.is_dir_auto = true;
                        style.direction = resolveFirstStrongDirection(doc, node_id_u32);
                        if (node.hasTag("pre") || node.hasTag("textarea")) {
                            style.unicode_bidi = UnicodeBidi.Plaintext;
                        } else {
                            style.unicode_bidi = UnicodeBidi.Isolate;
                        }
                    } else if (is_bdi) {
                        style.is_dir_auto = true;
                        style.direction = resolveFirstStrongDirection(doc, node_id_u32);
                        style.unicode_bidi = UnicodeBidi.Isolate;
                    }
                } else if (is_bdi) {
                    style.is_dir_auto = true;
                    style.direction = resolveFirstStrongDirection(doc, node_id_u32);
                    style.unicode_bidi = UnicodeBidi.Isolate;
                }
            }

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
            const is_range_input = node.hasTag("input") && std.ascii.eqlIgnoreCase(node.getAttribute("type") ?? "", "range");
            let placeholder_color_raw: []const u8 = "";
            let placeholder_color_weight: u64 = 0;
            let r_i: usize = 0;
            while (r_i < sheet.rule_count) : (r_i += 1) {
                const rule = &sheet.rules[r_i];
                let has_range_thumb_pseudo = false;
                let has_non_pseudo_sel = false;
                let matched_range_thumb = false;
                let matched_before_pseudo = false;
                let matched_after_pseudo = false;
                let matched_placeholder_pseudo = false;
                let placeholder_spec: u32 = 0;
                let si_p: usize = 0;
                while (si_p < rule.selectors.count) : (si_p += 1) {
                    const raw_sel = rule.selectors.selectors[si_p].raw;
                    let thumb_pos: usize? = std.mem.indexOf(u8, raw_sel, "::-webkit-slider-thumb");
                    if (thumb_pos == null) {
                        thumb_pos = std.mem.indexOf(u8, raw_sel, "::-moz-range-thumb");
                    }
                    if (tp := thumb_pos) {
                        has_range_thumb_pseudo = true;
                        if (is_range_input && !matched_range_thumb) {
                            const base_raw = std.mem.trim(u8, raw_sel[0..tp], " \t\r\n");
                            const base_sel_str = if (base_raw.len > 0) base_raw else "*";
                            const base_list = SelectorList.parse(base_sel_str);
                            if (Matcher.matchBestSpecificity(doc, node_id_u32, &base_list) != null) {
                                matched_range_thumb = true;
                            }
                        }
                    } else if (pp := Cascade.findPlaceholderPseudo(raw_sel)) {
                        const base_raw = std.mem.trim(u8, raw_sel[0..pp], " \t\r\n");
                        const base_sel_str = if (base_raw.len > 0) base_raw else "*";
                        const base_list = SelectorList.parse(base_sel_str);
                        if (sp := Matcher.matchBestSpecificity(doc, node_id_u32, &base_list)) {
                            if (!matched_placeholder_pseudo || sp > placeholder_spec) {
                                placeholder_spec = sp;
                            }
                            matched_placeholder_pseudo = true;
                        }
                    } else if (bp := Cascade.findBeforePseudo(raw_sel)) {
                        if (!matched_before_pseudo) {
                            const base_raw = std.mem.trim(u8, raw_sel[0..bp], " \t\r\n");
                            const base_sel_str = if (base_raw.len > 0) base_raw else "*";
                            const base_list = SelectorList.parse(base_sel_str);
                            if (Matcher.matchBestSpecificity(doc, node_id_u32, &base_list) != null) {
                                matched_before_pseudo = true;
                            }
                        }
                    } else if (ap := Cascade.findAfterPseudo(raw_sel)) {
                        if (!matched_after_pseudo) {
                            const base_raw = std.mem.trim(u8, raw_sel[0..ap], " \t\r\n");
                            const base_sel_str = if (base_raw.len > 0) base_raw else "*";
                            const base_list = SelectorList.parse(base_sel_str);
                            if (Matcher.matchBestSpecificity(doc, node_id_u32, &base_list) != null) {
                                matched_after_pseudo = true;
                            }
                        }
                    } else if (std.mem.indexOf(u8, raw_sel, "::") == null && Cascade.findBeforePseudo(raw_sel) == null && Cascade.findAfterPseudo(raw_sel) == null) {
                        has_non_pseudo_sel = true;
                    }
                }
                if (matched_placeholder_pseudo) {
                    let pd_i: usize = 0;
                    while (pd_i < rule.decl_count) : (pd_i += 1) {
                        const decl = &rule.declarations[pd_i];
                        if (std.ascii.eqlIgnoreCase(decl.name, "color")) {
                            const w = Cascade.packWeight(false, false, placeholder_spec, source_order);
                            source_order += 1;
                            if (w >= placeholder_color_weight) {
                                placeholder_color_weight = w;
                                placeholder_color_raw = decl.value;
                            }
                        }
                    }
                    let pi_i: usize = 0;
                    while (pi_i < rule.important_count) : (pi_i += 1) {
                        const decl = &rule.important_declarations[pi_i];
                        if (std.ascii.eqlIgnoreCase(decl.name, "color")) {
                            const w = Cascade.packWeight(true, false, placeholder_spec, source_order);
                            source_order += 1;
                            if (w >= placeholder_color_weight) {
                                placeholder_color_weight = w;
                                placeholder_color_raw = decl.value;
                            }
                        }
                    }
                }
                if (matched_before_pseudo || matched_after_pseudo) {
                    let pd_i: usize = 0;
                    while (pd_i < rule.decl_count) : (pd_i += 1) {
                        const decl = &rule.declarations[pd_i];
                        if (std.ascii.eqlIgnoreCase(decl.name, "content")) {
                            if (matched_before_pseudo) {
                                if (hasDynamicPseudoContent(decl.value)) {
                                    const elen = @min(decl.value.len, 128);
                                    std.mem.copyForwards(u8, style.before_content_expr_buf[0..elen], decl.value[0..elen]);
                                    style.before_content_expr_len = elen;
                                    style.has_before_content = true;
                                } else if (clen := parsePseudoContentValue(decl.value, style.before_content_buf[0..])) {
                                    style.before_content_len = clen;
                                    style.before_content_expr_len = 0;
                                    style.has_before_content = true;
                                } else {
                                    style.before_content_len = 0;
                                    style.before_content_expr_len = 0;
                                    style.has_before_content = false;
                                }
                            }
                            if (matched_after_pseudo) {
                                if (hasDynamicPseudoContent(decl.value)) {
                                    const elen = @min(decl.value.len, 128);
                                    std.mem.copyForwards(u8, style.after_content_expr_buf[0..elen], decl.value[0..elen]);
                                    style.after_content_expr_len = elen;
                                    style.has_after_content = true;
                                } else if (clen := parsePseudoContentValue(decl.value, style.after_content_buf[0..])) {
                                    style.after_content_len = clen;
                                    style.after_content_expr_len = 0;
                                    style.has_after_content = true;
                                } else {
                                    style.after_content_len = 0;
                                    style.after_content_expr_len = 0;
                                    style.has_after_content = false;
                                }
                            }
                        } else if (std.ascii.eqlIgnoreCase(decl.name, "visibility")) {
                            if (v := Visibility.parse(decl.value)) {
                                if (matched_before_pseudo) style.before_visibility = v;
                                if (matched_after_pseudo) style.after_visibility = v;
                            }
                        }
                    }
                    let pi_i: usize = 0;
                    while (pi_i < rule.important_count) : (pi_i += 1) {
                        const decl = &rule.important_declarations[pi_i];
                        if (std.ascii.eqlIgnoreCase(decl.name, "content")) {
                            if (matched_before_pseudo) {
                                if (hasDynamicPseudoContent(decl.value)) {
                                    const elen = @min(decl.value.len, 128);
                                    std.mem.copyForwards(u8, style.before_content_expr_buf[0..elen], decl.value[0..elen]);
                                    style.before_content_expr_len = elen;
                                    style.has_before_content = true;
                                } else if (clen := parsePseudoContentValue(decl.value, style.before_content_buf[0..])) {
                                    style.before_content_len = clen;
                                    style.before_content_expr_len = 0;
                                    style.has_before_content = true;
                                } else {
                                    style.before_content_len = 0;
                                    style.before_content_expr_len = 0;
                                    style.has_before_content = false;
                                }
                            }
                            if (matched_after_pseudo) {
                                if (hasDynamicPseudoContent(decl.value)) {
                                    const elen = @min(decl.value.len, 128);
                                    std.mem.copyForwards(u8, style.after_content_expr_buf[0..elen], decl.value[0..elen]);
                                    style.after_content_expr_len = elen;
                                    style.has_after_content = true;
                                } else if (clen := parsePseudoContentValue(decl.value, style.after_content_buf[0..])) {
                                    style.after_content_len = clen;
                                    style.after_content_expr_len = 0;
                                    style.has_after_content = true;
                                } else {
                                    style.after_content_len = 0;
                                    style.after_content_expr_len = 0;
                                    style.has_after_content = false;
                                }
                            }
                        } else if (std.ascii.eqlIgnoreCase(decl.name, "visibility")) {
                            if (v := Visibility.parse(decl.value)) {
                                if (matched_before_pseudo) style.before_visibility = v;
                                if (matched_after_pseudo) style.after_visibility = v;
                            }
                        }
                    }
                }
                if (has_range_thumb_pseudo && !has_non_pseudo_sel) {
                    if (matched_range_thumb) {
                        let d_i: usize = 0;
                        while (d_i < rule.decl_count) : (d_i += 1) {
                            const decl = &rule.declarations[d_i];
                            if (std.ascii.eqlIgnoreCase(decl.name, "background-color") || std.ascii.eqlIgnoreCase(decl.name, "background")) {
                                if (c := Cascade.parseColorTokenFromShorthand(decl.value)) {
                                    const w = Cascade.packWeight(false, false, 0, source_order);
                                    source_order += 1;
                                    if (style.tryClaimSlot(PROP_ACCENT_COLOR, w)) {
                                        style.has_accent_color = true;
                                        style.accent_color = c;
                                    }
                                }
                            }
                        }
                        let imp_i: usize = 0;
                        while (imp_i < rule.important_count) : (imp_i += 1) {
                            const decl = &rule.important_declarations[imp_i];
                            if (std.ascii.eqlIgnoreCase(decl.name, "background-color") || std.ascii.eqlIgnoreCase(decl.name, "background")) {
                                if (c := Cascade.parseColorTokenFromShorthand(decl.value)) {
                                    const w = Cascade.packWeight(false, false, 0, source_order);
                                    source_order += 1;
                                    if (style.tryClaimSlot(PROP_ACCENT_COLOR, w)) {
                                        style.has_accent_color = true;
                                        style.accent_color = c;
                                    }
                                }
                            }
                        }
                    }
                    continue;
                }
                if (!has_non_pseudo_sel && rule.selectors.count > 0) {
                    continue;
                }
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

            if (placeholder_color_raw.len > 0) {
                const ph_v1 = style.resolveVarValue(placeholder_color_raw);
                let attr_buf: [256]u8 = [0; 256];
                const ph_v2 = Cascade.expandCssAttrInValue(ph_v1, node, attr_buf[0..]);
                const ph_v3 = style.resolveVarValue(ph_v2);
                const ph_trimmed = std.mem.trim(u8, ph_v3, " \t\r\n");
                if (ph_trimmed.len > 0) {
                    if (ph_col := CssColor.parse(ph_trimmed)) {
                        style.placeholder_color = ph_col;
                        style.has_placeholder_color = true;
                    }
                }
            }

            if (sheet.font_face_count > 0 && style.font_family_len > 0) {
                style.webfont = WebfontStyleMetrics.resolveForFamily(
                    style.font_family_buf[0..style.font_family_len],
                    sheet.font_faces[0..sheet.font_face_count],
                );
            }

            if (style.display.isWebkitBox()) {
                if (style.box_orient == BoxOrient.Vertical && style.webkit_line_clamp > 0) {
                    style.display = Display.flowRoot();
                    style.is_line_clamped = true;
                } else {
                    style.flex_direction = if (style.box_orient == BoxOrient.Vertical) FlexDirection.Column else FlexDirection.Row;
                }
            }

            if (style.has_bg_light_dark) {
                style.background_color = if (style.color_scheme.isDarkUsed()) style.bg_dark_color else style.bg_light_color;
            }
            if (style.has_color_light_dark) {
                style.color = if (style.color_scheme.isDarkUsed()) style.color_dark else style.color_light;
            }

            out_styles[nid] = style;
        }
        Cascade.resolveCountersAndPseudo(doc, out_styles);
    }

    static fn resolveCountersRecursive(
        doc: *const Document,
        styles: []ComputedStyle,
        node_id: NodeId,
        stack: *CssCounterStack,
    ) {
        if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len || @as(usize, node_id) >= styles.len) return;
        const node = &doc.nodes.items[@as(usize, node_id)];
        const saved_count = stack.count;
        if (node.node_type == NodeType.Element) {
            let st = &styles[@as(usize, node_id)];
            if (st.has_counter_reset && st.counter_reset_len > 0) {
                stack.pushReset(st.counter_reset_name[0..st.counter_reset_len], st.counter_reset_val);
            }
            if (st.has_counter_inc && st.counter_inc_len > 0) {
                stack.increment(st.counter_inc_name[0..st.counter_inc_len], st.counter_inc_val);
            }
            if (st.before_content_expr_len > 0) {
                if (wlen := resolvePseudoContentExpr(st.before_content_expr_buf[0..st.before_content_expr_len], node, stack, st.before_content_buf[0..])) {
                    st.before_content_len = wlen;
                    st.has_before_content = true;
                }
            }
            if (st.after_content_expr_len > 0) {
                if (wlen := resolvePseudoContentExpr(st.after_content_expr_buf[0..st.after_content_expr_len], node, stack, st.after_content_buf[0..])) {
                    st.after_content_len = wlen;
                    st.has_after_content = true;
                }
            }
        }
        let ch = node.first_child;
        while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
            Cascade.resolveCountersRecursive(doc, styles, ch, stack);
            ch = doc.nodes.items[@as(usize, ch)].next_sibling;
        }
        stack.count = saved_count;
    }

    static fn resolveCountersAndPseudo(doc: *const Document, styles: []ComputedStyle) {
        let stack = CssCounterStack{};
        const root_id = if (doc.root_element != NULL_NODE) doc.root_element else 1;
        Cascade.resolveCountersRecursive(doc, styles, root_id, &stack);
    }

    static fn parseContainerLengthPx(raw: []const u8, root_font_px: f32): f32? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (s.len == 0) return null;
        if (l := CssLength.parse(s)) {
            return @floatCast(l.toPx(@as(f64, root_font_px), 1280.0, 800.0));
        }
        return std.fmt.parseFloat(f32, s) catch null;
    }

    static fn evalSingleContainerClause(clause_raw: []const u8, cw: f32, ch: f32, has_block: bool, root_font_px: f32): bool {
        let s = std.mem.trim(u8, clause_raw, " \t\r\n");
        while (s.len >= 2 && s[0] == '(' && s[s.len - 1] == ')') {
            s = std.mem.trim(u8, s[1 .. s.len - 1], " \t\r\n");
        }
        if (s.len == 0) return false;

        // Colon syntax: min-width: 500px, max-width: 800px, width: 500px, min-inline-size: ...
        if (colon := std.mem.indexOfScalar(u8, s, ':')) {
            const prop = std.mem.trim(u8, s[0..colon], " \t\r\n");
            const val_s = std.mem.trim(u8, s[colon + 1 ..], " \t\r\n");
            const rhs_opt = Cascade.parseContainerLengthPx(val_s, root_font_px);
            if (rhs_opt == null) return false;
            const rhs = rhs_opt.?;
            if (std.ascii.eqlIgnoreCase(prop, "min-width") || std.ascii.eqlIgnoreCase(prop, "min-inline-size")) {
                return cw >= rhs;
            }
            if (std.ascii.eqlIgnoreCase(prop, "max-width") || std.ascii.eqlIgnoreCase(prop, "max-inline-size")) {
                return cw <= rhs;
            }
            if (std.ascii.eqlIgnoreCase(prop, "width") || std.ascii.eqlIgnoreCase(prop, "inline-size")) {
                return @abs(cw - rhs) < 0.01;
            }
            if (std.ascii.eqlIgnoreCase(prop, "min-height") || std.ascii.eqlIgnoreCase(prop, "min-block-size")) {
                return has_block && ch >= rhs;
            }
            if (std.ascii.eqlIgnoreCase(prop, "max-height") || std.ascii.eqlIgnoreCase(prop, "max-block-size")) {
                return has_block && ch <= rhs;
            }
            if (std.ascii.eqlIgnoreCase(prop, "height") || std.ascii.eqlIgnoreCase(prop, "block-size")) {
                return has_block && @abs(ch - rhs) < 0.01;
            }
            return false;
        }

        // Range syntax: width > 730px, width < 300px, width >= 400px, width <= 400px, width = 400px
        let op_pos: usize = 0;
        let op_len: usize = 0;
        let op_kind: u8 = 0; // 1=>, 2=<, 3=>=, 4=<=, 5==
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            if (s[i] == '>') {
                op_pos = i;
                if (i + 1 < s.len && s[i + 1] == '=') { op_len = 2; op_kind = 3; } else { op_len = 1; op_kind = 1; }
                break;
            }
            if (s[i] == '<') {
                op_pos = i;
                if (i + 1 < s.len && s[i + 1] == '=') { op_len = 2; op_kind = 4; } else { op_len = 1; op_kind = 2; }
                break;
            }
            if (s[i] == '=') {
                op_pos = i;
                op_len = 1;
                op_kind = 5;
                break;
            }
        }
        if (op_kind == 0) return false;
        const lhs_s = std.mem.trim(u8, s[0..op_pos], " \t\r\n");
        const rhs_s = std.mem.trim(u8, s[op_pos + op_len ..], " \t\r\n");

        let lhs_is_prop = false;
        let prop_val: f32 = 0.0;
        if (std.ascii.eqlIgnoreCase(lhs_s, "width") || std.ascii.eqlIgnoreCase(lhs_s, "inline-size")) {
            lhs_is_prop = true;
            prop_val = cw;
        } else if (std.ascii.eqlIgnoreCase(lhs_s, "height") || std.ascii.eqlIgnoreCase(lhs_s, "block-size")) {
            if (!has_block) return false;
            lhs_is_prop = true;
            prop_val = ch;
        }

        if (lhs_is_prop) {
            const cmp_opt = Cascade.parseContainerLengthPx(rhs_s, root_font_px);
            if (cmp_opt == null) return false;
            const cmp_v = cmp_opt.?;
            return switch (op_kind) {
                1 => prop_val > cmp_v,
                2 => prop_val < cmp_v,
                3 => prop_val >= cmp_v,
                4 => prop_val <= cmp_v,
                5 => @abs(prop_val - cmp_v) < 0.01,
                * => false,
            };
        } else {
            const cmp_opt = Cascade.parseContainerLengthPx(lhs_s, root_font_px);
            if (cmp_opt == null) return false;
            const cmp_v = cmp_opt.?;
            if (std.ascii.eqlIgnoreCase(rhs_s, "width") || std.ascii.eqlIgnoreCase(rhs_s, "inline-size")) {
                prop_val = cw;
            } else if (std.ascii.eqlIgnoreCase(rhs_s, "height") || std.ascii.eqlIgnoreCase(rhs_s, "block-size")) {
                if (!has_block) return false;
                prop_val = ch;
            } else {
                return false;
            }
            return switch (op_kind) {
                1 => cmp_v > prop_val,
                2 => cmp_v < prop_val,
                3 => cmp_v >= prop_val,
                4 => cmp_v <= prop_val,
                5 => @abs(cmp_v - prop_val) < 0.01,
                * => false,
            };
        }
    }

    static fn evalContainerCondition(cond_raw: []const u8, cw: f32, ch: f32, has_block: bool, root_font_px: f32): bool {
        const s = std.mem.trim(u8, cond_raw, " \t\r\n");
        if (s.len == 0) return false;
        let pos: usize = 0;
        let seg_start: usize = 0;
        let depth: i32 = 0;
        let any_clause = false;
        while (pos < s.len) : (pos += 1) {
            if (s[pos] == '(') {
                depth += 1;
            } else if (s[pos] == ')') {
                if (depth > 0) depth -= 1;
            } else if (depth == 0 && pos + 5 <= s.len &&
                (s[pos] == ' ' || s[pos] == '\t') &&
                std.ascii.eqlIgnoreCase(s[pos + 1 .. pos + 4], "and") &&
                (s[pos + 4] == ' ' || s[pos + 4] == '\t' || s[pos + 4] == '(')) {
                const seg = std.mem.trim(u8, s[seg_start..pos], " \t\r\n");
                if (seg.len > 0) {
                    any_clause = true;
                    if (!Cascade.evalSingleContainerClause(seg, cw, ch, has_block, root_font_px)) return false;
                }
                seg_start = pos + 4;
                pos += 3;
            }
        }
        const tail = std.mem.trim(u8, s[seg_start..], " \t\r\n");
        if (tail.len > 0) {
            any_clause = true;
            if (!Cascade.evalSingleContainerClause(tail, cw, ch, has_block, root_font_px)) return false;
        }
        return any_clause;
    }

    /// Recomputes styles including `@container` queries resolved against the laid-out ancestor container boxes.
    static fn computeStylesWithContainers(
        doc: *const Document,
        sheet: *const StyleSheet,
        out_styles: []ComputedStyle,
        box_widths: []const f32,
        box_heights: []const f32,
        root_font_px: f32,
    ) {
        Cascade.computeStyles(doc, sheet, out_styles, root_font_px);
        if (sheet.container_rule_count == 0) return;

        const total = doc.nodes.items.len;
        let nid: usize = 1;
        while (nid < total && nid < out_styles.len) : (nid += 1) {
            const node = &doc.nodes.items[nid];
            if (node.node_type != NodeType.Element) continue;
            const node_id_u32: NodeId = @intCast(nid);
            let source_order: u32 = 100000;
            let matched_any = false;

            let ci: usize = 0;
            while (ci < sheet.container_rule_count) : (ci += 1) {
                const cr = &sheet.container_rules[ci];
                let anc = node.parent;
                let found_anc: NodeId = NULL_NODE;
                while (anc != NULL_NODE && @as(usize, anc) < out_styles.len) {
                    const anc_st = &out_styles[@as(usize, anc)];
                    if (anc_st.container_type != ContainerType.Normal) {
                        const anc_cname = anc_st.container_name_buf[0..anc_st.container_name_len];
                        if (cr.name.len == 0 || std.mem.eql(u8, anc_cname, cr.name)) {
                            found_anc = anc;
                            break;
                        }
                    }
                    anc = doc.nodes.items[@as(usize, anc)].parent;
                }
                if (found_anc == NULL_NODE) continue;
                const anc_idx = @as(usize, found_anc);
                const cw = if (anc_idx < box_widths.len) box_widths[anc_idx] else 0.0;
                const ch = if (anc_idx < box_heights.len) box_heights[anc_idx] else 0.0;
                const has_block = (out_styles[anc_idx].container_type == ContainerType.Size);
                if (!Cascade.evalContainerCondition(cr.condition, cw, ch, has_block, root_font_px)) continue;

                const nested_opt = std.heap.page_allocator.alloc(CssRule, 8) catch null;
                if (nested_opt == null) continue;
                let nested_rules = nested_opt.?;
                defer std.heap.page_allocator.free(nested_rules);
                let ni_init: usize = 0;
                while (ni_init < 8) : (ni_init += 1) {
                    nested_rules[ni_init] = CssRule{};
                }
                const n_cnt = CssStylesheetParser.parseStylesheet(cr.nested_body, nested_rules);
                const tmp_entry_opt = std.heap.page_allocator.alloc(StyleRuleEntry, 1) catch null;
                if (tmp_entry_opt == null) continue;
                let tmp_entry_slice = tmp_entry_opt.?;
                defer std.heap.page_allocator.free(tmp_entry_slice);

                let ri: usize = 0;
                while (ri < n_cnt) : (ri += 1) {
                    if (nested_rules[ri].kind != CssRuleKind.Style) continue;
                    tmp_entry_slice[0] = StyleRuleEntry{};
                    StyleSheet.fillStyleRuleEntry(&tmp_entry_slice[0], &nested_rules[ri]);
                    const rule = &tmp_entry_slice[0];
                    if (best_spec := Matcher.matchBestSpecificity(doc, node_id_u32, &rule.selectors)) {
                        matched_any = true;
                        let di: usize = 0;
                        while (di < rule.decl_count) : (di += 1) {
                            const w = Cascade.packWeight(false, false, best_spec, source_order);
                            source_order += 1;
                            out_styles[nid].applyProperty(&rule.declarations[di], w, root_font_px);
                        }
                        let ii: usize = 0;
                        while (ii < rule.important_count) : (ii += 1) {
                            const w = Cascade.packWeight(true, false, best_spec, source_order);
                            source_order += 1;
                            out_styles[nid].applyProperty(&rule.important_declarations[ii], w, root_font_px);
                        }
                    }
                }
            }

            if (matched_any && node.inline_style.len > 0) {
                let it = std.mem.splitScalar(u8, node.inline_style, ';');
                while (raw_decl := it.next()) {
                    if (prop := CssProperty.parse(raw_decl)) {
                        const w = Cascade.packWeight(prop.important, true, 0, source_order);
                        source_order += 1;
                        out_styles[nid].applyProperty(&prop, w, root_font_px);
                    }
                }
            }
        }
    }
}
