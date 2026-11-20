// CSS Writing Modes / HTML BiDi & UAX #9 Unicode Bidirectional Algorithm
// Pure CP implementation — zero heap allocation, stack-bounded paragraph reordering.

import { Document, NodeId, NodeType, NULL_NODE } from "../dom/dom.cp";

pub enum Direction {
    Ltr,
    Rtl,

    static fn parse(raw: []const u8): Direction? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "ltr")) return Direction.Ltr;
        if (std.ascii.eqlIgnoreCase(s, "rtl")) return Direction.Rtl;
        return null;
    }

    fn isRtl(): bool {
        return _ == Direction.Rtl;
    }
}

pub enum UnicodeBidi {
    Normal,
    Embed,
    Isolate,
    BidiOverride,
    IsolateOverride,
    Plaintext,

    static fn parse(raw: []const u8): UnicodeBidi? {
        const s = std.mem.trim(u8, raw, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "normal")) return UnicodeBidi.Normal;
        if (std.ascii.eqlIgnoreCase(s, "embed")) return UnicodeBidi.Embed;
        if (std.ascii.eqlIgnoreCase(s, "isolate")) return UnicodeBidi.Isolate;
        if (std.ascii.eqlIgnoreCase(s, "bidi-override")) return UnicodeBidi.BidiOverride;
        if (std.ascii.eqlIgnoreCase(s, "isolate-override")) return UnicodeBidi.IsolateOverride;
        if (std.ascii.eqlIgnoreCase(s, "plaintext")) return UnicodeBidi.Plaintext;
        return null;
    }
}

pub enum BidiClass {
    L,
    R,
    AL,
    EN,
    ES,
    ET,
    AN,
    CS,
    NSM,
    BN,
    B,
    S,
    WS,
    ON,
    LRE,
    LRO,
    RLE,
    RLO,
    PDF,
    LRI,
    RLI,
    FSI,
    PDI,
}

// Explicit bidi control codepoints (UAX #9)
pub const CP_LRM: u32 = 0x200E;
pub const CP_RLM: u32 = 0x200F;
pub const CP_LRE: u32 = 0x202A;
pub const CP_RLE: u32 = 0x202B;
pub const CP_PDF: u32 = 0x202C;
pub const CP_LRO: u32 = 0x202D;
pub const CP_RLO: u32 = 0x202E;
pub const CP_LRI: u32 = 0x2066;
pub const CP_RLI: u32 = 0x2067;
pub const CP_FSI: u32 = 0x2068;
pub const CP_PDI: u32 = 0x2069;

pub fn decodeUtf8Next(s: []const u8, pos: *usize): u32 {
    const i = pos.*;
    if (i >= s.len) return 0;
    const b0 = s[i];
    if (b0 < 0x80) {
        pos.* = i + 1;
        return @as(u32, b0);
    }
    if ((b0 & 0xE0) == 0xC0 && i + 1 < s.len) {
        const b1 = s[i + 1];
        if ((b1 & 0xC0) == 0x80) {
            pos.* = i + 2;
            return (@as(u32, b0 & 0x1F) << 6) | @as(u32, b1 & 0x3F);
        }
    } else if ((b0 & 0xF0) == 0xE0 && i + 2 < s.len) {
        const b1 = s[i + 1];
        const b2 = s[i + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            pos.* = i + 3;
            return (@as(u32, b0 & 0x0F) << 12) | (@as(u32, b1 & 0x3F) << 6) | @as(u32, b2 & 0x3F);
        }
    } else if ((b0 & 0xF8) == 0xF0 && i + 3 < s.len) {
        const b1 = s[i + 1];
        const b2 = s[i + 2];
        const b3 = s[i + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            pos.* = i + 4;
            return (@as(u32, b0 & 0x07) << 18) | (@as(u32, b1 & 0x3F) << 12) | (@as(u32, b2 & 0x3F) << 6) | @as(u32, b3 & 0x3F);
        }
    }
    pos.* = i + 1;
    return @as(u32, b0);
}

