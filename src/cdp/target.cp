// src/cdp/target.cp
// CDP Target Domain Handler for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/target.ts

import { CdpRequest, CdpResponse, CdpEvent, extractJsonString, extractJsonBool, extractJsonInt, MAX_CDP_EVENTS } from "./types.cp";
import { CdpPageSession } from "./session.cp";
import { CookieJar } from "../net/cookies.cp";

pub struct BrowserContextState {
    id_buf: [32]u8 = [0; 32],
    id_len: usize = 0,
    active: bool = false,
    cookie_jar: CookieJar = CookieJar{},

    fn id(*_): []const u8 {
        return _.id_buf[0.._.id_len];
    }
}

pub struct TargetItem {
    target_id_buf: [64]u8 = [0; 64],
    target_id_len: usize = 0,
    type_buf: [32]u8 = [0; 32],
    type_len: usize = 0,
    title_buf: [128]u8 = [0; 128],
    title_len: usize = 0,
    url_buf: [256]u8 = [0; 256],
    url_len: usize = 0,
    browser_context_id_buf: [64]u8 = [0; 64],
    browser_context_id_len: usize = 0,
    session_id_buf: [64]u8 = [0; 64],
    session_id_len: usize = 0,
    attached: bool = false,
    active: bool = false,

    fn targetId(*_): []const u8 {
        return _.target_id_buf[0.._.target_id_len];
    }

    fn targetType(*_): []const u8 {
        return if (_.type_len > 0) _.type_buf[0.._.type_len] else "page";
    }

    fn title(*_): []const u8 {
        return _.title_buf[0.._.title_len];
    }

    fn url(*_): []const u8 {
        return if (_.url_len > 0) _.url_buf[0.._.url_len] else "about:blank";
    }

    fn browserContextId(*_): []const u8 {
        return if (_.browser_context_id_len > 0) _.browser_context_id_buf[0.._.browser_context_id_len] else "default";
    }

    fn sessionId(*_): []const u8 {
        return _.session_id_buf[0.._.session_id_len];
    }
}

pub struct TargetRegistry {
    targets: [16]TargetItem = [TargetItem{}; 16],
    contexts: [8]BrowserContextState = [BrowserContextState{}; 8],
    next_target_id: u32 = 2,
    next_context_id: u32 = 1,
    next_session_id: u32 = 2,
    auto_attach: bool = false,
    flatten: bool = false,
    discover_targets: bool = false,
    bctx_json_buf: [512]u8 = [0; 512],
    event_buf0: [512]u8 = [0; 512],
    event_buf1: [512]u8 = [0; 512],
    create_ctx_resp_buf: [64]u8 = [0; 64],
    create_target_resp_buf: [64]u8 = [0; 64],
    attach_resp_buf: [64]u8 = [0; 64],

    static fn init(): TargetRegistry {
        let reg = TargetRegistry {
            targets: [TargetItem{}; 16],
            contexts: [BrowserContextState{}; 8],
            next_target_id: 2,
            next_context_id: 1,
            next_session_id: 2,
            auto_attach: false,
            flatten: false,
            discover_targets: false,
            bctx_json_buf: [0; 512],
            event_buf0: [0; 512],
            event_buf1: [0; 512],
            create_ctx_resp_buf: [0; 64],
            create_target_resp_buf: [0; 64],
            attach_resp_buf: [0; 64],
        };

        const tid = "capsurf-target-001";
        @memcpy(reg.targets[0].target_id_buf[0..tid.len], tid);
        reg.targets[0].target_id_len = tid.len;

        const ttype = "page";
        @memcpy(reg.targets[0].type_buf[0..ttype.len], ttype);
        reg.targets[0].type_len = ttype.len;

        const ttitle = "CapSurf Agent Blank Page";
        @memcpy(reg.targets[0].title_buf[0..ttitle.len], ttitle);
        reg.targets[0].title_len = ttitle.len;

        const turl = "about:blank";
        @memcpy(reg.targets[0].url_buf[0..turl.len], turl);
        reg.targets[0].url_len = turl.len;

        const tbctx = "default";
        @memcpy(reg.targets[0].browser_context_id_buf[0..tbctx.len], tbctx);
        reg.targets[0].browser_context_id_len = tbctx.len;

        const tsid = "capsurf-session-001";
        @memcpy(reg.targets[0].session_id_buf[0..tsid.len], tsid);
        reg.targets[0].session_id_len = tsid.len;

        reg.targets[0].attached = true;
        reg.targets[0].active = true;

        return reg;
    }

