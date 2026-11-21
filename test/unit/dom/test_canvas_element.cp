import { CssColor } from "cap:css";
import { NULL_NODE } from "../../../src/dom/dom.cp";
import {
    MAX_CANVAS_WIDTH,
    MAX_CANVAS_HEIGHT,
} from "../../../src/dom/canvas2d.cp";
import { HTMLCanvasElement } from "../../../src/dom/canvas_element.cp";

const FALLBACK_URL = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkAAIAAAoAAv/lxKUAAAAASUVORK5CYII=";
const DATA_URL_PREFIX = "data:image/png;base64,";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test_canvas_element assertion failed");
    }
}

fn testDimensionsClampingAndContextResolution() {
    let alloc = std.heap.page_allocator;

    // 1.1 Default dimensions (300x150) & unsupported context types
    let default_canvas = HTMLCanvasElement.init(alloc, 10);
    defer default_canvas.deinit();

    assert(default_canvas.node_id == 10, "node_id preserved");
    assert(default_canvas.width == 300, "default width is 300");
    assert(default_canvas.height == 150, "default height is 150");
    assert(default_canvas.ctx == null, "initial ctx is null");

    assert(default_canvas.getContext("webgl") == null, "webgl context unsupported");
    assert(default_canvas.getContext("bitmaprenderer") == null, "bitmaprenderer context unsupported");
    assert(default_canvas.getContext("") == null, "empty context kind returns null");
    assert(default_canvas.ctx == null, "unsupported getContext does not allocate ctx");

    let c2d_a = default_canvas.getContext("2d");
    let c2d_b = default_canvas.getContext("2d");
    assert(c2d_a != null, "2d context resolved");
    assert(c2d_a == c2d_b, "getContext('2d') returns cached context pointer");
    if (c := c2d_a) {
        assert(c.width == 300, "context inherits default width 300");
        assert(c.height == 150, "context inherits default height 150");
    }

    // 1.2 Explicit valid dimensions
    let custom_canvas = HTMLCanvasElement.init(alloc, 11);
    defer custom_canvas.deinit();
    custom_canvas.width = 120;
    custom_canvas.height = 80;
    if (c := custom_canvas.getContext("2d")) {
        assert(c.width == 120, "explicit width 120 applied");
        assert(c.height == 80, "explicit height 80 applied");
    } else {
        assert(false, "custom_canvas getContext('2d') failed");
    }

    // 1.3 Zero and over-max dimensions reset to defaults (300x150)
    let clamped_canvas = HTMLCanvasElement.init(alloc, 12);
    defer clamped_canvas.deinit();
    clamped_canvas.width = 0;
    clamped_canvas.height = MAX_CANVAS_HEIGHT + 1;
    if (c := clamped_canvas.getContext("2d")) {
        assert(c.width == 300, "zero width resets to 300");
        assert(c.height == 150, "over-max height resets to 150");
    } else {
        assert(false, "clamped_canvas getContext('2d') failed");
    }

    let over_w_canvas = HTMLCanvasElement.init(alloc, 13);
    defer over_w_canvas.deinit();
    over_w_canvas.width = MAX_CANVAS_WIDTH + 1;
    over_w_canvas.height = 0;
    if (c := over_w_canvas.getContext("2d")) {
        assert(c.width == 300, "over-max width resets to 300");
        assert(c.height == 150, "zero height resets to 150");
    } else {
        assert(false, "over_w_canvas getContext('2d') failed");
    }
}

fn testResizePixelBufferAndStateLifecycle() {
    let alloc = std.heap.page_allocator;
    let canvas = HTMLCanvasElement.init(alloc, 20);
    defer canvas.deinit();

    canvas.width = 40;
    canvas.height = 40;

    if (c := canvas.getContext("2d")) {
        c.setFillStyle(CssColor.rgb(255, 0, 0));
        c.setLineWidth(6.0);
        c.fillRect(0.0, 0.0, 20.0, 20.0);

        let buf: [4]u8 = [0; 4];
        let px = c.getImageData(5, 5, 1, 1, &buf);
        assert(px.data[0] == 255 && px.data[1] == 0 && px.data[2] == 0 && px.data[3] == 255, "red pixel drawn before resize");
    } else {
        assert(false, "initial 2d context failed");
    }

    // Mutate dimensions and reset backing store via deinit + getContext
    canvas.width = 60;
    canvas.height = 30;
    canvas.deinit();
    assert(canvas.ctx == null, "deinit clears cached ctx");

    if (c_new := canvas.getContext("2d")) {
        assert(c_new.width == 60, "resized context width is 60");
        assert(c_new.height == 30, "resized context height is 30");
        assert(c_new.current_state.line_width == 1.0, "drawing state line_width reset after resize");
        assert(c_new.current_state.fill_style.r == 0, "drawing state fill_style reset after resize");

        let buf_after: [4]u8 = [255; 4];
        let px_after = c_new.getImageData(5, 5, 1, 1, &buf_after);
        assert(px_after.data[0] == 0 && px_after.data[1] == 0 && px_after.data[2] == 0 && px_after.data[3] == 0, "pixel buffer cleared to transparent black after resize");
    } else {
        assert(false, "resized 2d context failed");
    }
}

