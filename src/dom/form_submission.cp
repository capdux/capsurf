// Migrated from vendor/kitesurf/packages/dom-shim/src/element/forms/submission.ts
// Form submission payload serializer: application/x-www-form-urlencoded and multipart/form-data.

import { FormControl, HTMLFormElement, FormControlType, MAX_FORM_CONTROLS } from "./forms.cp";

pub struct SubmissionEntry {
    name: []const u8 = "",
    value: []const u8 = "",
    filename: []const u8 = "",
    is_file: bool = false,
}

pub struct FormSubmissionPayload {
    method: []const u8 = "GET",
    action: []const u8 = "",
    content_type: []const u8 = "",
    body: []const u8 = "",
}

fn isUnreservedChar(c: u8): bool {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        return true;
    }
    return c == '-' || c == '_' || c == '.' || c == '~';
}

const HEX_CHARS = "0123456789ABCDEF";

fn appendPercentEncoded(out_buf: []u8, out_len: *usize, input: []const u8, replace_space_with_plus: bool) {
    let i: usize = 0;
    while (i < input.len) : (i += 1) {
        let c = input[i];
        if (c == ' ' && replace_space_with_plus) {
            if (out_len.* < out_buf.len) {
                out_buf[out_len.*] = '+';
                out_len.* += 1;
            }
        } else if (isUnreservedChar(c)) {
            if (out_len.* < out_buf.len) {
                out_buf[out_len.*] = c;
                out_len.* += 1;
            }
        } else {
            if (out_len.* + 3 <= out_buf.len) {
                out_buf[out_len.*] = '%';
                out_buf[out_len.* + 1] = HEX_CHARS[(c >> 4) & 0x0F];
                out_buf[out_len.* + 2] = HEX_CHARS[c & 0x0F];
                out_len.* += 3;
            }
        }
    }
}

pub fn collectFormEntries(form: *const HTMLFormElement, submitter_name: []const u8, out_entries: []SubmissionEntry): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < form.control_count && count < out_entries.len) : (i += 1) {
        let ctrl = form.controls[i];
        if (ctrl.disabled) continue;
        if (ctrl.name.len == 0) continue;

        switch (ctrl.control_type) {
            .submit, .button => {
                if (submitter_name.len > 0 && std.mem.eql(u8, ctrl.name, submitter_name)) {
                    out_entries[count] = SubmissionEntry{
                        name: ctrl.name,
                        value: ctrl.value,
                    };
                    count += 1;
                }
            },
            .reset => {
                // Reset buttons are not submitted
            },
            .checkbox, .radio => {
                if (ctrl.checked) {
                    out_entries[count] = SubmissionEntry{
                        name: ctrl.name,
                        value: if (ctrl.value.len > 0) ctrl.value else "on",
                    };
                    count += 1;
                }
            },
            .file => {
                out_entries[count] = SubmissionEntry{
                    name: ctrl.name,
                    value: ctrl.value,
                    filename: if (ctrl.value.len > 0) ctrl.value else "",
                    is_file: true,
                };
                count += 1;
            },
            else => {
                out_entries[count] = SubmissionEntry{
                    name: ctrl.name,
                    value: ctrl.value,
                };
                count += 1;
            },
        }
    }
    return count;
}

pub fn encodeUrlEncoded(entries: []const SubmissionEntry, out_buf: []u8): usize {
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < entries.len) : (i += 1) {
        if (i > 0 && out_len < out_buf.len) {
            out_buf[out_len] = '&';
            out_len += 1;
        }
        appendPercentEncoded(out_buf, &out_len, entries[i].name, true);
        if (out_len < out_buf.len) {
            out_buf[out_len] = '=';
            out_len += 1;
        }
        appendPercentEncoded(out_buf, &out_len, entries[i].value, true);
    }
    return out_len;
}

pub fn encodeMultipart(entries: []const SubmissionEntry, boundary: []const u8, out_buf: []u8): usize {
    let out_len: usize = 0;
    let i: usize = 0;

    while (i < entries.len) : (i += 1) {
        let entry = entries[i];
        // --{boundary}\r\n
        appendString(out_buf, &out_len, "--");
        appendString(out_buf, &out_len, boundary);
        appendString(out_buf, &out_len, "\r\n");

        // Content-Disposition: form-data; name="{name}"
        appendString(out_buf, &out_len, "Content-Disposition: form-data; name=\"");
        appendString(out_buf, &out_len, entry.name);
        appendString(out_buf, &out_len, "\"");

        if (entry.is_file) {
            appendString(out_buf, &out_len, "; filename=\"");
            appendString(out_buf, &out_len, entry.filename);
            appendString(out_buf, &out_len, "\"\r\nContent-Type: application/octet-stream");
        }
        appendString(out_buf, &out_len, "\r\n\r\n");

        // Value
        appendString(out_buf, &out_len, entry.value);
        appendString(out_buf, &out_len, "\r\n");
    }

    // Terminating boundary: --{boundary}--\r\n
    appendString(out_buf, &out_len, "--");
    appendString(out_buf, &out_len, boundary);
    appendString(out_buf, &out_len, "--\r\n");

    return out_len;
}

fn appendString(out_buf: []u8, out_len: *usize, s: []const u8) {
    let i: usize = 0;
    while (i < s.len && out_len.* < out_buf.len) : (i += 1) {
        out_buf[out_len.*] = s[i];
        out_len.* += 1;
    }
}

pub fn serializeSubmission(
    form: *const HTMLFormElement,
    submitter_name: []const u8,
    boundary: []const u8,
    out_buf: []u8,
): FormSubmissionPayload {
    let entries: [64]SubmissionEntry = [SubmissionEntry{}; 64];
    let entry_count = collectFormEntries(form, submitter_name, entries[0..]);

    let is_multipart = std.mem.indexOf(u8, form.enctype, "multipart/form-data") != null;
    let written: usize = 0;
    let ctype: []const u8 = form.enctype;

    if (is_multipart) {
        written = encodeMultipart(entries[0..entry_count], boundary, out_buf);
    } else {
        written = encodeUrlEncoded(entries[0..entry_count], out_buf);
    }

    return FormSubmissionPayload{
        method: form.method,
        action: form.action,
        content_type: ctype,
        body: out_buf[0..written],
    };
}