    fn setAutoAttach(*_, auto_attach: bool, flatten: bool) {
        _.auto_attach = auto_attach;
        _.flatten = flatten;
    }

    fn createBrowserContext(*_): []const u8 {
        let i: usize = 0;
        while (i < _.contexts.len) : (i += 1) {
            if (!_.contexts[i].active) {
                const cid = _.next_context_id;
                _.next_context_id += 1;
                const formatted = std.fmt.bufPrint(&_.contexts[i].id_buf, "ctx-{d:0>3}", .{cid}) catch return "";
                _.contexts[i].id_len = formatted.len;
                _.contexts[i].active = true;
                _.contexts[i].cookie_jar = CookieJar.init(std.heap.page_allocator);
                return _.contexts[i].id();
            }
        }
        return "";
    }

    fn disposeBrowserContext(*_, ctx_id: []const u8): bool {
        let found = false;
        let i: usize = 0;
        while (i < _.contexts.len) : (i += 1) {
            if (_.contexts[i].active && std.mem.eql(u8, _.contexts[i].id(), ctx_id)) {
                _.contexts[i].active = false;
                _.contexts[i].cookie_jar.deinit();
                _.contexts[i].cookie_jar = CookieJar.init(std.heap.page_allocator);
                found = true;
                break;
            }
        }
        if (!found) return false;

        let t: usize = 0;
        while (t < _.targets.len) : (t += 1) {
            if (_.targets[t].active && std.mem.eql(u8, _.targets[t].browserContextId(), ctx_id)) {
                _.targets[t].active = false;
                _.targets[t].attached = false;
                _.targets[t].session_id_len = 0;
            }
        }
        return true;
    }

    fn getCookieJarForContext(*_, ctx_id: []const u8): (*CookieJar)? {
        let i: usize = 0;
        while (i < _.contexts.len) : (i += 1) {
            if (_.contexts[i].active && std.mem.eql(u8, _.contexts[i].id(), ctx_id)) {
                return &_.contexts[i].cookie_jar;
            }
        }
        return null;
    }

    fn getBrowserContextIds(*_): []const u8 {
        if (_.bctx_json_buf.len < 2) return "[]";
        _.bctx_json_buf[0] = '[';
        let pos: usize = 1;
        let first = true;
        let i: usize = 0;
        while (i < _.contexts.len) : (i += 1) {
            if (_.contexts[i].active) {
                if (!first) {
                    if (pos >= _.bctx_json_buf.len) break;
                    _.bctx_json_buf[pos] = ',';
                    pos += 1;
                }
                const cid = _.contexts[i].id();
                if (pos + cid.len + 2 > _.bctx_json_buf.len) break;
                _.bctx_json_buf[pos] = '"';
                pos += 1;
                @memcpy(_.bctx_json_buf[pos .. pos + cid.len], cid);
                pos += cid.len;
                _.bctx_json_buf[pos] = '"';
                pos += 1;
                first = false;
            }
        }
        if (pos < _.bctx_json_buf.len) {
            _.bctx_json_buf[pos] = ']';
            pos += 1;
            return _.bctx_json_buf[0..pos];
        }
        return "[]";
    }