fn testToDataUrlAndFallbacks() {
    let alloc = std.heap.page_allocator;
    let canvas = HTMLCanvasElement.init(alloc, 30);
    defer canvas.deinit();

    canvas.width = 16;
    canvas.height = 16;

    // 3.1 Without an active context, toDataURL returns the 1x1 fallback PNG data URL
    let out_buf: [16384]u8 = [0; 16384];
    let no_ctx_url = canvas.toDataURL(&out_buf);
    assert(std.mem.eql(u8, no_ctx_url, FALLBACK_URL), "no-context toDataURL returns fallback PNG");

    // 3.2 With active context and drawing
    if (c := canvas.getContext("2d")) {
        c.setFillStyle(CssColor.rgb(10, 200, 100));
        c.fillRect(0.0, 0.0, 16.0, 16.0);
    } else {
        assert(false, "getContext('2d') failed for toDataURL");
    }

    // 3.3 Insufficient output buffer falls back gracefully
    let tiny_buf: [12]u8 = [0; 12];
    let tiny_url = canvas.toDataURL(&tiny_buf);
    assert(std.mem.eql(u8, tiny_url, FALLBACK_URL), "undersized buffer falls back to 1x1 PNG");

    // 3.4 Valid output buffer produces PNG base64 data URL with decodable PNG header
    let data_url = canvas.toDataURL(&out_buf);
    assert(std.mem.startsWith(u8, data_url, DATA_URL_PREFIX), "toDataURL starts with data:image/png;base64,");
    assert(!std.mem.eql(u8, data_url, FALLBACK_URL), "drawn canvas toDataURL differs from 1x1 fallback");

    let b64_payload = data_url[DATA_URL_PREFIX.len..];
    assert(b64_payload.len > 16, "non-empty base64 payload");

    let decoder = std.base64.standard.Decoder;
    let decoded_len = decoder.calcSizeForSlice(b64_payload) catch {
        assert(false, "calcSizeForSlice failed on base64 payload");
        return;
    };
    assert(decoded_len > 8, "decoded PNG has header and body");

    let decoded_buf: [8192]u8 = [0; 8192];
    decoder.decode(decoded_buf[0..decoded_len], b64_payload) catch {
        assert(false, "base64 decode failed");
        return;
    };
    assert(
        decoded_buf[0] == 0x89 && decoded_buf[1] == 'P' && decoded_buf[2] == 'N' && decoded_buf[3] == 'G',
        "decoded toDataURL bytes start with PNG magic header",
    );
}

fn testMultiCanvasIsolationAndCleanup() {
    let alloc = std.heap.page_allocator;

    let canvas_a = HTMLCanvasElement.init(alloc, NULL_NODE);
    let canvas_b = HTMLCanvasElement.init(alloc, 101);
    let canvas_c = HTMLCanvasElement.init(alloc, 102);

    canvas_a.width = 20;
    canvas_a.height = 20;
    canvas_b.width = 32;
    canvas_b.height = 32;

    if (ca := canvas_a.getContext("2d")) {
        ca.setFillStyle(CssColor.rgb(255, 0, 0));
        ca.fillRect(0.0, 0.0, 20.0, 20.0);
    }
    if (cb := canvas_b.getContext("2d")) {
        cb.setFillStyle(CssColor.rgb(0, 0, 255));
        cb.fillRect(0.0, 0.0, 32.0, 32.0);
    }

    // Cleanup canvas_b and unused canvas_c (idempotent deinit)
    canvas_b.deinit();
    canvas_b.deinit();
    canvas_c.deinit();

    assert(canvas_b.ctx == null, "canvas_b ctx cleared after deinit");
    assert(canvas_c.ctx == null, "canvas_c ctx remains null after deinit");

    // Verify canvas_a remains intact and isolated
    if (ca := canvas_a.getContext("2d")) {
        let px_buf: [4]u8 = [0; 4];
        let px = ca.getImageData(10, 10, 1, 1, &px_buf);
        assert(px.data[0] == 255 && px.data[1] == 0 && px.data[2] == 0 && px.data[3] == 255, "canvas_a pixels isolated from canvas_b deinit");
    } else {
        assert(false, "canvas_a context lost");
    }

    let url_buf: [16384]u8 = [0; 16384];
    let url_a = canvas_a.toDataURL(&url_buf);
    assert(std.mem.startsWith(u8, url_a, DATA_URL_PREFIX), "canvas_a still exports valid PNG data URL");

    canvas_a.deinit();
    assert(canvas_a.ctx == null, "canvas_a ctx cleared after final deinit");
}

pub fn main() {
    testDimensionsClampingAndContextResolution();
    testResizePixelBufferAndStateLifecycle();
    testToDataUrlAndFallbacks();
    testMultiCanvasIsolationAndCleanup();
    std.debug.print("test_canvas_element passed all assertions!\n", .{});
}
