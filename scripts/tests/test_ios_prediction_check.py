#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Contract of the iPad system prediction check (scripts/perf/ios_prediction_check.py).

The check reads a latency record written by the app on the device. It counts
the prediction events, prints them, and exits 1 when the record does not show
enough drawing, enough agreement with the OS entries, or captured points and
clears. Each case below builds a small record and reads the counts and the
`ANOMALY` lines.
"""

from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "scripts/perf/ios_prediction_check.py"


def write_record(
    path: Path,
    *,
    ios_stroke: bool = True,
    misses: int = 1,
    update_keys: int = 250,
) -> None:
    lines = ["# frequency=1000000000", "time,event,x,y,extra,thread"]
    for key in range(update_keys):
        lines.append(f"{key},prediction_update,0.00,0.00,{key},1")
    for _ in range(misses):
        lines.append("1,prediction_miss,0.00,0.00,1,1")
    source_extra = 3 if ios_stroke else 2
    lines.append(f"1,prediction_source,0.00,0.00,{source_extra},1")
    lines.append("1,prediction_os_entry,0.00,0.00,4,1")
    lines.append("1,prediction_point,0.00,0.00,0,1")
    lines.append("1,prediction_clear,0.00,0.00,1,1")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


class IosPredictionCheckTest(unittest.TestCase):
    def run_check(self, record: Path) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, str(SCRIPT), str(record)],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )

    def test_a_complete_record_passes_and_reports_the_match_rate(self):
        with tempfile.TemporaryDirectory() as directory:
            record = Path(directory) / "latency.csv"
            write_record(record)
            result = self.run_check(record)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("match_rate=0.9960", result.stdout)
        self.assertIn("ios_strokes=1", result.stdout)
        self.assertIn("clear_1=1", result.stdout)
        self.assertEqual(result.stderr, "")

    def test_many_missing_keys_report_the_match_rate(self):
        with tempfile.TemporaryDirectory() as directory:
            record = Path(directory) / "latency.csv"
            write_record(record, misses=10)
            result = self.run_check(record)

        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn("match_rate=0.9600", result.stdout)
        self.assertIn("ANOMALY match_rate", result.stderr)

    def test_a_record_without_ios_strokes_reports_them(self):
        with tempfile.TemporaryDirectory() as directory:
            record = Path(directory) / "latency.csv"
            write_record(record, ios_stroke=False)
            result = self.run_check(record)

        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn("ios_strokes=0", result.stdout)
        self.assertIn("ANOMALY ios_strokes", result.stderr)


if __name__ == "__main__":
    unittest.main()
