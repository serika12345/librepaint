#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Audit retained iOS artwork and replace selected branding with opaque white.

The manifest records the dimensions and preservation invariants captured from
the source assets. SVG and embedded raster legal/author metadata is retained
byte-for-byte. ICO and ICNS containers keep every representation and its PNG
metadata, and bundle rewriting changes only the root preview.png compressed
payload plus the ZIP bookkeeping that must change with it.

Paths reclassified as LibrePaint-owned CC0 branding are excluded from white
replacement and content-hash checked through their dedicated retained group.
"""

from __future__ import annotations

import argparse
import binascii
import gzip
import hashlib
import io
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
import zipfile
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any, Iterable


REPO_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_MANIFEST = REPO_ROOT / "packaging/ios/manifests/white-brand-assets.json"
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
ZIP_LOCAL_SIGNATURE = b"PK\x03\x04"
ZIP_CENTRAL_SIGNATURE = b"PK\x01\x02"
ZIP_EOCD_SIGNATURE = b"PK\x05\x06"
ZIP_DESCRIPTOR_SIGNATURE = b"PK\x07\x08"

ICNS_DIMENSIONS = {
    "ic04": 16,
    "ic05": 32,
    "ic11": 32,
    "ic12": 64,
    "ic07": 128,
    "ic08": 256,
    "ic13": 256,
    "ic09": 512,
    "ic14": 512,
    "ic10": 1024,
}

ANDROID_VECTOR_TEMPLATE = """<vector xmlns:android=\"http://schemas.android.com/apk/res/android\"
    android:width=\"108dp\"
    android:height=\"108dp\"
    android:viewportWidth=\"1309\"
    android:viewportHeight=\"1309\">
  <path
      android:fillColor=\"#FFFFFFFF\"
      android:pathData=\"M0,0h1309v1309H0z\" />
