// Migrated from vendor/kitesurf/packages/engine/src/cdp/accessibility.ts
// src/cdp/accessibility.cp
// CDP Accessibility Domain Handler for CapSurf
// Builds AXTree (Accessibility Tree) and handles CDP Accessibility protocol commands.

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt, extractJsonBool } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { NodeId, NULL_NODE, NodeData, NodeType, Document } from "../dom/dom.cp";
import { resolveUrl } from "../eval/script_gate.cp";

pub const MAX_AX_NODES: usize = 256;
pub const MAX_CHILDREN_PER_AX_NODE: usize = 32;
pub const MAX_PROPERTIES_PER_AX_NODE: usize = 16;
pub const TEXT_POOL_SIZE: usize = 32768;
pub const ID_BUFFER_SIZE: usize = MAX_AX_NODES * 16;

@tls let ax_event_buf: [4096]u8 = [0; 4096];

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

pub struct AXProperty {
    name: []const u8 = "",
    value: AxValue = AxValue{},

    static fn create(name: []const u8, val: AxValue): AXProperty {
        return AXProperty {
            name: name,
            value: val,
        };
    }
}
pub const AxProperty = AXProperty;

pub struct AXNode {
    node_id: []const u8 = "",
    ignored: bool = false,
    ignored_reason: []const u8 = "notRendered",
    role: AxValue? = null,
    chrome_role: i64 = 0,
    name: AxValue? = null,
    description: AxValue? = null,
    value: AxValue? = null,
    backend_dom_node_id: NodeId = 0,
    parent_id: []const u8 = "",
    frame_id: []const u8 = "",
    child_ids: [MAX_CHILDREN_PER_AX_NODE][]const u8 = [""; MAX_CHILDREN_PER_AX_NODE],
    child_count: usize = 0,
    properties: [MAX_PROPERTIES_PER_AX_NODE]AXProperty = [AXProperty{}; MAX_PROPERTIES_PER_AX_NODE],
    prop_count: usize = 0,

    fn addChildId(*_, cid: []const u8) {
        if (_.child_count < MAX_CHILDREN_PER_AX_NODE) {
            _.child_ids[_.child_count] = cid;
            _.child_count += 1;
        }
    }

