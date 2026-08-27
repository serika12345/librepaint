/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KisAnimationRender.h>

class KisAnimationRenderDimensionsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void detectsEvenWidthAndHeight_data();
    void detectsEvenWidthAndHeight();
    void videoFormatAndRenderModeDetermineRequirement_data();
    void videoFormatAndRenderModeDetermineRequirement();
};

void KisAnimationRenderDimensionsTest::detectsEvenWidthAndHeight_data()
{
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");
    QTest::addColumn<bool>("expected");

    QTest::newRow("zero") << 0 << 0 << true;
    QTest::newRow("positive-even") << 1920 << 1080 << true;
    QTest::newRow("negative-even") << -2 << -4 << true;
    QTest::newRow("odd-width") << 1919 << 1080 << false;
    QTest::newRow("odd-height") << 1920 << 1079 << false;
    QTest::newRow("both-odd") << 1 << 1 << false;
}

void KisAnimationRenderDimensionsTest::detectsEvenWidthAndHeight()
{
    QFETCH(int, width);
    QFETCH(int, height);
    QFETCH(bool, expected);

    QCOMPARE(KisAnimationRender::hasEvenDimensions(width, height), expected);
}

void KisAnimationRenderDimensionsTest::videoFormatAndRenderModeDetermineRequirement_data()
{
    QTest::addColumn<QString>("videoType");
    QTest::addColumn<KisAnimationRenderingOptions::RenderMode>("renderMode");
    QTest::addColumn<bool>("expected");

    QTest::newRow("mp4-video") << QStringLiteral("video/mp4") << KisAnimationRenderingOptions::RENDER_VIDEO_ONLY
                               << true;
    QTest::newRow("matroska-video") << QStringLiteral("video/x-matroska")
                                    << KisAnimationRenderingOptions::RENDER_VIDEO_ONLY << true;
    QTest::newRow("mp4-frames-and-video")
        << QStringLiteral("video/mp4") << KisAnimationRenderingOptions::RENDER_FRAMES_AND_VIDEO << true;
    QTest::newRow("mp4-frames-only") << QStringLiteral("video/mp4") << KisAnimationRenderingOptions::RENDER_FRAMES_ONLY
                                     << false;
    QTest::newRow("matroska-frames-only")
        << QStringLiteral("video/x-matroska") << KisAnimationRenderingOptions::RENDER_FRAMES_ONLY << false;
    QTest::newRow("other-video") << QStringLiteral("video/webm") << KisAnimationRenderingOptions::RENDER_VIDEO_ONLY
                                 << false;
}

void KisAnimationRenderDimensionsTest::videoFormatAndRenderModeDetermineRequirement()
{
    QFETCH(QString, videoType);
    QFETCH(KisAnimationRenderingOptions::RenderMode, renderMode);
    QFETCH(bool, expected);

    QCOMPARE(KisAnimationRender::mustHaveEvenDimensions(videoType, renderMode), expected);
}

QTEST_GUILESS_MAIN(KisAnimationRenderDimensionsTest)

#include "kis_animation_render_dimensions_test.moc"
