#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Normalise the ELF RPATHs of an assembled AppImage root."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import stat
import subprocess
import sys


ELF_MAGIC = b"\x7fELF"
ERASED_MARKER_PREFIX = "/nix/store/eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee-"
ERASED_MARKER_PATTERNS = (
    "*-libidn2-*/lib/libidn2.so.*.*",
    "*-libunistring-*/lib/libunistring.so.*.*",
)
ORIGIN_ABSOLUTE_PREFIX = "$ORIGIN/..//"
OPENCOLORIO_PATTERN = "*-opencolorio-*"


def is_elf(path: Path) -> bool:
    with path.open("rb") as stream:
        return stream.read(len(ELF_MAGIC)) == ELF_MAGIC


def read_rpath(path: Path) -> list[str]:
    output = subprocess.check_output(
        ["patchelf", "--print-rpath", str(path)], text=True
    )
    return [entry for entry in output.strip().split(":") if entry]


def write_rpath(path: Path, entries: list[str]) -> None:
    mode = stat.S_IMODE(path.stat().st_mode)
    path.chmod(mode | stat.S_IWUSR)
    try:
        if entries:
            command = ["patchelf", "--set-rpath", ":".join(entries), str(path)]
        else:
            command = ["patchelf", "--remove-rpath", str(path)]
        subprocess.run(command, check=True)
    finally:
        path.chmod(mode)


def drop_erased_markers(entries: list[str]) -> list[str]:
    """Drop erased markers; an RPATH of markers alone becomes empty.

    write_rpath removes an empty RPATH entirely, so the file keeps no
    empty RPATH entry.
    """
    return [
        entry
        for entry in entries
        if not entry.startswith(ERASED_MARKER_PREFIX)
    ]


def absolutise_origin_entries(entries: list[str]) -> list[str]:
    normalised: list[str | None] = list(entries)
    for index, entry in enumerate(entries):
        if normalised[index] is None:
            continue
        if not entry.startswith(ORIGIN_ABSOLUTE_PREFIX):
            continue
        absolute = "/" + entry[len(ORIGIN_ABSOLUTE_PREFIX) :]
        if any(normalised[other] == absolute for other in range(index)):
            normalised[index] = None
            continue
        normalised[index] = absolute
        for other in range(index + 1, len(entries)):
            if entries[other] == absolute:
                normalised[other] = None
    return [entry for entry in normalised if entry is not None]


def expand(store: Path, pattern: str) -> list[Path]:
    candidates: list[Path] = []
    for matched in sorted(store.glob(pattern)):
        if matched.is_dir() and not matched.is_symlink():
            for directory, directories, files in os.walk(matched):
                for name in directories + files:
                    candidates.append(Path(directory) / name)
        else:
            candidates.append(matched)
    return candidates


def elf_target(candidate: Path, root: Path) -> Path | None:
    if candidate.is_symlink():
        target = candidate.resolve()
        if not target.is_file() or not target.is_relative_to(root):
            raise ValueError(
                f"{candidate}: symlink resolves outside the AppImage root"
            )
        return target
    if not candidate.is_file():
        return None
    return candidate


def apply_rule(
    store: Path,
    root: Path,
    patterns: tuple[str, ...],
    transform,
    seen: set[Path],
) -> list[tuple[Path, list[str]]]:
    results: list[tuple[Path, list[str]]] = []
    for pattern in patterns:
        for candidate in expand(store, pattern):
            target = elf_target(candidate, root)
            if target is None or target in seen:
                continue
            seen.add(target)
            if not is_elf(target):
                continue
            entries = read_rpath(target)
            updated = transform(entries)
            if updated != entries:
                write_rpath(target, updated)
            results.append((target, updated))
    return results


def clean(root: Path) -> None:
    store = root / "nix/store"
    seen: set[Path] = set()
    apply_rule(store, root, ERASED_MARKER_PATTERNS, drop_erased_markers, seen)
    opencolorio = apply_rule(
        store, root, (OPENCOLORIO_PATTERN,), absolutise_origin_entries, seen
    )
    if not opencolorio:
        raise ValueError("OpenColorIO is missing from the AppImage root")
    for path, entries in opencolorio:
        for entry in entries:
            if entry.startswith(ORIGIN_ABSOLUTE_PREFIX):
                raise ValueError(
                    f"{path}: malformed RPATH entry remains: {entry}"
                )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Normalise the ELF RPATHs of an assembled AppImage root"
    )
    parser.add_argument("root", type=Path)
    options = parser.parse_args()
    try:
        clean(options.root.resolve())
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"clean-appimage-rpaths: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
