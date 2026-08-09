#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Audit licenses for images linked or installed by the iOS build."""

from __future__ import annotations

import argparse
import gzip
import json
import os
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_MANIFEST = REPO_ROOT / "packaging/ios/manifests/ios-image-licenses.json"

QRC_IMAGE_SUFFIXES = {
    ".bmp",
    ".gif",
    ".icns",
    ".ico",
    ".jpeg",
    ".jpg",
    ".png",
    ".svg",
    ".svgz",
    ".webp",
    ".xpm",
}
CC_NAMESPACE = "http://creativecommons.org/ns#"
RDF_NAMESPACE = "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
CMAKE_INSTALL_FILES_RE = re.compile(
    r"\binstall\s*\(\s*FILES\b(.*?)\bDESTINATION\b",
    re.IGNORECASE | re.DOTALL,
)
CMAKE_COMMENT_RE = re.compile(r"#[^\r\n]*")
CMAKE_TOKEN_RE = re.compile(r"(?:\\.|[^\s])+")
STATIC_RESOURCE_REGISTRATION_RE = re.compile(
    r"\bkrita_ios_add_static_resource\s*\(\s*([A-Za-z0-9_+-]+)\s*\)"
)
LITERAL_QRC_RE = re.compile(
    r"(?<![A-Za-z0-9_./+-])([A-Za-z0-9_./+-]+\.qrc)(?![A-Za-z0-9_./+-])"
)


class AuditError(RuntimeError):
    """The image inventory or its license classification changed."""


def repo_path(relative: str) -> Path:
    path = (REPO_ROOT / relative).resolve()
    try:
        path.relative_to(REPO_ROOT)
    except ValueError as exc:
        raise AuditError(f"path escapes repository: {relative}") from exc
    return path


def relative_path(path: Path) -> str:
    return path.resolve().relative_to(REPO_ROOT).as_posix()


def load_manifest(path: Path) -> dict[str, Any]:
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("schema") != 1:
        raise AuditError(f"unsupported manifest schema in {path}")
    return manifest


def require_unique_paths(paths: list[str], label: str) -> None:
    if len(paths) != len(set(paths)):
        raise AuditError(f"duplicate path in {label}")


def core_qrc_paths_from_cmake() -> list[str]:
    top_level = CMAKE_COMMENT_RE.sub(
        "", repo_path("CMakeLists.txt").read_text(encoding="utf-8")
    )
    branding_match = re.search(
        r'\bset\s*\(\s*BRANDING\s+"?([^"\s)]+)', top_level, re.IGNORECASE
    )
    if not branding_match:
        raise AuditError("cannot resolve the CMake BRANDING value")

    cmake = CMAKE_COMMENT_RE.sub(
        "", repo_path("krita/CMakeLists.txt").read_text(encoding="utf-8")
    )
    splash_match = re.search(
        r"\bif\s*\(\s*ANDROID\s*\).*?\belse\s*\(\s*\)\s*"
        r"\bset\s*\(\s*krita_splash_qrc\s+([^\s)]+)",
        cmake,
        re.IGNORECASE | re.DOTALL,
    )
    if not splash_match:
        raise AuditError("cannot resolve the non-Android splash QRC")
    block_match = re.search(
        r'\bset\s*\(\s*krita_QRCS\b(.*?)\bCACHE\s+INTERNAL\s+"krita_QRCS"\s*\)',
        cmake,
        re.IGNORECASE | re.DOTALL,
    )
    if not block_match:
        raise AuditError("cannot find the krita_QRCS CMake definition")
    blocks = [block_match.group(1)]
    blocks.extend(
        re.findall(
            r"\blist\s*\(\s*APPEND\s+krita_QRCS\b(.*?)\)",
            cmake,
            re.IGNORECASE | re.DOTALL,
        )
    )

    paths: list[str] = []
    for block in blocks:
        for token in re.findall(r"\$\{CMAKE_SOURCE_DIR\}/[^\s)]+", block):
            relative = token.removeprefix("${CMAKE_SOURCE_DIR}/")
            relative = relative.replace("${BRANDING}", branding_match.group(1))
            relative = relative.replace("${krita_splash_qrc}", splash_match.group(1))
            if "$" in relative:
                raise AuditError(f"unresolved variable in core QRC path: {token}")
            if Path(relative).suffix != ".qrc":
                raise AuditError(f"non-QRC entry in the core QRC list: {token}")
            paths.append(relative)
    require_unique_paths(paths, "core QRC CMake definition")
    return paths


