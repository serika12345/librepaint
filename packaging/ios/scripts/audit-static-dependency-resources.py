#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Seal the non-code resources pulled from static Qt and KF6 libraries on iOS."""

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
from typing import Any, Dict, Iterable, List, Mapping, Optional, Sequence, Set, Tuple


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

GROUP_ORDER = (
    "qpdf",
    "gui_shaders",
    "qstyle",
    "qstyle1",
    "qstyle_fusion",
    "qmessagebox",
    "color_schemes",
    "kcharselect_data",
)
RETAINED_GROUPS = {
    "qpdf",
    "gui_shaders",
    "qstyle",
    "qstyle1",
    "qstyle_fusion",
    "qmessagebox",
    "color_schemes",
}
EXCLUDED_GROUPS = {"kcharselect_data"}
EXPECTED_GROUP_COUNTS = {
    "qpdf": 1,
    "gui_shaders": 2,
    "qstyle": 203,
    "qstyle1": 26,
    "qstyle_fusion": 18,
    "qmessagebox": 1,
    "color_schemes": 2,
    "kcharselect_data": 1,
}

# These are the resources owned by the application and by dependencies outside
# this narrowly audited QtBase/KColorScheme set. They are intentionally fixed:
# the final-binary test must not learn a changed set from the binary it audits.
APPLICATION_RESOURCE_NAMES = {
    "aboutdata",
    "branding",
    "breeze_dark_icons",
    "breeze_light_icons",
    "cursors",
    "defaultpresets",
    "defaulttools",
    "flake",
    "gamutmasks",
    "icons",
    "karbontools",
    "krita",
    "kritawidgets",
    "kxmlgui",
    "layerbox_icons",
    "layers_icons",
    "mime_database",
    "misc_dark_icons",
    "misc_light_icons",
    "paintops_icons",
    "pathshapes",
    "polyline",
    "selectiontools",
    "shaders",
    "splash",
    "sql",
    "svg_icons",
    "tool_polygon",
    "tool_transform",
    "tool_transform_icons",
    "toolcrop",
    "tools_svg_16_icons",
}

EXPECTED_RETAINED_ROWS_SHA256 = (
    "47bdb8a5d999c997d92d58c1ed6f1a4b9a634845a908c700eebdfe8dd1252e2a"
)
EXPECTED_RETAINED_GROUP_PATHS_SHA256 = (
    "5c38e08615a236ed5d92e4c470b28b7f374a287d320350483b896afd97cc041b"
)
EXPECTED_EXCLUDED_ROWS_SHA256 = (
    "24b2a0144ca84f30fb66d38a89f6ef79f48b6e2f7e62c9eb7619eb5188d15d3c"
)
EXPECTED_CLASSIFIED_GROUPS_SHA256 = (
    "01517ce02cce0e412b6499abb2bd23d6542a7273483532d70eeeb33e0846ca30"
)

