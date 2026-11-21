// test/net/test_sandbox_fixtures.cp
// Pure CP white-box unit tests for Phase 248-249 MockFixtures sandbox routes:
// /__fixtures/sandbox-host.html?src=...&sandbox=...
// /__fixtures/sandbox-child.html
// /__fixtures/sandbox-worker-child.html?mode=blob

import { test, expect_eq } from "cap:test";
import { MockFixtures } from "../../../src/net/mock_fixtures.cp";

fn contains(haystack: []const u8, needle: []const u8): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

fn main() {
    test("1. /__fixtures/sandbox-host.html with src & sandbox query params", () => {
        let buf: [16384]u8 = [0; 16384];

        const resp = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/sandbox-host.html?src=%2F__fixtures%2Fsandbox-child.html&sandbox=allow-scripts%20allow-same-origin",
            "",
            "",
            &buf,
        );
        expect_eq(resp != null, true);
        if (body := resp) {
            expect_eq(contains(body, "<title>sandbox host</title>"), true);
            expect_eq(contains(body, "<iframe id=\"f\" src=\"/__fixtures/sandbox-child.html\" sandbox=\"allow-scripts allow-same-origin\""), true);
            expect_eq(contains(body, "window.__msgs = [];"), true);
        }

        // Without sandbox param -> no sandbox attribute
        const resp_no_sb = MockFixtures.handleFixtureRequest(
            "/__fixtures/sandbox-host.html?src=%2F__fixtures%2Fsandbox-child.html",
            "",
            "",
            &buf,
        );
        expect_eq(resp_no_sb != null, true);
        if (body := resp_no_sb) {
            expect_eq(contains(body, "<iframe id=\"f\" src=\"/__fixtures/sandbox-child.html\"\n"), true);
            expect_eq(contains(body, "sandbox=\""), false);
        }

        // Attribute escaping for quotes in src
        const resp_esc = MockFixtures.handleFixtureRequest(
            "/__fixtures/sandbox-host.html?src=%2Fchild%3Fa%3D1%26b%3D%222%22",
            "",
            "",
            &buf,
        );
        expect_eq(resp_esc != null, true);
        if (body := resp_esc) {
            expect_eq(contains(body, "src=\"/child?a=1&b=&quot;2&quot;\""), true);
        }
    });

    test("2. /__fixtures/sandbox-child.html", () => {
        let buf: [16384]u8 = [0; 16384];

        const resp = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/sandbox-child.html",
            "",
            "",
            &buf,
        );
        expect_eq(resp != null, true);
        if (body := resp) {
            expect_eq(contains(body, "<title>sandbox child</title>"), true);
            expect_eq(contains(body, "<p id=\"markup\">sandbox child markup</p>"), true);
            expect_eq(contains(body, "document.cookie;"), true);
            expect_eq(contains(body, "localStorage.getItem(\"k\");"), true);
            expect_eq(contains(body, "parent.postMessage({ ran: true, cookie: cookie, storage: storage }, \"*\");"), true);
        }
    });

    test("3. /__fixtures/sandbox-worker-child.html?mode=blob & default mode", () => {
        let buf: [16384]u8 = [0; 16384];

        const resp_blob = MockFixtures.handleFixtureRequest(
            "https://ks.invalid/__fixtures/sandbox-worker-child.html?mode=blob",
            "",
            "",
            &buf,
        );
        expect_eq(resp_blob != null, true);
        if (body := resp_blob) {
            expect_eq(contains(body, "<title>sandbox worker child</title>"), true);
            expect_eq(contains(body, "URL.createObjectURL(new Blob"), true);
            expect_eq(contains(body, "URL.revokeObjectURL(url);"), true);
            expect_eq(contains(body, "blobOrigin: new URL(url).origin"), true);
            expect_eq(contains(body, "new Worker("), false);
        }

        const resp_default = MockFixtures.handleFixtureRequest(
            "/__fixtures/sandbox-worker-child.html",
            "",
            "",
            &buf,
        );
        expect_eq(resp_default != null, true);
        if (body := resp_default) {
            expect_eq(contains(body, "<title>sandbox worker child</title>"), true);
            expect_eq(contains(body, "new Worker(\"/__fixtures/worker-echo.js\")"), true);
            expect_eq(contains(body, "sameOriginWorker"), true);
            expect_eq(contains(body, "createObjectURL"), false);
        }
    });
}
