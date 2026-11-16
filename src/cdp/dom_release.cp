// Migrated from vendor/kitesurf/packages/engine/src/cdp/dom-release.ts
// CapSurf DOM Resource Safe Deallocation & Noscript Stripper
// Ensures zero memory leaks on iframe/target lifecycle destruction and cleans up detached nodes.

import { Document, NodeId, NULL_NODE, NodeType } from "../dom/dom.cp";
import { Matcher } from "../css/matcher.cp";
import { FrameState } from "./frames.cp";
import { TargetState } from "./types.cp";

/// Strips all <noscript> elements from the document arena to eliminate inactive script fallbacks.
pub fn stripNoscript(doc: *Document) {
    while (true) {
        const nid = Matcher.querySelector(doc, "noscript");
        if (nid == NULL_NODE) {
            break;
        }
        doc.removeChild(nid);
        if (*n := doc.getNodeMut(nid)) {
            n.node_type = NodeType.Comment;
            n.tag_name = "#removed-noscript";
        }
    }
}

/// Safe optional wrapper for stripNoscript.
pub fn stripNoscriptOptional(doc: (*Document)?) {
    if (d := doc) {
        stripNoscript(d);
    }
}

/// Frees an unmanaged or detached Document tree and releases all Arena/ArrayList allocations.
pub fn freeDetachedDom(doc: *Document) {
    doc.deinit();
}

/// Releases DOM resources associated with a specific frame.
pub fn releaseFrameDom(frame_id: []const u8, doc: (*Document)?) {
    * = frame_id;
    if (d := doc) {
        freeDetachedDom(d);
    }
}

/// Releases DOM resources associated with a specific target.
pub fn releaseTargetDom(target_id: []const u8, doc: (*Document)?) {
    * = target_id;
    if (d := doc) {
        freeDetachedDom(d);
    }
}

/// Releases all DOM handles across target and associated frames.
pub fn releaseAllDomHandles(target_id: []const u8, doc: (*Document)?) {
    releaseTargetDom(target_id, doc);
}

/// Releases DOM resources held by a FrameState instance and clears srcdoc references.
pub fn releaseFrameStateDom(frame: *FrameState, doc: (*Document)?) {
    frame.srcdoc = null;
    releaseFrameDom(frame.frame_id, doc);
}

/// Releases DOM resources held by a TargetState instance.
pub fn releaseTargetStateDom(target: *TargetState, doc: (*Document)?) {
    releaseTargetDom(target.id, doc);
}

/// Releases arena-backed DOM storage.
pub fn releaseArenaDom(arena_: *std.heap.ArenaAllocator) {
    arena_.deinit();
}

/// Creates, parses and strips noscript elements for a new target DOM tree.
pub fn createTargetDom(allocator: std.mem.Allocator, html: []const u8): (Document)! {
    let doc = Document.init(allocator)?;
    doc.parseHtml(html) catch {
        doc.deinit();
        return error.HtmlParseFailed;
    };
    stripNoscript(&doc);
    return doc;
}
