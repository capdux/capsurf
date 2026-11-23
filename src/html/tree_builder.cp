// src/html/tree_builder.cp
// CapSurf W3C HTML5 High-Performance Tree Construction Engine
// 100% Native CP implementation modeled after html5ever's TreeSink architecture.
// Operates directly on u32 NodeIds in contiguous Arena DOM storage, eliminating borrow
// conflicts and providing Adoption Agency Algorithm (AAA) and Foster Parenting.

import { NodeId, NULL_NODE, Document, NodeType } from "../dom/dom.cp";
import { HtmlTag } from "./tags.cp";
import { Token, TagToken, TagKind, Attribute, DoctypeToken, NextTokenizerState } from "./tokens.cp";
import { decodeEntity } from "./entities.cp";

pub enum InsertionMode: u8 {
    Initial,
    BeforeHtml,
    BeforeHead,
    InHead,
    InHeadNoscript,
    AfterHead,
    InBody,
    Text,
    InTable,
    InTableText,
    InCaption,
    InColumnGroup,
    InTableBody,
    InRow,
    InCell,
    InTemplate,
    AfterBody,
    InFrameset,
    AfterFrameset,
    AfterAfterBody,
    AfterAfterFrameset,
}

pub struct FormattingEntry {
    is_marker: bool = false,
    node_id: NodeId = NULL_NODE,
    tag: HtmlTag = HtmlTag.Unknown,
    token: TagToken = TagToken{},
}

pub struct TreeBuilder {
    doc: *Document,
    mode: InsertionMode = InsertionMode.Initial,
    orig_mode: InsertionMode = InsertionMode.Initial,
    text_tokenizer_state: NextTokenizerState = NextTokenizerState.Data,

    open_elements: [128]NodeId = [NULL_NODE; 128],
    open_elements_len: usize = 0,

    active_formatting: [128]FormattingEntry = [FormattingEntry{}; 128],
    active_formatting_len: usize = 0,

    template_modes: [16]InsertionMode = [InsertionMode.Initial; 16],
    template_modes_len: usize = 0,

    head_element: NodeId = NULL_NODE,
    form_element: NodeId = NULL_NODE,

    frameset_ok: bool = true,
    foster_parenting: bool = false,

    static fn init(doc: *Document): TreeBuilder {
        return TreeBuilder {
            doc: doc,
            mode: InsertionMode.Initial,
            orig_mode: InsertionMode.Initial,
            text_tokenizer_state: NextTokenizerState.Data,
            open_elements: [NULL_NODE; 128],
            open_elements_len: 0,
            active_formatting: [FormattingEntry{}; 128],
            active_formatting_len: 0,
            template_modes: [InsertionMode.Initial; 16],
            template_modes_len: 0,
            head_element: NULL_NODE,
            form_element: NULL_NODE,
            frameset_ok: true,
            foster_parenting: false,
        };
    }

    // Stack of Open Elements Operations

    fn currentNode(): NodeId {
        if (_.open_elements_len == 0) return NULL_NODE;
        return _.open_elements[_.open_elements_len - 1];
    }

    fn currentTag(): HtmlTag {
        const nid = _.currentNode();
        if (nid == NULL_NODE || @as(usize, nid) >= _.doc.nodes.items.len) {
            return HtmlTag.Unknown;
        }
        return HtmlTag.fromStr(_.doc.nodes.items[@as(usize, nid)].tag_name);
    }

    fn pushElement(*_, nid: NodeId) {
        if (nid == NULL_NODE) return;
        if (_.open_elements_len < 128) {
            _.open_elements[_.open_elements_len] = nid;
            _.open_elements_len += 1;
        }
    }

    fn popElement(*_): NodeId {
        if (_.open_elements_len == 0) return NULL_NODE;
        _.open_elements_len -= 1;
        return _.open_elements[_.open_elements_len];
    }

    fn popUntil(*_, tag: HtmlTag) {
        while (_.open_elements_len > 0) {
            const popped = _.popElement();
            if (popped != NULL_NODE && @as(usize, popped) < _.doc.nodes.items.len) {
                if (HtmlTag.fromStr(_.doc.nodes.items[@as(usize, popped)].tag_name) == tag) {
                    break;
                }
            }
        }
    }

    fn popUntilName(*_, tag: HtmlTag, name: []const u8) {
        while (_.open_elements_len > 0) {
            const popped = _.popElement();
            if (popped != NULL_NODE && @as(usize, popped) < _.doc.nodes.items.len) {
                const node = &_.doc.nodes.items[@as(usize, popped)];
                const t = HtmlTag.fromStr(node.tag_name);
                if (tag != HtmlTag.Unknown) {
                    if (t == tag) break;
                } else {
                    if (std.ascii.eqlIgnoreCase(node.tag_name, name)) break;
                }
            }
        }
    }

    fn isElementInStack(nid: NodeId): bool {
        let i: usize = 0;
        while (i < _.open_elements_len) : (i += 1) {
            if (_.open_elements[i] == nid) return true;
        }
        return false;
    }

    fn isTagInStack(tag: HtmlTag): bool {
        let i: usize = 0;
        while (i < _.open_elements_len) : (i += 1) {
            const nid = _.open_elements[i];
            if (nid != NULL_NODE && @as(usize, nid) < _.doc.nodes.items.len) {
                if (HtmlTag.fromStr(_.doc.nodes.items[@as(usize, nid)].tag_name) == tag) {
                    return true;
                }
            }
        }
        return false;
    }

    fn removeElementFromStack(*_, nid: NodeId) {
        let i: usize = 0;
        while (i < _.open_elements_len) : (i += 1) {
            if (_.open_elements[i] == nid) {
                _.removeElementFromStackAt(i);
                return;
            }
        }
    }

    fn removeElementFromStackAt(*_, idx: usize) {
        if (idx >= _.open_elements_len) return;
        let i = idx;
        while (i + 1 < _.open_elements_len) : (i += 1) {
            _.open_elements[i] = _.open_elements[i + 1];
        }
        _.open_elements_len -= 1;
        _.open_elements[_.open_elements_len] = NULL_NODE;
    }

