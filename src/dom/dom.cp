// src/dom/dom.cp
// CapSurf Zero-Cycle Arena DOM Tree (Milestone 1)
// Uses contiguous array storage indexed by 32-bit NodeId (u32), saving 40% memory
// over pointer-linked trees, eliminating borrow conflicts, and enabling cache-friendly
// O(N) linear array scanning for querySelector / querySelectorAll.

import { HtmlParser } from "../html/parser.cp";

pub type NodeId = u32;
pub type StringSlice = []const u8;

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
    namespace: []const u8 = "";
}

pub fn containsDisplayNone(style: []const u8): bool {
    if (style.len < 12) return false;
    let i: usize = 0;
    while (i + 12 <= style.len) : (i += 1) {
        if (std.ascii.eqlIgnoreCase(style[i .. i + 7], "display")) {
            let j = i + 7;
            while (j < style.len && (style[j] == ' ' || style[j] == '\t' || style[j] == '\r' || style[j] == '\n')) : (j += 1) {}
            if (j < style.len && style[j] == ':') {
                j += 1;
                while (j < style.len && (style[j] == ' ' || style[j] == '\t' || style[j] == '\r' || style[j] == '\n')) : (j += 1) {}
                if (j + 4 <= style.len && std.ascii.eqlIgnoreCase(style[j .. j + 4], "none")) {
                    const after = j + 4;
                    if (after == style.len || style[after] == ';' || style[after] == '!' || style[after] == ' ' || style[after] == '\t' || style[after] == '\r' || style[after] == '\n') {
                        return true;
                    }
                }
            }
        }
    }
    return false;
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

    needs_restyle: bool = false;
    needs_layout: bool = false;
    is_display_none: bool = false;

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

    fn hasAttrInAttrs(name: []const u8): bool {
        let i: usize = 0;
        while (i < _.attr_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.attrs[i].name, name)) {
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

    fn getAttributeNS(ns: []const u8, local_name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.attr_count) : (i += 1) {
            const attr_name = _.attrs[i].name;
            let attr_local = attr_name;
            if (idx := std.mem.indexOfScalar(u8, attr_name, ':')) {
                attr_local = attr_name[idx + 1 ..];
            }
            if (!std.ascii.eqlIgnoreCase(attr_local, local_name)) {
                continue;
            }
            if (_.attrs[i].namespace.len > 0) {
                if (std.mem.eql(u8, _.attrs[i].namespace, ns)) {
                    return _.attrs[i].value;
                }
            } else {
                if (std.mem.startsWith(u8, attr_name, "xlink:") && std.mem.eql(u8, ns, "http://www.w3.org/1999/xlink")) {
                    return _.attrs[i].value;
                } else if (std.mem.startsWith(u8, attr_name, "xml:") && std.mem.eql(u8, ns, "http://www.w3.org/XML/1998/namespace")) {
                    return _.attrs[i].value;
                } else if ((std.mem.startsWith(u8, attr_name, "xmlns:") || std.mem.eql(u8, attr_name, "xmlns")) && std.mem.eql(u8, ns, "http://www.w3.org/2000/xmlns/")) {
                    return _.attrs[i].value;
                } else if (ns.len == 0 && std.mem.indexOfScalar(u8, attr_name, ':') == null) {
                    return _.attrs[i].value;
                }
            }
        }
        return null;
    }

    fn getAttributeNames(allocator: std.mem.Allocator): []StringSlice! {
        let names = std.ArrayListUnmanaged(StringSlice).empty;
        let i: usize = 0;
        while (i < _.attr_count) : (i += 1) {
            names.append(allocator, _.attrs[i].name) catch unreachable;
        }
        if (_.id_attr.len > 0 && !_.hasAttrInAttrs("id")) {
            names.append(allocator, "id") catch unreachable;
        }
        if (_.class_attr.len > 0 && !_.hasAttrInAttrs("class")) {
            names.append(allocator, "class") catch unreachable;
        }
        if (_.inline_style.len > 0 && !_.hasAttrInAttrs("style")) {
            names.append(allocator, "style") catch unreachable;
        }
        return names.toOwnedSlice(allocator);
    }

    fn hasAttribute(attr_name: []const u8): bool {
        return _.getAttribute(attr_name) != null;
    }

    fn setAttribute(*_, name: []const u8, value: []const u8) {
        if (std.ascii.eqlIgnoreCase(name, "id")) {
            _.id_attr = value;
        } else if (std.ascii.eqlIgnoreCase(name, "class")) {
            _.setClassName(value);
        } else if (std.ascii.eqlIgnoreCase(name, "style")) {
            _.inline_style = value;
            _.is_display_none = containsDisplayNone(value);
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

    fn setAttributeNS(*_, ns: []const u8, qualified_name: []const u8, value: []const u8) {
        let target_local = qualified_name;
        if (idx := std.mem.indexOfScalar(u8, qualified_name, ':')) {
            target_local = qualified_name[idx + 1 ..];
        }

        let i: usize = 0;
        while (i < _.attr_count) : (i += 1) {
            const attr_name = _.attrs[i].name;
            let attr_local = attr_name;
            if (idx := std.mem.indexOfScalar(u8, attr_name, ':')) {
                attr_local = attr_name[idx + 1 ..];
            }
            if (!std.ascii.eqlIgnoreCase(attr_local, target_local)) {
                continue;
            }
            let ns_match = false;
            if (_.attrs[i].namespace.len > 0) {
                ns_match = std.mem.eql(u8, _.attrs[i].namespace, ns);
            } else {
                if (std.mem.startsWith(u8, attr_name, "xlink:") && std.mem.eql(u8, ns, "http://www.w3.org/1999/xlink")) {
                    ns_match = true;
                } else if (std.mem.startsWith(u8, attr_name, "xml:") && std.mem.eql(u8, ns, "http://www.w3.org/XML/1998/namespace")) {
                    ns_match = true;
                } else if ((std.mem.startsWith(u8, attr_name, "xmlns:") || std.mem.eql(u8, attr_name, "xmlns")) && std.mem.eql(u8, ns, "http://www.w3.org/2000/xmlns/")) {
                    ns_match = true;
                } else if (ns.len == 0 && std.mem.indexOfScalar(u8, attr_name, ':') == null) {
                    ns_match = true;
                }
            }
            if (ns_match) {
                _.attrs[i].name = qualified_name;
                _.attrs[i].value = value;
                _.attrs[i].namespace = ns;
                if (std.ascii.eqlIgnoreCase(qualified_name, "id")) {
                    _.id_attr = value;
                } else if (std.ascii.eqlIgnoreCase(qualified_name, "class")) {
                    _.setClassName(value);
                } else if (std.ascii.eqlIgnoreCase(qualified_name, "style")) {
                    _.inline_style = value;
                    _.is_display_none = containsDisplayNone(value);
                }
                return;
            }
        }

        if (_.attr_count < MAX_ATTRS_PER_NODE) {
            _.attrs[_.attr_count] = Attr { name: qualified_name, value: value, namespace: ns };
            _.attr_count += 1;
        }
        if (std.ascii.eqlIgnoreCase(qualified_name, "id")) {
            _.id_attr = value;
        } else if (std.ascii.eqlIgnoreCase(qualified_name, "class")) {
            _.setClassName(value);
        } else if (std.ascii.eqlIgnoreCase(qualified_name, "style")) {
            _.inline_style = value;
            _.is_display_none = containsDisplayNone(value);
        }
    }

    fn removeAttribute(*_, name: []const u8): bool {
        let special_cleared = false;
        if (std.ascii.eqlIgnoreCase(name, "id")) {
            if (_.id_attr.len > 0) special_cleared = true;
            _.id_attr = "";
        } else if (std.ascii.eqlIgnoreCase(name, "class")) {
            if (_.class_attr.len > 0 || _.class_count > 0) special_cleared = true;
            _.class_attr = "";
            _.class_count = 0;
        } else if (std.ascii.eqlIgnoreCase(name, "style")) {
            if (_.inline_style.len > 0) special_cleared = true;
            _.inline_style = "";
            _.is_display_none = false;
        }

        let i: usize = 0;
        while (i < _.attr_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.attrs[i].name, name)) {
                let j = i;
                while (j + 1 < _.attr_count) : (j += 1) {
                    _.attrs[j] = _.attrs[j + 1];
                }
                _.attr_count -= 1;
                return true;
            }
        }
        return special_cleared;
    }

    fn removeAttributeNS(*_, ns: []const u8, local_name: []const u8): bool {
        let i: usize = 0;
        while (i < _.attr_count) : (i += 1) {
            const attr_name = _.attrs[i].name;
            let attr_local = attr_name;
            if (idx := std.mem.indexOfScalar(u8, attr_name, ':')) {
                attr_local = attr_name[idx + 1 ..];
            }
            if (!std.ascii.eqlIgnoreCase(attr_local, local_name)) {
                continue;
            }
            let ns_match = false;
            if (_.attrs[i].namespace.len > 0) {
                ns_match = std.mem.eql(u8, _.attrs[i].namespace, ns);
            } else {
                if (std.mem.startsWith(u8, attr_name, "xlink:") && std.mem.eql(u8, ns, "http://www.w3.org/1999/xlink")) {
                    ns_match = true;
                } else if (std.mem.startsWith(u8, attr_name, "xml:") && std.mem.eql(u8, ns, "http://www.w3.org/XML/1998/namespace")) {
                    ns_match = true;
                } else if ((std.mem.startsWith(u8, attr_name, "xmlns:") || std.mem.eql(u8, attr_name, "xmlns")) && std.mem.eql(u8, ns, "http://www.w3.org/2000/xmlns/")) {
                    ns_match = true;
                } else if (ns.len == 0 && std.mem.indexOfScalar(u8, attr_name, ':') == null) {
                    ns_match = true;
                }
            }
            if (ns_match) {
                if (std.ascii.eqlIgnoreCase(attr_name, "id")) {
                    _.id_attr = "";
                } else if (std.ascii.eqlIgnoreCase(attr_name, "class")) {
                    _.class_attr = "";
                    _.class_count = 0;
                } else if (std.ascii.eqlIgnoreCase(attr_name, "style")) {
                    _.inline_style = "";
                    _.is_display_none = false;
                }
                let j = i;
                while (j + 1 < _.attr_count) : (j += 1) {
                    _.attrs[j] = _.attrs[j + 1];
                }
                _.attr_count -= 1;
                return true;
            }
        }
        return false;
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
        let i: usize = 0;
        while (i < _.attr_count) : (i += 1) {
            if (std.ascii.eqlIgnoreCase(_.attrs[i].name, "class")) {
                _.attrs[i].value = class_str;
                return;
            }
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
        return _.doc.getAttribute(_.node_id, name);
    }

    fn getAttributeNS(ns: []const u8, local_name: []const u8): ([]const u8)? {
        return _.doc.getAttributeNS(_.node_id, ns, local_name);
    }

    fn setAttributeNS(*_, ns: []const u8, qualified_name: []const u8, value: []const u8) {
        _.doc.setAttributeNS(_.node_id, ns, qualified_name, value);
    }

    fn removeAttributeNS(*_, ns: []const u8, local_name: []const u8): bool {
        return _.doc.removeAttributeNS(_.node_id, ns, local_name);
    }

    fn getAttributeNames(): []StringSlice! {
        return _.doc.getAttributeNames(_.node_id);
    }

    fn setAttribute(*_, name: []const u8, value: []const u8) {
        _.doc.setAttribute(_.node_id, name, value);
    }

    fn removeAttribute(*_, name: []const u8): bool {
        return _.doc.removeAttribute(_.node_id, name);
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

    fn insertBefore(*_, child_id: NodeId, ref_id: NodeId) {
        _.doc.insertBefore(_.node_id, child_id, ref_id);
    }

    fn replaceChild(*_, new_child_id: NodeId, old_child_id: NodeId) {
        _.doc.replaceChild(_.node_id, new_child_id, old_child_id);
    }

    fn cloneNode(*_, deep: bool): NodeId! {
        return _.doc.cloneNode(_.node_id, deep);
    }

    fn getInnerHtml(): ([]const u8)! {
        return _.doc.getInnerHtml(_.node_id);
    }

    fn setInnerHtml(*_, html: []const u8)! {
        return _.doc.setInnerHtml(_.node_id, html);
    }

    fn getOuterHtml(): ([]const u8)! {
        return _.doc.getOuterHtml(_.node_id);
    }
}

pub struct HtmlSerializer {
    doc: *const Document;
    buf: std.ArrayListUnmanaged(u8) = std.ArrayListUnmanaged(u8).empty;

    static fn init(doc: *const Document): HtmlSerializer {
        return HtmlSerializer {
            doc: doc,
            buf: std.ArrayListUnmanaged(u8).empty,
        };
    }

    static fn isVoidTag(tag: []const u8): bool {
        if (tag.len == 2) {
            return std.ascii.eqlIgnoreCase(tag, "br") || std.ascii.eqlIgnoreCase(tag, "hr");
        }
        if (tag.len == 3) {
            return std.ascii.eqlIgnoreCase(tag, "col") || std.ascii.eqlIgnoreCase(tag, "img") || std.ascii.eqlIgnoreCase(tag, "wbr");
        }
        if (tag.len == 4) {
            return std.ascii.eqlIgnoreCase(tag, "area") || std.ascii.eqlIgnoreCase(tag, "base") || std.ascii.eqlIgnoreCase(tag, "link") || std.ascii.eqlIgnoreCase(tag, "meta");
        }
        if (tag.len == 5) {
            return std.ascii.eqlIgnoreCase(tag, "embed") || std.ascii.eqlIgnoreCase(tag, "input") || std.ascii.eqlIgnoreCase(tag, "param") || std.ascii.eqlIgnoreCase(tag, "track");
        }
        if (tag.len == 6) {
            return std.ascii.eqlIgnoreCase(tag, "source");
        }
        return false;
    }

    static fn isRawTextTag(tag: []const u8): bool {
        return std.ascii.eqlIgnoreCase(tag, "script") || std.ascii.eqlIgnoreCase(tag, "style");
    }

    fn serializeNode(*_, node_id: NodeId)! {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.doc.nodes.items.len) {
            return;
        }
        const node = &_.doc.nodes.items[@as(usize, node_id)];
        switch (node.node_type) {
            NodeType.Text => {
                let is_raw = false;
                if (node.parent != NULL_NODE && @as(usize, node.parent) < _.doc.nodes.items.len) {
                    const ptag = _.doc.nodes.items[@as(usize, node.parent)].tag_name;
                    if (HtmlSerializer.isRawTextTag(ptag)) {
                        is_raw = true;
                    }
                }
                if (is_raw) {
                    _.buf.appendSlice(_.doc.allocator, node.text_content)?;
                } else {
                    let i: usize = 0;
                    while (i < node.text_content.len) : (i += 1) {
                        const c = node.text_content[i];
                        switch (c) {
                            '&' => _.buf.appendSlice(_.doc.allocator, "&amp;")?,
                            '<' => _.buf.appendSlice(_.doc.allocator, "&lt;")?,
                            '>' => _.buf.appendSlice(_.doc.allocator, "&gt;")?,
                            else => _.buf.append(_.doc.allocator, c)?,
                        }
                    }
                }
            },
            NodeType.Comment => {
                _.buf.appendSlice(_.doc.allocator, "<!--")?;
                _.buf.appendSlice(_.doc.allocator, node.text_content)?;
                _.buf.appendSlice(_.doc.allocator, "-->")?;
            },
            NodeType.Element => {
                _.buf.append(_.doc.allocator, '<')?;
                _.buf.appendSlice(_.doc.allocator, node.tag_name)?;

                let i: usize = 0;
                while (i < node.attr_count) : (i += 1) {
                    const aname = node.attrs[i].name;
                    const avalue = node.attrs[i].value;
                    _.buf.append(_.doc.allocator, ' ')?;
                    _.buf.appendSlice(_.doc.allocator, aname)?;
                    _.buf.appendSlice(_.doc.allocator, "=\"")?;
                    let j: usize = 0;
                    while (j < avalue.len) : (j += 1) {
                        const ac = avalue[j];
                        switch (ac) {
                            '&' => _.buf.appendSlice(_.doc.allocator, "&amp;")?,
                            '"' => _.buf.appendSlice(_.doc.allocator, "&quot;")?,
                            '<' => _.buf.appendSlice(_.doc.allocator, "&lt;")?,
                            '>' => _.buf.appendSlice(_.doc.allocator, "&gt;")?,
                            else => _.buf.append(_.doc.allocator, ac)?,
                        }
                    }
                    _.buf.append(_.doc.allocator, '"')?;
                }

                if (node.id_attr.len > 0 && !node.hasAttrInAttrs("id")) {
                    _.buf.appendSlice(_.doc.allocator, " id=\"")?;
                    _.buf.appendSlice(_.doc.allocator, node.id_attr)?;
                    _.buf.append(_.doc.allocator, '"')?;
                }
                if (node.class_attr.len > 0 && !node.hasAttrInAttrs("class")) {
                    _.buf.appendSlice(_.doc.allocator, " class=\"")?;
                    _.buf.appendSlice(_.doc.allocator, node.class_attr)?;
                    _.buf.append(_.doc.allocator, '"')?;
                }
                if (node.inline_style.len > 0 && !node.hasAttrInAttrs("style")) {
                    _.buf.appendSlice(_.doc.allocator, " style=\"")?;
                    _.buf.appendSlice(_.doc.allocator, node.inline_style)?;
                    _.buf.append(_.doc.allocator, '"')?;
                }

                _.buf.append(_.doc.allocator, '>')?;

                if (HtmlSerializer.isVoidTag(node.tag_name)) {
                    return;
                }

                let child = node.first_child;
                while (child != NULL_NODE) {
                    const next_child = _.doc.nodes.items[@as(usize, child)].next_sibling;
                    _.serializeNode(child)?;
                    child = next_child;
                }

                _.buf.appendSlice(_.doc.allocator, "</")?;
                _.buf.appendSlice(_.doc.allocator, node.tag_name)?;
                _.buf.append(_.doc.allocator, '>')?;
            },
            NodeType.Document => {
                let child = node.first_child;
                while (child != NULL_NODE) {
                    const next_child = _.doc.nodes.items[@as(usize, child)].next_sibling;
                    _.serializeNode(child)?;
                    child = next_child;
                }
            },
        }
    }
}

pub struct Document {
    allocator: std.mem.Allocator;
    nodes: std.ArrayListUnmanaged(NodeData) = std.ArrayListUnmanaged(NodeData).empty;
    damaged_nodes: std.ArrayListUnmanaged(NodeId) = std.ArrayListUnmanaged(NodeId).empty;
    paint_damage_marked: bool = false;
    mutation_seq: u32 = 1;
    root_element: NodeId = NULL_NODE;
    focused_node: NodeId = NULL_NODE;

    static fn init(allocator: std.mem.Allocator): Document! {
        let doc = Document {
            allocator: allocator,
            nodes: std.ArrayListUnmanaged(NodeData).empty,
            damaged_nodes: std.ArrayListUnmanaged(NodeId).empty,
            paint_damage_marked: false,
            mutation_seq: 1,
            root_element: NULL_NODE,
            focused_node: NULL_NODE,
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
        _.damaged_nodes.deinit(_.allocator);
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

    fn createCommentNode(*_, comment: []const u8): NodeId! {
        const new_id: NodeId = @intCast(_.nodes.items.len);
        const node = NodeData {
            id: new_id,
            node_type: NodeType.Comment,
            tag_name: "#comment",
            text_content: comment,
        };
        _.nodes.append(_.allocator, node)?;
        return new_id;
    }

    fn cloneElementNode(*_, node_id: NodeId): NodeId! {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) {
            return NULL_NODE;
        }
        const src = &_.nodes.items[@as(usize, node_id)];
        const new_id: NodeId = @intCast(_.nodes.items.len);
        const clone = NodeData {
            id: new_id,
            node_type: src.node_type,
            tag_name: src.tag_name,
            local_name_hash: src.local_name_hash,
            id_attr: src.id_attr,
            class_attr: src.class_attr,
            inline_style: src.inline_style,
            classes: src.classes,
            class_count: src.class_count,
            attrs: src.attrs,
            attr_count: src.attr_count,
            is_display_none: src.is_display_none,
        };
        _.nodes.append(_.allocator, clone)?;
        return new_id;
    }

    fn cloneNode(*_, node_id: NodeId, deep: bool): NodeId! {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) {
            return NULL_NODE;
        }

        const sn_type = _.nodes.items[@as(usize, node_id)].node_type;
        const sn_text = _.nodes.items[@as(usize, node_id)].text_content;
        const sn_first_child = _.nodes.items[@as(usize, node_id)].first_child;

        let new_id: NodeId = NULL_NODE;

        if (sn_type == NodeType.Element) {
            new_id = _.cloneElementNode(node_id)?;
        } else if (sn_type == NodeType.Text) {
            new_id = _.createTextNode(sn_text)?;
        } else if (sn_type == NodeType.Comment) {
            new_id = _.createCommentNode(sn_text)?;
        } else {
            return NULL_NODE;
        }

        if (deep) {
            let child = sn_first_child;
            while (child != NULL_NODE) {
                const next_child = _.nodes.items[@as(usize, child)].next_sibling;
                const clone_child = _.cloneNode(child, true)?;
                _.appendChild(new_id, clone_child);
                child = next_child;
            }
        }
        return new_id;
    }

    fn getAttributeNS(node_id: NodeId, ns: []const u8, local_name: []const u8): ([]const u8)? {
        if (n := _.getNode(node_id)) {
            return n.getAttributeNS(ns, local_name);
        }
        return null;
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

    fn markPaintDamage(*_) {
        _.damaged_nodes.clearRetainingCapacity();
        _.paint_damage_marked = true;
    }

    fn recordDamage(*_, node_id: NodeId) {
        if (!_.paint_damage_marked) return;
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) return;
        let i: usize = 0;
        while (i < _.damaged_nodes.items.len) : (i += 1) {
            if (_.damaged_nodes.items[i] == node_id) return;
        }
        _.damaged_nodes.append(_.allocator, node_id) catch {};
    }

    fn isLineageOrSubtree(node_id: NodeId, target_id: NodeId): bool {
        if (target_id == NULL_NODE) return false;
        if (node_id == target_id) return true;

        let cur = target_id;
        while (cur != NULL_NODE && @as(usize, cur) < _.nodes.items.len) {
            if (cur == node_id) return true;
            cur = _.nodes.items[@as(usize, cur)].parent;
        }

        cur = node_id;
        while (cur != NULL_NODE && @as(usize, cur) < _.nodes.items.len) {
            if (cur == target_id) return true;
            cur = _.nodes.items[@as(usize, cur)].parent;
        }

        return false;
    }

    fn isNodeDisplayNone(node_id: NodeId): bool {
        let cur = node_id;
        while (cur != NULL_NODE && @as(usize, cur) < _.nodes.items.len) {
            const n = &_.nodes.items[@as(usize, cur)];
            if (n.is_display_none) return true;
            if (n.inline_style.len > 0 && containsDisplayNone(n.inline_style)) return true;
            cur = n.parent;
        }
        return false;
    }

    fn setAttribute(*_, node_id: NodeId, name: []const u8, value: []const u8) {
        _.mutation_seq += 1;
        _.recordDamage(node_id);
        if (*n := _.getNodeMut(node_id)) {
            n.setAttribute(name, value);
        }
        if (std.ascii.eqlIgnoreCase(name, "dir") || std.ascii.eqlIgnoreCase(name, "lang")) {
            _.markSubtreeNeedsRestyle(node_id);
        }
    }

    fn setAttributeNS(*_, node_id: NodeId, ns: []const u8, qualified_name: []const u8, value: []const u8) {
        _.mutation_seq += 1;
        _.recordDamage(node_id);
        if (*n := _.getNodeMut(node_id)) {
            n.setAttributeNS(ns, qualified_name, value);
        }
        let local_name = qualified_name;
        if (idx := std.mem.indexOfScalar(u8, qualified_name, ':')) {
            local_name = qualified_name[idx + 1 ..];
        }
        if (std.ascii.eqlIgnoreCase(local_name, "dir") || std.ascii.eqlIgnoreCase(local_name, "lang")) {
            _.markSubtreeNeedsRestyle(node_id);
        }
    }

    fn removeAttribute(*_, node_id: NodeId, name: []const u8): bool {
        _.mutation_seq += 1;
        _.recordDamage(node_id);
        let res = false;
        if (*n := _.getNodeMut(node_id)) {
            res = n.removeAttribute(name);
        }
        if (std.ascii.eqlIgnoreCase(name, "dir") || std.ascii.eqlIgnoreCase(name, "lang")) {
            _.markSubtreeNeedsRestyle(node_id);
        }
        return res;
    }

    fn removeAttributeNS(*_, node_id: NodeId, ns: []const u8, local_name: []const u8): bool {
        _.mutation_seq += 1;
        _.recordDamage(node_id);
        let res = false;
        if (*n := _.getNodeMut(node_id)) {
            res = n.removeAttributeNS(ns, local_name);
        }
        if (std.ascii.eqlIgnoreCase(local_name, "dir") || std.ascii.eqlIgnoreCase(local_name, "lang")) {
            _.markSubtreeNeedsRestyle(node_id);
        }
        return res;
    }

    pub fn markSubtreeNeedsRestyle(*_, node_id: NodeId) {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) return;
        const parent_id = _.nodes.items[@as(usize, node_id)].parent;
        let scope_id = node_id;
        if (parent_id != NULL_NODE && @as(usize, parent_id) < _.nodes.items.len) {
            if (_.nodes.items[@as(usize, parent_id)].isElement()) {
                scope_id = parent_id;
            }
        }
        _.markNodeAndDescendantsRestyle(scope_id);
    }

    fn markNodeAndDescendantsRestyle(*_, root_id: NodeId) {
        if (root_id == NULL_NODE || @as(usize, root_id) >= _.nodes.items.len) return;
        let stack: [128]NodeId = [NULL_NODE; 128];
        let stack_len: usize = 0;
        stack[stack_len] = root_id;
        stack_len += 1;

        while (stack_len > 0) {
            stack_len -= 1;
            const cur_id = stack[stack_len];
            if (cur_id == NULL_NODE || @as(usize, cur_id) >= _.nodes.items.len) continue;
            _.nodes.items[@as(usize, cur_id)].needs_restyle = true;
            _.nodes.items[@as(usize, cur_id)].needs_layout = true;
            _.recordDamage(cur_id);

            let child = _.nodes.items[@as(usize, cur_id)].first_child;
            while (child != NULL_NODE && @as(usize, child) < _.nodes.items.len) : (child = _.nodes.items[@as(usize, child)].next_sibling) {
                if (stack_len < stack.len) {
                    stack[stack_len] = child;
                    stack_len += 1;
                }
            }
        }
    }

    pub fn matchesDir(node_id: NodeId, raw_want: []const u8): bool {
        const want = std.mem.trim(u8, raw_want, " \t\r\n'\"");
        if (want.len == 0) return false;
        const want_rtl = std.ascii.eqlIgnoreCase(want, "rtl");
        const want_ltr = std.ascii.eqlIgnoreCase(want, "ltr");
        if (!want_rtl && !want_ltr) return false;

        let cur = node_id;
        while (cur != NULL_NODE && @as(usize, cur) < _.nodes.items.len) {
            const node = &_.nodes.items[@as(usize, cur)];
            if (node.isElement()) {
                if (dir_attr := node.getAttribute("dir")) {
                    const d = std.mem.trim(u8, dir_attr, " \t\r\n'\"");
                    if (std.ascii.eqlIgnoreCase(d, "rtl")) {
                        return want_rtl;
                    } else if (std.ascii.eqlIgnoreCase(d, "ltr")) {
                        return want_ltr;
                    } else if (std.ascii.eqlIgnoreCase(d, "auto")) {
                        const auto_dir = _.getAutoDirection(cur);
                        if (std.ascii.eqlIgnoreCase(auto_dir, "rtl")) {
                            return want_rtl;
                        } else {
                            return want_ltr;
                        }
                    }
                }
            }
            cur = node.parent;
        }

        // HTML default directionality is LTR
        return want_ltr;
    }

    pub fn getAutoDirection(node_id: NodeId): []const u8 {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) return "ltr";
        let stack: [64]NodeId = [NULL_NODE; 64];
        let stack_len: usize = 0;

        let child = _.nodes.items[@as(usize, node_id)].first_child;
        let children: [32]NodeId = [NULL_NODE; 32];
        let ch_count: usize = 0;
        while (child != NULL_NODE && @as(usize, child) < _.nodes.items.len && ch_count < 32) : (child = _.nodes.items[@as(usize, child)].next_sibling) {
            children[ch_count] = child;
            ch_count += 1;
        }
        let ci = ch_count;
        while (ci > 0) {
            ci -= 1;
            stack[stack_len] = children[ci];
            stack_len += 1;
        }

        while (stack_len > 0) {
            stack_len -= 1;
            const cur_id = stack[stack_len];
            if (cur_id == NULL_NODE || @as(usize, cur_id) >= _.nodes.items.len) continue;
            const cur_node = &_.nodes.items[@as(usize, cur_id)];
            if (cur_node.node_type == NodeType.Text && cur_node.text_content.len > 0) {
                let ti: usize = 0;
                while (ti < cur_node.text_content.len) : (ti += 1) {
                    const b = cur_node.text_content[ti];
                    if ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z')) {
                        return "ltr";
                    }
                    if (b >= 0xD6 && b <= 0xDB) {
                        return "rtl";
                    }
                }
            } else if (cur_node.isElement()) {
                if (cur_node.getAttribute("dir") != null) continue;
                let sub_child = cur_node.first_child;
                let sub_children: [32]NodeId = [NULL_NODE; 32];
                let sub_count: usize = 0;
                while (sub_child != NULL_NODE && @as(usize, sub_child) < _.nodes.items.len && sub_count < 32) : (sub_child = _.nodes.items[@as(usize, sub_child)].next_sibling) {
                    sub_children[sub_count] = sub_child;
                    sub_count += 1;
                }
                let sci = sub_count;
                while (sci > 0) {
                    sci -= 1;
                    if (stack_len < stack.len) {
                        stack[stack_len] = sub_children[sci];
                        stack_len += 1;
                    }
                }
            }
        }
        return "ltr";
    }

    pub fn matchesLang(node_id: NodeId, raw_want: []const u8): bool {
        const want = std.mem.trim(u8, raw_want, " \t\r\n'\"");
        if (want.len == 0) return false;

        let cur = node_id;
        while (cur != NULL_NODE && @as(usize, cur) < _.nodes.items.len) {
            const node = &_.nodes.items[@as(usize, cur)];
            if (node.isElement()) {
                let raw_lang = node.getAttribute("lang");
                if (raw_lang == null) raw_lang = node.getAttribute("xml:lang");
                if (lang_attr := raw_lang) {
                    const l = std.mem.trim(u8, lang_attr, " \t\r\n'\"");
                    if (l.len > 0) {
                        if (std.ascii.eqlIgnoreCase(l, want)) return true;
                        if (l.len > want.len && l[want.len] == '-') {
                            if (std.ascii.eqlIgnoreCase(l[0..want.len], want)) return true;
                        }
                        return false;
                    }
                }
            }
            cur = node.parent;
        }
        return false;
    }

    fn getAttributeNames(node_id: NodeId): []StringSlice! {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) {
            return _.allocator.alloc(StringSlice, 0);
        }
        return _.nodes.items[@as(usize, node_id)].getAttributeNames(_.allocator);
    }

    fn appendChild(*_, parent_id: NodeId, child_id: NodeId) {
        if (parent_id == NULL_NODE || child_id == NULL_NODE || parent_id == child_id) {
            return;
        }
        if (@as(usize, parent_id) >= _.nodes.items.len || @as(usize, child_id) >= _.nodes.items.len) {
            return;
        }

        _.mutation_seq += 1;
        _.recordDamage(parent_id);
        _.recordDamage(child_id);

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

    fn removeChild(*_, node_id: NodeId) {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) {
            return;
        }
        const parent_id = _.nodes.items[@as(usize, node_id)].parent;
        if (parent_id == NULL_NODE || @as(usize, parent_id) >= _.nodes.items.len) {
            return;
        }

        _.mutation_seq += 1;
        _.recordDamage(node_id);
        _.recordDamage(parent_id);

        const prev_id = _.nodes.items[@as(usize, node_id)].prev_sibling;
        const next_id = _.nodes.items[@as(usize, node_id)].next_sibling;

        if (prev_id != NULL_NODE) {
            _.nodes.items[@as(usize, prev_id)].next_sibling = next_id;
        } else {
            _.nodes.items[@as(usize, parent_id)].first_child = next_id;
        }

        if (next_id != NULL_NODE) {
            _.nodes.items[@as(usize, next_id)].prev_sibling = prev_id;
        } else {
            _.nodes.items[@as(usize, parent_id)].last_child = prev_id;
        }

        if (_.nodes.items[@as(usize, node_id)].node_type == NodeType.Element) {
            _.reindexElementChildren(parent_id);
        }

        _.nodes.items[@as(usize, node_id)].parent = NULL_NODE;
        _.nodes.items[@as(usize, node_id)].prev_sibling = NULL_NODE;
        _.nodes.items[@as(usize, node_id)].next_sibling = NULL_NODE;
    }

    fn reindexElementChildren(*_, parent_id: NodeId) {
        if (parent_id == NULL_NODE || @as(usize, parent_id) >= _.nodes.items.len) {
            return;
        }
        let count: u32 = 0;
        let curr = _.nodes.items[@as(usize, parent_id)].first_child;
        while (curr != NULL_NODE && @as(usize, curr) < _.nodes.items.len) {
            if (_.nodes.items[@as(usize, curr)].node_type == NodeType.Element) {
                count += 1;
                _.nodes.items[@as(usize, curr)].element_index_in_parent = count;
            } else {
                _.nodes.items[@as(usize, curr)].element_index_in_parent = 0;
            }
            curr = _.nodes.items[@as(usize, curr)].next_sibling;
        }
        _.nodes.items[@as(usize, parent_id)].child_element_count = count;
    }

    fn insertBefore(*_, parent_id: NodeId, child_id: NodeId, ref_id: NodeId) {
        if (parent_id == NULL_NODE || child_id == NULL_NODE || parent_id == child_id || child_id == ref_id) {
            return;
        }
        if (ref_id == NULL_NODE) {
            _.appendChild(parent_id, child_id);
            return;
        }
        if (@as(usize, parent_id) >= _.nodes.items.len ||
            @as(usize, child_id) >= _.nodes.items.len ||
            @as(usize, ref_id) >= _.nodes.items.len) {
            return;
        }
        if (_.nodes.items[@as(usize, ref_id)].parent != parent_id) {
            return;
        }
        if (_.nodes.items[@as(usize, child_id)].parent == parent_id &&
            _.nodes.items[@as(usize, child_id)].next_sibling == ref_id) {
            return;
        }

        // Detach if already attached
        if (_.nodes.items[@as(usize, child_id)].parent != NULL_NODE) {
            _.removeChild(child_id);
        }

        _.mutation_seq += 1;
        _.recordDamage(parent_id);
        _.recordDamage(child_id);
        _.recordDamage(ref_id);

        const prev_id = _.nodes.items[@as(usize, ref_id)].prev_sibling;

        _.nodes.items[@as(usize, child_id)].parent = parent_id;
        _.nodes.items[@as(usize, child_id)].next_sibling = ref_id;
        _.nodes.items[@as(usize, child_id)].prev_sibling = prev_id;

        _.nodes.items[@as(usize, ref_id)].prev_sibling = child_id;

        if (prev_id != NULL_NODE && ref_id != _.nodes.items[@as(usize, parent_id)].first_child) {
            _.nodes.items[@as(usize, prev_id)].next_sibling = child_id;
        } else {
            _.nodes.items[@as(usize, child_id)].prev_sibling = NULL_NODE;
            _.nodes.items[@as(usize, parent_id)].first_child = child_id;
        }

        if (_.nodes.items[@as(usize, parent_id)].last_child == NULL_NODE) {
            _.nodes.items[@as(usize, parent_id)].last_child = ref_id;
        }

        _.reindexElementChildren(parent_id);
    }

    fn replaceChild(*_, parent_id: NodeId, new_child_id: NodeId, old_child_id: NodeId) {
        if (parent_id == NULL_NODE || new_child_id == NULL_NODE || old_child_id == NULL_NODE) {
            return;
        }
        if (new_child_id == old_child_id) {
            return;
        }
        if (@as(usize, parent_id) >= _.nodes.items.len ||
            @as(usize, new_child_id) >= _.nodes.items.len ||
            @as(usize, old_child_id) >= _.nodes.items.len) {
            return;
        }
        if (_.nodes.items[@as(usize, old_child_id)].parent != parent_id) {
            return;
        }

        _.insertBefore(parent_id, new_child_id, old_child_id);
        _.removeChild(old_child_id);
    }

    fn reparentChildren(*_, from_id: NodeId, to_id: NodeId) {
        if (from_id == NULL_NODE || to_id == NULL_NODE || from_id == to_id) {
            return;
        }
        while (true) {
            const child_id = _.nodes.items[@as(usize, from_id)].first_child;
            if (child_id == NULL_NODE) {
                break;
            }
            _.removeChild(child_id);
            _.appendChild(to_id, child_id);
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

    fn isConnectedNode(nid: NodeId): bool {
        if (nid == NULL_NODE || nid == 0) return false;
        if (_.root_element == NULL_NODE) return false;
        let curr = nid;
        while (curr != NULL_NODE && @as(usize, curr) < _.nodes.items.len) {
            if (curr == _.root_element) return true;
            const sn = &_.nodes.items[@as(usize, curr)];
            if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "template")) {
                if (curr != nid) return false;
            }
            curr = sn.parent;
        }
        return false;
    }

    fn getElementById(target_id: []const u8): NodeId {
        if (target_id.len == 0) {
            return NULL_NODE;
        }
        let i: usize = 1;
        while (i < _.nodes.items.len) : (i += 1) {
            const node = &_.nodes.items[i];
            if (node.node_type == NodeType.Element && node.hasId(target_id)) {
                const nid: NodeId = @intCast(i);
                if (_.isConnectedNode(nid)) {
                    return nid;
                }
            }
        }
        return NULL_NODE;
    }

    // 返回按文档节点顺序匹配的 NodeId 切片，调用方用 _.allocator 释放
    fn getElementsByName(target: []const u8): ([]NodeId)! {
        let out = std.ArrayListUnmanaged(NodeId).empty;
        if (target.len == 0) {
            return out.toOwnedSlice(_.allocator);
        }
        let i: usize = 1;
        while (i < _.nodes.items.len) : (i += 1) {
            const node = &_.nodes.items[i];
            if (node.node_type == NodeType.Element) {
                if (v := node.getAttribute("name")) {
                    if (std.mem.eql(u8, v, target)) {
                        out.append(_.allocator, @intCast(i))?;
                    }
                }
            }
        }
        return out.toOwnedSlice(_.allocator);
    }

    fn getFirstChild(node_id: NodeId): NodeId {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) return NULL_NODE;
        return _.nodes.items[@as(usize, node_id)].first_child;
    }

    fn getParent(node_id: NodeId): NodeId {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) return NULL_NODE;
        return _.nodes.items[@as(usize, node_id)].parent;
    }

    fn getTextContent(node_id: NodeId): []const u8 {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) return "";
        return _.nodes.items[@as(usize, node_id)].text_content;
    }

    fn getTagName(node_id: NodeId): []const u8 {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) return "";
        return _.nodes.items[@as(usize, node_id)].tag_name;
    }

    fn isScriptElement(node_id: NodeId): bool {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) return false;
        const n = &_.nodes.items[@as(usize, node_id)];
        return n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, "script");
    }

    fn getScriptText(node_id: NodeId, allocator: std.mem.Allocator): []const u8 {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) return "";
        const n = &_.nodes.items[@as(usize, node_id)];
        if (n.text_content.len > 0) return n.text_content;
        let buf = std.ArrayListUnmanaged(u8).empty;
        let child = n.first_child;
        while (child != NULL_NODE && @as(usize, child) < _.nodes.items.len) {
            const cn = &_.nodes.items[@as(usize, child)];
            if (cn.node_type == NodeType.Text && cn.text_content.len > 0) {
                buf.appendSlice(allocator, cn.text_content) catch break;
            }
            child = cn.next_sibling;
        }
        return buf.toOwnedSlice(allocator) catch "";
    }

    fn getAttribute(node_id: NodeId, name: []const u8): ([]const u8)? {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) return null;
        return _.nodes.items[@as(usize, node_id)].getAttribute(name);
    }

    fn parseHtml(*_, html: []const u8)! {
        HtmlParser.parse(_, html)?;
    }

    // 深拷贝 src 文档中的 src_id 子树到本文档并挂到 parent_id 下（字符串全部 dupe）
    fn copySubtreeFrom(*_, src: *Document, src_id: NodeId, parent_id: NodeId)! {
        const sn = &src.nodes.items[@as(usize, src_id)];
        let new_id: NodeId = NULL_NODE;
        if (sn.node_type == NodeType.Element) {
            const tag = _.allocator.dupe(u8, sn.tag_name)?;
            new_id = _.createElement(tag)?;
            let ai: usize = 0;
            while (ai < sn.attr_count) : (ai += 1) {
                const an = _.allocator.dupe(u8, sn.attrs[ai].name)?;
                const av = _.allocator.dupe(u8, sn.attrs[ai].value)?;
                if (sn.attrs[ai].namespace.len > 0) {
                    const ans = _.allocator.dupe(u8, sn.attrs[ai].namespace)?;
                    _.setAttributeNS(new_id, ans, an, av);
                } else {
                    _.setAttribute(new_id, an, av);
                }
            }
        } else if (sn.node_type == NodeType.Text) {
            new_id = _.createTextNode(_.allocator.dupe(u8, sn.text_content)?)?;
        } else if (sn.node_type == NodeType.Comment) {
            new_id = _.createCommentNode(_.allocator.dupe(u8, sn.text_content)?)?;
        } else {
            return;
        }
        _.appendChild(parent_id, new_id);
        let child = sn.first_child;
        while (child != NULL_NODE) {
            _.copySubtreeFrom(src, child, new_id)?;
            child = src.nodes.items[@as(usize, child)].next_sibling;
        }
    }

    // innerHTML setter：清空子节点，解析片段并拷贝进 node_id
    fn setInnerHtml(*_, node_id: NodeId, html: []const u8)! {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) {
            return;
        }
        while (_.nodes.items[@as(usize, node_id)].first_child != NULL_NODE) {
            _.removeChild(_.nodes.items[@as(usize, node_id)].first_child);
        }
        if (html.len == 0) {
            return;
        }
        const wrapped = std.fmt.allocPrint(_.allocator, "<html><body>{s}", .{html})?;
        defer _.allocator.free(wrapped);
        let tmp = Document.init(_.allocator)?;
        defer tmp.deinit();
        tmp.parseHtml(wrapped)?;
        let i: usize = 1;
        while (i < tmp.nodes.items.len) : (i += 1) {
            const n = &tmp.nodes.items[i];
            if (n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, "body")) {
                let c = n.first_child;
                while (c != NULL_NODE) {
                    _.copySubtreeFrom(&tmp, c, node_id)?;
                    c = tmp.nodes.items[@as(usize, c)].next_sibling;
                }
                break;
            }
        }
    }

    fn getInnerHtml(*_, node_id: NodeId): ([]const u8)! {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) {
            return _.allocator.dupe(u8, "");
        }
        let s = HtmlSerializer.init(_);
        errdefer s.buf.deinit(_.allocator);

        let child = _.nodes.items[@as(usize, node_id)].first_child;
        while (child != NULL_NODE) {
            const next_child = _.nodes.items[@as(usize, child)].next_sibling;
            s.serializeNode(child)?;
            child = next_child;
        }

        return s.buf.toOwnedSlice(_.allocator);
    }

    fn getOuterHtml(*_, node_id: NodeId): ([]const u8)! {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.nodes.items.len) {
            return _.allocator.dupe(u8, "");
        }
        let s = HtmlSerializer.init(_);
        errdefer s.buf.deinit(_.allocator);

        s.serializeNode(node_id)?;
        return s.buf.toOwnedSlice(_.allocator);
    }
}


pub const Doc = Document;

