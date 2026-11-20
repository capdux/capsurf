// src/js/dom_binding.cp
// CapSurf QuickJS DOM API Binding Layer
// Mounts W3C DOM Living Standard APIs onto QuickJS-NG for native headless execution.

import {
    JSRuntime,
    JSContext,
    JSValue,
    JSClassDef,
    JsRuntime,
    JsContext,
    jsNull,
    jsUndefined,
    jsTrue,
    jsFalse,
    jsBool,
    jsToBool,
    JS_TAG_NULL,
    JS_TAG_UNDEFINED,
    jsInt,
    jsFloat,
    jsException,
    jsIsException,
    jsIsUndefined,
    jsToSlice,
    jsNewString,
    defineMethod,
    defineGetterSetter,
    JS_NewClassID,
    JS_NewClass,
    JS_NewObjectClass,
    JS_SetClassProto,
    JS_SetOpaque,
    JS_GetOpaque,
    JS_SetPropertyStr,
    JS_GetPropertyStr,
    JS_SetPropertyUint32,
    JS_NewObject,
    JS_NewArray,
    JS_GetGlobalObject,
    JS_DupValue,
    JS_FreeValue,
    JS_SetContextOpaque,
    JS_GetContextOpaque,
    JS_DefinePropertyValueStr,
    JS_Eval,
    JS_ToCStringLen2,
    JS_FreeCString,
    JS_ToInt32,
    JS_ToFloat64,
} from "./qjs_engine.cp";

import { Document, NodeId, NULL_NODE, NodeType } from "../dom/dom.cp";
import { Matcher } from "../css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle, Position } from "../css/cascade.cp";
import { LayoutTree, Rect } from "../layout/layout.cp";
import { RUNTIME_CHUNKS } from "./runtime_source.cp";
import { MockFixtures } from "../net/mock_fixtures.cp";
import { CookieJar } from "../net/cookies.cp";

pub @c {
    struct TimeVal {
        tv_sec: i64;
        tv_usec: i64;
    }
    fn gettimeofday(tv: *TimeVal, tz: *!?): i32;
}

pub fn getMilliTimestamp(): i64 {
    let tv = TimeVal{ tv_sec: 0, tv_usec: 0 };
    * = gettimeofday(&tv, null);
    return tv.tv_sec * 1000 + @divTrunc(tv.tv_usec, 1000);
}

pub threadlocal let g_element_class_id: u32 = 0;
pub threadlocal let g_document_class_id: u32 = 0;

pub struct ConsoleRecord {
    level: [16]u8 = [0; 16],
    level_len: usize = 0,
    text: [512]u8 = [0; 512],
    text_len: usize = 0,
    args_json: [2048]u8 = [0; 2048],
    args_json_len: usize = 0,
    timestamp: i64 = 0,

    fn getLevel(*_): []const u8 {
        return _.level[0.._.level_len];
    }

    fn getText(*_): []const u8 {
        return _.text[0.._.text_len];
    }

    fn getArgsJson(*_): []const u8 {
        return _.args_json[0.._.args_json_len];
    }
}

pub const MAX_CONSOLE_RECORDS: usize = 16;
pub const MAX_FETCH_RECORDS: usize = 16;
pub const MAX_DOM_MUTATIONS: usize = 1024;

pub enum DomMutationKind {
    AttrModified,
    AttrRemoved,
    CharDataModified,
    ChildInserted,
    ChildRemoved,
    InnerHtmlRebuilt,
}

pub struct DomMutationRecord {
    kind: DomMutationKind = .AttrModified,
    node_id: NodeId = NULL_NODE,
    parent_id: NodeId = NULL_NODE,
    prev_node_id: NodeId = NULL_NODE,
    name_buf: [128]u8 = [0; 128],
    name_len: usize = 0,
    val_buf: [512]u8 = [0; 512],
    val_len: usize = 0,

    fn getName(): []const u8 {
        return _.name_buf[0.._.name_len];
    }

    fn getValue(): []const u8 {
        return _.val_buf[0.._.val_len];
    }
}

pub struct FetchRecord {
    url_buf: [512]u8 = [0; 512],
    url_len: usize = 0,
    mime_buf: [64]u8 = [0; 64],
    mime_len: usize = 0,
    frame_id_buf: [64]u8 = [0; 64],
    frame_id_len: usize = 0,
    loader_id_buf: [64]u8 = [0; 64],
    loader_id_len: usize = 0,

    fn getUrl(*_): []const u8 {
        return _.url_buf[0.._.url_len];
    }

    fn getMimeType(*_): []const u8 {
        return if (_.mime_len > 0) _.mime_buf[0.._.mime_len] else "application/octet-stream";
    }

    fn getFrameId(*_): []const u8 {
        return _.frame_id_buf[0.._.frame_id_len];
    }

    fn getLoaderId(*_): []const u8 {
        return _.loader_id_buf[0.._.loader_id_len];
    }
}

pub struct JsDomContext {
    doc: *Document,
    layout: (*LayoutTree)? = null,
    layout_dirty: bool = false,
    paint_prepares: u64 = 0,
    cookie_jar: (*CookieJar)? = null,
    extra_headers: []const u8 = "",
    element_class_id: u32 = 0,
    document_class_id: u32 = 0,
    console_records: [MAX_CONSOLE_RECORDS]ConsoleRecord = [ConsoleRecord{}; MAX_CONSOLE_RECORDS],
    console_count: usize = 0,
    fetch_records: [MAX_FETCH_RECORDS]FetchRecord = [FetchRecord{}; MAX_FETCH_RECORDS],
    fetch_count: usize = 0,
    frame_id_buf: [64]u8 = [0; 64],
    frame_id_len: usize = 0,
    loader_id_buf: [64]u8 = [0; 64],
    loader_id_len: usize = 0,
    dom_events_enabled: bool = false,
    dom_mutations: std.ArrayListUnmanaged(DomMutationRecord) = std.ArrayListUnmanaged(DomMutationRecord).empty,

    fn recordDomMutation(*_, rec: DomMutationRecord) {
        if (!_.dom_events_enabled) return;
        if (_.dom_mutations.items.len >= MAX_DOM_MUTATIONS) return;
        _.dom_mutations.append(std.heap.c_allocator, rec) catch {};
    }

    fn recordAttrModified(*_, node_id: NodeId, name: []const u8, value: []const u8) {
        if (!_.dom_events_enabled) return;
        let rec = DomMutationRecord{
            kind: .AttrModified,
            node_id: node_id,
        };
        const n_len = @min(name.len, rec.name_buf.len);
        @memcpy(rec.name_buf[0..n_len], name[0..n_len]);
        rec.name_len = n_len;
        const v_len = @min(value.len, rec.val_buf.len);
        @memcpy(rec.val_buf[0..v_len], value[0..v_len]);
        rec.val_len = v_len;
        _.recordDomMutation(rec);
    }

    fn recordAttrRemoved(*_, node_id: NodeId, name: []const u8) {
        if (!_.dom_events_enabled) return;
        let rec = DomMutationRecord{
            kind: .AttrRemoved,
            node_id: node_id,
        };
        const n_len = @min(name.len, rec.name_buf.len);
        @memcpy(rec.name_buf[0..n_len], name[0..n_len]);
        rec.name_len = n_len;
        _.recordDomMutation(rec);
    }

    fn recordCharDataModified(*_, node_id: NodeId, data: []const u8) {
        if (!_.dom_events_enabled) return;
        let rec = DomMutationRecord{
            kind: .CharDataModified,
            node_id: node_id,
        };
        const v_len = @min(data.len, rec.val_buf.len);
        @memcpy(rec.val_buf[0..v_len], data[0..v_len]);
        rec.val_len = v_len;
        _.recordDomMutation(rec);
    }

    fn recordChildInserted(*_, parent_id: NodeId, prev_node_id: NodeId, node_id: NodeId) {
        if (!_.dom_events_enabled) return;
        _.recordDomMutation(DomMutationRecord{
            kind: .ChildInserted,
            node_id: node_id,
            parent_id: parent_id,
            prev_node_id: prev_node_id,
        });
    }

    fn recordChildRemoved(*_, parent_id: NodeId, node_id: NodeId) {
        if (!_.dom_events_enabled) return;
        _.recordDomMutation(DomMutationRecord{
            kind: .ChildRemoved,
            node_id: node_id,
            parent_id: parent_id,
        });
    }

    fn recordInnerHtmlRebuilt(*_, node_id: NodeId) {
        if (!_.dom_events_enabled) return;
        _.recordDomMutation(DomMutationRecord{
            kind: .InnerHtmlRebuilt,
            node_id: node_id,
        });
    }

    fn clearDomMutations(*_) {
        _.dom_mutations.clearRetainingCapacity();
    }

    fn deinitDomMutations(*_) {
        _.dom_mutations.deinit(std.heap.c_allocator);
        _.dom_mutations = std.ArrayListUnmanaged(DomMutationRecord).empty;
    }

    fn addFetchRecord(*_, url: []const u8, mime_type: []const u8) {
        if (_.fetch_count >= MAX_FETCH_RECORDS) return;
        let rec = &_.fetch_records[_.fetch_count];
        const u_len = @min(url.len, rec.url_buf.len);
        @memcpy(rec.url_buf[0..u_len], url[0..u_len]);
        rec.url_len = u_len;

        const m_len = @min(mime_type.len, rec.mime_buf.len);
        @memcpy(rec.mime_buf[0..m_len], mime_type[0..m_len]);
        rec.mime_len = m_len;

        const f_len = @min(_.frame_id_len, rec.frame_id_buf.len);
        @memcpy(rec.frame_id_buf[0..f_len], _.frame_id_buf[0..f_len]);
        rec.frame_id_len = f_len;

        const l_len = @min(_.loader_id_len, rec.loader_id_buf.len);
        @memcpy(rec.loader_id_buf[0..l_len], _.loader_id_buf[0..l_len]);
        rec.loader_id_len = l_len;

        _.fetch_count += 1;
    }

    fn clearFetchRecords(*_) {
        _.fetch_count = 0;
    }

    fn addConsoleRecord(*_, level: []const u8, text: []const u8, args_json: []const u8) {
        if (_.console_count >= MAX_CONSOLE_RECORDS) {
            let j: usize = 1;
            while (j < MAX_CONSOLE_RECORDS) : (j += 1) {
                _.console_records[j - 1] = _.console_records[j];
            }
            _.console_count = MAX_CONSOLE_RECORDS - 1;
        }
        let rec = &_.console_records[_.console_count];
        const l_len = @min(level.len, rec.level.len);
        @memcpy(rec.level[0..l_len], level[0..l_len]);
        rec.level_len = l_len;

        const t_len = @min(text.len, rec.text.len);
        @memcpy(rec.text[0..t_len], text[0..t_len]);
        rec.text_len = t_len;

        const a_len = @min(args_json.len, rec.args_json.len);
        @memcpy(rec.args_json[0..a_len], args_json[0..a_len]);
        rec.args_json_len = a_len;

        rec.timestamp = getMilliTimestamp();
        _.console_count += 1;
    }

    fn clearConsoleRecords(*_) {
        _.console_count = 0;
    }
}

pub struct JsElement {
    dom_ctx: *JsDomContext,
    node_id: NodeId,
}

fn jsIsObject(v: JSValue): bool {
    return v.tag == -1;
}

