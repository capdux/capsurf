// src/net/http_scheduler.cp
// Clean-room pure-CP non-blocking HTTP transfer scheduler and network idle tracker.
// Uses POSIX poll/PollFd/POLLIN/POLLOUT patterns compatible with src/cdp/server.cp.

pub @c {
    struct NetSockAddrIn {
        sin_family: u16;
        sin_port: u16;
        sin_addr: u32;
        sin_zero: [8]u8;
    }

    struct PollFd {
        fd: i32;
        events: i16;
        revents: i16;
    }

    fn socket(domain: i32, @"type": i32, protocol: i32): i32;
    fn connect(fd: i32, addr: *const NetSockAddrIn, len: u32): i32;
    fn send(fd: i32, buf: [*]const u8, len: usize, flags: i32): isize;
    fn recv(fd: i32, buf: [*]u8, len: usize, flags: i32): isize;
    fn poll(fds: *PollFd, nfds: u64, timeout: i32): i32;
    fn close(fd: i32): i32;
    fn getsockopt(fd: i32, level: i32, optname: i32, optval: *i32, optlen: *u32): i32;
}

pub const POLLIN: i16 = 1;
pub const POLLOUT: i16 = 4;
pub const POLLERR: i16 = 8;
pub const POLLHUP: i16 = 16;
pub const POLLNVAL: i16 = 32;

pub const AF_INET: i32 = 2;
pub const SOCK_STREAM: i32 = 1;
pub const SOCK_NONBLOCK: i32 = 2048;
pub const SOL_SOCKET: i32 = 1;
pub const SO_ERROR: i32 = 4;
pub const MSG_DONTWAIT: i32 = 64;
pub const MSG_NOSIGNAL: i32 = 16384;
pub const EINPROGRESS: i32 = 115;

pub const MAX_INFLIGHT: usize = 16;
pub const MAX_SCHEDULER_SLOTS: usize = 16;
pub const MAX_TRANSFER_SEND_BUF: usize = 1024;
pub const MAX_TRANSFER_RECV_BUF: usize = 4096;

pub struct IdleReadyStatus {
    network_idle: bool = false,
    almost_idle: bool = false,
}

/// Tracks networkidle0 (0 active, 0 queued, no pending dispatch)
/// and networkidle2 (<= 2 active, 0 queued, no pending dispatch).
pub struct NetworkIdleTracker {
    active_count: u32 = 0,
    queued_count: u32 = 0,
    dispatch_pending: bool = false,
    idle_since_ms: i64 = -1,
    almost_idle_since_ms: i64 = -1,
    idle_emitted: bool = false,
    almost_idle_emitted: bool = false,

    static fn init(): NetworkIdleTracker {
        return NetworkIdleTracker{};
    }

    fn onRequestStart(*_) {
        _.active_count += 1;
        _.idle_since_ms = -1;
        _.almost_idle_since_ms = -1;
        _.idle_emitted = false;
        _.almost_idle_emitted = false;
    }

    fn sampleIdle(*_, active: u32, queued: u32, dispatch_pending: bool, now_ms: i64) {
        _.active_count = active;
        _.queued_count = queued;
        _.dispatch_pending = dispatch_pending;

        const is_idle = (active == 0 && queued == 0 && !dispatch_pending);
        if (is_idle) {
            if (_.idle_since_ms < 0) {
                _.idle_since_ms = now_ms;
            }
        } else {
            _.idle_since_ms = -1;
            _.idle_emitted = false;
        }

        const is_almost_idle = (active <= 2 && queued == 0 && !dispatch_pending);
        if (is_almost_idle) {
            if (_.almost_idle_since_ms < 0) {
                _.almost_idle_since_ms = now_ms;
            }
        } else {
            _.almost_idle_since_ms = -1;
            _.almost_idle_emitted = false;
        }
    }

    fn checkIdleReady(*_, now_ms: i64, quiet_ms: i64): IdleReadyStatus {
        let status = IdleReadyStatus{};
        if (!_.idle_emitted && _.idle_since_ms >= 0 && (now_ms - _.idle_since_ms) >= quiet_ms) {
            _.idle_emitted = true;
            status.network_idle = true;
        }
        if (!_.almost_idle_emitted && _.almost_idle_since_ms >= 0 && (now_ms - _.almost_idle_since_ms) >= quiet_ms) {
            _.almost_idle_emitted = true;
            status.almost_idle = true;
        }
        return status;
    }

