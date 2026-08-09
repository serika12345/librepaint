#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Reject the legacy product name in user-visible source metadata."""

from __future__ import annotations

import argparse
import ast
import json
import re
import shlex
import subprocess
import sys
import xml.parsers.expat
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath
from typing import Any, Iterable, Sequence


REPO_ROOT = Path(__file__).resolve().parents[3]
AUDIT_MARKER = "LIBREPAINT-BRAND-AUDIT:"
ALLOWED_REASONS = frozenset({"upstream-attribution", "format-history"})
LEGACY_NAME = re.compile(r"(?<![A-Za-z0-9_])krita(?![A-Za-z0-9_])", re.IGNORECASE)
SOURCE_SUFFIXES = frozenset(
    {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".m", ".mm"}
)
XML_SUFFIXES = frozenset({".ui", ".action", ".xmlgui"})
ACTION_DISPLAY_ELEMENTS = frozenset(
    {"icontext", "statustip", "string", "text", "title", "tooltip", "whatsthis"}
)
JSON_DISPLAY_KEY = re.compile(
    r"^(?:Name|Description|Comment|GenericName)(?:\[[^\]]+\])?$"
)
SKIPPED_DIRECTORY_NAMES = frozenset(
    {
        ".git",
        ".hg",
        ".svn",
        "3rdparty",
        "benchmark",
        "benchmarks",
        "build",
        "build-ios",
        "manual_tests",
        "__pycache__",
        "tests",
    }
)
SUPPRESSION_DISTANCE_LIMIT = 8
SUPPRESSION_LINE = re.compile(
    r"^\s*(?://+|/\*+|\*+|<!--)\s*"
    + re.escape(AUDIT_MARKER)
    + r"\s*([A-Za-z0-9_-]+)\s*(?:\*/|-->)?\s*$"
)
MARKER_VALUE = re.compile(
    r"^" + re.escape(AUDIT_MARKER) + r"\s*([A-Za-z0-9_-]+)\s*$"
)


class AuditError(RuntimeError):
    """The audit could not inspect its inputs."""


@dataclass(order=True, frozen=True)
class Diagnostic:
    path: str
    line: int
    message: str


@dataclass
class Suppression:
    path: str
    line: int
    reason: str
    consumed: bool = False


@dataclass(frozen=True)
class Reference:
    path: str
    line: int


@dataclass
class PotEntry:
    msgid: str = ""
    msgid_plural: str = ""
    references: list[Reference] = field(default_factory=list)

    @property
    def visible_text(self) -> str:
        return "\n".join(part for part in (self.msgid, self.msgid_plural) if part)


@dataclass(frozen=True)
class DisplayText:
    path: str
    line: int
    text: str
    source: str
    inline_suppression: Suppression | None = None


@dataclass
class AuditStats:
    source_files: int = 0
    gettext_messages: int = 0
    xml_files: int = 0
    xml_messages: int = 0
    plugin_json_files: int = 0
    plugin_json_messages: int = 0
    allowed_suppressions: int = 0


def relative_name(root: Path, path: Path) -> str:
    return path.relative_to(root).as_posix()


def should_skip(relative: PurePosixPath) -> bool:
    parts = relative.parts
    if any(part in SKIPPED_DIRECTORY_NAMES for part in parts[:-1]):
        return True
    if any(part.startswith("cmake-build-") for part in parts[:-1]):
        return True
    if parts[:2] == ("sdk", "templates"):
        return True
    if parts[:2] == ("krita", "crashreporter"):
        return True
    return relative.as_posix() == "rc.cpp"


def discover_files(root: Path, suffixes: frozenset[str]) -> list[Path]:
    paths: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in suffixes:
            continue
        relative = PurePosixPath(relative_name(root, path))
        if not should_skip(relative):
            paths.append(path)
    return sorted(paths, key=lambda item: relative_name(root, item))


def discover_plugin_json(root: Path) -> list[Path]:
    plugin_root = root / "plugins"
    if not plugin_root.is_dir():
        return []
    return [
        path
        for path in discover_files(root, frozenset({".json"}))
        if path.is_relative_to(plugin_root)
    ]


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as exc:
        raise AuditError(f"cannot read {path}: {exc}") from exc


def collect_suppressions(
    root: Path,
    paths: Iterable[Path],
    diagnostics: list[Diagnostic],
) -> dict[str, list[Suppression]]:
    suppressions: dict[str, list[Suppression]] = {}
    for path in paths:
        name = relative_name(root, path)
        lines = read_text(path).splitlines()
        for line_number, line in enumerate(lines, 1):
            if AUDIT_MARKER not in line:
                continue
            match = SUPPRESSION_LINE.match(line)
            if not match:
                diagnostics.append(
                    Diagnostic(name, line_number, "malformed branding suppression")
                )
                continue
            reason = match.group(1)
            if reason not in ALLOWED_REASONS:
                diagnostics.append(
                    Diagnostic(
                        name,
                        line_number,
                        f"unknown branding suppression reason: {reason}",
                    )
                )
                continue
            suppressions.setdefault(name, []).append(
                Suppression(name, line_number, reason)
            )
    return suppressions