ARTIFACTS: Dict[str, Dict[str, Dict[str, str]]] = {
    "qpdf": {
        "init_wrapper": {
            "relative_path": "lib/objects-Release/Gui_resources_1/.qt/rcc/qrc_qpdf_init.cpp.o",
            "sha256": "4afd210e6009f6e190a3a6905336eeadc4ffca5844ca4fee56296b2881c7b0fb",
        },
        "payload": {
            "archive_relative_path": "lib/libQt6Gui.a",
            "member": "qrc_qpdf.cpp.o",
            "sha256": "d33341b88d77dba982eeb4210ae1e2017c92dca4997ecdcfd2d4b3c344173713",
        },
    },
    "gui_shaders": {
        "init_wrapper": {
            "relative_path": "lib/objects-Release/Gui_resources_2/.qt/rcc/qrc_gui_shaders_init.cpp.o",
            "sha256": "5218ca6e1ca4a2c4ba7a26031c5c71baa23c7e6e2147ad05c80f78899fcb60ab",
        },
        "payload": {
            "archive_relative_path": "lib/libQt6Gui.a",
            "member": "qrc_gui_shaders.cpp.o",
            "sha256": "22da096b123f518071c1003ef72e44a075e0c5f7305f29f5cf61fbfdeeaff084",
        },
    },
    "qstyle": {
        "init_wrapper": {
            "relative_path": "lib/objects-Release/Widgets_resources_1/.qt/rcc/qrc_qstyle_init.cpp.o",
            "sha256": "c5fa0fda87d10876627d841c8c2bbe5230900bfe60907c41ffaacab0c05407be",
        },
        "payload": {
            "archive_relative_path": "lib/libQt6Widgets.a",
            "member": "qrc_qstyle.cpp.o",
            "sha256": "08defc643961f385d0eb777fe9786346bb5ee6701eb6c9c79392b9027924358c",
        },
    },
    "qstyle1": {
        "init_wrapper": {
            "relative_path": "lib/objects-Release/Widgets_resources_2/.qt/rcc/qrc_qstyle1_init.cpp.o",
            "sha256": "11745378f5e0752f3aba5974e3df4200ea49328e5de409954acea3fcfdfd1e7f",
        },
        "payload": {
            "archive_relative_path": "lib/libQt6Widgets.a",
            "member": "qrc_qstyle1.cpp.o",
            "sha256": "99cc9762f7953c906ccca1fc77f2dccdec5428248bbb514822ade668d3db0b66",
        },
    },
    "qstyle_fusion": {
        "init_wrapper": {
            "relative_path": "lib/objects-Release/Widgets_resources_3/.qt/rcc/qrc_qstyle_fusion_init.cpp.o",
            "sha256": "9a7c368380354a46dfa7b7d871461975a72ac93f54f4fde90725f661d9d19489",
        },
        "payload": {
            "archive_relative_path": "lib/libQt6Widgets.a",
            "member": "qrc_qstyle_fusion.cpp.o",
            "sha256": "bccdc41f5676aedf52896f775aa72586c29de57eb95f44f35a83e17ba72db2f1",
        },
    },
    "qmessagebox": {
        "init_wrapper": {
            "relative_path": "lib/objects-Release/Widgets_resources_4/.qt/rcc/qrc_qmessagebox_init.cpp.o",
            "sha256": "5d186b3a3acda0d9e15944ecb37111590bfca8cf99a1e4ec5ee753c35e7e9cf2",
        },
        "payload": {
            "archive_relative_path": "lib/libQt6Widgets.a",
            "member": "qrc_qmessagebox.cpp.o",
            "sha256": "76ba4f77b5fdedb63adfa88d8d54ed5b3d06cecf3babe69882a2ae3210ac7f29",
        },
    },
    "color_schemes": {
        "init_wrapper": {
            "relative_path": "lib/objects-Release/KF6ColorScheme_resources_1/.qt/rcc/qrc_color_schemes_init.cpp.o",
            "sha256": "4ee22a8a9030de77dc353a5ced97e58be3d3c32acfecd0eb47cbf808d5640ace",
        },
        "payload": {
            "archive_relative_path": "lib/libKF6ColorScheme.a",
            "member": "qrc_color_schemes.cpp.o",
            "sha256": "667b5441f51595188629aceadc4cb9983d4edea0cd448a22a72bf31dba783944",
        },
    },
    "kcharselect_data": {
        "init_wrapper": {
            "relative_path": "lib/objects-Release/KF6WidgetsAddons_resources_1/.qt/rcc/qrc_kcharselect-data_init.cpp.o",
            "sha256": "1fc38e3bab871be9823659621f50aa0dd560700e69ebc9661639956b6d45efe9",
        },
        "payload": {
            "archive_relative_path": "lib/libKF6WidgetsAddons.a",
            "member": "qrc_kcharselect-data.cpp.o",
            "sha256": "8ae31eca2b432bf215c14dbbbb953a9acd45d1391d0aa09eab82b4bfeb8348ce",
        },
    },
}

EVIDENCE_SPECS = {
    "qt-widgets-reuse": (
        "qtbase",
        "src/widgets/REUSE.toml",
        "6e9816140116f820d8f7dc8871e3652138d1e673c25e1009c828313c581935e2",
        "REUSE annotation",
    ),
    "qt-shaders-reuse": (
        "qtbase",
        "src/gui/painting/shaders/REUSE.toml",
        "aeb9636baea3fd036d21e8aa7df8794c92b4f04bd7a254acc4a50fd9b6b27f35",
        "REUSE annotation",
    ),
    "icc-reuse": (
        "qtbase",
        "src/3rdparty/icc/REUSE.toml",
        "c92a340ee1f5706a5424885e2360a57b6e629aee75dbb266df035bb8b48a06bf",
        "REUSE annotation",
    ),
    "icc-license": (
        "qtbase",
        "src/3rdparty/icc/LICENSE.txt",
        "9e34ff47b3a44814e183342e7e198d127a6740e29d20e8381fba7688dafc79dd",
        "verbatim license notice",
    ),
    "icc-attribution": (
        "qtbase",
        "src/3rdparty/icc/qt_attribution.json",
        "509da9bd2b3f769c730aa4627b1080d65f39b0ecd4130111c9e1122e13cd35b5",
        "Qt attribution record",
    ),
    "breeze-dark-inline-spdx": (
        "kcolorscheme",
        "src/color-schemes/BreezeDark.colors",
        "1beba17e90c6b441c40a73108aeaccf4c2ee13845fa06b2c7449f3e8d04e6107",
        "in-file SPDX headers",
    ),
    "breeze-light-inline-spdx": (
        "kcolorscheme",
        "src/color-schemes/BreezeLight.colors",
        "63ffd2c6fc0a4225a7337e55c6b666ca1517ab11365f34f0b63e3e6629febeef",
        "in-file SPDX headers",
    ),
    "kcharselect-generator": (
        "kwidgetsaddons",
        "src/kcharselect-generate-datafile.py",
        "8d6cc202622e0aae07b09b67be2c0a8114176e1c8f6429a673d1f2640dfbc0b5",
        "generator LGPL header and Unicode-derived input references; insufficient distribution provenance",
    ),
}

