// src/cdp/accessibility.cp
// CDP Accessibility Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/accessibility.ts
// Builds AXTree (Accessibility Tree) and handles CDP Accessibility protocol commands.

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { NodeId, NULL_NODE, NodeData, NodeType, Document } from "../dom/dom.cp";

pub const MAX_AX_NODES: usize = 256;
pub const MAX_CHILDREN_PER_AX_NODE: usize = 32;
pub const MAX_PROPERTIES_PER_AX_NODE: usize = 8;
pub const TEXT_POOL_SIZE: usize = 8192;
pub const ID_BUFFER_SIZE: usize = MAX_AX_NODES * 16;

pub enum AxValueKind {
    String,
    Number,
    Boolean,
}

pub struct AxValue {
    val_type: []const u8 = "string",
    str_val: []const u8 = "",
    num_val: i64 = 0,
    bool_val: bool = false,
    kind: AxValueKind = AxValueKind.String,

    static fn fromString(val_type: []const u8, s: []const u8): AxValue {
        return AxValue {
            val_type: val_type,
            str_val: s,
            kind: AxValueKind.String,
        };
    }

    static fn fromComputedString(s: []const u8): AxValue {
        return AxValue {
            val_type: "computedString",
            str_val: s,
            kind: AxValueKind.String,
        };
    }

    static fn fromRole(r: []const u8, is_internal: bool): AxValue {
        return AxValue {
            val_type: if (is_internal) "internalRole" else "role",
            str_val: r,
            kind: AxValueKind.String,
        };
    }

    static fn fromBool(val_type: []const u8, b: bool): AxValue {
        return AxValue {
            val_type: val_type,
            bool_val: b,
            kind: AxValueKind.Boolean,
        };
    }

    static fn fromInt(val_type: []const u8, n: i64): AxValue {
        return AxValue {
            val_type: val_type,
            num_val: n,
            kind: AxValueKind.Number,
        };
    }
}

pub struct AxProperty {
    name: []const u8 = "",
    value: AxValue = AxValue{},

    static fn create(name: []const u8, val: AxValue): AxProperty {
        return AxProperty {
            name: name,
            value: val,
        };
    }
}

pub struct AxNode {
    node_id: []const u8 = "",
    ignored: bool = false,
    role: AxValue? = null,
    name: AxValue? = null,
    description: AxValue? = null,
    value: AxValue? = null,
    backend_dom_node_id: NodeId = 0,
    parent_id: []const u8 = "",
    frame_id: []const u8 = "",
    child_ids: [MAX_CHILDREN_PER_AX_NODE][]const u8 = [""; MAX_CHILDREN_PER_AX_NODE],
    child_count: usize = 0,
    properties: [MAX_PROPERTIES_PER_AX_NODE]AxProperty = [AxProperty{}; MAX_PROPERTIES_PER_AX_NODE],
    prop_count: usize = 0,

    fn addChildId(*_, cid: []const u8) {
        if (_.child_count < MAX_CHILDREN_PER_AX_NODE) {
            _.child_ids[_.child_count] = cid;
            _.child_count += 1;
        }
    }

    fn addProperty(*_, prop: AxProperty) {
        if (_.prop_count < MAX_PROPERTIES_PER_AX_NODE) {
            _.properties[_.prop_count] = prop;
            _.prop_count += 1;
        }
    }
}

