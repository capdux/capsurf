// Migrated from vendor/kitesurf/packages/engine/src/cdp/wheel-queue.ts
// Serializes per-target wheel events during scrolling and frame handoff.

pub struct WheelBatch {
    x: f32 = 0.0,
    y: f32 = 0.0,
    delta_x: f32 = 0.0,
    delta_y: f32 = 0.0,
    modifiers: i64 = 0,
    click_count: i64 = 1,
    buttons: i64 = 0,
    is_fence: bool = false,
    fence_id: u32 = 0,

    pub static fn create(x: f32, y: f32, dx: f32, dy: f32, modifiers: i64): WheelBatch {
        return WheelBatch {
            x: x,
            y: y,
            delta_x: dx,
            delta_y: dy,
            modifiers: modifiers,
            click_count: 1,
            buttons: 0,
            is_fence: false,
            fence_id: 0,
        };
    }

    pub static fn createFence(fence_id: u32): WheelBatch {
        return WheelBatch {
            x: 0.0,
            y: 0.0,
            delta_x: 0.0,
            delta_y: 0.0,
            modifiers: 0,
            click_count: 0,
            buttons: 0,
            is_fence: true,
            fence_id: fence_id,
        };
    }

    pub fn isFence(): bool {
        return _.is_fence;
    }
}

/// Checks coordinates, modifiers, and buttons consistency between two wheel events.
pub fn canCoalesceWheel(a: WheelBatch, b: WheelBatch): bool {
    if (a.is_fence || b.is_fence) return false;
    return a.x == b.x &&
        a.y == b.y &&
        a.modifiers == b.modifiers &&
        a.buttons == b.buttons &&
        a.click_count == b.click_count;
}

pub enum WheelLifecycle {
    Accepting,
    Stopping,
    Cancelled,
}

pub const WHEEL_QUEUE_CAPACITY: usize = 32;

/// Fixed-capacity (32 slots) ring buffer for coalescing wheel events and fence markers.
pub struct WheelQueue {
    items: [WHEEL_QUEUE_CAPACITY]WheelBatch = [WheelBatch{}; WHEEL_QUEUE_CAPACITY],
    head: usize = 0,
    tail: usize = 0,
    count: usize = 0,
    lifecycle: WheelLifecycle = .Accepting,
    next_fence_id: u32 = 1,
    active_fences: usize = 0,

    pub static fn init(): WheelQueue {
        return WheelQueue{};
    }

    pub fn isAccepting(): bool {
        return _.lifecycle == .Accepting;
    }

    pub fn isCancelled(): bool {
        return _.lifecycle == .Cancelled;
    }

    pub fn isStopping(): bool {
        return _.lifecycle == .Stopping;
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    pub fn isFull(): bool {
        return _.count >= WHEEL_QUEUE_CAPACITY;
    }

    pub fn activeFenceCount(): usize {
        return _.active_fences;
    }

    /// Enqueues a wheel batch, coalescing with the trailing event if compatible.
    pub fn enqueue(*_, batch: WheelBatch): bool {
        if (!_.isAccepting()) return false;

        // Try coalescing with the most recently enqueued item
        if (_.count > 0) {
            const last_idx = (_.tail + WHEEL_QUEUE_CAPACITY - 1) % WHEEL_QUEUE_CAPACITY;
            if (canCoalesceWheel(_.items[last_idx], batch)) {
                _.items[last_idx].delta_x += batch.delta_x;
                _.items[last_idx].delta_y += batch.delta_y;
                return true;
            }
        }

        // Insert new batch if capacity permits
        if (_.count >= WHEEL_QUEUE_CAPACITY) return false;

        _.items[_.tail] = batch;
        _.tail = (_.tail + 1) % WHEEL_QUEUE_CAPACITY;
        _.count += 1;
        return true;
    }

    /// Inserts a fence synchronization marker to wait for pending scrolls.
    pub fn fence(*_): bool {
        if (_.isCancelled() || _.count >= WHEEL_QUEUE_CAPACITY) return false;

        const fid = _.next_fence_id;
        _.next_fence_id += 1;

        _.items[_.tail] = WheelBatch.createFence(fid);
        _.tail = (_.tail + 1) % WHEEL_QUEUE_CAPACITY;
        _.count += 1;
        _.active_fences += 1;
        return true;
    }

    /// Extracts the next batch or fence marker from the head of the queue.
    pub fn drain(*_): WheelBatch? {
        if (_.count == 0) return null;

        const item = _.items[_.head];
        _.head = (_.head + 1) % WHEEL_QUEUE_CAPACITY;
        _.count -= 1;

        if (item.is_fence && _.active_fences > 0) {
            _.active_fences -= 1;
        }

        return item;
    }

    /// Drains all available batches into a caller-supplied slice, returning the number of items.
    pub fn drainAll(*_, out: []WheelBatch): usize {
        let drained: usize = 0;
        while (drained < out.len && _.count > 0) {
            if (item := _.drain()) {
                out[drained] = item;
                drained += 1;
            } else {
                break;
            }
        }
        return drained;
    }

    /// Peeks at the item at the head of the queue without removing it.
    pub fn peek(): WheelBatch? {
        if (_.count == 0) return null;
        return _.items[_.head];
    }

    /// Transitions queue to stopping state.
    pub fn stop(*_) {
        if (_.lifecycle == .Accepting) {
            _.lifecycle = .Stopping;
        }
    }

    /// Cancels all pending batches and fences in the queue.
    pub fn cancel(*_) {
        _.lifecycle = .Cancelled;
        _.clear();
    }

    /// Clears the queue without changing lifecycle.
    pub fn clear(*_) {
        _.head = 0;
        _.tail = 0;
        _.count = 0;
        _.active_fences = 0;
    }
}

pub const MAX_TARGET_WHEEL_QUEUES: usize = 16;

pub struct TargetWheelQueueEntry {
    target_id_buf: [64]u8 = [0; 64],
    target_id_len: usize = 0,
    session_id_buf: [64]u8 = [0; 64],
    session_id_len: usize = 0,
    queue: WheelQueue = WheelQueue.init(),
    active: bool = false,

