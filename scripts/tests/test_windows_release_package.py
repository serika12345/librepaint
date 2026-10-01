#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
import zipfile


SCRIPT = Path(__file__).resolve().parents[1] / "platform/audit-windows-package.py"
SPEC = importlib.util.spec_from_file_location("audit_windows_package", SCRIPT)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT}")
audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = audit
SPEC.loader.exec_module(audit)


def pe_report(*imports: str, machine: str = "pei-x86-64") -> str:
    import_lines = "\n".join(f"\tDLL Name: {name}" for name in imports)
    return f"sample.exe: file format {machine}\n{import_lines}\n"


class WindowsReleasePackageTest(unittest.TestCase):
    def inspect(self, reports: dict[str, str]) -> audit.PackageReport:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            for relative_path in reports:
                path = root / relative_path
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"PE fixture")

            def tool_result(command: list[str], **_kwargs: object) -> subprocess.CompletedProcess[str]:
                path = Path(command[-1])
                return subprocess.CompletedProcess(command, 0, reports[path.relative_to(root).as_posix()], "")

            with mock.patch.object(audit.subprocess, "run", side_effect=tool_result):
                return audit.inspect_package(root, "objdump")

    def test_all_pe_imports_resolve_case_insensitively(self) -> None:
        report = self.inspect(
            {
                "bin/LibrePaint.exe": pe_report("KERNEL32.dll", "Qt6Core.DLL"),
                "bin/qt6core.dll": pe_report("api-ms-win-core-file-l1-2-0.dll"),
                "lib/site-packages/extension.pyd": pe_report("qt6core.dll"),
            }
        )

        self.assertEqual(report.pe_count, 3)
        self.assertEqual(report.machine, "x86_64")

    def test_missing_non_system_import_reports_its_consumers(self) -> None:
        with self.assertRaisesRegex(
            audit.AuditError,
            r"libjxl[.]dll.*kritajxlexport[.]dll.*kritajxlimport[.]dll",
        ):
            self.inspect(
                {
                    "lib/kritaplugins/kritajxlimport.dll": pe_report("libjxl.dll"),
                    "lib/kritaplugins/kritajxlexport.dll": pe_report("LIBJXL.DLL"),
                }
            )

    def test_wrong_pe_architecture_is_rejected(self) -> None:
        with self.assertRaisesRegex(audit.AuditError, "pei-i386"):
            self.inspect({"bin/plugin.dll": pe_report(machine="pei-i386")})

    def test_conflicting_duplicate_dll_names_are_rejected(self) -> None:
        with self.assertRaisesRegex(audit.AuditError, "duplicate DLL basename"):
            self.inspect(
                {
                    "bin/helper.dll": pe_report(),
                    "bin/plugins/helper.dll": pe_report("KERNEL32.dll"),
                }
            )

    def test_pyqt_runtime_requires_sip_module(self) -> None:
        with self.assertRaisesRegex(audit.AuditError, "missing PyQt6 SIP runtime"):
            self.inspect(
                {
                    "bin/LibrePaint.exe": pe_report("Qt6Core.dll"),
                    "bin/Qt6Core.dll": pe_report("KERNEL32.dll"),
                    "lib/site-packages/PyQt6/QtCore.pyd": pe_report(
                        "Qt6Core.dll"
                    ),
                }
            )

    def test_development_and_test_payload_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            executable = root / "bin/LibrePaint.exe"
            executable.parent.mkdir(parents=True)
            executable.write_bytes(b"PE fixture")
            (root / "lib").mkdir()
            (root / "lib/libkrita.dll.a").write_bytes(b"import library")
            (root / "bin/plugins/qmltooling").mkdir(parents=True)
            (root / "bin/plugins/qmltooling/qmldbg_server.dll").write_bytes(
                b"PE fixture"
            )
            (root / "bin/_ctypes_test.cpython-311.dll").write_bytes(b"PE fixture")
            (root / "lib/site-packages/PyQt6").mkdir(parents=True)
            (root / "lib/site-packages/PyQt6/QtTest.pyd").write_bytes(
                b"PE fixture"
            )
            (root / "python").mkdir()
            with zipfile.ZipFile(root / "python/python311.zip", "w") as archive:
                archive.writestr("ctypes/test/test_loading.py", "")

            def tool_result(
                command: list[str], **_kwargs: object
            ) -> subprocess.CompletedProcess[str]:
                return subprocess.CompletedProcess(command, 0, pe_report(), "")

            with mock.patch.object(audit.subprocess, "run", side_effect=tool_result):
                with self.assertRaisesRegex(
                    audit.AuditError,
                    r"development or test payload.*_ctypes_test.*qmltooling.*libkrita[.]dll[.]a.*QtTest[.]pyd.*ctypes/test",
                ):
                    audit.inspect_package(root, "objdump")


if __name__ == "__main__":
    unittest.main()
