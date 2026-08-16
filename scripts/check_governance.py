#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
import unicodedata
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
BASELINE_PATH = Path("docs/architecture/source-size-baseline.json")
PRODUCTION_ROOTS = ("krita", "libs", "plugins", "qmlmodules")
PRODUCTION_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".mm"}
AUXILIARY_SOURCE_PARTS = {
    "3rdparty",
    "3rdparty_vendor",
    "autotest",
    "autotests",
    "test",
    "tests",
}
TEXT_SUFFIXES = {
    ".c",
    ".cc",
    ".cmake",
    ".cpp",
    ".css",
    ".cxx",
    ".d2",
    ".desktop",
    ".h",
    ".hh",
    ".hpp",
    ".html",
    ".in",
    ".json",
    ".md",
    ".mm",
    ".nix",
    ".plist",
    ".py",
    ".qml",
    ".qrc",
    ".sh",
    ".txt",
    ".xml",
    ".yaml",
    ".yml",
    ".zsh",
}
TEXT_FILENAMES = {
    ".clang-format",
    ".clang-tidy",
    ".editorconfig",
    ".envrc",
    ".gitattributes",
    ".gitignore",
    ".markdownlint-cli2.yaml",
    "CMakeLists.txt",
    "CMakePresets.json",
}


@dataclass(frozen=True)
class SourceSizeBaseline:
    threshold: int
    legacy_files: dict[str, int]
    exceptions: dict[str, dict[str, object]]


def repository_files(root: Path) -> list[str]:
    generated_top_level = {
        ".git",
        ".cache",
        "build",
        "build-ios",
        "logs",
        "result",
    }
    files: list[str] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        relative_path = path.relative_to(root)
        if relative_path.parts[0] in generated_top_level:
            continue
        if relative_path.parts[0].startswith("result-"):
            continue
        files.append(relative_path.as_posix())
    return sorted(files)


def is_text_file(relative_path: str) -> bool:
    path = Path(relative_path)
    return path.name in TEXT_FILENAMES or path.suffix.lower() in TEXT_SUFFIXES


def text_contract_finding(
    character: str,
    line_number: int,
    column: int,
) -> str | None:
    codepoint = ord(character)
    if codepoint in {0x09, 0x0A, 0x0D}:
        return None
    if codepoint <= 0x1F or codepoint == 0x7F:
        return f"control character U+{codepoint:04X}"
    if codepoint == 0xFEFF and line_number == 1 and column == 1:
        return None
    if codepoint in {0x200C, 0x200D, 0x200E, 0x200F}:
        return None
    if unicodedata.category(character) == "Cf":
        name = unicodedata.name(character, "unnamed format character")
        return f"format character U+{codepoint:04X} {name}"
    return None


def check_text_contract(root: Path, files: Iterable[str]) -> list[str]:
    findings: list[str] = []
    for relative_path in files:
        if not is_text_file(relative_path):
            continue
        path = root / relative_path
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            findings.append(f"{relative_path}: text contract requires UTF-8 encoding")
            continue

        for line_number, line in enumerate(text.splitlines(keepends=True), start=1):
            for column, character in enumerate(line, start=1):
                description = text_contract_finding(
                    character,
                    line_number,
                    column,
                )
                if description is not None:
                    findings.append(
                        f"{relative_path}:{line_number}:{column}: {description}"
                    )
    return findings


def is_production_source(relative_path: str) -> bool:
    path = Path(relative_path)
    if len(path.parts) < 2 or path.parts[0] not in PRODUCTION_ROOTS:
        return False
    if path.suffix.lower() not in PRODUCTION_SUFFIXES:
        return False
    return not any(part in AUXILIARY_SOURCE_PARTS for part in path.parts)


def source_line_counts(root: Path, files: Iterable[str]) -> dict[str, int]:
    counts: dict[str, int] = {}
    for relative_path in files:
        if not is_production_source(relative_path):
            continue
        text = (root / relative_path).read_text(encoding="utf-8")
        counts[relative_path] = len(text.splitlines())
    return counts


def load_source_size_baseline(root: Path) -> SourceSizeBaseline:
    path = root / BASELINE_PATH
    raw = json.loads(path.read_text(encoding="utf-8"))
    if raw.get("schemaVersion") != 1:
        raise ValueError(f"{BASELINE_PATH}: schemaVersion must be 1")

    threshold = raw.get("threshold")
    legacy_files = raw.get("legacyFiles")
    exceptions = raw.get("exceptions")
    if not isinstance(threshold, int) or threshold <= 0:
        raise ValueError(f"{BASELINE_PATH}: threshold must be a positive integer")
    if not isinstance(legacy_files, dict):
        raise ValueError(f"{BASELINE_PATH}: legacyFiles must be an object")
    if not isinstance(exceptions, dict):
        raise ValueError(f"{BASELINE_PATH}: exceptions must be an object")
    if not all(isinstance(path, str) and isinstance(lines, int) for path, lines in legacy_files.items()):
        raise ValueError(f"{BASELINE_PATH}: legacyFiles must map paths to line counts")

    return SourceSizeBaseline(
        threshold=threshold,
        legacy_files=dict(sorted(legacy_files.items())),
        exceptions=dict(sorted(exceptions.items())),
    )