def static_resources_from_cmake() -> dict[str, list[str]]:
    registrations: dict[str, list[str]] = {}
    qrc_candidates: list[tuple[str, str]] = []
    for directory, child_directories, filenames in os.walk(REPO_ROOT):
        child_directories[:] = [
            name
            for name in child_directories
            if name not in {".git", ".agents", ".codex", "build-ios"}
        ]
        if "CMakeLists.txt" not in filenames:
            continue
        path = Path(directory) / "CMakeLists.txt"
        try:
            text = CMAKE_COMMENT_RE.sub("", path.read_text(encoding="utf-8"))
        except UnicodeDecodeError as exc:
            raise AuditError(f"cannot decode CMake file: {relative_path(path)}") from exc
        for name in STATIC_RESOURCE_REGISTRATION_RE.findall(text):
            registrations.setdefault(name, []).append(relative_path(path))
        for token in LITERAL_QRC_RE.findall(text):
            qrc = (path.parent / token).resolve()
            if qrc.is_file():
                qrc_candidates.append((qrc.stem, relative_path(qrc)))

    duplicates = {name: paths for name, paths in registrations.items() if len(paths) != 1}
    if duplicates:
        raise AuditError(f"static resource registrations are not unique: {duplicates}")
    result = {
        name: sorted({qrc for stem, qrc in qrc_candidates if stem == name})
        for name in sorted(registrations)
    }
    missing = [name for name, paths in result.items() if not paths]
    if missing:
        raise AuditError(f"static resources have no matching QRC: {missing}")
    return result


def qrc_image_paths(manifest: dict[str, Any]) -> set[str]:
    core_qrcs = manifest["core_qrc_paths"]
    if core_qrcs != core_qrc_paths_from_cmake():
        raise AuditError("core QRC inventory differs from krita/CMakeLists.txt")
    static_resources = manifest["static_resources"]
    if static_resources != static_resources_from_cmake():
        raise AuditError("static resource/QRC inventory differs from CMake")
    qrcs = core_qrcs + [
        qrc for paths in static_resources.values() for qrc in paths
    ]
    require_unique_paths(qrcs, "QRC inventory")

    images: set[str] = set()
    for relative_qrc in qrcs:
        qrc_path = repo_path(relative_qrc)
        if not qrc_path.is_file():
            raise AuditError(f"QRC does not exist: {relative_qrc}")
        try:
            root = ET.parse(qrc_path).getroot()
        except ET.ParseError as exc:
            raise AuditError(f"cannot parse QRC {relative_qrc}: {exc}") from exc
        for element in root.iter("file"):
            source = (element.text or "").strip()
            if not source:
                raise AuditError(f"empty <file> entry in {relative_qrc}")
            source_path = (qrc_path.parent / source).resolve()
            try:
                relative_source = source_path.relative_to(REPO_ROOT).as_posix()
            except ValueError as exc:
                raise AuditError(
                    f"QRC image escapes repository: {relative_qrc}: {source}"
                ) from exc
            if source_path.suffix.lower() not in QRC_IMAGE_SUFFIXES:
                continue
            if not source_path.is_file():
                raise AuditError(f"QRC image does not exist: {relative_qrc}: {source}")
            images.add(relative_source)
    return images


def installed_image_paths(manifest: dict[str, Any]) -> set[str]:
    definitions = manifest["installed_image_definition_paths"]
    require_unique_paths(definitions, "installed-image definition inventory")

    images: set[str] = set()
    for relative_cmake in definitions:
        cmake_path = repo_path(relative_cmake)
        if not cmake_path.is_file():
            raise AuditError(f"installed-image definition does not exist: {relative_cmake}")
        text = CMAKE_COMMENT_RE.sub("", cmake_path.read_text(encoding="utf-8"))
        for match in CMAKE_INSTALL_FILES_RE.finditer(text):
            for encoded_token in CMAKE_TOKEN_RE.findall(match.group(1)):
                token = re.sub(r"\\(.)", r"\1", encoded_token)
                if Path(token).suffix.lower() not in QRC_IMAGE_SUFFIXES:
                    continue
                if "$" in token:
                    raise AuditError(
                        f"installed image uses an unresolved CMake variable: "
                        f"{relative_cmake}: {encoded_token}"
                    )
                source_path = (cmake_path.parent / token).resolve()
                try:
                    relative_source = source_path.relative_to(REPO_ROOT).as_posix()
                except ValueError as exc:
                    raise AuditError(
                        f"installed image escapes repository: {relative_cmake}: {encoded_token}"
                    ) from exc
                if not source_path.is_file():
                    raise AuditError(
                        f"installed image does not exist: {relative_cmake}: {encoded_token}"
                    )
                images.add(relative_source)
    return images


