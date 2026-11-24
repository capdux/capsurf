// src/css/matcher.cp
// CapSurf Servo-Architecture Selector Parser, 30-Bit Specificity Calculator & DOM Matcher
// Implements Servo's right-to-left compound / left-to-right simple Component state machine:
// - Simple selectors: .local_name, *, #id, .class, [attr op val i/s]
// - Structural pseudo-classes: :root, :empty, :first-child, :last-child, :only-child,
//   :nth-child(an+b), :nth-last-child(an+b), :first-of-type, :last-of-type, :only-of-type
// - Logical pseudo-classes (Level 4): :is(...), :where(...), :not(...), :has(...)
// - Combinators: ' ' (descendant), '>' (child), '+' (next-sibling), '~' (later-sibling)

import { Document, NodeData, NodeType, NodeId, NULL_NODE } from "../dom/dom.cp";
import { CssSpecificity } from "cap:css";

pub const MAX_COMPONENTS_PER_SELECTOR: usize = 24;
pub const MAX_SELECTORS_PER_LIST: usize = 8;

fn scanCssIdent(s: []const u8, idx: *usize): []const u8 {
    const start = idx.*;
    while (idx.* < s.len) {
        if (s[idx.*] == '\\' && idx.* + 1 < s.len) {
            if (hexDigitVal(s[idx.* + 1]) != null) {
                idx.* += 2;
                let hcount: usize = 1;
                while (idx.* < s.len && hcount < 6 && hexDigitVal(s[idx.*]) != null) : (hcount += 1) {
                    idx.* += 1;
                }
                if (idx.* < s.len && (s[idx.*] == ' ' || s[idx.*] == '\t' || s[idx.*] == '\n' || s[idx.*] == '\r' || s[idx.*] == 0x0c)) {
                    idx.* += 1;
                }
            } else {
                idx.* += 2;
            }
        } else if (SelectorParser.isIdentChar(s[idx.*])) {
            idx.* += 1;
        } else {
            break;
        }
    }
    return s[start..idx.*];
}

fn hexDigitVal(c: u8): (u32)? {
    if (c >= '0' and c <= '9') return @as(u32, c - '0');
    if (c >= 'a' and c <= 'f') return @as(u32, c - 'a' + 10);
    if (c >= 'A' and c <= 'F') return @as(u32, c - 'A' + 10);
    return null;
}

fn unescapeCssIdent(raw: []const u8, out_buf: []u8): []const u8 {
    if (std.mem.indexOf(u8, raw, "\\") == null) return raw;
    let oi: usize = 0;
    let i: usize = 0;
    while (i < raw.len && oi < out_buf.len) {
        if (raw[i] == '\\' && i + 1 < raw.len) {
            if (d0 := hexDigitVal(raw[i + 1])) {
                let cp: u32 = d0;
                let hi: usize = i + 2;
                let count: usize = 1;
                while (hi < raw.len && count < 6) : (count += 1) {
                    if (dv := hexDigitVal(raw[hi])) {
                        cp = (cp << 4) | dv;
                        hi += 1;
                    } else {
                        break;
                    }
                }
                if (hi < raw.len && (raw[hi] == ' ' || raw[hi] == '\t' || raw[hi] == '\n' || raw[hi] == '\r' || raw[hi] == 0x0c)) {
                    hi += 1;
                }
                if (cp == 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
                    cp = 0xFFFD;
                }
                if (cp < 0x80) {
                    out_buf[oi] = @as(u8, @truncate(cp));
                    oi += 1;
                } else if (cp < 0x800 && oi + 1 < out_buf.len) {
                    out_buf[oi] = @as(u8, @truncate(0xC0 | (cp >> 6)));
                    out_buf[oi + 1] = @as(u8, @truncate(0x80 | (cp & 0x3F)));
                    oi += 2;
                } else if (cp < 0x10000 && oi + 2 < out_buf.len) {
                    out_buf[oi] = @as(u8, @truncate(0xE0 | (cp >> 12)));
                    out_buf[oi + 1] = @as(u8, @truncate(0x80 | ((cp >> 6) & 0x3F)));
                    out_buf[oi + 2] = @as(u8, @truncate(0x80 | (cp & 0x3F)));
                    oi += 3;
                } else if (oi + 3 < out_buf.len) {
                    out_buf[oi] = @as(u8, @truncate(0xF0 | (cp >> 18)));
                    out_buf[oi + 1] = @as(u8, @truncate(0x80 | ((cp >> 12) & 0x3F)));
                    out_buf[oi + 2] = @as(u8, @truncate(0x80 | ((cp >> 6) & 0x3F)));
                    out_buf[oi + 3] = @as(u8, @truncate(0x80 | (cp & 0x3F)));
                    oi += 4;
                }
                i = hi;
            } else {
                out_buf[oi] = raw[i + 1];
                oi += 1;
                i += 2;
            }
        } else {
            out_buf[oi] = raw[i];
            oi += 1;
            i += 1;
        }
    }
    return out_buf[0..oi];
}

pub enum Combinator {
    Descendant,   // ' '
    Child,        // '>'
    NextSibling,  // '+'
    LaterSibling, // '~'
}

pub enum AttrOperator {
    Exists,    // [attr]
    Equal,     // [attr=val]
    Includes,  // [attr~=val]
    DashMatch, // [attr|=val]
    Prefix,    // [attr^=val]
    Suffix,    // [attr$=val]
    Substring, // [attr*=val]
}