pub fn createJsElement(ctx: *JSContext, dom_ctx: *JsDomContext, node_id: NodeId): JSValue {
    if (node_id == NULL_NODE) {
        return jsNull();
    }
    // 包装器身份缓存：同一 (文档, 节点) 始终返回同一 JS 对象，保证 === 语义
    let key_buf: [64]u8 = undefined;
    const key = std.fmt.bufPrint(&key_buf, "w{d}:{d}", .{ @intFromPtr(dom_ctx), node_id }) catch "";
    let key_z: [65]u8 = undefined;
    if (key.len > 0) {
        @memcpy(key_z[0..key.len], key);
        key_z[key.len] = 0;
    }
    const global_obj = JS_GetGlobalObject(ctx);
    defer JS_FreeValue(ctx, global_obj);
    let cache = JS_GetPropertyStr(ctx, global_obj, "__capsurf_wrappers");
    if (!jsIsObject(cache)) {
        JS_FreeValue(ctx, cache);
        cache = JS_NewObject(ctx);
        * = JS_DefinePropertyValueStr(ctx, global_obj, "__capsurf_wrappers", JS_DupValue(ctx, cache), 1);
    }
    defer JS_FreeValue(ctx, cache);
    if (key.len > 0) {
        const hit = JS_GetPropertyStr(ctx, cache, @ptrCast(&key_z));
        if (jsIsObject(hit)) {
            return hit;
        }
        JS_FreeValue(ctx, hit);
    }
    const el_val = JS_NewObjectClass(ctx, dom_ctx.element_class_id);
    if (jsIsException(el_val)) {
        return el_val;
    }
    let el = std.heap.c_allocator.create(JsElement) catch return jsNull();
    el.* = JsElement {
        dom_ctx: dom_ctx,
        node_id: node_id,
    };
    * = JS_SetOpaque(el_val, @ptrCast(el));
    * = JS_SetPropertyStr(ctx, el_val, "_nid", jsInt(@intCast(node_id)));
    if (key.len > 0) {
        * = JS_SetPropertyStr(ctx, cache, @ptrCast(&key_z), JS_DupValue(ctx, el_val));
    }
    return el_val;
}

fn getNodeText(doc: *Document, node_id: NodeId): []const u8 {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return "";
    const node = &doc.nodes.items[@as(usize, node_id)];
    if (node.node_type == NodeType.Text) {
        return node.text_content;
    }
    if (node.text_content.len > 0) {
        return node.text_content;
    }
    let child = node.first_child;
    if (child == NULL_NODE) return "";
    if (doc.nodes.items[@as(usize, child)].next_sibling == NULL_NODE &&
        doc.nodes.items[@as(usize, child)].node_type == NodeType.Text) {
        return doc.nodes.items[@as(usize, child)].text_content;
    }
    let buf = std.ArrayListUnmanaged(u8).empty;
    errdefer buf.deinit(doc.allocator);
    while (child != NULL_NODE) {
        const cnode = &doc.nodes.items[@as(usize, child)];
        if (cnode.node_type == NodeType.Text) {
            buf.appendSlice(doc.allocator, cnode.text_content) catch {};
        } else if (cnode.node_type == NodeType.Element) {
            const sub = getNodeText(doc, child);
            if (sub.len > 0) {
                buf.appendSlice(doc.allocator, sub) catch {};
            }
        }
        child = cnode.next_sibling;
    }
    return buf.toOwnedSlice(doc.allocator) catch "";
}

fn setNodeText(doc: *Document, node_id: NodeId, text: []const u8) {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return;
    while (doc.nodes.items[@as(usize, node_id)].first_child != NULL_NODE) {
        doc.removeChild(doc.nodes.items[@as(usize, node_id)].first_child);
    }
    doc.nodes.items[@as(usize, node_id)].text_content = "";
    if (text.len == 0) return;
    const text_id = doc.createTextNode(text) catch return;
    doc.appendChild(node_id, text_id);
}

@c fn js_element_finalizer(rt: *JSRuntime, val: JSValue) {
    * = rt;
    const ptr = JS_GetOpaque(val, g_element_class_id);
    if (ptr != null) {
        const el: *JsElement = @ptrCast(@alignCast(ptr.?));
        std.heap.c_allocator.destroy(el);
    }
}

@c fn js_element_get_attribute(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsNull();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const attr_name = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(attr_name);
    if (@as(usize, el.node_id) >= el.dom_ctx.doc.nodes.items.len) return jsNull();
    const node = &el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)];
    if (val := node.getAttribute(attr_name)) {
        return jsNewString(ctx, val);
    }
    return jsNull();
}

@c fn js_element_set_attribute(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 2) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const attr_name = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(attr_name);
    const attr_val = jsToSlice(ctx, argv[1]);
    defer std.heap.page_allocator.free(attr_val);
    const attr_name_dup = el.dom_ctx.doc.allocator.dupe(u8, attr_name) catch attr_name;
    const attr_val_dup = el.dom_ctx.doc.allocator.dupe(u8, attr_val) catch attr_val;
    el.dom_ctx.doc.setAttribute(el.node_id, attr_name_dup, attr_val_dup);
    el.dom_ctx.recordAttrModified(el.node_id, attr_name_dup, attr_val_dup);
    el.dom_ctx.layout_dirty = true;
    if (std.ascii.eqlIgnoreCase(attr_name, "style")) {
        if (lt := el.dom_ctx.layout) {
            if (@as(usize, el.node_id) < lt.boxes.items.len) {
                if (el.dom_ctx.doc.isNodeDisplayNone(el.node_id)) {
                    lt.boxes.items[@as(usize, el.node_id)].rect = Rect.zero();
                }
            }
        }
    }
    return jsUndefined();
}

@c fn js_element_remove_attribute(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const attr_name = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(attr_name);
    if (el.dom_ctx.doc.removeAttribute(el.node_id, attr_name)) {
        el.dom_ctx.recordAttrRemoved(el.node_id, attr_name);
    }
    el.dom_ctx.layout_dirty = true;
    return jsUndefined();
}

@c fn js_element_get_raw_attrs(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc; * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    if (@as(usize, el.node_id) >= el.dom_ctx.doc.nodes.items.len) return jsNull();
    const node = &el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)];
    const arr = JS_NewArray(ctx);
    let i: usize = 0;
    while (i < node.attr_count) : (i += 1) {
        const item_obj = JS_NewObject(ctx);
        * = JS_SetPropertyStr(ctx, item_obj, "name", jsNewString(ctx, node.attrs[i].name));
        * = JS_SetPropertyStr(ctx, item_obj, "value", jsNewString(ctx, node.attrs[i].value));
        * = JS_SetPropertyUint32(ctx, arr, @intCast(i), item_obj);
    }
    return arr;
}

@c fn js_element_has_attribute(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsFalse();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsFalse();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const attr_name = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(attr_name);
    if (@as(usize, el.node_id) >= el.dom_ctx.doc.nodes.items.len) return jsFalse();
    const node = &el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)];
    return jsBool(node.getAttribute(attr_name) != null);
}

@c fn js_element_append_child(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const child_ptr = JS_GetOpaque(argv[0], g_element_class_id);
    if (child_ptr == null) return jsUndefined();
    const child_el: *JsElement = @ptrCast(@alignCast(child_ptr.?));
    if (@as(usize, child_el.node_id) < el.dom_ctx.doc.nodes.items.len) {
        const old_parent = el.dom_ctx.doc.nodes.items[@as(usize, child_el.node_id)].parent;
        if (old_parent != NULL_NODE) {
            el.dom_ctx.recordChildRemoved(old_parent, child_el.node_id);
        }
    }
    el.dom_ctx.doc.appendChild(el.node_id, child_el.node_id);
    let prev_id: NodeId = NULL_NODE;
    if (@as(usize, child_el.node_id) < el.dom_ctx.doc.nodes.items.len) {
        prev_id = el.dom_ctx.doc.nodes.items[@as(usize, child_el.node_id)].prev_sibling;
    }
    el.dom_ctx.recordChildInserted(el.node_id, prev_id, child_el.node_id);
    return JS_DupValue(ctx, argv[0]);
}

@c fn js_element_insert_before(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const child_ptr = JS_GetOpaque(argv[0], g_element_class_id);
    if (child_ptr == null) return jsUndefined();
    const child_el: *JsElement = @ptrCast(@alignCast(child_ptr.?));

    let ref_node_id: NodeId = NULL_NODE;
    if (argc >= 2) {
        const ref_ptr = JS_GetOpaque(argv[1], g_element_class_id);
        if (ref_ptr != null) {
            const ref_el: *JsElement = @ptrCast(@alignCast(ref_ptr.?));
            ref_node_id = ref_el.node_id;
        }
    }
    if (@as(usize, child_el.node_id) < el.dom_ctx.doc.nodes.items.len) {
        const old_parent = el.dom_ctx.doc.nodes.items[@as(usize, child_el.node_id)].parent;
        if (old_parent != NULL_NODE) {
            el.dom_ctx.recordChildRemoved(old_parent, child_el.node_id);
        }
    }
    el.dom_ctx.doc.insertBefore(el.node_id, child_el.node_id, ref_node_id);
    let prev_id: NodeId = NULL_NODE;
    if (@as(usize, child_el.node_id) < el.dom_ctx.doc.nodes.items.len) {
        prev_id = el.dom_ctx.doc.nodes.items[@as(usize, child_el.node_id)].prev_sibling;
    }
    el.dom_ctx.recordChildInserted(el.node_id, prev_id, child_el.node_id);
    return JS_DupValue(ctx, argv[0]);
}

@c fn js_element_remove_child(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const child_ptr = JS_GetOpaque(argv[0], g_element_class_id);
    if (child_ptr == null) return jsUndefined();
    const child_el: *JsElement = @ptrCast(@alignCast(child_ptr.?));
    let parent_id: NodeId = el.node_id;
    if (@as(usize, child_el.node_id) < el.dom_ctx.doc.nodes.items.len) {
        const p = el.dom_ctx.doc.nodes.items[@as(usize, child_el.node_id)].parent;
        if (p != NULL_NODE) parent_id = p;
    }
    el.dom_ctx.doc.removeChild(child_el.node_id);
    el.dom_ctx.recordChildRemoved(parent_id, child_el.node_id);
    return JS_DupValue(ctx, argv[0]);
}

@c fn js_element_replace_child(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 2) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const new_ptr = JS_GetOpaque(argv[0], g_element_class_id);
    if (new_ptr == null) return jsUndefined();
    const new_el: *JsElement = @ptrCast(@alignCast(new_ptr.?));
    const old_ptr = JS_GetOpaque(argv[1], g_element_class_id);
    if (old_ptr == null) return jsUndefined();
    const old_el: *JsElement = @ptrCast(@alignCast(old_ptr.?));
    if (@as(usize, new_el.node_id) < el.dom_ctx.doc.nodes.items.len) {
        const old_parent = el.dom_ctx.doc.nodes.items[@as(usize, new_el.node_id)].parent;
        if (old_parent != NULL_NODE) {
            el.dom_ctx.recordChildRemoved(old_parent, new_el.node_id);
        }
    }
    el.dom_ctx.recordChildRemoved(el.node_id, old_el.node_id);
    el.dom_ctx.doc.replaceChild(el.node_id, new_el.node_id, old_el.node_id);
    let prev_id: NodeId = NULL_NODE;
    if (@as(usize, new_el.node_id) < el.dom_ctx.doc.nodes.items.len) {
        prev_id = el.dom_ctx.doc.nodes.items[@as(usize, new_el.node_id)].prev_sibling;
    }
    el.dom_ctx.recordChildInserted(el.node_id, prev_id, new_el.node_id);
    return JS_DupValue(ctx, argv[1]);
}

