#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "platform/clean-appimage-rpaths.py"
ELF_MAGIC = b"\x7fELF"
HASH = "a" * 32
OTHER_HASH = "b" * 32
ERASED_HASH = "e" * 32
OCIO_ITEM = f"{HASH}-opencolorio-2.5.2"
OCIO_LIB = f"lib/libOpenColorIO.so.2.5.2"
EXTERNAL_RPATH = f"/nix/store/{OTHER_HASH}-expat-2.9.0/lib"
MALFORMED_PREFIX = "$ORIGIN/..//nix/store/"

FAKE_PATCHELF = """#!INTERPRETER
import os
import sys


def rpath_file(path):
    return path + ".rpath"


def main():
    arguments = sys.argv[1:]
    with open(os.environ["FAKE_PATCHELF_LOG"], "a") as log:
        log.write(" ".join(arguments) + "\\n")
    command = arguments[0]
    if command == "--print-rpath":
        path = arguments[1]
        try:
            with open(path, "rb") as stream:
                magic = stream.read(4)
        except OSError as error:
            print(str(error), file=sys.stderr)
            return 1
        if magic != b"\\x7fELF":
            print("not an ELF file: " + path, file=sys.stderr)
            return 1
        try:
            with open(rpath_file(path)) as stream:
                print(stream.read().rstrip("\\n"))
        except FileNotFoundError:
            print("")
        return 0
    if command in ("--set-rpath", "--remove-rpath"):
        path = arguments[-1]
        if not os.access(path, os.W_OK):
            print("cannot write " + path, file=sys.stderr)
            return 1
        try:
            if command == "--set-rpath":
                with open(rpath_file(path), "w") as stream:
                    stream.write(arguments[1] + "\\n")
            else:
                try:
                    os.remove(rpath_file(path))
                except FileNotFoundError:
                    pass
        except OSError as error:
            print(str(error), file=sys.stderr)
            return 1
        return 0
    print("unsupported option: " + command, file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
"""


