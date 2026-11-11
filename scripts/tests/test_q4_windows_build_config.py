#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Q4 stage 1: the Windows default uses the official Qt without ANGLE or HDR."""

from __future__ import annotations

import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
WINDOWS_DEFAULT = REPO_ROOT / "nix/windows/default.nix"
WINDOWS_KRITA = REPO_ROOT / "nix/windows/krita.nix"
FLAKE = REPO_ROOT / "flake.nix"


class Q4WindowsBuildConfigTest(unittest.TestCase):
    def test_windows_default_is_the_official_qt(self):
        text = WINDOWS_DEFAULT.read_text(encoding="utf-8")
        self.assertIn("useQtFork ? false", text)

        krita = WINDOWS_KRITA.read_text(encoding="utf-8")
        self.assertIn("useQtFork ? false", krita)

    def test_angle_and_hdr_are_not_in_the_default_build(self):
        krita = WINDOWS_KRITA.read_text(encoding="utf-8")
        self.assertNotIn("-DHAVE_HDR", krita)
        self.assertIn(
            "angle = if useQtFork then import ./google-angle.nix { inherit pkgs; } else null;",
            krita,
        )
        self.assertIn("++ lib.optional useQtFork angle", krita)

    def test_fork_package_set_is_kept_explicitly_until_stage_7(self):
        text = FLAKE.read_text(encoding="utf-8")
        fork_start = text.index("windowsPackages = import ./nix/windows {")
        fork_end = text.index("};", fork_start)
        self.assertIn("useQtFork = true;", text[fork_start:fork_end])

        upstream_start = text.index("windowsUpstreamQtPackages = import ./nix/windows {")
        upstream_end = text.index("};", upstream_start)
        upstream = text[upstream_start:upstream_end]
        self.assertIn("useQtFork = false;", upstream)
        self.assertIn('pnameSuffix = "-upstream-qt";', upstream)


if __name__ == "__main__":
    unittest.main()
