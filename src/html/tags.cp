// src/html/tags.cp
// CapSurf HTML5 Standard Tag Enumeration and Scope Classification Sets
// High performance 1-byte enum mapping to accelerate TreeBuilder dispatch.

pub enum HtmlTag: u8 {
    Unknown = 0,

    A,
    Abbr,
    Address,
    Area,
    Article,
    Aside,
    Audio,

    B,
    Base,
    Bdi,
    Bdo,
    Blockquote,
    Body,
    Br,
    Button,

    Canvas,
    Caption,
    Center,
    Cite,
    Code,
    Col,
    Colgroup,

    Data,
    Datalist,
    Dd,
    Del,
    Details,
    Dfn,
    Dialog,
    Div,
    Dl,
    Dt,

    Em,
    Embed,

    Fieldset,
    Figcaption,
    Figure,
    Footer,
    Form,
    Frame,
    Frameset,

    H1,
    H2,
    H3,
    H4,
    H5,
    H6,
    Head,
    Header,
    Hgroup,
    Hr,
    Html,

    I,
    Iframe,
    Img,
    Input,
    Ins,

    Kbd,

    Label,
    Legend,
    Li,
    Link,

    Main,
    Map,
    Mark,
    Marquee,
    Math,
    Menu,
    Meta,
    Meter,

    Nav,
    Nobr,
    Noembed,
    Noframes,
    Noscript,

    Object,
    Ol,
    Optgroup,
    Option,
    Output,

    P,
    Param,
    Picture,
    Plaintext,
    Pre,
    Progress,

    Q,

    Rb,
    Rp,
    Rt,
    Rtc,
    Ruby,

    S,
    Samp,
    Script,
    Section,
    Select,
    Slot,
    Small,
    Source,
    Span,
    Strike,
    Strong,
    Style,
    Sub,
    Summary,
    Sup,
    Svg,

    Table,
    Tbody,
    Td,
    Template,
    Textarea,
    Tfoot,
    Th,
    Thead,
    Time,
    Title,
    Tr,
    Track,
    Tt,

    U,
    Ul,

    Var,
    Video,

    Wbr,

    Xmp,

    /// Self-closing void elements per HTML5 spec section 13.1.2.
    fn isVoid(): bool {
        return switch (_) {
            .Area, .Base, .Br, .Col, .Embed, .Hr, .Img, .Input, .Link, .Meta, .Param, .Source, .Track, .Wbr => true,
            * => false,
        };
    }

    /// Elements that switch tokenizer to RawText mode.
    fn isRawText(): bool {
        return switch (_) {
            .Style, .Xmp, .Iframe, .Noembed, .Noframes => true,
            * => false,
        };
    }

    /// Elements that switch tokenizer to RcData mode.
    fn isRcData(): bool {
        return switch (_) {
            .Title, .Textarea => true,
            * => false,
        };
    }

    /// Elements that switch tokenizer to ScriptData mode.
    fn isScript(): bool {
        return _ == .Script;
    }

    /// Formatting elements trackable in Active Formatting Elements (AFE) per HTML5 spec 13.2.4.3.
    fn isFormatting(): bool {
        return switch (_) {
            .A, .B, .Code, .Em, .I, .Nobr, .S, .Small, .Strike, .Strong, .Tt, .U => true,
            * => false,
        };
    }

    /// Special elements per HTML5 spec 13.2.4.2 (has specific parsing behaviors across insertion modes).
    fn isSpecial(): bool {
        return switch (_) {
            .Address, .Area, .Article, .Aside, .Base, .Blockquote, .Body, .Br, .Button, .Caption, .Center,
            .Col, .Colgroup, .Dd, .Details, .Div, .Dl, .Dt, .Embed, .Fieldset, .Figcaption, .Figure,
            .Footer, .Form, .Frame, .Frameset, .H1, .H2, .H3, .H4, .H5, .H6, .Head, .Header, .Hgroup,
            .Hr, .Html, .Iframe, .Img, .Input, .Li, .Link, .Main, .Marquee, .Menu, .Meta, .Nav,
            .Noembed, .Noframes, .Noscript, .Object, .Ol, .P, .Param, .Plaintext, .Pre, .Script, .Section,
            .Select, .Source, .Style, .Summary, .Table, .Tbody, .Td, .Template, .Textarea, .Tfoot, .Th,
            .Thead, .Title, .Tr, .Track, .Ul, .Wbr, .Xmp => true,
            * => false,
        };
    }