pub struct JsonWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    static fn init(buf: []u8): JsonWriter {
        return JsonWriter {
            buf: buf,
            pos: 0,
            overflow: false,
        };
    }

    fn writeByte(*_, b: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = b;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeRaw(*_, s: []const u8) {
        if (_.pos + s.len <= _.buf.len) {
            @memcpy(_.buf[_.pos .. _.pos + s.len], s);
            _.pos += s.len;
        } else {
            _.overflow = true;
        }
    }

    fn writeEscapedString(*_, s: []const u8) {
        _.writeByte('"');
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c == '"') {
                _.writeRaw("\\\"");
            } else if (c == '\\') {
                _.writeRaw("\\\\");
            } else if (c == '\n') {
                _.writeRaw("\\n");
            } else if (c == '\r') {
                _.writeRaw("\\r");
            } else if (c == '\t') {
                _.writeRaw("\\t");
            } else if (c < 0x20) {
                let hex_buf: [8]u8 = [0; 8];
                const hex_str = std.fmt.bufPrint(&hex_buf, "\\u00{x:0>2}", .{c}) catch "\\u0000";
                _.writeRaw(hex_str);
            } else {
                _.writeByte(c);
            }
        }
        _.writeByte('"');
    }

    fn writeInt(*_, n: i64) {
        let num_buf: [32]u8 = [0; 32];
        const num_str = std.fmt.bufPrint(&num_buf, "{d}", .{n}) catch "0";
        _.writeRaw(num_str);
    }

    fn writeBool(*_, b: bool) {
        if (b) {
            _.writeRaw("true");
        } else {
            _.writeRaw("false");
        }
    }

    fn writeAxValue(*_, val: AxValue) {
        _.writeRaw("{\"type\":");
        _.writeEscapedString(val.val_type);
        _.writeRaw(",\"value\":");
        if (val.kind == AxValueKind.Boolean) {
            _.writeBool(val.bool_val);
        } else if (val.kind == AxValueKind.Number) {
            _.writeInt(val.num_val);
        } else {
            _.writeEscapedString(val.str_val);
        }
        _.writeByte('}');
    }

    fn writeAxProperty(*_, prop: AxProperty) {
        _.writeRaw("{\"name\":");
        _.writeEscapedString(prop.name);
        _.writeRaw(",\"value\":");
        _.writeAxValue(prop.value);
        _.writeByte('}');
    }

    fn writeAxNode(*_, node: *const AxNode) {
        _.writeRaw("{\"nodeId\":");
        _.writeEscapedString(node.node_id);
        _.writeRaw(",\"ignored\":");
        _.writeBool(node.ignored);

        if (node.ignored) {
            _.writeRaw(",\"ignoredReasons\":[{\"name\":\"notRendered\",\"value\":{\"type\":\"boolean\",\"value\":true}}]");
            _.writeRaw(",\"role\":{\"type\":\"internalRole\",\"value\":\"none\"}");
        } else {
            if (r := node.role) {
                _.writeRaw(",\"role\":");
                _.writeAxValue(r);
            }
            if (n := node.name) {
                _.writeRaw(",\"name\":");
                _.writeAxValue(n);
            }
            if (d := node.description) {
                _.writeRaw(",\"description\":");
                _.writeAxValue(d);
            }
            if (v := node.value) {
                _.writeRaw(",\"value\":");
                _.writeAxValue(v);
            }
            if (node.prop_count > 0) {
                _.writeRaw(",\"properties\":[");
                let i: usize = 0;
                while (i < node.prop_count) : (i += 1) {
                    if (i > 0) _.writeByte(',');
                    _.writeAxProperty(node.properties[i]);
                }
                _.writeByte(']');
            }
        }

        if (node.child_count > 0) {
            _.writeRaw(",\"childIds\":[");
            let i: usize = 0;
            while (i < node.child_count) : (i += 1) {
                if (i > 0) _.writeByte(',');
                _.writeEscapedString(node.child_ids[i]);
            }
            _.writeByte(']');
        }

        if (node.parent_id.len > 0) {
            _.writeRaw(",\"parentId\":");
            _.writeEscapedString(node.parent_id);
        }

        _.writeRaw(",\"backendDOMNodeId\":");
        _.writeInt(@as(i64, @intCast(node.backend_dom_node_id)));

        if (node.frame_id.len > 0) {
            _.writeRaw(",\"frameId\":");
            _.writeEscapedString(node.frame_id);
        }

        _.writeByte('}');
    }

    fn getWritten(): []const u8 {
        return _.buf[0.._.pos];
    }
}

fn isSkipTag(tag: []const u8): bool {
    return std.ascii.eqlIgnoreCase(tag, "script") ||
        std.ascii.eqlIgnoreCase(tag, "style") ||
        std.ascii.eqlIgnoreCase(tag, "noscript") ||
        std.ascii.eqlIgnoreCase(tag, "template") ||
        std.ascii.eqlIgnoreCase(tag, "meta") ||
        std.ascii.eqlIgnoreCase(tag, "link") ||
        std.ascii.eqlIgnoreCase(tag, "base") ||
        std.ascii.eqlIgnoreCase(tag, "head") ||
        std.ascii.eqlIgnoreCase(tag, "title");
}

fn inlineStyleHidden(style: []const u8): bool {
    let it = std.mem.splitScalar(u8, style, ';');
    while (decl := it.next()) {
        const colon_opt = std.mem.indexOfScalar(u8, decl, ':');
        if (colon_opt == null) continue;
        const colon = colon_opt.?;
        if (colon == 0) continue;
        const prop = std.mem.trim(u8, decl[0..colon], " \t\r\n");
        const val = std.mem.trim(u8, decl[colon + 1 ..], " \t\r\n");
        if (std.ascii.eqlIgnoreCase(prop, "display")) {
            if (std.ascii.startsWithIgnoreCase(val, "none")) {
                return true;
            }
        }
        if (std.ascii.eqlIgnoreCase(prop, "visibility")) {
            if (std.ascii.startsWithIgnoreCase(val, "hidden")) {
                return true;
            }
        }
    }
    return false;
}

fn isIgnored(node: *const NodeData): bool {
    if (std.ascii.eqlIgnoreCase(node.tag_name, "html")) {
        return true;
    }
    if (role_attr := node.getAttribute("role")) {
        const first_tok = getFirstToken(role_attr);
        if (std.ascii.eqlIgnoreCase(first_tok, "presentation") ||
            std.ascii.eqlIgnoreCase(first_tok, "none")) {
            return true;
        }
    }
    if (ah := node.getAttribute("aria-hidden")) {
        if (std.ascii.eqlIgnoreCase(ah, "true")) {
            return true;
        }
    }
    if (node.getAttribute("hidden") != null) {
        return true;
    }
    if (node.inline_style.len > 0) {
        if (inlineStyleHidden(node.inline_style)) {
            return true;
        }
    }
    return false;
}

