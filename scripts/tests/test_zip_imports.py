#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Contract of the packaged-DLL import check (scripts/perf/check_zip_imports.py).

The stage 4 ZIP was assembled by replacing only the DLLs that changed in
the stage 10 ZIP, so its libkritaapplicationui.dll calls
KisL1Switches::reuseGlSync(), which the packaged libkritaglobal.dll does not
export. The check must report that import and exit 1. The ZIP is a local
measurement artifact, so the test is skipped where it is absent.
"""

from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "scripts/perf/check_zip_imports.py"
STAGE4_ZIP = (
    REPO_ROOT
    / "build/windows/q4-stage4/LibrePaint-1.0.3-q4-stage4-x86_64-windows.zip"
)
MISSING_SYMBOL = "_ZN13KisL1Switches11reuseGlSyncEv"


@unittest.skipUnless(STAGE4_ZIP.is_file(), "stage 4 ZIP is not available")
class ZipImportsTest(unittest.TestCase):
    def run_check(self, package: Path) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, str(SCRIPT), str(package)],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )

    def test_stage4_zip_reports_the_missing_entry_point(self):
        result = self.run_check(STAGE4_ZIP)

        output = result.stdout + result.stderr
        self.assertEqual(result.returncode, 1, output)
        self.assertIn(MISSING_SYMBOL, output)
        self.assertIn(
            "libkritaapplicationui.dll, libkritaglobal.dll,",
            output,
        )


if __name__ == "__main__":
    unittest.main()
