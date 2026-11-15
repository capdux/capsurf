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

pub struct LayoutTree {
    allocator: std.mem.Allocator;
    boxes: std.ArrayListUnmanaged(LayoutBox) = std.ArrayListUnmanaged(LayoutBox).empty;
    viewport_width: f32 = 1280.0;
    viewport_height: f32 = 800.0;
    root_font_px: f32 = 16.0;

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

        // Content width calculation
        let raw_w: f32 = 0.0;
        if (!style.width.is_auto && !style.width.is_none) {
            raw_w = style.width.resolvePx(parent_w, font_px, root_px, vw, vh);
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
        } else {
            box.content_height = 0.0; // Will be auto-expanded by children
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

        _.resolveBoxMetrics(node_id, style, avail_w, avail_h);
        const box = &_.boxes.items[@as(usize, node_id)];

        // Compute screen coordinates for this element's border box
        const border_box_x = origin_x + box.margin_left;
        const border_box_y = origin_y + box.margin_top;

        // Content box origin for children
        const content_x = border_box_x + box.border_left + box.padding_left;
        const content_y = border_box_y + box.border_top + box.padding_top;

        if (style.display.isFlex()) {
            _.layoutFlexContainer(doc, styles, node_id, content_x, content_y);
        } else {
            _.layoutBlockContainer(doc, styles, node_id, content_x, content_y);
        }

        // Re-read box after child layout (as content_height may have been updated)
        const final_box = &_.boxes.items[@as(usize, node_id)];
        const final_w = final_box.borderBoxWidth();
        const final_h = final_box.borderBoxHeight();

        // Apply relative positioning offsets (if any)
        let pos_offset_x: f32 = 0.0;
        let pos_offset_y: f32 = 0.0;
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
        }

        const is_visible = (style.visibility == Visibility.Visible);
        _.boxes.items[@as(usize, node_id)].rect = Rect {
            x: border_box_x + pos_offset_x,
            y: border_box_y + pos_offset_y,
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
                    _.layoutNode(doc, styles, child, content_x, cursor_y, content_w, 0.0);
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
            const auto_h = cursor_y - content_y;
            _.boxes.items[@as(usize, parent_id)].content_height = if (auto_h > 0.0) auto_h else 0.0;
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
                    _.resolveBoxMetrics(child, cstyle, container_w, container_h);
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
