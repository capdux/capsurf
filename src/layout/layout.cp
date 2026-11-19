// src/layout/layout.cp
// CapSurf Box Model & Flexbox Geometry Layout Engine (Milestone 1)
// Consumes ComputedStyle from cascade.cp, computes screen pixel coordinates
// Rect { x: f32, y: f32, width: f32, height: f32, visible: bool } for every NodeId,
// powering element.getBoundingClientRect() and AI Agent vision/action automation.

import { Document, NodeData, NodeType, NodeId, NULL_NODE } from "../dom/dom.cp";
import { sniffPng } from "../render/image_dims.cp";
import {
    ComputedStyle,
    Display,
    Visibility,
    BoxSizing,
    Position,
    FlexDirection,
    JustifyContent,
    AlignItems
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
}

pub struct LayoutTree {
    allocator: std.mem.Allocator;
    boxes: std.ArrayListUnmanaged(LayoutBox) = std.ArrayListUnmanaged(LayoutBox).empty;
    viewport_width: f32 = 1280.0;
    viewport_height: f32 = 800.0;
    root_font_px: f32 = 16.0;

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

    static fn parsePathBounds(d: []const u8): ?[4]f32 {
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
        return [4]f32{ min_x, min_y, max_x - min_x, max_y - min_y };
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

    static fn getReplacedSize(node: *const NodeData): ReplacedSize? {
        if (node.hasTag("canvas")) {
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
            if (!has_w || !has_h) {
                if (src_attr := node.getAttribute("src")) {
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
            return ReplacedSize{
                width: nat_w,
                height: nat_h,
                is_authored: is_auth,
            };
        } else if (node.hasTag("svg")) {
            let nat_w: f32 = 300.0;
            let nat_h: f32 = 150.0;
            let is_auth = false;
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
                    if (w_str := it.next()) {
                        if (w_val := LayoutTree.parseDimStr(w_str)) {
                            if (!has_w && w_val > 0.0) {
                                nat_w = w_val;
                                is_auth = true;
                            }
                        }
                    }
                    if (h_str := it.next()) {
                        if (h_val := LayoutTree.parseDimStr(h_str)) {
                            if (!has_h && h_val > 0.0) {
                                nat_h = h_val;
                                is_auth = true;
                            }
                        }
                    }
                }
            }
            return ReplacedSize{
                width: nat_w,
                height: nat_h,
                is_authored: is_auth,
            };
        }
        return null;
    }

    static fn getNodeTextLength(doc: *const Document, node_id: NodeId): usize {
        if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return 0;
        const n = &doc.nodes.items[@as(usize, node_id)];
        if (n.node_type == NodeType.Text) {
            return n.text_content.len;
        }
        let total: usize = 0;
        let ch = n.first_child;
        while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
            total += LayoutTree.getNodeTextLength(doc, ch);
            ch = doc.nodes.items[@as(usize, ch)].next_sibling;
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
        node_id: NodeId,
        style: *const ComputedStyle,
        parent_w: f32,
        parent_h: f32,
    ) {
        let box = &_.boxes.items[@as(usize, node_id)];
        const font_px = style.font_size_px;
        const root_px = _.root_font_px;
        const vw = _.viewport_width;
        const vh = _.viewport_height;

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
                replaced_opt = LayoutTree.getReplacedSize(node);
            }
        }

        // Content width calculation
        let raw_w: f32 = 0.0;
        if (!style.width.is_auto && !style.width.is_none) {
            raw_w = style.width.resolvePx(parent_w, font_px, root_px, vw, vh);
        } else if (rep := replaced_opt) {
            if (rep.is_authored) {
                raw_w = rep.width;
            } else {
                const avail = parent_w - box.marginHorizontal();
                if (avail > 0.0 && rep.width > avail) {
                    raw_w = avail;
                } else {
                    raw_w = rep.width;
                }
            }
        } else {
            if (style.display.isInlineBlock() || style.display.isInline()) {
                raw_w = 0.0;
            } else {
                // Default block width fills available parent content area minus horizontal margins
                const avail = parent_w - box.marginHorizontal();
                raw_w = if (avail > 0.0) avail else 0.0;
            }
        }