def strict_svg_license_urls(relative: str) -> set[str]:
    path = repo_path(relative)
    suffix = path.suffix.lower()
    if suffix not in {".svg", ".svgz"}:
        return set()
    data = path.read_bytes()
    if suffix == ".svgz":
        try:
            data = gzip.decompress(data)
        except (gzip.BadGzipFile, EOFError):
            return set()
    try:
        root = ET.fromstring(data)
    except ET.ParseError:
        return set()

    urls: set[str] = set()
    work_tag = f"{{{CC_NAMESPACE}}}Work"
    license_tag = f"{{{CC_NAMESPACE}}}license"
    resource_attribute = f"{{{RDF_NAMESPACE}}}resource"
    for work in root.iter(work_tag):
        for child in list(work):
            if child.tag == license_tag:
                resource = child.attrib.get(resource_attribute, "").strip()
                if resource:
                    urls.add(resource)
    return urls


def license_group_paths(
    manifest: dict[str, Any],
) -> tuple[set[str], set[str], dict[str, set[str]]]:
    qrc_paths: set[str] = set()
    installed_paths: set[str] = set()
    by_id: dict[str, set[str]] = {}

    for group in manifest["license_groups"]:
        group_id = group.get("id")
        if not isinstance(group_id, str) or not group_id or group_id in by_id:
            raise AuditError(f"invalid or duplicate license group ID: {group_id!r}")
        for key in ("license_expression", "source_url", "notice_path"):
            if not isinstance(group.get(key), str) or not group[key]:
                raise AuditError(f"license group {group_id} has no {key}")
        notice_path = repo_path(group["notice_path"])
        if not notice_path.is_file():
            raise AuditError(f"license group {group_id} notice does not exist")

        paths = group.get("paths", [])
        if paths != sorted(set(paths)):
            raise AuditError(f"license group {group_id} paths are not sorted and unique")
        path_set = set(paths)
        for relative in paths:
            if not repo_path(relative).is_file():
                raise AuditError(f"license group {group_id} image does not exist: {relative}")
        if (qrc_paths | installed_paths) & path_set:
            raise AuditError(f"license group {group_id} overlaps another group")

        if group["scope"] == "qrc":
            qrc_paths.update(path_set)
        elif group["scope"] == "installed-only":
            installed_paths.update(path_set)
        else:
            raise AuditError(f"license group {group_id} has unsupported scope")
        by_id[group_id] = path_set
    return qrc_paths, installed_paths, by_id


def audit(manifest: dict[str, Any]) -> int:
    qrc_images = qrc_image_paths(manifest)
    retained_qrc, installed_only, groups = license_group_paths(manifest)
    discovered_installed = installed_image_paths(manifest)

    if not retained_qrc <= qrc_images:
        raise AuditError(
            f"QRC license groups contain unlinked images: {sorted(retained_qrc - qrc_images)}"
        )
    if qrc_images & installed_only:
        raise AuditError("installed-only images overlap QRC images")
    if installed_only != discovered_installed:
        raise AuditError(
            "installed-image classification differs from CMake: "
            f"missing={sorted(discovered_installed - installed_only)}, "
            f"extra={sorted(installed_only - discovered_installed)}"
        )

    licensed_urls = {
        relative: urls
        for relative in qrc_images
        if (urls := strict_svg_license_urls(relative))
    }
    observed_urls = set().union(*licensed_urls.values()) if licensed_urls else set()
    allowed_urls = set(manifest["allowed_metadata_license_urls"])
    if observed_urls != allowed_urls:
        raise AuditError(
            f"SVG metadata license URLs changed: {sorted(observed_urls)} != {sorted(allowed_urls)}"
        )

    linked_art = set(licensed_urls) - retained_qrc
    unclassified = qrc_images - retained_qrc - linked_art
    if unclassified:
        raise AuditError(f"unclassified iOS QRC images: {sorted(unclassified)}")

    print(
        "iOS image license audit: "
        f"QRC={len(qrc_images)} "
        f"(metadata-classified={len(linked_art)}, grouped={len(retained_qrc)}); "
        f"installed={len(installed_only)}; groups={len(groups)}"
    )
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="run the audit (the default action; accepted for explicit build commands)",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=DEFAULT_MANIFEST,
        help="manifest path (default: %(default)s)",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    manifest_path = args.manifest.resolve()
    try:
        manifest_path.relative_to(REPO_ROOT)
        return audit(load_manifest(manifest_path))
    except (AuditError, OSError, UnicodeError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
