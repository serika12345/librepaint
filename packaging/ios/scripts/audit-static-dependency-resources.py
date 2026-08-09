#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Audit non-code resources pulled from static Qt and KF6 libraries on iOS."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import posixpath
import re
import shlex
import subprocess
import sys
import tarfile
from collections import Counter
from contextlib import ExitStack
from pathlib import Path, PurePosixPath
from typing import Any, Dict, List, Mapping, Optional, Sequence, Set, Tuple


REPO_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_MANIFEST = REPO_ROOT / "packaging/ios/manifests/static-dependency-resources.json"
QT_MANIFEST = REPO_ROOT / "packaging/ios/qt/modules.json"
KF_MANIFEST = REPO_ROOT / "packaging/ios/frameworks/frameworks.json"
MANIFEST_SCOPE = "Static QtBase and KF6 non-code resources linked into LibrePaint for iOS"

QT_LICENSE = (
    "LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only"
)
QT_COPYRIGHT = ["Copyright (C) The Qt Company Ltd."]
ICC_LICENSE = "LicenseRef-ICC-License"
ICC_COPYRIGHT = ["Copyright International Color Consortium, 2015"]

class AuditError(RuntimeError):
    """The pinned static-resource contract no longer holds."""


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    try:
        with path.open("rb") as handle:
            for block in iter(lambda: handle.read(1024 * 1024), b""):
                digest.update(block)
    except OSError as exc:
        raise AuditError(f"cannot read {path}: {exc}") from exc
    return digest.hexdigest()


