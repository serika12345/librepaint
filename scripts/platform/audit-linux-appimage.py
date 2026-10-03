#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import argparse
from collections import defaultdict, deque
from dataclasses import dataclass
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


NIX_HASH = rb"[0-9abcdfghijklmnpqrsvwxyz]{32}"
STORE_REFERENCE_PATTERN = re.compile(rb"/nix/store/(" + NIX_HASH + rb")-")
NEEDED_PATTERN = re.compile(r"Shared library: \[([^]]+)\]")
RPATH_PATTERN = re.compile(r"Library (?:rpath|runpath): \[([^]]*)\]")
INTERPRETER_PATTERN = re.compile(r"Requesting program interpreter: ([^]]+)\]")
FORBIDDEN_SUFFIXES = (
    ".a",
    ".cmake",
    ".gir",
    ".la",
    ".pc",
    ".pri",
    ".prl",
    ".vapi",
)
FORBIDDEN_PARTS = frozenset(
    {
        "aclocal",
        "cmake",
        "gdb",
        "mkspecs",
        "pkgconfig",
        "systemtap",
        "vala",
    }
)
FORBIDDEN_EXECUTABLE_PREFIXES = (
    "ocio",
    "protoc",
)
ALLOWED_HOST_RPATHS = frozenset(
    {
        "/run/opengl-driver/lib",
        "/run/opengl-driver-32/lib",
    }
)
FORBIDDEN_STORE_ITEMS = {
    "Qt WebEngine": "-qtwebengine-",
}
FORBIDDEN_EXECUTABLES = frozenset(
    {
        "assistant",
        "designer",
        "krita_version",
        "kritarunner",
        "lconvert",
        "linguist",
        "lrelease",
        "lupdate",
        "qmlcachegen",
        "qmldom",
        "qmllint",
        "qmlplugindump",
        "qmlprofiler",
        "qmlscene",
        "qmltestrunner",
        "qtdiag",
    }
)
REQUIRED_FEATURES = {
    "LibrePaint executable": "nix/store/*-librepaint-linux-unwrapped-*/bin/krita",
    "Pixel and clone brush engines": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritadefaultpaintops.so",
    "Basic canvas tools": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritadefaulttools.so",
    "Krita Python plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritapykrita.so",
    "Krita Python module": "nix/store/*-librepaint-linux-unwrapped-*/lib/krita-python-libs/krita/__init__.py",
    "KRA import plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritakraimport.so",
    "KRA export plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritakraexport.so",
    "PNG import plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritapngimport.so",
    "PNG export plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritapngexport.so",
    "JPEG import plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritajpegimport.so",
    "JPEG export plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritajpegexport.so",
    "OpenEXR import plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritaexrimport.so",
    "OpenEXR export plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritaexrexport.so",
    "TIFF import plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritatiffimport.so",
    "TIFF export plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritatiffexport.so",
    "PSD import plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritapsdimport.so",
    "PSD export plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritapsdexport.so",
    "SVG import plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritasvgimport.so",
    "WebP import plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritawebpimport.so",
    "WebP export plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritawebpexport.so",
    "HEIF import plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritaheifimport.so",
    "HEIF export plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritaheifexport.so",
    "JPEG XL import plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritajxlimport.so",
    "JPEG XL export plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritajxlexport.so",
    "OpenRaster import plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritaoraimport.so",
    "OpenRaster export plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritaoraexport.so",
    "XCF import plugin": "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/kritaxcfimport.so",
    "G'MIC plugin": "nix/store/*/lib/kritaplugins/krita_gmic_qt.so",
    "G'MIC runtime data": "nix/store/*/share/gmic/gmic_cluts.gmz",
    "PyQt6 runtime": "nix/store/*/lib/python*/site-packages/PyQt6/QtCore.abi3.so",
    "PyQt6 SIP runtime": "nix/store/*/lib/python*/site-packages/PyQt6/sip*.so",
    "FFmpeg": "nix/store/*/bin/ffmpeg",
    "FFprobe": "nix/store/*/bin/ffprobe",
    "MLT runtime": "nix/store/*/lib/libmlt*.so*",
    "MLT FFmpeg module": "nix/store/*/lib/mlt-*/libmltavformat.so",
    "Qt XCB platform plugin": "nix/store/*/lib/qt-6/plugins/platforms/libqxcb.so",
    "Qt Wayland platform plugin": "nix/store/*/lib/qt-6/plugins/platforms/libqwayland.so",
    "Breeze icon theme": "nix/store/*/share/icons/breeze/index.theme",
    "fontconfig configuration": "nix/store/*/etc/fonts/fonts.conf",
    "Krita translation catalog": "nix/store/*-librepaint-linux-unwrapped-*/share/locale/*/LC_MESSAGES/krita.mo",
}


