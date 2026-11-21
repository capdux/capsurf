import {
    MAX_MUTATION_RECORDS,
    MAX_OBSERVERS,
    MutationType,
    MutationRecord,
    MutationObserverInit,
    MutationObserver,
    MutationManager,
    mutationTypeToString,
} from "../../../src/dom/mutation_observer.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

// Test 1: attributeOldValue & attribute filtering behavior
// Note: src/dom/mutation_observer.cp does not yet implement `attribute_filter` on
// `MutationObserverInit`; this test covers `attributes: true/false` gating and
// `attribute_old_value: true/false` oldValue capture.
fn testAttributeOldValueAndGate() {
    let mgr: MutationManager = MutationManager{};
    let records: [128]MutationRecord = [MutationRecord{}; 128];

    // Observer 1: attributes=true, attribute_old_value=true
    let obs1_opt = mgr.createObserver();
    if (obs1 := obs1_opt) {
        obs1.observe(10, MutationObserverInit{
            attributes: true,
            attribute_old_value: true,
        });
    } else {
        assert(false, "obs1 created");
        return;
    }

    // Observer 2: attributes=true, attribute_old_value=false
    let obs2_opt = mgr.createObserver();
    if (obs2 := obs2_opt) {
        obs2.observe(10, MutationObserverInit{
            attributes: true,
            attribute_old_value: false,
        });
    } else {
        assert(false, "obs2 created");
        return;
    }

    // Observer 3: attributes=false, attribute_old_value=true (must ignore attribute changes)
    let obs3_opt = mgr.createObserver();
    if (obs3 := obs3_opt) {
        obs3.observe(10, MutationObserverInit{
            attributes: false,
            attribute_old_value: true,
        });
    } else {
        assert(false, "obs3 created");
        return;
    }

    mgr.recordAttributeChange(10, "data-state", "idle");
    mgr.recordAttributeChange(10, "class", "btn-primary");

    if (obs1 := obs1_opt) {
        let n1 = obs1.takeRecords(&records);
        assert(n1 == 2, "obs1 recorded 2 attribute changes");
        assert(records[0].mutation_type == .Attributes, "obs1 rec0 type is Attributes");
        assert(std.mem.eql(u8, mutationTypeToString(records[0].mutation_type), "attributes"), "type string attributes");
        assert(records[0].target_id == 10, "obs1 rec0 target_id == 10");
        assert(std.mem.eql(u8, records[0].attribute_name, "data-state"), "obs1 rec0 attr name");
        assert(std.mem.eql(u8, records[0].old_value, "idle"), "obs1 rec0 old_value preserved");
        assert(std.mem.eql(u8, records[1].attribute_name, "class"), "obs1 rec1 attr name");
        assert(std.mem.eql(u8, records[1].old_value, "btn-primary"), "obs1 rec1 old_value preserved");
    }

    if (obs2 := obs2_opt) {
        let n2 = obs2.takeRecords(&records);
        assert(n2 == 2, "obs2 recorded 2 attribute changes");
        assert(std.mem.eql(u8, records[0].attribute_name, "data-state"), "obs2 rec0 attr name");
        assert(std.mem.eql(u8, records[0].old_value, ""), "obs2 rec0 old_value omitted when attribute_old_value=false");
        assert(std.mem.eql(u8, records[1].old_value, ""), "obs2 rec1 old_value omitted when attribute_old_value=false");
    }

    if (obs3 := obs3_opt) {
        let n3 = obs3.takeRecords(&records);
        assert(n3 == 0, "obs3 ignored attribute changes when attributes=false");
    }
}