def load_json(path: Path) -> Dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise AuditError(f"cannot read JSON {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise AuditError(f"JSON root must be an object: {path}")
    return value


def safe_source_path(value: str) -> bool:
    path = PurePosixPath(value)
    return bool(value) and not path.is_absolute() and ".." not in path.parts


def canonical_source_pins() -> Dict[str, Dict[str, str]]:
    qt = load_json(QT_MANIFEST)
    kf = load_json(KF_MANIFEST)

    def select(records: Any, name: str, owner: str) -> Mapping[str, Any]:
        if not isinstance(records, list):
            raise AuditError(f"{owner} does not contain a list")
        matches = [record for record in records if record.get("name") == name]
        if len(matches) != 1:
            raise AuditError(f"expected one {name} record in {owner}; found {len(matches)}")
        return matches[0]

    result: Dict[str, Dict[str, str]] = {}
    for key, record, canonical in (
        ("qtbase", select(qt.get("modules"), "qtbase", str(QT_MANIFEST)), QT_MANIFEST),
        (
            "kcolorscheme",
            select(kf.get("packages"), "kcolorscheme", str(KF_MANIFEST)),
            KF_MANIFEST,
        ),
        (
            "kwidgetsaddons",
            select(kf.get("packages"), "kwidgetsaddons", str(KF_MANIFEST)),
            KF_MANIFEST,
        ),
    ):
        source = record.get("source")
        if not isinstance(source, dict):
            raise AuditError(f"canonical source record is missing for {key}")
        result[key] = {
            "version": str(record.get("version")),
            "flake_attr": str(source.get("flake_attr")),
            "archive_name": str(source.get("archive_name")),
            "archive_sha256": str(source.get("archive_sha256")),
            "canonical_manifest": str(canonical.relative_to(REPO_ROOT)),
        }
    return result


class TarSource:
    """Random access to regular files below the single root of a source tarball."""

    def __init__(self, path: Path) -> None:
        self.path = path
        try:
            self.archive = tarfile.open(str(path), "r:*")
        except (OSError, tarfile.TarError) as exc:
            raise AuditError(f"cannot open source archive {path}: {exc}") from exc
        regular = [member for member in self.archive.getmembers() if member.isfile()]
        first_parts: Set[str] = set()
        for member in regular:
            parts = PurePosixPath(member.name).parts
            if not parts or PurePosixPath(member.name).is_absolute() or ".." in parts:
                raise AuditError(f"unsafe member in {path}: {member.name}")
            first_parts.add(parts[0])
        self.prefix = next(iter(first_parts)) if len(first_parts) == 1 else ""
        self.members: Dict[str, tarfile.TarInfo] = {}
        for member in regular:
            name = member.name
            if self.prefix and name.startswith(self.prefix + "/"):
                name = name[len(self.prefix) + 1 :]
            if not safe_source_path(name) or name in self.members:
                raise AuditError(f"invalid or duplicate source member in {path}: {name}")
            self.members[name] = member

    def close(self) -> None:
        self.archive.close()

    def __enter__(self) -> "TarSource":
        return self

    def __exit__(self, *_args: Any) -> None:
        self.close()

    def read(self, relative: str) -> bytes:
        member = self.members.get(relative)
        if member is None:
            raise AuditError(f"source archive {self.path} is missing {relative}")
        handle = self.archive.extractfile(member)
        if handle is None:
            raise AuditError(f"cannot extract {relative} from {self.path}")
        return handle.read()

    def text(self, relative: str) -> str:
        try:
            return self.read(relative).decode("utf-8")
        except UnicodeDecodeError as exc:
            raise AuditError(f"source text is not UTF-8: {relative}") from exc


def cmake_set_list(text: str, variable: str) -> List[str]:
    pattern = re.compile(
        r"(?ms)^\s*set\(\s*" + re.escape(variable) + r"\s*(.*?)^\s*\)\s*$"
    )
    matches = pattern.findall(text)
    if len(matches) != 1:
        raise AuditError(f"expected one CMake set({variable}); found {len(matches)}")
    values = re.findall(r'"([^"\n]+)"', matches[0])
    if not values:
        raise AuditError(f"CMake set({variable}) has no quoted files")
    return values


def cmake_resource_call(text: str, resource_name: str) -> str:
    start = re.search(
        r"(?:qt_internal_add_resource|qt6_add_resources|qt_add_resources)\s*"
        r"\(\s*[^\s()]+\s+\"?"
        + re.escape(resource_name)
        + r"\"?(?=\s|\))",
        text,
    )
    if start is None:
        raise AuditError(f"CMake resource declaration is missing: {resource_name}")
    open_index = text.find("(", start.start())
    depth = 0
    quote = False
    escaped = False
    for index in range(open_index, len(text)):
        char = text[index]
        if quote:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quote = False
            continue
        if char == '"':
            quote = True
        elif char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                return text[start.start() : index + 1]
    raise AuditError(f"unterminated CMake resource declaration: {resource_name}")


def cmake_section_value(call: str, keyword: str) -> Optional[str]:
    match = re.search(
        r"(?m)^\s*" + re.escape(keyword) + r"\s*(?:\n\s*)?(?:\"([^\"]+)\"|([^\s()]+))",
        call,
    )
    if match is None:
        return None
    return match.group(1) or match.group(2)


def cmake_literal_files(call: str) -> List[str]:
    match = re.search(r"(?m)^\s*FILES\b", call)
    if match is None:
        raise AuditError("CMake resource declaration has no FILES section")
    body = call[match.end() :]
    stop = re.search(
        r"(?m)^\s*(?:OUTPUT_TARGETS|OPTIONS|BIG_RESOURCES|DISCARD_FILE_CONTENTS)\b",
        body,
    )
    if stop is not None:
        body = body[: stop.start()]
    body = re.sub(r"(?m)#.*$", "", body)
    values: List[str] = []
    for match in re.finditer(r'"([^"\n]+)"|([^\s()]+)', body):
        values.append(match.group(1) or match.group(2))
    return values


def assert_resource_call(
    text: str,
    name: str,
    prefix: str,
    base: Optional[str],
    file_reference: Optional[str],
) -> str:
    call = cmake_resource_call(text, name)
    if cmake_section_value(call, "PREFIX") != prefix:
        raise AuditError(f"resource prefix changed for {name}")
    if cmake_section_value(call, "BASE") != base:
        raise AuditError(f"resource base changed for {name}")
    if file_reference and "${" + file_reference + "}" not in call:
        raise AuditError(f"resource {name} no longer uses {file_reference}")
    return call


def normalize_source(base_dir: str, relative: str) -> str:
    result = posixpath.normpath(posixpath.join(base_dir, relative))
    if not safe_source_path(result):
        raise AuditError(f"unsafe CMake source path: {relative}")
    return result


def file_record(
    source: TarSource,
    source_path: str,
    resource_alias: str,
    copyright_lines: Sequence[str],
) -> Dict[str, Any]:
    source.read(source_path)
    suffix = PurePosixPath(source_path).suffix.lower()
    media_types = {
        ".png": "image/png",
        ".icc": "application/vnd.iccprofile",
        ".qsb": "application/vnd.qt.qsb",
        ".colors": "text/x-kde-colorscheme",
    }
    return {
        "source_path": source_path,
        "resource_alias": resource_alias,
        "media_type": media_types.get(suffix, "application/octet-stream"),
        "copyright": list(copyright_lines),
    }


def spdx_headers(text: str, path: str) -> Tuple[str, List[str]]:
    licenses = re.findall(r"(?m)^# SPDX-License-Identifier:\s*(.+?)\s*$", text)
    copyrights = re.findall(r"(?m)^# SPDX-FileCopyrightText:\s*(.+?)\s*$", text)
    if len(licenses) != 1 or not copyrights:
        raise AuditError(f"in-file SPDX headers changed: {path}")
    return licenses[0], copyrights


def validate_reuse(
    source: TarSource,
    path: str,
    required_paths: Sequence[str],
    expression: str,
    copyright_line: str,
) -> None:
    text = source.text(path)
    for required in required_paths:
        if '"' + required + '"' not in text:
            raise AuditError(f"REUSE coverage changed in {path}: {required}")
    if 'SPDX-License-Identifier = "' + expression + '"' not in text:
        raise AuditError(f"REUSE license changed in {path}")
    if copyright_line not in text:
        raise AuditError(f"REUSE copyright changed in {path}")


def evidence_records(
    sources: Mapping[str, TarSource], specifications: Sequence[Mapping[str, Any]]
) -> List[Dict[str, str]]:
    records: List[Dict[str, str]] = []
    for specification in specifications:
        evidence_id = specification["id"]
        source_name = specification["source"]
        path = specification["path"]
        kind = specification["kind"]
        actual = sha256(sources[source_name].read(path))
        records.append(
            {
                "id": evidence_id,
                "source": source_name,
                "path": path,
                "sha256": actual,
                "kind": kind,
            }
        )
    return records


def source_groups(
    sources: Mapping[str, TarSource], templates: Sequence[Mapping[str, Any]]
) -> List[Dict[str, Any]]:
    qt = sources["qtbase"]
    kcolor = sources["kcolorscheme"]
    kwidgets = sources["kwidgetsaddons"]
    widgets_cmake = qt.text("src/widgets/CMakeLists.txt")
    gui_cmake = qt.text("src/gui/CMakeLists.txt")
    color_cmake = kcolor.text("src/CMakeLists.txt")
    kwidgets_cmake = kwidgets.text("src/CMakeLists.txt")

    validate_reuse(
        qt,
        "src/widgets/REUSE.toml",
        ("styles/images/*.png", "dialogs/images/*.png"),
        QT_LICENSE,
        "Copyright (C) The Qt Company Ltd.",
    )
    validate_reuse(
        qt,
        "src/gui/painting/shaders/REUSE.toml",
        ("*",),
        QT_LICENSE,
        "Copyright (C) The Qt Company Ltd.",
    )
    validate_reuse(
        qt,
        "src/3rdparty/icc/REUSE.toml",
        ("**",),
        ICC_LICENSE,
        "Copyright International Color Consortium, 2015",
    )
    attribution = load_json_from_bytes(
        qt.read("src/3rdparty/icc/qt_attribution.json"), "Qt ICC attribution"
    )
    attribution_records = attribution if isinstance(attribution, list) else []
    if (
        len(attribution_records) != 1
        or attribution_records[0].get("Id") != "icc-srgb-color-profile"
        or attribution_records[0].get("LicenseId") != ICC_LICENSE
        or attribution_records[0].get("Files") != "sRGB2014.icc"
    ):
        raise AuditError("Qt ICC attribution record changed")

    template_by_name = {str(group["name"]): group for group in templates}
    groups: Dict[str, Dict[str, Any]] = {}

    def add_group(
        name: str,
        source_name: str,
        cmake_path: str,
        cmake_resource_name: str,
        prefix: str,
        base: Optional[str],
        file_variable: Optional[str],
        files: List[Dict[str, Any]],
        license_expression: Optional[str],
        evidence_ids: Sequence[str],
        exclusion_reason: Optional[str] = None,
    ) -> None:
        if name not in template_by_name:
            raise AuditError(f"manifest has no artifact template for resource group {name}")
        template = template_by_name[name]
        status = template["status"]
        group: Dict[str, Any] = {
            "name": name,
            "status": status,
            "source": source_name,
            "declaration": {
                "cmake_path": cmake_path,
                "resource_name": cmake_resource_name,
                "prefix": prefix,
                "base": base,
                "file_variable": file_variable,
            },
            "license": {
                "upstream_expression": license_expression,
                "selected_distribution_license": (
                    None
                    if status == "excluded"
                    else (
                        "LGPL-3.0-only"
                        if license_expression == QT_LICENSE
                        else license_expression
                    )
                ),
                "evidence_ids": list(evidence_ids),
            },
            "init_wrapper": dict(template["init_wrapper"]),
            "payload": dict(template["payload"]),
            "files": sorted(files, key=lambda record: record["source_path"]),
        }
        if exclusion_reason:
            group["exclusion_reason"] = exclusion_reason
        groups[name] = group

    style_specs = (
        (
            "qstyle",
            "qstyle_resource_files",
            "/qt-project.org/styles/commonstyle",
        ),
        (
            "qstyle1",
            "qstyle1_resource_files",
            "/qt-project.org/styles/macstyle",
        ),
        (
            "qstyle_fusion",
            "qstyle_resource_fusion_files",
            "/qt-project.org/styles/fusionstyle",
        ),
    )
    for name, variable, prefix in style_specs:
        assert_resource_call(widgets_cmake, name, prefix, "styles", variable)
        files = []
        for relative in cmake_set_list(widgets_cmake, variable):
            if not relative.startswith("styles/"):
                raise AuditError(f"unexpected {name} source outside styles/: {relative}")
            files.append(
                file_record(
                    qt,
                    normalize_source("src/widgets", relative),
                    prefix + "/" + relative[len("styles/") :],
                    QT_COPYRIGHT,
                )
            )
        add_group(
            name,
            "qtbase",
            "src/widgets/CMakeLists.txt",
            name,
            prefix,
            "styles",
            variable,
            files,
            QT_LICENSE,
            ("qt-widgets-reuse",),
        )

    qpdf_variable = "qpdf_resource_files"
    assert_resource_call(gui_cmake, "qpdf", "/qpdf/", "painting", qpdf_variable)
    qpdf_values = cmake_set_list(gui_cmake, qpdf_variable)
    if qpdf_values != ["../3rdparty/icc/sRGB2014.icc"]:
        raise AuditError(f"qpdf source declaration changed: {qpdf_values}")
    if not re.search(
        r'set_source_files_properties\(\s*"\.\./3rdparty/icc/sRGB2014\.icc".*?'
        r'QT_RESOURCE_ALIAS\s+"sRGB2014\.icc"',
        gui_cmake,
        re.DOTALL,
    ):
        raise AuditError("qpdf ICC resource alias changed")
    add_group(
        "qpdf",
        "qtbase",
        "src/gui/CMakeLists.txt",
        "qpdf",
        "/qpdf/",
        "painting",
        qpdf_variable,
        [
            file_record(
                qt,
                "src/3rdparty/icc/sRGB2014.icc",
                "/qpdf/sRGB2014.icc",
                ICC_COPYRIGHT,
            )
        ],
        ICC_LICENSE,
        ("icc-reuse", "icc-license", "icc-attribution"),
    )

    shader_call = assert_resource_call(
        gui_cmake, "gui_shaders", "/qt-project.org/gui", None, None
    )
    shader_values = cmake_literal_files(shader_call)
    expected_shaders = [
        "painting/shaders/backingstorecompose.vert.qsb",
        "painting/shaders/backingstorecompose.frag.qsb",
    ]
    if shader_values != expected_shaders:
        raise AuditError(f"gui_shaders declaration changed: {shader_values}")
    add_group(
        "gui_shaders",
        "qtbase",
        "src/gui/CMakeLists.txt",
        "gui_shaders",
        "/qt-project.org/gui",
        None,
        None,
        [
            file_record(
                qt,
                normalize_source("src/gui", relative),
                "/qt-project.org/gui/" + relative,
                QT_COPYRIGHT,
            )
            for relative in shader_values
        ],
        QT_LICENSE,
        ("qt-shaders-reuse",),
    )

    qmessage_variable = "qmessagebox_resource_files"
    assert_resource_call(
        widgets_cmake,
        "qmessagebox",
        "/qt-project.org/qmessagebox",
        "dialogs",
        qmessage_variable,
    )
    qmessage_values = cmake_set_list(widgets_cmake, qmessage_variable)
    if qmessage_values != ["dialogs/images/qtlogo-64.png"]:
        raise AuditError(f"qmessagebox declaration changed: {qmessage_values}")
    add_group(
        "qmessagebox",
        "qtbase",
        "src/widgets/CMakeLists.txt",
        "qmessagebox",
        "/qt-project.org/qmessagebox",
        "dialogs",
        qmessage_variable,
        [
            file_record(
                qt,
                "src/widgets/dialogs/images/qtlogo-64.png",
                "/qt-project.org/qmessagebox/images/qtlogo-64.png",
                QT_COPYRIGHT,
            )
        ],
        QT_LICENSE,
        ("qt-widgets-reuse",),
    )

    color_call = assert_resource_call(
        color_cmake, "color_schemes", "/org.kde.kcolorscheme", None, None
    )
    color_values = cmake_literal_files(color_call)
    expected_colors = [
        "color-schemes/BreezeDark.colors",
        "color-schemes/BreezeLight.colors",
    ]
    if color_values != expected_colors:
        raise AuditError(f"color_schemes declaration changed: {color_values}")
    color_files: List[Dict[str, Any]] = []
    color_evidence: List[str] = []
    for relative, evidence_id in zip(
        color_values, ("breeze-dark-inline-spdx", "breeze-light-inline-spdx")
    ):
        source_path = normalize_source("src", relative)
        license_expression, copyrights = spdx_headers(
            kcolor.text(source_path), source_path
        )
        if license_expression != "LGPL-2.0-or-later":
            raise AuditError(f"Breeze color-scheme license changed: {source_path}")
        color_files.append(
            file_record(
                kcolor,
                source_path,
                "/org.kde.kcolorscheme/" + relative,
                copyrights,
            )
        )
        color_evidence.append(evidence_id)
    add_group(
        "color_schemes",
        "kcolorscheme",
        "src/CMakeLists.txt",
        "color_schemes",
        "/org.kde.kcolorscheme",
        None,
        None,
        color_files,
        "LGPL-2.0-or-later",
        color_evidence,
    )

    kchar_call = assert_resource_call(
        kwidgets_cmake, "kcharselect-data", "/kf6/kcharselect/", None, None
    )
    if cmake_literal_files(kchar_call) != ["kcharselect-data"]:
        raise AuditError("kcharselect-data declaration changed")
    generator = kwidgets.text("src/kcharselect-generate-datafile.py")
    for required in (
        "SPDX-License-Identifier: LGPL-2.0-or-later",
        "UnicodeData.txt",
        "Unihan_Readings.txt",
        "NamesList.txt",
        "Blocks.txt",
    ):
        if required not in generator:
            raise AuditError(f"kcharselect generator provenance changed: {required}")
    add_group(
        "kcharselect_data",
        "kwidgetsaddons",
        "src/CMakeLists.txt",
        "kcharselect-data",
        "/kf6/kcharselect/",
        None,
        None,
        [
            file_record(
                kwidgets,
                "src/kcharselect-data",
                "/kf6/kcharselect/kcharselect-data",
                (),
            )
        ],
        None,
        ("kcharselect-generator",),
        "Unused KCharSelect payload; the pinned archive does not carry a self-contained license mapping for its generated Unicode data.",
    )

    template_order = [str(group["name"]) for group in templates]
    if set(groups) != set(template_order):
        raise AuditError(
            "source resource groups differ from the manifest: "
            f"source={sorted(groups)}, manifest={sorted(template_order)}"
        )
    return [groups[name] for name in template_order]


def load_json_from_bytes(data: bytes, label: str) -> Any:
    try:
        return json.loads(data.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise AuditError(f"invalid JSON in {label}: {exc}") from exc


def notice_records(
    specifications: Sequence[Mapping[str, Any]],
) -> List[Dict[str, str]]:
    records: List[Dict[str, str]] = []
    for specification in specifications:
        relative = specification["path"]
        path = REPO_ROOT / relative
        if not path.is_file():
            raise AuditError(f"required notice is missing: {relative}")
        records.append({"path": relative, "sha256": file_sha256(path)})
    return records


def generated_manifest(
    sources: Mapping[str, TarSource],
    pins: Mapping[str, Mapping[str, str]],
    template: Mapping[str, Any],
) -> Dict[str, Any]:
    return {
        "schema": 2,
        "scope": template["scope"],
        "sources": dict(pins),
        "license_evidence": evidence_records(
            sources, template["license_evidence"]
        ),
        "notice_files": notice_records(template["notice_files"]),
        "final_binary": {
            key: list(value) for key, value in template["final_binary"].items()
        },
        "groups": source_groups(sources, template["groups"]),
    }


def check_source_pins(manifest: Mapping[str, Any]) -> Dict[str, Dict[str, str]]:
    canonical = canonical_source_pins()
    if manifest.get("sources") != canonical:
        raise AuditError("static-resource source pins differ from canonical Qt/KF manifests")
    return canonical


def valid_sha256(value: Any) -> bool:
    return isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value) is not None


def string_list(value: Any, label: str, *, allow_empty: bool = False) -> List[str]:
    if not isinstance(value, list) or any(
        not isinstance(item, str) or not item for item in value
    ):
        raise AuditError(f"{label} must be a list of nonempty strings")
    if not allow_empty and not value:
        raise AuditError(f"{label} must not be empty")
    if len(value) != len(set(value)):
        raise AuditError(f"{label} contains duplicates")
    return value


def check_notice_files(manifest: Mapping[str, Any]) -> None:
    for record in manifest["notice_files"]:
        path = REPO_ROOT / record["path"]
        if not path.is_file():
            raise AuditError(f"required notice is missing: {record['path']}")
        actual = file_sha256(path)
        if actual != record["sha256"]:
            raise AuditError(
                f"notice changed: {record['path']} ({actual}, expected {record['sha256']})"
            )


def check_manifest_structure(manifest: Mapping[str, Any]) -> None:
    expected_top_level = {
        "schema",
        "scope",
        "sources",
        "license_evidence",
        "notice_files",
        "final_binary",
        "groups",
    }
    if set(manifest) != expected_top_level:
        raise AuditError(
            "static-resource manifest keys changed: "
            f"{sorted(manifest)} != {sorted(expected_top_level)}"
        )
    if manifest.get("schema") != 2:
        raise AuditError(f"unsupported static-resource schema: {manifest.get('schema')}")
    if manifest.get("scope") != MANIFEST_SCOPE:
        raise AuditError("static-resource manifest scope changed")

    sources = manifest.get("sources")
    if not isinstance(sources, dict) or not sources:
        raise AuditError("manifest sources must be a nonempty object")
    source_keys = {
        "version",
        "flake_attr",
        "archive_name",
        "archive_sha256",
        "canonical_manifest",
    }
    for name, source in sources.items():
        if not isinstance(name, str) or not name or not isinstance(source, dict):
            raise AuditError(f"invalid source record: {name!r}")
        if set(source) != source_keys:
            raise AuditError(f"invalid source fields for {name}")
        for key in source_keys - {"archive_sha256"}:
            if not isinstance(source[key], str) or not source[key]:
                raise AuditError(f"invalid source {key} for {name}")
        if not valid_sha256(source["archive_sha256"]):
            raise AuditError(f"invalid source archive hash for {name}")
        if not safe_source_path(source["canonical_manifest"]):
            raise AuditError(f"unsafe canonical manifest path for {name}")

    evidence = manifest.get("license_evidence")
    if not isinstance(evidence, list) or not evidence:
        raise AuditError("manifest license_evidence must be a nonempty list")
    evidence_ids: Set[str] = set()
    evidence_sources: Set[Tuple[str, str]] = set()
    evidence_keys = {"id", "source", "path", "sha256", "kind"}
    for record in evidence:
        if not isinstance(record, dict) or set(record) != evidence_keys:
            raise AuditError(f"invalid license-evidence record: {record!r}")
        evidence_id = record["id"]
        source_name = record["source"]
        path = record["path"]
        if not isinstance(evidence_id, str) or not evidence_id:
            raise AuditError(f"invalid license-evidence ID: {evidence_id!r}")
        if evidence_id in evidence_ids:
            raise AuditError(f"duplicate license-evidence ID: {evidence_id}")
        evidence_ids.add(evidence_id)
        if source_name not in sources or not isinstance(path, str) or not safe_source_path(path):
            raise AuditError(f"invalid license-evidence source: {evidence_id}")
        source_path = (source_name, path)
        if source_path in evidence_sources:
            raise AuditError(f"duplicate license-evidence source path: {source_name}:{path}")
        evidence_sources.add(source_path)
        if not valid_sha256(record["sha256"]):
            raise AuditError(f"invalid license-evidence hash: {evidence_id}")
        if not isinstance(record["kind"], str) or not record["kind"]:
            raise AuditError(f"invalid license-evidence kind: {evidence_id}")

    notices = manifest.get("notice_files")
    if not isinstance(notices, list) or not notices:
        raise AuditError("manifest notice_files must be a nonempty list")
    notice_paths: Set[str] = set()
    for record in notices:
        if not isinstance(record, dict) or set(record) != {"path", "sha256"}:
            raise AuditError(f"invalid notice record: {record!r}")
        path = record["path"]
        if not isinstance(path, str) or not safe_source_path(path):
            raise AuditError(f"unsafe notice path: {path!r}")
        if path in notice_paths:
            raise AuditError(f"duplicate notice path: {path}")
        notice_paths.add(path)
        if not valid_sha256(record["sha256"]):
            raise AuditError(f"invalid notice hash: {path}")

    groups = manifest.get("groups")
    if not isinstance(groups, list) or not groups:
        raise AuditError("manifest groups must be a nonempty list")
    group_names: Set[str] = set()
    retained_names: Set[str] = set()
    excluded_names: Set[str] = set()
    aliases: Set[str] = set()
    source_files: Set[Tuple[str, str]] = set()
    initializer_paths: Set[str] = set()
    payload_members: Set[Tuple[str, str]] = set()
    used_evidence: Set[str] = set()
    base_group_keys = {
        "name",
        "status",
        "source",
        "declaration",
        "license",
        "init_wrapper",
        "payload",
        "files",
    }
    for group in groups:
        if not isinstance(group, dict):
            raise AuditError(f"invalid resource group: {group!r}")
        name = group.get("name")
        status = group.get("status")
        if not isinstance(name, str) or not name or name in group_names:
            raise AuditError(f"invalid or duplicate resource group name: {name!r}")
        group_names.add(name)
        expected_keys = base_group_keys | ({"exclusion_reason"} if status == "excluded" else set())
        if set(group) != expected_keys:
            raise AuditError(f"invalid fields for resource group {name}")
        if status not in {"retained", "excluded"}:
            raise AuditError(f"invalid distribution status for {name}: {status!r}")
        if status == "retained":
            retained_names.add(name)
        else:
            excluded_names.add(name)
            if not isinstance(group["exclusion_reason"], str) or not group["exclusion_reason"]:
                raise AuditError(f"excluded group has no reason: {name}")
        source_name = group.get("source")
        if source_name not in sources:
            raise AuditError(f"unknown source for resource group {name}: {source_name!r}")

        declaration = group.get("declaration")
        declaration_keys = {"cmake_path", "resource_name", "prefix", "base", "file_variable"}
        if not isinstance(declaration, dict) or set(declaration) != declaration_keys:
            raise AuditError(f"invalid resource declaration for {name}")
        if not isinstance(declaration["cmake_path"], str) or not safe_source_path(
            declaration["cmake_path"]
        ):
            raise AuditError(f"unsafe CMake path for {name}")
        if not isinstance(declaration["resource_name"], str) or not declaration["resource_name"]:
            raise AuditError(f"invalid resource name for {name}")
        if not isinstance(declaration["prefix"], str) or not declaration["prefix"].startswith("/"):
            raise AuditError(f"invalid resource prefix for {name}")
        for key in ("base", "file_variable"):
            if declaration[key] is not None and (
                not isinstance(declaration[key], str) or not declaration[key]
            ):
                raise AuditError(f"invalid declaration {key} for {name}")

        license_record = group.get("license")
        if not isinstance(license_record, dict) or set(license_record) != {
            "upstream_expression",
            "selected_distribution_license",
            "evidence_ids",
        }:
            raise AuditError(f"invalid license record for {name}")
        upstream = license_record["upstream_expression"]
        selected = license_record["selected_distribution_license"]
        if upstream is not None and (not isinstance(upstream, str) or not upstream):
            raise AuditError(f"invalid upstream license for {name}")
        group_evidence = string_list(
            license_record["evidence_ids"], f"license evidence for {name}"
        )
        unknown_evidence = set(group_evidence) - evidence_ids
        if unknown_evidence:
            raise AuditError(
                f"unknown license evidence for {name}: {sorted(unknown_evidence)}"
            )
        used_evidence.update(group_evidence)
        if status == "retained":
            if not isinstance(selected, str) or not selected or not upstream:
                raise AuditError(f"retained group has no distribution license: {name}")
            upstream_choices = {part.strip() for part in upstream.split(" OR ")}
            if selected not in upstream_choices:
                raise AuditError(
                    f"selected license for {name} is absent from its upstream expression"
                )
        elif selected is not None:
            raise AuditError(f"excluded group selects a distribution license: {name}")

        init_wrapper = group.get("init_wrapper")
        if not isinstance(init_wrapper, dict) or set(init_wrapper) != {"relative_path"}:
            raise AuditError(f"invalid initializer record for {name}")
        init_path = init_wrapper["relative_path"]
        if not isinstance(init_path, str) or not safe_source_path(init_path):
            raise AuditError(f"unsafe initializer path for {name}")
        if init_path in initializer_paths:
            raise AuditError(f"duplicate initializer path: {init_path}")
        initializer_paths.add(init_path)
        payload = group.get("payload")
        if not isinstance(payload, dict) or set(payload) != {
            "archive_relative_path",
            "member",
        }:
            raise AuditError(f"invalid payload record for {name}")
        archive_path = payload["archive_relative_path"]
        member = payload["member"]
        if not isinstance(archive_path, str) or not safe_source_path(archive_path):
            raise AuditError(f"unsafe payload archive path for {name}")
        if (
            not isinstance(member, str)
            or not member
            or PurePosixPath(member).name != member
        ):
            raise AuditError(f"unsafe payload member for {name}")
        payload_key = (archive_path, member)
        if payload_key in payload_members:
            raise AuditError(f"duplicate payload member: {archive_path}:{member}")
        payload_members.add(payload_key)
        files = group.get("files")
        if not isinstance(files, list) or not files:
            raise AuditError(f"resource group has no files: {name}")
        if files != sorted(files, key=lambda record: record.get("source_path", "")):
            raise AuditError(f"files[] is not source-path sorted for {name}")
        for record in files:
            file_keys = {
                "source_path",
                "resource_alias",
                "media_type",
                "copyright",
            }
            if not isinstance(record, dict) or set(record) != file_keys:
                raise AuditError(f"invalid source-file record in {name}: {record!r}")
            path = record["source_path"]
            alias = record["resource_alias"]
            if not isinstance(path, str) or not safe_source_path(path):
                raise AuditError(f"unsafe source path in {name}: {path!r}")
            source_file = (source_name, path)
            if source_file in source_files:
                raise AuditError(f"duplicate source file: {source_name}:{path}")
            source_files.add(source_file)
            if not isinstance(alias, str) or not alias.startswith("/") or ".." in alias.split("/"):
                raise AuditError(f"unsafe resource alias in {name}: {alias!r}")
            if alias in aliases:
                raise AuditError(f"duplicate runtime resource alias: {alias}")
            aliases.add(alias)
            if not isinstance(record["media_type"], str) or not record["media_type"]:
                raise AuditError(f"invalid media type for {name}:{path}")
            copyrights = record["copyright"]
            if not isinstance(copyrights, list) or any(
                not isinstance(line, str) or not line for line in copyrights
            ):
                raise AuditError(f"invalid copyright lines for {name}:{path}")

    if used_evidence != evidence_ids:
        raise AuditError(
            "license evidence is not referenced by the inventory: "
            f"unused={sorted(evidence_ids - used_evidence)}"
        )

    binary = manifest.get("final_binary")
    binary_keys = {
        "expected_application_resource_names",
        "expected_dependency_resource_names",
        "expected_qinit_resources",
        "expected_qcleanup_resources",
        "forbidden_resource_names",
        "forbidden_symbol_substrings",
    }
    if not isinstance(binary, dict) or set(binary) != binary_keys:
        raise AuditError("invalid final-binary contract")
    for key in binary_keys:
        values = string_list(binary[key], f"final_binary.{key}")
        if values != sorted(values):
            raise AuditError(f"final_binary.{key} is not sorted")
    application = set(binary["expected_application_resource_names"])
    dependencies = set(binary["expected_dependency_resource_names"])
    initialized = sorted(application | dependencies)
    if application & dependencies:
        raise AuditError("application and dependency resource names overlap")
    if dependencies != retained_names:
        raise AuditError("dependency resource names differ from retained groups")
    if set(binary["forbidden_resource_names"]) != excluded_names:
        raise AuditError("forbidden resource names differ from excluded groups")
    if binary["expected_qinit_resources"] != initialized:
        raise AuditError("qInitResources contract is inconsistent")
    if binary["expected_qcleanup_resources"] != initialized:
        raise AuditError("qCleanupResources contract is inconsistent")
    if excluded_names & set(initialized):
        raise AuditError("excluded resource appears in the initialized set")


def open_sources(
    stack: ExitStack,
    paths: Mapping[str, Path],
    pins: Mapping[str, Mapping[str, str]],
) -> Dict[str, TarSource]:
    if set(paths) != set(pins):
        raise AuditError("qtbase, kcolorscheme, and kwidgetsaddons source archives are all required")
    result: Dict[str, TarSource] = {}
    for name, path in paths.items():
        pin = pins[name]
        if path.name != pin["archive_name"] and not path.name.endswith(
            "-" + pin["archive_name"]
        ):
            raise AuditError(
                f"wrong archive name for {name}: {path.name}, expected {pin['archive_name']}"
            )
        actual = file_sha256(path)
        if actual != pin["archive_sha256"]:
            raise AuditError(
                f"source archive hash changed for {name}: {actual}, expected {pin['archive_sha256']}"
            )
        result[name] = stack.enter_context(TarSource(path))
    return result


def audit_sources(
    manifest: Mapping[str, Any], sources: Mapping[str, TarSource]
) -> None:
    expected_groups = source_groups(sources, manifest["groups"])
    actual_groups = manifest["groups"]
    source_keys = (
        "name",
        "status",
        "source",
        "declaration",
        "license",
        "exclusion_reason",
        "files",
    )
    for expected, actual in zip(expected_groups, actual_groups):
        expected_view = {key: expected.get(key) for key in source_keys}
        actual_view = {key: actual.get(key) for key in source_keys}
        if actual_view != expected_view:
            raise AuditError(f"source-derived manifest data changed for {expected['name']}")
    if manifest["license_evidence"] != evidence_records(
        sources, manifest["license_evidence"]
    ):
        raise AuditError("source license evidence differs from manifest")


def ninja_logical_lines(text: str) -> List[str]:
    result: List[str] = []
    pending = ""
    for physical in text.splitlines():
        if physical.endswith("$") and not physical.endswith("$$"):
            pending += physical[:-1]
            continue
        result.append(pending + physical)
        pending = ""
    if pending:
        result.append(pending)
    return result


def final_link_tokens(binary: Path, build_ninja: Path) -> Tuple[List[str], str]:
    try:
        text = build_ninja.read_text(encoding="utf-8")
    except OSError as exc:
        raise AuditError(f"cannot read Ninja graph {build_ninja}: {exc}") from exc
    lines = ninja_logical_lines(text)
    try:
        relative = str(binary.resolve().relative_to(build_ninja.parent.resolve()))
    except ValueError:
        relative = ""
    candidates = [relative, "bin/LibrePaint.app/LibrePaint"]
    indices = []
    for index, line in enumerate(lines):
        if not line.startswith("build ") or "EXECUTABLE_LINKER" not in line:
            continue
        output = line[6:].split(":", 1)[0].split()[0]
        if output in candidates or output.endswith("/" + binary.name):
            indices.append(index)
    if len(indices) != 1:
        raise AuditError(f"expected one final executable edge in {build_ninja}; found {len(indices)}")
    index = indices[0]
    stanza = [lines[index]]
    for line in lines[index + 1 :]:
        if line and not line[0].isspace():
            break
        stanza.append(line)
    link_values = [line.split("=", 1)[1] for line in stanza if line.startswith("  LINK_LIBRARIES =")]
    if len(link_values) != 1:
        raise AuditError("final Ninja edge has no unique LINK_LIBRARIES value")
    try:
        tokens = shlex.split(link_values[0], posix=True)
    except ValueError as exc:
        raise AuditError(f"cannot parse final LINK_LIBRARIES: {exc}") from exc
    return tokens, "\n".join(stanza)


def resolve_link_path(token: str, build_dir: Path) -> Path:
    path = Path(token)
    return path if path.is_absolute() else build_dir / path


def link_matches(tokens: Sequence[str], relative_path: str) -> List[str]:
    suffix = "/" + relative_path
    return [token for token in tokens if token == relative_path or token.endswith(suffix)]


def require_ar_members(path: Path, targets: Set[str]) -> None:
    found: Set[str] = set()
    try:
        with path.open("rb") as handle:
            if handle.read(8) != b"!<arch>\n":
                raise AuditError(f"not an ar archive: {path}")
            long_names = b""
            while True:
                header = handle.read(60)
                if not header:
                    break
                if len(header) != 60 or header[58:60] != b"`\n":
                    raise AuditError(f"malformed ar member header: {path}")
                try:
                    size = int(header[48:58].decode("ascii").strip())
                except (UnicodeDecodeError, ValueError) as exc:
                    raise AuditError(f"invalid ar member size: {path}") from exc
                raw_name = header[:16].decode("utf-8", "replace").strip()
                payload_size = size
                name = raw_name.rstrip("/")
                extended_name_bytes = 0
                if raw_name.startswith("#1/"):
                    try:
                        extended_name_bytes = int(raw_name[3:])
                    except ValueError as exc:
                        raise AuditError(f"invalid BSD ar member name: {path}") from exc
                    name_bytes = handle.read(extended_name_bytes)
                    if len(name_bytes) != extended_name_bytes:
                        raise AuditError(f"short BSD ar member name: {path}")
                    name = name_bytes.rstrip(b"\0").decode("utf-8", "replace")
                    payload_size -= extended_name_bytes
                elif raw_name == "//":
                    long_names = handle.read(size)
                    payload_size = 0
                elif raw_name.startswith("/") and raw_name[1:].strip().isdigit():
                    offset = int(raw_name[1:].strip())
                    end = long_names.find(b"/\n", offset)
                    if end < 0:
                        raise AuditError(f"invalid GNU ar long-name offset: {path}")
                    name = long_names[offset:end].decode("utf-8", "replace")

                if name in targets:
                    if name in found:
                        raise AuditError(f"duplicate ar member {name}: {path}")
                    data = handle.read(payload_size)
                    if len(data) != payload_size:
                        raise AuditError(f"short ar member {name}: {path}")
                    found.add(name)
                elif payload_size:
                    handle.seek(payload_size, os.SEEK_CUR)
                if size % 2:
                    handle.seek(1, os.SEEK_CUR)
    except OSError as exc:
        raise AuditError(f"cannot read archive {path}: {exc}") from exc
    missing = targets - found
    if missing:
        raise AuditError(f"archive {path} is missing members: {sorted(missing)}")


def audit_link_artifacts(
    manifest: Mapping[str, Any], binary: Path, build_ninja: Path
) -> None:
    tokens, stanza = final_link_tokens(binary, build_ninja)
    groups = manifest["groups"]
    roots: Dict[str, Path] = {}
    for group in groups:
        name = group["name"]
        relative = group["init_wrapper"]["relative_path"]
        matches = link_matches(tokens, relative)
        if group["status"] == "retained":
            if len(matches) != 1:
                raise AuditError(f"final link has {len(matches)} initializer wrappers for {name}")
            path = resolve_link_path(matches[0], build_ninja.parent)
            if not path.is_file():
                raise AuditError(f"initializer wrapper is missing for {name}: {path}")
            suffix = "/" + relative
            root_text = str(path.resolve())[: -len(suffix)] if str(path.resolve()).endswith(suffix) else ""
            if not root_text:
                raise AuditError(f"cannot resolve package root for {name}: {path}")
            roots[name] = Path(root_text)
        elif matches or relative in stanza:
            raise AuditError(f"excluded initializer is still in the final link: {name}")

    archive_targets: Dict[Path, Set[str]] = {}
    for group in groups:
        if group["status"] != "retained":
            continue
        name = group["name"]
        payload = group["payload"]
        archive = roots[name] / payload["archive_relative_path"]
        archive_targets.setdefault(archive, set()).add(payload["member"])
        if not link_matches(tokens, payload["archive_relative_path"]):
            raise AuditError(f"payload archive is absent from final link for {name}: {archive}")
    for archive, members in archive_targets.items():
        require_ar_members(archive, members)


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


def source_arg_paths(args: argparse.Namespace) -> Dict[str, Path]:
    values = {
        "qtbase": args.qtbase_source_tar,
        "kcolorscheme": args.kcolorscheme_source_tar,
        "kwidgetsaddons": args.kwidgetsaddons_source_tar,
    }
    supplied = {name: path.resolve() for name, path in values.items() if path is not None}
    if supplied and len(supplied) != len(values):
        raise AuditError("all three --*-source-tar arguments must be supplied together")
    return supplied


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--qtbase-source-tar", type=Path)
    parser.add_argument("--kcolorscheme-source-tar", type=Path)
    parser.add_argument("--kwidgetsaddons-source-tar", type=Path)
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--build-ninja", type=Path)
    parser.add_argument(
        "--nm",
        default="nm",
        help="nm executable used for Mach-O inspection (default: nm)",
    )
    parser.add_argument(
        "--generate-manifest",
        action="store_true",
        help=(
            "refresh source-derived records from all three pinned source tarballs, "
            "using --manifest for artifact and binary policy"
        ),
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="generation output (defaults to --manifest)",
    )
    args = parser.parse_args()

    try:
        pins = canonical_source_pins()
        source_paths = source_arg_paths(args)
        if args.generate_manifest and not source_paths:
            raise AuditError("--generate-manifest requires all three --*-source-tar arguments")
        if (args.binary is None) != (args.build_ninja is None):
            raise AuditError("--binary and --build-ninja must be supplied together")

        with ExitStack() as stack:
            sources = open_sources(stack, source_paths, pins) if source_paths else {}
            if args.generate_manifest:
                template = load_json(args.manifest.resolve())
                check_manifest_structure(template)
                manifest = generated_manifest(sources, pins, template)
                check_manifest_structure(manifest)
                check_source_pins(manifest)
                check_notice_files(manifest)
                output = (args.output or args.manifest).resolve()
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_text(
                    json.dumps(manifest, indent=2, ensure_ascii=False) + "\n",
                    encoding="utf-8",
                )
                print(f"generated static dependency resource manifest: {output}")
            else:
                manifest = load_json(args.manifest.resolve())
                check_manifest_structure(manifest)
                check_source_pins(manifest)
                check_notice_files(manifest)
                if sources:
                    audit_sources(manifest, sources)

        if args.binary is not None and args.build_ninja is not None:
            audit_link_artifacts(
                manifest, args.binary.resolve(), args.build_ninja.resolve()
            )
            audit_binary(manifest, args.binary.resolve(), args.nm)
        retained = [group for group in manifest["groups"] if group["status"] == "retained"]
        excluded = [group for group in manifest["groups"] if group["status"] == "excluded"]
        retained_files = [record for group in retained for record in group["files"]]
        retained_images = sum(
            record["media_type"].startswith("image/") for record in retained_files
        )
        print(
            "static dependency resource audit: "
            f"{len(retained)} retained groups, {len(retained_files)} files "
            f"({retained_images} images), {len(excluded)} excluded "
            f"group{'s' if len(excluded) != 1 else ''}; "
            "unclassified=0"
        )
        return 0
    except AuditError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
