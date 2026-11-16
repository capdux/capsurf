// e2e/alert-non-throwing.test.js
// Migrated from e2e/alert-non-throwing.e2e.test.ts
// Verifies window.alert, confirm, prompt stubs do not throw and return expected defaults.

const { expect, test } = require("./harness");

test("window.alert, confirm, prompt do not throw and return expected defaults", async ({ cdpSession: cdp }) => {
  const r = await cdp.send("Runtime.evaluate", {
    expression: `JSON.stringify((function () {
      var out = {
        alertType: typeof window.alert,
        alertThrew: null,
        alertReturned: null,
        confirmType: typeof window.confirm,
        confirmReturned: null,
        promptType: typeof window.prompt,
        promptReturned: null,
        globalAlert: typeof globalThis.alert,
        isAlertSame: window.alert === globalThis.alert,
        isConfirmSame: window.confirm === globalThis.confirm,
        isPromptSame: window.prompt === globalThis.prompt,
      };
      try {
        out.alertReturned = String(alert("kitesurf alert probe"));
      } catch (e) {
        out.alertThrew = String((e && e.message) || e);
      }
      try {
        out.confirmReturned = confirm("confirm probe");
      } catch (e) {
        out.confirmThrew = String((e && e.message) || e);
      }
      try {
        out.promptReturned = prompt("prompt probe", "default");
      } catch (e) {
        out.promptThrew = String((e && e.message) || e);
      }
      return out;
    })())`,
    returnByValue: true,
  });

  expect(r.exceptionDetails).toBeFalsy();
  const parsed = JSON.parse(String(r.result?.value));
  expect(parsed.alertType).toBe("function");
  expect(parsed.alertThrew).toBe(null);
  expect(parsed.alertReturned).toBe("undefined");
  expect(parsed.confirmType).toBe("function");
  expect(parsed.confirmReturned).toBe(false);
  expect(parsed.promptType).toBe("function");
  expect(parsed.promptReturned).toBe(null);
  expect(parsed.globalAlert).toBe("function");
  expect(parsed.isAlertSame).toBe(true);
  expect(parsed.isConfirmSame).toBe(true);
  expect(parsed.isPromptSame).toBe(true);
});