// Test 2: characterData & characterDataOldValue mutations
// Note: MutationManager does not yet expose `recordCharacterDataChange`; this tests
// `MutationType.CharacterData`, `character_data` / `character_data_old_value` options,
// and queuing CharacterData MutationRecords directly on MutationObserver.
fn testCharacterDataAndOldValue() {
    let mgr: MutationManager = MutationManager{};
    let records: [128]MutationRecord = [MutationRecord{}; 128];

    assert(std.mem.eql(u8, mutationTypeToString(.CharacterData), "characterData"), "type string characterData");

    let obs_opt = mgr.createObserver();
    if (obs := obs_opt) {
        obs.observe(42, MutationObserverInit{
            character_data: true,
            character_data_old_value: true,
        });
        assert(obs.options.character_data, "character_data option stored");
        assert(obs.options.character_data_old_value, "character_data_old_value option stored");

        let ok1 = obs.queueRecord(MutationRecord{
            mutation_type: .CharacterData,
            target_id: 42,
            attribute_name: "",
            old_value: "before-text",
            added_node_id: 0,
            removed_node_id: 0,
        });
        assert(ok1, "queued CharacterData record with old_value");

        let ok2 = obs.queueRecord(MutationRecord{
            mutation_type: .CharacterData,
            target_id: 42,
            attribute_name: "",
            old_value: if (obs.options.character_data_old_value) "second-old" else "",
            added_node_id: 0,
            removed_node_id: 0,
        });
        assert(ok2, "queued second CharacterData record");

        let n = obs.takeRecords(&records);
        assert(n == 2, "took 2 CharacterData records");
        assert(records[0].mutation_type == .CharacterData, "rec0 is CharacterData");
        assert(records[0].target_id == 42, "rec0 target_id == 42");
        assert(std.mem.eql(u8, records[0].old_value, "before-text"), "rec0 old_value matches");
        assert(std.mem.eql(u8, records[1].old_value, "second-old"), "rec1 old_value matches");
    } else {
        assert(false, "characterData observer created");
    }
}

// Test 3: subtree: true vs subtree: false & childList added/removed edge cases
// Note: MutationManager matches `obs.target_id == target_id` directly without a DOM
// parent-chain lookup, so descendant node IDs are not automatically propagated.
fn testSubtreeAndChildListEdge() {
    let mgr: MutationManager = MutationManager{};
    let records: [128]MutationRecord = [MutationRecord{}; 128];

    let obs_sub_opt = mgr.createObserver();
    if (obs_sub := obs_sub_opt) {
        obs_sub.observe(100, MutationObserverInit{
            child_list: true,
            attributes: true,
            subtree: true,
        });
        assert(obs_sub.options.subtree, "subtree=true stored");
    } else {
        assert(false, "obs_sub created");
        return;
    }

    let obs_flat_opt = mgr.createObserver();
    if (obs_flat := obs_flat_opt) {
        obs_flat.observe(100, MutationObserverInit{
            child_list: true,
            attributes: false,
            subtree: false,
        });
        assert(!obs_flat.options.subtree, "subtree=false stored");
    } else {
        assert(false, "obs_flat created");
        return;
    }

    assert(std.mem.eql(u8, mutationTypeToString(.ChildList), "childList"), "type string childList");

    // Direct mutations on target 100: addition, removal, and attribute change
    mgr.recordChildListChange(100, 101, 0);
    mgr.recordChildListChange(100, 0, 101);
    mgr.recordAttributeChange(100, "role", "dialog");
    // Descendant node 101 mutation (not matched by direct target_id == 100 check)
    mgr.recordChildListChange(101, 102, 0);
    mgr.recordAttributeChange(101, "hidden", "");

    if (obs_sub := obs_sub_opt) {
        let n_sub = obs_sub.takeRecords(&records);
        assert(n_sub == 3, "obs_sub recorded 3 direct target 100 mutations");
        assert(records[0].mutation_type == .ChildList, "obs_sub rec0 ChildList");
        assert(records[0].added_node_id == 101, "obs_sub rec0 added 101");
        assert(records[0].removed_node_id == 0, "obs_sub rec0 removed 0");
        assert(records[1].mutation_type == .ChildList, "obs_sub rec1 ChildList");
        assert(records[1].added_node_id == 0, "obs_sub rec1 added 0");
        assert(records[1].removed_node_id == 101, "obs_sub rec1 removed 101");
        assert(records[2].mutation_type == .Attributes, "obs_sub rec2 Attributes");
    }

    if (obs_flat := obs_flat_opt) {
        let n_flat = obs_flat.takeRecords(&records);
        assert(n_flat == 2, "obs_flat recorded 2 childList mutations and ignored attributes");
        assert(records[0].added_node_id == 101, "obs_flat rec0 added 101");
        assert(records[1].removed_node_id == 101, "obs_flat rec1 removed 101");
    }
}

