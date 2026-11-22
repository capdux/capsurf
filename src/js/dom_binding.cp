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
    JS_IsFunction,
    JS_Call,
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
import { StyleSheet, Cascade, ComputedStyle, CssPosition } from "../css/cascade.cp";
import { LayoutTree, Rect } from "../layout/layout.cp";
import { RUNTIME_CHUNKS } from "./runtime_source.cp";
import { MockFixtures } from "../net/mock_fixtures.cp";
import { CookieJar } from "../net/cookies.cp";
import { getHostMilliTimestamp } from "../eval/host_clock.cp";

pub fn getMilliTimestamp(): i64 {
    return getHostMilliTimestamp();
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

    fn getName(*_): []const u8 {
        return _.name_buf[0.._.name_len];
    }

    fn getValue(*_): []const u8 {
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
    const key = std.fmt.bufPrint(&key_buf, "w{d}:{d}", .{ unsafe { @intFromPtr(dom_ctx) }, node_id }) catch "";
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
        const hit = JS_GetPropertyStr(ctx, cache, unsafe { @ptrCast(&key_z) });
        if (jsIsObject(hit)) {
            return hit;
        }
        JS_FreeValue(ctx, hit);
        const weak_cache = JS_GetPropertyStr(ctx, global_obj, "__capsurf_weak_wrappers");
        if (jsIsObject(weak_cache)) {
            const wref = JS_GetPropertyStr(ctx, weak_cache, unsafe { @ptrCast(&key_z) });
            if (jsIsObject(wref)) {
                const deref_fn = JS_GetPropertyStr(ctx, wref, "deref");
                if (JS_IsFunction(ctx, deref_fn)) {
                    let dummy_args: [1]JSValue = [jsUndefined()];
                    const target = JS_Call(ctx, deref_fn, wref, 0, unsafe { @ptrCast(&dummy_args) });
                    JS_FreeValue(ctx, deref_fn);
                    if (jsIsObject(target)) {
                        JS_FreeValue(ctx, wref);
                        JS_FreeValue(ctx, weak_cache);
                        return target;
                    }
                    JS_FreeValue(ctx, target);
                } else {
                    JS_FreeValue(ctx, deref_fn);
                }
            }
            JS_FreeValue(ctx, wref);
        }
        JS_FreeValue(ctx, weak_cache);
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
    * = JS_SetOpaque(el_val, unsafe { @ptrCast(el) });
    * = JS_SetPropertyStr(ctx, el_val, "_nid", jsInt(@intCast(node_id)));
    * = JS_SetPropertyStr(ctx, el_val, "_id", jsInt(@intCast(node_id)));
    if (key.len > 0) {
        * = JS_SetPropertyStr(ctx, cache, unsafe { @ptrCast(&key_z) }, JS_DupValue(ctx, el_val));
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
        const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
        std.heap.c_allocator.destroy(el);
    }
}

@c fn js_element_get_attribute(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsNull();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const attr_name = jsToSlice(ctx, unsafe { argv[0] });
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const attr_name = jsToSlice(ctx, unsafe { argv[0] });
    defer std.heap.page_allocator.free(attr_name);
    const attr_val = jsToSlice(ctx, unsafe { argv[1] });
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const attr_name = jsToSlice(ctx, unsafe { argv[0] });
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const attr_name = jsToSlice(ctx, unsafe { argv[0] });
    defer std.heap.page_allocator.free(attr_name);
    if (@as(usize, el.node_id) >= el.dom_ctx.doc.nodes.items.len) return jsFalse();
    const node = &el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)];
    return jsBool(node.getAttribute(attr_name) != null);
}

@c fn js_element_append_child(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const child_ptr = JS_GetOpaque(unsafe { argv[0] }, g_element_class_id);
    if (child_ptr == null) return jsUndefined();
    const child_el: *JsElement = unsafe { @ptrCast(@alignCast(child_ptr.?)) };
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
    return JS_DupValue(ctx, unsafe { argv[0] });
}

@c fn js_element_insert_before(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const child_ptr = JS_GetOpaque(unsafe { argv[0] }, g_element_class_id);
    if (child_ptr == null) return jsUndefined();
    const child_el: *JsElement = unsafe { @ptrCast(@alignCast(child_ptr.?)) };

    let ref_node_id: NodeId = NULL_NODE;
    if (argc >= 2) {
        const ref_ptr = JS_GetOpaque(unsafe { argv[1] }, g_element_class_id);
        if (ref_ptr != null) {
            const ref_el: *JsElement = unsafe { @ptrCast(@alignCast(ref_ptr.?)) };
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
    return JS_DupValue(ctx, unsafe { argv[0] });
}

@c fn js_element_remove_child(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const child_ptr = JS_GetOpaque(unsafe { argv[0] }, g_element_class_id);
    if (child_ptr == null) return jsUndefined();
    const child_el: *JsElement = unsafe { @ptrCast(@alignCast(child_ptr.?)) };
    let parent_id: NodeId = el.node_id;
    if (@as(usize, child_el.node_id) < el.dom_ctx.doc.nodes.items.len) {
        const p = el.dom_ctx.doc.nodes.items[@as(usize, child_el.node_id)].parent;
        if (p != NULL_NODE) parent_id = p;
    }
    el.dom_ctx.doc.removeChild(child_el.node_id);
    el.dom_ctx.recordChildRemoved(parent_id, child_el.node_id);
    return JS_DupValue(ctx, unsafe { argv[0] });
}

@c fn js_element_replace_child(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 2) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const new_ptr = JS_GetOpaque(unsafe { argv[0] }, g_element_class_id);
    if (new_ptr == null) return jsUndefined();
    const new_el: *JsElement = unsafe { @ptrCast(@alignCast(new_ptr.?)) };
    const old_ptr = JS_GetOpaque(unsafe { argv[1] }, g_element_class_id);
    if (old_ptr == null) return jsUndefined();
    const old_el: *JsElement = unsafe { @ptrCast(@alignCast(old_ptr.?)) };
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
    return JS_DupValue(ctx, unsafe { argv[1] });
}

struct ViewportRectInfo {
    rect: Rect = Rect.zero(),
    win_x: f32 = 0.0,
    win_y: f32 = 0.0,
    anc_scroll_x: f32 = 0.0,
    anc_scroll_y: f32 = 0.0,
}

fn docHasContainerQuery(doc: *const Document): bool {
    let snid: usize = 0;
    while (snid < doc.nodes.items.len) : (snid += 1) {
        const sn = &doc.nodes.items[snid];
        if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style")) {
            if (std.mem.indexOf(u8, sn.text_content, "@container") != null) return true;
            let child = sn.first_child;
            while (child != NULL_NODE && @as(usize, child) < doc.nodes.items.len) : (child = doc.nodes.items[@as(usize, child)].next_sibling) {
                const scn = &doc.nodes.items[@as(usize, child)];
                if (scn.node_type == NodeType.Text && std.mem.indexOf(u8, scn.text_content, "@container") != null) return true;
            }
        }
    }
    return false;
}

fn linkMediaMatchesScreen(media_raw: []const u8): bool {
    const trimmed = std.mem.trim(u8, media_raw, " \t\r\n");
    if (trimmed.len == 0) return true;
    let it = std.mem.splitScalar(u8, trimmed, ',');
    while (part := it.next()) {
        const q = std.mem.trim(u8, part, " \t\r\n");
        if (q.len == 0) continue;
        let low_buf: [256]u8 = undefined;
        const n = @min(q.len, low_buf.len);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            low_buf[i] = std.ascii.toLower(q[i]);
        }
        const lq = low_buf[0..n];
        if (std.mem.startsWith(u8, lq, "not screen") || std.mem.startsWith(u8, lq, "not all")) {
            continue;
        }
        const is_print = std.mem.indexOf(u8, lq, "print") != null;
        const is_speech = std.mem.indexOf(u8, lq, "speech") != null;
        const is_screen = std.mem.indexOf(u8, lq, "screen") != null;
        const is_all = std.mem.indexOf(u8, lq, "all") != null;
        if ((is_print || is_speech) && !is_screen && !is_all) {
            continue;
        }
        return true;
    }
    return false;
}

