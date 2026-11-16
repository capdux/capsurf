// Migrated from vendor/kitesurf/packages/dom-shim/src/collections.ts
// Implements WHATWG DOMTokenList (classList), NodeList, and HTMLCollection over Arena DOM tree.

import { Document, NodeData, NodeId, NULL_NODE, NodeType } from "./dom.cp";

pub const MAX_TOKENS: usize = 32;
pub const MAX_COLLECTION_SIZE: usize = 128;

pub struct DOMTokenList {
    tokens: [MAX_TOKENS][]const u8 = [""; 32],
    token_count: usize = 0,

    static fn init(): DOMTokenList {
        return DOMTokenList{};
    }

    fn fromString(*_, class_str: []const u8) {
        _.token_count = 0;
        let pos: usize = 0;
        while (pos < class_str.len && _.token_count < MAX_TOKENS) {
            // Skip whitespace
            while (pos < class_str.len && isSpace(class_str[pos])) : (pos += 1) {}
            if (pos >= class_str.len) break;

            let start = pos;
            while (pos < class_str.len && !isSpace(class_str[pos])) : (pos += 1) {}
            let tok = class_str[start..pos];

            if (!_.contains(tok)) {
                _.tokens[_.token_count] = tok;
                _.token_count += 1;
            }
        }
    }

    fn length(): usize {
        return _.token_count;
    }

    fn item(idx: usize): ([]const u8)? {
        if (idx >= _.token_count) return null;
        return _.tokens[idx];
    }

    fn contains(token: []const u8): bool {
        let i: usize = 0;
        while (i < _.token_count) : (i += 1) {
            if (std.mem.eql(u8, _.tokens[i], token)) {
                return true;
            }
        }
        return false;
    }

    fn add(*_, token: []const u8): bool {
        if (token.len == 0 || _.contains(token)) return false;
        if (_.token_count >= MAX_TOKENS) return false;
        _.tokens[_.token_count] = token;
        _.token_count += 1;
        return true;
    }

    fn remove(*_, token: []const u8): bool {
        let i: usize = 0;
        let found = false;
        while (i < _.token_count) : (i += 1) {
            if (std.mem.eql(u8, _.tokens[i], token)) {
                found = true;
                break;
            }
        }
        if (!found) return false;

        let j = i;
        while (j + 1 < _.token_count) : (j += 1) {
            _.tokens[j] = _.tokens[j + 1];
        }
        _.token_count -= 1;
        return true;
    }

    fn toggle(*_, token: []const u8): bool {
        if (_.contains(token)) {
            * = _.remove(token);
            return false;
        } else {
            * = _.add(token);
            return true;
        }
    }

    fn replaceToken(*_, old_token: []const u8, new_token: []const u8): bool {
        let i: usize = 0;
        while (i < _.token_count) : (i += 1) {
            if (std.mem.eql(u8, _.tokens[i], old_token)) {
                _.tokens[i] = new_token;
                return true;
            }
        }
        return false;
    }

    fn serialize(out_buf: []u8): []const u8 {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.token_count) : (i += 1) {
            let tok = _.tokens[i];
            if (i > 0) {
                if (pos < out_buf.len) {
                    out_buf[pos] = ' ';
                    pos += 1;
                }
            }
            if (pos + tok.len > out_buf.len) break;
            @memcpy(out_buf[pos .. pos + tok.len], tok);
            pos += tok.len;
        }
        return out_buf[0..pos];
    }
}

pub struct NodeList {
    items: [MAX_COLLECTION_SIZE]NodeId = [NULL_NODE; 128],
    item_count: usize = 0,

    static fn init(): NodeList {
        return NodeList{};
    }

    fn length(): usize {
        return _.item_count;
    }

    fn item(idx: usize): NodeId {
        if (idx >= _.item_count) return NULL_NODE;
        return _.items[idx];
    }

    fn append(*_, id: NodeId): bool {
        if (_.item_count >= MAX_COLLECTION_SIZE) return false;
        _.items[_.item_count] = id;
        _.item_count += 1;
        return true;
    }
}

pub struct HTMLCollection {
    elements: [MAX_COLLECTION_SIZE]NodeId = [NULL_NODE; 128],
    element_count: usize = 0,

    static fn init(): HTMLCollection {
        return HTMLCollection{};
    }

    fn length(): usize {
        return _.element_count;
    }

    fn item(idx: usize): NodeId {
        if (idx >= _.element_count) return NULL_NODE;
        return _.elements[idx];
    }

    fn append(*_, id: NodeId): bool {
        if (_.element_count >= MAX_COLLECTION_SIZE) return false;
        _.elements[_.element_count] = id;
        _.element_count += 1;
        return true;
    }

    fn namedItem(doc: *const Document, name_or_id: []const u8): NodeId {
        let i: usize = 0;
        while (i < _.element_count) : (i += 1) {
            let nid = _.elements[i];
            let n_opt = doc.getNode(nid);
            if (n_opt == null) continue;
            let node = n_opt.?;
            if (node.hasId(name_or_id)) return nid;
            if (name_attr := node.getAttribute("name")) {
                if (std.mem.eql(u8, name_attr, name_or_id)) {
                    return nid;
                }
            }
        }
        return NULL_NODE;
    }
}

fn isSpace(c: u8): bool {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C);
}
