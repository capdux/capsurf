// test/cdp/test_screencast_resources.cp
// CapSurf Phase 13 Comprehensive Test Suite: ResourceStore & Screencast Protocol

import {
    ResourceStore,
    ResourceKind,
    ENGINE_KEY_MARKER,
} from "../../../src/cdp/resource_store.cp";

import {
    ScreencastState,
    ScreencastHandler,
    formatScreencastFrameEvent,
    formatScreencastVisibilityChangedEvent,
} from "../../../src/cdp/screencast.cp";

import { CdpPageSession } from "../../../src/cdp/session.cp";
import { CdpRequest, CdpResponse, CdpEvent } from "../../../src/cdp/types.cp";
import { CdpDispatcher } from "../../../src/cdp/dispatch.cp";

fn expect(cond: bool) {
    if (!cond) @panic("assertion failed");
}

fn testResourceStore() {
    let store = ResourceStore.init();
    expect(store.isEmpty());
    expect(store.totalBytes() == 0);
    expect(store.entryCount() == 0);

    // 1. Check mintKey bit 63 marker
    const k1 = store.mintKey();
    expect((k1 & ENGINE_KEY_MARKER) != 0);

    // 2. Put and retrieve Image
    const img_data = "\x89PNG\r\n\x1a\n\x00\x00\x00\x0dIHDR_TEST_PNG_DATA";
    const img_key = store.putImage("conn-01", "target-01", img_data);
    expect(img_key != null);

    if (k := img_key) {
        expect((k & ENGINE_KEY_MARKER) != 0);
        expect(store.has(k));
        const got_img = store.getImage(k);
        expect(got_img != null);
        if (img := got_img) {
            expect(std.mem.eql(u8, img, img_data));
        }
    }

    // 3. Put and retrieve Font
    const font_data = "OTTO_FONT_GLYPH_TABLE_BYTES";
    const font_key = store.putFont("conn-01", "target-02", font_data);
    expect(font_key != null);

    if (k := font_key) {
        const got_font = store.getFont(k);
        expect(got_font != null);
        if (f := got_font) {
            expect(std.mem.eql(u8, f, font_data));
        }
    }

    expect(store.entryCount() == 2);
    expect(store.totalBytes() > 0);

    // 4. Drop target removes only target-02 font, keeps target-01 image
    const dropped_target = store.dropTarget("conn-01", "target-02");
    expect(dropped_target == 1);
    expect(store.entryCount() == 1);

    if (k := font_key) {
        expect(!store.has(k));
    }
    if (k := img_key) {
        expect(store.has(k));
    }

    // 5. Drop connection removes remaining resources
    const dropped_conn = store.dropConnection("conn-01");
    expect(dropped_conn == 1);
    expect(store.entryCount() == 0);
    expect(store.totalBytes() == 0);
    expect(store.isEmpty());
}

fn testScreencastProtocol() {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    let out_buf: [4096]u8 = undefined;
    let out_evts: [128]CdpEvent = [CdpEvent{}; 128];
    let evt_cnt: usize = 0;

    expect(!session.screencast_handler.isActive());

    // 1. Page.startScreencast
    const req_start = CdpRequest{
        id: 1,
        method: "Page.startScreencast",
        params_raw: "{\"format\":\"png\",\"quality\":90,\"maxWidth\":1920,\"maxHeight\":1080,\"everyNthFrame\":1}",
    };
    const resp_start = CdpDispatcher.dispatch(session, req_start, out_buf[0..], &out_evts, &evt_cnt);
    expect(resp_start.isOk());
    expect(session.screencast_handler.isActive());

    const state = session.screencast_handler.getState();
    expect(state.active);
    expect(state.max_width == 1920);
    expect(state.max_height == 1080);
    expect(std.mem.eql(u8, state.format, "png"));
    expect(state.quality == 90);

    // 2. Next frame sequence & event formatting
    const sid1 = session.screencast_handler.nextFrameSeq();
    expect(sid1 == 1);
    expect(session.screencast_handler.state.hasPendingAck());

    let evt_buf: [2048]u8 = undefined;
    const evt_json = formatScreencastFrameEvent(sid1, "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==", 1920, 1080, 1234567.89, evt_buf[0..]);
    expect(evt_json != null);
    if (j := evt_json) {
        expect(std.mem.indexOf(u8, j, "\"Page.screencastFrame\"") != null);
        expect(std.mem.indexOf(u8, j, "\"deviceWidth\":1920") != null);
        expect(std.mem.indexOf(u8, j, "\"deviceHeight\":1080") != null);
        expect(std.mem.indexOf(u8, j, "\"sessionId\":1") != null);
    }

    // 3. Page.screencastFrameAck
    const req_ack = CdpRequest{
        id: 2,
        method: "Page.screencastFrameAck",
        params_raw: "{\"sessionId\":1}",
    };
    const resp_ack = CdpDispatcher.dispatch(session, req_ack, out_buf[0..], &out_evts, &evt_cnt);
    expect(resp_ack.isOk());
    expect(!session.screencast_handler.state.hasPendingAck());
    expect(session.screencast_handler.state.last_acked_seq == 1);

    // 4. Page.stopScreencast
    const req_stop = CdpRequest{
        id: 3,
        method: "Page.stopScreencast",
        params_raw: "{}",
    };
    const resp_stop = CdpDispatcher.dispatch(session, req_stop, out_buf[0..], &out_evts, &evt_cnt);
    expect(resp_stop.isOk());
    expect(!session.screencast_handler.isActive());
}

fn main() {
    testResourceStore();
    testScreencastProtocol();
    std.debug.print("ALL PHASE 13 RESOURCE STORE & SCREENCAST TESTS PASSED SUCCESSFULLY!\n", .{});
}
