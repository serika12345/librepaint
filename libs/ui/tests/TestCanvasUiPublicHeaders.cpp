/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <type_traits>

#include <QTest>
#include <QWidget>

#include <animation/kis_animation_frame_cache_fwd.h>
#include <animation/kis_animation_frame_cache.h>
#include <canvas/KisWidgetWithIdleTask.h>

static_assert(std::is_base_of_v<QWidget, KisWidgetWithIdleTask<QWidget>>);

class TestCanvasUiPublicHeaders : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void headersCompileAsOnePublicSurface();
};

void TestCanvasUiPublicHeaders::headersCompileAsOnePublicSurface()
{
    QVERIFY(true);
}

QTEST_MAIN(TestCanvasUiPublicHeaders)

#include "TestCanvasUiPublicHeaders.moc"