struct ViewportRectInfo {
    rect: Rect = Rect.zero(),
    win_x: f32 = 0.0,
    win_y: f32 = 0.0,
    anc_scroll_x: f32 = 0.0,
    anc_scroll_y: f32 = 0.0,
}

fn ensureLayoutUpToDate(dom_ctx: *JsDomContext) {
    if (!dom_ctx.layout_dirty) return;
    dom_ctx.layout_dirty = false;
    if (lt := dom_ctx.layout) {
        const doc = dom_ctx.doc;
        const count = @max(@as(usize, 1), doc.nodeCount());
        let css_buf = std.heap.page_allocator.alloc(u8, 65536) catch null;
        defer if (css_buf != null) std.heap.page_allocator.free(css_buf.?);
        let css_len: usize = 0;
        if (css_buf != null) {
            const base_css = "";
            @memcpy(css_buf.?[0..base_css.len], base_css);
            css_len = base_css.len;
            let snid: usize = 0;
            while (snid < doc.nodes.items.len) : (snid += 1) {
                const sn = &doc.nodes.items[snid];
                if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style")) {
                    if (sn.text_content.len > 0) {
                        const copy_len = @min(sn.text_content.len, css_buf.?.len - css_len);
                        @memcpy(css_buf.?[css_len .. css_len + copy_len], sn.text_content[0..copy_len]);
                        css_len += copy_len;
                    }
                    let child = sn.first_child;
                    while (child != NULL_NODE && @as(usize, child) < doc.nodes.items.len) : (child = doc.nodes.items[@as(usize, child)].next_sibling) {
                        const scn = &doc.nodes.items[@as(usize, child)];
                        if (scn.node_type == NodeType.Text && scn.text_content.len > 0) {
                            const copy_len = @min(scn.text_content.len, css_buf.?.len - css_len);
                            @memcpy(css_buf.?[css_len .. css_len + copy_len], scn.text_content[0..copy_len]);
                            css_len += copy_len;
                        }
                    }
                }
            }
        }
        const full_css = if (css_buf != null) css_buf.?[0..css_len] else "";
        const sheet = StyleSheet.parse(full_css, lt.viewport_width, false);
        let styles = std.heap.page_allocator.alloc(ComputedStyle, count) catch return;
        defer std.heap.page_allocator.free(styles);
        Cascade.computeStyles(doc, &sheet, styles, 16.0);
        lt.computeLayout(doc, styles) catch {};
        dom_ctx.paint_prepares += 1;
    }
}

fn getJsNumberProp(ctx: *JSContext, obj: JSValue, prop: [*c]const u8): f32 {
    const val = JS_GetPropertyStr(ctx, obj, prop);
    defer JS_FreeValue(ctx, val);
    if (val.tag == JS_TAG_UNDEFINED || val.tag == JS_TAG_NULL) return 0.0;
    let num: f64 = 0.0;
    if (JS_ToFloat64(ctx, &num, val) == 0 && !std.math.isNan(num) && !std.math.isInf(num)) {
        return @floatCast(num);
    }
    return 0.0;
}

fn getNodeScrollOffsets(ctx: *JSContext, cache: JSValue, dom_ctx: *JsDomContext, node_id: NodeId, out_sx: *f32, out_sy: *f32) {
    out_sx.* = 0.0;
    out_sy.* = 0.0;
    if (!jsIsObject(cache)) return;
    let key_buf: [64]u8 = undefined;
    const key = std.fmt.bufPrint(&key_buf, "w{d}:{d}", .{ @intFromPtr(dom_ctx), node_id }) catch return;
    if (key.len == 0) return;
    let key_z: [65]u8 = undefined;
    @memcpy(key_z[0..key.len], key);
    key_z[key.len] = 0;
    const el_val = JS_GetPropertyStr(ctx, cache, @ptrCast(&key_z));
    defer JS_FreeValue(ctx, el_val);
    if (!jsIsObject(el_val)) return;
    out_sx.* = getJsNumberProp(ctx, el_val, "_scrollLeft");
    out_sy.* = getJsNumberProp(ctx, el_val, "_scrollTop");
}

fn computeViewportRect(ctx: *JSContext, dom_ctx: *JsDomContext, lt: *LayoutTree, node_id: NodeId, ignore_sticky: bool): ViewportRectInfo {
    let info = ViewportRectInfo{};
    if (node_id == NULL_NODE || @as(usize, node_id) >= lt.boxes.items.len) return info;

    const doc = dom_ctx.doc;
    const global_obj = JS_GetGlobalObject(ctx);
    defer JS_FreeValue(ctx, global_obj);

    let win_x = getJsNumberProp(ctx, global_obj, "scrollX");
    if (win_x == 0.0) win_x = getJsNumberProp(ctx, global_obj, "pageXOffset");
    let win_y = getJsNumberProp(ctx, global_obj, "scrollY");
    if (win_y == 0.0) win_y = getJsNumberProp(ctx, global_obj, "pageYOffset");
    info.win_x = win_x;
    info.win_y = win_y;

    // Clamp document scroll offsets for layout/sticky calculation to actual scrollable document bounds
    let max_doc_w: f32 = lt.viewport_width;
    let max_doc_h: f32 = lt.viewport_height;
    let scan_i: usize = 1;
    while (scan_i < doc.nodes.items.len && scan_i < lt.boxes.items.len) : (scan_i += 1) {
        const sn = &doc.nodes.items[scan_i];
        if (sn.node_type == NodeType.Element && (sn.hasTag("html") || sn.hasTag("body"))) {
            const sb = &lt.boxes.items[scan_i];
            const bw = @max(sb.rect.width, @max(sb.content_width, sb.scroll_content_width));
            const bh = @max(sb.rect.height, @max(sb.content_height, sb.scroll_content_height));
            if (bw > max_doc_w) max_doc_w = bw;
            if (bh > max_doc_h) max_doc_h = bh;
        }
    }
    const max_scroll_x = @max(0.0, max_doc_w - lt.viewport_width);
    const max_scroll_y = @max(0.0, max_doc_h - lt.viewport_height);
    if (win_x < 0.0) win_x = 0.0;
    if (win_x > max_scroll_x) win_x = max_scroll_x;
    if (win_y < 0.0) win_y = 0.0;
    if (win_y > max_scroll_y) win_y = max_scroll_y;

    const cache = JS_GetPropertyStr(ctx, global_obj, "__capsurf_wrappers");
    defer JS_FreeValue(ctx, cache);

    // Collect ancestor chain from root element down to node_id
    let rev_chain: [64]NodeId = [NULL_NODE; 64];
    let depth: usize = 0;
    let cur = node_id;
    while (cur != NULL_NODE && @as(usize, cur) < doc.nodes.items.len && depth < 64) {
        if (doc.nodes.items[@as(usize, cur)].node_type == NodeType.Element) {
            rev_chain[depth] = cur;
            depth += 1;
        }
        cur = doc.nodes.items[@as(usize, cur)].parent;
    }
    if (depth == 0) {
        info.rect = lt.getBoundingClientRect(node_id);
        return info;
    }

    let chain: [64]NodeId = [NULL_NODE; 64];
    let node_sx: [64]f32 = [0.0; 64];
    let node_sy: [64]f32 = [0.0; 64];
    let vp_x: [64]f32 = [0.0; 64];
    let vp_y: [64]f32 = [0.0; 64];

    let ci: usize = 0;
    while (ci < depth) : (ci += 1) {
        const nid = rev_chain[depth - 1 - ci];
        chain[ci] = nid;
        getNodeScrollOffsets(ctx, cache, dom_ctx, nid, &node_sx[ci], &node_sy[ci]);
    }

    let cum_scroll_x: f32 = win_x;
    let cum_scroll_y: f32 = win_y;
    let cum_sticky_x: f32 = 0.0;
    let cum_sticky_y: f32 = 0.0;
    let anc_scroll_x: f32 = 0.0;
    let anc_scroll_y: f32 = 0.0;

    let i: usize = 0;
    while (i < depth) : (i += 1) {
        const nid = chain[i];
        const box = &lt.boxes.items[@as(usize, nid)];
        const node = &doc.nodes.items[@as(usize, nid)];
        let vx: f32 = box.rect.x;
        let vy: f32 = box.rect.y;

        if (box.position == Position.Fixed) {
            // Fixed boxes escape ancestor scroll and sticky offsets
            vx = box.rect.x;
            vy = box.rect.y;
            cum_scroll_x = 0.0;
            cum_scroll_y = 0.0;
            cum_sticky_x = 0.0;
            cum_sticky_y = 0.0;
        } else {
            const base_x = box.rect.x + cum_sticky_x - cum_scroll_x;
            const base_y = box.rect.y + cum_sticky_y - cum_scroll_y;
            let sticky_dx: f32 = 0.0;
            let sticky_dy: f32 = 0.0;

            if (box.position == Position.Sticky && !ignore_sticky) {
                // Find nearest non-root scrollport ancestor (html/body overflow propagates to viewport)
                let sp_left: f32 = 0.0;
                let sp_top: f32 = 0.0;
                let sp_right: f32 = lt.viewport_width;
                let sp_bottom: f32 = lt.viewport_height;

                let k: usize = i;
                while (k > 0) {
                    k -= 1;
                    const anc_nid = chain[k];
                    const anc_node = &doc.nodes.items[@as(usize, anc_nid)];
                    const anc_box = &lt.boxes.items[@as(usize, anc_nid)];
                    if (!anc_node.hasTag("html") && !anc_node.hasTag("body") && !anc_box.overflow_visible) {
                        sp_left = vp_x[k] + anc_box.border_left;
                        sp_top = vp_y[k] + anc_box.border_top;
                        sp_right = vp_x[k] + anc_box.rect.width - anc_box.border_right;
                        sp_bottom = vp_y[k] + anc_box.rect.height - anc_box.border_bottom;
                        break;
                    }
                }

                // Determine sticky containing block bounds in viewport coordinates
                let cb_left: f32 = -1000000.0;
                let cb_top: f32 = -1000000.0;
                let cb_right: f32 = 1000000.0;
                let cb_bottom: f32 = 1000000.0;

                if (i > 0) {
                    const p_idx = i - 1;
                    const p_nid = chain[p_idx];
                    const p_node = &doc.nodes.items[@as(usize, p_nid)];
                    const p_box = &lt.boxes.items[@as(usize, p_nid)];
                    if (p_node.hasTag("html") || p_node.hasTag("body")) {
                        cb_left = p_box.rect.x + p_box.border_left + p_box.padding_left - win_x;
                        cb_top = p_box.rect.y + p_box.border_top + p_box.padding_top - win_y;
                        cb_right = cb_left + @max(p_box.content_width, p_box.scroll_content_width);
                        cb_bottom = cb_top + @max(p_box.content_height, p_box.scroll_content_height);
                    } else if (!p_box.overflow_visible) {
                        cb_left = vp_x[p_idx] + p_box.border_left + p_box.padding_left - node_sx[p_idx];
                        cb_top = vp_y[p_idx] + p_box.border_top + p_box.padding_top - node_sy[p_idx];
                        cb_right = cb_left + @max(p_box.content_width, p_box.scroll_content_width);
                        cb_bottom = cb_top + @max(p_box.content_height, p_box.scroll_content_height);
                    } else {
                        cb_left = vp_x[p_idx] + p_box.border_left + p_box.padding_left;
                        cb_top = vp_y[p_idx] + p_box.border_top + p_box.padding_top;
                        cb_right = cb_left + p_box.content_width;
                        cb_bottom = cb_top + p_box.content_height;
                    }
                }

                if (box.has_sticky_top) {
                    const min_y = sp_top + box.sticky_top;
                    const max_y = cb_bottom - box.rect.height;
                    if (base_y < min_y) {
                        const target_y = @min(min_y, max_y);
                        if (target_y > base_y) {
                            sticky_dy = target_y - base_y;
                        }
                    }
                }
                if (box.has_sticky_bottom) {
                    const max_y = sp_bottom - box.sticky_bottom - box.rect.height;
                    const min_y = cb_top;
                    if (base_y > max_y) {
                        const target_y = @max(max_y, min_y);
                        if (target_y < base_y) {
                            sticky_dy = target_y - base_y;
                        }
                    }
                }
                if (box.has_sticky_left) {
                    const min_x = sp_left + box.sticky_left;
                    const max_x = cb_right - box.rect.width;
                    if (base_x < min_x) {
                        const target_x = @min(min_x, max_x);
                        if (target_x > base_x) {
                            sticky_dx = target_x - base_x;
                        }
                    }
                }
                if (box.has_sticky_right) {
                    const max_x = sp_right - box.sticky_right - box.rect.width;
                    const min_x = cb_left;
                    if (base_x > max_x) {
                        const target_x = @max(max_x, min_x);
                        if (target_x < base_x) {
                            sticky_dx = target_x - base_x;
                        }
                    }
                }
            }

            vx = base_x + sticky_dx;
            vy = base_y + sticky_dy;
            cum_sticky_x += sticky_dx;
            cum_sticky_y += sticky_dy;
        }

        vp_x[i] = vx;
        vp_y[i] = vy;

        if (i + 1 < depth) {
            if (!node.hasTag("html") && !node.hasTag("body")) {
                cum_scroll_x += node_sx[i];
                cum_scroll_y += node_sy[i];
                anc_scroll_x += node_sx[i];
                anc_scroll_y += node_sy[i];
            }
        }
    }

    const last_box = &lt.boxes.items[@as(usize, node_id)];
    info.rect = Rect {
        x: vp_x[depth - 1],
        y: vp_y[depth - 1],
        width: last_box.rect.width,
        height: last_box.rect.height,
        visible: last_box.rect.visible,
    };
    info.anc_scroll_x = anc_scroll_x;
    info.anc_scroll_y = anc_scroll_y;
    return info;
}

