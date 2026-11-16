// Migrated from vendor/kitesurf/packages/page-script/src/message-channel.ts
// Implements page MessageChannel and MessagePort queue and state machine.

pub const MAX_PORT_QUEUE: usize = 64;

pub struct MessagePacket {
    data: []const u8 = "",
}

pub struct MessagePort {
    id: u32 = 0,
    is_started: bool = false,
    is_closed: bool = false,
    queue: [64]MessagePacket = [MessagePacket{}; 64],
    queue_len: usize = 0,

    fn postMessage(*_, data: []const u8, other_port: *MessagePort) {
        if (_.is_closed || other_port.is_closed) return;
        if (other_port.queue_len >= other_port.queue.len) return;
        other_port.queue[other_port.queue_len] = MessagePacket{ data: data };
        other_port.queue_len += 1;
    }

    fn start(*_) {
        if (_.is_closed) return;
        _.is_started = true;
    }

    fn close(*_) {
        _.is_closed = true;
        _.queue_len = 0;
    }

    fn popMessage(*_): MessagePacket? {
        if (!_.is_started || _.is_closed || _.queue_len == 0) return null;
        let pkt = _.queue[0];
        let i: usize = 0;
        while (i + 1 < _.queue_len) : (i += 1) {
            _.queue[i] = _.queue[i + 1];
        }
        _.queue_len -= 1;
        return pkt;
    }

    fn pendingCount(): usize {
        return _.queue_len;
    }

    fn isStarted(): bool {
        return _.is_started;
    }

    fn isClosed(): bool {
        return _.is_closed;
    }
}

pub struct MessageChannel {
    port1: MessagePort = MessagePort{},
    port2: MessagePort = MessagePort{},

    static fn init(id1: u32, id2: u32): MessageChannel {
        return MessageChannel{
            port1: MessagePort{
                id: id1,
            },
            port2: MessagePort{
                id: id2,
            },
        };
    }

    fn post1To2(*_, data: []const u8) {
        _.port1.postMessage(data, &_.port2);
    }

    fn post2To1(*_, data: []const u8) {
        _.port2.postMessage(data, &_.port1);
    }

    fn close(*_) {
        _.port1.close();
        _.port2.close();
    }
}
