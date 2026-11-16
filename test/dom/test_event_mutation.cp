import {
    EventPhase,
    DOMEvent,
    EventListenerOptions,
    EventListenerEntry,
    NodeEventListenerRegistry,
    DOMEventSystem,
} from "../../src/dom/event_target.cp";
import {
    MutationType,
    MutationRecord,
    MutationObserverInit,
    MutationObserver,
    MutationManager,
    mutationTypeToString,
} from "../../src/dom/mutation_observer.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. Test EventTarget and Dispatching Pipeline
    {
        let sys: DOMEventSystem = DOMEventSystem{};

        // Node 1 (Root): capture listener (callback 101) and bubble listener (callback 102)
        assert(sys.addEventListener(1, "click", 101, EventListenerOptions{ capture: true, passive: false, once: false }), "add root capture");
        assert(sys.addEventListener(1, "click", 102, EventListenerOptions{ capture: false, passive: false, once: false }), "add root bubble");

        // Node 2 (Parent): bubble listener (callback 201)
        assert(sys.addEventListener(2, "click", 201, EventListenerOptions{ capture: false, passive: false, once: false }), "add parent bubble");

        // Node 3 (Target): target listener (callback 301)
        assert(sys.addEventListener(3, "click", 301, EventListenerOptions{ capture: false, passive: false, once: true }), "add target listener once");

        // Dispatch path: 1 -> 2 -> 3
        let path: [3]u32 = [1, 2, 3];
        let ev: DOMEvent = DOMEvent{};
        ev.init("click", 3, true, true);

        let invoked: [32]u32 = [0; 32];
        let count: usize = 0;
        let not_prevented = sys.dispatchEvent(&path, 3, &ev, &invoked, &count);
        assert(not_prevented, "default was not prevented");
        assert(count == 4, "4 listeners invoked in order");

        // Order should be:
        // 1. Root capture: 101
        // 2. Target: 301
        // 3. Parent bubble: 201
        // 4. Root bubble: 102
        assert(invoked[0] == 101, "first is root capture (101)");
        assert(invoked[1] == 301, "second is target (301)");
        assert(invoked[2] == 201, "third is parent bubble (201)");
        assert(invoked[3] == 102, "fourth is root bubble (102)");

        // Because Node 3 had `once: true`, dispatching again should not invoke 301!
        let ev2: DOMEvent = DOMEvent{};
        ev2.init("click", 3, true, true);
        let count2: usize = 0;
        * = sys.dispatchEvent(&path, 3, &ev2, &invoked, &count2);
        assert(count2 == 3, "only 3 listeners invoked after once is consumed");
        assert(invoked[0] == 101, "root capture");
        assert(invoked[1] == 201, "parent bubble");
        assert(invoked[2] == 102, "root bubble");

        // Test preventDefault
        let ev3: DOMEvent = DOMEvent{};
        ev3.init("click", 3, true, true);
        ev3.preventDefault();
        let count3: usize = 0;
        let ok3 = sys.dispatchEvent(&path, 3, &ev3, &invoked, &count3);
        assert(!ok3, "dispatchEvent returns false when preventDefault is called");
    }

    // 2. Test MutationObserver and MutationManager
    {
        let mgr: MutationManager = MutationManager{};
        let obs_opt = mgr.createObserver();
        assert(obs_opt != null, "create mutation observer");
        let obs = obs_opt.?;

        let opts = MutationObserverInit{
            child_list: true,
            attributes: true,
            attribute_old_value: true,
        };
        obs.observe(10, opts);
        assert(mgr.activeObserverCount() == 1, "active observer count is 1");

        // Record attribute change on target 10
        mgr.recordAttributeChange(10, "class", "old-btn");
        // Record attribute change on unrelated node 20 (should NOT be recorded)
        mgr.recordAttributeChange(20, "id", "other");
        // Record child insertion on target 10
        mgr.recordChildListChange(10, 101, 0);

        let records: [128]MutationRecord = [MutationRecord{}; 128];
        let rec_count = obs.takeRecords(&records);
        assert(rec_count == 2, "2 records queued for target 10");

        assert(records[0].mutation_type == .Attributes, "first is attribute mutation");
        assert(std.mem.eql(u8, records[0].attribute_name, "class"), "attr name matches");
        assert(std.mem.eql(u8, records[0].old_value, "old-btn"), "old value recorded");

        assert(records[1].mutation_type == .ChildList, "second is childList mutation");
        assert(records[1].added_node_id == 101, "added node id matches");

        // After takeRecords, buffer is empty
        assert(obs.takeRecords(&records) == 0, "buffer is empty after takeRecords");

        obs.disconnect();
        assert(mgr.activeObserverCount() == 0, "active observer count 0 after disconnect");
    }

    std.debug.print("test_event_mutation: ALL ASSERTIONS PASSED\n", .{});
}