    fn resetForNavigation(*_) {
        _.active_count = 0;
        _.queued_count = 0;
        _.dispatch_pending = false;
        _.idle_since_ms = -1;
        _.almost_idle_since_ms = -1;
        _.idle_emitted = false;
        _.almost_idle_emitted = false;
    }
}

pub enum TransferState {
    queued,
    connecting,
    writing,
    reading,
    done,
    failed,
    aborted,

    fn isInflight(): bool {
        return switch (_) {
            .connecting, .writing, .reading => true,
            else => false,
        };
    }

    fn isTerminal(): bool {
        return switch (_) {
            .done, .failed, .aborted => true,
            else => false,
        };
    }
}

pub struct TransferSlot {
    active: bool = false,
    id: u32 = 0,
    state: TransferState = .queued,
    fd: i32 = -1,
    ip_addr: u32 = 0,
    port_be: u16 = 0,
    send_buf: [MAX_TRANSFER_SEND_BUF]u8 = [0; MAX_TRANSFER_SEND_BUF],
    send_len: usize = 0,
    send_offset: usize = 0,
    recv_buf: [MAX_TRANSFER_RECV_BUF]u8 = [0; MAX_TRANSFER_RECV_BUF],
    recv_len: usize = 0,
    err_code: i32 = 0,
    started_ms: i64 = 0,
    deadline_ms: i64 = 0,

    fn closeFd(*_) {
        if (_.fd >= 0) {
            * = close(_.fd);
            _.fd = -1;
        }
    }

    fn reset(*_) {
        _.closeFd();
        _.active = false;
        _.id = 0;
        _.state = .queued;
        _.ip_addr = 0;
        _.port_be = 0;
        _.send_len = 0;
        _.send_offset = 0;
        _.recv_len = 0;
        _.err_code = 0;
        _.started_ms = 0;
        _.deadline_ms = 0;
    }

    fn receivedSlice(): []const u8 {
        return _.recv_buf[0.._.recv_len];
    }
}

fn htonsU16(port: u16): u16 {
    return ((port & 0x00FF) << 8) | ((port & 0xFF00) >> 8);
}

pub struct NetScheduler {
    slots: [MAX_SCHEDULER_SLOTS]TransferSlot = [TransferSlot{}; MAX_SCHEDULER_SLOTS],
    next_id: u32 = 1,

    static fn init(): NetScheduler {
        return NetScheduler{};
    }

