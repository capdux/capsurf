// src/js/qjs_engine.cp
// CapSurf QuickJS-NG Native JavaScript Engine Wrapper
// Zero-GC FFI bindings with Reductions preemptive interrupt control and memory limits.

pub @c {
    @opaque struct JSRuntime {}
    @opaque struct JSContext {}

    struct JSValueUnion {
        uint64: u64;
    }

    struct JSValue {
        u: JSValueUnion;
        tag: i64;
    }

    struct JSClassDef {
        class_name: [*c]const u8;
        finalizer: (@c fn(rt: *JSRuntime, val: JSValue))?;
        gc_mark: *!?;
        call_fn: *!?;
        exotic: *!?;
    }

    fn JS_NewRuntime(): (*JSRuntime)?;
    fn JS_FreeRuntime(rt: *JSRuntime);
    fn JS_SetMemoryLimit(rt: *JSRuntime, limit: usize);
    fn JS_SetMaxStackSize(rt: *JSRuntime, stack_size: usize);
    fn JS_UpdateStackTop(rt: *JSRuntime);
    fn JS_SetInterruptHandler(rt: *JSRuntime, cb: (@c fn(rt: *JSRuntime, opaque_ptr: *!?): i32)?, opaque_ptr: *!?);
    fn JS_ExecutePendingJob(rt: *JSRuntime, pctx: *(*JSContext)?): i32;

    fn JS_NewContext(rt: *JSRuntime): (*JSContext)?;
    fn JS_FreeContext(ctx: *JSContext);
    fn JS_SetContextOpaque(ctx: *JSContext, opaque_ptr: *!?);
    fn JS_GetContextOpaque(ctx: *JSContext): *!?;

    fn JS_NewClassID(rt: *JSRuntime, pclass_id: *u32): u32;
    fn JS_NewClass(rt: *JSRuntime, class_id: u32, class_def: *const JSClassDef): i32;
    fn JS_NewObjectClass(ctx: *JSContext, class_id: u32): JSValue;
    fn JS_SetClassProto(ctx: *JSContext, class_id: u32, proto: JSValue);
    fn JS_SetOpaque(obj: JSValue, ptr: *!?): i32;
    fn JS_GetOpaque(obj: JSValue, class_id: u32): *!?;

    fn JS_GetGlobalObject(ctx: *JSContext): JSValue;
    fn JS_SetPropertyStr(ctx: *JSContext, this_obj: JSValue, prop: [*c]const u8, val: JSValue): i32;
    fn JS_GetPropertyStr(ctx: *JSContext, this_obj: JSValue, prop: [*c]const u8): JSValue;
    fn JS_SetPropertyUint32(ctx: *JSContext, this_obj: JSValue, idx: u32, val: JSValue): i32;
    fn JS_GetPropertyUint32(ctx: *JSContext, this_obj: JSValue, idx: u32): JSValue;

    fn JS_NewAtom(ctx: *JSContext, str: [*c]const u8): u32;
    fn JS_FreeAtom(ctx: *JSContext, v: u32);
    fn JS_DefinePropertyGetSet(ctx: *JSContext, this_obj: JSValue, prop: u32, getter: JSValue, setter: JSValue, flags: i32): i32;
    fn JS_DefinePropertyValueStr(ctx: *JSContext, this_obj: JSValue, prop: [*c]const u8, val: JSValue, flags: i32): i32;

    fn JS_NewObject(ctx: *JSContext): JSValue;
    fn JS_NewArray(ctx: *JSContext): JSValue;
    fn JS_IsArray(ctx: *JSContext, val: JSValue): i32;
    fn JS_IsFunction(ctx: *JSContext, val: JSValue): bool;
    fn JS_NewCFunction2(
        ctx: *JSContext,
        func: (@c fn(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue),
        name: [*c]const u8,
        length: i32,
        cproto: i32,
        magic: i32,
    ): JSValue;
    fn JS_NewStringLen(ctx: *const JSContext, str: [*c]const u8, len: usize): JSValue;

    fn JS_Eval(ctx: *const JSContext, input: [*c]const u8, input_len: usize, filename: [*c]const u8, eval_flags: i32): JSValue;
    fn JS_GetException(ctx: *JSContext): JSValue;
    fn JS_ToCStringLen2(ctx: *const JSContext, plen: *usize, val: JSValue, cesu8: i32): [*]const u8!?;
    fn JS_FreeCString(ctx: *const JSContext, ptr: [*]const u8!?);
    fn JS_FreeValue(ctx: *const JSContext, v: JSValue);
    fn JS_DupValue(ctx: *const JSContext, v: JSValue): JSValue;
    fn JS_IsPromise(val: JSValue): bool;
    fn JS_PromiseState(ctx: *JSContext, promise: JSValue): i32;
    fn JS_PromiseResult(ctx: *JSContext, promise: JSValue): JSValue;
    fn JS_JSONStringify(ctx: *JSContext, obj: JSValue, replacer: JSValue, space0: JSValue): JSValue;
}

pub const JS_PROP_CONFIGURABLE: i32 = 1;
pub const JS_PROP_WRITABLE: i32 = 2;
pub const JS_PROP_ENUMERABLE: i32 = 4;
pub const JS_PROP_C_W_E: i32 = 7;
pub const JS_CFUNC_generic: i32 = 0;
pub const JS_CFUNC_getter: i32 = 8;
pub const JS_CFUNC_setter: i32 = 9;

pub const JS_TAG_FIRST: i64 = -9;
pub const JS_TAG_STRING: i64 = -7;
pub const JS_TAG_OBJECT: i64 = -1;
pub const JS_TAG_INT: i64 = 0;
pub const JS_TAG_BOOL: i64 = 1;
pub const JS_TAG_NULL: i64 = 2;
pub const JS_TAG_UNDEFINED: i64 = 3;
pub const JS_TAG_EXCEPTION: i64 = 6;
pub const JS_TAG_FLOAT64: i64 = 8;

pub fn jsNull(): JSValue {
    return JSValue { u: JSValueUnion { uint64: 0 }, tag: JS_TAG_NULL };
}

pub fn jsUndefined(): JSValue {
    return JSValue { u: JSValueUnion { uint64: 0 }, tag: JS_TAG_UNDEFINED };
}

pub fn jsTrue(): JSValue {
    return JSValue { u: JSValueUnion { uint64: 1 }, tag: JS_TAG_BOOL };
}

pub fn jsFalse(): JSValue {
    return JSValue { u: JSValueUnion { uint64: 0 }, tag: JS_TAG_BOOL };
}

pub fn jsBool(b: bool): JSValue {
    return if (b) jsTrue() else jsFalse();
}

pub fn jsInt(n: i32): JSValue {
    return JSValue { u: JSValueUnion { uint64: @as(u32, @bitCast(n)) }, tag: JS_TAG_INT };
}

pub fn jsFloat(f: f64): JSValue {
    return JSValue { u: JSValueUnion { uint64: @as(u64, @bitCast(f)) }, tag: JS_TAG_FLOAT64 };
}

pub fn jsException(): JSValue {
    return JSValue { u: JSValueUnion { uint64: 0 }, tag: JS_TAG_EXCEPTION };
}

pub fn jsIsException(v: JSValue): bool {
    return v.tag == JS_TAG_EXCEPTION;
}

pub fn jsIsNull(v: JSValue): bool {
    return v.tag == JS_TAG_NULL;
}

pub fn jsIsUndefined(v: JSValue): bool {
    return v.tag == JS_TAG_UNDEFINED;
}

pub fn jsNewString(ctx: *const JSContext, s: []const u8): JSValue {
    return JS_NewStringLen(ctx, s.ptr, s.len);
}

pub fn jsToSlice(ctx: *const JSContext, val: JSValue): []const u8 {
    let len: usize = 0;
    const str_ptr = JS_ToCStringLen2(ctx, &len, val, 0);
    if (str_ptr != null) {
        defer JS_FreeCString(ctx, str_ptr);
        return std.heap.page_allocator.dupe(u8, str_ptr.?[0..len]) catch "";
    }
    return "";
}

pub fn jsJsonStringify(ctx: *JSContext, val: JSValue): []const u8 {
    const jval = JS_JSONStringify(ctx, val, jsUndefined(), jsUndefined());
    let len: usize = 0;
    const str_ptr = JS_ToCStringLen2(ctx, &len, jval, 0);
    if (str_ptr != null) {
        defer JS_FreeCString(ctx, str_ptr);
        const dup = std.heap.page_allocator.dupe(u8, str_ptr.?[0..len]) catch "";
        JS_FreeValue(ctx, jval);
        return dup;
    }
    JS_FreeValue(ctx, jval);
    return "";
}

pub fn defineMethod(
    ctx: *JSContext,
    obj: JSValue,
    method_name: [*c]const u8,
    func: (@c fn(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue),
    length: i32,
) {
    const fn_val = JS_NewCFunction2(ctx, func, method_name, length, JS_CFUNC_generic, 0);
    * = JS_SetPropertyStr(ctx, obj, method_name, fn_val);
}

pub fn defineGetterSetter(
    ctx: *JSContext,
    obj: JSValue,
    prop_name: [*c]const u8,
    getter_fn: (@c fn(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue)?,
    setter_fn: (@c fn(ctx: *JSContext, this_val: JSValue, argc: i32, argv: [*]const JSValue!): JSValue)?,
) {
    const atom = JS_NewAtom(ctx, prop_name);
    defer JS_FreeAtom(ctx, atom);
    let getter_val = jsUndefined();
    if (g := getter_fn) {
        getter_val = JS_NewCFunction2(ctx, g, prop_name, 0, JS_CFUNC_generic, 0);
    }
    let setter_val = jsUndefined();
    if (s := setter_fn) {
        setter_val = JS_NewCFunction2(ctx, s, prop_name, 1, JS_CFUNC_generic, 0);
    }
    * = JS_DefinePropertyGetSet(ctx, obj, atom, getter_val, setter_val, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
}

pub struct ReductionState {
    reduction_limit: u32 = 100000,
    reduction_count: u32 = 0,
    interrupted: bool = false,
}

/// Reduction counter and timeout interrupt handler preventing infinite loops.
@c fn jsInterruptHandler(rt: *JSRuntime, opaque_ptr: *!?): i32 {
    * = rt;
    if (opaque_ptr == null) return 0;
    const state: *ReductionState = @ptrCast(@alignCast(opaque_ptr.?));
    state.reduction_count += 1;
    if (state.reduction_limit > 0 && state.reduction_count >= state.reduction_limit) {
        state.interrupted = true;
        return 1; // Interrupt execution in QuickJS!
    }
    return 0;
}

pub struct JsRuntime {
    raw_rt: *JSRuntime,
    state: *ReductionState,

    static fn init(mem_limit_bytes: usize, reduction_slice: u32): JsRuntime! {
        const rt = JS_NewRuntime() orelse return error.RuntimeInitFailed;
        if (mem_limit_bytes > 0) {
            JS_SetMemoryLimit(rt, mem_limit_bytes);
        }
        JS_SetMaxStackSize(rt, 0); // Disable static thread stack limit to support cross-thread execution
        let st = std.heap.c_allocator.create(ReductionState) catch return error.ReductionStateAllocFailed;
        st.* = ReductionState {
            reduction_limit: reduction_slice,
            reduction_count: 0,
            interrupted: false,
        };
        JS_SetInterruptHandler(rt, jsInterruptHandler, @ptrCast(st));
        return JsRuntime {
            raw_rt: rt,
            state: st,
        };
    }

    fn deinit(*_) {
        JS_FreeRuntime(_.raw_rt);
        std.heap.c_allocator.destroy(_.state);
    }

    fn isInterrupted(): bool {
        return _.state.interrupted;
    }

    fn resetReductions(*_) {
        JS_UpdateStackTop(_.raw_rt);
        _.state.reduction_count = 0;
        _.state.interrupted = false;
    }

    fn createContext(*_): JsContext! {
        const ctx = JS_NewContext(_.raw_rt) orelse return error.ContextInitFailed;
        return JsContext {
            raw_ctx: ctx,
            runtime: _,
        };
    }
}

pub struct JsContext {
    raw_ctx: *JSContext,
    runtime: *JsRuntime,

    fn deinit(*_) {
        JS_FreeContext(_.raw_ctx);
    }

    fn getGlobal(): JSValue {
        return JS_GetGlobalObject(_.raw_ctx);
    }

    fn eval(code: []const u8): JSValue {
        _.runtime.resetReductions();
        let buf: [4096]u8 = [0; 4096];
        if (code.len + 1 <= buf.len) {
            @memcpy(buf[0..code.len], code);
            buf[code.len] = 0;
            return JS_Eval(_.raw_ctx, buf[0..code.len].ptr, code.len, "<eval>", 0);
        }
        return JS_Eval(_.raw_ctx, code.ptr, code.len, "<eval>", 0);
    }

    fn getException(): JSValue {
        return JS_GetException(_.raw_ctx);
    }

    fn toCString(val: JSValue): []const u8 {
        return jsToSlice(_.raw_ctx, val);
    }

    fn newString(s: []const u8): JSValue {
        return JS_NewStringLen(_.raw_ctx, s.ptr, s.len);
    }

    fn newArray(): JSValue {
        return JS_NewArray(_.raw_ctx);
    }

    fn freeValue(val: JSValue) {
        JS_FreeValue(_.raw_ctx, val);
    }

    fn dupValue(val: JSValue): JSValue {
        return JS_DupValue(_.raw_ctx, val);
    }

    fn executePendingJobs(*_): i32 {
        let pctx: (*JSContext)? = null;
        return JS_ExecutePendingJob(_.runtime.raw_rt, &pctx);
    }

    fn drainJobs(*_) {
        let pctx: (*JSContext)? = null;
        while (JS_ExecutePendingJob(_.runtime.raw_rt, &pctx) > 0) {}
    }

    fn isPromise(val: JSValue): bool {
        * = _.raw_ctx;
        return JS_IsPromise(val);
    }

    fn promiseState(val: JSValue): i32 {
        return JS_PromiseState(_.raw_ctx, val);
    }

    fn promiseResult(val: JSValue): JSValue {
        return JS_PromiseResult(_.raw_ctx, val);
    }

    fn jsonStringify(val: JSValue): []const u8 {
        return jsJsonStringify(_.raw_ctx, val);
    }

    fn setPropertyStr(name: []const u8, val: JSValue) {
        let name_buf: [128]u8 = [0; 128];
        const to_copy = @min(name.len, name_buf.len - 1);
        @memcpy(name_buf[0..to_copy], name[0..to_copy]);
        name_buf[to_copy] = 0;
        * = JS_SetPropertyStr(_.raw_ctx, _.getGlobal(), @ptrCast(&name_buf), JS_DupValue(_.raw_ctx, val));
    }

    fn isArray(val: JSValue): bool {
        return JS_IsArray(_.raw_ctx, val) != 0;
    }

    fn isFunction(val: JSValue): bool {
        return JS_IsFunction(_.raw_ctx, val);
    }
}
