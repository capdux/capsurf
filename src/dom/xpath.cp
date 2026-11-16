// Migrated from vendor/kitesurf/packages/dom-shim/src/xpath.ts
// Evaluates W3C DOM Level 3 XPath expressions over CapSurf Arena DOM tree.

import { Document, NodeData, NodeId, NULL_NODE, NodeType } from "./dom.cp";

pub const ANY_TYPE: u16 = 0;
pub const NUMBER_TYPE: u16 = 1;
pub const STRING_TYPE: u16 = 2;
pub const BOOLEAN_TYPE: u16 = 3;
pub const UNORDERED_NODE_ITERATOR_TYPE: u16 = 4;
pub const ORDERED_NODE_ITERATOR_TYPE: u16 = 5;
pub const UNORDERED_NODE_SNAPSHOT_TYPE: u16 = 6;
pub const ORDERED_NODE_SNAPSHOT_TYPE: u16 = 7;
pub const ANY_UNORDERED_NODE_TYPE: u16 = 8;
pub const FIRST_ORDERED_NODE_TYPE: u16 = 9;

pub enum XPathResultType: u16 {
    Any = 0,
    Number = 1,
    String = 2,
    Boolean = 3,
    UnorderedNodeIterator = 4,
    OrderedNodeIterator = 5,
    UnorderedNodeSnapshot = 6,
    OrderedNodeSnapshot = 7,
    AnyUnorderedNode = 8,
    FirstOrderedNode = 9,
}

pub enum AxisType {
    Child,
    Descendant,
    DescendantOrSelf,
    Parent,
    Self,
    Ancestor,
    AncestorOrSelf,
    FollowingSibling,
    PrecedingSibling,
    Attribute,
}

pub enum PredicateType {
    AttributeEquals,
    AttributeNotEquals,
    AttributeContains,
    AttributeExists,
    Position,
    TextEquals,
    TextContains,
}

pub struct Predicate {
    pred_type: PredicateType = PredicateType.AttributeExists,
    attr_name: []const u8 = "",
    expected_value: []const u8 = "",
    position_index: usize = 0,
}

pub struct Step {
    axis: AxisType = AxisType.Child,
    from_root: bool = false,
    from_context: bool = false,
    tag_name: []const u8 = "",
    is_any_tag: bool = false,
    is_text: bool = false,
    is_node: bool = false,
    is_self: bool = false,
    is_parent: bool = false,
    predicates: [8]Predicate = [Predicate{}; 8],
    predicate_count: usize = 0,
}

pub struct XPathResult {
    const ANY_TYPE: u16 = 0;
    const NUMBER_TYPE: u16 = 1;
    const STRING_TYPE: u16 = 2;
    const BOOLEAN_TYPE: u16 = 3;
    const UNORDERED_NODE_ITERATOR_TYPE: u16 = 4;
    const ORDERED_NODE_ITERATOR_TYPE: u16 = 5;
    const UNORDERED_NODE_SNAPSHOT_TYPE: u16 = 6;
    const ORDERED_NODE_SNAPSHOT_TYPE: u16 = 7;
    const ANY_UNORDERED_NODE_TYPE: u16 = 8;
    const FIRST_ORDERED_NODE_TYPE: u16 = 9;

    result_type: u16 = 0,
    number_value: f64 = 0.0,
    string_value: []const u8 = "",
    boolean_value: bool = false,
    single_node_value: NodeId = NULL_NODE,
    nodes: [64]NodeId = [NULL_NODE; 64],
    node_count: usize = 0,
    iter_index: usize = 0,

    fn iterateNext(*_): NodeId {
        if (_.iter_index < _.node_count) {
            const nid = _.nodes[_.iter_index];
            _.iter_index += 1;
            return nid;
        }
        return NULL_NODE;
    }

    fn snapshotItem(*_, index: usize): NodeId {
        if (index < _.node_count) {
            return _.nodes[index];
        }
        return NULL_NODE;
    }

