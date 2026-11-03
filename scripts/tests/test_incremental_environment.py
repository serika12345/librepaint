#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


ENTRY = Path(__file__).resolve().parents[1] / "platform/enter-incremental-environment"
IOS_ENTRY = Path(__file__).resolve().parents[2] / "packaging/ios/scripts/build-librepaint-incremental.sh"
PROBE_KEYS = (
    "INCREMENTAL_CONTRACT_READY", "INCREMENTAL_CONTRACT_ENTERED",
    "Boost_DIR", "Qt6WaylandClient_DIR", "LIBREPAINT_CACHE_ROOT",
    "LIBREPAINT_WINDOWS_BUILD_ROOT", "LIBREPAINT_ANDROID_BUILD_DIR",
    "LIBREPAINT_ANDROID_REQUESTED_PROFILE", "CMAKE_BUILD_PARALLEL_LEVEL",
    "ANDROID_SERIAL", "LIBREPAINT_ANDROID_TEST_POLL_LIMIT",
)


class IncrementalEnvironmentTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # A source-independent environment exercises the real Nix process boundary.
        variables = {
            "PATH": os.environ["PATH"],
            "outputs": "out",
            "out": "/unbuilt-incremental-contract-output",
            "shellHook": "export INCREMENTAL_CONTRACT_READY=1",
        }
        with tempfile.TemporaryDirectory() as directory:
            environment = Path(directory) / "incremental-contract-env"
            environment.write_text(json.dumps({
                "bashFunctions": {},
                "variables": {
                    key: {"type": "exported", "value": value}
                    for key, value in variables.items()
                },
            }, sort_keys=True))
            cls.profile = subprocess.check_output(
                ["nix-store", "--add", str(environment)], text=True
            ).strip()

    def invoke(self, platform, controls=None, arguments=(), exit_code=0):
        shell = f"librepaint-{platform}"
        with tempfile.TemporaryDirectory(prefix="incremental environment ") as directory:
            root = Path(directory).resolve()
            entry = root / "scripts/platform/enter-incremental-environment"
            entry.parent.mkdir(parents=True)
            shutil.copy2(ENTRY, entry)
            configuration = root / "nix" / (
                "android" if platform == "android-x86_64" else platform
            )
            configuration.mkdir(parents=True)
            (configuration / "default.nix").write_text("{}\n")
            for name in ("flake.nix", "flake.lock"):
                (root / name).write_text("{}\n")
            inputs = sorted([root / "flake.nix", root / "flake.lock",
                             configuration / "default.nix"])
            fingerprint = f"schema=1\nshell={shell}\n"
            for path in inputs:
                digest = hashlib.sha256(path.read_bytes()).hexdigest()
                fingerprint += f"path={path.relative_to(root)}\n{digest}  {path}\n"
            profile_root = root / f"build/nix-profiles/{platform}-incremental"
            profile_root.mkdir(parents=True)
            (profile_root / "profile").symlink_to(self.profile)
            (profile_root / "fingerprint").write_text(
                hashlib.sha256(fingerprint.encode()).hexdigest() + "\n"
            )
            probe = root / "consumer.py"
            probe.write_text(
                "import json, os, sys\n"
                f"keys = {PROBE_KEYS!r}\n"
                "environment = {key: os.environ[key] for key in keys if key in os.environ}\n"
                "print(json.dumps({'environment': environment, 'arguments': sys.argv[1:]}))\n"
                f"sys.exit({exit_code})\n"
            )
            environment = os.environ | {
                "Boost_DIR": "/host-only/boost",
                "Qt6WaylandClient_DIR": "/host-only/wayland",
            } | (controls or {})
            result = subprocess.run(
                ["bash", str(entry), platform, shell, "INCREMENTAL_CONTRACT_ENTERED",
                 sys.executable, str(probe), *arguments],
                cwd=root, env=environment, capture_output=True, text=True,
                check=False, timeout=30,
            )
            self.assertEqual(result.returncode, exit_code, result.stderr)
            return json.loads(result.stdout)

    def test_cross_build_consumer_uses_only_its_pinned_dependency_environment(self):
        for platform in ("windows", "android", "android-x86_64"):
            with self.subTest(platform=platform):
                environment = self.invoke(platform)["environment"]
                self.assertEqual(environment["INCREMENTAL_CONTRACT_READY"], "1")
                self.assertEqual(environment["INCREMENTAL_CONTRACT_ENTERED"], "1")
                self.assertFalse("Boost_DIR" in environment,
                                 "host Boost configuration reached the cross-build consumer")
                self.assertFalse("Qt6WaylandClient_DIR" in environment,
                                 "host Qt configuration reached the cross-build consumer")

    def test_explicit_build_controls_arguments_and_failure_reach_the_caller(self):
        controls = {
            "LIBREPAINT_CACHE_ROOT": "/task/cache with spaces",
            "LIBREPAINT_WINDOWS_BUILD_ROOT": "/task/windows build",
            "LIBREPAINT_ANDROID_BUILD_DIR": "/task/android build",
            "LIBREPAINT_ANDROID_REQUESTED_PROFILE": "x86_64",
            "CMAKE_BUILD_PARALLEL_LEVEL": "2",
            "ANDROID_SERIAL": "contract-device",
            "LIBREPAINT_ANDROID_TEST_POLL_LIMIT": "12",
        }
        arguments = ["target with spaces", "literal$(value);*"]
        result = self.invoke("android-x86_64", controls, arguments, exit_code=23)
        self.assertEqual(result["arguments"], arguments)
        for key, value in controls.items():
            self.assertEqual(result["environment"].get(key), value, key)

    def test_shared_gpu_patch_refreshes_cross_profiles_while_application_edits_reuse_them(self):
        for platform in ("windows", "android", "android-x86_64", "ios"):
            with self.subTest(platform=platform), tempfile.TemporaryDirectory() as directory:
                root = Path(directory).resolve()
                ios = platform == "ios"
                entry = root / ("packaging/ios/scripts/build-librepaint-incremental.sh" if ios
                                else "scripts/platform/enter-incremental-environment")
                entry.parent.mkdir(parents=True)
                shutil.copy2(IOS_ENTRY if ios else ENTRY, entry)
                configuration = root / "nix" / ("android" if platform == "android-x86_64" else platform)
                configuration.mkdir(parents=True)
                (configuration / "default.nix").write_text("{}\n")
                patch = root / "nix/gpu/patches/device-errors.patch"
                patch.parent.mkdir(parents=True)
                patch.write_text("initial GPU dependency\n")
                for name in ("flake.nix", "flake.lock"):
                    (root / name).write_text("{}\n")
                refreshes = root / "refreshes"
                binary = root / "bin/nix"
                binary.parent.mkdir()
                if ios:
                    for name in ("deps", "frameworks", "manifests", "qt"):
                        (root / "packaging/ios" / name).mkdir()
                    (root / "packaging/ios/versions.env").write_text("\n")
                    git = binary.parent / "git"
                    git.write_text(f"#!{sys.executable}\nprint({str(root)!r})\n")
                    git.chmod(0o755)
                binary.write_text(
                    f"#!{sys.executable}\n"
                    "import os, pathlib, sys\n"
                    "args = sys.argv[1:]\n"
                    "if len(args) > 1 and args[0] == 'develop' and args[1].startswith('.#'):\n"
                    f"    with open({str(refreshes)!r}, 'a') as out: out.write('refresh\\n')\n"
                    "    profile = pathlib.Path(args[args.index('--profile') + 1])\n"
                    "    if not profile.exists():\n"
                    f"        profile.symlink_to({self.profile!r})\n"
                    "    sys.exit(0)\n"
                    "if args[:2] == ['profile', 'wipe-history']: sys.exit(0)\n"
                    + ("if args and args[0] == 'develop': sys.exit(0)\n" if ios else "") +
                    f"os.execv({shutil.which('nix')!r}, [{shutil.which('nix')!r}, *args])\n"
                )
                binary.chmod(0o755)
                environment = os.environ | {"PATH": str(binary.parent) + os.pathsep + os.environ["PATH"]}
                command = ["bash", str(entry), platform, f"librepaint-{platform}",
                           "INCREMENTAL_CONTRACT_ENTERED", sys.executable, "-c",
                           "import os; assert os.environ['INCREMENTAL_CONTRACT_READY'] == '1'"]
                if ios:
                    command = ["bash", str(entry), "path"]

                def enter():
                    result = subprocess.run(command, cwd=root, env=environment,
                                            capture_output=True, text=True, timeout=30)
                    self.assertEqual(result.returncode, 0, result.stderr)

                enter()
                self.assertEqual(refreshes.read_text().splitlines(), ["refresh"])
                patch.write_text("updated GPU dependency\n")
                enter()
                self.assertEqual(refreshes.read_text().splitlines(), ["refresh", "refresh"])
                source = root / "libs/paint.cpp"
                source.parent.mkdir()
                source.write_text("application edit\n")
                enter()
                self.assertEqual(refreshes.read_text().splitlines(), ["refresh", "refresh"])


if __name__ == "__main__":
    unittest.main()
