import { NULL_NODE } from "../../../src/dom/dom.cp";
import {
    FormControl,
    HTMLFormElement,
    FormControlType,
    MAX_FORM_CONTROLS,
} from "../../../src/dom/forms.cp";
import {
    SubmissionEntry,
    FormSubmissionPayload,
    collectFormEntries,
    encodeUrlEncoded,
    encodeMultipart,
    serializeSubmission,
} from "../../../src/dom/form_submission.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test_forms_and_validation_edge assertion failed");
    }
}

fn testDisabledFilteringAndValidation() {
    let form = HTMLFormElement.init();
    form.action = "/submit";
    form.method = "POST";

    // Disabled required text field: must pass validation and be excluded from submission
    let dis_text = FormControl.init();
    dis_text.name = "dis_user";
    dis_text.control_type = .text;
    dis_text.required = true;
    dis_text.disabled = true;
    dis_text.value = "";
    assert(dis_text.checkValidity(), "disabled required text control is valid");
    form.addControl(dis_text);

    // Disabled checked checkbox: must be excluded from submission
    let dis_chk = FormControl.init();
    dis_chk.name = "dis_agree";
    dis_chk.control_type = .checkbox;
    dis_chk.checked = true;
    dis_chk.disabled = true;
    dis_chk.value = "yes";
    form.addControl(dis_chk);

    // Disabled checked radio: must be excluded from submission
    let dis_rad = FormControl.init();
    dis_rad.name = "dis_plan";
    dis_rad.control_type = .radio;
    dis_rad.checked = true;
    dis_rad.disabled = true;
    dis_rad.value = "pro";
    form.addControl(dis_rad);

    // Disabled submitter matching submitter_name: must still be excluded
    let dis_sub = FormControl.init();
    dis_sub.name = "go";
    dis_sub.control_type = .submit;
    dis_sub.disabled = true;
    dis_sub.value = "Send";
    form.addControl(dis_sub);

    // Disabled file input: must be excluded
    let dis_file = FormControl.init();
    dis_file.name = "dis_doc";
    dis_file.control_type = .file;
    dis_file.disabled = true;
    dis_file.value = "secret.pdf";
    form.addControl(dis_file);

    // Active hidden field
    let active = FormControl.init();
    active.name = "token";
    active.control_type = .hidden;
    active.value = "ok123";
    form.addControl(active);

    assert(form.checkValidity(), "form with only disabled required controls is valid");

    let entries: [8]SubmissionEntry = [SubmissionEntry{}; 8];
    let count = collectFormEntries(&form, "go", entries[0..]);
    assert(count == 1, "only 1 active non-disabled entry collected");
    assert(std.mem.eql(u8, entries[0].name, "token"), "active entry name matches");
    assert(std.mem.eql(u8, entries[0].value, "ok123"), "active entry value matches");
}

fn testMultiSelectAndCheckboxRadioEdge() {
    let form = HTMLFormElement.init();
    form.action = "/prefs";
    form.method = "POST";

    // Multi-select represented by multiple selected entries with the same name
    let sel_a = FormControl.init();
    sel_a.name = "tags";
    sel_a.control_type = .select;
    sel_a.value = "cp";
    form.addControl(sel_a);

    let sel_b = FormControl.init();
    sel_b.name = "tags";
    sel_b.control_type = .select;
    sel_b.value = "browser";
    form.addControl(sel_b);

    let sel_c = FormControl.init();
    sel_c.name = "tags";
    sel_c.control_type = .select;
    sel_c.value = "wasm";
    sel_c.disabled = true;
    form.addControl(sel_c);

    // Checkboxes: unchecked, checked with empty value -> "on", checked with explicit value
    let cb_off = FormControl.init();
    cb_off.name = "opt";
    cb_off.control_type = .checkbox;
    cb_off.checked = false;
    cb_off.value = "skip";
    form.addControl(cb_off);

    let cb_default = FormControl.init();
    cb_default.name = "opt";
    cb_default.control_type = .checkbox;
    cb_default.checked = true;
    cb_default.value = "";
    form.addControl(cb_default);

    let cb_custom = FormControl.init();
    cb_custom.name = "opt";
    cb_custom.control_type = .checkbox;
    cb_custom.checked = true;
    cb_custom.value = "dark";
    form.addControl(cb_custom);

    // Radio group: required validation + checked selection
    let rad_1 = FormControl.init();
    rad_1.name = "tier";
    rad_1.control_type = .radio;
    rad_1.required = true;
    rad_1.checked = false;
    rad_1.value = "basic";
    assert(!rad_1.checkValidity(), "unchecked required radio fails validity");
    assert(rad_1.validity.value_missing, "unchecked required radio sets value_missing");
    rad_1.required = false;
    form.addControl(rad_1);

    let rad_2 = FormControl.init();
    rad_2.name = "tier";
    rad_2.control_type = .radio;
    rad_2.required = true;
    rad_2.checked = true;
    rad_2.value = "";
    assert(rad_2.checkValidity(), "checked required radio passes validity");
    form.addControl(rad_2);

    let buf: [512]u8 = [0; 512];
    let payload = serializeSubmission(&form, "", "B", &buf);
    let expected = "tags=cp&tags=browser&opt=on&opt=dark&tier=on";
    assert(std.mem.eql(u8, payload.body, expected), "multi-select, checkbox, and radio serialization matches");

    // Also verify multipart preserves duplicate multi-select keys
    form.enctype = "multipart/form-data";
    let mp_buf: [1024]u8 = [0; 1024];
    let mp_payload = serializeSubmission(&form, "", "B1", &mp_buf);
    assert(std.mem.indexOf(u8, mp_payload.body, "name=\"tags\"\r\n\r\ncp\r\n") != null, "multipart has first tags entry");
    assert(std.mem.indexOf(u8, mp_payload.body, "name=\"tags\"\r\n\r\nbrowser\r\n") != null, "multipart has second tags entry");
    assert(std.mem.indexOf(u8, mp_payload.body, "wasm") == null, "disabled select option excluded in multipart");
}