    fn snapshotLength(): usize {
        return _.node_count;
    }
}

fn unquote(s: []const u8): []const u8 {
    if (s.len >= 2) {
        if ((s[0] == '\'' && s[s.len - 1] == '\'') || (s[0] == '"' && s[s.len - 1] == '"')) {
            return s[1 .. s.len - 1];
        }
    }
    return s;
}

fn isCountFn(expr: []const u8): bool {
    const s = std.mem.trim(u8, expr, " \t\r\n");
    if (s.len > 7 && std.mem.startsWith(u8, s, "count(") && std.mem.endsWith(u8, s, ")")) {
        return true;
    }
    return false;
}

fn extractCountInner(expr: []const u8): []const u8 {
    const s = std.mem.trim(u8, expr, " \t\r\n");
    if (s.len > 7 && std.mem.startsWith(u8, s, "count(") && std.mem.endsWith(u8, s, ")")) {
        return std.mem.trim(u8, s[6 .. s.len - 1], " \t\r\n");
    }
    return expr;
}

fn getNodeText(doc: *const Document, node_id: NodeId): []const u8 {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return "";
    const node = &doc.nodes.items[@as(usize, node_id)];
    if (node.node_type == NodeType.Text) {
        return node.text_content;
    }
    if (node.text_content.len > 0) {
        return node.text_content;
    }
    let child = node.first_child;
    while (child != NULL_NODE) {
        if (@as(usize, child) < doc.nodes.items.len) {
            const cnode = &doc.nodes.items[@as(usize, child)];
            if (cnode.node_type == NodeType.Text && cnode.text_content.len > 0) {
                return cnode.text_content;
            }
        }
        child = doc.nodes.items[@as(usize, child)].next_sibling;
    }
    return "";
}

fn findRoot(doc: *const Document, context_node: NodeId): NodeId {
    if (context_node != NULL_NODE && @as(usize, context_node) < doc.nodes.items.len) {
        let curr = context_node;
        while (true) {
            const parent = doc.nodes.items[@as(usize, curr)].parent;
            if (parent == NULL_NODE) {
                return curr;
            }
            curr = parent;
        }
    }
    if (doc.root_element != NULL_NODE) {
        return doc.root_element;
    }
    if (doc.nodes.items.len > 1) {
        return 1;
    }
    return NULL_NODE;
}

fn addToSet(nodes: *[64]NodeId, count: *usize, id: NodeId): bool {
    if (id == NULL_NODE) return false;
    let i: usize = 0;
    while (i < count.*) : (i += 1) {
        if (nodes.*[i] == id) return false;
    }
    if (count.* < 64) {
        nodes.*[count.*] = id;
        count.* += 1;
        return true;
    }
    return false;
}

fn collectChildren(doc: *const Document, parent_id: NodeId, out: *[64]NodeId, out_count: *usize) {
    if (parent_id == NULL_NODE || @as(usize, parent_id) >= doc.nodes.items.len) return;
    let child = doc.nodes.items[@as(usize, parent_id)].first_child;
    while (child != NULL_NODE) {
        if (out_count.* < 64) {
            out.*[out_count.*] = child;
            out_count.* += 1;
        }
        child = doc.nodes.items[@as(usize, child)].next_sibling;
    }
}

fn collectDescendants(doc: *const Document, node_id: NodeId, out: *[64]NodeId, out_count: *usize) {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return;
    let child = doc.nodes.items[@as(usize, node_id)].first_child;
    while (child != NULL_NODE) {
        if (out_count.* < 64) {
            out.*[out_count.*] = child;
            out_count.* += 1;
        }
        collectDescendants(doc, child, out, out_count);
        child = doc.nodes.items[@as(usize, child)].next_sibling;
    }
}

fn collectDescendantsOrSelf(doc: *const Document, node_id: NodeId, out: *[64]NodeId, out_count: *usize) {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return;
    if (out_count.* < 64) {
        out.*[out_count.*] = node_id;
        out_count.* += 1;
    }
    collectDescendants(doc, node_id, out, out_count);
}

