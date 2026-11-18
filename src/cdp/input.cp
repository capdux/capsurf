// src/cdp/input.cp
// CDP Input Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/input.ts

import { CdpRequest, CdpResponse, extractJsonString, extractJsonInt, extractJsonFloat, extractJsonBool, unescapeJsonString } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { NodeId, NULL_NODE } from "../dom/dom.cp";

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

pub fn applyInputText(session: *CdpPageSession, text: []const u8) {
    if (text.len == 0) return;

    // Filter control characters (< 32 or == 127)
    let clean_buf: [1024]u8 = [0; 1024];
    let clean_len: usize = 0;
    let ci: usize = 0;
    while (ci < text.len && clean_len < clean_buf.len) : (ci += 1) {
        const c = text[ci];
        if (c >= 32 && c != 127) {
            clean_buf[clean_len] = c;
            clean_len += 1;
        }
    }
    if (clean_len == 0) return;
    const clean_text = clean_buf[0..clean_len];

    // Escape clean_text for JS string literal
    let esc_buf: [2048]u8 = [0; 2048];
    let ei: usize = 0;
    let si: usize = 0;
    while (si < clean_text.len && ei + 2 < esc_buf.len) : (si += 1) {
        const c = clean_text[si];
        if (c == '"' || c == '\\' || c == '\'') {
            esc_buf[ei] = '\\';
            ei += 1;
        }
        esc_buf[ei] = c;
        ei += 1;
    }
    const escaped_text = esc_buf[0..ei];

    if (session.has_document) {
        let js_buf: [2048]u8 = [0; 2048];
        const js_expr = std.fmt.bufPrint(
            &js_buf,
            "(() => {{ let el = null; if ({d} > 0) {{ function f(r, nid) {{ if (!r) return null; if (r._nid === nid) return r; const all = r.querySelectorAll ? r.querySelectorAll('*') : []; for (let i = 0; i < all.length; i++) {{ if (all[i]._nid === nid) return all[i]; }} return null; }} el = f(document, {d}); }} if (!el) {{ el = document.activeElement; }} if (!el || el === document.body || el === document.documentElement) return 0; const prev = el.value !== undefined ? String(el.value) : ''; const next = prev + \"{s}\"; el.value = next; if (el.tagName === 'TEXTAREA' || el.tagName === 'INPUT') {{ try {{ const ev = typeof InputEvent !== 'undefined' ? new InputEvent('input', {{ bubbles: true, cancelable: false, data: \"{s}\" }}) : new Event('input', {{ bubbles: true }}); el.dispatchEvent(ev); }} catch (e) {{ try {{ el.dispatchEvent(new Event('input', {{ bubbles: true }})); }} catch (e2) {{}} }} }} return (typeof el._nid === 'number') ? el._nid : 0; }})()",
            .{ session.focused_node, session.focused_node, escaped_text, escaped_text },
        ) catch null;
        if (e := js_expr) {
            const val = session.ctx.eval(e);
            if (val.tag == 0 && val.u.uint64 > 0) {
                const nid = @as(u32, @truncate(val.u.uint64));
                session.focused_node = nid;
            }
            session.ctx.freeValue(val);
        }
    }
}

pub struct InputHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8): CdpResponse {
        * = out_buf;

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

            // Hit test and trigger onclick if applicable
            if (ev_type == .MouseReleased || ev_type == .MousePressed) {
                const target_node = hitTest(session, x, y);
                if (target_node != NULL_NODE) {
                    session.focused_node = target_node;
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

            if (text.len > 0 && (ev_type == .Char || ev_type == .KeyDown || ev_type == .RawKeyDown)) {
                applyInputText(session, text);
            }

            return CdpResponse.ok(req.id, "{}");
        }

        // 3. Text Insertion: insertText
        if (std.mem.eql(u8, req.method, "Input.insertText")) {
            const raw_text = extractJsonString(req.params_raw, "text") ?? "";
            let unesc_buf: [1024]u8 = [0; 1024];
            const text = unescapeJsonString(raw_text, &unesc_buf);
            applyInputText(session, text);
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
