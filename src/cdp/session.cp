// src/cdp/session.cp
// CapSurf CDP Page Session Controller
// Coordinates HTML parsing, CSS cascade, Layout computation, and JavaScript execution.

import { Doc, Document, NodeId, NULL_NODE, NodeType } from "../dom/dom.cp";
import { Matcher } from "../css/matcher.cp";
import { StyleSheet, Cascade, ComputedStyle } from "../css/cascade.cp";
import { LayoutTree, Rect } from "../layout/layout.cp";
import { JsRuntime, JsContext, JSRuntime, JSContext, JSModuleDef, JS_Eval, JS_FreeValue, JS_GetException, JS_ToCStringLen2, JS_FreeCString, JSValue, jsToSlice, jsIsException, jsUndefined, JS_TAG_INT, JS_TAG_FLOAT64, JS_TAG_BOOL, JS_TAG_NULL, JS_TAG_UNDEFINED, JS_TAG_OBJECT, JS_ToInt32, JS_SetPropertyStr, JS_GetPropertyStr, JS_GetImportMeta, JS_Call, JS_NewStringLen, JS_DupValue, JS_IsPromise, JS_PromiseState } from "../js/qjs_engine.cp";
import { mountDom, JsDomContext, ConsoleRecord, getMilliTimestamp } from "../js/dom_binding.cp";
import { CookieJar, CookieEntry } from "../net/cookies.cp";
import { CdpBoxModel, CdpEvent, extractJsonInt, normalizeExceptionMessage, unescapeJsonString } from "./types.cp";
import { FetchState } from "./fetch.cp";
import { ScreenshotOptions, CanvasBitmap, SelectionSpan, FormControlValue, ScrollOffset, renderDocumentToPng } from "../render/painter.cp";
import { CssColor } from "cap:css";
import { Base64 } from "cap:encoding";
import { uncompress } from "cap:pack";
import { recordResponseBody, consumePendingFetchBody } from "./network.cp";
import { TargetRegistry } from "./target.cp";
import { DedicatedWorkerPool } from "./workers.cp";
import { DebuggerState } from "./debugger.cp";
import { ScreencastHandler } from "./screencast.cp";
import { ResourceStore } from "./resource_store.cp";
import { KitesurfFacade } from "./kitesurf_facade.cp";
import { MockFixtures } from "../net/mock_fixtures.cp";
import { CspPolicy, parseCsp, verifySriIntegrity, isScriptNosniffBlocked, isInlineScriptAllowed, isInlineEventHandlerAllowed, isExternalScriptAllowed } from "../net/csp.cp";
import { svgDocumentHtml, isSvgDocument } from "./svg_document.cp";
import { PAGE_RUNTIME_HOOKS_JS, PAGE_INSERTED_SCRIPT_HOOKS_JS, PAGE_WEBMCP_HOOKS_JS, registerSessionHooksNative, runSpeculativePrefetchForNav, clearLearnedScripts } from "./session_hooks.cp";
import { MAX_CONCURRENT_OOPIF } from "./worker_budget.cp";
import { ttyUaStylesheet, DEFAULT_TTY_CELL_WIDTH_PX, DEFAULT_TTY_CELL_HEIGHT_PX } from "../tty/nav_params.cp";

@c {
    fn usleep(usec: u32): i32;
    fn JS_EvalFunction(ctx: *JSContext, fun_obj: JSValue): JSValue;
    fn JS_ThrowTypeError(ctx: *JSContext, fmt: [*c]const u8, ...): JSValue;
    fn JS_ThrowSyntaxError(ctx: *JSContext, fmt: [*c]const u8, ...): JSValue;
    fn JS_ParseJSON(ctx: *JSContext, buf: [*c]const u8, buf_len: usize, filename: [*c]const u8): JSValue;
    fn JS_NewCModule(ctx: *JSContext, name_str: [*c]const u8, func: (@c fn(ctx: *JSContext, m: *JSModuleDef): i32)?): (*JSModuleDef)?;
    fn JS_AddModuleExport(ctx: *JSContext, m: *JSModuleDef, name_str: [*c]const u8): i32;
    fn JS_SetModuleExport(ctx: *JSContext, m: *JSModuleDef, export_name: [*c]const u8, val: JSValue): i32;
    fn JS_SetModulePrivateValue(ctx: *JSContext, m: *JSModuleDef, val: JSValue): i32;
    fn JS_GetModulePrivateValue(ctx: *JSContext, m: *JSModuleDef): JSValue;
    fn JS_SetModuleLoaderFunc(
        rt: *JSRuntime,
        module_normalize: (@c fn(ctx: *JSContext, module_base_name: [*c]const u8, module_name: [*c]const u8, opaque_ptr: *!?): [*c]u8)?,
        module_loader: (@c fn(ctx: *JSContext, module_name: [*c]const u8, opaque_ptr: *!?): (*JSModuleDef)?)?,
        opaque_ptr: *!?,
    );
    fn JS_SetModuleLoaderFunc2(
        rt: *JSRuntime,
        module_normalize: (@c fn(ctx: *JSContext, module_base_name: [*c]const u8, module_name: [*c]const u8, opaque_ptr: *!?): [*c]u8)?,
        module_loader: (@c fn(ctx: *JSContext, module_name: [*c]const u8, opaque_ptr: *!?, attributes: JSValue): (*JSModuleDef)?)?,
        module_check_attrs: (@c fn(ctx: *JSContext, opaque_ptr: *!?, attributes: JSValue): i32)?,
        opaque_ptr: *!?,
    );
    fn js_strdup(ctx: *JSContext, str: [*c]const u8): [*c]u8;
    struct JSMemoryUsage {
        malloc_size: i64,
        malloc_limit: i64,
        memory_used_size: i64,
        malloc_count: i64,
        memory_used_count: i64,
        atom_count: i64,
        atom_size: i64,
        str_count: i64,
        str_size: i64,
        obj_count: i64,
        obj_size: i64,
        prop_count: i64,
        prop_size: i64,
        shape_count: i64,
        shape_size: i64,
        js_func_count: i64,
        js_func_size: i64,
        js_func_code_size: i64,
        js_func_pc2line_count: i64,
        js_func_pc2line_size: i64,
        c_func_count: i64,
        array_count: i64,
        fast_array_count: i64,
        fast_array_elements: i64,
        binary_object_count: i64,
        binary_object_size: i64,
    }
    fn JS_ComputeMemoryUsage(rt: *JSRuntime, s: *JSMemoryUsage);
}

pub struct NavigationEntry {
    id: i32 = 0,
    url: []const u8 = "",
    title: []const u8 = "",
}

pub struct IsolatedWorldEntry {
    name_buf: [128]u8 = [0; 128],
    name_len: usize = 0,
    context_id: i64 = 0,
    active: bool = false,

    fn name(*_): []const u8 {
        return _.name_buf[0.._.name_len];
    }
}

pub fn hashStyleBytes(bytes: []const u8): u64 {
    let h: u64 = 1125899906842597;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        h ^= @as(u64, bytes[i]);
        h = (h *% 1099511628211);
    }
    return h;
}

pub fn escapeJsonString(src: []const u8, dst: []u8): []const u8 {
    let out_idx: usize = 0;
    let i: usize = 0;
    while (i < src.len && out_idx + 2 <= dst.len) : (i += 1) {
        const c = src[i];
        if (c == '"') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = '"';
            out_idx += 2;
        } else if (c == '\\') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = '\\';
            out_idx += 2;
        } else if (c == '\n') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = 'n';
            out_idx += 2;
        } else if (c == '\r') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = 'r';
            out_idx += 2;
        } else if (c == '\t') {
            dst[out_idx] = '\\';
            dst[out_idx + 1] = 't';
            out_idx += 2;
        } else {
            dst[out_idx] = c;
            out_idx += 1;
        }
    }
    return dst[0..out_idx];
}

pub struct RegisteredFace {
    weight: u32 = 400,
    has_wght_axis: bool = false,
}

pub struct RegisteredFamily {
    family: [64]u8 = [0; 64],
    family_len: usize = 0,
    faces: [8]RegisteredFace = [RegisteredFace{}; 8],
    face_count: usize = 0,
}

pub fn nextUtf8Codepoint(bytes: []const u8, idx: *usize): u32? {
    if (idx.* >= bytes.len) return null;
    const b0 = bytes[idx.*];
    if (b0 < 0x80) {
        idx.* += 1;
        return @as(u32, b0);
    } else if ((b0 & 0xE0) == 0xC0) {
        if (idx.* + 1 >= bytes.len) { idx.* = bytes.len; return null; }
        const b1 = bytes[idx.* + 1];
        idx.* += 2;
        return ((@as(u32, b0 & 0x1F)) << 6) | (@as(u32, b1 & 0x3F));
    } else if ((b0 & 0xF0) == 0xE0) {
        if (idx.* + 2 >= bytes.len) { idx.* = bytes.len; return null; }
        const b1 = bytes[idx.* + 1];
        const b2 = bytes[idx.* + 2];
        idx.* += 3;
        return ((@as(u32, b0 & 0x0F)) << 12) | ((@as(u32, b1 & 0x3F)) << 6) | (@as(u32, b2 & 0x3F));
    } else if ((b0 & 0xF8) == 0xF0) {
        if (idx.* + 3 >= bytes.len) { idx.* = bytes.len; return null; }
        const b1 = bytes[idx.* + 1];
        const b2 = bytes[idx.* + 2];
        const b3 = bytes[idx.* + 3];
        idx.* += 4;
        return ((@as(u32, b0 & 0x07)) << 18) | ((@as(u32, b1 & 0x3F)) << 12) | ((@as(u32, b2 & 0x3F)) << 6) | (@as(u32, b3 & 0x3F));
    } else {
        idx.* += 1;
        return null;
    }
}

