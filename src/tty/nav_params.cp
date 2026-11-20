// Migrated from vendor/kitesurf/packages/engine/src/tty/nav-params.ts
// and vendor/kitesurf/packages/engine/src/tty/stylesheet.ts
// Parses terminal style, cell-size, and color-scheme options and generates TTY stylesheets.

pub const TTY_CELL_PX_MIN: u32 = 1;
pub const TTY_CELL_PX_MAX: u32 = 64;
pub const DEFAULT_TTY_CELL_WIDTH_PX: u32 = 8;
pub const DEFAULT_TTY_CELL_HEIGHT_PX: u32 = 17;

pub struct TtyNavState {
    tty_ua_css: u32? = null,
    tty_cell_w: u32? = null,
    tty_cell_h: u32? = null,
    dark_color_scheme: bool? = null,
}

pub fn parseCell(value: []const u8, out_w: *u32, out_h: *u32): bool {
    let x_idx_opt = std.mem.indexOf(u8, value, "x");
    if (x_idx_opt == null) return false;
    let x_idx = x_idx_opt.?;
    if (x_idx == 0 || x_idx + 1 >= value.len) return false;

    let w_part = value[0..x_idx];
    let h_part = value[x_idx + 1 ..];

    let w_opt = parseU32(w_part);
    if (w_opt == null) return false;
    let w = w_opt.?;

    let h_opt = parseU32(h_part);
    if (h_opt == null) return false;
    let h = h_opt.?;

    if (w < TTY_CELL_PX_MIN || w > TTY_CELL_PX_MAX) return false;
    if (h < TTY_CELL_PX_MIN || h > TTY_CELL_PX_MAX) return false;

    out_w.* = w;
    out_h.* = h;
    return true;
}

fn parseU32(s: []const u8): u32? {
    if (s.len == 0) return null;
    let val: u32 = 0;
    for (i in 0..s.len) {
        let ch = s[i];
        if (ch < '0' || ch > '9') return null;
        val = val * 10 + @as(u32, ch - '0');
    }
    return val;
}

pub fn applyTtyNavParams(query: []const u8, state: *TtyNavState): bool {
    let seen = false;
    let pos: usize = 0;
    // Strip leading '?' if present
    if (query.len > 0 && query[0] == '?') {
        pos = 1;
    }

    while (pos < query.len) {
        let amp_opt = std.mem.indexOf(u8, query[pos..], "&");
        let end = query.len;
        if (a := amp_opt) {
            end = pos + a;
        }
        let pair = query[pos..end];
        pos = end + 1;

        if (eq_opt := std.mem.indexOf(u8, pair, "=")) {
            let key = pair[0..eq_opt];
            let val = pair[eq_opt + 1 ..];

            if (std.mem.eql(u8, key, "ks_tty_css")) {
                seen = true;
                if (level := parseU32(val)) {
                    state.tty_ua_css = if (level > 2) 2 else level;
                } else {
                    state.tty_ua_css = 0;
                }
            } else if (std.mem.eql(u8, key, "ks_color_scheme")) {
                seen = true;
                if (std.mem.eql(u8, val, "dark")) {
                    state.dark_color_scheme = true;
                } else if (std.mem.eql(u8, val, "light")) {
                    state.dark_color_scheme = false;
                }
            } else if (std.mem.eql(u8, key, "ks_tty_cell")) {
                seen = true;
                let w: u32 = 0;
                let h: u32 = 0;
                if (parseCell(val, &w, &h)) {
                    state.tty_cell_w = w;
                    state.tty_cell_h = h;
                }
            }
        }
    }
    return seen;
}

pub fn ttyFocusStylesheet(): []const u8 {
    return ":focus { outline: 1px dotted currentColor !important; }";
}

pub fn ttyUaStylesheet(
    level: u32,
    cell_h: u32,
    cell_w: u32,
    out_buf: []u8,
): []const u8 {
    let base_css = 
        ":root {\n" ++
        "  font-family: monospace !important;\n" ++
        "  line-height: 1 !important;\n" ++
        "}\n" ++
        "input[type=\"checkbox\"][class*=\"toggle-all\" i] + label {\n" ++
        "  font-size: 0 !important;\n" ++
        "}\n" ++
        "input[type=\"checkbox\"][class*=\"toggle-all\" i] + label::before {\n" ++
        "  content: none !important;\n" ++
        "}\n" ++
        "input:not([type=\"checkbox\"]):not([type=\"radio\"]).new-todo {\n" ++
        "  padding-inline-start: 3ch !important;\n" ++
        "}\n";

    let prefix = ":root {\n  --ks-tty-cell-w: ";
    let mid1 = "px !important;\n  --ks-tty-cell-h: ";
    let mid2 = "px !important;\n}\n";
    let level2_css = "body { margin: 0 !important; overflow: hidden !important; }\n";

    let pos: usize = 0;
    if (pos + base_css.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + base_css.len], base_css);
    pos += base_css.len;

    if (pos + prefix.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + prefix.len], prefix);
    pos += prefix.len;

    let w_buf: [16]u8 = [0; 16];
    let w_str = formatU32(&w_buf, cell_w);
    if (pos + w_str.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + w_str.len], w_str);
    pos += w_str.len;

    if (pos + mid1.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + mid1.len], mid1);
    pos += mid1.len;

    let h_buf: [16]u8 = [0; 16];
    let h_str = formatU32(&h_buf, cell_h);
    if (pos + h_str.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + h_str.len], h_str);
    pos += h_str.len;

    if (pos + mid2.len > out_buf.len) return "";
    @memcpy(out_buf[pos .. pos + mid2.len], mid2);
    pos += mid2.len;

    if (level >= 2) {
        if (pos + level2_css.len > out_buf.len) return "";
        @memcpy(out_buf[pos .. pos + level2_css.len], level2_css);
        pos += level2_css.len;
    }

    return out_buf[0..pos];
}

fn formatU32(buf: *[16]u8, val: u32): []const u8 {
    if (val == 0) {
        buf[0] = '0';
        return buf[0..1];
    }
    let v = val;
    let temp: [16]u8 = [0; 16];
    let digits: usize = 0;
    while (v > 0) {
        temp[digits] = @as(u8, @intCast(v % 10)) + '0';
        v = v / 10;
        digits += 1;
    }
    let j: usize = 0;
    while (j < digits) : (j += 1) {
        buf[j] = temp[digits - 1 - j];
    }
    return buf[0..digits];
}
