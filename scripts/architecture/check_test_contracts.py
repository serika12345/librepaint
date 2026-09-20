#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from typing import Iterable


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
TEST_SOURCE_SUFFIXES = {".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp"}
COMPATIBILITY_REQUIREMENT = re.compile(
    r"^\s*//\s*Compatibility requirement:\s*\S.*$", re.MULTILINE
)
STRUCTURAL_PATTERNS = (
    (
        re.compile(r"#\s*include\s*<type_traits>"),
        "type-trait header in a contract test",
    ),
    (
        re.compile(r"\bstd::is_[A-Za-z0-9_]+"),
        "type-trait assertion in a contract test",
    ),
    (
        re.compile(r"\b(?:static_assert|Q_STATIC_ASSERT)\s*\("),
        "compile-time shape assertion in a contract test",
    ),
    (
        re.compile(r"\bdecltype\s*\(\s*&[A-Za-z_][A-Za-z0-9_:]*::[A-Za-z_]"),
        "exact member signature in a contract test",
    ),
    (
        re.compile(r"\busing\s+[A-Za-z_][A-Za-z0-9_]*Signature\s*="),
        "signature alias in a contract test",
    ),
    (
        re.compile(
            r"\busing\s+[A-Za-z_][A-Za-z0-9_]*\s*=\s*[^;]{0,500}"
            r"\(\s*[A-Za-z_][A-Za-z0-9_:]*::\s*\*",
            re.DOTALL,
        ),
        "member-function pointer alias in a contract test",
    ),
)


def discover_test_sources(root: Path) -> list[Path]:
    sources: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in TEST_SOURCE_SUFFIXES:
            continue
        relative = path.relative_to(root)
        if "tests" not in relative.parts:
            continue
        if any(part in {".git", "build", "build-ios"} for part in relative.parts):
            continue
        sources.append(path)
    return sorted(sources)


def is_contract_test(path: Path) -> bool:
    return path.stem.endswith("ContractTest") or path.stem.endswith("_contract_test")


def is_compatibility_test(path: Path) -> bool:
    return path.stem.endswith("CompatibilityTest") or path.stem.endswith(
        "_compatibility_test"
    )


def _line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def check_test_source(root: Path, path: Path) -> list[str]:
    relative = path.relative_to(root).as_posix()
    try:
        source = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return [f"{relative}: test contract source must be UTF-8"]

    findings: list[str] = []
    if is_contract_test(path):
        for pattern, description in STRUCTURAL_PATTERNS:
            for match in pattern.finditer(source):
                findings.append(
                    f"{relative}:{_line_number(source, match.start())}: {description}; "
                    "verify an observable result instead"
                )

    if is_compatibility_test(path) and not COMPATIBILITY_REQUIREMENT.search(source):
        findings.append(
            f"{relative}: compatibility test must contain a "
            "'// Compatibility requirement:' line naming the consumer and stable property"
        )

    return findings


def collect_findings(root: Path, sources: Iterable[Path] | None = None) -> list[str]:
    checked_sources = discover_test_sources(root) if sources is None else sources
    findings: list[str] = []
    for path in checked_sources:
        findings.extend(check_test_source(root, path))
    return findings


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Reject declaration-shape assertions from behavioral contract tests."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=REPOSITORY_ROOT,
        help="repository root used by the contract check",
    )
    return parser


def main() -> int:
    root = _argument_parser().parse_args().root.resolve()
    findings = collect_findings(root)
    if findings:
        print("Test contract findings:", file=sys.stderr)
        for finding in findings:
            print(f"- {finding}", file=sys.stderr)
        return 1
    print("test contracts verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