pub fn isBidiFormatControl(cp: u32): bool {
    return cp == CP_LRM || cp == CP_RLM ||
        (cp >= CP_LRE && cp <= CP_RLO) ||
        (cp >= CP_LRI && cp <= CP_PDI);
}

pub fn textHasBidiSignal(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const b0 = s[i];
        if (b0 < 0x80) {
            i += 1;
            continue;
        }
        const cp = decodeUtf8Next(s, &i);
        if (isBidiFormatControl(cp)) return true;
        const cls = bidiClassOf(cp);
        if (cls == BidiClass.R || cls == BidiClass.AL || cls == BidiClass.AN) return true;
    }
    return false;
}

pub fn bidiClassOf(cp: u32): BidiClass {
    // Explicit directional formatting characters
    if (cp == CP_LRM) return BidiClass.L;
    if (cp == CP_RLM) return BidiClass.R;
    if (cp == CP_LRE) return BidiClass.LRE;
    if (cp == CP_RLE) return BidiClass.RLE;
    if (cp == CP_PDF) return BidiClass.PDF;
    if (cp == CP_LRO) return BidiClass.LRO;
    if (cp == CP_RLO) return BidiClass.RLO;
    if (cp == CP_LRI) return BidiClass.LRI;
    if (cp == CP_RLI) return BidiClass.RLI;
    if (cp == CP_FSI) return BidiClass.FSI;
    if (cp == CP_PDI) return BidiClass.PDI;

    // ASCII fast path
    if (cp < 0x80) {
        if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) return BidiClass.L;
        if (cp >= '0' && cp <= '9') return BidiClass.EN;
        if (cp == '+' || cp == '-') return BidiClass.ES;
        if (cp == '#' || cp == '$' || cp == '%') return BidiClass.ET;
        if (cp == ',' || cp == '.' || cp == ':' || cp == '/') return BidiClass.CS;
        if (cp == ' ' || cp == 0x0C) return BidiClass.WS;
        if (cp == '\t') return BidiClass.S;
        if (cp == '\n' || cp == '\r') return BidiClass.B;
        return BidiClass.ON;
    }

    // Hebrew (U+0590..U+05FF)
    if (cp >= 0x0590 && cp <= 0x05FF) {
        if (cp >= 0x0591 && cp <= 0x05BD) return BidiClass.NSM;
        if (cp == 0x05BF || cp == 0x05C1 || cp == 0x05C2 || cp == 0x05C4 || cp == 0x05C5 || cp == 0x05C7) return BidiClass.NSM;
        return BidiClass.R;
    }

    // Arabic & Syriac & Thaana
    if ((cp >= 0x0600 && cp <= 0x06FF) || (cp >= 0x0750 && cp <= 0x077F) || (cp >= 0x08A0 && cp <= 0x08FF)) {
        if (cp >= 0x0660 && cp <= 0x0669) return BidiClass.AN;
        if (cp >= 0x06F0 && cp <= 0x06F9) return BidiClass.EN;
        if (cp >= 0x064B && cp <= 0x065F) return BidiClass.NSM;
        if (cp == 0x0670) return BidiClass.NSM;
        return BidiClass.AL;
    }
    if ((cp >= 0x0700 && cp <= 0x074F) || (cp >= 0x0780 && cp <= 0x07BF) || (cp >= 0x07C0 && cp <= 0x085F)) {
        return BidiClass.R;
    }

    // Hebrew & Arabic Presentation Forms
    if (cp >= 0xFB1D && cp <= 0xFB4F) return BidiClass.R;
    if ((cp >= 0xFB50 && cp <= 0xFDFF) || (cp >= 0xFE70 && cp <= 0xFEFF)) return BidiClass.AL;

    // Combining diacritical marks
    if (cp >= 0x0300 && cp <= 0x036F) return BidiClass.NSM;

    // Default strong L for Latin/Greek/Cyrillic/CJK/etc., ON for punctuation/symbols
    if (cp >= 0x2000 && cp <= 0x206F) return BidiClass.ON;
    if (cp >= 0x2070 && cp <= 0x2BFF) return BidiClass.ON;
    return BidiClass.L;
}

