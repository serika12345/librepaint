#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Audit persistent iOS document, plugin, action, and configuration identifiers."""

from __future__ import annotations

import argparse
import hashlib
import json
import plistlib
import re
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Any, Callable, Iterable, Sequence


REPO_ROOT = Path(__file__).resolve().parents[3]
MANIFEST_RELATIVE_PATH = Path("packaging/ios/manifests/compatibility-identifiers.json")
SCHEMA = 1
GENERATED_BY = "packaging/ios/scripts/audit-ios-compatibility-identifiers.py"
POLICY = (
    "Persistent format, application, plugin, action, configuration, and resource "
    "identifiers retained across the LibrePaint iPadOS rename. Display names and "
    "translated text are intentionally excluded."
)
CPP_SUFFIXES = frozenset(
    {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".m", ".mm"}
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
        "test",
        "tests",
    }
)
CONFIG_IDENTIFIERS = frozenset(
    {"kritarc", "/kritarc", "kritadisplayrc", "/kritadisplayrc"}
)
RESOURCE_PATH = re.compile(r"(?:^|/)share/krita(?:/|$)")
EXPLICIT_CATEGORIES = (
    "document_identifiers",
    "application_identity",
    "configuration_files",
    "resource_paths",
)
INVENTORY_CATEGORIES = ("plugin_metadata", "action_ids")
PLUGIN_STABLE_KEYS = frozenset({"Id", "X-KDE-ServiceTypes"})
KRA_PLUGIN_METADATA = (
    "plugins/impex/kra/krita_kra_export.json",
    "plugins/impex/kra/krita_kra_import.json",
    "plugins/impex/krz/krita_krz_export.json",
)


class AuditError(RuntimeError):
    """The audit input or manifest is invalid."""


@dataclass(frozen=True)
class CppToken:
    kind: str
    value: str


@dataclass(frozen=True)
class Inventory:
    records: tuple[dict[str, Any], ...]
    file_count: int


def relative_name(root: Path, path: Path) -> str:
    return path.relative_to(root).as_posix()


def should_skip(relative: PurePosixPath) -> bool:
    directories = relative.parts[:-1]
    if any(part in SKIPPED_DIRECTORY_NAMES for part in directories):
        return True
    return any(part.startswith("cmake-build-") for part in directories)


def discover_files(root: Path, suffixes: frozenset[str]) -> list[Path]:
    result: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in suffixes:
            continue
        relative = PurePosixPath(relative_name(root, path))
        if not should_skip(relative):
            result.append(path)
    return sorted(result, key=lambda item: relative_name(root, item))


def require_file(root: Path, relative: str) -> Path:
    path = root / relative
    if not path.is_file():
        raise AuditError(f"required compatibility source is missing: {relative}")
    return path


