#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
import unicodedata
from pathlib import Path
from typing import Iterable


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
TEXT_SUFFIXES = {
    ".c", ".cc", ".cmake", ".cpp", ".css", ".cxx", ".d2", ".desktop",
    ".h", ".hh", ".hpp", ".html", ".in", ".json", ".md", ".mm",
    ".nix", ".plist", ".py", ".qml", ".qrc", ".sh", ".txt", ".xml",
    ".yaml", ".yml", ".zsh",
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


def repository_files(root: Path) -> list[str]:
    ignored_top_level = {
        ".cache", ".direnv", ".git", "build", "build-ios", "logs", "result"
    }
    files: list[str] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        relative_path = path.relative_to(root)
        if relative_path.parts[0] in ignored_top_level:
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
                description = text_contract_finding(character, line_number, column)
                if description is not None:
                    findings.append(
                        f"{relative_path}:{line_number}:{column}: {description}"
                    )
    return findings


def collect_findings(root: Path, files: list[str]) -> list[str]:
    return check_text_contract(root, files)


def _argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Verify LibrePaint text representation."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=REPOSITORY_ROOT,
        help="repository root used by tests and Nix checks",
    )
    return parser


def main() -> int:
    root = _argument_parser().parse_args().root.resolve()
    findings = collect_findings(root, repository_files(root))
    if findings:
        print("Text contract findings:", file=sys.stderr)
        for finding in findings:
            print(f"- {finding}", file=sys.stderr)
        return 1
    print("check-governance: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
