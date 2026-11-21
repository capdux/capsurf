#!/usr/bin/env python3
"""Concurrent E2E test runner for CapSurf (test/e2e/*.e2e.test.ts and test/e2e/demo/*.e2e.test.ts)."""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import queue
import re
import subprocess
import sys
import time
from pathlib import Path
from typing import Any

MAX_WORKERS = int(os.environ.get("MAX_WORKERS", "6"))
BASE_PORT = 19310

ARCH_SKIP_CATEGORIES: dict[str, tuple[str, set[str]]] = {
    "boa_v8_bridge": (
        "Boa-Wasm + V8 bridge internals",
        {
            "boa-bridged-intrinsic-proto",
            "boa-code-stats",
            "boa-ctx-reentry",
            "boa-heap-shape",
            "boa-interner-reclaim",
            "boa-object-from-page",
            "bridge-array-lazy",
            "bridge-cycle-collect",
            "bridge-defineproperty",
            "bridge-dom-fastpath",
            "bridge-fn-liveness",
            "bridge-inherit-v8",
            "bridge-pin-stats",
            "bridge-proxy-stack-limit",
        },
    ),
    "workers_wasm_external": (
        "Cloudflare Workers / Wasm-Isolate / missing external package",
        {
            "cdp-scene-host",
            "cdp-scene-stream",
            "idl-conformance",
            "inline-wasm-bake",
            "isolate-kill",
            "isolate-wall-cap",
            "local-tracing",
            "navigate-isolate-death",
            "wasm-bake-demo",
            "wasm-crash-surface",
            "wasm-panic-unwind",
            "web-bot-auth",
        },
    ),
    "quickjs_rc_vs_v8_gc": (
        "QuickJS immediate RC vs V8 delayed-GC leak-control assertions",
        {
            "style-churn-memory",
            "text-content-memory",
        },
    ),
}

SMOKE_SUITES: list[str] = [
    "test/e2e/smoke.e2e.test.ts",
    "test/e2e/blank-page-document.e2e.test.ts",
    "test/e2e/alert-non-throwing.e2e.test.ts",
    "test/test/e2e/demo/hello.e2e.test.ts",
    "test/test/e2e/demo/cookies-demo.e2e.test.ts",
]

PASS_RE = re.compile(r"^\s*(\d+)\s+pass\b", re.MULTILINE)
FAIL_RE = re.compile(r"^\s*(\d+)\s+fail\b", re.MULTILINE)
EXPECT_RE = re.compile(r"^\s*(\d+)\s+expect\(\)\s+calls\b", re.MULTILINE)


def suite_stem(rel_path: str) -> str:
    name = Path(rel_path).name
    if name.endswith(".e2e.test.ts"):
        return name[: -len(".e2e.test.ts")]
    return Path(name).stem


def classify_arch_skip(rel_path: str) -> tuple[str, str] | None:
    stem = suite_stem(rel_path)
    for cat_key, (cat_desc, stems) in ARCH_SKIP_CATEGORIES.items():
        if stem in stems:
            return cat_key, cat_desc
    return None


def discover_suites(repo_root: Path) -> list[str]:
    suites: list[Path] = []
    e2e_dir = repo_root / "test" / "e2e"
    demo_dir = e2e_dir / "demo"
    if e2e_dir.is_dir():
        suites.extend(sorted(e2e_dir.glob("*.e2e.test.ts")))
    if demo_dir.is_dir():
        suites.extend(sorted(demo_dir.glob("*.e2e.test.ts")))
    return [str(p.relative_to(repo_root)) for p in suites]


