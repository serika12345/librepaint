/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <input/ui/KisPopupWidgetInterface.h>
#include <events/kis_cursor_override_hijacker.h>
#include <resources/kis_favorite_resource_manager.h>
#include <resources/kis_popup_palette.h>
#include <theme/KisUiFont.h>

class TestRemainingUiRootPublicHeaders : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void headersCompileAsOnePublicSurface();
};

void TestRemainingUiRootPublicHeaders::headersCompileAsOnePublicSurface()
{
    QVERIFY(true);
}

QTEST_MAIN(TestRemainingUiRootPublicHeaders)

#include "TestRemainingUiRootPublicHeaders.moc"
