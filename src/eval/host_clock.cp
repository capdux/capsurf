// Migrated from vendor/kitesurf/packages/engine/src/eval/host-clock.ts
// Builds an epoch-millisecond clock from host hook or system real time.

@c {
    struct TimeVal {
        tv_sec: i64;
        tv_usec: i64;
    }
    fn gettimeofday(tv: *TimeVal, tz: *!?): i32;
}

pub fn getSystemMilliTimestamp(): f64 {
    let tv = TimeVal{ tv_sec: 0, tv_usec: 0 };
    * = gettimeofday(&tv, null);
    return @as(f64, @floatFromInt(tv.tv_sec)) * 1000.0 + @as(f64, @floatFromInt(tv.tv_usec)) / 1000.0;
}

pub fn getHostMilliTimestamp(): i64 {
    let tv = TimeVal{ tv_sec: 0, tv_usec: 0 };
    * = gettimeofday(&tv, null);
    return tv.tv_sec * 1000 + @divTrunc(tv.tv_usec, 1000);
}

pub fn getHostNowMs(): u64 {
    let tv = TimeVal{ tv_sec: 0, tv_usec: 0 };
    * = gettimeofday(&tv, null);
    if (tv.tv_sec <= 0) return 0;
    return @as(u64, @intCast(tv.tv_sec)) * 1000 + @as(u64, @intCast(tv.tv_usec)) / 1000;
}

pub type HostNowFn = *const fn(user_data: *!?): f64;

pub struct HostClock {
    time_origin: f64 = 0.0,
    now_fn: HostNowFn? = null,
    user_data: *!? = null,
}

pub struct WallClock {
    time_origin: f64 = 0.0,
    has_host_clock: bool = false,
    now_fn: HostNowFn? = null,
    user_data: *!? = null,

    static fn init(): WallClock {
        return WallClock{};
    }

    static fn withHostClock(clock: HostClock): WallClock {
        return WallClock{
            time_origin: clock.time_origin,
            has_host_clock: clock.now_fn != null,
            now_fn: clock.now_fn,
            user_data: clock.user_data,
        };
    }

    static fn create(time_origin: f64, now_fn: HostNowFn?, user_data: *!?): WallClock {
        return WallClock{
            time_origin: time_origin,
            has_host_clock: now_fn != null,
            now_fn: now_fn,
            user_data: user_data,
        };
    }

    fn now(): f64 {
        if (_.has_host_clock) {
            if (f := _.now_fn) {
                return _.time_origin + f(_.user_data);
            }
        }
        return getSystemMilliTimestamp();
    }

    fn nowMs(): i64 {
        return @as(i64, @intFromFloat(_.now()));
    }
}

pub fn makeWallClock(host_clock: HostClock?): WallClock {
    if (hc := host_clock) {
        return WallClock.withHostClock(hc);
    }
    return WallClock.init();
}