    fn isHeading(): bool {
        return switch (_) {
            .H1, .H2, .H3, .H4, .H5, .H6 => true,
            * => false,
        };
    }

    fn isDefaultScope(): bool {
        return switch (_) {
            .Caption, .Html, .Table, .Td, .Th, .Marquee, .Object, .Select, .Template => true,
            * => false,
        };
    }

    fn isListItemScope(): bool {
        return _.isDefaultScope() || _ == .Ol || _ == .Ul;
    }

    fn isButtonScope(): bool {
        return _.isDefaultScope() || _ == .Button;
    }

    fn isTableScope(): bool {
        return _ == .Html || _ == .Table || _ == .Template;
    }

    fn isTableBodyContext(): bool {
        return _ == .Tbody || _ == .Tfoot || _ == .Thead || _ == .Template || _ == .Html;
    }

    fn isTableRowContext(): bool {
        return _ == .Tr || _ == .Template || _ == .Html;
    }

    fn isCursoryImpliedEnd(): bool {
        return switch (_) {
            .Dd, .Dt, .Li, .Option, .Optgroup, .P, .Rb, .Rp, .Rt, .Rtc => true,
            * => false,
        };
    }

    fn isThoroughImpliedEnd(): bool {
        return _.isCursoryImpliedEnd() || switch (_) {
            .Caption, .Colgroup, .Tbody, .Td, .Tfoot, .Th, .Thead, .Tr => true,
            * => false,
        };
    }