@c fn js_element_get_bounding_client_rect(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    if (el.dom_ctx.doc.isNodeDisplayNone(el.node_id)) {
        const obj = JS_NewObject(ctx);
        * = JS_SetPropertyStr(ctx, obj, "x", jsFloat(0.0));
        * = JS_SetPropertyStr(ctx, obj, "y", jsFloat(0.0));
        * = JS_SetPropertyStr(ctx, obj, "width", jsFloat(0.0));
        * = JS_SetPropertyStr(ctx, obj, "height", jsFloat(0.0));
        * = JS_SetPropertyStr(ctx, obj, "left", jsFloat(0.0));
        * = JS_SetPropertyStr(ctx, obj, "top", jsFloat(0.0));
        * = JS_SetPropertyStr(ctx, obj, "right", jsFloat(0.0));
        * = JS_SetPropertyStr(ctx, obj, "bottom", jsFloat(0.0));
        return obj;
    }
    ensureLayoutUpToDate(el.dom_ctx);
    let rect = Rect.zero();
    if (lt := el.dom_ctx.layout) {
        const global_obj = JS_GetGlobalObject(ctx);
        const ignore_val = JS_GetPropertyStr(ctx, global_obj, "__capsurf_ignore_sticky");
        const ignore_sticky = jsToBool(ctx, ignore_val);
        JS_FreeValue(ctx, ignore_val);
        JS_FreeValue(ctx, global_obj);

        const info = computeViewportRect(ctx, el.dom_ctx, lt, el.node_id, ignore_sticky);
        // runtime.js subtracts (win_x + anc_scroll_x, win_y + anc_scroll_y) from the returned rect,
        // so return pre-compensated coordinates so the final JS rect equals info.rect exactly.
        rect = Rect {
            x: info.rect.x + info.win_x + info.anc_scroll_x,
            y: info.rect.y + info.win_y + info.anc_scroll_y,
            width: info.rect.width,
            height: info.rect.height,
            visible: info.rect.visible,
        };
    }
    const obj = JS_NewObject(ctx);
    * = JS_SetPropertyStr(ctx, obj, "x", jsFloat(@floatCast(rect.x)));
    * = JS_SetPropertyStr(ctx, obj, "y", jsFloat(@floatCast(rect.y)));
    * = JS_SetPropertyStr(ctx, obj, "width", jsFloat(@floatCast(rect.width)));
    * = JS_SetPropertyStr(ctx, obj, "height", jsFloat(@floatCast(rect.height)));
    * = JS_SetPropertyStr(ctx, obj, "left", jsFloat(@floatCast(rect.x)));
    * = JS_SetPropertyStr(ctx, obj, "top", jsFloat(@floatCast(rect.y)));
    * = JS_SetPropertyStr(ctx, obj, "right", jsFloat(@floatCast(rect.right())));
    * = JS_SetPropertyStr(ctx, obj, "bottom", jsFloat(@floatCast(rect.bottom())));
    return obj;
}

@c fn js_document_element_from_point(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 2) return jsNull();
    const opaque_ptr = JS_GetContextOpaque(ctx);
    if (opaque_ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(opaque_ptr.?));

    let px_f64: f64 = 0.0;
    let py_f64: f64 = 0.0;
    if (JS_ToFloat64(ctx, &px_f64, argv[0]) != 0 || JS_ToFloat64(ctx, &py_f64, argv[1]) != 0) {
        return jsNull();
    }
    if (std.math.isNan(px_f64) || std.math.isNan(py_f64)) return jsNull();
    const px: f32 = @floatCast(px_f64);
    const py: f32 = @floatCast(py_f64);

    ensureLayoutUpToDate(dom_ctx);
    if (lt := dom_ctx.layout) {
        if (px < 0.0 || py < 0.0 || px >= lt.viewport_width || py >= lt.viewport_height) {
            return jsNull();
        }
        const doc = dom_ctx.doc;
        let best_node: NodeId = NULL_NODE;
        let best_layer: i32 = -1;

        let nid: usize = 1;
        while (nid < doc.nodes.items.len && nid < lt.boxes.items.len) : (nid += 1) {
            const node = &doc.nodes.items[nid];
            if (node.node_type != NodeType.Element) continue;
            if (node.hasTag("head") || node.hasTag("style") || node.hasTag("script") || node.hasTag("title") || node.hasTag("meta") || node.hasTag("link")) continue;
            const node_id: NodeId = @intCast(nid);
            const box = &lt.boxes.items[nid];
            if (!box.rect.visible || box.rect.width <= 0.0 || box.rect.height <= 0.0) continue;

            const info = computeViewportRect(ctx, dom_ctx, lt, node_id, false);
            const r = info.rect;
            if (px < r.x || px >= r.x + r.width || py < r.y || py >= r.y + r.height) continue;

            // Check ancestor overflow clipping (fixed escapes all ancestor clips)
            let clipped = false;
            if (box.position != Position.Fixed) {
                let anc = node.parent;
                let guard: usize = 0;
                while (anc != NULL_NODE && @as(usize, anc) < doc.nodes.items.len && @as(usize, anc) < lt.boxes.items.len && guard < 64) : (guard += 1) {
                    const anc_node = &doc.nodes.items[@as(usize, anc)];
                    const anc_box = &lt.boxes.items[@as(usize, anc)];
                    if (anc_node.node_type == NodeType.Element && !anc_node.hasTag("html") && !anc_node.hasTag("body") && !anc_box.overflow_visible) {
                        const ar = computeViewportRect(ctx, dom_ctx, lt, anc, false).rect;
                        if (px < ar.x || px >= ar.x + ar.width || py < ar.y || py >= ar.y + ar.height) {
                            clipped = true;
                            break;
                        }
                    }
                    anc = anc_node.parent;
                }
            }
            if (clipped) continue;

            // Layer priority: fixed/absolute/sticky > in-flow; later in tree order wins within same layer
            const layer: i32 = if (box.position == Position.Fixed)
                @as(i32, 3)
            else if (box.position == Position.Absolute)
                @as(i32, 2)
            else if (box.position == Position.Sticky || box.position == Position.Relative)
                @as(i32, 1)
            else
                @as(i32, 0);

            if (layer >= best_layer) {
                best_layer = layer;
                best_node = node_id;
            }
        }

        if (best_node != NULL_NODE) {
            return createJsElement(ctx, dom_ctx, best_node);
        }
    }
    return jsNull();
}

@c fn js_element_get_id(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNewString(ctx, "");
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    if (@as(usize, el.node_id) >= el.dom_ctx.doc.nodes.items.len) return jsNewString(ctx, "");
    const node = &el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)];
    return jsNewString(ctx, node.id_attr);
}

@c fn js_element_set_id(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const new_id = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(new_id);
    const new_id_dup = el.dom_ctx.doc.allocator.dupe(u8, new_id) catch new_id;
    el.dom_ctx.doc.setAttribute(el.node_id, "id", new_id_dup);
    el.dom_ctx.recordAttrModified(el.node_id, "id", new_id_dup);
    return jsUndefined();
}

@c fn js_element_get_class_name(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNewString(ctx, "");
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    if (@as(usize, el.node_id) >= el.dom_ctx.doc.nodes.items.len) return jsNewString(ctx, "");
    const node = &el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)];
    return jsNewString(ctx, node.class_attr);
}

@c fn js_element_set_class_name(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const new_cls = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(new_cls);
    const new_cls_dup = el.dom_ctx.doc.allocator.dupe(u8, new_cls) catch new_cls;
    el.dom_ctx.doc.setAttribute(el.node_id, "class", new_cls_dup);
    el.dom_ctx.recordAttrModified(el.node_id, "class", new_cls_dup);
    return jsUndefined();
}

@c fn js_element_get_node_type(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = ctx;
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsInt(1);
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    if (@as(usize, el.node_id) >= el.dom_ctx.doc.nodes.items.len) return jsInt(1);
    const node = &el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)];
    switch (node.node_type) {
        NodeType.Document => return jsInt(9),
        NodeType.Element => return jsInt(1),
        NodeType.Text => return jsInt(3),
        NodeType.Comment => return jsInt(8),
    }
}

@c fn js_element_get_tag_name(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNewString(ctx, "");
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    if (@as(usize, el.node_id) >= el.dom_ctx.doc.nodes.items.len) return jsNewString(ctx, "");
    const node = &el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)];
    let buf: [64]u8 = [0; 64];
    const len = @min(node.tag_name.len, buf.len);
    let i: usize = 0;
    while (i < len) : (i += 1) {
        buf[i] = std.ascii.toUpper(node.tag_name[i]);
    }
    return jsNewString(ctx, buf[0..len]);
}

@c fn js_element_get_text_content(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNewString(ctx, "");
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const text = getNodeText(el.dom_ctx.doc, el.node_id);
    return jsNewString(ctx, text);
}

