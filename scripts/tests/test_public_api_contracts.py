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
    def test_qt_registration_macros_do_not_consume_following_declarations(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            header = root / "PublicFlags.h"
            header.write_text(
                """
template<typename Enum>
class QFlags;

enum class PublicFlag
{
    First = 1
};

Q_DECLARE_FLAGS(PublicFlags, PublicFlag)
Q_DECLARE_METATYPE(PublicFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(PublicFlags)

void firstFunction();
void secondFunction();
""",
                encoding="utf-8",
            )

            tags = check_public_api_contracts.collect_ctags(
                root, ["PublicFlags.h"]
            )
            apis = check_public_api_contracts.extract_public_apis(
                tags, {"PublicFlags.h"}
            )

            self.assertIn("function:firstFunction()", [api["id"] for api in apis])
            self.assertIn("function:secondFunction()", [api["id"] for api in apis])

    def test_collects_qt_flag_declarations_as_aliases(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            header = root / "PublicFlags.h"
            header.write_text(
                """
template<typename Enum>
class QFlags;

enum class PublicFlag
{
    First = 1,
    Second = 2
};

Q_DECLARE_FLAGS(PublicFlags, PublicFlag)
""",
                encoding="utf-8",
            )

            tags = check_public_api_contracts.collect_ctags(
                root, ["PublicFlags.h"]
            )
            apis = check_public_api_contracts.extract_public_apis(
                tags, {"PublicFlags.h"}
            )

            self.assertIn(
                {
                    "header": "PublicFlags.h",
                    "id": "alias:PublicFlags",
                    "kind": "alias",
                    "symbol": "PublicFlags: QFlags<PublicFlag>",
                },
                apis,
            )
            self.assertFalse(
                any("Q_DECLARE_FLAGS" in api["id"] for api in apis)
            )

    def test_collects_overridden_public_destructor(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            header = root / "PublicWidget.h"
            header.write_text(
                """
class Base
{
public:
    virtual ~Base();
};

class PublicWidget : public Base
{
public:
    ~PublicWidget() override;
};
""",
                encoding="utf-8",
            )

            tags = check_public_api_contracts.collect_ctags(
                root, ["PublicWidget.h"]
            )
            apis = check_public_api_contracts.extract_public_apis(
                tags, {"PublicWidget.h"}
            )

            self.assertIn(
                "method:PublicWidget::~PublicWidget()",
                [api["id"] for api in apis],
            )

    def test_excludes_extern_template_instantiations_from_variables(self) -> None:
        header = "libs/example/PublicTemplate.h"
        tags = [
            {
                "_type": "tag",
                "name": "PublicTemplate",
                "path": header,
                "pattern": "/^extern template class EXAMPLE_EXPORT PublicTemplate<int>;$/",
                "kind": "externvar",
                "typeref": "class:EXAMPLE_EXPORT",
            },
            {
                "_type": "tag",
                "name": "publicValue",
                "path": header,
                "pattern": "/^extern int publicValue;$/",
                "kind": "externvar",
                "typeref": "typename:int",
            },
        ]

        apis = check_public_api_contracts.extract_public_apis(tags, {header})

        self.assertEqual(
            ["variable:publicValue"],
            [api["id"] for api in apis],
        )

    def test_collects_friend_function_declarations_as_namespace_functions(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            header = root / "PublicValue.h"
            header.write_text(
                """
namespace Example
{
class PublicValue
{
public:
    friend bool operator==(const PublicValue &lhs, const PublicValue &rhs);

private:
    friend int inspect(
        const PublicValue &value,
        int detail);
    friend class Helper;
};
}
""",
                encoding="utf-8",
            )

            tags = check_public_api_contracts.collect_ctags(
                root, ["PublicValue.h"]
            )
            apis = check_public_api_contracts.extract_public_apis(
                tags, {"PublicValue.h"}
            )

            self.assertIn(
                "function:Example::operator ==(const PublicValue & lhs,const PublicValue & rhs)",
                [api["id"] for api in apis],
            )
            self.assertIn(
                "function:Example::inspect(const PublicValue & value,int detail)",
                [api["id"] for api in apis],
            )
            self.assertFalse(any("Helper" in api["id"] for api in apis))

    def test_friend_declaration_transform_preserves_non_declarations(self) -> None:
        source = """
// friend bool commentedOut(int value);
const char *description = "friend bool textOnly(int value);";
friend class Helper;
friend bool declared(int value);
inline friend bool defined(int value) { return value > 0; }
"""

        transformed = (
            check_public_api_contracts._friend_declarations_with_empty_bodies(
                source
            )
        )

        self.assertIn("// friend bool commentedOut(int value);", transformed)
        self.assertIn('"friend bool textOnly(int value);"', transformed)
        self.assertIn("friend class Helper;", transformed)
        self.assertIn("friend bool declared(int value) {}", transformed)
        self.assertIn(
            "inline friend bool defined(int value) { return value > 0; }",
            transformed,
        )

    def test_collects_qt_signals_as_public_after_private_slots(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            header = root / "PublicWidget.h"
            header.write_text(
                """
class PublicWidget
{
public:
    void visible();

private Q_SLOTS:
    void internalUpdate();

Q_SIGNALS:
    void changed(int value);
    void reset();
};
""",
                encoding="utf-8",
            )

            tags = check_public_api_contracts.collect_ctags(
                root, ["PublicWidget.h"]
            )
            apis = check_public_api_contracts.extract_public_apis(
                tags, {"PublicWidget.h"}
            )

            self.assertEqual(
                [
                    "class:PublicWidget",
                    "method:PublicWidget::changed(int value)",
                    "method:PublicWidget::reset()",
                    "method:PublicWidget::visible()",
                ],
                [api["id"] for api in apis],
            )

    def test_qt_enum_registration_does_not_consume_following_method(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            header = root / "PublicWidget.h"
            header.write_text(
                """
class PublicWidget
{
public:
    enum Mode {
        First,
        Second
    };
    Q_ENUMS(Mode)

    static Mode modeFor(int value);
};
""",
                encoding="utf-8",
            )

            tags = check_public_api_contracts.collect_ctags(
                root, ["PublicWidget.h"]
            )
            apis = check_public_api_contracts.extract_public_apis(
                tags, {"PublicWidget.h"}
            )

            self.assertIn(
                "method:PublicWidget::modeFor(int value)",
                [api["id"] for api in apis],
            )

    def test_excludes_declarations_inside_non_public_records(self) -> None:
        header = "libs/example/PublicWidget.h"
        tags = [
            {
                "_type": "tag",
                "name": "PublicWidget",
                "path": header,
                "kind": "class",
                "end": 60,
            },
            {
                "_type": "tag",
                "name": "PrivateState",
                "path": header,
                "kind": "struct",
                "access": "private",
                "scope": "PublicWidget",
                "scopeKind": "class",
                "end": 20,
            },
            {
                "_type": "tag",
                "name": "value",
                "path": header,
                "kind": "member",
                "access": "public",
                "scope": "PublicWidget::PrivateState",
                "scopeKind": "struct",
                "typeref": "typename:int",
            },
            {
                "_type": "tag",
                "name": "NestedState",
                "path": header,
                "kind": "struct",
                "access": "public",
                "scope": "PublicWidget::PrivateState",
                "scopeKind": "struct",
                "end": 18,
            },
            {
                "_type": "tag",
                "name": "nestedValue",
                "path": header,
                "kind": "member",
                "access": "public",
                "scope": "PublicWidget::PrivateState::NestedState",
                "scopeKind": "struct",
                "typeref": "typename:int",
            },
            {
                "_type": "tag",
                "name": "VisibleState",
                "path": header,
                "kind": "struct",
                "access": "public",
                "scope": "PublicWidget",
                "scopeKind": "class",
                "end": 40,
            },
            {
                "_type": "tag",
                "name": "visibleValue",
                "path": header,
                "kind": "member",
                "access": "public",
                "scope": "PublicWidget::VisibleState",
                "scopeKind": "struct",
                "typeref": "typename:int",
            },
        ]

        apis = check_public_api_contracts.extract_public_apis(tags, {header})

        self.assertEqual(
            [
                "class:PublicWidget",
                "member:PublicWidget::VisibleState::visibleValue",
                "struct:PublicWidget::VisibleState",
            ],
            [api["id"] for api in apis],
        )

    def test_excludes_declarations_inside_routines(self) -> None:
        header = "libs/example/PublicValues.h"
        tags = [
            {
                "_type": "tag",
                "name": "nextValue",
                "path": header,
                "kind": "function",
                "signature": "(int value)",
                "typeref": "typename:int",
            },
            {
                "_type": "tag",
                "name": "value_type",
                "path": header,
                "kind": "typedef",
                "scope": "nextValue",
                "scopeKind": "function",
                "typeref": "typename:int",
            },
            {
                "_type": "tag",
                "name": "LocalState",
                "path": header,
                "kind": "struct",
                "scope": "nextValue",
                "scopeKind": "function",
                "end": 20,
            },
            {
                "_type": "tag",
                "name": "value",
                "path": header,
                "kind": "member",
                "access": "public",
                "scope": "nextValue::LocalState",
                "scopeKind": "struct",
                "typeref": "typename:int",
            },
        ]

        apis = check_public_api_contracts.extract_public_apis(tags, {header})

        self.assertEqual(
            ["function:nextValue(int value)"],
            [api["id"] for api in apis],
        )

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