    fn createTargetWithType(*_, url: []const u8, target_type: []const u8, bctx: []const u8): []const u8 {
        let i: usize = 0;
        while (i < _.targets.len) : (i += 1) {
            if (!_.targets[i].active) {
                const tid = _.next_target_id;
                _.next_target_id += 1;
                const id_str = std.fmt.bufPrint(&_.targets[i].target_id_buf, "capsurf-target-{d:0>3}", .{tid}) catch return "";
                _.targets[i].target_id_len = id_str.len;

                const target_url = if (url.len > 0) url else "about:blank";
                const u_len = @min(target_url.len, _.targets[i].url_buf.len);
                @memcpy(_.targets[i].url_buf[0..u_len], target_url[0..u_len]);
                _.targets[i].url_len = u_len;

                const type_str = if (target_type.len > 0) target_type else "page";
                const ty_len = @min(type_str.len, _.targets[i].type_buf.len);
                @memcpy(_.targets[i].type_buf[0..ty_len], type_str[0..ty_len]);
                _.targets[i].type_len = ty_len;

                const title_str = if (std.mem.eql(u8, type_str, "worker")) "CapSurf Worker" else "CapSurf Page";
                const ti_len = @min(title_str.len, _.targets[i].title_buf.len);
                @memcpy(_.targets[i].title_buf[0..ti_len], title_str[0..ti_len]);
                _.targets[i].title_len = ti_len;

                const bctx_str = if (bctx.len > 0) bctx else "default";
                const bc_len = @min(bctx_str.len, _.targets[i].browser_context_id_buf.len);
                @memcpy(_.targets[i].browser_context_id_buf[0..bc_len], bctx_str[0..bc_len]);
                _.targets[i].browser_context_id_len = bc_len;

                _.targets[i].session_id_len = 0;
                _.targets[i].attached = false;
                _.targets[i].active = true;
                return _.targets[i].targetId();
            }
        }
        return "";
    }

    fn createTarget(*_, url: []const u8, bctx: []const u8): []const u8 {
        return _.createTargetWithType(url, "page", bctx);
    }

    fn closeTarget(*_, target_id: []const u8): bool {
        let i: usize = 0;
        while (i < _.targets.len) : (i += 1) {
            if (_.targets[i].active && std.mem.eql(u8, _.targets[i].targetId(), target_id)) {
                _.targets[i].active = false;
                _.targets[i].attached = false;
                _.targets[i].session_id_len = 0;
                return true;
            }
        }
        return false;
    }

    fn attachToTarget(*_, target_id: []const u8): []const u8 {
        let target_idx: usize = 0;
        let found = false;
        let i: usize = 0;
        while (i < _.targets.len) : (i += 1) {
            if (_.targets[i].active && (target_id.len == 0 || std.mem.eql(u8, _.targets[i].targetId(), target_id))) {
                target_idx = i;
                found = true;
                break;
            }
        }
        if (!found) return "";

        if (_.targets[target_idx].session_id_len > 0 && _.targets[target_idx].attached) {
            return _.targets[target_idx].sessionId();
        }

        const sid = _.next_session_id;
        _.next_session_id += 1;
        const sid_str = std.fmt.bufPrint(&_.targets[target_idx].session_id_buf, "capsurf-session-{d:0>3}", .{sid}) catch return "";
        _.targets[target_idx].session_id_len = sid_str.len;
        _.targets[target_idx].attached = true;
        return _.targets[target_idx].sessionId();
    }

    fn detachFromTarget(*_, session_id: []const u8): bool {
        let i: usize = 0;
        while (i < _.targets.len) : (i += 1) {
            if (_.targets[i].active && _.targets[i].attached && std.mem.eql(u8, _.targets[i].sessionId(), session_id)) {
                _.targets[i].attached = false;
                _.targets[i].session_id_len = 0;
                return true;
            }
        }
        return false;
    }