def validate_exception(relative_path: str, value: object) -> list[str]:
    findings: list[str] = []
    if not isinstance(value, dict):
        return [f"{relative_path}: source-size exception must be an object"]

    maximum = value.get("maxLines")
    if not isinstance(maximum, int) or maximum <= 0:
        findings.append(f"{relative_path}: exception maxLines requires a positive integer")
    for field in ("reason", "todo", "removalCondition"):
        field_value = value.get(field)
        if not isinstance(field_value, str) or not field_value.strip():
            findings.append(f"{relative_path}: exception {field} requires text")
    return findings


def check_source_size_contract(
    counts: dict[str, int], baseline: SourceSizeBaseline
) -> list[str]:
    findings: list[str] = []

    for relative_path, maximum in baseline.legacy_files.items():
        current = counts.get(relative_path)
        if current is None:
            findings.append(f"{relative_path}: baseline entry requires a current production source")
        elif current <= baseline.threshold:
            findings.append(
                f"{relative_path}: baseline maintenance removes entries at {current} lines"
            )
        elif current > maximum:
            findings.append(
                f"{relative_path}: source-size maximum is {maximum}; measured {current} lines"
            )

    for relative_path, value in baseline.exceptions.items():
        findings.extend(validate_exception(relative_path, value))
        if not isinstance(value, dict) or not isinstance(value.get("maxLines"), int):
            continue
        current = counts.get(relative_path)
        maximum = value["maxLines"]
        if current is None:
            findings.append(f"{relative_path}: exception requires a current production source")
        elif current <= baseline.threshold:
            findings.append(
                f"{relative_path}: exception maintenance removes entries at {current} lines"
            )
        elif current > maximum:
            findings.append(
                f"{relative_path}: exception maximum is {maximum}; measured {current} lines"
            )

    accounted_paths = set(baseline.legacy_files) | set(baseline.exceptions)
    for relative_path, current in sorted(counts.items()):
        if current > baseline.threshold and relative_path not in accounted_paths:
            findings.append(
                f"{relative_path}: {current} lines require a baseline entry or reviewed exception; "
                f"standard maximum is {baseline.threshold}"
            )

    return findings


def write_source_size_baseline(root: Path, files: Iterable[str]) -> None:
    baseline_path = root / BASELINE_PATH
    threshold = 1000
    existing_exceptions: dict[str, object] = {}
    if baseline_path.exists():
        raw = json.loads(baseline_path.read_text(encoding="utf-8"))
        candidate = raw.get("exceptions", {})
        if isinstance(candidate, dict):
            existing_exceptions = candidate

    counts = source_line_counts(root, files)
    legacy_files = {
        relative_path: line_count
        for relative_path, line_count in sorted(counts.items())
        if line_count > threshold and relative_path not in existing_exceptions
    }
    document = {
        "schemaVersion": 1,
        "threshold": threshold,
        "trackingTodo": "docs/architecture/TODO.md#r1-コードパッケージングの改善",
        "policy": (
            "Legacy entries record the G0 maximum. R1 and R6 reduce each "
            "maximum as responsibilities become smaller."
        ),
        "legacyFiles": legacy_files,
        "exceptions": existing_exceptions,
    }
    baseline_path.write_text(
        json.dumps(document, ensure_ascii=False, indent=2, sort_keys=False) + "\n",
        encoding="utf-8",
    )
    print(f"updated {BASELINE_PATH} with {len(legacy_files)} legacy files")


def collect_findings(root: Path, files: list[str]) -> list[str]:
    findings = check_text_contract(root, files)
    try:
        baseline = load_source_size_baseline(root)
    except (FileNotFoundError, json.JSONDecodeError, ValueError) as error:
        findings.append(str(error))
        return findings
    findings.extend(check_source_size_contract(source_line_counts(root, files), baseline))
    return findings


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Verify LibrePaint repository governance")
    parser.add_argument(
        "--root",
        type=Path,
        default=REPOSITORY_ROOT,
        help="repository root used by tests and Nix checks",
    )
    parser.add_argument(
        "--update-source-size-baseline",
        action="store_true",
        help="replace the legacy source-size baseline with the current measured state",
    )
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    root = arguments.root.resolve()
    files = repository_files(root)

    if arguments.update_source_size_baseline:
        write_source_size_baseline(root, files)
        return 0

    findings = collect_findings(root, files)
    if findings:
        print("Governance contract findings:", file=sys.stderr)
        for finding in findings:
            print(f"- {finding}", file=sys.stderr)
        return 1

    print("check-governance: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
