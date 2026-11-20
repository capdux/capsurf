// src/cdp/input.cp
// CDP Input Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/input.ts

import { CdpRequest, CdpResponse, CdpEvent, MAX_CDP_EVENTS, extractJsonString, extractJsonInt, extractJsonFloat, extractJsonBool, unescapeJsonString } from "./types.cp";
import { CdpPageSession, escapeJsonString } from "./session.cp";
import { NodeId, NULL_NODE } from "../dom/dom.cp";
import { HttpLoader } from "../net/http_loader.cp";
import { jsIsException } from "../js/qjs_engine.cp";

pub enum MouseEventType {
    MousePressed,
    MouseReleased,
    MouseMoved,
    MouseWheel,
    Unknown,

    static fn fromString(s: []const u8): MouseEventType {
        if (std.mem.eql(u8, s, "mousePressed")) return .MousePressed;
        if (std.mem.eql(u8, s, "mouseReleased")) return .MouseReleased;
        if (std.mem.eql(u8, s, "mouseMoved")) return .MouseMoved;
        if (std.mem.eql(u8, s, "mouseWheel")) return .MouseWheel;
        return .Unknown;
    }
}

pub enum MouseButton {
    None,
    Left,
    Middle,
    Right,
    Back,
    Forward,

    static fn fromString(s: []const u8): MouseButton {
        if (std.mem.eql(u8, s, "left")) return .Left;
        if (std.mem.eql(u8, s, "middle")) return .Middle;
        if (std.mem.eql(u8, s, "right")) return .Right;
        if (std.mem.eql(u8, s, "back")) return .Back;
        if (std.mem.eql(u8, s, "forward")) return .Forward;
        return .None;
    }
}

pub struct MouseEvent {
    event_type: MouseEventType = .Unknown,
    x: f32 = 0.0,
    y: f32 = 0.0,
    button: MouseButton = .None,
    click_count: i64 = 1,
    buttons: i64 = 0,
    modifiers: i64 = 0,
    delta_x: f32 = 0.0,
    delta_y: f32 = 0.0,
}

pub enum KeyEventType {
    KeyDown,
    KeyUp,
    Char,
    RawKeyDown,
    Unknown,

    static fn fromString(s: []const u8): KeyEventType {
        if (std.mem.eql(u8, s, "keyDown")) return .KeyDown;
        if (std.mem.eql(u8, s, "keyUp")) return .KeyUp;
        if (std.mem.eql(u8, s, "char")) return .Char;
        if (std.mem.eql(u8, s, "rawKeyDown")) return .RawKeyDown;
        return .Unknown;
    }
}

pub struct KeyEvent {
    event_type: KeyEventType = .Unknown,
    key: []const u8 = "",
    code: []const u8 = "",
    text: []const u8 = "",
    windows_virtual_key_code: i64 = 0,
    modifiers: i64 = 0,
    auto_repeat: bool = false,
}

/// Hit-tests screen pixel coordinates (x, y) against session's layout boxes.
pub fn hitTest(session: *CdpPageSession, x: f32, y: f32): NodeId {
    let best_node: NodeId = NULL_NODE;
    let i: usize = 0;
    const count = session.layout.boxes.items.len;
    while (i < count) : (i += 1) {
        const box = session.layout.boxes.items[i];
        if (box.rect.visible && box.rect.width > 0.0 && box.rect.height > 0.0) {
            if (x >= box.rect.x && x <= (box.rect.x + box.rect.width) &&
                y >= box.rect.y && y <= (box.rect.y + box.rect.height)) {
                best_node = box.node_id;
            }
        }
    }
    return best_node;
}

fn escapeJsString(input_str: []const u8, out: []u8): usize {
    let ei: usize = 0;
    let i: usize = 0;
    while (i < input_str.len) : (i += 1) {
        const c = input_str[i];
        if (c == '"' || c == '\\' || c == '\'') {
            if (ei + 1 < out.len) out[ei] = '\\';
            ei += 1;
        }
        if (ei < out.len) out[ei] = c;
        ei += 1;
    }
    return ei;
}