def read_text(path: Path, root: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as exc:
        raise AuditError(f"cannot read {relative_name(root, path)}: {exc}") from exc


def local_name(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def parse_xml(path: Path, root: Path) -> ET.Element:
    try:
        return ET.parse(path).getroot()
    except (OSError, ET.ParseError) as exc:
        raise AuditError(f"cannot parse XML {relative_name(root, path)}: {exc}") from exc


def parse_json(path: Path, root: Path) -> Any:
    try:
        return json.loads(read_text(path, root))
    except json.JSONDecodeError as exc:
        raise AuditError(
            f"cannot parse JSON {relative_name(root, path)}:{exc.lineno}: {exc.msg}"
        ) from exc


def canonical_record(record: dict[str, Any]) -> str:
    return json.dumps(record, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def sort_records(records: Iterable[dict[str, Any]]) -> tuple[dict[str, Any], ...]:
    return tuple(sorted(records, key=canonical_record))


def records_digest(records: Sequence[dict[str, Any]]) -> str:
    payload = "".join(canonical_record(record) + "\n" for record in records)
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()


def document_identifier_records(root: Path) -> tuple[dict[str, Any], ...]:
    records: list[dict[str, Any]] = []
    plist_path = require_file(root, "krita/Info.ios.plist.in")
    try:
        with plist_path.open("rb") as handle:
            info = plistlib.load(handle)
    except (OSError, plistlib.InvalidFileException) as exc:
        raise AuditError(f"cannot parse plist krita/Info.ios.plist.in: {exc}") from exc
    if not isinstance(info, dict):
        raise AuditError("krita/Info.ios.plist.in root is not a dictionary")

    document_types = info.get("CFBundleDocumentTypes")
    if not isinstance(document_types, list):
        raise AuditError("CFBundleDocumentTypes is missing or is not an array")
    for document_type in document_types:
        if not isinstance(document_type, dict):
            raise AuditError("CFBundleDocumentTypes contains a non-dictionary entry")
        identifiers = document_type.get("LSItemContentTypes", [])
        if not isinstance(identifiers, list) or not all(
            isinstance(identifier, str) for identifier in identifiers
        ):
            raise AuditError("LSItemContentTypes is not an array of strings")
        for identifier in identifiers:
            if identifier.startswith("org.krita."):
                records.append(
                    {
                        "source": "krita/Info.ios.plist.in",
                        "field": "CFBundleDocumentTypes.LSItemContentTypes",
                        "value": identifier,
                    }
                )

    for declaration_key in ("UTExportedTypeDeclarations", "UTImportedTypeDeclarations"):
        declarations = info.get(declaration_key)
        if not isinstance(declarations, list):
            raise AuditError(f"{declaration_key} is missing or is not an array")
        for declaration in declarations:
            if not isinstance(declaration, dict):
                raise AuditError(f"{declaration_key} contains a non-dictionary entry")
            identifier = declaration.get("UTTypeIdentifier")
            if not isinstance(identifier, str):
                raise AuditError(f"{declaration_key} entry has no string UTTypeIdentifier")
            if not identifier.startswith("org.krita."):
                continue
            tags = declaration.get("UTTypeTagSpecification")
            if not isinstance(tags, dict):
                raise AuditError(f"{identifier} has no UTTypeTagSpecification dictionary")
            for field in ("public.filename-extension", "public.mime-type"):
                value = tags.get(field)
                values = value if isinstance(value, list) else [value]
                if not values or not all(isinstance(item, str) for item in values):
                    raise AuditError(f"{identifier} has an invalid {field}")
                for item in values:
                    records.append(
                        {
                            "source": "krita/Info.ios.plist.in",
                            "field": f"{declaration_key}.{identifier}.{field}",
                            "value": item,
                        }
                    )
            records.append(
                {
                    "source": "krita/Info.ios.plist.in",
                    "field": f"{declaration_key}.UTTypeIdentifier",
                    "value": identifier,
                }
            )

    mime_path = require_file(root, "krita/data/mime-database/freedesktop.org.xml")
    mime_root = parse_xml(mime_path, root)
    matching_mime_nodes = [
        element
        for element in mime_root.iter()
        if local_name(element.tag) == "mime-type"
        and element.attrib.get("type") == "application/x-krita"
    ]
    if len(matching_mime_nodes) != 1:
        raise AuditError(
            "expected exactly one application/x-krita MIME declaration in "
            "krita/data/mime-database/freedesktop.org.xml"
        )
    records.append(
        {
            "source": "krita/data/mime-database/freedesktop.org.xml",
            "field": "mime-type.type",
            "value": "application/x-krita",
        }
    )
    glob_patterns = sorted(
        element.attrib["pattern"]
        for element in matching_mime_nodes[0]
        if local_name(element.tag) == "glob" and "pattern" in element.attrib
    )
    if not glob_patterns:
        raise AuditError("application/x-krita MIME declaration has no glob patterns")
    for pattern in glob_patterns:
        records.append(
            {
                "source": "krita/data/mime-database/freedesktop.org.xml",
                "field": "mime-type.glob",
                "value": pattern,
            }
        )

    for relative in KRA_PLUGIN_METADATA:
        value = parse_json(require_file(root, relative), root)
        if not isinstance(value, dict):
            raise AuditError(f"plugin metadata root is not a dictionary: {relative}")
        seen_dispatch_field = False
        for field in ("X-KDE-Import", "X-KDE-Export", "X-KDE-Extensions"):
            field_value = value.get(field)
            if field_value is None:
                continue
            if not isinstance(field_value, str) or not field_value.strip():
                raise AuditError(f"{relative} has an invalid {field}")
            seen_dispatch_field = True
            for item in sorted(part.strip() for part in field_value.split(",")):
                if not item:
                    raise AuditError(f"{relative} has an empty {field} item")
                records.append(
                    {"source": relative, "field": field, "value": item}
                )
        if not seen_dispatch_field:
            raise AuditError(f"{relative} has no KRA import/export/extension metadata")

    return sort_records(records)


def decode_cpp_escaped(raw: str) -> str:
    result: list[str] = []
    index = 0
    simple = {
        "a": "\a",
        "b": "\b",
        "f": "\f",
        "n": "\n",
        "r": "\r",
        "t": "\t",
        "v": "\v",
        "\\": "\\",
        "\"": "\"",
        "'": "'",
        "?": "?",
    }
    while index < len(raw):
        if raw[index] != "\\":
            result.append(raw[index])
            index += 1
            continue
        index += 1
        if index >= len(raw):
            result.append("\\")
            break
        escape = raw[index]
        if escape in simple:
            result.append(simple[escape])
            index += 1
            continue
        if escape in "01234567":
            end = index + 1
            while end < min(index + 3, len(raw)) and raw[end] in "01234567":
                end += 1
            result.append(chr(int(raw[index:end], 8)))
            index = end
            continue
        if escape == "x":
            end = index + 1
            while end < len(raw) and raw[end] in "0123456789abcdefABCDEF":
                end += 1
            if end > index + 1:
                result.append(chr(int(raw[index + 1 : end], 16)))
                index = end
                continue
        if escape in {"u", "U"}:
            width = 4 if escape == "u" else 8
            digits = raw[index + 1 : index + 1 + width]
            if len(digits) == width and all(
                character in "0123456789abcdefABCDEF" for character in digits
            ):
                result.append(chr(int(digits, 16)))
                index += width + 1
                continue
        # Unknown implementation-defined escapes are irrelevant to the ASCII
        # identifiers audited here. Preserve their payload without failing the tree.
        result.append(escape)
        index += 1
    return "".join(result)


def cpp_tokens(text: str) -> list[CppToken]:
    tokens: list[CppToken] = []
    index = 0
    length = len(text)
    while index < length:
        character = text[index]
        if character.isspace():
            index += 1
            continue
        if text.startswith("//", index):
            newline = text.find("\n", index + 2)
            index = length if newline < 0 else newline + 1
            continue
        if text.startswith("/*", index):
            end = text.find("*/", index + 2)
            if end < 0:
                raise AuditError("unterminated C/C++ block comment")
            index = end + 2
            continue

        raw_match = re.match(r'(?:u8|u|U|L)?R"([^ ()\\\t\r\n]{0,16})\(', text[index:])
        if raw_match:
            delimiter = raw_match.group(1)
            content_start = index + raw_match.end()
            terminator = ")" + delimiter + '"'
            content_end = text.find(terminator, content_start)
            if content_end < 0:
                raise AuditError("unterminated C++ raw string literal")
            tokens.append(CppToken("string", text[content_start:content_end]))
            index = content_end + len(terminator)
            continue

        string_match = re.match(r'(?:u8|u|U|L)?"', text[index:])
        if string_match:
            cursor = index + string_match.end()
            raw: list[str] = []
            while cursor < length:
                if text[cursor] == '"':
                    break
                if text[cursor] == "\\" and cursor + 1 < length:
                    raw.extend((text[cursor], text[cursor + 1]))
                    cursor += 2
                    continue
                raw.append(text[cursor])
                cursor += 1
            if cursor >= length:
                raise AuditError("unterminated C/C++ string literal")
            tokens.append(CppToken("string", decode_cpp_escaped("".join(raw))))
            index = cursor + 1
            continue

        char_match = re.match(r"(?:u8|u|U|L)?'", text[index:])
        if char_match:
            cursor = index + char_match.end()
            while cursor < length:
                if text[cursor] == "'":
                    break
                cursor += 2 if text[cursor] == "\\" and cursor + 1 < length else 1
            if cursor >= length:
                raise AuditError("unterminated C/C++ character literal")
            index = cursor + 1
            continue

        identifier = re.match(r"[A-Za-z_][A-Za-z0-9_]*", text[index:])
        if identifier:
            value = identifier.group(0)
            tokens.append(CppToken("identifier", value))
            index += len(value)
            continue
        if text.startswith("::", index):
            tokens.append(CppToken("punct", "::"))
            index += 2
            continue
        tokens.append(CppToken("punct", character))
        index += 1
    return tokens


def first_argument_string(tokens: Sequence[CppToken], opening: int) -> str | None:
    depth = 0
    for token in tokens[opening + 1 :]:
        if token.value in {"(", "[", "{"}:
            depth += 1
        elif token.value in {")",
            "]",
            "}",
        }:
            if token.value == ")" and depth == 0:
                return None
            depth -= 1
        elif token.value == "," and depth == 0:
            return None
        elif token.kind == "string":
            return token.value
    return None


def application_identity_records(root: Path) -> tuple[dict[str, Any], ...]:
    cmake_relative = "krita/CMakeLists.txt"
    cmake_text = read_text(require_file(root, cmake_relative), root)
    ios_blocks = re.findall(
        r"(?ims)^[ \t]*if[ \t]*\([ \t]*IOS[ \t]*\)[ \t]*$"
        r"(.*?)"
        r"^[ \t]*endif[ \t]*\([^\r\n)]*\)[ \t]*(?:\#[^\r\n]*)?$",
        cmake_text,
    )
    bundle_identifiers = [
        match.group(1)
        for block in ios_blocks
        for match in re.finditer(
            r'(?m)^[ \t]*MACOSX_BUNDLE_GUI_IDENTIFIER[ \t]+"([^"\r\n]+)"[ \t]*$',
            block,
        )
    ]
    if len(bundle_identifiers) != 1:
        raise AuditError(
            f"expected exactly one literal iOS MACOSX_BUNDLE_GUI_IDENTIFIER; "
            f"found {len(bundle_identifiers)}"
        )

    relative = "krita/main.cc"
    path = require_file(root, relative)
    try:
        tokens = cpp_tokens(read_text(path, root))
    except AuditError as exc:
        raise AuditError(f"cannot tokenize {relative}: {exc}") from exc
    records: list[dict[str, Any]] = [
        {
            "source": cmake_relative,
            "field": "MACOSX_BUNDLE_GUI_IDENTIFIER",
            "value": bundle_identifiers[0],
        }
    ]
    for index, token in enumerate(tokens):
        if token.kind != "identifier":
            continue
        if token.value in {"setApplicationDomain", "addDomainLocaleDir"}:
            if index < 2 or tokens[index - 1].value != "::":
                continue
            if tokens[index - 2] != CppToken("identifier", "KLocalizedString"):
                continue
            if index + 1 >= len(tokens) or tokens[index + 1].value != "(":
                continue
            value = first_argument_string(tokens, index + 1)
            if value is None:
                raise AuditError(f"{relative} has a non-literal {token.value} domain")
            records.append(
                {"source": relative, "field": f"KLocalizedString::{token.value}", "value": value}
            )
        elif token.value == "KAboutData":
            if index + 2 >= len(tokens):
                continue
            if tokens[index + 1].kind != "identifier" or tokens[index + 2].value != "(":
                continue
            value = first_argument_string(tokens, index + 2)
            if value is None:
                raise AuditError(f"{relative} has a non-literal KAboutData application ID")
            records.append(
                {"source": relative, "field": "KAboutData.applicationId", "value": value}
            )
    if not records:
        raise AuditError("no KLocalizedString domain or KAboutData application ID found")
    return sort_records(records)


def cpp_literal_records(
    root: Path,
    predicate: Callable[[str], bool],
    prefilter: Sequence[str],
) -> tuple[dict[str, Any], ...]:
    records: list[dict[str, Any]] = []
    for path in discover_files(root, CPP_SUFFIXES):
        relative = relative_name(root, path)
        text = read_text(path, root)
        # This substring check is only an I/O optimization. A match is never
        # accepted until the C/C++ lexer identifies a real string literal.
        if not any(candidate in text for candidate in prefilter):
            continue
        try:
            tokens = cpp_tokens(text)
        except AuditError as exc:
            raise AuditError(f"cannot tokenize {relative}: {exc}") from exc
        for token in tokens:
            if token.kind == "string" and predicate(token.value):
                records.append({"source": relative, "value": token.value})
    return sort_records(records)


def configuration_file_records(root: Path) -> tuple[dict[str, Any], ...]:
    records = list(
        cpp_literal_records(
            root,
            lambda value: value in CONFIG_IDENTIFIERS,
            ("kritarc", "kritadisplayrc"),
        )
    )
    qrc_relative = "krita/krita.qrc"
    qrc_root = parse_xml(require_file(root, qrc_relative), root)
    qrc_entries = []
    for element in qrc_root.iter():
        if local_name(element.tag) != "file" or element.attrib.get("alias") != "kritarc":
            continue
        target = (element.text or "").strip()
        if not target:
            raise AuditError("krita/krita.qrc has an empty kritarc alias target")
        qrc_entries.append(target)
        require_file(root, f"krita/{target}")
    if len(qrc_entries) != 1:
        raise AuditError("krita/krita.qrc must contain exactly one kritarc alias")
    records.append(
        {
            "source": qrc_relative,
            "alias": "kritarc",
            "target": qrc_entries[0],
        }
    )
    return sort_records(records)


def resource_path_records(root: Path) -> tuple[dict[str, Any], ...]:
    records = cpp_literal_records(
        root,
        lambda value: bool(RESOURCE_PATH.search(value)),
        ("share/krita",),
    )
    if not records:
        raise AuditError("no C/C++ share/krita resource lookup path found")
    return records


def is_plugin_metadata(value: Any) -> bool:
    if not isinstance(value, dict):
        return False
    return (
        "Id" in value
        or "KPlugin" in value
        or "X-KDE-ServiceTypes" in value
        or any(isinstance(key, str) and key.startswith("X-Krita-") for key in value)
    )


def json_pointer_component(value: str) -> str:
    return value.replace("~", "~0").replace("/", "~1")


def plugin_metadata_inventory(root: Path) -> Inventory:
    plugin_root = root / "plugins"
    if not plugin_root.is_dir():
        raise AuditError("plugin source directory is missing: plugins")
    paths = [
        path
        for path in discover_files(root, frozenset({".json"}))
        if path.is_relative_to(plugin_root)
    ]
    records: list[dict[str, Any]] = []
    metadata_file_count = 0
    for path in paths:
        value = parse_json(path, root)
        if not is_plugin_metadata(value):
            continue
        metadata_file_count += 1
        relative = relative_name(root, path)

        def walk(node: Any, pointer: str) -> None:
            if isinstance(node, dict):
                for key, child in node.items():
                    if not isinstance(key, str):
                        raise AuditError(f"{relative} contains a non-string JSON key")
                    child_pointer = pointer + "/" + json_pointer_component(key)
                    if key in PLUGIN_STABLE_KEYS or key.startswith("X-Krita-"):
                        if key == "Id" and (not isinstance(child, str) or not child):
                            raise AuditError(f"{relative}{child_pointer} is not a non-empty string")
                        if key == "X-KDE-ServiceTypes" and not (
                            isinstance(child, list)
                            and child
                            and all(isinstance(item, str) and item for item in child)
                        ):
                            raise AuditError(
                                f"{relative}{child_pointer} is not a non-empty string array"
                            )
                        records.append(
                            {
                                "source": relative,
                                "pointer": child_pointer,
                                "value": child,
                            }
                        )
                    walk(child, child_pointer)
            elif isinstance(node, list):
                for item_index, child in enumerate(node):
                    walk(child, pointer + f"/{item_index}")

        walk(value, "")
    if metadata_file_count == 0 or not records:
        raise AuditError("no plugin compatibility metadata found")
    return Inventory(sort_records(records), metadata_file_count)


def action_id_inventory(root: Path) -> Inventory:
    paths = discover_files(root, frozenset({".action"}))
    records: list[dict[str, Any]] = []
    for path in paths:
        relative = relative_name(root, path)
        xml_root = parse_xml(path, root)
        for element in xml_root.iter():
            if local_name(element.tag).lower() != "action":
                continue
            name = element.attrib.get("name")
            if not isinstance(name, str) or not name:
                raise AuditError(f"{relative} contains an Action without a non-empty name")
            records.append({"source": relative, "name": name})
    if not paths or not records:
        raise AuditError("no production .action identifiers found")
    return Inventory(sort_records(records), len(paths))


def inventory_summary(inventory: Inventory) -> dict[str, Any]:
    return {
        "file_count": inventory.file_count,
        "record_count": len(inventory.records),
        "sha256": records_digest(inventory.records),
    }


def current_manifest(root: Path) -> dict[str, Any]:
    explicit_records = {
        "document_identifiers": document_identifier_records(root),
        "application_identity": application_identity_records(root),
        "configuration_files": configuration_file_records(root),
        "resource_paths": resource_path_records(root),
    }
    plugins = plugin_metadata_inventory(root)
    actions = action_id_inventory(root)
    return {
        "schema": SCHEMA,
        "generated_by": GENERATED_BY,
        "policy": POLICY,
        "explicit_contracts": {
            category: {
                "record_count": len(explicit_records[category]),
                "records": list(explicit_records[category]),
            }
            for category in EXPLICIT_CATEGORIES
        },
        "inventories": {
            "plugin_metadata": inventory_summary(plugins),
            "action_ids": inventory_summary(actions),
        },
    }


def load_manifest(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise AuditError(f"cannot read manifest {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise AuditError(f"cannot parse manifest {path}:{exc.lineno}: {exc.msg}") from exc
    if not isinstance(value, dict):
        raise AuditError("compatibility manifest root is not a dictionary")
    if value.get("schema") != SCHEMA:
        raise AuditError(f"unsupported compatibility manifest schema: {value.get('schema')!r}")
    expected_top_level = {
        "schema",
        "generated_by",
        "policy",
        "explicit_contracts",
        "inventories",
    }
    if set(value) != expected_top_level:
        raise AuditError(
            "compatibility manifest fields changed: "
            f"{sorted(value)} != {sorted(expected_top_level)}"
        )
    if value.get("generated_by") != GENERATED_BY:
        raise AuditError("compatibility manifest has an unexpected generator")
    if not isinstance(value.get("policy"), str) or not value["policy"]:
        raise AuditError("compatibility manifest policy is missing")

    explicit = value.get("explicit_contracts")
    if not isinstance(explicit, dict) or set(explicit) != set(EXPLICIT_CATEGORIES):
        raise AuditError("compatibility manifest explicit contract categories changed")
    for category in EXPLICIT_CATEGORIES:
        entry = explicit[category]
        if not isinstance(entry, dict) or set(entry) != {"record_count", "records"}:
            raise AuditError(f"invalid explicit contract entry: {category}")
        records = entry.get("records")
        if not isinstance(records, list) or not all(
            isinstance(record, dict) for record in records
        ):
            raise AuditError(f"invalid explicit contract records: {category}")
        if entry.get("record_count") != len(records):
            raise AuditError(f"explicit contract count does not match records: {category}")
        if records != list(sort_records(records)):
            raise AuditError(f"explicit contract records are not sorted: {category}")

    inventories = value.get("inventories")
    if not isinstance(inventories, dict) or set(inventories) != set(INVENTORY_CATEGORIES):
        raise AuditError("compatibility manifest inventory categories changed")
    for category in INVENTORY_CATEGORIES:
        entry = inventories[category]
        if not isinstance(entry, dict) or set(entry) != {
            "file_count",
            "record_count",
            "sha256",
        }:
            raise AuditError(f"invalid inventory entry: {category}")
        if not isinstance(entry["file_count"], int) or entry["file_count"] <= 0:
            raise AuditError(f"invalid inventory file count: {category}")
        if not isinstance(entry["record_count"], int) or entry["record_count"] <= 0:
            raise AuditError(f"invalid inventory record count: {category}")
        if not isinstance(entry["sha256"], str) or not re.fullmatch(
            r"[0-9a-f]{64}", entry["sha256"]
        ):
            raise AuditError(f"invalid inventory digest: {category}")
    return value


def format_record_difference(
    category: str,
    expected: Sequence[dict[str, Any]],
    current: Sequence[dict[str, Any]],
) -> str:
    expected_lines = set(map(canonical_record, expected))
    current_lines = set(map(canonical_record, current))
    removed = sorted(expected_lines - current_lines)
    added = sorted(current_lines - expected_lines)
    details: list[str] = []
    for line in removed[:3]:
        details.append(f"missing {line}")
    for line in added[:3]:
        details.append(f"added {line}")
    omitted = len(removed) + len(added) - len(details)
    if omitted > 0:
        details.append(f"... {omitted} more difference(s)")
    return f"{category} changed: " + "; ".join(details)


def compare_manifest(expected: dict[str, Any], current: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    for category in EXPLICIT_CATEGORIES:
        expected_entry = expected["explicit_contracts"][category]
        current_entry = current["explicit_contracts"][category]
        if expected_entry != current_entry:
            errors.append(
                format_record_difference(
                    category, expected_entry["records"], current_entry["records"]
                )
            )
    for category in INVENTORY_CATEGORIES:
        expected_entry = expected["inventories"][category]
        current_entry = current["inventories"][category]
        if expected_entry != current_entry:
            errors.append(
                f"{category} changed: expected "
                f"{expected_entry['file_count']} files/{expected_entry['record_count']} records/"
                f"{expected_entry['sha256']}, found "
                f"{current_entry['file_count']} files/{current_entry['record_count']} records/"
                f"{current_entry['sha256']}"
            )
    return errors


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root",
        type=Path,
        default=REPO_ROOT,
        help="source tree to audit (default: repository root)",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        help="compatibility manifest (default: SOURCE_ROOT/packaging/ios/manifests/compatibility-identifiers.json)",
    )
    parser.add_argument(
        "--print-current-manifest",
        action="store_true",
        help="print the structurally extracted manifest instead of comparing it",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = parse_args(argv)
    root = args.source_root.resolve()
    if not root.is_dir():
        print(f"<compatibility-audit>:0: error: source root is not a directory: {root}", file=sys.stderr)
        return 1
    try:
        current = current_manifest(root)
        if args.print_current_manifest:
            print(json.dumps(current, ensure_ascii=False, indent=2) + "")
            return 0
        manifest_path = args.manifest or root / MANIFEST_RELATIVE_PATH
        expected = load_manifest(manifest_path.resolve())
        errors = compare_manifest(expected, current)
    except AuditError as exc:
        print(f"<compatibility-audit>:0: error: {exc}", file=sys.stderr)
        return 1

    if errors:
        for error in errors:
            print(f"<compatibility-audit>:0: error: {error}", file=sys.stderr)
        print(
            f"iOS compatibility identifier audit failed: {len(errors)} changed category/categories",
            file=sys.stderr,
        )
        return 1

    explicit_count = sum(
        entry["record_count"] for entry in current["explicit_contracts"].values()
    )
    plugins = current["inventories"]["plugin_metadata"]
    actions = current["inventories"]["action_ids"]
    print(
        "iOS compatibility identifier audit: "
        f"{explicit_count} explicit records; "
        f"{plugins['record_count']} plugin metadata records in {plugins['file_count']} files; "
        f"{actions['record_count']} action IDs in {actions['file_count']} files"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
