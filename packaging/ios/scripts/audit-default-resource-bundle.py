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
    """The bundle no longer satisfies its audited license inventory."""


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


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


def text_param(root: ET.Element, name: str) -> str:
    for element in root.findall("param"):
        if element.get("name") == name:
            return "".join(element.itertext()).strip()
    return ""


def classify(path: str, manifest: dict[str, Any]) -> tuple[str, str, str]:
    suffix = PurePosixPath(path).suffix.lower()
    if path.startswith("brushes/") and suffix in {".gbr", ".gih"}:
        return (
            "CC-BY-3.0",
            "krita/data/README",
            "krita/data/README plus meta.xml creator fields",
        )
    if path == "preview.png":
        return (
            "CC0-1.0",
            "LibrePaint white replacement",
            "LibrePaint contributors",
        )
    attribution = "meta.xml creator fields"
    if path in manifest["embedded_attribution"]:
        attribution += f"; embedded PNG Author={manifest['embedded_attribution'][path]}"
    return "CC0-1.0", "meta.xml bundle license", attribution


def check_external_notice(manifest: dict[str, Any]) -> None:
    for record in manifest["external_notices"]:
        path = REPO_ROOT / record["path"]
        if not path.is_file():
            raise AuditError(f"external notice is missing: {record['path']}")
        actual = sha256(path.read_bytes())
        if actual != record["sha256"]:
            raise AuditError(
                f"external notice changed: {record['path']} ({actual}, expected {record['sha256']})"
            )


