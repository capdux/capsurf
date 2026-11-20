import {
    MAX_EVENT_PATH_HOPS,
    MAX_LISTENERS_PER_NODE,
    MAX_TARGET_NODES,
    EventPhase,
    DOMEvent,
    EventListenerOptions,
    EventListenerEntry,
    NodeEventListenerRegistry,
    DOMEventSystem,
} from "../../src/dom/event_target.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test_dom_events_and_propagation_edge assertion failed");
    }
}

fn testThreePhaseCaptureTargetBubbleAndNonBubbling() {
    let sys: DOMEventSystem = DOMEventSystem{};

    // Path: 1 (root) -> 2 (mid) -> 3 (child) -> 4 (target)
    assert(sys.addEventListener(1, "click", 11, EventListenerOptions{ capture: true }), "root capture");
    assert(sys.addEventListener(1, "click", 12, EventListenerOptions{ capture: false }), "root bubble");
    assert(sys.addEventListener(2, "click", 21, EventListenerOptions{ capture: true }), "mid capture");
    assert(sys.addEventListener(2, "click", 22, EventListenerOptions{ capture: false }), "mid bubble");
    assert(sys.addEventListener(3, "click", 31, EventListenerOptions{ capture: true }), "child capture");
    assert(sys.addEventListener(3, "click", 32, EventListenerOptions{ capture: false }), "child bubble");
    // Target listener with once: true so it fires cleanly once at target
    assert(sys.addEventListener(4, "click", 41, EventListenerOptions{ capture: false, once: true }), "target once");
    // Unrelated event type should never fire
    assert(sys.addEventListener(2, "keydown", 99, EventListenerOptions{ capture: true }), "mid keydown");

    let path: [4]u32 = [1, 2, 3, 4];
    let ev: DOMEvent = DOMEvent{};
    ev.init("click", 4, true, true);
    assert(ev.phase == .None, "initial phase is None");

    let invoked: [32]u32 = [0; 32];
    let count: usize = 0;
    const not_prevented = sys.dispatchEvent(&path, 4, &ev, &invoked, &count);
    assert(not_prevented, "bubbling click not prevented");
    assert(ev.phase == .None, "phase resets to None after dispatch");
    assert(ev.target_id == 4, "target_id preserved");
    assert(count == 7, "expected 7 callbacks in capture -> target -> bubble order");

    assert(invoked[0] == 11, "1st: root capture");
    assert(invoked[1] == 21, "2nd: mid capture");
    assert(invoked[2] == 31, "3rd: child capture");
    assert(invoked[3] == 41, "4th: target");
    assert(invoked[4] == 32, "5th: child bubble");
    assert(invoked[5] == 22, "6th: mid bubble");
    assert(invoked[6] == 12, "7th: root bubble");

    // Non-bubbling event (bubbles = false): capture + target run, bubble phase skipped
    assert(sys.addEventListener(4, "focus", 42, EventListenerOptions{ capture: true, once: true }), "target focus");
    assert(sys.addEventListener(1, "focus", 13, EventListenerOptions{ capture: true }), "root focus capture");
    assert(sys.addEventListener(1, "focus", 14, EventListenerOptions{ capture: false }), "root focus bubble");
    assert(sys.addEventListener(3, "focus", 33, EventListenerOptions{ capture: false }), "child focus bubble");

    let ev_nb: DOMEvent = DOMEvent{};
    ev_nb.init("focus", 4, false, false);
    let count_nb: usize = 0;
    const ok_nb = sys.dispatchEvent(&path, 4, &ev_nb, &invoked, &count_nb);
    assert(ok_nb, "non-bubbling focus returns true");
    assert(count_nb == 2, "only capture (13) and target (42) fire when bubbles=false");
    assert(invoked[0] == 13, "non-bubbling 1st: root capture");
    assert(invoked[1] == 42, "non-bubbling 2nd: target");
}

fn testSingleNodeAndEmptyPathEdge() {
    let sys: DOMEventSystem = DOMEventSystem{};
    assert(sys.addEventListener(10, "custom", 1001, EventListenerOptions{ capture: false, once: true }), "node 10 once");

    let single_path: [1]u32 = [10];
    let ev1: DOMEvent = DOMEvent{};
    ev1.init("custom", 10, true, true);
    let invoked: [32]u32 = [0; 32];
    let count: usize = 0;
    assert(sys.dispatchEvent(&single_path, 10, &ev1, &invoked, &count), "single path dispatch succeeds");
    assert(count == 1 && invoked[0] == 1001, "single path triggers target listener");
    assert(ev1.phase == .None, "single path resets phase to None");

    // Empty path still resets phase and returns !default_prevented without crashing
    let ev2: DOMEvent = DOMEvent{};
    ev2.init("custom", 10, true, true);
    let count2: usize = 99;
    assert(sys.dispatchEvent(single_path[0..0], 10, &ev2, &invoked, &count2), "empty path dispatch returns true");
    assert(count2 == 0, "empty path invokes 0 listeners after once consumed");
    assert(ev2.phase == .None, "empty path resets phase to None");
}