    fn insertElementIntoStackAfter(*_, ref_elem: NodeId, new_elem: NodeId) {
        if (_.open_elements_len >= 128) return;
        let ref_idx: usize = _.open_elements_len;
        let i: usize = 0;
        while (i < _.open_elements_len) : (i += 1) {
            if (_.open_elements[i] == ref_elem) {
                ref_idx = i;
                break;
            }
        }
        if (ref_idx >= _.open_elements_len) {
            _.pushElement(new_elem);
            return;
        }
        const insert_pos = ref_idx + 1;
        let j = _.open_elements_len;
        while (j > insert_pos) {
            _.open_elements[j] = _.open_elements[j - 1];
            j -= 1;
        }
        _.open_elements[insert_pos] = new_elem;
        _.open_elements_len += 1;
    }

    // Active Formatting Elements Operations

    fn pushActiveFormatting(*_, nid: NodeId, tag_tok: *const TagToken) {
        if (_.active_formatting_len >= 128) return;
        // HTML5 Noah's Ark condition: keep at most 3 identical formatting elements
        let count: usize = 0;
        if (_.active_formatting_len > 0) {
            let i = _.active_formatting_len;
            while (i > 0) {
                i -= 1;
                if (_.active_formatting[i].is_marker) break;
                if (_.active_formatting[i].tag == tag_tok.tag) {
                    count += 1;
                }
            }
        }
        if (count >= 3) {
            // Remove the earliest of the 3 identical entries
            let i = _.active_formatting_len;
            while (i > 0) {
                i -= 1;
                if (_.active_formatting[i].is_marker) break;
                if (_.active_formatting[i].tag == tag_tok.tag) {
                    count -= 1;
                    if (count == 2) {
                        _.removeActiveFormattingAt(i);
                        break;
                    }
                }
            }
        }
        _.active_formatting[_.active_formatting_len] = FormattingEntry {
            is_marker: false,
            node_id: nid,
            tag: tag_tok.tag,
            token: tag_tok.*,
        };
        _.active_formatting_len += 1;
    }

    fn pushActiveFormattingMarker(*_) {
        if (_.active_formatting_len >= 128) return;
        _.active_formatting[_.active_formatting_len] = FormattingEntry {
            is_marker: true,
            node_id: NULL_NODE,
            tag: HtmlTag.Unknown,
            token: TagToken{},
        };
        _.active_formatting_len += 1;
    }

    fn clearActiveFormattingToMarker(*_) {
        while (_.active_formatting_len > 0) {
            _.active_formatting_len -= 1;
            const entry = _.active_formatting[_.active_formatting_len];
            if (entry.is_marker) {
                break;
            }
        }
    }

    fn isInActiveFormatting(nid: NodeId): bool {
        let i: usize = 0;
        while (i < _.active_formatting_len) : (i += 1) {
            if (!_.active_formatting[i].is_marker && _.active_formatting[i].node_id == nid) {
                return true;
            }
        }
        return false;
    }

    fn findInActiveFormatting(nid: NodeId): usize? {
        let i: usize = 0;
        while (i < _.active_formatting_len) : (i += 1) {
            if (!_.active_formatting[i].is_marker && _.active_formatting[i].node_id == nid) {
                return i;
            }
        }
        return null;
    }

    fn removeActiveFormattingAt(*_, idx: usize) {
        if (idx >= _.active_formatting_len) return;
        let i = idx;
        while (i + 1 < _.active_formatting_len) : (i += 1) {
            _.active_formatting[i] = _.active_formatting[i + 1];
        }
        _.active_formatting_len -= 1;
        _.active_formatting[_.active_formatting_len] = FormattingEntry{};
    }

    fn removeElementFromActiveFormatting(*_, nid: NodeId) {
        let i: usize = 0;
        while (i < _.active_formatting_len) : (i += 1) {
            if (!_.active_formatting[i].is_marker && _.active_formatting[i].node_id == nid) {
                _.removeActiveFormattingAt(i);
                return;
            }
        }
    }

    fn reconstructActiveFormattingElements(*_) {
        if (_.active_formatting_len == 0) return;
        const last_idx = _.active_formatting_len - 1;
        if (_.active_formatting[last_idx].is_marker || _.isElementInStack(_.active_formatting[last_idx].node_id)) {
            return;
        }

        let entry_idx = last_idx;
        while (entry_idx > 0) {
            entry_idx -= 1;
            if (_.active_formatting[entry_idx].is_marker || _.isElementInStack(_.active_formatting[entry_idx].node_id)) {
                entry_idx += 1;
                break;
            }
        }

        while (entry_idx < _.active_formatting_len) : (entry_idx += 1) {
            let entry = &_.active_formatting[entry_idx];
            if (entry.is_marker) continue;
            const new_id = _.insertElement(&entry.token);
            entry.node_id = new_id;
        }
    }

    // Scope Checks

    fn inDefaultScope(tag: HtmlTag): bool {
        if (_.open_elements_len == 0) return false;
        let i = _.open_elements_len;
        while (i > 0) {
            i -= 1;
            const nid = _.open_elements[i];
            if (nid == NULL_NODE || @as(usize, nid) >= _.doc.nodes.items.len) continue;
            const t = HtmlTag.fromStr(_.doc.nodes.items[@as(usize, nid)].tag_name);
            if (t == tag) return true;
            if (t.isDefaultScope()) return false;
        }
        return false;
    }

    fn inDefaultScopeName(tag: HtmlTag, name: []const u8): bool {
        if (_.open_elements_len == 0) return false;
        let i = _.open_elements_len;
        while (i > 0) {
            i -= 1;
            const nid = _.open_elements[i];
            if (nid == NULL_NODE || @as(usize, nid) >= _.doc.nodes.items.len) continue;
            const node = &_.doc.nodes.items[@as(usize, nid)];
            const t = HtmlTag.fromStr(node.tag_name);
            if (tag != HtmlTag.Unknown) {
                if (t == tag) return true;
            } else {
                if (std.ascii.eqlIgnoreCase(node.tag_name, name)) return true;
            }
            if (t.isDefaultScope()) return false;
        }
        return false;
    }

    fn inListItemScope(tag: HtmlTag): bool {
        if (_.open_elements_len == 0) return false;
        let i = _.open_elements_len;
        while (i > 0) {
            i -= 1;
            const nid = _.open_elements[i];
            if (nid == NULL_NODE || @as(usize, nid) >= _.doc.nodes.items.len) continue;
            const t = HtmlTag.fromStr(_.doc.nodes.items[@as(usize, nid)].tag_name);
            if (t == tag) return true;
            if (t.isListItemScope()) return false;
        }
        return false;
    }

