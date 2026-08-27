#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts/architecture/check_public_api_contracts.py"
SPEC = importlib.util.spec_from_file_location(
    "check_public_api_contracts", SCRIPT_PATH
)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_public_api_contracts = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_public_api_contracts
SPEC.loader.exec_module(check_public_api_contracts)


class PublicApiContractTests(unittest.TestCase):
    def test_extracts_only_owned_public_declarations(self) -> None:
        tags = [
            {
                "_type": "tag",
                "name": "PublicWidget",
                "path": "libs/example/PublicWidget.h",
                "kind": "class",
                "end": 40,
            },
            {
                "_type": "tag",
                "name": "value",
                "path": "libs/example/PublicWidget.h",
                "kind": "prototype",
                "access": "public",
                "scope": "Example::PublicWidget",
                "scopeKind": "class",
                "signature": "() const",
                "typeref": "typename:int",
            },
            {
                "_type": "tag",
                "name": "value",
                "path": "libs/example/PublicWidget.h",
                "kind": "slot",
                "access": "public",
                "scope": "Example::PublicWidget",
                "scopeKind": "class",
                "signature": "() const",
                "typeref": "typename:int",
            },
            {
                "_type": "tag",
                "name": "reset",
                "path": "libs/example/PublicWidget.h",
                "kind": "prototype",
                "access": "protected",
                "scope": "Example::PublicWidget",
                "scopeKind": "class",
                "signature": "()",
                "typeref": "typename:void",
            },
            {
                "_type": "tag",
                "name": "size",
                "path": "libs/example/PublicWidget.h",
                "kind": "member",
                "access": "public",
                "scope": "Example::PublicWidget",
                "scopeKind": "class",
                "typeref": "typename:int",
            },
            {
                "_type": "tag",
                "name": "state",
                "path": "libs/example/PublicWidget.h",
                "kind": "member",
                "access": "private",
                "scope": "Example::PublicWidget",
                "scopeKind": "class",
                "typeref": "typename:int",
            },
            {
                "_type": "tag",
                "name": "Mode",
                "path": "libs/example/PublicWidget.h",
                "kind": "enum",
                "access": "public",
                "scope": "Example::PublicWidget",
                "scopeKind": "class",
                "end": 25,
            },
            {
                "_type": "tag",
                "name": "Enabled",
                "path": "libs/example/PublicWidget.h",
                "kind": "enumerator",
                "access": "public",
                "scope": "Example::PublicWidget::Mode",
                "scopeKind": "enum",
            },
            {
                "_type": "tag",
                "name": "InternalMode",
                "path": "libs/example/PublicWidget.h",
                "kind": "enum",
                "access": "protected",
                "scope": "Example::PublicWidget",
                "scopeKind": "class",
                "end": 30,
            },
            {
                "_type": "tag",
                "name": "Internal",
                "path": "libs/example/PublicWidget.h",
                "kind": "enumerator",
                "access": "public",
                "scope": "Example::PublicWidget::InternalMode",
                "scopeKind": "enum",
            },
            {
                "_type": "tag",
                "name": "Q_ENUMS",
                "path": "libs/example/PublicWidget.h",
                "kind": "prototype",
                "access": "public",
                "scope": "Example::PublicWidget",
                "scopeKind": "class",
                "signature": "(Mode)",
            },
            {
                "_type": "tag",
                "name": "PublicWidgetList",
                "path": "libs/example/PublicWidget.h",
                "kind": "typedef",
                "typeref": "typename:QList<PublicWidget>",
            },
            {
                "_type": "tag",
                "name": "other",
                "path": "libs/example/Other.h",
                "kind": "prototype",
                "signature": "()",
                "typeref": "typename:void",
            },
        ]

        apis = check_public_api_contracts.extract_public_apis(
            tags, {"libs/example/PublicWidget.h"}
        )

        self.assertEqual(
            [
                {
                    "header": "libs/example/PublicWidget.h",
                    "id": "alias:PublicWidgetList",
                    "kind": "alias",
                    "symbol": "PublicWidgetList: QList<PublicWidget>",
                },
                {
                    "header": "libs/example/PublicWidget.h",
                    "id": "class:PublicWidget",
                    "kind": "class",
                    "symbol": "PublicWidget",
                },
                {
                    "header": "libs/example/PublicWidget.h",
                    "id": "enum:Example::PublicWidget::Mode",
                    "kind": "enum",
                    "symbol": "Example::PublicWidget::Mode",
                },
                {
                    "header": "libs/example/PublicWidget.h",
                    "id": "enumerator:Example::PublicWidget::Mode::Enabled",
                    "kind": "enumerator",
                    "symbol": "Example::PublicWidget::Mode::Enabled",
                },
                {
                    "header": "libs/example/PublicWidget.h",
                    "id": "member:Example::PublicWidget::size",
                    "kind": "member",
                    "symbol": "Example::PublicWidget::size: int",
                },
                {
                    "header": "libs/example/PublicWidget.h",
                    "id": "method:Example::PublicWidget::value() const",
                    "kind": "method",
                    "symbol": "Example::PublicWidget::value() const -> int",
                },
            ],
            apis,
        )

    def test_contract_requires_a_real_behavior_test(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            (root / "libs/example/tests").mkdir(parents=True)
            (root / "libs/example/tests/CMakeLists.txt").write_text(
                "kis_add_tests(PublicWidgetTest.cpp)\n", encoding="utf-8"
            )
            (root / "libs/example/tests/PublicWidgetTest.cpp").write_text(
                "void PublicWidgetTest::returnsStoredValue() {}\n",
                encoding="utf-8",
            )
            apis = [
                {
                    "header": "libs/example/PublicWidget.h",
                    "id": "method:PublicWidget::value() const",
                    "kind": "method",
                    "symbol": "PublicWidget::value() const -> int",
                }
            ]
            contracts = [
                {
                    "target": "PublicWidgetTest",
                    "source": "libs/example/tests/PublicWidgetTest.cpp",
                    "test": "PublicWidgetTest::returnsStoredValue",
                    "behavior": "格納した値を変更せずに返す。",
                    "classification": "maintained",
                    "apis": ["method:PublicWidget::value() const"],
                }
            ]

            covered = check_public_api_contracts.validate_contracts(
                root, apis, contracts
            )

            self.assertEqual({"method:PublicWidget::value() const"}, covered)

            invalid_contracts = [dict(contracts[0], behavior="")]
            with self.assertRaisesRegex(
                check_public_api_contracts.PublicApiContractError,
                "behavior",
            ):
                check_public_api_contracts.validate_contracts(
                    root, apis, invalid_contracts
                )

    def test_registry_detects_public_api_and_missing_count_drift(self) -> None:
        apis = [
            {
                "header": "libs/example/PublicWidget.h",
                "id": "class:PublicWidget",
                "kind": "class",
                "symbol": "PublicWidget",
            }
        ]
        headers = ["libs/example/PublicWidget.h"]
        registry = {
            "schemaVersion": 1,
            "scope": {
                "publicHeaderCount": 1,
                "publicHeaderFingerprint": (
                    check_public_api_contracts.fingerprint_public_headers(headers)
                ),
                "publicApiCount": 1,
                "publicApiFingerprint": (
                    check_public_api_contracts.fingerprint_public_apis(apis)
                ),
            },
            "migration": {"expectedMissingApis": 1},
            "contracts": [],
        }

        result = check_public_api_contracts.validate_registry(
            Path.cwd(), headers, apis, registry
        )
        self.assertEqual(0, result["covered"])
        self.assertEqual(1, result["missing"])

        registry["scope"]["publicApiCount"] = 2
        with self.assertRaisesRegex(
            check_public_api_contracts.PublicApiContractError,
            "public API inventory changed",
        ):
            check_public_api_contracts.validate_registry(
                Path.cwd(), headers, apis, registry
            )


if __name__ == "__main__":
    unittest.main()
