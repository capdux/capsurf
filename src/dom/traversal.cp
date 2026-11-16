// Migrated from vendor/kitesurf/packages/dom-shim/src/traversal.ts
// Implements WHATWG TreeWalker, NodeIterator, and NodeFilter over Arena DOM tree.

import { Document, NodeData, NodeId, NULL_NODE, NodeType } from "./dom.cp";

pub const FILTER_ACCEPT: u16 = 1;
pub const FILTER_REJECT: u16 = 2;
pub const FILTER_SKIP: u16 = 3;

pub const SHOW_ALL: u32 = 0xFFFFFFFF;
pub const SHOW_ELEMENT: u32 = 0x00000001;
pub const SHOW_TEXT: u32 = 0x00000004;
pub const SHOW_COMMENT: u32 = 0x00000080;
pub const SHOW_DOCUMENT: u32 = 0x00000100;

pub fn nodeTypeToWhatToShow(nt: NodeType): u32 {
    return switch (nt) {
        .Element => SHOW_ELEMENT,
        .Text => SHOW_TEXT,
        .Comment => SHOW_COMMENT,
        .Document => SHOW_DOCUMENT,
    };
}

pub fn filterNode(doc: *const Document, node_id: NodeId, what_to_show: u32): u16 {
    if (node_id == NULL_NODE) return FILTER_REJECT;
    let node_opt = doc.getNode(node_id);
    if (node_opt == null) return FILTER_REJECT;
    let node = node_opt.?;
    let mask = nodeTypeToWhatToShow(node.node_type);
    if ((what_to_show & mask) != 0) {
        return FILTER_ACCEPT;
    }
    return FILTER_SKIP;
}

pub struct TreeWalker {
    root: NodeId = NULL_NODE,
    current_node: NodeId = NULL_NODE,
    what_to_show: u32 = SHOW_ALL,

    static fn init(root_node: NodeId, show_mask: u32): TreeWalker {
        return TreeWalker{
            root: root_node,
            current_node: root_node,
            what_to_show: show_mask,
        };
    }

    fn parentNode(*_, doc: *const Document): NodeId {
        let node = _.current_node;
        while (node != NULL_NODE && node != _.root) {
            let n_opt = doc.getNode(node);
            if (n_opt == null) break;
            node = n_opt.?.parent;
            if (node != NULL_NODE) {
                if (filterNode(doc, node, _.what_to_show) == FILTER_ACCEPT) {
                    _.current_node = node;
                    return node;
                }
            }
        }
        return NULL_NODE;
    }

    fn firstChild(*_, doc: *const Document): NodeId {
        return _.traverseChildren(doc, true);
    }

    fn lastChild(*_, doc: *const Document): NodeId {
        return _.traverseChildren(doc, false);
    }

    fn traverseChildren(*_, doc: *const Document, forward: bool): NodeId {
        let n_opt = doc.getNode(_.current_node);
        if (n_opt == null) return NULL_NODE;
        let child = if (forward) n_opt.?.first_child else n_opt.?.last_child;

        while (child != NULL_NODE) {
            let c_opt = doc.getNode(child);
            if (c_opt == null) break;
            let filter = filterNode(doc, child, _.what_to_show);
            if (filter == FILTER_ACCEPT) {
                _.current_node = child;
                return child;
            }
            if (filter == FILTER_SKIP) {
                // If skipped, dive into its children
                let grand = if (forward) c_opt.?.first_child else c_opt.?.last_child;
                if (grand != NULL_NODE) {
                    child = grand;
                    continue;
                }
            }
            child = if (forward) c_opt.?.next_sibling else c_opt.?.prev_sibling;
        }
        return NULL_NODE;
    }

    fn nextSibling(*_, doc: *const Document): NodeId {
        return _.traverseSiblings(doc, true);
    }

    fn previousSibling(*_, doc: *const Document): NodeId {
        return _.traverseSiblings(doc, false);
    }

