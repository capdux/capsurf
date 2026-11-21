// test/cdp/test_rbi_wheel_session.cp
// CapSurf Phase 12 Comprehensive Test Suite: RBI Protocol, Wheel Queue & SessionStore

import {
    WheelBatch,
    WheelQueue,
    WheelLifecycle,
    canCoalesceWheel,
} from "../../../src/cdp/wheel_queue.cp";

import {
    RbiResourceEntry,
    RbiMissingResource,
    DeliveredKeys,
    RBI_KIND_IMAGE,
    RBI_KIND_FONT_DATA,
    encodeResourceBatch,
    decodeResourceBatch,
    dropAlreadySent,
    noteDelivered,
    encodeMissingResources,
    decodeMissingResources,
} from "../../../src/cdp/rbi_protocol.cp";

import {
    StoreStats,
    StoreTable,
    SessionStore,
    TABLE_NETWORK_LOGS,
    TABLE_RESPONSE_BODIES,
} from "../../../src/cdp/session_store.cp";

fn expect(cond: bool) {
    if (!cond) @panic("assertion failed");
}

fn testWheelQueue() {
    let q = WheelQueue{};
    expect(q.isEmpty());
    expect(q.len() == 0);

    // 1. Enqueue first wheel event
    const b1 = WheelBatch.create(100.0, 200.0, 0.0, 50.0, 0);
    expect(q.enqueue(b1));
    expect(q.len() == 1);

    // 2. Enqueue coalescable second wheel event (same coordinates and modifiers)
    const b2 = WheelBatch.create(100.0, 200.0, 10.0, 30.0, 0);
    expect(canCoalesceWheel(b1, b2));
    expect(q.enqueue(b2));
    // Length stays 1 because it was coalesced into the previous batch!
    expect(q.len() == 1);

    if (p := q.peek()) {
        expect(p.delta_x == 10.0);
        expect(p.delta_y == 80.0); // 50 + 30
    } else {
        @panic("peek failed");
    }

    // 3. Insert a fence barrier
    expect(q.fence());
    expect(q.len() == 2);
    expect(q.activeFenceCount() == 1);

    // 4. Enqueue event after fence (cannot coalesce across fence)
    const b3 = WheelBatch.create(100.0, 200.0, 5.0, 15.0, 0);
    expect(q.enqueue(b3));
    expect(q.len() == 3);

    // 5. Drain batches in order
    const d1 = q.drain();
    expect(d1 != null);
    if (v1 := d1) {
        expect(!v1.isFence());
        expect(v1.delta_y == 80.0);
    }

    const d2 = q.drain();
    expect(d2 != null);
    if (v2 := d2) {
        expect(v2.isFence());
    }
    expect(q.activeFenceCount() == 0);

    const d3 = q.drain();
    expect(d3 != null);
    if (v3 := d3) {
        expect(!v3.isFence());
        expect(v3.delta_y == 15.0);
    }

    expect(q.isEmpty());
}

