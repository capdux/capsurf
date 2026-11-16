// Migrated from vendor/kitesurf/packages/dom-shim/src/mutations/observer.ts
// Implements WHATWG MutationObserver, MutationRecord, and DOM mutation delivery.

pub const MAX_MUTATION_RECORDS: usize = 128;
pub const MAX_OBSERVERS: usize = 16;

pub enum MutationType {
    Attributes,
    CharacterData,
    ChildList,
}

pub fn mutationTypeToString(t: MutationType): []const u8 {
    return switch (t) {
        .Attributes => "attributes",
        .CharacterData => "characterData",
        .ChildList => "childList",
    };
}

pub struct MutationRecord {
    mutation_type: MutationType = .Attributes,
    target_id: u32 = 0,
    attribute_name: []const u8 = "",
    old_value: []const u8 = "",
    added_node_id: u32 = 0,
    removed_node_id: u32 = 0,
}

pub struct MutationObserverInit {
    child_list: bool = false,
    attributes: bool = false,
    character_data: bool = false,
    subtree: bool = false,
    attribute_old_value: bool = false,
    character_data_old_value: bool = false,
}

pub struct MutationObserver {
    observer_id: u32 = 0,
    target_id: u32 = 0,
    options: MutationObserverInit = MutationObserverInit{},
    records: [MAX_MUTATION_RECORDS]MutationRecord = [MutationRecord{}; 128],
    record_count: usize = 0,
    is_active: bool = false,

    fn init(*_, id: u32) {
        _.observer_id = id;
        _.target_id = 0;
        _.options = MutationObserverInit{};
        _.record_count = 0;
        _.is_active = false;
    }

    fn observe(*_, target: u32, opts: MutationObserverInit) {
        _.target_id = target;
        _.options = opts;
        _.is_active = true;
    }

    fn disconnect(*_) {
        _.is_active = false;
        _.record_count = 0;
    }

    fn takeRecords(*_, out_records: *[MAX_MUTATION_RECORDS]MutationRecord): usize {
        let count = _.record_count;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            out_records[i] = _.records[i];
        }
        _.record_count = 0;
        return count;
    }

    fn queueRecord(*_, rec: MutationRecord): bool {
        if (!_.is_active) return false;
        if (_.record_count >= MAX_MUTATION_RECORDS) return false;
        _.records[_.record_count] = rec;
        _.record_count += 1;
        return true;
    }
}

pub struct MutationManager {
    observers: [MAX_OBSERVERS]MutationObserver = [MutationObserver{}; 16],
    observer_count: usize = 0,

    fn createObserver(*_): (*MutationObserver)? {
        if (_.observer_count >= MAX_OBSERVERS) return null;
        let idx = _.observer_count;
        _.observers[idx].init(@as(u32, @intCast(idx + 1)));
        _.observer_count += 1;
        return &_.observers[idx];
    }

    fn recordAttributeChange(*_, target_id: u32, attr_name: []const u8, old_val: []const u8) {
        let i: usize = 0;
        while (i < _.observer_count) : (i += 1) {
            let obs = &_.observers[i];
            if (!obs.is_active) continue;
            if (obs.target_id == target_id && obs.options.attributes) {
                let rec = MutationRecord{
                    mutation_type: .Attributes,
                    target_id: target_id,
                    attribute_name: attr_name,
                    old_value: if (obs.options.attribute_old_value) old_val else "",
                    added_node_id: 0,
                    removed_node_id: 0,
                };
                * = obs.queueRecord(rec);
            }
        }
    }

    fn recordChildListChange(*_, parent_id: u32, added_id: u32, removed_id: u32) {
        let i: usize = 0;
        while (i < _.observer_count) : (i += 1) {
            let obs = &_.observers[i];
            if (!obs.is_active) continue;
            if (obs.target_id == parent_id && obs.options.child_list) {
                let rec = MutationRecord{
                    mutation_type: .ChildList,
                    target_id: parent_id,
                    attribute_name: "",
                    old_value: "",
                    added_node_id: added_id,
                    removed_node_id: removed_id,
                };
                * = obs.queueRecord(rec);
            }
        }
    }

    fn activeObserverCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.observer_count) : (i += 1) {
            if (_.observers[i].is_active) {
                count += 1;
            }
        }
        return count;
    }
}