    fn traverseSiblings(*_, doc: *const Document, forward: bool): NodeId {
        let node = _.current_node;
        if (node == _.root) return NULL_NODE;

        while (node != NULL_NODE) {
            let n_opt = doc.getNode(node);
            if (n_opt == null) break;
            let sibling = if (forward) n_opt.?.next_sibling else n_opt.?.prev_sibling;

            while (sibling != NULL_NODE) {
                let s_opt = doc.getNode(sibling);
                if (s_opt == null) break;
                let filter = filterNode(doc, sibling, _.what_to_show);
                if (filter == FILTER_ACCEPT) {
                    _.current_node = sibling;
                    return sibling;
                }
                sibling = if (forward) s_opt.?.next_sibling else s_opt.?.prev_sibling;
            }

            node = n_opt.?.parent;
            if (node == NULL_NODE || node == _.root) break;
            if (filterNode(doc, node, _.what_to_show) == FILTER_ACCEPT) break;
        }
        return NULL_NODE;
    }

    fn nextNode(*_, doc: *const Document): NodeId {
        let node = _.current_node;
        while (true) {
            let n_opt = doc.getNode(node);
            if (n_opt == null) break;

            // 1. Try first child
            let child = n_opt.?.first_child;
            if (child != NULL_NODE) {
                node = child;
                if (filterNode(doc, node, _.what_to_show) == FILTER_ACCEPT) {
                    _.current_node = node;
                    return node;
                }
                continue;
            }

            // 2. Try next sibling or backtrack up
            while (node != NULL_NODE && node != _.root) {
                let cur_opt = doc.getNode(node);
                if (cur_opt == null) break;
                let sib = cur_opt.?.next_sibling;
                if (sib != NULL_NODE) {
                    node = sib;
                    if (filterNode(doc, node, _.what_to_show) == FILTER_ACCEPT) {
                        _.current_node = node;
                        return node;
                    }
                    break;
                }
                node = cur_opt.?.parent;
            }

            if (node == NULL_NODE || node == _.root) break;
        }
        return NULL_NODE;
    }
}

pub struct NodeIterator {
    root: NodeId = NULL_NODE,
    reference_node: NodeId = NULL_NODE,
    pointer_before_ref: bool = true,
    what_to_show: u32 = SHOW_ALL,

    static fn init(root_node: NodeId, show_mask: u32): NodeIterator {
        return NodeIterator{
            root: root_node,
            reference_node: root_node,
            pointer_before_ref: true,
            what_to_show: show_mask,
        };
    }

    fn nextNode(*_, doc: *const Document): NodeId {
        let node = _.reference_node;
        let before = _.pointer_before_ref;

        while (true) {
            if (!before) {
                // Advance to next node in tree order
                node = nextInSubtree(doc, node, _.root);
                if (node == NULL_NODE) return NULL_NODE;
            } else {
                before = false;
            }

            _.reference_node = node;
            _.pointer_before_ref = false;

            if (filterNode(doc, node, _.what_to_show) == FILTER_ACCEPT) {
                return node;
            }
        }
    }

    fn previousNode(*_, doc: *const Document): NodeId {
        let node = _.reference_node;
        let before = _.pointer_before_ref;

        while (true) {
            if (before) {
                node = prevInSubtree(doc, node, _.root);
                if (node == NULL_NODE) return NULL_NODE;
            } else {
                before = true;
            }

            _.reference_node = node;
            _.pointer_before_ref = true;

            if (filterNode(doc, node, _.what_to_show) == FILTER_ACCEPT) {
                return node;
            }
        }
    }
}

fn nextInSubtree(doc: *const Document, node: NodeId, root: NodeId): NodeId {
    let n_opt = doc.getNode(node);
    if (n_opt == null) return NULL_NODE;

    if (n_opt.?.first_child != NULL_NODE) {
        return n_opt.?.first_child;
    }

    let cur = node;
    while (cur != NULL_NODE && cur != root) {
        let c_opt = doc.getNode(cur);
        if (c_opt == null) break;
        if (c_opt.?.next_sibling != NULL_NODE) {
            return c_opt.?.next_sibling;
        }
        cur = c_opt.?.parent;
    }
    return NULL_NODE;
}

fn prevInSubtree(doc: *const Document, node: NodeId, root: NodeId): NodeId {
    if (node == root) return NULL_NODE;
    let n_opt = doc.getNode(node);
    if (n_opt == null) return NULL_NODE;

    let prev = n_opt.?.prev_sibling;
    if (prev != NULL_NODE) {
        // Deepest last descendant of prev
        let cur = prev;
        while (true) {
            let c_opt = doc.getNode(cur);
            if (c_opt == null || c_opt.?.last_child == NULL_NODE) break;
            cur = c_opt.?.last_child;
        }
        return cur;
    }

    return n_opt.?.parent;
}
