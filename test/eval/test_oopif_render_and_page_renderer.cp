// test/eval/test_oopif_render_and_page_renderer.cp
// Unit tests for src/eval/oopif_render.cp and src/eval/page_renderer.cp

import {
    MAX_OOPIF_SIDE,
    MAX_OOPIF_AREA,
    RGBA_BYTES_PER_PIXEL,
    RGBA_FRAME_HEADER_BYTES,
    RasterizeOptions,
    RenderFrameArgs,
    ClampedFrameViewport,
    OopifRenderCoordinator,
    clampFrameViewport,
    rgbaFrameExpectedBytes,
    originOfFrameUrl,
} from "../../src/eval/oopif_render.cp";

import {
    PAGE_RENDERER_COMPAT_DATE,
    MAX_RENDER_RASTER_ATTEMPTS,
    RenderStage,
    RenderFrameOptions,
    RenderFrameResult,
    RenderPdfOptions,
    RenderPdfResult,
    PageRendererManager,
    renderStageToString,
} from "../../src/eval/page_renderer.cp";

fn check(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test failure");
    }
}

fn testOopifViewportClampingAndBounds() {
    std.debug.print("1. OOPIF viewport clamping, zero-size bounds, and URL origin\n", .{});

    const zero_both = clampFrameViewport(0, 0);
    check(zero_both.width == 1 && zero_both.height == 1, "zero width/height clamped to 1x1");

    const zero_w = clampFrameViewport(0, 500);
    check(zero_w.width == 1 && zero_w.height == 500, "zero width clamped to 1");

    const zero_h = clampFrameViewport(500, 0);
    check(zero_h.width == 500 && zero_h.height == 1, "zero height clamped to 1");

    const normal = clampFrameViewport(1280, 720);
    check(normal.width == 1280 && normal.height == 720, "in-bounds viewport unchanged");

    const wide = clampFrameViewport(5000, 300);
    check(wide.width == MAX_OOPIF_SIDE && wide.height == 300, "width clamped to MAX_OOPIF_SIDE");

    const huge = clampFrameViewport(4096, 4096);
    check(huge.width <= MAX_OOPIF_SIDE && huge.height <= MAX_OOPIF_SIDE, "area-clamped sides <= MAX_OOPIF_SIDE");
    check(huge.width >= 1 && huge.height >= 1, "area-clamped sides >= 1");
    check(@as(u64, huge.width) * @as(u64, huge.height) <= @as(u64, MAX_OOPIF_AREA), "clamped area <= MAX_OOPIF_AREA");

    check(rgbaFrameExpectedBytes(0, 0) == @as(usize, RGBA_FRAME_HEADER_BYTES), "0x0 frame has header bytes only");
    check(
        rgbaFrameExpectedBytes(1280, 720) == @as(usize, RGBA_FRAME_HEADER_BYTES) + 1280 * 720 * @as(usize, RGBA_BYTES_PER_PIXEL),
        "1280x720 RGBA frame byte size",
    );

    check(std.mem.eql(u8, originOfFrameUrl("https://sub.example.com:8443/frame.html?a=1"), "https://sub.example.com:8443"), "origin with path");
    check(std.mem.eql(u8, originOfFrameUrl("https://example.com"), "https://example.com"), "origin without path");
    check(std.mem.eql(u8, originOfFrameUrl("about:blank"), ""), "non-scheme URL returns empty origin");
}

