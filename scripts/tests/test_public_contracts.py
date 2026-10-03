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

    def test_plugin_registration_requires_unique_ids_and_known_service_types(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            plugin = root / "plugins/sample"
            plugin.mkdir(parents=True)
            (plugin / "sample.cpp").write_text(
                'K_PLUGIN_FACTORY_WITH_JSON(Sample, "sample.json", value)\n',
                encoding="utf-8",
            )
            metadata = {"Id": "sample", "X-KDE-ServiceTypes": ["Krita/Filter"]}
            (plugin / "sample.json").write_text(json.dumps(metadata), encoding="utf-8")
            self.assertEqual(check_public_contracts.validate_plugins(root), 1)

            metadata["X-KDE-ServiceTypes"] = ["Unknown/Service"]
            (plugin / "sample.json").write_text(json.dumps(metadata), encoding="utf-8")
            with self.assertRaisesRegex(
                check_public_contracts.PublicContractError, "invalid service type"
            ):
                check_public_contracts.validate_plugins(root)

            metadata["X-KDE-ServiceTypes"] = ["Krita/Filter"]
            (plugin / "sample.json").write_text(json.dumps(metadata), encoding="utf-8")
            (plugin / "other.json").write_text(json.dumps(metadata), encoding="utf-8")
            (plugin / "other.cpp").write_text(
                'K_PLUGIN_FACTORY_WITH_JSON(Other, "other.json", value)\n',
                encoding="utf-8",
            )
            with self.assertRaisesRegex(
                check_public_contracts.PublicContractError, "duplicate plugin id"
            ):
                check_public_contracts.validate_plugins(root)


if __name__ == "__main__":
    unittest.main()