fn getFirstToken(s: []const u8): []const u8 {
    let it = std.mem.tokenizeAny(u8, s, " \t\r\n");
    if (tok := it.next()) {
        return tok;
    }
    return "";
}

fn getExplicitRole(node: *const NodeData): ([]const u8)? {
    const role_opt = node.getAttribute("role");
    if (role_opt != null) {
        const tok = getFirstToken(role_opt.?);
        if (tok.len > 0) return tok;
    }
    return null;
}

fn computeImplicitRole(tag: []const u8, node: *const NodeData): []const u8 {
    if (std.ascii.eqlIgnoreCase(tag, "button")) return "button";
    if (std.ascii.eqlIgnoreCase(tag, "a") || std.ascii.eqlIgnoreCase(tag, "area")) {
        return if (node.getAttribute("href") != null) "link" else "generic";
    }
    if (tag.len == 2 && (tag[0] == 'h' || tag[0] == 'H') && tag[1] >= '1' && tag[1] <= '6') {
        return "heading";
    }
    if (std.ascii.eqlIgnoreCase(tag, "input")) {
        const typ = node.getAttribute("type") ?? "text";
        if (std.ascii.eqlIgnoreCase(typ, "button") ||
            std.ascii.eqlIgnoreCase(typ, "submit") ||
            std.ascii.eqlIgnoreCase(typ, "reset") ||
            std.ascii.eqlIgnoreCase(typ, "image")) {
            return "button";
        }
        if (std.ascii.eqlIgnoreCase(typ, "checkbox")) return "checkbox";
        if (std.ascii.eqlIgnoreCase(typ, "radio")) return "radio";
        if (std.ascii.eqlIgnoreCase(typ, "range")) return "slider";
        if (std.ascii.eqlIgnoreCase(typ, "number")) return "spinbutton";
        if (std.ascii.eqlIgnoreCase(typ, "search")) return "searchbox";
        return "textbox";
    }
    if (std.ascii.eqlIgnoreCase(tag, "img")) {
        if (alt := node.getAttribute("alt")) {
            if (alt.len == 0) return "presentation";
        }
        return "image";
    }
    if (std.ascii.eqlIgnoreCase(tag, "form")) return "form";
    if (std.ascii.eqlIgnoreCase(tag, "select")) {
        if (node.getAttribute("multiple") != null) return "listbox";
        return "combobox";
    }
    if (std.ascii.eqlIgnoreCase(tag, "textarea")) return "textbox";
    if (std.ascii.eqlIgnoreCase(tag, "ul") ||
        std.ascii.eqlIgnoreCase(tag, "ol") ||
        std.ascii.eqlIgnoreCase(tag, "menu")) {
        return "list";
    }
    if (std.ascii.eqlIgnoreCase(tag, "li")) return "listitem";
    if (std.ascii.eqlIgnoreCase(tag, "table")) return "table";
    if (std.ascii.eqlIgnoreCase(tag, "tr")) return "row";
    if (std.ascii.eqlIgnoreCase(tag, "td")) return "cell";
    if (std.ascii.eqlIgnoreCase(tag, "th")) {
        if (sc := node.getAttribute("scope")) {
            if (std.ascii.eqlIgnoreCase(sc, "row")) return "rowheader";
        }
        return "columnheader";
    }
    if (std.ascii.eqlIgnoreCase(tag, "thead") ||
        std.ascii.eqlIgnoreCase(tag, "tbody") ||
        std.ascii.eqlIgnoreCase(tag, "tfoot")) {
        return "rowgroup";
    }
    if (std.ascii.eqlIgnoreCase(tag, "nav")) return "navigation";
    if (std.ascii.eqlIgnoreCase(tag, "header")) return "banner";
    if (std.ascii.eqlIgnoreCase(tag, "footer")) return "contentinfo";
    if (std.ascii.eqlIgnoreCase(tag, "main")) return "main";
    if (std.ascii.eqlIgnoreCase(tag, "aside")) return "complementary";
    if (std.ascii.eqlIgnoreCase(tag, "section")) {
        if (node.getAttribute("aria-label") != null || node.getAttribute("aria-labelledby") != null) {
            return "region";
        }
        return "generic";
    }
    if (std.ascii.eqlIgnoreCase(tag, "article")) return "article";
    if (std.ascii.eqlIgnoreCase(tag, "p")) return "paragraph";
    if (std.ascii.eqlIgnoreCase(tag, "hr")) return "separator";
    if (std.ascii.eqlIgnoreCase(tag, "dialog")) return "dialog";
    if (std.ascii.eqlIgnoreCase(tag, "summary")) return "button";
    if (std.ascii.eqlIgnoreCase(tag, "figure")) return "figure";
    if (std.ascii.eqlIgnoreCase(tag, "caption") || std.ascii.eqlIgnoreCase(tag, "figcaption")) return "caption";
    if (std.ascii.eqlIgnoreCase(tag, "blockquote")) return "blockquote";
    if (std.ascii.eqlIgnoreCase(tag, "code")) return "code";
    if (std.ascii.eqlIgnoreCase(tag, "time")) return "time";
    if (std.ascii.eqlIgnoreCase(tag, "strong")) return "strong";
    if (std.ascii.eqlIgnoreCase(tag, "em")) return "emphasis";
    if (std.ascii.eqlIgnoreCase(tag, "sub")) return "subscript";
    if (std.ascii.eqlIgnoreCase(tag, "sup")) return "superscript";
    if (std.ascii.eqlIgnoreCase(tag, "mark")) return "mark";
    if (std.ascii.eqlIgnoreCase(tag, "del")) return "deletion";
    if (std.ascii.eqlIgnoreCase(tag, "ins")) return "insertion";
    if (std.ascii.eqlIgnoreCase(tag, "details") || std.ascii.eqlIgnoreCase(tag, "fieldset")) return "group";
    if (std.ascii.eqlIgnoreCase(tag, "svg")) return "graphics-document";
    if (std.ascii.eqlIgnoreCase(tag, "html")) return "document";
    if (std.ascii.eqlIgnoreCase(tag, "body")) return "generic";
    return "generic";
}

