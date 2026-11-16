// e2e/preload.ts
// Native Bun preload plugin for Kitesurf E2E test runner
import fs from "node:fs";
import path from "node:path";

// Auto-seed node_modules/cloudflare:test shim before test files are evaluated
try {
  const cfTestDir = path.resolve(__dirname, "../node_modules/cloudflare:test");
  if (!fs.existsSync(cfTestDir)) {
    fs.mkdirSync(cfTestDir, { recursive: true });
    fs.writeFileSync(path.join(cfTestDir, "index.ts"), 'export * from "../../e2e/shim/cloudflare-test.ts";\n');
    fs.writeFileSync(path.join(cfTestDir, "package.json"), JSON.stringify({
      name: "cloudflare:test",
      main: "index.ts",
      module: "index.ts",
      types: "index.ts",
    }, null, 2));
  }
} catch {}
