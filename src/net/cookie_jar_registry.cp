// Migrated from vendor/kitesurf/packages/engine/src/cdp/cookie-jar-registry.ts
// CapSurf Multi-tenant Cookie Jar Registry for CDP connection and browser context isolation

import { CookieJar } from "./cookies.cp";

pub const MAX_COOKIE_REGISTRY_ENTRIES: usize = 16;
pub const MAX_KEY_LEN: usize = 128;

pub struct CookieJarRegistry {
    jars: [16]CookieJar = [CookieJar{}; 16],
    keys: [16][]const u8 = [""; 16],
    key_storage: [2048]u8 = [0; 2048],
    active: [16]bool = [false; 16],
    count: usize = 0,

    static fn init(): CookieJarRegistry {
        return CookieJarRegistry{};
    }

    static fn makeKey(buf: []u8, conn_id: []const u8, ctx_id: []const u8): []const u8 {
        if (ctx_id.len > 0) {
            const needed = conn_id.len + 2 + ctx_id.len;
            if (buf.len < needed) return "";
            @memcpy(buf[0..conn_id.len], conn_id);
            buf[conn_id.len] = ':';
            buf[conn_id.len + 1] = ':';
            @memcpy(buf[conn_id.len + 2 .. needed], ctx_id);
            return buf[0..needed];
        } else {
            if (buf.len < conn_id.len) return "";
            @memcpy(buf[0..conn_id.len], conn_id);
            return buf[0..conn_id.len];
        }
    }

    fn registerCookieJar(*_, key: []const u8, jar: CookieJar): bool {
        if (key.len == 0 || key.len > 128) return false;

        let i: usize = 0;
        while (i < 16) : (i += 1) {
            if (_.active[i] && std.mem.eql(u8, _.keys[i], key)) {
                _.jars[i] = jar;
                return true;
            }
        }

        i = 0;
        while (i < 16) : (i += 1) {
            if (!_.active[i]) {
                const start = i * 128;
                @memcpy(_.key_storage[start .. start + key.len], key);
                _.keys[i] = _.key_storage[start .. start + key.len];
                _.jars[i] = jar;
                _.active[i] = true;
                _.count += 1;
                return true;
            }
        }
        return false;
    }

    fn getCookieJar(*_, key: []const u8): (*CookieJar)? {
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            if (_.active[i] && std.mem.eql(u8, _.keys[i], key)) {
                let slice: []CookieJar = _.jars[i .. i + 1];
                unsafe {
                    return @ptrCast(slice.ptr);
                }
            }
        }
        return null;
    }

    fn unregisterCookieJar(*_, key: []const u8): bool {
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            if (_.active[i] && std.mem.eql(u8, _.keys[i], key)) {
                _.active[i] = false;
                _.keys[i] = "";
                if (_.count > 0) _.count -= 1;
                return true;
            }
        }
        return false;
    }

    fn unregisterConnection(*_, conn_id: []const u8) {
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            if (!_.active[i]) continue;
            const k = _.keys[i];
            let is_match = false;
            if (std.mem.eql(u8, k, conn_id)) {
                is_match = true;
            } else if (k.len > conn_id.len + 2 && std.mem.startsWith(u8, k, conn_id) && std.mem.eql(u8, k[conn_id.len .. conn_id.len + 2], "::")) {
                is_match = true;
            }
            if (is_match) {
                _.active[i] = false;
                _.keys[i] = "";
                if (_.count > 0) _.count -= 1;
            }
        }
    }

    fn unregisterConnectionCookieJars(*_, conn_id: []const u8, context_ids: []const []const u8) {
        * = _.unregisterCookieJar(conn_id);
        let key_buf: [128]u8 = [0; 128];
        for (ctx_id of context_ids) {
            const k = CookieJarRegistry.makeKey(key_buf[0..], conn_id, ctx_id);
            if (k.len > 0) {
                * = _.unregisterCookieJar(k);
            }
        }
    }

    fn size(): usize {
        return _.count;
    }

    fn has(*_, key: []const u8): bool {
        return _.getCookieJar(key) != null;
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            _.active[i] = false;
            _.keys[i] = "";
        }
        _.count = 0;
    }
}

pub threadlocal let g_cookie_jar_registry: CookieJarRegistry = CookieJarRegistry.init();

pub fn makeKey(buf: []u8, conn_id: []const u8, ctx_id: []const u8): []const u8 {
    return CookieJarRegistry.makeKey(buf, conn_id, ctx_id);
}

pub fn cookieKeyFor(buf: []u8, conn_id: []const u8, ctx_id: []const u8): []const u8 {
    return CookieJarRegistry.makeKey(buf, conn_id, ctx_id);
}

pub fn registerCookieJar(key: []const u8, jar: CookieJar): bool {
    return g_cookie_jar_registry.registerCookieJar(key, jar);
}

pub fn getCookieJar(key: []const u8): (*CookieJar)? {
    return g_cookie_jar_registry.getCookieJar(key);
}

pub fn unregisterCookieJar(key: []const u8): bool {
    return g_cookie_jar_registry.unregisterCookieJar(key);
}

pub fn unregisterConnection(conn_id: []const u8) {
    g_cookie_jar_registry.unregisterConnection(conn_id);
}

pub fn unregisterConnectionCookieJars(conn_id: []const u8, context_ids: []const []const u8) {
    g_cookie_jar_registry.unregisterConnectionCookieJars(conn_id, context_ids);
}

pub fn cookieJarRegistrySize(): usize {
    return g_cookie_jar_registry.size();
}

pub fn hasCookieJar(key: []const u8): bool {
    return g_cookie_jar_registry.has(key);
}
