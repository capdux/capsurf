// src/cdp/input.cp
// CDP Input Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/input.ts

import { CdpRequest, CdpResponse, CdpEvent, MAX_CDP_EVENTS, extractJsonString, extractJsonInt, extractJsonFloat, extractJsonBool, extractJsonArray, unescapeJsonString } from "./types.cp";
import { CdpPageSession, escapeJsonString } from "./session.cp";
import { NodeId, NULL_NODE, NodeType } from "../dom/dom.cp";
import { HttpLoader } from "../net/http_loader.cp";
import { MockFixtures } from "../net/mock_fixtures.cp";
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
struct HitScrollOffset {
    node_id: NodeId = NULL_NODE,
    scroll_left: f32 = 0.0,
    scroll_top: f32 = 0.0,
}

pub fn hitTest(session: *CdpPageSession, x: f32, y: f32): NodeId {
    let sx: f32 = session.scroll_x;
    let sy: f32 = session.scroll_y;
    let scroll_offsets: [64]HitScrollOffset = [HitScrollOffset{}; 64];
    let scroll_offset_count: usize = 0;
    if (session.has_document) {
        const sval = session.ctx.eval("((window.scrollX||0)+','+(window.scrollY||0))");
        if (!jsIsException(sval)) {
            const sstr = session.ctx.toCString(sval);
            if (sstr.len > 0) {
                let it = std.mem.splitScalar(u8, sstr, ',');
                if (x_str := it.next()) {
                    if (vx := std.fmt.parseFloat(f32, x_str) catch null) {
                        sx = vx;
                        session.scroll_x = vx;
                    }
                }
                if (y_str := it.next()) {
                    if (vy := std.fmt.parseFloat(f32, y_str) catch null) {
                        sy = vy;
                        session.scroll_y = vy;
                    }
                }
            }
        }
        session.ctx.freeValue(sval);

        const js_sc = "(function() { try { const cache = globalThis.__capsurf_wrappers; if (!cache) return ''; const de = (typeof document !== 'undefined') ? document.documentElement : null; const out = []; const keys = Object.keys(cache); for (let i = 0; i < keys.length && out.length < 64; i++) { const el = cache[keys[i]]; if (!el || el === de || typeof el._nid !== 'number' || el._nid <= 0) continue; const st = Number(el._scrollTop) || 0; const sl = Number(el._scrollLeft) || 0; if (st !== 0 || sl !== 0) out.push(el._nid + ':' + sl + ':' + st); } return out.join(';'); } catch (e) { return ''; } })();";
        const val_sc = session.ctx.eval(js_sc);
        if (!jsIsException(val_sc)) {
            const cstr_sc = session.ctx.toCString(val_sc);
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
                                    scroll_offsets[scroll_offset_count] = HitScrollOffset{
                                        node_id: @as(NodeId, snid),
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
        }
        session.ctx.freeValue(val_sc);
    }
    let best_node: NodeId = NULL_NODE;
    let best_z: i32 = -2147483648;
    let best_pos: i32 = -1;
    const box_count = session.layout.boxes.items.len;
    const node_count = session.doc.nodes.items.len;
    if (session.doc.root_element != NULL_NODE && @as(usize, session.doc.root_element) < node_count) {
        let stack: [512]NodeId = [NULL_NODE; 512];
        let sp: usize = 0;
        stack[sp] = session.doc.root_element;
        sp += 1;
        let steps: usize = 0;
        while (sp > 0 && steps < 65536) : (steps += 1) {
            sp -= 1;
            const nid = stack[sp];
            const nidx = @as(usize, nid);
            if (nid == NULL_NODE || nidx >= node_count) continue;
            const nd = session.doc.nodes.items[nidx];
            if (nd.next_sibling != NULL_NODE && sp < 512) {
                stack[sp] = nd.next_sibling;
                sp += 1;
            }
            if (nd.first_child != NULL_NODE && sp < 512) {
                stack[sp] = nd.first_child;
                sp += 1;
            }
            if (nidx < box_count) {
                const box = session.layout.boxes.items[nidx];
                if (box.rect.visible && !box.pointer_events_none && box.rect.width > 0.0 && box.rect.height > 0.0) {
                    let anc_sx: f32 = 0.0;
                    let anc_sy: f32 = 0.0;
                    if (box.position != .Fixed && scroll_offset_count > 0 && box.node_id != NULL_NODE && @as(usize, box.node_id) < node_count) {
                        let cur: NodeId = session.doc.nodes.items[box.node_id].parent;
                        let depth: usize = 0;
                        while (cur != NULL_NODE && @as(usize, cur) < node_count && depth < 128) : (depth += 1) {
                            let si: usize = 0;
                            while (si < scroll_offset_count) : (si += 1) {
                                if (scroll_offsets[si].node_id == cur) {
                                    anc_sx += scroll_offsets[si].scroll_left;
                                    anc_sy += scroll_offsets[si].scroll_top;
                                    break;
                                }
                            }
                            cur = session.doc.nodes.items[cur].parent;
                        }
                    }
                    const tx = if (box.position == .Fixed) x else x + sx + anc_sx;
                    const ty = if (box.position == .Fixed) y else y + sy + anc_sy;
                    if (tx >= box.rect.x && tx <= (box.rect.x + box.rect.width) &&
                        ty >= box.rect.y && ty <= (box.rect.y + box.rect.height)) {
                        const bz: i32 = if (box.has_z_index) box.z_index else 0;
                        const bpos: i32 = if (box.position != .Static) 1 else 0;
                        if (best_node == NULL_NODE || bz > best_z || (bz == best_z && bpos >= best_pos)) {
                            best_node = box.node_id;
                            best_z = bz;
                            best_pos = bpos;
                        }
                    }
                }
            }
        }
    } else {
        let i: usize = 0;
        while (i < box_count) : (i += 1) {
            const box = session.layout.boxes.items[i];
            if (box.rect.visible && !box.pointer_events_none && box.rect.width > 0.0 && box.rect.height > 0.0) {
                let anc_sx: f32 = 0.0;
                let anc_sy: f32 = 0.0;
                if (box.position != .Fixed && scroll_offset_count > 0 && box.node_id != NULL_NODE && @as(usize, box.node_id) < node_count) {
                    let cur: NodeId = session.doc.nodes.items[box.node_id].parent;
                    let depth: usize = 0;
                    while (cur != NULL_NODE && @as(usize, cur) < node_count && depth < 128) : (depth += 1) {
                        let si: usize = 0;
                        while (si < scroll_offset_count) : (si += 1) {
                            if (scroll_offsets[si].node_id == cur) {
                                anc_sx += scroll_offsets[si].scroll_left;
                                anc_sy += scroll_offsets[si].scroll_top;
                                break;
                            }
                        }
                        cur = session.doc.nodes.items[cur].parent;
                    }
                }
                const tx = if (box.position == .Fixed) x else x + sx + anc_sx;
                const ty = if (box.position == .Fixed) y else y + sy + anc_sy;
                if (tx >= box.rect.x && tx <= (box.rect.x + box.rect.width) &&
                    ty >= box.rect.y && ty <= (box.rect.y + box.rect.height)) {
                    const bz: i32 = if (box.has_z_index) box.z_index else 0;
                    const bpos: i32 = if (box.position != .Static) 1 else 0;
                    if (best_node == NULL_NODE || bz > best_z || (bz == best_z && bpos >= best_pos)) {
                        best_node = box.node_id;
                        best_z = bz;
                        best_pos = bpos;
                    }
                }
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
        if (c == '\n') {
            if (ei + 1 < out.len) {
                out[ei] = '\\';
                out[ei + 1] = 'n';
            }
            ei += 2;
            continue;
        }
        if (c == '\r') {
            if (ei + 1 < out.len) {
                out[ei] = '\\';
                out[ei + 1] = 'r';
            }
            ei += 2;
            continue;
        }
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

fn dispatchKeyJs(session: *CdpPageSession, type_str_js: []const u8, escaped_key: []const u8, escaped_code: []const u8, modifiers: i64, is_repeat: bool) {
    let js_buf: [1024]u8 = [0; 1024];
    const rep_i: i32 = if (is_repeat) 1 else 0;
    const preserve_scroll = std.mem.eql(u8, escaped_key, "Tab") && session.facade.stream_mgr.isTerminalActive();
    const js_expr = if (preserve_scroll)
        std.fmt.bufPrint(
            &js_buf,
            "(() => {{ var sx=window.scrollX||0,sy=window.scrollY||0; var r=0; if (typeof __capsurf_dispatch_key_event === 'function') {{ r=__capsurf_dispatch_key_event('{s}', '{s}', '{s}', {d}, {d}); }} if((window.scrollX||0)!==sx||(window.scrollY||0)!==sy)window.scrollTo(sx,sy); return r; }})()",
            .{ type_str_js, escaped_key, escaped_code, modifiers, rep_i },
        ) catch null
    else
        std.fmt.bufPrint(
            &js_buf,
            "(() => {{ if (typeof __capsurf_dispatch_key_event === 'function') {{ return __capsurf_dispatch_key_event('{s}', '{s}', '{s}', {d}, {d}); }} return 0; }})()",
            .{ type_str_js, escaped_key, escaped_code, modifiers, rep_i },
        ) catch null;
    if (e := js_expr) {
        const val = session.ctx.eval(e);
        if (val.tag == 0 && val.u.uint64 > 0) {
            session.focused_node = @as(u32, @truncate(val.u.uint64));
            session.doc.focused_node = session.focused_node;
        }
        session.ctx.freeValue(val);
        session.ctx.drainJobs();
    }
}

pub fn applyInputText(session: *CdpPageSession, text: []const u8, key: []const u8, modifiers: i64, is_select_all: bool) {
    if (text.len == 0 && key.len == 0) return;

    // Filter and escape text and key
    let clean_buf: [1024]u8 = [0; 1024];
    let clean_len: usize = 0;
    for (c of text) {
        if ((c >= 32 || c == '\n') && c != 127 && clean_len < clean_buf.len) {
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
        const has_shift: i32 = if ((modifiers & 8) != 0) 1 else 0;
        const has_ctrl_meta: i32 = if ((modifiers & 6) != 0) 1 else 0;
        const has_word_mod: i32 = if ((modifiers & 3) != 0) 1 else 0;
        const sel_all_i: i32 = if (is_select_all) 1 else 0;
        let js_buf: [12288]u8 = [0; 12288];
        const js_expr = std.fmt.bufPrint(
            &js_buf,
            "(() => {{ const _selAll = {d}; const _ctrlMeta = {d}; const _shiftMod = {d}; const _wordMod = {d}; const _k = '{s}'; const _t = '{s}'; let el = document.activeElement; while (el && (el.shadowRoot || el._shadowRoot) && (el.shadowRoot || el._shadowRoot).activeElement) {{ el = (el.shadowRoot || el._shadowRoot).activeElement; }} if (!el || el === document.body || el === document.documentElement) {{ if ({d} > 0) {{ function f(r, nid) {{ if (!r) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; if (all[i]._shadowRoot) {{ const sh = f(all[i]._shadowRoot, nid); if (sh) return sh; }} }} return null; }} el = f(document, {d}); }} }} if (!el || el === document.body || el === document.documentElement) return 0; if (typeof globalThis !== 'undefined' && globalThis.__capsurf_last_key_prevented && (_t.length > 0 || _selAll === 1)) return (typeof el._nid === 'number') ? el._nid : 0; if (el.isContentEditable) {{ const sel = typeof window !== 'undefined' && window.getSelection ? window.getSelection() : null; if (_selAll === 1) {{ const tlen = (el.textContent || '').length; el._ceAnchor = 0; el._ceFocus = tlen; if (sel && document.createRange) {{ const r = document.createRange(); r.selectNodeContents(el); sel.removeAllRanges(); sel.addRange(r); }} return (typeof el._nid === 'number') ? el._nid : 0; }} if (_ctrlMeta === 1 && (_k === 'c' || _k === 'C')) {{ globalThis.__capsurf_clipboard_text = sel ? String(sel.toString() || '') : ''; return (typeof el._nid === 'number') ? el._nid : 0; }} if (_ctrlMeta === 1 && (_k === 'x' || _k === 'X')) {{ const st = sel ? String(sel.toString() || '') : ''; globalThis.__capsurf_clipboard_text = st; if (st.length > 0) {{ el.textContent = ''; el._ceAnchor = 0; el._ceFocus = 0; if (sel) sel.removeAllRanges(); if (typeof el.dispatchEvent === 'function') {{ try {{ el.dispatchEvent(new Event('input', {{ bubbles: true }})); }} catch(e) {{}} }} }} return (typeof el._nid === 'number') ? el._nid : 0; }} if (_ctrlMeta === 1 && (_k === 'v' || _k === 'V')) {{ const clip = String(globalThis.__capsurf_clipboard_text || ''); const oldTxt = el.textContent || ''; let nextTxt = oldTxt; let caret = typeof el._ceFocus === 'number' ? Math.max(0, Math.min(el._ceFocus, oldTxt.length)) : 0; if (sel && sel.rangeCount > 0 && !sel.isCollapsed) {{ nextTxt = clip; caret = clip.length; sel.removeAllRanges(); }} else {{ nextTxt = oldTxt.slice(0, caret) + clip + oldTxt.slice(caret); caret += clip.length; }} if (nextTxt !== oldTxt) {{ el.textContent = nextTxt; el._ceAnchor = caret; el._ceFocus = caret; if (typeof el.dispatchEvent === 'function') {{ try {{ el.dispatchEvent(new Event('input', {{ bubbles: true }})); }} catch(e) {{}} }} }} return (typeof el._nid === 'number') ? el._nid : 0; }} if (_k === 'ArrowLeft' || _k === 'ArrowRight') {{ const txt = el.textContent || ''; const tlen = txt.length; let a = typeof el._ceAnchor === 'number' ? Math.min(el._ceAnchor, tlen) : 0; let f = typeof el._ceFocus === 'number' ? Math.min(el._ceFocus, tlen) : 0; if (_k === 'ArrowLeft') {{ if (_wordMod === 1) {{ while (f > 0 && /\\s/.test(txt[f - 1])) f--; while (f > 0 && !/\\s/.test(txt[f - 1])) f--; }} else {{ f = f > 0 ? f - 1 : 0; }} }} else {{ if (_wordMod === 1) {{ while (f < tlen && !/\\s/.test(txt[f])) f++; while (f < tlen && /\\s/.test(txt[f])) f++; }} else {{ f = f < tlen ? f + 1 : tlen; }} }} if (_shiftMod !== 1) a = f; el._ceAnchor = a; el._ceFocus = f; if (sel && document.createRange) {{ const tn = el.firstChild; if (tn && tn.nodeType === 3) {{ const r = document.createRange(); r.setStart(tn, Math.min(a, f)); r.setEnd(tn, Math.max(a, f)); sel.removeAllRanges(); sel.addRange(r); }} }} return (typeof el._nid === 'number') ? el._nid : 0; }} const oldTxt = el.textContent || ''; let nextTxt = oldTxt; let caret = typeof el._ceFocus === 'number' ? Math.max(0, Math.min(el._ceFocus, oldTxt.length)) : 0; if (_k === 'Backspace') {{ if (sel && sel.rangeCount > 0 && !sel.isCollapsed) {{ nextTxt = ''; caret = 0; sel.removeAllRanges(); }} else if (caret > 0) {{ nextTxt = oldTxt.slice(0, caret - 1) + oldTxt.slice(caret); caret -= 1; }} }} else if (_t.length > 0) {{ if (_k === '') {{ if (sel && sel.rangeCount > 0 && !sel.isCollapsed) {{ nextTxt = _t; caret = _t.length; sel.removeAllRanges(); }} else {{ nextTxt = oldTxt.slice(0, caret) + _t + oldTxt.slice(caret); caret += _t.length; }} }} else {{ el._ceFocus = oldTxt.length; el._ceAnchor = oldTxt.length; }} }} if (nextTxt !== oldTxt) {{ el.textContent = nextTxt; el._ceAnchor = caret; el._ceFocus = caret; if (typeof el.dispatchEvent === 'function') {{ try {{ el.dispatchEvent(new Event('input', {{ bubbles: true }})); }} catch(e) {{}} }} }} return (typeof el._nid === 'number') ? el._nid : 0; }} const getRaw = (typeof getValueRaw === 'function') ? getValueRaw : (e) => (e._value !== undefined ? e._value : (e.value || '')); const setRaw = (typeof setValueRaw === 'function') ? setValueRaw : (e, v) => {{ e._value = String(v); if (e.setAttribute) e.setAttribute('value', String(v)); }}; const oldVal = getRaw(el); const oldLen = oldVal.length; const isNum = String(el.type || '').toLowerCase() === 'number'; if ({d} === 1) {{ if (isNum) el._numSelAll = true; if (typeof el.select === 'function') el.select(); else if (typeof el.setSelectionRange === 'function') el.setSelectionRange(0, oldLen); el._selDir = 'forward'; return (typeof el._nid === 'number') ? el._nid : 0; }} const start = typeof el.selectionStart === 'number' ? el.selectionStart : (isNum ? (el._numSelAll ? 0 : (typeof el._numCaret === 'number' ? el._numCaret : 0)) : oldLen); const end = typeof el.selectionEnd === 'number' ? el.selectionEnd : (isNum && el._numSelAll ? oldLen : start); const dir = el._selDir || 'forward'; let nextVal = oldVal; let cursor = start; const isPrintable = '{s}'.length > 0; if (isPrintable) {{ if (isNum) el._numSelAll = false; nextVal = oldVal.slice(0, start) + '{s}' + oldVal.slice(end); cursor = start + '{s}'.length; el._selDir = 'forward'; }} else if ('{s}' === 'Backspace') {{ if (isNum) el._numSelAll = false; if (start !== end) {{ nextVal = oldVal.slice(0, start) + oldVal.slice(end); cursor = start; }} else if (start > 0) {{ nextVal = oldVal.slice(0, start - 1) + oldVal.slice(end); cursor = start - 1; }} el._selDir = 'forward'; }} else if ('{s}' === 'Delete') {{ if (isNum) el._numSelAll = false; if (start !== end) {{ nextVal = oldVal.slice(0, start) + oldVal.slice(end); cursor = start; }} else if (start < oldLen) {{ nextVal = oldVal.slice(0, start) + oldVal.slice(start + 1); cursor = start; }} el._selDir = 'forward'; }} else if ('{s}' === 'ArrowLeft') {{ if (isNum) el._numSelAll = false; if ({d} === 1) {{ let a = dir === 'backward' ? end : start; let f = dir === 'backward' ? start : end; if (start === end) {{ a = start; f = start; }} const nf = f > 0 ? f - 1 : 0; const ns = Math.min(a, nf); const ne = Math.max(a, nf); el._selDir = nf < a ? 'backward' : 'forward'; if (typeof el.setSelectionRange === 'function') el.setSelectionRange(ns, ne, el._selDir); }} else {{ cursor = (start !== end) ? start : (start > 0 ? start - 1 : 0); if (isNum) el._numCaret = cursor; el._selDir = 'forward'; if (typeof el.setSelectionRange === 'function') el.setSelectionRange(cursor, cursor); }} }} else if ('{s}' === 'ArrowRight') {{ if (isNum) el._numSelAll = false; if ({d} === 1) {{ let a = dir === 'backward' ? end : start; let f = dir === 'backward' ? start : end; if (start === end) {{ a = start; f = start; }} const nf = f < oldLen ? f + 1 : oldLen; const ns = Math.min(a, nf); const ne = Math.max(a, nf); el._selDir = nf < a ? 'backward' : 'forward'; if (typeof el.setSelectionRange === 'function') el.setSelectionRange(ns, ne, el._selDir); }} else {{ cursor = (start !== end) ? end : (end < oldLen ? end + 1 : oldLen); if (isNum) el._numCaret = cursor; el._selDir = 'forward'; if (typeof el.setSelectionRange === 'function') el.setSelectionRange(cursor, cursor); }} }} else if ('{s}' === 'Home') {{ if (isNum) el._numSelAll = false; if ({d} === 1) {{ const a = dir === 'backward' ? end : start; el._selDir = 'backward'; if (typeof el.setSelectionRange === 'function') el.setSelectionRange(0, a, 'backward'); }} else {{ if (isNum) el._numCaret = 0; el._selDir = 'forward'; if (typeof el.setSelectionRange === 'function') el.setSelectionRange(0, 0); }} }} else if ('{s}' === 'End') {{ if (isNum) el._numSelAll = false; if ({d} === 1) {{ const a = dir === 'backward' ? end : start; el._selDir = 'forward'; if (typeof el.setSelectionRange === 'function') el.setSelectionRange(a, oldLen, 'forward'); }} else {{ if (isNum) el._numCaret = oldLen; el._selDir = 'forward'; if (typeof el.setSelectionRange === 'function') el.setSelectionRange(oldLen, oldLen); }} }} if (nextVal !== oldVal) {{ setRaw(el, nextVal); if (isNum) el._numCaret = cursor; if (typeof notifyControlValueMutation === 'function') {{ notifyControlValueMutation(el, cursor); }} else {{ if (typeof el.setSelectionRange === 'function') el.setSelectionRange(cursor, cursor); if (typeof el.dispatchEvent === 'function') {{ try {{ const InEv = typeof InputEvent === 'function' ? InputEvent : Event; el.dispatchEvent(new InEv('input', {{ bubbles: true, cancelable: false }})); }} catch(e) {{}} }} }} }} return (typeof el._nid === 'number') ? el._nid : 0; }})()",
            .{ sel_all_i, has_ctrl_meta, has_shift, has_word_mod, escaped_key, escaped_text, session.focused_node, session.focused_node, sel_all_i, escaped_text, escaped_text, escaped_text, escaped_key, escaped_key, escaped_key, has_shift, escaped_key, has_shift, escaped_key, has_shift, escaped_key, has_shift },
        ) catch null;
        if (e := js_expr) {
            const val = session.ctx.eval(e);
            if (val.tag == 0 && val.u.uint64 > 0) {
                session.focused_node = @as(u32, @truncate(val.u.uint64));
            }
            session.ctx.freeValue(val);
        }
    }

    if (session.focused_node != NULL_NODE && @as(usize, session.focused_node) < session.doc.nodes.items.len) {
        const old_val = session.doc.getAttribute(session.focused_node, "value") ?? "";
        if (clean_text.len > 0) {
            const next_val = std.fmt.allocPrint(session.doc.allocator, "{s}{s}", .{ old_val, clean_text }) catch null;
            if (nv := next_val) {
                session.doc.setAttribute(session.focused_node, "value", nv);
            }
        } else if (std.mem.eql(u8, key, "Backspace") && old_val.len > 0) {
            session.doc.setAttribute(session.focused_node, "value", old_val[0..old_val.len - 1]);
        }
    }
}

fn dispatchMouseDomEvent(session: *CdpPageSession, target_node: NodeId, ev_name: []const u8, x: f32, y: f32, detail: i64, btn_num: i32, buttons_mask: i64, modifiers: i64, has_force: bool, force_val: f32): bool {
    if (!session.has_document) return true;
    const eff_detail = if (detail > 0) detail else 1;
    const use_force_i: i32 = if (has_force) 1 else 0;
    const p_name: []const u8 = if (std.mem.eql(u8, ev_name, "mousedown"))
        "pointerdown"
    else if (std.mem.eql(u8, ev_name, "mouseup"))
        "pointerup"
    else if (std.mem.eql(u8, ev_name, "mousemove"))
        "pointermove"
    else if (std.mem.eql(u8, ev_name, "mouseover"))
        "pointerover"
    else if (std.mem.eql(u8, ev_name, "mouseenter"))
        "pointerenter"
    else if (std.mem.eql(u8, ev_name, "mouseout"))
        "pointerout"
    else if (std.mem.eql(u8, ev_name, "mouseleave"))
        "pointerleave"
    else
        "";
    const ev_bubbles = !(std.mem.eql(u8, ev_name, "mouseenter") || std.mem.eql(u8, ev_name, "mouseleave") || std.mem.eql(u8, ev_name, "pointerenter") || std.mem.eql(u8, ev_name, "pointerleave"));
    const is_leave = std.mem.eql(u8, ev_name, "mouseout") || std.mem.eql(u8, ev_name, "mouseleave") || std.mem.eql(u8, ev_name, "pointerout") || std.mem.eql(u8, ev_name, "pointerleave");
    const prefer_nid_i: i32 = if (is_leave && target_node > 0) 1 else 0;
    const bubbles_js: []const u8 = if (ev_bubbles) "true" else "false";
    let js_buf: [8192]u8 = [0; 8192];
    const js_expr = std.fmt.bufPrint(
        &js_buf,
        "(() => {{ function f(r, nid) {{ if (!r || nid <= 0) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; }} return null; }} const byNid = f(document, {d}); let el = null; if ({d} !== 0 && byNid) {{ el = byNid; }} else {{ const byPt = (typeof document.elementFromPoint === 'function') ? document.elementFromPoint({d:.2}, {d:.2}) : null; el = (byPt && byPt !== document.body && byPt !== document.documentElement) ? ((byNid && byNid !== byPt && typeof byPt.contains === 'function' && byPt.contains(byNid)) ? byNid : byPt) : (byNid || byPt || document.body || document.documentElement); while (el && el._shadowRoot && typeof el._shadowRoot.elementFromPoint === 'function') {{ const inner = el._shadowRoot.elementFromPoint({d:.2}, {d:.2}); if (!inner || inner === el) break; el = inner; }} }} let cx = {d:.2}; let cy = {d:.2}; let doc = document; let win = window; let depth = 0; while (el && el.nodeType === 1 && depth < 16) {{ const tag = String(el.localName || el.tagName || '').toLowerCase(); if (tag !== 'iframe' && tag !== 'frame') break; let cdoc = null; try {{ cdoc = el.contentDocument || (el.contentWindow && el.contentWindow.document) || null; }} catch (e) {{}} if (!cdoc || typeof el.getBoundingClientRect !== 'function') break; const r = el.getBoundingClientRect(); if (!r) break; let ox = r.left || 0; let oy = r.top || 0; if (typeof win.getComputedStyle === 'function') {{ try {{ const cs = win.getComputedStyle(el); if (cs) {{ ox += (parseFloat(cs.borderLeftWidth) || 0) + (parseFloat(cs.paddingLeft) || 0); oy += (parseFloat(cs.borderTopWidth) || 0) + (parseFloat(cs.paddingTop) || 0); }} }} catch (e) {{}} }} cx -= ox; cy -= oy; doc = cdoc; win = cdoc.defaultView || el.contentWindow || win; let inner = (typeof cdoc.elementFromPoint === 'function') ? cdoc.elementFromPoint(cx, cy) : null; if (!inner) {{ const all = cdoc.querySelectorAll ? cdoc.querySelectorAll('*') : []; for (let i = all.length - 1; i >= 0; i--) {{ const it = all[i]; if (!it || typeof it.getBoundingClientRect !== 'function') continue; const ir = it.getBoundingClientRect(); if (ir && ir.width > 0 && ir.height > 0 && cx >= ir.left && cx <= ir.left + ir.width && cy >= ir.top && cy <= ir.top + ir.height) {{ inner = it; break; }} }} }} if (!inner) inner = cdoc.body || cdoc.documentElement; if (!inner) break; el = inner; while (el && el._shadowRoot && typeof el._shadowRoot.elementFromPoint === 'function') {{ const sh = el._shadowRoot.elementFromPoint(cx, cy); if (!sh || sh === el) break; el = sh; }} depth++; }} if (!el || typeof el.dispatchEvent !== 'function') return 1; const fx = Math.floor(cx); const fy = Math.floor(cy); const m = {d}; const altK = (m & 1) !== 0; const ctrlK = (m & 2) !== 0; const metaK = (m & 4) !== 0; const shiftK = (m & 8) !== 0; const disp = (e) => (typeof globalThis.__capsurf_dispatchTrustedEvent === 'function') ? globalThis.__capsurf_dispatchTrustedEvent(el, e) : el.dispatchEvent(e); const pName = '{s}'; if (pName) {{ const pPress = (pName === 'pointerup') ? 0 : ({d} !== 0 ? {d:.4} : (({d} !== 0 || pName === 'pointerdown') ? 0.5 : 0)); const pBubbles = (pName !== 'pointerenter' && pName !== 'pointerleave'); const PCtor = (win && typeof win.PointerEvent === 'function') ? win.PointerEvent : ((typeof PointerEvent === 'function') ? PointerEvent : ((typeof MouseEvent === 'function') ? MouseEvent : Event)); disp(new PCtor(pName, {{ bubbles: pBubbles, cancelable: true, composed: true, clientX: fx, clientY: fy, button: {d}, buttons: {d}, pressure: pPress, altKey: altK, ctrlKey: ctrlK, metaKey: metaK, shiftKey: shiftK, pointerId: 1, pointerType: 'mouse', isPrimary: true }})); }} const Ctor = (win && typeof win.MouseEvent === 'function') ? win.MouseEvent : ((typeof MouseEvent === 'function') ? MouseEvent : Event); const ev = new Ctor('{s}', {{ bubbles: {s}, cancelable: true, composed: true, clientX: fx, clientY: fy, button: {d}, buttons: {d}, altKey: altK, ctrlKey: ctrlK, metaKey: metaK, shiftKey: shiftK, detail: {d} }}); const ok = disp(ev); if ('{s}' === 'mousedown' && ok && !ev.defaultPrevented) {{ let cur = el; while (cur && cur.nodeType === 1) {{ if (typeof cur.focus === 'function') {{ const prev = doc.activeElement; cur.focus({{ preventScroll: true }}); if (doc.activeElement !== prev || cur === prev) break; }} cur = cur.parentNode; }} }} if ('{s}' === 'click' && ok && !ev.defaultPrevented && typeof globalThis.__capsurf_activate_clicked_element === 'function') {{ globalThis.__capsurf_activate_clicked_element(el, true, fx, fy, {d}); }} return (ok && !ev.defaultPrevented) ? 1 : 0; }})()",
        .{ target_node, prefer_nid_i, x, y, x, y, x, y, modifiers, p_name, use_force_i, force_val, buttons_mask, btn_num, buttons_mask, ev_name, bubbles_js, btn_num, buttons_mask, eff_detail, ev_name, ev_name, eff_detail },
    ) catch return true;
    const val = session.ctx.eval(js_expr);
    let not_prevented = true;
    if (val.tag == 0 && val.u.uint64 == 0) {
        not_prevented = false;
    }
    session.ctx.freeValue(val);
    session.ctx.drainJobs();
    return not_prevented;
}

fn updateRangeFromX(session: *CdpPageSession, node_id: NodeId, x: f32): bool {
    if (!session.has_document || node_id == NULL_NODE) return false;
    const rect = session.layout.getBoundingClientRect(node_id);
    if (rect.width <= 0.0) return false;
    const ratio = std.math.clamp((x - rect.x) / rect.width, 0.0, 1.0);
    let js_buf: [2048]u8 = [0; 2048];
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
    if (pending_raw.len == 0 || pending_raw[0] == '#' || std.ascii.startsWithIgnoreCase(pending_raw, "javascript:")) return;

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

pub struct InputState {
    ctx_key: usize = 0,
    has_drag_dt: bool = false,
    drag_over_default_prevented: bool = false,
}

let g_input_states: [16]InputState = [InputState{}; 16];
let g_input_state_next: usize = 0;

fn getInputState(ctx_key: usize): *InputState {
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        if (g_input_states[i].ctx_key == ctx_key) {
            return &g_input_states[i];
        }
    }
    let j: usize = 0;
    while (j < 16) : (j += 1) {
        if (g_input_states[j].ctx_key == 0) {
            g_input_states[j] = InputState{ ctx_key: ctx_key, has_drag_dt: false, drag_over_default_prevented: false };
            return &g_input_states[j];
        }
    }
    const slot = g_input_state_next % 16;
    g_input_state_next += 1;
    g_input_states[slot] = InputState{ ctx_key: ctx_key, has_drag_dt: false, drag_over_default_prevented: false };
    return &g_input_states[slot];
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
            const buttons_opt = extractJsonInt(req.params_raw, "buttons");
            const force_opt = extractJsonFloat(req.params_raw, "force");
            const has_force = force_opt != null;
            const force_val = force_opt ?? 0.0;
            const btn_mask: i32 = switch (button) {
                .Left => 1,
                .Right => 2,
                .Middle => 4,
                .Back => 8,
                .Forward => 16,
                * => 0,
            };
            let buttons: i64 = 0;
            if (ev_type == .MousePressed) {
                const eff: i32 = if (b := buttons_opt) (@as(i32, @truncate(b)) | btn_mask) else (session.mouse_buttons | btn_mask);
                session.mouse_buttons = eff;
                buttons = @as(i64, eff);
            } else if (ev_type == .MouseReleased) {
                const eff: i32 = if (b := buttons_opt) (@as(i32, @truncate(b)) & ~btn_mask) else (session.mouse_buttons & ~btn_mask);
                session.mouse_buttons = eff;
                buttons = @as(i64, eff);
            } else if (ev_type == .MouseMoved) {
                const eff: i32 = if (b := buttons_opt) @as(i32, @truncate(b)) else session.mouse_buttons;
                if (buttons_opt != null) session.mouse_buttons = eff;
                buttons = @as(i64, eff);
            } else {
                buttons = buttons_opt ?? 0;
            }
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

            // Handle mouseWheel: dispatch WheelEvent + scroll chaining + overscroll-behavior
            if (ev_type == .MouseWheel) {
                if (session.has_document) {
                    let wh_buf: [512]u8 = [0; 512];
                    const wh_js = std.fmt.bufPrint(
                        &wh_buf,
                        "(() => {{ if (typeof __capsurf_dispatch_wheel_event === 'function') {{ const r = __capsurf_dispatch_wheel_event({d:.4}, {d:.4}, {d:.4}, {d:.4}, {d}); if (r) return r[0] + ',' + r[1]; }} return ''; }})()",
                        .{ x, y, delta_x, delta_y, modifiers },
                    ) catch null;
                    if (wjs := wh_js) {
                        const wval = session.ctx.eval(wjs);
                        if (!jsIsException(wval)) {
                            const ws = session.ctx.toCString(wval);
                            if (ws.len > 0) {
                                let it = std.mem.splitScalar(u8, ws, ',');
                                if (sx_str := it.next()) {
                                    if (sx_val := std.fmt.parseFloat(f32, sx_str) catch null) {
                                        session.scroll_x = sx_val;
                                    }
                                }
                                if (sy_str := it.next()) {
                                    if (sy_val := std.fmt.parseFloat(f32, sy_str) catch null) {
                                        session.scroll_y = sy_val;
                                    }
                                }
                            }
                        }
                        session.ctx.freeValue(wval);
                        session.ctx.drainJobs();
                    }
                } else {
                    session.scroll_x += delta_x;
                    session.scroll_y += delta_y;
                }
            }

            const btn_num: i32 = switch (button) {
                .Left => 0,
                .Middle => 1,
                .Right => 2,
                .Back => 3,
                .Forward => 4,
                * => 0,
            };

            // Hit test and dispatch mouse / range drag events
            if (ev_type == .MousePressed) {
                if (session.layout.boxes.items.len == 0) {
                    session.ensureLayout();
                } else if (dctx := session.dom_ctx) {
                    if (dctx.layout_dirty) {
                        session.ensureLayout();
                    }
                }
                const target_node = hitTest(session, x, y);
                const not_prevented = dispatchMouseDomEvent(session, target_node, "mousedown", x, y, click_count, btn_num, buttons, modifiers, has_force, force_val);
                if (btn_num == 2) {
                    * = dispatchMouseDomEvent(session, target_node, "contextmenu", x, y, click_count, btn_num, buttons, modifiers, has_force, force_val);
                }
                if (target_node != NULL_NODE) {
                    session.focused_node = target_node;
                    session.doc.focused_node = target_node;
                    if (not_prevented && @as(usize, target_node) < session.doc.nodes.items.len) {
                        const tn = &session.doc.nodes.items[@as(usize, target_node)];
                        if (tn.hasTag("input") && std.ascii.eqlIgnoreCase(tn.getAttribute("type") ?? "", "range")) {
                            session.range_drag_node = target_node;
                            session.range_drag_changed = updateRangeFromX(session, target_node, x);
                        }
                    }
                }
                if (session.has_document && button == .Left && not_prevented && session.range_drag_node == NULL_NODE) {
                    let sel_buf: [4096]u8 = [0; 4096];
                    const sel_js = std.fmt.bufPrint(
                        &sel_buf,
                        "(() => {{ const cc = {d}; function f(r, nid) {{ if (!r || nid <= 0) return null; if (r._nid === nid) return r; const cs = r.childNodes || []; for (let i = 0; i < cs.length; i++) {{ const h = f(cs[i], nid); if (h) return h; }} return null; }} function firstText(n) {{ if (!n) return null; if (n.nodeType === 3) return n; const cs = n.childNodes || []; for (let i = 0; i < cs.length; i++) {{ const t = firstText(cs[i]); if (t) return t; }} return null; }} function wordBounds(str, off) {{ let s = Math.min(off, str.length); let e = s; while (s > 0 && /\\w/.test(str[s - 1])) s--; while (e < str.length && /\\w/.test(str[e])) e++; return [s, e]; }} let el = f(document, {d}); if (!el && typeof document.elementFromPoint === 'function') el = document.elementFromPoint({d:.2}, {d:.2}); if (!el) el = document.body || document.documentElement; if (!el) return; const tag = String(el.tagName || '').toUpperCase(); const internals = globalThis[Symbol.for('kitesurf.internals')]; const cw = (internals && typeof internals.ttyCellWidth === 'number' && internals.ttyCellWidth > 0) ? internals.ttyCellWidth : 8; if (tag === 'INPUT' || tag === 'TEXTAREA') {{ const r = el.getBoundingClientRect ? el.getBoundingClientRect() : {{ left: 0, top: 0 }}; const val = String(el.value || ''); const startOff = Math.max(0, Math.min(val.length, Math.floor(({d:.2} - r.left) / cw))); if (cc >= 3) {{ globalThis.__capsurf_drag_sel = null; if (typeof el.setSelectionRange === 'function') el.setSelectionRange(0, val.length); return; }} if (cc === 2) {{ globalThis.__capsurf_drag_sel = null; const [ws, we] = wordBounds(val, startOff); if (typeof el.setSelectionRange === 'function') el.setSelectionRange(ws, we); return; }} globalThis.__capsurf_drag_sel = {{ ctrl: el, startOff: startOff, left: r.left, top: r.top || 0, cw: cw }}; if (typeof el.setSelectionRange === 'function') el.setSelectionRange(startOff, startOff); return; }} if (tag === 'SELECT' || tag === 'BUTTON') return; const tn = firstText(el); if (!tn) return; const boxEl = tn.parentNode || el; const r = (boxEl && boxEl.getBoundingClientRect) ? boxEl.getBoundingClientRect() : {{ left: 0 }}; const txt = tn.nodeValue || tn.textContent || ''; const len = txt.length; const startOff = Math.max(0, Math.min(len, Math.floor(({d:.2} - r.left) / cw))); if (cc >= 3 && typeof getSelection === 'function' && document.createRange) {{ globalThis.__capsurf_drag_sel = null; const range = document.createRange(); range.selectNodeContents(boxEl); const sel = getSelection(); if (sel) {{ sel.removeAllRanges(); sel.addRange(range); }} return; }} if (cc === 2 && typeof getSelection === 'function' && document.createRange) {{ globalThis.__capsurf_drag_sel = null; const [ws, we] = wordBounds(txt, startOff); const range = document.createRange(); range.setStart(tn, ws); range.setEnd(tn, we); const sel = getSelection(); if (sel) {{ sel.removeAllRanges(); sel.addRange(range); }} return; }} globalThis.__capsurf_drag_sel = {{ tn: tn, startOff: startOff, left: r.left, cw: cw }}; if (typeof getSelection === 'function') {{ const sel = getSelection(); if (sel && sel.removeAllRanges) sel.removeAllRanges(); }} }})()",
                        .{ click_count, target_node, x, y, x, x },
                    ) catch null;
                    if (sjs := sel_js) {
                        const sval = session.ctx.eval(sjs);
                        session.ctx.freeValue(sval);
                    }
                }
            } else if (ev_type == .MouseMoved) {
                if (session.layout.boxes.items.len == 0) {
                    session.ensureLayout();
                }
                const prev_hover = session.doc.hovered_node;
                let mv_target = hitTest(session, x, y);
                while (mv_target != NULL_NODE && @as(usize, mv_target) < session.doc.nodes.items.len &&
                       session.doc.nodes.items[@as(usize, mv_target)].node_type != NodeType.Element) {
                    mv_target = session.doc.nodes.items[@as(usize, mv_target)].parent;
                }
                if (session.has_document) {
                    let is_root_or_null = (mv_target == NULL_NODE);
                    if (!is_root_or_null && @as(usize, mv_target) < session.doc.nodes.items.len) {
                        const mn = &session.doc.nodes.items[@as(usize, mv_target)];
                        if (mn.hasTag("html") || mn.hasTag("body")) {
                            is_root_or_null = true;
                        }
                    }
                    if (is_root_or_null) {
                        let efp_buf: [512]u8 = [0; 512];
                        const efp_js = std.fmt.bufPrint(
                            &efp_buf,
                            "(() => {{ if (typeof document === 'undefined' || typeof document.elementFromPoint !== 'function') return 0; let el = document.elementFromPoint({d:.2}, {d:.2}); while (el && el._shadowRoot && typeof el._shadowRoot.elementFromPoint === 'function') {{ const inner = el._shadowRoot.elementFromPoint({d:.2}, {d:.2}); if (!inner || inner === el) break; el = inner; }} return (el && typeof el._nid === 'number' && el._nid > 0) ? el._nid : 0; }})()",
                            .{ x, y, x, y },
                        ) catch "";
                        if (efp_js.len > 0) {
                            const eval_v = session.ctx.eval(efp_js);
                            if (eval_v.tag == 0 && eval_v.u.uint64 > 0) {
                                mv_target = @as(NodeId, @truncate(eval_v.u.uint64));
                            }
                            session.ctx.freeValue(eval_v);
                        }
                    }
                }
                session.doc.hovered_node = mv_target;
                if (mv_target != prev_hover) {
                    if (session.has_document) {
                        let hov_buf: [64]u8 = [0; 64];
                        const hov_js = std.fmt.bufPrint(&hov_buf, "globalThis.__capsurf_hovered_nid = {d};", .{mv_target}) catch "";
                        if (hov_js.len > 0) {
                            const hv = session.ctx.eval(hov_js);
                            session.ctx.freeValue(hv);
                        }
                    }
                    if (dctx := session.dom_ctx) {
                        dctx.layout_dirty = true;
                    }
                    session.ensureLayout();
                    if (prev_hover != NULL_NODE) {
                        * = dispatchMouseDomEvent(session, prev_hover, "mouseout", x, y, 0, btn_num, buttons, modifiers, has_force, force_val);
                        * = dispatchMouseDomEvent(session, prev_hover, "mouseleave", x, y, 0, btn_num, buttons, modifiers, has_force, force_val);
                    }
                    if (mv_target != NULL_NODE) {
                        * = dispatchMouseDomEvent(session, mv_target, "mouseover", x, y, 0, btn_num, buttons, modifiers, has_force, force_val);
                        * = dispatchMouseDomEvent(session, mv_target, "mouseenter", x, y, 0, btn_num, buttons, modifiers, has_force, force_val);
                    }
                    if (dctx := session.dom_ctx) {
                        dctx.layout_dirty = true;
                    }
                    session.ensureLayout();
                }
                * = dispatchMouseDomEvent(session, mv_target, "mousemove", x, y, 0, btn_num, buttons, modifiers, has_force, force_val);
                if (session.range_drag_node != NULL_NODE) {
                    if (updateRangeFromX(session, session.range_drag_node, x)) {
                        session.range_drag_changed = true;
                    }
                } else if (session.has_document && (buttons & 1) != 0) {
                    let mv_buf: [1536]u8 = [0; 1536];
                    const mv_js = std.fmt.bufPrint(
                        &mv_buf,
                        "(() => {{ const ds = globalThis.__capsurf_drag_sel; if (!ds) return; if (ds.ctrl) {{ const len = String(ds.ctrl.value || '').length; const rawOff = ({d:.2} - ds.left) / ds.cw; let endOff = rawOff >= ds.startOff ? Math.max(0, Math.min(len, Math.ceil(rawOff))) : Math.max(0, Math.min(len, Math.floor(rawOff))); if (String(ds.ctrl.tagName || '').toUpperCase() === 'TEXTAREA' && {d:.2} > ds.top + 18) endOff = len; const s = Math.min(ds.startOff, endOff); const e = Math.max(ds.startOff, endOff); if (typeof ds.ctrl.setSelectionRange === 'function') ds.ctrl.setSelectionRange(s, e); return; }} if (!ds.tn || typeof getSelection !== 'function' || !document.createRange) return; const len = (ds.tn.nodeValue || ds.tn.textContent || '').length; const rawOff = ({d:.2} - ds.left) / ds.cw; const endOff = rawOff >= ds.startOff ? Math.max(0, Math.min(len, Math.ceil(rawOff))) : Math.max(0, Math.min(len, Math.floor(rawOff))); const s = Math.min(ds.startOff, endOff); const e = Math.max(ds.startOff, endOff); const range = document.createRange(); range.setStart(ds.tn, s); range.setEnd(ds.tn, e); const sel = getSelection(); if (sel) {{ sel.removeAllRanges(); sel.addRange(range); }} }})()",
                        .{ x, y, x },
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
                if (session.layout.boxes.items.len == 0) {
                    session.ensureLayout();
                } else if (dctx := session.dom_ctx) {
                    if (dctx.layout_dirty) {
                        session.ensureLayout();
                    }
                }
                const target_node = hitTest(session, x, y);
                * = dispatchMouseDomEvent(session, target_node, "mouseup", x, y, click_count, btn_num, buttons, modifiers, has_force, force_val);
                if (btn_num == 0) {
                    * = dispatchMouseDomEvent(session, target_node, "click", x, y, click_count, btn_num, buttons, modifiers, has_force, force_val);
                    if (click_count == 2) {
                        * = dispatchMouseDomEvent(session, target_node, "dblclick", x, y, click_count, btn_num, buttons, modifiers, has_force, force_val);
                    }
                }
                if (session.range_drag_node != NULL_NODE) {
                    if (session.range_drag_changed) {
                        dispatchRangeChange(session, session.range_drag_node);
                    }
                    session.range_drag_node = NULL_NODE;
                    session.range_drag_changed = false;
                }
                if (target_node != NULL_NODE) {
                    session.focused_node = target_node;
                    session.doc.focused_node = target_node;
                }
                emitPendingNavigationEvents(session, out_events, event_count);
            }
            if (session.has_document) {
                let polled_binding_evts: [4]CdpEvent = [CdpEvent{}; 4];
                const polled_b_count = session.pollBindingEvents(&polled_binding_evts);
                let b_i: usize = 0;
                while (b_i < polled_b_count && event_count.* < out_events.len) : (b_i += 1) {
                    out_events[event_count.*] = polled_binding_evts[b_i];
                    event_count.* += 1;
                }
                let polled_nav_evts: [4]CdpEvent = [CdpEvent{}; 4];
                const polled_n_count = session.pollNavigatedWithinDocEvents(&polled_nav_evts);
                let n_i: usize = 0;
                while (n_i < polled_n_count && event_count.* < out_events.len) : (n_i += 1) {
                    out_events[event_count.*] = polled_nav_evts[n_i];
                    event_count.* += 1;
                }
                let polled_fc_evts: [4]CdpEvent = [CdpEvent{}; 4];
                const polled_fc_count = session.pollFileChooserEvents(&polled_fc_evts);
                let fc_i: usize = 0;
                while (fc_i < polled_fc_count && event_count.* < out_events.len) : (fc_i += 1) {
                    out_events[event_count.*] = polled_fc_evts[fc_i];
                    event_count.* += 1;
                }
                let polled_con_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_con_count = session.pollConsoleEvents(&polled_con_evts);
                let con_i: usize = 0;
                while (con_i < polled_con_count && event_count.* < out_events.len) : (con_i += 1) {
                    out_events[event_count.*] = polled_con_evts[con_i];
                    event_count.* += 1;
                }
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
                        dispatchKeyJs(session, "keydown", escaped_key, escaped_code, modifiers, repeat);
                        if (ev_type == .KeyDown && (text.len > 0 || std.mem.eql(u8, key, "Enter")) && isPrintableKey(key, text)) {
                            const kp_key = if (text.len > 0) text else key;
                            let kp_esc_buf: [256]u8 = [0; 256];
                            const kp_esc_len = escapeJsString(kp_key, &kp_esc_buf);
                            dispatchKeyJs(session, "keypress", kp_esc_buf[0..kp_esc_len], escaped_code, modifiers, repeat);
                        }
                    } else if (ev_type == .Char) {
                        const kp_key = if (text.len > 0) text else key;
                        let kp_esc_buf: [256]u8 = [0; 256];
                        const kp_esc_len = escapeJsString(kp_key, &kp_esc_buf);
                        dispatchKeyJs(session, "keypress", kp_esc_buf[0..kp_esc_len], escaped_code, modifiers, repeat);
                    } else if (ev_type == .KeyUp) {
                        dispatchKeyJs(session, "keyup", escaped_key, escaped_code, modifiers, false);
                    }
                }
            }

            const has_select_all = std.mem.indexOf(u8, req.params_raw, "\"selectAll\"") != null ||
                ((modifiers & 6) != 0 && (std.mem.eql(u8, key, "a") || std.mem.eql(u8, key, "A")));
            const has_clip_cmd = (modifiers & 6) != 0 && (std.mem.eql(u8, key, "c") || std.mem.eql(u8, key, "C") ||
                std.mem.eql(u8, key, "x") || std.mem.eql(u8, key, "X") ||
                std.mem.eql(u8, key, "v") || std.mem.eql(u8, key, "V"));
            if (has_select_all && (ev_type == .KeyDown || ev_type == .RawKeyDown)) {
                applyInputText(session, "", key, modifiers, true);
            } else if (has_clip_cmd && (ev_type == .KeyDown || ev_type == .RawKeyDown)) {
                applyInputText(session, "", key, modifiers, false);
            } else if (!has_clip_cmd && !has_select_all) {
                if (text.len > 0 && (ev_type == .Char || ev_type == .KeyDown || ev_type == .RawKeyDown)) {
                    const norm_text = if (std.mem.eql(u8, text, "\r") || std.mem.eql(u8, text, "\\r")) "\n" else text;
                    applyInputText(session, norm_text, key, modifiers, false);
                }
                if ((ev_type == .KeyDown || ev_type == .RawKeyDown) && (std.mem.eql(u8, key, "Backspace") || std.mem.eql(u8, key, "Delete") || std.mem.eql(u8, key, "ArrowLeft") || std.mem.eql(u8, key, "ArrowRight") || std.mem.eql(u8, key, "Home") || std.mem.eql(u8, key, "End"))) {
                    applyInputText(session, "", key, modifiers, false);
                }
            }
            if (session.has_document) {
                let polled_binding_evts: [4]CdpEvent = [CdpEvent{}; 4];
                const polled_b_count = session.pollBindingEvents(&polled_binding_evts);
                let b_i: usize = 0;
                while (b_i < polled_b_count && event_count.* < out_events.len) : (b_i += 1) {
                    out_events[event_count.*] = polled_binding_evts[b_i];
                    event_count.* += 1;
                }
                let polled_nav_evts: [4]CdpEvent = [CdpEvent{}; 4];
                const polled_n_count = session.pollNavigatedWithinDocEvents(&polled_nav_evts);
                let n_i: usize = 0;
                while (n_i < polled_n_count && event_count.* < out_events.len) : (n_i += 1) {
                    out_events[event_count.*] = polled_nav_evts[n_i];
                    event_count.* += 1;
                }
                let polled_fc_evts: [4]CdpEvent = [CdpEvent{}; 4];
                const polled_fc_count = session.pollFileChooserEvents(&polled_fc_evts);
                let fc_i: usize = 0;
                while (fc_i < polled_fc_count && event_count.* < out_events.len) : (fc_i += 1) {
                    out_events[event_count.*] = polled_fc_evts[fc_i];
                    event_count.* += 1;
                }
                let polled_con_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_con_count = session.pollConsoleEvents(&polled_con_evts);
                let con_i: usize = 0;
                while (con_i < polled_con_count && event_count.* < out_events.len) : (con_i += 1) {
                    out_events[event_count.*] = polled_con_evts[con_i];
                    event_count.* += 1;
                }
            }

            return CdpResponse.ok(req.id, "{}");
        }

        // 3. Text Insertion: insertText
        if (std.mem.eql(u8, req.method, "Input.insertText")) {
            const raw_text = extractJsonString(req.params_raw, "text") ?? "";
            let unesc_buf: [1024]u8 = [0; 1024];
            const text = unescapeJsonString(raw_text, &unesc_buf);
            applyInputText(session, text, "", 0, false);
            return CdpResponse.ok(req.id, "{}");
        }

        // 4. Drag Events: dispatchDragEvent
        if (std.mem.eql(u8, req.method, "Input.dispatchDragEvent")) {
            const type_str = extractJsonString(req.params_raw, "type") ?? "";
            const x = extractJsonFloat(req.params_raw, "x") ?? 0.0;
            const y = extractJsonFloat(req.params_raw, "y") ?? 0.0;
            const is_drag_enter = std.mem.eql(u8, type_str, "dragEnter");
            const is_drag_over = std.mem.eql(u8, type_str, "dragOver");
            const is_drop = std.mem.eql(u8, type_str, "drop");
            const is_drag_cancel = std.mem.eql(u8, type_str, "dragCancel");
            let dom_type: []const u8 = "";
            if (is_drag_enter) {
                dom_type = "dragenter";
            } else if (is_drag_over) {
                dom_type = "dragover";
            } else if (is_drop) {
                dom_type = "drop";
            } else if (is_drag_cancel) {
                dom_type = "dragleave";
            }
            if (dom_type.len > 0 && session.has_document) {
                const ctx_key: usize = unsafe { @intFromPtr(session.ctx.raw_ctx) };
                const drag_state = getInputState(ctx_key);

                if (is_drop && !drag_state.drag_over_default_prevented) {
                    drag_state.has_drag_dt = false;
                    drag_state.drag_over_default_prevented = false;
                    const clr_dt = session.ctx.eval("globalThis.__capsurf_drag_dt=null;globalThis.__capsurf_drag_over_prevented=false;");
                    session.ctx.freeValue(clr_dt);
                    return CdpResponse.ok(req.id, "{}");
                }

                if (is_drag_enter && !drag_state.has_drag_dt) {
                    const clr_dt = session.ctx.eval("globalThis.__capsurf_drag_dt=null;globalThis.__capsurf_drag_over_prevented=false;");
                    session.ctx.freeValue(clr_dt);
                    drag_state.has_drag_dt = true;
                    drag_state.drag_over_default_prevented = false;
                } else if (is_drag_over && !drag_state.has_drag_dt) {
                    drag_state.has_drag_dt = true;
                }

                const items_json = extractJsonArray(req.params_raw, "items") ?? "[]";
                const files_json = extractJsonArray(req.params_raw, "files") ?? "";

                let js_list = std.ArrayListUnmanaged(u8).empty;
                defer js_list.deinit(session.allocator);

                let hdr_buf: [512]u8 = undefined;
                const hdr = std.fmt.bufPrint(
                    &hdr_buf,
                    "(function(){{if(typeof document==='undefined')return '0';var t=(document.elementFromPoint?document.elementFromPoint({d},{d}):null)||document.body||document.documentElement;if(!t)return '0';var dt=globalThis.__capsurf_drag_dt||null;var isNew=false;if(!dt&&typeof DataTransfer==='function'){{dt=new DataTransfer();globalThis.__capsurf_drag_dt=dt;isNew=true;}}if(dt&&dt.items&&isNew){{",
                    .{ @as(i64, @intFromFloat(x)), @as(i64, @intFromFloat(y)) },
                ) catch "";
                if (hdr.len > 0) {
                    js_list.appendSlice(session.allocator, hdr) catch {};

                    if (items_json.len > 2) {
                        js_list.appendSlice(session.allocator, "try{var arr=JSON.parse('") catch {};
                        let ii: usize = 0;
                        while (ii < items_json.len) : (ii += 1) {
                            const ch = items_json[ii];
                            if (ch == '\\' || ch == '\'') {
                                js_list.append(session.allocator, '\\') catch break;
                                js_list.append(session.allocator, ch) catch break;
                            } else if (ch >= 0x20) {
                                js_list.append(session.allocator, ch) catch break;
                            }
                        }
                        js_list.appendSlice(session.allocator, "');for(var i=0;i<arr.length;i++){var it=arr[i];if(it&&it.mimeType&&typeof it.data==='string')dt.items.add(it.data,it.mimeType);}}catch(e){}") catch {};
                    }

                    if (files_json.len > 2) {
                        let f_alloc: ([]u8)? = session.allocator.alloc(u8, 262144) catch null;
                        defer if (f_alloc != null) session.allocator.free(f_alloc.?);

                        let pos: usize = 1;
                        while (pos < files_json.len) {
                            const q1_opt = std.mem.indexOfScalarPos(u8, files_json, pos, '"');
                            if (q1_opt == null) break;
                            const q1 = q1_opt.? + 1;
                            let q2 = q1;
                            while (q2 < files_json.len) : (q2 += 1) {
                                if (files_json[q2] == '\\') {
                                    q2 += 1;
                                    continue;
                                }
                                if (files_json[q2] == '"') break;
                            }
                            if (q2 >= files_json.len) break;
                            const raw_path = files_json[q1..q2];
                            pos = q2 + 1;

                            let path_buf: [1024]u8 = [0; 1024];
                            const file_path = unescapeJsonString(raw_path, &path_buf);

                            let base_name = file_path;
                            if (sl := std.mem.lastIndexOfScalar(u8, base_name, '/')) {
                                base_name = base_name[sl + 1 ..];
                            }
                            if (bsl := std.mem.lastIndexOfScalar(u8, base_name, '\\')) {
                                base_name = base_name[bsl + 1 ..];
                            }

                            let fbytes_opt: ([]const u8)? = null;
                            if (fb_buf := f_alloc) {
                                fbytes_opt = MockFixtures.readFileContent(file_path, fb_buf);
                                if (fbytes_opt == null) {
                                    let pw_path_buf: [1024]u8 = [0; 1024];
                                    if (pw_p := std.fmt.bufPrint(&pw_path_buf, "test/pw/.playwright/{s}", .{file_path}) catch null) {
                                        fbytes_opt = MockFixtures.readFileContent(pw_p, fb_buf);
                                    }
                                }
                            }

                            if (fb_raw := fbytes_opt) {
                                js_list.appendSlice(session.allocator, "{var b=atob('") catch break;
                                const fb = if (fb_raw.len > 262144) fb_raw[0..262144] else fb_raw;
                                const enc_len = std.base64.standard.Encoder.calcSize(fb.len);
                                if (b64_buf := session.allocator.alloc(u8, enc_len) catch null) {
                                    defer session.allocator.free(b64_buf);
                                    const b64 = std.base64.standard.Encoder.encode(b64_buf, fb);
                                    js_list.appendSlice(session.allocator, b64) catch break;
                                }
                                let mime_type: []const u8 = "";
                                if (dot := std.mem.lastIndexOfScalar(u8, base_name, '.')) {
                                    const ext = base_name[dot..];
                                    if (std.ascii.eqlIgnoreCase(ext, ".png")) {
                                        mime_type = "image/png";
                                    } else if (std.ascii.eqlIgnoreCase(ext, ".jpg") || std.ascii.eqlIgnoreCase(ext, ".jpeg")) {
                                        mime_type = "image/jpeg";
                                    } else if (std.ascii.eqlIgnoreCase(ext, ".gif")) {
                                        mime_type = "image/gif";
                                    } else if (std.ascii.eqlIgnoreCase(ext, ".webp")) {
                                        mime_type = "image/webp";
                                    } else if (std.ascii.eqlIgnoreCase(ext, ".svg")) {
                                        mime_type = "image/svg+xml";
                                    } else if (std.ascii.eqlIgnoreCase(ext, ".txt")) {
                                        mime_type = "text/plain";
                                    } else if (std.ascii.eqlIgnoreCase(ext, ".html") || std.ascii.eqlIgnoreCase(ext, ".htm")) {
                                        mime_type = "text/html";
                                    } else if (std.ascii.eqlIgnoreCase(ext, ".css")) {
                                        mime_type = "text/css";
                                    } else if (std.ascii.eqlIgnoreCase(ext, ".js") || std.ascii.eqlIgnoreCase(ext, ".mjs")) {
                                        mime_type = "text/javascript";
                                    } else if (std.ascii.eqlIgnoreCase(ext, ".json")) {
                                        mime_type = "application/json";
                                    } else if (std.ascii.eqlIgnoreCase(ext, ".pdf")) {
                                        mime_type = "application/pdf";
                                    } else if (std.ascii.eqlIgnoreCase(ext, ".xml")) {
                                        mime_type = "application/xml";
                                    } else if (std.ascii.eqlIgnoreCase(ext, ".csv")) {
                                        mime_type = "text/csv";
                                    }
                                }
                                let bn_esc_buf: [512]u8 = [0; 512];
                                const bn_esc = escapeJsonString(base_name, &bn_esc_buf);
                                js_list.appendSlice(session.allocator, "');var u=new Uint8Array(b.length);for(var i=0;i<b.length;i++)u[i]=b.charCodeAt(i);dt.items.add(new File([u],\"") catch break;
                                js_list.appendSlice(session.allocator, bn_esc) catch break;
                                js_list.appendSlice(session.allocator, "\",{type:\"") catch break;
                                js_list.appendSlice(session.allocator, mime_type) catch break;
                                js_list.appendSlice(session.allocator, "\"}));}") catch break;
                            }
                        }
                    }

                    let tail_buf: [512]u8 = undefined;
                    const tail = std.fmt.bufPrint(
                        &tail_buf,
                        "}}var Ctor=globalThis.DragEvent||globalThis.MouseEvent||globalThis.Event;var ev=new Ctor('{s}',{{bubbles:true,cancelable:true,clientX:{d},clientY:{d}}});try{{Object.defineProperty(ev,'dataTransfer',{{value:dt,configurable:true}});}}catch(e){{ev.dataTransfer=dt;}}if(typeof t.dispatchEvent==='function')t.dispatchEvent(ev);return ev.defaultPrevented?'1':'0';}})()",
                        .{ dom_type, @as(i64, @intFromFloat(x)), @as(i64, @intFromFloat(y)) },
                    ) catch "";
                    if (tail.len > 0) {
                        js_list.appendSlice(session.allocator, tail) catch {};
                        const dv = session.ctx.eval(js_list.items);
                        if (is_drag_over && !jsIsException(dv)) {
                            const res_str = session.ctx.toCString(dv);
                            drag_state.drag_over_default_prevented = std.mem.eql(u8, res_str, "1");
                        }
                        session.ctx.freeValue(dv);
                        session.ctx.drainJobs();
                    }
                }

                if (is_drop || is_drag_cancel) {
                    drag_state.has_drag_dt = false;
                    drag_state.drag_over_default_prevented = false;
                    const clr_dt = session.ctx.eval("globalThis.__capsurf_drag_dt=null;globalThis.__capsurf_drag_over_prevented=false;");
                    session.ctx.freeValue(clr_dt);
                }
            }
            return CdpResponse.ok(req.id, "{}");
        }

        // 5. Touch Events: dispatchTouchEvent
        if (std.mem.eql(u8, req.method, "Input.dispatchTouchEvent")) {
            const type_str = extractJsonString(req.params_raw, "type") ?? "";
            const is_valid_touch = std.mem.eql(u8, type_str, "touchStart") ||
                std.mem.eql(u8, type_str, "touchMove") ||
                std.mem.eql(u8, type_str, "touchEnd") ||
                std.mem.eql(u8, type_str, "touchCancel");
            if (!is_valid_touch) {
                return CdpResponse.err(req.id, -32602, "Unknown touch event type");
            }
            const modifiers = extractJsonInt(req.params_raw, "modifiers") ?? 0;
            const points_json = extractJsonArray(req.params_raw, "touchPoints") ?? "[]";

            if (session.has_document) {
                if (session.layout.boxes.items.len == 0) {
                    session.ensureLayout();
                } else if (dctx := session.dom_ctx) {
                    if (dctx.layout_dirty) {
                        session.ensureLayout();
                    }
                }

                let js_list = std.ArrayListUnmanaged(u8).empty;
                defer js_list.deinit(session.allocator);

                let hdr_buf: [256]u8 = undefined;
                const hdr = std.fmt.bufPrint(
                    &hdr_buf,
                    "(() => {{ if (typeof __capsurf_dispatch_touch_event === 'function') {{ let pts = []; try {{ pts = JSON.parse('",
                    .{},
                ) catch "";
                if (hdr.len > 0) {
                    js_list.appendSlice(session.allocator, hdr) catch {};
                    let ti: usize = 0;
                    while (ti < points_json.len) : (ti += 1) {
                        const ch = points_json[ti];
                        if (ch == '\\' || ch == '\'') {
                            js_list.append(session.allocator, '\\') catch break;
                            js_list.append(session.allocator, ch) catch break;
                        } else if (ch >= 0x20) {
                            js_list.append(session.allocator, ch) catch break;
                        }
                    }
                    let tail_buf: [256]u8 = undefined;
                    const tail = std.fmt.bufPrint(
                        &tail_buf,
                        "'); }} catch (e) {{ pts = []; }} __capsurf_dispatch_touch_event('{s}', pts, {d}); }} }})()",
                        .{ type_str, modifiers },
                    ) catch "";
                    if (tail.len > 0) {
                        js_list.appendSlice(session.allocator, tail) catch {};
                        const tv = session.ctx.eval(js_list.items);
                        session.ctx.freeValue(tv);
                        session.ctx.drainJobs();
                    }
                }

                let polled_binding_evts: [4]CdpEvent = [CdpEvent{}; 4];
                const polled_b_count = session.pollBindingEvents(&polled_binding_evts);
                let b_i: usize = 0;
                while (b_i < polled_b_count && event_count.* < out_events.len) : (b_i += 1) {
                    out_events[event_count.*] = polled_binding_evts[b_i];
                    event_count.* += 1;
                }
                let polled_nav_evts: [4]CdpEvent = [CdpEvent{}; 4];
                const polled_n_count = session.pollNavigatedWithinDocEvents(&polled_nav_evts);
                let n_i: usize = 0;
                while (n_i < polled_n_count && event_count.* < out_events.len) : (n_i += 1) {
                    out_events[event_count.*] = polled_nav_evts[n_i];
                    event_count.* += 1;
                }
                let polled_fc_evts: [4]CdpEvent = [CdpEvent{}; 4];
                const polled_fc_count = session.pollFileChooserEvents(&polled_fc_evts);
                let fc_i: usize = 0;
                while (fc_i < polled_fc_count && event_count.* < out_events.len) : (fc_i += 1) {
                    out_events[event_count.*] = polled_fc_evts[fc_i];
                    event_count.* += 1;
                }
                let polled_con_evts: [MAX_CDP_EVENTS]CdpEvent = [CdpEvent{}; MAX_CDP_EVENTS];
                const polled_con_count = session.pollConsoleEvents(&polled_con_evts);
                let con_i: usize = 0;
                while (con_i < polled_con_count && event_count.* < out_events.len) : (con_i += 1) {
                    out_events[event_count.*] = polled_con_evts[con_i];
                    event_count.* += 1;
                }
            }

            return CdpResponse.ok(req.id, "{}");
        }

        // 6. Ignore Input Events flag: setIgnoreInputEvents
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
