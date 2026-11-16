// Migrated from vendor/kitesurf/packages/dom-shim/src/element/forms/
// Implements HTML Form Controls, ValidityState, and HTMLFormElement over Arena DOM tree.

import { NodeId, NULL_NODE } from "./dom.cp";

pub enum FormControlType {
    text,
    password,
    email,
    url,
    number,
    checkbox,
    radio,
    submit,
    reset,
    button,
    hidden,
    file,
    textarea,
    select,
    search,
    tel,
    range,
    date,
    color,
}

pub struct ValidityState {
    value_missing: bool = false,
    type_mismatch: bool = false,
    pattern_mismatch: bool = false,
    too_long: bool = false,
    too_short: bool = false,
    range_underflow: bool = false,
    range_overflow: bool = false,
    step_mismatch: bool = false,
    bad_input: bool = false,
    custom_error: bool = false,

    fn valid(): bool {
        return !(_.value_missing || _.type_mismatch || _.pattern_mismatch ||
            _.too_long || _.too_short || _.range_underflow ||
            _.range_overflow || _.step_mismatch || _.bad_input || _.custom_error);
    }
}

pub const MAX_FORM_CONTROLS: usize = 64;

fn isValidEmail(s: []const u8): bool {
    if (s.len < 3) return false;
    let at_idx: usize = 0;
    let found_at = false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '@') {
            if (found_at) return false; // Multiple '@'
            at_idx = i;
            found_at = true;
        }
    }
    if (!found_at || at_idx == 0 || at_idx == s.len - 1) return false;

    // Check for dot after '@'
    let dot_idx: usize = 0;
    let found_dot = false;
    i = at_idx + 1;
    while (i < s.len) : (i += 1) {
        if (s[i] == '.') {
            dot_idx = i;
            found_dot = true;
            break;
        }
    }
    if (!found_dot || dot_idx == at_idx + 1 || dot_idx == s.len - 1) return false;
    return true;
}

fn isValidUrl(s: []const u8): bool {
    if (s.len < 8) return false;
    if (std.mem.startsWith(u8, s, "http://") || std.mem.startsWith(u8, s, "https://") || std.mem.startsWith(u8, s, "ftp://")) {
        return true;
    }
    return false;
}

fn parseAsciiF64(s: []const u8): f64? {
    if (s.len == 0) return null;
    let val: f64 = 0.0;
    let sign: f64 = 1.0;
    let idx: usize = 0;
    if (s[0] == '-') {
        sign = -1.0;
        idx = 1;
    } else if (s[0] == '+') {
        idx = 1;
    }
    if (idx >= s.len) return null;

    let saw_digit = false;
    while (idx < s.len && s[idx] >= '0' && s[idx] <= '9') : (idx += 1) {
        val = val * 10.0 + @as(f64, @floatFromInt(s[idx] - '0'));
        saw_digit = true;
    }

    if (idx < s.len && s[idx] == '.') {
        idx += 1;
        let frac: f64 = 0.1;
        while (idx < s.len && s[idx] >= '0' && s[idx] <= '9') : (idx += 1) {
            val += @as(f64, @floatFromInt(s[idx] - '0')) * frac;
            frac *= 0.1;
            saw_digit = true;
        }
    }
    if (!saw_digit || idx < s.len) return null;
    return val * sign;
}

pub struct FormControl {
    node_id: NodeId = NULL_NODE,
    control_type: FormControlType = .text,
    name: []const u8 = "",
    value: []const u8 = "",
    default_value: []const u8 = "",
    checked: bool = false,
    default_checked: bool = false,
    disabled: bool = false,
    required: bool = false,
    read_only: bool = false,
    min_len: i32 = -1,
    max_len: i32 = -1,
    min_val: f64? = null,
    max_val: f64? = null,
    step: f64? = null,
    pattern: []const u8 = "",
    custom_error: []const u8 = "",
    validity: ValidityState = ValidityState{},
    validation_message: []const u8 = "",
    form_id: []const u8 = "",

    static fn init(): FormControl {
        return FormControl{};
    }

    fn setValue(*_, val: []const u8) {
        _.value = val;
        _.updateValidity();
    }

    fn setChecked(*_, chk: bool) {
        _.checked = chk;
        _.updateValidity();
    }

    fn setCustomValidity(*_, msg: []const u8) {
        _.custom_error = msg;
        _.updateValidity();
    }

    fn checkValidity(*_): bool {
        _.updateValidity();
        return _.validity.valid();
    }

    fn reset(*_) {
        _.value = _.default_value;
        _.checked = _.default_checked;
        _.custom_error = "";
        _.updateValidity();
    }