fn hasValidDirAttribute(doc: *const Document, node_id: NodeId): bool {
    const node = &doc.nodes.items[@as(usize, node_id)];
    if (d := node.getAttribute("dir")) {
        const s = std.mem.trim(u8, d, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(s, "ltr") ||
            std.ascii.eqlIgnoreCase(s, "rtl") ||
            std.ascii.eqlIgnoreCase(s, "auto")) {
            return true;
        }
    }
    return false;
}

fn scanFirstStrongDescendant(doc: *const Document, node_id: NodeId, depth: usize): Direction? {
    if (node_id == NULL_NODE || @as(usize, node_id) >= doc.nodes.items.len || depth > 64) {
        return null;
    }
    let ch = doc.nodes.items[@as(usize, node_id)].first_child;
    while (ch != NULL_NODE && @as(usize, ch) < doc.nodes.items.len) {
        const cn = &doc.nodes.items[@as(usize, ch)];
        if (cn.node_type == NodeType.Text) {
            let i: usize = 0;
            while (i < cn.text_content.len) {
                const cp = decodeUtf8Next(cn.text_content, &i);
                const cls = bidiClassOf(cp);
                if (cls == BidiClass.L) return Direction.Ltr;
                if (cls == BidiClass.R || cls == BidiClass.AL) return Direction.Rtl;
            }
        } else if (cn.node_type == NodeType.Element) {
            // HTML spec: skip <bdi>, <script>, <style>, <textarea>, and any element with a valid dir attribute
            const skip = cn.hasTag("bdi") || cn.hasTag("script") || cn.hasTag("style") || cn.hasTag("textarea") || hasValidDirAttribute(doc, ch);
            if (!skip) {
                if (found := scanFirstStrongDescendant(doc, ch, depth + 1)) {
                    return found;
                }
            }
        }
        ch = cn.next_sibling;
    }
    return null;
}

/// HTML Standard "The directionality" for `dir="auto"`:
/// Finds the first strong character in descendants (excluding dir-attributed descendants, <bdi>, <script>, <style>, <textarea>).
pub fn resolveFirstStrongDirection(doc: *const Document, root_id: NodeId): Direction {
    if (found := scanFirstStrongDescendant(doc, root_id, 0)) {
        return found;
    }
    return Direction.Ltr;
}

/// UAX #9 P2-P3: First strong character in a codepoint sequence, skipping isolated sub-runs (LRI/RLI/FSI .. PDI).
pub fn resolveParagraphFirstStrong(cps: []const u32): Direction {
    let isolate_depth: usize = 0;
    let i: usize = 0;
    while (i < cps.len) : (i += 1) {
        const cp = cps[i];
        if (cp == CP_LRI || cp == CP_RLI || cp == CP_FSI) {
            isolate_depth += 1;
            continue;
        }
        if (cp == CP_PDI) {
            if (isolate_depth > 0) isolate_depth -= 1;
            continue;
        }
        if (isolate_depth > 0) continue;
        const cls = bidiClassOf(cp);
        if (cls == BidiClass.L) return Direction.Ltr;
        if (cls == BidiClass.R || cls == BidiClass.AL) return Direction.Rtl;
    }
    return Direction.Ltr;
}

const MAX_BIDI_LEN: usize = 1024;
const MAX_BIDI_DEPTH: usize = 64;

struct BidiStatus {
    level: u8 = 0,
    override_cls: BidiClass = BidiClass.ON, // ON = neutral/none, L = LRO, R = RLO
    is_isolate: bool = false,
}

fn isIsolateInitiator(cls: BidiClass): bool {
    return cls == BidiClass.LRI || cls == BidiClass.RLI || cls == BidiClass.FSI;
}

fn isExplicitFormat(cls: BidiClass): bool {
    return cls == BidiClass.LRE || cls == BidiClass.RLE ||
        cls == BidiClass.LRO || cls == BidiClass.RLO ||
        cls == BidiClass.PDF || cls == BidiClass.BN;
}

