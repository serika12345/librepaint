#!/usr/bin/env python3

import json
import os
import pathlib
import subprocess
import tempfile
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
BUILD_SCRIPT = REPO_ROOT / "scripts" / "build-incremental"


class IncrementalDevelopmentContractTests(unittest.TestCase):
    def run_build_script(self, *arguments: str, environment=None):
        return subprocess.run(
            [str(BUILD_SCRIPT), *arguments],
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
            fake_bin = temp_root / "bin"
            fake_bin.mkdir()
            fake_cmake = fake_bin / "cmake"
            fake_cmake.write_text(
                "#!/usr/bin/env bash\nprintf '%s\\n' \"$*\" >>\"$COMMAND_LOG\"\n",
                encoding="utf-8",
            )
            fake_cmake.chmod(0o755)
            environment = os.environ.copy()
            environment.update(
                {
                    "COMMAND_LOG": str(command_log),
                    "LIBREPAINT_NATIVE_MARKER_PATH": str(
                        temp_root / "native-config"
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


if __name__ == "__main__":
    unittest.main()
