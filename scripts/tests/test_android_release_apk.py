#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[1] / "platform/verify-android-release-apk.py"
ASSET_SCRIPT = Path(__file__).resolve().parents[1] / "platform/check-android-release-assets"
SPEC = importlib.util.spec_from_file_location("android_release_apk", SCRIPT)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT}")
release = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = release
SPEC.loader.exec_module(release)


GOOD_BADGING = """package: name='io.github.serika12345.librepaint' versionCode='1000003' versionName='1.0.3'
minSdkVersion:'28'
targetSdkVersion:'35'
application-label:'LibrePaint'
"""
CERTIFICATE = "a" * 64
GOOD_SIGNATURE = f"Verified using v2 scheme (APK Signature Scheme v2): true\nNumber of signers: 1\nSigner #1 certificate SHA-256 digest: {CERTIFICATE}\n"


class AndroidReleaseApkTest(unittest.TestCase):
    def inspect(self, badging=GOOD_BADGING, signature=GOOD_SIGNATURE):
        with tempfile.TemporaryDirectory() as directory:
            apk = Path(directory) / "LibrePaint-v1.0.3-arm64-v8a.apk"
            apk.write_bytes(b"apk")

            def tool_result(command, **_kwargs):
                if command[0] == "aapt2":
                    return subprocess.CompletedProcess(command, 0, badging, "")
                if command[0] == "apksigner":
                    return subprocess.CompletedProcess(command, 0, signature, "")
                return subprocess.CompletedProcess(command, 0, "", "")

            with mock.patch.object(release.subprocess, "run", side_effect=tool_result):
                release.verify_apk(apk, "aapt2", "zipalign", "1.0.3", 1000003, "apksigner", CERTIFICATE)

    def test_signed_release_identity_is_accepted(self):
        self.inspect()

    def test_krita_identity_and_version_are_rejected(self):
        old_badging = GOOD_BADGING.replace(
            "io.github.serika12345.librepaint", "org.krita"
        ).replace("1000003", "5050400").replace("1.0.3", "5.4.0-prealpha")
        with self.assertRaisesRegex(release.VerificationError, "application ID"):
            self.inspect(badging=old_badging)

    def test_wrong_release_certificate_is_rejected(self):
        with self.assertRaisesRegex(release.VerificationError, "signing certificate"):
            self.inspect(signature=GOOD_SIGNATURE.replace(CERTIFICATE, "b" * 64))

    def test_wrong_version_code_is_rejected(self):
        with self.assertRaisesRegex(release.VerificationError, "versionCode"):
            self.inspect(badging=GOOD_BADGING.replace("1000003", "1000002"))

    def inspect_release_assets(self, extra_asset):
        with tempfile.TemporaryDirectory() as directory:
            fake_gh = Path(directory) / "gh"
            fake_gh.write_text(
                "#!/bin/sh\nprintf '%s\\n' "
                "'LibrePaint-v1.0.3-arm64-v8a.apk' "
                "'LibrePaint-v1.0.3-x86_64.apk' "
                "'SHA256SUMS.android' "
                "'LibrePaint-1.0.3-x86_64-windows.zip' "
                "'LibrePaint-1.0.3-x86_64.AppImage' "
                "'LibrePaint-iOS-v1.0.3-unsigned.ipa' "
                f"'{extra_asset}'\n"
            )
            fake_gh.chmod(0o755)
            env = os.environ | {
                "PATH": f"{directory}:{os.environ['PATH']}",
                "ANDROID_RELEASE_BUILD_TOOLS": directory,
            }
            result = subprocess.run(
                ["bash", str(ASSET_SCRIPT), "v1.0.3", "1000003", CERTIFICATE],
                capture_output=True,
                text=True,
                env=env,
                check=False,
            )
            self.assertEqual(result.returncode, 1)
            return result.stderr

    def test_internal_update_apk_cannot_pass_public_release_check(self):
        diagnostic = self.inspect_release_assets("LibrePaint-v1.0.3-update-baseline-arm64-v8a.apk")
        self.assertIn("unsigned or internal Android APK", diagnostic)

    def test_aab_cannot_pass_public_release_check(self):
        diagnostic = self.inspect_release_assets("LibrePaint-v1.0.3-arm64-v8a.aab")
        self.assertIn("release asset inventory", diagnostic)


if __name__ == "__main__":
    unittest.main()
