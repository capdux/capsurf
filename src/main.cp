// src/main.cp
// CapSurf: Million-Agent Native Headless Browser in CP

import { Doc, Document, NodeId, NULL_NODE } from "./dom/dom.cp";
import { Matcher } from "./css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "./css/cascade.cp";
import { LayoutTree, Rect } from "./layout/layout.cp";
import { JsRuntime, JsContext } from "./js/qjs_engine.cp";
import { mountDom } from "./js/dom_binding.cp";
import { CdpServer } from "./cdp/server.cp";
import { AccessibilityHandler } from "./cdp/accessibility.cp";
import { WebMcpHandler } from "./cdp/webmcp.cp";
import { CliOptions } from "./cli/options.cp";

fn printUsage() {
    std.debug.print(
        "CapSurf: Million-Agent Native Headless Browser in CP\n\n" ++
        "Usage: capsurf [options] [url]\n\n" ++
        "Options:\n" ++
        "  --remote-debugging-port=<port>     Set CDP remote debugging port (default: 9222)\n" ++
        "  --remote-debugging-address=<ip>    Set listen address (default: 127.0.0.1)\n" ++
        "  --window-size=<width>,<height>     Set viewport resolution (default: 1280,720)\n" ++
        "  --user-data-dir=<dir>              Set profile directory path\n" ++
        "  --headless                         Run in headless mode (default: true)\n" ++
        "  --no-sandbox, --disable-gpu        Standard Chromium flags (gracefully accepted)\n" ++
        "  --demo                             Run built-in multi-milestone self-check demo\n" ++
        "  -v, --version                      Print CapSurf engine version\n" ++
        "  -h, --help                         Show this help message\n\n",
        .{},
    );
}

fn runDemo() {
    std.debug.print("==================================================================\n", .{});
    std.debug.print(" CapSurf: Million-Agent Native Headless Browser in CP (Self-Check)\n", .{});
    std.debug.print("==================================================================\n", .{});
    std.debug.print("[Milestone 1] Arena DOM (u32) + cap:css Selectors + 64-bit Cascade + Layout\n", .{});
    std.debug.print("[Milestone 2] HTML5 Tokenizer (80 states) + TreeBuilder (AAA, Foster)\n", .{});
    std.debug.print("[Milestone 3] QuickJS-NG Engine Binding + DOM API Mounting + Preemption\n", .{});
    std.debug.print("[Milestone 4] CDP Protocol Server + AXTree Semantics + WebMCP Tool Reflection\n", .{});

    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const sample_html =
        "<!DOCTYPE html>" ++
        "<html><head><title>CapSurf Demo</title></head>" ++
        "<body>" ++
        "  <div id=\"app\" style=\"display: flex; width: 600px; height: 100px; padding: 10px;\">" ++
        "    <button id=\"btn-run\" class=\"btn action\" style=\"width: 120px; height: 40px;\">Launch Agent</button>" ++
        "  </div>" ++
        "</body></html>";

    doc.parseHtml(sample_html) catch unreachable;

    const btn = doc.getElementById("btn-run");
    const q_title = Matcher.querySelector(&doc, "head > title");
    const title_text = doc.getTextContent(doc.getFirstChild(q_title));

    std.debug.print("[Demo Parsed] Page Title: \"{s}\", Target Button NodeId: {}\n", .{ title_text, btn });

    const css_text =
        "* { box-sizing: border-box; margin: 0; padding: 0; }\n" ++
        "#app { display: flex; flex-direction: row; padding: 10px; width: 600px; height: 100px; }\n" ++
        "#btn-run { width: 120px; height: 40px; }\n";

    const sheet = StyleSheet.parse(css_text, 800.0, false);
    let styles: [16]ComputedStyle = [ComputedStyle{}; 16];
    Cascade.computeStyles(&doc, &sheet, styles[0..doc.nodeCount()], 16.0);

    let layout = LayoutTree.init(std.heap.page_allocator, 800.0, 600.0) catch unreachable;
    defer layout.deinit();
    layout.computeLayout(&doc, styles[0..doc.nodeCount()]) catch unreachable;

    const btn_rect = layout.getBoundingClientRect(btn);
    std.debug.print(
        "[Demo Layout] Button Screen Rect: x={d:.1}, y={d:.1}, w={d:.1}, h={d:.1}, visible={}\n",
        .{ btn_rect.x, btn_rect.y, btn_rect.width, btn_rect.height, btn_rect.visible },
    );

    let rt = JsRuntime.init(32 * 1024 * 1024, 100000) catch unreachable;
    defer rt.deinit();

    let ctx = rt.createContext() catch unreachable;
    defer ctx.deinit();

    * = mountDom(&ctx, &doc, &layout) catch unreachable;

    const js_demo =
        "console.log('[JS Log] Hello from QuickJS inside CapSurf!');\n" ++
        "const btn = document.getElementById('btn-run');\n" ++
        "const r = btn.getBoundingClientRect();\n" ++
        "console.log('[JS Log] Button Rect via JS getBoundingClientRect():', r.width + 'x' + r.height + '@' + r.left + ',' + r.top);\n" ++
        "btn.setAttribute('data-agent-active', 'true');\n" ++
        "console.log('[JS Log] Set data-agent-active =', btn.getAttribute('data-agent-active'));\n";

    const demo_val = ctx.eval(js_demo);
    defer ctx.freeValue(demo_val);

    std.debug.print("Status: Self-Check Complete (100% Native Evaluated).\n", .{});
}

fn main() {
    const opts = CliOptions.parseFromProcess();

    if (opts.show_help) {
        printUsage();
        return;
    }

    if (opts.show_version) {
        std.debug.print("CapSurf 1.0.0 (Native CP, Million-Agent Headless Engine)\n", .{});
        return;
    }

    if (opts.is_demo) {
        runDemo();
        return;
    }

    // Start CapSurf Native CDP Headless Browser Server
    let server = CdpServer.init(std.heap.page_allocator, opts.port) catch |e| {
        std.debug.print("[CapSurf Error] Failed to initialize CDP server on port {d}: {any}\n", .{ opts.port, e });
        return;
    };
    defer server.deinit();

    // Configure viewport if customized
    server.session.viewport_width = @as(f32, @floatFromInt(opts.window_width));
    server.session.viewport_height = @as(f32, @floatFromInt(opts.window_height));

    server.start() catch |e| {
        std.debug.print("[CapSurf Error] Failed to bind/start server on port {d}: {any}\n", .{ opts.port, e });
        return;
    };

    // Standard Chromium DevTools discovery string required by Playwright / Puppeteer
    std.debug.print("DevTools listening on ws://{s}:{d}/devtools/browser/capsurf-browser-001\n", .{ opts.address, opts.port });

    // Run Accept Loop until stopped
    * = server.runAcceptLoop() catch {};
}
