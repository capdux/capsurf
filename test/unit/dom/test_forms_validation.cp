import { NodeId, NULL_NODE } from "../../../src/dom/dom.cp";
import {
    FormControl,
    HTMLFormElement,
    FormControlType,
    ValidityState,
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
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. Test FormControl Validation State Machine
    {
        // 1.1 Text input with required constraint
        let input: FormControl = FormControl.init();
        input.name = "username";
        input.control_type = .text;
        input.required = true;
        input.default_value = "guest";

        assert(!input.checkValidity(), "empty required input is invalid");
        assert(input.validity.value_missing, "value_missing flag is true");

        input.setValue("alice");
        assert(input.checkValidity(), "filled required input is valid");
        assert(!input.validity.value_missing, "value_missing flag cleared");

        // 1.2 Length constraints (min_len / max_len)
        input.max_len = 8;
        input.min_len = 3;
        input.setValue("a");
        assert(!input.checkValidity(), "too short input is invalid");
        assert(input.validity.too_short, "too_short flag set");

        input.setValue("alice_super_long_username");
        assert(!input.checkValidity(), "too long input is invalid");
        assert(input.validity.too_long, "too_long flag set");

        input.setValue("alicesmith");
        input.max_len = -1;
        input.min_len = -1;
        assert(input.checkValidity(), "valid length");

        // 1.3 Email validation
        let email_input: FormControl = FormControl.init();
        email_input.name = "email";
        email_input.control_type = .email;
        email_input.setValue("not_an_email");
        assert(!email_input.checkValidity(), "invalid email syntax");
        assert(email_input.validity.type_mismatch, "type_mismatch flag set for email");

        email_input.setValue("user@domain.com");
        assert(email_input.checkValidity(), "valid email passes");
        assert(!email_input.validity.type_mismatch, "type_mismatch flag cleared");

        // 1.4 Number range constraints
        let num_input: FormControl = FormControl.init();
        num_input.name = "age";
        num_input.control_type = .number;
        num_input.min_val = 18.0;
        num_input.max_val = 65.0;

        num_input.setValue("10");
        assert(!num_input.checkValidity(), "underflow number");
        assert(num_input.validity.range_underflow, "range_underflow flag set");

        num_input.setValue("70");
        assert(!num_input.checkValidity(), "overflow number");
        assert(num_input.validity.range_overflow, "range_overflow flag set");

        num_input.setValue("25");
        assert(num_input.checkValidity(), "valid range number");
        assert(!num_input.validity.range_underflow && !num_input.validity.range_overflow, "range flags cleared");

        // 1.5 Custom validity
        num_input.setCustomValidity("Custom failure message");
        assert(!num_input.checkValidity(), "custom validity triggers invalid");
        assert(num_input.validity.custom_error, "custom_error flag set");
        assert(std.mem.eql(u8, num_input.validation_message, "Custom failure message"), "validation message matches");

        num_input.setCustomValidity("");
        assert(num_input.checkValidity(), "cleared custom validity restores valid");

        // 1.6 Form control reset
        input.setValue("modified");
        input.reset();
        assert(std.mem.eql(u8, input.value, "guest"), "reset restored default value");
    }

    // 2. Test HTMLFormElement Management & Validation
    {
        let form: HTMLFormElement = HTMLFormElement.init();
        form.action = "/api/login";
        form.method = "POST";

        let user_ctrl: FormControl = FormControl.init();
        user_ctrl.name = "user";
        user_ctrl.control_type = .text;
        user_ctrl.required = true;
        form.addControl(user_ctrl);

        let pass_ctrl: FormControl = FormControl.init();
        pass_ctrl.name = "credential";
        pass_ctrl.control_type = .password;
        pass_ctrl.required = true;
        form.addControl(pass_ctrl);

        let chk_ctrl: FormControl = FormControl.init();
        chk_ctrl.name = "remember";
        chk_ctrl.control_type = .checkbox;
        chk_ctrl.checked = true;
        form.addControl(chk_ctrl);

        let btn_ctrl: FormControl = FormControl.init();
        btn_ctrl.name = "submit_action";
        btn_ctrl.value = "sign_in";
        btn_ctrl.control_type = .submit;
        form.addControl(btn_ctrl);

        assert(form.control_count == 4, "form contains 4 controls");
        assert(form.findControlByName("user") != null, "found user control");
        assert(form.findControlByName("non_existent") == null, "non existent control is null");

        // Both required fields are currently empty
        assert(!form.checkValidity(), "form with empty required controls is invalid");

        // Fill required fields
        if (c := form.findControlByName("user")) {
            c.setValue("bob");
        }
        if (c := form.findControlByName("credential")) {
            c.setValue("secret123");
        }

        assert(form.checkValidity(), "form is now valid");
    }

    // 3. Test Form Submission Serialization (UrlEncoded & Multipart)
    {
        let form: HTMLFormElement = HTMLFormElement.init();
        form.action = "/submit";
        form.method = "POST";
        form.enctype = "application/x-www-form-urlencoded";

        let q_ctrl: FormControl = FormControl.init();
        q_ctrl.name = "query";
        q_ctrl.value = "hello world & more";
        form.addControl(q_ctrl);

        let tag_ctrl: FormControl = FormControl.init();
        tag_ctrl.name = "category";
        tag_ctrl.value = "tech";
        form.addControl(tag_ctrl);

        let disabled_ctrl: FormControl = FormControl.init();
        disabled_ctrl.name = "ignored";
        disabled_ctrl.value = "val";
        disabled_ctrl.disabled = true;
        form.addControl(disabled_ctrl);

        let submit_btn: FormControl = FormControl.init();
        submit_btn.name = "action";
        submit_btn.value = "search";
        submit_btn.control_type = .submit;
        form.addControl(submit_btn);

        // 3.1 UrlEncoded submission
        let buf: [2048]u8 = [0; 2048];
        let payload = serializeSubmission(&form, "action", "----WebKitFormBoundary", &buf);

        assert(std.mem.eql(u8, payload.method, "POST"), "payload method POST");
        assert(std.mem.eql(u8, payload.action, "/submit"), "payload action matches");
        assert(std.mem.indexOf(u8, payload.body, "query=hello+world+%26+more") != null, "encoded space and ampersand");
        assert(std.mem.indexOf(u8, payload.body, "category=tech") != null, "category present");
        assert(std.mem.indexOf(u8, payload.body, "ignored") == null, "disabled control omitted");
        assert(std.mem.indexOf(u8, payload.body, "action=search") != null, "submit button value included");

        // 3.2 Multipart/form-data submission
        form.enctype = "multipart/form-data";
        let file_ctrl: FormControl = FormControl.init();
        file_ctrl.name = "avatar";
        file_ctrl.value = "photo.png";
        file_ctrl.control_type = .file;
        form.addControl(file_ctrl);

        let multipart_buf: [4096]u8 = [0; 4096];
        let boundary = "----TestBoundary123456";
        let multipart_payload = serializeSubmission(&form, "action", boundary, &multipart_buf);

        assert(std.mem.indexOf(u8, multipart_payload.body, "----TestBoundary123456") != null, "boundary present");
        assert(std.mem.indexOf(u8, multipart_payload.body, "Content-Disposition: form-data; name=\"query\"") != null, "name query disposition");
        assert(std.mem.indexOf(u8, multipart_payload.body, "filename=\"photo.png\"") != null, "filename in file control");
        assert(std.mem.indexOf(u8, multipart_payload.body, "Content-Type: application/octet-stream") != null, "content-type for file");
        assert(std.mem.indexOf(u8, multipart_payload.body, "----TestBoundary123456--\r\n") != null, "terminating boundary present");
    }

    std.debug.print("test_forms_validation passed all assertions!\n", .{});
}
