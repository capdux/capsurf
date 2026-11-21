// test/cdp/test_runtime_call_function_on.cp
// CapSurf CDP Runtime RemoteObject & callFunctionOn Unit Tests

import { CdpPageSession } from "../../../src/cdp/session.cp";

fn check(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main()! {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    // 1. Evaluate "globalThis" with returnByValue: false
    let buf1: [4096]u8 = [0; 4096];
    const len1 = session.evaluateWithOptions("globalThis", false, true, &buf1);
    check(len1 > 0, "evaluate globalThis returns output");
    check(std.mem.indexOf(u8, buf1[0..len1], "\"objectId\"") != null, "contains objectId");
    check(std.mem.indexOf(u8, buf1[0..len1], "\"className\"") != null, "contains className");
    std.debug.print("PASS: 1. Evaluate globalThis RemoteObject\n", .{});

    // 2. callFunctionOn globalThis returning an Object
    let buf2: [4096]u8 = [0; 4096];
    const len2 = session.callFunctionOn("globalThis", "function() { return { answer: 42 }; }", false, true, &buf2);
    check(len2 > 0, "callFunctionOn returns output");
    check(std.mem.indexOf(u8, buf2[0..len2], "\"className\":\"Object\"") != null, "returns className Object");
    check(std.mem.indexOf(u8, buf2[0..len2], "\"objectId\"") != null, "returns objectId for object");
    std.debug.print("PASS: 2. callFunctionOn returns Object RemoteObject\n", .{});

    // 3. callFunctionOn globalThis returning an Array
    let buf3: [4096]u8 = [0; 4096];
    const len3 = session.callFunctionOn("globalThis", "function() { return [1, 2, 3]; }", false, true, &buf3);
    check(len3 > 0, "callFunctionOn array returns output");
    check(std.mem.indexOf(u8, buf3[0..len3], "\"className\":\"Array\"") != null, "returns className Array");
    check(std.mem.indexOf(u8, buf3[0..len3], "\"subtype\":\"array\"") != null, "returns subtype array");
    std.debug.print("PASS: 3. callFunctionOn returns Array RemoteObject\n", .{});

    // 4. callFunctionOn globalThis returning a Function
    let buf4: [4096]u8 = [0; 4096];
    const len4 = session.callFunctionOn("globalThis", "function() { return function named() {}; }", false, true, &buf4);
    check(len4 > 0, "callFunctionOn function returns output");
    check(std.mem.indexOf(u8, buf4[0..len4], "\"className\":\"Function\"") != null, "returns className Function");
    check(std.mem.indexOf(u8, buf4[0..len4], "\"type\":\"function\"") != null, "returns type function");
    std.debug.print("PASS: 4. callFunctionOn returns Function RemoteObject\n", .{});

    std.debug.print("\n=== ALL REMOTE OBJECT & CALLFUNCTIONON TESTS PASSED (100% OK) ===\n", .{});
}
