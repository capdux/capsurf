import {
    MessagePacket,
    MessagePort,
    MessageChannel,
    MAX_PORT_QUEUE,
} from "../../src/eval/message_channel.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. Test basic MessageChannel creation and communication
    {
        let ch = MessageChannel.init(1, 2);
        assert(ch.port1.id == 1, "port1 id is 1");
        assert(ch.port2.id == 2, "port2 id is 2");
        assert(!ch.port1.isStarted(), "port1 initially not started");
        assert(!ch.port2.isStarted(), "port2 initially not started");

        // Post message from port1 to port2 while port2 is not started
        ch.post1To2("hello from port1");
        assert(ch.port2.pendingCount() == 1, "port2 has 1 queued message");
        assert(ch.port2.popMessage() == null, "cannot pop before start()");

        // Start port2 and consume
        ch.port2.start();
        assert(ch.port2.isStarted(), "port2 is now started");
        if (msg := ch.port2.popMessage()) {
            assert(std.mem.eql(u8, msg.data, "hello from port1"), "received hello from port1");
        } else {
            @panic("expected message on port2");
        }
        assert(ch.port2.pendingCount() == 0, "port2 queue empty");

        // Bi-directional: Post from port2 to port1
        ch.post2To1("ack from port2");
        ch.port1.start();
        if (msg := ch.port1.popMessage()) {
            assert(std.mem.eql(u8, msg.data, "ack from port2"), "received ack on port1");
        } else {
            @panic("expected message on port1");
        }
    }

    // 2. Test FIFO queue ordering and capacity bounds
    {
        let ch = MessageChannel.init(10, 20);
        ch.port2.start();

        ch.post1To2("msg-1");
        ch.post1To2("msg-2");
        ch.post1To2("msg-3");
        assert(ch.port2.pendingCount() == 3, "3 messages queued");

        if (m1 := ch.port2.popMessage()) {
            assert(std.mem.eql(u8, m1.data, "msg-1"), "fifo 1st message");
        }
        if (m2 := ch.port2.popMessage()) {
            assert(std.mem.eql(u8, m2.data, "msg-2"), "fifo 2nd message");
        }
        if (m3 := ch.port2.popMessage()) {
            assert(std.mem.eql(u8, m3.data, "msg-3"), "fifo 3rd message");
        }
        assert(ch.port2.popMessage() == null, "no more messages");
    }

    // 3. Test close behavior
    {
        let ch = MessageChannel.init(100, 200);
        ch.port1.start();
        ch.port2.start();

        ch.post1To2("will be cleared");
        ch.port2.close();
        assert(ch.port2.isClosed(), "port2 closed");
        assert(ch.port2.pendingCount() == 0, "queue cleared on close");
        assert(ch.port2.popMessage() == null, "cannot pop from closed port");

        // Posting to or from closed port is dropped
        ch.post1To2("dropped message");
        assert(ch.port2.pendingCount() == 0, "message to closed port dropped");

        ch.post2To1("dropped outgoing");
        assert(ch.port1.pendingCount() == 0, "message from closed port dropped");
    }

    std.debug.print("test_message_channel passed all assertions!\n", .{});
}
