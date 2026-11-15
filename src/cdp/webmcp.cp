// src/cdp/webmcp.cp
// CDP WebMCP (Model Context Protocol) Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/webmcp.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { Document, NodeId, NULL_NODE, NodeType } from "../dom/dom.cp";

pub struct PageTool {
    name: []const u8 = "",
    description: []const u8 = "",
    input_schema: []const u8 = "{\"type\":\"object\",\"properties\":{}}",
    node_id: NodeId = NULL_NODE,
    action_type: u8 = 0, // 0 = click, 1 = submit, 2 = input
    target_id: []const u8 = "",
    target_name: []const u8 = "",

    fn getName(): []const u8 {
        return _.name;
    }

    fn getDescription(): []const u8 {
        return _.description;
    }

    fn getSchema(): []const u8 {
        if (_.input_schema.len == 0) {
            return "{\"type\":\"object\",\"properties\":{}}";
        }
        return _.input_schema;
    }

    fn getTargetId(): []const u8 {
        return _.target_id;
    }

    fn getTargetName(): []const u8 {
        return _.target_name;
    }
}

pub struct WebMcpState {
    enabled: bool = false,
    invocation_count: u32 = 0,

    fn enable(*_) {
        _.enabled = true;
    }

    fn disable(*_) {
        _.enabled = false;
    }

    fn nextInvocationId(*_, buf: []u8): []const u8 {
        _.invocation_count += 1;
        return std.fmt.bufPrint(buf, "webmcp-{d}", .{_.invocation_count}) catch "webmcp-1";
    }
}

pub threadlocal let g_webmcp_state: WebMcpState = WebMcpState{};
pub threadlocal let g_event_params_buf: [1024]u8 = [0; 1024];
pub threadlocal let g_tool_str_buf: [8192]u8 = [0; 8192];
pub threadlocal let g_tool_str_off: usize = 0;

fn allocToolStr(str: []const u8): []const u8 {
    if (g_tool_str_off + str.len > g_tool_str_buf.len) return str;
    const start = g_tool_str_off;
    @memcpy(g_tool_str_buf[start .. start + str.len], str[0..str.len]);
    g_tool_str_off += str.len;
    return g_tool_str_buf[start .. start + str.len];
}

fn escapeJson(src: []const u8, dst: []u8): []const u8 {
    let out_idx: usize = 0;
    let i: usize = 0;
    while (i < src.len && out_idx + 2 <= dst.len) : (i += 1) {
        const c = src[i];
        if (c == '"') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = '"';
            out_idx += 2;
        } else if (c == '\\') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = '\\';
            out_idx += 2;
        } else if (c == '\n') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = 'n';
            out_idx += 2;
        } else if (c == '\r') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = 'r';
            out_idx += 2;
        } else if (c == '\t') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = 't';
            out_idx += 2;
        } else if (c >= 32 && c <= 126) {
            dst[out_idx] = c;
            out_idx += 1;
        }
    }
    return dst[0..out_idx];
}

fn sanitizeSlug(text: []const u8, out_buf: []u8): []const u8 {
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < text.len && out_len < out_buf.len) : (i += 1) {
        const c = text[i];
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            out_buf[out_len] = c;
            out_len += 1;
        } else if (c >= 'A' && c <= 'Z') {
            out_buf[out_len] = c + 32;
            out_len += 1;
        } else if (c == ' ' || c == '-' || c == '_') {
            if (out_len > 0 && out_buf[out_len - 1] != '_') {
                out_buf[out_len] = '_';
                out_len += 1;
            }
        }
    }
    if (out_len > 0 && out_buf[out_len - 1] == '_') {
        out_len -= 1;
    }
    if (out_len == 0) return "action";
    return out_buf[0..out_len];
}

fn getMcpNodeText(doc: *const Document, node_id: NodeId): []const u8 {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return "";
    const node = &doc.nodes.items[@as(usize, node_id)];
    if (node.node_type == NodeType.Text) {
        return node.text_content;
    }
    if (node.text_content.len > 0) {
        return node.text_content;
    }
    let child_id = node.first_child;
    while (child_id != NULL_NODE) {
        if (cnode := doc.getNode(child_id)) {
            if (cnode.node_type == NodeType.Text && cnode.text_content.len > 0) {
                return cnode.text_content;
            }
            child_id = cnode.next_sibling;
        } else {
            break;
        }
    }
    return "";
}