fn collectDocCssInto(doc: *const Document, cookie_jar: (*CookieJar)?, css_buf: []u8): usize {
    let css_len: usize = 0;
    let snid: usize = 0;
    while (snid < doc.nodes.items.len) : (snid += 1) {
        const sn = &doc.nodes.items[snid];
        if (sn.node_type != NodeType.Element) continue;
        if (std.ascii.eqlIgnoreCase(sn.tag_name, "style")) {
            if (sn.text_content.len > 0) {
                const copy_len = @min(sn.text_content.len, css_buf.len - css_len);
                @memcpy(css_buf[css_len .. css_len + copy_len], sn.text_content[0..copy_len]);
                css_len += copy_len;
            }
            let child = sn.first_child;
            while (child != NULL_NODE && @as(usize, child) < doc.nodes.items.len) : (child = doc.nodes.items[@as(usize, child)].next_sibling) {
                const scn = &doc.nodes.items[@as(usize, child)];
                if (scn.node_type == NodeType.Text && scn.text_content.len > 0) {
                    const copy_len = @min(scn.text_content.len, css_buf.len - css_len);
                    @memcpy(css_buf[css_len .. css_len + copy_len], scn.text_content[0..copy_len]);
                    css_len += copy_len;
                }
            }
            if (css_len < css_buf.len) {
                css_buf[css_len] = '\n';
                css_len += 1;
            }
        } else if (std.ascii.eqlIgnoreCase(sn.tag_name, "link")) {
            if (rel := sn.getAttribute("rel")) {
                let is_sheet = false;
                let rit = std.mem.tokenizeAny(u8, rel, " \t\r\n");
                while (rtok := rit.next()) {
                    if (std.ascii.eqlIgnoreCase(rtok, "stylesheet")) {
                        is_sheet = true;
                        break;
                    }
                }
                if (is_sheet) {
                    const media_attr = sn.getAttribute("media") ?? "";
                    if (linkMediaMatchesScreen(media_attr)) {
                        if (href := sn.getAttribute("href")) {
                            let link_buf: [65536]u8 = undefined;
                            let res_opt = MockFixtures.handleFixtureRequest(href, "", "", &link_buf, cookie_jar);
                            if (res_opt == null) {
                                let bin_href_buf: [512]u8 = undefined;
                                const bin_href = std.fmt.bufPrint(&bin_href_buf, "{s}.bin", .{ href }) catch "";
                                if (bin_href.len > 0) {
                                    res_opt = MockFixtures.handleFixtureRequest(bin_href, "", "", &link_buf, cookie_jar);
                                }
                            }
                            if (res := res_opt) {
                                let is_utf8 = true;
                                let ui: usize = 0;
                                while (ui < res.len) {
                                    const b = res[ui];
                                    if (b < 0x80) {
                                        ui += 1;
                                    } else if (b >= 0xC2 && b <= 0xDF && ui + 1 < res.len && (res[ui + 1] & 0xC0) == 0x80) {
                                        ui += 2;
                                    } else if (b >= 0xE0 && b <= 0xEF && ui + 2 < res.len && (res[ui + 1] & 0xC0) == 0x80 && (res[ui + 2] & 0xC0) == 0x80) {
                                        ui += 3;
                                    } else if (b >= 0xF0 && b <= 0xF4 && ui + 3 < res.len && (res[ui + 1] & 0xC0) == 0x80 && (res[ui + 2] & 0xC0) == 0x80 && (res[ui + 3] & 0xC0) == 0x80) {
                                        ui += 4;
                                    } else {
                                        is_utf8 = false;
                                        break;
                                    }
                                }
                                if (is_utf8) {
                                    const copy_len = @min(res.len, css_buf.len - css_len);
                                    @memcpy(css_buf[css_len .. css_len + copy_len], res[0..copy_len]);
                                    css_len += copy_len;
                                } else {
                                    let li: usize = 0;
                                    while (li < res.len) : (li += 1) {
                                        const lb = res[li];
                                        if (lb < 0x80) {
                                            if (css_len >= css_buf.len) break;
                                            css_buf[css_len] = lb;
                                            css_len += 1;
                                        } else {
                                            const cp: u21 = switch (lb) {
                                                0xA4 => 0x20AC,
                                                0xA6 => 0x0160,
                                                0xA8 => 0x0161,
                                                0xB4 => 0x017D,
                                                0xB8 => 0x017E,
                                                0xBC => 0x0152,
                                                0xBD => 0x0153,
                                                0xBE => 0x0178,
                                                * => @as(u21, lb),
                                            };
                                            if (cp < 0x800) {
                                                if (css_len + 2 > css_buf.len) break;
                                                css_buf[css_len] = @as(u8, @intCast(0xC0 | (cp >> 6)));
                                                css_buf[css_len + 1] = @as(u8, @intCast(0x80 | (cp & 0x3F)));
                                                css_len += 2;
                                            } else {
                                                if (css_len + 3 > css_buf.len) break;
                                                css_buf[css_len] = @as(u8, @intCast(0xE0 | (cp >> 12)));
                                                css_buf[css_len + 1] = @as(u8, @intCast(0x80 | ((cp >> 6) & 0x3F)));
                                                css_buf[css_len + 2] = @as(u8, @intCast(0x80 | (cp & 0x3F)));
                                                css_len += 3;
                                            }
                                        }
                                    }
                                }
                                if (css_len < css_buf.len) {
                                    css_buf[css_len] = '\n';
                                    css_len += 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return css_len;
}

fn ensureLayoutUpToDate(dom_ctx: *JsDomContext) {
    if (!dom_ctx.layout_dirty && (dom_ctx.paint_prepares > 0 || !docHasContainerQuery(dom_ctx.doc))) return;
    dom_ctx.layout_dirty = false;
    if (lt := dom_ctx.layout) {
        const doc = dom_ctx.doc;
        const count = @max(@as(usize, 1), doc.nodeCount());
        let css_buf = std.heap.page_allocator.alloc(u8, 65536) catch null;
        defer if (css_buf != null) std.heap.page_allocator.free(css_buf.?);
        let css_len: usize = 0;
        if (css_buf != null) {
            css_len = collectDocCssInto(doc, dom_ctx.cookie_jar, css_buf.?);
        }
        const full_css = if (css_buf != null) css_buf.?[0..css_len] else "";
        const sheet = StyleSheet.parse(full_css, lt.viewport_width, false);
        let styles = std.heap.page_allocator.alloc(ComputedStyle, count) catch return;
        defer std.heap.page_allocator.free(styles);
        Cascade.computeStyles(doc, &sheet, styles, 16.0);
        lt.computeLayout(doc, styles) catch {};
        if (sheet.container_rule_count > 0) {
            let box_w = std.heap.page_allocator.alloc(f32, count) catch null;
            let box_h = std.heap.page_allocator.alloc(f32, count) catch null;
            defer if (box_w != null) std.heap.page_allocator.free(box_w.?);
            defer if (box_h != null) std.heap.page_allocator.free(box_h.?);
            if (box_w != null && box_h != null) {
                let cq_pass: usize = 0;
                while (cq_pass < 3) : (cq_pass += 1) {
                    let bi: usize = 0;
                    while (bi < count) : (bi += 1) {
                        const br = lt.getBoundingClientRect(@as(u32, @intCast(bi)));
                        box_w.?[bi] = br.width;
                        box_h.?[bi] = br.height;
                    }
                    Cascade.computeStylesWithContainers(doc, &sheet, styles, box_w.?, box_h.?, 16.0);
                    lt.computeLayout(doc, styles) catch {};
                }
            }
        }
        dom_ctx.paint_prepares += 1;
    }
}

@c fn js_get_computed_prop(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 2) return jsNewString(ctx, "");
    const opaque_ptr = JS_GetContextOpaque(ctx);
    if (opaque_ptr == null) return jsNewString(ctx, "");
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(opaque_ptr.?)) };

    let nid_i32: i32 = 0;
    if (JS_ToInt32(ctx, &nid_i32, unsafe { argv[0] }) != 0 || nid_i32 <= 0) return jsNewString(ctx, "");
    const prop_slice = jsToSlice(ctx, unsafe { argv[1] });
    defer if (prop_slice.len > 0) std.heap.page_allocator.free(prop_slice);
    if (prop_slice.len == 0) return jsNewString(ctx, "");

    const doc = dom_ctx.doc;
    const nid_u = @as(usize, @intCast(nid_i32));
    if (nid_u >= doc.nodes.items.len) return jsNewString(ctx, "");

    const count = @max(@as(usize, 1), doc.nodeCount());
    const css_buf = std.heap.page_allocator.alloc(u8, 65536) catch return jsNewString(ctx, "");
    defer std.heap.page_allocator.free(css_buf);
    const css_len = collectDocCssInto(doc, dom_ctx.cookie_jar, css_buf);
    const full_css = css_buf[0..css_len];
    let vw: f32 = 1280.0;
    let vh: f32 = 720.0;
    if (lt := dom_ctx.layout) {
        vw = lt.viewport_width;
        vh = lt.viewport_height;
    }
    const sheet = StyleSheet.parse(full_css, vw, false);
    const styles = std.heap.page_allocator.alloc(ComputedStyle, count) catch return jsNewString(ctx, "");
    defer std.heap.page_allocator.free(styles);
    Cascade.computeStyles(doc, &sheet, styles, 16.0);
    if (lt := dom_ctx.layout) {
        lt.computeLayout(doc, styles) catch {};
        if (sheet.container_rule_count > 0) {
            let box_w = std.heap.page_allocator.alloc(f32, count) catch null;
            let box_h = std.heap.page_allocator.alloc(f32, count) catch null;
            defer if (box_w != null) std.heap.page_allocator.free(box_w.?);
            defer if (box_h != null) std.heap.page_allocator.free(box_h.?);
            if (box_w != null && box_h != null) {
                let cq_pass: usize = 0;
                while (cq_pass < 3) : (cq_pass += 1) {
                    let bi: usize = 0;
                    while (bi < count) : (bi += 1) {
                        const br = lt.getBoundingClientRect(@as(u32, @intCast(bi)));
                        box_w.?[bi] = br.width;
                        box_h.?[bi] = br.height;
                    }
                    Cascade.computeStylesWithContainers(doc, &sheet, styles, box_w.?, box_h.?, 16.0);
                    lt.computeLayout(doc, styles) catch {};
                }
            }
        }
    }

    const st = &styles[nid_u];
    let out_buf: [128]u8 = undefined;
    if (std.ascii.eqlIgnoreCase(prop_slice, "container-type") || std.ascii.eqlIgnoreCase(prop_slice, "containerType")) {
        return jsNewString(ctx, st.containerTypeStr());
    }
    if (std.ascii.eqlIgnoreCase(prop_slice, "container-name") || std.ascii.eqlIgnoreCase(prop_slice, "containerName")) {
        const cn = st.container_name_buf[0..st.container_name_len];
        return jsNewString(ctx, if (cn.len > 0) cn else "none");
    }
    if (std.ascii.eqlIgnoreCase(prop_slice, "padding-left") || std.ascii.eqlIgnoreCase(prop_slice, "paddingLeft")) {
        const px = st.padding_left.resolvePx(vw, st.font_size_px, 16.0, vw, vh);
        const s = std.fmt.bufPrint(&out_buf, "{d}px", .{@as(i32, @intFromFloat(@round(px)))}) catch return jsNewString(ctx, "");
        return jsNewString(ctx, s);
    }
    if (std.ascii.eqlIgnoreCase(prop_slice, "padding-right") || std.ascii.eqlIgnoreCase(prop_slice, "paddingRight")) {
        const px = st.padding_right.resolvePx(vw, st.font_size_px, 16.0, vw, vh);
        const s = std.fmt.bufPrint(&out_buf, "{d}px", .{@as(i32, @intFromFloat(@round(px)))}) catch return jsNewString(ctx, "");
        return jsNewString(ctx, s);
    }
    if (std.ascii.eqlIgnoreCase(prop_slice, "padding-top") || std.ascii.eqlIgnoreCase(prop_slice, "paddingTop")) {
        const px = st.padding_top.resolvePx(vh, st.font_size_px, 16.0, vw, vh);
        const s = std.fmt.bufPrint(&out_buf, "{d}px", .{@as(i32, @intFromFloat(@round(px)))}) catch return jsNewString(ctx, "");
        return jsNewString(ctx, s);
    }
    if (std.ascii.eqlIgnoreCase(prop_slice, "padding-bottom") || std.ascii.eqlIgnoreCase(prop_slice, "paddingBottom")) {
        const px = st.padding_bottom.resolvePx(vh, st.font_size_px, 16.0, vw, vh);
        const s = std.fmt.bufPrint(&out_buf, "{d}px", .{@as(i32, @intFromFloat(@round(px)))}) catch return jsNewString(ctx, "");
        return jsNewString(ctx, s);
    }
    if (std.ascii.eqlIgnoreCase(prop_slice, "color")) {
        const c = st.color;
        const s = std.fmt.bufPrint(&out_buf, "rgb({d}, {d}, {d})", .{ c.r, c.g, c.b }) catch return jsNewString(ctx, "");
        return jsNewString(ctx, s);
    }
    if (std.ascii.eqlIgnoreCase(prop_slice, "background-color") || std.ascii.eqlIgnoreCase(prop_slice, "backgroundColor")) {
        const c = st.background_color;
        const s = if (c.a == 255)
            (std.fmt.bufPrint(&out_buf, "rgb({d}, {d}, {d})", .{ c.r, c.g, c.b }) catch return jsNewString(ctx, ""))
        else
            (std.fmt.bufPrint(&out_buf, "rgba({d}, {d}, {d}, {d})", .{ c.r, c.g, c.b, c.a }) catch return jsNewString(ctx, ""));
        return jsNewString(ctx, s);
    }
    return jsNewString(ctx, "");
}

@c fn js_get_layout_box(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 1) return jsNull();
    const opaque_ptr = JS_GetContextOpaque(ctx);
    if (opaque_ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(opaque_ptr.?)) };

    let nid_i32: i32 = 0;
    if (JS_ToInt32(ctx, &nid_i32, unsafe { argv[0] }) != 0 || nid_i32 <= 0) return jsNull();
    const nid_u = @as(usize, @intCast(nid_i32));
    if (nid_u >= dom_ctx.doc.nodes.items.len) return jsNull();

    ensureLayoutUpToDate(dom_ctx);
    if (lt := dom_ctx.layout) {
        if (nid_u < lt.boxes.items.len) {
            const box = &lt.boxes.items[nid_u];
            const obj = JS_NewObject(ctx);
            * = JS_SetPropertyStr(ctx, obj, "x", jsFloat(@floatCast(box.rect.x)));
            * = JS_SetPropertyStr(ctx, obj, "y", jsFloat(@floatCast(box.rect.y)));
            * = JS_SetPropertyStr(ctx, obj, "width", jsFloat(@floatCast(box.rect.width)));
            * = JS_SetPropertyStr(ctx, obj, "height", jsFloat(@floatCast(box.rect.height)));
            * = JS_SetPropertyStr(ctx, obj, "borderLeft", jsFloat(@floatCast(box.border_left)));
            * = JS_SetPropertyStr(ctx, obj, "borderTop", jsFloat(@floatCast(box.border_top)));
            * = JS_SetPropertyStr(ctx, obj, "visible", jsBool(box.rect.visible));
            return obj;
        }
    }
    return jsNull();
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
    const key = std.fmt.bufPrint(&key_buf, "w{d}:{d}", .{ unsafe { @intFromPtr(dom_ctx) }, node_id }) catch return;
    if (key.len == 0) return;
    let key_z: [65]u8 = undefined;
    @memcpy(key_z[0..key.len], key);
    key_z[key.len] = 0;
    const el_val = JS_GetPropertyStr(ctx, cache, unsafe { @ptrCast(&key_z) });
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

        if (box.position == CssPosition.Fixed) {
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

            if (box.position == CssPosition.Sticky && !ignore_sticky) {
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(opaque_ptr.?)) };

    let px_f64: f64 = 0.0;
    let py_f64: f64 = 0.0;
    if (JS_ToFloat64(ctx, &px_f64, unsafe { argv[0] }) != 0 || JS_ToFloat64(ctx, &py_f64, unsafe { argv[1] }) != 0) {
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
        let best_sc_root: NodeId = NULL_NODE;
        let best_sc_z: i32 = -2147483647;
        let best_sc_pos: i32 = -1;
        let best_local_z: i32 = -2147483647;
        let best_local_pos: i32 = -1;

        let nid: usize = 1;
        while (nid < doc.nodes.items.len && nid < lt.boxes.items.len) : (nid += 1) {
            const node = &doc.nodes.items[nid];
            if (node.node_type != NodeType.Element) continue;
            if (node.hasTag("head") || node.hasTag("style") || node.hasTag("script") || node.hasTag("title") || node.hasTag("meta") || node.hasTag("link")) continue;
            const node_id: NodeId = @intCast(nid);
            const box = &lt.boxes.items[nid];
            if (!box.rect.visible || box.rect.width <= 0.0 || box.rect.height <= 0.0) continue;
            if (box.pointer_events_none) continue;

            const info = computeViewportRect(ctx, dom_ctx, lt, node_id, false);
            const r = info.rect;
            if (px < r.x || px >= r.x + r.width || py < r.y || py >= r.y + r.height) continue;

            // Build ancestor chain (from root down to node_id) for overflow clipping & stacking context
            let chain: [64]NodeId = [NULL_NODE; 64];
            let depth: usize = 0;
            let cur_a = node_id;
            while (cur_a != NULL_NODE && @as(usize, cur_a) < doc.nodes.items.len && @as(usize, cur_a) < lt.boxes.items.len && depth < 64) {
                chain[depth] = cur_a;
                depth += 1;
                cur_a = doc.nodes.items[@as(usize, cur_a)].parent;
            }
            // Reverse chain so chain[0] is outermost ancestor, chain[depth - 1] is node_id
            let li: usize = 0;
            while (li < depth / 2) : (li += 1) {
                const tmp = chain[li];
                chain[li] = chain[depth - 1 - li];
                chain[depth - 1 - li] = tmp;
            }

            // Check ancestor overflow clipping (fixed escapes all ancestor clips)
            let clipped = false;
            if (box.position != CssPosition.Fixed) {
                let ci: usize = depth;
                while (ci > 1) {
                    ci -= 1;
                    const anc = chain[ci - 1];
                    const child_id = chain[ci];
                    if (lt.boxes.items[@as(usize, child_id)].position == CssPosition.Fixed) {
                        break;
                    }
                    const anc_node = &doc.nodes.items[@as(usize, anc)];
                    const anc_box = &lt.boxes.items[@as(usize, anc)];
                    if (anc_node.node_type == NodeType.Element && !anc_node.hasTag("html") && !anc_node.hasTag("body") && !anc_box.overflow_visible) {
                        const ar = computeViewportRect(ctx, dom_ctx, lt, anc, false).rect;
                        if (px < ar.x || px >= ar.x + ar.width || py < ar.y || py >= ar.y + ar.height) {
                            clipped = true;
                            break;
                        }
                    }
                }
            }
            if (clipped) continue;

            // Compute CSS stacking order:
            // Positioned elements (fixed/absolute/relative/sticky) share the same layer (1) when z-index is auto;
            // descendants inherit their positioned/stacking-context ancestor's layer.
            let sc_root: NodeId = NULL_NODE;
            let sc_z: i32 = 0;
            let sc_pos: i32 = 0;
            let local_z: i32 = 0;
            let local_pos: i32 = 0;
            let ci2: usize = 0;
            while (ci2 < depth) : (ci2 += 1) {
                const aid = chain[ci2];
                const anode = &doc.nodes.items[@as(usize, aid)];
                if (anode.node_type != NodeType.Element || anode.hasTag("html") || anode.hasTag("body")) continue;
                const abox = &lt.boxes.items[@as(usize, aid)];
                const is_pos = (abox.position != CssPosition.Static);
                if (sc_root == NULL_NODE) {
                    if (is_pos) sc_pos = 1;
                    if (abox.creates_stacking_context) {
                        sc_root = aid;
                        sc_z = if (abox.has_z_index) abox.z_index else 0;
                    }
                } else {
                    if (is_pos) local_pos = 1;
                    if (abox.has_z_index) local_z = abox.z_index;
                }
            }

            let wins = false;
            if (best_node == NULL_NODE) {
                wins = true;
            } else if (sc_z != best_sc_z) {
                wins = (sc_z > best_sc_z);
            } else if (sc_pos != best_sc_pos) {
                wins = (sc_pos > best_sc_pos);
            } else if (sc_root != NULL_NODE && sc_root == best_sc_root) {
                if (local_z != best_local_z) {
                    wins = (local_z > best_local_z);
                } else {
                    wins = (local_pos >= best_local_pos);
                }
            } else {
                // Same root stacking level, different or no sub-context -> later in DOM tree order wins
                wins = true;
            }

            if (wins) {
                best_node = node_id;
                best_sc_root = sc_root;
                best_sc_z = sc_z;
                best_sc_pos = sc_pos;
                best_local_z = local_z;
                best_local_pos = local_pos;
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    if (@as(usize, el.node_id) >= el.dom_ctx.doc.nodes.items.len) return jsNewString(ctx, "");
    const node = &el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)];
    return jsNewString(ctx, node.id_attr);
}

@c fn js_element_set_id(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const new_id = jsToSlice(ctx, unsafe { argv[0] });
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    if (@as(usize, el.node_id) >= el.dom_ctx.doc.nodes.items.len) return jsNewString(ctx, "");
    const node = &el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)];
    return jsNewString(ctx, node.class_attr);
}

@c fn js_element_set_class_name(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const new_cls = jsToSlice(ctx, unsafe { argv[0] });
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
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

@c fn js_element_get_node_value(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    if (@as(usize, el.node_id) < el.dom_ctx.doc.nodes.items.len) {
        const node = &el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)];
        if (node.node_type == NodeType.Text || node.node_type == NodeType.Comment) {
            return jsNewString(ctx, node.text_content);
        }
    }
    return jsNull();
}

@c fn js_element_set_node_value(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    if (@as(usize, el.node_id) < el.dom_ctx.doc.nodes.items.len) {
        const ntype = el.dom_ctx.doc.nodes.items[@as(usize, el.node_id)].node_type;
        if (ntype == NodeType.Text || ntype == NodeType.Comment) {
            return js_element_set_text_content(ctx, this_val, argc, argv);
        }
    }
    return jsUndefined();
}

@c fn js_element_get_text_content(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNewString(ctx, "");
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const text = getNodeText(el.dom_ctx.doc, el.node_id);
    return jsNewString(ctx, text);
}

@c fn js_element_set_text_content(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const new_text = jsToSlice(ctx, unsafe { argv[0] });
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const html = el.dom_ctx.doc.getInnerHtml(el.node_id) catch return jsNewString(ctx, "");
    defer el.dom_ctx.doc.allocator.free(html);
    return jsNewString(ctx, html);
}

@c fn js_element_set_inner_html(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsUndefined();
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsUndefined();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const html = jsToSlice(ctx, unsafe { argv[0] });
    defer std.heap.page_allocator.free(html);
    el.dom_ctx.doc.setInnerHtml(el.node_id, html) catch return jsUndefined();
    el.dom_ctx.recordInnerHtmlRebuilt(el.node_id);
    el.dom_ctx.layout_dirty = true;
    return jsUndefined();
}

@c fn js_element_get_outer_html(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNewString(ctx, "");
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const html = el.dom_ctx.doc.getOuterHtml(el.node_id) catch return jsNewString(ctx, "");
    defer el.dom_ctx.doc.allocator.free(html);
    return jsNewString(ctx, html);
}

@c fn js_element_clone_node(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    let deep = false;
    if (argc >= 1) {
        deep = jsToBool(ctx, unsafe { argv[0] });
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };

    let ns: []const u8 = "";
    let free_ns = false;
    if (unsafe { argv[0] }.tag != JS_TAG_NULL && unsafe { argv[0] }.tag != JS_TAG_UNDEFINED) {
        ns = jsToSlice(ctx, unsafe { argv[0] });
        free_ns = true;
    }
    defer {
        if (free_ns) std.heap.page_allocator.free(ns);
    }

    const local_name = jsToSlice(ctx, unsafe { argv[1] });
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };

    let ns: []const u8 = "";
    let free_ns = false;
    if (unsafe { argv[0] }.tag != JS_TAG_NULL && unsafe { argv[0] }.tag != JS_TAG_UNDEFINED) {
        ns = jsToSlice(ctx, unsafe { argv[0] });
        free_ns = true;
    }
    defer {
        if (free_ns) std.heap.page_allocator.free(ns);
    }

    const qname = jsToSlice(ctx, unsafe { argv[1] });
    defer std.heap.page_allocator.free(qname);

    const val = jsToSlice(ctx, unsafe { argv[2] });
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };

    let ns: []const u8 = "";
    let free_ns = false;
    if (unsafe { argv[0] }.tag != JS_TAG_NULL && unsafe { argv[0] }.tag != JS_TAG_UNDEFINED) {
        ns = jsToSlice(ctx, unsafe { argv[0] });
        free_ns = true;
    }
    defer {
        if (free_ns) std.heap.page_allocator.free(ns);
    }

    const local_name = jsToSlice(ctx, unsafe { argv[1] });
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
        * = JS_SetPropertyStr(ctx, this_val, "_parentNode", JS_DupValue(ctx, unsafe { argv[0] }));
    }
    return jsUndefined();
}