fn collectParent(doc: *const Document, node_id: NodeId, out: *[64]NodeId, out_count: *usize) {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return;
    const pid = doc.nodes.items[@as(usize, node_id)].parent;
    if (pid != NULL_NODE && out_count.* < 64) {
        out.*[out_count.*] = pid;
        out_count.* += 1;
    }
}

fn collectFollowingSiblings(doc: *const Document, node_id: NodeId, out: *[64]NodeId, out_count: *usize) {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return;
    let s = doc.nodes.items[@as(usize, node_id)].next_sibling;
    while (s != NULL_NODE) {
        if (out_count.* < 64) {
            out.*[out_count.*] = s;
            out_count.* += 1;
        }
        s = doc.nodes.items[@as(usize, s)].next_sibling;
    }
}

fn collectPrecedingSiblings(doc: *const Document, node_id: NodeId, out: *[64]NodeId, out_count: *usize) {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return;
    let s = doc.nodes.items[@as(usize, node_id)].prev_sibling;
    while (s != NULL_NODE) {
        if (out_count.* < 64) {
            out.*[out_count.*] = s;
            out_count.* += 1;
        }
        s = doc.nodes.items[@as(usize, s)].prev_sibling;
    }
}

fn collectAncestors(doc: *const Document, node_id: NodeId, out: *[64]NodeId, out_count: *usize) {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return;
    let p = doc.nodes.items[@as(usize, node_id)].parent;
    while (p != NULL_NODE) {
        if (out_count.* < 64) {
            out.*[out_count.*] = p;
            out_count.* += 1;
        }
        p = doc.nodes.items[@as(usize, p)].parent;
    }
}

fn matchesNode(doc: *const Document, node_id: NodeId, step: *const Step): bool {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return false;
    const node = &doc.nodes.items[@as(usize, node_id)];

    if (step.is_node) {
        // matches any node
    } else if (step.is_text) {
        if (node.node_type != NodeType.Text) return false;
    } else if (step.is_any_tag) {
        if (node.node_type != NodeType.Element) return false;
    } else if (step.tag_name.len > 0) {
        if (node.node_type != NodeType.Element) return false;
        if (!std.ascii.eqlIgnoreCase(node.tag_name, step.tag_name)) return false;
    } else if (step.is_self || step.is_parent) {
        // matches structural step
    }

    let i: usize = 0;
    while (i < step.predicate_count) : (i += 1) {
        const pred = &step.predicates[i];
        switch (pred.pred_type) {
            .AttributeEquals => {
                if (node.node_type != NodeType.Element) return false;
                const val = node.getAttribute(pred.attr_name);
                if (val == null) return false;
                if (!std.mem.eql(u8, val.?, pred.expected_value)) {
                    if (std.ascii.eqlIgnoreCase(pred.attr_name, "class") && node.hasClass(pred.expected_value)) {
                        // matches class token
                    } else {
                        return false;
                    }
                }
            },
            .AttributeNotEquals => {
                if (node.node_type != NodeType.Element) return false;
                const val = node.getAttribute(pred.attr_name);
                if (val != null && std.mem.eql(u8, val.?, pred.expected_value)) return false;
            },
            .AttributeContains => {
                if (node.node_type != NodeType.Element) return false;
                const val = node.getAttribute(pred.attr_name);
                if (val == null) return false;
                if (std.mem.indexOf(u8, val.?, pred.expected_value) == null) return false;
            },
            .AttributeExists => {
                if (node.node_type != NodeType.Element) return false;
                if (!node.hasAttribute(pred.attr_name)) return false;
            },
            .TextEquals => {
                const txt = getNodeText(doc, node_id);
                if (!std.mem.eql(u8, txt, pred.expected_value)) return false;
            },
            .TextContains => {
                const txt = getNodeText(doc, node_id);
                if (std.mem.indexOf(u8, txt, pred.expected_value) == null) return false;
            },
            .Position => {},
        }
    }
    return true;
}