fn collectFormFields(
    doc: *const Document,
    curr_id: NodeId,
    field_names: *[16][]const u8,
    field_descs: *[16][]const u8,
    field_reqs: *[16]bool,
    field_count: *usize,
) {
    let child_id = doc.getFirstChild(curr_id);
    while (child_id != NULL_NODE) {
        if (child := doc.getNode(child_id)) {
            if (child.node_type == NodeType.Element) {
                const is_field = child.hasTag("input") || child.hasTag("select") || child.hasTag("textarea");
                if (is_field) {
                    const type_attr = child.getAttribute("type") ?? "";
                    const is_btn = std.mem.eql(u8, type_attr, "submit") || std.mem.eql(u8, type_attr, "button") || std.mem.eql(u8, type_attr, "reset");
                    if (!is_btn && field_count.* < 16) {
                        let name_opt: ([]const u8)? = child.getAttribute("name");
                        if (name_opt == null && child.id_attr.len > 0) {
                            name_opt = child.id_attr;
                        }
                        if (fname := name_opt) {
                            if (fname.len > 0) {
                                const idx = field_count.*;
                                field_names[idx] = fname;
                                let d_desc = child.getAttribute("toolparamdescription");
                                if (d_desc == null) d_desc = child.getAttribute("placeholder");
                                field_descs[idx] = d_desc ?? fname;
                                field_reqs[idx] = (child.getAttribute("required") != null);
                                field_count.* += 1;
                            }
                        }
                    }
                }
                collectFormFields(doc, child_id, field_names, field_descs, field_reqs, field_count);
            }
            child_id = child.next_sibling;
        } else {
            break;
        }
    }
}

fn buildFormSchema(doc: *const Document, form_id: NodeId, schema_buf: []u8): usize {
    let field_names: [16][]const u8 = [""; 16];
    let field_descs: [16][]const u8 = [""; 16];
    let field_reqs: [16]bool = [false; 16];
    let field_count: usize = 0;

    collectFormFields(doc, form_id, &field_names, &field_descs, &field_reqs, &field_count);

    if (field_count == 0) {
        const empty_schema = "{\"type\":\"object\",\"properties\":{}}";
        const len = @min(empty_schema.len, schema_buf.len);
        @memcpy(schema_buf[0..len], empty_schema[0..len]);
        return len;
    }

    let off: usize = 0;
    const prefix = "{\"type\":\"object\",\"properties\":{";
    if (prefix.len > schema_buf.len) return 0;
    @memcpy(schema_buf[0..prefix.len], prefix);
    off += prefix.len;

    let i: usize = 0;
    while (i < field_count) : (i += 1) {
        if (i > 0) {
            if (off >= schema_buf.len) break;
            schema_buf[off] = ',';
            off += 1;
        }
        const fn_slice = field_names[i];
        const fd_slice = field_descs[i];
        const part = std.fmt.bufPrint(
            schema_buf[off..],
            "\"{s}\":{{\"type\":\"string\",\"description\":\"{s}\"}}",
            .{ fn_slice, fd_slice },
        ) catch break;
        off += part.len;
    }

    if (off + 1 <= schema_buf.len) {
        schema_buf[off] = '}';
        off += 1;
    }

    let has_req = false;
    i = 0;
    while (i < field_count) : (i += 1) {
        if (field_reqs[i]) {
            has_req = true;
            break;
        }
    }

    if (has_req) {
        const req_prefix = ",\"required\":[";
        if (off + req_prefix.len <= schema_buf.len) {
            @memcpy(schema_buf[off .. off + req_prefix.len], req_prefix);
            off += req_prefix.len;
        }
        let req_i: usize = 0;
        i = 0;
        while (i < field_count) : (i += 1) {
            if (field_reqs[i]) {
                if (req_i > 0) {
                    if (off >= schema_buf.len) break;
                    schema_buf[off] = ',';
                    off += 1;
                }
                const fn_slice = field_names[i];
                const rpart = std.fmt.bufPrint(schema_buf[off..], "\"{s}\"", .{fn_slice}) catch break;
                off += rpart.len;
                req_i += 1;
            }
        }
        if (off + 1 <= schema_buf.len) {
            schema_buf[off] = ']';
            off += 1;
        }
    }

    if (off + 1 <= schema_buf.len) {
        schema_buf[off] = '}';
        off += 1;
    }
    return off;
}

