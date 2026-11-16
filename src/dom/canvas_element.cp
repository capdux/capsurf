// Migrated from vendor/kitesurf/packages/dom-shim/src/element/canvas.ts
// Implements HTMLCanvasElement dimensions, cached 2D contexts, and bitmap export.

import { NodeId, NULL_NODE } from "./dom.cp";
import { CanvasRenderingContext2D } from "./canvas2d.cp";

const DEFAULT_CANVAS_WIDTH: u32 = 300;
const DEFAULT_CANVAS_HEIGHT: u32 = 150;
const FALLBACK_PNG_DATA_URL = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkAAIAAAoAAv/lxKUAAAAASUVORK5CYII=";
const DATA_URL_PREFIX = "data:image/png;base64,";

pub struct HTMLCanvasElement {
    node_id: NodeId = NULL_NODE,
    width: u32 = DEFAULT_CANVAS_WIDTH,
    height: u32 = DEFAULT_CANVAS_HEIGHT,
    ctx: CanvasRenderingContext2D? = null,
    allocator: std.mem.Allocator,

    static fn init(allocator: std.mem.Allocator, node_id: NodeId): HTMLCanvasElement {
        return HTMLCanvasElement{
            node_id: node_id,
            width: DEFAULT_CANVAS_WIDTH,
            height: DEFAULT_CANVAS_HEIGHT,
            ctx: null,
            allocator: allocator,
        };
    }

    fn deinit(*_) {
        if (_.ctx != null) {
            _.ctx.?.deinit();
            _.ctx = null;
        }
    }

    fn getContext(*_, kind: []const u8): (*CanvasRenderingContext2D)? {
        if (!std.mem.eql(u8, kind, "2d")) {
            return null;
        }
        if (_.ctx == null) {
            let new_ctx = CanvasRenderingContext2D.init(_.allocator, _.width, _.height) catch return null;
            _.ctx = new_ctx;
        }
        if (_.ctx != null) {
            return &_.ctx.?;
        }
        return null;
    }

    fn toDataURL(*_, out_buf: []u8): []const u8 {
        if (*c := _.ctx) {
            let png_bytes = c.encodePng();
            if (png_bytes.len == 0) return FALLBACK_PNG_DATA_URL;

            let encoder = std.base64.standard.Encoder;
            let b64_len = encoder.calcSize(png_bytes.len);
            let total_len = DATA_URL_PREFIX.len + b64_len;

            if (out_buf.len < total_len) return FALLBACK_PNG_DATA_URL;

            @memcpy(out_buf[0..DATA_URL_PREFIX.len], DATA_URL_PREFIX);
            let b64_dest = out_buf[DATA_URL_PREFIX.len..total_len];
            * = encoder.encode(b64_dest, png_bytes);

            return out_buf[0..total_len];
        }
        return FALLBACK_PNG_DATA_URL;
    }
}