fn parsePredicate(raw: []const u8): Predicate {
    const s = std.mem.trim(u8, raw, " \t\r\n");
    let pred = Predicate{};
    if (s.len == 0) return pred;

    if (std.ascii.isDigit(s[0])) {
        pred.pred_type = PredicateType.Position;
        pred.position_index = std.fmt.parseInt(usize, s, 10) catch 1;
        return pred;
    }

    if (std.mem.startsWith(u8, s, "position()")) {
        const rest = std.mem.trim(u8, s[10..], " \t\r\n");
        if (std.mem.startsWith(u8, rest, "=")) {
            const num_part = std.mem.trim(u8, rest[1..], " \t\r\n");
            pred.pred_type = PredicateType.Position;
            pred.position_index = std.fmt.parseInt(usize, num_part, 10) catch 1;
            return pred;
        }
    }

    if (std.mem.startsWith(u8, s, "contains(")) {
        let inner = std.mem.trim(u8, s[9..], " \t\r\n");
        if (std.mem.endsWith(u8, inner, ")")) {
            inner = std.mem.trim(u8, inner[0 .. inner.len - 1], " \t\r\n");
        }
        if (comma_pos := std.mem.indexOfScalar(u8, inner, ',')) {
            const arg1 = std.mem.trim(u8, inner[0..comma_pos], " \t\r\n");
            const arg2 = std.mem.trim(u8, inner[comma_pos + 1..], " \t\r\n");
            const val = unquote(arg2);
            if (std.mem.startsWith(u8, arg1, "@")) {
                pred.pred_type = PredicateType.AttributeContains;
                pred.attr_name = arg1[1..];
                pred.expected_value = val;
                return pred;
            } else if (std.mem.startsWith(u8, arg1, "text()") || std.mem.eql(u8, arg1, ".")) {
                pred.pred_type = PredicateType.TextContains;
                pred.expected_value = val;
                return pred;
            }
        }
    }

    if (std.mem.startsWith(u8, s, "text()")) {
        const rest = std.mem.trim(u8, s[6..], " \t\r\n");
        if (std.mem.startsWith(u8, rest, "=")) {
            const val = unquote(std.mem.trim(u8, rest[1..], " \t\r\n"));
            pred.pred_type = PredicateType.TextEquals;
            pred.expected_value = val;
            return pred;
        }
    }

    if (std.mem.startsWith(u8, s, "@")) {
        const after_at = s[1..];
        if (neq_pos := std.mem.indexOf(u8, after_at, "!=")) {
            const attr = std.mem.trim(u8, after_at[0..neq_pos], " \t\r\n");
            const val = unquote(std.mem.trim(u8, after_at[neq_pos + 2..], " \t\r\n"));
            pred.pred_type = PredicateType.AttributeNotEquals;
            pred.attr_name = attr;
            pred.expected_value = val;
            return pred;
        } else if (eq_pos := std.mem.indexOfScalar(u8, after_at, '=')) {
            const attr = std.mem.trim(u8, after_at[0..eq_pos], " \t\r\n");
            const val = unquote(std.mem.trim(u8, after_at[eq_pos + 1..], " \t\r\n"));
            pred.pred_type = PredicateType.AttributeEquals;
            pred.attr_name = attr;
            pred.expected_value = val;
            return pred;
        } else {
            pred.pred_type = PredicateType.AttributeExists;
            pred.attr_name = std.mem.trim(u8, after_at, " \t\r\n");
            return pred;
        }
    }

    return pred;
}