fn resolveFsiDirection(cps: []const u32, start_idx: usize): bool {
    let depth: usize = 0;
    let i = start_idx + 1;
    while (i < cps.len) : (i += 1) {
        const cp = cps[i];
        if (cp == CP_LRI || cp == CP_RLI || cp == CP_FSI) {
            depth += 1;
            continue;
        }
        if (cp == CP_PDI) {
            if (depth == 0) break;
            depth -= 1;
            continue;
        }
        if (depth > 0) continue;
        const cls = bidiClassOf(cp);
        if (cls == BidiClass.L) return false;
        if (cls == BidiClass.R || cls == BidiClass.AL) return true;
    }
    return false;
}

fn nextEvenLevel(lvl: u8): u8 {
    return (lvl + 2) & 0xFE;
}

fn nextOddLevel(lvl: u8): u8 {
    return (lvl + 1) | 1;
}

fn levelDir(lvl: u8): BidiClass {
    return if ((lvl & 1) == 1) BidiClass.R else BidiClass.L;
}

fn resolveIsolatingRunSequence(
    seq_indices: []const usize,
    types: []BidiClass,
    levels: []u8,
    sos: BidiClass,
    eos: BidiClass,
) {
    if (seq_indices.len == 0) return;
    const base_lvl = levels[seq_indices[0]];
    const embed_dir = levelDir(base_lvl);

    // W1: NSM takes previous type (or sos)
    let prev_t = sos;
    let i: usize = 0;
    while (i < seq_indices.len) : (i += 1) {
        const idx = seq_indices[i];
        const t = types[idx];
        if (t == BidiClass.NSM) {
            if (isIsolateInitiator(prev_t) || prev_t == BidiClass.PDI) {
                types[idx] = BidiClass.ON;
            } else {
                types[idx] = prev_t;
            }
        }
        if (!isExplicitFormat(types[idx])) {
            prev_t = types[idx];
        }
    }

    // W2: EN -> AN if previous strong type is AL
    let last_strong = sos;
    i = 0;
    while (i < seq_indices.len) : (i += 1) {
        const idx = seq_indices[i];
        const t = types[idx];
        if (t == BidiClass.L || t == BidiClass.R || t == BidiClass.AL) {
            last_strong = t;
        } else if (t == BidiClass.EN && last_strong == BidiClass.AL) {
            types[idx] = BidiClass.AN;
        }
    }

    // W3: AL -> R
    i = 0;
    while (i < seq_indices.len) : (i += 1) {
        const idx = seq_indices[i];
        if (types[idx] == BidiClass.AL) {
            types[idx] = BidiClass.R;
        }
    }

    // W4: Single ES between EN..EN -> EN; single CS between EN..EN -> EN or AN..AN -> AN
    i = 1;
    while (i + 1 < seq_indices.len) : (i += 1) {
        const idx = seq_indices[i];
        const t = types[idx];
        const pt = types[seq_indices[i - 1]];
        const nt = types[seq_indices[i + 1]];
        if (t == BidiClass.ES && pt == BidiClass.EN && nt == BidiClass.EN) {
            types[idx] = BidiClass.EN;
        } else if (t == BidiClass.CS) {
            if (pt == BidiClass.EN && nt == BidiClass.EN) {
                types[idx] = BidiClass.EN;
            } else if (pt == BidiClass.AN && nt == BidiClass.AN) {
                types[idx] = BidiClass.AN;
            }
        }
    }

    // W5: ET adjacent to EN -> EN
    i = 0;
    while (i < seq_indices.len) : (i += 1) {
        const idx = seq_indices[i];
        if (types[idx] == BidiClass.ET) {
            let end = i + 1;
            while (end < seq_indices.len && types[seq_indices[end]] == BidiClass.ET) : (end += 1) {}
            const prev_en = (i > 0 && types[seq_indices[i - 1]] == BidiClass.EN);
            const next_en = (end < seq_indices.len && types[seq_indices[end]] == BidiClass.EN);
            if (prev_en || next_en) {
                let k = i;
                while (k < end) : (k += 1) {
                    types[seq_indices[k]] = BidiClass.EN;
                }
            }
            i = end - 1;
        }
    }

    // W6: remaining ES, ET, CS -> ON
    i = 0;
    while (i < seq_indices.len) : (i += 1) {
        const idx = seq_indices[i];
        const t = types[idx];
        if (t == BidiClass.ES || t == BidiClass.ET || t == BidiClass.CS) {
            types[idx] = BidiClass.ON;
        }
    }

    // W7: EN -> L if previous strong type is L
    last_strong = sos;
    i = 0;
    while (i < seq_indices.len) : (i += 1) {
        const idx = seq_indices[i];
        const t = types[idx];
        if (t == BidiClass.L || t == BidiClass.R) {
            last_strong = t;
        } else if (t == BidiClass.EN && last_strong == BidiClass.L) {
            types[idx] = BidiClass.L;
        }
    }

    // N1 & N2: Resolve neutral & isolate formatting types (ON, WS, S, B, LRI, RLI, FSI, PDI)
    i = 0;
    while (i < seq_indices.len) {
        const idx = seq_indices[i];
        const t = types[idx];
        const is_ni = (t == BidiClass.ON || t == BidiClass.WS || t == BidiClass.S || t == BidiClass.B ||
            t == BidiClass.LRI || t == BidiClass.RLI || t == BidiClass.FSI || t == BidiClass.PDI ||
            isExplicitFormat(t));
        if (!is_ni) {
            i += 1;
            continue;
        }
        let end = i + 1;
        while (end < seq_indices.len) : (end += 1) {
            const et = types[seq_indices[end]];
            const e_ni = (et == BidiClass.ON || et == BidiClass.WS || et == BidiClass.S || et == BidiClass.B ||
                et == BidiClass.LRI || et == BidiClass.RLI || et == BidiClass.FSI || et == BidiClass.PDI ||
                isExplicitFormat(et));
            if (!e_ni) break;
        }

        let prev_dir = sos;
        if (i > 0) {
            const pt = types[seq_indices[i - 1]];
            if (pt == BidiClass.L) {
                prev_dir = BidiClass.L;
            } else if (pt == BidiClass.R || pt == BidiClass.EN || pt == BidiClass.AN) {
                prev_dir = BidiClass.R;
            }
        }
        let next_dir = eos;
        if (end < seq_indices.len) {
            const nt = types[seq_indices[end]];
            if (nt == BidiClass.L) {
                next_dir = BidiClass.L;
            } else if (nt == BidiClass.R || nt == BidiClass.EN || nt == BidiClass.AN) {
                next_dir = BidiClass.R;
            }
        }

        const resolved = if (prev_dir == next_dir) prev_dir else embed_dir;
        let k = i;
        while (k < end) : (k += 1) {
            if (!isExplicitFormat(types[seq_indices[k]])) {
                types[seq_indices[k]] = resolved;
            }
        }
        i = end;
    }

    // I1 & I2: Resolve implicit embedding levels
    const is_odd = ((base_lvl & 1) == 1);
    i = 0;
    while (i < seq_indices.len) : (i += 1) {
        const idx = seq_indices[i];
        const t = types[idx];
        if (!is_odd) {
            if (t == BidiClass.R) {
                levels[idx] = base_lvl + 1;
            } else if (t == BidiClass.AN || t == BidiClass.EN) {
                levels[idx] = base_lvl + 2;
            }
        } else {
            if (t == BidiClass.L || t == BidiClass.EN || t == BidiClass.AN) {
                levels[idx] = base_lvl + 1;
            }
        }
    }
}

