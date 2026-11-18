// src/cdp/session.cp
// CapSurf CDP Page Session Controller
// Coordinates HTML parsing, CSS cascade, Layout computation, and JavaScript execution.

import { Doc, Document, NodeId, NULL_NODE, NodeType } from "../dom/dom.cp";
import { Matcher } from "../css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../css/cascade.cp";
import { LayoutTree, Rect } from "../layout/layout.cp";
import { JsRuntime, JsContext, JSValue, jsToSlice, jsIsException, JS_TAG_INT, JS_TAG_FLOAT64, JS_TAG_BOOL, JS_TAG_NULL, JS_TAG_UNDEFINED, JS_TAG_OBJECT, JS_ToInt32, JS_SetPropertyStr } from "../js/qjs_engine.cp";
import { mountDom, JsDomContext } from "../js/dom_binding.cp";
import { CookieJar, CookieEntry } from "../net/cookies.cp";
import { CdpBoxModel, CdpEvent, extractJsonInt } from "./types.cp";
import { FetchState } from "./fetch.cp";
import { ScreenshotOptions, renderDocumentToPng } from "../render/painter.cp";
import { CssColor } from "cap:css";
import { TargetRegistry } from "./target.cp";
import { DedicatedWorkerPool } from "./workers.cp";
import { DebuggerState } from "./debugger.cp";
import { ScreencastHandler } from "./screencast.cp";
import { ResourceStore } from "./resource_store.cp";
import { KitesurfFacade } from "./kitesurf_facade.cp";

pub struct NavigationEntry {
    id: i32 = 0,
    url: []const u8 = "",
    title: []const u8 = "",
}

pub fn escapeJsonString(src: []const u8, dst: []u8): []const u8 {
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
        } else {
            dst[out_idx] = c;
            out_idx += 1;
        }
    }
    return dst[0..out_idx];
}

pub fn canRewriteDocumentUrl(doc_url: []const u8, target_url: []const u8): bool {
    if (target_url.len == 0) return true;
    if (target_url[0] == '#') return true;

    const target_scheme_end = std.mem.indexOf(u8, target_url, "://");
    const doc_scheme_end = std.mem.indexOf(u8, doc_url, "://");

    if (target_scheme_end == null) {
        return true;
    }

    if (doc_scheme_end == null) {
        let doc_base = doc_url;
        if (d_h := std.mem.indexOf(u8, doc_url, "#")) {
            doc_base = doc_url[0..d_h];
        }
        let tgt_base = target_url;
        if (t_h := std.mem.indexOf(u8, target_url, "#")) {
            tgt_base = target_url[0..t_h];
        }
        return std.mem.eql(u8, doc_base, tgt_base);
    }

    const doc_scheme = doc_url[0..doc_scheme_end.?];
    const target_scheme = target_url[0..target_scheme_end.?];
    if (!std.mem.eql(u8, doc_scheme, target_scheme)) return false;

    const doc_auth_start = doc_scheme_end.? + 3;
    let doc_auth = doc_url[doc_auth_start..];
    if (d_slash := std.mem.indexOfPos(u8, doc_url, doc_auth_start, "/")) {
        doc_auth = doc_url[doc_auth_start..d_slash];
    }

    const tgt_auth_start = target_scheme_end.? + 3;
    let tgt_auth = target_url[tgt_auth_start..];
    if (t_slash := std.mem.indexOfPos(u8, target_url, tgt_auth_start, "/")) {
        tgt_auth = target_url[tgt_auth_start..t_slash];
    }

    if (!std.mem.eql(u8, doc_auth, tgt_auth)) return false;

    if (std.mem.eql(u8, target_scheme, "http") || std.mem.eql(u8, target_scheme, "https")) {
        return true;
    }

    let doc_base = doc_url;
    if (d_h := std.mem.indexOf(u8, doc_url, "#")) {
        doc_base = doc_url[0..d_h];
    }
    let tgt_base = target_url;
    if (t_h := std.mem.indexOf(u8, target_url, "#")) {
        tgt_base = target_url[0..t_h];
    }
    return std.mem.eql(u8, doc_base, tgt_base);
}

pub const DEFAULT_HTTP_USER_AGENT: []const u8 = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/145.0.0.0 Safari/537.36 CapSurf/1.0.0";

pub struct CdpPageSession {
    allocator: std.mem.Allocator,
    doc: Document,
    layout: LayoutTree,
    rt: JsRuntime,
    ctx: JsContext,
    dom_ctx: (*JsDomContext)? = null,
    url: []const u8 = "about:blank",
    referrer: []const u8 = "",
    title: []const u8 = "Blank Page",
    frame_id: []const u8 = "capsurf-frame-001",
    loader_id: []const u8 = "capsurf-loader-001",
    target_id: []const u8 = "capsurf-target-001",
    has_document: bool = false,
    raw_html: []const u8 = "",

    // Emulation & Viewport overrides
    viewport_width: f32 = 1280.0,
    viewport_height: f32 = 720.0,
    device_scale_factor: f32 = 1.0,
    is_mobile: bool = false,
    user_agent: []const u8 = DEFAULT_HTTP_USER_AGENT,
    js_user_agent: []const u8 = "",
    user_agent_buf: [512]u8 = [0; 512],

    // Network & Fetch state
    extra_headers: []const u8 = "{}",
    cookie_jar: CookieJar,
    fetch_enabled: bool = false,
    fetch_state: FetchState = FetchState{},
    target_registry: TargetRegistry = TargetRegistry.init(),

    // Runtime & Log domain flags
    runtime_enabled: bool = false,
    log_enabled: bool = false,
    dedicated_workers: DedicatedWorkerPool = DedicatedWorkerPool.init(),

    debugger_state: DebuggerState = DebuggerState{},
    screencast_handler: ScreencastHandler = ScreencastHandler.init(),
    resource_store: ResourceStore = ResourceStore.init(),
    facade: KitesurfFacade = KitesurfFacade{},

    // Dedicated buffers for event formatting
    console_event_buf: [1024]u8 = [0; 1024],
    log_event_buf: [1024]u8 = [0; 1024],
    rt_ctx_event_buf: [512]u8 = [0; 512],
    page_frame_event_buf: [2048]u8 = [0; 2048],
    binding_event_buf: [1024]u8 = [0; 1024],
    same_doc_event_buf: [2048]u8 = [0; 2048],

    // Navigation History state
    nav_entries: [16]NavigationEntry = [NavigationEntry{}; 16],
    nav_entry_count: usize = 0,
    current_nav_index: usize = 0,
    next_nav_id: i32 = 1,
    pending_same_doc_urls: [4][]const u8 = [""; 4],
    pending_same_doc_count: usize = 0,

    // Focus & Scroll state
    focused_node: NodeId = NULL_NODE,
    scroll_x: f32 = 0.0,
    scroll_y: f32 = 0.0,
    next_object_id: u32 = 1,

