// test/cdp/test_iframe_dom_release.cp
// Tests for CapSurf CDP Child Frame Events Generator & DOM Resource Deallocation

import {
    ChildFrameInfo,
    ChildFrameEventStrings,
    formatFrameAttached,
    formatFrameStartedLoading,
    formatFrameNavigated,
    formatLifecycleEvent,
    formatChildFrameEvents,
    emitChildFrameAttachAndStart,
    emitChildFrameEvents,
    ChildFrameEventEmitter,
} from "../../../src/cdp/iframe_events.cp";
import {
    stripNoscript,
    stripNoscriptOptional,
    freeDetachedDom,
    releaseFrameDom,
    releaseTargetDom,
    releaseAllDomHandles,
    releaseFrameStateDom,
    releaseTargetStateDom,
    createTargetDom,
} from "../../../src/cdp/dom_release.cp";
import { Document, NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import { Matcher } from "../../../src/css/matcher.cp";
import { CdpEvent } from "../../../src/cdp/types.cp";
import { FrameState } from "../../../src/cdp/frames.cp";
import { TargetState } from "../../../src/cdp/types.cp";

fn main() {
    // ---------------------------------------------------------
    // Part 1: iframe_events tests
    // ---------------------------------------------------------
    const frame = ChildFrameInfo{
        frame_id: "capsurf-child-01",
        parent_frame_id: "capsurf-root-00",
        loader_id: "capsurf-loader-01",
        url: "https://example.com/subframe",
    };

    let buf: [2048]u8 = [0; 2048];

    // 1.1 formatFrameAttached
    const att_str = formatFrameAttached(frame, buf[0..]);
    std.debug.assert(std.mem.indexOf(u8, att_str, "\"frameId\":\"capsurf-child-01\"") != null);
    std.debug.assert(std.mem.indexOf(u8, att_str, "\"parentFrameId\":\"capsurf-root-00\"") != null);

    // 1.2 formatFrameStartedLoading
    const start_str = formatFrameStartedLoading(frame, buf[0..]);
    std.debug.assert(std.mem.indexOf(u8, start_str, "\"frameId\":\"capsurf-child-01\"") != null);

    // 1.3 formatFrameNavigated
    const nav_str = formatFrameNavigated(frame, buf[0..]);
    std.debug.assert(std.mem.indexOf(u8, nav_str, "\"parentId\":\"capsurf-root-00\"") != null);
    std.debug.assert(std.mem.indexOf(u8, nav_str, "\"url\":\"https://example.com/subframe\"") != null);
    std.debug.assert(std.mem.indexOf(u8, nav_str, "\"type\":\"Navigation\"") != null);

    // 1.4 formatLifecycleEvent
    const life_str = formatLifecycleEvent(frame, 1700000000.5, buf[0..]);
    std.debug.assert(std.mem.indexOf(u8, life_str, "\"name\":\"init\"") != null);
    std.debug.assert(std.mem.indexOf(u8, life_str, "\"loaderId\":\"capsurf-loader-01\"") != null);

    // 1.5 formatChildFrameEvents (bundle)
    const bundle_str = formatChildFrameEvents(frame, 1700000000.5, buf[0..]);
    std.debug.assert(std.mem.startsWith(u8, bundle_str, "["));
    std.debug.assert(std.mem.endsWith(u8, bundle_str, "]"));
    std.debug.assert(std.mem.indexOf(u8, bundle_str, "Page.frameAttached") != null);
    std.debug.assert(std.mem.indexOf(u8, bundle_str, "Page.frameStartedLoading") != null);
    std.debug.assert(std.mem.indexOf(u8, bundle_str, "Page.frameNavigated") != null);
    std.debug.assert(std.mem.indexOf(u8, bundle_str, "Page.lifecycleEvent") != null);

    // 1.6 emitChildFrameAttachAndStart
    let emit_buf: [2048]u8 = [0; 2048];
    const emitted = emitChildFrameAttachAndStart(frame, 1700000000.5, emit_buf[0..]);
    std.debug.assert(emitted.frame_attached.len > 0);
    std.debug.assert(emitted.frame_started_loading.len > 0);
    std.debug.assert(emitted.frame_navigated.len > 0);
    std.debug.assert(emitted.lifecycle_event.len > 0);

    // 1.7 emitChildFrameEvents into CdpEvent array
    let cdp_evts: [8]CdpEvent = [CdpEvent{}; 8];
    let evt_cnt: usize = 0;
    let json_scratch: [4096]u8 = [0; 4096];
    let frames_slice: [1]ChildFrameInfo = [frame];
    emitChildFrameEvents(
        frames_slice[0..],
        1700000000.5,
        "session-test-01",
        cdp_evts[0..],
        &evt_cnt,
        json_scratch[0..],
    );
    std.debug.assert(evt_cnt == 4);
    std.debug.assert(std.mem.eql(u8, cdp_evts[0].method, "Page.frameAttached"));
    std.debug.assert(std.mem.eql(u8, cdp_evts[1].method, "Page.frameStartedLoading"));
    std.debug.assert(std.mem.eql(u8, cdp_evts[2].method, "Page.frameNavigated"));
    std.debug.assert(std.mem.eql(u8, cdp_evts[3].method, "Page.lifecycleEvent"));

    // 1.8 ChildFrameEventEmitter
    let emitter_buf: [2048]u8 = [0; 2048];
    let emitter = ChildFrameEventEmitter.init(emitter_buf[0..]);
    const all_strings = emitter.emitAll(frame, 1700000000.5);
    std.debug.assert(all_strings.frame_attached.len > 0);
    std.debug.assert(all_strings.frame_started_loading.len > 0);
    std.debug.assert(all_strings.frame_navigated.len > 0);
    std.debug.assert(all_strings.lifecycle_event.len > 0);

    // ---------------------------------------------------------
    // Part 2: dom_release tests
    // ---------------------------------------------------------
    // 2.1 stripNoscript
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    doc.parseHtml(
        "<html><head><noscript><style>.hide{display:none;}</style></noscript></head><body><div id=\"content\"><p>Normal</p></div><noscript><p>Please enable JS</p></noscript></body></html>",
    ) catch unreachable;

    // Before strip: noscript exists
    const before_nid = Matcher.querySelector(&doc, "noscript");
    std.debug.assert(before_nid != NULL_NODE);

    // Strip noscripts
    stripNoscript(&doc);

    // After strip: noscript must be NULL_NODE
    const after_nid = Matcher.querySelector(&doc, "noscript");
    std.debug.assert(after_nid == NULL_NODE);

    // Normal content remains intact
    const content_nid = doc.getElementById("content");
    std.debug.assert(content_nid != NULL_NODE);

    // 2.2 releaseFrameDom with valid and null doc
    releaseFrameDom("capsurf-child-01", &doc);
    releaseFrameDom("capsurf-child-01", null);

    // 2.3 releaseTargetDom with valid and null doc
    let target_doc = Document.init(std.heap.page_allocator) catch unreachable;
    target_doc.parseHtml("<html><body><h1>Target Root</h1></body></html>") catch unreachable;
    releaseTargetDom("capsurf-target-01", &target_doc);
    releaseTargetDom("capsurf-target-01", null);

    // 2.4 releaseAllDomHandles
    releaseAllDomHandles("capsurf-target-01", null);

    // 2.5 releaseFrameStateDom
    let f_state = FrameState{
        frame_id: "frame-state-01",
        srcdoc: "<html><body>srcdoc test</body></html>",
    };
    std.debug.assert(f_state.srcdoc != null);
    releaseFrameStateDom(&f_state, null);
    std.debug.assert(f_state.srcdoc == null);

    // 2.6 releaseTargetStateDom
    let t_state = TargetState{
        id: "target-state-01",
    };
    releaseTargetStateDom(&t_state, null);

    // 2.7 createTargetDom
    let created_doc = createTargetDom(
        std.heap.page_allocator,
        "<html><head><noscript>No script here</noscript></head><body><h2>Created Target</h2></body></html>",
    ) catch unreachable;
    defer freeDetachedDom(&created_doc);

    const created_noscript = Matcher.querySelector(&created_doc, "noscript");
    std.debug.assert(created_noscript == NULL_NODE);

    std.debug.assert(created_doc.root_element != NULL_NODE);

    std.debug.print("ALL IFRAME_EVENTS & DOM_RELEASE TESTS PASSED SUCCESSFULLY!\n", .{});
}
