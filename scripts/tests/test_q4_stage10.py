#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-3.0-or-later

"""Q4 stage 10 contracts for the Windows changes that need no device."""

from __future__ import annotations

import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


def read_source(relative: str) -> str:
    return (REPO_ROOT / relative).read_text(encoding="utf-8", errors="replace")


class Q4Stage10Test(unittest.TestCase):
    def test_tab_title_follows_the_active_view(self):
        source = read_source("libs/application/ui/workspace/KisMainWindow.cpp")
        self.assertIn("&QWidget::windowTitleChanged", source)
        self.assertIn("updateWindowTitleForActiveView", source)
        self.assertIn("setWindowTitle(d->activeView->windowTitle());", source)

    def test_windows_uses_the_unbalanced_key_workaround(self):
        source = read_source("libs/input/ui/kis_input_manager_p.cpp")
        assignment = source.index("useUnbalancedKeyPressEventWorkaround = true;")
        block = source[max(0, assignment - 700) : assignment]
        self.assertIn("Q_OS_WIN", block)
        self.assertIn("Q_OS_LINUX", block)
        self.assertIn("Q_OS_MACOS", block)

    def test_accessibility_is_configured_only_at_startup(self):
        source = read_source("krita/main.cc")
        setter = source.index('qputenv("QT_DISABLE_ACCESSIBILITY", "1")')
        application = source.index("KisApplication app(key, argc, argv)")
        self.assertLess(setter, application)
        self.assertEqual(source.count('qputenv("QT_DISABLE_ACCESSIBILITY"'), 1)

    def test_angle_resize_workarounds_are_gone(self):
        source = read_source("krita/main.cc")
        for name in (
            "QT_USE_PREMATURE_RESIZE_EVENTS",
            "QT_ANGLE_MANUALLY_UPDATE_SURFACE_SIZE",
            "QT_PREFILL_RHI_SURFACE",
        ):
            self.assertNotIn(name, source)

    def test_qscroller_detection_is_gone(self):
        files = [REPO_ROOT / "CMakeLists.txt"]
        files.extend(sorted((REPO_ROOT / "cmake").rglob("*.cmake")))
        for path in files:
            text = path.read_text(encoding="utf-8", errors="replace")
            self.assertNotIn("KRITA_QSCROLLER_PATCH", text, str(path))
            self.assertNotIn("QScroller", text, str(path))

    def test_fork_update_environment_variables_are_gone(self):
        removed = (
            "QT_BACKING_STORE_USE_FAST_QIMAGE_TRANSFER",
            "QT_FRAME_RATE_OVERRIDE",
        )
        files = [REPO_ROOT / "CMakeLists.txt"]
        for root_name in ("krita", "libs", "nix", "plugins", "cmake"):
            root = REPO_ROOT / root_name
            files.extend(
                path
                for path in root.rglob("*")
                if path.is_file()
                and path.suffix
                in {".c", ".cc", ".cmake", ".cpp", ".h", ".hpp", ".nix", ".txt"}
            )

        failures = []
        for path in files:
            text = path.read_text(encoding="utf-8", errors="replace")
            for name in removed:
                if name in text:
                    failures.append(f"{path.relative_to(REPO_ROOT)}: {name}")
        self.assertEqual(
            failures,
            [],
            "fork-only update environment variables remain:\n" + "\n".join(failures),
        )


if __name__ == "__main__":
    unittest.main()
