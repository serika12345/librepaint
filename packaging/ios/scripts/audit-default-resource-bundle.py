#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Verify the complete iOS default-resource-bundle license inventory."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
import zlib
import zipfile
import xml.etree.ElementTree as ET
from collections import Counter
from fnmatch import fnmatchcase
from pathlib import Path, PurePosixPath
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_BUNDLE = REPO_ROOT / "krita/data/bundles/Krita_4_Default_Resources.bundle"
DEFAULT_MANIFEST = (
    REPO_ROOT / "packaging/ios/manifests/default-resource-bundle-licenses.json"
)
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MANIFEST_NAMESPACE = "urn:oasis:names:tc:opendocument:xmlns:manifest:1.0"
LEGAL_METADATA_PATTERN = re.compile(
    r"(?:copyright|copyleft|licen[cs]e|spdx|creative\s+commons|\bcc[- ]?by|\bgpl\b)",
    re.IGNORECASE,
)


class AuditError(RuntimeError):
    """The bundle no longer satisfies its license classification."""


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise AuditError(f"cannot read audit manifest {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise AuditError("audit manifest root must be an object")
    return value


def metadata_value(xml: bytes, name: str) -> str:
    pattern = re.compile(
        rb"<meta:meta-userdefined\s+meta:name=\""
        + re.escape(name.encode("utf-8"))
        + rb"\"\s+meta:value=\"([^\"]*)\"\s*/>"
    )
    match = pattern.search(xml)
    if not match:
        raise AuditError(f"meta.xml is missing user-defined field: {name}")
    return match.group(1).decode("utf-8")


def element_text(xml: bytes, element: str) -> str:
    pattern = re.compile(
        rb"<" + re.escape(element.encode("utf-8")) + rb">(.*?)</"
        + re.escape(element.encode("utf-8")) + rb">",
        re.DOTALL,
    )
    match = pattern.search(xml)
    if not match:
        raise AuditError(f"meta.xml is missing element: {element}")
    return match.group(1).decode("utf-8")


def png_text_fields(data: bytes) -> dict[str, str]:
    if not data.startswith(PNG_SIGNATURE):
        raise AuditError("expected PNG data")
    fields: dict[str, str] = {}
    offset = len(PNG_SIGNATURE)
    while offset + 12 <= len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        chunk_type = data[offset + 4 : offset + 8]
        payload = data[offset + 8 : offset + 8 + length]
        offset += 12 + length
        try:
            if chunk_type == b"tEXt":
                keyword, value = payload.split(b"\0", 1)
                fields[keyword.decode("latin-1").lower()] = value.decode("latin-1")
            elif chunk_type == b"zTXt":
                keyword, compressed = payload.split(b"\0", 1)
                if not compressed or compressed[0] != 0:
                    raise AuditError("unsupported PNG zTXt compression method")
                fields[keyword.decode("latin-1").lower()] = zlib.decompress(
                    compressed[1:]
                ).decode("utf-8", "replace")
            elif chunk_type == b"iTXt":
                keyword, remainder = payload.split(b"\0", 1)
                if len(remainder) < 2:
                    raise AuditError("short PNG iTXt chunk")
                compressed, compression_method = remainder[0], remainder[1]
                remainder = remainder[2:]
                _language, remainder = remainder.split(b"\0", 1)
                _translated_keyword, value = remainder.split(b"\0", 1)
                if compressed:
                    if compression_method != 0:
                        raise AuditError("unsupported PNG iTXt compression method")
                    value = zlib.decompress(value)
                fields[keyword.decode("latin-1").lower()] = value.decode(
                    "utf-8", "replace"
                )
        except (ValueError, UnicodeDecodeError, zlib.error) as exc:
            raise AuditError(f"invalid PNG text chunk: {exc}") from exc
    return fields


def preset_xml(data: bytes) -> bytes:
    if not data.startswith(PNG_SIGNATURE):
        raise AuditError("KPP is not a PNG container")
    offset = len(PNG_SIGNATURE)
    matches: list[bytes] = []
    while offset + 12 <= len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        chunk_type = data[offset + 4 : offset + 8]
        payload = data[offset + 8 : offset + 8 + length]
        offset += 12 + length
        if chunk_type == b"tEXt" and payload.startswith(b"preset\0"):
            matches.append(payload.split(b"\0", 1)[1])
        elif chunk_type == b"zTXt" and payload.startswith(b"preset\0"):
            compressed = payload.split(b"\0", 1)[1]
            if not compressed or compressed[0] != 0:
                raise AuditError("unsupported KPP zTXt compression method")
            matches.append(zlib.decompress(compressed[1:]))
    if len(matches) != 1:
        raise AuditError(f"KPP must contain exactly one preset chunk; found {len(matches)}")
    return matches[0]


def classify(path: str, manifest: dict[str, Any]) -> tuple[str, str, str, str]:
    matches: list[dict[str, Any]] = []
    defaults: list[dict[str, Any]] = []
    for group in manifest["license_groups"]:
        if group.get("default"):
            defaults.append(group)
            continue
        if path in group.get("paths", []) or any(
            fnmatchcase(path, pattern) for pattern in group.get("patterns", [])
        ):
            matches.append(group)
    if len(matches) > 1:
        raise AuditError(f"overlapping license groups for {path}")
    if not matches:
        if len(defaults) != 1:
            raise AuditError("license inventory must contain one default group")
        matches = defaults
    group = matches[0]
    attribution = group["attribution"]
    if path in manifest["embedded_attribution"]:
        attribution += f"; embedded PNG Author={manifest['embedded_attribution'][path]}"
    return (
        group["id"],
        group["license_expression"],
        group["basis"],
        attribution,
    )


def verify_external_notices(manifest: dict[str, Any]) -> None:
    notice_paths = {
        path
        for group in manifest["license_groups"]
        for path in group["notice_paths"]
    }
    for relative in sorted(notice_paths):
        path = REPO_ROOT / relative
        if not path.is_file():
            raise AuditError(f"external notice is missing: {relative}")


def check_manifest_xml(xml: bytes, resource_files: dict[str, bytes]) -> None:
    try:
        root = ET.fromstring(xml)
    except ET.ParseError as exc:
        raise AuditError(f"invalid META-INF/manifest.xml: {exc}") from exc
    path_key = f"{{{MANIFEST_NAMESPACE}}}full-path"
    type_key = f"{{{MANIFEST_NAMESPACE}}}media-type"
    md5_key = f"{{{MANIFEST_NAMESPACE}}}md5sum"
    records: dict[str, list[ET.Element]] = {}
    for element in root:
        path = element.get(path_key)
        if path and path != "/":
            records.setdefault(path, []).append(element)
    if set(records) != set(resource_files):
        missing = sorted(set(resource_files) - set(records))
        extra = sorted(set(records) - set(resource_files))
        raise AuditError(f"resource manifest path mismatch; missing={missing}, extra={extra}")
    for path, data in resource_files.items():
        expected_type = path.split("/", 1)[0]
        actual_md5 = hashlib.md5(data, usedforsecurity=False).hexdigest()
        for element in records[path]:
            if element.get(type_key) != expected_type:
                raise AuditError(f"wrong resource type for {path}: {element.get(type_key)}")
            if element.get(md5_key, "").lower() != actual_md5:
                raise AuditError(f"resource manifest checksum mismatch: {path}")


def audit(
    bundle_path: Path,
    manifest_path: Path,
    *,
    check_external_notices: bool = True,
) -> tuple[Counter[str], list[dict[str, Any]]]:
    manifest = load_json(manifest_path)
    if manifest.get("schema") != 2:
        raise AuditError(f"unsupported audit manifest schema: {manifest.get('schema')}")
    groups = manifest.get("license_groups", [])
    group_ids = [group.get("id") for group in groups]
    if (
        not groups
        or any(not isinstance(group_id, str) or not group_id for group_id in group_ids)
        or len(group_ids) != len(set(group_ids))
    ):
        raise AuditError("license group IDs must be present and unique")
    for group in groups:
        for key in ("license_expression", "basis", "attribution", "notice_paths"):
            if not group.get(key):
                raise AuditError(f"license group {group['id']} has no {key}")
    if sum(bool(group.get("default")) for group in groups) != 1:
        raise AuditError("license inventory must contain one default group")
    if check_external_notices:
        verify_external_notices(manifest)

    try:
        archive = zipfile.ZipFile(bundle_path)
    except (OSError, zipfile.BadZipFile) as exc:
        raise AuditError(f"cannot open resource bundle {bundle_path}: {exc}") from exc

    with archive:
        infos = archive.infolist()
        names = [info.filename for info in infos]
        if len(names) != len(set(names)):
            raise AuditError("bundle contains duplicate ZIP entry names")
        for name in names:
            path = PurePosixPath(name)
            if path.is_absolute() or ".." in path.parts:
                raise AuditError(f"unsafe ZIP entry path: {name}")

        files = [info for info in infos if not info.is_dir()]

        required_admin = {"META-INF/manifest.xml", "meta.xml", "mimetype", "preview.png"}
        if not required_admin.issubset(names):
            raise AuditError(f"bundle administrative entries are missing: {sorted(required_admin - set(names))}")
        if archive.read("mimetype") != b"application/x-krita-resourcebundle":
            raise AuditError("bundle mimetype changed")

        meta_xml = archive.read("meta.xml")
        metadata = manifest["bundle_metadata"]
        if metadata_value(meta_xml, "license") != metadata["license"]:
            raise AuditError("bundle license metadata changed")
        for element, expected in metadata["attribution_elements"].items():
            if element_text(meta_xml, element) != expected:
                raise AuditError(f"bundle metadata changed: {element}")

        contents = {info.filename: archive.read(info) for info in files}
        resource_files = {
            path: data
            for path, data in contents.items()
            if path.startswith(("brushes/", "paintoppresets/", "patterns/"))
        }
        check_manifest_xml(contents["META-INF/manifest.xml"], resource_files)

        inventory: list[dict[str, Any]] = []
        license_counts: Counter[str] = Counter()
        embedded_attribution: dict[str, str] = {}
        embedded_legal_metadata: dict[str, dict[str, str]] = {}

        for info in files:
            path = info.filename
            data = contents[path]
            group_id, license_id, basis, attribution = classify(path, manifest)
            license_counts[license_id] += 1
            inventory.append(
                {
                    "path": path,
                    "group": group_id,
                    "license": license_id,
                    "basis": basis,
                    "attribution": attribution,
                }
            )

            suffix = PurePosixPath(path).suffix.lower()
            if path.startswith("paintoppresets/") and suffix == ".kpp":
                xml = preset_xml(data)
                try:
                    ET.fromstring(xml)
                except ET.ParseError as exc:
                    raise AuditError(f"invalid KPP preset XML in {path}: {exc}") from exc
                if LEGAL_METADATA_PATTERN.search(xml.decode("utf-8", "replace")):
                    raise AuditError(f"unexpected per-preset legal override: {path}")
            elif path.startswith("brushes/") and suffix == ".svg":
                text = data.decode("utf-8", "replace")
                if LEGAL_METADATA_PATTERN.search(text):
                    raise AuditError(f"unexpected per-file SVG legal override: {path}")
            elif path.startswith(("brushes/", "patterns/")) and suffix == ".png":
                fields = png_text_fields(data)
                author = fields.get("author")
                if author:
                    embedded_attribution[path] = author
                legal_fields = {
                    key: value
                    for key, value in fields.items()
                    if LEGAL_METADATA_PATTERN.search(key)
                    or LEGAL_METADATA_PATTERN.search(value)
                }
                if legal_fields:
                    embedded_legal_metadata[path] = legal_fields

        if dict(sorted(embedded_attribution.items())) != manifest["embedded_attribution"]:
            raise AuditError(
                "embedded PNG attribution changed: "
                f"{dict(sorted(embedded_attribution.items()))}"
            )
        if embedded_legal_metadata:
            raise AuditError(
                "individual PNG legal metadata found: "
                f"{dict(sorted(embedded_legal_metadata.items()))}"
            )
        if dict(license_counts) != manifest["expected_license_counts"]:
            raise AuditError(
                "license counts changed: "
                f"{dict(license_counts)} (expected {manifest['expected_license_counts']})"
            )

    return license_counts, inventory


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle", type=Path, default=DEFAULT_BUNDLE)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument(
        "--skip-external-notice-check",
        action="store_true",
        help="skip repository-path notice checks when auditing a packaged copy",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="print the path, license, and classification basis for every file",
    )
    args = parser.parse_args()
    try:
        counts, inventory = audit(
            args.bundle.resolve(),
            args.manifest.resolve(),
            check_external_notices=not args.skip_external_notice_check,
        )
    except AuditError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    if args.list:
        for record in inventory:
            print(
                f"{record['path']}\t{record['license']}\t{record['basis']}\t"
                f"{record['attribution']}"
            )
    print(
        "default resource bundle audit: "
        f"{sum(counts.values())} files classified (CC0-1.0={counts['CC0-1.0']}, "
        f"CC-BY-3.0={counts['CC-BY-3.0']}); unclassified=0"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