pub enum PseudoKind {
    Root,
    Empty,
    FirstChild,
    LastChild,
    OnlyChild,
    FirstOfType,
    LastOfType,
    OnlyOfType,
    NthChild,
    NthLastChild,
    NthOfType,
    NthLastOfType,
    Is,
    Where,
    Not,
    Has,
    Focus,
    Hover,
    Dir,
    Lang,
    Link,
    Visited,
}

pub enum ComponentKind {
    Universal,
    LocalName,
    Id,
    Class,
    Attribute,
    Pseudo,
    CombinatorComp,
}

pub struct Specificity {
    id_selectors: u32 = 0;
    class_like_selectors: u32 = 0;
    element_selectors: u32 = 0;

    static fn fromCounts(ids: u32, classes: u32, elements: u32): Specificity {
        return Specificity {
            id_selectors: ids,
            class_like_selectors: classes,
            element_selectors: elements,
        };
    }

    /// Packs specificity into a 30-bit unsigned integer per Servo (`10-bit ID | 10-bit Class | 10-bit Tag`).
    fn toU32(): u32 {
        const max_10bit: u32 = 1023;
        const id_part = if (_.id_selectors > max_10bit) max_10bit else _.id_selectors;
        const cls_part = if (_.class_like_selectors > max_10bit) max_10bit else _.class_like_selectors;
        const el_part = if (_.element_selectors > max_10bit) max_10bit else _.element_selectors;
        return (id_part << @as(u32, 20)) | (cls_part << @as(u32, 10)) | el_part;
    }

    fn toCssSpecificity(): CssSpecificity {
        return CssSpecificity.init(_.id_selectors, _.class_like_selectors, _.element_selectors);
    }

    fn add(*_, other: Specificity) {
        _.id_selectors += other.id_selectors;
        _.class_like_selectors += other.class_like_selectors;
        _.element_selectors += other.element_selectors;
    }
}

pub struct Component {
    kind: ComponentKind = ComponentKind.Universal;
    ident: []const u8 = "";
    ident_hash: u32 = 0;

    // Attribute selector payload
    attr_op: AttrOperator = AttrOperator.Exists;
    attr_val: []const u8 = "";
    attr_case_insensitive: bool = false;

    // Pseudo-class payload
    pseudo_kind: PseudoKind = PseudoKind.Root;
    nth_a: i32 = 0;
    nth_b: i32 = 0;
    sub_selector_raw: []const u8 = "";
    has_combinator: Combinator = Combinator.Descendant;

    // Combinator payload
    combinator: Combinator = Combinator.Descendant;
}

pub struct Selector {
    raw: []const u8 = "";
    // Stored in Servo match order: compounds right-to-left, simple selectors left-to-right
    components: [24]Component = [Component{}; 24];
    comp_count: usize = 0;
    specificity: Specificity = Specificity{};

    fn specificityU32(): u32 {
        return _.specificity.toU32();
    }
}

pub struct SelectorList {
    selectors: [8]Selector = [Selector{}; 8];
    count: usize = 0;

    static fn parse(input: []const u8): SelectorList {
        let list = SelectorList{};
        let start: usize = 0;
        let i: usize = 0;
        let paren_depth: usize = 0;
        let bracket_depth: usize = 0;

        while (i < input.len) : (i += 1) {
            const c = input[i];
            if (c == '(') {
                paren_depth += 1;
            } else if (c == ')' && paren_depth > 0) {
                paren_depth -= 1;
            } else if (c == '[') {
                bracket_depth += 1;
            } else if (c == ']' && bracket_depth > 0) {
                bracket_depth -= 1;
            } else if (c == ',' && paren_depth == 0 && bracket_depth == 0) {
                const seg = std.mem.trim(u8, input[start..i], " \t\r\n");
                if (seg.len > 0 && list.count < MAX_SELECTORS_PER_LIST) {
                    list.selectors[list.count] = SelectorParser.parseSingleSelector(seg);
                    list.count += 1;
                }
                start = i + 1;
            }
        }

        const last_seg = std.mem.trim(u8, input[start..input.len], " \t\r\n");
        if (last_seg.len > 0 && list.count < MAX_SELECTORS_PER_LIST) {
            list.selectors[list.count] = SelectorParser.parseSingleSelector(last_seg);
            list.count += 1;
        }
        return list;
    }
}

pub struct NthFormula {
    a: i32 = 0;
    b: i32 = 0;
}

pub struct CompoundSpan {
    start: usize = 0;
    len: usize = 0;
    combinator_before: Combinator = Combinator.Descendant;
    has_combinator_before: bool = false;
}

