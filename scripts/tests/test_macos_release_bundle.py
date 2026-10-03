#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import importlib.util
import os
from pathlib import Path
import plistlib
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[1] / "platform/audit-macos-bundle.py"
SPEC = importlib.util.spec_from_file_location("audit_macos_bundle", SCRIPT)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT}")
audit = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = audit
SPEC.loader.exec_module(audit)

DEPLOY_SCRIPT = Path(__file__).resolve().parents[2] / "packaging/macos/macos-deploy.py"
DEPLOY_SPEC = importlib.util.spec_from_file_location("macos_deploy", DEPLOY_SCRIPT)
if DEPLOY_SPEC is None or DEPLOY_SPEC.loader is None:
    raise RuntimeError(f"cannot import {DEPLOY_SCRIPT}")
deploy = importlib.util.module_from_spec(DEPLOY_SPEC)
sys.modules[DEPLOY_SPEC.name] = deploy
DEPLOY_SPEC.loader.exec_module(deploy)
DMG_CHECK = Path(__file__).resolve().parents[1] / "platform/check-macos-release-dmg"


def otool_l(path: str, dependencies: tuple[str, ...]) -> str:
    lines = [f"{path}:"]
    lines.extend(
        f"\t{dependency} (compatibility version 1.0.0, current version 1.0.0)"
        for dependency in dependencies
    )
    return "\n".join(lines) + "\n"


def otool_rpaths(rpaths: tuple[str, ...]) -> str:
    blocks = []
    for rpath in rpaths:
        blocks.append(
            "Load command 1\n"
            "          cmd LC_RPATH\n"
            "      cmdsize 48\n"
            f"         path {rpath} (offset 12)\n"
        )
    return "".join(blocks)