def check_manifest_xml(
    xml: bytes, resource_files: dict[str, bytes], expected: dict[str, int]
) -> None:
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
    record_count = sum(len(elements) for elements in records.values())
    duplicate_path_count = sum(len(elements) > 1 for elements in records.values())
    actual_counts = {
        "record_count": record_count,
        "unique_path_count": len(records),
        "duplicate_path_count": duplicate_path_count,
    }
    if actual_counts != expected:
        raise AuditError(
            f"resource manifest counts changed: {actual_counts} (expected {expected})"
        )
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
    if manifest.get("schema") != 1:
        raise AuditError(f"unsupported audit manifest schema: {manifest.get('schema')}")
    if check_external_notices:
        check_external_notice(manifest)

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

        directories = [info.filename for info in infos if info.is_dir()]
        if directories != manifest["expected_directory_entries"]:
            raise AuditError(
                f"directory entries changed: {directories} (expected {manifest['expected_directory_entries']})"
            )
        files = [info for info in infos if not info.is_dir()]
        if len(files) != manifest["expected_file_count"]:
            raise AuditError(
                f"bundle file count is {len(files)}; expected {manifest['expected_file_count']}"
            )

        required_admin = {"META-INF/manifest.xml", "meta.xml", "mimetype", "preview.png"}
        if not required_admin.issubset(names):
            raise AuditError(f"bundle administrative entries are missing: {sorted(required_admin - set(names))}")
        if archive.read("mimetype") != b"application/x-krita-resourcebundle":
            raise AuditError("bundle mimetype changed")

        meta_xml = archive.read("meta.xml")
        metadata = manifest["bundle_metadata"]
        if sha256(meta_xml) != metadata["sha256"]:
            raise AuditError("meta.xml changed")
        if metadata_value(meta_xml, "license") != metadata["license"]:
            raise AuditError("bundle license metadata changed")
        for element, expected in metadata["elements"].items():
            if element_text(meta_xml, element) != expected:
                raise AuditError(f"bundle metadata changed: {element}")

        contents = {info.filename: archive.read(info) for info in files}
        resource_files = {
            path: data
            for path, data in contents.items()
            if path.startswith(("brushes/", "paintoppresets/", "patterns/"))
        }
        check_manifest_xml(
            contents["META-INF/manifest.xml"],
            resource_files,
            manifest["resource_manifest_counts"],
        )

        inventory: list[dict[str, Any]] = []
        license_counts: Counter[str] = Counter()
        kind_counts: Counter[str] = Counter()
        suffix_counts: Counter[str] = Counter()
        embedded_attribution: dict[str, str] = {}
        embedded_legal_metadata: dict[str, dict[str, str]] = {}
        required_presets = manifest["required_self_contained_presets"]
        seen_required: set[str] = set()
        preset_count = 0

        for info in files:
            path = info.filename
            data = contents[path]
            license_id, basis, attribution = classify(path, manifest)
            license_counts[license_id] += 1
            top_level = path.split("/", 1)[0]
            kind_counts[top_level] += 1
            suffix_counts[PurePosixPath(path).suffix.lower() or "<none>"] += 1

            content_hash = "WHITE_PREVIEW" if path == "preview.png" else sha256(data)
            content_size = 0 if path == "preview.png" else info.file_size
            inventory.append(
                {
                    "path": path,
                    "size": content_size,
                    "sha256": content_hash,
                    "license": license_id,
                    "basis": basis,
                    "attribution": attribution,
                }
            )

            suffix = PurePosixPath(path).suffix.lower()
            if path.startswith("paintoppresets/") and suffix == ".kpp":
                preset_count += 1
                xml = preset_xml(data)
                try:
                    root = ET.fromstring(xml)
                except ET.ParseError as exc:
                    raise AuditError(f"invalid KPP preset XML in {path}: {exc}") from exc
                if root.tag != "Preset" or not root.get("name") or not root.get("paintopid"):
                    raise AuditError(f"incomplete KPP preset metadata: {path}")
                if LEGAL_METADATA_PATTERN.search(xml.decode("utf-8", "replace")):
                    raise AuditError(f"unexpected per-preset legal override: {path}")
                filename = PurePosixPath(path).name
                if filename in required_presets:
                    expected = required_presets[filename]
                    if root.get("name") != expected["name"]:
                        raise AuditError(f"name changed for required preset: {path}")
                    if root.get("paintopid") != expected["paintopid"]:
                        raise AuditError(f"paintop changed for required preset: {path}")
                    brush_definition = text_param(root, "brush_definition")
                    if expected["auto_brush"] and 'type="auto_brush"' not in brush_definition:
                        raise AuditError(f"required preset is no longer an auto brush: {path}")
                    required_files = text_param(root, "requiredBrushFile") + text_param(
                        root, "requiredBrushFilesList"
                    )
                    if expected["self_contained"] and required_files:
                        raise AuditError(f"required preset gained an external brush dependency: {path}")
                    seen_required.add(filename)
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

        if preset_count != manifest["expected_preset_count"]:
            raise AuditError(
                f"KPP preset count is {preset_count}; expected {manifest['expected_preset_count']}"
            )
        if seen_required != set(required_presets):
            raise AuditError(
                f"required self-contained presets changed; found={sorted(seen_required)}"
            )
        if dict(sorted(embedded_attribution.items())) != manifest["embedded_attribution"]:
            raise AuditError(
                "embedded PNG attribution changed: "
                f"{dict(sorted(embedded_attribution.items()))}"
            )
        if (
            dict(sorted(embedded_legal_metadata.items()))
            != manifest["individual_legal_overrides"]
        ):
            raise AuditError(
                "individual PNG legal metadata changed: "
                f"{dict(sorted(embedded_legal_metadata.items()))}"
            )
        if dict(license_counts) != manifest["expected_license_counts"]:
            raise AuditError(
                f"license counts changed: {dict(license_counts)} (expected {manifest['expected_license_counts']})"
            )
        if dict(kind_counts) != manifest["expected_top_level_counts"]:
            raise AuditError(
                f"top-level counts changed: {dict(kind_counts)} (expected {manifest['expected_top_level_counts']})"
            )
        if dict(suffix_counts) != manifest["expected_suffix_counts"]:
            raise AuditError(
                f"file-format counts changed: {dict(suffix_counts)} "
                f"(expected {manifest['expected_suffix_counts']})"
            )

        payload = json.dumps(
            inventory, sort_keys=True, separators=(",", ":"), ensure_ascii=True
        ).encode("utf-8")
        actual_inventory_hash = sha256(payload)
        if actual_inventory_hash != manifest["classified_inventory_sha256"]:
            raise AuditError(
                "classified bundle inventory changed: "
                f"{actual_inventory_hash} (expected {manifest['classified_inventory_sha256']})"
            )

    return license_counts, inventory


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle", type=Path, default=DEFAULT_BUNDLE)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument(
        "--skip-external-notice-check",
        action="store_true",
        help="skip repository-path notice hashes when auditing a packaged copy",
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
        f"281 files classified (CC0-1.0={counts['CC0-1.0']}, "
        f"CC-BY-3.0={counts['CC-BY-3.0']}); unclassified=0"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
