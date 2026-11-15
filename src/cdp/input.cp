// src/cdp/input.cp
// CDP Input Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/input.ts

import { CdpRequest, CdpResponse, extractJsonString, extractJsonInt } from "./types.cp";
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

/// Zero-allocation JSON float field extractor.
pub fn extractJsonFloat(json: []const u8, key: []const u8): (f32)? {
    let key_pat_buf: [64]u8 = [0; 64];
    if (key.len + 3 > key_pat_buf.len) return null;
    key_pat_buf[0] = '"';
    @memcpy(key_pat_buf[1 .. 1 + key.len], key);
    key_pat_buf[1 + key.len] = '"';
    const key_pat = key_pat_buf[0 .. 2 + key.len];

    const pos = std.mem.indexOf(u8, json, key_pat) orelse return null;
    let idx = pos + key_pat.len;

    // Skip whitespace and colon
    while (idx < json.len && (json[idx] == ' ' || json[idx] == ':' || json[idx] == '\t' || json[idx] == '\r' || json[idx] == '\n')) : (idx += 1) {}
    if (idx >= json.len) return null;

    const start = idx;
    let has_digits = false;
    while (idx < json.len) : (idx += 1) {
        const c = json[idx];
        if (c >= '0' && c <= '9') {
            has_digits = true;
        } else if (c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E') {
            // continue parsing valid float chars
        } else {
            break;
        }
    }

    if (!has_digits) return null;
    const slice = json[start..idx];
    return std.fmt.parseFloat(f32, slice) catch null;
}

/// Zero-allocation JSON boolean field extractor.
pub fn extractJsonBool(json: []const u8, key: []const u8): (bool)? {
    let key_pat_buf: [64]u8 = [0; 64];
    if (key.len + 3 > key_pat_buf.len) return null;
    key_pat_buf[0] = '"';
    @memcpy(key_pat_buf[1 .. 1 + key.len], key);
    key_pat_buf[1 + key.len] = '"';
    const key_pat = key_pat_buf[0 .. 2 + key.len];

    const pos = std.mem.indexOf(u8, json, key_pat) orelse return null;
    let idx = pos + key_pat.len;

    // Skip whitespace and colon
    while (idx < json.len && (json[idx] == ' ' || json[idx] == ':' || json[idx] == '\t' || json[idx] == '\r' || json[idx] == '\n')) : (idx += 1) {}
    if (idx >= json.len) return null;

    if (idx + 4 <= json.len && std.mem.eql(u8, json[idx .. idx + 4], "true")) return true;
    if (idx + 5 <= json.len && std.mem.eql(u8, json[idx .. idx + 5], "false")) return false;
    return null;
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

            // Hit test and trigger onclick if applicable
            if (ev_type == .MouseReleased || ev_type == .MousePressed) {
                const target_node = hitTest(session, x, y);
                if (target_node != NULL_NODE) {
                    if (onclick_code := session.doc.getAttribute(target_node, "onclick")) {
                        if (session.has_document) {
                            const val = session.ctx.eval(onclick_code);
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

            return CdpResponse.ok(req.id, "{}");
        }

        // 3. Text Insertion: insertText
        if (std.mem.eql(u8, req.method, "Input.insertText")) {
            const text = extractJsonString(req.params_raw, "text") ?? "";
            * = text;
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