pub struct SelectorParser {
    static fn isIdentChar(c: u8): bool {
        return (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '-' ||
            c == '_';
    }

    /// Parses `an+b`, `odd`, `even`, `2n+1`, `-n+3`, `5` into `NthFormula`.
    static fn parseNthFormula(expr_raw: []const u8): NthFormula {
        const s = std.mem.trim(u8, expr_raw, " \t\r\n");
        if (s.len == 0) {
            return NthFormula { a: 0, b: 0 };
        }
        if (std.ascii.eqlIgnoreCase(s, "odd")) {
            return NthFormula { a: 2, b: 1 };
        }
        if (std.ascii.eqlIgnoreCase(s, "even")) {
            return NthFormula { a: 2, b: 0 };
        }

        // Remove spaces inside formula like `2n + 1`
        let compact: [32]u8 = [0; 32];
        let clen: usize = 0;
        let idx: usize = 0;
        while (idx < s.len && clen < 32) : (idx += 1) {
            const ch = s[idx];
            if (ch != ' ' && ch != '\t') {
                compact[clen] = std.ascii.toLower(ch);
                clen += 1;
            }
        }
        const clean = compact[0..clen];

        if (n_pos := std.mem.indexOfScalar(u8, clean, 'n')) {
            const a_str = clean[0..n_pos];
            let a_val: i32 = 1;
            if (a_str.len == 0 || std.mem.eql(u8, a_str, "+")) {
                a_val = 1;
            } else if (std.mem.eql(u8, a_str, "-")) {
                a_val = -1;
            } else {
                a_val = std.fmt.parseInt(i32, a_str, 10) catch 0;
            }

            const b_str = clean[n_pos + 1 .. clean.len];
            let b_val: i32 = 0;
            if (b_str.len > 0) {
                b_val = std.fmt.parseInt(i32, b_str, 10) catch 0;
            }
            return NthFormula { a: a_val, b: b_val };
        }

        const const_b = std.fmt.parseInt(i32, clean, 10) catch 0;
        return NthFormula { a: 0, b: const_b };
    }

    static fn parseAttributeComponent(raw_inside: []const u8): Component {
        const s = std.mem.trim(u8, raw_inside, " \t\r\n");
        let comp = Component {
            kind: ComponentKind.Attribute,
            attr_op: AttrOperator.Exists,
        };
        if (s.len == 0) {
            return comp;
        }

        if (eq_idx := std.mem.indexOfScalar(u8, s, '=')) {
            let lhs = s[0..eq_idx];
            let op = AttrOperator.Equal;
            if (lhs.len > 0) {
                const last_c = lhs[lhs.len - 1];
                if (last_c == '~') {
                    op = AttrOperator.Includes;
                    lhs = lhs[0 .. lhs.len - 1];
                } else if (last_c == '|') {
                    op = AttrOperator.DashMatch;
                    lhs = lhs[0 .. lhs.len - 1];
                } else if (last_c == '^') {
                    op = AttrOperator.Prefix;
                    lhs = lhs[0 .. lhs.len - 1];
                } else if (last_c == '$') {
                    op = AttrOperator.Suffix;
                    lhs = lhs[0 .. lhs.len - 1];
                } else if (last_c == '*') {
                    op = AttrOperator.Substring;
                    lhs = lhs[0 .. lhs.len - 1];
                }
            }
            comp.ident = std.mem.trim(u8, lhs, " \t");
            comp.attr_op = op;

            let rhs = std.mem.trim(u8, s[eq_idx + 1 .. s.len], " \t");
            // Check trailing `i` / `s` case-sensitivity modifier (with or without space after quote)
            if (rhs.len >= 2 && (rhs[rhs.len - 2] == ' ' || rhs[rhs.len - 2] == '\t' || rhs[rhs.len - 2] == '"' || rhs[rhs.len - 2] == '\'')) {
                const flag_ch = rhs[rhs.len - 1];
                if (flag_ch == 'i' || flag_ch == 'I') {
                    comp.attr_case_insensitive = true;
                    rhs = std.mem.trim(u8, rhs[0 .. rhs.len - 1], " \t");
                } else if (flag_ch == 's' || flag_ch == 'S') {
                    comp.attr_case_insensitive = false;
                    rhs = std.mem.trim(u8, rhs[0 .. rhs.len - 1], " \t");
                }
            }
            if (rhs.len >= 2 && ((rhs[0] == '"' && rhs[rhs.len - 1] == '"') || (rhs[0] == '\'' && rhs[rhs.len - 1] == '\''))) {
                rhs = rhs[1 .. rhs.len - 1];
            }
            comp.attr_val = rhs;
        } else {
            comp.ident = s;
        }
        return comp;
    }

    static fn maxSpecificityOfList(raw_list: []const u8): Specificity {
        let best = Specificity{};
        let best_score: u32 = 0;
        let start: usize = 0;
        let i: usize = 0;
        let depth: usize = 0;

        while (i <= raw_list.len) : (i += 1) {
            const at_end = (i == raw_list.len);
            if (!at_end) {
                if (raw_list[i] == '(') depth += 1;
                if (raw_list[i] == ')' && depth > 0) depth -= 1;
            }
            if (at_end || (raw_list[i] == ',' && depth == 0)) {
                let sub = std.mem.trim(u8, raw_list[start..i], " \t\r\n");
                if (sub.len > 0 && (sub[0] == '>' || sub[0] == '+' || sub[0] == '~')) {
                    sub = std.mem.trim(u8, sub[1..sub.len], " \t\r\n");
                }
                if (sub.len > 0) {
                    const parsed = SelectorParser.parseSingleSelector(sub);
                    const sc = parsed.specificity.toU32();
                    if (sc >= best_score) {
                        best_score = sc;
                        best = parsed.specificity;
                    }
                }
                start = i + 1;
            }
        }
        return best;
    }

    static fn extractPseudoArg(s: []const u8, i_ptr: *usize): []const u8 {
        let i = i_ptr.*;
        if (i < s.len && s[i] == '(') {
            i += 1;
            const arg_start = i;
            let depth: usize = 1;
            while (i < s.len && depth > 0) : (i += 1) {
                if (s[i] == '(') depth += 1;
                if (s[i] == ')') depth -= 1;
            }
            i_ptr.* = i;
            if (i > arg_start) {
                return std.mem.trim(u8, s[arg_start .. i - 1], " \t\r\n");
            }
        }
        return "";
    }

    /// Parses a single complex selector into Servo's match-ordered `Selector` (compounds reversed right-to-left,
    /// simple selectors within each compound preserved left-to-right).
    static fn parseSingleSelector(raw_in: []const u8): Selector {
        const s = std.mem.trim(u8, raw_in, " \t\r\n");
        let result = Selector { raw: s };
        if (s.len == 0) {
            return result;
        }

        let left_to_right: [24]Component = [Component{}; 24];
        let ltr_count: usize = 0;

        let compounds: [12]CompoundSpan = [CompoundSpan{}; 12];
        let compound_count: usize = 0;
        let cur_compound_start: usize = 0;
        let cur_compound_len: usize = 0;
        let pending_combinator = Combinator.Descendant;
        let has_pending_combinator = false;

        let spec = Specificity{};
        let i: usize = 0;

        while (i < s.len && ltr_count < MAX_COMPONENTS_PER_SELECTOR) {
            const c = s[i];

            // Combinators: whitespace, '>', '+', '~'
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0c || c == '>' || c == '+' || c == '~') {
                let comb = Combinator.Descendant;
                while (i < s.len) {
                    const ch = s[i];
                    if (ch == '>') {
                        comb = Combinator.Child;
                        i += 1;
                    } else if (ch == '+') {
                        comb = Combinator.NextSibling;
                        i += 1;
                    } else if (ch == '~') {
                        comb = Combinator.LaterSibling;
                        i += 1;
                    } else if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == 0x0c) {
                        i += 1;
                    } else {
                        break;
                    }
                }

                if (cur_compound_len > 0 && compound_count < 12) {
                    compounds[compound_count] = CompoundSpan {
                        start: cur_compound_start,
                        len: cur_compound_len,
                        combinator_before: pending_combinator,
                        has_combinator_before: has_pending_combinator,
                    };
                    compound_count += 1;
                    cur_compound_start = ltr_count;
                    cur_compound_len = 0;
                    pending_combinator = comb;
                    has_pending_combinator = true;
                }
                continue;
            }

            // Universal selector '*'
            if (c == '*') {
                left_to_right[ltr_count] = Component { kind: ComponentKind.Universal };
                ltr_count += 1;
                cur_compound_len += 1;
                i += 1;
                continue;
            }

            // ID selector '#id'
            if (c == '#') {
                i += 1;
                const id_name = scanCssIdent(s, &i);
                if (id_name.len > 0) {
                    left_to_right[ltr_count] = Component {
                        kind: ComponentKind.Id,
                        ident: id_name,
                    };
                    ltr_count += 1;
                    cur_compound_len += 1;
                    spec.id_selectors += 1;
                }
                continue;
            }

            // Class selector '.class'
            if (c == '.') {
                i += 1;
                const cls_name = scanCssIdent(s, &i);
                if (cls_name.len > 0) {
                    left_to_right[ltr_count] = Component {
                        kind: ComponentKind.Class,
                        ident: cls_name,
                    };
                    ltr_count += 1;
                    cur_compound_len += 1;
                    spec.class_like_selectors += 1;
                }
                continue;
            }

            // Attribute selector '[...]'
            if (c == '[') {
                i += 1;
                const start = i;
                let quote: u8 = 0;
                while (i < s.len) : (i += 1) {
                    const ch = s[i];
                    if (quote != 0) {
                        if (ch == '\\' && i + 1 < s.len) {
                            i += 1;
                            continue;
                        }
                        if (ch == quote) {
                            quote = 0;
                        }
                    } else {
                        if (ch == '"' || ch == '\'') {
                            quote = ch;
                        } else if (ch == ']') {
                            break;
                        }
                    }
                }
                const inside = s[start..i];
                if (i < s.len && s[i] == ']') {
                    i += 1;
                }
                left_to_right[ltr_count] = SelectorParser.parseAttributeComponent(inside);
                ltr_count += 1;
                cur_compound_len += 1;
                spec.class_like_selectors += 1;
                continue;
            }

            // Pseudo-class or Pseudo-element ':' / '::'
            if (c == ':') {
                if (i + 1 < s.len && s[i + 1] == ':') {
                    // Pseudo-element `::before` / `::after`
                    i += 2;
                    while (i < s.len && SelectorParser.isIdentChar(s[i])) : (i += 1) {}
                    spec.element_selectors += 1;
                    continue;
                }
                i += 1;
                const name_start = i;
                while (i < s.len && SelectorParser.isIdentChar(s[i])) : (i += 1) {}
                const p_name = s[name_start..i];

                const arg_raw = SelectorParser.extractPseudoArg(s, &i);

                let p_comp = Component {
                    kind: ComponentKind.Pseudo,
                    ident: p_name,
                    sub_selector_raw: arg_raw,
                };

                if (std.ascii.eqlIgnoreCase(p_name, "root")) {
                    p_comp.pseudo_kind = PseudoKind.Root;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "empty")) {
                    p_comp.pseudo_kind = PseudoKind.Empty;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "first-child")) {
                    p_comp.pseudo_kind = PseudoKind.FirstChild;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "last-child")) {
                    p_comp.pseudo_kind = PseudoKind.LastChild;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "only-child")) {
                    p_comp.pseudo_kind = PseudoKind.OnlyChild;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "first-of-type")) {
                    p_comp.pseudo_kind = PseudoKind.FirstOfType;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "last-of-type")) {
                    p_comp.pseudo_kind = PseudoKind.LastOfType;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "only-of-type")) {
                    p_comp.pseudo_kind = PseudoKind.OnlyOfType;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "nth-child")) {
                    p_comp.pseudo_kind = PseudoKind.NthChild;
                    const ab = SelectorParser.parseNthFormula(arg_raw);
                    p_comp.nth_a = ab.a;
                    p_comp.nth_b = ab.b;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "nth-last-child")) {
                    p_comp.pseudo_kind = PseudoKind.NthLastChild;
                    const ab = SelectorParser.parseNthFormula(arg_raw);
                    p_comp.nth_a = ab.a;
                    p_comp.nth_b = ab.b;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "nth-of-type")) {
                    p_comp.pseudo_kind = PseudoKind.NthOfType;
                    const ab = SelectorParser.parseNthFormula(arg_raw);
                    p_comp.nth_a = ab.a;
                    p_comp.nth_b = ab.b;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "nth-last-of-type")) {
                    p_comp.pseudo_kind = PseudoKind.NthLastOfType;
                    const ab = SelectorParser.parseNthFormula(arg_raw);
                    p_comp.nth_a = ab.a;
                    p_comp.nth_b = ab.b;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "where")) {
                    p_comp.pseudo_kind = PseudoKind.Where;
                    // :where() always contributes 0 specificity!
                } else if (std.ascii.eqlIgnoreCase(p_name, "is") || std.ascii.eqlIgnoreCase(p_name, "matches")) {
                    p_comp.pseudo_kind = PseudoKind.Is;
                    spec.add(SelectorParser.maxSpecificityOfList(arg_raw));
                } else if (std.ascii.eqlIgnoreCase(p_name, "not")) {
                    p_comp.pseudo_kind = PseudoKind.Not;
                    spec.add(SelectorParser.maxSpecificityOfList(arg_raw));
                } else if (std.ascii.eqlIgnoreCase(p_name, "has")) {
                    p_comp.pseudo_kind = PseudoKind.Has;
                    let rel = arg_raw;
                    let rel_comb = Combinator.Descendant;
                    if (rel.len > 0) {
                        if (rel[0] == '>') {
                            rel_comb = Combinator.Child;
                            rel = std.mem.trim(u8, rel[1..rel.len], " \t\r\n");
                        } else if (rel[0] == '+') {
                            rel_comb = Combinator.NextSibling;
                            rel = std.mem.trim(u8, rel[1..rel.len], " \t\r\n");
                        } else if (rel[0] == '~') {
                            rel_comb = Combinator.LaterSibling;
                            rel = std.mem.trim(u8, rel[1..rel.len], " \t\r\n");
                        }
                    }
                    p_comp.has_combinator = rel_comb;
                    p_comp.sub_selector_raw = rel;
                    spec.add(SelectorParser.maxSpecificityOfList(rel));
                } else if (std.ascii.eqlIgnoreCase(p_name, "focus")) {
                    p_comp.pseudo_kind = PseudoKind.Focus;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "hover")) {
                    p_comp.pseudo_kind = PseudoKind.Hover;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "dir")) {
                    p_comp.pseudo_kind = PseudoKind.Dir;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "lang")) {
                    p_comp.pseudo_kind = PseudoKind.Lang;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "link") || std.ascii.eqlIgnoreCase(p_name, "any-link")) {
                    p_comp.pseudo_kind = PseudoKind.Link;
                    spec.class_like_selectors += 1;
                } else if (std.ascii.eqlIgnoreCase(p_name, "visited")) {
                    p_comp.pseudo_kind = PseudoKind.Visited;
                    spec.class_like_selectors += 1;
                } else {
                    spec.class_like_selectors += 1;
                }

                left_to_right[ltr_count] = p_comp;
                ltr_count += 1;
                cur_compound_len += 1;
                continue;
            }

            // LocalName (type/tag selector)
            if (SelectorParser.isIdentChar(c) || (c == '\\' && i + 1 < s.len)) {
                const tag = scanCssIdent(s, &i);
                let u_buf: [128]u8 = [0; 128];
                const u_tag = unescapeCssIdent(tag, &u_buf);
                left_to_right[ltr_count] = Component {
                    kind: ComponentKind.LocalName,
                    ident: tag,
                    ident_hash: NodeData.hashLowerAscii(u_tag),
                };
                ltr_count += 1;
                cur_compound_len += 1;
                spec.element_selectors += 1;
                continue;
            }

            i += 1;
        }

        if (cur_compound_len > 0 && compound_count < 12) {
            compounds[compound_count] = CompoundSpan {
                start: cur_compound_start,
                len: cur_compound_len,
                combinator_before: pending_combinator,
                has_combinator_before: has_pending_combinator,
            };
            compound_count += 1;
        }

        // Emit into `result.components` in Servo match order:
        // Compound selectors from right to left; simple selectors within each compound from left to right.
        let out_idx: usize = 0;
        let c_idx: usize = compound_count;
        while (c_idx > 0) {
            c_idx -= 1;
            const span = compounds[c_idx];
            let k: usize = 0;
            while (k < span.len && out_idx < MAX_COMPONENTS_PER_SELECTOR) : (k += 1) {
                result.components[out_idx] = left_to_right[span.start + k];
                out_idx += 1;
            }
            if (span.has_combinator_before && out_idx < MAX_COMPONENTS_PER_SELECTOR) {
                result.components[out_idx] = Component {
                    kind: ComponentKind.CombinatorComp,
                    combinator: span.combinator_before,
                };
                out_idx += 1;
            }
        }

        result.comp_count = out_idx;
        result.specificity = spec;
        return result;
    }
}

