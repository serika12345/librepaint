#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
import runpy
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


SDK = runpy.run_path(str(Path(__file__).resolve().parents[1] / "plugin-sdk"))


class PluginSdkTests(unittest.TestCase):
    def test_modified_sdk_fails_before_configuring_a_plugin(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "header.h").write_bytes(b"modified")
            (root / "manifest.json").write_text(json.dumps({
                "schemaVersion": 1,
                "files": {"header.h": hashlib.sha256(b"original").hexdigest()},
            }))
            with patch("subprocess.run") as command:
                with self.assertRaisesRegex(SDK["SdkError"], "SDK contents changed"):
                    SDK["build_plugin"](root, root / "source", root / "build", root / "output")
                command.assert_not_called()

    def test_existing_plugin_output_is_preserved(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "manifest.json").write_text(json.dumps({"schemaVersion": 1, "files": {}}))
            output = root / "output"
            output.mkdir()
            installed = output / "plugin.so"
            installed.write_bytes(b"existing plugin")
            with patch("subprocess.run") as command:
                with self.assertRaisesRegex(SDK["SdkError"], "output already exists"):
                    SDK["build_plugin"](root, root / "source", root / "build", output)
                command.assert_not_called()
            self.assertEqual(installed.read_bytes(), b"existing plugin")

    def test_sdk_manifest_cannot_read_files_outside_the_package(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            sdk = root / "sdk"
            sdk.mkdir()
            (root / "external").write_bytes(b"outside")
            (sdk / "manifest.json").write_text(json.dumps({
                "schemaVersion": 1,
                "files": {"../external": hashlib.sha256(b"outside").hexdigest()},
            }))
            with patch("subprocess.run") as command:
                with self.assertRaisesRegex(SDK["SdkError"], "SDK contents changed"):
                    SDK["build_plugin"](sdk, root / "source", root / "build", root / "output")
                command.assert_not_called()

    def test_existing_output_symlink_is_preserved_even_when_its_target_is_absent(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "manifest.json").write_text(json.dumps({"schemaVersion": 1, "files": {}}))
            output = root / "output"
            output.symlink_to(root / "missing")
            with patch("subprocess.run") as command:
                with self.assertRaisesRegex(SDK["SdkError"], "output already exists"):
                    SDK["build_plugin"](root, root / "source", root / "build", output)
                command.assert_not_called()
            self.assertTrue(output.is_symlink())
            self.assertFalse((root / "missing").exists())

    def test_plugin_for_another_sdk_reports_how_to_rebuild(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            sdk, output = root / "sdk", root / "output"
            sdk.mkdir()
            output.mkdir()
            (sdk / "manifest.json").write_text(json.dumps({"schemaVersion": 1, "packageId": "current", "files": {}}))
            (output / "librepaint-plugin.json").write_text(json.dumps({"schemaVersion": 1, "sdk": "previous", "files": {}}))
            with self.assertRaisesRegex(SDK["SdkError"], "Rebuild with plugin-sdk build"):
                SDK["check_plugin"](sdk, output)

    def test_modified_plugin_output_rejects_compatibility_check(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            sdk, output = root / "sdk", root / "output"
            sdk.mkdir()
            output.mkdir()
            (sdk / "manifest.json").write_text(json.dumps({"schemaVersion": 1, "packageId": "current", "files": {}}))
            (output / "plugin.so").write_bytes(b"modified")
            (output / "librepaint-plugin.json").write_text(json.dumps({
                "schemaVersion": 1, "sdk": "current",
                "files": {"plugin.so": hashlib.sha256(b"original").hexdigest()},
            }))
            with self.assertRaisesRegex(SDK["SdkError"], "output changed"):
                SDK["check_plugin"](sdk, output)


if __name__ == "__main__":
    unittest.main()
