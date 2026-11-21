// test/cdp/test_cli_options.cp
// CapSurf Phase 8 Comprehensive CLI Options & Arguments Test Suite

import { test, expect_eq } from "cap:test";
import { CliOptions } from "../../../src/cli/options.cp";

fn testCliDefaults()! {
    let empty_args: [][]const u8 = [];
    const opts = CliOptions.parseFromArgs(empty_args);
    expect_eq(opts.port, 9222);
    expect_eq(opts.headless, true);
    expect_eq(opts.window_width, 1280);
    expect_eq(opts.window_height, 720);
    expect_eq(std.mem.eql(u8, opts.address, "127.0.0.1"), true);
    expect_eq(std.mem.eql(u8, opts.url, "about:blank"), true);
    expect_eq(opts.show_help, false);
    expect_eq(opts.show_version, false);
}

fn testCliChromiumFlags()! {
    let raw_args: [9][]const u8 = [
        "--remote-debugging-port=19888",
        "--remote-debugging-address=0.0.0.0",
        "--window-size=1920,1080",
        "--user-data-dir=/tmp/capsurf-profile",
        "--headless=new",
        "--no-sandbox",
        "--disable-gpu",
        "--disable-dev-shm-usage",
        "https://example.com/login",
    ];

    const opts = CliOptions.parseFromArgs(raw_args[0..]);
    expect_eq(opts.port, 19888);
    expect_eq(std.mem.eql(u8, opts.address, "0.0.0.0"), true);
    expect_eq(opts.window_width, 1920);
    expect_eq(opts.window_height, 1080);
    expect_eq(std.mem.eql(u8, opts.user_data_dir, "/tmp/capsurf-profile"), true);
    expect_eq(opts.headless, true);
    expect_eq(std.mem.eql(u8, opts.url, "https://example.com/login"), true);
}

fn testCliSeparatePortArg()! {
    let raw_args: [3][]const u8 = [
        "--remote-debugging-port",
        "9333",
        "--version",
    ];

    const opts = CliOptions.parseFromArgs(raw_args[0..]);
    expect_eq(opts.port, 9333);
    expect_eq(opts.show_version, true);
}

fn main() {
    test("1. CLI Options Default Configuration", () => {
        testCliDefaults() catch unreachable;
    });

    test("2. CLI Options Standard Chromium Flags Parsing", () => {
        testCliChromiumFlags() catch unreachable;
    });

    test("3. CLI Options Separate Port Argument & Version", () => {
        testCliSeparatePortArg() catch unreachable;
    });
}
