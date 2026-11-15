// src/dom/dom.cp
// CapSurf Zero-Cycle Arena DOM Tree (Milestone 1)
// Uses contiguous array storage indexed by 32-bit NodeId (u32), saving 40% memory
// over pointer-linked trees, eliminating borrow conflicts, and enabling cache-friendly
// O(N) linear array scanning for querySelector / querySelectorAll.

pub type NodeId = u32;
pub const NULL_NODE: NodeId = 0;

pub const MAX_ATTRS_PER_NODE: usize = 16;
pub const MAX_CLASSES_PER_NODE: usize = 16;

pub enum NodeType {
    Document,
    Element,
    Text,
    Comment,
}

pub struct Attr {
    name: []const u8 = "";
    value: []const u8 = "";
}

pub struct NodeData {
    id: NodeId = NULL_NODE;
    node_type: NodeType = NodeType.Element;

    // 4-byte index links (20 bytes total vs 40+ bytes for 64-bit pointers)
    parent: NodeId = NULL_NODE;
    first_child: NodeId = NULL_NODE;
    last_child: NodeId = NULL_NODE;
    prev_sibling: NodeId = NULL_NODE;
    next_sibling: NodeId = NULL_NODE;

    // Precomputed structural indices for O(1) :first-child, :last-child, :only-child, :nth-child
    child_element_count: u32 = 0;
    element_index_in_parent: u32 = 0; // 1-based index among Element siblings

    // Fast-reject hash for tag name (lowercase FNV-1a 32-bit)
    local_name_hash: u32 = 0;

    // Element identity & attributes
    tag_name: []const u8 = "";
    id_attr: []const u8 = "";
    class_attr: []const u8 = "";
    inline_style: []const u8 = "";
    text_content: []const u8 = "";

    classes: [16][]const u8 = [""; 16];
    class_count: usize = 0;

    attrs: [16]Attr = [Attr{}; 16];
    attr_count: usize = 0;

    static fn hashLowerAscii(s: []const u8): u32 {
        let h: u32 = 2166136261;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = std.ascii.toLower(s[i]);
            h = (h ^ @as(u32, c)) *% 16777619;
        }
        return h;
    }

    fn isElement(): bool {
        return _.node_type == NodeType.Element;
    }

    fn hasTag(tag: []const u8): bool {
        if (_.node_type != NodeType.Element) {
            return false;
        }
        if (tag.len == 1 && tag[0] == '*') {
            return true;
        }
        if (_.tag_name.len != tag.len) {
            return false;
        }
        if (_.local_name_hash != NodeData.hashLowerAscii(tag)) {
            return false;
        }
        return std.ascii.eqlIgnoreCase(_.tag_name, tag);
    }

    fn hasId(target_id: []const u8): bool {
        if (_.id_attr.len != target_id.len) {
            return false;
        }
        return std.mem.eql(u8, _.id_attr, target_id);
    }

    fn hasClass(cls: []const u8): bool {
        if (cls.len == 0 || _.class_count == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < _.class_count) : (i += 1) {
            if (std.mem.eql(u8, _.classes[i], cls)) {
                return true;
            }
        }
        return false;
    }

    fn getAttribute(attr_name: []const u8): ([]const u8)? {
        if (std.ascii.eqlIgnoreCase(attr_name, "id") && _.id_attr.len > 0) {
            return _.id_attr;
        }
        if (std.ascii.eqlIgnoreCase(attr_name, "class") && _.class_attr.len > 0) {
            return _.class_attr;
        }
        if (std.ascii.eqlIgnoreCase(attr_name, "style") && _.inline_style.len > 0) {
            return _.inline_style;
        }
        let i: usize = 0;
        while (i < _.attr_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.attrs[i].name, attr_name)) {
                return _.attrs[i].value;
            }
        }
        return null;
    }

    fn setAttribute(*_, name: []const u8, value: []const u8) {
        if (std.ascii.eqlIgnoreCase(name, "id")) {
            _.id_attr = value;
        } else if (std.ascii.eqlIgnoreCase(name, "class")) {
            _.setClassName(value);
        } else if (std.ascii.eqlIgnoreCase(name, "style")) {
            _.inline_style = value;
        }

        let i: usize = 0;
        while (i < _.attr_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.attrs[i].name, name)) {
                _.attrs[i].value = value;
                return;
            }
        }
        if (_.attr_count < MAX_ATTRS_PER_NODE) {
            _.attrs[_.attr_count] = Attr { name: name, value: value };
            _.attr_count += 1;
        }
    }

    fn setClassName(*_, class_str: []const u8) {
        _.class_attr = class_str;
        _.class_count = 0;
        let it = std.mem.tokenizeAny(u8, class_str, " \t\r\n");
        while (tok := it.next()) {
            if (_.class_count >= MAX_CLASSES_PER_NODE) {
                break;
            }
            _.classes[_.class_count] = tok;
            _.class_count += 1;
        }
    }
}