@c fn js_element_set_text_content(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const new_text = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(new_text);
    const new_text_dup = el.dom_ctx.doc.allocator.dupe(u8, new_text) catch new_text;
    if (@as(usize, el.node_id) < el.dom_ctx.doc.nodes.items.len) {
        const ntype = el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)].node_type;
        if (ntype == NodeType.Text || ntype == NodeType.Comment) {
            el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)].text_content = new_text_dup;
            el.dom_ctx.recordCharDataModified(el.node_id, new_text_dup);
            return jsUndefined();
        }
    }
    setNodeText(el.dom_ctx.doc, el.node_id, new_text_dup);
    el.dom_ctx.recordInnerHtmlRebuilt(el.node_id);
    return jsUndefined();
}

@c fn js_element_get_inner_html(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNewString(ctx, "");
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const html = el.dom_ctx.doc.getInnerHtml(el.node_id) catch return jsNewString(ctx, "");
    defer el.dom_ctx.doc.allocator.free(html);
    return jsNewString(ctx, html);
}

@c fn js_element_set_inner_html(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const html = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(html);
    el.dom_ctx.doc.setInnerHtml(el.node_id, html) catch return jsUndefined();
    el.dom_ctx.recordInnerHtmlRebuilt(el.node_id);
    return jsUndefined();
}

@c fn js_element_get_outer_html(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNewString(ctx, "");
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const html = el.dom_ctx.doc.getOuterHtml(el.node_id) catch return jsNewString(ctx, "");
    defer el.dom_ctx.doc.allocator.free(html);
    return jsNewString(ctx, html);
}

@c fn js_element_clone_node(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    let deep = false;
    if (argc >= 1) {
        deep = jsToBool(ctx, argv[0]);
    }
    const clone_id = el.dom_ctx.doc.cloneNode(el.node_id, deep) catch return jsNull();
    if (clone_id == NULL_NODE) return jsNull();
    return createJsElement(ctx, el.dom_ctx, clone_id);
}

@c fn js_element_get_attribute_names(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return JS_NewArray(ctx);
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const names = el.dom_ctx.doc.getAttributeNames(el.node_id) catch return JS_NewArray(ctx);
    defer el.dom_ctx.doc.allocator.free(names);
    const arr = JS_NewArray(ctx);
    let i: usize = 0;
    while (i < names.len) : (i += 1) {
        const item_str = jsNewString(ctx, names[i]);
        * = JS_SetPropertyUint32(ctx, arr, @intCast(i), item_str);
    }
    return arr;
}

@c fn js_element_get_attribute_ns(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 2) return jsNull();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));

    let ns: []const u8 = "";
    let free_ns = false;
    if (argv[0].tag != JS_TAG_NULL && argv[0].tag != JS_TAG_UNDEFINED) {
        ns = jsToSlice(ctx, argv[0]);
        free_ns = true;
    }
    defer {
        if (free_ns) std.heap.page_allocator.free(ns);
    }

    const local_name = jsToSlice(ctx, argv[1]);
    defer std.heap.page_allocator.free(local_name);

    if (val := el.dom_ctx.doc.getAttributeNS(el.node_id, ns, local_name)) {
        return jsNewString(ctx, val);
    }
    return jsNull();
}

@c fn js_element_set_attribute_ns(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 3) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));

    let ns: []const u8 = "";
    let free_ns = false;
    if (argv[0].tag != JS_TAG_NULL && argv[0].tag != JS_TAG_UNDEFINED) {
        ns = jsToSlice(ctx, argv[0]);
        free_ns = true;
    }
    defer {
        if (free_ns) std.heap.page_allocator.free(ns);
    }

    const qname = jsToSlice(ctx, argv[1]);
    defer std.heap.page_allocator.free(qname);

    const val = jsToSlice(ctx, argv[2]);
    defer std.heap.page_allocator.free(val);

    const ns_dup = if (ns.len > 0) el.dom_ctx.doc.allocator.dupe(u8, ns) catch ns else "";
    const qname_dup = el.dom_ctx.doc.allocator.dupe(u8, qname) catch qname;
    const val_dup = el.dom_ctx.doc.allocator.dupe(u8, val) catch val;

    el.dom_ctx.doc.setAttributeNS(el.node_id, ns_dup, qname_dup, val_dup);
    el.dom_ctx.recordAttrModified(el.node_id, qname_dup, val_dup);
    return jsUndefined();
}

@c fn js_element_remove_attribute_ns(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 2) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));

    let ns: []const u8 = "";
    let free_ns = false;
    if (argv[0].tag != JS_TAG_NULL && argv[0].tag != JS_TAG_UNDEFINED) {
        ns = jsToSlice(ctx, argv[0]);
        free_ns = true;
    }
    defer {
        if (free_ns) std.heap.page_allocator.free(ns);
    }

    const local_name = jsToSlice(ctx, argv[1]);
    defer std.heap.page_allocator.free(local_name);

    if (el.dom_ctx.doc.removeAttributeNS(el.node_id, ns, local_name)) {
        el.dom_ctx.recordAttrRemoved(el.node_id, local_name);
    }
    return jsUndefined();
}

@c fn js_element_get_parent_element(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const doc = el.dom_ctx.doc;
    if (el.node_id == NULL_NODE || @as(usize, el.node_id) >= doc.nodes.items.len) return jsNull();
    const node = &doc.nodes.items[@as(usize, el.node_id)];
    const parent_id = node.parent;
    if (parent_id == NULL_NODE || @as(usize, parent_id) >= doc.nodes.items.len) return jsNull();
    const parent_node = &doc.nodes.items[@as(usize, parent_id)];
    if (parent_node.node_type == NodeType.Document) return jsNull();
    return createJsElement(ctx, el.dom_ctx, parent_id);
}

@c fn js_element_get_parent_node(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const doc = el.dom_ctx.doc;
    if (el.node_id == NULL_NODE || @as(usize, el.node_id) >= doc.nodes.items.len) return jsNull();
    const node = &doc.nodes.items[@as(usize, el.node_id)];
    const parent_id = node.parent;
    if (parent_id == NULL_NODE || @as(usize, parent_id) >= doc.nodes.items.len) {
        if (el.node_id == doc.root_element && doc.root_element != NULL_NODE) {
            const global_obj = JS_GetGlobalObject(ctx);
            defer JS_FreeValue(ctx, global_obj);
            return JS_GetPropertyStr(ctx, global_obj, "document");
        }
        const custom_parent = JS_GetPropertyStr(ctx, this_val, "_parentNode");
        if (jsIsObject(custom_parent)) {
            return custom_parent;
        }
        JS_FreeValue(ctx, custom_parent);
        const frag = JS_GetPropertyStr(ctx, this_val, "_ownerFragment");
        if (jsIsObject(frag)) {
            return frag;
        }
        JS_FreeValue(ctx, frag);
        return jsNull();
    }
    const parent_node = &doc.nodes.items[@as(usize, parent_id)];
    if (parent_node.node_type == NodeType.Document) {
        const global_obj = JS_GetGlobalObject(ctx);
        defer JS_FreeValue(ctx, global_obj);
        return JS_GetPropertyStr(ctx, global_obj, "document");
    }
    return createJsElement(ctx, el.dom_ctx, parent_id);
}

@c fn js_element_set_parent_node(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc >= 1) {
        * = JS_SetPropertyStr(ctx, this_val, "_parentNode", JS_DupValue(ctx, argv[0]));
    }
    return jsUndefined();
}

@c fn js_element_get_first_child(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const doc = el.dom_ctx.doc;
    if (el.node_id == NULL_NODE || @as(usize, el.node_id) >= doc.nodes.items.len) return jsNull();
    const node = &doc.nodes.items[@as(usize, el.node_id)];
    if (node.first_child == NULL_NODE) return jsNull();
    return createJsElement(ctx, el.dom_ctx, node.first_child);
}

@c fn js_element_get_last_child(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const doc = el.dom_ctx.doc;
    if (el.node_id == NULL_NODE || @as(usize, el.node_id) >= doc.nodes.items.len) return jsNull();
    const node = &doc.nodes.items[@as(usize, el.node_id)];
    if (node.last_child == NULL_NODE) return jsNull();
    return createJsElement(ctx, el.dom_ctx, node.last_child);
}

@c fn js_element_get_previous_sibling(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const doc = el.dom_ctx.doc;
    if (el.node_id == NULL_NODE || @as(usize, el.node_id) >= doc.nodes.items.len) return jsNull();
    const node = &doc.nodes.items[@as(usize, el.node_id)];
    if (node.prev_sibling == NULL_NODE) return jsNull();
    return createJsElement(ctx, el.dom_ctx, node.prev_sibling);
}

@c fn js_element_get_next_sibling(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    const doc = el.dom_ctx.doc;
    if (el.node_id == NULL_NODE || @as(usize, el.node_id) >= doc.nodes.items.len) return jsNull();
    const node = &doc.nodes.items[@as(usize, el.node_id)];
    if (node.next_sibling == NULL_NODE) return jsNull();
    return createJsElement(ctx, el.dom_ctx, node.next_sibling);
}

@c fn js_document_finalizer(rt: *JSRuntime, val: JSValue) {
    * = rt;
    * = val;
}

@c fn js_document_query_selector(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsNull();
    const ptr = JS_GetOpaque(this_val, g_document_class_id);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    const sel_str = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(sel_str);
    const target_id = Matcher.querySelector(dom_ctx.doc, sel_str);
    if (target_id == NULL_NODE) return jsNull();
    return createJsElement(ctx, dom_ctx, target_id);
}

@c fn js_document_query_selector_all(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return JS_NewArray(ctx);
    const ptr = JS_GetOpaque(this_val, g_document_class_id);
    if (ptr == null) return JS_NewArray(ctx);
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    const sel_str = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(sel_str);
    let matched_ids: [256]NodeId = [@as(NodeId, 0); 256];
    const matched_count = Matcher.querySelectorAll(dom_ctx.doc, sel_str, &matched_ids);
    const arr = JS_NewArray(ctx);
    let i: u32 = 0;
    while (i < matched_count) : (i += 1) {
        const el_val = createJsElement(ctx, dom_ctx, matched_ids[@as(usize, i)]);
        * = JS_SetPropertyUint32(ctx, arr, i, el_val);
    }
    return arr;
}

@c fn js_document_get_element_by_id(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsNull();
    const ptr = JS_GetOpaque(this_val, g_document_class_id);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    const id_str = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(id_str);
    const target_id = dom_ctx.doc.getElementById(id_str);
    if (target_id == NULL_NODE) return jsNull();
    return createJsElement(ctx, dom_ctx, target_id);
}

@c fn js_document_create_element(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsNull();
    const ptr = JS_GetOpaque(this_val, g_document_class_id);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    const tag_str = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(tag_str);
    const tag_dup = dom_ctx.doc.allocator.dupe(u8, tag_str) catch tag_str;
    const new_id = dom_ctx.doc.createElement(tag_dup) catch return jsNull();
    return createJsElement(ctx, dom_ctx, new_id);
}

@c fn js_document_get_body(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_document_class_id);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    let i: usize = 1;
    while (i < dom_ctx.doc.nodes.items.len) : (i += 1) {
        const node = &dom_ctx.doc.nodes.items[i];
        if (node.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(node.tag_name, "body")) {
            return createJsElement(ctx, dom_ctx, @intCast(i));
        }
    }
    return jsNull();
}

