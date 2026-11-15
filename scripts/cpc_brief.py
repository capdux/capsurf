#!/usr/bin/env python3
"""
Token-saving verifier for CapSurf: compile + ast-check + static gates, compact output.
Usage: python3 scripts/cpc_brief.py [-n 8] <stem.cp> [more.cp ...]
Leverages `/data/dev/cp/cap/bin/cpc check --json` for structured error diagnostics,
groups errors by code, prints first N distinct root errors, and checks CP syntax gates.
Never prints raw verbose compiler output. Emits <stem>.zig (removes *.zig.map).
"""
import argparse
import json
import os
import re
import shutil
import subprocess
from collections import Counter, OrderedDict

CPC_BIN = "/data/dev/cp/cap/bin/cpc" if os.path.exists("/data/dev/cp/cap/bin/cpc") else "cpc"
ZIG_BIN = (
    shutil.which("zig")
    or ("/data/dev/bun/vendor/zig/zig" if os.path.exists("/data/dev/bun/vendor/zig/zig") else "zig")
)


def run(cmd):
    return subprocess.run(cmd, shell=True, capture_output=True, text=True)


def gates(cp, bak):
    txt = open(cp, encoding="utf-8").read()
    res = OrderedDict()
    res["zig{}"] = len(re.findall(r"\bzig\s*\{", txt))
    res["var"] = len(re.findall(r"^\s*(?:pub\s+)?var\s", txt, re.M))
    res["tuple.@N"] = len(re.findall(r'\.@"\d+"', txt))
    res["anyopaque"] = len(re.findall(r"\*anyopaque|\*any!", txt))
    if os.path.exists(bak):
        a = sum(1 for _ in open(cp, encoding="utf-8"))
        b = sum(1 for _ in open(bak, encoding="utf-8"))
        res["ratio"] = round(a / b, 3)
        res["ratio_ok"] = a / b <= 1.2
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-n", type=int, default=8, help="Max distinct errors to display")
    ap.add_argument("files", nargs="+", help="Target .cp files")
    a = ap.parse_args()

    for cp_arg in a.files:
        stem = cp_arg[:-3] if cp_arg.endswith(".cp") else cp_arg
        cp, zig, bak = stem + ".cp", stem + ".zig", stem + "_.zig"

        if not os.path.exists(cp):
            print(f"== {cp}: FAIL(not found)")
            continue

        src_lines = open(cp, encoding="utf-8").read().splitlines()

        # Step 1: Run structured cpc check --json
        r_json = run(f"{CPC_BIN} check --json {cp}")
        errs = []
        try:
            parsed = json.loads(r_json.stdout)
            for d in parsed.get("diagnostics", []):
                errs.append({
                    "code": d.get("code", "E0000"),
                    "msg": d.get("message", ""),
                    "line": d.get("line", 0),
                    "suggestion": d.get("suggestion") or d.get("help", ""),
                })
        except Exception:
            err_re = re.compile(r"^error\[(E\d+)\]: (.*)$")
            loc_re = re.compile(r"^\s*--> (.+?):(\d+):(\d+)")
            cur = None
            for ln in (r_json.stdout + r_json.stderr).splitlines():
                m = err_re.match(ln)
                if m:
                    cur = {"code": m.group(1), "msg": m.group(2), "line": 0, "suggestion": ""}
                    errs.append(cur)
                    continue
                m = loc_re.match(ln)
                if m and cur and not cur["line"]:
                    cur["line"] = int(m.group(2))

        # Step 2: Try compilation & ast-check if check passed
        status = "PASS"
        if errs:
            status = "FAIL(cpc)"
        else:
            r_comp = run(f"{CPC_BIN} --lib {cp} -o {zig}")
            if r_comp.returncode != 0:
                status = "FAIL(codegen)"
                for ln in (r_comp.stdout + r_comp.stderr).splitlines():
                    if "error" in ln:
                        errs.append({"code": "E_CODEGEN", "msg": ln.strip(), "line": 0, "suggestion": ""})
            else:
                z = run(f"{ZIG_BIN} ast-check {zig}")
                if z.returncode != 0:
                    status = "FAIL(ast-check)"
                    zl = [l for l in (z.stderr + z.stdout).splitlines() if re.match(r".*\.zig:\d+:\d+: error", l)]
                    for l in zl[: a.n]:
                        print("    [ast-check]", l[:180])
                    print(f"    total ast-check errors: {len(zl)}")

        if os.path.exists(zig + ".map"):
            os.remove(zig + ".map")

        print(f"== {cp}: {status}")
        if errs:
            cnt = Counter(e["code"] for e in errs)
            print(f"   total={len(errs)} by code: {', '.join(f'{k} x{v}' for k, v in cnt.most_common())}")
            seen = set()
            shown = 0
            for e in errs:
                key = (e["code"], e["msg"][:60])
                if key in seen:
                    continue
                seen.add(key)
                s = src_lines[e["line"] - 1].strip()[:100] if 0 < e["line"] <= len(src_lines) else ""
                print(f"   {e['code']} L{e['line']}: {e['msg'][:100]}")
                if s:
                    print(f"        | {s}")
                if e.get("suggestion"):
                    print(f"        -> suggestion: {e['suggestion'][:100]}")
                shown += 1
                if shown >= a.n:
                    break

        g = gates(cp, bak)
        bad = [
            k
            for k, v in g.items()
            if (k in ("zig{}", "var", "tuple.@N", "anyopaque") and v) or (k == "ratio_ok" and not v)
        ]
        print(f"   gates: {dict(g)}{'  VIOLATIONS=' + str(bad) if bad else '  OK'}")


if __name__ == "__main__":
    main()