    fn inflightCount(*_): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < MAX_SCHEDULER_SLOTS) : (i += 1) {
            if (_.slots[i].active && _.slots[i].state.isInflight()) {
                count += 1;
            }
        }
        return count;
    }

    fn queuedCount(*_): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < MAX_SCHEDULER_SLOTS) : (i += 1) {
            if (_.slots[i].active && _.slots[i].state == .queued) {
                count += 1;
            }
        }
        return count;
    }

    fn hasPendingDispatch(*_): bool {
        return _.queuedCount() > 0 && _.inflightCount() < MAX_INFLIGHT;
    }

    /// Submits a new transfer into a free slot in `.queued` state.
    /// Returns the assigned TransferId (> 0), or 0 if no slot is available or request exceeds buffer.
    fn submit(*_, ip_addr: u32, port: u16, request_bytes: []const u8, now_ms: i64, timeout_ms: i64): u32 {
        if (request_bytes.len > MAX_TRANSFER_SEND_BUF) {
            return 0;
        }
        let i: usize = 0;
        while (i < MAX_SCHEDULER_SLOTS) : (i += 1) {
            if (!_.slots[i].active) {
                const tid = _.next_id;
                _.next_id += 1;
                if (_.next_id == 0) _.next_id = 1;

                _.slots[i].reset();
                _.slots[i].active = true;
                _.slots[i].id = tid;
                _.slots[i].state = .queued;
                _.slots[i].ip_addr = ip_addr;
                _.slots[i].port_be = htonsU16(port);
                _.slots[i].send_len = request_bytes.len;
                if (request_bytes.len > 0) {
                    std.mem.copyForwards(u8, _.slots[i].send_buf[0..request_bytes.len], request_bytes);
                }
                _.slots[i].started_ms = now_ms;
                _.slots[i].deadline_ms = if (timeout_ms > 0) (now_ms + timeout_ms) else 0;
                return tid;
            }
        }
        return 0;
    }

    /// Starts queued transfers up to `MAX_INFLIGHT`. Returns number of transfers started.
    fn startPending(*_, now_ms: i64): usize {
        let inflight = _.inflightCount();
        let started: usize = 0;
        let i: usize = 0;
        while (i < MAX_SCHEDULER_SLOTS && inflight < MAX_INFLIGHT) : (i += 1) {
            if (!_.slots[i].active || _.slots[i].state != .queued) {
                continue;
            }
            if (_.slots[i].deadline_ms > 0 && now_ms >= _.slots[i].deadline_ms) {
                _.slots[i].state = .failed;
                _.slots[i].err_code = -1;
                continue;
            }
            const fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
            if (fd < 0) {
                _.slots[i].state = .failed;
                _.slots[i].err_code = -2;
                continue;
            }
            _.slots[i].fd = fd;
            const addr = NetSockAddrIn{
                sin_family: @as(u16, AF_INET),
                sin_port: _.slots[i].port_be,
                sin_addr: _.slots[i].ip_addr,
                sin_zero: [0; 8],
            };
            const rc = connect(fd, &addr, @as(u32, @sizeOf(NetSockAddrIn)));
            if (rc == 0) {
                _.slots[i].state = if (_.slots[i].send_len > 0) .writing else .reading;
                inflight += 1;
                started += 1;
            } else {
                let so_err: i32 = 0;
                let so_len: u32 = @as(u32, @sizeOf(i32));
                const gsrc = getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_err, &so_len);
                if (gsrc == 0 && (so_err == 0 || so_err == EINPROGRESS)) {
                    _.slots[i].state = .connecting;
                    inflight += 1;
                    started += 1;
                } else {
                    _.slots[i].closeFd();
                    _.slots[i].state = .failed;
                    _.slots[i].err_code = if (so_err != 0) so_err else -3;
                }
            }
        }
        return started;
    }

    /// Populates `fds` and `slot_indices` for all active non-blocking sockets.
    /// Returns the number of PollFd entries written.
    fn fillPollFds(fds: []PollFd, slot_indices: []usize): usize {
        const cap = if (fds.len < slot_indices.len) fds.len else slot_indices.len;
        let nfds: usize = 0;
        let i: usize = 0;
        while (i < MAX_SCHEDULER_SLOTS && nfds < cap) : (i += 1) {
            if (!_.slots[i].active || _.slots[i].fd < 0) {
                continue;
            }
            const ev: i16 = switch (_.slots[i].state) {
                .connecting, .writing => POLLOUT,
                .reading => POLLIN,
                else => 0,
            };
            if (ev == 0) {
                continue;
            }
            fds[nfds] = PollFd{
                fd: _.slots[i].fd,
                events: ev,
                revents: 0,
            };
            slot_indices[nfds] = i;
            nfds += 1;
        }
        return nfds;
    }

    /// Advances active socket state machines based on `poll` results and deadlines.
    /// Returns the number of slots that made progress or transitioned state.
    fn stepSockets(*_, fds: []const PollFd, slot_indices: []const usize, count: usize, now_ms: i64): usize {
        let progressed: usize = 0;
        let p: usize = 0;
        while (p < count && p < fds.len && p < slot_indices.len) : (p += 1) {
            const idx = slot_indices[p];
            if (idx >= MAX_SCHEDULER_SLOTS) continue;
            if (!_.slots[idx].active || _.slots[idx].fd < 0) continue;

            const revents = fds[p].revents;
            if ((revents & (POLLERR | POLLNVAL)) != 0) {
                _.slots[idx].closeFd();
                _.slots[idx].state = .failed;
                _.slots[idx].err_code = -4;
                progressed += 1;
                continue;
            }

            if (_.slots[idx].state == .connecting) {
                if ((revents & POLLOUT) != 0) {
                    let so_err: i32 = 0;
                    let so_len: u32 = @as(u32, @sizeOf(i32));
                    const gsrc = getsockopt(_.slots[idx].fd, SOL_SOCKET, SO_ERROR, &so_err, &so_len);
                    if (gsrc == 0 && so_err == 0) {
                        _.slots[idx].state = if (_.slots[idx].send_len > 0) .writing else .reading;
                    } else {
                        _.slots[idx].closeFd();
                        _.slots[idx].state = .failed;
                        _.slots[idx].err_code = if (so_err != 0) so_err else -5;
                    }
                    progressed += 1;
                }
            } else if (_.slots[idx].state == .writing) {
                if ((revents & POLLOUT) != 0) {
                    const rem = _.slots[idx].send_len - _.slots[idx].send_offset;
                    if (rem > 0) {
                        const off = _.slots[idx].send_offset;
                        const send_slice: []const u8 = _.slots[idx].send_buf[off.._.slots[idx].send_len];
                        const sent = send(_.slots[idx].fd, send_slice.ptr, rem, MSG_DONTWAIT | MSG_NOSIGNAL);
                        if (sent > 0) {
                            _.slots[idx].send_offset += @as(usize, @intCast(sent));
                            progressed += 1;
                        } else if (sent == 0) {
                            _.slots[idx].closeFd();
                            _.slots[idx].state = .failed;
                            _.slots[idx].err_code = -6;
                            progressed += 1;
                            continue;
                        }
                    }
                    if (_.slots[idx].send_offset >= _.slots[idx].send_len) {
                        _.slots[idx].state = .reading;
                        progressed += 1;
                    }
                }
            } else if (_.slots[idx].state == .reading) {
                if ((revents & (POLLIN | POLLHUP)) != 0) {
                    const avail = MAX_TRANSFER_RECV_BUF - _.slots[idx].recv_len;
                    if (avail == 0) {
                        _.slots[idx].closeFd();
                        _.slots[idx].state = .done;
                        progressed += 1;
                    } else {
                        const off = _.slots[idx].recv_len;
                        const recv_slice: []u8 = _.slots[idx].recv_buf[off..];
                        const n = recv(_.slots[idx].fd, recv_slice.ptr, avail, MSG_DONTWAIT);
                        if (n > 0) {
                            _.slots[idx].recv_len += @as(usize, @intCast(n));
                            progressed += 1;
                        } else if (n == 0) {
                            _.slots[idx].closeFd();
                            _.slots[idx].state = .done;
                            progressed += 1;
                        } else if ((revents & POLLHUP) != 0) {
                            _.slots[idx].closeFd();
                            _.slots[idx].state = .done;
                            progressed += 1;
                        }
                    }
                }
            }
        }

        // Enforce per-slot deadlines.
        let i: usize = 0;
        while (i < MAX_SCHEDULER_SLOTS) : (i += 1) {
            if (!_.slots[i].active || _.slots[i].state.isTerminal()) continue;
            if (_.slots[i].deadline_ms > 0 && now_ms >= _.slots[i].deadline_ms) {
                _.slots[i].closeFd();
                _.slots[i].state = .failed;
                _.slots[i].err_code = -7;
                progressed += 1;
            }
        }
        return progressed;
    }

    /// Aborts all active non-terminal transfers and closes their sockets.
    fn abortAll(*_) {
        let i: usize = 0;
        while (i < MAX_SCHEDULER_SLOTS) : (i += 1) {
            if (!_.slots[i].active) continue;
            if (!_.slots[i].state.isTerminal()) {
                _.slots[i].closeFd();
                _.slots[i].state = .aborted;
            }
        }
    }

    fn findSlotIndex(transfer_id: u32): usize? {
        let i: usize = 0;
        while (i < MAX_SCHEDULER_SLOTS) : (i += 1) {
            if (_.slots[i].active && _.slots[i].id == transfer_id) {
                return i;
            }
        }
        return null;
    }

    fn releaseSlot(*_, transfer_id: u32): bool {
        if (idx := _.findSlotIndex(transfer_id)) {
            _.slots[idx].reset();
            return true;
        }
        return false;
    }
}
