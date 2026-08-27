#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/check_public_contracts.py"
SPEC = importlib.util.spec_from_file_location("check_public_contracts", SCRIPT_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_public_contracts = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_public_contracts
SPEC.loader.exec_module(check_public_contracts)


class PublicContractTests(unittest.TestCase):
    def test_exported_headers_are_in_the_public_api_scope(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            (root / "libs/example").mkdir(parents=True)
            (root / "libs/example/Public.h").write_text(
                "class EXAMPLE_EXPORT Public {};\n", encoding="utf-8"
            )
            (root / "libs/example/Internal.h").write_text(
                "class Internal {};\n", encoding="utf-8"
            )
            (root / "libs/example/HeaderOnly.h").write_text(
                "class HeaderOnly {};\n", encoding="utf-8"
            )
            (root / "libs/consumer").mkdir(parents=True)
            (root / "libs/consumer/use.cpp").write_text(
                '#include "HeaderOnly.h"\n', encoding="utf-8"
            )
            with mock.patch.object(
                check_public_contracts, "PUBLIC_HEADER_COMPILE_CONTRACTS", {}
            ):
                self.assertEqual(
                    [
                        "libs/example/HeaderOnly.h",
                        "libs/example/Public.h",
                    ],
                    check_public_contracts.discover_public_headers(root),
                )

    def test_external_header_without_publication_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            (root / "libs/example").mkdir(parents=True)
            (root / "libs/consumer").mkdir(parents=True)
            (root / "libs/example/Public.h").write_text(
                "class Public {};\n", encoding="utf-8"
            )
            (root / "libs/consumer/use.cpp").write_text(
                '#include "Public.h"\n', encoding="utf-8"
            )
            owners = (("libs/example", ("libs/example",), (), "EXAMPLE_EXPORT"),)
            with mock.patch.object(
                check_public_contracts, "PUBLIC_HEADER_OWNERS", owners
            ), mock.patch.object(
                check_public_contracts, "PUBLIC_HEADER_COMPILE_CONTRACTS", {}
            ):
                with self.assertRaisesRegex(
                    check_public_contracts.PublicContractError,
                    "external internal-header references",
                ):
                    check_public_contracts.validate_public_headers(root)

    def test_plugin_owner_requires_cmake_source_membership(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            plugin = root / "plugins/sample"
            plugin.mkdir(parents=True)
            (plugin / "sample.cpp").write_text(
                'K_PLUGIN_FACTORY_WITH_JSON(Sample, "sample.json", value)\n',
                encoding="utf-8",
            )
            (plugin / "sample.json").write_text(
                json.dumps(
                    {
                        "Id": "sample",
                        "X-KDE-ServiceTypes": ["Krita/Filter"],
                        "X-KDE-Library": "sampleplugin",
                    }
                ),
                encoding="utf-8",
            )
            (plugin / "CMakeLists.txt").write_text(
                "kis_add_library(otherplugin MODULE sample.cpp)\n",
                encoding="utf-8",
            )
            with mock.patch.object(
                check_public_contracts, "PLUGIN_OWNER_TARGET_OVERRIDES", {}
            ):
                with self.assertRaisesRegex(
                    check_public_contracts.PublicContractError,
                    "CMake source membership",
                ):
                    check_public_contracts.validate_plugins(root)

    def test_quick_verification_runs_the_direct_contract(self) -> None:
        verify_quick = (REPO_ROOT / "scripts/verify-quick").read_text(
            encoding="utf-8"
        )
        self.assertIn("check_public_contracts.py", verify_quick)
        self.assertIn("check_public_api_contracts.py", verify_quick)


if __name__ == "__main__":
    unittest.main()