pub struct Matcher {
    static fn strEql(a: []const u8, b: []const u8, case_insensitive: bool): bool {
        if (case_insensitive) {
            return std.ascii.eqlIgnoreCase(a, b);
        }
        return std.mem.eql(u8, a, b);
    }

    static fn matchesAttribute(node: *const NodeData, comp: *const Component): bool {
        const val_opt = node.getAttribute(comp.ident);
        if (val_opt == null) {
            return false;
        }
        const actual = val_opt ?? "";
        let unesc_buf: [512]u8 = [0; 512];
        let expected = comp.attr_val;
        if (std.mem.indexOfScalar(u8, expected, '\\') != null) {
            let ui: usize = 0;
            let ulen: usize = 0;
            while (ui < expected.len && ulen < unesc_buf.len) : (ui += 1) {
                if (expected[ui] == '\\' && ui + 1 < expected.len) {
                    ui += 1;
                }
                unesc_buf[ulen] = expected[ui];
                ulen += 1;
            }
            expected = unesc_buf[0..ulen];
        }
        const ci = comp.attr_case_insensitive;

        return switch (comp.attr_op) {
            .Exists => true,
            .Equal => Matcher.strEql(actual, expected, ci),
            .Includes => {
                if (expected.len == 0) return false;
                let it = std.mem.tokenizeAny(u8, actual, " \t\r\n");
                while (word := it.next()) {
                    if (Matcher.strEql(word, expected, ci)) {
                        return true;
                    }
                }
                return false;
            },
            .DashMatch => {
                if (Matcher.strEql(actual, expected, ci)) {
                    return true;
                }
                if (actual.len > expected.len && actual[expected.len] == '-') {
                    return Matcher.strEql(actual[0..expected.len], expected, ci);
                }
                return false;
            },
            .Prefix => {
                if (expected.len == 0 || actual.len < expected.len) return false;
                return Matcher.strEql(actual[0..expected.len], expected, ci);
            },
            .Suffix => {
                if (expected.len == 0 || actual.len < expected.len) return false;
                return Matcher.strEql(actual[actual.len - expected.len .. actual.len], expected, ci);
            },
            .Substring => {
                if (expected.len == 0 || actual.len < expected.len) return false;
                if (!ci) {
                    return std.mem.indexOf(u8, actual, expected) != null;
                }
                let pos: usize = 0;
                while (pos + expected.len <= actual.len) : (pos += 1) {
                    if (std.ascii.eqlIgnoreCase(actual[pos .. pos + expected.len], expected)) {
                        return true;
                    }
                }
                return false;
            },
        };
    }

