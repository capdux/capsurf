// e2e/html-parser.e2e.test.ts
import { expect, test } from "./harness";

test("parseHtml: Basic HTML structure", async ({ page }) => {
  await page.setContent("<html><body><h1>Hello</h1></body></html>");
  const h1 = await page.evaluate(() => document.querySelector("h1").textContent);
  expect(h1).toBe("Hello");
});

test("parseHtml: Misnested tags (AAA)", async ({ page }) => {
  await page.setContent("<div><b>Bold <i>Italic</b> Still Italic?</i></div>");
  const boldText = await page.evaluate(() => document.querySelector("b").textContent);
  // Based on AAA logic, <b> should contain "Bold " and "Italic".
  // The "Still Italic?" should be outside.
  expect(boldText).toContain("Bold ");
  expect(boldText).toContain("Italic");
});