fn isFocusable(tag: []const u8, node: *const NodeData): bool {
    if (std.ascii.eqlIgnoreCase(tag, "button") ||
        std.ascii.eqlIgnoreCase(tag, "select") ||
        std.ascii.eqlIgnoreCase(tag, "textarea") ||
        std.ascii.eqlIgnoreCase(tag, "summary")) {
        return true;
    }
    if (std.ascii.eqlIgnoreCase(tag, "input")) {
        const typ = node.getAttribute("type") ?? "text";
        return !std.ascii.eqlIgnoreCase(typ, "hidden");
    }
    if (std.ascii.eqlIgnoreCase(tag, "a") || std.ascii.eqlIgnoreCase(tag, "area")) {
        return node.getAttribute("href") != null;
    }
    if (node.getAttribute("tabindex") != null) {
        return true;
    }
    if (ce := node.getAttribute("contenteditable")) {
        return !std.ascii.eqlIgnoreCase(ce, "false");
    }
    return false;
}

fn collectSubtreeTextInternal(
    doc: *const Document,
    node_id: NodeId,
    out: []u8,
    depth: usize,
): usize {
    if (depth > 16 || out.len == 0) return 0;
    let offset: usize = 0;
    let curr = doc.getFirstChild(node_id);
    while (curr != NULL_NODE && offset < out.len) {
        if (n := doc.getNode(curr)) {
            if (n.node_type == NodeType.Text) {
                const txt = std.mem.trim(u8, n.text_content, " \t\r\n");
                if (txt.len > 0) {
                    if (offset > 0 && offset < out.len && out[offset - 1] != ' ') {
                        out[offset] = ' ';
                        offset += 1;
                    }
                    const copy_len = @min(txt.len, out.len - offset);
                    @memcpy(out[offset .. offset + copy_len], txt[0..copy_len]);
                    offset += copy_len;
                }
            } else if (n.node_type == NodeType.Element) {
                if (!isSkipTag(n.tag_name)) {
                    const sub_len = collectSubtreeTextInternal(doc, curr, out[offset..], depth + 1);
                    offset += sub_len;
                }
            }
            curr = n.next_sibling;
        } else {
            break;
        }
    }
    return offset;
}

pub struct AxTreeBuilder {
    nodes: [MAX_AX_NODES]AxNode = [AxNode{}; MAX_AX_NODES],
    node_count: usize = 0,
    id_buffers: [ID_BUFFER_SIZE]u8 = [0; ID_BUFFER_SIZE],
    text_pool: [TEXT_POOL_SIZE]u8 = [0; TEXT_POOL_SIZE],
    text_pool_len: usize = 0,

    fn allocNode(*_): usize? {
        if (_.node_count >= MAX_AX_NODES) return null;
        const idx = _.node_count;
        _.node_count += 1;
        _.nodes[idx] = AxNode{};
        return idx;
    }

    fn getIdBuffer(*_, ax_idx: usize): []u8 {
        const start = ax_idx * 16;
        return _.id_buffers[start .. start + 16];
    }

    fn collectSubtreeText(*_, doc: *const Document, dom_id: NodeId): []const u8 {
        const remain = TEXT_POOL_SIZE - _.text_pool_len;
        if (remain < 16) return "";
        const out_slice = _.text_pool[_.text_pool_len..];
        const written = collectSubtreeTextInternal(doc, dom_id, out_slice, 0);
        const result = _.text_pool[_.text_pool_len .. _.text_pool_len + written];
        _.text_pool_len += written;
        return result;
    }