fn testRbiProtocol() {
    let buf: [2048]u8 = [0; 2048];
    let decoded: [8]RbiResourceEntry = [RbiResourceEntry{}; 8];

    // 1. Encode resource batch
    const img_payload = "PNG_SAMPLE_DATA_1234";
    const font_payload = "FONT_GLYPH_METRICS";

    let entries: [2]RbiResourceEntry = [
        RbiResourceEntry{
            kind: RBI_KIND_IMAGE,
            hash: 0x1122334455667788,
            length: @as(u32, @intCast(img_payload.len)),
            payload: img_payload,
        },
        RbiResourceEntry{
            kind: RBI_KIND_FONT_DATA,
            hash: 0x2233445566778899,
            length: @as(u32, @intCast(font_payload.len)),
            payload: font_payload,
        },
    ];

    const enc_len = encodeResourceBatch(entries[0..2], buf[0..]);
    expect(enc_len > 0);

    // 2. Decode resource batch
    const dec_count = decodeResourceBatch(buf[0..enc_len], decoded[0..]);
    expect(dec_count == 2);
    expect(decoded[0].kind == RBI_KIND_IMAGE);
    expect(decoded[0].hash == 0x1122334455667788);
    expect(std.mem.eql(u8, decoded[0].payload, img_payload));

    expect(decoded[1].kind == RBI_KIND_FONT_DATA);
    expect(decoded[1].hash == 0x2233445566778899);
    expect(std.mem.eql(u8, decoded[1].payload, font_payload));

    // 3. DeliveredKeys tracking
    let delivered = DeliveredKeys{};
    expect(!delivered.contains(0x1122334455667788));
    delivered.add(0x1122334455667788);
    expect(delivered.contains(0x1122334455667788));
    expect(!delivered.contains(0x2233445566778899));

    // 4. Drop already sent resources from binary batch
    let filtered_buf: [2048]u8 = [0; 2048];
    const filtered_len = dropAlreadySent(buf[0..enc_len], &delivered, filtered_buf[0..]);
    expect(filtered_len > 0);

    let decoded_filtered: [8]RbiResourceEntry = [RbiResourceEntry{}; 8];
    const filtered_count = decodeResourceBatch(filtered_buf[0..filtered_len], decoded_filtered[0..]);
    // The image was already delivered, so only the font entry remains!
    expect(filtered_count == 1);
    expect(decoded_filtered[0].kind == RBI_KIND_FONT_DATA);
    expect(decoded_filtered[0].hash == 0x2233445566778899);

    // 5. Missing resources serialization
    let missing_buf: [256]u8 = [0; 256];
    let missing_items: [1]RbiMissingResource = [
        RbiMissingResource{ kind: RBI_KIND_IMAGE, hash: 0x3344556677889900 },
    ];
    const miss_len = encodeMissingResources(missing_items[0..1], missing_buf[0..]);
    expect(miss_len > 0);

    let decoded_missing: [4]RbiMissingResource = [RbiMissingResource{}; 4];
    const miss_count = decodeMissingResources(missing_buf[0..miss_len], decoded_missing[0..]);
    expect(miss_count == 1);
    expect(decoded_missing[0].kind == RBI_KIND_IMAGE);
    expect(decoded_missing[0].hash == 0x3344556677889900);
}

fn testSessionStore() {
    let store = SessionStore{};
    expect(store.tableCount() == 0);

    // 1. Create and populate TABLE_NETWORK_LOGS
    const net_table = store.table(TABLE_NETWORK_LOGS);
    expect(net_table != null);
    if (*t := net_table) {
        expect(t.set("req-001", "{\"url\":\"https://example.com/api\",\"status\":200}"));
        expect(t.set("req-002", "{\"url\":\"https://example.com/login\",\"status\":302}"));
        expect(t.count() == 2);
        expect(t.has("req-001"));
        expect(!t.has("req-999"));

        if (val := t.get("req-001")) {
            expect(std.mem.indexOf(u8, val, "status\":200") != null);
        } else {
            @panic("get req-001 failed");
        }
    }

    // 2. Create and populate TABLE_RESPONSE_BODIES
    const body_table = store.table(TABLE_RESPONSE_BODIES);
    expect(body_table != null);
    if (*t := body_table) {
        expect(t.set("req-001", "<html><body>Agent Dashboard</body></html>"));
        expect(t.count() == 1);
    }

    expect(store.tableCount() == 2);
    expect(store.hasTable(TABLE_NETWORK_LOGS));
    expect(store.hasTable(TABLE_RESPONSE_BODIES));

    // 3. Aggregate Stats
    const stats = store.stats();
    expect(stats.table_count == 2);
    expect(stats.key_count == 3);
    expect(stats.value_bytes > 0);

    // 4. Delete and Drop
    if (*t := store.getTable(TABLE_NETWORK_LOGS)) {
        expect(t.delete("req-002"));
        expect(t.count() == 1);
        expect(!t.has("req-002"));
    }

    expect(store.dropTable(TABLE_NETWORK_LOGS));
    expect(store.tableCount() == 1);
    expect(!store.hasTable(TABLE_NETWORK_LOGS));
}

fn main() {
    testWheelQueue();
    testRbiProtocol();
    testSessionStore();
    std.debug.print("ALL PHASE 12 RBI PROTOCOL, WHEEL QUEUE & SESSION STORE TESTS PASSED!\n", .{});
}