</vector>
"""
XML_COMMENT_RE = re.compile(r"<!--.*?-->", re.DOTALL)

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
PNG_PRESERVED_METADATA_TYPES = {b"eXIf", b"iCCP", b"iTXt", b"tEXt", b"zTXt"}
WEBP_PRESERVED_METADATA_TYPES = {b"EXIF", b"ICCP", b"XMP "}
JPEG_PRESERVED_METADATA_MARKERS = {0xE1, 0xE2, 0xEB, 0xED, 0xFE}
LEGAL_METADATA_RE = re.compile(br"copyright|creative\s*commons|creativecommons|attribution-sharealike", re.IGNORECASE)

RETAINED_NOTICE_PATH = "packaging/ios/notices/retained-functional-assets.md"
LIBREPAINT_BRAND_NOTICE_PATH = "packaging/ios/notices/librepaint-brand-assets.md"
RETAINED_LICENSE_GROUP_CONTRACT = {
    "krita-cc-by-sa-4-functional-qrc": {
        "scope": "qrc",
        "license_expression": "CC-BY-SA-4.0",
        "source_url": "https://creativecommons.org/licenses/by-sa/4.0/",
        "notice_path": RETAINED_NOTICE_PATH,
    },
    "kde-oxygen-icons-qrc": {
        "scope": "qrc",
        "license_expression": "LGPL-3.0-or-later",
        "source_url": "https://invent.kde.org/frameworks/oxygen-icons/-/raw/246760ed7e382d1ecc92cbfc194acee888d2d308/COPYING",
        "notice_path": RETAINED_NOTICE_PATH,
    },
    "kde-breeze-icons-qrc": {
        "scope": "qrc",
        "license_expression": "LGPL-3.0-or-later",
        "source_url": "https://invent.kde.org/frameworks/breeze-icons/-/raw/a0ce9f0faea836e45d765bfb48af2ada6f9bfc70/COPYING-ICONS",
        "notice_path": RETAINED_NOTICE_PATH,
    },
    "android-robot-functional-qrc": {
        "scope": "qrc",
        "license_expression": "CC-BY-SA-4.0 AND CC-BY-3.0",
        "source_url": "https://developer.android.com/distribute/marketing-tools/brand-guidelines",
        "notice_path": RETAINED_NOTICE_PATH,
    },
    "kde-kxmlgui-functional-qrc": {
        "scope": "qrc",
        "license_expression": "LGPL-2.0-or-later",
        "source_url": "https://invent.kde.org/frameworks/kxmlgui",
        "notice_path": RETAINED_NOTICE_PATH,
    },
    "krita-functional-qrc": {
        "scope": "qrc",
        "license_expression": "GPL-3.0-only",
        "source_url": "https://krita.org/en/about/license/",
        "notice_path": RETAINED_NOTICE_PATH,
    },
    "krita-functional-installed": {
        "scope": "installed-only",
        "license_expression": "GPL-3.0-only",
        "source_url": "https://krita.org/en/about/license/",
        "notice_path": RETAINED_NOTICE_PATH,
    },
    "librepaint-branding-qrc": {
        "scope": "qrc",
        "license_expression": "CC0-1.0",
        "source_url": "https://creativecommons.org/publicdomain/zero/1.0/",
        "notice_path": LIBREPAINT_BRAND_NOTICE_PATH,
    },
}

LIBREPAINT_BRANDING_QRC_PATHS = {
    "krita/data/splash/electrichearts_20250824A_kiki_4K.png",
    "krita/pics/branding/Next/sc-apps-krita.svgz",
}

MIGRATED_BREEZE_LOGO_PATHS = {
    "krita/pics/Breeze-dark/dark_kde.svg",
    "krita/pics/Breeze-light/light_kde.svg",
}
MIGRATED_ADOBE_PDF_ICON_PATHS = {
    "krita/pics/Breeze-dark/dark_application-pdf.svg",
    "krita/pics/Breeze-light/light_application-pdf.svg",
}
MIGRATED_ABOUT_KDE_PATH = "libs/widgetutils/xmlgui/aboutkde.png"
MIGRATED_KXMLGUI_THUMB_PATH = "libs/widgetutils/xmlgui/thumb_frame.png"
MIGRATED_ANDROID_ROBOT_PATHS = {
    "krita/pics/svg/dark_show_android_log.svg",
    "krita/pics/svg/light_show_android_log.svg",
}
MIGRATED_PROJECT_WHITE_PATHS = {
    "krita/pics/svg/dark_krita_log.svg",
    "krita/pics/svg/light_krita_log.svg",
    "krita/pics/tools/SVG/16/dark_tool_comic_panel_move_point.svg",
    "krita/pics/tools/SVG/16/light_tool_comic_panel_move_point.svg",
}
MIGRATED_COMMIT_CC_PATHS = {
    "krita/pics/tools/SVG/16/dark_krita_tool_assistant.svg",
    "krita/pics/tools/SVG/16/dark_shape_handling.svg",
    "krita/pics/tools/SVG/16/light_krita_tool_assistant.svg",
    "krita/pics/tools/SVG/16/light_shape_handling.svg",
}
MIGRATED_OXYGEN_PATHS = {
    "libs/flake/pics/16-actions-snap-boundingbox.png",
    "libs/flake/pics/16-actions-snap-extension.png",
    "libs/flake/pics/22-actions-convert-to-path.png",
    "libs/flake/pics/22-actions-createpath.png",
    "libs/flake/pics/22-actions-editpath.png",
    "libs/flake/pics/22-actions-hand.png",
    "libs/flake/pics/22-actions-pathshape.png",
    "libs/flake/pics/sc-actions-snap-boundingbox.svg",
    "libs/flake/pics/sc-actions-snap-extension.svg",
    "plugins/flake/pathshapes/pics/22-actions-callout-shape.png",
    "plugins/flake/pathshapes/pics/22-actions-circular-arrow-shape.png",
    "plugins/flake/pathshapes/pics/22-actions-cross-shape.png",
    "plugins/flake/pathshapes/pics/22-actions-ellipse-shape.png",
    "plugins/flake/pathshapes/pics/22-actions-flower-shape.png",
    "plugins/flake/pathshapes/pics/22-actions-gearhead-shape.png",
    "plugins/flake/pathshapes/pics/22-actions-hexagon-shape.png",
    "plugins/flake/pathshapes/pics/22-actions-pentagon-shape.png",
    "plugins/flake/pathshapes/pics/22-actions-polygon-shape.png",
    "plugins/flake/pathshapes/pics/22-actions-rectangle-shape.png",
    "plugins/flake/pathshapes/pics/22-actions-smiley-shape.png",
    "plugins/flake/pathshapes/pics/22-actions-spiral-shape.png",
    "plugins/flake/pathshapes/pics/22-actions-star-shape.png",
}
MIGRATED_ROOT_PICS_GPL_PATHS = {
    "pics/16_dark_format-text-direction-horizontal-tb.svg",
    "pics/16_dark_format-text-direction-ltr.svg",
    "pics/16_dark_format-text-direction-vertical-lr.svg",
    "pics/16_dark_format-text-direction-vertical-rl.svg",
    "pics/16_dark_warning.svg",
    "pics/16_light_format-text-direction-horizontal-tb.svg",
    "pics/16_light_format-text-direction-ltr.svg",
    "pics/16_light_format-text-direction-vertical-lr.svg",
    "pics/16_light_format-text-direction-vertical-rl.svg",
    "pics/16_light_warning.svg",
    "pics/22_dark_format-text-direction-horizontal-tb.svg",
    "pics/22_dark_format-text-direction-ltr.svg",
    "pics/22_dark_format-text-direction-vertical-lr.svg",
    "pics/22_dark_format-text-direction-vertical-rl.svg",
    "pics/22_light_format-text-direction-horizontal-tb.svg",
    "pics/22_light_format-text-direction-ltr.svg",
    "pics/22_light_format-text-direction-vertical-lr.svg",
    "pics/22_light_format-text-direction-vertical-rl.svg",
}

INSTALLED_IMAGE_DEFINITION_PATHS = (
    "libs/flake/styles/CMakeLists.txt",
    "plugins/assistants/Assistants/CMakeLists.txt",
    "plugins/dockers/advancedcolorselector/CMakeLists.txt",
    "plugins/paintops/colorsmudge/CMakeLists.txt",
    "plugins/paintops/curvebrush/CMakeLists.txt",
    "plugins/paintops/defaultpaintops/CMakeLists.txt",
    "plugins/paintops/deform/CMakeLists.txt",
    "plugins/paintops/experiment/CMakeLists.txt",
    "plugins/paintops/filterop/CMakeLists.txt",
    "plugins/paintops/gridbrush/CMakeLists.txt",
    "plugins/paintops/hairy/CMakeLists.txt",
    "plugins/paintops/hatching/CMakeLists.txt",
    "plugins/paintops/mypaint/CMakeLists.txt",
    "plugins/paintops/mypaint/brushes/CMakeLists.txt",
    "plugins/paintops/particle/CMakeLists.txt",
    "plugins/paintops/roundmarker/CMakeLists.txt",
    "plugins/paintops/sketch/CMakeLists.txt",
    "plugins/paintops/spray/CMakeLists.txt",
    "plugins/paintops/tangentnormal/CMakeLists.txt",
)
CMAKE_INSTALL_FILES_RE = re.compile(
    r"\binstall\s*\(\s*FILES\b(.*?)\bDESTINATION\b",
    re.IGNORECASE | re.DOTALL,
)
CMAKE_COMMENT_RE = re.compile(r"#[^\r\n]*")
CMAKE_TOKEN_RE = re.compile(r"(?:\\.|[^\s])+")


class AssetError(RuntimeError):
    """An asset does not satisfy the manifest or white-art policy."""


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def repo_path(relative: str) -> Path:
    path = (REPO_ROOT / relative).resolve()
    try:
        path.relative_to(REPO_ROOT)
    except ValueError as exc:
        raise AssetError(f"path escapes repository: {relative}") from exc
    return path


def relative_path(path: Path) -> str:
    return path.resolve().relative_to(REPO_ROOT).as_posix()


def atomic_write(path: Path, data: bytes) -> bool:
    if path.read_bytes() == data:
        return False
    mode = path.stat().st_mode
    fd, temporary_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    temporary = Path(temporary_name)
    try:
        with os.fdopen(fd, "wb") as handle:
            handle.write(data)
            handle.flush()
            os.fsync(handle.fileno())
        os.chmod(temporary, mode)
        os.replace(temporary, path)
    finally:
        if temporary.exists():
            temporary.unlink()
    return True


def png_chunk(chunk_type: bytes, data: bytes) -> bytes:
    checksum = binascii.crc32(chunk_type + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", checksum)


def parse_png_chunks(data: bytes) -> list[tuple[bytes, bytes, bytes]]:
    if not data.startswith(PNG_SIGNATURE):
        raise AssetError("invalid PNG signature")
    chunks: list[tuple[bytes, bytes, bytes]] = []
    offset = len(PNG_SIGNATURE)
    saw_iend = False
    while offset + 12 <= len(data):
        size = struct.unpack_from(">I", data, offset)[0]
        end = offset + 12 + size
        if end > len(data):
            raise AssetError("short PNG chunk")
        chunk_type = data[offset + 4 : offset + 8]
        payload = data[offset + 8 : offset + 8 + size]
        raw = data[offset:end]
        expected_crc = struct.unpack_from(">I", raw, len(raw) - 4)[0]
        actual_crc = binascii.crc32(chunk_type + payload) & 0xFFFFFFFF
        if expected_crc != actual_crc:
            raise AssetError(f"invalid PNG {chunk_type.decode('latin-1')} CRC")
        chunks.append((chunk_type, payload, raw))
        offset = end
        if chunk_type == b"IEND":
            saw_iend = True
            break
    if not saw_iend or offset != len(data):
        raise AssetError("PNG has no final IEND chunk")
    return chunks


def png_metadata_chunks(data: bytes) -> list[tuple[str, bytes]]:
    return [
        (chunk_type.decode("ascii"), raw)
        for chunk_type, _payload, raw in parse_png_chunks(data)
        if chunk_type in PNG_PRESERVED_METADATA_TYPES
    ]


def white_png(
    width: int,
    height: int,
    *,
    alpha: bool = False,
    metadata_chunks: Iterable[bytes] = (),
) -> bytes:
    if width <= 0 or height <= 0:
        raise AssetError(f"invalid PNG dimensions: {width}x{height}")
    pixel = b"\xff\xff\xff\xff" if alpha else b"\xff\xff\xff"
    raw_row = b"\x00" + pixel * width
    color_type = 6 if alpha else 2
    header = struct.pack(">IIBBBBB", width, height, 8, color_type, 0, 0, 0)
    metadata_chunks = tuple(metadata_chunks)
    preserved = b"".join(metadata_chunks)
    result = PNG_SIGNATURE + png_chunk(b"IHDR", header) + preserved + png_chunk(b"IDAT", zlib.compress(raw_row * height, 9)) + png_chunk(b"IEND", b"")
    for chunk_type, _payload, raw in parse_png_chunks(result):
        if raw in metadata_chunks and chunk_type not in PNG_PRESERVED_METADATA_TYPES:
            raise AssetError(f"unsupported preserved PNG chunk type: {chunk_type.decode('latin-1')}")
    return result


def png_dimensions(data: bytes) -> tuple[int, int]:
    if not data.startswith(PNG_SIGNATURE) or data[12:16] != b"IHDR":
        raise AssetError("invalid PNG signature or IHDR")
    width, height = struct.unpack(">II", data[16:24])
    return width, height


def jpeg_dimensions(data: bytes) -> tuple[int, int]:
    if not data.startswith(b"\xff\xd8"):
        raise AssetError("invalid JPEG signature")
    offset = 2
    sof_markers = {
        0xC0,
        0xC1,
        0xC2,
        0xC3,
        0xC5,
        0xC6,
        0xC7,
        0xC9,
        0xCA,
        0xCB,
        0xCD,
        0xCE,
        0xCF,
    }
    while offset < len(data):
        while offset < len(data) and data[offset] == 0xFF:
            offset += 1
        if offset >= len(data):
            break
        marker = data[offset]
        offset += 1
        if marker in {0x01, 0xD8, 0xD9} or 0xD0 <= marker <= 0xD7:
            continue
        if offset + 2 > len(data):
            break
        segment_length = struct.unpack_from(">H", data, offset)[0]
        if segment_length < 2 or offset + segment_length > len(data):
            raise AssetError("invalid JPEG segment length")
        if marker in sof_markers:
            if segment_length < 7:
                raise AssetError("short JPEG SOF segment")
            height, width = struct.unpack_from(">HH", data, offset + 3)
            return width, height
        offset += segment_length
    raise AssetError("JPEG has no supported SOF marker")


def webp_dimensions(data: bytes) -> tuple[int, int]:
    if len(data) < 16 or data[:4] != b"RIFF" or data[8:12] != b"WEBP":
        raise AssetError("invalid WebP signature")
    offset = 12
    while offset + 8 <= len(data):
        chunk_type = data[offset : offset + 4]
        chunk_size = struct.unpack_from("<I", data, offset + 4)[0]
        payload = data[offset + 8 : offset + 8 + chunk_size]
        if chunk_type == b"VP8X" and len(payload) >= 10:
            width = int.from_bytes(payload[4:7], "little") + 1
            height = int.from_bytes(payload[7:10], "little") + 1
            return width, height
        if chunk_type == b"VP8L" and len(payload) >= 5 and payload[0] == 0x2F:
            bits = int.from_bytes(payload[1:5], "little")
            return (bits & 0x3FFF) + 1, ((bits >> 14) & 0x3FFF) + 1
        if chunk_type == b"VP8 " and len(payload) >= 10 and payload[3:6] == b"\x9d\x01\x2a":
            width, height = struct.unpack_from("<HH", payload, 6)
            return width & 0x3FFF, height & 0x3FFF
        offset += 8 + chunk_size + (chunk_size & 1)
    raise AssetError("WebP has no supported image chunk")


def parse_webp_chunks(data: bytes) -> list[tuple[bytes, bytes, bytes]]:
    if len(data) < 12 or data[:4] != b"RIFF" or data[8:12] != b"WEBP":
        raise AssetError("invalid WebP signature")
    declared_size = struct.unpack_from("<I", data, 4)[0] + 8
    if declared_size != len(data):
        raise AssetError("WebP RIFF size mismatch")
    chunks: list[tuple[bytes, bytes, bytes]] = []
    offset = 12
    while offset + 8 <= len(data):
        chunk_type = data[offset : offset + 4]
        size = struct.unpack_from("<I", data, offset + 4)[0]
        end = offset + 8 + size + (size & 1)
        if end > len(data):
            raise AssetError("short WebP chunk")
        chunks.append((chunk_type, data[offset + 8 : offset + 8 + size], data[offset:end]))
        offset = end
    if offset != len(data):
        raise AssetError("trailing bytes in WebP container")
    return chunks


def webp_metadata_chunks(data: bytes) -> list[tuple[str, bytes]]:
    return [
        (chunk_type.decode("ascii").rstrip(), raw)
        for chunk_type, _payload, raw in parse_webp_chunks(data)
        if chunk_type in WEBP_PRESERVED_METADATA_TYPES
    ]


def attach_webp_metadata(data: bytes, width: int, height: int, metadata_chunks: Iterable[bytes]) -> bytes:
    metadata_chunks = tuple(metadata_chunks)
    if not metadata_chunks:
        return data
    parsed_metadata: dict[bytes, list[bytes]] = {}
    for raw in metadata_chunks:
        if len(raw) < 8:
            raise AssetError("short preserved WebP metadata chunk")
        chunk_type = raw[:4]
        size = struct.unpack_from("<I", raw, 4)[0]
        if chunk_type not in WEBP_PRESERVED_METADATA_TYPES or len(raw) != 8 + size + (size & 1):
            raise AssetError("invalid preserved WebP metadata chunk")
        parsed_metadata.setdefault(chunk_type, []).append(raw)

    image_chunks = [
        raw
        for chunk_type, _payload, raw in parse_webp_chunks(data)
        if chunk_type != b"VP8X" and chunk_type not in WEBP_PRESERVED_METADATA_TYPES
    ]
    flags = 0
    if b"ICCP" in parsed_metadata:
        flags |= 0x20
    if b"EXIF" in parsed_metadata:
        flags |= 0x08
    if b"XMP " in parsed_metadata:
        flags |= 0x04
    vp8x_payload = bytes([flags, 0, 0, 0]) + (width - 1).to_bytes(3, "little") + (height - 1).to_bytes(3, "little")
    vp8x = b"VP8X" + struct.pack("<I", len(vp8x_payload)) + vp8x_payload
    ordered = [vp8x]
    ordered.extend(parsed_metadata.get(b"ICCP", []))
    ordered.extend(image_chunks)
    ordered.extend(parsed_metadata.get(b"EXIF", []))
    ordered.extend(parsed_metadata.get(b"XMP ", []))
    payload = b"WEBP" + b"".join(ordered)
    return b"RIFF" + struct.pack("<I", len(payload)) + payload


def parse_jpeg_header_segments(data: bytes) -> list[tuple[int, bytes, int, int]]:
    if not data.startswith(b"\xff\xd8"):
        raise AssetError("invalid JPEG signature")
    segments: list[tuple[int, bytes, int, int]] = []
    offset = 2
    while offset < len(data):
        start = offset
        if data[offset] != 0xFF:
            raise AssetError("invalid JPEG marker prefix")
        while offset < len(data) and data[offset] == 0xFF:
            offset += 1
        if offset >= len(data):
            raise AssetError("truncated JPEG marker")
        marker = data[offset]
        offset += 1
        if marker == 0xDA:
            if offset + 2 > len(data):
                raise AssetError("short JPEG scan header")
            size = struct.unpack_from(">H", data, offset)[0]
            end = offset + size
            if size < 2 or end > len(data):
                raise AssetError("invalid JPEG scan header")
            segments.append((marker, data[start:end], start, end))
            break
        if marker in {0x01, 0xD8, 0xD9} or 0xD0 <= marker <= 0xD7:
            segments.append((marker, data[start:offset], start, offset))
            continue
        if offset + 2 > len(data):
            raise AssetError("short JPEG segment")
        size = struct.unpack_from(">H", data, offset)[0]
        end = offset + size
        if size < 2 or end > len(data):
            raise AssetError("invalid JPEG segment length")
        segments.append((marker, data[start:end], start, end))
        offset = end
    return segments


def jpeg_metadata_chunks(data: bytes) -> list[tuple[str, bytes]]:
    return [
        ("COM" if marker == 0xFE else f"APP{marker - 0xE0}", raw)
        for marker, raw, _start, _end in parse_jpeg_header_segments(data)
        if marker in JPEG_PRESERVED_METADATA_MARKERS
    ]


def attach_jpeg_metadata(data: bytes, metadata_chunks: Iterable[bytes]) -> bytes:
    metadata_chunks = tuple(metadata_chunks)
    segments = parse_jpeg_header_segments(data)
    if any(marker in JPEG_PRESERVED_METADATA_MARKERS for marker, _raw, _start, _end in segments):
        rebuilt = bytearray(data[:2])
        for marker, raw, start, _end in segments:
            if marker == 0xDA:
                rebuilt += data[start:]
                break
            if marker not in JPEG_PRESERVED_METADATA_MARKERS:
                rebuilt += raw
        data = bytes(rebuilt)
        segments = parse_jpeg_header_segments(data)
    if not metadata_chunks:
        return data
    insertion = 2
    for marker, _raw, _start, end in segments:
        if marker == 0xE0:
            insertion = end
            continue
        break
    for raw in metadata_chunks:
        if len(raw) < 4 or raw[0] != 0xFF or raw[1] not in JPEG_PRESERVED_METADATA_MARKERS:
            raise AssetError("invalid preserved JPEG metadata segment")
        if struct.unpack_from(">H", raw, 2)[0] + 2 != len(raw):
            raise AssetError("preserved JPEG metadata segment length mismatch")
    return data[:insertion] + b"".join(metadata_chunks) + data[insertion:]


def raster_metadata_chunks(path: Path, data: bytes | None = None) -> list[tuple[str, bytes]]:
    data = path.read_bytes() if data is None else data
    suffix = path.suffix.lower()
    if suffix == ".png":
        return png_metadata_chunks(data)
    if suffix in {".jpg", ".jpeg"}:
        return jpeg_metadata_chunks(data)
    if suffix == ".webp":
        return webp_metadata_chunks(data)
    raise AssetError(f"unsupported raster format: {path}")


def metadata_records(chunks: Iterable[tuple[str, bytes]]) -> list[dict[str, Any]]:
    return [
        {"type": chunk_type, "size": len(raw), "sha256": sha256(raw)}
        for chunk_type, raw in chunks
    ]


def metadata_hash(chunks: Iterable[tuple[str, bytes]]) -> str:
    payload = bytearray()
    for chunk_type, raw in chunks:
        payload += chunk_type.encode("ascii") + b"\x00" + struct.pack(">Q", len(raw)) + raw
    return sha256(bytes(payload))


def raster_dimensions(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    suffix = path.suffix.lower()
    if suffix == ".png":
        return png_dimensions(data)
    if suffix in {".jpg", ".jpeg"}:
        return jpeg_dimensions(data)
    if suffix == ".webp":
        return webp_dimensions(data)
    raise AssetError(f"unsupported raster format: {path}")


def require_ffmpeg() -> str:
    executable = shutil.which("ffmpeg")
    if not executable:
        raise AssetError("ffmpeg is required for JPEG/WebP conversion and validation")
    return executable


def encoded_white_raster(
    path: Path,
    width: int,
    height: int,
    metadata: Iterable[tuple[str, bytes]] | None = None,
) -> bytes:
    suffix = path.suffix.lower()
    metadata = list(raster_metadata_chunks(path) if metadata is None else metadata)
    raw_metadata = [raw for _chunk_type, raw in metadata]
    if suffix == ".png":
        return white_png(width, height, metadata_chunks=raw_metadata)
    ffmpeg = require_ffmpeg()
    with tempfile.TemporaryDirectory(prefix="librepaint-white-raster-") as directory_name:
        directory = Path(directory_name)
        source = directory / "white.png"
        destination = directory / ("white.jpg" if suffix in {".jpg", ".jpeg"} else "white.webp")
        source.write_bytes(white_png(width, height))
        command = [ffmpeg, "-v", "error", "-nostdin", "-i", str(source), "-frames:v", "1", "-map_metadata", "-1"]
        if suffix in {".jpg", ".jpeg"}:
            command += ["-q:v", "1", "-pix_fmt", "yuvj444p"]
        elif suffix == ".webp":
            command += ["-c:v", "libwebp", "-lossless", "1", "-compression_level", "6", "-pix_fmt", "bgra"]
        else:
            raise AssetError(f"unsupported raster format: {path}")
        command += ["-y", str(destination)]
        subprocess.run(command, check=True)
        encoded = destination.read_bytes()
        if suffix in {".jpg", ".jpeg"}:
            return attach_jpeg_metadata(encoded, raw_metadata)
        return attach_webp_metadata(encoded, width, height, raw_metadata)


def raster_is_opaque_white(path: Path, width: int, height: int) -> bool:
    suffix = path.suffix.lower()
    if suffix == ".png":
        metadata = [raw for _chunk_type, raw in raster_metadata_chunks(path)]
        return path.read_bytes() == white_png(width, height, metadata_chunks=metadata)
    ffmpeg = require_ffmpeg()
    command = [
        ffmpeg,
        "-v",
        "error",
        "-nostdin",
        "-i",
        str(path),
        "-f",
        "rawvideo",
        "-pix_fmt",
        "rgba",
        "-",
    ]
    result = subprocess.run(command, check=True, stdout=subprocess.PIPE)
    pixels_are_white = len(result.stdout) == width * height * 4 and result.stdout == b"\xff" * len(result.stdout)
    if not pixels_are_white:
        return False
    return path.read_bytes() == encoded_white_raster(path, width, height)


XPM_STRING_RE = re.compile(r'"((?:\\.|[^"\\])*)"')
XPM_COMMENT_RE = re.compile(r"/\*.*?\*/|//[^\r\n]*", re.DOTALL)


def xpm_dimensions(text: str) -> tuple[int, int]:
    strings = XPM_STRING_RE.findall(text)
    if not strings:
        raise AssetError("XPM has no quoted header")
    fields = strings[0].split()
    if len(fields) < 4:
        raise AssetError("invalid XPM header")
    try:
        width, height, colors, chars_per_pixel = (int(value) for value in fields[:4])
    except ValueError as exc:
        raise AssetError("invalid numeric value in XPM header") from exc
    if width <= 0 or height <= 0 or colors <= 0 or chars_per_pixel <= 0:
        raise AssetError("invalid XPM dimensions or color table")
    return width, height


def xpm_comments_hash(text: str) -> str:
    payload = b"\x00".join(match.group(0).encode("utf-8") for match in XPM_COMMENT_RE.finditer(text))
    return sha256(payload)


def white_xpm_text(text: str) -> str:
    width, height = xpm_dimensions(text)
    comments = [match.group(0) for match in XPM_COMMENT_RE.finditer(text)]
    prefix = "\n".join(comments)
    if prefix:
        prefix += "\n"
    rows = ",\n".join(f'"{"." * width}"' for _index in range(height))
    return (
        prefix
        + "static char *librepaint_white_xpm[] = {\n"
        + f'"{width} {height} 1 1",\n'
        + '". c #FFFFFF",\n'
        + rows
        + "\n};\n"
    )


def android_vector_comments(text: str) -> list[str]:
    return [match.group(0) for match in XML_COMMENT_RE.finditer(text)]


def android_vector_comments_hash(text: str) -> str:
    payload = b"\x00".join(comment.encode("utf-8") for comment in android_vector_comments(text))
    return sha256(payload)


def white_android_vector_text(text: str) -> str:
    comments = android_vector_comments(text)
    prefix = "\n".join(comments)
    if prefix:
        prefix += "\n"
    return prefix + ANDROID_VECTOR_TEMPLATE


SVG_ROOT_RE = re.compile(r"<svg\b[^>]*>", re.IGNORECASE | re.DOTALL)
SVG_CLOSE_RE = re.compile(r"</svg\s*>", re.IGNORECASE)
SVG_ATTRIBUTE_RE = re.compile(r"([:\w.-]+)\s*=\s*([\"'])(.*?)\2", re.DOTALL)
SVG_PRESERVE_RES = (
    re.compile(r"<!--.*?-->", re.DOTALL),
    re.compile(r"<(?:[\w.-]+:)?metadata\b[^>]*>.*?</(?:[\w.-]+:)?metadata\s*>", re.IGNORECASE | re.DOTALL),
    re.compile(r"<(?:[\w.-]+:)?title\b[^>]*>.*?</(?:[\w.-]+:)?title\s*>", re.IGNORECASE | re.DOTALL),
    re.compile(r"<(?:[\w.-]+:)?desc\b[^>]*>.*?</(?:[\w.-]+:)?desc\s*>", re.IGNORECASE | re.DOTALL),
)


def read_svg(path: Path, compressed: bool) -> str:
    data = path.read_bytes()
    if compressed:
        data = gzip.decompress(data)
    return data.decode("utf-8")


def svg_structure(text: str) -> tuple[re.Match[str], re.Match[str], list[tuple[str, str]]]:
    root = SVG_ROOT_RE.search(text)
    if not root:
        raise AssetError("SVG root element not found")
    closes = list(SVG_CLOSE_RE.finditer(text, root.end()))
    if not closes:
        raise AssetError("SVG closing element not found")
    close = closes[-1]
    attributes = [(match.group(1), match.group(3)) for match in SVG_ATTRIBUTE_RE.finditer(root.group(0))]
    return root, close, attributes


def svg_dimensions(text: str) -> dict[str, str | None]:
    _root, _close, attributes = svg_structure(text)
    values = {name.lower(): value for name, value in attributes}
    return {
        "width": values.get("width"),
        "height": values.get("height"),
        "viewBox": values.get("viewbox"),
    }


def svg_preserved_ranges(text: str) -> list[tuple[int, int, str]]:
    candidates: list[tuple[int, int, str]] = []
    for pattern in SVG_PRESERVE_RES:
        candidates.extend((match.start(), match.end(), match.group(0)) for match in pattern.finditer(text))
    candidates.sort(key=lambda item: (item[0], -item[1]))
    outermost: list[tuple[int, int, str]] = []
    for candidate in candidates:
        if any(candidate[0] >= item[0] and candidate[1] <= item[1] for item in outermost):
            continue
        outermost.append(candidate)
    return outermost


def svg_preservation_hash(text: str) -> str:
    payload = b"\x00".join(fragment.encode("utf-8") for _start, _end, fragment in svg_preserved_ranges(text))
    return sha256(payload)


def white_svg_text(text: str) -> str:
    root, close, attributes = svg_structure(text)
    kept_attributes: list[tuple[str, str]] = []
    for name, value in attributes:
        lowered = name.lower()
        if lowered in {"width", "height", "viewbox", "version", "xml:space"} or lowered == "xmlns" or lowered.startswith("xmlns:"):
            kept_attributes.append((name, value))
    if not any(name.lower() == "xmlns" for name, _value in kept_attributes):
        kept_attributes.insert(0, ("xmlns", "http://www.w3.org/2000/svg"))
    attribute_text = "\n    ".join(f'{name}="{value}"' for name, value in kept_attributes)
    inner_fragments = [
        fragment
        for start, end, fragment in svg_preserved_ranges(text)
        if start >= root.end() and end <= close.start()
    ]
    body = ""
    if inner_fragments:
        body = "\n".join(inner_fragments) + "\n"
    return (
        text[: root.start()]
        + f"<svg {attribute_text}>\n"
        + body
        + '  <rect x="0" y="0" width="100%" height="100%" fill="#ffffff" />\n'
        + "</svg>"
        + text[close.end() :]
    )


def encoded_white_svg(path: Path, compressed: bool) -> bytes:
    text = white_svg_text(read_svg(path, compressed))
    data = text.encode("utf-8")
    return gzip.compress(data, compresslevel=9, mtime=0) if compressed else data


def parse_ico(data: bytes) -> list[dict[str, Any]]:
    if len(data) < 6:
        raise AssetError("short ICO header")
    reserved, image_type, count = struct.unpack_from("<HHH", data, 0)
    if reserved != 0 or image_type != 1 or len(data) < 6 + count * 16:
        raise AssetError("invalid ICO header")
    entries: list[dict[str, Any]] = []
    for index in range(count):
        fields = struct.unpack_from("<BBBBHHII", data, 6 + index * 16)
        width_byte, height_byte, colors, entry_reserved, planes, bits, size, offset = fields
        payload = data[offset : offset + size]
        if len(payload) != size:
            raise AssetError("short ICO representation")
        entries.append(
            {
                "width": width_byte or 256,
                "height": height_byte or 256,
                "width_byte": width_byte,
                "height_byte": height_byte,
                "colors": colors,
                "reserved": entry_reserved,
                "planes": planes,
                "bits": bits,
                "encoding": "png" if payload.startswith(PNG_SIGNATURE) else "dib",
                "payload": payload,
            }
        )
    return entries


def ico_manifest_entries(data: bytes) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for entry in parse_ico(data):
        record = {key: entry[key] for key in ("width", "height", "colors", "reserved", "planes", "bits", "encoding")}
        if entry["encoding"] == "png":
            metadata = png_metadata_chunks(entry["payload"])
            if metadata:
                record["preserved_metadata"] = metadata_records(metadata)
                record["preserved_metadata_sha256"] = metadata_hash(metadata)
        records.append(record)
    return records


def white_dib(entry: dict[str, Any]) -> bytes:
    payload = entry["payload"]
    if len(payload) < 40:
        raise AssetError("short ICO DIB")
    header_size, width, doubled_height, planes, bits, compression, _image_size, xppm, yppm, colors_used, important = struct.unpack_from(
        "<IiiHHIIiiII", payload, 0
    )
    expected_width = entry["width"]
    expected_height = entry["height"]
    if header_size != 40 or width != expected_width or doubled_height != expected_height * 2 or planes != 1 or bits != 32 or compression != 0:
        raise AssetError("unsupported ICO DIB representation")
    xor_size = expected_width * expected_height * 4
    mask_stride = ((expected_width + 31) // 32) * 4
    header = struct.pack(
        "<IiiHHIIiiII",
        40,
        expected_width,
        expected_height * 2,
        1,
        32,
        0,
        xor_size,
        xppm,
        yppm,
        colors_used,
        important,
    )
    return header + b"\xff" * xor_size + b"\x00" * (mask_stride * expected_height)


def white_ico(data: bytes) -> bytes:
    entries = parse_ico(data)
    payloads = [
        white_png(
            entry["width"],
            entry["height"],
            alpha=True,
            metadata_chunks=[raw for _chunk_type, raw in png_metadata_chunks(entry["payload"])],
        )
        if entry["encoding"] == "png"
        else white_dib(entry)
        for entry in entries
    ]
    offset = 6 + len(entries) * 16
    directory = bytearray(struct.pack("<HHH", 0, 1, len(entries)))
    for entry, payload in zip(entries, payloads):
        directory += struct.pack(
            "<BBBBHHII",
            entry["width_byte"],
            entry["height_byte"],
            entry["colors"],
            entry["reserved"],
            entry["planes"],
            entry["bits"],
            len(payload),
            offset,
        )
        offset += len(payload)
    return bytes(directory) + b"".join(payloads)


def ico_is_white(data: bytes) -> bool:
    for entry in parse_ico(data):
        payload = entry["payload"]
        if entry["encoding"] == "png":
            metadata = [raw for _chunk_type, raw in png_metadata_chunks(payload)]
            if payload != white_png(entry["width"], entry["height"], alpha=True, metadata_chunks=metadata):
                return False
            continue
        if payload != white_dib(entry):
            return False
    return True


def parse_icns(data: bytes) -> list[tuple[str, bytes]]:
    if len(data) < 8 or data[:4] != b"icns" or struct.unpack_from(">I", data, 4)[0] != len(data):
        raise AssetError("invalid ICNS container")
    chunks: list[tuple[str, bytes]] = []
    offset = 8
    while offset < len(data):
        if offset + 8 > len(data):
            raise AssetError("short ICNS chunk")
        chunk_type = data[offset : offset + 4].decode("latin-1")
        size = struct.unpack_from(">I", data, offset + 4)[0]
        if size < 8 or offset + size > len(data):
            raise AssetError("invalid ICNS chunk length")
        chunks.append((chunk_type, data[offset + 8 : offset + size]))
        offset += size
    return chunks


def icns_manifest_chunks(data: bytes) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for chunk_type, payload in parse_icns(data):
        if chunk_type in ICNS_DIMENSIONS:
            dimension = ICNS_DIMENSIONS[chunk_type]
            record: dict[str, Any] = {"type": chunk_type, "width": dimension, "height": dimension}
            if payload.startswith(PNG_SIGNATURE):
                metadata = png_metadata_chunks(payload)
                if metadata:
                    record["preserved_metadata"] = metadata_records(metadata)
                    record["preserved_metadata_sha256"] = metadata_hash(metadata)
            records.append(record)
        else:
            records.append({"type": chunk_type, "preserved_sha256": sha256(payload)})
    return records


def white_icns(data: bytes) -> bytes:
    chunks = parse_icns(data)
    encoded: list[bytes] = []
    for chunk_type, payload in chunks:
        if chunk_type in ICNS_DIMENSIONS:
            dimension = ICNS_DIMENSIONS[chunk_type]
            metadata = png_metadata_chunks(payload) if payload.startswith(PNG_SIGNATURE) else []
            payload = white_png(
                dimension,
                dimension,
                alpha=True,
                metadata_chunks=[raw for _chunk_type, raw in metadata],
            )
        encoded.append(chunk_type.encode("latin-1") + struct.pack(">I", len(payload) + 8) + payload)
    result = b"icns" + struct.pack(">I", 8 + sum(len(chunk) for chunk in encoded)) + b"".join(encoded)
    return result


def icns_is_white(data: bytes) -> bool:
    for chunk_type, payload in parse_icns(data):
        if chunk_type in ICNS_DIMENSIONS:
            dimension = ICNS_DIMENSIONS[chunk_type]
            metadata = png_metadata_chunks(payload) if payload.startswith(PNG_SIGNATURE) else []
            if payload != white_png(
                dimension,
                dimension,
                alpha=True,
                metadata_chunks=[raw for _chunk_type, raw in metadata],
            ):
                return False
    return True


def bundle_invariant(path: Path) -> str:
    records: list[dict[str, Any]] = []
    with zipfile.ZipFile(path) as archive:
        for info in archive.infolist():
            record: dict[str, Any] = {
                "name": info.filename,
                "date_time": list(info.date_time),
                "compress_type": info.compress_type,
                "flag_bits": info.flag_bits,
                "create_system": info.create_system,
                "create_version": info.create_version,
                "extract_version": info.extract_version,
                "volume": info.volume,
                "internal_attr": info.internal_attr,
                "external_attr": info.external_attr,
                "extra": info.extra.hex(),
                "comment": info.comment.hex(),
            }
            if info.filename != "preview.png":
                record["content_sha256"] = sha256(archive.read(info))
            records.append(record)
        payload = {"archive_comment": archive.comment.hex(), "entries": records}
    return sha256(json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf-8"))


def bundle_preview(path: Path) -> tuple[bytes, zipfile.ZipInfo]:
    with zipfile.ZipFile(path) as archive:
        matches = [info for info in archive.infolist() if info.filename == "preview.png"]
        if len(matches) != 1:
            raise AssetError(f"bundle must contain exactly one root preview.png: {path}")
        return archive.read(matches[0]), matches[0]


def raw_deflate(data: bytes) -> bytes:
    compressor = zlib.compressobj(level=9, method=zlib.DEFLATED, wbits=-15)
    return compressor.compress(data) + compressor.flush()


def patch_bundle_preview(path: Path, replacement: bytes) -> bool:
    current_preview, _preview_info = bundle_preview(path)
    if current_preview == replacement:
        return False
    data = path.read_bytes()
    eocd_offset = data.rfind(ZIP_EOCD_SIGNATURE)
    if eocd_offset < 0 or eocd_offset + 22 > len(data):
        raise AssetError(f"ZIP EOCD not found: {path}")
    _signature, disk, central_disk, disk_entries, entry_count, central_size, central_offset, comment_length = struct.unpack_from(
        "<IHHHHIIH", data, eocd_offset
    )
    if disk != 0 or central_disk != 0 or disk_entries != entry_count or central_offset == 0xFFFFFFFF or central_size == 0xFFFFFFFF:
        raise AssetError(f"ZIP64 or multi-disk bundle is unsupported: {path}")
    if eocd_offset + 22 + comment_length > len(data):
        raise AssetError(f"short ZIP comment: {path}")

    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        infos = archive.infolist()
    local_infos = sorted(infos, key=lambda item: item.header_offset)
    local_output = bytearray()
    cursor = 0
    new_offsets: dict[int, int] = {}
    preview_crc = binascii.crc32(replacement) & 0xFFFFFFFF
    preview_compressed = b""

    for index, info in enumerate(local_infos):
        start = info.header_offset
        end = local_infos[index + 1].header_offset if index + 1 < len(local_infos) else central_offset
        if data[start : start + 4] != ZIP_LOCAL_SIGNATURE:
            raise AssetError(f"invalid local ZIP header for {info.filename}")
        local_output += data[cursor:start]
        new_offsets[start] = len(local_output)
        block = data[start:end]
        if info.filename == "preview.png":
            (
                _local_signature,
                _needed,
                flags,
                compression,
                _time,
                _date,
                _crc,
                _compressed_size,
                _uncompressed_size,
                name_length,
                extra_length,
            ) = struct.unpack_from("<IHHHHHIIIHH", block, 0)
            data_offset = 30 + name_length + extra_length
            suffix_offset = data_offset + info.compress_size
            prefix = bytearray(block[:data_offset])
            suffix = bytearray(block[suffix_offset:])
            if compression == zipfile.ZIP_STORED:
                preview_compressed = replacement
            elif compression == zipfile.ZIP_DEFLATED:
                preview_compressed = raw_deflate(replacement)
            else:
                raise AssetError(f"unsupported preview compression {compression}: {path}")
            if flags & 0x08:
                descriptor_offset = 4 if suffix.startswith(ZIP_DESCRIPTOR_SIGNATURE) else 0
                if len(suffix) < descriptor_offset + 12:
                    raise AssetError(f"short ZIP data descriptor: {path}")
                struct.pack_into("<III", suffix, descriptor_offset, preview_crc, len(preview_compressed), len(replacement))
            else:
                struct.pack_into("<III", prefix, 14, preview_crc, len(preview_compressed), len(replacement))
            block = bytes(prefix) + preview_compressed + bytes(suffix)
        local_output += block
        cursor = end
    local_output += data[cursor:central_offset]

    central_output = bytearray()
    cursor = central_offset
    central_end = central_offset + central_size
    central_records = 0
    while cursor < central_end:
        if data[cursor : cursor + 4] != ZIP_CENTRAL_SIGNATURE or cursor + 46 > central_end:
            raise AssetError(f"invalid central ZIP directory: {path}")
        name_length, extra_length, entry_comment_length = struct.unpack_from("<HHH", data, cursor + 28)
        record_length = 46 + name_length + extra_length + entry_comment_length
        record = bytearray(data[cursor : cursor + record_length])
        old_local_offset = struct.unpack_from("<I", record, 42)[0]
        if old_local_offset not in new_offsets:
            raise AssetError(f"central directory has unknown local offset: {path}")
        struct.pack_into("<I", record, 42, new_offsets[old_local_offset])
        name = bytes(record[46 : 46 + name_length])
        if name == b"preview.png":
            struct.pack_into("<III", record, 16, preview_crc, len(preview_compressed), len(replacement))
        central_output += record
        central_records += 1
        cursor += record_length
    if cursor != central_end or central_records != entry_count:
        raise AssetError(f"central ZIP directory count/size mismatch: {path}")

    between_central_and_eocd = data[central_end:eocd_offset]
    eocd_and_tail = bytearray(data[eocd_offset:])
    struct.pack_into("<II", eocd_and_tail, 12, len(central_output), len(local_output))
    result = bytes(local_output) + bytes(central_output) + between_central_and_eocd + bytes(eocd_and_tail)
    return atomic_write(path, result)


def load_manifest(path: Path) -> dict[str, Any]:
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("schema") != 1:
        raise AssetError(f"unsupported manifest schema in {path}")
    return manifest


def expand_globs(patterns: Iterable[str]) -> list[str]:
    paths: set[str] = set()
    for pattern in patterns:
        matches = [path for path in REPO_ROOT.glob(pattern) if path.is_file()]
        if not matches:
            raise AssetError(f"manifest glob matched no files: {pattern}")
        paths.update(relative_path(path) for path in matches)
    return sorted(paths)


def image_kind(relative: str) -> str:
    suffix = Path(relative).suffix.lower()
    if suffix in {".png", ".jpg", ".jpeg", ".webp"}:
        return "raster"
    if suffix == ".svg":
        return "svg"
    if suffix == ".svgz":
        return "svgz"
    if suffix == ".xpm":
        return "xpm"
    if suffix == ".ico":
        return "ico"
    if suffix == ".icns":
        return "icns"
    raise AssetError(f"unsupported white asset format: {relative}")


def selection_paths(manifest: dict[str, Any]) -> dict[str, list[str]]:
    selection = manifest["selection"]
    result = {
        "raster": expand_globs(selection["raster_globs"]),
        "svg": expand_globs(selection["svg_globs"]),
        "svgz": expand_globs(selection["svgz_globs"]),
        "xpm": [],
        "ico": expand_globs(selection["ico_globs"]),
        "icns": sorted(selection["icns_paths"]),
        "bundle": sorted(selection["bundle_paths"]),
        "android-vector": sorted(selection["android_vector_paths"]),
    }
    for relative in selection.get("ios_qrc_white_paths", []):
        result[image_kind(relative)].append(relative)
    restored_brand_paths = set(selection.get("restored_brand_paths", []))
    originally_selected = {relative for paths in result.values() for relative in paths}
    if restored_brand_paths - originally_selected:
        raise AssetError(
            "restored brand path was not part of the white selection: "
            f"{sorted(restored_brand_paths - originally_selected)}"
        )
    for kind, paths in result.items():
        result[kind] = [relative for relative in paths if relative not in restored_brand_paths]
    for paths in result.values():
        paths.sort()
    seen: dict[str, str] = {}
    for kind, paths in result.items():
        for relative in paths:
            path = repo_path(relative)
            if not path.is_file():
                raise AssetError(f"selected asset does not exist: {relative}")
            if relative in seen:
                raise AssetError(f"selected as both {seen[relative]} and {kind}: {relative}")
            seen[relative] = kind
    return result


def build_asset_record(kind: str, relative: str) -> dict[str, Any]:
    path = repo_path(relative)
    record: dict[str, Any] = {"kind": kind, "path": relative}
    if kind == "raster":
        width, height = raster_dimensions(path)
        record.update({"format": path.suffix.lower().lstrip("."), "width": width, "height": height})
        metadata = raster_metadata_chunks(path)
        if metadata:
            record["preserved_metadata"] = metadata_records(metadata)
            record["preserved_metadata_sha256"] = metadata_hash(metadata)
    elif kind in {"svg", "svgz"}:
        text = read_svg(path, kind == "svgz")
        record.update(svg_dimensions(text))
        record["preserved_sha256"] = svg_preservation_hash(text)
    elif kind == "xpm":
        text = path.read_text(encoding="utf-8")
        width, height = xpm_dimensions(text)
        record.update({"width": width, "height": height, "preserved_sha256": xpm_comments_hash(text)})
    elif kind == "ico":
        record["representations"] = ico_manifest_entries(path.read_bytes())
    elif kind == "icns":
        record["chunks"] = icns_manifest_chunks(path.read_bytes())
    elif kind == "bundle":
        preview, _info = bundle_preview(path)
        width, height = png_dimensions(preview)
        record.update(
            {
                "preview_width": width,
                "preview_height": height,
                "preserved_sha256": bundle_invariant(path),
            }
        )
    elif kind == "android-vector":
        record.update({"width": "108dp", "height": "108dp", "viewportWidth": "1309", "viewportHeight": "1309"})
        record["preserved_sha256"] = android_vector_comments_hash(path.read_text(encoding="utf-8"))
    else:
        raise AssetError(f"unknown asset kind: {kind}")
    return record


def content_inventory_hash(paths: Iterable[str]) -> str:
    payload = bytearray()
    for relative in sorted(paths):
        path = repo_path(relative)
        if not path.is_file():
            raise AssetError(f"retained asset does not exist: {relative}")
        payload += relative.encode("utf-8") + b"\x00" + sha256(path.read_bytes()).encode("ascii") + b"\n"
    return sha256(bytes(payload))


def license_url_inventory_hash(licensed_urls: dict[str, set[str]]) -> str:
    payload = bytearray()
    for relative, urls in sorted(licensed_urls.items()):
        payload += relative.encode("utf-8") + b"\x00"
        payload += b"\x00".join(url.encode("utf-8") for url in sorted(urls))
        payload += b"\n"
    return sha256(bytes(payload))


def installed_image_paths(manifest: dict[str, Any]) -> set[str]:
    audit = manifest["ios_image_audit"]
    definitions = audit.get("installed_image_definition_paths", [])
    if tuple(definitions) != INSTALLED_IMAGE_DEFINITION_PATHS:
        raise AssetError("installed-image CMake definition set changed")

    images: set[str] = set()
    for relative_cmake in definitions:
        cmake_path = repo_path(relative_cmake)
        if not cmake_path.is_file():
            raise AssetError(f"installed-image CMake definition does not exist: {relative_cmake}")
        text = CMAKE_COMMENT_RE.sub("", cmake_path.read_text(encoding="utf-8"))
        for match in CMAKE_INSTALL_FILES_RE.finditer(text):
            for encoded_token in CMAKE_TOKEN_RE.findall(match.group(1)):
                token = re.sub(r"\\(.)", r"\1", encoded_token)
                if Path(token).suffix.lower() not in QRC_IMAGE_SUFFIXES:
                    continue
                if "$" in token:
                    raise AssetError(
                        f"installed image uses an unresolved CMake variable: {relative_cmake}: {encoded_token}"
                    )
                source_path = (cmake_path.parent / token).resolve()
                try:
                    relative_source = source_path.relative_to(REPO_ROOT).as_posix()
                except ValueError as exc:
                    raise AssetError(
                        f"installed image escapes repository: {relative_cmake}: {encoded_token}"
                    ) from exc
                if not source_path.is_file():
                    raise AssetError(
                        f"installed image does not exist: {relative_cmake}: {encoded_token}"
                    )
                images.add(relative_source)
    return images


def retained_group_record(group_id: str, paths: Iterable[str]) -> dict[str, Any]:
    contract = RETAINED_LICENSE_GROUP_CONTRACT[group_id]
    return {
        "id": group_id,
        **contract,
        "paths": sorted(paths),
        "expected": {},
    }


def migrate_retained_license_groups(manifest: dict[str, Any]) -> None:
    selection = manifest["selection"]
    old_qrc = selection.pop("ios_qrc_unresolved_paths", None)
    old_installed = selection.pop("ios_installed_only_paths", None)
    if (old_qrc is None) != (old_installed is None):
        raise AssetError("legacy retained-image path lists are only partially present")
    if old_qrc is None:
        return

    old_qrc_set = set(old_qrc)
    breeze_paths = {
        relative
        for relative in old_qrc_set
        if relative.startswith("krita/pics/Breeze-dark/")
        or relative.startswith("krita/pics/Breeze-light/")
    }
    project_paths = old_qrc_set - breeze_paths
    if not MIGRATED_ADOBE_PDF_ICON_PATHS <= breeze_paths:
        raise AssetError("legacy QRC white set is missing one or more Adobe PDF icons")
    breeze_paths.difference_update(MIGRATED_ADOBE_PDF_ICON_PATHS)
    breeze_paths.update(MIGRATED_BREEZE_LOGO_PATHS)
    selection["ios_qrc_white_paths"] = sorted(MIGRATED_ADOBE_PDF_ICON_PATHS)

    raster_globs = selection["raster_globs"]
    if MIGRATED_ABOUT_KDE_PATH not in raster_globs:
        raise AssetError("standard About KDE artwork is absent from the legacy white selection")
    raster_globs.remove(MIGRATED_ABOUT_KDE_PATH)
    svg_globs = selection["svg_globs"]
    for relative in sorted(MIGRATED_BREEZE_LOGO_PATHS):
        if relative not in svg_globs:
            raise AssetError(f"Breeze KDE attribution icon is absent from the legacy white selection: {relative}")
        svg_globs.remove(relative)

    manifest["ios_image_audit"]["retained_license_groups"] = [
        retained_group_record("krita-functional-qrc", project_paths),
        retained_group_record("kde-breeze-icons-qrc", breeze_paths),
        retained_group_record("kde-about-dialog-qrc", [MIGRATED_ABOUT_KDE_PATH]),
        retained_group_record("krita-functional-installed", old_installed),
    ]
    manifest["policy"]["scope_policy"] = (
        "Keep functional images when a project-, family-, or asset-level license applies; "
        "replace branding and unresolved artwork with same-size white content."
    )


def migrate_precise_retained_license_groups(manifest: dict[str, Any]) -> None:
    audit = manifest["ios_image_audit"]
    groups = audit.get("retained_license_groups", [])
    observed_ids = [group.get("id") for group in groups]
    current_ids = list(RETAINED_LICENSE_GROUP_CONTRACT)
    if observed_ids == current_ids:
        return

    previous_ids = current_ids[:-1]
    if (
        observed_ids == previous_ids
        and current_ids[-1] == "librepaint-branding-qrc"
    ):
        groups.append(
            retained_group_record(
                "librepaint-branding-qrc",
                LIBREPAINT_BRANDING_QRC_PATHS,
            )
        )
        return

    legacy_ids = [
        "krita-functional-qrc",
        "kde-breeze-icons-qrc",
        "kde-about-dialog-qrc",
        "krita-functional-installed",
    ]
    if observed_ids != legacy_ids:
        raise AssetError(f"cannot migrate retained license groups: {observed_ids}")

    legacy = {group["id"]: set(group["paths"]) for group in groups}
    project_paths = legacy["krita-functional-qrc"]
    legacy_breeze_paths = legacy["kde-breeze-icons-qrc"]
    about_paths = legacy["kde-about-dialog-qrc"]
    installed_paths = legacy["krita-functional-installed"]
    if len(project_paths) != 480 or about_paths != {MIGRATED_ABOUT_KDE_PATH}:
        raise AssetError("legacy retained functional inventory changed before precise-license migration")

    raw_cc_paths = {
        relative
        for relative in project_paths
        if b"creativecommons.org/licenses/by-sa/4.0" in repo_path(relative).read_bytes()
    }
    if len(raw_cc_paths) != 93:
        raise AssetError(f"raw CC-BY-SA-4.0 evidence set changed: {len(raw_cc_paths)} != 93")
    cc_paths = (raw_cc_paths - MIGRATED_ANDROID_ROBOT_PATHS - MIGRATED_PROJECT_WHITE_PATHS) | MIGRATED_COMMIT_CC_PATHS

    root_pics_paths = {relative for relative in project_paths if relative.startswith("pics/")}
    root_breeze_paths = root_pics_paths - MIGRATED_ROOT_PICS_GPL_PATHS
    kxmlgui_paths = {MIGRATED_KXMLGUI_THUMB_PATH}
    classified = (
        cc_paths
        | MIGRATED_OXYGEN_PATHS
        | root_breeze_paths
        | MIGRATED_ANDROID_ROBOT_PATHS
        | kxmlgui_paths
        | MIGRATED_PROJECT_WHITE_PATHS
    )
    gpl_paths = project_paths - classified
    groups_to_check = {
        "CC-BY-SA-4.0": (cc_paths, 93, "4651e48e840baa15d07a0bae1cfebc56ff848c899c40494459a2a7d6232e79f4"),
        "Oxygen": (MIGRATED_OXYGEN_PATHS, 22, "a6899f49689403f35da7b5969859b51534367cbc9cf3ff4759399019031048c7"),
        "root Breeze": (root_breeze_paths, 210, "05c59cff7b514a7e33f94aefad88900d9b088fb04f1915a28d8e3cad2b82c1df"),
        "root GPL fallback": (MIGRATED_ROOT_PICS_GPL_PATHS, 18, "e4c5c41c6a4f76950c61b6df42f6de19a9c2781f439ee9972792eebdee4a262d"),
        "project GPL fallback": (gpl_paths, 148, "50c2f1c4c8033dc094ec18788bed902bd34986db52286217204e803c43bc8a49"),
        "project white": (MIGRATED_PROJECT_WHITE_PATHS, 4, "cbfdbb011fc41cc55d023d26cd2fb80605fe9fa248f48a6622f959897a4a6787"),
    }
    for label, (paths, expected_count, expected_hash) in groups_to_check.items():
        if len(paths) != expected_count or sorted_path_hash(paths) != expected_hash:
            raise AssetError(f"{label} migration inventory changed")
    if classified | gpl_paths != project_paths or len(classified) + len(gpl_paths) != len(project_paths):
        raise AssetError("precise-license migration does not partition the legacy project group")

    selection = manifest["selection"]
    white_paths = set(selection.get("ios_qrc_white_paths", []))
    white_paths.update(MIGRATED_PROJECT_WHITE_PATHS)
    selection["ios_qrc_white_paths"] = sorted(white_paths)
    audit["retained_license_groups"] = [
        retained_group_record("krita-cc-by-sa-4-functional-qrc", cc_paths),
        retained_group_record("kde-oxygen-icons-qrc", MIGRATED_OXYGEN_PATHS),
        retained_group_record("kde-breeze-icons-qrc", legacy_breeze_paths | root_breeze_paths),
        retained_group_record("android-robot-functional-qrc", MIGRATED_ANDROID_ROBOT_PATHS),
        retained_group_record("kde-kxmlgui-functional-qrc", about_paths | kxmlgui_paths),
        retained_group_record("krita-functional-qrc", gpl_paths),
        retained_group_record("krita-functional-installed", installed_paths),
    ]
    manifest["policy"]["scope_policy"] = (
        "Keep functional images under the most specific verified asset, family, or project license; "
        "replace application branding and unresolved artwork with same-size white content."
    )


def migrate_installed_image_audit(manifest: dict[str, Any]) -> None:
    audit = manifest["ios_image_audit"]
    audit["installed_image_definition_paths"] = list(INSTALLED_IMAGE_DEFINITION_PATHS)
    scope_definitions = audit["scope_definition_paths"]
    for relative in INSTALLED_IMAGE_DEFINITION_PATHS:
        if relative not in scope_definitions:
            scope_definitions.append(relative)


def refresh_retained_group_expectations(manifest: dict[str, Any]) -> None:
    groups = manifest["ios_image_audit"].get("retained_license_groups", [])
    for group in groups:
        group_id = group.get("id")
        if group_id not in RETAINED_LICENSE_GROUP_CONTRACT:
            raise AssetError(f"unknown retained license group while generating: {group_id}")
        group.update(RETAINED_LICENSE_GROUP_CONTRACT[group_id])
        notice_path = repo_path(group["notice_path"])
        group["notice_sha256"] = sha256(notice_path.read_bytes())
        paths = group["paths"]
        group["expected"] = {
            "files": len(paths),
            "paths_sha256": sorted_path_hash(paths),
            "content_sha256": content_inventory_hash(paths),
        }


def retained_group_paths(
    manifest: dict[str, Any],
) -> tuple[set[str], set[str], dict[str, set[str]]]:
    groups = manifest["ios_image_audit"].get("retained_license_groups", [])
    observed_ids = [group.get("id") for group in groups]
    expected_ids = list(RETAINED_LICENSE_GROUP_CONTRACT)
    if observed_ids != expected_ids:
        raise AssetError(f"retained license group IDs changed: {observed_ids} != {expected_ids}")

    qrc_paths: set[str] = set()
    installed_paths: set[str] = set()
    by_id: dict[str, set[str]] = {}
    for group in groups:
        group_id = group["id"]
        contract = RETAINED_LICENSE_GROUP_CONTRACT[group_id]
        for key, value in contract.items():
            if group.get(key) != value:
                raise AssetError(f"retained group {group_id} {key} changed: {group.get(key)!r} != {value!r}")
        paths = group.get("paths", [])
        if paths != sorted(set(paths)):
            raise AssetError(f"retained group {group_id} paths are not sorted and unique")
        path_set = set(paths)
        if (qrc_paths | installed_paths) & path_set:
            raise AssetError(f"retained group {group_id} overlaps another retained group")
        expected = group.get("expected", {})
        if expected.get("files") != len(paths):
            raise AssetError(f"retained group {group_id} file count changed")
        if expected.get("paths_sha256") != sorted_path_hash(paths):
            raise AssetError(f"retained group {group_id} path inventory changed")
        if expected.get("content_sha256") != content_inventory_hash(paths):
            raise AssetError(f"retained group {group_id} source content changed")
        notice_path = repo_path(group["notice_path"])
        if not notice_path.is_file() or group.get("notice_sha256") != sha256(notice_path.read_bytes()):
            raise AssetError(f"retained group {group_id} notice changed")
        if group["scope"] == "qrc":
            qrc_paths.update(path_set)
        elif group["scope"] == "installed-only":
            installed_paths.update(path_set)
        else:
            raise AssetError(f"retained group {group_id} has unsupported scope: {group['scope']}")
        by_id[group_id] = path_set
    return qrc_paths, installed_paths, by_id


def refresh_ios_image_expectations(manifest: dict[str, Any]) -> None:
    audit = manifest["ios_image_audit"]
    qrc_images = qrc_image_paths(manifest)
    white_paths = {record["path"] for record in manifest.get("assets", [])}
    zero_paths = set(audit["intentional_zero_paths"])
    retained_qrc, installed_only, _groups = retained_group_paths(manifest)
    discovered_installed_only = installed_image_paths(manifest)
    licensed_urls = {
        relative: urls
        for relative in qrc_images
        if (urls := strict_svg_license_urls(relative))
    }
    observed_urls = set().union(*licensed_urls.values()) if licensed_urls else set()
    if observed_urls != set(audit["allowed_strict_license_urls"]):
        raise AssetError(f"strict SVG license URL set changed while generating: {sorted(observed_urls)}")
    white_qrc = qrc_images & white_paths
    zero_qrc = qrc_images & zero_paths
    linked_art = set(licensed_urls) - white_qrc - zero_qrc - retained_qrc
    unresolved = qrc_images - white_qrc - zero_qrc - linked_art - retained_qrc
    if unresolved:
        raise AssetError(f"cannot generate manifest with unclassified iOS QRC images: {sorted(unresolved)}")
    if retained_qrc - qrc_images:
        raise AssetError(f"retained QRC group contains non-QRC images: {sorted(retained_qrc - qrc_images)}")
    if installed_only & qrc_images:
        raise AssetError("retained installed-only images unexpectedly overlap QRC images")
    if installed_only != discovered_installed_only:
        raise AssetError(
            "retained installed-only classification differs from CMake install definitions: "
            f"missing={sorted(discovered_installed_only - installed_only)}, "
            f"extra={sorted(installed_only - discovered_installed_only)}"
        )

    scope_definitions = audit["scope_definition_paths"]
    expected_registrations = set(audit["static_resource_registration_paths"])
    observed_registrations = static_resource_registration_paths()
    if observed_registrations != expected_registrations:
        raise AssetError("cannot generate manifest after static-resource registration scope changed")
    audit["expected"] = {
        "core_qrc_files": len(audit["core_qrc_paths"]),
        "static_plugin_qrc_files": len(audit["static_plugin_qrc_paths"]),
        "scope_definition_files": len(scope_definitions),
        "scope_definition_sha256": scope_definition_hash(scope_definitions),
        "static_resource_registration_files": len(expected_registrations),
        "qrc_unique_images": len(qrc_images),
        "qrc_image_paths_sha256": sorted_path_hash(qrc_images),
        "retained_qrc_images": len(retained_qrc),
        "retained_qrc_paths_sha256": sorted_path_hash(retained_qrc),
        "strict_metadata_images": len(licensed_urls),
        "strict_metadata_paths_sha256": sorted_path_hash(licensed_urls),
        "strict_metadata_content_sha256": content_inventory_hash(licensed_urls),
        "strict_metadata_license_urls_sha256": license_url_inventory_hash(licensed_urls),
        "strict_white_overlap": len(set(licensed_urls) & white_qrc),
        "linked_art": len(linked_art),
        "linked_art_paths_sha256": sorted_path_hash(linked_art),
        "linked_art_content_sha256": content_inventory_hash(linked_art),
        "white": len(white_qrc),
        "zero": len(zero_qrc),
        "unresolved": len(unresolved),
        "installed_only_images": len(installed_only),
        "installed_image_definition_files": len(audit["installed_image_definition_paths"]),
        "installed_image_definition_paths_sha256": sorted_path_hash(
            audit["installed_image_definition_paths"]
        ),
        "installed_only_paths_sha256": sorted_path_hash(installed_only),
        "installed_only_retained": len(installed_only),
        "installed_only_white": len(installed_only & white_paths),
    }


def generate_manifest(path: Path) -> int:
    manifest = load_manifest(path)
    migrate_retained_license_groups(manifest)
    migrate_precise_retained_license_groups(manifest)
    migrate_installed_image_audit(manifest)
    refresh_retained_group_expectations(manifest)
    selected = selection_paths(manifest)
    assets = [build_asset_record(kind, relative) for kind, paths in selected.items() for relative in paths]
    assets.sort(key=lambda record: (record["path"], record["kind"]))
    manifest["assets"] = assets
    legal_audit = manifest.get("legal_metadata_audit")
    if legal_audit is not None:
        legal_paths = sorted(
            record["path"]
            for record in assets
            if record["kind"] == "raster" and raster_contains_legal_metadata(repo_path(record["path"]))
        )
        legal_audit["asset_paths"] = legal_paths
        chunk_count, aggregate = legal_metadata_aggregate(legal_paths)
        legal_audit["expected"] = {
            "assets": len(legal_paths),
            "asset_paths_sha256": sorted_path_hash(legal_paths),
            "metadata_chunks": chunk_count,
            "metadata_bytes_sha256": aggregate,
        }
        manifest["policy"]["legal_wording_gate"] = (
            f"The {len(legal_paths)} white raster assets with copyright, Creative Commons, or "
            "attribution/share-alike bytes are exact-hash checked; legal wording is never rewritten."
        )
    refresh_ios_image_expectations(manifest)
    encoded = (json.dumps(manifest, indent=2, ensure_ascii=False) + "\n").encode("utf-8")
    changed = atomic_write(path, encoded)
    print(f"manifest: {len(assets)} assets ({'updated' if changed else 'unchanged'})")
    return 0


def assert_record_invariants(record: dict[str, Any]) -> None:
    path = repo_path(record["path"])
    kind = record["kind"]
    if kind == "raster":
        actual = raster_dimensions(path)
        expected = (record["width"], record["height"])
        if actual != expected:
            raise AssetError(f"dimension mismatch for {record['path']}: {actual} != {expected}")
        metadata = raster_metadata_chunks(path)
        if metadata_records(metadata) != record.get("preserved_metadata", []):
            raise AssetError(f"embedded raster metadata topology/bytes changed: {record['path']}")
        if metadata and metadata_hash(metadata) != record.get("preserved_metadata_sha256"):
            raise AssetError(f"embedded raster metadata hash changed: {record['path']}")
        if not metadata and "preserved_metadata_sha256" in record:
            raise AssetError(f"embedded raster metadata unexpectedly missing: {record['path']}")
    elif kind in {"svg", "svgz"}:
        text = read_svg(path, kind == "svgz")
        expected_dimensions = {key: record.get(key) for key in ("width", "height", "viewBox")}
        if svg_dimensions(text) != expected_dimensions:
            raise AssetError(f"SVG dimensions changed: {record['path']}")
        if svg_preservation_hash(text) != record["preserved_sha256"]:
            raise AssetError(f"SVG metadata/comments/title/desc changed: {record['path']}")
    elif kind == "xpm":
        text = path.read_text(encoding="utf-8")
        if xpm_dimensions(text) != (record["width"], record["height"]):
            raise AssetError(f"XPM dimensions changed: {record['path']}")
        if xpm_comments_hash(text) != record["preserved_sha256"]:
            raise AssetError(f"XPM comments changed: {record['path']}")
    elif kind == "ico":
        if ico_manifest_entries(path.read_bytes()) != record["representations"]:
            raise AssetError(f"ICO representation topology changed: {record['path']}")
    elif kind == "icns":
        if icns_manifest_chunks(path.read_bytes()) != record["chunks"]:
            raise AssetError(f"ICNS representation topology or preserved chunk changed: {record['path']}")
    elif kind == "bundle":
        preview, _info = bundle_preview(path)
        if png_dimensions(preview) != (record["preview_width"], record["preview_height"]):
            raise AssetError(f"bundle preview dimensions changed: {record['path']}")
        if bundle_invariant(path) != record["preserved_sha256"]:
            raise AssetError(f"bundle non-preview content/order/metadata changed: {record['path']}")
    elif kind == "android-vector":
        text = path.read_text(encoding="utf-8")
        if android_vector_comments_hash(text) != record["preserved_sha256"]:
            raise AssetError(f"Android vector XML comments changed: {record['path']}")
    else:
        raise AssetError(f"unknown asset kind: {kind}")


def replace_assets(path: Path) -> int:
    manifest = load_manifest(path)
    if not manifest.get("assets"):
        raise AssetError("manifest assets are empty; run --generate-manifest first")
    changed_by_kind: dict[str, int] = {}
    for record in manifest["assets"]:
        kind = record["kind"]
        asset_path = repo_path(record["path"])
        assert_record_invariants(record)
        changed = False
        if kind == "raster":
            replacement = encoded_white_raster(asset_path, record["width"], record["height"])
            changed = atomic_write(asset_path, replacement)
        elif kind in {"svg", "svgz"}:
            replacement = encoded_white_svg(asset_path, kind == "svgz")
            changed = atomic_write(asset_path, replacement)
        elif kind == "xpm":
            replacement = white_xpm_text(asset_path.read_text(encoding="utf-8")).encode("utf-8")
            changed = atomic_write(asset_path, replacement)
        elif kind == "ico":
            changed = atomic_write(asset_path, white_ico(asset_path.read_bytes()))
        elif kind == "icns":
            changed = atomic_write(asset_path, white_icns(asset_path.read_bytes()))
        elif kind == "bundle":
            replacement = white_png(record["preview_width"], record["preview_height"])
            changed = patch_bundle_preview(asset_path, replacement)
        elif kind == "android-vector":
            replacement = white_android_vector_text(asset_path.read_text(encoding="utf-8")).encode("utf-8")
            changed = atomic_write(asset_path, replacement)
        if changed:
            changed_by_kind[kind] = changed_by_kind.get(kind, 0) + 1
    total = sum(changed_by_kind.values())
    details = ", ".join(f"{kind}={count}" for kind, count in sorted(changed_by_kind.items())) or "no changes"
    print(f"replacement: {total} files changed ({details})")
    return 0


def check_selection(manifest: dict[str, Any]) -> None:
    selected = selection_paths(manifest)
    expected = sorted((kind, relative) for kind, paths in selected.items() for relative in paths)
    recorded = sorted((record["kind"], record["path"]) for record in manifest.get("assets", []))
    if expected != recorded:
        raise AssetError("expanded selection does not match manifest assets; run --generate-manifest")


def sorted_path_hash(paths: Iterable[str]) -> str:
    payload = "".join(f"{relative}\n" for relative in sorted(paths)).encode("utf-8")
    return sha256(payload)


def raster_contains_legal_metadata(path: Path) -> bool:
    return any(LEGAL_METADATA_RE.search(raw) for _chunk_type, raw in raster_metadata_chunks(path))


def legal_metadata_aggregate(paths: Iterable[str]) -> tuple[int, str]:
    payload = bytearray()
    chunk_count = 0
    for relative in sorted(paths):
        chunks = raster_metadata_chunks(repo_path(relative))
        if not chunks:
            raise AssetError(f"legal-metadata asset has no preserved metadata chunks: {relative}")
        if not any(LEGAL_METADATA_RE.search(raw) for _chunk_type, raw in chunks):
            raise AssetError(f"legal wording is absent from preserved metadata bytes: {relative}")
        payload += relative.encode("utf-8") + b"\x00"
        for chunk_type, raw in chunks:
            payload += chunk_type.encode("ascii") + b"\x00" + struct.pack(">Q", len(raw)) + raw
            chunk_count += 1
    return chunk_count, sha256(bytes(payload))


def non_svg_embedded_legal_paths(manifest: dict[str, Any]) -> set[str]:
    paths: set[str] = set()
    for record in manifest.get("assets", []):
        relative = record["path"]
        path = repo_path(relative)
        kind = record["kind"]
        payloads: list[bytes] = []
        if kind == "raster":
            payloads.extend(raw for _chunk_type, raw in raster_metadata_chunks(path))
        elif kind == "ico":
            for entry in parse_ico(path.read_bytes()):
                if entry["encoding"] == "png":
                    payloads.extend(raw for _chunk_type, raw in png_metadata_chunks(entry["payload"]))
                else:
                    payloads.append(entry["payload"])
        elif kind == "icns":
            for _chunk_type, payload in parse_icns(path.read_bytes()):
                if payload.startswith(PNG_SIGNATURE):
                    payloads.extend(raw for _metadata_type, raw in png_metadata_chunks(payload))
                else:
                    payloads.append(payload)
        elif kind == "bundle":
            preview, _info = bundle_preview(path)
            payloads.extend(raw for _chunk_type, raw in png_metadata_chunks(preview))
        elif kind == "android-vector":
            payloads.extend(comment.encode("utf-8") for comment in android_vector_comments(path.read_text(encoding="utf-8")))
        if any(LEGAL_METADATA_RE.search(payload) for payload in payloads):
            paths.add(relative)
    return paths


def audit_legal_metadata(manifest: dict[str, Any]) -> int:
    audit = manifest["legal_metadata_audit"]
    paths = audit["asset_paths"]
    expected = audit["expected"]
    require_expected(len(paths), expected, "assets")
    require_expected(sorted_path_hash(paths), expected, "asset_paths_sha256")
    observed_paths = non_svg_embedded_legal_paths(manifest)
    if observed_paths != set(paths):
        raise AssetError(
            "embedded legal-metadata asset set changed: "
            f"{sorted(observed_paths)} != {sorted(paths)}"
        )
    chunk_count, aggregate = legal_metadata_aggregate(paths)
    require_expected(chunk_count, expected, "metadata_chunks")
    require_expected(aggregate, expected, "metadata_bytes_sha256")
    print(
        "legal metadata audit: "
        f"assets={len(paths)}, chunks={chunk_count}, byte-preservation-sha256={aggregate}"
    )
    return 0


def scope_definition_hash(paths: Iterable[str]) -> str:
    payload = bytearray()
    for relative in sorted(paths):
        path = repo_path(relative)
        if not path.is_file():
            raise AssetError(f"iOS image scope-definition file does not exist: {relative}")
        payload += relative.encode("utf-8") + b"\x00" + sha256(path.read_bytes()).encode("ascii") + b"\n"
    return sha256(bytes(payload))


def static_resource_registration_paths() -> set[str]:
    registration = re.compile(r"(?m)^[ \t]*krita_ios_add_static_resource\s*\(")
    paths: set[str] = set()
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
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError as exc:
            raise AssetError(f"cannot decode CMake scope source: {relative_path(path)}") from exc
        if registration.search(text):
            paths.add(relative_path(path))
    return paths


def qrc_image_paths(manifest: dict[str, Any]) -> set[str]:
    audit = manifest["ios_image_audit"]
    core_qrcs = audit["core_qrc_paths"]
    static_qrcs = audit["static_plugin_qrc_paths"]
    if len(core_qrcs) != audit["expected"]["core_qrc_files"]:
        raise AssetError("iOS core QRC file count changed")
    if len(static_qrcs) != audit["expected"]["static_plugin_qrc_files"]:
        raise AssetError("iOS static-plugin QRC file count changed")
    qrcs = core_qrcs + static_qrcs
    if len(set(qrcs)) != len(qrcs):
        raise AssetError("duplicate QRC path in iOS image audit")

    images: set[str] = set()
    for relative_qrc in qrcs:
        qrc_path = repo_path(relative_qrc)
        if not qrc_path.is_file():
            raise AssetError(f"iOS image-audit QRC does not exist: {relative_qrc}")
        try:
            root = ET.parse(qrc_path).getroot()
        except ET.ParseError as exc:
            raise AssetError(f"cannot parse QRC {relative_qrc}: {exc}") from exc
        for element in root.iter("file"):
            source = (element.text or "").strip()
            if not source:
                raise AssetError(f"empty <file> entry in {relative_qrc}")
            source_path = (qrc_path.parent / source).resolve()
            try:
                relative_source = source_path.relative_to(REPO_ROOT).as_posix()
            except ValueError as exc:
                raise AssetError(f"QRC image escapes repository: {relative_qrc}: {source}") from exc
            if source_path.suffix.lower() not in QRC_IMAGE_SUFFIXES:
                continue
            if not source_path.is_file():
                raise AssetError(f"QRC image does not exist: {relative_qrc}: {source}")
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


def require_expected(actual: int | str, expected: dict[str, Any], key: str) -> None:
    if actual != expected[key]:
        raise AssetError(f"iOS image audit {key} changed: {actual!r} != {expected[key]!r}")


def audit_ios_images(manifest: dict[str, Any], *, verify_white: bool = True) -> int:
    audit = manifest["ios_image_audit"]
    expected = audit["expected"]
    qrc_images = qrc_image_paths(manifest)
    records = {record["path"]: record for record in manifest.get("assets", [])}
    white_paths = set(records)
    zero_paths = set(audit["intentional_zero_paths"])
    retained_qrc, installed_only, retained_groups = retained_group_paths(manifest)
    discovered_installed_only = installed_image_paths(manifest)
    scope_definitions = audit["scope_definition_paths"]
    expected_registrations = set(audit["static_resource_registration_paths"])
    observed_registrations = static_resource_registration_paths()

    require_expected(len(scope_definitions), expected, "scope_definition_files")
    require_expected(scope_definition_hash(scope_definitions), expected, "scope_definition_sha256")
    require_expected(len(expected_registrations), expected, "static_resource_registration_files")
    if observed_registrations != expected_registrations:
        raise AssetError(
            "iOS static-resource registration sources changed: "
            f"{sorted(observed_registrations)} != {sorted(expected_registrations)}"
        )
    require_expected(len(qrc_images), expected, "qrc_unique_images")
    require_expected(sorted_path_hash(qrc_images), expected, "qrc_image_paths_sha256")
    require_expected(len(retained_qrc), expected, "retained_qrc_images")
    require_expected(sorted_path_hash(retained_qrc), expected, "retained_qrc_paths_sha256")
    require_expected(len(installed_only), expected, "installed_only_images")
    require_expected(sorted_path_hash(installed_only), expected, "installed_only_paths_sha256")
    require_expected(
        len(audit["installed_image_definition_paths"]),
        expected,
        "installed_image_definition_files",
    )
    require_expected(
        sorted_path_hash(audit["installed_image_definition_paths"]),
        expected,
        "installed_image_definition_paths_sha256",
    )
    if qrc_images & installed_only:
        raise AssetError("installed-only iOS images unexpectedly overlap QRC images")
    if installed_only != discovered_installed_only:
        raise AssetError(
            "retained installed-only classification differs from CMake install definitions: "
            f"missing={sorted(discovered_installed_only - installed_only)}, "
            f"extra={sorted(installed_only - discovered_installed_only)}"
        )
    if not retained_qrc <= qrc_images:
        extras = sorted(retained_qrc - qrc_images)
        raise AssetError(f"retained QRC group contains non-QRC images: {extras}")

    licensed_urls = {relative: strict_svg_license_urls(relative) for relative in qrc_images}
    licensed_urls = {relative: urls for relative, urls in licensed_urls.items() if urls}
    observed_urls = set().union(*licensed_urls.values()) if licensed_urls else set()
    allowed_urls = set(audit["allowed_strict_license_urls"])
    if observed_urls != allowed_urls:
        raise AssetError(f"strict SVG license URL set changed: {sorted(observed_urls)} != {sorted(allowed_urls)}")

    white_qrc = qrc_images & white_paths
    zero_qrc = qrc_images & zero_paths
    linked_art = set(licensed_urls) - white_qrc - zero_qrc - retained_qrc
    unresolved = qrc_images - white_qrc - zero_qrc - linked_art - retained_qrc
    strict_white_overlap = set(licensed_urls) & white_qrc

    require_expected(len(licensed_urls), expected, "strict_metadata_images")
    require_expected(sorted_path_hash(licensed_urls), expected, "strict_metadata_paths_sha256")
    require_expected(content_inventory_hash(licensed_urls), expected, "strict_metadata_content_sha256")
    require_expected(
        license_url_inventory_hash(licensed_urls),
        expected,
        "strict_metadata_license_urls_sha256",
    )
    require_expected(len(strict_white_overlap), expected, "strict_white_overlap")
    require_expected(len(linked_art), expected, "linked_art")
    require_expected(sorted_path_hash(linked_art), expected, "linked_art_paths_sha256")
    require_expected(content_inventory_hash(linked_art), expected, "linked_art_content_sha256")
    require_expected(len(white_qrc), expected, "white")
    require_expected(len(zero_qrc), expected, "zero")
    require_expected(len(unresolved), expected, "unresolved")
    require_expected(len(installed_only), expected, "installed_only_retained")
    require_expected(len(installed_only & white_paths), expected, "installed_only_white")

    for relative in zero_paths:
        path = repo_path(relative)
        if not path.is_file() or path.stat().st_size != 0:
            raise AssetError(f"intentional zero-byte QRC image changed: {relative}")
    if verify_white:
        for relative in sorted(white_qrc | (installed_only & white_paths)):
            try:
                assert_record_is_white(records[relative])
            except (AssetError, OSError, subprocess.CalledProcessError, zipfile.BadZipFile) as exc:
                raise AssetError(f"iOS white image check failed for {relative}: {exc}") from exc

    audit_legal_metadata(manifest)
    print(
        "iOS image audit: "
        f"QRC unique={len(qrc_images)} "
        f"(linked-art={len(linked_art)}, retained-licensed={len(retained_qrc)}, "
        f"white={len(white_qrc)}, zero={len(zero_qrc)}, unresolved={len(unresolved)}; "
        f"strict-metadata={len(licensed_urls)}, strict/white-overlap={len(strict_white_overlap)}); "
        f"installed-only={len(installed_only)} (retained={len(installed_only)}, white={len(installed_only & white_paths)}); "
        f"retained-groups={len(retained_groups)}"
    )
    return 0


def assert_record_is_white(record: dict[str, Any]) -> None:
    kind = record["kind"]
    asset_path = repo_path(record["path"])
    assert_record_invariants(record)
    if kind == "raster" and not raster_is_opaque_white(asset_path, record["width"], record["height"]):
        raise AssetError("raster pixels are not opaque white")
    if kind in {"svg", "svgz"}:
        text = read_svg(asset_path, kind == "svgz")
        if white_svg_text(text) != text:
            raise AssetError("SVG contains non-preserved/non-white drawing content")
        if kind == "svgz" and asset_path.read_bytes() != gzip.compress(text.encode("utf-8"), compresslevel=9, mtime=0):
            raise AssetError("SVGZ stream is not deterministic")
    if kind == "xpm":
        text = asset_path.read_text(encoding="utf-8")
        if white_xpm_text(text) != text:
            raise AssetError("XPM is not the deterministic opaque-white image")
    if kind == "ico" and not ico_is_white(asset_path.read_bytes()):
        raise AssetError("one or more ICO representations are not opaque white")
    if kind == "icns" and not icns_is_white(asset_path.read_bytes()):
        raise AssetError("one or more ICNS representations are not opaque white")
    if kind == "bundle":
        preview, _info = bundle_preview(asset_path)
        expected = white_png(record["preview_width"], record["preview_height"])
        if preview != expected:
            raise AssetError("bundle root preview.png is not opaque white")
    if kind == "android-vector" and white_android_vector_text(asset_path.read_text(encoding="utf-8")) != asset_path.read_text(encoding="utf-8"):
        raise AssetError("Android launcher drawable is not the minimal white vector")


def check_assets(path: Path) -> int:
    manifest = load_manifest(path)
    if not manifest.get("assets"):
        raise AssetError("manifest assets are empty; run --generate-manifest first")
    check_selection(manifest)
    counts: dict[str, int] = {}
    failures: list[str] = []
    for record in manifest["assets"]:
        kind = record["kind"]
        try:
            assert_record_is_white(record)
            counts[kind] = counts.get(kind, 0) + 1
        except (AssetError, OSError, subprocess.CalledProcessError, zipfile.BadZipFile) as exc:
            failures.append(f"{record['path']}: {exc}")
    dummy = repo_path("krita/data/splash/splash_holidays_dummy.png")
    if not dummy.is_file() or dummy.stat().st_size != 0:
        failures.append("krita/data/splash/splash_holidays_dummy.png: expected intentional zero-byte dummy")
    if failures:
        raise AssetError("white asset check failed:\n  " + "\n  ".join(failures))
    details = ", ".join(f"{kind}={count}" for kind, count in sorted(counts.items()))
    print(f"check: {sum(counts.values())} assets passed ({details}); zero-byte dummy preserved")
    audit_ios_images(manifest, verify_white=False)
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group()
    action.add_argument("--generate-manifest", action="store_true", help="capture dimensions and preservation invariants")
    action.add_argument("--check", action="store_true", help="verify selection, invariants, and opaque-white content")
    action.add_argument(
        "--audit-ios-images",
        action="store_true",
        help="audit every image linked or installed by the strict iOS resource scope",
    )
    action.add_argument(
        "--audit-ios-classification",
        action="store_true",
        help="stdlib-only iOS scope/classification gate without decoding white raster pixels",
    )
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST, help="manifest path (default: %(default)s)")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    manifest_path = args.manifest.resolve()
    try:
        manifest_path.relative_to(REPO_ROOT)
        if args.generate_manifest:
            return generate_manifest(manifest_path)
        if args.check:
            return check_assets(manifest_path)
        if args.audit_ios_images:
            return audit_ios_images(load_manifest(manifest_path))
        if args.audit_ios_classification:
            return audit_ios_images(load_manifest(manifest_path), verify_white=False)
        return replace_assets(manifest_path)
    except (AssetError, OSError, UnicodeError, json.JSONDecodeError, struct.error, subprocess.CalledProcessError, zipfile.BadZipFile) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
