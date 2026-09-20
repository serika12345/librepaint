/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisDlgImportVideoAnimation.h"

#include <QTest>

class KisVideoFrameImportContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sequentialFramesKeepSequentialPlacement();
    void sceneFilteredFramesRequestTimestampPlacement();
};

void KisVideoFrameImportContractTest::sequentialFramesKeepSequentialPlacement()
{
    // Consumer: people importing every extracted video frame into an animation.
    // Operation: the video importer returns sequentially rendered frame files.
    // Observable result: the caller observes frames and keeps sequential placement.
    // Failure impact: imported animation frames are placed at incorrect timeline positions.
    RenderedFrames frames;
    frames.renderedFrameFiles = {QStringLiteral("output_0001.png"),
                                 QStringLiteral("output_0002.png")};

    QVERIFY(!frames.isEmpty());
    QCOMPARE(frames.size(), size_t(2));
    QVERIFY(!frames.framesNeedRelocation());
}

void KisVideoFrameImportContractTest::sceneFilteredFramesRequestTimestampPlacement()
{
    // Consumer: people importing video with duplicate-frame filtering enabled.
    // Operation: the video importer returns surviving frames with their source timestamps.
    // Observable result: the caller observes that timestamp-based placement is required.
    // Failure impact: filtered frames are compressed into consecutive timeline positions.
    RenderedFrames frames;
    frames.renderedFrameFiles = {QStringLiteral("output_0001.png"),
                                 QStringLiteral("output_0002.png")};
    frames.renderedFrameTargetTimes = {0, 5};

    QVERIFY(!frames.isEmpty());
    QCOMPARE(frames.size(), size_t(2));
    QVERIFY(frames.framesNeedRelocation());
}

QTEST_APPLESS_MAIN(KisVideoFrameImportContractTest)

#include "KisVideoFrameImportContractTest.moc"