fn testOopifCoordinatorMultiFrameAndRasterize() {
    std.debug.print("2. OOPIF coordinator multi-frame ordering, capacity, and rasterize clipping\n", .{});

    let coord = OopifRenderCoordinator{};
    check(coord.activeCount() == 0, "initial activeCount is 0");
    check(!coord.prespawnFrame("", "iso-0", 100), "empty frame_id rejected");
    check(!coord.discardPrespawnedFrame(""), "empty discard rejected");
    check(!coord.hasFrame(""), "empty hasFrame is false");

    const frame_ids: [16][]const u8 = [
        "f-00", "f-01", "f-02", "f-03",
        "f-04", "f-05", "f-06", "f-07",
        "f-08", "f-09", "f-10", "f-11",
        "f-12", "f-13", "f-14", "f-15",
    ];
    let idx: usize = 0;
    while (idx < 16) : (idx += 1) {
        check(coord.prespawnFrame(frame_ids[idx], "iso-batch", @as(u64, idx + 1) * 10), "prespawn slot succeeds");
    }
    check(coord.activeCount() == 16, "all 16 slots active");
    check(coord.hasFrame("f-00") && coord.hasFrame("f-15"), "first and last frames present");
    check(!coord.prespawnFrame("f-16", "iso-overflow", 999), "17th frame rejected when full");
    check(coord.activeCount() == 16, "activeCount remains 16 on overflow");

    // Updating an existing frame keeps activeCount unchanged and updates metadata
    check(coord.prespawnFrame("f-05", "iso-updated", 777), "existing frame update returns true");
    check(coord.activeCount() == 16, "updating existing frame does not increment activeCount");
    check(std.mem.eql(u8, coord.frames[5].isolate_id, "iso-updated"), "updated isolate_id persisted");
    check(coord.frames[5].prespawn_time_ms == 777, "updated prespawn_time_ms persisted");

    // Discarding middle frame frees slot for a new frame
    check(coord.discardPrespawnedFrame("f-05"), "discard middle frame succeeds");
    check(!coord.hasFrame("f-05"), "discarded frame no longer present");
    check(coord.activeCount() == 15, "activeCount decremented to 15");
    check(!coord.discardPrespawnedFrame("f-05"), "double discard returns false");
    check(coord.prespawnFrame("f-16", "iso-reused", 888), "freed slot accepts new frame");
    check(coord.activeCount() == 16, "activeCount back to 16");

    // Record and rasterize validation and viewport clamping
    const bad_res = coord.recordAndRasterizeFrame(
        RenderFrameArgs{ frame_id: "", width: 800, height: 600 },
        RasterizeOptions{},
    );
    check(!bad_res.ok, "missing frame_id fails rasterize");
    check(std.mem.eql(u8, bad_res.error_msg, "missing frame_id in RenderFrameArgs"), "missing frame_id error message");

    const clipped_res = coord.recordAndRasterizeFrame(
        RenderFrameArgs{ frame_id: "f-16", url: "https://child.test/embed", width: 8192, height: 0, scale: 2.0 },
        RasterizeOptions{ format: "png", quality: 95, clip_x: 10.0, clip_y: 20.0, clip_width: 400.0, clip_height: 300.0 },
    );
    check(clipped_res.ok, "valid frame_id rasterize succeeds");
    check(clipped_res.width == MAX_OOPIF_SIDE && clipped_res.height == 1, "out-of-bounds/zero dimensions clamped");

    coord.clear();
    check(coord.activeCount() == 0, "clear resets activeCount to 0");
    check(!coord.hasFrame("f-00") && !coord.hasFrame("f-16"), "clear removes all frames");
}

fn testPageRendererViewportAndMetrics() {
    std.debug.print("3. PageRendererManager viewport/DPR/device-metrics and PDF rendering\n", .{});

    check(std.mem.eql(u8, PAGE_RENDERER_COMPAT_DATE, "2026-09-11"), "compat date matches");
    check(MAX_RENDER_RASTER_ATTEMPTS == 2, "max raster attempts is 2");
    check(std.mem.eql(u8, renderStageToString(.Record), "record"), "Record stage string");
    check(std.mem.eql(u8, renderStageToString(.Raster), "raster"), "Raster stage string");
    check(std.mem.eql(u8, renderStageToString(.Encode), "encode"), "Encode stage string");

    let mgr = PageRendererManager{};
    mgr.init("renderer-metrics-1");
    check(mgr.isActive(), "manager active after init");
    check(mgr.getRenderCount() == 0, "renderCount starts at 0");
    check(mgr.getLastRenderMs() == 0, "lastRenderMs starts at 0");

    const default_res = mgr.renderFrame(RenderFrameOptions{});
    check(default_res.ok, "default frame render succeeds");
    check(default_res.width == 1280 && default_res.height == 720, "default viewport 1280x720");
    check(mgr.getRenderCount() == 1 && mgr.frames_rendered == 1, "counters incremented to 1");
    check(mgr.getLastRenderMs() == 1, "lastRenderMs updated");

    const hidpi_res = mgr.renderFrame(RenderFrameOptions{
        width: 1920,
        height: 1080,
        device_scale_factor: 2.0,
        format: "webp",
        quality: 85,
    });
    check(hidpi_res.ok, "high-DPR frame render succeeds");
    check(hidpi_res.width == 1920 && hidpi_res.height == 1080, "high-DPR dimensions preserved");
    check(mgr.getRenderCount() == 2 && mgr.frames_rendered == 2, "counters incremented to 2");

    const zero_w = mgr.renderFrame(RenderFrameOptions{ width: 0, height: 720 });
    check(!zero_w.ok, "zero width frame rejected");
    const zero_h = mgr.renderFrame(RenderFrameOptions{ width: 1280, height: 0 });
    check(!zero_h.ok, "zero height frame rejected");
    check(mgr.getRenderCount() == 2, "invalid frame dims do not increment renderCount");

    const pdf_ok = mgr.renderPdf(RenderPdfOptions{
        paper_width: 8.5,
        paper_height: 11.0,
        margin_top: 0.5,
        margin_bottom: 0.5,
        landscape: true,
    });
    check(pdf_ok.ok && pdf_ok.page_count == 1, "valid PDF render succeeds");
    check(mgr.getRenderCount() == 3, "PDF render increments renderCount");
    check(mgr.frames_rendered == 2, "PDF render does not increment frames_rendered");

    const pdf_bad = mgr.renderPdf(RenderPdfOptions{ paper_width: 0.0, paper_height: 11.0 });
    check(!pdf_bad.ok, "zero paper_width PDF rejected");
    const pdf_neg = mgr.renderPdf(RenderPdfOptions{ paper_width: 8.5, paper_height: -1.0 });
    check(!pdf_neg.ok, "negative paper_height PDF rejected");
    check(mgr.getRenderCount() == 3, "invalid PDF dims do not increment renderCount");
}