@c fn js_element_get_first_child(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const doc = el.dom_ctx.doc;
    if (el.node_id == NULL_NODE || @as(usize, el.node_id) >= doc.nodes.items.len) return jsNull();
    const node = &doc.nodes.items[@as(usize, el.node_id)];
    if (node.first_child == NULL_NODE) return jsNull();
    return createJsElement(ctx, el.dom_ctx, node.first_child);
}

@c fn js_element_get_first_element_child(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const doc = el.dom_ctx.doc;
    if (el.node_id == NULL_NODE || @as(usize, el.node_id) >= doc.nodes.items.len) return jsNull();
    let ch = doc.nodes.items[@as(usize, el.node_id)].first_child;
    while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
        const cn = &doc.nodes.items[@as(usize, ch)];
        if (cn.node_type == NodeType.Element) {
            return createJsElement(ctx, el.dom_ctx, ch);
        }
        ch = cn.next_sibling;
    }
    return jsNull();
}

@c fn js_element_get_last_child(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    const el: *JsElement = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const sel_str = jsToSlice(ctx, unsafe { argv[0] });
    defer std.heap.page_allocator.free(sel_str);
    const target_id = Matcher.querySelector(dom_ctx.doc, sel_str);
    if (target_id == NULL_NODE) return jsNull();
    return createJsElement(ctx, dom_ctx, target_id);
}

