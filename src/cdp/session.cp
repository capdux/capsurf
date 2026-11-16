// src/cdp/session.cp
// CapSurf CDP Page Session Controller
// Coordinates HTML parsing, CSS cascade, Layout computation, and JavaScript execution.

import { Doc, Document, NodeId, NULL_NODE, NodeType } from "../dom/dom.cp";
import { Matcher } from "../css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../css/cascade.cp";
import { LayoutTree, Rect } from "../layout/layout.cp";
import { JsRuntime, JsContext, jsToSlice, jsIsException } from "../js/qjs_engine.cp";
import { mountDom, JsDomContext } from "../js/dom_binding.cp";
import { CookieJar, CookieEntry } from "../net/cookies.cp";
import { CdpBoxModel } from "./types.cp";
import { FetchState } from "./fetch.cp";
import { ScreenshotOptions, renderDocumentToPng } from "../render/painter.cp";
import { CssColor } from "cap:css";
import { TargetRegistry } from "./target.cp";

pub struct CdpPageSession {
    allocator: std.mem.Allocator,
    doc: Document,
    layout: LayoutTree,
    rt: JsRuntime,
    ctx: JsContext,
    dom_ctx: *JsDomContext? = null,
    url: []const u8 = "about:blank",
    title: []const u8 = "Blank Page",
    frame_id: []const u8 = "capsurf-frame-001",
    loader_id: []const u8 = "capsurf-loader-001",
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
            focused_node: NULL_NODE,
            scroll_x: 0.0,
            scroll_y: 0.0,
        };
        ptr.ctx = ptr.rt.createContext() catch return error.JsContextFailed;
        return ptr;
    }

    pub fn deinit(*_) {
        if (_.raw_html.len > 0) {
            _.allocator.free(_.raw_html);
            _.raw_html = "";
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
        const m = mountDom(&_.ctx, &_.doc, &_.layout) catch return error.MountDomFailed;
        _.dom_ctx = m;
        _.url = nav_url;
        _.has_document = true;
        _.focused_node = NULL_NODE;
        _.scroll_x = 0.0;
        _.scroll_y = 0.0;
    }

    pub fn evaluate(*_, expr: []const u8, out_buf: []u8): usize {
        const val = _.ctx.eval(expr);
        defer _.ctx.freeValue(val);

        if (jsIsException(val)) {
            const exc = _.ctx.getException();
            defer _.ctx.freeValue(exc);
            const err_json = "{\"result\":{\"type\":\"undefined\",\"description\":\"EvaluationException\"}}";
            const len = @min(err_json.len, out_buf.len);
            @memcpy(out_buf[0..len], err_json[0..len]);
            return len;
        }

        const res_str = _.ctx.toCString(val);
        defer std.heap.page_allocator.free(res_str);

        const formatted = std.fmt.bufPrint(
            out_buf,
            "{{\"result\":{{\"type\":\"string\",\"value\":\"{s}\"}}}}",
            .{res_str},
        ) catch return 0;
        return formatted.len;
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
}