fn collectPageTools(doc: *const Document, tools_out: []PageTool): usize {
    g_tool_str_off = 0;
    let count: usize = 0;
    const total_nodes = doc.nodeCount();
    let nid: usize = 1;

    while (nid < total_nodes && count < tools_out.len) : (nid += 1) {
        const node_id: NodeId = @as(NodeId, @truncate(nid));
        if (node := doc.getNode(node_id)) {
            if (node.node_type != NodeType.Element) continue;

            // 1. Declarative WebMCP attributes: toolname, tool-name, data-tool-name
            let decl_name: ([]const u8)? = node.getAttribute("toolname");
            if (decl_name == null) decl_name = node.getAttribute("tool-name");
            if (decl_name == null) decl_name = node.getAttribute("data-tool-name");

            if (tname := decl_name) {
                let tool = PageTool {
                    name: tname,
                    node_id: node_id,
                };

                let decl_desc: ([]const u8)? = node.getAttribute("tooldescription");
                if (decl_desc == null) decl_desc = node.getAttribute("tool-description");
                if (decl_desc == null) decl_desc = node.getAttribute("data-tool-description");

                if (desc := decl_desc) {
                    tool.description = desc;
                } else {
                    let dbuf: [128]u8 = [0; 128];
                    const d = std.fmt.bufPrint(&dbuf, "Declarative tool {s}", .{tname}) catch "Declarative tool";
                    tool.description = allocToolStr(d);
                }

                if (node.hasTag("form")) {
                    tool.action_type = 1;
                    let sbuf: [512]u8 = [0; 512];
                    const slen = buildFormSchema(doc, node_id, &sbuf);
                    tool.input_schema = allocToolStr(sbuf[0..slen]);
                } else {
                    tool.action_type = 0;
                    tool.input_schema = "{\"type\":\"object\",\"properties\":{}}";
                }

                if (node.id_attr.len > 0) tool.target_id = node.id_attr;
                if (n := node.getAttribute("name")) tool.target_name = n;

                tools_out[count] = tool;
                count += 1;
                continue;
            }

            // 2. Interactive <button>
            if (node.hasTag("button")) {
                let tool = PageTool { node_id: node_id, action_type: 0 };
                let name_buf: [64]u8 = [0; 64];
                let desc_buf: [128]u8 = [0; 128];
                const text = getMcpNodeText(doc, node_id);

                if (node.id_attr.len > 0) {
                    const n = std.fmt.bufPrint(&name_buf, "click_{s}", .{node.id_attr}) catch "click_btn";
                    tool.name = allocToolStr(n);
                    tool.target_id = node.id_attr;
                } else if (name_attr := node.getAttribute("name")) {
                    const n = std.fmt.bufPrint(&name_buf, "click_{s}", .{name_attr}) catch "click_btn";
                    tool.name = allocToolStr(n);
                    tool.target_name = name_attr;
                } else if (text.len > 0 && text.len <= 24) {
                    let slug_buf: [32]u8 = [0; 32];
                    const slug = sanitizeSlug(text, &slug_buf);
                    const n = std.fmt.bufPrint(&name_buf, "click_{s}", .{slug}) catch "click_btn";
                    tool.name = allocToolStr(n);
                } else {
                    const n = std.fmt.bufPrint(&name_buf, "click_button_{d}", .{node_id}) catch "click_btn";
                    tool.name = allocToolStr(n);
                }

                if (text.len > 0) {
                    const d = std.fmt.bufPrint(&desc_buf, "Click button \"{s}\"", .{text}) catch "Click button";
                    tool.description = allocToolStr(d);
                } else if (node.id_attr.len > 0) {
                    const d = std.fmt.bufPrint(&desc_buf, "Click button #{s}", .{node.id_attr}) catch "Click button";
                    tool.description = allocToolStr(d);
                } else {
                    tool.description = "Click button";
                }

                tool.input_schema = "{\"type\":\"object\",\"properties\":{}}";
                tools_out[count] = tool;
                count += 1;
                continue;
            }

            // 3. Interactive <form>
            if (node.hasTag("form")) {
                let tool = PageTool { node_id: node_id, action_type: 1 };
                let name_buf: [64]u8 = [0; 64];
                let desc_buf: [128]u8 = [0; 128];

                if (node.id_attr.len > 0) {
                    const n = std.fmt.bufPrint(&name_buf, "submit_{s}", .{node.id_attr}) catch "submit_form";
                    tool.name = allocToolStr(n);
                    tool.target_id = node.id_attr;
                    const d = std.fmt.bufPrint(&desc_buf, "Submit form #{s}", .{node.id_attr}) catch "Submit form";
                    tool.description = allocToolStr(d);
                } else if (name_attr := node.getAttribute("name")) {
                    const n = std.fmt.bufPrint(&name_buf, "submit_{s}", .{name_attr}) catch "submit_form";
                    tool.name = allocToolStr(n);
                    tool.target_name = name_attr;
                    const d = std.fmt.bufPrint(&desc_buf, "Submit form \"{s}\"", .{name_attr}) catch "Submit form";
                    tool.description = allocToolStr(d);
                } else {
                    const n = std.fmt.bufPrint(&name_buf, "submit_form_{d}", .{node_id}) catch "submit_form";
                    tool.name = allocToolStr(n);
                    tool.description = "Submit form";
                }

                let sbuf: [512]u8 = [0; 512];
                const slen = buildFormSchema(doc, node_id, &sbuf);
                tool.input_schema = allocToolStr(sbuf[0..slen]);

                tools_out[count] = tool;
                count += 1;
                continue;
            }

            // 4. Interactive <input>
            if (node.hasTag("input")) {
                const type_attr = node.getAttribute("type") ?? "text";
                if (std.mem.eql(u8, type_attr, "hidden")) continue;

                if (std.mem.eql(u8, type_attr, "submit") || std.mem.eql(u8, type_attr, "button")) {
                    let tool = PageTool { node_id: node_id, action_type: 0 };
                    let name_buf: [64]u8 = [0; 64];
                    let desc_buf: [128]u8 = [0; 128];
                    const val_attr = node.getAttribute("value") ?? "";

                    if (node.id_attr.len > 0) {
                        const n = std.fmt.bufPrint(&name_buf, "click_{s}", .{node.id_attr}) catch "click_input";
                        tool.name = allocToolStr(n);
                        tool.target_id = node.id_attr;
                    } else if (name_attr := node.getAttribute("name")) {
                        const n = std.fmt.bufPrint(&name_buf, "click_{s}", .{name_attr}) catch "click_input";
                        tool.name = allocToolStr(n);
                        tool.target_name = name_attr;
                    } else {
                        const n = std.fmt.bufPrint(&name_buf, "click_input_{d}", .{node_id}) catch "click_input";
                        tool.name = allocToolStr(n);
                    }

                    if (val_attr.len > 0) {
                        const d = std.fmt.bufPrint(&desc_buf, "Click button \"{s}\"", .{val_attr}) catch "Click button";
                        tool.description = allocToolStr(d);
                    } else {
                        tool.description = "Click input button";
                    }
                    tool.input_schema = "{\"type\":\"object\",\"properties\":{}}";
                    tools_out[count] = tool;
                    count += 1;
                    continue;
                }

                if (node.id_attr.len > 0 || node.getAttribute("name") != null) {
                    let tool = PageTool { node_id: node_id, action_type: 2 };
                    let name_buf: [64]u8 = [0; 64];
                    let desc_buf: [128]u8 = [0; 128];

                    if (node.id_attr.len > 0) {
                        const n = std.fmt.bufPrint(&name_buf, "input_{s}", .{node.id_attr}) catch "input_field";
                        tool.name = allocToolStr(n);
                        tool.target_id = node.id_attr;
                        const d = std.fmt.bufPrint(&desc_buf, "Input value for #{s}", .{node.id_attr}) catch "Input value";
                        tool.description = allocToolStr(d);
                    } else if (name_attr := node.getAttribute("name")) {
                        const n = std.fmt.bufPrint(&name_buf, "input_{s}", .{name_attr}) catch "input_field";
                        tool.name = allocToolStr(n);
                        tool.target_name = name_attr;
                        const d = std.fmt.bufPrint(&desc_buf, "Input value for field \"{s}\"", .{name_attr}) catch "Input value";
                        tool.description = allocToolStr(d);
                    }

                    tool.input_schema = "{\"type\":\"object\",\"properties\":{\"value\":{\"type\":\"string\",\"description\":\"Text value to enter\"}},\"required\":[\"value\"]}";
                    tools_out[count] = tool;
                    count += 1;
                    continue;
                }
            }

            // 5. Interactive <a> with button role or id
            if (node.hasTag("a")) {
                const role_attr = node.getAttribute("role") ?? "";
                const is_btn_role = std.mem.eql(u8, role_attr, "button");
                if (is_btn_role || node.id_attr.len > 0) {
                    let tool = PageTool { node_id: node_id, action_type: 0 };
                    let name_buf: [64]u8 = [0; 64];
                    let desc_buf: [128]u8 = [0; 128];
                    const text = getMcpNodeText(doc, node_id);

                    if (node.id_attr.len > 0) {
                        const n = std.fmt.bufPrint(&name_buf, "click_{s}", .{node.id_attr}) catch "click_link";
                        tool.name = allocToolStr(n);
                        tool.target_id = node.id_attr;
                    } else if (text.len > 0 && text.len <= 24) {
                        let slug_buf: [32]u8 = [0; 32];
                        const slug = sanitizeSlug(text, &slug_buf);
                        const n = std.fmt.bufPrint(&name_buf, "click_{s}", .{slug}) catch "click_link";
                        tool.name = allocToolStr(n);
                    } else {
                        const n = std.fmt.bufPrint(&name_buf, "click_link_{d}", .{node_id}) catch "click_link";
                        tool.name = allocToolStr(n);
                    }

                    if (text.len > 0) {
                        const d = std.fmt.bufPrint(&desc_buf, "Click link \"{s}\"", .{text}) catch "Click link";
                        tool.description = allocToolStr(d);
                    } else {
                        tool.description = "Click interactive link";
                    }

                    tool.input_schema = "{\"type\":\"object\",\"properties\":{}}";
                    tools_out[count] = tool;
                    count += 1;
                    continue;
                }
            }

            // 6. Interactive <select> / <textarea>
            if (node.hasTag("select") || node.hasTag("textarea")) {
                if (node.id_attr.len > 0 || node.getAttribute("name") != null) {
                    let tool = PageTool { node_id: node_id, action_type: 2 };
                    let name_buf: [64]u8 = [0; 64];
                    let desc_buf: [128]u8 = [0; 128];
                    const tag = if (node.hasTag("select")) "select" else "input";

                    if (node.id_attr.len > 0) {
                        const n = std.fmt.bufPrint(&name_buf, "{s}_{s}", .{ tag, node.id_attr }) catch "field";
                        tool.name = allocToolStr(n);
                        tool.target_id = node.id_attr;
                        const d = std.fmt.bufPrint(&desc_buf, "Set {s} #{s}", .{ tag, node.id_attr }) catch "Set field";
                        tool.description = allocToolStr(d);
                    } else if (name_attr := node.getAttribute("name")) {
                        const n = std.fmt.bufPrint(&name_buf, "{s}_{s}", .{ tag, name_attr }) catch "field";
                        tool.name = allocToolStr(n);
                        tool.target_name = name_attr;
                        const d = std.fmt.bufPrint(&desc_buf, "Set {s} \"{s}\"", .{ tag, name_attr }) catch "Set field";
                        tool.description = allocToolStr(d);
                    }

                    tool.input_schema = "{\"type\":\"object\",\"properties\":{\"value\":{\"type\":\"string\",\"description\":\"Value to set\"}},\"required\":[\"value\"]}";
                    tools_out[count] = tool;
                    count += 1;
                    continue;
                }
            }
        }
    }

    return count;
}

