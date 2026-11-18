#!/usr/bin/env python3
# scripts/bundle_runtime.py
# Concatenates modular JS files in src/js/shim/ into src/js/runtime.js,
# validates syntax with node -c, and generates src/js/runtime_source.cp
# with chunked constants to prevent compiler stack recursion.

import os
import subprocess
import sys

MODULE_ORDER = [
    "constants.js",
    "util/strings.js",
    "util/validation.js",
    "events/event.js",
    "events/dispatch.js",
    "mutations/observer.js",
    "collections.js",
    "node-base.js",
    "element/element.js",
    "element/html-element.js",
    "element/html-classes.js",
    "element/forms/validity.js",
    "element/forms/input.js",
    "element/forms/textarea.js",
    "element/forms/select.js",
    "element/forms/button.js",
    "element/forms/form.js",
    "file.js",
    "form-data.js",
    "traversal.js",
    "range.js",
    "parser.js",
    "xpath.js",
    "canvas2d.js",
    "animations.js",
    "window.js",
    "index.js",
]

CHUNK_SIZE = 12000

def main():
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    shim_dir = os.path.join(repo_root, "src", "js", "shim")
    runtime_js_path = os.path.join(repo_root, "src", "js", "runtime.js")
    runtime_source_cp_path = os.path.join(repo_root, "src", "js", "runtime_source.cp")

    print(f"[*] Bundling {len(MODULE_ORDER)} modules from {shim_dir}...")

    parts = [
        "// src/js/runtime.js\n"
        "// CapSurf High-Performance Native Web API Runtime\n"
        "// Auto-bundled from src/js/shim/ (25 modules) - DO NOT EDIT BY HAND\n\n"
        "(function (global) {\n"
        '  "use strict";\n\n'
    ]

    for rel_path in MODULE_ORDER:
        full_path = os.path.join(shim_dir, rel_path)
        if not os.path.isfile(full_path):
            print(f"[!] Error: missing module file: {full_path}", file=sys.stderr)
            sys.exit(1)
        with open(full_path, "r", encoding="utf-8") as f:
            content = f.read()
        parts.append(f"  // =============================================================\n")
        parts.append(f"  // Module: {rel_path}\n")
        parts.append(f"  // =============================================================\n")
        indented = "\n".join("  " + line if line.strip() else "" for line in content.splitlines())
        parts.append(indented + "\n\n")

    parts.append("})(typeof window !== \"undefined\" ? window : globalThis);\n")

    bundle_code = "".join(parts)

    with open(runtime_js_path, "w", encoding="utf-8") as f:
        f.write(bundle_code)
    print(f"[+] Written {len(bundle_code)} bytes to {runtime_js_path}")

    # Validate syntax with node -c
    res = subprocess.run(["node", "-c", runtime_js_path], capture_output=True, text=True)
    if res.returncode != 0:
        print(f"[!] Syntax error in bundled JS:\n{res.stderr}", file=sys.stderr)
        sys.exit(1)
    print(f"[+] node -c validation PASSED.")

    # Split into chunks of CHUNK_SIZE characters
    chunks = [bundle_code[i:i + CHUNK_SIZE] for i in range(0, len(bundle_code), CHUNK_SIZE)]

    cp_parts = [
        "// src/js/runtime_source.cp\n",
        "// Auto-generated chunked source from src/js/runtime.js - DO NOT EDIT BY HAND\n\n",
        f"pub const RUNTIME_CHUNKS: [{len(chunks)}][]const u8 = [\n",
    ]

    for chunk in chunks:
        esc = chunk.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n").replace("\r", "")
        cp_parts.append(f'    "{esc}",\n')

    cp_parts.append("];\n")
    cp_code = "".join(cp_parts)

    with open(runtime_source_cp_path, "w", encoding="utf-8") as f:
        f.write(cp_code)
    print(f"[+] Generated {len(chunks)} chunks ({len(cp_code)} bytes) in {runtime_source_cp_path}")

if __name__ == "__main__":
    main()
