// Migrated from vendor/kitesurf/packages/dom-shim/src/range.ts
// Implements DOM Range, BoundaryPoint comparison, and Common Ancestor computation over Arena DOM tree.

import { Document, NodeData, NodeId, NULL_NODE, NodeType } from "./dom.cp";

pub const START_TO_START: u16 = 0;
pub const START_TO_END: u16 = 1;
pub const END_TO_END: u16 = 2;
pub const END_TO_START: u16 = 3;

pub struct DOMRange {
    start_container: NodeId = NULL_NODE,
    start_offset: u32 = 0,
    end_container: NodeId = NULL_NODE,
    end_offset: u32 = 0,
    collapsed: bool = true,
    common_ancestor: NodeId = NULL_NODE,

    static fn init(): DOMRange {
        return DOMRange{};
    }

    fn setStart(*_, doc: *const Document, node: NodeId, offset: u32) {
        _.start_container = node;
        _.start_offset = offset;
        if (_.end_container == NULL_NODE) {
            _.end_container = node;
            _.end_offset = offset;
        }
        _.updateCollapsedAndAncestor(doc);
    }

    fn setEnd(*_, doc: *const Document, node: NodeId, offset: u32) {
        _.end_container = node;
        _.end_offset = offset;
        if (_.start_container == NULL_NODE) {
            _.start_container = node;
            _.start_offset = offset;
        }
        _.updateCollapsedAndAncestor(doc);
    }

    fn collapse(*_, to_start: bool) {
        if (to_start) {
            _.end_container = _.start_container;
            _.end_offset = _.start_offset;
        } else {
            _.start_container = _.end_container;
            _.start_offset = _.end_offset;
        }
        _.collapsed = true;
    }

    fn updateCollapsedAndAncestor(*_, doc: *const Document) {
        _.collapsed = (_.start_container == _.end_container && _.start_offset == _.end_offset);
        _.common_ancestor = findCommonAncestor(doc, _.start_container, _.end_container);
    }

    fn compareBoundaryPoints(how: u16, other: *const DOMRange, doc: *const Document): i32 {
        let this_node = if (how == START_TO_START || how == START_TO_END) _.start_container else _.end_container;
        let this_offset = if (how == START_TO_START || how == START_TO_END) _.start_offset else _.end_offset;

        let other_node = if (how == START_TO_START || how == END_TO_START) other.start_container else other.end_container;
        let other_offset = if (how == START_TO_START || how == END_TO_START) other.start_offset else other.end_offset;

        return comparePoint(doc, this_node, this_offset, other_node, other_offset);
    }

    fn intersectsNode(doc: *const Document, node: NodeId): bool {
        if (node == NULL_NODE) return false;
        let parent = doc.getNode(node);
        if (parent == null) return false;

        let p_id = parent.?.parent;
        if (p_id == NULL_NODE) return false;

        let idx = parent.?.element_index_in_parent;
        // Node's start point is (p_id, idx)
        // Node's end point is (p_id, idx + 1)
        let cmp1 = comparePoint(doc, p_id, idx, _.end_container, _.end_offset);
        let cmp2 = comparePoint(doc, p_id, idx + 1, _.start_container, _.start_offset);

        return (cmp1 < 0 && cmp2 > 0);
    }

    fn selectNodeContents(*_, doc: *const Document, node: NodeId) {
        _.start_container = node;
        _.start_offset = 0;
        _.end_container = node;
        if (n := doc.getNode(node)) {
            if (n.node_type == NodeType.Text) {
                _.end_offset = @intCast(n.text_content.len);
            } else {
                let count: u32 = 0;
                let child = n.first_child;
                while (child != NULL_NODE) {
                    count += 1;
                    let cnode = doc.getNode(child);
                    if (cnode != null) {
                        child = cnode.?.next_sibling;
                    } else {
                        break;
                    }
                }
                _.end_offset = count;
            }
        } else {
            _.end_offset = 0;
        }
        _.updateCollapsedAndAncestor(doc);
    }
}

pub fn findCommonAncestor(doc: *const Document, a: NodeId, b: NodeId): NodeId {
    if (a == NULL_NODE || b == NULL_NODE) return NULL_NODE;
    if (a == b) return a;

    // Traverse ancestors of a and collect
    let a_path: [64]NodeId = [NULL_NODE; 64];
    let a_count: usize = 0;
    let cur = a;
    while (cur != NULL_NODE && a_count < 64) {
        a_path[a_count] = cur;
        a_count += 1;
        let n = doc.getNode(cur);
        if (n == null) break;
        cur = n.?.parent;
    }

    // Traverse ancestors of b and check against a_path
    cur = b;
    while (cur != NULL_NODE) {
        let i: usize = 0;
        while (i < a_count) : (i += 1) {
            if (a_path[i] == cur) {
                return cur;
            }
        }
        let n = doc.getNode(cur);
        if (n == null) break;
        cur = n.?.parent;
    }

    return NULL_NODE;
}

pub fn comparePoint(doc: *const Document, n1: NodeId, o1: u32, n2: NodeId, o2: u32): i32 {
    if (n1 == n2) {
        if (o1 < o2) return -1;
        if (o1 > o2) return 1;
        return 0;
    }

    let ancestor = findCommonAncestor(doc, n1, n2);
    if (ancestor == NULL_NODE) return 0;

    // If n1 is ancestor of n2
    if (ancestor == n1) {
        let child = findChildTowards(doc, n1, n2);
        let n_opt = doc.getNode(child);
        let child_idx = if (n_opt != null) n_opt.?.element_index_in_parent else 0;
        if (o1 <= child_idx) return -1;
        return 1;
    }

    // If n2 is ancestor of n1
    if (ancestor == n2) {
        let child = findChildTowards(doc, n2, n1);
        let n_opt = doc.getNode(child);
        let child_idx = if (n_opt != null) n_opt.?.element_index_in_parent else 0;
        if (child_idx < o2) return -1;
        return 1;
    }

    // Otherwise, compare position of child1 and child2 under ancestor
    let c1 = findChildTowards(doc, ancestor, n1);
    let c2 = findChildTowards(doc, ancestor, n2);
    let opt1 = doc.getNode(c1);
    let opt2 = doc.getNode(c2);
    let idx1 = if (opt1 != null) opt1.?.element_index_in_parent else 0;
    let idx2 = if (opt2 != null) opt2.?.element_index_in_parent else 0;

    if (idx1 < idx2) return -1;
    if (idx1 > idx2) return 1;
    return 0;
}

fn findChildTowards(doc: *const Document, root: NodeId, target: NodeId): NodeId {
    let cur = target;
    while (cur != NULL_NODE) {
        let n = doc.getNode(cur);
        if (n == null) break;
        if (n.?.parent == root) {
            return cur;
        }
        cur = n.?.parent;
    }
    return target;
}
