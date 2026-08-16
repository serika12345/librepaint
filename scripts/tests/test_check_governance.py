#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
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
        (self.root / "docs/architecture").mkdir(parents=True)
        (self.root / "libs/example").mkdir(parents=True)

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def write_baseline(
        self,
        legacy_files: dict[str, int] | None = None,
        exceptions: dict[str, object] | None = None,
    ) -> None:
        baseline = {
            "schemaVersion": 1,
            "threshold": 1000,
            "trackingTodo": "docs/architecture/TODO.md#r1",
            "policy": "test fixture",
            "legacyFiles": legacy_files or {},
            "exceptions": exceptions or {},
        }
        (self.root / check_governance.BASELINE_PATH).write_text(
            json.dumps(baseline) + "\n",
            encoding="utf-8",
        )

    def files(self) -> list[str]:
        return check_governance.repository_files(self.root)

    def test_legacy_source_matches_recorded_maximum(self) -> None:
        source_path = self.root / "libs/example/legacy.cpp"
        source_path.write_text("line\n" * 1001, encoding="utf-8")
        self.write_baseline({"libs/example/legacy.cpp": 1001})

        findings = check_governance.collect_findings(
            self.root,
            self.files(),
        )

        self.assertEqual([], findings)

    def test_growth_contract_reports_maximum_overrun(self) -> None:
        source_path = self.root / "libs/example/legacy.cpp"
        source_path.write_text("line\n" * 1002, encoding="utf-8")
        self.write_baseline({"libs/example/legacy.cpp": 1001})

        findings = check_governance.collect_findings(
            self.root,
            self.files(),
        )

        self.assertTrue(any("source-size maximum is 1001" in finding for finding in findings))

    def test_large_source_contract_requires_accounting(self) -> None:
        source_path = self.root / "libs/example/new.cpp"
        source_path.write_text("line\n" * 1001, encoding="utf-8")
        self.write_baseline()

        findings = check_governance.collect_findings(
            self.root,
            self.files(),
        )

        self.assertTrue(any("require a baseline entry" in finding for finding in findings))

    def test_text_contract_reports_directional_override(self) -> None:
        (self.root / "libs/example/source.cpp").write_text(
            "int safe\u202e_name = 1;\n",
            encoding="utf-8",
        )
        self.write_baseline()

        findings = check_governance.collect_findings(
            self.root,
            self.files(),
        )

        self.assertTrue(any("U+202E" in finding for finding in findings))


if __name__ == "__main__":
    unittest.main()
