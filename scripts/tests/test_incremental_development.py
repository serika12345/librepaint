#!/usr/bin/env python3

import json
import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
BUILD_SCRIPT = REPO_ROOT / "scripts" / "build-incremental"
BASH = shutil.which("bash")


class IncrementalDevelopmentContractTests(unittest.TestCase):
    def run_build_script(self, *arguments: str, environment=None):
        self.assertIsNotNone(BASH, "bash must be available in the test environment")
        return subprocess.run(
            [BASH, str(BUILD_SCRIPT), *arguments],
            cwd=REPO_ROOT,
            env=environment,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )

    def test_direnv_enters_the_test_shell_and_exposes_repository_commands(self):
        envrc = (REPO_ROOT / ".envrc").read_text(encoding="utf-8")

        self.assertIn("use flake .#test", envrc)
        self.assertIn("PATH_add scripts", envrc)
        self.assertIn("watch_file CMakePresets.json", envrc)

    def test_native_path_uses_the_host_tdd_tree(self):
        result = self.run_build_script("native", "path")

        self.assertEqual(result.returncode, 0, result.stderr)
        expected_platform = "macos" if os.uname().sysname == "Darwin" else "linux"
        self.assertEqual(
            result.stdout.strip(),
            str(REPO_ROOT / "build" / f"tdd-{expected_platform}"),
        )

    def test_native_configure_uses_the_host_preset(self):
        with tempfile.TemporaryDirectory() as temp_directory:
            temp_root = pathlib.Path(temp_directory)
            command_log = temp_root / "commands"
            boundary_log = temp_root / "boundaries"
            compile_commands_link = temp_root / "compile_commands.json"
            fake_bin = temp_root / "bin"
            fake_bin.mkdir()
            fake_cmake = fake_bin / "cmake"
            fake_cmake.write_text(
                f"#!{BASH}\nprintf '%s\\n' \"$*\" >>\"$COMMAND_LOG\"\n",
                encoding="utf-8",
            )
            fake_cmake.chmod(0o755)
            fake_python = fake_bin / "python3"
            fake_python.write_text(
                f"#!{BASH}\nprintf '%s\\n' \"$*\" >>\"$BOUNDARY_LOG\"\n",
                encoding="utf-8",
            )
            fake_python.chmod(0o755)
            environment = os.environ.copy()
            environment.update(
                {
                    "BOUNDARY_LOG": str(boundary_log),
                    "COMMAND_LOG": str(command_log),
                    "LIBREPAINT_NATIVE_MARKER_PATH": str(
                        temp_root / "native-config"
                    ),
                    "LIBREPAINT_COMPILE_COMMANDS_LINK_PATH": str(
                        compile_commands_link
                    ),
                    "LIBREPAINT_TEST_SHELL": "1",
                    "PATH": f"{fake_bin}:{environment['PATH']}",
                }
            )

            result = self.run_build_script(
                "native", "configure", environment=environment
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            expected_platform = "macos" if os.uname().sysname == "Darwin" else "linux"
            self.assertEqual(
                command_log.read_text(encoding="utf-8").strip(),
                f"--preset tdd-{expected_platform}",
            )
            boundary_commands = boundary_log.read_text(encoding="utf-8").splitlines()
            self.assertEqual(
                boundary_commands[0],
                f"{REPO_ROOT / 'scripts/architecture/check_package_boundaries.py'} "
                f"--prepare-query {REPO_ROOT / 'build' / f'tdd-{expected_platform}'}",
            )
            self.assertIn(
                f"{REPO_ROOT / 'scripts/architecture/check_package_boundaries.py'} "
                f"--reply-directory {REPO_ROOT / 'build' / f'tdd-{expected_platform}' / '.cmake/api/v1/reply'} "
                f"--platform {expected_platform} --build-profile tdd-{expected_platform}",
                boundary_commands[1],
            )
            self.assertTrue(compile_commands_link.is_symlink())
            self.assertEqual(
                compile_commands_link.resolve(strict=False),
                REPO_ROOT
                / "build"
                / f"tdd-{expected_platform}"
                / "compile_commands.json",
            )

    def test_clang_tidy_configuration_matches_the_development_toolchain(self):
        clang_tidy = shutil.which("clang-tidy")
        self.assertIsNotNone(
            clang_tidy, "clang-tidy must be available in the test environment"
        )

        result = subprocess.run(
            [clang_tidy, "--verify-config"],
            cwd=REPO_ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )

        self.assertNotIn("Error parsing", result.stdout)
        self.assertNotIn("warning:", result.stdout)
        self.assertIn("No config errors detected.", result.stdout)

    def test_native_presets_route_compilers_through_the_shared_cache(self):
        presets = json.loads(
            (REPO_ROOT / "CMakePresets.json").read_text(encoding="utf-8")
        )
        common = next(
            preset
            for preset in presets["configurePresets"]
            if preset["name"] == "tdd-common"
        )

        self.assertEqual(
            common["cacheVariables"]["CMAKE_C_COMPILER_LAUNCHER"], "ccache"
        )
        self.assertEqual(
            common["cacheVariables"]["CMAKE_CXX_COMPILER_LAUNCHER"], "ccache"
        )

    def test_native_test_runtime_uses_the_incremental_build_tree(self):
        presets = json.loads(
            (REPO_ROOT / "CMakePresets.json").read_text(encoding="utf-8")
        )
        test_presets = {
            preset["name"]: preset for preset in presets["testPresets"]
        }

        self.assertEqual(
            test_presets["tdd-macos"]["environment"]["KIS_TEST_PREFIX_PATH"],
            "${sourceDir}/build/tdd-macos",
        )
        self.assertEqual(
            test_presets["tdd-linux"]["environment"]["KIS_TEST_PREFIX_PATH"],
            "${sourceDir}/build/tdd-linux",
        )

        test_config = (
            REPO_ROOT / "cmake" / "config" / "KoTestConfig.h.cmake"
        ).read_text(encoding="utf-8")
        self.assertIn(
            '#define KRITA_PLUGINS_DIR_FOR_TESTS "${CMAKE_BINARY_DIR}/bin"',
            test_config,
        )

    def test_every_platform_entry_point_is_executable(self):
        entry_points = [
            BUILD_SCRIPT,
            REPO_ROOT / "packaging/ios/scripts/build-librepaint-incremental.sh",
            REPO_ROOT / "scripts/platform/build-android-incremental",
            REPO_ROOT / "scripts/platform/build-windows-incremental",
        ]

        for entry_point in entry_points:
            with self.subTest(entry_point=entry_point):
                self.assertTrue(os.access(entry_point, os.X_OK))

    def test_unknown_platform_reports_the_supported_platforms(self):
        result = self.run_build_script("haiku", "build")

        self.assertEqual(result.returncode, 2)
        self.assertIn("native, macos, linux, ios, android, windows", result.stderr)

    def test_windows_source_preparer_runs_in_an_isolated_process(self):
        incremental_script = (
            REPO_ROOT / "scripts/platform/build-windows-incremental"
        ).read_text(encoding="utf-8")
        windows_expression = (
            REPO_ROOT / "nix/windows/krita.nix"
        ).read_text(encoding="utf-8")

        self.assertIn('"$LIBREPAINT_WINDOWS_SOURCE_PREPARER"', incremental_script)
        self.assertNotIn(
            'source "$LIBREPAINT_WINDOWS_SOURCE_PREPARER"', incremental_script
        )
        self.assertIn(
            'sourcePreparer = buildPkgs.writeShellScript', windows_expression
        )
        self.assertIn('source ${buildPkgs.stdenv}/setup', windows_expression)

    def test_windows_configuration_runs_inside_the_build_tree(self):
        incremental_script = (
            REPO_ROOT / "scripts/platform/build-windows-incremental"
        ).read_text(encoding="utf-8")

        self.assertRegex(
            incremental_script,
            r'(?s)configure_prepared_tree\(\).*?cd "\$build_dir".*?cmake -S',
        )

    def test_windows_incremental_configuration_has_a_build_type(self):
        windows_expression = (
            REPO_ROOT / "nix/windows/default.nix"
        ).read_text(encoding="utf-8")

        self.assertIn('"-DCMAKE_BUILD_TYPE:STRING=Release"', windows_expression)

    def test_android_incremental_configuration_has_a_build_type(self):
        android_expression = (
            REPO_ROOT / "nix/android/default.nix"
        ).read_text(encoding="utf-8")

        self.assertIn('"-DCMAKE_BUILD_TYPE:STRING=Release"', android_expression)


if __name__ == "__main__":
    unittest.main()
