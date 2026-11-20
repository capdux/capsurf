import {
    FormControl,
    HTMLFormElement,
    FormControlType,
} from "../../src/dom/forms.cp";
import {
    SubmissionEntry,
    FormSubmissionPayload,
    collectFormEntries,
    encodeUrlEncoded,
    encodeMultipart,
    serializeSubmission,
} from "../../src/dom/form_submission.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("test_form_submission assertion failed");
    }
}

fn testGetVsPostResolution() {
    // 1. Default form properties (GET + default enctype)
    let get_form = HTMLFormElement.init();
    get_form.action = "/search";

    let q_ctrl = FormControl.init();
    q_ctrl.name = "q";
    q_ctrl.control_type = .search;
    q_ctrl.value = "cp browser";
    get_form.addControl(q_ctrl);

    let get_buf: [512]u8 = [0; 512];
    let get_payload = serializeSubmission(&get_form, "", "----BoundaryGet", &get_buf);
    assert(std.mem.eql(u8, get_payload.method, "GET"), "default method is GET");
    assert(std.mem.eql(u8, get_payload.action, "/search"), "GET action preserved");
    assert(std.mem.eql(u8, get_payload.content_type, "application/x-www-form-urlencoded"), "default enctype is urlencoded");
    assert(std.mem.eql(u8, get_payload.body, "q=cp+browser"), "GET urlencoded body");

    // 2. Explicit POST form with multipart/form-data enctype and submitter button
    let post_form = HTMLFormElement.init();
    post_form.action = "/upload";
    post_form.method = "POST";
    post_form.enctype = "multipart/form-data";

    let title_ctrl = FormControl.init();
    title_ctrl.name = "title";
    title_ctrl.value = "report";
    post_form.addControl(title_ctrl);

    let submit_a = FormControl.init();
    submit_a.name = "intent";
    submit_a.value = "save";
    submit_a.control_type = .submit;
    post_form.addControl(submit_a);

    let submit_b = FormControl.init();
    submit_b.name = "publish";
    submit_b.value = "now";
    submit_b.control_type = .submit;
    post_form.addControl(submit_b);

    let post_buf: [1024]u8 = [0; 1024];
    let post_payload = serializeSubmission(&post_form, "publish", "----BoundaryPost", &post_buf);
    assert(std.mem.eql(u8, post_payload.method, "POST"), "POST method preserved");
    assert(std.mem.eql(u8, post_payload.action, "/upload"), "POST action preserved");
    assert(std.mem.eql(u8, post_payload.content_type, "multipart/form-data"), "POST multipart enctype preserved");
    assert(std.mem.indexOf(u8, post_payload.body, "name=\"publish\"\r\n\r\nnow\r\n") != null, "matching submitter included");
    assert(std.mem.indexOf(u8, post_payload.body, "name=\"intent\"") == null, "non-matching submitter excluded");
}