@c fn js_document_query_selector_all(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return JS_NewArray(ctx);
    const ptr = JS_GetOpaque(this_val, g_document_class_id);
    if (ptr == null) return JS_NewArray(ctx);
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const max_cap = dom_ctx.doc.nodes.items.len;
    if (max_cap == 0) return JS_NewArray(ctx);
    const matched_ids = std.heap.page_allocator.alloc(NodeId, max_cap) catch return JS_NewArray(ctx);
    defer std.heap.page_allocator.free(matched_ids);
    let matched_count: usize = 0;
    {
        const sel_str = jsToSlice(ctx, unsafe { argv[0] });
        defer std.heap.page_allocator.free(sel_str);
        matched_count = Matcher.querySelectorAll(dom_ctx.doc, sel_str, matched_ids);
    }
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
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const id_str = jsToSlice(ctx, unsafe { argv[0] });
    defer std.heap.page_allocator.free(id_str);
    const target_id = dom_ctx.doc.getElementById(id_str);
    if (target_id == NULL_NODE) return jsNull();
    return createJsElement(ctx, dom_ctx, target_id);
}

@c fn js_document_create_element(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    if (argc < 1) return jsNull();
    const ptr = JS_GetOpaque(this_val, g_document_class_id);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const tag_str = jsToSlice(ctx, unsafe { argv[0] });
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
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const str = jsToSlice(ctx, unsafe { argv[0] });
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
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const name_str = jsToSlice(ctx, unsafe { argv[0] });
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
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    const parent_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(parent_opaque)) };
    const alloc = std.heap.c_allocator;
    let new_doc = alloc.create(Document) catch return jsNull();
    new_doc.* = Document.init(alloc) catch return jsNull();
    const html_id = new_doc.createElement("html") catch return jsNull();
    const head_id = new_doc.createElement("head") catch return jsNull();
    const body_id = new_doc.createElement("body") catch return jsNull();
    new_doc.appendChild(html_id, head_id);
    new_doc.appendChild(html_id, body_id);
    if (argc >= 1 && !jsIsUndefined(unsafe { argv[0] })) {
        const title_str = jsToSlice(ctx, unsafe { argv[0] });
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
    * = JS_SetOpaque(doc_val, unsafe { @ptrCast(new_ctx) });
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
        const str = jsToSlice(ctx, unsafe { argv[i] });
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
        * = JS_SetPropertyUint32(ctx, args_array, @intCast(j), JS_DupValue(ctx, unsafe { argv[j] }));
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
    const format_res = JS_Eval(ctx, unsafe { @ptrCast(format_script) }, format_script.len, "format.js", 0);
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
        const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(opaque_ptr)) };
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
    if (JS_ToInt32(ctx, &nid, unsafe { argv[0] }) != 0) return jsNull();
    if (nid <= 0) return jsNull();
    const ptr = JS_GetContextOpaque(ctx);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
    const node_id = @as(NodeId, @intCast(nid));
    if (@as(usize, node_id) >= dom_ctx.doc.nodes.items.len) return jsNull();
    return createJsElement(ctx, dom_ctx, node_id);
}

@c fn js_capsurf_load_fixture(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 1) return jsNull();
    let len: usize = 0;
    const ptr = JS_ToCStringLen2(ctx, &len, unsafe { argv[0] }, 0);
    if (ptr != null) {
        defer JS_FreeCString(ctx, ptr);
        let url_buf: [4096]u8 = [0; 4096];
        const actual_len = @min(len, url_buf.len);
        @memcpy(url_buf[0..actual_len], ptr.?[0..actual_len]);
        const url_slice = url_buf[0..actual_len];

        let cookie_jar_ptr: (*CookieJar)? = null;
        if (opaq := JS_GetContextOpaque(ctx)) {
            const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(opaq)) };
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
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
    if (dom_ctx.cookie_jar == null) return jsNull();

    let len: usize = 0;
    const str_ptr = JS_ToCStringLen2(ctx, &len, unsafe { argv[0] }, 0);
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
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
    if (dom_ctx.cookie_jar == null) return jsNull();

    let url_len: usize = 0;
    const url_ptr = JS_ToCStringLen2(ctx, &url_len, unsafe { argv[0] }, 0);
    if (url_ptr == null) return jsNull();
    defer JS_FreeCString(ctx, url_ptr);

    let val_len: usize = 0;
    const val_ptr = JS_ToCStringLen2(ctx, &val_len, unsafe { argv[1] }, 0);
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

