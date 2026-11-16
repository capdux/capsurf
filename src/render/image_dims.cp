// Migrated from vendor/kitesurf/packages/engine/src/eval/image-dims.ts
// CapSurf image dimensions sniffing and SVG marker detection directly from raw response bytes.

pub struct ImageDims {
    width: u32 = 0,
    height: u32 = 0,
}

pub const MAX_IMAGE_DIMENSION: u32 = 65535;
pub const MAX_SVG_SNIFF_BYTES: usize = 2048;
pub const MAX_ISOBMFF_SCAN_BYTES: usize = 64 * 1024;

fn u16be(b: []const u8, at: usize): u16 {
    if (at + 2 > b.len) return 0;
    return (@as(u16, b[at]) << 8) | @as(u16, b[at + 1]);
}

fn u32be(b: []const u8, at: usize): u32 {
    if (at + 4 > b.len) return 0;
    return (@as(u32, b[at]) << 24) |
           (@as(u32, b[at + 1]) << 16) |
           (@as(u32, b[at + 2]) << 8) |
           @as(u32, b[at + 3]);
}

fn u16le(b: []const u8, at: usize): u16 {
    if (at + 2 > b.len) return 0;
    return @as(u16, b[at]) | (@as(u16, b[at + 1]) << 8);
}

fn u32le(b: []const u8, at: usize): u32 {
    if (at + 4 > b.len) return 0;
    return @as(u32, b[at]) |
           (@as(u32, b[at + 1]) << 8) |
           (@as(u32, b[at + 2]) << 16) |
           (@as(u32, b[at + 3]) << 24);
}

pub fn sniffPng(b: []const u8): ImageDims? {
    if (b.len < 24) return null;
    if (b[0] != 0x89 || b[1] != 'P' || b[2] != 'N' || b[3] != 'G' ||
        b[4] != '\r' || b[5] != '\n' || b[6] != 0x1a || b[7] != '\n')
    {
        return null;
    }
    if (!std.mem.eql(u8, b[12..16], "IHDR")) return null;
    const w = u32be(b, 16);
    const h = u32be(b, 20);
    return ImageDims{ width: w, height: h };
}

pub fn sniffGif(b: []const u8): ImageDims? {
    if (b.len < 10) return null;
    if (!std.mem.startsWith(u8, b, "GIF")) return null;
    const w: u32 = u16le(b, 6);
    const h: u32 = u16le(b, 8);
    return ImageDims{ width: w, height: h };
}

pub fn sniffBmp(b: []const u8): ImageDims? {
    if (b.len < 26) return null;
    if (b[0] != 'B' || b[1] != 'M') return null;
    const header_size = u32le(b, 14);
    if (header_size == 12) {
        return ImageDims{ width: u16le(b, 18), height: u16le(b, 20) };
    }
    const raw_w = @as(i32, @bitCast(u32le(b, 18)));
    const raw_h = @as(i32, @bitCast(u32le(b, 22)));
    const w: u32 = if (raw_w < 0) @intCast(-raw_w) else @intCast(raw_w);
    const h: u32 = if (raw_h < 0) @intCast(-raw_h) else @intCast(raw_h);
    return ImageDims{ width: w, height: h };
}

pub fn sniffJpeg(b: []const u8): ImageDims? {
    if (b.len < 4 || b[0] != 0xff || b[1] != 0xd8) return null;
    let at: usize = 2;
    while (at + 4 <= b.len) {
        if (b[at] != 0xff) {
            at += 1;
            continue;
        }
        const marker = b[at + 1];
        if (marker == 0xd8 || marker == 0x01 || (marker >= 0xd0 && marker <= 0xd7)) {
            at += 2;
            continue;
        }
        if (marker == 0xda || marker == 0xd9) return null;
        const len = u16be(b, at + 2);
        if (len < 2) return null;
        const is_sof = (marker >= 0xc0 && marker <= 0xcf && marker != 0xc4 && marker != 0xc8 && marker != 0xcc);
        if (is_sof) {
            if (at + 9 > b.len) return null;
            const h: u32 = u16be(b, at + 5);
            const w: u32 = u16be(b, at + 7);
            return ImageDims{ width: w, height: h };
        }
        at += 2 + @as(usize, len);
    }
    return null;
}

pub fn sniffWebp(b: []const u8): ImageDims? {
    if (b.len < 30) return null;
    if (!std.mem.startsWith(u8, b, "RIFF")) return null;
    if (!std.mem.eql(u8, b[8..12], "WEBP")) return null;

    const chunk = b[12..16];
    if (std.mem.eql(u8, chunk, "VP8X")) {
        const w: u32 = (@as(u32, b[24]) | (@as(u32, b[25]) << 8) | (@as(u32, b[26]) << 16)) + 1;
        const h: u32 = (@as(u32, b[27]) | (@as(u32, b[28]) << 8) | (@as(u32, b[29]) << 16)) + 1;
        return ImageDims{ width: w, height: h };
    }
    if (std.mem.eql(u8, chunk, "VP8L")) {
        if (b[20] != 0x2f) return null;
        const bits: u32 = u32le(b, 21);
        const w: u32 = (bits & 0x3fff) + 1;
        const h: u32 = ((bits >> 14) & 0x3fff) + 1;
        return ImageDims{ width: w, height: h };
    }
    if (std.mem.eql(u8, chunk, "VP8 ")) {
        if (b[23] != 0x9d || b[24] != 0x01 || b[25] != 0x2a) return null;
        const w: u32 = @as(u32, u16le(b, 26)) & 0x3fff;
        const h: u32 = @as(u32, u16le(b, 28)) & 0x3fff;
        return ImageDims{ width: w, height: h };
    }
    return null;
}

