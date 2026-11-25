// src/layout/layout.cp
// CapSurf Box Model & Flexbox Geometry Layout Engine (Milestone 1)
// Consumes ComputedStyle from cascade.cp, computes screen pixel coordinates
// Rect { x: f32, y: f32, width: f32, height: f32, visible: bool } for every NodeId,
// powering element.getBoundingClientRect() and AI Agent vision/action automation.

import { Document, NodeData, NodeType, NodeId, NULL_NODE } from "../dom/dom.cp";
import { decodeUtf8Next, isBidiFormatControl } from "../css/bidi.cp";
import { sniffPng } from "../render/image_dims.cp";
import {
    ComputedStyle,
    ObjectViewBoxKind,
    CssUnit,
    Display,
    Visibility,
    BoxSizing,
    CssPosition,
    FlexDirection,
    FlexWrap,
    JustifyContent,
    AlignItems,
    BorderCollapse,
    CaptionSide,
    TableLayout,
    FloatMode,
    ClearMode,
    VerticalAlignKind,
    WhiteSpace,
    IntrinsicSizeKeyword,
    ListStylePosition,
    formatListMarker
} from "../css/cascade.cp";

pub struct Rect {
    x: f32 = 0.0;
    y: f32 = 0.0;
    width: f32 = 0.0;
    height: f32 = 0.0;
    visible: bool = false;

    static fn zero(): Rect {
        return Rect {
            x: 0.0,
            y: 0.0,
            width: 0.0,
            height: 0.0,
            visible: false,
        };
    }

    fn right(): f32 {
        return _.x + _.width;
    }

    fn bottom(): f32 {
        return _.y + _.height;
    }

    fn isClickable(): bool {
        return _.visible && _.width > 0.0 && _.height > 0.0;
    }
}

pub struct LayoutBox {
    node_id: NodeId = NULL_NODE;
    rect: Rect = Rect{};

    // Box model margins, borders, paddings in pixels
    margin_top: f32 = 0.0;
    margin_right: f32 = 0.0;
    margin_bottom: f32 = 0.0;
    margin_left: f32 = 0.0;

    padding_top: f32 = 0.0;
    padding_right: f32 = 0.0;
    padding_bottom: f32 = 0.0;
    padding_left: f32 = 0.0;

    border_top: f32 = 0.0;
    border_right: f32 = 0.0;
    border_bottom: f32 = 0.0;
    border_left: f32 = 0.0;

    content_width: f32 = 0.0;
    content_height: f32 = 0.0;

    // Scroll & positioning metadata for sticky/fixed viewport resolution and hit testing
    position: CssPosition = CssPosition.Static;
    overflow_visible: bool = true;
    height_is_auto: bool = true;
    z_index: i32 = 0;
    has_z_index: bool = false;
    pointer_events_none: bool = false;
    creates_stacking_context: bool = false;
    sticky_top: f32 = 0.0;
    sticky_bottom: f32 = 0.0;
    sticky_left: f32 = 0.0;
    sticky_right: f32 = 0.0;
    has_sticky_top: bool = false;
    has_sticky_bottom: bool = false;
    has_sticky_left: bool = false;
    has_sticky_right: bool = false;
    scroll_content_width: f32 = 0.0;
    scroll_content_height: f32 = 0.0;
    forced_height: f32 = -1.0;
    baseline_offset: f32 = -1.0;

    fn paddingHorizontal(): f32 {
        return _.padding_left + _.padding_right;
    }

    fn paddingVertical(): f32 {
        return _.padding_top + _.padding_bottom;
    }

    fn borderHorizontal(): f32 {
        return _.border_left + _.border_right;
    }

    fn borderVertical(): f32 {
        return _.border_top + _.border_bottom;
    }

    fn marginHorizontal(): f32 {
        return _.margin_left + _.margin_right;
    }

    fn marginVertical(): f32 {
        return _.margin_top + _.margin_bottom;
    }

    fn borderBoxWidth(): f32 {
        return _.content_width + _.paddingHorizontal() + _.borderHorizontal();
    }

    fn borderBoxHeight(): f32 {
        return _.content_height + _.paddingVertical() + _.borderVertical();
    }

    fn outerWidth(): f32 {
        return _.borderBoxWidth() + _.marginHorizontal();
    }

    fn outerHeight(): f32 {
        return _.borderBoxHeight() + _.marginVertical();
    }
}

pub struct FlexItem {
    node_id: NodeId = NULL_NODE;
    base_main: f32 = 0.0;
    base_cross: f32 = 0.0;
    target_main: f32 = 0.0;
    target_cross: f32 = 0.0;
    grow: f32 = 0.0;
    shrink: f32 = 1.0;
}

pub struct ReplacedSize {
    width: f32 = 300.0;
    height: f32 = 150.0;
    is_authored: bool = false;
    is_ratio_only: bool = false;
}

pub struct TableCellEntry {
    node_id: NodeId = NULL_NODE;
    is_anon: bool = false;
    anon_nodes: [16]NodeId = [NULL_NODE; 16];
    anon_count: usize = 0;
    row: usize = 0;
    col: usize = 0;
    rowspan: usize = 1;
    colspan: usize = 1;
    pref_w: f32 = 0.0;
    measured_h: f32 = 0.0;
    layout_x: f32 = 0.0;
    layout_y: f32 = 0.0;
}

pub struct TableRowEntry {
    node_id: NodeId = NULL_NODE;
    group_id: NodeId = NULL_NODE;
}

pub enum InlineItemKind {
    Text,
    Atomic,
    Break,
    AbsPos,
    FloatItem,
}

pub struct InlineItem {
    kind: InlineItemKind = InlineItemKind.Text;
    node_id: NodeId = NULL_NODE;
    style_node_id: NodeId = NULL_NODE;
    parent_style_id: NodeId = NULL_NODE;
    width: f32 = 0.0;
    height: f32 = 0.0;
    orig_border_x: f32 = 0.0;
    orig_border_y: f32 = 0.0;
    line_x: f32 = 0.0;
    wrapped_lines: usize = 1;
    is_replaced: bool = false;
    has_leading_space: bool = false;
    has_trailing_space: bool = false;
    is_space_only: bool = false;
    trailing_space_w: f32 = 0.0;
    text_slice: []const u8 = "";
    has_tabs: bool = false;
}

pub struct LayoutTree {
    allocator: std.mem.Allocator;
    boxes: std.ArrayListUnmanaged(LayoutBox) = std.ArrayListUnmanaged(LayoutBox).empty;
    viewport_width: f32 = 1280.0;
    viewport_height: f32 = 800.0;
    root_font_px: f32 = 16.0;
    abs_cb_x: f32 = 0.0;
    abs_cb_y: f32 = 0.0;
    abs_cb_w: f32 = 1280.0;
    abs_cb_h: f32 = 800.0;

    static fn parseSvgTranslate(attr: []const u8): [2]f32 {
        let tx: f32 = 0.0;
        let ty: f32 = 0.0;
        let pos: usize = 0;
        while (pos < attr.len) {
            const rem = attr[pos..];
            const idx_opt = std.mem.indexOf(u8, rem, "translate");
            if (idx_opt == null) break;
            let p = pos + idx_opt.? + 9;
            while (p < attr.len && (attr[p] == ' ' || attr[p] == '\t' || attr[p] == '\r' || attr[p] == '\n')) : (p += 1) {}
            if (p >= attr.len || attr[p] != '(') {
                pos = p;
                continue;
            }
            p += 1;
            const close_opt = std.mem.indexOfScalar(u8, attr[p..], ')');
            if (close_opt == null) break;
            const args = attr[p .. p + close_opt.?];
            let it = std.mem.tokenizeAny(u8, args, " ,\t\r\n");
            if (s0 := it.next()) {
                if (v0 := std.fmt.parseFloat(f32, s0) catch null) {
                    tx += v0;
                }
            }
            if (s1 := it.next()) {
                if (v1 := std.fmt.parseFloat(f32, s1) catch null) {
                    ty += v1;
                }
            }
            pos = p + close_opt.? + 1;
        }
        const res: [2]f32 = [tx, ty];
        return res;
    }

    static fn parseDimStr(s: []const u8): f32? {
        let trimmed = std.mem.trim(u8, s, " \t\r\n");
        if (trimmed.len > 2 && std.ascii.eqlIgnoreCase(trimmed[trimmed.len - 2 ..], "px")) {
            trimmed = trimmed[0 .. trimmed.len - 2];
        }
        if (v := std.fmt.parseFloat(f32, trimmed) catch null) {
            if (v >= 0.0) return v;
        }
        return null;
    }

    static fn parsePathBounds(d: []const u8): ([4]f32)? {
        if (d.len == 0) return null;
        let cur_x: f32 = 0.0;
        let cur_y: f32 = 0.0;
        let min_x: f32 = 0.0;
        let min_y: f32 = 0.0;
        let max_x: f32 = 0.0;
        let max_y: f32 = 0.0;
        let has_points = false;

        let i: usize = 0;
        let cur_cmd: u8 = 'M';

        while (i < d.len) {
            while (i < d.len && (d[i] == ' ' || d[i] == '\t' || d[i] == '\r' || d[i] == '\n' || d[i] == ',')) : (i += 1) {}
            if (i >= d.len) break;

            const ch = d[i];
            if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
                cur_cmd = ch;
                i += 1;
                while (i < d.len && (d[i] == ' ' || d[i] == '\t' || d[i] == '\r' || d[i] == '\n' || d[i] == ',')) : (i += 1) {}
                if (i >= d.len) break;
            }

            if (cur_cmd == 'z' || cur_cmd == 'Z') {
                continue;
            }

            if (cur_cmd == 'h' || cur_cmd == 'H') {
                const num_start = i;
                if (i < d.len && (d[i] == '-' || d[i] == '+')) i += 1;
                while (i < d.len && ((d[i] >= '0' && d[i] <= '9') || d[i] == '.')) : (i += 1) {}
                if (i > num_start) {
                    if (val := std.fmt.parseFloat(f32, d[num_start..i]) catch null) {
                        cur_x = if (cur_cmd == 'h') (cur_x + val) else val;
                        if (!has_points) {
                            min_x = cur_x; max_x = cur_x; min_y = cur_y; max_y = cur_y; has_points = true;
                        } else {
                            min_x = @min(min_x, cur_x); max_x = @max(max_x, cur_x);
                        }
                    }
                }
            } else if (cur_cmd == 'v' || cur_cmd == 'V') {
                const num_start = i;
                if (i < d.len && (d[i] == '-' || d[i] == '+')) i += 1;
                while (i < d.len && ((d[i] >= '0' && d[i] <= '9') || d[i] == '.')) : (i += 1) {}
                if (i > num_start) {
                    if (val := std.fmt.parseFloat(f32, d[num_start..i]) catch null) {
                        cur_y = if (cur_cmd == 'v') (cur_y + val) else val;
                        if (!has_points) {
                            min_x = cur_x; max_x = cur_x; min_y = cur_y; max_y = cur_y; has_points = true;
                        } else {
                            min_y = @min(min_y, cur_y); max_y = @max(max_y, cur_y);
                        }
                    }
                }
            } else {
                let x_val: f32 = 0.0;
                let y_val: f32 = 0.0;
                let got_x = false;
                let got_y = false;

                const n1_start = i;
                if (i < d.len && (d[i] == '-' || d[i] == '+')) i += 1;
                while (i < d.len && ((d[i] >= '0' && d[i] <= '9') || d[i] == '.')) : (i += 1) {}
                if (i > n1_start) {
                    if (v1 := std.fmt.parseFloat(f32, d[n1_start..i]) catch null) {
                        x_val = v1; got_x = true;
                    }
                }
                while (i < d.len && (d[i] == ' ' || d[i] == '\t' || d[i] == '\r' || d[i] == '\n' || d[i] == ',')) : (i += 1) {}
                const n2_start = i;
                if (i < d.len && (d[i] == '-' || d[i] == '+')) i += 1;
                while (i < d.len && ((d[i] >= '0' && d[i] <= '9') || d[i] == '.')) : (i += 1) {}
                if (i > n2_start) {
                    if (v2 := std.fmt.parseFloat(f32, d[n2_start..i]) catch null) {
                        y_val = v2; got_y = true;
                    }
                }
                if (got_x && got_y) {
                    if (cur_cmd >= 'a' && cur_cmd <= 'z') {
                        cur_x += x_val;
                        cur_y += y_val;
                    } else {
                        cur_x = x_val;
                        cur_y = y_val;
                    }
                    if (!has_points) {
                        min_x = cur_x; max_x = cur_x; min_y = cur_y; max_y = cur_y; has_points = true;
                    } else {
                        min_x = @min(min_x, cur_x); max_x = @max(max_x, cur_x);
                        min_y = @min(min_y, cur_y); max_y = @max(max_y, cur_y);
                    }
                }
            }
        }