fn parseSelector(selector: []const u8, step: *Step) {
    const s = std.mem.trim(u8, selector, " \t\r\n");
    if (s.len == 0) return;

    if (std.mem.eql(u8, s, ".")) {
        step.is_self = true;
        step.axis = AxisType.Self;
        return;
    }
    if (std.mem.eql(u8, s, "..")) {
        step.is_parent = true;
        step.axis = AxisType.Parent;
        return;
    }

    let rest = s;
    if (axis_sep := std.mem.indexOf(u8, s, "::")) {
        const axis_name = s[0..axis_sep];
        rest = s[axis_sep + 2 ..];
        if (std.mem.eql(u8, axis_name, "child")) {
            step.axis = AxisType.Child;
        } else if (std.mem.eql(u8, axis_name, "descendant")) {
            step.axis = AxisType.Descendant;
        } else if (std.mem.eql(u8, axis_name, "descendant-or-self")) {
            step.axis = AxisType.DescendantOrSelf;
        } else if (std.mem.eql(u8, axis_name, "parent")) {
            step.axis = AxisType.Parent;
        } else if (std.mem.eql(u8, axis_name, "self")) {
            step.axis = AxisType.Self;
        } else if (std.mem.eql(u8, axis_name, "ancestor")) {
            step.axis = AxisType.Ancestor;
        } else if (std.mem.eql(u8, axis_name, "ancestor-or-self")) {
            step.axis = AxisType.AncestorOrSelf;
        } else if (std.mem.eql(u8, axis_name, "following-sibling")) {
            step.axis = AxisType.FollowingSibling;
        } else if (std.mem.eql(u8, axis_name, "preceding-sibling")) {
            step.axis = AxisType.PrecedingSibling;
        } else if (std.mem.eql(u8, axis_name, "attribute")) {
            step.axis = AxisType.Attribute;
        }
    }

    if (std.mem.startsWith(u8, rest, "@")) {
        step.axis = AxisType.Attribute;
        step.tag_name = rest[1..];
        return;
    }

    let tag_part = rest;
    if (first_bracket := std.mem.indexOfScalar(u8, rest, '[')) {
        tag_part = rest[0..first_bracket];
    }
    tag_part = std.mem.trim(u8, tag_part, " \t\r\n");

    if (std.mem.eql(u8, tag_part, "*")) {
        step.is_any_tag = true;
    } else if (std.mem.eql(u8, tag_part, "text()")) {
        step.is_text = true;
    } else if (std.mem.eql(u8, tag_part, "node()")) {
        step.is_node = true;
    } else if (std.mem.eql(u8, tag_part, ".")) {
        step.is_self = true;
    } else if (std.mem.eql(u8, tag_part, "..")) {
        step.is_parent = true;
    } else {
        step.tag_name = tag_part;
    }

    let i: usize = 0;
    while (i < rest.len) {
        if (rest[i] == '[') {
            const start = i + 1;
            let depth: usize = 1;
            let j = start;
            let in_q: u8 = 0;
            while (j < rest.len) : (j += 1) {
                const ch = rest[j];
                if (in_q != 0) {
                    if (ch == in_q) in_q = 0;
                } else if (ch == '\'' || ch == '"') {
                    in_q = ch;
                } else if (ch == '[') {
                    depth += 1;
                } else if (ch == ']') {
                    depth -= 1;
                    if (depth == 0) break;
                }
            }
            if (depth == 0 && j > start) {
                const pred_str = rest[start..j];
                if (step.predicate_count < 8) {
                    step.predicates[step.predicate_count] = parsePredicate(pred_str);
                    step.predicate_count += 1;
                }
                i = j + 1;
            } else {
                i += 1;
            }
        } else {
            i += 1;
        }
    }
}

