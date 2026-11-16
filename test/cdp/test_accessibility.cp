// test/cdp/test_accessibility.cp
// CapSurf Phase 14 Comprehensive Test Suite: W3C Accessibility Domain & AXTree Engine

import { Document, NodeId, NULL_NODE } from "../../src/dom/dom.cp";
import { buildAXTree, AccessibilityHandler, AXNode, AXTree } from "../../src/cdp/accessibility.cp";
import { CdpPageSession } from "../../src/cdp/session.cp";
import { CdpRequest, CdpResponse, CdpEvent } from "../../src/cdp/types.cp";
import { CdpDispatcher } from "../../src/cdp/dispatch.cp";

fn expect(cond: bool) {
    if (!cond) @panic("assertion failed");
}

fn testAXTreeBuilding() {
    let doc = Document.init(std.heap.page_allocator) catch unreachable;
    defer doc.deinit();

    const html = 
        "<!DOCTYPE html>" ++
        "<html>" ++
        "  <head><title>AX Test</title></head>" ++
        "  <body>" ++
        "    <header role=\"banner\">Header Content</header>" ++
        "    <main>" ++
        "      <h1 id=\"title\">Accessibility Suite</h1>" ++
        "      <button aria-label=\"Custom Submit Action\">OK</button>" ++
        "      <a href=\"https://example.com\">Visit Link</a>" ++
        "      <input type=\"checkbox\" checked id=\"agree\">" ++
        "      <input type=\"text\" placeholder=\"Your username\">" ++
        "      <div style=\"display:none;\"><p>Hidden content</p></div>" ++
        "      <span style=\"visibility:hidden;\">Invisible text</span>" ++
        "    </main>" ++
        "  </body>" ++
        "</html>";

    * = doc.parseHtml(html) catch unreachable;

    let buf: [16384]u8 = [0; 16384];
    const json = buildAXTree(&doc, buf[0..]);
    expect(json.len > 0);

    // 1. Valid JSON root structure
    expect(std.mem.startsWith(u8, json, "{\"nodes\":["));
    expect(std.mem.endsWith(u8, json, "]}"));

    // 2. Contains RootWebArea root role
    expect(std.mem.indexOf(u8, json, "\"RootWebArea\"") != null);

    // 3. Contains explicit ARIA role "banner"
    expect(std.mem.indexOf(u8, json, "\"banner\"") != null);

    // 4. Contains computed accessible name from aria-label
    expect(std.mem.indexOf(u8, json, "\"Custom Submit Action\"") != null);

    // 5. Contains semantic roles: button, link, heading, checkbox, textbox
    expect(std.mem.indexOf(u8, json, "\"button\"") != null);
    expect(std.mem.indexOf(u8, json, "\"link\"") != null);
    expect(std.mem.indexOf(u8, json, "\"heading\"") != null);
    expect(std.mem.indexOf(u8, json, "\"checkbox\"") != null);
    expect(std.mem.indexOf(u8, json, "\"textbox\"") != null);

    // 6. Contains placeholder name
    expect(std.mem.indexOf(u8, json, "\"Your username\"") != null);

    // 7. Contains checked property
    expect(std.mem.indexOf(u8, json, "\"checked\"") != null);

    // 8. Hidden elements marked as ignored
    expect(std.mem.indexOf(u8, json, "\"ignored\":true") != null);
}

fn testAccessibilityProtocol() {
    let session = CdpPageSession.init(std.heap.page_allocator) catch unreachable;
    defer session.deinit();

    const sample_html = 
        "<!DOCTYPE html><html><body>" ++
        "<button id=\"test-btn\">Click Me</button>" ++
        "</body></html>";
    * = session.doc.parseHtml(sample_html) catch unreachable;
    session.has_document = true;

    let out_buf: [16384]u8 = [0; 16384];
    let out_evts: [4]CdpEvent = [CdpEvent{}; 4];
    let evt_cnt: usize = 0;

    // 1. Accessibility.enable
    const req_enable = CdpRequest{
        id: 1,
        method: "Accessibility.enable",
        params_raw: "{}",
    };
    const resp_enable = CdpDispatcher.dispatch(session, req_enable, out_buf[0..], &out_evts, &evt_cnt);
    expect(resp_enable.isOk());
    expect(evt_cnt == 1);
    expect(std.mem.eql(u8, out_evts[0].method, "Accessibility.loadComplete"));
    expect(std.mem.indexOf(u8, out_evts[0].params_json, "\"RootWebArea\"") != null);

    // 2. Accessibility.getRootAXNode
    const req_root = CdpRequest{
        id: 2,
        method: "Accessibility.getRootAXNode",
        params_raw: "{}",
    };
    const resp_root = CdpDispatcher.dispatch(session, req_root, out_buf[0..], &out_evts, &evt_cnt);
    expect(resp_root.isOk());
    expect(std.mem.indexOf(u8, resp_root.result_json, "\"RootWebArea\"") != null);

    // 3. Accessibility.getFullAXTree
    const req_full = CdpRequest{
        id: 3,
        method: "Accessibility.getFullAXTree",
        params_raw: "{}",
    };
    const resp_full = CdpDispatcher.dispatch(session, req_full, out_buf[0..], &out_evts, &evt_cnt);
    expect(resp_full.isOk());
    expect(std.mem.indexOf(u8, resp_full.result_json, "\"nodes\":[") != null);
    expect(std.mem.indexOf(u8, resp_full.result_json, "\"button\"") != null);
    expect(std.mem.indexOf(u8, resp_full.result_json, "\"Click Me\"") != null);

    // 4. Accessibility.disable
    const req_disable = CdpRequest{
        id: 4,
        method: "Accessibility.disable",
        params_raw: "{}",
    };
    const resp_disable = CdpDispatcher.dispatch(session, req_disable, out_buf[0..], &out_evts, &evt_cnt);
    expect(resp_disable.isOk());
}

fn main() {
    testAXTreeBuilding();
    testAccessibilityProtocol();
    std.debug.print("ALL PHASE 14 ACCESSIBILITY TESTS PASSED SUCCESSFULLY!\n", .{});
}