    fn inButtonScope(tag: HtmlTag): bool {
        if (_.open_elements_len == 0) return false;
        let i = _.open_elements_len;
        while (i > 0) {
            i -= 1;
            const nid = _.open_elements[i];
            if (nid == NULL_NODE || @as(usize, nid) >= _.doc.nodes.items.len) continue;
            const t = HtmlTag.fromStr(_.doc.nodes.items[@as(usize, nid)].tag_name);
            if (t == tag) return true;
            if (t.isButtonScope()) return false;
        }
        return false;
    }

    fn inTableScope(tag: HtmlTag): bool {
        if (_.open_elements_len == 0) return false;
        let i = _.open_elements_len;
        while (i > 0) {
            i -= 1;
            const nid = _.open_elements[i];
            if (nid == NULL_NODE || @as(usize, nid) >= _.doc.nodes.items.len) continue;
            const t = HtmlTag.fromStr(_.doc.nodes.items[@as(usize, nid)].tag_name);
            if (t == tag) return true;
            if (t.isTableScope()) return false;
        }
        return false;
    }

    fn isElementInDefaultScope(nid: NodeId): bool {
        if (_.open_elements_len == 0) return false;
        let i = _.open_elements_len;
        while (i > 0) {
            i -= 1;
            const sid = _.open_elements[i];
            if (sid == nid) return true;
            if (sid != NULL_NODE && @as(usize, sid) < _.doc.nodes.items.len) {
                const t = HtmlTag.fromStr(_.doc.nodes.items[@as(usize, sid)].tag_name);
                if (t.isDefaultScope()) return false;
            }
        }
        return false;
    }

    fn generateImpliedEndTags(*_, except_tag: HtmlTag) {
        while (_.open_elements_len > 0) {
            const t = _.currentTag();
            if (t == except_tag || !t.isCursoryImpliedEnd()) {
                break;
            }
            _.popElement();
        }
    }

    fn generateThoroughImpliedEndTags(*_, except_tag: HtmlTag) {
        while (_.open_elements_len > 0) {
            const t = _.currentTag();
            if (t == except_tag || !t.isThoroughImpliedEnd()) {
                break;
            }
            _.popElement();
        }
    }

    // Node Insertion & Foster Parenting

    fn currentAppropriatePlace(): NodeId {
        if (_.foster_parenting) {
            let i = _.open_elements_len;
            while (i > 0) {
                i -= 1;
                const sid = _.open_elements[i];
                if (sid != NULL_NODE && @as(usize, sid) < _.doc.nodes.items.len) {
                    if (HtmlTag.fromStr(_.doc.nodes.items[@as(usize, sid)].tag_name) == HtmlTag.Table) {
                        const table_parent = _.doc.nodes.items[@as(usize, sid)].parent;
                        if (table_parent != NULL_NODE) {
                            return table_parent;
                        }
                        if (i > 0) {
                            return _.open_elements[i - 1];
                        }
                    }
                }
            }
        }
        if (_.open_elements_len > 0) {
            return _.currentNode();
        }
        if (_.doc.root_element != NULL_NODE) {
            return _.doc.root_element;
        }
        return NULL_NODE;
    }

    fn fosterParentInsert(*_, child_id: NodeId) {
        let i = _.open_elements_len;
        while (i > 0) {
            i -= 1;
            const sid = _.open_elements[i];
            if (sid != NULL_NODE && @as(usize, sid) < _.doc.nodes.items.len) {
                if (HtmlTag.fromStr(_.doc.nodes.items[@as(usize, sid)].tag_name) == HtmlTag.Table) {
                    const table_parent = _.doc.nodes.items[@as(usize, sid)].parent;
                    if (table_parent != NULL_NODE) {
                        _.doc.insertBefore(table_parent, child_id, sid);
                        return;
                    }
                    if (i > 0) {
                        _.doc.appendChild(_.open_elements[i - 1], child_id);
                        return;
                    }
                }
            }
        }
        if (_.open_elements_len > 0) {
            _.doc.appendChild(_.currentNode(), child_id);
        } else if (_.doc.root_element != NULL_NODE) {
            _.doc.appendChild(_.doc.root_element, child_id);
        }
    }

    static fn decodeAttrValue(allocator: std.mem.Allocator, raw: []const u8): []const u8 {
        if (std.mem.indexOfScalar(u8, raw, '&') == null) return raw;
        let buf = std.ArrayListUnmanaged(u8).empty;
        let pos: usize = 0;
        while (pos < raw.len) {
            if (raw[pos] == '&') {
                if (ent := decodeEntity(raw[pos..], true)) {
                    let eb: [8]u8 = [0; 8];
                    const written = ent.toUtf8(eb[0..8]);
                    buf.appendSlice(allocator, eb[0..written]) catch return raw;
                    pos += ent.bytes_consumed;
                    continue;
                }
            }
            buf.append(allocator, raw[pos]) catch return raw;
            pos += 1;
        }
        return buf.toOwnedSlice(allocator) catch raw;
    }

    fn insertElement(*_, tag_tok: *const TagToken): NodeId {
        const nid = _.doc.createElement(tag_tok.name) catch NULL_NODE;
        if (nid == NULL_NODE) return NULL_NODE;

        let i: usize = 0;
        while (i < tag_tok.attr_count) : (i += 1) {
            const decoded_val = TreeBuilder.decodeAttrValue(_.doc.allocator, tag_tok.attrs[i].value);
            _.doc.setAttribute(nid, tag_tok.attrs[i].name, decoded_val);
        }

        if (_.foster_parenting) {
            _.fosterParentInsert(nid);
        } else {
            const parent_id = _.currentAppropriatePlace();
            if (parent_id != NULL_NODE) {
                _.doc.appendChild(parent_id, nid);
            }
        }

        _.pushElement(nid);
        return nid;
    }

    fn insertPhantomElement(*_, tag: HtmlTag): NodeId {
        const name = tag.nameStr();
        const nid = _.doc.createElement(name) catch NULL_NODE;
        if (nid == NULL_NODE) return NULL_NODE;

        const parent_id = _.currentAppropriatePlace();
        if (parent_id != NULL_NODE) {
            _.doc.appendChild(parent_id, nid);
        }
        _.pushElement(nid);
        return nid;
    }