class AuditError(RuntimeError):
    pass


@dataclass(frozen=True)
class ElfRecord:
    path: Path
    needed: tuple[str, ...]
    rpaths: tuple[str, ...]
    interpreter: str | None


@dataclass(frozen=True)
class PackageReport:
    store_items: int
    files: int
    symlinks: int
    elf_files: int
    krita_plugins: int
    unpacked_bytes: int


def _relative(root: Path, path: Path) -> str:
    return path.relative_to(root).as_posix()


def _store_target(root: Path, target: str) -> Path:
    return root / target.removeprefix("/")


def _packaged_search_directory(root: Path, origin: Path, value: str) -> Path | None:
    expanded = value.replace("${ORIGIN}", str(origin)).replace("$ORIGIN", str(origin))
    root_text = str(root)
    if expanded == root_text or expanded.startswith(root_text + os.sep):
        candidate = Path(expanded)
    elif expanded.startswith("/"):
        candidate = _store_target(root, expanded)
    else:
        return None
    candidate = Path(os.path.normpath(candidate))
    return candidate if candidate == root or root in candidate.parents else None


def _packaged_interpreter(root: Path, value: str) -> Path | None:
    if not value.startswith("/"):
        return None
    try:
        candidate = _store_target(root, value).resolve()
    except (OSError, RuntimeError):
        return None
    return candidate if root in candidate.parents and candidate.is_file() else None


def _read_elf_header(root: Path, path: Path) -> tuple[int, int] | None:
    try:
        with path.open("rb") as stream:
            header = stream.read(20)
    except OSError as error:
        raise AuditError(f"cannot read {_relative(root, path)}: {error}") from error
    if not header.startswith(b"\x7fELF"):
        return None
    if len(header) < 20 or header[4] != 2 or header[5] != 1:
        return (-1, -1)
    return (int.from_bytes(header[16:18], "little"), int.from_bytes(header[18:20], "little"))


