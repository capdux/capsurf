// Migrated from vendor/kitesurf/packages/engine/src/cdp/linear-regex.ts
// Pure CP Pike VM Linear-time Regular Expression Engine for CapSurf CDP Egress Filters
// Strictly O(input_len * program_size), backtracking-free and ReDoS immune.

pub const MAX_REPEAT: usize = 1000;
pub const MAX_PROGRAM: usize = 8192;
pub const SIZE_LIMIT: usize = MAX_PROGRAM + 1;
pub const INFINITY_REPEAT: u32 = 0x7fffffff;
pub const GENERATION_RESET: i32 = 0x7fff0000;

pub struct Range {
    low: u32 = 0,
    high: u32 = 0,
}

pub const DIGITS_RANGES: [1]Range = [
    Range{ low: 0x30, high: 0x39 },
];

pub const WORD_RANGES: [4]Range = [
    Range{ low: 0x30, high: 0x39 },
    Range{ low: 0x41, high: 0x5a },
    Range{ low: 0x5f, high: 0x5f },
    Range{ low: 0x61, high: 0x7a },
];

pub const SPACES_RANGES: [10]Range = [
    Range{ low: 0x09, high: 0x0d },
    Range{ low: 0x20, high: 0x20 },
    Range{ low: 0xa0, high: 0xa0 },
    Range{ low: 0x1680, high: 0x1680 },
    Range{ low: 0x2000, high: 0x200a },
    Range{ low: 0x2028, high: 0x2029 },
    Range{ low: 0x202f, high: 0x202f },
    Range{ low: 0x205f, high: 0x205f },
    Range{ low: 0x3000, high: 0x3000 },
    Range{ low: 0xfeff, high: 0xfeff },
];

pub const DIGITS_COMPLEMENT: [2]Range = [
    Range{ low: 0x0000, high: 0x002f },
    Range{ low: 0x003a, high: 0xffff },
];

pub const WORD_COMPLEMENT: [5]Range = [
    Range{ low: 0x0000, high: 0x002f },
    Range{ low: 0x003a, high: 0x0040 },
    Range{ low: 0x005b, high: 0x005e },
    Range{ low: 0x0060, high: 0x0060 },
    Range{ low: 0x007b, high: 0xffff },
];

pub const SPACES_COMPLEMENT: [11]Range = [
    Range{ low: 0x0000, high: 0x0008 },
    Range{ low: 0x000e, high: 0x001f },
    Range{ low: 0x0021, high: 0x009f },
    Range{ low: 0x00a1, high: 0x167f },
    Range{ low: 0x1681, high: 0x1fff },
    Range{ low: 0x200b, high: 0x2027 },
    Range{ low: 0x202a, high: 0x202e },
    Range{ low: 0x2030, high: 0x205e },
    Range{ low: 0x2060, high: 0x2fff },
    Range{ low: 0x3001, high: 0xfefe },
    Range{ low: 0xff00, high: 0xffff },
];

fn inRanges(ranges: []const Range, code: u32): bool {
    let i: usize = 0;
    while (i < ranges.len) : (i += 1) {
        if (code >= ranges[i].low && code <= ranges[i].high) return true;
    }
    return false;
}

fn isDigitByte(code: u8): bool {
    return (code >= '0' && code <= '9');
}

fn isLineTerminator(code: u32): bool {
    return (code == 0x0a || code == 0x0d || code == 0x2028 || code == 0x2029);
}