        if (!has_points) return null;
        const res: [4]f32 = [min_x, min_y, max_x - min_x, max_y - min_y];
        return res;
    }

    static fn extractSvgRootTag(src: []const u8): ([]const u8)? {
        let i: usize = 0;
        let start: usize = 0;
        let found_start = false;
        while (i < src.len) : (i += 1) {
            if (i + 4 <= src.len && std.ascii.eqlIgnoreCase(src[i .. i + 4], "<svg")) {
                start = i;
                found_start = true;
                break;
            }
            if (i + 6 <= src.len && std.ascii.eqlIgnoreCase(src[i .. i + 6], "%3Csvg")) {
                start = i;
                found_start = true;
                break;
            }
        }
        if (!found_start) return null;
        let j = start + 4;
        while (j < src.len) : (j += 1) {
            if (src[j] == '>') {
                return src[start .. j + 1];
            }
            if (j + 3 <= src.len && std.ascii.eqlIgnoreCase(src[j .. j + 3], "%3E")) {
                return src[start .. j + 3];
            }
        }
        return src[start..];
    }

    static fn extractSvgViewBox(tag_slice: []const u8): ([2]f32)? {
        let cur: usize = 0;
        while (cur + 7 <= tag_slice.len) : (cur += 1) {
            if (!std.ascii.eqlIgnoreCase(tag_slice[cur .. cur + 7], "viewbox")) continue;
            let pos = cur + 7;
            while (pos < tag_slice.len && (tag_slice[pos] == ' ' || tag_slice[pos] == '\t' || tag_slice[pos] == '\r' || tag_slice[pos] == '\n')) : (pos += 1) {}
            if (pos < tag_slice.len && tag_slice[pos] == '=') {
                pos += 1;
            } else if (pos + 3 <= tag_slice.len && std.ascii.eqlIgnoreCase(tag_slice[pos .. pos + 3], "%3D")) {
                pos += 3;
            } else {
                continue;
            }
            if (pos < tag_slice.len && (tag_slice[pos] == '"' || tag_slice[pos] == '\'')) {
                pos += 1;
            } else if (pos + 3 <= tag_slice.len && (std.ascii.eqlIgnoreCase(tag_slice[pos .. pos + 3], "%22") || std.ascii.eqlIgnoreCase(tag_slice[pos .. pos + 3], "%27"))) {
                pos += 3;
            }
            let nums: [4]f32 = [0.0, 0.0, 0.0, 0.0];
            let count: usize = 0;
            while (pos < tag_slice.len && count < 4) {
                while (pos < tag_slice.len) {
                    const ch = tag_slice[pos];
                    if (ch == ' ' || ch == '\t' || ch == ',' || ch == '+' || ch == '\r' || ch == '\n') {
                        pos += 1;
                    } else if (pos + 3 <= tag_slice.len && (std.ascii.eqlIgnoreCase(tag_slice[pos .. pos + 3], "%20") || std.ascii.eqlIgnoreCase(tag_slice[pos .. pos + 3], "%2C"))) {
                        pos += 3;
                    } else {
                        break;
                    }
                }
                const nstart = pos;
                if (pos < tag_slice.len && (tag_slice[pos] == '-' || tag_slice[pos] == '+')) pos += 1;
                while (pos < tag_slice.len && (std.ascii.isDigit(tag_slice[pos]) || tag_slice[pos] == '.')) : (pos += 1) {}
                if (pos == nstart) break;
                if (v := std.fmt.parseFloat(f32, tag_slice[nstart..pos]) catch null) {
                    nums[count] = v;
                    count += 1;
                } else {
                    break;
                }
            }
            if (count == 4 && nums[2] > 0.0 && nums[3] > 0.0) {
                const res: [2]f32 = [nums[2], nums[3]];
                return res;
            }
        }
        return null;
    }

    static fn extractSvgDim(src: []const u8, name: []const u8): f32? {
        let cur_start: usize = 0;
        while (cur_start < src.len) {
            const found_opt = std.mem.indexOf(u8, src[cur_start..], name);
            if (found_opt == null) break;
            const at = cur_start + found_opt.?;
            let pos = at + name.len;
            while (pos < src.len && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\r' || src[pos] == '\n')) : (pos += 1) {}
            let has_eq = false;
            if (pos < src.len && src[pos] == '=') {
                pos += 1;
                has_eq = true;
            } else if (pos + 3 <= src.len && std.ascii.eqlIgnoreCase(src[pos .. pos + 3], "%3D")) {
                pos += 3;
                has_eq = true;
            }
            if (has_eq) {
                if (pos < src.len && (src[pos] == '"' || src[pos] == '\'')) {
                    pos += 1;
                } else if (pos + 3 <= src.len && (std.ascii.eqlIgnoreCase(src[pos .. pos + 3], "%22") || std.ascii.eqlIgnoreCase(src[pos .. pos + 3], "%27"))) {
                    pos += 3;
                }
                const num_start = pos;
                while (pos < src.len && (std.ascii.isDigit(src[pos]) || src[pos] == '.')) : (pos += 1) {}
                if (pos > num_start) {
                    if (v := std.fmt.parseFloat(f32, src[num_start..pos]) catch null) {
                        if (v > 0.0) return v;
                    }
                }
            }
            cur_start = at + name.len;
        }
        return null;
    }

    static fn extractPngDim(src: []const u8, which: []const u8): f32? {
        const prefix = "data:image/png;base64,";
        if (src.len <= prefix.len || !std.ascii.eqlIgnoreCase(src[0..prefix.len], prefix)) return null;
        const b64 = src[prefix.len..];
        let buf: [64]u8 = undefined;
        const take_len = @min(b64.len, 48);
        const decoded_len = std.base64.standard.Decoder.calcSizeForSlice(b64[0..take_len]) catch return null;
        if (decoded_len < 24) return null;
        std.base64.standard.Decoder.decode(buf[0..decoded_len], b64[0..take_len]) catch return null;
        if (dims := sniffPng(buf[0..decoded_len])) {
            if (std.mem.eql(u8, which, "width")) return @as(f32, @floatFromInt(dims.width));
            if (std.mem.eql(u8, which, "height")) return @as(f32, @floatFromInt(dims.height));
        }
        return null;
    }

    static fn getReplacedSizeWithStyle(node: *const NodeData, style: *const ComputedStyle): ReplacedSize? {
        let rep = LayoutTree.getReplacedSize(node) ?? return null;
        if (style.object_view_box.kind != ObjectViewBoxKind.None) {
            const vb = style.object_view_box.computeRect(rep.width, rep.height);
            rep.width = vb.width;
            rep.height = vb.height;
        }
        return rep;
    }

    static fn getReplacedSize(node: *const NodeData): ReplacedSize? {
        if (node.hasTag("canvas") || node.hasTag("video") || node.hasTag("iframe")) {
            let nat_w: f32 = 300.0;
            let nat_h: f32 = 150.0;
            if (w_attr := node.getAttribute("width")) {
                if (w_val := LayoutTree.parseDimStr(w_attr)) {
                    nat_w = w_val;
                }
            }
            if (h_attr := node.getAttribute("height")) {
                if (h_val := LayoutTree.parseDimStr(h_attr)) {
                    nat_h = h_val;
                }
            }
            return ReplacedSize{
                width: nat_w,
                height: nat_h,
                is_authored: true,
            };
        } else if (node.hasTag("img")) {
            let nat_w: f32 = 300.0;
            let nat_h: f32 = 150.0;
            let is_auth = false;
            let has_w = false;
            let has_h = false;
            if (w_attr := node.getAttribute("width")) {
                if (w_val := LayoutTree.parseDimStr(w_attr)) {
                    nat_w = w_val;
                    has_w = true;
                    is_auth = true;
                }
            }
            if (h_attr := node.getAttribute("height")) {
                if (h_val := LayoutTree.parseDimStr(h_attr)) {
                    nat_h = h_val;
                    has_h = true;
                }
            }
            let is_ratio_only = false;
            if (!has_w || !has_h) {
                if (src_attr := node.getAttribute("src")) {
                    const is_svg_data = std.mem.indexOf(u8, src_attr, "image/svg+xml") != null;
                    if (is_svg_data) {
                        if (root_tag := LayoutTree.extractSvgRootTag(src_attr)) {
                            let svg_has_w = has_w;
                            let svg_has_h = has_h;
                            if (!svg_has_w) {
                                if (sw := LayoutTree.extractSvgDim(root_tag, "width")) {
                                    nat_w = sw;
                                    svg_has_w = true;
                                }
                            }
                            if (!svg_has_h) {
                                if (sh := LayoutTree.extractSvgDim(root_tag, "height")) {
                                    nat_h = sh;
                                    svg_has_h = true;
                                }
                            }
                            if (!svg_has_w || !svg_has_h) {
                                if (vb := LayoutTree.extractSvgViewBox(root_tag)) {
                                    if (!svg_has_w && !svg_has_h) {
                                        nat_w = vb[0];
                                        nat_h = vb[1];
                                        is_ratio_only = true;
                                    } else if (svg_has_w && !svg_has_h) {
                                        nat_h = nat_w * (vb[1] / vb[0]);
                                    } else if (!svg_has_w && svg_has_h) {
                                        nat_w = nat_h * (vb[0] / vb[1]);
                                    }
                                }
                            }
                        }
                    } else {
                        if (!has_w) {
                            if (pw := LayoutTree.extractPngDim(src_attr, "width")) {
                                nat_w = pw;
                            } else if (sw := LayoutTree.extractSvgDim(src_attr, "width")) {
                                nat_w = sw;
                            }
                        }
                        if (!has_h) {
                            if (ph := LayoutTree.extractPngDim(src_attr, "height")) {
                                nat_h = ph;
                            } else if (sh := LayoutTree.extractSvgDim(src_attr, "height")) {
                                nat_h = sh;
                            }
                        }
                    }
                }
            }
            return ReplacedSize{
                width: nat_w,
                height: nat_h,
                is_authored: is_auth,
                is_ratio_only: is_ratio_only,
            };
        } else if (node.hasTag("svg")) {
            let nat_w: f32 = 300.0;
            let nat_h: f32 = 150.0;
            let is_auth = false;
            let is_ratio_only = false;
            let has_w = false;
            let has_h = false;
            if (w_attr := node.getAttribute("width")) {
                if (w_val := LayoutTree.parseDimStr(w_attr)) {
                    if (w_val > 0.0) {
                        nat_w = w_val;
                        has_w = true;
                        is_auth = true;
                    }
                }
            }
            if (h_attr := node.getAttribute("height")) {
                if (h_val := LayoutTree.parseDimStr(h_attr)) {
                    if (h_val > 0.0) {
                        nat_h = h_val;
                        has_h = true;
                        is_auth = true;
                    }
                }
            }
            if (!has_w || !has_h) {
                if (vb_attr := node.getAttribute("viewBox")) {
                    let it = std.mem.tokenizeAny(u8, vb_attr, " ,");
                    * = it.next();
                    * = it.next();
                    let vb_w: f32 = 0.0;
                    let vb_h: f32 = 0.0;
                    if (w_str := it.next()) {
                        if (w_val := LayoutTree.parseDimStr(w_str)) {
                            if (w_val > 0.0) vb_w = w_val;
                        }
                    }
                    if (h_str := it.next()) {
                        if (h_val := LayoutTree.parseDimStr(h_str)) {
                            if (h_val > 0.0) vb_h = h_val;
                        }
                    }
                    if (vb_w > 0.0 && vb_h > 0.0) {
                        if (!has_w && !has_h) {
                            nat_w = vb_w;
                            nat_h = vb_h;
                            is_ratio_only = true;
                        } else if (has_w && !has_h) {
                            nat_h = nat_w * (vb_h / vb_w);
                        } else if (!has_w && has_h) {
                            nat_w = nat_h * (vb_w / vb_h);
                        }
                    }
                }
            }
            return ReplacedSize{
                width: nat_w,
                height: nat_h,
                is_authored: is_auth,
                is_ratio_only: is_ratio_only,
            };
        } else if (node.hasTag("input")) {
            if (t := node.getAttribute("type")) {
                if (std.ascii.eqlIgnoreCase(std.mem.trim(u8, t, " \t\r\n"), "hidden")) {
                    return null;
                }
            }
            let nat_w: f32 = 150.0;
            let nat_h: f32 = 18.0;
            let is_auth = false;
            if (w_attr := node.getAttribute("width")) {
                if (w_val := LayoutTree.parseDimStr(w_attr)) {
                    nat_w = w_val;
                    is_auth = true;
                }
            }
            if (h_attr := node.getAttribute("height")) {
                if (h_val := LayoutTree.parseDimStr(h_attr)) {
                    nat_h = h_val;
                }
            }
            return ReplacedSize{
                width: nat_w,
                height: nat_h,
                is_authored: is_auth,
            };
        } else if (node.hasTag("textarea")) {
            let cols: f32 = 20.0;
            let rows: f32 = 2.0;
            if (c_attr := node.getAttribute("cols")) {
                if (c_val := LayoutTree.parseDimStr(c_attr)) {
                    if (c_val > 0.0) cols = c_val;
                }
            }
            if (r_attr := node.getAttribute("rows")) {
                if (r_val := LayoutTree.parseDimStr(r_attr)) {
                    if (r_val > 0.0) rows = r_val;
                }
            }
            return ReplacedSize{
                width: cols * 8.0 + 16.0,
                height: rows * 16.0 + 6.0,
                is_authored: false,
            };
        }
        return null;
    }

    static fn getNodeTextLength(doc: *const Document, styles: []const ComputedStyle, node_id: NodeId): usize {
        if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return 0;
        const n = &doc.nodes.items[@as(usize, node_id)];
        if (n.node_type == NodeType.Text) {
            return std.mem.trim(u8, n.text_content, " \t\r\n").len;
        }
        let total: usize = 0;
        if (n.node_type == NodeType.Element && @as(usize, node_id) < styles.len) {
            const est = &styles[@as(usize, node_id)];
            if (est.has_before_content) total += est.before_content_len;
            if (est.has_after_content) total += est.after_content_len;
        }
        let ch = n.first_child;
        while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
            const cn = &doc.nodes.items[@as(usize, ch)];
            if (cn.node_type == NodeType.Element && @as(usize, ch) < styles.len) {
                const cst = &styles[@as(usize, ch)];
                if (cst.display.isNone() || cst.position == CssPosition.Absolute || cst.position == CssPosition.Fixed) {
                    ch = cn.next_sibling;
                    continue;
                }
            }
            total += LayoutTree.getNodeTextLength(doc, styles, ch);
            ch = cn.next_sibling;
        }
        return total;
    }

    static fn hasLatinText(doc: *const Document, node_id: NodeId): bool {
        if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return false;
        const n = &doc.nodes.items[@as(usize, node_id)];
        if (n.node_type == NodeType.Text) {
            let i: usize = 0;
            while (i < n.text_content.len) : (i += 1) {
                const c = n.text_content[i];
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return true;
            }
            return false;
        }
        let ch = n.first_child;
        while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
            if (LayoutTree.hasLatinText(doc, ch)) return true;
            ch = doc.nodes.items[@as(usize, ch)].next_sibling;
        }
        return false;
    }

    static fn init(allocator: std.mem.Allocator, vw: f32, vh: f32): LayoutTree! {
        let tree = LayoutTree {
            allocator: allocator,
            boxes: std.ArrayListUnmanaged(LayoutBox).empty,
            viewport_width: vw,
            viewport_height: vh,
            root_font_px: 16.0,
        };
        return tree;
    }

    fn deinit(*_) {
        _.boxes.deinit(_.allocator);
    }

    fn getBoundingClientRect(node_id: NodeId): Rect {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.boxes.items.len) {
            return Rect.zero();
        }
        return _.boxes.items[@as(usize, node_id)].rect;
    }

    fn isClickable(node_id: NodeId): bool {
        const r = _.getBoundingClientRect(node_id);
        return r.isClickable();
    }

    /// Resolves box model metrics (margin, padding, border, content size) for `node_id`.
    fn resolveBoxMetrics(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        node_id: NodeId,
        style: *const ComputedStyle,
        parent_w: f32,
        parent_h: f32,
        parent_h_definite: bool,
    ) {
        let box = &_.boxes.items[@as(usize, node_id)];
        const font_px = style.font_size_px;
        const root_px = _.root_font_px;
        const vw = _.viewport_width;
        const vh = _.viewport_height;

        box.position = style.position;
        box.overflow_visible = style.overflow_visible;
        box.z_index = style.z_index;
        box.has_z_index = style.has_z_index;
        box.pointer_events_none = style.pointer_events_none;
        const has_tx = !style.transform_tx.is_auto && !style.transform_tx.is_none;
        const has_ty = !style.transform_ty.is_auto && !style.transform_ty.is_none;
        box.creates_stacking_context = style.isolation_isolate ||
            (style.position != CssPosition.Static && style.has_z_index) ||
            (style.opacity < 1.0) ||
            has_tx || has_ty;
        box.has_sticky_top = (style.position == CssPosition.Sticky) && !style.top.is_auto && !style.top.is_none;
        box.has_sticky_bottom = (style.position == CssPosition.Sticky) && !style.bottom.is_auto && !style.bottom.is_none;
        box.has_sticky_left = (style.position == CssPosition.Sticky) && !style.left.is_auto && !style.left.is_none;
        box.has_sticky_right = (style.position == CssPosition.Sticky) && !style.right.is_auto && !style.right.is_none;
        if (box.has_sticky_top) {
            box.sticky_top = style.top.resolvePx(parent_h, font_px, root_px, vw, vh);
        }
        if (box.has_sticky_bottom) {
            box.sticky_bottom = style.bottom.resolvePx(parent_h, font_px, root_px, vw, vh);
        }
        if (box.has_sticky_left) {
            box.sticky_left = style.left.resolvePx(parent_w, font_px, root_px, vw, vh);
        }
        if (box.has_sticky_right) {
            box.sticky_right = style.right.resolvePx(parent_w, font_px, root_px, vw, vh);
        }

        box.margin_top = style.margin_top.resolvePx(parent_w, font_px, root_px, vw, vh);
        box.margin_right = style.margin_right.resolvePx(parent_w, font_px, root_px, vw, vh);
        box.margin_bottom = style.margin_bottom.resolvePx(parent_w, font_px, root_px, vw, vh);
        box.margin_left = style.margin_left.resolvePx(parent_w, font_px, root_px, vw, vh);

        box.padding_top = style.padding_top.resolvePx(parent_w, font_px, root_px, vw, vh);
        box.padding_right = style.padding_right.resolvePx(parent_w, font_px, root_px, vw, vh);
        box.padding_bottom = style.padding_bottom.resolvePx(parent_w, font_px, root_px, vw, vh);
        box.padding_left = style.padding_left.resolvePx(parent_w, font_px, root_px, vw, vh);

        box.border_top = style.border_top_width;
        box.border_right = style.border_right_width;
        box.border_bottom = style.border_bottom_width;
        box.border_left = style.border_left_width;

        let replaced_opt: ReplacedSize? = null;
        if (node_id != NULL_NODE && @as(usize, node_id) < doc.nodes.items.len) {
            const node = &doc.nodes.items[@as(usize, node_id)];
            if (node.isElement()) {
                replaced_opt = LayoutTree.getReplacedSizeWithStyle(node, style);
            }
        }

        // Content width calculation
        const is_abs_or_fixed = (style.position == CssPosition.Absolute || style.position == CssPosition.Fixed);
        const h_decor = box.paddingHorizontal() + box.borderHorizontal();
        const mh = box.marginHorizontal();
        let raw_w: f32 = 0.0;
        let raw_is_border_box = false;

        if (style.width.isIntrinsic()) {
            const avail = if (parent_w > mh) (parent_w - mh) else 0.0;
            let used_ratio_w = false;
            if (rep := replaced_opt) {
                if (rep.height > 0.0 && !style.height.is_auto && !style.height.is_none && !style.height.isIntrinsic() &&
                    (style.width.intrinsic == IntrinsicSizeKeyword.MinContent ||
                     style.width.intrinsic == IntrinsicSizeKeyword.MaxContent ||
                     style.width.intrinsic == IntrinsicSizeKeyword.FitContent)) {
                    const h_is_pct = !style.height.is_calc && style.height.length.unit == CssUnit.Percent;
                    if (!h_is_pct || parent_h > 0.0) {
                        const h_px = style.height.resolvePx(parent_h, font_px, root_px, vw, vh);
                        const v_decor = box.paddingVertical() + box.borderVertical();
                        const ch = if (style.box_sizing == BoxSizing.BorderBox) (if (h_px > v_decor) (h_px - v_decor) else 0.0) else h_px;
                        raw_w = ch * (rep.width / rep.height);
                        raw_is_border_box = false;
                        used_ratio_w = true;
                    }
                }
            }
            if (!used_ratio_w) {
                if (style.width.intrinsic == IntrinsicSizeKeyword.MinContent) {
                    const mc = _.measureMinContentWidth(doc, styles, node_id, parent_w);
                    raw_w = if (mc > mh) (mc - mh) else 0.0;
                } else if (style.width.intrinsic == IntrinsicSizeKeyword.MaxContent) {
                    const mc = _.measureIntrinsicWidth(doc, styles, node_id, parent_w);
                    raw_w = if (mc > mh) (mc - mh) else 0.0;
                } else if (style.width.intrinsic == IntrinsicSizeKeyword.FitContent) {
                    const min_o = _.measureMinContentWidth(doc, styles, node_id, parent_w);
                    const max_o = _.measureIntrinsicWidth(doc, styles, node_id, parent_w);
                    const min_c = if (min_o > mh) (min_o - mh) else 0.0;
                    const max_c = if (max_o > mh) (max_o - mh) else 0.0;
                    raw_w = @min(@max(min_c, avail), max_c);
                } else {
                    // Stretch / -webkit-fill-available
                    raw_w = avail;
                }
                raw_is_border_box = true;
            }
        } else if (!style.width.is_auto && !style.width.is_none) {
            if (!style.width.is_calc && style.width.length.unit == CssUnit.Ch && LayoutTree.isMonospaceFont(style)) {
                const ch_v: f32 = @floatCast(style.width.length.value);
                raw_w = ch_v * font_px * 0.60;
            } else {
                raw_w = style.width.resolvePx(parent_w, font_px, root_px, vw, vh);
            }
            raw_is_border_box = (style.box_sizing == BoxSizing.BorderBox);
        } else if (is_abs_or_fixed && !style.left.is_auto && !style.left.is_none && !style.right.is_auto && !style.right.is_none) {
            const l_px = style.left.resolvePx(parent_w, font_px, root_px, vw, vh);
            const r_px = style.right.resolvePx(parent_w, font_px, root_px, vw, vh);
            const avail = parent_w - l_px - r_px - mh;
            raw_w = if (avail > 0.0) avail else 0.0;
            raw_is_border_box = true;
        } else if (rep := replaced_opt) {
            if (rep.is_ratio_only) {
                const avail = parent_w - mh;
                if (avail > 0.0) {
                    raw_w = avail;
                } else {
                    raw_w = rep.width;
                }
            } else if (rep.is_authored) {
                raw_w = rep.width;
            } else {
                const avail = parent_w - mh;
                if (avail > 0.0 && rep.width > avail) {
                    raw_w = avail;
                } else {
                    raw_w = rep.width;
                }
            }
            raw_is_border_box = (style.box_sizing == BoxSizing.BorderBox);
        } else if (style.display.isInlineFlex() || style.display.isInlineBlock() || is_abs_or_fixed) {
            // Shrink-to-fit auto width (CSS 2.1 §10.3.7 / §10.3.9 / CSS Flexbox §9)
            const avail = if (parent_w > mh) (parent_w - mh) else 0.0;
            const min_o = _.measureMinContentWidth(doc, styles, node_id, parent_w);
            const max_o = _.measureIntrinsicWidth(doc, styles, node_id, parent_w);
            const min_c = if (min_o > mh) (min_o - mh) else 0.0;
            const max_c = if (max_o > mh) (max_o - mh) else 0.0;
            raw_w = @min(@max(min_c, avail), max_c);
            raw_is_border_box = true;
        } else {
            if (style.display.isInline() || style.float_mode != FloatMode.None) {
                raw_w = 0.0;
            } else {
                // Default block width fills available parent content area minus horizontal margins
                const avail = parent_w - mh;
                raw_w = if (avail > 0.0) avail else 0.0;
            }
            raw_is_border_box = true;
        }

        // Adjust for box-sizing or border-box resolved width
        if (raw_is_border_box) {
            box.content_width = if (raw_w > h_decor) (raw_w - h_decor) else 0.0;
        } else {
            box.content_width = raw_w;
        }

        // Min/max width constraints
        if (!style.min_width.is_auto && !style.min_width.is_none && !style.min_width.isIntrinsic()) {
            const min_w_raw = style.min_width.resolvePx(parent_w, font_px, root_px, vw, vh);
            const min_w = if (style.box_sizing == BoxSizing.BorderBox) (if (min_w_raw > h_decor) (min_w_raw - h_decor) else 0.0) else min_w_raw;
            if (box.content_width < min_w) {
                box.content_width = min_w;
            }
        }
        if (!style.max_width.is_auto && !style.max_width.is_none && !style.max_width.isIntrinsic()) {
            const max_w_raw = style.max_width.resolvePx(parent_w, font_px, root_px, vw, vh);
            const max_w = if (style.box_sizing == BoxSizing.BorderBox) (if (max_w_raw > h_decor) (max_w_raw - h_decor) else 0.0) else max_w_raw;
            if (box.content_width > max_w) {
                box.content_width = max_w;
            }
        }

        // Content height calculation
        // CSS Sizing: percentage height against an indefinite (auto) parent height behaves as auto
        const height_is_indefinite_pct = (!style.height.is_auto && !style.height.is_none && style.height.length.unit == CssUnit.Percent && !parent_h_definite);
        const has_forced_h = (box.forced_height >= 0.0);
        const effective_height_auto = !has_forced_h && (style.height.is_auto || style.height.is_none || height_is_indefinite_pct);
        box.height_is_auto = effective_height_auto;
        if (has_forced_h) {
            const v_decor = box.paddingVertical() + box.borderVertical();
            box.content_height = if (box.forced_height > v_decor) (box.forced_height - v_decor) else 0.0;
        } else if (!effective_height_auto) {
            const raw_h = style.height.resolvePx(parent_h, font_px, root_px, vw, vh);
            if (style.box_sizing == BoxSizing.BorderBox) {
                const v_decor = box.paddingVertical() + box.borderVertical();
                box.content_height = if (raw_h > v_decor) (raw_h - v_decor) else 0.0;
            } else {
                box.content_height = raw_h;
            }
            if ((style.width.is_auto || style.width.is_none)) {
                if (rep := replaced_opt) {
                    if (rep.width > 0.0 && rep.height > 0.0 && box.content_height > 0.0) {
                        box.content_width = box.content_height * (rep.width / rep.height);
                    }
                }
            }
        } else if (is_abs_or_fixed && !style.top.is_auto && !style.top.is_none && !style.bottom.is_auto && !style.bottom.is_none) {
            const t_px = style.top.resolvePx(parent_h, font_px, root_px, vw, vh);
            const b_px = style.bottom.resolvePx(parent_h, font_px, root_px, vw, vh);
            const raw_h = parent_h - t_px - b_px - box.marginVertical();
            const v_decor = box.paddingVertical() + box.borderVertical();
            box.content_height = if (raw_h > v_decor) (raw_h - v_decor) else 0.0;
        } else if (rep := replaced_opt) {
            if (rep.width > 0.0 && rep.height > 0.0) {
                const ratio = rep.height / rep.width;
                box.content_height = box.content_width * ratio;
            } else {
                box.content_height = rep.height;
            }
        } else {
            box.content_height = 0.0; // Will be auto-expanded by children
        }

        // Min/max height constraints
        if (!style.min_height.is_auto && !style.min_height.is_none) {
            const min_h = style.min_height.resolvePx(parent_h, font_px, root_px, vw, vh);
            if (box.content_height < min_h) {
                box.content_height = min_h;
            }
        }
        if (!style.max_height.is_auto && !style.max_height.is_none) {
            const max_h = style.max_height.resolvePx(parent_h, font_px, root_px, vw, vh);
            if (box.content_height > max_h) {
                box.content_height = max_h;
            }
        }
    }

    /// Recursively computes geometry for `node_id`.
    fn layoutNode(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        node_id: NodeId,
        origin_x: f32,
        origin_y: f32,
        avail_w: f32,
        avail_h: f32,
    ) {
        _.layoutNodeEx(doc, styles, node_id, origin_x, origin_y, avail_w, avail_h, origin_x, origin_y, true);
    }

    fn layoutNodeEx(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        node_id: NodeId,
        origin_x: f32,
        origin_y: f32,
        avail_w: f32,
        avail_h: f32,
        static_x: f32,
        static_y: f32,
        parent_h_definite: bool,
    ) {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.boxes.items.len) {
            return;
        }
        const style = &styles[@as(usize, node_id)];

        // display: none -> Zero geometry, invisible, blocks whole subtree
        if (style.display.isNone()) {
            _.boxes.items[@as(usize, node_id)].rect = Rect.zero();
            return;
        }

        _.resolveBoxMetrics(doc, styles, node_id, style, avail_w, avail_h, parent_h_definite);
        const box = &_.boxes.items[@as(usize, node_id)];

        // Compute screen coordinates for this element's border box.
        // For out-of-flow absolute/fixed boxes, apply known left/top or fall back to static position
        // BEFORE laying out children so descendant static positions follow positioned ancestors.
        let border_box_x = origin_x + box.margin_left;
        let border_box_y = origin_y + box.margin_top;
        if (style.position == CssPosition.Absolute || style.position == CssPosition.Fixed) {
            if (!style.left.is_auto && !style.left.is_none) {
                border_box_x = origin_x + style.left.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height) + box.margin_left;
            } else if (!style.right.is_auto && !style.right.is_none) {
                const r_px = style.right.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                border_box_x = origin_x + avail_w - r_px - box.borderBoxWidth() - box.margin_right;
            } else {
                border_box_x = static_x + box.margin_left;
            }

            if (!style.top.is_auto && !style.top.is_none) {
                border_box_y = origin_y + style.top.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height) + box.margin_top;
            } else if (!style.bottom.is_auto && !style.bottom.is_none) {
                const b_px = style.bottom.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                border_box_y = origin_y + avail_h - b_px - box.borderBoxHeight() - box.margin_bottom;
            } else {
                border_box_y = static_y + box.margin_top;
            }
        }

        // Content box origin for children
        const content_x = border_box_x + box.border_left + box.padding_left;
        const content_y = border_box_y + box.border_top + box.padding_top;

        let is_replaced = false;
        let is_svg_elem = false;
        if (node_id != NULL_NODE && @as(usize, node_id) < doc.nodes.items.len) {
            const node = &doc.nodes.items[@as(usize, node_id)];
            if (node.isElement()) {
                if (LayoutTree.getReplacedSize(node) != null) {
                    is_replaced = true;
                }
                if (node.hasTag("svg")) {
                    is_svg_elem = true;
                }
            }
        }

        // Track positioned containing block for descendant position:absolute elements
        const prev_abs_cb_x = _.abs_cb_x;
        const prev_abs_cb_y = _.abs_cb_y;
        const prev_abs_cb_w = _.abs_cb_w;
        const prev_abs_cb_h = _.abs_cb_h;
        if (style.position != CssPosition.Static) {
            let cb_x = border_box_x + box.border_left;
            let cb_y = border_box_y + box.border_top;
            if (style.position == CssPosition.Relative) {
                if (!style.left.is_auto && !style.left.is_none) {
                    cb_x += style.left.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                } else if (!style.right.is_auto && !style.right.is_none) {
                    cb_x -= style.right.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                }
                if (!style.top.is_auto && !style.top.is_none) {
                    if (style.top.length.unit != CssUnit.Percent || parent_h_definite) {
                        cb_y += style.top.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                    }
                } else if (!style.bottom.is_auto && !style.bottom.is_none) {
                    if (style.bottom.length.unit != CssUnit.Percent || parent_h_definite) {
                        cb_y -= style.bottom.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                    }
                }
            } else if (style.position == CssPosition.Absolute || style.position == CssPosition.Fixed) {
                if (!style.left.is_auto && !style.left.is_none) {
                    cb_x = origin_x + style.left.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height) + box.margin_left + box.border_left;
                } else if (!style.right.is_auto && !style.right.is_none) {
                    const r_px = style.right.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                    cb_x = origin_x + avail_w - r_px - box.borderBoxWidth() - box.margin_right + box.border_left;
                }
                if (!style.top.is_auto && !style.top.is_none) {
                    cb_y = origin_y + style.top.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height) + box.margin_top + box.border_top;
                } else if (!style.bottom.is_auto && !style.bottom.is_none) {
                    const b_px = style.bottom.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                    cb_y = origin_y + avail_h - b_px - box.borderBoxHeight() - box.margin_bottom + box.border_top;
                }
            }
            _.abs_cb_x = cb_x;
            _.abs_cb_y = cb_y;
            _.abs_cb_w = box.content_width + box.paddingHorizontal();
            _.abs_cb_h = box.content_height + box.paddingVertical();
        }
        defer {
            _.abs_cb_x = prev_abs_cb_x;
            _.abs_cb_y = prev_abs_cb_y;
            _.abs_cb_w = prev_abs_cb_w;
            _.abs_cb_h = prev_abs_cb_h;
        }

        if (!is_replaced && !is_svg_elem && style.display.isTable()) {
            _.layoutTableContainer(doc, styles, node_id, border_box_x, border_box_y, avail_w);
        } else if (style.display.isFlex() || style.display.isWebkitBox()) {
            _.layoutFlexContainer(doc, styles, node_id, content_x, content_y);
        } else if (style.display.isGrid()) {
            _.layoutGridContainer(doc, styles, node_id, content_x, content_y);
        } else if (!is_replaced && !is_svg_elem) {
            _.layoutBlockContainer(doc, styles, node_id, content_x, content_y);
        }

        let box_after = &_.boxes.items[@as(usize, node_id)];
        if (!is_replaced && !is_svg_elem && !style.display.isTable()) {
            const text_len = LayoutTree.getNodeTextLength(doc, styles, node_id);
            if (text_len > 0) {
                if (box_after.height_is_auto && box_after.content_height <= 0.0) {
                    const font_h = style.resolvedLineHeight();
                    let line_count: usize = 1;
                    if (!style.display.isInline() && !style.display.isInlineBlock() && box_after.content_width > 0.0) {
                        const base_sz = if (style.font_size_px > 0.0) style.font_size_px else 16.0;
                        line_count = LayoutTree.estimateWrappedLines(doc, node_id, box_after.content_width, base_sz);
                    }
                    if (style.is_line_clamped && style.webkit_line_clamp > 0 && line_count > @as(usize, style.webkit_line_clamp)) {
                        line_count = @as(usize, style.webkit_line_clamp);
                    }
                    box_after.content_height = @as(f32, @floatFromInt(line_count)) * font_h;
                }
                if (style.width.is_auto && box_after.content_width <= 0.0) {
                    const intrinsic_w = _.measureIntrinsicWidth(doc, styles, node_id, avail_w);
                    const h_extra = box_after.marginHorizontal() + box_after.paddingHorizontal() + box_after.borderHorizontal();
                    if (intrinsic_w > h_extra) {
                        box_after.content_width = intrinsic_w - h_extra;
                    } else {
                        const base_size = if (style.font_size_px > 0.0) style.font_size_px else 16.0;
                        let char_ratio: f32 = 0.50;
                        const wf_r = style.webfont.advanceRatioForCodepoint(@as(u32, 'M'));
                        if (wf_r > 0.0) {
                            char_ratio = wf_r;
                        } else if (style.hasFontFamily("Bungee") || style.hasFontFamily("bungee")) {
                            char_ratio = 0.85;
                        } else if (style.hasFontFamily("RangeFont") && LayoutTree.hasLatinText(doc, node_id)) {
                            char_ratio = 0.85;
                        }
                        if (style.font_weight != 400.0) {
                            char_ratio += (style.font_weight - 400.0) * 0.00015;
                        }
                        const char_w: f32 = base_size * char_ratio;
                        box_after.content_width = @as(f32, @floatFromInt(text_len)) * char_w;
                    }
                }
            }
        }

        // Re-enforce min/max constraints after child & text layout
        if (!style.min_height.is_auto && !style.min_height.is_none) {
            const min_h = style.min_height.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            if (box_after.content_height < min_h) {
                box_after.content_height = min_h;
            }
        }
        if (!style.max_height.is_auto && !style.max_height.is_none) {
            const max_h = style.max_height.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            if (box_after.content_height > max_h) {
                box_after.content_height = max_h;
            }
        }
        if (!style.min_width.is_auto && !style.min_width.is_none) {
            const min_w = style.min_width.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            if (box_after.content_width < min_w) {
                box_after.content_width = min_w;
            }
        }
        if (!style.max_width.is_auto && !style.max_width.is_none) {
            const max_w = style.max_width.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            if (box_after.content_width > max_w) {
                box_after.content_width = max_w;
            }
        }

        // Re-read box after child layout (as content_height may have been updated)
        const final_box = &_.boxes.items[@as(usize, node_id)];
        const final_w = final_box.borderBoxWidth();
        let final_h = final_box.borderBoxHeight();

        if (!style.min_height.is_auto && !style.min_height.is_none) {
            const min_h_px = style.min_height.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            if (final_h < min_h_px) {
                final_h = min_h_px;
            }
        }

        // Apply positioning offsets (if any)
        let pos_offset_x: f32 = 0.0;
        let pos_offset_y: f32 = 0.0;
        let rect_x = border_box_x;
        let rect_y = border_box_y;

        if (style.position == CssPosition.Relative) {
            if (!style.left.is_auto && !style.left.is_none) {
                pos_offset_x = style.left.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            } else if (!style.right.is_auto && !style.right.is_none) {
                const r_px = style.right.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                pos_offset_x = 0.0 - r_px;
            }
            if (!style.top.is_auto && !style.top.is_none) {
                if (style.top.length.unit != CssUnit.Percent || parent_h_definite) {
                    pos_offset_y = style.top.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                }
            } else if (!style.bottom.is_auto && !style.bottom.is_none) {
                if (style.bottom.length.unit != CssUnit.Percent || parent_h_definite) {
                    const b_px = style.bottom.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                    pos_offset_y = 0.0 - b_px;
                }
            }
            rect_x += pos_offset_x;
            rect_y += pos_offset_y;
        } else if (style.position == CssPosition.Absolute || style.position == CssPosition.Fixed) {
            if (!style.left.is_auto && !style.left.is_none) {
                rect_x = origin_x + style.left.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height) + final_box.margin_left;
            } else if (!style.right.is_auto && !style.right.is_none) {
                const r_px = style.right.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                rect_x = origin_x + avail_w - r_px - final_w - final_box.margin_right;
            } else {
                rect_x = static_x + final_box.margin_left;
            }
            if (!style.top.is_auto && !style.top.is_none) {
                rect_y = origin_y + style.top.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height) + final_box.margin_top;
            } else if (!style.bottom.is_auto && !style.bottom.is_none) {
                const b_px = style.bottom.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                rect_y = origin_y + avail_h - b_px - final_h - final_box.margin_bottom;
            } else {
                rect_y = static_y + final_box.margin_top;
            }
        }

        // Apply transform translation offsets
        let tx_px: f32 = 0.0;
        let ty_px: f32 = 0.0;
        if (!style.transform_tx.is_auto && !style.transform_tx.is_none) {
            tx_px = style.transform_tx.resolvePx(final_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            rect_x += tx_px;
        }
        if (!style.transform_ty.is_auto && !style.transform_ty.is_none) {
            ty_px = style.transform_ty.resolvePx(final_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            rect_y += ty_px;
        }

        const is_visible = (style.visibility == Visibility.Visible);
        _.boxes.items[@as(usize, node_id)].rect = Rect {
            x: rect_x,
            y: rect_y,
            width: final_w,
            height: final_h,
            visible: is_visible,
        };

        if (tx_px != 0.0 || ty_px != 0.0) {
            if (@as(usize, node_id) < doc.nodes.items.len) {
                let ch = doc.nodes.items[@as(usize, node_id)].first_child;
                while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
                    _.shiftSubtree(doc, ch, tx_px, ty_px);
                    ch = doc.nodes.items[@as(usize, ch)].next_sibling;
                }
            }
        }

        if (node_id < doc.nodes.items.len && doc.nodes.items[@as(usize, node_id)].hasTag("svg")) {
            const final_content_x = rect_x + final_box.border_left + final_box.padding_left;
            const final_content_y = rect_y + final_box.border_top + final_box.padding_top;
            _.layoutSvgContainer(doc, styles, node_id, final_content_x, final_content_y);
        }
    }

    fn layoutSvgContainer(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        node_id: NodeId,
        content_x: f32,
        content_y: f32,
    ) {
        const svg_box = &_.boxes.items[@as(usize, node_id)];
        const svg_w = svg_box.content_width;
        const svg_h = svg_box.content_height;
        const svg_node = &doc.nodes.items[@as(usize, node_id)];

        let min_x: f32 = 0.0;
        let min_y: f32 = 0.0;
        let vb_w: f32 = svg_w;
        let vb_h: f32 = svg_h;
        let has_vb = false;

        if (vb_attr := svg_node.getAttribute("viewBox")) {
            let it = std.mem.tokenizeAny(u8, vb_attr, " ,");
            if (s0 := it.next()) {
                if (v0 := LayoutTree.parseDimStr(s0)) min_x = v0;
            }
            if (s1 := it.next()) {
                if (v1 := LayoutTree.parseDimStr(s1)) min_y = v1;
            }
            if (s2 := it.next()) {
                if (v2 := LayoutTree.parseDimStr(s2)) {
                    if (v2 > 0.0) {
                        vb_w = v2;
                        has_vb = true;
                    }
                }
            }
            if (s3 := it.next()) {
                if (v3 := LayoutTree.parseDimStr(s3)) {
                    if (v3 > 0.0) {
                        vb_h = v3;
                    }
                }
            }
        }

        let scale: f32 = 1.0;
        let offset_x: f32 = 0.0;
        let offset_y: f32 = 0.0;

        if (has_vb && vb_w > 0.0 && vb_h > 0.0) {
            scale = @min(svg_w / vb_w, svg_h / vb_h);
            const drawn_w = vb_w * scale;
            const drawn_h = vb_h * scale;
            offset_x = (svg_w - drawn_w) * 0.5;
            offset_y = (svg_h - drawn_h) * 0.5;
        }

        const clip_to_svg = !styles[@as(usize, node_id)].overflow_visible;
        _.layoutSvgChildren(doc, styles, node_id, content_x, content_y, offset_x, offset_y, min_x, min_y, scale, svg_w, svg_h, clip_to_svg, 0.0, 0.0);
    }

    fn layoutSvgChildren(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        parent_id: NodeId,
        content_x: f32,
        content_y: f32,
        offset_x: f32,
        offset_y: f32,
        min_x: f32,
        min_y: f32,
        scale: f32,
        svg_w: f32,
        svg_h: f32,
        clip_to_svg: bool,
        tx: f32,
        ty: f32,
    ) {
        let child = doc.nodes.items[@as(usize, parent_id)].first_child;
        while (child != NULL_NODE && @as(usize, child) < doc.nodes.items.len) : (child = doc.nodes.items[@as(usize, child)].next_sibling) {
            const cnode = &doc.nodes.items[@as(usize, child)];
            if (!cnode.isElement()) continue;

            let cur_tx = tx;
            let cur_ty = ty;
            if (tr_attr := cnode.getAttribute("transform")) {
                const dt = LayoutTree.parseSvgTranslate(tr_attr);
                cur_tx += dt[0];
                cur_ty += dt[1];
            }

            if (cnode.hasTag("rect")) {
                let rx: f32 = 0.0;
                let ry: f32 = 0.0;
                let rw: f32 = 0.0;
                let rh: f32 = 0.0;
                if (x_attr := cnode.getAttribute("x")) {
                    if (v := LayoutTree.parseDimStr(x_attr)) rx = v;
                }
                if (y_attr := cnode.getAttribute("y")) {
                    if (v := LayoutTree.parseDimStr(y_attr)) ry = v;
                }
                if (w_attr := cnode.getAttribute("width")) {
                    if (v := LayoutTree.parseDimStr(w_attr)) rw = v;
                }
                if (h_attr := cnode.getAttribute("height")) {
                    if (v := LayoutTree.parseDimStr(h_attr)) rh = v;
                }
                const screen_x = content_x + offset_x + (rx + cur_tx - min_x) * scale;
                const screen_y = content_y + offset_y + (ry + cur_ty - min_y) * scale;
                let screen_w = rw * scale;
                let screen_h = rh * scale;

                if (clip_to_svg) {
                    if (screen_x < content_x + svg_w && screen_y < content_y + svg_h) {
                        if (screen_x + screen_w > content_x + svg_w) {
                            screen_w = (content_x + svg_w) - screen_x;
                        }
                        if (screen_y + screen_h > content_y + svg_h) {
                            screen_h = (content_y + svg_h) - screen_y;
                        }
                    }
                }

                _.boxes.items[@as(usize, child)].rect = Rect {
                    x: screen_x,
                    y: screen_y,
                    width: screen_w,
                    height: screen_h,
                    visible: true,
                };
            } else if (cnode.hasTag("circle")) {
                let cx: f32 = 0.0;
                let cy: f32 = 0.0;
                let cr: f32 = 0.0;
                if (cx_attr := cnode.getAttribute("cx")) {
                    if (v := LayoutTree.parseDimStr(cx_attr)) cx = v;
                }
                if (cy_attr := cnode.getAttribute("cy")) {
                    if (v := LayoutTree.parseDimStr(cy_attr)) cy = v;
                }
                if (r_attr := cnode.getAttribute("r")) {
                    if (v := LayoutTree.parseDimStr(r_attr)) cr = v;
                }
                const screen_x = content_x + offset_x + (cx - cr + cur_tx - min_x) * scale;
                const screen_y = content_y + offset_y + (cy - cr + cur_ty - min_y) * scale;
                const screen_w = (cr * 2.0) * scale;
                const screen_h = (cr * 2.0) * scale;
                _.boxes.items[@as(usize, child)].rect = Rect {
                    x: screen_x,
                    y: screen_y,
                    width: screen_w,
                    height: screen_h,
                    visible: true,
                };
            } else if (cnode.hasTag("ellipse")) {
                let cx: f32 = 0.0;
                let cy: f32 = 0.0;
                let erx: f32 = 0.0;
                let ery: f32 = 0.0;
                if (cx_attr := cnode.getAttribute("cx")) {
                    if (v := LayoutTree.parseDimStr(cx_attr)) cx = v;
                }
                if (cy_attr := cnode.getAttribute("cy")) {
                    if (v := LayoutTree.parseDimStr(cy_attr)) cy = v;
                }
                if (rx_attr := cnode.getAttribute("rx")) {
                    if (v := LayoutTree.parseDimStr(rx_attr)) erx = v;
                }
                if (ry_attr := cnode.getAttribute("ry")) {
                    if (v := LayoutTree.parseDimStr(ry_attr)) ery = v;
                }
                const screen_x = content_x + offset_x + (cx - erx + cur_tx - min_x) * scale;
                const screen_y = content_y + offset_y + (cy - ery + cur_ty - min_y) * scale;
                const screen_w = (erx * 2.0) * scale;
                const screen_h = (ery * 2.0) * scale;
                _.boxes.items[@as(usize, child)].rect = Rect {
                    x: screen_x,
                    y: screen_y,
                    width: screen_w,
                    height: screen_h,
                    visible: true,
                };
            } else if (cnode.hasTag("use")) {
                let ux: f32 = 0.0;
                let uy: f32 = 0.0;
                let uw: f32 = if (scale > 0.0) (svg_w / scale) else svg_w;
                let uh: f32 = if (scale > 0.0) (svg_h / scale) else svg_h;
                if (x_attr := cnode.getAttribute("x")) {
                    if (v := LayoutTree.parseDimStr(x_attr)) ux = v;
                }
                if (y_attr := cnode.getAttribute("y")) {
                    if (v := LayoutTree.parseDimStr(y_attr)) uy = v;
                }
                if (w_attr := cnode.getAttribute("width")) {
                    if (v := LayoutTree.parseDimStr(w_attr)) uw = v;
                }
                if (h_attr := cnode.getAttribute("height")) {
                    if (v := LayoutTree.parseDimStr(h_attr)) uh = v;
                }
                const screen_x = content_x + offset_x + (ux + cur_tx - min_x) * scale;
                const screen_y = content_y + offset_y + (uy + cur_ty - min_y) * scale;
                let screen_w = uw * scale;
                let screen_h = uh * scale;
                if (clip_to_svg) {
                    if (screen_x < content_x + svg_w && screen_y < content_y + svg_h) {
                        if (screen_x + screen_w > content_x + svg_w) {
                            screen_w = (content_x + svg_w) - screen_x;
                        }
                        if (screen_y + screen_h > content_y + svg_h) {
                            screen_h = (content_y + svg_h) - screen_y;
                        }
                    }
                }
                _.boxes.items[@as(usize, child)].rect = Rect {
                    x: screen_x,
                    y: screen_y,
                    width: screen_w,
                    height: screen_h,
                    visible: true,
                };
            } else if (cnode.hasTag("path") || cnode.hasTag("line") || cnode.hasTag("polygon") || cnode.hasTag("polyline") || cnode.hasTag("text")) {
                _.boxes.items[@as(usize, child)].rect = Rect {
                    x: content_x + offset_x + cur_tx * scale,
                    y: content_y + offset_y + cur_ty * scale,
                    width: svg_w,
                    height: svg_h,
                    visible: true,
                };
            } else if (cnode.hasTag("g")) {
                _.layoutSvgChildren(doc, styles, child, content_x, content_y, offset_x, offset_y, min_x, min_y, scale, svg_w, svg_h, clip_to_svg, cur_tx, cur_ty);
                let has_bounds = false;
                let g_min_x: f32 = 0.0;
                let g_min_y: f32 = 0.0;
                let g_max_x: f32 = 0.0;
                let g_max_y: f32 = 0.0;
                let gc = cnode.first_child;
                while (gc != NULL_NODE && @as(usize, gc) < doc.nodes.items.len) : (gc = doc.nodes.items[@as(usize, gc)].next_sibling) {
                    const gr = _.boxes.items[@as(usize, gc)].rect;
                    if (gr.visible && (gr.width > 0.0 || gr.height > 0.0)) {
                        if (!has_bounds) {
                            g_min_x = gr.x;
                            g_min_y = gr.y;
                            g_max_x = gr.x + gr.width;
                            g_max_y = gr.y + gr.height;
                            has_bounds = true;
                        } else {
                            g_min_x = @min(g_min_x, gr.x);
                            g_min_y = @min(g_min_y, gr.y);
                            g_max_x = @max(g_max_x, gr.x + gr.width);
                            g_max_y = @max(g_max_y, gr.y + gr.height);
                        }
                    }
                }
                if (has_bounds) {
                    _.boxes.items[@as(usize, child)].rect = Rect {
                        x: g_min_x,
                        y: g_min_y,
                        width: g_max_x - g_min_x,
                        height: g_max_y - g_min_y,
                        visible: true,
                    };
                }
            }
        }
    }

    static fn isMonospaceFont(st: *const ComputedStyle): bool {
        return st.hasFontFamily("monospace");
    }

    static fn weightWidthScale(st: *const ComputedStyle): f32 {
        if (st.font_weight == 400.0) return 1.0;
        return 1.0 + (st.font_weight - 400.0) * 0.00015;
    }

    static fn spaceWidth(st: *const ComputedStyle): f32 {
        const base_sz = if (st.font_size_px > 0.0) st.font_size_px else 16.0;
        const wf_ratio = st.webfont.advanceRatioForCodepoint(@as(u32, ' '));
        if (wf_ratio > 0.0) return base_sz * wf_ratio * LayoutTree.weightWidthScale(st);
        if (LayoutTree.isMonospaceFont(st)) return base_sz * 0.60;
        return base_sz * 0.28;
    }

    static fn charWidthForStyle(ch: u8, st: *const ComputedStyle): f32 {
        const base_sz = if (st.font_size_px > 0.0) st.font_size_px else 16.0;
        const wf_ratio = st.webfont.advanceRatioForCodepoint(@as(u32, ch));
        if (wf_ratio > 0.0) return base_sz * wf_ratio * LayoutTree.weightWidthScale(st);
        if (LayoutTree.isMonospaceFont(st)) return base_sz * 0.60;
        const w = LayoutTree.estimateCharWidth(ch, base_sz);
        if (st.font_weight >= 600.0) return w * 1.06;
        return w;
    }

    static fn advanceTabStop(cur_x: f32, space_w: f32): f32 {
        const tab_stop = space_w * 8.0;
        if (tab_stop <= 0.0001) return space_w;
        const stops_passed = @floor(cur_x / tab_stop);
        let next_stop = (stops_passed + 1.0) * tab_stop;
        if (next_stop - cur_x < space_w * 0.5) {
            next_stop += tab_stop;
        }
        return next_stop - cur_x;
    }

    static fn measureTextSegmentWithTabs(seg: []const u8, st: *const ComputedStyle, start_x: f32): f32 {
        const sp_w = LayoutTree.spaceWidth(st);
        let cur_x = start_x;
        let ti: usize = 0;
        while (ti < seg.len) {
            const b0 = seg[ti];
            if (b0 >= 0x80) {
                const cp = decodeUtf8Next(seg, &ti);
                if (isBidiFormatControl(cp)) continue;
                cur_x += LayoutTree.charWidthForStyle('M', st);
                continue;
            }
            ti += 1;
            if (b0 == '\r') continue;
            if (b0 == '\t') {
                cur_x += LayoutTree.advanceTabStop(cur_x, sp_w);
            } else if (b0 == ' ') {
                cur_x += sp_w;
            } else {
                cur_x += LayoutTree.charWidthForStyle(b0, st);
            }
        }
        return cur_x - start_x;
    }

    static fn estimateCharWidth(ch: u8, font_size_px: f32): f32 {
        if (ch == ' ' || ch == '\t') return font_size_px * 0.28;
        if (ch == 'i' || ch == 'l' || ch == 'I' || ch == 'j' || ch == 't' || ch == 'f' || ch == 'r' || ch == '.' || ch == ',' || ch == ':' || ch == ';' || ch == '!') {
            return font_size_px * 0.30;
        }
        if (ch == 'm' || ch == 'w' || ch == 'M' || ch == 'W') {
            return font_size_px * 0.78;
        }
        if (ch >= 'A' && ch <= 'Z') {
            return font_size_px * 0.62;
        }
        return font_size_px * 0.51;
    }

    static fn estimateWrappedLines(doc: *const Document, node_id: NodeId, max_w: f32, font_size_px: f32): usize {
        if (max_w <= 0.0 || font_size_px <= 0.0) return 1;
        let lines: usize = 1;
        let line_w: f32 = 0.0;
        let word_w: f32 = 0.0;
        let in_ws = true;
        let has_any = false;
        const space_w = font_size_px * 0.28;

        // Collect direct text children (and inline text descendants)
        let stack: [64]NodeId = [NULL_NODE; 64];
        let sp: usize = 0;
        stack[0] = node_id;
        sp = 1;
        // Traverse in document order using recursive helper via direct child walk
        let child = doc.nodes.items[@as(usize, node_id)].first_child;
        while (child != NULL_NODE && @as(usize, child) < doc.nodes.items.len) {
            const cn = &doc.nodes.items[@as(usize, child)];
            if (cn.node_type == NodeType.Text) {
                let ti: usize = 0;
                while (ti < cn.text_content.len) : (ti += 1) {
                    const ch = cn.text_content[ti];
                    const is_ws = (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
                    if (is_ws) {
                        if (!in_ws) {
                            // flush word
                            if (line_w > 0.0 && line_w + space_w + word_w > max_w) {
                                lines += 1;
                                line_w = word_w;
                            } else if (line_w > 0.0) {
                                line_w += space_w + word_w;
                            } else {
                                line_w = word_w;
                            }
                            word_w = 0.0;
                            in_ws = true;
                        }
                    } else {
                        has_any = true;
                        in_ws = false;
                        word_w += LayoutTree.estimateCharWidth(ch, font_size_px);
                    }
                }
            }
            child = cn.next_sibling;
        }
        if (!in_ws && word_w > 0.0) {
            if (line_w > 0.0 && line_w + space_w + word_w > max_w) {
                lines += 1;
            }
        }
        if (!has_any) return 1;
        return lines;
    }

    static fn parseSpanAttr(node: *const NodeData, attr_name: []const u8): usize {
        if (val_str := node.getAttribute(attr_name)) {
            const trimmed = std.mem.trim(u8, val_str, " \t\r\n");
            if (v := std.fmt.parseInt(usize, trimmed, 10) catch null) {
                if (v >= 1 && v <= 32) return v;
            }
        }
        return 1;
    }

    fn shiftSubtree(*_, doc: *const Document, node_id: NodeId, dx: f32, dy: f32) {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.boxes.items.len) return;
        if (dx == 0.0 && dy == 0.0) return;
        _.boxes.items[@as(usize, node_id)].rect.x += dx;
        _.boxes.items[@as(usize, node_id)].rect.y += dy;
        let ch = doc.nodes.items[@as(usize, node_id)].first_child;
        while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
            _.shiftSubtree(doc, ch, dx, dy);
            ch = doc.nodes.items[@as(usize, ch)].next_sibling;
        }
    }

    fn measureInlineMaxRun(
        doc: *const Document,
        styles: []const ComputedStyle,
        node_id: NodeId,
        style_parent_id: NodeId,
        avail_w: f32,
        cur_w: *f32,
        best_w: *f32,
        prev_ws: *bool,
        trailing_ws_w: *f32,
    ) {
        if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return;
        const node = &doc.nodes.items[@as(usize, node_id)];
        if (node.node_type == NodeType.Text) {
            const st = &styles[@as(usize, style_parent_id)];
            const space_w = LayoutTree.spaceWidth(st);
            const preserve_sp = st.white_space.preservesSpaces();
            const preserve_nl = st.white_space.preservesNewlines();
            let ti: usize = 0;
            while (ti < node.text_content.len) : (ti += 1) {
                const ch = node.text_content[ti];
                if (ch == '\n' && preserve_nl) {
                    const line_final = if (!preserve_sp && cur_w.* >= trailing_ws_w.*) (cur_w.* - trailing_ws_w.*) else cur_w.*;
                    if (line_final > best_w.*) best_w.* = line_final;
                    cur_w.* = 0.0;
                    prev_ws.* = true;
                    trailing_ws_w.* = 0.0;
                } else if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                    if (preserve_sp) {
                        if (ch == '\t') {
                            cur_w.* += LayoutTree.advanceTabStop(cur_w.*, space_w);
                            trailing_ws_w.* = 0.0;
                            prev_ws.* = false;
                        } else if (ch != '\r') {
                            cur_w.* += space_w;
                            trailing_ws_w.* = 0.0;
                            prev_ws.* = false;
                        }
                    } else {
                        if (!prev_ws.*) {
                            cur_w.* += space_w;
                            trailing_ws_w.* = space_w;
                            prev_ws.* = true;
                        }
                    }
                } else {
                    cur_w.* += LayoutTree.charWidthForStyle(ch, st);
                    trailing_ws_w.* = 0.0;
                    prev_ws.* = false;
                }
            }
            return;
        }
        if (node.node_type != NodeType.Element) return;
        const st = &styles[@as(usize, node_id)];
        if (st.display.isNone() || st.position == CssPosition.Absolute || st.position == CssPosition.Fixed) return;
        if (node.hasTag("br")) {
            const line_final = if (cur_w.* >= trailing_ws_w.*) (cur_w.* - trailing_ws_w.*) else cur_w.*;
            if (line_final > best_w.*) best_w.* = line_final;
            cur_w.* = 0.0;
            prev_ws.* = true;
            trailing_ws_w.* = 0.0;
            return;
        }
        if (LayoutTree.isAtomicInline(node, st)) {
            cur_w.* += _.measureIntrinsicWidth(doc, styles, node_id, avail_w);
            trailing_ws_w.* = 0.0;
            prev_ws.* = false;
            return;
        }
        // Inline non-replaced wrapper: add its horizontal padding/border/margin and recurse
        const fs = st.font_size_px;
        const ml = st.margin_left.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
        const mr = st.margin_right.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
        const pl = st.padding_left.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
        const pr = st.padding_right.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
        const h_extra = ml + mr + pl + pr + st.border_left_width + st.border_right_width;
        if (h_extra > 0.0) {
            cur_w.* += h_extra;
            trailing_ws_w.* = 0.0;
        }
        if (st.has_before_content && st.before_content_len > 0) {
            cur_w.* += LayoutTree.measureTextSegmentWithTabs(st.before_content_buf[0..st.before_content_len], st, cur_w.*);
            trailing_ws_w.* = 0.0;
            prev_ws.* = false;
        }
        let ch = node.first_child;
        while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
            _.measureInlineMaxRun(doc, styles, ch, node_id, avail_w, cur_w, best_w, prev_ws, trailing_ws_w);
            ch = doc.nodes.items[@as(usize, ch)].next_sibling;
        }
        if (st.has_after_content && st.after_content_len > 0) {
            cur_w.* += LayoutTree.measureTextSegmentWithTabs(st.after_content_buf[0..st.after_content_len], st, cur_w.*);
            trailing_ws_w.* = 0.0;
            prev_ws.* = false;
        }
    }

    fn measureIntrinsicWidth(
        doc: *const Document,
        styles: []const ComputedStyle,
        node_id: NodeId,
        avail_w: f32,
    ): f32 {
        if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return 0.0;
        const node = &doc.nodes.items[@as(usize, node_id)];
        if (node.node_type == NodeType.Text) {
            let parent_st_id: NodeId = NULL_NODE;
            if (node.parent != NULL_NODE && @as(usize, node.parent) < styles.len) {
                parent_st_id = node.parent;
            }
            let cur_w: f32 = 0.0;
            let best_w: f32 = 0.0;
            let prev_ws = true;
            let trailing_ws_w: f32 = 0.0;
            _.measureInlineMaxRun(doc, styles, node_id, parent_st_id, avail_w, &cur_w, &best_w, &prev_ws, &trailing_ws_w);
            const final_cur = if (cur_w >= trailing_ws_w) (cur_w - trailing_ws_w) else cur_w;
            return @max(best_w, final_cur);
        }
        if (node.node_type != NodeType.Element) return 0.0;
        const st = &styles[@as(usize, node_id)];
        if (st.display.isNone()) return 0.0;

        const fs = st.font_size_px;
        const ml = st.margin_left.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
        const mr = st.margin_right.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
        const pl = st.padding_left.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
        const pr = st.padding_right.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
        const bl = st.border_left_width;
        const br = st.border_right_width;
        const h_decor = pl + pr + bl + br;

        let content_w: f32 = 0.0;
        const rep_opt = LayoutTree.getReplacedSizeWithStyle(node, st);
        const width_is_pct = !st.width.is_auto && !st.width.is_none && !st.width.isIntrinsic() &&
            !st.width.is_calc && st.width.length.unit == CssUnit.Percent;
        const max_width_is_pct = !st.max_width.is_auto && !st.max_width.is_none && !st.max_width.isIntrinsic() &&
            !st.max_width.is_calc && st.max_width.length.unit == CssUnit.Percent;
        const width_is_definite = !st.width.is_auto && !st.width.is_none && !st.width.isIntrinsic() &&
            (st.width.is_calc || st.width.length.unit != CssUnit.Percent);
        if (rep_opt != null && avail_w <= 0.0 && (width_is_pct || max_width_is_pct)) {
            content_w = 0.0;
        } else if (width_is_definite) {
            const w = st.width.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
            content_w = if (st.box_sizing == BoxSizing.BorderBox) (if (w > h_decor) (w - h_decor) else 0.0) else w;
        } else if (rep := rep_opt) {
            content_w = rep.width;
        } else if (st.display.isFlex() || st.display.isWebkitBox()) {
            const is_row = st.flex_direction.isRow();
            let sum_w: f32 = 0.0;
            let max_w: f32 = 0.0;
            let cnt: usize = 0;
            let ch = node.first_child;
            while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
                const cn = &doc.nodes.items[@as(usize, ch)];
                if (cn.node_type == NodeType.Element) {
                    const cst = &styles[@as(usize, ch)];
                    if (!cst.display.isNone() && cst.position != CssPosition.Absolute && cst.position != CssPosition.Fixed) {
                        const cw = _.measureIntrinsicWidth(doc, styles, ch, avail_w);
                        if (is_row) {
                            sum_w += cw;
                            cnt += 1;
                        } else {
                            if (cw > max_w) max_w = cw;
                        }
                    }
                } else if (cn.node_type == NodeType.Text) {
                    const trimmed = std.mem.trim(u8, cn.text_content, " \t\r\n");
                    if (trimmed.len > 0) {
                        const cw = LayoutTree.estimateTextWidth(trimmed, st);
                        if (is_row) {
                            sum_w += cw;
                            cnt += 1;
                        } else {
                            if (cw > max_w) max_w = cw;
                        }
                    }
                }
                ch = cn.next_sibling;
            }
            if (is_row) {
                if (cnt > 1) sum_w += @as(f32, @floatFromInt(cnt - 1)) * st.column_gap;
                content_w = sum_w;
            } else {
                content_w = max_w;
            }
        } else {
            let best_w: f32 = 0.0;
            let cur_w: f32 = 0.0;
            let prev_ws = true;
            let trailing_ws_w: f32 = 0.0;
            if (st.has_before_content && st.before_content_len > 0) {
                cur_w += LayoutTree.measureTextSegmentWithTabs(st.before_content_buf[0..st.before_content_len], st, cur_w);
                prev_ws = false;
                trailing_ws_w = 0.0;
            }
            let ch = node.first_child;
            while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
                const cn = &doc.nodes.items[@as(usize, ch)];
                if (cn.node_type == NodeType.Element) {
                    const cst = &styles[@as(usize, ch)];
                    if (cst.display.isNone() || cst.position == CssPosition.Absolute || cst.position == CssPosition.Fixed) {
                        ch = cn.next_sibling;
                        continue;
                    }
                    if (LayoutTree.isBlockLevelInFlow(cn, cst)) {
                        const final_cur = if (cur_w >= trailing_ws_w) (cur_w - trailing_ws_w) else cur_w;
                        if (final_cur > best_w) best_w = final_cur;
                        cur_w = 0.0;
                        prev_ws = true;
                        trailing_ws_w = 0.0;
                        const cw = _.measureIntrinsicWidth(doc, styles, ch, avail_w);
                        if (cw > best_w) best_w = cw;
                        ch = cn.next_sibling;
                        continue;
                    }
                }
                _.measureInlineMaxRun(doc, styles, ch, node_id, avail_w, &cur_w, &best_w, &prev_ws, &trailing_ws_w);
                ch = cn.next_sibling;
            }
            if (st.has_after_content && st.after_content_len > 0) {
                cur_w += LayoutTree.measureTextSegmentWithTabs(st.after_content_buf[0..st.after_content_len], st, cur_w);
                prev_ws = false;
                trailing_ws_w = 0.0;
            }
            const final_cur = if (cur_w >= trailing_ws_w) (cur_w - trailing_ws_w) else cur_w;
            if (final_cur > best_w) best_w = final_cur;
            content_w = best_w;
        }

        if (!st.min_width.is_auto && !st.min_width.is_none && !st.min_width.isIntrinsic() &&
            (st.min_width.is_calc || st.min_width.length.unit != CssUnit.Percent)) {
            const min_w_raw = st.min_width.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
            const min_c = if (st.box_sizing == BoxSizing.BorderBox) (if (min_w_raw > h_decor) (min_w_raw - h_decor) else 0.0) else min_w_raw;
            if (content_w < min_c) content_w = min_c;
        }
        if (!st.max_width.is_auto && !st.max_width.is_none && !st.max_width.isIntrinsic() &&
            (st.max_width.is_calc || st.max_width.length.unit != CssUnit.Percent)) {
            const max_w_raw = st.max_width.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
            const max_c = if (st.box_sizing == BoxSizing.BorderBox) (if (max_w_raw > h_decor) (max_w_raw - h_decor) else 0.0) else max_w_raw;
            if (content_w > max_c) content_w = max_c;
        }
        return content_w + h_decor + ml + mr;
    }

    fn measureMinContentWidth(
        doc: *const Document,
        styles: []const ComputedStyle,
        node_id: NodeId,
        avail_w: f32,
    ): f32 {
        if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return 0.0;
        const node = &doc.nodes.items[@as(usize, node_id)];
        if (node.node_type == NodeType.Text) {
            let parent_fs: f32 = 16.0;
            let allows_wrap = true;
            let parent_st_opt: (*const ComputedStyle)? = null;
            if (node.parent != NULL_NODE && @as(usize, node.parent) < styles.len) {
                parent_st_opt = &styles[@as(usize, node.parent)];
                parent_fs = styles[@as(usize, node.parent)].font_size_px;
                allows_wrap = styles[@as(usize, node.parent)].white_space.allowsWrap();
            }
            if (!allows_wrap) {
                return _.measureIntrinsicWidth(doc, styles, node_id, avail_w);
            }
            // Longest single word width
            let max_word_w: f32 = 0.0;
            let cur_w: f32 = 0.0;
            let ti: usize = 0;
            while (ti < node.text_content.len) : (ti += 1) {
                const ch = node.text_content[ti];
                if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                    if (cur_w > max_word_w) max_word_w = cur_w;
                    cur_w = 0.0;
                } else {
                    if (pst := parent_st_opt) {
                        cur_w += LayoutTree.charWidthForStyle(ch, pst);
                    } else {
                        cur_w += LayoutTree.estimateCharWidth(ch, parent_fs);
                    }
                }
            }
            if (cur_w > max_word_w) max_word_w = cur_w;
            return max_word_w;
        }
        if (node.node_type != NodeType.Element) return 0.0;
        const st = &styles[@as(usize, node_id)];
        if (st.display.isNone()) return 0.0;

        const fs = st.font_size_px;
        const ml = st.margin_left.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
        const mr = st.margin_right.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
        const pl = st.padding_left.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
        const pr = st.padding_right.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
        const bl = st.border_left_width;
        const br = st.border_right_width;
        const h_decor = pl + pr + bl + br;

        let content_w: f32 = 0.0;
        const rep_opt = LayoutTree.getReplacedSizeWithStyle(node, st);
        const width_is_pct = !st.width.is_auto && !st.width.is_none && !st.width.isIntrinsic() &&
            !st.width.is_calc && st.width.length.unit == CssUnit.Percent;
        const max_width_is_pct = !st.max_width.is_auto && !st.max_width.is_none && !st.max_width.isIntrinsic() &&
            !st.max_width.is_calc && st.max_width.length.unit == CssUnit.Percent;
        const width_is_definite = !st.width.is_auto && !st.width.is_none && !st.width.isIntrinsic() &&
            (st.width.is_calc || st.width.length.unit != CssUnit.Percent);
        if (rep_opt != null && avail_w <= 0.0 && (width_is_pct || max_width_is_pct)) {
            content_w = 0.0;
        } else if (width_is_definite) {
            const w = st.width.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
            content_w = if (st.box_sizing == BoxSizing.BorderBox) (if (w > h_decor) (w - h_decor) else 0.0) else w;
        } else if (rep := rep_opt) {
            content_w = rep.width;
        } else if (st.display.isFlex() || st.display.isWebkitBox()) {
            const is_row_nowrap = st.flex_direction.isRow() && st.flex_wrap == FlexWrap.NoWrap;
            let sum_w: f32 = 0.0;
            let max_w: f32 = 0.0;
            let cnt: usize = 0;
            let ch = node.first_child;
            while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
                const cn = &doc.nodes.items[@as(usize, ch)];
                if (cn.node_type == NodeType.Element) {
                    const cst = &styles[@as(usize, ch)];
                    if (!cst.display.isNone() && cst.position != CssPosition.Absolute && cst.position != CssPosition.Fixed) {
                        const cw = _.measureMinContentWidth(doc, styles, ch, avail_w);
                        if (is_row_nowrap) {
                            sum_w += cw;
                            cnt += 1;
                        } else {
                            if (cw > max_w) max_w = cw;
                        }
                    }
                }
                ch = cn.next_sibling;
            }
            if (is_row_nowrap) {
                if (cnt > 1) sum_w += @as(f32, @floatFromInt(cnt - 1)) * st.column_gap;
                content_w = sum_w;
            } else {
                content_w = max_w;
            }
        } else {
            let max_child_w: f32 = 0.0;
            let ch = node.first_child;
            while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
                const cn = &doc.nodes.items[@as(usize, ch)];
                if (cn.node_type == NodeType.Element) {
                    const cst = &styles[@as(usize, ch)];
                    if (cst.display.isNone() || cst.position == CssPosition.Absolute || cst.position == CssPosition.Fixed) {
                        ch = cn.next_sibling;
                        continue;
                    }
                }
                const cw = _.measureMinContentWidth(doc, styles, ch, avail_w);
                if (cw > max_child_w) max_child_w = cw;
                ch = cn.next_sibling;
            }
            content_w = max_child_w;
        }

        if (!st.min_width.is_auto && !st.min_width.is_none && !st.min_width.isIntrinsic() &&
            (st.min_width.is_calc || st.min_width.length.unit != CssUnit.Percent)) {
            const min_w_raw = st.min_width.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
            const min_c = if (st.box_sizing == BoxSizing.BorderBox) (if (min_w_raw > h_decor) (min_w_raw - h_decor) else 0.0) else min_w_raw;
            if (content_w < min_c) content_w = min_c;
        }
        if (!st.max_width.is_auto && !st.max_width.is_none && !st.max_width.isIntrinsic() &&
            (st.max_width.is_calc || st.max_width.length.unit != CssUnit.Percent)) {
            const max_w_raw = st.max_width.resolvePx(avail_w, fs, _.root_font_px, _.viewport_width, _.viewport_height);
            const max_c = if (st.box_sizing == BoxSizing.BorderBox) (if (max_w_raw > h_decor) (max_w_raw - h_decor) else 0.0) else max_w_raw;
            if (content_w > max_c) content_w = max_c;
        }
        return content_w + h_decor + ml + mr;
    }

    fn layoutTableContainer(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        table_id: NodeId,
        origin_x: f32,
        origin_y: f32,
        avail_w: f32,
    ) {
        let children: [64]NodeId = [NULL_NODE; 64];
        let child_count: usize = 0;
        let ch = doc.nodes.items[@as(usize, table_id)].first_child;
        while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
            const cn = &doc.nodes.items[@as(usize, ch)];
            if (cn.node_type == NodeType.Element) {
                if (!styles[@as(usize, ch)].display.isNone()) {
                    if (child_count < 64) {
                        children[child_count] = ch;
                        child_count += 1;
                    }
                } else {
                    _.boxes.items[@as(usize, ch)].rect = Rect.zero();
                }
            }
            ch = cn.next_sibling;
        }

        const tstyle = &styles[@as(usize, table_id)];
        const dims = _.layoutTableCore(doc, styles, table_id, tstyle, children[0..child_count], origin_x, origin_y, avail_w);
        let tbox = &_.boxes.items[@as(usize, table_id)];
        const h_decor = tbox.paddingHorizontal() + tbox.borderHorizontal();
        const v_decor = tbox.paddingVertical() + tbox.borderVertical();
        const core_w = if (dims[0] > h_decor) (dims[0] - h_decor) else 0.0;
        const core_h = if (dims[1] > v_decor) (dims[1] - v_decor) else 0.0;
        if (tstyle.border_collapse == BorderCollapse.Collapse) {
            tbox.content_width = core_w;
            tbox.content_height = core_h;
        } else {
            tbox.content_width = if (!tstyle.width.is_auto && !tstyle.width.is_none) @max(tbox.content_width, core_w) else core_w;
            tbox.content_height = if (!tbox.height_is_auto) @max(tbox.content_height, core_h) else core_h;
        }
    }

    fn layoutTableCore(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        table_id: NodeId,
        tstyle_opt: (*const ComputedStyle)?,
        direct_children: []const NodeId,
        origin_x: f32,
        origin_y: f32,
        avail_w: f32,
    ): [2]f32 {
        let is_collapse = false;
        let h_spacing: f32 = 0.0;
        let v_spacing: f32 = 0.0;
        let is_fixed = false;
        let has_explicit_table_w = false;
        let explicit_table_w: f32 = 0.0;
        let table_border_h: f32 = 0.0;
        let table_border_v: f32 = 0.0;
        let table_pad_h: f32 = 0.0;
        let table_pad_v: f32 = 0.0;
        let table_border_left: f32 = 0.0;
        let table_border_top: f32 = 0.0;
        let table_pad_left: f32 = 0.0;
        let table_pad_top: f32 = 0.0;

        if (ts := tstyle_opt) {
            is_collapse = (ts.border_collapse == BorderCollapse.Collapse);
            if (!is_collapse) {
                h_spacing = ts.border_spacing_h;
                v_spacing = ts.border_spacing_v;
            }
            is_fixed = (ts.table_layout == TableLayout.Fixed);
            if (table_id != NULL_NODE) {
                const tb = &_.boxes.items[@as(usize, table_id)];
                if (!is_collapse) {
                    table_border_h = tb.borderHorizontal();
                    table_border_v = tb.borderVertical();
                    table_pad_h = tb.paddingHorizontal();
                    table_pad_v = tb.paddingVertical();
                    table_border_left = tb.border_left;
                    table_border_top = tb.border_top;
                    table_pad_left = tb.padding_left;
                    table_pad_top = tb.padding_top;
                }
            }
            if (!ts.width.is_auto && !ts.width.is_none) {
                has_explicit_table_w = true;
                const raw_tw = ts.width.resolvePx(avail_w, ts.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                if (ts.box_sizing == BoxSizing.BorderBox || is_collapse) {
                    explicit_table_w = raw_tw;
                } else {
                    explicit_table_w = raw_tw + table_border_h + table_pad_h;
                }
            }
        }

        // 1. Partition children into top_captions, bottom_captions, and ordered row/cell structure
        let top_captions: [16]NodeId = [NULL_NODE; 16];
        let top_cap_count: usize = 0;
        let bot_captions: [16]NodeId = [NULL_NODE; 16];
        let bot_cap_count: usize = 0;

        // Collect row groups in CSS order: thead, tbody/stray, tfoot
        let head_items: [64]NodeId = [NULL_NODE; 64];
        let head_count: usize = 0;
        let body_items: [64]NodeId = [NULL_NODE; 64];
        let body_count: usize = 0;
        let foot_items: [64]NodeId = [NULL_NODE; 64];
        let foot_count: usize = 0;

        let ci: usize = 0;
        while (ci < direct_children.len) : (ci += 1) {
            const cid = direct_children[ci];
            const cstyle = &styles[@as(usize, cid)];
            if (cstyle.display.isTableCaption()) {
                if (cstyle.caption_side == CaptionSide.Bottom) {
                    if (bot_cap_count < 16) {
                        bot_captions[bot_cap_count] = cid;
                        bot_cap_count += 1;
                    }
                } else {
                    if (top_cap_count < 16) {
                        top_captions[top_cap_count] = cid;
                        top_cap_count += 1;
                    }
                }
            } else if (cstyle.display.isTableColumn()) {
                _.boxes.items[@as(usize, cid)].rect = Rect.zero();
            } else if (cstyle.display.isTableHeaderGroup()) {
                if (head_count < 64) {
                    head_items[head_count] = cid;
                    head_count += 1;
                }
            } else if (cstyle.display.isTableFooterGroup()) {
                if (foot_count < 64) {
                    foot_items[foot_count] = cid;
                    foot_count += 1;
                }
            } else {
                if (body_count < 64) {
                    body_items[body_count] = cid;
                    body_count += 1;
                }
            }
        }

        // Flatten ordered sections into rows & cells
        let rows: [64]TableRowEntry = [TableRowEntry{}; 64];
        let row_count: usize = 0;
        let cells: [128]TableCellEntry = [TableCellEntry{}; 128];
        let cell_count: usize = 0;
        let occupied: [2048]bool = [false; 2048]; // 64 rows x 32 cols
        let max_cols: usize = 0;

        // Build unified list of items to process across head, body, foot
        let ordered: [128]NodeId = [NULL_NODE; 128];
        let ordered_group: [128]NodeId = [NULL_NODE; 128];
        let ord_count: usize = 0;

        let pass: usize = 0;
        while (pass < 3) : (pass += 1) {
            const src_len = if (pass == 0) head_count else (if (pass == 1) body_count else foot_count);
            let si: usize = 0;
            while (si < src_len) : (si += 1) {
                const item_id = if (pass == 0) head_items[si] else (if (pass == 1) body_items[si] else foot_items[si]);
                const istyle = &styles[@as(usize, item_id)];
                if (istyle.display.isTableRowGroup()) {
                    let gc = doc.nodes.items[@as(usize, item_id)].first_child;
                    while (gc != NULL_NODE && @as(usize, gc) < doc.nodes.items.len) {
                        const gcn = &doc.nodes.items[@as(usize, gc)];
                        if (gcn.node_type == NodeType.Element) {
                            if (!styles[@as(usize, gc)].display.isNone()) {
                                if (ord_count < 128) {
                                    ordered[ord_count] = gc;
                                    ordered_group[ord_count] = item_id;
                                    ord_count += 1;
                                }
                            } else {
                                _.boxes.items[@as(usize, gc)].rect = Rect.zero();
                            }
                        }
                        gc = gcn.next_sibling;
                    }
                } else {
                    if (ord_count < 128) {
                        ordered[ord_count] = item_id;
                        ordered_group[ord_count] = NULL_NODE;
                        ord_count += 1;
                    }
                }
            }
        }

        // Convert ordered items into rows & cells with rowspan/colspan grid placement
        let oi: usize = 0;
        while (oi < ord_count && row_count < 64) {
            const item_id = ordered[oi];
            const grp_id = ordered_group[oi];
            const istyle = &styles[@as(usize, item_id)];

            if (istyle.display.isTableRow()) {
                const r_idx = row_count;
                rows[r_idx] = TableRowEntry{ node_id: item_id, group_id: grp_id };
                row_count += 1;
                let cur_col: usize = 0;

                let rc = doc.nodes.items[@as(usize, item_id)].first_child;
                while (rc != NULL_NODE && @as(usize, rc) < doc.nodes.items.len) {
                    const rcn = &doc.nodes.items[@as(usize, rc)];
                    if (rcn.node_type == NodeType.Element) {
                        const rcstyle = &styles[@as(usize, rc)];
                        if (rcstyle.display.isNone()) {
                            _.boxes.items[@as(usize, rc)].rect = Rect.zero();
                        } else if (rcstyle.display.isTableCell()) {
                            while (cur_col < 32 && occupied[r_idx * 32 + cur_col]) : (cur_col += 1) {}
                            if (cur_col < 32 && cell_count < 128) {
                                const cs = LayoutTree.parseSpanAttr(rcn, "colspan");
                                const rs = LayoutTree.parseSpanAttr(rcn, "rowspan");
                                cells[cell_count] = TableCellEntry{
                                    node_id: rc,
                                    is_anon: false,
                                    row: r_idx,
                                    col: cur_col,
                                    rowspan: rs,
                                    colspan: cs,
                                };
                                cell_count += 1;
                                let dr: usize = 0;
                                while (dr < rs && r_idx + dr < 64) : (dr += 1) {
                                    let dc: usize = 0;
                                    while (dc < cs && cur_col + dc < 32) : (dc += 1) {
                                        occupied[(r_idx + dr) * 32 + (cur_col + dc)] = true;
                                    }
                                }
                                if (cur_col + cs > max_cols) max_cols = cur_col + cs;
                                cur_col += cs;
                            }
                        } else {
                            // Non-table-cell inside table-row -> wrap in anonymous cell
                            while (cur_col < 32 && occupied[r_idx * 32 + cur_col]) : (cur_col += 1) {}
                            if (cur_col < 32 && cell_count < 128) {
                                let entry = TableCellEntry{
                                    node_id: NULL_NODE,
                                    is_anon: true,
                                    row: r_idx,
                                    col: cur_col,
                                    rowspan: 1,
                                    colspan: 1,
                                };
                                entry.anon_nodes[0] = rc;
                                entry.anon_count = 1;
                                cells[cell_count] = entry;
                                cell_count += 1;
                                occupied[r_idx * 32 + cur_col] = true;
                                if (cur_col + 1 > max_cols) max_cols = cur_col + 1;
                                cur_col += 1;
                            }
                        }
                    }
                    rc = rcn.next_sibling;
                }
                oi += 1;
            } else if (istyle.display.isTableCell()) {
                // Consecutive stray table-cells form a single anonymous table-row
                const r_idx = row_count;
                rows[r_idx] = TableRowEntry{ node_id: NULL_NODE, group_id: grp_id };
                row_count += 1;
                let cur_col: usize = 0;
                while (oi < ord_count) {
                    const cid = ordered[oi];
                    if (ordered_group[oi] != grp_id || !styles[@as(usize, cid)].display.isTableCell()) break;
                    const cn = &doc.nodes.items[@as(usize, cid)];
                    while (cur_col < 32 && occupied[r_idx * 32 + cur_col]) : (cur_col += 1) {}
                    if (cur_col < 32 && cell_count < 128) {
                        const cs = LayoutTree.parseSpanAttr(cn, "colspan");
                        const rs = LayoutTree.parseSpanAttr(cn, "rowspan");
                        cells[cell_count] = TableCellEntry{
                            node_id: cid,
                            is_anon: false,
                            row: r_idx,
                            col: cur_col,
                            rowspan: rs,
                            colspan: cs,
                        };
                        cell_count += 1;
                        let dr: usize = 0;
                        while (dr < rs && r_idx + dr < 64) : (dr += 1) {
                            let dc: usize = 0;
                            while (dc < cs && cur_col + dc < 32) : (dc += 1) {
                                occupied[(r_idx + dr) * 32 + (cur_col + dc)] = true;
                            }
                        }
                        if (cur_col + cs > max_cols) max_cols = cur_col + cs;
                        cur_col += cs;
                    }
                    oi += 1;
                }
            } else {
                // Consecutive non-table children inside display:table -> single anonymous cell in an anonymous row
                const r_idx = row_count;
                rows[r_idx] = TableRowEntry{ node_id: NULL_NODE, group_id: grp_id };
                row_count += 1;
                let cur_col: usize = 0;
                while (cur_col < 32 && occupied[r_idx * 32 + cur_col]) : (cur_col += 1) {}
                if (cur_col < 32 && cell_count < 128) {
                    let entry = TableCellEntry{
                        node_id: NULL_NODE,
                        is_anon: true,
                        row: r_idx,
                        col: cur_col,
                        rowspan: 1,
                        colspan: 1,
                    };
                    while (oi < ord_count) {
                        const nid = ordered[oi];
                        const nst = &styles[@as(usize, nid)];
                        if (ordered_group[oi] != grp_id || nst.display.isTableRow() || nst.display.isTableCell()) break;
                        if (entry.anon_count < 16) {
                            entry.anon_nodes[entry.anon_count] = nid;
                            entry.anon_count += 1;
                        }
                        oi += 1;
                    }
                    cells[cell_count] = entry;
                    cell_count += 1;
                    occupied[r_idx * 32 + cur_col] = true;
                    if (cur_col + 1 > max_cols) max_cols = cur_col + 1;
                } else {
                    oi += 1;
                }
            }
        }

        // Clamp rowspan to actual row_count
        let ci2: usize = 0;
        while (ci2 < cell_count) : (ci2 += 1) {
            if (cells[ci2].row + cells[ci2].rowspan > row_count) {
                cells[ci2].rowspan = row_count - cells[ci2].row;
            }
            if (cells[ci2].rowspan == 0) cells[ci2].rowspan = 1;
        }

        // 2. Measure preferred width of each cell
        let col_w: [32]f32 = [0.0; 32];
        let ci3: usize = 0;
        while (ci3 < cell_count) : (ci3 += 1) {
            let cell = &cells[ci3];
            let pw: f32 = 0.0;
            if (!cell.is_anon) {
                const cstyle = &styles[@as(usize, cell.node_id)];
                if (!cstyle.width.is_auto && !cstyle.width.is_none) {
                    const rw = cstyle.width.resolvePx(avail_w, cstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                    if (cstyle.box_sizing == BoxSizing.BorderBox) {
                        pw = rw;
                    } else {
                        const pl = cstyle.padding_left.resolvePx(avail_w, cstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                        const pr = cstyle.padding_right.resolvePx(avail_w, cstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                        const bdr_h = if (is_collapse) 0.0 else (cstyle.border_left_width + cstyle.border_right_width);
                        pw = rw + pl + pr + bdr_h;
                    }
                } else {
                    pw = _.measureIntrinsicWidth(doc, styles, cell.node_id, avail_w);
                }
            } else {
                let ai: usize = 0;
                while (ai < cell.anon_count) : (ai += 1) {
                    const aw = _.measureIntrinsicWidth(doc, styles, cell.anon_nodes[ai], avail_w);
                    if (aw > pw) pw = aw;
                }
            }
            cell.pref_w = pw;
            if (cell.colspan == 1 && cell.col < 32) {
                if (pw > col_w[cell.col]) col_w[cell.col] = pw;
            }
        }

        // Distribute colspan > 1 widths
        let ci4: usize = 0;
        while (ci4 < cell_count) : (ci4 += 1) {
            const cell = &cells[ci4];
            if (cell.colspan > 1) {
                let cur_span_w: f32 = 0.0;
                let dc: usize = 0;
                while (dc < cell.colspan && cell.col + dc < 32) : (dc += 1) {
                    cur_span_w += col_w[cell.col + dc];
                }
                if (cell.colspan > 1) {
                    cur_span_w += @as(f32, @floatFromInt(cell.colspan - 1)) * h_spacing;
                }
                if (cell.pref_w > cur_span_w) {
                    const add_each = (cell.pref_w - cur_span_w) / @as(f32, @floatFromInt(cell.colspan));
                    let dc2: usize = 0;
                    while (dc2 < cell.colspan && cell.col + dc2 < 32) : (dc2 += 1) {
                        col_w[cell.col + dc2] += add_each;
                    }
                }
            }
        }

        let sum_cols_w: f32 = 0.0;
        let k: usize = 0;
        while (k < max_cols) : (k += 1) {
            sum_cols_w += col_w[k];
        }
        const total_h_spacing = if (max_cols > 0) (@as(f32, @floatFromInt(max_cols + 1)) * h_spacing) else 0.0;
        const natural_grid_w = sum_cols_w + total_h_spacing + table_border_h + table_pad_h;

        // 3. Measure caption min-content widths (CSS Table 3: table width >= max caption min-content width)
        let max_cap_min_w: f32 = 0.0;
        let tci: usize = 0;
        while (tci < top_cap_count) : (tci += 1) {
            const cw = _.measureMinContentWidth(doc, styles, top_captions[tci], avail_w);
            if (cw > max_cap_min_w) max_cap_min_w = cw;
        }
        let bci: usize = 0;
        while (bci < bot_cap_count) : (bci += 1) {
            const cw = _.measureMinContentWidth(doc, styles, bot_captions[bci], avail_w);
            if (cw > max_cap_min_w) max_cap_min_w = cw;
        }

        // 4. Final table border-box width
        let final_table_w: f32 = natural_grid_w;
        if (has_explicit_table_w) {
            final_table_w = @max(explicit_table_w, max_cap_min_w);
        } else {
            final_table_w = @max(natural_grid_w, max_cap_min_w);
        }

        // Expand or distribute column widths to match final_table_w
        if (max_cols > 0) {
            const target_cols_sum = @max(0.0, final_table_w - table_border_h - table_pad_h - total_h_spacing);
            if (sum_cols_w <= 0.0) {
                const each_w = target_cols_sum / @as(f32, @floatFromInt(max_cols));
                let c0: usize = 0;
                while (c0 < max_cols) : (c0 += 1) {
                    col_w[c0] = each_w;
                }
            } else if (target_cols_sum > sum_cols_w) {
                const extra_each = (target_cols_sum - sum_cols_w) / @as(f32, @floatFromInt(max_cols));
                let c0: usize = 0;
                while (c0 < max_cols) : (c0 += 1) {
                    col_w[c0] += extra_each;
                }
            } else if (is_fixed && target_cols_sum < sum_cols_w) {
                const scale = target_cols_sum / sum_cols_w;
                let c0: usize = 0;
                while (c0 < max_cols) : (c0 += 1) {
                    col_w[c0] *= scale;
                }
            }
        }

        // 5. Lay out Top Captions
        let cur_y = origin_y;
        let tci2: usize = 0;
        while (tci2 < top_cap_count) : (tci2 += 1) {
            const cap_id = top_captions[tci2];
            _.layoutNodeEx(doc, styles, cap_id, origin_x, cur_y, final_table_w, 0.0, origin_x, cur_y, false);
            cur_y += _.boxes.items[@as(usize, cap_id)].outerHeight();
        }

        // 6. Lay out Table Grid Rows & Cells
        const grid_top_y = cur_y;
        const grid_inner_x = origin_x + table_border_left + table_pad_left;
        let col_x: [32]f32 = [0.0; 32];
        let cx_cur = grid_inner_x + h_spacing;
        let c1: usize = 0;
        while (c1 < max_cols) : (c1 += 1) {
            col_x[c1] = cx_cur;
            cx_cur += col_w[c1] + h_spacing;
        }

        // First pass: lay out each cell at its final width to measure height
        let row_h: [64]f32 = [0.0; 64];
        let ci5: usize = 0;
        while (ci5 < cell_count) : (ci5 += 1) {
            let cell = &cells[ci5];
            let cell_w: f32 = 0.0;
            let dc: usize = 0;
            while (dc < cell.colspan && cell.col + dc < max_cols) : (dc += 1) {
                cell_w += col_w[cell.col + dc];
            }
            if (cell.colspan > 1) {
                cell_w += @as(f32, @floatFromInt(cell.colspan - 1)) * h_spacing;
            }
            const cell_x = if (cell.col < max_cols) col_x[cell.col] else grid_inner_x;
            cell.layout_x = cell_x;
            cell.layout_y = grid_top_y;

            if (!cell.is_anon) {
                const cid = cell.node_id;
                const cstyle = &styles[@as(usize, cid)];
                _.layoutNodeEx(doc, styles, cid, cell_x, grid_top_y, cell_w, 0.0, cell_x, grid_top_y, false);
                let cbox = &_.boxes.items[@as(usize, cid)];
                const ch_decor = cbox.paddingHorizontal() + cbox.borderHorizontal();
                cbox.content_width = if (cell_w > ch_decor) (cell_w - ch_decor) else 0.0;
                cbox.rect.x = cell_x;
                cbox.rect.width = cell_w;
                let measured = cbox.borderBoxHeight();
                if (is_collapse && !cstyle.height.is_auto && !cstyle.height.is_none) {
                    const rh = cstyle.height.resolvePx(0.0, cstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                    const bdr_half = (cstyle.border_top_width + cstyle.border_bottom_width) * 0.5;
                    const expected_h = rh + cbox.paddingVertical() + bdr_half;
                    if (measured > expected_h) {
                        measured = expected_h;
                    }
                }
                cell.measured_h = measured;
            } else {
                let anon_y = grid_top_y;
                let ai: usize = 0;
                while (ai < cell.anon_count) : (ai += 1) {
                    const aid = cell.anon_nodes[ai];
                    _.layoutNodeEx(doc, styles, aid, cell_x, anon_y, cell_w, 0.0, cell_x, anon_y, false);
                    anon_y += _.boxes.items[@as(usize, aid)].outerHeight();
                }
                cell.measured_h = anon_y - grid_top_y;
            }

            if (cell.rowspan == 1 && cell.row < 64) {
                if (cell.measured_h > row_h[cell.row]) {
                    row_h[cell.row] = cell.measured_h;
                }
            }
        }

        // Distribute rowspan > 1 heights
        let ci6: usize = 0;
        while (ci6 < cell_count) : (ci6 += 1) {
            const cell = &cells[ci6];
            if (cell.rowspan > 1) {
                let cur_span_h: f32 = 0.0;
                let dr: usize = 0;
                while (dr < cell.rowspan && cell.row + dr < row_count) : (dr += 1) {
                    cur_span_h += row_h[cell.row + dr];
                }
                if (cell.rowspan > 1) {
                    cur_span_h += @as(f32, @floatFromInt(cell.rowspan - 1)) * v_spacing;
                }
                if (cell.measured_h > cur_span_h && cell.row + cell.rowspan > 0) {
                    const last_r = cell.row + cell.rowspan - 1;
                    if (last_r < row_count) {
                        row_h[last_r] += (cell.measured_h - cur_span_h);
                    }
                }
            }
        }

        // Distribute extra explicit table height across rows (CSS Table 3 / HTML tables)
        if (table_id != NULL_NODE && row_count > 0) {
            const tb_h = &_.boxes.items[@as(usize, table_id)];
            if (!tb_h.height_is_auto) {
                const total_v_sp = @as(f32, @floatFromInt(row_count + 1)) * v_spacing;
                let sum_rows_h: f32 = 0.0;
                let rh_i: usize = 0;
                while (rh_i < row_count) : (rh_i += 1) {
                    sum_rows_h += row_h[rh_i];
                }
                if (tb_h.content_height > sum_rows_h + total_v_sp) {
                    const add_per_row = (tb_h.content_height - sum_rows_h - total_v_sp) / @as(f32, @floatFromInt(row_count));
                    let rh_j: usize = 0;
                    while (rh_j < row_count) : (rh_j += 1) {
                        row_h[rh_j] += add_per_row;
                    }
                }
            }
        }

        // Compute row Y positions
        let row_y: [64]f32 = [0.0; 64];
        let ry_cur = grid_top_y + table_border_top + table_pad_top + v_spacing;
        let r1: usize = 0;
        while (r1 < row_count) : (r1 += 1) {
            row_y[r1] = ry_cur;
            ry_cur += row_h[r1] + v_spacing;
        }
        const grid_total_h = if (row_count > 0) (ry_cur - grid_top_y + table_border_v - table_border_top + table_pad_v - table_pad_top) else (table_border_v + table_pad_v);

        // Second pass: position each cell at (col_x[col], row_y[row]) with spanned height
        let ci7: usize = 0;
        while (ci7 < cell_count) : (ci7 += 1) {
            const cell = &cells[ci7];
            let cell_h: f32 = 0.0;
            let dr: usize = 0;
            while (dr < cell.rowspan && cell.row + dr < row_count) : (dr += 1) {
                cell_h += row_h[cell.row + dr];
            }
            if (cell.rowspan > 1) {
                cell_h += @as(f32, @floatFromInt(cell.rowspan - 1)) * v_spacing;
            }
            const target_y = if (cell.row < row_count) row_y[cell.row] else grid_top_y;
            const dy = target_y - cell.layout_y;

            if (!cell.is_anon) {
                const cid = cell.node_id;
                if (dy != 0.0) {
                    _.shiftSubtree(doc, cid, 0.0, dy);
                }
                let cbox = &_.boxes.items[@as(usize, cid)];
                cbox.rect.y = target_y;
                cbox.rect.height = cell_h;
                const cv_decor = cbox.paddingVertical() + cbox.borderVertical();
                cbox.content_height = if (cell_h > cv_decor) (cell_h - cv_decor) else 0.0;
            } else {
                if (dy != 0.0) {
                    let ai: usize = 0;
                    while (ai < cell.anon_count) : (ai += 1) {
                        _.shiftSubtree(doc, cell.anon_nodes[ai], 0.0, dy);
                    }
                }
            }
        }

        // Set rects for explicit <tr> and row-group elements so painter visits their children
        let r2: usize = 0;
        while (r2 < row_count) : (r2 += 1) {
            const rid = rows[r2].node_id;
            if (rid != NULL_NODE) {
                _.boxes.items[@as(usize, rid)].rect = Rect{
                    x: grid_inner_x,
                    y: row_y[r2],
                    width: @max(0.0, final_table_w - table_border_h - table_pad_h),
                    height: row_h[r2],
                    visible: styles[@as(usize, rid)].visibility == Visibility.Visible,
                };
            }
            const gid = rows[r2].group_id;
            if (gid != NULL_NODE) {
                let gbox = &_.boxes.items[@as(usize, gid)];
                if (!gbox.rect.visible) {
                    gbox.rect = Rect{
                        x: grid_inner_x,
                        y: row_y[r2],
                        width: @max(0.0, final_table_w - table_border_h - table_pad_h),
                        height: row_h[r2],
                        visible: styles[@as(usize, gid)].visibility == Visibility.Visible,
                    };
                } else {
                    const new_bottom = row_y[r2] + row_h[r2];
                    if (new_bottom > gbox.rect.bottom()) {
                        gbox.rect.height = new_bottom - gbox.rect.y;
                    }
                }
            }
        }

        cur_y = grid_top_y + grid_total_h;

        // 7. Lay out Bottom Captions
        let bci2: usize = 0;
        while (bci2 < bot_cap_count) : (bci2 += 1) {
            const cap_id = bot_captions[bci2];
            _.layoutNodeEx(doc, styles, cap_id, origin_x, cur_y, final_table_w, 0.0, origin_x, cur_y, false);
            cur_y += _.boxes.items[@as(usize, cap_id)].outerHeight();
        }

        const total_h = cur_y - origin_y;
        const res: [2]f32 = [final_table_w, total_h];
        return res;
    }

    static fn isBlockLevelInFlow(node: *const NodeData, style: *const ComputedStyle): bool {
        if (node.node_type != NodeType.Element) return false;
        if (style.display.isNone()) return false;
        if (style.position == CssPosition.Absolute || style.position == CssPosition.Fixed) return false;
        if (style.float_mode != FloatMode.None) return false;
        if (style.display.isTableInternal()) return true;
        if (node.hasTag("br")) return false;
        if (style.display.isInline() || style.display.isInlineBlock() || style.display.isInlineFlex() || style.display.isInlineTable()) {
            return false;
        }
        return true;
    }

    static fn isAtomicInline(node: *const NodeData, style: *const ComputedStyle): bool {
        if (node.node_type != NodeType.Element) return false;
        if (style.display.isInlineBlock() || style.display.isInlineFlex() || style.display.isInlineTable()) return true;
        if (node.hasTag("svg") || LayoutTree.getReplacedSize(node) != null) return true;
        return false;
    }

    static fn estimateTextWidth(text: []const u8, style: *const ComputedStyle): f32 {
        const trimmed = std.mem.trim(u8, text, " \t\r\n");
        if (trimmed.len == 0) return 0.0;
        const base_size = if (style.font_size_px > 0.0) style.font_size_px else 16.0;
        let w: f32 = 0.0;
        let in_ws = false;
        const space_w = base_size * 0.28;
        let i: usize = 0;
        while (i < trimmed.len) : (i += 1) {
            const ch = trimmed[i];
            const is_ws = (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
            if (is_ws) {
                if (!in_ws) {
                    w += space_w;
                    in_ws = true;
                }
            } else {
                in_ws = false;
                w += LayoutTree.estimateCharWidth(ch, base_size);
            }
        }
        return w;
    }

    static fn estimateTextWrappedLines(text: []const u8, max_w: f32, font_size_px: f32): usize {
        if (max_w <= 0.0 || font_size_px <= 0.0) return 1;
        let lines: usize = 1;
        let line_w: f32 = 0.0;
        let word_w: f32 = 0.0;
        let in_ws = true;
        let has_any = false;
        const space_w = font_size_px * 0.28;
        let ti: usize = 0;
        while (ti < text.len) : (ti += 1) {
            const ch = text[ti];
            const is_ws = (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
            if (is_ws) {
                if (!in_ws) {
                    if (line_w > 0.0 && line_w + space_w + word_w > max_w) {
                        lines += 1;
                        line_w = word_w;
                    } else if (line_w > 0.0) {
                        line_w += space_w + word_w;
                    } else {
                        line_w = word_w;
                    }
                    word_w = 0.0;
                    in_ws = true;
                }
            } else {
                has_any = true;
                in_ws = false;
                word_w += LayoutTree.estimateCharWidth(ch, font_size_px);
            }
        }
        if (!in_ws && word_w > 0.0) {
            if (line_w > 0.0 && line_w + space_w + word_w > max_w) {
                lines += 1;
            }
        }
        if (!has_any) return 1;
        return lines;
    }

    fn appendTextSegment(
        *_,
        seg: []const u8,
        node_id: NodeId,
        style_parent_id: NodeId,
        st: *const ComputedStyle,
        content_w: f32,
        items: *std.ArrayListUnmanaged(InlineItem),
        prev_ws: *bool,
    ) {
        if (seg.len == 0) return;
        const base_sz = if (st.font_size_px > 0.0) st.font_size_px else 16.0;
        const space_w = LayoutTree.spaceWidth(st);
        const preserve_sp = st.white_space.preservesSpaces();
        const allows_wrap = st.white_space.allowsWrap();

        if (preserve_sp) {
            let has_tabs = false;
            let has_non_space = false;
            let ti: usize = 0;
            while (ti < seg.len) : (ti += 1) {
                if (seg[ti] == '\t') {
                    has_tabs = true;
                    has_non_space = true;
                } else if (seg[ti] != ' ' && seg[ti] != '\r') {
                    has_non_space = true;
                }
            }
            const tw = LayoutTree.measureTextSegmentWithTabs(seg, st, 0.0);
            const ends_with_space = (seg.len > 0 && seg[seg.len - 1] == ' ');
            const starts_with_space = (seg.len > 0 && seg[0] == ' ');
            const wlines = if (allows_wrap && content_w > 0.0 && tw > content_w && !has_tabs) LayoutTree.estimateTextWrappedLines(seg, content_w, base_sz) else @as(usize, 1);
            items.append(_.allocator, InlineItem {
                kind: InlineItemKind.Text,
                node_id: node_id,
                style_node_id: style_parent_id,
                width: tw,
                height: st.naturalLineHeight(),
                wrapped_lines: wlines,
                has_leading_space: allows_wrap && starts_with_space,
                has_trailing_space: allows_wrap && ends_with_space,
                is_space_only: allows_wrap && !has_non_space,
                trailing_space_w: if (allows_wrap && ends_with_space) space_w else 0.0,
                text_slice: seg,
                has_tabs: has_tabs,
            }) catch {};
            prev_ws.* = false;
            return;
        }

        // Collapsible whitespace (normal, nowrap, pre-line)
        let tw: f32 = 0.0;
        let has_non_ws = false;
        let has_lead_sp = false;
        let has_trail_sp = false;
        let trail_sp_w: f32 = 0.0;
        let in_ws = prev_ws.*;
        let ti: usize = 0;
        while (ti < seg.len) {
            const b0 = seg[ti];
            if (b0 >= 0x80) {
                const cp = decodeUtf8Next(seg, &ti);
                if (isBidiFormatControl(cp)) continue;
                has_non_ws = true;
                has_trail_sp = false;
                trail_sp_w = 0.0;
                in_ws = false;
                tw += LayoutTree.charWidthForStyle('M', st);
                continue;
            }
            ti += 1;
            const ch = b0;
            const is_ws = (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
            if (is_ws) {
                if (!in_ws) {
                    tw += space_w;
                    if (!has_non_ws) {
                        has_lead_sp = true;
                    }
                    has_trail_sp = true;
                    trail_sp_w = space_w;
                    in_ws = true;
                }
            } else {
                has_non_ws = true;
                has_trail_sp = false;
                trail_sp_w = 0.0;
                in_ws = false;
                tw += LayoutTree.charWidthForStyle(ch, st);
            }
        }
        prev_ws.* = in_ws;
        if (tw <= 0.0) return;

        const trimmed = std.mem.trim(u8, seg, " \t\r\n");
        const wlines = if (allows_wrap && content_w > 0.0 && has_non_ws) LayoutTree.estimateTextWrappedLines(trimmed, content_w, base_sz) else @as(usize, 1);
        items.append(_.allocator, InlineItem {
            kind: InlineItemKind.Text,
            node_id: node_id,
            style_node_id: style_parent_id,
            width: tw,
            height: st.naturalLineHeight(),
            wrapped_lines: wlines,
            has_leading_space: has_lead_sp,
            has_trailing_space: has_trail_sp,
            is_space_only: !has_non_ws,
            trailing_space_w: trail_sp_w,
        }) catch {};
    }

    fn collectInlineItems(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        node_id: NodeId,
        style_parent_id: NodeId,
        content_x: f32,
        cursor_y: f32,
        content_w: f32,
        content_h: f32,
        child_h_definite: bool,
        items: *std.ArrayListUnmanaged(InlineItem),
        wrappers: *std.ArrayListUnmanaged(NodeId),
        prev_ws: *bool,
    ) {
        if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return;
        const node = &doc.nodes.items[@as(usize, node_id)];
        if (node.node_type == NodeType.Text) {
            const st = &styles[@as(usize, style_parent_id)];
            if (st.white_space.preservesNewlines()) {
                let start: usize = 0;
                let ti: usize = 0;
                while (ti < node.text_content.len) : (ti += 1) {
                    if (node.text_content[ti] == '\n') {
                        _.appendTextSegment(node.text_content[start..ti], node_id, style_parent_id, st, content_w, items, prev_ws);
                        items.append(_.allocator, InlineItem {
                            kind: InlineItemKind.Break,
                            node_id: node_id,
                            style_node_id: style_parent_id,
                        }) catch {};
                        prev_ws.* = true;
                        start = ti + 1;
                    }
                }
                if (start < node.text_content.len) {
                    _.appendTextSegment(node.text_content[start..], node_id, style_parent_id, st, content_w, items, prev_ws);
                }
            } else {
                _.appendTextSegment(node.text_content, node_id, style_parent_id, st, content_w, items, prev_ws);
            }
            return;
        }
        if (node.node_type != NodeType.Element) return;
        const cstyle = &styles[@as(usize, node_id)];
        if (cstyle.display.isNone()) {
            _.boxes.items[@as(usize, node_id)].rect = Rect.zero();
            return;
        }
        if (cstyle.position == CssPosition.Absolute || cstyle.position == CssPosition.Fixed) {
            items.append(_.allocator, InlineItem {
                kind: InlineItemKind.AbsPos,
                node_id: node_id,
                style_node_id: node_id,
            }) catch {};
            return;
        }
        if (cstyle.float_mode != FloatMode.None) {
            items.append(_.allocator, InlineItem {
                kind: InlineItemKind.FloatItem,
                node_id: node_id,
                style_node_id: node_id,
            }) catch {};
            return;
        }
        if (node.hasTag("br")) {
            items.append(_.allocator, InlineItem {
                kind: InlineItemKind.Break,
                node_id: node_id,
                style_node_id: style_parent_id,
            }) catch {};
            prev_ws.* = true;
            return;
        }
        if (LayoutTree.isAtomicInline(node, cstyle)) {
            _.layoutNodeEx(doc, styles, node_id, content_x, cursor_y, content_w, content_h, content_x, cursor_y, child_h_definite);
            const cbox = &_.boxes.items[@as(usize, node_id)];
            const is_rep = node.hasTag("svg") || LayoutTree.getReplacedSize(node) != null;
            items.append(_.allocator, InlineItem {
                kind: InlineItemKind.Atomic,
                node_id: node_id,
                style_node_id: node_id,
                parent_style_id: style_parent_id,
                width: cbox.outerWidth(),
                height: cbox.outerHeight(),
                orig_border_x: content_x + cbox.margin_left,
                orig_border_y: cursor_y + cbox.margin_top,
                is_replaced: is_rep,
            }) catch {};
            prev_ws.* = false;
            return;
        }
        // Non-replaced display: inline wrapper
        wrappers.append(_.allocator, node_id) catch {};
        if (cstyle.has_before_content && cstyle.before_content_len > 0) {
            _.appendTextSegment(cstyle.before_content_buf[0..cstyle.before_content_len], node_id, node_id, cstyle, content_w, items, prev_ws);
        }
        let ch = node.first_child;
        while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
            _.collectInlineItems(doc, styles, ch, node_id, content_x, cursor_y, content_w, content_h, child_h_definite, items, wrappers, prev_ws);
            ch = doc.nodes.items[@as(usize, ch)].next_sibling;
        }
        if (cstyle.has_after_content && cstyle.after_content_len > 0) {
            _.appendTextSegment(cstyle.after_content_buf[0..cstyle.after_content_len], node_id, node_id, cstyle, content_w, items, prev_ws);
        }
    }

    fn computeVerticalAlignShift(
        cstyle: *const ComputedStyle,
        pstyle: *const ComputedStyle,
        outer_h: f32,
        strut_ascent: f32,
        strut_descent: f32,
    ): f32 {
        return switch (cstyle.vertical_align_kind) {
            .Baseline => 0.0,
            .Length => cstyle.vertical_align_length.resolvePx(0.0, cstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height),
            .Percent => cstyle.vertical_align_length.resolvePx(cstyle.resolvedLineHeight(), cstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height),
            .Super => (if (cstyle.font_size_px > 0.0) cstyle.font_size_px else 16.0) * 0.3835,
            .Sub => 0.0 - (if (cstyle.font_size_px > 0.0) cstyle.font_size_px else 16.0) * 0.25,
            .Middle => ((if (pstyle.font_size_px > 0.0) pstyle.font_size_px else 16.0) * pstyle.xHeightRatio() * 0.5) - (outer_h * 0.5),
            .TextTop => strut_ascent - outer_h,
            .TextBottom => 0.0 - strut_descent,
            * => 0.0,
        };
    }

    fn flushInlineLine(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        pstyle: *const ComputedStyle,
        line_items: []InlineItem,
        content_x: f32,
        content_w: f32,
        content_h: f32,
        line_y: f32,
        child_h_definite: bool,
        has_inflow: bool,
        float_left_bottom: *f32,
        float_right_bottom: *f32,
        max_child_right: *f32,
        out_baseline_y: *f32,
        out_lines_used: *u32,
    ): f32 {
        out_lines_used.* = 0;
        if (!has_inflow) {
            // Out-of-flow-only line generates no line box (CSS 2.1 §9.4.2),
            // but still positions abs/fixed placeholders and floats at line_y.
            let i: usize = 0;
            while (i < line_items.len) : (i += 1) {
                const it = &line_items[i];
                if (it.kind == InlineItemKind.AbsPos) {
                    const cst = &styles[@as(usize, it.node_id)];
                    if (cst.position == CssPosition.Fixed) {
                        _.layoutNodeEx(doc, styles, it.node_id, 0.0, 0.0, _.viewport_width, _.viewport_height, content_x + it.line_x, line_y, true);
                    } else {
                        _.layoutNodeEx(doc, styles, it.node_id, _.abs_cb_x, _.abs_cb_y, _.abs_cb_w, _.abs_cb_h, content_x + it.line_x, line_y, true);
                    }
                } else if (it.kind == InlineItemKind.FloatItem) {
                    const cst = &styles[@as(usize, it.node_id)];
                    _.layoutNodeEx(doc, styles, it.node_id, content_x, line_y, content_w, content_h, content_x, line_y, child_h_definite);
                    const cbox = &_.boxes.items[@as(usize, it.node_id)];
                    if (cst.float_mode == FloatMode.Right) {
                        const target_x = content_x + content_w - cbox.borderBoxWidth() - cbox.margin_right;
                        const dx = target_x - (content_x + cbox.margin_left);
                        if (dx != 0.0) _.shiftSubtree(doc, it.node_id, dx, 0.0);
                        const fb = line_y + cbox.outerHeight();
                        if (fb > float_right_bottom.*) float_right_bottom.* = fb;
                    } else {
                        const fb = line_y + cbox.outerHeight();
                        if (fb > float_left_bottom.*) float_left_bottom.* = fb;
                    }
                    const cr = _.boxes.items[@as(usize, it.node_id)].rect.right();
                    if (cr > max_child_right.*) max_child_right.* = cr;
                }
            }
            return 0.0;
        }

        const strut_ascent = pstyle.naturalAscent();
        const strut_descent = pstyle.naturalDescent();
        const strut_nat = strut_ascent + strut_descent;
        const strut_lh = pstyle.resolvedLineHeight();
        const half_leading = (strut_lh - strut_nat) * 0.5;
        const strut_above = strut_ascent + half_leading;
        const strut_below = strut_descent + half_leading;

        let min_y: f32 = 0.0 - strut_above;
        let max_y: f32 = strut_below;
        let max_tb_h: f32 = 0.0;
        let max_wrapped_lines: usize = 1;

        let i: usize = 0;
        while (i < line_items.len) : (i += 1) {
            const it = &line_items[i];
            if (it.kind == InlineItemKind.Text) {
                const tst = &styles[@as(usize, it.style_node_id)];
                const t_lh = tst.resolvedLineHeight();
                if (tst.font_size_px > pstyle.font_size_px || t_lh > strut_lh) {
                    const t_asc = tst.naturalAscent();
                    const t_desc = tst.naturalDescent();
                    const t_nat = t_asc + t_desc;
                    const t_hl = (t_lh - t_nat) * 0.5;
                    const t_above = t_asc + t_hl;
                    const t_below = t_desc + t_hl;
                    if (0.0 - t_above < min_y) min_y = 0.0 - t_above;
                    if (t_below > max_y) max_y = t_below;
                }
                if (it.wrapped_lines > max_wrapped_lines) max_wrapped_lines = it.wrapped_lines;
            } else if (it.kind == InlineItemKind.Atomic) {
                const cst = &styles[@as(usize, it.node_id)];
                const cbox = &_.boxes.items[@as(usize, it.node_id)];
                const outer_h = it.height;
                if (cst.vertical_align_kind == VerticalAlignKind.Top || cst.vertical_align_kind == VerticalAlignKind.Bottom) {
                    if (outer_h > max_tb_h) max_tb_h = outer_h;
                } else {
                    const atomic_pstyle = if (it.parent_style_id != NULL_NODE && @as(usize, it.parent_style_id) < styles.len) &styles[@as(usize, it.parent_style_id)] else pstyle;
                    const shift = _.computeVerticalAlignShift(cst, atomic_pstyle, outer_h, strut_ascent, strut_descent);
                    let item_above = outer_h;
                    let item_below: f32 = 0.0;
                    if (cbox.baseline_offset >= 0.0 && cst.overflow_visible && !it.is_replaced) {
                        item_above = cbox.margin_top + cbox.baseline_offset;
                        item_below = outer_h - item_above;
                    }
                    const iy_top = 0.0 - (item_above + shift);
                    const iy_bottom = item_below - shift;
                    if (iy_top < min_y) min_y = iy_top;
                    if (iy_bottom > max_y) max_y = iy_bottom;
                }
            }
        }

        const base_line_h = max_y - min_y;
        let single_line_h = @max(base_line_h, max_tb_h);
        if (single_line_h < 0.0) single_line_h = 0.0;
        const baseline_y = 0.0 - min_y;
        out_baseline_y.* = baseline_y;

        out_lines_used.* = @intCast(max_wrapped_lines);
        const total_line_h = if (max_wrapped_lines > 1) (@as(f32, @floatFromInt(max_wrapped_lines)) * single_line_h) else single_line_h;

        // CssPosition every item on this line
        i = 0;
        while (i < line_items.len) : (i += 1) {
            const it = &line_items[i];
            if (it.kind == InlineItemKind.Text) {
                const tst = &styles[@as(usize, it.style_node_id)];
                const t_asc = tst.naturalAscent();
                const t_nat = tst.naturalLineHeight();
                const text_y = line_y + baseline_y - t_asc;
                const vis = (tst.visibility == Visibility.Visible);
                const new_rect = Rect {
                    x: content_x + it.line_x,
                    y: text_y,
                    width: it.width,
                    height: if (it.wrapped_lines > 1) (@as(f32, @floatFromInt(it.wrapped_lines)) * single_line_h) else t_nat,
                    visible: vis,
                };
                const is_elem_pseudo = (doc.nodes.items[@as(usize, it.node_id)].node_type == NodeType.Element);
                if (is_elem_pseudo && (_.boxes.items[@as(usize, it.node_id)].rect.width > 0.0 || _.boxes.items[@as(usize, it.node_id)].rect.height > 0.0)) {
                    const prev_r = _.boxes.items[@as(usize, it.node_id)].rect;
                    const ux0 = @min(prev_r.x, new_rect.x);
                    const uy0 = @min(prev_r.y, new_rect.y);
                    const ux1 = @max(prev_r.x + prev_r.width, new_rect.x + new_rect.width);
                    const uy1 = @max(prev_r.y + prev_r.height, new_rect.y + new_rect.height);
                    _.boxes.items[@as(usize, it.node_id)].rect = Rect {
                        x: ux0,
                        y: uy0,
                        width: ux1 - ux0,
                        height: uy1 - uy0,
                        visible: vis || prev_r.visible,
                    };
                } else {
                    _.boxes.items[@as(usize, it.node_id)].rect = new_rect;
                }
                const tr = content_x + it.line_x + it.width;
                if (tr > max_child_right.*) max_child_right.* = tr;
            } else if (it.kind == InlineItemKind.Atomic) {
                const cst = &styles[@as(usize, it.node_id)];
                const cbox = &_.boxes.items[@as(usize, it.node_id)];
                const outer_h = it.height;
                let target_outer_y = line_y;
                if (cst.vertical_align_kind == VerticalAlignKind.Top) {
                    target_outer_y = line_y;
                } else if (cst.vertical_align_kind == VerticalAlignKind.Bottom) {
                    target_outer_y = line_y + single_line_h - outer_h;
                } else {
                    const atomic_pstyle = if (it.parent_style_id != NULL_NODE && @as(usize, it.parent_style_id) < styles.len) &styles[@as(usize, it.parent_style_id)] else pstyle;
                    const shift = _.computeVerticalAlignShift(cst, atomic_pstyle, outer_h, strut_ascent, strut_descent);
                    let item_above = outer_h;
                    if (cbox.baseline_offset >= 0.0 && cst.overflow_visible && !it.is_replaced) {
                        item_above = cbox.margin_top + cbox.baseline_offset;
                    }
                    const iy_top = 0.0 - (item_above + shift);
                    target_outer_y = line_y + baseline_y + iy_top;
                }
                const target_border_x = content_x + it.line_x + cbox.margin_left;
                const target_border_y = target_outer_y + cbox.margin_top;
                const dx = target_border_x - it.orig_border_x;
                const dy = target_border_y - it.orig_border_y;
                if (dx != 0.0 || dy != 0.0) {
                    _.shiftSubtree(doc, it.node_id, dx, dy);
                }
                const cr = content_x + it.line_x + it.width;
                if (cr > max_child_right.*) max_child_right.* = cr;
            } else if (it.kind == InlineItemKind.AbsPos) {
                const cst = &styles[@as(usize, it.node_id)];
                if (cst.position == CssPosition.Fixed) {
                    _.layoutNodeEx(doc, styles, it.node_id, 0.0, 0.0, _.viewport_width, _.viewport_height, content_x + it.line_x, line_y, true);
                } else {
                    _.layoutNodeEx(doc, styles, it.node_id, _.abs_cb_x, _.abs_cb_y, _.abs_cb_w, _.abs_cb_h, content_x + it.line_x, line_y, true);
                }
            } else if (it.kind == InlineItemKind.FloatItem) {
                const cst = &styles[@as(usize, it.node_id)];
                _.layoutNodeEx(doc, styles, it.node_id, content_x, line_y, content_w, content_h, content_x, line_y, child_h_definite);
                const cbox = &_.boxes.items[@as(usize, it.node_id)];
                if (cst.float_mode == FloatMode.Right) {
                    const target_x = content_x + content_w - cbox.borderBoxWidth() - cbox.margin_right;
                    const dx = target_x - (content_x + cbox.margin_left);
                    if (dx != 0.0) _.shiftSubtree(doc, it.node_id, dx, 0.0);
                    const fb = line_y + cbox.outerHeight();
                    if (fb > float_right_bottom.*) float_right_bottom.* = fb;
                } else {
                    const fb = line_y + cbox.outerHeight();
                    if (fb > float_left_bottom.*) float_left_bottom.* = fb;
                }
                const cr = _.boxes.items[@as(usize, it.node_id)].rect.right();
                if (cr > max_child_right.*) max_child_right.* = cr;
            }
        }
        return total_line_h;
    }

    fn computeInlineWrapperRect(*_, doc: *const Document, styles: []const ComputedStyle, wrapper_id: NodeId, fallback_x: f32, fallback_y: f32) {
        const wstyle = &styles[@as(usize, wrapper_id)];
        let wbox = &_.boxes.items[@as(usize, wrapper_id)];
        const pl = wstyle.padding_left.resolvePx(0.0, wstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
        const pr = wstyle.padding_right.resolvePx(0.0, wstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
        const pt = wstyle.padding_top.resolvePx(0.0, wstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
        const pb = wstyle.padding_bottom.resolvePx(0.0, wstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
        wbox.padding_left = pl;
        wbox.padding_right = pr;
        wbox.padding_top = pt;
        wbox.padding_bottom = pb;
        wbox.border_left = wstyle.border_left_width;
        wbox.border_right = wstyle.border_right_width;
        wbox.border_top = wstyle.border_top_width;
        wbox.border_bottom = wstyle.border_bottom_width;

        let has_any = false;
        let min_x: f32 = 0.0;
        let min_y: f32 = 0.0;
        let max_x: f32 = 0.0;
        let max_y: f32 = 0.0;

        if ((wstyle.has_before_content && wstyle.before_content_len > 0) || (wstyle.has_after_content && wstyle.after_content_len > 0)) {
            if (wbox.rect.width > 0.0 || wbox.rect.height > 0.0) {
                has_any = true;
                min_x = wbox.rect.x;
                min_y = wbox.rect.y;
                max_x = wbox.rect.x + wbox.rect.width;
                max_y = wbox.rect.y + wbox.rect.height;
            }
        }

        let ch = doc.nodes.items[@as(usize, wrapper_id)].first_child;
        while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
            const cn = &doc.nodes.items[@as(usize, ch)];
            let include = false;
            if (cn.node_type == NodeType.Text) {
                const tr = _.boxes.items[@as(usize, ch)].rect;
                if (tr.width > 0.0 || tr.height > 0.0) include = true;
            } else if (cn.node_type == NodeType.Element) {
                const cst = &styles[@as(usize, ch)];
                if (!cst.display.isNone() && cst.position != CssPosition.Absolute && cst.position != CssPosition.Fixed && cst.float_mode == FloatMode.None) {
                    const cr = _.boxes.items[@as(usize, ch)].rect;
                    if (cr.width > 0.0 || cr.height > 0.0) include = true;
                }
            }
            if (include) {
                const cr = _.boxes.items[@as(usize, ch)].rect;
                if (!has_any) {
                    min_x = cr.x;
                    min_y = cr.y;
                    max_x = cr.x + cr.width;
                    max_y = cr.y + cr.height;
                    has_any = true;
                } else {
                    if (cr.x < min_x) min_x = cr.x;
                    if (cr.y < min_y) min_y = cr.y;
                    if (cr.x + cr.width > max_x) max_x = cr.x + cr.width;
                    if (cr.y + cr.height > max_y) max_y = cr.y + cr.height;
                }
            }
            ch = cn.next_sibling;
        }

        const is_vis = (wstyle.visibility == Visibility.Visible);
        if (!has_any) {
            _.boxes.items[@as(usize, wrapper_id)].content_width = 0.0;
            _.boxes.items[@as(usize, wrapper_id)].content_height = 0.0;
            _.boxes.items[@as(usize, wrapper_id)].rect = Rect {
                x: fallback_x,
                y: fallback_y,
                width: 0.0,
                height: 0.0,
                visible: is_vis,
            };
        } else {
            const cw = max_x - min_x;
            const ch_val = max_y - min_y;
            _.boxes.items[@as(usize, wrapper_id)].content_width = cw;
            _.boxes.items[@as(usize, wrapper_id)].content_height = ch_val;
            _.boxes.items[@as(usize, wrapper_id)].rect = Rect {
                x: min_x - pl - wstyle.border_left_width,
                y: min_y - pt - wstyle.border_top_width,
                width: cw + pl + pr + wstyle.border_left_width + wstyle.border_right_width,
                height: ch_val + pt + pb + wstyle.border_top_width + wstyle.border_bottom_width,
                visible: is_vis,
            };
        }
    }

    /// Block formatting layout: stacks block children vertically and lays out inline runs via IFC.
    fn layoutBlockContainer(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        parent_id: NodeId,
        content_x: f32,
        content_y: f32,
    ) {
        const parent_box = &_.boxes.items[@as(usize, parent_id)];
        const content_w = parent_box.content_width;
        const content_h = parent_box.content_height;
        const parent_border_top_y = content_y - parent_box.padding_top - parent_box.border_top;
        let cursor_y = content_y;
        let max_child_right = content_x;
        let float_left_bottom = content_y;
        let float_right_bottom = content_y;
        let last_baseline_rel: f32 = -1.0;

        const pstyle_blk = &styles[@as(usize, parent_id)];
        let emitted_lines: u32 = 0;
        const clamp_limit: u32 = if (pstyle_blk.is_line_clamped) pstyle_blk.webkit_line_clamp else 0;
        const parent_pinned_v = (pstyle_blk.position == CssPosition.Absolute || pstyle_blk.position == CssPosition.Fixed) &&
            !pstyle_blk.top.is_auto && !pstyle_blk.top.is_none &&
            !pstyle_blk.bottom.is_auto && !pstyle_blk.bottom.is_none;
        const child_h_definite = !parent_box.height_is_auto || parent_pinned_v;
        let prev_block_mb: f32 = 0.0;
        let has_prev_block_mb: bool = false;

        let child = doc.nodes.items[@as(usize, parent_id)].first_child;
        while (child != NULL_NODE && @as(usize, child) < doc.nodes.items.len) {
            const cnode = &doc.nodes.items[@as(usize, child)];
            if (cnode.node_type == NodeType.Element) {
                const cstyle = &styles[@as(usize, child)];
                if (cstyle.display.isNone()) {
                    _.boxes.items[@as(usize, child)].rect = Rect.zero();
                    child = cnode.next_sibling;
                    continue;
                }
                if (LayoutTree.isBlockLevelInFlow(cnode, cstyle)) {
                    if (cstyle.display.isTableInternal()) {
                        let run: [64]NodeId = [NULL_NODE; 64];
                        let run_count: usize = 0;
                        let scan = child;
                        let last_consumed = child;
                        while (scan != NULL_NODE && @as(usize, scan) < doc.nodes.items.len) {
                            const sn = &doc.nodes.items[@as(usize, scan)];
                            if (sn.node_type == NodeType.Element) {
                                const sst = &styles[@as(usize, scan)];
                                if (sst.display.isNone()) {
                                    _.boxes.items[@as(usize, scan)].rect = Rect.zero();
                                    last_consumed = scan;
                                    scan = sn.next_sibling;
                                    continue;
                                }
                                if (!sst.display.isTableInternal()) break;
                                if (run_count < 64) {
                                    run[run_count] = scan;
                                    run_count += 1;
                                }
                                last_consumed = scan;
                            } else if (sn.node_type == NodeType.Text) {
                                const trimmed = std.mem.trim(u8, sn.text_content, " \t\r\n");
                                if (trimmed.len > 0) break;
                            }
                            scan = sn.next_sibling;
                        }
                        const dims = _.layoutTableCore(doc, styles, NULL_NODE, null, run[0..run_count], content_x, cursor_y, content_w);
                        cursor_y += dims[1];
                        if (content_x + dims[0] > max_child_right) {
                            max_child_right = content_x + dims[0];
                        }
                        child = doc.nodes.items[@as(usize, last_consumed)].next_sibling;
                        continue;
                    }

                    // Handle CSS clear
                    if (cstyle.clear_mode == ClearMode.Left || cstyle.clear_mode == ClearMode.Both) {
                        if (float_left_bottom > cursor_y) { cursor_y = float_left_bottom; has_prev_block_mb = false; }
                    }
                    if (cstyle.clear_mode == ClearMode.Right || cstyle.clear_mode == ClearMode.Both) {
                        if (float_right_bottom > cursor_y) { cursor_y = float_right_bottom; has_prev_block_mb = false; }
                    }

                    _.layoutNodeEx(doc, styles, child, content_x, cursor_y, content_w, content_h, content_x, cursor_y, child_h_definite);
                    const cbox = &_.boxes.items[@as(usize, child)];
                    if (has_prev_block_mb && prev_block_mb > 0.0 && cbox.margin_top > 0.0) {
                        const overlap = @min(prev_block_mb, cbox.margin_top);
                        if (overlap > 0.0) {
                            _.shiftSubtree(doc, child, 0.0, -overlap);
                            cursor_y -= overlap;
                        }
                    }

                    if (cstyle.margin_left.is_auto && cstyle.margin_right.is_auto) {
                        const bw = cbox.borderBoxWidth();
                        if (content_w > bw) {
                            const target_x = content_x + (content_w - bw) * 0.5;
                            const dx = target_x - (content_x + cbox.margin_left);
                            if (dx != 0.0) {
                                _.shiftSubtree(doc, child, dx, 0.0);
                            }
                        }
                    }
                    if (cbox.baseline_offset >= 0.0) {
                        last_baseline_rel = (cursor_y + cbox.margin_top + cbox.baseline_offset) - parent_border_top_y;
                    }
                    cursor_y += cbox.outerHeight();
                    prev_block_mb = cbox.margin_bottom;
                    has_prev_block_mb = true;
                    const cr = _.boxes.items[@as(usize, child)].rect.right();
                    if (cr > max_child_right) {
                        max_child_right = cr;
                    }
                    child = cnode.next_sibling;
                    continue;
                }
            }

            // Collect contiguous inline / float / out-of-flow run until next block-level in-flow element
            let items: std.ArrayListUnmanaged(InlineItem) = std.ArrayListUnmanaged(InlineItem).empty;
            defer items.deinit(_.allocator);
            let wrappers: std.ArrayListUnmanaged(NodeId) = std.ArrayListUnmanaged(NodeId).empty;
            defer wrappers.deinit(_.allocator);

            const run_start_y = cursor_y;
            let prev_ws = true;
            let scan = child;
            while (scan != NULL_NODE && @as(usize, scan) < doc.nodes.items.len) {
                const sn = &doc.nodes.items[@as(usize, scan)];
                if (sn.node_type == NodeType.Element) {
                    const sst = &styles[@as(usize, scan)];
                    if (!sst.display.isNone() && LayoutTree.isBlockLevelInFlow(sn, sst)) {
                        break;
                    }
                }
                _.collectInlineItems(doc, styles, scan, parent_id, content_x, cursor_y, content_w, content_h, child_h_definite, &items, &wrappers, &prev_ws);
                scan = sn.next_sibling;
            }
            child = scan;

            // Break items into lines and flush each line
            let line_start: usize = 0;
            let line_x: f32 = 0.0;
            if (doc.nodes.items[@as(usize, parent_id)].hasTag("li") && pstyle_blk.list_style_position == ListStylePosition.Inside && run_start_y == content_y) {
                let mbuf: [32]u8 = undefined;
                const mstr = formatListMarker(doc, styles, parent_id, &mbuf);
                if (mstr.len > 0) {
                    let mbuf_sp: [33]u8 = undefined;
                    std.mem.copyForwards(u8, mbuf_sp[0..mstr.len], mstr);
                    mbuf_sp[mstr.len] = ' ';
                    line_x = LayoutTree.measureTextSegmentWithTabs(mbuf_sp[0 .. mstr.len + 1], pstyle_blk, 0.0);
                }
            }
            let line_has_inflow = false;
            let last_break_idx: usize = 0;
            let has_break_opp = false;
            let idx: usize = 0;
            while (idx < items.items.len) {
                let it = &items.items[idx];
                if (it.kind == InlineItemKind.Break) {
                    // Strip trailing collapsible space before forced break
                    if (idx > line_start) {
                        let prev_it = &items.items[idx - 1];
                        if (prev_it.kind == InlineItemKind.Text && prev_it.has_trailing_space && prev_it.trailing_space_w > 0.0) {
                            prev_it.width = @max(0.0, prev_it.width - prev_it.trailing_space_w);
                            prev_it.trailing_space_w = 0.0;
                        }
                    }
                    let bl: f32 = 0.0;
                    let used_l: u32 = 0;
                    const lh = _.flushInlineLine(doc, styles, pstyle_blk, items.items[line_start .. idx + 1], content_x, content_w, content_h, cursor_y, child_h_definite, true, &float_left_bottom, &float_right_bottom, &max_child_right, &bl, &used_l);
                    if (clamp_limit == 0 || emitted_lines < clamp_limit) {
                        let eff_lh = lh;
                        if (clamp_limit > 0 && emitted_lines + used_l > clamp_limit && used_l > 0) {
                            const rem = clamp_limit - emitted_lines;
                            eff_lh = (lh / @as(f32, @floatFromInt(used_l))) * @as(f32, @floatFromInt(rem));
                        }
                        last_baseline_rel = (cursor_y + bl) - parent_border_top_y;
                        cursor_y += eff_lh;
                    }
                    emitted_lines += used_l;
                    line_start = idx + 1;
                    line_x = 0.0;
                    line_has_inflow = false;
                    has_break_opp = false;
                    idx += 1;
                } else if (it.kind == InlineItemKind.AbsPos || it.kind == InlineItemKind.FloatItem) {
                    it.line_x = line_x;
                    idx += 1;
                } else {
                    if (it.kind == InlineItemKind.Text && it.has_tabs && it.text_slice.len > 0) {
                        it.width = LayoutTree.measureTextSegmentWithTabs(it.text_slice, &styles[@as(usize, it.style_node_id)], line_x);
                    }
                    const allows_wrap = pstyle_blk.white_space.allowsWrap();
                    if (allows_wrap && line_has_inflow && content_w > 0.0 && line_x > 0.0 && !it.is_space_only && line_x + it.width > content_w) {
                        let break_at = idx;
                        if (has_break_opp && last_break_idx > line_start && last_break_idx <= idx) {
                            break_at = last_break_idx;
                        }
                        // Strip trailing space on the line being flushed
                        if (break_at > line_start) {
                            let prev_it = &items.items[break_at - 1];
                            if (prev_it.kind == InlineItemKind.Text && prev_it.has_trailing_space && prev_it.trailing_space_w > 0.0) {
                                prev_it.width = @max(0.0, prev_it.width - prev_it.trailing_space_w);
                                prev_it.trailing_space_w = 0.0;
                            }
                        }
                        let bl: f32 = 0.0;
                        let used_l: u32 = 0;
                        const lh = _.flushInlineLine(doc, styles, pstyle_blk, items.items[line_start..break_at], content_x, content_w, content_h, cursor_y, child_h_definite, true, &float_left_bottom, &float_right_bottom, &max_child_right, &bl, &used_l);
                        if (clamp_limit == 0 || emitted_lines < clamp_limit) {
                            let eff_lh = lh;
                            if (clamp_limit > 0 && emitted_lines + used_l > clamp_limit && used_l > 0) {
                                const rem = clamp_limit - emitted_lines;
                                eff_lh = (lh / @as(f32, @floatFromInt(used_l))) * @as(f32, @floatFromInt(rem));
                            }
                            last_baseline_rel = (cursor_y + bl) - parent_border_top_y;
                            cursor_y += eff_lh;
                        }
                        emitted_lines += used_l;
                        line_start = break_at;
                        // Skip any space-only item at the start of the new line
                        while (line_start < items.items.len && items.items[line_start].kind == InlineItemKind.Text && items.items[line_start].is_space_only) {
                            items.items[line_start].width = 0.0;
                            items.items[line_start].line_x = 0.0;
                            line_start += 1;
                        }
                        line_x = 0.0;
                        line_has_inflow = false;
                        has_break_opp = false;
                        idx = line_start;
                        continue;
                    }
                    // Strip leading space-only item at start of a line
                    if (!line_has_inflow && it.kind == InlineItemKind.Text && it.is_space_only) {
                        it.width = 0.0;
                        it.line_x = 0.0;
                        idx += 1;
                        continue;
                    }
                    if (it.kind == InlineItemKind.Text && it.has_leading_space && line_has_inflow) {
                        last_break_idx = idx;
                        has_break_opp = true;
                    }
                    it.line_x = line_x;
                    line_x += it.width;
                    line_has_inflow = true;
                    if (it.kind == InlineItemKind.Text && (it.is_space_only || it.has_trailing_space)) {
                        last_break_idx = idx + 1;
                        has_break_opp = true;
                    }
                    idx += 1;
                }
            }
            if (line_start < items.items.len) {
                // Strip trailing collapsible space at end of last line in run
                let end_i = items.items.len;
                while (end_i > line_start) {
                    const last_it = &items.items[end_i - 1];
                    if (last_it.kind == InlineItemKind.Text && last_it.is_space_only) {
                        items.items[end_i - 1].width = 0.0;
                        end_i -= 1;
                    } else if (last_it.kind == InlineItemKind.Text && last_it.has_trailing_space && last_it.trailing_space_w > 0.0) {
                        items.items[end_i - 1].width = @max(0.0, last_it.width - last_it.trailing_space_w);
                        items.items[end_i - 1].trailing_space_w = 0.0;
                        break;
                    } else {
                        break;
                    }
                }
                let bl: f32 = 0.0;
                let used_l: u32 = 0;
                const lh = _.flushInlineLine(doc, styles, pstyle_blk, items.items[line_start..items.items.len], content_x, content_w, content_h, cursor_y, child_h_definite, line_has_inflow, &float_left_bottom, &float_right_bottom, &max_child_right, &bl, &used_l);
                if (line_has_inflow) {
                    if (clamp_limit == 0 || emitted_lines < clamp_limit) {
                        let eff_lh = lh;
                        if (clamp_limit > 0 && emitted_lines + used_l > clamp_limit && used_l > 0) {
                            const rem = clamp_limit - emitted_lines;
                            eff_lh = (lh / @as(f32, @floatFromInt(used_l))) * @as(f32, @floatFromInt(rem));
                        }
                        last_baseline_rel = (cursor_y + bl) - parent_border_top_y;
                        cursor_y += eff_lh;
                    }
                    emitted_lines += used_l;
                }
            }
            if (cursor_y > run_start_y) {
                has_prev_block_mb = false;
            }

            // Compute bounding rects for inline wrappers bottom-up (deepest first)
            if (wrappers.items.len > 0) {
                let wi: usize = wrappers.items.len;
                while (wi > 0) {
                    wi -= 1;
                    _.computeInlineWrapperRect(doc, styles, wrappers.items[wi], content_x, run_start_y);
                }
            }
        }

        if (last_baseline_rel >= 0.0) {
            _.boxes.items[@as(usize, parent_id)].baseline_offset = last_baseline_rel;
        }

        const establishes_bfc = pstyle_blk.display.isFlowRoot() ||
            !pstyle_blk.overflow_visible ||
            pstyle_blk.display.isInlineBlock() ||
            pstyle_blk.float_mode != FloatMode.None ||
            pstyle_blk.position == CssPosition.Absolute ||
            pstyle_blk.position == CssPosition.Fixed ||
            pstyle_blk.display.isTableCell() ||
            pstyle_blk.display.isTableCaption();
        const scroll_bottom = @max(cursor_y, @max(float_left_bottom, float_right_bottom));
        const effective_bottom = if (establishes_bfc) scroll_bottom else cursor_y;
        const scroll_h = if (scroll_bottom > content_y) (scroll_bottom - content_y) else 0.0;
        const auto_h = if (effective_bottom > content_y) (effective_bottom - content_y) else 0.0;
        const auto_w = if (max_child_right > content_x) (max_child_right - content_x) else 0.0;
        _.boxes.items[@as(usize, parent_id)].scroll_content_height = @max(_.boxes.items[@as(usize, parent_id)].content_height, scroll_h);
        _.boxes.items[@as(usize, parent_id)].scroll_content_width = @max(_.boxes.items[@as(usize, parent_id)].content_width, auto_w);

        // Shrink-to-fit auto width for inline-block, inline, float, or abspos without left+right
        if (pstyle_blk.width.is_auto && (pstyle_blk.display.isInlineBlock() || pstyle_blk.display.isInline() || pstyle_blk.float_mode != FloatMode.None)) {
            if (auto_w > _.boxes.items[@as(usize, parent_id)].content_width) {
                _.boxes.items[@as(usize, parent_id)].content_width = auto_w;
            }
        }

        // If parent has auto height, expand content_height to fit child stream
        if (_.boxes.items[@as(usize, parent_id)].height_is_auto && !parent_pinned_v) {
            let is_p_rep = false;
            if (parent_id != NULL_NODE && @as(usize, parent_id) < doc.nodes.items.len) {
                const pnode = &doc.nodes.items[@as(usize, parent_id)];
                if (pnode.isElement() && LayoutTree.getReplacedSize(pnode) != null) {
                    is_p_rep = true;
                }
            }
            if (!is_p_rep) {
                _.boxes.items[@as(usize, parent_id)].content_height = auto_h;
            }
        }
    }

    /// Grid layout container: places grid items into columns and rows.
    fn layoutGridContainer(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        parent_id: NodeId,
        content_x: f32,
        content_y: f32,
    ) {
        const pstyle = &styles[@as(usize, parent_id)];
        const pbox = &_.boxes.items[@as(usize, parent_id)];
        const container_w = pbox.content_width;
        const container_h = pbox.content_height;

        const cols = if (pstyle.grid_cols > 0) pstyle.grid_cols else 1;
        const col_gap = pstyle.column_gap;
        const row_gap = pstyle.row_gap;

        // Resolve column track sizes and cumulative line offsets (1-based line k => col_offsets[k-1])
        let col_widths: [16]f32 = [0.0; 16];
        let col_offsets: [17]f32 = [0.0; 17];
        const eff_cols: usize = @min(@as(usize, cols), 16);
        const total_col_gaps = if (eff_cols > 1) @as(f32, @floatFromInt(eff_cols - 1)) * col_gap else 0.0;
        const default_col_w = if (container_w > total_col_gaps) (container_w - total_col_gaps) / @as(f32, @floatFromInt(eff_cols)) else 0.0;

        if (pstyle.grid_col_track_count > 0) {
            let fixed_sum: f32 = 0.0;
            let flex_count: u32 = 0;
            let ci: usize = 0;
            while (ci < eff_cols) : (ci += 1) {
                if (ci < @as(usize, pstyle.grid_col_track_count)) {
                    const tr = pstyle.grid_col_tracks[ci];
                    if (!tr.is_auto && !tr.is_none) {
                        const w = tr.resolvePx(container_w, pstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                        col_widths[ci] = w;
                        fixed_sum += w;
                    } else {
                        flex_count += 1;
                    }
                } else {
                    flex_count += 1;
                }
            }
            const rem_w = if (container_w > fixed_sum + total_col_gaps) (container_w - fixed_sum - total_col_gaps) else 0.0;
            const flex_w = if (flex_count > 0) rem_w / @as(f32, @floatFromInt(flex_count)) else 0.0;
            ci = 0;
            while (ci < eff_cols) : (ci += 1) {
                if (ci < @as(usize, pstyle.grid_col_track_count)) {
                    const tr = pstyle.grid_col_tracks[ci];
                    if (tr.is_auto || tr.is_none) {
                        col_widths[ci] = flex_w;
                    }
                } else {
                    col_widths[ci] = flex_w;
                }
            }
        } else {
            let ci: usize = 0;
            while (ci < eff_cols) : (ci += 1) {
                col_widths[ci] = default_col_w;
            }
        }

        col_offsets[0] = 0.0;
        let oi: usize = 0;
        while (oi < eff_cols) : (oi += 1) {
            col_offsets[oi + 1] = col_offsets[oi] + col_widths[oi] + (if (oi + 1 < eff_cols) col_gap else 0.0);
        }

        // Resolve explicit row track sizes if declared
        let row_heights: [16]f32 = [0.0; 16];
        let row_offsets: [17]f32 = [0.0; 17];
        const explicit_rows: usize = @min(@as(usize, pstyle.grid_row_track_count), 16);
        let ri: usize = 0;
        while (ri < explicit_rows) : (ri += 1) {
            const tr = pstyle.grid_row_tracks[ri];
            if (!tr.is_auto && !tr.is_none) {
                row_heights[ri] = tr.resolvePx(container_h, pstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            }
        }

        let cur_col: u32 = 0;
        let cur_y: f32 = content_y;
        let cur_row_max_h: f32 = 0.0;
        let row_count: u32 = 0;

        let abs_children: [32]NodeId = [NULL_NODE; 32];
        let abs_count: usize = 0;

        let child = doc.nodes.items[@as(usize, parent_id)].first_child;
        while (child != NULL_NODE) {
            const cnode = &doc.nodes.items[@as(usize, child)];
            if (cnode.node_type == NodeType.Element) {
                const cstyle = &styles[@as(usize, child)];
                if (cstyle.display.isNone()) {
                    _.boxes.items[@as(usize, child)].rect = Rect.zero();
                } else if (cstyle.position == CssPosition.Absolute || cstyle.position == CssPosition.Fixed) {
                    if (abs_count < 32) {
                        abs_children[abs_count] = child;
                        abs_count += 1;
                    }
                } else {
                    const span_raw = if (cstyle.grid_col_span > 0) cstyle.grid_col_span else 1;
                    const span = if (span_raw > cols) cols else span_raw;

                    // If item does not fit in current row, advance to next row
                    if (cur_col + span > cols && cur_col > 0) {
                        if (@as(usize, row_count) < 16 && cur_row_max_h > row_heights[@as(usize, row_count)]) {
                            row_heights[@as(usize, row_count)] = cur_row_max_h;
                        }
                        const adv_h = if (@as(usize, row_count) < 16) row_heights[@as(usize, row_count)] else cur_row_max_h;
                        cur_y += adv_h + row_gap;
                        cur_col = 0;
                        cur_row_max_h = 0.0;
                        row_count += 1;
                    }

                    const col_idx: usize = @min(@as(usize, cur_col), eff_cols - 1);
                    const item_x = content_x + col_offsets[col_idx];
                    let item_w: f32 = 0.0;
                    let si: u32 = 0;
                    while (si < span) : (si += 1) {
                        const ti: usize = @min(col_idx + @as(usize, si), eff_cols - 1);
                        item_w += col_widths[ti];
                        if (si + 1 < span) item_w += col_gap;
                    }
                    const item_y = cur_y;
                    const cell_row_h = if (@as(usize, row_count) < explicit_rows && @as(usize, row_count) < 16) row_heights[@as(usize, row_count)] else 0.0;

                    _.layoutNodeEx(doc, styles, child, item_x, item_y, item_w, cell_row_h, item_x, item_y, cell_row_h > 0.0);

                    const cbox = &_.boxes.items[@as(usize, child)];
                    let auto_dx: f32 = 0.0;
                    let auto_dy: f32 = 0.0;
                    if (cstyle.margin_left.is_auto || cstyle.margin_right.is_auto) {
                        const free_w = @max(0.0, item_w - cbox.outerWidth());
                        if (cstyle.margin_left.is_auto && cstyle.margin_right.is_auto) auto_dx = free_w * 0.5;
                        else if (cstyle.margin_left.is_auto) auto_dx = free_w;
                    }
                    if (cell_row_h > 0.0) {
                        if (cstyle.margin_top.is_auto || cstyle.margin_bottom.is_auto) {
                            const free_h = @max(0.0, cell_row_h - cbox.outerHeight());
                            if (cstyle.margin_top.is_auto && cstyle.margin_bottom.is_auto) auto_dy = free_h * 0.5;
                            else if (cstyle.margin_top.is_auto) auto_dy = free_h;
                        } else {
                            const eff_align = pstyle.align_items;
                            const free_h = @max(0.0, cell_row_h - cbox.outerHeight());
                            if (eff_align == AlignItems.Center) {
                                auto_dy = free_h * 0.5;
                            } else if (eff_align == AlignItems.FlexEnd) {
                                auto_dy = free_h;
                            }
                        }
                    }
                    if (auto_dx != 0.0 || auto_dy != 0.0) {
                        _.shiftSubtree(doc, child, auto_dx, auto_dy);
                    }
                    const item_outer_h = _.boxes.items[@as(usize, child)].outerHeight();
                    if (item_outer_h > cur_row_max_h) {
                        cur_row_max_h = item_outer_h;
                    }

                    cur_col += span;
                    if (cur_col >= cols) {
                        if (@as(usize, row_count) < 16 && cur_row_max_h > row_heights[@as(usize, row_count)]) {
                            row_heights[@as(usize, row_count)] = cur_row_max_h;
                        }
                        const adv_h = if (@as(usize, row_count) < 16) row_heights[@as(usize, row_count)] else cur_row_max_h;
                        cur_y += adv_h + row_gap;
                        cur_col = 0;
                        cur_row_max_h = 0.0;
                        row_count += 1;
                    }
                }
            }
            child = cnode.next_sibling;
        }

        // Calculate total grid height
        let total_grid_h: f32 = 0.0;
        if (cur_col > 0) {
            if (@as(usize, row_count) < 16 && cur_row_max_h > row_heights[@as(usize, row_count)]) {
                row_heights[@as(usize, row_count)] = cur_row_max_h;
            }
            const adv_h = if (@as(usize, row_count) < 16) row_heights[@as(usize, row_count)] else cur_row_max_h;
            cur_y += adv_h;
            row_count += 1;
            total_grid_h = cur_y - content_y;
        } else if (row_count > 0) {
            total_grid_h = (cur_y - content_y) - row_gap;
        }

        const total_rows: usize = @min(@max(@as(usize, row_count), explicit_rows), 16);
        row_offsets[0] = 0.0;
        let roi: usize = 0;
        while (roi < total_rows) : (roi += 1) {
            row_offsets[roi + 1] = row_offsets[roi] + row_heights[roi] + (if (roi + 1 < total_rows) row_gap else 0.0);
        }

        if (pstyle.height.is_auto) {
            const explicit_h = if (total_rows > 0) row_offsets[total_rows] else 0.0;
            const final_h = @max(total_grid_h, explicit_h);
            _.boxes.items[@as(usize, parent_id)].content_height = if (final_h > 0.0) final_h else 0.0;
        }

        // Lay out out-of-flow absolute/fixed children per CSS Grid 1 §10.1
        if (abs_count > 0) {
            const pbox_now = &_.boxes.items[@as(usize, parent_id)];
            const pad_x = _.abs_cb_x;
            const pad_y = _.abs_cb_y;
            const pad_w = _.abs_cb_w;
            const pad_h = if (pstyle.position != CssPosition.Static) (pbox_now.content_height + pbox_now.paddingVertical()) else _.abs_cb_h;
            let ai: usize = 0;
            while (ai < abs_count) : (ai += 1) {
                const abs_id = abs_children[ai];
                const astyle = &styles[@as(usize, abs_id)];
                if (astyle.position == CssPosition.Fixed) {
                    _.layoutNodeEx(doc, styles, abs_id, 0.0, 0.0, _.viewport_width, _.viewport_height, content_x, content_y, true);
                } else {
                    let cb_left = pad_x;
                    let cb_right = pad_x + pad_w;
                    let cb_top = pad_y;
                    let cb_bottom = pad_y + pad_h;
                    let sx = content_x;
                    let sy = content_y;

                    if (pstyle.position != CssPosition.Static) {
                        if (astyle.grid_col_start > 0) {
                            const line_idx: usize = @min(@as(usize, @intCast(astyle.grid_col_start - 1)), eff_cols);
                            cb_left = content_x + col_offsets[line_idx];
                            sx = cb_left;
                        }
                        if (astyle.grid_col_end > 0) {
                            const line_idx: usize = @min(@as(usize, @intCast(astyle.grid_col_end - 1)), eff_cols);
                            cb_right = content_x + col_offsets[line_idx];
                        }
                        if (astyle.grid_row_start > 0) {
                            const line_idx: usize = @min(@as(usize, @intCast(astyle.grid_row_start - 1)), total_rows);
                            cb_top = content_y + row_offsets[line_idx];
                            sy = cb_top;
                        }
                        if (astyle.grid_row_end > 0) {
                            const line_idx: usize = @min(@as(usize, @intCast(astyle.grid_row_end - 1)), total_rows);
                            cb_bottom = content_y + row_offsets[line_idx];
                        }
                    }

                    const cb_w = if (cb_right > cb_left) (cb_right - cb_left) else 0.0;
                    const cb_h = if (cb_bottom > cb_top) (cb_bottom - cb_top) else 0.0;
                    _.layoutNodeEx(doc, styles, abs_id, cb_left, cb_top, cb_w, cb_h, sx, sy, true);
                }
            }
        }
    }

    fn layoutFlexAbsChild(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        pstyle: *const ComputedStyle,
        is_row: bool,
        aid: NodeId,
        content_x: f32,
        content_y: f32,
        container_w: f32,
        container_h: f32,
    ) {
        const astyle = &styles[@as(usize, aid)];
        if (astyle.position == CssPosition.Fixed) {
            _.layoutNodeEx(doc, styles, aid, 0.0, 0.0, _.viewport_width, _.viewport_height, content_x, content_y, true);
            return;
        }
        _.resolveBoxMetrics(doc, styles, aid, astyle, _.abs_cb_w, _.abs_cb_h, true);
        const abox = &_.boxes.items[@as(usize, aid)];
        const mb_w = abox.borderBoxWidth() + abox.marginHorizontal();
        const mb_h = abox.borderBoxHeight() + abox.marginVertical();
        const is_rev = (pstyle.flex_direction == FlexDirection.RowReverse || pstyle.flex_direction == FlexDirection.ColumnReverse);
        const rem_main = if (is_row) (container_w - mb_w) else (container_h - mb_h);
        const rem_cross = if (is_row) (container_h - mb_h) else (container_w - mb_w);
        const main_off: f32 = switch (pstyle.justify_content) {
            .FlexStart, .SpaceBetween => if (is_rev) rem_main else 0.0,
            .FlexEnd => if (is_rev) 0.0 else rem_main,
            .Center, .SpaceAround, .SpaceEvenly => rem_main / 2.0,
        };
        const cross_off: f32 = switch (pstyle.align_items) {
            .FlexEnd => rem_cross,
            .Center => rem_cross / 2.0,
            * => 0.0,
        };
        const sx = content_x + (if (is_row) main_off else cross_off);
        const sy = content_y + (if (is_row) cross_off else main_off);
        _.layoutNodeEx(doc, styles, aid, _.abs_cb_x, _.abs_cb_y, _.abs_cb_w, _.abs_cb_h, sx, sy, true);
    }

    /// Flexbox layout: implements W3C Flexbox Level 1 main-axis distribution and cross-axis alignment.
    fn layoutFlexContainer(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
        parent_id: NodeId,
        content_x: f32,
        content_y: f32,
    ) {
        const pstyle = &styles[@as(usize, parent_id)];
        const pbox = &_.boxes.items[@as(usize, parent_id)];
        const container_w = pbox.content_width;
        const container_h = pbox.content_height;

        const is_row = pstyle.flex_direction.isRow();
        const is_auto_main = if (is_row) pstyle.width.is_auto else pbox.height_is_auto;
        let main_size = if (is_row) container_w else container_h;
        if (!is_row && is_auto_main && !pstyle.max_height.is_auto && !pstyle.max_height.is_none && !pstyle.max_height.isIntrinsic()) {
            const mh_px = pstyle.max_height.resolvePx(container_h, pstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            if (mh_px > 0.0) main_size = mh_px;
        }
        const cross_size = if (is_row) container_h else container_w;
        const gap = if (is_row) pstyle.column_gap else pstyle.row_gap;

        // Collect visible flex items (excluding out-of-flow absolute/fixed children)
        let items: [32]FlexItem = [FlexItem{}; 32];
        let item_count: usize = 0;
        let abs_children: [16]NodeId = [NULL_NODE; 16];
        let abs_count: usize = 0;

        let child = doc.nodes.items[@as(usize, parent_id)].first_child;
        while (child != NULL_NODE) {
            const cnode = &doc.nodes.items[@as(usize, child)];
            if (cnode.node_type == NodeType.Element) {
                const cstyle = &styles[@as(usize, child)];
                if (cstyle.display.isNone()) {
                    _.boxes.items[@as(usize, child)].rect = Rect.zero();
                } else if (cstyle.position == CssPosition.Absolute || cstyle.position == CssPosition.Fixed) {
                    if (abs_count < 16) {
                        abs_children[abs_count] = child;
                        abs_count += 1;
                    }
                } else if (item_count < 32) {
                    _.resolveBoxMetrics(doc, styles, child, cstyle, container_w, container_h, !pbox.height_is_auto);
                    const cbox = &_.boxes.items[@as(usize, child)];

                    let base_m: f32 = 0.0;
                    let base_c: f32 = 0.0;

                    if (is_row) {
                        // Main axis = width
                        if (!cstyle.flex_basis.is_auto) {
                            base_m = cstyle.flex_basis.resolvePx(container_w, cstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                        } else if (!cstyle.width.is_auto) {
                            base_m = cbox.borderBoxWidth();
                        } else {
                            base_m = if (cbox.borderBoxWidth() > 0.0) cbox.borderBoxWidth() else 0.0;
                        }
                        if (!cstyle.min_width.is_auto && !cstyle.min_width.is_none) {
                            const min_w = cstyle.min_width.resolvePx(container_w, cstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                            if (base_m < min_w) base_m = min_w;
                        }
                        base_c = cbox.borderBoxHeight();
                    } else {
                        // Main axis = height
                        if (!cstyle.flex_basis.is_auto) {
                            base_m = cstyle.flex_basis.resolvePx(container_h, cstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                        } else if (!cstyle.height.is_auto) {
                            base_m = cbox.borderBoxHeight();
                        } else {
                            base_m = if (cbox.borderBoxHeight() > 0.0) cbox.borderBoxHeight() else 0.0;
                        }
                        if (!cstyle.min_height.is_auto && !cstyle.min_height.is_none) {
                            const min_h = cstyle.min_height.resolvePx(container_h, cstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                            if (base_m < min_h) base_m = min_h;
                        }
                        base_c = cbox.borderBoxWidth();
                    }

                    items[item_count] = FlexItem {
                        node_id: child,
                        base_main: base_m,
                        base_cross: base_c,
                        target_main: base_m,
                        target_cross: base_c,
                        grow: cstyle.flex_grow,
                        shrink: cstyle.flex_shrink,
                    };
                    item_count += 1;
                }
            } else if (cnode.node_type == NodeType.Text) {
                const trimmed = std.mem.trim(u8, cnode.text_content, " \t\r\n");
                if (trimmed.len > 0 && item_count < 32) {
                    const tw = LayoutTree.estimateTextWidth(trimmed, pstyle);
                    const fs = if (pstyle.font_size_px > 0.0) pstyle.font_size_px else 16.0;
                    const th = if (pstyle.has_line_height && pstyle.line_height_px > 0.0) pstyle.line_height_px else (fs * 1.2);
                    _.boxes.items[@as(usize, child)].content_width = tw;
                    _.boxes.items[@as(usize, child)].content_height = th;
                    const base_m = if (is_row) tw else th;
                    const base_c = if (is_row) th else tw;
                    items[item_count] = FlexItem {
                        node_id: child,
                        base_main: base_m,
                        base_cross: base_c,
                        target_main: base_m,
                        target_cross: base_c,
                        grow: 0.0,
                        shrink: 1.0,
                    };
                    item_count += 1;
                }
            }
            child = cnode.next_sibling;
        }

        if (item_count == 0) {
            // Still lay out any out-of-flow absolute/fixed children relative to their containing block
            let ai: usize = 0;
            while (ai < abs_count) : (ai += 1) {
                _.layoutFlexAbsChild(doc, styles, pstyle, is_row, abs_children[ai], content_x, content_y, container_w, container_h);
            }
            return;
        }

        // Split items into flex lines (single line for nowrap, multi-line for wrap/wrap-reverse)
        const cross_gap = if (is_row) pstyle.row_gap else pstyle.column_gap;
        const can_wrap = (pstyle.flex_wrap != FlexWrap.NoWrap) && (main_size > 0.0) && (item_count > 1);
        let line_starts: [32]usize = [0; 32];
        let line_ends: [32]usize = [0; 32];
        let line_count: usize = 0;

        if (!can_wrap) {
            line_starts[0] = 0;
            line_ends[0] = item_count;
            line_count = 1;
        } else {
            let cur_i: usize = 0;
            while (cur_i < item_count && line_count < 32) {
                const start_i = cur_i;
                let line_sum = items[cur_i].base_main;
                cur_i += 1;
                while (cur_i < item_count) {
                    const next_sum = line_sum + gap + items[cur_i].base_main;
                    if (next_sum > main_size + 0.01) break;
                    line_sum = next_sum;
                    cur_i += 1;
                }
                line_starts[line_count] = start_i;
                line_ends[line_count] = cur_i;
                line_count += 1;
            }
        }

        let line_cross_sizes: [32]f32 = [0.0; 32];
        if (line_count > 1) {
            let total_lc: f32 = 0.0;
            let lci: usize = 0;
            while (lci < line_count) : (lci += 1) {
                let mx_c: f32 = 0.0;
                let ii: usize = line_starts[lci];
                while (ii < line_ends[lci]) : (ii += 1) {
                    if (items[ii].base_cross > mx_c) mx_c = items[ii].base_cross;
                }
                line_cross_sizes[lci] = mx_c;
                total_lc += mx_c;
            }
            const total_cg = @as(f32, @floatFromInt(line_count - 1)) * cross_gap;
            if (cross_size > total_lc + total_cg) {
                const add_c = (cross_size - total_lc - total_cg) / @as(f32, @floatFromInt(line_count));
                let lcj: usize = 0;
                while (lcj < line_count) : (lcj += 1) {
                    line_cross_sizes[lcj] += add_c;
                }
            }
        }

        let current_cross: f32 = 0.0;
        let max_used_main: f32 = 0.0;
        let li: usize = 0;
        while (li < line_count) : (li += 1) {
            const l_start = line_starts[li];
            const l_end = line_ends[li];
            const l_count = l_end - l_start;
            if (l_count == 0) continue;

            let total_base_main: f32 = 0.0;
            let total_grow: f32 = 0.0;
            let total_shrink: f32 = 0.0;

            let idx: usize = l_start;
            while (idx < l_end) : (idx += 1) {
                total_base_main += items[idx].base_main;
                total_grow += items[idx].grow;
                total_shrink += items[idx].shrink;
            }

            const gaps_count_f: f32 = @floatFromInt(l_count - 1);
            const total_gaps: f32 = gaps_count_f * gap;
            const line_auto_main = is_auto_main && (line_count == 1);
            const free_space = if (line_auto_main) 0.0 else (main_size - total_base_main - total_gaps);

            // Distribute flex-grow or flex-shrink (with max-size freeze loop for flex-grow)
            if (!line_auto_main && free_space > 0.0 && total_grow > 0.0) {
                let frozen: [32]bool = [false; 32];
                let iter_g: usize = 0;
                while (iter_g < l_count) : (iter_g += 1) {
                    let rem_free: f32 = main_size - total_gaps;
                    let rem_grow: f32 = 0.0;
                    let ig: usize = l_start;
                    while (ig < l_end) : (ig += 1) {
                        if (frozen[ig]) {
                            rem_free -= items[ig].target_main;
                        } else {
                            rem_free -= items[ig].base_main;
                            rem_grow += items[ig].grow;
                        }
                    }
                    if (rem_grow <= 0.0 || rem_free <= 0.0) break;
                    let any_clamped = false;
                    let i: usize = l_start;
                    while (i < l_end) : (i += 1) {
                        if (frozen[i]) continue;
                        const ratio = items[i].grow / rem_grow;
                        let tgt = items[i].base_main + rem_free * ratio;
                        const cs = &styles[@as(usize, items[i].node_id)];
                        if (is_row) {
                            if (!cs.max_width.is_auto && !cs.max_width.is_none && !cs.max_width.isIntrinsic()) {
                                const mw = cs.max_width.resolvePx(container_w, cs.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                                if (tgt > mw) { tgt = mw; frozen[i] = true; any_clamped = true; }
                            }
                        } else {
                            if (!cs.max_height.is_auto && !cs.max_height.is_none && !cs.max_height.isIntrinsic()) {
                                const mh = cs.max_height.resolvePx(main_size, cs.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                                if (tgt > mh) { tgt = mh; frozen[i] = true; any_clamped = true; }
                            }
                        }
                        items[i].target_main = tgt;
                    }
                    if (!any_clamped) break;
                }
            } else if (!line_auto_main && free_space < 0.0 && total_shrink > 0.0) {
                let i: usize = l_start;
                while (i < l_end) : (i += 1) {
                    const ratio = items[i].shrink / total_shrink;
                    const shrunk = items[i].base_main + free_space * ratio;
                    items[i].target_main = if (shrunk > 0.0) shrunk else 0.0;
                }
            }

            // Enforce min main size constraints
            let mi: usize = l_start;
            while (mi < l_end) : (mi += 1) {
                const cid = items[mi].node_id;
                const cs = &styles[@as(usize, cid)];
                if (is_row) {
                    if (!cs.min_width.is_auto && !cs.min_width.is_none) {
                        const min_w = cs.min_width.resolvePx(container_w, cs.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                        if (items[mi].target_main < min_w) items[mi].target_main = min_w;
                    }
                } else {
                    if (!cs.min_height.is_auto && !cs.min_height.is_none) {
                        const min_h = cs.min_height.resolvePx(container_h, cs.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                        if (items[mi].target_main < min_h) items[mi].target_main = min_h;
                    }
                }
            }

            // Justify-content offsets
            let used_main: f32 = total_gaps;
            let j: usize = l_start;
            while (j < l_end) : (j += 1) {
                used_main += items[j].target_main;
            }
            const remaining_main = if (main_size > used_main) (main_size - used_main) else 0.0;

            const count_f: f32 = @floatFromInt(l_count);
            const count_plus_1: f32 = @floatFromInt(l_count + 1);
            let count_minus_1: f32 = 1.0;
            if (l_count > 1) {
                count_minus_1 = @floatFromInt(l_count - 1);
            }

            const is_rev = (pstyle.flex_direction == FlexDirection.RowReverse || pstyle.flex_direction == FlexDirection.ColumnReverse);
            const start_offset: f32 = switch (pstyle.justify_content) {
                .FlexStart => if (is_rev) remaining_main else 0.0,
                .FlexEnd => if (is_rev) 0.0 else remaining_main,
                .Center => remaining_main / 2.0,
                .SpaceAround => (remaining_main / count_f) / 2.0,
                .SpaceEvenly => remaining_main / count_plus_1,
                .SpaceBetween => 0.0,
            };

            if (is_rev && l_count > 1) {
                let ri: usize = 0;
                while (ri < l_count / 2) : (ri += 1) {
                    const tmp_it = items[l_start + ri];
                    items[l_start + ri] = items[l_end - 1 - ri];
                    items[l_end - 1 - ri] = tmp_it;
                }
            }

            let space_between_step = gap;
            if (l_count > 1) {
                space_between_step = gap + remaining_main / count_minus_1;
            }

            const between_step: f32 = switch (pstyle.justify_content) {
                .SpaceBetween => space_between_step,
                .SpaceAround => gap + remaining_main / count_f,
                .SpaceEvenly => gap + remaining_main / count_plus_1,
                * => gap,
            };

            // CssPosition each flex item in this line and layout its children
            let current_main = start_offset;
            let line_max_cross: f32 = 0.0;
            const line_cross_avail = if (line_count == 1) cross_size else line_cross_sizes[li];

            let k: usize = l_start;
            while (k < l_end) : (k += 1) {
                let itm = &items[k];
                const cid = itm.node_id;
                const cbox = &_.boxes.items[@as(usize, cid)];

                const cross_item_size = if (is_row) cbox.borderBoxHeight() else cbox.borderBoxWidth();
                let cross_pos: f32 = current_cross;
                if (line_cross_avail > cross_item_size) {
                    if (pstyle.align_items == .FlexEnd) {
                        cross_pos = current_cross + (line_cross_avail - cross_item_size);
                    } else if (pstyle.align_items == .Center) {
                        cross_pos = current_cross + (line_cross_avail - cross_item_size) / 2.0;
                    }
                }

                if (pstyle.align_items == .Stretch && line_cross_avail > 0.0) {
                    itm.target_cross = line_cross_avail;
                    if (is_row && styles[@as(usize, cid)].height.is_auto) {
                        _.boxes.items[@as(usize, cid)].forced_height = line_cross_avail;
                    }
                }

                const item_x = if (is_row) (content_x + current_main) else (content_x + cross_pos);
                const item_y = if (is_row) (content_y + cross_pos) else (content_y + current_main);

                // Pass resolved main dimension as available content size
                if (is_row) {
                    const h_decor = cbox.paddingHorizontal() + cbox.borderHorizontal();
                    _.boxes.items[@as(usize, cid)].content_width = if (itm.target_main > h_decor) (itm.target_main - h_decor) else 0.0;
                } else {
                    const v_decor = cbox.paddingVertical() + cbox.borderVertical();
                    if (!line_auto_main && itm.target_main > 0.0) {
                        _.boxes.items[@as(usize, cid)].forced_height = itm.target_main;
                        if (itm.target_main > v_decor) {
                            _.boxes.items[@as(usize, cid)].content_height = itm.target_main - v_decor;
                        }
                    }
                }

                const item_avail_w = if (is_row) itm.target_main else container_w;
                const item_avail_h = if (is_row) (if (container_h > 0.0) container_h else 0.0) else itm.target_main;

                if (doc.nodes.items[@as(usize, cid)].node_type == NodeType.Text) {
                    const tw = if (is_row) itm.target_main else itm.target_cross;
                    const th = if (is_row) itm.target_cross else itm.target_main;
                    _.boxes.items[@as(usize, cid)].content_width = tw;
                    _.boxes.items[@as(usize, cid)].content_height = th;
                    _.boxes.items[@as(usize, cid)].rect = Rect {
                        x: item_x,
                        y: item_y,
                        width: tw,
                        height: th,
                        visible: (pstyle.visibility == Visibility.Visible),
                    };
                } else {
                    _.layoutNode(doc, styles, cid, item_x, item_y, item_avail_w, item_avail_h);
                }

                const rendered_cross = if (is_row) _.boxes.items[@as(usize, cid)].borderBoxHeight() else _.boxes.items[@as(usize, cid)].borderBoxWidth();
                if (rendered_cross > line_max_cross) {
                    line_max_cross = rendered_cross;
                }

                const actual_main = if (is_row) _.boxes.items[@as(usize, cid)].borderBoxWidth() else _.boxes.items[@as(usize, cid)].borderBoxHeight();
                const step_main = if (actual_main > itm.target_main) actual_main else itm.target_main;
                current_main += step_main + between_step;
            }

            const line_used_main = if (l_count > 0) (current_main - between_step) else 0.0;
            if (line_used_main > max_used_main) {
                max_used_main = line_used_main;
            }
            current_cross += line_max_cross;
            if (li + 1 < line_count) {
                current_cross += cross_gap;
            }
        }

        // Expand container auto dimension on cross-axis or main-axis if auto
        const used_main_total = max_used_main;
        const max_item_cross = current_cross;
        if (is_row) {
            _.boxes.items[@as(usize, parent_id)].scroll_content_width = @max(_.boxes.items[@as(usize, parent_id)].content_width, used_main_total);
            _.boxes.items[@as(usize, parent_id)].scroll_content_height = @max(_.boxes.items[@as(usize, parent_id)].content_height, max_item_cross);
            if (_.boxes.items[@as(usize, parent_id)].height_is_auto) {
                if (max_item_cross > _.boxes.items[@as(usize, parent_id)].content_height) {
                    _.boxes.items[@as(usize, parent_id)].content_height = max_item_cross;
                }
            }
        } else {
            _.boxes.items[@as(usize, parent_id)].scroll_content_width = @max(_.boxes.items[@as(usize, parent_id)].content_width, max_item_cross);
            _.boxes.items[@as(usize, parent_id)].scroll_content_height = @max(_.boxes.items[@as(usize, parent_id)].content_height, used_main_total);
            if (styles[@as(usize, parent_id)].width.is_auto) {
                if (max_item_cross > _.boxes.items[@as(usize, parent_id)].content_width) {
                    _.boxes.items[@as(usize, parent_id)].content_width = max_item_cross;
                }
            }
            if (_.boxes.items[@as(usize, parent_id)].height_is_auto) {
                if (used_main_total > _.boxes.items[@as(usize, parent_id)].content_height) {
                    _.boxes.items[@as(usize, parent_id)].content_height = used_main_total;
                }
            }
        }

        // Lay out out-of-flow absolute/fixed children relative to their containing block
        if (abs_count > 0) {
            let ai: usize = 0;
            while (ai < abs_count) : (ai += 1) {
                _.layoutFlexAbsChild(doc, styles, pstyle, is_row, abs_children[ai], content_x, content_y, container_w, container_h);
            }
        }

        // CSS Flexbox §4.4: compute baseline_offset for inline-flex containers from first in-flow flex item
        const pstyle_flex = &styles[@as(usize, parent_id)];
        if (pstyle_flex.overflow_visible && pstyle_flex.display.isInlineFlex()) {
            const pbox_y = _.boxes.items[@as(usize, parent_id)].rect.y;
            const pbox_h = _.boxes.items[@as(usize, parent_id)].rect.height;
            let fch = doc.nodes.items[@as(usize, parent_id)].first_child;
            while (fch != NULL_NODE && @as(usize, fch) < doc.nodes.items.len) {
                const fn_d = &doc.nodes.items[@as(usize, fch)];
                if (fn_d.node_type == NodeType.Element) {
                    const fst = &styles[@as(usize, fch)];
                    if (!fst.display.isNone() && fst.position != CssPosition.Absolute && fst.position != CssPosition.Fixed) {
                        const fbox = &_.boxes.items[@as(usize, fch)];
                        let bl: f32 = -1.0;
                        if (fbox.baseline_offset >= 0.0) {
                            bl = (fbox.rect.y - pbox_y) + fbox.baseline_offset;
                        } else if (fbox.rect.height > 0.0) {
                            bl = (fbox.rect.y - pbox_y) + fbox.rect.height;
                        } else {
                            bl = @min(pbox_h, pbox_h * 0.5 + pstyle_flex.naturalAscent() * 0.5);
                        }
                        if (bl >= 0.0) {
                            _.boxes.items[@as(usize, parent_id)].baseline_offset = bl;
                        }
                        break;
                    }
                }
                fch = fn_d.next_sibling;
            }
        }
    }

    /// Primary entry point: computes screen coordinates for all nodes in `doc` into `tree.boxes`.
    fn computeLayout(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
    )! {
        _.abs_cb_x = 0.0;
        _.abs_cb_y = 0.0;
        _.abs_cb_w = _.viewport_width;
        _.abs_cb_h = _.viewport_height;
        const total = doc.nodes.items.len;
        _.boxes.clearRetainingCapacity();
        let i: usize = 0;
        while (i < total) : (i += 1) {
            _.boxes.append(_.allocator, LayoutBox {
                node_id: @intCast(i),
            })?;
        }

        if (doc.root_element != NULL_NODE) {
            _.layoutNode(
                doc,
                styles,
                doc.root_element,
                0.0,
                0.0,
                _.viewport_width,
                _.viewport_height,
            );
        }
    }
}