fn testStopPropagationAndImmediatePropagation() {
    let sys: DOMEventSystem = DOMEventSystem{};
    assert(sys.addEventListener(1, "click", 101, EventListenerOptions{ capture: true }), "root capture 101");
    assert(sys.addEventListener(1, "click", 102, EventListenerOptions{ capture: true }), "root capture 102");
    assert(sys.addEventListener(2, "click", 201, EventListenerOptions{ capture: true }), "mid capture 201");
    assert(sys.addEventListener(2, "click", 202, EventListenerOptions{ capture: false }), "mid bubble 202");

    // 1. Pre-stopped propagation halts all phases in dispatchEvent
    let path: [2]u32 = [1, 2];
    let ev_pre: DOMEvent = DOMEvent{};
    ev_pre.init("click", 2, true, true);
    ev_pre.stopPropagation();
    assert(ev_pre.propagation_stopped, "propagation_stopped is true");
    assert(!ev_pre.immediate_propagation_stopped, "stopPropagation does not set immediate flag");

    let invoked: [32]u32 = [0; 32];
    let count: usize = 0;
    assert(sys.dispatchEvent(&path, 2, &ev_pre, &invoked, &count), "pre-stopped event still returns true if not prevented");
    assert(count == 0, "pre-stopped event invokes 0 listeners");

    // 2. Simulating stopPropagation during Capturing on node 1:
    // Remaining listeners on the SAME node 1 still run, while subsequent nodes do not.
    let ev_step: DOMEvent = DOMEvent{};
    ev_step.init("click", 2, true, true);
    ev_step.phase = .Capturing;
    ev_step.stopPropagation();

    let step_count: usize = 0;
    sys.invokeListenersForNode(1, &ev_step, true, &invoked, &step_count);
    assert(step_count == 2, "stopPropagation allows remaining listeners on same node");
    assert(invoked[0] == 101 && invoked[1] == 102, "both node 1 capture listeners invoked");

    // 3. Simulating stopImmediatePropagation: halts remaining listeners on the SAME node immediately
    let ev_imm: DOMEvent = DOMEvent{};
    ev_imm.init("click", 2, true, true);
    ev_imm.phase = .Capturing;
    ev_imm.stopImmediatePropagation();
    assert(ev_imm.propagation_stopped, "stopImmediatePropagation sets propagation_stopped");
    assert(ev_imm.immediate_propagation_stopped, "stopImmediatePropagation sets immediate_propagation_stopped");

    let imm_count: usize = 0;
    sys.invokeListenersForNode(1, &ev_imm, true, &invoked, &imm_count);
    assert(imm_count == 0, "immediate_propagation_stopped blocks all subsequent listeners on same node");

    let imm_dispatch_count: usize = 0;
    assert(sys.dispatchEvent(&path, 2, &ev_imm, &invoked, &imm_dispatch_count), "imm-stopped dispatch returns true");
    assert(imm_dispatch_count == 0, "imm-stopped dispatch invokes 0 listeners across path");
}

fn testPreventDefaultCancelableAndNonCancelable() {
    let sys: DOMEventSystem = DOMEventSystem{};
    assert(sys.addEventListener(1, "submit", 10, EventListenerOptions{ capture: true }), "root capture");
    assert(sys.addEventListener(2, "submit", 20, EventListenerOptions{ capture: false, once: true }), "target once");
    assert(sys.addEventListener(1, "submit", 11, EventListenerOptions{ capture: false }), "root bubble");

    let path: [2]u32 = [1, 2];
    let invoked: [32]u32 = [0; 32];

    // Cancelable = true -> preventDefault sets default_prevented and dispatchEvent returns false,
    // while still executing all propagation phases.
    let ev_cancel: DOMEvent = DOMEvent{};
    ev_cancel.init("submit", 2, true, true);
    ev_cancel.preventDefault();
    assert(ev_cancel.default_prevented, "cancelable event default_prevented == true");

    let c1: usize = 0;
    const res1 = sys.dispatchEvent(&path, 2, &ev_cancel, &invoked, &c1);
    assert(!res1, "dispatchEvent returns false when default prevented");
    assert(c1 == 3, "preventDefault does not stop propagation");
    assert(invoked[0] == 10 && invoked[1] == 20 && invoked[2] == 11, "all 3 listeners ran");

    // Cancelable = false -> preventDefault is a no-op, dispatchEvent returns true
    let ev_nocancel: DOMEvent = DOMEvent{};
    ev_nocancel.init("submit", 2, true, false);
    ev_nocancel.preventDefault();
    assert(!ev_nocancel.default_prevented, "non-cancelable event ignores preventDefault");

    let c2: usize = 0;
    const res2 = sys.dispatchEvent(&path, 2, &ev_nocancel, &invoked, &c2);
    assert(res2, "non-cancelable event dispatchEvent returns true");
    assert(c2 == 2, "once listener 20 not re-invoked; 10 and 11 still run");
    assert(invoked[0] == 10 && invoked[1] == 11, "root capture and bubble remain active");
}

