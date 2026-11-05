#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from importlib.machinery import SourceFileLoader
from importlib.util import module_from_spec, spec_from_loader
from pathlib import Path
import unittest


loader = SourceFileLoader("agent_loop_coverage", str(Path(__file__).resolve().parents[1] / "agent-loop/coverage"))
spec = spec_from_loader(loader.name, loader)
coverage = module_from_spec(spec)
loader.exec_module(coverage)


class AgentLoopCoverageTest(unittest.TestCase):
    def test_completed_operations_count_cpu_scope_and_resident_gaps_separately(self):
        report = {"schema": 1, "rows": [
            {"id": "brush/marker", "required": "gpu", "result": "gpu", "detail": ""},
            {"id": "filter/posterize", "required": "cpu", "result": "cpu", "detail": ""},
            {"id": "brush/smudge", "required": "gpu", "result": "cpu", "detail": ""},
            {"id": "animation/start", "required": "cpu", "result": "failure", "detail": "missing | migration\nstep"},
        ]}
        self.assertEqual(coverage.summarize(report), {"gpu": 1, "cpu": 2, "failure": 1, "resident_cpu": 1})
        table = coverage.render(report)
        self.assertIn("GPU: 1 / CPU経由: 2 / 失敗: 1", table)
        self.assertIn("missing \\| migration step", table)

    def test_incomplete_unknown_and_duplicate_reports_are_rejected(self):
        row = {"id": "selection/rectangle", "required": "gpu", "result": "gpu", "detail": ""}
        invalid = [
            {}, {"schema": 1, "rows": []}, {"schema": 2, "rows": [row]},
            {"schema": 1, "rows": [row, row]},
            {"schema": 1, "rows": [row | {"result": "skipped"}]},
            {"schema": 1, "rows": [row | {"required": "unknown"}]},
        ]
        for report in invalid:
            with self.subTest(report=report), self.assertRaises(ValueError):
                coverage.summarize(report)


if __name__ == "__main__":
    unittest.main()
