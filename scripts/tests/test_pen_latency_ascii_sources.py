#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Windows measurement sources stay pure ASCII.

Windows PowerShell 5.1 reads a BOM-less script with the ANSI code page, so a
non-ASCII character corrupts the quoting and the parser rejects the file.
C# and C++ sources are compiled on the Surface too, so they follow the same
rule.
"""

from __future__ import annotations

import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
PEN_LATENCY_DIR = REPO_ROOT / "scripts/perf/pen-latency"
SUFFIXES = {".ps1", ".cs", ".cpp"}


class PenLatencyAsciiSourcesTest(unittest.TestCase):
    def test_sources_contain_only_ascii(self):
        sources = sorted(
            path
            for path in PEN_LATENCY_DIR.glob("*")
            if path.is_file() and path.suffix in SUFFIXES
        )
        self.assertTrue(sources, "no measurement sources were found")

        failures = []
        for path in sources:
            data = path.read_bytes()
            for offset, byte in enumerate(data):
                if byte >= 0x80:
                    line = data.count(b"\n", 0, offset) + 1
                    column = offset - (data.rfind(b"\n", 0, offset) + 1) + 1
                    failures.append(
                        f"{path.relative_to(REPO_ROOT)}:{line}:{column}"
                    )
                    break

        self.assertEqual(
            failures,
            [],
            "measurement sources must be pure ASCII:\n" + "\n".join(failures),
        )


if __name__ == "__main__":
    unittest.main()
