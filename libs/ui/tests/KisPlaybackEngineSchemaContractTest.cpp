/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <animation/KisPlaybackEngine.h>
#include <animation/KisPlaybackEngineMLT.h>
#include <canvas/KisCanvasAnimationState.h>

#include <QTest>

#include <utility>

namespace
{


class PlaybackEngineConstructorProbe final : public KisPlaybackEngine
{
public:
    using KisPlaybackEngine::KisPlaybackEngine;

    void seek(int frameIndex, SeekOptionFlags options) override;
    void setMute(bool value) override;
    bool isMute() override;
    bool supportsAudio() override;
    bool supportsVariablePlaybackSpeed() override;
    PlaybackStats playbackStatistics() const override;

protected:
    void setCanvas(KoCanvasBase *canvas) override;
    void unsetCanvas() override;
};
} // namespace

class KisPlaybackEngineSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void playbackStatisticsValueSchemaRemainsStable();
};

void KisPlaybackEngineSchemaContractTest::playbackStatisticsValueSchemaRemainsStable()
{
    using PlaybackStats = KisPlaybackEngine::PlaybackStats;


    PlaybackStats original;
    QCOMPARE(original.expectedFps, 0.0);
    QCOMPARE(original.realFps, 0.0);
    QCOMPARE(original.droppedFramesPortion, 0.0);

    PlaybackStats copy = original;
    copy.realFps = 24.0;
    QCOMPARE(original.realFps, 0.0);
    QCOMPARE(copy.realFps, 24.0);
}

QTEST_GUILESS_MAIN(KisPlaybackEngineSchemaContractTest)

#include "KisPlaybackEngineSchemaContractTest.moc"
