// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
import { expect, test } from "./harness/index.cjs";
import { captureTrustedFetch, takeTrustedRequestMeta, trustedFetch, } from "../../../page-script/src/trusted-fetch.ts";
test("trusted metadata survives repeated reads of the same retry init", async () => {
    const originalFetch = globalThis.fetch;
    const expected = { resourceType: "Stylesheet", initiator: "parser" };
    const seen = [];
    globalThis.fetch = (async (_input, init) => {
        seen.push(takeTrustedRequestMeta(init));
        seen.push(takeTrustedRequestMeta(init));
        return new Response(null, { status: 204 });
    });
    try {
        captureTrustedFetch();
        await trustedFetch("https://ks.invalid/retry", {}, expected);
    }
    finally {
        globalThis.fetch = originalFetch;
    }
    expect(seen).toEqual([expected, expected]);
});