fn isPrintableKey(key: []const u8, text: []const u8): bool {
    if (text.len > 0) {
        const b = text[0];
        if (b >= 32 && b != 127) return true;
        if (b == '\r' || b == '\n' || b == '\t') return true;
    }
    if (key.len > 0) {
        if (std.mem.eql(u8, key, "Enter")) return true;
        const b = key[0];
        if (b >= 32 && b != 127) {
            if (key.len == 1 || b >= 0x80) return true;
        }
    }
    return false;
}

fn dispatchKeyJs(session: *CdpPageSession, type_str_js: []const u8, escaped_key: []const u8, escaped_code: []const u8, modifiers: i64) {
    let js_buf: [1024]u8 = [0; 1024];
    const js_expr = std.fmt.bufPrint(
        &js_buf,
        "(() => {{ if (typeof __capsurf_dispatch_key_event === 'function') {{ return __capsurf_dispatch_key_event('{s}', '{s}', '{s}', {d}); }} return 0; }})()",
        .{ type_str_js, escaped_key, escaped_code, modifiers },
    ) catch null;
    if (e := js_expr) {
        const val = session.ctx.eval(e);
        if (val.tag == 0 && val.u.uint64 > 0) {
            session.focused_node = @as(u32, @truncate(val.u.uint64));
            session.doc.focused_node = session.focused_node;
        }
        session.ctx.freeValue(val);
    }
}

pub fn applyInputText(session: *CdpPageSession, text: []const u8, key: []const u8) {
    if (text.len == 0 && key.len == 0) return;

    // Filter and escape text and key
    let clean_buf: [1024]u8 = [0; 1024];
    let clean_len: usize = 0;
    for (c of text) {
        if (c >= 32 && c != 127 && clean_len < clean_buf.len) {
            clean_buf[clean_len] = c;
            clean_len += 1;
        }
    }
    const clean_text = clean_buf[0..clean_len];
    
    // Escaping helper
    let esc_buf: [2048]u8 = [0; 2048];
    const escaped_text_len = escapeJsString(clean_text, &esc_buf);
    const escaped_text = esc_buf[0..escaped_text_len];
    
    let k_esc_buf: [1024]u8 = [0; 1024];
    const escaped_key_len = escapeJsString(key, &k_esc_buf);
    const escaped_key = k_esc_buf[0..escaped_key_len];

    if (session.has_document) {
        let js_buf: [4096]u8 = [0; 4096];
        const js_expr = std.fmt.bufPrint(
            &js_buf,
            "(() => {{ let el = document.activeElement; if (!el || el === document.body || el === document.documentElement) {{ if ({d} > 0) {{ function f(r, nid) {{ if (!r) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; }} return null; }} el = f(document, {d}); }} }} if (!el || el === document.body || el === document.documentElement) return 0; const getRaw = (typeof getValueRaw === 'function') ? getValueRaw : (e) => (e._value !== undefined ? e._value : (e.value || '')); const setRaw = (typeof setValueRaw === 'function') ? setValueRaw : (e, v) => {{ e._value = String(v); if (e.setAttribute) e.setAttribute('value', String(v)); }}; const oldVal = getRaw(el); const oldLen = oldVal.length; const start = typeof el.selectionStart === 'number' ? el.selectionStart : oldLen; const end = typeof el.selectionEnd === 'number' ? el.selectionEnd : start; let nextVal = oldVal; let cursor = start; const isPrintable = '{s}'.length > 0; if (isPrintable) {{ nextVal = oldVal.slice(0, start) + '{s}' + oldVal.slice(end); cursor = start + '{s}'.length; }} else if ('{s}' === 'Backspace') {{ if (start !== end) {{ nextVal = oldVal.slice(0, start) + oldVal.slice(end); cursor = start; }} else if (start > 0) {{ nextVal = oldVal.slice(0, start - 1) + oldVal.slice(end); cursor = start - 1; }} }} else if ('{s}' === 'Delete') {{ if (start !== end) {{ nextVal = oldVal.slice(0, start) + oldVal.slice(end); cursor = start; }} else if (start < oldLen) {{ nextVal = oldVal.slice(0, start) + oldVal.slice(start + 1); cursor = start; }} }} if (nextVal !== oldVal) {{ setRaw(el, nextVal); if (typeof notifyControlValueMutation === 'function') {{ notifyControlValueMutation(el, cursor); }} else if (typeof el.setSelectionRange === 'function') {{ el.setSelectionRange(cursor, cursor); }} }} return (typeof el._nid === 'number') ? el._nid : 0; }})()",
            .{ session.focused_node, session.focused_node, escaped_text, escaped_text, escaped_text, escaped_key, escaped_key },
        ) catch null;
        if (e := js_expr) {
            const val = session.ctx.eval(e);
            if (val.tag == 0 && val.u.uint64 > 0) {
                session.focused_node = @as(u32, @truncate(val.u.uint64));
            }
            session.ctx.freeValue(val);
        }
    }
}