fn extractJsonObject(json: []const u8, key: []const u8): ([]const u8)? {
    let key_pat_buf: [64]u8 = [0; 64];
    if (key.len + 3 > key_pat_buf.len) return null;
    key_pat_buf[0] = '"';
    @memcpy(key_pat_buf[1 .. 1 + key.len], key);
    key_pat_buf[1 + key.len] = '"';
    const key_pat = key_pat_buf[0 .. 2 + key.len];

    const pos = std.mem.indexOf(u8, json, key_pat) orelse return null;
    let idx = pos + key_pat.len;

    while (idx < json.len && (json[idx] == ' ' || json[idx] == ':' || json[idx] == '\t' || json[idx] == '\r' || json[idx] == '\n')) : (idx += 1) {}
    if (idx >= json.len) return null;

    if (json[idx] == '{') {
        const start = idx;
        let depth: i32 = 0;
        let in_str = false;
        let escaped = false;
        while (idx < json.len) : (idx += 1) {
            const ch = json[idx];
            if (escaped) {
                escaped = false;
                continue;
            }
            if (ch == '\\' && in_str) {
                escaped = true;
                continue;
            }
            if (ch == '"') {
                in_str = !in_str;
                continue;
            }
            if (!in_str) {
                if (ch == '{') depth += 1;
                if (ch == '}') {
                    depth -= 1;
                    if (depth == 0) {
                        return json[start .. idx + 1];
                    }
                }
            }
        }
    }
    return null;
}