@c fn js_capsurf_native_record_fetch(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = this_val;
    if (argc < 1) return jsNull();
    const ptr = JS_GetContextOpaque(ctx);
    if (ptr == null) return jsNull();
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };

    let u_len: usize = 0;
    const u_ptr = JS_ToCStringLen2(ctx, &u_len, unsafe { argv[0] }, 0);
    if (u_ptr != null) {
        defer JS_FreeCString(ctx, u_ptr);
        let m_str: []const u8 = "application/octet-stream";
        if (argc >= 2) {
            let m_len: usize = 0;
            const m_ptr = JS_ToCStringLen2(ctx, &m_len, unsafe { argv[1] }, 0);
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
    const dom_ctx: *JsDomContext = unsafe { @ptrCast(@alignCast(ptr.?)) };
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
    defineGetterSetter(raw_ctx, el_proto, "nodeValue", js_element_get_node_value, js_element_set_node_value);
    defineGetterSetter(raw_ctx, el_proto, "textContent", js_element_get_text_content, js_element_set_text_content);
    defineGetterSetter(raw_ctx, el_proto, "innerHTML", js_element_get_inner_html, js_element_set_inner_html);
    defineGetterSetter(raw_ctx, el_proto, "outerHTML", js_element_get_outer_html, null);
    defineGetterSetter(raw_ctx, el_proto, "parentElement", js_element_get_parent_element, null);
    defineGetterSetter(raw_ctx, el_proto, "parentNode", js_element_get_parent_node, js_element_set_parent_node);
    defineGetterSetter(raw_ctx, el_proto, "firstChild", js_element_get_first_child, null);
    defineGetterSetter(raw_ctx, el_proto, "firstElementChild", js_element_get_first_element_child, null);
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
    JS_SetContextOpaque(raw_ctx, unsafe { @ptrCast(dom_ctx) });

    // Create document instance
    const doc_val = JS_NewObjectClass(raw_ctx, g_document_class_id);
    * = JS_SetOpaque(doc_val, unsafe { @ptrCast(dom_ctx) });
    * = JS_SetPropertyStr(raw_ctx, doc_val, "_nid", jsInt(0));

    // Register globals
    const global_obj = ctx.getGlobal();
    defer ctx.freeValue(global_obj);

    * = JS_SetPropertyStr(raw_ctx, global_obj, "_nid", jsInt(-1));
    defineMethod(raw_ctx, global_obj, "__capsurf_get_computed_prop", js_get_computed_prop, 2);
    defineMethod(raw_ctx, global_obj, "__capsurf_get_layout_box", js_get_layout_box, 1);

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
    defineMethod(raw_ctx, global_obj, "__capsurf_native_record_fetch", js_capsurf_native_record_fetch, 2);

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
        "    if (typeof id === 'string' && id.startsWith('node:')) {" +
        "      const nid = parseInt(id.slice(5), 10);" +
        "      if (nid > 0 && typeof document !== 'undefined' && document) {" +
        "        const stack = [document];" +
        "        while (stack.length > 0) {" +
        "          const cur = stack.pop();" +
        "          if (cur && cur._nid === nid) return cur;" +
        "          if (cur && cur.shadowRoot) stack.push(cur.shadowRoot);" +
        "          const ch = cur && cur.childNodes;" +
        "          if (ch && ch.length) {" +
        "            for (let i = ch.length - 1; i >= 0; i--) stack.push(ch[i]);" +
        "          }" +
        "        }" +
        "      }" +
        "      return undefined;" +
        "    }" +
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
        "    if (t === 'boolean' || t === 'string') return { type: t, value: val };" +
        "    if (t === 'number') {" +
        "      if (Number.isNaN(val)) return { type: 'number', unserializableValue: 'NaN' };" +
        "      if (Object.is(val, -0)) return { type: 'number', unserializableValue: '-0' };" +
        "      if (val === Infinity) return { type: 'number', unserializableValue: 'Infinity' };" +
        "      if (val === -Infinity) return { type: 'number', unserializableValue: '-Infinity' };" +
        "      return { type: 'number', value: val };" +
        "    }" +
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
        "      else if (typeof val.nodeType === 'number') obj.subtype = 'node';" +
        "      else if (val instanceof Date) obj.subtype = 'date';" +
        "      else if (val instanceof RegExp) obj.subtype = 'regexp';" +
        "      else if (val instanceof Error) obj.subtype = 'error';" +
        "      else if (typeof Map !== 'undefined' && val instanceof Map) obj.subtype = 'map';" +
        "      else if (typeof Set !== 'undefined' && val instanceof Set) obj.subtype = 'set';" +
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
        "  globalThis.__capsurf_call_fn = function (id, fnDecl, argsJson) {" +
        "    const target = globalThis.__capsurf_get_object(id);" +
        "    if (!target && id !== 'global' && id !== 'globalThis') {" +
        "      throw new Error('Could not find object with id ' + id);" +
        "    }" +
        "    let fn;" +
        "    try {" +
        "      fn = (0, eval)('(' + fnDecl + ')');" +
        "    } catch (e) {" +
        "      if (e instanceof SyntaxError) {" +
        "        const obj = (0, eval)('({' + fnDecl + '})');" +
        "        const keys = Object.keys(obj);" +
        "        if (keys.length === 1) fn = obj[keys[0]];" +
        "      } else {" +
        "        throw e;" +
        "      }" +
        "    }" +
        "    if (typeof fn !== 'function') throw new TypeError('functionDeclaration did not evaluate to a function');" +
        "    const rawArgs = argsJson ? JSON.parse(argsJson) : [];" +
        "    const args = [];" +
        "    for (let i = 0; i < rawArgs.length; i++) {" +
        "      const arg = rawArgs[i] || {};" +
        "      if (arg.objectId) {" +
        "        const resolved = globalThis.__capsurf_get_object(arg.objectId);" +
        "        if (resolved === undefined && arg.objectId !== 'global' && arg.objectId !== 'globalThis') {" +
        "          throw new Error('Could not find object with id ' + arg.objectId);" +
        "        }" +
        "        args.push(resolved);" +
        "      } else if (arg.unserializableValue !== undefined) {" +
        "        const u = arg.unserializableValue;" +
        "        if (u === 'NaN') args.push(NaN);" +
        "        else if (u === '-0') args.push(-0);" +
        "        else if (u === 'Infinity') args.push(Infinity);" +
        "        else if (u === '-Infinity') args.push(-Infinity);" +
        "        else if (/^-?\\d+n$/.test(u)) args.push(BigInt(u.slice(0, -1)));" +
        "        else args.push(undefined);" +
        "      } else if ('value' in arg) {" +
        "        args.push(arg.value);" +
        "      } else {" +
        "        args.push(undefined);" +
        "      }" +
        "    }" +
        "    return fn.apply(target || globalThis, args);" +
        "  };" +
        "  if (typeof document !== 'undefined' && !document.fonts) {" +
        "    const _readyP = Promise.resolve();" +
        "    const _fontsObj = { status: 'loaded', size: 0, ready: _readyP, check: function () { return true; }, load: function () { return Promise.resolve([]); }, forEach: function () {} };" +
        "    try { Object.defineProperty(document, 'fonts', { get: function () { return _fontsObj; }, configurable: true }); } catch (e) {}" +
        "  }" +
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
        " function _hasContainerRules() { try { const st = document.querySelectorAll('style'); for (let i = 0; i < st.length; i++) { if ((st[i].textContent || '').indexOf('@container') !== -1) return true; } } catch (e) {} return false; }" +
        " function _cpProp(el, name) { if (!el || typeof el._nid !== 'number' || el._nid <= 0 || typeof globalThis.__capsurf_get_computed_prop !== 'function') return ''; return globalThis.__capsurf_get_computed_prop(el._nid, name) || ''; }" +
        " function _decodeNumEnt(s) { if (!s || s.indexOf('&#') === -1) return s; return s.replace(/&#x([0-9a-fA-F]+);/g, function(_, h) { const cp = parseInt(h, 16); return isNaN(cp) ? _ : String.fromCodePoint(cp); }).replace(/&#([0-9]+);/g, function(_, d) { const cp = parseInt(d, 10); return isNaN(cp) ? _ : String.fromCodePoint(cp); }); }" +
        " function _firstStrongDir(str) { if (!str || typeof str !== 'string') return null; str = _decodeNumEnt(str); for (let i = 0; i < str.length; i++) { const cp = str.codePointAt(i); if (cp > 0xFFFF) i++; if ((cp >= 0x0590 && cp <= 0x08FF) || (cp >= 0xFB1D && cp <= 0xFDFF) || (cp >= 0xFE70 && cp <= 0xFEFF) || (cp >= 0x10800 && cp <= 0x10FFF) || (cp >= 0x1E800 && cp <= 0x1EFFF)) return 'rtl'; if ((cp >= 0x0041 && cp <= 0x005A) || (cp >= 0x0061 && cp <= 0x007A) || (cp >= 0x00C0 && cp <= 0x02B8) || (cp >= 0x0370 && cp <= 0x058F) || (cp >= 0x0900 && cp <= 0x1FFF) || (cp >= 0x2C00 && cp <= 0x2DFF) || (cp >= 0x3040 && cp <= 0xA4CF) || (cp >= 0xAC00 && cp <= 0xD7AF)) return 'ltr'; } return null; }" +
        " function _formDirAuto(el) { if (!el || el.nodeType !== 1) return null; const tag = String(el.localName || el.tagName || '').toLowerCase(); if (tag !== 'input' && tag !== 'textarea') return null; const da = String((el.getAttribute && el.getAttribute('dir')) || '').trim().toLowerCase(); if (da !== 'auto') return null; let val = ''; if (tag === 'input') { val = el._valueDirty ? String(el.value || '') : String((el.getAttribute && el.getAttribute('value')) || el.value || ''); } else { val = el._valueDirty ? String(el.value || '') : String(el.value || el.textContent || ''); } return _firstStrongDir(val) || 'ltr'; }" +
        " function _linkMediaMatches(linkEl) { if (!linkEl || typeof linkEl.getAttribute !== 'function') return true; const m = String(linkEl.getAttribute('media') || '').trim(); if (!m) return true; if (typeof window !== 'undefined' && typeof window.matchMedia === 'function') { try { return Boolean(window.matchMedia(m).matches); } catch (e) {} } const lm = m.toLowerCase(); if ((lm.indexOf('print') !== -1 || lm.indexOf('speech') !== -1) && lm.indexOf('screen') === -1 && lm.indexOf('all') === -1) return false; return true; }" +
        " if (typeof CSSStyleSheet !== 'undefined' && CSSStyleSheet.prototype && typeof CSSStyleSheet.prototype._syncFromOwner === 'function') { const _origSync = CSSStyleSheet.prototype._syncFromOwner; CSSStyleSheet.prototype._syncFromOwner = function () { _origSync.call(this); const on = this.ownerNode; if (on && String(on.localName || on.tagName || '').toLowerCase() === 'link') { if (!_linkMediaMatches(on)) { this._cascadeCssText = ''; } } }; }" +
        " function _parseColorSchemeDark(raw) { const s = String(raw || '').trim().toLowerCase(); if (!s) return null; if (s === 'normal') return false; const toks = s.split(/\\s+/); const hasL = toks.indexOf('light') !== -1; const hasD = toks.indexOf('dark') !== -1; if (hasD && !hasL) return true; if (hasL) return false; return null; }" +
        " function _usedColorSchemeDark(el) { let cur = el; while (cur && cur.nodeType === 1) { let raw = ''; try { if (cur.style && typeof cur.style.getPropertyValue === 'function') raw = cur.style.getPropertyValue('color-scheme') || ''; } catch (e) {} if (!raw) { try { raw = _origGcs(cur).getPropertyValue('color-scheme') || ''; } catch (e) {} } const parsed = _parseColorSchemeDark(raw); if (parsed !== null) return parsed; cur = cur.parentElement; } return false; }" +
        " function _normRgbSpace(raw) { const m = String(raw || '').trim().match(/^rgb\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\)$/i); if (m) return 'rgb(' + m[1] + ', ' + m[2] + ', ' + m[3] + ')'; return _normColor(raw); }" +
        " function _resolveLightDark(val, el) { if (typeof val !== 'string') return val; const low = val.trim().toLowerCase(); if (!low.startsWith('light-dark(') || !low.endsWith(')')) return val; const inner = val.trim().slice(11, -1); let depth = 0, splitIdx = -1; for (let i = 0; i < inner.length; i++) { const ch = inner[i]; if (ch === '(') depth++; else if (ch === ')') depth--; else if (ch === ',' && depth === 0) { splitIdx = i; break; } } if (splitIdx === -1) return val; const lightArg = inner.slice(0, splitIdx).trim(); const darkArg = inner.slice(splitIdx + 1).trim(); const picked = _usedColorSchemeDark(el) ? darkArg : lightArg; return _normRgbSpace(picked); }" +
        " function _hasLinkSheets() { try { const ls = document.querySelectorAll('link'); for (let i = 0; i < ls.length; i++) { const r = String(ls[i].getAttribute && ls[i].getAttribute('rel') || '').toLowerCase(); if (r.indexOf('stylesheet') !== -1) return true; } } catch (e) {} return false; }" +
        " const _wrappedGcs = function (el, pseudo) { const cs = _origGcs(el, pseudo); return new Proxy(cs, { get: function (target, prop) { if (prop === 'getPropertyValue') { return function (name) { const ln = String(name).trim().toLowerCase(); if (ln === 'accent-color') return _resolveAccent(el, target); if (ln === 'direction') { const idir = (el && el.style && typeof el.style.getPropertyValue === 'function') ? String(el.style.getPropertyValue('direction') || '').trim().toLowerCase() : ''; if (idir !== 'ltr' && idir !== 'rtl') { const fd = _formDirAuto(el); if (fd) return fd; } } if (ln === 'container-type' || ln === 'container-name') { const cv = _cpProp(el, ln); if (cv) return cv; } if (_hasContainerRules() && (ln === 'padding-left' || ln === 'padding-right' || ln === 'padding-top' || ln === 'padding-bottom' || ln === 'color' || ln === 'background-color')) { const cv = _cpProp(el, ln); if (cv) return cv; } let res = target.getPropertyValue(name); if (typeof res === 'string' && res.toLowerCase().indexOf('light-dark(') !== -1) res = _resolveLightDark(res, el); if (!res && ln === 'visibility') return 'visible'; if (!res && ln === 'background-color' && _hasLinkSheets()) { const cv = _cpProp(el, 'background-color'); if (cv && cv !== 'rgba(0, 0, 0, 0)') return cv; } return res; }; } if (prop === 'accentColor' || prop === 'accent-color') return _resolveAccent(el, target); if (prop === 'direction') { const idir = (el && el.style && typeof el.style.getPropertyValue === 'function') ? String(el.style.getPropertyValue('direction') || '').trim().toLowerCase() : ''; if (idir !== 'ltr' && idir !== 'rtl') { const fd = _formDirAuto(el); if (fd) return fd; } } if (prop === 'containerType' || prop === 'container-type') { const cv = _cpProp(el, 'container-type'); if (cv) return cv; } if (prop === 'containerName' || prop === 'container-name') { const cv = _cpProp(el, 'container-name'); if (cv) return cv; } if (_hasContainerRules() && (prop === 'paddingLeft' || prop === 'paddingRight' || prop === 'paddingTop' || prop === 'paddingBottom' || prop === 'color' || prop === 'backgroundColor')) { const cv = _cpProp(el, String(prop)); if (cv) return cv; } let v = target[prop]; if (typeof v === 'string' && v.toLowerCase().indexOf('light-dark(') !== -1) v = _resolveLightDark(v, el); if (!v && prop === 'visibility') return 'visible'; if (!v && (prop === 'backgroundColor' || prop === 'background-color') && _hasLinkSheets()) { const cv = _cpProp(el, 'background-color'); if (cv && cv !== 'rgba(0, 0, 0, 0)') return cv; } return typeof v === 'function' ? v.bind(target) : v; } }); };" +
        " globalThis.getComputedStyle = _wrappedGcs; if (typeof window !== 'undefined') window.getComputedStyle = _wrappedGcs;" +
        " if (typeof Element !== 'undefined' && Element.prototype && typeof Element.prototype.matches === 'function') { const _origMatches = Element.prototype.matches; Element.prototype.matches = function (sel) { const m = String(sel || '').trim().match(/^:dir\\(\\s*([a-zA-Z0-9_-]+)\\s*\\)$/i); if (m) { const fd = _formDirAuto(this); if (fd) { const td = m[1].toLowerCase(); return (td === 'ltr' || td === 'rtl') && fd === td; } } return _origMatches.call(this, sel); }; }" +
        " (function () { if (typeof globalThis.fetch === 'function') { const _origFetch = globalThis.fetch; const _wrappedFetch = function (input, init) { try { const raw = typeof input === 'string' ? input : (input && input.url ? String(input.url) : String(input || '')); const pth = raw.replace(/^https?:\\/\\/[^\\/]+/i, '').split('?')[0].split('#')[0]; if (pth.startsWith('/__ks_resource/')) { return Promise.resolve({ ok: false, status: 404, statusText: 'Not Found', url: raw, headers: { get: function () { return null; } }, text: function () { return Promise.resolve(''); }, arrayBuffer: function () { return Promise.resolve(new ArrayBuffer(0)); } }); } } catch (e) {} return _origFetch.apply(this, arguments); }; globalThis.fetch = _wrappedFetch; if (typeof window !== 'undefined') window.fetch = _wrappedFetch; } })();" +
        " (function () { if (typeof Element === 'undefined' || !Element.prototype || typeof Element.prototype.getBoundingClientRect !== 'function') return; const _origGbcr = Element.prototype.getBoundingClientRect; const _animStarts = new WeakMap(); function _parseDurMs(tok) { const m = String(tok || '').trim().match(/^([0-9.]+)(ms|s)$/i); if (!m) return 0; const v = parseFloat(m[1]); return m[2].toLowerCase() === 's' ? v * 1000 : v; } function _findKeyframes(name) { const styles = document.querySelectorAll('style'); for (let i = 0; i < styles.length; i++) { const txt = styles[i].textContent || ''; const idx = txt.toLowerCase().indexOf('@keyframes'); if (idx === -1) continue; let pos = 0; while (true) { const kpos = txt.toLowerCase().indexOf('@keyframes', pos); if (kpos === -1) break; const after = txt.slice(kpos + 10); const nm = after.match(/^\\s+([a-zA-Z0-9_-]+)\\s*\\{/); if (nm && nm[1] === name) { const bStart = kpos + 10 + nm[0].length; let depth = 1, j = bStart; while (j < txt.length && depth > 0) { if (txt[j] === '{') depth++; else if (txt[j] === '}') depth--; j++; } return txt.slice(bStart, j - 1); } pos = kpos + 10; } } return null; } function _extractKfPx(body, prop) { const reFrom = new RegExp('(?:from|0%)\\s*\\{([^}]*)\\}', 'i'); const reTo = new RegExp('(?:to|100%)\\s*\\{([^}]*)\\}', 'i'); const mf = body.match(reFrom), mt = body.match(reTo); if (!mf || !mt) return null; const reP = new RegExp(prop + '\\s*:\\s*([0-9.]+)px', 'i'); const pf = mf[1].match(reP), pt = mt[1].match(reP); if (!pf || !pt) return null; return [parseFloat(pf[1]), parseFloat(pt[1])]; } Element.prototype.getBoundingClientRect = function () { const rect = _origGbcr.call(this); try { const cs = _origGcs(this); let anim = String((cs && (cs.getPropertyValue('animation') || cs.getPropertyValue('animation-name'))) || '').trim(); if (!anim) { const sts = document.querySelectorAll('style'); for (let si = 0; si < sts.length; si++) { let t = sts[si].textContent || ''; while (true) { const kp = t.toLowerCase().indexOf('@keyframes'); if (kp === -1) break; const ob = t.indexOf('{', kp); if (ob === -1) break; let d = 1, jj = ob + 1; while (jj < t.length && d > 0) { if (t[jj] === '{') d++; else if (t[jj] === '}') d--; jj++; } t = t.slice(0, kp) + t.slice(jj); } const rr = /([^{}]+)\\{([^{}]*)\\}/g; let mm; while ((mm = rr.exec(t)) !== null) { const sels = mm[1].split(','); let hit = false; for (let k = 0; k < sels.length; k++) { const ss = sels[k].trim(); if (ss && typeof this.matches === 'function' && this.matches(ss)) { hit = true; break; } } if (hit) { const am = mm[2].match(/(?:^|;)\\s*animation\\s*:\\s*([^;]+)/i); if (am) anim = am[1].trim(); } } } } if (anim && anim !== 'none') { const toks = anim.split(/\\s+/); let name = '', durMs = 0, isInf = false, isAlt = false; for (let i = 0; i < toks.length; i++) { const t = toks[i].toLowerCase(); if (t === 'infinite') isInf = true; else if (t === 'alternate' || t === 'alternate-reverse') isAlt = true; else if (!durMs && /^[0-9.]+(ms|s)$/.test(t)) durMs = _parseDurMs(t); else if (!name && t !== 'linear' && t !== 'ease' && t !== 'ease-in' && t !== 'ease-out' && t !== 'ease-in-out' && t !== 'normal' && t !== 'reverse' && t !== 'forwards' && t !== 'backwards' && t !== 'both' && t !== 'none' && t !== 'running' && t !== 'paused' && !/^[0-9.]+$/.test(t)) name = toks[i]; } if (!durMs) durMs = _parseDurMs(cs.getPropertyValue('animation-duration')); if (name && durMs > 0) { const kf = _findKeyframes(name); if (kf) { const wPair = _extractKfPx(kf, 'width'); const hPair = _extractKfPx(kf, 'height'); if (wPair || hPair) { let t0 = _animStarts.get(this); const now = (typeof performance !== 'undefined' && performance.now) ? performance.now() : Date.now(); if (t0 === undefined) { t0 = now; _animStarts.set(this, t0); } const elapsed = Math.max(0, now - t0); const cyc = elapsed / durMs; const iter = Math.floor(cyc); let frac = isInf ? (cyc - iter) : Math.min(1, cyc); if (isAlt && (iter % 2 === 1)) frac = 1 - frac; const nw = wPair ? (wPair[0] + (wPair[1] - wPair[0]) * frac) : rect.width; const nh = hPair ? (hPair[0] + (hPair[1] - hPair[0]) * frac) : rect.height; return { x: rect.x, y: rect.y, left: rect.left, top: rect.top, width: nw, height: nh, right: rect.left + nw, bottom: rect.top + nh }; } } } } } catch (e) {} return rect; }; })();" +
        " (function () { const proto = (typeof HTMLElement !== 'undefined' && HTMLElement.prototype) ? HTMLElement.prototype : (typeof Element !== 'undefined' ? Element.prototype : null); if (!proto) return; function _isNoBox(el) { if (!el || el.nodeType !== 1 || !el.isConnected) return true; const cs = _wrappedGcs(el); const d = String((cs && cs.display) || '').trim().toLowerCase(); return d === 'none' || d === 'contents'; } function _calcOffsetParent(el) { if (!el || el.nodeType !== 1 || !el.isConnected) return null; const tag = String(el.localName || el.tagName || '').toLowerCase(); if (tag === 'html' || tag === 'body') return null; if (_isNoBox(el)) return null; const cs = _wrappedGcs(el); const pos = String((cs && cs.position) || 'static').trim().toLowerCase(); if (pos === 'fixed') return null; let anc = el.parentElement; while (anc) { const atag = String(anc.localName || anc.tagName || '').toLowerCase(); if (atag === 'body') return anc; if (atag === 'html') return null; const acs = _wrappedGcs(anc); const apos = String((acs && acs.position) || 'static').trim().toLowerCase(); if (apos && apos !== 'static') return anc; if (pos === 'static' && (atag === 'td' || atag === 'th' || atag === 'table')) return anc; anc = anc.parentElement; } return null; } function _calcOffsetCoord(el, axis) { if (!el || el.nodeType !== 1 || !el.isConnected) return 0; const tag = String(el.localName || el.tagName || '').toLowerCase(); if (tag === 'html' || tag === 'body') return 0; if (_isNoBox(el)) return 0; const box = (typeof el._nid === 'number' && el._nid > 0 && typeof globalThis.__capsurf_get_layout_box === 'function') ? globalThis.__capsurf_get_layout_box(el._nid) : null; if (!box) return 0; const op = _calcOffsetParent(el); const elCoord = axis === 'y' ? box.y : box.x; if (!op) return Math.round(elCoord); const optag = String(op.localName || op.tagName || '').toLowerCase(); if (optag === 'body') { const bcs = _wrappedGcs(op); const bpos = String((bcs && bcs.position) || 'static').trim().toLowerCase(); if (bpos === 'static') return Math.round(elCoord); } const opBox = (typeof op._nid === 'number' && op._nid > 0 && typeof globalThis.__capsurf_get_layout_box === 'function') ? globalThis.__capsurf_get_layout_box(op._nid) : null; if (!opBox) return Math.round(elCoord); const opOrigin = axis === 'y' ? (opBox.y + opBox.borderTop) : (opBox.x + opBox.borderLeft); return Math.round(elCoord - opOrigin); } Object.defineProperty(proto, 'offsetParent', { get: function () { return _calcOffsetParent(this); }, configurable: true }); Object.defineProperty(proto, 'offsetTop', { get: function () { return _calcOffsetCoord(this, 'y'); }, configurable: true }); Object.defineProperty(proto, 'offsetLeft', { get: function () { return _calcOffsetCoord(this, 'x'); }, configurable: true }); })();" +
        " (function () { if (typeof Range === 'undefined' || !Range.prototype || typeof Range.prototype.getClientRects !== 'function') return; const _origGcr = Range.prototype.getClientRects; function _isWideCp(cp) { return (cp >= 0x2E80 && cp <= 0x9FFF) || (cp >= 0xAC00 && cp <= 0xD7AF) || (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0xFE30 && cp <= 0xFE6F) || (cp >= 0xFF00 && cp <= 0xFF60) || (cp >= 0xFFE0 && cp <= 0xFFE6) || (cp >= 0x20000 && cp <= 0x2FA1F); } function _measureRangeStr(str, fsz) { let w = 0; for (let i = 0; i < str.length; i++) { const cp = str.codePointAt(i); if (cp > 0xFFFF) i++; w += _isWideCp(cp) ? fsz : (fsz * 0.5); } return w; } Range.prototype.getClientRects = function () { const list = _origGcr.call(this); try { if (!this.collapsed && list && list.length > 0 && this.startContainer) { const sc = this.startContainer; const ec = this.endContainer; let txt = ''; let hostEl = null; if (sc === ec && sc.nodeType === 3) { txt = String(sc.data || sc.textContent || '').slice(this.startOffset, this.endOffset); hostEl = sc.parentElement || sc.parentNode; } else if (sc === ec && sc.nodeType === 1 && this.startOffset === 0 && this.endOffset === (sc.childNodes ? sc.childNodes.length : 0)) { txt = String(sc.textContent || ''); hostEl = sc; } if (txt) { let hasWide = false; for (let i = 0; i < txt.length; i++) { const cp = txt.codePointAt(i); if (cp > 0xFFFF) i++; if (_isWideCp(cp)) { hasWide = true; break; } } if (hasWide) { let fsz = 16; if (hostEl && hostEl.nodeType === 1) { try { const cs = _origGcs(hostEl); const fs = parseFloat(String((cs && cs.fontSize) || '')); if (Number.isFinite(fs) && fs > 0) fsz = fs; } catch (e) {} } const mw = _measureRangeStr(txt, fsz); const r0 = list[0]; const nr = typeof DOMRect === 'function' ? new DOMRect(r0.x, r0.y, mw, Math.max(r0.height || 0, fsz)) : { x: r0.x, y: r0.y, left: r0.left, top: r0.top, width: mw, height: Math.max(r0.height || 0, fsz), right: r0.left + mw, bottom: r0.top + Math.max(r0.height || 0, fsz) }; const out = [nr]; out.item = function (idx) { return out[idx] || null; }; return out; } } } } catch (e) {} return list; }; })();" +
        " })();";
    const accent_val = ctx.eval(accent_script);
    ctx.freeValue(accent_val);

    const ua_widgets_script =
        "(function() {" +
        // 1. iframe bitmap bridge (kitesurf.internals.domDebug.setIframeImage)
        "  const ksSym = Symbol.for('kitesurf.internals');" +
        "  const ks = globalThis[ksSym] || (globalThis[ksSym] = {});" +
        "  if (!ks.domDebug) ks.domDebug = {};" +
        "  const _iframeBitmaps = new Map();" +
        "  ks.domDebug.setIframeImage = function (nodeId, w, h, rgba) {" +
        "    if (typeof nodeId !== 'number' || nodeId <= 0 || !w || !h || !rgba) return -1;" +
        "    const bytes = (rgba instanceof Uint8Array) ? rgba : new Uint8Array(rgba.buffer || rgba);" +
        "    const need = (w | 0) * (h | 0) * 4;" +
        "    if (bytes.length < need) return -1;" +
        "    let bin = '';" +
        "    const chunk = 8192;" +
        "    for (let i = 0; i < need; i += chunk) {" +
        "      const end = i + chunk < need ? i + chunk : need;" +
        "      let s = '';" +
        "      for (let j = i; j < end; j++) s += String.fromCharCode(bytes[j]);" +
        "      bin += s;" +
        "    }" +
        "    const b64 = btoa(bin);" +
        "    _iframeBitmaps.set(nodeId, nodeId + ':' + (w | 0) + ':' + (h | 0) + ':' + b64);" +
        "    return 0;" +
        "  };" +
        "  const _origGetBitmaps = globalThis.__capsurf_get_canvas_bitmaps;" +
        "  function _bytesToB64(bytes) {" +
        "    let bin = '';" +
        "    const chunk = 8192;" +
        "    for (let i = 0; i < bytes.length; i += chunk) {" +
        "      const end = i + chunk < bytes.length ? i + chunk : bytes.length;" +
        "      let s = '';" +
        "      for (let j = i; j < end; j++) s += String.fromCharCode(bytes[j]);" +
        "      bin += s;" +
        "    }" +
        "    return btoa(bin);" +
        "  }" +
        "  globalThis.__capsurf_get_canvas_bitmaps = function () {" +
        "    let res = typeof _origGetBitmaps === 'function' ? _origGetBitmaps() : '';" +
        "    if (_iframeBitmaps.size > 0) {" +
        "      const extra = Array.from(_iframeBitmaps.values()).join(';');" +
        "      res = res ? (res + ';' + extra) : extra;" +
        "    }" +
        "    try {" +
        "      const curKs = globalThis[ksSym];" +
        "      const rbiFn = curKs && curKs.domDebug && typeof curKs.domDebug.renderRbiResources === 'function' ? curKs.domDebug.renderRbiResources : null;" +
        "      if (rbiFn && typeof document !== 'undefined' && document.querySelectorAll) {" +
        "        const imgs = [];" +
        "        const allImgs = document.querySelectorAll('img[src]');" +
        "        for (let i = 0; i < allImgs.length; i++) {" +
        "          const im = allImgs[i];" +
        "          if (im && typeof im._nid === 'number' && im._nid > 0) imgs.push(im);" +
        "        }" +
        "        if (imgs.length > 0) {" +
        "          const hashes = new Uint8Array(imgs.length * 8);" +
        "          for (let i = 0; i < imgs.length; i++) {" +
        "            const nid = imgs[i]._nid >>> 0;" +
        "            hashes[i * 8] = nid & 0xff;" +
        "            hashes[i * 8 + 1] = (nid >>> 8) & 0xff;" +
        "            hashes[i * 8 + 2] = (nid >>> 16) & 0xff;" +
        "            hashes[i * 8 + 3] = (nid >>> 24) & 0xff;" +
        "          }" +
        "          const out = rbiFn(new Uint8Array(0), new Uint8Array(0), hashes);" +
        "          if (out && out.length >= 4) {" +
        "            const view = new DataView(out.buffer, out.byteOffset, out.byteLength);" +
        "            const count = view.getUint32(0, true);" +
        "            let pos = 4;" +
        "            const rbiList = [];" +
        "            for (let i = 0; i < count && pos + 13 <= out.length; i++) {" +
        "              const kind = out[pos];" +
        "              const payloadLen = view.getUint32(pos + 9, true);" +
        "              if (kind === 2 && pos + 22 <= out.length) {" +
        "                const w = view.getUint32(pos + 13, true);" +
        "                const h = view.getUint32(pos + 17, true);" +
        "                const need = w * h * 4;" +
        "                const rgbaOff = pos + 22;" +
        "                if (w > 0 && h > 0 && rgbaOff + need <= out.length) {" +
        "                  const targetNid = (out[pos + 1] | (out[pos + 2] << 8) | (out[pos + 3] << 16) | (out[pos + 4] << 24)) >>> 0;" +
        "                  const nid = targetNid > 0 ? targetNid : (i < imgs.length ? imgs[i]._nid : 0);" +
        "                  if (nid > 0) {" +
        "                    const b64 = _bytesToB64(out.subarray(rgbaOff, rgbaOff + need));" +
        "                    rbiList.push(nid + ':' + w + ':' + h + ':' + b64);" +
        "                  }" +
        "                }" +
        "              }" +
        "              pos += 13 + payloadLen;" +
        "            }" +
        "            if (rbiList.length > 0) {" +
        "              const extraRbi = rbiList.join(';');" +
        "              res = res ? (res + ';' + extraRbi) : extraRbi;" +
        "            }" +
        "          }" +
        "        }" +
        "      }" +
        "    } catch (e) {}" +
        "    return res;" +
        "  };" +
        // 2. UA <select> dropdown overlay
        "  let _openSelect = null;" +
        "  let _openOverlay = null;" +
        "  function _closeSelectDropdown() {" +
        "    if (_openOverlay && _openOverlay.parentNode) {" +
        "      try { _openOverlay.parentNode.removeChild(_openOverlay); } catch (e) {}" +
        "    }" +
        "    _openOverlay = null;" +
        "    _openSelect = null;" +
        "  }" +
        "  function _openSelectDropdown(sel) {" +
        "    _closeSelectDropdown();" +
        "    if (!sel || !document.body) return;" +
        "    const r = typeof sel.getBoundingClientRect === 'function' ? sel.getBoundingClientRect() : { left: 0, bottom: 24, width: 200 };" +
        "    const overlay = document.createElement('div');" +
        "    overlay.setAttribute('data-kitesurf-ua', 'select-dropdown');" +
        "    const w = Math.max(r.width || 200, 120);" +
        "    overlay.style.cssText = 'position:fixed;left:' + (r.left || 0) + 'px;top:' + (r.bottom || 24) + 'px;width:' + w + 'px;background:#ffffff;border:1px solid #888;z-index:2147483647;display:block;';" +
        "    const opts = sel.querySelectorAll ? sel.querySelectorAll('option') : [];" +
        "    for (let i = 0; i < opts.length; i++) {" +
        "      const opt = opts[i];" +
        "      const row = document.createElement('div');" +
        "      row.textContent = opt.textContent || '';" +
        "      row.style.cssText = 'display:block;height:24px;line-height:24px;padding:0 6px;box-sizing:border-box;';" +
        "      row._ksSelect = sel;" +
        "      row._ksOption = opt;" +
        "      overlay.appendChild(row);" +
        "    }" +
        "    document.body.appendChild(overlay);" +
        "    _openSelect = sel;" +
        "    _openOverlay = overlay;" +
        "  }" +
        "  function _findSelectAncestor(el) {" +
        "    let cur = el;" +
        "    while (cur && cur.nodeType === 1) {" +
        "      if (String(cur.localName || cur.tagName || '').toLowerCase() === 'select') return cur;" +
        "      cur = cur.parentNode || cur.parentElement;" +
        "    }" +
        "    return null;" +
        "  }" +
        "  function _hitOverlayRow(ev) {" +
        "    if (!_openOverlay) return null;" +
        "    let cur = ev && ev.target;" +
        "    while (cur) {" +
        "      if (cur._ksOption && cur._ksSelect === _openSelect) return cur;" +
        "      cur = cur.parentNode || cur.parentElement;" +
        "    }" +
        "    if (ev && typeof ev.clientX === 'number' && typeof ev.clientY === 'number') {" +
        "      const kids = _openOverlay.children || [];" +
        "      for (let i = 0; i < kids.length; i++) {" +
        "        const row = kids[i];" +
        "        if (typeof row.getBoundingClientRect === 'function') {" +
        "          const rr = row.getBoundingClientRect();" +
        "          if (ev.clientX >= rr.left && ev.clientX <= rr.right && ev.clientY >= rr.top && ev.clientY <= rr.bottom) {" +
        "            return row;" +
        "          }" +
        "        }" +
        "      }" +
        "    }" +
        "    return null;" +
        "  }" +
        "  function _handleSelectEvent(ev, target) {" +
        "    if (!ev || ev._ksSelectHandled) return;" +
        "    const et = String(ev.type || '');" +
        "    if (et !== 'mousedown' && et !== 'click') return;" +
        "    ev._ksSelectHandled = true;" +
        "    if (_openOverlay && !_openOverlay.parentNode) { _openOverlay = null; _openSelect = null; }" +
        "    const tgt = (ev.target && ev.target.nodeType === 1) ? ev.target : target;" +
        "    if (et === 'click' && Date.now() - _lastSelectOpenMs < 100) return;" +
        "    if (_openOverlay) {" +
        "      const row = _hitOverlayRow({ target: tgt, clientX: ev.clientX, clientY: ev.clientY });" +
        "      if (row && row._ksOption) {" +
        "        const opt = row._ksOption;" +
        "        const sel = row._ksSelect;" +
        "        const isDis = Boolean(opt.disabled || (opt.hasAttribute && opt.hasAttribute('disabled')));" +
        "        if (!isDis && sel) {" +
        "          const newVal = (opt.hasAttribute && opt.hasAttribute('value')) ? (opt.getAttribute('value') || '') : (opt.textContent || '');" +
        "          sel.value = newVal;" +
        "          _closeSelectDropdown();" +
        "          try { sel.dispatchEvent(new Event('input', { bubbles: true })); } catch (e) {}" +
        "          try { sel.dispatchEvent(new Event('change', { bubbles: true })); } catch (e) {}" +
        "        }" +
        "        return;" +
        "      }" +
        "      const clickedSel = _findSelectAncestor(tgt);" +
        "      const wasSame = clickedSel && clickedSel === _openSelect;" +
        "      _closeSelectDropdown();" +
        "      if (clickedSel && !wasSame) { _openSelectDropdown(clickedSel); _lastSelectOpenMs = Date.now(); }" +
        "      return;" +
        "    }" +
        "    const sel = _findSelectAncestor(tgt);" +
        "    if (sel) { _openSelectDropdown(sel); _lastSelectOpenMs = Date.now(); }" +
        "  }" +
        "  let _lastSelectOpenMs = 0;" +
        "  const _wrapProtos = [];" +
        "  if (typeof EventTarget !== 'undefined' && EventTarget.prototype) _wrapProtos.push(EventTarget.prototype);" +
        "  if (typeof Node !== 'undefined' && Node.prototype) _wrapProtos.push(Node.prototype);" +
        "  if (typeof Element !== 'undefined' && Element.prototype) _wrapProtos.push(Element.prototype);" +
        "  if (typeof HTMLElement !== 'undefined' && HTMLElement.prototype) _wrapProtos.push(HTMLElement.prototype);" +
        "  if (typeof HTMLSelectElement !== 'undefined' && HTMLSelectElement.prototype) _wrapProtos.push(HTMLSelectElement.prototype);" +
        "  for (let _pi = 0; _pi < _wrapProtos.length; _pi++) {" +
        "    const _pr = _wrapProtos[_pi];" +
        "    if (Object.prototype.hasOwnProperty.call(_pr, 'dispatchEvent') && typeof _pr.dispatchEvent === 'function' && !_pr.dispatchEvent._ksWrapped) {" +
        "      const _origDispatch = _pr.dispatchEvent;" +
        "      const _wrappedDispatch = function (ev) {" +
        "        const ret = _origDispatch.call(this, ev);" +
        "        if (ret && ev && !ev.defaultPrevented) { _handleSelectEvent(ev, this); }" +
        "        return ret;" +
        "      };" +
        "      _wrappedDispatch._ksWrapped = true;" +
        "      _pr.dispatchEvent = _wrappedDispatch;" +
        "    }" +
        "  }" +
        // 3. HTMLImageElement srcset/src setAttribute terminal broken & cache-hit reload
        "  if (typeof Element !== 'undefined' && Element.prototype && typeof Element.prototype.setAttribute === 'function') {" +
        "    const _origSetAttr = Element.prototype.setAttribute;" +
        "    function _parseFirstSrcsetUrl(raw) {" +
        "      if (!raw || typeof raw !== 'string') return '';" +
        "      const parts = raw.split(',');" +
        "      for (let i = 0; i < parts.length; i++) {" +
        "        const t = parts[i].trim();" +
        "        if (!t) continue;" +
        "        const sp = t.search(/\\s/);" +
        "        const url = sp === -1 ? t : t.slice(0, sp);" +
        "        if (url) return url;" +
        "      }" +
        "      return '';" +
        "    }" +
        "    function _isFetchableImgUrl(u) {" +
        "      if (!u) return false;" +
        "      const m = u.match(/^([a-zA-Z][a-zA-Z0-9+.-]*):/);" +
        "      if (!m) return true;" +
        "      const sch = m[1].toLowerCase();" +
        "      return sch === 'http' || sch === 'https' || sch === 'data' || sch === 'blob';" +
        "    }" +
        "    Element.prototype.setAttribute = function (name, value) {" +
        "      const res = _origSetAttr.call(this, name, value);" +
        "      const tag = String(this.localName || this.tagName || '').toLowerCase();" +
        "      if (tag === 'img') {" +
        "        const ln = String(name || '').toLowerCase();" +
        "        if (ln === 'srcset' || ln === 'src') {" +
        "          const self = this;" +
        "          const srcsetRaw = this.getAttribute('srcset') || '';" +
        "          const srcRaw = this.getAttribute('src') || '';" +
        "          const candidate = srcsetRaw ? _parseFirstSrcsetUrl(srcsetRaw) : srcRaw;" +
        "          if (!candidate || !_isFetchableImgUrl(candidate)) {" +
        "            self._imgComplete = false;" +
        "            self._imgLoadSeq = (self._imgLoadSeq || 0) + 1;" +
        "            const seq = self._imgLoadSeq;" +
        "            setTimeout(function () {" +
        "              if (self._imgLoadSeq !== seq) return;" +
        "              self._imgComplete = true;" +
        "              self._lastCandidate = candidate;" +
        "              self._lastDims = { width: 0, height: 0 };" +
        "              try { self.dispatchEvent(new Event('error')); } catch (e) {}" +
        "            }, 0);" +
        "          } else if (self._lastCandidate && (candidate === self._lastCandidate || srcRaw === self._lastCandidate)) {" +
        "            self._imgComplete = false;" +
        "            self._imgLoadSeq = (self._imgLoadSeq || 0) + 1;" +
        "            const seq = self._imgLoadSeq;" +
        "            setTimeout(function () {" +
        "              if (self._imgLoadSeq !== seq) return;" +
        "              self._imgComplete = true;" +
        "              try { self.dispatchEvent(new Event('load')); } catch (e) {}" +
        "            }, 0);" +
        "          }" +
        "        }" +
        "      } else if (tag === 'input') {" +
        "        const ln = String(name || '').toLowerCase();" +
        "        if (ln === 'checked' && (!this._checkedDirty || this._uncheckedByRadioAttr)) {" +
        "          this._checkedDirty = false;" +
        "          this._uncheckedByRadioAttr = false;" +
        "          const itype = String(this.getAttribute('type') || '').toLowerCase();" +
        "          const rname = this.getAttribute('name') || '';" +
        "          if (itype === 'radio' && rname) {" +
        "            const root = (this.getRootNode ? this.getRootNode() : null) || this.ownerDocument || (typeof document !== 'undefined' ? document : null);" +
        "            if (root && root.querySelectorAll) {" +
        "              const radios = root.querySelectorAll('input[type=\"radio\"]');" +
        "              for (let ri = 0; ri < radios.length; ri++) {" +
        "                const rr = radios[ri];" +
        "                if (rr !== this && (rr.getAttribute('name') || '') === rname) {" +
        "                  if (!rr._checkedDirty) rr._uncheckedByRadioAttr = true;" +
        "                  rr._checkedDirty = true;" +
        "                  rr._checked = false;" +
        "                }" +
        "              }" +
        "            }" +
        "          }" +
        "        }" +
        "      }" +
        "      return res;" +
        "    };" +
        "  }" +
        "})();";
    const ua_widgets_val = ctx.eval(ua_widgets_script);
    ctx.freeValue(ua_widgets_val);

    return dom_ctx;
}