/// Reorders a paragraph of codepoints into visual order using UAX #9.
/// Writes original indices into `out_indices[0..n]` and returns `n`.
pub fn reorderVisual(cps: []const u32, base_rtl: bool, out_indices: []usize): usize {
    const n = @min(cps.len, @min(out_indices.len, MAX_BIDI_LEN));
    if (n == 0) return 0;

    let orig_types: [MAX_BIDI_LEN]BidiClass = [BidiClass.ON; MAX_BIDI_LEN];
    let types: [MAX_BIDI_LEN]BidiClass = [BidiClass.ON; MAX_BIDI_LEN];
    let levels: [MAX_BIDI_LEN]u8 = [0; MAX_BIDI_LEN];
    let match_pdi: [MAX_BIDI_LEN]i32 = [-1; MAX_BIDI_LEN];

    const base_level: u8 = if (base_rtl) 1 else 0;

    let i: usize = 0;
    while (i < n) : (i += 1) {
        const cls = bidiClassOf(cps[i]);
        orig_types[i] = cls;
        types[i] = cls;
        levels[i] = base_level;
        out_indices[i] = i;
    }

    // X1-X8: Explicit levels and directions with isolate stack
    let stack: [MAX_BIDI_DEPTH]BidiStatus = [BidiStatus{}; MAX_BIDI_DEPTH];
    let sp: usize = 0;
    stack[0] = BidiStatus {
        level: base_level,
        override_cls: BidiClass.ON,
        is_isolate: false,
    };
    let overflow_isolate: usize = 0;
    let overflow_embed: usize = 0;
    let valid_isolate: usize = 0;

    let iso_open_stack: [MAX_BIDI_DEPTH]usize = [0; MAX_BIDI_DEPTH];
    let iso_sp: usize = 0;

    let idx2: usize = 0;
    while (idx2 < n) : (idx2 += 1) {
        const cls = orig_types[idx2];
        const cur = stack[sp];

        if (cls == BidiClass.RLE || cls == BidiClass.LRE || cls == BidiClass.RLO || cls == BidiClass.LRO) {
            levels[idx2] = cur.level;
            const next_lvl = if (cls == BidiClass.RLE || cls == BidiClass.RLO) nextOddLevel(cur.level) else nextEvenLevel(cur.level);
            if (next_lvl <= 125 && overflow_isolate == 0 && overflow_embed == 0 && sp + 1 < MAX_BIDI_DEPTH) {
                sp += 1;
                let ov = BidiClass.ON;
                if (cls == BidiClass.LRO) ov = BidiClass.L;
                if (cls == BidiClass.RLO) ov = BidiClass.R;
                stack[sp] = BidiStatus {
                    level: next_lvl,
                    override_cls: ov,
                    is_isolate: false,
                };
            } else if (overflow_isolate == 0) {
                overflow_embed += 1;
            }
        } else if (cls == BidiClass.RLI || cls == BidiClass.LRI || cls == BidiClass.FSI) {
            levels[idx2] = cur.level;
            if (cur.override_cls != BidiClass.ON) {
                types[idx2] = cur.override_cls;
            }
            const is_rtl_iso = if (cls == BidiClass.RLI) true else if (cls == BidiClass.LRI) false else resolveFsiDirection(cps[0..n], idx2);
            const next_lvl = if (is_rtl_iso) nextOddLevel(cur.level) else nextEvenLevel(cur.level);
            if (next_lvl <= 125 && overflow_isolate == 0 && overflow_embed == 0 && sp + 1 < MAX_BIDI_DEPTH) {
                valid_isolate += 1;
                if (iso_sp < MAX_BIDI_DEPTH) {
                    iso_open_stack[iso_sp] = idx2;
                    iso_sp += 1;
                }
                sp += 1;
                stack[sp] = BidiStatus {
                    level: next_lvl,
                    override_cls: BidiClass.ON,
                    is_isolate: true,
                };
            } else {
                overflow_isolate += 1;
            }
        } else if (cls == BidiClass.PDI) {
            if (overflow_isolate > 0) {
                overflow_isolate -= 1;
            } else if (valid_isolate > 0) {
                overflow_embed = 0;
                while (sp > 0 && !stack[sp].is_isolate) {
                    sp -= 1;
                }
                if (sp > 0) sp -= 1;
                valid_isolate -= 1;
                if (iso_sp > 0) {
                    iso_sp -= 1;
                    match_pdi[iso_open_stack[iso_sp]] = @intCast(idx2);
                }
            }
            levels[idx2] = stack[sp].level;
            if (stack[sp].override_cls != BidiClass.ON) {
                types[idx2] = stack[sp].override_cls;
            }
        } else if (cls == BidiClass.PDF) {
            levels[idx2] = cur.level;
            if (overflow_isolate > 0) {
                // ignore
            } else if (overflow_embed > 0) {
                overflow_embed -= 1;
            } else if (!cur.is_isolate && sp > 0) {
                sp -= 1;
            }
        } else {
            levels[idx2] = cur.level;
            if (cur.override_cls != BidiClass.ON) {
                types[idx2] = cur.override_cls;
            }
        }
    }

    // BD13: Build isolating run sequences (group maximal level runs linked by LRI/RLI/FSI -> matching PDI)
    let visited: [MAX_BIDI_LEN]bool = [false; MAX_BIDI_LEN];
    let seq_buf: [MAX_BIDI_LEN]usize = [0; MAX_BIDI_LEN];

    let idx3: usize = 0;
    while (idx3 < n) : (idx3 += 1) {
        if (visited[idx3]) continue;
        if (isExplicitFormat(orig_types[idx3])) {
            visited[idx3] = true;
            continue;
        }
        const run_lvl = levels[idx3];
        let seq_len: usize = 0;
        let cur_pos: usize = idx3;

        // Determine sos from preceding non-format character (or base_level)
        let prev_lvl = base_level;
        if (idx3 > 0) {
            let p: usize = idx3;
            while (p > 0) {
                p -= 1;
                if (!isExplicitFormat(orig_types[p])) {
                    prev_lvl = levels[p];
                    break;
                }
            }
        }
        const sos = levelDir(@max(run_lvl, prev_lvl));

        let last_idx: usize = idx3;
        while (cur_pos < n) {
            if (visited[cur_pos] || levels[cur_pos] != run_lvl) break;
            if (isExplicitFormat(orig_types[cur_pos])) {
                visited[cur_pos] = true;
                cur_pos += 1;
                continue;
            }
            visited[cur_pos] = true;
            seq_buf[seq_len] = cur_pos;
            seq_len += 1;
            last_idx = cur_pos;
            if (isIsolateInitiator(orig_types[cur_pos]) && match_pdi[cur_pos] >= 0) {
                // Jump over the isolated content directly to the matching PDI
                cur_pos = @intCast(match_pdi[cur_pos]);
                continue;
            }
            cur_pos += 1;
        }

        // Determine eos from following non-format character (or base_level)
        let next_lvl = base_level;
        let q: usize = last_idx + 1;
        while (q < n) : (q += 1) {
            if (!isExplicitFormat(orig_types[q])) {
                next_lvl = levels[q];
                break;
            }
        }
        const eos = levelDir(@max(run_lvl, next_lvl));

        resolveIsolatingRunSequence(seq_buf[0..seq_len], types[0..n], levels[0..n], sos, eos);
    }

    // L1 (simplified): Trailing whitespace/separators reset to base_level
    let tr: usize = n;
    while (tr > 0) {
        tr -= 1;
        const ot = orig_types[tr];
        if (ot == BidiClass.WS || ot == BidiClass.S || ot == BidiClass.B || isIsolateInitiator(ot) || ot == BidiClass.PDI || isExplicitFormat(ot)) {
            levels[tr] = base_level;
        } else {
            break;
        }
    }

    // L2: Reverse contiguous runs from highest level down to lowest odd level
    let max_lvl: u8 = 0;
    let min_odd: u8 = 255;
    let idx4: usize = 0;
    while (idx4 < n) : (idx4 += 1) {
        if (levels[idx4] > max_lvl) max_lvl = levels[idx4];
        if ((levels[idx4] & 1) == 1 && levels[idx4] < min_odd) min_odd = levels[idx4];
    }

    if (min_odd <= max_lvl) {
        let lvl = max_lvl;
        while (lvl >= min_odd) {
            let start: usize = 0;
            while (start < n) {
                if (levels[start] >= lvl) {
                    let end = start + 1;
                    while (end < n && levels[end] >= lvl) : (end += 1) {}
                    let lo = start;
                    let hi = end - 1;
                    while (lo < hi) {
                        const tmp = out_indices[lo];
                        out_indices[lo] = out_indices[hi];
                        out_indices[hi] = tmp;
                        lo += 1;
                        hi -= 1;
                    }
                    start = end;
                } else {
                    start += 1;
                }
            }
            if (lvl == 0) break;
            lvl -= 1;
        }
    }

    return n;
}