fn executeClickAction(session: *CdpPageSession, tool: *const PageTool, out_buf: []u8): usize {
    const tid = tool.getTargetId();
    const tname = tool.getTargetName();
    const ttool = tool.getName();

    let js_buf: [512]u8 = [0; 512];
    const js = std.fmt.bufPrint(
        &js_buf,
        "(function(){{ let el = null; if('{s}'.length > 0) el = document.getElementById('{s}'); if(!el && '{s}'.length > 0) el = document.querySelector('[name=\"{s}\"]'); if(!el && '{s}'.length > 0) el = document.querySelector('[toolname=\"{s}\"]'); if(el && typeof el.click === 'function') {{ el.click(); return 'Clicked ' + (el.id || el.getAttribute('name') || el.tagName); }} return 'Clicked element'; }})()",
        .{ tid, tid, tname, tname, ttool, ttool },
    ) catch "";

    let eval_out: [256]u8 = [0; 256];
    if (js.len > 0) {
        * = session.evaluate(js, &eval_out);
    }

    const res = std.fmt.bufPrint(out_buf, "Clicked {s}", .{tool.getName()}) catch "Clicked";
    return res.len;
}

fn executeSubmitAction(session: *CdpPageSession, tool: *const PageTool, args_json: []const u8, out_buf: []u8): usize {
    const tid = tool.getTargetId();
    const tname = tool.getTargetName();
    const ttool = tool.getName();

    let js_buf: [1024]u8 = [0; 1024];
    const js = std.fmt.bufPrint(
        &js_buf,
        "(function(){{ let f = null; if('{s}'.length > 0) f = document.getElementById('{s}'); if(!f && '{s}'.length > 0) f = document.querySelector('[name=\"{s}\"]'); if(!f && '{s}'.length > 0) f = document.querySelector('[toolname=\"{s}\"]'); if(!f) f = document.querySelector('form'); if(!f) return 'Form not found'; const args = {s}; for(const k of Object.keys(args)) {{ const inp = f.querySelector('[name=\"' + k + '\"]') || f.querySelector('#' + k); if(inp) {{ inp.value = args[k]; if(typeof inp.dispatchEvent === 'function') {{ inp.dispatchEvent(new Event('input', {{bubbles: true}})); inp.dispatchEvent(new Event('change', {{bubbles: true}})); }} }} }} if(typeof f.dispatchEvent === 'function') {{ f.dispatchEvent(new Event('submit', {{bubbles: true, cancelable: true}})); }} return 'Form submitted'; }})()",
        .{ tid, tid, tname, tname, ttool, ttool, args_json },
    ) catch "";

    let eval_out: [256]u8 = [0; 256];
    if (js.len > 0) {
        * = session.evaluate(js, &eval_out);
    }

    const res = std.fmt.bufPrint(out_buf, "Submitted {s}", .{tool.getName()}) catch "Submitted";
    return res.len;
}