    fn addProperty(*_, prop: AXProperty) {
        if (_.prop_count < MAX_PROPERTIES_PER_AX_NODE) {
            _.properties[_.prop_count] = prop;
            _.prop_count += 1;
        }
    }
}
pub const AxNode = AXNode;

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

    fn writeAxProperty(*_, prop: AXProperty) {
        _.writeRaw("{\"name\":");
        _.writeEscapedString(prop.name);
        _.writeRaw(",\"value\":");
        _.writeAxValue(prop.value);
        _.writeByte('}');
    }

    fn writeAxNode(*_, node: *const AXNode) {
        _.writeRaw("{\"nodeId\":");
        _.writeEscapedString(node.node_id);
        _.writeRaw(",\"ignored\":");
        _.writeBool(node.ignored);

        if (node.ignored) {
            _.writeRaw(",\"ignoredReasons\":[{\"name\":");
            _.writeEscapedString(node.ignored_reason);
            _.writeRaw(",\"value\":{\"type\":\"boolean\",\"value\":true}}]");
            _.writeRaw(",\"role\":{\"type\":\"internalRole\",\"value\":\"none\"}");
        } else {
            if (r := node.role) {
                _.writeRaw(",\"role\":");
                _.writeAxValue(r);
            }
            if (node.chrome_role > 0) {
                _.writeRaw(",\"chromeRole\":{\"type\":\"internalRole\",\"value\":");
                _.writeInt(node.chrome_role);
                _.writeByte('}');
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

fn inlineStyleDisplayNone(style: []const u8): bool {
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
    }
    return false;
}

fn inlineStyleVisibilityHidden(style: []const u8): bool {
    let it = std.mem.splitScalar(u8, style, ';');
    while (decl := it.next()) {
        const colon_opt = std.mem.indexOfScalar(u8, decl, ':');
        if (colon_opt == null) continue;
        const colon = colon_opt.?;
        if (colon == 0) continue;
        const prop = std.mem.trim(u8, decl[0..colon], " \t\r\n");
        const val = std.mem.trim(u8, decl[colon + 1 ..], " \t\r\n");
        if (std.ascii.eqlIgnoreCase(prop, "visibility")) {
            if (std.ascii.startsWithIgnoreCase(val, "hidden")) {
                return true;
            }
        }
    }
    return false;
}

fn inlineStyleVisibilityVisible(style: []const u8): bool {
    let it = std.mem.splitScalar(u8, style, ';');
    while (decl := it.next()) {
        const colon_opt = std.mem.indexOfScalar(u8, decl, ':');
        if (colon_opt == null) continue;
        const colon = colon_opt.?;
        if (colon == 0) continue;
        const prop = std.mem.trim(u8, decl[0..colon], " \t\r\n");
        const val = std.mem.trim(u8, decl[colon + 1 ..], " \t\r\n");
        if (std.ascii.eqlIgnoreCase(prop, "visibility")) {
            if (std.ascii.startsWithIgnoreCase(val, "visible")) {
                return true;
            }
        }
    }
    return false;
}

fn isNoBox(node: *const NodeData): bool {
    if (node.getAttribute("hidden") != null) return true;
    if (std.ascii.eqlIgnoreCase(node.tag_name, "input")) {
        if (typ := node.getAttribute("type")) {
            if (std.ascii.eqlIgnoreCase(typ, "hidden")) return true;
        }
    }
    if (node.inline_style.len > 0) {
        if (inlineStyleDisplayNone(node.inline_style)) return true;
    }
    return false;
}

fn isTablePart(tag: []const u8): bool {
    return std.ascii.eqlIgnoreCase(tag, "caption") ||
        std.ascii.eqlIgnoreCase(tag, "thead") ||
        std.ascii.eqlIgnoreCase(tag, "tbody") ||
        std.ascii.eqlIgnoreCase(tag, "tfoot") ||
        std.ascii.eqlIgnoreCase(tag, "tr") ||
        std.ascii.eqlIgnoreCase(tag, "th") ||
        std.ascii.eqlIgnoreCase(tag, "td") ||
        std.ascii.eqlIgnoreCase(tag, "colgroup") ||
        std.ascii.eqlIgnoreCase(tag, "col");
}

fn isLayoutTable(doc: *const Document, table_id: NodeId): bool {
    if (t_node := doc.getNode(table_id)) {
        if (t_node.getAttribute("summary") != null) return false;
    }
    let data_signals: usize = 0;
    let stack: [64]NodeId = [@as(NodeId, 0); 64];
    let depths: [64]usize = [@as(usize, 0); 64];
    let stack_len: usize = 0;

    if (t_node := doc.getNode(table_id)) {
        let c = t_node.first_child;
        while (c != NULL_NODE) {
            if (stack_len < 64) {
                stack[stack_len] = c;
                depths[stack_len] = 1;
                stack_len += 1;
            }
            if (cn := doc.getNode(c)) {
                c = cn.next_sibling;
            } else break;
        }
    }

    while (stack_len > 0) {
        stack_len -= 1;
        const curr_id = stack[stack_len];
        const depth = depths[stack_len];
        if (curr := doc.getNode(curr_id)) {
            if (curr.node_type == NodeType.Element) {
                const tag = curr.tag_name;
                if (std.ascii.eqlIgnoreCase(tag, "caption") || std.ascii.eqlIgnoreCase(tag, "th")) {
                    data_signals += 1;
                    break;
                }
                if (std.ascii.eqlIgnoreCase(tag, "td")) {
                    if (curr.getAttribute("scope") != null ||
                        curr.getAttribute("headers") != null ||
                        curr.getAttribute("abbr") != null) {
                        data_signals += 1;
                        break;
                    }
                }
                if (std.ascii.eqlIgnoreCase(tag, "table")) continue;
                if (depth < 5) {
                    let c = curr.first_child;
                    while (c != NULL_NODE) {
                        if (stack_len < 64) {
                            stack[stack_len] = c;
                            depths[stack_len] = depth + 1;
                            stack_len += 1;
                        }
                        if (cn := doc.getNode(c)) {
                            c = cn.next_sibling;
                        } else break;
                    }
                }
            }
        }
    }

    return data_signals == 0;
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
    if (std.ascii.eqlIgnoreCase(tag, "option")) return "option";
    if (std.ascii.eqlIgnoreCase(tag, "datalist")) return "listbox";
    if (std.ascii.eqlIgnoreCase(tag, "meter")) return "meter";
    if (std.ascii.eqlIgnoreCase(tag, "progress")) return "progressbar";
    if (std.ascii.eqlIgnoreCase(tag, "search")) return "search";
    if (std.ascii.eqlIgnoreCase(tag, "iframe")) return "Iframe";
    return "generic";
}

fn isFocusable(tag: []const u8, node: *const NodeData): bool {
    if (node.getAttribute("disabled") != null) return false;
    if (std.ascii.eqlIgnoreCase(tag, "button") ||
        std.ascii.eqlIgnoreCase(tag, "select") ||
        std.ascii.eqlIgnoreCase(tag, "textarea") ||
        std.ascii.eqlIgnoreCase(tag, "summary") ||
        std.ascii.eqlIgnoreCase(tag, "iframe")) {
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

pub struct LiveSelect {
    select_nid: NodeId = NULL_NODE,
    selected_index: i32 = -1,
    selected_opts: [16]NodeId = [NULL_NODE; 16],
    selected_opts_len: usize = 0,
}

pub struct AXTree {
    nodes: [MAX_AX_NODES]AXNode = [AXNode{}; MAX_AX_NODES],
    node_count: usize = 0,
    id_buffers: [ID_BUFFER_SIZE]u8 = [0; ID_BUFFER_SIZE],
    text_pool: [TEXT_POOL_SIZE]u8 = [0; TEXT_POOL_SIZE],
    text_pool_len: usize = 0,
    root_url: []const u8 = "",
    live_selects: [32]LiveSelect = [LiveSelect{}; 32],
    live_selects_len: usize = 0,
    has_live_state: bool = false,

    fn allocNode(*_): usize? {
        if (_.node_count >= MAX_AX_NODES) return null;
        const idx = _.node_count;
        _.node_count += 1;
        _.nodes[idx] = AXNode{};
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

    fn fetchLiveState(*_, session: *CdpPageSession) {
        if (!session.has_document) return;
        const js_expr = "(() => { const selects = document.querySelectorAll ? document.querySelectorAll('select') : []; const res = []; for (let i = 0; i < selects.length; i++) { const s = selects[i]; const s_nid = (typeof s._nid === 'number') ? s._nid : 0; const s_idx = typeof s.selectedIndex === 'number' ? s.selectedIndex : -1; const opts = s.options; const opt_ids = []; if (opts) { for (let j = 0; j < opts.length; j++) { if (opts[j].selected) { const o_nid = (typeof opts[j]._nid === 'number') ? opts[j]._nid : 0; if (o_nid > 0) opt_ids.push(o_nid); } } } res.push(s_nid + ':' + s_idx + ';' + opt_ids.join(',')); } return res.join('|'); })()";
        const val = session.ctx.eval(js_expr);
        const cstr = session.ctx.toCString(val);
        if (cstr.len > 0) {
            _.has_live_state = true;
            let sel_it = std.mem.splitScalar(u8, cstr, '|');
            while (sel_chunk := sel_it.next()) {
                if (_.live_selects_len >= 32) break;
                let semi_it = std.mem.splitScalar(u8, sel_chunk, ';');
                const head = semi_it.next() ?? "";
                const opts_str = semi_it.next() ?? "";

                let colon_it = std.mem.splitScalar(u8, head, ':');
                const s_nid_str = colon_it.next() ?? "";
                const s_idx_str = colon_it.next() ?? "";

                const s_nid = @as(NodeId, std.fmt.parseInt(u32, s_nid_str, 10) catch 0);
                const s_idx = std.fmt.parseInt(i32, s_idx_str, 10) catch -1;

                let ls = LiveSelect{
                    select_nid: s_nid,
                    selected_index: s_idx,
                    selected_opts: [NULL_NODE; 16],
                    selected_opts_len: 0,
                };

                if (opts_str.len > 0) {
                    let opt_it = std.mem.splitScalar(u8, opts_str, ',');
                    while (opt_str := opt_it.next()) {
                        if (ls.selected_opts_len >= 16) break;
                        const onid = @as(NodeId, std.fmt.parseInt(u32, opt_str, 10) catch 0);
                        if (onid != NULL_NODE) {
                            ls.selected_opts[ls.selected_opts_len] = onid;
                            ls.selected_opts_len += 1;
                        }
                    }
                }

                _.live_selects[_.live_selects_len] = ls;
                _.live_selects_len += 1;
            }
        }
        session.ctx.freeValue(val);
    }

    fn findSelectedOptionText(*_, doc: *const Document, select_id: NodeId): ([]const u8)? {
        if (_.has_live_state) {
            let i: usize = 0;
            while (i < _.live_selects_len) : (i += 1) {
                if (_.live_selects[i].select_nid == select_id) {
                    const ls = &_.live_selects[i];
                    if (ls.selected_index < 0 || ls.selected_opts_len == 0) {
                        return null;
                    }
                    const chosen = ls.selected_opts[0];
                    if (chosen != NULL_NODE) {
                        if (opt_node := doc.getNode(chosen)) {
                            if (lbl := opt_node.getAttribute("label")) return lbl;
                            const txt = _.collectSubtreeText(doc, chosen);
                            if (txt.len > 0) return txt;
                            if (val := opt_node.getAttribute("value")) return val;
                        }
                    }
                    return null;
                }
            }
        }

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
                const txt = _.collectSubtreeText(doc, chosen);
                if (txt.len > 0) return txt;
                if (val := opt_node.getAttribute("value")) return val;
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

        // 4. Content naming tags / roles
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
            std.mem.eql(u8, role, "heading") ||
            std.mem.eql(u8, role, "cell") ||
            std.mem.eql(u8, role, "checkbox") ||
            std.mem.eql(u8, role, "columnheader") ||
            std.mem.eql(u8, role, "gridcell") ||
            std.mem.eql(u8, role, "menuitem") ||
            std.mem.eql(u8, role, "option") ||
            std.mem.eql(u8, role, "radio") ||
            std.mem.eql(u8, role, "row") ||
            std.mem.eql(u8, role, "rowheader") ||
            std.mem.eql(u8, role, "switch") ||
            std.mem.eql(u8, role, "tab") ||
            std.mem.eql(u8, role, "tooltip") ||
            std.mem.eql(u8, role, "treeitem")) {
            const txt = _.collectSubtreeText(doc, node.id);
            if (txt.len > 0) return txt;
        }

        // 5. Title fallback
        if (title := node.getAttribute("title")) {
            const trimmed = std.mem.trim(u8, title, " \t\r\n");
            if (trimmed.len > 0) return trimmed;
        }

        // 6. Subtree text fallback
        const fallback_txt = _.collectSubtreeText(doc, node.id);
        if (fallback_txt.len > 0) return fallback_txt;

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

    fn populateProperties(*_, doc: *const Document, ax_idx: usize, node: *const NodeData, role: []const u8) {
        let node_ref = &_.nodes[ax_idx];

        if (node.tag_name.len == 2 && (node.tag_name[0] == 'h' || node.tag_name[0] == 'H')) {
            const c = node.tag_name[1];
            if (c >= '1' && c <= '6') {
                node_ref.addProperty(AXProperty.create("level", AxValue.fromInt("integer", @as(i64, c - '0'))));
            }
        }
        if (al := node.getAttribute("aria-level")) {
            const level_num = std.fmt.parseInt(i64, std.mem.trim(u8, al, " \t\r\n"), 10) catch 0;
            if (level_num > 0) {
                node_ref.addProperty(AXProperty.create("level", AxValue.fromInt("integer", level_num)));
            }
        } else if (std.ascii.eqlIgnoreCase(node.tag_name, "li") || std.ascii.eqlIgnoreCase(role, "listitem")) {
            let depth: i64 = 0;
            let p_dom = node.parent;
            while (p_dom != NULL_NODE) {
                if (p_node := doc.getNode(p_dom)) {
                    if (p_node.node_type == NodeType.Element) {
                        if (std.ascii.eqlIgnoreCase(p_node.tag_name, "ul") ||
                            std.ascii.eqlIgnoreCase(p_node.tag_name, "ol") ||
                            std.ascii.eqlIgnoreCase(p_node.tag_name, "menu")) {
                            depth += 1;
                        }
                    }
                    p_dom = p_node.parent;
                } else {
                    break;
                }
            }
            if (depth > 0) {
                node_ref.addProperty(AXProperty.create("level", AxValue.fromInt("integer", depth)));
            }
        }

        if (std.mem.eql(u8, role, "link") || std.ascii.eqlIgnoreCase(node.tag_name, "a") || std.ascii.eqlIgnoreCase(node.tag_name, "area")) {
            if (href := node.getAttribute("href")) {
                let resolved = href;
                if (_.root_url.len > 0) {
                    const remain = TEXT_POOL_SIZE - _.text_pool_len;
                    if (remain >= 256) {
                        const start = _.text_pool_len;
                        const res = resolveUrl(href, _.root_url, _.text_pool[start..]);
                        if (res.len > 0) {
                            _.text_pool_len += res.len;
                            resolved = _.text_pool[start .. start + res.len];
                        }
                    }
                }
                node_ref.addProperty(AXProperty.create("url", AxValue.fromString("string", resolved)));
            }
        } else if (std.ascii.eqlIgnoreCase(node.tag_name, "img")) {
            if (src := node.getAttribute("src")) {
                let resolved = src;
                if (_.root_url.len > 0) {
                    const remain = TEXT_POOL_SIZE - _.text_pool_len;
                    if (remain >= 256) {
                        const start = _.text_pool_len;
                        const res = resolveUrl(src, _.root_url, _.text_pool[start..]);
                        if (res.len > 0) {
                            _.text_pool_len += res.len;
                            resolved = _.text_pool[start .. start + res.len];
                        }
                    }
                }
                node_ref.addProperty(AXProperty.create("url", AxValue.fromString("string", resolved)));
            }
        }

        if (aks := node.getAttribute("aria-keyshortcuts")) {
            node_ref.addProperty(AXProperty.create("keyshortcuts", AxValue.fromString("string", aks)));
        } else if (ak := node.getAttribute("accesskey")) {
            const remain = TEXT_POOL_SIZE - _.text_pool_len;
            if (remain >= 32) {
                const out_slice = _.text_pool[_.text_pool_len..];
                const ks_str = std.fmt.bufPrint(out_slice, "Ctrl+Alt+{s}", .{ak}) catch "";
                if (ks_str.len > 0) {
                    _.text_pool_len += ks_str.len;
                    node_ref.addProperty(AXProperty.create("keyshortcuts", AxValue.fromString("string", ks_str)));
                }
            }
        }

        if (isFocusable(node.tag_name, node)) {
            node_ref.addProperty(AXProperty.create("focusable", AxValue.fromBool("booleanOrUndefined", true)));
        }

        if (node.getAttribute("disabled") != null) {
            node_ref.addProperty(AXProperty.create("disabled", AxValue.fromBool("boolean", true)));
        }

        if (std.ascii.eqlIgnoreCase(node.tag_name, "input")) {
            const typ = node.getAttribute("type") ?? "text";
            if (std.ascii.eqlIgnoreCase(typ, "checkbox") || std.ascii.eqlIgnoreCase(typ, "radio")) {
                const is_chk = node.getAttribute("checked") != null;
                node_ref.addProperty(AXProperty.create("checked", AxValue.fromString("tristate", if (is_chk) "true" else "false")));
            } else if (std.ascii.eqlIgnoreCase(typ, "range") || std.ascii.eqlIgnoreCase(typ, "number")) {
                if (min_str := node.getAttribute("min")) {
                    const min_val = std.fmt.parseInt(i64, std.mem.trim(u8, min_str, " \t\r\n"), 10) catch -1;
                    if (min_val >= 0) node_ref.addProperty(AXProperty.create("valuemin", AxValue.fromInt("number", min_val)));
                }
                if (max_str := node.getAttribute("max")) {
                    const max_val = std.fmt.parseInt(i64, std.mem.trim(u8, max_str, " \t\r\n"), 10) catch -1;
                    if (max_val >= 0) node_ref.addProperty(AXProperty.create("valuemax", AxValue.fromInt("number", max_val)));
                }
                if (now_str := node.getAttribute("value")) {
                    const now_val = std.fmt.parseInt(i64, std.mem.trim(u8, now_str, " \t\r\n"), 10) catch -1;
                    if (now_val >= 0) node_ref.addProperty(AXProperty.create("valuenow", AxValue.fromInt("number", now_val)));
                }
            }
        } else if (ac := node.getAttribute("aria-checked")) {
            node_ref.addProperty(AXProperty.create("checked", AxValue.fromString("tristate", ac)));
        }

        if (vmin := node.getAttribute("aria-valuemin")) {
            const min_val = std.fmt.parseInt(i64, std.mem.trim(u8, vmin, " \t\r\n"), 10) catch 0;
            node_ref.addProperty(AXProperty.create("valuemin", AxValue.fromInt("number", min_val)));
        }
        if (vmax := node.getAttribute("aria-valuemax")) {
            const max_val = std.fmt.parseInt(i64, std.mem.trim(u8, vmax, " \t\r\n"), 10) catch 0;
            node_ref.addProperty(AXProperty.create("valuemax", AxValue.fromInt("number", max_val)));
        }
        if (vnow := node.getAttribute("aria-valuenow")) {
            const now_val = std.fmt.parseInt(i64, std.mem.trim(u8, vnow, " \t\r\n"), 10) catch 0;
            node_ref.addProperty(AXProperty.create("valuenow", AxValue.fromInt("number", now_val)));
        }

        if (ap := node.getAttribute("aria-pressed")) {
            node_ref.addProperty(AXProperty.create("pressed", AxValue.fromString("tristate", ap)));
        }

        if (node.getAttribute("required") != null) {
            node_ref.addProperty(AXProperty.create("required", AxValue.fromBool("boolean", true)));
        }

        if (node.getAttribute("readonly") != null) {
            node_ref.addProperty(AXProperty.create("readonly", AxValue.fromBool("boolean", true)));
        }

        let is_selected = false;
        if (_.has_live_state && std.ascii.eqlIgnoreCase(node.tag_name, "option")) {
            let si: usize = 0;
            while (si < _.live_selects_len) : (si += 1) {
                const ls = &_.live_selects[si];
                let oi: usize = 0;
                while (oi < ls.selected_opts_len) : (oi += 1) {
                    if (ls.selected_opts[oi] == node.id) {
                        is_selected = true;
                        break;
                    }
                }
                if (is_selected) break;
            }
        } else if (node.getAttribute("selected") != null) {
            is_selected = true;
        }

        if (is_selected) {
            node_ref.addProperty(AXProperty.create("selected", AxValue.fromBool("boolean", true)));
        }

        if (ae := node.getAttribute("aria-expanded")) {
            if (std.ascii.eqlIgnoreCase(ae, "true")) {
                node_ref.addProperty(AXProperty.create("expanded", AxValue.fromBool("boolean", true)));
            } else if (std.ascii.eqlIgnoreCase(ae, "false")) {
                node_ref.addProperty(AXProperty.create("expanded", AxValue.fromBool("boolean", false)));
            }
        } else if (std.ascii.eqlIgnoreCase(node.tag_name, "details") && node.getAttribute("open") != null) {
            node_ref.addProperty(AXProperty.create("expanded", AxValue.fromBool("boolean", true)));
        }

        if (hp := node.getAttribute("aria-haspopup")) {
            if (!std.ascii.eqlIgnoreCase(hp, "false")) {
                const pop_val = if (std.ascii.eqlIgnoreCase(hp, "true")) "menu" else hp;
                node_ref.addProperty(AXProperty.create("hasPopup", AxValue.fromString("token", pop_val)));
            }
        }
    }

    fn buildNode(
        *_,
        doc: *const Document,
        dom_id: NodeId,
        parent_ax_idx: usize,
        frame_id: []const u8,
        inherited_hidden: bool,
        inherited_aria_hidden: bool,
        inherited_vis_hidden: bool,
        inh_layout_table: bool,
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
            _.nodes[ax_idx].parent_id = _.nodes[parent_ax_idx].node_id;

            const text_hidden = inherited_hidden || inherited_vis_hidden;
            if (text_hidden) {
                _.nodes[ax_idx].ignored = true;
                _.nodes[ax_idx].ignored_reason = if (inherited_aria_hidden) "ariaHiddenSubtree" else "notRendered";
                _.nodes[ax_idx].role = AxValue.fromRole("none", true);
            } else {
                _.nodes[ax_idx].ignored = false;
                _.nodes[ax_idx].role = AxValue.fromRole("StaticText", true);
                _.nodes[ax_idx].name = AxValue.fromComputedString(txt);
            }

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

            let self_aria_hidden = false;
            if (ah := node.getAttribute("aria-hidden")) {
                self_aria_hidden = std.ascii.eqlIgnoreCase(ah, "true");
            }
            const aria_hidden = inherited_aria_hidden || self_aria_hidden;
            const self_no_box = isNoBox(node);
            let self_vis_hidden = false;
            let self_vis_visible = false;
            if (node.inline_style.len > 0) {
                self_vis_hidden = inlineStyleVisibilityHidden(node.inline_style);
                self_vis_visible = inlineStyleVisibilityVisible(node.inline_style);
            }

            let vis_hidden = inherited_vis_hidden;
            if (self_vis_visible) {
                vis_hidden = false;
            } else if (self_vis_hidden) {
                vis_hidden = true;
            }

            const node_hidden = self_no_box || aria_hidden || vis_hidden;

            const explicit_role = getExplicitRole(node);
            const role_str = if (explicit_role != null) explicit_role.? else computeImplicitRole(node.tag_name, node);

            const is_table = std.ascii.eqlIgnoreCase(node.tag_name, "table") && explicit_role == null;
            const starts_layout_table = is_table && isLayoutTable(doc, dom_id);
            const in_layout_table = if (is_table) starts_layout_table else inh_layout_table;
            const demoted_by_table = starts_layout_table || (inh_layout_table && isTablePart(node.tag_name) && explicit_role == null);

            let ignored = false;
            let reason: []const u8 = "notRendered";

            if (node_hidden) {
                ignored = true;
                if (self_aria_hidden) {
                    reason = "ariaHiddenElement";
                } else if (aria_hidden) {
                    reason = "ariaHiddenSubtree";
                } else {
                    reason = "notRendered";
                }
            } else if (std.ascii.eqlIgnoreCase(role_str, "presentation") || std.ascii.eqlIgnoreCase(role_str, "none") || demoted_by_table) {
                ignored = true;
                reason = "presentationalRole";
            } else if (std.ascii.eqlIgnoreCase(node.tag_name, "html")) {
                ignored = true;
                reason = "uninteresting";
            }

            _.nodes[ax_idx].ignored = ignored;
            _.nodes[ax_idx].ignored_reason = reason;

            if (ignored) {
                _.nodes[ax_idx].role = AxValue.fromRole("none", true);
            } else {
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

                _.populateProperties(doc, ax_idx, node, role_str);
            }

            // If boxless (display:none or hidden attr), children are not rendered and have no boxes
            if (self_no_box) {
                return ax_idx;
            }

            // Recurse children
            let child_id = node.first_child;
            while (child_id != NULL_NODE) {
                if (c_node := doc.getNode(child_id)) {
                    const next_sib = c_node.next_sibling;
                    * = _.buildNode(doc, child_id, ax_idx, frame_id, node_hidden, aria_hidden, vis_hidden, in_layout_table);
                    child_id = next_sib;
                } else {
                    break;
                }
            }

            return ax_idx;
        }

        return null;
    }

    fn buildFromDoc(
        *_,
        doc: *const Document,
        root_title: []const u8,
        root_url: []const u8,
        frame_id: []const u8,
    ) {
        _.node_count = 0;
        _.text_pool_len = 0;
        _.root_url = root_url;

        // Allocate node 0: RootWebArea
        const root_ax_opt = _.allocNode();
        if (root_ax_opt == null) return;
        const root_idx = root_ax_opt.?;

        _.nodes[root_idx].node_id = "root";
        _.nodes[root_idx].ignored = false;
        _.nodes[root_idx].role = AxValue.fromRole("RootWebArea", true);
        _.nodes[root_idx].chrome_role = 144;
        _.nodes[root_idx].name = AxValue.fromComputedString(root_title);
        _.nodes[root_idx].value = AxValue.fromString("string", root_url);
        _.nodes[root_idx].backend_dom_node_id = doc.root_element;
        _.nodes[root_idx].frame_id = frame_id;

        _.nodes[root_idx].addProperty(AXProperty.create("url", AxValue.fromString("string", root_url)));
        _.nodes[root_idx].addProperty(AXProperty.create("focusable", AxValue.fromBool("booleanOrUndefined", true)));

        if (doc.root_element != NULL_NODE) {
            * = _.buildNode(doc, doc.root_element, root_idx, frame_id, false, false, false, false);
        }
    }

    fn buildFromSession(*_, session: *CdpPageSession) {
        _.live_selects_len = 0;
        _.has_live_state = false;
        if (session.has_document) {
            _.fetchLiveState(session);
        }
        _.buildFromDoc(&session.doc, session.title, session.url, session.frame_id);
    }
}
pub const AxTree = AXTree;
pub const AxTreeBuilder = AXTree;

fn findDocumentTitle(doc: *const Document): []const u8 {
    let i: usize = 1;
    while (i < doc.nodeCount()) : (i += 1) {
        if (n := doc.getNode(@intCast(i))) {
            if (n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, "title")) {
                let curr = n.first_child;
                while (curr != NULL_NODE) {
                    if (c := doc.getNode(curr)) {
                        if (c.node_type == NodeType.Text) {
                            const trimmed = std.mem.trim(u8, c.text_content, " \t\r\n");
                            if (trimmed.len > 0) return trimmed;
                        }
                        curr = c.next_sibling;
                    } else {
                        break;
                    }
                }
            }
        }
    }
    return "";
}

pub fn buildAXTree(doc: *Document, out_buf: []u8): []const u8 {
    let tree = std.heap.c_allocator.create(AXTree) catch return "{\"nodes\":[]}";
    defer std.heap.c_allocator.destroy(tree);
    tree.* = AXTree{};

    let doc_title = findDocumentTitle(doc);
    if (doc_title.len == 0) doc_title = "Document";

    tree.buildFromDoc(doc, doc_title, "", "capsurf-frame-001");

    let w = JsonWriter.init(out_buf);
    w.writeRaw("{\"nodes\":[");
    let i: usize = 0;
    while (i < tree.node_count) : (i += 1) {
        if (i > 0) w.writeByte(',');
        w.writeAxNode(&tree.nodes[i]);
    }
    w.writeRaw("]}");
    if (w.overflow) {
        return "{\"nodes\":[]}";
    }
    return w.getWritten();
}

pub struct AccessibilityHandler {
    static fn handle(
        session: *CdpPageSession,
        req: CdpRequest,
        out_buf: []u8,
        out_events: *[4]CdpEvent,
        event_count: *usize,
    ): CdpResponse {
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "Accessibility.disable")) {
            return CdpResponse.ok(req.id, "{}");
        }

        let builder = std.heap.c_allocator.create(AXTree) catch {
            return CdpResponse.err(req.id, -32000, "Out of memory");
        };
        defer std.heap.c_allocator.destroy(builder);
        builder.* = AXTree{};

        builder.buildFromSession(session);

        if (std.mem.eql(u8, req.method, "Accessibility.enable")) {
            if (builder.node_count > 0) {
                let ew = JsonWriter.init(&ax_event_buf);
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
            let target_idx: usize? = null;
            let target_dom_id: u32? = null;

            if (b_id := extractJsonInt(req.params_raw, "backendNodeId")) {
                if (b_id > 0) target_dom_id = @as(u32, @intCast(b_id));
            } else if (n_id := extractJsonInt(req.params_raw, "nodeId")) {
                if (n_id > 0) target_dom_id = @as(u32, @intCast(n_id));
            } else if (obj_id := extractJsonString(req.params_raw, "objectId")) {
                let num_str: []const u8 = "";
                if (std.mem.startsWith(u8, obj_id, "node:")) {
                    num_str = obj_id[5..];
                } else if (std.mem.startsWith(u8, obj_id, "ps:")) {
                    if (last_colon := std.mem.lastIndexOfScalar(u8, obj_id, ':')) {
                        num_str = obj_id[last_colon + 1 ..];
                    }
                }
                if (num_str.len > 0) {
                    const parsed = std.fmt.parseInt(u32, num_str, 10) catch 0;
                    if (parsed > 0) target_dom_id = parsed;
                }
            }

            if (dom_id := target_dom_id) {
                // Match by backend_dom_node_id. Prefer element nodes (index >= 1)
                let i: usize = 1;
                while (i < builder.node_count) : (i += 1) {
                    if (builder.nodes[i].backend_dom_node_id == dom_id) {
                        target_idx = i;
                        break;
                    }
                }
                if (target_idx == null && builder.node_count > 0) {
                    if (builder.nodes[0].backend_dom_node_id == dom_id) {
                        target_idx = 0;
                    }
                }
            } else if (id_str := extractJsonString(req.params_raw, "id")) {
                let i: usize = 0;
                while (i < builder.node_count) : (i += 1) {
                    if (std.mem.eql(u8, builder.nodes[i].node_id, id_str)) {
                        target_idx = i;
                        break;
                    }
                }
            } else {
                if (builder.node_count > 0) {
                    target_idx = 0;
                }
            }

            if (target_idx == null) {
                let w = JsonWriter.init(out_buf);
                w.writeRaw("{\"nodes\":[]}");
                if (w.overflow) {
                    return CdpResponse.err(req.id, -32000, "Buffer overflow");
                }
                return CdpResponse.ok(req.id, w.getWritten());
            }

            const t_idx = target_idx.?;
            const is_and_ancestors = std.mem.eql(u8, req.method, "Accessibility.getAXNodeAndAncestors");
            const want_subtree = !is_and_ancestors;
            const want_ancestors = if (is_and_ancestors) true else (extractJsonBool(req.params_raw, "fetchRelatives") ?? true);

            let selected: [256]bool = [false; 256];
            selected[t_idx] = true;

            if (want_subtree) {
                let stack: [256]usize = [@as(usize, 0); 256];
                let stack_len: usize = 1;
                stack[0] = t_idx;

                while (stack_len > 0) {
                    stack_len -= 1;
                    const curr_idx = stack[stack_len];
                    const curr_node = &builder.nodes[curr_idx];
                    let c: usize = 0;
                    while (c < curr_node.child_count) : (c += 1) {
                        const cid = curr_node.child_ids[c];
                        let j: usize = 0;
                        while (j < builder.node_count) : (j += 1) {
                            if (std.mem.eql(u8, builder.nodes[j].node_id, cid)) {
                                if (!selected[j]) {
                                    selected[j] = true;
                                    if (stack_len < MAX_AX_NODES) {
                                        stack[stack_len] = j;
                                        stack_len += 1;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }

            if (want_ancestors) {
                let cur_idx: usize? = t_idx;
                let depth: usize = 0;
                while (cur_idx != null && depth < MAX_AX_NODES) : (depth += 1) {
                    const cur_node = &builder.nodes[cur_idx.?];
                    if (cur_node.parent_id.len == 0) break;
                    let p_idx: usize? = null;
                    let j: usize = 0;
                    while (j < builder.node_count) : (j += 1) {
                        if (std.mem.eql(u8, builder.nodes[j].node_id, cur_node.parent_id)) {
                            p_idx = j;
                            break;
                        }
                    }
                    if (p_idx == null) break;
                    selected[p_idx.?] = true;
                    cur_idx = p_idx;
                }
            }

            let w = JsonWriter.init(out_buf);
            w.writeRaw("{\"nodes\":[");
            let written_count: usize = 0;
            let i: usize = 0;
            while (i < builder.node_count) : (i += 1) {
                if (selected[i]) {
                    if (written_count > 0) w.writeByte(',');
                    w.writeAxNode(&builder.nodes[i]);
                    written_count += 1;
                }
            }
            w.writeRaw("]}");
            if (w.overflow) {
                return CdpResponse.err(req.id, -32000, "Buffer overflow");
            }
            return CdpResponse.ok(req.id, w.getWritten());
        }

        return CdpResponse.ok(req.id, "{}");
    }
}