fn isHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn hexVal(c: u8): u32 {
    if (c >= '0' && c <= '9') return @as(u32, c - '0');
    if (c >= 'a' && c <= 'f') return @as(u32, c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return @as(u32, c - 'A' + 10);
    return 0;
}

fn addSize(total: usize, size: usize): usize {
    const sum = total + size;
    return if (sum < SIZE_LIMIT) sum else SIZE_LIMIT;
}

fn multiplySize(count: usize, size: usize): usize {
    if (size == 0 || count == 0) return 0;
    const prod = count * size;
    return if (prod < SIZE_LIMIT) prod else SIZE_LIMIT;
}

pub struct ReadDigitsResult {
    has_value: bool = false,
    value: u32 = 0,
    next: usize = 0,
}

fn readDigits(src: []const u8, start: usize): ReadDigitsResult {
    let end = start;
    while (end < src.len && isDigitByte(src[end])) {
        end += 1;
    }
    if (end == start) {
        return ReadDigitsResult{ .has_value = false, .value = 0, .next = start };
    }
    let val: u32 = 0;
    let i = start;
    while (i < end) : (i += 1) {
        val = val * 10 + @as(u32, src[i] - '0');
    }
    return ReadDigitsResult{ .has_value = true, .value = val, .next = end };
}

pub enum NodeKind: u8 {
    Empty,
    Char,
    Any,
    Word,
    Class,
    Bol,
    Eol,
    Cat,
    Alt,
    Repeat,
}

pub struct AstNode {
    kind: NodeKind = NodeKind.Empty,
    c: u32 = 0,
    neg: bool = false,
    ranges_start: u32 = 0,
    ranges_count: u32 = 0,
    children_start: u32 = 0,
    children_count: u32 = 0,
    body: u32 = 0,
    min: u32 = 0,
    max: u32 = 0,
}

pub enum InstOp: u8 {
    Char,
    Any,
    Word,
    Class,
    Split,
    Jmp,
    Bol,
    Eol,
    Match,
}

pub struct Inst {
    op: InstOp = InstOp.Match,
    c: u32 = 0,
    neg: bool = false,
    ranges: []const Range = [],
    ranges_start: u32 = 0,
    ranges_count: u32 = 0,
    x: i32 = 0,
    y: i32 = 0,
}

pub struct StartSet {
    has_ascii: bool = false,
    ascii: [128]u8 = [0; 128],
    non_ascii: bool = false,
    seed: []usize = [],
    has_seed: bool = false,
    seed_has_match: bool = false,
    anchored: bool = false,
}

fn canStart(start: *const StartSet, code: u32): bool {
    if (!start.has_ascii) return true;
    if (code < 128) {
        return start.ascii[code] == 1;
    }
    return start.non_ascii;
}

pub struct QuantifierResult {
    min: u32 = 0,
    max: u32 = 0,
}

pub struct Parser {
    src: []const u8 = "",
    i: usize = 0,
    allocator: std.mem.Allocator,
    nodes: std.ArrayListUnmanaged(AstNode) = std.ArrayListUnmanaged(AstNode).empty,
    children: std.ArrayListUnmanaged(u32) = std.ArrayListUnmanaged(u32).empty,
    ranges: std.ArrayListUnmanaged(Range) = std.ArrayListUnmanaged(Range).empty,
    temp_ranges: std.ArrayListUnmanaged(Range) = std.ArrayListUnmanaged(Range).empty,
    program: std.ArrayListUnmanaged(Inst) = std.ArrayListUnmanaged(Inst).empty,

    fn deinit(*_) {
        _.nodes.deinit(_.allocator);
        _.children.deinit(_.allocator);
        _.ranges.deinit(_.allocator);
        _.temp_ranges.deinit(_.allocator);
        _.program.deinit(_.allocator);
    }

    fn allocNode(*_, kind: NodeKind): (u32)! {
        const idx = @as(u32, @intCast(_.nodes.items.len));
        _.nodes.append(_.allocator, AstNode{ .kind = kind })?;
        return idx;
    }

    fn parse(*_): (u32)! {
        const node = _.alternation()?;
        if (_.i < _.src.len) return error.UnexpectedChar;
        return node;
    }

    fn alternation(*_): (u32)! {
        const first = _.concat()?;
        if (_.i >= _.src.len || _.src[_.i] != '|') return first;

        let items = std.ArrayListUnmanaged(u32).empty;
        defer items.deinit(_.allocator);
        items.append(_.allocator, first)?;

        while (_.i < _.src.len && _.src[_.i] == '|') {
            _.i += 1;
            const next_item = _.concat()?;
            items.append(_.allocator, next_item)?;
        }

        const start_child = @as(u32, @intCast(_.children.items.len));
        let i: usize = 0;
        while (i < items.items.len) : (i += 1) {
            _.children.append(_.allocator, items.items[i])?;
        }
        const count = @as(u32, @intCast(items.items.len));

        const alt_idx = _.allocNode(NodeKind.Alt)?;
        _.nodes.items[alt_idx].children_start = start_child;
        _.nodes.items[alt_idx].children_count = count;
        return alt_idx;
    }

    fn concat(*_): (u32)! {
        let items = std.ArrayListUnmanaged(u32).empty;
        defer items.deinit(_.allocator);

        while (_.i < _.src.len) {
            const ch = _.src[_.i];
            if (ch == '|' || ch == ')') break;
            const t = _.term()?;
            items.append(_.allocator, t)?;
        }

        if (items.items.len == 0) {
            return _.allocNode(NodeKind.Empty);
        }
        if (items.items.len == 1) {
            return items.items[0];
        }

        const start_child = @as(u32, @intCast(_.children.items.len));
        let i: usize = 0;
        while (i < items.items.len) : (i += 1) {
            _.children.append(_.allocator, items.items[i])?;
        }
        const count = @as(u32, @intCast(items.items.len));

        const cat_idx = _.allocNode(NodeKind.Cat)?;
        _.nodes.items[cat_idx].children_start = start_child;
        _.nodes.items[cat_idx].children_count = count;
        return cat_idx;
    }

    fn term(*_): (u32)! {
        const atom_node = _.atom()?;
        const rep = _.quantifier()?;
        if (rep == null) return atom_node;

        const q = rep.?;
        const rep_idx = _.allocNode(NodeKind.Repeat)?;
        _.nodes.items[rep_idx].body = atom_node;
        _.nodes.items[rep_idx].min = q.min;
        _.nodes.items[rep_idx].max = q.max;
        return rep_idx;
    }

    fn atom(*_): (u32)! {
        if (_.i >= _.src.len) return error.UnexpectedChar;
        const ch = _.src[_.i];
        if (ch == '(') return _.group();
        if (ch == '[') return _.charClass();
        if (ch == '\\') {
            const next = if (_.i + 1 < _.src.len) _.src[_.i + 1] else 0;
            if (next == 'b' || next == 'B') {
                _.i += 2;
                const w_idx = _.allocNode(NodeKind.Word)?;
                _.nodes.items[w_idx].neg = (next == 'B');
                return w_idx;
            }
            if (next == 'c') return _.controlEscape();

            const r_start = @as(u32, @intCast(_.ranges.items.len));
            const r_save = _.temp_ranges.items.len;
            _.readEscape()?;
            let k = r_save;
            while (k < _.temp_ranges.items.len) : (k += 1) {
                _.ranges.append(_.allocator, _.temp_ranges.items[k])?;
            }
            _.temp_ranges.items.len = r_save;
            const r_count = @as(u32, @intCast(_.ranges.items.len)) - r_start;

            const cls_idx = _.allocNode(NodeKind.Class)?;
            _.nodes.items[cls_idx].neg = false;
            _.nodes.items[cls_idx].ranges_start = r_start;
            _.nodes.items[cls_idx].ranges_count = r_count;
            return cls_idx;
        }

        _.i += 1;
        if (ch == '.') return _.allocNode(NodeKind.Any);
        if (ch == '^') return _.allocNode(NodeKind.Bol);
        if (ch == '$') return _.allocNode(NodeKind.Eol);

        const ch_idx = _.allocNode(NodeKind.Char)?;
        _.nodes.items[ch_idx].c = @as(u32, ch);
        return ch_idx;
    }

    fn controlEscape(*_): (u32)! {
        const letter = if (_.i + 2 < _.src.len) _.src[_.i + 2] else 0;
        if ((letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z')) {
            _.i += 3;
            const ch_idx = _.allocNode(NodeKind.Char)?;
            _.nodes.items[ch_idx].c = @as(u32, letter % 32);
            return ch_idx;
        }
        _.i += 1;
        const ch_idx = _.allocNode(NodeKind.Char)?;
        _.nodes.items[ch_idx].c = 0x5c;
        return ch_idx;
    }

    fn group(*_): (u32)! {
        _.i += 1; // skip '('
        if (_.i < _.src.len && _.src[_.i] == '?') {
            const kind = if (_.i + 1 < _.src.len) _.src[_.i + 1] else 0;
            const after = if (_.i + 2 < _.src.len) _.src[_.i + 2] else 0;
            if (kind == ':') {
                _.i += 2;
            } else if (kind == '<' && after != '=' && after != '!') {
                let close_pos: usize = 0;
                let found = false;
                let j = _.i + 2;
                while (j < _.src.len) : (j += 1) {
                    if (_.src[j] == '>') {
                        close_pos = j;
                        found = true;
                        break;
                    }
                }
                if (!found) return error.UnterminatedGroupName;
                _.i = close_pos + 1;
            } else {
                return error.UnsupportedGroupKind;
            }
        }
        const inner = _.alternation()?;
        if (_.i >= _.src.len || _.src[_.i] != ')') return error.UnterminatedGroup;
        _.i += 1;
        return inner;
    }

    fn charClass(*_): (u32)! {
        _.i += 1; // skip '['
        const neg = (_.i < _.src.len && _.src[_.i] == '^');
        if (neg) _.i += 1;

        const r_start = @as(u32, @intCast(_.ranges.items.len));
        while (_.i < _.src.len && _.src[_.i] != ']') {
            _.classItem()?;
        }
        if (_.i >= _.src.len || _.src[_.i] != ']') return error.UnterminatedCharClass;
        _.i += 1; // skip ']'

        const r_count = @as(u32, @intCast(_.ranges.items.len)) - r_start;
        const cls_idx = _.allocNode(NodeKind.Class)?;
        _.nodes.items[cls_idx].neg = neg;
        _.nodes.items[cls_idx].ranges_start = r_start;
        _.nodes.items[cls_idx].ranges_count = r_count;
        return cls_idx;
    }

    fn classItem(*_)! {
        const low_start = _.temp_ranges.items.len;
        _.classAtom()?;
        const low_count = _.temp_ranges.items.len - low_start;

        const is_range = (_.i < _.src.len && _.src[_.i] == '-' &&
                          _.i + 1 < _.src.len && _.src[_.i + 1] != ']');
        if (!is_range) {
            let k = low_start;
            while (k < _.temp_ranges.items.len) : (k += 1) {
                _.ranges.append(_.allocator, _.temp_ranges.items[k])?;
            }
            _.temp_ranges.items.len = low_start;
            return;
        }

        _.i += 1; // skip '-'
        const high_start = _.temp_ranges.items.len;
        _.classAtom()?;
        const high_count = _.temp_ranges.items.len - high_start;

        const low_code = if (low_count == 1 && _.temp_ranges.items[low_start].low == _.temp_ranges.items[low_start].high)
            _.temp_ranges.items[low_start].low
        else
            null;
        const high_code = if (high_count == 1 && _.temp_ranges.items[high_start].low == _.temp_ranges.items[high_start].high)
            _.temp_ranges.items[high_start].low
        else
            null;

        if (low_code == null || high_code == null) {
            let k = low_start;
            while (k < high_start) : (k += 1) {
                _.ranges.append(_.allocator, _.temp_ranges.items[k])?;
            }
            _.ranges.append(_.allocator, Range{ .low = 0x2d, .high = 0x2d })?;
            k = high_start;
            while (k < _.temp_ranges.items.len) : (k += 1) {
                _.ranges.append(_.allocator, _.temp_ranges.items[k])?;
            }
            _.temp_ranges.items.len = low_start;
            return;
        }

        if (low_code.? > high_code.?) {
            return error.CharClassRangeOutOfOrder;
        }

        _.ranges.append(_.allocator, Range{ .low = low_code.?, .high = high_code.? })?;
        _.temp_ranges.items.len = low_start;
    }

    fn classAtom(*_)! {
        if (_.i >= _.src.len) return error.UnexpectedChar;
        if (_.src[_.i] == '\\') {
            if (_.i + 1 < _.src.len && _.src[_.i + 1] == 'c') {
                _.classControlEscape()?;
                return;
            }
            _.readEscape()?;
            return;
        }
        const code = @as(u32, _.src[_.i]);
        _.i += 1;
        _.temp_ranges.append(_.allocator, Range{ .low = code, .high = code })?;
    }

    fn classControlEscape(*_)! {
        const letter = if (_.i + 2 < _.src.len) _.src[_.i + 2] else 0;
        if ((letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z') ||
            (letter >= '0' && letter <= '9') || letter == '_') {
            _.i += 3;
            const code = @as(u32, letter % 32);
            _.temp_ranges.append(_.allocator, Range{ .low = code, .high = code })?;
            return;
        }
        _.i += 1;
        _.temp_ranges.append(_.allocator, Range{ .low = 0x5c, .high = 0x5c })?;
    }

    fn readEscape(*_)! {
        _.i += 1; // skip '\'
        if (_.i >= _.src.len) return error.TrailingBackslash;
        const ch = _.src[_.i];
        const code = @as(u32, ch);
        _.i += 1;

        switch (ch) {
            'd' => {
                let k: usize = 0;
                while (k < DIGITS_RANGES.len) : (k += 1) {
                    _.temp_ranges.append(_.allocator, DIGITS_RANGES[k])?;
                }
                return;
            },
            'D' => {
                let k: usize = 0;
                while (k < DIGITS_COMPLEMENT.len) : (k += 1) {
                    _.temp_ranges.append(_.allocator, DIGITS_COMPLEMENT[k])?;
                }
                return;
            },
            'w' => {
                let k: usize = 0;
                while (k < WORD_RANGES.len) : (k += 1) {
                    _.temp_ranges.append(_.allocator, WORD_RANGES[k])?;
                }
                return;
            },
            'W' => {
                let k: usize = 0;
                while (k < WORD_COMPLEMENT.len) : (k += 1) {
                    _.temp_ranges.append(_.allocator, WORD_COMPLEMENT[k])?;
                }
                return;
            },
            's' => {
                let k: usize = 0;
                while (k < SPACES_RANGES.len) : (k += 1) {
                    _.temp_ranges.append(_.allocator, SPACES_RANGES[k])?;
                }
                return;
            },
            'S' => {
                let k: usize = 0;
                while (k < SPACES_COMPLEMENT.len) : (k += 1) {
                    _.temp_ranges.append(_.allocator, SPACES_COMPLEMENT[k])?;
                }
                return;
            },
            'b' => {
                _.temp_ranges.append(_.allocator, Range{ .low = 0x08, .high = 0x08 })?;
                return;
            },
            'B' => {
                _.temp_ranges.append(_.allocator, Range{ .low = 0x42, .high = 0x42 })?;
                return;
            },
            't' => {
                _.temp_ranges.append(_.allocator, Range{ .low = 0x09, .high = 0x09 })?;
                return;
            },
            'n' => {
                _.temp_ranges.append(_.allocator, Range{ .low = 0x0a, .high = 0x0a })?;
                return;
            },
            'v' => {
                _.temp_ranges.append(_.allocator, Range{ .low = 0x0b, .high = 0x0b })?;
                return;
            },
            'f' => {
                _.temp_ranges.append(_.allocator, Range{ .low = 0x0c, .high = 0x0c })?;
                return;
            },
            'r' => {
                _.temp_ranges.append(_.allocator, Range{ .low = 0x0d, .high = 0x0d })?;
                return;
            },
            '0' => {
                if (_.i < _.src.len && isDigitByte(_.src[_.i])) {
                    return error.OctalNotSupported;
                }
                _.temp_ranges.append(_.allocator, Range{ .low = 0x00, .high = 0x00 })?;
                return;
            },
            'x' => {
                const hex_opt = _.readHex(2);
                if (val := hex_opt) {
                    _.temp_ranges.append(_.allocator, Range{ .low = val, .high = val })?;
                } else {
                    _.temp_ranges.append(_.allocator, Range{ .low = 0x78, .high = 0x78 })?;
                }
                return;
            },
            'u' => {
                const hex_opt = _.readHex(4);
                if (val := hex_opt) {
                    _.temp_ranges.append(_.allocator, Range{ .low = val, .high = val })?;
                } else {
                    _.temp_ranges.append(_.allocator, Range{ .low = 0x75, .high = 0x75 })?;
                }
                return;
            },
            else => {},
        }

        if (code >= 0x31 && code <= 0x37) return error.BackreferencesNotSupported;
        if (ch == 'k') return error.BackreferencesNotSupported;
        _.temp_ranges.append(_.allocator, Range{ .low = code, .high = code })?;
    }

    fn readHex(*_, digits: usize): (u32)? {
        if (_.src.len - _.i < digits) return null;
        let k: usize = 0;
        while (k < digits) : (k += 1) {
            if (!isHexDigit(_.src[_.i + k])) return null;
        }
        let val: u32 = 0;
        k = 0;
        while (k < digits) : (k += 1) {
            val = val * 16 + hexVal(_.src[_.i + k]);
        }
        _.i += digits;
        return val;
    }

    fn quantifier(*_): (QuantifierResult)?! {
        if (_.i >= _.src.len) return null;
        const ch = _.src[_.i];
        if (ch == '*') {
            _.i += 1;
            _.skipLazy();
            return QuantifierResult{ .min = 0, .max = INFINITY_REPEAT };
        }
        if (ch == '+') {
            _.i += 1;
            _.skipLazy();
            return QuantifierResult{ .min = 1, .max = INFINITY_REPEAT };
        }
        if (ch == '?') {
            _.i += 1;
            _.skipLazy();
            return QuantifierResult{ .min = 0, .max = 1 };
        }
        if (ch == '{') {
            return _.bracedQuantifier();
        }
        return null;
    }

    fn bracedQuantifier(*_): (QuantifierResult)?! {
        const lower = readDigits(_.src, _.i + 1);
        if (!lower.has_value) return null;
        let next = lower.next;
        let max_val: u32 = lower.value;
        let has_max = true;
        if (next < _.src.len && _.src[next] == ',') {
            const upper = readDigits(_.src, next + 1);
            if (upper.has_value) {
                max_val = upper.value;
                has_max = true;
                next = upper.next;
            } else {
                max_val = INFINITY_REPEAT;
                has_max = false;
                next = next + 1;
            }
        }
        if (next >= _.src.len || _.src[next] != '}') return null;
        if (lower.value > MAX_REPEAT || (has_max && max_val > MAX_REPEAT)) {
            return error.RepeatBoundTooLarge;
        }
        if (has_max && lower.value > max_val) {
            return error.RepeatOutOfOrder;
        }
        _.i = next + 1;
        _.skipLazy();
        return QuantifierResult{ .min = lower.value, .max = max_val };
    }

    fn skipLazy(*_) {
        if (_.i < _.src.len && _.src[_.i] == '?') {
            _.i += 1;
        }
    }

    fn expandedSize(*_, node_idx: u32): usize {
        const node = &_.nodes.items[node_idx];
        return switch (node.kind) {
            .Empty => 0,
            .Char, .Any, .Word, .Class, .Bol, .Eol => 1,
            .Cat => {
                let total: usize = 0;
                let i: usize = 0;
                while (i < node.children_count) : (i += 1) {
                    const child_idx = _.children.items[node.children_start + i];
                    total = addSize(total, _.expandedSize(child_idx));
                }
                return total;
            },
            .Alt => {
                let total = multiplySize(node.children_count - 1, 2);
                let i: usize = 0;
                while (i < node.children_count) : (i += 1) {
                    const child_idx = _.children.items[node.children_start + i];
                    total = addSize(total, _.expandedSize(child_idx));
                }
                return total;
            },
            .Repeat => {
                const body_sz = _.expandedSize(node.body);
                if (body_sz == 0) return 0;
                let optional: usize = 0;
                if (node.max == INFINITY_REPEAT) {
                    optional = addSize(body_sz, 2);
                } else {
                    optional = multiplySize(node.max - node.min, body_sz + 1);
                }
                return addSize(multiplySize(node.min, body_sz), optional);
            },
        };
    }

    fn emit(*_, node_idx: u32)! {
        if (_.program.items.len > MAX_PROGRAM) return error.PatternTooLarge;
        const node = &_.nodes.items[node_idx];
        switch (node.kind) {
            .Empty => {},
            .Char => {
                _.program.append(_.allocator, Inst{ .op = InstOp.Char, .c = node.c })?;
            },
            .Any => {
                _.program.append(_.allocator, Inst{ .op = InstOp.Any })?;
            },
            .Word => {
                _.program.append(_.allocator, Inst{ .op = InstOp.Word, .neg = node.neg })?;
            },
            .Class => {
                _.program.append(_.allocator, Inst{
                    .op = InstOp.Class,
                    .neg = node.neg,
                    .ranges_start = node.ranges_start,
                    .ranges_count = node.ranges_count,
                })?;
            },
            .Bol => {
                _.program.append(_.allocator, Inst{ .op = InstOp.Bol })?;
            },
            .Eol => {
                _.program.append(_.allocator, Inst{ .op = InstOp.Eol })?;
            },
            .Cat => {
                let i: usize = 0;
                while (i < node.children_count) : (i += 1) {
                    const child_idx = _.children.items[node.children_start + i];
                    _.emit(child_idx)?;
                }
            },
            .Alt => {
                _.emitAlternation(node)?;
            },
            .Repeat => {
                _.emitRepeat(node.body, node.min, node.max)?;
            },
        }
    }

    fn emitAlternation(*_, node: *const AstNode)! {
        let jumps = std.ArrayListUnmanaged(usize).empty;
        defer jumps.deinit(_.allocator);

        const last = node.children_count - 1;
        let i: usize = 0;
        while (i < node.children_count) : (i += 1) {
            const item_idx = _.children.items[node.children_start + i];
            if (i == last) {
                _.emit(item_idx)?;
                break;
            }
            const split_idx = _.program.items.len;
            _.program.append(_.allocator, Inst{
                .op = InstOp.Split,
                .x = @as(i32, @intCast(split_idx + 1)),
                .y = -1,
            })?;
            _.emit(item_idx)?;
            const jump_idx = _.program.items.len;
            _.program.append(_.allocator, Inst{
                .op = InstOp.Jmp,
                .x = -1,
            })?;
            jumps.append(_.allocator, jump_idx)?;
            _.program.items[split_idx].y = @as(i32, @intCast(_.program.items.len));
        }
        let j: usize = 0;
        while (j < jumps.items.len) : (j += 1) {
            const j_idx = jumps.items[j];
            _.program.items[j_idx].x = @as(i32, @intCast(_.program.items.len));
        }
    }

    fn emitRepeat(*_, body_idx: u32, min: u32, max: u32)! {
        if (_.expandedSize(body_idx) == 0) return;
        let count: u32 = 0;
        while (count < min) : (count += 1) {
            _.emit(body_idx)?;
        }
        if (max == INFINITY_REPEAT) {
            const start = @as(i32, @intCast(_.program.items.len));
            const split_idx = _.program.items.len;
            _.program.append(_.allocator, Inst{
                .op = InstOp.Split,
                .x = start + 1,
                .y = -1,
            })?;
            _.emit(body_idx)?;
            _.program.append(_.allocator, Inst{
                .op = InstOp.Jmp,
                .x = start,
            })?;
            _.program.items[split_idx].y = @as(i32, @intCast(_.program.items.len));
            return;
        }
        count = min;
        while (count < max) : (count += 1) {
            const split_idx = _.program.items.len;
            _.program.append(_.allocator, Inst{
                .op = InstOp.Split,
                .x = @as(i32, @intCast(split_idx + 1)),
                .y = -1,
            })?;
            _.emit(body_idx)?;
            _.program.items[split_idx].y = @as(i32, @intCast(_.program.items.len));
        }
    }
}

fn isStartAnchored(program: []const Inst, allocator: std.mem.Allocator): bool {
    let seen = allocator.alloc(u8, program.len) catch return false;
    defer allocator.free(seen);
    @memset(seen, 0);

    let stack = std.ArrayListUnmanaged(usize).empty;
    defer stack.deinit(allocator);

    stack.append(allocator, 0) catch return false;
    while (stack.items.len > 0) {
        const at = stack.items[stack.items.len - 1];
        stack.items.len -= 1;
        if (at >= program.len) return false;
        if (seen[at] == 1) continue;
        seen[at] = 1;
        const inst = &program[at];
        if (inst.op == .Bol) continue;
        if (inst.op == .Split) {
            if (inst.y >= 0) stack.append(allocator, @as(usize, @intCast(inst.y))) catch return false;
            if (inst.x >= 0) stack.append(allocator, @as(usize, @intCast(inst.x))) catch return false;
            continue;
        }
        if (inst.op == .Jmp) {
            if (inst.x >= 0) stack.append(allocator, @as(usize, @intCast(inst.x))) catch return false;
            continue;
        }
        return false;
    }
    return true;
}

fn buildStartSet(program: []const Inst, allocator: std.mem.Allocator): (StartSet)! {
    let ascii: [128]u8 = [0; 128];
    let non_ascii = false;

    let stack = std.ArrayListUnmanaged(usize).empty;
    defer stack.deinit(allocator);

    let seen = allocator.alloc(u8, program.len)?;
    defer allocator.free(seen);
    @memset(seen, 0);

    let order = std.ArrayListUnmanaged(usize).empty;

    let position_dependent = false;
    let has_match = false;

    stack.append(allocator, 0)?;
    while (stack.items.len > 0) {
        const at = stack.items[stack.items.len - 1];
        stack.items.len -= 1;
        if (at >= program.len) continue;
        if (seen[at] == 1) continue;
        seen[at] = 1;
        const inst = &program[at];
        if (inst.op == .Split) {
            if (inst.y >= 0) stack.append(allocator, @as(usize, @intCast(inst.y)))?;
            if (inst.x >= 0) stack.append(allocator, @as(usize, @intCast(inst.x)))?;
            continue;
        }
        if (inst.op == .Jmp) {
            if (inst.x >= 0) stack.append(allocator, @as(usize, @intCast(inst.x)))?;
            continue;
        }
        if (inst.op == .Bol || inst.op == .Eol || inst.op == .Word) {
            position_dependent = true;
            stack.append(allocator, at + 1)?;
            continue;
        }
        if (inst.op == .Match) {
            has_match = true;
            continue;
        }
        order.append(allocator, at)?;
        if (inst.op == .Any) {
            let c: usize = 0;
            while (c < 128) : (c += 1) {
                ascii[c] = 1;
            }
            non_ascii = true;
        } else if (inst.op == .Char) {
            if (inst.c < 128) {
                ascii[inst.c] = 1;
            } else {
                non_ascii = true;
            }
        } else if (inst.op == .Class) {
            if (!inst.neg) {
                let r_i: usize = 0;
                while (r_i < inst.ranges.len) : (r_i += 1) {
                    const r = inst.ranges[r_i];
                    let code = if (r.low < 128) r.low else 127;
                    const high = if (r.high < 127) r.high else 127;
                    while (code <= high) : (code += 1) {
                        ascii[code] = 1;
                    }
                    if (r.high > 127) non_ascii = true;
                }
            } else {
                let code: u32 = 0;
                while (code < 128) : (code += 1) {
                    if (!inRanges(inst.ranges, code)) {
                        ascii[code] = 1;
                    }
                }
                non_ascii = true;
            }
        }
    }

    let all = !non_ascii;
    let code: usize = 0;
    while (code < 128) : (code += 1) {
        if (ascii[code] == 0) {
            all = false;
            break;
        }
    }

    const has_ascii = !(has_match || (all && !non_ascii));

    let seed_slice: []usize = [];
    if (!position_dependent) {
        seed_slice = order.toOwnedSlice(allocator)?;
    } else {
        order.deinit(allocator);
    }

    const anchored = isStartAnchored(program, allocator);

    return StartSet{
        .has_ascii = has_ascii,
        .ascii = ascii,
        .non_ascii = non_ascii,
        .seed = seed_slice,
        .has_seed = !position_dependent,
        .seed_has_match = has_match,
        .anchored = anchored,
    };
}

fn consumes(inst: *const Inst, code: u32): bool {
    if (inst.op == .Char) return inst.c == code;
    if (inst.op == .Any) return !isLineTerminator(code);
    if (inst.op == .Class) return inRanges(inst.ranges, code) != inst.neg;
    return false;
}

pub struct LinearRegex {
    source: []const u8 = "",
    program: []Inst = [],
    start: StartSet = StartSet{},
    seen: []i32 = [],
    generation: i32 = 0,
    list_buf_0: []usize = [],
    list_buf_1: []usize = [],
    list_len_0: usize = 0,
    list_len_1: usize = 0,
    stack: []usize = [],
    stack_len: usize = 0,
    range_pool: []Range = [],
    allocator: (std.mem.Allocator)? = null,

    fn @"test"(*_, input: []const u8): bool {
        if (_.program.len == 0) return false;

        if (_.generation > GENERATION_RESET) {
            @memset(_.seen, -1);
            _.generation = 0;
        }

        const seed = _.start.seed;
        const seed_has_match = _.start.has_seed && _.start.seed_has_match;

        let current_idx: usize = 0;
        let next_idx: usize = 1;
        _.list_len_0 = 0;
        _.list_len_1 = 0;

        let pos: usize = 0;
        while (true) : (pos += 1) {
            const cur_len = if (current_idx == 0) _.list_len_0 else _.list_len_1;
            if (cur_len == 0) {
                if (pos > 0 && _.start.anchored) return false;
                if (pos < input.len && !canStart(&_.start, @as(u32, input[pos]))) continue;
            }

            if (_.start.has_seed && cur_len == 0) {
                let len: usize = 0;
                let s_i: usize = 0;
                while (s_i < seed.len) : (s_i += 1) {
                    const pc = seed[s_i];
                    if (current_idx == 0) {
                        _.list_buf_0[len] = pc;
                    } else {
                        _.list_buf_1[len] = pc;
                    }
                    len += 1;
                }
                if (current_idx == 0) {
                    _.list_len_0 = len;
                } else {
                    _.list_len_1 = len;
                }
                if (seed_has_match) return true;
            } else {
                _.generation += 1;
                const live_len = if (current_idx == 0) _.list_len_0 else _.list_len_1;
                let idx: usize = 0;
                while (idx < live_len) : (idx += 1) {
                    const pc = if (current_idx == 0) _.list_buf_0[idx] else _.list_buf_1[idx];
                    if (pc < _.seen.len) {
                        _.seen[pc] = _.generation;
                    }
                }
                if (_.addThread(current_idx, 0, pos, input)) return true;
            }

            if (pos == input.len) return false;

            const code = @as(u32, input[pos]);
            _.generation += 1;
            if (next_idx == 0) {
                _.list_len_0 = 0;
            } else {
                _.list_len_1 = 0;
            }

            const active_len = if (current_idx == 0) _.list_len_0 else _.list_len_1;
            let idx: usize = 0;
            while (idx < active_len) : (idx += 1) {
                const pc = if (current_idx == 0) _.list_buf_0[idx] else _.list_buf_1[idx];
                if (pc < _.program.len) {
                    const inst = &_.program[pc];
                    if (consumes(inst, code) && _.addThread(next_idx, pc + 1, pos + 1, input)) {
                        return true;
                    }
                }
            }

            const tmp = current_idx;
            current_idx = next_idx;
            next_idx = tmp;
        }
    }

    fn addThread(*_, list_idx: usize, pc: usize, pos: usize, input: []const u8): bool {
        let matched = false;
        _.stack_len = 0;
        _.stack[_.stack_len] = pc;
        _.stack_len += 1;

        while (_.stack_len > 0) {
            _.stack_len -= 1;
            const at = _.stack[_.stack_len];
            if (at >= _.program.len) continue;
            if (_.seen[at] == _.generation) continue;
            _.seen[at] = _.generation;

            const inst = &_.program[at];
            if (inst.op == .Split) {
                if (inst.y >= 0) {
                    _.stack[_.stack_len] = @as(usize, @intCast(inst.y));
                    _.stack_len += 1;
                }
                if (inst.x >= 0) {
                    _.stack[_.stack_len] = @as(usize, @intCast(inst.x));
                    _.stack_len += 1;
                }
                continue;
            }
            if (inst.op == .Jmp) {
                if (inst.x >= 0) {
                    _.stack[_.stack_len] = @as(usize, @intCast(inst.x));
                    _.stack_len += 1;
                }
                continue;
            }
            if (inst.op == .Bol) {
                if (pos == 0) {
                    _.stack[_.stack_len] = at + 1;
                    _.stack_len += 1;
                }
                continue;
            }
            if (inst.op == .Eol) {
                if (pos == input.len) {
                    _.stack[_.stack_len] = at + 1;
                    _.stack_len += 1;
                }
                continue;
            }
            if (inst.op == .Word) {
                const prev = (pos > 0 && inRanges(WORD_RANGES[0..], @as(u32, input[pos - 1])));
                const cur = (pos < input.len && inRanges(WORD_RANGES[0..], @as(u32, input[pos])));
                if ((prev != cur) != inst.neg) {
                    _.stack[_.stack_len] = at + 1;
                    _.stack_len += 1;
                }
                continue;
            }
            if (inst.op == .Match) {
                matched = true;
            }
            if (list_idx == 0) {
                _.list_buf_0[_.list_len_0] = at;
                _.list_len_0 += 1;
            } else {
                _.list_buf_1[_.list_len_1] = at;
                _.list_len_1 += 1;
            }
        }
        return matched;
    }

    fn deinit(*_) {
        if (alloc := _.allocator) {
            if (_.program.len > 0) alloc.free(_.program);
            if (_.seen.len > 0) alloc.free(_.seen);
            if (_.list_buf_0.len > 0) alloc.free(_.list_buf_0);
            if (_.list_buf_1.len > 0) alloc.free(_.list_buf_1);
            if (_.stack.len > 0) alloc.free(_.stack);
            if (_.start.seed.len > 0) alloc.free(_.start.seed);
            if (_.range_pool.len > 0) alloc.free(_.range_pool);
            if (_.source.len > 0) alloc.free(_.source);
            _.program = [];
            _.seen = [];
            _.list_buf_0 = [];
            _.list_buf_1 = [];
            _.stack = [];
            _.start.seed = [];
            _.range_pool = [];
            _.source = "";
        }
    }
}

pub struct CompileResult {
    ok: bool = false,
    regex: LinearRegex = LinearRegex{},
    error_msg: []const u8 = "",
    @"error": []const u8 = "",
}

fn compileInternal(source: []const u8, allocator: std.mem.Allocator): (LinearRegex)! {
    let parser = Parser{
        .src = source,
        .allocator = allocator,
    };
    defer parser.deinit();

    const root = parser.parse()?;
    if (parser.expandedSize(root) > MAX_PROGRAM) {
        return error.PatternTooLarge;
    }

    parser.emit(root)?;
    parser.program.append(allocator, Inst{ .op = InstOp.Match })?;

    const range_pool = parser.ranges.toOwnedSlice(allocator)?;

    let i: usize = 0;
    while (i < parser.program.items.len) : (i += 1) {
        if (parser.program.items[i].op == .Class) {
            const start = parser.program.items[i].ranges_start;
            const count = parser.program.items[i].ranges_count;
            parser.program.items[i].ranges = range_pool[start..start + count];
        }
    }

    const start_set = buildStartSet(parser.program.items, allocator)?;

    const prog_len = parser.program.items.len;
    const seen = allocator.alloc(i32, prog_len)?;
    @memset(seen, -1);

    const list_buf_0 = allocator.alloc(usize, prog_len)?;
    const list_buf_1 = allocator.alloc(usize, prog_len)?;
    const stack = allocator.alloc(usize, prog_len * 2 + 16)?;

    const source_dup = allocator.dupe(u8, source)?;
    const owned_program = parser.program.toOwnedSlice(allocator)?;

    return LinearRegex{
        .source = source_dup,
        .program = owned_program,
        .start = start_set,
        .seen = seen,
        .generation = 0,
        .list_buf_0 = list_buf_0,
        .list_buf_1 = list_buf_1,
        .list_len_0 = 0,
        .list_len_1 = 0,
        .stack = stack,
        .stack_len = 0,
        .range_pool = range_pool,
        .allocator = allocator,
    };
}

pub fn compileLinearRegex(source: []const u8): CompileResult {
    const allocator = std.heap.page_allocator;
    const res = compileInternal(source, allocator) catch |e| {
        let msg: []const u8 = "invalid regular expression";
        const ename = @errorName(e);
        if (std.mem.eql(u8, ename, "UnexpectedChar")) {
            msg = "unexpected character in regex";
        } else if (std.mem.eql(u8, ename, "UnterminatedGroup")) {
            msg = "unterminated group";
        } else if (std.mem.eql(u8, ename, "UnterminatedGroupName")) {
            msg = "unterminated group name";
        } else if (std.mem.eql(u8, ename, "UnsupportedGroupKind")) {
            msg = "only (?:...) and (?<name>...) groups are supported";
        } else if (std.mem.eql(u8, ename, "UnterminatedCharClass")) {
            msg = "unterminated character class";
        } else if (std.mem.eql(u8, ename, "CharClassRangeOutOfOrder")) {
            msg = "character class range out of order";
        } else if (std.mem.eql(u8, ename, "TrailingBackslash")) {
            msg = "trailing backslash";
        } else if (std.mem.eql(u8, ename, "OctalNotSupported")) {
            msg = "octal escapes are not supported";
        } else if (std.mem.eql(u8, ename, "BackreferencesNotSupported")) {
            msg = "backreferences are not supported";
        } else if (std.mem.eql(u8, ename, "RepeatBoundTooLarge")) {
            msg = "repeat bounds above 1000 are not supported";
        } else if (std.mem.eql(u8, ename, "RepeatOutOfOrder")) {
            msg = "numbers out of order in {} quantifier";
        } else if (std.mem.eql(u8, ename, "PatternTooLarge")) {
            msg = "pattern expands past 8192 steps";
        } else if (std.mem.eql(u8, ename, "OutOfMemory")) {
            msg = "out of memory";
        }
        return CompileResult{
            .ok = false,
            .error_msg = msg,
            .@"error" = msg,
        };
    };
    return CompileResult{
        .ok = true,
        .regex = res,
    };
}