    pub static fn init(allocator: std.mem.Allocator): (*CdpPageSession)! {
        let ptr = std.heap.c_allocator.create(CdpPageSession) catch return error.AllocFailed;
        const document = Document.init(allocator) catch return error.DocInitFailed;
        const layout_tree = LayoutTree.init(allocator, 1280.0, 720.0) catch return error.LayoutInitFailed;
        const js_rt = JsRuntime.init(64 * 1024 * 1024, 1000000) catch return error.JsRuntimeFailed;
        const jar = CookieJar.init(allocator);

        ptr.* = CdpPageSession {
            allocator: allocator,
            doc: document,
            layout: layout_tree,
            rt: js_rt,
            ctx: JsContext { raw_ctx: undefined, runtime: undefined },
            dom_ctx: null,
            url: "about:blank",
            title: "Blank Page",
            frame_id: "capsurf-frame-001",
            loader_id: "capsurf-loader-001",
            target_id: "capsurf-target-001",
            has_document: false,
            raw_html: "",
            viewport_width: 1280.0,
            viewport_height: 720.0,
            device_scale_factor: 1.0,
            is_mobile: false,
            user_agent: DEFAULT_HTTP_USER_AGENT,
            js_user_agent: "",
            user_agent_buf: [0; 512],
            extra_headers: "{}",
            cookie_jar: jar,
            fetch_enabled: false,
            fetch_state: FetchState{},
            target_registry: TargetRegistry.init(),
            runtime_enabled: false,
            log_enabled: false,
            dedicated_workers: DedicatedWorkerPool.init(),
            debugger_state: DebuggerState{},
            screencast_handler: ScreencastHandler.init(),
            resource_store: ResourceStore.init(),
            console_event_buf: [0; 1024],
            log_event_buf: [0; 1024],
            rt_ctx_event_buf: [0; 512],
            page_frame_event_buf: [0; 2048],
            binding_event_buf: [0; 1024],
            same_doc_event_buf: [0; 2048],
            nav_entries: [NavigationEntry{}; 16],
            nav_entry_count: 0,
            current_nav_index: 0,
            next_nav_id: 1,
            pending_same_doc_urls: [""; 4],
            pending_same_doc_count: 0,
            next_object_id: 1,
            focused_node: NULL_NODE,
            scroll_x: 0.0,
            scroll_y: 0.0,
        };
        ptr.ctx = ptr.rt.createContext() catch return error.JsContextFailed;
        ptr.navigate("<!DOCTYPE html><html><head><title>Blank Page</title></head><body></body></html>", "about:blank", "") catch {};
        return ptr;
    }

    pub fn deinit(*_) {
        const nav_count = @min(_.nav_entry_count, _.nav_entries.len);
        let ni: usize = 0;
        while (ni < nav_count) : (ni += 1) {
            if (_.nav_entries[ni].url.len > 0) {
                _.allocator.free(_.nav_entries[ni].url);
                _.nav_entries[ni].url = "";
            }
            if (_.nav_entries[ni].title.len > 0) {
                _.allocator.free(_.nav_entries[ni].title);
                _.nav_entries[ni].title = "";
            }
        }
        _.nav_entry_count = 0;

        const p_count = @min(_.pending_same_doc_count, _.pending_same_doc_urls.len);
        let pi: usize = 0;
        while (pi < p_count) : (pi += 1) {
            if (_.pending_same_doc_urls[pi].len > 0) {
                _.allocator.free(_.pending_same_doc_urls[pi]);
                _.pending_same_doc_urls[pi] = "";
            }
        }
        _.pending_same_doc_count = 0;

        if (_.raw_html.len > 0) {
            _.allocator.free(_.raw_html);
            _.raw_html = "";
        }
        if (_.dom_ctx != null) {
            std.heap.c_allocator.destroy(_.dom_ctx.?);
            _.dom_ctx = null;
        }
        _.cookie_jar.deinit();
        _.ctx.deinit();
        _.rt.deinit();
        _.layout.deinit();
        _.doc.deinit();
        std.heap.c_allocator.destroy(_);
    }

    pub fn setUserAgentOverride(*_, ua: []const u8) {
        if (ua.len == 0) {
            _.user_agent = DEFAULT_HTTP_USER_AGENT;
            _.js_user_agent = "";
        } else {
            const copy_len = @min(ua.len, _.user_agent_buf.len);
            @memcpy(_.user_agent_buf[0..copy_len], ua[0..copy_len]);
            _.user_agent = _.user_agent_buf[0..copy_len];
            _.js_user_agent = _.user_agent;
        }
        _.syncUserAgentToJs();
    }

    pub fn syncUserAgentToJs(*_) {
        let ua_buf: [2048]u8 = undefined;
        let json_buf: [1024]u8 = undefined;
        const esc_ua = escapeJsonString(_.js_user_agent, json_buf[0..]);
        const script = std.fmt.bufPrint(
            &ua_buf,
            "(function(){{if(typeof __capsurf_set_user_agent==='function')__capsurf_set_user_agent(\"{s}\");}})()",
            .{esc_ua},
        ) catch return;
        const v = _.ctx.eval(script);
        _.ctx.freeValue(v);
    }

    pub fn navigate(*_, html: []const u8, nav_url: []const u8, nav_referrer: []const u8)! {
        _.title = "";
        _.doc.deinit();
        if (_.raw_html.len > 0) {
            _.allocator.free(_.raw_html);
            _.raw_html = "";
        }
        _.raw_html = _.allocator.dupe(u8, html) catch return error.AllocFailed;
        _.doc = Document.init(_.allocator) catch return error.DocReinitFailed;
        _.doc.parseHtml(_.raw_html) catch return error.HtmlParseFailed;

        // Extract Title
        const title_node = Matcher.querySelector(&_.doc, "title");
        if (title_node != NULL_NODE) {
            const first_text = _.doc.getFirstChild(title_node);
            if (first_text != NULL_NODE) {
                _.title = _.doc.getTextContent(first_text);
            }
        }

        // Layout & Styles: collect embedded <style> tags
        let css_buf = _.allocator.alloc(u8, 65536) catch null;
        defer if (css_buf != null) _.allocator.free(css_buf.?);
        let css_len: usize = 0;
        if (css_buf != null) {
            const base_css = "* { box-sizing: border-box; }\n";
            @memcpy(css_buf.?[0..base_css.len], base_css);
            css_len = base_css.len;
            let snid: usize = 0;
            while (snid < _.doc.nodes.items.len) : (snid += 1) {
                const sn = &_.doc.nodes.items[snid];
                if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style")) {
                    let child = sn.first_child;
                    while (child != NULL_NODE && @as(usize, child) < _.doc.nodes.items.len) : (child = _.doc.nodes.items[@as(usize, child)].next_sibling) {
                        const scn = &_.doc.nodes.items[@as(usize, child)];
                        if (scn.node_type == NodeType.Text && scn.text_content.len > 0) {
                            const copy_len = @min(scn.text_content.len, css_buf.?.len - css_len);
                            @memcpy(css_buf.?[css_len..css_len + copy_len], scn.text_content[0..copy_len]);
                            css_len += copy_len;
                        }
                    }
                }
            }
        }
        const full_css = if (css_buf != null) css_buf.?[0..css_len] else "* { box-sizing: border-box; }";
        const sheet = StyleSheet.parse(full_css, 1280.0, false);
        const count = @max(@as(usize, 1), _.doc.nodeCount());
        let styles = _.allocator.alloc(ComputedStyle, count) catch null;
        defer if (styles != null) _.allocator.free(styles.?);
        if (styles != null) {
            Cascade.computeStyles(&_.doc, &sheet, styles.?, 16.0);
            _.layout.computeLayout(&_.doc, styles.?) catch {};
        }

