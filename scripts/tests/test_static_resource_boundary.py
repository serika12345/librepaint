#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


SCRIPT = (
    Path(__file__).resolve().parents[2]
    / "packaging/ios/scripts/audit-static-dependency-resources.py"
)
SPEC = importlib.util.spec_from_file_location("static_resource_boundary", SCRIPT)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT}")
boundary = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = boundary
SPEC.loader.exec_module(boundary)


class StaticResourceBoundaryTest(unittest.TestCase):
    def inspect(self, symbols, returncode=0):
        manifest = {
            "final_binary": {
                "expected_qinit_resources": ["icons"],
                "expected_qcleanup_resources": ["icons"],
                "forbidden_resource_names": ["excluded"],
                "forbidden_symbol_substrings": ["KCharSelect"],
            }
        }
        with tempfile.TemporaryDirectory() as directory:
            binary = Path(directory) / "LibrePaint"
            binary.touch()
            result = subprocess.CompletedProcess(
                [], returncode, symbols, "unreadable binary" if returncode else ""
            )
            with mock.patch.object(boundary.subprocess, "run", return_value=result):
                boundary.audit_binary(manifest, binary, "nm")

    def test_selected_resources_survive_linking(self):
        self.inspect("__Z20qInitResources_iconsv\n__Z23qCleanupResources_iconsv\n")

    def test_missing_extra_or_duplicate_resources_are_rejected(self):
        valid = "__Z20qInitResources_iconsv\n__Z23qCleanupResources_iconsv\n"
        for symbols in (
            "",
            valid.splitlines()[0],
            valid + "__Z20qInitResources_iconsv\n",
            valid + "__Z23qInitResources_excludedv\n",
        ):
            with self.subTest(symbols=symbols):
                with self.assertRaisesRegex(boundary.AuditError, "exact set changed"):
                    self.inspect(symbols)

    def test_excluded_implementation_is_rejected(self):
        with self.assertRaisesRegex(boundary.AuditError, "forbidden implementation"):
            self.inspect("qInitResources_iconsv\nqCleanupResources_iconsv\nKCharSelect\n")

    def test_failed_binary_inspection_is_rejected(self):
        with self.assertRaisesRegex(boundary.AuditError, "nm failed"):
            self.inspect("", returncode=1)


if __name__ == "__main__":
    unittest.main()