    fn findLabelFor(*_, doc: *const Document, id_val: []const u8): ([]const u8)? {
        if (id_val.len == 0) return null;
        let i: usize = 1;
        while (i < doc.nodeCount()) : (i += 1) {
            if (n := doc.getNode(@intCast(i))) {
                if (n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, "label")) {
                    if (for_attr := n.getAttribute("for")) {
                        if (std.mem.eql(u8, for_attr, id_val)) {
                            return _.collectSubtreeText(doc, @intCast(i));
                        }
                    }
                }
            }
        }
        return null;
    }

    fn findWrappingLabel(*_, doc: *const Document, dom_id: NodeId): ([]const u8)? {
        let curr = doc.getParent(dom_id);
        let depth: usize = 0;
        while (curr != NULL_NODE && depth < 8) : (depth += 1) {
            if (n := doc.getNode(curr)) {
                if (n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, "label")) {
                    return _.collectSubtreeText(doc, curr);
                }
                curr = n.parent;
            } else {
                break;
            }
        }
        return null;
    }

    fn findChildTextByTag(*_, doc: *const Document, parent_id: NodeId, tag: []const u8): ([]const u8)? {
        let curr = doc.getFirstChild(parent_id);
        while (curr != NULL_NODE) {
            if (n := doc.getNode(curr)) {
                if (n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, tag)) {
                    return _.collectSubtreeText(doc, curr);
                }
                curr = n.next_sibling;
            } else {
                break;
            }
        }
        return null;
    }

    fn findSelectedOptionText(*_, doc: *const Document, select_id: NodeId): ([]const u8)? {
        let first_opt: NodeId = NULL_NODE;
        let selected_opt: NodeId = NULL_NODE;
        let curr = doc.getFirstChild(select_id);
        while (curr != NULL_NODE) {
            if (n := doc.getNode(curr)) {
                if (n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, "option")) {
                    if (first_opt == NULL_NODE) first_opt = curr;
                    if (n.getAttribute("selected") != null) {
                        selected_opt = curr;
                    }
                }
                curr = n.next_sibling;
            } else {
                break;
            }
        }
        const chosen = if (selected_opt != NULL_NODE) selected_opt else first_opt;
        if (chosen != NULL_NODE) {
            if (opt_node := doc.getNode(chosen)) {
                if (lbl := opt_node.getAttribute("label")) return lbl;
                if (val := opt_node.getAttribute("value")) return val;
                return _.collectSubtreeText(doc, chosen);
            }
        }
        return null;
    }

    fn computeName(*_, doc: *const Document, node: *const NodeData, role: []const u8): []const u8 {
        // 1. aria-labelledby
        if (labelled_by := node.getAttribute("aria-labelledby")) {
            let it = std.mem.tokenizeAny(u8, labelled_by, " \t\r\n");
            let found_any = false;
            const start_pos = _.text_pool_len;
            while (id_tok := it.next()) {
                const target_id = doc.getElementById(id_tok);
                if (target_id != NULL_NODE) {
                    const txt = _.collectSubtreeText(doc, target_id);
                    if (txt.len > 0) {
                        found_any = true;
                    }
                }
            }
            if (found_any) {
                return _.text_pool[start_pos .. _.text_pool_len];
            }
        }

        // 2. aria-label
        if (aria_label := node.getAttribute("aria-label")) {
            const trimmed = std.mem.trim(u8, aria_label, " \t\r\n");
            if (trimmed.len > 0) return trimmed;
        }

        // 3. Tag-specific name calculation
        const tag = node.tag_name;
        if (std.ascii.eqlIgnoreCase(tag, "img") || std.ascii.eqlIgnoreCase(tag, "area")) {
            if (alt := node.getAttribute("alt")) return alt;
        }

        if (std.ascii.eqlIgnoreCase(tag, "input")) {
            const typ = node.getAttribute("type") ?? "text";
            if (std.ascii.eqlIgnoreCase(typ, "button") ||
                std.ascii.eqlIgnoreCase(typ, "submit") ||
                std.ascii.eqlIgnoreCase(typ, "reset")) {
                if (v := node.getAttribute("value")) return v;
                if (std.ascii.eqlIgnoreCase(typ, "submit")) return "Submit";
                if (std.ascii.eqlIgnoreCase(typ, "reset")) return "Reset";
            }
            if (std.ascii.eqlIgnoreCase(typ, "image")) {
                if (alt := node.getAttribute("alt")) return alt;
            }
            if (id_val := node.getAttribute("id")) {
                if (lbl := _.findLabelFor(doc, id_val)) return lbl;
            }
            if (lbl := _.findWrappingLabel(doc, node.id)) return lbl;
            if (ph := node.getAttribute("placeholder")) return ph;
        }

        if (std.ascii.eqlIgnoreCase(tag, "textarea")) {
            if (id_val := node.getAttribute("id")) {
                if (lbl := _.findLabelFor(doc, id_val)) return lbl;
            }
            if (lbl := _.findWrappingLabel(doc, node.id)) return lbl;
            if (ph := node.getAttribute("placeholder")) return ph;
        }

        if (std.ascii.eqlIgnoreCase(tag, "select")) {
            if (id_val := node.getAttribute("id")) {
                if (lbl := _.findLabelFor(doc, id_val)) return lbl;
            }
            if (lbl := _.findWrappingLabel(doc, node.id)) return lbl;
        }

        if (std.ascii.eqlIgnoreCase(tag, "fieldset")) {
            if (legend := _.findChildTextByTag(doc, node.id, "legend")) return legend;
        }

        if (std.ascii.eqlIgnoreCase(tag, "table")) {
            if (caption := _.findChildTextByTag(doc, node.id, "caption")) return caption;
        }

        if (std.ascii.eqlIgnoreCase(tag, "figure")) {
            if (figcap := _.findChildTextByTag(doc, node.id, "figcaption")) return figcap;
        }

        if (std.ascii.eqlIgnoreCase(tag, "svg")) {
            if (title_text := _.findChildTextByTag(doc, node.id, "title")) return title_text;
        }

        // 4. Content naming tags
        if (std.ascii.eqlIgnoreCase(tag, "button") ||
            std.ascii.eqlIgnoreCase(tag, "a") ||
            std.ascii.eqlIgnoreCase(tag, "summary") ||
            std.ascii.eqlIgnoreCase(tag, "li") ||
            std.ascii.eqlIgnoreCase(tag, "dt") ||
            std.ascii.eqlIgnoreCase(tag, "th") ||
            std.ascii.eqlIgnoreCase(tag, "td") ||
            std.ascii.eqlIgnoreCase(tag, "caption") ||
            std.ascii.eqlIgnoreCase(tag, "p") ||
            std.ascii.eqlIgnoreCase(tag, "option") ||
            std.ascii.eqlIgnoreCase(tag, "em") ||
            std.ascii.eqlIgnoreCase(tag, "strong") ||
            std.ascii.eqlIgnoreCase(tag, "code") ||
            (tag.len == 2 && (tag[0] == 'h' || tag[0] == 'H') && tag[1] >= '1' && tag[1] <= '6') ||
            std.mem.eql(u8, role, "button") ||
            std.mem.eql(u8, role, "link") ||
            std.mem.eql(u8, role, "heading")) {
            const txt = _.collectSubtreeText(doc, node.id);
            if (txt.len > 0) return txt;
        }

        // 5. Title fallback
        if (title := node.getAttribute("title")) return title;
        return "";
    }

    fn computeDescription(*_, doc: *const Document, node: *const NodeData): []const u8 {
        if (described_by := node.getAttribute("aria-describedby")) {
            let it = std.mem.tokenizeAny(u8, described_by, " \t\r\n");
            let found_any = false;
            const start_pos = _.text_pool_len;
            while (id_tok := it.next()) {
                const target_id = doc.getElementById(id_tok);
                if (target_id != NULL_NODE) {
                    const txt = _.collectSubtreeText(doc, target_id);
                    if (txt.len > 0) {
                        found_any = true;
                    }
                }
            }
            if (found_any) {
                return _.text_pool[start_pos .. _.text_pool_len];
            }
        }
        if (aria_desc := node.getAttribute("aria-description")) {
            return std.mem.trim(u8, aria_desc, " \t\r\n");
        }
        return "";
    }

    fn computeValue(*_, doc: *const Document, node: *const NodeData): ([]const u8)? {
        if (vn := node.getAttribute("aria-valuenow")) return vn;
        if (std.ascii.eqlIgnoreCase(node.tag_name, "input")) {
            if (v := node.getAttribute("value")) return v;
        }
        if (std.ascii.eqlIgnoreCase(node.tag_name, "textarea")) {
            if (v := node.getAttribute("value")) return v;
            const txt = _.collectSubtreeText(doc, node.id);
            if (txt.len > 0) return txt;
        }
        if (std.ascii.eqlIgnoreCase(node.tag_name, "select")) {
            return _.findSelectedOptionText(doc, node.id);
        }
        if (vt := node.getAttribute("aria-valuetext")) return vt;
        return null;
    }

    fn populateProperties(*_, ax_idx: usize, node: *const NodeData, role: []const u8) {
        let node_ref = &_.nodes[ax_idx];

        if (node.tag_name.len == 2 && (node.tag_name[0] == 'h' || node.tag_name[0] == 'H')) {
            const c = node.tag_name[1];
            if (c >= '1' && c <= '6') {
                node_ref.addProperty(AxProperty.create("level", AxValue.fromInt("integer", @as(i64, c - '0'))));
            }
        }
        if (al := node.getAttribute("aria-level")) {
            const level_num = std.fmt.parseInt(i64, std.mem.trim(u8, al, " \t\r\n"), 10) catch 0;
            if (level_num > 0) {
                node_ref.addProperty(AxProperty.create("level", AxValue.fromInt("integer", level_num)));
            }
        }

        if (std.mem.eql(u8, role, "link")) {
            if (href := node.getAttribute("href")) {
                node_ref.addProperty(AxProperty.create("url", AxValue.fromString("string", href)));
            }
        } else if (std.ascii.eqlIgnoreCase(node.tag_name, "img")) {
            if (src := node.getAttribute("src")) {
                node_ref.addProperty(AxProperty.create("url", AxValue.fromString("string", src)));
            }
        }

        if (isFocusable(node.tag_name, node)) {
            node_ref.addProperty(AxProperty.create("focusable", AxValue.fromBool("booleanOrUndefined", true)));
        }

        if (node.getAttribute("disabled") != null) {
            node_ref.addProperty(AxProperty.create("disabled", AxValue.fromBool("boolean", true)));
        }

        if (node.getAttribute("checked") != null) {
            node_ref.addProperty(AxProperty.create("checked", AxValue.fromBool("boolean", true)));
        }

        if (node.getAttribute("required") != null) {
            node_ref.addProperty(AxProperty.create("required", AxValue.fromBool("boolean", true)));
        }

        if (node.getAttribute("readonly") != null) {
            node_ref.addProperty(AxProperty.create("readonly", AxValue.fromBool("boolean", true)));
        }
    }

    fn buildNode(
        *_,
        doc: *const Document,
        dom_id: NodeId,
        parent_ax_idx: usize,
        frame_id: []const u8,
    ): usize? {
        if (dom_id == NULL_NODE) return null;
        const opt = doc.getNode(dom_id);
        if (opt == null) return null;
        const node = opt.?;

        if (node.node_type == NodeType.Text) {
            const txt = std.mem.trim(u8, node.text_content, " \t\r\n");
            if (txt.len == 0) return null;
            const ax_opt = _.allocNode();
            if (ax_opt == null) return null;
            const ax_idx = ax_opt.?;

            const id_str = std.fmt.bufPrint(_.getIdBuffer(ax_idx), "{d}", .{dom_id}) catch "0";
            _.nodes[ax_idx].node_id = id_str;
            _.nodes[ax_idx].backend_dom_node_id = dom_id;
            _.nodes[ax_idx].frame_id = frame_id;
            _.nodes[ax_idx].role = AxValue.fromRole("StaticText", true);
            _.nodes[ax_idx].name = AxValue.fromComputedString(txt);
            _.nodes[ax_idx].parent_id = _.nodes[parent_ax_idx].node_id;

            _.nodes[parent_ax_idx].addChildId(id_str);
            return ax_idx;
        }

        if (node.node_type == NodeType.Element) {
            if (isSkipTag(node.tag_name)) return null;

            const ax_opt = _.allocNode();
            if (ax_opt == null) return null;
            const ax_idx = ax_opt.?;

            const id_str = std.fmt.bufPrint(_.getIdBuffer(ax_idx), "{d}", .{dom_id}) catch "0";
            _.nodes[ax_idx].node_id = id_str;
            _.nodes[ax_idx].backend_dom_node_id = dom_id;
            _.nodes[ax_idx].frame_id = frame_id;
            _.nodes[ax_idx].parent_id = _.nodes[parent_ax_idx].node_id;

            _.nodes[parent_ax_idx].addChildId(id_str);

            const ignored = isIgnored(node);
            _.nodes[ax_idx].ignored = ignored;

            if (ignored) {
                _.nodes[ax_idx].role = AxValue.fromRole("none", true);
            } else {
                const explicit_role = getExplicitRole(node);
                const role_str = if (explicit_role != null) explicit_role.? else computeImplicitRole(node.tag_name, node);
                _.nodes[ax_idx].role = AxValue.fromRole(role_str, explicit_role == null);

                const name_str = _.computeName(doc, node, role_str);
                if (name_str.len > 0) {
                    _.nodes[ax_idx].name = AxValue.fromComputedString(name_str);
                }

                const desc_str = _.computeDescription(doc, node);
                if (desc_str.len > 0) {
                    _.nodes[ax_idx].description = AxValue.fromComputedString(desc_str);
                }

                if (val_str := _.computeValue(doc, node)) {
                    _.nodes[ax_idx].value = AxValue.fromString("string", val_str);
                }

                _.populateProperties(ax_idx, node, role_str);
            }

            // Recurse children
            let child_id = node.first_child;
            while (child_id != NULL_NODE) {
                if (c_node := doc.getNode(child_id)) {
                    const next_sib = c_node.next_sibling;
                    * = _.buildNode(doc, child_id, ax_idx, frame_id);
                    child_id = next_sib;
                } else {
                    break;
                }
            }

            return ax_idx;
        }

        return null;
    }

    fn build(*_, session: *CdpPageSession) {
        _.node_count = 0;
        _.text_pool_len = 0;

        // Allocate node 0: RootWebArea
        const root_ax_opt = _.allocNode();
        if (root_ax_opt == null) return;
        const root_idx = root_ax_opt.?;

        _.nodes[root_idx].node_id = "root";
        _.nodes[root_idx].ignored = false;
        _.nodes[root_idx].role = AxValue.fromRole("RootWebArea", true);
        _.nodes[root_idx].name = AxValue.fromComputedString(session.title);
        _.nodes[root_idx].value = AxValue.fromString("string", session.url);
        _.nodes[root_idx].backend_dom_node_id = session.doc.root_element;
        _.nodes[root_idx].frame_id = session.frame_id;

        _.nodes[root_idx].addProperty(AxProperty.create("url", AxValue.fromString("string", session.url)));
        _.nodes[root_idx].addProperty(AxProperty.create("focusable", AxValue.fromBool("booleanOrUndefined", true)));

        if (session.doc.root_element != NULL_NODE) {
            * = _.buildNode(&session.doc, session.doc.root_element, root_idx, session.frame_id);
        }
    }
}

