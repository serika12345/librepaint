#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "check_governance.py"
SPEC = importlib.util.spec_from_file_location("check_governance", SCRIPT_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_governance = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_governance
SPEC.loader.exec_module(check_governance)


class GovernanceTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        (self.root / "libs/example").mkdir(parents=True)

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def files(self) -> list[str]:
        return check_governance.repository_files(self.root)

    def test_utf8_text_is_accepted(self) -> None:
        (self.root / "libs/example/source.cpp").write_text(
            "// 日本語\nint value = 1;\n",
            encoding="utf-8",
        )

        self.assertEqual(
            [],
            check_governance.collect_findings(self.root, self.files()),
        )

    def test_text_contract_reports_directional_override(self) -> None:
        (self.root / "libs/example/source.cpp").write_text(
            "int safe\u202e_name = 1;\n",
            encoding="utf-8",
        )

        findings = check_governance.collect_findings(self.root, self.files())

        self.assertTrue(any("U+202E" in finding for finding in findings))


if __name__ == "__main__":
    unittest.main()
