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


if __name__ == "__main__":
    unittest.main()