    /// Evaluates whether 1-based `index` satisfies $a \cdot n + b = \text{index}$ for integer $n \ge 0$.
    static fn matchesNthEquation(a: i32, b: i32, index: i32): bool {
        if (a == 0) {
            return index == b;
        }
        const diff = index - b;
        if (a > 0) {
            if (diff < 0) return false;
            return @rem(diff, a) == 0;
        }
        // a < 0
        if (diff > 0) return false;
        return @rem(diff, a) == 0;
    }

    static fn matchesPseudo(doc: *const Document, node_id: NodeId, comp: *const Component): bool {
        const node = &doc.nodes.items[@as(usize, node_id)];
        const parent_id = node.parent;

        return switch (comp.pseudo_kind) {
            .Root => node_id == doc.root_element,
            .Empty => doc.isEmptyElement(node_id),
            .FirstChild => node.element_index_in_parent == 1,
            .LastChild => {
                if (parent_id == NULL_NODE) return false;
                const total = doc.nodes.items[@as(usize, parent_id)].child_element_count;
                return total > 0 && node.element_index_in_parent == total;
            },
            .OnlyChild => {
                if (parent_id == NULL_NODE) return false;
                return doc.nodes.items[@as(usize, parent_id)].child_element_count == 1;
            },
            .FirstOfType => {
                let prev = doc.prevElementSibling(node_id);
                while (prev != NULL_NODE) {
                    if (doc.nodes.items[@as(usize, prev)].hasTag(node.tag_name)) {
                        return false;
                    }
                    prev = doc.prevElementSibling(prev);
                }
                return true;
            },
            .LastOfType => {
                let next = doc.nextElementSibling(node_id);
                while (next != NULL_NODE) {
                    if (doc.nodes.items[@as(usize, next)].hasTag(node.tag_name)) {
                        return false;
                    }
                    next = doc.nextElementSibling(next);
                }
                return true;
            },
            .OnlyOfType => {
                let prev = doc.prevElementSibling(node_id);
                while (prev != NULL_NODE) {
                    if (doc.nodes.items[@as(usize, prev)].hasTag(node.tag_name)) return false;
                    prev = doc.prevElementSibling(prev);
                }
                let next = doc.nextElementSibling(node_id);
                while (next != NULL_NODE) {
                    if (doc.nodes.items[@as(usize, next)].hasTag(node.tag_name)) return false;
                    next = doc.nextElementSibling(next);
                }
                return true;
            },
            .NthChild => {
                if (node.element_index_in_parent == 0) return false;
                return Matcher.matchesNthEquation(comp.nth_a, comp.nth_b, @intCast(node.element_index_in_parent));
            },
            .NthLastChild => {
                if (parent_id == NULL_NODE || node.element_index_in_parent == 0) return false;
                const total = doc.nodes.items[@as(usize, parent_id)].child_element_count;
                const from_end: i32 = @intCast(total - node.element_index_in_parent + 1);
                return Matcher.matchesNthEquation(comp.nth_a, comp.nth_b, from_end);
            },
            .NthOfType => {
                if (parent_id == NULL_NODE) return false;
                let idx: i32 = 1;
                let prev = doc.prevElementSibling(node_id);
                while (prev != NULL_NODE) {
                    if (doc.nodes.items[@as(usize, prev)].hasTag(node.tag_name)) {
                        idx += 1;
                    }
                    prev = doc.prevElementSibling(prev);
                }
                return Matcher.matchesNthEquation(comp.nth_a, comp.nth_b, idx);
            },
            .NthLastOfType => {
                if (parent_id == NULL_NODE) return false;
                let from_end: i32 = 1;
                let next = doc.nextElementSibling(node_id);
                while (next != NULL_NODE) {
                    if (doc.nodes.items[@as(usize, next)].hasTag(node.tag_name)) {
                        from_end += 1;
                    }
                    next = doc.nextElementSibling(next);
                }
                return Matcher.matchesNthEquation(comp.nth_a, comp.nth_b, from_end);
            },
            .Is, .Where => {
                const sub_list = SelectorList.parse(comp.sub_selector_raw);
                return Matcher.matchesSelectorList(doc, node_id, sub_list);
            },
            .Not => {
                const sub_list = SelectorList.parse(comp.sub_selector_raw);
                if (Matcher.matchesSelectorList(doc, node_id, sub_list)) {
                    return false;
                }
                return true;
            },
            .Has => {
                const sub_list = SelectorList.parse(comp.sub_selector_raw);
                return switch (comp.has_combinator) {
                    .Child => {
                        let ch = node.first_child;
                        while (ch != NULL_NODE) {
                            if (doc.nodes.items[@as(usize, ch)].node_type == NodeType.Element) {
                                if (Matcher.matchesSelectorList(doc, ch, sub_list)) return true;
                            }
                            ch = doc.nodes.items[@as(usize, ch)].next_sibling;
                        }
                        return false;
                    },
                    .NextSibling => {
                        const nxt = doc.nextElementSibling(node_id);
                        if (nxt == NULL_NODE) return false;
                        return Matcher.matchesSelectorList(doc, nxt, sub_list);
                    },
                    .LaterSibling => {
                        let nxt = doc.nextElementSibling(node_id);
                        while (nxt != NULL_NODE) {
                            if (Matcher.matchesSelectorList(doc, nxt, sub_list)) return true;
                            nxt = doc.nextElementSibling(nxt);
                        }
                        return false;
                    },
                    .Descendant => {
                        return Matcher.subtreeMatches(doc, node_id, sub_list);
                    },
                };
            },
            .Focus => node_id != NULL_NODE && node_id == doc.focused_node,
            .Hover => Matcher.isHovered(doc, node_id),
            .Dir => doc.matchesDir(node_id, comp.sub_selector_raw),
            .Lang => doc.matchesLang(node_id, comp.sub_selector_raw),
            .Link => (std.ascii.eqlIgnoreCase(node.tag_name, "a") || std.ascii.eqlIgnoreCase(node.tag_name, "area") || std.ascii.eqlIgnoreCase(node.tag_name, "link")) && node.getAttribute("href") != null,
            .Visited => false,
        };
    }