fn parseSteps(expr: []const u8, steps: *[16]Step, step_count: *usize) {
    const s = std.mem.trim(u8, expr, " \t\r\n");
    if (s.len == 0) return;

    let i: usize = 0;
    const n = s.len;

    let initial_axis = AxisType.Child;
    let is_root = false;
    let is_ctx = false;

    if (std.mem.startsWith(u8, s, ".//")) {
        initial_axis = AxisType.DescendantOrSelf;
        is_ctx = true;
        i = 3;
    } else if (std.mem.startsWith(u8, s, "./")) {
        initial_axis = AxisType.Child;
        is_ctx = true;
        i = 2;
    } else if (std.mem.startsWith(u8, s, "//")) {
        initial_axis = AxisType.DescendantOrSelf;
        is_root = true;
        i = 2;
    } else if (std.mem.startsWith(u8, s, "/")) {
        initial_axis = AxisType.Child;
        is_root = true;
        i = 1;
        if (i == n) {
            if (step_count.* < 16) {
                let st = Step{};
                st.axis = AxisType.Self;
                st.from_root = true;
                st.is_self = true;
                steps.*[step_count.*] = st;
                step_count.* += 1;
            }
            return;
        }
    } else {
        initial_axis = AxisType.Child;
        is_ctx = true;
        i = 0;
    }

    let current_axis = initial_axis;
    let first_step = true;

    while (i < n) {
        while (i < n && (s[i] == ' ' || s[i] == '\t')) : (i += 1) {}
        if (i >= n) break;

        const start = i;
        let bracket_depth: usize = 0;
        let in_quote: u8 = 0;

        while (i < n) {
            const ch = s[i];
            if (in_quote != 0) {
                if (ch == in_quote) in_quote = 0;
                i += 1;
            } else if (ch == '\'' || ch == '"') {
                in_quote = ch;
                i += 1;
            } else if (ch == '[') {
                bracket_depth += 1;
                i += 1;
            } else if (ch == ']') {
                if (bracket_depth > 0) bracket_depth -= 1;
                i += 1;
            } else if (bracket_depth == 0 && ch == '/') {
                break;
            } else {
                i += 1;
            }
        }

        const selector = s[start..i];
        if (selector.len > 0) {
            if (step_count.* < 16) {
                let st = Step{};
                st.axis = current_axis;
                if (first_step) {
                    st.from_root = is_root;
                    st.from_context = is_ctx;
                    first_step = false;
                }
                parseSelector(selector, &st);
                steps.*[step_count.*] = st;
                step_count.* += 1;
            }
        }

        if (i < n && s[i] == '/') {
            if (i + 1 < n && s[i + 1] == '/') {
                current_axis = AxisType.DescendantOrSelf;
                i += 2;
            } else {
                current_axis = AxisType.Child;
                i += 1;
            }
        }
    }
}