    pub fn targetId(*_): []const u8 {
        return _.target_id_buf[0.._.target_id_len];
    }

    pub fn sessionId(*_): []const u8 {
        return _.session_id_buf[0.._.session_id_len];
    }
}

/// Multi-target wheel queue registry matching Kitesurf target/session mapping.
pub struct WheelQueueStore {
    entries: [MAX_TARGET_WHEEL_QUEUES]TargetWheelQueueEntry = [TargetWheelQueueEntry{}; MAX_TARGET_WHEEL_QUEUES],
    count: usize = 0,

    pub static fn init(): WheelQueueStore {
        return WheelQueueStore{};
    }

    pub fn getQueue(*_, target_id: []const u8, session_id: []const u8): (*WheelQueue)? {
        let i: usize = 0;
        while (i < MAX_TARGET_WHEEL_QUEUES) : (i += 1) {
            if (_.entries[i].active &&
                std.mem.eql(u8, _.entries[i].targetId(), target_id) &&
                std.mem.eql(u8, _.entries[i].sessionId(), session_id)) {
                return &_.entries[i].queue;
            }
        }

        // Allocate slot for new target/session pair
        if (_.count >= MAX_TARGET_WHEEL_QUEUES) return null;
        if (target_id.len > 64 || session_id.len > 64) return null;

        let slot: usize = 0;
        while (slot < MAX_TARGET_WHEEL_QUEUES) : (slot += 1) {
            if (!_.entries[slot].active) break;
        }
        if (slot >= MAX_TARGET_WHEEL_QUEUES) return null;

        @memcpy(_.entries[slot].target_id_buf[0..target_id.len], target_id);
        _.entries[slot].target_id_len = target_id.len;
        @memcpy(_.entries[slot].session_id_buf[0..session_id.len], session_id);
        _.entries[slot].session_id_len = session_id.len;
        _.entries[slot].queue.clear();
        _.entries[slot].queue.lifecycle = .Accepting;
        _.entries[slot].active = true;
        _.count += 1;

        return &_.entries[slot].queue;
    }

    pub fn removeQueue(*_, target_id: []const u8, session_id: []const u8): bool {
        let i: usize = 0;
        while (i < MAX_TARGET_WHEEL_QUEUES) : (i += 1) {
            if (_.entries[i].active &&
                std.mem.eql(u8, _.entries[i].targetId(), target_id) &&
                std.mem.eql(u8, _.entries[i].sessionId(), session_id)) {
                _.entries[i].queue.clear();
                _.entries[i].active = false;
                _.entries[i].target_id_len = 0;
                _.entries[i].session_id_len = 0;
                _.count -= 1;
                return true;
            }
        }
        return false;
    }

    pub fn cancelTargetQueues(*_, target_id: []const u8) {
        let i: usize = 0;
        while (i < MAX_TARGET_WHEEL_QUEUES) : (i += 1) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].targetId(), target_id)) {
                _.entries[i].queue.cancel();
                _.entries[i].active = false;
                _.entries[i].target_id_len = 0;
                _.entries[i].session_id_len = 0;
            }
        }
    }
}
