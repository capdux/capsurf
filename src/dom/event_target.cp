// Migrated from vendor/kitesurf/packages/dom-shim/src/events/dispatch.ts
// Implements DOM Level 3 / WHATWG EventTarget, Event phases, and propagation path dispatching.

pub const MAX_EVENT_PATH_HOPS: usize = 128;
pub const MAX_LISTENERS_PER_NODE: usize = 32;
pub const MAX_TARGET_NODES: usize = 128;

pub enum EventPhase {
    None,
    Capturing,
    AtTarget,
    Bubbling,
}

pub struct DOMEvent {
    type_str: []const u8 = "",
    target_id: u32 = 0,
    current_target_id: u32 = 0,
    phase: EventPhase = .None,
    bubbles: bool = true,
    cancelable: bool = true,
    default_prevented: bool = false,
    propagation_stopped: bool = false,
    immediate_propagation_stopped: bool = false,
    timestamp_ms: u64 = 0,

    fn init(*_, t: []const u8, target: u32, can_bubble: bool, can_cancel: bool) {
        _.type_str = t;
        _.target_id = target;
        _.current_target_id = target;
        _.phase = .None;
        _.bubbles = can_bubble;
        _.cancelable = can_cancel;
        _.default_prevented = false;
        _.propagation_stopped = false;
        _.immediate_propagation_stopped = false;
        _.timestamp_ms = 0;
    }

    fn preventDefault(*_) {
        if (_.cancelable) {
            _.default_prevented = true;
        }
    }

    fn stopPropagation(*_) {
        _.propagation_stopped = true;
    }

    fn stopImmediatePropagation(*_) {
        _.propagation_stopped = true;
        _.immediate_propagation_stopped = true;
    }
}

pub struct EventListenerOptions {
    capture: bool = false,
    passive: bool = false,
    once: bool = false,
}

pub struct EventListenerEntry {
    type_str: []const u8 = "",
    callback_id: u32 = 0,
    capture: bool = false,
    passive: bool = false,
    once: bool = false,
    is_active: bool = false,
}

pub struct NodeEventListenerRegistry {
    node_id: u32 = 0,
    listeners: [MAX_LISTENERS_PER_NODE]EventListenerEntry = [EventListenerEntry{}; 32],
    listener_count: usize = 0,

    fn addListener(*_, type_str: []const u8, callback_id: u32, opts: EventListenerOptions): bool {
        if (_.listener_count >= MAX_LISTENERS_PER_NODE) return false;
        _.listeners[_.listener_count] = EventListenerEntry{
            type_str: type_str,
            callback_id: callback_id,
            capture: opts.capture,
            passive: opts.passive,
            once: opts.once,
            is_active: true,
        };
        _.listener_count += 1;
        return true;
    }

    fn removeListener(*_, type_str: []const u8, callback_id: u32, capture: bool): bool {
        let i: usize = 0;
        while (i < _.listener_count) : (i += 1) {
            let l = &_.listeners[i];
            if (l.is_active && l.callback_id == callback_id && l.capture == capture && std.mem.eql(u8, l.type_str, type_str)) {
                l.is_active = false;
                return true;
            }
        }
        return false;
    }
}

pub struct DOMEventSystem {
    nodes: [MAX_TARGET_NODES]NodeEventListenerRegistry = [NodeEventListenerRegistry{}; 128],
    node_count: usize = 0,

    fn getOrAddNode(*_, node_id: u32): (*NodeEventListenerRegistry)? {
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            if (_.nodes[i].node_id == node_id) {
                return &_.nodes[i];
            }
        }
        if (_.node_count >= MAX_TARGET_NODES) return null;
        let idx = _.node_count;
        _.nodes[idx].node_id = node_id;
        _.nodes[idx].listener_count = 0;
        _.node_count += 1;
        return &_.nodes[idx];
    }

    fn addEventListener(*_, node_id: u32, type_str: []const u8, callback_id: u32, opts: EventListenerOptions): bool {
        let n_opt = _.getOrAddNode(node_id);
        if (n_opt == null) return false;
        return n_opt.?.addListener(type_str, callback_id, opts);
    }

    fn removeEventListener(*_, node_id: u32, type_str: []const u8, callback_id: u32, capture: bool): bool {
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            if (_.nodes[i].node_id == node_id) {
                return _.nodes[i].removeListener(type_str, callback_id, capture);
            }
        }
        return false;
    }

    fn invokeListenersForNode(*_, node_id: u32, ev: *DOMEvent, is_capture: bool, invoked_callbacks: *[32]u32, invoked_count: *usize) {
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            if (_.nodes[i].node_id == node_id) {
                let reg = &_.nodes[i];
                let l_idx: usize = 0;
                while (l_idx < reg.listener_count) : (l_idx += 1) {
                    if (ev.immediate_propagation_stopped) break;
                    let l = &reg.listeners[l_idx];
                    if (!l.is_active) continue;
                    if (!std.mem.eql(u8, l.type_str, ev.type_str)) continue;
                    if (l.capture != is_capture && ev.phase != .AtTarget) continue;

                    if (invoked_count.* < 32) {
                        invoked_callbacks[invoked_count.*] = l.callback_id;
                        invoked_count.* += 1;
                    }
                    if (l.once) {
                        l.is_active = false;
                    }
                }
                break;
            }
        }
    }

    fn dispatchEvent(
        *_,
        path: []const u32,
        target_id: u32,
        ev: *DOMEvent,
        invoked_callbacks: *[32]u32,
        invoked_count: *usize,
    ): bool {
        invoked_count.* = 0;
        ev.target_id = target_id;

        // 1. Capturing phase: root down to target's parent
        if (path.len > 1) {
            ev.phase = .Capturing;
            let i: usize = 0;
            while (i < path.len - 1) : (i += 1) {
                if (ev.propagation_stopped) break;
                ev.current_target_id = path[i];
                _.invokeListenersForNode(path[i], ev, true, invoked_callbacks, invoked_count);
            }
        }

        // 2. At target phase
        if (!ev.propagation_stopped) {
            ev.phase = .AtTarget;
            ev.current_target_id = target_id;
            _.invokeListenersForNode(target_id, ev, true, invoked_callbacks, invoked_count);
            if (!ev.immediate_propagation_stopped) {
                _.invokeListenersForNode(target_id, ev, false, invoked_callbacks, invoked_count);
            }
        }

        // 3. Bubbling phase: target's parent up to root
        if (ev.bubbles && !ev.propagation_stopped && path.len > 1) {
            ev.phase = .Bubbling;
            let j: usize = path.len - 1;
            while (j > 0) {
                j -= 1;
                if (ev.propagation_stopped) break;
                ev.current_target_id = path[j];
                _.invokeListenersForNode(path[j], ev, false, invoked_callbacks, invoked_count);
            }
        }

        ev.phase = .None;
        return !ev.default_prevented;
    }
}