fn testSpecialCharsUrlAndMultipartEncoding() {
    // 1. URL-encoded special characters: spaces, punctuation, unreserved, UTF-8 bytes
    let url_entries: [4]SubmissionEntry = [
        SubmissionEntry{ name: "k 1", value: "hello world!" },
        SubmissionEntry{ name: "sym", value: "@#$%^&*()+=[]{};:'\",<>/?\\|" },
        SubmissionEntry{ name: "safe", value: "AZaz09-_.~" },
        SubmissionEntry{ name: "utf8", value: "中" },
    ];

    let url_buf: [512]u8 = [0; 512];
    let url_len = encodeUrlEncoded(url_entries[0..], &url_buf);
    let url_out = url_buf[0..url_len];
    let expected_url =
        "k+1=hello+world%21" ++
        "&sym=%40%23%24%25%5E%26%2A%28%29%2B%3D%5B%5D%7B%7D%3B%3A%27%22%2C%3C%3E%2F%3F%5C%7C" ++
        "&safe=AZaz09-_.~" ++
        "&utf8=%E4%B8%AD";
    assert(std.mem.eql(u8, url_out, expected_url), "url-encoding handles spaces, reserved ASCII, unreserved, and UTF-8");

    // Small buffer truncation safety for urlencoded (must not overflow)
    let tiny_buf: [8]u8 = [0; 8];
    let tiny_len = encodeUrlEncoded(url_entries[0..1], &tiny_buf);
    assert(tiny_len <= tiny_buf.len, "tiny url buffer never overflows");

    // 2. Multipart encoding with special characters, empty values, and file entries
    let mp_entries: [2]SubmissionEntry = [
        SubmissionEntry{ name: "note", value: "line1\r\nline2 & 100%", is_file: false },
        SubmissionEntry{ name: "upload", value: "DATA", filename: "my file (1).txt", is_file: true },
    ];
    let mp_buf: [512]u8 = [0; 512];
    let mp_len = encodeMultipart(mp_entries[0..], "Bound!@#", &mp_buf);
    let mp_out = mp_buf[0..mp_len];
    let expected_mp =
        "--Bound!@#\r\n" ++
        "Content-Disposition: form-data; name=\"note\"\r\n\r\n" ++
        "line1\r\nline2 & 100%\r\n" ++
        "--Bound!@#\r\n" ++
        "Content-Disposition: form-data; name=\"upload\"; filename=\"my file (1).txt\"\r\n" ++
        "Content-Type: application/octet-stream\r\n\r\n" ++
        "DATA\r\n" ++
        "--Bound!@#--\r\n";
    assert(std.mem.eql(u8, mp_out, expected_mp), "multipart preserves special chars in values and filenames");

    // Small buffer truncation safety for multipart
    let tiny_mp: [12]u8 = [0; 12];
    let tiny_mp_len = encodeMultipart(mp_entries[0..], "B", &tiny_mp);
    assert(tiny_mp_len <= tiny_mp.len, "tiny multipart buffer never overflows");
}

