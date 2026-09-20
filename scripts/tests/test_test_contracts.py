#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT_PATH = (
    Path(__file__).resolve().parents[1] / "architecture" / "check_test_contracts.py"
)
REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("check_test_contracts", SCRIPT_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"cannot import {SCRIPT_PATH}")
check_test_contracts = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = check_test_contracts
SPEC.loader.exec_module(check_test_contracts)


class TestContractCheckTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        (self.root / "libs/example/tests").mkdir(parents=True)

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def write_test(self, name: str, source: str) -> Path:
        path = self.root / "libs/example/tests" / name
        path.write_text(source, encoding="utf-8")
        return path

    def findings(self) -> list[str]:
        return check_test_contracts.collect_findings(self.root)

    def test_behavioral_contract_accepts_observable_assertion(self) -> None:
        self.write_test(
            "StrokeContractTest.cpp",
            "void test() { stroke.start(); QVERIFY(stroke.isActive()); }\n",
        )

        self.assertEqual([], self.findings())

    def test_contract_rejects_type_traits_and_compile_time_shape(self) -> None:
        self.write_test(
            "StrokeContractTest.cpp",
            "#include <type_traits>\n"
            "static_assert(std::is_class_v<Stroke>);\n",
        )

        findings = self.findings()

        self.assertTrue(any("type-trait header" in item for item in findings))
        self.assertTrue(any("type-trait assertion" in item for item in findings))
        self.assertTrue(any("compile-time shape assertion" in item for item in findings))

    def test_contract_rejects_exact_member_signature_alias(self) -> None:
        self.write_test(
            "StrokeContractTest.cpp",
            "using StartSignature = void (Stroke::*)(int) const;\n",
        )

        findings = self.findings()

        self.assertTrue(any("signature alias" in item for item in findings))
        self.assertTrue(any("member-function pointer alias" in item for item in findings))

    def test_compatibility_test_requires_consumer_and_stable_property(self) -> None:
        self.write_test(
            "PluginCompatibilityTest.cpp",
            "static_assert(pluginAbi == 4);\n",
        )

        self.assertTrue(
            any("Compatibility requirement:" in item for item in self.findings())
        )

    def test_documented_compatibility_requirement_is_accepted(self) -> None:
        self.write_test(
            "PluginCompatibilityTest.cpp",
            "// Compatibility requirement: third-party plugins consume ABI version 4.\n"
            "static_assert(pluginAbi == 4);\n",
        )

        self.assertEqual([], self.findings())

    def test_quick_verification_runs_the_contract_check(self) -> None:
        verify_quick = (REPOSITORY_ROOT / "scripts/verify-quick").read_text(
            encoding="utf-8"
        )

        self.assertIn("check_test_contracts.py", verify_quick)


if __name__ == "__main__":
    unittest.main()
