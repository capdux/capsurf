// src/cdp/session.cp
// CapSurf CDP Page Session Controller
// Coordinates HTML parsing, CSS cascade, Layout computation, and JavaScript execution.

import { Doc, Document, NodeId, NULL_NODE, NodeType } from "../dom/dom.cp";
import { Matcher } from "../css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../css/cascade.cp";
import { LayoutTree, Rect } from "../layout/layout.cp";
import { JsRuntime, JsContext, JSValue, jsToSlice, jsIsException, JS_TAG_INT, JS_TAG_FLOAT64, JS_TAG_BOOL, JS_TAG_NULL, JS_TAG_UNDEFINED, JS_TAG_OBJECT } from "../js/qjs_engine.cp";
import { mountDom, JsDomContext } from "../js/dom_binding.cp";
import { CookieJar, CookieEntry } from "../net/cookies.cp";
import { CdpBoxModel, CdpEvent } from "./types.cp";
import { FetchState } from "./fetch.cp";
import { ScreenshotOptions, renderDocumentToPng } from "../render/painter.cp";
import { CssColor } from "cap:css";
import { TargetRegistry } from "./target.cp";
import { DedicatedWorkerPool } from "./workers.cp";
import { DebuggerState } from "./debugger.cp";
import { ScreencastHandler } from "./screencast.cp";
import { ResourceStore } from "./resource_store.cp";
import { KitesurfFacade } from "./kitesurf_facade.cp";

pub struct CdpPageSession {
    allocator: std.mem.Allocator,
    doc: Document,
    layout: LayoutTree,
    rt: JsRuntime,
    ctx: JsContext,
    dom_ctx: (*JsDomContext)? = null,
    url: []const u8 = "about:blank",
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
    user_agent: []const u8 = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/145.0.0.0 Safari/537.36 CapSurf/1.0.0",

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

    // Focus & Scroll state
    focused_node: NodeId = NULL_NODE,
    scroll_x: f32 = 0.0,
    scroll_y: f32 = 0.0,

    pub static fn init(allocator: std.mem.Allocator): (*CdpPageSession)! {
        let ptr = std.heap.c_allocator.create(CdpPageSession) catch return error.AllocFailed;
        const document = Document.init(allocator) catch return error.DocInitFailed;
        const layout_tree = LayoutTree.init(allocator, 1280.0, 720.0) catch return error.LayoutInitFailed;
        const js_rt = JsRuntime.init(16 * 1024 * 1024, 200000) catch return error.JsRuntimeFailed;
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
            user_agent: "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/145.0.0.0 Safari/537.36 CapSurf/1.0.0",
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
            focused_node: NULL_NODE,
            scroll_x: 0.0,
            scroll_y: 0.0,
        };
        ptr.ctx = ptr.rt.createContext() catch return error.JsContextFailed;
        ptr.navigate("<!DOCTYPE html><html><head><title>Blank Page</title></head><body></body></html>", "about:blank") catch {};
        return ptr;
    }

    pub fn deinit(*_) {
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

    pub fn navigate(*_, html: []const u8, nav_url: []const u8)! {
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

        // Layout & Styles
        const css_text = "* { box-sizing: border-box; }";
        const sheet = StyleSheet.parse(css_text, 1280.0, false);
        let styles: [64]ComputedStyle = [ComputedStyle{}; 64];
        const count = @min(_.doc.nodeCount(), styles.len);
        Cascade.computeStyles(&_.doc, &sheet, styles[0..count], 16.0);
        _.layout.computeLayout(&_.doc, styles[0..count]) catch {};

        // Mount to JS Context
        if (_.dom_ctx != null) {
            std.heap.c_allocator.destroy(_.dom_ctx.?);
            _.dom_ctx = null;
        }
        const m = mountDom(&_.ctx, &_.doc, &_.layout) catch return error.MountDomFailed;
        _.dom_ctx = m;
        _.url = nav_url;
        _.has_document = true;
        _.focused_node = NULL_NODE;
        _.scroll_x = 0.0;
        _.scroll_y = 0.0;

        // Execute inline <script> tags in document order
        let nid: usize = 1;
        while (nid < _.doc.nodes.items.len) : (nid += 1) {
            const n = &_.doc.nodes.items[nid];
            if (n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, "script")) {
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
                    * = js_res;
                }
            }
        }

        // Dispatch HTML5 lifecycle events (DOMContentLoaded on document, load on window)
        const lc_res = _.ctx.eval("(function(){if(typeof __capsurf_dispatch_lifecycle==='function')__capsurf_dispatch_lifecycle();})()");
        _.ctx.freeValue(lc_res);
    }

fn escapeJsonString(src: []const u8, dst: []u8): []const u8 {
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

    pub fn evaluate(*_, expr: []const u8, out_buf: []u8): usize {
        const val = _.ctx.eval(expr);
        defer _.ctx.freeValue(val);

        if (_.ctx.isPromise(val)) {
            let limit: u32 = 100000;
            while (_.ctx.promiseState(val) == 0 && limit > 0) : (limit -= 1) {
                const r = _.ctx.executePendingJobs();
                if (r <= 0) break;
            }

            if (_.ctx.promiseState(val) == 1) {
                const res_val = _.ctx.promiseResult(val);
                defer _.ctx.freeValue(res_val);
                return _.formatCdpResult(res_val, out_buf);
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

        return _.formatCdpResult(val, out_buf);
    }

    pub fn querySelector(*_, selector: []const u8): NodeId {
        return Matcher.querySelector(&_.doc, selector);
    }

    pub fn querySelectorAll(*_, selector: []const u8, out_ids: []NodeId): usize {
        return Matcher.querySelectorAll(&_.doc, selector, out_ids);
    }

    pub fn getBoxModel(*_, node_id: NodeId): CdpBoxModel {
        const rect = _.layout.getBoundingClientRect(node_id);
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

        const opts = ScreenshotOptions{
            viewport_w: vw,
            viewport_h: vh,
            compress_level: if (compress_level <= 9) compress_level else 6,
            clear_color: CssColor.rgb(255, 255, 255),
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
                    const rt_json = std.fmt.bufPrint(
                        &_.console_event_buf,
                        "{{\"type\":\"{s}\",\"args\":[{{\"type\":\"string\",\"value\":\"{s}\"}}],\"executionContextId\":1,\"timestamp\":{d}}}",
                        .{ lvl, txt, rec.timestamp },
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
}
