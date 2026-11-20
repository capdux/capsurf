// test/cdp/test_frames.cp
// Tests for CapSurf CDP Multi-Frame / IFrame Isolation & Management

import {
    mintFrameId,
    mintLoaderId,
    originOf,
    domainAndRegistryOf,
    matchesAboutBlank,
    isLocalhost,
    inheritsCreatorOrigin,
    FrameState,
    FrameContext,
    MainFrameArgs,
    ChildFrameArgs,
    makeMainFrameState,
    makeChildFrameState,
    frameContextOf,
    makeFrameInfo,
    buildFrameTree,
    buildFrameResourceTree,
    committedFrameOriginOf,
    frameAncestorOrigins,
    CommittedOrigin,
} from "../../src/cdp/frames.cp";

fn main() {
    // 1. Mint Frame & Loader ID
    const f1 = mintFrameId();
    const f2 = mintFrameId();
    std.debug.assert(std.mem.startsWith(u8, f1, "capsurf-frame-"));
    std.debug.assert(std.mem.startsWith(u8, f2, "capsurf-frame-"));
    std.debug.assert(!std.mem.eql(u8, f1, f2));

    const l1 = mintLoaderId();
    std.debug.assert(std.mem.startsWith(u8, l1, "capsurf-loader-"));

    // 2. Origin & Domain Calculation
    std.debug.assert(std.mem.eql(u8, originOf("https://example.com/path?query#hash"), "https://example.com"));
    std.debug.assert(std.mem.eql(u8, originOf("http://localhost:8080/foo"), "http://localhost:8080"));
    std.debug.assert(std.mem.eql(u8, originOf("about:blank"), "about://"));
    std.debug.assert(std.mem.eql(u8, originOf("invalid-url"), "://"));

    std.debug.assert(matchesAboutBlank("about:blank"));
    std.debug.assert(matchesAboutBlank("about:blank?foo=1"));
    std.debug.assert(!matchesAboutBlank("https://example.com"));

    std.debug.assert(inheritsCreatorOrigin("about:blank"));
    std.debug.assert(inheritsCreatorOrigin("about:srcdoc"));
    std.debug.assert(!inheritsCreatorOrigin("https://example.com"));

    std.debug.assert(isLocalhost("localhost"));
    std.debug.assert(isLocalhost("sub.localhost"));
    std.debug.assert(isLocalhost("127.0.0.1"));
    std.debug.assert(!isLocalhost("example.com"));

    std.debug.assert(std.mem.eql(u8, domainAndRegistryOf("https://sub.example.com/foo"), "example.com"));
    std.debug.assert(std.mem.eql(u8, domainAndRegistryOf("http://localhost:8080/foo"), ""));

    // 3. FrameState creation & FrameContext calculation
    const main_frame = makeMainFrameState(MainFrameArgs{
        frame_id: "main-01",
        loader_id: "load-01",
        url: "https://example.com/index.html",
    });
    std.debug.assert(std.mem.eql(u8, main_frame.frame_id, "main-01"));
    std.debug.assert(main_frame.parent_frame_id == null);

    const child_frame = makeChildFrameState(ChildFrameArgs{
        frame_id: "child-01",
        parent_frame_id: "main-01",
        loader_id: "load-02",
        url: "https://example.com/iframe.html",
        name: "subframe",
        owner_element_id: "subframe-elem",
    });
    std.debug.assert(std.mem.eql(u8, child_frame.frame_id, "child-01"));
    std.debug.assert(std.mem.eql(u8, child_frame.parent_frame_id.?, "main-01"));

    let frames: [2]FrameState = [ main_frame, child_frame ];

    const ctx_main = frameContextOf(&main_frame, frames[0..2]);
    std.debug.assert(std.mem.eql(u8, ctx_main.secure_context_type, "Secure"));
    std.debug.assert(std.mem.eql(u8, ctx_main.cross_origin_isolated_context_type, "NotIsolated"));

    const ctx_child = frameContextOf(&child_frame, frames[0..2]);
    std.debug.assert(std.mem.eql(u8, ctx_child.secure_context_type, "Secure"));
    std.debug.assert(ctx_child.same_site_as_parent);

    // 4. makeFrameInfo and buildFrameTree CDP JSON
    let main_frame_2 = makeMainFrameState(MainFrameArgs{
        frame_id: "main-01",
        loader_id: "load-01",
        url: "https://example.com/index.html#sec1",
    });
    let frames_2: [2]FrameState = [ main_frame_2, child_frame ];

    let info_buf: [2048]u8 = undefined;
    const info_json = makeFrameInfo(&main_frame_2, info_buf[0..]);
    std.debug.assert(std.mem.indexOf(u8, info_json, "\"id\":\"main-01\"") != null);
    std.debug.assert(std.mem.indexOf(u8, info_json, "\"urlFragment\":\"#sec1\"") != null);
    std.debug.assert(std.mem.indexOf(u8, info_json, "\"domainAndRegistry\":\"example.com\"") != null);

    let tree_buf: [4096]u8 = undefined;
    const tree_json = buildFrameTree(frames_2[0..2], tree_buf[0..]);
    std.debug.assert(std.mem.indexOf(u8, tree_json, "\"frame\":{\"id\":\"main-01\"") != null);
    std.debug.assert(std.mem.indexOf(u8, tree_json, "\"childFrames\":[{\"frame\":{\"id\":\"child-01\"") != null);

    let res_buf: [4096]u8 = undefined;
    const res_tree_json = buildFrameResourceTree(frames_2[0..2], res_buf[0..]);
    std.debug.assert(std.mem.indexOf(u8, res_tree_json, "\"resources\":[]") != null);

    // 5. Committed Frame Origin & Ancestors
    const orig = committedFrameOriginOf(&child_frame, frames_2[0..2], "https://example.com");
    std.debug.assert(std.mem.eql(u8, orig.kind, "tuple"));
    std.debug.assert(std.mem.eql(u8, orig.origin, "https://example.com"));

    let ancestors: [4]CommittedOrigin = [CommittedOrigin{}; 4];
    const ancestor_count = frameAncestorOrigins(&child_frame, frames_2[0..2], "https://example.com", ancestors[0..4]);
    std.debug.assert(ancestor_count == 1);
    std.debug.assert(std.mem.eql(u8, ancestors[0].origin, "https://example.com"));

    std.debug.print("All test_frames assertions passed successfully!\n", .{});
}