        // Adjust for box-sizing or auto width filling parent
        if (style.box_sizing == BoxSizing.BorderBox || style.width.is_auto) {
            const h_decor = box.paddingHorizontal() + box.borderHorizontal();
            box.content_width = if (raw_w > h_decor) (raw_w - h_decor) else 0.0;
        } else {
            box.content_width = raw_w;
        }

        // Min/max width constraints
        if (!style.min_width.is_auto && !style.min_width.is_none) {
            const min_w = style.min_width.resolvePx(parent_w, font_px, root_px, vw, vh);
            if (box.content_width < min_w) {
                box.content_width = min_w;
            }
        }
        if (!style.max_width.is_auto && !style.max_width.is_none) {
            const max_w = style.max_width.resolvePx(parent_w, font_px, root_px, vw, vh);
            if (box.content_width > max_w) {
                box.content_width = max_w;
            }
        }

        // Content height calculation
        if (!style.height.is_auto && !style.height.is_none) {
            const raw_h = style.height.resolvePx(parent_h, font_px, root_px, vw, vh);
            if (style.box_sizing == BoxSizing.BorderBox) {
                const v_decor = box.paddingVertical() + box.borderVertical();
                box.content_height = if (raw_h > v_decor) (raw_h - v_decor) else 0.0;
            } else {
                box.content_height = raw_h;
            }
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
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.boxes.items.len) {
            return;
        }
        const style = &styles[@as(usize, node_id)];

        // display: none -> Zero geometry, invisible, blocks whole subtree
        if (style.display.isNone()) {
            _.boxes.items[@as(usize, node_id)].rect = Rect.zero();
            return;
        }

        _.resolveBoxMetrics(doc, node_id, style, avail_w, avail_h);
        const box = &_.boxes.items[@as(usize, node_id)];

        // Compute screen coordinates for this element's border box
        const border_box_x = origin_x + box.margin_left;
        const border_box_y = origin_y + box.margin_top;

        // Content box origin for children
        const content_x = border_box_x + box.border_left + box.padding_left;
        const content_y = border_box_y + box.border_top + box.padding_top;

        let is_replaced = false;
        if (node_id != NULL_NODE && @as(usize, node_id) < doc.nodes.items.len) {
            const node = &doc.nodes.items[@as(usize, node_id)];
            if (node.isElement() && LayoutTree.getReplacedSize(node) != null) {
                is_replaced = true;
            }
        }

        if (style.display.isFlex()) {
            _.layoutFlexContainer(doc, styles, node_id, content_x, content_y);
        } else if (style.display.isGrid()) {
            _.layoutGridContainer(doc, styles, node_id, content_x, content_y);
        } else if (!is_replaced && !(node_id < doc.nodes.items.len && doc.nodes.items[@as(usize, node_id)].hasTag("svg"))) {
            _.layoutBlockContainer(doc, styles, node_id, content_x, content_y);
        }

