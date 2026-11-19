// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
export async function waitForTitle(page, expected, options = {}) {
    const { prefix = false, timeout = 10_000 } = options;
    await page
        .waitForFunction((wanted, startsWith) => startsWith ? document.title.startsWith(wanted) : document.title === wanted, { timeout }, expected, prefix)
        .catch(() => { });
    return page.title();
}