fn evaluateSteps(
    doc: *const Document,
    steps: *const [16]Step,
    step_count: usize,
    context_node: NodeId,
    out: *[64]NodeId,
    out_count: *usize,
) {
    if (step_count == 0) return;

    const root_node = findRoot(doc, context_node);
    let current_nodes: [64]NodeId = [NULL_NODE; 64];
    let current_count: usize = 0;

    const step0 = &steps[0];
    let init_candidates: [64]NodeId = [NULL_NODE; 64];
    let init_candidate_count: usize = 0;

    if (step0.from_root) {
        if (step0.axis == AxisType.DescendantOrSelf) {
            collectDescendantsOrSelf(doc, root_node, &init_candidates, &init_candidate_count);
        } else if (step0.axis == AxisType.Child) {
            if (root_node != NULL_NODE && init_candidate_count < 64) {
                init_candidates[init_candidate_count] = root_node;
                init_candidate_count += 1;
            }
        } else if (step0.axis == AxisType.Self) {
            if (root_node != NULL_NODE && init_candidate_count < 64) {
                init_candidates[init_candidate_count] = root_node;
                init_candidate_count += 1;
            }
        }
    } else {
        const ctx = if (context_node != NULL_NODE) context_node else root_node;
        if (step0.axis == AxisType.Child) {
            collectChildren(doc, ctx, &init_candidates, &init_candidate_count);
        } else if (step0.axis == AxisType.DescendantOrSelf) {
            collectDescendantsOrSelf(doc, ctx, &init_candidates, &init_candidate_count);
        } else if (step0.axis == AxisType.Descendant) {
            collectDescendants(doc, ctx, &init_candidates, &init_candidate_count);
        } else if (step0.axis == AxisType.Self) {
            if (ctx != NULL_NODE && init_candidate_count < 64) {
                init_candidates[init_candidate_count] = ctx;
                init_candidate_count += 1;
            }
        } else if (step0.axis == AxisType.Parent) {
            collectParent(doc, ctx, &init_candidates, &init_candidate_count);
        }
    }

    let step0_matched: [64]NodeId = [NULL_NODE; 64];
    let step0_matched_count: usize = 0;
    let c_i: usize = 0;
    while (c_i < init_candidate_count) : (c_i += 1) {
        const cand = init_candidates[c_i];
        if (matchesNode(doc, cand, step0)) {
            * = addToSet(&step0_matched, &step0_matched_count, cand);
        }
    }

    let step0_has_pos = false;
    let step0_pos_idx: usize = 0;
    let p_i: usize = 0;
    while (p_i < step0.predicate_count) : (p_i += 1) {
        if (step0.predicates[p_i].pred_type == PredicateType.Position) {
            step0_has_pos = true;
            step0_pos_idx = step0.predicates[p_i].position_index;
            break;
        }
    }

    if (step0_has_pos) {
        if (step0_pos_idx >= 1 && step0_pos_idx <= step0_matched_count) {
            * = addToSet(&current_nodes, &current_count, step0_matched[step0_pos_idx - 1]);
        }
    } else {
        let m_i: usize = 0;
        while (m_i < step0_matched_count) : (m_i += 1) {
            * = addToSet(&current_nodes, &current_count, step0_matched[m_i]);
        }
    }

    let step_idx: usize = 1;
    while (step_idx < step_count) : (step_idx += 1) {
        const step = &steps[step_idx];
        let next_nodes: [64]NodeId = [NULL_NODE; 64];
        let next_count: usize = 0;

        let curr_i: usize = 0;
        while (curr_i < current_count) : (curr_i += 1) {
            const cnode = current_nodes[curr_i];
            let step_candidates: [64]NodeId = [NULL_NODE; 64];
            let step_candidate_count: usize = 0;

            switch (step.axis) {
                .Child => collectChildren(doc, cnode, &step_candidates, &step_candidate_count),
                .Descendant => collectDescendants(doc, cnode, &step_candidates, &step_candidate_count),
                .DescendantOrSelf => collectDescendantsOrSelf(doc, cnode, &step_candidates, &step_candidate_count),
                .Parent => collectParent(doc, cnode, &step_candidates, &step_candidate_count),
                .Self => {
                    if (step_candidate_count < 64) {
                        step_candidates[step_candidate_count] = cnode;
                        step_candidate_count += 1;
                    }
                },
                .FollowingSibling => collectFollowingSiblings(doc, cnode, &step_candidates, &step_candidate_count),
                .PrecedingSibling => collectPrecedingSiblings(doc, cnode, &step_candidates, &step_candidate_count),
                .Ancestor => collectAncestors(doc, cnode, &step_candidates, &step_candidate_count),
                .AncestorOrSelf => {
                    if (step_candidate_count < 64) {
                        step_candidates[step_candidate_count] = cnode;
                        step_candidate_count += 1;
                    }
                    collectAncestors(doc, cnode, &step_candidates, &step_candidate_count);
                },
                .Attribute => {
                    if (step_candidate_count < 64) {
                        step_candidates[step_candidate_count] = cnode;
                        step_candidate_count += 1;
                    }
                },
            }

            let step_matched: [64]NodeId = [NULL_NODE; 64];
            let step_matched_count: usize = 0;
            let sc_i: usize = 0;
            while (sc_i < step_candidate_count) : (sc_i += 1) {
                const cand = step_candidates[sc_i];
                if (matchesNode(doc, cand, step)) {
                    * = addToSet(&step_matched, &step_matched_count, cand);
                }
            }

            let has_pos = false;
            let pos_idx: usize = 0;
            let pi: usize = 0;
            while (pi < step.predicate_count) : (pi += 1) {
                if (step.predicates[pi].pred_type == PredicateType.Position) {
                    has_pos = true;
                    pos_idx = step.predicates[pi].position_index;
                    break;
                }
            }

            if (has_pos) {
                if (pos_idx >= 1 && pos_idx <= step_matched_count) {
                    * = addToSet(&next_nodes, &next_count, step_matched[pos_idx - 1]);
                }
            } else {
                let mi: usize = 0;
                while (mi < step_matched_count) : (mi += 1) {
                    * = addToSet(&next_nodes, &next_count, step_matched[mi]);
                }
            }
        }

        current_nodes = next_nodes;
        current_count = next_count;
    }

    let out_i: usize = 0;
    while (out_i < current_count) : (out_i += 1) {
        if (out_count.* < 64) {
            out.*[out_count.*] = current_nodes[out_i];
            out_count.* += 1;
        }
    }
}