fn executeInputAction(session: *CdpPageSession, tool: *const PageTool, val: []const u8, out_buf: []u8): usize {
    const tid = tool.getTargetId();
    const tname = tool.getTargetName();

    let js_buf: [512]u8 = [0; 512];
    const js = std.fmt.bufPrint(
        &js_buf,
        "(function(){{ let el = null; if('{s}'.length > 0) el = document.getElementById('{s}'); if(!el && '{s}'.length > 0) el = document.querySelector('[name=\"{s}\"]'); if(!el) return 'Input not found'; el.value = '{s}'; if(typeof el.dispatchEvent === 'function') {{ el.dispatchEvent(new Event('input', {{bubbles: true}})); el.dispatchEvent(new Event('change', {{bubbles: true}})); }} return 'Value set'; }})()",
        .{ tid, tid, tname, tname, val },
    ) catch "";

    let eval_out: [256]u8 = [0; 256];
    if (js.len > 0) {
        * = session.evaluate(js, &eval_out);
    }

    const res = std.fmt.bufPrint(out_buf, "Input value '{s}' into {s}", .{ val, tool.getName() }) catch "Value set";
    return res.len;
}

pub struct WebMcpHandler {
    static fn handleListTools(session: *CdpPageSession, req: CdpRequest, out_buf: []u8): CdpResponse {
        let tools: [64]PageTool = [PageTool{}; 64];
        const tool_count = collectPageTools(&session.doc, &tools);

        let off: usize = 0;
        const prefix = "{\"tools\":[";
        if (prefix.len > out_buf.len) return CdpResponse.err(req.id, -32000, "Buffer overflow");
        @memcpy(out_buf[0..prefix.len], prefix);
        off += prefix.len;

        let esc_name_buf: [128]u8 = [0; 128];
        let esc_desc_buf: [256]u8 = [0; 256];

        let i: usize = 0;
        while (i < tool_count) : (i += 1) {
            if (i > 0) {
                if (off >= out_buf.len) break;
                out_buf[off] = ',';
                off += 1;
            }
            const t = &tools[i];
            const esc_name = escapeJson(t.getName(), &esc_name_buf);
            const esc_desc = escapeJson(t.getDescription(), &esc_desc_buf);

            const item = std.fmt.bufPrint(
                out_buf[off..],
                "{{\"name\":\"{s}\",\"description\":\"{s}\",\"frameId\":\"{s}\",\"inputSchema\":{s},\"backendNodeId\":{d}}}",
                .{ esc_name, esc_desc, session.frame_id, t.getSchema(), t.node_id },
            ) catch break;
            off += item.len;
        }

        if (off + 2 <= out_buf.len) {
            out_buf[off] = ']';
            out_buf[off + 1] = '}';
            off += 2;
        } else {
            return CdpResponse.err(req.id, -32000, "Buffer overflow");
        }

        return CdpResponse.ok(req.id, out_buf[0..off]);
    }