    fn getTargetsJson(*_, out_buf: []u8): []const u8 {
        const prefix = "{\"targetInfos\":[";
        if (out_buf.len < prefix.len + 2) return "{\"targetInfos\":[]}";
        @memcpy(out_buf[0..prefix.len], prefix);
        let pos: usize = prefix.len;
        let first = true;
        let i: usize = 0;
        while (i < _.targets.len) : (i += 1) {
            if (_.targets[i].active) {
                if (!first) {
                    if (pos >= out_buf.len) break;
                    out_buf[pos] = ',';
                    pos += 1;
                }
                const formatted = std.fmt.bufPrint(
                    out_buf[pos..],
                    "{{\"targetId\":\"{s}\",\"type\":\"{s}\",\"title\":\"{s}\",\"url\":\"{s}\",\"attached\":{s},\"browserContextId\":\"{s}\"}}",
                    .{
                        _.targets[i].targetId(),
                        _.targets[i].targetType(),
                        _.targets[i].title(),
                        _.targets[i].url(),
                        if (_.targets[i].attached) "true" else "false",
                        _.targets[i].browserContextId(),
                    },
                ) catch break;
                pos += formatted.len;
                first = false;
            }
        }
        if (pos + 2 <= out_buf.len) {
            out_buf[pos] = ']';
            out_buf[pos + 1] = '}';
            pos += 2;
            return out_buf[0..pos];
        }
        return "{\"targetInfos\":[]}";
    }

    fn getTargetInfoJson(*_, target_id: []const u8, out_buf: []u8): ([]const u8)? {
        let target_idx: usize = 0;
        let found = false;
        let i: usize = 0;
        while (i < _.targets.len) : (i += 1) {
            if (_.targets[i].active && (target_id.len == 0 || std.mem.eql(u8, _.targets[i].targetId(), target_id) || (std.mem.eql(u8, target_id, "browser") && i == 0))) {
                target_idx = i;
                found = true;
                break;
            }
        }
        if (!found) return null;

        const res = std.fmt.bufPrint(
            out_buf,
            "{{\"targetInfo\":{{\"targetId\":\"{s}\",\"type\":\"{s}\",\"title\":\"{s}\",\"url\":\"{s}\",\"attached\":{s},\"canAccessOpener\":false,\"browserContextId\":\"{s}\"}}}}",
            .{
                _.targets[target_idx].targetId(),
                _.targets[target_idx].targetType(),
                _.targets[target_idx].title(),
                _.targets[target_idx].url(),
                if (_.targets[target_idx].attached) "true" else "false",
                _.targets[target_idx].browserContextId(),
            },
        ) catch return null;
        return res;
    }

    fn formatTargetInfo(*_, idx: usize, out_buf: []u8): []const u8 {
        if (idx >= _.targets.len || !_.targets[idx].active) return "{}";
        const res = std.fmt.bufPrint(
            out_buf,
            "{{\"targetId\":\"{s}\",\"type\":\"{s}\",\"title\":\"{s}\",\"url\":\"{s}\",\"attached\":{s},\"canAccessOpener\":false,\"browserContextId\":\"{s}\"}}",
            .{
                _.targets[idx].targetId(),
                _.targets[idx].targetType(),
                _.targets[idx].title(),
                _.targets[idx].url(),
                if (_.targets[idx].attached) "true" else "false",
                _.targets[idx].browserContextId(),
            },
        ) catch return "{}";
        return res;
    }

    fn findTargetIndex(*_, target_id: []const u8): usize? {
        let i: usize = 0;
        while (i < _.targets.len) : (i += 1) {
            if (_.targets[i].active && (target_id.len == 0 || std.mem.eql(u8, _.targets[i].targetId(), target_id))) {
                return i;
            }
        }
        return null;
    }

    fn findTargetIdBySessionId(*_, session_id: []const u8): ([]const u8)? {
        if (session_id.len == 0) return null;
        let i: usize = 0;
        while (i < _.targets.len) : (i += 1) {
            if (_.targets[i].active && _.targets[i].attached && std.mem.eql(u8, _.targets[i].sessionId(), session_id)) {
                return _.targets[i].targetId();
            }
        }
        return null;
    }

    fn hasBrowserContext(*_, ctx_id: []const u8): bool {
        if (ctx_id.len == 0 || std.mem.eql(u8, ctx_id, "default")) return true;
        let i: usize = 0;
        while (i < _.contexts.len) : (i += 1) {
            if (_.contexts[i].active && std.mem.eql(u8, _.contexts[i].id(), ctx_id)) {
                return true;
            }
        }
        return false;
    }
}