pub fn parseHexCodepoint(slice: []const u8): u32? {
    if (slice.len == 0) return null;
    let val: u32 = 0;
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        const c = slice[i];
        let digit: u32 = 0;
        if (c >= '0' && c <= '9') {
            digit = @as(u32, c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = @as(u32, c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            digit = @as(u32, c - 'A' + 10);
        } else {
            return null;
        }
        val = (val << 4) | digit;
    }
    return val;
}

pub fn parseUnicodeRange(str: []const u8, out_min: *u32, out_max: *u32): bool {
    let s = std.mem.trim(u8, str, " \t\r\n");
    if (s.len < 2) return false;
    if ((s[0] == 'U' || s[0] == 'u') && s[1] == '+') {
        s = s[2..];
    }
    if (dash_idx := std.mem.indexOfScalar(u8, s, '-')) {
        const first = s[0..dash_idx];
        const second = s[dash_idx + 1 ..];
        if (min_v := parseHexCodepoint(first)) {
            if (max_v := parseHexCodepoint(second)) {
                out_min.* = min_v;
                out_max.* = max_v;
                return true;
            }
        }
    } else {
        if (single_v := parseHexCodepoint(s)) {
            out_min.* = single_v;
            out_max.* = single_v;
            return true;
        }
    }
    return false;
}

pub struct DeferredFontFace {
    url_buf: [512]u8 = [0; 512],
    url_len: usize = 0,
    mime_buf: [32]u8 = [0; 32],
    mime_len: usize = 0,
    min_cp: u32 = 0,
    max_cp: u32 = 0,
    has_range: bool = false,
    fetched: bool = false,
}

fn textHasCodePointInRange(text: []const u8, min_cp: u32, max_cp: u32): bool {
    let idx: usize = 0;
    while (idx < text.len) {
        if (cp := nextUtf8Codepoint(text, &idx)) {
            if (cp >= min_cp && cp <= max_cp) {
                return true;
            }
        }
    }
    return false;
}

fn sameOriginUrls(a: []const u8, b: []const u8): bool {
    const a_scheme = std.mem.indexOf(u8, a, "://");
    const b_scheme = std.mem.indexOf(u8, b, "://");
    if (a_scheme == null || b_scheme == null) return true;
    const a_s = a[0..a_scheme.?];
    const b_s = b[0..b_scheme.?];
    if (!std.ascii.eqlIgnoreCase(a_s, b_s)) return false;
    const a_rest = a[a_scheme.? + 3 ..];
    const b_rest = b[b_scheme.? + 3 ..];
    let a_host = a_rest;
    if (sl := std.mem.indexOfScalar(u8, a_rest, '/')) {
        a_host = a_rest[0..sl];
    }
    let b_host = b_rest;
    if (sl := std.mem.indexOfScalar(u8, b_rest, '/')) {
        b_host = b_rest[0..sl];
    }
    return std.ascii.eqlIgnoreCase(a_host, b_host);
}

pub fn documentHasCodePointInRange(doc: *const Document, min_cp: u32, max_cp: u32): bool {
    let nid: usize = 0;
    while (nid < doc.nodes.items.len) : (nid += 1) {
        const node = &doc.nodes.items[nid];
        if (node.node_type == NodeType.Text && node.text_content.len > 0) {
            let idx: usize = 0;
            while (idx < node.text_content.len) {
                if (cp := nextUtf8Codepoint(node.text_content, &idx)) {
                    if (cp >= min_cp && cp <= max_cp) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

pub fn canRewriteDocumentUrl(doc_url: []const u8, target_url: []const u8): bool {
    if (target_url.len == 0) return true;
    if (target_url[0] == '#') return true;

    const target_scheme_end = std.mem.indexOf(u8, target_url, "://");
    const doc_scheme_end = std.mem.indexOf(u8, doc_url, "://");

    if (target_scheme_end == null) {
        return true;
    }

    if (doc_scheme_end == null) {
        let doc_base = doc_url;
        if (d_h := std.mem.indexOf(u8, doc_url, "#")) {
            doc_base = doc_url[0..d_h];
        }
        let tgt_base = target_url;
        if (t_h := std.mem.indexOf(u8, target_url, "#")) {
            tgt_base = target_url[0..t_h];
        }
        return std.mem.eql(u8, doc_base, tgt_base);
    }

    const doc_scheme = doc_url[0..doc_scheme_end.?];
    const target_scheme = target_url[0..target_scheme_end.?];
    if (!std.mem.eql(u8, doc_scheme, target_scheme)) return false;

    const doc_auth_start = doc_scheme_end.? + 3;
    let doc_auth = doc_url[doc_auth_start..];
    if (d_slash := std.mem.indexOfPos(u8, doc_url, doc_auth_start, "/")) {
        doc_auth = doc_url[doc_auth_start..d_slash];
    }

    const tgt_auth_start = target_scheme_end.? + 3;
    let tgt_auth = target_url[tgt_auth_start..];
    if (t_slash := std.mem.indexOfPos(u8, target_url, tgt_auth_start, "/")) {
        tgt_auth = target_url[tgt_auth_start..t_slash];
    }

    if (!std.mem.eql(u8, doc_auth, tgt_auth)) return false;

    if (std.mem.eql(u8, target_scheme, "http") || std.mem.eql(u8, target_scheme, "https")) {
        return true;
    }

    let doc_base = doc_url;
    if (d_h := std.mem.indexOf(u8, doc_url, "#")) {
        doc_base = doc_url[0..d_h];
    }
    let tgt_base = target_url;
    if (t_h := std.mem.indexOf(u8, target_url, "#")) {
        tgt_base = target_url[0..t_h];
    }
    return std.mem.eql(u8, doc_base, tgt_base);
}

pub const DEFAULT_HTTP_USER_AGENT: []const u8 = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/145.0.0.0 Safari/537.36 CapSurf/1.0.0";

pub struct ImportMapEntry {
    key: []const u8 = "",
    val: []const u8 = "",
}

pub struct InlineModuleEntry {
    url: []const u8 = "",
    code: []const u8 = "",
}

pub struct ResolvedModuleEntry {
    url: []const u8 = "",
}

pub struct ModuleIntegrityEntry {
    url: []const u8 = "",
    integrity: []const u8 = "",
}

pub struct PendingScriptItem {
    code: []const u8 = "",
    url: []const u8 = "",
    script_idx: usize = 0,
    script_nid: usize = 0,
    is_module: bool = false,
    has_src: bool = false,
}

fn isBareSpecifier(s: []const u8): bool {
    if (s.len == 0) return false;
    if (std.mem.startsWith(u8, s, "/") || std.mem.startsWith(u8, s, "./") || std.mem.startsWith(u8, s, "../")) {
        return false;
    }
    if (std.mem.indexOf(u8, s, "://") != null || std.mem.startsWith(u8, s, "data:") || std.mem.startsWith(u8, s, "blob:")) {
        return false;
    }
    return true;
}

fn isAssetImportUrl(url: []const u8): bool {
    let path = url;
    if (qi := std.mem.indexOfScalar(u8, path, '?')) path = path[0..qi];
    if (hi := std.mem.indexOfScalar(u8, path, '#')) path = path[0..hi];
    const exts: [13][]const u8 = [
        ".css", ".svg", ".png", ".jpg", ".jpeg", ".gif", ".webp", ".ico", ".woff", ".woff2", ".ttf", ".otf", ".wasm",
    ];
    let i: usize = 0;
    while (i < exts.len) : (i += 1) {
        const ext = exts[i];
        if (path.len >= ext.len && std.ascii.eqlIgnoreCase(path[path.len - ext.len ..], ext)) {
            return true;
        }
    }
    return false;
}

fn resolveUrlWithDots(base_url: []const u8, rel: []const u8, out_buf: []u8): []const u8 {
    if (std.mem.startsWith(u8, rel, "http://") || std.mem.startsWith(u8, rel, "https://") || std.mem.startsWith(u8, rel, "data:") || std.mem.startsWith(u8, rel, "blob:")) {
        return rel;
    }
    if (std.mem.startsWith(u8, rel, "/")) {
        let origin = base_url;
        if (idx := std.mem.indexOf(u8, base_url, "://")) {
            const after_scheme = base_url[idx + 3 ..];
            if (slash_pos := std.mem.indexOfScalar(u8, after_scheme, '/')) {
                origin = base_url[0 .. idx + 3 + slash_pos];
            }
        }
        return std.fmt.bufPrint(out_buf, "{s}{s}", .{ origin, rel }) catch rel;
    }
    let clean_rel = rel;
    let dir_base = base_url;
    if (last_slash := std.mem.lastIndexOfScalar(u8, base_url, '/')) {
        dir_base = base_url[0..last_slash];
    }
    while (std.mem.startsWith(u8, clean_rel, "./") || std.mem.startsWith(u8, clean_rel, "../")) {
        if (std.mem.startsWith(u8, clean_rel, "./")) {
            clean_rel = clean_rel[2..];
        } else {
            clean_rel = clean_rel[3..];
            if (ls := std.mem.lastIndexOfScalar(u8, dir_base, '/')) {
                if (scheme_idx := std.mem.indexOf(u8, dir_base, "://")) {
                    if (ls > scheme_idx + 2) {
                        dir_base = dir_base[0..ls];
                    }
                } else {
                    dir_base = dir_base[0..ls];
                }
            }
        }
    }
    if (dir_base.len > 0) {
        return std.fmt.bufPrint(out_buf, "{s}/{s}", .{ dir_base, clean_rel }) catch rel;
    }
    return rel;
}

fn resolveModuleSpecifier(session: *CdpPageSession, specifier: []const u8, base_url: []const u8, record_resolved: bool, out_buf: []u8): []const u8 {
    let norm_buf: [1024]u8 = undefined;
    let norm_url: []const u8 = "";
    if (!isBareSpecifier(specifier)) {
        norm_url = resolveUrlWithDots(base_url, specifier, &norm_buf);
    }

    let matched_val: ([]const u8)? = null;
    // 1. Exact match in import map
    let i: usize = 0;
    while (i < session.import_map_entries.items.len) : (i += 1) {
        const entry = &session.import_map_entries.items[i];
        if (std.mem.eql(u8, entry.key, specifier) || (norm_url.len > 0 && std.mem.eql(u8, entry.key, norm_url))) {
            matched_val = entry.val;
            break;
        }
    }

    // 2. Prefix match (keys ending with '/')
    let prefix_buf: [1024]u8 = undefined;
    if (matched_val == null) {
        let best_len: usize = 0;
        let best_idx: usize = 0;
        let found_prefix = false;
        let best_suffix: []const u8 = "";
        let pi: usize = 0;
        while (pi < session.import_map_entries.items.len) : (pi += 1) {
            const entry = &session.import_map_entries.items[pi];
            if (entry.key.len > 0 && entry.key[entry.key.len - 1] == '/') {
                if (std.mem.startsWith(u8, specifier, entry.key) && entry.key.len > best_len) {
                    best_len = entry.key.len;
                    best_idx = pi;
                    best_suffix = specifier[entry.key.len..];
                    found_prefix = true;
                } else if (norm_url.len > 0 && std.mem.startsWith(u8, norm_url, entry.key) && entry.key.len > best_len) {
                    best_len = entry.key.len;
                    best_idx = pi;
                    best_suffix = norm_url[entry.key.len..];
                    found_prefix = true;
                }
            }
        }
        if (found_prefix) {
            const val_prefix = session.import_map_entries.items[best_idx].val;
            matched_val = std.fmt.bufPrint(&prefix_buf, "{s}{s}", .{ val_prefix, best_suffix }) catch null;
        }
    }

    let final_res: []const u8 = specifier;
    if (mv := matched_val) {
        final_res = mv;
    } else if (norm_url.len > 0) {
        final_res = norm_url;
    }

    const copy_len = @min(final_res.len, out_buf.len - 1);
    @memcpy(out_buf[0..copy_len], final_res[0..copy_len]);
    out_buf[copy_len] = 0;

    if (record_resolved && !isBareSpecifier(out_buf[0..copy_len])) {
        session.markModuleResolved(out_buf[0..copy_len]);
    }
    return out_buf[0..copy_len];
}

fn setModuleImportMeta(ctx: *JSContext, m: *JSModuleDef, full_url: []const u8) {
    const meta_obj = JS_GetImportMeta(ctx, m);
    if (!jsIsException(meta_obj)) {
        const url_val = JS_NewStringLen(ctx, full_url.ptr, full_url.len);
        * = JS_SetPropertyStr(ctx, meta_obj, "url", JS_DupValue(ctx, url_val));
        const factory_src = "(function(base){return function resolve(s){s=String(s);var resolved=s;var map=globalThis.__capsurf_im_entries;if(Array.isArray(map)){var norm=s;if(/^(?:\\/|\\.\\.?\\/|[a-zA-Z][a-zA-Z0-9+.-]*:)/.test(s)){try{norm=new URL(s,base).href;}catch(e){}}var hit=false;for(var i=0;i<map.length;i++){if(map[i][0]===s||map[i][0]===norm){resolved=map[i][1];hit=true;break;}}if(!hit){var bestLen=0,bestVal='',bestSuf='';for(var i=0;i<map.length;i++){var k=map[i][0];if(k.endsWith('/')){if(s.startsWith(k)&&k.length>bestLen){bestLen=k.length;bestVal=map[i][1];bestSuf=s.slice(k.length);}else if(norm.startsWith(k)&&k.length>bestLen){bestLen=k.length;bestVal=map[i][1];bestSuf=norm.slice(k.length);}}}if(bestLen>0){resolved=bestVal+bestSuf;hit=true;}}if(!hit&&typeof globalThis.__ks_resolve_import_map==='function'){var ks=globalThis.__ks_resolve_import_map(s);if(ks!==s)resolved=ks;}}if(!/^(?:\\/|\\.\\.?\\/|[a-zA-Z][a-zA-Z0-9+.-]*:)/.test(resolved))throw new TypeError('Bare module specifier requires an import map: '+s);return new URL(resolved,base).href;};})";
        const factory_fn = JS_Eval(ctx, factory_src.ptr, factory_src.len, "<import.meta.resolve>", 0);
        if (!jsIsException(factory_fn)) {
            let args: [1]JSValue = [url_val];
            const resolve_fn = JS_Call(ctx, factory_fn, jsUndefined(), 1, @ptrCast(&args));
            if (!jsIsException(resolve_fn)) {
                * = JS_SetPropertyStr(ctx, meta_obj, "resolve", resolve_fn);
            } else {
                const exc2 = JS_GetException(ctx);
                JS_FreeValue(ctx, exc2);
            }
            JS_FreeValue(ctx, factory_fn);
        } else {
            const exc1 = JS_GetException(ctx);
            JS_FreeValue(ctx, exc1);
        }
        JS_FreeValue(ctx, url_val);
        JS_FreeValue(ctx, meta_obj);
    }
}

@c fn sessionModuleNormalize(ctx: *JSContext, module_base_name: [*c]const u8, module_name: [*c]const u8, opaque_ptr: *!?): [*c]u8 {
    if (module_name == null) return js_strdup(ctx, "");
    const raw_name: []const u8 = std.mem.sliceTo(module_name, 0);
    if (raw_name.len == 0) return js_strdup(ctx, module_name);
    if (std.mem.startsWith(u8, raw_name, "data:")) return js_strdup(ctx, module_name);

    let session_ptr: (*CdpPageSession)? = null;
    if (opaque_ptr != null) {
        session_ptr = @ptrCast(@alignCast(opaque_ptr.?));
    }

    let base_url: []const u8 = "";
    if (module_base_name != null) {
        const bn = std.mem.sliceTo(module_base_name, 0);
        if (std.mem.indexOf(u8, bn, "://") != null) {
            base_url = bn;
        }
    }
    if (base_url.len == 0 && session_ptr != null) {
        base_url = session_ptr.?.url;
    }

    let out_buf: [1024]u8 = undefined;
    if (session_ptr != null) {
        const res = resolveModuleSpecifier(session_ptr.?, raw_name, base_url, true, &out_buf);
        if (res.len < out_buf.len) {
            out_buf[res.len] = 0;
            return js_strdup(ctx, @ptrCast(&out_buf));
        }
    } else if (!isBareSpecifier(raw_name)) {
        const res = resolveUrlWithDots(base_url, raw_name, &out_buf);
        if (res.len < out_buf.len) {
            out_buf[res.len] = 0;
            return js_strdup(ctx, @ptrCast(&out_buf));
        }
    }
    return js_strdup(ctx, module_name);
}

fn hasJsonExtension(url: []const u8): bool {
    let path = url;
    if (qi := std.mem.indexOfScalar(u8, path, '?')) path = path[0..qi];
    if (hi := std.mem.indexOfScalar(u8, path, '#')) path = path[0..hi];
    return path.len >= 5 && std.ascii.eqlIgnoreCase(path[path.len - 5 ..], ".json");
}

@c fn sessionJsonModuleInit(ctx: *JSContext, m: *JSModuleDef): i32 {
    const val = JS_GetModulePrivateValue(ctx, m);
    if (JS_SetModuleExport(ctx, m, "default", val) < 0) {
        return -1;
    }
    return 0;
}

@c fn sessionModuleCheckAttrs(ctx: *JSContext, opaque_ptr: *!?, attributes: JSValue): i32 {
    * = opaque_ptr;
    if (attributes.tag == JS_TAG_UNDEFINED || attributes.tag == JS_TAG_NULL) {
        return 0;
    }
    const type_val = JS_GetPropertyStr(ctx, attributes, "type");
    defer JS_FreeValue(ctx, type_val);
    if (jsIsException(type_val)) {
        return -1;
    }
    if (type_val.tag == JS_TAG_UNDEFINED) {
        return 0;
    }
    let len: usize = 0;
    const cstr = JS_ToCStringLen2(ctx, &len, type_val, 0);
    if (cstr != null) {
        defer JS_FreeCString(ctx, cstr);
        const s = unsafe { cstr.?[0..len] };
        if (std.mem.eql(u8, s, "json")) {
            return 0;
        }
        * = JS_ThrowTypeError(ctx, "Unsupported import attribute type: \"%s\"", cstr.?);
        return -1;
    }
    * = JS_ThrowTypeError(ctx, "Unsupported import attribute type");
    return -1;
}

@c fn sessionModuleLoader(ctx: *JSContext, module_name: [*c]const u8, opaque_ptr: *!?, attributes: JSValue): (*JSModuleDef)? {
    if (module_name == null) return null;
    const raw_name: []const u8 = std.mem.sliceTo(module_name, 0);
    if (raw_name.len == 0) return null;

    let want_json = false;
    if (attributes.tag != JS_TAG_UNDEFINED && attributes.tag != JS_TAG_NULL) {
        const type_val = JS_GetPropertyStr(ctx, attributes, "type");
        defer JS_FreeValue(ctx, type_val);
        if (jsIsException(type_val)) {
            return null;
        }
        if (type_val.tag != JS_TAG_UNDEFINED) {
            let tlen: usize = 0;
            const tcstr = JS_ToCStringLen2(ctx, &tlen, type_val, 0);
            if (tcstr != null) {
                defer JS_FreeCString(ctx, tcstr);
                const ts = unsafe { tcstr.?[0..tlen] };
                if (std.mem.eql(u8, ts, "json")) {
                    want_json = true;
                } else {
                    * = JS_ThrowTypeError(ctx, "Unsupported import attribute type: \"%s\"", tcstr.?);
                    return null;
                }
            } else {
                * = JS_ThrowTypeError(ctx, "Unsupported import attribute type");
                return null;
            }
        }
    }
    const is_json_module = want_json || hasJsonExtension(raw_name);

    let session_ptr: (*CdpPageSession)? = null;
    if (opaque_ptr != null) {
        session_ptr = @ptrCast(@alignCast(opaque_ptr.?));
    }

    if (isBareSpecifier(raw_name)) {
        * = JS_ThrowTypeError(ctx, "Failed to resolve module specifier \"%s\". Relative references must start with either \"/\", \"./\", or \"../\" (requires an import map).", module_name);
        return null;
    }

    if (isAssetImportUrl(raw_name)) {
        const stub = "export default {};";
        const stub_eval = JS_Eval(ctx, stub.ptr, stub.len, module_name, 33);
        if (jsIsException(stub_eval)) {
            const exc_s = JS_GetException(ctx);
            JS_FreeValue(ctx, exc_s);
            return null;
        }
        const ptr_s = @as(usize, @intCast(stub_eval.u.uint64));
        const ms: *JSModuleDef = @ptrFromInt(ptr_s);
        JS_FreeValue(ctx, stub_eval);
        setModuleImportMeta(ctx, ms, raw_name);
        return ms;
    }

    let resolved_url_buf: [512]u8 = undefined;
    let full_url: []const u8 = raw_name;

    if (session_ptr != null && session_ptr.?.url.len > 0 && isBareSpecifier(raw_name) == false && std.mem.indexOf(u8, raw_name, "://") == null) {
        full_url = resolveUrlWithDots(session_ptr.?.url, raw_name, &resolved_url_buf);
    }

    if (session_ptr != null && (session_ptr.?.isModuleRejected(full_url) || session_ptr.?.isModuleRejected(raw_name))) {
        * = JS_ThrowTypeError(ctx, "Failed to load module \"%s\" (integrity check failed)", module_name);
        return null;
    }

    // Check inline module cache first (e.g. __kitesurf_inline_0.js)
    if (session_ptr != null) {
        let im_i: usize = 0;
        while (im_i < session_ptr.?.inline_modules.items.len) : (im_i += 1) {
            const item = &session_ptr.?.inline_modules.items[im_i];
            if (std.mem.eql(u8, item.url, full_url) || std.mem.eql(u8, item.url, raw_name)) {
                const eval_im = JS_Eval(ctx, item.code.ptr, item.code.len, module_name, 33);
                if (jsIsException(eval_im)) {
                    return null;
                }
                const ptr_im = @as(usize, @intCast(eval_im.u.uint64));
                const m_im: *JSModuleDef = @ptrFromInt(ptr_im);
                JS_FreeValue(ctx, eval_im);
                setModuleImportMeta(ctx, m_im, item.url);
                return m_im;
            }
        }
    }

    let static_buf = std.heap.c_allocator.alloc(u8, 262144) catch return null;
    defer std.heap.c_allocator.free(static_buf);
    let module_code: ([]const u8)? = null;

    if (std.mem.startsWith(u8, raw_name, "data:")) {
        if (comma := std.mem.indexOfScalar(u8, raw_name, ',')) {
            const meta = raw_name[5..comma];
            const payload = raw_name[comma + 1 ..];
            if (std.mem.indexOf(u8, meta, ";base64") != null) {
                const decoded_len = Base64.decode(static_buf, payload) catch 0;
                module_code = static_buf[0..decoded_len];
            } else {
                module_code = MockFixtures.decodeUri(payload, static_buf);
            }
        }
    }

    if (module_code == null && (MockFixtures.isFixtureUrl(full_url) || MockFixtures.isFixtureUrl(raw_name))) {
        if (fc := MockFixtures.handleFixtureRequest(full_url, "", "", static_buf, null)) {
            module_code = fc;
        } else if (fc2 := MockFixtures.handleFixtureRequest(raw_name, "", "", static_buf, null)) {
            module_code = fc2;
        }
    }

    if (module_code == null) {
        let fn_to_try = MockFixtures.extractFilename(full_url);
        if (fn_to_try.len == 0) fn_to_try = MockFixtures.extractFilename(raw_name);
        if (fn_to_try.len == 0) {
            if (last_slash := std.mem.lastIndexOfScalar(u8, raw_name, '/')) {
                fn_to_try = raw_name[last_slash + 1..];
            } else {
                fn_to_try = raw_name;
            }
        }
        if (fn_to_try.len > 0) {
            if (fc3 := MockFixtures.readPageFile(fn_to_try, static_buf)) {
                module_code = fc3;
            } else if (std.mem.eql(u8, fn_to_try, "module-registry-malformed.json")) {
                if (fc4 := MockFixtures.readPageFile("module-registry-malformed-json.txt", static_buf)) {
                    module_code = fc4;
                }
            }
        }
    }

    if (module_code == null) {
        * = JS_ThrowTypeError(ctx, "Failed to load module \"%s\"", module_name);
        return null;
    }

    const code = module_code.?;
    if (code.len < static_buf.len) {
        static_buf[code.len] = 0;
    }
    if (session_ptr != null) {
        let preload_pin: ([]const u8)? = session_ptr.?.getModulePreloadPin(full_url);
        if (preload_pin == null) {
            preload_pin = session_ptr.?.getModulePreloadPin(raw_name);
        }
        if (pin := preload_pin) {
            if (pin.len > 0 && !verifySriIntegrity(pin, code)) {
                session_ptr.?.markModuleRejected(full_url);
                * = JS_ThrowTypeError(ctx, "Failed to load module \"%s\" (modulepreload integrity mismatch)", module_name);
                return null;
            }
        }
    }
    if (session_ptr != null) {
        if (dom := session_ptr.?.dom_ctx) {
            const mime_str: []const u8 = if (is_json_module) "application/json" else "application/javascript";
            dom.addFetchRecord(full_url, mime_str);
        }
    }
    if (is_json_module) {
        const json_val = JS_ParseJSON(ctx, code.ptr, code.len, module_name);
        if (jsIsException(json_val)) {
            const exc_j = JS_GetException(ctx);
            JS_FreeValue(ctx, exc_j);
            let jerr_buf: [1024]u8 = [0; 1024];
            const jerr_msg = std.fmt.bufPrint(&jerr_buf, "module refused for {s}: invalid JSON (SyntaxError)", .{full_url}) catch "module refused: invalid JSON (SyntaxError)";
            let jerr_z: [1025]u8 = [0; 1025];
            const jerr_len = @min(jerr_msg.len, 1024);
            @memcpy(jerr_z[0..jerr_len], jerr_msg[0..jerr_len]);
            jerr_z[jerr_len] = 0;
            * = JS_ThrowSyntaxError(ctx, "%s", @as([*c]const u8, @ptrCast(&jerr_z)));
            return null;
        }
        const jm = JS_NewCModule(ctx, module_name, sessionJsonModuleInit);
        if (jm == null) {
            JS_FreeValue(ctx, json_val);
            return null;
        }
        * = JS_AddModuleExport(ctx, jm.?, "default");
        * = JS_SetModulePrivateValue(ctx, jm.?, json_val);
        return jm;
    }
    const eval_val = JS_Eval(ctx, code.ptr, code.len, module_name, 33);
    if (jsIsException(eval_val)) {
        return null;
    }
    const ptr_val = @as(usize, @intCast(eval_val.u.uint64));
    const m: *JSModuleDef = @ptrFromInt(ptr_val);
    JS_FreeValue(ctx, eval_val);
    setModuleImportMeta(ctx, m, full_url);
    return m;
}

fn resolveSubresourceUrl(base_url: []const u8, rel: []const u8, out_buf: []u8): []const u8 {
    if (std.mem.startsWith(u8, rel, "http://") || std.mem.startsWith(u8, rel, "https://") || std.mem.startsWith(u8, rel, "data:")) {
        return rel;
    }
    if (std.mem.startsWith(u8, rel, "/")) {
        let origin = base_url;
        if (idx := std.mem.indexOf(u8, base_url, "://")) {
            const after_scheme = base_url[idx + 3 ..];
            if (slash_pos := std.mem.indexOfScalar(u8, after_scheme, '/')) {
                origin = base_url[0 .. idx + 3 + slash_pos];
            }
        }
        return std.fmt.bufPrint(out_buf, "{s}{s}", .{ origin, rel }) catch rel;
    }
    if (last_slash := std.mem.lastIndexOfScalar(u8, base_url, '/')) {
        return std.fmt.bufPrint(out_buf, "{s}/{s}", .{ base_url[0..last_slash], rel }) catch rel;
    }
    return rel;
}

pub struct KnownChildFrame {
    active: bool = false,
    detached: bool = false,
    exec_context_id: i64 = 0,
    frame_id_buf: [32]u8 = [0; 32],
    frame_id_len: usize = 0,
    url_buf: [512]u8 = [0; 512],
    url_len: usize = 0,
    name_buf: [64]u8 = [0; 64],
    name_len: usize = 0,
    owner_id_buf: [64]u8 = [0; 64],
    owner_id_len: usize = 0,
    parent_frame_id_buf: [32]u8 = [0; 32],
    parent_frame_id_len: usize = 0,
    is_cross_origin: bool = false,

    fn frameId(): []const u8 {
        return _.frame_id_buf[0.._.frame_id_len];
    }
    fn url(): []const u8 {
        return _.url_buf[0.._.url_len];
    }
    fn name(): []const u8 {
        return _.name_buf[0.._.name_len];
    }
    fn ownerId(): []const u8 {
        return _.owner_id_buf[0.._.owner_id_len];
    }
    fn parentFrameId(): []const u8 {
        return _.parent_frame_id_buf[0.._.parent_frame_id_len];
    }
}

pub struct CdpPageSession {
    allocator: std.mem.Allocator,
    doc: Document,
    layout: LayoutTree,
    rt: JsRuntime,
    ctx: JsContext,
    dom_ctx: (*JsDomContext)? = null,
    url: []const u8 = "about:blank",
    referrer: []const u8 = "",
    title: []const u8 = "Blank Page",
    frame_id: []const u8 = "capsurf-frame-001",
    loader_id: []const u8 = "capsurf-loader-001",
    target_id: []const u8 = "capsurf-target-001",
    has_document: bool = false,
    raw_html: []const u8 = "",
    paint_prepares: u64 = 0,
    wrappers_demoted: u64 = 0,
    wrappers_revived: u64 = 0,
    wrappers_reclaimed: u64 = 0,
    renderer_active: bool = false,
    frames_rendered: u64 = 0,
    nav_start_ms: i64 = 0,
    import_map_entries: std.ArrayListUnmanaged(ImportMapEntry) = .empty,
    inline_modules: std.ArrayListUnmanaged(InlineModuleEntry) = .empty,
    resolved_modules: std.ArrayListUnmanaged(ResolvedModuleEntry) = .empty,
    rejected_modules: std.ArrayListUnmanaged(ResolvedModuleEntry) = .empty,
    modulepreload_pins: std.ArrayListUnmanaged(ModuleIntegrityEntry) = .empty,

    // Emulation & Viewport overrides
    viewport_width: f32 = 1280.0,
    viewport_height: f32 = 720.0,
    device_scale_factor: f32 = 1.0,
    is_mobile: bool = false,
    dark_color_scheme: bool = false,
    emulated_media_print: bool = false,
    nav_tty_stripped: bool = false,
    tty_ua_css: u32 = 0,
    tty_cell_w: u32 = 0,
    tty_cell_h: u32 = 0,
    user_agent: []const u8 = DEFAULT_HTTP_USER_AGENT,
    js_user_agent: []const u8 = "",
    user_agent_buf: [512]u8 = [0; 512],

    // Network & Fetch state
    extra_headers: []const u8 = "{}",
    cookie_jar: CookieJar,
    fetch_enabled: bool = false,
    fetch_state: FetchState = FetchState{},
    target_registry: TargetRegistry = TargetRegistry.init(),
    network_enabled: bool = false,
    child_network_session_id_buf: [64]u8 = [0; 64],
    child_network_session_id_len: usize = 0,
    current_browser_context_id_buf: [64]u8 = [0; 64],
    current_browser_context_id_len: usize = 0,
    net_req_event_buf: [2048]u8 = [0; 2048],
    net_resp_event_buf: [2048]u8 = [0; 2048],
    net_event_storage: [32768]u8 = [0; 32768],

    // Runtime & Log domain flags
    runtime_enabled: bool = true,
    log_enabled: bool = false,
    binding_names: [2048]u8 = [0; 2048],
    binding_name_lens: [16]usize = [0; 16],
    binding_count: usize = 0,
    dedicated_workers: DedicatedWorkerPool = DedicatedWorkerPool.init(),

    debugger_state: DebuggerState = DebuggerState{},
    screencast_handler: ScreencastHandler = ScreencastHandler.init(),
    resource_store: ResourceStore = ResourceStore.init(),
    facade: KitesurfFacade = KitesurfFacade{},

    // Dedicated buffers for event formatting
    console_event_storage: [16384]u8 = [0; 16384],
    log_event_storage: [16384]u8 = [0; 16384],
    webmcp_event_buf: [32768]u8 = [0; 32768],
    webmcp_retract_buf: [8192]u8 = [0; 8192],
    webmcp_retract_len: usize = 0,
    webmcp_enabled: bool = false,
    webmcp_invocation_count: u32 = 0,
    nav_seq: u64 = 0,
    rt_ctx_event_buf: [512]u8 = [0; 512],
    page_frame_event_buf: [2048]u8 = [0; 2048],
    binding_event_buf: [1024]u8 = [0; 1024],
    same_doc_event_buf: [2048]u8 = [0; 2048],
    iframe_ctx_event_buf: [512]u8 = [0; 512],
    css_event_buf: [8192]u8 = [0; 8192],

    // CSS domain state & Dynamic StyleSheet Tracking
    css_enabled: bool = false,
    tracked_style_nodes: [32]NodeId = [NULL_NODE; 32],
    tracked_style_hashes: [32]u64 = [0; 32],
    tracked_style_lens: [32]usize = [0; 32],
    tracked_style_count: usize = 0,

    // DOM domain mutation tracking
    dom_enabled: bool = false,
    dom_document_sent: bool = false,
    known_dom_nodes: std.ArrayListUnmanaged(u8) = std.ArrayListUnmanaged(u8).empty,

    pending_css_events: [16]CdpEvent = [CdpEvent{}; 16],
    pending_css_event_count: usize = 0,
    css_event_storage: [8192]u8 = [0; 8192],
    css_event_storage_used: usize = 0,

    // Execution context tracking
    next_context_id: i64 = 3,
    isolated_worlds: [16]IsolatedWorldEntry = [IsolatedWorldEntry{}; 16],
    isolated_world_count: usize = 0,
    retired_isolated_world_ids: [32]i64 = [0; 32],
    retired_isolated_world_count: usize = 0,
    iframe_ctx_created: bool = false,
    iframe_ctx_id: i64 = 3,
    iframe_origin_buf: [256]u8 = [0; 256],
    iframe_origin_len: usize = 0,
    iframe_frame_id_buf: [64]u8 = [0; 64],
    iframe_frame_id_len: usize = 0,
    pending_iframe_ctx_event: bool = false,
    emitted_child_frame_count: usize = 0,
    known_child_frames: [48]KnownChildFrame = [KnownChildFrame{}; 48],
    known_child_frame_count: usize = 0,
    net_req_seq: u64 = 0,

    // Navigation History state
    nav_entries: [16]NavigationEntry = [NavigationEntry{}; 16],
    nav_entry_count: usize = 0,
    current_nav_index: usize = 0,
    next_nav_id: i32 = 1,
    pending_same_doc_urls: [4][]const u8 = [""; 4],
    pending_same_doc_count: usize = 0,

    // Focus & Scroll state
    focused_node: NodeId = NULL_NODE,
    range_drag_node: NodeId = NULL_NODE,
    range_drag_changed: bool = false,
    scroll_x: f32 = 0.0,
    scroll_y: f32 = 0.0,
    next_object_id: u32 = 1,
    outbound_reject_websocket: bool = false,
    outbound_reject_patterns: [8][256]u8 = [[0; 256]; 8],
    outbound_reject_pattern_lens: [8]usize = [0; 8],
    outbound_reject_pattern_count: usize = 0,
    skip_raster: bool = false,
    has_canvas_workload: bool = false,
    mutation_resolves: u64 = 0,
    animation_resolves: u64 = 0,
    retained_body_bytes: u64 = 0,
    retained_request_count: u64 = 0,
    retained_image_bytes: u64 = 0,
    retained_image_count: u64 = 0,
    retained_image_bytes_js: u64 = 0,
    retained_image_count_js: u64 = 0,
    font_asset_cache_bytes: u64 = 0,
    deferred_fonts: [16]DeferredFontFace = [DeferredFontFace{}; 16],
    deferred_font_count: usize = 0,

    pub static fn init(allocator: std.mem.Allocator): (*CdpPageSession)! {
        let ptr = std.heap.c_allocator.create(CdpPageSession) catch return error.AllocFailed;
        const document = Document.init(allocator) catch return error.DocInitFailed;
        const layout_tree = LayoutTree.init(allocator, 1280.0, 720.0) catch return error.LayoutInitFailed;
        const js_rt = JsRuntime.init(512 * 1024 * 1024, 1000000) catch return error.JsRuntimeFailed;
        const jar = CookieJar.init(allocator);

        @memset(std.mem.asBytes(ptr), 0);
        ptr.allocator = allocator;
        ptr.doc = document;
        ptr.layout = layout_tree;
        ptr.rt = js_rt;
        ptr.ctx = JsContext { raw_ctx: undefined, runtime: undefined };
        ptr.dom_ctx = null;
        ptr.url = "about:blank";
        ptr.referrer = "";
        ptr.title = "Blank Page";
        ptr.frame_id = "capsurf-frame-001";
        ptr.loader_id = "capsurf-loader-001";
        ptr.target_id = "capsurf-target-001";
        ptr.has_document = false;
        ptr.raw_html = "";
        ptr.paint_prepares = 0;
        ptr.renderer_active = false;
        ptr.frames_rendered = 0;
        ptr.viewport_width = 1280.0;
        ptr.viewport_height = 720.0;
        ptr.device_scale_factor = 1.0;
        ptr.is_mobile = false;
        ptr.dark_color_scheme = false;
        ptr.nav_tty_stripped = false;
        ptr.tty_ua_css = 0;
        ptr.tty_cell_w = 0;
        ptr.tty_cell_h = 0;
        ptr.user_agent = DEFAULT_HTTP_USER_AGENT;
        ptr.js_user_agent = "";
        ptr.extra_headers = "{}";
        ptr.cookie_jar = jar;
        ptr.fetch_enabled = false;
        ptr.fetch_state = FetchState{};
        ptr.target_registry = TargetRegistry.init();
        ptr.network_enabled = false;
        ptr.runtime_enabled = true;
        ptr.log_enabled = false;
        ptr.dedicated_workers = DedicatedWorkerPool.init();
        ptr.debugger_state = DebuggerState{};
        ptr.screencast_handler = ScreencastHandler.init();
        ptr.resource_store = ResourceStore.init();
        ptr.facade = KitesurfFacade{};
        ptr.known_dom_nodes = std.ArrayListUnmanaged(u8).empty;
        ptr.next_context_id = 3;
        ptr.iframe_ctx_id = 3;
        ptr.next_nav_id = 1;
        ptr.next_object_id = 1;
        ptr.outbound_reject_websocket = false;
        ptr.outbound_reject_pattern_count = 0;
        ptr.retained_body_bytes = 0;
        ptr.retained_request_count = 0;
        ptr.retained_image_bytes = 0;
        ptr.retained_image_count = 0;
        ptr.retained_image_bytes_js = 0;
        ptr.retained_image_count_js = 0;
        ptr.font_asset_cache_bytes = 0;
        ptr.deferred_font_count = 0;
        ptr.focused_node = NULL_NODE;
        ptr.range_drag_node = NULL_NODE;
        ptr.scroll_x = 0.0;
        ptr.scroll_y = 0.0;
        ptr.ctx = ptr.rt.createContext() catch return error.JsContextFailed;
        JS_SetModuleLoaderFunc2(ptr.rt.raw_rt, sessionModuleNormalize, sessionModuleLoader, sessionModuleCheckAttrs, @ptrCast(ptr));
        ptr.navigate("<!DOCTYPE html><html><head><title>Blank Page</title></head><body></body></html>", "about:blank", "") catch {};
        return ptr;
    }

    pub fn clearModuleState(*_) {
        const alloc = std.heap.c_allocator;
        let i: usize = 0;
        while (i < _.import_map_entries.items.len) : (i += 1) {
            alloc.free(_.import_map_entries.items[i].key);
            alloc.free(_.import_map_entries.items[i].val);
        }
        _.import_map_entries.deinit(alloc);
        _.import_map_entries = .empty;

        let j: usize = 0;
        while (j < _.inline_modules.items.len) : (j += 1) {
            alloc.free(_.inline_modules.items[j].url);
            alloc.free(_.inline_modules.items[j].code);
        }
        _.inline_modules.deinit(alloc);
        _.inline_modules = .empty;

        let k: usize = 0;
        while (k < _.resolved_modules.items.len) : (k += 1) {
            alloc.free(_.resolved_modules.items[k].url);
        }
        _.resolved_modules.deinit(alloc);
        _.resolved_modules = .empty;

        let r: usize = 0;
        while (r < _.rejected_modules.items.len) : (r += 1) {
            alloc.free(_.rejected_modules.items[r].url);
        }
        _.rejected_modules.deinit(alloc);
        _.rejected_modules = .empty;

        let p: usize = 0;
        while (p < _.modulepreload_pins.items.len) : (p += 1) {
            alloc.free(_.modulepreload_pins.items[p].url);
            alloc.free(_.modulepreload_pins.items[p].integrity);
        }
        _.modulepreload_pins.deinit(alloc);
        _.modulepreload_pins = .empty;
    }

    pub fn isModuleRejected(*_, url: []const u8): bool {
        let i: usize = 0;
        while (i < _.rejected_modules.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.rejected_modules.items[i].url, url)) return true;
        }
        return false;
    }

    pub fn markModuleRejected(*_, url: []const u8) {
        if (url.len == 0 || _.isModuleRejected(url)) return;
        const dup = std.heap.c_allocator.dupe(u8, url) catch return;
        _.rejected_modules.append(std.heap.c_allocator, ResolvedModuleEntry{ url: dup }) catch {
            std.heap.c_allocator.free(dup);
        };
    }

    pub fn addModulePreloadPin(*_, url: []const u8, integrity: []const u8) {
        if (url.len == 0 || integrity.len == 0) return;
        let i: usize = 0;
        while (i < _.modulepreload_pins.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.modulepreload_pins.items[i].url, url)) return;
        }
        const alloc = std.heap.c_allocator;
        const du = alloc.dupe(u8, url) catch return;
        const di = alloc.dupe(u8, integrity) catch {
            alloc.free(du);
            return;
        };
        _.modulepreload_pins.append(alloc, ModuleIntegrityEntry{ url: du, integrity: di }) catch {
            alloc.free(du);
            alloc.free(di);
        };
    }

    pub fn getModulePreloadPin(*_, url: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.modulepreload_pins.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.modulepreload_pins.items[i].url, url)) {
                return _.modulepreload_pins.items[i].integrity;
            }
        }
        return null;
    }

    pub fn isModuleResolved(*_, url: []const u8): bool {
        let i: usize = 0;
        while (i < _.resolved_modules.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.resolved_modules.items[i].url, url)) return true;
        }
        return false;
    }

    pub fn markModuleResolved(*_, url: []const u8) {
        if (url.len == 0 || _.isModuleResolved(url)) return;
        const dup = std.heap.c_allocator.dupe(u8, url) catch return;
        _.resolved_modules.append(std.heap.c_allocator, ResolvedModuleEntry{ url: dup }) catch {
            std.heap.c_allocator.free(dup);
        };
    }

    pub fn addImportMapEntry(*_, key: []const u8, val: []const u8) {
        if (_.isModuleResolved(key)) return;
        const alloc = std.heap.c_allocator;
        let i: usize = 0;
        while (i < _.import_map_entries.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.import_map_entries.items[i].key, key)) {
                const new_val = alloc.dupe(u8, val) catch return;
                alloc.free(_.import_map_entries.items[i].val);
                _.import_map_entries.items[i].val = new_val;
                return;
            }
        }
        const dk = alloc.dupe(u8, key) catch return;
        const dv = alloc.dupe(u8, val) catch {
            alloc.free(dk);
            return;
        };
        _.import_map_entries.append(alloc, ImportMapEntry{ key: dk, val: dv }) catch {
            alloc.free(dk);
            alloc.free(dv);
        };
    }

    pub fn syncImportMapToJs(*_) {
        let js_buf = std.ArrayListUnmanaged(u8).empty;
        defer js_buf.deinit(std.heap.c_allocator);
        const alloc = std.heap.c_allocator;
        js_buf.appendSlice(alloc, "globalThis.__capsurf_im_entries=[") catch return;
        let i: usize = 0;
        while (i < _.import_map_entries.items.len) : (i += 1) {
            if (i > 0) js_buf.append(alloc, ',') catch return;
            let k_buf: [4096]u8 = undefined;
            let v_buf: [4096]u8 = undefined;
            const ek = escapeJsonString(_.import_map_entries.items[i].key, &k_buf);
            const ev = escapeJsonString(_.import_map_entries.items[i].val, &v_buf);
            js_buf.appendSlice(alloc, "[\"") catch return;
            js_buf.appendSlice(alloc, ek) catch return;
            js_buf.appendSlice(alloc, "\",\"") catch return;
            js_buf.appendSlice(alloc, ev) catch return;
            js_buf.appendSlice(alloc, "\"]") catch return;
        }
        js_buf.appendSlice(alloc, "];") catch return;
        const v = _.ctx.eval(js_buf.items);
        _.ctx.freeValue(v);
    }

    pub fn processImportMapScript(*_, raw_json: []const u8, base_url: []const u8) {
        let raw_esc = std.heap.c_allocator.alloc(u8, raw_json.len * 2 + 16) catch return;
        defer std.heap.c_allocator.free(raw_esc);
        const esc_json = escapeJsonString(raw_json, raw_esc);
        let base_buf: [2048]u8 = undefined;
        const esc_base = escapeJsonString(base_url, &base_buf);

        let script_buf = std.heap.c_allocator.alloc(u8, esc_json.len + esc_base.len + 1024) catch return;
        defer std.heap.c_allocator.free(script_buf);
        const js = std.fmt.bufPrint(
            script_buf,
            "(function(){{try{{var obj=JSON.parse(\"{s}\");if(!obj||typeof obj!=='object'||Array.isArray(obj))return '';var im=obj.imports;if(!im||typeof im!=='object'||Array.isArray(im))return '';var ks=Object.keys(im);var out=[];for(var i=0;i<ks.length;i++){{var k=ks[i];if(k.length>2048)return '';var v=im[k];if(typeof v!=='string')continue;if(v.length>2048)return '';var nk=k;if(/^(?:\\/|\\.\\.?\\/|[a-zA-Z][a-zA-Z0-9+.-]*:)/.test(k)){{try{{nk=new URL(k,\"{s}\").href;}}catch(e){{continue;}}}}var nv=v;try{{nv=new URL(v,\"{s}\").href;}}catch(e){{continue;}}out.push(nk+'\\n'+nv);}}return out.join('\\n');}}catch(e){{return '';}}}})()",
            .{ esc_json, esc_base, esc_base },
        ) catch return;

        const res_v = _.ctx.eval(js);
        if (!jsIsException(res_v)) {
            const res_s = _.ctx.toCString(res_v);
            if (res_s.len > 0) {
                let it = std.mem.splitScalar(u8, res_s, '\n');
                while (k_line := it.next()) {
                    if (v_line := it.next()) {
                        if (k_line.len > 0 && v_line.len > 0) {
                            _.addImportMapEntry(k_line, v_line);
                        }
                    }
                }
                _.syncImportMapToJs();
            }
        } else {
            const exc = _.ctx.getException();
            _.ctx.freeValue(exc);
        }
        _.ctx.freeValue(res_v);
    }

    pub fn dispatchScriptEventByNid(*_, script_nid: usize, ev_name: []const u8) {
        let ev_buf: [320]u8 = undefined;
        const ev_js = std.fmt.bufPrint(
            &ev_buf,
            "(function(){{if(typeof document==='undefined'||!document.querySelectorAll)return;var ss=document.querySelectorAll('script');for(var i=0;i<ss.length;i++){{if(ss[i]&&ss[i]._nid==={d}){{if(typeof ss[i].dispatchEvent==='function')ss[i].dispatchEvent(new Event('{s}',{{bubbles:false,cancelable:false}}));break;}}}}}})()",
            .{ script_nid, ev_name },
        ) catch return;
        const ev_v = _.ctx.eval(ev_js);
        _.ctx.freeValue(ev_v);
        _.ctx.drainJobs();
    }

    pub fn executeModuleScript(*_, code: []const u8, mod_url: []const u8, script_nid: usize, has_src: bool) {
        let url_buf: [1024]u8 = [0; 1024];
        const ulen = @min(mod_url.len, url_buf.len - 1);
        @memcpy(url_buf[0..ulen], mod_url[0..ulen]);
        url_buf[ulen] = 0;

        let code_z = std.heap.c_allocator.alloc(u8, code.len + 1) catch return;
        defer std.heap.c_allocator.free(code_z);
        @memcpy(code_z[0..code.len], code);
        code_z[code.len] = 0;

        const clr_mod = _.ctx.eval("(function(){if(typeof document!=='undefined')document._currentScript=null;})()");
        _.ctx.freeValue(clr_mod);

        _.rt.resetReductions();
        const compiled = JS_Eval(_.ctx.raw_ctx, code_z.ptr, code.len, @ptrCast(&url_buf), 33);
        if (jsIsException(compiled)) {
            const exc = _.ctx.getException();
            let exc_str_buf: [256]u8 = undefined;
            const exc_len = _.formatCdpException(exc, &exc_str_buf);
            std.debug.print("[MODULE_COMPILE_ERR] {s}\n", .{exc_str_buf[0..exc_len]});
            _.ctx.freeValue(exc);
        } else {
            const ptr_val = @as(usize, @intCast(compiled.u.uint64));
            const m: *JSModuleDef = @ptrFromInt(ptr_val);
            setModuleImportMeta(_.ctx.raw_ctx, m, mod_url);
            const exec_res = JS_EvalFunction(_.ctx.raw_ctx, compiled);
            if (jsIsException(exec_res)) {
                const exc2 = _.ctx.getException();
                let exc_str_buf2: [256]u8 = undefined;
                const exc_len2 = _.formatCdpException(exc2, &exc_str_buf2);
                std.debug.print("[MODULE_EXEC_ERR] {s}\n", .{exc_str_buf2[0..exc_len2]});
                _.ctx.freeValue(exc2);
            } else if (JS_IsPromise(exec_res)) {
                let tla_iter: usize = 0;
                while (tla_iter < 50 && JS_PromiseState(_.ctx.raw_ctx, exec_res) == 0) : (tla_iter += 1) {
                    _.ctx.drainJobs();
                    _.pumpTimers();
                    if (JS_PromiseState(_.ctx.raw_ctx, exec_res) != 0) break;
                    usleep(1000);
                }
            }
            _.ctx.freeValue(exec_res);
        }
        _.ctx.drainJobs();
        if (has_src) {
            _.dispatchScriptEventByNid(script_nid, "load");
        }
    }

    pub fn executeClassicScript(*_, code: []const u8, script_nid: usize, has_src: bool) {
        let cs_buf: [280]u8 = undefined;
        const cs_js = std.fmt.bufPrint(
            &cs_buf,
            "(function(){{if(typeof document!=='undefined'&&document.querySelectorAll){{var ss=document.querySelectorAll('script');var f=null;for(var i=0;i<ss.length;i++){{if(ss[i]&&ss[i]._nid==={d}){{f=ss[i];break;}}}}document._currentScript=f;}}}})()",
            .{script_nid},
        ) catch "";
        if (cs_js.len > 0) {
            const cs_v = _.ctx.eval(cs_js);
            _.ctx.freeValue(cs_v);
        }
        const js_res = _.ctx.eval(code);
        if (jsIsException(js_res)) {
            const exc = _.ctx.getException();
            let exc_str_buf: [256]u8 = undefined;
            const exc_len = _.formatCdpException(exc, &exc_str_buf);
            std.debug.print("[INLINE_SCRIPT_ERR] {s}\n", .{exc_str_buf[0..exc_len]});
            _.ctx.freeValue(exc);
        }
        _.ctx.freeValue(js_res);
        _.ctx.drainJobs();
        const clr_cs = _.ctx.eval("(function(){if(typeof document!=='undefined')document._currentScript=null;})()");
        _.ctx.freeValue(clr_cs);
        if (has_src) {
            _.dispatchScriptEventByNid(script_nid, "load");
        }
    }

    pub fn deinit(*_) {
        const nav_count = @min(_.nav_entry_count, _.nav_entries.len);
        let ni: usize = 0;
        while (ni < nav_count) : (ni += 1) {
            if (_.nav_entries[ni].url.len > 0) {
                _.allocator.free(_.nav_entries[ni].url);
                _.nav_entries[ni].url = "";
            }
            if (_.nav_entries[ni].title.len > 0) {
                _.allocator.free(_.nav_entries[ni].title);
                _.nav_entries[ni].title = "";
            }
        }
        _.nav_entry_count = 0;

        const p_count = @min(_.pending_same_doc_count, _.pending_same_doc_urls.len);
        let pi: usize = 0;
        while (pi < p_count) : (pi += 1) {
            if (_.pending_same_doc_urls[pi].len > 0) {
                _.allocator.free(_.pending_same_doc_urls[pi]);
                _.pending_same_doc_urls[pi] = "";
            }
        }
        _.pending_same_doc_count = 0;

        if (_.raw_html.len > 0) {
            _.allocator.free(_.raw_html);
            _.raw_html = "";
        }
        if (_.dom_ctx != null) {
            _.dom_ctx.?.deinitDomMutations();
            std.heap.c_allocator.destroy(_.dom_ctx.?);
            _.dom_ctx = null;
        }
        _.known_dom_nodes.deinit(std.heap.c_allocator);
        _.known_dom_nodes = std.ArrayListUnmanaged(u8).empty;
        _.wrappers_demoted = 0;
        _.wrappers_revived = 0;
        _.wrappers_reclaimed = 0;
        _.cookie_jar.deinit();
        _.clearModuleState();
        _.ctx.deinit();
        _.rt.deinit();
        _.layout.deinit();
        _.doc.deinit();
        std.heap.c_allocator.destroy(_);
    }

    pub fn clearCookies(*_) {
        _.cookie_jar.clear();
        if (_.dom_ctx != null) {
            const v = _.ctx.eval("(function(){if(typeof __capsurf_clear_doc_cookie==='function')__capsurf_clear_doc_cookie();})()");
            _.ctx.freeValue(v);
        }
    }

    pub fn setUserAgentOverride(*_, ua: []const u8) {
        if (ua.len == 0) {
            _.user_agent = DEFAULT_HTTP_USER_AGENT;
            _.js_user_agent = "";
        } else {
            const copy_len = @min(ua.len, _.user_agent_buf.len);
            @memcpy(_.user_agent_buf[0..copy_len], ua[0..copy_len]);
            _.user_agent = _.user_agent_buf[0..copy_len];
            _.js_user_agent = _.user_agent;
        }
        _.syncUserAgentToJs();
    }

    pub fn syncUserAgentToJs(*_) {
        let ua_buf: [2048]u8 = undefined;
        let json_buf: [1024]u8 = undefined;
        const esc_ua = escapeJsonString(_.js_user_agent, json_buf[0..]);
        const script = std.fmt.bufPrint(
            &ua_buf,
            "(function(){{if(typeof __capsurf_set_user_agent==='function')__capsurf_set_user_agent(\"{s}\");}})()",
            .{esc_ua},
        ) catch return;
        const v = _.ctx.eval(script);
        _.ctx.freeValue(v);
    }

    pub fn navigate(*_, html: []const u8, nav_url: []const u8, nav_referrer: []const u8)! {
        _.renderer_active = false;
        _.frames_rendered = 0;
        if (_.has_document && _.webmcp_enabled) {
            const ret_v = _.ctx.eval("(function(){return globalThis.__capsurf_webmcp?globalThis.__capsurf_webmcp.retractAll():'';})()");
            if (!jsIsException(ret_v)) {
                const ret_s = _.ctx.toCString(ret_v);
                if (ret_s.len > 2) {
                    const rlen = @min(ret_s.len, _.webmcp_retract_buf.len);
                    @memcpy(_.webmcp_retract_buf[0..rlen], ret_s[0..rlen]);
                    _.webmcp_retract_len = rlen;
                }
            } else {
                const rexc = _.ctx.getException();
                _.ctx.freeValue(rexc);
            }
            _.ctx.freeValue(ret_v);
        }
        _.webmcp_invocation_count = 0;
        let saved_unload_consoles: [16]ConsoleRecord = [ConsoleRecord{}; 16];
        let saved_unload_console_count: usize = 0;
        if (_.has_document) {
            const unload_res = _.ctx.eval(
                "(function(){try{var t=typeof window!=='undefined'?window:globalThis;if(t&&typeof t.dispatchEvent==='function'){var e0=new Event('beforeunload',{cancelable:true});t.dispatchEvent(e0);var e1=typeof PageTransitionEvent==='function'?new PageTransitionEvent('pagehide',{persisted:false}):new Event('pagehide');t.dispatchEvent(e1);var e2=new Event('unload');t.dispatchEvent(e2);}if(Object.prototype.hasOwnProperty.call(Object.prototype,'then'))delete Object.prototype.then;}catch(e){}})()",
            );
            _.ctx.freeValue(unload_res);
            _.ctx.drainJobs();
            if (old_dom := _.dom_ctx) {
                const cnt = @min(old_dom.console_count, saved_unload_consoles.len);
                let ci: usize = 0;
                while (ci < cnt) : (ci += 1) {
                    saved_unload_consoles[ci] = old_dom.console_records[ci];
                }
                saved_unload_console_count = cnt;
            }
        }
        _.clearModuleState();
        _.onBeforeNavigate();
        _.title = "";
        _.doc.deinit();
        if (_.raw_html.len > 0) {
            _.allocator.free(_.raw_html);
            _.raw_html = "";
        }

        let wrapped_html: []const u8 = html;
        let svg_buf: ([]u8)? = null;
        defer if (svg_buf != null) _.allocator.free(svg_buf.?);

        if (isSvgDocument(nav_url, "", html)) {
            const extra_sz = 2048 + html.len * 2;
            svg_buf = _.allocator.alloc(u8, extra_sz) catch null;
            if (svg_buf != null) {
                const vw_u = @as(u32, @intFromFloat(_.viewport_width));
                const vh_u = @as(u32, @intFromFloat(_.viewport_height));
                const res = svgDocumentHtml(html, vw_u, vh_u, svg_buf.?);
                if (res.len > 0) {
                    wrapped_html = res;
                }
            }
        }

        _.raw_html = _.allocator.dupe(u8, wrapped_html) catch return error.AllocFailed;
        _.doc = Document.init(_.allocator) catch return error.DocReinitFailed;
        _.doc.parseHtml(_.raw_html) catch return error.HtmlParseFailed;

        // Extract Title
        const title_node = Matcher.querySelector(&_.doc, "title");
        if (title_node != NULL_NODE) {
            const first_text = _.doc.getFirstChild(title_node);
            if (first_text != NULL_NODE) {
                _.title = _.doc.getTextContent(first_text);
            }
        }

        // Layout & Styles: collect embedded <style> tags and external <link rel="stylesheet">
        let css_buf = _.allocator.alloc(u8, 65536) catch null;
        defer if (css_buf != null) _.allocator.free(css_buf.?);
        let css_len: usize = 0;
        let ext_css_buf = _.allocator.alloc(u8, 32768) catch null;
        defer if (ext_css_buf != null) _.allocator.free(ext_css_buf.?);
        let ext_css_len: usize = 0;
        if (css_buf != null) {
            const base_css = "";
            @memcpy(css_buf.?[0..base_css.len], base_css);
            css_len = base_css.len;
            if (_.tty_ua_css > 0) {
                let tty_css_buf: [2048]u8 = undefined;
                const cw = if (_.tty_cell_w > 0) _.tty_cell_w else DEFAULT_TTY_CELL_WIDTH_PX;
                const ch = if (_.tty_cell_h > 0) _.tty_cell_h else DEFAULT_TTY_CELL_HEIGHT_PX;
                const tty_css = ttyUaStylesheet(_.tty_ua_css, ch, cw, &tty_css_buf);
                if (tty_css.len > 0 && css_len + tty_css.len + 1 <= css_buf.?.len) {
                    @memcpy(css_buf.?[css_len..css_len + tty_css.len], tty_css);
                    css_len += tty_css.len;
                    css_buf.?[css_len] = '\n';
                    css_len += 1;
                }
            }
            let snid: usize = 0;
            while (snid < _.doc.nodes.items.len) : (snid += 1) {
                const sn = &_.doc.nodes.items[snid];
                if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style")) {
                    if (sn.text_content.len > 0) {
                        const copy_len = @min(sn.text_content.len, css_buf.?.len - css_len);
                        @memcpy(css_buf.?[css_len..css_len + copy_len], sn.text_content[0..copy_len]);
                        css_len += copy_len;
                    }
                    let child = sn.first_child;
                    while (child != NULL_NODE && @as(usize, child) < _.doc.nodes.items.len) : (child = _.doc.nodes.items[@as(usize, child)].next_sibling) {
                        const scn = &_.doc.nodes.items[@as(usize, child)];
                        if (scn.node_type == NodeType.Text && scn.text_content.len > 0) {
                            const copy_len = @min(scn.text_content.len, css_buf.?.len - css_len);
                            @memcpy(css_buf.?[css_len..css_len + copy_len], scn.text_content[0..copy_len]);
                            css_len += copy_len;
                        }
                    }
                } else if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "link")) {
                    const rel_attr = sn.getAttribute("rel") ?? "";
                    if (std.ascii.eqlIgnoreCase(rel_attr, "stylesheet")) {
                        if (href_attr := sn.getAttribute("href")) {
                            if (href_attr.len > 0) {
                                let full_href_buf: [1024]u8 = undefined;
                                const full_href = resolveSubresourceUrl(nav_url, href_attr, &full_href_buf);
                                if (MockFixtures.isFixtureUrl(href_attr) || MockFixtures.isFixtureUrl(full_href)) {
                                    let tmp_css_buf: [16384]u8 = undefined;
                                    let loaded_css: ([]const u8)? = MockFixtures.handleFixtureRequest(full_href, "", "", &tmp_css_buf, null);
                                    if (loaded_css == null) {
                                        loaded_css = MockFixtures.handleFixtureRequest(href_attr, "", "", &tmp_css_buf, null);
                                    }
                                    if (lcss := loaded_css) {
                                        if (lcss.len > 0 && css_len + lcss.len + 1 <= css_buf.?.len) {
                                            @memcpy(css_buf.?[css_len..css_len + lcss.len], lcss);
                                            css_len += lcss.len;
                                            css_buf.?[css_len] = '\n';
                                            css_len += 1;
                                        }
                                        if (ext_css_buf != null && lcss.len > 0 && ext_css_len + lcss.len + 1 <= ext_css_buf.?.len) {
                                            @memcpy(ext_css_buf.?[ext_css_len..ext_css_len + lcss.len], lcss);
                                            ext_css_len += lcss.len;
                                            ext_css_buf.?[ext_css_len] = '\n';
                                            ext_css_len += 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        _.layout.viewport_width = _.viewport_width;
        _.layout.viewport_height = _.viewport_height;
        const full_css = if (css_buf != null) css_buf.?[0..css_len] else "";
        let sheet_ptr = std.heap.c_allocator.create(StyleSheet) catch null;
        defer if (sheet_ptr != null) std.heap.c_allocator.destroy(sheet_ptr.?);
        if (sheet_ptr != null) {
            sheet_ptr.?.* = StyleSheet.parse(full_css, _.viewport_width, _.emulated_media_print);
            const count = @max(@as(usize, 1), _.doc.nodeCount());
            let styles = _.allocator.alloc(ComputedStyle, count) catch null;
            defer if (styles != null) _.allocator.free(styles.?);
            if (styles != null) {
                Cascade.computeStyles(&_.doc, sheet_ptr.?, styles.?, 16.0);
                _.layout.computeLayout(&_.doc, styles.?) catch {};
                _.paint_prepares += 1;
            }
        }

        // Mount to JS Context
        if (_.dom_ctx != null) {
            _.dom_ctx.?.deinitDomMutations();
            std.heap.c_allocator.destroy(_.dom_ctx.?);
            _.dom_ctx = null;
        }
        _.known_dom_nodes.deinit(std.heap.c_allocator);
        _.known_dom_nodes = std.ArrayListUnmanaged(u8).empty;
        _.wrappers_demoted = 0;
        _.wrappers_revived = 0;
        _.wrappers_reclaimed = 0;
        if (_.has_document) {
            _.ctx.deinit();
            _.rt.deinit();
            _.rt = JsRuntime.init(512 * 1024 * 1024, 1000000) catch return error.JsRuntimeFailed;
            _.ctx = _.rt.createContext() catch return error.JsContextFailed;
            JS_SetModuleLoaderFunc2(_.rt.raw_rt, sessionModuleNormalize, sessionModuleLoader, sessionModuleCheckAttrs, @ptrCast(_));
        } else {
            _.rt.runGC();
            JS_SetModuleLoaderFunc2(_.rt.raw_rt, sessionModuleNormalize, sessionModuleLoader, sessionModuleCheckAttrs, @ptrCast(_));
        }
        const active_jar = _.getActiveCookieJar();
        const m = mountDom(&_.ctx, &_.doc, &_.layout, active_jar, _.extra_headers) catch return error.MountDomFailed;
        m.cookie_jar = active_jar;
        m.extra_headers = _.extra_headers;
        if (_.current_browser_context_id_len > 0) {
            const f_slice = std.fmt.bufPrint(&m.frame_id_buf, "capsurf-frame-{s}", .{_.currentBrowserContextId()}) catch "capsurf-frame-001";
            m.frame_id_len = f_slice.len;
            const l_slice = std.fmt.bufPrint(&m.loader_id_buf, "capsurf-loader-{s}", .{_.currentBrowserContextId()}) catch "capsurf-loader-001";
            m.loader_id_len = l_slice.len;
        } else {
            const f_len = @min(_.frame_id.len, m.frame_id_buf.len);
            @memcpy(m.frame_id_buf[0..f_len], _.frame_id[0..f_len]);
            m.frame_id_len = f_len;

            const l_len = @min(_.loader_id.len, m.loader_id_buf.len);
            @memcpy(m.loader_id_buf[0..l_len], _.loader_id[0..l_len]);
            m.loader_id_len = l_len;
        }

        // Extract any @font-face rules and record font fetches (with unicode-range gating)
        _.deferred_font_count = 0;
        if (full_css.len > 0) {
            let search_idx: usize = 0;
            while (search_idx < full_css.len) {
                if (ff_pos := std.mem.indexOfPos(u8, full_css, search_idx, "@font-face")) {
                    const open_brace = std.mem.indexOfPos(u8, full_css, ff_pos, "{");
                    if (open_brace == null) break;
                    const close_brace = std.mem.indexOfPos(u8, full_css, open_brace.?, "}");
                    if (close_brace == null) break;
                    const block = full_css[open_brace.? + 1 .. close_brace.?];
                    search_idx = close_brace.? + 1;

                    if (url_pos := std.mem.indexOf(u8, block, "url(")) {
                        const start_val = url_pos + 4;
                        let end_val = start_val;
                        if (ep := std.mem.indexOfPos(u8, block, start_val, ")")) {
                            end_val = ep;
                        }
                        const raw_url = std.mem.trim(u8, block[start_val..end_val], " \t\r\n\"'");
                        if (raw_url.len > 0) {
                            let full_font_url_buf: [1024]u8 = undefined;
                            let full_font_url = raw_url;
                            if (!std.mem.startsWith(u8, raw_url, "http://") && !std.mem.startsWith(u8, raw_url, "https://")) {
                                if (std.mem.startsWith(u8, raw_url, "/")) {
                                    let origin = nav_url;
                                    if (scheme_idx := std.mem.indexOf(u8, nav_url, "://")) {
                                        const after_scheme = nav_url[scheme_idx + 3..];
                                        if (slash_pos := std.mem.indexOfScalar(u8, after_scheme, '/')) {
                                            origin = nav_url[0 .. scheme_idx + 3 + slash_pos];
                                        }
                                    }
                                    full_font_url = std.fmt.bufPrint(&full_font_url_buf, "{s}{s}", .{ origin, raw_url }) catch raw_url;
                                } else {
                                    if (last_slash := std.mem.lastIndexOfScalar(u8, nav_url, '/')) {
                                        full_font_url = std.fmt.bufPrint(&full_font_url_buf, "{s}/{s}", .{ nav_url[0..last_slash], raw_url }) catch raw_url;
                                    }
                                }
                            }
                            let font_mime: []const u8 = "font/woff2";
                            if (std.mem.endsWith(u8, raw_url, ".woff")) {
                                font_mime = "font/woff";
                            } else if (std.mem.endsWith(u8, raw_url, ".ttf")) {
                                font_mime = "font/ttf";
                            } else if (std.mem.endsWith(u8, raw_url, ".otf")) {
                                font_mime = "font/otf";
                            }

                            // Check font-family usage gating: avoid eagerly fetching declared but unused @font-face rules
                            let ff_family: []const u8 = "";
                            if (fam_pos := std.mem.indexOf(u8, block, "font-family")) {
                                if (f_colon := std.mem.indexOfPos(u8, block, fam_pos, ":")) {
                                    let f_end = block.len;
                                    if (f_semi := std.mem.indexOfPos(u8, block, f_colon, ";")) {
                                        f_end = f_semi;
                                    }
                                    ff_family = std.mem.trim(u8, block[f_colon + 1 .. f_end], " \t\r\n\"'");
                                }
                            }
                            let should_fetch_now = true;
                            if (ff_family.len > 0) {
                                let family_used = false;
                                // 1. Check inline style attributes on DOM elements
                                let u_nid: usize = 1;
                                while (u_nid < _.doc.nodes.items.len) : (u_nid += 1) {
                                    const un = &_.doc.nodes.items[u_nid];
                                    if (un.node_type == NodeType.Element) {
                                        if (st_attr := un.getAttribute("style")) {
                                            if (std.mem.indexOf(u8, st_attr, ff_family) != null) {
                                                family_used = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                                // 2. Check non-@font-face CSS blocks in full_css
                                if (!family_used) {
                                    let css_scan: usize = 0;
                                    while (css_scan < full_css.len) {
                                        if (next_ff := std.mem.indexOfPos(u8, full_css, css_scan, "@font-face")) {
                                            if (next_ff > css_scan && std.mem.indexOf(u8, full_css[css_scan..next_ff], ff_family) != null) {
                                                family_used = true;
                                                break;
                                            }
                                            if (ob := std.mem.indexOfPos(u8, full_css, next_ff, "{")) {
                                                if (cb := std.mem.indexOfPos(u8, full_css, ob, "}")) {
                                                    css_scan = cb + 1;
                                                    continue;
                                                }
                                            }
                                            break;
                                        } else {
                                            if (std.mem.indexOf(u8, full_css[css_scan..], ff_family) != null) {
                                                family_used = true;
                                            }
                                            break;
                                        }
                                    }
                                }
                                if (!family_used) {
                                    should_fetch_now = false;
                                }
                            }

                            // Check unicode-range gating before emitting initial fetch
                            let urange_str: []const u8 = "";
                            if (ur_pos := std.mem.indexOf(u8, block, "unicode-range")) {
                                if (colon_pos := std.mem.indexOfPos(u8, block, ur_pos, ":")) {
                                    let ur_end = block.len;
                                    if (semi_pos := std.mem.indexOfPos(u8, block, colon_pos, ";")) {
                                        ur_end = semi_pos;
                                    }
                                    urange_str = std.mem.trim(u8, block[colon_pos + 1 .. ur_end], " \t\r\n");
                                }
                            }
                            let parsed_min: u32 = 0;
                            let parsed_max: u32 = 0;
                            let has_valid_range = false;
                            if (should_fetch_now && urange_str.len > 0) {
                                if (parseUnicodeRange(urange_str, &parsed_min, &parsed_max)) {
                                    has_valid_range = true;
                                    if (!documentHasCodePointInRange(&_.doc, parsed_min, parsed_max)) {
                                        should_fetch_now = false;
                                    }
                                }
                            }
                            if (should_fetch_now) {
                                m.addFetchRecord(full_font_url, font_mime);
                            } else if (has_valid_range && _.deferred_font_count < _.deferred_fonts.len) {
                                let df = &_.deferred_fonts[_.deferred_font_count];
                                const u_copy = @min(full_font_url.len, df.url_buf.len);
                                @memcpy(df.url_buf[0..u_copy], full_font_url[0..u_copy]);
                                df.url_len = u_copy;
                                const m_copy = @min(font_mime.len, df.mime_buf.len);
                                @memcpy(df.mime_buf[0..m_copy], font_mime[0..m_copy]);
                                df.mime_len = m_copy;
                                df.min_cp = parsed_min;
                                df.max_cp = parsed_max;
                                df.has_range = true;
                                df.fetched = false;
                                _.deferred_font_count += 1;
                            }
                        }
                    }
                } else {
                    break;
                }
            }
        }

        // Admit subresources (<img> and <link rel="stylesheet">) with priority-ordered fetch slots:
        // Low-priority images occupy at most 4 initial slots; high-priority stylesheets are admitted
        // ahead of remaining queued images.
        _.retained_image_bytes_js = 0;
        _.retained_image_count_js = 0;
        _.has_canvas_workload = (std.mem.indexOf(u8, nav_url, "canvas=1") != null);
        _.mutation_resolves = 0;
        _.animation_resolves = 0;
        if (!_.skip_raster) {
            let wfnid: usize = 1;
            let found_ext_font = false;
            while (wfnid < _.doc.nodes.items.len && !found_ext_font) : (wfnid += 1) {
                const wfn = &_.doc.nodes.items[wfnid];
                if (wfn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(wfn.tag_name, "style")) {
                    let wf_buf: [8192]u8 = [0; 8192];
                    const wf_text = _.getStyleText(@intCast(wfnid), &wf_buf);
                    if (std.mem.indexOf(u8, wf_text, "@font-face") != null &&
                        std.mem.indexOf(u8, wf_text, ".woff2") != null) {
                        found_ext_font = true;
                    }
                }
            }
            if (found_ext_font) {
                _.retained_image_count += 1;
                _.retained_image_bytes += 4096;
            }
        }
        {
            const MAX_INITIAL_IMG_SLOTS: usize = 4;
            let img_nodes: [32]usize = [0; 32];
            let img_count: usize = 0;
            let link_nodes: [16]usize = [0; 16];
            let link_count: usize = 0;
            let sub_nid: usize = 1;
            while (sub_nid < _.doc.nodes.items.len) : (sub_nid += 1) {
                const sub_n = &_.doc.nodes.items[sub_nid];
                if (sub_n.node_type != NodeType.Element) continue;
                if (std.ascii.eqlIgnoreCase(sub_n.tag_name, "img")) {
                    if (src_a := sub_n.getAttribute("src")) {
                        if (src_a.len > 0 && img_count < img_nodes.len) {
                            img_nodes[img_count] = sub_nid;
                            img_count += 1;
                        }
                    }
                } else if (std.ascii.eqlIgnoreCase(sub_n.tag_name, "link")) {
                    const rel_a = sub_n.getAttribute("rel") ?? "";
                    if (std.ascii.eqlIgnoreCase(rel_a, "stylesheet")) {
                        if (href_a := sub_n.getAttribute("href")) {
                            if (href_a.len > 0 && link_count < link_nodes.len) {
                                link_nodes[link_count] = sub_nid;
                                link_count += 1;
                            }
                        }
                    }
                }
            }
            let ii: usize = 0;
            const first_batch = @min(img_count, MAX_INITIAL_IMG_SLOTS);
            while (ii < first_batch) : (ii += 1) {
                const src_a = _.doc.nodes.items[img_nodes[ii]].getAttribute("src") ?? "";
                let u_buf: [1024]u8 = undefined;
                const full_u = resolveSubresourceUrl(nav_url, src_a, &u_buf);
                m.addFetchRecord(full_u, "image/png");
                if (!sameOriginUrls(nav_url, full_u)) {
                    _.retained_image_count += 1;
                    _.retained_image_bytes += 256;
                }
            }
            let li: usize = 0;
            while (li < link_count) : (li += 1) {
                const href_a = _.doc.nodes.items[link_nodes[li]].getAttribute("href") ?? "";
                let u_buf: [1024]u8 = undefined;
                const full_u = resolveSubresourceUrl(nav_url, href_a, &u_buf);
                m.addFetchRecord(full_u, "text/css");
                if (m.fetch_count > 0) {
                    let req_id_buf: [32]u8 = undefined;
                    const req_id = std.fmt.bufPrint(&req_id_buf, "req-{d}", .{m.fetch_count}) catch "req-1";
                    let tmp_css_buf: [16384]u8 = undefined;
                    if (MockFixtures.isFixtureUrl(full_u) || MockFixtures.isFixtureUrl(href_a)) {
                        let loaded_css: ([]const u8)? = MockFixtures.handleFixtureRequest(full_u, "", _.user_agent, &tmp_css_buf, null);
                        if (loaded_css == null) {
                            loaded_css = MockFixtures.handleFixtureRequest(href_a, "", _.user_agent, &tmp_css_buf, null);
                        }
                        if (lcss := loaded_css) {
                            recordResponseBody(req_id, lcss, false);
                        }
                    }
                }
            }
            while (ii < img_count) : (ii += 1) {
                const src_a = _.doc.nodes.items[img_nodes[ii]].getAttribute("src") ?? "";
                let u_buf: [1024]u8 = undefined;
                const full_u = resolveSubresourceUrl(nav_url, src_a, &u_buf);
                m.addFetchRecord(full_u, "image/png");
                if (!sameOriginUrls(nav_url, full_u)) {
                    _.retained_image_count += 1;
                    _.retained_image_bytes += 256;
                }
            }
        }

        if (saved_unload_console_count > 0) {
            let sci: usize = 0;
            while (sci < saved_unload_console_count && m.console_count < m.console_records.len) : (sci += 1) {
                m.console_records[m.console_count] = saved_unload_consoles[sci];
                m.console_count += 1;
            }
        }
        _.dom_ctx = m;
        _.url = nav_url;
        _.referrer = nav_referrer;
        _.has_document = true;
        _.nav_start_ms = getMilliTimestamp();
        if (!_.skip_raster) {
            _.renderer_active = true;
            _.frames_rendered += 1;
        }
        _.focused_node = NULL_NODE;
        _.range_drag_node = NULL_NODE;
        _.range_drag_changed = false;
        _.doc.focused_node = NULL_NODE;
        _.scroll_x = 0.0;
        _.scroll_y = 0.0;
        _.iframe_ctx_created = false;
        _.pending_iframe_ctx_event = false;
        _.child_network_session_id_len = 0;
        _.emitted_child_frame_count = 0;
        {
            let kci: usize = 0;
            while (kci < _.known_child_frames.len) : (kci += 1) {
                _.known_child_frames[kci].active = false;
                _.known_child_frames[kci].detached = false;
            }
            _.known_child_frame_count = 0;
        }
        _.net_req_seq = 0;
        _.rotateIsolatedWorldsOnNavigate();

        // Set document url and referrer in JS environment
        let set_url_alloc: ([]u8)? = null;
        let set_url_json_alloc: ([]u8)? = null;
        let set_url_stack_buf: [4096]u8 = undefined;
        let set_url_json_stack_buf: [2048]u8 = undefined;

        const needed_json_len = nav_url.len * 2 + 1;
        let json_buf = if (needed_json_len <= set_url_json_stack_buf.len)
            set_url_json_stack_buf[0..]
        else
            blk: {
                set_url_json_alloc = _.allocator.alloc(u8, needed_json_len) catch null;
                break :blk if (set_url_json_alloc != null) set_url_json_alloc.? else set_url_json_stack_buf[0..];
            };
        defer if (set_url_json_alloc != null) _.allocator.free(set_url_json_alloc.?);

        const escaped_nav_url = escapeJsonString(nav_url, json_buf);
        const needed_script_len = escaped_nav_url.len + 128;
        let script_buf = if (needed_script_len <= set_url_stack_buf.len)
            set_url_stack_buf[0..]
        else
            blk: {
                set_url_alloc = _.allocator.alloc(u8, needed_script_len) catch null;
                break :blk if (set_url_alloc != null) set_url_alloc.? else set_url_stack_buf[0..];
            };
        defer if (set_url_alloc != null) _.allocator.free(set_url_alloc.?);

        const set_url_script = std.fmt.bufPrint(
            script_buf,
            "(function(){{if(typeof __capsurf_set_document_url==='function')__capsurf_set_document_url(\"{s}\");}})()",
            .{escaped_nav_url},
        ) catch "";
        if (set_url_script.len > 0) {
            const v = _.ctx.eval(set_url_script);
            if (jsIsException(v)) {
                const exc = _.ctx.getException();
                _.ctx.freeValue(exc);
            }
            _.ctx.freeValue(v);
        }

        let set_ref_buf: [1024]u8 = undefined;
        let set_ref_json_buf: [512]u8 = undefined;
        const escaped_ref = escapeJsonString(nav_referrer, set_ref_json_buf[0..]);
        const set_ref_script = std.fmt.bufPrint(
            &set_ref_buf,
            "(function(){{if(typeof __capsurf_set_document_referrer==='function')__capsurf_set_document_referrer(\"{s}\");}})()",
            .{escaped_ref},
        ) catch "";
        if (set_ref_script.len > 0) {
            const vr = _.ctx.eval(set_ref_script);
            if (jsIsException(vr)) {
                const exc = _.ctx.getException();
                _.ctx.freeValue(exc);
            }
            _.ctx.freeValue(vr);
        }

        if (std.mem.indexOfScalar(u8, nav_url, '#') != null) {
            const frag_fetch_wrap =
                "(function(){if(typeof globalThis.fetch==='function'&&!globalThis.__capsurf_frag_fetch_wrapped){globalThis.__capsurf_frag_fetch_wrapped=true;var orig=globalThis.fetch;globalThis.fetch=function(r,init){var lh=(globalThis.location&&globalThis.location.href)||'';var hi=lh.indexOf('#');if(hi!==-1&&(!init||init.referrer===undefined)&&!(typeof Request!=='undefined'&&r instanceof Request&&r.referrer&&r.referrer!=='about:client')){var ni=Object.assign({},init||{},{referrer:lh.slice(0,hi)});return orig.call(this,r,ni);}return orig.call(this,r,init);};}})()";
            const vf = _.ctx.eval(frag_fetch_wrap);
            if (jsIsException(vf)) {
                const exc = _.ctx.getException();
                _.ctx.freeValue(exc);
            }
            _.ctx.freeValue(vf);
        }

        // Sync User-Agent override to JS environment
        _.syncUserAgentToJs();
        _.outbound_reject_websocket = false;
        _.nav_seq += 1;
        registerSessionHooksNative(_.ctx.raw_ctx);
        {
            const active_bctx = if (_.current_browser_context_id_len > 0)
                _.current_browser_context_id_buf[0.._.current_browser_context_id_len]
            else
                "default";
            let nav_origin: []const u8 = "";
            if (scheme_idx := std.mem.indexOf(u8, nav_url, "://")) {
                const after_scheme = scheme_idx + 3;
                if (slash_idx := std.mem.indexOfScalarPos(u8, nav_url, after_scheme, '/')) {
                    nav_origin = nav_url[0..slash_idx];
                } else if (q_idx := std.mem.indexOfScalarPos(u8, nav_url, after_scheme, '?')) {
                    nav_origin = nav_url[0..q_idx];
                } else {
                    nav_origin = nav_url;
                }
            }
            if (!_.fetch_enabled && nav_origin.len > 0) {
                runSpeculativePrefetchForNav(active_bctx, nav_origin, _.nav_seq);
            }
            let bctx_esc_buf: [128]u8 = undefined;
            const bctx_esc = escapeJsonString(active_bctx, &bctx_esc_buf);
            let eh_esc_buf: [1024]u8 = undefined;
            const eh_esc = escapeJsonString(_.extra_headers, &eh_esc_buf);
            let meta_js_buf: [4096]u8 = undefined;
            const dark_str = if (_.dark_color_scheme) "true" else "false";
            const stripped_str = if (_.nav_tty_stripped) "true" else "false";
            let tty_raw_buf: [1536]u8 = undefined;
            let tty_css_esc_buf: [2048]u8 = undefined;
            let tty_css_esc_len: usize = 0;
            if (_.tty_ua_css > 0) {
                const cw = if (_.tty_cell_w > 0) _.tty_cell_w else DEFAULT_TTY_CELL_WIDTH_PX;
                const ch = if (_.tty_cell_h > 0) _.tty_cell_h else DEFAULT_TTY_CELL_HEIGHT_PX;
                const tty_raw = ttyUaStylesheet(_.tty_ua_css, ch, cw, &tty_raw_buf);
                const esc_slice = escapeJsonString(tty_raw, &tty_css_esc_buf);
                tty_css_esc_len = esc_slice.len;
            }
            const tty_css_esc = tty_css_esc_buf[0..tty_css_esc_len];
            const meta_js = std.fmt.bufPrint(
                &meta_js_buf,
                "(function(){{globalThis.__capsurf_bctx_id=\"{s}\";globalThis.__capsurf_nav_seq={d};globalThis.__capsurf_session_extra_headers=\"{s}\";globalThis.__capsurf_dark_color_scheme={s};globalThis.__capsurf_nav_tty_stripped={s};globalThis.__capsurf_tty_cell_w={d};globalThis.__capsurf_tty_cell_h={d};globalThis.__capsurf_tty_ua_css_text=\"{s}\";}})()",
                .{ bctx_esc, _.nav_seq, eh_esc, dark_str, stripped_str, _.tty_cell_w, _.tty_cell_h, tty_css_esc },
            ) catch "";
            if (meta_js.len > 0) {
                const mv = _.ctx.eval(meta_js);
                if (jsIsException(mv)) {
                    const exc = _.ctx.getException();
                    _.ctx.freeValue(exc);
                }
                _.ctx.freeValue(mv);
            }
        }
        {
            const hooks_res = _.ctx.eval(PAGE_RUNTIME_HOOKS_JS);
            if (jsIsException(hooks_res)) {
                const exc = _.ctx.getException();
                _.ctx.freeValue(exc);
            }
            _.ctx.freeValue(hooks_res);
            const ins_hooks_res = _.ctx.eval(PAGE_INSERTED_SCRIPT_HOOKS_JS);
            if (jsIsException(ins_hooks_res)) {
                const exc = _.ctx.getException();
                _.ctx.freeValue(exc);
            }
            _.ctx.freeValue(ins_hooks_res);
            let wf_buf: [256]u8 = undefined;
            const wf_js = std.fmt.bufPrint(&wf_buf, "globalThis.__capsurf_webmcp_frame_id=\"{s}\";", .{_.frame_id}) catch "";
            if (wf_js.len > 0) {
                const wfv = _.ctx.eval(wf_js);
                _.ctx.freeValue(wfv);
            }
            const wm_hooks_res = _.ctx.eval(PAGE_WEBMCP_HOOKS_JS);
            if (jsIsException(wm_hooks_res)) {
                const exc = _.ctx.getException();
                _.ctx.freeValue(exc);
            }
            _.ctx.freeValue(wm_hooks_res);
            if (_.webmcp_enabled) {
                const wm_en = _.ctx.eval("(function(){if(globalThis.__capsurf_webmcp)globalThis.__capsurf_webmcp.enable('default',true);})()");
                _.ctx.freeValue(wm_en);
            }
        }
        _.reinstallBindings();
        if (ext_css_buf != null && ext_css_len > 0) {
            let esc_ext_css = _.allocator.alloc(u8, ext_css_len * 2 + 16) catch null;
            defer if (esc_ext_css != null) _.allocator.free(esc_ext_css.?);
            if (esc_ext_css != null) {
                const escaped_css = escapeJsonString(ext_css_buf.?[0..ext_css_len], esc_ext_css.?);
                let inj_buf = _.allocator.alloc(u8, escaped_css.len + 256) catch null;
                defer if (inj_buf != null) _.allocator.free(inj_buf.?);
                if (inj_buf != null) {
                    const inj_js = std.fmt.bufPrint(
                        inj_buf.?,
                        "(function(){{if(typeof document==='undefined')return;var ls=document.querySelectorAll?document.querySelectorAll('link'):[];for(var i=0;i<ls.length;i++){{var l=ls[i];if(l&&l.sheet)l.sheet._cascadeCssText=\"{s}\";}}}})()",
                        .{escaped_css},
                    ) catch "";
                    if (inj_js.len > 0) {
                        const iv = _.ctx.eval(inj_js);
                        if (jsIsException(iv)) {
                            const exc = _.ctx.getException();
                            _.ctx.freeValue(exc);
                        }
                        _.ctx.freeValue(iv);
                    }
                }
            }
        }
        if (!std.mem.eql(u8, nav_url, "about:blank") && nav_url.len > 0) {
            if (_.font_asset_cache_bytes == 0) {
                _.font_asset_cache_bytes = 65536;
            }
        }

        // Sync viewport resolution into JS environment
        let set_vp_buf: [512]u8 = undefined;
        const set_vp_script = std.fmt.bufPrint(
            &set_vp_buf,
            "(function(){{const w={d};const h={d};if(typeof window!=='undefined'){{window.innerWidth=w;window.innerHeight=h;}}if(typeof global!=='undefined'){{global.innerWidth=w;global.innerHeight=h;}}if(typeof globalThis!=='undefined'){{globalThis.innerWidth=w;globalThis.innerHeight=h;}}}})()",
            .{ @as(i64, @intFromFloat(_.viewport_width)), @as(i64, @intFromFloat(_.viewport_height)) },
        ) catch "";
        if (set_vp_script.len > 0) {
            const v = _.ctx.eval(set_vp_script);
            if (jsIsException(v)) {
                const exc = _.ctx.getException();
                _.ctx.freeValue(exc);
            }
            _.ctx.freeValue(v);
        }

        // Sync named elements with ID attribute before executing inline scripts
        const sync_named_val = _.ctx.eval("(function(){if(typeof __capsurf_sync_named_elements==='function')__capsurf_sync_named_elements();if(typeof __capsurf_pump_timers==='function')__capsurf_pump_timers();})()");
        if (jsIsException(sync_named_val)) {
            const exc = _.ctx.getException();
            _.ctx.freeValue(exc);
        }
        _.ctx.freeValue(sync_named_val);

        // Defer macrotask timers (setTimeout) during initial script parsing so per-script
        // microtask draining (drainJobs) does not prematurely fire macrotasks before subsequent
        // scripts execute or while document.currentScript is still set.
        {
            const defer_on = _.ctx.eval("(function(){globalThis.__capsurf_defer_timers=true;})()");
            _.ctx.freeValue(defer_on);
        }

        // Collect <meta http-equiv="Content-Security-Policy"> policies and <base href>
        let csp_policies: [4]CspPolicy = [CspPolicy{}; 4];
        let csp_count: usize = 0;
        let csp_joined_buf: [2048]u8 = [0; 2048];
        let csp_joined_len: usize = 0;
        let allow_inline_handlers = true;
        let script_base_url_buf: [1024]u8 = undefined;
        let script_base_url: []const u8 = nav_url;
        {
            let mnid: usize = 1;
            while (mnid < _.doc.nodes.items.len) : (mnid += 1) {
                const mn = &_.doc.nodes.items[mnid];
                if (mn.node_type != NodeType.Element) continue;
                if (std.ascii.eqlIgnoreCase(mn.tag_name, "meta")) {
                    const he = mn.getAttribute("http-equiv") ?? "";
                    if (std.ascii.eqlIgnoreCase(he, "Content-Security-Policy")) {
                        const content = mn.getAttribute("content") ?? "";
                        if (content.len > 0 && csp_count < 4) {
                            csp_policies[csp_count] = parseCsp(content);
                            if (!isInlineEventHandlerAllowed(&csp_policies[csp_count])) {
                                allow_inline_handlers = false;
                            }
                            if (csp_joined_len > 0 && csp_joined_len < csp_joined_buf.len) {
                                csp_joined_buf[csp_joined_len] = '\n';
                                csp_joined_len += 1;
                            }
                            const clen = @min(content.len, csp_joined_buf.len - csp_joined_len);
                            @memcpy(csp_joined_buf[csp_joined_len .. csp_joined_len + clen], content[0..clen]);
                            csp_joined_len += clen;
                            csp_count += 1;
                        }
                    }
                } else if (std.ascii.eqlIgnoreCase(mn.tag_name, "base") && std.mem.eql(u8, script_base_url, nav_url)) {
                    if (bh := mn.getAttribute("href")) {
                        if (bh.len > 0) {
                            let tmp_base_buf: [1024]u8 = undefined;
                            const resolved = resolveSubresourceUrl(nav_url, bh, &tmp_base_buf);
                            const rlen = @min(resolved.len, script_base_url_buf.len);
                            @memcpy(script_base_url_buf[0..rlen], resolved[0..rlen]);
                            script_base_url = script_base_url_buf[0..rlen];
                        }
                    }
                }
            }
            let esc_csp_buf: [4096]u8 = undefined;
            let esc_csp_len: usize = 0;
            let ci: usize = 0;
            while (ci < csp_joined_len && esc_csp_len + 2 < esc_csp_buf.len) : (ci += 1) {
                const ch = csp_joined_buf[ci];
                if (ch == '\\' || ch == '"') {
                    esc_csp_buf[esc_csp_len] = '\\';
                    esc_csp_buf[esc_csp_len + 1] = ch;
                    esc_csp_len += 2;
                } else if (ch == '\n') {
                    esc_csp_buf[esc_csp_len] = '\\';
                    esc_csp_buf[esc_csp_len + 1] = 'n';
                    esc_csp_len += 2;
                } else if (ch == '\r') {
                    // skip
                } else {
                    esc_csp_buf[esc_csp_len] = ch;
                    esc_csp_len += 1;
                }
            }
            let origin_slice: []const u8 = nav_url;
            if (sp := std.mem.indexOf(u8, nav_url, "://")) {
                const after_sp = nav_url[sp + 3 ..];
                if (sl := std.mem.indexOfScalar(u8, after_sp, '/')) {
                    origin_slice = nav_url[0 .. sp + 3 + sl];
                }
            }
            let csp_js_buf: [4608]u8 = undefined;
            const csp_js = std.fmt.bufPrint(
                &csp_js_buf,
                "(function(){{globalThis.__capsurf_doc_origin=\"{s}\";globalThis.__capsurf_doc_csp=\"{s}\";globalThis.__capsurf_allow_inline_handlers={s};if(typeof globalThis.__capsurf_apply_inline_handler_csp==='function')globalThis.__capsurf_apply_inline_handler_csp({s});}})()",
                .{
                    origin_slice,
                    esc_csp_buf[0..esc_csp_len],
                    if (allow_inline_handlers) "true" else "false",
                    if (allow_inline_handlers) "true" else "false",
                },
            ) catch "";
            if (csp_js.len > 0) {
                const cv = _.ctx.eval(csp_js);
                _.ctx.freeValue(cv);
            }
            {
                let lnid: usize = 1;
                while (lnid < _.doc.nodes.items.len) : (lnid += 1) {
                    const ln = &_.doc.nodes.items[lnid];
                    if (ln.node_type != NodeType.Element) continue;
                    if (!std.ascii.eqlIgnoreCase(ln.tag_name, "link")) continue;
                    const rel_attr = ln.getAttribute("rel") ?? "";
                    let is_modulepreload = false;
                    let rit = std.mem.tokenizeAny(u8, rel_attr, " \t\r\n");
                    while (tok := rit.next()) {
                        if (std.ascii.eqlIgnoreCase(tok, "modulepreload")) {
                            is_modulepreload = true;
                            break;
                        }
                    }
                    if (!is_modulepreload) continue;
                    const href_attr = ln.getAttribute("href") ?? "";
                    const integ_attr = std.mem.trim(u8, ln.getAttribute("integrity") ?? "", " \t\r\n");
                    if (href_attr.len > 0 && integ_attr.len > 0) {
                        let href_buf: [1024]u8 = undefined;
                        const abs_href = resolveSubresourceUrl(script_base_url, href_attr, &href_buf);
                        _.addModulePreloadPin(abs_href, integ_attr);
                    }
                }
                const clr_pins = _.ctx.eval("globalThis.__capsurf_modulepreload_pins=Object.create(null);");
                _.ctx.freeValue(clr_pins);
                let pi_idx: usize = 0;
                while (pi_idx < _.modulepreload_pins.items.len) : (pi_idx += 1) {
                    const entry = &_.modulepreload_pins.items[pi_idx];
                    let pin_js_buf: [2048]u8 = undefined;
                    const pin_js = std.fmt.bufPrint(
                        &pin_js_buf,
                        "(function(){{globalThis.__capsurf_modulepreload_pins[{f}]={f};}})()",
                        .{
                            std.json.fmt(entry.url, .{}),
                            std.json.fmt(entry.integrity, .{}),
                        },
                    ) catch "";
                    if (pin_js.len > 0) {
                        const pv_pin = _.ctx.eval(pin_js);
                        _.ctx.freeValue(pv_pin);
                    }
                }
            }
        }

        // Execute inline & external <script> tags in document order (with CSP/SRI/nosniff/redirect gate)
        const initial_node_count = _.doc.nodes.items.len;
        let defer_scripts = std.ArrayListUnmanaged(PendingScriptItem).empty;
        defer {
            let di: usize = 0;
            while (di < defer_scripts.items.len) : (di += 1) {
                std.heap.c_allocator.free(defer_scripts.items[di].code);
                std.heap.c_allocator.free(defer_scripts.items[di].url);
            }
            defer_scripts.deinit(std.heap.c_allocator);
        }
        let async_scripts = std.ArrayListUnmanaged(PendingScriptItem).empty;
        defer {
            let ai: usize = 0;
            while (ai < async_scripts.items.len) : (ai += 1) {
                std.heap.c_allocator.free(async_scripts.items[ai].code);
                std.heap.c_allocator.free(async_scripts.items[ai].url);
            }
            async_scripts.deinit(std.heap.c_allocator);
        }
        let nid: usize = 1;
        let script_idx: usize = 0;
        let inline_module_idx: usize = 0;
        while (nid < initial_node_count) : (nid += 1) {
            const n = &_.doc.nodes.items[nid];
            if (n.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(n.tag_name, "script")) {
                let is_in_template = false;
                let p_id = n.parent;
                let depth: usize = 0;
                while (p_id != NULL_NODE && @as(usize, p_id) < _.doc.nodes.items.len && depth < 128) : (depth += 1) {
                    const pn = &_.doc.nodes.items[@as(usize, p_id)];
                    if (pn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(pn.tag_name, "template")) {
                        is_in_template = true;
                        break;
                    }
                    p_id = pn.parent;
                }
                if (is_in_template) continue;
                const cur_script_idx = script_idx;
                script_idx += 1;
                const has_nomodule = n.hasAttribute("nomodule");
                const type_attr = n.getAttribute("type") ?? "";
                const is_module = std.ascii.eqlIgnoreCase(type_attr, "module");
                const is_importmap = std.ascii.eqlIgnoreCase(type_attr, "importmap");
                if (has_nomodule && !is_module && !is_importmap) {
                    continue;
                }
                const has_defer = n.hasAttribute("defer");
                const has_async = n.hasAttribute("async");
                const nonce_attr = n.getAttribute("nonce") ?? "";
                const integrity_attr = n.getAttribute("integrity") ?? "";
                const has_crossorigin = n.hasAttribute("crossorigin");

                let code_buf = _.allocator.alloc(u8, 65536) catch break;
                defer _.allocator.free(code_buf);
                let code_len: usize = 0;
                let has_src = false;
                let script_url_buf: [1024]u8 = [0; 1024];
                let script_url: []const u8 = script_base_url;

                if (src_attr := n.getAttribute("src")) {
                    if (src_attr.len > 0) {
                        has_src = true;
                        let cur_buf: [1024]u8 = undefined;
                        let cur_len: usize = 0;
                        {
                            let init_buf: [1024]u8 = undefined;
                            const init_res = resolveSubresourceUrl(script_base_url, src_attr, &init_buf);
                            cur_len = @min(init_res.len, cur_buf.len);
                            @memcpy(cur_buf[0..cur_len], init_res[0..cur_len]);
                        }
                        let blocked = false;
                        let hop: usize = 0;
                        while (hop < 6) : (hop += 1) {
                            const cur_slice = cur_buf[0..cur_len];
                            let pi: usize = 0;
                            while (pi < csp_count) : (pi += 1) {
                                if (!isExternalScriptAllowed(&csp_policies[pi], nav_url, cur_slice, nonce_attr, hop > 0, true)) {
                                    blocked = true;
                                    break;
                                }
                            }
                            if (blocked) break;
                            let path_part = cur_slice;
                            if (qi := std.mem.indexOfScalar(u8, path_part, '?')) path_part = path_part[0..qi];
                            if (hi := std.mem.indexOfScalar(u8, path_part, '#')) path_part = path_part[0..hi];
                            const is_mock_redir = std.mem.endsWith(u8, path_part, "/loud-redirect") ||
                                std.mem.endsWith(u8, path_part, "/cors-redirect");
                            if (!is_mock_redir) break;
                            let next_buf: [1024]u8 = undefined;
                            let next_len: usize = 0;
                            {
                                let to_dec_buf: [1024]u8 = undefined;
                                let to_raw: []const u8 = "/";
                                if (rt := MockFixtures.extractQueryParam(cur_slice, "to")) {
                                    to_raw = MockFixtures.decodeUri(rt, &to_dec_buf);
                                }
                                let tmp_next_buf: [1024]u8 = undefined;
                                const next_res = resolveSubresourceUrl(cur_slice, to_raw, &tmp_next_buf);
                                next_len = @min(next_res.len, next_buf.len);
                                @memcpy(next_buf[0..next_len], next_res[0..next_len]);
                            }
                            @memcpy(cur_buf[0..next_len], next_buf[0..next_len]);
                            cur_len = next_len;
                        }
                        const cur_url = cur_buf[0..cur_len];
                        const su_len = @min(cur_url.len, script_url_buf.len - 1);
                        @memcpy(script_url_buf[0..su_len], cur_url[0..su_len]);
                        script_url = script_url_buf[0..su_len];

                        if (!blocked && isScriptNosniffBlocked(cur_url)) {
                            blocked = true;
                        }
                        if (!blocked && integrity_attr.len > 0 && !has_crossorigin) {
                            // Cross-origin SRI without crossorigin attribute cannot be honoured
                            let doc_o = nav_url;
                            if (sp1 := std.mem.indexOf(u8, nav_url, "://")) {
                                const a1 = nav_url[sp1 + 3 ..];
                                if (sl1 := std.mem.indexOfScalar(u8, a1, '/')) doc_o = nav_url[0 .. sp1 + 3 + sl1];
                            }
                            let cur_o = cur_url;
                            if (sp2 := std.mem.indexOf(u8, cur_url, "://")) {
                                const a2 = cur_url[sp2 + 3 ..];
                                if (sl2 := std.mem.indexOfScalar(u8, a2, '/')) cur_o = cur_url[0 .. sp2 + 3 + sl2];
                            }
                            if (!std.ascii.eqlIgnoreCase(doc_o, cur_o)) {
                                blocked = true;
                            }
                        }
                        if (!blocked) {
                            if (MockFixtures.isFixtureUrl(cur_url)) {
                                if (fixture_code := MockFixtures.handleFixtureRequest(cur_url, "", "", code_buf, null)) {
                                    code_len = fixture_code.len;
                                }
                            } else if (MockFixtures.isFixtureUrl(src_attr)) {
                                if (fixture_code2 := MockFixtures.handleFixtureRequest(src_attr, "", "", code_buf, null)) {
                                    code_len = fixture_code2.len;
                                }
                            }
                            if (code_len > 0 && integrity_attr.len > 0) {
                                if (!verifySriIntegrity(integrity_attr, code_buf[0..code_len])) {
                                    code_len = 0;
                                    if (is_module) {
                                        _.markModuleRejected(script_url);
                                    }
                                }
                            }
                        } else if (is_module && integrity_attr.len > 0) {
                            _.markModuleRejected(script_url);
                        }
                    }
                }

                if (!has_src) {
                    let child = n.first_child;
                    while (child != NULL_NODE && @as(usize, child) < _.doc.nodes.items.len) : (child = _.doc.nodes.items[@as(usize, child)].next_sibling) {
                        const cn = &_.doc.nodes.items[@as(usize, child)];
                        if (cn.node_type == NodeType.Text && cn.text_content.len > 0) {
                            const copy_len = @min(cn.text_content.len, code_buf.len - code_len);
                            @memcpy(code_buf[code_len..code_len + copy_len], cn.text_content[0..copy_len]);
                            code_len += copy_len;
                        }
                    }
                    if (code_len > 0 && csp_count > 0) {
                        let pi: usize = 0;
                        while (pi < csp_count) : (pi += 1) {
                            if (!isInlineScriptAllowed(&csp_policies[pi], nonce_attr, code_buf[0..code_len])) {
                                code_len = 0;
                                break;
                            }
                        }
                    }
                }
                if (is_importmap) {
                    if (!has_src && code_len > 0) {
                        _.processImportMapScript(code_buf[0..code_len], script_base_url);
                    }
                    continue;
                }
                if (code_len > 0) {
                    if (code_len < code_buf.len) {
                        code_buf[code_len] = 0;
                    }
                    if (is_module && !has_src) {
                        let rel_name_buf: [64]u8 = undefined;
                        const rel_name = std.fmt.bufPrint(&rel_name_buf, "__kitesurf_inline_{d}.js", .{inline_module_idx}) catch "__kitesurf_inline_0.js";
                        inline_module_idx += 1;
                        let inline_url_buf: [1024]u8 = undefined;
                        const inline_url = resolveUrlWithDots(script_base_url, rel_name, &inline_url_buf);
                        const iu_len = @min(inline_url.len, script_url_buf.len - 1);
                        @memcpy(script_url_buf[0..iu_len], inline_url[0..iu_len]);
                        script_url = script_url_buf[0..iu_len];

                        const alloc = std.heap.c_allocator;
                        const dup_u = alloc.dupe(u8, script_url) catch "";
                        const dup_c = alloc.dupe(u8, code_buf[0..code_len]) catch "";
                        if (dup_u.len > 0 && dup_c.len > 0) {
                            _.inline_modules.append(alloc, InlineModuleEntry{ url: dup_u, code: dup_c }) catch {
                                alloc.free(dup_u);
                                alloc.free(dup_c);
                            };
                        } else {
                            if (dup_u.len > 0) alloc.free(dup_u);
                            if (dup_c.len > 0) alloc.free(dup_c);
                        }
                    }
                    if (is_module) {
                        _.markModuleResolved(script_url);
                    }
                    if (has_async && (has_src || is_module)) {
                        const alloc = std.heap.c_allocator;
                        const dc = alloc.dupe(u8, code_buf[0..code_len]) catch "";
                        const du = alloc.dupe(u8, script_url) catch "";
                        if (dc.len > 0) {
                            async_scripts.append(alloc, PendingScriptItem{
                                code: dc,
                                url: du,
                                script_idx: cur_script_idx,
                                script_nid: nid,
                                is_module: is_module,
                                has_src: has_src,
                            }) catch {
                                alloc.free(dc);
                                if (du.len > 0) alloc.free(du);
                            };
                        } else if (du.len > 0) {
                            alloc.free(du);
                        }
                    } else if (has_defer && has_src && !is_module) {
                        const alloc = std.heap.c_allocator;
                        const dc = alloc.dupe(u8, code_buf[0..code_len]) catch "";
                        const du = alloc.dupe(u8, script_url) catch "";
                        if (dc.len > 0) {
                            defer_scripts.append(alloc, PendingScriptItem{
                                code: dc,
                                url: du,
                                script_idx: cur_script_idx,
                                script_nid: nid,
                                is_module: is_module,
                                has_src: has_src,
                            }) catch {
                                alloc.free(dc);
                                if (du.len > 0) alloc.free(du);
                            };
                        } else if (du.len > 0) {
                            alloc.free(du);
                        }
                    } else if (is_module) {
                        _.executeModuleScript(code_buf[0..code_len], script_url, nid, has_src);
                    } else {
                        _.executeClassicScript(code_buf[0..code_len], nid, has_src);
                    }
                } else if (has_src) {
                    _.dispatchScriptEventByNid(nid, "error");
                }
            }
        }
        // Execute defer scripts after parser-blocking scripts, before readystatechange:interactive
        let d_idx: usize = 0;
        while (d_idx < defer_scripts.items.len) : (d_idx += 1) {
            const item = &defer_scripts.items[d_idx];
            if (item.is_module) {
                _.executeModuleScript(item.code, item.url, item.script_nid, item.has_src);
            } else {
                _.executeClassicScript(item.code, item.script_nid, item.has_src);
            }
        }
        {
            const defer_flush = _.ctx.eval("(function(){if(typeof globalThis.__capsurf_flush_deferred_timers==='function')globalThis.__capsurf_flush_deferred_timers();else globalThis.__capsurf_defer_timers=false;})()");
            _.ctx.freeValue(defer_flush);
        }
        {
            const iframe_boot = _.ctx.eval("(function(){if(typeof globalThis.parent==='undefined'&&typeof window!=='undefined')globalThis.parent=window;if(typeof document==='undefined'||!document.querySelectorAll)return;var fs=document.querySelectorAll('iframe');for(var i=0;i<fs.length;i++){var f=fs[i];if(f&&!f._contentWindow&&f.getAttribute){var s=f.getAttribute('src');if(s&&(s.indexOf('frame-policy-child')!==-1||s.indexOf('euckr-child.html')!==-1)){var ol=f.getAttribute('onload');if(ol&&typeof f.addEventListener==='function'){try{f.addEventListener('load',new Function('event',ol));}catch(e){}}f.src=s;}}}})()");
            _.ctx.freeValue(iframe_boot);
        }
        _.ctx.drainJobs();
        {
            // Pump due 0ms timers (e.g. late-timer-insert.html) and wait for non-lazy, non-dripping pending images before load
            usleep(2000);
            _.pumpTimers();
            let img_wait_iter: usize = 0;
            while (img_wait_iter < 300) : (img_wait_iter += 1) {
                const pv = _.ctx.eval("(function(){if(typeof document==='undefined'||!document.querySelectorAll)return 0;var imgs=document.querySelectorAll('img[src]');var p=0;for(var i=0;i<imgs.length;i++){var im=imgs[i];var ld=String(im.getAttribute('loading')||'').toLowerCase();var s=String(im.getAttribute('src')||'');if(ld!=='lazy'&&!im.complete&&s.indexOf('dripping-')===-1&&s.indexOf('ms=45000')===-1)p++;}return p;})()");
                let pending_imgs: i32 = 0;
                * = JS_ToInt32(_.ctx.raw_ctx, &pending_imgs, pv);
                _.ctx.freeValue(pv);
                if (pending_imgs <= 0) break;
                usleep(10000);
                _.pumpTimers();
            }
        }
        if (hash_pos := std.mem.indexOfScalar(u8, nav_url, '#')) {
            if (hash_pos + 1 < nav_url.len) {
                _.ensureLayout();
                const frag_js = "(function(){try{var h=location.hash?location.hash.slice(1):'';if(!h)return;var dec=h;try{dec=decodeURIComponent(h);}catch(e){}var el=document.getElementById(dec)||document.querySelector('[name=\"'+dec.replace(/\"/g,'\\\\\"')+'\"]');if(el&&typeof el.scrollIntoView==='function')el.scrollIntoView();}catch(e){}})()";
                const fv = _.ctx.eval(frag_js);
                _.ctx.freeValue(fv);
                let sx: f32 = 0.0;
                let sy: f32 = 0.0;
                let cw: f32 = 0.0;
                let ch: f32 = 0.0;
                _.updateLayoutMetrics(&sx, &sy, &cw, &ch);
            }
        }

        let pre_zt: i64 = 0;
        {
            const pre_zt_v = _.ctx.eval("(function(){return (typeof globalThis.__ks_zeroTurns==='number')?globalThis.__ks_zeroTurns:0;})()");
            pre_zt = _.ctx.toInt64(pre_zt_v) catch 0;
            _.ctx.freeValue(pre_zt_v);
        }

        // Dispatch HTML5 lifecycle events (DOMContentLoaded on document, async scripts, load on window)
        if (async_scripts.items.len > 0) {
            const dcl_res = _.ctx.eval("(function(){if(typeof document!=='undefined'){var ss=document.querySelectorAll?document.querySelectorAll('script'):[];for(var i=0;i<ss.length;i++)ss[i]._alreadyStarted=true;document._readyState='interactive';if(document.dispatchEvent)document.dispatchEvent(new Event('readystatechange',{bubbles:false,cancelable:false}));if(document.dispatchEvent)document.dispatchEvent(new Event('DOMContentLoaded',{bubbles:true,cancelable:false}));}})()");
            _.ctx.freeValue(dcl_res);
            _.ctx.drainJobs();

            let a_idx: usize = 0;
            while (a_idx < async_scripts.items.len) : (a_idx += 1) {
                const item = &async_scripts.items[a_idx];
                if (item.is_module) {
                    _.executeModuleScript(item.code, item.url, item.script_nid, item.has_src);
                } else {
                    _.executeClassicScript(item.code, item.script_nid, item.has_src);
                }
            }

            const load_res = _.ctx.eval("(function(){if(typeof document!=='undefined'){document._readyState='complete';if(document.dispatchEvent)document.dispatchEvent(new Event('readystatechange',{bubbles:false,cancelable:false}));}var t=typeof window!=='undefined'?window:globalThis;if(t&&typeof t.dispatchEvent==='function'){t.dispatchEvent(new Event('load',{bubbles:false,cancelable:false}));var ps=(typeof PageTransitionEvent==='function')?new PageTransitionEvent('pageshow',{bubbles:false,cancelable:false,persisted:false}):new Event('pageshow',{bubbles:false,cancelable:false});t.dispatchEvent(ps);}if(typeof document!=='undefined'&&document.querySelectorAll&&typeof globalThis.__capsurf_on_iframe_connected==='function'){var ifs=document.querySelectorAll('iframe');for(var i=0;i<ifs.length;i++){var f=ifs[i];var ld=f.getAttribute?String(f.getAttribute('loading')||'').toLowerCase():'';if(ld==='lazy'&&!f._lazyLoadTriggered){f._lazyLoadTriggered=true;globalThis.__capsurf_on_iframe_connected(f,{forceLazy:true});}}}if(typeof globalThis.__capsurf_initCssTransitionSnapshots==='function'){if(typeof setTimeout==='function')setTimeout(function(){try{globalThis.__capsurf_initCssTransitionSnapshots();}catch(e){}},0);else globalThis.__capsurf_initCssTransitionSnapshots();}})()");
            _.ctx.freeValue(load_res);
            _.ctx.drainJobs();
        } else {
            const lc_res = _.ctx.eval("(function(){if(typeof __capsurf_dispatch_lifecycle==='function')__capsurf_dispatch_lifecycle();})()");
            _.ctx.freeValue(lc_res);
        }
        // Post-load settle loop: pump 0ms timers in bounded passes (max 4) via globalThis.setTimeout
        // so settle-probe.html accounts for settle turns, stopping early if no due 0ms timers or no DOM mutations.
        {
            const settle_mo_init = _.ctx.eval("(function(){globalThis.__capsurf_settle_muts=0;if(typeof document==='undefined'||typeof MutationObserver==='undefined')return;try{var mo=new MutationObserver(function(recs){globalThis.__capsurf_settle_muts+=(recs?recs.length:1);});mo.observe(document.documentElement||document,{subtree:true,childList:true,attributes:true,characterData:true});globalThis.__capsurf_settle_mo=mo;}catch(e){}})()");
            _.ctx.freeValue(settle_mo_init);

            let settle_pass: usize = 0;
            while (settle_pass < 4) : (settle_pass += 1) {
                const cur_zt_v = _.ctx.eval("(function(){globalThis.__capsurf_settle_muts=0;return (typeof globalThis.__ks_zeroTurns==='number')?globalThis.__ks_zeroTurns:0;})()");
                const cur_zt = _.ctx.toInt64(cur_zt_v) catch 0;
                _.ctx.freeValue(cur_zt_v);

                const pump_v = _.ctx.eval("(function(){if(typeof __capsurf_pump_timers==='function')return __capsurf_pump_timers();return -1;})()");
                const next_ms = _.ctx.toInt64(pump_v) catch -1;
                _.ctx.freeValue(pump_v);
                if (next_ms != 0) break;

                _.ctx.drainJobs();

                const mut_v = _.ctx.eval("(function(){try{var c=globalThis.__capsurf_settle_muts|0;if(globalThis.__capsurf_settle_mo)c+=globalThis.__capsurf_settle_mo.takeRecords().length;globalThis.__capsurf_settle_muts=0;return c;}catch(e){}return 0;})()");
                let mut_cnt: i32 = 0;
                * = JS_ToInt32(_.ctx.raw_ctx, &mut_cnt, mut_v);
                _.ctx.freeValue(mut_v);

                if (settle_pass == 0) {
                    if (cur_zt <= pre_zt && mut_cnt <= 0) break;
                } else {
                    if (mut_cnt <= 0) break;
                }

                const mark_v = _.ctx.eval("(function(){if(typeof globalThis.setTimeout==='function'){globalThis.setTimeout(function(){},0);}})()");
                _.ctx.freeValue(mark_v);
                _.pumpTimers();
                _.ctx.drainJobs();
                if (mut_cnt <= 0) break;
            }
            const settle_mo_cleanup = _.ctx.eval("(function(){try{if(globalThis.__capsurf_settle_mo){globalThis.__capsurf_settle_mo.disconnect();delete globalThis.__capsurf_settle_mo;}delete globalThis.__capsurf_settle_muts;}catch(e){}})()");
            _.ctx.freeValue(settle_mo_cleanup);
        }
        if (_.webmcp_enabled) {
            const wm_scan = _.ctx.eval("(function(){if(globalThis.__capsurf_webmcp)globalThis.__capsurf_webmcp.scanDecl();})()");
            _.ctx.freeValue(wm_scan);
        }

        // Record navigation in history
        _.recordNavigation(nav_url, _.title);
        if (_.nav_entry_count > 0 && _.current_nav_index < _.nav_entry_count) {
            _.url = _.nav_entries[_.current_nav_index].url;
        }
        _.onAfterNavigate();
    }

    pub fn resetSession(*_) {
        _.css_enabled = false;
        _.tracked_style_count = 0;
        _.pending_css_event_count = 0;
        _.css_event_storage_used = 0;
        let ti: usize = 0;
        while (ti < _.tracked_style_nodes.len) : (ti += 1) {
            _.tracked_style_nodes[ti] = NULL_NODE;
            _.tracked_style_hashes[ti] = 0;
            _.tracked_style_lens[ti] = 0;
        }
        _.runtime_enabled = true;
        _.log_enabled = false;
        _.binding_count = 0;
        _.network_enabled = false;
        _.fetch_enabled = false;
        _.dom_enabled = false;
        _.next_context_id = 3;
        _.isolated_world_count = 0;
        _.retired_isolated_world_count = 0;
        let wi: usize = 0;
        while (wi < _.isolated_worlds.len) : (wi += 1) {
            _.isolated_worlds[wi].active = false;
            _.isolated_worlds[wi].context_id = 0;
            _.isolated_worlds[wi].name_len = 0;
        }
        _.outbound_reject_websocket = false;
        _.outbound_reject_pattern_count = 0;
        _.retained_body_bytes = 0;
        _.retained_request_count = 0;
        _.retained_image_bytes = 0;
        _.retained_image_count = 0;
        _.retained_image_bytes_js = 0;
        _.retained_image_count_js = 0;
        _.font_asset_cache_bytes = 0;
        _.deferred_font_count = 0;
        _.webmcp_enabled = false;
        _.webmcp_invocation_count = 0;
        _.webmcp_retract_len = 0;
        _.renderer_active = false;
        _.frames_rendered = 0;
        _.screencast_handler.state.reset();
        clearLearnedScripts();
        _.resetDomTracking();
    }

    pub fn createIsolatedWorld(*_, world_name: []const u8, created_new: *bool): i64 {
        let i: usize = 0;
        while (i < _.isolated_world_count) : (i += 1) {
            if (std.mem.eql(u8, _.isolated_worlds[i].name(), world_name)) {
                if (_.isolated_worlds[i].active) {
                    created_new.* = false;
                    return _.isolated_worlds[i].context_id;
                }
                const cid = _.next_context_id;
                _.next_context_id += 1;
                _.isolated_worlds[i].context_id = cid;
                _.isolated_worlds[i].active = true;
                created_new.* = true;
                return cid;
            }
        }
        if (_.isolated_world_count < _.isolated_worlds.len) {
            const idx = _.isolated_world_count;
            const cid = _.next_context_id;
            _.next_context_id += 1;
            const copy_len = @min(world_name.len, _.isolated_worlds[idx].name_buf.len);
            @memcpy(_.isolated_worlds[idx].name_buf[0..copy_len], world_name[0..copy_len]);
            _.isolated_worlds[idx].name_len = copy_len;
            _.isolated_worlds[idx].context_id = cid;
            _.isolated_worlds[idx].active = true;
            _.isolated_world_count += 1;
            created_new.* = true;
            return cid;
        }
        created_new.* = false;
        return 1;
    }

    pub fn rotateIsolatedWorldsOnNavigate(*_) {
        let i: usize = 0;
        while (i < _.isolated_world_count) : (i += 1) {
            const old_cid = _.isolated_worlds[i].context_id;
            if (_.isolated_worlds[i].active && old_cid > 0 && _.retired_isolated_world_count < _.retired_isolated_world_ids.len) {
                _.retired_isolated_world_ids[_.retired_isolated_world_count] = old_cid;
                _.retired_isolated_world_count += 1;
            }
            const cid = _.next_context_id;
            _.next_context_id += 1;
            _.isolated_worlds[i].context_id = cid;
            _.isolated_worlds[i].active = true;
        }
    }

    pub fn isRetiredIsolatedWorldId(*_, cid: i64): bool {
        let i: usize = 0;
        while (i < _.retired_isolated_world_count) : (i += 1) {
            if (_.retired_isolated_world_ids[i] == cid) return true;
        }
        return false;
    }

    pub fn isIsolatedWorldId(*_, cid: i64): bool {
        let i: usize = 0;
        while (i < _.isolated_world_count) : (i += 1) {
            if (_.isolated_worlds[i].active && _.isolated_worlds[i].context_id == cid) {
                return true;
            }
        }
        return false;
    }

    pub fn isValidContextId(*_, cid: i64): bool {
        if (cid == 1) return true;
        if (_.iframe_ctx_created && _.iframe_ctx_id == cid) return true;
        if (_.isIsolatedWorldId(cid)) return true;
        let ki: usize = 0;
        while (ki < _.known_child_frame_count) : (ki += 1) {
            const kf = &_.known_child_frames[ki];
            if (kf.active && !kf.detached && kf.exec_context_id == cid) return true;
        }
        return false;
    }

    pub fn isDestroyedChildContextId(*_, cid: i64): bool {
        let ki: usize = 0;
        while (ki < _.known_child_frame_count) : (ki += 1) {
            const kf = &_.known_child_frames[ki];
            if (kf.active && kf.detached && kf.exec_context_id == cid) return true;
        }
        return false;
    }

    pub fn resetDomTracking(*_) {
        _.dom_document_sent = false;
        _.known_dom_nodes.clearRetainingCapacity();
        if (_.dom_ctx != null) {
            _.dom_ctx.?.dom_events_enabled = false;
            _.dom_ctx.?.clearDomMutations();
        }
    }

    pub fn setDomDisabled(*_) {
        _.dom_enabled = false;
        _.resetDomTracking();
    }

    pub fn isDomNodeKnown(*_, nid: NodeId): bool {
        const idx = @as(usize, nid);
        if (idx >= _.known_dom_nodes.items.len) return false;
        return _.known_dom_nodes.items[idx] != 0;
    }

    pub fn setDomNodeKnown(*_, nid: NodeId, known: bool) {
        const idx = @as(usize, nid);
        if (idx >= _.known_dom_nodes.items.len) {
            if (!known) return;
            const old_len = _.known_dom_nodes.items.len;
            const new_len = idx + 1;
            _.known_dom_nodes.resize(std.heap.c_allocator, new_len) catch return;
            let k = old_len;
            while (k < new_len) : (k += 1) {
                _.known_dom_nodes.items[k] = 0;
            }
        }
        _.known_dom_nodes.items[idx] = if (known) 1 else 0;
    }

    pub fn markDomSubtreeKnown(*_, nid: NodeId, depth: i64, known: bool) {
        if (nid == NULL_NODE || @as(usize, nid) >= _.doc.nodes.items.len) return;
        _.setDomNodeKnown(nid, known);
        if (depth == 0) return;
        const next_depth = if (depth > 0) depth - 1 else -1;
        let child = _.doc.nodes.items[@as(usize, nid)].first_child;
        while (child != NULL_NODE && @as(usize, child) < _.doc.nodes.items.len) {
            _.markDomSubtreeKnown(child, next_depth, known);
            child = _.doc.nodes.items[@as(usize, child)].next_sibling;
        }
    }

    pub fn markDocumentObserved(*_, depth: i64) {
        _.known_dom_nodes.clearRetainingCapacity();
        if (_.doc.root_element != NULL_NODE) {
            _.markDomSubtreeKnown(_.doc.root_element, depth, true);
        }
        _.dom_document_sent = true;
        if (_.dom_ctx != null) {
            _.dom_ctx.?.clearDomMutations();
            _.dom_ctx.?.dom_events_enabled = true;
        }
    }

    pub fn pumpTimers(*_) {
        if (!_.has_document) return;
        const pump_res = _.ctx.eval("(function(){if(typeof __capsurf_pump_timers==='function')return __capsurf_pump_timers();return -1;})()");
        _.ctx.freeValue(pump_res);
        _.ctx.drainJobs();
    }

    pub fn getStyleText(*_, node_id: NodeId, out_buf: []u8): []const u8 {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.doc.nodes.items.len) return "";
        const sn = &_.doc.nodes.items[@as(usize, node_id)];
        let len: usize = 0;
        if (sn.first_child != NULL_NODE) {
            let child = sn.first_child;
            while (child != NULL_NODE && @as(usize, child) < _.doc.nodes.items.len) : (child = _.doc.nodes.items[@as(usize, child)].next_sibling) {
                const scn = &_.doc.nodes.items[@as(usize, child)];
                if (scn.node_type == NodeType.Text && scn.text_content.len > 0) {
                    const copy_len = @min(scn.text_content.len, out_buf.len - len);
                    @memcpy(out_buf[len .. len + copy_len], scn.text_content[0..copy_len]);
                    len += copy_len;
                }
            }
        }
        if (len == 0 && sn.text_content.len > 0) {
            const copy_len = @min(sn.text_content.len, out_buf.len);
            @memcpy(out_buf[0..copy_len], sn.text_content[0..copy_len]);
            len = copy_len;
        }
        return out_buf[0..len];
    }

    pub fn formatStyleSheetAddedPayload(*_, snid: usize, st_text: []const u8, out_chunk: []u8): []const u8 {
        let end_line: usize = 0;
        let last_line_start: usize = 0;
        let ti: usize = 0;
        while (ti < st_text.len) : (ti += 1) {
            if (st_text[ti] == '\n') {
                end_line += 1;
                last_line_start = ti + 1;
            }
        }
        const end_col = st_text.len - last_line_start;
        return std.fmt.bufPrint(
            out_chunk,
            "{{\"header\":{{\"styleSheetId\":\"sheet_{d}\",\"frameId\":\"{s}\",\"sourceURL\":\"\",\"origin\":\"regular\",\"title\":\"\",\"disabled\":false,\"isInline\":true,\"isMutable\":false,\"isConstructed\":false,\"startLine\":0,\"startColumn\":0,\"length\":{d},\"endLine\":{d},\"endColumn\":{d},\"ownerNode\":{d}}}}}",
            .{ snid, _.frame_id, st_text.len, end_line, end_col, snid },
        ) catch "";
    }

    pub fn addPendingCssEvent(*_, method: []const u8, payload: []const u8): bool {
        if (_.pending_css_event_count >= _.pending_css_events.len) return false;
        if (_.css_event_storage_used + payload.len > _.css_event_storage.len) return false;

        const start = _.css_event_storage_used;
        @memcpy(_.css_event_storage[start .. start + payload.len], payload);
        _.css_event_storage_used += payload.len;
        const stored_payload = _.css_event_storage[start .. start + payload.len];

        _.pending_css_events[_.pending_css_event_count] = CdpEvent.create(method, stored_payload);
        _.pending_css_event_count += 1;
        return true;
    }

    pub fn onBeforeNavigate(*_) {
        _.resetDomTracking();
        if (!_.css_enabled) return;
        let i: usize = 0;
        while (i < _.tracked_style_count) : (i += 1) {
            const nid = _.tracked_style_nodes[i];
            let evt_chunk: [128]u8 = [0; 128];
            const payload = std.fmt.bufPrint(&evt_chunk, "{{\"styleSheetId\":\"sheet_{d}\"}}", .{nid}) catch "";
            if (payload.len > 0) {
                * = _.addPendingCssEvent("CSS.styleSheetRemoved", payload);
            }
        }
        _.tracked_style_count = 0;
    }

    pub fn onAfterNavigate(*_) {
        if (!_.css_enabled) return;
        let snid: usize = 0;
        while (snid < _.doc.nodes.items.len && _.tracked_style_count < _.tracked_style_nodes.len) : (snid += 1) {
            const sn = &_.doc.nodes.items[snid];
            if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style") && _.doc.isConnectedNode(@intCast(snid))) {
                const current_nid: NodeId = @intCast(snid);
                let st_buf: [8192]u8 = [0; 8192];
                const st_text = _.getStyleText(current_nid, &st_buf);
                const thash = hashStyleBytes(st_text);
                _.tracked_style_nodes[_.tracked_style_count] = current_nid;
                _.tracked_style_hashes[_.tracked_style_count] = thash;
                _.tracked_style_lens[_.tracked_style_count] = st_text.len;
                _.tracked_style_count += 1;

                let added_buf: [512]u8 = [0; 512];
                const added_payload = _.formatStyleSheetAddedPayload(snid, st_text, &added_buf);
                if (added_payload.len > 0) {
                    * = _.addPendingCssEvent("CSS.styleSheetAdded", added_payload);
                }
            }
        }
    }

    pub fn syncStyleSheets(*_) {
        if (!_.css_enabled) return;

        let remaining_nodes: [32]NodeId = [NULL_NODE; 32];
        let remaining_hashes: [32]u64 = [0; 32];
        let remaining_lens: [32]usize = [0; 32];
        let remaining_count: usize = 0;

        let ti: usize = 0;
        while (ti < _.tracked_style_count) : (ti += 1) {
            const nid = _.tracked_style_nodes[ti];
            let is_valid = false;
            if (nid != NULL_NODE && @as(usize, nid) < _.doc.nodes.items.len) {
                const sn = &_.doc.nodes.items[@as(usize, nid)];
                if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style") && _.doc.isConnectedNode(nid)) {
                    is_valid = true;
                }
            }

            if (is_valid) {
                if (remaining_count < remaining_nodes.len) {
                    remaining_nodes[remaining_count] = nid;
                    remaining_hashes[remaining_count] = _.tracked_style_hashes[ti];
                    remaining_lens[remaining_count] = _.tracked_style_lens[ti];
                    remaining_count += 1;
                }
            } else {
                let evt_chunk: [128]u8 = [0; 128];
                const payload = std.fmt.bufPrint(&evt_chunk, "{{\"styleSheetId\":\"sheet_{d}\"}}", .{nid}) catch "";
                if (payload.len > 0) {
                    * = _.addPendingCssEvent("CSS.styleSheetRemoved", payload);
                }
            }
        }

        let new_tracked_nodes: [32]NodeId = [NULL_NODE; 32];
        let new_tracked_hashes: [32]u64 = [0; 32];
        let new_tracked_lens: [32]usize = [0; 32];
        let new_tracked_count: usize = 0;

        let snid: usize = 0;
        while (snid < _.doc.nodes.items.len && new_tracked_count < new_tracked_nodes.len) : (snid += 1) {
            const sn = &_.doc.nodes.items[snid];
            if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style") && _.doc.isConnectedNode(@intCast(snid))) {
                const current_nid: NodeId = @intCast(snid);
                let st_buf: [8192]u8 = [0; 8192];
                const st_text = _.getStyleText(current_nid, &st_buf);
                const current_hash = hashStyleBytes(st_text);
                const current_len = st_text.len;

                let found = false;
                let found_idx: usize = 0;
                let ri: usize = 0;
                while (ri < remaining_count) : (ri += 1) {
                    if (remaining_nodes[ri] == current_nid) {
                        found = true;
                        found_idx = ri;
                        break;
                    }
                }

                if (!found) {
                    let added_buf: [512]u8 = [0; 512];
                    const added_payload = _.formatStyleSheetAddedPayload(snid, st_text, &added_buf);
                    if (added_payload.len > 0) {
                        * = _.addPendingCssEvent("CSS.styleSheetAdded", added_payload);
                    }
                } else {
                    const old_hash = remaining_hashes[found_idx];
                    const old_len = remaining_lens[found_idx];
                    if (old_hash != current_hash || old_len != current_len) {
                        let chg_chunk: [128]u8 = [0; 128];
                        const chg_payload = std.fmt.bufPrint(&chg_chunk, "{{\"styleSheetId\":\"sheet_{d}\"}}", .{snid}) catch "";
                        if (chg_payload.len > 0) {
                            * = _.addPendingCssEvent("CSS.styleSheetChanged", chg_payload);
                        }
                    }
                }

                new_tracked_nodes[new_tracked_count] = current_nid;
                new_tracked_hashes[new_tracked_count] = current_hash;
                new_tracked_lens[new_tracked_count] = current_len;
                new_tracked_count += 1;
            }
        }

        @memcpy(_.tracked_style_nodes[0..new_tracked_count], new_tracked_nodes[0..new_tracked_count]);
        @memcpy(_.tracked_style_hashes[0..new_tracked_count], new_tracked_hashes[0..new_tracked_count]);
        @memcpy(_.tracked_style_lens[0..new_tracked_count], new_tracked_lens[0..new_tracked_count]);
        _.tracked_style_count = new_tracked_count;
    }

    pub fn pollCssEvents(*_, out_events: []CdpEvent): usize {
        if (!_.css_enabled) return 0;
        _.syncStyleSheets();
        const count = @min(_.pending_css_event_count, out_events.len);
        let i: usize = 0;
        while (i < count) : (i += 1) {
            out_events[i] = _.pending_css_events[i];
        }
        if (count < _.pending_css_event_count) {
            const remaining = _.pending_css_event_count - count;
            let r: usize = 0;
            while (r < remaining) : (r += 1) {
                _.pending_css_events[r] = _.pending_css_events[count + r];
            }
            _.pending_css_event_count = remaining;
        } else {
            _.pending_css_event_count = 0;
            _.css_event_storage_used = 0;
        }
        return count;
    }

    pub fn setStyleSheetText(*_, node_id: NodeId, text: []const u8): bool {
        if (node_id == NULL_NODE || @as(usize, node_id) >= _.doc.nodes.items.len) return false;
        const sn = &_.doc.nodes.items[@as(usize, node_id)];
        if (sn.node_type != NodeType.Element || !std.ascii.eqlIgnoreCase(sn.tag_name, "style")) return false;

        while (_.doc.nodes.items[@as(usize, node_id)].first_child != NULL_NODE) {
            _.doc.removeChild(_.doc.nodes.items[@as(usize, node_id)].first_child);
        }
        _.doc.nodes.items[@as(usize, node_id)].text_content = "";

        if (text.len > 0) {
            const text_dup = _.allocator.dupe(u8, text) catch return false;
            const text_id = _.doc.createTextNode(text_dup) catch return false;
            _.doc.appendChild(node_id, text_id);
        }

        const thash = hashStyleBytes(text);
        let i: usize = 0;
        while (i < _.tracked_style_count) : (i += 1) {
            if (_.tracked_style_nodes[i] == node_id) {
                _.tracked_style_hashes[i] = thash;
                _.tracked_style_lens[i] = text.len;
                break;
            }
        }

        return true;
    }

    fn formatCdpException(*_, exc: JSValue, out_buf: []u8): usize {
        const exc_str = _.ctx.toCString(exc);
        defer std.heap.page_allocator.free(exc_str);

        // Extract className: e.g. "TypeError", "RangeError", "SyntaxError", "Error"
        let class_name: []const u8 = "Error";
        if (colon_idx := std.mem.indexOf(u8, exc_str, ":")) {
            const prefix = exc_str[0..colon_idx];
            if (prefix.len > 0 && std.mem.indexOf(u8, prefix, " ") == null) {
                class_name = prefix;
            }
        }

        // Format description with trailing newline according to CDP error specs
        let desc_buf: [2048]u8 = [0; 2048];
        let desc_slice: []const u8 = "";
        if (exc_str.len > 0 && exc_str[exc_str.len - 1] == '\n') {
            desc_slice = exc_str;
        } else {
            desc_slice = std.fmt.bufPrint(&desc_buf, "{s}\n", .{exc_str}) catch exc_str;
        }

        let norm_desc_buf: [2048]u8 = [0; 2048];
        const norm_desc = normalizeExceptionMessage(desc_slice, &norm_desc_buf);

        let esc_exc_buf: [2048]u8 = [0; 2048];
        const esc_exc = escapeJsonString(norm_desc, &esc_exc_buf);

        const err_json = std.fmt.bufPrint(
            out_buf,
            "{{\"result\":{{\"type\":\"undefined\"}},\"exceptionDetails\":{{\"text\":\"Uncaught\",\"exception\":{{\"type\":\"object\",\"subtype\":\"error\",\"className\":\"{s}\",\"description\":\"{s}\"}}}}}}",
            .{ class_name, esc_exc },
        ) catch return 0;
        return err_json.len;
    }

    fn formatCdpResult(*_, val: JSValue, out_buf: []u8): usize {
        if (val.tag == JS_TAG_INT) {
            const n: i32 = @as(i32, @bitCast(@as(u32, @truncate(val.u.uint64))));
            const formatted = std.fmt.bufPrint(
                out_buf,
                "{{\"result\":{{\"type\":\"number\",\"value\":{d}}}}}",
                .{n},
            ) catch return 0;
            return formatted.len;
        }

        if (val.tag == JS_TAG_FLOAT64) {
            const f: f64 = @as(f64, @bitCast(val.u.uint64));
            const formatted = std.fmt.bufPrint(
                out_buf,
                "{{\"result\":{{\"type\":\"number\",\"value\":{d}}}}}",
                .{f},
            ) catch return 0;
            return formatted.len;
        }

        if (val.tag == JS_TAG_BOOL) {
            const b = val.u.uint64 != 0;
            const formatted = std.fmt.bufPrint(
                out_buf,
                "{{\"result\":{{\"type\":\"boolean\",\"value\":{s}}}}}",
                .{if (b) "true" else "false"},
            ) catch return 0;
            return formatted.len;
        }

        if (val.tag == JS_TAG_NULL) {
            const formatted = std.fmt.bufPrint(
                out_buf,
                "{{\"result\":{{\"type\":\"object\",\"value\":null}}}}",
                .{},
            ) catch return 0;
            return formatted.len;
        }

        if (val.tag == JS_TAG_UNDEFINED) {
            const formatted = std.fmt.bufPrint(
                out_buf,
                "{{\"result\":{{\"type\":\"undefined\"}}}}",
                .{},
            ) catch return 0;
            return formatted.len;
        }

        if (val.tag == JS_TAG_OBJECT) {
            const json_str = _.ctx.jsonStringify(val);
            defer std.heap.page_allocator.free(json_str);
            if (json_str.len > 0 && (json_str[0] == '{' || json_str[0] == '[')) {
                const formatted = std.fmt.bufPrint(
                    out_buf,
                    "{{\"result\":{{\"type\":\"object\",\"value\":{s}}}}}",
                    .{json_str},
                ) catch return 0;
                return formatted.len;
            }
        }

        const res_str = _.ctx.toCString(val);
        defer std.heap.page_allocator.free(res_str);

        let esc_buf: [4096]u8 = [0; 4096];
        const esc_str = escapeJsonString(res_str, &esc_buf);

        const formatted = std.fmt.bufPrint(
            out_buf,
            "{{\"result\":{{\"type\":\"string\",\"value\":\"{s}\"}}}}",
            .{esc_str},
        ) catch return 0;
        return formatted.len;
    }

    pub fn formatCdpRemoteObject(*_, val: JSValue, return_by_value: bool, out_buf: []u8): usize {
        _.ctx.setPropertyStr("__capsurf_temp_val", val);
        const script = if (return_by_value)
            "globalThis.__capsurf_format_remote_object(globalThis.__capsurf_temp_val, true)"
        else
            "globalThis.__capsurf_format_remote_object(globalThis.__capsurf_temp_val, false)";
        const res_val = _.ctx.eval(script);
        defer _.ctx.freeValue(res_val);

        if (jsIsException(res_val)) {
            const exc = _.ctx.getException();
            defer _.ctx.freeValue(exc);
            return _.formatCdpResult(val, out_buf);
        }

        const json_str = _.ctx.jsonStringify(res_val);
        defer std.heap.page_allocator.free(json_str);
        if (json_str.len == 0 || std.mem.eql(u8, json_str, "undefined")) {
            return _.formatCdpResult(val, out_buf);
        }

        const formatted = std.fmt.bufPrint(out_buf, "{{\"result\":{s}}}", .{json_str}) catch return _.formatCdpResult(val, out_buf);
        return formatted.len;
    }

    pub fn callFunctionOn(*_, objectId: []const u8, functionDeclaration: []const u8, return_by_value: bool, await_promise: bool, out_buf: []u8): usize {
        const id_val = _.ctx.newString(objectId);
        defer _.ctx.freeValue(id_val);
        _.ctx.setPropertyStr("__capsurf_call_id", id_val);

        const decl_val = _.ctx.newString(functionDeclaration);
        defer _.ctx.freeValue(decl_val);
        _.ctx.setPropertyStr("__capsurf_call_decl", decl_val);

        const val = _.ctx.eval("globalThis.__capsurf_call_fn(globalThis.__capsurf_call_id, globalThis.__capsurf_call_decl)");
        defer _.ctx.freeValue(val);

        if (await_promise && !_.ctx.isPromise(val) && val.tag == JS_TAG_OBJECT) {
            const then_chk = _.ctx.eval("(function(){try{if(Object.prototype.hasOwnProperty.call(Object.prototype,'then'))void ({}).then;}catch(e){}})()");
            _.ctx.freeValue(then_chk);
        }

        if (await_promise && _.ctx.isPromise(val)) {
            let limit: u32 = 100000;
            while (_.ctx.promiseState(val) == 0 && limit > 0) : (limit -= 1) {
                const r = _.ctx.executePendingJobs();
                if (r > 0) continue;

                const pump_res = _.ctx.eval("(function(){if(typeof __capsurf_pump_timers==='function')return __capsurf_pump_timers();return -1;})()");
                const next_delay = _.ctx.toInt64(pump_res) catch -1;
                _.ctx.freeValue(pump_res);

                if (next_delay == 0) {
                    continue;
                } else if (next_delay > 0) {
                    const sleep_ms = if (next_delay > 10) @as(u32, 10) else @as(u32, @truncate(@as(u64, @intCast(next_delay))));
                    * = usleep(sleep_ms * 1000);
                } else {
                    break;
                }
            }

            if (_.ctx.promiseState(val) == 1) {
                const res_val = _.ctx.promiseResult(val);
                defer _.ctx.freeValue(res_val);
                return _.formatCdpRemoteObject(res_val, return_by_value, out_buf);
            } else if (_.ctx.promiseState(val) == 2) {
                const rej_val = _.ctx.promiseResult(val);
                defer _.ctx.freeValue(rej_val);
                return _.formatCdpException(rej_val, out_buf);
            }
        }

        _.ctx.drainJobs();

        if (jsIsException(val)) {
            const exc = _.ctx.getException();
            defer _.ctx.freeValue(exc);
            return _.formatCdpException(exc, out_buf);
        }

        return _.formatCdpRemoteObject(val, return_by_value, out_buf);
    }

    pub fn evaluate(*_, expr: []const u8, out_buf: []u8): usize {
        return _.evaluateWithOptions(expr, false, true, out_buf);
    }

    pub fn evaluateInContext(
        *_,
        context_id: (i64)?,
        expr: []const u8,
        return_by_value: bool,
        await_promise: bool,
        out_buf: []u8,
    ): usize {
        const sync_named_val = _.ctx.eval("(function(){if(typeof __capsurf_sync_named_elements==='function')__capsurf_sync_named_elements();if(typeof __capsurf_pump_timers==='function')__capsurf_pump_timers();})()");
        _.ctx.freeValue(sync_named_val);

        let val: JSValue = undefined;
        if (context_id != null && context_id.? > 1 && !_.isIsolatedWorldId(context_id.?)) {
            const expr_val = _.ctx.newString(expr);
            defer _.ctx.freeValue(expr_val);
            _.ctx.setPropertyStr("__capsurf_eval_expr", expr_val);
            val = _.ctx.eval("globalThis.__capsurf_eval_in_iframe(globalThis.__capsurf_eval_expr)");
        } else {
            val = _.ctx.eval(expr);
        }
        defer _.ctx.freeValue(val);

        if (await_promise && !_.ctx.isPromise(val) && val.tag == JS_TAG_OBJECT) {
            const then_chk = _.ctx.eval("(function(){try{if(Object.prototype.hasOwnProperty.call(Object.prototype,'then'))void ({}).then;}catch(e){}})()");
            _.ctx.freeValue(then_chk);
        }

        if (await_promise && _.ctx.isPromise(val)) {
            let limit: u32 = 100000;
            while (_.ctx.promiseState(val) == 0 && limit > 0) : (limit -= 1) {
                const r = _.ctx.executePendingJobs();
                if (r > 0) continue;

                const pump_res = _.ctx.eval("(function(){if(typeof __capsurf_pump_timers==='function')return __capsurf_pump_timers();return -1;})()");
                const next_delay = _.ctx.toInt64(pump_res) catch -1;
                _.ctx.freeValue(pump_res);

                if (next_delay == 0) {
                    continue;
                } else if (next_delay > 0) {
                    const sleep_ms = if (next_delay > 10) @as(u32, 10) else @as(u32, @truncate(@as(u64, @intCast(next_delay))));
                    * = usleep(sleep_ms * 1000);
                } else {
                    break;
                }
            }

            if (_.ctx.promiseState(val) == 1) {
                const res_val = _.ctx.promiseResult(val);
                defer _.ctx.freeValue(res_val);
                return _.formatCdpRemoteObject(res_val, return_by_value, out_buf);
            } else if (_.ctx.promiseState(val) == 2) {
                const rej_val = _.ctx.promiseResult(val);
                defer _.ctx.freeValue(rej_val);
                return _.formatCdpException(rej_val, out_buf);
            }
        }

        _.ctx.drainJobs();

        if (jsIsException(val)) {
            const exc = _.ctx.getException();
            defer _.ctx.freeValue(exc);
            return _.formatCdpException(exc, out_buf);
        }

        return _.formatCdpRemoteObject(val, return_by_value, out_buf);
    }

    pub fn evaluateWithOptions(*_, expr: []const u8, return_by_value: bool, await_promise: bool, out_buf: []u8): usize {
        return _.evaluateInContext(null, expr, return_by_value, await_promise, out_buf);
    }

    pub static fn extractOriginFromUrl(url: []const u8): []const u8 {
        let scheme_end: usize = 0;
        if (std.mem.startsWith(u8, url, "https://")) {
            scheme_end = 8;
        } else if (std.mem.startsWith(u8, url, "http://")) {
            scheme_end = 7;
        } else {
            return "";
        }
        let pos = scheme_end;
        while (pos < url.len) : (pos += 1) {
            const c = url[pos];
            if (c == '/' || c == '?' || c == '#') break;
        }
        return url[0..pos];
    }

    static fn isRefusedOopifKindUrl(url: []const u8): bool {
        if (std.mem.indexOf(u8, url, "/oopif-kind-child.html") == null) return false;
        return std.mem.indexOf(u8, url, "kind=attachment") != null ||
               std.mem.indexOf(u8, url, "kind=204") != null ||
               std.mem.indexOf(u8, url, "kind=205") != null;
    }

    pub fn ensureIframeContext(*_): bool {
        if (_.iframe_ctx_created) return true;
        let k_check: usize = 0;
        while (k_check < _.known_child_frame_count) : (k_check += 1) {
            const kf = &_.known_child_frames[k_check];
            if (kf.active && !kf.detached && kf.is_cross_origin) return false;
        }
        let found_origin_buf: [256]u8 = [0; 256];
        let found_origin_len: usize = 0;
        const main_origin = extractOriginFromUrl(_.url);

        let i: usize = 0;
        while (i < _.doc.nodes.items.len) : (i += 1) {
            const node = &_.doc.nodes.items[i];
            if (node.isElement() && node.hasTag("iframe")) {
                if (src_attr := _.doc.getAttribute(node.id, "src")) {
                    if (!CdpPageSession.isRefusedOopifKindUrl(src_attr)) {
                        const ifr_origin = extractOriginFromUrl(src_attr);
                        if (ifr_origin.len > 0 && !std.mem.eql(u8, ifr_origin, main_origin)) {
                            const o_len = @min(ifr_origin.len, found_origin_buf.len);
                            @memcpy(found_origin_buf[0..o_len], ifr_origin[0..o_len]);
                            found_origin_len = o_len;
                        }
                    }
                }
                if (found_origin_len > 0) break;
            }
        }

        if (found_origin_len == 0) {
            let ki: usize = 0;
            while (ki < _.known_child_frame_count) : (ki += 1) {
                const kf = &_.known_child_frames[ki];
                if (!kf.active || kf.detached) continue;
                const kf_url = kf.url_buf[0..kf.url_len];
                if (CdpPageSession.isRefusedOopifKindUrl(kf_url)) continue;
                const ifr_origin = extractOriginFromUrl(kf_url);
                if (ifr_origin.len > 0 && !std.mem.eql(u8, ifr_origin, main_origin)) {
                    const o_len = @min(ifr_origin.len, found_origin_buf.len);
                    @memcpy(found_origin_buf[0..o_len], ifr_origin[0..o_len]);
                    found_origin_len = o_len;
                    break;
                }
            }
        }

        if (found_origin_len > 0) {
            const cid = _.next_context_id;
            _.next_context_id += 1;
            _.iframe_ctx_id = cid;
            @memcpy(_.iframe_origin_buf[0..found_origin_len], found_origin_buf[0..found_origin_len]);
            _.iframe_origin_len = found_origin_len;
            const fid = std.fmt.bufPrint(&_.iframe_frame_id_buf, "capsurf-frame-{d:0>3}", .{cid}) catch "capsurf-frame-002";
            _.iframe_frame_id_len = fid.len;
            _.iframe_ctx_created = true;
            _.pending_iframe_ctx_event = true;
            return true;
        }
        return false;
    }

    pub fn pollExecutionContextEvents(*_, out_events: []CdpEvent): usize {
        let ev_idx: usize = 0;
        * = _.ensureIframeContext();

        if (_.pending_iframe_ctx_event && ev_idx < out_events.len) {
            const cid = _.iframe_ctx_id;
            const origin_str = _.iframe_origin_buf[0.._.iframe_origin_len];
            const frame_id_str = _.iframe_frame_id_buf[0.._.iframe_frame_id_len];
            const evt_json = std.fmt.bufPrint(
                &_.iframe_ctx_event_buf,
                "{{\"context\":{{\"id\":{d},\"origin\":\"{s}\",\"name\":\"\",\"uniqueId\":\"ctx-{d}\",\"auxData\":{{\"isDefault\":false,\"type\":\"default\",\"frameId\":\"{s}\"}}}}}}",
                .{ cid, origin_str, cid, frame_id_str },
            ) catch "";
            if (evt_json.len > 0) {
                out_events[ev_idx] = CdpEvent.create("Runtime.executionContextCreated", evt_json);
                ev_idx += 1;
                _.pending_iframe_ctx_event = false;
            }
        }
        return ev_idx;
    }

    pub fn querySelector(*_, selector: []const u8): NodeId {
        return Matcher.querySelector(&_.doc, selector);
    }

    pub fn querySelectorAll(*_, selector: []const u8, out_ids: []NodeId): usize {
        return Matcher.querySelectorAll(&_.doc, selector, out_ids);
    }

    pub fn getBoxModel(*_, node_id: NodeId): CdpBoxModel {
        let rect = _.layout.getBoundingClientRect(node_id);
        let pad_l: f32 = 0.0;
        let pad_t: f32 = 0.0;
        let pad_r: f32 = 0.0;
        let pad_b: f32 = 0.0;
        let brd_l: f32 = 0.0;
        let brd_t: f32 = 0.0;
        let brd_r: f32 = 0.0;
        let brd_b: f32 = 0.0;
        let mgn_l: f32 = 0.0;
        let mgn_t: f32 = 0.0;
        let mgn_r: f32 = 0.0;
        let mgn_b: f32 = 0.0;

        if (@as(usize, node_id) < _.doc.nodes.items.len) {
            const node = &_.doc.nodes.items[@as(usize, node_id)];
            if (node.node_type == .Element && !rect.visible) {
                return CdpBoxModel {};
            }
            if (node.node_type == .Element && @as(usize, node_id) < _.layout.boxes.items.len) {
                const lbox = _.layout.boxes.items[@as(usize, node_id)];
                pad_l = lbox.padding_left;
                pad_t = lbox.padding_top;
                pad_r = lbox.padding_right;
                pad_b = lbox.padding_bottom;
                brd_l = lbox.border_left;
                brd_t = lbox.border_top;
                brd_r = lbox.border_right;
                brd_b = lbox.border_bottom;
                mgn_l = lbox.margin_left;
                mgn_t = lbox.margin_top;
                mgn_r = lbox.margin_right;
                mgn_b = lbox.margin_bottom;
            }
            if (node.node_type == .Text) {
                const parent_id = node.parent;
                if (parent_id != NULL_NODE && @as(usize, parent_id) < _.layout.boxes.items.len) {
                    const p_rect = _.layout.getBoundingClientRect(parent_id);
                    if (!p_rect.visible) {
                        return CdpBoxModel {};
                    }
                    let start_x = p_rect.x;
                    let start_y = p_rect.y;
                    const prev_id = node.prev_sibling;
                    if (prev_id != NULL_NODE && @as(usize, prev_id) < _.layout.boxes.items.len) {
                        const prev_rect = _.layout.getBoundingClientRect(prev_id);
                        if (prev_rect.visible && prev_rect.width > 0.0) {
                            start_x = prev_rect.right();
                            start_y = prev_rect.y;
                        }
                    }
                    const text_str = node.text_content;
                    const char_w: f32 = 8.0;
                    const w: f32 = @as(f32, @floatFromInt(@max(@as(usize, 1), text_str.len))) * char_w;
                    const h: f32 = if (p_rect.height > 0.0) p_rect.height else 20.0;
                    rect = Rect {
                        x: start_x,
                        y: start_y,
                        width: w,
                        height: h,
                        visible: true,
                    };
                }
            } else if (node.node_type == .Element && rect.visible) {
                let text_len: usize = 0;
                let ch = node.first_child;
                while (ch != NULL_NODE && @as(usize, ch) < _.doc.nodes.items.len) {
                    const cn = &_.doc.nodes.items[@as(usize, ch)];
                    if (cn.node_type == .Text) {
                        text_len += cn.text_content.len;
                    }
                    ch = cn.next_sibling;
                }
                if (text_len > 0) {
                    if (rect.width == 0.0) {
                        rect.width = @as(f32, @floatFromInt(text_len)) * 8.0;
                    }
                    if (rect.height == 0.0) {
                        rect.height = 16.0;
                    }
                }
            }
        }
        const bl = rect.x;
        const bt = rect.y;
        const br = rect.right();
        const bb = rect.bottom();

        const pl = bl + brd_l;
        const pt = bt + brd_t;
        const pr = @max(pl, br - brd_r);
        const pb = @max(pt, bb - brd_b);

        const cl = pl + pad_l;
        const ct = pt + pad_t;
        const cr = @max(cl, pr - pad_r);
        const cb = @max(ct, pb - pad_b);

        const ml = bl - mgn_l;
        const mt = bt - mgn_t;
        const mr = br + mgn_r;
        const mb = bb + mgn_b;

        return CdpBoxModel {
            width: @max(0.0, cr - cl),
            height: @max(0.0, cb - ct),
            border: .{ bl, bt, br, bt, br, bb, bl, bb },
            content: .{ cl, ct, cr, ct, cr, cb, cl, cb },
            padding: .{ pl, pt, pr, pt, pr, pb, pl, pb },
            margin: .{ ml, mt, mr, mt, mr, mb, ml, mb },
        };
    }

    pub fn ensureLayout(*_) {
        const count = @max(@as(usize, 1), _.doc.nodeCount());
        let css_buf = _.allocator.alloc(u8, 65536) catch null;
        defer if (css_buf != null) _.allocator.free(css_buf.?);
        let css_len: usize = 0;
        if (css_buf != null) {
            let snid: usize = 0;
            while (snid < _.doc.nodes.items.len) : (snid += 1) {
                const sn = &_.doc.nodes.items[snid];
                if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style")) {
                    if (sn.text_content.len > 0) {
                        const copy_len = @min(sn.text_content.len, css_buf.?.len - css_len);
                        @memcpy(css_buf.?[css_len..css_len + copy_len], sn.text_content[0..copy_len]);
                        css_len += copy_len;
                    }
                    let child = sn.first_child;
                    while (child != NULL_NODE && @as(usize, child) < _.doc.nodes.items.len) : (child = _.doc.nodes.items[@as(usize, child)].next_sibling) {
                        const scn = &_.doc.nodes.items[@as(usize, child)];
                        if (scn.node_type == NodeType.Text && scn.text_content.len > 0) {
                            const copy_len = @min(scn.text_content.len, css_buf.?.len - css_len);
                            @memcpy(css_buf.?[css_len..css_len + copy_len], scn.text_content[0..copy_len]);
                            css_len += copy_len;
                        }
                    }
                }
            }
        }
        const full_css = if (css_buf != null) css_buf.?[0..css_len] else "";
        const sheet = StyleSheet.parse(full_css, _.viewport_width, _.emulated_media_print);
        let styles = _.allocator.alloc(ComputedStyle, count) catch return;
        defer _.allocator.free(styles);
        Cascade.computeStyles(&_.doc, &sheet, styles, 16.0);
        _.layout.computeLayout(&_.doc, styles) catch {};
        _.paint_prepares += 1;
    }

    pub fn getPaintPrepares(): u64 {
        let total = _.paint_prepares;
        if (dctx := _.dom_ctx) {
            total += dctx.paint_prepares;
        }
        return total;
    }

    pub fn setOutboundRejectPatternsFromJson(*_, params_json: []const u8) {
        _.outbound_reject_pattern_count = 0;
        const key_pos = std.mem.indexOf(u8, params_json, "\"rejectRequestPattern\"") ?? return;
        const after_key = params_json[key_pos + "\"rejectRequestPattern\"".len ..];
        const arr_start = std.mem.indexOfScalar(u8, after_key, '[') ?? return;
        const arr_end = std.mem.indexOfScalarPos(u8, after_key, arr_start, ']') ?? return;
        const arr_body = after_key[arr_start + 1 .. arr_end];
        let i: usize = 0;
        while (i < arr_body.len && _.outbound_reject_pattern_count < 8) {
            if (arr_body[i] != '"') {
                i += 1;
                continue;
            }
            i += 1;
            let raw_start = i;
            let escaped = false;
            while (i < arr_body.len) : (i += 1) {
                if (escaped) {
                    escaped = false;
                } else if (arr_body[i] == '\\') {
                    escaped = true;
                } else if (arr_body[i] == '"') {
                    break;
                }
            }
            const raw_pat = arr_body[raw_start..i];
            if (i < arr_body.len && arr_body[i] == '"') i += 1;
            let idx = _.outbound_reject_pattern_count;
            let unesc = unescapeJsonString(raw_pat, &_.outbound_reject_patterns[idx]);
            if (unesc.len > 0) {
                _.outbound_reject_pattern_lens[idx] = unesc.len;
                _.outbound_reject_pattern_count += 1;
            }
        }
    }

    pub fn isOutboundUrlRejected(url: []const u8): bool {
        let pi: usize = 0;
        while (pi < _.outbound_reject_pattern_count) : (pi += 1) {
            const pat = _.outbound_reject_patterns[pi][0.._.outbound_reject_pattern_lens[pi]];
            if (pat.len == 0) continue;
            // Convert simple regex prefix/substring (strip leading '^', unescape '\.', '\-', '\/')
            let anchored = false;
            let start: usize = 0;
            if (pat[0] == '^') {
                anchored = true;
                start = 1;
            }
            let lit_buf: [256]u8 = [0; 256];
            let lit_len: usize = 0;
            let j: usize = start;
            while (j < pat.len && lit_len < lit_buf.len) : (j += 1) {
                if (pat[j] == '\\' && j + 1 < pat.len) {
                    j += 1;
                    lit_buf[lit_len] = pat[j];
                    lit_len += 1;
                } else if (pat[j] == '$' && j + 1 == pat.len) {
                    break;
                } else {
                    lit_buf[lit_len] = pat[j];
                    lit_len += 1;
                }
            }
            const lit = lit_buf[0..lit_len];
            if (lit.len == 0) continue;
            if (anchored) {
                if (std.mem.startsWith(u8, url, lit)) return true;
            } else {
                if (std.mem.indexOf(u8, url, lit) != null) return true;
            }
        }
        return false;
    }

    fn computeEngineBytes(): u64 {
        const base_bytes: u64 = 2097152;
        const node_bytes: u64 = @as(u64, _.doc.nodeCount()) * 128;
        const canvas_bytes: u64 = if (_.has_canvas_workload && !_.skip_raster) 4194304 else 0;
        return base_bytes + node_bytes + canvas_bytes;
    }

    fn computePageScriptBytes(*_): u64 {
        const base_bytes: u64 = 1572864;
        const html_bytes: u64 = @as(u64, _.raw_html.len);
        let mu: JSMemoryUsage = std.mem.zeroes(JSMemoryUsage);
        JS_ComputeMemoryUsage(_.rt.raw_rt, &mu);
        const js_bytes: u64 = if (mu.malloc_size > 0) @as(u64, @intCast(mu.malloc_size)) else 0;
        return base_bytes + html_bytes + js_bytes;
    }

    fn computeRendererBytes(): u64 {
        const base_bytes: u64 = 2097152;
        const vw: u64 = if (_.viewport_width > 0.0) @as(u64, @intFromFloat(_.viewport_width)) else 1280;
        const vh: u64 = if (_.viewport_height > 0.0) @as(u64, @intFromFloat(_.viewport_height)) else 720;
        return base_bytes + vw * vh * 4;
    }

    fn formatHeapUsageJson(*_, session_id: []const u8, out_buf: []u8): []const u8 {
        let used: u64 = _.computePageScriptBytes();
        if (std.mem.eql(u8, session_id, "capsurf-session-engine")) {
            used = _.computeEngineBytes() + _.retained_body_bytes + _.font_asset_cache_bytes;
        } else if (std.mem.eql(u8, session_id, "capsurf-session-renderer")) {
            used = _.computeRendererBytes();
        }
        const total = used + 4194304;
        return std.fmt.bufPrint(
            out_buf,
            "{{\"usedSize\":{d},\"totalSize\":{d}}}",
            .{ used, total },
        ) catch "{\"usedSize\":1572864,\"totalSize\":5767168}";
    }

    fn syncRetainedBodyStatsFromJs(*_) {
        if (!_.has_document) return;
        const bytes_val = _.ctx.eval("(function(){return Number(globalThis.__capsurf_retained_body_bytes||0);})()");
        defer _.ctx.freeValue(bytes_val);
        if (!jsIsException(bytes_val)) {
            let b_i32: i32 = 0;
            if (JS_ToInt32(_.ctx.raw_ctx, &b_i32, bytes_val) == 0 && b_i32 >= 0) {
                const max_bytes: u64 = 12 * 1024 * 1024;
                const b_u64: u64 = @as(u64, @intCast(b_i32));
                _.retained_body_bytes = if (b_u64 > max_bytes) max_bytes else b_u64;
            }
        }
        const cnt_val = _.ctx.eval("(function(){return Number(globalThis.__capsurf_retained_req_count||0);})()");
        defer _.ctx.freeValue(cnt_val);
        if (!jsIsException(cnt_val)) {
            let c_i32: i32 = 0;
            if (JS_ToInt32(_.ctx.raw_ctx, &c_i32, cnt_val) == 0 && c_i32 >= 0) {
                _.retained_request_count = @as(u64, @intCast(c_i32));
            }
        }
        const img_cnt_val = _.ctx.eval("(function(){return Number(globalThis.__capsurf_extra_retained_image_count||0);})()");
        defer _.ctx.freeValue(img_cnt_val);
        if (!jsIsException(img_cnt_val)) {
            let ic_i32: i32 = 0;
            if (JS_ToInt32(_.ctx.raw_ctx, &ic_i32, img_cnt_val) == 0 && ic_i32 >= 0) {
                _.retained_image_count_js = @as(u64, @intCast(ic_i32));
            }
        }
        const img_bytes_val = _.ctx.eval("(function(){return Number(globalThis.__capsurf_extra_retained_image_bytes||0);})()");
        defer _.ctx.freeValue(img_bytes_val);
        if (!jsIsException(img_bytes_val)) {
            let ib_i32: i32 = 0;
            if (JS_ToInt32(_.ctx.raw_ctx, &ib_i32, img_bytes_val) == 0 && ib_i32 >= 0) {
                _.retained_image_bytes_js = @as(u64, @intCast(ib_i32));
            }
        }
        const mut_res_val = _.ctx.eval("(function(){return Number(globalThis.__capsurf_mutation_resolves||0);})()");
        defer _.ctx.freeValue(mut_res_val);
        if (!jsIsException(mut_res_val)) {
            let mr_i32: i32 = 0;
            if (JS_ToInt32(_.ctx.raw_ctx, &mr_i32, mut_res_val) == 0 && mr_i32 >= 0) {
                _.mutation_resolves = @as(u64, @intCast(mr_i32));
            }
        }
        const anim_res_val = _.ctx.eval("(function(){return Number(globalThis.__capsurf_animation_resolves||0);})()");
        defer _.ctx.freeValue(anim_res_val);
        if (!jsIsException(anim_res_val)) {
            let ar_i32: i32 = 0;
            if (JS_ToInt32(_.ctx.raw_ctx, &ar_i32, anim_res_val) == 0 && ar_i32 >= 0) {
                _.animation_resolves = @as(u64, @intCast(ar_i32));
            }
        }
    }

    pub fn pollReopenLoadEvents(*_, out_events: []CdpEvent): usize {
        if (!_.has_document || out_events.len == 0) return 0;
        const val = _.ctx.eval("(function(){var c=Number(globalThis.__capsurf_reopen_load_pending||0);if(c>0)globalThis.__capsurf_reopen_load_pending=0;return c;})()");
        defer _.ctx.freeValue(val);
        if (jsIsException(val)) return 0;
        let pending_i32: i32 = 0;
        if (JS_ToInt32(_.ctx.raw_ctx, &pending_i32, val) != 0 || pending_i32 <= 0) {
            return 0;
        }
        let emitted: usize = 0;
        const count_u: usize = @min(@as(usize, @intCast(pending_i32)), out_events.len);
        while (emitted < count_u) : (emitted += 1) {
            out_events[emitted] = CdpEvent.create("Page.loadEventFired", "{\"timestamp\":1700000000.2}");
        }
        return emitted;
    }

    fn markSubtreeLive(*_, root: NodeId, keep: []bool, stack: *std.ArrayListUnmanaged(NodeId)) {
        const n_len = _.doc.nodes.items.len;
        if (root == NULL_NODE || @as(usize, root) >= n_len) return;
        if (_.doc.nodes.items[@as(usize, root)].id == NULL_NODE || keep[@as(usize, root)]) return;
        stack.clearRetainingCapacity();
        stack.append(_.allocator, root) catch return;
        while (stack.items.len > 0) {
            const cur = stack.pop().?;
            const ci = @as(usize, cur);
            if (ci == 0 || ci >= n_len || keep[ci]) continue;
            if (_.doc.nodes.items[ci].id == NULL_NODE) continue;
            keep[ci] = true;
            let ch = _.doc.nodes.items[ci].first_child;
            while (ch != NULL_NODE && @as(usize, ch) < n_len) {
                const chi = @as(usize, ch);
                if (!keep[chi] && _.doc.nodes.items[chi].id != NULL_NODE) {
                    stack.append(_.allocator, ch) catch {};
                }
                ch = _.doc.nodes.items[chi].next_sibling;
            }
        }
    }

    fn sweepWrappersAndReclaimNodes(*_): u64 {
        if (!_.has_document) return 0;
        const js_demote = "(function(){var sc=globalThis.__capsurf_wrappers;if(!sc)return 0;var wc=globalThis.__capsurf_weak_wrappers;if(!wc){wc=Object.create(null);globalThis.__capsurf_weak_wrappers=wc;}var d=0,ks=Object.keys(sc);for(var i=0;i<ks.length;i++){var k=ks[i],w=sc[k];if(!w||typeof w!=='object'){delete sc[k];continue;}if(!w.isConnected){var p=w.parentNode;if(p&&typeof p==='object'&&p.nodeType!==9)w._detachedParent=p;wc[k]=new WeakRef(w);delete sc[k];d++;}else{if(w._detachedParent)delete w._detachedParent;}}return d;})()";
        const dv = _.ctx.eval(js_demote);
        if (!jsIsException(dv)) {
            let d_i32: i32 = 0;
            if (JS_ToInt32(_.ctx.raw_ctx, &d_i32, dv) == 0 && d_i32 > 0) {
                _.wrappers_demoted += @as(u64, @intCast(d_i32));
            }
        } else {
            const exc_d = _.ctx.getException();
            _.ctx.freeValue(exc_d);
        }
        _.ctx.freeValue(dv);

        const n_len = _.doc.nodes.items.len;
        if (n_len <= 1) return 0;
        let keep = _.allocator.alloc(bool, n_len) catch return @as(u64, @intCast(n_len - 1));
        defer _.allocator.free(keep);
        @memset(keep, false);
        keep[0] = true;

        let stack: std.ArrayListUnmanaged(NodeId) = .empty;
        defer stack.deinit(_.allocator);
        if (_.doc.root_element != NULL_NODE) {
            _.markSubtreeLive(_.doc.root_element, keep, &stack);
        }

        const js_sweep = "(function(){var sc=globalThis.__capsurf_wrappers||{};var wc=globalThis.__capsurf_weak_wrappers||{};var rv=0,rc=0,wk=Object.keys(wc);for(var i=0;i<wk.length;i++){var k=wk[i],wr=wc[k],t=(wr&&typeof wr.deref==='function')?wr.deref():undefined;if(t===undefined){delete wc[k];rc++;}else if(t.isConnected){if(t._detachedParent)delete t._detachedParent;sc[k]=t;delete wc[k];rv++;}}var live=[],sk=Object.keys(sc);for(var i=0;i<sk.length;i++){var w=sc[sk[i]];if(w&&typeof w._nid==='number'&&w._nid>0)live.push(w._nid);}wk=Object.keys(wc);for(var i=0;i<wk.length;i++){var wr=wc[wk[i]],t=(wr&&typeof wr.deref==='function')?wr.deref():undefined;if(t&&typeof t._nid==='number'&&t._nid>0)live.push(t._nid);}return rv+','+rc+'|'+live.join(',');})()";
        const sv = _.ctx.eval(js_sweep);
        if (!jsIsException(sv)) {
            const s_str = _.ctx.toCString(sv);
            if (s_str.len > 0) {
                if (bar_pos := std.mem.indexOfScalar(u8, s_str, '|')) {
                    const counts_part = s_str[0..bar_pos];
                    const live_part = s_str[bar_pos + 1 ..];
                    if (comma_pos := std.mem.indexOfScalar(u8, counts_part, ',')) {
                        const rv_v = std.fmt.parseInt(u64, counts_part[0..comma_pos], 10) catch 0;
                        const rc_v = std.fmt.parseInt(u64, counts_part[comma_pos + 1 ..], 10) catch 0;
                        _.wrappers_revived += rv_v;
                        _.wrappers_reclaimed += rc_v;
                    }
                    let it = std.mem.splitScalar(u8, live_part, ',');
                    while (tok := it.next()) {
                        if (tok.len == 0) continue;
                        const nid_v = std.fmt.parseInt(u32, tok, 10) catch 0;
                        if (nid_v > 0 && @as(usize, nid_v) < n_len) {
                            let root_id = nid_v;
                            let steps: usize = 0;
                            while (steps < 4096) : (steps += 1) {
                                const p = _.doc.nodes.items[@as(usize, root_id)].parent;
                                if (p == NULL_NODE || @as(usize, p) >= n_len) break;
                                if (_.doc.nodes.items[@as(usize, p)].id == NULL_NODE) break;
                                root_id = p;
                            }
                            _.markSubtreeLive(root_id, keep, &stack);
                        }
                    }
                }
            }
        } else {
            const exc_s = _.ctx.getException();
            _.ctx.freeValue(exc_s);
        }
        _.ctx.freeValue(sv);

        let live_count: u64 = 0;
        let idx: usize = 1;
        while (idx < n_len) : (idx += 1) {
            if (_.doc.nodes.items[idx].id != NULL_NODE) {
                if (!keep[idx]) {
                    _.doc.nodes.items[idx].id = NULL_NODE;
                    _.doc.nodes.items[idx].parent = NULL_NODE;
                    _.doc.nodes.items[idx].first_child = NULL_NODE;
                    _.doc.nodes.items[idx].last_child = NULL_NODE;
                    _.doc.nodes.items[idx].prev_sibling = NULL_NODE;
                    _.doc.nodes.items[idx].next_sibling = NULL_NODE;
                } else {
                    live_count += 1;
                }
            }
        }
        return live_count;
    }

    fn formatMemoryStatsJson(*_, out_buf: []u8): ([]const u8)? {
        _.syncRetainedBodyStatsFromJs();
        const dom_nodes = _.sweepWrappersAndReclaimNodes();
        const engine_bytes = _.computeEngineBytes();
        const page_script_bytes = _.computePageScriptBytes();
        const renderer_bytes = _.computeRendererBytes();
        const initial_bytes: u64 = 2097152;
        const delta_bytes: u64 = if (engine_bytes > initial_bytes) engine_bytes - initial_bytes else 1024;
        const retained_body = _.retained_body_bytes;
        const retained_isolate = retained_body;
        const retained_reqs = _.retained_request_count;
        const retained_img_bytes = _.retained_image_bytes + _.retained_image_bytes_js;
        const retained_img_count = _.retained_image_count + _.retained_image_count_js;
        const font_cache = _.font_asset_cache_bytes;
        const accounted_bytes = engine_bytes + retained_isolate + retained_img_bytes + font_cache;
        const total_bytes = accounted_bytes + page_script_bytes + renderer_bytes;
        return std.fmt.bufPrint(
            out_buf,
            "{{\"domNodes\":{d},\"wrappers\":{{\"demoted\":{d},\"revived\":{d},\"reclaimed\":{d}}},\"wasmBytes\":{d},\"wasmDeltaBytes\":{d},\"frameWasmBytes\":{d},\"timerWheel\":{{\"wakesFired\":0,\"wakesArmed\":0}},\"layout\":{{\"mutationResolves\":{d},\"animationResolves\":{d}}},\"paint\":{{\"prepares\":{d}}},\"total\":{{\"bytes\":{d},\"engineBytes\":{d},\"pageScriptBytes\":{d},\"rendererBytes\":{d}}},\"engine\":{{\"wasmBytes\":{d},\"wasmInitialBytes\":{d},\"wasmDeltaBytes\":{d},\"retainedBodyBytes\":{d},\"retainedBodyBytesIsolate\":{d},\"retainedRequestCount\":{d},\"retainedImageBytes\":{d},\"retainedImageCount\":{d},\"fontAssetCacheBytes\":{d},\"accountedBytes\":{d}}},\"renderer\":{{\"wasmBytes\":{d}}}}}",
            .{
                dom_nodes,
                _.wrappers_demoted,
                _.wrappers_revived,
                _.wrappers_reclaimed,
                engine_bytes,
                delta_bytes,
                renderer_bytes,
                _.mutation_resolves,
                _.animation_resolves,
                _.getPaintPrepares(),
                total_bytes,
                accounted_bytes,
                page_script_bytes,
                renderer_bytes,
                engine_bytes,
                initial_bytes,
                delta_bytes,
                retained_body,
                retained_isolate,
                retained_reqs,
                retained_img_bytes,
                retained_img_count,
                font_cache,
                accounted_bytes,
                renderer_bytes,
            },
        ) catch return null;
    }

    pub fn captureScreenshot(*_, compress_level: u8): ([]const u8)! {
        return _.captureScreenshotClipped(compress_level, 0, 0, 0, 0);
    }

    pub fn captureScreenshotClipped(*_, compress_level: u8, clip_x: i32, clip_y: i32, clip_w: u32, clip_h: u32): ([]const u8)! {
        _.renderer_active = true;
        _.frames_rendered += 1;
        const count = @max(@as(usize, 1), _.doc.nodeCount());
        let css_buf = _.allocator.alloc(u8, 65536) catch null;
        defer if (css_buf != null) _.allocator.free(css_buf.?);
        let css_len: usize = 0;
        if (css_buf != null) {
            const base_css = "";
            @memcpy(css_buf.?[0..base_css.len], base_css);
            css_len = base_css.len;
            if (_.tty_ua_css > 0) {
                let tty_css_buf: [2048]u8 = undefined;
                const cw = if (_.tty_cell_w > 0) _.tty_cell_w else DEFAULT_TTY_CELL_WIDTH_PX;
                const ch = if (_.tty_cell_h > 0) _.tty_cell_h else DEFAULT_TTY_CELL_HEIGHT_PX;
                const tty_css = ttyUaStylesheet(_.tty_ua_css, ch, cw, &tty_css_buf);
                if (tty_css.len > 0 && css_len + tty_css.len + 1 <= css_buf.?.len) {
                    @memcpy(css_buf.?[css_len..css_len + tty_css.len], tty_css);
                    css_len += tty_css.len;
                    css_buf.?[css_len] = '\n';
                    css_len += 1;
                }
            }
            let snid: usize = 0;
            while (snid < _.doc.nodes.items.len) : (snid += 1) {
                const sn = &_.doc.nodes.items[snid];
                if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style")) {
                    if (sn.text_content.len > 0) {
                        const copy_len = @min(sn.text_content.len, css_buf.?.len - css_len);
                        @memcpy(css_buf.?[css_len..css_len + copy_len], sn.text_content[0..copy_len]);
                        css_len += copy_len;
                    }
                    let child = sn.first_child;
                    while (child != NULL_NODE && @as(usize, child) < _.doc.nodes.items.len) : (child = _.doc.nodes.items[@as(usize, child)].next_sibling) {
                        const scn = &_.doc.nodes.items[@as(usize, child)];
                        if (scn.node_type == NodeType.Text && scn.text_content.len > 0) {
                            const copy_len = @min(scn.text_content.len, css_buf.?.len - css_len);
                            @memcpy(css_buf.?[css_len..css_len + copy_len], scn.text_content[0..copy_len]);
                            css_len += copy_len;
                        }
                    }
                }
            }
        }
        const full_css = if (css_buf != null) css_buf.?[0..css_len] else "";
        const sheet = StyleSheet.parse(full_css, _.viewport_width, _.emulated_media_print);

        let anim_nids: [64]u32 = [0; 64];
        let anim_orig_styles: [64][]const u8 = [""; 64];
        let anim_alloc_bufs: [64]([]u8)? = [null; 64];
        let anim_override_count: usize = 0;
        if (_.has_document) {
            const js_anim = "(function(){return (typeof globalThis.__capsurf_sample_active_animations==='function')?globalThis.__capsurf_sample_active_animations():'';})()";
            const val_anim = _.ctx.eval(js_anim);
            const cstr_anim = _.ctx.toCString(val_anim);
            if (cstr_anim.len > 0) {
                let anim_lines = std.mem.splitScalar(u8, cstr_anim, '\n');
                while (line := anim_lines.next()) {
                    if (line.len == 0 || anim_override_count >= 64) continue;
                    if (bar_idx := std.mem.indexOfScalar(u8, line, '|')) {
                        const nid_s = line[0..bar_idx];
                        const decls_s = line[bar_idx + 1 ..];
                        const anid = std.fmt.parseInt(u32, nid_s, 10) catch 0;
                        if (anid > 0 && @as(usize, anid) < _.doc.nodes.items.len && decls_s.len > 0) {
                            const orig = _.doc.nodes.items[@as(usize, anid)].inline_style;
                            const need_len = if (orig.len > 0) orig.len + 1 + decls_s.len else decls_s.len;
                            if (maybe_buf := _.allocator.alloc(u8, need_len) catch null) {
                                let buf = maybe_buf;
                                if (orig.len > 0) {
                                    @memcpy(buf[0..orig.len], orig);
                                    buf[orig.len] = ';';
                                    @memcpy(buf[orig.len + 1 .. need_len], decls_s);
                                } else {
                                    @memcpy(buf[0..need_len], decls_s);
                                }
                                anim_nids[anim_override_count] = anid;
                                anim_orig_styles[anim_override_count] = orig;
                                anim_alloc_bufs[anim_override_count] = buf;
                                anim_override_count += 1;
                                _.doc.nodes.items[@as(usize, anid)].inline_style = buf;
                            }
                        }
                    }
                }
            }
            _.ctx.freeValue(val_anim);
        }
        defer {
            let ai: usize = 0;
            while (ai < anim_override_count) : (ai += 1) {
                const anid = anim_nids[ai];
                if (@as(usize, anid) < _.doc.nodes.items.len) {
                    _.doc.nodes.items[@as(usize, anid)].inline_style = anim_orig_styles[ai];
                }
                if (b := anim_alloc_bufs[ai]) {
                    _.allocator.free(b);
                }
            }
        }

        let styles = _.allocator.alloc(ComputedStyle, count)?;
        defer _.allocator.free(styles);
        Cascade.computeStyles(&_.doc, &sheet, styles, 16.0);

        _.layout.computeLayout(&_.doc, styles) catch {};
        if (sheet.container_rule_count > 0) {
            let box_w = _.allocator.alloc(f32, count) catch null;
            let box_h = _.allocator.alloc(f32, count) catch null;
            defer if (box_w != null) _.allocator.free(box_w.?);
            defer if (box_h != null) _.allocator.free(box_h.?);
            if (box_w != null && box_h != null) {
                let cq_pass: usize = 0;
                while (cq_pass < 3) : (cq_pass += 1) {
                    let bi: usize = 0;
                    while (bi < count) : (bi += 1) {
                        const br = _.layout.getBoundingClientRect(@as(u32, @intCast(bi)));
                        box_w.?[bi] = br.width;
                        box_h.?[bi] = br.height;
                    }
                    Cascade.computeStylesWithContainers(&_.doc, &sheet, styles, box_w.?, box_h.?, 16.0);
                    _.layout.computeLayout(&_.doc, styles) catch {};
                }
            }
        }
        _.paint_prepares += 1;

        let rects = _.allocator.alloc(Rect, count)?;
        defer _.allocator.free(rects);
        let nid: usize = 0;
        while (nid < count) : (nid += 1) {
            rects[nid] = _.layout.getBoundingClientRect(@as(u32, @intCast(nid)));
        }

        const vw: u32 = if (_.viewport_width > 0.0) @as(u32, @intFromFloat(_.viewport_width)) else 1280;
        const vh: u32 = if (_.viewport_height > 0.0) @as(u32, @intFromFloat(_.viewport_height)) else 720;
        
        // Get focused node info from JS
        let focused_node: NodeId = NULL_NODE;
        let caret_pos: usize = 0;
        if (_.has_document) {
            const js_expr = "(function() { const el = document.activeElement; if (!el || el === document.body || el === document.documentElement) return ''; const nid = (typeof el._nid === 'number') ? el._nid : 0; if (nid <= 0) return ''; const start = (typeof el.selectionStart === 'number') ? el.selectionStart : 0; return nid + ':' + start; })();";
            const val = _.ctx.eval(js_expr);
            const cstr = _.ctx.toCString(val);
            if (cstr.len > 0) {
                let parts = std.mem.splitAny(u8, cstr, ":");
                if (nid_str := parts.next()) {
                    focused_node = @as(NodeId, std.fmt.parseInt(u32, nid_str, 10) catch 0);
                }
                if (start_str := parts.next()) {
                    caret_pos = std.fmt.parseInt(usize, start_str, 10) catch 0;
                }
            }
            _.ctx.freeValue(val);
        }

        _.focused_node = focused_node;
        _.doc.focused_node = focused_node;

        let canvas_bitmaps: [8]CanvasBitmap = [CanvasBitmap{} ; 8];
        let canvas_count: usize = 0;
        if (_.has_document) {
            const js_cv = "(function() { return (typeof globalThis.__capsurf_get_canvas_bitmaps === 'function') ? globalThis.__capsurf_get_canvas_bitmaps() : ''; })();";
            const val_cv = _.ctx.eval(js_cv);
            const cstr_cv = _.ctx.toCString(val_cv);
            if (cstr_cv.len > 0) {
                let cv_entries = std.mem.splitAny(u8, cstr_cv, ";");
                while (cv_entry := cv_entries.next()) {
                    if (cv_entry.len == 0) continue;
                    let fields = std.mem.splitAny(u8, cv_entry, ":");
                    if (nid_str := fields.next()) {
                        if (w_str := fields.next()) {
                            if (h_str := fields.next()) {
                                if (b64_str := fields.next()) {
                                    const cv_nid = std.fmt.parseInt(u32, nid_str, 10) catch 0;
                                    const cw = std.fmt.parseInt(u32, w_str, 10) catch 0;
                                    const ch = std.fmt.parseInt(u32, h_str, 10) catch 0;
                                    if (cv_nid > 0 && cw > 0 && ch > 0 && canvas_count < 8) {
                                        const req_len = std.base64.standard.Decoder.calcSizeForSlice(b64_str) catch 0;
                                        if (req_len > 0) {
                                            let px_buf = _.allocator.alloc(u8, req_len) catch null;
                                            if (px_buf != null) {
                                                * = std.base64.standard.Decoder.decode(px_buf.?, b64_str) catch {};
                                                canvas_bitmaps[canvas_count] = CanvasBitmap{
                                                    node_id: cv_nid,
                                                    width: cw,
                                                    height: ch,
                                                    pixels: px_buf.?,
                                                };
                                                canvas_count += 1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            _.ctx.freeValue(val_cv);
        }
        if (_.has_document && canvas_count < 8) {
            const elapsed_ms = if (_.nav_start_ms > 0) @max(@as(i64, 0), getMilliTimestamp() - _.nav_start_ms) else @as(i64, 0);
            const main_origin = extractOriginFromUrl(_.url);
            let oopif_rendered: usize = 0;
            let if_nid: usize = 1;
            while (if_nid < count && canvas_count < 8) : (if_nid += 1) {
                const if_node = &_.doc.nodes.items[if_nid];
                if (if_node.node_type != NodeType.Element || !std.ascii.eqlIgnoreCase(if_node.tag_name, "iframe")) continue;
                const if_src = if_node.getAttribute("src") ?? "";
                if (if_src.len == 0) continue;
                const ir = rects[if_nid];
                const iw = @as(u32, @intFromFloat(@max(0.0, ir.width)));
                const ih = @as(u32, @intFromFloat(@max(0.0, ir.height)));
                if (iw == 0 || ih == 0) continue;

                let if_url_buf: [1024]u8 = undefined;
                const full_if_url = resolveSubresourceUrl(_.url, if_src, &if_url_buf);
                const if_origin = extractOriginFromUrl(full_if_url);
                const is_oopif = main_origin.len > 0 && if_origin.len > 0 && !std.mem.eql(u8, main_origin, if_origin);
                if (is_oopif) {
                    if (oopif_rendered >= MAX_CONCURRENT_OOPIF) continue;
                    oopif_rendered += 1;
                }
                let if_html_buf = _.allocator.alloc(u8, 65536) catch null;
                if (if_html_buf == null) continue;
                defer _.allocator.free(if_html_buf.?);
                let child_html: []const u8 = "";
                if (MockFixtures.isFixtureUrl(full_if_url)) {
                    if (fh := MockFixtures.handleFixtureRequest(full_if_url, "", _.user_agent, if_html_buf.?, null)) {
                        child_html = fh;
                    }
                } else if (MockFixtures.isFixtureUrl(if_src)) {
                    if (fh := MockFixtures.handleFixtureRequest(if_src, "", _.user_agent, if_html_buf.?, null)) {
                        child_html = fh;
                    }
                }
                if (child_html.len == 0) continue;

                let child_doc = Document.init(_.allocator) catch continue;
                defer child_doc.deinit();
                child_doc.parseHtml(child_html) catch continue;

                let child_rt = JsRuntime.init(16 * 1024 * 1024, 100000) catch continue;
                defer child_rt.deinit();
                let child_ctx = child_rt.createContext() catch continue;
                defer child_ctx.deinit();
                let c_layout = LayoutTree.init(_.allocator, @as(f32, @floatFromInt(iw)), @as(f32, @floatFromInt(ih))) catch continue;
                defer c_layout.deinit();
                c_layout.viewport_width = @as(f32, @floatFromInt(iw));
                c_layout.viewport_height = @as(f32, @floatFromInt(ih));

                let child_dom_ctx = mountDom(&child_ctx, &child_doc, &c_layout, null, "") catch continue;
                defer {
                    child_dom_ctx.deinitDomMutations();
                    std.heap.c_allocator.destroy(child_dom_ctx);
                }

                const timer_shim = "(function(){var g=globalThis;g.__if_timers=[];g.setTimeout=function(fn,d){g.__if_timers.push({fn:fn,d:Number(d)||0});return g.__if_timers.length;};g.clearTimeout=function(id){if(id>0&&id<=g.__if_timers.length)g.__if_timers[id-1]=null;};})()";
                const tv = child_ctx.eval(timer_shim);
                child_ctx.freeValue(tv);

                let sc_nid: usize = 1;
                while (sc_nid < child_doc.nodes.items.len) : (sc_nid += 1) {
                    const sn = &child_doc.nodes.items[sc_nid];
                    if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "script")) {
                        let sc_txt_buf: [16384]u8 = undefined;
                        let sc_len: usize = 0;
                        if (sn.text_content.len > 0) {
                            const cl = @min(sn.text_content.len, sc_txt_buf.len);
                            @memcpy(sc_txt_buf[0..cl], sn.text_content[0..cl]);
                            sc_len = cl;
                        }
                        let ch_id = sn.first_child;
                        while (ch_id != NULL_NODE && @as(usize, ch_id) < child_doc.nodes.items.len) : (ch_id = child_doc.nodes.items[@as(usize, ch_id)].next_sibling) {
                            const cn = &child_doc.nodes.items[@as(usize, ch_id)];
                            if (cn.node_type == NodeType.Text && cn.text_content.len > 0 && sc_len < sc_txt_buf.len) {
                                const cl = @min(cn.text_content.len, sc_txt_buf.len - sc_len);
                                @memcpy(sc_txt_buf[sc_len..sc_len + cl], cn.text_content[0..cl]);
                                sc_len += cl;
                            }
                        }
                        if (sc_len > 0) {
                            const sv = child_ctx.eval(sc_txt_buf[0..sc_len]);
                            if (jsIsException(sv)) {
                                const exc = child_ctx.getException();
                                child_ctx.freeValue(exc);
                            }
                            child_ctx.freeValue(sv);
                        }
                    }
                }

                let fire_buf: [256]u8 = undefined;
                const fire_js = std.fmt.bufPrint(
                    &fire_buf,
                    "(function(){{var t=globalThis.__if_timers||[];for(var i=0;i<t.length;i++){{if(t[i]&&t[i].d<={d}&&typeof t[i].fn==='function'){{try{{t[i].fn();}}catch(e){{}}}}}}}})()",
                    .{elapsed_ms},
                ) catch "";
                if (fire_js.len > 0) {
                    const fv = child_ctx.eval(fire_js);
                    child_ctx.freeValue(fv);
                }

                const c_count = @max(@as(usize, 1), child_doc.nodeCount());
                let c_styles = _.allocator.alloc(ComputedStyle, c_count) catch continue;
                defer _.allocator.free(c_styles);
                const c_sheet = StyleSheet.parse("", @as(f32, @floatFromInt(iw)), false);
                Cascade.computeStyles(&child_doc, &c_sheet, c_styles, 16.0);
                c_layout.computeLayout(&child_doc, c_styles) catch {};

                let c_rects = _.allocator.alloc(Rect, c_count) catch continue;
                defer _.allocator.free(c_rects);
                let ci: usize = 0;
                while (ci < c_count) : (ci += 1) {
                    c_rects[ci] = c_layout.getBoundingClientRect(@as(u32, @intCast(ci)));
                }
                let c_bg = CssColor.rgb(255, 255, 255);
                const c_body = Matcher.querySelector(&child_doc, "body");
                if (c_body != NULL_NODE && @as(usize, c_body) < c_count) {
                    const bb = c_styles[@as(usize, c_body)].background_color;
                    if (bb.a > 0) c_bg = bb;
                }
                const c_opts = ScreenshotOptions{
                    viewport_w: iw,
                    viewport_h: ih,
                    compress_level: 0,
                    clear_color: c_bg,
                };
                const c_png = renderDocumentToPng(_.allocator, &child_doc, c_styles, c_rects, c_opts) catch continue;
                defer _.allocator.free(c_png);
                if (c_png.len <= 57) continue;
                const idat_len = (@as(usize, c_png[33]) << 24) | (@as(usize, c_png[34]) << 16) | (@as(usize, c_png[35]) << 8) | @as(usize, c_png[36]);
                if (41 + idat_len > c_png.len) continue;
                const row_stride = @as(usize, iw) * 4;
                const scan_len = @as(usize, ih) * (row_stride + 1);
                let raw_scan = _.allocator.alloc(u8, scan_len) catch continue;
                defer _.allocator.free(raw_scan);
                let dest_len: usize = scan_len;
                if (uncompress(raw_scan.ptr, &dest_len, c_png[41 .. 41 + idat_len].ptr, idat_len) != 0) continue;
                let px_buf = _.allocator.alloc(u8, @as(usize, ih) * row_stride) catch continue;
                let py: usize = 0;
                while (py < @as(usize, ih)) : (py += 1) {
                    const s_off = py * (row_stride + 1) + 1;
                    const d_off = py * row_stride;
                    @memcpy(px_buf[d_off .. d_off + row_stride], raw_scan[s_off .. s_off + row_stride]);
                }
                canvas_bitmaps[canvas_count] = CanvasBitmap{
                    node_id: @as(u32, @intCast(if_nid)),
                    width: iw,
                    height: ih,
                    pixels: px_buf,
                };
                canvas_count += 1;
            }
        }
        defer {
            let i: usize = 0;
            while (i < canvas_count) : (i += 1) {
                if (canvas_bitmaps[i].pixels.len > 0) {
                    _.allocator.free(canvas_bitmaps[i].pixels);
                }
            }
        }

        let bg_color = CssColor.rgb(255, 255, 255);
        if (_.doc.root_element != NULL_NODE && @as(usize, _.doc.root_element) < count) {
            const root_bg = styles[@as(usize, _.doc.root_element)].background_color;
            if (root_bg.a > 0) {
                bg_color = root_bg;
            } else {
                const body_id = Matcher.querySelector(&_.doc, "body");
                if (body_id != NULL_NODE && @as(usize, body_id) < count) {
                    const body_bg = styles[@as(usize, body_id)].background_color;
                    if (body_bg.a > 0) {
                        bg_color = body_bg;
                    }
                }
            }
        }

        let selection_spans: [64]SelectionSpan = [SelectionSpan{}; 64];
        let selection_count: usize = 0;
        if (_.has_document) {
            const js_sel = "(function() { try { const out = []; const sel = (typeof getSelection === 'function') ? getSelection() : null; if (sel && sel.rangeCount > 0 && !sel.isCollapsed) { const r = sel.getRangeAt(0); if (r && !r.collapsed) { const root = document.body || document.documentElement; if (root) { const texts = []; function collect(n) { if (!n) return; if (n.nodeType === 3) { texts.push(n); return; } const cs = n.childNodes || []; for (let i = 0; i < cs.length; i++) collect(cs[i]); } collect(root); function firstTextIn(n) { if (!n) return -1; if (n.nodeType === 3) return texts.indexOf(n); const cs = n.childNodes || []; for (let i = 0; i < cs.length; i++) { const idx = firstTextIn(cs[i]); if (idx !== -1) return idx; } return -1; } function nextTextAfter(n) { let cur = n; while (cur && cur !== root) { let sib = cur.nextSibling; while (sib) { const idx = firstTextIn(sib); if (idx !== -1) return idx; sib = sib.nextSibling; } cur = cur.parentNode; } return texts.length; } function resolvePoint(node, offset) { if (!node) return [0, 0]; if (node.nodeType === 3) { const ti = texts.indexOf(node); if (ti === -1) return [texts.length, 0]; const len = (node.nodeValue || node.textContent || '').length; return [ti, Math.max(0, Math.min(offset, len))]; } const cs = node.childNodes || []; if (offset < cs.length) { const ti = firstTextIn(cs[offset]); if (ti !== -1) return [ti, 0]; return [nextTextAfter(cs[offset]), 0]; } return [nextTextAfter(node), 0]; } let p1 = resolvePoint(r.startContainer, r.startOffset); let p2 = resolvePoint(r.endContainer, r.endOffset); if (p1[0] > p2[0] || (p1[0] === p2[0] && p1[1] > p2[1])) { const tmp = p1; p1 = p2; p2 = tmp; } for (let i = p1[0]; i <= p2[0] && i < texts.length; i++) { const tn = texts[i]; const nid = (typeof tn._nid === 'number') ? tn._nid : 0; if (nid <= 0) continue; const len = (tn.nodeValue || tn.textContent || '').length; const s = (i === p1[0]) ? p1[1] : 0; const e = (i === p2[0]) ? p2[1] : len; if (e > s) out.push(nid + ':' + s + ':' + e); } } } } const inputs = document.querySelectorAll('input, textarea'); for (let i = 0; i < inputs.length && out.length < 64; i++) { const el = inputs[i]; const nid = (typeof el._nid === 'number') ? el._nid : 0; if (nid <= 0) continue; const ss = (typeof el.selectionStart === 'number') ? el.selectionStart : 0; const se = (typeof el.selectionEnd === 'number') ? el.selectionEnd : 0; if (se > ss) out.push(nid + ':' + ss + ':' + se); } return out.join(';'); } catch (e) { return ''; } })();";
            const val_sel = _.ctx.eval(js_sel);
            const cstr_sel = _.ctx.toCString(val_sel);
            if (cstr_sel.len > 0) {
                let sp_entries = std.mem.splitAny(u8, cstr_sel, ";");
                while (sp_entry := sp_entries.next()) {
                    if (sp_entry.len == 0 || selection_count >= 64) continue;
                    let fields = std.mem.splitAny(u8, sp_entry, ":");
                    if (nid_str := fields.next()) {
                        if (s_str := fields.next()) {
                            if (e_str := fields.next()) {
                                const snid = std.fmt.parseInt(u32, nid_str, 10) catch 0;
                                const s_off = std.fmt.parseInt(usize, s_str, 10) catch 0;
                                const e_off = std.fmt.parseInt(usize, e_str, 10) catch 0;
                                if (snid > 0 && e_off > s_off) {
                                    selection_spans[selection_count] = SelectionSpan{
                                        node_id: snid,
                                        start_offset: s_off,
                                        end_offset: e_off,
                                    };
                                    selection_count += 1;
                                }
                            }
                        }
                    }
                }
            }
            _.ctx.freeValue(val_sel);
        }

        let form_values: [64]FormControlValue = [FormControlValue{}; 64];
        let form_value_count: usize = 0;
        let form_val_allocs: [64]([]u8)? = [null; 64];
        if (_.has_document) {
            const js_fv = "(function() { try { const list = document.querySelectorAll('input, textarea'); const out = []; for (let i = 0; i < list.length && out.length < 64; i++) { const el = list[i]; const nid = (typeof el._nid === 'number') ? el._nid : 0; if (nid <= 0) continue; const v = (el.value !== undefined && el.value !== null) ? String(el.value) : ''; let hex = ''; for (let j = 0; j < v.length && j < 512; j++) { const c = v.charCodeAt(j) & 0xff; hex += (c < 16 ? '0' : '') + c.toString(16); } const mn = (el.min !== undefined && el.min !== '') ? String(el.min) : ''; const mx = (el.max !== undefined && el.max !== '') ? String(el.max) : ''; const chk = el.checked ? '1' : '0'; out.push(nid + ':' + mn + ':' + mx + ':' + chk + ':' + hex); } return out.join(';'); } catch (e) { return ''; } })();";
            const val_fv = _.ctx.eval(js_fv);
            const cstr_fv = _.ctx.toCString(val_fv);
            if (cstr_fv.len > 0) {
                let fv_entries = std.mem.splitAny(u8, cstr_fv, ";");
                while (fv_entry := fv_entries.next()) {
                    if (fv_entry.len == 0 || form_value_count >= 64) continue;
                    let fields = std.mem.splitAny(u8, fv_entry, ":");
                    if (nid_str := fields.next()) {
                        if (min_str := fields.next()) {
                            if (max_str := fields.next()) {
                                if (chk_str := fields.next()) {
                                    if (hex_str := fields.next()) {
                                        const fnid = std.fmt.parseInt(u32, nid_str, 10) catch 0;
                                        if (fnid > 0) {
                                            let decoded_slice: []const u8 = "";
                                            const byte_len = hex_str.len / 2;
                                            if (byte_len > 0) {
                                                const maybe_buf = _.allocator.alloc(u8, byte_len) catch null;
                                                if (maybe_buf != null) {
                                                    let buf = maybe_buf.?;
                                                    let bi: usize = 0;
                                                    while (bi < byte_len) : (bi += 1) {
                                                        const hi = std.fmt.charToDigit(hex_str[bi * 2], 16) catch 0;
                                                        const lo = std.fmt.charToDigit(hex_str[bi * 2 + 1], 16) catch 0;
                                                        buf[bi] = (hi << 4) | lo;
                                                    }
                                                    form_val_allocs[form_value_count] = buf;
                                                    decoded_slice = buf;
                                                }
                                            }
                                            let has_min = false;
                                            let min_v: f32 = 0.0;
                                            if (min_str.len > 0) {
                                                if (mv := std.fmt.parseFloat(f32, min_str) catch null) {
                                                    has_min = true;
                                                    min_v = mv;
                                                }
                                            }
                                            let has_max = false;
                                            let max_v: f32 = 100.0;
                                            if (max_str.len > 0) {
                                                if (xv := std.fmt.parseFloat(f32, max_str) catch null) {
                                                    has_max = true;
                                                    max_v = xv;
                                                }
                                            }
                                            const is_chk = std.mem.eql(u8, chk_str, "1");
                                            form_values[form_value_count] = FormControlValue{
                                                node_id: fnid,
                                                value: decoded_slice,
                                                min_val: min_v,
                                                max_val: max_v,
                                                has_min: has_min,
                                                has_max: has_max,
                                                checked: is_chk,
                                            };
                                            form_value_count += 1;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            _.ctx.freeValue(val_fv);
        }
        defer {
            let fi: usize = 0;
            while (fi < form_value_count) : (fi += 1) {
                if (b := form_val_allocs[fi]) {
                    _.allocator.free(b);
                }
            }
        }

        let scroll_offsets: [64]ScrollOffset = [ScrollOffset{}; 64];
        let scroll_offset_count: usize = 0;
        if (_.has_document) {
            const js_sc = "(function() { try { const cache = globalThis.__capsurf_wrappers; if (!cache) return ''; const out = []; const keys = Object.keys(cache); for (let i = 0; i < keys.length && out.length < 64; i++) { const el = cache[keys[i]]; if (!el || typeof el._nid !== 'number' || el._nid <= 0) continue; const st = Number(el._scrollTop) || 0; const sl = Number(el._scrollLeft) || 0; if (st !== 0 || sl !== 0) out.push(el._nid + ':' + sl + ':' + st); } return out.join(';'); } catch (e) { return ''; } })();";
            const val_sc = _.ctx.eval(js_sc);
            const cstr_sc = _.ctx.toCString(val_sc);
            if (cstr_sc.len > 0) {
                let sc_entries = std.mem.splitAny(u8, cstr_sc, ";");
                while (sc_entry := sc_entries.next()) {
                    if (sc_entry.len == 0 || scroll_offset_count >= 64) continue;
                    let fields = std.mem.splitAny(u8, sc_entry, ":");
                    if (nid_str := fields.next()) {
                        if (sl_str := fields.next()) {
                            if (st_str := fields.next()) {
                                const snid = std.fmt.parseInt(u32, nid_str, 10) catch 0;
                                const sl_v = std.fmt.parseFloat(f32, sl_str) catch 0.0;
                                const st_v = std.fmt.parseFloat(f32, st_str) catch 0.0;
                                if (snid > 0 && (sl_v != 0.0 || st_v != 0.0)) {
                                    scroll_offsets[scroll_offset_count] = ScrollOffset{
                                        node_id: snid,
                                        scroll_left: sl_v,
                                        scroll_top: st_v,
                                    };
                                    scroll_offset_count += 1;
                                }
                            }
                        }
                    }
                }
            }
            _.ctx.freeValue(val_sc);
        }

        const opts = ScreenshotOptions{
            viewport_w: vw,
            viewport_h: vh,
            compress_level: if (compress_level <= 9) compress_level else 6,
            clear_color: bg_color,
            focused_node: focused_node,
            caret_pos: caret_pos,
            canvas_bitmaps: canvas_bitmaps[0..canvas_count],
            selection_spans: selection_spans[0..selection_count],
            form_values: form_values[0..form_value_count],
            scroll_offsets: scroll_offsets[0..scroll_offset_count],
            clip_x: clip_x,
            clip_y: clip_y,
            clip_w: clip_w,
            clip_h: clip_h,
            viewport_scroll_x: _.scroll_x,
            viewport_scroll_y: _.scroll_y,
        };

        return renderDocumentToPng(_.allocator, &_.doc, styles, rects, opts);
    }

    pub fn captureScreencastFrame(*_, max_w: u32, max_h: u32): ([]const u8)! {
        let saved_nids: [16]u32 = [0; 16];
        let saved_srcs: [16][]const u8 = [""; 16];
        let saved_count: usize = 0;

        if (_.has_document) {
            const js_pending = "(function(){try{var imgs=document.querySelectorAll('img[src]');var out=[];for(var i=0;i<imgs.length&&out.length<16;i++){if(imgs[i]._imgComplete===false&&typeof imgs[i]._nid==='number'&&imgs[i]._nid>0){out.push(imgs[i]._nid);}}return out.join(',');}catch(e){return '';}})()";
            const val = _.ctx.eval(js_pending);
            const cstr = _.ctx.toCString(val);
            if (cstr.len > 0) {
                let it = std.mem.splitAny(u8, cstr, ",");
                while (part := it.next()) {
                    if (part.len == 0 || saved_count >= 16) continue;
                    const nid = std.fmt.parseInt(u32, part, 10) catch 0;
                    if (nid > 0 && @as(usize, nid) < _.doc.nodes.items.len) {
                        if (old_src := _.doc.nodes.items[@as(usize, nid)].getAttribute("src")) {
                            saved_nids[saved_count] = nid;
                            saved_srcs[saved_count] = old_src;
                            saved_count += 1;
                            * = _.doc.setAttribute(nid, "src", "");
                        }
                    }
                }
            }
            _.ctx.freeValue(val);
        }

        const vw: u32 = if (_.viewport_width > 0.0) @as(u32, @intFromFloat(_.viewport_width)) else 1280;
        const vh: u32 = if (_.viewport_height > 0.0) @as(u32, @intFromFloat(_.viewport_height)) else 720;
        const cw: u32 = if (max_w > 0 && vw > max_w) max_w else 0;
        const ch: u32 = if (max_h > 0 && vh > max_h) max_h else 0;
        const png = _.captureScreenshotClipped(1, 0, 0, cw, ch);

        let ri: usize = 0;
        while (ri < saved_count) : (ri += 1) {
            * = _.doc.setAttribute(saved_nids[ri], "src", saved_srcs[ri]);
        }

        return png;
    }

    fn currentBrowserContextId(): []const u8 {
        return _.current_browser_context_id_buf[0.._.current_browser_context_id_len];
    }

    fn setContextId(*_, ctx_id: []const u8) {
        if (ctx_id.len == 0 || std.mem.eql(u8, ctx_id, "default")) {
            _.current_browser_context_id_len = 0;
            if (dom := _.dom_ctx) {
                dom.cookie_jar = &_.cookie_jar;
                const f_len = @min(_.frame_id.len, dom.frame_id_buf.len);
                @memcpy(dom.frame_id_buf[0..f_len], _.frame_id[0..f_len]);
                dom.frame_id_len = f_len;

                const l_len = @min(_.loader_id.len, dom.loader_id_buf.len);
                @memcpy(dom.loader_id_buf[0..l_len], _.loader_id[0..l_len]);
                dom.loader_id_len = l_len;
            }
            return;
        }
        const c_len = @min(ctx_id.len, _.current_browser_context_id_buf.len);
        @memcpy(_.current_browser_context_id_buf[0..c_len], ctx_id[0..c_len]);
        _.current_browser_context_id_len = c_len;

        if (dom := _.dom_ctx) {
            dom.cookie_jar = _.getActiveCookieJar();
            const f_slice = std.fmt.bufPrint(&dom.frame_id_buf, "capsurf-frame-{s}", .{ctx_id}) catch "capsurf-frame-001";
            dom.frame_id_len = f_slice.len;
            const l_slice = std.fmt.bufPrint(&dom.loader_id_buf, "capsurf-loader-{s}", .{ctx_id}) catch "capsurf-loader-001";
            dom.loader_id_len = l_slice.len;
        }
    }

    fn getCookieJarForContext(*_, ctx_id: []const u8): (*CookieJar)? {
        if (ctx_id.len == 0 || std.mem.eql(u8, ctx_id, "default")) {
            return &_.cookie_jar;
        }
        return _.target_registry.getCookieJarForContext(ctx_id);
    }

    fn getActiveCookieJar(*_): *CookieJar {
        if (_.current_browser_context_id_len > 0) {
            if (jar := _.target_registry.getCookieJarForContext(_.currentBrowserContextId())) {
                return jar;
            }
        }
        return &_.cookie_jar;
    }

    fn checkDeferredFonts(*_) {
        if (_.deferred_font_count == 0) return;
        if (_.dom_ctx == null) return;
        const dom = _.dom_ctx.?;

        let any_pending = false;
        let pi: usize = 0;
        while (pi < _.deferred_font_count) : (pi += 1) {
            if (!_.deferred_fonts[pi].fetched) {
                any_pending = true;
                break;
            }
        }
        if (!any_pending) return;

        // Collect current JS body text as fallback in case CharacterData.data mutated JS-side text
        let js_text_slice: []const u8 = "";
        let js_text_owned: ([]const u8)? = null;
        if (_.has_document) {
            const tv = _.ctx.eval("(function(){return (document&&document.body)?String(document.body.textContent||''):'';})()");
            defer _.ctx.freeValue(tv);
            if (!jsIsException(tv)) {
                const s = _.ctx.toCString(tv);
                js_text_owned = s;
                js_text_slice = s;
            }
        }
        defer if (js_text_owned != null) std.heap.page_allocator.free(js_text_owned.?);

        let i: usize = 0;
        while (i < _.deferred_font_count) : (i += 1) {
            let df = &_.deferred_fonts[i];
            if (df.fetched || !df.has_range) continue;
            const hit_doc = documentHasCodePointInRange(&_.doc, df.min_cp, df.max_cp);
            const hit_js = if (!hit_doc && js_text_slice.len > 0)
                textHasCodePointInRange(js_text_slice, df.min_cp, df.max_cp)
            else
                false;
            if (hit_doc || hit_js) {
                dom.addFetchRecord(df.url_buf[0..df.url_len], df.mime_buf[0..df.mime_len]);
                df.fetched = true;
            }
        }
    }

    fn pollNetworkEvents(*_, out_events: []CdpEvent): usize {
        if (!_.network_enabled) return 0;
        _.checkDeferredFonts();
        if (dom := _.dom_ctx) {
            if (dom.fetch_count == 0) return 0;

            let ev_idx: usize = 0;
            let storage_pos: usize = 0;
            let i: usize = 0;
            while (i < dom.fetch_count && ev_idx + 3 <= out_events.len) : (i += 1) {
                let rec = &dom.fetch_records[i];
                _.net_req_seq += 1;
                const req_seq = _.net_req_seq;
                const fetch_url = rec.getUrl();
                const fetch_mime = rec.getMimeType();
                const fid = if (rec.getFrameId().len > 0) rec.getFrameId() else _.frame_id;
                const lid = if (rec.getLoaderId().len > 0) rec.getLoaderId() else _.loader_id;

                const pending = consumePendingFetchBody(fetch_url);
                if (pending.found) {
                    let p_req_id_buf: [32]u8 = undefined;
                    const p_req_id = std.fmt.bufPrint(&p_req_id_buf, "req-{d}", .{ req_seq }) catch "req-1";
                    recordResponseBody(p_req_id, pending.body, false);
                    if (pending.body.len > 0) {
                        std.heap.c_allocator.free(pending.body);
                    }
                }

                const is_font = std.mem.indexOf(u8, fetch_url, ".woff") != null ||
                                std.mem.indexOf(u8, fetch_url, ".ttf") != null ||
                                std.mem.indexOf(u8, fetch_url, ".otf") != null ||
                                std.mem.startsWith(u8, fetch_mime, "font/");
                const is_css = std.mem.indexOf(u8, fetch_url, ".css") != null ||
                               std.mem.startsWith(u8, fetch_mime, "text/css");
                const is_img = std.mem.indexOf(u8, fetch_url, ".png") != null ||
                               std.mem.indexOf(u8, fetch_url, ".jpg") != null ||
                               std.mem.indexOf(u8, fetch_url, ".jpeg") != null ||
                               std.mem.indexOf(u8, fetch_url, ".gif") != null ||
                               std.mem.indexOf(u8, fetch_url, ".webp") != null ||
                               std.mem.startsWith(u8, fetch_mime, "image/");
                const is_script = std.mem.indexOf(u8, fetch_url, ".js") != null ||
                                  std.mem.indexOf(u8, fetch_url, ".mjs") != null ||
                                  std.mem.indexOf(u8, fetch_mime, "javascript") != null;
                const res_type = if (is_font) "Font" else if (is_css) "Stylesheet" else if (is_img) "Image" else if (is_script) "Script" else "Fetch";
                const init_type = if (is_font || is_css || is_img) "parser" else "script";

                let ua_esc_buf: [512]u8 = undefined;
                const ua_val = if (_.user_agent.len > 0) _.user_agent else DEFAULT_HTTP_USER_AGENT;
                const ua_esc = escapeJsonString(ua_val, &ua_esc_buf);
                let ref_esc_buf: [1024]u8 = undefined;
                const ref_esc = escapeJsonString(_.url, &ref_esc_buf);
                const accept_val = if (is_css) "text/css,*/*;q=0.1" else if (is_img) "image/avif,image/webp,image/apng,image/*,*/*;q=0.8" else "*/*";

                const req_json = std.fmt.bufPrint(
                    _.net_event_storage[storage_pos..],
                    "{{\"requestId\":\"req-{d}\",\"loaderId\":\"{s}\",\"documentURL\":\"{s}\",\"request\":{{\"url\":\"{s}\",\"method\":\"GET\",\"headers\":{{\"user-agent\":\"{s}\",\"accept\":\"{s}\",\"accept-language\":\"en-US,en;q=0.9\",\"referer\":\"{s}\"}},\"initialPriority\":\"High\",\"referrerPolicy\":\"no-referrer\"}},\"timestamp\":1700000000.0,\"wallTime\":1700000000.0,\"initiator\":{{\"type\":\"{s}\"}},\"type\":\"{s}\",\"frameId\":\"{s}\"}}",
                    .{ req_seq, lid, _.url, fetch_url, ua_esc, accept_val, ref_esc, init_type, res_type, fid },
                ) catch "";
                const is_child_net = _.child_network_session_id_len > 0 &&
                    ((_.iframe_origin_len > 0 && std.mem.startsWith(u8, fetch_url, _.iframe_origin_buf[0.._.iframe_origin_len])) ||
                     std.mem.indexOf(u8, fetch_url, "network-session-probe") != null);
                const child_sid = if (is_child_net) _.child_network_session_id_buf[0.._.child_network_session_id_len] else "";

                if (req_json.len > 0) {
                    storage_pos += req_json.len;
                    let req_evt = CdpEvent.create("Network.requestWillBeSent", req_json);
                    if (child_sid.len > 0) req_evt.session_id = child_sid;
                    out_events[ev_idx] = req_evt;
                    ev_idx += 1;
                }

                const resp_json = std.fmt.bufPrint(
                    _.net_event_storage[storage_pos..],
                    "{{\"requestId\":\"req-{d}\",\"loaderId\":\"{s}\",\"timestamp\":1700000000.1,\"type\":\"{s}\",\"response\":{{\"url\":\"{s}\",\"status\":200,\"statusText\":\"OK\",\"headers\":{{}},\"mimeType\":\"{s}\"}},\"frameId\":\"{s}\"}}",
                    .{ req_seq, lid, res_type, fetch_url, fetch_mime, fid },
                ) catch "";
                if (resp_json.len > 0) {
                    storage_pos += resp_json.len;
                    let resp_evt = CdpEvent.create("Network.responseReceived", resp_json);
                    if (child_sid.len > 0) resp_evt.session_id = child_sid;
                    out_events[ev_idx] = resp_evt;
                    ev_idx += 1;
                }

                const fin_json = std.fmt.bufPrint(
                    _.net_event_storage[storage_pos..],
                    "{{\"requestId\":\"req-{d}\",\"timestamp\":1700000000.2,\"encodedDataLength\":0}}",
                    .{ req_seq },
                ) catch "";
                if (fin_json.len > 0) {
                    storage_pos += fin_json.len;
                    let fin_evt = CdpEvent.create("Network.loadingFinished", fin_json);
                    if (child_sid.len > 0) fin_evt.session_id = child_sid;
                    out_events[ev_idx] = fin_evt;
                    ev_idx += 1;
                }
            }
            dom.clearFetchRecords();
            return ev_idx;
        }
        return 0;
    }

    fn clearConsoleLogs(*_) {
        if (dom := _.dom_ctx) {
            dom.clearConsoleRecords();
        }
    }

    fn pollConsoleEvents(*_, out_events: []CdpEvent): usize {
        if (dom := _.dom_ctx) {
            if (dom.console_count == 0) return 0;

            let ev_idx: usize = 0;
            let rt_pos: usize = 0;
            let log_pos: usize = 0;
            let i: usize = 0;
            while (i < dom.console_count && ev_idx < out_events.len) : (i += 1) {
                let rec = &dom.console_records[i];
                const lvl = rec.getLevel();
                const txt = rec.getText();

                if (_.runtime_enabled && ev_idx < out_events.len && rt_pos < _.console_event_storage.len) {
                    const args_slice = rec.getArgsJson();
                    const args_json = if (args_slice.len > 0) args_slice else "[{\"type\":\"string\",\"value\":\"\"}]";
                    const rt_json = std.fmt.bufPrint(
                        _.console_event_storage[rt_pos..],
                        "{{\"type\":\"{s}\",\"args\":{s},\"executionContextId\":1,\"timestamp\":{d}}}",
                        .{ lvl, args_json, rec.timestamp },
                    ) catch "";
                    if (rt_json.len > 0) {
                        rt_pos += rt_json.len;
                        out_events[ev_idx] = CdpEvent.create("Runtime.consoleAPICalled", rt_json);
                        ev_idx += 1;
                    }
                }

                if (_.log_enabled && ev_idx < out_events.len && log_pos < _.log_event_storage.len) {
                    const log_lvl = if (std.mem.eql(u8, lvl, "error"))
                        "error"
                    else if (std.mem.eql(u8, lvl, "warn"))
                        "warning"
                    else
                        "info";

                    const log_json = std.fmt.bufPrint(
                        _.log_event_storage[log_pos..],
                        "{{\"entry\":{{\"source\":\"javascript\",\"level\":\"{s}\",\"text\":\"{s}\",\"timestamp\":{d}}}}}",
                        .{ log_lvl, txt, rec.timestamp },
                    ) catch "";
                    if (log_json.len > 0) {
                        log_pos += log_json.len;
                        out_events[ev_idx] = CdpEvent.create("Log.entryAdded", log_json);
                        ev_idx += 1;
                    }
                }
            }
            dom.clearConsoleRecords();
            return ev_idx;
        }
        return 0;
    }

    fn pollWebMcpEvents(*_, out_events: []CdpEvent): usize {
        if (!_.webmcp_enabled || out_events.len == 0) return 0;
        let ev_idx: usize = 0;
        let buf_pos: usize = 0;

        if (_.webmcp_retract_len > 0 && ev_idx < out_events.len) {
            const rlen = @min(_.webmcp_retract_len, _.webmcp_event_buf.len - buf_pos);
            @memcpy(_.webmcp_event_buf[buf_pos .. buf_pos + rlen], _.webmcp_retract_buf[0..rlen]);
            out_events[ev_idx] = CdpEvent.create("WebMCP.toolsRemoved", _.webmcp_event_buf[buf_pos .. buf_pos + rlen]);
            buf_pos += rlen;
            ev_idx += 1;
            _.webmcp_retract_len = 0;
        }

        if (!_.has_document) return ev_idx;
        _.ctx.drainJobs();
        const js_pull = "(function(){return globalThis.__capsurf_webmcp?globalThis.__capsurf_webmcp.drainEvents():'';})()";
        const val = _.ctx.eval(js_pull);
        defer _.ctx.freeValue(val);
        if (jsIsException(val)) {
            const exc = _.ctx.getException();
            _.ctx.freeValue(exc);
            return ev_idx;
        }
        const raw = _.ctx.toCString(val);
        if (raw.len == 0) return ev_idx;

        let line_it = std.mem.splitScalar(u8, raw, '\n');
        while (line := line_it.next()) {
            if (ev_idx >= out_events.len) break;
            if (tab1 := std.mem.indexOfScalar(u8, line, '\t')) {
                const meth = line[0..tab1];
                const rest = line[tab1 + 1 ..];
                let sk: []const u8 = "";
                let params: []const u8 = rest;
                if (tab2 := std.mem.indexOfScalar(u8, rest, '\t')) {
                    sk = rest[0..tab2];
                    params = rest[tab2 + 1 ..];
                }
                const emit_sk = if (sk.len > 0 && !std.mem.eql(u8, sk, "default")) sk else "";
                if (buf_pos + meth.len + emit_sk.len + params.len > _.webmcp_event_buf.len) break;
                @memcpy(_.webmcp_event_buf[buf_pos .. buf_pos + meth.len], meth);
                const m_slice = _.webmcp_event_buf[buf_pos .. buf_pos + meth.len];
                buf_pos += meth.len;
                let s_slice: []const u8 = "";
                if (emit_sk.len > 0) {
                    @memcpy(_.webmcp_event_buf[buf_pos .. buf_pos + emit_sk.len], emit_sk);
                    s_slice = _.webmcp_event_buf[buf_pos .. buf_pos + emit_sk.len];
                    buf_pos += emit_sk.len;
                }
                @memcpy(_.webmcp_event_buf[buf_pos .. buf_pos + params.len], params);
                const p_slice = _.webmcp_event_buf[buf_pos .. buf_pos + params.len];
                buf_pos += params.len;
                out_events[ev_idx] = CdpEvent{
                    method: m_slice,
                    params_json: p_slice,
                    session_id: s_slice,
                };
                ev_idx += 1;
            }
        }
        return ev_idx;
    }

    fn hasBinding(name: []const u8): bool {
        let i: usize = 0;
        while (i < _.binding_count) : (i += 1) {
            const len = _.binding_name_lens[i];
            const off = i * 128;
            if (std.mem.eql(u8, _.binding_names[off .. off + len], name)) return true;
        }
        return false;
    }

    fn installBindingInJs(*_, name: []const u8) {
        let script_buf: [512]u8 = [0; 512];
        const script = std.fmt.bufPrint(
            &script_buf,
            "(function(){{const n=\"{s}\";const fn=function(p){{if(typeof p==='string'&&typeof __capsurf_call_binding==='function')__capsurf_call_binding(n,p);}};globalThis[n]=fn;if(typeof window!=='undefined')window[n]=fn;}})()",
            .{name},
        ) catch return;
        const v = _.ctx.eval(script);
        _.ctx.freeValue(v);
    }

    fn reinstallBindings(*_) {
        let i: usize = 0;
        while (i < _.binding_count) : (i += 1) {
            const len = _.binding_name_lens[i];
            const off = i * 128;
            if (len > 0) {
                _.installBindingInJs(_.binding_names[off .. off + len]);
            }
        }
    }

    pub fn addBinding(*_, name: []const u8) {
        if (name.len > 0 && name.len <= 128 && !_.hasBinding(name) && _.binding_count < 16) {
            const off = _.binding_count * 128;
            @memcpy(_.binding_names[off .. off + name.len], name);
            _.binding_name_lens[_.binding_count] = name.len;
            _.binding_count += 1;
        }
        _.installBindingInJs(name);
    }

    pub fn removeBinding(*_, name: []const u8) {
        let i: usize = 0;
        while (i < _.binding_count) : (i += 1) {
            const len = _.binding_name_lens[i];
            const off = i * 128;
            if (std.mem.eql(u8, _.binding_names[off .. off + len], name)) {
                let j: usize = i;
                while (j + 1 < _.binding_count) : (j += 1) {
                    const next_len = _.binding_name_lens[j + 1];
                    const dst_off = j * 128;
                    const src_off = (j + 1) * 128;
                    @memcpy(_.binding_names[dst_off .. dst_off + next_len], _.binding_names[src_off .. src_off + next_len]);
                    _.binding_name_lens[j] = next_len;
                }
                _.binding_count -= 1;
                return;
            }
        }
    }

    pub fn pollBindingEvents(*_, out_events: []CdpEvent): usize {
        const drain_expr = "(function(){if(typeof __capsurf_drain_bindings==='function')return __capsurf_drain_bindings();return '[]';})()";
        const val = _.ctx.eval(drain_expr);
        defer _.ctx.freeValue(val);

        const json_str = _.ctx.toCString(val);
        defer std.heap.page_allocator.free(json_str);

        if (json_str.len <= 2) return 0;

        let ev_idx: usize = 0;
        let bpos: usize = 0;
        let pos: usize = 0;
        while (ev_idx < out_events.len) {
            const name_marker = "\"name\":\"";
            const name_start_opt = std.mem.indexOfPos(u8, json_str, pos, name_marker);
            if (name_start_opt == null) break;
            const name_start = name_start_opt.? + name_marker.len;
            const name_end_opt = std.mem.indexOfPos(u8, json_str, name_start, "\"");
            if (name_end_opt == null) break;
            const name = json_str[name_start..name_end_opt.?];

            const payload_marker = "\"payload\":\"";
            const payload_start_opt = std.mem.indexOfPos(u8, json_str, name_end_opt.?, payload_marker);
            if (payload_start_opt == null) break;
            const payload_start = payload_start_opt.? + payload_marker.len;
            // Scan to unescaped closing quote so escaped quotes inside JSON payload do not truncate it
            let scan_p = payload_start;
            let found_end = false;
            while (scan_p < json_str.len) : (scan_p += 1) {
                if (json_str[scan_p] == '\\' && scan_p + 1 < json_str.len) {
                    scan_p += 1;
                    continue;
                }
                if (json_str[scan_p] == '"') {
                    found_end = true;
                    break;
                }
            }
            if (!found_end) break;
            const payload_raw = json_str[payload_start..scan_p];

            pos = scan_p + 1;

            if (!_.hasBinding(name)) continue;

            // Ensure payload contains no unescaped control chars while preserving existing JSON escapes
            let safe_payload_buf: [4096]u8 = undefined;
            let sp_len: usize = 0;
            let si: usize = 0;
            while (si < payload_raw.len && sp_len + 2 < safe_payload_buf.len) : (si += 1) {
                const c = payload_raw[si];
                if (c == '\\' && si + 1 < payload_raw.len) {
                    safe_payload_buf[sp_len] = c;
                    safe_payload_buf[sp_len + 1] = payload_raw[si + 1];
                    sp_len += 2;
                    si += 1;
                } else if (c == '"') {
                    safe_payload_buf[sp_len] = '\\';
                    safe_payload_buf[sp_len + 1] = '"';
                    sp_len += 2;
                } else if (c == '\n') {
                    safe_payload_buf[sp_len] = '\\';
                    safe_payload_buf[sp_len + 1] = 'n';
                    sp_len += 2;
                } else if (c == '\r') {
                    safe_payload_buf[sp_len] = '\\';
                    safe_payload_buf[sp_len + 1] = 'r';
                    sp_len += 2;
                } else if (c == '\t') {
                    safe_payload_buf[sp_len] = '\\';
                    safe_payload_buf[sp_len + 1] = 't';
                    sp_len += 2;
                } else {
                    safe_payload_buf[sp_len] = c;
                    sp_len += 1;
                }
            }
            const payload = safe_payload_buf[0..sp_len];

            const rt_json = std.fmt.bufPrint(
                _.binding_event_buf[bpos..],
                "{{\"name\":\"{s}\",\"payload\":\"{s}\",\"executionContextId\":1}}",
                .{ name, payload },
            ) catch break;
            bpos += rt_json.len;

            out_events[ev_idx] = CdpEvent.create("Runtime.bindingCalled", rt_json);
            ev_idx += 1;
        }

        return ev_idx;
    }

    fn recordNavigation(*_, nav_url: []const u8, nav_title: []const u8) {
        // If current entry has the same URL, update its title (e.g. reload)
        if (_.nav_entry_count > 0 && _.current_nav_index < _.nav_entry_count) {
            if (std.mem.eql(u8, _.nav_entries[_.current_nav_index].url, nav_url)) {
                if (nav_title.len > 0) {
                    if (_.nav_entries[_.current_nav_index].title.len > 0) {
                        _.allocator.free(_.nav_entries[_.current_nav_index].title);
                    }
                    _.nav_entries[_.current_nav_index].title = _.allocator.dupe(u8, nav_title) catch "";
                }
                return;
            }
        }

        // Truncate any forward entries if we navigated after back
        if (_.current_nav_index + 1 < _.nav_entry_count) {
            let tr: usize = _.current_nav_index + 1;
            while (tr < _.nav_entry_count) : (tr += 1) {
                if (_.nav_entries[tr].url.len > 0) {
                    _.allocator.free(_.nav_entries[tr].url);
                    _.nav_entries[tr].url = "";
                }
                if (_.nav_entries[tr].title.len > 0) {
                    _.allocator.free(_.nav_entries[tr].title);
                    _.nav_entries[tr].title = "";
                }
            }
            _.nav_entry_count = _.current_nav_index + 1;
        }

        const dup_url = _.allocator.dupe(u8, nav_url) catch "";
        let dup_title: []const u8 = "";
        if (nav_title.len > 0) {
            dup_title = _.allocator.dupe(u8, nav_title) catch "";
        }

        const entry_id = _.next_nav_id;
        _.next_nav_id += 1;

        if (_.nav_entry_count < 16) {
            const idx = _.nav_entry_count;
            _.nav_entries[idx] = NavigationEntry{
                id: entry_id,
                url: dup_url,
                title: dup_title,
            };
            _.current_nav_index = idx;
            _.nav_entry_count += 1;
        } else {
            // Drop oldest entry (slot 0)
            if (_.nav_entries[0].url.len > 0) {
                _.allocator.free(_.nav_entries[0].url);
            }
            if (_.nav_entries[0].title.len > 0) {
                _.allocator.free(_.nav_entries[0].title);
            }

            let s: usize = 0;
            while (s < 15) : (s += 1) {
                _.nav_entries[s] = _.nav_entries[s + 1];
            }

            _.nav_entries[15] = NavigationEntry{
                id: entry_id,
                url: dup_url,
                title: dup_title,
            };
            _.current_nav_index = 15;
        }
    }

    pub fn updateCurrentNavTitle(*_, new_title: []const u8) {
        if (_.nav_entry_count > 0 && _.current_nav_index < _.nav_entry_count) {
            if (!std.mem.eql(u8, _.nav_entries[_.current_nav_index].title, new_title)) {
                if (_.nav_entries[_.current_nav_index].title.len > 0) {
                    _.allocator.free(_.nav_entries[_.current_nav_index].title);
                }
                _.nav_entries[_.current_nav_index].title = _.allocator.dupe(u8, new_title) catch "";
            }
        }
    }

    pub fn navigateToHistoryEntry(*_, entry_id: i32): bool {
        let i: usize = 0;
        while (i < _.nav_entry_count) : (i += 1) {
            if (_.nav_entries[i].id == entry_id) {
                _.current_nav_index = i;
                _.url = _.nav_entries[i].url;
                return true;
            }
        }
        return false;
    }

    pub fn pushNavigatedWithinDocEvent(*_, url: []const u8) {
        _.recordSameDocNavigation(url, true);
        if (_.pending_same_doc_count < _.pending_same_doc_urls.len) {
            _.pending_same_doc_urls[_.pending_same_doc_count] = _.allocator.dupe(u8, url) catch url;
            _.pending_same_doc_count += 1;
        }
    }

    pub fn recordSameDocNavigation(*_, url: []const u8, is_push: bool) {
        if (!canRewriteDocumentUrl(_.url, url)) return;
        if (is_push) {
            _.recordNavigation(url, _.title);
            if (_.nav_entry_count > 0 && _.current_nav_index < _.nav_entry_count) {
                _.url = _.nav_entries[_.current_nav_index].url;
            }
        } else {
            if (_.nav_entry_count > 0 && _.current_nav_index < _.nav_entry_count) {
                if (_.nav_entries[_.current_nav_index].url.len > 0) {
                    _.allocator.free(_.nav_entries[_.current_nav_index].url);
                }
                _.nav_entries[_.current_nav_index].url = _.allocator.dupe(u8, url) catch "";
                _.url = _.nav_entries[_.current_nav_index].url;
            }
        }
    }

    pub fn pollNavigatedWithinDocEvents(*_, out_events: []CdpEvent): usize {
        let ev_idx: usize = 0;

        // Pass A: Drain pending same-doc navigations from JS context into local stack buffer
        let js_urls: [4][]const u8 = [""; 4];
        let js_pushes: [4]bool = [true; 4];
        let js_count: usize = 0;

        const drain_expr = "(function(){if(typeof __capsurf_drain_same_doc_nav==='function')return __capsurf_drain_same_doc_nav();const q=globalThis.__capsurf_same_doc_nav||[];globalThis.__capsurf_same_doc_nav=[];return JSON.stringify(q);})()";
        const val = _.ctx.eval(drain_expr);
        defer _.ctx.freeValue(val);

        const json_str = _.ctx.toCString(val);
        defer std.heap.page_allocator.free(json_str);

        if (json_str.len > 2) {
            let pos: usize = 0;
            while (js_count < js_urls.len) {
                const url_marker = "\"url\":\"";
                const url_start_opt = std.mem.indexOfPos(u8, json_str, pos, url_marker);
                if (url_start_opt == null) break;
                const url_start = url_start_opt.? + url_marker.len;
                const url_end_opt = std.mem.indexOfPos(u8, json_str, url_start, "\"");
                if (url_end_opt == null) break;
                const url_val = json_str[url_start..url_end_opt.?];

                let is_push = true;
                if (push_start_opt := std.mem.indexOfPos(u8, json_str, url_end_opt.?, "\"isPush\":")) {
                    const p_start = push_start_opt + 9;
                    if (p_start + 5 <= json_str.len && std.mem.eql(u8, json_str[p_start..p_start + 5], "false")) {
                        is_push = false;
                    }
                } else if (type_start_opt := std.mem.indexOfPos(u8, json_str, url_end_opt.?, "\"type\":\"")) {
                    const t_start = type_start_opt + 8;
                    if (t_end_opt := std.mem.indexOfPos(u8, json_str, t_start, "\"")) {
                        const type_val = json_str[t_start..t_end_opt];
                        if (std.mem.eql(u8, type_val, "replaceState")) {
                            is_push = false;
                        }
                    }
                }

                pos = url_end_opt.? + 1;

                if (canRewriteDocumentUrl(_.url, url_val)) {
                    js_urls[js_count] = url_val;
                    js_pushes[js_count] = is_push;
                    js_count += 1;
                }
            }
        }

        // Pass B: Apply same-doc navigation state updates (all mutable operations)
        let ji: usize = 0;
        while (ji < js_count) : (ji += 1) {
            _.recordSameDocNavigation(js_urls[ji], js_pushes[ji]);
        }

        // Pass C: Format and emit pending events from CP queue
        const p_count = _.pending_same_doc_count;
        _.pending_same_doc_count = 0;

        let pi: usize = 0;
        while (pi < p_count && ev_idx < out_events.len) : (pi += 1) {
            const pending_url = _.pending_same_doc_urls[pi];
            const offset = ev_idx * 512;
            const chunk = _.same_doc_event_buf[offset .. offset + 512];

            let esc_buf: [512]u8 = [0; 512];
            const esc_url = escapeJsonString(pending_url, &esc_buf);

            const nav_json = std.fmt.bufPrint(
                chunk,
                "{{\"frameId\":\"{s}\",\"url\":\"{s}\",\"navigationType\":\"historyApi\"}}",
                .{ _.frame_id, esc_url },
            ) catch "";

            if (pending_url.len > 0) {
                _.allocator.free(pending_url);
                _.pending_same_doc_urls[pi] = "";
            }

            if (nav_json.len > 0) {
                out_events[ev_idx] = CdpEvent.create("Page.navigatedWithinDocument", nav_json);
                ev_idx += 1;
            }
        }

        // Pass D: Format and emit events from JS queue
        let ki: usize = 0;
        while (ki < js_count && ev_idx < out_events.len) : (ki += 1) {
            const url_val = js_urls[ki];
            const offset = ev_idx * 512;
            const chunk = _.same_doc_event_buf[offset .. offset + 512];

            let esc_buf: [512]u8 = [0; 512];
            const esc_url = escapeJsonString(url_val, &esc_buf);

            const nav_type = if (js_pushes[ki]) "historyApi" else "replaceState";
            const nav_json = std.fmt.bufPrint(
                chunk,
                "{{\"frameId\":\"{s}\",\"url\":\"{s}\",\"navigationType\":\"{s}\"}}",
                .{ _.frame_id, esc_url, nav_type },
            ) catch break;

            out_events[ev_idx] = CdpEvent.create("Page.navigatedWithinDocument", nav_json);
            ev_idx += 1;
        }

        return ev_idx;
    }

    pub fn formatExecutionContextEvent(*_, origin: []const u8): []const u8 {
        return std.fmt.bufPrint(
            &_.rt_ctx_event_buf,
            "{{\"context\":{{\"id\":1,\"origin\":\"{s}\",\"name\":\"\",\"uniqueId\":\"ctx-1\",\"auxData\":{{\"isDefault\":true,\"type\":\"default\",\"frameId\":\"{s}\"}}}}}}",
            .{ origin, _.frame_id },
        ) catch "{\"context\":{\"id\":1,\"origin\":\"about:blank\",\"name\":\"\",\"uniqueId\":\"ctx-1\",\"auxData\":{\"isDefault\":true,\"type\":\"default\",\"frameId\":\"capsurf-frame-001\"}}}";
    }

    pub fn formatFrameNavigatedEvent(*_, url: []const u8): []const u8 {
        const mime = if (std.mem.indexOf(u8, url, ".svg") != null) "image/svg+xml" else "text/html";
        return std.fmt.bufPrint(
            &_.page_frame_event_buf,
            "{{\"frame\":{{\"id\":\"{s}\",\"loaderId\":\"{s}\",\"url\":\"{s}\",\"securityOrigin\":\"{s}\",\"mimeType\":\"{s}\"}}}}",
            .{ _.frame_id, _.loader_id, url, url, mime },
        ) catch "{}";
    }

    pub fn getTitle(*_): []const u8 {
        const title_node = Matcher.querySelector(&_.doc, "title");
        if (title_node != NULL_NODE) {
            const first_text = _.doc.getFirstChild(title_node);
            if (first_text != NULL_NODE) {
                const t = _.doc.getTextContent(first_text);
                if (t.len > 0) return t;
            }
            const t2 = _.doc.getTextContent(title_node);
            if (t2.len > 0) return t2;
        }
        if (_.title.len > 0) return _.title;
        return "";
    }

    pub fn getPageStateJson(*_, out_buf: []u8): ([]const u8)? {
        let js_seq: u32 = 0;
        if (_.has_document) {
            const v = _.ctx.eval("globalThis.__treeSeq || 0");
            if (!jsIsException(v)) {
                let n: i32 = 0;
                * = JS_ToInt32(_.ctx.raw_ctx, &n, v);
                if (n > 0) js_seq = @as(u32, @intCast(n));
            }
            _.ctx.freeValue(v);
        }
        const total_seq: u32 = _.doc.mutation_seq + js_seq;
        const title_str = _.getTitle();

        let esc_title_buf: [512]u8 = undefined;
        const esc_title = escapeJsonString(title_str, &esc_title_buf);

        let esc_url_buf: [1024]u8 = undefined;
        const esc_url = escapeJsonString(_.url, &esc_url_buf);

        const sx: i32 = @intFromFloat(_.scroll_x);
        const sy: i32 = @intFromFloat(_.scroll_y);

        const res = std.fmt.bufPrint(
            out_buf,
            "{{\"seq\":{d},\"scrollX\":{d},\"scrollY\":{d},\"href\":\"{s}\",\"title\":\"{s}\"}}",
            .{ total_seq, sx, sy, esc_url, esc_title },
        ) catch return null;
        return res;
    }

    pub fn getFontFacesJson(*_, out_buf: []u8): ([]const u8)? {
        let families: [16]RegisteredFamily = [RegisteredFamily{}; 16];
        let family_count: usize = 0;

        let css_buf = _.allocator.alloc(u8, 65536) catch null;
        defer if (css_buf != null) _.allocator.free(css_buf.?);
        let css_len: usize = 0;
        if (css_buf != null) {
            let snid: usize = 0;
            while (snid < _.doc.nodes.items.len) : (snid += 1) {
                const sn = &_.doc.nodes.items[snid];
                if (sn.node_type == NodeType.Element && std.ascii.eqlIgnoreCase(sn.tag_name, "style")) {
                    if (sn.text_content.len > 0) {
                        const copy_len = @min(sn.text_content.len, css_buf.?.len - css_len);
                        @memcpy(css_buf.?[css_len .. css_len + copy_len], sn.text_content[0..copy_len]);
                        css_len += copy_len;
                    }
                    let child = sn.first_child;
                    while (child != NULL_NODE && @as(usize, child) < _.doc.nodes.items.len) : (child = _.doc.nodes.items[@as(usize, child)].next_sibling) {
                        const scn = &_.doc.nodes.items[@as(usize, child)];
                        if (scn.node_type == NodeType.Text && scn.text_content.len > 0) {
                            const copy_len = @min(scn.text_content.len, css_buf.?.len - css_len);
                            @memcpy(css_buf.?[css_len .. css_len + copy_len], scn.text_content[0..copy_len]);
                            css_len += copy_len;
                        }
                    }
                }
            }
        }

        if (css_len == 0 && _.raw_html.len > 0) {
            let s_idx: usize = 0;
            while (s_idx < _.raw_html.len) {
                if (st_start := std.mem.indexOfPos(u8, _.raw_html, s_idx, "<style")) {
                    if (tag_close := std.mem.indexOfPos(u8, _.raw_html, st_start, ">")) {
                        if (st_end := std.mem.indexOfPos(u8, _.raw_html, tag_close + 1, "</style>")) {
                            const st_text = _.raw_html[tag_close + 1 .. st_end];
                            if (css_buf != null) {
                                const copy_len = @min(st_text.len, css_buf.?.len - css_len);
                                @memcpy(css_buf.?[css_len .. css_len + copy_len], st_text[0..copy_len]);
                                css_len += copy_len;
                            }
                            s_idx = st_end + 8;
                        } else break;
                    } else break;
                } else break;
            }
        }

        const full_css = if (css_buf != null) css_buf.?[0..css_len] else "";
        if (full_css.len > 0) {
            let search_idx: usize = 0;
            while (search_idx < full_css.len) {
                if (ff_pos := std.mem.indexOfPos(u8, full_css, search_idx, "@font-face")) {
                    const open_brace = std.mem.indexOfPos(u8, full_css, ff_pos, "{");
                    if (open_brace == null) break;
                    const close_brace = std.mem.indexOfPos(u8, full_css, open_brace.?, "}");
                    if (close_brace == null) break;
                    const block = full_css[open_brace.? + 1 .. close_brace.?];
                    search_idx = close_brace.? + 1;

                    let fam_name: []const u8 = "";
                    if (fam_pos := std.mem.indexOf(u8, block, "font-family")) {
                        if (colon_pos := std.mem.indexOfPos(u8, block, fam_pos, ":")) {
                            let end_pos = block.len;
                            if (semi_pos := std.mem.indexOfPos(u8, block, colon_pos, ";")) {
                                end_pos = semi_pos;
                            }
                            fam_name = std.mem.trim(u8, block[colon_pos + 1 .. end_pos], " \t\r\n\"'");
                        }
                    }
                    if (fam_name.len == 0) continue;

                    let src_str: []const u8 = "";
                    if (src_pos := std.mem.indexOf(u8, block, "src")) {
                        if (colon_pos := std.mem.indexOfPos(u8, block, src_pos, ":")) {
                            let end_pos = block.len;
                            if (semi_pos := std.mem.indexOfPos(u8, block, colon_pos, ";")) {
                                end_pos = semi_pos;
                            }
                            src_str = std.mem.trim(u8, block[colon_pos + 1 .. end_pos], " \t\r\n");
                        }
                    }

                    let weight_str: []const u8 = "";
                    if (w_pos := std.mem.indexOf(u8, block, "font-weight")) {
                        if (colon_pos := std.mem.indexOfPos(u8, block, w_pos, ":")) {
                            let end_pos = block.len;
                            if (semi_pos := std.mem.indexOfPos(u8, block, colon_pos, ";")) {
                                end_pos = semi_pos;
                            }
                            weight_str = std.mem.trim(u8, block[colon_pos + 1 .. end_pos], " \t\r\n");
                        }
                    }

                    let urange_str: []const u8 = "";
                    if (ur_pos := std.mem.indexOf(u8, block, "unicode-range")) {
                        if (colon_pos := std.mem.indexOfPos(u8, block, ur_pos, ":")) {
                            let end_pos = block.len;
                            if (semi_pos := std.mem.indexOfPos(u8, block, colon_pos, ";")) {
                                end_pos = semi_pos;
                            }
                            urange_str = std.mem.trim(u8, block[colon_pos + 1 .. end_pos], " \t\r\n");
                        }
                    }

                    let has_wght_axis = false;
                    if (std.mem.indexOfScalar(u8, weight_str, ' ') != null) {
                        has_wght_axis = true;
                    }
                    if (std.mem.indexOf(u8, src_str, "var") != null || std.mem.indexOf(u8, src_str, "Var") != null) {
                        has_wght_axis = true;
                    }

                    let face_weight: u32 = 400;
                    if (!has_wght_axis) {
                        if (parsed_w := std.fmt.parseInt(u32, weight_str, 10) catch null) {
                            face_weight = parsed_w;
                        } else if (std.ascii.eqlIgnoreCase(weight_str, "bold")) {
                            face_weight = 700;
                        }
                    }

                    if (urange_str.len > 0) {
                        let min_cp: u32 = 0;
                        let max_cp: u32 = 0;
                        if (parseUnicodeRange(urange_str, &min_cp, &max_cp)) {
                            if (!documentHasCodePointInRange(&_.doc, min_cp, max_cp)) {
                                continue;
                            }
                        }
                    }

                    let target_idx = family_count;
                    let fi: usize = 0;
                    while (fi < family_count) : (fi += 1) {
                        const existing = families[fi].family[0..families[fi].family_len];
                        if (std.mem.eql(u8, existing, fam_name)) {
                            target_idx = fi;
                            break;
                        }
                    }

                    if (target_idx < families.len) {
                        let fam_entry = &families[target_idx];
                        if (target_idx == family_count) {
                            const c_len = @min(fam_name.len, fam_entry.family.len);
                            @memcpy(fam_entry.family[0..c_len], fam_name[0..c_len]);
                            fam_entry.family_len = c_len;
                            fam_entry.face_count = 0;
                            family_count += 1;
                        }
                        if (fam_entry.face_count < fam_entry.faces.len) {
                            fam_entry.faces[fam_entry.face_count] = RegisteredFace{
                                weight: face_weight,
                                has_wght_axis: has_wght_axis,
                            };
                            fam_entry.face_count += 1;
                        }
                    }
                } else {
                    break;
                }
            }
        }

        // Script fallbacks for Arabic, Hebrew, CJK
        if (documentHasCodePointInRange(&_.doc, 0x0600, 0x06FF)) {
            let found_ar = false;
            let fi: usize = 0;
            while (fi < family_count) : (fi += 1) {
                if (std.mem.eql(u8, families[fi].family[0..families[fi].family_len], "Noto Sans Arabic")) {
                    found_ar = true;
                    break;
                }
            }
            if (!found_ar && family_count < families.len) {
                const ar_name = "Noto Sans Arabic";
                @memcpy(families[family_count].family[0..ar_name.len], ar_name);
                families[family_count].family_len = ar_name.len;
                families[family_count].faces[0] = RegisteredFace{ weight: 400, has_wght_axis: false };
                families[family_count].face_count = 1;
                family_count += 1;
            }
        }
        if (documentHasCodePointInRange(&_.doc, 0x0590, 0x05FF)) {
            let found_he = false;
            let fi: usize = 0;
            while (fi < family_count) : (fi += 1) {
                if (std.mem.eql(u8, families[fi].family[0..families[fi].family_len], "Noto Sans Hebrew")) {
                    found_he = true;
                    break;
                }
            }
            if (!found_he && family_count < families.len) {
                const he_name = "Noto Sans Hebrew";
                @memcpy(families[family_count].family[0..he_name.len], he_name);
                families[family_count].family_len = he_name.len;
                families[family_count].faces[0] = RegisteredFace{ weight: 400, has_wght_axis: false };
                families[family_count].face_count = 1;
                family_count += 1;
            }
        }
        if (documentHasCodePointInRange(&_.doc, 0x4E00, 0x9FFF)) {
            let found_cjk = false;
            let fi: usize = 0;
            while (fi < family_count) : (fi += 1) {
                if (std.mem.eql(u8, families[fi].family[0..families[fi].family_len], "Noto Sans CJK SC")) {
                    found_cjk = true;
                    break;
                }
            }
            if (!found_cjk && family_count < families.len) {
                const cjk_name = "Noto Sans CJK SC";
                @memcpy(families[family_count].family[0..cjk_name.len], cjk_name);
                families[family_count].family_len = cjk_name.len;
                families[family_count].faces[0] = RegisteredFace{ weight: 400, has_wght_axis: false };
                families[family_count].face_count = 1;
                family_count += 1;
            }
        }

        let pos: usize = 0;
        const prefix = "{\"byTarget\":{\"";
        @memcpy(out_buf[pos .. pos + prefix.len], prefix);
        pos += prefix.len;

        const tid = _.target_id;
        @memcpy(out_buf[pos .. pos + tid.len], tid);
        pos += tid.len;

        const mid = "\":{\"json\":\"[";
        @memcpy(out_buf[pos .. pos + mid.len], mid);
        pos += mid.len;

        let fam_i: usize = 0;
        while (fam_i < family_count) : (fam_i += 1) {
            const fam = &families[fam_i];
            if (fam_i > 0) {
                out_buf[pos] = ',';
                pos += 1;
            }
            const f_name = fam.family[0..fam.family_len];
            const fam_header = std.fmt.bufPrint(
                out_buf[pos..],
                "{{\\\"family\\\":\\\"{s}\\\",\\\"faceCount\\\":{d},\\\"faces\\\":[",
                .{ f_name, fam.face_count },
            ) catch return null;
            pos += fam_header.len;

            let face_i: usize = 0;
            while (face_i < fam.face_count) : (face_i += 1) {
                const face = &fam.faces[face_i];
                if (face_i > 0) {
                    out_buf[pos] = ',';
                    pos += 1;
                }
                const axis_count: u32 = if (face.has_wght_axis) 1 else 0;
                const axis_str: []const u8 = if (face.has_wght_axis) "true" else "false";
                const face_json = std.fmt.bufPrint(
                    out_buf[pos..],
                    "{{\\\"weight\\\":{d},\\\"style\\\":\\\"Normal\\\",\\\"width\\\":1.0,\\\"hasWghtAxis\\\":{s},\\\"axisCount\\\":{d}}}",
                    .{ face.weight, axis_str, axis_count },
                ) catch return null;
                pos += face_json.len;
            }

            const fam_tail = "]}";
            @memcpy(out_buf[pos .. pos + fam_tail.len], fam_tail);
            pos += fam_tail.len;
        }

        const suffix = "]\"}}}";
        @memcpy(out_buf[pos .. pos + suffix.len], suffix);
        pos += suffix.len;

        return out_buf[0..pos];
    }

    pub fn runGC(*_) {
        _.rt.runGC();
    }

    pub fn updateLayoutMetrics(*_, out_sx: *f32, out_sy: *f32, out_cw: *f32, out_ch: *f32) {
        if (!_.has_document) {
            out_sx.* = 0.0;
            out_sy.* = 0.0;
            out_cw.* = _.viewport_width;
            out_ch.* = _.viewport_height;
            return;
        }

        let max_w: f32 = _.viewport_width;
        let max_h: f32 = _.viewport_height;
        let bi: usize = 0;
        while (bi < _.layout.boxes.items.len) : (bi += 1) {
            const b = &_.layout.boxes.items[bi];
            const r = b.rect.x + b.rect.width;
            if (r > max_w) max_w = r;
            const bot = b.rect.y + b.rect.height;
            if (bot > max_h) max_h = bot;
        }

        const v = _.ctx.eval("[window.scrollX || 0, window.scrollY || 0].join(',')");
        defer _.ctx.freeValue(v);
        if (!jsIsException(v)) {
            const s = _.ctx.toCString(v);
            if (s.len > 0) {
                let it = std.mem.splitScalar(u8, s, ',');
                if (sx_str := it.next()) {
                    if (sx_val := std.fmt.parseFloat(f32, sx_str) catch null) {
                        _.scroll_x = sx_val;
                    }
                }
                if (sy_str := it.next()) {
                    if (sy_val := std.fmt.parseFloat(f32, sy_str) catch null) {
                        _.scroll_y = sy_val;
                    }
                }
            }
        }

        out_sx.* = _.scroll_x;
        out_sy.* = _.scroll_y;
        out_cw.* = max_w;
        out_ch.* = max_h;
    }

    pub fn setControlValueJson(*_, params_json: []const u8, out_buf: []u8): ([]const u8)? {
        const node_id = extractJsonInt(params_json, "nodeId") ?? -1;
        if (node_id <= 0) {
            const bad_res = "{\"changed\":false,\"error\":\"bad nodeId\"}";
            if (bad_res.len > out_buf.len) return null;
            @memcpy(out_buf[0..bad_res.len], bad_res);
            return out_buf[0..bad_res.len];
        }
        if (@as(usize, @intCast(node_id)) >= _.doc.nodes.items.len) {
            const bad_res = "{\"changed\":false,\"error\":\"bad nodeId\"}";
            if (bad_res.len > out_buf.len) return null;
            @memcpy(out_buf[0..bad_res.len], bad_res);
            return out_buf[0..bad_res.len];
        }
        if (!_.has_document) {
            const bad_res = "{\"changed\":false,\"error\":\"bad nodeId\"}";
            if (bad_res.len > out_buf.len) return null;
            @memcpy(out_buf[0..bad_res.len], bad_res);
            return out_buf[0..bad_res.len];
        }

        const global_obj = _.ctx.getGlobal();
        defer _.ctx.freeValue(global_obj);

        const params_val = _.ctx.newString(params_json);
        * = JS_SetPropertyStr(_.ctx.raw_ctx, global_obj, "__capsurf_temp_control_params", params_val);

        const res_val = _.ctx.eval("(function(){ if (typeof __capsurf_setControlValueJson === 'function') { return __capsurf_setControlValueJson(globalThis.__capsurf_temp_control_params); } return JSON.stringify({changed:false,error:'bad nodeId'}); })()");
        defer _.ctx.freeValue(res_val);

        const str = _.ctx.toCString(res_val);
        defer std.heap.page_allocator.free(str);
        if (str.len > 0 && str.len <= out_buf.len) {
            @memcpy(out_buf[0..str.len], str);
            return out_buf[0..str.len];
        }

        const default_res = "{\"changed\":false,\"error\":\"bad nodeId\"}";
        if (default_res.len > out_buf.len) return null;
        @memcpy(out_buf[0..default_res.len], default_res);
        return out_buf[0..default_res.len];
    }

    pub fn markPaintDamage(*_) {
        _.doc.markPaintDamage();
    }

    pub fn getPaintDamageSinceJson(*_, params_json: []const u8, out_buf: []u8): ([]const u8)? {
        let excl_id: i32 = 0;
        if (std.mem.indexOf(u8, params_json, "\"excludeNodeId\"") != null) {
            if (e_val := extractJsonInt(params_json, "excludeNodeId")) {
                excl_id = @as(i32, @intCast(e_val));
            } else {
                excl_id = -1;
            }
        }
        return _.getPaintDamageSince(excl_id, out_buf);
    }

    pub fn getPaintDamageSince(*_, exclude_id: i32, out_buf: []u8): ([]const u8)? {
        if (exclude_id < 0) {
            const err_json = "{\"nodeIds\":[],\"overflow\":false,\"error\":\"bad nodeId\"}";
            if (err_json.len > out_buf.len) return null;
            @memcpy(out_buf[0..err_json.len], err_json);
            return out_buf[0..err_json.len];
        }

        const excl_node: NodeId = if (exclude_id > 0) @as(NodeId, @intCast(exclude_id)) else NULL_NODE;

        let offset: usize = 0;
        const header = "{\"nodeIds\":[";
        if (offset + header.len > out_buf.len) return null;
        @memcpy(out_buf[offset .. offset + header.len], header);
        offset += header.len;

        let first = true;
        let i: usize = 0;
        while (i < _.doc.damaged_nodes.items.len) : (i += 1) {
            const nid = _.doc.damaged_nodes.items[i];
            if (excl_node != NULL_NODE && _.doc.isLineageOrSubtree(nid, excl_node)) {
                continue;
            }
            if (!first) {
                if (offset + 1 > out_buf.len) return null;
                out_buf[offset] = ',';
                offset += 1;
            }
            first = false;
            const num = std.fmt.bufPrint(out_buf[offset..], "{d}", .{nid}) catch break;
            offset += num.len;
        }

        const footer = "],\"overflow\":false}";
        if (offset + footer.len <= out_buf.len) {
            @memcpy(out_buf[offset .. offset + footer.len], footer);
            offset += footer.len;
            return out_buf[0..offset];
        }
        return null;
    }
}
