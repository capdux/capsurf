// test/js/test_qjs_eval_and_error_edge.cp
// Whitebox unit tests for QuickJS-NG engine: eval return types, exception & error
// message extraction, Promise/microtask drainJobs, interrupt budget recovery,
// and multi-context isolation.

import { test, expect_eq } from "cap:test";
import {
    JsRuntime,
    JsContext,
    JSValue,
    JS_GetPropertyStr,
    jsIsException,
    jsIsNull,
    jsIsUndefined,
    jsToBool,
} from "../../src/js/qjs_engine.cp";

fn main() {
    test("1. eval return types: number (int32/int64), bool, string, undefined, null", () => {
        let rt = JsRuntime.init(1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        // Number: int32 (positive, zero, negative)
        const n_pos = ctx.eval("100 + 234");
        defer ctx.freeValue(n_pos);
        expect_eq(jsIsException(n_pos), false);
        expect_eq(jsIsNull(n_pos), false);
        expect_eq(jsIsUndefined(n_pos), false);
        expect_eq(ctx.toInt32(n_pos) catch unreachable, 334);

        const n_neg = ctx.eval("-42");
        defer ctx.freeValue(n_neg);
        expect_eq(jsIsException(n_neg), false);
        expect_eq(ctx.toInt32(n_neg) catch unreachable, -42);

        // Number: int64 (beyond 32-bit range)
        const n_big = ctx.eval("5000000000 + 123");
        defer ctx.freeValue(n_big);
        expect_eq(jsIsException(n_big), false);
        expect_eq(ctx.toInt64(n_big) catch unreachable, 5000000123);

        // Bool: true & false
        const b_true = ctx.eval("3 > 1 && true");
        defer ctx.freeValue(b_true);
        expect_eq(jsIsException(b_true), false);
        expect_eq(jsToBool(ctx.raw_ctx, b_true), true);

        const b_false = ctx.eval("3 < 1 || false");
        defer ctx.freeValue(b_false);
        expect_eq(jsIsException(b_false), false);
        expect_eq(jsToBool(ctx.raw_ctx, b_false), false);

        // String: empty and non-empty
        const s_empty = ctx.eval("''");
        defer ctx.freeValue(s_empty);
        expect_eq(jsIsException(s_empty), false);
        const s_empty_out = ctx.toCString(s_empty);
        defer std.heap.page_allocator.free(s_empty_out);
        expect_eq(std.mem.eql(u8, s_empty_out, ""), true);

        const s_val = ctx.eval("'capsurf-' + (200 + 23)");
        defer ctx.freeValue(s_val);
        expect_eq(jsIsException(s_val), false);
        const s_out = ctx.toCString(s_val);
        defer std.heap.page_allocator.free(s_out);
        expect_eq(std.mem.eql(u8, s_out, "capsurf-223"), true);

        // Undefined: explicit undefined and statement with no completion value
        const u_val = ctx.eval("undefined");
        defer ctx.freeValue(u_val);
        expect_eq(jsIsException(u_val), false);
        expect_eq(jsIsUndefined(u_val), true);
        expect_eq(jsIsNull(u_val), false);

        const u_stmt = ctx.eval("let _tmp_var = 10;");
        defer ctx.freeValue(u_stmt);
        expect_eq(jsIsException(u_stmt), false);
        expect_eq(jsIsUndefined(u_stmt), true);

        // Null
        const null_val = ctx.eval("null");
        defer ctx.freeValue(null_val);
        expect_eq(jsIsException(null_val), false);
        expect_eq(jsIsNull(null_val), true);
        expect_eq(jsIsUndefined(null_val), false);
    });

    test("2. Exception catching and error message/property extraction", () => {
        let rt = JsRuntime.init(1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        // TypeError with custom message
        const type_val = ctx.eval("throw new TypeError('invalid dom node');");
        defer ctx.freeValue(type_val);
        expect_eq(jsIsException(type_val), true);
        expect_eq(rt.isInterrupted(), false);

        const type_exc = ctx.getException();
        defer ctx.freeValue(type_exc);
        const type_str = ctx.toCString(type_exc);
        defer std.heap.page_allocator.free(type_str);
        expect_eq(std.mem.indexOf(u8, type_str, "TypeError") != null, true);
        expect_eq(std.mem.indexOf(u8, type_str, "invalid dom node") != null, true);

        // Extract .name and .message properties from Error object
        const name_prop = JS_GetPropertyStr(ctx.raw_ctx, type_exc, "name");
        defer ctx.freeValue(name_prop);
        const name_str = ctx.toCString(name_prop);
        defer std.heap.page_allocator.free(name_str);
        expect_eq(std.mem.eql(u8, name_str, "TypeError"), true);

        const msg_prop = JS_GetPropertyStr(ctx.raw_ctx, type_exc, "message");
        defer ctx.freeValue(msg_prop);
        const msg_str = ctx.toCString(msg_prop);
        defer std.heap.page_allocator.free(msg_str);
        expect_eq(std.mem.eql(u8, msg_str, "invalid dom node"), true);

        // After getException(), pending exception slot is cleared (JS_TAG_UNINITIALIZED == 4)
        const cleared_exc = ctx.getException();
        defer ctx.freeValue(cleared_exc);
        expect_eq(jsIsException(cleared_exc), false);
        expect_eq(cleared_exc.tag == 4, true);

        // Non-Error thrown value (plain string)
        const str_throw = ctx.eval("throw 'plain-string-fault';");
        defer ctx.freeValue(str_throw);
        expect_eq(jsIsException(str_throw), true);
        const str_exc = ctx.getException();
        defer ctx.freeValue(str_exc);
        const str_exc_msg = ctx.toCString(str_exc);
        defer std.heap.page_allocator.free(str_exc_msg);
        expect_eq(std.mem.eql(u8, str_exc_msg, "plain-string-fault"), true);

        // Non-Error thrown value (number)
        const num_throw = ctx.eval("throw 404;");
        defer ctx.freeValue(num_throw);
        expect_eq(jsIsException(num_throw), true);
        const num_exc = ctx.getException();
        defer ctx.freeValue(num_exc);
        expect_eq(ctx.toInt32(num_exc) catch unreachable, 404);

        // Context still evaluates cleanly after multiple exceptions
        const ok_val = ctx.eval("'recovered'");
        defer ctx.freeValue(ok_val);
        expect_eq(jsIsException(ok_val), false);
        const ok_str = ctx.toCString(ok_val);
        defer std.heap.page_allocator.free(ok_str);
        expect_eq(std.mem.eql(u8, ok_str, "recovered"), true);
    });

    test("3. Promise and microtask drainJobs behavior", () => {
        let rt = JsRuntime.init(1024 * 1024, 100000) catch unreachable;
        defer rt.deinit();

        let ctx = rt.createContext() catch unreachable;
        defer ctx.deinit();

        // Immediate resolved Promise
        const p_res = ctx.eval("Promise.resolve(77)");
        defer ctx.freeValue(p_res);
        expect_eq(jsIsException(p_res), false);
        expect_eq(ctx.isPromise(p_res), true);
        expect_eq(ctx.promiseState(p_res), 1); // JS_PROMISE_FULFILLED == 1
        const p_res_val = ctx.promiseResult(p_res);
        defer ctx.freeValue(p_res_val);
        expect_eq(ctx.toInt32(p_res_val) catch unreachable, 77);

        // Immediate rejected Promise
        const p_rej = ctx.eval("Promise.reject(new Error('rejected-promise'))");
        defer ctx.freeValue(p_rej);
        expect_eq(jsIsException(p_rej), false);
        expect_eq(ctx.isPromise(p_rej), true);
        expect_eq(ctx.promiseState(p_rej), 2); // JS_PROMISE_REJECTED == 2
        const p_rej_val = ctx.promiseResult(p_rej);
        defer ctx.freeValue(p_rej_val);
        const p_rej_str = ctx.toCString(p_rej_val);
        defer std.heap.page_allocator.free(p_rej_str);
        expect_eq(std.mem.indexOf(u8, p_rej_str, "rejected-promise") != null, true);

        // Chained Promise + queueMicrotask pending until drainJobs()
        const chain = ctx.eval(
            "globalThis.microTrace = [];" ++
            "queueMicrotask(() => { globalThis.microTrace.push('q1'); });" ++
            "Promise.resolve(5).then(x => { globalThis.microTrace.push('t1'); return x * 2; }).then(x => { globalThis.microTrace.push('t2'); return x + 3; });"
        );
        defer ctx.freeValue(chain);
        expect_eq(jsIsException(chain), false);
        expect_eq(ctx.isPromise(chain), true);
        expect_eq(ctx.promiseState(chain), 0); // JS_PROMISE_PENDING == 0

        const pre_len = ctx.eval("globalThis.microTrace.length");
        defer ctx.freeValue(pre_len);
        expect_eq(ctx.toInt32(pre_len) catch unreachable, 0);

        // Drain all queued microtasks
        ctx.drainJobs();

        expect_eq(ctx.promiseState(chain), 1);
        const chain_res = ctx.promiseResult(chain);
        defer ctx.freeValue(chain_res);
        expect_eq(ctx.toInt32(chain_res) catch unreachable, 13);

        const trace_val = ctx.eval("globalThis.microTrace.join(',')");
        defer ctx.freeValue(trace_val);
        const trace_str = ctx.toCString(trace_val);
        defer std.heap.page_allocator.free(trace_str);
        expect_eq(std.mem.eql(u8, trace_str, "q1,t1,t2"), true);
    });

    test("4. Interrupt budget recovery and multi-context isolation", () => {
        let rt = JsRuntime.init(2 * 1024 * 1024, 1500) catch unreachable;
        defer rt.deinit();

        let ctx1 = rt.createContext() catch unreachable;
        defer ctx1.deinit();

        let ctx2 = rt.createContext() catch unreachable;
        defer ctx2.deinit();

        // Set distinct properties via setPropertyStr and eval
        const tag1 = ctx1.newString("alpha");
        defer ctx1.freeValue(tag1);
        ctx1.setPropertyStr("ctxTag", tag1);

        const chk_c2 = ctx2.eval("typeof globalThis.ctxTag");
        defer ctx2.freeValue(chk_c2);
        const chk_c2_str = ctx2.toCString(chk_c2);
        defer std.heap.page_allocator.free(chk_c2_str);
        expect_eq(std.mem.eql(u8, chk_c2_str, "undefined"), true);

        // Exhaust reduction budget in ctx1 twice and recover cleanly each time
        let round: u32 = 0;
        while (round < 2) : (round += 1) {
            const loop_val = ctx1.eval("while (true) {}");
            defer ctx1.freeValue(loop_val);
            expect_eq(jsIsException(loop_val), true);
            expect_eq(rt.isInterrupted(), true);

            const exc_val = ctx1.getException();
            defer ctx1.freeValue(exc_val);
            const exc_str = ctx1.toCString(exc_val);
            defer std.heap.page_allocator.free(exc_str);
            expect_eq(std.mem.indexOf(u8, exc_str, "interrupted") != null, true);

            // ctx2 remains unaffected and resets reductions on its own eval
            const c2_ok = ctx2.eval("10 + 20");
            defer ctx2.freeValue(c2_ok);
            expect_eq(jsIsException(c2_ok), false);
            expect_eq(rt.isInterrupted(), false);
            expect_eq(ctx2.toInt32(c2_ok) catch unreachable, 30);
        }

        // ctx1 recovers and preserves its prior state
        const c1_rec = ctx1.eval("globalThis.ctxTag");
        defer ctx1.freeValue(c1_rec);
        expect_eq(jsIsException(c1_rec), false);
        expect_eq(rt.isInterrupted(), false);
        const c1_rec_str = ctx1.toCString(c1_rec);
        defer std.heap.page_allocator.free(c1_rec_str);
        expect_eq(std.mem.eql(u8, c1_rec_str, "alpha"), true);

        // Microtasks scheduled in ctx1 only mutate ctx1's global state
        const c1_task = ctx1.eval("Promise.resolve().then(() => { globalThis.onlyInCtx1 = 99; })");
        defer ctx1.freeValue(c1_task);
        ctx1.drainJobs();

        const c1_val = ctx1.eval("globalThis.onlyInCtx1");
        defer ctx1.freeValue(c1_val);
        expect_eq(ctx1.toInt32(c1_val) catch unreachable, 99);

        const c2_val = ctx2.eval("typeof globalThis.onlyInCtx1");
        defer ctx2.freeValue(c2_val);
        const c2_val_str = ctx2.toCString(c2_val);
        defer std.heap.page_allocator.free(c2_val_str);
        expect_eq(std.mem.eql(u8, c2_val_str, "undefined"), true);
    });
}
