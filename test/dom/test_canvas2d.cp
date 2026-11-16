import { CssColor } from "cap:css";
import {
    CanvasRenderingContext2D,
    ImageData,
    Point,
} from "../../src/dom/canvas2d.cp";
import { HTMLCanvasElement } from "../../src/dom/canvas_element.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. Test CanvasRenderingContext2D State & Direct Drawing
    {
        let ctx = CanvasRenderingContext2D.init(std.heap.page_allocator, 100, 100) catch {
            @panic("Failed to init canvas context");
        };
        defer ctx.deinit();

        assert(ctx.width == 100, "width is 100");
        assert(ctx.height == 100, "height is 100");

        // 1.1 State save and restore
        ctx.setFillStyle(CssColor.rgb(255, 0, 0));
        ctx.setLineWidth(4.0);
        ctx.save();

        ctx.setFillStyle(CssColor.rgb(0, 0, 255));
        ctx.setLineWidth(10.0);
        assert(ctx.current_state.line_width == 10.0, "line width updated to 10");

        ctx.restore();
        assert(ctx.current_state.line_width == 4.0, "line width restored to 4");
        assert(ctx.current_state.fill_style.r == 255, "fill style red restored");

        // 1.2 Fill rect and pixel inspection
        ctx.setFillStyle(CssColor.rgb(255, 128, 0));
        ctx.fillRect(10.0, 10.0, 20.0, 20.0);

        let buf: [400]u8 = [0; 400]; // 10x10 * 4
        let img = ctx.getImageData(10, 10, 10, 10, &buf);
        assert(img.width == 10 && img.height == 10, "read 10x10 image data");
        // Verify pixel color inside filled rect (255, 128, 0, 255)
        assert(img.data[0] == 255, "red channel is 255");
        assert(img.data[1] == 128, "green channel is 128");
        assert(img.data[2] == 0, "blue channel is 0");
        assert(img.data[3] == 255, "alpha channel is 255");

        // 1.3 Clear rect
        ctx.clearRect(10.0, 10.0, 5.0, 5.0);
        let clear_img = ctx.getImageData(10, 10, 5, 5, &buf);
        assert(clear_img.data[3] == 0, "cleared pixel alpha is 0");

        // 1.4 Path drawing (moveTo / lineTo / stroke)
        ctx.beginPath();
        ctx.setStrokeStyle(CssColor.rgb(0, 255, 0));
        ctx.moveTo(30.0, 30.0);
        ctx.lineTo(30.0, 50.0);
        ctx.stroke();

        let line_img = ctx.getImageData(30, 40, 1, 1, &buf);
        assert(line_img.data[1] == 255, "green line drawn at y=40");

        // 1.5 PutImageData
        let put_buf: [16]u8 = [
            200, 100, 50, 255,
            200, 100, 50, 255,
            200, 100, 50, 255,
            200, 100, 50, 255,
        ];
        let put_data = ImageData{
            width: 2,
            height: 2,
            data: put_buf[0..],
        };
        ctx.putImageData(put_data, 60, 60);
        let verify_put = ctx.getImageData(60, 60, 1, 1, &buf);
        assert(verify_put.data[0] == 200, "putImageData red channel 200");
        assert(verify_put.data[1] == 100, "putImageData green channel 100");

        // 1.6 PNG encoding
        let png = ctx.encodePng();
        assert(png.len > 8, "png bytes generated");
        assert(png[0] == 0x89 && png[1] == 'P' && png[2] == 'N' && png[3] == 'G', "PNG magic header present");
    }

    // 2. Test HTMLCanvasElement DOM Integration
    {
        let canvas = HTMLCanvasElement.init(std.heap.page_allocator, 42);
        defer canvas.deinit();

        assert(canvas.width == 300, "default width 300");
        assert(canvas.height == 150, "default height 150");

        // 2.1 Context resolution
        assert(canvas.getContext("webgl") == null, "unsupported context returns null");

        let c2d = canvas.getContext("2d");
        assert(c2d != null, "2d context resolved");

        if (c := c2d) {
            c.setFillStyle(CssColor.rgb(0, 128, 255));
            c.fillRect(0.0, 0.0, 50.0, 50.0);
        }

        // 2.2 Data URL export
        let data_url_buf: [16384]u8 = [0; 16384];
        let data_url = canvas.toDataURL(&data_url_buf);
        assert(std.mem.startsWith(u8, data_url, "data:image/png;base64,"), "data url prefix matches");
        assert(data_url.len > 30, "non-trivial base64 image payload generated");
    }

    std.debug.print("test_canvas2d passed all assertions!\n", .{});
}