pub struct ElementHandle {
    doc: *Document;
    node_id: NodeId;

    static fn init(doc: *Document, node_id: NodeId): ElementHandle {
        return ElementHandle { doc: doc, node_id: node_id };
    }

    fn id(): NodeId {
        return _.node_id;
    }

    fn tagName(): []const u8 {
        return _.doc.nodes.items[_.node_id].tag_name;
    }

    fn getAttribute(name: []const u8): ([]const u8)? {
        return _.doc.nodes.items[_.node_id].getAttribute(name);
    }

    fn setAttribute(*_, name: []const u8, value: []const u8) {
        _.doc.setAttribute(_.node_id, name, value);
    }

    fn setId(*_, id_val: []const u8) {
        _.doc.setId(_.node_id, id_val);
    }

    fn setClassName(*_, cls: []const u8) {
        _.doc.setClassName(_.node_id, cls);
    }

    fn setInlineStyle(*_, style_str: []const u8) {
        _.doc.setInlineStyle(_.node_id, style_str);
    }

    fn appendChild(*_, child_id: NodeId) {
        _.doc.appendChild(_.node_id, child_id);
    }
}

pub struct Document {
    allocator: std.mem.Allocator;
    nodes: std.ArrayListUnmanaged(NodeData) = std.ArrayListUnmanaged(NodeData).empty;
    root_element: NodeId = NULL_NODE;

    static fn init(allocator: std.mem.Allocator): Document! {
        let doc = Document {
            allocator: allocator,
            nodes: std.ArrayListUnmanaged(NodeData).empty,
            root_element: NULL_NODE,
        };
        // Slot 0 is reserved for NULL_NODE sentinel
        doc.nodes.append(allocator, NodeData {
            id: NULL_NODE,
            node_type: NodeType.Document,
            tag_name: "#null",
        })?;
        return doc;
    }

    fn deinit(*_) {
        _.nodes.deinit(_.allocator);
    }

    fn nodeCount(): usize {
        return _.nodes.items.len;
    }