fn testEmptyFormEdge() {
    let empty_form = HTMLFormElement.init();
    empty_form.action = "/empty";

    assert(empty_form.control_count == 0, "empty form has 0 controls");
    assert(empty_form.checkValidity(), "empty form passes checkValidity");

    let entries: [4]SubmissionEntry = [SubmissionEntry{}; 4];
    let collected = collectFormEntries(&empty_form, "submit", entries[0..]);
    assert(collected == 0, "empty form collects 0 entries");

    // Empty form urlencoded serialization
    let url_buf: [128]u8 = [0; 128];
    let url_payload = serializeSubmission(&empty_form, "", "EmptyB", &url_buf);
    assert(url_payload.body.len == 0, "empty urlencoded form body is empty");
    assert(std.mem.eql(u8, url_payload.method, "GET"), "empty form default method is GET");

    // Empty form multipart serialization must still emit closing boundary
    empty_form.method = "POST";
    empty_form.enctype = "multipart/form-data";
    let mp_buf: [128]u8 = [0; 128];
    let mp_payload = serializeSubmission(&empty_form, "", "EmptyB", &mp_buf);
    assert(std.mem.eql(u8, mp_payload.body, "--EmptyB--\r\n"), "empty multipart form emits terminating boundary");
}

fn testNullNodeAndControlBoundsEdge() {
    let form = HTMLFormElement.init();
    assert(form.node_id == NULL_NODE, "default HTMLFormElement node_id is NULL_NODE");
    assert(form.getControl(0) == null, "getControl(0) on empty form returns null");
    assert(form.findControlByName("missing") == null, "findControlByName on empty form returns null");

    let ctrl = FormControl.init();
    assert(ctrl.node_id == NULL_NODE, "default FormControl node_id is NULL_NODE");

    // Read-only required control must bypass value_missing and length checks
    ctrl.name = "ro_field";
    ctrl.required = true;
    ctrl.read_only = true;
    ctrl.min_len = 5;
    ctrl.max_len = 2;
    ctrl.value = "";
    assert(ctrl.checkValidity(), "read-only control bypasses required and length checks");

    // Invalid number bad_input boundary
    let num_ctrl = FormControl.init();
    num_ctrl.name = "qty";
    num_ctrl.control_type = .number;
    num_ctrl.setValue("abc");
    assert(!num_ctrl.checkValidity(), "non-numeric string fails number validation");
    assert(num_ctrl.validity.bad_input, "bad_input flag set for non-numeric number input");

    // URL type_mismatch boundary
    let url_ctrl = FormControl.init();
    url_ctrl.name = "site";
    url_ctrl.control_type = .url;
    url_ctrl.setValue("javascript:void(0)");
    assert(!url_ctrl.checkValidity(), "non-http/https/ftp url fails validation");
    assert(url_ctrl.validity.type_mismatch, "type_mismatch set for unsupported scheme");

    url_ctrl.setValue("http://");
    assert(!url_ctrl.checkValidity(), "too-short http url fails validation");

    url_ctrl.setValue("https://capsurf.dev");
    assert(url_ctrl.checkValidity(), "https url passes validation");

    // Form no_validate overrides invalid child controls
    form.no_validate = true;
    form.addControl(num_ctrl);
    assert(form.checkValidity(), "no_validate form returns true despite invalid child");

    // getControl & findControlByName with valid NULL_NODE-backed control
    if (found := form.findControlByName("qty")) {
        assert(found.node_id == NULL_NODE, "found control retains NULL_NODE");
    } else {
        assert(false, "findControlByName should locate qty");
    }
    assert(form.getControl(1) == null, "out-of-bounds getControl returns null");

    // Out-of-entries capacity boundary: collectFormEntries stops at out_entries.len
    let form_cap = HTMLFormElement.init();
    let idx: usize = 0;
    while (idx < 5) : (idx += 1) {
        let item = FormControl.init();
        item.name = "k";
        item.value = "v";
        form_cap.addControl(item);
    }
    let small_entries: [2]SubmissionEntry = [SubmissionEntry{}; 2];
    let capped = collectFormEntries(&form_cap, "", small_entries[0..]);
    assert(capped == 2, "collectFormEntries respects out_entries.len cap");
    assert(form_cap.control_count <= MAX_FORM_CONTROLS, "control_count within MAX_FORM_CONTROLS");

    // Empty output slice in collectFormEntries
    let zero_entries = collectFormEntries(&form_cap, "", small_entries[0..0]);
    assert(zero_entries == 0, "collectFormEntries with 0-len slice returns 0");
}

pub fn main() {
    testDisabledFilteringAndValidation();
    testMultiSelectAndCheckboxRadioEdge();
    testSpecialCharsUrlAndMultipartEncoding();
    testEmptyFormEdge();
    testNullNodeAndControlBoundsEdge();
    std.debug.print("test_forms_and_validation_edge OK\n", .{});
}