def cleanup_port(port: int) -> None:
    subprocess.run(
        ["pkill", "-f", f"--remote-debugging-port={port}"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )


def parse_bun_counts(output: str) -> tuple[int, int, int]:
    passed = 0
    failed = 0
    expects = 0
    if m := PASS_RE.search(output):
        passed = int(m.group(1))
    if m := FAIL_RE.search(output):
        failed = int(m.group(1))
    if m := EXPECT_RE.search(output):
        expects = int(m.group(1))
    return passed, failed, expects


def run_one_suite(
    rel_path: str,
    repo_root: Path,
    capsurf_bin: str,
    port_slots: queue.Queue[int],
    timeout_sec: int,
) -> dict[str, Any]:
    slot = port_slots.get()
    port = BASE_PORT + slot
    t0 = time.monotonic()
    cleanup_port(port)
    env = os.environ.copy()
    env["CAPSURF_BIN"] = capsurf_bin
    env["REMOTE_DEBUGGING_PORT"] = str(port)

    status = "PASS"
    returncode = 0
    output = ""
    timed_out = False

    try:
        proc = subprocess.run(
            ["bun", "test", rel_path],
            cwd=str(repo_root),
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=timeout_sec,
            check=False,
        )
        returncode = proc.returncode
        output = proc.stdout or ""
        if returncode != 0:
            status = "FAIL"
    except subprocess.TimeoutExpired as exc:
        timed_out = True
        status = "TIMEOUT"
        returncode = 124
        if isinstance(exc.stdout, bytes):
            output = exc.stdout.decode("utf-8", errors="replace")
        elif isinstance(exc.stdout, str):
            output = exc.stdout
    finally:
        cleanup_port(port)
        port_slots.put(slot)

    duration = round(time.monotonic() - t0, 2)
    passed, failed, expects = parse_bun_counts(output)
    return {
        "suite": rel_path,
        "stem": suite_stem(rel_path),
        "status": status,
        "returncode": returncode,
        "timed_out": timed_out,
        "port": port,
        "duration_s": duration,
        "pass_count": passed,
        "fail_count": failed,
        "expect_count": expects,
        "output": output,
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Concurrent CapSurf E2E runner for bun test"
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=MAX_WORKERS,
        help=f"Max concurrent worker slots (default: {MAX_WORKERS})",
    )
    parser.add_argument(
        "--include-arch",
        action="store_true",
        help="Include Kitesurf-exclusive architecture suites instead of skipping them",
    )
    parser.add_argument(
        "--filter",
        dest="filter_sub",
        default="",
        help="Only run suites whose path contains this substring",
    )
    parser.add_argument(
        "--smoke",
        action="store_true",
        help="Run 5 fast representative smoke suites",
    )
    parser.add_argument(
        "--json-out",
        default="",
        help="Optional path to write JSON results summary",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=120,
        help="Per-suite timeout in seconds (default: 120)",
    )
    args = parser.parse_args()

    if args.workers < 1:
        print("Error: --workers must be >= 1", file=sys.stderr)
        return 2

    repo_root = Path(__file__).resolve().parent.parent
    capsurf_bin = os.environ.get("CAPSURF_BIN", "./bin/main")
    bin_path = (repo_root / capsurf_bin) if not os.path.isabs(capsurf_bin) else Path(capsurf_bin)
    if not bin_path.exists():
        print(f"Error: CAPSURF_BIN not found: {bin_path}", file=sys.stderr)
        return 2

    if args.smoke:
        candidates = [s for s in SMOKE_SUITES if (repo_root / s).exists()]
    else:
        candidates = discover_suites(repo_root)

    if args.filter_sub:
        candidates = [s for s in candidates if args.filter_sub in s]

    to_run: list[str] = []
    skipped: list[dict[str, str]] = []
    skip_by_cat: dict[str, int] = {k: 0 for k in ARCH_SKIP_CATEGORIES}

    for rel_path in candidates:
        if not args.include_arch:
            arch_info = classify_arch_skip(rel_path)
            if arch_info is not None:
                cat_key, cat_desc = arch_info
                skip_by_cat[cat_key] += 1
                skipped.append(
                    {
                        "suite": rel_path,
                        "stem": suite_stem(rel_path),
                        "category": cat_key,
                        "reason": cat_desc,
                    }
                )
                continue
        to_run.append(rel_path)

    print(
        f"CapSurf E2E Runner | bin={capsurf_bin} | workers={args.workers} "
        f"| selected={len(candidates)} | run={len(to_run)} | skipped={len(skipped)}"
    )
    if skipped:
        cat_summary = ", ".join(
            f"{k}={v}" for k, v in skip_by_cat.items() if v > 0
        )
        print(f"Skipped Kitesurf-arch suites ({len(skipped)}): {cat_summary}")

    if not to_run:
        print("No suites to run.")
        if args.json_out:
            out_path = Path(args.json_out)
            out_path.parent.mkdir(parents=True, exist_ok=True)
            out_path.write_text(
                json.dumps(
                    {
                        "summary": {
                            "total_selected": len(candidates),
                            "run": 0,
                            "passed": 0,
                            "failed": 0,
                            "skipped": len(skipped),
                            "duration_s": 0.0,
                        },
                        "skipped": skipped,
                        "results": [],
                    },
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )
        return 0

    port_slots: queue.Queue[int] = queue.Queue()
    for i in range(args.workers):
        port_slots.put(i)

    t_start = time.monotonic()
    results: list[dict[str, Any]] = []
    total = len(to_run)

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
        future_map = {
            pool.submit(
                run_one_suite, rel_path, repo_root, capsurf_bin, port_slots, args.timeout
            ): rel_path
            for rel_path in to_run
        }
        completed = 0
        for fut in concurrent.futures.as_completed(future_map):
            completed += 1
            res = fut.result()
            results.append(res)
            print(
                f"[{completed}/{total}] {res['status']:7s} {res['suite']} "
                f"({res['duration_s']:.2f}s, {res['pass_count']} pass, "
                f"{res['fail_count']} fail, port={res['port']})",
                flush=True,
            )

    results.sort(key=lambda r: r["suite"])
    total_duration = round(time.monotonic() - t_start, 2)
    passed_suites = [r for r in results if r["status"] == "PASS"]
    failed_suites = [r for r in results if r["status"] != "PASS"]
    total_tests_pass = sum(r["pass_count"] for r in results)
    total_tests_fail = sum(r["fail_count"] for r in results)
    total_expects = sum(r["expect_count"] for r in results)

    if failed_suites:
        print("\n--- Failed Suite Details ---")
        for r in failed_suites:
            print(f"\n[{r['status']}] {r['suite']} (rc={r['returncode']}, port={r['port']}):")
            lines = r["output"].strip().splitlines()
            for line in lines[-25:]:
                print(f"  {line}")

    print(
        f"\nSummary: {len(passed_suites)}/{len(results)} suites passed, "
        f"{len(failed_suites)} failed, {len(skipped)} skipped | "
        f"tests: {total_tests_pass} pass, {total_tests_fail} fail, "
        f"{total_expects} expect() | {total_duration:.2f}s"
    )

    if args.json_out:
        out_path = Path(args.json_out)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        serializable_results = [
            {k: v for k, v in r.items() if k != "output"} for r in results
        ]
        payload = {
            "summary": {
                "total_selected": len(candidates),
                "run": len(results),
                "passed": len(passed_suites),
                "failed": len(failed_suites),
                "skipped": len(skipped),
                "skip_by_category": skip_by_cat,
                "tests_pass": total_tests_pass,
                "tests_fail": total_tests_fail,
                "expect_calls": total_expects,
                "duration_s": total_duration,
            },
            "skipped": skipped,
            "results": serializable_results,
        }
        out_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    return 0 if not failed_suites else 1


if __name__ == "__main__":
    sys.exit(main())
