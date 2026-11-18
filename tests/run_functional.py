#!/usr/bin/env python3
"""Fast functional test runner for recparse.

Runs a compiled recparse binary over the test cases in the patch-validator
configuration and checks each result against its specification oracle
(exit code, stdout, stderr).  Security reproducers are skipped by default:
judging them is the job of patch-validator, which compares base and patched
builds side by side under sanitizers and a sandbox.

usage:
    python3 tests/run_functional.py --binary build/recparse \
        [--config .patch-validator/recparse.yaml] [--categories valid,boundary,invalid] \
        [--junit results.xml]
"""

from __future__ import annotations

import argparse
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

import yaml


def check_stream(name: str, actual: str, spec: dict | None) -> str | None:
    if not spec:
        return None
    match = spec.get("match")
    value = spec.get("value", "")
    shown = actual if len(actual) <= 120 else actual[:120] + "..."
    if match == "exact" and actual != value:
        return f"{name}: expected {value!r}, got {shown!r}"
    if match == "contains" and value not in actual:
        return f"{name}: expected to contain {value!r}, got {shown!r}"
    if match == "absent" and actual:
        return f"{name}: expected empty, got {shown!r}"
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", required=True, type=Path)
    ap.add_argument("--config", default=".patch-validator/recparse.yaml", type=Path)
    ap.add_argument("--categories", default="valid,boundary,invalid")
    ap.add_argument("--junit", type=Path)
    args = ap.parse_args()

    config = yaml.safe_load(args.config.read_text())
    base_dir = (args.config.parent / config.get("base_dir", ".")).resolve()
    wanted = set(args.categories.split(","))

    suite = ET.Element("testsuite", name="recparse-functional")
    failures = 0
    ran = 0
    for case in config["tests"]:
        if case["category"] not in wanted:
            continue
        ran += 1
        oracle = case["oracle"]
        input_path = base_dir / case["input"]["path"]
        proc = subprocess.run(
            [str(args.binary), str(input_path)],
            capture_output=True,
            timeout=10,
        )
        stdout = proc.stdout.decode(errors="replace")
        stderr = proc.stderr.decode(errors="replace")

        problems = []
        if proc.returncode < 0:
            problems.append(f"killed by signal {-proc.returncode}")
        elif proc.returncode != oracle["exit_code"]:
            problems.append(f"exit code: expected {oracle['exit_code']}, got {proc.returncode}")
        for name, actual in (("stdout", stdout), ("stderr", stderr)):
            problem = check_stream(name, actual, oracle.get(name))
            if problem:
                problems.append(problem)

        tc = ET.SubElement(suite, "testcase", classname=case["category"], name=case["id"])
        if problems:
            failures += 1
            ET.SubElement(tc, "failure", message="; ".join(problems)).text = stderr
            print(f"FAIL  {case['id']:<28} {'; '.join(problems)}")
        else:
            print(f"ok    {case['id']}")

    suite.set("tests", str(ran))
    suite.set("failures", str(failures))
    if args.junit:
        ET.ElementTree(suite).write(args.junit, encoding="utf-8", xml_declaration=True)

    print(f"\n{ran - failures}/{ran} passed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