fn dispatchMouseDomEvent(session: *CdpPageSession, target_node: NodeId, ev_name: []const u8, x: f32, y: f32): bool {
    if (!session.has_document) return true;
    let js_buf: [1280]u8 = [0; 1280];
    const js_expr = std.fmt.bufPrint(
        &js_buf,
        "(() => {{ function f(r, nid) {{ if (!r || nid <= 0) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; }} return null; }} const byNid = f(document, {d}); const byPt = (typeof document.elementFromPoint === 'function') ? document.elementFromPoint({d:.2}, {d:.2}) : null; const el = (byPt && byPt !== document.body && byPt !== document.documentElement) ? byPt : (byNid || byPt || document.body || document.documentElement); if (!el || typeof el.dispatchEvent !== 'function') return 1; const Ctor = (typeof MouseEvent === 'function') ? MouseEvent : Event; const ev = new Ctor('{s}', {{ bubbles: true, cancelable: true, clientX: {d:.2}, clientY: {d:.2}, button: 0 }}); const ok = el.dispatchEvent(ev); if ('{s}' === 'click' && ok && !ev.defaultPrevented && typeof globalThis.__capsurf_activate_clicked_element === 'function') {{ globalThis.__capsurf_activate_clicked_element(el, true, {d:.2}, {d:.2}, 1); }} return (ok && !ev.defaultPrevented) ? 1 : 0; }})()",
        .{ target_node, x, y, ev_name, x, y, ev_name, x, y },
    ) catch return true;
    const val = session.ctx.eval(js_expr);
    let not_prevented = true;
    if (val.tag == 0 && val.u.uint64 == 0) {
        not_prevented = false;
    }
    session.ctx.freeValue(val);
    return not_prevented;
}

fn updateRangeFromX(session: *CdpPageSession, node_id: NodeId, x: f32): bool {
    if (!session.has_document || node_id == NULL_NODE) return false;
    const rect = session.layout.getBoundingClientRect(node_id);
    if (rect.width <= 0.0) return false;
    const ratio = std.math.clamp((x - rect.x) / rect.width, 0.0, 1.0);
    let js_buf: [1280]u8 = [0; 1280];
    const js_expr = std.fmt.bufPrint(
        &js_buf,
        "(() => {{ function f(r, nid) {{ if (!r) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; }} return null; }} const el = f(document, {d}); if (!el) return 0; const mn = (el.min !== '' && !isNaN(Number(el.min))) ? Number(el.min) : 0; const mx = (el.max !== '' && !isNaN(Number(el.max))) ? Number(el.max) : 100; const st = (el.step && el.step !== 'any' && !isNaN(Number(el.step)) && Number(el.step) > 0) ? Number(el.step) : 1; const raw = mn + {d:.6} * (mx - mn); const stepped = Math.min(mx, Math.max(mn, mn + Math.round((raw - mn) / st) * st)); const nextStr = String(stepped); const prevStr = String(el.value); if (nextStr !== prevStr) {{ el.value = nextStr; if (typeof el.dispatchEvent === 'function') el.dispatchEvent(new Event('input', {{ bubbles: true }})); return 1; }} return 0; }})()",
        .{ node_id, ratio },
    ) catch return false;
    const val = session.ctx.eval(js_expr);
    let changed = false;
    if (val.tag == 0 && val.u.uint64 != 0) {
        changed = true;
    }
    session.ctx.freeValue(val);
    return changed;
}

