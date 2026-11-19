// e2e/run_all.js
// CapSurf E2E Test Suite Runner
// Discovers and executes all migrated E2E test files in e2e/,
// collects test outcomes, calculates pass rate, and outputs a formatted report.

import fs from 'node:fs';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const BASE_PORT = 19230;

function findTestFiles(dir) {
  return fs.readdirSync(dir)
    .filter((file) => file.endsWith('.test.js'))
    .sort()
    .map((file) => path.join(dir, file));
}

function runTestFile(filePath, index) {
  const port = BASE_PORT + index;
  const fileName = path.basename(filePath);

  return new Promise((resolve) => {
    const start = Date.now();
    const env = {
      ...process.env,
      REMOTE_DEBUGGING_PORT: String(port),
    };

    const proc = spawn('node', [filePath], {
      env,
      stdio: ['ignore', 'pipe', 'pipe'],
    });

    let stdout = '';
    let stderr = '';

    proc.stdout.on('data', (d) => stdout += d.toString());
    proc.stderr.on('data', (d) => stderr += d.toString());

    proc.on('close', (code) => {
      const duration = Date.now() - start;
      const passMatch = stdout.match(/Test Summary:\s+(\d+)\s+passed,\s+(\d+)\s+failed\s+of\s+(\d+)\s+total/);
      let passedTests = 0;
      let failedTests = 0;
      let totalTests = 0;

      if (passMatch) {
        passedTests = parseInt(passMatch[1], 10);
        failedTests = parseInt(passMatch[2], 10);
        totalTests = parseInt(passMatch[3], 10);
      } else if (code === 0) {
        passedTests = 1;
        totalTests = 1;
      } else {
        failedTests = 1;
        totalTests = 1;
      }

      resolve({
        fileName,
        filePath,
        code,
        duration,
        passedTests,
        failedTests,
        totalTests,
        stdout,
        stderr,
      });
    });
  });
}

async function main() {
  console.log('╔════════════════════════════════════════════════════════════════════╗');
  console.log('║       CapSurf Native Headless Browser: End-to-End Test Suite       ║');
  console.log('╚════════════════════════════════════════════════════════════════════╝\n');

  const testFiles = findTestFiles(__dirname);

  if (testFiles.length === 0) {
    console.warn('[!] No *.test.js files found in', __dirname);
    process.exit(0);
  }

  console.log(`Discovered ${testFiles.length} test suite(s):`);
  testFiles.forEach((f, i) => console.log(`  ${i + 1}. ${path.basename(f)}`));
  console.log('\nRunning test suites...\n');

  let totalSuitesPassed = 0;
  let totalSuitesFailed = 0;
  let totalTestsPassed = 0;
  let totalTestsFailed = 0;
  const suiteResults = [];

  const overallStart = Date.now();

  for (let i = 0; i < testFiles.length; i++) {
    const file = testFiles[i];
    const fileName = path.basename(file);
    process.stdout.write(`• Running [${i + 1}/${testFiles.length}] ${fileName} ... `);

    const result = await runTestFile(file, i);
    suiteResults.push(result);

    totalTestsPassed += result.passedTests;
    totalTestsFailed += result.failedTests;

    if (result.code === 0 && result.failedTests === 0) {
      totalSuitesPassed++;
      console.log(`✔ PASSED (${result.duration}ms, ${result.passedTests}/${result.totalTests} tests)`);
    } else {
      totalSuitesFailed++;
      console.log(`✘ FAILED (exit code: ${result.code}, ${result.duration}ms)`);
      if (result.stdout) console.log(result.stdout.trim());
      if (result.stderr) console.error(result.stderr.trim());
    }
  }

  const overallDuration = Date.now() - overallStart;
  const totalTests = totalTestsPassed + totalTestsFailed;
  const passRate = totalTests > 0 ? ((totalTestsPassed / totalTests) * 100).toFixed(1) : '0.0';

  console.log('\n======================================================================');
  console.log('                        TEST RUNNER SUMMARY                           ');
  console.log('======================================================================');
  console.log(` Suites   : ${totalSuitesPassed} passed, ${totalSuitesFailed} failed, ${testFiles.length} total`);
  console.log(` Tests    : ${totalTestsPassed} passed, ${totalTestsFailed} failed, ${totalTests} total`);
  console.log(` Pass Rate: ${passRate}%`);
  console.log(` Duration : ${(overallDuration / 1000).toFixed(2)}s`);
  console.log('======================================================================\n');

  if (totalSuitesFailed > 0 || totalTestsFailed > 0) {
    console.error('❌ Status: FAILED - One or more tests failed.');
    process.exit(1);
  } else {
    console.log('✅ Status: 100% PASSED - All End-to-End tests succeeded.');
    process.exit(0);
  }
}

// ESM equivalent of require.main === module
if (import.meta.url === new URL(`file://${process.argv[1]}`).href) {
  main().catch((err) => {
    console.error('Unexpected runner error:', err);
    process.exit(1);
  });
}

export { main, findTestFiles, runTestFile };