    static fn handleCallTool(
        session: *CdpPageSession,
        req: CdpRequest,
        out_buf: []u8,
        out_events: *[1]CdpEvent,
        event_count: *usize,
    ): CdpResponse {
        const is_invoke = std.mem.eql(u8, req.method, "WebMCP.invokeTool");
        let inv_buf: [32]u8 = [0; 32];
        const inv_id = g_webmcp_state.nextInvocationId(&inv_buf);

        let tool_name: []const u8 = "";
        if (n := extractJsonString(req.params_raw, "name")) {
            tool_name = n;
        } else if (tn := extractJsonString(req.params_raw, "toolName")) {
            tool_name = tn;
        }

        if (tool_name.len == 0) {
            if (is_invoke) {
                const ev = std.fmt.bufPrint(
                    &g_event_params_buf,
                    "{{\"invocationId\":\"{s}\",\"status\":\"Error\",\"errorText\":\"Missing tool name\"}}",
                    .{inv_id},
                ) catch "{\"status\":\"Error\"}";
                out_events[0] = CdpEvent.create("WebMCP.toolResponded", ev);
                event_count.* = 1;
                const res = std.fmt.bufPrint(out_buf, "{{\"invocationId\":\"{s}\"}}", .{inv_id}) catch "{}";
                return CdpResponse.ok(req.id, res);
            }
            return CdpResponse.err(req.id, -32602, "WebMCP: tool name is required");
        }

        let args_obj: []const u8 = "{}";
        if (a := extractJsonObject(req.params_raw, "arguments")) {
            args_obj = a;
        } else if (inp := extractJsonObject(req.params_raw, "input")) {
            args_obj = inp;
        } else if (ar := extractJsonObject(req.params_raw, "args")) {
            args_obj = ar;
        }

        let tools: [64]PageTool = [PageTool{}; 64];
        const tool_count = collectPageTools(&session.doc, &tools);

        let matched_idx: (usize)? = null;
        let i: usize = 0;
        while (i < tool_count) : (i += 1) {
            if (std.mem.eql(u8, tools[i].getName(), tool_name)) {
                matched_idx = i;
                break;
            }
        }

        let result_buf: [512]u8 = [0; 512];
        let result_len: usize = 0;
        let is_error = false;

        if (idx := matched_idx) {
            const tool = &tools[idx];
            if (tool.action_type == 0) {
                result_len = executeClickAction(session, tool, &result_buf);
            } else if (tool.action_type == 1) {
                result_len = executeSubmitAction(session, tool, args_obj, &result_buf);
            } else {
                let val_str: []const u8 = "";
                if (v := extractJsonString(args_obj, "value")) {
                    val_str = v;
                } else if (v2 := extractJsonString(req.params_raw, "value")) {
                    val_str = v2;
                }
                result_len = executeInputAction(session, tool, val_str, &result_buf);
            }
        } else {
            let js_probe_buf: [512]u8 = [0; 512];
            const probe_script = std.fmt.bufPrint(
                &js_probe_buf,
                "(function(){{ if(typeof document !== 'undefined' && document.modelContext) return 'mcp'; if(typeof window !== 'undefined' && typeof window['{s}'] === 'function') return 'fn'; return 'none'; }})()",
                .{tool_name},
            ) catch "";

            let probe_res_buf: [256]u8 = [0; 256];
            let probe_len: usize = 0;
            if (probe_script.len > 0) {
                probe_len = session.evaluate(probe_script, &probe_res_buf);
            }

            if (probe_len > 0 && !std.mem.containsAtLeast(u8, probe_res_buf[0..probe_len], 1, "none")) {
                let js_call_buf: [512]u8 = [0; 512];
                const call_script = std.fmt.bufPrint(
                    &js_call_buf,
                    "(function(){{ if(typeof window['{s}'] === 'function') return String(window['{s}']({s})); return 'Executed'; }})()",
                    .{ tool_name, tool_name, args_obj },
                ) catch "";
                let call_out_buf: [256]u8 = [0; 256];
                const call_len = session.evaluate(call_script, &call_out_buf);
                const out_slice = if (call_len > 0) call_out_buf[0..call_len] else "Executed";
                const res = std.fmt.bufPrint(&result_buf, "Tool {s} executed: {s}", .{ tool_name, out_slice }) catch "Tool executed";
                result_len = res.len;
            } else {
                is_error = true;
                const res = std.fmt.bufPrint(&result_buf, "no such tool: {s}", .{tool_name}) catch "no such tool";
                result_len = res.len;
            }
        }

        const result_text = result_buf[0..result_len];
        let esc_res_buf: [512]u8 = [0; 512];
        const esc_result = escapeJson(result_text, &esc_res_buf);

        if (is_invoke) {
            let ev_res: []const u8 = "";
            if (is_error) {
                ev_res = std.fmt.bufPrint(
                    &g_event_params_buf,
                    "{{\"invocationId\":\"{s}\",\"status\":\"Error\",\"errorText\":\"{s}\"}}",
                    .{ inv_id, esc_result },
                ) catch "{\"status\":\"Error\"}";
            } else {
                ev_res = std.fmt.bufPrint(
                    &g_event_params_buf,
                    "{{\"invocationId\":\"{s}\",\"status\":\"Completed\",\"output\":{{\"content\":[{{\"type\":\"text\",\"text\":\"{s}\"}}]}}}}",
                    .{ inv_id, esc_result },
                ) catch "{\"status\":\"Completed\"}";
            }
            out_events[0] = CdpEvent.create("WebMCP.toolResponded", ev_res);
            event_count.* = 1;

            const res = std.fmt.bufPrint(
                out_buf,
                "{{\"invocationId\":\"{s}\"}}",
                .{inv_id},
            ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        const res = std.fmt.bufPrint(
            out_buf,
            "{{\"content\":[{{\"type\":\"text\",\"text\":\"{s}\"}}],\"isError\":{s}}}",
            .{ esc_result, if (is_error) "true" else "false" },
        ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
        return CdpResponse.ok(req.id, res);
    }

    static fn handle(
        session: *CdpPageSession,
        req: CdpRequest,
        out_buf: []u8,
        out_events: *[1]CdpEvent,
        event_count: *usize,
    ): CdpResponse {
        event_count.* = 0;

        if (std.mem.eql(u8, req.method, "WebMCP.enable")) {
            g_webmcp_state.enable();
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "WebMCP.disable")) {
            g_webmcp_state.disable();
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "WebMCP.listTools") ||
            std.mem.eql(u8, req.method, "WebMCP.getTools")) {
            return WebMcpHandler.handleListTools(session, req, out_buf);
        }

        if (std.mem.eql(u8, req.method, "WebMCP.callTool") ||
            std.mem.eql(u8, req.method, "WebMCP.invokeTool")) {
            return WebMcpHandler.handleCallTool(session, req, out_buf, out_events, event_count);
        }

        if (std.mem.eql(u8, req.method, "WebMCP.cancelInvocation")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.err(req.id, -32601, "'WebMCP' domain method not implemented");
    }
}
