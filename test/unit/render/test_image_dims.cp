// test/render/test_image_dims.cp
// CapSurf Phase 10 Comprehensive Image Dimensions Sniffing & CookieJar Partitioning Suite

import { test, expect_eq } from "cap:test";
import {
    ImageDims,
    sniffPng,
    sniffGif,
    sniffBmp,
    sniffJpeg,
    sniffWebp,
    sniffImageDimensions,
    looksLikeSvg,
} from "../../../src/render/image_dims.cp";
import {
    CookieJarRegistry,
    makeKey,
    registerCookieJar,
    getCookieJar,
    unregisterCookieJar,
    unregisterConnection,
} from "../../../src/net/cookie_jar_registry.cp";
import { CookieJar } from "../../../src/net/cookies.cp";

fn testImageSniffing()! {
    // 1. PNG Sniffing (100x200)
    const png_data = "\x89PNG\r\n\x1a\n\x00\x00\x00\x0dIHDR\x00\x00\x00\x64\x00\x00\x00\xc8\x08\x06\x00\x00\x00";
    const png_dims = sniffPng(png_data);
    expect_eq(png_dims != null, true);
    if (d := png_dims) {
        expect_eq(d.width, 100);
        expect_eq(d.height, 200);
    }

    const bad_png = "\x89PNG\r\n\x1a\n\x00\x00\x00\x0dNOTH\x00\x00\x00\x64\x00\x00\x00\xc8";
    expect_eq(sniffPng(bad_png) == null, true);

    // 2. GIF Sniffing (320x240)
    const gif_data = "GIF89a\x40\x01\xf0\x00";
    const gif_dims = sniffGif(gif_data);
    expect_eq(gif_dims != null, true);
    if (d := gif_dims) {
        expect_eq(d.width, 320);
        expect_eq(d.height, 240);
    }

    // 3. BMP Sniffing (64x48)
    const bmp_data = "BM\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x28\x00\x00\x00\x40\x00\x00\x00\x30\x00\x00\x00";
    const bmp_dims = sniffBmp(bmp_data);
    expect_eq(bmp_dims != null, true);
    if (d := bmp_dims) {
        expect_eq(d.width, 64);
        expect_eq(d.height, 48);
    }

    // 4. JPEG Sniffing (300x150)
    const jpeg_data = "\xff\xd8\xff\xc0\x00\x11\x08\x00\x96\x01\x2c\x03\x01\x11\x00\x02\x11\x01\x03\x11\x01";
    const jpeg_dims = sniffJpeg(jpeg_data);
    expect_eq(jpeg_dims != null, true);
    if (d := jpeg_dims) {
        expect_eq(d.width, 300);
        expect_eq(d.height, 150);
    }

    // 5. WebP VP8X Sniffing (200x100)
    const webp_data = "RIFF\x20\x00\x00\x00WEBPVP8X\x0a\x00\x00\x00\x00\x00\x00\x00\xc7\x00\x00\x63\x00\x00";
    const webp_dims = sniffWebp(webp_data);
    expect_eq(webp_dims != null, true);
    if (d := webp_dims) {
        expect_eq(d.width, 200);
        expect_eq(d.height, 100);
    }

    // 6. SVG Detection
    const svg_str = "  <?xml version=\"1.0\"?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\"></svg>";
    expect_eq(looksLikeSvg(svg_str), true);
    expect_eq(looksLikeSvg("<html><body>Not SVG</body></html>"), false);
}

fn testCookieJarMultiTenantPartitioning()! {
    let reg = CookieJarRegistry.init();
    let buf: [128]u8 = [0; 128];

    const key1 = CookieJarRegistry.makeKey(buf[0..], "conn-1", "");
    expect_eq(std.mem.eql(u8, key1, "conn-1"), true);

    const key2 = CookieJarRegistry.makeKey(buf[0..], "conn-1", "ctx-A");
    expect_eq(std.mem.eql(u8, key2, "conn-1::ctx-A"), true);

    const jar1 = CookieJar.init(std.heap.page_allocator);
    const jar2 = CookieJar.init(std.heap.page_allocator);

    expect_eq(reg.registerCookieJar(key1, jar1), true);
    expect_eq(reg.registerCookieJar(key2, jar2), true);
    expect_eq(reg.size(), 2);

    expect_eq(reg.getCookieJar(key1) != null, true);
    expect_eq(reg.getCookieJar(key2) != null, true);
    expect_eq(reg.getCookieJar("conn-nonexistent") == null, true);

    // Unregister connection cleans both conn-1 and conn-1::ctx-A
    reg.unregisterConnection("conn-1");
    expect_eq(reg.size(), 0);
    expect_eq(reg.getCookieJar(key1) == null, true);
    expect_eq(reg.getCookieJar(key2) == null, true);
}

fn main() {
    test("1. Zero-Allocation Fast Image Dimensions Sniffing (PNG/GIF/BMP/JPEG/WebP/SVG)", () => {
        testImageSniffing() catch unreachable;
    });

    test("2. CookieJar Multi-Tenant Connection & BrowserContext Partitioning", () => {
        testCookieJarMultiTenantPartitioning() catch unreachable;
    });
}
