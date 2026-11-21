// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
// https://opensource.org/licenses/Apache-2.0
export async function setContent(page, html, options = {}) {
    await page.goto(`data:text/html;charset=utf-8,${encodeURIComponent(html)}`, {
        waitUntil: options.waitUntil ?? "load",
        timeout: options.timeout ?? 15_000,
    });
}
