#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Q4 stage 1: the source tree no longer references the Qt fork API."""

from __future__ import annotations

import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]

# Documents describe the removal, and the tests themselves spell the names out.
ALLOWED_PREFIXES = (
    "build/",
    "docs/",
    "scripts/tests/",
)

# Build trees, caches and vendored checkouts are not part of the sources.
SKIPPED_DIRECTORIES = {
    ".cache",
    ".direnv",
    ".git",
    "build",
    "result",
    "result-linux-release",
    "result-windows-release",
}

SCAN_ROOTS = (
    "cmake",
    "krita",
    "libs",
    "nix",
    "packaging",
    "plugins",
    "scripts",
)
ROOT_FILES = ("CMakeLists.txt", "CMakePresets.json", "flake.nix")

SOURCE_SUFFIXES = {
    ".c",
    ".cc",
    ".cmake",
    ".cpp",
    ".h",
    ".hpp",
    ".nix",
    ".py",
    ".txt",
    ".ui",
}

FORBIDDEN_IDENTIFIERS = (
    "KRITA_QT_HAS_",
    "HAVE_HDR",
    "setTextureColorSpace",
    "AlwaysShowSubwindowNameInTitleBar",
    "QT5_HAS_WINTAB_SWITCH",
    "KRITA_QSCROLLER_PATCH",
    "isLastFrameCompletedOnGPU",
)


def source_files():
    for name in ROOT_FILES:
        path = REPO_ROOT / name
        if path.is_file():
            yield Path(name), path

    for root_name in SCAN_ROOTS:
        root = REPO_ROOT / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            relative = path.relative_to(REPO_ROOT)
            if any(part in SKIPPED_DIRECTORIES for part in relative.parts):
                continue
            if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
                continue
            if any(str(relative).startswith(prefix) for prefix in ALLOWED_PREFIXES):
                continue
            yield relative, path


class Q4NoForkApiTest(unittest.TestCase):
    def test_sources_do_not_reference_the_fork_api(self):
        scanned = 0
        failures = []
        for relative, path in source_files():
            scanned += 1
            text = path.read_text(encoding="utf-8", errors="replace")
            for identifier in FORBIDDEN_IDENTIFIERS:
                if identifier in text:
                    failures.append(f"{relative}: {identifier}")

        self.assertGreater(scanned, 500, "the source scan found too few files")
        self.assertEqual(
            failures,
            [],
            "Q4 stage 1 must not reference the Qt fork API:\n"
            + "\n".join(failures),
        )

    def test_config_headers_are_gone(self):
        for relative in (
            "cmake/config/config-qt-patches-present.h.cmake",
            "cmake/config/config-hdr.h.cmake",
            "cmake/config/config-qmdiarea-always-show-subwindow-title.h.cmake",
            "cmake/config/config_qt5_has_wintab_switch.h.cmake",
            "plugins/platforms/wayland/config-qt-wayland-patches-present.h.cmake",
        ):
            self.assertFalse(
                (REPO_ROOT / relative).exists(),
                f"{relative} still exists",
            )


if __name__ == "__main__":
    unittest.main()