NOTICE_PATHS = (
    "LICENSES/CC0-1.0.txt",
    "LICENSES/GPL-3.0-only.txt",
    "LICENSES/LGPL-2.0-or-later.txt",
    "LICENSES/LGPL-3.0-only.txt",
    "LICENSES/LGPL-3.0-or-later.txt",
    "LICENSES/LicenseRef-ICC-License.txt",
    "packaging/ios/manifests/non-code-licenses.md",
    "packaging/ios/notices/librepaint-brand-assets.md",
    "packaging/ios/notices/qtbase-icc-attribution.json",
    "packaging/ios/notices/retained-functional-assets.md",
)


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
    license_expression: Optional[str],
    copyright_lines: Sequence[str],
) -> Dict[str, Any]:
    data = source.read(source_path)
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
        "size": len(data),
        "sha256": sha256(data),
        "media_type": media_types.get(suffix, "application/octet-stream"),
        "license": license_expression,
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


def evidence_records(sources: Mapping[str, TarSource]) -> List[Dict[str, str]]:
    records: List[Dict[str, str]] = []
    for evidence_id, (source_name, path, expected_hash, kind) in EVIDENCE_SPECS.items():
        actual = sha256(sources[source_name].read(path))
        if actual != expected_hash:
            raise AuditError(
                f"license evidence changed: {source_name}:{path} ({actual}, expected {expected_hash})"
            )
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


def source_groups(sources: Mapping[str, TarSource]) -> List[Dict[str, Any]]:
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
        group: Dict[str, Any] = {
            "name": name,
            "status": "retained" if name in RETAINED_GROUPS else "excluded",
            "source": source_name,
            "declaration": {
                "cmake_path": cmake_path,
                "resource_name": cmake_resource_name,
                "prefix": prefix,
                "base": base,
                "file_variable": file_variable,
            },
            "file_count": len(files),
            "source_bytes": sum(record["size"] for record in files),
            "license": {
                "upstream_expression": license_expression,
                "selected_distribution_license": (
                    None
                    if name in EXCLUDED_GROUPS
                    else (
                        "LGPL-3.0-only"
                        if license_expression == QT_LICENSE
                        else license_expression
                    )
                ),
                "evidence_ids": list(evidence_ids),
            },
            "init_wrapper": ARTIFACTS[name]["init_wrapper"],
            "payload": ARTIFACTS[name]["payload"],
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
                    QT_LICENSE,
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
                ICC_LICENSE,
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
                QT_LICENSE,
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
                QT_LICENSE,
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
                license_expression,
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
                None,
                (),
            )
        ],
        None,
        ("kcharselect-generator",),
        "Unused KCharSelect payload; the pinned archive does not carry a self-contained license mapping for its generated Unicode data.",
    )

    for name, count in EXPECTED_GROUP_COUNTS.items():
        if len(groups[name]["files"]) != count:
            raise AuditError(
                f"resource count changed for {name}: {len(groups[name]['files'])}, expected {count}"
            )
    return [groups[name] for name in GROUP_ORDER]