@c fn js_document_get_title(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_document_class_id);
    if (ptr == null) return jsNewString(ctx, "");
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    let i: usize = 1;
    while (i < dom_ctx.doc.nodes.items.len) : (i += 1) {
        const node = &dom_ctx.doc.nodes.items[i];
        if (node.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(node.tag_name, "title")) {
            const text = getNodeText(dom_ctx.doc, @intCast(i));
            return jsNewString(ctx, text);
        }
    }
    return jsNewString(ctx, "");
}

@c fn js_document_set_title(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_document_class_id);
    if (ptr == null) return jsUndefined();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    const str = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(str);
    const str_dup = dom_ctx.doc.allocator.dupe(u8, str) catch str;

    let i: usize = 1;
    while (i < dom_ctx.doc.nodes.items.len) : (i += 1) {
        const node = &dom_ctx.doc.nodes.items[i];
        if (node.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(node.tag_name, "title")) {
            setNodeText(dom_ctx.doc, @intCast(i), str_dup);
            return jsUndefined();
        }
    }

    // No <title> found; create <title> in <head>
    let head_id: NodeId = NULL_NODE;
    let html_id: NodeId = NULL_NODE;
    i = 1;
    while (i < dom_ctx.doc.nodes.items.len) : (i += 1) {
        const node = &dom_ctx.doc.nodes.items[i];
        if (node.node_type == NodeType.Element) {
            if (std.ascii.eqlIgnoreCase(node.tag_name, "head")) {
                head_id = @intCast(i);
                break;
            } else if (std.ascii.eqlIgnoreCase(node.tag_name, "html") && html_id == NULL_NODE) {
                html_id = @intCast(i);
            }
        }
    }

    if (head_id == NULL_NODE) {
        head_id = dom_ctx.doc.createElement("head") catch return jsUndefined();
        if (html_id != NULL_NODE) {
            dom_ctx.doc.appendChild(html_id, head_id);
        } else if (dom_ctx.doc.root_element != NULL_NODE) {
            dom_ctx.doc.appendChild(dom_ctx.doc.root_element, head_id);
        }
    }

    const title_id = dom_ctx.doc.createElement("title") catch return jsUndefined();
    dom_ctx.doc.appendChild(head_id, title_id);
    setNodeText(dom_ctx.doc, title_id, str_dup);
    return jsUndefined();
}

@c fn js_document_get_document_element(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_document_class_id);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    let i: usize = 1;
    while (i < dom_ctx.doc.nodes.items.len) : (i += 1) {
        const node = &dom_ctx.doc.nodes.items[i];
        if (node.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(node.tag_name, "html")) {
            return createJsElement(ctx, dom_ctx, @intCast(i));
        }
    }
    return jsNull();
}

@c fn js_document_get_elements_by_name(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    const arr = JS_NewArray(ctx);
    if (argc < 1) return arr;
    const ptr = JS_GetOpaque(this_val, g_document_class_id);
    if (ptr == null) return arr;
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    const name_str = jsToSlice(ctx, argv[0]);
    defer std.heap.page_allocator.free(name_str);
    const ids = dom_ctx.doc.getElementsByName(name_str) catch return arr;
    defer dom_ctx.doc.allocator.free(ids);
    let i: usize = 0;
    while (i < ids.len) : (i += 1) {
        const el_val = createJsElement(ctx, dom_ctx, ids[i]);
        * = JS_SetPropertyUint32(ctx, arr, @intCast(i), el_val);
    }
    return arr;
}

@c fn js_document_get_head(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_document_class_id);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    let i: usize = 1;
    while (i < dom_ctx.doc.nodes.items.len) : (i += 1) {
        const node = &dom_ctx.doc.nodes.items[i];
        if (node.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(node.tag_name, "head")) {
            return createJsElement(ctx, dom_ctx, @intCast(i));
        }
    }
    return jsNull();
}

@c fn js_implementation_create_html_document(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    const parent_opaque = JS_GetContextOpaque(ctx);
    if (parent_opaque == null) return jsNull();
    const parent_ctx: *JsDomContext = @ptrCast(@alignCast(parent_opaque));
    const alloc = std.heap.c_allocator;
    let new_doc = alloc.create(Document) catch return jsNull();
    new_doc.* = Document.init(alloc) catch return jsNull();
    const html_id = new_doc.createElement("html") catch return jsNull();
    const head_id = new_doc.createElement("head") catch return jsNull();
    const body_id = new_doc.createElement("body") catch return jsNull();
    new_doc.appendChild(html_id, head_id);
    new_doc.appendChild(html_id, body_id);
    if (argc >= 1 && !jsIsUndefined(argv[0])) {
        const title_str = jsToSlice(ctx, argv[0]);
        defer std.heap.page_allocator.free(title_str);
        const title_dup = alloc.dupe(u8, title_str) catch return jsNull();
        const title_id = new_doc.createElement("title") catch return jsNull();
        const text_id = new_doc.createTextNode(title_dup) catch return jsNull();
        new_doc.appendChild(head_id, title_id);
        new_doc.appendChild(title_id, text_id);
    }
    let new_ctx = alloc.create(JsDomContext) catch return jsNull();
    new_ctx.* = JsDomContext {
        doc: new_doc,
        layout: null,
        element_class_id: parent_ctx.element_class_id,
        document_class_id: parent_ctx.document_class_id,
    };
    const doc_val = JS_NewObjectClass(ctx, parent_ctx.document_class_id);
    * = JS_SetOpaque(doc_val, @ptrCast(new_ctx));
    * = JS_SetPropertyStr(ctx, doc_val, "_nid", jsInt(0));
    return doc_val;
}

@c fn js_document_get_implementation(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    * = argc;
    * = argv;
    const impl_obj = JS_NewObject(ctx);
    defineMethod(ctx, impl_obj, "createHTMLDocument", js_implementation_create_html_document, 1);
    return impl_obj;
}

@c fn js_document_get_node_type(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = ctx;
    * = this_val;
    * = argc;
    * = argv;
    return jsInt(9); // Node.DOCUMENT_NODE
}

@c fn js_document_get_node_name(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    * = argc;
    * = argv;
    return jsNewString(ctx, "#document");
}

fn handleJsConsole(ctx: *JSContext, argc: i32, argv: [*]const JSValue!, level: []const u8): JSValue {
    let text_buf: [512]u8 = [0; 512];
    let offset: usize = 0;
    let i: usize = 0;
    const n: usize = @intCast(argc);
    while (i < n) : (i += 1) {
        const str = jsToSlice(ctx, argv[i]);
        defer std.heap.page_allocator.free(str);
        if (i > 0 && offset < text_buf.len) {
            text_buf[offset] = ' ';
            offset += 1;
        }
        const to_copy = @min(str.len, text_buf.len - offset);
        if (to_copy > 0) {
            @memcpy(text_buf[offset .. offset + to_copy], str[0..to_copy]);
            offset += to_copy;
        }
    }
    const full_text = text_buf[0..offset];
    
    // Format arguments
    const args_array = JS_NewArray(ctx);
    let j: usize = 0;
    while (j < n) : (j += 1) {
        * = JS_SetPropertyUint32(ctx, args_array, @intCast(j), JS_DupValue(ctx, argv[j]));
    }
    
    // Store array in temp for format helper
    * = JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "__capsurf_temp_args", args_array);
    const format_script = "(function(){" +
        "  const args = globalThis.__capsurf_temp_args;" +
        "  const formatted = [];" +
        "  for(let i=0; i<args.length; i++) {" +
        "    formatted.push(globalThis.__capsurf_format_remote_object(args[i], true));" +
        "  }" +
        "  return JSON.stringify(formatted);" +
        "})()";
    const format_res = JS_Eval(ctx, @ptrCast(format_script), format_script.len, "format.js", 0);
    let args_buf: [1024]u8 = [0; 1024];
    let args_len: usize = 0;
    let cstr_len: usize = 0;
    const cstr = JS_ToCStringLen2(ctx, &cstr_len, format_res, 0);
    if (cstr != null) {
        args_len = @min(cstr_len, args_buf.len);
        @memcpy(args_buf[0..args_len], cstr.?[0..args_len]);
        JS_FreeCString(ctx, cstr);
    }
    JS_FreeValue(ctx, format_res);

    const opaque_ptr = JS_GetContextOpaque(ctx);
    if (opaque_ptr != null) {
        const dom_ctx: *JsDomContext = @ptrCast(@alignCast(opaque_ptr));
        dom_ctx.addConsoleRecord(level, full_text, args_buf[0..args_len]);
    }
    return jsUndefined();
}

@c fn js_console_log(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    return handleJsConsole(ctx, argc, argv, "log");
}

@c fn js_console_info(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    return handleJsConsole(ctx, argc, argv, "info");
}

@c fn js_console_warn(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    return handleJsConsole(ctx, argc, argv, "warn");
}

@c fn js_console_error(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    return handleJsConsole(ctx, argc, argv, "error");
}

@c fn js_console_debug(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    return handleJsConsole(ctx, argc, argv, "debug");
}

@c fn js_capsurf_find_node_by_id(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 1) return jsNull();
    let nid: i32 = 0;
    if (JS_ToInt32(ctx, &nid, argv[0]) != 0) return jsNull();
    if (nid <= 0) return jsNull();
    const ptr = JS_GetContextOpaque(ctx);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    const node_id = @as(NodeId, @intCast(nid));
    if (@as(usize, node_id) >= dom_ctx.doc.nodes.items.len) return jsNull();
    return createJsElement(ctx, dom_ctx, node_id);
}

@c fn js_capsurf_load_fixture(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 1) return jsNull();
    let len: usize = 0;
    const ptr = JS_ToCStringLen2(ctx, &len, argv[0], 0);
    if (ptr != null) {
        defer JS_FreeCString(ctx, ptr);
        let url_buf: [4096]u8 = [0; 4096];
        const actual_len = @min(len, url_buf.len);
        @memcpy(url_buf[0..actual_len], ptr.?[0..actual_len]);
        const url_slice = url_buf[0..actual_len];

        let cookie_jar_ptr: (*CookieJar)? = null;
        if (opaq := JS_GetContextOpaque(ctx)) {
            const dom_ctx: *JsDomContext = @ptrCast(@alignCast(opaq));
            cookie_jar_ptr = dom_ctx.cookie_jar;
        }

        let out_buf: [65536]u8 = [0; 65536];
        if (res := MockFixtures.handleFixtureRequest(url_slice, "", "", &out_buf, cookie_jar_ptr)) {
            return jsNewString(ctx, res);
        }
    }
    return jsNull();
}

@c fn js_capsurf_get_cookie_header(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 1) return jsNull();
    const ptr = JS_GetContextOpaque(ctx);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    if (dom_ctx.cookie_jar == null) return jsNull();

    let len: usize = 0;
    const str_ptr = JS_ToCStringLen2(ctx, &len, argv[0], 0);
    if (str_ptr != null) {
        defer JS_FreeCString(ctx, str_ptr);
        let url_buf: [4096]u8 = [0; 4096];
        const actual_len = @min(len, url_buf.len);
        @memcpy(url_buf[0..actual_len], str_ptr.?[0..actual_len]);
        const url_slice = url_buf[0..actual_len];

        const jar = dom_ctx.cookie_jar.?;
        const maybe_hdr = jar.cookieHeaderForUrl(url_slice, null, std.heap.page_allocator);
        if (hdr := maybe_hdr) {
            defer std.heap.page_allocator.free(hdr);
            return jsNewString(ctx, hdr);
        }
    }
    return jsNull();
}