fn testOnceRemoveAndCapacityBoundaries() {
    let sys: DOMEventSystem = DOMEventSystem{};

    // Register capture and bubble with same callback_id on node 5
    assert(sys.addEventListener(5, "input", 501, EventListenerOptions{ capture: true, once: false }), "add capture 501");
    assert(sys.addEventListener(5, "input", 501, EventListenerOptions{ capture: false, once: true }), "add bubble once 501");
    assert(sys.addEventListener(6, "input", 601, EventListenerOptions{ capture: false, once: true }), "add target once 601");

    // Mismatched removeEventListener calls return false
    assert(!sys.removeEventListener(999, "input", 501, true), "remove on missing node returns false");
    assert(!sys.removeEventListener(5, "change", 501, true), "remove mismatched type returns false");
    assert(!sys.removeEventListener(5, "input", 999, true), "remove mismatched callback returns false");

    // Removing capture=true does not remove capture=false
    assert(sys.removeEventListener(5, "input", 501, true), "remove capture=true succeeds");
    assert(!sys.removeEventListener(5, "input", 501, true), "double remove capture=true returns false");

    let path: [2]u32 = [5, 6];
    let invoked: [32]u32 = [0; 32];
    let count: usize = 0;
    let ev: DOMEvent = DOMEvent{};
    ev.init("input", 6, true, true);
    * = sys.dispatchEvent(&path, 6, &ev, &invoked, &count);
    assert(count == 2, "target 601 + node 5 bubble 501 fired (capture 501 was removed)");
    assert(invoked[0] == 601 && invoked[1] == 501, "expected order 601 -> 501");

    // Both 601 and bubble 501 had once: true, so second dispatch invokes 0 listeners
    let count2: usize = 0;
    let ev2: DOMEvent = DOMEvent{};
    ev2.init("input", 6, true, true);
    * = sys.dispatchEvent(&path, 6, &ev2, &invoked, &count2);
    assert(count2 == 0, "once listeners consumed after first dispatch");

    // Capacity boundary: MAX_LISTENERS_PER_NODE (32) on a single node
    let cap_sys: DOMEventSystem = DOMEventSystem{};
    let i: u32 = 0;
    while (i < @as(u32, MAX_LISTENERS_PER_NODE)) : (i += 1) {
        assert(
            cap_sys.addEventListener(1, "tick", i + 1, EventListenerOptions{ capture: true }),
            "add listener within MAX_LISTENERS_PER_NODE",
        );
    }
    assert(
        !cap_sys.addEventListener(1, "tick", 999, EventListenerOptions{ capture: true }),
        "exceeding MAX_LISTENERS_PER_NODE returns false",
    );

    // Capacity boundary: MAX_TARGET_NODES (128)
    let node_i: u32 = 2;
    while (node_i <= @as(u32, MAX_TARGET_NODES)) : (node_i += 1) {
        assert(
            cap_sys.addEventListener(node_i, "tick", node_i, EventListenerOptions{}),
            "add target node within MAX_TARGET_NODES",
        );
    }
    assert(
        !cap_sys.addEventListener(129, "tick", 129, EventListenerOptions{}),
        "exceeding MAX_TARGET_NODES returns false",
    );
}

pub fn main() {
    testThreePhaseCaptureTargetBubbleAndNonBubbling();
    testSingleNodeAndEmptyPathEdge();
    testStopPropagationAndImmediatePropagation();
    testPreventDefaultCancelableAndNonCancelable();
    testOnceRemoveAndCapacityBoundaries();
    std.debug.print("test_dom_events_and_propagation_edge: ALL ASSERTIONS PASSED\n", .{});
}