    /// Maps case-insensitive ASCII string to `HtmlTag` via length pre-filtering and byte comparison.
    static fn fromStr(s: []const u8): HtmlTag {
        if (s.len == 0 || s.len > 10) return .Unknown;
        return switch (s.len) {
            1 => switch (std.ascii.toLower(s[0])) {
                'a' => .A,
                'b' => .B,
                'i' => .I,
                'p' => .P,
                'q' => .Q,
                's' => .S,
                'u' => .U,
                * => .Unknown,
            },
            2 => {
                const c0 = std.ascii.toLower(s[0]);
                const c1 = std.ascii.toLower(s[1]);
                if (c0 == 'b') {
                    if (c1 == 'r') return .Br;
                } else if (c0 == 'd') {
                    if (c1 == 'd') return .Dd;
                    if (c1 == 'l') return .Dl;
                    if (c1 == 't') return .Dt;
                } else if (c0 == 'e') {
                    if (c1 == 'm') return .Em;
                } else if (c0 == 'h') {
                    if (c1 == '1') return .H1;
                    if (c1 == '2') return .H2;
                    if (c1 == '3') return .H3;
                    if (c1 == '4') return .H4;
                    if (c1 == '5') return .H5;
                    if (c1 == '6') return .H6;
                    if (c1 == 'r') return .Hr;
                } else if (c0 == 'l') {
                    if (c1 == 'i') return .Li;
                } else if (c0 == 'o') {
                    if (c1 == 'l') return .Ol;
                } else if (c0 == 'r') {
                    if (c1 == 'b') return .Rb;
                    if (c1 == 'p') return .Rp;
                    if (c1 == 't') return .Rt;
                } else if (c0 == 't') {
                    if (c1 == 'd') return .Td;
                    if (c1 == 'h') return .Th;
                    if (c1 == 'r') return .Tr;
                    if (c1 == 't') return .Tt;
                } else if (c0 == 'u') {
                    if (c1 == 'l') return .Ul;
                }
                return .Unknown;
            },
            3 => {
                if (std.ascii.eqlIgnoreCase(s, "div")) return .Div;
                if (std.ascii.eqlIgnoreCase(s, "nav")) return .Nav;
                if (std.ascii.eqlIgnoreCase(s, "img")) return .Img;
                if (std.ascii.eqlIgnoreCase(s, "col")) return .Col;
                if (std.ascii.eqlIgnoreCase(s, "pre")) return .Pre;
                if (std.ascii.eqlIgnoreCase(s, "sub")) return .Sub;
                if (std.ascii.eqlIgnoreCase(s, "sup")) return .Sup;
                if (std.ascii.eqlIgnoreCase(s, "svg")) return .Svg;
                if (std.ascii.eqlIgnoreCase(s, "del")) return .Del;
                if (std.ascii.eqlIgnoreCase(s, "ins")) return .Ins;
                if (std.ascii.eqlIgnoreCase(s, "kbd")) return .Kbd;
                if (std.ascii.eqlIgnoreCase(s, "map")) return .Map;
                if (std.ascii.eqlIgnoreCase(s, "bdi")) return .Bdi;
                if (std.ascii.eqlIgnoreCase(s, "bdo")) return .Bdo;
                if (std.ascii.eqlIgnoreCase(s, "dfn")) return .Dfn;
                if (std.ascii.eqlIgnoreCase(s, "var")) return .Var;
                if (std.ascii.eqlIgnoreCase(s, "wbr")) return .Wbr;
                if (std.ascii.eqlIgnoreCase(s, "xmp")) return .Xmp;
                if (std.ascii.eqlIgnoreCase(s, "rtc")) return .Rtc;
                return .Unknown;
            },
            4 => {
                if (std.ascii.eqlIgnoreCase(s, "html")) return .Html;
                if (std.ascii.eqlIgnoreCase(s, "head")) return .Head;
                if (std.ascii.eqlIgnoreCase(s, "body")) return .Body;
                if (std.ascii.eqlIgnoreCase(s, "span")) return .Span;
                if (std.ascii.eqlIgnoreCase(s, "form")) return .Form;
                if (std.ascii.eqlIgnoreCase(s, "link")) return .Link;
                if (std.ascii.eqlIgnoreCase(s, "meta")) return .Meta;
                if (std.ascii.eqlIgnoreCase(s, "main")) return .Main;
                if (std.ascii.eqlIgnoreCase(s, "time")) return .Time;
                if (std.ascii.eqlIgnoreCase(s, "slot")) return .Slot;
                if (std.ascii.eqlIgnoreCase(s, "ruby")) return .Ruby;
                if (std.ascii.eqlIgnoreCase(s, "mark")) return .Mark;
                if (std.ascii.eqlIgnoreCase(s, "menu")) return .Menu;
                if (std.ascii.eqlIgnoreCase(s, "data")) return .Data;
                if (std.ascii.eqlIgnoreCase(s, "cite")) return .Cite;
                if (std.ascii.eqlIgnoreCase(s, "code")) return .Code;
                if (std.ascii.eqlIgnoreCase(s, "abbr")) return .Abbr;
                if (std.ascii.eqlIgnoreCase(s, "base")) return .Base;
                if (std.ascii.eqlIgnoreCase(s, "area")) return .Area;
                if (std.ascii.eqlIgnoreCase(s, "samp")) return .Samp;
                if (std.ascii.eqlIgnoreCase(s, "nobr")) return .Nobr;
                return .Unknown;
            },
            5 => {
                if (std.ascii.eqlIgnoreCase(s, "table")) return .Table;
                if (std.ascii.eqlIgnoreCase(s, "tbody")) return .Tbody;
                if (std.ascii.eqlIgnoreCase(s, "tfoot")) return .Tfoot;
                if (std.ascii.eqlIgnoreCase(s, "thead")) return .Thead;
                if (std.ascii.eqlIgnoreCase(s, "title")) return .Title;
                if (std.ascii.eqlIgnoreCase(s, "style")) return .Style;
                if (std.ascii.eqlIgnoreCase(s, "input")) return .Input;
                if (std.ascii.eqlIgnoreCase(s, "audio")) return .Audio;
                if (std.ascii.eqlIgnoreCase(s, "video")) return .Video;
                if (std.ascii.eqlIgnoreCase(s, "aside")) return .Aside;
                if (std.ascii.eqlIgnoreCase(s, "embed")) return .Embed;
                if (std.ascii.eqlIgnoreCase(s, "meter")) return .Meter;
                if (std.ascii.eqlIgnoreCase(s, "param")) return .Param;
                if (std.ascii.eqlIgnoreCase(s, "small")) return .Small;
                if (std.ascii.eqlIgnoreCase(s, "track")) return .Track;
                if (std.ascii.eqlIgnoreCase(s, "label")) return .Label;
                return .Unknown;
            },
            6 => {
                if (std.ascii.eqlIgnoreCase(s, "button")) return .Button;
                if (std.ascii.eqlIgnoreCase(s, "select")) return .Select;
                if (std.ascii.eqlIgnoreCase(s, "option")) return .Option;
                if (std.ascii.eqlIgnoreCase(s, "header")) return .Header;
                if (std.ascii.eqlIgnoreCase(s, "footer")) return .Footer;
                if (std.ascii.eqlIgnoreCase(s, "script")) return .Script;
                if (std.ascii.eqlIgnoreCase(s, "iframe")) return .Iframe;
                if (std.ascii.eqlIgnoreCase(s, "canvas")) return .Canvas;
                if (std.ascii.eqlIgnoreCase(s, "figure")) return .Figure;
                if (std.ascii.eqlIgnoreCase(s, "source")) return .Source;
                if (std.ascii.eqlIgnoreCase(s, "strong")) return .Strong;
                if (std.ascii.eqlIgnoreCase(s, "strike")) return .Strike;
                if (std.ascii.eqlIgnoreCase(s, "legend")) return .Legend;
                if (std.ascii.eqlIgnoreCase(s, "dialog")) return .Dialog;
                if (std.ascii.eqlIgnoreCase(s, "hgroup")) return .Hgroup;
                if (std.ascii.eqlIgnoreCase(s, "center")) return .Center;
                if (std.ascii.eqlIgnoreCase(s, "output")) return .Output;
                return .Unknown;
            },
            7 => {
                if (std.ascii.eqlIgnoreCase(s, "section")) return .Section;
                if (std.ascii.eqlIgnoreCase(s, "article")) return .Article;
                if (std.ascii.eqlIgnoreCase(s, "caption")) return .Caption;
                if (std.ascii.eqlIgnoreCase(s, "details")) return .Details;
                if (std.ascii.eqlIgnoreCase(s, "summary")) return .Summary;
                if (std.ascii.eqlIgnoreCase(s, "address")) return .Address;
                if (std.ascii.eqlIgnoreCase(s, "picture")) return .Picture;
                if (std.ascii.eqlIgnoreCase(s, "marquee")) return .Marquee;
                return .Unknown;
            },
            8 => {
                if (std.ascii.eqlIgnoreCase(s, "template")) return .Template;
                if (std.ascii.eqlIgnoreCase(s, "textarea")) return .Textarea;
                if (std.ascii.eqlIgnoreCase(s, "colgroup")) return .Colgroup;
                if (std.ascii.eqlIgnoreCase(s, "fieldset")) return .Fieldset;
                if (std.ascii.eqlIgnoreCase(s, "datalist")) return .Datalist;
                if (std.ascii.eqlIgnoreCase(s, "optgroup")) return .Optgroup;
                if (std.ascii.eqlIgnoreCase(s, "progress")) return .Progress;
                if (std.ascii.eqlIgnoreCase(s, "noscript")) return .Noscript;
                if (std.ascii.eqlIgnoreCase(s, "noembed")) return .Noembed;
                if (std.ascii.eqlIgnoreCase(s, "noframes")) return .Noframes;
                return .Unknown;
            },
            10 => {
                if (std.ascii.eqlIgnoreCase(s, "blockquote")) return .Blockquote;
                if (std.ascii.eqlIgnoreCase(s, "figcaption")) return .Figcaption;
                if (std.ascii.eqlIgnoreCase(s, "plaintext")) return .Plaintext;
                return .Unknown;
            },
            * => .Unknown,
        };
    }