def claim_suppression(
    path: str,
    line: int,
    suppressions: dict[str, list[Suppression]],
) -> Suppression | None:
    candidates = suppressions.get(path, [])
    for suppression in reversed(candidates):
        if suppression.consumed or suppression.line > line:
            continue
        if line - suppression.line > SUPPRESSION_DISTANCE_LIMIT:
            break
        # xgettext reports the first msgid literal, which may follow the i18n
        # call itself by a few lines. Consume only the closest marker once.
        suppression.consumed = True
        return suppression
    return None


def po_string(fragment: str) -> str:
    try:
        value = ast.literal_eval(fragment)
    except (SyntaxError, ValueError) as exc:
        raise AuditError(f"invalid quoted string in xgettext output: {fragment}") from exc
    if not isinstance(value, str):
        raise AuditError(f"non-string value in xgettext output: {fragment}")
    return value


def parse_references(line: str) -> list[Reference]:
    references: list[Reference] = []
    try:
        tokens = shlex.split(line[2:].strip())
    except ValueError as exc:
        raise AuditError(f"invalid source reference in xgettext output: {line}") from exc
    for token in tokens:
        match = re.match(r"^(.*):(\d+)$", token)
        if not match:
            continue
        path = PurePosixPath(match.group(1)).as_posix()
        while path.startswith("./"):
            path = path[2:]
        references.append(Reference(path, int(match.group(2))))
    return references


def parse_pot(text: str) -> list[PotEntry]:
    entries: list[PotEntry] = []
    entry = PotEntry()
    active_field = ""

    def finish() -> None:
        nonlocal entry, active_field
        if entry.msgid or entry.msgid_plural or entry.references:
            entries.append(entry)
        entry = PotEntry()
        active_field = ""

    for raw_line in text.splitlines() + [""]:
        line = raw_line.rstrip("\r")
        if not line:
            finish()
            continue
        if line.startswith("#:"):
            entry.references.extend(parse_references(line))
            continue
        if line.startswith("msgid_plural "):
            active_field = "msgid_plural"
            entry.msgid_plural = po_string(line[len("msgid_plural ") :])
            continue
        if line.startswith("msgid "):
            active_field = "msgid"
            entry.msgid = po_string(line[len("msgid ") :])
            continue
        if line.startswith('"') and active_field:
            setattr(entry, active_field, getattr(entry, active_field) + po_string(line))
            continue
        if not line.startswith("#"):
            active_field = ""
    return [entry for entry in entries if entry.visible_text]


XGETTEXT_KEYWORDS = (
    "i18n:1",
    "i18nc:1c,2",
    "i18np:1,2",
    "i18ncp:1c,2,3",
    "i18nd:2",
    "i18ndc:2c,3",
    "i18ndp:2,3",
    "i18ndcp:2c,3,4",
    "ki18n:1",
    "ki18nc:1c,2",
    "ki18np:1,2",
    "ki18ncp:1c,2,3",
    "kundo2_i18n:1",
    "kundo2_i18nc:1c,2",
    "kundo2_i18np:1,2",
    "kundo2_i18ncp:1c,2,3",
    "xi18n:1",
    "xi18nc:1c,2",
    "xi18np:1,2",
    "xi18ncp:1c,2,3",
    "kxi18n:1",
    "kxi18nc:1c,2",
    "I18N_NOOP:1",
    "I18NC_NOOP:1c,2",
    "I18N_NOOP2:1c,2",
    "QT_TR_NOOP:1",
    "QT_TRANSLATE_NOOP:1c,2",
    "tr:1",
    "trUtf8:1",
)


