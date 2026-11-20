// test/js/test_qjs_interrupt_and_budget.cp
// Unit tests for QuickJS-NG engine: basic eval, reductions preemption,
// syntax/reference error handling, isolated contexts, memory limits & GC.

import { test, expect_eq } from "cap:test";
import { JsRuntime, JsContext, JSValue, JS_SetMemoryLimit, jsIsException } from "../../src/js/qjs_engine.cp";

fn main() {
    test("1. Basic JS evaluation (arithmetic, string, JSON roundtrip) and clean RAII deinit", () => {
        let rt = JsRuntime.init(1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        // Arithmetic evaluation
        const math_val = ctx.eval("(15 + 27) * 2 - 4");
        defer ctx.freeValue(math_val);
        expect_eq(jsIsException(math_val), false);
        const math_num = ctx.toInt32(math_val) catch unreachable;
        expect_eq(math_num, 80);

        // String evaluation
        const str_val = ctx.eval("'hello' + ' ' + 'qjs'");
        defer ctx.freeValue(str_val);
        expect_eq(jsIsException(str_val), false);
        const str_out = ctx.toCString(str_val);
        defer std.heap.page_allocator.free(str_out);
        expect_eq(std.mem.eql(u8, str_out, "hello qjs"), true);

        // JSON roundtrip via JS eval and ctx.jsonStringify
        const obj_val = ctx.eval("JSON.parse('{\"name\":\"capsurf\",\"version\":1,\"ok\":true}')");
        defer ctx.freeValue(obj_val);
        expect_eq(jsIsException(obj_val), false);

        const json_out = ctx.jsonStringify(obj_val);
        defer std.heap.page_allocator.free(json_out);
        expect_eq(std.mem.eql(u8, json_out, "{\"name\":\"capsurf\",\"version\":1,\"ok\":true}"), true);
    });

    test("2. Reductions preemption interrupt handler halts infinite while(true){} loop", () => {
        // Small reduction budget so the infinite loop aborts quickly
        let rt = JsRuntime.init(1024 * 1024, 2000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        expect_eq(rt.isInterrupted(), false);

        const loop_val = ctx.eval("while (true) {}");
        defer ctx.freeValue(loop_val);
        expect_eq(jsIsException(loop_val), true);
        expect_eq(rt.isInterrupted(), true);

        // Drain the thrown internal interrupt exception cleanly
        const exc_val = ctx.getException();
        defer ctx.freeValue(exc_val);

        // Next eval resets reductions and runs normally
        const ok_val = ctx.eval("21 * 2");
        defer ctx.freeValue(ok_val);
        expect_eq(jsIsException(ok_val), false);
        expect_eq(rt.isInterrupted(), false);
        const ok_num = ctx.toInt32(ok_val) catch unreachable;
        expect_eq(ok_num, 42);
    });

    test("3. Runtime syntax and reference error handling without leaking or crashing", () => {
        let rt = JsRuntime.init(1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        // SyntaxError
        const syn_val = ctx.eval("function broken( { return 1; }");
        defer ctx.freeValue(syn_val);
        expect_eq(jsIsException(syn_val), true);
        expect_eq(rt.isInterrupted(), false);

        const syn_exc = ctx.getException();
        defer ctx.freeValue(syn_exc);
        const syn_msg = ctx.toCString(syn_exc);
        defer std.heap.page_allocator.free(syn_msg);
        expect_eq(std.mem.indexOf(u8, syn_msg, "SyntaxError") != null, true);

        // ReferenceError
        const ref_val = ctx.eval("undeclaredIdentifier + 10");
        defer ctx.freeValue(ref_val);
        expect_eq(jsIsException(ref_val), true);
        expect_eq(rt.isInterrupted(), false);

        const ref_exc = ctx.getException();
        defer ctx.freeValue(ref_exc);
        const ref_msg = ctx.toCString(ref_exc);
        defer std.heap.page_allocator.free(ref_msg);
        expect_eq(std.mem.indexOf(u8, ref_msg, "ReferenceError") != null, true);

        // Context remains healthy after errors
        const after_val = ctx.eval("100 + 23");
        defer ctx.freeValue(after_val);
        expect_eq(jsIsException(after_val), false);
        const after_num = ctx.toInt32(after_val) catch unreachable;
        expect_eq(after_num, 123);
    });

    test("4. Isolated multiple JS contexts and setMemoryLimit / runGC behavior", () => {
        let rt = JsRuntime.init(2 * 1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx1 = rt.createContext() catch unreachable;
        defer ctx1.deinit();

        let ctx2 = rt.createContext() catch unreachable;
        defer ctx2.deinit();

        // Isolate global scope across contexts sharing the same runtime
        const set1 = ctx1.eval("globalThis.isolatedVal = 111; globalThis.isolatedVal;");
        defer ctx1.freeValue(set1);
        expect_eq(ctx1.toInt32(set1) catch unreachable, 111);

        const chk2 = ctx2.eval("typeof globalThis.isolatedVal");
        defer ctx2.freeValue(chk2);
        const chk2_str = ctx2.toCString(chk2);
        defer std.heap.page_allocator.free(chk2_str);
        expect_eq(std.mem.eql(u8, chk2_str, "undefined"), true);

        const set2 = ctx2.eval("globalThis.isolatedVal = 222; globalThis.isolatedVal;");
        defer ctx2.freeValue(set2);
        expect_eq(ctx2.toInt32(set2) catch unreachable, 222);

        const rechk1 = ctx1.eval("globalThis.isolatedVal");
        defer ctx1.freeValue(rechk1);
        expect_eq(ctx1.toInt32(rechk1) catch unreachable, 111);

        // Enforce a tight memory limit and verify OOM exception + runGC recovery
        JS_SetMemoryLimit(rt.raw_rt, 256 * 1024);

        const oom_val = ctx1.eval("new Array(200000).fill('capsurf')");
        defer ctx1.freeValue(oom_val);
        expect_eq(jsIsException(oom_val), true);

        const oom_exc = ctx1.getException();
        defer ctx1.freeValue(oom_exc);

        rt.runGC();
        ctx2.runGC();

        // Raise limit back and confirm runtime/contexts recover cleanly
        JS_SetMemoryLimit(rt.raw_rt, 2 * 1024 * 1024);
        const rec_val = ctx1.eval("globalThis.isolatedVal + 9");
        defer ctx1.freeValue(rec_val);
        expect_eq(jsIsException(rec_val), false);
        expect_eq(ctx1.toInt32(rec_val) catch unreachable, 120);
    });
}
