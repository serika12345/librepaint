#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import json
import plistlib
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


DEFAULT_HELPER = (
    Path(__file__).resolve().parents[1]
    / "scripts"
    / "audit-ios-compatibility-identifiers.py"
)
if len(sys.argv) > 1 and sys.argv[1].endswith(".py"):
    HELPER = Path(sys.argv.pop(1)).resolve()
else:
    HELPER = DEFAULT_HELPER


class CompatibilityIdentifierAuditTest(unittest.TestCase):
    def write_text(self, root: Path, relative: str, text: str) -> Path:
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return path

    def write_json(self, root: Path, relative: str, value: object) -> Path:
        return self.write_text(
            root,
            relative,
            json.dumps(value, ensure_ascii=False, indent=2) + "\n",
        )

    def run_helper(self, root: Path, manifest: Path, *extra: str, succeeds: bool = True):
        result = subprocess.run(
            [
                sys.executable,
                str(HELPER),
                "--source-root",
                str(root),
                "--manifest",
                str(manifest),
                *extra,
            ],
            check=False,
            capture_output=True,
            text=True,
        )
        if succeeds:
            self.assertEqual(result.returncode, 0, result.stderr)
        else:
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertNotIn("Traceback", result.stderr)
        return result

    def generate_manifest(self, root: Path, manifest: Path) -> None:
        result = self.run_helper(
            root, manifest, "--print-current-manifest", succeeds=True
        )
        value = json.loads(result.stdout)
        manifest.write_text(
            json.dumps(value, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )

    def make_fixture(self) -> tuple[Path, Path]:
        temporary = tempfile.TemporaryDirectory(prefix="compatibility-audit-test.")
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)

        info = {
            "CFBundleDocumentTypes": [
                {"LSItemContentTypes": ["org.krita.kra"]},
                {"LSItemContentTypes": ["org.krita.openraster"]},
                {"LSItemContentTypes": ["public.png"]},
            ],
            "UTExportedTypeDeclarations": [
                {
                    "UTTypeIdentifier": "org.krita.kra",
                    "UTTypeTagSpecification": {
                        "public.filename-extension": ["kra"],
                        "public.mime-type": "application/x-krita",
                    },
                }
            ],
            "UTImportedTypeDeclarations": [
                {
                    "UTTypeIdentifier": "org.krita.openraster",
                    "UTTypeTagSpecification": {
                        "public.filename-extension": ["ora"],
                        "public.mime-type": "image/openraster",
                    },
                }
            ],
        }
        plist_path = root / "krita/Info.ios.plist.in"
        plist_path.parent.mkdir(parents=True)
        with plist_path.open("wb") as handle:
            plistlib.dump(info, handle)

        self.write_text(
            root,
            "krita/data/mime-database/freedesktop.org.xml",
            """<?xml version="1.0"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <mime-type type="application/x-krita">
    <comment>Visible product description is not an identifier</comment>
    <glob pattern="*.kra"/>
  </mime-type>
</mime-info>
""",
        )

        common_plugin = {
            "Name": "Visible Plugin Name",
            "Description": "Visible plugin description",
            "X-KDE-ServiceTypes": ["Krita/FileFilter"],
            "X-Krita-Version": "28",
        }
        kra_import = {
            **common_plugin,
            "Id": "Krita Native Import Filter",
            "X-KDE-Library": "kritakraimport",
            "X-KDE-Import": "application/x-krita,application/x-krita-archive",
            "X-KDE-Extensions": "kra, krz",
        }
        kra_export = {
            **common_plugin,
            "Id": "Krita Native Export Filter",
            "X-KDE-Library": "kritakraexport",
            "X-KDE-Export": "application/x-krita",
            "X-KDE-Extensions": "kra",
        }
        krz_export = {
            **common_plugin,
            "Id": "Krita Archival Export Filter",
            "X-KDE-Library": "kritakrzexport",
            "X-KDE-Export": "application/x-krita-archive",
            "X-KDE-Extensions": "krz",
        }
        self.write_json(
            root, "plugins/impex/kra/krita_kra_import.json", kra_import
        )
        self.write_json(
            root, "plugins/impex/kra/krita_kra_export.json", kra_export
        )
        self.write_json(
            root, "plugins/impex/krz/krita_krz_export.json", krz_export
        )
        self.write_json(
            root,
            "plugins/example/example.json",
            {
                "Id": "Stable Example Plugin",
                "Name": "Visible Example Plugin",
                "X-KDE-Library": "kritaexample",
                "X-KDE-ServiceTypes": ["Krita/Filter"],
                "X-Krita-Version": "28",
                "X-Krita-PlatformId": "org.krita.example",
            },
        )

        self.write_text(
            root,
            "krita/main.cc",
            """
// KLocalizedString::setApplicationDomain("comment-is-ignored");
KLocalizedString::addDomainLocaleDir("krita", QDir(root + "share/locale").absolutePath());
KLocalizedString::setApplicationDomain("krita");
KAboutData aboutData("krita", i18n("Visible Product Name"), QStringLiteral("1"));
const auto displayConfig = QStringLiteral("/kritadisplayrc");
""",
        )
        self.write_text(
            root,
            "krita/CMakeLists.txt",
            """
if(IOS)
    set_target_properties(krita PROPERTIES
        OUTPUT_NAME "LibrePaint"
        MACOSX_BUNDLE_GUI_IDENTIFIER "local.librepaint.ipad"
    )
endif()
""",
        )
        self.write_text(
            root,
            "krita/krita.qrc",
            """<RCC>
  <qresource prefix="/kconfig">
    <file alias="kritarc">data/kritarc</file>
  </qresource>
</RCC>
""",
        )
        self.write_text(root, "krita/data/kritarc", "[General]\nExample=true\n")
        self.write_text(
            root,
            "libs/ui/config.cpp",
            """
const auto defaultConfig = QStringLiteral("kritarc");
const auto displayConfig = QStringLiteral("/kritadisplayrc");
const auto resources = QStringLiteral("/share/krita");
// QStringLiteral("/share/comment-is-ignored");
""",
        )

        self.write_text(
            root,
            "krita/core.action",
            """<ActionCollection version="2" name="core">
  <Actions category="file">
    <text>Visible category</text>
    <Action name="file_save"><text>Visible Save</text></Action>
    <Action name="file_open"><text>Visible Open</text></Action>
  </Actions>
</ActionCollection>
""",
        )
        self.write_text(
            root,
            "plugins/example/example.action",
            """<ActionCollection version="2" name="example">
  <Actions category="example">
    <text>Visible example category</text>
    <Action name="stable_example_action"><text>Visible Action</text></Action>
  </Actions>
</ActionCollection>
""",
        )

        manifest = root / "compatibility-identifiers.json"
        self.generate_manifest(root, manifest)
        return root, manifest

    def load_json(self, path: Path):
        return json.loads(path.read_text(encoding="utf-8"))

    def test_baseline_fixture_passes(self):
        root, manifest = self.make_fixture()
        result = self.run_helper(root, manifest)
        self.assertIn("iOS compatibility identifier audit:", result.stdout)

    def test_unknown_schema_and_manifest_shape_are_rejected(self):
        for mutation, expected_error in (
            (lambda value: value.__setitem__("schema", 99), "unsupported"),
            (
                lambda value: value["inventories"].pop("action_ids"),
                "inventory categories changed",
            ),
        ):
            with self.subTest(expected_error=expected_error):
                root, manifest = self.make_fixture()
                value = self.load_json(manifest)
                mutation(value)
                manifest.write_text(json.dumps(value) + "\n", encoding="utf-8")
                result = self.run_helper(root, manifest, succeeds=False)
                self.assertIn(expected_error, result.stderr)

    def test_kra_mime_uti_and_extension_changes_are_rejected(self):
        mutations = (
            (
                "krita/Info.ios.plist.in",
                b"org.krita.kra",
                b"org.librepaint.kra",
            ),
            (
                "krita/data/mime-database/freedesktop.org.xml",
                b"application/x-krita",
                b"application/x-librepaint",
            ),
            (
                "plugins/impex/kra/krita_kra_export.json",
                b'"kra"',
                b'"lpa"',
            ),
        )
        for relative, old, new in mutations:
            with self.subTest(relative=relative):
                root, manifest = self.make_fixture()
                path = root / relative
                data = path.read_bytes()
                self.assertIn(old, data)
                path.write_bytes(data.replace(old, new, 1))
                self.run_helper(root, manifest, succeeds=False)

    def test_application_config_and_resource_identity_changes_are_rejected(self):
        mutations = (
            (
                "krita/main.cc",
                'setApplicationDomain("krita")',
                'setApplicationDomain("librepaint")',
            ),
            (
                "krita/CMakeLists.txt",
                'MACOSX_BUNDLE_GUI_IDENTIFIER "local.librepaint.ipad"',
                'MACOSX_BUNDLE_GUI_IDENTIFIER "local.librepaint.renamed"',
            ),
            (
                "libs/ui/config.cpp",
                'QStringLiteral("kritarc")',
                'QStringLiteral("librepaintrc")',
            ),
            (
                "libs/ui/config.cpp",
                'QStringLiteral("/share/krita")',
                'QStringLiteral("/share/librepaint")',
            ),
        )
        for relative, old, new in mutations:
            with self.subTest(relative=relative, old=old):
                root, manifest = self.make_fixture()
                path = root / relative
                text = path.read_text(encoding="utf-8")
                self.assertIn(old, text)
                path.write_text(text.replace(old, new, 1), encoding="utf-8")
                self.run_helper(root, manifest, succeeds=False)

    def test_ios_output_name_is_not_a_compatibility_identifier(self):
        root, manifest = self.make_fixture()
        path = root / "krita/CMakeLists.txt"
        text = path.read_text(encoding="utf-8")
        path.write_text(
            text.replace('OUTPUT_NAME "LibrePaint"', 'OUTPUT_NAME "RenamedArtifact"'),
            encoding="utf-8",
        )
        self.run_helper(root, manifest)

    def test_plugin_stable_metadata_changes_are_rejected(self):
        mutations = (
            ("Id", "Renamed Example Plugin"),
            ("X-KDE-ServiceTypes", ["LibrePaint/Filter"]),
            ("X-Krita-Version", "29"),
            ("X-Krita-PlatformId", "org.librepaint.example"),
        )
        for key, replacement in mutations:
            with self.subTest(key=key):
                root, manifest = self.make_fixture()
                path = root / "plugins/example/example.json"
                value = self.load_json(path)
                value[key] = replacement
                self.write_json(root, "plugins/example/example.json", value)
                result = self.run_helper(root, manifest, succeeds=False)
                self.assertIn("plugin_metadata changed", result.stderr)

    def test_plugin_display_and_internal_library_changes_are_not_fixed(self):
        root, manifest = self.make_fixture()
        path = root / "plugins/example/example.json"
        value = self.load_json(path)
        value["Name"] = "A completely different visible name"
        value["Description"] = "A completely different visible description"
        value["X-KDE-Library"] = "librepaint-example-internal-target"
        self.write_json(root, "plugins/example/example.json", value)
        self.run_helper(root, manifest)

    def test_action_identifier_missing_added_and_changed_are_rejected(self):
        for mutation in ("missing", "added", "changed"):
            with self.subTest(mutation=mutation):
                root, manifest = self.make_fixture()
                path = root / "krita/core.action"
                text = path.read_text(encoding="utf-8")
                if mutation == "missing":
                    text = text.replace(
                        '    <Action name="file_open"><text>Visible Open</text></Action>\n',
                        "",
                    )
                elif mutation == "added":
                    text = text.replace(
                        "  </Actions>",
                        '    <Action name="new_action"><text>New</text></Action>\n  </Actions>',
                    )
                else:
                    text = text.replace('name="file_save"', 'name="file_store"')
                path.write_text(text, encoding="utf-8")
                result = self.run_helper(root, manifest, succeeds=False)
                self.assertIn("action_ids changed", result.stderr)

    def test_action_display_text_is_not_fixed(self):
        root, manifest = self.make_fixture()
        path = root / "krita/core.action"
        text = path.read_text(encoding="utf-8")
        path.write_text(
            text.replace("Visible Save", "A different translated display label"),
            encoding="utf-8",
        )
        self.run_helper(root, manifest)

    def test_added_or_removed_plugin_file_is_rejected(self):
        for mutation in ("added", "removed"):
            with self.subTest(mutation=mutation):
                root, manifest = self.make_fixture()
                if mutation == "added":
                    self.write_json(
                        root,
                        "plugins/example/added.json",
                        {
                            "Id": "New Stable Plugin",
                            "X-KDE-ServiceTypes": ["Krita/Filter"],
                            "X-Krita-Version": "28",
                        },
                    )
                else:
                    (root / "plugins/example/example.json").unlink()
                result = self.run_helper(root, manifest, succeeds=False)
                self.assertIn("plugin_metadata changed", result.stderr)

    def test_missing_structural_source_is_rejected(self):
        root, manifest = self.make_fixture()
        (root / "krita/krita.qrc").unlink()
        result = self.run_helper(root, manifest, succeeds=False)
        self.assertIn("required compatibility source is missing", result.stderr)


if __name__ == "__main__":
    unittest.main()