    static fn isHovered(doc: *const Document, node_id: NodeId): bool {
        if (node_id == NULL_NODE || doc.hovered_node == NULL_NODE) return false;
        let cur = doc.hovered_node;
        let guard: usize = 0;
        while (cur != NULL_NODE && @as(usize, cur) < doc.nodes.items.len && guard < 1024) : (guard += 1) {
            if (cur == node_id) return true;
            cur = doc.nodes.items[@as(usize, cur)].parent;
        }
        return false;
    }

    static fn subtreeMatches(doc: *const Document, root_id: NodeId, sub_list: SelectorList): bool {
        let ch = doc.nodes.items[@as(usize, root_id)].first_child;
        while (ch != NULL_NODE) {
            if (doc.nodes.items[@as(usize, ch)].node_type == NodeType.Element) {
                if (Matcher.matchesSelectorList(doc, ch, sub_list)) {
                    return true;
                }
                if (Matcher.subtreeMatches(doc, ch, sub_list)) {
                    return true;
                }
            }
            ch = doc.nodes.items[@as(usize, ch)].next_sibling;
        }
        return false;
    }

    static fn matchesSimpleComponent(doc: *const Document, node_id: NodeId, comp: *const Component): bool {
        const node = &doc.nodes.items[@as(usize, node_id)];
        if (node.node_type != NodeType.Element) {
            return false;
        }
        return switch (comp.kind) {
            .Universal => true,
            .LocalName => {
                let u_buf: [128]u8 = [0; 128];
                const u_tag = unescapeCssIdent(comp.ident, &u_buf);
                if (node.tag_name.len != u_tag.len) return false;
                if (node.local_name_hash != comp.ident_hash) return false;
                return std.ascii.eqlIgnoreCase(node.tag_name, u_tag);
            },
            .Id => {
                let u_buf: [128]u8 = [0; 128];
                const target_id = unescapeCssIdent(comp.ident, &u_buf);
                if (doc.in_quirks_mode) {
                    if (actual_id := node.getAttribute("id")) {
                        return std.ascii.eqlIgnoreCase(actual_id, target_id);
                    }
                    return false;
                }
                return node.hasId(target_id);
            },
            .Class => {
                let u_buf: [128]u8 = [0; 128];
                const target_cls = unescapeCssIdent(comp.ident, &u_buf);
                if (doc.in_quirks_mode) {
                    if (cls_attr := node.getAttribute("class")) {
                        let it = std.mem.tokenizeAny(u8, cls_attr, " \t\r\n");
                        while (word := it.next()) {
                            if (std.ascii.eqlIgnoreCase(word, target_cls)) return true;
                        }
                    }
                    return false;
                }
                return node.hasClass(target_cls);
            },
            .Attribute => Matcher.matchesAttribute(node, comp),
            .Pseudo => Matcher.matchesPseudo(doc, node_id, comp),
            .CombinatorComp => false,
        };
    }

