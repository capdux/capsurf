// test/net/test_mock_fixtures_edge.cp
// Edge-case unit tests for MockFixtures: oopif-child-solid, oopif-host-many, module-registry-*, and 404/null boundaries.

import { MockFixtures } from "../../../src/net/mock_fixtures.cp";

fn assertCheck(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

fn countOccurrences(haystack: []const u8, needle: []const u8): usize {
    if (needle.len == 0 || haystack.len < needle.len) return 0;
    let count: usize = 0;
    let pos: usize = 0;
    while (pos <= haystack.len - needle.len) {
        if (found := std.mem.indexOfPos(u8, haystack, pos, needle)) {
            count += 1;
            pos = found + needle.len;
        } else {
            break;
        }
    }
    return count;
}

pub fn main() {
    let buf: [16384]u8 = [0; 16384];

    // 1. /oopif-child-solid.html?c=HEX (valid & invalid hex)
    {
        // 1.1 Valid lowercase 6-hex
        const r_valid = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-child-solid.html?c=ff00aa",
            "",
            "",
            &buf,
        );
        assertCheck(r_valid != null, "solid valid lowercase hex not null");
        if (body := r_valid) {
            assertCheck(std.mem.indexOf(u8, body, "<title>solid ff00aa</title>") != null, "solid title ff00aa");
            assertCheck(std.mem.indexOf(u8, body, "background:#ff00aa") != null, "solid background ff00aa");
            assertCheck(std.mem.indexOf(u8, body, "window.__c=\"ff00aa\"") != null, "solid script ff00aa");
        }

        // 1.2 Valid mixed/uppercase 6-hex
        const r_upper = MockFixtures.handleFixtureRequest(
            "/__fixtures/oopif-child-solid.html?c=A1B2C3",
            "",
            "",
            &buf,
        );
        assertCheck(r_upper != null, "solid valid uppercase hex not null");
        if (body := r_upper) {
            assertCheck(std.mem.indexOf(u8, body, "<title>solid A1B2C3</title>") != null, "solid title A1B2C3");
            assertCheck(std.mem.indexOf(u8, body, "background:#A1B2C3") != null, "solid background A1B2C3");
        }

        // 1.3 Missing c param -> fallback 000000
        const r_missing = MockFixtures.handleFixtureRequest(
            "/oopif-child-solid.html",
            "",
            "",
            &buf,
        );
        assertCheck(r_missing != null, "solid missing c not null");
        if (body := r_missing) {
            assertCheck(std.mem.indexOf(u8, body, "<title>solid 000000</title>") != null, "solid missing c defaults 000000");
        }

        // 1.4 Too short hex (3 chars, 5 chars) -> fallback 000000
        const r_short3 = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-child-solid.html?c=abc",
            "",
            "",
            &buf,
        );
        assertCheck(r_short3 != null, "solid short3 not null");
        if (body := r_short3) {
            assertCheck(std.mem.indexOf(u8, body, "<title>solid 000000</title>") != null, "solid short3 defaults 000000");
        }

        const r_short5 = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-child-solid.html?c=12345",
            "",
            "",
            &buf,
        );
        assertCheck(r_short5 != null, "solid short5 not null");
        if (body := r_short5) {
            assertCheck(std.mem.indexOf(u8, body, "<title>solid 000000</title>") != null, "solid short5 defaults 000000");
        }

        // 1.5 Too long hex (7 chars) -> fallback 000000
        const r_long7 = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-child-solid.html?c=1234567",
            "",
            "",
            &buf,
        );
        assertCheck(r_long7 != null, "solid long7 not null");
        if (body := r_long7) {
            assertCheck(std.mem.indexOf(u8, body, "<title>solid 000000</title>") != null, "solid long7 defaults 000000");
        }

        // 1.6 Non-hex characters -> fallback 000000
        const r_nonhex1 = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-child-solid.html?c=zzzzzz",
            "",
            "",
            &buf,
        );
        assertCheck(r_nonhex1 != null, "solid nonhex zzzzzz not null");
        if (body := r_nonhex1) {
            assertCheck(std.mem.indexOf(u8, body, "<title>solid 000000</title>") != null, "solid zzzzzz defaults 000000");
        }

        const r_nonhex2 = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-child-solid.html?c=12345g",
            "",
            "",
            &buf,
        );
        assertCheck(r_nonhex2 != null, "solid nonhex 12345g not null");
        if (body := r_nonhex2) {
            assertCheck(std.mem.indexOf(u8, body, "<title>solid 000000</title>") != null, "solid 12345g defaults 000000");
        }

        // 1.7 Buffer too small -> null
        let tiny_buf: [16]u8 = [0; 16];
        const r_tiny = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-child-solid.html?c=112233",
            "",
            "",
            &tiny_buf,
        );
        assertCheck(r_tiny == null, "solid tiny buffer returns null");

        std.debug.print("PASS: 1. /oopif-child-solid.html hex validation & fallback\n", .{});
    }

    // 2. /oopif-host-many.html?k=N (boundary clamping)
    {
        // 2.1 Missing k -> default 4
        const r_def = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-host-many.html",
            "",
            "",
            &buf,
        );
        assertCheck(r_def != null, "host-many default not null");
        if (body := r_def) {
            assertCheck(std.mem.indexOf(u8, body, "<title>oopif-host-many 4</title>") != null, "host-many default title 4");
            assertCheck(countOccurrences(body, "<iframe ") == 4, "host-many default 4 iframes");
            assertCheck(std.mem.indexOf(u8, body, "c=ffff00") != null, "host-many default includes 4th color");
            assertCheck(std.mem.indexOf(u8, body, "c=ff00ff") == null, "host-many default excludes 5th color");
        }

        // 2.2 Lower clamp: k=0 -> 1
        const r_zero = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-host-many.html?k=0",
            "",
            "",
            &buf,
        );
        assertCheck(r_zero != null, "host-many k=0 not null");
        if (body := r_zero) {
            assertCheck(std.mem.indexOf(u8, body, "<title>oopif-host-many 1</title>") != null, "host-many k=0 clamps to 1");
            assertCheck(countOccurrences(body, "<iframe ") == 1, "host-many k=0 has 1 iframe");
            assertCheck(std.mem.indexOf(u8, body, "c=ff0000") != null, "host-many k=0 has red");
            assertCheck(std.mem.indexOf(u8, body, "c=00ff00") == null, "host-many k=0 excludes green");
        }

        // 2.3 Invalid/negative k -> parseInt catches to 4
        const r_neg = MockFixtures.handleFixtureRequest(
            "/__fixtures/oopif-host-many.html?k=-3",
            "",
            "",
            &buf,
        );
        assertCheck(r_neg != null, "host-many k=-3 not null");
        if (body := r_neg) {
            assertCheck(std.mem.indexOf(u8, body, "<title>oopif-host-many 4</title>") != null, "host-many k=-3 falls back to 4");
            assertCheck(countOccurrences(body, "<iframe ") == 4, "host-many k=-3 has 4 iframes");
        }

        const r_nan = MockFixtures.handleFixtureRequest(
            "/__fixtures/oopif-host-many.html?k=abc",
            "",
            "",
            &buf,
        );
        assertCheck(r_nan != null, "host-many k=abc not null");
        if (body := r_nan) {
            assertCheck(std.mem.indexOf(u8, body, "<title>oopif-host-many 4</title>") != null, "host-many k=abc falls back to 4");
        }

        // 2.4 Mid value: k=2 -> 2
        const r_two = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-host-many.html?k=2",
            "",
            "",
            &buf,
        );
        assertCheck(r_two != null, "host-many k=2 not null");
        if (body := r_two) {
            assertCheck(std.mem.indexOf(u8, body, "<title>oopif-host-many 2</title>") != null, "host-many k=2 title");
            assertCheck(countOccurrences(body, "<iframe ") == 2, "host-many k=2 has 2 iframes");
        }

        // 2.5 Upper clamp: k=8 -> 8, k=99 -> 8
        const r_eight = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-host-many.html?k=8",
            "",
            "",
            &buf,
        );
        assertCheck(r_eight != null, "host-many k=8 not null");
        if (body := r_eight) {
            assertCheck(std.mem.indexOf(u8, body, "<title>oopif-host-many 8</title>") != null, "host-many k=8 title");
            assertCheck(countOccurrences(body, "<iframe ") == 8, "host-many k=8 has 8 iframes");
            assertCheck(std.mem.indexOf(u8, body, "c=008080") != null, "host-many k=8 includes 8th color");
        }

        const r_over = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-host-many.html?k=99",
            "",
            "",
            &buf,
        );
        assertCheck(r_over != null, "host-many k=99 not null");
        if (body := r_over) {
            assertCheck(std.mem.indexOf(u8, body, "<title>oopif-host-many 8</title>") != null, "host-many k=99 clamps to 8");
            assertCheck(countOccurrences(body, "<iframe ") == 8, "host-many k=99 has 8 iframes");
        }

        // 2.6 Buffer too small -> null
        let small_buf: [64]u8 = [0; 64];
        const r_small = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/oopif-host-many.html?k=4",
            "",
            "",
            &small_buf,
        );
        assertCheck(r_small == null, "host-many small buffer returns null");

        std.debug.print("PASS: 2. /oopif-host-many.html k boundary clamping\n", .{});
    }

    // 3. module-registry-* static fixture files
    {
        const mr_html = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/module-registry.html",
            "",
            "",
            &buf,
        );
        assertCheck(mr_html != null, "module-registry.html not null");
        if (c := mr_html) {
            assertCheck(c.len > 0, "module-registry.html non-empty");
        }
        assertCheck(std.mem.eql(u8, MockFixtures.getContentType("https://ks.invalid/__fixtures/module-registry.html"), "text/html"), "module-registry.html mime");

        const mr_cfg = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/module-registry-config.json",
            "",
            "",
            &buf,
        );
        assertCheck(mr_cfg != null, "module-registry-config.json not null");
        assertCheck(std.mem.eql(u8, MockFixtures.getContentType("https://ks.invalid/__fixtures/module-registry-config.json"), "application/json"), "module-registry-config.json mime");

        const mr_fb = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/module-registry-fallback.json",
            "",
            "",
            &buf,
        );
        assertCheck(mr_fb != null, "module-registry-fallback.json not null");

        const mr_lib = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/module-registry-lib.js",
            "",
            "",
            &buf,
        );
        assertCheck(mr_lib != null, "module-registry-lib.js not null");
        assertCheck(std.mem.eql(u8, MockFixtures.getContentType("https://ks.invalid/__fixtures/module-registry-lib.js"), "application/javascript"), "module-registry-lib.js mime");

        const mr_live = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/module-registry-live-reader.js",
            "",
            "",
            &buf,
        );
        assertCheck(mr_live != null, "module-registry-live-reader.js not null");

        const mr_meta = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/module-registry-meta.js",
            "",
            "",
            &buf,
        );
        assertCheck(mr_meta != null, "module-registry-meta.js not null");

        const mr_tla = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/module-registry-tla.js",
            "",
            "",
            &buf,
        );
        assertCheck(mr_tla != null, "module-registry-tla.js not null");

        const mr_css = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/module-registry-sheet.css",
            "",
            "",
            &buf,
        );
        assertCheck(mr_css != null, "module-registry-sheet.css not null");
        assertCheck(std.mem.eql(u8, MockFixtures.getContentType("https://ks.invalid/__fixtures/module-registry-sheet.css"), "text/css"), "module-registry-sheet.css mime");

        const mr_mal = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/module-registry-malformed-json.txt",
            "",
            "",
            &buf,
        );
        assertCheck(mr_mal != null, "module-registry-malformed-json.txt not null");

        const mr_missing = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/module-registry-nonexistent.json",
            "",
            "",
            &buf,
        );
        assertCheck(mr_missing == null, "module-registry-nonexistent.json is null");

        std.debug.print("PASS: 3. module-registry-* fixtures & MIME\n", .{});
    }

    // 4. Non-existent paths returning null / 404 boundary
    {
        const n1 = MockFixtures.handleFixtureRequest(
            "/__fixtures/nonexistent-edge-404.html",
            "",
            "",
            &buf,
        );
        assertCheck(n1 == null, "relative nonexistent fixture returns null");

        const n2 = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/no-such-dir/no-such-file.js",
            "",
            "",
            &buf,
        );
        assertCheck(n2 == null, "nested nonexistent fixture returns null");

        const n3 = MockFixtures.handleFixtureRequest(
            "http://other.domain.invalid",
            "",
            "",
            &buf,
        );
        assertCheck(n3 == null, "empty path on .invalid returns null");

        const n4 = MockFixtures.handleFixtureRequest(
            "https://example.com/not-a-fixture.html",
            "",
            "",
            &buf,
        );
        assertCheck(n4 == null, "non-fixture host returns null");

        const n5 = MockFixtures.handleFixtureRequest(
            "",
            "",
            "",
            &buf,
        );
        assertCheck(n5 == null, "empty url returns null");

        std.debug.print("PASS: 4. non-existent paths & 404/null boundaries\n", .{});
    }
}