@c fn js_capsurf_set_cookie(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 2) return jsNull();
    const ptr = JS_GetContextOpaque(ctx);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    if (dom_ctx.cookie_jar == null) return jsNull();

    let url_len: usize = 0;
    const url_ptr = JS_ToCStringLen2(ctx, &url_len, argv[0], 0);
    if (url_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, url_ptr);

    let val_len: usize = 0;
    const val_ptr = JS_ToCStringLen2(ctx, &val_len, argv[1], 0);
    if (val_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, val_ptr);

    let url_buf: [4096]u8 = [0; 4096];
    const actual_url_len = @min(url_len, url_buf.len);
    @memcpy(url_buf[0..actual_url_len], url_ptr.?[0..actual_url_len]);
    const url_slice = url_buf[0..actual_url_len];

    let val_buf: [4096]u8 = [0; 4096];
    const actual_val_len = @min(val_len, val_buf.len);
    @memcpy(val_buf[0..actual_val_len], val_ptr.?[0..actual_val_len]);
    const val_slice = val_buf[0..actual_val_len];

    const jar = dom_ctx.cookie_jar.?;
    * = jar.setFromDocumentCookie(url_slice, val_slice);
    return jsNull();
}

@c fn js_capsurf_record_fetch(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 1) return jsNull();
    const ptr = JS_GetContextOpaque(ctx);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));

    let u_len: usize = 0;
    const u_ptr = JS_ToCStringLen2(ctx, &u_len, argv[0], 0);
    if (u_ptr != null) {
        defer JS_FreeCString(ctx, u_ptr);
        let m_str: []const u8 = "application/octet-stream";
        if (argc >= 2) {
            let m_len: usize = 0;
            const m_ptr = JS_ToCStringLen2(ctx, &m_len, argv[1], 0);
            if (m_ptr != null) {
                defer JS_FreeCString(ctx, m_ptr);
                dom_ctx.addFetchRecord(u_ptr.?[0..u_len], m_ptr.?[0..m_len]);
                return jsNull();
            }
        }
        dom_ctx.addFetchRecord(u_ptr.?[0..u_len], m_str);
    }
    return jsNull();
}

@c fn js_capsurf_get_extra_headers(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    * = argc;
    * = argv;
    const ptr = JS_GetContextOpaque(ctx);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
    if (dom_ctx.extra_headers.len == 0 || std.mem.eql(u8, dom_ctx.extra_headers, "{}")) {
        return jsNull();
    }
    return jsNewString(ctx, dom_ctx.extra_headers);
}