        let box_after = &_.boxes.items[@as(usize, node_id)];
        const text_len = LayoutTree.getNodeTextLength(doc, node_id);
        if (text_len > 0) {
            if (style.height.is_auto && box_after.content_height <= 0.0) {
                const font_h = if (style.line_height_px > 0.0) style.line_height_px else (if (style.font_size_px > 0.0) style.font_size_px else 16.0);
                box_after.content_height = font_h;
            }
            if (style.width.is_auto && (box_after.content_width <= 0.0 || style.display.isInlineBlock() || style.display.isInline())) {
                const base_size = if (style.font_size_px > 0.0) style.font_size_px else 16.0;
                let char_ratio: f32 = 0.50;
                if (style.hasFontFamily("Bungee") || style.hasFontFamily("bungee")) {
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

        if (style.position == Position.Relative) {
            if (!style.left.is_auto && !style.left.is_none) {
                pos_offset_x = style.left.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            } else if (!style.right.is_auto && !style.right.is_none) {
                const r_px = style.right.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                pos_offset_x = 0.0 - r_px;
            }
            if (!style.top.is_auto && !style.top.is_none) {
                pos_offset_y = style.top.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            } else if (!style.bottom.is_auto && !style.bottom.is_none) {
                const b_px = style.bottom.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                pos_offset_y = 0.0 - b_px;
            }
            rect_x += pos_offset_x;
            rect_y += pos_offset_y;
        } else if (style.position == Position.Absolute || style.position == Position.Fixed) {
            if (!style.left.is_auto && !style.left.is_none) {
                rect_x = origin_x + style.left.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height) + final_box.margin_left;
            } else if (!style.right.is_auto && !style.right.is_none) {
                const r_px = style.right.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                rect_x = origin_x + avail_w - r_px - final_w - final_box.margin_right;
            }
            if (!style.top.is_auto && !style.top.is_none) {
                rect_y = origin_y + style.top.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height) + final_box.margin_top;
            } else if (!style.bottom.is_auto && !style.bottom.is_none) {
                const b_px = style.bottom.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                rect_y = origin_y + avail_h - b_px - final_h - final_box.margin_bottom;
            }
        }

        // Apply transform translation offsets
        if (!style.transform_tx.is_auto && !style.transform_tx.is_none) {
            const tx_px = style.transform_tx.resolvePx(final_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            rect_x += tx_px;
        }
        if (!style.transform_ty.is_auto && !style.transform_ty.is_none) {
            const ty_px = style.transform_ty.resolvePx(final_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
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

        _.layoutSvgChildren(doc, styles, node_id, content_x, content_y, offset_x, offset_y, min_x, min_y, scale, svg_w, svg_h);
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
    ) {
        let child = doc.nodes.items[@as(usize, parent_id)].first_child;
        while (child != NULL_NODE && @as(usize, child) < doc.nodes.items.len) : (child = doc.nodes.items[@as(usize, child)].next_sibling) {
            const cnode = &doc.nodes.items[@as(usize, child)];
            if (!cnode.isElement()) continue;

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
                const screen_x = content_x + offset_x + (rx - min_x) * scale;
                const screen_y = content_y + offset_y + (ry - min_y) * scale;
                let screen_w = rw * scale;
                let screen_h = rh * scale;

                if (screen_x < content_x + svg_w && screen_y < content_y + svg_h) {
                    if (screen_x + screen_w > content_x + svg_w) {
                        screen_w = (content_x + svg_w) - screen_x;
                    }
                    if (screen_y + screen_h > content_y + svg_h) {
                        screen_h = (content_y + svg_h) - screen_y;
                    }
                }

                _.boxes.items[@as(usize, child)].rect = Rect {
                    x: screen_x,
                    y: screen_y,
                    width: screen_w,
                    height: screen_h,
                    visible: true,
                };
            } else if (cnode.hasTag("g")) {
                _.layoutSvgChildren(doc, styles, child, content_x, content_y, offset_x, offset_y, min_x, min_y, scale, svg_w, svg_h);
            }
        }
    }

    /// Block formatting layout: stacks children vertically and calculates auto height.
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
        let cursor_y = content_y;
        let max_child_right = content_x;

        let child = doc.nodes.items[@as(usize, parent_id)].first_child;
        while (child != NULL_NODE) {
            const cnode = &doc.nodes.items[@as(usize, child)];
            if (cnode.node_type == NodeType.Element) {
                const cstyle = &styles[@as(usize, child)];
                if (!cstyle.display.isNone()) {
                    let child_x = content_x;
                    if (cstyle.display.isInline()) {
                        const prev_id = cnode.prev_sibling;
                        if (prev_id != NULL_NODE && @as(usize, prev_id) < doc.nodes.items.len) {
                            const prev_node = &doc.nodes.items[@as(usize, prev_id)];
                            if (prev_node.node_type == .Text && prev_node.text_content.len > 0) {
                                const non_ws = std.mem.trim(u8, prev_node.text_content, " \t\r\n");
                                if (non_ws.len > 0) {
                                    child_x += @as(f32, @floatFromInt(prev_node.text_content.len)) * 8.0;
                                }
                            }
                        }
                    }
                    _.layoutNode(doc, styles, child, child_x, cursor_y, content_w, 0.0);
                    const cbox = &_.boxes.items[@as(usize, child)];
                    cursor_y += cbox.outerHeight();
                    const cr = cbox.rect.right();
                    if (cr > max_child_right) {
                        max_child_right = cr;
                    }
                } else {
                    _.boxes.items[@as(usize, child)].rect = Rect.zero();
                }
            }
            child = cnode.next_sibling;
        }

        // If parent has auto height, expand content_height to fit child stream
        if (styles[@as(usize, parent_id)].height.is_auto) {
            let is_p_rep = false;
            if (parent_id != NULL_NODE && @as(usize, parent_id) < doc.nodes.items.len) {
                const pnode = &doc.nodes.items[@as(usize, parent_id)];
                if (pnode.isElement() && LayoutTree.getReplacedSize(pnode) != null) {
                    is_p_rep = true;
                }
            }
            if (!is_p_rep) {
                const auto_h = cursor_y - content_y;
                _.boxes.items[@as(usize, parent_id)].content_height = if (auto_h > 0.0) auto_h else 0.0;
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

        const cols = if (pstyle.grid_cols > 0) pstyle.grid_cols else 1;
        const col_gap = pstyle.column_gap;
        const row_gap = pstyle.row_gap;

        const total_col_gaps = if (cols > 1) @as(f32, @floatFromInt(cols - 1)) * col_gap else 0.0;
        const col_w = if (container_w > total_col_gaps) (container_w - total_col_gaps) / @as(f32, @floatFromInt(cols)) else 0.0;

        let cur_col: u32 = 0;
        let cur_y: f32 = content_y;
        let cur_row_max_h: f32 = 0.0;
        let row_count: u32 = 0;

        let child = doc.nodes.items[@as(usize, parent_id)].first_child;
        while (child != NULL_NODE) {
            const cnode = &doc.nodes.items[@as(usize, child)];
            if (cnode.node_type == NodeType.Element) {
                const cstyle = &styles[@as(usize, child)];
                if (!cstyle.display.isNone()) {
                    const span_raw = if (cstyle.grid_col_span > 0) cstyle.grid_col_span else 1;
                    const span = if (span_raw > cols) cols else span_raw;

                    // If item does not fit in current row, advance to next row
                    if (cur_col + span > cols && cur_col > 0) {
                        cur_y += cur_row_max_h + row_gap;
                        cur_col = 0;
                        cur_row_max_h = 0.0;
                        row_count += 1;
                    }

                    const item_x = content_x + @as(f32, @floatFromInt(cur_col)) * (col_w + col_gap);
                    const item_w = @as(f32, @floatFromInt(span)) * col_w + if (span > 1) @as(f32, @floatFromInt(span - 1)) * col_gap else 0.0;
                    const item_y = cur_y;

                    // Grid item measures its content-based height (auto row)
                    _.layoutNode(doc, styles, child, item_x, item_y, item_w, 0.0);

                    const cbox = &_.boxes.items[@as(usize, child)];
                    const item_outer_h = cbox.outerHeight();
                    if (item_outer_h > cur_row_max_h) {
                        cur_row_max_h = item_outer_h;
                    }

                    cur_col += span;
                    if (cur_col >= cols) {
                        cur_y += cur_row_max_h + row_gap;
                        cur_col = 0;
                        cur_row_max_h = 0.0;
                        row_count += 1;
                    }
                } else {
                    _.boxes.items[@as(usize, child)].rect = Rect.zero();
                }
            }
            child = cnode.next_sibling;
        }

        // Calculate total grid height
        let total_grid_h: f32 = 0.0;
        if (cur_col > 0) {
            cur_y += cur_row_max_h;
            row_count += 1;
            total_grid_h = cur_y - content_y;
        } else if (row_count > 0) {
            total_grid_h = (cur_y - content_y) - row_gap;
        }

        if (pstyle.height.is_auto) {
            _.boxes.items[@as(usize, parent_id)].content_height = if (total_grid_h > 0.0) total_grid_h else 0.0;
        }
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
        const is_auto_main = if (is_row) pstyle.width.is_auto else pstyle.height.is_auto;
        const main_size = if (is_row) container_w else container_h;
        const cross_size = if (is_row) container_h else container_w;
        const gap = if (is_row) pstyle.column_gap else pstyle.row_gap;

        // Collect visible flex items
        let items: [32]FlexItem = [FlexItem{}; 32];
        let item_count: usize = 0;

        let child = doc.nodes.items[@as(usize, parent_id)].first_child;
        while (child != NULL_NODE && item_count < 32) {
            const cnode = &doc.nodes.items[@as(usize, child)];
            if (cnode.node_type == NodeType.Element) {
                const cstyle = &styles[@as(usize, child)];
                if (!cstyle.display.isNone()) {
                    _.resolveBoxMetrics(doc, child, cstyle, container_w, container_h);
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
                } else {
                    _.boxes.items[@as(usize, child)].rect = Rect.zero();
                }
            }
            child = cnode.next_sibling;
        }

        if (item_count == 0) return;

        // Sum up base main sizes and gaps
        let total_base_main: f32 = 0.0;
        let total_grow: f32 = 0.0;
        let total_shrink: f32 = 0.0;

        let idx: usize = 0;
        while (idx < item_count) : (idx += 1) {
            total_base_main += items[idx].base_main;
            total_grow += items[idx].grow;
            total_shrink += items[idx].shrink;
        }

        const gaps_count_f: f32 = @floatFromInt(item_count - 1);
        const total_gaps: f32 = gaps_count_f * gap;
        const free_space = if (is_auto_main) 0.0 else (main_size - total_base_main - total_gaps);

        // Distribute flex-grow or flex-shrink
        if (!is_auto_main && free_space > 0.0 && total_grow > 0.0) {
            let i: usize = 0;
            while (i < item_count) : (i += 1) {
                const ratio = items[i].grow / total_grow;
                items[i].target_main = items[i].base_main + free_space * ratio;
            }
        } else if (!is_auto_main && free_space < 0.0 && total_shrink > 0.0) {
            let i: usize = 0;
            while (i < item_count) : (i += 1) {
                const ratio = items[i].shrink / total_shrink;
                const shrunk = items[i].base_main + free_space * ratio;
                items[i].target_main = if (shrunk > 0.0) shrunk else 0.0;
            }
        }

        // Enforce min main size constraints
        let mi: usize = 0;
        while (mi < item_count) : (mi += 1) {
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
        let j: usize = 0;
        while (j < item_count) : (j += 1) {
            used_main += items[j].target_main;
        }
        const remaining_main = if (main_size > used_main) (main_size - used_main) else 0.0;

        const count_f: f32 = @floatFromInt(item_count);
        const count_plus_1: f32 = @floatFromInt(item_count + 1);
        let count_minus_1: f32 = 1.0;
        if (item_count > 1) {
            count_minus_1 = @floatFromInt(item_count - 1);
        }

        const start_offset: f32 = switch (pstyle.justify_content) {
            .FlexStart => 0.0,
            .FlexEnd => remaining_main,
            .Center => remaining_main / 2.0,
            .SpaceAround => (remaining_main / count_f) / 2.0,
            .SpaceEvenly => remaining_main / count_plus_1,
            .SpaceBetween => 0.0,
        };

        let space_between_step = gap;
        if (item_count > 1) {
            space_between_step = gap + remaining_main / count_minus_1;
        }

        const between_step: f32 = switch (pstyle.justify_content) {
            .SpaceBetween => space_between_step,
            .SpaceAround => gap + remaining_main / count_f,
            .SpaceEvenly => gap + remaining_main / count_plus_1,
            * => gap,
        };

        // Position each flex item and layout its children
        let current_main = start_offset;
        let max_item_cross: f32 = 0.0;

        let k: usize = 0;
        while (k < item_count) : (k += 1) {
            let itm = &items[k];
            const cid = itm.node_id;
            const cbox = &_.boxes.items[@as(usize, cid)];

            const cross_item_size = if (is_row) cbox.borderBoxHeight() else cbox.borderBoxWidth();
            let cross_pos: f32 = 0.0;
            if (cross_size > cross_item_size) {
                if (pstyle.align_items == .FlexEnd) {
                    cross_pos = cross_size - cross_item_size;
                } else if (pstyle.align_items == .Center) {
                    cross_pos = (cross_size - cross_item_size) / 2.0;
                }
            }

            if (pstyle.align_items == .Stretch && cross_size > 0.0) {
                itm.target_cross = cross_size;
            }

            const item_x = if (is_row) (content_x + current_main) else (content_x + cross_pos);
            const item_y = if (is_row) (content_y + cross_pos) else (content_y + current_main);

            // Pass resolved main dimension as available content size
            if (is_row) {
                const h_decor = cbox.paddingHorizontal() + cbox.borderHorizontal();
                _.boxes.items[@as(usize, cid)].content_width = if (itm.target_main > h_decor) (itm.target_main - h_decor) else 0.0;
            } else {
                const v_decor = cbox.paddingVertical() + cbox.borderVertical();
                if (!is_auto_main && itm.target_main > v_decor) {
                    _.boxes.items[@as(usize, cid)].content_height = itm.target_main - v_decor;
                }
            }

            const item_avail_w = if (is_row) itm.target_main else container_w;
            const item_avail_h = if (is_row) (if (container_h > 0.0) container_h else 0.0) else itm.target_main;

            _.layoutNode(doc, styles, cid, item_x, item_y, item_avail_w, item_avail_h);

            const rendered_cross = if (is_row) _.boxes.items[@as(usize, cid)].borderBoxHeight() else _.boxes.items[@as(usize, cid)].borderBoxWidth();
            if (rendered_cross > max_item_cross) {
                max_item_cross = rendered_cross;
            }

            const actual_main = if (is_row) _.boxes.items[@as(usize, cid)].borderBoxWidth() else _.boxes.items[@as(usize, cid)].borderBoxHeight();
            const step_main = if (actual_main > itm.target_main) actual_main else itm.target_main;
            current_main += step_main + between_step;
        }

        // Expand container auto dimension on cross-axis or main-axis if auto
        if (is_row) {
            if (styles[@as(usize, parent_id)].height.is_auto) {
                _.boxes.items[@as(usize, parent_id)].content_height = max_item_cross;
            }
        } else {
            if (styles[@as(usize, parent_id)].width.is_auto) {
                _.boxes.items[@as(usize, parent_id)].content_width = max_item_cross;
            }
            if (styles[@as(usize, parent_id)].height.is_auto) {
                const used_h = if (item_count > 0) (current_main - between_step) else 0.0;
                if (used_h > _.boxes.items[@as(usize, parent_id)].content_height) {
                    _.boxes.items[@as(usize, parent_id)].content_height = used_h;
                }
            }
        }
    }

    /// Primary entry point: computes screen coordinates for all nodes in `doc` into `tree.boxes`.
    fn computeLayout(
        *_,
        doc: *const Document,
        styles: []const ComputedStyle,
    )! {
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