        // Mount to JS Context
        if (_.dom_ctx != null) {
            std.heap.c_allocator.destroy(_.dom_ctx.?);
            _.dom_ctx = null;
        }
        if (_.has_document) {
            _.ctx.deinit();
            _.ctx = _.rt.createContext() catch return error.JsContextFailed;
        }
        const m = mountDom(&_.ctx, &_.doc, &_.layout) catch return error.MountDomFailed;
        _.dom_ctx = m;
        _.url = nav_url;
        _.referrer = nav_referrer;
        _.has_document = true;
        _.focused_node = NULL_NODE;
        _.scroll_x = 0.0;
        _.scroll_y = 0.0;

        // Set document url and referrer in JS environment
        let set_url_buf: [1024]u8 = undefined;
        let set_url_json_buf: [512]u8 = undefined;
        const escaped_nav_url = escapeJsonString(nav_url, set_url_json_buf[0..]);
        const set_url_script = std.fmt.bufPrint(
            &set_url_buf,
            "(function(){{if(typeof __capsurf_set_document_url==='function')__capsurf_set_document_url(\"{s}\");}})()",
            .{escaped_nav_url},
        ) catch "";
        if (set_url_script.len > 0) {
            const v = _.ctx.eval(set_url_script);
            _.ctx.freeValue(v);
        }

        let set_ref_buf: [1024]u8 = undefined;
        let set_ref_json_buf: [512]u8 = undefined;
        const escaped_ref = escapeJsonString(nav_referrer, set_ref_json_buf[0..]);
        const set_ref_script = std.fmt.bufPrint(
            &set_ref_buf,
            "(function(){{if(typeof __capsurf_set_document_referrer==='function')__capsurf_set_document_referrer(\"{s}\");}})()",
            .{escaped_ref},
        ) catch "";
        if (set_ref_script.len > 0) {
            const vr = _.ctx.eval(set_ref_script);
            _.ctx.freeValue(vr);
        }

        // Sync User-Agent override to JS environment
        _.syncUserAgentToJs();

