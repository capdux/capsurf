// src/cli/options.cp
// Command-line argument parsing and configuration for CapSurf
// Emulates standard Chromium CLI flags for Playwright, Puppeteer, and AI Agent compatibility

import { extractJsonInt } from "../cdp/types.cp";
import { open, read, close } from "../cdp/server.cp";

pub struct CliOptions {
    port: u16 = 9222,
    address: []const u8 = "127.0.0.1",
    headless: bool = true,
    window_width: u32 = 1280,
    window_height: u32 = 720,
    user_data_dir: []const u8 = "",
    url: []const u8 = "about:blank",
    show_help: bool = false,
    show_version: bool = false,
    is_demo: bool = false,

    static fn parseFromArgs(args: []const []const u8): CliOptions {
        let opts = CliOptions{};
        let i: usize = 0;
        while (i < args.len) : (i += 1) {
            const arg = args[i];
            if (arg.len == 0) continue;

            if (std.mem.eql(u8, arg, "--demo")) {
                opts.is_demo = true;
                continue;
            }

            if (std.mem.eql(u8, arg, "--help") || std.mem.eql(u8, arg, "-h")) {
                opts.show_help = true;
                continue;
            }
            if (std.mem.eql(u8, arg, "--version") || std.mem.eql(u8, arg, "-v")) {
                opts.show_version = true;
                continue;
            }
            if (std.mem.startsWith(u8, arg, "--remote-debugging-port=")) {
                const val_str = arg["--remote-debugging-port=".len..];
                if (p := std.fmt.parseInt(u16, val_str, 10) catch null) {
                    opts.port = p;
                }
                continue;
            }
            if (std.mem.eql(u8, arg, "--remote-debugging-port")) {
                if (i + 1 < args.len) {
                    i += 1;
                    if (p := std.fmt.parseInt(u16, args[i], 10) catch null) {
                        opts.port = p;
                    }
                }
                continue;
            }
            if (std.mem.startsWith(u8, arg, "--remote-debugging-address=")) {
                opts.address = arg["--remote-debugging-address=".len..];
                continue;
            }
            if (std.mem.startsWith(u8, arg, "--window-size=")) {
                const val_str = arg["--window-size=".len..];
                if (comma := std.mem.indexOfScalar(u8, val_str, ',')) {
                    const w_str = val_str[0..comma];
                    const h_str = val_str[comma + 1..];
                    const w = std.fmt.parseInt(u32, w_str, 10) catch 1280;
                    const h = std.fmt.parseInt(u32, h_str, 10) catch 720;
                    opts.window_width = w;
                    opts.window_height = h;
                }
                continue;
            }
            if (std.mem.startsWith(u8, arg, "--user-data-dir=")) {
                opts.user_data_dir = arg["--user-data-dir=".len..];
                continue;
            }
            if (std.mem.eql(u8, arg, "--headless") || std.mem.startsWith(u8, arg, "--headless=")) {
                opts.headless = true;
                continue;
            }
            // Safely ignored standard Chromium flags:
            if (std.mem.eql(u8, arg, "--no-sandbox") ||
                std.mem.eql(u8, arg, "--disable-gpu") ||
                std.mem.eql(u8, arg, "--disable-setuid-sandbox") ||
                std.mem.eql(u8, arg, "--disable-dev-shm-usage") ||
                std.mem.eql(u8, arg, "--no-first-run")) {
                continue;
            }
            // Positional argument: URL
            if (!std.mem.startsWith(u8, arg, "-")) {
                opts.url = arg;
            }
        }
        return opts;
    }

    /// Read CLI options directly from Linux `/proc/self/cmdline`
    static fn parseFromProcess(): CliOptions {
        const fd = open("/proc/self/cmdline", 0);
        if (fd < 0) return CliOptions{};
        defer * = close(fd);

        let buf: [4096]u8 = [0; 4096];
        const bytes_read = read(fd, &buf, buf.len);
        if (bytes_read <= 0) return CliOptions{};
        const u_bytes_read: usize = @intCast(bytes_read);

        let args_arr: [64][]const u8 = [""; 64];
        let arg_count: usize = 0;

        let start: usize = 0;
        let idx: usize = 0;
        while (idx < u_bytes_read && arg_count < args_arr.len) : (idx += 1) {
            if (buf[idx] == 0) {
                if (idx > start) {
                    args_arr[arg_count] = buf[start..idx];
                    arg_count += 1;
                }
                start = idx + 1;
            }
        }

        // Skip argv[0] (program binary name) if more than 1 arg
        if (arg_count > 1) {
            return CliOptions.parseFromArgs(args_arr[1..arg_count]);
        }
        return CliOptions{};
    }
}
