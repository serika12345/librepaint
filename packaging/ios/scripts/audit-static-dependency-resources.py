#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Check the retained and excluded static resource boundary in an iOS binary."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path
from typing import Any, Mapping


DEFAULT_MANIFEST = (
    Path(__file__).resolve().parents[1] / "manifests/static-dependency-resources.json"
)


class AuditError(RuntimeError):
    """The final binary violates the static resource boundary."""


def resource_symbol_counts(nm_output: str, operation: str) -> Counter:
    pattern = re.compile(r"q" + operation + r"Resources_([A-Za-z0-9_]+)v$")
    result: Counter = Counter()
    for line in nm_output.splitlines():
        match = pattern.search(line)
        if match:
            result[match.group(1)] += 1
    return result


def audit_binary(manifest: Mapping[str, Any], binary: Path, nm_tool: str) -> None:
    if not binary.is_file():
        raise AuditError(f"Mach-O binary does not exist: {binary}")
    try:
        process = subprocess.run(
            [nm_tool, "-j", str(binary)],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except OSError as exc:
        raise AuditError(f"cannot execute {nm_tool}: {exc}") from exc
    if process.returncode != 0:
        raise AuditError(f"nm failed for {binary}: {process.stderr.strip()}")
    contract = manifest["final_binary"]
    for operation, key in (
        ("Init", "expected_qinit_resources"),
        ("Cleanup", "expected_qcleanup_resources"),
    ):
        counts = resource_symbol_counts(process.stdout, operation)
        actual = set(counts)
        expected = set(contract[key])
        duplicates = sorted(name for name, count in counts.items() if count != 1)
        if actual != expected or duplicates:
            raise AuditError(
                f"q{operation}Resources exact set changed; "
                f"missing={sorted(expected - actual)}, extra={sorted(actual - expected)}, "
                f"non_unique={duplicates}"
            )
    for forbidden in contract["forbidden_resource_names"]:
        if re.search(r"q(?:Init|Cleanup)Resources_" + re.escape(forbidden) + r"v$", process.stdout, re.MULTILINE):
            raise AuditError(f"excluded resource symbol remains in final binary: {forbidden}")
    for substring in contract["forbidden_symbol_substrings"]:
        if substring in process.stdout:
            raise AuditError(f"forbidden implementation symbol remains in final binary: {substring}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--nm", default="nm")
    args = parser.parse_args()
    try:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        audit_binary(manifest, args.binary, args.nm)
    except (OSError, ValueError, KeyError, TypeError, AuditError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    print("static resource boundary verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