    fn getNode(node_id: NodeId): (*const NodeData)? {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) {
            return null;
        }
        return &_.nodes.items[@as(usize, node_id)];
    }

    fn getNodeMut(*_, node_id: NodeId): (*NodeData)? {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) {
            return null;
        }
        return &_.nodes.items[@as(usize, node_id)];
    }

    fn createElement(*_, tag_name: []const u8): NodeId! {
        const new_id: NodeId = @intCast(_.nodes.items.len);
        let node = NodeData {
            id: new_id,
            node_type: NodeType.Element,
            tag_name: tag_name,
            local_name_hash: NodeData.hashLowerAscii(tag_name),
        };
        _.nodes.append(_.allocator, node)?;
        if (_.root_element == NULL_NODE) {
            _.root_element = new_id;
        }
        return new_id;
    }

    fn createElementWithAttrs(
        *_,
        tag_name: []const u8,
        id_val: []const u8,
        class_val: []const u8,
        inline_style: []const u8,
    ): NodeId! {
        const nid = _.createElement(tag_name)?;
        if (id_val.len > 0) {
            _.setId(nid, id_val);
        }
        if (class_val.len > 0) {
            _.setClassName(nid, class_val);
        }
        if (inline_style.len > 0) {
            _.setInlineStyle(nid, inline_style);
        }
        return nid;
    }

    fn createTextNode(*_, text: []const u8): NodeId! {
        const new_id: NodeId = @intCast(_.nodes.items.len);
        const node = NodeData {
            id: new_id,
            node_type: NodeType.Text,
            tag_name: "#text",
            text_content: text,
        };
        _.nodes.append(_.allocator, node)?;
        return new_id;
    }

    fn setId(*_, node_id: NodeId, id_val: []const u8) {
        if (*n := _.getNodeMut(node_id)) {
            n.setAttribute("id", id_val);
        }
    }

    fn setClassName(*_, node_id: NodeId, class_val: []const u8) {
        if (*n := _.getNodeMut(node_id)) {
            n.setAttribute("class", class_val);
        }
    }

    fn setInlineStyle(*_, node_id: NodeId, style_val: []const u8) {
        if (*n := _.getNodeMut(node_id)) {
            n.setAttribute("style", style_val);
        }
    }

    fn setAttribute(*_, node_id: NodeId, name: []const u8, value: []const u8) {
        if (*n := _.getNodeMut(node_id)) {
            n.setAttribute(name, value);
        }
    }

    fn appendChild(*_, parent_id: NodeId, child_id: NodeId) {
        if (parent_id == NULL_NODE || child_id == NULL_NODE || parent_id == child_id) {
            return;
        }
        if (@as(usize, parent_id) >= _.nodes.items.len || @as(usize, child_id) >= _.nodes.items.len) {
            return;
        }

        const prev_last = _.nodes.items[@as(usize, parent_id)].last_child;
        _.nodes.items[@as(usize, child_id)].parent = parent_id;
        _.nodes.items[@as(usize, child_id)].prev_sibling = prev_last;
        _.nodes.items[@as(usize, child_id)].next_sibling = NULL_NODE;

        if (prev_last != NULL_NODE) {
            _.nodes.items[@as(usize, prev_last)].next_sibling = child_id;
        } else {
            _.nodes.items[@as(usize, parent_id)].first_child = child_id;
        }
        _.nodes.items[@as(usize, parent_id)].last_child = child_id;

        if (_.nodes.items[@as(usize, child_id)].node_type == NodeType.Element) {
            _.nodes.items[@as(usize, parent_id)].child_element_count += 1;
            _.nodes.items[@as(usize, child_id)].element_index_in_parent =
                _.nodes.items[@as(usize, parent_id)].child_element_count;
        }
    }

    fn prevElementSibling(node_id: NodeId): NodeId {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) {
            return NULL_NODE;
        }
        let curr = _.nodes.items[@as(usize, node_id)].prev_sibling;
        while (curr != NULL_NODE) {
            if (_.nodes.items[@as(usize, curr)].node_type == NodeType.Element) {
                return curr;
            }
            curr = _.nodes.items[@as(usize, curr)].prev_sibling;
        }
        return NULL_NODE;
    }

    fn nextElementSibling(node_id: NodeId): NodeId {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) {
            return NULL_NODE;
        }
        let curr = _.nodes.items[@as(usize, node_id)].next_sibling;
        while (curr != NULL_NODE) {
            if (_.nodes.items[@as(usize, curr)].node_type == NodeType.Element) {
                return curr;
            }
            curr = _.nodes.items[@as(usize, curr)].next_sibling;
        }
        return NULL_NODE;
    }

    fn isEmptyElement(node_id: NodeId): bool {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) {
            return true;
        }
        let child = _.nodes.items[@as(usize, node_id)].first_child;
        while (child != NULL_NODE) {
            const cnode = &_.nodes.items[@as(usize, child)];
            if (cnode.node_type == NodeType.Element) {
                return false;
            }
            if (cnode.node_type == NodeType.Text && cnode.text_content.len > 0) {
                return false;
            }
            child = cnode.next_sibling;
        }
        return true;
    }

    fn getElementById(target_id: []const u8): NodeId {
        if (target_id.len == 0) {
            return NULL_NODE;
        }
        let i: usize = 1;
        while (i < _.nodes.items.len) : (i += 1) {
            const node = &_.nodes.items[i];
            if (node.node_type == NodeType.Element && node.hasId(target_id)) {
                return @intCast(i);
            }
        }
        return NULL_NODE;
    }
}