pub struct AccessibilityHandler {
    static fn handle(
        session: *CdpPageSession,
        req: CdpRequest,
        out_buf: []u8,
        out_events: *[1]CdpEvent,
        event_count: *usize,
    ): CdpResponse {
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "Accessibility.disable")) {
            return CdpResponse.ok(req.id, "{}");
        }

        let builder = std.heap.c_allocator.create(AxTreeBuilder) catch {
            return CdpResponse.err(req.id, -32000, "Out of memory");
        };
        defer std.heap.c_allocator.destroy(builder);

        builder.build(session);

        if (std.mem.eql(u8, req.method, "Accessibility.enable")) {
            if (builder.node_count > 0) {
                let evt_buf: [2048]u8 = [0; 2048];
                let ew = JsonWriter.init(&evt_buf);
                ew.writeRaw("{\"root\":");
                ew.writeAxNode(&builder.nodes[0]);
                ew.writeByte('}');
                if (!ew.overflow) {
                    out_events[0] = CdpEvent.create("Accessibility.loadComplete", ew.getWritten());
                    event_count.* = 1;
                }
            }
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Accessibility.getRootAXNode")) {
            if (builder.node_count == 0) {
                return CdpResponse.err(req.id, -32000, "No document loaded");
            }
            let w = JsonWriter.init(out_buf);
            w.writeRaw("{\"node\":");
            w.writeAxNode(&builder.nodes[0]);
            w.writeByte('}');
            if (w.overflow) {
                return CdpResponse.err(req.id, -32000, "Buffer overflow");
            }
            return CdpResponse.ok(req.id, w.getWritten());
        }

        if (std.mem.eql(u8, req.method, "Accessibility.getFullAXTree")) {
            let w = JsonWriter.init(out_buf);
            w.writeRaw("{\"nodes\":[");
            let i: usize = 0;
            while (i < builder.node_count) : (i += 1) {
                if (i > 0) w.writeByte(',');
                w.writeAxNode(&builder.nodes[i]);
            }
            w.writeRaw("]}");
            if (w.overflow) {
                return CdpResponse.err(req.id, -32000, "Buffer overflow");
            }
            return CdpResponse.ok(req.id, w.getWritten());
        }

        if (std.mem.eql(u8, req.method, "Accessibility.getChildAXNodes")) {
            const id_str = extractJsonString(req.params_raw, "id") ?? "";
            let w = JsonWriter.init(out_buf);
            w.writeRaw("{\"nodes\":[");
            let parent_idx: usize? = null;
            let i: usize = 0;
            while (i < builder.node_count) : (i += 1) {
                if (std.mem.eql(u8, builder.nodes[i].node_id, id_str)) {
                    parent_idx = i;
                    break;
                }
            }
            if (parent_idx != null) {
                const parent = &builder.nodes[parent_idx.?];
                let c_idx: usize = 0;
                let written_count: usize = 0;
                while (c_idx < parent.child_count) : (c_idx += 1) {
                    const cid = parent.child_ids[c_idx];
                    let j: usize = 0;
                    while (j < builder.node_count) : (j += 1) {
                        if (std.mem.eql(u8, builder.nodes[j].node_id, cid)) {
                            if (written_count > 0) w.writeByte(',');
                            w.writeAxNode(&builder.nodes[j]);
                            written_count += 1;
                            break;
                        }
                    }
                }
            }
            w.writeRaw("]}");
            if (w.overflow) {
                return CdpResponse.err(req.id, -32000, "Buffer overflow");
            }
            return CdpResponse.ok(req.id, w.getWritten());
        }

        if (std.mem.eql(u8, req.method, "Accessibility.queryAXTree")) {
            const want_role = extractJsonString(req.params_raw, "role");
            const want_name = extractJsonString(req.params_raw, "accessibleName");
            let w = JsonWriter.init(out_buf);
            w.writeRaw("{\"nodes\":[");
            let matches: usize = 0;
            let i: usize = 0;
            while (i < builder.node_count) : (i += 1) {
                const n = &builder.nodes[i];
                if (n.ignored) continue;
                if (wr := want_role) {
                    if (r := n.role) {
                        if (!std.mem.eql(u8, r.str_val, wr)) continue;
                    } else {
                        continue;
                    }
                }
                if (wn := want_name) {
                    if (nm := n.name) {
                        if (!std.mem.eql(u8, nm.str_val, wn)) continue;
                    } else {
                        continue;
                    }
                }
                if (matches > 0) w.writeByte(',');
                w.writeAxNode(n);
                matches += 1;
            }
            w.writeRaw("]}");
            if (w.overflow) {
                return CdpResponse.err(req.id, -32000, "Buffer overflow");
            }
            return CdpResponse.ok(req.id, w.getWritten());
        }

        if (std.mem.eql(u8, req.method, "Accessibility.getPartialAXTree") ||
            std.mem.eql(u8, req.method, "Accessibility.getAXNodeAndAncestors")) {
            const res = std.fmt.bufPrint(out_buf, "{{\"nodes\":[]}}", .{}) catch {
                return CdpResponse.err(req.id, -32000, "Buffer overflow");
            };
            return CdpResponse.ok(req.id, res);
        }

        return CdpResponse.ok(req.id, "{}");
    }
}