// Test 4: takeRecords(), disconnect() clearing & stopping notifications,
// re-observe recovery, multi-observer isolation, and capacity bounds.
fn testTakeRecordsDisconnectAndIsolation() {
    let mgr: MutationManager = MutationManager{};
    let records: [128]MutationRecord = [MutationRecord{}; 128];

    let obs_a_opt = mgr.createObserver();
    let obs_b_opt = mgr.createObserver();
    let obs_c_opt = mgr.createObserver();

    if (obs_a := obs_a_opt) {
        obs_a.observe(1, MutationObserverInit{ attributes: true, attribute_old_value: true });
    } else {
        assert(false, "obs_a created");
        return;
    }
    if (obs_b := obs_b_opt) {
        obs_b.observe(1, MutationObserverInit{ child_list: true });
    } else {
        assert(false, "obs_b created");
        return;
    }
    if (obs_c := obs_c_opt) {
        obs_c.observe(2, MutationObserverInit{ attributes: true });
    } else {
        assert(false, "obs_c created");
        return;
    }

    assert(mgr.activeObserverCount() == 3, "3 active observers");

    // Queue records on target 1 (hits obs_a and obs_b, not obs_c)
    mgr.recordAttributeChange(1, "id", "old-id");
    mgr.recordChildListChange(1, 55, 0);

    // Disconnect obs_a BEFORE taking records: must clear queued records and stop future ones
    if (obs_a := obs_a_opt) {
        obs_a.disconnect();
        assert(!obs_a.is_active, "obs_a inactive after disconnect");
        assert(obs_a.takeRecords(&records) == 0, "disconnect cleared obs_a queued records");
        assert(!obs_a.queueRecord(MutationRecord{ mutation_type: .Attributes, target_id: 1 }), "queueRecord returns false when disconnected");
    }
    assert(mgr.activeObserverCount() == 2, "2 active observers after obs_a disconnect");

    // Further mutations on target 1 must NOT reach obs_a, MUST reach obs_b
    mgr.recordAttributeChange(1, "title", "t1");
    mgr.recordChildListChange(1, 56, 0);

    if (obs_a := obs_a_opt) {
        assert(obs_a.takeRecords(&records) == 0, "obs_a receives no records after disconnect");
        // Re-observing reactivates obs_a cleanly
        obs_a.observe(1, MutationObserverInit{ attributes: true });
        assert(obs_a.is_active, "obs_a reactivated on re-observe");
        assert(mgr.activeObserverCount() == 3, "3 active observers after re-observe");
    }

    if (obs_b := obs_b_opt) {
        let nb = obs_b.takeRecords(&records);
        assert(nb == 2, "obs_b isolated and received 2 childList records");
        assert(records[0].added_node_id == 55, "obs_b rec0 == 55");
        assert(records[1].added_node_id == 56, "obs_b rec1 == 56");
        assert(obs_b.takeRecords(&records) == 0, "obs_b drained after takeRecords");
    }

    if (obs_c := obs_c_opt) {
        assert(obs_c.takeRecords(&records) == 0, "obs_c on target 2 unaffected by target 1 mutations");
    }

    // Verify MAX_MUTATION_RECORDS overflow guard on obs_c
    if (obs_c := obs_c_opt) {
        let k: usize = 0;
        while (k < MAX_MUTATION_RECORDS) : (k += 1) {
            let queued = obs_c.queueRecord(MutationRecord{
                mutation_type: .Attributes,
                target_id: 2,
            });
            assert(queued, "queue within MAX_MUTATION_RECORDS succeeds");
        }
        let overflow = obs_c.queueRecord(MutationRecord{
            mutation_type: .Attributes,
            target_id: 2,
        });
        assert(!overflow, "queue beyond MAX_MUTATION_RECORDS returns false");
        assert(obs_c.takeRecords(&records) == MAX_MUTATION_RECORDS, "takeRecords returns full MAX_MUTATION_RECORDS");
    }

    // Verify MAX_OBSERVERS capacity limit (3 already created, 13 more allowed, 17th returns null)
    let created: usize = 3;
    while (created < MAX_OBSERVERS) : (created += 1) {
        assert(mgr.createObserver() != null, "createObserver within MAX_OBSERVERS succeeds");
    }
    assert(mgr.createObserver() == null, "createObserver beyond MAX_OBSERVERS returns null");
}

pub fn main() {
    testAttributeOldValueAndGate();
    testCharacterDataAndOldValue();
    testSubtreeAndChildListEdge();
    testTakeRecordsDisconnectAndIsolation();
    std.debug.print("test_mutation_observer_edge: ALL ASSERTIONS PASSED\n", .{});
}