pub fn sniffIsobmff(b: []const u8): ImageDims? {
    if (b.len < 12) return null;
    if (!std.mem.eql(u8, b[4..8], "ftyp")) return null;
    const brand = b[8..12];
    if (!std.mem.eql(u8, brand, "avif") &&
        !std.mem.eql(u8, brand, "avis") &&
        !std.mem.eql(u8, brand, "heic") &&
        !std.mem.eql(u8, brand, "heix") &&
        !std.mem.eql(u8, brand, "mif1") &&
        !std.mem.eql(u8, brand, "msf1"))
    {
        return null;
    }
    const limit: usize = if (b.len - 12 < MAX_ISOBMFF_SCAN_BYTES) b.len - 12 else MAX_ISOBMFF_SCAN_BYTES;
    let at: usize = 12;
    while (at + 16 <= b.len && at < limit) : (at += 1) {
        if (!std.mem.eql(u8, b[at..at + 4], "ispe")) continue;
        const w = u32be(b, at + 8);
        const h = u32be(b, at + 12);
        return ImageDims{ width: w, height: h };
    }
    return null;
}

fn isXmlSpace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn matchesAscii(bytes: []const u8, at: usize, needle: []const u8): bool {
    if (at + needle.len > bytes.len) return false;
    return std.mem.eql(u8, bytes[at..at + needle.len], needle);
}

fn indexOfAscii(bytes: []const u8, needle: []const u8, start_idx: usize, limit: usize): isize {
    const end = if (limit < bytes.len) limit else bytes.len;
    let i: usize = start_idx;
    while (i + needle.len <= end) : (i += 1) {
        if (matchesAscii(bytes, i, needle)) return @as(isize, @intCast(i));
    }
    return -1;
}

fn indexOfByte(bytes: []const u8, byte: u8, start_idx: usize, limit: usize): isize {
    const end = if (limit < bytes.len) limit else bytes.len;
    let i: usize = start_idx;
    while (i < end) : (i += 1) {
        if (bytes[i] == byte) return @as(isize, @intCast(i));
    }
    return -1;
}

fn isSvgRootName(bytes: []const u8, at: usize, limit: usize): bool {
    let end = at;
    while (end < limit && end < bytes.len) {
        const b = bytes[end];
        if (isXmlSpace(b) || b == '/' || b == '>') break;
        end += 1;
    }
    if (end == at || end > bytes.len) return false;
    const name = bytes[at..end];
    if (std.ascii.eqlIgnoreCase(name, "svg")) return true;
    if (name.len > 4 && std.ascii.eqlIgnoreCase(name[name.len - 4..], ":svg")) return true;
    return false;
}

pub fn looksLikeSvg(bytes: []const u8): bool {
    const limit = if (bytes.len < MAX_SVG_SNIFF_BYTES) bytes.len else MAX_SVG_SNIFF_BYTES;
    let i: usize = 0;
    if (bytes.len >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb && bytes[2] == 0xbf) {
        i = 3;
    }
    while (true) {
        while (i < limit && isXmlSpace(bytes[i])) : (i += 1) {}
        if (i >= limit || bytes[i] != '<') return false;
        if (i + 1 >= limit) return false;
        const next = bytes[i + 1];
        if (next == '?') {
            const end = indexOfAscii(bytes, "?>", i + 2, limit);
            if (end < 0) return false;
            i = @as(usize, @intCast(end)) + 2;
            continue;
        }
        if (next == '!') {
            if (matchesAscii(bytes, i + 2, "--")) {
                const end = indexOfAscii(bytes, "-->", i + 4, limit);
                if (end < 0) return false;
                i = @as(usize, @intCast(end)) + 3;
                continue;
            }
            let at = i + 2;
            const open = indexOfAscii(bytes, "[", at, limit);
            if (open >= 0) {
                const close = indexOfAscii(bytes, "]", @as(usize, @intCast(open)) + 1, limit);
                if (close < 0) return false;
                at = @as(usize, @intCast(close)) + 1;
            }
            const end = indexOfByte(bytes, '>', at, limit);
            if (end < 0) return false;
            i = @as(usize, @intCast(end)) + 1;
            continue;
        }
        return isSvgRootName(bytes, i + 1, limit);
    }
}

pub fn sniffImageDimensions(b: []const u8): ImageDims? {
    let dims: ImageDims? = sniffPng(b);
    if (dims == null) dims = sniffGif(b);
    if (dims == null) dims = sniffJpeg(b);
    if (dims == null) dims = sniffWebp(b);
    if (dims == null) dims = sniffBmp(b);
    if (dims == null) dims = sniffIsobmff(b);

    if (d := dims) {
        if (d.width == 0 || d.height == 0 || d.width > MAX_IMAGE_DIMENSION || d.height > MAX_IMAGE_DIMENSION) {
            return null;
        }
        return d;
    }
    return null;
}

pub fn sniffImageDims(b: []const u8): ImageDims? {
    return sniffImageDimensions(b);
}