fn dispatchRangeChange(session: *CdpPageSession, node_id: NodeId) {
    if (!session.has_document || node_id == NULL_NODE) return;
    let js_buf: [512]u8 = [0; 512];
    const js_expr = std.fmt.bufPrint(
        &js_buf,
        "(() => {{ function f(r, nid) {{ if (!r) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; }} return null; }} const el = f(document, {d}); if (el && typeof el.dispatchEvent === 'function') el.dispatchEvent(new Event('change', {{ bubbles: true }})); }})()",
        .{node_id},
    ) catch return;
    const val = session.ctx.eval(js_expr);
    session.ctx.freeValue(val);
}

pub fn emitPendingNavigationEvents(
    session: *CdpPageSession,
    out_events: *[MAX_CDP_EVENTS]CdpEvent,
    event_count: *usize,
) {
    if (!session.has_document) return;
    const val = session.ctx.eval(
        "(function(){return String(globalThis.__capsurf_pending_navigation||'');})()",
    );
    defer session.ctx.freeValue(val);
    if (jsIsException(val)) return;

    const pending_raw = session.ctx.toCString(val);
    if (pending_raw.len == 0 || pending_raw[0] == '#') return;

    let resolved_buf: [2048]u8 = [0; 2048];
    const target_url = HttpLoader.resolveRelativeUrl(session.url, pending_raw, &resolved_buf);
    if (target_url.len == 0) return;

    // Do not emit differentDocument navigation events for same-document fragment-only changes
    let cur_base = session.url;
    if (h := std.mem.indexOfScalar(u8, session.url, '#')) {
        cur_base = session.url[0..h];
    }
    let tgt_base = target_url;
    if (h := std.mem.indexOfScalar(u8, target_url, '#')) {
        tgt_base = target_url[0..h];
    }
    if (std.mem.eql(u8, cur_base, tgt_base) && std.mem.indexOfScalar(u8, target_url, '#') != null) {
        return;
    }

    let esc_u_buf: [2048]u8 = [0; 2048];
    const esc_u = escapeJsonString(target_url, &esc_u_buf);

    if (event_count.* < out_events.len) {
        const req_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\",\"reason\":\"other\",\"url\":\"{s}\",\"disposition\":\"currentTab\"}}",
            .{ session.frame_id, esc_u },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.frameRequestedNavigation", req_json);
        event_count.* += 1;
    }

    if (event_count.* < out_events.len) {
        const start_json = std.fmt.allocPrint(
            session.allocator,
            "{{\"frameId\":\"{s}\",\"url\":\"{s}\",\"loaderId\":\"{s}\",\"navigationType\":\"differentDocument\"}}",
            .{ session.frame_id, esc_u, session.loader_id },
        ) catch "{}";
        out_events[event_count.*] = CdpEvent.create("Page.frameStartedNavigating", start_json);
        event_count.* += 1;
    }
}

