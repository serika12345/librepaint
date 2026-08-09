#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[1] / "scripts/audit-user-visible-branding.py"
SPEC = importlib.util.spec_from_file_location("audit_user_visible_branding", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
AUDIT = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = AUDIT
SPEC.loader.exec_module(AUDIT)


def pot_entry(message: str, *references: str) -> str:
    return "\n".join(
        [
            f"#: {' '.join(references)}",
            f"msgid {json.dumps(message)}",
            'msgstr ""',
            "",
            "",
        ]
    )


class BrandingAuditTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def write(self, relative: str, text: str) -> Path:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return path

    def run_audit(self, pot: str = ""):
        completed = subprocess.CompletedProcess([], 0, pot, "")
        with mock.patch.object(AUDIT.subprocess, "run", return_value=completed) as runner:
            diagnostics, stats = AUDIT.audit(self.root, "/fixture/xgettext")
        return diagnostics, stats, runner

    def test_xgettext_msgid_and_exact_cli_are_audited(self) -> None:
        self.write("app.cpp", 'auto text = i18n("Open in Krita");\n')
        diagnostics, _stats, runner = self.run_audit(
            pot_entry("Open in Krita", "app.cpp:1")
        )
        self.assertEqual([(item.path, item.line) for item in diagnostics], [("app.cpp", 1)])
        command = runner.call_args.args[0]
        self.assertEqual(command[0], "/fixture/xgettext")
        self.assertIn("--files-from=-", command)

    def test_only_the_two_reasoned_suppressions_are_allowed(self) -> None:
        self.write(
            "app.cpp",
            "// LIBREPAINT-BRAND-AUDIT: upstream-attribution\n"
            'auto credit = i18n("Derived from Krita");\n'
            "// LIBREPAINT-BRAND-AUDIT: format-history\n"
            'auto history = i18n("Created by Krita 4");\n',
        )
        diagnostics, stats, _runner = self.run_audit(
            pot_entry("Derived from Krita", "app.cpp:2")
            + pot_entry("Created by Krita 4", "app.cpp:4")
        )
        self.assertEqual(diagnostics, [])
        self.assertEqual(stats.allowed_suppressions, 2)

    def test_unknown_and_stale_suppressions_fail_at_the_marker_line(self) -> None:
        self.write(
            "app.cpp",
            "// LIBREPAINT-BRAND-AUDIT: format-history\n"
            'auto current = i18n("LibrePaint");\n'
            "// LIBREPAINT-BRAND-AUDIT: typo\n"
            'auto legacy = i18n("Krita");\n',
        )
        diagnostics, _stats, _runner = self.run_audit(
            pot_entry("LibrePaint", "app.cpp:2") + pot_entry("Krita", "app.cpp:4")
        )
        actual = {(item.line, item.message) for item in diagnostics}
        self.assertIn((1, "stale branding suppression (format-history)"), actual)
        self.assertIn((3, "unknown branding suppression reason: typo"), actual)
        self.assertIn((4, "legacy product name in user-visible msgid"), actual)

    def test_merged_msgid_requires_suppression_at_every_reference(self) -> None:
        self.write(
            "first.cpp",
            "// LIBREPAINT-BRAND-AUDIT: format-history\n"
            'auto text = i18n("Krita 4");\n',
        )
        self.write("second.cpp", 'auto text = i18n("Krita 4");\n')
        diagnostics, _stats, _runner = self.run_audit(
            pot_entry("Krita 4", "first.cpp:2", "second.cpp:1")
        )
        self.assertEqual(
            [(item.path, item.line) for item in diagnostics], [("second.cpp", 1)]
        )

    def test_ui_action_and_xmlgui_display_elements_are_structural(self) -> None:
        self.write(
            "dialog.ui",
            "<ui>\n"
            "  <string notr=\"true\">Krita internal token</string>\n"
            "  <string>Welcome to Krita</string>\n"
            "  <string extracomment=\"LIBREPAINT-BRAND-AUDIT: format-history\">"
            "Krita 4 document</string>\n"
            "</ui>\n",
        )
        self.write(
            "menu.action",
            "<ActionCollection name=\"Krita/Internal\">\n"
            "  <Action name=\"krita_action\"><text>About Krita</text>"
            "<statusTip>LibrePaint status</statusTip></Action>\n"
            "</ActionCollection>\n",
        )
        self.write(
            "shell.xmlgui",
            "<gui name=\"Krita\"><Menu name=\"krita_menu\"><title>Krita Tools</title>"
            "</Menu></gui>\n",
        )
        diagnostics, stats, _runner = self.run_audit()
        actual = {(item.path, item.line) for item in diagnostics}
        self.assertEqual(
            actual,
            {("dialog.ui", 3), ("menu.action", 2), ("shell.xmlgui", 1)},
        )
        self.assertEqual(stats.allowed_suppressions, 1)

    def test_ui_extracomment_unknown_and_stale_reasons_fail(self) -> None:
        self.write(
            "dialog.ui",
            "<ui>\n"
            " <string extracomment=\"LIBREPAINT-BRAND-AUDIT: typo\">Krita</string>\n"
            " <string extracomment=\"LIBREPAINT-BRAND-AUDIT: format-history\">LibrePaint</string>\n"
            "</ui>\n",
        )
        diagnostics, _stats, _runner = self.run_audit()
        actual = {(item.line, item.message) for item in diagnostics}
        self.assertIn((2, "unknown branding suppression reason: typo"), actual)
        self.assertIn((2, "legacy product name in user-visible <string>"), actual)
        self.assertIn((3, "stale branding suppression (format-history)"), actual)

    def test_multiline_msgid_is_checked_but_msgctxt_and_literals_are_not(self) -> None:
        self.write(
            "app.cpp",
            'auto identity = QStringLiteral("Krita/Internal");\n'
            'auto visible = i18nc("Krita compatibility context", "Open in Krita");\n',
        )
        pot = (
            "#: app.cpp:2\n"
            'msgctxt "Krita compatibility context"\n'
            'msgid ""\n'
            '"Open in "\n'
            '"Krita"\n'
            'msgstr ""\n\n'
        )
        diagnostics, _stats, _runner = self.run_audit(pot)
        self.assertEqual(
            [(item.path, item.line) for item in diagnostics], [("app.cpp", 2)]
        )

    def test_plugin_display_fields_fail_but_compatibility_metadata_passes(self) -> None:
        self.write(
            "plugins/identity.json",
            json.dumps(
                {
                    "Id": "Krita Import Filter",
                    "X-KDE-PluginInfo-Name": "Krita Compatibility Identity",
                    "X-KDE-ServiceTypes": ["Krita/FileFilter"],
                    "X-Krita-Version": "28",
                },
                indent=2,
            ),
        )
        self.write(
            "plugins/display.json",
            '{\n  "Id": "stable",\n  "Name": "Krita Filter",\n'
            '  "KPlugin": {"Description": "Works with Krita"}\n}\n',
        )
        diagnostics, _stats, _runner = self.run_audit()
        self.assertEqual(
            [(item.path, item.line) for item in diagnostics],
            [("plugins/display.json", 3), ("plugins/display.json", 4)],
        )


if __name__ == "__main__":
    unittest.main()
