// test/net/test_sse_parser.cp
// Tests for Server-Sent Events (SSE) streaming protocol parser

import {
    ParsedEvent,
    EventStreamParser,
} from "../../../src/net/sse_parser.cp";

fn check(desc: []const u8, cond: bool) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{desc});
        @panic("test failure");
    }
}

fn main() {
    let events: [16]ParsedEvent = [ParsedEvent{}; 16];

    // 1. Basic event parsing: data: a\n\n
    {
        let parser = EventStreamParser.init();
        let count = parser.feedChunk("data: a\n\n", events[0..]);
        check("test 1 count", count == 1);
        check("test 1 type", std.mem.eql(u8, events[0].event_type, "message"));
        check("test 1 data", std.mem.eql(u8, events[0].data, "a"));
        check("test 1 id", std.mem.eql(u8, events[0].last_event_id, ""));
    }

    // 2. UTF-8 BOM handling: \xEF\xBB\xBFdata: bom\n\n
    {
        let parser = EventStreamParser.init();
        let count = parser.feedChunk("\xef\xbb\xbfdata: bom\n\n", events[0..]);
        check("test 2 count", count == 1);
        check("test 2 type", std.mem.eql(u8, events[0].event_type, "message"));
        check("test 2 data", std.mem.eql(u8, events[0].data, "bom"));
        check("test 2 id", std.mem.eql(u8, events[0].last_event_id, ""));
    }

    // 3. Comments and CRLF: :comment\n\ndata:b\r\n\r\n
    {
        let parser = EventStreamParser.init();
        let count = parser.feedChunk(":comment\n\ndata:b\r\n\r\n", events[0..]);
        check("test 3 count", count == 1);
        check("test 3 type", std.mem.eql(u8, events[0].event_type, "message"));
        check("test 3 data", std.mem.eql(u8, events[0].data, "b"));
        check("test 3 id", std.mem.eql(u8, events[0].last_event_id, ""));
    }

    // 4. Custom event type: event: tick\ndata: 1\n\n
    {
        let parser = EventStreamParser.init();
        let count = parser.feedChunk("event: tick\ndata: 1\n\n", events[0..]);
        check("test 4 count", count == 1);
        check("test 4 type", std.mem.eql(u8, events[0].event_type, "tick"));
        check("test 4 data", std.mem.eql(u8, events[0].data, "1"));
        check("test 4 id", std.mem.eql(u8, events[0].last_event_id, ""));
    }

    // 5. Event with ID: id: 7\ndata: x\n\n
    {
        let parser = EventStreamParser.init();
        let count = parser.feedChunk("id: 7\ndata: x\n\n", events[0..]);
        check("test 5 count", count == 1);
        check("test 5 type", std.mem.eql(u8, events[0].event_type, "message"));
        check("test 5 data", std.mem.eql(u8, events[0].data, "x"));
        check("test 5 id", std.mem.eql(u8, events[0].last_event_id, "7"));
        check("test 5 parser.last_event_id", std.mem.eql(u8, parser.last_event_id, "7"));
    }

    // 6. Multi-line data: data: a\ndata: b\n\n
    {
        let parser = EventStreamParser.init();
        let count = parser.feedChunk("data: a\ndata: b\n\n", events[0..]);
        check("test 6 count", count == 1);
        check("test 6 type", std.mem.eql(u8, events[0].event_type, "message"));
        check("test 6 data", std.mem.eql(u8, events[0].data, "a\nb"));
        check("test 6 id", std.mem.eql(u8, events[0].last_event_id, ""));
    }

    // 7. Incomplete stream: data: 1\n -> 0 events
    {
        let parser = EventStreamParser.init();
        let count = parser.feedChunk("data: 1\n", events[0..]);
        check("test 7 count", count == 0);
    }

    // 8. CRLF split across chunk boundaries
    {
        let parser = EventStreamParser.init();
        let count1 = parser.feedChunk("data: a\r", events[0..]);
        check("test 8 chunk1 count", count1 == 0);
        let count2 = parser.feedChunk("\ndata: b\r\n\r\n", events[0..]);
        check("test 8 chunk2 count", count2 == 1);
        check("test 8 data", std.mem.eql(u8, events[0].data, "a\nb"));
        check("test 8 type", std.mem.eql(u8, events[0].event_type, "message"));
        check("test 8 id", std.mem.eql(u8, events[0].last_event_id, ""));
    }

    // 9. Retry field parsing: retry: 50\n\n
    {
        let parser = EventStreamParser.init();
        let count = parser.feedChunk("retry: 50\n\n", events[0..]);
        check("test 9 count", count == 0);
        check("test 9 reconnectionTimeMs", parser.reconnection_time_ms == 50);
    }

    // 10. ID-only block: id: 7\n\n keeps last_event_id
    {
        let parser = EventStreamParser.init();
        let count = parser.feedChunk("id: 7\n\n", events[0..]);
        check("test 10 count", count == 0);
        check("test 10 last_event_id", std.mem.eql(u8, parser.last_event_id, "7"));
    }

    // 11. Multiple events in a single chunk
    {
        let parser = EventStreamParser.init();
        let count = parser.feedChunk("data: first\n\ndata: second\n\n", events[0..]);
        check("test 11 count", count == 2);
        check("test 11 event 0 data", std.mem.eql(u8, events[0].data, "first"));
        check("test 11 event 1 data", std.mem.eql(u8, events[1].data, "second"));
    }

    // 12. Combined retry with data event
    {
        let parser = EventStreamParser.init();
        let count = parser.feedChunk("retry: 1500\nevent: update\ndata: payload\n\n", events[0..]);
        check("test 12 count", count == 1);
        check("test 12 retry_ms", events[0].retry_ms == 1500);
        check("test 12 reconnection_time_ms", parser.reconnection_time_ms == 1500);
        check("test 12 event_type", std.mem.eql(u8, events[0].event_type, "update"));
        check("test 12 data", std.mem.eql(u8, events[0].data, "payload"));
    }

    // 13. Reset clears state back to defaults
    {
        let parser = EventStreamParser.init();
        * = parser.feedChunk("id: 99\nretry: 5000\ndata: temp\n\n", events[0..]);
        check("test 13 before reset id", std.mem.eql(u8, parser.last_event_id, "99"));
        check("test 13 before reset retry", parser.reconnection_time_ms == 5000);

        parser.reset();
        check("test 13 after reset id", std.mem.eql(u8, parser.last_event_id, ""));
        check("test 13 after reset retry", parser.reconnection_time_ms == 3000);
        check("test 13 after reset current_event", std.mem.eql(u8, parser.current_event, "message"));
    }

    // 14. Fine-grained byte-by-byte streaming
    {
        let parser = EventStreamParser.init();
        const raw = "data: stream\n\n";
        let total_emitted: usize = 0;
        let k: usize = 0;
        while (k < raw.len) : (k += 1) {
            total_emitted += parser.feedChunk(raw[k .. k + 1], events[total_emitted..]);
        }
        check("test 14 total emitted", total_emitted == 1);
        check("test 14 data", std.mem.eql(u8, events[0].data, "stream"));
    }

    std.debug.print("All sse_parser tests passed successfully!\n", .{});
}