pub struct InputHandler {
    pub static fn handle(
        session: *CdpPageSession,
        req: CdpRequest,
        out_buf: []u8,
        out_events: *[MAX_CDP_EVENTS]CdpEvent,
        event_count: *usize,
    ): CdpResponse {
        * = out_buf;
        event_count.* = 0;

        // 1. Mouse Events: dispatchMouseEvent
        if (std.mem.eql(u8, req.method, "Input.dispatchMouseEvent")) {
            const type_str = extractJsonString(req.params_raw, "type") ?? "";
            const ev_type = MouseEventType.fromString(type_str);
            const x = extractJsonFloat(req.params_raw, "x") ?? 0.0;
            const y = extractJsonFloat(req.params_raw, "y") ?? 0.0;
            const btn_str = extractJsonString(req.params_raw, "button") ?? "none";
            const button = MouseButton.fromString(btn_str);
            const click_count = extractJsonInt(req.params_raw, "clickCount") ?? 1;
            const buttons = extractJsonInt(req.params_raw, "buttons") ?? 0;
            const modifiers = extractJsonInt(req.params_raw, "modifiers") ?? 0;
            const delta_x = extractJsonFloat(req.params_raw, "deltaX") ?? 0.0;
            const delta_y = extractJsonFloat(req.params_raw, "deltaY") ?? 0.0;

            const mouse_ev = MouseEvent {
                event_type: ev_type,
                x: x,
                y: y,
                button: button,
                click_count: click_count,
                buttons: buttons,
                modifiers: modifiers,
                delta_x: delta_x,
                delta_y: delta_y,
            };
            * = mouse_ev;

            // Handle mouseWheel: scroll_x / scroll_y
            if (ev_type == .MouseWheel) {
                session.scroll_x += delta_x;
                session.scroll_y += delta_y;
            }

            // Hit test and dispatch mouse / range drag events
            if (ev_type == .MousePressed) {
                if (session.layout.boxes.items.len == 0) {
                    session.ensureLayout();
                }
                const target_node = hitTest(session, x, y);
                const not_prevented = dispatchMouseDomEvent(session, target_node, "mousedown", x, y);
                if (target_node != NULL_NODE) {
                    session.focused_node = target_node;
                    if (not_prevented && @as(usize, target_node) < session.doc.nodes.items.len) {
                        const tn = &session.doc.nodes.items[@as(usize, target_node)];
                        if (tn.hasTag("input") && std.ascii.eqlIgnoreCase(tn.getAttribute("type") ?? "", "range")) {
                            session.range_drag_node = target_node;
                            session.range_drag_changed = updateRangeFromX(session, target_node, x);
                        }
                    }
                    // Trigger onclick JS attribute or property
                    if (onclick_code := session.doc.getAttribute(target_node, "onclick")) {
                        if (session.has_document) {
                            const val = session.ctx.eval(onclick_code);
                            session.ctx.freeValue(val);
                        }
                    }
                    if (session.has_document) {
                        let expr_buf: [384]u8 = [0; 384];
                        const expr = std.fmt.bufPrint(
                            &expr_buf,
                            "(() => {{ function f(r, nid) {{ if (!r) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; }} return null; }} const el = f(document, {d}); if (el) {{ if (typeof el.onclick === 'function') el.onclick(); }} }})()",
                            .{target_node},
                        ) catch null;
                        if (e := expr) {
                            const val = session.ctx.eval(e);
                            session.ctx.freeValue(val);
                        }
                    }
                }
                if (session.has_document && button == .Left && not_prevented && session.range_drag_node == NULL_NODE) {
                    let sel_buf: [2048]u8 = [0; 2048];
                    const sel_js = std.fmt.bufPrint(
                        &sel_buf,
                        "(() => {{ function f(r, nid) {{ if (!r || nid <= 0) return null; if (r._nid === nid) return r; const cs = r.childNodes || []; for (let i = 0; i < cs.length; i++) {{ const h = f(cs[i], nid); if (h) return h; }} return null; }} function firstText(n) {{ if (!n) return null; if (n.nodeType === 3) return n; const cs = n.childNodes || []; for (let i = 0; i < cs.length; i++) {{ const t = firstText(cs[i]); if (t) return t; }} return null; }} let el = f(document, {d}); if (!el && typeof document.elementFromPoint === 'function') el = document.elementFromPoint({d:.2}, {d:.2}); if (!el) el = document.body || document.documentElement; if (!el) return; const tag = String(el.tagName || '').toUpperCase(); if (tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT' || tag === 'BUTTON') return; const tn = firstText(el); if (!tn) return; const boxEl = tn.parentNode || el; const r = (boxEl && boxEl.getBoundingClientRect) ? boxEl.getBoundingClientRect() : {{ left: 0 }}; const internals = globalThis[Symbol.for('kitesurf.internals')]; const cw = (internals && typeof internals.ttyCellWidth === 'number' && internals.ttyCellWidth > 0) ? internals.ttyCellWidth : 8; const len = (tn.nodeValue || tn.textContent || '').length; const startOff = Math.max(0, Math.min(len, Math.floor(({d:.2} - r.left) / cw))); globalThis.__capsurf_drag_sel = {{ tn: tn, startOff: startOff, left: r.left, cw: cw }}; if (typeof getSelection === 'function') {{ const sel = getSelection(); if (sel && sel.removeAllRanges) sel.removeAllRanges(); }} }})()",
                        .{ target_node, x, y, x },
                    ) catch null;
                    if (sjs := sel_js) {
                        const sval = session.ctx.eval(sjs);
                        session.ctx.freeValue(sval);
                    }
                }
            } else if (ev_type == .MouseMoved) {
                if (session.range_drag_node != NULL_NODE) {
                    if (updateRangeFromX(session, session.range_drag_node, x)) {
                        session.range_drag_changed = true;
                    }
                } else if (session.has_document && (buttons & 1) != 0) {
                    let mv_buf: [1024]u8 = [0; 1024];
                    const mv_js = std.fmt.bufPrint(
                        &mv_buf,
                        "(() => {{ const ds = globalThis.__capsurf_drag_sel; if (!ds || !ds.tn || typeof getSelection !== 'function' || !document.createRange) return; const len = (ds.tn.nodeValue || ds.tn.textContent || '').length; const rawOff = ({d:.2} - ds.left) / ds.cw; const endOff = rawOff >= ds.startOff ? Math.max(0, Math.min(len, Math.ceil(rawOff))) : Math.max(0, Math.min(len, Math.floor(rawOff))); const s = Math.min(ds.startOff, endOff); const e = Math.max(ds.startOff, endOff); const range = document.createRange(); range.setStart(ds.tn, s); range.setEnd(ds.tn, e); const sel = getSelection(); if (sel) {{ sel.removeAllRanges(); sel.addRange(range); }} }})()",
                        .{x},
                    ) catch null;
                    if (mjs := mv_js) {
                        const mval = session.ctx.eval(mjs);
                        session.ctx.freeValue(mval);
                    }
                }
            } else if (ev_type == .MouseReleased) {
                if (session.has_document) {
                    const clr = session.ctx.eval("globalThis.__capsurf_drag_sel = null;");
                    session.ctx.freeValue(clr);
                }
                const target_node = hitTest(session, x, y);
                * = dispatchMouseDomEvent(session, target_node, "mouseup", x, y);
                * = dispatchMouseDomEvent(session, target_node, "click", x, y);
                if (session.range_drag_node != NULL_NODE) {
                    if (session.range_drag_changed) {
                        dispatchRangeChange(session, session.range_drag_node);
                    }
                    session.range_drag_node = NULL_NODE;
                    session.range_drag_changed = false;
                }
                if (target_node != NULL_NODE) {
                    session.focused_node = target_node;
                    if (onclick_code := session.doc.getAttribute(target_node, "onclick")) {
                        if (session.has_document) {
                            const val = session.ctx.eval(onclick_code);
                            session.ctx.freeValue(val);
                        }
                    }
                    if (session.has_document) {
                        let expr_buf: [384]u8 = [0; 384];
                        const expr = std.fmt.bufPrint(
                            &expr_buf,
                            "(() => {{ function f(r, nid) {{ if (!r) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; }} return null; }} const el = f(document, {d}); if (el) {{ if (typeof el.onclick === 'function') el.onclick(); }} }})()",
                            .{target_node},
                        ) catch null;
                        if (e := expr) {
                            const val = session.ctx.eval(e);
                            session.ctx.freeValue(val);
                        }
                    }
                }
                emitPendingNavigationEvents(session, out_events, event_count);
            }

            return CdpResponse.ok(req.id, "{}");
        }

        // 2. Keyboard Events: dispatchKeyEvent
        if (std.mem.eql(u8, req.method, "Input.dispatchKeyEvent")) {
            const type_str = extractJsonString(req.params_raw, "type") ?? "";
            const ev_type = KeyEventType.fromString(type_str);
            const text = extractJsonString(req.params_raw, "text") ?? "";
            const key = extractJsonString(req.params_raw, "key") ?? "";
            const code = extractJsonString(req.params_raw, "code") ?? "";
            const vk_code = extractJsonInt(req.params_raw, "windowsVirtualKeyCode") ?? 0;
            const modifiers = extractJsonInt(req.params_raw, "modifiers") ?? 0;
            const repeat = extractJsonBool(req.params_raw, "autoRepeat") ?? false;

            const key_ev = KeyEvent {
                event_type: ev_type,
                key: key,
                code: code,
                text: text,
                windows_virtual_key_code: vk_code,
                modifiers: modifiers,
                auto_repeat: repeat,
            };
            * = key_ev;

            if (session.has_document) {
                if (ev_type == .KeyDown || ev_type == .KeyUp || ev_type == .RawKeyDown || ev_type == .Char) {
                    let k_esc_buf: [256]u8 = [0; 256];
                    let c_esc_buf: [256]u8 = [0; 256];

                    const escaped_key_len = escapeJsString(key, &k_esc_buf);
                    const escaped_key = k_esc_buf[0..escaped_key_len];

                    const escaped_code_len = escapeJsString(code, &c_esc_buf);
                    const escaped_code = c_esc_buf[0..escaped_code_len];

                    if (ev_type == .KeyDown || ev_type == .RawKeyDown) {
                        dispatchKeyJs(session, "keydown", escaped_key, escaped_code, modifiers);
                        if (ev_type == .KeyDown && (text.len > 0 || std.mem.eql(u8, key, "Enter")) && isPrintableKey(key, text)) {
                            const kp_key = if (text.len > 0) text else key;
                            let kp_esc_buf: [256]u8 = [0; 256];
                            const kp_esc_len = escapeJsString(kp_key, &kp_esc_buf);
                            dispatchKeyJs(session, "keypress", kp_esc_buf[0..kp_esc_len], escaped_code, modifiers);
                        }
                    } else if (ev_type == .Char) {
                        const kp_key = if (text.len > 0) text else key;
                        let kp_esc_buf: [256]u8 = [0; 256];
                        const kp_esc_len = escapeJsString(kp_key, &kp_esc_buf);
                        dispatchKeyJs(session, "keypress", kp_esc_buf[0..kp_esc_len], escaped_code, modifiers);
                    } else if (ev_type == .KeyUp) {
                        dispatchKeyJs(session, "keyup", escaped_key, escaped_code, modifiers);
                    }
                }
            }

            if (text.len > 0 && (ev_type == .Char || ev_type == .KeyDown || ev_type == .RawKeyDown)) {
                applyInputText(session, text, key);
            }
            if (ev_type == .KeyDown && (std.mem.eql(u8, key, "Backspace") || std.mem.eql(u8, key, "Delete"))) {
                applyInputText(session, "", key);
            }

            return CdpResponse.ok(req.id, "{}");
        }

        // 3. Text Insertion: insertText
        if (std.mem.eql(u8, req.method, "Input.insertText")) {
            const raw_text = extractJsonString(req.params_raw, "text") ?? "";
            let unesc_buf: [1024]u8 = [0; 1024];
            const text = unescapeJsonString(raw_text, &unesc_buf);
            applyInputText(session, text, "");
            return CdpResponse.ok(req.id, "{}");
        }

        // 4. Ignore Input Events flag: setIgnoreInputEvents
        if (std.mem.eql(u8, req.method, "Input.setIgnoreInputEvents")) {
            const ignore = extractJsonBool(req.params_raw, "ignore") ?? false;
            * = ignore;
            return CdpResponse.ok(req.id, "{}");
        }

        // 5. Generic fallback for any other Input domain methods (stubs)
        if (std.mem.startsWith(u8, req.method, "Input.")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.err(req.id, -32601, "'Input' domain method not implemented");
    }
}