pub fn mountDom(
    ctx: *JsContext,
    doc: *Document,
    layout: *LayoutTree?,
    cookie_jar: (*CookieJar)? = null,
    extra_headers: []const u8 = "",
): (*JsDomContext)! {
    const raw_ctx = ctx.raw_ctx;
    const raw_rt = ctx.runtime.raw_rt;

    if (g_element_class_id == 0) {
        * = JS_NewClassID(raw_rt, &g_element_class_id);
    }
    const el_class_def = JSClassDef {
        class_name: "Element",
        finalizer: js_element_finalizer,
        gc_mark: null,
        call_fn: null,
        exotic: null,
    };
    * = JS_NewClass(raw_rt, g_element_class_id, &el_class_def);

    if (g_document_class_id == 0) {
        * = JS_NewClassID(raw_rt, &g_document_class_id);
    }
    const doc_class_def = JSClassDef {
        class_name: "Document",
        finalizer: null,
        gc_mark: null,
        call_fn: null,
        exotic: null,
    };
    * = JS_NewClass(raw_rt, g_document_class_id, &doc_class_def);

    // Setup Element Prototype
    const el_proto = JS_NewObject(raw_ctx);
    defineMethod(raw_ctx, el_proto, "getAttribute", js_element_get_attribute, 1);
    defineMethod(raw_ctx, el_proto, "setAttribute", js_element_set_attribute, 2);
    defineMethod(raw_ctx, el_proto, "removeAttribute", js_element_remove_attribute, 1);
    defineMethod(raw_ctx, el_proto, "hasAttribute", js_element_has_attribute, 1);
    defineMethod(raw_ctx, el_proto, "getAttributeNames", js_element_get_attribute_names, 0);
    defineMethod(raw_ctx, el_proto, "getAttributeNS", js_element_get_attribute_ns, 2);
    defineMethod(raw_ctx, el_proto, "setAttributeNS", js_element_set_attribute_ns, 3);
    defineMethod(raw_ctx, el_proto, "removeAttributeNS", js_element_remove_attribute_ns, 2);
    defineMethod(raw_ctx, el_proto, "cloneNode", js_element_clone_node, 1);
    defineMethod(raw_ctx, el_proto, "_getRawAttrs", js_element_get_raw_attrs, 0);
    defineMethod(raw_ctx, el_proto, "appendChild", js_element_append_child, 1);
    defineMethod(raw_ctx, el_proto, "insertBefore", js_element_insert_before, 2);
    defineMethod(raw_ctx, el_proto, "removeChild", js_element_remove_child, 1);
    defineMethod(raw_ctx, el_proto, "replaceChild", js_element_replace_child, 2);
    defineMethod(raw_ctx, el_proto, "getBoundingClientRect", js_element_get_bounding_client_rect, 0);

    defineGetterSetter(raw_ctx, el_proto, "id", js_element_get_id, js_element_set_id);
    defineGetterSetter(raw_ctx, el_proto, "className", js_element_get_class_name, js_element_set_class_name);
    defineGetterSetter(raw_ctx, el_proto, "tagName", js_element_get_tag_name, null);
    defineGetterSetter(raw_ctx, el_proto, "nodeType", js_element_get_node_type, null);
    defineGetterSetter(raw_ctx, el_proto, "textContent", js_element_get_text_content, js_element_set_text_content);
    defineGetterSetter(raw_ctx, el_proto, "innerHTML", js_element_get_inner_html, js_element_set_inner_html);
    defineGetterSetter(raw_ctx, el_proto, "outerHTML", js_element_get_outer_html, null);
    defineGetterSetter(raw_ctx, el_proto, "parentElement", js_element_get_parent_element, null);
    defineGetterSetter(raw_ctx, el_proto, "parentNode", js_element_get_parent_node, js_element_set_parent_node);
    defineGetterSetter(raw_ctx, el_proto, "firstChild", js_element_get_first_child, null);
    defineGetterSetter(raw_ctx, el_proto, "lastChild", js_element_get_last_child, null);
    defineGetterSetter(raw_ctx, el_proto, "previousSibling", js_element_get_previous_sibling, null);
    defineGetterSetter(raw_ctx, el_proto, "nextSibling", js_element_get_next_sibling, null);

    JS_SetClassProto(raw_ctx, g_element_class_id, el_proto);

    // Setup Document Prototype
    const doc_proto = JS_NewObject(raw_ctx);
    defineMethod(raw_ctx, doc_proto, "querySelector", js_document_query_selector, 1);
    defineMethod(raw_ctx, doc_proto, "querySelectorAll", js_document_query_selector_all, 1);
    defineMethod(raw_ctx, doc_proto, "getElementById", js_document_get_element_by_id, 1);
    defineMethod(raw_ctx, doc_proto, "createElement", js_document_create_element, 1);
    defineMethod(raw_ctx, doc_proto, "getElementsByName", js_document_get_elements_by_name, 1);
    defineMethod(raw_ctx, doc_proto, "elementFromPoint", js_document_element_from_point, 2);
    defineGetterSetter(raw_ctx, doc_proto, "head", js_document_get_head, null);
    defineGetterSetter(raw_ctx, doc_proto, "implementation", js_document_get_implementation, null);
    defineGetterSetter(raw_ctx, doc_proto, "body", js_document_get_body, null);
    defineGetterSetter(raw_ctx, doc_proto, "title", js_document_get_title, js_document_set_title);
    defineGetterSetter(raw_ctx, doc_proto, "nodeType", js_document_get_node_type, null);
    defineGetterSetter(raw_ctx, doc_proto, "nodeName", js_document_get_node_name, null);
    defineGetterSetter(raw_ctx, doc_proto, "documentElement", js_document_get_document_element, null);
    defineGetterSetter(raw_ctx, doc_proto, "firstChild", js_element_get_first_child, null);
    defineGetterSetter(raw_ctx, doc_proto, "lastChild", js_element_get_last_child, null);

    JS_SetClassProto(raw_ctx, g_document_class_id, doc_proto);

    // Allocate JsDomContext
    let dom_ctx = std.heap.c_allocator.create(JsDomContext) catch return error.DomContextAllocFailed;
    dom_ctx.* = JsDomContext {
        doc: doc,
        layout: layout,
        cookie_jar: cookie_jar,
        extra_headers: extra_headers,
        element_class_id: g_element_class_id,
        document_class_id: g_document_class_id,
    };
    JS_SetContextOpaque(raw_ctx, @ptrCast(dom_ctx));

    // Create document instance
    const doc_val = JS_NewObjectClass(raw_ctx, g_document_class_id);
    * = JS_SetOpaque(doc_val, @ptrCast(dom_ctx));
    * = JS_SetPropertyStr(raw_ctx, doc_val, "_nid", jsInt(0));

    // Register globals
    const global_obj = ctx.getGlobal();
    defer ctx.freeValue(global_obj);

    * = JS_SetPropertyStr(raw_ctx, global_obj, "_nid", jsInt(-1));

    // Expose Element and Document constructors
    const el_ctor = JS_NewObject(raw_ctx);
    * = JS_SetPropertyStr(raw_ctx, el_ctor, "prototype", JS_DupValue(raw_ctx, el_proto));
    * = JS_SetPropertyStr(raw_ctx, el_proto, "constructor", JS_DupValue(raw_ctx, el_ctor));
    * = JS_SetPropertyStr(raw_ctx, global_obj, "Element", el_ctor);

    const doc_ctor = JS_NewObject(raw_ctx);
    * = JS_SetPropertyStr(raw_ctx, doc_ctor, "prototype", JS_DupValue(raw_ctx, doc_proto));
    * = JS_SetPropertyStr(raw_ctx, doc_proto, "constructor", JS_DupValue(raw_ctx, doc_ctor));
    * = JS_SetPropertyStr(raw_ctx, global_obj, "Document", doc_ctor);

    const win_proto = JS_NewObject(raw_ctx);
    const win_ctor = JS_NewObject(raw_ctx);
    * = JS_SetPropertyStr(raw_ctx, win_ctor, "prototype", JS_DupValue(raw_ctx, win_proto));
    * = JS_SetPropertyStr(raw_ctx, win_proto, "constructor", JS_DupValue(raw_ctx, win_ctor));
    * = JS_SetPropertyStr(raw_ctx, global_obj, "Window", win_ctor);

    * = JS_DefinePropertyValueStr(raw_ctx, global_obj, "document", doc_val, 1 | 4);
    * = JS_SetPropertyStr(raw_ctx, global_obj, "window", JS_DupValue(raw_ctx, global_obj));
    * = JS_SetPropertyStr(raw_ctx, global_obj, "globalThis", JS_DupValue(raw_ctx, global_obj));
    * = JS_SetPropertyStr(raw_ctx, global_obj, "self", JS_DupValue(raw_ctx, global_obj));

    // Register console
    const console_obj = JS_NewObject(raw_ctx);
    defineMethod(raw_ctx, console_obj, "log", js_console_log, 1);
    defineMethod(raw_ctx, console_obj, "info", js_console_info, 1);
    defineMethod(raw_ctx, console_obj, "warn", js_console_warn, 1);
    defineMethod(raw_ctx, console_obj, "error", js_console_error, 1);
    defineMethod(raw_ctx, console_obj, "debug", js_console_debug, 1);
    * = JS_SetPropertyStr(raw_ctx, global_obj, "console", console_obj);
    defineMethod(raw_ctx, global_obj, "__capsurf_native_find_node_by_id", js_capsurf_find_node_by_id, 1);
    defineMethod(raw_ctx, global_obj, "__capsurf_native_load_fixture", js_capsurf_load_fixture, 1);
    defineMethod(raw_ctx, global_obj, "__capsurf_get_cookie_header", js_capsurf_get_cookie_header, 1);
    defineMethod(raw_ctx, global_obj, "__capsurf_get_extra_headers", js_capsurf_get_extra_headers, 0);
    defineMethod(raw_ctx, global_obj, "__capsurf_set_cookie", js_capsurf_set_cookie, 2);
    defineMethod(raw_ctx, global_obj, "__capsurf_native_record_fetch", js_capsurf_record_fetch, 2);

    // Assemble and evaluate modular runtime shim
    let total_len: usize = 0;
    let chunk_idx: usize = 0;
    while (chunk_idx < RUNTIME_CHUNKS.len) : (chunk_idx += 1) {
        total_len += RUNTIME_CHUNKS[chunk_idx].len;
    }

    const runtime_buf = std.heap.page_allocator.alloc(u8, total_len) catch return error.RuntimeSourceAllocFailed;
    defer std.heap.page_allocator.free(runtime_buf);

    let offset: usize = 0;
    let copy_idx: usize = 0;
    while (copy_idx < RUNTIME_CHUNKS.len) : (copy_idx += 1) {
        const chunk = RUNTIME_CHUNKS[copy_idx];
        @memcpy(runtime_buf[offset .. offset + chunk.len], chunk);
        offset += chunk.len;
    }

    const saved_limit = ctx.runtime.state.reduction_limit;
    ctx.runtime.state.reduction_limit = 0;
    ctx.runtime.resetReductions();

    const shim_val = ctx.eval(runtime_buf);

    ctx.runtime.state.reduction_limit = saved_limit;
    ctx.runtime.resetReductions();

    if (jsIsException(shim_val)) {
        const exc = ctx.getException();
        const exc_str = ctx.toCString(exc);
        defer std.heap.page_allocator.free(exc_str);
        defer ctx.freeValue(exc);
        std.debug.print("Runtime shim eval error: {s}\n", .{exc_str});
    }
    ctx.freeValue(shim_val);

    const helper_script = 
        "(function() {" +
        "  if (!globalThis.__capsurf_remote_objects) {" +
        "    globalThis.__capsurf_remote_objects = new Map();" +
        "    globalThis.__capsurf_next_object_id = 1;" +
        "  }" +
        "  globalThis.__capsurf_store_object = function (val) {" +
        "    if (val === null || val === undefined) return null;" +
        "    const t = typeof val;" +
        "    if (t !== 'object' && t !== 'function') return null;" +
        "    const id = 'capsurf:obj:' + (globalThis.__capsurf_next_object_id++);" +
        "    globalThis.__capsurf_remote_objects.set(id, val);" +
        "    return id;" +
        "  };" +
        "  globalThis.__capsurf_get_object = function (id) {" +
        "    if (id === 'global' || id === 'globalThis') return globalThis;" +
        "    return globalThis.__capsurf_remote_objects.get(id);" +
        "  };" +
        "  globalThis.__capsurf_get_class_name = function (val) {" +
        "    if (val === null || val === undefined) return undefined;" +
        "    const t = typeof val;" +
        "    if (t !== 'object' && t !== 'function') return undefined;" +
        "    let name = undefined;" +
        "    try { name = val.constructor && val.constructor.name; } catch (e) {}" +
        "    if (typeof name === 'string' && name.length > 0) return name;" +
        "    try {" +
        "      const s = Object.prototype.toString.call(val);" +
        "      if (s.startsWith('[object ') && s.endsWith(']')) return s.slice(8, -1);" +
        "    } catch (e) {}" +
        "    if (Array.isArray(val)) return 'Array';" +
        "    if (t === 'function') return 'Function';" +
        "    return 'Object';" +
        "  };" +
        "  globalThis.__capsurf_format_remote_object = function (val, returnByValue) {" +
        "    if (val === undefined) return { type: 'undefined' };" +
        "    if (val === null) return { type: 'object', subtype: 'null', value: null };" +
        "    const t = typeof val;" +
        "    if (t === 'boolean' || t === 'number' || t === 'string') return { type: t, value: val };" +
        "    if (t === 'symbol') return { type: 'symbol', description: String(val) };" +
        "    if (t === 'bigint') return { type: 'bigint', unserializableValue: String(val) + 'n' };" +
        "    if (t === 'function') {" +
        "      const className = 'Function';" +
        "      const desc = String(val);" +
        "      const obj = { type: 'function', className: className, description: desc };" +
        "      if (!returnByValue) obj.objectId = globalThis.__capsurf_store_object(val);" +
        "      return obj;" +
        "    }" +
        "    if (t === 'object') {" +
        "      const isArr = Array.isArray(val);" +
        "      const className = globalThis.__capsurf_get_class_name(val);" +
        "      const obj = { type: 'object', className: className };" +
        "      if (isArr) obj.subtype = 'array';" +
        "      if (returnByValue) {" +
        "        obj.value = val;" +
        "      } else {" +
        "        obj.objectId = globalThis.__capsurf_store_object(val);" +
        "        obj.description = className;" +
        "      }" +
        "      return obj;" +
        "    }" +
        "    return { type: 'undefined' };" +
        "  };" +
        "  globalThis.__capsurf_call_fn = function (id, fnDecl) {" +
        "    const target = globalThis.__capsurf_get_object(id);" +
        "    if (!target && id !== 'global' && id !== 'globalThis') {" +
        "      throw new Error('Could not find object with id ' + id);" +
        "    }" +
        "    const fn = (0, eval)('(' + fnDecl + ')');" +
        "    if (typeof fn !== 'function') throw new TypeError('functionDeclaration did not evaluate to a function');" +
        "    return fn.call(target || globalThis);" +
        "  };" +
        "  if (typeof Element !== 'undefined' && Element.prototype && typeof Element.prototype.scrollIntoView === 'function') {" +
        "    const _origSiv = Element.prototype.scrollIntoView;" +
        "    Element.prototype.scrollIntoView = function (arg) {" +
        "      globalThis.__capsurf_ignore_sticky = true;" +
        "      try { return _origSiv.call(this, arg); }" +
        "      finally { globalThis.__capsurf_ignore_sticky = false; }" +
        "    };" +
        "  }" +
        "})();";
    const helper_val = ctx.eval(helper_script);
    ctx.freeValue(helper_val);

    const accent_script =
        "(function() { if (typeof globalThis.getComputedStyle !== 'function') return; const _origGcs = globalThis.getComputedStyle;" +
        " const _namedColors = { crimson:'rgb(220, 20, 60)', navy:'rgb(0, 0, 128)', red:'rgb(255, 0, 0)', green:'rgb(0, 128, 0)', blue:'rgb(0, 0, 255)', black:'rgb(0, 0, 0)', white:'rgb(255, 255, 255)', gray:'rgb(128, 128, 128)', grey:'rgb(128, 128, 128)', orange:'rgb(255, 165, 0)', purple:'rgb(128, 0, 128)', pink:'rgb(255, 192, 203)', yellow:'rgb(255, 255, 0)', cyan:'rgb(0, 255, 255)', aqua:'rgb(0, 255, 255)', magenta:'rgb(255, 0, 255)', fuchsia:'rgb(255, 0, 255)', teal:'rgb(0, 128, 128)', indigo:'rgb(75, 0, 130)', violet:'rgb(238, 130, 238)', tomato:'rgb(255, 99, 71)', coral:'rgb(255, 127, 80)', gold:'rgb(255, 215, 0)', lime:'rgb(0, 255, 0)', olive:'rgb(128, 128, 0)', maroon:'rgb(128, 0, 0)', silver:'rgb(192, 192, 192)', rebeccapurple:'rgb(102, 51, 153)', dodgerblue:'rgb(30, 144, 255)', steelblue:'rgb(70, 130, 180)' };" +
        " function _normColor(raw) { if (!raw) return ''; const s = String(raw).trim().toLowerCase(); if (!s || s === 'auto') return 'auto'; if (_namedColors[s]) return _namedColors[s]; if (s.startsWith('#')) { let h = s.slice(1); if (h.length === 3) h = h[0]+h[0]+h[1]+h[1]+h[2]+h[2]; if (h.length === 6) { const r = parseInt(h.slice(0,2), 16), g = parseInt(h.slice(2,4), 16), b = parseInt(h.slice(4,6), 16); if (!isNaN(r) && !isNaN(g) && !isNaN(b)) return 'rgb(' + r + ', ' + g + ', ' + b + ')'; } } return raw.trim(); }" +
        " function _resolveAccent(el, cs) { const direct = cs.getPropertyValue('accent-color'); if (direct && direct.trim()) return _normColor(direct); let p = el && el.parentElement; while (p) { try { const pcs = _origGcs(p); const pv = pcs.getPropertyValue('accent-color'); if (pv && pv.trim()) return _normColor(pv); } catch (e) {} p = p.parentElement; } if (el && el.tagName && el.tagName.toLowerCase() === 'input' && String(el.getAttribute && el.getAttribute('type') || '').toLowerCase() === 'range') { try { const styles = document.querySelectorAll('style'); let found = ''; for (let i = 0; i < styles.length; i++) { const txt = styles[i].textContent || ''; const re = /([^{}]+)\\{([^{}]*)\\}/g; let m; while ((m = re.exec(txt)) !== null) { const sels = m[1].split(','); let matched = false; for (let j = 0; j < sels.length; j++) { const s = sels[j].trim(); const idx = s.indexOf('::-webkit-slider-thumb') !== -1 ? s.indexOf('::-webkit-slider-thumb') : s.indexOf('::-moz-range-thumb'); if (idx !== -1) { const baseSel = s.slice(0, idx).trim() || '*'; if (typeof el.matches === 'function' && el.matches(baseSel)) { matched = true; break; } } } if (matched) { const decls = m[2].split(';'); for (let k = 0; k < decls.length; k++) { const col = decls[k].indexOf(':'); if (col === -1) continue; const pn = decls[k].slice(0, col).trim().toLowerCase(); const pv = decls[k].slice(col + 1).replace(/!important/i, '').trim(); if (pn === 'background-color' || pn === 'background') { const nc = _normColor(pv); if (nc && nc !== 'auto') found = nc; } } } } } if (found) return found; } catch (e) {} } return 'auto'; }" +
        " const _wrappedGcs = function (el, pseudo) { const cs = _origGcs(el, pseudo); return new Proxy(cs, { get: function (target, prop) { if (prop === 'getPropertyValue') { return function (name) { if (String(name).trim().toLowerCase() === 'accent-color') return _resolveAccent(el, target); return target.getPropertyValue(name); }; } if (prop === 'accentColor' || prop === 'accent-color') return _resolveAccent(el, target); const v = target[prop]; return typeof v === 'function' ? v.bind(target) : v; } }); };" +
        " globalThis.getComputedStyle = _wrappedGcs; if (typeof window !== 'undefined') window.getComputedStyle = _wrappedGcs; })();";
    const accent_val = ctx.eval(accent_script);
    ctx.freeValue(accent_val);

    return dom_ctx;
}