        // Execute inline <script> tags in document order
        const initial_node_count = _.doc.nodes.items.len;
        let nid: usize = 1;
        while (nid < initial_node_count) : (nid += 1) {
            const n = &_.doc.nodes.items[nid];
            if (n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, "script")) {
                const has_nomodule = n.hasAttribute("nomodule");
                const type_attr = n.getAttribute("type") ?? "";
                const is_module = std.ascii.eqlIgnoreCase(type_attr, "module");
                if (has_nomodule && !is_module) {
                    continue;
                }
                let code_buf = _.allocator.alloc(u8, 65536) catch break;
                defer _.allocator.free(code_buf);
                let code_len: usize = 0;
                let child = n.first_child;
                while (child != NULL_NODE && @as(usize, child) < _.doc.nodes.items.len) : (child = _.doc.nodes.items[@as(usize, child)].next_sibling) {
                    const cn = &_.doc.nodes.items[@as(usize, child)];
                    if (cn.node_type == NodeType.Text && cn.text_content.len > 0) {
                        const copy_len = @min(cn.text_content.len, code_buf.len - code_len);
                        @memcpy(code_buf[code_len..code_len + copy_len], cn.text_content[0..copy_len]);
                        code_len += copy_len;
                    }
                }
                if (code_len > 0) {
                    const js_res = _.ctx.eval(code_buf[0..code_len]);
                    if (jsIsException(js_res)) {
                        const exc = _.ctx.getException();
                        let exc_str_buf: [256]u8 = undefined;
                        const exc_len = _.formatCdpException(exc, &exc_str_buf);
                        std.debug.print("[INLINE_SCRIPT_ERR] {s}\n", .{exc_str_buf[0..exc_len]});
                        _.ctx.freeValue(exc);
                    }
                    _.ctx.freeValue(js_res);
                }
            }
        }

        // Dispatch HTML5 lifecycle events (DOMContentLoaded on document, load on window)
        const lc_res = _.ctx.eval("(function(){if(typeof __capsurf_dispatch_lifecycle==='function')__capsurf_dispatch_lifecycle();})()");
        _.ctx.freeValue(lc_res);

        // Record navigation in history
        _.recordNavigation(nav_url, _.title);
        if (_.nav_entry_count > 0 && _.current_nav_index < _.nav_entry_count) {
            _.url = _.nav_entries[_.current_nav_index].url;
        }
    }

    fn formatCdpException(*_, exc: JSValue, out_buf: []u8): usize {
        const exc_str = _.ctx.toCString(exc);
        defer std.heap.page_allocator.free(exc_str);

        // Extract className: e.g. "TypeError", "RangeError", "SyntaxError", "Error"
        let class_name: []const u8 = "Error";
        if (colon_idx := std.mem.indexOf(u8, exc_str, ":")) {
            const prefix = exc_str[0..colon_idx];
            if (prefix.len > 0 && std.mem.indexOf(u8, prefix, " ") == null) {
                class_name = prefix;
            }
        }

        // Format description with trailing newline according to CDP error specs
        let desc_buf: [2048]u8 = [0; 2048];
        let desc_slice: []const u8 = "";
        if (exc_str.len > 0 && exc_str[exc_str.len - 1] == '\n') {
            desc_slice = exc_str;
        } else {
            desc_slice = std.fmt.bufPrint(&desc_buf, "{s}\n", .{exc_str}) catch exc_str;
        }

        let esc_exc_buf: [2048]u8 = [0; 2048];
        const esc_exc = escapeJsonString(desc_slice, &esc_exc_buf);

        const err_json = std.fmt.bufPrint(
            out_buf,
            "{{\"result\":{{\"type\":\"undefined\"}},\"exceptionDetails\":{{\"text\":\"Uncaught\",\"exception\":{{\"type\":\"object\",\"subtype\":\"error\",\"className\":\"{s}\",\"description\":\"{s}\"}}}}}}",
            .{ class_name, esc_exc },
        ) catch return 0;
        return err_json.len;
    }

    fn formatCdpResult(*_, val: JSValue, out_buf: []u8): usize {
        if (val.tag == JS_TAG_INT) {
            const n: i32 = @as(i32, @bitCast(@as(u32, @truncate(val.u.uint64))));
            const formatted = std.fmt.bufPrint(
                out_buf,
                "{{\"result\":{{\"type\":\"number\",\"value\":{d}}}}}",
                .{n},
            ) catch return 0;
            return formatted.len;
        }

        if (val.tag == JS_TAG_FLOAT64) {
            const f: f64 = @as(f64, @bitCast(val.u.uint64));
            const formatted = std.fmt.bufPrint(
                out_buf,
                "{{\"result\":{{\"type\":\"number\",\"value\":{d}}}}}",
                .{f},
            ) catch return 0;
            return formatted.len;
        }

        if (val.tag == JS_TAG_BOOL) {
            const b = val.u.uint64 != 0;
            const formatted = std.fmt.bufPrint(
                out_buf,
                "{{\"result\":{{\"type\":\"boolean\",\"value\":{s}}}}}",
                .{if (b) "true" else "false"},
            ) catch return 0;
            return formatted.len;
        }

        if (val.tag == JS_TAG_NULL) {
            const formatted = std.fmt.bufPrint(
                out_buf,
                "{{\"result\":{{\"type\":\"object\",\"value\":null}}}}",
                .{},
            ) catch return 0;
            return formatted.len;
        }

        if (val.tag == JS_TAG_UNDEFINED) {
            const formatted = std.fmt.bufPrint(
                out_buf,
                "{{\"result\":{{\"type\":\"undefined\"}}}}",
                .{},
            ) catch return 0;
            return formatted.len;
        }

        if (val.tag == JS_TAG_OBJECT) {
            const json_str = _.ctx.jsonStringify(val);
            defer std.heap.page_allocator.free(json_str);
            if (json_str.len > 0 && (json_str[0] == '{' || json_str[0] == '[')) {
                const formatted = std.fmt.bufPrint(
                    out_buf,
                    "{{\"result\":{{\"type\":\"object\",\"value\":{s}}}}}",
                    .{json_str},
                ) catch return 0;
                return formatted.len;
            }
        }

        const res_str = _.ctx.toCString(val);
        defer std.heap.page_allocator.free(res_str);

        let esc_buf: [4096]u8 = [0; 4096];
        const esc_str = escapeJsonString(res_str, &esc_buf);

        const formatted = std.fmt.bufPrint(
            out_buf,
            "{{\"result\":{{\"type\":\"string\",\"value\":\"{s}\"}}}}",
            .{esc_str},
        ) catch return 0;
        return formatted.len;
    }

    pub fn formatCdpRemoteObject(*_, val: JSValue, return_by_value: bool, out_buf: []u8): usize {
        _.ctx.setPropertyStr("__capsurf_temp_val", val);
        const script = if (return_by_value)
            "globalThis.__capsurf_format_remote_object(globalThis.__capsurf_temp_val, true)"
        else
            "globalThis.__capsurf_format_remote_object(globalThis.__capsurf_temp_val, false)";
        const res_val = _.ctx.eval(script);
        defer _.ctx.freeValue(res_val);

        if (jsIsException(res_val)) {
             return _.formatCdpException(_.ctx.getException(), out_buf);
        }

        const json_str = _.ctx.jsonStringify(res_val);
        defer std.heap.page_allocator.free(json_str);
        
        const formatted = std.fmt.bufPrint(out_buf, "{{\"result\":{s}}}", .{json_str}) catch return 0;
        return formatted.len;
    }

    pub fn callFunctionOn(*_, objectId: []const u8, functionDeclaration: []const u8, return_by_value: bool, await_promise: bool, out_buf: []u8): usize {
        const id_val = _.ctx.newString(objectId);
        defer _.ctx.freeValue(id_val);
        _.ctx.setPropertyStr("__capsurf_call_id", id_val);

        const decl_val = _.ctx.newString(functionDeclaration);
        defer _.ctx.freeValue(decl_val);
        _.ctx.setPropertyStr("__capsurf_call_decl", decl_val);

        const val = _.ctx.eval("globalThis.__capsurf_call_fn(globalThis.__capsurf_call_id, globalThis.__capsurf_call_decl)");
        defer _.ctx.freeValue(val);

        if (await_promise && _.ctx.isPromise(val)) {
            let limit: u32 = 100000;
            while (_.ctx.promiseState(val) == 0 && limit > 0) : (limit -= 1) {
                const r = _.ctx.executePendingJobs();
                if (r <= 0) break;
            }

            if (_.ctx.promiseState(val) == 1) {
                const res_val = _.ctx.promiseResult(val);
                defer _.ctx.freeValue(res_val);
                return _.formatCdpRemoteObject(res_val, return_by_value, out_buf);
            } else if (_.ctx.promiseState(val) == 2) {
                const rej_val = _.ctx.promiseResult(val);
                defer _.ctx.freeValue(rej_val);
                return _.formatCdpException(rej_val, out_buf);
            }
        }

        _.ctx.drainJobs();

        if (jsIsException(val)) {
            const exc = _.ctx.getException();
            defer _.ctx.freeValue(exc);
            return _.formatCdpException(exc, out_buf);
        }

        return _.formatCdpRemoteObject(val, return_by_value, out_buf);
    }

    pub fn evaluate(*_, expr: []const u8, out_buf: []u8): usize {
        return _.evaluateWithOptions(expr, false, true, out_buf);
    }

    pub fn evaluateWithOptions(*_, expr: []const u8, return_by_value: bool, await_promise: bool, out_buf: []u8): usize {
        const val = _.ctx.eval(expr);
        defer _.ctx.freeValue(val);

        if (await_promise && _.ctx.isPromise(val)) {
            let limit: u32 = 100000;
            while (_.ctx.promiseState(val) == 0 && limit > 0) : (limit -= 1) {
                const r = _.ctx.executePendingJobs();
                if (r <= 0) break;
            }

            if (_.ctx.promiseState(val) == 1) {
                const res_val = _.ctx.promiseResult(val);
                defer _.ctx.freeValue(res_val);
                return _.formatCdpRemoteObject(res_val, return_by_value, out_buf);
            } else if (_.ctx.promiseState(val) == 2) {
                const rej_val = _.ctx.promiseResult(val);
                defer _.ctx.freeValue(rej_val);
                return _.formatCdpException(rej_val, out_buf);
            }
        }

        _.ctx.drainJobs();

        if (jsIsException(val)) {
            const exc = _.ctx.getException();
            defer _.ctx.freeValue(exc);
            return _.formatCdpException(exc, out_buf);
        }

        return _.formatCdpRemoteObject(val, return_by_value, out_buf);
    }

    pub fn querySelector(*_, selector: []const u8): NodeId {
        return Matcher.querySelector(&_.doc, selector);
    }

    pub fn querySelectorAll(*_, selector: []const u8, out_ids: []NodeId): usize {
        return Matcher.querySelectorAll(&_.doc, selector, out_ids);
    }

    pub fn getBoxModel(*_, node_id: NodeId): CdpBoxModel {
        let rect = _.layout.getBoundingClientRect(node_id);
        if (@as(usize, node_id) < _.doc.nodes.items.len) {
            const node = &_.doc.nodes.items[@as(usize, node_id)];
            if (node.node_type == .Element && !rect.visible) {
                return CdpBoxModel {
                    width: 0.0,
                    height: 0.0,
                    border: .{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
                    content: .{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
                };
            }
            if (node.node_type == .Text) {
                const parent_id = node.parent;
                if (parent_id != NULL_NODE && @as(usize, parent_id) < _.layout.boxes.items.len) {
                    const p_rect = _.layout.getBoundingClientRect(parent_id);
                    if (!p_rect.visible) {
                        return CdpBoxModel {
                            width: 0.0,
                            height: 0.0,
                            border: .{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
                            content: .{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 },
                        };
                    }
                    let start_x = p_rect.x;
                    let start_y = p_rect.y;
                    const prev_id = node.prev_sibling;
                    if (prev_id != NULL_NODE && @as(usize, prev_id) < _.layout.boxes.items.len) {
                        const prev_rect = _.layout.getBoundingClientRect(prev_id);
                        if (prev_rect.visible && prev_rect.width > 0.0) {
                            start_x = prev_rect.right();
                            start_y = prev_rect.y;
                        }
                    }
                    const text_str = node.text_content;
                    const char_w: f32 = 8.0;
                    const w: f32 = @as(f32, @floatFromInt(@max(@as(usize, 1), text_str.len))) * char_w;
                    const h: f32 = if (p_rect.height > 0.0) p_rect.height else 20.0;
                    rect = Rect {
                        x: start_x,
                        y: start_y,
                        width: w,
                        height: h,
                        visible: true,
                    };
                }
            } else if (node.node_type == .Element && rect.visible) {
                let text_len: usize = 0;
                let ch = node.first_child;
                while (ch != NULL_NODE && @as(usize, ch) < _.doc.nodes.items.len) {
                    const cn = &_.doc.nodes.items[@as(usize, ch)];
                    if (cn.node_type == .Text) {
                        text_len += cn.text_content.len;
                    }
                    ch = cn.next_sibling;
                }
                if (text_len > 0) {
                    if (rect.width == 0.0) {
                        rect.width = @as(f32, @floatFromInt(text_len)) * 8.0;
                    }
                    if (rect.height == 0.0) {
                        rect.height = 16.0;
                    }
                }
            }
        }
        const l = rect.x;
        const t = rect.y;
        const r = rect.right();
        const b = rect.bottom();

        return CdpBoxModel {
            width: rect.width,
            height: rect.height,
            border: .{ l, t, r, t, r, b, l, b },
            content: .{ l, t, r, t, r, b, l, b },
        };
    }

    pub fn captureScreenshot(*_, compress_level: u8): ([]const u8)! {
        const count = @max(@as(usize, 1), _.doc.nodeCount());
        const sheet = StyleSheet{};
        let styles = _.allocator.alloc(ComputedStyle, count)?;
        defer _.allocator.free(styles);
        Cascade.computeStyles(&_.doc, &sheet, styles, 16.0);

        _.layout.computeLayout(&_.doc, styles) catch {};

        let rects = _.allocator.alloc(Rect, count)?;
        defer _.allocator.free(rects);
        let nid: usize = 0;
        while (nid < count) : (nid += 1) {
            rects[nid] = _.layout.getBoundingClientRect(@as(u32, @intCast(nid)));
        }

        const vw: u32 = if (_.viewport_width > 0.0) @as(u32, @intFromFloat(_.viewport_width)) else 1280;
        const vh: u32 = if (_.viewport_height > 0.0) @as(u32, @intFromFloat(_.viewport_height)) else 720;
        
        // Get focused node info from JS
        let focused_node: NodeId = NULL_NODE;
        let caret_pos: usize = 0;
        if (_.has_document) {
            const js_expr = "(function() { const el = document.activeElement; if (!el || el === document.body || el === document.documentElement) return ''; const nid = (typeof el._nid === 'number') ? el._nid : 0; if (nid <= 0) return ''; const start = (typeof el.selectionStart === 'number') ? el.selectionStart : 0; return nid + ':' + start; })();";
            const val = _.ctx.eval(js_expr);
            const cstr = _.ctx.toCString(val);
            if (cstr.len > 0) {
                let parts = std.mem.splitAny(u8, cstr, ":");
                if (nid_str := parts.next()) {
                    focused_node = @as(NodeId, std.fmt.parseInt(u32, nid_str, 10) catch 0);
                }
                if (start_str := parts.next()) {
                    caret_pos = std.fmt.parseInt(usize, start_str, 10) catch 0;
                }
            }
            _.ctx.freeValue(val);
        }

        const opts = ScreenshotOptions{
            viewport_w: vw,
            viewport_h: vh,
            compress_level: if (compress_level <= 9) compress_level else 6,
            clear_color: CssColor.rgb(255, 255, 255),
            focused_node: focused_node,
            caret_pos: caret_pos,
        };

        return renderDocumentToPng(_.allocator, &_.doc, styles, rects, opts);
    }

    fn clearConsoleLogs(*_) {
        if (dom := _.dom_ctx) {
            dom.clearConsoleRecords();
        }
    }

    fn pollConsoleEvents(*_, out_events: []CdpEvent): usize {
        if (dom := _.dom_ctx) {
            if (dom.console_count == 0) return 0;

            let ev_idx: usize = 0;
            let i: usize = 0;
            while (i < dom.console_count && ev_idx < out_events.len) : (i += 1) {
                let rec = &dom.console_records[i];
                const lvl = rec.getLevel();
                const txt = rec.getText();

                if (_.runtime_enabled && ev_idx < out_events.len) {
                    const args_slice = rec.getArgsJson();
                    const args_json = if (args_slice.len > 0) args_slice else "[{\"type\":\"string\",\"value\":\"\"}]";
                    const rt_json = std.fmt.bufPrint(
                        &_.console_event_buf,
                        "{{\"type\":\"{s}\",\"args\":{s},\"executionContextId\":1,\"timestamp\":{d}}}",
                        .{ lvl, args_json, rec.timestamp },
                    ) catch "";
                    if (rt_json.len > 0) {
                        out_events[ev_idx] = CdpEvent.create("Runtime.consoleAPICalled", rt_json);
                        ev_idx += 1;
                    }
                }

                if (_.log_enabled && ev_idx < out_events.len) {
                    const log_lvl = if (std.mem.eql(u8, lvl, "error"))
                        "error"
                    else if (std.mem.eql(u8, lvl, "warn"))
                        "warning"
                    else
                        "info";

                    const log_json = std.fmt.bufPrint(
                        &_.log_event_buf,
                        "{{\"entry\":{{\"source\":\"javascript\",\"level\":\"{s}\",\"text\":\"{s}\",\"timestamp\":{d}}}}}",
                        .{ log_lvl, txt, rec.timestamp },
                    ) catch "";
                    if (log_json.len > 0) {
                        out_events[ev_idx] = CdpEvent.create("Log.entryAdded", log_json);
                        ev_idx += 1;
                    }
                }
            }
            dom.clearConsoleRecords();
            return ev_idx;
        }
        return 0;
    }

    pub fn addBinding(*_, name: []const u8) {
        let script_buf: [512]u8 = [0; 512];
        const script = std.fmt.bufPrint(
            &script_buf,
            "(function(){{const n=\"{s}\";const fn=function(p){{if(typeof __capsurf_call_binding==='function')__capsurf_call_binding(n,p);}};globalThis[n]=fn;if(typeof window!=='undefined')window[n]=fn;}})()",
            .{name},
        ) catch return;
        const v = _.ctx.eval(script);
        _.ctx.freeValue(v);
    }

    pub fn pollBindingEvents(*_, out_events: []CdpEvent): usize {
        const drain_expr = "(function(){if(typeof __capsurf_drain_bindings==='function')return __capsurf_drain_bindings();return '[]';})()";
        const val = _.ctx.eval(drain_expr);
        defer _.ctx.freeValue(val);

        const json_str = _.ctx.toCString(val);
        defer std.heap.page_allocator.free(json_str);
        if (json_str.len > 2) {
            std.debug.print("drain_expr json_str: {s}\n", .{json_str});
        }

        if (json_str.len <= 2) return 0;

        let ev_idx: usize = 0;
        let pos: usize = 0;
        while (ev_idx < out_events.len) {
            const name_marker = "\"name\":\"";
            const name_start_opt = std.mem.indexOfPos(u8, json_str, pos, name_marker);
            if (name_start_opt == null) break;
            const name_start = name_start_opt.? + name_marker.len;
            const name_end_opt = std.mem.indexOfPos(u8, json_str, name_start, "\"");
            if (name_end_opt == null) break;
            const name = json_str[name_start..name_end_opt.?];

            const payload_marker = "\"payload\":\"";
            const payload_start_opt = std.mem.indexOfPos(u8, json_str, name_end_opt.?, payload_marker);
            if (payload_start_opt == null) break;
            const payload_start = payload_start_opt.? + payload_marker.len;
            const payload_end_opt = std.mem.indexOfPos(u8, json_str, payload_start, "\"");
            if (payload_end_opt == null) break;
            const payload = json_str[payload_start..payload_end_opt.?];

            pos = payload_end_opt.? + 1;

            const rt_json = std.fmt.bufPrint(
                &_.binding_event_buf,
                "{{\"name\":\"{s}\",\"payload\":\"{s}\",\"executionContextId\":1}}",
                .{ name, payload },
            ) catch break;

            out_events[ev_idx] = CdpEvent.create("Runtime.bindingCalled", rt_json);
            ev_idx += 1;
        }

        return ev_idx;
    }

    fn recordNavigation(*_, nav_url: []const u8, nav_title: []const u8) {
        // If current entry has the same URL, update its title (e.g. reload)
        if (_.nav_entry_count > 0 && _.current_nav_index < _.nav_entry_count) {
            if (std.mem.eql(u8, _.nav_entries[_.current_nav_index].url, nav_url)) {
                if (nav_title.len > 0) {
                    if (_.nav_entries[_.current_nav_index].title.len > 0) {
                        _.allocator.free(_.nav_entries[_.current_nav_index].title);
                    }
                    _.nav_entries[_.current_nav_index].title = _.allocator.dupe(u8, nav_title) catch "";
                }
                return;
            }
        }

        // Truncate any forward entries if we navigated after back
        if (_.current_nav_index + 1 < _.nav_entry_count) {
            let tr: usize = _.current_nav_index + 1;
            while (tr < _.nav_entry_count) : (tr += 1) {
                if (_.nav_entries[tr].url.len > 0) {
                    _.allocator.free(_.nav_entries[tr].url);
                    _.nav_entries[tr].url = "";
                }
                if (_.nav_entries[tr].title.len > 0) {
                    _.allocator.free(_.nav_entries[tr].title);
                    _.nav_entries[tr].title = "";
                }
            }
            _.nav_entry_count = _.current_nav_index + 1;
        }

        const dup_url = _.allocator.dupe(u8, nav_url) catch "";
        let dup_title: []const u8 = "";
        if (nav_title.len > 0) {
            dup_title = _.allocator.dupe(u8, nav_title) catch "";
        }

        const entry_id = _.next_nav_id;
        _.next_nav_id += 1;

        if (_.nav_entry_count < 16) {
            const idx = _.nav_entry_count;
            _.nav_entries[idx] = NavigationEntry{
                id: entry_id,
                url: dup_url,
                title: dup_title,
            };
            _.current_nav_index = idx;
            _.nav_entry_count += 1;
        } else {
            // Drop oldest entry (slot 0)
            if (_.nav_entries[0].url.len > 0) {
                _.allocator.free(_.nav_entries[0].url);
            }
            if (_.nav_entries[0].title.len > 0) {
                _.allocator.free(_.nav_entries[0].title);
            }

            let s: usize = 0;
            while (s < 15) : (s += 1) {
                _.nav_entries[s] = _.nav_entries[s + 1];
            }

            _.nav_entries[15] = NavigationEntry{
                id: entry_id,
                url: dup_url,
                title: dup_title,
            };
            _.current_nav_index = 15;
        }
    }

    pub fn updateCurrentNavTitle(*_, new_title: []const u8) {
        if (_.nav_entry_count > 0 && _.current_nav_index < _.nav_entry_count) {
            if (!std.mem.eql(u8, _.nav_entries[_.current_nav_index].title, new_title)) {
                if (_.nav_entries[_.current_nav_index].title.len > 0) {
                    _.allocator.free(_.nav_entries[_.current_nav_index].title);
                }
                _.nav_entries[_.current_nav_index].title = _.allocator.dupe(u8, new_title) catch "";
            }
        }
    }

    pub fn navigateToHistoryEntry(*_, entry_id: i32): bool {
        let i: usize = 0;
        while (i < _.nav_entry_count) : (i += 1) {
            if (_.nav_entries[i].id == entry_id) {
                _.current_nav_index = i;
                _.url = _.nav_entries[i].url;
                return true;
            }
        }
        return false;
    }

    pub fn pushNavigatedWithinDocEvent(*_, url: []const u8) {
        _.recordSameDocNavigation(url, true);
        if (_.pending_same_doc_count < _.pending_same_doc_urls.len) {
            _.pending_same_doc_urls[_.pending_same_doc_count] = _.allocator.dupe(u8, url) catch url;
            _.pending_same_doc_count += 1;
        }
    }

    pub fn recordSameDocNavigation(*_, url: []const u8, is_push: bool) {
        if (!canRewriteDocumentUrl(_.url, url)) return;
        if (is_push) {
            _.recordNavigation(url, _.title);
            if (_.nav_entry_count > 0 && _.current_nav_index < _.nav_entry_count) {
                _.url = _.nav_entries[_.current_nav_index].url;
            }
        } else {
            if (_.nav_entry_count > 0 && _.current_nav_index < _.nav_entry_count) {
                if (_.nav_entries[_.current_nav_index].url.len > 0) {
                    _.allocator.free(_.nav_entries[_.current_nav_index].url);
                }
                _.nav_entries[_.current_nav_index].url = _.allocator.dupe(u8, url) catch "";
                _.url = _.nav_entries[_.current_nav_index].url;
            }
        }
    }

    pub fn pollNavigatedWithinDocEvents(*_, out_events: []CdpEvent): usize {
        let ev_idx: usize = 0;

        // Pass A: Drain pending same-doc navigations from JS context into local stack buffer
        let js_urls: [4][]const u8 = [""; 4];
        let js_pushes: [4]bool = [true; 4];
        let js_count: usize = 0;

        const drain_expr = "(function(){if(typeof __capsurf_drain_same_doc_nav==='function')return __capsurf_drain_same_doc_nav();const q=globalThis.__capsurf_same_doc_nav||[];globalThis.__capsurf_same_doc_nav=[];return JSON.stringify(q);})()";
        const val = _.ctx.eval(drain_expr);
        defer _.ctx.freeValue(val);

        const json_str = _.ctx.toCString(val);
        defer std.heap.page_allocator.free(json_str);

        if (json_str.len > 2) {
            let pos: usize = 0;
            while (js_count < js_urls.len) {
                const url_marker = "\"url\":\"";
                const url_start_opt = std.mem.indexOfPos(u8, json_str, pos, url_marker);
                if (url_start_opt == null) break;
                const url_start = url_start_opt.? + url_marker.len;
                const url_end_opt = std.mem.indexOfPos(u8, json_str, url_start, "\"");
                if (url_end_opt == null) break;
                const url_val = json_str[url_start..url_end_opt.?];

                let is_push = true;
                if (push_start_opt := std.mem.indexOfPos(u8, json_str, url_end_opt.?, "\"isPush\":")) {
                    const p_start = push_start_opt + 9;
                    if (p_start + 5 <= json_str.len && std.mem.eql(u8, json_str[p_start..p_start + 5], "false")) {
                        is_push = false;
                    }
                } else if (type_start_opt := std.mem.indexOfPos(u8, json_str, url_end_opt.?, "\"type\":\"")) {
                    const t_start = type_start_opt + 8;
                    if (t_end_opt := std.mem.indexOfPos(u8, json_str, t_start, "\"")) {
                        const type_val = json_str[t_start..t_end_opt];
                        if (std.mem.eql(u8, type_val, "replaceState")) {
                            is_push = false;
                        }
                    }
                }

                pos = url_end_opt.? + 1;

                if (canRewriteDocumentUrl(_.url, url_val)) {
                    js_urls[js_count] = url_val;
                    js_pushes[js_count] = is_push;
                    js_count += 1;
                }
            }
        }

        // Pass B: Apply same-doc navigation state updates (all mutable operations)
        let ji: usize = 0;
        while (ji < js_count) : (ji += 1) {
            _.recordSameDocNavigation(js_urls[ji], js_pushes[ji]);
        }

        // Pass C: Format and emit pending events from CP queue
        const p_count = _.pending_same_doc_count;
        _.pending_same_doc_count = 0;

        let pi: usize = 0;
        while (pi < p_count && ev_idx < out_events.len) : (pi += 1) {
            const pending_url = _.pending_same_doc_urls[pi];
            const offset = ev_idx * 512;
            const chunk = _.same_doc_event_buf[offset .. offset + 512];

            let esc_buf: [512]u8 = [0; 512];
            const esc_url = escapeJsonString(pending_url, &esc_buf);

            const nav_json = std.fmt.bufPrint(
                chunk,
                "{{\"frameId\":\"{s}\",\"url\":\"{s}\",\"navigationType\":\"historyApi\"}}",
                .{ _.frame_id, esc_url },
            ) catch "";

            if (pending_url.len > 0) {
                _.allocator.free(pending_url);
                _.pending_same_doc_urls[pi] = "";
            }

            if (nav_json.len > 0) {
                out_events[ev_idx] = CdpEvent.create("Page.navigatedWithinDocument", nav_json);
                ev_idx += 1;
            }
        }

        // Pass D: Format and emit events from JS queue
        let ki: usize = 0;
        while (ki < js_count && ev_idx < out_events.len) : (ki += 1) {
            const url_val = js_urls[ki];
            const offset = ev_idx * 512;
            const chunk = _.same_doc_event_buf[offset .. offset + 512];

            let esc_buf: [512]u8 = [0; 512];
            const esc_url = escapeJsonString(url_val, &esc_buf);

            const nav_type = if (js_pushes[ki]) "historyApi" else "replaceState";
            const nav_json = std.fmt.bufPrint(
                chunk,
                "{{\"frameId\":\"{s}\",\"url\":\"{s}\",\"navigationType\":\"{s}\"}}",
                .{ _.frame_id, esc_url, nav_type },
            ) catch break;

            out_events[ev_idx] = CdpEvent.create("Page.navigatedWithinDocument", nav_json);
            ev_idx += 1;
        }

        return ev_idx;
    }

    pub fn formatExecutionContextEvent(*_, origin: []const u8): []const u8 {
        return std.fmt.bufPrint(
            &_.rt_ctx_event_buf,
            "{{\"context\":{{\"id\":1,\"origin\":\"{s}\",\"name\":\"\",\"uniqueId\":\"ctx-1\",\"auxData\":{{\"isDefault\":true,\"type\":\"default\",\"frameId\":\"{s}\"}}}}}}",
            .{ origin, _.frame_id },
        ) catch "{\"context\":{\"id\":1,\"origin\":\"about:blank\",\"name\":\"\",\"uniqueId\":\"ctx-1\",\"auxData\":{\"isDefault\":true,\"type\":\"default\",\"frameId\":\"capsurf-frame-001\"}}}";
    }

    pub fn formatFrameNavigatedEvent(*_, url: []const u8): []const u8 {
        return std.fmt.bufPrint(
            &_.page_frame_event_buf,
            "{{\"frame\":{{\"id\":\"{s}\",\"loaderId\":\"{s}\",\"url\":\"{s}\",\"securityOrigin\":\"{s}\",\"mimeType\":\"text/html\"}}}}",
            .{ _.frame_id, _.loader_id, url, url },
        ) catch "{}";
    }

    pub fn getTitle(*_): []const u8 {
        const title_node = Matcher.querySelector(&_.doc, "title");
        if (title_node != NULL_NODE) {
            const first_text = _.doc.getFirstChild(title_node);
            if (first_text != NULL_NODE) {
                const t = _.doc.getTextContent(first_text);
                if (t.len > 0) return t;
            }
            const t2 = _.doc.getTextContent(title_node);
            if (t2.len > 0) return t2;
        }
        if (_.title.len > 0) return _.title;
        return "";
    }

    pub fn getPageStateJson(*_, out_buf: []u8): ([]const u8)? {
        let js_seq: u32 = 0;
        if (_.has_document) {
            const v = _.ctx.eval("globalThis.__treeSeq || 0");
            if (!jsIsException(v)) {
                let n: i32 = 0;
                * = JS_ToInt32(_.ctx.raw_ctx, &n, v);
                if (n > 0) js_seq = @as(u32, @intCast(n));
            }
            _.ctx.freeValue(v);
        }
        const total_seq: u32 = _.doc.mutation_seq + js_seq;
        const title_str = _.getTitle();

        let esc_title_buf: [512]u8 = undefined;
        const esc_title = escapeJsonString(title_str, &esc_title_buf);

        let esc_url_buf: [1024]u8 = undefined;
        const esc_url = escapeJsonString(_.url, &esc_url_buf);

        const sx: i32 = @intFromFloat(_.scroll_x);
        const sy: i32 = @intFromFloat(_.scroll_y);

        const res = std.fmt.bufPrint(
            out_buf,
            "{{\"seq\":{d},\"scrollX\":{d},\"scrollY\":{d},\"href\":\"{s}\",\"title\":\"{s}\"}}",
            .{ total_seq, sx, sy, esc_url, esc_title },
        ) catch return null;
        return res;
    }

    pub fn setControlValueJson(*_, params_json: []const u8, out_buf: []u8): ([]const u8)? {
        const node_id = extractJsonInt(params_json, "nodeId") ?? -1;
        if (node_id <= 0) {
            const bad_res = "{\"changed\":false,\"error\":\"bad nodeId\"}";
            if (bad_res.len > out_buf.len) return null;
            @memcpy(out_buf[0..bad_res.len], bad_res);
            return out_buf[0..bad_res.len];
        }
        if (@as(usize, @intCast(node_id)) >= _.doc.nodes.items.len) {
            const bad_res = "{\"changed\":false,\"error\":\"bad nodeId\"}";
            if (bad_res.len > out_buf.len) return null;
            @memcpy(out_buf[0..bad_res.len], bad_res);
            return out_buf[0..bad_res.len];
        }
        if (!_.has_document) {
            const bad_res = "{\"changed\":false,\"error\":\"bad nodeId\"}";
            if (bad_res.len > out_buf.len) return null;
            @memcpy(out_buf[0..bad_res.len], bad_res);
            return out_buf[0..bad_res.len];
        }

        const global_obj = _.ctx.getGlobal();
        defer _.ctx.freeValue(global_obj);

        const params_val = _.ctx.newString(params_json);
        * = JS_SetPropertyStr(_.ctx.raw_ctx, global_obj, "__capsurf_temp_control_params", params_val);

        const res_val = _.ctx.eval("(function(){ if (typeof __capsurf_setControlValueJson === 'function') { return __capsurf_setControlValueJson(globalThis.__capsurf_temp_control_params); } return JSON.stringify({changed:false,error:'bad nodeId'}); })()");
        defer _.ctx.freeValue(res_val);

        const str = _.ctx.toCString(res_val);
        defer std.heap.page_allocator.free(str);
        if (str.len > 0 && str.len <= out_buf.len) {
            @memcpy(out_buf[0..str.len], str);
            return out_buf[0..str.len];
        }

        const default_res = "{\"changed\":false,\"error\":\"bad nodeId\"}";
        if (default_res.len > out_buf.len) return null;
        @memcpy(out_buf[0..default_res.len], default_res);
        return out_buf[0..default_res.len];
    }

    pub fn markPaintDamage(*_) {
        _.doc.markPaintDamage();
    }

    pub fn getPaintDamageSinceJson(*_, params_json: []const u8, out_buf: []u8): ([]const u8)? {
        let excl_id: i32 = 0;
        if (std.mem.indexOf(u8, params_json, "\"excludeNodeId\"") != null) {
            if (e_val := extractJsonInt(params_json, "excludeNodeId")) {
                excl_id = @as(i32, @intCast(e_val));
            } else {
                excl_id = -1;
            }
        }
        return _.getPaintDamageSince(excl_id, out_buf);
    }

    pub fn getPaintDamageSince(*_, exclude_id: i32, out_buf: []u8): ([]const u8)? {
        if (exclude_id < 0) {
            const err_json = "{\"nodeIds\":[],\"overflow\":false,\"error\":\"bad nodeId\"}";
            if (err_json.len > out_buf.len) return null;
            @memcpy(out_buf[0..err_json.len], err_json);
            return out_buf[0..err_json.len];
        }

        const excl_node: NodeId = if (exclude_id > 0) @as(NodeId, @intCast(exclude_id)) else NULL_NODE;

        let offset: usize = 0;
        const header = "{\"nodeIds\":[";
        if (offset + header.len > out_buf.len) return null;
        @memcpy(out_buf[offset .. offset + header.len], header);
        offset += header.len;

        let first = true;
        let i: usize = 0;
        while (i < _.doc.damaged_nodes.items.len) : (i += 1) {
            const nid = _.doc.damaged_nodes.items[i];
            if (excl_node != NULL_NODE && _.doc.isLineageOrSubtree(nid, excl_node)) {
                continue;
            }
            if (!first) {
                if (offset + 1 > out_buf.len) return null;
                out_buf[offset] = ',';
                offset += 1;
            }
            first = false;
            const num = std.fmt.bufPrint(out_buf[offset..], "{d}", .{nid}) catch break;
            offset += num.len;
        }

        const footer = "],\"overflow\":false}";
        if (offset + footer.len <= out_buf.len) {
            @memcpy(out_buf[offset .. offset + footer.len], footer);
            offset += footer.len;
            return out_buf[0..offset];
        }
        return null;
    }
}
