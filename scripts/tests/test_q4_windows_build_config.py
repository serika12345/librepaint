#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Windows builds the official Qt only, with one name for each Windows output."""

from __future__ import annotations

import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
WINDOWS_NIX = REPO_ROOT / "nix/windows"
FLAKE = REPO_ROOT / "flake.nix"
# Built from parts so that a repository search for the old suffix is empty.
FORK_NAME_SUFFIX = "upstream" + "-qt"


class WindowsBuildConfigTest(unittest.TestCase):
    def test_the_fork_switch_and_its_names_are_gone(self):
        texts = [FLAKE] + sorted(WINDOWS_NIX.glob("*.nix"))
        for path in texts:
            text = path.read_text(encoding="utf-8")
            with self.subTest(path=path.relative_to(REPO_ROOT).as_posix()):
                self.assertNotIn("useQtFork", text)
                self.assertNotIn("pnameSuffix", text)
                self.assertNotIn(FORK_NAME_SUFFIX, text)

    def test_windows_package_set_is_imported_once(self):
        text = FLAKE.read_text(encoding="utf-8")
        self.assertEqual(text.count("windowsPackages = import ./nix/windows"), 1)

    def test_the_angle_expression_is_removed(self):
        self.assertFalse((WINDOWS_NIX / "google-angle.nix").exists())


if __name__ == "__main__":
    unittest.main()
