import {
    PageIsolateCaps,
    PageScriptKey,
    pageIsolateCaps,
    pageScriptCacheKey,
    hostnameOf,
    originOf,
    formatRemoteObject,
    formatConsoleLogEntry,
} from "../../src/eval/page_script.cp";
import {
    InflightRpcCall,
    PageScriptRpcSession,
    openPageScriptRpcSession,
} from "../../src/eval/page_script_rpc_session.cp";
import {
    RenderStage,
    RenderFrameOptions,
    RenderFrameResult,
    RenderPdfOptions,
    RenderPdfResult,
    PageRendererManager,
    renderStageToString,
} from "../../src/eval/page_renderer.cp";
import {
    RasterizeOptions,
    RenderFrameArgs,
    PrespawnedFrame,
    ClampedFrameViewport,
    OopifRenderCoordinator,
    clampFrameViewport,
    rgbaFrameExpectedBytes,
    originOfFrameUrl,
} from "../../src/eval/oopif_render.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. Test page_script.cp
    {
        let caps = pageIsolateCaps(100, 500, 256);
        assert(caps.cpu_ms == 100, "caps cpu_ms");
        assert(caps.subrequests == 500, "caps subrequests");
        assert(caps.memory_mb == 256, "caps memory_mb");

        let key = PageScriptKey{
            origin: "https://example.com",
            key: "session-k",
            frame_id: "frame-main-001",
            loader_id: "loader-001",
        };
        let k_buf: [256]u8 = [0; 256];
        let cache_key = pageScriptCacheKey(key, &k_buf);
        assert(cache_key.len > 0, "cache key not empty");
        assert(std.mem.startsWith(u8, cache_key, "page-"), "cache key prefix");

        let host = hostnameOf("https://sub.domain.com:8080/path?query=1");
        assert(std.mem.eql(u8, host, "sub.domain.com"), "hostnameOf matches");

        let orig = originOf("https://sub.domain.com:8080/path?query=1");
        assert(std.mem.eql(u8, orig, "https://sub.domain.com:8080"), "originOf matches");

        let ro_buf: [256]u8 = [0; 256];
        let ro = formatRemoteObject("42", &ro_buf);
        assert(std.mem.indexOf(u8, ro, "\"type\":\"string\"") != null, "remoteObject formatted");

        let c_buf: [256]u8 = [0; 256];
        let c_log = formatConsoleLogEntry("log", "Hello CapSurf", &c_buf);
        assert(std.mem.indexOf(u8, c_log, "\"type\":\"log\"") != null, "console log formatted");
    }

    // 2. Test page_script_rpc_session.cp
    {
        let session = openPageScriptRpcSession("sess-eval-001");
        assert(!session.is_closed, "session is open");
        assert(session.getInflightCount() == 0, "initial inflight is 0");

        let req1 = session.trackCall("Runtime.evaluate", 10000);
        assert(req1 > 0, "req1 id positive");
        assert(session.getInflightCount() == 1, "inflight is 1");

        let req2 = session.trackCall("Page.captureScreenshot", 10000);
        assert(req2 > req1, "req2 id incremented");
        assert(session.getInflightCount() == 2, "inflight is 2");

        assert(session.completeCall(req1), "completeCall req1");
        assert(session.getInflightCount() == 1, "inflight is 1 after complete");

        session.finishSession("navigation");
        assert(session.is_closed, "session is closed");
        assert(session.getInflightCount() == 0, "inflight is 0 after finish");
    }

    // 3. Test page_renderer.cp
    {
        let mgr: PageRendererManager = PageRendererManager{};
        mgr.init("renderer-main-001");
        assert(mgr.isActive(), "manager is active");
        assert(mgr.getRenderCount() == 0, "initial render count 0");

        let frame_opts = RenderFrameOptions{
            width: 1920,
            height: 1080,
            device_scale_factor: 2.0,
            format: "png",
            quality: 90,
        };
        let frame_res = mgr.renderFrame(frame_opts);
        assert(frame_res.ok, "frame render ok");
        assert(frame_res.width == 1920, "frame width matches");
        assert(frame_res.height == 1080, "frame height matches");
        assert(mgr.getRenderCount() == 1, "render count incremented");

        let pdf_opts = RenderPdfOptions{
            paper_width: 8.5,
            paper_height: 11.0,
            margin_top: 0.5,
            margin_bottom: 0.5,
            landscape: false,
        };
        let pdf_res = mgr.renderPdf(pdf_opts);
        assert(pdf_res.ok, "pdf render ok");
        assert(pdf_res.page_count == 1, "pdf page count 1");
        assert(mgr.getRenderCount() == 2, "render count 2");

        mgr.close();
        assert(!mgr.isActive(), "manager is inactive after close");

        assert(std.mem.eql(u8, renderStageToString(.Raster), "raster"), "render stage string");
    }

    // 4. Test oopif_render.cp
    {
        let clamped = clampFrameViewport(10000, 10000);
        assert(clamped.width <= 4096, "clamped width <= MAX_SIDE");
        assert(clamped.height <= 4096, "clamped height <= MAX_SIDE");
        assert(@as(u64, clamped.width) * @as(u64, clamped.height) <= 7800000, "clamped area <= MAX_AREA");

        let exp_bytes = rgbaFrameExpectedBytes(800, 600);
        assert(exp_bytes == 800 * 600 * 4 + 8, "expected bytes calculation");

        let frame_orig = originOfFrameUrl("https://child.iframe.org/embed?id=123");
        assert(std.mem.eql(u8, frame_orig, "https://child.iframe.org"), "originOfFrameUrl matches");

        let coord: OopifRenderCoordinator = OopifRenderCoordinator{};
        assert(coord.activeCount() == 0, "initial active count 0");

        assert(coord.prespawnFrame("frame-sub-99", "iso-sub-99", 5000), "prespawnFrame ok");
        assert(coord.activeCount() == 1, "active count is 1");
        assert(coord.hasFrame("frame-sub-99"), "hasFrame true");

        let r_args = RenderFrameArgs{
            frame_id: "frame-sub-99",
            url: "https://child.iframe.org/embed",
            width: 800,
            height: 600,
            scale: 1.0,
        };
        let r_opts = RasterizeOptions{
            format: "png",
            quality: 85,
        };
        let r_res = coord.recordAndRasterizeFrame(r_args, r_opts);
        assert(r_res.ok, "recordAndRasterizeFrame ok");
        assert(r_res.width == 800, "rasterized width matches");

        assert(coord.discardPrespawnedFrame("frame-sub-99"), "discardPrespawnedFrame ok");
        assert(coord.activeCount() == 0, "active count 0 after discard");
        assert(!coord.hasFrame("frame-sub-99"), "hasFrame false after discard");
    }

    std.debug.print("test_page_script_render: ALL ASSERTIONS PASSED\n", .{});
}