def run_xgettext(root: Path, source_files: Sequence[Path], executable: str) -> list[PotEntry]:
    if not source_files:
        return []
    relative_paths = [relative_name(root, path) for path in source_files]
    command = [
        executable,
        "--language=C++",
        "--from-code=UTF-8",
        "--force-po",
        "--no-wrap",
        f"--add-comments={AUDIT_MARKER}",
        "--output=-",
        "--files-from=-",
        "--keyword",
        *(f"--keyword={keyword}" for keyword in XGETTEXT_KEYWORDS),
    ]
    try:
        completed = subprocess.run(
            command,
            cwd=root,
            input="\n".join(relative_paths) + "\n",
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
    except OSError as exc:
        raise AuditError(f"cannot execute xgettext {executable!r}: {exc}") from exc
    if completed.returncode != 0:
        detail = completed.stderr.strip() or f"exit status {completed.returncode}"
        raise AuditError(f"xgettext failed: {detail}")
    return parse_pot(completed.stdout)


def marker_from_value(
    path: str,
    line: int,
    value: str,
    diagnostics: list[Diagnostic],
) -> Suppression | None:
    if AUDIT_MARKER not in value:
        return None
    match = MARKER_VALUE.match(value.strip())
    if not match:
        diagnostics.append(Diagnostic(path, line, "malformed branding suppression"))
        return None
    reason = match.group(1)
    if reason not in ALLOWED_REASONS:
        diagnostics.append(
            Diagnostic(path, line, f"unknown branding suppression reason: {reason}")
        )
        return None
    return Suppression(path, line, reason)


def xml_display_texts(
    root: Path, path: Path
) -> tuple[list[DisplayText], list[Diagnostic]]:
    name = relative_name(root, path)
    wanted = frozenset({"string"}) if path.suffix.lower() == ".ui" else ACTION_DISPLAY_ELEMENTS
    results: list[DisplayText] = []
    diagnostics: list[Diagnostic] = []
    active: list[dict[str, Any]] = []
    depth = 0
    parser = xml.parsers.expat.ParserCreate()

    def start_element(element: str, attributes: dict[str, str]) -> None:
        nonlocal depth
        local_name = element.rsplit(":", 1)[-1].lower()
        not_translatable = any(
            key.rsplit(":", 1)[-1].lower() == "notr"
            and value.strip().lower() in {"1", "true", "yes"}
            for key, value in attributes.items()
        )
        extra_comment = next(
            (
                value
                for key, value in attributes.items()
                if key.rsplit(":", 1)[-1].lower() == "extracomment"
            ),
            "",
        )
        inline_suppression = marker_from_value(
            name, parser.CurrentLineNumber, extra_comment, diagnostics
        )
        if local_name in wanted and not not_translatable:
            active.append(
                {
                    "depth": depth,
                    "line": parser.CurrentLineNumber,
                    "parts": [],
                    "element": local_name,
                    "suppression": inline_suppression,
                }
            )
        elif inline_suppression is not None:
            diagnostics.append(
                Diagnostic(
                    name,
                    parser.CurrentLineNumber,
                    f"stale branding suppression ({inline_suppression.reason})",
                )
            )
        depth += 1

    def characters(data: str) -> None:
        for record in active:
            record["parts"].append(data)

    def end_element(_element: str) -> None:
        nonlocal depth
        depth -= 1
        if active and active[-1]["depth"] == depth:
            record = active.pop()
            results.append(
                DisplayText(
                    name,
                    int(record["line"]),
                    "".join(record["parts"]),
                    f"<{record['element']}>",
                    record["suppression"],
                )
            )

    parser.StartElementHandler = start_element
    parser.CharacterDataHandler = characters
    parser.EndElementHandler = end_element
    try:
        parser.Parse(path.read_bytes(), True)
    except (OSError, xml.parsers.expat.ExpatError) as exc:
        line = getattr(exc, "lineno", parser.CurrentLineNumber or 1)
        raise AuditError(f"{name}:{line}: cannot parse XML: {exc}") from exc
    return results, diagnostics


def is_plugin_metadata(value: Any) -> bool:
    if not isinstance(value, dict):
        return False
    return (
        "Id" in value
        or "KPlugin" in value
        or "X-KDE-ServiceTypes" in value
        or any(key.startswith("X-Krita-") for key in value)
    )


def string_leaves(value: Any) -> list[str]:
    if isinstance(value, str):
        return [value]
    if isinstance(value, dict):
        return [item for child in value.values() for item in string_leaves(child)]
    if isinstance(value, list):
        return [item for child in value for item in string_leaves(child)]
    return []


def plugin_json_display_texts(root: Path, path: Path) -> list[DisplayText]:
    name = relative_name(root, path)
    text = read_text(path)
    try:
        value = json.loads(text)
    except json.JSONDecodeError as exc:
        raise AuditError(f"{name}:{exc.lineno}: cannot parse plugin JSON: {exc.msg}") from exc
    if not is_plugin_metadata(value):
        return []

    key_lines: dict[str, list[int]] = {}
    for match in re.finditer(r'"((?:\\.|[^"\\])*)"\s*:', text):
        try:
            key = json.loads(f'"{match.group(1)}"')
        except json.JSONDecodeError:
            continue
        if isinstance(key, str) and JSON_DISPLAY_KEY.match(key):
            key_lines.setdefault(key, []).append(text.count("\n", 0, match.start()) + 1)
    key_offsets: dict[str, int] = {}
    results: list[DisplayText] = []

    def walk(node: Any) -> None:
        if isinstance(node, dict):
            for key, child in node.items():
                if isinstance(key, str) and JSON_DISPLAY_KEY.match(key):
                    offset = key_offsets.get(key, 0)
                    locations = key_lines.get(key, [])
                    line = locations[offset] if offset < len(locations) else 1
                    key_offsets[key] = offset + 1
                    for visible in string_leaves(child):
                        results.append(DisplayText(name, line, visible, f"JSON {key}"))
                walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(value)
    return results


def inspect_display_text(
    item: DisplayText,
    suppressions: dict[str, list[Suppression]],
    diagnostics: list[Diagnostic],
    stats: AuditStats,
) -> None:
    suppression = item.inline_suppression or claim_suppression(
        item.path, item.line, suppressions
    )
    if LEGACY_NAME.search(item.text):
        if suppression is None:
            diagnostics.append(
                Diagnostic(
                    item.path,
                    item.line,
                    f"legacy product name in user-visible {item.source}",
                )
            )
        else:
            stats.allowed_suppressions += 1
    elif suppression is not None:
        diagnostics.append(
            Diagnostic(
                suppression.path,
                suppression.line,
                f"stale branding suppression ({suppression.reason})",
            )
        )


def audit(source_root: Path, xgettext: str) -> tuple[list[Diagnostic], AuditStats]:
    root = source_root.resolve()
    if not root.is_dir():
        raise AuditError(f"source root is not a directory: {root}")

    source_files = discover_files(root, SOURCE_SUFFIXES)
    xml_files = discover_files(root, XML_SUFFIXES)
    plugin_json_files = discover_plugin_json(root)
    diagnostics: list[Diagnostic] = []
    suppressions = collect_suppressions(root, source_files, diagnostics)
    stats = AuditStats(
        source_files=len(source_files),
        xml_files=len(xml_files),
        plugin_json_files=len(plugin_json_files),
    )

    entries = run_xgettext(root, source_files, xgettext)
    stats.gettext_messages = len(entries)
    for entry in entries:
        has_legacy_name = bool(LEGACY_NAME.search(entry.visible_text))
        if not entry.references:
            if has_legacy_name:
                diagnostics.append(
                    Diagnostic("<xgettext>", 0, "legacy product name in an unlocated msgid")
                )
            continue
        for reference in entry.references:
            item = DisplayText(reference.path, reference.line, entry.visible_text, "msgid")
            inspect_display_text(item, suppressions, diagnostics, stats)

    for path in xml_files:
        messages, xml_diagnostics = xml_display_texts(root, path)
        diagnostics.extend(xml_diagnostics)
        stats.xml_messages += len(messages)
        for item in messages:
            inspect_display_text(item, suppressions, diagnostics, stats)

    for path in plugin_json_files:
        messages = plugin_json_display_texts(root, path)
        stats.plugin_json_messages += len(messages)
        for item in messages:
            # JSON has no comments; display metadata must use the current name.
            if LEGACY_NAME.search(item.text):
                diagnostics.append(
                    Diagnostic(
                        item.path,
                        item.line,
                        f"legacy product name in user-visible {item.source}",
                    )
                )

    for records in suppressions.values():
        for suppression in records:
            if not suppression.consumed:
                diagnostics.append(
                    Diagnostic(
                        suppression.path,
                        suppression.line,
                        f"stale branding suppression ({suppression.reason})",
                    )
                )

    return sorted(set(diagnostics)), stats


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root",
        type=Path,
        default=REPO_ROOT,
        help="source tree to audit (default: repository root)",
    )
    parser.add_argument(
        "--xgettext",
        default="xgettext",
        help="xgettext executable (default: %(default)s)",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    try:
        diagnostics, stats = audit(args.source_root, args.xgettext)
    except AuditError as exc:
        message = str(exc)
        if re.match(r"^.+:\d+:", message):
            print(f"{message}: error: audit failed", file=sys.stderr)
        else:
            print(f"<branding-audit>:0: error: {message}", file=sys.stderr)
        return 1

    if diagnostics:
        for diagnostic in diagnostics:
            print(
                f"{diagnostic.path}:{diagnostic.line}: error: {diagnostic.message}",
                file=sys.stderr,
            )
        print(
            f"user-visible branding audit failed: {len(diagnostics)} issue(s)",
            file=sys.stderr,
        )
        return 1

    print(
        "user-visible branding audit: "
        f"{stats.gettext_messages} gettext messages, "
        f"{stats.xml_messages} XML messages, "
        f"{stats.plugin_json_messages} plugin metadata strings; "
        f"{stats.allowed_suppressions} justified legacy reference(s)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
