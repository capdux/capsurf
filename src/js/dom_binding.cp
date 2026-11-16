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
    jsInt,
    jsFloat,
    jsException,
    jsIsException,
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
} from "./qjs_engine.cp";

import { Document, NodeId, NULL_NODE, NodeType } from "../dom/dom.cp";
import { Matcher } from "../css/matcher.cp";
import { LayoutTree, Rect } from "../layout/layout.cp";
import { RUNTIME_CHUNKS } from "./runtime_source.cp";

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
    timestamp: i64 = 0,

    fn getLevel(*_): []const u8 {
        return _.level[0.._.level_len];
    }

    fn getText(*_): []const u8 {
        return _.text[0.._.text_len];
    }
}

pub const MAX_CONSOLE_RECORDS: usize = 16;

pub struct JsDomContext {
    doc: *Document,
    layout: *LayoutTree? = null,
    element_class_id: u32 = 0,
    document_class_id: u32 = 0,
    console_records: [MAX_CONSOLE_RECORDS]ConsoleRecord = [ConsoleRecord{}; MAX_CONSOLE_RECORDS],
    console_count: usize = 0,

    fn addConsoleRecord(*_, level: []const u8, text: []const u8) {
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

pub fn createJsElement(ctx: *JSContext, dom_ctx: *JsDomContext, node_id: NodeId): JSValue {
    if (node_id == NULL_NODE) {
        return jsNull();
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
    while (child != NULL_NODE) {
        const cnode = &doc.nodes.items[@as(usize, child)];
        if (cnode.node_type == NodeType.Text && cnode.text_content.len > 0) {
            return cnode.text_content;
        }
        child = cnode.next_sibling;
    }
    return "";
}

fn setNodeText(doc: *Document, node_id: NodeId, text: []const u8) {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) return;
    const node = &doc.nodes.items[@as(usize, node_id)];
    let child = node.first_child;
    while (child != NULL_NODE) {
        if (doc.nodes.items[@as(usize, child)].node_type == NodeType.Text) {
            doc.nodes.items[@as(usize, child)].text_content = text;
            return;
        }
        child = doc.nodes.items[@as(usize, child)].next_sibling;
    }
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
    return jsUndefined();
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
    el.dom_ctx.doc.appendChild(el.node_id, child_el.node_id);
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
    el.dom_ctx.doc.removeChild(child_el.node_id);
    return JS_DupValue(ctx, argv[0]);
}

@c fn js_element_get_bounding_client_rect(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue {
    * = argc;
    * = argv;
    const ptr = JS_GetOpaque(this_val, g_element_class_id);
    if (ptr == null) return jsNull();
    const el: *JsElement = @ptrCast(@alignCast(ptr.?));
    let rect = Rect.zero();
    if (lt := el.dom_ctx.layout) {
        rect = lt.getBoundingClientRect(el.node_id);
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
    return jsInt(@intFromEnum(node.node_type));
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
    setNodeText(el.dom_ctx.doc, el.node_id, new_text_dup);
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
        // Parent of root element (or NULL_NODE parent) is the Document node
        const global_obj = JS_GetGlobalObject(ctx);
        defer JS_FreeValue(ctx, global_obj);
        return JS_GetPropertyStr(ctx, global_obj, "document");
    }
    const parent_node = &doc.nodes.items[@as(usize, parent_id)];
    if (parent_node.node_type == NodeType.Document) {
        const global_obj = JS_GetGlobalObject(ctx);
        defer JS_FreeValue(ctx, global_obj);
        return JS_GetPropertyStr(ctx, global_obj, "document");
    }
    return createJsElement(ctx, el.dom_ctx, parent_id);
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
    const ptr = JS_GetOpaque(val, g_document_class_id);
    if (ptr != null) {
        const ctx: *JsDomContext = @ptrCast(@alignCast(ptr.?));
        std.heap.c_allocator.destroy(ctx);
    }
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

    const opaque_ptr = JS_GetContextOpaque(ctx);
    if (opaque_ptr != null) {
        const dom_ctx: *JsDomContext = @ptrCast(@alignCast(opaque_ptr));
        dom_ctx.addConsoleRecord(level, full_text);
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

pub fn mountDom(ctx: *JsContext, doc: *Document, layout: *LayoutTree?): (*JsDomContext)! {
    const raw_ctx = ctx.raw_ctx;
    const raw_rt = ctx.runtime.raw_rt;

    * = JS_NewClassID(raw_rt, &g_element_class_id);
    * = JS_NewClassID(raw_rt, &g_document_class_id);

    const el_class_def = JSClassDef {
        class_name: "Element",
        finalizer: js_element_finalizer,
        gc_mark: null,
        call_fn: null,
        exotic: null,
    };
    * = JS_NewClass(raw_rt, g_element_class_id, &el_class_def);

    const doc_class_def = JSClassDef {
        class_name: "Document",
        finalizer: js_document_finalizer,
        gc_mark: null,
        call_fn: null,
        exotic: null,
    };
    * = JS_NewClass(raw_rt, g_document_class_id, &doc_class_def);

    // Setup Element Prototype
    const el_proto = JS_NewObject(raw_ctx);
    defineMethod(raw_ctx, el_proto, "getAttribute", js_element_get_attribute, 1);
    defineMethod(raw_ctx, el_proto, "setAttribute", js_element_set_attribute, 2);
    defineMethod(raw_ctx, el_proto, "hasAttribute", js_element_has_attribute, 1);
    defineMethod(raw_ctx, el_proto, "appendChild", js_element_append_child, 1);
    defineMethod(raw_ctx, el_proto, "removeChild", js_element_remove_child, 1);
    defineMethod(raw_ctx, el_proto, "getBoundingClientRect", js_element_get_bounding_client_rect, 0);

    defineGetterSetter(raw_ctx, el_proto, "id", js_element_get_id, js_element_set_id);
    defineGetterSetter(raw_ctx, el_proto, "className", js_element_get_class_name, js_element_set_class_name);
    defineGetterSetter(raw_ctx, el_proto, "tagName", js_element_get_tag_name, null);
    defineGetterSetter(raw_ctx, el_proto, "nodeType", js_element_get_node_type, null);
    defineGetterSetter(raw_ctx, el_proto, "textContent", js_element_get_text_content, js_element_set_text_content);
    defineGetterSetter(raw_ctx, el_proto, "parentElement", js_element_get_parent_element, null);
    defineGetterSetter(raw_ctx, el_proto, "parentNode", js_element_get_parent_node, null);
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
    defineGetterSetter(raw_ctx, doc_proto, "body", js_document_get_body, null);
    defineGetterSetter(raw_ctx, doc_proto, "documentElement", js_document_get_document_element, null);
    defineGetterSetter(raw_ctx, doc_proto, "firstChild", js_element_get_first_child, null);
    defineGetterSetter(raw_ctx, doc_proto, "lastChild", js_element_get_last_child, null);

    JS_SetClassProto(raw_ctx, g_document_class_id, doc_proto);

    // Allocate JsDomContext
    let dom_ctx = std.heap.c_allocator.create(JsDomContext) catch return error.DomContextAllocFailed;
    dom_ctx.* = JsDomContext {
        doc: doc,
        layout: layout,
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

    * = JS_SetPropertyStr(raw_ctx, global_obj, "document", doc_val);
    * = JS_SetPropertyStr(raw_ctx, global_obj, "window", JS_DupValue(raw_ctx, global_obj));
    * = JS_SetPropertyStr(raw_ctx, global_obj, "globalThis", JS_DupValue(raw_ctx, global_obj));

    // Register console
    const console_obj = JS_NewObject(raw_ctx);
    defineMethod(raw_ctx, console_obj, "log", js_console_log, 1);
    defineMethod(raw_ctx, console_obj, "info", js_console_info, 1);
    defineMethod(raw_ctx, console_obj, "warn", js_console_warn, 1);
    defineMethod(raw_ctx, console_obj, "error", js_console_error, 1);
    defineMethod(raw_ctx, console_obj, "debug", js_console_debug, 1);
    * = JS_SetPropertyStr(raw_ctx, global_obj, "console", console_obj);

    // Assemble and evaluate modular runtime shim
    let total_len: usize = 0;
    for (chunk of RUNTIME_CHUNKS) {
        total_len += chunk.len;
    }

    const runtime_buf = std.heap.page_allocator.alloc(u8, total_len) catch return error.RuntimeSourceAllocFailed;
    defer std.heap.page_allocator.free(runtime_buf);

    let offset: usize = 0;
    for (chunk of RUNTIME_CHUNKS) {
        @memcpy(runtime_buf[offset .. offset + chunk.len], chunk);
        offset += chunk.len;
    }

    const shim_val = ctx.eval(runtime_buf);
    if (jsIsException(shim_val)) {
        const exc = ctx.getException();
        const exc_str = ctx.toCString(exc);
        defer std.heap.page_allocator.free(exc_str);
        defer ctx.freeValue(exc);
        std.debug.print("Runtime shim eval error: {s}\n", .{exc_str});
    }
    ctx.freeValue(shim_val);

    return dom_ctx;
}