    fn insertCharacter(*_, text: []const u8) {
        if (text.len == 0) return;
        if (!_.foster_parenting) {
            const parent_id = _.currentAppropriatePlace();
            if (parent_id != NULL_NODE) {
                const last_child = _.doc.nodes.items[@as(usize, parent_id)].last_child;
                if (last_child != NULL_NODE && _.doc.nodes.items[@as(usize, last_child)].node_type == NodeType.Text) {
                    const old_text = _.doc.nodes.items[@as(usize, last_child)].text_content;
                    const new_text = std.fmt.allocPrint(_.doc.allocator, "{s}{s}", .{old_text, text}) catch return;
                    _.doc.nodes.items[@as(usize, last_child)].text_content = new_text;
                    return;
                }
            }
        }

        const text_dup = _.doc.allocator.dupe(u8, text) catch text;
        const tid = _.doc.createTextNode(text_dup) catch NULL_NODE;
        if (tid == NULL_NODE) return;

        if (_.foster_parenting) {
            _.fosterParentInsert(tid);
        } else {
            const parent_id = _.currentAppropriatePlace();
            if (parent_id != NULL_NODE) {
                _.doc.appendChild(parent_id, tid);
            }
        }
    }

    fn insertComment(*_, comment: []const u8) {
        const cid = _.doc.createCommentNode(comment) catch NULL_NODE;
        if (cid == NULL_NODE) return;

        if (_.open_elements_len > 0) {
            _.doc.appendChild(_.currentNode(), cid);
        } else if (_.doc.root_element != NULL_NODE) {
            _.doc.appendChild(_.doc.root_element, cid);
        }
    }

    // Adoption Agency Algorithm (W3C HTML5 § 13.2.6.4.7)

    fn adoptionAgencyAlgorithm(*_, subject: HtmlTag) {
        // 1. If current node is subject and not in active_formatting, pop and return
        if (_.open_elements_len > 0 && _.currentTag() == subject) {
            if (!_.isInActiveFormatting(_.currentNode())) {
                _.popElement();
                return;
            }
        }

        // 2. Loop up to 8 times
        let outer_loop: usize = 0;
        while (outer_loop < 8) : (outer_loop += 1) {
            // 5. Find formatting element in active_formatting matching subject
            let fmt_idx: usize = 0;
            let found_fmt = false;
            if (_.active_formatting_len > 0) {
                let i = _.active_formatting_len;
                while (i > 0) {
                    i -= 1;
                    if (_.active_formatting[i].is_marker) break;
                    if (_.active_formatting[i].tag == subject) {
                        fmt_idx = i;
                        found_fmt = true;
                        break;
                    }
                }
            }
            if (!found_fmt) {
                _.popUntil(subject);
                return;
            }

            const fmt_entry = _.active_formatting[fmt_idx];
            const fmt_elem = fmt_entry.node_id;

            // 6. Find fmt_elem in open_elements
            let stack_idx: usize = 0;
            let found_in_stack = false;
            if (_.open_elements_len > 0) {
                let si = _.open_elements_len;
                while (si > 0) {
                    si -= 1;
                    if (_.open_elements[si] == fmt_elem) {
                        stack_idx = si;
                        found_in_stack = true;
                        break;
                    }
                }
            }
            if (!found_in_stack) {
                _.removeActiveFormattingAt(fmt_idx);
                return;
            }

            // 7. Check if fmt_elem is in default scope
            if (!_.isElementInDefaultScope(fmt_elem)) {
                return;
            }

            // 9. Find furthest_block: first special tag in open_elements above fmt_elem
            let furthest_block: NodeId = NULL_NODE;
            let furthest_idx: usize = 0;
            let fi = stack_idx + 1;
            while (fi < _.open_elements_len) : (fi += 1) {
                const sid = _.open_elements[fi];
                if (sid != NULL_NODE && @as(usize, sid) < _.doc.nodes.items.len) {
                    const stag = HtmlTag.fromStr(_.doc.nodes.items[@as(usize, sid)].tag_name);
                    if (stag.isSpecial()) {
                        furthest_block = sid;
                        furthest_idx = fi;
                        break;
                    }
                }
            }

            if (furthest_block == NULL_NODE) {
                // 10. No furthest block: truncate open_elements to stack_idx, remove from formatting
                _.open_elements_len = stack_idx;
                _.removeActiveFormattingAt(fmt_idx);
                return;
            }

            // 11. common_ancestor is open_elements[stack_idx - 1]
            const common_ancestor = if (stack_idx > 0) _.open_elements[stack_idx - 1] else NULL_NODE;

            // 12 & 13. Step backward from furthest_block to fmt_elem
            let node_idx = furthest_idx;
            let last_node = furthest_block;
            let inner_counter: usize = 0;

            while (true) {
                inner_counter += 1;
                node_idx -= 1;
                const node = _.open_elements[node_idx];
                if (node == fmt_elem) break;

                if (inner_counter > 3) {
                    _.removeElementFromActiveFormatting(node);
                    _.removeElementFromStackAt(node_idx);
                    continue;
                }

                const af_pos = _.findInActiveFormatting(node);
                if (af_pos == null) {
                    _.removeElementFromStackAt(node_idx);
                    continue;
                }

                // Clone node
                const cloned_node = _.doc.cloneElementNode(node) catch node;
                _.open_elements[node_idx] = cloned_node;
                _.active_formatting[af_pos.?].node_id = cloned_node;

                // Reparent last_node into cloned_node
                _.doc.removeChild(last_node);
                _.doc.appendChild(cloned_node, last_node);
                last_node = cloned_node;
            }

            // 14. Insert last_node into common_ancestor
            _.doc.removeChild(last_node);
            if (common_ancestor != NULL_NODE) {
                _.doc.appendChild(common_ancestor, last_node);
            }

            // 15. Create new element for fmt_elem
            const new_element = _.doc.cloneElementNode(fmt_elem) catch fmt_elem;

            // 16. Move children of furthest_block to new_element
            _.doc.reparentChildren(furthest_block, new_element);

            // 17. Append new_element into furthest_block
            _.doc.appendChild(furthest_block, new_element);

            // 18. Update active_formatting
            _.active_formatting[fmt_idx].node_id = new_element;

            // 19. Remove old fmt_elem from open_elements
            _.removeElementFromStack(fmt_elem);
            _.insertElementIntoStackAfter(furthest_block, new_element);
        }
    }

    // Table Context Helpers

    fn clearStackToTableContext(*_) {
        while (_.open_elements_len > 0) {
            const t = _.currentTag();
            if (t == HtmlTag.Table || t == HtmlTag.Template || t == HtmlTag.Html) {
                break;
            }
            _.popElement();
        }
    }

    fn clearStackToTableBodyContext(*_) {
        while (_.open_elements_len > 0) {
            const t = _.currentTag();
            if (t.isTableBodyContext() || t == HtmlTag.Template || t == HtmlTag.Html) {
                break;
            }
            _.popElement();
        }
    }