class MacOSReleaseBundleTest(unittest.TestCase):
    def inspect(
        self,
        records: dict[str, dict[str, object]],
        *,
        extra_files: tuple[str, ...] = (),
        symlinks: dict[str, str] | None = None,
        verify_signature: bool = False,
        signature_returncode: int = 0,
    ) -> audit.BundleReport:
        with tempfile.TemporaryDirectory() as directory:
            app = (Path(directory) / "LibrePaint.app").resolve()
            for relative_path, record in records.items():
                path = app / relative_path
                path.parent.mkdir(parents=True, exist_ok=True)
                payload = record.get("payload", relative_path.encode())
                path.write_bytes(b"\xcf\xfa\xed\xfe" + bytes(payload))
            for relative_path in extra_files:
                path = app / relative_path
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("fixture")
            for relative_path, target in (symlinks or {}).items():
                path = app / relative_path
                path.parent.mkdir(parents=True, exist_ok=True)
                path.symlink_to(target)

            def tool_result(
                command: list[str], **_kwargs: object
            ) -> subprocess.CompletedProcess[str]:
                tool = Path(command[0]).name
                if tool == "codesign":
                    return subprocess.CompletedProcess(
                        command,
                        signature_returncode,
                        "",
                        "invalid signature" if signature_returncode else "",
                    )

                path = Path(command[-1])
                relative = path.relative_to(app).as_posix()
                record = records[relative]
                if tool == "nm":
                    symbols = (
                        record.get("undefined_symbols", ())
                        if "-u" in command
                        else record.get("exported_symbols", ())
                    )
                    output = "".join(f"00000000 T {symbol}\n" for symbol in symbols)
                    return subprocess.CompletedProcess(command, 0, output, "")
                if tool == "lipo":
                    return subprocess.CompletedProcess(
                        command, 0, str(record.get("architectures", "arm64")) + "\n", ""
                    )
                if command[1] == "-D":
                    install_name = record.get("install_name")
                    output = f"{path}\n{install_name}\n" if install_name else f"{path}\n"
                    return subprocess.CompletedProcess(command, 0, output, "")
                if command[1] == "-L":
                    dependencies = tuple(record.get("dependencies", ()))
                    install_name = record.get("install_name")
                    if install_name:
                        dependencies = (str(install_name),) + dependencies
                    return subprocess.CompletedProcess(
                        command, 0, otool_l(str(path), dependencies), ""
                    )
                if command[1] == "-l":
                    return subprocess.CompletedProcess(
                        command,
                        0,
                        otool_rpaths(tuple(record.get("rpaths", ()))),
                        "",
                    )
                raise AssertionError(command)

            with mock.patch.object(audit.subprocess, "run", side_effect=tool_result):
                return audit.inspect_bundle(
                    app,
                    lipo="lipo",
                    otool="otool",
                    codesign="codesign",
                    require_features=False,
                    verify_signature=verify_signature,
                )

    def valid_records(self) -> dict[str, dict[str, object]]:
        return {
            "Contents/MacOS/LibrePaint": {
                "dependencies": ("@rpath/libsample.dylib", "/usr/lib/libc++.1.dylib"),
                "rpaths": ("@executable_path/../Frameworks",),
            },
            "Contents/Frameworks/libsample.dylib": {
                "install_name": "@rpath/libsample.dylib",
                "dependencies": ("/System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation",),
                "rpaths": ("@loader_path",),
            },
        }

    def test_arm64_bundle_resolves_internal_and_system_dependencies(self) -> None:
        report = self.inspect(self.valid_records())

        self.assertEqual(report.mach_o_count, 2)
        self.assertEqual(report.architecture, "arm64")

    def test_missing_internal_dependency_reports_consumer(self) -> None:
        records = self.valid_records()
        records["Contents/MacOS/LibrePaint"]["dependencies"] = (
            "@rpath/libmissing.dylib",
        )
        with self.assertRaisesRegex(
            audit.AuditError, r"libmissing[.]dylib.*Contents/MacOS/LibrePaint"
        ):
            self.inspect(records)

    def test_non_arm64_mach_o_is_rejected(self) -> None:
        records = self.valid_records()
        records["Contents/Frameworks/libsample.dylib"]["architectures"] = "x86_64"
        with self.assertRaisesRegex(audit.AuditError, "x86_64"):
            self.inspect(records)

    def test_missing_and_nix_store_rpaths_are_rejected(self) -> None:
        records = self.valid_records()
        records["Contents/Frameworks/libsample.dylib"]["rpaths"] = (
            "@loader_path/not-present",
            "@loader_path/../nix/store/example/lib",
        )
        with self.assertRaisesRegex(
            audit.AuditError, r"(?s)invalid RPATH.*not-present.*nix/store"
        ):
            self.inspect(records)

    def test_conflicting_install_names_are_rejected(self) -> None:
        records = self.valid_records()
        records["Contents/Frameworks/libother.dylib"] = {
            "install_name": "@rpath/libsample.dylib",
            "payload": b"different library",
        }
        with self.assertRaisesRegex(
            audit.AuditError, r"(?s)conflicting Mach-O install names.*libsample"
        ):
            self.inspect(records)

    def test_identical_install_name_alias_is_allowed(self) -> None:
        records = self.valid_records()
        records["Contents/Frameworks/libsample.dylib"]["payload"] = b"same library"
        records["Contents/Frameworks/libsample.1.dylib"] = {
            "install_name": "@rpath/libsample.dylib",
            "payload": b"same library",
        }

        report = self.inspect(records)

        self.assertEqual(report.mach_o_count, 3)

    def test_iconv_abi_mismatch_is_rejected(self) -> None:
        records = {
            "Contents/MacOS/LibrePaint": {
                "dependencies": ("@rpath/libiconv.2.dylib",),
                "rpaths": ("@executable_path/../Frameworks",),
                "undefined_symbols": ("_libiconv", "_libiconv_open"),
            },
            "Contents/Frameworks/libiconv.2.dylib": {
                "install_name": "@rpath/libiconv.2.dylib",
                "exported_symbols": ("_iconv", "_iconv_open"),
            },
        }

        with self.assertRaisesRegex(
            audit.AuditError,
            r"iconv ABI mismatch.*_libiconv.*Contents/MacOS/LibrePaint",
        ):
            self.inspect(records)

        records["Contents/MacOS/LibrePaint"]["dependencies"] = ("/usr/lib/libiconv.2.dylib",)
        del records["Contents/Frameworks/libiconv.2.dylib"]
        with self.assertRaisesRegex(
            audit.AuditError,
            r"iconv ABI mismatch.*_libiconv.*Contents/MacOS/LibrePaint",
        ):
            self.inspect(records)

    def test_development_and_qml_debug_payload_is_rejected(self) -> None:
        with self.assertRaisesRegex(
            audit.AuditError,
            r"development or test payload.*Headers.*[.]prl.*qmltooling.*QtTest",
        ):
            self.inspect(
                self.valid_records(),
                extra_files=(
                    "Contents/Frameworks/QtCore.framework/Versions/A/Headers/qobject.h",
                    "Contents/Frameworks/QtCore.framework/Versions/A/Resources/QtCore.prl",
                    "Contents/Resources/qml/QtTest/qmldir",
                    "Contents/PlugIns/qmltooling/libqmldbg_debugger.txt",
                ),
            )

    def test_python_development_and_test_payload_is_rejected(self) -> None:
        python_root = (
            "Contents/Frameworks/Python.framework/Versions/3.14/lib/"
            "python3.14/site-packages/PyQt6"
        )
        with self.assertRaisesRegex(
            audit.AuditError,
            r"development or test payload.*PyQt6/QtCore[.]pyi.*"
            r"PyQt6/QtTest[.]abi3[.]so.*PyQt6/bindings",
        ):
            self.inspect(
                self.valid_records(),
                extra_files=(
                    f"{python_root}/QtCore.pyi",
                    f"{python_root}/QtTest.abi3.so",
                    f"{python_root}/bindings/QtCore/QtCore.toml",
                ),
            )

    def test_broken_and_external_symlinks_are_rejected(self) -> None:
        with self.assertRaisesRegex(
            audit.AuditError, r"invalid bundle symlink.*external.*missing"
        ):
            self.inspect(
                self.valid_records(),
                symlinks={
                    "Contents/Frameworks/external": "/nix/store/example/lib.dylib",
                    "Contents/Frameworks/missing": "not-present.dylib",
                },
            )

    def test_invalid_code_signature_is_rejected(self) -> None:
        with self.assertRaisesRegex(audit.AuditError, "code signature"):
            self.inspect(
                self.valid_records(),
                verify_signature=True,
                signature_returncode=1,
            )

    def test_required_runtime_features_are_checked(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            app = (Path(directory) / "LibrePaint.app").resolve()
            executable = app / "Contents/MacOS/LibrePaint"
            executable.parent.mkdir(parents=True)
            executable.write_bytes(b"\xcf\xfa\xed\xfeproduct")

            def tool_result(
                command: list[str], **_kwargs: object
            ) -> subprocess.CompletedProcess[str]:
                if Path(command[0]).name == "lipo":
                    return subprocess.CompletedProcess(command, 0, "arm64\n", "")
                if command[1] == "-D":
                    return subprocess.CompletedProcess(command, 0, f"{executable}\n", "")
                if command[1] == "-L":
                    return subprocess.CompletedProcess(
                        command, 0, otool_l(str(executable), ()), ""
                    )
                return subprocess.CompletedProcess(command, 0, "", "")

            with mock.patch.object(audit.subprocess, "run", side_effect=tool_result):
                with self.assertRaisesRegex(
                    audit.AuditError, r"missing macOS runtime features.*ffmpeg.*PyKrita"
                ):
                    audit.inspect_bundle(
                        app,
                        lipo="lipo",
                        otool="otool",
                        codesign="codesign",
                        verify_signature=False,
                    )

    def test_python_runtime_requires_stdlib_and_pyqt(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            missing = audit._feature_diagnostics(Path(directory))

        self.assertIn("Python standard library", missing)
        self.assertIn("PyQt6 runtime", missing)
        self.assertIn("macOS font configuration", missing)

    def test_qt_configuration_checks_runtime_locations(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            configuration = root / "Contents/Resources/qt.conf"
            configuration.parent.mkdir(parents=True)
            configuration.write_text(
                "[Paths]\nPlugins=PlugIns\nImports=Resources/qml\nQmlImports=Resources/qml\n"
            )
            self.assertNotIn("relocatable Qt runtime configuration", audit._feature_diagnostics(root))
            configuration.write_text(
                "[Paths]\nPlugins=/nix/store/qt/plugins\nImports=Resources/qml\nQmlImports=Resources/qml\n"
            )
            self.assertIn("relocatable Qt runtime configuration", audit._feature_diagnostics(root))

    def test_font_configuration_uses_only_macos_runtime_directories(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            fontconfig = root / "Contents/Resources/fontconfig/fonts.conf"
            fontconfig.parent.mkdir(parents=True)
            fontconfig.write_text(
                "<fontconfig>"
                "<dir>/System/Library/Fonts</dir>"
                "<dir>/Library/Fonts</dir>"
                "<dir>~/Library/Fonts</dir>"
                "</fontconfig>"
            )

            missing = audit._feature_diagnostics(root)

        self.assertNotIn("macOS font configuration", missing)

    def test_deployment_installs_owned_font_configuration(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source"
            resources = root / "LibrePaint.app/Contents/Resources"
            config = source / "packaging/macos/fonts.conf"
            config.parent.mkdir(parents=True)
            config.write_text("runtime fonts")

            destination = deploy.installFontconfigConfiguration(source, resources)

            self.assertEqual(
                destination,
                resources / "fontconfig/fonts.conf",
            )
            self.assertEqual(destination.read_text(), "runtime fonts")

    def test_deployment_cleanup_removes_non_runtime_payload(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            app = Path(directory) / "LibrePaint.app"
            removable = (
                "Contents/Frameworks/QtTest.framework/Versions/A/QtTest",
                "Contents/Frameworks/QtQuickTest.framework/Versions/A/QtQuickTest",
                "Contents/Frameworks/QtCore.framework/Versions/A/Headers/qobject.h",
                "Contents/Frameworks/QtCore.framework/Versions/A/Resources/QtCore.prl",
                "Contents/PlugIns/permissions/plugin.a",
                "Contents/PlugIns/qmllint/plugin.dylib",
                "Contents/PlugIns/qmlls/plugin.dylib",
                "Contents/PlugIns/qmltooling/plugin.dylib",
                "Contents/Resources/qml/Qt/test/controls/qmldir",
                "Contents/Resources/qml/QtTest/qmldir",
                (
                    "Contents/Frameworks/Python.framework/Versions/3.14/lib/"
                    "python3.14/site-packages/PyQt6/QtTest.abi3.so"
                ),
                (
                    "Contents/Frameworks/Python.framework/Versions/3.14/lib/"
                    "python3.14/site-packages/PyQt6/QtCore.pyi"
                ),
                (
                    "Contents/Frameworks/Python.framework/Versions/3.14/lib/"
                    "python3.14/site-packages/PyQt6/py.typed"
                ),
                (
                    "Contents/Frameworks/Python.framework/Versions/3.14/lib/"
                    "python3.14/site-packages/PyQt6/bindings/QtCore/QtCore.toml"
                ),
            )
            for relative_path in removable:
                path = app / relative_path
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("remove")
            runtime = app / "Contents/Frameworks/QtCore.framework/Versions/A/QtCore"
            runtime.write_text("keep")
            sdl3_runtime = app / "Contents/Frameworks/libSDL3.dylib"
            sdl3_runtime.write_text("keep")

            removed = deploy.removeDeploymentOnlyPayload(app)

            self.assertGreaterEqual(len(removed), len(removable))
            self.assertTrue(runtime.is_file())
            self.assertTrue(sdl3_runtime.is_file())
            for relative_path in removable:
                self.assertFalse((app / relative_path).exists())

    def test_deployment_installs_sdl3_runtime_used_by_sdl2_compat(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            install = root / "librepaint"
            sdl3_output = root / "sdl3-3.4.12-lib"
            source = sdl3_output / "lib/libSDL3.dylib"
            source.parent.mkdir(parents=True)
            source.write_bytes(b"\xcf\xfa\xed\xfeSDL3")
            frameworks = root / "LibrePaint.app/Contents/Frameworks"
            frameworks.mkdir(parents=True)
            closure = f"{install}\n{sdl3_output}\n"

            with mock.patch.object(
                deploy.subprocess,
                "run",
                return_value=subprocess.CompletedProcess(
                    ["nix-store"], 0, closure, ""
                ),
            ):
                destination = deploy.installSdl3Runtime(install, frameworks)

            self.assertEqual(destination, frameworks / "libSDL3.dylib")
            self.assertEqual(destination.read_bytes(), b"\xcf\xfa\xed\xfeSDL3")

    def test_bundle_launcher_wraps_the_deployed_executable(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source"
            launcher_source = source / "packaging/macos/macos-bundle-launcher.c"
            launcher_source.parent.mkdir(parents=True)
            launcher_source.write_text("launcher")
            macos = root / "LibrePaint.app/Contents/MacOS"
            macos.mkdir(parents=True)
            wrapped_source = root / "kritarunner"
            wrapped_source.write_bytes(b"Mach-O")

            with mock.patch.object(deploy.subprocess, "run") as run:
                launcher = deploy.installBundleLauncher(
                    source,
                    macos,
                    "kritarunner",
                    wrapped_source,
                )

            self.assertEqual(launcher, macos / "kritarunner")
            self.assertEqual(
                (macos / ".kritarunner-wrapped").read_bytes(),
                b"Mach-O",
            )
            run.assert_called_once_with(
                [
                    "/usr/bin/clang",
                    "-Os",
                    '-DBUNDLED_EXECUTABLE=".kritarunner-wrapped"',
                    launcher_source,
                    "-o",
                    launcher,
                ],
                check=True,
            )

    def test_deployment_identifies_mach_o_by_file_magic(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            mach_o = root / "plugin.so"
            mach_o.write_bytes(b"\xcf\xfa\xed\xfe\xd9")
            data = root / "data.bin"
            data.write_bytes(b"text\xd9")

            self.assertTrue(deploy.isBinary(mach_o))
            self.assertFalse(deploy.isBinary(data))

    def test_dependency_closure_visits_transitive_cycle_once(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            app = root / "LibrePaint.app"
            consumer = app / "Contents/MacOS/LibrePaint"
            first = root / "install/first.dylib"
            second = root / "install/second.dylib"
            for path in (consumer, first, second):
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"\xcf\xfa\xed\xfe" + path.name.encode())
            edges = {consumer.name: (first,), first.name: (second,), second.name: (first,)}

            def inspect(command, **kwargs):
                path = Path(command[-1])
                output = f"{path}:\n"
                if command[0] == "otool" and command[1] == "-L":
                    output = otool_l(str(path), tuple(map(str, edges[path.name])))
                return subprocess.CompletedProcess(command, 0, output, "")

            with mock.patch.object(deploy.subprocess, "run", side_effect=inspect) as run:
                copied = deploy.copyNixStoreDependencyClosure(app)

            frameworks = app / "Contents/Frameworks"
            self.assertEqual(set(copied), {frameworks / first.name, frameworks / second.name})
            self.assertEqual(sum(call.args[0][:2] == ["otool", "-L"] for call in run.call_args_list), 3)

    def test_dependency_collection_copies_and_relocates_absolute_inputs(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            app = root / "LibrePaint.app"
            consumer = app / "Contents/MacOS/LibrePaint"
            consumer.parent.mkdir(parents=True)
            consumer.write_bytes(b"\xcf\xfa\xed\xfeconsumer")
            library = root / "install/lib/libsample.1.dylib"
            library.parent.mkdir(parents=True)
            library.write_bytes(b"\xcf\xfa\xed\xfelibrary")

            def inspect(command, **kwargs):
                path = Path(command[-1])
                if command[0] == "otool":
                    if command[1] == "-D":
                        output = f"{path}:\n"
                        if path.name == library.name:
                            output += f"{library}\n"
                    else:
                        output = otool_l(str(path), (str(library),))
                    return subprocess.CompletedProcess(command, 0, output, "")
                return subprocess.CompletedProcess(command, 0, "", "")

            with mock.patch.object(deploy.subprocess, "run", side_effect=inspect) as run:
                copied = deploy.copyNixStoreDependencyClosure(app)

            destination = app / "Contents/Frameworks" / library.name
            self.assertEqual(copied, (destination,))
            self.assertEqual(destination.read_bytes(), library.read_bytes())
            run.assert_any_call(
                ["install_name_tool", "-change", str(library),
                 "@loader_path/../Frameworks/libsample.1.dylib", consumer],
                check=True,
            )
            run.assert_any_call(
                ["install_name_tool", "-id", "@rpath/libsample.1.dylib", destination],
                check=True,
            )

    def test_dependency_collection_preserves_apple_and_gnu_iconv(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            app = root / "LibrePaint.app"
            apple = root / "apple/libiconv.2.dylib"
            gnu = root / "gnu/libiconv.2.dylib"
            apple_consumer = app / "Contents/MacOS/Apple"
            gnu_consumer = app / "Contents/MacOS/GNU"
            for path in (apple, gnu, apple_consumer, gnu_consumer):
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"\xcf\xfa\xed\xfe" + path.parent.name.encode())

            def inspect(command, **kwargs):
                path = Path(command[-1])
                output = f"{path}:\n"
                if command[0] == "otool" and command[1] == "-L":
                    dependencies = {apple_consumer: (str(apple),), gnu_consumer: (str(gnu),)}
                    output = otool_l(str(path), dependencies.get(path, ()))
                return subprocess.CompletedProcess(command, 0, output, "")

            with mock.patch.object(deploy.subprocess, "run", side_effect=inspect) as run, \
                    mock.patch.object(deploy, "_iconvSymbols", side_effect=lambda p, **k:
                                      frozenset({"_iconv"}) if p == apple else frozenset({"_libiconv"})):
                copied = deploy.copyNixStoreDependencyClosure(app)

            destination = app / "Contents/Frameworks/libiconv.2.dylib"
            self.assertEqual(copied, (destination,))
            self.assertEqual(destination.read_bytes(), gnu.read_bytes())
            run.assert_any_call(
                ["install_name_tool", "-change", str(apple), "/usr/lib/libiconv.2.dylib", apple_consumer],
                check=True,
            )
            run.assert_any_call(
                ["install_name_tool", "-change", str(gnu), "@loader_path/../Frameworks/libiconv.2.dylib", gnu_consumer],
                check=True,
            )

    def test_dependency_collection_rejects_different_same_name_library(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            app = root / "LibrePaint.app"
            consumer = app / "Contents/MacOS/LibrePaint"
            existing = app / "Contents/Frameworks/libsample.dylib"
            dependency = root / "other/libsample.dylib"
            for path, payload in (
                (consumer, b"consumer"), (existing, b"existing"),
                (dependency, b"incompatible"),
            ):
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"\xcf\xfa\xed\xfe" + payload)

            def inspect(command, **kwargs):
                path = Path(command[-1])
                if command[1] == "-D":
                    output = f"{path}:\n"
                else:
                    dependencies = (str(dependency),) if path == consumer else ()
                    output = otool_l(str(path), dependencies)
                return subprocess.CompletedProcess(command, 0, output, "")

            with mock.patch.object(deploy.subprocess, "run", side_effect=inspect):
                with self.assertRaisesRegex(RuntimeError, "conflicting runtime dependency.*libsample"):
                    deploy.copyNixStoreDependencyClosure(app)
            self.assertEqual(existing.read_bytes(), b"\xcf\xfa\xed\xfeexisting")

    def test_dependency_collection_preserves_two_required_library_providers(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            app = root / "LibrePaint.app"
            first = root / "first/libsample.dylib"
            second = root / "second/libsample.dylib"
            consumers = {app / "Contents/MacOS/First": first,
                         app / "Contents/MacOS/Second": second}
            for path in (*consumers, first, second):
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"\xcf\xfa\xed\xfe" + str(path).encode())

            def inspect(command, **kwargs):
                path = Path(command[-1])
                output = f"{path}:\n"
                if command[0] == "otool":
                    if command[1] == "-L":
                        dependencies = (str(consumers[path]),) if path in consumers else ()
                        output = otool_l(str(path), dependencies)
                    elif path.name == "libsample.dylib":
                        provider = first if path.read_bytes() == first.read_bytes() else second
                        output += str(provider) + "\n"
                return subprocess.CompletedProcess(command, 0, output, "")

            with mock.patch.object(deploy.subprocess, "run", side_effect=inspect) as run:
                copied = deploy.copyNixStoreDependencyClosure(app)

            self.assertEqual(len(copied), 2)
            identities = []
            for call in run.call_args_list:
                command = call.args[0]
                if command[0] != "install_name_tool":
                    continue
                if command[1] == "-id":
                    identities.append(command[2])
                if command[1] == "-change":
                    consumer = Path(command[-1])
                    target = consumer.parent / command[3].removeprefix("@loader_path/")
                    self.assertEqual(target.read_bytes(), consumers[consumer].read_bytes())
            self.assertEqual(len(set(identities)), 2)

    def test_dependency_closure_accepts_bundled_version_symlink(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            app = root / "LibrePaint.app"
            binary = app / "Contents/Frameworks/libsample.21.0.dylib"
            binary.parent.mkdir(parents=True)
            binary.write_bytes(b"\xcf\xfa\xed\xfelibrary")
            binary.with_name("libsample.21.dylib").symlink_to(binary.name)
            dependency = root / "install/libsample.21.dylib"
            dependency.parent.mkdir()
            dependency.write_bytes(binary.read_bytes())

            def inspect(command, **kwargs):
                output = f"{binary}:\n"
                if command[0] == "otool" and command[1] == "-L":
                    output = otool_l(str(binary), (str(dependency),))
                return subprocess.CompletedProcess(command, 0, output, "")

            with mock.patch.object(deploy.subprocess, "run", side_effect=inspect) as run:
                self.assertEqual(deploy.copyNixStoreDependencyClosure(app), ())
            self.assertEqual(sum(call.args[0][:2] == ["otool", "-L"] for call in run.call_args_list), 1)

    def test_dependency_closure_rejects_non_binary_dependency(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            app = Path(directory) / "LibrePaint.app"
            frameworks = app / "Contents/Frameworks"
            frameworks.mkdir(parents=True)
            consumer = frameworks / "consumer.dylib"
            consumer.write_bytes(b"\xcf\xfa\xed\xfeconsumer")
            dependency = Path("/nix/store/test-invalid/lib/invalid.dylib")
            original_exists = Path.exists

            def copy(source, destination):
                Path(destination).write_bytes(b"invalid")
                if copy_mock.call_count > 1:
                    raise AssertionError("repeated copying without progress")

            with mock.patch.object(Path, "exists", lambda p: p == dependency or original_exists(p)), \
                    mock.patch.object(deploy.shutil, "copy2", side_effect=copy) as copy_mock, \
                    mock.patch.object(deploy, "getLinkedLibs", return_value=[dependency]):
                with self.assertRaisesRegex(RuntimeError, "invalid.dylib"):
                    deploy.copyNixStoreDependencyClosure(app)

    def test_deployment_finds_qml_runtime_roots_with_application_last(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            qt = root / "qtdeclarative"
            application = root / "librepaint"
            for output in (qt, application):
                (output / "lib/qt-6/qml").mkdir(parents=True)
            for module in ("QtQml", "QtQuick"):
                module_root = qt / "lib/qt-6/qml" / module
                module_root.mkdir()
                (module_root / "qmldir").write_text("module")
            closure = f"{application}\n{qt}\n"
            with mock.patch.object(
                deploy.subprocess,
                "run",
                return_value=subprocess.CompletedProcess(
                    ["nix-store"], 0, closure, ""
                ),
            ):
                roots = deploy.findNixQmlRuntimeRoots(application)

            self.assertEqual(
                roots,
                (
                    (qt / "lib/qt-6/qml").resolve(),
                    (application / "lib/qt-6/qml").resolve(),
                ),
            )

    def test_audit_rejects_nix_paths_in_application_launcher(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            launcher = root / "Contents/MacOS/LibrePaint"
            launcher.parent.mkdir(parents=True)
            launcher.write_bytes(b"/nix/store/runtime")

            missing = audit._feature_diagnostics(root)

        self.assertIn("portable application launchers", missing)

    def test_python_framework_stripping_keeps_only_plugin_modules(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            framework = Path(directory) / "Python.framework"
            pyqt = (
                framework
                / "Versions/Current/lib/python3.14/site-packages/PyQt6"
            )
            pyqt.mkdir(parents=True)
            runtime = (
                "QtCore.abi3.so",
                "QtGui.abi3.so",
                "QtNetwork.abi3.so",
                "QtQml.abi3.so",
                "QtWidgets.abi3.so",
                "QtXml.abi3.so",
                "sip.cpython-314-darwin.so",
            )
            unused = ("QtTest.abi3.so",)
            for name in runtime + unused:
                (pyqt / name).write_bytes(b"Mach-O")
            (pyqt / "uic").mkdir()
            (pyqt / "uic/__init__.py").write_text("runtime")

            deploy.kritaStripPythonFramework(framework)

            for name in runtime:
                self.assertTrue((pyqt / name).is_file())
            self.assertTrue((pyqt / "uic/__init__.py").is_file())
            for name in unused:
                self.assertFalse((pyqt / name).exists())

    def test_python_framework_gets_signable_bundle_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            framework = Path(directory) / "Python.framework"
            version = framework / "Versions/3.14"
            version.mkdir(parents=True)
            (framework / "Versions/Current").symlink_to("3.14")

            info_plist = deploy.installPythonFrameworkInfo(framework)

            with info_plist.open("rb") as handle:
                info = plistlib.load(handle)
            self.assertEqual(info["CFBundleExecutable"], "Python")
            self.assertEqual(info["CFBundleIdentifier"], "org.python.python")
            self.assertEqual(info["CFBundlePackageType"], "FMWK")
            self.assertEqual(info["CFBundleShortVersionString"], "3.14")

    def test_python_framework_stripping_relocates_runtime_configuration(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            framework = Path(directory) / "Python.framework"
            stdlib = framework / "Versions/Current/lib/python3.14"
            stdlib.mkdir(parents=True)
            subprocess_module = stdlib / "subprocess.py"
            subprocess_module.write_text(
                "shell = '/nix/store/hash-bash-5.3/bin/sh'\n"
            )
            mimetypes_module = stdlib / "mimetypes.py"
            mimetypes_module.write_text(
                "knownfiles = "
                "['/nix/store/hash-mailcap-2.1/etc/mime.types']\n"
            )

            deploy.kritaStripPythonFramework(framework)

            self.assertEqual(subprocess_module.read_text(), "shell = '/bin/sh'\n")
            self.assertEqual(
                mimetypes_module.read_text(),
                "knownfiles = ['/etc/apache2/mime.types']\n",
            )

    def test_audit_rejects_nix_runtime_paths_in_python_standard_library(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            stdlib = (
                Path(directory)
                / "Contents/Frameworks/Python.framework/Versions/Current/lib/python3.14"
            )
            stdlib.mkdir(parents=True)
            (stdlib / "subprocess.py").write_text(
                "shell = '/nix/store/hash-bash/bin/sh'\n"
            )

            missing = audit._feature_diagnostics(Path(directory))

        self.assertIn("portable Python standard library", missing)

    def test_deployment_cleanup_deletes_only_invalid_rpaths(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            app = Path(directory) / "LibrePaint.app"
            binary = app / "Contents/Frameworks/libsample.dylib"
            binary.parent.mkdir(parents=True)
            binary.write_bytes(b"Mach-O")
            (app / "Contents/MacOS").mkdir(parents=True)

            output = otool_rpaths(
                (
                    "@loader_path",
                    "@executable_path/../Frameworks",
                    "@loader_path/not-present",
                    "@loader_path/../nix/store/example/lib",
                )
            )
            with mock.patch.object(
                deploy.subprocess,
                "run",
                return_value=subprocess.CompletedProcess(
                    ["otool", "-l", str(binary)], 0, output, ""
                ),
            ) as run:
                removed = deploy.cleanInvalidBundleRpaths(app, [binary])

            self.assertEqual(
                removed,
                (
                    (binary, "@loader_path/not-present"),
                    (binary, "@loader_path/../nix/store/example/lib"),
                ),
            )
            self.assertEqual(run.call_count, 3)
            run.assert_any_call(
                ["install_name_tool", "-delete_rpath", "@loader_path/not-present", binary], check=True
            )

    def test_python_consumers_link_to_bundled_framework(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            app = root / "LibrePaint.app"
            frameworks = app / "Contents/Frameworks"
            python = frameworks / "Python.framework"
            (python / "Versions/3.14").mkdir(parents=True)
            (python / "Versions/Current").symlink_to("3.14")
            python_binary = python / "Versions/3.14/Python"
            python_binary.write_bytes(b"\xcf\xfa\xed\xfePython framework")
            pykrita = (
                frameworks
                / "PyKrita.framework/Versions/1.0.3/PyKrita"
            )
            pykrita.parent.mkdir(parents=True)
            pykrita.write_bytes(b"\xcf\xfa\xed\xfePyKrita")
            dependency = root / "install/libpython3.14.dylib"
            dependency.parent.mkdir()
            dependency.write_bytes(b"\xcf\xfa\xed\xfeoriginal Python library")

            def inspect(command, **kwargs):
                path = Path(command[-1])
                output = f"{path}:\n"
                if command[0] == "otool" and command[1] == "-L":
                    dependencies = (str(dependency),) if path == pykrita else ()
                    output = otool_l(str(path), dependencies)
                return subprocess.CompletedProcess(command, 0, output, "")

            with mock.patch.object(deploy.subprocess, "run", side_effect=inspect) as run:
                copied = deploy.copyNixStoreDependencyClosure(app)

            expected = (
                "@loader_path/../../../Python.framework/Versions/3.14/Python"
            )
            self.assertEqual(copied, ())
            self.assertFalse((frameworks / dependency.name).exists())
            run.assert_any_call(
                ["install_name_tool", "-change", str(dependency), expected, pykrita],
                check=True,
            )

    def test_deployment_reports_signing_failure(self) -> None:
        app = Path("/tmp/LibrePaint.app")
        failure = subprocess.CalledProcessError(1, ["codesign"], stderr="signing failed")
        with mock.patch.object(deploy.subprocess, "run", side_effect=failure):
            with self.assertRaises(subprocess.CalledProcessError):
                deploy.signAppBundle(app, "-")

    def test_deployment_audits_before_and_after_signing(self) -> None:
        app = Path("/tmp/LibrePaint.app")
        source = Path("/tmp/librepaint")
        auditor = source / "scripts/platform/audit-macos-bundle.py"
        with mock.patch.object(deploy.subprocess, "run") as run:
            deploy.auditAppBundle(app, source, verify_signature=False)
            deploy.auditAppBundle(app, source, verify_signature=True)

        self.assertEqual(
            run.call_args_list,
            [
                mock.call(
                    [sys.executable, auditor, app, "--skip-signature"],
                    check=True,
                ),
                mock.call([sys.executable, auditor, app], check=True),
            ],
        )

    def test_release_dmg_is_mounted_audited_and_detached(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            bin_dir = root / "bin"
            bin_dir.mkdir()
            dmg = root / "LibrePaint-1.0.3-aarch64-macos.dmg"
            dmg.write_bytes(b"fixture")
            detach_log = root / "detach.log"
            audit_log = root / "audit.log"

            hdiutil = bin_dir / "hdiutil"
            hdiutil.write_text(
                "#!/usr/bin/env python3\n"
                "import os, pathlib, sys\n"
                "args = sys.argv[1:]\n"
                "if args[0] == 'attach':\n"
                "    mount = pathlib.Path(args[args.index('-mountpoint') + 1])\n"
                "    (mount / 'LibrePaint.app').mkdir(parents=True)\n"
                "    print(f'/dev/disk99 Apple_APFS {mount}')\n"
                "elif args[0] == 'detach':\n"
                "    pathlib.Path(os.environ['DETACH_LOG']).write_text(args[-1])\n"
            )
            hdiutil.chmod(0o755)
            auditor = root / "auditor.py"
            auditor.write_text(
                "import os, pathlib, sys\n"
                "app = pathlib.Path(sys.argv[1])\n"
                "assert app.name == 'LibrePaint.app' and app.is_dir()\n"
                "pathlib.Path(os.environ['AUDIT_LOG']).write_text(str(app))\n"
            )

            environment = os.environ.copy()
            environment.update(
                {
                    "PATH": f"{bin_dir}:{environment['PATH']}",
                    "MACOS_BUNDLE_AUDITOR": str(auditor),
                    "DETACH_LOG": str(detach_log),
                    "AUDIT_LOG": str(audit_log),
                }
            )
            result = subprocess.run(
                [DMG_CHECK, dmg],
                capture_output=True,
                text=True,
                env=environment,
                check=False,
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(detach_log.read_text(), "/dev/disk99")
            self.assertTrue(audit_log.read_text().endswith("/LibrePaint.app"))


if __name__ == "__main__":
    unittest.main()
