#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Contract of the measurement event log (scripts/perf/events_log.py).

The auditor follows events.log to notice a failed run or a stuck automatic
check right away. The line format is fixed: the ISO 8601 time to the second,
the kind, the label, the target, the run number and a detail, separated by
tabs, one line per event, the file closed after every line.
"""

from __future__ import annotations

import datetime
import importlib.util
import os
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPEC = importlib.util.spec_from_file_location(
    "perf_events", os.path.join(ROOT, "scripts", "perf", "events_log.py"))
events_log = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(events_log)

NOW = datetime.datetime(2026, 11, 11, 19, 26, 28)


class FormatTest(unittest.TestCase):
    def test_line_has_six_tab_separated_fields(self):
        text = events_log.line("target-done", "l1", "LibrePaint", 3,
                               "state=low hover=55.4 ink=61.2", now=NOW)
        self.assertEqual(text.split("\t"),
                         ["2026-11-11T19:26:28", "target-done", "l1", "LibrePaint", "3",
                          "state=low hover=55.4 ink=61.2"])

    def test_the_time_is_iso_8601_without_fractions(self):
        text = events_log.line("step", "q4-e2e", "q4-e2e-stage3", 1, "clipboard start", now=NOW)
        stamp = text.split("\t")[0]
        self.assertEqual(stamp, "2026-11-11T19:26:28")
        self.assertEqual(datetime.datetime.fromisoformat(stamp), NOW)

    def test_a_tab_or_newline_in_a_field_stays_one_line(self):
        text = events_log.line("target-failed", "l1", "T", 1, "failed: a\tb\nc")
        self.assertEqual(len(text.split("\n")), 1)
        self.assertNotIn("\t", text.split("\t")[5])
        self.assertEqual(len(text.split("\t")), 6)

    def test_unknown_kind_is_rejected(self):
        with self.assertRaises(ValueError):
            events_log.line("worker-started", "l1", "T", 1, "")

    def test_kinds_are_the_designed_set(self):
        self.assertEqual(events_log.KINDS,
                         ("run-start", "target-start", "target-done", "target-failed",
                          "target-abandoned", "step", "step-timeout", "run-done"))


class WriteTest(unittest.TestCase):
    def test_every_event_is_appended_and_the_file_is_closed(self):
        with tempfile.TemporaryDirectory() as root:
            path = os.path.join(root, "events.log")
            events_log.write(path, "run-start", "l1", "-", "-", "scenario test", now=NOW)
            events_log.write(path, "run-done", "l1", "-", "-", "0", now=NOW)
            with open(path, encoding="utf-8") as handle:
                lines = handle.read().splitlines()
        self.assertEqual(len(lines), 2)
        self.assertEqual([line.split("\t")[1] for line in lines], ["run-start", "run-done"])
        self.assertTrue(lines[0].endswith("\tscenario test"))

    def test_a_missing_directory_is_created(self):
        with tempfile.TemporaryDirectory() as root:
            path = os.path.join(root, "perf", "events.log")
            events_log.write(path, "run-start", "l1", "-", "-", "", now=NOW)
            self.assertTrue(os.path.exists(path))


if __name__ == "__main__":
    unittest.main()