def _inspect_elf(root: Path, path: Path, readelf: str) -> ElfRecord:
    result = subprocess.run(
        [readelf, "--program-headers", "--dynamic", str(path)],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        diagnostic = result.stderr.strip() or result.stdout.strip()
        raise AuditError(f"readelf could not inspect {_relative(root, path)}: {diagnostic}")
    # glibc ignores an empty tag; empty colon-separated entries search the cwd.
    rpaths = tuple(
        entry
        for match in RPATH_PATTERN.findall(result.stdout)
        if match
        for entry in match.split(":")
    )
    interpreter_match = INTERPRETER_PATTERN.search(result.stdout)
    return ElfRecord(
        path=path,
        needed=tuple(NEEDED_PATTERN.findall(result.stdout)),
        rpaths=rpaths,
        interpreter=interpreter_match.group(1) if interpreter_match else None,
    )


def _development_payload(root: Path, files: tuple[Path, ...]) -> tuple[str, ...]:
    rejected: list[str] = []
    for path in files:
        relative = path.relative_to(root)
        lowered_parts = tuple(part.casefold() for part in relative.parts)
        if any(part in FORBIDDEN_PARTS for part in lowered_parts):
            rejected.append(relative.as_posix())
            continue
        payload_parts = lowered_parts[3:]
        if payload_parts and (
            payload_parts[0] == "include"
            or (payload_parts[0] == "lib" and "include" in payload_parts[1:])
        ):
            rejected.append(relative.as_posix())
            continue
        if path.name.casefold().endswith(FORBIDDEN_SUFFIXES):
            rejected.append(relative.as_posix())
            continue
        if "bin" in lowered_parts and (
            path.name.casefold() in FORBIDDEN_EXECUTABLES
            or path.name.casefold().startswith(FORBIDDEN_EXECUTABLE_PREFIXES)
        ):
            rejected.append(relative.as_posix())
    return tuple(rejected)


def _symlink_resolves_in_package(root: Path, path: Path) -> bool:
    current = path
    seen: set[Path] = set()
    while current.is_symlink():
        if current in seen:
            return False
        seen.add(current)
        target = os.readlink(current)
        if target.startswith("/etc/") or target == "/etc/environment":
            return True
        current = (
            _store_target(root, target)
            if target.startswith("/")
            else Path(os.path.normpath(current.parent / target))
        )
        if current == root / "etc" or root / "etc" in current.parents:
            return True
        if current != root and root not in current.parents:
            return False
    return current.exists()


def _broken_symlinks(root: Path) -> tuple[str, ...]:
    return tuple(
        f"{_relative(root, path)} -> {os.readlink(path)}"
        for path in root.rglob("*")
        if path.is_symlink() and not _symlink_resolves_in_package(root, path)
    )


def _referenced_store_hashes(path: Path) -> set[bytes]:
    references: set[bytes] = set()
    overlap = b""
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            data = overlap + chunk
            references.update(STORE_REFERENCE_PATTERN.findall(data))
            overlap = data[-80:]
    return references


def _store_item_hash(root: Path, path: Path) -> bytes | None:
    relative = path.relative_to(root)
    if len(relative.parts) < 3 or relative.parts[:2] != ("nix", "store"):
        return None
    return relative.parts[2][:32].encode()


def _store_references(
    root: Path, files: tuple[Path, ...], store_hashes: frozenset[bytes]
) -> tuple[tuple[str, ...], dict[bytes, set[bytes]]]:
    broken: dict[bytes, Path] = {}
    graph: dict[bytes, set[bytes]] = defaultdict(set)
    seen_inodes: set[tuple[int, int]] = set()
    for path in files:
        stat = path.stat()
        inode = (stat.st_dev, stat.st_ino)
        if inode in seen_inodes:
            continue
        seen_inodes.add(inode)
        source_hash = _store_item_hash(root, path)
        for reference in _referenced_store_hashes(path):
            if reference not in store_hashes:
                broken.setdefault(reference, path)
            elif source_hash is not None:
                graph[source_hash].add(reference)
    for path in root.rglob("*"):
        if not path.is_symlink():
            continue
        source_hash = _store_item_hash(root, path)
        if source_hash is None:
            continue
        target = os.readlink(path).encode()
        graph[source_hash].update(
            reference
            for reference in STORE_REFERENCE_PATTERN.findall(target)
            if reference in store_hashes
        )
    return (
        tuple(
            f"{reference.decode()}: {_relative(root, path)}"
            for reference, path in sorted(broken.items())
        ),
        graph,
    )


def inspect_root(root: Path, readelf: str) -> PackageReport:
    root = root.resolve()
    store = root / "nix/store"
    entrypoint = root / "entrypoint"
    if not store.is_dir() or not entrypoint.is_symlink():
        raise AuditError("AppImage root must contain nix/store and an entrypoint symlink")
    entrypoint_target = os.readlink(entrypoint)
    if not entrypoint_target.startswith("/nix/store/"):
        raise AuditError(f"entrypoint is outside the packaged Nix store: {entrypoint_target}")
    packaged_entrypoint = _store_target(root, entrypoint_target)
    if not packaged_entrypoint.is_file() or not os.access(packaged_entrypoint, os.X_OK):
        raise AuditError(f"entrypoint is missing or not executable: {entrypoint_target}")

    store_items = tuple(sorted(store.iterdir()))
    store_hashes = frozenset(path.name[:32].encode() for path in store_items)
    entries = tuple(sorted(root.rglob("*")))
    files = tuple(
        path for path in entries if not path.is_symlink() and path.is_file()
    )
    unique_inodes: set[tuple[int, int]] = set()
    unpacked_bytes = 0
    elf_paths: list[Path] = []
    wrong_elf: list[str] = []
    for path in files:
        stat = path.stat()
        inode = (stat.st_dev, stat.st_ino)
        if inode not in unique_inodes:
            unique_inodes.add(inode)
            unpacked_bytes += stat.st_size
        header = _read_elf_header(root, path)
        if header is None:
            continue
        elf_type, machine = header
        if machine != 62:
            wrong_elf.append(f"{_relative(root, path)}: ELF machine {machine}")
        if elf_type in (2, 3):
            elf_paths.append(path)

    diagnostics: list[str] = []
    if wrong_elf:
        diagnostics.append("non-x86_64 ELF files: " + ", ".join(wrong_elf))

    forbidden_store_items = [
        f"{label}: {item.name}"
        for label, marker in FORBIDDEN_STORE_ITEMS.items()
        for item in store_items
        if marker in item.name
    ]
    if forbidden_store_items:
        diagnostics.append(
            "unused runtime store items: " + ", ".join(forbidden_store_items)
        )

    missing_features = [
        label for label, pattern in REQUIRED_FEATURES.items() if not any(root.glob(pattern))
    ]
    if missing_features:
        diagnostics.append("missing runtime features: " + ", ".join(missing_features))

    plugins = tuple(
        root.glob(
            "nix/store/*-librepaint-linux-unwrapped-*/lib/kritaplugins/*.so"
        )
    )

    rejected_payload = _development_payload(root, entries)
    if rejected_payload:
        preview = ", ".join(rejected_payload[:20])
        suffix = " ..." if len(rejected_payload) > 20 else ""
        diagnostics.append(
            f"development payload ({len(rejected_payload)} files): {preview}{suffix}"
        )

    broken_links = _broken_symlinks(root)
    if broken_links:
        diagnostics.append("broken symlinks: " + ", ".join(broken_links[:20]))

    broken_references, reference_graph = _store_references(
        root, files, store_hashes
    )
    if broken_references:
        diagnostics.append(
            "missing Nix store references: " + ", ".join(broken_references[:20])
        )
    entrypoint_hash = entrypoint_target.split("/", 4)[3][:32].encode()
    reachable_hashes: set[bytes] = set()
    pending = deque([entrypoint_hash])
    while pending:
        current = pending.popleft()
        if current in reachable_hashes:
            continue
        reachable_hashes.add(current)
        pending.extend(reference_graph.get(current, set()) - reachable_hashes)
    unreferenced_items = tuple(
        item.name
        for item in store_items
        if item.name[:32].encode() not in reachable_hashes
    )
    if unreferenced_items:
        diagnostics.append(
            "unreferenced Nix store items: " + ", ".join(unreferenced_items[:20])
        )

    elf_records = tuple(_inspect_elf(root, path, readelf) for path in elf_paths)
    # Nix glibc searches its own lib directory by default. Only interpreters
    # actually selected by packaged executables establish this runtime path.
    glibc_default_directories = frozenset(
        interpreter.parent
        for record in elf_records
        if record.interpreter
        and (interpreter := _packaged_interpreter(root, record.interpreter)) is not None
        and interpreter.name == "ld-linux-x86-64.so.2"
        and interpreter.parent.name == "lib"
        and interpreter.parent.parent.parent == store
        and "-glibc-" in interpreter.parent.parent.name
    )
    missing_interpreters: list[str] = []
    missing_rpaths: list[str] = []
    missing_libraries: dict[str, list[str]] = defaultdict(list)
    for record in elf_records:
        relative = _relative(root, record.path)
        search_directories = tuple(
            directory
            for rpath in record.rpaths
            if (directory := _packaged_search_directory(root, record.path.parent, rpath))
            is not None
            and directory.is_dir()
        )
        if record.interpreter:
            if _packaged_interpreter(root, record.interpreter) is None:
                missing_interpreters.append(f"{relative}: {record.interpreter}")
        unusable_rpaths: list[str] = []
        for rpath in record.rpaths:
            if rpath in ALLOWED_HOST_RPATHS:
                continue
            packaged_directory = _packaged_search_directory(
                root, record.path.parent, rpath
            )
            if packaged_directory is None or not packaged_directory.is_dir():
                unusable_rpaths.append(rpath)
        if unusable_rpaths:
            missing_rpaths.extend(
                f"{relative}: {rpath or '<empty>'}" for rpath in unusable_rpaths
            )
        for needed in record.needed:
            if needed.startswith("/"):
                resolved = _store_target(root, needed)
                found = resolved.exists() or resolved.is_symlink()
            elif "/" in needed:
                found = False
            else:
                found = any(
                    (directory / needed).exists()
                    or (directory / needed).is_symlink()
                    for directory in search_directories
                )
                if not found:
                    found = any(
                        (directory / needed).is_file()
                        for directory in glibc_default_directories
                    )
            if not found:
                missing_libraries[needed].append(relative)
    if missing_interpreters:
        diagnostics.append("missing ELF interpreters: " + ", ".join(missing_interpreters))
    if missing_rpaths:
        diagnostics.append("missing ELF RPATH directories: " + ", ".join(missing_rpaths[:20]))
    if missing_libraries:
        diagnostics.append("missing ELF dependencies:")
        diagnostics.extend(
            f"  {name}: {', '.join(consumers[:10])}"
            for name, consumers in sorted(missing_libraries.items())
        )

    if diagnostics:
        raise AuditError("\n".join(diagnostics))

    return PackageReport(
        store_items=len(store_items),
        files=len(files),
        symlinks=sum(path.is_symlink() for path in entries),
        elf_files=len(elf_records),
        krita_plugins=len(plugins),
        unpacked_bytes=unpacked_bytes,
    )


def _squashfs_offset(appimage: Path, unsquashfs: str) -> int:
    candidates: list[int] = []
    offset = 0
    overlap = b""
    with appimage.open("rb") as stream:
        while offset < 64 * 1024 * 1024 and (chunk := stream.read(1024 * 1024)):
            data = overlap + chunk
            start = 0
            while (index := data.find(b"hsqs", start)) >= 0:
                candidates.append(offset - len(overlap) + index)
                start = index + 1
            overlap = data[-3:]
            offset += len(chunk)
    for candidate in candidates:
        result = subprocess.run(
            [unsquashfs, "-o", str(candidate), "-s", str(appimage)],
            capture_output=True,
            check=False,
        )
        if result.returncode == 0:
            return candidate
    raise AuditError(f"cannot locate the AppImage SquashFS payload: {appimage}")


def inspect_appimage(appimage: Path, readelf: str, unsquashfs: str) -> PackageReport:
    if not appimage.is_file():
        raise AuditError(f"AppImage does not exist: {appimage}")
    runtime_header = _read_elf_header(appimage.parent, appimage)
    if runtime_header is None:
        raise AuditError(f"AppImage runtime is not an ELF executable: {appimage}")
    _runtime_type, runtime_machine = runtime_header
    if runtime_machine != 62:
        raise AuditError(f"AppImage runtime has ELF machine {runtime_machine}, expected 62")
    offset = _squashfs_offset(appimage, unsquashfs)
    with tempfile.TemporaryDirectory(prefix="librepaint-appimage-audit-") as directory:
        root = Path(directory) / "root"
        result = subprocess.run(
            [
                unsquashfs,
                "-no-progress",
                "-o",
                str(offset),
                "-d",
                str(root),
                str(appimage),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            diagnostic = result.stderr.strip() or result.stdout.strip()
            raise AuditError(f"cannot extract {appimage}: {diagnostic}")
        return inspect_root(root, readelf)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify the runtime closure and payload of a Linux AppImage"
    )
    parser.add_argument("package", type=Path)
    parser.add_argument("--readelf", default="readelf")
    parser.add_argument("--unsquashfs", default="unsquashfs")
    options = parser.parse_args()
    try:
        report = (
            inspect_root(options.package, options.readelf)
            if options.package.is_dir()
            else inspect_appimage(options.package, options.readelf, options.unsquashfs)
        )
    except (AuditError, OSError) as error:
        print(f"audit-linux-appimage: {error}", file=sys.stderr)
        return 1
    print(
        "Linux AppImage verified: "
        f"{report.store_items} store items, {report.files} regular files, "
        f"{report.symlinks} symlinks, "
        f"{report.elf_files} ELF files, {report.krita_plugins} Krita plugins, "
        f"{report.unpacked_bytes} unpacked bytes"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
