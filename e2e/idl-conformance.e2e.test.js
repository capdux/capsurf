// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
// The generated-IDL ledger rejects both new gaps and stale entries. After an intended change, run
// this test with `-u`, review `idl-gaps.json`, then run `pnpm webidl:generate`.
import { expect, test } from "./harness/index.cjs";
import { setContent } from "./set-content";
import { readJsonReport } from "./eval-report";
import { WEBIDL_MODEL } from "../../../dom-shim/src/idl/model.generated.ts";
import { buildRequest, grade } from "./idl-conformance";
import html from "./html/idl-conformance.html?raw";
const MODEL = Object.values(WEBIDL_MODEL);
test("page globals match the pinned WebIDL, up to the gap ledger", async ({ page, cdpSession }) => {
    const request = JSON.stringify(buildRequest(MODEL));
    await setContent(page, html.replace("/*__IDL_REQUEST__*/ null", () => request), {
        waitUntil: "domcontentloaded",
    });
    const rawReport = await readJsonReport(cdpSession, "globalThis.__idlConformance");
    // The fixture emits this schema; readJsonReport checks only the outer JSON object.
    // oxlint-disable-next-line kitesurf/no-chained-type-assertions -- shallow JSON boundary.
    const report = rawReport;
    expect(Object.keys(report.interfaces)).toEqual(Object.keys(JSON.parse(request).interfaces));
    const gaps = grade(MODEL, report);
    await expect(`${JSON.stringify(gaps, null, 2)}\n`).toMatchFileSnapshot("./idl-gaps.json");
}, 120_000);
