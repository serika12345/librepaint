#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[1] / "platform/audit-linux-appimage.py"
SPEC = importlib.util.spec_from_file_location("audit_linux_appimage", SCRIPT)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT}")
audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = audit
SPEC.loader.exec_module(audit)

APP_HASH = "0" * 32
RUNTIME_HASH = "1" * 32
APP_ITEM = f"{APP_HASH}-librepaint-linux-unwrapped-1.0.3"
RUNTIME_ITEM = f"{RUNTIME_HASH}-runtime"


def elf_bytes(machine: int = 62) -> bytes:
    header = bytearray(64)
    header[:6] = b"\x7fELF\x02\x01"
    header[16:18] = (3).to_bytes(2, "little")
    header[18:20] = machine.to_bytes(2, "little")
    return bytes(header)


class LinuxReleaseAppImageTest(unittest.TestCase):
    def make_package(self, directory: str) -> Path:
        root = Path(directory)
        app = root / "nix/store" / APP_ITEM
        runtime = root / "nix/store" / RUNTIME_ITEM
        executable = app / "bin/krita"
        executable.parent.mkdir(parents=True)
        executable.write_bytes(
            elf_bytes() + f"/nix/store/{RUNTIME_ITEM}/lib".encode()
        )
        executable.chmod(0o755)
        (root / "entrypoint").symlink_to(f"/nix/store/{APP_ITEM}/bin/krita")

        plugin_dir = app / "lib/kritaplugins"
        plugin_dir.mkdir(parents=True)
        for index in range(170):
            (plugin_dir / f"plugin-{index}.so").write_bytes(elf_bytes())
        for name in (
            "kritaexrexport.so",
            "kritaexrimport.so",
            "kritaheifexport.so",
            "kritaheifimport.so",
            "kritajpegexport.so",
            "kritajpegimport.so",
            "kritajxlexport.so",
            "kritajxlimport.so",
            "kritakraexport.so",
            "kritakraimport.so",
            "kritaoraexport.so",
            "kritaoraimport.so",
            "kritapngexport.so",
            "kritapngimport.so",
            "kritapsdexport.so",
            "kritapsdimport.so",
            "kritapykrita.so",
            "kritasvgimport.so",
            "kritatiffexport.so",
            "kritatiffimport.so",
            "kritawebpexport.so",
            "kritawebpimport.so",
            "kritaxcfimport.so",
        ):
            (plugin_dir / name).write_bytes(elf_bytes())
        (app / "lib/krita-python-libs/krita").mkdir(parents=True)
        (app / "lib/krita-python-libs/krita/__init__.py").write_text("# fixture\n")
        (app / "share/locale/en/LC_MESSAGES").mkdir(parents=True)
        (app / "share/locale/en/LC_MESSAGES/krita.mo").write_bytes(b"fixture")
        (runtime / "lib/kritaplugins").mkdir(parents=True)
        (runtime / "lib/kritaplugins/krita_gmic_qt.so").write_bytes(elf_bytes())
        (runtime / "share/gmic").mkdir(parents=True)
        (runtime / "share/gmic/gmic_cluts.gmz").write_bytes(b"fixture")
        (runtime / "lib/python3.14/site-packages/PyQt6").mkdir(parents=True)
        (runtime / "lib/python3.14/site-packages/PyQt6/QtCore.abi3.so").write_bytes(
            elf_bytes()
        )
        (runtime / "bin").mkdir(parents=True)
        for name in ("ffmpeg", "ffprobe"):
            path = runtime / "bin" / name
            path.write_bytes(elf_bytes())
            path.chmod(0o755)
        (runtime / "lib/libmlt.so.7").write_bytes(elf_bytes())
        (runtime / "lib/mlt-7").mkdir()
        (runtime / "lib/mlt-7/libmltavformat.so").write_bytes(elf_bytes())
        (runtime / "lib/qt-6/plugins/platforms").mkdir(parents=True)
        (runtime / "lib/qt-6/plugins/platforms/libqxcb.so").write_bytes(elf_bytes())
        (runtime / "lib/qt-6/plugins/platforms/libqwayland.so").write_bytes(elf_bytes())
        (runtime / "share/icons/breeze").mkdir(parents=True)
        (runtime / "share/icons/breeze/index.theme").write_text("[Icon Theme]\n")
        (runtime / "etc/fonts").mkdir(parents=True)
        (runtime / "etc/fonts/fonts.conf").write_text("<fontconfig/>\n")
        return root

    def inspect(
        self,
        root: Path,
        needed: str | None = None,
        interpreter: str | None = None,
        rpath: str | None = None,
    ) -> audit.PackageReport:
        def tool_result(command: list[str], **_kwargs: object) -> subprocess.CompletedProcess[str]:
            dynamic = f" (NEEDED) Shared library: [{needed}]\n" if needed else ""
            if interpreter:
                dynamic += f" Requesting program interpreter: {interpreter}]\n"
            if rpath:
                dynamic += f" Library runpath: [{rpath}]\n"
            return subprocess.CompletedProcess(command, 0, dynamic, "")

        with mock.patch.object(audit.subprocess, "run", side_effect=tool_result):
            return audit.inspect_root(root, "readelf")

    def test_non_x86_64_appimage_runtime_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            appimage = Path(directory) / "LibrePaint.AppImage"
            appimage.write_bytes(elf_bytes(machine=183))
            with self.assertRaisesRegex(audit.AuditError, "ELF machine 183"):
                audit.inspect_appimage(appimage, "readelf", "unsquashfs")

    def test_complete_runtime_is_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            report = self.inspect(self.make_package(directory))
        self.assertEqual(report.store_items, 2)
        self.assertGreaterEqual(report.krita_plugins, 170)

    def test_missing_store_reference_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            missing_hash = "2" * 32
            (root / "reference.txt").write_text(
                f"/nix/store/{missing_hash}-missing/share/data"
            )
            with self.assertRaisesRegex(audit.AuditError, "missing Nix store references"):
                self.inspect(root)

    def test_unreferenced_store_item_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            unused = root / "nix/store/33333333333333333333333333333333-unused"
            unused.mkdir()
            (unused / "data").write_text("unused")
            with self.assertRaisesRegex(audit.AuditError, "unreferenced Nix store items"):
                self.inspect(root)

    def test_store_file_can_connect_runtime_data(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            config_hash = "6" * 32
            data_hash = "7" * 32
            config_item = f"{config_hash}-runtime.conf"
            data_item = f"{data_hash}-runtime-data"
            (root / "nix/store" / config_item).write_text(
                f"data=/nix/store/{data_item}/data\n"
            )
            data = root / "nix/store" / data_item
            data.mkdir()
            (data / "data").write_text("runtime data\n")
            executable = root / "nix/store" / APP_ITEM / "bin/krita"
            with executable.open("ab") as stream:
                stream.write(f"/nix/store/{config_item}".encode())
            report = self.inspect(root)
        self.assertEqual(report.store_items, 4)

    def test_unused_qt_webengine_runtime_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            webengine_item = "44444444444444444444444444444444-qtwebengine-6.11.1"
            webengine = root / "nix/store" / webengine_item
            webengine.mkdir()
            (webengine / "resources.pak").write_bytes(b"unused web runtime")
            executable = root / "nix/store" / APP_ITEM / "bin/krita"
            with executable.open("ab") as stream:
                stream.write(f"/nix/store/{webengine_item}/resources.pak".encode())
            with self.assertRaisesRegex(audit.AuditError, "Qt WebEngine"):
                self.inspect(root)

    def test_missing_major_image_format_plugin_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            (root / "nix/store" / APP_ITEM / "lib/kritaplugins/kritajpegimport.so").unlink()
            with self.assertRaisesRegex(audit.AuditError, "JPEG import plugin"):
                self.inspect(root)

    def test_missing_krita_python_module_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            (
                root
                / "nix/store"
                / APP_ITEM
                / "lib/krita-python-libs/krita/__init__.py"
            ).unlink()
            with self.assertRaisesRegex(audit.AuditError, "Krita Python module"):
                self.inspect(root)

    def test_unresolved_elf_dependency_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            with self.assertRaisesRegex(audit.AuditError, "libmissing[.]so"):
                self.inspect(root, needed="libmissing.so")

    def test_library_outside_elf_runpath_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            runtime = root / "nix/store" / RUNTIME_ITEM
            (runtime / "lib/libmisplaced.so").write_text("not reachable")
            unrelated = runtime / "unrelated"
            unrelated.mkdir()
            with self.assertRaisesRegex(audit.AuditError, "libmisplaced[.]so"):
                self.inspect(
                    root,
                    needed="libmisplaced.so",
                    rpath=f"/nix/store/{RUNTIME_ITEM}/unrelated",
                )

    def test_host_absolute_library_cannot_satisfy_dependency(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            with self.assertRaisesRegex(audit.AuditError, "/bin/sh"):
                self.inspect(
                    root,
                    needed="/bin/sh",
                    rpath=f"/nix/store/{RUNTIME_ITEM}/lib",
                )

    def test_missing_elf_interpreter_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            missing = "/nix/store/22222222222222222222222222222222-glibc/lib/ld-linux.so.2"
            with self.assertRaisesRegex(audit.AuditError, "missing ELF interpreters"):
                self.inspect(root, interpreter=missing)

    def test_missing_elf_runpath_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            missing = "/nix/store/22222222222222222222222222222222-missing/lib"
            with self.assertRaisesRegex(audit.AuditError, "missing ELF RPATH directories"):
                self.inspect(root, rpath=missing)

    def test_missing_origin_runpath_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            with self.assertRaisesRegex(audit.AuditError, "missing ELF RPATH directories"):
                self.inspect(root, rpath="$ORIGIN/missing")

    def test_redundant_missing_runpath_is_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            self.inspect(root, rpath="$ORIGIN:$ORIGIN/missing")

    def test_allowed_graphics_driver_runpath_is_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            self.inspect(root, rpath="/run/opengl-driver/lib")

    def test_glibc_runtime_library_is_resolved_as_already_loaded(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            glibc_item = "5" * 32 + "-glibc-2.42"
            glibc = root / "nix/store" / glibc_item / "lib"
            glibc.mkdir(parents=True)
            (glibc / "libc.so.6").write_bytes(elf_bytes())
            executable = root / "nix/store" / APP_ITEM / "bin/krita"
            with executable.open("ab") as stream:
                stream.write(f"/nix/store/{glibc_item}/lib/libc.so.6".encode())
            self.inspect(root, needed="libc.so.6", rpath="$ORIGIN")

    def test_non_x86_64_elf_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            (root / "nix/store" / RUNTIME_ITEM / "lib/wrong.so").write_bytes(
                elf_bytes(machine=183)
            )
            with self.assertRaisesRegex(audit.AuditError, "ELF machine 183"):
                self.inspect(root)

    def test_development_payload_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            header = root / "nix/store" / RUNTIME_ITEM / "include/library.h"
            header.parent.mkdir()
            header.write_text("development header")
            with self.assertRaisesRegex(audit.AuditError, "development payload"):
                self.inspect(root)

    def test_development_payload_symlink_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            include = root / "nix/store" / RUNTIME_ITEM / "include"
            include.mkdir()
            (include / "host-header.h").symlink_to("/etc/hosts")
            with self.assertRaisesRegex(audit.AuditError, "development payload"):
                self.inspect(root)

    def test_runtime_data_directory_named_include_is_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            keymap = (
                root
                / "nix/store"
                / RUNTIME_ITEM
                / "share/keymaps/i386/include/compose.inc"
            )
            keymap.parent.mkdir(parents=True)
            keymap.write_text("runtime keymap data\n")
            self.inspect(root)

    def test_build_metadata_is_rejected(self) -> None:
        for relative in (
            "lib/libfixture.prl",
            "share/gir-1.0/Fixture.gir",
            "share/vala/vapi/fixture.vapi",
        ):
            with self.subTest(relative=relative), tempfile.TemporaryDirectory() as directory:
                root = self.make_package(directory)
                metadata = root / "nix/store" / RUNTIME_ITEM / relative
                metadata.parent.mkdir(parents=True, exist_ok=True)
                metadata.write_text("development metadata\n")
                with self.assertRaisesRegex(audit.AuditError, "development payload"):
                    self.inspect(root)

    def test_broken_packaged_symlink_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            broken = root / "nix/store" / RUNTIME_ITEM / "lib/broken.so"
            broken.symlink_to("/nix/store/22222222222222222222222222222222-missing/lib.so")
            with self.assertRaisesRegex(audit.AuditError, "broken symlinks"):
                self.inspect(root)

    def test_symlink_loop_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            library_dir = root / "nix/store" / RUNTIME_ITEM / "lib"
            (library_dir / "loop-a.so").symlink_to("loop-b.so")
            (library_dir / "loop-b.so").symlink_to("loop-a.so")
            with self.assertRaisesRegex(audit.AuditError, "broken symlinks"):
                self.inspect(root)

    def test_relative_host_etc_symlink_is_accepted(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_package(directory)
            environment_dir = root / "nix/store" / RUNTIME_ITEM / "lib/environment.d"
            environment_dir.mkdir(parents=True)
            (environment_dir / "99-environment.conf").symlink_to(
                "../../../../../etc/environment"
            )
            self.inspect(root)


if __name__ == "__main__":
    unittest.main()