fn testFormEntryConstruction() {
    let form = HTMLFormElement.init();

    // Active text input
    let user = FormControl.init();
    user.name = "user";
    user.control_type = .text;
    user.value = "alice";
    form.addControl(user);

    // Disabled control must be filtered out
    let disabled_ctrl = FormControl.init();
    disabled_ctrl.name = "secret";
    disabled_ctrl.value = "hidden_val";
    disabled_ctrl.disabled = true;
    form.addControl(disabled_ctrl);

    // Unnamed control must be filtered out
    let unnamed_ctrl = FormControl.init();
    unnamed_ctrl.name = "";
    unnamed_ctrl.value = "orphan";
    form.addControl(unnamed_ctrl);

    // Unchecked checkbox must be filtered out
    let unchecked_box = FormControl.init();
    unchecked_box.name = "newsletter";
    unchecked_box.control_type = .checkbox;
    unchecked_box.checked = false;
    unchecked_box.value = "yes";
    form.addControl(unchecked_box);

    // Checked checkbox with empty value defaults to "on"
    let checked_default = FormControl.init();
    checked_default.name = "agree";
    checked_default.control_type = .checkbox;
    checked_default.checked = true;
    checked_default.value = "";
    form.addControl(checked_default);

    // Radio group: one unchecked, one checked
    let radio_off = FormControl.init();
    radio_off.name = "plan";
    radio_off.control_type = .radio;
    radio_off.checked = false;
    radio_off.value = "free";
    form.addControl(radio_off);

    let radio_on = FormControl.init();
    radio_on.name = "plan";
    radio_on.control_type = .radio;
    radio_on.checked = true;
    radio_on.value = "pro";
    form.addControl(radio_on);

    // Select control
    let country = FormControl.init();
    country.name = "country";
    country.control_type = .select;
    country.value = "au";
    form.addControl(country);

    // Reset button must never be submitted
    let reset_btn = FormControl.init();
    reset_btn.name = "reset_all";
    reset_btn.control_type = .reset;
    reset_btn.value = "Reset";
    form.addControl(reset_btn);

    // Non-matching button must be excluded when submitter_name differs
    let other_btn = FormControl.init();
    other_btn.name = "preview";
    other_btn.control_type = .button;
    other_btn.value = "1";
    form.addControl(other_btn);

    let entries: [16]SubmissionEntry = [SubmissionEntry{}; 16];
    let count = collectFormEntries(&form, "go", entries[0..]);

    assert(count == 4, "expected 4 active entries");
    assert(std.mem.eql(u8, entries[0].name, "user") && std.mem.eql(u8, entries[0].value, "alice"), "entry 0 is user=alice");
    assert(std.mem.eql(u8, entries[1].name, "agree") && std.mem.eql(u8, entries[1].value, "on"), "entry 1 is agree=on");
    assert(std.mem.eql(u8, entries[2].name, "plan") && std.mem.eql(u8, entries[2].value, "pro"), "entry 2 is plan=pro");
    assert(std.mem.eql(u8, entries[3].name, "country") && std.mem.eql(u8, entries[3].value, "au"), "entry 3 is country=au");
}

fn testUrlEncodedSerialization() {
    let entries: [3]SubmissionEntry = [
        SubmissionEntry{ name: "full name", value: "Ada Lovelace" },
        SubmissionEntry{ name: "expr", value: "a+b=c&d/e%f" },
        SubmissionEntry{ name: "unreserved_~1", value: "A-Z_0.9~" },
    ];

    let buf: [512]u8 = [0; 512];
    let n = encodeUrlEncoded(entries[0..], &buf);
    let encoded = buf[0..n];

    let expected = "full+name=Ada+Lovelace&expr=a%2Bb%3Dc%26d%2Fe%25f&unreserved_~1=A-Z_0.9~";
    assert(std.mem.eql(u8, encoded, expected), "urlencoded formatting with spaces as + and percent-encoding");

    // Empty entry list produces empty output
    let empty_n = encodeUrlEncoded(entries[0..0], &buf);
    assert(empty_n == 0, "empty entries produce 0 bytes");
}

fn testMultipartSerialization() {
    let entries: [3]SubmissionEntry = [
        SubmissionEntry{ name: "username", value: "alice", is_file: false },
        SubmissionEntry{ name: "avatar", value: "PNGDATA", filename: "avatar.png", is_file: true },
        SubmissionEntry{ name: "empty_file", value: "", filename: "", is_file: true },
    ];

    let buf: [1024]u8 = [0; 1024];
    let boundary = "----CapSurfBoundary42";
    let n = encodeMultipart(entries[0..], boundary, &buf);
    let body = buf[0..n];

    let expected =
        "------CapSurfBoundary42\r\n" ++
        "Content-Disposition: form-data; name=\"username\"\r\n\r\n" ++
        "alice\r\n" ++
        "------CapSurfBoundary42\r\n" ++
        "Content-Disposition: form-data; name=\"avatar\"; filename=\"avatar.png\"\r\n" ++
        "Content-Type: application/octet-stream\r\n\r\n" ++
        "PNGDATA\r\n" ++
        "------CapSurfBoundary42\r\n" ++
        "Content-Disposition: form-data; name=\"empty_file\"; filename=\"\"\r\n" ++
        "Content-Type: application/octet-stream\r\n\r\n" ++
        "\r\n" ++
        "------CapSurfBoundary42--\r\n";

    assert(std.mem.eql(u8, body, expected), "multipart/form-data payload and boundaries match exact RFC format");
}

pub fn main() {
    testGetVsPostResolution();
    testFormEntryConstruction();
    testUrlEncodedSerialization();
    testMultipartSerialization();
    std.debug.print("test_form_submission OK\n", .{});
}