fn testPageRendererStateTransitionsAndDisposal() {
    std.debug.print("4. PageRendererManager state transitions, lazy activation, disposal, and close\n", .{});

    let mgr = PageRendererManager{};
    check(!mgr.isActive(), "fresh manager starts inactive");

    // Lazy activation on first renderFrame without explicit init
    const lazy_res = mgr.renderFrame(RenderFrameOptions{ width: 640, height: 480 });
    check(lazy_res.ok, "lazy renderFrame succeeds");
    check(mgr.isActive(), "manager lazily activated");
    check(mgr.getRenderCount() == 1, "renderCount is 1");

    // Dispose clears renderer_id and deactivates, but does not permanently close
    mgr.dispose();
    check(!mgr.isActive(), "inactive after dispose");
    check(mgr.renderer_id.len == 0, "renderer_id cleared on dispose");
    check(!mgr.closed, "dispose does not set closed");

    const post_dispose = mgr.renderPdf(RenderPdfOptions{});
    check(post_dispose.ok, "renderPdf lazily reactivates after dispose");
    check(mgr.isActive(), "reactivated after dispose");
    check(mgr.getRenderCount() == 2, "renderCount incremented to 2");

    // Close permanently blocks rendering until re-init
    mgr.close();
    check(!mgr.isActive(), "inactive after close");
    check(mgr.closed, "closed flag set");

    const closed_frame = mgr.renderFrame(RenderFrameOptions{});
    check(!closed_frame.ok, "renderFrame fails when closed");
    check(std.mem.eql(u8, closed_frame.error_msg, "renderer closed with its connection"), "closed frame error message");
    check(!mgr.isActive(), "remains inactive after closed renderFrame");

    const closed_pdf = mgr.renderPdf(RenderPdfOptions{});
    check(!closed_pdf.ok, "renderPdf fails when closed");
    check(std.mem.eql(u8, closed_pdf.error_msg, "renderer closed with its connection"), "closed pdf error message");
    check(mgr.getRenderCount() == 2, "closed renders do not increment renderCount");

    // Re-init restores clean active state
    mgr.init("renderer-reinit");
    check(mgr.isActive() && !mgr.closed, "re-init clears closed and activates");
    check(mgr.getRenderCount() == 0 && mgr.frames_rendered == 0 && mgr.getLastRenderMs() == 0, "re-init resets counters");
    check(std.mem.eql(u8, mgr.renderer_id, "renderer-reinit"), "re-init sets new renderer_id");
}

pub fn main() {
    std.debug.print("\n=== OOPIF Render & PageRenderer Unit Tests ===\n", .{});
    testOopifViewportClampingAndBounds();
    testOopifCoordinatorMultiFrameAndRasterize();
    testPageRendererViewportAndMetrics();
    testPageRendererStateTransitionsAndDisposal();
    std.debug.print("\nAll OOPIF Render & PageRenderer tests passed!\n", .{});
}