def load_json_from_bytes(data: bytes, label: str) -> Any:
    try:
        return json.loads(data.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise AuditError(f"invalid JSON in {label}: {exc}") from exc


def inventory_digest(groups: Iterable[Mapping[str, Any]], include_alias_hash: bool) -> str:
    rows: List[str] = []
    for group in groups:
        for record in group["files"]:
            fields = [group["name"], record["source_path"]]
            if include_alias_hash:
                fields.extend((record["resource_alias"], record["sha256"]))
            rows.append("\t".join(fields))
    return sha256(("\n".join(sorted(rows)) + "\n").encode("utf-8"))


def inventory_summary(groups: Sequence[Mapping[str, Any]]) -> Dict[str, Any]:
    retained = [group for group in groups if group["status"] == "retained"]
    excluded = [group for group in groups if group["status"] == "excluded"]
    retained_files = [record for group in retained for record in group["files"]]
    excluded_files = [record for group in excluded for record in group["files"]]
    return {
        "retained_group_names": sorted(group["name"] for group in retained),
        "excluded_group_names": sorted(group["name"] for group in excluded),
        "retained_file_count": len(retained_files),
        "retained_source_bytes": sum(record["size"] for record in retained_files),
        "retained_image_count": sum(
            record["media_type"].startswith("image/") for record in retained_files
        ),
        "retained_group_paths_sha256": inventory_digest(retained, False),
        "retained_rows_sha256": inventory_digest(retained, True),
        "excluded_file_count": len(excluded_files),
        "excluded_source_bytes": sum(record["size"] for record in excluded_files),
        "excluded_rows_sha256": inventory_digest(excluded, True),
        "classified_groups_sha256": classified_groups_digest(groups),
    }


def classified_groups_digest(groups: Sequence[Mapping[str, Any]]) -> str:
    """Seal every source-derived field, including all legal metadata."""

    keys = (
        "name",
        "status",
        "source",
        "declaration",
        "file_count",
        "source_bytes",
        "license",
        "exclusion_reason",
        "files",
    )
    records = [{key: group.get(key) for key in keys} for group in groups]
    payload = json.dumps(
        records, sort_keys=True, separators=(",", ":"), ensure_ascii=True
    ).encode("utf-8")
    return sha256(payload)


def notice_records() -> List[Dict[str, str]]:
    records = []
    for relative in NOTICE_PATHS:
        path = REPO_ROOT / relative
        if not path.is_file():
            raise AuditError(f"required notice is missing: {relative}")
        records.append({"path": relative, "sha256": file_sha256(path)})
    return records


def generated_manifest(
    sources: Mapping[str, TarSource], pins: Mapping[str, Mapping[str, str]]
) -> Dict[str, Any]:
    groups = source_groups(sources)
    expected_resources = sorted(APPLICATION_RESOURCE_NAMES | RETAINED_GROUPS)
    return {
        "schema": 1,
        "scope": MANIFEST_SCOPE,
        "sources": dict(pins),
        "license_evidence": evidence_records(sources),
        "notice_files": notice_records(),
        "inventory": inventory_summary(groups),
        "final_binary": {
            "expected_application_resource_names": sorted(APPLICATION_RESOURCE_NAMES),
            "expected_dependency_resource_names": sorted(RETAINED_GROUPS),
            "expected_qinit_resources": expected_resources,
            "expected_qcleanup_resources": expected_resources,
            "forbidden_resource_names": sorted(EXCLUDED_GROUPS),
            "forbidden_symbol_substrings": ["KCharSelect", "KCharSelectData"],
        },
        "groups": groups,
    }


def check_source_pins(manifest: Mapping[str, Any]) -> Dict[str, Dict[str, str]]:
    canonical = canonical_source_pins()
    if manifest.get("sources") != canonical:
        raise AuditError("static-resource source pins differ from canonical Qt/KF manifests")
    return canonical


def check_notice_files(manifest: Mapping[str, Any]) -> None:
    records = manifest.get("notice_files")
    if not isinstance(records, list):
        raise AuditError("manifest notice_files must be a list")
    paths = [record.get("path") for record in records if isinstance(record, dict)]
    if sorted(paths) != sorted(NOTICE_PATHS) or len(paths) != len(set(paths)):
        raise AuditError(f"notice-file set changed: {paths}")
    for record in records:
        path = REPO_ROOT / record["path"]
        if not path.is_file():
            raise AuditError(f"required notice is missing: {record['path']}")
        actual = file_sha256(path)
        if actual != record.get("sha256"):
            raise AuditError(
                f"notice changed: {record['path']} ({actual}, expected {record.get('sha256')})"
            )


def check_manifest_structure(manifest: Mapping[str, Any]) -> None:
    if manifest.get("schema") != 1:
        raise AuditError(f"unsupported static-resource schema: {manifest.get('schema')}")
    if manifest.get("scope") != MANIFEST_SCOPE:
        raise AuditError("static-resource manifest scope changed")
    check_source_pins(manifest)
    check_notice_files(manifest)

    evidence = manifest.get("license_evidence")
    if not isinstance(evidence, list):
        raise AuditError("manifest license_evidence must be a list")
    expected_evidence = [
        {
            "id": evidence_id,
            "source": source,
            "path": path,
            "sha256": digest,
            "kind": kind,
        }
        for evidence_id, (source, path, digest, kind) in EVIDENCE_SPECS.items()
    ]
    if evidence != expected_evidence:
        raise AuditError("license-evidence set or hash changed")

    groups = manifest.get("groups")
    if not isinstance(groups, list) or [group.get("name") for group in groups] != list(
        GROUP_ORDER
    ):
        raise AuditError("resource group order/set changed")
    aliases: Set[str] = set()
    for group in groups:
        name = group["name"]
        expected_status = "retained" if name in RETAINED_GROUPS else "excluded"
        if group.get("status") != expected_status:
            raise AuditError(f"wrong distribution status for {name}")
        files = group.get("files")
        if not isinstance(files, list) or len(files) != EXPECTED_GROUP_COUNTS[name]:
            raise AuditError(f"wrong files[] count for {name}")
        if group.get("file_count") != len(files):
            raise AuditError(f"file_count mismatch for {name}")
        if group.get("source_bytes") != sum(record.get("size", -1) for record in files):
            raise AuditError(f"source byte count mismatch for {name}")
        if group.get("init_wrapper") != ARTIFACTS[name]["init_wrapper"]:
            raise AuditError(f"initializer artifact changed for {name}")
        if group.get("payload") != ARTIFACTS[name]["payload"]:
            raise AuditError(f"payload artifact changed for {name}")
        if files != sorted(files, key=lambda record: record.get("source_path", "")):
            raise AuditError(f"files[] is not source-path sorted for {name}")
        for record in files:
            path = record.get("source_path")
            alias = record.get("resource_alias")
            digest = record.get("sha256")
            if not isinstance(path, str) or not safe_source_path(path):
                raise AuditError(f"unsafe source path in {name}: {path}")
            if not isinstance(alias, str) or not alias.startswith("/") or ".." in alias.split("/"):
                raise AuditError(f"unsafe resource alias in {name}: {alias}")
            if alias in aliases:
                raise AuditError(f"duplicate runtime resource alias: {alias}")
            aliases.add(alias)
            if not isinstance(digest, str) or not re.fullmatch(r"[0-9a-f]{64}", digest):
                raise AuditError(f"invalid source hash for {name}:{path}")
            if not isinstance(record.get("size"), int) or record["size"] < 0:
                raise AuditError(f"invalid source size for {name}:{path}")
            if expected_status == "retained" and not record.get("license"):
                raise AuditError(f"retained source has no license: {name}:{path}")

    expected_summary = inventory_summary(groups)
    if manifest.get("inventory") != expected_summary:
        raise AuditError("manifest inventory summary is not self-consistent")
    if expected_summary != {
        "retained_group_names": sorted(RETAINED_GROUPS),
        "excluded_group_names": sorted(EXCLUDED_GROUPS),
        "retained_file_count": 253,
        "retained_source_bytes": 375390,
        "retained_image_count": 248,
        "retained_group_paths_sha256": EXPECTED_RETAINED_GROUP_PATHS_SHA256,
        "retained_rows_sha256": EXPECTED_RETAINED_ROWS_SHA256,
        "excluded_file_count": 1,
        "excluded_source_bytes": 3170758,
        "excluded_rows_sha256": EXPECTED_EXCLUDED_ROWS_SHA256,
        "classified_groups_sha256": EXPECTED_CLASSIFIED_GROUPS_SHA256,
    }:
        raise AuditError("closed static-resource inventory changed")

    binary = manifest.get("final_binary")
    expected_all = sorted(APPLICATION_RESOURCE_NAMES | RETAINED_GROUPS)
    expected_binary = {
        "expected_application_resource_names": sorted(APPLICATION_RESOURCE_NAMES),
        "expected_dependency_resource_names": sorted(RETAINED_GROUPS),
        "expected_qinit_resources": expected_all,
        "expected_qcleanup_resources": expected_all,
        "forbidden_resource_names": sorted(EXCLUDED_GROUPS),
        "forbidden_symbol_substrings": ["KCharSelect", "KCharSelectData"],
    }
    if binary != expected_binary or len(expected_all) != 39:
        raise AuditError("final Mach-O resource-symbol contract changed")


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
    expected_groups = source_groups(sources)
    actual_groups = manifest["groups"]
    source_keys = (
        "name",
        "status",
        "source",
        "declaration",
        "file_count",
        "source_bytes",
        "license",
        "exclusion_reason",
        "files",
    )
    for expected, actual in zip(expected_groups, actual_groups):
        expected_view = {key: expected.get(key) for key in source_keys}
        actual_view = {key: actual.get(key) for key in source_keys}
        if actual_view != expected_view:
            raise AuditError(f"source-derived manifest data changed for {expected['name']}")
    if manifest["license_evidence"] != evidence_records(sources):
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
    candidates = [relative, "bin/krita.app/krita"]
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


def read_ar_selected(path: Path, targets: Set[str]) -> Dict[str, bytes]:
    found: Dict[str, bytes] = {}
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
                    found[name] = data
                elif payload_size:
                    handle.seek(payload_size, os.SEEK_CUR)
                if size % 2:
                    handle.seek(1, os.SEEK_CUR)
    except OSError as exc:
        raise AuditError(f"cannot read archive {path}: {exc}") from exc
    missing = targets - set(found)
    if missing:
        raise AuditError(f"archive {path} is missing members: {sorted(missing)}")
    return found


def audit_link_artifacts(
    manifest: Mapping[str, Any], binary: Path, build_ninja: Path
) -> None:
    tokens, stanza = final_link_tokens(binary, build_ninja)
    groups = {group["name"]: group for group in manifest["groups"]}
    roots: Dict[str, Path] = {}
    for name in GROUP_ORDER:
        group = groups[name]
        relative = group["init_wrapper"]["relative_path"]
        matches = link_matches(tokens, relative)
        if name in RETAINED_GROUPS:
            if len(matches) != 1:
                raise AuditError(f"final link has {len(matches)} initializer wrappers for {name}")
            path = resolve_link_path(matches[0], build_ninja.parent)
            actual = file_sha256(path)
            expected = group["init_wrapper"]["sha256"]
            if actual != expected:
                raise AuditError(f"initializer wrapper changed for {name}: {actual}, expected {expected}")
            suffix = "/" + relative
            root_text = str(path.resolve())[: -len(suffix)] if str(path.resolve()).endswith(suffix) else ""
            if not root_text:
                raise AuditError(f"cannot resolve package root for {name}: {path}")
            roots[name] = Path(root_text)
        elif matches or relative in stanza:
            raise AuditError(f"excluded initializer is still in the final link: {name}")

    archive_targets: Dict[Path, Set[str]] = {}
    archive_expected: Dict[Tuple[Path, str], str] = {}
    for name in sorted(RETAINED_GROUPS):
        payload = groups[name]["payload"]
        archive = roots[name] / payload["archive_relative_path"]
        archive_targets.setdefault(archive, set()).add(payload["member"])
        archive_expected[(archive, payload["member"])] = payload["sha256"]
        if not link_matches(tokens, payload["archive_relative_path"]):
            raise AuditError(f"payload archive is absent from final link for {name}: {archive}")
    for archive, members in archive_targets.items():
        data = read_ar_selected(archive, members)
        for member, payload in data.items():
            actual = sha256(payload)
            expected = archive_expected[(archive, member)]
            if actual != expected:
                raise AuditError(
                    f"resource payload changed: {archive}:{member} ({actual}, expected {expected})"
                )


def resource_symbol_counts(nm_output: str, operation: str) -> Counter:
    pattern = re.compile(r"q" + operation + r"Resources_([A-Za-z0-9_]+)v$")
    result: Counter = Counter()
    for line in nm_output.splitlines():
        match = pattern.search(line)
        if match:
            result[match.group(1)] += 1
    return result


def resource_symbols(nm_output: str, operation: str) -> Set[str]:
    return set(resource_symbol_counts(nm_output, operation))


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
        help="generate the manifest from all three pinned source tarballs",
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
                manifest = generated_manifest(sources, pins)
                check_manifest_structure(manifest)
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
                if sources:
                    audit_sources(manifest, sources)

        if args.binary is not None and args.build_ninja is not None:
            audit_link_artifacts(
                manifest, args.binary.resolve(), args.build_ninja.resolve()
            )
            audit_binary(manifest, args.binary.resolve(), args.nm)
        print(
            "static dependency resource audit: 7 retained groups, "
            "253 files (248 images), 1 excluded group; unclassified=0"
        )
        return 0
    except AuditError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