    fn clearStackToTableRowContext(*_) {
        while (_.open_elements_len > 0) {
            const t = _.currentTag();
            if (t.isTableRowContext() || t == HtmlTag.Template || t == HtmlTag.Html) {
                break;
            }
            _.popElement();
        }
    }

    fn closeCell(*_) {
        _.generateImpliedEndTags(HtmlTag.Unknown);
        const t = _.currentTag();
        if (t == HtmlTag.Td || t == HtmlTag.Th) {
            _.popElement();
        }
        _.clearActiveFormattingToMarker();
        _.mode = InsertionMode.InRow;
    }

    fn resetInsertionMode(*_) {
        let i = _.open_elements_len;
        while (i > 0) {
            i -= 1;
            const nid = _.open_elements[i];
            if (nid == NULL_NODE || @as(usize, nid) >= _.doc.nodes.items.len) continue;
            const t = HtmlTag.fromStr(_.doc.nodes.items[@as(usize, nid)].tag_name);

            if (t == HtmlTag.Td || t == HtmlTag.Th) {
                _.mode = InsertionMode.InCell;
                return;
            } else if (t == HtmlTag.Tr) {
                _.mode = InsertionMode.InRow;
                return;
            } else if (t.isTableBodyContext()) {
                _.mode = InsertionMode.InTableBody;
                return;
            } else if (t == HtmlTag.Caption) {
                _.mode = InsertionMode.InCaption;
                return;
            } else if (t == HtmlTag.Colgroup) {
                _.mode = InsertionMode.InColumnGroup;
                return;
            } else if (t == HtmlTag.Table) {
                _.mode = InsertionMode.InTable;
                return;
            } else if (t == HtmlTag.Head) {
                _.mode = InsertionMode.InHead;
                return;
            } else if (t == HtmlTag.Body) {
                _.mode = InsertionMode.InBody;
                return;
            } else if (t == HtmlTag.Frameset) {
                _.mode = InsertionMode.InFrameset;
                return;
            }
        }
        _.mode = InsertionMode.InBody;
    }