    fn nameStr(): []const u8 {
        return switch (_) {
            .A => "a",
            .Abbr => "abbr",
            .Address => "address",
            .Area => "area",
            .Article => "article",
            .Aside => "aside",
            .Audio => "audio",
            .B => "b",
            .Base => "base",
            .Bdi => "bdi",
            .Bdo => "bdo",
            .Blockquote => "blockquote",
            .Body => "body",
            .Br => "br",
            .Button => "button",
            .Canvas => "canvas",
            .Caption => "caption",
            .Center => "center",
            .Cite => "cite",
            .Code => "code",
            .Col => "col",
            .Colgroup => "colgroup",
            .Data => "data",
            .Datalist => "datalist",
            .Dd => "dd",
            .Del => "del",
            .Details => "details",
            .Dfn => "dfn",
            .Dialog => "dialog",
            .Div => "div",
            .Dl => "dl",
            .Dt => "dt",
            .Em => "em",
            .Embed => "embed",
            .Fieldset => "fieldset",
            .Figcaption => "figcaption",
            .Figure => "figure",
            .Footer => "footer",
            .Form => "form",
            .Frame => "frame",
            .Frameset => "frameset",
            .H1 => "h1",
            .H2 => "h2",
            .H3 => "h3",
            .H4 => "h4",
            .H5 => "h5",
            .H6 => "h6",
            .Head => "head",
            .Header => "header",
            .Hgroup => "hgroup",
            .Hr => "hr",
            .Html => "html",
            .I => "i",
            .Iframe => "iframe",
            .Img => "img",
            .Input => "input",
            .Ins => "ins",
            .Kbd => "kbd",
            .Label => "label",
            .Legend => "legend",
            .Li => "li",
            .Link => "link",
            .Main => "main",
            .Map => "map",
            .Mark => "mark",
            .Marquee => "marquee",
            .Math => "math",
            .Menu => "menu",
            .Meta => "meta",
            .Meter => "meter",
            .Nav => "nav",
            .Nobr => "nobr",
            .Noembed => "noembed",
            .Noframes => "noframes",
            .Noscript => "noscript",
            .Object => "object",
            .Ol => "ol",
            .Optgroup => "optgroup",
            .Option => "option",
            .Output => "output",
            .P => "p",
            .Param => "param",
            .Picture => "picture",
            .Plaintext => "plaintext",
            .Pre => "pre",
            .Progress => "progress",
            .Q => "q",
            .Rb => "rb",
            .Rp => "rp",
            .Rt => "rt",
            .Rtc => "rtc",
            .Ruby => "ruby",
            .S => "s",
            .Samp => "samp",
            .Script => "script",
            .Section => "section",
            .Select => "select",
            .Slot => "slot",
            .Small => "small",
            .Source => "source",
            .Span => "span",
            .Strike => "strike",
            .Strong => "strong",
            .Style => "style",
            .Sub => "sub",
            .Summary => "summary",
            .Sup => "sup",
            .Svg => "svg",
            .Table => "table",
            .Tbody => "tbody",
            .Td => "td",
            .Template => "template",
            .Textarea => "textarea",
            .Tfoot => "tfoot",
            .Th => "th",
            .Thead => "thead",
            .Time => "time",
            .Title => "title",
            .Tr => "tr",
            .Track => "track",
            .Tt => "tt",
            .U => "u",
            .Ul => "ul",
            .Var => "var",
            .Video => "video",
            .Wbr => "wbr",
            .Xmp => "xmp",
            .Unknown => "unknown",
        };
    }
}
