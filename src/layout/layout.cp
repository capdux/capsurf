// src/layout/layout.cp
// CapSurf Box Model & Flexbox Geometry Layout Engine (Milestone 1)
// Consumes ComputedStyle from cascade.cp, computes screen pixel coordinates
// Rect { x: f32, y: f32, width: f32, height: f32, visible: bool } for every NodeId,
// powering element.getBoundingClientRect() and AI Agent vision/action automation.

import { Document, NodeData, NodeType, NodeId, NULL_NODE } from "../dom/dom.cp";
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
            if (v > 0.0) return v;
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
                        if (sw := LayoutTree.extractSvgDim(src_attr, "width")) {
                            nat_w = sw;
                        }
                    }
                    if (!has_h) {
                        if (sh := LayoutTree.extractSvgDim(src_attr, "height")) {
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
            // Default block width fills available parent content area minus horizontal margins
            const avail = parent_w - box.marginHorizontal();
            raw_w = if (avail > 0.0) avail else 0.0;
        }

        // Adjust for box-sizing
        if (style.box_sizing == BoxSizing.BorderBox) {
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
        } else if (!is_replaced) {
            _.layoutBlockContainer(doc, styles, node_id, content_x, content_y);
        }

        let box_after = &_.boxes.items[@as(usize, node_id)];
        const text_len = LayoutTree.getNodeTextLength(doc, node_id);
        if (text_len > 0) {
            if (style.height.is_auto && box_after.content_height <= 0.0) {
                const font_h = if (style.font_size_px > 0.0) style.font_size_px else 16.0;
                box_after.content_height = font_h;
            }
            if (style.width.is_auto && box_after.content_width <= 0.0) {
                const char_w: f32 = 8.0;
                box_after.content_width = @as(f32, @floatFromInt(text_len)) * char_w;
            }
        }

        // Re-read box after child layout (as content_height may have been updated)
        const final_box = &_.boxes.items[@as(usize, node_id)];
        const final_w = final_box.borderBoxWidth();
        const final_h = final_box.borderBoxHeight();

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
                rect_x = style.left.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            } else if (!style.right.is_auto && !style.right.is_none) {
                const r_px = style.right.resolvePx(avail_w, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                rect_x = avail_w - r_px - final_w;
            }
            if (!style.top.is_auto && !style.top.is_none) {
                rect_y = style.top.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
            } else if (!style.bottom.is_auto && !style.bottom.is_none) {
                const b_px = style.bottom.resolvePx(avail_h, style.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                rect_y = avail_h - b_px - final_h;
            }
        }

        const is_visible = (style.visibility == Visibility.Visible);
        _.boxes.items[@as(usize, node_id)].rect = Rect {
            x: rect_x,
            y: rect_y,
            width: final_w,
            height: final_h,
            visible: is_visible,
        };
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
                    const prev_id = cnode.prev_sibling;
                    if (prev_id != NULL_NODE && @as(usize, prev_id) < doc.nodes.items.len) {
                        const prev_node = &doc.nodes.items[@as(usize, prev_id)];
                        if (prev_node.node_type == .Text && prev_node.text_content.len > 0) {
                            child_x += @as(f32, @floatFromInt(prev_node.text_content.len)) * 8.0;
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
                            base_m = 0.0;
                        }
                        base_c = cbox.borderBoxHeight();
                    } else {
                        // Main axis = height
                        if (!cstyle.flex_basis.is_auto) {
                            base_m = cstyle.flex_basis.resolvePx(container_h, cstyle.font_size_px, _.root_font_px, _.viewport_width, _.viewport_height);
                        } else if (!cstyle.height.is_auto) {
                            base_m = cbox.borderBoxHeight();
                        } else {
                            base_m = 0.0;
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
        const free_space = main_size - total_base_main - total_gaps;

        // Distribute flex-grow or flex-shrink
        if (free_space > 0.0 && total_grow > 0.0) {
            let i: usize = 0;
            while (i < item_count) : (i += 1) {
                const ratio = items[i].grow / total_grow;
                items[i].target_main = items[i].base_main + free_space * ratio;
            }
        } else if (free_space < 0.0 && total_shrink > 0.0) {
            let i: usize = 0;
            while (i < item_count) : (i += 1) {
                const ratio = items[i].shrink / total_shrink;
                const shrunk = items[i].base_main + free_space * ratio;
                items[i].target_main = if (shrunk > 0.0) shrunk else 0.0;
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
                _.boxes.items[@as(usize, cid)].content_height = if (itm.target_main > v_decor) (itm.target_main - v_decor) else 0.0;
            }

            _.layoutNode(doc, styles, cid, item_x, item_y, itm.target_main, itm.target_cross);

            const rendered_cross = if (is_row) _.boxes.items[@as(usize, cid)].borderBoxHeight() else _.boxes.items[@as(usize, cid)].borderBoxWidth();
            if (rendered_cross > max_item_cross) {
                max_item_cross = rendered_cross;
            }

            current_main += itm.target_main + between_step;
        }

        // Expand container auto dimension on cross-axis if auto
        if (is_row) {
            if (styles[@as(usize, parent_id)].height.is_auto) {
                _.boxes.items[@as(usize, parent_id)].content_height = max_item_cross;
            }
        } else {
            if (styles[@as(usize, parent_id)].width.is_auto) {
                _.boxes.items[@as(usize, parent_id)].content_width = max_item_cross;
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