    static fn isAllWhitespace(s: []const u8): bool {
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != 0x0c) {
                return false;
            }
        }
        return true;
    }

    // Token Dispatcher

    fn processToken(*_, tok: Token): NextTokenizerState {
        return switch (_.mode) {
            .Initial => _.processInitial(tok),
            .BeforeHtml => _.processBeforeHtml(tok),
            .BeforeHead => _.processBeforeHead(tok),
            .InHead => _.processInHead(tok),
            .AfterHead => _.processAfterHead(tok),
            .InBody => _.processInBody(tok),
            .Text => _.processText(tok),
            .InTable => _.processInTable(tok),
            .InTableBody => _.processInTableBody(tok),
            .InRow => _.processInRow(tok),
            .InCell => _.processInCell(tok),
            .AfterBody => _.processAfterBody(tok),
            .AfterAfterBody => _.processAfterAfterBody(tok),
            * => _.processInBody(tok),
        };
    }

    fn processInitial(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Characters => |text| {
                if (TreeBuilder.isAllWhitespace(text)) {
                    return NextTokenizerState.Data;
                }
            },
            .Comment => |comment| {
                _.insertComment(comment);
                return NextTokenizerState.Data;
            },
            .Doctype => |dt| {
                _.doc.in_quirks_mode = dt.force_quirks || !std.ascii.eqlIgnoreCase(dt.name, "html");
                _.mode = InsertionMode.BeforeHtml;
                return NextTokenizerState.Data;
            },
            * => {},
        }
        _.doc.in_quirks_mode = true;
        _.mode = InsertionMode.BeforeHtml;
        return _.processBeforeHtml(tok);
    }

    fn processBeforeHtml(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Characters => |text| {
                if (TreeBuilder.isAllWhitespace(text)) {
                    return NextTokenizerState.Data;
                }
            },
            .Comment => |comment| {
                _.insertComment(comment);
                return NextTokenizerState.Data;
            },
            .Tag => |*tag_tok| {
                if (tag_tok.kind == TagKind.Start && tag_tok.tag == HtmlTag.Html) {
                    const html_id = _.insertElement(tag_tok);
                    _.doc.root_element = html_id;
                    _.mode = InsertionMode.BeforeHead;
                    return NextTokenizerState.Data;
                }
            },
            * => {},
        }
        const html_id = _.insertPhantomElement(HtmlTag.Html);
        _.doc.root_element = html_id;
        _.mode = InsertionMode.BeforeHead;
        return _.processBeforeHead(tok);
    }

    fn processBeforeHead(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Characters => |text| {
                if (TreeBuilder.isAllWhitespace(text)) {
                    return NextTokenizerState.Data;
                }
            },
            .Comment => |comment| {
                _.insertComment(comment);
                return NextTokenizerState.Data;
            },
            .Tag => |*tag_tok| {
                if (tag_tok.kind == TagKind.Start && tag_tok.tag == HtmlTag.Head) {
                    _.head_element = _.insertElement(tag_tok);
                    _.mode = InsertionMode.InHead;
                    return NextTokenizerState.Data;
                }
            },
            * => {},
        }
        _.head_element = _.insertPhantomElement(HtmlTag.Head);
        _.mode = InsertionMode.InHead;
        return _.processInHead(tok);
    }

    fn processInHead(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Characters => |text| {
                if (TreeBuilder.isAllWhitespace(text)) {
                    return NextTokenizerState.Data;
                }
            },
            .Comment => |comment| {
                _.insertComment(comment);
                return NextTokenizerState.Data;
            },
            .Tag => |*tag_tok| {
                if (tag_tok.kind == TagKind.Start) {
                    if (tag_tok.tag == HtmlTag.Title) {
                        _.insertElement(tag_tok);
                        _.orig_mode = InsertionMode.InHead;
                        _.mode = InsertionMode.Text;
                        _.text_tokenizer_state = NextTokenizerState.RcData;
                        return NextTokenizerState.RcData;
                    } else if (tag_tok.tag == HtmlTag.Style) {
                        _.insertElement(tag_tok);
                        _.orig_mode = InsertionMode.InHead;
                        _.mode = InsertionMode.Text;
                        _.text_tokenizer_state = NextTokenizerState.RawText;
                        return NextTokenizerState.RawText;
                    } else if (tag_tok.tag == HtmlTag.Script) {
                        _.insertElement(tag_tok);
                        _.orig_mode = InsertionMode.InHead;
                        _.mode = InsertionMode.Text;
                        _.text_tokenizer_state = NextTokenizerState.ScriptData;
                        return NextTokenizerState.ScriptData;
                    } else if (tag_tok.tag == HtmlTag.Base ||
                               tag_tok.tag == HtmlTag.Link ||
                               tag_tok.tag == HtmlTag.Meta) {
                        const nid = _.insertElement(tag_tok);
                        _.popElement();
                        * = nid;
                        return NextTokenizerState.Data;
                    }
                } else {
                    if (tag_tok.tag == HtmlTag.Head) {
                        _.popElement();
                        _.mode = InsertionMode.AfterHead;
                        return NextTokenizerState.Data;
                    }
                }
            },
            * => {},
        }
        _.popElement();
        _.mode = InsertionMode.AfterHead;
        return _.processAfterHead(tok);
    }

    fn processAfterHead(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Characters => |text| {
                if (TreeBuilder.isAllWhitespace(text)) {
                    return NextTokenizerState.Data;
                }
            },
            .Comment => |comment| {
                _.insertComment(comment);
                return NextTokenizerState.Data;
            },
            .Tag => |*tag_tok| {
                if (tag_tok.kind == TagKind.Start) {
                    if (tag_tok.tag == HtmlTag.Body) {
                        _.insertElement(tag_tok);
                        _.mode = InsertionMode.InBody;
                        return NextTokenizerState.Data;
                    } else if (tag_tok.tag == HtmlTag.Frameset) {
                        _.insertElement(tag_tok);
                        _.mode = InsertionMode.InFrameset;
                        return NextTokenizerState.Data;
                    } else if (tag_tok.tag == HtmlTag.Title) {
                        _.insertElement(tag_tok);
                        _.orig_mode = InsertionMode.AfterHead;
                        _.mode = InsertionMode.Text;
                        _.text_tokenizer_state = NextTokenizerState.RcData;
                        return NextTokenizerState.RcData;
                    } else if (tag_tok.tag == HtmlTag.Style) {
                        _.insertElement(tag_tok);
                        _.orig_mode = InsertionMode.AfterHead;
                        _.mode = InsertionMode.Text;
                        _.text_tokenizer_state = NextTokenizerState.RawText;
                        return NextTokenizerState.RawText;
                    } else if (tag_tok.tag == HtmlTag.Script) {
                        _.insertElement(tag_tok);
                        _.orig_mode = InsertionMode.AfterHead;
                        _.mode = InsertionMode.Text;
                        _.text_tokenizer_state = NextTokenizerState.ScriptData;
                        return NextTokenizerState.ScriptData;
                    } else if (tag_tok.tag == HtmlTag.Base ||
                               tag_tok.tag == HtmlTag.Link ||
                               tag_tok.tag == HtmlTag.Meta) {
                        const nid = _.insertElement(tag_tok);
                        _.popElement();
                        * = nid;
                        return NextTokenizerState.Data;
                    }
                }
            },
            * => {},
        }
        _.insertPhantomElement(HtmlTag.Body);
        _.mode = InsertionMode.InBody;
        return _.processInBody(tok);
    }

    fn processInBody(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Characters => |text| {
                _.reconstructActiveFormattingElements();
                _.insertCharacter(text);
                return NextTokenizerState.Data;
            },
            .Comment => |comment| {
                _.insertComment(comment);
                return NextTokenizerState.Data;
            },
            .Tag => |*tag_tok| {
                if (tag_tok.kind == TagKind.Start) {
                    const tag = tag_tok.tag;

                    if (tag == HtmlTag.P) {
                        if (_.inButtonScope(HtmlTag.P)) {
                            _.generateImpliedEndTags(HtmlTag.P);
                            _.popUntil(HtmlTag.P);
                        }
                        _.insertElement(tag_tok);
                        return NextTokenizerState.Data;
                    } else if (tag.isHeading()) {
                        if (_.inButtonScope(HtmlTag.P)) {
                            _.generateImpliedEndTags(HtmlTag.P);
                            _.popUntil(HtmlTag.P);
                        }
                        if (_.currentTag().isHeading()) {
                            _.popElement();
                        }
                        _.insertElement(tag_tok);
                        return NextTokenizerState.Data;
                    } else if (tag.isFormatting()) {
                        if (tag == HtmlTag.A) {
                            if (_.isTagInStack(HtmlTag.A)) {
                                _.adoptionAgencyAlgorithm(HtmlTag.A);
                                if (_.isTagInStack(HtmlTag.A)) {
                                    _.popUntil(HtmlTag.A);
                                }
                            }
                        }
                        _.reconstructActiveFormattingElements();
                        const nid = _.insertElement(tag_tok);
                        _.pushActiveFormatting(nid, tag_tok);
                        return NextTokenizerState.Data;
                    } else if (tag.isVoid()) {
                        _.reconstructActiveFormattingElements();
                        if (tag == HtmlTag.Hr && _.inButtonScope(HtmlTag.P)) {
                            _.generateImpliedEndTags(HtmlTag.P);
                            _.popUntil(HtmlTag.P);
                        }
                        const nid = _.insertElement(tag_tok);
                        _.popElement();
                        * = nid;
                        return NextTokenizerState.Data;
                    } else if (tag == HtmlTag.Ul || tag == HtmlTag.Ol || tag == HtmlTag.Menu) {
                        if (_.inButtonScope(HtmlTag.P)) {
                            _.generateImpliedEndTags(HtmlTag.P);
                            _.popUntil(HtmlTag.P);
                        }
                        _.insertElement(tag_tok);
                        return NextTokenizerState.Data;
                    } else if (tag == HtmlTag.Li) {
                        if (_.inButtonScope(HtmlTag.P)) {
                            _.generateImpliedEndTags(HtmlTag.P);
                            _.popUntil(HtmlTag.P);
                        }
                        if (_.inListItemScope(HtmlTag.Li)) {
                            _.generateImpliedEndTags(HtmlTag.Li);
                            _.popUntil(HtmlTag.Li);
                        }
                        _.insertElement(tag_tok);
                        return NextTokenizerState.Data;
                    } else if (tag == HtmlTag.Dd || tag == HtmlTag.Dt) {
                        if (_.inButtonScope(HtmlTag.P)) {
                            _.generateImpliedEndTags(HtmlTag.P);
                            _.popUntil(HtmlTag.P);
                        }
                        if (_.currentTag() == HtmlTag.Dd || _.currentTag() == HtmlTag.Dt) {
                            _.popElement();
                        }
                        _.insertElement(tag_tok);
                        return NextTokenizerState.Data;
                    } else if (tag == HtmlTag.Option) {
                        if (_.currentTag() == HtmlTag.Option) {
                            _.popElement();
                        }
                        _.insertElement(tag_tok);
                        return NextTokenizerState.Data;
                    } else if (tag == HtmlTag.Optgroup) {
                        if (_.currentTag() == HtmlTag.Option) {
                            _.popElement();
                        }
                        if (_.currentTag() == HtmlTag.Optgroup) {
                            _.popElement();
                        }
                        _.insertElement(tag_tok);
                        return NextTokenizerState.Data;
                    } else if (tag == HtmlTag.Table) {
                        if (_.inButtonScope(HtmlTag.P)) {
                            _.generateImpliedEndTags(HtmlTag.P);
                            _.popUntil(HtmlTag.P);
                        }
                        _.insertElement(tag_tok);
                        _.mode = InsertionMode.InTable;
                        return NextTokenizerState.Data;
                    } else if (tag == HtmlTag.Style || tag == HtmlTag.Xmp || tag == HtmlTag.Iframe) {
                        _.insertElement(tag_tok);
                        _.orig_mode = InsertionMode.InBody;
                        _.mode = InsertionMode.Text;
                        _.text_tokenizer_state = NextTokenizerState.RawText;
                        return NextTokenizerState.RawText;
                    } else if (tag == HtmlTag.Script) {
                        _.insertElement(tag_tok);
                        _.orig_mode = InsertionMode.InBody;
                        _.mode = InsertionMode.Text;
                        _.text_tokenizer_state = NextTokenizerState.ScriptData;
                        return NextTokenizerState.ScriptData;
                    } else if (tag == HtmlTag.Plaintext) {
                        _.insertElement(tag_tok);
                        _.mode = InsertionMode.InBody;
                        return NextTokenizerState.PlainText;
                    } else {
                        // General block/inline element
                        if (tag.closesP() && _.inButtonScope(HtmlTag.P)) {
                            _.generateImpliedEndTags(HtmlTag.P);
                            _.popUntil(HtmlTag.P);
                        }
                        _.reconstructActiveFormattingElements();
                        const nid = _.insertElement(tag_tok);
                        if (tag_tok.self_closing) {
                            * = _.popElement();
                        }
                        * = nid;
                        return NextTokenizerState.Data;
                    }
                } else {
                    // End Tag
                    const tag = tag_tok.tag;

                    if (tag == HtmlTag.Body) {
                        if (_.inDefaultScope(HtmlTag.Body)) {
                            _.mode = InsertionMode.AfterBody;
                        }
                        return NextTokenizerState.Data;
                    } else if (tag == HtmlTag.Html) {
                        if (_.inDefaultScope(HtmlTag.Body)) {
                            _.mode = InsertionMode.AfterBody;
                            return _.processAfterBody(tok);
                        }
                        return NextTokenizerState.Data;
                    } else if (tag.isFormatting()) {
                        _.adoptionAgencyAlgorithm(tag);
                        return NextTokenizerState.Data;
                    } else if (tag == HtmlTag.P) {
                        if (!_.inButtonScope(HtmlTag.P)) {
                            _.insertPhantomElement(HtmlTag.P);
                        }
                        _.generateImpliedEndTags(HtmlTag.P);
                        _.popUntil(HtmlTag.P);
                        return NextTokenizerState.Data;
                    } else if (tag.isHeading()) {
                        let i = _.open_elements_len;
                        let has_heading = false;
                        while (i > 0) {
                            i -= 1;
                            const sid = _.open_elements[i];
                            if (sid != NULL_NODE && @as(usize, sid) < _.doc.nodes.items.len) {
                                if (HtmlTag.fromStr(_.doc.nodes.items[@as(usize, sid)].tag_name).isHeading()) {
                                    has_heading = true;
                                    break;
                                }
                            }
                        }
                        if (has_heading) {
                            _.generateImpliedEndTags(HtmlTag.Unknown);
                            while (_.open_elements_len > 0) {
                                const popped = _.popElement();
                                if (popped != NULL_NODE && @as(usize, popped) < _.doc.nodes.items.len) {
                                    if (HtmlTag.fromStr(_.doc.nodes.items[@as(usize, popped)].tag_name).isHeading()) {
                                        break;
                                    }
                                }
                            }
                        }
                        return NextTokenizerState.Data;
                    } else {
                        // General End Tag
                        if (_.inDefaultScopeName(tag, tag_tok.name)) {
                            _.generateImpliedEndTags(tag);
                            _.popUntilName(tag, tag_tok.name);
                        }
                        return NextTokenizerState.Data;
                    }
                }
            },
            .Eof => {
                return NextTokenizerState.Data;
            },
            * => {},
        }
        return NextTokenizerState.Data;
    }

    fn processText(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Characters => |text| {
                _.insertCharacter(text);
                return _.text_tokenizer_state;
            },
            .Tag => |*tag_tok| {
                if (tag_tok.kind == TagKind.End) {
                    _.popElement();
                    _.mode = _.orig_mode;
                    _.text_tokenizer_state = NextTokenizerState.Data;
                    return NextTokenizerState.Data;
                }
            },
            .Eof => {
                _.popElement();
                _.mode = _.orig_mode;
                _.text_tokenizer_state = NextTokenizerState.Data;
                return NextTokenizerState.Data;
            },
            * => {},
        }
        return _.text_tokenizer_state;
    }

    fn processInTable(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Characters => |text| {
                _.foster_parenting = true;
                _.insertCharacter(text);
                _.foster_parenting = false;
                return NextTokenizerState.Data;
            },
            .Comment => |comment| {
                _.insertComment(comment);
                return NextTokenizerState.Data;
            },
            .Tag => |*tag_tok| {
                if (tag_tok.kind == TagKind.Start) {
                    const tag = tag_tok.tag;
                    if (tag == HtmlTag.Caption) {
                        _.clearStackToTableContext();
                        _.pushActiveFormattingMarker();
                        _.insertElement(tag_tok);
                        _.mode = InsertionMode.InCaption;
                        return NextTokenizerState.Data;
                    } else if (tag == HtmlTag.Colgroup) {
                        _.clearStackToTableContext();
                        _.insertElement(tag_tok);
                        _.mode = InsertionMode.InColumnGroup;
                        return NextTokenizerState.Data;
                    } else if (tag == HtmlTag.Col) {
                        _.clearStackToTableContext();
                        _.insertPhantomElement(HtmlTag.Colgroup);
                        _.mode = InsertionMode.InColumnGroup;
                        return _.processToken(tok);
                    } else if (tag.isTableBodyContext()) {
                        _.clearStackToTableContext();
                        _.insertElement(tag_tok);
                        _.mode = InsertionMode.InTableBody;
                        return NextTokenizerState.Data;
                    } else if (tag == HtmlTag.Tr) {
                        _.clearStackToTableContext();
                        _.insertPhantomElement(HtmlTag.Tbody);
                        _.mode = InsertionMode.InTableBody;
                        return _.processInTableBody(tok);
                    } else if (tag == HtmlTag.Td || tag == HtmlTag.Th) {
                        _.clearStackToTableContext();
                        _.insertPhantomElement(HtmlTag.Tbody);
                        _.mode = InsertionMode.InTableBody;
                        return _.processInTableBody(tok);
                    }
                } else {
                    if (tag_tok.tag == HtmlTag.Table) {
                        if (_.inTableScope(HtmlTag.Table)) {
                            _.popUntil(HtmlTag.Table);
                            _.resetInsertionMode();
                        }
                        return NextTokenizerState.Data;
                    }
                }
            },
            * => {},
        }
        // Foster parent anything else
        _.foster_parenting = true;
        const res = _.processInBody(tok);
        _.foster_parenting = false;
        return res;
    }

    fn processInTableBody(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Tag => |*tag_tok| {
                if (tag_tok.kind == TagKind.Start) {
                    if (tag_tok.tag == HtmlTag.Tr) {
                        _.clearStackToTableBodyContext();
                        _.insertElement(tag_tok);
                        _.mode = InsertionMode.InRow;
                        return NextTokenizerState.Data;
                    } else if (tag_tok.tag == HtmlTag.Td || tag_tok.tag == HtmlTag.Th) {
                        _.clearStackToTableBodyContext();
                        _.insertPhantomElement(HtmlTag.Tr);
                        _.mode = InsertionMode.InRow;
                        return _.processInRow(tok);
                    }
                } else {
                    if (tag_tok.tag.isTableBodyContext()) {
                        if (_.inTableScope(tag_tok.tag)) {
                            _.clearStackToTableBodyContext();
                            _.popElement();
                            _.mode = InsertionMode.InTable;
                        }
                        return NextTokenizerState.Data;
                    } else if (tag_tok.tag == HtmlTag.Table) {
                        if (_.inTableScope(HtmlTag.Table)) {
                            _.clearStackToTableBodyContext();
                            _.popElement();
                            _.mode = InsertionMode.InTable;
                            return _.processInTable(tok);
                        }
                        return NextTokenizerState.Data;
                    }
                }
            },
            * => {},
        }
        return _.processInTable(tok);
    }

    fn processInRow(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Tag => |*tag_tok| {
                if (tag_tok.kind == TagKind.Start) {
                    if (tag_tok.tag == HtmlTag.Td || tag_tok.tag == HtmlTag.Th) {
                        _.clearStackToTableRowContext();
                        _.insertElement(tag_tok);
                        _.mode = InsertionMode.InCell;
                        _.pushActiveFormattingMarker();
                        return NextTokenizerState.Data;
                    }
                } else {
                    if (tag_tok.tag == HtmlTag.Tr) {
                        if (_.inTableScope(HtmlTag.Tr)) {
                            _.clearStackToTableRowContext();
                            _.popElement();
                            _.mode = InsertionMode.InTableBody;
                        }
                        return NextTokenizerState.Data;
                    } else if (tag_tok.tag == HtmlTag.Table) {
                        if (_.inTableScope(HtmlTag.Table)) {
                            _.clearStackToTableRowContext();
                            _.popElement();
                            _.mode = InsertionMode.InTableBody;
                            return _.processInTableBody(tok);
                        }
                        return NextTokenizerState.Data;
                    }
                }
            },
            * => {},
        }
        return _.processInTable(tok);
    }

    fn processInCell(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Tag => |*tag_tok| {
                if (tag_tok.kind == TagKind.End) {
                    if (tag_tok.tag == HtmlTag.Td || tag_tok.tag == HtmlTag.Th) {
                        if (_.inTableScope(tag_tok.tag)) {
                            _.closeCell();
                        }
                        return NextTokenizerState.Data;
                    }
                } else {
                    if (tag_tok.tag == HtmlTag.Caption ||
                        tag_tok.tag == HtmlTag.Col ||
                        tag_tok.tag == HtmlTag.Colgroup ||
                        tag_tok.tag.isTableBodyContext() ||
                        tag_tok.tag == HtmlTag.Tr ||
                        tag_tok.tag == HtmlTag.Td ||
                        tag_tok.tag == HtmlTag.Th) {
                        if (_.inTableScope(HtmlTag.Td) || _.inTableScope(HtmlTag.Th)) {
                            _.closeCell();
                            return _.processToken(tok);
                        }
                    }
                }
            },
            * => {},
        }
        return _.processInBody(tok);
    }

    fn processAfterBody(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Characters => |text| {
                if (TreeBuilder.isAllWhitespace(text)) {
                    return NextTokenizerState.Data;
                }
            },
            .Comment => |comment| {
                if (_.doc.root_element != NULL_NODE) {
                    const cid = _.doc.createCommentNode(comment) catch NULL_NODE;
                    if (cid != NULL_NODE) {
                        _.doc.appendChild(_.doc.root_element, cid);
                    }
                }
                return NextTokenizerState.Data;
            },
            .Tag => |*tag_tok| {
                if (tag_tok.kind == TagKind.End && tag_tok.tag == HtmlTag.Html) {
                    _.mode = InsertionMode.AfterAfterBody;
                    return NextTokenizerState.Data;
                }
            },
            .Eof => {
                return NextTokenizerState.Data;
            },
            * => {},
        }
        _.mode = InsertionMode.InBody;
        return _.processInBody(tok);
    }

    fn processAfterAfterBody(*_, tok: Token): NextTokenizerState {
        switch (tok) {
            .Comment => |comment| {
                _.insertComment(comment);
                return NextTokenizerState.Data;
            },
            .Characters => |text| {
                if (TreeBuilder.isAllWhitespace(text)) {
                    return NextTokenizerState.Data;
                }
            },
            .Eof => {
                return NextTokenizerState.Data;
            },
            * => {},
        }
        _.mode = InsertionMode.InBody;
        return _.processInBody(tok);
    }
}
