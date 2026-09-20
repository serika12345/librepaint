/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <animation/KisPlaybackEngineQT.h>

#include <QSignalSpy>
#include <QTest>

class KisPlaybackEngineContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dropFramesToggleSynchronizesStateAndSignal();
};

void KisPlaybackEngineContractTest::dropFramesToggleSynchronizesStateAndSignal()
{
    /*
     * Consumer: The animation docker's playback controls model.
     * Operation: Changes the user's drop-frames toggle in the playback engine.
     * Observable result: The queried mode and change signal agree, without a duplicate signal for the same mode.
     * Failure impact: The docker can show a different mode from the renderer or repeatedly update its controls.
     */
    KisPlaybackEngineQT engine;
    QSignalSpy modeChangedSpy(&engine, &KisPlaybackEngine::sigDropFramesModeChanged);

    const bool initialMode = engine.dropFrames();
    engine.setDropFramesMode(initialMode);
    QCOMPARE(modeChangedSpy.count(), 0);

    const bool changedMode = !initialMode;
    engine.setDropFramesMode(changedMode);

    QCOMPARE(engine.dropFrames(), changedMode);
    QCOMPARE(modeChangedSpy.count(), 1);
    QCOMPARE(modeChangedSpy.takeFirst().at(0).toBool(), changedMode);

    engine.setDropFramesMode(changedMode);
    QCOMPARE(modeChangedSpy.count(), 0);
}

QTEST_GUILESS_MAIN(KisPlaybackEngineContractTest)

#include "KisPlaybackEngineContractTest.moc"