    fn updateValidity(*_) {
        let v = ValidityState{};
        let msg: []const u8 = "";

        // 1. Custom error check
        if (_.custom_error.len > 0) {
            v.custom_error = true;
            msg = _.custom_error;
        }

        // 2. Value missing check
        if (_.required && !_.disabled && !_.read_only) {
            if (_.control_type == .checkbox || _.control_type == .radio) {
                if (!_.checked) {
                    v.value_missing = true;
                    if (msg.len == 0) msg = "Please check this box if you want to proceed.";
                }
            } else {
                if (_.value.len == 0) {
                    v.value_missing = true;
                    if (msg.len == 0) msg = "Please fill out this field.";
                }
            }
        }

        // 3. Length checks (min_len / max_len)
        if (_.value.len > 0 && !_.read_only) {
            let vlen: i32 = @as(i32, @intCast(_.value.len));
            if (_.max_len >= 0 && vlen > _.max_len) {
                v.too_long = true;
                if (msg.len == 0) msg = "Please shorten this text.";
            }
            if (_.min_len >= 0 && vlen < _.min_len) {
                v.too_short = true;
                if (msg.len == 0) msg = "Please lengthen this text.";
            }
        }

        // 4. Type mismatch check
        if (_.value.len > 0) {
            if (_.control_type == .email) {
                if (!isValidEmail(_.value)) {
                    v.type_mismatch = true;
                    if (msg.len == 0) msg = "Please enter an email address.";
                }
            } else if (_.control_type == .url) {
                if (!isValidUrl(_.value)) {
                    v.type_mismatch = true;
                    if (msg.len == 0) msg = "Please enter a URL.";
                }
            } else if (_.control_type == .number) {
                if (num := parseAsciiF64(_.value)) {
                    if (mn := _.min_val) {
                        if (num < mn) {
                            v.range_underflow = true;
                            if (msg.len == 0) msg = "Value must be greater than or equal to minimum.";
                        }
                    }
                    if (mx := _.max_val) {
                        if (num > mx) {
                            v.range_overflow = true;
                            if (msg.len == 0) msg = "Value must be less than or equal to maximum.";
                        }
                    }
                } else {
                    v.bad_input = true;
                    if (msg.len == 0) msg = "Please enter a valid number.";
                }
            }
        }

        _.validity = v;
        _.validation_message = msg;
    }

    static fn parseType(t: []const u8): FormControlType {
        if (std.mem.eql(u8, t, "password")) return .password;
        if (std.mem.eql(u8, t, "email")) return .email;
        if (std.mem.eql(u8, t, "url")) return .url;
        if (std.mem.eql(u8, t, "number")) return .number;
        if (std.mem.eql(u8, t, "checkbox")) return .checkbox;
        if (std.mem.eql(u8, t, "radio")) return .radio;
        if (std.mem.eql(u8, t, "submit")) return .submit;
        if (std.mem.eql(u8, t, "reset")) return .reset;
        if (std.mem.eql(u8, t, "button")) return .button;
        if (std.mem.eql(u8, t, "hidden")) return .hidden;
        if (std.mem.eql(u8, t, "file")) return .file;
        if (std.mem.eql(u8, t, "textarea")) return .textarea;
        if (std.mem.eql(u8, t, "select")) return .select;
        if (std.mem.eql(u8, t, "search")) return .search;
        if (std.mem.eql(u8, t, "tel")) return .tel;
        if (std.mem.eql(u8, t, "range")) return .range;
        if (std.mem.eql(u8, t, "date")) return .date;
        if (std.mem.eql(u8, t, "color")) return .color;
        return .text;
    }
}

pub struct HTMLFormElement {
    node_id: NodeId = NULL_NODE,
    action: []const u8 = "",
    method: []const u8 = "GET",
    enctype: []const u8 = "application/x-www-form-urlencoded",
    name: []const u8 = "",
    no_validate: bool = false,
    target: []const u8 = "",
    controls: [MAX_FORM_CONTROLS]FormControl = [FormControl{}; 64],
    control_count: usize = 0,

    static fn init(): HTMLFormElement {
        return HTMLFormElement{};
    }

    fn addControl(*_, ctrl: FormControl) {
        if (_.control_count < MAX_FORM_CONTROLS) {
            _.controls[_.control_count] = ctrl;
            _.control_count += 1;
        }
    }

    fn getControl(*_, idx: usize): (*FormControl)? {
        if (idx < _.control_count) {
            return &_.controls[idx];
        }
        return null;
    }

    fn findControlByName(*_, cname: []const u8): (*FormControl)? {
        let i: usize = 0;
        while (i < _.control_count) : (i += 1) {
            if (std.mem.eql(u8, _.controls[i].name, cname)) {
                return &_.controls[i];
            }
        }
        return null;
    }

    fn checkValidity(*_): bool {
        if (_.no_validate) return true;
        let all_valid = true;
        let i: usize = 0;
        while (i < _.control_count) : (i += 1) {
            if (!_.controls[i].checkValidity()) {
                all_valid = false;
            }
        }
        return all_valid;
    }

    fn reset(*_) {
        let i: usize = 0;
        while (i < _.control_count) : (i += 1) {
            _.controls[i].reset();
        }
    }
}