    /// Recursive state machine over Servo's reverse-ordered `components` slice starting at `comp_idx`.
    static fn matchFromIndex(doc: *const Document, node_id: NodeId, sel: *const Selector, comp_idx: usize): bool {
        if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len) {
            return false;
        }
        let idx = comp_idx;
        while (idx < sel.comp_count) {
            const comp = &sel.components[idx];
            if (comp.kind == ComponentKind.CombinatorComp) {
                const next_idx = idx + 1;
                const node = &doc.nodes.items[@as(usize, node_id)];
                return switch (comp.combinator) {
                    .Child => {
                        const p = node.parent;
                        if (p == NULL_NODE || doc.nodes.items[@as(usize, p)].node_type != NodeType.Element) {
                            return false;
                        }
                        return Matcher.matchFromIndex(doc, p, sel, next_idx);
                    },
                    .Descendant => {
                        let anc = node.parent;
                        while (anc != NULL_NODE) {
                            if (doc.nodes.items[@as(usize, anc)].node_type == NodeType.Element) {
                                if (Matcher.matchFromIndex(doc, anc, sel, next_idx)) {
                                    return true;
                                }
                            }
                            anc = doc.nodes.items[@as(usize, anc)].parent;
                        }
                        return false;
                    },
                    .NextSibling => {
                        const prev = doc.prevElementSibling(node_id);
                        if (prev == NULL_NODE) return false;
                        return Matcher.matchFromIndex(doc, prev, sel, next_idx);
                    },
                    .LaterSibling => {
                        let prev = doc.prevElementSibling(node_id);
                        while (prev != NULL_NODE) {
                            if (Matcher.matchFromIndex(doc, prev, sel, next_idx)) {
                                return true;
                            }
                            prev = doc.prevElementSibling(prev);
                        }
                        return false;
                    },
                };
            }

            if (!Matcher.matchesSimpleComponent(doc, node_id, comp)) {
                return false;
            }
            idx += 1;
        }
        return true;
    }

    static fn matchesSelector(doc: *const Document, node_id: NodeId, sel: *const Selector): bool {
        if (sel.comp_count == 0) {
            return false;
        }
        return Matcher.matchFromIndex(doc, node_id, sel, 0);
    }

    static fn matchesSelectorList(doc: *const Document, node_id: NodeId, list: SelectorList): bool {
        let i: usize = 0;
        while (i < list.count) : (i += 1) {
            if (Matcher.matchesSelector(doc, node_id, &list.selectors[i])) {
                return true;
            }
        }
        return false;
    }

    /// Evaluates a `SelectorList` against `node_id` and returns `(matched, max_specificity_u32)`.
    /// Per W3C Selectors Level 4, when multiple selectors in a selector list match an element,
    /// the most specific matching selector's specificity applies in cascade resolution.
    static fn matchBestSpecificity(doc: *const Document, node_id: NodeId, list: *const SelectorList): u32? {
        let matched = false;
        let max_spec: u32 = 0;
        let i: usize = 0;
        while (i < list.count) : (i += 1) {
            const sel = &list.selectors[i];
            if (Matcher.matchesSelector(doc, node_id, sel)) {
                matched = true;
                const sp = sel.specificityU32();
                if (sp >= max_spec) {
                    max_spec = sp;
                }
            }
        }
        if (!matched) {
            return null;
        }
        return max_spec;
    }

    /// Cache-friendly O(N) linear scan over `doc.nodes.items[1..]` returning the first matching `NodeId`.
    static fn querySelector(doc: *const Document, selector_str: []const u8): NodeId {
        const list = SelectorList.parse(selector_str);
        if (list.count == 0) {
            return NULL_NODE;
        }
        let i: usize = 1;
        while (i < doc.nodes.items.len) : (i += 1) {
            if (doc.nodes.items[i].node_type == NodeType.Element) {
                const nid: NodeId = @intCast(i);
                if (doc.isConnectedNode(nid) && Matcher.matchesSelectorList(doc, nid, list)) {
                    return nid;
                }
            }
        }
        return NULL_NODE;
    }

    /// Cache-friendly O(N) linear scan over `doc.nodes.items[1..]` writing all matching `NodeId`s into `out_ids`.
    static fn querySelectorAll(doc: *const Document, selector_str: []const u8, out_ids: []NodeId): usize {
        const list = SelectorList.parse(selector_str);
        if (list.count == 0) {
            return 0;
        }
        let count: usize = 0;
        let i: usize = 1;
        while (i < doc.nodes.items.len && count < out_ids.len) : (i += 1) {
            if (doc.nodes.items[i].node_type == NodeType.Element) {
                const nid: NodeId = @intCast(i);
                if (doc.isConnectedNode(nid) && Matcher.matchesSelectorList(doc, nid, list)) {
                    out_ids[count] = nid;
                    count += 1;
                }
            }
        }
        return count;
    }
}
