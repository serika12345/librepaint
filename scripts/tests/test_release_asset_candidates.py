#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import zipfile


SCRIPT = Path(__file__).resolve().parents[1] / "platform/check-release-assets"
ASSETS = (
    "LibrePaint-v1.0.3-arm64-v8a.apk",
    "LibrePaint-v1.0.3-x86_64.apk",
    "SHA256SUMS.android",
    "LibrePaint-1.0.3-x86_64-windows.zip",
    "LibrePaint-1.0.3-x86_64.AppImage",
    "LibrePaint-iOS-v1.0.3-unsigned.ipa",
    "LibrePaint-1.0.3-aarch64-macos.dmg",
)


class ReleaseAssetCandidatesTest(unittest.TestCase):
    def inspect(self, directory: Path) -> subprocess.CompletedProcess[str]:
        environment = os.environ.copy()
        environment["ANDROID_RELEASE_BUILD_TOOLS"] = str(directory / "tools")
        return subprocess.run(
            ["bash", str(SCRIPT), "v1.0.3", "1000003", "00", str(directory)],
            env=environment,
            capture_output=True,
            text=True,
            check=False,
        )

    def populate(self, directory: Path) -> None:
        for name in ASSETS:
            (directory / name).write_bytes(b"candidate fixture")
        (directory / "RELEASE_NOTES.md").write_text("candidate release notes\n")

    def test_missing_candidate_reports_the_required_asset(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            result = self.inspect(directory)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("release asset is missing: " + ASSETS[0], result.stderr)

    def test_unapproved_candidate_is_rejected_before_binary_inspection(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.populate(directory)
            (directory / "internal.apk").write_bytes(b"internal fixture")
            result = self.inspect(directory)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("release asset inventory differs", result.stderr)

    def test_incomplete_local_zip_uses_the_release_integrity_check(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.populate(directory)
            with zipfile.ZipFile(directory / ASSETS[3], "w") as archive:
                archive.writestr("readme.txt", "incomplete Windows candidate")
            result = self.inspect(directory)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Windows release ZIP is incomplete or damaged", result.stderr)


if __name__ == "__main__":
    unittest.main()