class CleanAppImageRpathsTest(unittest.TestCase):
    def setUp(self):
        self._temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self._temporary.cleanup)
        self.base = Path(self._temporary.name)
        self.root = self.base / "root"
        self.store = self.root / "nix/store"
        self.store.mkdir(parents=True)
        self.bin = self.base / "bin"
        self.bin.mkdir()
        self.fake = self.bin / "patchelf"
        self.fake.write_text(FAKE_PATCHELF.replace("INTERPRETER", sys.executable))
        self.fake.chmod(0o755)
        self.log = self.base / "patchelf.log"
        self.log.write_text("")

    def elf(self, path: Path, mode: int = 0o644) -> Path:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(ELF_MAGIC + b"\x02\x01\x01" + b"\x00" * 56)
        path.chmod(mode)
        return path

    def set_rpath(self, path: Path, value: str) -> None:
        Path(f"{path}.rpath").write_text(value + "\n")

    def current_rpath(self, path: Path):
        sidecar = Path(f"{path}.rpath")
        return sidecar.read_text().rstrip("\n") if sidecar.exists() else None

    def calls(self) -> list[str]:
        return [line for line in self.log.read_text().splitlines() if line]

    def run_script(self) -> subprocess.CompletedProcess:
        environment = dict(os.environ)
        environment["PATH"] = f"{self.bin}:{environment['PATH']}"
        environment["FAKE_PATCHELF_LOG"] = str(self.log)
        return subprocess.run(
            [sys.executable, str(SCRIPT), str(self.root)],
            capture_output=True,
            text=True,
            env=environment,
        )

    def test_library_rpath_is_absolutised(self):
        library = self.elf(self.store / OCIO_ITEM / OCIO_LIB)
        self.set_rpath(
            library, f"{MALFORMED_PREFIX}{OCIO_ITEM}/lib:$ORIGIN:{EXTERNAL_RPATH}"
        )
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            self.current_rpath(library),
            f"/nix/store/{OCIO_ITEM}/lib:$ORIGIN:{EXTERNAL_RPATH}",
        )

    def test_executable_rpath_is_absolutised_and_mode_restored(self):
        executable = self.elf(self.store / OCIO_ITEM / "bin/ocioexe", 0o555)
        self.set_rpath(executable, f"{MALFORMED_PREFIX}{OCIO_ITEM}/lib")
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            self.current_rpath(executable), f"/nix/store/{OCIO_ITEM}/lib"
        )
        self.assertEqual(
            stat.S_IMODE(executable.stat().st_mode), 0o555
        )

    def test_later_duplicate_entry_is_removed(self):
        library = self.elf(self.store / OCIO_ITEM / OCIO_LIB)
        absolute = f"/nix/store/{OCIO_ITEM}/lib"
        self.set_rpath(library, f"{MALFORMED_PREFIX}{OCIO_ITEM}/lib:{absolute}:$ORIGIN")
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.current_rpath(library), f"{absolute}:$ORIGIN")

    def test_valid_rpath_is_left_untouched(self):
        executable = self.elf(self.store / OCIO_ITEM / "bin/ocioexe", 0o550)
        self.set_rpath(executable, f"$ORIGIN:{EXTERNAL_RPATH}")
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn("--set-rpath", " ".join(self.calls()))
        self.assertEqual(self.current_rpath(executable), f"$ORIGIN:{EXTERNAL_RPATH}")
        self.assertEqual(stat.S_IMODE(executable.stat().st_mode), 0o550)

    def test_erased_markers_are_dropped(self):
        library = self.elf(self.store / f"{HASH}-libidn2-2.3.8/lib/libidn2.so.0.4.0")
        self.set_rpath(
            library,
            f"/nix/store/{ERASED_HASH}-libidn2-2.3.8/lib:{EXTERNAL_RPATH}",
        )
        self.elf(self.store / OCIO_ITEM / OCIO_LIB)
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.current_rpath(library), EXTERNAL_RPATH)

    def test_erased_markers_only_rpath_is_removed(self):
        library = self.elf(self.store / f"{HASH}-libidn2-2.3.8/lib/libidn2.so.0.4.0")
        self.set_rpath(library, f"/nix/store/{ERASED_HASH}-libidn2-2.3.8/lib:")
        self.elf(self.store / OCIO_ITEM / OCIO_LIB)
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(f"--remove-rpath {library}", self.calls())
        self.assertFalse(self.current_rpath(library))

    def test_non_elf_and_hidden_files(self):
        directory = self.store / OCIO_ITEM / "lib"
        directory.mkdir(parents=True)
        script = directory / "script.sh"
        script.write_text("#!/bin/sh\n")
        self.elf(directory / ".hidden.so")
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn(str(script), " ".join(self.calls()))
        self.assertNotIn(str(script), result.stderr)

    def test_symlink_inside_root_is_processed_once(self):
        library = self.elf(self.store / OCIO_ITEM / OCIO_LIB)
        self.set_rpath(library, f"{MALFORMED_PREFIX}{OCIO_ITEM}/lib")
        link = self.store / OCIO_ITEM / "lib/libOpenColorIO.so.2"
        link.symlink_to("libOpenColorIO.so.2.5.2")
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        calls = self.calls()
        self.assertEqual(
            sum(1 for call in calls if call.startswith("--set-rpath")), 1
        )
        self.assertEqual(
            sum(1 for call in calls if call.startswith("--print-rpath")), 1
        )
        self.assertEqual(
            self.current_rpath(library), f"/nix/store/{OCIO_ITEM}/lib"
        )

    def test_symlink_outside_root_fails(self):
        outside = self.elf(self.base / "outside.so")
        self.set_rpath(outside, f"{MALFORMED_PREFIX}{OCIO_ITEM}/lib")
        link = self.store / OCIO_ITEM / "lib/libOpenColorIO.so.2"
        link.parent.mkdir(parents=True)
        link.symlink_to(outside)
        result = self.run_script()
        self.assertEqual(result.returncode, 1)
        self.assertIn("symlink resolves outside the AppImage root", result.stderr)
        self.assertEqual(
            self.current_rpath(outside), f"{MALFORMED_PREFIX}{OCIO_ITEM}/lib"
        )

    def test_missing_opencolorio_fails(self):
        self.elf(self.store / f"{HASH}-expat-2.9.0/lib/libexpat.so.1")
        result = self.run_script()
        self.assertEqual(result.returncode, 1)
        self.assertIn("OpenColorIO is missing from the AppImage root", result.stderr)

    def test_unwritable_write_fails(self):
        library = self.elf(self.store / OCIO_ITEM / OCIO_LIB)
        self.set_rpath(library, f"{MALFORMED_PREFIX}{OCIO_ITEM}/lib")
        sidecar = Path(f"{library}.rpath")
        sidecar.chmod(0o444)
        self.addCleanup(sidecar.chmod, 0o644)
        result = self.run_script()
        self.assertEqual(result.returncode, 1)
        self.assertIn("clean-appimage-rpaths:", result.stderr)
        self.assertIn("--set-rpath", " ".join(self.calls()))


if __name__ == "__main__":
    unittest.main()