pub struct TargetHandler {
    pub static fn handle(
        session: *CdpPageSession,
        req: CdpRequest,
        out_buf: []u8,
        out_events: *[MAX_CDP_EVENTS]CdpEvent,
        event_count: *usize,
    ): CdpResponse {
        event_count.* = 0;

        // Keep slot 0 target in sync with active session state
        if (session.title.len > 0) {
            const t_len = @min(session.title.len, session.target_registry.targets[0].title_buf.len);
            @memcpy(session.target_registry.targets[0].title_buf[0..t_len], session.title[0..t_len]);
            session.target_registry.targets[0].title_len = t_len;
        }
        if (session.url.len > 0) {
            const u_len = @min(session.url.len, session.target_registry.targets[0].url_buf.len);
            @memcpy(session.target_registry.targets[0].url_buf[0..u_len], session.url[0..u_len]);
            session.target_registry.targets[0].url_len = u_len;
        }

        // Target.setDiscoverTargets
        if (std.mem.eql(u8, req.method, "Target.setDiscoverTargets")) {
            const discover = extractJsonBool(req.params_raw, "discover") ?? true;
            session.target_registry.discover_targets = discover;
            if (discover && session.target_registry.targets[0].active) {
                let info_buf: [512]u8 = undefined;
                const info_str = session.target_registry.formatTargetInfo(0, &info_buf);
                const tc_json = std.fmt.bufPrint(&session.target_registry.event_buf0, "{{\"targetInfo\":{s}}}", .{info_str}) catch "{}";
                out_events[0] = CdpEvent.create("Target.targetCreated", tc_json);
                event_count.* = 1;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        // Target.setAutoAttach
        if (std.mem.eql(u8, req.method, "Target.setAutoAttach")) {
            const auto_attach = extractJsonBool(req.params_raw, "autoAttach") ?? true;
            const flatten = extractJsonBool(req.params_raw, "flatten") ?? false;
            session.target_registry.auto_attach = auto_attach;
            session.target_registry.flatten = flatten;

            if (auto_attach) {
                if (!session.target_registry.targets[0].attached || session.target_registry.targets[0].session_id_len == 0) {
                    * = session.target_registry.attachToTarget("capsurf-target-001");
                }
                let info_buf: [512]u8 = undefined;
                const info_str = session.target_registry.formatTargetInfo(0, &info_buf);
                const att_json = std.fmt.bufPrint(
                    &session.target_registry.event_buf0,
                    "{{\"sessionId\":\"{s}\",\"targetInfo\":{s},\"waitingForDebugger\":false}}",
                    .{ session.target_registry.targets[0].sessionId(), info_str },
                ) catch "{}";
                out_events[0] = CdpEvent.create("Target.attachedToTarget", att_json);
                event_count.* = 1;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        // Target.getTargets
        if (std.mem.eql(u8, req.method, "Target.getTargets")) {
            const res = session.target_registry.getTargetsJson(out_buf);
            return CdpResponse.ok(req.id, res);
        }

        // Target.getTargetInfo
        if (std.mem.eql(u8, req.method, "Target.getTargetInfo")) {
            const tid = extractJsonString(req.params_raw, "targetId") ?? "";
            if (std.mem.eql(u8, tid, "kitesurf-page-renderer") || std.mem.eql(u8, tid, "kitesurf-engine")) {
                const title_str = if (std.mem.eql(u8, tid, "kitesurf-page-renderer")) "Kitesurf Page Renderer" else "Kitesurf Engine";
                const res = std.fmt.bufPrint(
                    out_buf,
                    "{{\"targetInfo\":{{\"targetId\":\"{s}\",\"type\":\"other\",\"title\":\"{s}\",\"url\":\"about:blank\",\"attached\":true,\"canAccessOpener\":false,\"browserContextId\":\"default\"}}}}",
                    .{ tid, title_str },
                ) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, res);
            }
            const info_opt = session.target_registry.getTargetInfoJson(tid, out_buf);
            if (info_opt == null) {
                return CdpResponse.err(req.id, -32000, "No target with given id");
            }
            return CdpResponse.ok(req.id, info_opt.?);
        }

        // Target.createBrowserContext
        if (std.mem.eql(u8, req.method, "Target.createBrowserContext")) {
            const cid = session.target_registry.createBrowserContext();
            if (cid.len == 0) {
                return CdpResponse.err(req.id, -32000, "Browser context limit reached");
            }
            const res = std.fmt.bufPrint(&session.target_registry.create_ctx_resp_buf, "{{\"browserContextId\":\"{s}\"}}", .{cid}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        // Target.disposeBrowserContext
        if (std.mem.eql(u8, req.method, "Target.disposeBrowserContext")) {
            const cid = extractJsonString(req.params_raw, "browserContextId") ?? "";
            * = session.target_registry.disposeBrowserContext(cid);
            return CdpResponse.ok(req.id, "{}");
        }

        // Target.getBrowserContexts
        if (std.mem.eql(u8, req.method, "Target.getBrowserContexts")) {
            const ids_json = session.target_registry.getBrowserContextIds();
            const res = std.fmt.bufPrint(out_buf, "{{\"browserContextIds\":{s}}}", .{ids_json}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        // Target.createTarget
        if (std.mem.eql(u8, req.method, "Target.createTarget")) {
            const bctx = extractJsonString(req.params_raw, "browserContextId") ?? "";
            if (bctx.len > 0 && !session.target_registry.hasBrowserContext(bctx)) {
                return CdpResponse.err(req.id, -32000, "Failed to find browser context");
            }
            const url = extractJsonString(req.params_raw, "url") ?? "about:blank";
            const target_id = session.target_registry.createTarget(url, bctx);
            if (target_id.len == 0) {
                return CdpResponse.err(req.id, -32000, "Target limit reached");
            }

            const t_idx_opt = session.target_registry.findTargetIndex(target_id);
            if (t_idx_opt != null) {
                const t_idx = t_idx_opt.?;
                let info_buf: [512]u8 = undefined;
                const info_str = session.target_registry.formatTargetInfo(t_idx, &info_buf);
                const tc_json = std.fmt.bufPrint(&session.target_registry.event_buf0, "{{\"targetInfo\":{s}}}", .{info_str}) catch "{}";
                out_events[0] = CdpEvent.create("Target.targetCreated", tc_json);
                event_count.* = 1;

                if (session.target_registry.auto_attach) {
                    const sid = session.target_registry.attachToTarget(target_id);
                    const att_json = std.fmt.bufPrint(
                        &session.target_registry.event_buf1,
                        "{{\"sessionId\":\"{s}\",\"targetInfo\":{s},\"waitingForDebugger\":false}}",
                        .{ sid, info_str },
                    ) catch "{}";
                    out_events[1] = CdpEvent.create("Target.attachedToTarget", att_json);
                    event_count.* = 2;
                }
            }

            const res = std.fmt.bufPrint(&session.target_registry.create_target_resp_buf, "{{\"targetId\":\"{s}\"}}", .{target_id}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        // Target.closeTarget
        if (std.mem.eql(u8, req.method, "Target.closeTarget")) {
            const target_id = extractJsonString(req.params_raw, "targetId") ?? "";
            const existed = session.target_registry.closeTarget(target_id);
            if (existed) {
                const td_json = std.fmt.bufPrint(&session.target_registry.event_buf0, "{{\"targetId\":\"{s}\"}}", .{target_id}) catch "{}";
                out_events[0] = CdpEvent.create("Target.targetDestroyed", td_json);
                event_count.* = 1;
            }
            return CdpResponse.ok(req.id, if (existed) "{\"success\":true}" else "{\"success\":false}");
        }

        // Target.attachToTarget
        if (std.mem.eql(u8, req.method, "Target.attachToTarget")) {
            const target_id = extractJsonString(req.params_raw, "targetId") ?? "";
            if (std.mem.eql(u8, target_id, "kitesurf-page-renderer") || std.mem.eql(u8, target_id, "kitesurf-engine")) {
                const is_renderer = std.mem.eql(u8, target_id, "kitesurf-page-renderer");
                const sid = if (is_renderer) "capsurf-session-renderer" else "capsurf-session-engine";
                const title_str = if (is_renderer) "Kitesurf Page Renderer" else "Kitesurf Engine";
                const att_json = std.fmt.bufPrint(
                    &session.target_registry.event_buf0,
                    "{{\"sessionId\":\"{s}\",\"targetInfo\":{{\"targetId\":\"{s}\",\"type\":\"other\",\"title\":\"{s}\",\"url\":\"about:blank\",\"attached\":true,\"canAccessOpener\":false,\"browserContextId\":\"default\"}},\"waitingForDebugger\":false}}",
                    .{ sid, target_id, title_str },
                ) catch "{}";
                out_events[0] = CdpEvent.create("Target.attachedToTarget", att_json);
                event_count.* = 1;
                const res = std.fmt.bufPrint(&session.target_registry.attach_resp_buf, "{{\"sessionId\":\"{s}\"}}", .{sid}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
                return CdpResponse.ok(req.id, res);
            }
            const sid = session.target_registry.attachToTarget(target_id);
            if (sid.len == 0) {
                return CdpResponse.err(req.id, -32000, "No target with given id");
            }
            const t_idx_opt = session.target_registry.findTargetIndex(target_id);
            if (t_idx_opt != null) {
                const t_idx = t_idx_opt.?;
                let info_buf: [512]u8 = undefined;
                const info_str = session.target_registry.formatTargetInfo(t_idx, &info_buf);
                const att_json = std.fmt.bufPrint(
                    &session.target_registry.event_buf0,
                    "{{\"sessionId\":\"{s}\",\"targetInfo\":{s},\"waitingForDebugger\":false}}",
                    .{ sid, info_str },
                ) catch "{}";
                out_events[0] = CdpEvent.create("Target.attachedToTarget", att_json);
                event_count.* = 1;
            }
            const res = std.fmt.bufPrint(&session.target_registry.attach_resp_buf, "{{\"sessionId\":\"{s}\"}}", .{sid}) catch return CdpResponse.err(req.id, -32000, "Buffer overflow");
            return CdpResponse.ok(req.id, res);
        }

        // Target.detachFromTarget
        if (std.mem.eql(u8, req.method, "Target.detachFromTarget")) {
            const sid = extractJsonString(req.params_raw, "sessionId") ?? "";
            let tid: []const u8 = "";
            let i: usize = 0;
            while (i < session.target_registry.targets.len) : (i += 1) {
                if (session.target_registry.targets[i].active && session.target_registry.targets[i].attached && std.mem.eql(u8, session.target_registry.targets[i].sessionId(), sid)) {
                    tid = session.target_registry.targets[i].targetId();
                    break;
                }
            }
            const success = session.target_registry.detachFromTarget(sid);
            if (success && tid.len > 0) {
                const det_json = std.fmt.bufPrint(
                    &session.target_registry.event_buf0,
                    "{{\"sessionId\":\"{s}\",\"targetId\":\"{s}\"}}",
                    .{ sid, tid },
                ) catch "{}";
                out_events[0] = CdpEvent.create("Target.detachedFromTarget", det_json);
                event_count.* = 1;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        // Target.activateTarget
        if (std.mem.eql(u8, req.method, "Target.activateTarget")) {
            return CdpResponse.ok(req.id, "{}");
        }

        // Stubs for Puppeteer / Playwright compatibility
        if (std.mem.eql(u8, req.method, "Target.setRemoteLocations") ||
            std.mem.eql(u8, req.method, "Target.exposeDevToolsProtocol") ||
            std.mem.eql(u8, req.method, "Target.sendMessageToTarget")) {
            return CdpResponse.ok(req.id, "{}");
        }

        if (std.mem.eql(u8, req.method, "Target.attachToBrowserTarget")) {
            return CdpResponse.ok(req.id, "{\"sessionId\":\"capsurf-session-browser\"}");
        }

        return CdpResponse.err(req.id, -32601, "'Target' domain method not implemented");
    }
}