pub struct XPathExpression {
    expr_str: []const u8 = "",

    static fn init(expression: []const u8): XPathExpression {
        return XPathExpression{ expr_str: expression };
    }

    fn evaluate(*_, doc: *const Document, context_node: NodeId, result_type: u16): XPathResult {
        let evaluator = XPathEvaluator{};
        return evaluator.evaluate(doc, _.expr_str, context_node, result_type);
    }
}

pub struct XPathEvaluator {
    static fn init(): XPathEvaluator {
        return XPathEvaluator{};
    }

    fn createExpression(expression: []const u8): XPathExpression {
        return XPathExpression.init(expression);
    }

    fn evaluate(*_, doc: *const Document, expr: []const u8, context_node: NodeId, result_type: u16): XPathResult {
        if (isCountFn(expr)) {
            const inner = extractCountInner(expr);
            let inner_res = _.evaluate(doc, inner, context_node, ORDERED_NODE_SNAPSHOT_TYPE);
            let res = XPathResult{};
            res.result_type = NUMBER_TYPE;
            res.number_value = @floatFromInt(inner_res.node_count);
            res.boolean_value = (inner_res.node_count > 0);
            return res;
        }

        let steps: [16]Step = [Step{}; 16];
        let step_count: usize = 0;
        parseSteps(expr, &steps, &step_count);

        let matched_nodes: [64]NodeId = [NULL_NODE; 64];
        let matched_count: usize = 0;
        evaluateSteps(doc, &steps, step_count, context_node, &matched_nodes, &matched_count);

        let res = XPathResult{};
        res.node_count = matched_count;
        let i: usize = 0;
        while (i < matched_count) : (i += 1) {
            res.nodes[i] = matched_nodes[i];
        }

        if (matched_count > 0) {
            res.single_node_value = matched_nodes[0];
            res.boolean_value = true;
            res.number_value = @floatFromInt(matched_count);
            res.string_value = getNodeText(doc, matched_nodes[0]);
        } else {
            res.single_node_value = NULL_NODE;
            res.boolean_value = false;
            res.number_value = 0.0;
            res.string_value = "";
        }

        if (result_type == ANY_TYPE) {
            res.result_type = UNORDERED_NODE_ITERATOR_TYPE;
        } else {
            res.result_type = result_type;
        }

        if (res.result_type == NUMBER_TYPE) {
            res.number_value = @floatFromInt(matched_count);
        } else if (res.result_type == STRING_TYPE) {
            res.string_value = if (matched_count > 0) getNodeText(doc, matched_nodes[0]) else "";
        } else if (res.result_type == BOOLEAN_TYPE) {
            res.boolean_value = (matched_count > 0);
        }

        return res;
    }
}

pub fn evaluateXPath(doc: *const Document, expr: []const u8, context_node: NodeId, result_type: u16): XPathResult {
    let evaluator = XPathEvaluator{};
    return evaluator.evaluate(doc, expr, context_node, result_type);
}
